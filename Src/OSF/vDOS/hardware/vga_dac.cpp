#include "vDos.h"
#include "inout.h"
#include "vga.h"
#include "ttf.h"

enum {DAC_READ, DAC_WRITE};

static void VGA_DAC_SendColor(Bitu index, Bitu src)
	{
	Bit8u red = vga.dac.rgb[src].red;
	Bit8u green = vga.dac.rgb[src].green;
	Bit8u blue = vga.dac.rgb[src].blue;
	// Set entry in 16bit output lookup table
//	vga.dac.xlat16[index] = ((blue>>1)&0x1f) | (((green)&0x3f)<<5) | (((red>>1)&0x1f) << 11);
	render.pal[index].b =  (red << 2) | (red >> 4);
	render.pal[index].g = (green << 2) | (green >> 4);
	render.pal[index].r = (blue << 2) | (blue >> 4);

	}

static void VGA_DAC_UpdateColor(Bitu index)
	{
	Bitu maskIndex = index & vga.dac.pel_mask;
	VGA_DAC_SendColor(index, maskIndex);
	}

static void write_p3c6(Bitu port, Bitu val)
	{
	if (vga.dac.hidac_counter > 3)
		{
		vga.dac.hidac_counter = 0;
		VGA_StartResize();
		return;
		}
	if (vga.dac.pel_mask != val)
		{
		vga.dac.pel_mask = val;
		for (Bitu i = 0; i < 256; i++) 
			VGA_DAC_UpdateColor(i);
		}
	}

static Bit8u read_p3c6(Bitu port)
	{
	vga.dac.hidac_counter++;
	return vga.dac.pel_mask;
	}

static void write_p3c7(Bitu port, Bitu val)
	{
	vga.dac.hidac_counter = 0;
	vga.dac.read_index = val;
	vga.dac.pel_index = 0;
	vga.dac.state = DAC_READ;
	vga.dac.write_index = val + 1;
	}

static Bit8u read_p3c7(Bitu port)
	{
	vga.dac.hidac_counter = 0;
	if (vga.dac.state == DAC_READ)
		return 0x3;
	else
		return 0x0;
}

static void write_p3c8(Bitu port,Bitu val)
	{
	vga.dac.hidac_counter = 0;
	vga.dac.write_index = val;
	vga.dac.pel_index = 0;
	vga.dac.state = DAC_WRITE;
	}

static Bit8u read_p3c8(Bitu port)
	{
	vga.dac.hidac_counter = 0;
	return vga.dac.write_index;
	}

static void write_p3c9(Bitu port, Bitu val)
	{
	vga.dac.hidac_counter = 0;
	val &= 0x3f;
	switch (vga.dac.pel_index)
		{
	case 0:
		vga.dac.rgb[vga.dac.write_index].red = val;
		vga.dac.pel_index = 1;
		break;
	case 1:
		vga.dac.rgb[vga.dac.write_index].green = val;
		vga.dac.pel_index = 2;
		break;
	case 2:
		vga.dac.rgb[vga.dac.write_index].blue = val;
		// Check for attributes and DAC entry link
		for (Bitu i = 0; i < 16; i++)
			if (vga.dac.combine[i] == vga.dac.write_index)
				VGA_DAC_SendColor(i, vga.dac.write_index);
		vga.dac.write_index++;
		vga.dac.pel_index = 0;
		break;
		}
	}

static Bit8u read_p3c9(Bitu port)
	{
	vga.dac.hidac_counter = 0;
	Bit8u ret;
	switch (vga.dac.pel_index)
		{
	case 0:
		ret = vga.dac.rgb[vga.dac.read_index].red;
		vga.dac.pel_index = 1;
		break;
	case 1:
		ret = vga.dac.rgb[vga.dac.read_index].green;
		vga.dac.pel_index = 2;
		break;
	case 2:
		ret = vga.dac.rgb[vga.dac.read_index].blue;
		vga.dac.read_index++;
		vga.dac.pel_index = 0;
//		vga.dac.write_index=vga.dac.read_index+1;//disabled as it breaks wari
		break;
	default:
		ret = 0;
		break;
		}
	return ret;
	}

void VGA_DAC_CombineColor(Bit8u attr, Bit8u pal)
	{
	vga.dac.combine[attr] = pal;
	VGA_DAC_SendColor(attr, pal);
	}

void VGA_SetupDAC(void)
	{
	vga.dac.pel_mask = 0xff;
	vga.dac.pel_index = 0;
	vga.dac.state = DAC_READ;
	vga.dac.read_index = 0;
	vga.dac.write_index = 0;
	vga.dac.hidac_counter = 0;

	// Setup the DAC IO port Handlers
	IO_RegisterWriteHandler(0x3c6, write_p3c6);
	IO_RegisterReadHandler(0x3c6, read_p3c6);
	IO_RegisterWriteHandler(0x3c7, write_p3c7);
	IO_RegisterReadHandler(0x3c7, read_p3c7);
	IO_RegisterWriteHandler(0x3c8, write_p3c8);
	IO_RegisterReadHandler(0x3c8, read_p3c8);
	IO_RegisterWriteHandler(0x3c9, write_p3c9);
	IO_RegisterReadHandler(0x3c9, read_p3c9);
	}
