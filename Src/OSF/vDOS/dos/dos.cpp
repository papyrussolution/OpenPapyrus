#include <stdlib.h>
#include "vDos.h"
#include "bios.h"
#include "mem.h"
#include "callback.h"
#include "regs.h"
#include "parport.h"
#include "serialport.h"
#include "dos_inc.h"
#include "video.h"

DOS_Block dos;
DOS_InfoBlock dos_infoblock;

#define DOS_COPYBUFSIZE 0x10000
Bit8u dos_copybuf[DOS_COPYBUFSIZE];
static Bit8u daysInMonth[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
static Bit16u countryNo = 0;

void DOS_SetError(Bit16u code)
	{
	dos.errorcode = code;
	}

#define DOSNAMEBUF 256

// Something like FCBNameToStr, strips padding spaces right before '.', '\' and '/'
void DosPathStripSpaces(char *name)
	{
	int k = 0;
	for (int i = 0, j = 0; char c = name[i]; i++)
		{
		if (c == '.' || c == '/' || c == '\\')
			j = k;
		if (c != ' ')
			k = j + 1;																// Last non-space position (+1)
		name[j++] = c;
		}
	name[k] = 0;
	}

bool do_debug = false;
Bit32u deb_address = 0;
static Bitu DOS_21Handler(void)
	{
	if (((reg_ah != 0x50) && (reg_ah != 0x51) && (reg_ah != 0x62) && (reg_ah != 0x64)) && (reg_ah < 0x6c))
		if (dos.psp() != 0)															// No program loaded yet?
			{
			DOS_PSP psp(dos.psp());
			psp.SetStack(SegOff2dWord(SegValue(ss), reg_sp-18));
			psp.FindFreeFileEntry();
			}
	char name1[DOSNAMEBUF+2+DOS_NAMELENGTH_ASCII];
	char name2[DOSNAMEBUF+2+DOS_NAMELENGTH_ASCII];
//	if (reg_ah != 0x0b)
//vLog("DOS_21Handler: %04x", reg_ax);
	switch (reg_ah)
		{
	case 0x00:																		// Terminate Program
		DOS_Terminate(Mem_Lodsw(SegPhys(ss)+reg_sp+2), false, 0);
		break;
	case 0x01:																		// Read character from STDIN, with echo
		{
		Bit8u c;
		Bit16u n = 1;
		dos.echo = true;
		DOS_ReadFile(STDIN, &c, &n);
		reg_al = c;
		dos.echo = false;
		}
		break;
	case 0x02:																		// Write character to STDOUT
		{
		Bit8u c = reg_dl;
		Bit16u n = 1;
		DOS_WriteFile(STDOUT, &c, &n);
		// Not in the official specs, but happens nonetheless. (last written character)
		reg_al = c;																	// reg_al=(c==9)?0x20:c; //Officially: tab to spaces
		}
		break;
	case 0x03:																		// Read character from STDAUX
		{
		Bit16u port = Mem_aLodsw(BIOS_BASE_ADDRESS_COM1);
		if (port != 0 && serialPorts[0])
			serialPorts[0]->Getchar(&reg_al);
		}
		break;
	case 0x04:																		// Write Character to STDAUX
		{
		Bit16u port = Mem_aLodsw(BIOS_BASE_ADDRESS_COM1);
		if (port != 0 && serialPorts[0])
			serialPorts[0]->Putchar(reg_dl);
		}
		break;
	case 0x05:																		// Write Character to PRINTER
		parallelPorts[0]->Putchar(reg_dl);
		break;
	case 0x06:																		// Direct Console Output/Input
		if (reg_dl == 0xff)															// Input
			{
			if (!DOS_GetSTDINStatus())
				{
				reg_al = 0;
				CALLBACK_SZF(true);
				break;
				}
			Bit8u c;
			Bit16u n = 1;
			DOS_ReadFile(RealHandle(STDIN), &c, &n);
			reg_al = c;
			CALLBACK_SZF(false);
			}
		else																		// Ouput
			{
			Bit8u c = reg_dl;
			Bit16u n = 1;
			DOS_WriteFile(RealHandle(STDOUT), &c, &n);
			reg_al = reg_dl;
			}
		break;
	case 0x07:																		// Character Input, without echo
	case 0x08:																		// Direct Character Input, without echo (checks for breaks officially :)
		{
		Bit8u c;
		Bit16u n = 1;
		DOS_ReadFile (RealHandle(STDIN), &c, &n);
		reg_al = c;
		}
		break;
	case 0x09:																		// Write string to STDOUT
		{
		Bit8u c;
		Bit16u n = 1;
		PhysPt buf = SegPhys(ds)+reg_dx;
		while ((c = Mem_Lodsb(buf++)) != '$')
			DOS_WriteFile(RealHandle(STDOUT), &c, &n);
		}
		break;
	case 0x0a:																		// Buffered Input
		{
		// TODO ADD Break checkin in STDIN but can't care that much for it
		PhysPt data = SegPhys(ds)+reg_dx;
		Bit8u free = Mem_Lodsb(data);
		if (!free)
			break;
		Bit8u in = RealHandle(STDIN);
		Bit8u out = RealHandle(STDIN);
		Bit8u read = 0;
		for(;;)
			{
			Bit8u c;
			Bit16u n = 1;
			DOS_ReadFile(in, &c ,&n);
			if (c == 8)
				{																	// Backspace
				if (read)
					{																// Something to backspace.
					n = 3;
					DOS_WriteFile(out, (Bit8u *)"\b \b", &n);
					--read;
					}
				continue;
				}
			if (read >= free)														// Keyboard buffer full
				{
				Beep(1750, 300);
				continue;
				}
			DOS_WriteFile(out, &c, &n);
			Mem_Stosb(data+read+2, c);
			if (c == 13) 
				break;
			read++;
			}
		Mem_Stosb(data+1, read);
		}
		break;
	case 0x0b:																		// Get STDIN Status
		reg_al = DOS_GetSTDINStatus() ? 0xff : 0x00;
		break;
	case 0x0c:																		// Flush Buffer and read STDIN call
		{
		Bit8u handle = RealHandle(STDIN);
		Bit8u c;
		Bit16u n;
		while (DOS_GetSTDINStatus())
			{
			n = 1;
			DOS_ReadFile(STDIN, &c, &n);
			}
		if (reg_al == 0x1 || reg_al == 0x6 || reg_al == 0x7 || reg_al == 0x8 || reg_al == 0xa)
			{ 
			Bit8u oldah = reg_ah;
			reg_ah = reg_al;
			DOS_21Handler();
			reg_ah = oldah;
			}
		else
			reg_al = 0;
		}
		break;
	case 0x0d:																		// Disk Reset
		break;	
	case 0x0e:																		// Select Default Drive
		DOS_SetDefaultDrive(reg_dl);
		reg_al = DOS_DRIVES;
		break;
	case 0x0f:																		// Open File using FCB
		reg_al = FCB_OpenFile(SegValue(ds), reg_dx) ? 0 : 0xFF;
		break;
	case 0x10:																		// Close File using FCB
		reg_al = FCB_CloseFile(SegValue(ds), reg_dx) ? 0 : 0xFF;
		break;
	case 0x11:																		// Find First Matching File using FCB
		reg_al = FCB_FindFirst(SegValue(ds), reg_dx) ? 0 : 0xFF;					// No test for C:\COMMAND.COM!
		break;
	case 0x12:																		// Find Next Matching File using FCB
		reg_al = FCB_FindNext(SegValue(ds), reg_dx) ? 0 : 0xFF;
		break;
	case 0x13:																		// Delete File using FCB
		reg_al = FCB_DeleteFile(SegValue(ds), reg_dx) ? 0 : 0xFF;
		break;
	case 0x14:																		// Sequential read from FCB
		reg_al = FCB_ReadFile(SegValue(ds), reg_dx, 0);
		break;
	case 0x15:																		// Sequential write to FCB
		reg_al = FCB_WriteFile(SegValue(ds), reg_dx, 0);
		break;
	case 0x16:																		// Create or truncate file using FCB
		reg_al = FCB_CreateFile(SegValue(ds), reg_dx) ? 0 : 0xFF;
		break;
	case 0x17:																		// Rename file using FCB
		reg_al = FCB_RenameFile(SegValue(ds), reg_dx) ? 0 : 0xFF;
		break;
	case 0x19:																		// Get current default drive
		reg_al = DOS_GetDefaultDrive();
		break;
	case 0x1a:																		// Set Disk Transfer Area Address
		{
		dos.dta(RealMakeSeg(ds, reg_dx));
		DOS_DTA dta(dos.dta());
		break;
		}
	case 0x1b:																		// Get allocation info for default drive
		if (!DOS_GetAllocationInfo(0, &reg_cx, &reg_al, &reg_dx))
			reg_al = 0xFF;
		break;
	case 0x1c:																		// Get allocation info for specific drive
		if (!DOS_GetAllocationInfo(reg_dl, &reg_cx, &reg_al, &reg_dx))
			reg_al = 0xFF;
		break;
	case 0x21:																		// Read random record from FCB
		{
		Bit16u toread = 1;
		reg_al = FCB_RandomRead(SegValue(ds), reg_dx, &toread, true);
		}
		break;
	case 0x22:																		// Write random record to FCB
		{
		Bit16u towrite = 1;
		reg_al = FCB_RandomWrite(SegValue(ds), reg_dx, &towrite, true);
		}
		break;
	case 0x23:																		// Get file size for FCB
		reg_al = FCB_GetFileSize(SegValue(ds), reg_dx) ? 0 : 0xFF;
		break;
	case 0x24:																		// Set Random Record number for FCB
		FCB_SetRandomRecord(SegValue(ds), reg_dx);
		break;
	case 0x25:																		// Set Interrupt Vector
		RealSetVec(reg_al, RealMakeSeg(ds, reg_dx));
		break;
	case 0x26:																		// Create new PSP
		DOS_NewPSP(reg_dx, DOS_PSP(dos.psp()).GetSize());
		break;
	case 0x27:																		// Random block read from FCB
		reg_al = FCB_RandomRead(SegValue(ds), reg_dx, &reg_cx, false);
		break;
	case 0x28:																		// Random Block write to FCB
		reg_al = FCB_RandomWrite(SegValue(ds), reg_dx, &reg_cx, false);
		break;
	case 0x29:																		// Parse filename into FCB
		{   
		Bit8u difference;
		char string[1024];
		Mem_StrnCopyFrom(string, SegPhys(ds)+reg_si, 1023);							// 1024 toasts the stack
		reg_al = FCB_Parsename(SegValue(es), reg_di, reg_al, string, &difference);
		reg_si += difference;
		}
		break;
	case 0x2a:																		// Get System Date
		{
		_SYSTEMTIME systime;														// Return the Windows localdate
		GetLocalTime(&systime);
		reg_al = (Bit8u)systime.wDayOfWeek;											// NB Sunday = 0, despite of MSDN documentation
		reg_cx = systime.wYear;
		reg_dx = (systime.wMonth<<8)+systime.wDay;
		}
		break;
	case 0x2b:																		// Set System Date (we don't!)
		reg_al = 0xff;
		daysInMonth[2] = reg_cx&3 ? 28 : 29;										// Year is from 1980 to 2099, it is this simple
		if (reg_cx >= 1980 && reg_cx <= 2099)
			if (reg_dh > 0 && reg_dh <= 12)
				if (reg_dl > 0 && reg_dl <= daysInMonth[reg_dh])
					reg_al = 0;														// Date is valid, fake set
		break;			
	case 0x2c:																		// Get System Time
		{
		_SYSTEMTIME systime;														// Return the Windows localtime
		GetLocalTime(&systime);
		reg_cx = (systime.wHour<<8) + systime.wMinute;
		reg_dx = (systime.wSecond<<8) + systime.wMilliseconds/10;
		}
		break;
	case 0x2d:																		// Set System Time (we don't!)
		if (reg_ch < 24 && reg_cl < 60 && reg_dh < 60 && reg_dl < 100)
			reg_al = 0;																// Time is valid, fake set
		else
			reg_al = 0xff; 
		break;
	case 0x2e:																		// Set Verify flag
		dos.verify = (reg_al == 1);
		break;
	case 0x2f:																		// Get Disk Transfer Area
		SegSet16(es, RealSeg(dos.dta()));
		reg_bx = RealOff(dos.dta());
		break;
	case 0x30:																		// Get DOS Version
		if (reg_al == 0)
			reg_bh = 0xFF;															// Fake Microsoft DOS
		else if (reg_al == 1)
			reg_bh = 0;																// DOS is NOT in HMA
		reg_al = dos.version.major;
		reg_ah = dos.version.minor;
		reg_bl = 0;																	// Serialnumber
		reg_cx = 0;
		break;
	case 0x31:																		// Terminate and stay resident
		// Important: This service does not set the carry flag!
		DOS_ResizeMemory(dos.psp(), &reg_dx);
		DOS_Terminate(dos.psp(), true, reg_al);
		break;
	case 0x1f:																		// Get drive parameter block for default drive
	case 0x32:																		// Get drive parameter block for specific drive
		{																			// Officially a dpb should be returned as well. The disk detection part is implemented
		Bit8u drive = reg_dl;
		if (!drive || reg_ah == 0x1f)
			drive = DOS_GetDefaultDrive();
		else
			drive--;
		if (Drives[drive])
			{
			reg_al = 0;
			SegSet16(ds, dos.tables.dpb);
			reg_bx = drive;															// Faking only the first entry (that is the driveletter)
			}
		else
			reg_al = 0xff;
		}
		break;
	case 0x33:																		// Extended Break Checking
		switch (reg_al)
			{
		case 0:																		// Get the breakcheck flag
			reg_dl = dos.breakcheck;
			break;
		case 1:																		// Set the breakcheck flag
			dos.breakcheck = (reg_dl > 0);
			break;
		case 2:
			{
			bool old = dos.breakcheck;
			dos.breakcheck = (reg_dl > 0);
			reg_dl = old;
			}
			break;
		case 3:																		// Get cpsw
		case 4:																		// Set cpsw, both not used really
			break;
		case 5:
			reg_dl = 3;																// Boot drive = C:
			break;
		case 6:																		// Get true version number
			reg_bx = (dos.version.minor<<8) + dos.version.major;
			reg_dx = 0x1000;														// Dos in ROM
			break;
		default:
			reg_al = 0xff;
		}
		break;
	case 0x34:																		// Get address of INDos Flag
		SegSet16(es, DOS_SDA_SEG);
		reg_bx = DOS_SDA_OFS + 0x01;
		break;
	case 0x35:																		// Get interrupt vector
		reg_bx = Mem_aLodsw(((Bit16u)reg_al)*4);
		SegSet16(es, Mem_aLodsw(((Bit16u)reg_al)*4+2));
		break;
	case 0x36:																		// Get Free Disk Space
		{
		Bit16u bytes, clusters, free;
		Bit8u sectors;
		if (DOS_GetFreeDiskSpace(reg_dl, &bytes, &sectors, &clusters, &free))
			{
			reg_ax = sectors;
			reg_bx = free;
			reg_cx = bytes;
			reg_dx = clusters;
			}
		else
			reg_ax = 0xffff;														// invalid drive specified
		}
		break;
	case 0x37:																		// Get/Set Switch char Get/Set Availdev thing
		switch (reg_al)
			{
		case 0:
			 reg_dl = 0x2f;															// Always return '/' like dos 5.0+
			 break;
		case 1:
			 reg_al = 0;
			 break;
		case 2:
			 reg_al = 0;
			 reg_dl = 0x2f;
			 break;
		case 3:
			 reg_al = 0;
			 break;
			}
		break;
	case 0x38:																		// Get/set Country Code
		if (reg_al == 0)															// Get country specidic information
			{
			PhysPt dest = SegPhys(ds)+reg_dx;
			Mem_CopyTo(dest, dos.tables.country, 0x18);
			reg_ax = reg_bx = 1;
			CALLBACK_SCF(false);
			}
		else																		// Set country code
			CALLBACK_SCF(true);
		break;
	case 0x39:																		// MKDIR Create directory
		Mem_StrnCopyFrom(name1, SegPhys(ds)+reg_dx, DOSNAMEBUF);
		rSpTrim(name1);
		if (DOS_MakeDir(name1))
			{
			reg_ax = 0xffff;														// AX destroyed
			CALLBACK_SCF(false);
			}
		else
			{
			reg_ax = dos.errorcode;
			CALLBACK_SCF(true);
			}
		break;
	case 0x3a:																		// RMDIR Remove directory
		Mem_StrnCopyFrom(name1, SegPhys(ds)+reg_dx, DOSNAMEBUF);
		rSpTrim(name1);
		if (DOS_RemoveDir(name1))
			{
			reg_ax = 0xffff;														// AX destroyed
			CALLBACK_SCF(false);
			}
		else
			{
			reg_ax = dos.errorcode;
			CALLBACK_SCF(true);
			}
		break;
	case 0x3b:																		// CHDIR Set current directory
		Mem_StrnCopyFrom(name1, SegPhys(ds)+reg_dx, DOSNAMEBUF);
		rSpTrim(name1);
		if (DOS_ChangeDir(name1))
			{
			reg_ax = 0xff00;														// AX destroyed
			CALLBACK_SCF(false);
			}
		else
			{
			reg_ax = dos.errorcode;
			CALLBACK_SCF(true);
			}
		break;
	case 0x3c:																		// CREATE Create or truncate file
		Mem_StrnCopyFrom(name1, SegPhys(ds)+reg_dx, DOSNAMEBUF);
		rSpTrim(name1);
		if (DOS_CreateFile(name1, reg_cx, &reg_ax))
			CALLBACK_SCF(false);
		else
			{
			reg_ax = dos.errorcode;
			CALLBACK_SCF(true);
			}
		break;
	case 0x3d:																		// OPEN Open existing file
		{
		Mem_StrnCopyFrom(name1, SegPhys(ds)+reg_dx, DOSNAMEBUF);
		rSpTrim(name1);
		if (DOS_OpenFile(name1, reg_al, &reg_ax))
			CALLBACK_SCF(false);
		else
			{
			reg_ax = dos.errorcode;
			CALLBACK_SCF(true);
			}
		break;
		}
	case 0x3e:																		// CLOSE Close file
		if (DOS_CloseFile(reg_bx))
			CALLBACK_SCF(false);
		else
			{
			reg_ax = dos.errorcode;
			CALLBACK_SCF(true);
			}
		break;
	case 0x3f:																		// READ Read from file or device
		{
		Bit16u toread = reg_cx;
		dos.echo = true;
		if (DOS_ReadFile(reg_bx, dos_copybuf, &toread))
			{
			Mem_CopyTo(SegPhys(ds)+reg_dx, dos_copybuf, toread);
			reg_ax = toread;
			CALLBACK_SCF(false);
			}
		else
			{
			reg_ax = dos.errorcode;
			CALLBACK_SCF(true);
			}
		dos.echo = false;
		}
		break;
	case 0x40:																		// WRITE Write to file or device
		{
		Bit16u towrite = reg_cx;
		Mem_CopyFrom(SegPhys(ds)+reg_dx, dos_copybuf, towrite);
		if (DOS_WriteFile(reg_bx, dos_copybuf, &towrite))
			{
			reg_ax = towrite;
   			CALLBACK_SCF(false);
			}
		else
			{
			reg_ax = dos.errorcode;
			CALLBACK_SCF(true);
			}
		}
		break;
	case 0x41:																		// UNLINK Delete file
		Mem_StrnCopyFrom(name1, SegPhys(ds)+reg_dx, DOSNAMEBUF);
		rSpTrim(name1);
		if (DOS_DeleteFile(name1))
			CALLBACK_SCF(false);
		else
			{
			reg_ax = dos.errorcode;
			CALLBACK_SCF(true);
			}
		break;
	case 0x42:																		// LSEEK Set current file position
		{
		Bit32u pos = (reg_cx<<16) + reg_dx;
		if (DOS_SeekFile(reg_bx, &pos, reg_al))
			{
			reg_dx = (Bit16u)(pos >> 16);
			reg_ax = (Bit16u)(pos & 0xFFFF);
			CALLBACK_SCF(false);
			}
		else
			{
			reg_ax = dos.errorcode;
			CALLBACK_SCF(true);
			}
		}
		break;
	case 0x43:																		// Get/Set file attributes
		Mem_StrnCopyFrom(name1, SegPhys(ds)+reg_dx, DOSNAMEBUF);
		rSpTrim(name1);
		if (reg_al == 0)															// Get
			{
			Bit16u attr_val = reg_cx;
			if (!stricmp(name1, "C:\\AUTOEXEC.BAT") || !stricmp(name1, "4DOS.HLP"))	// 4DOS uses this to test for existance
				{
				dos.errorcode = 0;
				reg_cx = 1;															// Read-only seems right
				CALLBACK_SCF(false);
				}
			else if (DOS_GetFileAttr(name1, &attr_val))
				{
				reg_cx = attr_val;
				CALLBACK_SCF(false);
				}
			else
				{
				reg_ax = dos.errorcode;
				CALLBACK_SCF(true);
				}
			}
		else if (reg_al == 1)														// Set
			{
			if (DOS_SetFileAttr(name1, reg_cx))
				{
				reg_ax = 0x202;														// AX destroyed
				CALLBACK_SCF(false);
				}
			else
				{
				reg_ax = dos.errorcode;
				CALLBACK_SCF(true);
				}
			}
		else
			{
			reg_ax = 1;
			CALLBACK_SCF(true);
			}
		break;
	case 0x44:																		// IOCTL Functions
		if (DOS_IOCTL())
			CALLBACK_SCF(false);
		else
			{
			reg_ax = dos.errorcode;
			CALLBACK_SCF(true);
			}
		break;
	case 0x45:																		// DUP Duplicate file handle
		if (DOS_DuplicateEntry(reg_bx, &reg_ax))
			CALLBACK_SCF(false);
		else
			{
			reg_ax = dos.errorcode;
			CALLBACK_SCF(true);
			}
		break;
	case 0x46:																		// DUP2, FORCEDUP Force duplicate file handle
		if (DOS_ForceDuplicateEntry(reg_bx, reg_cx))
			{
			reg_ax = reg_cx;														// Not all sources agree on it.
			CALLBACK_SCF(false);
			}
		else
			{
			reg_ax = dos.errorcode;
			CALLBACK_SCF(true);
			}
		break;
	case 0x47:																		// CWD Get current directory
		if (DOS_GetCurrentDir(reg_dl, name1))
			{
			Mem_CopyTo(SegPhys(ds)+reg_si, name1, (Bitu)(strlen(name1)+1));	
			reg_ax = 0x0100;
			CALLBACK_SCF(false);
			}
		else
			{
			reg_ax = dos.errorcode;
			CALLBACK_SCF(true);
			}
		break;
	case 0x48:																		// Allocate memory
		{
		Bit16u size = reg_bx;
		Bit16u seg;
		if (DOS_AllocateMemory(&seg, &size))
			{
			reg_ax = seg;
			CALLBACK_SCF(false);
			}
		else
			{
			reg_ax = dos.errorcode;
			reg_bx = size;
			CALLBACK_SCF(true);
			}
		break;
		}
	case 0x49:																		// Free memory
		if (SegValue(es) && DOS_FreeMemory(SegValue(es)))							// No idea why ES=0 with LINK.EXE 3.69
			CALLBACK_SCF(false);
		else
			{            
			reg_ax = dos.errorcode;
			CALLBACK_SCF(true);
			}
		break;
	case 0x4a:																		// Resize memory block
		{
		Bit16u size = reg_bx;
		if (DOS_ResizeMemory(SegValue(es), &size))
			{
			reg_ax = SegValue(es);
			CALLBACK_SCF(false);
			}
		else
			{            
			reg_ax = dos.errorcode;
			reg_bx = size;
			CALLBACK_SCF(true);
			}
		}
		break;
	case 0x4b:																		// EXEC Load and/or execute program
		if (reg_al > 3)
			{
			reg_ax = 1;
			CALLBACK_SCF(true);
			}
		else
			{
			Mem_StrnCopyFrom(name1, SegPhys(ds)+reg_dx, DOSNAMEBUF);
			rSpTrim(name1);
			if (!DOS_Execute(name1, SegPhys(es)+reg_bx, reg_al))
				{
				reg_ax = dos.errorcode;
				CALLBACK_SCF(true);
				}
			else
				CALLBACK_SCF(false);
			WinProgNowait = true;
			}
		break;
	case 0x4c:																		// EXIT Terminate with return code
		DOS_Terminate(dos.psp(), false, reg_al);
		break;
	case 0x4d:																		// Get Return code
		reg_al = dos.return_code;													// Officially read from SDA and clear when read
		reg_ah = dos.return_mode;
		break;
	case 0x4e:																		// FINDFIRST Find first matching file
		{
		Mem_StrnCopyFrom(name1, SegPhys(ds)+reg_dx, DOSNAMEBUF);
		rSpTrim(name1);
		DosPathStripSpaces(name1);													// NB FiAd calls this with 8.3 format
		if (DOS_FindFirst(name1, reg_cx))
			{
			reg_ax = 0;																// Undocumented
			CALLBACK_SCF(false);
			}
		else
			{
			reg_ax = dos.errorcode;
			CALLBACK_SCF(true);
			}
		break;
		}
	case 0x4f:																		// FINDNEXT Find next matching file
		if (DOS_FindNext())
			{
			reg_ax = 0;																// Undocumented:Qbix Willy beamish
			CALLBACK_SCF(false);
			}
		else
			{
			reg_ax = dos.errorcode;
			CALLBACK_SCF(true);
			}
		break;		
	case 0x50:																		// Set current PSP
		dos.psp(reg_bx);
		break;
	case 0x51:																		// Get current PSP
		reg_bx = dos.psp();
		break;
	case 0x52:																		// Get list of lists
		{
		RealPt addr = dos_infoblock.GetPointer();
		SegSet16(es, RealSeg(addr));
		reg_bx = RealOff(addr);
		}
		break;
	case 0x54:																		// Get verify flag
		reg_al = dos.verify ? 1 : 0;
		break;
	case 0x55:																		// Create Child PSP
		DOS_ChildPSP(reg_dx, reg_si);
		dos.psp(reg_dx);
		reg_al = 0xf0;																// AL destroyed
		break;
	case 0x56:																		// Rename file
		Mem_StrnCopyFrom(name1, SegPhys(ds)+reg_dx, DOSNAMEBUF);
		rSpTrim(name1);
		Mem_StrnCopyFrom(name2, SegPhys(es)+reg_di, DOSNAMEBUF);
		rSpTrim(name2);
		if (DOS_Rename(name1, name2))
			CALLBACK_SCF(false);			
		else
			{
			reg_ax = dos.errorcode;
			CALLBACK_SCF(true);
			}
		break;		
	case 0x57:																		// Get/set File's date and time
		if (reg_al == 0)
			CALLBACK_SCF(DOS_GetFileDate(reg_bx, &reg_cx, &reg_dx) ? false : true);
		else if (reg_al == 1)
			CALLBACK_SCF(false);
		break;
	case 0x58:																		// Get/set memory allocation strategy
		switch (reg_al)
			{
		case 0:																		// Get strategy
			reg_ax = DOS_GetMemAllocStrategy();
			break;
		case 1:																		// Set strategy
			if (DOS_SetMemAllocStrategy(reg_bl))									// State is BL, not BX?
				CALLBACK_SCF(false);
			else
				{
				reg_ax = 1;
				CALLBACK_SCF(true);
				}
			break;
		case 2:																		// Get UMB link status
			reg_al = dos_infoblock.GetUMBChainState()&1;
			CALLBACK_SCF(false);
			break;
		case 3:																		// Set UMB link status
			if (DOS_LinkUMBsToMemChain(reg_bl))										// State is BL, not BX?
				CALLBACK_SCF(false);
			else
				{
				reg_ax = 1;
				CALLBACK_SCF(true);
				}
			break;
		default:
			reg_ax = 1;
			CALLBACK_SCF(true);
			}
		break;
	case 0x59:																		// Get extended error information
		reg_ax = dos.errorcode;
		if (dos.errorcode == DOSERR_FILE_NOT_FOUND || dos.errorcode == DOSERR_PATH_NOT_FOUND)
			reg_bh = 8;																// Not found error class (Road Hog)
		else
			reg_bh = 0;																// Unspecified error class
		reg_bl = 1;																	// Retry retry retry
		reg_ch = 0;																	// Unkown error locus
		break;
	case 0x5a:																		// Create temporary file
		{
		Mem_StrnCopyFrom(name1, SegPhys(ds)+reg_dx, DOSNAMEBUF);
		Bit16u handle;
		if (DOS_CreateTempFile(name1, &handle))
			{
			reg_ax = handle;
			Mem_CopyTo(SegPhys(ds)+reg_dx, name1, (Bitu)(strlen(name1)+1));
			CALLBACK_SCF(false);
			}
		else
			{
			reg_ax = dos.errorcode;
			CALLBACK_SCF(true);
			}
		}
		break;
	case 0x5b:																		// Create new file
		{
		Mem_StrnCopyFrom(name1, SegPhys(ds)+reg_dx, DOSNAMEBUF);
		rSpTrim(name1);

		Bit16u handle;
		if (DOS_OpenFile(name1, 0, &handle))										// ??? what about devices ???
			{
			DOS_CloseFile(handle);
			DOS_SetError(DOSERR_FILE_ALREADY_EXISTS);
			reg_ax = dos.errorcode;
			CALLBACK_SCF(true);
			}
		else if (DOS_CreateFile(name1, reg_cx, &handle))
			{
			reg_ax = handle;
			CALLBACK_SCF(false);
			}
		else
			{
			reg_ax = dos.errorcode;
			CALLBACK_SCF(true);
			}
		}
		break;
	case 0x5c:																		// FLOCK File region locking
		{
		if (DOS_LockFile(reg_bx, reg_al, (reg_cx<<16) + reg_dx, (reg_si<<16) + reg_di))
			{
			reg_ax = 0;
			CALLBACK_SCF(false);
			}
		else
			{
			reg_ax = dos.errorcode;
			CALLBACK_SCF(true);
			}
		}
		break;
	case 0x5d:																		// Network Functions
		if (reg_al == 0x06)
			{
			SegSet16(ds, DOS_SDA_SEG);
			reg_si = DOS_SDA_OFS;
//			reg_cx = 0x80;															// Swap if in dos
			reg_cx = 0x78c;															// Found by Edward Mendelson, fixes problems with WP (others) shelling out
			reg_dx = 0x1a;															// Swap always
			break;
			}
	case 0x5e:																		// Network
		if (reg_al == 0)															// Get machine name
			{
			DWORD size = DOSNAMEBUF;
			GetComputerName(name1, &size);
			if (size)
				{
				strcat(name1, "               ");									// Simply add 15 spaces
				if (reg_ip == 0x11e5)												// 4DOS expects it to be 0 terminated (not documented)
					{
					name1[16] = 0;
					Mem_CopyTo(SegPhys(ds)+reg_dx, name1, 17);
					}
				else
					Mem_CopyTo(SegPhys(ds)+reg_dx, name1, 16);
				reg_cx = 0x1ff;														// 01h name valid, FFh NetBIOS number for machine name
				CALLBACK_SCF(false);
				break;
				}
			}
		CALLBACK_SCF(true);
		break;
	case 0x5f:																		// Network redirection
		reg_ax = 1;																	// Failing it
		CALLBACK_SCF(true);
		break; 
	case 0x60:																		// Canonicalize filename or path
		Mem_StrnCopyFrom(name1, SegPhys(ds)+reg_si, DOSNAMEBUF);
		rSpTrim(name1);
		if (DOS_Canonicalize(name1, name2))
			{
			Mem_CopyTo(SegPhys(es)+reg_di, name2, (Bitu)(strlen(name2)+1));	
			CALLBACK_SCF(false);
			}
		else
			{
			reg_ax = dos.errorcode;
			CALLBACK_SCF(true);
			}
		break;
	case 0x62:																		// Get Current PSP Address
		reg_bx = dos.psp();
		break;
	case 0x63:																		// DOUBLE BYTE CHARACTER SET
		if (reg_al == 0)
			{
			SegSet16(ds, RealSeg(dos.tables.dbcs));
			reg_si = RealOff(dos.tables.dbcs);		
			reg_al = 0;
			CALLBACK_SCF(false);													// Undocumented
			}
		else
			reg_al = 0xff;															// Doesn't officially touch carry flag
		break;
	case 0x65:																		// Get extented country information and a lot of other useless shit
		{
		if ((reg_al <= 7) && (reg_cx < 5))											// Todo maybe fully support this for now we set it standard for USA
			{
			DOS_SetError(DOSERR_FUNCTION_NUMBER_INVALID);
			CALLBACK_SCF(true);
			break;
			}
		Bitu len = 0;																// For 0x21 and 0x22
		PhysPt data = SegPhys(es)+reg_di;
		switch (reg_al)
			{
		case 0x01:
			Mem_Stosb(data+0x00, reg_al);
			Mem_Stosw(data+0x01, 0x26);
			if (!countryNo)
				{
				char buffer[128];
				if (GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_ICOUNTRY, buffer, 128))
					countryNo = Bit16u(atoi(buffer));
				else
					countryNo = 1;													// Defaults to 1 (US) if failed
				}
			Mem_Stosw(data+0x03, countryNo);
			if(reg_cx > 0x06)
				Mem_Stosw(data+0x05, dos.loaded_codepage);
			if(reg_cx > 0x08)
				{
				Bitu amount = (reg_cx>=0x29) ? 0x22 : (reg_cx-7);
				Mem_CopyTo(data + 0x07, dos.tables. country, amount);
				reg_cx = (reg_cx>=0x29) ? 0x29 : reg_cx;
				}
			CALLBACK_SCF(false);
			break;
		case 0x05:																	// Get pointer to filename terminator table
			Mem_Stosb(data + 0x00, reg_al);
			Mem_Stosd(data + 0x01, dos.tables.filenamechar);
			reg_cx = 5;
			CALLBACK_SCF(false);
			break;
		case 0x02:																	// Get pointer to uppercase table
			Mem_Stosb(data + 0x00, reg_al);
			Mem_Stosd(data + 0x01, dos.tables.upcase);
			reg_cx = 5;
			CALLBACK_SCF(false);
			break;
		case 0x06:																	// Get pointer to collating sequence table
			Mem_Stosb(data + 0x00, reg_al);
			Mem_Stosd(data + 0x01, dos.tables.collatingseq);
			reg_cx = 5;
			CALLBACK_SCF(false);
			break;
		case 0x03:																	// Get pointer to lowercase table
		case 0x04:																	// Get pointer to filename uppercase table
		case 0x07:																	// Get pointer to double byte char set table
			Mem_Stosb(data + 0x00, reg_al);
			Mem_Stosd(data + 0x01, dos.tables.dbcs);								// Used to be 0
			reg_cx = 5;
			CALLBACK_SCF(false);
			break;
		case 0x20:																	// Capitalize Character
			reg_dl = (Bit8u)toupper(reg_dl);
			CALLBACK_SCF(false);
			break;
		case 0x21:																	// Capitalize String (cx=length)
		case 0x22:																	// Capatilize ASCIZ string
			data = SegPhys(ds) + reg_dx;
			len = (reg_al == 0x21) ? reg_cx : Mem_StrLen(data);
			if (len)
				{
				if (len + reg_dx > DOS_COPYBUFSIZE - 1)								// Is limited to 65535 / within DS
					E_Exit("DOS: 0x65 Buffer overflow");
				Mem_CopyFrom(data, dos_copybuf, len);
				dos_copybuf[len] = 0;
				for (Bitu count = 0; count < len; count++)							// No upcase as String(0x21) might be multiple asciiz strings
					dos_copybuf[count] = (Bit8u)toupper(*reinterpret_cast<unsigned char*>(dos_copybuf+count));
				Mem_CopyTo(data, dos_copybuf, len);
				}
			CALLBACK_SCF(false);
			break;
		default:
			vLog("INT 21-65: Unhandled country information call %2X", reg_al);	
			}
		}
		break;
	case 0x66:
		if (reg_al == 1)															// Get global code page table
			{
			reg_bx = reg_dx = dos.loaded_codepage;									// (For now?) Consider active and system code page being the same
			CALLBACK_SCF(false);
			}
		else
			CALLBACK_SCF(true);
		break;
	case 0x67:																		// Set handle count
		{
		if (reg_bx > 255)															// Limit to max 255
			{
			reg_ax = DOSERR_TOO_MANY_OPEN_FILES;
			CALLBACK_SCF(true);
			}
		else
			{
			DOS_PSP psp(dos.psp());
			psp.SetNumFiles(reg_bx);
			CALLBACK_SCF(false);
			}
		}
		break;
	case 0x68:																		// FFLUSH Commit file
	case 0x6a:
		if (DOS_FlushFile(reg_bl))
			CALLBACK_SCF(false);
		else
			{
			reg_ax = dos.errorcode;
			CALLBACK_SCF(true);
			}
		break;
	case 0x69:																		// Get/set disk serial number
		CALLBACK_SCF(true);															// What about AX = error code?
		break;
	case 0x6c:																		// Extended Open/Create
		Mem_StrnCopyFrom(name1, SegPhys(ds)+reg_si, DOSNAMEBUF);
		rSpTrim(name1);
		if (DOS_OpenFileExtended(name1, reg_bx, reg_cx, reg_dx, &reg_ax, &reg_cx))
			CALLBACK_SCF(false);
		else
			{
			reg_ax = dos.errorcode;
			CALLBACK_SCF(true);
			}
		break;
	default:
		Bit32u ipVal =  Mem_Lodsw(SegPhys(ss)+reg_sp);
		if (ipVal != 0x9539 && ipVal != 0x849d && ipVal != 0x83d1)					// Crude method to exclude 4DOS unhandled calls in log
			vLog("Int 21 unhandled call %4X", reg_ax);
		reg_al = 0;																	// Default value
		break;
		}
	return CBRET_NONE;
	}

static Bitu DOS_20Handler(void)
	{
	reg_ah = 0;
	DOS_21Handler();
	return CBRET_NONE;
	}

static Bitu DOS_27Handler(void)														// Terminate & stay resident
	{
	Bit16u para = (reg_dx+15)/16;
	Bit16u psp = dos.psp();	
	if (DOS_ResizeMemory(psp, &para))
		DOS_Terminate(psp, true, 0);
	return CBRET_NONE;
	}

static Bitu DOS_2526Handler(void)
	{
	reg_ax = 0x0103;
	SETFLAGBIT(CF,true);
    return CBRET_NONE;
	}

static Bitu DOS_28Handler(void)														// DOS idle
	{
	idleCount = idleTrigger;
    return CBRET_NONE;
	}

void DOS_Init()
	{
	CALLBACK_Install(0x20, &DOS_20Handler, CB_IRET);								// DOS Int 20
	CALLBACK_Install(0x21, &DOS_21Handler, CB_IRET_STI);							// DOS Int 21
	CALLBACK_Install(0x25, &DOS_2526Handler, CB_RETF);								// DOS Int 25
	CALLBACK_Install(0x26, &DOS_2526Handler, CB_RETF);								// DOS Int 26
	CALLBACK_Install(0x27, &DOS_27Handler, CB_IRET);								// DOS Int 27
	CALLBACK_Install(0x28, &DOS_28Handler, CB_IRET_STI);								// DOS Int 28 - Idle
	CALLBACK_Install(0x29, NULL, CB_INT29);											// DOS Int 29 - CON output

	DOS_SetupFiles();																// Setup system File tables
	DOS_SetupDevices();																// Setup dos devices
	DOS_SetupTables();
	DOS_SetupMemory(ConfGetBool("low"));											// Setup first MCB
	DOS_SetupMisc();																// Some additional dos interrupts

	char winDirCur[512];															// Initially set C: to Windows work directory
	if (GetCurrentDirectory(512, winDirCur))
		Drives[2] = new DOS_Drive(winDirCur, 2);
	DOS_SDA(DOS_SDA_SEG,DOS_SDA_OFS).SetDrive(2);									// Default drive C:
	DOS_SetDefaultDrive(2);

	dos.version.major = 5;
	dos.version.minor = 0;
	dos.loaded_codepage = GetOEMCP();
	}
