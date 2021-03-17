// GNUPLOT - screenbuf.h 
// Copyright (c) 2011,2016 Bastian Maerkisch. All rights reserved.
//
#ifndef SCREENBUF_H
#define SCREENBUF_H

struct LB {
	LB() : size(0), len(0), P_Str(0), attr(0), def_attr(0)
	{
	}
	uint   size; // actual size of the memory buffer 
	uint   len;  // length of the string 
	LPWSTR P_Str;
	BYTE * attr;
	BYTE   def_attr;
};

struct SB {
	uint   size;
	uint   head;
	uint   tail;
	uint   wrap_at; // wrap lines at this position 
	LB   * lb;
	LB   * current_line;
	uint   last_line;
	uint   last_line_index;
	uint   Length;
};

//typedef SB * LPSB;

/* ------------------------------------ */

void sb_init(SB * sb, uint size);
void sb_resize(SB * sb, uint size);
void sb_free(SB * sb);
LB * sb_get(SB * sb, uint index);
LB * sb_get_last(SB * sb);
int  sb_append(SB * sb, LB * lb);
uint sb_length(const SB * sb);
uint sb_calc_length(SB * sb);
uint sb_lines(const SB * sb, LB * lb);
uint sb_max_line_length(const SB * sb);
void sb_find_new_pos(SB * sb, uint x, uint y, uint new_wrap_at, uint * new_x, uint * new_y);
void sb_wrap(SB * sb, uint wrap_at);
void sb_last_insert_str(SB * sb, uint pos, LPCWSTR s, uint count);

/* ------------------------------------ */

void   lb_init(LB * lb);
uint   lb_length(const LB * lb);
void   lb_insert_char(LB * lb, uint pos, WCHAR ch);
void   lb_insert_str(LB * lb, uint pos, LPCWSTR s, uint count);
LPWSTR lb_substr(LB * lb, uint offset, uint count);
PBYTE  lb_subattr(LB * lb, uint offset, uint count);
void   lb_set_attr(LB * lb, BYTE attr);

#endif
