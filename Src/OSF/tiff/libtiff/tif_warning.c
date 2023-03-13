/* $Header: /cvs/maptools/cvsroot/libtiff/libtiff/tif_warning.c,v 1.4 2017-07-04 12:54:42 erouault Exp $ */
/*
 * Copyright (c) 1988-1997 Sam Leffler
 * Copyright (c) 1991-1997 Silicon Graphics, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and
 * its documentation for any purpose is hereby granted without fee, provided
 * that (i) the above copyright notices and this permission notice appear in
 * all copies of the software and related documentation, and (ii) the names of
 * Sam Leffler and Silicon Graphics may not be used in any advertising or
 * publicity relating to the software without the specific, prior written
 * permission of Sam Leffler and Silicon Graphics.
 */
/*
 * TIFF Library.
 */
#include "tiffiop.h"
#pragma hdrstop

TIFFErrorHandlerExt _TIFFwarningHandlerExt = NULL;

TIFFErrorHandler TIFFSetWarningHandler(TIFFErrorHandler handler)
{
	TIFFErrorHandler prev = _TIFFwarningHandler;
	_TIFFwarningHandler = handler;
	return (prev);
}

TIFFErrorHandlerExt TIFFSetWarningHandlerExt(TIFFErrorHandlerExt handler)
{
	TIFFErrorHandlerExt prev = _TIFFwarningHandlerExt;
	_TIFFwarningHandlerExt = handler;
	return (prev);
}

void TIFFWarning(const char * module, const char * fmt, ...)
{
	va_list ap;
	if(_TIFFwarningHandler) {
		va_start(ap, fmt);
		(*_TIFFwarningHandler)(module, fmt, ap);
		va_end(ap);
	}
	if(_TIFFwarningHandlerExt) {
		va_start(ap, fmt);
		(*_TIFFwarningHandlerExt)(0, module, fmt, ap);
		va_end(ap);
	}
}

void TIFFWarningExt(thandle_t fd, const char * module, const char * fmt, ...)
{
	va_list ap;
	if(_TIFFwarningHandler) {
		va_start(ap, fmt);
		(*_TIFFwarningHandler)(module, fmt, ap);
		va_end(ap);
	}
	if(_TIFFwarningHandlerExt) {
		va_start(ap, fmt);
		(*_TIFFwarningHandlerExt)(fd, module, fmt, ap);
		va_end(ap);
	}
}
