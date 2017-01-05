#ifndef VDOS_H
#define VDOS_H

#include "config.h"
#include <io.h>
#include <windows.h>
#include "logging.h"

void E_Exit(const char * message,...);

extern char vDosVersion[];

extern bool do_debug;

void RunPC();
bool ConfGetBool(const char *name);
int ConfGetInt(const char *name);
char * ConfGetString(const char *name);
void ConfAddError(char* desc, char* errLine);
void vDos_Init(void);

void INT2F_Cont(void);
void showAbout(void);

extern HWND	vDosHwnd;

#define MAX_PATH_LEN 512 // Maximum filename size

#define txtMaxCols	160
#define txtMaxLins	60

#define DOS_FILES 255

extern bool winHidden;
extern bool usesMouse;
extern int wpVersion;
extern int codepage;
extern bool printTimeout;

extern Bit8u initialvMode;

#define idleTrigger 5																// When to sleep
extern int idleCount;
extern bool idleSkip;																// Don't sleep!

extern Bit32s CPU_Cycles;
#define CPU_CyclesLimit 32768
#define PIT_TICK_RATE 1193182


extern Bitu lastOpcode;

extern bool ISR;																	// Interrupt Service Routine

extern bool WinProgNowait;															// Should Windows program return immediately (not called from command.com)

#endif
