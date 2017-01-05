#include "dos_inc.h"
#include "../ints/int10.h"

#define NUMBER_ANSI_DATA 10

class device_CON : public DOS_Device
	{
public:
	device_CON();
	bool Read(Bit8u* data, Bit16u* size);
	bool Write(Bit8u* data, Bit16u* size);
	void Close();
	Bit16u GetInformation(void);
private:
	Bit8u readcache;
	Bit8u lastwrite;
	void ClearAnsi(void);
	struct ansi {
		bool esc;
		bool sci;
		bool enabled;
		Bit8u attr;
		Bit8s data[NUMBER_ANSI_DATA];
		Bit8u numberofarg;
		Bit16u nrows;
		Bit16u ncols;
		Bit8s savecol;
		Bit8s saverow;
		} ansi;
	};

bool device_CON::Read(Bit8u* data, Bit16u* size)
	{
	Bit16u count = 0;
	Bit16u keyStroke;
	Bit8u lowKey, highKey;
	if ((readcache) && (*size))
		{
		data[count++] = readcache;
		if (dos.echo)
			INT10_TeletypeOutput(readcache, 7);
		readcache = 0;
		}
	while (*size > count)
		{
		while (!BIOS_GetKey(keyStroke))												// Wait for keystroke
			{
			CALLBACK_Idle();														// To start vDos and let interrupts take place (change this at some time!)
			Sleep(2);
			}
		lowKey = keyStroke&0xff;
		highKey = keyStroke>>8;
		switch (lowKey)
			{
		case 13:
			data[count++] = 0x0D;
			if (*size > count)
				data[count++] = 0x0A;												// It's only expanded if there is room for it. (NO cache)
			*size = count;
			if (dos.echo)
				{ 
				INT10_TeletypeOutput(13, 7);										// Maybe don't do this (no need for it actually, but it's compatible)
				INT10_TeletypeOutput(10, 7);
				}
			return true;
			break;
		case 8:
			if (*size == 1)
				data[count++] = lowKey;												// one char at the time so give back that BS
			else if (count)
				{																	// Remove data if it exists (extended keys don't go right)
				data[count--] = 0;
				INT10_TeletypeOutput(8, 7);
				INT10_TeletypeOutput(' ',7);
				}
			else
				continue;															// no data read yet so restart whileloop.
			break;
		case 0xe0:																	// Extended keys in the int 16 0x10 case
			if (!highKey)															// Extended key if high isn't 0
				data[count++] = lowKey;
			else
				{
				data[count++] = 0;
				if (*size > count)
					data[count++] = highKey;
				else
					readcache = highKey;
				}
			break;
		case 0x0:																	// Special enhanced key
			if (highKey)
				{
				data[count++] = 0;
				if (*size > count)
					data[count++] = highKey;
				else
					readcache = highKey;
				}
			break;
		default:
			data[count++] = lowKey;
			break;
			}
		if (dos.echo)	
			INT10_TeletypeOutput(lowKey, 7);										// what to do if *size==1 and character is BS ?????
		}
	*size = count;
	return true;
	}

bool device_CON::Write(Bit8u* data, Bit16u* size)
	{
	Bit16u count = 0;
	Bitu i;
	Bit8u col, row;
	while (*size > count)
		{
		if (!ansi.esc)
			{
			if (data[count] == '\033')
				{
				ClearAnsi();		// clear the datastructure
				ansi.esc = true;	// start the sequence
				count++;
				continue;
				}
			// Some sort of "hack" now that \n doesn't set col to 0 (int10_char.cpp old chessgame)
			if ((data[count] == '\n') && (lastwrite != '\r'))
				INT10_TeletypeOutputAttr('\r', ansi.attr, ansi.enabled);
			// pass attribute only if ansi is enabled
			INT10_TeletypeOutputAttr(data[count], ansi.attr, ansi.enabled);
			lastwrite = data[count++];
			continue;
			}
		if (!ansi.sci)
			{
			if (data[count] == '[')
				ansi.sci = true;
			else
				ClearAnsi();
			count++;
			continue;
			}
		// ansi.esc and ansi.sci are true
		Bit8u page = Mem_aLodsb(BIOS_CURRENT_SCREEN_PAGE);
		switch (data[count])
			{
		case '0':	case '1':	case '2':	case '3':	case '4':	case '5':	case '6':	case '7':	case '8':	case '9':
			if (ansi.data[ansi.numberofarg] == -1)		// initialy -1, init
				ansi.data[ansi.numberofarg] = data[count]-'0';
			else
				ansi.data[ansi.numberofarg] = 10*ansi.data[ansi.numberofarg]+(data[count]-'0');
			count++;
			continue;
		case ';':	// till a max of NUMBER_ANSI_DATA
			ansi.numberofarg++;
			count++;
			continue;
		case 'm':	// SGR
			for (i = 0; i <= ansi.numberofarg; i++)
				{
				ansi.enabled = true;
				switch (ansi.data[i])
					{
				case 0:	// normal
					ansi.attr = 0x07;	// Real ansi does this as well. (should do current defaults)
					ansi.enabled = false;
					break;
				case 1: // bold mode on
					ansi.attr |= 0x08;
					break;
				case 4: // underline
					break;
				case 5: // blinking
					ansi.attr |= 0x80;
					break;
				case 7:	// reverse
					ansi.attr = 0x70;	//Just like real ansi. (should do use current colors reversed)
					break;
				case 30:	// fg color black
					ansi.attr &= 0xf8;
					ansi.attr |= 0x0;
					break;
				case 31:	// fg color red
					ansi.attr &= 0xf8;
					ansi.attr|= 0x4;
					break;
				case 32:	// fg color green
					ansi.attr &= 0xf8;
					ansi.attr |= 0x2;
					break;
				case 33:	// fg color yellow
					ansi.attr &= 0xf8;
					ansi.attr |= 0x6;
					break;
				case 34:	// fg color blue
					ansi.attr &= 0xf8;
					ansi.attr |= 0x1;
					break;
				case 35:	// fg color magenta
					ansi.attr &= 0xf8;
					ansi.attr |= 0x5;
					break;
				case 36:	// fg color cyan
					ansi.attr &= 0xf8;
					ansi.attr |= 0x3;
					break;
				case 37:	// fg color white
					ansi.attr &= 0xf8;
					ansi.attr |= 0x7;
					break;
				case 40:
					ansi.attr &= 0x8f;
					ansi.attr |= 0x0;
					break;
				case 41:
					ansi.attr &= 0x8f;
					ansi.attr |= 0x40;
					break;
				case 42:
					ansi.attr &= 0x8f;
					ansi.attr |= 0x20;
					break;
				case 43:
					ansi.attr &= 0x8f;
					ansi.attr |= 0x60;
					break;
				case 44:
					ansi.attr &= 0x8f;
					ansi.attr |= 0x10;
					break;
				case 45:
					ansi.attr &= 0x8f;
					ansi.attr |= 0x50;
					break;
				case 46:
					ansi.attr &= 0x8f;
					ansi.attr |= 0x30;
					break;	
				case 47:
					ansi.attr &= 0x8f;
					ansi.attr |= 0x70;
					break;
				default:
					break;
					}
				}
			break;
		case 'A':	// cursor up
			row = CURSOR_POS_ROW(page) - (ansi.data[0] == -1 ? 1 : ansi.data[0]);
			if (row < 0)
				row = 0;
			INT10_SetCursorPos(row, CURSOR_POS_COL(page), page);
			break;
		case 'B':	// cursor Down
			row = CURSOR_POS_ROW(page) + (ansi.data[0] == -1 ? 1 : ansi.data[0]);
			if (row >= ansi.nrows)
				row = ansi.nrows - 1;
			INT10_SetCursorPos(row, CURSOR_POS_COL(page), page);
			break;
		case 'C':	// cursor forward
			col = CURSOR_POS_COL(page) + (ansi.data[0] == -1 ? 1 : ansi.data[0]);
			if (col >= ansi.ncols)
				col = ansi.ncols - 1;
			INT10_SetCursorPos(CURSOR_POS_ROW(page), col, page);
			break;
		case 'D':	// Cursor Backward
			col = CURSOR_POS_COL(page) - (ansi.data[0] == -1 ? 1 : ansi.data[0]);
			if (col < 0)
				col = 0;
			INT10_SetCursorPos(CURSOR_POS_ROW(page), col, page);
			break;
		case 'E':	// Cursor Next Line - Moves cursor to beginning of the line n (default 1) lines down
			row = CURSOR_POS_ROW(page) + (ansi.data[0]= -1  ? 1 : ansi.data[0]);
			if (row >= ansi.nrows)
				row = ansi.nrows - 1;
			INT10_SetCursorPos(row, 0, page);
			break;
		case 'F':	//  Cursor Previous Line - Moves cursor to beginning of the line n (default 1) lines up
			row = CURSOR_POS_ROW(page) - (ansi.data[0] == -1 ? 1 : ansi.data[0]);
			if (row < 0)
				row = 0;
			INT10_SetCursorPos(row, 0, page);
			break;
		case 'G':	// Cursor Horizontal Absolute - Moves the cursor to column n
			col = ansi.data[0] == -1 ? 1 : ansi.data[0];
			if (col > ansi.ncols)
				col = (Bit8u)ansi.ncols;
			INT10_SetCursorPos(CURSOR_POS_ROW(page), --col, page);	// ansi=1 based, int10 is 0 based
			break;
		case 'H':	case 'f':		// Cursor Pos
			row = ansi.data[0] == -1 ? 1 : ansi.data[0];
			if (row > ansi.nrows)
				row = (Bit8u)ansi.nrows;
			col = ansi.data[1] == -1 ? 1 : ansi.data[1];
			if (col > ansi.ncols)
				col = (Bit8u)ansi.ncols;
			INT10_SetCursorPos(--row, --col, page);	// ansi=1 based, int10 is 0 based
			break;
		case 'J':	// erase screen and move cursor home (modes 0 and 1 are not supported)
			if (ansi.data[0] == 2)
				{
				INT10_ScrollWindow(0, 0, 255, 255, 0, ansi.attr, page);
				INT10_SetCursorPos(0, 0, page);
				}
			break;
		case 'K':	// Erase in Line
			col = CURSOR_POS_COL(page);
			row = CURSOR_POS_ROW(page);
			if (ansi.data[0] == -1 || ansi.data[0] == 0)	// Clear from cursor to the end of the line
				INT10_WriteChar(' ', ansi.attr, page, ansi.ncols-col-1, true);	// Extra -1 to prevent scrolling when end of screen is reached
			else if (ansi.data[0] == 1)						// Clear from cursor to beginning of the line
				{
				INT10_SetCursorPos(row, 0, page);
				INT10_WriteChar(' ', ansi.attr, page, col == ansi.ncols-1 ? col : col+1, true);
				}
			if (ansi.data[0] == 2)							// Clear entire line
				{
				INT10_SetCursorPos(row, 0, page);
				INT10_WriteChar(' ', ansi.attr, page, ansi.ncols-1, true);	// Extra -1 to prevent scrolling when end of screen is reached
				}
			INT10_SetCursorPos(row, col, page);
			break;
//		case 'M':	// delete line (NANSI)
//			col = CURSOR_POS_COL(page);
//			row = CURSOR_POS_ROW(page);
//			INT10_ScrollWindow(row, 0, ansi.nrows-1, ansi.ncols-1,ansi.data[0] ? -ansi.data[0] : -1, ansi.attr, 0xFF);
//			break;
		case 'u':	// Restore Cursor Pos
			INT10_SetCursorPos(ansi.saverow, ansi.savecol, page);
			break;
		case 's':	// SAVE CURSOR POS
			ansi.savecol = CURSOR_POS_COL(page);
			ansi.saverow = CURSOR_POS_ROW(page);
			break;
		default:
			break;
			}
		ClearAnsi();
		count++;
		}
	return true;
	}

void device_CON::Close()
	{
	}

Bit16u device_CON::GetInformation(void)
	{
	Bit16u head = Mem_aLodsw(BIOS_KEYBOARD_BUFFER_HEAD);
	Bit16u tail = Mem_aLodsw(BIOS_KEYBOARD_BUFFER_TAIL);

	if ((head == tail) && !readcache)
		return 0x80D3;																// No Key Available
	return 0x8093;																	// Key Available
	}

device_CON::device_CON()
	{
	SetName("CON");
	readcache = 0;
	lastwrite = 0;
	ansi.enabled = false;
	ansi.attr = 0x7;
	ansi.ncols = Mem_aLodsw(BIOS_SCREEN_COLUMNS);
	ansi.nrows = Mem_aLodsb(BIOS_ROWS_ON_SCREEN_MINUS_1)+1;
	ansi.saverow = 0;
	ansi.savecol = 0;
	ClearAnsi();
	}

void device_CON::ClearAnsi(void)
	{
	for (Bit8u i = 0; i < NUMBER_ANSI_DATA; i++)
		ansi.data[i] = -1;
	ansi.esc = false;
	ansi.sci = false;
	ansi.numberofarg = 0;
	}
