
/*-------------------------------------------------------------*/
/*--- Huffman coding low-level stuff                        ---*/
/*---                                             huffman.c ---*/
/*-------------------------------------------------------------*/

/* ------------------------------------------------------------------
   This file is part of bzip2/libbzip2, a program and library for
   lossless, block-sorting data compression.

   bzip2/libbzip2 version 1.0.6 of 6 September 2010
   Copyright (C) 1996-2010 Julian Seward <jseward@bzip.org>

   Please read the WARNING, DISCLAIMER and PATENTS sections in the
   README file.

   This program is released under the terms of the license contained
   in the file LICENSE.
   ------------------------------------------------------------------ */

#include "bzlib_private.h"

/*---------------------------------------------------*/
#define WEIGHTOF(zz0)  ((zz0) & 0xffffff00)
#define DEPTHOF(zz1)   ((zz1) & 0x000000ff)
#define MYMAX(zz2, zz3) ((zz2) > (zz3) ? (zz2) : (zz3))

#define ADDWEIGHTS(zw1, zw2) (WEIGHTOF(zw1)+WEIGHTOF(zw2)) | (1 + MYMAX(DEPTHOF(zw1), DEPTHOF(zw2)))

#define UPHEAP(z)				      \
	{						      \
		int32 zz = z; \
		int32 tmp = heap[zz];				   \
		while(weight[tmp] < weight[heap[zz >> 1]]) {	  \
			heap[zz] = heap[zz >> 1];			\
			zz >>= 1;					\
		}						   \
		heap[zz] = tmp;					   \
	}

#define DOWNHEAP(z)				      \
	{						      \
		int32 zz = z; \
		int32 tmp = heap[zz];				   \
		while(true) {					  \
			int32 yy = zz << 1;					\
			if(yy > nHeap) break;			       \
			if(yy < nHeap && weight[heap[yy+1]] < weight[heap[yy]])	\
				yy++;					     \
			if(weight[tmp] < weight[heap[yy]]) break;      \
			heap[zz] = heap[yy];				\
			zz = yy;					\
		}						   \
		heap[zz] = tmp;					   \
	}

/*---------------------------------------------------*/
void BZ2_hbMakeCodeLengths(uchar * len, int32 * freq, int32 alphaSize, int32 maxLen)
{
	/*--
	   Nodes and heap entries run from 1.  Entry 0
	   for both the heap and nodes is a sentinel.
	   --*/
	int32 nNodes, nHeap, n1, n2, i, j, k;
	bool tooLong;
	int32 heap   [ BZ_MAX_ALPHA_SIZE + 2 ];
	int32 weight [ BZ_MAX_ALPHA_SIZE * 2 ];
	int32 parent [ BZ_MAX_ALPHA_SIZE * 2 ];
	for(i = 0; i < alphaSize; i++)
		weight[i+1] = (freq[i] == 0 ? 1 : freq[i]) << 8;
	while(true) {
		nNodes = alphaSize;
		nHeap = 0;

		heap[0] = 0;
		weight[0] = 0;
		parent[0] = -2;

		for(i = 1; i <= alphaSize; i++) {
			parent[i] = -1;
			nHeap++;
			heap[nHeap] = i;
			UPHEAP(nHeap);
		}

		AssertH(nHeap < (BZ_MAX_ALPHA_SIZE+2), 2001);

		while(nHeap > 1) {
			n1 = heap[1]; heap[1] = heap[nHeap]; nHeap--; DOWNHEAP(1);
			n2 = heap[1]; heap[1] = heap[nHeap]; nHeap--; DOWNHEAP(1);
			nNodes++;
			parent[n1] = parent[n2] = nNodes;
			weight[nNodes] = ADDWEIGHTS(weight[n1], weight[n2]);
			parent[nNodes] = -1;
			nHeap++;
			heap[nHeap] = nNodes;
			UPHEAP(nHeap);
		}

		AssertH(nNodes < (BZ_MAX_ALPHA_SIZE * 2), 2002);
		tooLong = false;
		for(i = 1; i <= alphaSize; i++) {
			j = 0;
			k = i;
			while(parent[k] >= 0) {
				k = parent[k]; j++;
			}
			len[i-1] = (uchar)j;
			if(j > maxLen) tooLong = true;
		}
		if(!tooLong) 
			break;

		/* 17 Oct 04: keep-going condition for the following loop used
		   to be 'i < alphaSize', which missed the last element,
		   theoretically leading to the possibility of the compressor
		   looping.  However, this count-scaling step is only needed if
		   one of the generated Huffman code words is longer than
		   maxLen, which up to and including version 1.0.2 was 20 bits,
		   which is extremely unlikely.  In version 1.0.3 maxLen was
		   changed to 17 bits, which has minimal effect on compression
		   ratio, but does mean this scaling step is used from time to
		   time, enough to verify that it works.

		   This means that bzip2-1.0.3 and later will only produce
		   Huffman codes with a maximum length of 17 bits.  However, in
		   order to preserve backwards compatibility with bitstreams
		   produced by versions pre-1.0.3, the decompressor must still
		   handle lengths of up to 20. */

		for(i = 1; i <= alphaSize; i++) {
			j = weight[i] >> 8;
			j = 1 + (j / 2);
			weight[i] = j << 8;
		}
	}
}

void BZ2_hbAssignCodes(int32 * code, uchar * length, int32 minLen, int32 maxLen, int32 alphaSize)
{
	int32 vec = 0;
	for(int32 n = minLen; n <= maxLen; n++) {
		for(int32 i = 0; i < alphaSize; i++)
			if(length[i] == n) {
				code[i] = vec; 
				vec++;
			}
		vec <<= 1;
	}
}

void BZ2_hbCreateDecodeTables(int32 * limit, int32 * base, int32 * perm, uchar * length, int32 minLen, int32 maxLen, int32 alphaSize)
{
	int32 i, j, vec;
	int32 pp = 0;
	for(i = minLen; i <= maxLen; i++) {
		for(j = 0; j < alphaSize; j++)
			if(length[j] == i) {
				perm[pp] = j; 
				pp++;
			}
	}
	for(i = 0; i < BZ_MAX_CODE_LEN; i++) 
		base[i] = 0;
	for(i = 0; i < alphaSize; i++) 
		base[length[i]+1]++;
	for(i = 1; i < BZ_MAX_CODE_LEN; i++) 
		base[i] += base[i-1];
	for(i = 0; i < BZ_MAX_CODE_LEN; i++) 
		limit[i] = 0;
	vec = 0;
	for(i = minLen; i <= maxLen; i++) {
		vec += (base[i+1] - base[i]);
		limit[i] = vec-1;
		vec <<= 1;
	}
	for(i = minLen + 1; i <= maxLen; i++)
		base[i] = ((limit[i-1] + 1) << 1) - base[i];
}

