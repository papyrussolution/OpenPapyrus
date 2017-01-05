#include <stdlib.h>
#include "vDos.h"
#include "inout.h"
#include "vga.h"
#include "cpu.h"
#include "video.h"


#define crtc(blah) vga.crtc.blah

void vga_write_p3d4(Bitu port, Bitu val)
	{
	crtc(index) = val;
	}

Bit8u vga_read_p3d4(Bitu port)
	{
	return crtc(index);
	}

void vga_write_p3d5(Bitu port, Bitu val)
	{
	switch(crtc(index))
		{
	case 0x00:																						// Horizontal total register
		crtc(horizontal_total) = val;																// 0-7  horizontal total character clocks-5
		break;
	case 0x01:																						// Horizontal display end register
		if (val != crtc(horizontal_display_end))
			{
			crtc(horizontal_display_end) = val;														// 0-7  number of character clocks displayed -1
			VGA_StartResize();
			}
		break;
	case 0x02:																						// Start horizontal blanking register
		crtc(start_horizontal_blanking) = val;														// 0-7  the count at which horizontal blanking starts
		break;
	case 0x03:																						// End horizontal blanking register
		crtc(end_horizontal_blanking) = val;
		/*
			0-4	Horizontal Blanking ends when the last 6 bits of the character
				counter equals this field. Bit 5 is at 3d4h index 5 bit 7.
			5-6	Number of character clocks to delay start of display after Horizontal
				Total has been reached.
			7	Access to Vertical Retrace registers if set. If clear reads to 3d4h
				index 10h and 11h access the Lightpen read back registers ??
		*/
		break;
	case 0x04:																						// Start horizontal retrace register
		crtc(start_horizontal_retrace) = val;														// 0-7  horizontal retrace starts when the character counter reaches this value.
		break;
	case 0x05:																						// End horizontal retrace register
		crtc(end_horizontal_retrace) = val;
		/*
			0-4	Horizontal Retrace ends when the last 5 bits of the character counter
				equals this value.
			5-6	Number of character clocks to delay start of display after Horizontal
				Retrace.
			7	bit 5 of the End Horizontal Blanking count (See 3d4h index 3 bit 0-4)
		*/	
		break;
	case 0x06:																						// Vertical total register
		crtc(vertical_total) = val;
		/*	0-7	Lower 8 bits of the Vertical Total. Bit 8 is found in 3d4h index 7
				bit 0. Bit 9 is found in 3d4h index 7 bit 5.
			Note: For the VGA this value is the number of scan lines in the display -2.
		*/
		break;
	case 0x07:																						// Overflow register
		crtc(overflow) = val;
		break;
	case 0x08:																						// Preset row scan register
		crtc(preset_row_scan) = val;
//		vga.config.hlines_skip = val&31;
		/*
			0-4	Number of lines we have scrolled down in the first character row.
				Provides Smooth Vertical Scrolling.b
			5-6	Number of bytes to skip at the start of scanline. Provides Smooth
				Horizontal Scrolling together with the Horizontal Panning Register
				(3C0h index 13h).
		*/
		break;
	case 0x09:																						// Maximum scan line register
		{
		Bit8u old = crtc(maximum_scan_line);
		crtc(maximum_scan_line) = val;
		if ((old^val)&0x20)
			VGA_StartResize();
//		vga.draw.address_line_total = (val&0x1f)+1;
//		if (val&0x80)
//			vga.draw.address_line_total *= 2;
		/*
			0-4	Number of scan lines in a character row -1. In graphics modes this is
				the number of times (-1) the line is displayed before passing on to
				the next line (0: normal, 1: double, 2: triple...).
				This is independent of bit 7, except in CGA modes which seems to
				require this field to be 1 and bit 7 to be set to work.
			5	Bit 9 of Start Vertical Blanking
			6	Bit 9 of Line Compare Register
			7	Doubles each scan line if set. I.e. displays 200 lines on a 400 display.
		*/
		break;
		}
	case 0x0A:																						// Cursor start register
		crtc(cursor_start) = val;
		vga.draw.cursor.sline = val&0x1f;
		vga.draw.cursor.enabled = !(val&0x20);
		/*
			0-4	First scanline of cursor within character.
			5	Turns Cursor off if set
		*/
		break;
	case 0x0B:																						// Cursor end register
		crtc(cursor_end) = val;
		vga.draw.cursor.eline = val&0x1f;
		/* 
			0-4	Last scanline of cursor within character
			5-6	Delay of cursor data in character clocks.
		*/
		break;
	case 0x0C:																						// Start address high register
		crtc(start_address_high) = val;
		vga.config.display_start = (vga.config.display_start & 0xFF00FF)| (val << 8);				// 0-7  upper 8 bits of the start address of the display buffer
		break;
	case 0x0D:																						// Start address low register
		crtc(start_address_low) = val;
		vga.config.display_start = (vga.config.display_start & 0xFFFF00)| val;						// 0-7	lower 8 bits of the start address of the display buffer
		break;
	case 0x0E:																						// Cursor location high register
		crtc(cursor_location_high) = val;
		vga.config.cursor_start &= 0xff00ff;
		vga.config.cursor_start |= val << 8;														//	0-7  upper 8 bits of the address of the cursor
		break;
	case 0x0F:																						// Cursor location low register
// TODO update cursor on screen
		crtc(cursor_location_low) = val;
		vga.config.cursor_start &= 0xffff00;
		vga.config.cursor_start |= val;																//	0-7  lower 8 bits of the address of the cursor
		break;
	case 0x10:																						// Vertical retrace startr register
		crtc(vertical_retrace_start) = val;
		/*	
			0-7	Lower 8 bits of Vertical Retrace Start. Vertical Retrace starts when
			the line counter reaches this value. Bit 8 is found in 3d4h index 7
			bit 2. Bit 9 is found in 3d4h index 7 bit 7.
		*/
		break;
	case 0x11:																						// Vertical retrace end register
		crtc(vertical_retrace_end) = val;
		/*
			0-3	Vertical Retrace ends when the last 4 bits of the line counter equals
				this value.
			4	if clear Clears pending Vertical Interrupts.
			5	Vertical Interrupts (IRQ 2) disabled if set. Can usually be left
				disabled, but some systems (including PS/2) require it to be enabled.
			6	If set selects 5 refresh cycles per scanline rather than 3.
			7	Disables writing to registers 0-7 if set 3d4h index 7 bit 4 is not
				affected by this bit.
		*/
		break;
	case 0x12:																						// Vertical display end register
		if (val != crtc(vertical_display_end))
			{
			crtc(vertical_display_end) = val;
			VGA_StartResize();
			}
		/*
			0-7	Lower 8 bits of Vertical Display End. The display ends when the line
				counter reaches this value. Bit 8 is found in 3d4h index 7 bit 1.
			Bit 9 is found in 3d4h index 7 bit 6.
		*/
		break;
	case 0x13:																						// Offset register
		crtc(offset) = val;
		vga.config.scan_len &= 0x300;
		vga.config.scan_len |= val;
//		VGA_CheckScanLength();
		/*
			0-7	Number of bytes in a scanline / K. Where K is 2 for byte mode, 4 for
				word mode and 8 for Double Word mode.
		*/
		break;
	case 0x14:																						// Underline location register
		crtc(underline_location) = val;
		// Byte,word,dword mode
		/*
			0-4	Position of underline within Character cell.
			5	If set memory address is only changed every fourth character clock.
			6	Double Word mode addressing if set
		*/
		break;
	case 0x15:																						// Start vertical blank register
		crtc(start_vertical_blanking) = val;
		break;
	case 0x16:																						// End vertical blank register
		crtc(end_vertical_blanking) = val;
		break;
	case 0x17:																						// Mode control register
		crtc(mode_control) = val;
		// Byte,word,dword mode
		// Should we really need to do a determinemode here?
//		VGA_DetermineMode();
		/*
			0	If clear use CGA compatible memory addressing system
				by substituting character row scan counter bit 0 for address bit 13,
				thus creating 2 banks for even and odd scan lines.
			1	If clear use Hercules compatible memory addressing system by
				substituting character row scan counter bit 1 for address bit 14,
				thus creating 4 banks.
			2	If set increase scan line counter only every second line.
			3	If set increase memory address counter only every other character clock.
			5	When in Word Mode bit 15 is rotated to bit 0 if this bit is set else
				bit 13 is rotated into bit 0.
			6	If clear system is in word mode. Addresses are rotated 1 position up
				bringing either bit 13 or 15 into bit 0.
			7	Clearing this bit will reset the display system until the bit is set again.
		*/
		break;
	case 0x18:																						// Line compare register
		crtc(line_compare) = val;
		break;
		}
	}

Bit8u vga_read_p3d5(Bitu port)
	{
	switch (crtc(index))
		{
	case 0x00:																						// Horizontal total register
		return crtc(horizontal_total);
	case 0x01:																						// Horizontal Display End Register
		return crtc(horizontal_display_end);
	case 0x02:																						// Start Horizontal Blanking Register
		return crtc(start_horizontal_blanking);
	case 0x03:																						// End Horizontal Blanking Register
		return crtc(end_horizontal_blanking);
	case 0x04:																						// Start Horizontal Retrace Register
		return crtc(start_horizontal_retrace);
	case 0x05:																						// End Horizontal Retrace Register
		return crtc(end_horizontal_retrace);
	case 0x06:																						// Vertical Total Register
		return crtc(vertical_total);	
	case 0x07:																						// Overflow Register
		return crtc(overflow);
	case 0x08:																						// Preset Row Scan Register
		return crtc(preset_row_scan);
	case 0x09:																						// Maximum Scan Line Register
		return crtc(maximum_scan_line);
	case 0x0A:																						// Cursor Start Register
		return crtc(cursor_start);
	case 0x0B:																						// Cursor End Register
		return crtc(cursor_end);
	case 0x0C:																						// Start Address High Register
		return crtc(start_address_high);
	case 0x0D:																						// Start Address Low Register
		return crtc(start_address_low);
	case 0x0E:																						// Cursor Location High Register
		return crtc(cursor_location_high);
	case 0x0F:																						// Cursor Location Low Register
		return crtc(cursor_location_low);
	case 0x10:																						// Vertical Retrace Start Register
		return crtc(vertical_retrace_start);
	case 0x11:																						// Vertical Retrace End Register
		return crtc(vertical_retrace_end);
	case 0x12:																						// Vertical Display End Register
		return crtc(vertical_display_end);
	case 0x13:																						// Offset register
		return crtc(offset);
	case 0x14:																						// Underline Location Register
		return crtc(underline_location);
	case 0x15:																						// Start Vertical Blank Register
		return crtc(start_vertical_blanking);
	case 0x16:																						// End Vertical Blank Register
		return crtc(end_vertical_blanking);
	case 0x17:																						// Mode Control Register
		return crtc(mode_control);
	case 0x18:																						// Line Compare Register
		return crtc(line_compare);
	default:
		return 0;
		}
	}
