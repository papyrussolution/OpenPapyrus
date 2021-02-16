// Hello, Emacs, this is -*-C-*- 
// GNUPLOT - estimate.trm 
/*
 * This file is included by ../src/term.c via term.h.
 *
 * This terminal driver supports:
 *   On return from ENHest_put_text()
 *	(*term)->xmax = estimated string width in characters
 *	(*term)->ymax = estimated string height (in tenths of a character height)
 *	ENHest_plaintext = non-enhanced text approximation of original string
 *
 * AUTHORS
 *
 *   Ethan A Merritt - Dec 2004
 *
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

#ifdef TERM_PROTO
	TERM_PUBLIC void ENHest_put_text(GpTermEntry * pThis, uint x, uint y, const char str[]);
	TERM_PUBLIC void ENHest_OPEN(GpTermEntry * pThis, char * fontname, double fontsize, double base, bool widthflag, bool showflag, int overprint);
	TERM_PUBLIC void ENHest_FLUSH(GpTermEntry * pThis);
#endif
#ifdef TERM_BODY
static double ENHest_x, ENHest_y;
static double ENHest_xsave, ENHest_ysave;
static double ENHest_fragment_width;
static double ENHest_fontsize;
static double ENHest_min_height, ENHest_max_height;
static double ENHest_total_width;
static int ENHest_plaintext_buflen = 0;
static int ENHest_plaintext_pos = 0;
char * ENHest_plaintext = NULL;

static bool ENHest_opened_string;
static bool ENHest_show = TRUE;
static int ENHest_overprint = 0;
static bool ENHest_widthflag = TRUE;
#define ENHest_font ""
static double ENHest_base;
//static size_t strwidth_utf8(const char * s); // Internal routines for UTF-8 support 
// 
// DEBUG 
// place holder for an estimate of the current font size at the time
// we switched from a "real" terminal to this special-purpose terminal.
// 
#define ENHest_ORIG_FONTSIZE 12.

TERM_PUBLIC void ENHest_OPEN(GpTermEntry * pThis, char * fontname, double fontsize, double base, bool widthflag, bool showflag, int overprint)
{
	// There are two special cases:
	// overprint = 3 means save current position
	// overprint = 4 means restore saved position
	if(overprint == 3) {
		ENHest_xsave = ENHest_x;
		ENHest_ysave = ENHest_y;
	}
	else if(overprint == 4) {
		ENHest_x = ENHest_xsave;
		ENHest_y = ENHest_ysave;
	}
	else if(!ENHest_opened_string) {
		ENHest_opened_string = TRUE;
		// Start new text fragment 
		ENHest_fragment_width = 0;
		// font size will be used to estimate width of each character 
		ENHest_fontsize = fontsize;
		// Scale fractional font height 
		ENHest_base = base * 1.0;
		SETMAX(ENHest_max_height, ENHest_base + fontsize);
		SETMIN(ENHest_min_height, ENHest_base);
		FPRINTF((stderr, "ENHest_OPEN: base %g fontsize %g  min %g max %g\n", base, fontsize, ENHest_min_height, ENHest_max_height));
		// Keep track of whether we are supposed to show this string 
		ENHest_show = showflag;
		// 0/1/2  no overprint / 1st pass / 2nd pass 
		ENHest_overprint = overprint;
		// widthflag FALSE means do not update text position after printing 
		ENHest_widthflag = widthflag;
	}
}

TERM_PUBLIC void ENHest_FLUSH(GpTermEntry * pThis)
{
	double len = ENHest_fragment_width;
	if(ENHest_opened_string) {
		ENHest_fragment_width = 0;
		if(!ENHest_widthflag)
			; // don't update position 
		else if(ENHest_overprint == 1)
			ENHest_x += len / 2; // First pass of overprint, leave position in center of fragment 
		else
			ENHest_x += len; // Normal case is to update position to end of fragment 
		ENHest_total_width = MAX(ENHest_total_width, ENHest_x);
		ENHest_opened_string = FALSE;
	}
}

TERM_PUBLIC void ENHest_put_text(GpTermEntry * pThis, uint x, uint y, const char * str)
{
	// Set up global variables needed by enhanced_recursion() 
	ENHest_fontsize  = ENHest_ORIG_FONTSIZE;
	ENHest_opened_string = FALSE;
	ENHest_max_height = ENHest_fontsize;
	ENHest_min_height = 0.0;
	ENHest_total_width = 0.0;
	strncpy(GPO.Enht.EscapeFormat, ".", sizeof(GPO.Enht.EscapeFormat));
	// buffer in which we will return plaintext version of enhanced text string 
	while(ENHest_plaintext_buflen <= strlen(str)) {
		ENHest_plaintext_buflen += MAX_ID_LEN;
		ENHest_plaintext = (char *)gp_realloc(ENHest_plaintext, ENHest_plaintext_buflen+1, "ENHest_plaintext");
	}
	ENHest_plaintext[0] = '\0';
	ENHest_plaintext_pos = 0;
	// If no enhanced text processing is needed, strlen() is sufficient 
	if(GPO.Enht.Ignore || (!strpbrk(str, "{}^_@&~\n") && !contains_unicode(str))) {
		term->MaxX = (encoding == S_ENC_UTF8) ? strwidth_utf8(str) : strlen(str);
		term->MaxY = 10; /* 1 character height */
		strcpy(ENHest_plaintext, str);
	}
	else {
		ENHest_x = x;
		ENHest_y = y;
		while(*(str = enhanced_recursion(term, (char*)str, TRUE, ENHest_font, ENHest_fontsize, 0.0, TRUE, TRUE, 0))) {
			(term->enhanced_flush)(pThis);
			enh_err_check(str);
			if(!*++str)
				break; // end of string 
		}
		ENHest_plaintext[ENHest_plaintext_pos] = '\0';
		if(ENHest_x > 0.0 && ENHest_x < 1.0)
			ENHest_x = 1;
		term->MaxX = ENHest_total_width;
		term->MaxY = 10. * (ENHest_max_height - ENHest_min_height)/ENHest_ORIG_FONTSIZE + 0.5;
	}
}

TERM_PUBLIC void ENHest_writec(GpTermEntry * pThis, int c)
{
	if(c == '\n') {
		ENHest_FLUSH(pThis);
		ENHest_opened_string = TRUE;
		ENHest_min_height -= 1.0 * ENHest_fontsize;
		ENHest_base -= 1.0 * ENHest_fontsize;
		ENHest_x = 0;
	}
	if(encoding == S_ENC_UTF8) {
		/* Skip all but the first byte of UTF-8 multi-byte characters. */
		if((c & 0xc0) != 0x80) {
			ENHest_fragment_width += 1;
			/* [most] characters above 0x3000 are square CJK glyphs, */
			/* which are wider than western characters.              */
			if((uchar)c >= 0xec)
				ENHest_fragment_width += 1;
		}
	}
	else
		ENHest_fragment_width += 1;
	ENHest_plaintext[ENHest_plaintext_pos++] = c;
}

GpTermEntry ENHest = {
	"estimate", 
	NULL,
	1, 
	1, 
	1, 
	1, 
	1, 
	1,
	NULL, 
	NULL, 
	NULL,
	NULL, 
	NULL, 
	NULL, 
	NULL, 
	NULL,
	NULL, 
	ENHest_put_text, 
	NULL,
	NULL, 
	NULL, 
	NULL, 
	NULL,
	0, 
	0,                   /* pointsize, flags */
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
	NULL, 
	NULL, 
	NULL, 
	NULL, 
	NULL, 
	ENHest_OPEN, 
	ENHest_FLUSH, 
	ENHest_writec
};

#endif /* TERM_BODY */
