#include "vDos.h"
#include "callback.h"
#include "mem.h"
#include "regs.h"
#include "cpu.h"
#include "mouse.h"
#include "inout.h"
#include "int10.h"
#include "bios.h"
#include "dos_inc.h"

#define MOUSE_BUTTONS 2

static struct {
	Bit8u	buttons;
	Bit16u	times_pressed[MOUSE_BUTTONS];
	Bit16u	times_released[MOUSE_BUTTONS];
	Bit16u	last_released_x[MOUSE_BUTTONS];
	Bit16u	last_released_y[MOUSE_BUTTONS];
	Bit16u	last_pressed_x[MOUSE_BUTTONS];
	Bit16u	last_pressed_y[MOUSE_BUTTONS];
	Bit16u	x, y;
	Bit16u	sub_seg, sub_ofs;
	Bit16u	sub_mask;
} mouse;

bool mouseWP6x;																		// WP6.x with Mouse Driver (Absolute/Pen)

Bit8u	mouse_event_type;
Bit8u	mouse_event_buttons;

#define MOUSE_HAS_MOVED			1
#define MOUSE_LEFT_PRESSED		2
#define MOUSE_LEFT_RELEASED		4
#define MOUSE_RIGHT_PRESSED		8
#define MOUSE_RIGHT_RELEASED	16
#define MOUSE_MIDDLE_PRESSED	32
#define MOUSE_MIDDLE_RELEASED	64


void Mouse_AddEvent(Bit8u type)
	{
	if (!(mouse.sub_mask&type))														// No sense to go into the Int trouble if not handled by replaced mouse driver
		return;																		// NB, sub_mask = 0 with the internal driver
	if (mouse_event_type && type == 1)												// If mouse move and some mouse operation pending, drop
		return;
	mouse_event_type = type;
	mouse_event_buttons = mouse.buttons;
	}

void Mouse_Moved(Bit16u x, Bit16u y, Bit16u scale_x, Bit16u scale_y)
	{
	Bit16u oldX = mouse.x;
	Bit16u oldY = mouse.y;
	if (CurMode->type == M_TEXT)
		{
		mouse.x = x*8/scale_x;
		mouse.y = y*(mouseWP6x ? 16 : 8)/scale_y;									// WP6.x with Mouse Driver (Absolute/Pen) expects times 2
		}
	else
		{
		mouse.x = x/scale_x;
		mouse.y = y/scale_y;
		}
	if ((mouse.x-oldX)|(mouse.y-oldY))
		Mouse_AddEvent(MOUSE_HAS_MOVED);
	}

void Mouse_ButtonPressed(Bit8u button)												// NB button is already 0 (left) or 1 (right)
	{
	mouse.buttons |= button+1;
	mouse.times_pressed[button]++;
	mouse.last_pressed_x[button] = mouse.x;
	mouse.last_pressed_y[button] = mouse.y;
	Mouse_AddEvent(button == 0 ? MOUSE_LEFT_PRESSED : MOUSE_RIGHT_PRESSED);
	}

void Mouse_ButtonReleased(Bit8u button)												// NB button is already 0 (left) or 1 (right)
	{
	mouse.buttons &= ~(button+1);
	mouse.times_released[button]++;	
	mouse.last_released_x[button] = mouse.x;
	mouse.last_released_y[button] = mouse.y;
	Mouse_AddEvent(button == 0 ? MOUSE_LEFT_RELEASED : MOUSE_RIGHT_RELEASED);
	}

static Bitu INT33_Handler(void)
	{
	switch (reg_ax)
		{
	case 0x00:																		// Reset driver and read status
		mouse.sub_mask = mouse.sub_seg = mouse.sub_ofs = 0;
		mouseWP6x = false;															// Signature for WP6.x with Mouse Driver (Absolute/Pen)
	case 0x21:																		// Software Reset
		reg_ax = 0xffff;
		reg_bx = MOUSE_BUTTONS;
		break;
	case 0x01:																		// Show Mouse
	case 0x02:																		// Hide Mouse
		break;
	case 0x03:																		// Return position and button status
		reg_bx = mouse.buttons;
		reg_cx = mouse.x;
		reg_dx = mouse.y;
		break;
	case 0x04:																		// Position mouse, we don't
		break;
	case 0x05:																		// Return button press data
		{
		reg_ax = mouse.buttons;
		Bit16u but = reg_bx;
		if (but >= MOUSE_BUTTONS)
			reg_bx = reg_cx = reg_dx = 0;
		else
			{
			reg_cx = mouse.last_pressed_x[but];
			reg_dx = mouse.last_pressed_y[but];
			reg_bx = mouse.times_pressed[but];
			mouse.times_pressed[but] = 0;
			}
		break;
		}
	case 0x06:																		// Return button release data
		{
		reg_ax = mouse.buttons;
		Bit16u but = reg_bx;
		if (but >= MOUSE_BUTTONS)
			reg_bx = reg_cx = reg_dx = 0;
		else
			{
			reg_cx = mouse.last_released_x[but];
			reg_dx = mouse.last_released_y[but];
			reg_bx = mouse.times_released[but];
			mouse.times_released[but] = 0;
			}
		break;
		}
	case 0x07:																		// Define horizontal cursor range
	case 0x08:																		// Define vertical cursor range
	case 0x09:																		// Define GFX Cursor (removed)
	case 0x0a:																		// Define Text Cursor
		break;
	case 0x0b:																		// Read motion data, we don't do mickeys
		reg_cx = reg_dx = 0;
		break;
	case 0x0c:																		// Define interrupt subroutine parameters
		mouse.sub_mask = reg_cx;
		mouse.sub_seg = SegValue(es);
		mouse.sub_ofs = reg_dx;
		mouseWP6x = reg_dx == 0x1b6 ? true : false;									// Signature for WP6.x with Mouse Driver (Absolute/Pen)
		break;
	case 0x0f:																		// Define mickey/pixel rate
	case 0x10:																		// Define screen region for updating
	case 0x13:																		// Set double-speed threshold
 		break;
	case 0x14:																		// Exchange event-handler
		{
		Bit16u oldMask = mouse.sub_mask;
		Bit16u oldOfs = mouse.sub_ofs;
		Bit16u oldSeg = mouse.sub_seg;
		// Set new values 
		mouse.sub_mask = reg_cx;
		mouse.sub_seg = SegValue(es);
		mouse.sub_ofs = reg_dx;
		mouseWP6x = reg_dx == 0x1b6 ? true : false;									// Signature for WP6.x with Mouse Driver (Absolute/Pen)
		// Return old values
		reg_cx = oldMask;
		reg_dx = oldOfs;
		SegSet16(es, oldSeg);
		}
		break;		
	case 0x15:																		// Get Driver storage space requirements
		reg_bx = sizeof(mouse);
		break;
	case 0x16:																		// Save driver state
		Mem_CopyTo(SegPhys(es)+reg_dx, &mouse, sizeof(mouse));
		break;
	case 0x17:																		// Load driver state
		Mem_CopyFrom(SegPhys(es)+reg_dx, &mouse, sizeof(mouse));
		break;
	case 0x1a:																		// Set mouse sensitivity
		break;
	case 0x1b:																		// Get mouse sensitivity (fixed values)
		reg_bx = reg_cx = reg_dx = 50;
		break;
	case 0x1c:																		// Set interrupt rate
	case 0x1d:																		// Set display page number
		break;
	case 0x1e:																		// Get display page number
		reg_bx = 0;
		break;
	case 0x1f:																		// Disable mousedriver
		reg_bx = 0;																	// ES:BX old mouse driver
		SegSet16(es, 0);	   
		break;
	case 0x20:																		// Enable Mousedriver
	case 0x22:																		// Set language for messages
		break;
	case 0x23:																		// Get language for messages
		reg_bx = 0;																	// USA
		break;
		}
	return CBRET_NONE;
	}

static Bitu INT74_Handler(void)
	{
	reg_ax = mouse_event_type;
	reg_bx = mouse_event_buttons;
	reg_cx = mouse.x;
	reg_dx = mouse.y;
	reg_si = reg_di = 0;															// We don't do mickeys
	CPU_Push16(SegValue(cs));
	CPU_Push16(reg_ip);
	SegSet16(cs, mouse.sub_seg);
	reg_ip = mouse.sub_ofs;
	mouse_event_type = 0;
	return CBRET_NONE;
	}

void MOUSE_Init()
	{
	Bitu cb = CALLBACK_Allocate();													// Callback for mouse interrupt 33
	CALLBACK_Setup(cb, &INT33_Handler, CB_IRET_STI);
	RealSetVec(0x33, CALLBACK_RealPointer(cb)+(usesMouse ? 0 : 5));					// Programs should check for 0 (or IRET at location), 0 as Int address stalls some
	CALLBACK_Install(0x74, &INT74_Handler, CB_IRQ12);								// Callback for ps2 irq - Int 74
	}
