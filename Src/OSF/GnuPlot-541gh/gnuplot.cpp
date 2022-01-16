// GNUPLOT.CPP
//
#include <gnuplot.h>
#pragma hdrstop
//
// Version
//
const char gnuplot_version[] = "5.5";
const char gnuplot_patchlevel[] = "0";
#ifdef DEVELOPMENT_VERSION
	#include "timestamp.h"
#else
	const char gnuplot_date[] = "2020-02-07 ";
#endif
const char gnuplot_copyright[] = "Copyright (C) 1986-1993, 1998, 2004, 2007-2020";
const char faq_location[] = FAQ_LOCATION;
char * compile_options = 0; // Will be loaded at runtime 
const char bug_email[] = "gnuplot-beta@lists.sourceforge.net";
const char help_email[] = "gnuplot-beta@lists.sourceforge.net";
//
//
//
GnuPlot GPO; // @global
//extern GpTermEntry ENHest;

void ENHest_put_text(GpTermEntry * pThis, uint x, uint y, const char * str);
void ENHest_OPEN(GpTermEntry * pThis, const char * fontname, double fontsize, double base, bool widthflag, bool showflag, int overprint);
void ENHest_FLUSH(GpTermEntry * pThis);
void ENHest_writec(GpTermEntry * pThis, int c);

GnuPlot::GnuPlot() : TermPointSize(1.0), TermInitialised(false), TermGraphics(false), TermSuspended(false), TermOpenedBinary(false), TermForceInit(false),
	P_LfHead(0), P_PushTermName(0), P_PushTermOpts(0), VarShowAll(false),
	Pm3dLastSetPaletteMode(SMPAL_COLOR_MODE_NONE), EnableResetPalette(1)
{
	_ENHest = {
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
	_ENHest.P_Gp = this;
	PrevPltt = { -1, (palette_color_mode)-1, -1, -1, -1, -1, -1, -1, (rgb_color*)0, /*-1*/true };
}

GnuPlot::~GnuPlot()
{
	SAlloc::F(P_PushTermName);
	SAlloc::F(P_PushTermOpts);
}
// 
// Descr: allocates a curve_points structure that can hold 'num' points. Initialize all fields to NULL.
// 
curve_points * GnuPlot::CpAlloc(int num) 
{
	lp_style_type default_lp_properties(lp_style_type::defCommon); // = DEFAULT_LP_STYLE_TYPE;
	curve_points * cp = (curve_points *)SAlloc::M(sizeof(curve_points));
	memzero(cp, sizeof(curve_points));
	cp->p_max = (num >= 0 ? num : 0);
	if(num > 0)
		cp->points = (GpCoordinate *)SAlloc::M(num * sizeof(GpCoordinate));
	// Initialize various fields 
	cp->lp_properties = default_lp_properties;
	default_arrow_style(&(cp->arrow_properties));
	cp->fill_properties = Gg.default_fillstyle;
	cp->filledcurves_options = Gg.filledcurves_opts_data;
	return (cp);
}
// 
// Descr: releases any memory which was previously malloc()'d to hold curve points (and recursively down the linked list).
// 
void GnuPlot::CpFree(curve_points * cp)
{
	while(cp) {
		curve_points * next = cp->next;
		ZFREE(cp->title);
		ZFREE(cp->title_position);
		ZFREE(cp->points);
		ZFREE(cp->varcolor);
		free_labels(cp->labels);
		cp->labels = NULL;
		FREEANDASSIGN(cp, next);
	}
}
// 
// DynArray author: Hans-Bernhard Broeker
//
// This module implements a dynamically growing array of arbitrary
// elements parametrized by their sizeof(). There is no 'access
// function', i.e. you'll have to access the elements of the
// dynarray->v memory block by hand. It's implemented in OO-style,
// even though this is C, not C++.  In particular, every function
// takes a pointer to a data structure type 'dynarray', which mimics
// the 'this' pointer of an object. 
// 
static const char * P_DynarrayInitFailureMsg = "dynarray wasn't initialized";
// 
// The 'constructor' of a dynarray object: initializes all the
// variables to well-defined startup values 
// 
void init_dynarray(dynarray * pThis, size_t entry_size, long size, long increment)
{
	pThis->v = 0; /* preset value, in case SAlloc::M fails */
	if(size)
		pThis->v = SAlloc::M(entry_size*size);
	pThis->size = size;
	pThis->end = 0;
	pThis->increment = increment;
	pThis->entry_size = entry_size;
}
// 
// The 'destructor'; sets all crucial elements of the structure to
// well-defined values to avoid problems by use of bad pointers... 
// 
void free_dynarray(dynarray * pThis)
{
	ZFREE(pThis->v); // should work, even if SAlloc::M failed 
	pThis->end = pThis->size = 0;
}
// 
// Set the size of the dynamical array to a new, fixed value 
// 
//void resize_dynarray(dynarray * pThis, long newsize)
void GnuPlot::ResizeDynArray(dynarray * pArray, long newsize)
{
	if(!pArray->v)
		IntError(NO_CARET, P_DynarrayInitFailureMsg);
	if(newsize == 0)
		free_dynarray(pArray);
	else {
		pArray->v = SAlloc::R(pArray->v, pArray->entry_size * newsize);
		pArray->size = newsize;
	}
}
// 
// Increase the size of the dynarray by a given amount 
// 
//void extend_dynarray(dynarray * pThis, long increment)
void GnuPlot::ExtendDynArray(dynarray * pArray, long increment)
{
	ResizeDynArray(pArray, pArray->size + increment);
}
// 
// Get pointer to the element one past the current end of the dynamic
// array. Resize it if necessary. Returns a pointer-to-void to that element. 
// 
//void * nextfrom_dynarray(dynarray * pThis)
void * GnuPlot::NextFromDynArray(dynarray * pArray)
{
	if(!pArray->v)
		IntError(NO_CARET, P_DynarrayInitFailureMsg);
	if(pArray->end >= pArray->size)
		ExtendDynArray(pArray, pArray->increment);
	return (PTR8(pArray->v) + pArray->entry_size * (pArray->end++));
}
// 
// Release the element at the current end of the dynamic array, by
// moving the 'end' index one element backwards 
// 
//void droplast_dynarray(dynarray * pThis)
void GnuPlot::DropLastDynArray(dynarray * pArray)
{
	if(!pArray->v)
		IntError(NO_CARET, P_DynarrayInitFailureMsg);
	if(pArray->end)
		pArray->end--;
}
// 
// This routine accounts for multi-byte characters in UTF-8.
// NB:  It does not return the _number_ of characters in the string, but
// rather their approximate _width_ in units of typical character width.
// As with the ENHest_writec() routine, it approximates the width of characters
// above unicode 0x3000 as being twice that of western alphabetic characters.
// 
size_t FASTCALL strwidth_utf8(const char * s) 
{
	int j = 0;
	for(int i = 0; s[i];) {
		if((s[i] & 0xc0) != 0x80) {
			j++;
			if((uchar)(s[i]) >= 0xe3)
				j++;
		}
		i++;
	}
	return j;
}