// GNUPLOT - screenbuf.h 
// Copyright (c) 2011,2016 Bastian Maerkisch. All rights reserved.
//
#ifndef SCREENBUF_H
#define SCREENBUF_H

typedef struct typLB {
	uint size;      /* actual size of the memory buffer */
	uint len;       /* length of the string */
	LPWSTR str;
	BYTE  * attr;
	BYTE def_attr;
} LB;
typedef LB * LPLB;

typedef struct typSB {
	uint size;
	uint head;
	uint tail;
	uint wrap_at; /* wrap lines at this position */
	LPLB lb;
	LPLB current_line;
	uint last_line;
	uint last_line_index;
	uint length;
} SB;
typedef SB * LPSB;

/* ------------------------------------ */

void sb_init(LPSB sb, uint size);
void sb_resize(LPSB sb, uint size);
void sb_free(LPSB sb);
LPLB sb_get(LPSB sb, uint index);
LPLB sb_get_last(LPSB sb);
int  sb_append(LPSB sb, LPLB lb);
uint sb_length(LPSB sb);
uint sb_calc_length(LPSB sb);
uint sb_lines(LPSB sb, LPLB lb);
uint sb_max_line_length(LPSB sb);
void sb_find_new_pos(LPSB sb, uint x, uint y, uint new_wrap_at, uint * new_x, uint * new_y);
void sb_wrap(LPSB sb, uint wrap_at);
void sb_last_insert_str(LPSB sb, uint pos, LPCWSTR s, uint count);

/* ------------------------------------ */

void lb_init(LPLB lb);
uint lb_length(LPLB lb);
void lb_insert_char(LPLB lb, uint pos, WCHAR ch);
void lb_insert_str(LPLB lb, uint pos, LPCWSTR s, uint count);
LPWSTR lb_substr(LPLB lb, uint offset, uint count);
PBYTE  lb_subattr(LPLB lb, uint offset, uint count);
void lb_set_attr(LPLB lb, BYTE attr);

#endif
