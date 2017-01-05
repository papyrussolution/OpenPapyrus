#include "vga.h"
#include "mem.h"
#include "bios.h"

// VGA registers
#define VGAREG_ACTL_ADDRESS				0x3c0
#define VGAREG_ACTL_WRITE_DATA			0x3c0
#define VGAREG_ACTL_READ_DATA			0x3c1

#define VGAREG_INPUT_STATUS				0x3c2
#define VGAREG_WRITE_MISC_OUTPUT		0x3c2
#define VGAREG_VIDEO_ENABLE				0x3c3
#define VGAREG_SEQU_ADDRESS				0x3c4
#define VGAREG_SEQU_DATA				0x3c5

#define VGAREG_PEL_MASK					0x3c6
#define VGAREG_DAC_STATE				0x3c7
#define VGAREG_DAC_READ_ADDRESS			0x3c7
#define VGAREG_DAC_WRITE_ADDRESS		0x3c8
#define VGAREG_DAC_DATA					0x3c9

#define VGAREG_READ_FEATURE_CTL			0x3ca
#define VGAREG_READ_MISC_OUTPUT			0x3cc

#define VGAREG_GRDC_ADDRESS				0x3ce
#define VGAREG_GRDC_DATA				0x3cf

#define VGAREG_MDA_CRTC_ADDRESS			0x3b4
#define VGAREG_MDA_CRTC_DATA			0x3b5
#define VGAREG_VGA_CRTC_ADDRESS			0x3d4
#define VGAREG_VGA_CRTC_DATA			0x3d5

#define VGAREG_MDA_WRITE_FEATURE_CTL	0x3ba
#define VGAREG_VGA_WRITE_FEATURE_CTL	0x3da
#define VGAREG_ACTL_RESET				0x3da
#define VGAREG_TDY_RESET				0x3da
#define VGAREG_TDY_ADDRESS				0x3da
#define VGAREG_TDY_DATA					0x3de
#define VGAREG_PCJR_DATA				0x3da

#define VGAREG_MDA_MODECTL				0x3b8
#define VGAREG_CGA_MODECTL				0x3d8
#define VGAREG_CGA_PALETTE				0x3d9

#define BIOS_NCOLS Bit16u ncols = Mem_aLodsw(BIOS_SCREEN_COLUMNS);
#define BIOS_NROWS Bit16u nrows = (Bit16u)Mem_aLodsb(BIOS_ROWS_ON_SCREEN_MINUS_1)+1;

extern Bit8u int10_font_08[256*8];
extern Bit8u int10_font_14[256*14];
extern Bit8u int10_font_16[256*16];

typedef struct {
	struct {
		RealPt font_8_first;
		RealPt font_8_second;
		RealPt font_14;
		RealPt font_16;
		RealPt font_14_alternate;
		RealPt font_16_alternate;
		RealPt static_state;
		RealPt video_save_pointers;
		RealPt video_parameter_table;
		RealPt video_save_pointer_table;
		RealPt video_dcc_table;
		RealPt oemstring;
		RealPt vesa_modes;
		Bit16u used;
	} rom;
} Int10Data;

extern Int10Data int10;

static Bit8u CURSOR_POS_COL(Bit8u page)
	{
	return Mem_aLodsb(BIOS_VIDEO_CURSOR_POS+page*2);
	}

static Bit8u CURSOR_POS_ROW(Bit8u page)
	{
	return Mem_aLodsb(BIOS_VIDEO_CURSOR_POS+page*2+1);
	}

bool INT10_SetVideoMode(Bit8u mode);

void INT10_ScrollWindow(Bit8u rul,Bit8u cul,Bit8u rlr,Bit8u clr,Bit8s nlines,Bit8u attr,Bit8u page);

void INT10_SetActivePage(Bit8u page);

void INT10_SetCursorShape(Bit8u first, Bit8u last);
void INT10_SetCursorPos(Bit8u row, Bit8u col, Bit8u page);
void INT10_TeletypeOutput(Bit8u chr, Bit8u attr);
void INT10_TeletypeOutputAttr(Bit8u chr, Bit8u attr, bool useattr);
void INT10_ReadCharAttr(Bit16u * result, Bit8u page);
void INT10_WriteChar(Bit8u chr, Bit8u attr, Bit8u page, Bit16u count, bool showattr);
void INT10_WriteString(Bit8u row, Bit8u col, Bit8u flag, Bit8u attr, PhysPt string, Bit16u count, Bit8u page);

// Graphics Stuff
void INT10_PutPixel(Bit16u x, Bit16u y, Bit8u page, Bit8u color);
void INT10_GetPixel(Bit16u x, Bit16u y, Bit8u page, Bit8u * color);

/* Palette Group */
void INT10_SetSinglePaletteRegister(Bit8u reg, Bit8u val);
void INT10_SetAllPaletteRegisters(PhysPt data);
void INT10_GetSinglePaletteRegister(Bit8u reg,Bit8u * val);
void INT10_GetAllPaletteRegisters(PhysPt data);
void INT10_SetSingleDACRegister(Bit8u index, Bit8u red, Bit8u green, Bit8u blue);
void INT10_GetSingleDACRegister(Bit8u index, Bit8u * red, Bit8u * green, Bit8u * blue);
void INT10_SetDACBlock(Bit16u index, Bit16u count, PhysPt data);
void INT10_GetDACBlock(Bit16u index, Bit16u count, PhysPt data);
void INT10_SelectDACPage(Bit8u function, Bit8u mode);
void INT10_GetDACPage(Bit8u* mode, Bit8u* page);
void INT10_SetPelMask(Bit8u mask);
void INT10_GetPelMask(Bit8u & mask);

// Sub Groups
void INT10_SetupRomMemory(void);

// Video Parameter Tables
Bit16u INT10_SetupVideoParameterTable(PhysPt basepos);
void INT10_SetupBasicVideoParameterTable(void);

void FinishSetMode(bool clearmem);