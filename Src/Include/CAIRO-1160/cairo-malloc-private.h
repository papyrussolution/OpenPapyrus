/* -*- Mode: c; tab-width: 8; c-basic-offset: 4; indent-tabs-mode: t; -*- */
/* Cairo - a vector graphics library with display and print output
 *
 * Copyright © 2007 Mozilla Corporation
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
 * The Initial Developer of the Original Code is Mozilla Foundation
 * Contributor(s): Vladimir Vukicevic <vladimir@pobox.com>
 */
#ifndef CAIRO_MALLOC_PRIVATE_H
#define CAIRO_MALLOC_PRIVATE_H

#if HAVE_MEMFAULT
	#include <memfault.h>
	#define CAIRO_INJECT_FAULT() MEMFAULT_INJECT_FAULT()
#else
	#define CAIRO_INJECT_FAULT() 0
#endif

/**
 * _cairo_malloc:
 * @size: size in bytes
 *
 * Allocate @size memory using malloc().
 * The memory should be freed using SAlloc::F().
 * malloc is skipped, if 0 bytes are requested, and %NULL will be returned.
 *
 * Return value: A pointer to the newly allocated memory, or %NULL in
 * case of malloc() failure or size is 0.
 **/
// @v11.7.11 (replaced with SAlloc::M_zon0) #define _cairo_malloc_Removed(size) ((size) > 0 ? SAlloc::M(static_cast<uint>(size)) : NULL)
/**
 * _cairo_malloc_ab:
 * @a: number of elements to allocate
 * @size: size of each element
 *
 * Allocates @a*@size memory using SAlloc::M_zon0(), taking care to not
 * overflow when doing the multiplication.  Behaves much like
 * calloc(), except that the returned memory is not set to zero.
 * The memory should be freed using SAlloc::F().
 *
 * @size should be a constant so that the compiler can optimize out a constant division.
 *
 * Return value: A pointer to the newly allocated memory, or %NULL in case of malloc() failure or overflow.
 **/
#define _cairo_malloc_ab(a, size) ((size) && static_cast<uint>(a) >= INT32_MAX / static_cast<uint>(size) ? NULL : SAlloc::M_zon0(static_cast<uint>(a) * static_cast<uint>(size)))
/**
 * _cairo_realloc_ab:
 * @ptr: original pointer to block of memory to be resized
 * @a: number of elements to allocate
 * @size: size of each element
 *
 * Reallocates @ptr a block of @a*@size memory using realloc(), taking
 * care to not overflow when doing the multiplication.  The memory
 * should be freed using SAlloc::F().
 *
 * @size should be a constant so that the compiler can optimize
 * out a constant division.
 *
 * Return value: A pointer to the newly allocated memory, or %NULL in
 * case of realloc() failure or overflow (whereupon the original block
 * of memory * is left untouched).
 **/
#define _cairo_realloc_ab(ptr, a, size) ((size) && static_cast<uint>(a) >= INT32_MAX / static_cast<uint>(size) ? NULL : SAlloc::R(ptr, static_cast<uint>(a) * static_cast<uint>(size)))
/**
 * _cairo_malloc_abc:
 * @a: first factor of number of elements to allocate
 * @b: second factor of number of elements to allocate
 * @size: size of each element
 *
 * Allocates @a*@b*@size memory using SAlloc::M_zon0(), taking care to not
 * overflow when doing the multiplication.  Behaves like
 * _cairo_malloc_ab().  The memory should be freed using SAlloc::F().
 *
 * @size should be a constant so that the compiler can optimize
 * out a constant division.
 *
 * Return value: A pointer to the newly allocated memory, or %NULL in
 * case of malloc() failure or overflow.
 **/
#define _cairo_malloc_abc(a, b, size) ((b) && (uint)(a) >= INT32_MAX / (uint)(b) ? NULL : (size) && (uint)((a)*(b)) >= INT32_MAX / (uint)(size) ? NULL : SAlloc::M_zon0((uint)(a) * (uint)(b) * (uint)(size)))
/**
 * _cairo_malloc_ab_plus_c:
 * @a: number of elements to allocate
 * @size: size of each element
 * @c: additional size to allocate
 *
 * Allocates @a*@size+@c memory using SAlloc::M_zon0(), taking care to not
 * overflow when doing the arithmetic.  Behaves similar to
 * _cairo_malloc_ab().  The memory should be freed using SAlloc::F().
 *
 * Return value: A pointer to the newly allocated memory, or %NULL in
 * case of malloc() failure or overflow.
 **/
#define _cairo_malloc_ab_plus_c(a, size, c) ((size) && (uint)(a) >= INT32_MAX / (uint)(size) ? NULL : (uint)(c) >= INT32_MAX - (uint)(a) * (uint)(size) ? NULL : SAlloc::M_zon0((uint)(a) * (uint)(size) + (uint)(c)))

#endif /* CAIRO_MALLOC_PRIVATE_H */
