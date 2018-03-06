/* HuffEnc.c -- functions for Huffman encoding
   2017-04-03 : Igor Pavlov : Public domain */

#include <7z-internal.h>
#pragma hdrstop

#define kMaxLen 16
#define NUM_BITS 10
#define MASK ((1 << NUM_BITS) - 1)
#define NUM_COUNTERS 64
#define HUFFMAN_SPEED_OPT

void Huffman_Generate(const uint32 * freqs, uint32 * p, Byte * lens, uint32 numSymbols, uint32 maxLen)
{
	uint32 num = 0;
	/* if(maxLen > 10) maxLen = 10; */
	{
		uint32 i;
#ifdef HUFFMAN_SPEED_OPT
		uint32 counters[NUM_COUNTERS];
		for(i = 0; i < NUM_COUNTERS; i++)
			counters[i] = 0;
		for(i = 0; i < numSymbols; i++) {
			uint32 freq = freqs[i];
			counters[(freq < NUM_COUNTERS - 1) ? freq : NUM_COUNTERS - 1]++;
		}
		for(i = 1; i < NUM_COUNTERS; i++) {
			uint32 temp = counters[i];
			counters[i] = num;
			num += temp;
		}
		for(i = 0; i < numSymbols; i++) {
			uint32 freq = freqs[i];
			if(freq == 0)
				lens[i] = 0;
			else
				p[counters[((freq < NUM_COUNTERS - 1) ? freq : NUM_COUNTERS - 1)]++] = i | (freq << NUM_BITS);
		}
		counters[0] = 0;
		HeapSort(p + counters[NUM_COUNTERS - 2], counters[NUM_COUNTERS - 1] - counters[NUM_COUNTERS - 2]);
#else
		for(i = 0; i < numSymbols; i++) {
			uint32 freq = freqs[i];
			if(freq == 0)
				lens[i] = 0;
			else
				p[num++] = i | (freq << NUM_BITS);
		}
		HeapSort(p, num);
#endif
	}
	if(num < 2) {
		unsigned minCode = 0;
		unsigned maxCode = 1;
		if(num == 1) {
			maxCode = (uint)p[0] & MASK;
			if(maxCode == 0)
				maxCode++;
		}
		p[minCode] = 0;
		p[maxCode] = 1;
		lens[minCode] = lens[maxCode] = 1;
		return;
	}
	{
		uint32 i = 0;
		uint32 b = 0;
		uint32 e = 0;
		do {
			uint32 m;
			uint32 n = (i != num && (b == e || (p[i] >> NUM_BITS) <= (p[b] >> NUM_BITS))) ? i++ : b++;
			uint32 freq = (p[n] & ~MASK);
			p[n] = (p[n] & MASK) | (e << NUM_BITS);
			m = (i != num && (b == e || (p[i] >> NUM_BITS) <= (p[b] >> NUM_BITS))) ? i++ : b++;
			freq += (p[m] & ~MASK);
			p[m] = (p[m] & MASK) | (e << NUM_BITS);
			p[e] = (p[e] & MASK) | freq;
			e++;
		} while(num - e > 1);
		{
			uint32 lenCounters[kMaxLen + 1];
			for(i = 0; i <= kMaxLen; i++)
				lenCounters[i] = 0;
			p[--e] &= MASK;
			lenCounters[1] = 2;
			while(e > 0) {
				uint32 len = (p[p[--e] >> NUM_BITS] >> NUM_BITS) + 1;
				p[e] = (p[e] & MASK) | (len << NUM_BITS);
				if(len >= maxLen)
					for(len = maxLen - 1; lenCounters[len] == 0; len--) 
						;
				lenCounters[len]--;
				lenCounters[(size_t)len + 1] += 2;
			}
			{
				i = 0;
				for(uint32 len = maxLen; len != 0; len--) {
					for(uint32 k = lenCounters[len]; k != 0; k--)
						lens[p[i++] & MASK] = (Byte)len;
				}
			}

			{
				uint32 nextCodes[kMaxLen + 1];
				{
					uint32 code = 0;
					for(uint32 len = 1; len <= kMaxLen; len++)
						nextCodes[len] = code = (code + lenCounters[(size_t)len - 1]) << 1;
				}
				/* if(code + lenCounters[kMaxLen] - 1 != (1 << kMaxLen) - 1) throw 1; */
				{
					for(uint32 k = 0; k < numSymbols; k++)
						p[k] = nextCodes[lens[k]]++;
				}
			}
		}
	}
}
