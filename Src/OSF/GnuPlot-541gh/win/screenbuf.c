// GNUPLOT - screenbuf.c
// Copyright (c) 2011,2016 Bastian Maerkisch. All rights reserved.
//
// Data structure and methods to implement a screen buffer.
// Implementation as a circular buffer.
// (NB: sb->head == sb->tail means NO element stored)
//
#include <gnuplot.h>
#pragma hdrstop
#ifdef _WIN32
	//#include <windows.h>
#else
typedef uchar BYTE;
#endif
#include "screenbuf.h"
#include "winmain.h"

static uint sb_internal_length(const SB * sb);
static LB * sb_internal_get(const SB * sb, uint index);
static void lb_free(LB * lb);
static void lb_copy(LB * dest, LB * src);
// 
// line buffer functions
// 
// 
// lb_init:
// initialize a line buffer, mark as free
// 
void lb_init(LB * lb)
{
	assert(lb);
	lb->P_Str = NULL;
	lb->attr = NULL;
	lb->size = 0;
	lb->len = 0;
}
// 
// lb_free:
// free members of a line buffer, mark as free
// 
static void lb_free(LB * lb)
{
	assert(lb);
	SAlloc::F(lb->P_Str);
	SAlloc::F(lb->attr);
	lb_init(lb);
}
//
// copy a line buffer from <src> to <dest>
//
static void lb_copy(LB * dest, LB * src)
{
	assert(dest);
	assert(src);
	dest->P_Str = src->P_Str;
	dest->attr = src->attr;
	dest->size = src->size;
	dest->len = src->len;
	dest->def_attr = src->def_attr;
}

uint lb_length(const LB * lb)
{
	assert(lb);
	return lb->len;
}
// 
// lb_insert_char:
// insert a character <ch> at position <pos> into the line buffer,
// increase the size of the line buffer if necessary,
// fill gaps with spaces
// 
void lb_insert_char(LB * lb, uint pos, WCHAR ch)
{
	lb_insert_str(lb, pos, &ch, 1);
}
// 
// lb_insert_str:
// (actually this is a misnomer as it overwrites any previous text)
// insert a string <s> at position <pos> into the line buffer,
// increase the size of the line buffer if necessary,
// fill gaps with spaces
// 
void lb_insert_str(LB * lb, uint pos, LPCWSTR s, uint count)
{
	assert(lb);
	// enlarge string buffer if necessary 
	if(lb->size <= (pos + count)) {
		uint newsize = (((pos + count + 8) / 8) * 8 + 32);
		LPWSTR newstr = (LPWSTR)SAlloc::R(lb->P_Str, newsize * sizeof(WCHAR));
		PBYTE newattr = (PBYTE)SAlloc::R(lb->attr, newsize * sizeof(BYTE));
		if(newstr && newattr) {
			lb->P_Str = newstr;
			lb->attr = newattr;
			lb->size = newsize;
		}
		else {
			// memory allocation failed 
			if(pos < lb->size)
				return;
			else
				count = lb->size - pos - 1;
		}
	}
	// fill up with spaces 
	if(pos > lb->len) {
		wmemset(lb->P_Str + lb->len, L' ', pos - lb->len);
		memset(lb->attr + lb->len, lb->def_attr, pos - lb->len);
	}
	// copy characters 
	wmemcpy(lb->P_Str + pos, s, count);
	memset(lb->attr + pos, lb->def_attr, count);
	lb->len = MAX(pos + count, lb->len);
	lb->P_Str[lb->len] = 0;
	lb->attr[lb->len] = 0;
}
// 
// lb_substr:
// get a substring from the line buffer,
// this string has to bee free'd afterwards!
//
LPWSTR lb_substr(LB * lb, uint offset, uint count)
{
	uint len = lb ? lb->len : 0;
	// allocate return string 
	LPWSTR retval = (LPWSTR)SAlloc::M((count + 1) * sizeof(WCHAR));
	if(retval) {
		if(offset >= len) {
			wmemset(retval, L' ', count);
		}
		else {
			if(len >= (count + offset)) {
				wmemcpy(retval, lb->P_Str + offset, count);
			}
			else {
				wmemcpy(retval, lb->P_Str + offset, len - offset);
				wmemset(retval + len - offset, L' ', count + offset - len);
			}
		}
		retval[count] = 0;
	}
	return retval;
}

/*  lb_subattr:
 *  get a sub-range of attribute from the line buffer,
 *  this result has to bee free'd afterwards!
 */
PBYTE lb_subattr(LB * lb, uint offset, uint count)
{
	uint len = lb ? lb->len : 0;
	// allocate return string 
	PBYTE retval = (PBYTE)SAlloc::M((count + 1) * sizeof(BYTE));
	if(retval == NULL)
		return NULL;
	if(offset >= len) {
		memset(retval, lb->def_attr, count);
	}
	else {
		if(len >= (count + offset)) {
			memcpy(retval, lb->attr + offset, count);
		}
		else {
			memcpy(retval, lb->attr + offset, len - offset);
			memset(retval + len - offset, lb->def_attr, count + offset - len);
		}
	}
	retval[count] = '\0';
	return retval;
}

void lb_set_attr(LB * lb, BYTE attr)
{
	lb->def_attr = attr;
}
// 
// screen buffer functions
// 
// 
// sb_init:
// initialize a screen buffer with <size> line buffer elements
// 
void sb_init(SB * sb, uint size)
{
	assert(sb);
	sb->head = sb->tail = 0;
	sb->wrap_at = 0;
	sb->lb = (LB *)SAlloc::C(size + 1, sizeof(LB));
	sb->size = sb->lb ? (size + 1) : 0;
	sb->current_line = (LB *)SAlloc::M(sizeof(LB));
	sb->Length = 0;
	sb->last_line = 0;
	sb->last_line_index = 0;
}
// 
// sb_free:
// free all line buffers of a screen buffer
// 
void sb_free(SB * sb)
{
	uint idx;
	assert(sb);
	assert(sb->lb);
	// free all line buffers 
	uint len = sb_internal_length(sb);
	for(idx = 0; idx < len; idx++)
		lb_free(&(sb->lb[idx]));
	ZFREE(sb->lb);
	sb->head = sb->tail = 0;
	sb->size = 0;
}
// 
// sb_internal_get:
// retrieve line buffer according to index
// 
LB * sb_internal_get(const SB * sb, uint index)
{
	LB * line = NULL;
	assert(sb);
	assert(index < sb->size);
	assert(sb->lb);
	if(index < sb_internal_length(sb))
		line = &(sb->lb[(sb->head + index) % sb->size]);
	return line;
}
//
// sb_get:
// retrieve (wrapped) line buffer
//
LB * sb_get(SB * sb, uint index)
{
	LB * line = NULL;
	assert(sb); assert((index < sb->size) || (sb->wrap_at > 0));
	assert(sb->lb);
	if(sb->wrap_at == 0) {
		if(index < sb_internal_length(sb))
			line = &(sb->lb[(sb->head + index) % sb->size]);
	}
	else {
		// count number of wrapped lines 
		uint line_count;
		uint idx;
		uint internal_length = sb_internal_length(sb);
		if(sb->last_line <= index) {
			// use cached values 
			line_count = sb->last_line;
			idx = sb->last_line_index;
		}
		else {
			line_count = 0;
			idx = 0;
		}
		for(; (idx < internal_length); idx++) {
			line_count += sb_lines(sb, sb_internal_get(sb, idx));
			if(line_count > index) 
				break;
		}
		if(idx < internal_length) {
			uint start, count;
			// get last line buffer 
			LB * lb = sb_internal_get(sb, idx);
			uint len = lb_length(lb);
			uint wrapped_lines = sb_lines(sb, lb);
			line_count -= wrapped_lines;
			// cache current index 
			sb->last_line_index = idx;
			sb->last_line = line_count;
			// index into current line buffer 
			start = (index - line_count) * sb->wrap_at;
			count = MIN(len - start, sb->wrap_at);
			// copy substring from current line buffer 
			lb_init(sb->current_line);
			if(lb->P_Str) {
				sb->current_line->len = count;
				sb->current_line->P_Str = lb->P_Str + start;
				sb->current_line->attr = lb->attr + start;
				//lb_insert_str(sb->current_line, 0, lb->str + start, count);
			}
			line = sb->current_line; // return temporary buffer 
		}
	}
	return line;
}

/*  sb_get_last:
 *  retrieve last line buffer
 */
LB * sb_get_last(SB * sb)
{
	assert(sb);
	uint last = sb_internal_length(sb) - 1;
	return sb_internal_get(sb, last);
}
// 
// sb_append:
// append a line buffer at the end of the screen buffer,
// if the screen buffer is full discard the first line;
// the line is _copied_ to the screen buffer
// 
int sb_append(SB * sb, LB * lb)
{
	uint idx;
	int y_correction = 0;
	assert(sb);
	assert(lb);
	idx = sb->tail;
	sb->tail = (sb->tail + 1) % sb->size;
	if(sb->tail == sb->head) {
		y_correction = sb_lines(sb, &(sb->lb[sb->head]));
		lb_free(&(sb->lb[sb->head]));
		sb->head = (sb->head + 1) % sb->size;
	}
	lb_copy(&(sb->lb[idx]), lb);
	sb->Length += sb_lines(sb, lb) - y_correction;
	return y_correction;
}
// 
// sb_internal_length:
// return number of entries (line buffers) of the screen buffer
// 
uint sb_internal_length(const SB * sb)
{
	assert(sb);
	uint lines = (sb->head <= sb->tail) ? (sb->tail - sb->head) : (sb->size - 1);
	return lines;
}
// 
// sb_length:
// get the current number of lines in the screen buffer
// 
uint sb_length(const SB * sb)
{
	return sb->Length;
}
// 
// sb_length:
// get the current number of lines in the screen buffer
// 
uint sb_calc_length(SB * sb)
{
	int lines = 0;
	assert(sb);
	if(sb->wrap_at == 0) {
		lines = sb_internal_length(sb);
	}
	else {
		// count number of wrapped lines 
		for(uint idx = 0; idx < sb_internal_length(sb); idx++)
			lines += sb_lines(sb, sb_internal_get(sb, idx));
	}
	return lines;
}

/*  sb_resize:
 *  change the maximum number of lines in the screen buffer to <size>
 *  discard lines at the top if necessary
 */
void sb_resize(SB * sb, uint size)
{
	uint sidx, idx, count;
	uint len;
	// allocate new buffer 
	LB * lb = (LB *)SAlloc::C(size + 1, sizeof(LB));
	if(lb) {
		len = sb_internal_length(sb);
		sidx = (size > len) ? 0 : (len - size);
		count = (size > len) ? len : size;
		// free elements if necessary 
		for(idx = 0; idx < sidx; idx++)
			lb_free(sb_internal_get(sb, idx));
		// copy elements to new buffer 
		for(idx = 0; idx < count; idx++, sidx++)
			lb_copy(&(lb[idx]), sb_internal_get(sb, sidx));
		// replace old buffer by new one 
		FREEANDASSIGN(sb->lb, lb);
		sb->size = size + 1;
		sb->head = 0;
		sb->tail = count;
	}
}
//
//  sb_lines:
//  return the number of (wrapped) text lines
//
uint sb_lines(const SB * sb, LB * lb)
{
	if(sb->wrap_at != 0)
		return (lb_length(lb) + sb->wrap_at) / sb->wrap_at;
	else
		return 1;
}
//
//  sb_max_line_length:
//  determine maximum length of a single text line
//
uint sb_max_line_length(const SB * sb)
{
	uint len = 0;
	uint count = sb_internal_length(sb);
	for(uint idx = 0; idx < count; idx++)
		len = MAX(lb_length(sb_internal_get(sb, idx)), len);
	if(sb->wrap_at && len > sb->wrap_at)
		len = sb->wrap_at;
	return len;
}
// 
// sb_find_new_pos:
// determine new x,y position after a change of the wrapping position
//
#if 0 // 
void sb_find_new_pos(SB * sb, uint x, uint y, uint new_wrap_at, uint * new_x, uint * new_y)
{
	uint line_count = 0;
	uint idx = 0;
	// determine index of corresponding internal line 
	uint internal_length = sb_internal_length(sb);
	for(; idx < internal_length; idx++) {
		uint lines = sb_lines(sb, sb_internal_get(sb, idx));
		if(line_count + lines > y) 
			break;
		line_count += lines;
	}
	if(line_count == 0) {
		*new_x = *new_y = 0;
	}
	else {
		// calculate x offset within this line 
		uint xofs = x + (y - line_count) * sb->wrap_at;
		if(new_wrap_at) {
			// temporarily switch wrapping 
			const uint old_wrap_at = sb->wrap_at;
			sb->wrap_at = new_wrap_at;
			// count lines with new wrapping 
			for(uint i = line_count = 0; i < idx; i++)
				line_count += sb_lines(sb, sb_internal_get(sb, i));
			// determine new position 
			*new_x = xofs % new_wrap_at;
			*new_y = line_count + (xofs / new_wrap_at);
			// switch wrapping back 
			sb->wrap_at = old_wrap_at;
		}
		else {
			*new_x = xofs;
			*new_y = idx;
		}
	}
}
#endif // } 0

SPoint2I sb_find_new_pos2(SB * sb, uint x, uint y, uint new_wrap_at)
{
	SPoint2I result;
	uint line_count = 0;
	uint idx = 0;
	// determine index of corresponding internal line 
	uint internal_length = sb_internal_length(sb);
	for(; idx < internal_length; idx++) {
		uint lines = sb_lines(sb, sb_internal_get(sb, idx));
		if(line_count + lines > y) 
			break;
		line_count += lines;
	}
	if(line_count) {
		// calculate x offset within this line 
		uint xofs = x + (y - line_count) * sb->wrap_at;
		if(new_wrap_at) {
			// temporarily switch wrapping 
			const uint old_wrap_at = sb->wrap_at;
			sb->wrap_at = new_wrap_at;
			// count lines with new wrapping 
			for(uint i = line_count = 0; i < idx; i++)
				line_count += sb_lines(sb, sb_internal_get(sb, i));
			result.Set(xofs % new_wrap_at, line_count + (xofs / new_wrap_at)); // determine new position 
			sb->wrap_at = old_wrap_at; // switch wrapping back 
		}
		else
			result.Set(xofs, idx);
	}
	return result;
}

void sb_wrap(SB * sb, uint wrap_at)
{
	sb->wrap_at = wrap_at;
	// invalidate line cache 
	sb->last_line = 0;
	sb->last_line_index = 0;
	sb->Length = sb_calc_length(sb); // update length cache 
}
//
// sb_last_insert_str:
// call lb_insert str for the last line,
// adjust total number of lines accordingly
//
void sb_last_insert_str(SB * sb, uint pos, LPCWSTR s, uint count)
{
	LB * lb = sb_get_last(sb);
	uint len = sb_lines(sb, lb);
	lb_insert_str(lb, pos, s, count);
	// check if total length of sb has changed 
	sb->Length += sb_lines(sb, lb) - len;
}
