#include "vDos.h"
#include "inout.h"
#include "vga.h"

Bit8u read_p3c4(Bitu /*port*/)
	{
	return vga.seq.index;
	}

void write_p3c4(Bitu /*port*/, Bitu val)
	{
	vga.seq.index = val;
	}

void write_p3c5(Bitu /*port*/, Bitu val)
	{
	switch(vga.seq.index)
		{
	case 0:																			// Reset
		vga.seq.reset = val;
		break;
	case 1:																			// Clocking Mode
		if (val != vga.seq.clocking_mode)
			{
			if ((val&(~0x20)) != (vga.seq.clocking_mode&(~0x20)))					// Don't resize if only the screen off bit was changed
				{
				vga.seq.clocking_mode = val;
				VGA_StartResize();
				}
			else
				vga.seq.clocking_mode = val;
			}
		break;
	case 2:																			// Map mask
		vga.seq.map_mask = val & 15;
		vga.config.full_map_mask = FillTable[val & 15];
		vga.config.full_not_map_mask = ~vga.config.full_map_mask;
		/*
			0  Enable writes to plane 0 if set
			1  Enable writes to plane 1 if set
			2  Enable writes to plane 2 if set
			3  Enable writes to plane 3 if set
		*/
		break;
	case 3:																			// Character map select
		{
		vga.seq.character_map_select = val;
		}
		/*
			0,1,4  Selects VGA Character Map (0..7) if bit 3 of the character
					attribute is clear.
			2,3,5  Selects VGA Character Map (0..7) if bit 3 of the character
					attribute is set.
			Note: Character Maps are placed as follows:
			Map 0 at 0k, 1 at 16k, 2 at 32k, 3: 48k, 4: 8k, 5: 24k, 6: 40k, 7: 56k
		*/
		break;
	case 4:																			// Memory mode
		/* 
			0  Set if in an alphanumeric mode, clear in graphics modes.
			1  Set if more than 64kbytes on the adapter.
			2  Enables Odd/Even addressing mode if set. Odd/Even mode places all odd
				bytes in plane 1&3, and all even bytes in plane 0&2.
			3  If set address bit 0-1 selects video memory planes (256 color mode),
				rather than the Map Mask and Read Map Select Registers.
		*/
		if (val != vga.seq.memory_mode)
			vga.seq.memory_mode = val;
		break;
		}
	}

Bit8u read_p3c5(Bitu /*port*/)
	{
	switch (vga.seq.index)
		{
	case 0:																			// Reset
		return vga.seq.reset;
	case 1:																			// Clocking mode
		return vga.seq.clocking_mode;
	case 2:																			// Map mask
		return vga.seq.map_mask;
	case 3:																			// Character map select
		return vga.seq.character_map_select;
	case 4:																			// Memory mode
		return vga.seq.memory_mode;
	default:
		return 0;
		}
	}

void VGA_SetupSEQ(void)
	{
	IO_RegisterWriteHandler(0x3c4, write_p3c4);
	IO_RegisterWriteHandler(0x3c5, write_p3c5);
	IO_RegisterReadHandler(0x3c4, read_p3c4);
	IO_RegisterReadHandler(0x3c5, read_p3c5);
	}

