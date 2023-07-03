/* $Header: /cvs/maptools/cvsroot/libtiff/libtiff/tif_error.c,v 1.6 2017-07-04 12:54:42 erouault Exp $ */
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
#include <slib-internal.h>
#pragma hdrstop
#include "tiffiop.h"

TIFFErrorHandlerExt _TIFFerrorHandlerExt = NULL;

TIFFErrorHandler TIFFSetErrorHandler(TIFFErrorHandler handler)
{
	TIFFErrorHandler prev = _TIFFerrorHandler;
	_TIFFerrorHandler = handler;
	return (prev);
}

TIFFErrorHandlerExt TIFFSetErrorHandlerExt(TIFFErrorHandlerExt handler)
{
	TIFFErrorHandlerExt prev = _TIFFerrorHandlerExt;
	_TIFFerrorHandlerExt = handler;
	return (prev);
}

void TIFFError(const char * module, const char * fmt, ...)
{
	va_list ap;
	if(_TIFFerrorHandler) {
		va_start(ap, fmt);
		(*_TIFFerrorHandler)(module, fmt, ap);
		va_end(ap);
	}
	if(_TIFFerrorHandlerExt) {
		va_start(ap, fmt);
		(*_TIFFerrorHandlerExt)(0, module, fmt, ap);
		va_end(ap);
	}
}

void TIFFErrorExt(thandle_t fd, const char * module, const char * fmt, ...)
{
	va_list ap;
	if(_TIFFerrorHandler) {
		va_start(ap, fmt);
		(*_TIFFerrorHandler)(module, fmt, ap);
		va_end(ap);
	}
	if(_TIFFerrorHandlerExt) {
		va_start(ap, fmt);
		(*_TIFFerrorHandlerExt)(fd, module, fmt, ap);
		va_end(ap);
	}
}

void FASTCALL TIFFErrorExtOutOfMemory(thandle_t fd, const char * module)
{
	TIFFErrorExt(fd, module, SlTxtOutOfMem);
}
