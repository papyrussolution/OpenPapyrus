/* Delta.c -- Delta converter
   2009-05-26 : Igor Pavlov : Public domain */

#include <7z-internal.h>
#pragma hdrstop

void Delta_Init(Byte * state)
{
	for(uint i = 0; i < DELTA_STATE_SIZE; i++)
		state[i] = 0;
}

/*static void MyMemCpy(Byte * dest, const Byte * src, uint size)
{
	for(uint i = 0; i < size; i++)
		dest[i] = src[i];
}*/

void Delta_Encode(Byte * state, unsigned delta, Byte * data, SizeT size)
{
	Byte   buf[DELTA_STATE_SIZE];
	uint   j = 0;
	memcpy(buf, state, delta);
	{
		for(SizeT i = 0; i < size;) {
			for(j = 0; j < delta && i < size; i++, j++) {
				Byte b = data[i];
				data[i] = (Byte)(b - buf[j]);
				buf[j] = b;
			}
		}
	}
	if(j == delta)
		j = 0;
	memcpy(state, buf + j, delta - j);
	memcpy(state + delta - j, buf, j);
}

void Delta_Decode(Byte * state, unsigned delta, Byte * data, SizeT size)
{
	Byte   buf[DELTA_STATE_SIZE];
	uint   j = 0;
	memcpy(buf, state, delta);
	{
		for(SizeT i = 0; i < size;) {
			for(j = 0; j < delta && i < size; i++, j++) {
				buf[j] = data[i] = (Byte)(buf[j] + data[i]);
			}
		}
	}
	if(j == delta)
		j = 0;
	memcpy(state, buf + j, delta - j);
	memcpy(state + delta - j, buf, j);
}

