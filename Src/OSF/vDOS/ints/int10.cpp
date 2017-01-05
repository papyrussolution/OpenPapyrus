#include "vDos.h"
#include "mem.h"
#include "callback.h"
#include "regs.h"
#include "inout.h"
#include "int10.h"
#include "bios.h"

Int10Data int10;

static Bitu INT10_Handler(void)
	{
	switch (reg_ah)
		{
	case 0x00:																		// Set videomode
		INT10_SetVideoMode(reg_al);
		reg_al = Mem_aLodsb(BIOS_VIDEO_MODE);
		break;
	case 0x01:																		// Set textmode cursor shape
		INT10_SetCursorShape(reg_ch, reg_cl);
		break;
	case 0x02:																		// Set cursor pos
		INT10_SetCursorPos(reg_dh, reg_dl, reg_bh);
		break;
	case 0x03:																		// Get cursor pos and shape
		idleCount++;
		reg_dl = CURSOR_POS_COL(reg_bh);
		reg_dh = CURSOR_POS_ROW(reg_bh);
		reg_cx = Mem_aLodsw(BIOS_CURSOR_SHAPE);
		break;
	case 0x04:																		// Read light pen pos
		reg_ax = 0;																	// Light pen is not supported
		break;
	case 0x05:																		// Set active page
		INT10_SetActivePage(reg_al);
		break;	
	case 0x06:																		// Scroll up
		INT10_ScrollWindow(reg_ch, reg_cl, reg_dh, reg_dl, -reg_al, reg_bh, 0xFF);
		break;
	case 0x07:																		// Scroll down
		INT10_ScrollWindow(reg_ch, reg_cl, reg_dh, reg_dl, reg_al, reg_bh, 0xFF);
		break;
	case 0x08:																		// Read character & attribute at cursor
		INT10_ReadCharAttr(&reg_ax, reg_bh);
		break;						
	case 0x09:																		// Write character & attribute at cursor CX times
		INT10_WriteChar(reg_al, Mem_aLodsb(BIOS_VIDEO_MODE) == 0x11 ? (reg_bl&0x80)|0x3f : reg_bl, reg_bh, reg_cx, true);
		break;
	case 0x0A:																		// Write character at cursor CX times
		INT10_WriteChar(reg_al, reg_bl, reg_bh, reg_cx, false);
		break;
	case 0x0B:																		// Set background/border colour & palette
		break;
	case 0x0C:																		// Write graphics pixel
		INT10_PutPixel(reg_cx, reg_dx, reg_bh, reg_al);
		break;
	case 0x0D:																		// Read graphics pixel
		INT10_GetPixel(reg_cx, reg_dx, reg_bh, &reg_al);
		break;
	case 0x0E:																		// Teletype outPut
		INT10_TeletypeOutput(reg_al, reg_bl);
		break;
	case 0x0F:																		// Get videomode
		reg_bh = Mem_aLodsb(BIOS_CURRENT_SCREEN_PAGE);
		reg_al = Mem_aLodsb(BIOS_VIDEO_MODE)|(Mem_aLodsb(BIOS_VIDEO_INFO_0)&0x80);
		reg_ah = (Bit8u)Mem_aLodsw(BIOS_SCREEN_COLUMNS);
		break;					
	case 0x10:																		// Palette functions
		switch (reg_al)
			{
		case 0x00:																	// Set single palette register
			INT10_SetSinglePaletteRegister(reg_bl, reg_bh);
			break;
		case 0x01:																	// Set border (overscan) color
			break;
		case 0x02:																	// Set all palette registers
			INT10_SetAllPaletteRegisters(SegPhys(es)+reg_dx);
			break;
		case 0x03:																	// Toggle intensity/blinking bit
			break;
		case 0x07:																	// Get single palette register
			INT10_GetSinglePaletteRegister(reg_bl, &reg_bh);
			break;
		case 0x08:																	// Read overscan (border color) register
			reg_bh = 0;
			break;
		case 0x09:																	// Read all palette registers and overscan register
			INT10_GetAllPaletteRegisters(SegPhys(es)+reg_dx);
			break;
		case 0x10:																	// Set individual dac register
			INT10_SetSingleDACRegister(reg_bl, reg_dh, reg_ch, reg_cl);
			break;
		case 0x12:																	// Set block of dac registers
			INT10_SetDACBlock(reg_bx, reg_cx, SegPhys(es)+reg_dx);
			break;
		case 0x13:																	// Select video dac color page
			INT10_SelectDACPage(reg_bl, reg_bh);
			break;
		case 0x15:																	// Get individual dac register
			INT10_GetSingleDACRegister(reg_bl, &reg_dh, &reg_ch, &reg_cl);
			break;
		case 0x17:																	// Get block of dac register
			INT10_GetDACBlock(reg_bx, reg_cx, SegPhys(es)+reg_dx);
			break;
		case 0x18:																	// Undocumented - Set pel mask
			INT10_SetPelMask(reg_bl);
			break;
		case 0x19:																	// Undocumented - Get pel mask
			INT10_GetPelMask(reg_bl);
			reg_bh = 0;	// bx for get mask
			break;
		case 0x1A:																	// Get video dac color page
			INT10_GetDACPage(&reg_bl, &reg_bh);
			break;
		default:
			break;
			}
		break;
	case 0x11:																		// Character generator functions
		switch (reg_al)
			{
		// Textmode calls
		case 0x00:																	// Load user font
		case 0x10:
		case 0x01:																	// Load 8x14 font
		case 0x11:
		case 0x02:																	// Load 8x8 font
		case 0x12:
		case 0x04:																	// Load 8x16 font
		case 0x14:
			break;
		case 0x03:																	// Set Block Specifier
			IO_WriteB(0x3c4, 0x3);
			IO_WriteB(0x3c5, reg_bl);
			break;
		case 0x20:																	// Set User 8x8 Graphics characters
			RealSetVec(0x1f, SegOff2dWord(SegValue(es), reg_bp));
			break;
		case 0x21:																	// Set user graphics characters
			RealSetVec(0x43, SegOff2dWord(SegValue(es), reg_bp));
			Mem_aStosw(BIOS_FONT_HEIGHT, reg_cx);
			goto graphics_chars;
		case 0x22:																	// Rom 8x14 set
			RealSetVec(0x43, int10.rom.font_14);
			Mem_aStosw(BIOS_FONT_HEIGHT, 14);
			goto graphics_chars;
		case 0x23:																	// Rom 8x8 double dot set
			RealSetVec(0x43, int10.rom.font_8_first);
			Mem_aStosw(BIOS_FONT_HEIGHT, 8);
			goto graphics_chars;
		case 0x24:																	// Rom 8x16 set
			RealSetVec(0x43, int10.rom.font_16);
			Mem_aStosw(BIOS_FONT_HEIGHT, 16);
graphics_chars:
			switch (reg_bl)
				{
			case 0:
				Mem_aStosb(BIOS_ROWS_ON_SCREEN_MINUS_1, reg_dl-1);
				break;
			case 1:
				Mem_aStosb(BIOS_ROWS_ON_SCREEN_MINUS_1, 13);
				break;
			case 3:
				Mem_aStosb(BIOS_ROWS_ON_SCREEN_MINUS_1, 42);
				break;
			case 2:
			default:
				Mem_aStosb(BIOS_ROWS_ON_SCREEN_MINUS_1, 24);
				break;
				}
			break;
		// General
		case 0x30:																	// Get Font Information
			switch (reg_bh)
				{
			case 0:																	// Interupt 0x1f vector
				{
				RealPt int_1f = RealGetVec(0x1f);
				SegSet16(es, RealSeg(int_1f));
				reg_bp = RealOff(int_1f);
				}
				break;
			case 1:																	// Interupt 0x43 vector
				{
				RealPt int_43 = RealGetVec(0x43);
				SegSet16(es, RealSeg(int_43));
				reg_bp = RealOff(int_43);
				}
				break;
			case 2:																	// Font 8x14
				SegSet16(es, RealSeg(int10.rom.font_14));
				reg_bp = RealOff(int10.rom.font_14);
				break;
			case 3:																	// Font 8x8 first 128
				SegSet16(es, RealSeg(int10.rom.font_8_first));
				reg_bp = RealOff(int10.rom.font_8_first);
				break;
			case 4:																	// Font 8x8 second 128
				SegSet16(es, RealSeg(int10.rom.font_8_second));
				reg_bp = RealOff(int10.rom.font_8_second);
				break;
			case 5:																	// Alpha alternate 9x14
				SegSet16(es, RealSeg(int10.rom.font_14_alternate));
				reg_bp = RealOff(int10.rom.font_14_alternate);
				break;
			case 6:																	// Font 8x16
				SegSet16(es, RealSeg(int10.rom.font_16));
				reg_bp = RealOff(int10.rom.font_16);
				break;
			case 7:																	// Alpha alternate 9x16
				SegSet16(es, RealSeg(int10.rom.font_16_alternate));
				reg_bp = RealOff(int10.rom.font_16_alternate);
				break;
			default:
				break;
				}
			if ((reg_bh <= 7))
				{
				reg_cx = Mem_aLodsw(BIOS_FONT_HEIGHT);
				reg_dl = Mem_aLodsb(BIOS_ROWS_ON_SCREEN_MINUS_1);
				}
			break;
		default:
			break;
			}
		break;
	case 0x12:																		// Alternate function select
		switch (reg_bl)
			{
		case 0x10:																	// Get EGA information
			reg_bh = (Mem_aLodsw(BIOS_VIDEO_PORT) == 0x3B4);	
			reg_bl = 3;																// 256 kb
			reg_cl = Mem_aLodsb(BIOS_VIDEO_INFO_1)&0x0F;
			reg_ch = Mem_aLodsb(BIOS_VIDEO_INFO_1)>>4;
			break;
		case 0x20:																	// Set alternate printscreen
		case 0x30:																	// Select vertical resolution
		case 0x31:																	// Palette loading on modeset
		case 0x32:																	// Video adressing
		case 0x33:																	// Switch gray-scale summing
			break;	
		case 0x34:																	// Alternate function select (VGA) - cursor emulation
			{   
			if (reg_al > 1)															// Bit 0: 0=enable, 1=disable
				{
				reg_al = 0;
				break;
				}
			Bit8u temp = Mem_aLodsb(BIOS_VIDEO_INFO_0) & 0xfe;
			Mem_aStosb(BIOS_VIDEO_INFO_0, temp|reg_al);
			reg_al = 0x12;
			break;	
			}		
		case 0x35:
		case 0x36:																	// VGA refresh control
			break;
		default:
			break;
			}
		break;
	case 0x13:																		// Write string
		INT10_WriteString(reg_dh, reg_dl, reg_al, reg_bl, SegPhys(es)+reg_bp, reg_cx, reg_bh);
		break;
	case 0x1A:																		// Display Combination
		if (reg_al == 0)															// Get DCC
			{
			reg_bx = 8;																// Only active  display code (VGA)
			reg_ax = 0x1a;															// High part destroyed or zeroed depending on BIOS
			}
		break;
	default:
		break;
		};
	return CBRET_NONE;
	}

static void INT10_Seg40Init(void)
	{
	Mem_aStosb(BIOS_FONT_HEIGHT, 16);												// The default char height
	Mem_aStosb(BIOS_VIDEO_INFO_0, 0x60);											// Clear the screen 
	Mem_aStosb(BIOS_VIDEO_INFO_1, 0xF9);											// Set the basic screen we have
	Mem_aStosb(BIOS_VIDEO_INFO_2, 0x51);											// Set the basic modeset options
	Mem_aStosb(BIOS_VDU_CONTROL, 0x09);												// Set the default MSR
	}

static void INT10_InitVGA(void)
	{
	IO_WriteB(0x3c2, 0xc3);															// Switch to color mode and enable CPU access 480 lines
	IO_WriteB(0x3c4, 0x04);															// More than 64k
	IO_WriteB(0x3c5, 0x02);
	}

void INT10_Init()
	{
	INT10_InitVGA();
	CALLBACK_Install(0x10, &INT10_Handler, CB_IRET_STI);							// Int 10 video
	INT10_SetupRomMemory();															// Init the 0x40 segment and init the datastructures in the the video rom area
	INT10_Seg40Init();
	INT10_SetVideoMode(initialvMode);
	Mem_aStosw(BIOS_CONFIGURATION, 0x24+((initialvMode&4)<<3));						// Startup 80x25 color or mono, PS2 mouse
	}
