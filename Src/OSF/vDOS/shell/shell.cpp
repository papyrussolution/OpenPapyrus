#include <stdlib.h>
#include "vDos.h"
#include "regs.h"
#include "callback.h"
#include "dos_inc.h"

Bitu call_shellstop;

static Bitu shellstop_handler(void)
	{
	return CBRET_STOP;
	}

void Execute(char* name, char* args)
	{
	reg_sp -= 0x200;																// Allocate some stack space for tables in physical memory
		
	DOS_ParamBlock block(SegPhys(ss)+reg_sp);										// Add Parameter block
	block.Clear();
	RealPt file_name = RealMakeSeg(ss, reg_sp+0x20);								// Add a filename
	Mem_CopyTo(dWord2Ptr(file_name), name, (Bitu)(strlen(name)+1));

	CommandTail cmdtail;															// Fill the command line
	memset(&cmdtail.buffer, 0, 127);												// Else some part of the string is unitialized (valgrind)
	cmdtail.count = (Bit8u)strlen(args);
	memcpy(cmdtail.buffer, args, strlen(args));
	cmdtail.buffer[strlen(args)] = 0xd;
		
	Mem_CopyTo(SegPhys(ss)+reg_sp+0x100, &cmdtail, 128);							// Copy command line in stack block too
																					// Parse FCB (first two parameters) and put them into the current DOS_PSP
	Bit8u add;
	FCB_Parsename(dos.psp(), 0x5C, 0x00, cmdtail.buffer, &add);
	FCB_Parsename(dos.psp(), 0x6C, 0x00, &cmdtail.buffer[add], &add);
	block.exec.fcb1 = SegOff2dWord(dos.psp(), 0x5C);
	block.exec.fcb2 = SegOff2dWord(dos.psp(), 0x6C);
	block.exec.cmdtail = RealMakeSeg(ss, reg_sp+0x100);								// Set the command line in the block and save it
	block.SaveData();

	reg_ax = 0x4b00;																// Start up a dos execute interrupt
	SegSet16(ds, SegValue(ss));														// Filename pointer
	reg_dx = RealOff(file_name);
	SegSet16(es, SegValue(ss));														// Paramblock
	reg_bx = reg_sp;
	SETFLAGBIT(IF, false);
	WinProgNowait = false;
	CALLBACK_RunRealInt(0x21);
	reg_sp += 0x200;																// Restore CS:IP and the stack
	}

void SHELL_Init()
	{
	// Regular startup
	call_shellstop = CALLBACK_Allocate();
	CALLBACK_Setup(call_shellstop, shellstop_handler, CB_IRET);						// Shell stop
	// Setup the startup CS:IP to kill the last running machine when exitted
	RealPt newcsip = CALLBACK_RealPointer(call_shellstop);
	SegSet16(cs, RealSeg(newcsip));
	reg_ip = RealOff(newcsip);

	// Now call up the shell for the first time
	Bit16u psp_seg = DOS_FIRST_SHELL;
	Bit16u env_seg = DOS_FIRST_SHELL+19;
	//DOS_GetPrivatMemory(1+(4096/16))+1;
	Bit16u stack_seg = DOS_GetPrivatMemory(2048/16);
	SegSet16(ss, stack_seg);
	reg_sp = 2046;

	// Set up int 24 and psp (Telarium games)
	Mem_aStosb((DOS_FIRST_SHELL+16+1)<<4, 0xea);									// far jmp
	Mem_aStosd(((DOS_FIRST_SHELL+16+1)<<4)+1, Mem_aLodsd(0x24*4));
	Mem_aStosd(0x24*4, ((Bit32u)psp_seg<<16)|((16+1)<<4));

	// Set up int 23 to "int 20" in the psp. Fixes what.exe
	Mem_aStosd(0x23*4, ((Bit32u)psp_seg<<16));

	// Setup MCBs
	DOS_MCB pspmcb((Bit16u)(psp_seg-1));
	pspmcb.SetPSPSeg(psp_seg);														// MCB of the command shell psp
	pspmcb.SetSize(0x10+2);
	pspmcb.SetType(0x4d);
	DOS_MCB envmcb((Bit16u)(env_seg-1));
	envmcb.SetPSPSeg(psp_seg);														// MCB of the command shell environment
//	envmcb.SetSize(DOS_MEM_START-env_seg);
	envmcb.SetSize(32);
	envmcb.SetType(0x4d);
	
	// Setup environment
	char env2[512];
	int total = 0;
	strcpy(env2, "WIN_USERNAME=");
	int size = GetEnvironmentVariable("USERNAME", env2+strlen(env2), 128);			// Add Windows USERNAME=
	if (size > 0 && size < 128)
		total = strlen(env2)+1;
	strcpy(env2+total, "WIN_VDOS=");
	size = GetEnvironmentVariable("VDOS", env2+total+9, 128);						// Add Windows VDOS=
	if (size == 0 || size > 127)
		strcpy(env2+total+9, "not_set");
	total += strlen(env2+total)+1;
	char env1[] = "COMSPEC=C:\\COMMAND.COM\0PATH=C:\\\0PROMPT=$P$G\0\0\1\0C:\\COMMAND.COM\0";	// The standard settings
	memcpy(env2+total, env1, sizeof(env1));
	Mem_CopyTo(SegOff2Ptr(env_seg, 0), env2, total+sizeof(env1));

	DOS_PSP psp(psp_seg);
	psp.MakeNew(0);
	dos.psp(psp_seg);

	/* The start of the file handle array in the psp must look like this:
	 * 01 01 01 00 02
	 * In order to achieve this: First open 2 files. Close the first and
	 * duplicate the second (so the entries get 01) */
	Bit16u dummy = 0;
	DOS_OpenFile("CON", OPEN_READWRITE, &dummy);									// STDIN
	DOS_OpenFile("CON", OPEN_READWRITE, &dummy);									// STDOUT
	DOS_CloseFile(0);																// Close STDIN
	DOS_ForceDuplicateEntry(1, 0);													// "new" STDIN
	DOS_ForceDuplicateEntry(1, 2);													// STDERR
	DOS_OpenFile("CON", OPEN_READWRITE, &dummy);									// STDAUX
	DOS_OpenFile("PRN", OPEN_READWRITE, &dummy);									// STDPRN (troubles me with FiAd, accessing it in subprograms and getting closed)

	psp.SetParent(psp_seg);

	psp.SetEnvironment(env_seg);													// Set the environment

	CommandTail tail;																// Set the command line for the shell start up
	strcpy(tail.buffer, "/INIT");
	tail.count = (Bit8u)strlen(tail.buffer);
	Mem_CopyTo(SegOff2Ptr(psp_seg, 128), &tail, 128);
	
	dos.dta(SegOff2dWord(psp_seg, 0x80));											// Setup internal DOS Variables
	dos.psp(psp_seg);

	Execute("C:\\COMMAND.COM", GetFileAttributes("AUTOEXEC.TXT") == INVALID_FILE_ATTRIBUTES ? "" : " C:\\AUTOEXEC.BAT");
	}
