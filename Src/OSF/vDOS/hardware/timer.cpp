#include "vDos.h"
#include "inout.h"
#include "Mmsystem.h"

LARGE_INTEGER PerfFreq;
LARGE_INTEGER PerfCount;
LARGE_INTEGER PerfStart;

static inline void BIN2BCD(Bit16u& val)
	{
	Bit16u temp = val%10+(((val/10)%10)<<4)+(((val/100)%10)<<8)+(((val/1000)%10)<<12);
	val = temp;
	}

static inline void BCD2BIN(Bit16u& val)
	{
	Bit16u temp = (val&0x0f)+((val>>4)&0x0f)*10+((val>>8)&0x0f)*100+((val>>12)&0x0f)*1000;
	val = temp;
	}

static struct PIT_Block {
	Bitu cntr;

	Bit16u read_latch;
	Bit16u write_latch;

	Bit8u mode;
	Bit8u read_state;
	Bit8u write_state;

	bool bcd;
	bool go_read_latch;
	bool new_mode;
}pit0;

static void counter_latch()
	{
	pit0.go_read_latch = false;														// Fill the read_latch with current count

	if (pit0.new_mode)
		{
		pit0.read_latch = 0;
		return;
		}
	QueryPerformanceCounter(&PerfCount);
	LONGLONG ticks = ((PerfCount.QuadPart-PerfStart.QuadPart)*PIT_TICK_RATE/PerfFreq.QuadPart);
	if (pit0.mode == 1)																// Countdown
		{
        if (ticks > pit0.cntr)
			pit0.read_latch = pit0.cntr;											// Unconfirmed
        else
            pit0.read_latch = pit0.cntr-(Bit16u)ticks;
		}
	else																			// Counter keeps on counting after passing terminal count
		pit0.read_latch = pit0.cntr-(Bit16u)(ticks%pit0.cntr);
	}

static void write_latch(Bitu port, Bitu val)
	{
	if (pit0.bcd)
		BIN2BCD(pit0.write_latch);
	switch (pit0.write_state)
		{
	case 0:
		pit0.write_latch |= ((val&0xff)<<8);
		pit0.write_state = 3;
		break;
	case 1:
		pit0.write_latch = (pit0.write_latch&0xff00)|(val&0xff);
		break;
	case 2:
		pit0.write_latch = (pit0.write_latch&0xff)|((val&0xff)<<8);
		break;
	case 3:
		pit0.write_latch = val&0xff;
		pit0.write_state = 0;
		break;
		}
	if (pit0.bcd)
		BCD2BIN(pit0.write_latch);
   	if (pit0.write_state != 0)
		{
		if (pit0.write_latch == 0)
			pit0.cntr = pit0.bcd ? 9999 : 0x10000;
		else
			pit0.cntr = pit0.write_latch;
		if (!pit0.new_mode && pit0.mode == 2)										// In mode 2 writing another value has no direct effect on the count
			return;																	// until the old one has run out. This might apply to other modes too.
		QueryPerformanceCounter(&PerfStart);
		pit0.new_mode = false;
		}
	}

static Bit8u read_latch(Bitu port)
	{
	Bit8u ret;
	if (pit0.go_read_latch) 
		counter_latch();

	if(pit0.bcd)
		BIN2BCD(pit0.read_latch);
	switch (pit0.read_state)
		{
	case 0:																			// Read MSB & return to state 3
		ret = (pit0.read_latch>>8) & 0xff;
		pit0.read_state = 3;
		pit0.go_read_latch = true;
		break;
	case 3:																			// Read LSB followed by MSB
		ret = pit0.read_latch&0xff;
		pit0.read_state = 0;
		break;
	case 1:																			// Read LSB
		ret = pit0.read_latch&0xff;
		pit0.go_read_latch = true;
		break;
	case 2:																			// Read MSB
		ret = (pit0.read_latch>>8)&0xff;
		pit0.go_read_latch = true;
		break;
		}
	if(pit0.bcd)
		BCD2BIN(pit0.read_latch);
	return ret;
	}

static void write_p43(Bitu /*port*/, Bitu val)
	{
	if (!(val&0xc0))																// Channel 0
		{
		if ((val&0x30) == 0)
			counter_latch();														// Counter latch command
		else
			{
			counter_latch();														// Save the current count value to be re-used in undocumented newmode
			pit0.bcd = (val&1) != 0;   
			if (pit0.bcd && pit0.cntr >= 9999)
				pit0.cntr = 9999;
			QueryPerformanceCounter(&PerfStart);
			pit0.go_read_latch = true;
			pit0.read_state = pit0.write_state = (val>>4)&0x03;
			pit0.mode = (val>>1)&0x07;
			if (pit0.mode > 5)
				pit0.mode -= 4;														// 6, 7 become 2 and 3
			pit0.new_mode = true;
			}
		}
	}

/*
// It all doesn't work, only beep and I want to use this, it's a synchronous function, the program is paused for the duration of the sound
static int beepDur = 0;
static bool beepLSB = 1;
static void write_p42(Bitu port, Bitu val)
	{
	if (beepLSB)
		beepDur = val;
	else
		{
		beepDur |= val<<8;
		if (beepDur > 100)
			beepDur = 100;
		if (beepDur > 0)
//			MessageBeep(0xFFFFFFFF);								// MessageBeep() returns not supported!
		PlaySound((LPCTSTR)SND_ALIAS_SYSTEMEXCLAMATION, NULL, SND_ASYNC|SND_ALIAS_ID );	// mostly too long
//		Beep(1750, beepDur);										// Too introsive and the program pauses
		}
	beepLSB = !beepLSB;
	}
*/
void TIMER_Init()
	{
	IO_RegisterWriteHandler(0x40, write_latch);
	IO_RegisterWriteHandler(0x43, write_p43);
//	IO_RegisterWriteHandler(0x42, write_p42);
	IO_RegisterReadHandler(0x40, read_latch);
	// Setup Timer 0
	pit0.cntr = 0x10000;
	pit0.write_state = pit0.read_state = 3;
	pit0.read_latch = pit0.write_latch = 0;
	pit0.mode = 3;
	pit0.bcd = false;
	pit0.go_read_latch = true;
	QueryPerformanceFrequency(&PerfFreq);
	}
