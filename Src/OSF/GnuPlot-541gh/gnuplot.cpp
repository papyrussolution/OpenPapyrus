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
// 
// Descr: allocate memory
//   This is a protected version of malloc. It causes an int_error
//   if there is not enough memory. If message is NULL, we allow NULL return.
//   Otherwise, we handle the error, using the message to create the int_error string.
//   Note cp/sp_extend uses realloc, so it depends on this using malloc().
// 
generic * FASTCALL gp_alloc(size_t size, const char * message)
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
// note gp_realloc assumes that failed realloc calls leave the original mem
// block allocated. If this is not the case with any C compiler, a substitute
// realloc function has to be used.
//
generic * gp_realloc(generic * p, size_t size, const char * message)
{
	char * res = 0; // the new allocation 
	// realloc(NULL,x) is meant to do malloc(x), but doesn't always 
	if(!p)
		res = (char *)gp_alloc(size, message);
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
// 
// Descr: allocates a curve_points structure that can hold 'num' points. Initialize all fields to NULL.
// 
/*static*/curve_points * GnuPlot::CpAlloc(int num) 
{
	lp_style_type default_lp_properties(lp_style_type::defCommon); // = DEFAULT_LP_STYLE_TYPE;
	curve_points * cp = (curve_points *)gp_alloc(sizeof(curve_points), "curve");
	memzero(cp, sizeof(curve_points));
	cp->p_max = (num >= 0 ? num : 0);
	if(num > 0)
		cp->points = (struct coordinate *)gp_alloc(num * sizeof(struct coordinate), "curve points");
	// Initialize various fields 
	cp->lp_properties = default_lp_properties;
	default_arrow_style(&(cp->arrow_properties));
	cp->fill_properties = default_fillstyle;
	cp->filledcurves_options = filledcurves_opts_data;
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
		if(cp->labels)
			free_labels(cp->labels);
		cp->labels = NULL;
		SAlloc::F(cp);
		cp = next;
	}
}
