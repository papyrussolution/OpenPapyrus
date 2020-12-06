//
//
//
#include "vDos.h"
#include "video.h"
#include "cpu.h"
#include "callback.h"
#include "parport.h"
#include "support.h"
#include "bios.h"
#include "mouse.h"
#include "vga.h"
#include "paging.h"

char vDosVersion[] = "2015.11.01";

// The whole load of startups for all the subfunctions
void GUI_StartUp();
void MEM_Init();
//void PAGING_Init();
void IO_Init();
void CALLBACK_Init();
void VGA_Init();
void DOS_Init();
void CPU_Init();
void KEYBOARD_Init();
void MOUSE_Init();
void SERIAL_Init();
void PIC_Init();
void TIMER_Init();
void BIOS_Init();
// void CMOS_Init();
void PARALLEL_Init();
// Dos Internal mostly
void XMS_Init();
void EMS_Init();
void SHELL_Init();
void INT10_Init();

static Bit32u mSecsLast = 0;
bool winHidden = true;
bool usesMouse;
int wpVersion; // 1 - 99 (mostly 51..62, negative value will exclude some WP additions)
Bit8u initialvMode = 3; // Initial videomode, 3 = color, 7 = Hercules for as far it works
int codepage = 0; // Current code page, defaults to Windows OEM
bool printTimeout; // Should the spoolfile timeout?
bool WinProgNowait = true;
bool idleSkip = false; // Don't try to sleep!
Bitu lastOpcode;
Bit32s CPU_Cycles =     0;

static Bit32u prevWinRefresh;
bool ISR;
static bool intPending = false;

static Bit32u int8_var, int1c, int74;

void RunPC(void)
{
	while(1) {
		while(CPU_Cycles > 0) {
			if(BIOS_HostTimeSync() && !ISR)
				intPending = true; // New timer tick
			if(GETFLAG(IF)) // (hardware) Interrupts handled
//			if (GETFLAG(IF) && !cpu.pmode && !PAGING_Enabled())						//
// (hardware) Interrupts handled (extended check)
				if(intPending) {
					intPending = false;
					if((Mem_aLodsb(4*8+3)|Mem_aLodsb(4*0x1c+3)) != 0xf0) { // And Int 8 or 1C replaced
						ISR = true;
						CPU_HW_Interrupt(8); // Setup executing Int 8 (IRQ0)
					}
				}
				else if(mouse_event_type)
					CPU_HW_Interrupt(0x74); // Setup executing Int 74 (Mouse)
			Bits ret = (*cpudecoder)();
			if(ret < 0)
				return;
			if(ret > 0 && (*CallBack_Handlers[ret])())
				return;
		}
		HandleUserActions();
		Bit32u mSecsNew = GetTickCount();
		if(mSecsNew >= prevWinRefresh+40) {                                                                                     // 25
		                                                                                                                        // refreshes
		                                                                                                                        // per
		                                                                                                                        // second
			prevWinRefresh = mSecsNew;
			VGA_VerticalTimer();
		}
		if(mSecsNew <= mSecsLast+55) { // To be real save???
			LPT_CheckTimeOuts(mSecsNew);
			if(!idleSkip)
				Sleep(idleCount >= idleTrigger ? 2 : 0); // If idleTrigger or more repeated idle keyboard requests or int 28h called, sleep fixed (CPU usage drops down)
		}
		mSecsLast = mSecsNew;
		idleCount = 0;
		idleSkip = false;
		CPU_Cycles = CPU_CyclesLimit;
	}
}

#define LENNAME 10 // Max length of name
#define MAXNAMES 50 // Max number of names
#define MAXSTRLEN 8192 // Max storage of all config strings

enum Vtype { V_BOOL, V_INT, V_STRING};

static int confEntries = 0; // Entries so far
static char confStrings[MAXSTRLEN];
static unsigned int confStrOffset = 0; // Offset to store new strings
static char errorMess[600];

static struct {
	char name[LENNAME+1];
	Vtype type;
	bool set;
	union {bool _bool; int _int; char * _string;} value;
} ConfSetting[MAXNAMES];

// No checking on add/get functions, we call them ourselves...
void ConfAddBool(const char * name, bool value)
{
	strcpy(ConfSetting[confEntries].name, name);
	ConfSetting[confEntries].type = V_BOOL;
	ConfSetting[confEntries].value._bool = value;
	confEntries++;
}

void ConfAddInt(const char * name, int value)
{
	strcpy(ConfSetting[confEntries].name, name);
	ConfSetting[confEntries].type = V_INT;
	ConfSetting[confEntries].value._int = value;
	confEntries++;
}

void ConfAddString(const char * name, char* value)
{
	strcpy(ConfSetting[confEntries].name, name);
	ConfSetting[confEntries].type = V_STRING;
	ConfSetting[confEntries].value._string = value;
	confEntries++;
}

static int findEntry(const char * name)
{
	for(int found = 0; found < confEntries; found++)
		if(!stricmp(name, ConfSetting[found].name))
			return found;
	return -1;
}

bool ConfGetBool(const char * name)
{
	int entry = findEntry(name);
	if(entry >= 0)
		return ConfSetting[entry].value._bool;
	return false;                                                                                                                                   // To
	                                                                                                                                                // satisfy
	                                                                                                                                                // compiler
}

int ConfGetInt(const char * name)
{
	int entry = findEntry(name);
	if(entry >= 0)
		return ConfSetting[entry].value._int;
	return 0;                                                                                                                                               // To
	                                                                                                                                                        // satisfy
	                                                                                                                                                        // compiler
}

char * ConfGetString(const char * name)
{
	int entry = findEntry(name);
	if(entry >= 0)
		return ConfSetting[entry].value._string;
	return "";                                                                                                                                              // To
	                                                                                                                                                        // satisfy
	                                                                                                                                                        // compiler
}

static char* ConfSetValue(const char* name, char* value)
{
	int entry = findEntry(name);
	if(entry == -1)
		return "No valid option\n";
	if(ConfSetting[entry].set)
		return "Option already set\n";
	ConfSetting[entry].set = true;
	switch(ConfSetting[entry].type)
	{
		case V_BOOL:
		    if(!stricmp(value, "on")) {
			    ConfSetting[entry].value._bool = true;
			    return NULL;
		    }
		    else if(!stricmp(value, "off")) {
			    ConfSetting[entry].value._bool = false;
			    return NULL;
		    }
		    break;
		case V_INT:
	    {
		    int testVal = atoi(value);
		    char testStr[32];
		    sprintf(testStr, "%d", testVal);
		    if(!strcmp(value, testStr)) {
			    ConfSetting[entry].value._int = testVal;
			    return NULL;
		    }
		    break;
	    }
		case V_STRING:
		    if(strlen(value) >= MAXSTRLEN-confStrOffset)
			    return "vDos ran out of space to store settings\n";
		    strcpy(confStrings+confStrOffset, value);
		    ConfSetting[entry].value._string = confStrings+confStrOffset;
		    confStrOffset += strlen(value)+1;
		    return NULL;
	}
	return "Invalid value for this option\n";
}

static char * ParseConfigLine(char * line)
{
	char * val = strchr(line, '=');
	if(!val)
		return "= assignment missing\n";
	*val = 0;
	char * name = lrTrim(line);
	if(!strlen(name))
		return "Option name missing\n";
	val = lrTrim(val+1);
	if(!strlen(val))
		return "Option value missing\n";
	if(strlen(name) == 4 && (!strnicmp(name, "LPT", 3) || !strnicmp(name, "COM", 3)) && (name[3] > '0' && name[3] <= '9')) {
		ConfAddString(name, val);
		ConfSetValue(name, val); // Have to use this, ConfAddString() uses static ref to val!
		return NULL;
	}
	return ConfSetValue(name, val);
}

void ConfAddError(char* desc, char* errLine)
{
	static bool addMess = true;
	if(addMess) {
		if(strlen(errLine) > 40)
			strcpy(errLine+37, "...");
		strcat(strcat(strcat(errorMess, "\n"), desc), errLine);
		if(strlen(errorMess) > 500) { // Don't flood the MesageBox with error lines
			strcat(errorMess, "\n...");
			addMess = false;
		}
	}
}

void ParseConfigFile()
{
	char * parseRes;
	char lineIn[1024];
	FILE * cFile;
	errorMess[0] = 0;
	if(!(cFile = fopen("config.txt", "r")))
		return;
	while(fgets(lineIn, 1023, cFile)) {
		char * line = lrTrim(lineIn);
		if(strlen(line) && !(!strnicmp(line, "rem", 3) && (line[3] == 0 || line[3] == 32 || line[3] == 9)))     // Filter out rem ...
			if(parseRes = ParseConfigLine(line))
				ConfAddError(parseRes, line);
	}
	fclose(cFile);
}

void vDos_Init(void)
{
	window.hideTill = GetTickCount()+2500; // Auto hidden till first keyboard check, parachute at 2.5 secs
	vLog("vDos version: %s", vDosVersion);
	ConfAddInt("scale", 0);
	ConfAddString("window", "");
	ConfAddBool("low", false);
	ConfAddString("xmem", "");
	ConfAddString("colors", "");
	ConfAddBool("mouse", false);
	ConfAddInt("lins", 25);
	ConfAddInt("cols", 80);
	ConfAddBool("frame", false);
	ConfAddBool("timeout", true);
	ConfAddString("font", "");
	ConfAddString("wp", "");
	ConfAddInt("euro", -1);
	ParseConfigFile();

	GUI_StartUp();
	IO_Init();
//	PAGING_Init();
	MEM_Init();
	CALLBACK_Init();
	PIC_Init();
	TIMER_Init();
//	CMOS_Init();
	VGA_Init();
	CPU_Init();
	KEYBOARD_Init();
	BIOS_Init();
	INT10_Init();
	MOUSE_Init();
	SERIAL_Init();
	PARALLEL_Init();
	printTimeout = ConfGetBool("timeout");
	DOS_Init();
	XMS_Init();
	EMS_Init();
	if(errorMess[0])
		MessageBox(NULL, errorMess+1, "vDos: CONFIG.TXT has unresolved items", MB_OK|MB_ICONWARNING);
	SHELL_Init(); // Start up main machine
}
