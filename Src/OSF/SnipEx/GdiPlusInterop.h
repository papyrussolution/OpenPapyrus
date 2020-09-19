// GdiPlusInterop.h
// Author: Joseph Ryan Ries, 2017
// The snip.exe that comes bundled with Microsoft Windows is *almost* good enough. So I made one just a little better.

// I want to use some bits from GDI+, especially for convenient saving of PNG format images.
// But GDI+ is a C++ API, and this is a purely C project, so... some hacking is required.

#pragma once

DEFINE_GUID(gEncoderCompressionGuid, 0xe09d739d, 0xccd4, 0x44ee, 0x8e, 0xba, 0x3f, 0xbf, 0x8b, 0xe4, 0xfc, 0x58);

typedef enum EncoderParameterValueType {
	EncoderParameterValueTypeByte = 1,
	EncoderParameterValueTypeASCII = 2,
	EncoderParameterValueTypeShort = 3,
	EncoderParameterValueTypeLong = 4,
	EncoderParameterValueTypeRational = 5,
	EncoderParameterValueTypeLongRange = 6,
	EncoderParameterValueTypeUndefined = 7,
	EncoderParameterValueTypeRationalRange = 8,
	EncoderParameterValueTypePointer = 9
} EncoderParameterValueType;

typedef struct EncoderParameter {
	GUID  Guid;               // GUID of the parameter
	ULONG NumberOfValues;     // Number of the parameter values
	ULONG Type;               // Value type, like ValueTypeLONG  etc.
	VOID* Value;              // A pointer to the parameter values
} EncoderParameter;

typedef struct EncoderParameters {
	UINT Count;                      // Number of parameters in this structure
	EncoderParameter Parameter[1];   // Parameter values
} EncoderParameters;

typedef struct GdiplusStartupInput {
	UINT32 GdiplusVersion;
	void*  DebugEventCallback;
	BOOL   SuppressBackgroundThread;
	BOOL   SuppressExternalCodecs;
} GdiplusStartupInput;

int (WINAPI* GdiplusStartup)(ULONG_PTR* Token, struct GdiplusStartupInput* Size, void*);
int (WINAPI* GdiplusShutdown)(ULONG_PTR Token);
int (WINAPI* GdipCreateBitmapFromHBITMAP)(HBITMAP hBitmap, HPALETTE hPalette, ULONG** Bitmap);
int (WINAPI* GdipDisposeImage)(ULONG* Bitmap);
int (WINAPI* GdipSaveImageToFile)(ULONG* Image, const WCHAR* Filename, const CLSID* clsidEncoder, const EncoderParameters* EncoderParams);