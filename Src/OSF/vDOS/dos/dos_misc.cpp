#include "vDos.h"
#include "callback.h"
#include "mem.h"
#include "regs.h"
#include "dos_inc.h"
#include "../ints/xms.h" // "../src/ints/xms.h"-->"../ints/xms.h"
#include "video.h"

static void INT2F_Cont() // @sobolev @stub
{
}

static Bitu INT2F_Handler(void)
{
	switch(reg_ax) {
		case 0x1000: // SHARE.EXE installation check
			reg_al = 0xff; // Pretend it's installed.
			break;
		case 0x1600: // MS Windows - WINDOWS ENHANCED MODE INSTALLATION CHECK
			reg_ax = 0; // Just failing doesn't reset fWin95 flag 4DOS
			break;
		case 0x1680: // Release current virtual machine time-slice
			idleCount = idleTrigger;
			break;
		case 0x1a00: // ANSI.SYS installation check
			reg_al = 0xff;
			break;
		case 0x4300: // XMS installed check
			if (TotXMSMB)
				reg_al = 0x80;
			break;
		case 0x4310: // XMS handler seg:offset
			if(TotXMSMB) {
				SegSet16(es, RealSeg(xms_callback));
				reg_bx = RealOff(xms_callback);
			}
			break;			
		case 0x4a01: // Query free HMA space
		case 0x4a02: // Allocate HMA space
			reg_bx = 0; // Number of bytes available in HMA
			SegSet16(es, 0xffff); // ES:DI = ffff:ffff HMA not used
			reg_di = 0xffff;
			break;
		case 0xb800: // Network - installation check
			reg_al = 1; // Installed
			reg_bx = 8; // Bit 3 - redirector
			break;
		case 0xb809: // Network - get version
			reg_ax = 0x0201; // Major-minor version as returned by NTVDM-Windows XP
			break;
		case 0x150b: // CD-ROM v2.00+ - DRIVE CHECK
		case 0x168f: // Windows95 - CLOSE-AWARENESS
		case 0x4680: // MS Windows v3.0 - INSTALLATION CHECK
		case 0xb706: // DOS 4.0+ APPEND - GET APPEND FUNCTION STATE
		case 0xd44d: // 4DOS - Retrieve information from previous shell
		case 0xd44e: // 4DOS - AWAITING USER INPUT
			break; // Dummy to prevent log as unhandled
		default:
			INT2F_Cont();
	}
	return CBRET_NONE;
}

static Bitu INT2A_Handler(void)
{
	return CBRET_NONE;
}

void DOS_SetupMisc(void)
{
	CALLBACK_Install(0x2f, &INT2F_Handler, CB_IRET_STI); // DOS Int 2f - Multiplex
	CALLBACK_Install(0x2a, &INT2A_Handler, CB_IRET); // DOS Int 2a - Network
}
