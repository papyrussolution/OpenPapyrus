#ifndef VDOS_CALLBACK_H
#define VDOS_CALLBACK_H

#include "mem.h"

typedef Bitu (*CallBack_Handler)(void);
extern CallBack_Handler CallBack_Handlers[];

enum {CB_RETF, CB_IRET, CB_IRET_STI, CB_IRET_EOI_PIC1,
		CB_IRQ0, CB_IRQ1, /*CB_IRQ9,*/ CB_IRQ12,
		CB_INT29, CB_INT16, CB_HOOKABLE};

#define CB_MAX		128
#define CB_SIZE		32
#define CB_SEG		0xF000
#define CB_SOFFSET	0x1000

enum {CBRET_NONE=0, CBRET_STOP=1};

static RealPt CALLBACK_RealPointer(Bitu callback)
	{
	return SegOff2dWord(CB_SEG, (Bit16u)(CB_SOFFSET+callback*CB_SIZE));
	}

static PhysPt CALLBACK_PhysPointer(Bitu callback)
	{
	return SegOff2Ptr(CB_SEG, (Bit16u)(CB_SOFFSET+callback*CB_SIZE));
	}

static PhysPt CALLBACK_GetBase(void)
	{
	return (CB_SEG<<4)+CB_SOFFSET;
	}

Bitu CALLBACK_Allocate();

void CALLBACK_Idle(void);

void CALLBACK_RunRealInt(Bit8u intnum);

void CALLBACK_Install(Bit8u intNo, CallBack_Handler handler, Bitu type);
void CALLBACK_Setup(Bitu callback,CallBack_Handler handler,Bitu type);
void CALLBACK_Setup(Bitu callback,CallBack_Handler handler,Bitu type,PhysPt addr);
void CALLBACK_SetupExtra(Bitu callback, Bitu type, PhysPt physAddress);

void CALLBACK_SCF(bool val);
void CALLBACK_SZF(bool val);

#endif
