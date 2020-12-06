//
//
#include "vDos.h"
#include "video.h"
#include "events.h"
#include "mouse.h"
#include "vDosTTF.h"
#include "ttf.h"
#include "..\ints\int10.h"
#include "dos_inc.h"

#define WM_SC_SYSMENUABOUT              0x1110 // Pretty arbitrary, just needs to be < 0xF000
#define WM_SC_SYSMENUPCOPY              (WM_SC_SYSMENUABOUT+1)
#define WM_SC_SYSMENUPASTE              (WM_SC_SYSMENUABOUT+2)
#define WM_SC_SYSMENUDECREASE   (WM_SC_SYSMENUABOUT+3)
#define WM_SC_SYSMENUINCREASE   (WM_SC_SYSMENUABOUT+4)

RGBQUAD altBGR0[16], altBGR1[16];

static DWORD ttfSizeData = sizeof(vDosTTFbi);
static void * ttfFontData = vDosTTFbi;
static bool fontBoxed = false; // ASCII 176-223 box/lines or extended ASCII
static int prevPointSize = 0;
static RECT prevPosition;
static int winPerc = 75;
static int initialX = -1;
static int initialY = -1;
static int eurAscii = -1; // ASCII value to use for the EUro symbol, standard none

static bool selectingText = false;
static int selStartX, selPosX1, selPosX2, selStartY, selPosY1, selPosY2;

int wsVersion; // For now just 0 (no WordStar) or 1
int wsBackGround; // BackGround text color WordStar

Render_ttf ttf;
Render_t render;
Bit16u curAttrChar[txtMaxLins*txtMaxCols];                                                                                  //
                                                                                                                            // Current
                                                                                                                            // displayed
                                                                                                                            // textpage
Bit16u * newAttrChar;                                                                                                                        //
                                                                                                                                             // To
                                                                                                                                             // be
                                                                                                                                             // replaced
                                                                                                                                             // by

static RGB_Color ttf_fgColor = {0, 0, 0, 0};
static RGB_Color ttf_bgColor = {0, 0, 0, 0};
static RECT ttf_textRect = {0, 0, 0, 0};
static RECT ttf_textClip = {0, 0, 0, 0};

static WNDPROC fnDefaultWndProc;

static void TimedSetSize()
{
	HMONITOR monitor = MonitorFromWindow(vDosHwnd, MONITOR_DEFAULTTONEAREST);
	MONITORINFO info;
	info.cbSize = sizeof(MONITORINFO);
	GetMonitorInfo(monitor, &info);
	if(vga.mode == M_TEXT && ttf.fullScrn)
		window.surface = SetVideoMode(info.rcMonitor.right-info.rcMonitor.left, info.rcMonitor.bottom-info.rcMonitor.top, false);
	else
		window.surface = SetVideoMode(window.width, window.height, window.framed);
	if(!window.surface)
		E_Exit("Failed to create surface for vDos window");
	if(initialX >= 0) {                                                                                                                         //
	                                                                                                                                            // Position
	                                                                                                                                            // window
	                                                                                                                                            // (only
	                                                                                                                                            // once
	                                                                                                                                            // at
	                                                                                                                                            // startup)
		RECT rect;
		GetWindowRect(vDosHwnd, &rect);                                                                                     //
		                                                                                                                    // Window
		                                                                                                                    // has
		                                                                                                                    // to
		                                                                                                                    // be
		                                                                                                                    // shown
		                                                                                                                    // completely
		if(initialX+rect.right-rect.left <= info.rcMonitor.right-info.rcMonitor.left && initialY+rect.bottom-rect.top <=
		    info.rcMonitor.bottom-info.rcMonitor.top)
			MoveWindow(vDosHwnd, initialX, initialY, rect.right-rect.left, rect.bottom-rect.top, true);
		initialX = -1;
	}
	window.active = true;
}

void SetVideoSize()
{
	if(vga.mode == M_TEXT) {
		window.width = ttf.cols*ttf.charWidth;
		window.height = ttf.lins*ttf.charHeight;
	}
	else {
		window.width = render.cache.width*window.scale;
		window.height = render.cache.height*window.scale;
	}
	if(!winHidden)
		TimedSetSize();
}

bool StartVideoUpdate()
{
	if(winHidden && GetTickCount() >= window.hideTill) {
		winHidden = false;
		TimedSetSize();
	}
	return window.active;
}

int SetCodePage(int cp)
{
	unsigned char cTest[256];                                                                                                           //
	                                                                                                                                    // ASCII
	                                                                                                                                    // format
	for(int i = 0; i < 256; i++)
		cTest[i] = i;
	Bit16u wcTest[512];
	if(MultiByteToWideChar(cp, MB_PRECOMPOSED|MB_ERR_INVALID_CHARS, (char*)cTest, 256, (LPWSTR)wcTest, 256) == 256) {
		TTF_Flush_Cache(ttf.font);                                                                                                  //
		                                                                                                                            // Rendered
		                                                                                                                            // glyph
		                                                                                                                            // cache
		                                                                                                                            // has
		                                                                                                                            // to
		                                                                                                                            // be
		                                                                                                                            // cleared!
		int notMapped = 0;                                                                                                                  //
		                                                                                                                                    // Number
		                                                                                                                                    // of
		                                                                                                                                    // characters
		                                                                                                                                    // not
		                                                                                                                                    // defined
		                                                                                                                                    // in
		                                                                                                                                    // font
		Bit16u unimap;
		for(int c = 128; c < 256; c++) {
			if(wcTest[c] != 0x20ac)                                                                                             //
				                                                                                                            // To
				                                                                                                            // be
				                                                                                                            // consistent,
				                                                                                                            // no
				                                                                                                            // Euro
				                                                                                                            // substitution
				unimap = wcTest[c];
			if(!fontBoxed || c < 176 || c > 223) {
				if(!TTF_GlyphIsProvided(ttf.font, unimap))
					notMapped++;
				else
					cpMap[c] = unimap;
			}
		}
		if(eurAscii != -1 && TTF_GlyphIsProvided(ttf.font, 0x20ac))
			cpMap[eurAscii] = 0x20ac;
		return notMapped;
	}
	return -1;                                                                                                                                          //
	                                                                                                                                                    // Code
	                                                                                                                                                    // page
	                                                                                                                                                    // not
	                                                                                                                                                    // set
}

void SelectFontByPoints(int ptsize)
{
	if(ttf.font == 0)
		ttf.font = TTF_New_Memory_Face((const unsigned char*)ttfFontData, ttfSizeData, ptsize);
	else
		TTF_SetCharSize(ttf.font, ptsize);
	ttf.pointsize = ptsize;
	ttf.charWidth = TTF_FontWidth(ttf.font);
	ttf.charHeight = TTF_FontHeight(ttf.font);
	ttf.offX = ttf.offY = 0;
	if(ttf.fullScrn) {
		HMONITOR monitor = MonitorFromWindow(vDosHwnd, MONITOR_DEFAULTTONEAREST);
		MONITORINFO info;
		info.cbSize = sizeof(MONITORINFO);
		GetMonitorInfo(monitor, &info);
		ttf.offX = (info.rcMonitor.right-info.rcMonitor.left-ttf.charWidth*ttf.cols)/2;
		ttf.offY = (info.rcMonitor.bottom-info.rcMonitor.top-ttf.charHeight*ttf.lins)/2;
	}
	if(!codepage) {
		int cp = GetOEMCP();
		codepage = SetCodePage(cp) == -1 ? 437 : cp;
	}
}

static RGBQUAD * rgbColors = (RGBQUAD*)render.pal;

static int prev_sline = -1;
static bool hasFocus = true;                                                                                                        //
                                                                                                                                    // Only
                                                                                                                                    // used
                                                                                                                                    // if
                                                                                                                                    // not
                                                                                                                                    // framed
static bool hasMinButton = false; // ,,

static void dumpScreen(void)
{
	FILE * fp;

	if(vga.mode == M_TEXT)
		if(fp = fopen("screen.txt", "wb")) {
			Bit16u textLine[txtMaxCols+1];
			Bit16u * curAC = curAttrChar;
			fprintf(fp, "\xff\xfe"); // It's a Unicode text file
			for(int lineno = 0; lineno < ttf.lins; lineno++) {
				int chars = 0;
				for(int xPos = 0; xPos < ttf.cols; xPos++) {
					Bit8u textChar =  *(curAC++)&255;
					textLine[xPos] = cpMap[textChar];
					if(textChar != 32)
						chars = xPos+1;
				}
				if(chars)  // If not blank line
					fwrite(textLine, 2, chars, fp);  // Write truncated
				if(lineno < ttf.lins-1)  // Append linefeed for all but the last line
					fwrite("\x0d\x00\x0a\x00", 1, 4, fp);
			}
			fclose(fp);
			ShellExecute(NULL, "open", "screen.txt", NULL, NULL, SW_SHOWNORMAL); // Open text file
		}
	return;
}

void getClipboard(void)
{
	if(OpenClipboard(NULL)) {
		if(HANDLE cbText = GetClipboardData(CF_UNICODETEXT)) {
			Bit16u * p = (Bit16u*)GlobalLock(cbText);
			BIOS_PasteClipboard(p);
			GlobalUnlock(cbText);
		}
		CloseClipboard();
	}
}

void EndTextLines(void)
{
	if(selectingText)  // The easy way: don't update when selecting text
		return;

	char asciiStr[txtMaxCols+1]; // Max+1 charaters in a line
	int xmin = ttf.cols; // Keep track of changed area
	int ymin = ttf.lins;
	int xmax = -1;
	int ymax = -1;
	int style = 0;
	Bit16u * curAC = curAttrChar; // Old/current buffer
	Bit16u * newAC = newAttrChar; // New/changed buffer

	if(!window.framed)
		if((GetFocus() == vDosHwnd) != hasFocus) {
			hasFocus = !hasFocus;
			memset(curAC, -1, ttf.cols*2); // Force redraw of top line
		}
	if(ttf.cursor >= 0 && ttf.cursor < ttf.cols*ttf.lins)  // Hide/restore (previous) cursor-character if we had one
//		if (ttf.cursor != vga.draw.cursor.address>>1 || vga.draw.cursor.sline > vga.draw.cursor.eline ||
// vga.draw.cursor.sline > 15)
		if(ttf.cursor != vga.draw.cursor.address>>1)
			curAC[ttf.cursor] = newAC[ttf.cursor]^0xf0f0;  // Force redraw (differs)

	ttf_textClip.top = 0;
	ttf_textClip.bottom = ttf.charHeight;
	for(int y = 0; y < ttf.lins; y++) {
		rgbColors = altBGR1;
		if(!hasFocus && (y == 0))  // Dim topmost line
			rgbColors = altBGR0;

		ttf_textRect.top = ttf.offY+y*ttf.charHeight;
		for(int x = 0; x < ttf.cols; x++) {
			if(newAC[x] != curAC[x]) {
				xmin = MIN(x, xmin);
				ymin = MIN(y, ymin);
				ymax = y;
				ttf_textRect.left = ttf.offX+x*ttf.charWidth;
				style = TTF_STYLE_NORMAL;

				Bit8u colorBG = newAC[x]>>12;
				Bit8u colorFG = (newAC[x]>>8)&15;
				if(CurMode->mode == 7) { // Mono (Hercules)
					style = (colorFG&7) == 1 ? TTF_STYLE_UNDERLINE : TTF_STYLE_NORMAL;
					if((colorFG&0xa) == colorFG && colorBG == 0)
						colorFG = 8;
					else if(colorFG&7)
						colorFG |= 7;
				}
				else if(wpVersion > 0) { // If WP and not negative (color value to text attribute
				                         // excluded)
//					if (colorFG == 0xe && colorBG == 1) // Temporary? disabled
//						{
//						style =  TTF_STYLE_ITALIC;
//						colorFG = 7;
//						}
					/*else*/ if((colorFG == 1 || colorFG == 0xf) && colorBG == 7) {
						style = TTF_STYLE_UNDERLINE;
						colorBG = 1;
						colorFG = colorFG == 1 ? 7 : 0xf;
					}
					else
						style = TTF_STYLE_NORMAL;
				}
				else if(wsVersion) { // If WordStar
					if(colorBG&8) { // If "blinking" set, modify attributes
						if(colorBG&1)
							style |= TTF_STYLE_UNDERLINE;
//						if (colorBG&2) // Temporary? disabled
//							style |= TTF_STYLE_ITALIC;
						if(colorBG&4)
							style |= TTF_STYLE_STRIKETHROUGH;
					}
					if(style)
						colorBG = wsBackGround;  // Background color (text) is fixed at this
				}
				ttf_bgColor.b = rgbColors[colorBG].rgbBlue;
				ttf_bgColor.g = rgbColors[colorBG].rgbGreen;
				ttf_bgColor.r = rgbColors[colorBG].rgbRed;
				ttf_fgColor.b = rgbColors[colorFG].rgbBlue;
				ttf_fgColor.g = rgbColors[colorFG].rgbGreen;
				ttf_fgColor.r = rgbColors[colorFG].rgbRed;

				int x1 = x;
				Bit8u ascii = newAC[x]&255;
				if(cpMap[ascii] > 0x2590 && cpMap[ascii] < 0x2594) { // Special: characters 176-178 = shaded block
					ttf_bgColor.b = (ttf_bgColor.b*(179-ascii) + ttf_fgColor.b*(ascii-175))>>2;
					ttf_bgColor.g = (ttf_bgColor.g*(179-ascii) + ttf_fgColor.g*(ascii-175))>>2;
					ttf_bgColor.r = (ttf_bgColor.r*(179-ascii) + ttf_fgColor.r*(ascii-175))>>2;
					do { // As long char and foreground/background color equal
						curAC[x] = newAC[x];
						asciiStr[x-x1] = 32; // Shaded space
						x++;
					}
					while(x < ttf.cols && newAC[x] == newAC[x1] && newAC[x] != curAC[x]);
				}
				else {
					Bit8u color = newAC[x]>>8;
					do { // As long foreground/background color equal
						curAC[x] = newAC[x];
						asciiStr[x-x1] = ascii;
						x++;
						ascii = newAC[x]&255;
					} while(x < ttf.cols && newAC[x] != curAC[x] && newAC[x]>>8 == color && (ascii < 176 || ascii > 178));
				}
				xmax = MAX(x-1, xmax);
				vSurface* textSurface = TTF_RenderASCII(ttf.font, asciiStr, x-x1, ttf_fgColor, ttf_bgColor, style);
				ttf_textClip.right = (x-x1)*ttf.charWidth;
				vBlitSurface(textSurface, &ttf_textClip, &ttf_textRect);
				x--;
			}
		}
		curAC += ttf.cols;
		newAC += ttf.cols;
	}
	int newPos = vga.draw.cursor.address>>1;
	// Draw cursor?
	if(hasFocus && vga.draw.cursor.enabled && vga.draw.cursor.sline <= vga.draw.cursor.eline && vga.draw.cursor.sline < 16) { 
		if(newPos >= 0 && newPos < ttf.cols*ttf.lins) { // If on screen
			int y = newPos/ttf.cols;
			int x = newPos%ttf.cols;
			if(ttf.cursor != newPos || vga.draw.cursor.sline != prev_sline) { // If new position or shape changed, forse draw
				prev_sline = vga.draw.cursor.sline;
				xmin = MIN(x, xmin);
				xmax = MAX(x, xmax);
				ymin = MIN(y, ymin);
				ymax = MAX(y, ymax);
			}
			if(x >= xmin && x <= xmax && y >= ymin && y <= ymax) { // If overdrawn previuosly (or new shape)
				Bit8u colorBG = newAttrChar[newPos]>>12;
				Bit8u colorFG = (newAttrChar[newPos]>>8)&15;
				style = TTF_STYLE_NORMAL;
				if(wpVersion > 0) { // If WP and not negative (color value to text attribute excluded)
//					if (colorFG == 0xe && colorBG == 1) // Temporary? disabled
//						{
//						style = TTF_STYLE_ITALIC;
//						colorFG = 7;
//						}
					/*else*/ if((colorFG == 1 || colorFG == 0xf) && colorBG == 7) {
						style = TTF_STYLE_UNDERLINE;
						colorBG = 1;
						colorFG = colorFG == 1 ? 7 : 0xf;
					}
					else
						style = TTF_STYLE_NORMAL;
				}
				ttf_bgColor.b = rgbColors[colorBG].rgbBlue;
				ttf_bgColor.g = rgbColors[colorBG].rgbGreen;
				ttf_bgColor.r = rgbColors[colorBG].rgbRed;
				ttf_fgColor.b = rgbColors[colorFG].rgbBlue;
				ttf_fgColor.g = rgbColors[colorFG].rgbGreen;
				ttf_fgColor.r = rgbColors[colorFG].rgbRed;
				asciiStr[0] = newAttrChar[newPos]&255;
// First redraw character
				vSurface* textSurface = TTF_RenderASCII(ttf.font, asciiStr, 1, ttf_fgColor, ttf_bgColor, style);
				ttf_textClip.right = ttf_textClip.left+ttf.charWidth;
				ttf_textRect.left = ttf.offX+x*ttf.charWidth;
				ttf_textRect.top = ttf.offY+y*ttf.charHeight;
				vBlitSurface(textSurface, &ttf_textClip, &ttf_textRect);
// Then reverse lower lines
				textSurface = TTF_RenderASCII(ttf.font, asciiStr, 1, ttf_bgColor, ttf_fgColor, style);
				ttf_textClip.top = (ttf.charHeight*vga.draw.cursor.sline)>>4;
				ttf_textClip.bottom = ttf.charHeight; // For now, cursor to bottom
				ttf_textRect.top = ttf.offY+y*ttf.charHeight + ttf_textClip.top;
				vBlitSurface(textSurface, &ttf_textClip, &ttf_textRect);
			}
		}
	}
	ttf.cursor = newPos;
	// (Re)draw Minimize button?
	if((!hasFocus || hasMinButton) && !ttf.fullScrn && xmax == ttf.cols-1 && ymin == 0) { 
		Bit8u color = newAttrChar[xmax]>>12;
		ttf_fgColor.b = rgbColors[color].rgbBlue;
		ttf_fgColor.g = rgbColors[color].rgbGreen;
		ttf_fgColor.r = rgbColors[color].rgbRed;
		color = (newAttrChar[xmax]>>8)&15;
		ttf_bgColor.b = rgbColors[color].rgbBlue;
		ttf_bgColor.g = rgbColors[color].rgbGreen;
		ttf_bgColor.r = rgbColors[color].rgbRed;
		asciiStr[0] = 45; // '-'
		vSurface* textSurface = TTF_RenderASCII(ttf.font, asciiStr, 1, ttf_fgColor, ttf_bgColor, 0);
		ttf_textClip.right = ttf.charWidth;
		ttf_textClip.top = ttf.charHeight>>2;
		ttf_textClip.bottom = (ttf.charHeight>>1)+(ttf.charHeight>>2);
		ttf_textRect.left = xmax*ttf.charWidth;
		ttf_textRect.top = 0;
		vBlitSurface(textSurface, &ttf_textClip, &ttf_textRect);
	}
	if(xmin <= xmax)  // If any changes
		vUpdateRect(ttf.offX+xmin*ttf.charWidth,
		    ttf.offY+ymin*ttf.charHeight,
		    (xmax-xmin+1)*ttf.charWidth,
		    (ymax-ymin+1)*ttf.charHeight);
}

static void decreaseFontSize()
{
	if(vga.mode == M_TEXT && ttf.pointsize > 12) {
		SelectFontByPoints(ttf.pointsize-2);
		SetVideoSize();
		memset(curAttrChar, -1, ttf.cols*ttf.lins*2); // Force redraw of complete window
	}
}

static void increaseFontSize()
{
	if(vga.mode == M_TEXT) { // Increase fontsize
		int currSize = ttf.pointsize;
		HMONITOR monitor = MonitorFromWindow(vDosHwnd, MONITOR_DEFAULTTONEAREST);
		MONITORINFO info;
		info.cbSize = sizeof(MONITORINFO);
		GetMonitorInfo(monitor, &info);
		int maxWidth = info.rcMonitor.right-info.rcMonitor.left;
		int maxHeight = info.rcMonitor.bottom-info.rcMonitor.top;
		if(!ttf.fullScrn && window.framed) { // 3D borders
			maxWidth -= GetSystemMetrics(SM_CXBORDER)*2;
			maxHeight -= GetSystemMetrics(SM_CYCAPTION) + GetSystemMetrics(SM_CYBORDER)*2;
		}
		SelectFontByPoints(ttf.pointsize+2);
		if(ttf.cols*ttf.charWidth <= maxWidth && ttf.lins*ttf.charHeight <= maxHeight) { // if it fits on screen
			SetVideoSize();
			memset(curAttrChar, -1, ttf.cols*ttf.lins*2); // Force redraw of complete window
		}
		else
			SelectFontByPoints(currSize);
	}
}

void readTTF(const char * fName) // Open and read alternative font
{
	FILE * ttf_fh;
	char ttfPath[1024];

	if(!(ttf_fh = fopen(strcat(strcpy(ttfPath, fName), ".ttf"), "rb"))) { // Try to load it from working directory
		strcpy(strrchr(strcpy(ttfPath, _pgmptr), '\\')+1, fName); // Try to load it from where vDos was started
		ttf_fh  = fopen(strcat(ttfPath, ".ttf"), "rb");
	}
	if(ttf_fh) {
		if(!fseek(ttf_fh, 0, SEEK_END))
			if((ttfSizeData = ftell(ttf_fh)) != -1L)
				if(ttfFontData = malloc((size_t)ttfSizeData))
					if(!fseek(ttf_fh, 0, SEEK_SET))
						if(fread(ttfFontData, 1, (size_t)ttfSizeData, ttf_fh) == (size_t)ttfSizeData) {
							fclose(ttf_fh);
							return;
						}
		fclose(ttf_fh);
	}
	ConfAddError("Could not load TTF font file\n", (char*)fName);
}

static void showAbout() // @sobolev @stub
{
}

static LRESULT CALLBACK SysMenuExtendWndProc(HWND hwnd, UINT uiMsg, WPARAM wparam, LPARAM lparam)
{
	if(uiMsg  == WM_SYSCOMMAND) {
		switch(wparam) {
			case WM_SC_SYSMENUABOUT:
			    showAbout();
			    return 0;
			case WM_SC_SYSMENUPCOPY:
			    dumpScreen();
			    return 0;
			case WM_SC_SYSMENUPASTE:
			    getClipboard();
			    return 0;
			case WM_SC_SYSMENUDECREASE:
			    decreaseFontSize();
			    return 0;
			case WM_SC_SYSMENUINCREASE:
			    increaseFontSize();
			    return 0;
		}
	}
	return CallWindowProc(fnDefaultWndProc, hwnd, uiMsg, wparam, lparam);
}

bool getWP(char * wpStr)
{
	if(!strnicmp(wpStr, "WS", 2)) { // WP = WS (WordStar)
		wpStr += 2;
		wsVersion = 1;
		lTrim(wpStr);
		if(*wpStr == 0)
			return true;
		if(sscanf(wpStr, ", %d", &wsBackGround) == 1)
			if(wsBackGround >= 0 && wsBackGround <= 15)
				return true;
		wsBackGround = 0;
		wsVersion = 0;
		return false;
	}
	if(sscanf(wpStr, "%d", &wpVersion) == 1)  // No checking on version number
		return true;
	return false;
}

bool setColors(const char * colorArray)
{
	const char * nextRGB = colorArray;
	Bit8u * altPtr = (Bit8u*)altBGR1;
	int rgbVal[3];
	for(int colNo = 0; colNo < 16; colNo++) {
		if(sscanf(nextRGB, " ( %d , %d , %d)", &rgbVal[0], &rgbVal[1], &rgbVal[2]) == 3) { // Decimal:
		                                                                                   // (red,green,blue)
			for(int i = 0; i< 3; i++) {
				if(rgbVal[i] < 0 || rgbVal[i] >255)
					return false;
				altPtr[2-i] = rgbVal[i];
			}
			while(*nextRGB != ')')
				nextRGB++;
			nextRGB++;
		}
		else if(sscanf(nextRGB, " #%6x", &rgbVal[0]) == 1) { // Hexadecimal
			if(rgbVal < 0)
				return false;
			for(int i = 0; i < 3; i++) {
				altPtr[i] = rgbVal[0]&255;
				rgbVal[0] >>= 8;
			}
			nextRGB = strchr(nextRGB, '#') + 7;
		}
		else
			return false;
		altPtr += 4;
	}
	for(int i = 0; i < 16; i++) {
		altBGR0[i].rgbBlue = (altBGR1[i].rgbBlue*2 + 128)/4;
		altBGR0[i].rgbGreen = (altBGR1[i].rgbGreen*2 + 128)/4;
		altBGR0[i].rgbRed = (altBGR1[i].rgbRed*2 + 128)/4;
	}
	return true;
}

bool setWinInitial(const char * winDef)
{        // Format = <MAX perc>[,x-pos:y-pos]
	if(!*winDef)  // Nothing set
		return true;
	int testVal1, testVal2, testVal3;
	char testStr[512];
	if(sscanf(winDef, "%d%s", &testVal1, testStr) == 1)  // Only <MAX perc>
		if(testVal1 > 0 && testVal1 <= 100) { // 1/100% are absolute minimum/maximum
			winPerc = testVal1;
			return true;
		}
	if(sscanf(winDef, "%d,%d:%d%s", &testVal1, &testVal2, &testVal3, testStr) == 3)  // All parameters
		if(testVal1 > 0 && testVal1 <= 100 && testVal2 >= 0 && testVal3 >= 0) { // X-and y-pos only tested for positive values
			// Values too high are checked later and uActually dropped
			winPerc = testVal1;
			initialX = testVal2;
			initialY = testVal3;
			return true;
		}
	return false;
}

void GUI_StartUp()
{
	STARTUPINFO si;

	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	GetStartupInfo(&si);
	if(char * p = strrchr(si.lpTitle, '\\')) { // Set title bar to shortcut name of vDos
		char caption[1024];
		strcpy(caption, p+1);
		if(p = strrchr(caption, '.'))
			*p = 0;
		SetWindowText(vDosHwnd, caption);
	}
	else
		SetWindowText(vDosHwnd, "vDos");

	SetClassLong(vDosHwnd, GCL_HICON, (LONG)LoadIcon(GetModuleHandle(NULL), "vDos_ico")); // Set vDos icon
	char * fName = ConfGetString("font");
	if(*fName == '-') { // (Preceeding) - indicates ASCII 176-223 = lines
		fontBoxed = true;
		fName = lTrim(fName+1);
	}
	if(*fName)
		readTTF(fName);
	else
		ttf.vDos = true; ;

	ttf.lins = ConfGetInt("lins");
	ttf.lins = MAX(24, MIN(txtMaxLins, ttf.lins));
	ttf.cols = ConfGetInt("cols");
	ttf.cols = MAX(80, MIN(txtMaxCols, ttf.cols));
	for(Bitu i = 0; ModeList_VGA[i].mode <= 7; i++) { // Set the cols and lins in video mode 2,3,7
		ModeList_VGA[i].twidth = ttf.cols;
		ModeList_VGA[i].theight = ttf.lins;
	}

	usesMouse = ConfGetBool("mouse");
	window.framed = ConfGetBool("frame");
	window.scale = ConfGetInt("scale");
	eurAscii = ConfGetInt("euro");
	if(eurAscii != -1 && (eurAscii < 33 || eurAscii > 255))
		E_Exit("Config.txt: Euro ASCII value has to be between 33 and 255");
	if(window.scale < 0 || window.scale > 9)
		E_Exit("Config.txt: scale factor has to be between 1 and 9");
	char * wpStr = ConfGetString("WP");
	if(*wpStr)
		if(!getWP(wpStr))
			ConfAddError("Invalid WP= parameters\n", wpStr);
	rgbColors = altBGR1;
	char * colors = ConfGetString("colors");
	if(!strnicmp(colors, "mono", 4)) { // Mono, "Hercules" mode?
		colors += 4;
		lTrim(colors);
		if(*colors == ',' || *colors == 0) {
			initialvMode = 7;
			if(*colors == ',') {
				colors += 1;
				lTrim(colors);
			}
		}
	}
	if(!setColors(colors)) {
		setColors(
		    "#000000 #0000aa #00aa00 #00aaaa #aa0000 #aa00aa #aa5500 #aaaaaa #555555 #5555ff #55ff55 #55ffff #ff5555 #ff55ff #ffff55 #ffffff");       //
		                                                                                                                                              // Standard
		                                                                                                                                              // DOS
		                                                                                                                                              // colors
		if(*colors) {
			initialvMode = 3;
			ConfAddError("Invalid COLORS= parameters\n", colors);
		}
	}
	char * winDef = ConfGetString("window");
	if(!setWinInitial(winDef))
		ConfAddError("Invalid WINDOWS= parameters\n", winDef);
	if(winPerc == 100)
		ttf.fullScrn = true;

	HMONITOR monitor = MonitorFromWindow(vDosHwnd, MONITOR_DEFAULTTONEAREST);
	MONITORINFO info;
	info.cbSize = sizeof(MONITORINFO);
	GetMonitorInfo(monitor, &info);
	int maxWidth = info.rcMonitor.right-info.rcMonitor.left;
	int maxHeight = info.rcMonitor.bottom-info.rcMonitor.top;
	if(!ttf.fullScrn && window.framed) { // 3D borders
		maxWidth -= GetSystemMetrics(SM_CXBORDER)*2;
		maxHeight -= GetSystemMetrics(SM_CYCAPTION) + GetSystemMetrics(SM_CYBORDER)*2;
	}

	int maxScale = MIN(maxWidth/640, maxHeight/480); // Based on MAX resolution supported VGA modes
	if(window.scale > maxScale)
		E_Exit("Config.txt: scale factor too high for screen resolution");
	if(window.scale < 1) { // 0 = probably not set in config.txt
		window.scale = MIN(maxWidth/640, maxHeight/480); // Based on MAX resolution supported VGA modes
		if(window.scale < 1)  // Probably overkill
			window.scale = 1;
		if(window.scale > 9)
			window.scale = 9;
	}

	int curSize = 30;     // No clear idea what would be a good starting value
	int lastGood = -1;
	int trapLoop = 0;

	while(curSize != lastGood) {
		SelectFontByPoints(curSize);
		if(ttf.cols*ttf.charWidth <= maxWidth && ttf.lins*ttf.charHeight <= maxHeight) { // If it fits on screen
			lastGood = curSize;
			float coveredPerc = float(100*ttf.cols*ttf.charWidth/maxWidth*ttf.lins*ttf.charHeight/maxHeight);
			if(trapLoop++ > 4 && coveredPerc <= winPerc)  // We can get into a +/-/+/-... loop!
				break;
			curSize = (int)(curSize*sqrt((float)winPerc/coveredPerc)); // Rounding down is ok
			if(curSize < 12)  // Minimum size = 12
				curSize = 12;
		}
		else if(--curSize < 12)  // Silly, but OK, you never know..
			E_Exit("Cannot accommodate a window for %dx%d", ttf.lins, ttf.cols);
	}
	curSize &= ~1; // Make it's even (a bit nicer)

	SelectFontByPoints(curSize);
	HMENU hSysMenu = GetSystemMenu(vDosHwnd, FALSE);

	RemoveMenu(hSysMenu, SC_MAXIMIZE, MF_BYCOMMAND); // Remove some useless items
	RemoveMenu(hSysMenu, SC_SIZE, MF_BYCOMMAND);
	RemoveMenu(hSysMenu, SC_RESTORE, MF_BYCOMMAND);

	AppendMenu(hSysMenu, MF_SEPARATOR, NULL, "");
	AppendMenu(hSysMenu, MF_STRING, WM_SC_SYSMENUPCOPY,         "Copy all text to and open file\tWin+Ctrl+C");
	AppendMenu(hSysMenu, MF_STRING, WM_SC_SYSMENUPASTE,         "Paste from Windows clipboard\tWin+Ctrl+V");
	AppendMenu(hSysMenu, MF_STRING, WM_SC_SYSMENUDECREASE,  "Decrease font/window size\tWin+F11");
	AppendMenu(hSysMenu, MF_STRING, WM_SC_SYSMENUINCREASE,  "Increase font/window size\tWin+F12");
	AppendMenu(hSysMenu, MF_SEPARATOR, NULL, "");
	AppendMenu(hSysMenu, MF_STRING, WM_SC_SYSMENUABOUT,         "About...");

	fnDefaultWndProc = (WNDPROC)GetWindowLongPtr(vDosHwnd, GWLP_WNDPROC);
	SetWindowLongPtr(vDosHwnd, GWLP_WNDPROC, (LONG_PTR)&SysMenuExtendWndProc);
	return;
}

static bool winMoving = false;
static int winMovingX, winMovingY = 0;

void HandleUserActions()
{
	userAction uAct;
	while(vPollEvent(&uAct)) {
		if(!newAttrChar)  // Don't process uActs as long not initialized
			continue;
		switch(uAct.type)
		{
			case WM_MOUSEMOVE:
			    if(selectingText) {
				    RECT selBox1 =
				    {selPosX1* ttf.charWidth, selPosY1*ttf.charHeight, selPosX2*ttf.charWidth+ttf.charWidth, selPosY2*
				ttf.charHeight+ttf.charHeight};
				    int newX = uAct.x/ttf.charWidth;
				    selPosX1 = MIN(selStartX, newX);
				    selPosX2 = MAX(selStartX, newX);
				    int newY = uAct.y/ttf.charHeight;
				    selPosY1 = MIN(selStartY, newY);
				    selPosY2 = MAX(selStartY, newY);
				    RECT selBox2 =
				    {selPosX1* ttf.charWidth, selPosY1*ttf.charHeight, selPosX2*ttf.charWidth+ttf.charWidth, selPosY2*
				ttf.charHeight+ttf.charHeight};
				    if(memcmp(&selBox1, &selBox2, sizeof(RECT))) { // Update selected block if needed
					    HDC hDC = GetDC(vDosHwnd);
					    InvertRect(hDC, &selBox1);
					    InvertRect(hDC, &selBox2);
					    ReleaseDC(vDosHwnd, hDC);
				    }
				    break;
			    }
			    if(vga.mode == M_TEXT) {
				    if(!hasFocus)
					    break;
				    if(winMoving) {
					    RECT rect;
					    GetWindowRect(vDosHwnd, &rect);
// Repaint true due to Windows XP
					    MoveWindow(vDosHwnd,
					    rect.left+uAct.x-winMovingX,
					    rect.top+uAct.y-winMovingY,
					    rect.right-rect.left,
					    rect.bottom-rect.top,
					    TRUE);
					    break;
				    }
				    if(!window.framed && !ttf.fullScrn)
					    if((uAct.y < ttf.charHeight) != hasMinButton) {
						    hasMinButton = !hasMinButton;
						    curAttrChar[ttf.cols-1] = newAttrChar[ttf.cols-1]^0xffff;
					    }
				    if(usesMouse) {
					    uAct.x -= ttf.offX;
					    uAct.y -= ttf.offY;
					    if(uAct.x < 0 || uAct.x >= window.width || uAct.y < 0 || uAct.y >= window.height)
						    break;
					    Mouse_Moved(uAct.x, uAct.y, ttf.charWidth, ttf.charHeight);
				    }
			    }
			    else if(usesMouse)
				    Mouse_Moved(uAct.x, uAct.y, window.scale, window.scale);
			    break;
			case WM_LBUTTONDOWN:
			case WM_LBUTTONUP:
			case WM_RBUTTONDOWN:
			case WM_RBUTTONUP:
		    {
			    if(selectingText) { // End selecting text
				    vUpdateWin(); // To restore the window
				    ClipCursor(NULL); // Free mouse
				    if(OpenClipboard(NULL)) {
					    if(EmptyClipboard()) {
						    HGLOBAL hCbData = GlobalAlloc(NULL, 2*(selPosY2-selPosY1+1)*(selPosX2-selPosX1+3)-2);
						    Bit16u* pChData = (Bit16u*)GlobalLock(hCbData);
						    if(pChData) {
							    int dataIdx = 0;
							    for(int line = selPosY1; line <= selPosY2; line++) {
								    Bit16u * curAC = curAttrChar + line*ttf.cols + selPosX1;
								    int lastNotSpace = dataIdx;
								    for(int col = selPosX1; col <= selPosX2; col++) {
									    Bit8u textChar =  *(curAC++)&255;
									    pChData[dataIdx++] = cpMap[textChar];
									    if(textChar != 32)
										    lastNotSpace = dataIdx;
								    }
								    dataIdx = lastNotSpace;
								    if(line != selPosY2) { // Append line feed for all
									                   // bur the last line
									    pChData[dataIdx++] = 0x0d;
									    pChData[dataIdx++] = 0x0a;
								    }
								    curAC += ttf.cols;
							    }
							    pChData[dataIdx] = 0;
							    SetClipboardData(CF_UNICODETEXT, hCbData);
							    GlobalUnlock(hCbData);
						    }
					    }
					    CloseClipboard();
				    }
				    selectingText = false;
				    return;
			    }
// [Win][Ctrl+Left mouse button starts block select for clipboard copy
			    if(vga.mode == M_TEXT && !ttf.fullScrn && uAct.type == WM_LBUTTONDOWN && (uAct.flags1&4) && (uAct.flags&2)) {
				    selStartX = selPosX1 = selPosX2 = uAct.x/ttf.charWidth;
				    selStartY = selPosY1 = selPosY2 = uAct.y/ttf.charHeight;
				    RECT selBox =
				    {selPosX1* ttf.charWidth, selPosY1*ttf.charHeight, selPosX2*ttf.charWidth+ttf.charWidth, selPosY2*
					ttf.charHeight+ttf.charHeight};
				    HDC hDC = GetDC(vDosHwnd);
				    InvertRect(hDC, &selBox);
				    ReleaseDC(vDosHwnd, hDC);

				    RECT rcClip; // Restrict mouse to window
				    GetWindowRect(vDosHwnd, &rcClip);
				    ClipCursor(&rcClip);

				    selectingText = true;
				    break;
			    }
			    winMoving = false;
			    if(!window.framed && !ttf.fullScrn && vga.mode == M_TEXT && uAct.type == WM_LBUTTONDOWN) //Minimize?
				    if(uAct.y < ttf.charHeight) {
					    if(uAct.y <= ttf.charHeight/2 && uAct.x >= window.width-ttf.charWidth) {
						    ShowWindow(vDosHwnd, SW_MINIMIZE);
						    break;
					    }
					    winMoving = true;
					    winMovingX = uAct.x;
					    winMovingY = uAct.y;
				    }
			    if(usesMouse) {
				    switch(uAct.type)
				    {
					    case WM_LBUTTONDOWN:
						Mouse_ButtonPressed(0);
						break;
					    case WM_RBUTTONDOWN:
						Mouse_ButtonPressed(1);
						break;
					    case WM_LBUTTONUP:
						Mouse_ButtonReleased(0);
						break;
					    case WM_RBUTTONUP:
						Mouse_ButtonReleased(1);
						break;
				    }
			    }
			    break;
		    }
			case WM_CLOSE:
		    {
			    for(Bit8u handle = 0; handle < DOS_FILES; handle++)
				    if(Files[handle])
					    if((Files[handle]->GetInformation()&0x8000) == 0) { // If not device
						    MessageBox(vDosHwnd,
							    "One or more files are open!",
							    "Unsafe to exit this way",
							    MB_OK | MB_ICONWARNING);
						    return;
					    }
			    VideoQuit();
			    exit(0);
			    break;
		    }
			case WM_KEYDOWN:
			case WM_KEYUP:
// [Left or Right Alt][Enter] siwtches from window to "fullscreen"
			    if(uAct.type == WM_KEYDOWN && uAct.scancode == 28 && uAct.flags1&0x08) {
				    if(vga.mode != M_TEXT)
					    break;
				    ttf.fullScrn = !ttf.fullScrn;
				    HMONITOR monitor = MonitorFromWindow(vDosHwnd, MONITOR_DEFAULTTONEAREST);
				    MONITORINFO info;
				    info.cbSize = sizeof(MONITORINFO);
				    GetMonitorInfo(monitor, &info);
				    int maxWidth = info.rcMonitor.right-info.rcMonitor.left;
				    int maxHeight = info.rcMonitor.bottom-info.rcMonitor.top;
				    if(ttf.fullScrn) {
					    if(GetWindowRect(vDosHwnd, &prevPosition)) // Save position and point size
						    prevPointSize = ttf.pointsize;
					    while(ttf.cols*ttf.charWidth <= maxWidth && ttf.lins*ttf.charHeight <= maxHeight) //
						                                                                              // Very
						                                                                              // lazy
						                                                                              // method
						                                                                              // indeed
						    SelectFontByPoints(ttf.pointsize+2);
					    SelectFontByPoints(ttf.pointsize-2);
				    }
				    else {
					    ttf.offX = ttf.offY = 0;
					    if(prevPointSize) { // Switching back to windowed mode
						    if(ttf.pointsize != prevPointSize)
							    SelectFontByPoints(prevPointSize);
					    }
					    else if(window.framed) { // First switch to window mode with frame and
					                             // maximized font size could give a too big window
						    maxWidth -= GetSystemMetrics(SM_CXBORDER)*2;
						    maxHeight -= GetSystemMetrics(SM_CYCAPTION)+GetSystemMetrics(SM_CYBORDER)*2;
						    if(ttf.cols*ttf.charWidth > maxWidth || ttf.lins*ttf.charHeight > maxHeight) //
							                                                                         // If
							                                                                         // it
							                                                                         // doesn't
							                                                                         // fits
							                                                                         // on
							                                                                         // screen
							    SelectFontByPoints(ttf.pointsize-2);  // Should do the trick
					    }
				    }
				    SetVideoSize();
				    TimedSetSize();
				    if(!ttf.fullScrn && prevPointSize)
					    MoveWindow(vDosHwnd,
					    prevPosition.left,
					    prevPosition.top,
					    prevPosition.right-prevPosition.left,
					    prevPosition.bottom-prevPosition.top,
					    false);
				    memset(curAttrChar, -1, ttf.cols*ttf.lins*2);
				    break;
			    }
			    if(uAct.flags&2) { // Win key down
				    if(uAct.type == WM_KEYDOWN) {
// [Win][Ctrl]+C dumps screen to file screen.txt and opens it with the application (default notepad) assigned to .txt
// files
					    if((uAct.flags1&4) && uAct.scancode == 46)
						    dumpScreen();
// [Win][Ctrl]+V pastes clipboard into keyboard buffer
					    else if((uAct.flags1&4) && uAct.scancode == 47)
						    getClipboard();
					    // Window/font size modifiers
					    else if(!ttf.fullScrn)
						    if(uAct.scancode == 87) // F11
							    decreaseFontSize();
						    else if(uAct.scancode == 88) // F12
							    increaseFontSize();
				    }
			    }
			    else {
				    Mem_aStosb(BIOS_KEYBOARD_FLAGS1, uAct.flags1); // Update keyboard flags
				    Mem_aStosb(BIOS_KEYBOARD_FLAGS2, uAct.flags2);
				    Mem_aStosb(BIOS_KEYBOARD_FLAGS3, uAct.flags3);
				    Mem_aStosb(BIOS_KEYBOARD_LEDS, uAct.leds);
				    BIOS_AddKey(uAct.flags1, uAct.flags2, uAct.flags, uAct.scancode, uAct.unicode, uAct.type == WM_KEYDOWN);
			    }
			    break;
		}
	}
}

static FILE * LogFile;

void vLog(char const* format, ...)
{
	if(LogFile != NULL) {
		va_list args;
		va_start(args, format);
		vfprintf(LogFile, format, args);
		va_end(args);
		fprintf(LogFile, "\n");
		fflush(LogFile);
	}
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	if(!stricmp(lpCmdLine, "/log"))
		LogFile = fopen("vDos.log", "w");
	try {
		InitWindow(); // Init window
		TTF_Init(); // Init TTF
		vDos_Init();
	}
	catch(char * error) {
		MessageBox(NULL, error, "vDos - Exception", MB_OK|MB_ICONSTOP);
	}
	VideoQuit();
	return 0;
}

