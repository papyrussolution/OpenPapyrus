// GNUPLOT - dumb.trm 
// Copyright 1991 - 1993, 1998, 2004   Thomas Williams, Colin Kelley
/*
 * This file is included by ../term.c.
 *
 * This terminal driver supports: DUMB terminals
 *
 * AUTHORS: Francois Pinard, 91-04-03 INTERNET: pinard@iro.umontreal.ca; Ethan A Merritt Nov 2003
 *	Added support for enhanced text mode.
 *	Yes, this is frivolous, but it serves as an example for
 *	adding enhanced text to other terminals.  You can disable
 *	it by adding a line
 *	#define NO_DUMB_ENHANCED_SUPPORT
 *
 *   Bastian Maerkisch Nov 2016
 *	ANSI color support.  Filled polygons.
 *
 * send your comments or suggestions to (gnuplot-info@lists.sourceforge.net).
 */
#include <gnuplot.h>
#pragma hdrstop
#include "driver.h"

// @experimental {
#define TERM_BODY
#define TERM_PUBLIC static
#define TERM_TABLE
#define TERM_TABLE_START(x) GpTermEntry x {
#define TERM_TABLE_END(x)   };
// } @experimental

#ifdef TERM_REGISTER
	register_term(dumb_driver)
#endif

#ifdef TERM_BODY
#define DUMB_AXIS_CONST '\1'
#define DUMB_BORDER_CONST '\2'
#define DUMB_FILL_CONST '\3'
#define DUMB_NODRAW_CONST '\4'
#ifdef HAVE_STDINT_H
	#include <stdint.h>
#endif

//#define DUMB_PIXEL(x, y) GPO.TDumbB.P_Matrix[GPO.TDumbB.XMax*(y)+(x)]
//static void dumb_set_pixel(int x, int y, int v);

static struct gen_table DUMB_opts[] = {
	{ "f$eed", DUMB_FEED },
	{ "nof$eed", DUMB_NOFEED },
	{ "enh$anced", DUMB_ENH },
	{ "noe$nhanced", DUMB_NOENH },
	{ "size", DUMB_SIZE },
	{ "aspect", DUMB_ASPECT },
	{ "ansi", DUMB_ANSI },
	{ "ansi256", DUMB_ANSI256 },
	{ "ansirgb", DUMB_ANSIRGB },
	{ "mono", DUMB_NOCOLOR },
	{ NULL, DUMB_OTHER }
};

TERM_PUBLIC void DUMB_options(GpTermEntry * pThis, GnuPlot * pGp)
{
	int x, y;
	int cmd;
	bool set_size = FALSE;
	while(!pGp->Pgm.EndOfCommand()) {
		switch((cmd = pGp->Pgm.LookupTableForCurrentToken(&DUMB_opts[0]))) {
			case DUMB_FEED:
			    pGp->Pgm.Shift();
			    pGp->TDumbB.Feed = TRUE;
			    break;
			case DUMB_NOFEED:
			    pGp->Pgm.Shift();
			    pGp->TDumbB.Feed = FALSE;
			    break;
#ifndef NO_DUMB_ENHANCED_SUPPORT
			case DUMB_ENH:
			    pGp->Pgm.Shift();
			    pThis->put_text = ENHdumb_put_text;
			    pThis->flags |= TERM_ENHANCED_TEXT;
			    break;
			case DUMB_NOENH:
			    pGp->Pgm.Shift();
			    pThis->put_text = DUMB_put_text;
			    pThis->flags &= ~TERM_ENHANCED_TEXT;
			    break;
#endif
			case DUMB_ASPECT:
			    pGp->Pgm.Shift();
			    x = pGp->IntExpression();
			    y = 1;
			    if(!pGp->Pgm.EndOfCommand() && pGp->Pgm.EqualsCur(",")) {
				    pGp->Pgm.Shift();
				    y = pGp->IntExpression();
			    }
			    if(x <= 0) x = 1;
			    if(y <= 0) y = 1;
			    pThis->TicH = x;
			    pThis->TicV = y;
			    break;
			case DUMB_ANSI:
			case DUMB_ANSI256:
			case DUMB_ANSIRGB:
			    pGp->Pgm.Shift();
			    pGp->TDumbB.ColorMode = cmd;
#ifndef NO_DUMB_COLOR_SUPPORT
			    pThis->make_palette = DUMB_make_palette;
			    pThis->set_color = DUMB_set_color;
#endif
			    break;
			case DUMB_NOCOLOR:
			    pGp->Pgm.Shift();
			    pGp->TDumbB.ColorMode = 0;
			    pThis->make_palette = NULL;
			    pThis->set_color = GnuPlot::NullSetColor;
			    break;
			case DUMB_SIZE:
			    pGp->Pgm.Shift();
			// @fallthrough
			case DUMB_OTHER:
			default:
			    if(set_size) {
				    pGp->IntWarn(pGp->Pgm.GetCurTokenIdx(), "unrecognized option");
					pGp->Pgm.Shift();
				    break;
			    }
			    x = pGp->IntExpression();
			    if(x <= 0 || x > 1024)
				    x = DUMB_XMAX;
			    if(!pGp->Pgm.EndOfCommand()) {
				    if(pGp->Pgm.EqualsCur(","))
					    pGp->Pgm.Shift();
				    y = pGp->IntExpression();
				    if(y <= 0 || y > 1024)
					    y = DUMB_YMAX;
				    pGp->TDumbB.XMax = pThis->MaxX = x;
				    pGp->TDumbB.YMax = pThis->MaxY = y;
			    }
			    set_size = TRUE;
			    break;
		}
	}
	{
		const char * coloropts[] = {"mono", "ansi", "ansi256", "ansirgb"};
		sprintf(term_options, "%sfeed %s size %d, %d aspect %i, %i %s", pGp->TDumbB.Feed ? "" : "no",
		    pThis->put_text == ENHdumb_put_text ? "enhanced" : "", pGp->TDumbB.XMax, pGp->TDumbB.YMax,
		    pThis->TicH, pThis->TicV, coloropts[pGp->TDumbB.ColorMode == 0 ? 0 : pGp->TDumbB.ColorMode - DUMB_ANSI + 1]);
	}
}

static void dumb_set_pixel(GpTermEntry * pThis, int x, int y, int v)
{
	GnuPlot * p_gp = pThis->P_Gp;
	char * charpixel;
	if((uint)x <= p_gp->TDumbB.XMax /* ie x>=0 && x<=p_gp->TDumbB.XMax */ && (uint)y <= p_gp->TDumbB.YMax) {
		charpixel = (char *)(&p_gp->TDumbB.P_Matrix[p_gp->TDumbB.XMax * y + x]);
		// null-terminate single ascii character (needed for UTF-8) 
		p_gp->TDumbB.P_Matrix[p_gp->TDumbB.XMax * y + x] = 0;
		*charpixel = v;
#ifndef NO_DUMB_COLOR_SUPPORT
		memcpy(&p_gp->TDumbB.P_Colors[p_gp->TDumbB.XMax * y + x], &p_gp->TDumbB.Color, sizeof(t_colorspec));
#endif
	}
}

void DUMB_init(GpTermEntry * pThis)
{
	GnuPlot * p_gp = pThis->P_Gp;
	int size = (p_gp->TDumbB.XMax+1) * (p_gp->TDumbB.YMax+1);
	p_gp->TDumbB.P_Matrix = (charcell *)SAlloc::R(p_gp->TDumbB.P_Matrix, size*sizeof(charcell));
#ifndef NO_DUMB_COLOR_SUPPORT
	p_gp->TDumbB.P_Colors = (t_colorspec *)SAlloc::R(p_gp->TDumbB.P_Colors, size*sizeof(t_colorspec));
#endif
}

void DUMB_graphics(GpTermEntry * pThis)
{
	GnuPlot * p_gp = pThis->P_Gp;
	int size = (p_gp->TDumbB.XMax+1) * (p_gp->TDumbB.YMax+1);
	charcell * pm = p_gp->TDumbB.P_Matrix;
	memzero(p_gp->TDumbB.P_Matrix, size * sizeof(charcell));
#ifndef NO_DUMB_COLOR_SUPPORT
	memzero(p_gp->TDumbB.P_Colors, size * sizeof(t_colorspec));
#endif
	for(int i = 0; i<size; i++) {
		char * c = (char *)pm++;
		*c = ' ';
	}
}

void DUMB_text(GpTermEntry * pThis)
{
	GnuPlot * p_gp = pThis->P_Gp;
	int x, y, i;
	putc('\f', gpoutfile);
	for(y = p_gp->TDumbB.YMax - 1; y >= 0; y--) {
#ifndef NO_DUMB_COLOR_SUPPORT
		if(p_gp->TDumbB.ColorMode > 0) {
			fputs("\033[0;39m", gpoutfile); /* reset colors to default */
			memzero(&p_gp->TDumbB.PrevColor, sizeof(t_colorspec));
		}
#endif
		for(x = 0; x < p_gp->TDumbB.XMax; x++) {
			char * c;
#ifndef NO_DUMB_COLOR_SUPPORT
			t_colorspec * color = &p_gp->TDumbB.P_Colors[p_gp->TDumbB.XMax*y + x];
			const char * colorstring = p_gp->AnsiColorString(color, &p_gp->TDumbB.PrevColor);
			if(colorstring[0] != NUL)
				fputs(colorstring, gpoutfile);
			memcpy(&p_gp->TDumbB.PrevColor, color, sizeof(t_colorspec));
#endif
			c = (char *)(&p_gp->TDumbB.P_Matrix[p_gp->TDumbB.XMax*y + x]);
			// The UTF-8 character might be four bytes long and so there's no guarantee that the charcell ends in a NUL. 
			for(i = 0; i < sizeof(charcell) && *c != NUL; i++, c++)
				fputc(*c, gpoutfile);
		}
		if(p_gp->TDumbB.Feed || y > 0)
			putc('\n', gpoutfile);
	}
#ifndef NO_DUMB_COLOR_SUPPORT
	if(p_gp->TDumbB.ColorMode > 0) {
		fputs("\033[0;39;49m", gpoutfile); /* reset colors to default */
	}
#endif
	fflush(gpoutfile);
}

void DUMB_reset(GpTermEntry * pThis)
{
	GnuPlot * p_gp = pThis->P_Gp;
	SAlloc::F(p_gp->TDumbB.P_Matrix);
	p_gp->TDumbB.P_Matrix = NULL;
#ifndef NO_DUMB_COLOR_SUPPORT
	SAlloc::F(p_gp->TDumbB.P_Colors);
	p_gp->TDumbB.P_Colors = NULL;
#endif
}

void DUMB_linetype(GpTermEntry * pThis, int linetype)
{
	GnuPlot * p_gp = pThis->P_Gp;
	static char pen_type[7] = { '*', '#', '$', '%', '@', '&', '=' };
	if(linetype == LT_BLACK)
		p_gp->TDumbB.Pen = DUMB_BORDER_CONST;
	else if(linetype == LT_AXIS)
		p_gp->TDumbB.Pen = DUMB_AXIS_CONST;
	else if(linetype == LT_NODRAW)
		p_gp->TDumbB.Pen = DUMB_NODRAW_CONST;
	else if(linetype <= LT_NODRAW)
		p_gp->TDumbB.Pen = ' ';
	else {
		linetype = linetype % 7;
		p_gp->TDumbB.Pen = pen_type[linetype];
	}
#ifndef NO_DUMB_COLOR_SUPPORT
	p_gp->TDumbB.Color.type = TC_LT;
	p_gp->TDumbB.Color.lt = linetype;
#endif
}

void DUMB_move(GpTermEntry * pThis, uint x, uint y)
{
	GnuPlot * p_gp = pThis->P_Gp;
	p_gp->TDumbB.X = x;
	p_gp->TDumbB.Y = y;
}

void DUMB_point(GpTermEntry * pThis, uint x, uint y, int point)
{
	dumb_set_pixel(pThis, x, y, point == -1 ? '.' : point % 26 + 'A');
}

void DUMB_vector(GpTermEntry * pThis, uint arg_x, uint arg_y)
{
	GnuPlot * p_gp = pThis->P_Gp;
	int x = arg_x; // we need signed int, since unsigned-signed=unsigned and 
	int y = arg_y; // abs and cast to double wouldn't work 
	char pen, pen1;
	int delta;
	if(p_gp->TDumbB.Pen == DUMB_NODRAW_CONST) {
		DUMB_move(pThis, x, y);
		return;
	}
	if(ABS(y - p_gp->TDumbB.Y) > ABS(x - p_gp->TDumbB.X)) {
		switch(p_gp->TDumbB.Pen) {
			case DUMB_AXIS_CONST:
			    pen = ':';
			    pen1 = '+';
			    break;
			case DUMB_BORDER_CONST:
			    pen = '|';
			    pen1 = '+';
			    break;
			case DUMB_FILL_CONST:
			    pen = pen1 = 'X';
			    break;
			default:
			    pen = p_gp->TDumbB.Pen;
			    pen1 = p_gp->TDumbB.Pen;
			    break;
		}
		dumb_set_pixel(pThis, p_gp->TDumbB.X, p_gp->TDumbB.Y, pen1);
		for(delta = 1; delta < ABS(y - p_gp->TDumbB.Y); delta++) {
			dumb_set_pixel(pThis, p_gp->TDumbB.X  + (int)((double)(x - p_gp->TDumbB.X) * delta / ABS(y - p_gp->TDumbB.Y) + 0.5), p_gp->TDumbB.Y + delta * sign(y - p_gp->TDumbB.Y), pen);
		}
		dumb_set_pixel(pThis, x, y, pen1);
	}
	else if(ABS(x - p_gp->TDumbB.X) > ABS(y - p_gp->TDumbB.Y)) {
		switch(p_gp->TDumbB.Pen) {
			case DUMB_AXIS_CONST:
			    pen = '.';
			    pen1 = '+';
			    break;
			case DUMB_BORDER_CONST:
			    pen = '-';
			    pen1 = '+';
			    break;
			case DUMB_FILL_CONST:
			    pen = pen1 = 'X';
			    break;
			default:
			    pen = p_gp->TDumbB.Pen;
			    pen1 = p_gp->TDumbB.Pen;
			    break;
		}
		dumb_set_pixel(pThis, p_gp->TDumbB.X, p_gp->TDumbB.Y, pen1);
		for(delta = 1; delta < ABS(x - p_gp->TDumbB.X); delta++)
			dumb_set_pixel(pThis, p_gp->TDumbB.X + delta * sign(x - p_gp->TDumbB.X),
			    p_gp->TDumbB.Y + (int)((double)(y - p_gp->TDumbB.Y) * delta / ABS(x - p_gp->TDumbB.X) + 0.5), pen);
		dumb_set_pixel(pThis, x, y, pen1);
	}
	else {
		switch(p_gp->TDumbB.Pen) {
			case DUMB_AXIS_CONST: /* zero length axis */
			    pen = '+';
			    break;
			case DUMB_BORDER_CONST: /* zero length border */
			    pen = '+';
			    break;
			case DUMB_FILL_CONST:
			    pen = '#';
			    break;
			default:
			    pen = p_gp->TDumbB.Pen;
			    break;
		}
		for(delta = 0; delta <= ABS(x - p_gp->TDumbB.X); delta++)
			dumb_set_pixel(pThis, p_gp->TDumbB.X + delta * sign(x - p_gp->TDumbB.X), p_gp->TDumbB.Y + delta * sign(y - p_gp->TDumbB.Y), pen);
	}
	p_gp->TDumbB.X = x;
	p_gp->TDumbB.Y = y;
}

static void utf8_copy_one(GpTermEntry * pThis, char * dest, const char * orig)
{
	const char * nextchar = orig;
	ulong wch;
	*(charcell *)dest = 0; // zero-fill 
	if(encoding != S_ENC_UTF8)
		*dest = *orig;
	else {
		// Valid UTF8 byte sequence 
		if(utf8toulong(&wch, &nextchar)) {
			while(orig < nextchar)
				*dest++ = *orig++;
		}
		else {
			pThis->P_Gp->IntWarn(NO_CARET, "invalid UTF-8 byte sequence");
			*dest++ = *orig++;
		}
	}
}

void DUMB_put_text(GpTermEntry * pThis, uint x, uint y, const char * str)
{
	GnuPlot * p_gp = pThis->P_Gp;
	if(y <= p_gp->TDumbB.YMax) {
		const int length = gp_strlen(str);
		if(x + length > p_gp->TDumbB.XMax)
			x = MAX(0, p_gp->TDumbB.XMax - length);
		for(int i = 0; i < length && x < p_gp->TDumbB.XMax; i++, x++) {
			utf8_copy_one(pThis, reinterpret_cast<char *>(&p_gp->TDumbB.Pixel(x, y)), gp_strchrn(str, i));
	#ifndef NO_DUMB_COLOR_SUPPORT
			memcpy(&p_gp->TDumbB.P_Colors[p_gp->TDumbB.XMax * y + x], &p_gp->TDumbB.Color, sizeof(t_colorspec));
	#endif
		}
	}
}

TERM_PUBLIC void DUMB_arrow(GpTermEntry * pThis, uint usx, uint usy, uint uex, uint uey, int head)  /* mostly ignored */
{
	GnuPlot * p_gp = pThis->P_Gp;
	// we have GOT to ditch this unsigned coord madness! 
	int sx = (int)(usx);
	int sy = (int)(usy);
	int ex = (int)(uex);
	int ey = (int)(uey);
	char saved_pen = p_gp->TDumbB.Pen;
	char saved_x = p_gp->TDumbB.X;
	char saved_y = p_gp->TDumbB.Y;
	// Arrow shaft 
	if(ex == sx) p_gp->TDumbB.Pen = '|';
	else if(ey == sy) p_gp->TDumbB.Pen = '-';
	else p_gp->TDumbB.Pen = '.';
	p_gp->TDumbB.X = sx;
	p_gp->TDumbB.Y = sy;
	if(!(head & HEADS_ONLY))
		DUMB_vector(pThis, ex, ey);
	// Arrow tail 
	if(head & BACKHEAD) {
		char tailsym;
		if(ex > sx) tailsym = '<';
		else if(ex < sx) tailsym = '>';
		else if(ey > sy) tailsym = 'v';
		else tailsym = '^';
		dumb_set_pixel(pThis, sx, sy, tailsym);
	}
	// Arrow head 
	if(head & END_HEAD) {
		char headsym;
		if(ex > sx) headsym = '>';
		else if(ex < sx) headsym = '<';
		else if(ey > sy) headsym = '^';
		else headsym = 'v';
		dumb_set_pixel(pThis, ex, ey, headsym);
	}
	p_gp->TDumbB.Pen = saved_pen;
	p_gp->TDumbB.X = saved_x;
	p_gp->TDumbB.Y = saved_y;
}

#ifndef NO_DUMB_ENHANCED_SUPPORT
/*
 * The code from here on serves as an example of how to
 * add enhanced text mode support to even a dumb driver.
 */

static bool ENHdumb_opened_string;
static bool ENHdumb_show = TRUE;
static int ENHdumb_overprint = 0;
static bool ENHdumb_widthflag = TRUE;
static uint ENHdumb_xsave, ENHdumb_ysave;
#define ENHdumb_fontsize 1
#define ENHdumb_font ""
static double ENHdumb_base;

void ENHdumb_OPEN(GpTermEntry * pThis, char * fontname, double fontsize, double base, bool widthflag, bool showflag, int overprint)
{
	GnuPlot * p_gp = pThis->P_Gp;
	// There are two special cases:
	// overprint = 3 means save current position
	// overprint = 4 means restore saved position
	if(overprint == 3) {
		ENHdumb_xsave = p_gp->TDumbB.X;
		ENHdumb_ysave = p_gp->TDumbB.Y;
		return;
	}
	else if(overprint == 4) {
		DUMB_move(pThis, ENHdumb_xsave, ENHdumb_ysave);
		return;
	}
	if(!ENHdumb_opened_string) {
		ENHdumb_opened_string = TRUE;
		// Start new text fragment 
		p_gp->Enht.P_CurText = &p_gp->Enht.Text[0];
		// Scale fractional font height to vertical units of display 
		ENHdumb_base = base * 2 / fontsize;
		// Keep track of whether we are supposed to show this string 
		ENHdumb_show = showflag;
		// 0/1/2  no overprint / 1st pass / 2nd pass 
		ENHdumb_overprint = overprint;
		// widthflag FALSE means do not update text position after printing 
		ENHdumb_widthflag = widthflag;
		// Many drivers will need to do something about font selection here 
		// but dumb is dumb 
	}
}

void ENHdumb_FLUSH(GpTermEntry * pThis)
{
	GnuPlot * p_gp = pThis->P_Gp;
	char * str = p_gp->Enht.Text; /* The fragment to print */
	int x = p_gp->TDumbB.X;         /* The current position  */
	int y = p_gp->TDumbB.Y + (int)ENHdumb_base;
	int i, len;
	if(ENHdumb_opened_string) {
		*p_gp->Enht.P_CurText = '\0';
		len = gp_strlen(str);
		// print the string fragment, perhaps invisibly 
		// NB: base expresses offset from current y pos 
		if(ENHdumb_show && y < p_gp->TDumbB.YMax) {
			for(i = 0; i < len && x < p_gp->TDumbB.XMax; i++, x++) {
				utf8_copy_one(pThis, reinterpret_cast<char *>(&p_gp->TDumbB.Pixel(x, y)), gp_strchrn(str, i));
#ifndef NO_DUMB_COLOR_SUPPORT
				memcpy(&p_gp->TDumbB.P_Colors[p_gp->TDumbB.XMax * y + x], &p_gp->TDumbB.Color, sizeof(t_colorspec));
#endif
			}
		}
		if(!ENHdumb_widthflag)
			; // don't update position 
		else if(ENHdumb_overprint == 1)
			p_gp->TDumbB.X += len / 2; // First pass of overprint, leave position in center of fragment 
		else
			p_gp->TDumbB.X += len; // Normal case is to update position to end of fragment 
		ENHdumb_opened_string = FALSE;
	}
}

void ENHdumb_put_text(GpTermEntry * pThis, uint x, uint y, const char * str)
{
	GnuPlot * p_gp = pThis->P_Gp;
	int length;
	// If no enhanced text processing is needed, we can use the plain  
	// vanilla put_text() routine instead of this fancy recursive one. 
	if(p_gp->Enht.Ignore || (!strpbrk(str, "{}^_@&~") && !contains_unicode(str))) {
		DUMB_put_text(pThis, x, y, str);
		return;
	}
	length = p_gp->EstimateStrlen(str, NULL);
	if(x + length > p_gp->TDumbB.XMax)
		x = MAX(0, p_gp->TDumbB.XMax - length);
	if(y > p_gp->TDumbB.YMax)
		return;
	// Set up global variables needed by enhanced_recursion() 
	p_gp->Enht.FontScale = 1.0;
	ENHdumb_opened_string = FALSE;
	strncpy(p_gp->Enht.EscapeFormat, "%c", sizeof(p_gp->Enht.EscapeFormat));
	DUMB_move(pThis, x, y);
	/* Set the recursion going. We say to keep going until a
	 * closing brace, but we don't really expect to find one.
	 * If the return value is not the nul-terminator of the
	 * string, that can only mean that we did find an unmatched
	 * closing brace in the string. We increment past it (else
	 * we get stuck in an infinite loop) and try again.
	 */
	while(*(str = enhanced_recursion(pThis, (char *)str, TRUE, ENHdumb_font, ENHdumb_fontsize, 0.0, TRUE, TRUE, 0))) {
		pThis->enhanced_flush(pThis);
		// I think we can only get here if *str == '}' 
		enh_err_check(str);
		if(!*++str)
			break; /* end of string */
		// else carry on and process the rest of the string 
	}
}
#endif /* NO_DUMB_ENHANCED_SUPPORT */

#ifndef NO_DUMB_COLOR_SUPPORT
	int DUMB_make_palette(GpTermEntry * pThis, t_sm_palette * palette)
	{
		return 0; // report continuous colors 
	}
	void DUMB_set_color(GpTermEntry * pThis, const t_colorspec * colorspec)
	{
		memcpy(&pThis->P_Gp->TDumbB.Color, colorspec, sizeof(t_colorspec));
	}
#endif

static int dumb_float_compare(const void * elem1, const void * elem2)
{
	int val = static_cast<int>(*static_cast<const float *>(elem1) - *static_cast<const float *>(elem2));
	return (0 < val) - (val < 0);
}
//
// adopted copy from caca.trm 
//
TERM_PUBLIC void dumb_filled_polygon(GpTermEntry * pThis, int points, gpiPoint * corners)
{
	GnuPlot * p_gp = pThis->P_Gp;
	char save_pen;
	// Eliminate duplicate polygon points. 
	if((corners[0].x == corners[points-1].x) && (corners[0].y == corners[points-1].y))
		points--;
	// Need at least three remaining points 
	if(points < 3)
		return;
	// temporarily change pen 
	save_pen = p_gp->TDumbB.Pen;
	p_gp->TDumbB.Pen = DUMB_FILL_CONST;
	{
		/* ----------------------------------------------------------------
		 * Derived from
		 *  public-domain code by Darel Rex Finley, 2007
		 *  http://alienryderflex.com/polygon_fill/
		 * ---------------------------------------------------------------- */
		int nodes;
		float * nodeX;
		int pixelY;
		int i, j;
		int ymin = p_gp->TDumbB.YMax, ymax = 0;
		int xmin = p_gp->TDumbB.XMax, xmax = 0;
		// Find bounding box 
		for(i = 0; i < points; i++) {
			SETMIN(xmin, corners[i].x);
			SETMAX(xmax, corners[i].x);
			SETMIN(ymin, corners[i].y);
			SETMAX(ymax, corners[i].y);
		}
		// Dynamically allocate node list. 
		nodeX = (float*)SAlloc::M(sizeof(*nodeX) * points);
		// Loop through the rows of the image. 
		for(pixelY = ymin; pixelY <= ymax + 1; pixelY++) {
			// Build a sorted list of nodes. 
			nodes = 0;
			j = points - 1;
			for(i = 0; i < points; i++) {
				if(((corners[i].y < pixelY) && (corners[j].y >= pixelY)) || ((corners[j].y < pixelY) && (corners[i].y >= pixelY))) {
					nodeX[nodes++] = static_cast<float>(corners[i].x + +(double)(pixelY - corners[i].y) / (double)(corners[j].y - corners[i].y) * (double)(corners[j].x - corners[i].x));
				}
				j = i;
			}
			qsort(nodeX, nodes, sizeof(float), dumb_float_compare);
			// Fill the pixels between node pairs. 
			for(i = 0; i < nodes; i += 2) {
				if(nodeX[i] > xmax)
					break;
				if(nodeX[i + 1] >= 0) {
					// TODO: Are these checks ever required? 
					SETMAX(nodeX[i], static_cast<float>(xmin));
					SETMIN(nodeX[i+1], static_cast<float>(xmax));
					// skip lines with zero length 
					if(nodeX[i + 1] - nodeX[i] < 0.5)
						continue;
					DUMB_move(pThis, (int)(nodeX[i] + 0.5), pixelY);
					DUMB_vector(pThis, (int)(nodeX[i + 1]), pixelY);
				}
			}
		}
		/* cleanup */
		SAlloc::F(nodeX);
		/* ---------------------------------------------------------------- */
	}
	// restore pen 
	p_gp->TDumbB.Pen = save_pen;
}

#endif /* TERM_BODY */

#ifdef TERM_TABLE
TERM_TABLE_START(dumb_driver)
	"dumb", 
	"ascii art for anything that prints text",
	DUMB_XMAX, 
	DUMB_YMAX, 
	1, 
	1,
	1, 
	2,     /* account for typical aspect ratio of characters */
	DUMB_options, 
	DUMB_init, 
	DUMB_reset,
	DUMB_text, 
	GnuPlot::NullScale, 
	DUMB_graphics, 
	DUMB_move, 
	DUMB_vector,
	DUMB_linetype, 
	DUMB_put_text, 
	GnuPlot::NullTextAngle,
	GnuPlot::NullJustifyText, 
	DUMB_point, 
	DUMB_arrow, 
	set_font_null,
	0,                              /* pointsize */
	TERM_CAN_MULTIPLOT,
	NULL, 
	NULL, 
	NULL, 
	NULL,
	#ifdef USE_MOUSE
	NULL, 
	NULL, 
	NULL, 
	NULL, 
	NULL,
	#endif
	NULL,           /* Color support sets this to DUMB_make_palette */
	NULL,           /* previous_palette */
	NULL,           /* Color support sets this to DUMB_set_color */
	dumb_filled_polygon,     /* filled_polygon */
	NULL,     /* image */
	#ifndef NO_DUMB_ENHANCED_SUPPORT
	ENHdumb_OPEN, 
	ENHdumb_FLUSH, 
	do_enh_writec
	#endif /* NO_DUMB_ENHANCED_SUPPORT */
TERM_TABLE_END(dumb_driver)

#undef LAST_TERM
#define LAST_TERM dumb_driver

#endif /* TERM_TABLE */

#ifdef TERM_HELP
START_HELP(dumb)
"1 dumb",
"?commands set terminal dumb",
"?set terminal dumb",
"?set term dumb",
"?terminal dumb",
"?term dumb",
"?dumb",
" The `dumb` terminal driver plots into a text block using ascii characters.",
" It has an optional size specification and a trailing linefeed flag.",
"",
" Syntax:",
"       set terminal dumb {size <xchars>,<ychars>} {[no]feed}",
"                         {aspect <htic>{,<vtic>}}",
#ifndef NO_DUMB_ENHANCED_SUPPORT
"                         {[no]enhanced}",
#endif
#ifndef NO_DUMB_COLOR_SUPPORT
"                         {mono|ansi|ansi256|ansirgb}",
#endif
"",
" where <xchars> and <ychars> set the size of the text block. The default is",
" 79 by 24. The last newline is printed only if `feed` is enabled.",
"",
" The `aspect` option can be used to control the aspect ratio of the plot by",
" setting the length of the horizontal and vertical tic marks. Only integer",
" values are allowed. Default is 2,1 -- corresponding to the aspect ratio of",
" common screen fonts.",
"",
#ifndef NO_DUMB_COLOR_SUPPORT
" The `ansi`, `ansi256`, and `ansirgb` options will include escape",
" sequences in the output to handle colors.  Note that these might",
" not be handled by your terminal.  Default is `mono`.",
" To obtain the best color match in `ansi` mode, you should use",
" `set colorsequence classic`.",
" Depending on the mode, the `dumb` terminal will emit the",
" following sequences (without the additional whitespace):",
"",
"       ESC [ 0 m           reset attributes to defaults",
"       foreground color:",
"       ESC [ 1 m           set intense/bold",
"       ESC [ 22 m          intense/bold off",
"       ESC [ <fg> m        with color code 30 <= <fg> <= 37",
"       ESC [ 39 m          reset to default",
"       ESC [ 38; 5; <c> m  with palette index 16 <= <c> <= 255",
"       ESC [ 38; 2; <r>; <g>; <b> m  with components 0 <= <r,g,b> <= 255",
"       background color:",
"       ESC [ <bg> m        with color code 40 <= <bg> <= 47",
"       ESC [ 49 m          reset to default",
"       ESC [ 48; 5; <c> m  with palette index 16 <= <c> <= 231",
"       ESC [ 48; 2; <r>; <g>; <b> m  with components 0 <= <r,g,b> <= 255",
"",
" See also e.g. the description at",
"^ <a href=\"https://en.wikipedia.org/wiki/ANSI_escape_code#Colors\">",
"           https://en.wikipedia.org/wiki/ANSI_escape_code#Colors",
"^ </a>",
"",
#endif
" Example:",
"       set term dumb mono size 60,15 aspect 1",
"       set tics nomirror scale 0.5",
"       plot [-5:6.5] sin(x) with impulse ls -1",
"",
"           1 +-------------------------------------------------+",
"         0.8 +|||++                   ++||||++                 |",
"         0.6 +|||||+                 ++|||||||+  sin(x) +----+ |",
"         0.4 +||||||+               ++|||||||||+               |",
"         0.2 +|||||||+             ++|||||||||||+             +|",
"           0 ++++++++++++++++++++++++++++++++++++++++++++++++++|",
"        -0.2 +        +|||||||||||+              +|||||||||||+ |",
"        -0.4 +         +|||||||||+                +|||||||||+  |",
"        -0.6 +          +|||||||+                  +|||||||+   |",
"        -0.8 +           ++||||+                    ++||||+    |",
"          -1 +---+--------+--------+-------+--------+--------+-+",
"                -4       -2        0       2        4        6  "
END_HELP(dumb)
#endif /* TERM_HELP */
