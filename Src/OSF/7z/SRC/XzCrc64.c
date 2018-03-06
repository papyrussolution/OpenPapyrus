/* XzCrc64.c -- CRC64 calculation
   2017-04-03 : Igor Pavlov : Public domain */

#include <7z-internal.h>
#pragma hdrstop

#define kCrc64Poly UINT64_CONST(0xC96C5795D7870F42)

#ifdef MY_CPU_LE
  #define CRC_NUM_TABLES 4
#else
  #define CRC_NUM_TABLES 5
  #define CRC_UINT64_SWAP(v) \
	((v >> 56) \
	    | ((v >> 40) & ((uint64)0xFF <<  8)) \
	    | ((v >> 24) & ((uint64)0xFF << 16)) \
	    | ((v >>  8) & ((uint64)0xFF << 24)) \
	    | ((v <<  8) & ((uint64)0xFF << 32)) \
	    | ((v << 24) & ((uint64)0xFF << 40)) \
	    | ((v << 40) & ((uint64)0xFF << 48)) \
	    | ((v << 56)))

uint64 FASTCALL XzCrc64UpdateT1_BeT4(uint64 v, const void * data, size_t size, const uint64 * table);
#endif

#ifndef MY_CPU_BE
uint64 FASTCALL XzCrc64UpdateT4(uint64 v, const void * data, size_t size, const uint64 * table);
#endif

typedef uint64 (FASTCALL *CRC_FUNC)(uint64 v, const void * data, size_t size, const uint64 * table);

static CRC_FUNC g_Crc64Update;
uint64 g_Crc64Table[256 * CRC_NUM_TABLES];

uint64 FASTCALL Crc64Update(uint64 v, const void * data, size_t size)
{
	return g_Crc64Update(v, data, size, g_Crc64Table);
}

uint64 FASTCALL Crc64Calc(const void * data, size_t size)
{
	return g_Crc64Update(CRC64_INIT_VAL, data, size, g_Crc64Table) ^ CRC64_INIT_VAL;
}

void FASTCALL Crc64GenerateTable()
{
	uint32 i;
	for(i = 0; i < 256; i++) {
		uint64 r = i;
		unsigned j;
		for(j = 0; j < 8; j++)
			r = (r >> 1) ^ (kCrc64Poly & ((uint64)0 - (r & 1)));
		g_Crc64Table[i] = r;
	}
	for(i = 256; i < 256 * CRC_NUM_TABLES; i++) {
		uint64 r = g_Crc64Table[(size_t)i - 256];
		g_Crc64Table[i] = g_Crc64Table[r & 0xFF] ^ (r >> 8);
	}

  #ifdef MY_CPU_LE

	g_Crc64Update = XzCrc64UpdateT4;

  #else
	{
    #ifndef MY_CPU_BE
		uint32 k = 1;
		if(*(const Byte*)&k == 1)
			g_Crc64Update = XzCrc64UpdateT4;
		else
    #endif
		{
			for(i = 256 * CRC_NUM_TABLES - 1; i >= 256; i--) {
				uint64 x = g_Crc64Table[(size_t)i - 256];
				g_Crc64Table[i] = CRC_UINT64_SWAP(x);
			}
			g_Crc64Update = XzCrc64UpdateT1_BeT4;
		}
	}
  #endif
}

