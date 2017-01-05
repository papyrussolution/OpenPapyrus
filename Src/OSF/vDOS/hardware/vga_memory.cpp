#include <stdlib.h>
#include "vDos.h"
#include "ttf.h"
#include "mem.h"
#include "vga.h"
#include "paging.h"

__forceinline Bit32u RasterOp(Bit32u input, Bit32u mask)
	{
	if (vga.config.raster_op == 0)													// None
		return (input&mask)|(vga.latch.d&~mask);
	if (vga.config.raster_op == 3)													// XOR
		return (input&mask)^vga.latch.d;
	if (vga.config.raster_op == 2)													// OR
		return (input&mask)|vga.latch.d;
	return (input|~mask)&vga.latch.d;												// AND
	}

__forceinline Bit32u ModeOperation(Bit8u val)
	{
	switch (vga.config.write_mode)
		{
	case 0:
		{
		// Write Mode 0: In this mode, the host data is first rotated as per the Rotate Count field, then the Enable Set/Reset mechanism selects data from this or the Set/Reset field. Then the selected Logical Operation is performed on the resulting data and the data in the latch register. Then the Bit Mask field is used to select which bits come from the resulting data and which come from the latch register. Finally, only the bit planes enabled by the Memory Plane Write Enable field are written to memory. 
//		val = ((val>>vga.config.data_rotate)|(val<<(8-vga.config.data_rotate)));
		Bit32u full = ExpandTable[val];
		full = (full&vga.config.full_not_enable_set_reset)|vga.config.full_enable_and_set_reset; 
		return RasterOp(full, vga.config.full_bit_mask);
		}
	case 1:
		// Write Mode 1: In this mode, data is transferred directly from the 32 bit latch register to display memory, affected only by the Memory Plane Write Enable field. The host data is not used in this mode. 
		return vga.latch.d;
	case 2:
		//Write Mode 2: In this mode, the bits 3-0 of the host data are replicated across all 8 bits of their respective planes. Then the selected Logical Operation is performed on the resulting data and the data in the latch register. Then the Bit Mask field is used to select which bits come from the resulting data and which come from the latch register. Finally, only the bit planes enabled by the Memory Plane Write Enable field are written to memory. 
		return RasterOp(FillTable[val&0xF], vga.config.full_bit_mask);
	case 3:
		// Write Mode 3: In this mode, the data in the Set/Reset field is used as if the Enable Set/Reset field were set to 1111b. Then the host data is first rotated as per the Rotate Count field, then logical ANDed with the value of the Bit Mask field. The resulting value is used on the data obtained from the Set/Reset field in the same way that the Bit Mask field would ordinarily be used. to select which bits come from the expansion of the Set/Reset field and which come from the latch register. Finally, only the bit planes enabled by the Memory Plane Write Enable field are written to memory.
		val = ((val>>vga.config.data_rotate)|(val<<(8-vga.config.data_rotate)));
		return RasterOp(vga.config.full_set_reset, ExpandTable[val] & vga.config.full_bit_mask);
	default:
		return 0;
		}
	}

#define VGA_PAGE_A0		(0xA0000/MEM_PAGESIZE)
#define VGA_PAGE_B0		(0xB0000/MEM_PAGESIZE)
#define VGA_PAGE_B8		(0xB8000/MEM_PAGESIZE)

class VGA_UnchainedEGA_Handler : public PageHandler
	{
protected:
	Bit8u readHandler(PhysPt start)
		{
		vga.latch.d = ((Bit32u*)vga.memlinear)[start];
		if (vga.config.read_mode == 0)
			return (vga.latch.b[vga.config.read_map_select]);
		VGA_Latch templatch;
		templatch.d = (vga.latch.d&FillTable[vga.config.color_dont_care])^FillTable[vga.config.color_compare&vga.config.color_dont_care];
		return (Bit8u)~(templatch.b[0]|templatch.b[1]|templatch.b[2]|templatch.b[3]);
		}
	void writeHandler(PhysPt start, Bit8u val)
		{
		// Update video memory and the pixel buffer
		Bit32u pixels = ((Bit32u*)vga.memlinear)[start];
		pixels &= vga.config.full_not_map_mask;
		pixels |= (ModeOperation(val)&vga.config.full_map_mask);
		((Bit32u*)vga.memlinear)[start] = pixels;
		Bit32u end = start<<3;
//		Bit8u * write_pixels = &vga.fastmem[start<<3];
		Bit8u * write_pixels = &vga.fastmem[end];
		int x = end%(CurMode->swidth);												// Register changed area
		int y = end/(CurMode->swidth);
		if (y < (int)CurMode->sheight)												// Restrict it to the displayed surface
			{
			if (x < render.cache.minX)
				render.cache.minX = x;
			if (y < render.cache.minY)
				render.cache.minY = y;
			}
		end += 7;
		x = end%(CurMode->swidth);
		y = end/(CurMode->swidth);
		if (y < (int)CurMode->sheight)
			{
			if (x > render.cache.maxX)
				render.cache.maxX = x;
			if (y > render.cache.maxY)
				render.cache.maxY = y;
			}
		VGA_Latch temp1, temp2;
		temp1.d = (pixels>>4)&0x0f0f0f0f;
		Bit32u colors0_3 = Expand16Table[0][temp1.b[0]]|Expand16Table[1][temp1.b[1]]|Expand16Table[2][temp1.b[2]]|Expand16Table[3][temp1.b[3]];
		*(Bit32u *)write_pixels = colors0_3;
		temp2.d = pixels&0x0f0f0f0f;
		if (temp2.d == temp1.d)
			*(Bit32u *)(write_pixels+4) = colors0_3;
		else
			{
			Bit32u colors4_7 = Expand16Table[0][temp2.b[0]]|Expand16Table[1][temp2.b[1]]|Expand16Table[2][temp2.b[2]]|Expand16Table[3][temp2.b[3]];
			*(Bit32u *)(write_pixels+4) = colors4_7;
			}
		}

public:
	VGA_UnchainedEGA_Handler()
		{
		flags = 0;
		}
	Bit8u readb(PhysPt addr)
		{
		return readHandler(addr-0xa0000);
		}
	Bit16u readw(PhysPt addr)
		{
		return (readHandler(addr-0xa0000)|(readHandler(addr-0xa0000+1)<<8));
		}
	Bit32u readd(PhysPt addr)
		{
		return (readHandler(addr-0xa0000))|(readHandler(addr-0xa0000+1)<<8)|(readHandler(addr-0xa0000+2)<<16)|(readHandler(addr-0xa0000+3)<<24);
		}
	void writeb(PhysPt addr, Bit8u val)
		{
		writeHandler(addr-0xa0000, (Bit8u)val);
		}
	void writew(PhysPt addr, Bit16u val)
		{
		addr -= 0xa0000;
		for (int i = 0; i < 2; i++)
			{
			writeHandler(addr, (Bit8u)val);
			val >>= 8;
			}
		}
	void writed(PhysPt addr, Bit32u val)
		{
		addr -= 0xa0000;
		for (int i = 0; i < 4; i++)
			{
			writeHandler(addr, (Bit8u)val);
			val >>= 8;
			}
		}
	};


static VGA_UnchainedEGA_Handler	hUega;

void VGA_SetupHandlers(void)
	{
	if (!vga.memlinear)
		{
		Bit32u vga_allocsize = 256*1024;											// Keep lower limit at 256KB
		//	vga_allocsize += 4096*4;												// We reserve an extra scan line (max S3 scanline 4096)

		vga.memlinear = (Bit8u*)malloc(vga_allocsize);
		memset(vga.memlinear, 0, vga_allocsize);

		vga.fastmem = (Bit8u*)malloc((vga.vmemsize<<1)+4096);
		render.cache.maxX = render.cache.maxY = -1;
		render.cache.minX = render.cache.minY = 10000;
		}
	MEM_ResetPageHandler(VGA_PAGE_A0, 128*1024/MEM_PAGESIZE);
	switch ((vga.gfx.miscellaneous>>2)&3)
		{
	case 0:
		MEM_SetPageHandler(VGA_PAGE_A0, 128*1024/MEM_PAGESIZE, &hUega);
		break;
	case 1:
		MEM_SetPageHandler(VGA_PAGE_A0, 64*1024/MEM_PAGESIZE, &hUega);
		break;
		}
	}
