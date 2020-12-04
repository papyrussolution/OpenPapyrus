/****************************************************************************
 * ftbase.c
 *   Single object library component (body only).
 *
 * Copyright (C) 1996-2020 by David Turner, Robert Wilhelm, and Werner Lemberg.
 *
 * This file is part of the FreeType project, and may only be used,
 * modified, and distributed under the terms of the FreeType project
 * license, LICENSE.TXT.  By continuing to use, modify, or distribute
 * this file you indicate that you have read the license and
 * understand and accept it fully.
 */
#define  FT_MAKE_OPTION_SINGLE_OBJECT
#include <ft2build.h>
#pragma hdrstop

#include <freetype/internal/ftdebug.h>
#include <freetype/internal/ftmemory.h>
#include <freetype/internal/ftobjs.h>
#include <freetype/ftlist.h>
#include <freetype/internal/ftcalc.h>
#include <freetype/fttrigon.h>

#include "ftadvanc.c"
#include "ftcalc.c"
#include "ftcolor.c"
#include "ftdbgmem.c"
#include "fterrors.c"
#include "ftfntfmt.c"
#include "ftgloadr.c"
#include "fthash.c"
#include "ftlcdfil.c"
#include "ftmac.c"
#include "ftobjs.c"
#include "ftoutln.c"
#include "ftpsprop.c"
#include "ftrfork.c"
#include "ftsnames.c"
#include "ftstream.c"
//#include "fttrigon.c"
/****************************************************************************
 * fttrigon.c
 *   FreeType trigonometric functions (body).
 *
 * Copyright (C) 2001-2020 by David Turner, Robert Wilhelm, and Werner Lemberg.
 */
// 
// This is a fixed-point CORDIC implementation of trigonometric
// functions as well as transformations between Cartesian and polar
// coordinates.  The angles are represented as 16.16 fixed-point values
// in degrees, i.e., the angular resolution is 2^-16 degrees.  Note that
// only vectors longer than 2^16*180/pi (or at least 22 bits) on a
// discrete Cartesian grid can have the same or better angular
// resolution.  Therefore, to maintain this precision, some functions
// require an interim upscaling of the vectors, whereas others operate
// with 24-bit long vectors directly.
// 
#define FT_TRIG_SCALE      0xDBD95B16UL /* the Cordic shrink factor 0.858785336480436 * 2^32 */
/* the highest bit in overflow-safe vector components, */
/* MSB of 0.858785336480436 * sqrt(0.5) * 2^30         */
#define FT_TRIG_SAFE_MSB   29
#define FT_TRIG_MAX_ITERS  23 /* this table was generated for FT_PI = 180L << 16, i.e. degrees */

static const FT_Angle ft_trig_arctan_table[] = {
	1740967L, 919879L, 466945L, 234379L, 117304L, 58666L, 29335L,
	14668L, 7334L, 3667L, 1833L, 917L, 458L, 229L, 115L, 57L, 29L, 14L, 7L, 4L, 2L, 1L
};

#ifdef FT_LONG64
	/* multiply a given value by the CORDIC shrink factor */
	static FT_Fixed ft_trig_downscale(FT_Fixed val)
	{
		FT_Int s = 1;
		if(val < 0) {
			val = -val;
			s = -1;
		}
		/* 0x40000000 comes from regression analysis between true */
		/* and CORDIC hypotenuse, so it minimizes the error       */
		val = (FT_Fixed)(( (FT_UInt64)val * FT_TRIG_SCALE + 0x40000000UL ) >> 32 );
		return s < 0 ? -val : val;
	}
#else /* !FT_LONG64 */
	/* multiply a given value by the CORDIC shrink factor */
	static FT_Fixed ft_trig_downscale(FT_Fixed val)
	{
		FT_Int s = 1;
		FT_UInt32 lo1, hi1, lo2, hi2, lo, hi, i1, i2;
		if(val < 0) {
			val = -val;
			s = -1;
		}
		lo1 = (FT_UInt32)val & 0x0000FFFFU;
		hi1 = (FT_UInt32)val >> 16;
		lo2 = FT_TRIG_SCALE & 0x0000FFFFU;
		hi2 = FT_TRIG_SCALE >> 16;
		lo = lo1 * lo2;
		i1 = lo1 * hi2;
		i2 = lo2 * hi1;
		hi = hi1 * hi2;
		/* Check carry overflow of i1 + i2 */
		i1 += i2;
		hi += (FT_UInt32)( i1 < i2 ) << 16;

		hi += i1 >> 16;
		i1  = i1 << 16;
		/* Check carry overflow of i1 + lo */
		lo += i1;
		hi += ( lo < i1 );

		/* 0x40000000 comes from regression analysis between true */
		/* and CORDIC hypotenuse, so it minimizes the error       */

		/* Check carry overflow of lo + 0x40000000 */
		lo += 0x40000000UL;
		hi += ( lo < 0x40000000UL );
		val = (FT_Fixed)hi;
		return s < 0 ? -val : val;
	}
#endif /* !FT_LONG64 */

/* undefined and never called for zero vector */
static FT_Int ft_trig_prenorm(FT_Vector*  vec)
{
	FT_Pos x = vec->x;
	FT_Pos y = vec->y;
	FT_Int shift = FT_MSB( (FT_UInt32)( FT_ABS(x) | FT_ABS(y) ) );
	if(shift <= FT_TRIG_SAFE_MSB) {
		shift  = FT_TRIG_SAFE_MSB - shift;
		vec->x = (FT_Pos)( (FT_ULong)x << shift );
		vec->y = (FT_Pos)( (FT_ULong)y << shift );
	}
	else{
		shift -= FT_TRIG_SAFE_MSB;
		vec->x = x >> shift;
		vec->y = y >> shift;
		shift  = -shift;
	}
	return shift;
}

static void ft_trig_pseudo_rotate(FT_Vector*  vec, FT_Angle theta)
{
	FT_Int i;
	FT_Fixed xtemp, b;
	const FT_Angle * arctanptr;
	FT_Fixed x = vec->x;
	FT_Fixed y = vec->y;
	/* Rotate inside [-PI/4,PI/4] sector */
	while(theta < -FT_ANGLE_PI4) {
		xtemp  =  y;
		y      = -x;
		x      =  xtemp;
		theta +=  FT_ANGLE_PI2;
	}
	while(theta > FT_ANGLE_PI4) {
		xtemp  = -y;
		y      =  x;
		x      =  xtemp;
		theta -=  FT_ANGLE_PI2;
	}
	arctanptr = ft_trig_arctan_table;
	/* Pseudorotations, with right shifts */
	for(i = 1, b = 1; i < FT_TRIG_MAX_ITERS; b <<= 1, i++) {
		if(theta < 0) {
			xtemp  = x + ( ( y + b ) >> i );
			y      = y - ( ( x + b ) >> i );
			x      = xtemp;
			theta += *arctanptr++;
		}
		else{
			xtemp  = x - ( ( y + b ) >> i );
			y      = y + ( ( x + b ) >> i );
			x      = xtemp;
			theta -= *arctanptr++;
		}
	}
	vec->x = x;
	vec->y = y;
}

static void ft_trig_pseudo_polarize(FT_Vector*  vec)
{
	FT_Angle theta;
	FT_Int i;
	FT_Fixed xtemp, b;
	const FT_Angle  * arctanptr;
	FT_Fixed x = vec->x;
	FT_Fixed y = vec->y;
	/* Get the vector into [-PI/4,PI/4] sector */
	if(y > x) {
		if(y > -x) {
			theta =  FT_ANGLE_PI2;
			xtemp =  y;
			y     = -x;
			x     =  xtemp;
		}
		else{
			theta =  y > 0 ? FT_ANGLE_PI : -FT_ANGLE_PI;
			x     = -x;
			y     = -y;
		}
	}
	else{
		if(y < -x) {
			theta = -FT_ANGLE_PI2;
			xtemp = -y;
			y     =  x;
			x     =  xtemp;
		}
		else{
			theta = 0;
		}
	}
	arctanptr = ft_trig_arctan_table;
	/* Pseudorotations, with right shifts */
	for(i = 1, b = 1; i < FT_TRIG_MAX_ITERS; b <<= 1, i++) {
		if(y > 0) {
			xtemp  = x + ( ( y + b ) >> i );
			y      = y - ( ( x + b ) >> i );
			x      = xtemp;
			theta += *arctanptr++;
		}
		else{
			xtemp  = x - ( ( y + b ) >> i );
			y      = y + ( ( x + b ) >> i );
			x      = xtemp;
			theta -= *arctanptr++;
		}
	}
	/* round theta to acknowledge its error that mostly comes */
	/* from accumulated rounding errors in the arctan table   */
	if(theta >= 0)
		theta = FT_PAD_ROUND(theta, 16);
	else
		theta = -FT_PAD_ROUND(-theta, 16);
	vec->x = x;
	vec->y = theta;
}

/* documentation is in fttrigon.h */
FT_EXPORT_DEF(FT_Fixed) FT_Cos(FT_Angle angle)
{
	FT_Vector v;
	FT_Vector_Unit(&v, angle);
	return v.x;
}

/* documentation is in fttrigon.h */
FT_EXPORT_DEF(FT_Fixed) FT_Sin(FT_Angle angle)
{
	FT_Vector v;
	FT_Vector_Unit(&v, angle);
	return v.y;
}

/* documentation is in fttrigon.h */
FT_EXPORT_DEF(FT_Fixed) FT_Tan(FT_Angle angle)
{
	FT_Vector v = { 1 << 24, 0 };
	ft_trig_pseudo_rotate(&v, angle);
	return FT_DivFix(v.y, v.x);
}

/* documentation is in fttrigon.h */
FT_EXPORT_DEF(FT_Angle) FT_Atan2(FT_Fixed dx, FT_Fixed dy)
{
	FT_Vector v;
	if(dx == 0 && dy == 0)
		return 0;
	v.x = dx;
	v.y = dy;
	ft_trig_prenorm(&v);
	ft_trig_pseudo_polarize(&v);
	return v.y;
}

/* documentation is in fttrigon.h */
FT_EXPORT_DEF(void) FT_Vector_Unit(FT_Vector*  vec, FT_Angle angle)
{
	if(vec) {
		vec->x = FT_TRIG_SCALE >> 8;
		vec->y = 0;
		ft_trig_pseudo_rotate(vec, angle);
		vec->x = ( vec->x + 0x80L ) >> 8;
		vec->y = ( vec->y + 0x80L ) >> 8;
	}
}

/* documentation is in fttrigon.h */
FT_EXPORT_DEF(void) FT_Vector_Rotate(FT_Vector*  vec, FT_Angle angle)
{
	FT_Int shift;
	FT_Vector v;
	if(!vec || !angle)
		return;
	v = *vec;
	if(v.x == 0 && v.y == 0)
		return;
	shift = ft_trig_prenorm(&v);
	ft_trig_pseudo_rotate(&v, angle);
	v.x = ft_trig_downscale(v.x);
	v.y = ft_trig_downscale(v.y);
	if(shift > 0) {
		FT_Int32 half = (FT_Int32)1L << ( shift - 1 );
		vec->x = ( v.x + half - ( v.x < 0 ) ) >> shift;
		vec->y = ( v.y + half - ( v.y < 0 ) ) >> shift;
	}
	else{
		shift  = -shift;
		vec->x = (FT_Pos)( (FT_ULong)v.x << shift );
		vec->y = (FT_Pos)( (FT_ULong)v.y << shift );
	}
}

/* documentation is in fttrigon.h */
FT_EXPORT_DEF(FT_Fixed) FT_Vector_Length(FT_Vector*  vec)
{
	FT_Int shift;
	FT_Vector v;
	if(!vec)
		return 0;
	v = *vec;
	/* handle trivial cases */
	if(v.x == 0) {
		return FT_ABS(v.y);
	}
	else if(v.y == 0) {
		return FT_ABS(v.x);
	}
	/* general case */
	shift = ft_trig_prenorm(&v);
	ft_trig_pseudo_polarize(&v);
	v.x = ft_trig_downscale(v.x);
	if(shift > 0)
		return ( v.x + ( 1L << ( shift - 1 ) ) ) >> shift;
	return (FT_Fixed)( (FT_UInt32)v.x << -shift );
}

/* documentation is in fttrigon.h */
FT_EXPORT_DEF(void) FT_Vector_Polarize(FT_Vector*  vec, FT_Fixed   *length, FT_Angle   *angle)
{
	FT_Int shift;
	FT_Vector v;
	if(!vec || !length || !angle)
		return;
	v = *vec;
	if(v.x == 0 && v.y == 0)
		return;
	shift = ft_trig_prenorm(&v);
	ft_trig_pseudo_polarize(&v);
	v.x = ft_trig_downscale(v.x);
	*length = shift >= 0 ? ( v.x >>  shift ) : (FT_Fixed)( (FT_UInt32)v.x << -shift );
	*angle  = v.y;
}

/* documentation is in fttrigon.h */
FT_EXPORT_DEF(void) FT_Vector_From_Polar(FT_Vector*  vec, FT_Fixed length, FT_Angle angle)
{
	if(vec) {
		vec->x = length;
		vec->y = 0;
		FT_Vector_Rotate(vec, angle);
	}
}

/* documentation is in fttrigon.h */
FT_EXPORT_DEF(FT_Angle) FT_Angle_Diff(FT_Angle angle1, FT_Angle angle2)
{
	FT_Angle delta = angle2 - angle1;
	while(delta <= -FT_ANGLE_PI)
		delta += FT_ANGLE_2PI;
	while(delta > FT_ANGLE_PI)
		delta -= FT_ANGLE_2PI;
	return delta;
}
//
//#include "ftutil.c"
/****************************************************************************
 * ftutil.c
 *   FreeType utility file for memory and list management (body).
 *
 * Copyright (C) 2002-2020 by David Turner, Robert Wilhelm, and Werner Lemberg.
 */
// 
// The macro FT_COMPONENT is used in trace mode.  It is an implicit
// parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log
// messages during execution.
// 
#undef  FT_COMPONENT
#define FT_COMPONENT  memory
// 
// M E M O R Y   M A N A G E M E N T
// 
FT_BASE_DEF(FT_Pointer) ft_mem_alloc(FT_Memory memory, FT_Long size, FT_Error  *p_error)
{
	FT_Error error;
	FT_Pointer block = ft_mem_qalloc(memory, size, &error);
	if(!error && block && size > 0)
		memzero(block, size);
	*p_error = error;
	return block;
}

FT_BASE_DEF(FT_Pointer) ft_mem_qalloc(FT_Memory memory, FT_Long size, FT_Error  *p_error)
{
	FT_Error error = FT_Err_Ok;
	FT_Pointer block = NULL;
	if(size > 0) {
		block = memory->alloc(memory, size);
		if(!block)
			error = FT_THROW(Out_Of_Memory);
	}
	else if(size < 0) {
		/* may help catch/prevent security issues */
		error = FT_THROW(Invalid_Argument);
	}
	*p_error = error;
	return block;
}

FT_BASE_DEF(FT_Pointer) ft_mem_realloc(FT_Memory memory, FT_Long item_size, FT_Long cur_count, FT_Long new_count, void * block, FT_Error  *p_error)
{
	FT_Error error = FT_Err_Ok;
	block = ft_mem_qrealloc(memory, item_size, cur_count, new_count, block, &error);
	if(!error && block && new_count > cur_count)
		memzero( (char*)block + cur_count * item_size, ( new_count - cur_count ) * item_size);
	*p_error = error;
	return block;
}

FT_BASE_DEF(FT_Pointer) ft_mem_qrealloc(FT_Memory memory, FT_Long item_size, FT_Long cur_count,
    FT_Long new_count, void*      block, FT_Error  *p_error)
{
	FT_Error error = FT_Err_Ok;
	/* Note that we now accept `item_size == 0' as a valid parameter, in
	 * order to cover very weird cases where an ALLOC_MULT macro would be
	 * called.
	 */
	if(cur_count < 0 || new_count < 0 || item_size < 0) {
		/* may help catch/prevent nasty security issues */
		error = FT_THROW(Invalid_Argument);
	}
	else if(new_count == 0 || item_size == 0) {
		ft_mem_free(memory, block);
		block = NULL;
	}
	else if(new_count > FT_INT_MAX / item_size) {
		error = FT_THROW(Array_Too_Large);
	}
	else if(cur_count == 0) {
		FT_ASSERT(!block);
		block = memory->alloc(memory, new_count * item_size);
		if(block == NULL)
			error = FT_THROW(Out_Of_Memory);
	}
	else{
		FT_Pointer block2;
		FT_Long cur_size = cur_count * item_size;
		FT_Long new_size = new_count * item_size;
		block2 = memory->realloc(memory, cur_size, new_size, block);
		if(!block2)
			error = FT_THROW(Out_Of_Memory);
		else
			block = block2;
	}
	*p_error = error;
	return block;
}

FT_BASE_DEF(void) ft_mem_free(FT_Memory memory, const void * P)
{
	if(P)
		memory->free(memory, (void*)P);
}

FT_BASE_DEF(FT_Pointer) ft_mem_dup(FT_Memory memory, const void*  address, FT_ULong size, FT_Error    *p_error)
{
	FT_Error error;
	FT_Pointer p = ft_mem_qalloc(memory, (FT_Long)size, &error);
	if(!error && address && size > 0)
		ft_memcpy(p, address, size);
	*p_error = error;
	return p;
}

FT_BASE_DEF(FT_Pointer) ft_mem_strdup(FT_Memory memory, const char*  str, FT_Error    *p_error)
{
	FT_ULong len = str ? (FT_ULong)ft_strlen(str) + 1 : 0;
	return ft_mem_dup(memory, str, len, p_error);
}

FT_BASE_DEF(FT_Int) ft_mem_strcpyn(char * dst, const char*  src, FT_ULong size)
{
	while(size > 1 && *src != 0) {
		*dst++ = *src++;
		size--;
	}
	*dst = 0; /* always zero-terminate */
	return *src != 0;
}

/*************************************************************************/
/*****            D O U B L Y   L I N K E D   L I S T S              *****/
/*************************************************************************/

#undef  FT_COMPONENT
#define FT_COMPONENT  list

/* documentation is in ftlist.h */
FT_EXPORT_DEF(FT_ListNode) FT_List_Find(FT_List list, void*    data)
{
	if(list) {
		FT_ListNode cur = list->head;
		while(cur) {
			if(cur->data == data)
				return cur;
			cur = cur->next;
		}
	}
	return NULL;
}

/* documentation is in ftlist.h */
FT_EXPORT_DEF(void) FT_List_Add(FT_List list, FT_ListNode node)
{
	if(list && node) {
		FT_ListNode before = list->tail;
		node->next = NULL;
		node->prev = before;
		if(before)
			before->next = node;
		else
			list->head = node;
		list->tail = node;
	}
}

/* documentation is in ftlist.h */
FT_EXPORT_DEF(void) FT_List_Insert(FT_List list, FT_ListNode node)
{
	if(list && node) {
		FT_ListNode after = list->head;
		node->next = after;
		node->prev = NULL;
		if(!after)
			list->tail = node;
		else
			after->prev = node;
		list->head = node;
	}
}

/* documentation is in ftlist.h */
FT_EXPORT_DEF(void) FT_List_Remove(FT_List list, FT_ListNode node)
{
	if(list && node) {
		FT_ListNode before = node->prev;
		FT_ListNode after  = node->next;
		if(before)
			before->next = after;
		else
			list->head = after;
		if(after)
			after->prev = before;
		else
			list->tail = before;
	}
}

/* documentation is in ftlist.h */
FT_EXPORT_DEF(void) FT_List_Up(FT_List list, FT_ListNode node)
{
	if(list && node) {
		FT_ListNode before = node->prev;
		FT_ListNode after  = node->next;
		/* check whether we are already on top of the list */
		if(before) {
			before->next = after;
			if(after)
				after->prev = before;
			else
				list->tail = before;
			node->prev       = NULL;
			node->next       = list->head;
			list->head->prev = node;
			list->head       = node;
		}
	}
}

/* documentation is in ftlist.h */
FT_EXPORT_DEF(FT_Error) FT_List_Iterate(FT_List list, FT_List_Iterator iterator, void * user)
{
	FT_ListNode cur;
	FT_Error error = FT_Err_Ok;
	if(!list || !iterator)
		return FT_THROW(Invalid_Argument);
	cur = list->head;
	while(cur) {
		FT_ListNode next = cur->next;
		error = iterator(cur, user);
		if(error)
			break;
		cur = next;
	}
	return error;
}

/* documentation is in ftlist.h */
FT_EXPORT_DEF(void) FT_List_Finalize(FT_List list, FT_List_Destructor destroy, FT_Memory memory, void * user)
{
	if(list && memory) {
		FT_ListNode cur = list->head;
		while(cur) {
			FT_ListNode next = cur->next;
			void * data = cur->data;
			if(destroy)
				destroy(memory, data, user);
			FT_FREE(cur);
			cur = next;
		}
		list->head = NULL;
		list->tail = NULL;
	}
}
//
