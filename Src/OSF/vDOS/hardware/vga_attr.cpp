#include "vDos.h"
#include "inout.h"
#include "vga.h"

#define attr(blah) vga.attr.blah

void VGA_ATTR_SetPalette(Bit8u index, Bit8u val)
	{
	vga.attr.palette[index] = val;
	if (vga.attr.mode_control & 0x80)
		val = (val&0xf) | (vga.attr.color_select << 4);
	val &= 63;
	val |= (vga.attr.color_select & 0xc) << 4;
	VGA_DAC_CombineColor(index, val);
	}

Bit8u read_p3c0(Bitu /*port*/)
	{
	Bitu retval = attr(index) & 0x1f;
	return retval;
	}
 
void write_p3c0(Bitu /*port*/,Bitu val)
	{
	if (!vga.attrindex)
		{
		attr(index) = val & 0x1F;
		vga.attrindex = true;
		/* 
			0-4	Address of data register to write to port 3C0h or read from port 3C1h
		*/
		return;
		}
	vga.attrindex = false;
	// Palette
	if (attr(index) < 0x10)
		{
		VGA_ATTR_SetPalette(attr(index), (Bit8u)val);
		/*
			0-5	Index into the 256 color DAC table. May be modified by 3C0h index
			10h and 14h.
		*/
		return;
		}
	switch (attr(index))
		{
	case 0x10:
		{																			// Mode control register
		Bitu difference = attr(mode_control)^val;
		attr(mode_control) = (Bit8u)val;
		if (difference & 0x80)
			for (Bit8u i = 0; i < 0x10; i++)
				VGA_ATTR_SetPalette(i, vga.attr.palette[i]);
		if (difference & 0x41)
			VGA_DetermineMode();
		/*
			0	Graphics mode if set, Alphanumeric mode else.
			1	Monochrome mode if set, color mode else.
			2	9-bit wide characters if set.
				The 9th bit of characters C0h-DFh will be the same as
				the 8th bit. Otherwise it will be the background color.
			3	If set Attribute bit 7 is blinking, else high intensity.
			5	If set the PEL panning register (3C0h index 13h) is temporarily set
				to 0 from when the line compare causes a wrap around until the next
				vertical retrace when the register is automatically reloaded with
				the old value, else the PEL panning register ignores line compares.
			6	If set pixels are 8 bits wide. Used in 256 color modes.
			7	If set bit 4-5 of the index into the DAC table are taken from port
				3C0h index 14h bit 0-1, else the bits in the palette register are
				used.
		*/
		break;
		}
	case 0x11:																		// Overscan color register
		break;
	case 0x12:																		// Color plane enable register
		// Why disable colour planes?
		attr(color_plane_enable) = (Bit8u)val;
		/* 
			0	Bit plane 0 is enabled if set.
			1	Bit plane 1 is enabled if set.
			2	Bit plane 2 is enabled if set.
			3	Bit plane 3 is enabled if set.
			4-5	Video Status MUX. Diagnostics use only.
				Two attribute bits appear on bits 4 and 5 of the Input Status
				Register 1 (3dAh). 0: Bit 2/0, 1: Bit 5/4, 2: bit 3/1, 3: bit 7/6
		*/
		break;
	case 0x13:																		// Horizontal PEL panning register, we don't do this
		attr(horizontal_pel_panning) = val & 0xF;
		/*
			0-3	Indicates number of pixels to shift the display left
				Value  9bit textmode   256color mode   Other modes
				0          1               0              0
				1          2              n/a             1
				2          3               1              2
				3          4              n/a             3
				4          5               2              4
				5          6              n/a             5
				6          7               3              6
				7          8              n/a             7
				8          0              n/a            n/a
		*/
		break;
	case 0x14:																		// Color select register
		if (attr(color_select) ^ val)
			{
			attr(color_select) = (Bit8u)val;
			for (Bit8u i = 0; i < 0x10; i++)
				VGA_ATTR_SetPalette(i, vga.attr.palette[i]);
			}
		/*
			0-1	If 3C0h index 10h bit 7 is set these 2 bits are used as bits 4-5 of
				the index into the DAC table.
			2-3	These 2 bits are used as bit 6-7 of the index into the DAC table
				except in 256 color mode.
				Note: this register does not affect 256 color modes.
		*/
		break;
		}
	}

Bit8u read_p3c1(Bitu /*port*/)
	{
	if (attr(index) < 0x10)															// Palette
		return attr(palette[attr(index)]);
	switch (attr(index))
		{
	case 0x10:																		// Mode control register
		return attr(mode_control);
//	case 0x11:																		// Overscan color register
//		return 0;
	case 0x12:																		// Color plane enable register
		return attr(color_plane_enable);
	case 0x13:																		// Horizontal PEL panning register
		return attr(horizontal_pel_panning);
	case 0x14:																		// Color select register
		return attr(color_select);
	}
	return 0;
}

void VGA_SetupAttr(void)
	{
	IO_RegisterWriteHandler(0x3c0, write_p3c0);
	IO_RegisterReadHandler(0x3c0, read_p3c0);
	IO_RegisterReadHandler(0x3c1, read_p3c1);
	}
