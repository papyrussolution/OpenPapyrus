#ifndef _video_h
#define _video_h

#include <windows.h>
#include "config.h"

typedef struct {
	Bit8u r;
	Bit8u g;
	Bit8u b;
	Bit8u unused;
} RGB_Color;

typedef struct {
	Bit32u * P_Colors;
	int		w;
	int     h;
	void	* pixels;
} vSurface;

#if 0 // @construction @sobolev {

vSurface * vCreateSurface(int width, int height, RGB_Color fg, RGB_Color bg)
{
	vSurface * p_s = (vSurface *)malloc(sizeof(vSurface));
	if(p_s) {
		memzero(p_s, sizeof(*p_s));
		p_s->w = width;
		p_s->h = height;
		p_s->pixels = malloc(sizeof(Bit8u) * width * height);
		p_s->P_Colors = (Bit32u *)malloc(sizeof(Bit32u) * width * height);
		if(!p_s->pixels || !p_s->P_Colors) {
			free(p_s->pixels);
			free(p_s->P_Colors);
			free(p_s);
			p_s = 0;
		}
	}
	return p_s;
}

#endif // } 0 @construction @sobolev

typedef struct {
	bool   active; // If this isn't set don't draw
	Bit32u hideTill;
	bool   framed;
	Bit8u  scale;
	Bit16u width;
	Bit16u height;
	vSurface * surface;
} vWinSpecs;

// The main window
extern HWND vDosHwnd;
extern vWinSpecs window;
extern HBITMAP DIBSection;

// Function prototypes
void InitWindow(void);
void VideoQuit(void);
vSurface * SetVideoMode(int width, int height, bool framed);
void HandleUserActions(void);
void SetVideoSize();
bool StartVideoUpdate(void);
void EndTextLines(void);
int SetCodePage(int cp);
vSurface * vCreateSurface(int width, int height, RGB_Color fg, RGB_Color bg);
void vUpdateRect(Bit32s x, Bit32s y, Bit32u w, Bit32u h);
void vUpdateWin(void);
void vBlitSurface(vSurface *src, RECT *srcrect,  RECT *dstrect);

LRESULT CALLBACK WinMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

#endif

