/* 7z.h -- 7z interface
   2017-04-03 : Igor Pavlov : Public domain */

#ifndef __7Z_H
#define __7Z_H

//#include <7zTypes.h>

EXTERN_C_BEGIN

#define k7zStartHeaderSize 0x20
#define k7zSignatureSize 6

extern const Byte k7zSignature[k7zSignatureSize];

typedef struct {
	const Byte * Data;
	size_t Size;
} CSzData;

/* CSzCoderInfo & CSzFolder support only default methods */

typedef struct {
	size_t PropsOffset;
	uint32 MethodID;
	Byte NumStreams;
	Byte PropsSize;
} CSzCoderInfo;

typedef struct {
	uint32 InIndex;
	uint32 OutIndex;
} CSzBond;

#define SZ_NUM_CODERS_IN_FOLDER_MAX 4
#define SZ_NUM_BONDS_IN_FOLDER_MAX 3
#define SZ_NUM_PACK_STREAMS_IN_FOLDER_MAX 4

typedef struct {
	uint32 NumCoders;
	uint32 NumBonds;
	uint32 NumPackStreams;
	uint32 UnpackStream;
	uint32 PackStreams[SZ_NUM_PACK_STREAMS_IN_FOLDER_MAX];
	CSzBond Bonds[SZ_NUM_BONDS_IN_FOLDER_MAX];
	CSzCoderInfo Coders[SZ_NUM_CODERS_IN_FOLDER_MAX];
} CSzFolder;

SRes SzGetNextFolderItem(CSzFolder * f, CSzData * sd);

typedef struct {
	uint32 Low;
	uint32 High;
} CNtfsFileTime;

typedef struct {
	Byte * Defs; /* MSB 0 bit numbering */
	uint32 * Vals;
} CSzBitUi32s;

typedef struct {
	Byte * Defs; /* MSB 0 bit numbering */
	// uint64 *Vals;
	CNtfsFileTime * Vals;
} CSzBitUi64s;

#define SzBitArray_Check(p, i) (((p)[(i) >> 3] & (0x80 >> ((i) & 7))) != 0)

#define SzBitWithVals_Check(p, i) ((p)->Defs && ((p)->Defs[(i) >> 3] & (0x80 >> ((i) & 7))) != 0)

typedef struct {
	uint32 NumPackStreams;
	uint32 NumFolders;

	uint64 * PackPositions;   // NumPackStreams + 1
	CSzBitUi32s FolderCRCs;   // NumFolders

	size_t * FoCodersOffsets; // NumFolders + 1
	uint32 * FoStartPackStreamIndex; // NumFolders + 1
	uint32 * FoToCoderUnpackSizes; // NumFolders + 1
	Byte * FoToMainUnpackSizeIndex; // NumFolders
	uint64 * CoderUnpackSizes; // for all coders in all folders

	Byte * CodersData;
} CSzAr;

uint64 SzAr_GetFolderUnpackSize(const CSzAr * p, uint32 folderIndex);
SRes SzAr_DecodeFolder(const CSzAr * p, uint32 folderIndex, ILookInStream * stream, uint64 startPos, Byte * outBuffer, size_t outSize, ISzAllocPtr allocMain);

typedef struct {
	CSzAr db;
	uint64 startPosAfterHeader;
	uint64 dataPos;
	uint32 NumFiles;
	uint64 * UnpackPositions; // NumFiles + 1
	// Byte *IsEmptyFiles;
	Byte * IsDirs;
	CSzBitUi32s CRCs;

	CSzBitUi32s Attribs;
	// CSzBitUi32s Parents;
	CSzBitUi64s MTime;
	CSzBitUi64s CTime;

	uint32 * FolderToFile; // NumFolders + 1
	uint32 * FileToFolder; // NumFiles

	size_t * FileNameOffsets; /* in 2-byte steps */
	Byte * FileNames; /* UTF-16-LE */
} CSzArEx;

#define SzArEx_IsDir(p, i) (SzBitArray_Check((p)->IsDirs, i))

#define SzArEx_GetFileSize(p, i) ((p)->UnpackPositions[(i) + 1] - (p)->UnpackPositions[i])

void SzArEx_Init(CSzArEx * p);
void SzArEx_Free(CSzArEx * p, ISzAllocPtr alloc);
uint64 SzArEx_GetFolderStreamPos(const CSzArEx * p, uint32 folderIndex, uint32 indexInFolder);
int SzArEx_GetFolderFullPackSize(const CSzArEx * p, uint32 folderIndex, uint64 * resSize);

/*
   if dest == NULL, the return value specifies the required size of the buffer,
   in 16-bit characters, including the null-terminating character.
   if dest != NULL, the return value specifies the number of 16-bit characters that
   are written to the dest, including the null-terminating character. */

size_t SzArEx_GetFileNameUtf16(const CSzArEx * p, size_t fileIndex, uint16 * dest);

/*
   size_t SzArEx_GetFullNameLen(const CSzArEx *p, size_t fileIndex);
   uint16 *SzArEx_GetFullNameUtf16_Back(const CSzArEx *p, size_t fileIndex, uint16 *dest);
 */

/*
   SzArEx_Extract extracts file from archive

   *outBuffer must be 0 before first call for each new archive.

   Extracting cache:
    If you need to decompress more than one file, you can send
    these values from previous call:
   *blockIndex,
   *outBuffer,
   *outBufferSize
    You can consider "*outBuffer" as cache of solid block. If your archive is solid,
    it will increase decompression speed.

    If you use external function, you can declare these 3 cache variables
    (blockIndex, outBuffer, outBufferSize) as static in that external function.

    Free *outBuffer and set *outBuffer to 0, if you want to flush cache.
 */

SRes SzArEx_Extract(const CSzArEx * db,
    ILookInStream * inStream,
    uint32 fileIndex,         /* index of file */
    uint32 * blockIndex,       /* index of solid block */
    Byte ** outBuffer,         /* pointer to pointer to output buffer (allocated with allocMain) */
    size_t * outBufferSize,    /* buffer size for output buffer */
    size_t * offset,           /* offset of stream for required file in *outBuffer */
    size_t * outSizeProcessed, /* size of file in *outBuffer */
    ISzAllocPtr allocMain,
    ISzAllocPtr allocTemp);

/*
   SzArEx_Open Errors:
   SZ_ERROR_NO_ARCHIVE
   SZ_ERROR_ARCHIVE
   SZ_ERROR_UNSUPPORTED
   SZ_ERROR_MEM
   SZ_ERROR_CRC
   SZ_ERROR_INPUT_EOF
   SZ_ERROR_FAIL
 */

SRes SzArEx_Open(CSzArEx * p, ILookInStream * inStream,
    ISzAllocPtr allocMain, ISzAllocPtr allocTemp);

EXTERN_C_END

#endif
