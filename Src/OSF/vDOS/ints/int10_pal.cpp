#include "vDos.h"
#include "mem.h"
#include "inout.h"
#include "int10.h"

#define ACTL_MAX_REG   0x14

static void ResetACTL(void)
	{
	IO_ReadB(Mem_aLodsw(BIOS_VIDEO_PORT)+6);
	}

void INT10_SetSinglePaletteRegister(Bit8u reg, Bit8u val)
	{
	if (reg <= ACTL_MAX_REG)
		{
		ResetACTL();
		IO_WriteB(VGAREG_ACTL_ADDRESS, reg);
		IO_WriteB(VGAREG_ACTL_WRITE_DATA, val);
		}
	IO_WriteB(VGAREG_ACTL_ADDRESS, 32);												// Enable output and protect palette
	}

void INT10_SetAllPaletteRegisters(PhysPt data)
	{
	ResetACTL();
	for (Bit8u i = 0; i < 0x10; i++)												// First the colors
		{
		IO_WriteB(VGAREG_ACTL_ADDRESS, i);
		IO_WriteB(VGAREG_ACTL_WRITE_DATA, Mem_Lodsb(data));
		data++;
		}
	IO_WriteB(VGAREG_ACTL_ADDRESS, 0x11);											// Then the border
	IO_WriteB(VGAREG_ACTL_WRITE_DATA, Mem_Lodsb(data));
	IO_WriteB(VGAREG_ACTL_ADDRESS, 32);												// Enable output and protect palette
	}

void INT10_GetSinglePaletteRegister(Bit8u reg, Bit8u * val)
	{
	if (reg <= ACTL_MAX_REG)
		{
		ResetACTL();
		IO_WriteB(VGAREG_ACTL_ADDRESS, reg+32);
		*val = IO_ReadB(VGAREG_ACTL_READ_DATA);
		IO_WriteB(VGAREG_ACTL_WRITE_DATA, *val);
		}
	}
/*
void INT10_GetAllPaletteRegisters(PhysPt data)
	{
	ResetACTL();
	for (Bit8u i = 0; i < 0x10; i++)												// First the colors
		{
		IO_WriteB(VGAREG_ACTL_ADDRESS, i);
		Mem_Stosb(data, IO_ReadB(VGAREG_ACTL_READ_DATA));
		ResetACTL();
		data++;
		}
	IO_WriteB(VGAREG_ACTL_ADDRESS, 0x11+32);										// Then the border
	Mem_Stosb(data, IO_ReadB(VGAREG_ACTL_READ_DATA));
	ResetACTL();
	}
*/
void INT10_GetAllPaletteRegisters(PhysPt data)
	{
	Bit8u rByte;
	ResetACTL();
	for (Bit8u i = 0; i < 0x10; i++)												// First the colors
		{
		IO_WriteB(VGAREG_ACTL_ADDRESS, i+32);
		rByte = IO_ReadB(VGAREG_ACTL_READ_DATA);
		Mem_Stosb(data, rByte);
		ResetACTL();
		data++;
		}
	IO_WriteB(VGAREG_ACTL_ADDRESS, 0x11+32);										// Then the border
	rByte = IO_ReadB(VGAREG_ACTL_READ_DATA);
	Mem_Stosb(data, rByte);
	ResetACTL();
	}

void INT10_SetSingleDACRegister(Bit8u index, Bit8u red,Bit8u green, Bit8u blue)
	{
	IO_WriteB(VGAREG_DAC_WRITE_ADDRESS, index);
	IO_WriteB(VGAREG_DAC_DATA, red);
	IO_WriteB(VGAREG_DAC_DATA, green);
	IO_WriteB(VGAREG_DAC_DATA, blue);
	}

void INT10_GetSingleDACRegister(Bit8u index, Bit8u * red, Bit8u * green, Bit8u * blue)
	{
	IO_WriteB(VGAREG_DAC_READ_ADDRESS, index);
	*red = IO_ReadB(VGAREG_DAC_DATA);
	*green = IO_ReadB(VGAREG_DAC_DATA);
	*blue = IO_ReadB(VGAREG_DAC_DATA);
	}

void INT10_SetDACBlock(Bit16u index, Bit16u count, PhysPt data)
	{
 	IO_WriteB(VGAREG_DAC_WRITE_ADDRESS, (Bit8u)index);
	for (;count > 0; count--)
		{
		IO_WriteB(VGAREG_DAC_DATA, Mem_Lodsb(data++));
		IO_WriteB(VGAREG_DAC_DATA, Mem_Lodsb(data++));
		IO_WriteB(VGAREG_DAC_DATA, Mem_Lodsb(data++));
		}
	}

void INT10_GetDACBlock(Bit16u index, Bit16u count, PhysPt data)
	{
 	IO_WriteB(VGAREG_DAC_READ_ADDRESS, (Bit8u)index);
	for (; count > 0; count--)
		{
		Mem_Stosb(data++, IO_ReadB(VGAREG_DAC_DATA));
		Mem_Stosb(data++, IO_ReadB(VGAREG_DAC_DATA));
		Mem_Stosb(data++, IO_ReadB(VGAREG_DAC_DATA));
		}
	}

void INT10_SelectDACPage(Bit8u function, Bit8u mode)
	{
	ResetACTL();
	IO_WriteB(VGAREG_ACTL_ADDRESS, 0x10);
	Bit8u old10 = IO_ReadB(VGAREG_ACTL_READ_DATA);
	if (!function)																	// Select paging mode
		{
		if (mode)
			old10 |= 0x80;
		else
			old10 &= 0x7f;
		//IO_WriteB(VGAREG_ACTL_ADDRESS,0x10);
		IO_WriteB(VGAREG_ACTL_WRITE_DATA, old10);
		}
	else																			// Select page
		{
		IO_WriteB(VGAREG_ACTL_WRITE_DATA, old10);
		if (!(old10 & 0x80))
			mode <<= 2;
		mode &= 0xf;
		IO_WriteB(VGAREG_ACTL_ADDRESS, 0x14);
		IO_WriteB(VGAREG_ACTL_WRITE_DATA, mode);
		}
	IO_WriteB(VGAREG_ACTL_ADDRESS, 32);												// Enable output and protect palette
	}

void INT10_GetDACPage(Bit8u* mode, Bit8u* page)
	{
	ResetACTL();
	IO_WriteB(VGAREG_ACTL_ADDRESS, 0x10);
	Bit8u reg10 = IO_ReadB(VGAREG_ACTL_READ_DATA);
	IO_WriteB(VGAREG_ACTL_WRITE_DATA, reg10);
	*mode = (reg10&0x80) ? 0x01 : 0x00;
	IO_WriteB(VGAREG_ACTL_ADDRESS, 0x14);
	*page = IO_ReadB(VGAREG_ACTL_READ_DATA);
	IO_WriteB(VGAREG_ACTL_WRITE_DATA, *page);
	if (*mode)
		*page &= 0xf;
	else
		{
		*page &= 0xc;
		*page >>= 2;
		}
	}

void INT10_SetPelMask(Bit8u mask)
	{
	IO_WriteB(VGAREG_PEL_MASK, mask);
	}	

void INT10_GetPelMask(Bit8u & mask)
	{
	mask = IO_ReadB(VGAREG_PEL_MASK);
	}
