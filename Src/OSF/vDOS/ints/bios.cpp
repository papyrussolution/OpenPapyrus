#include "vDos.h"
#include "mem.h"
#include "bios.h"
#include "regs.h"
#include "cpu.h"
#include "callback.h"
#include "inout.h"
#include "serialport.h"
#include "parport.h"

static Bitu INT1A_Handler(void)
	{
	switch (reg_ah)
		{
	case 0x00:																		// Get system time
		{
		reg_dx = Mem_Lodsw(BIOS_TIMER);
		reg_cx = Mem_Lodsw(BIOS_TIMER+2);
		reg_al = Mem_aLodsb(BIOS_24_HOURS_FLAG);
		Mem_Stosb(BIOS_24_HOURS_FLAG, 0);											// Reset the midnight flag
		break;
		}
	case 0x01:																		// Set system time, we don't
		Mem_Stosb(BIOS_24_HOURS_FLAG, 0);											// Reset the midnight flag
		break;
	case 0x02:																		// Get real-time clock time in BCD (AT, XT286, PS)
		{
		_SYSTEMTIME systime;														// Windows localdate/time
		GetLocalTime(&systime);
		reg_ch = (Bit8u)(systime.wHour/10*16+systime.wHour%10);
		reg_cl = (Bit8u)(systime.wMinute/10*16+systime.wMinute%10);
		reg_dh = (Bit8u)(systime.wSecond/10*16+systime.wSecond%10);
		reg_dl = 0;																	// Daylight saving disabled
		CALLBACK_SCF(false);
		break;
		}
	case 0x04:																		// Get real-time clock date in BCD (AT, XT286, PS)
		{
		_SYSTEMTIME systime;														// Windows localdate/time
		GetLocalTime(&systime);
		reg_ch = (Bit8u)(systime.wYear/1000*16+((systime.wYear/100)%10));			// Centuries
		reg_cl = (Bit8u)((systime.wYear%100)/10*16+systime.wYear%10);				// Years
		reg_dh = (Bit8u)(systime.wMonth/10*16+systime.wMonth%10);					// Months
		reg_dl = (Bit8u)(systime.wDay/10*16+systime.wDay%10);						// Days
		CALLBACK_SCF(false);
		break;
		}
		}
	return CBRET_NONE;
	}	

static Bitu INT11_Handler(void)
	{
	reg_ax = Mem_aLodsw(BIOS_CONFIGURATION);
	return CBRET_NONE;
	}

bool BIOS_HostTimeSync()
	{
	_SYSTEMTIME systime;															// Windows localdate/time
	GetLocalTime(&systime);
	dos.date.day = (Bit8u)systime.wDay;
	dos.date.month = (Bit8u)systime.wMonth;
	dos.date.year = (Bit16u)systime.wYear;

	Bit32u ticks = (Bit32u)(((double)(
		systime.wHour*3600*1000+
		systime.wMinute*60*1000+
		systime.wSecond*1000+ 
		systime.wMilliseconds))*(((double)PIT_TICK_RATE/65536.0)/1000.0));
	if (ticks == Mem_aLodsd(BIOS_TIMER))
		return false;																// Already in sync			
	Mem_aStosd(BIOS_TIMER, ticks);
	return true;
	}

static Bitu INT12_Handler(void)
	{
	reg_ax = Mem_aLodsw(BIOS_MEMORY_SIZE);
	return CBRET_NONE;
	}

static Bitu INT17_Handler(void)
	{
	if (reg_ah > 2 || reg_dx > 2)													// 0-2 printer port functions, no more than 3 parallel ports
		return CBRET_NONE;

	switch(reg_ah)
		{
	case 0:																			// Write character
		parallelPorts[reg_dx]->Putchar(reg_al);
		break;
	case 1:																			// Initialize port
		parallelPorts[reg_dx]->initialize();
		break;
		};
	reg_ah = parallelPorts[reg_dx]->getPrinterStatus();
	return CBRET_NONE;
	}

static Bitu INT13_DiskHandler(void)
	{
	reg_ah = 1;																		// No low level disk IO
	CALLBACK_SCF(true);
	return CBRET_NONE;
	}

static Bitu INT14_Handler(void)
	{
	if (reg_ah > 3 || reg_dx > 3)													// 0-3 serial port functions, no more than 4 serial ports
		return CBRET_NONE;
	
	switch (reg_ah)
		{
	case 0:																			// Initialize port
	case 3:																			// Get status
		{
		reg_ah = 0;																	// Line status
		reg_al = 0x10;																// Modem status, Clear to send
		break;
		}
	case 1:																			// Transmit character
		serialPorts[reg_dx]->Putchar(reg_al);
		reg_ah = 0;																	// Line status
		break;
	case 2:																			// Read character
		reg_ah = 0x80;																// Nothing received
		break;
		}
	return CBRET_NONE;
	}

static Bitu INT15_Handler(void)
	{
	static Bit16u biosConfigSeg = 0;
	CALLBACK_SCF(false);
	switch (reg_ah)
		{
	case 0xC0:																		// Get Configuration
		{
		if (biosConfigSeg == 0)
			biosConfigSeg = DOS_GetPrivatMemory(1);									// We have 16 bytes
		PhysPt data	= SegOff2Ptr(biosConfigSeg, 0);
		Mem_Stosw(data, 8);															// 8 Bytes following
		Mem_Stosb(data+2, 0xFC);													// Model ID (PC)
		Mem_Stosb(data+3, 0x02);													// Submodel ID
		Mem_Stosb(data+4, 0x74);													// Bios Revision
		Mem_Stosb(data+5, 0x70);													// Feature Byte 1
		Mem_Stosw(data+6, 0);														// Feature Byte 2 + 3
		Mem_Stosw(data+8, 0);														// Feature Byte 4 + 5
		CPU_SetSegGeneral(es, biosConfigSeg);
		reg_bx = 0;
		reg_ah = 0;
		}
		break;
	case 0x87:																		// Copy extended memory
		{
		PhysPt data = SegPhys(es)+reg_si;
		PhysPt source = (Mem_Lodsd(data+0x12)&0x00FFFFFF)+(Mem_Lodsb(data+0x16)<<24);
		PhysPt dest = (Mem_Lodsd(data+0x1A)&0x00FFFFFF)+(Mem_Lodsb(data+0x1E)<<24);
		Mem_rMovsb(dest, source, reg_cx*2);
		reg_ax = 0;
		break;
		}
	case 0x88:																		// SYSTEM - Get extended memory size in KB (286+)
		reg_ax = TotEXTMB*1024;
		break;
	case 0xc9:																		// BIOS: Get cpu type and mask revision
		reg_ah = 0;
		reg_cx = 0x2304;															// CL, mask revision (stepping level) = 04h = Intel A0?
		break;
	default:
		if (reg_ax == 0xE801														// Phoenix BIOS v4.0 - GET MEMORY SIZE FOR >64M CONFIGURATIONS
			&& reg_sp == 0xc4ac && Mem_Lodsw(SegPhys(ss)+reg_sp) == 0xf423)			// 4DOS signature
			{																		// To fool it and not access CMOS
			reg_ax = 64;
			reg_bx = TotEXTMB*1024/64-1;											// We report bare Extended memory (not/excluding XMS)!
			}
		else
			{
			vLog("Int 15 unhandled call %4X", reg_ax);
			reg_ah = 0x86;
			CALLBACK_SCF(true);
			}
		}
	return CBRET_NONE;
	}

void BIOS_SetupKeyboard(void);

// set com port data in bios data area
// parameter: array of 4 com port base addresses, all 4 are set!
void BIOS_SetComPorts(Bit16u baseaddr[])
	{
	for (Bitu i = 0; i < 4; i++)
		Mem_aStosw(BIOS_BASE_ADDRESS_COM1 + i*2, baseaddr[i]);
	// set equipment word
	Bit16u equipmentword = Mem_aLodsw(BIOS_CONFIGURATION);
	equipmentword &= (~0x0E00);
	equipmentword |= (4 << 9);
	Mem_aStosw(BIOS_CONFIGURATION, equipmentword);
	}

void BIOS_SetLPTPort(Bitu port, Bit16u baseaddr)									// Port is always < 3
	{
	Mem_aStosw(BIOS_ADDRESS_LPT1 + port*2, baseaddr);
	Mem_aStosb(BIOS_LPT1_TIMEOUT + port, 10);
	// set equipment word: count ports
	Bit16u portcount = 0;
	for (Bitu i = 0; i < 3; i++)
		if (Mem_aLodsw(BIOS_ADDRESS_LPT1 + i*2) != 0)
			portcount++;
	Bit16u equipmentword = Mem_aLodsw(BIOS_CONFIGURATION);
	equipmentword &= (~0xC000);
	equipmentword |= (portcount << 14);
	Mem_aStosw(BIOS_CONFIGURATION, equipmentword);
	}

void BIOS_Init()
	{
	Mem_rStosb(0x400, 0, 0x200);													// Clear the Bios Data Area (0x400-0x5ff, 0x600- is accounted to DOS)

	// Setup all the interrupt handlers the Bios controls
	CALLBACK_SetupExtra(0, CB_IRQ0, dWord2Ptr(BIOS_DEFAULT_IRQ0_LOCATION));
	RealSetVec(0x08, BIOS_DEFAULT_IRQ0_LOCATION);
	BIOS_HostTimeSync();															// Initialize the timer ticks

	CALLBACK_Install(0x11, &INT11_Handler, CB_IRET);								// INT 11 Get equipment list

	CALLBACK_Install(0x12, &INT12_Handler, CB_IRET);								// INT 12 Memory size default at 640 kb
	Mem_aStosw(BIOS_MEMORY_SIZE, 640);
		
	CALLBACK_Install(0x13, &INT13_DiskHandler, CB_IRET_STI);						// INT 13 BIOS Disk support
	Mem_Stosb(BIOS_HARDDISK_COUNT, 2);												// Setup the Bios Area

	CALLBACK_Install(0x14, &INT14_Handler, CB_IRET_STI);							// INT 14 Serial ports

	CALLBACK_Install(0x15, &INT15_Handler, CB_IRET_STI);							// INT 15 Misc calls

	BIOS_SetupKeyboard();															// INT 16 Keyboard handled in another file

	CALLBACK_Install(0x17, &INT17_Handler, CB_IRET_STI);							// INT 17 Printer Routines

	CALLBACK_Install(0x1a, &INT1A_Handler, CB_IRET_STI);							// INT 1A Time and some other functions

//	These are already setup as dummy by CALLBACK_Init()
//	CALLBACK_SetupDummyIRET(0x1c);													// INT 1C System timer tick called from INT 8
//	CALLBACK_Install(0x71, NULL, CB_IRQ9);											// Irq 9 rerouted to irq 2
//	CALLBACK_SetupDummyIRET(0x18);													// Reboot dummies
//	CALLBACK_SetupDummyIRET(0x19);

	// set system BIOS entry point too
	Mem_aStosb(0xffff0, 0xEA);														// FAR JMP
	Mem_aStosd(0xffff1, Mem_Lodsd(0x18*4));

	Bitu cbID = CALLBACK_Allocate();												// Irq 2
	CALLBACK_Setup(cbID, NULL, CB_IRET_EOI_PIC1, dWord2Ptr(BIOS_DEFAULT_IRQ2_LOCATION));
	RealSetVec(0x0a, BIOS_DEFAULT_IRQ2_LOCATION);

	Mem_aStosb(BIOS_DEFAULT_HANDLER_LOCATION, 0xcf);								// BIOS default interrupt vector location -> IRET

	Mem_aWrites(0xfff00, "vDos BIOS",  9);											// System BIOS identification
	Mem_aWrites(0xffff5, "01/01/99\x00\xfc\x24",  11);								// System BIOS date + signature

	Mem_rStosb(BIOS_COM1_TIMEOUT, 1, BIOS_COM4_TIMEOUT-BIOS_COM1_TIMEOUT+1);		// Port timeouts
	}
