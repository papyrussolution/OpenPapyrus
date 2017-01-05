#include <stdlib.h>
#include "logging.h"
#include "vDos.h"
#include "support.h"
#include "ttf.h"
#include "video.h"

#include <ft2build.h>
#include FT_FREETYPE_H

#define NUM_GRAYS	256

// Handy routines for converting from fixed point
#define FT_FLOOR(X)	(X>>6)
#define FT_CEIL(X)	((X+63)>>6)

// Cached glyph information
typedef struct {
	FT_Bitmap pixmap;
	int minx;
	int maxx;
	int miny;
	int maxy;
	int yoffset;
	} c_glyph;

// The structure used to hold internal font information
struct _TTF_Font
	{
	FT_Face face;
	int height;
	int width;
	int ascent;
	int descent;
	int underline_offset;
	int underline_height;
	c_glyph cache[256];																// Cached glyphs
	};

static FT_Library library = NULL;													// The FreeType font engine/library

void TTF_Init(void)
	{
	if (FT_Init_FreeType(&library))
		E_Exit("TTF: Couldn't init FreeType engine");
	}

void TTF_Flush_Cache(TTF_Font* font)
	{
	for (int i = 0; i < 256; ++i)
		if (font->cache[i].pixmap.buffer)
			{
			free(font->cache[i].pixmap.buffer);
			font->cache[i].pixmap.buffer = 0;
			}	
	}

void TTF_SetCharSize(TTF_Font* font, int ptsize)
	{
	TTF_Flush_Cache(font);
	FT_Face face = font->face;
	if (FT_Set_Char_Size(face, 0, ptsize*64, 0, 0))									// Set the character size and use default DPI (72)
		E_Exit("TTF: Couldn't set font size");

	// Get the scalable font metrics for this font
	FT_Fixed scale = face->size->metrics.y_scale;
	font->ascent  = FT_CEIL(FT_MulFix(face->ascender, scale));
	font->descent = FT_CEIL(FT_MulFix(face->descender, scale));
	font->height  = font->ascent-font->descent;
	font->underline_offset = FT_FLOOR(FT_MulFix(face->underline_position, scale));
	font->underline_height = FT_FLOOR(FT_MulFix(face->underline_thickness, scale));
	if (font->underline_height < 1)
		font->underline_height = 1;
	if (font->ascent-font->underline_offset+font->underline_height > font->height)	// With some font(s) underline position can exceed the height (Catastrophic Anomaly)
		font->underline_offset = font->ascent-font->height+font->underline_height;
	font->width = FT_FLOOR(FT_MulFix(face->max_advance_width+32, face->size->metrics.x_scale));	// Round it
	}

TTF_Font* TTF_New_Memory_Face(const FT_Byte* file_base, FT_Long file_size, int ptsize)
	{
	TTF_Font *font = (TTF_Font*)malloc(sizeof *font);
	if (font == NULL)
		E_Exit("TTF: Out of memory");
	memset(font, 0, sizeof(*font));

	if (FT_New_Memory_Face(library, file_base, file_size, 0, &font->face))
		E_Exit("TTF: Couldn't init font");
	FT_Face face = font->face;
	if (!FT_IS_SCALABLE(face))														// Make sure that our font face is scalable (global metrics)
		E_Exit("TTF: Font is not scalable");

	for (int i = 0; i < face->num_charmaps; i++)									// Set charmap for loaded font
		{
		FT_CharMap charmap = face->charmaps[i];
		if (charmap->platform_id == 3 && charmap->encoding_id == 1)					// Windows Unicode
			{
			FT_Set_Charmap(face, charmap);
			break;
			}
		}
 
	TTF_SetCharSize(font, ptsize);
	bool fontOK = false;
	if (!FT_Load_Glyph(face, 0, FT_LOAD_DEFAULT))									// Test pixel mode
		if (!FT_Render_Glyph(font->face->glyph, FT_RENDER_MODE_NORMAL))				// Render the glyph
			if (font->face->glyph->bitmap.pixel_mode == FT_PIXEL_MODE_GRAY)
				fontOK = true;
	if (!fontOK)
		E_Exit("TTF: Font is not 8 bits gray scale");
	return font;
	}

static c_glyph* LoadGlyph(TTF_Font* font, Bit8u ch)
	{
	c_glyph* cached = &font->cache[ch];
	if (cached->pixmap.buffer)														// If already cached
		return cached;

	FT_Face face = font->face;
	if (FT_Load_Glyph(face, FT_Get_Char_Index(face, cpMap[ch]), FT_LOAD_NO_AUTOHINT))
		return 0;

	FT_GlyphSlot glyph = face->glyph;
	FT_Glyph_Metrics* metrics = &glyph->metrics;

	cached->minx = FT_FLOOR(metrics->horiBearingX);
	cached->maxx = FT_CEIL(metrics->horiBearingX+metrics->width);
	cached->maxy = FT_FLOOR(metrics->horiBearingY);
	cached->miny = cached->maxy-FT_CEIL(metrics->height);
	cached->yoffset = font->ascent-cached->maxy;

	if (FT_Render_Glyph(glyph, FT_RENDER_MODE_NORMAL))								// Render the glyph
		return 0;

	FT_Bitmap* src = &glyph->bitmap;												// Copy information to cache
	FT_Bitmap* dst = &cached->pixmap;
	memcpy(dst, src, sizeof(*dst));
	if (dst->rows != 0)
		{
		int len = dst->pitch*dst->rows;
		dst->buffer = (unsigned char *)malloc(len);
		if (!dst->buffer)
			return 0;
		memcpy(dst->buffer, src->buffer, len);
		}
	return cached;
	}

void TTF_CloseFont(TTF_Font* font)
	{
	if (font)
		{
		TTF_Flush_Cache(font);
		if (font->face)
			FT_Done_Face(font->face);
		free(font);
		}
	}

int TTF_FontHeight(const TTF_Font *font)
{
	return(font->height);
}

int TTF_GlyphIsProvided(const TTF_Font *font, Bit16u ch)
{
	return(FT_Get_Char_Index(font->face, ch));
}

int TTF_FontWidth(TTF_Font *font)
{
	return font->width;
}

vSurface* TTF_RenderASCII(TTF_Font* font, const char* text, int textlen, RGB_Color fg, RGB_Color bg, int style)
{
	vSurface* surface = vCreateSurface(textlen*font->width, font->height, fg, bg);	// Create the target surface
	if(surface == NULL)
		E_Exit("TTF: RenderASCII couldn't create surface");
	int prevChar = 256;
	for(int xPos = 0; xPos < textlen; xPos++) { // Load and render each character
		if(text[xPos] == prevChar) { // If it's the same as the previous, just copy
			// Could be futher optimzed counting repeated characters
			Bit8u* dst = (Bit8u*)surface->pixels+xPos*font->width; // and using a "smart" copy procedure
			for(int row = 0; row < font->height; row++) { // but updating the window is no hotspot
				memcpy(dst, dst-font->width, font->width);
				dst += surface->w;
			}
		}
		else {
			prevChar = text[xPos];
			c_glyph *glyph = LoadGlyph(font, prevChar);
			if(!glyph)
				E_Exit("TTF: Couldn't load glyph");
			FT_Bitmap* pixmap = &glyph->pixmap;
			int width = pixmap->width;
			if(width > font->width)
				width = font->width;
			if(width+glyph->minx > font->width)
				width--;
			int xstart = xPos*font->width;
			for(int row = 0; row < (int)pixmap->rows; ++row) {
				if(row+glyph->yoffset < 0 || row+glyph->yoffset >= surface->h) // Make sure we don't go either over, or under the limit
					continue;
				Bit8u* dst = (Bit8u*)surface->pixels+(row+glyph->yoffset)*surface->w+xstart+glyph->minx;
				Bit8u* src = pixmap->buffer+row*pixmap->pitch;
				memmove(dst, src, width);
			}
		}
	}
	if(style&TTF_STYLE_UNDERLINE) // Add underline
		memset((Bit8u *)surface->pixels+(font->ascent-font->underline_offset-1)*surface->w, NUM_GRAYS-1, font->underline_height*surface->w);
	if(style&TTF_STYLE_STRIKETHROUGH) // Add strikethrough
		memset((Bit8u *)surface->pixels+(font->height/2)*surface->w, NUM_GRAYS-1, font->underline_height*surface->w);
	return surface;
}

void TTF_Quit(void)
	{
	if (library)
		FT_Done_FreeType(library);
	}
