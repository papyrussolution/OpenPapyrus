#include "vDos.h"
#include "inout.h"
#include "vga.h"
#include <math.h>


void vga_write_p3d4(Bitu port, Bitu val);
Bit8u vga_read_p3d4(Bitu port);
void vga_write_p3d5(Bitu port, Bitu val);
Bit8u vga_read_p3d5(Bitu port);

Bit8u vga_read_p3da(Bitu port)														// bit 0 = horizontal or vertical blanking, 3 = vertical sync
	{
	static Bitu hvBlank = 0;														// Just cycle through 0 and 1 (don't mind this hard level check)
	vga.attrindex = false;
	hvBlank ^= 1;
	Bit8u retval = hvBlank;
	if (vga.draw.vertRetrace)														// Set by VGA_VerticalTimer()
		retval |= 8;
	return retval;
	}

static void write_p3c2(Bitu port, Bitu val)
	{
	vga.misc_output = val;
    Bit16u freePort = 0x3d4-((val&1)<<5);
    Bit16u regPort = 0x3b4+((val&1)<<5);
	IO_RegisterWriteHandler(regPort, vga_write_p3d4);
	IO_RegisterReadHandler(regPort, vga_read_p3d4);
	IO_RegisterWriteHandler(regPort+1, vga_write_p3d5);
	IO_RegisterReadHandler(regPort+1, vga_read_p3d5);
	IO_RegisterReadHandler(regPort+6, vga_read_p3da);

	IO_FreeWriteHandler(freePort);
	IO_FreeReadHandler(freePort);
	IO_FreeWriteHandler(freePort+1);
	IO_FreeReadHandler(freePort+1);
	IO_FreeReadHandler(freePort+6);
	}


static Bit8u read_p3cc(Bitu port)
	{
	return vga.misc_output;
	}

// VGA feature control register
static Bit8u read_p3ca(Bitu port)
	{
	return 0;
	}

static Bit8u read_p3c2(Bitu port)
	{
	Bit8u retval = 0x60;

	switch((vga.misc_output>>2)&3)
		{
	case 0:
	case 3:
		retval |= 0x10;				// 0110 switch positions
		break;
	default:
		break;
		}

	return retval;
	/*
		0-3 0xF on EGA, 0x0 on VGA 
		4	Status of the switch selected by the Miscellaneous Output
			Register 3C2h bit 2-3. Switch high if set.
			(apparently always 1 on VGA)
		5	(EGA) Pin 19 of the Feature Connector (FEAT0) is high if set
		6	(EGA) Pin 17 of the Feature Connector (FEAT1) is high if set
			(default differs by card, ET4000 sets them both)
		7	If set IRQ 2 has happened due to Vertical Retrace.
			Should be cleared by IRQ 2 interrupt routine by clearing port 3d4h
			index 11h bit 4.
	*/
	}

void VGA_SetupMisc(void)
	{
	IO_RegisterReadHandler(0x3c2, read_p3c2);
	IO_RegisterWriteHandler(0x3c2, write_p3c2);
	IO_RegisterReadHandler(0x3ca, read_p3ca);
	IO_RegisterReadHandler(0x3cc, read_p3cc);
	}
