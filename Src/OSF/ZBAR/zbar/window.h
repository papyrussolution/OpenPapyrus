/*------------------------------------------------------------------------
 *  Copyright 2007-2009 (c) Jeff Brown <spadix@users.sourceforge.net>
 *
 *  This file is part of the ZBar Bar Code Reader.
 *
 *  The ZBar Bar Code Reader is free software; you can redistribute it
 *  and/or modify it under the terms of the GNU Lesser Public License as
 *  published by the Free Software Foundation; either version 2.1 of
 *  the License, or (at your option) any later version.
 *
 *  The ZBar Bar Code Reader is distributed in the hope that it will be
 *  useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 *  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser Public License
 *  along with the ZBar Bar Code Reader; if not, write to the Free
 *  Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 *  Boston, MA  02110-1301  USA
 *
 *  http://sourceforge.net/projects/zbar
 *------------------------------------------------------------------------*/
#ifndef _WINDOW_H_
#define _WINDOW_H_

#include <config.h>
#ifdef HAVE_INTTYPES_H
	#include <inttypes.h>
#endif
#include <stdlib.h>

#include <zbar.h>
#include "symbol.h"
//#include "error.h"
#include "mutex.h"

typedef struct window_state_s window_state_t;

struct zbar_window_s {
	errinfo_t err;          /* error reporting */
	zbar_image_t * image;   /* last displayed image NB image access must be locked! */
	uint overlay;       /* user set overlay level */
	uint32 format;        /* output format */
	uint width, height; /* current output size */
	uint max_width, max_height;
	uint32 src_format;    /* current input format */
	uint src_width;     /* last displayed image size */
	uint src_height;
	uint dst_width;     /* conversion target */
	uint dst_height;
	uint scale_num;     /* output scaling */
	uint scale_den;
	point_t scaled_offset;  /* output position and size */
	point_t scaled_size;
	uint32 * formats;     /* supported formats (zero terminated) */
	zbar_mutex_t imglock;   /* lock displayed image */
	void * display;
	ulong xwin;
	ulong time;     /* last image display in milliseconds */
	ulong time_avg; /* average of inter-frame times */
	window_state_t * state; /* platform/interface specific state */
	/* interface dependent methods */
	int (* init)(zbar_window_t*, zbar_image_t*, int);
	int (* draw_image)(zbar_window_t*, zbar_image_t*);
	int (* cleanup)(zbar_window_t*);
};

/* window.draw has to be thread safe wrt/other apis
 * FIXME should be a semaphore
 */
static inline int window_lock(zbar_window_t * w)
{
	int rc = 0;
	if((rc = _zbar_mutex_lock(&w->imglock))) {
		err_capture(w, SEV_FATAL, ZBAR_ERR_LOCKING, __func__, "unable to acquire lock");
		w->err.errnum = rc;
		return -1;
	}
	return 0;
}

static inline int window_unlock(zbar_window_t * w)
{
	int rc = 0;
	if((rc = _zbar_mutex_unlock(&w->imglock))) {
		err_capture(w, SEV_FATAL, ZBAR_ERR_LOCKING, __func__, "unable to release lock");
		w->err.errnum = rc;
		return -1;
	}
	return 0;
}

static inline int _zbar_window_add_format(zbar_window_t * w, uint32 fmt)
{
	int i;
	for(i = 0; w->formats && w->formats[i]; i++)
		if(w->formats[i] == fmt)
			return(i);
	w->formats = (uint32 *)SAlloc::R(w->formats, (i + 2) * sizeof(uint32));
	w->formats[i] = fmt;
	w->formats[i + 1] = 0;
	return(i);
}

static inline point_t window_scale_pt(zbar_window_t * w, point_t p)
{
	p.x = ((long)p.x * w->scale_num + w->scale_den - 1) / w->scale_den;
	p.y = ((long)p.y * w->scale_num + w->scale_den - 1) / w->scale_den;
	return(p);
}

/* PAL interface */
extern int _zbar_window_attach(zbar_window_t*, void*, ulong);
extern int _zbar_window_expose(zbar_window_t*, int, int, int, int);
extern int _zbar_window_resize(zbar_window_t*);
extern int _zbar_window_clear(zbar_window_t*);
extern int _zbar_window_begin(zbar_window_t*);
extern int _zbar_window_end(zbar_window_t*);
extern int _zbar_window_draw_marker(zbar_window_t*, uint32, point_t);
extern int _zbar_window_draw_polygon(zbar_window_t*, uint32, const point_t*, int);
extern int _zbar_window_draw_text(zbar_window_t*, uint32,
    point_t, const char*);
extern int _zbar_window_fill_rect(zbar_window_t*, uint32, point_t, point_t);
extern int _zbar_window_draw_logo(zbar_window_t*);

#endif
