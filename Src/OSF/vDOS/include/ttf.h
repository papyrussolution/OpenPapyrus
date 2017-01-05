#ifndef _TTF_H
#define _TTF_H

#include "video.h"

// The internal structure containing font information
typedef struct _TTF_Font TTF_Font;

// Initialize the TTF engine
extern void TTF_Init(void);

extern TTF_Font* TTF_New_Memory_Face(const unsigned char* file_base, long file_size, int ptsize);
extern void TTF_SetCharSize(TTF_Font* font, int ptsize);

#define TTF_STYLE_NORMAL	0x00
#define TTF_STYLE_BOLD		0x01
#define TTF_STYLE_ITALIC	0x02
#define TTF_STYLE_UNDERLINE	0x04
#define TTF_STYLE_STRIKETHROUGH	0x08

// Get the total height of the font - usually equal to point size
extern int TTF_FontHeight(const TTF_Font *font);

// Check wether a glyph is provided by the font or not
extern int TTF_GlyphIsProvided(const TTF_Font *font, Bit16u ch);

extern int TTF_FontWidth(TTF_Font *font);

typedef struct {
	struct {
		Bitu width, height;
		int minX, minY, maxX, maxY;
	} cache;
	RGB_Color pal[256];
} Render_t;

typedef struct {
	TTF_Font *font;
	bool	vDos;																	// Is internal vDos font active
	int		pointsize;																// Pointsize of characters
	int		charHeight;																// Height of character cell
	int		charWidth;																// Width ,,
	int		cursor;																	// Cursor position
	int		lins;																	// Number of lines 24-60
	int		cols;																	// Number of columns 80-160
	bool	fullScrn;																// In fake fullscreen
	int		offX;																	// Horizontal offset to center content
	int		offY;																	// vertical ,,
} Render_ttf;

extern Render_t render;
extern Render_ttf ttf;
extern Bit16u curAttrChar[];					// currently displayed textpage
extern Bit16u *newAttrChar;						// to be replaced by


/* Create an 8-bit palettized surface and render the given text at
   high quality with the given font and colors.  The 0 pixel is background,
   while other pixels have varying degrees of the foreground color.
*/
extern vSurface* TTF_RenderASCII(TTF_Font* font, const char* text, int textlen, RGB_Color fg, RGB_Color bg, int style);

// After changing the code page, the glyph cache has to be cleared!
extern void TTF_Flush_Cache(TTF_Font* font);
// Close an opened font file
extern void TTF_CloseFont(TTF_Font *font);

// De-initialize the TTF engine
extern void TTF_Quit(void);

#endif /* _TTF_H */
