// 7Z-CRC.C
// Igor Pavlov : Public domain 
//
#include <7z-internal.h>
#pragma hdrstop

#define kCrcPoly   0xEDB88320
#define kCrc64Poly UINT64_CONST(0xC96C5795D7870F42)

#define CRC_UPDATE_BYTE_2(crc, b) (table[((crc) ^ (b)) & 0xFF] ^ ((crc) >> 8))

// 7zCrc.c -- CRC32 init

#ifdef MY_CPU_LE
	#define CRC32_NUM_TABLES 8
#else
	#define CRC32_NUM_TABLES 9
	#define CRC_UINT32_SWAP(v) ((v >> 24) | ((v >> 8) & 0xFF00) | ((v << 8) & 0xFF0000) | (v << 24))

	uint32 FASTCALL CrcUpdateT1_BeT4(uint32 v, const void * data, size_t size, const uint32 * table);
	uint32 FASTCALL CrcUpdateT1_BeT8(uint32 v, const void * data, size_t size, const uint32 * table);
#endif
#ifndef MY_CPU_BE
	uint32 FASTCALL CrcUpdateT4(uint32 v, const void * data, size_t size, const uint32 * table);
	uint32 FASTCALL CrcUpdateT8(uint32 v, const void * data, size_t size, const uint32 * table);
#endif

typedef uint32 (FASTCALL * CRC32_FUNC)(uint32 v, const void * data, size_t size, const uint32 * table);

CRC32_FUNC g_CrcUpdateT4 = 0;
CRC32_FUNC g_CrcUpdateT8 = 0;
CRC32_FUNC g_CrcUpdate = 0;

uint32 g_CrcTable[256 * CRC32_NUM_TABLES];

uint32 FASTCALL CrcUpdate(uint32 v, const void * data, size_t size) { return g_CrcUpdate(v, data, size, g_CrcTable); }
uint32 FASTCALL CrcCalc(const void * data, size_t size) { return g_CrcUpdate(CRC_INIT_VAL, data, size, g_CrcTable) ^ CRC_INIT_VAL; }

uint32 FASTCALL CrcUpdateT1(uint32 v, const void * data, size_t size, const uint32 * table)
{
	const Byte * p = (const Byte *)data;
	const Byte * pEnd = p + size;
	for(; p != pEnd; p++)
		v = CRC_UPDATE_BYTE_2(v, *p);
	return v;
}

void FASTCALL CrcGenerateTable()
{
	uint32 i;
	for(i = 0; i < 256; i++) {
		uint32 r = i;
		uint   j;
		for(j = 0; j < 8; j++)
			r = (r >> 1) ^ (kCrcPoly & ((uint32)0 - (r & 1)));
		g_CrcTable[i] = r;
	}
	for(i = 256; i < 256 * CRC32_NUM_TABLES; i++) {
		uint32 r = g_CrcTable[(size_t)i - 256];
		g_CrcTable[i] = g_CrcTable[r & 0xFF] ^ (r >> 8);
	}
#if CRC32_NUM_TABLES < 4
	g_CrcUpdate = CrcUpdateT1;
#else
	#ifdef MY_CPU_LE
	g_CrcUpdateT4 = CrcUpdateT4;
	g_CrcUpdate = CrcUpdateT4;
		#if CRC32_NUM_TABLES >= 8
	g_CrcUpdateT8 = CrcUpdateT8;
			#ifdef MY_CPU_X86_OR_AMD64
	if(!CPU_Is_InOrder())
		g_CrcUpdate = CrcUpdateT8;
			#endif
		#endif
	#else
	{
    #ifndef MY_CPU_BE
		uint32 k = 0x01020304;
		const Byte * p = (const Byte *)&k;
		if(p[0] == 4 && p[1] == 3) {
			g_CrcUpdateT4 = CrcUpdateT4;
			g_CrcUpdate = CrcUpdateT4;
      #if CRC32_NUM_TABLES >= 8
			g_CrcUpdateT8 = CrcUpdateT8;
			// g_CrcUpdate = CrcUpdateT8;
      #endif
		}
		else if(p[0] != 1 || p[1] != 2)
			g_CrcUpdate = CrcUpdateT1;
		else
    #endif
		{
			for(i = 256 * CRC32_NUM_TABLES - 1; i >= 256; i--) {
				uint32 x = g_CrcTable[(size_t)i - 256];
				g_CrcTable[i] = CRC_UINT32_SWAP(x);
			}
			g_CrcUpdateT4 = CrcUpdateT1_BeT4;
			g_CrcUpdate = CrcUpdateT1_BeT4;
      #if CRC32_NUM_TABLES >= 8
			g_CrcUpdateT8 = CrcUpdateT1_BeT8;
			// g_CrcUpdate = CrcUpdateT1_BeT8;
      #endif
		}
	}
  #endif

  #endif
}
//
// 7zCrcOpt.c -- CRC32 calculation
#ifndef MY_CPU_BE
	//#define CRC_UPDATE_BYTE_2(crc, b) (table[((crc) ^ (b)) & 0xFF] ^ ((crc) >> 8))

	uint32 FASTCALL CrcUpdateT4(uint32 v, const void * data, size_t size, const uint32 * table)
	{
		const Byte * p = (const Byte *)data;
		for(; size > 0 && ((uint)(ptrdiff_t)p & 3) != 0; size--, p++)
			v = CRC_UPDATE_BYTE_2(v, *p);
		for(; size >= 4; size -= 4, p += 4) {
			v ^= *(const uint32 *)p;
			v = (table + 0x300)[((v) & 0xFF)] ^ (table + 0x200)[((v >>  8) & 0xFF)] ^ (table + 0x100)[((v >> 16) & 0xFF)] ^ (table + 0x000)[((v >> 24))];
		}
		for(; size > 0; size--, p++)
			v = CRC_UPDATE_BYTE_2(v, *p);
		return v;
	}

	uint32 FASTCALL CrcUpdateT8(uint32 v, const void * data, size_t size, const uint32 * table)
	{
		const Byte * p = (const Byte *)data;
		for(; size > 0 && ((uint)(ptrdiff_t)p & 7) != 0; size--, p++)
			v = CRC_UPDATE_BYTE_2(v, *p);
		for(; size >= 8; size -= 8, p += 8) {
			uint32 d;
			v ^= *(const uint32 *)p;
			v = (table + 0x700)[((v) & 0xFF)] ^ (table + 0x600)[((v >>  8) & 0xFF)] ^ (table + 0x500)[((v >> 16) & 0xFF)] ^ (table + 0x400)[((v >> 24))];
			d = *((const uint32 *)p + 1);
			v ^= (table + 0x300)[((d) & 0xFF)] ^ (table + 0x200)[((d >>  8) & 0xFF)] ^ (table + 0x100)[((d >> 16) & 0xFF)] ^ (table + 0x000)[((d >> 24))];
		}
		for(; size > 0; size--, p++)
			v = CRC_UPDATE_BYTE_2(v, *p);
		return v;
	}
#endif
#ifndef MY_CPU_LE
	#define CRC_UINT32_SWAP(v) ((v >> 24) | ((v >> 8) & 0xFF00) | ((v << 8) & 0xFF0000) | (v << 24))
	#define CRC_UPDATE_BYTE_2_BE(crc, b) (table[(((crc) >> 24) ^ (b))] ^ ((crc) << 8))

	uint32 FASTCALL CrcUpdateT1_BeT4(uint32 v, const void * data, size_t size, const uint32 * table)
	{
		const Byte * p = (const Byte *)data;
		table += 0x100;
		v = CRC_UINT32_SWAP(v);
		for(; size > 0 && ((uint)(ptrdiff_t)p & 3) != 0; size--, p++)
			v = CRC_UPDATE_BYTE_2_BE(v, *p);
		for(; size >= 4; size -= 4, p += 4) {
			v ^= *(const uint32 *)p;
			v = (table + 0x000)[((v) & 0xFF)] ^ (table + 0x100)[((v >>  8) & 0xFF)] ^ (table + 0x200)[((v >> 16) & 0xFF)] ^ (table + 0x300)[((v >> 24))];
		}
		for(; size > 0; size--, p++)
			v = CRC_UPDATE_BYTE_2_BE(v, *p);
		return CRC_UINT32_SWAP(v);
	}

	uint32 FASTCALL CrcUpdateT1_BeT8(uint32 v, const void * data, size_t size, const uint32 * table)
	{
		const Byte * p = (const Byte *)data;
		table += 0x100;
		v = CRC_UINT32_SWAP(v);
		for(; size > 0 && ((uint)(ptrdiff_t)p & 7) != 0; size--, p++)
			v = CRC_UPDATE_BYTE_2_BE(v, *p);
		for(; size >= 8; size -= 8, p += 8) {
			uint32 d;
			v ^= *(const uint32 *)p;
			v = (table + 0x400)[((v) & 0xFF)] ^ (table + 0x500)[((v >>  8) & 0xFF)] ^ (table + 0x600)[((v >> 16) & 0xFF)] ^ (table + 0x700)[((v >> 24))];
			d = *((const uint32 *)p + 1);
			v ^= (table + 0x000)[((d) & 0xFF)] ^ (table + 0x100)[((d >>  8) & 0xFF)] ^ (table + 0x200)[((d >> 16) & 0xFF)] ^ (table + 0x300)[((d >> 24))];
		}
		for(; size > 0; size--, p++)
			v = CRC_UPDATE_BYTE_2_BE(v, *p);
		return CRC_UINT32_SWAP(v);
	}
#endif
//
// XzCrc64.c -- CRC64 calculation
#ifdef MY_CPU_LE
	#define CRC64_NUM_TABLES 4
#else
	#define CRC64_NUM_TABLES 5
	#define CRC_UINT64_SWAP(v) ((v >> 56) | ((v >> 40) & ((uint64)0xFF <<  8)) | ((v >> 24) & ((uint64)0xFF << 16)) | \
		((v >>  8) & ((uint64)0xFF << 24)) | ((v <<  8) & ((uint64)0xFF << 32)) | ((v << 24) & ((uint64)0xFF << 40)) | \
		((v << 40) & ((uint64)0xFF << 48)) | ((v << 56)))

	uint64 FASTCALL XzCrc64UpdateT1_BeT4(uint64 v, const void * data, size_t size, const uint64 * table);
#endif
#ifndef MY_CPU_BE
	uint64 FASTCALL XzCrc64UpdateT4(uint64 v, const void * data, size_t size, const uint64 * table);
#endif

typedef uint64 (FASTCALL * CRC64_FUNC)(uint64 v, const void * data, size_t size, const uint64 * table);

static CRC64_FUNC g_Crc64Update;
uint64 g_Crc64Table[256 * CRC64_NUM_TABLES];

uint64 FASTCALL Crc64Update(uint64 v, const void * data, size_t size) { return g_Crc64Update(v, data, size, g_Crc64Table); }
uint64 FASTCALL Crc64Calc(const void * data, size_t size) { return g_Crc64Update(CRC64_INIT_VAL, data, size, g_Crc64Table) ^ CRC64_INIT_VAL; }

void FASTCALL Crc64GenerateTable()
{
	uint32 i;
	for(i = 0; i < 256; i++) {
		uint64 r = i;
		uint   j;
		for(j = 0; j < 8; j++)
			r = (r >> 1) ^ (kCrc64Poly & ((uint64)0 - (r & 1)));
		g_Crc64Table[i] = r;
	}
	for(i = 256; i < 256 * CRC64_NUM_TABLES; i++) {
		uint64 r = g_Crc64Table[(size_t)i - 256];
		g_Crc64Table[i] = g_Crc64Table[r & 0xFF] ^ (r >> 8);
	}
#ifdef MY_CPU_LE
	g_Crc64Update = XzCrc64UpdateT4;
#else
	{
    #ifndef MY_CPU_BE
		uint32 k = 1;
		if(*(const Byte *)&k == 1)
			g_Crc64Update = XzCrc64UpdateT4;
		else
    #endif
		{
			for(i = 256 * CRC64_NUM_TABLES - 1; i >= 256; i--) {
				uint64 x = g_Crc64Table[(size_t)i - 256];
				g_Crc64Table[i] = CRC_UINT64_SWAP(x);
			}
			g_Crc64Update = XzCrc64UpdateT1_BeT4;
		}
	}
#endif
}
//
// XzCrc64Opt.c -- CRC64 calculation
#ifndef MY_CPU_BE
	//#define CRC_UPDATE_BYTE_2(crc, b) (table[((crc) ^ (b)) & 0xFF] ^ ((crc) >> 8))

	uint64 FASTCALL XzCrc64UpdateT4(uint64 v, const void * data, size_t size, const uint64 * table)
	{
		const Byte * p = (const Byte *)data;
		for(; size > 0 && ((uint)(ptrdiff_t)p & 3) != 0; size--, p++)
			v = CRC_UPDATE_BYTE_2(v, *p);
		for(; size >= 4; size -= 4, p += 4) {
			uint32 d = (uint32)v ^ *(const uint32 *)p;
			v = (v >> 32) ^ (table + 0x300)[((d) & 0xFF)] ^ (table + 0x200)[((d >>  8) & 0xFF)] ^ (table + 0x100)[((d >> 16) & 0xFF)] ^ (table + 0x000)[((d >> 24))];
		}
		for(; size > 0; size--, p++)
			v = CRC_UPDATE_BYTE_2(v, *p);
		return v;
	}
#endif
#ifndef MY_CPU_LE
	#define CRC_UINT64_SWAP(v) ((v >> 56) | ((v >> 40) & ((uint64)0xFF <<  8)) | ((v >> 24) & ((uint64)0xFF << 16)) | \
		((v >>  8) & ((uint64)0xFF << 24)) | ((v <<  8) & ((uint64)0xFF << 32)) | ((v << 24) & ((uint64)0xFF << 40)) | ((v << 40) & ((uint64)0xFF << 48)) | ((v << 56)))
	#define CRC64_UPDATE_BYTE_2_BE(crc, b) (table[(Byte)((crc) >> 56) ^ (b)] ^ ((crc) << 8))

	uint64 FASTCALL XzCrc64UpdateT1_BeT4(uint64 v, const void * data, size_t size, const uint64 * table)
	{
		const Byte * p = (const Byte *)data;
		table += 0x100;
		v = CRC_UINT64_SWAP(v);
		for(; size > 0 && ((uint)(ptrdiff_t)p & 3) != 0; size--, p++)
			v = CRC64_UPDATE_BYTE_2_BE(v, *p);
		for(; size >= 4; size -= 4, p += 4) {
			uint32 d = (uint32)(v >> 32) ^ *(const uint32 *)p;
			v = (v << 32) ^ (table + 0x000)[((d) & 0xFF)] ^ (table + 0x100)[((d >>  8) & 0xFF)] ^ (table + 0x200)[((d >> 16) & 0xFF)] ^ (table + 0x300)[((d >> 24))];
		}
		for(; size > 0; size--, p++)
			v = CRC64_UPDATE_BYTE_2_BE(v, *p);
		return CRC_UINT64_SWAP(v);
	}
#endif
//
