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
extern GpTermEntry ENHest;

GnuPlot::GnuPlot() : TermPointSize(1.0), TermInitialised(false), TermGraphics(false), TermSuspended(false), TermOpenedBinary(false), TermForceInit(false)
{
	ENHest.P_Gp = this;
}
// 
// Descr: allocate memory
//   This is a protected version of malloc. It causes an int_error
//   if there is not enough memory. If message is NULL, we allow NULL return.
//   Otherwise, we handle the error, using the message to create the int_error string.
//   Note cp/sp_extend uses realloc, so it depends on this using malloc().
// 
#if 0 // {
generic * FASTCALL gp_alloc_Removed(size_t size, const char * message)
{
	char * p = (char *)SAlloc::M(size); /* the new allocation */   /* try again */
	if(!p) {
		// really out of memory 
		if(message) {
			GPO.IntError(NO_CARET, "out of memory for %s", message);
			// NOTREACHED 
		}
		// else we return NULL 
	}
	return (p);
}
//
// note SAlloc::R assumes that failed realloc calls leave the original mem
// block allocated. If this is not the case with any C compiler, a substitute
// realloc function has to be used.
//
generic * gp_realloc_Removed(generic * p, size_t size, const char * message)
{
	char * res = 0; // the new allocation 
	// realloc(NULL,x) is meant to do malloc(x), but doesn't always 
	if(!p)
		res = (char *)SAlloc::M(size);
	else {
		res = (char *)SAlloc::R(p, size);
		if(!res) {
			if(message) {
				GPO.IntError(NO_CARET, "out of memory for %s", message);
				// NOTREACHED 
			}
			// else we return NULL 
		}
	}
	return res;
}
#endif // } 0
// 
// Descr: allocates a curve_points structure that can hold 'num' points. Initialize all fields to NULL.
// 
/*static*/curve_points * GnuPlot::CpAlloc(int num) 
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
	cp->fill_properties = GPO.Gg.default_fillstyle;
	cp->filledcurves_options = GPO.Gg.filledcurves_opts_data;
	return (cp);
}
// 
// Descr: releases any memory which was previously malloc()'d to hold curve points (and recursively down the linked list).
// 
/*static*/void GnuPlot::CpFree(curve_points * cp)
{
	while(cp) {
		curve_points * next = cp->next;
		ZFREE(cp->title);
		ZFREE(cp->title_position);
		ZFREE(cp->points);
		ZFREE(cp->varcolor);
		free_labels(cp->labels);
		cp->labels = NULL;
		SAlloc::F(cp);
		cp = next;
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
static char * P_DynarrayInitFailureMsg = "dynarray wasn't initialized";
// 
// The 'constructor' of a dynarray object: initializes all the
// variables to well-defined startup values 
// 
void init_dynarray(dynarray * pThis, size_t entry_size, long size, long increment)
{
	pThis->v = 0;            /* preset value, in case SAlloc::M fails */
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
void resize_dynarray(dynarray * pThis, long newsize)
{
	if(!pThis->v)
		GPO.IntError(NO_CARET, P_DynarrayInitFailureMsg);
	if(newsize == 0)
		free_dynarray(pThis);
	else {
		pThis->v = SAlloc::R(pThis->v, pThis->entry_size * newsize);
		pThis->size = newsize;
	}
}
// 
// Increase the size of the dynarray by a given amount 
// 
void extend_dynarray(dynarray * pThis, long increment)
{
	resize_dynarray(pThis, pThis->size + increment);
}
// 
// Get pointer to the element one past the current end of the dynamic
// array. Resize it if necessary. Returns a pointer-to-void to that element. 
// 
void * nextfrom_dynarray(dynarray * pThis)
{
	if(!pThis->v)
		GPO.IntError(NO_CARET, P_DynarrayInitFailureMsg);
	if(pThis->end >= pThis->size)
		extend_dynarray(pThis, pThis->increment);
	return (PTR8(pThis->v) + pThis->entry_size * (pThis->end++));
}
// 
// Release the element at the current end of the dynamic array, by
// moving the 'end' index one element backwards 
// 
void droplast_dynarray(dynarray * pThis)
{
	if(!pThis->v)
		GPO.IntError(NO_CARET, P_DynarrayInitFailureMsg);
	if(pThis->end)
		pThis->end--;
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