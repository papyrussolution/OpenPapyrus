/* large.c - Handles binary manipulation of large numbers */

/*
    libzint - the open source barcode library
    Copyright (C) 2008-2016 Robin Stuart <rstuart114@gmail.com>

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:

    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.
    3. Neither the name of the project nor the names of its contributors
       may be used to endorse or promote products derived from this software
       without specific prior written permission.
 */
#include "common.h"
#pragma hdrstop
//#include "large.h"

static const short BCD[40] = {
	0, 0, 0, 0,
	1, 0, 0, 0,
	0, 1, 0, 0,
	1, 1, 0, 0,
	0, 0, 1, 0,
	1, 0, 1, 0,
	0, 1, 1, 0,
	1, 1, 1, 0,
	0, 0, 0, 1,
	1, 0, 0, 1
};
//
// Binary addition 
//
void FASTCALL binary_add(short accumulator[], short input_buffer[]) 
{
	int    carry = 0;
	for(int i = 0; i < 112; i++) {
		int done = 0;
		if(((input_buffer[i] == 0) && (accumulator[i] == 0)) && ((carry == 0) && !done)) {
			accumulator[i] = 0;
			carry = 0;
			done = 1;
		}
		if(((input_buffer[i] == 0) && (accumulator[i] == 0)) && ((carry == 1) && !done)) {
			accumulator[i] = 1;
			carry = 0;
			done = 1;
		}
		if(((input_buffer[i] == 0) && (accumulator[i] == 1)) && ((carry == 0) && !done)) {
			accumulator[i] = 1;
			carry = 0;
			done = 1;
		}
		if(((input_buffer[i] == 0) && (accumulator[i] == 1)) && ((carry == 1) && !done)) {
			accumulator[i] = 0;
			carry = 1;
			done = 1;
		}
		if(((input_buffer[i] == 1) && (accumulator[i] == 0)) && ((carry == 0) && !done)) {
			accumulator[i] = 1;
			carry = 0;
			done = 1;
		}
		if(((input_buffer[i] == 1) && (accumulator[i] == 0)) && ((carry == 1) && !done)) {
			accumulator[i] = 0;
			carry = 1;
			done = 1;
		}
		if(((input_buffer[i] == 1) && (accumulator[i] == 1)) && ((carry == 0) && !done)) {
			accumulator[i] = 0;
			carry = 1;
			done = 1;
		}
		if(((input_buffer[i] == 1) && (accumulator[i] == 1)) && ((carry == 1) && !done)) {
			accumulator[i] = 1;
			carry = 1;
			done = 1;
		}
	}
}

void binary_subtract(short accumulator[], short input_buffer[])
{
	/* 2's compliment subtraction */
	/* take input_buffer from accumulator and put answer in accumulator */
	int i;
	short sub_buffer[112];
	for(i = 0; i < 112; i++) {
		sub_buffer[i] = (input_buffer[i] == 0) ? 1 : 0;
	}
	binary_add(accumulator, sub_buffer);
	sub_buffer[0] = 1;
	for(i = 1; i < 112; i++) {
		sub_buffer[i] = 0;
	}
	binary_add(accumulator, sub_buffer);
}

void shiftdown(short buffer[])
{
	buffer[102] = 0;
	buffer[103] = 0;
	for(int i = 0; i < 102; i++) {
		buffer[i] = buffer[i+1];
	}
}

void shiftup(short buffer[])
{
	for(int i = 102; i > 0; i--) {
		buffer[i] = buffer[i-1];
	}
	buffer[0] = 0;
}

short islarger(short accum[], short reg[])
{
	// Returns 1 if accum[] is larger than reg[], else 0 
	int latch = 0;
	int i = 103;
	int larger = 0;
	do {
		if((accum[i] == 1) && (reg[i] == 0)) {
			latch = 1;
			larger = 1;
		}
		if((accum[i] == 0) && (reg[i] == 1)) {
			latch = 1;
		}
		i--;
	} while((latch == 0) && (i >= -1));
	return larger;
}

void FASTCALL binary_load(short reg[], const char pData[], const uint srcLen)
{
	short  temp[112] = {0};
	memzero(reg, sizeof(reg[0]) * 112);
	for(uint read = 0; read < srcLen; read++) {
		memcpy(temp, reg, sizeof(temp));
		for(int i = 0; i < 9; i++) {
			binary_add(reg, temp);
		}
		temp[0] = BCD[hex(pData[read]) * 4];
		temp[1] = BCD[(hex(pData[read]) * 4) + 1];
		temp[2] = BCD[(hex(pData[read]) * 4) + 2];
		temp[3] = BCD[(hex(pData[read]) * 4) + 3];
		memzero(temp+4, sizeof(temp) - 4 * sizeof(temp[0]));
		binary_add(reg, temp);
	}
}

void FASTCALL binary_loads(short reg[], const char pData[])
{
	binary_load(reg, pData, sstrlen(pData));
}

