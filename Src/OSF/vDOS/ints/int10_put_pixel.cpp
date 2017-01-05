#include "vDos.h"
#include "mem.h"
#include "inout.h"
#include "int10.h"
#include "bios.h"

void INT10_PutPixel(Bit16u x, Bit16u y, Bit8u page, Bit8u color)
	{
	if (CurMode->type == M_EGA)
		{
		// Set the correct bitmask for the pixel position
		IO_WriteB(0x3ce, 0x8);
		Bit8u mask = 128>>(x&7);
		IO_WriteB(0x3cf, mask);
		// Set the color to set/reset register
		IO_WriteB(0x3ce, 0);
		IO_WriteB(0x3cf, color);
		// Enable all the set/resets
		IO_WriteB(0x3ce, 1);
		IO_WriteB(0x3cf, 0xf);
		// test for xorring
		if (color & 0x80)
			{
			IO_WriteB(0x3ce, 3);
			IO_WriteB(0x3cf, 0x18);
			}
		// Perhaps also set mode 1 
		// Calculate where the pixel is in video memory
		PhysPt off = 0xa0000+Mem_aLodsw(BIOS_VIDEO_PAGE_SIZE)*page+((y*Mem_aLodsw(BIOS_SCREEN_COLUMNS)*8+x)>>3);
		// Bitmask and set/reset should do the rest
		Mem_Lodsb(off);
		Mem_Stosb(off, 0xff);
		// Restore bitmask
		IO_WriteB(0x3ce, 8);
		IO_WriteB(0x3cf, 0xff);
		IO_WriteB(0x3ce, 1);
		IO_WriteB(0x3cf, 0);
		// Restore write operating if changed
		if (color & 0x80)
			{
			IO_WriteB(0x3ce, 3);
			IO_WriteB(0x3cf, 0);
			}
		}	
	}

void INT10_GetPixel(Bit16u x, Bit16u y, Bit8u page, Bit8u * color)
	{
	if (CurMode->type == M_EGA)
		{
		// Calculate where the pixel is in video memory
		PhysPt off = 0xa0000+Mem_aLodsw(BIOS_VIDEO_PAGE_SIZE)*page+((y*Mem_aLodsw(BIOS_SCREEN_COLUMNS)*8+x)>>3);
		Bitu shift = 7-(x & 7);
		// Set the read map
		*color = 0;
		IO_WriteB(0x3ce, 4);
		IO_WriteB(0x3cf, 0);
		*color |= ((Mem_Lodsb(off)>>shift)&1)<<0;
		IO_WriteB(0x3ce, 4);
		IO_WriteB(0x3cf, 1);
		*color |= ((Mem_Lodsb(off)>>shift)&1)<<1;
		IO_WriteB(0x3ce, 4);
		IO_WriteB(0x3cf, 2);
		*color |= ((Mem_Lodsb(off)>>shift)&1)<<2;
		IO_WriteB(0x3ce, 4);
		IO_WriteB(0x3cf, 3);
		*color |= ((Mem_Lodsb(off)>>shift)&1)<<3;
		}	
	}

