#ifndef _events_h
#define _events_h

#include "config.h"

// General event structure
typedef struct {
	Bit16u	type;
	Bit8u	flags1;																	// Bios data flags
	Bit8u	flags2;
	Bit8u	flags3;
	Bit8u	leds;
	Bit8u	flags;																	// Extra flags bit 0: extended key, bit 1: Windows key down
	Bit8u	scancode;																// Hardware specific scancode
	Bit8u	virtKey;																// Windows virtual key
	Bit16u	unicode;																// Translated character
	Bit16u	x, y;																	// The X/Y coordinates of the mouse (at press time)
} userAction;


// Polls for currently pending events, and returns true if there are any pending
extern bool vPollEvent(userAction *uAct);

#endif
