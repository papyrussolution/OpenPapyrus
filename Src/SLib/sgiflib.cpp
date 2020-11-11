// SGIFLIB.CPP
//
#include <slib-internal.h>
#pragma hdrstop
#include <gif_lib.h>
//#include "gif_lib_private.h"
//#pragma hdrstop
//#include <fcntl.h>
//#include <sys/stat.h>
//#include <unistd.h>
//#include <stdint.h>

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
#define HT_GET_KEY(l)	((l) >> 12)
#define HT_GET_CODE(l)	((l) & 0x0FFF)
#define HT_PUT_KEY(l)	((l) << 12)
#define HT_PUT_CODE(l)	((l) & 0x0FFF)

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
//
//
//
#define ABS(x)    ((x) > 0 ? (x) : (-(x)))
/* extract bytes from an unsigned word */
// @sobolev #define LOBYTE(x)       ((x) & 0xff)
// @sobolev #define HIBYTE(x)       (((x) >> 8) & 0xff)
// compose unsigned little endian value 
#define UNSIGNED_LITTLE_ENDIAN(lo, hi)  ((lo) | ((hi) << 8))
// avoid extra function call in case we use fread (TVT) 
#define READ(_gif, _buf, _len) (static_cast<GifFilePrivateType *>((_gif)->Private)->Read ? static_cast<GifFilePrivateType *>((_gif)->Private)->Read(_gif, _buf, _len) : \
	fread(_buf, 1, _len, static_cast<GifFilePrivateType *>((_gif)->Private)->File))
#define COLOR_ARRAY_SIZE 32768
#define BITS_PER_PRIM_COLOR 5
#define MAX_PRIM_COLOR      0x1f
//
//
//
struct GifHashTableType {
    uint32 HTable[HT_SIZE];
};

struct GifFilePrivateType {
	int    FileState;
	int    FileHandle; /* Where all this data goes to! */
	int    BitsPerPixel; /* Bits per pixel (Codes uses at least this + 1). */
	int    ClearCode; /* The CLEAR LZ code. */
	int    EOFCode; /* The EOF LZ code. */
	int    RunningCode; /* The next code algorithm can generate. */
	int    RunningBits; /* The number of bits required to represent RunningCode. */
	int    MaxCode1; /* 1 bigger than max. possible code, in RunningBits bits. */
	int    LastCode; /* The code before the current code. */
	int    CrntCode; /* Current algorithm code. */
	int    StackPtr; /* For character stack (see below). */
	int    CrntShiftState; /* Number of bits in CrntShiftDWord. */
	ulong  CrntShiftDWord; /* For bytes decomposition into codes. */
	ulong  PixelCount; /* Number of pixels in image. */
	FILE * File; /* File as stream. */
	InputFunc Read; /* function to read gif input (TVT) */
	OutputFunc Write; /* function to write gif output (MRB) */
	GifByteType Buf[256]; /* Compressed input is buffered here. */
	GifByteType Stack[LZ_MAX_CODE]; /* Decoded pixels are stacked here. */
	GifByteType Suffix[LZ_MAX_CODE + 1]; /* So we can trace the codes. */
	GifPrefixType Prefix[LZ_MAX_CODE + 1];
	GifHashTableType * HashTable;
	bool gif89;
};

GifHashTableType * _InitHashTable(void);
void _ClearHashTable(GifHashTableType *HashTable);
void _InsertHashTable(GifHashTableType *HashTable, uint32 Key, int Code);
int _ExistsHashTable(GifHashTableType *HashTable, uint32 Key);
//
// Masks given codes to BitsPerPixel, to make sure all codes are in range: */
// @+charint@
static const GifPixelType CodeMask[] = { 0x00, 0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0xff };
static int SortRGBAxis; // @global
// @-charint@
// #define  DEBUG_HIT_RATE    Debug number of misses per hash Insert/Exists. */
#ifdef  DEBUG_HIT_RATE
	static long NumberOfTests = 0; // @global
	static long NumberOfMisses = 0; // @global
#endif
// 
// Return a string description of  the last GIF error
// 
char * GifErrorString(int ErrorCode)
{
	char * Err;
	switch(ErrorCode) {
		case E_GIF_ERR_OPEN_FAILED: Err = "Failed to open given file"; break;
		case E_GIF_ERR_WRITE_FAILED: Err = "Failed to write to given file"; break;
		case E_GIF_ERR_HAS_SCRN_DSCR: Err = "Screen descriptor has already been set"; break;
		case E_GIF_ERR_HAS_IMAG_DSCR: Err = "Image descriptor is still active"; break;
		case E_GIF_ERR_NO_COLOR_MAP: Err = "Neither global nor local color map"; break;
		case E_GIF_ERR_DATA_TOO_BIG: Err = "Number of pixels bigger than width * height"; break;
		case E_GIF_ERR_NOT_ENOUGH_MEM: Err = "Failed to allocate required memory"; break;
		case E_GIF_ERR_DISK_IS_FULL: Err = "Write failed (disk full?)"; break;
		case E_GIF_ERR_CLOSE_FAILED: Err = "Failed to close given file"; break;
		case E_GIF_ERR_NOT_WRITEABLE: Err = "Given file was not opened for write"; break;
		case D_GIF_ERR_OPEN_FAILED: Err = "Failed to open given file"; break;
		case D_GIF_ERR_READ_FAILED: Err = "Failed to read from given file"; break;
		case D_GIF_ERR_NOT_GIF_FILE: Err = "Data is not in GIF format"; break;
		case D_GIF_ERR_NO_SCRN_DSCR: Err = "No screen descriptor detected"; break;
		case D_GIF_ERR_NO_IMAG_DSCR: Err = "No Image Descriptor detected"; break;
		case D_GIF_ERR_NO_COLOR_MAP: Err = "Neither global nor local color map"; break;
		case D_GIF_ERR_WRONG_RECORD: Err = "Wrong record type detected"; break;
		case D_GIF_ERR_DATA_TOO_BIG: Err = "Number of pixels bigger than width * height"; break;
		case D_GIF_ERR_NOT_ENOUGH_MEM: Err = "Failed to allocate required memory"; break;
		case D_GIF_ERR_CLOSE_FAILED: Err = "Failed to close given file"; break;
		case D_GIF_ERR_NOT_READABLE: Err = "Given file was not opened for read"; break;
		case D_GIF_ERR_IMAGE_DEFECT: Err = "Image is defective, decoding aborted"; break;
		case D_GIF_ERR_EOF_TOO_SOON: Err = "Image EOF detected before image complete"; break;
		default: Err = NULL; break;
	}
	return Err;
}
// 
// dgif_lib.c - GIF decoding
// 
// The functions here and in egif_lib.c are partitioned carefully so that
// if you only require one of read and write capability, only one of these
// two modules will be linked.  Preserve this property!
// 
// Open a new GIF file for read, given by its name.
// Returns dynamically allocated GifFileType pointer which serves as the GIF info record.
// 
GifFileType * DGifOpenFileName(const char * FileName, int * Error)
{
	int FileHandle;
	GifFileType * GifFile = 0;
	if((FileHandle = open(FileName, O_RDONLY)) == -1) {
		ASSIGN_PTR(Error, D_GIF_ERR_OPEN_FAILED);
	}
	else
		GifFile = DGifOpenFileHandle(FileHandle, Error);
	// cppcheck-suppress resourceLeak
	return GifFile;
}
// 
// Update a new GIF file, given its file handle.
// Returns dynamically allocated GifFileType pointer which serves as the GIF info record.
// 
GifFileType * DGifOpenFileHandle(int FileHandle, int * Error)
{
	char Buf[GIF_STAMP_LEN + 1];
	GifFilePrivateType * p_private = 0;
	FILE * f = 0;
	GifFileType * p_giffile = static_cast<GifFileType *>(SAlloc::M(sizeof(GifFileType)));
	if(p_giffile == NULL) {
		ASSIGN_PTR(Error, D_GIF_ERR_NOT_ENOUGH_MEM);
		CALLEXCEPT();
	}
	/*@i1@*/ memzero(p_giffile, sizeof(GifFileType));
	/* Belt and suspenders, in case the null pointer isn't zero */
	p_giffile->SavedImages = NULL;
	p_giffile->SColorMap = NULL;
	p_private = static_cast<GifFilePrivateType *>(SAlloc::M(sizeof(GifFilePrivateType)));
	if(p_private == NULL) {
		ASSIGN_PTR(Error, D_GIF_ERR_NOT_ENOUGH_MEM);
		CALLEXCEPT();
	}
#ifdef _WIN32
	_setmode(FileHandle, O_BINARY); /* Make sure it is in binary mode. */
#endif
	f = fdopen(FileHandle, "rb"); /* Make it into a stream: */
	/*@-mustfreeonly@*/
	p_giffile->Private = p_private;
	p_private->FileHandle = FileHandle;
	p_private->File = f;
	p_private->FileState = FILE_STATE_READ;
	p_private->Read = NULL;    /* don't use alternate input method (TVT) */
	p_giffile->UserData = NULL; /* TVT */
	FileHandle = -1; // @sobolev
	/*@=mustfreeonly@*/
	// Let's see if this is a GIF file: 
	if(READ(p_giffile, (uint8 *)Buf, GIF_STAMP_LEN) != GIF_STAMP_LEN) {
		ASSIGN_PTR(Error, D_GIF_ERR_READ_FAILED);
		CALLEXCEPT();
	}
	// Check for GIF prefix at start of file 
	Buf[GIF_STAMP_LEN] = 0;
	if(strncmp(GIF_STAMP, Buf, GIF_VERSION_POS) != 0) {
		ASSIGN_PTR(Error, D_GIF_ERR_NOT_GIF_FILE);
		CALLEXCEPT();
	}
	THROW(DGifGetScreenDesc(p_giffile) != GIF_ERROR);
	p_giffile->Error = 0;
	// What version of GIF? 
	p_private->gif89 = (Buf[GIF_VERSION_POS] == '9');
	CATCH
		if(FileHandle >= 0)
			close(FileHandle);
		SFile::ZClose(&f);
		SAlloc::F(p_private);
		ZFREE(p_giffile);
	ENDCATCH
	return p_giffile;
}
// 
// GifFileType constructor with user supplied input function (TVT)
// 
GifFileType * DGifOpen(void * userData, InputFunc readFunc, int * Error)
{
	char Buf[GIF_STAMP_LEN + 1];
	GifFilePrivateType * Private;
	GifFileType * GifFile = static_cast<GifFileType *>(SAlloc::M(sizeof(GifFileType)));
	if(GifFile == NULL) {
		ASSIGN_PTR(Error, D_GIF_ERR_NOT_ENOUGH_MEM);
		return NULL;
	}
	memzero(GifFile, sizeof(GifFileType));
	// Belt and suspenders, in case the null pointer isn't zero 
	GifFile->SavedImages = NULL;
	GifFile->SColorMap = NULL;
	Private = static_cast<GifFilePrivateType *>(SAlloc::M(sizeof(GifFilePrivateType)));
	if(!Private) {
		ASSIGN_PTR(Error, D_GIF_ERR_NOT_ENOUGH_MEM);
		SAlloc::F(GifFile);
		return NULL;
	}
	GifFile->Private = Private;
	Private->FileHandle = 0;
	Private->File = NULL;
	Private->FileState = FILE_STATE_READ;
	Private->Read = readFunc; /* TVT */
	GifFile->UserData = userData; /* TVT */
	// Lets see if this is a GIF file: 
	const size_t hdr_size = GIF_STAMP_LEN;
	const size_t rd_hdr_size = READ(GifFile, (uint8 *)Buf, GIF_STAMP_LEN);
	if(rd_hdr_size != hdr_size) {
	//if(READ(GifFile, (uint8 *)Buf, GIF_STAMP_LEN) != GIF_STAMP_LEN) {
		ASSIGN_PTR(Error, D_GIF_ERR_READ_FAILED);
		SAlloc::F(Private);
		SAlloc::F(GifFile);
		return NULL;
	}
	// Check for GIF prefix at start of file 
	Buf[GIF_STAMP_LEN] = '\0';
	if(strncmp(GIF_STAMP, Buf, GIF_VERSION_POS) != 0) {
		ASSIGN_PTR(Error, D_GIF_ERR_NOT_GIF_FILE);
		SAlloc::F(Private);
		SAlloc::F(GifFile);
		return NULL;
	}
	if(DGifGetScreenDesc(GifFile) == GIF_ERROR) {
		SAlloc::F(Private);
		SAlloc::F(GifFile);
		return NULL;
	}
	GifFile->Error = 0;
	// What version of GIF? 
	Private->gif89 = (Buf[GIF_VERSION_POS] == '9');
	return GifFile;
}
// 
// Get 2 bytes (word) from the given file:
// 
static int FASTCALL DGifGetWord(GifFileType * GifFile, int * Word)
{
	uint8 c[2];
	if(READ(GifFile, c, 2) != 2) {
		GifFile->Error = D_GIF_ERR_READ_FAILED;
		return GIF_ERROR;
	}
	else {
		*Word = (int)UNSIGNED_LITTLE_ENDIAN(c[0], c[1]);
		return GIF_OK;
	}
}
// 
// This routine should be called before any other DGif calls. Note that
// this routine is called automatically from DGif file open routines.
// 
int DGifGetScreenDesc(GifFileType * GifFile)
{
	int BitsPerPixel;
	bool SortFlag;
	GifByteType Buf[3];
	GifFilePrivateType * Private = static_cast<GifFilePrivateType *>(GifFile->Private);
	if(!IS_READABLE(Private)) {
		/* This file was NOT open for reading: */
		GifFile->Error = D_GIF_ERR_NOT_READABLE;
		return GIF_ERROR;
	}
	/* Put the screen descriptor into the file: */
	if(DGifGetWord(GifFile, &GifFile->SWidth) == GIF_ERROR || DGifGetWord(GifFile, &GifFile->SHeight) == GIF_ERROR)
		return GIF_ERROR;
	if(READ(GifFile, Buf, 3) != 3) {
		GifFile->Error = D_GIF_ERR_READ_FAILED;
		GifFreeMapObject(GifFile->SColorMap);
		GifFile->SColorMap = NULL;
		return GIF_ERROR;
	}
	GifFile->SColorResolution = (((Buf[0] & 0x70) + 1) >> 4) + 1;
	SortFlag = (Buf[0] & 0x08) != 0;
	BitsPerPixel = (Buf[0] & 0x07) + 1;
	GifFile->SBackGroundColor = Buf[1];
	GifFile->AspectByte = Buf[2];
	if(Buf[0] & 0x80) { /* Do we have global color map? */
		int i;
		GifFile->SColorMap = GifMakeMapObject(1 << BitsPerPixel, 0);
		if(GifFile->SColorMap == NULL) {
			GifFile->Error = D_GIF_ERR_NOT_ENOUGH_MEM;
			return GIF_ERROR;
		}
		/* Get the global color map: */
		GifFile->SColorMap->SortFlag = SortFlag;
		for(i = 0; i < GifFile->SColorMap->ColorCount; i++) {
			if(READ(GifFile, Buf, 3) != 3) {
				GifFreeMapObject(GifFile->SColorMap);
				GifFile->SColorMap = NULL;
				GifFile->Error = D_GIF_ERR_READ_FAILED;
				return GIF_ERROR;
			}
			GifFile->SColorMap->Colors[i].Set(Buf[0], Buf[1], Buf[2]);
		}
	}
	else {
		GifFile->SColorMap = NULL;
	}
	return GIF_OK;
}
// 
// This routine should be called before any attempt to read an image.
// 
int DGifGetRecordType(GifFileType * GifFile, GifRecordType* Type)
{
	GifByteType Buf;
	GifFilePrivateType * Private = static_cast<GifFilePrivateType *>(GifFile->Private);
	if(!IS_READABLE(Private)) {
		// This file was NOT open for reading: 
		GifFile->Error = D_GIF_ERR_NOT_READABLE;
		return GIF_ERROR;
	}
	if(READ(GifFile, &Buf, 1) != 1) {
		GifFile->Error = D_GIF_ERR_READ_FAILED;
		return GIF_ERROR;
	}
	switch(Buf) {
		case DESCRIPTOR_INTRODUCER: *Type = IMAGE_DESC_RECORD_TYPE; break;
		case EXTENSION_INTRODUCER:  *Type = EXTENSION_RECORD_TYPE; break;
		case TERMINATOR_INTRODUCER: *Type = TERMINATE_RECORD_TYPE; break;
		default:
		    *Type = UNDEFINED_RECORD_TYPE;
		    GifFile->Error = D_GIF_ERR_WRONG_RECORD;
		    return GIF_ERROR;
	}
	return GIF_OK;
}
//
// Setup the LZ decompression for this image:
//
static int DGifSetupDecompress(GifFileType * GifFile)
{
	int i, BitsPerPixel;
	GifByteType CodeSize;
	GifPrefixType * Prefix;
	GifFilePrivateType * Private = static_cast<GifFilePrivateType *>(GifFile->Private);
	READ(GifFile, &CodeSize, 1); /* Read Code size from file. */
	BitsPerPixel = CodeSize;
	Private->Buf[0] = 0; /* Input Buffer empty. */
	Private->BitsPerPixel = BitsPerPixel;
	Private->ClearCode = (1 << BitsPerPixel);
	Private->EOFCode = Private->ClearCode + 1;
	Private->RunningCode = Private->EOFCode + 1;
	Private->RunningBits = BitsPerPixel + 1; /* Number of bits per code. */
	Private->MaxCode1 = 1 << Private->RunningBits; /* Max. code + 1. */
	Private->StackPtr = 0; /* No pixels on the pixel stack. */
	Private->LastCode = NO_SUCH_CODE;
	Private->CrntShiftState = 0; /* No information in CrntShiftDWord. */
	Private->CrntShiftDWord = 0;
	Prefix = Private->Prefix;
	for(i = 0; i <= LZ_MAX_CODE; i++)
		Prefix[i] = NO_SUCH_CODE;
	return GIF_OK;
}
// 
// This routine should be called before any attempt to read an image.
// Note it is assumed the Image desc. header has been read.
// 
int DGifGetImageDesc(GifFileType * GifFile)
{
	uint BitsPerPixel;
	GifByteType Buf[3];
	GifFilePrivateType * Private = static_cast<GifFilePrivateType *>(GifFile->Private);
	GifSavedImage * sp;
	if(!IS_READABLE(Private)) {
		/* This file was NOT open for reading: */
		GifFile->Error = D_GIF_ERR_NOT_READABLE;
		return GIF_ERROR;
	}
	if(DGifGetWord(GifFile, &GifFile->Image.Left) == GIF_ERROR || DGifGetWord(GifFile, &GifFile->Image.Top) == GIF_ERROR ||
	    DGifGetWord(GifFile, &GifFile->Image.Width) == GIF_ERROR || DGifGetWord(GifFile, &GifFile->Image.Height) == GIF_ERROR)
		return GIF_ERROR;
	if(READ(GifFile, Buf, 1) != 1) {
		GifFile->Error = D_GIF_ERR_READ_FAILED;
		GifFreeMapObject(GifFile->Image.ColorMap);
		GifFile->Image.ColorMap = NULL;
		return GIF_ERROR;
	}
	BitsPerPixel = (Buf[0] & 0x07) + 1;
	GifFile->Image.Interlace = LOGIC(Buf[0] & 0x40);
	/* Setup the colormap */
	if(GifFile->Image.ColorMap) {
		GifFreeMapObject(GifFile->Image.ColorMap);
		GifFile->Image.ColorMap = NULL;
	}
	/* Does this image have local color map? */
	if(Buf[0] & 0x80) {
		uint i;
		GifFile->Image.ColorMap = GifMakeMapObject(1 << BitsPerPixel, 0);
		if(GifFile->Image.ColorMap == NULL) {
			GifFile->Error = D_GIF_ERR_NOT_ENOUGH_MEM;
			return GIF_ERROR;
		}
		/* Get the image local color map: */
		for(i = 0; i < (uint)GifFile->Image.ColorMap->ColorCount; i++) {
			if(READ(GifFile, Buf, 3) != 3) {
				GifFreeMapObject(GifFile->Image.ColorMap);
				GifFile->Error = D_GIF_ERR_READ_FAILED;
				GifFile->Image.ColorMap = NULL;
				return GIF_ERROR;
			}
			GifFile->Image.ColorMap->Colors[i].Set(Buf[0], Buf[1], Buf[2]);
		}
	}
	if(GifFile->SavedImages) {
		if((GifFile->SavedImages = static_cast<GifSavedImage *>(SAlloc::R(GifFile->SavedImages, sizeof(GifSavedImage) * (GifFile->ImageCount + 1)))) == NULL) {
			GifFile->Error = D_GIF_ERR_NOT_ENOUGH_MEM;
			return GIF_ERROR;
		}
	}
	else {
		if((GifFile->SavedImages = static_cast<GifSavedImage *>(SAlloc::M(sizeof(GifSavedImage)))) == NULL) {
			GifFile->Error = D_GIF_ERR_NOT_ENOUGH_MEM;
			return GIF_ERROR;
		}
	}
	sp = &GifFile->SavedImages[GifFile->ImageCount];
	memcpy(&sp->ImageDesc, &GifFile->Image, sizeof(GifImageDesc));
	if(GifFile->Image.ColorMap) {
		sp->ImageDesc.ColorMap = GifMakeMapObject(GifFile->Image.ColorMap->ColorCount, GifFile->Image.ColorMap->Colors);
		if(sp->ImageDesc.ColorMap == NULL) {
			GifFile->Error = D_GIF_ERR_NOT_ENOUGH_MEM;
			return GIF_ERROR;
		}
	}
	sp->RasterBits = NULL;
	sp->ExtensionBlockCount = 0;
	sp->ExtensionBlocks = NULL;
	GifFile->ImageCount++;
	Private->PixelCount = (long)GifFile->Image.Width * (long)GifFile->Image.Height;
	// Reset decompress algorithm parameters
	DGifSetupDecompress(GifFile);
	return GIF_OK;
}
// 
// Routine to trace the Prefixes linked list until we get a prefix which is
// not code, but a pixel value (less than ClearCode). Returns that pixel value.
// If image is defective, we might loop here forever, so we limit the loops to
// the maximum possible if image O.k. - LZ_MAX_CODE times.
// 
static int DGifGetPrefixChar(const GifPrefixType * Prefix, int Code, int ClearCode)
{
	int i = 0;
	while(Code > ClearCode && i++ <= LZ_MAX_CODE) {
		if(Code > LZ_MAX_CODE) {
			return NO_SUCH_CODE;
		}
		Code = Prefix[Code];
	}
	return Code;
}
// 
// This routines read one GIF data block at a time and buffers it internally
// so that the decompression routine could access it.
// The routine returns the next byte from its internal buffer (or read next
// block in if buffer empty) and returns GIF_OK if succesful.
// 
static int DGifBufferedInput(GifFileType * GifFile, GifByteType * Buf, GifByteType * NextByte)
{
	if(Buf[0] == 0) {
		/* Needs to read the next buffer - this one is empty: */
		if(READ(GifFile, Buf, 1) != 1) {
			GifFile->Error = D_GIF_ERR_READ_FAILED;
			return GIF_ERROR;
		}
		/* There shouldn't be any empty data blocks here as the LZW spec
		 * says the LZW termination code should come first.  Therefore we
		 * shouldn't be inside this routine at that point.
		 */
		if(Buf[0] == 0) {
			GifFile->Error = D_GIF_ERR_IMAGE_DEFECT;
			return GIF_ERROR;
		}
		/* There shouldn't be any empty data blocks here as the LZW spec
		 * says the LZW termination code should come first.  Therefore we
		 * shouldn't be inside this routine at that point.
		 */
		if(Buf[0] == 0) {
			GifFile->Error = D_GIF_ERR_IMAGE_DEFECT;
			return GIF_ERROR;
		}
		if(READ(GifFile, &Buf[1], Buf[0]) != Buf[0]) {
			GifFile->Error = D_GIF_ERR_READ_FAILED;
			return GIF_ERROR;
		}
		*NextByte = Buf[1];
		Buf[1] = 2; /* We use now the second place as last char read! */
		Buf[0]--;
	}
	else {
		*NextByte = Buf[Buf[1]++];
		Buf[0]--;
	}
	return GIF_OK;
}
// 
// The LZ decompression input routine:
// This routine is responsable for the decompression of the bit stream from
// 8 bits (bytes) packets, into the real codes.
// Returns GIF_OK if read successfully.
// 
static int FASTCALL DGifDecompressInput(GifFileType * GifFile, int * Code)
{
	static const ushort CodeMasks[] = {
		0x0000, 0x0001, 0x0003, 0x0007,
		0x000f, 0x001f, 0x003f, 0x007f,
		0x00ff, 0x01ff, 0x03ff, 0x07ff,
		0x0fff
	};
	GifFilePrivateType * Private = static_cast<GifFilePrivateType *>(GifFile->Private);
	GifByteType NextByte;
	// The image can't contain more than LZ_BITS per code. 
	if(Private->RunningBits > LZ_BITS) {
		GifFile->Error = D_GIF_ERR_IMAGE_DEFECT;
		return GIF_ERROR;
	}
	while(Private->CrntShiftState < Private->RunningBits) {
		// Needs to get more bytes from input stream for next code: 
		if(DGifBufferedInput(GifFile, Private->Buf, &NextByte) == GIF_ERROR) {
			return GIF_ERROR;
		}
		Private->CrntShiftDWord |= ((ulong)NextByte) << Private->CrntShiftState;
		Private->CrntShiftState += 8;
	}
	*Code = Private->CrntShiftDWord & CodeMasks[Private->RunningBits];
	Private->CrntShiftDWord >>= Private->RunningBits;
	Private->CrntShiftState -= Private->RunningBits;
	// 
	// If code cannot fit into RunningBits bits, must raise its size. Note
	// however that codes above 4095 are used for special signaling.
	// If we're using LZ_BITS bits already and we're at the max code, just
	// keep using the table as it is, don't increment Private->RunningCode.
	// 
	if(Private->RunningCode < LZ_MAX_CODE + 2 && ++Private->RunningCode > Private->MaxCode1 &&
	    Private->RunningBits < LZ_BITS) {
		Private->MaxCode1 <<= 1;
		Private->RunningBits++;
	}
	return GIF_OK;
}
// 
// The LZ decompression routine:
// This version decompress the given GIF file into Line of length LineLen.
// This routine can be called few times (one per scan line, for example), in
// order the complete the whole image.
// 
static int DGifDecompressLine(GifFileType * GifFile, GifPixelType * Line, int LineLen)
{
	int i = 0;
	int j, CrntCode, EOFCode, ClearCode, CrntPrefix, LastCode, StackPtr;
	GifByteType * Stack, * Suffix;
	GifPrefixType * Prefix;
	GifFilePrivateType * Private = static_cast<GifFilePrivateType *>(GifFile->Private);
	StackPtr = Private->StackPtr;
	Prefix = Private->Prefix;
	Suffix = Private->Suffix;
	Stack = Private->Stack;
	EOFCode = Private->EOFCode;
	ClearCode = Private->ClearCode;
	LastCode = Private->LastCode;
	if(StackPtr > LZ_MAX_CODE) {
		return GIF_ERROR;
	}
	if(StackPtr != 0) {
		// Let pop the stack off before continueing to read the GIF file: 
		while(StackPtr != 0 && i < LineLen)
			Line[i++] = Stack[--StackPtr];
	}
	while(i < LineLen) { // Decode LineLen items. 
		if(DGifDecompressInput(GifFile, &CrntCode) == GIF_ERROR)
			return GIF_ERROR;
		if(CrntCode == EOFCode) {
			/* Note however that usually we will not be here as we will stop
			 * decoding as soon as we got all the pixel, or EOF code will
			 * not be read at all, and DGifGetLine/Pixel clean everything.  */
			GifFile->Error = D_GIF_ERR_EOF_TOO_SOON;
			return GIF_ERROR;
		}
		else if(CrntCode == ClearCode) {
			/* We need to start over again: */
			for(j = 0; j <= LZ_MAX_CODE; j++)
				Prefix[j] = NO_SUCH_CODE;
			Private->RunningCode = Private->EOFCode + 1;
			Private->RunningBits = Private->BitsPerPixel + 1;
			Private->MaxCode1 = 1 << Private->RunningBits;
			LastCode = Private->LastCode = NO_SUCH_CODE;
		}
		else {
			/* Its regular code - if in pixel range simply add it to output
			 * stream, otherwise trace to codes linked list until the prefix
			 * is in pixel range: */
			if(CrntCode < ClearCode) {
				/* This is simple - its pixel scalar, so add it to output: */
				Line[i++] = CrntCode;
			}
			else {
				/* Its a code to needed to be traced: trace the linked list
				 * until the prefix is a pixel, while pushing the suffix
				 * pixels on our stack. If we done, pop the stack in reverse
				 * (thats what stack is good for!) order to output.  */
				if(Prefix[CrntCode] == NO_SUCH_CODE) {
					/* Only allowed if CrntCode is exactly the running code:
					 * In that case CrntCode = XXXCode, CrntCode or the
					 * prefix code is last code and the suffix char is
					 * exactly the prefix of last code! */
					if(CrntCode == Private->RunningCode - 2) {
						CrntPrefix = LastCode;
						Suffix[Private->RunningCode - 2] = Stack[StackPtr++] = DGifGetPrefixChar(Prefix, LastCode, ClearCode);
					}
					else {
						GifFile->Error = D_GIF_ERR_IMAGE_DEFECT;
						return GIF_ERROR;
					}
				}
				else
					CrntPrefix = CrntCode;

				/* Now (if image is O.K.) we should not get a NO_SUCH_CODE
				 * during the trace. As we might loop forever, in case of
				 * defective image, we use StackPtr as loop counter and stop
				 * before overflowing Stack[]. */
				while(StackPtr < LZ_MAX_CODE && CrntPrefix > ClearCode && CrntPrefix <= LZ_MAX_CODE) {
					Stack[StackPtr++] = Suffix[CrntPrefix];
					CrntPrefix = Prefix[CrntPrefix];
				}
				if(StackPtr >= LZ_MAX_CODE || CrntPrefix > LZ_MAX_CODE) {
					GifFile->Error = D_GIF_ERR_IMAGE_DEFECT;
					return GIF_ERROR;
				}
				/* Push the last character on stack: */
				Stack[StackPtr++] = CrntPrefix;
				/* Now lets pop all the stack into output: */
				while(StackPtr != 0 && i < LineLen)
					Line[i++] = Stack[--StackPtr];
			}
			if(LastCode != NO_SUCH_CODE) {
				Prefix[Private->RunningCode - 2] = LastCode;
				if(CrntCode == Private->RunningCode - 2) {
					/* Only allowed if CrntCode is exactly the running code:
					 * In that case CrntCode = XXXCode, CrntCode or the
					 * prefix code is last code and the suffix char is
					 * exactly the prefix of last code! */
					Suffix[Private->RunningCode - 2] = DGifGetPrefixChar(Prefix, LastCode, ClearCode);
				}
				else {
					Suffix[Private->RunningCode - 2] = DGifGetPrefixChar(Prefix, CrntCode, ClearCode);
				}
			}
			LastCode = CrntCode;
		}
	}
	Private->LastCode = LastCode;
	Private->StackPtr = StackPtr;
	return GIF_OK;
}
// 
// Get one full scanned line (Line) of length LineLen from GIF file.
// 
int DGifGetLine(GifFileType * GifFile, GifPixelType * Line, int LineLen)
{
	GifByteType * Dummy;
	GifFilePrivateType * Private = static_cast<GifFilePrivateType *>(GifFile->Private);
	if(!IS_READABLE(Private)) {
		/* This file was NOT open for reading: */
		GifFile->Error = D_GIF_ERR_NOT_READABLE;
		return GIF_ERROR;
	}
	SETIFZ(LineLen, GifFile->Image.Width);
	if((Private->PixelCount -= LineLen) > 0xffff0000UL) {
		GifFile->Error = D_GIF_ERR_DATA_TOO_BIG;
		return GIF_ERROR;
	}
	if(DGifDecompressLine(GifFile, Line, LineLen) == GIF_OK) {
		if(Private->PixelCount == 0) {
			/* We probably won't be called any more, so let's clean up
			 * everything before we return: need to flush out all the
			 * rest of image until an empty block (size 0)
			 * detected. We use GetCodeNext.
			 */
			do {
				if(DGifGetCodeNext(GifFile, &Dummy) == GIF_ERROR)
					return GIF_ERROR;
			} while(Dummy);
		}
		return GIF_OK;
	}
	else
		return GIF_ERROR;
}
// 
// Put one pixel (Pixel) into GIF file.
// 
int DGifGetPixel(GifFileType * GifFile, GifPixelType Pixel)
{
	GifByteType * Dummy;
	GifFilePrivateType * Private = static_cast<GifFilePrivateType *>(GifFile->Private);
	if(!IS_READABLE(Private)) {
		/* This file was NOT open for reading: */
		GifFile->Error = D_GIF_ERR_NOT_READABLE;
		return GIF_ERROR;
	}
	else if(--Private->PixelCount > 0xffff0000UL) {
		GifFile->Error = D_GIF_ERR_DATA_TOO_BIG;
		return GIF_ERROR;
	}
	else if(DGifDecompressLine(GifFile, &Pixel, 1) == GIF_OK) {
		if(Private->PixelCount == 0) {
			/* We probably won't be called any more, so let's clean up
			 * everything before we return: need to flush out all the
			 * rest of image until an empty block (size 0)
			 * detected. We use GetCodeNext.
			 */
			do {
				if(DGifGetCodeNext(GifFile, &Dummy) == GIF_ERROR)
					return GIF_ERROR;
			} while(Dummy);
		}
		return GIF_OK;
	}
	else
		return GIF_ERROR;
}
// 
// Get an extension block (see GIF manual) from GIF file. This routine only
// returns the first data block, and DGifGetExtensionNext should be called
// after this one until NULL extension is returned.
// The Extension should NOT be freed by the user (not dynamically allocated).
// Note it is assumed the Extension description header has been read.
// 
int DGifGetExtension(GifFileType * GifFile, int * ExtCode, GifByteType ** Extension)
{
	GifByteType Buf;
	GifFilePrivateType * Private = static_cast<GifFilePrivateType *>(GifFile->Private);
	if(!IS_READABLE(Private)) {
		/* This file was NOT open for reading: */
		GifFile->Error = D_GIF_ERR_NOT_READABLE;
		return GIF_ERROR;
	}
	else if(READ(GifFile, &Buf, 1) != 1) {
		GifFile->Error = D_GIF_ERR_READ_FAILED;
		return GIF_ERROR;
	}
	else {
		*ExtCode = Buf;
		return DGifGetExtensionNext(GifFile, Extension);
	}
}
// 
// Get a following extension block (see GIF manual) from GIF file. This
// routine should be called until NULL Extension is returned.
// The Extension should NOT be freed by the user (not dynamically allocated).
// 
int DGifGetExtensionNext(GifFileType * GifFile, GifByteType ** Extension)
{
	GifByteType Buf;
	GifFilePrivateType * Private = static_cast<GifFilePrivateType *>(GifFile->Private);
	if(READ(GifFile, &Buf, 1) != 1) {
		GifFile->Error = D_GIF_ERR_READ_FAILED;
		return GIF_ERROR;
	}
	if(Buf > 0) {
		*Extension = Private->Buf; /* Use private unused buffer. */
		(*Extension)[0] = Buf; /* Pascal strings notation (pos. 0 is len.). */
		/* coverity[tainted_data] */
		if(READ(GifFile, &((*Extension)[1]), Buf) != Buf) {
			GifFile->Error = D_GIF_ERR_READ_FAILED;
			return GIF_ERROR;
		}
	}
	else
		*Extension = NULL;
	return GIF_OK;
}
// 
// Extract a Graphics Control Block from raw extension data
// 
int DGifExtensionToGCB(const size_t GifExtensionLength, const GifByteType * GifExtension, GraphicsControlBlock * GCB)
{
	if(GifExtensionLength != 4) {
		return GIF_ERROR;
	}
	else {
		GCB->DisposalMode = (GifExtension[0] >> 2) & 0x07;
		GCB->UserInputFlag = (GifExtension[0] & 0x02) != 0;
		GCB->DelayTime = UNSIGNED_LITTLE_ENDIAN(GifExtension[1], GifExtension[2]);
		GCB->TransparentColor = (GifExtension[0] & 0x01) ? (int)GifExtension[3] : NO_TRANSPARENT_COLOR;
		return GIF_OK;
	}
}
// 
// Extract the Graphics Control Block for a saved image, if it exists.
// 
int DGifSavedExtensionToGCB(GifFileType * GifFile, int ImageIndex, GraphicsControlBlock * GCB)
{
	int i;
	if(ImageIndex < 0 || ImageIndex > GifFile->ImageCount - 1)
		return GIF_ERROR;
	GCB->DisposalMode = DISPOSAL_UNSPECIFIED;
	GCB->UserInputFlag = false;
	GCB->DelayTime = 0;
	GCB->TransparentColor = NO_TRANSPARENT_COLOR;
	for(i = 0; i < GifFile->SavedImages[ImageIndex].ExtensionBlockCount; i++) {
		ExtensionBlock * ep = &GifFile->SavedImages[ImageIndex].ExtensionBlocks[i];
		if(ep->Function == GRAPHICS_EXT_FUNC_CODE)
			return DGifExtensionToGCB(ep->ByteCount, ep->Bytes, GCB);
	}
	return GIF_ERROR;
}
//
// This routine should be called last, to close the GIF file.
//
int DGifCloseFile(GifFileType * GifFile)
{
	GifFilePrivateType * Private;
	if(GifFile == NULL || GifFile->Private == NULL)
		return GIF_ERROR;
	if(GifFile->Image.ColorMap) {
		GifFreeMapObject(GifFile->Image.ColorMap);
		GifFile->Image.ColorMap = NULL;
	}
	if(GifFile->SColorMap) {
		GifFreeMapObject(GifFile->SColorMap);
		GifFile->SColorMap = NULL;
	}
	if(GifFile->SavedImages) {
		GifFreeSavedImages(GifFile);
		GifFile->SavedImages = NULL;
	}
	GifFreeExtensions(&GifFile->ExtensionBlockCount, &GifFile->ExtensionBlocks);
	Private = static_cast<GifFilePrivateType *>(GifFile->Private);
	if(!IS_READABLE(Private)) {
		/* This file was NOT open for reading: */
		GifFile->Error = D_GIF_ERR_NOT_READABLE;
		return GIF_ERROR;
	}
	if(Private->File && (fclose(Private->File) != 0)) {
		GifFile->Error = D_GIF_ERR_CLOSE_FAILED;
		return GIF_ERROR;
	}
	SAlloc::F((char *)GifFile->Private);
	//
	// Without the #ifndef, we get spurious warnings because Coverity mistakenly
	// thinks the GIF structure is freed on an error return.
	//
#ifndef __COVERITY__
	SAlloc::F(GifFile);
#endif /* __COVERITY__ */
	return GIF_OK;
}
// 
// Get the image code in compressed form.  This routine can be called if the
// information needed to be piped out as is. Obviously this is much faster
// than decoding and encoding again. This routine should be followed by calls
// to DGifGetCodeNext, until NULL block is returned.
// The block should NOT be freed by the user (not dynamically allocated).
// 
int DGifGetCode(GifFileType * GifFile, int * CodeSize, GifByteType ** CodeBlock)
{
	GifFilePrivateType * Private = static_cast<GifFilePrivateType *>(GifFile->Private);
	if(!IS_READABLE(Private)) {
		/* This file was NOT open for reading: */
		GifFile->Error = D_GIF_ERR_NOT_READABLE;
		return GIF_ERROR;
	}
	*CodeSize = Private->BitsPerPixel;
	return DGifGetCodeNext(GifFile, CodeBlock);
}
// 
// Continue to get the image code in compressed form. This routine should be
// called until NULL block is returned.
// The block should NOT be freed by the user (not dynamically allocated).
// 
int DGifGetCodeNext(GifFileType * GifFile, GifByteType ** CodeBlock)
{
	GifByteType Buf;
	GifFilePrivateType * Private = static_cast<GifFilePrivateType *>(GifFile->Private);

	/* coverity[tainted_data_argument] */
	if(READ(GifFile, &Buf, 1) != 1) {
		GifFile->Error = D_GIF_ERR_READ_FAILED;
		return GIF_ERROR;
	}
	/* coverity[lower_bounds] */
	if(Buf > 0) {
		*CodeBlock = Private->Buf; /* Use private unused buffer. */
		(*CodeBlock)[0] = Buf; /* Pascal strings notation (pos. 0 is len.). */
		/* coverity[tainted_data] */
		if(READ(GifFile, &((*CodeBlock)[1]), Buf) != Buf) {
			GifFile->Error = D_GIF_ERR_READ_FAILED;
			return GIF_ERROR;
		}
	}
	else {
		*CodeBlock = NULL;
		Private->Buf[0] = 0; /* Make sure the buffer is empty! */
		Private->PixelCount = 0; /* And local info. indicate image read. */
	}

	return GIF_OK;
}
// 
// Interface for accessing the LZ codes directly. Set Code to the real code
// (12bits), or to -1 if EOF code is returned.
// 
int DGifGetLZCodes(GifFileType * GifFile, int * Code)
{
	GifFilePrivateType * Private = static_cast<GifFilePrivateType *>(GifFile->Private);
	if(!IS_READABLE(Private)) {
		// This file was NOT open for reading: 
		GifFile->Error = D_GIF_ERR_NOT_READABLE;
		return GIF_ERROR;
	}
	else if(DGifDecompressInput(GifFile, Code) == GIF_ERROR)
		return GIF_ERROR;
	else {
		if(*Code == Private->EOFCode) {
			GifByteType * p_code_block;
			// Skip rest of codes (hopefully only NULL terminating block): 
			do {
				if(DGifGetCodeNext(GifFile, &p_code_block) == GIF_ERROR)
					return GIF_ERROR;
			} while(p_code_block);
			*Code = -1;
		}
		else if(*Code == Private->ClearCode) {
			// We need to start over again: 
			Private->RunningCode = Private->EOFCode + 1;
			Private->RunningBits = Private->BitsPerPixel + 1;
			Private->MaxCode1 = 1 << Private->RunningBits;
		}
		return GIF_OK;
	}
}
// 
// This routine reads an entire GIF into core, hanging all its state info off
// the GifFileType pointer.  Call DGifOpenFileName() or DGifOpenFileHandle()
// first to initialize I/O.  Its inverse is EGifSpew().
// 
int DGifSlurp(GifFileType * pGifFile)
{
	size_t image_size;
	GifRecordType RecordType;
	GifSavedImage * p_sp;
	GifByteType * p_ext_data;
	int ExtFunction;
	pGifFile->ExtensionBlocks = NULL;
	pGifFile->ExtensionBlockCount = 0;
	do {
		if(DGifGetRecordType(pGifFile, &RecordType) == GIF_ERROR)
			return GIF_ERROR;
		switch(RecordType) {
			case IMAGE_DESC_RECORD_TYPE:
			    if(DGifGetImageDesc(pGifFile) == GIF_ERROR)
				    return GIF_ERROR;
			    p_sp = &pGifFile->SavedImages[pGifFile->ImageCount - 1];
			    // Allocate memory for the image 
			    if(p_sp->ImageDesc.Width < 0 && p_sp->ImageDesc.Height < 0 && p_sp->ImageDesc.Width > (INT_MAX / p_sp->ImageDesc.Height)) {
				    return GIF_ERROR;
			    }
			    image_size = p_sp->ImageDesc.Width * p_sp->ImageDesc.Height;
			    if(image_size > (SIZE_MAX / sizeof(GifPixelType))) {
				    return GIF_ERROR;
			    }
			    p_sp->RasterBits = static_cast<uint8 *>(SAlloc::M(image_size * sizeof(GifPixelType)));
			    if(p_sp->RasterBits == NULL) {
				    return GIF_ERROR;
			    }
			    if(p_sp->ImageDesc.Interlace) {
				    int i, j;
				    //
					// The way an interlaced image should be read - offsets and jumps...
					//
				    static const int InterlacedOffset[] = { 0, 4, 2, 1 };
				    static const int InterlacedJumps[] = { 8, 8, 4, 2 };
				    // Need to perform 4 passes on the image 
				    for(i = 0; i < 4; i++)
					    for(j = InterlacedOffset[i]; j < p_sp->ImageDesc.Height; j += InterlacedJumps[i]) {
						    if(DGifGetLine(pGifFile,
							    p_sp->RasterBits+j*p_sp->ImageDesc.Width,
							    p_sp->ImageDesc.Width) == GIF_ERROR)
							    return GIF_ERROR;
					    }
			    }
			    else {
				    if(DGifGetLine(pGifFile, p_sp->RasterBits, image_size)==GIF_ERROR)
					    return GIF_ERROR;
			    }
			    if(pGifFile->ExtensionBlocks) {
				    p_sp->ExtensionBlocks = pGifFile->ExtensionBlocks;
				    p_sp->ExtensionBlockCount = pGifFile->ExtensionBlockCount;
				    pGifFile->ExtensionBlocks = NULL;
				    pGifFile->ExtensionBlockCount = 0;
			    }
			    break;
			case EXTENSION_RECORD_TYPE:
			    if(DGifGetExtension(pGifFile, &ExtFunction, &p_ext_data) == GIF_ERROR)
				    return GIF_ERROR;
			    // Create an extension block with our data 
			    if(GifAddExtensionBlock(&pGifFile->ExtensionBlockCount, &pGifFile->ExtensionBlocks, ExtFunction, p_ext_data[0], &p_ext_data[1]) == GIF_ERROR)
				    return GIF_ERROR;
			    while(p_ext_data) {
				    if(DGifGetExtensionNext(pGifFile, &p_ext_data) == GIF_ERROR)
					    return GIF_ERROR;
				    /* Continue the extension block */
				    if(p_ext_data)
					    if(GifAddExtensionBlock(&pGifFile->ExtensionBlockCount, &pGifFile->ExtensionBlocks, CONTINUE_EXT_FUNC_CODE, p_ext_data[0], &p_ext_data[1]) == GIF_ERROR)
						    return GIF_ERROR;
			    }
			    break;
			case TERMINATE_RECORD_TYPE:
			    break;
			default: /* Should be trapped by DGifGetRecordType */
			    break;
		}
	} while(RecordType != TERMINATE_RECORD_TYPE);
	return GIF_OK;
}
//
// All writes to the GIF should go through this.
//
static int FASTCALL InternalWrite(GifFileType * GifFileOut, const uint8 * buf, size_t len)
{
	GifFilePrivateType * p_private = static_cast<GifFilePrivateType *>(GifFileOut->Private);
	return p_private->Write ? p_private->Write(GifFileOut, buf, len) : fwrite(buf, 1, len, p_private->File);
}
// 
// This routines buffers the given characters until 255 characters are ready
// to be output. If Code is equal to -1 the buffer is flushed (EOF).
// The buffer is Dumped with first byte as its size, as GIF format requires.
// Returns GIF_OK if written successfully.
// 
static int EGifBufferedOutput(GifFileType * GifFile, GifByteType * Buf, int c)
{
	if(c == FLUSH_OUTPUT) {
		// Flush everything out
		if(Buf[0] != 0 && InternalWrite(GifFile, Buf, Buf[0] + 1) != (uint)(Buf[0] + 1)) {
			GifFile->Error = E_GIF_ERR_WRITE_FAILED;
			return GIF_ERROR;
		}
		// Mark end of compressed data, by an empty block (see GIF doc): 
		Buf[0] = 0;
		if(InternalWrite(GifFile, Buf, 1) != 1) {
			GifFile->Error = E_GIF_ERR_WRITE_FAILED;
			return GIF_ERROR;
		}
	}
	else {
		if(Buf[0] == 255) {
			/* Dump out this buffer - it is full: */
			if(InternalWrite(GifFile, Buf, Buf[0] + 1) != (uint)(Buf[0] + 1)) {
				GifFile->Error = E_GIF_ERR_WRITE_FAILED;
				return GIF_ERROR;
			}
			Buf[0] = 0;
		}
		Buf[++Buf[0]] = c;
	}
	return GIF_OK;
}
// 
// The LZ compression output routine:
// This routine is responsible for the compression of the bit stream into
// 8 bits (bytes) packets.
// Returns GIF_OK if written successfully.
// 
static int EGifCompressOutput(GifFileType * GifFile, int Code)
{
	GifFilePrivateType * Private = static_cast<GifFilePrivateType *>(GifFile->Private);
	int retval = GIF_OK;
	if(Code == FLUSH_OUTPUT) {
		while(Private->CrntShiftState > 0) {
			// Get Rid of what is left in DWord, and flush it
			if(EGifBufferedOutput(GifFile, Private->Buf, Private->CrntShiftDWord & 0xff) == GIF_ERROR)
				retval = GIF_ERROR;
			Private->CrntShiftDWord >>= 8;
			Private->CrntShiftState -= 8;
		}
		Private->CrntShiftState = 0; /* For next time. */
		if(EGifBufferedOutput(GifFile, Private->Buf, FLUSH_OUTPUT) == GIF_ERROR)
			retval = GIF_ERROR;
	}
	else {
		Private->CrntShiftDWord |= ((long)Code) << Private->CrntShiftState;
		Private->CrntShiftState += Private->RunningBits;
		while(Private->CrntShiftState >= 8) {
			// Dump out full bytes: 
			if(EGifBufferedOutput(GifFile, Private->Buf, Private->CrntShiftDWord & 0xff) == GIF_ERROR)
				retval = GIF_ERROR;
			Private->CrntShiftDWord >>= 8;
			Private->CrntShiftState -= 8;
		}
	}
	// If code cannt fit into RunningBits bits, must raise its size. Note 
	// however that codes above 4095 are used for special signaling.      
	if(Private->RunningCode >= Private->MaxCode1 && Code <= 4095) {
		Private->MaxCode1 = 1 << ++Private->RunningBits;
	}
	return retval;
}
//
// Setup the LZ compression for this image:
//
static int EGifSetupCompress(GifFileType * GifFile)
{
	int BitsPerPixel;
	GifByteType Buf;
	GifFilePrivateType * Private = static_cast<GifFilePrivateType *>(GifFile->Private);
	// Test and see what color map to use, and from it # bits per pixel: 
	if(GifFile->Image.ColorMap)
		BitsPerPixel = GifFile->Image.ColorMap->BitsPerPixel;
	else if(GifFile->SColorMap)
		BitsPerPixel = GifFile->SColorMap->BitsPerPixel;
	else {
		GifFile->Error = E_GIF_ERR_NO_COLOR_MAP;
		return GIF_ERROR;
	}
	Buf = BitsPerPixel = (BitsPerPixel < 2 ? 2 : BitsPerPixel);
	InternalWrite(GifFile, &Buf, 1); /* Write the Code size to file. */
	Private->Buf[0] = 0; /* Nothing was output yet. */
	Private->BitsPerPixel = BitsPerPixel;
	Private->ClearCode = (1 << BitsPerPixel);
	Private->EOFCode = Private->ClearCode + 1;
	Private->RunningCode = Private->EOFCode + 1;
	Private->RunningBits = BitsPerPixel + 1; /* Number of bits per code. */
	Private->MaxCode1 = 1 << Private->RunningBits; /* Max. code + 1. */
	Private->CrntCode = FIRST_CODE; /* Signal that this is first one! */
	Private->CrntShiftState = 0; /* No information in CrntShiftDWord. */
	Private->CrntShiftDWord = 0;
	// Clear hash table and send Clear to make sure the decoder do the same
	_ClearHashTable(Private->HashTable);
	if(EGifCompressOutput(GifFile, Private->ClearCode) == GIF_ERROR) {
		GifFile->Error = E_GIF_ERR_DISK_IS_FULL;
		return GIF_ERROR;
	}
	return GIF_OK;
}
// 
// The LZ compression routine:
// This version compresses the given buffer Line of length LineLen.
// This routine can be called a few times (one per scan line, for example), in
// order to complete the whole image.
// 
static int EGifCompressLine(GifFileType * GifFile, GifPixelType * Line, int LineLen)
{
	int i = 0, CrntCode, NewCode;
	ulong NewKey;
	GifPixelType Pixel;
	GifHashTableType * HashTable;
	GifFilePrivateType * Private = static_cast<GifFilePrivateType *>(GifFile->Private);
	HashTable = Private->HashTable;
	if(Private->CrntCode == FIRST_CODE) /* Its first time! */
		CrntCode = Line[i++];
	else
		CrntCode = Private->CrntCode;  /* Get last code in compression. */
	while(i < LineLen) { /* Decode LineLen items. */
		Pixel = Line[i++]; /* Get next pixel from stream. */
		/* Form a new unique key to search hash table for the code combines
		 * CrntCode as Prefix string with Pixel as postfix char.
		 */
		NewKey = (((uint32)CrntCode) << 8) + Pixel;
		if((NewCode = _ExistsHashTable(HashTable, NewKey)) >= 0) {
			/* This Key is already there, or the string is old one, so
			 * simple take new code as our CrntCode:
			 */
			CrntCode = NewCode;
		}
		else {
			/* Put it in hash table, output the prefix code, and make our
			 * CrntCode equal to Pixel.
			 */
			if(EGifCompressOutput(GifFile, CrntCode) == GIF_ERROR) {
				GifFile->Error = E_GIF_ERR_DISK_IS_FULL;
				return GIF_ERROR;
			}
			CrntCode = Pixel;

			/* If however the HashTable if full, we send a clear first and
			 * Clear the hash table.
			 */
			if(Private->RunningCode >= LZ_MAX_CODE) {
				/* Time to do some clearance: */
				if(EGifCompressOutput(GifFile, Private->ClearCode) == GIF_ERROR) {
					GifFile->Error = E_GIF_ERR_DISK_IS_FULL;
					return GIF_ERROR;
				}
				Private->RunningCode = Private->EOFCode + 1;
				Private->RunningBits = Private->BitsPerPixel + 1;
				Private->MaxCode1 = 1 << Private->RunningBits;
				_ClearHashTable(HashTable);
			}
			else {
				/* Put this unique key with its relative Code in hash table: */
				_InsertHashTable(HashTable, NewKey, Private->RunningCode++);
			}
		}
	}
	/* Preserve the current state of the compression algorithm: */
	Private->CrntCode = CrntCode;
	if(Private->PixelCount == 0) {
		/* We are done - output last Code and flush output buffers: */
		if(EGifCompressOutput(GifFile, CrntCode) == GIF_ERROR) {
			GifFile->Error = E_GIF_ERR_DISK_IS_FULL;
			return GIF_ERROR;
		}
		if(EGifCompressOutput(GifFile, Private->EOFCode) == GIF_ERROR) {
			GifFile->Error = E_GIF_ERR_DISK_IS_FULL;
			return GIF_ERROR;
		}
		if(EGifCompressOutput(GifFile, FLUSH_OUTPUT) == GIF_ERROR) {
			GifFile->Error = E_GIF_ERR_DISK_IS_FULL;
			return GIF_ERROR;
		}
	}
	return GIF_OK;
}
//
// Put 2 bytes (a word) into the given file in little-endian order:
//
static int FASTCALL EGifPutWord(int Word, GifFileType * GifFile)
{
	uint8 c[2];
	c[0] = LOBYTE(Word);
	c[1] = HIBYTE(Word);
	return (InternalWrite(GifFile, c, 2) == 2) ? GIF_OK : GIF_ERROR;
}
// 
// Open a new GIF file for write, specified by name. If TestExistance then
// if the file exists this routines fails (returns NULL).
// Returns a dynamically allocated GifFileType pointer which serves as the GIF
// info record. The Error member is cleared if successful.
// 
GifFileType * EGifOpenFileName(const char * FileName, const bool TestExistence, int * Error)
{
	GifFileType * GifFile = 0;
	int    FileHandle = -1;
	if(TestExistence)
		FileHandle = open(FileName, O_WRONLY|O_CREAT|O_EXCL, S_IREAD|S_IWRITE);
	else
		FileHandle = open(FileName, O_WRONLY|O_CREAT|O_TRUNC, S_IREAD|S_IWRITE);
	if(FileHandle == -1) {
		ASSIGN_PTR(Error, E_GIF_ERR_OPEN_FAILED);
	}
	else {
		GifFile = EGifOpenFileHandle(FileHandle, Error);
		if(!GifFile)
			close(FileHandle);
	}
	return GifFile;
}
// 
// Update a new GIF file, given its file handle, which must be opened for write in binary mode.
// Returns dynamically allocated a GifFileType pointer which serves as the GIF info record.
// Only fails on a memory allocation error.
// 
GifFileType * EGifOpenFileHandle(const int FileHandle, int * Error)
{
	GifFilePrivateType * Private;
	FILE * f = 0;
	GifFileType * GifFile = static_cast<GifFileType *>(SAlloc::M(sizeof(GifFileType)));
	if(GifFile) {
		memzero(GifFile, sizeof(GifFileType));
		Private = static_cast<GifFilePrivateType *>(SAlloc::M(sizeof(GifFilePrivateType)));
		if(Private == NULL) {
			SAlloc::F(GifFile);
			ASSIGN_PTR(Error, E_GIF_ERR_NOT_ENOUGH_MEM);
			return NULL;
		}
		if((Private->HashTable = _InitHashTable()) == NULL) {
			SAlloc::F(GifFile);
			SAlloc::F(Private);
			ASSIGN_PTR(Error, E_GIF_ERR_NOT_ENOUGH_MEM);
			return NULL;
		}
#ifdef _WIN32
		_setmode(FileHandle, O_BINARY); /* Make sure it is in binary mode. */
#endif /* _WIN32 */
		f = fdopen(FileHandle, "wb"); /* Make it into a stream: */
		GifFile->Private = Private;
		Private->FileHandle = FileHandle;
		Private->File = f;
		Private->FileState = FILE_STATE_WRITE;
		Private->Write = (OutputFunc)0; /* No user write routine (MRB) */
		GifFile->UserData = NULL; // No user write handle (MRB) 
		GifFile->Error = 0;
	}
	return GifFile;
}
// 
// Output constructor that takes user supplied output function.
// Basically just a copy of EGifOpenFileHandle. (MRB)
// 
GifFileType * EGifOpen(void * userData, OutputFunc writeFunc, int * pError)
{
	GifFilePrivateType * Private;
	GifFileType * GifFile = static_cast<GifFileType *>(SAlloc::M(sizeof(GifFileType)));
	if(GifFile == NULL) {
		ASSIGN_PTR(pError, E_GIF_ERR_NOT_ENOUGH_MEM);
		return NULL;
	}
	memzero(GifFile, sizeof(GifFileType));
	Private = static_cast<GifFilePrivateType *>(SAlloc::M(sizeof(GifFilePrivateType)));
	if(Private == NULL) {
		SAlloc::F(GifFile);
		ASSIGN_PTR(pError, E_GIF_ERR_NOT_ENOUGH_MEM);
		return NULL;
	}
	Private->HashTable = _InitHashTable();
	if(Private->HashTable == NULL) {
		SAlloc::F(GifFile);
		SAlloc::F(Private);
		ASSIGN_PTR(pError, E_GIF_ERR_NOT_ENOUGH_MEM);
		return NULL;
	}
	GifFile->Private = Private;
	Private->FileHandle = 0;
	Private->File = 0;
	Private->FileState = FILE_STATE_WRITE;
	Private->Write = writeFunc; /* User write routine (MRB) */
	GifFile->UserData = userData; /* User write handle (MRB) */
	Private->gif89 = false; /* initially, write GIF87 */
	GifFile->Error = 0;
	return GifFile;
}
// 
// Routine to compute the GIF version that will be written on output.
// 
char * EGifGetGifVersion(GifFileType * pGifFile)
{
	GifFilePrivateType * p_private = static_cast<GifFilePrivateType *>(pGifFile->Private);
	int i, j;
	/*
	 * Bulletproofing - always write GIF89 if we need to.
	 * Note, we don't clear the gif89 flag here because
	 * users of the sequential API might have called EGifSetGifVersion()
	 * in order to set that flag.
	 */
	for(i = 0; i < pGifFile->ImageCount; i++) {
		for(j = 0; j < pGifFile->SavedImages[i].ExtensionBlockCount; j++) {
			int function = pGifFile->SavedImages[i].ExtensionBlocks[j].Function;
			if(oneof4(function, COMMENT_EXT_FUNC_CODE, GRAPHICS_EXT_FUNC_CODE, PLAINTEXT_EXT_FUNC_CODE, APPLICATION_EXT_FUNC_CODE))
				p_private->gif89 = true;
		}
	}
	for(i = 0; i < pGifFile->ExtensionBlockCount; i++) {
		int function = pGifFile->ExtensionBlocks[i].Function;
		if(oneof4(function, COMMENT_EXT_FUNC_CODE, GRAPHICS_EXT_FUNC_CODE, PLAINTEXT_EXT_FUNC_CODE, APPLICATION_EXT_FUNC_CODE))
			p_private->gif89 = true;
	}
	return p_private->gif89 ? GIF89_STAMP : GIF87_STAMP;
}
// 
// Set the GIF version. In the extremely unlikely event that there is ever
// another version, replace the bool argument with ann enum in which the
// GIF87 value is 0 (numerically the same as bool false) and the GIF89 value
// is 1 (numerically the same as bool true).  That way we'll even preserve object-file compatibility!
// 
int EGifSetGifVersion(GifFileType * GifFile, const bool gif89)
{
	GifFilePrivateType * Private = static_cast<GifFilePrivateType *>(GifFile->Private);
	Private->gif89 = gif89;
	return 1;
}
// 
// This routine should be called before any other EGif calls, immediately
// following the GIF file opening.
// 
int EGifPutScreenDesc(GifFileType * GifFile, const int Width, const int Height, const int ColorRes, const int BackGround, const ColorMapObject * ColorMap)
{
	GifByteType Buf[3];
	GifFilePrivateType * Private = static_cast<GifFilePrivateType *>(GifFile->Private);
	char * write_version;
	if(Private->FileState & FILE_STATE_SCREEN) {
		/* If already has screen descriptor - something is wrong! */
		GifFile->Error = E_GIF_ERR_HAS_SCRN_DSCR;
		return GIF_ERROR;
	}
	if(!IS_WRITEABLE(Private)) {
		/* This file was NOT open for writing: */
		GifFile->Error = E_GIF_ERR_NOT_WRITEABLE;
		return GIF_ERROR;
	}
	write_version = EGifGetGifVersion(GifFile);
	/* First write the version prefix into the file. */
	if(InternalWrite(GifFile, reinterpret_cast<const uint8 *>(write_version), strlen(write_version)) != strlen(write_version)) {
		GifFile->Error = E_GIF_ERR_WRITE_FAILED;
		return GIF_ERROR;
	}
	GifFile->SWidth = Width;
	GifFile->SHeight = Height;
	GifFile->SColorResolution = ColorRes;
	GifFile->SBackGroundColor = BackGround;
	if(ColorMap) {
		GifFile->SColorMap = GifMakeMapObject(ColorMap->ColorCount, ColorMap->Colors);
		if(GifFile->SColorMap == NULL) {
			GifFile->Error = E_GIF_ERR_NOT_ENOUGH_MEM;
			return GIF_ERROR;
		}
	}
	else
		GifFile->SColorMap = NULL;
	/*
	 * Put the logical screen descriptor into the file:
	 */
	/* Logical Screen Descriptor: Dimensions */
	EGifPutWord(Width, GifFile);
	EGifPutWord(Height, GifFile);

	/* Logical Screen Descriptor: Packed Fields */
	/* Note: We have actual size of the color table default to the largest
	 * possible size (7+1 == 8 bits) because the decoder can use it to decide
	 * how to display the files.
	 */
	Buf[0] = (ColorMap ? 0x80 : 0x00) | /* Yes/no global colormap */
	    ((ColorRes - 1) << 4) |  /* Bits allocated to each primary color */
	    (ColorMap ? ColorMap->BitsPerPixel - 1 : 0x07 ); /* Actual size of the color table. */
	if(ColorMap && ColorMap->SortFlag)
		Buf[0] |= 0x08;
	Buf[1] = BackGround; /* Index into the ColorTable for background color */
	Buf[2] = GifFile->AspectByte; /* Pixel Aspect Ratio */
	InternalWrite(GifFile, Buf, 3);
	// If we have Global color map - dump it also: 
	if(ColorMap) {
		for(int i = 0; i < ColorMap->ColorCount; i++) {
			// Put the ColorMap out also: 
			Buf[0] = ColorMap->Colors[i].R;
			Buf[1] = ColorMap->Colors[i].G;
			Buf[2] = ColorMap->Colors[i].B;
			if(InternalWrite(GifFile, Buf, 3) != 3) {
				GifFile->Error = E_GIF_ERR_WRITE_FAILED;
				return GIF_ERROR;
			}
		}
	}
	Private->FileState |= FILE_STATE_SCREEN; /* Mark this file as has screen descriptor, and no pixel written yet: */
	return GIF_OK;
}
// 
// This routine should be called before any attempt to dump an image - any
// call to any of the pixel dump routines.
// 
int EGifPutImageDesc(GifFileType * GifFile, const int Left, const int Top, const int Width, const int Height, const bool Interlace, ColorMapObject * ColorMap)
{
	GifByteType Buf[3];
	GifFilePrivateType * Private = static_cast<GifFilePrivateType *>(GifFile->Private);
	if(Private->FileState & FILE_STATE_IMAGE && Private->PixelCount > 0xffff0000UL) {
		/* If already has active image descriptor - something is wrong! */
		GifFile->Error = E_GIF_ERR_HAS_IMAG_DSCR;
		return GIF_ERROR;
	}
	if(!IS_WRITEABLE(Private)) {
		/* This file was NOT open for writing: */
		GifFile->Error = E_GIF_ERR_NOT_WRITEABLE;
		return GIF_ERROR;
	}
	GifFile->Image.Left = Left;
	GifFile->Image.Top = Top;
	GifFile->Image.Width = Width;
	GifFile->Image.Height = Height;
	GifFile->Image.Interlace = Interlace;
	if(ColorMap) {
		GifFile->Image.ColorMap = GifMakeMapObject(ColorMap->ColorCount, ColorMap->Colors);
		if(GifFile->Image.ColorMap == NULL) {
			GifFile->Error = E_GIF_ERR_NOT_ENOUGH_MEM;
			return GIF_ERROR;
		}
	}
	else {
		GifFile->Image.ColorMap = NULL;
	}
	/* Put the image descriptor into the file: */
	Buf[0] = DESCRIPTOR_INTRODUCER; /* Image separator character. */
	InternalWrite(GifFile, Buf, 1);
	EGifPutWord(Left, GifFile);
	EGifPutWord(Top, GifFile);
	EGifPutWord(Width, GifFile);
	EGifPutWord(Height, GifFile);
	Buf[0] = (ColorMap ? 0x80 : 0x00) | (Interlace ? 0x40 : 0x00) | (ColorMap ? ColorMap->BitsPerPixel - 1 : 0);
	InternalWrite(GifFile, Buf, 1);
	// If we have Global color map - dump it also: 
	if(ColorMap) {
		for(int i = 0; i < ColorMap->ColorCount; i++) {
			// Put the ColorMap out also: 
			Buf[0] = ColorMap->Colors[i].R;
			Buf[1] = ColorMap->Colors[i].G;
			Buf[2] = ColorMap->Colors[i].B;
			if(InternalWrite(GifFile, Buf, 3) != 3) {
				GifFile->Error = E_GIF_ERR_WRITE_FAILED;
				return GIF_ERROR;
			}
		}
	}
	if(GifFile->SColorMap == NULL && GifFile->Image.ColorMap == NULL) {
		GifFile->Error = E_GIF_ERR_NO_COLOR_MAP;
		return GIF_ERROR;
	}
	/* Mark this file as has screen descriptor: */
	Private->FileState |= FILE_STATE_IMAGE;
	Private->PixelCount = (long)Width *(long)Height;
	/* Reset compress algorithm parameters. */
	(void)EGifSetupCompress(GifFile);
	return GIF_OK;
}
// 
// Put one full scanned line (Line) of length LineLen into GIF file.
// 
int EGifPutLine(GifFileType * GifFile, GifPixelType * Line, int LineLen)
{
	int i;
	GifPixelType Mask;
	GifFilePrivateType * Private = static_cast<GifFilePrivateType *>(GifFile->Private);
	if(!IS_WRITEABLE(Private)) {
		/* This file was NOT open for writing: */
		GifFile->Error = E_GIF_ERR_NOT_WRITEABLE;
		return GIF_ERROR;
	}
	if(!LineLen)
		LineLen = GifFile->Image.Width;
	if(Private->PixelCount < (uint)LineLen) {
		GifFile->Error = E_GIF_ERR_DATA_TOO_BIG;
		return GIF_ERROR;
	}
	Private->PixelCount -= LineLen;
	/* Make sure the codes are not out of bit range, as we might generate
	 * wrong code (because of overflow when we combine them) in this case: */
	Mask = CodeMask[Private->BitsPerPixel];
	for(i = 0; i < LineLen; i++)
		Line[i] &= Mask;
	return EGifCompressLine(GifFile, Line, LineLen);
}
// 
// Put one pixel (Pixel) into GIF file.
// 
int EGifPutPixel(GifFileType * GifFile, GifPixelType Pixel)
{
	GifFilePrivateType * Private = static_cast<GifFilePrivateType *>(GifFile->Private);
	if(!IS_WRITEABLE(Private)) {
		/* This file was NOT open for writing: */
		GifFile->Error = E_GIF_ERR_NOT_WRITEABLE;
		return GIF_ERROR;
	}
	if(Private->PixelCount == 0) {
		GifFile->Error = E_GIF_ERR_DATA_TOO_BIG;
		return GIF_ERROR;
	}
	--Private->PixelCount;
	/* Make sure the code is not out of bit range, as we might generate
	 * wrong code (because of overflow when we combine them) in this case: */
	Pixel &= CodeMask[Private->BitsPerPixel];
	return EGifCompressLine(GifFile, &Pixel, 1);
}
// 
// Put a comment into GIF file using the GIF89 comment extension block.
// 
int EGifPutComment(GifFileType * GifFile, const char * Comment)
{
	uint length = strlen(Comment);
	if(length <= 255) {
		return EGifPutExtension(GifFile, COMMENT_EXT_FUNC_CODE, length, Comment);
	}
	else {
		const char * buf = Comment;
		if(EGifPutExtensionLeader(GifFile, COMMENT_EXT_FUNC_CODE) == GIF_ERROR) {
			return GIF_ERROR;
		}
		/* Break the comment into 255 byte sub blocks */
		while(length > 255) {
			if(EGifPutExtensionBlock(GifFile, 255, buf) == GIF_ERROR) {
				return GIF_ERROR;
			}
			buf = buf + 255;
			length -= 255;
		}
		/* Output any partial block and the clear code. */
		if(length > 0) {
			if(EGifPutExtensionBlock(GifFile, length, buf) == GIF_ERROR) {
				return GIF_ERROR;
			}
		}
		if(EGifPutExtensionTrailer(GifFile) == GIF_ERROR) {
			return GIF_ERROR;
		}
	}
	return GIF_OK;
}

/******************************************************************************
   Begin an extension block (see GIF manual).  More
   extensions can be dumped using EGifPutExtensionBlock until
   EGifPutExtensionTrailer is invoked.
******************************************************************************/
int EGifPutExtensionLeader(GifFileType * GifFile, const int ExtCode)
{
	GifByteType Buf[3];
	GifFilePrivateType * Private = static_cast<GifFilePrivateType *>(GifFile->Private);
	if(!IS_WRITEABLE(Private)) {
		/* This file was NOT open for writing: */
		GifFile->Error = E_GIF_ERR_NOT_WRITEABLE;
		return GIF_ERROR;
	}
	else {
		Buf[0] = EXTENSION_INTRODUCER;
		Buf[1] = ExtCode;
		InternalWrite(GifFile, Buf, 2);
		return GIF_OK;
	}
}

/******************************************************************************
   Put extension block data (see GIF manual) into a GIF file.
******************************************************************************/
int EGifPutExtensionBlock(GifFileType * GifFile, const int ExtLen, const void * Extension)
{
	GifByteType Buf;
	GifFilePrivateType * Private = static_cast<GifFilePrivateType *>(GifFile->Private);
	if(!IS_WRITEABLE(Private)) {
		/* This file was NOT open for writing: */
		GifFile->Error = E_GIF_ERR_NOT_WRITEABLE;
		return GIF_ERROR;
	}
	else {
		Buf = ExtLen;
		InternalWrite(GifFile, &Buf, 1);
		InternalWrite(GifFile, (const uint8 *)Extension, ExtLen);
		return GIF_OK;
	}
}

/******************************************************************************
   Put a terminating block (see GIF manual) into a GIF file.
******************************************************************************/
int EGifPutExtensionTrailer(GifFileType * GifFile) 
{
	GifByteType Buf;
	GifFilePrivateType * Private = static_cast<GifFilePrivateType *>(GifFile->Private);
	if(!IS_WRITEABLE(Private)) {
		/* This file was NOT open for writing: */
		GifFile->Error = E_GIF_ERR_NOT_WRITEABLE;
		return GIF_ERROR;
	}
	else {
		/* Write the block terminator */
		Buf = 0;
		InternalWrite(GifFile, &Buf, 1);
		return GIF_OK;
	}
}

/******************************************************************************
   Put an extension block (see GIF manual) into a GIF file.
   Warning: This function is only useful for Extension blocks that have at
   most one subblock.  Extensions with more than one subblock need to use the
   EGifPutExtension{Leader,Block,Trailer} functions instead.
******************************************************************************/
int EGifPutExtension(GifFileType * GifFile, const int ExtCode, const int ExtLen, const void * Extension) 
{
	GifByteType Buf[3];
	GifFilePrivateType * Private = static_cast<GifFilePrivateType *>(GifFile->Private);
	if(!IS_WRITEABLE(Private)) {
		/* This file was NOT open for writing: */
		GifFile->Error = E_GIF_ERR_NOT_WRITEABLE;
		return GIF_ERROR;
	}
	if(ExtCode == 0)
		InternalWrite(GifFile, (GifByteType *)&ExtLen, 1);
	else {
		Buf[0] = EXTENSION_INTRODUCER;
		Buf[1] = ExtCode; /* Extension Label */
		Buf[2] = ExtLen; /* Extension length */
		InternalWrite(GifFile, Buf, 3);
	}
	InternalWrite(GifFile, (const uint8 *)Extension, ExtLen);
	Buf[0] = 0;
	InternalWrite(GifFile, Buf, 1);
	return GIF_OK;
}

/******************************************************************************
   Render a Graphics Control Block as raw extension data
******************************************************************************/

size_t EGifGCBToExtension(const GraphicsControlBlock * GCB, GifByteType * GifExtension)
{
	GifExtension[0] = 0;
	GifExtension[0] |= (GCB->TransparentColor == NO_TRANSPARENT_COLOR) ? 0x00 : 0x01;
	GifExtension[0] |= GCB->UserInputFlag ? 0x02 : 0x00;
	GifExtension[0] |= ((GCB->DisposalMode & 0x07) << 2);
	GifExtension[1] = LOBYTE(GCB->DelayTime);
	GifExtension[2] = HIBYTE(GCB->DelayTime);
	GifExtension[3] = (char)GCB->TransparentColor;
	return 4;
}

/******************************************************************************
   Replace the Graphics Control Block for a saved image, if it exists.
******************************************************************************/

int EGifGCBToSavedExtension(const GraphicsControlBlock * GCB, GifFileType * GifFile, int ImageIndex)
{
	int i;
	size_t Len;
	GifByteType buf[sizeof(GraphicsControlBlock)]; /* a bit dodgy... */
	if(ImageIndex < 0 || ImageIndex > GifFile->ImageCount - 1)
		return GIF_ERROR;
	for(i = 0; i < GifFile->SavedImages[ImageIndex].ExtensionBlockCount; i++) {
		ExtensionBlock * ep = &GifFile->SavedImages[ImageIndex].ExtensionBlocks[i];
		if(ep->Function == GRAPHICS_EXT_FUNC_CODE) {
			EGifGCBToExtension(GCB, ep->Bytes);
			return GIF_OK;
		}
	}
	Len = EGifGCBToExtension(GCB, (GifByteType *)buf);
	if(GifAddExtensionBlock(&GifFile->SavedImages[ImageIndex].ExtensionBlockCount, 
		&GifFile->SavedImages[ImageIndex].ExtensionBlocks, GRAPHICS_EXT_FUNC_CODE, Len, (uint8 *)buf) == GIF_ERROR)
		return GIF_ERROR;
	return GIF_OK;
}

/******************************************************************************
   Put the image code in compressed form. This routine can be called if the
   information needed to be piped out as is. Obviously this is much faster
   than decoding and encoding again. This routine should be followed by calls
   to EGifPutCodeNext, until NULL block is given.
   The block should NOT be freed by the user (not dynamically allocated).
******************************************************************************/
int EGifPutCode(GifFileType * GifFile, int CodeSize, const GifByteType * CodeBlock)
{
	GifFilePrivateType * Private = static_cast<GifFilePrivateType *>(GifFile->Private);
	if(!IS_WRITEABLE(Private)) {
		/* This file was NOT open for writing: */
		GifFile->Error = E_GIF_ERR_NOT_WRITEABLE;
		return GIF_ERROR;
	}
	/* No need to dump code size as Compression set up does any for us: */
	/*
	 * Buf = CodeSize;
	 * if (InternalWrite(GifFile, &Buf, 1) != 1) {
	 * GifFile->Error = E_GIF_ERR_WRITE_FAILED;
	 * return GIF_ERROR;
	 * }
	 */
	return EGifPutCodeNext(GifFile, CodeBlock);
}

/******************************************************************************
   Continue to put the image code in compressed form. This routine should be
   called with blocks of code as read via DGifGetCode/DGifGetCodeNext. If
   given buffer pointer is NULL, empty block is written to mark end of code.
******************************************************************************/
int EGifPutCodeNext(GifFileType * GifFile, const GifByteType * CodeBlock)
{
	GifByteType Buf;
	GifFilePrivateType * Private = static_cast<GifFilePrivateType *>(GifFile->Private);
	if(CodeBlock) {
		if(InternalWrite(GifFile, CodeBlock, CodeBlock[0] + 1) != (uint)(CodeBlock[0] + 1)) {
			GifFile->Error = E_GIF_ERR_WRITE_FAILED;
			return GIF_ERROR;
		}
	}
	else {
		Buf = 0;
		if(InternalWrite(GifFile, &Buf, 1) != 1) {
			GifFile->Error = E_GIF_ERR_WRITE_FAILED;
			return GIF_ERROR;
		}
		Private->PixelCount = 0; /* And local info. indicate image read. */
	}
	return GIF_OK;
}

/******************************************************************************
   This routine should be called last, to close the GIF file.
******************************************************************************/
int EGifCloseFile(GifFileType * GifFile)
{
	GifByteType Buf;
	GifFilePrivateType * Private;
	FILE * File;
	if(GifFile == NULL)
		return GIF_ERROR;
	Private = static_cast<GifFilePrivateType *>(GifFile->Private);
	if(Private == NULL)
		return GIF_ERROR;
	if(!IS_WRITEABLE(Private)) {
		/* This file was NOT open for writing: */
		GifFile->Error = E_GIF_ERR_NOT_WRITEABLE;
		return GIF_ERROR;
	}
	File = Private->File;
	Buf = TERMINATOR_INTRODUCER;
	InternalWrite(GifFile, &Buf, 1);
	if(GifFile->Image.ColorMap) {
		GifFreeMapObject(GifFile->Image.ColorMap);
		GifFile->Image.ColorMap = NULL;
	}
	if(GifFile->SColorMap) {
		GifFreeMapObject(GifFile->SColorMap);
		GifFile->SColorMap = NULL;
	}
	if(Private) {
		if(Private->HashTable) {
			SAlloc::F((char *)Private->HashTable);
		}
		SAlloc::F((char *)Private);
	}
	if(File && fclose(File) != 0) {
		GifFile->Error = E_GIF_ERR_CLOSE_FAILED;
		return GIF_ERROR;
	}
	/*
	 * Without the #ifndef, we get spurious warnings because Coverity mistakenly
	 * thinks the GIF structure is freed on an error return.
	 */
#ifndef __COVERITY__
	SAlloc::F(GifFile);
#endif /* __COVERITY__ */
	return GIF_OK;
}
// 
// This routine writes to disk an in-core representation of a GIF previously
// created by DGifSlurp().
// 
static int EGifWriteExtensions(GifFileType * GifFileOut, ExtensionBlock * ExtensionBlocks, int ExtensionBlockCount)
{
	if(ExtensionBlocks) {
		for(int j = 0; j < ExtensionBlockCount; j++) {
			ExtensionBlock * ep = &ExtensionBlocks[j];
			if(ep->Function != CONTINUE_EXT_FUNC_CODE)
				if(EGifPutExtensionLeader(GifFileOut, ep->Function) == GIF_ERROR)
					return GIF_ERROR;
			if(EGifPutExtensionBlock(GifFileOut, ep->ByteCount, ep->Bytes) == GIF_ERROR)
				return GIF_ERROR;
			if(j == ExtensionBlockCount - 1 || (ep+1)->Function != CONTINUE_EXT_FUNC_CODE)
				if(EGifPutExtensionTrailer(GifFileOut) == GIF_ERROR)
					return GIF_ERROR;
		}
	}
	return GIF_OK;
}

int EGifSpew(GifFileType * GifFileOut)
{
	int i, j;
	if(EGifPutScreenDesc(GifFileOut, GifFileOut->SWidth, GifFileOut->SHeight,
	    GifFileOut->SColorResolution, GifFileOut->SBackGroundColor, GifFileOut->SColorMap) == GIF_ERROR) {
		return GIF_ERROR;
	}
	for(i = 0; i < GifFileOut->ImageCount; i++) {
		GifSavedImage * sp = &GifFileOut->SavedImages[i];
		int SavedHeight = sp->ImageDesc.Height;
		int SavedWidth = sp->ImageDesc.Width;
		/* this allows us to delete images by nuking their rasters */
		if(sp->RasterBits == NULL)
			continue;
		if(EGifWriteExtensions(GifFileOut, sp->ExtensionBlocks, sp->ExtensionBlockCount) == GIF_ERROR)
			return GIF_ERROR;
		if(EGifPutImageDesc(GifFileOut, sp->ImageDesc.Left, sp->ImageDesc.Top,
			SavedWidth, SavedHeight, sp->ImageDesc.Interlace, sp->ImageDesc.ColorMap) == GIF_ERROR)
			return GIF_ERROR;
		if(sp->ImageDesc.Interlace) {
			/*
			 * The way an interlaced image should be written - offsets and jumps...
			 */
			const int InterlacedOffset[] = { 0, 4, 2, 1 };
			const int InterlacedJumps[] = { 8, 8, 4, 2 };
			int k;
			/* Need to perform 4 passes on the images: */
			for(k = 0; k < 4; k++)
				for(j = InterlacedOffset[k]; j < SavedHeight; j += InterlacedJumps[k]) {
					if(EGifPutLine(GifFileOut, sp->RasterBits + j * SavedWidth, SavedWidth) == GIF_ERROR)
						return GIF_ERROR;
				}
		}
		else {
			for(j = 0; j < SavedHeight; j++) {
				if(EGifPutLine(GifFileOut, sp->RasterBits + j * SavedWidth, SavedWidth) == GIF_ERROR)
					return GIF_ERROR;
			}
		}
	}
	if(EGifWriteExtensions(GifFileOut, GifFileOut->ExtensionBlocks, GifFileOut->ExtensionBlockCount) == GIF_ERROR)
		return GIF_ERROR;
	if(EGifCloseFile(GifFileOut) == GIF_ERROR)
		return GIF_ERROR;
	return GIF_OK;
}
// 
// Ascii 8 by 8 regular font - only first 128 characters are supported.
// 
// Each array entry holds the bits for 8 horizontal scan lines, topmost
// first. The most significant bit of each constant is the leftmost bit of the scan line.
// 
// @+charint@
const uint8 GifAsciiTable8x8[][GIF_FONT_WIDTH] = {
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /* Ascii 0 */
	{0x3c, 0x42, 0xa5, 0x81, 0xbd, 0x42, 0x3c, 0x00}, /* Ascii 1 */
	{0x3c, 0x7e, 0xdb, 0xff, 0xc3, 0x7e, 0x3c, 0x00}, /* Ascii 2 */
	{0x00, 0xee, 0xfe, 0xfe, 0x7c, 0x38, 0x10, 0x00}, /* Ascii 3 */
	{0x10, 0x38, 0x7c, 0xfe, 0x7c, 0x38, 0x10, 0x00}, /* Ascii 4 */
	{0x00, 0x3c, 0x18, 0xff, 0xff, 0x08, 0x18, 0x00}, /* Ascii 5 */
	{0x10, 0x38, 0x7c, 0xfe, 0xfe, 0x10, 0x38, 0x00}, /* Ascii 6 */
	{0x00, 0x00, 0x18, 0x3c, 0x18, 0x00, 0x00, 0x00}, /* Ascii 7 */
	{0xff, 0xff, 0xe7, 0xc3, 0xe7, 0xff, 0xff, 0xff}, /* Ascii 8 */
	{0x00, 0x3c, 0x42, 0x81, 0x81, 0x42, 0x3c, 0x00}, /* Ascii 9 */
	{0xff, 0xc3, 0xbd, 0x7e, 0x7e, 0xbd, 0xc3, 0xff}, /* Ascii 10 */
	{0x1f, 0x07, 0x0d, 0x7c, 0xc6, 0xc6, 0x7c, 0x00}, /* Ascii 11 */
	{0x00, 0x7e, 0xc3, 0xc3, 0x7e, 0x18, 0x7e, 0x18}, /* Ascii 12 */
	{0x04, 0x06, 0x07, 0x04, 0x04, 0xfc, 0xf8, 0x00}, /* Ascii 13 */
	{0x0c, 0x0a, 0x0d, 0x0b, 0xf9, 0xf9, 0x1f, 0x1f}, /* Ascii 14 */
	{0x00, 0x92, 0x7c, 0x44, 0xc6, 0x7c, 0x92, 0x00}, /* Ascii 15 */
	{0x00, 0x00, 0x60, 0x78, 0x7e, 0x78, 0x60, 0x00}, /* Ascii 16 */
	{0x00, 0x00, 0x06, 0x1e, 0x7e, 0x1e, 0x06, 0x00}, /* Ascii 17 */
	{0x18, 0x7e, 0x18, 0x18, 0x18, 0x18, 0x7e, 0x18}, /* Ascii 18 */
	{0x66, 0x66, 0x66, 0x66, 0x66, 0x00, 0x66, 0x00}, /* Ascii 19 */
	{0xff, 0xb6, 0x76, 0x36, 0x36, 0x36, 0x36, 0x00}, /* Ascii 20 */
	{0x7e, 0xc1, 0xdc, 0x22, 0x22, 0x1f, 0x83, 0x7e}, /* Ascii 21 */
	{0x00, 0x00, 0x00, 0x7e, 0x7e, 0x00, 0x00, 0x00}, /* Ascii 22 */
	{0x18, 0x7e, 0x18, 0x18, 0x7e, 0x18, 0x00, 0xff}, /* Ascii 23 */
	{0x18, 0x7e, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00}, /* Ascii 24 */
	{0x18, 0x18, 0x18, 0x18, 0x18, 0x7e, 0x18, 0x00}, /* Ascii 25 */
	{0x00, 0x04, 0x06, 0xff, 0x06, 0x04, 0x00, 0x00}, /* Ascii 26 */
	{0x00, 0x20, 0x60, 0xff, 0x60, 0x20, 0x00, 0x00}, /* Ascii 27 */
	{0x00, 0x00, 0x00, 0xc0, 0xc0, 0xc0, 0xff, 0x00}, /* Ascii 28 */
	{0x00, 0x24, 0x66, 0xff, 0x66, 0x24, 0x00, 0x00}, /* Ascii 29 */
	{0x00, 0x00, 0x10, 0x38, 0x7c, 0xfe, 0x00, 0x00}, /* Ascii 30 */
	{0x00, 0x00, 0x00, 0xfe, 0x7c, 0x38, 0x10, 0x00}, /* Ascii 31 */
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /* */
	{0x30, 0x30, 0x30, 0x30, 0x30, 0x00, 0x30, 0x00}, /* ! */
	{0x66, 0x66, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /* " */
	{0x6c, 0x6c, 0xfe, 0x6c, 0xfe, 0x6c, 0x6c, 0x00}, /* # */
	{0x10, 0x7c, 0xd2, 0x7c, 0x86, 0x7c, 0x10, 0x00}, /* $ */
	{0xf0, 0x96, 0xfc, 0x18, 0x3e, 0x72, 0xde, 0x00}, /* % */
	{0x30, 0x48, 0x30, 0x78, 0xce, 0xcc, 0x78, 0x00}, /* & */
	{0x0c, 0x0c, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00}, /* ' */
	{0x10, 0x60, 0xc0, 0xc0, 0xc0, 0x60, 0x10, 0x00}, /* ( */
	{0x10, 0x0c, 0x06, 0x06, 0x06, 0x0c, 0x10, 0x00}, /* ) */
	{0x00, 0x54, 0x38, 0xfe, 0x38, 0x54, 0x00, 0x00}, /* * */
	{0x00, 0x18, 0x18, 0x7e, 0x18, 0x18, 0x00, 0x00}, /* + */
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x70}, /* , */
	{0x00, 0x00, 0x00, 0x7e, 0x00, 0x00, 0x00, 0x00}, /* - */
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x00}, /* . */
	{0x02, 0x06, 0x0c, 0x18, 0x30, 0x60, 0xc0, 0x00}, /* / */
	{0x7c, 0xc6, 0xc6, 0xc6, 0xc6, 0xc6, 0x7c, 0x00}, /* 0 */
	{0x18, 0x38, 0x78, 0x18, 0x18, 0x18, 0x3c, 0x00}, /* 1 */
	{0x7c, 0xc6, 0x06, 0x0c, 0x30, 0x60, 0xfe, 0x00}, /* 2 */
	{0x7c, 0xc6, 0x06, 0x3c, 0x06, 0xc6, 0x7c, 0x00}, /* 3 */
	{0x0e, 0x1e, 0x36, 0x66, 0xfe, 0x06, 0x06, 0x00}, /* 4 */
	{0xfe, 0xc0, 0xc0, 0xfc, 0x06, 0x06, 0xfc, 0x00}, /* 5 */
	{0x7c, 0xc6, 0xc0, 0xfc, 0xc6, 0xc6, 0x7c, 0x00}, /* 6 */
	{0xfe, 0x06, 0x0c, 0x18, 0x30, 0x60, 0x60, 0x00}, /* 7 */
	{0x7c, 0xc6, 0xc6, 0x7c, 0xc6, 0xc6, 0x7c, 0x00}, /* 8 */
	{0x7c, 0xc6, 0xc6, 0x7e, 0x06, 0xc6, 0x7c, 0x00}, /* 9 */
	{0x00, 0x30, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00}, /* : */
	{0x00, 0x30, 0x00, 0x00, 0x00, 0x30, 0x20, 0x00}, /* }, */
	{0x00, 0x1c, 0x30, 0x60, 0x30, 0x1c, 0x00, 0x00}, /* < */
	{0x00, 0x00, 0x7e, 0x00, 0x7e, 0x00, 0x00, 0x00}, /* = */
	{0x00, 0x70, 0x18, 0x0c, 0x18, 0x70, 0x00, 0x00}, /* > */
	{0x7c, 0xc6, 0x0c, 0x18, 0x30, 0x00, 0x30, 0x00}, /* ? */
	{0x7c, 0x82, 0x9a, 0xaa, 0xaa, 0x9e, 0x7c, 0x00}, /* @ */
	{0x38, 0x6c, 0xc6, 0xc6, 0xfe, 0xc6, 0xc6, 0x00}, /* A */
	{0xfc, 0xc6, 0xc6, 0xfc, 0xc6, 0xc6, 0xfc, 0x00}, /* B */
	{0x7c, 0xc6, 0xc6, 0xc0, 0xc0, 0xc6, 0x7c, 0x00}, /* C */
	{0xf8, 0xcc, 0xc6, 0xc6, 0xc6, 0xcc, 0xf8, 0x00}, /* D */
	{0xfe, 0xc0, 0xc0, 0xfc, 0xc0, 0xc0, 0xfe, 0x00}, /* E */
	{0xfe, 0xc0, 0xc0, 0xfc, 0xc0, 0xc0, 0xc0, 0x00}, /* F */
	{0x7c, 0xc6, 0xc0, 0xce, 0xc6, 0xc6, 0x7e, 0x00}, /* G */
	{0xc6, 0xc6, 0xc6, 0xfe, 0xc6, 0xc6, 0xc6, 0x00}, /* H */
	{0x78, 0x30, 0x30, 0x30, 0x30, 0x30, 0x78, 0x00}, /* I */
	{0x1e, 0x06, 0x06, 0x06, 0xc6, 0xc6, 0x7c, 0x00}, /* J */
	{0xc6, 0xcc, 0xd8, 0xf0, 0xd8, 0xcc, 0xc6, 0x00}, /* K */
	{0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xfe, 0x00}, /* L */
	{0xc6, 0xee, 0xfe, 0xd6, 0xc6, 0xc6, 0xc6, 0x00}, /* M */
	{0xc6, 0xe6, 0xf6, 0xde, 0xce, 0xc6, 0xc6, 0x00}, /* N */
	{0x7c, 0xc6, 0xc6, 0xc6, 0xc6, 0xc6, 0x7c, 0x00}, /* O */
	{0xfc, 0xc6, 0xc6, 0xfc, 0xc0, 0xc0, 0xc0, 0x00}, /* P */
	{0x7c, 0xc6, 0xc6, 0xc6, 0xc6, 0xc6, 0x7c, 0x06}, /* Q */
	{0xfc, 0xc6, 0xc6, 0xfc, 0xc6, 0xc6, 0xc6, 0x00}, /* R */
	{0x78, 0xcc, 0x60, 0x30, 0x18, 0xcc, 0x78, 0x00}, /* S */
	{0xfc, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x00}, /* T */
	{0xc6, 0xc6, 0xc6, 0xc6, 0xc6, 0xc6, 0x7c, 0x00}, /* U */
	{0xc6, 0xc6, 0xc6, 0xc6, 0xc6, 0x6c, 0x38, 0x00}, /* V */
	{0xc6, 0xc6, 0xc6, 0xd6, 0xfe, 0xee, 0xc6, 0x00}, /* W */
	{0xc6, 0xc6, 0x6c, 0x38, 0x6c, 0xc6, 0xc6, 0x00}, /* X */
	{0xc3, 0xc3, 0x66, 0x3c, 0x18, 0x18, 0x18, 0x00}, /* Y */
	{0xfe, 0x0c, 0x18, 0x30, 0x60, 0xc0, 0xfe, 0x00}, /* Z */
	{0x3c, 0x30, 0x30, 0x30, 0x30, 0x30, 0x3c, 0x00}, /* [ */
	{0xc0, 0x60, 0x30, 0x18, 0x0c, 0x06, 0x03, 0x00}, /* \ */
	{0x3c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x3c, 0x00}, /* ] */
	{0x00, 0x38, 0x6c, 0xc6, 0x00, 0x00, 0x00, 0x00}, /* ^ */
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff}, /* _ */
	{0x30, 0x30, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00}, /* ` */
	{0x00, 0x00, 0x7c, 0x06, 0x7e, 0xc6, 0x7e, 0x00}, /* a */
	{0xc0, 0xc0, 0xfc, 0xc6, 0xc6, 0xe6, 0xdc, 0x00}, /* b */
	{0x00, 0x00, 0x7c, 0xc6, 0xc0, 0xc0, 0x7e, 0x00}, /* c */
	{0x06, 0x06, 0x7e, 0xc6, 0xc6, 0xce, 0x76, 0x00}, /* d */
	{0x00, 0x00, 0x7c, 0xc6, 0xfe, 0xc0, 0x7e, 0x00}, /* e */
	{0x1e, 0x30, 0x7c, 0x30, 0x30, 0x30, 0x30, 0x00}, /* f */
	{0x00, 0x00, 0x7e, 0xc6, 0xce, 0x76, 0x06, 0x7c}, /* g */
	{0xc0, 0xc0, 0xfc, 0xc6, 0xc6, 0xc6, 0xc6, 0x00}, /* */
	{0x18, 0x00, 0x38, 0x18, 0x18, 0x18, 0x3c, 0x00}, /* i */
	{0x18, 0x00, 0x38, 0x18, 0x18, 0x18, 0x18, 0xf0}, /* j */
	{0xc0, 0xc0, 0xcc, 0xd8, 0xf0, 0xd8, 0xcc, 0x00}, /* k */
	{0x38, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3c, 0x00}, /* l */
	{0x00, 0x00, 0xcc, 0xfe, 0xd6, 0xc6, 0xc6, 0x00}, /* m */
	{0x00, 0x00, 0xfc, 0xc6, 0xc6, 0xc6, 0xc6, 0x00}, /* n */
	{0x00, 0x00, 0x7c, 0xc6, 0xc6, 0xc6, 0x7c, 0x00}, /* o */
	{0x00, 0x00, 0xfc, 0xc6, 0xc6, 0xe6, 0xdc, 0xc0}, /* p */
	{0x00, 0x00, 0x7e, 0xc6, 0xc6, 0xce, 0x76, 0x06}, /* q */
	{0x00, 0x00, 0x6e, 0x70, 0x60, 0x60, 0x60, 0x00}, /* r */
	{0x00, 0x00, 0x7c, 0xc0, 0x7c, 0x06, 0xfc, 0x00}, /* s */
	{0x30, 0x30, 0x7c, 0x30, 0x30, 0x30, 0x1c, 0x00}, /* t */
	{0x00, 0x00, 0xc6, 0xc6, 0xc6, 0xc6, 0x7e, 0x00}, /* u */
	{0x00, 0x00, 0xc6, 0xc6, 0xc6, 0x6c, 0x38, 0x00}, /* v */
	{0x00, 0x00, 0xc6, 0xc6, 0xd6, 0xfe, 0x6c, 0x00}, /* w */
	{0x00, 0x00, 0xc6, 0x6c, 0x38, 0x6c, 0xc6, 0x00}, /* x */
	{0x00, 0x00, 0xc6, 0xc6, 0xce, 0x76, 0x06, 0x7c}, /* y */
	{0x00, 0x00, 0xfc, 0x18, 0x30, 0x60, 0xfc, 0x00}, /* z */
	{0x0e, 0x18, 0x18, 0x70, 0x18, 0x18, 0x0e, 0x00}, /* { */
	{0x18, 0x18, 0x18, 0x00, 0x18, 0x18, 0x18, 0x00}, /* | */
	{0xe0, 0x30, 0x30, 0x1c, 0x30, 0x30, 0xe0, 0x00}, /* } */
	{0x00, 0x00, 0x70, 0x9a, 0x0e, 0x00, 0x00, 0x00}, /* ~ */
	{0x00, 0x00, 0x18, 0x3c, 0x66, 0xff, 0x00, 0x00} /* Ascii 127 */
};
/*@=charint@*/

void GifDrawText8x8(GifSavedImage * Image, const int x, const int y, const char * legend, const int color)
{
	int i, j;
	int base;
	const char * cp;
	for(i = 0; i < GIF_FONT_HEIGHT; i++) {
		base = Image->ImageDesc.Width * (y + i) + x;
		for(cp = legend; *cp; cp++)
			for(j = 0; j < GIF_FONT_WIDTH; j++) {
				if(GifAsciiTable8x8[(short)(*cp)][i] & (1 << (GIF_FONT_WIDTH - j)))
					Image->RasterBits[base] = color;
				base++;
			}
	}
}

void GifDrawBox(GifSavedImage * Image, const int x, const int y, const int w, const int d, const int color)
{
	int j, base = Image->ImageDesc.Width * y + x;
	for(j = 0; j < w; j++)
		Image->RasterBits[base + j] = Image->RasterBits[base + (d * Image->ImageDesc.Width) + j] = color;
	for(j = 0; j < d; j++)
		Image->RasterBits[base + j * Image->ImageDesc.Width] = Image->RasterBits[base + j * Image->ImageDesc.Width + w] = color;
}

void GifDrawRectangle(GifSavedImage * Image, const int x, const int y, const int w, const int d, const int color)
{
	uint8 * bp = Image->RasterBits + Image->ImageDesc.Width * y + x;
	for(int i = 0; i < d; i++)
		memset(bp + (i * Image->ImageDesc.Width), color, (size_t)w);
}

void GifDrawBoxedText8x8(GifSavedImage * Image, const int x, const int y, const char * legend, const int border, const int bg, const int fg)
{
	int i, j = 0, LineCount = 0, TextWidth = 0;
	const char * cp;
	/* compute size of text to box */
	for(cp = legend; *cp; cp++)
		if(*cp == '\r') {
			if(j > TextWidth)
				TextWidth = j;
			j = 0;
			LineCount++;
		}
		else if(*cp != '\t')
			++j;
	LineCount++; /* count last line */
	if(j > TextWidth) /* last line might be longer than any previous */
		TextWidth = j;
	/* fill the box */
	GifDrawRectangle(Image, x + 1, y + 1, border + TextWidth * GIF_FONT_WIDTH + border - 1, border + LineCount * GIF_FONT_HEIGHT + border - 1, bg);
	/* draw the text */
	i = 0;
	cp = strtok((char *)legend, "\r\n");
	do {
		int leadspace = 0;
		if(cp[0] == '\t')
			leadspace = (TextWidth - strlen(++cp)) / 2;
		GifDrawText8x8(Image, x + border + (leadspace * GIF_FONT_WIDTH), y + border + (GIF_FONT_HEIGHT * i++), cp, fg);
		cp = strtok(NULL, "\r\n");
	} while(cp);
	/* outline the box */
	GifDrawBox(Image, x, y, border + TextWidth * GIF_FONT_WIDTH + border, border + LineCount * GIF_FONT_HEIGHT + border, fg);
}
// 
// gif_hash.c -- module to support the following operations:
// 
// 1. InitHashTable - initialize hash table.
// 2. ClearHashTable - clear the hash table to an empty state.
// 2. InsertHashTable - insert one item into data structure.
// 3. ExistsHashTable - test if item exists in data structure.
// 
// This module is used to hash the GIF codes during encoding.
// 
// Routine to generate an HKey for the hashtable out of the given unique key. 
// The given Key is assumed to be 20 bits as follows: lower 8 bits are the
// new postfix character, while the upper 12 bits are the prefix code.
// Because the average hit ratio is only 2 (2 hash references per entry),
// evaluating more complex keys (such as twin prime keys) does not worth it!
// 
static int FASTCALL KeyItem(uint32 Item)
{
	return ((Item >> 12) ^ Item) & HT_KEY_MASK;
}
// 
// Initialize HashTable - allocate the memory needed and clear it.	     
// 
GifHashTableType * _InitHashTable(void)
{
	GifHashTableType * HashTable;
	if((HashTable = static_cast<GifHashTableType *>(SAlloc::M(sizeof(GifHashTableType)))) == NULL)
		return NULL;
	_ClearHashTable(HashTable);
	return HashTable;
}
// 
// Routine to clear the HashTable to an empty state.			
// This part is a little machine depended. Use the commented part otherwise. 
// 
void _ClearHashTable(GifHashTableType * HashTable)
{
	memset(HashTable->HTable, 0xFF, HT_SIZE * sizeof(uint32));
}
//
// Routine to insert a new Item into the HashTable. The data is assumed to be new one.								      *
//
void _InsertHashTable(GifHashTableType * HashTable, uint32 Key, int Code)
{
	int HKey = KeyItem(Key);
	uint32 * HTable = HashTable->HTable;
#ifdef DEBUG_HIT_RATE
	NumberOfTests++;
	NumberOfMisses++;
#endif /* DEBUG_HIT_RATE */
	while(HT_GET_KEY(HTable[HKey]) != 0xFFFFFL) {
#ifdef DEBUG_HIT_RATE
		NumberOfMisses++;
#endif /* DEBUG_HIT_RATE */
		HKey = (HKey + 1) & HT_KEY_MASK;
	}
	HTable[HKey] = HT_PUT_KEY(Key) | HT_PUT_CODE(Code);
}
//
// Routine to test if given Key exists in HashTable and if so returns its code
// Returns the Code if key was found, -1 if not.
//
int _ExistsHashTable(GifHashTableType * HashTable, uint32 Key)
{
	int HKey = KeyItem(Key);
	uint32 * HTable = HashTable->HTable, HTKey;
#ifdef DEBUG_HIT_RATE
	NumberOfTests++;
	NumberOfMisses++;
#endif /* DEBUG_HIT_RATE */
	while((HTKey = HT_GET_KEY(HTable[HKey])) != 0xFFFFFL) {
#ifdef DEBUG_HIT_RATE
		NumberOfMisses++;
#endif /* DEBUG_HIT_RATE */
		if(Key == HTKey) return HT_GET_CODE(HTable[HKey]);
		HKey = (HKey + 1) & HT_KEY_MASK;
	}
	return -1;
}

#ifdef  DEBUG_HIT_RATE
// 
// Debugging routine to print the hit ratio - number of times the hash table
// was tested per operation. This routine was used to test the KeyItem routine
// 
void HashTablePrintHitRatio(void)
{
	printf("Hash Table Hit Ratio is %ld/%ld = %ld%%.\n", NumberOfMisses, NumberOfTests, NumberOfMisses * 100 / NumberOfTests);
}

#endif  /* DEBUG_HIT_RATE */
// 
// GIF construction tools
// 
// @sobolev #define MAX(x, y)    (((x) > (y)) ? (x) : (y))
// 
// Miscellaneous utility functions
// 
// 
// return smallest bitfield size n will fit in 
int FASTCALL GifBitSize(int n)
{
	int i;
	for(i = 1; i <= 8; i++)
		if((1 << i) >= n)
			break;
	return (i);
}
// 
// Color map object functions
// 
// Allocate a color map of given size; initialize with contents of
// ColorMap if that pointer is non-NULL.
// 
ColorMapObject * FASTCALL GifMakeMapObject(int ColorCount, const SColorRGB * ColorMap)
{
	ColorMapObject * Object;
	/*** FIXME: Our ColorCount has to be a power of two.  Is it necessary to
	 * make the user know that or should we automatically round up instead? */
	if(ColorCount != (1 << GifBitSize(ColorCount))) {
		return 0;
	}
	Object = static_cast<ColorMapObject *>(SAlloc::M(sizeof(ColorMapObject)));
	if(Object == 0) {
		return 0;
	}
	Object->Colors = static_cast<SColorRGB *>(SAlloc::C(ColorCount, sizeof(SColorRGB)));
	if(!Object->Colors) {
		SAlloc::F(Object);
		return 0;
	}
	Object->ColorCount = ColorCount;
	Object->BitsPerPixel = GifBitSize(ColorCount);
	if(ColorMap) {
		memcpy(Object->Colors, ColorMap, ColorCount * sizeof(SColorRGB));
	}
	return (Object);
}
// 
// Free a color map object
// 
void FASTCALL GifFreeMapObject(ColorMapObject * Object)
{
	if(Object) {
		SAlloc::F(Object->Colors);
		SAlloc::F(Object);
	}
}

#ifdef DEBUG
void DumpColorMap(ColorMapObject * Object, FILE * fp)
{
	if(Object) {
		int i, j, Len = Object->ColorCount;
		for(i = 0; i < Len; i += 4) {
			for(j = 0; j < 4 && j < Len; j++) {
				(void)fprintf(fp, "%3d: %02x %02x %02x   ", i + j, Object->Colors[i + j].Red, Object->Colors[i + j].Green, Object->Colors[i + j].Blue);
			}
			(void)fprintf(fp, "\n");
		}
	}
}
#endif /* DEBUG */
// 
// Compute the union of two given color maps and return it.  If result can't
// fit into 256 colors, NULL is returned, the allocated union otherwise.
// ColorIn1 is copied as is to ColorUnion, while colors from ColorIn2 are
// copied iff they didn't exist before.  ColorTransIn2 maps the old
// ColorIn2 into the ColorUnion color map table./
// 
ColorMapObject * GifUnionColorMap(const ColorMapObject * ColorIn1, const ColorMapObject * ColorIn2, GifPixelType ColorTransIn2[])
{
	int i, j, CrntSlot, RoundUpTo, NewGifBitSize;
	ColorMapObject * p_color_union;
	/*
	 * We don't worry about duplicates within either color map; if
	 * the caller wants to resolve those, he can perform unions
	 * with an empty color map.
	 */
	/* Allocate table which will hold the result for sure. */
	p_color_union = GifMakeMapObject(MAX(ColorIn1->ColorCount, ColorIn2->ColorCount) * 2, 0);
	if(p_color_union == NULL)
		return NULL;
	/*
	 * Copy ColorIn1 to ColorUnion.
	 */
	for(i = 0; i < ColorIn1->ColorCount; i++)
		p_color_union->Colors[i] = ColorIn1->Colors[i];
	CrntSlot = ColorIn1->ColorCount;
	/*
	 * Potentially obnoxious hack:
	 *
	 * Back CrntSlot down past all contiguous {0, 0, 0} slots at the end
	 * of table 1.  This is very useful if your display is limited to
	 * 16 colors.
	 */
	while(ColorIn1->Colors[CrntSlot-1].IsZero())
		CrntSlot--;
	// Copy ColorIn2 to ColorUnion (use old colors if they exist): 
	for(i = 0; i < ColorIn2->ColorCount && CrntSlot <= 256; i++) {
		/* Let's see if this color already exists: */
		for(j = 0; j < ColorIn1->ColorCount; j++)
			if(memcmp(&ColorIn1->Colors[j], &ColorIn2->Colors[i], sizeof(SColorRGB)) == 0)
				break;
		if(j < ColorIn1->ColorCount)
			ColorTransIn2[i] = j; // color exists in Color1 
		else {
			// Color is new - copy it to a new slot: 
			p_color_union->Colors[CrntSlot] = ColorIn2->Colors[i];
			ColorTransIn2[i] = CrntSlot++;
		}
	}
	if(CrntSlot > 256) {
		GifFreeMapObject(p_color_union);
		return 0;
	}
	NewGifBitSize = GifBitSize(CrntSlot);
	RoundUpTo = (1 << NewGifBitSize);
	if(RoundUpTo != p_color_union->ColorCount) {
		register SColorRGB * Map = p_color_union->Colors;
		// 
		// Zero out slots up to next power of 2.
		// We know these slots exist because of the way ColorUnion's
		// start dimension was computed.
		// 
		for(j = CrntSlot; j < RoundUpTo; j++) {
			Map[j].Set(0);
		}
		// perhaps we can shrink the map? 
		if(RoundUpTo < p_color_union->ColorCount)
			p_color_union->Colors = static_cast<SColorRGB *>(SAlloc::R(Map, sizeof(SColorRGB) * RoundUpTo));
	}
	p_color_union->ColorCount = RoundUpTo;
	p_color_union->BitsPerPixel = NewGifBitSize;
	return (p_color_union);
}
// 
// Apply a given color translation to the raster bits of an image
// 
void GifApplyTranslation(GifSavedImage * pImage, GifPixelType Translation[])
{
	int RasterSize = pImage->ImageDesc.Height * pImage->ImageDesc.Width;
	for(int i = 0; i < RasterSize; i++)
		pImage->RasterBits[i] = Translation[pImage->RasterBits[i]];
}
// 
// Extension record functions
// 
int GifAddExtensionBlock(int * pExtensionBlockCount, ExtensionBlock ** ppExtensionBlocks, int Function, uint Len, uint8 ExtData[])
{
	ExtensionBlock * ep;
	if(*ppExtensionBlocks == NULL)
		*ppExtensionBlocks = static_cast<ExtensionBlock *>(SAlloc::M(sizeof(ExtensionBlock)));
	else
		*ppExtensionBlocks = static_cast<ExtensionBlock *>(SAlloc::R(*ppExtensionBlocks, sizeof(ExtensionBlock) * (*pExtensionBlockCount + 1)));
	if(*ppExtensionBlocks == NULL)
		return GIF_ERROR;
	ep = &(*ppExtensionBlocks)[(*pExtensionBlockCount)++];
	ep->Function = Function;
	ep->ByteCount = Len;
	ep->Bytes = static_cast<GifByteType *>(SAlloc::M(ep->ByteCount));
	if(ep->Bytes == NULL)
		return GIF_ERROR;
	if(ExtData)
		memcpy(ep->Bytes, ExtData, Len);
	return GIF_OK;
}

void GifFreeExtensions(int * ExtensionBlockCount, ExtensionBlock ** ExtensionBlocks)
{
	ExtensionBlock * ep;
	if(*ExtensionBlocks) {
		for(ep = *ExtensionBlocks; ep < (*ExtensionBlocks + *ExtensionBlockCount); ep++)
			SAlloc::F(ep->Bytes);
		SAlloc::F(*ExtensionBlocks);
		*ExtensionBlocks = NULL;
		*ExtensionBlockCount = 0;
	}
}
// 
// Image block allocation functions
// 
// 
// Frees the last image in the GifFile->SavedImages array
// 
void FreeLastSavedImage(GifFileType * pGifFile)
{
	if(pGifFile && pGifFile->SavedImages) {
		// Remove one GifSavedImage from the GifFile 
		pGifFile->ImageCount--;
		GifSavedImage * sp = &pGifFile->SavedImages[pGifFile->ImageCount];
		// Deallocate its Colormap 
		if(sp->ImageDesc.ColorMap) {
			GifFreeMapObject(sp->ImageDesc.ColorMap);
			sp->ImageDesc.ColorMap = NULL;
		}
		SAlloc::F((char *)sp->RasterBits); // Deallocate the image data 
		GifFreeExtensions(&sp->ExtensionBlockCount, &sp->ExtensionBlocks); // Deallocate any extensions 
		/*** FIXME: We could SAlloc::R the GifFile->SavedImages structure but is
		 * there a point to it? Saves some memory but we'd have to do it every
		 * time.  If this is used in GifFreeSavedImages then it would be inefficient
		 * (The whole array is going to be deallocated.)  If we just use it when
		 * we want to SAlloc::F the last Image it's convenient to do it here.
		 */
	}
}
// 
// Append an image block to the SavedImages array
// 
GifSavedImage * GifMakeSavedImage(GifFileType * pGifFile, const GifSavedImage * pCopyFrom)
{
	GifSavedImage * sp = 0;
	if(pGifFile->SavedImages == NULL)
		pGifFile->SavedImages = static_cast<GifSavedImage *>(SAlloc::M(sizeof(GifSavedImage)));
	else
		pGifFile->SavedImages = static_cast<GifSavedImage *>(SAlloc::R(pGifFile->SavedImages, sizeof(GifSavedImage) * (pGifFile->ImageCount + 1)));
	if(pGifFile->SavedImages) {
		sp = &pGifFile->SavedImages[pGifFile->ImageCount++];
		memzero(sp, sizeof(GifSavedImage));
		if(pCopyFrom) {
			memcpy(sp, pCopyFrom, sizeof(GifSavedImage));
			/*
			 * Make our own allocated copies of the heap fields in the
			 * copied record.  This guards against potential aliasing problems.
			 */
			// first, the local color map 
			if(sp->ImageDesc.ColorMap) {
				sp->ImageDesc.ColorMap = GifMakeMapObject(pCopyFrom->ImageDesc.ColorMap->ColorCount, pCopyFrom->ImageDesc.ColorMap->Colors);
				if(!sp->ImageDesc.ColorMap) {
					FreeLastSavedImage(pGifFile);
					return 0;
				}
			}
			// next, the raster 
			sp->RasterBits = static_cast<uint8 *>(SAlloc::M(sizeof(GifPixelType) * pCopyFrom->ImageDesc.Height * pCopyFrom->ImageDesc.Width));
			if(!sp->RasterBits) {
				FreeLastSavedImage(pGifFile);
				return 0;
			}
			else {
				memcpy(sp->RasterBits, pCopyFrom->RasterBits, sizeof(GifPixelType) * pCopyFrom->ImageDesc.Height * pCopyFrom->ImageDesc.Width);
				// finally, the extension blocks 
				if(sp->ExtensionBlocks) {
					sp->ExtensionBlocks = static_cast<ExtensionBlock *>(SAlloc::M(sizeof(ExtensionBlock) * pCopyFrom->ExtensionBlockCount));
					if(sp->ExtensionBlocks == NULL) {
						FreeLastSavedImage(pGifFile);
						return 0;
					}
					memcpy(sp->ExtensionBlocks, pCopyFrom->ExtensionBlocks, sizeof(ExtensionBlock) * pCopyFrom->ExtensionBlockCount);
				}
			}
		}
	}
	return sp;
}

void GifFreeSavedImages(GifFileType * pGifFile)
{
	GifSavedImage * sp;
	if(pGifFile && pGifFile->SavedImages) {
		for(sp = pGifFile->SavedImages; sp < pGifFile->SavedImages + pGifFile->ImageCount; sp++) {
			if(sp->ImageDesc.ColorMap) {
				GifFreeMapObject(sp->ImageDesc.ColorMap);
				sp->ImageDesc.ColorMap = NULL;
			}
			SAlloc::F(sp->RasterBits);
			GifFreeExtensions(&sp->ExtensionBlockCount, &sp->ExtensionBlocks);
		}
		ZFREE(pGifFile->SavedImages);
	}
}
// 
// quantize.c - quantize a high resolution image into lower one
// 
// Based on: "Color Image Quantization for frame buffer Display", by
// Paul Heckbert SIGGRAPH 1982 page 297-307.
// 
// This doesn't really belong in the core library, was undocumented,
// and was removed in 4.2.  Then it turned out some client apps were
// actually using it, so it was restored in 5.0.
// 
typedef struct QuantizedColorType {
	GifByteType RGB[3];
	GifByteType NewColorIndex;
	long Count;
	struct QuantizedColorType * Pnext;
} QuantizedColorType;

typedef struct NewColorMapType {
	GifByteType RGBMin[3], RGBWidth[3];
	uint NumEntries; /* # of QuantizedColorType in linked list below */
	ulong Count; /* Total number of pixels in all the entries */
	QuantizedColorType * QuantizedColors;
} NewColorMapType;
//
// Routine called by qsort to compare two entries.
//
static int SortCmpRtn(const void * Entry1, const void * Entry2) 
{
	return (*((QuantizedColorType**)Entry1))->RGB[SortRGBAxis] - (*((QuantizedColorType**)Entry2))->RGB[SortRGBAxis];
}
//
// Routine to subdivide the RGB space recursively using median cut in each
// axes alternatingly until ColorMapSize different cubes exists.
// The biggest cube in one dimension is subdivide unless it has only one entry.
// Returns GIF_ERROR if failed, otherwise GIF_OK.
// 
static int SubdivColorMap(NewColorMapType * pNewColorSubdiv, uint ColorMapSize, uint * pNewColorMapSize) 
{
	int MaxSize;
	uint i, j, Index = 0, NumEntries, MinColor, MaxColor;
	long Sum, Count;
	QuantizedColorType * QuantizedColor, ** SortArray;
	while(ColorMapSize > *pNewColorMapSize) {
		/* Find candidate for subdivision: */
		MaxSize = -1;
		for(i = 0; i < *pNewColorMapSize; i++) {
			for(j = 0; j < 3; j++) {
				if((((int)pNewColorSubdiv[i].RGBWidth[j]) > MaxSize) && (pNewColorSubdiv[i].NumEntries > 1)) {
					MaxSize = pNewColorSubdiv[i].RGBWidth[j];
					Index = i;
					SortRGBAxis = j;
				}
			}
		}
		if(MaxSize == -1)
			return GIF_OK;

		/* Split the entry Index into two along the axis SortRGBAxis: */

		/* Sort all elements in that entry along the given axis and split at
		 * the median.  */
		SortArray = static_cast<QuantizedColorType **>(SAlloc::M(sizeof(QuantizedColorType *) * pNewColorSubdiv[Index].NumEntries));
		if(SortArray == NULL)
			return GIF_ERROR;
		for(j = 0, QuantizedColor = pNewColorSubdiv[Index].QuantizedColors; j < pNewColorSubdiv[Index].NumEntries && QuantizedColor; j++, QuantizedColor = QuantizedColor->Pnext)
			SortArray[j] = QuantizedColor;
		qsort(SortArray, pNewColorSubdiv[Index].NumEntries, sizeof(QuantizedColorType *), SortCmpRtn);
		// Relink the sorted list into one: 
		for(j = 0; j < pNewColorSubdiv[Index].NumEntries - 1; j++)
			SortArray[j]->Pnext = SortArray[j + 1];
		SortArray[pNewColorSubdiv[Index].NumEntries - 1]->Pnext = NULL;
		pNewColorSubdiv[Index].QuantizedColors = QuantizedColor = SortArray[0];
		SAlloc::F((char *)SortArray);
		// Now simply add the Counts until we have half of the Count: 
		Sum = pNewColorSubdiv[Index].Count / 2 - QuantizedColor->Count;
		NumEntries = 1;
		Count = QuantizedColor->Count;
		while(QuantizedColor->Pnext && (Sum -= QuantizedColor->Pnext->Count) >= 0 && QuantizedColor->Pnext->Pnext) {
			QuantizedColor = QuantizedColor->Pnext;
			NumEntries++;
			Count += QuantizedColor->Count;
		}
		/* Save the values of the last color of the first half, and first
		 * of the second half so we can update the Bounding Boxes later.
		 * Also as the colors are quantized and the BBoxes are full 0..255,
		 * they need to be rescaled.
		 */
		MaxColor = QuantizedColor->RGB[SortRGBAxis]; /* Max. of first half */
		/* coverity[var_deref_op] */
		MinColor = QuantizedColor->Pnext->RGB[SortRGBAxis]; /* of second */
		MaxColor <<= (8 - BITS_PER_PRIM_COLOR);
		MinColor <<= (8 - BITS_PER_PRIM_COLOR);

		/* Partition right here: */
		pNewColorSubdiv[*pNewColorMapSize].QuantizedColors = QuantizedColor->Pnext;
		QuantizedColor->Pnext = NULL;
		pNewColorSubdiv[*pNewColorMapSize].Count = Count;
		pNewColorSubdiv[Index].Count -= Count;
		pNewColorSubdiv[*pNewColorMapSize].NumEntries = pNewColorSubdiv[Index].NumEntries - NumEntries;
		pNewColorSubdiv[Index].NumEntries = NumEntries;
		for(j = 0; j < 3; j++) {
			pNewColorSubdiv[*pNewColorMapSize].RGBMin[j] = pNewColorSubdiv[Index].RGBMin[j];
			pNewColorSubdiv[*pNewColorMapSize].RGBWidth[j] = pNewColorSubdiv[Index].RGBWidth[j];
		}
		pNewColorSubdiv[*pNewColorMapSize].RGBWidth[SortRGBAxis] = pNewColorSubdiv[*pNewColorMapSize].RGBMin[SortRGBAxis] + pNewColorSubdiv[*pNewColorMapSize].RGBWidth[SortRGBAxis] - MinColor;
		pNewColorSubdiv[*pNewColorMapSize].RGBMin[SortRGBAxis] = MinColor;
		pNewColorSubdiv[Index].RGBWidth[SortRGBAxis] = MaxColor - pNewColorSubdiv[Index].RGBMin[SortRGBAxis];
		(*pNewColorMapSize)++;
	}
	return GIF_OK;
}
// 
// Quantize high resolution image into lower one. Input image consists of a
// 2D array for each of the RGB colors with size Width by Height. There is no
// Color map for the input. Output is a quantized image with 2D array of indexes into the output color map.
// Note input image can be 24 bits at the most (8 for red/green/blue) and
// the output has 256 colors at the most (256 entries in the color map.).
// ColorMapSize specifies size of color map up to 256 and will be updated to real size before returning.
// Also non of the parameter are allocated by this routine.
// This function returns GIF_OK if successful, GIF_ERROR otherwise.
// 
int GifQuantizeBuffer(uint Width, uint Height, int * pColorMapSize, GifByteType * RedInput, GifByteType * GreenInput, 
	GifByteType * BlueInput, GifByteType * OutputBuffer, SColorRGB * OutputColorMap) 
{
	uint Index, NumOfEntries;
	int i, j, MaxRGBError[3];
	/*unsigned*/ int NewColorMapSize;
	long Red, Green, Blue;
	NewColorMapType NewColorSubdiv[256];
	QuantizedColorType * ColorArrayEntries, * QuantizedColor;
	ColorArrayEntries = static_cast<QuantizedColorType *>(SAlloc::M(sizeof(QuantizedColorType) * COLOR_ARRAY_SIZE));
	if(ColorArrayEntries == NULL) {
		return GIF_ERROR;
	}
	for(i = 0; i < COLOR_ARRAY_SIZE; i++) {
		ColorArrayEntries[i].RGB[0] = i >> (2 * BITS_PER_PRIM_COLOR);
		ColorArrayEntries[i].RGB[1] = (i >> BITS_PER_PRIM_COLOR) & MAX_PRIM_COLOR;
		ColorArrayEntries[i].RGB[2] = i & MAX_PRIM_COLOR;
		ColorArrayEntries[i].Count = 0;
	}
	/* Sample the colors and their distribution: */
	for(i = 0; i < (int)(Width * Height); i++) {
		Index = ((RedInput[i] >> (8 - BITS_PER_PRIM_COLOR)) << (2 * BITS_PER_PRIM_COLOR)) +
		    ((GreenInput[i] >> (8 - BITS_PER_PRIM_COLOR)) << BITS_PER_PRIM_COLOR) +
		    (BlueInput[i] >> (8 - BITS_PER_PRIM_COLOR));
		ColorArrayEntries[Index].Count++;
	}
	/* Put all the colors in the first entry of the color map, and call the
	 * recursive subdivision process.  */
	for(i = 0; i < 256; i++) {
		NewColorSubdiv[i].QuantizedColors = NULL;
		NewColorSubdiv[i].Count = NewColorSubdiv[i].NumEntries = 0;
		for(j = 0; j < 3; j++) {
			NewColorSubdiv[i].RGBMin[j] = 0;
			NewColorSubdiv[i].RGBWidth[j] = 255;
		}
	}
	/* Find the non empty entries in the color table and chain them: */
	for(i = 0; i < COLOR_ARRAY_SIZE; i++)
		if(ColorArrayEntries[i].Count > 0)
			break;
	QuantizedColor = NewColorSubdiv[0].QuantizedColors = &ColorArrayEntries[i];
	NumOfEntries = 1;
	while(++i < COLOR_ARRAY_SIZE)
		if(ColorArrayEntries[i].Count > 0) {
			QuantizedColor->Pnext = &ColorArrayEntries[i];
			QuantizedColor = &ColorArrayEntries[i];
			NumOfEntries++;
		}
	QuantizedColor->Pnext = NULL;

	NewColorSubdiv[0].NumEntries = NumOfEntries; /* Different sampled colors */
	NewColorSubdiv[0].Count = ((long)Width) * Height; /* Pixels */
	NewColorMapSize = 1;
	if(SubdivColorMap(NewColorSubdiv, *pColorMapSize, (uint *)&NewColorMapSize) != GIF_OK) {
		SAlloc::F((char *)ColorArrayEntries);
		return GIF_ERROR;
	}
	if(NewColorMapSize < *pColorMapSize) {
		// And clear rest of color map: 
		for(i = NewColorMapSize; i < *pColorMapSize; i++)
			OutputColorMap[i].Set(0);
	}
	/* Average the colors in each entry to be the color to be used in the
	 * output color map, and plug it into the output color map itself. */
	for(i = 0; i < NewColorMapSize; i++) {
		if((j = NewColorSubdiv[i].NumEntries) > 0) {
			QuantizedColor = NewColorSubdiv[i].QuantizedColors;
			Red = Green = Blue = 0;
			while(QuantizedColor) {
				QuantizedColor->NewColorIndex = i;
				Red += QuantizedColor->RGB[0];
				Green += QuantizedColor->RGB[1];
				Blue += QuantizedColor->RGB[2];
				QuantizedColor = QuantizedColor->Pnext;
			}
			OutputColorMap[i].R = (uint8)((Red << (8 - BITS_PER_PRIM_COLOR)) / j);
			OutputColorMap[i].G = (uint8)((Green << (8 - BITS_PER_PRIM_COLOR)) / j);
			OutputColorMap[i].B = (uint8)((Blue << (8 - BITS_PER_PRIM_COLOR)) / j);
		}
	}
	// 
	// Finally scan the input buffer again and put the mapped index in the output buffer.
	//
	MaxRGBError[0] = MaxRGBError[1] = MaxRGBError[2] = 0;
	for(i = 0; i < (int)(Width * Height); i++) {
		Index = ((RedInput[i] >> (8 - BITS_PER_PRIM_COLOR)) << (2 * BITS_PER_PRIM_COLOR)) +
		    ((GreenInput[i] >> (8 - BITS_PER_PRIM_COLOR)) << BITS_PER_PRIM_COLOR) + (BlueInput[i] >> (8 - BITS_PER_PRIM_COLOR));
		Index = ColorArrayEntries[Index].NewColorIndex;
		OutputBuffer[i] = Index;
		if(MaxRGBError[0] < ABS(OutputColorMap[Index].R - RedInput[i]))
			MaxRGBError[0] = ABS(OutputColorMap[Index].R - RedInput[i]);
		if(MaxRGBError[1] < ABS(OutputColorMap[Index].G - GreenInput[i]))
			MaxRGBError[1] = ABS(OutputColorMap[Index].G - GreenInput[i]);
		if(MaxRGBError[2] < ABS(OutputColorMap[Index].B - BlueInput[i]))
			MaxRGBError[2] = ABS(OutputColorMap[Index].B - BlueInput[i]);
	}
#ifdef DEBUG
	fprintf(stderr, "Quantization L(0) errors: Red = %d, Green = %d, Blue = %d.\n", MaxRGBError[0], MaxRGBError[1], MaxRGBError[2]);
#endif /* DEBUG */
	SAlloc::F((char *)ColorArrayEntries);
	*pColorMapSize = NewColorMapSize;
	return GIF_OK;
}
