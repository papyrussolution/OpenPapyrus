/* This code illustrates a sample implementation
 * of the Arcfour algorithm
 * Copyright (c) April 29, 1997 Kalle Kaukonen.
 * All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that this copyright
 * notice and disclaimer are retained.
 */
#include "mupdf/fitz.h"
#pragma hdrstop

void fz_arc4_init(fz_arc4 * arc4, const uchar * key, size_t keylen)
{
	uint t, u;
	size_t keyindex;
	uint stateindex;
	uchar * state;
	uint counter;

	state = arc4->state;

	arc4->x = 0;
	arc4->y = 0;

	for(counter = 0; counter < 256; counter++) {
		state[counter] = counter;
	}

	keyindex = 0;
	stateindex = 0;

	for(counter = 0; counter < 256; counter++) {
		t = state[counter];
		stateindex = (stateindex + key[keyindex] + t) & 0xff;
		u = state[stateindex];

		state[stateindex] = t;
		state[counter] = u;

		if(++keyindex >= keylen) {
			keyindex = 0;
		}
	}
}

static uchar fz_arc4_next(fz_arc4 * arc4)
{
	uint x;
	uint y;
	uint sx, sy;
	uchar * state;

	state = arc4->state;

	x = (arc4->x + 1) & 0xff;
	sx = state[x];
	y = (sx + arc4->y) & 0xff;
	sy = state[y];

	arc4->x = x;
	arc4->y = y;

	state[y] = sx;
	state[x] = sy;

	return state[(sx + sy) & 0xff];
}

void fz_arc4_encrypt(fz_arc4 * arc4, uchar * dest, const uchar * src, size_t len)
{
	for(size_t i = 0; i < len; i++) {
		uchar x = fz_arc4_next(arc4);
		dest[i] = src[i] ^ x;
	}
}

void fz_arc4_final(fz_arc4 * state)
{
	memzero(state, sizeof(*state));
}
