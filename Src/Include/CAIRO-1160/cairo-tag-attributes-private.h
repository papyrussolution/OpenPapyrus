/* -*- Mode: c; c-basic-offset: 4; indent-tabs-mode: t; tab-width: 8; -*- */
/* cairo - a vector graphics library with display and print output
 *
 * Copyright © 2016 Adrian Johnson
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
 * The Initial Developer of the Original Code is Adrian Johnson.
 *
 * Contributor(s): Adrian Johnson <ajohnson@redneon.com>
 */
#ifndef CAIRO_TAG_ATTRIBUTES_PRIVATE_H
#define CAIRO_TAG_ATTRIBUTES_PRIVATE_H

typedef enum {
	TAG_LINK_INVALID = 0,
	TAG_LINK_EMPTY,
	TAG_LINK_DEST,
	TAG_LINK_URI,
	TAG_LINK_FILE,
} cairo_tag_link_type_t;

typedef struct _cairo_link_attrs {
	cairo_tag_link_type_t link_type;
	cairo_array_t rects;
	char * dest;
	char * uri;
	char * file;
	int page;
	boolint has_pos;
	cairo_point_double_t pos;
} cairo_link_attrs_t;

typedef struct _cairo_dest_attrs {
	char * name;
	double x;
	double y;
	boolint x_valid;
	boolint y_valid;
	boolint internal;
} cairo_dest_attrs_t;

typedef struct _cairo_ccitt_params {
	int columns;
	int rows;
	int k;
	boolint end_of_line;
	boolint encoded_byte_align;
	boolint end_of_block;
	boolint black_is_1;
	int damaged_rows_before_error;
} cairo_ccitt_params_t;

typedef struct _cairo_eps_params {
	cairo_box_double_t bbox;
} cairo_eps_params_t;

cairo_private cairo_int_status_t _cairo_tag_parse_link_attributes(const char * attributes, cairo_link_attrs_t * link_attrs);
cairo_private cairo_int_status_t _cairo_tag_parse_dest_attributes(const char * attributes, cairo_dest_attrs_t * dest_attrs);
cairo_private cairo_int_status_t _cairo_tag_parse_ccitt_params(const char * attributes, cairo_ccitt_params_t * dest_attrs);
cairo_private cairo_int_status_t _cairo_tag_parse_eps_params(const char * attributes, cairo_eps_params_t * dest_attrs);

#endif /* CAIRO_TAG_ATTRIBUTES_PRIVATE_H */
