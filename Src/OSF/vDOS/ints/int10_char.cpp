#include "vDos.h"
#include "bios.h"
#include "mem.h"
#include "inout.h"
#include "int10.h"
#include "callback.h"

static void EGA16_CopyRow(Bit8u cleft, Bit8u cright, Bit8u rold, Bit8u rnew, PhysPt base)
	{
	PhysPt src, dest;
	Bit8u cheight = Mem_aLodsb(BIOS_FONT_HEIGHT);
	dest = base+(CurMode->twidth*rnew)*cheight+cleft;
	src = base+(CurMode->twidth*rold)*cheight+cleft;
	Bitu nextline = CurMode->twidth;
	// Setup registers correctly
	IO_WriteB(0x3ce, 5);															// Memory transfer mode
	IO_WriteB(0x3cf, 1);
	IO_WriteB(0x3c4, 2);
	IO_WriteB(0x3c5, 0xf);															// Enable all Write planes
	// Do some copying
	Bitu rowsize = (cright-cleft);
	for (Bitu copy = cheight; copy > 0; copy--)
		{
		Mem_rMovsb(dest, src, rowsize);
		dest += nextline;
		src += nextline;
		}
	IO_WriteB(0x3ce, 5);															// Restore registers
	IO_WriteB(0x3cf, 0);															// Normal transfer mode
	}

static inline void TEXT_CopyRow(Bit8u cleft, Bit8u cright, Bit8u rold, Bit8u rnew, PhysPt base)
	{
	Mem_rMovsb(base+(rnew*CurMode->twidth+cleft)*2, base+(rold*CurMode->twidth+cleft)*2, (cright-cleft)*2);
	}

static void EGA16_FillRow(Bit8u cleft, Bit8u cright, Bit8u row, PhysPt base, Bit8u attr)
	{
	IO_WriteB(0x3ce, 0x8);															// Set Bitmask / Color / Full Set Reset
	IO_WriteB(0x3cf, 0xff);
	IO_WriteB(0x3ce, 0x0);
	IO_WriteB(0x3cf, attr);
	IO_WriteB(0x3ce, 0x1);
	IO_WriteB(0x3cf, 0xf);
	
	Bit8u cheight = Mem_aLodsb(BIOS_FONT_HEIGHT);									// Write some bytes
	PhysPt dest = base+(CurMode->twidth*row)*cheight+cleft;	
	Bitu nextline = CurMode->twidth;
	Bitu rowsize = (cright-cleft);
	for (Bitu copy = cheight; copy > 0; copy--)
		{
		Mem_rStosb(dest, 0xff, rowsize);
		dest += nextline;
		}
	IO_WriteB(0x3cf, 0);
	}

static inline void TEXT_FillRow(Bit8u cleft, Bit8u cright, Bit8u row, PhysPt base, Bit8u attr)
	{
	LinPt dest = base+(row*CurMode->twidth+cleft)*2;								// Do some filing
	Bit16u fill = (attr<<8) + ' ';
	Mem_rStosw(dest, fill, cright-cleft);
	}

void INT10_ScrollWindow(Bit8u rul, Bit8u cul, Bit8u rlr, Bit8u clr, Bit8s nlines, Bit8u attr, Bit8u page)
	{
	if (CurMode->type != M_TEXT)													// Do some range checking
		page = 0xff;
	BIOS_NCOLS;
	BIOS_NROWS;
	if (rul > rlr || cul > clr)
		return;
	if (rlr >= nrows)
		rlr = (Bit8u)nrows-1;
	if (clr >= ncols)
		clr = (Bit8u)ncols-1;
	clr++;

	if (page == 0xFF)																// Get the correct page
		page = Mem_aLodsb(BIOS_CURRENT_SCREEN_PAGE);
	PhysPt base = CurMode->pstart+page*Mem_aLodsw(BIOS_VIDEO_PAGE_SIZE);

	Bit8u start, end;																// See how much lines need to be copied
	Bits next;
	if (nlines > 0)																	// Copy some lines
		{
		start = rlr-nlines+1;
		end = rul;
		next = -1;
		}
	else if (nlines < 0)
		{
		start = rul-nlines-1;
		end = rlr;
		next = 1;
		}
	else
		{
		nlines = rlr-rul+1;
		goto filling;
		}
	while (start != end)
		{
		start += next;
		switch (CurMode->type)
			{
		case M_TEXT:
			TEXT_CopyRow(cul, clr, start, start+nlines, base);
			break;
		case M_EGA:		
			EGA16_CopyRow(cul, clr, start, start+nlines, base);
			break;
			}	
		} 

filling:																			// Fill some lines
	if (nlines > 0)
		start = rul;
	else
		{
		nlines = -nlines;
		start = rlr-nlines+1;
		}
	for (; nlines > 0; nlines--)
		{
		switch (CurMode->type)
			{
		case M_TEXT:
			TEXT_FillRow(cul, clr, start, base, attr);
			break;
		case M_EGA:		
			EGA16_FillRow(cul, clr, start, base, attr);
			break;
			}	
		start++;
		} 
	}

void INT10_SetActivePage(Bit8u page)
	{
	if (page < 8)
		{
		Bit16u mem_address = page*Mem_aLodsw(BIOS_VIDEO_PAGE_SIZE);
		Mem_aStosw(BIOS_VIDEO_MEMORY_ADDRESS, mem_address);							// Write the new page start
		if (CurMode->mode < 8)
			mem_address >>= 1;
		Bit16u base = Mem_aLodsw(BIOS_VIDEO_PORT);									// Write the new start address in vgahardware
		IO_WriteB(base, 0x0c);
		IO_WriteB(base+1, (Bit8u)(mem_address>>8));
		IO_WriteB(base, 0x0d);
		IO_WriteB(base+1, (Bit8u)mem_address);
		
		Mem_aStosb(BIOS_CURRENT_SCREEN_PAGE, page);									// And change the BIOS page
		INT10_SetCursorPos(CURSOR_POS_ROW(page), CURSOR_POS_COL(page), page);		// Display the cursor, now the page is active
		}
	}

void INT10_SetCursorShape(Bit8u first, Bit8u last)
	{
	Mem_aStosw(BIOS_CURSOR_SHAPE, last|(first<<8));
	if ((first&0x60) == 0x20)														// Check for CGA type 01, invisible
		{
		first = 0x1e;
		last = 0x00;
		goto dowrite;
		}
	if (!(Mem_aLodsb(BIOS_VIDEO_INFO_0)&0x1))										// Check if we need to convert CGA Bios cursor values
		{																			// Set by int10 fun12 sub34
//		if (CurMode->mode>0x3) goto dowrite;										// Only mode 0-3 are text modes on cga
		if ((first&0xe0) || (last&0xe0))
			goto dowrite;
		Bit8u cheight = Mem_aLodsb(BIOS_FONT_HEIGHT)-1;
		if (last < first)															// Creative routine I based of the original ibmvga bios
			{
			if (!last)
				goto dowrite;
			first = last;
			last = cheight;
			// Test if this might be a cga style cursor set, if not don't do anything
			}
		else if (((first|last) >= cheight) || !(last == (cheight-1)) || !(first == cheight))
			{
			if (last <= 3)
				goto dowrite;
			if (first+2 < last)
				{
				if (first > 2)
					first = (cheight+1)/2;
				last = cheight;
				}
			else
				{
				first = (first-last)+cheight;
				last = cheight;
				if (cheight > 0xc)
					{																// Vgatest sets 15 15 2x where only one should be decremented to 14 14
					first--;														// Implementing int10 fun12 sub34 fixed this.
					last--;
					}
				}
			}
		}
dowrite:
	Bit16u base = Mem_aLodsw(BIOS_VIDEO_PORT);
	IO_WriteB(base, 0xa);
	IO_WriteB(base+1, first);
	IO_WriteB(base, 0xb);
	IO_WriteB(base+1, last);
	}

void INT10_SetCursorPos(Bit8u row, Bit8u col, Bit8u page)
	{
	Mem_aStosw(BIOS_VIDEO_CURSOR_POS+page*2, (row<<8)+col);							// BIOS cursor pos
	if (page == Mem_aLodsb(BIOS_CURRENT_SCREEN_PAGE))								// Set the hardware cursor
		{
		BIOS_NCOLS;																	// Get the dimensions
		// Calculate the address knowing nbcols nbrows and page num
		// NOTE: BIOS_VIDEO_MEMORY_ADDRESS counts in colour/flag pairs
		Bit16u address = (ncols*row)+col+Mem_aLodsw(BIOS_VIDEO_MEMORY_ADDRESS)/2;
		// CRTC regs 0x0e and 0x0f
		Bit16u base = Mem_aLodsw(BIOS_VIDEO_PORT);
		IO_WriteB(base, 0x0e);
		IO_WriteB(base+1, (Bit8u)(address>>8));
		IO_WriteB(base, 0x0f);
		IO_WriteB(base+1, (Bit8u)address);
		}
	}

void ReadCharAttr(Bit16u col, Bit16u row, Bit8u page, Bit16u * result)				// Externally used by the mouse routine
	{
	Bit16u address = page*Mem_aLodsw(BIOS_VIDEO_PAGE_SIZE);							// Compute the address
	address += (row*Mem_aLodsw(BIOS_SCREEN_COLUMNS)+col)*2;
	*result = Mem_Lodsw(CurMode->pstart+address);									// Read the char
	}

void INT10_ReadCharAttr(Bit16u * result, Bit8u page)
	{
	if (CurMode->type != M_TEXT)													// If not in text mode, simply return 0
		{
		*result = 0;
		return;
		}
	if (page == 0xFF)
		page = Mem_aLodsb(BIOS_CURRENT_SCREEN_PAGE);
	ReadCharAttr(CURSOR_POS_COL(page), CURSOR_POS_ROW(page), page, result);
	}

void WriteChar(Bit16u col, Bit16u row, Bit8u page, Bit8u chr, Bit8u attr, bool useattr)	// Externally used by the mouse routine
	{
	if (CurMode->type == M_TEXT)
		{
		// Compute the address  
		Bit16u address = page*Mem_aLodsw(BIOS_VIDEO_PAGE_SIZE) + (row*Mem_aLodsw(BIOS_SCREEN_COLUMNS)+col)*2;
		PhysPt pos = CurMode->pstart+address;										// Write the char 
		Mem_Stosb(pos, chr);
		if (useattr)
			Mem_Stosb(pos+1, attr);
		return;
		}

	Bit8u cheight = Mem_aLodsb(BIOS_FONT_HEIGHT);
	RealPt fontdata = SegOff2dWord(RealSeg(RealGetVec(0x43)), RealOff(RealGetVec(0x43)) + chr*cheight);
	if (!useattr)
		attr = 0xf;																	// Set attribute(color) to a sensible value

	// Some weird behavior of mode 6 (and 11) 
	if ((CurMode->mode == 0x6)/* || (CurMode->mode==0x11)*/)
		attr = (attr&0x80)|1;
	// (same fix for 11 fixes vgatest2, but it's not entirely correct according to wd)

	Bitu x = 8*col;
	Bitu y = cheight*row;
	// TODO Check for out of bounds
	if (CurMode->type == M_EGA)
		{
		// enable all planes for EGA modes (Ultima 1 colour bug)
		/* might be put into INT10_PutPixel but different vga bios
		   implementations have different opinions about this */
		IO_WriteB(0x3c4, 0x2);
		IO_WriteB(0x3c5, 0xf);
		}
	for (Bit8u h = 0; h < cheight; h++)
		{
		Bit8u bitsel = 128;
		Bit8u bitline = Mem_Lodsb(dWord2Ptr(fontdata));
		fontdata = SegOff2dWord(RealSeg(fontdata), RealOff(fontdata)+ 1);
		Bit16u tx = (Bit16u)x;
		while (bitsel)
			{
			if (bitline&bitsel)
				INT10_PutPixel(tx, (Bit16u)y, page, attr);
			else
				INT10_PutPixel(tx, (Bit16u)y, page, attr & 0x80);
			tx++;
			bitsel >>= 1;
			}
		y++;
		}
	}

void INT10_WriteChar(Bit8u chr, Bit8u attr, Bit8u page, Bit16u count, bool showattr)
	{
	if (CurMode->type != M_TEXT)
		{
		showattr = true;															// Use attr in graphics mode always
		page %= CurMode->ptotal;
		}

	Bit8u cur_row = CURSOR_POS_ROW(page);
	Bit8u cur_col = CURSOR_POS_COL(page);
	BIOS_NCOLS;
	while (count > 0)
		{
		WriteChar(cur_col, cur_row, page, chr, attr, showattr);
		count--;
		if (++cur_col == ncols)
			{
			cur_col = 0;
			cur_row++;
			}
		}
	}

static void INT10_TeletypeOutputAttr(Bit8u chr, Bit8u attr, bool useattr, Bit8u page)
	{
	BIOS_NCOLS;
	BIOS_NROWS;
	Bit8u cur_row = CURSOR_POS_ROW(page);
	Bit8u cur_col = CURSOR_POS_COL(page);
	switch (chr)
		{
	case 7:
		Beep(1750, 300);
		break;
	case 8:
		if (cur_col > 0)
			cur_col--;
		break;
	case '\r':
		cur_col = 0;
		break;
	case '\n':
//		cur_col=0; //Seems to break an old chess game
		cur_row++;
		break;
	case '\t':
		do
			{
			INT10_TeletypeOutputAttr(' ', attr, useattr, page);
			cur_row = CURSOR_POS_ROW(page);
			cur_col = CURSOR_POS_COL(page);
			}
		while(cur_col%8);
		break;
	default:
		WriteChar(cur_col, cur_row, page, chr, attr, useattr);						// Draw the actual Character
		cur_col++;
		}
	if (cur_col == ncols)
		{
		cur_col = 0;
		cur_row++;
		}
	if (cur_row == nrows)															// Do we need to scroll ?
		{
		Bit8u fill = (CurMode->type == M_TEXT) ? 0x7 : 0;							// Fill with black on non-text modes and with 0x7 on textmode
		INT10_ScrollWindow(0, 0, (Bit8u)(nrows-1), (Bit8u)(ncols-1), -1, fill, page);
		cur_row--;
		}
	INT10_SetCursorPos(cur_row, cur_col, page);										// Set the cursor for the page
	}

void INT10_TeletypeOutputAttr(Bit8u chr, Bit8u attr, bool useattr)
	{
	INT10_TeletypeOutputAttr(chr, attr, useattr, Mem_aLodsb(BIOS_CURRENT_SCREEN_PAGE));
	}

void INT10_TeletypeOutput(Bit8u chr, Bit8u attr)
	{
	INT10_TeletypeOutputAttr(chr, attr, CurMode->type != M_TEXT);
	}

void INT10_WriteString(Bit8u row, Bit8u col, Bit8u flag, Bit8u attr, PhysPt string, Bit16u count, Bit8u page)
	{
	Bit8u cur_row = CURSOR_POS_ROW(page);
	Bit8u cur_col = CURSOR_POS_COL(page);
	
	if (row == 0xff)																// If row=0xff special case : use current cursor position
		{
		row = cur_row;
		col = cur_col;
		}
	INT10_SetCursorPos(row, col, page);
	while (count > 0)
		{
		Bit8u chr = Mem_Lodsb(string++);
		if (flag&2)
			attr = Mem_Lodsb(string++);
		INT10_TeletypeOutputAttr(chr, attr, true, page);
		count--;
		}
	if (!(flag&1))
		INT10_SetCursorPos(cur_row, cur_col, page);
	}
