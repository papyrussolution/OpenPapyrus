#include "vDos.h"
#include "ttf.h"
#include "video.h"
#include "vga.h"
#include "mem.h"

VGA_Type vga;

Bit32u ExpandTable[256];
Bit32u Expand16Table[4][16];
Bit32u FillTable[16];

void VGA_DetermineMode(void)
	{
	if (!(vga.attr.mode_control&1))													// Test for VGA output active or direct color modes
		vga.mode = M_TEXT;
	else if (vga.mode != M_EGA)
		{
		vga.mode = M_EGA;
		VGA_SetupHandlers();
		}
	VGA_StartResize();
	}


static bool inReset = false;
static bool doWinRefresh = false;


void EndVideoUpdate(void)
	{
	if (vga.mode == M_TEXT)
		EndTextLines();
	else if (render.cache.maxY != -1)
		{
		int pixs = window.surface->w/render.cache.width;							// Parachute/more safe than scale???
		Bit32u *pointer = (Bit32u *)window.surface->pixels + (render.cache.minY*window.surface->w+render.cache.minX)*pixs;

		for (int i = render.cache.minY; i <= render.cache.maxY; i++)				// Build first to last changed line
			{
			Bit8u *cache = vga.fastmem+i*render.cache.width+render.cache.minX;
			for (int j = render.cache.minX; j <= render.cache.maxX; j++)			// Build one line
				{
				Bit32u pColor = ((Bit32u *)render.pal)[*cache++];
				int k = pixs;
				do
					*pointer++ = pColor;
				while (--k);
				}
			pointer = (Bit32u *)window.surface->pixels + (i*window.surface->w+render.cache.minX)*pixs+window.surface->w;
			for (int j = pixs-1; j; j--)											// Duplicate line pixs
				{
				memcpy(pointer, pointer-window.surface->w, (render.cache.maxX-render.cache.minX+1)*pixs*4);
				pointer += window.surface->w;
				}
			}
		vUpdateRect(render.cache.minX*pixs, render.cache.minY*pixs, (render.cache.maxX-render.cache.minX+1)*pixs, (render.cache.maxY-render.cache.minY+1)*pixs);
		render.cache.maxX = render.cache.maxY = -1;
		render.cache.minX = render.cache.minY = 10000;
		}
	}

void VGA_VerticalTimer()
	{
	if (!doWinRefresh)
		return;
	vga.draw.vertRetrace = !vga.draw.vertRetrace;									// So it's just half the times
	inReset = false;

	if (vga.mode == M_TEXT)
		{
		if (StartVideoUpdate())
			{
			vga.draw.cursor.address = vga.config.cursor_start*2;
			newAttrChar = (Bit16u *)(MemBase+((CurMode->mode) == 7 ? 0xB0000 : 0xb8000));	// Pointer to chars+attribs
			EndVideoUpdate();
			}
		return;
		}
	EndVideoUpdate();
	}

void VGA_ResetVertTimer(bool delay)													// Trial to sync keyboard with screen (delay: don't update, for pasting keys)
	{
	if (!doWinRefresh)
		return;
	if (!delay)
		if (inReset)
			VGA_VerticalTimer();
	inReset = true;
	}

void VGA_StartResize(void)
	{
	Bitu width = vga.crtc.horizontal_display_end+1;
	Bitu height = (vga.crtc.vertical_display_end|((vga.crtc.overflow & 2)<<7)|((vga.crtc.overflow & 0x40) << 3))+1; 

	width *= vga.mode == M_TEXT ? 9 : 8;
	if ((width != vga.draw.width) || (height != vga.draw.height))					// Need to resize the output window?
		{
		vga.draw.width = width;
		vga.draw.height = height;
		if (width >= 640 && height >= 350)
			{
			render.cache.width	= width;
			render.cache.height	= height;
			SetVideoSize();															// Setup the scaler variables
			}
		}
	doWinRefresh = true;
	}

void VGA_Init()
	{
	vga.mode = M_ERROR;																// For first init
	vga.vmemsize = 256*1024;														// 256kB VGA memory
	VGA_SetupMisc();
	VGA_SetupDAC();
	VGA_SetupGFX();
	VGA_SetupSEQ();
	VGA_SetupAttr();

	for (Bitu i = 0; i < 256; i++)													// Generate tables
		ExpandTable[i] = i | (i << 8) | (i <<16) | (i << 24);
	for (Bitu i = 0; i < 16; i++)
		FillTable[i] = ((i&1) ? 0x000000ff : 0)|((i&2) ? 0x0000ff00:0)|((i&4) ? 0x00ff0000 : 0)|((i&8) ? 0xff000000 : 0);
	for (Bitu j = 0; j < 4; j++)
		for (Bitu i = 0; i < 16; i++)
			Expand16Table[j][i] = ((i&1) ? 1<<(24+j) : 0)|((i&2) ? 1<<(16+j) : 0)|((i&4) ? 1<<(8+j) : 0)|((i&8) ? 1<<j : 0);
	}
