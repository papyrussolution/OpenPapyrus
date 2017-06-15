/****************************************************************************

   gif_lib_private.h - internal giflib routines and structures

****************************************************************************/

#ifndef _GIF_LIB_PRIVATE_H
#define _GIF_LIB_PRIVATE_H

#include "gif_lib.h"
//#include "gif_hash.h"
#include <unistd.h>
#include <stdint.h>

#define HT_SIZE           8192 // 12bits = 4096 or twice as big! 
#define HT_KEY_MASK     0x1FFF // 13bits keys 
#define HT_KEY_NUM_BITS     13 // 13bits keys 
#define HT_MAX_KEY        8191 // 13bits - 1, maximal code possible 
#define HT_MAX_CODE       4095 // Biggest code possible in 12 bits. 
// 
// The 32 bits of the long are divided into two parts for the key & code:   
// 1. The code is 12 bits as our compression algorithm is limited to 12bits 
// 2. The key is 12 bits Prefix code + 8 bit new char or 20 bits.	    
// The key is the upper 20 bits.  The code is the lower 12. 
#define HT_GET_KEY(l)	(l >> 12)
#define HT_GET_CODE(l)	(l & 0x0FFF)
#define HT_PUT_KEY(l)	(l << 12)
#define HT_PUT_CODE(l)	(l & 0x0FFF)

typedef struct GifHashTableType {
    uint32 HTable[HT_SIZE];
} GifHashTableType;

GifHashTableType *_InitHashTable(void);
void _ClearHashTable(GifHashTableType *HashTable);
void _InsertHashTable(GifHashTableType *HashTable, uint32 Key, int Code);
int _ExistsHashTable(GifHashTableType *HashTable, uint32 Key);

#define EXTENSION_INTRODUCER      0x21
#define DESCRIPTOR_INTRODUCER     0x2c
#define TERMINATOR_INTRODUCER     0x3b

#define LZ_MAX_CODE         4095    /* Biggest code possible in 12 bits. */
#define LZ_BITS             12

#define FLUSH_OUTPUT        4096    /* Impossible code, to signal flush. */
#define FIRST_CODE          4097    /* Impossible code, to signal first. */
#define NO_SUCH_CODE        4098    /* Impossible code, to signal empty. */

#define FILE_STATE_WRITE    0x01
#define FILE_STATE_SCREEN   0x02
#define FILE_STATE_IMAGE    0x04
#define FILE_STATE_READ     0x08

#define IS_READABLE(Private)    (Private->FileState & FILE_STATE_READ)
#define IS_WRITEABLE(Private)   (Private->FileState & FILE_STATE_WRITE)

typedef struct GifFilePrivateType {
	GifWord FileState, FileHandle; /* Where all this data goes to! */
	GifWord BitsPerPixel; /* Bits per pixel (Codes uses at least this + 1). */
	GifWord ClearCode; /* The CLEAR LZ code. */
	GifWord EOFCode; /* The EOF LZ code. */
	GifWord RunningCode; /* The next code algorithm can generate. */
	GifWord RunningBits; /* The number of bits required to represent RunningCode. */
	GifWord MaxCode1; /* 1 bigger than max. possible code, in RunningBits bits. */
	GifWord LastCode; /* The code before the current code. */
	GifWord CrntCode; /* Current algorithm code. */
	GifWord StackPtr; /* For character stack (see below). */
	GifWord CrntShiftState; /* Number of bits in CrntShiftDWord. */
	ulong CrntShiftDWord; /* For bytes decomposition into codes. */
	ulong PixelCount; /* Number of pixels in image. */
	FILE * File; /* File as stream. */
	InputFunc Read; /* function to read gif input (TVT) */
	OutputFunc Write; /* function to write gif output (MRB) */
	GifByteType Buf[256]; /* Compressed input is buffered here. */
	GifByteType Stack[LZ_MAX_CODE]; /* Decoded pixels are stacked here. */
	GifByteType Suffix[LZ_MAX_CODE + 1]; /* So we can trace the codes. */
	GifPrefixType Prefix[LZ_MAX_CODE + 1];
	GifHashTableType * HashTable;
	bool gif89;
} GifFilePrivateType;

#endif /* _GIF_LIB_PRIVATE_H */

/* end */
