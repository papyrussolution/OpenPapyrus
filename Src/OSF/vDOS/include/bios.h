#ifndef VDOS_BIOS_H
#define VDOS_BIOS_H

#define BIOS_BASE_ADDRESS				0x400

#define BIOS_BASE_ADDRESS_COM1          0x400
#define BIOS_BASE_ADDRESS_COM2          0x402
#define BIOS_BASE_ADDRESS_COM3          0x404
#define BIOS_BASE_ADDRESS_COM4          0x406
#define BIOS_ADDRESS_LPT1               0x408
#define BIOS_ADDRESS_LPT2               0x40a
#define BIOS_ADDRESS_LPT3               0x40c

#define BIOS_CONFIGURATION              0x410

#define BIOS_MEMORY_SIZE                0x413
#define BIOS_TRUE_MEMORY_SIZE           0x415
#define BIOS_KEYBOARD_FLAGS1            0x417
#define BIOS_KEYBOARD_FLAGS2            0x418
#define BIOS_KEYBOARD_TOKEN             0x419

#define BIOS_KEYBOARD_BUFFER_HEAD       0x41a
#define BIOS_KEYBOARD_BUFFER_TAIL       0x41c
#define BIOS_KEYBOARD_BUFFER            0x41e

#define BIOS_DRIVE_ACTIVE               0x43e
#define BIOS_DRIVE_RUNNING              0x43f
#define BIOS_DISK_MOTOR_TIMEOUT         0x440
#define BIOS_DISK_STATUS                0x441

#define BIOS_VIDEO_MODE					0x449
#define BIOS_SCREEN_COLUMNS             0x44a
#define BIOS_VIDEO_PAGE_SIZE			0x44c
#define BIOS_VIDEO_MEMORY_ADDRESS       0x44e
#define BIOS_VIDEO_CURSOR_POS	        0x450

#define BIOS_CURSOR_SHAPE               0x460
#define BIOS_CURSOR_LAST_LINE           0x460
#define BIOS_CURSOR_FIRST_LINE          0x461
#define BIOS_CURRENT_SCREEN_PAGE        0x462
#define BIOS_VIDEO_PORT                 0x463
#define BIOS_VDU_CONTROL                0x465
#define BIOS_VDU_COLOR_REGISTER         0x466

#define BIOS_TIMER                      0x46c
#define BIOS_24_HOURS_FLAG              0x470
#define BIOS_KEYBOARD_FLAGS             0x471
#define BIOS_CTRL_ALT_DEL_FLAG          0x472
#define BIOS_HARDDISK_COUNT				0x475

#define BIOS_LPT1_TIMEOUT               0x478
#define BIOS_LPT2_TIMEOUT               0x479
#define BIOS_LPT3_TIMEOUT               0x47a

#define BIOS_COM1_TIMEOUT               0x47c
#define BIOS_COM2_TIMEOUT               0x47d
#define BIOS_COM3_TIMEOUT               0x47e
#define BIOS_COM4_TIMEOUT               0x47f

#define BIOS_KEYBOARD_BUFFER_START      0x480
#define BIOS_KEYBOARD_BUFFER_END        0x482

#define BIOS_ROWS_ON_SCREEN_MINUS_1     0x484
#define BIOS_FONT_HEIGHT                0x485

#define BIOS_VIDEO_INFO_0               0x487
#define BIOS_VIDEO_INFO_1               0x488
#define BIOS_VIDEO_INFO_2               0x489
#define BIOS_VIDEO_COMBO                0x48a

#define BIOS_KEYBOARD_FLAGS3            0x496
#define BIOS_KEYBOARD_LEDS              0x497

#define BIOS_WAIT_FLAG_POINTER          0x498
#define BIOS_WAIT_FLAG_COUNT	        0x49c		
#define BIOS_WAIT_FLAG_ACTIVE			0x4a0
#define BIOS_WAIT_FLAG_TEMP				0x4a1

#define BIOS_VS_POINTER					0x4a8

#define BIOS_DEFAULT_HANDLER_LOCATION	0xfff53
#define BIOS_DEFAULT_IRQ0_LOCATION		0xf000fea5
#define BIOS_DEFAULT_IRQ1_LOCATION		0xf000e987
#define BIOS_DEFAULT_IRQ2_LOCATION		0xf000ff55

#define MAX_SCAN_CODE					0x58										// Maximum of scancodes handled by keyboard bios routines

void BIOS_PasteClipboard(Bit16u * data);											// To paste Windows clipboard to keyboard buffer
bool BIOS_AddKeyToBuffer(Bit16u code);
void BIOS_AddKey(Bit8u flags1, Bit8u falgs2, Bit8u flags, Bit8u scancode, Bit16u unicode, bool pressed);

bool BIOS_CheckKey(Bit16u &code);
bool BIOS_GetKey(Bit16u &code);

void BIOS_SetComPorts (Bit16u baseaddr[]);
void BIOS_SetLPTPort (Bitu port, Bit16u baseaddr);

bool BIOS_HostTimeSync(void);

#endif
