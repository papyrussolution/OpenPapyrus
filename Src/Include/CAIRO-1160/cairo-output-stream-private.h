/* cairo - a vector graphics library with display and print output
 *
 * Copyright © 2006 Red Hat, Inc
 *
 * This library is free software; you can redistribute it and/or
 * modify it either under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation
 * (the "LGPL") or, at your option, under the terms of the Mozilla
 * Public License Version 1.1 (the "MPL"). If you do not alter this
 * notice, a recipient may use your version of this file under either
 * the MPL or the LGPL.
 *
 * You should have received a copy of the LGPL along with this library
 * in the file COPYING-LGPL-2.1; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA 02110-1335, USA
 * You should have received a copy of the MPL along with this library
 * in the file COPYING-MPL-1.1
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * The Original Code is the cairo graphics library.
 * The Initial Developer of the Original Code is Red Hat, Inc.
 * Author(s): Kristian Høgsberg <krh@redhat.com>
 */
#ifndef CAIRO_OUTPUT_STREAM_PRIVATE_H
#define CAIRO_OUTPUT_STREAM_PRIVATE_H

typedef cairo_status_t (* cairo_output_stream_write_func_t) (cairo_output_stream_t * output_stream, const uchar * data, uint length);
typedef cairo_status_t (* cairo_output_stream_flush_func_t) (cairo_output_stream_t * output_stream);
typedef cairo_status_t (* cairo_output_stream_close_func_t) (cairo_output_stream_t * output_stream);

struct _cairo_output_stream {
	cairo_output_stream_write_func_t write_func;
	cairo_output_stream_flush_func_t flush_func;
	cairo_output_stream_close_func_t close_func;
	ulong position;
	cairo_status_t status;
	boolint closed;
};

extern const cairo_private cairo_output_stream_t _cairo_output_stream_nil;

cairo_private void _cairo_output_stream_init(cairo_output_stream_t * stream, cairo_output_stream_write_func_t write_func, cairo_output_stream_flush_func_t flush_func, cairo_output_stream_close_func_t close_func);
cairo_private cairo_status_t _cairo_output_stream_fini(cairo_output_stream_t * stream);

/* We already have the following declared in cairo.h:
   typedef cairo_status_t (*cairo_write_func_t) (void * closure, const uchar *data, uint length);
 */
typedef cairo_status_t (* cairo_close_func_t)(void * closure);

/* This function never returns %NULL. If an error occurs (NO_MEMORY)
 * while trying to create the output stream this function returns a
 * valid pointer to a nil output stream.
 *
 * Note that even with a nil surface, the close_func callback will be
 * called by a call to _cairo_output_stream_close or
 * _cairo_output_stream_destroy.
 */
cairo_private cairo_output_stream_t * _cairo_output_stream_create(cairo_write_func_t write_func, cairo_close_func_t close_func, void * closure);
cairo_private cairo_output_stream_t * _cairo_output_stream_create_in_error(cairo_status_t status);
/* Tries to flush any buffer maintained by the stream or its delegates. */
cairo_private cairo_status_t _cairo_output_stream_flush(cairo_output_stream_t * stream);

/* Returns the final status value associated with this object, just
 * before its last gasp. This final status value will capture any
 * status failure returned by the stream's close_func as well. */
cairo_private cairo_status_t _cairo_output_stream_close(cairo_output_stream_t * stream);

/* Returns the final status value associated with this object, just
 * before its last gasp. This final status value will capture any
 * status failure returned by the stream's close_func as well. */
cairo_private cairo_status_t FASTCALL _cairo_output_stream_destroy(cairo_output_stream_t * stream);
cairo_private void STDCALL _cairo_output_stream_write(cairo_output_stream_t * stream, const void * data, size_t length);
cairo_private void _cairo_output_stream_write_hex_string(cairo_output_stream_t * stream, const uchar * data, size_t length);
cairo_private void _cairo_output_stream_vprintf(cairo_output_stream_t * stream, const char * fmt, va_list ap) CAIRO_PRINTF_FORMAT(2, 0);
cairo_private void _cairo_output_stream_printf(cairo_output_stream_t * stream, const char * fmt, ...) CAIRO_PRINTF_FORMAT(2, 3);
/* Print matrix element values with rounding of insignificant digits. */
cairo_private void FASTCALL _cairo_output_stream_print_matrix(cairo_output_stream_t * stream, const cairo_matrix_t * matrix);
cairo_private long FASTCALL _cairo_output_stream_get_position(const cairo_output_stream_t * stream);
cairo_private cairo_status_t FASTCALL _cairo_output_stream_get_status(const cairo_output_stream_t * stream);

/* This function never returns %NULL. If an error occurs (NO_MEMORY or
 * WRITE_ERROR) while trying to create the output stream this function
 * returns a valid pointer to a nil output stream.
 *
 * Note: Even if a nil surface is returned, the caller should still
 * call _cairo_output_stream_destroy (or _cairo_output_stream_close at
 * least) in order to ensure that everything is properly cleaned up.
 */
cairo_private cairo_output_stream_t * _cairo_output_stream_create_for_filename(const char * filename);

/* This function never returns %NULL. If an error occurs (NO_MEMORY or
 * WRITE_ERROR) while trying to create the output stream this function
 * returns a valid pointer to a nil output stream.
 *
 * The caller still "owns" file and is responsible for calling fclose
 * on it when finished. The stream will not do this itself.
 */
cairo_private cairo_output_stream_t * _cairo_output_stream_create_for_file(FILE * file);
cairo_private cairo_output_stream_t * _cairo_memory_stream_create();
cairo_private void _cairo_memory_stream_copy(cairo_output_stream_t * base, cairo_output_stream_t * dest);
cairo_private int _cairo_memory_stream_length(cairo_output_stream_t * stream);
cairo_private cairo_status_t _cairo_memory_stream_destroy(cairo_output_stream_t * abstract_stream, uchar ** data_out, ulong * length_out);
cairo_private cairo_output_stream_t * _cairo_null_stream_create();
/* cairo-base85-stream.c */
cairo_private cairo_output_stream_t * _cairo_base85_stream_create(cairo_output_stream_t * output);
/* cairo-base64-stream.c */
cairo_private cairo_output_stream_t * _cairo_base64_stream_create(cairo_output_stream_t * output);
/* cairo-deflate-stream.c */
cairo_private cairo_output_stream_t * _cairo_deflate_stream_create(cairo_output_stream_t * output);

#endif /* CAIRO_OUTPUT_STREAM_PRIVATE_H */
