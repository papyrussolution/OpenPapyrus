#include <stdlib.h>
#include "vDos.h"
#include "callback.h"
#include "mem.h"
#include "cpu.h"
#include "paging.h"

/* CallBack are located at 0xF000:0x1000  (see CB_SEG and CB_SOFFSET in callback.h)
   And they are 16 bytes each and you can define them to behave in certain ways like a
   far return or and IRET
*/

CallBack_Handler CallBack_Handlers[CB_MAX];

static Bitu call_stop;

static Bitu illegal_handler(void)
	{
	E_Exit("Illegal callback called");
	return 1;
	}

void CALLBACK_Idle(void)
	{
	// this makes the cpu execute instructions to handle irq's and then come back
	Bitu oldIF = GETFLAG(IF);
	SETFLAGBIT(IF, true);
	Bit16u oldcs = SegValue(cs);
	Bit32u oldeip = reg_eip;
	SegSet16(cs, CB_SEG);
	reg_eip = CB_SOFFSET+call_stop*CB_SIZE;
	RunPC();
	reg_eip = oldeip;
	SegSet16(cs, oldcs);
	SETFLAGBIT(IF, oldIF);
	if (CPU_Cycles > 0) 
		CPU_Cycles = 0;
	}

static Bitu stop_handler(void)
	{
	return CBRET_STOP;
	}

// This way to execute only the INT in a RunPC() and stop
void CALLBACK_RunRealInt(Bit8u intnum)
	{
	Bit32u oldeip = reg_eip;
	Bit16u oldcs = SegValue(cs);
	reg_eip = CB_SOFFSET+(CB_MAX*CB_SIZE)+(intnum*6);
	SegSet16(cs, CB_SEG);
	RunPC();
	reg_eip = oldeip;
	SegSet16(cs, oldcs);
	}

void CALLBACK_SZF(bool val)
	{
	PhysPt addrFlags = SegPhys(ss)+reg_sp+4;
	Bit16u tempFlags = Mem_Lodsw(addrFlags);
	if (val)
		tempFlags |= FLAG_ZF;
	else
		tempFlags &= ~FLAG_ZF;
	Mem_Stosw(addrFlags, tempFlags); 
	}

void CALLBACK_SCF(bool val)
	{
	PhysPt addrFlags = SegPhys(ss)+reg_sp+4;
	Bit16u tempFlags = Mem_Lodsw(addrFlags);
	if (val)
		tempFlags |= FLAG_CF;
	else
		tempFlags &= ~FLAG_CF;
	Mem_Stosw(addrFlags, tempFlags); 
	}

void CALLBACK_SetupDummyIRET(Bit8u intno)
	{
	Mem_Stosd(intno*4, SegOff2dWord(CB_SEG, CB_SOFFSET));
	}

void CALLBACK_SetupExtra(Bitu callback, Bitu type, PhysPt physAddress)
	{
	switch (type)
		{
	case CB_RETF:
		Mem_aStosw(physAddress+0, 0x38FE);											// GRP 4 + Extra Callback instruction
		Mem_aStosw(physAddress+2, callback);										// The immediate word
		Mem_aStosb(physAddress+4, 0xCB);											// RETF
		break;
	case CB_IRET:
		Mem_aStosw(physAddress+0, 0x38FE);											// GRP 4 + Extra Callback instruction
		Mem_aStosw(physAddress+2, callback);										// The immediate word
		Mem_aStosb(physAddress+4, 0xCF);											// IRET
		break;
	case CB_IRET_STI:
		Mem_aStosb(physAddress+0, 0xFB);											// STI
		Mem_aStosw(physAddress+1, 0x38FE);											// GRP 4 + Extra Callback instruction
		Mem_aStosw(physAddress+3, callback);										// The immediate word
		Mem_aStosb(physAddress+5, 0xCF);											// IRET
		Mem_aStosb(physAddress+6, 0xCB);											// RETF (only needed for EXEC)
		Mem_aStosb(physAddress+7, 0xCF);											// IRET
		break;
	case CB_IRET_EOI_PIC1:
		Mem_aStosb(physAddress, 0xCF);												// IRET
		break;
	case CB_IRQ0:																	// Timer int8
		Mem_aStosw(physAddress+0, 0x5250);											// push ax + push dx
		Mem_aStosb(physAddress+2, 0x1e);											// push ds
		Mem_aStosw(physAddress+3, 0x1ccd);											// int 1c
		Mem_aStosb(physAddress+5, 0xfa);											// cli
		Mem_aStosw(physAddress+6, 0x5a1f);											// pop ds + pop dx
		Mem_aStosw(physAddress+8, 0x20b0);											// mov al, 0x20
		Mem_aStosw(physAddress+10, 0x20e6);											// out 0x20, al
		Mem_aStosw(physAddress+12, 0xcf58);											// pop ax + IRET
		break;
/*	case CB_IRQ9:																	// Pic cascade interrupt
		Mem_aStosb(physAddress+0, 0x50);											// push ax
		Mem_aStosw(physAddress+1, 0x61b0);											// mov al, 0x61
		Mem_aStosw(physAddress+3, 0xa0e6);											// out 0xa0, al
		Mem_aStosw(physAddress+5, 0x0acd);											// int a
		Mem_aStosw(physAddress+7, 0x58fa);											// cli + pop ax
		Mem_aStosb(physAddress+9, 0xcf);											// IRET
		break;
*/	case CB_IRQ12:																	// PS2 mouse int74
		Mem_aStosw(physAddress+0, 0xfcfb);											// STI + CLD
		Mem_aStosw(physAddress+2, 0x061e);											// push ds + push es
		Mem_aStosw(physAddress+4, 0x6066);											// pushad
		Mem_aStosw(physAddress+6, 0x38fe);											// GRP 4 + Extra Callback instruction
		Mem_aStosw(physAddress+8, callback);										// The immediate word
		Mem_aStosw(physAddress+10, 0x6166);											// popad
		Mem_aStosw(physAddress+12, 0x1f07);											// pop es + pop ds
		Mem_aStosb(physAddress+14, 0xcf);											// IRET
		break;
	case CB_INT29:																	// Fast console output
		Mem_aStosw(physAddress+0, 0x5350);											// push ax + push bx
		Mem_aStosw(physAddress+2, 0x0eb4);											// mov ah, 0x0e
		Mem_aStosb(physAddress+4, 0xbb);											// mov bx,
		Mem_aStosw(physAddress+5, 7);												// 0x0007
		Mem_aStosw(physAddress+7, 0x10cd);											// int 10
		Mem_aStosw(physAddress+9, 0x585b);											// pop bx + pop ax
		Mem_aStosb(physAddress+11, 0xcf);											// IRET
		break;
	case CB_INT16:																	// Keyboard
		Mem_aStosb(physAddress+0, 0xFB);											// STI
		Mem_aStosw(physAddress+1, 0x38FE);											// GRP 4 + Extra Callback instruction
		Mem_aStosw(physAddress+3, callback);										// The immediate word
		Mem_aStosb(physAddress+5, 0xCF);											// IRET
		Mem_aStosw(physAddress+6, 0xF9eb);											// JMP callback
		break;
	case CB_HOOKABLE:
		Mem_aStosw(physAddress+0, 0x03eb);											// jump near + offset
		Mem_aStosw(physAddress+2, 0x9090);											// NOP + NOP
		Mem_aStosb(physAddress+4, 0x90);											// NOP
		Mem_aStosw(physAddress+5, 0x38FE);											// GRP 4 + Extra Callback instruction
		Mem_aStosw(physAddress+7, callback);										// The immediate word
		Mem_aStosb(physAddress+9, 0xCB);											// RETF
		break;
		}
	}

Bitu CALLBACK_Allocate(void)
	{
	static Bitu cbAllocated = 0;
	return ++cbAllocated;
	}

void CALLBACK_Install(Bit8u intNo, CallBack_Handler handler, Bitu type)
	{
	Bitu cbID = CALLBACK_Allocate();	
	CALLBACK_SetupExtra(cbID, type, CALLBACK_PhysPointer(cbID));
	CallBack_Handlers[cbID] = handler;
	RealSetVec(intNo, CALLBACK_RealPointer(cbID));
	}

void CALLBACK_Setup(Bitu callback, CallBack_Handler handler, Bitu type)
	{
	CALLBACK_SetupExtra(callback, type, CALLBACK_PhysPointer(callback));
	CallBack_Handlers[callback] = handler;
	}

void CALLBACK_Setup(Bitu callback, CallBack_Handler handler, Bitu type, PhysPt addr)
	{
	CALLBACK_SetupExtra(callback, type, addr);
	CallBack_Handlers[callback] = handler;
	}

void CALLBACK_Init()
	{
	for (Bitu i = 0; i < CB_MAX; i++)
		CallBack_Handlers[i] = &illegal_handler;

	Mem_aStosb(CALLBACK_GetBase(), 0xCF);											// Setup dummy (IRET) at first entry

	call_stop = CALLBACK_Allocate();												// Setup the Stop handler
	CallBack_Handlers[call_stop] = stop_handler;
	Mem_aStosw(CALLBACK_PhysPointer(call_stop)+0, 0x38FE);							// GRP 4 + Extra callback instruction
	Mem_aStosw(CALLBACK_PhysPointer(call_stop)+2, (Bit16u)call_stop);

	// Only setup dummy handler for first part of interrupt table
	for (Bit8u ct = 0; ct < 0x60; ct++)
		CALLBACK_SetupDummyIRET(ct);
	for (Bit8u ct = 0x68; ct < 0x70; ct++)
		CALLBACK_SetupDummyIRET(ct);

	// Setup block of 0xCD 0xxx instructions
	PhysPt rint_base = CALLBACK_GetBase()+CB_MAX*CB_SIZE;
	for (Bitu i = 0; i <= 0xff; i++)
		{
		Mem_aStosw(rint_base, (i<<8)|0xcd);											// Int i
		Mem_aStosw(rint_base+2, 0x38FE);											// GRP 4 + Extra Callback instruction
		Mem_aStosw(rint_base+4, (Bit16u)call_stop);
		rint_base += 6;
		}
	}
