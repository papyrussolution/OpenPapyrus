//
//
#include "vDos.h"
#include "mem.h"
#include "dos_inc.h"
#include "regs.h"
#include "callback.h"
#include "cpu.h"
#include "4DOS_img.h"
#include "ttf.h"
/*
#ifdef _MSC_VER
#pragma pack(1)
#endif
*/
struct EXE_Header {
	Bit16u signature;																// EXE Signature MZ
	Bit16u extrabytes;																// Bytes on the last page
	Bit16u pages;																	// Pages in file
	Bit16u relocations;																// Relocations in file
	Bit16u headersize;																// Paragraphs in header
	Bit16u minmemory;																// Minimum amount of memory
	Bit16u maxmemory;																// Maximum amount of memory
	Bit16u initSS;
	Bit16u initSP;
	Bit16u checksum;
	Bit16u initIP;
	Bit16u initCS;
	Bit16u reloctable;																// 40h or greater for new-format (NE, LE, LX, W3, PE, etc.) executable
	Bit16u overlay;
	Bit16u reserved1[4];
	Bit16u oem_id;
	Bit16u oem_info;
	Bit16u reserved2[10];
	Bit32u e_lfanew;
};
/*
#ifdef _MSC_VER
#pragma pack()
#endif
*/
#define MAGIC1 0x5a4d
#define MAXENV 512
#define LOADNGO 0
#define LOAD    1
#define OVERLAY 3

static void SaveRegisters(void)
	{
	reg_sp -= 18;
	PhysPt ss_sp = SegPhys(ss)+reg_sp;
	Mem_Stosw(ss_sp+ 0, reg_ax);
	Mem_Stosw(ss_sp+ 2, reg_cx);
	Mem_Stosw(ss_sp+ 4, reg_dx);
	Mem_Stosw(ss_sp+ 6, reg_bx);
	Mem_Stosw(ss_sp+ 8, reg_si);
	Mem_Stosw(ss_sp+10, reg_di);
	Mem_Stosw(ss_sp+12, reg_bp);
	Mem_Stosw(ss_sp+14, SegValue(ds));
	Mem_Stosw(ss_sp+16, SegValue(es));
	}

static void RestoreRegisters(void)
	{
	PhysPt ss_sp = SegPhys(ss)+reg_sp;
	reg_ax = Mem_Lodsw(ss_sp+ 0);
	reg_cx = Mem_Lodsw(ss_sp+ 2);
	reg_dx = Mem_Lodsw(ss_sp+ 4);
	reg_bx = Mem_Lodsw(ss_sp+ 6);
	reg_si = Mem_Lodsw(ss_sp+ 8);
	reg_di = Mem_Lodsw(ss_sp+10);
	reg_bp = Mem_Lodsw(ss_sp+12);
	SegSet16(ds, Mem_Lodsw(ss_sp+14));
	SegSet16(es, Mem_Lodsw(ss_sp+16));
	reg_sp += 18;
	}

static Bit16u swappedPSP = 0;
static void *swappedAddr;
static Bit32u swappedSize;
static Bit16u swappedSS;
static Bit16u swappedSP;

static int is4DOS = 0;																// 1 = 4DOS, 2 = 4HELP, last executed
Bit16u cpSwap[48];																	// ASCII 176-223, box/line drawing
Bit16u cpLines[48] = {
	0x2591, 0x2592, 0x2593, 0x2502, 0x2524, 0x2561, 0x2562, 0x2556, 0x2555, 0x2563, 0x2551, 0x2557, 0x255d, 0x255c, 0x255b, 0x2510,		// 176 - 223 line/box drawing
	0x2514, 0x2534, 0x252c, 0x251c, 0x2500, 0x253c, 0x255e, 0x255f, 0x255a, 0x2554, 0x2569, 0x2566, 0x2560, 0x2550, 0x256c, 0x2567,
	0x2568, 0x2564, 0x2565, 0x2559, 0x2558, 0x2552, 0x2553, 0x256b, 0x256a, 0x2518, 0x250c, 0x2588, 0x2584, 0x258c, 0x2590, 0x2580
	};

void DOS_Terminate(Bit16u pspseg, bool tsr, Bit8u exitcode)
	{
	if (is4DOS == 2)
		{
		memmove(&cpMap[176], cpSwap, 96);											// Restore ASCII moxlines characters
		TTF_Flush_Cache(ttf.font);													// Rendered glyph cache has to be cleared!
		}
	is4DOS = 0;

	dos.return_code = exitcode;
	dos.return_mode = (tsr) ? (Bit8u)RETURN_TSR : (Bit8u)RETURN_EXIT;
	
	DOS_PSP curpsp(pspseg);
	if (pspseg == curpsp.GetParent())
		return;
	if (!tsr)																		// Free Files owned by process
		{
		closeWinSeachByPSP(pspseg);													// Close pending Window search handles
		curpsp.CloseFiles();
		curpsp.SetNumFiles(20);														// To eventualy release allocated privat DOS memory for extended JFT
		}

	RealPt old22 = curpsp.GetInt22();												// Get the termination address
	curpsp.RestoreVectors();														// Restore vector 22, 23, 24
	dos.psp(curpsp.GetParent());													// Set the parent PSP
	DOS_PSP parentpsp(curpsp.GetParent());

	SegSet16(ss, RealSeg(parentpsp.GetStack()));									// Restore the SS:SP to the previous one
	reg_sp = RealOff(parentpsp.GetStack());

	RestoreRegisters();																// Restore the old CS:IP from int 22h
	Mem_Stosd(SegPhys(ss)+reg_sp, old22);											// Set the CS:IP stored in int 0x22 back on the stack
	Mem_Stosw(SegPhys(ss)+reg_sp+4, 0x7202);										// set IOPL=3 (Strike Commander), nested task set, interrupts enabled, test flags cleared

	if (!tsr)																		// Free memory owned by process
		{
		if (swappedPSP == curpsp.GetParent())
			{
			SegSet16(ss, swappedSS);												// Restore the SS:SP to the previous one
			reg_sp = swappedSP;

			memmove(MemBase+swappedPSP*16-16, swappedAddr, swappedSize);
			free(swappedAddr);
			swappedAddr = NULL;
			swappedPSP = 0;
			swappedSize = 0;
			}
		else
			DOS_FreeProcessMemory(pspseg);
		}
	}

static bool MakeEnv(char *name, Bit16u *segment)
	{
	char namebuf[DOS_PATHLENGTH];
	if (!DOS_Canonicalize(name, namebuf))											// Shouldn't happen, but test now
		return false;																// To prevent memory allocation

	char * envread, * envwrite;														// If segment to copy environment is 0 copy the caller's environment
	if (*segment == 0)
		{
		DOS_PSP psp(dos.psp());
		envread = (char *)(MemBase + (psp.GetEnvironment()<<4));
		}
	else
		envread = (char *)(MemBase + (*segment<<4));

	Bit16u envsize = 1;
	if (envread)
		while (*(envread+envsize))
			envsize += (Bit16u)strlen(envread+envsize)+1;
	envsize++;																		// Account for trailing \0\0 (note we started at offset 1)
	if (envsize+strlen(namebuf)+3 > MAXENV)											// Room to append \1\0filespec\0?
		return false;
	Bit16u size = MAXENV/16;
	if (!DOS_AllocateMemory(segment, &size))
		return false;
	envwrite = (char *)(MemBase + (*segment<<4));
	if (envread)
		memcpy(envwrite, envread, envsize);
	else
		*((Bit16u *)envwrite) = 0;
	envwrite += envsize;
	*((Bit16u *)envwrite) = 1;
	envwrite += 2;
	envread = namebuf;
	while (*envread)																// Pathname should be in uppercase
		*envwrite++ =  toupper(*(envread++));
	*envwrite = 0;
	return true;
	}

// To be removed
bool DOS_NewPSP(Bit16u segment, Bit16u size)
	{
	DOS_PSP psp(segment);
	psp.MakeNew(size);
	Bit16u parent_psp_seg = psp.GetParent();
	DOS_PSP psp_parent(parent_psp_seg);
	psp.CopyFileTable(&psp_parent, false);
	psp.SetCommandTail(SegOff2dWord(parent_psp_seg, 0x80));							// Copy command line as well (Kings Quest AGI -cga switch)
	return true;
	}

bool DOS_ChildPSP(Bit16u segment, Bit16u size)
	{
	DOS_PSP psp(segment);
	psp.MakeNew(size);
	Bit16u parent_psp_seg = psp.GetParent();
	DOS_PSP psp_parent(parent_psp_seg);
	psp.CopyFileTable(&psp_parent, true);
	psp.SetCommandTail(SegOff2dWord(parent_psp_seg, 0x80));
	psp.SetFCB1(SegOff2dWord(parent_psp_seg, 0x5c));
	psp.SetFCB2(SegOff2dWord(parent_psp_seg, 0x6c));
	psp.SetEnvironment(psp_parent.GetEnvironment());
	psp.SetSize(size);
	return true;
	}

static void SetupPSP(Bit16u pspseg, Bit16u memsize, Bit16u envseg)
	{
	DOS_MCB mcb((Bit16u)(pspseg-1));												// Fix the PSP for psp and environment MCB's
	mcb.SetPSPSeg(pspseg);
	mcb.SetPt((Bit16u)(envseg-1));
	mcb.SetPSPSeg(pspseg);

	DOS_PSP psp(pspseg);
	psp.MakeNew(memsize);
	psp.SetEnvironment(envseg);

	DOS_PSP oldpsp(dos.psp());														// Copy file handles
	psp.CopyFileTable(&oldpsp, true);
	}

static void SetupCMDLine(Bit16u pspseg, DOS_ParamBlock & block)
	{
	DOS_PSP psp(pspseg);
	psp.SetCommandTail(block.exec.cmdtail);											// If cmdtail==0 it will inited as empty in SetCommandTail
	}

static bool swapOut()
	{
	Bit16u swapSize = 0;
	Bit16u cur_psp = dos.psp();
	Bit16u mcb_seg = cur_psp-1;

	if (swappedPSP || cur_psp > 0x5000)												// No sense to swap if less then 320K to get or in UMB
		return false;
	DOS_MCB mcb(mcb_seg);
	while (mcb.GetPSPSeg() == cur_psp)												// Walk the MCB's and calculate size
		{
		swapSize += mcb.GetSize()+1;
		mcb_seg = cur_psp+swapSize-1;
		mcb.SetPt(mcb_seg);
		}
	if (mcb.GetType() != 'Z')														// After MCB's program has to free block
		return false;
	Bit16u lastSize = mcb.GetSize();
	if (swapSize < 4096)															// Min 64KB to swap for now also to prevent 4DOS itself to get swapped out
		return false;
	swapSize++;																		// Include last MCB free space
	swapSize++;																		// Include last MCB free space
	swappedSize = swapSize*16;
	if (!(swappedAddr = malloc(swappedSize)))										// PSP is included (no need to)
		return false;
	memmove(swappedAddr, MemBase+cur_psp*16-16, swappedSize);
	swappedPSP = cur_psp;
	swappedSS = SegValue(ss);
	swappedSP = reg_sp;
	mcb_seg = cur_psp-1;
	mcb.SetPt(mcb_seg);
	mcb.SetSize(32);																// PSP size + 256 bytes stack?
	mcb_seg += 33;
	mcb.SetPt(mcb_seg);
	mcb.SetType('Z');																// Last one
	mcb.SetPSPSeg(MCB_FREE);														// Currently free
	mcb.SetSize(lastSize+swapSize-34);
	mcb_seg = cur_psp-1;
	mcb.SetPt(mcb_seg);
	return true;
	}

bool DOS_Execute(char * name, PhysPt block_pt, Bit8u flags)
	{
	EXE_Header head;
	Bit16u fhandle;
	Bit16u fLen;
	Bit32u fPos;
	Bit16u pspseg, envseg, loadseg, memsize, readsize;
	Bitu headersize = 0, imagesize = 0;
	DOS_ParamBlock block(block_pt);
	bool isCom = false;
	bool progSwapped = false;

	block.LoadData();
	if (flags != LOADNGO && flags != OVERLAY && flags != LOAD)
		{
		DOS_SetError(DOSERR_FORMAT_INVALID);
		return false;
		}
	fLen = strlen(name);
	is4DOS = 0;
	if ((fLen > 7 && !stricmp(name+fLen-8, "4DOS.COM"))								// Trap 4DOS.COM and COMMAND.COM
		|| (fLen > 10 && !stricmp(name+fLen-11, "COMMAND.COM")))
		is4DOS = 1;
	else if (fLen > 8 && !stricmp(name+fLen-9, "4HELP.EXE"))						// Trap 4HELP.EXE
		{
		is4DOS = 2;
		memmove(cpSwap, &cpMap[176], 96);											// Save box/line drawing characters
		memmove(&cpMap[176], cpLines, 96);											// Replace for corners HELP screens
		TTF_Flush_Cache(ttf.font);													// Rendered glyph cache has to be cleared!
		}
	fLen = sizeof(EXE_Header);														// Check for EXE or COM File
	if (is4DOS)
		{
		memmove(&head, is4DOS == 1 ? _4DOS_img : _4HELP_img, fLen);
		headersize = head.headersize*16;
		imagesize = head.pages*512-headersize; 
		}
	else
		{
		if (!DOS_OpenFile(name, OPEN_READ, &fhandle))
			return false;
		if (!DOS_ReadFile(fhandle, (Bit8u *)&head, &fLen))
			{
			DOS_SetError(DOSERR_ACCESS_DENIED);
			DOS_CloseFile(fhandle);
			return false;
			}
		if (fLen < sizeof(EXE_Header))
			{
			if (fLen == 0)															// Prevent executing zero byte files
				{
				DOS_SetError(DOSERR_ACCESS_DENIED);
				DOS_CloseFile(fhandle);
				return false;
				}
			isCom = true;															// If less than header size: .com file
			}
		else
			{
			if (head.signature != MAGIC1)
				isCom = true;
			else																	// Test if Windows program
				{
				fPos = 0;
				DOS_SeekFile(fhandle, &fPos, DOS_SEEK_END);
				if (head.e_lfanew < fPos-2)
					{
					fPos = head.e_lfanew;
					DOS_SeekFile(fhandle, &fPos, DOS_SEEK_SET);
					fLen = 2;														// Check for PE signature
					Bit16u PEsign;
					DOS_ReadFile(fhandle, (Bit8u *)&PEsign, &fLen);
					if (fLen == 2 && fPos == head.e_lfanew && PEsign == 0x4550)
						{															// Init.. always 0 with Windows? Q&D
						DOS_CloseFile(fhandle);
						char comline[256];
						char winDirCur[512];										// Setting Windows directory to DOS drive+current directory
						char winDirNew[512];										// and calling the program
						char winName[256];
						Bit8u drive;
						DOS_MakeName(name, winDirNew, &drive);						// Mainly to get the drive and pathname w/o it
						if (GetCurrentDirectory(512, winDirCur))
							{
							strcpy(winName, Drives[drive]->GetWinDir());
							strcat(winName, winDirNew);
							strcpy(winDirNew, Drives[DOS_GetDefaultDrive()]->GetWinDir());	// Windows directory of DOS drive
							strcat(winDirNew, Drives[DOS_GetDefaultDrive()]->curdir);	// Append DOS current directory
							if (SetCurrentDirectory(winDirNew))
								{
								PhysPt comPtr = SegOff2Ptr(Mem_Lodsw(block_pt+4), Mem_Lodsw(block_pt+2));
								memset(comline, 0, 256);
								Mem_CopyFrom(comPtr+1, comline, Mem_Lodsb(comPtr));	// Get commandline, directories are supposed Windows at this moment!
WinProgNowait = false;
//								int ret = _spawnl(WinProgNowait, winName, winName, comline, NULL);
								int ret = _spawnl(P_NOWAIT, winName, winName, comline, NULL);
								SetCurrentDirectory(winDirCur);
//								if (WinProgNowait)
//									ret = 0; // P_NOWAIT returns process handle
								if (ret > 0) // Got a process handle
									if (WinProgNowait)
										ret = 0; // If we don't wait, return
									else { // Else wait for completion
										DWORD exitCode = 0;
										while (GetExitCodeProcess((HANDLE)ret, &exitCode) && exitCode == STILL_ACTIVE) {
											Sleep(10);
											HandleUserActions();
											}
										dos.return_code = (Bit8u)(exitCode&255);
										dos.return_mode = 0;
										ret = 0;
										}
								else // No process handle returned
									ret = errno;
								DOS_SetError(ret);
								return ret == 0;
								}
							}
						DOS_SetError(DOSERR_FILE_NOT_FOUND); // Just pick one
						return false;
						}
					}
				head.pages &= 0x07ff;
				headersize = head.headersize*16;
				imagesize = head.pages*512-headersize; 
				if (imagesize+headersize < 512)
					imagesize = 512-headersize;
				}
			}
		}
	if (flags != OVERLAY)
		{
		Bit16u minsize = 18;
		Bit16u maxsize = 0xffff;
		if (isCom)
			{
			fPos = 0;																// Reduce minimum of needed memory size to filesize
			DOS_SeekFile(fhandle, &fPos, DOS_SEEK_END);
			if (fPos > 0xff00)														// Maximum file size is 64KB - 256 bytes
				{
				DOS_CloseFile(fhandle);
				DOS_SetError(DOSERR_FORMAT_INVALID);
				return false;
				}
			minsize = (Bit16u)(fPos>>4)+16;
			}
		else
			{
			minsize = long2para(imagesize+(head.minmemory<<4)+256);					// Exe size calculated from header
			if (head.maxmemory != 0)
				maxsize = long2para(imagesize+(head.maxmemory<<4)+256);
			}
		Bit16u maxfree = 0xffff;
		envseg = block.exec.envseg;
		if (is4DOS == 1)
			progSwapped = swapOut();
		if (!MakeEnv(name, &envseg))												// Create an environment block
			{
			if (!is4DOS)
				DOS_CloseFile(fhandle);
			return false;
			}
if (DOS_GetMemAllocStrategy()&0xc0)				// If to load in UMB
	{
	Bit16u UMBtotal, UMBlargest, UMBcount;
	if (DOS_GetFreeUMB(&UMBtotal, &UMBlargest, &UMBcount))
		if (maxsize == 0xffff && minsize <= UMBlargest)
			maxfree = UMBlargest;
	}
		if (maxfree == 0xffff)
			{
			DOS_AllocateMemory(&pspseg, &maxfree);
			if (maxfree < minsize)
				{
				if (!is4DOS)
					DOS_CloseFile(fhandle);
				DOS_FreeMemory(envseg);
				DOS_SetError(DOSERR_INSUFFICIENT_MEMORY);
				return false;
				}
			}
		memsize = MIN(maxfree, maxsize);
		if (!DOS_AllocateMemory(&pspseg, &memsize))
			E_Exit("DOS: Exec error in memory");
		loadseg = pspseg+16;
		if (!isCom)
			if ((head.minmemory == 0) && (head.maxmemory == 0))						// Load program into upper part of allocated memory?
				loadseg = (Bit16u)(((pspseg+memsize)*0x10-imagesize)/0x10);
		}
	else
		{
		if (isCom)																	// Can't overlay a .COM
			{
			DOS_SetError(DOSERR_FORMAT_INVALID);
			DOS_CloseFile(fhandle);
			return false;
			}
		loadseg = block.overlay.loadseg;
		}
	Bit8u* loadaddress = MemBase+(loadseg<<4);										// Load the executable

	if (isCom)																		// COM Load 64k - 256 bytes max
		{
		fPos = 0;
		DOS_SeekFile(fhandle, &fPos, DOS_SEEK_SET);	
		readsize = 0xffff-256;
		DOS_ReadFile(fhandle, loadaddress, &readsize);
		}
	else																			// Load EXE in 32kb blocks and then relocates
		{
	if (progSwapped)
	{
	SetupPSP(pspseg, memsize, envseg);												// Create psp after closing exe, to avoid dead file handle of exe in copied psp
	SetupCMDLine(pspseg, block);
	}
		if (is4DOS)
			memmove(loadaddress, (is4DOS == 1 ? _4DOS_img : _4HELP_img)+headersize, imagesize);
		else
			{
			fPos = headersize;
			DOS_SeekFile(fhandle, &fPos, DOS_SEEK_SET);
			while (readsize = MIN(0x8000, imagesize))
				{
				imagesize -= readsize;												// Sometimes imagsize != filesize
				DOS_ReadFile(fhandle, loadaddress, &readsize);
				loadaddress += readsize;
				}
			}
		if (head.relocations)
			{																		// Relocate the exe image
			Bit16u relocate;
			if (flags == OVERLAY)
				relocate = block.overlay.relocation;
			else
				relocate = loadseg;
			fPos = head.reloctable;
			RealPt * relocpts = new Bit32u[head.relocations];
			readsize = 4*head.relocations;
			if (is4DOS)
				memmove(relocpts,  (is4DOS == 1 ? _4DOS_img : _4HELP_img)+fPos, readsize);							// Simplify me (include the loop below)
			else
				{
				DOS_SeekFile(fhandle, &fPos, DOS_SEEK_SET);
				DOS_ReadFile(fhandle, (Bit8u *)relocpts, &readsize);
				}
			for (int i = 0; i < head.relocations; i++)
				{
				PhysPt address = SegOff2Ptr(RealSeg(relocpts[i])+loadseg, RealOff(relocpts[i]));
				Mem_Stosw(address, Mem_Lodsw(address)+relocate);
				}
			delete[] relocpts;
			}
		}
	if (!is4DOS)
		DOS_CloseFile(fhandle);

	if (flags == OVERLAY)
		return true;																// Everything done for overlays

	if (!progSwapped)
	{
	SetupPSP(pspseg, memsize, envseg);												// Create psp after closing exe, to avoid dead file handle of exe in copied psp
	SetupCMDLine(pspseg, block);
	}

	RealPt csip, sssp;
	if (isCom)
		{
		csip = SegOff2dWord(pspseg, 0x100);
		sssp = SegOff2dWord(pspseg, 0xfffe);
		Mem_Stosw(SegOff2Ptr(pspseg, 0xfffe), 0);
		}
	else
		{
		csip = SegOff2dWord(loadseg+head.initCS, head.initIP);
		sssp = SegOff2dWord(loadseg+head.initSS, head.initSP);
		}
	if (flags == LOAD)
		{
		SaveRegisters();
		DOS_PSP callpsp(dos.psp());
		// Save the SS:SP on the PSP of calling program
		callpsp.SetStack(RealMakeSeg(ss, reg_sp));
		reg_sp += 18;
		// Switch the psp's
		dos.psp(pspseg);
		DOS_PSP newpsp(dos.psp());
		dos.dta(SegOff2dWord(newpsp.GetSegment(), 0x80));
		// First word on the stack is the value ax should contain on startup
		Mem_Stosw(RealSeg(sssp-2), RealOff(sssp-2), 0xffff);
		block.exec.initsssp = sssp-2;
		block.exec.initcsip = csip;
		block.SaveData();
		return true;
		}

	if (flags == LOADNGO)
		{
		// Get Caller's program CS:IP of the stack and set termination address to that
		RealSetVec(0x22, SegOff2dWord(Mem_Lodsw(SegPhys(ss)+reg_sp+2), Mem_Lodsw(SegPhys(ss)+reg_sp)));
		if (progSwapped)
			{
			reg_sp = 0xfe;
			SegSet16(ss, pspseg+16);
			}
		SaveRegisters();
		DOS_PSP callpsp(dos.psp());
		// Save the SS:SP on the PSP of calling program
		callpsp.SetStack(RealMakeSeg(ss, reg_sp));
		// Switch the psp's and set new DTA
		dos.psp(pspseg);
		DOS_PSP newpsp(dos.psp());
		dos.dta(SegOff2dWord(newpsp.GetSegment(), 0x80));
		// save vectors
		newpsp.SaveVectors();
		// copy fcbs
		newpsp.SetFCB1(block.exec.fcb1);
		newpsp.SetFCB2(block.exec.fcb2);
		// Set the stack for new program
		SegSet16(ss, RealSeg(sssp));
		reg_sp = RealOff(sssp);
		// Add some flags and CS:IP on the stack for the IRET
		CPU_Push16(RealSeg(csip));
		CPU_Push16(RealOff(csip));
		/* DOS starts programs with a RETF, so critical flags
		 * should not be modified (IOPL in v86 mode);
		 * interrupt flag is set explicitly, test flags cleared */
		reg_flags = (reg_flags&(~FMASK_TEST))|FLAG_IF;
		// Jump to retf so that we only need to store cs:ip on the stack
		reg_ip++;
		// Setup the rest of the registers
		reg_ax = reg_bx = 0;
		reg_cx = 0xff;
		reg_dx = pspseg;
		reg_si = RealOff(csip);
		reg_di = RealOff(sssp);
		reg_bp = 0x91c;																// DOS internal stack begin relict
		SegSet16(ds, pspseg);
		SegSet16(es, pspseg);
		char bareName[8] = { 0 };													// Add the filename to PSP and environment's MCB
		if (is4DOS)
			strcpy(bareName, is4DOS == 1 ? "4DOS" : "4HELP");
		else
			{
			int i = 0;
			for (char *ptr = name; *ptr; ptr++)
				{
				switch (*ptr)
					{
				case ':': case '\\':
					i = 0;
					memset(bareName, 0, 8);
					break;
				case '.':
					i = 8;
					break;
				default:
					if (i < 8)
						bareName[i++] = (char)toupper(*ptr);
					}
				}
			}
		DOS_MCB pspmcb(dos.psp()-1);
		pspmcb.SetFileName(bareName);
		return true;
		}
	return false;
	}
