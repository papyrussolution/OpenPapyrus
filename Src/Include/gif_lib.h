// gif_lib.h - service library for decoding and encoding GIF images
// @codepage UTF-8
// 
#ifndef _GIF_LIB_H_
#define _GIF_LIB_H_ 1

#include <slib.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GIFLIB_MAJOR 5
#define GIFLIB_MINOR 0
#define GIFLIB_RELEASE 1

#define GIF_ERROR   0
#define GIF_OK      1

// @sobolev #include <stdbool.h>

#define GIF_STAMP "GIFVER"          /* First chars in file - GIF stamp.  */
#define GIF_STAMP_LEN (sizeof(GIF_STAMP)-1)
#define GIF_VERSION_POS 3           /* Version first character in stamp. */
#define GIF87_STAMP "GIF87a"        /* First chars in file - GIF stamp.  */
#define GIF89_STAMP "GIF89a"        /* First chars in file - GIF stamp.  */

typedef uint8 GifPixelType;
typedef uint8 * GifRowType;
//typedef uint8 GifByteType__Removed;
typedef uint GifPrefixType;
//typedef int GifWord_Removed;

/*#if !defined(__BOOL_DEFINED)
	#define bool int
	#define true 1
	#define false 0
#endif*/
#ifndef SIZE_MAX
	#define SIZE_MAX ((size_t)-1)
#endif

/* Replaced with SColorRGB
struct GifColorType {
	uint8  Red;
	uint8  Green;
	uint8  Blue;
};
*/

struct ColorMapObject {
	int    ColorCount;
	int    BitsPerPixel;
	bool   SortFlag;
	uint8  Reserve[3]; // @v11.3.4 @alignment
	SColorRGB * Colors; // on SAlloc::M(3) heap 
};

struct GifImageDesc {
	int    Left;
	int    Top;
	int    Width;
	int    Height; // Current image dimensions
	bool   Interlace; // Sequential/Interlaced lines
	uint8  Reserve[3]; // @v11.3.4 @alignment
	ColorMapObject * ColorMap; // The local color map
};

struct ExtensionBlock {
	int    ByteCount;
	uint8 * Bytes; /* on SAlloc::M(3) heap */
	int    Function;   /* The block function code */
#define CONTINUE_EXT_FUNC_CODE    0x00    /* continuation subblock */
#define COMMENT_EXT_FUNC_CODE     0xfe    /* comment */
#define GRAPHICS_EXT_FUNC_CODE    0xf9    /* graphics control (GIF89) */
#define PLAINTEXT_EXT_FUNC_CODE   0x01    /* plaintext */
#define APPLICATION_EXT_FUNC_CODE 0xff    /* application block */
};

struct GifSavedImage {
	GifImageDesc ImageDesc;
	uint8 * RasterBits;    /* on SAlloc::M(3) heap */
	int    ExtensionBlockCount;     /* Count of extensions before image */
	ExtensionBlock * ExtensionBlocks; /* Extensions before image */
};

struct GifFileType {
	int    SWidth;
	int    SHeight; // Size of virtual canvas 
	int    SColorResolution; // How many colors can we generate? 
	int    SBackGroundColor; // Background color for virtual canvas 
	uint8  AspectByte; // Used to compute pixel aspect ratio
	uint8  Reserve[3]; // @v11.3.4 @aligment
	ColorMapObject * SColorMap; // Global colormap, NULL if nonexistent
	int    ImageCount;          // Number of current image (both APIs) 
	GifImageDesc Image;         // Current image (low-level API)
	GifSavedImage * SavedImages; // Image sequence (high-level API) 
	int    ExtensionBlockCount;  // Count extensions past last image 
	ExtensionBlock * ExtensionBlocks; // Extensions past last image
	int    Error;    // Last error condition reported
	void * UserData; // hook to attach user data (TVT) 
	void * Private;  // Don't mess with this! 
};

#define GIF_ASPECT_RATIO(n)     ((n)+15.0/64.0)

enum GifRecordType {
	UNDEFINED_RECORD_TYPE,
	SCREEN_DESC_RECORD_TYPE,
	IMAGE_DESC_RECORD_TYPE, /* Begin with ',' */
	EXTENSION_RECORD_TYPE, /* Begin with '!' */
	TERMINATE_RECORD_TYPE /* Begin with ';' */
};
//
// func type to read gif data from arbitrary sources (TVT)
//
typedef int (*InputFunc)(GifFileType *, uint8 *, int);
//
// func type to write gif data to arbitrary targets.
// Returns count of bytes written. (MRB)
//
typedef int (*OutputFunc)(GifFileType *, const uint8 *, int);
// 
// GIF89 structures
// 
typedef struct GraphicsControlBlock {
	int    DisposalMode;
#define DISPOSAL_UNSPECIFIED      0 // No disposal specified
#define DISPOSE_DO_NOT            1 // Leave image in place 
#define DISPOSE_BACKGROUND        2 // Set area too background color 
#define DISPOSE_PREVIOUS          3 // Restore to previous content 
	bool   UserInputFlag;    // User confirmation required before disposal 
	uint8  Reserve[3];       // @v11.3.4 @alignment 
	int    DelayTime;        // pre-display delay in 0.01sec units 
	int    TransparentColor; // Palette index for transparency, -1 if none 
#define NO_TRANSPARENT_COLOR    -1
} GraphicsControlBlock;
// 
// GIF encoding routines
// 
// Main entry points 
GifFileType * EGifOpenFileName(const char * GifFileName, const bool GifTestExistence, int * Error);
GifFileType * EGifOpenFileHandle(const int GifFileHandle, int * Error);
GifFileType * EGifOpen(void * userPtr, OutputFunc writeFunc, int * Error);
int EGifSpew(GifFileType * GifFile);
const char * EGifGetGifVersion(GifFileType * GifFile); /* new in 5.x */
int EGifCloseFile(GifFileType * GifFile);

#define E_GIF_ERR_OPEN_FAILED    1    /* And EGif possible errors. */
#define E_GIF_ERR_WRITE_FAILED   2
#define E_GIF_ERR_HAS_SCRN_DSCR  3
#define E_GIF_ERR_HAS_IMAG_DSCR  4
#define E_GIF_ERR_NO_COLOR_MAP   5
#define E_GIF_ERR_DATA_TOO_BIG   6
#define E_GIF_ERR_NOT_ENOUGH_MEM 7
#define E_GIF_ERR_DISK_IS_FULL   8
#define E_GIF_ERR_CLOSE_FAILED   9
#define E_GIF_ERR_NOT_WRITEABLE  10

// These are legacy.  You probably do not want to call them directly 
int EGifPutScreenDesc(GifFileType * GifFile, const int GifWidth, const int GifHeight, const int GifColorRes, const int GifBackGround, const ColorMapObject * GifColorMap);
int EGifPutImageDesc(GifFileType * GifFile, const int GifLeft, const int GifTop, const int GifWidth, const int GifHeight, const bool GifInterlace, const ColorMapObject * GifColorMap);
int EGifSetGifVersion(GifFileType * GifFile, const bool gif89);
int EGifPutLine(GifFileType * GifFile, GifPixelType * GifLine, int GifLineLen);
int EGifPutPixel(GifFileType * GifFile, GifPixelType GifPixel);
int EGifPutComment(GifFileType * GifFile, const char * GifComment);
int EGifPutExtensionLeader(GifFileType * GifFile, const int GifExtCode);
int EGifPutExtensionBlock(GifFileType * GifFile, const int GifExtLen, const void * GifExtension);
int EGifPutExtensionTrailer(GifFileType * GifFile);
int EGifPutExtension(GifFileType * GifFile, const int GifExtCode, const int GifExtLen, const void * GifExtension);
int EGifPutCode(GifFileType * GifFile, int GifCodeSize, const uint8 * GifCodeBlock);
int EGifPutCodeNext(GifFileType * GifFile, const uint8 * GifCodeBlock);
// 
// GIF decoding routines
// 
// Main entry points 
GifFileType * DGifOpenFileName(const char * GifFileName, int * Error);
GifFileType * DGifOpenFileHandle(int GifFileHandle, int * Error);
int DGifSlurp(GifFileType * GifFile);
GifFileType * DGifOpen(void * userPtr, InputFunc readFunc, int * Error);    /* new one (TVT) */
int DGifCloseFile(GifFileType * GifFile);
int DGifCloseFile2(GifFileType * GifFile, int * pErrorCode); // @v11.2.8 Это - прототип реализации из более свежей версии GifLib

#define D_GIF_SUCCEEDED            0
#define D_GIF_ERR_OPEN_FAILED    101 // And DGif possible errors
#define D_GIF_ERR_READ_FAILED    102
#define D_GIF_ERR_NOT_GIF_FILE   103
#define D_GIF_ERR_NO_SCRN_DSCR   104
#define D_GIF_ERR_NO_IMAG_DSCR   105
#define D_GIF_ERR_NO_COLOR_MAP   106
#define D_GIF_ERR_WRONG_RECORD   107
#define D_GIF_ERR_DATA_TOO_BIG   108
#define D_GIF_ERR_NOT_ENOUGH_MEM 109
#define D_GIF_ERR_CLOSE_FAILED   110
#define D_GIF_ERR_NOT_READABLE   111
#define D_GIF_ERR_IMAGE_DEFECT   112
#define D_GIF_ERR_EOF_TOO_SOON   113
// 
// These are legacy.  You probably do not want to call them directly 
// 
int DGifGetScreenDesc(GifFileType * GifFile);
int DGifGetRecordType(GifFileType * GifFile, GifRecordType * GifType);
int DGifGetImageDesc(GifFileType * GifFile);
int DGifGetLine(GifFileType * GifFile, GifPixelType * GifLine, int GifLineLen);
int DGifGetPixel(GifFileType * GifFile, GifPixelType GifPixel);
int DGifGetComment(GifFileType * GifFile, char * GifComment);
int DGifGetExtension(GifFileType * GifFile, int * GifExtCode, uint8 ** GifExtension);
int DGifGetExtensionNext(GifFileType * GifFile, uint8 ** GifExtension);
int DGifGetCode(GifFileType * GifFile, int * GifCodeSize, uint8 ** GifCodeBlock);
int DGifGetCodeNext(GifFileType * GifFile, uint8 ** GifCodeBlock);
int DGifGetLZCodes(GifFileType * GifFile, int * GifCode);
// 
// Color table quantization (deprecated)
// 
int GifQuantizeBuffer(uint Width, uint Height, int * ColorMapSize, uint8 * RedInput,
    uint8 * GreenInput, uint8 * BlueInput, uint8 * OutputBuffer, SColorRGB * OutputColorMap);
// 
// Error handling and reporting.
// 
extern const char * GifErrorString(int ErrorCode);     /* new in 2012 - ESR */
// 
// Everything below this point is new after version 1.2, supporting `slurp
// mode' for doing I/O in two big belts with all the image-bashing in core.
// 
// 
// Color map handling from gif_alloc.c
// 
extern ColorMapObject * FASTCALL GifMakeMapObject(int ColorCount, const SColorRGB * ColorMap);
extern void FASTCALL GifFreeMapObject(ColorMapObject * Object);
extern ColorMapObject * GifUnionColorMap(const ColorMapObject *ColorIn1, const ColorMapObject *ColorIn2, GifPixelType ColorTransIn2[]);
extern int FASTCALL GifBitSize(int n);
// 
// Support for the in-core structures allocation (slurp mode).
// 
extern void GifApplyTranslation(GifSavedImage * Image, GifPixelType Translation[]);
extern int GifAddExtensionBlock(int * ExtensionBlock_Count, ExtensionBlock **ExtensionBlocks, int Function, uint Len, uint8 ExtData[]);
extern void GifFreeExtensions(int * ExtensionBlock_Count, ExtensionBlock ** ExtensionBlocks);
extern GifSavedImage * GifMakeSavedImage(GifFileType * GifFile, const GifSavedImage * CopyFrom);
extern void GifFreeSavedImages(GifFileType * GifFile);
// 
// 5.x functions for GIF89 graphics control blocks
// 
int DGifExtensionToGCB(const size_t GifExtensionLength, const uint8 * GifExtension, GraphicsControlBlock * GCB);
size_t EGifGCBToExtension(const GraphicsControlBlock * GCB, uint8 * GifExtension);
int DGifSavedExtensionToGCB(GifFileType * GifFile, int ImageIndex, GraphicsControlBlock * GCB);
int EGifGCBToSavedExtension(const GraphicsControlBlock * GCB, GifFileType * GifFile, int ImageIndex);
// 
// The library's internal utility font
// 
#define GIF_FONT_WIDTH  8
#define GIF_FONT_HEIGHT 8
extern const uint8 GifAsciiTable8x8[][GIF_FONT_WIDTH];

extern void GifDrawText8x8(GifSavedImage * Image, const int x, const int y, const char * legend, const int color);
extern void GifDrawBox(GifSavedImage * Image, const int x, const int y, const int w, const int d, const int color);
extern void GifDrawRectangle(GifSavedImage * Image, const int x, const int y, const int w, const int d, const int color);
extern void GifDrawBoxedText8x8(GifSavedImage * Image, const int x, const int y, const char * legend, const int border, const int bg, const int fg);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* _GIF_LIB_H */
