//
//  Little Color Management System
//  Copyright (c) 1998-2020 Marti Maria Saguer
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the Software
// is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
#include "lcms2_internal.h"
#pragma hdrstop

// ----------------------------------------------------------------------------------
// Encoding & Decoding support functions
// ----------------------------------------------------------------------------------

//      Little-Endian to Big-Endian

// Adjust a word value after being read/ before being written from/to an ICC profile
uint16 /*CMSEXPORT*/FASTCALL _cmsAdjustEndianess16(uint16 Word)
{
#ifndef CMS_USE_BIG_ENDIAN
	uint8 * pByte = (uint8 *)&Word;
	uint8 tmp = pByte[0];
	pByte[0] = pByte[1];
	pByte[1] = tmp;
#endif
	return Word;
}

// Transports to properly encoded values - note that icc profiles does use big endian notation.

// 1 2 3 4
// 4 3 2 1

uint32 /*CMSEXPORT*/FASTCALL _cmsAdjustEndianess32(uint32 DWord)
{
#ifndef CMS_USE_BIG_ENDIAN
	uint8 * pByte = (uint8 *)&DWord;
	uint8 temp1;
	uint8 temp2;
	temp1 = *pByte++;
	temp2 = *pByte++;
	*(pByte-1) = *pByte;
	*pByte++ = temp2;
	*(pByte-3) = *pByte;
	*pByte = temp1;
#endif
	return DWord;
}

// 1 2 3 4 5 6 7 8
// 8 7 6 5 4 3 2 1

void /*CMSEXPORT*/FASTCALL _cmsAdjustEndianess64(uint64* Result, uint64* QWord)
{
#ifndef CMS_USE_BIG_ENDIAN
	uint8 * pIn  = (uint8 *)QWord;
	uint8 * pOut = (uint8 *)Result;
	assert(Result != NULL);
	pOut[7] = pIn[0];
	pOut[6] = pIn[1];
	pOut[5] = pIn[2];
	pOut[4] = pIn[3];
	pOut[3] = pIn[4];
	pOut[2] = pIn[5];
	pOut[1] = pIn[6];
	pOut[0] = pIn[7];
#else
	assert(Result != NULL);
#ifdef CMS_DONT_USE_INT64
	(*Result)[0] = (*QWord)[0];
	(*Result)[1] = (*QWord)[1];
#else
	*Result = *QWord;
#endif
#endif
}

// Auxiliary -- read 8, 16 and 32-bit numbers
boolint /*CMSEXPORT*/FASTCALL _cmsReadUInt8Number(cmsIOHANDLER* io, uint8 * n)
{
	uint8 tmp;
	assert(io);
	if(io->Read(io, &tmp, sizeof(uint8), 1) != 1)
		return FALSE;
	ASSIGN_PTR(n, tmp);
	return TRUE;
}

boolint /*CMSEXPORT*/FASTCALL _cmsReadUInt16Number(cmsIOHANDLER* io, uint16* n)
{
	uint16 tmp;
	assert(io);
	if(io->Read(io, &tmp, sizeof(uint16), 1) != 1)
		return FALSE;
	ASSIGN_PTR(n, _cmsAdjustEndianess16(tmp));
	return TRUE;
}

boolint /*CMSEXPORT*/STDCALL _cmsReadUInt16Array(cmsIOHANDLER* io, uint32 n, uint16* Array)
{
	assert(io);
	for(uint32 i = 0; i < n; i++) {
		if(Array) {
			if(!_cmsReadUInt16Number(io, Array + i)) 
				return FALSE;
		}
		else {
			if(!_cmsReadUInt16Number(io, NULL)) 
				return FALSE;
		}
	}
	return TRUE;
}

boolint /*CMSEXPORT*/FASTCALL _cmsReadUInt32Number(cmsIOHANDLER * io, uint32 * n)
{
	uint32 tmp;
	assert(io);
	if(io->Read(io, &tmp, sizeof(uint32), 1) != 1)
		return FALSE;
	ASSIGN_PTR(n, _cmsAdjustEndianess32(tmp));
	return TRUE;
}

boolint /*CMSEXPORT*/FASTCALL _cmsReadFloat32Number(cmsIOHANDLER* io, float* n)
{
	uint32 tmp;
	assert(io);
	if(io->Read(io, &tmp, sizeof(uint32), 1) != 1)
		return FALSE;
	if(n) {
		tmp = _cmsAdjustEndianess32(tmp);
		*n = *(float *)(void *)&tmp;
		// Safeguard which covers against absurd values
		if(*n > 1E+20 || *n < -1E+20) 
			return FALSE;
	#if defined(_MSC_VER) && _MSC_VER < 1800
		return TRUE;
	#elif defined (__BORLANDC__)
		return TRUE;
	#elif !defined(_MSC_VER) && (defined(__STDC_VERSION__) && __STDC_VERSION__ < 199901L)
		return TRUE;
	#else
		// fpclassify() required by C99 (only provided by MSVC >= 1800, VS2013 onwards)
		return ((fpclassify(*n) == FP_ZERO) || (fpclassify(*n) == FP_NORMAL));
	#endif
	}
	return TRUE;
}

boolint /*CMSEXPORT*/FASTCALL _cmsReadUInt64Number(cmsIOHANDLER* io, uint64* n)
{
	uint64 tmp;
	assert(io);
	if(io->Read(io, &tmp, sizeof(uint64), 1) != 1)
		return FALSE;
	if(n) {
		_cmsAdjustEndianess64(n, &tmp);
	}
	return TRUE;
}

boolint /*CMSEXPORT*/FASTCALL _cmsRead15Fixed16Number(cmsIOHANDLER* io, double * n)
{
	uint32 tmp;
	assert(io);
	if(io->Read(io, &tmp, sizeof(uint32), 1) != 1)
		return FALSE;
	if(n) {
		*n = _cms15Fixed16toDouble((cmsS15Fixed16Number)_cmsAdjustEndianess32(tmp));
	}
	return TRUE;
}

boolint CMSEXPORT  _cmsReadXYZNumber(cmsIOHANDLER* io, cmsCIEXYZ* XYZ)
{
	cmsEncodedXYZNumber xyz;
	assert(io);
	if(io->Read(io, &xyz, sizeof(cmsEncodedXYZNumber), 1) != 1) 
		return FALSE;
	if(XYZ) {
		XYZ->X = _cms15Fixed16toDouble((cmsS15Fixed16Number)_cmsAdjustEndianess32((uint32)xyz.X));
		XYZ->Y = _cms15Fixed16toDouble((cmsS15Fixed16Number)_cmsAdjustEndianess32((uint32)xyz.Y));
		XYZ->Z = _cms15Fixed16toDouble((cmsS15Fixed16Number)_cmsAdjustEndianess32((uint32)xyz.Z));
	}
	return TRUE;
}

boolint /*CMSEXPORT*/FASTCALL _cmsWriteUInt8Number(cmsIOHANDLER* io, uint8 n)
{
	assert(io);
	if(io->Write(io, sizeof(uint8), &n) != 1)
		return FALSE;
	return TRUE;
}

boolint /*CMSEXPORT*/FASTCALL _cmsWriteUInt16Number(cmsIOHANDLER* io, uint16 n)
{
	uint16 tmp;
	assert(io);
	tmp = _cmsAdjustEndianess16(n);
	if(io->Write(io, sizeof(uint16), &tmp) != 1)
		return FALSE;
	return TRUE;
}

boolint CMSEXPORT _cmsWriteUInt16Array(cmsIOHANDLER* io, uint32 n, const uint16* Array)
{
	assert(io);
	assert(Array != NULL);
	for(uint32 i = 0; i < n; i++) {
		if(!_cmsWriteUInt16Number(io, Array[i])) return FALSE;
	}
	return TRUE;
}

boolint /*CMSEXPORT*/FASTCALL _cmsWriteUInt32Number(cmsIOHANDLER* io, uint32 n)
{
	uint32 tmp;
	assert(io);
	tmp = _cmsAdjustEndianess32(n);
	if(io->Write(io, sizeof(uint32), &tmp) != 1)
		return FALSE;
	return TRUE;
}

boolint CMSEXPORT _cmsWriteFloat32Number(cmsIOHANDLER* io, float n)
{
	uint32 tmp;
	assert(io);
	tmp = *(uint32 *)(void *)&n;
	tmp = _cmsAdjustEndianess32(tmp);
	if(io->Write(io, sizeof(uint32), &tmp) != 1)
		return FALSE;
	return TRUE;
}

boolint CMSEXPORT _cmsWriteUInt64Number(cmsIOHANDLER* io, uint64* n)
{
	uint64 tmp;
	assert(io);
	_cmsAdjustEndianess64(&tmp, n);
	if(io->Write(io, sizeof(uint64), &tmp) != 1)
		return FALSE;
	return TRUE;
}

boolint /*CMSEXPORT*/FASTCALL _cmsWrite15Fixed16Number(cmsIOHANDLER* io, double n)
{
	uint32 tmp;
	assert(io);
	tmp = _cmsAdjustEndianess32((uint32)_cmsDoubleTo15Fixed16(n));
	if(io->Write(io, sizeof(uint32), &tmp) != 1)
		return FALSE;
	return TRUE;
}

boolint CMSEXPORT _cmsWriteXYZNumber(cmsIOHANDLER* io, const cmsCIEXYZ* XYZ)
{
	cmsEncodedXYZNumber xyz;
	assert(io);
	assert(XYZ != NULL);
	xyz.X = (cmsS15Fixed16Number)_cmsAdjustEndianess32((uint32)_cmsDoubleTo15Fixed16(XYZ->X));
	xyz.Y = (cmsS15Fixed16Number)_cmsAdjustEndianess32((uint32)_cmsDoubleTo15Fixed16(XYZ->Y));
	xyz.Z = (cmsS15Fixed16Number)_cmsAdjustEndianess32((uint32)_cmsDoubleTo15Fixed16(XYZ->Z));
	return io->Write(io,  sizeof(cmsEncodedXYZNumber), &xyz);
}

// from Fixed point 8.8 to double
double CMSEXPORT _cms8Fixed8toDouble(uint16 fixed8)
{
	uint8 lsb = (uint8)(fixed8 & 0xff);
	uint8 msb = (uint8)(((uint16)fixed8 >> 8) & 0xff);
	return (double)((double)msb + ((double)lsb / 256.0));
}

uint16 CMSEXPORT _cmsDoubleTo8Fixed8(double val)
{
	cmsS15Fixed16Number GammaFixed32 = _cmsDoubleTo15Fixed16(val);
	return (uint16)((GammaFixed32 >> 8) & 0xFFFF);
}

// from Fixed point 15.16 to double
double /*CMSEXPORT*/FASTCALL _cms15Fixed16toDouble(cmsS15Fixed16Number fix32)
{
	double floater, mid;
	int Whole, FracPart;
	double sign  = (fix32 < 0 ? -1 : 1);
	fix32 = abs(fix32);
	Whole     = (uint16)(fix32 >> 16) & 0xffff;
	FracPart  = (uint16)(fix32 & 0xffff);
	mid     = (double)FracPart / 65536.0;
	floater = (double)Whole + mid;
	return sign * floater;
}

// from double to Fixed point 15.16
cmsS15Fixed16Number CMSEXPORT _cmsDoubleTo15Fixed16(double v) { return ((cmsS15Fixed16Number)floor((v)*65536.0 + 0.5)); }

// Date/Time functions

void CMSEXPORT _cmsDecodeDateTimeNumber(const cmsDateTimeNumber * Source, struct tm * Dest)
{
	assert(Dest != NULL);
	assert(Source != NULL);
	Dest->tm_sec   = _cmsAdjustEndianess16(Source->seconds);
	Dest->tm_min   = _cmsAdjustEndianess16(Source->minutes);
	Dest->tm_hour  = _cmsAdjustEndianess16(Source->hours);
	Dest->tm_mday  = _cmsAdjustEndianess16(Source->day);
	Dest->tm_mon   = _cmsAdjustEndianess16(Source->month) - 1;
	Dest->tm_year  = _cmsAdjustEndianess16(Source->year) - 1900;
	Dest->tm_wday  = -1;
	Dest->tm_yday  = -1;
	Dest->tm_isdst = 0;
}

void CMSEXPORT _cmsEncodeDateTimeNumber(cmsDateTimeNumber * Dest, const struct tm * Source)
{
	assert(Dest != NULL);
	assert(Source != NULL);
	Dest->seconds = _cmsAdjustEndianess16((uint16)Source->tm_sec);
	Dest->minutes = _cmsAdjustEndianess16((uint16)Source->tm_min);
	Dest->hours   = _cmsAdjustEndianess16((uint16)Source->tm_hour);
	Dest->day     = _cmsAdjustEndianess16((uint16)Source->tm_mday);
	Dest->month   = _cmsAdjustEndianess16((uint16)(Source->tm_mon + 1));
	Dest->year    = _cmsAdjustEndianess16((uint16)(Source->tm_year + 1900));
}

// Read base and return type base
cmsTagTypeSignature CMSEXPORT _cmsReadTypeBase(cmsIOHANDLER* io)
{
	_cmsTagBase Base;
	assert(io);
	if(io->Read(io, &Base, sizeof(_cmsTagBase), 1) != 1)
		return (cmsTagTypeSignature)0;
	return (cmsTagTypeSignature)_cmsAdjustEndianess32(Base.sig);
}

// Setup base marker
boolint CMSEXPORT _cmsWriteTypeBase(cmsIOHANDLER* io, cmsTagTypeSignature sig)
{
	_cmsTagBase Base;
	assert(io);
	Base.sig = (cmsTagTypeSignature)_cmsAdjustEndianess32(sig);
	memzero(&Base.reserved, sizeof(Base.reserved));
	return io->Write(io, sizeof(_cmsTagBase), &Base);
}

boolint CMSEXPORT _cmsReadAlignment(cmsIOHANDLER* io)
{
	uint8 Buffer[4];
	uint32 NextAligned, At;
	uint32 BytesToNextAlignedPos;
	assert(io);
	At = io->Tell(io);
	NextAligned = _cmsALIGNLONG(At);
	BytesToNextAlignedPos = NextAligned - At;
	if(BytesToNextAlignedPos == 0) 
		return TRUE;
	if(BytesToNextAlignedPos > 4) 
		return FALSE;
	return (io->Read(io, Buffer, BytesToNextAlignedPos, 1) == 1);
}

boolint CMSEXPORT _cmsWriteAlignment(cmsIOHANDLER* io)
{
	uint8 Buffer[4];
	uint32 NextAligned, At;
	uint32 BytesToNextAlignedPos;
	assert(io);
	At = io->Tell(io);
	NextAligned = _cmsALIGNLONG(At);
	BytesToNextAlignedPos = NextAligned - At;
	if(BytesToNextAlignedPos == 0) 
		return TRUE;
	if(BytesToNextAlignedPos > 4) 
		return FALSE;
	memzero(Buffer, BytesToNextAlignedPos);
	return io->Write(io, BytesToNextAlignedPos, Buffer);
}

// To deal with text streams. 2K at most
boolint CMSEXPORT _cmsIOPrintf(cmsIOHANDLER* io, const char * frm, ...)
{
	va_list args;
	int len;
	uint8 Buffer[2048];
	boolint rc;
	assert(io);
	assert(frm != NULL);
	va_start(args, frm);
	len = vsnprintf((char *)Buffer, 2047, frm, args);
	if(len < 0) {
		va_end(args);
		return FALSE; // Truncated, which is a fatal error for us
	}
	rc = io->Write(io, (uint32)len, Buffer);
	va_end(args);
	return rc;
}

// Plugin memory management
// -------------------------------------------------------------------------------------------------

// Specialized malloc for plug-ins, that is freed upon exit.
void * _cmsPluginMalloc(cmsContext ContextID, uint32 size)
{
	struct _cmsContext_struct* ctx = _cmsGetContext(ContextID);
	if(ctx->MemPool == NULL) {
		if(ContextID == NULL) {
			ctx->MemPool = _cmsCreateSubAlloc(0, 2*1024);
			if(ctx->MemPool == NULL) 
				return NULL;
		}
		else {
			cmsSignalError(ContextID, cmsERROR_CORRUPTION_DETECTED, "NULL memory pool on context");
			return NULL;
		}
	}
	return _cmsSubAlloc(ctx->MemPool, size);
}

// Main plug-in dispatcher
boolint CMSEXPORT cmsPlugin(void * Plug_in)
{
	return cmsPluginTHR(NULL, Plug_in);
}

boolint CMSEXPORT cmsPluginTHR(cmsContext id, void * Plug_in)
{
	cmsPluginBase* Plugin;
	for(Plugin = (cmsPluginBase*)Plug_in; Plugin; Plugin = Plugin->Next) {
		if(Plugin->Magic != cmsPluginMagicNumber) {
			cmsSignalError(id, cmsERROR_UNKNOWN_EXTENSION, "Unrecognized plugin");
			return FALSE;
		}
		if(Plugin->ExpectedVersion > LCMS_VERSION) {
			cmsSignalError(id, cmsERROR_UNKNOWN_EXTENSION, "plugin needs Little CMS %d, current version is %d", Plugin->ExpectedVersion, LCMS_VERSION);
			return FALSE;
		}
		switch(Plugin->Type) {
			case cmsPluginMemHandlerSig:
			    if(!_cmsRegisterMemHandlerPlugin(id, Plugin)) return FALSE;
			    break;
			case cmsPluginInterpolationSig:
			    if(!_cmsRegisterInterpPlugin(id, Plugin)) return FALSE;
			    break;
			case cmsPluginTagTypeSig:
			    if(!_cmsRegisterTagTypePlugin(id, Plugin)) return FALSE;
			    break;
			case cmsPluginTagSig:
			    if(!_cmsRegisterTagPlugin(id, Plugin)) return FALSE;
			    break;
			case cmsPluginFormattersSig:
			    if(!_cmsRegisterFormattersPlugin(id, Plugin)) return FALSE;
			    break;
			case cmsPluginRenderingIntentSig:
			    if(!_cmsRegisterRenderingIntentPlugin(id, Plugin)) return FALSE;
			    break;
			case cmsPluginParametricCurveSig:
			    if(!_cmsRegisterParametricCurvesPlugin(id, Plugin)) return FALSE;
			    break;
			case cmsPluginMultiProcessElementSig:
			    if(!_cmsRegisterMultiProcessElementPlugin(id, Plugin)) return FALSE;
			    break;
			case cmsPluginOptimizationSig:
			    if(!_cmsRegisterOptimizationPlugin(id, Plugin)) return FALSE;
			    break;
			case cmsPluginTransformSig:
			    if(!_cmsRegisterTransformPlugin(id, Plugin)) return FALSE;
			    break;
			case cmsPluginMutexSig:
			    if(!_cmsRegisterMutexPlugin(id, Plugin)) return FALSE;
			    break;
			default:
			    cmsSignalError(id, cmsERROR_UNKNOWN_EXTENSION, "Unrecognized plugin type '%X'", Plugin->Type);
			    return FALSE;
		}
	}
	// Keep a reference to the plug-in
	return TRUE;
}

// Revert all plug-ins to default
void CMSEXPORT cmsUnregisterPlugins()
{
	cmsUnregisterPluginsTHR(NULL);
}

// The Global storage for system context. This is the one and only global variable
// pointers structure. All global vars are referenced here.
static struct _cmsContext_struct globalContext = {
	NULL,                          // Not in the linked list
	NULL,                          // No suballocator
	{
		NULL,                  //  UserPtr,
		&_cmsLogErrorChunk,    //  Logger,
		&_cmsAlarmCodesChunk,  //  AlarmCodes,
		&_cmsAdaptationStateChunk, //  AdaptationState,
		&_cmsMemPluginChunk,   //  MemPlugin,
		&_cmsInterpPluginChunk, //  InterpPlugin,
		&_cmsCurvesPluginChunk, //  CurvesPlugin,
		&_cmsFormattersPluginChunk, //  FormattersPlugin,
		&_cmsTagTypePluginChunk, //  TagTypePlugin,
		&_cmsTagPluginChunk,   //  TagPlugin,
		&_cmsIntentsPluginChunk, //  IntentPlugin,
		&_cmsMPETypePluginChunk, //  MPEPlugin,
		&_cmsOptimizationPluginChunk, //  OptimizationPlugin,
		&_cmsTransformPluginChunk, //  TransformPlugin,
		&_cmsMutexPluginChunk  //  MutexPlugin
	},

	{ NULL, NULL, NULL, NULL, NULL, NULL } // The default memory allocator is not used for context 0
};

// The context pool (linked list head)
static _cmsMutex _cmsContextPoolHeadMutex = CMS_MUTEX_INITIALIZER;
static struct _cmsContext_struct* _cmsContextPoolHead = NULL;

// Internal, get associated pointer, with guessing. Never returns NULL.
struct _cmsContext_struct * FASTCALL _cmsGetContext(cmsContext ContextID)
{
	struct _cmsContext_struct* id = (struct _cmsContext_struct*)ContextID;
	struct _cmsContext_struct* ctx;
	// On 0, use global settings
	if(id == NULL)
		return &globalContext;
	// Search
	_cmsEnterCriticalSectionPrimitive(&_cmsContextPoolHeadMutex);
	for(ctx = _cmsContextPoolHead; ctx; ctx = ctx->Next) {
		// Found it?
		if(id == ctx) {
			_cmsLeaveCriticalSectionPrimitive(&_cmsContextPoolHeadMutex);
			return ctx; // New-style context
		}
	}
	_cmsLeaveCriticalSectionPrimitive(&_cmsContextPoolHeadMutex);
	return &globalContext;
}

// Internal: get the memory area associanted with each context client
// Returns the block assigned to the specific zone. Never return NULL.
void * FASTCALL _cmsContextGetClientChunk(cmsContext ContextID, _cmsMemoryClient mc)
{
	struct _cmsContext_struct* ctx;
	void * ptr;
	if((int)mc < 0 || mc >= MemoryClientMax) {
		cmsSignalError(ContextID, cmsERROR_INTERNAL, "Bad context client -- possible corruption");
		// This is catastrophic. Should never reach here
		assert(0);
		// Reverts to global context
		return globalContext.chunks[UserPtr];
	}
	ctx = _cmsGetContext(ContextID);
	ptr = ctx->chunks[mc];
	if(ptr)
		return ptr;
	// A null ptr means no special settings for that context, and this
	// reverts to Context0 globals
	return globalContext.chunks[mc];
}

// This function returns the given context its default pristine state,
// as no plug-ins were declared. There is no way to unregister a single
// plug-in, as a single call to cmsPluginTHR() function may register
// many different plug-ins simultaneously, then there is no way to
// identify which plug-in to unregister.
void CMSEXPORT cmsUnregisterPluginsTHR(cmsContext ContextID)
{
	_cmsRegisterMemHandlerPlugin(ContextID, NULL);
	_cmsRegisterInterpPlugin(ContextID, NULL);
	_cmsRegisterTagTypePlugin(ContextID, NULL);
	_cmsRegisterTagPlugin(ContextID, NULL);
	_cmsRegisterFormattersPlugin(ContextID, NULL);
	_cmsRegisterRenderingIntentPlugin(ContextID, NULL);
	_cmsRegisterParametricCurvesPlugin(ContextID, NULL);
	_cmsRegisterMultiProcessElementPlugin(ContextID, NULL);
	_cmsRegisterOptimizationPlugin(ContextID, NULL);
	_cmsRegisterTransformPlugin(ContextID, NULL);
	_cmsRegisterMutexPlugin(ContextID, NULL);
}

// Returns the memory manager plug-in, if any, from the Plug-in bundle
static cmsPluginMemHandler* _cmsFindMemoryPlugin(void * PluginBundle)
{
	for(cmsPluginBase * Plugin = (cmsPluginBase*)PluginBundle; Plugin; Plugin = Plugin->Next) {
		if(Plugin->Magic == cmsPluginMagicNumber && Plugin->ExpectedVersion <= LCMS_VERSION && Plugin->Type == cmsPluginMemHandlerSig) {
			// Found!
			return (cmsPluginMemHandler*)Plugin;
		}
	}
	// Nope, revert to defaults
	return NULL;
}

// Creates a new context with optional associated plug-ins. Caller may also specify an optional pointer to user-defined
// data that will be forwarded to plug-ins and logger.
cmsContext CMSEXPORT cmsCreateContext(void * Plugin, void * UserData)
{
	struct _cmsContext_struct* ctx;
	struct _cmsContext_struct fakeContext;
	// See the comments regarding locking in lcms2_internal.h
	// for an explanation of why we need the following code.
#ifndef CMS_NO_PTHREADS
#ifdef CMS_IS_WINDOWS_
#ifndef CMS_RELY_ON_WINDOWS_STATIC_MUTEX_INIT
	{
		static HANDLE _cmsWindowsInitMutex = NULL;
		static volatile HANDLE* mutex = &_cmsWindowsInitMutex;
		if(*mutex == NULL) {
			HANDLE p = CreateMutex(NULL, FALSE, NULL);
			if(p && InterlockedCompareExchangePointer((void **)mutex, (void *)p, NULL))
				CloseHandle(p);
		}
		if(*mutex == NULL || WaitForSingleObject(*mutex, INFINITE) == WAIT_FAILED)
			return NULL;
		if(((void **)&_cmsContextPoolHeadMutex)[0] == NULL)
			InitializeCriticalSection(&_cmsContextPoolHeadMutex);
		if(*mutex == NULL || !ReleaseMutex(*mutex))
			return NULL;
	}
#endif
#endif
#endif

	_cmsInstallAllocFunctions(_cmsFindMemoryPlugin(Plugin), &fakeContext.DefaultMemoryManager);

	fakeContext.chunks[UserPtr]     = UserData;
	fakeContext.chunks[MemPlugin]   = &fakeContext.DefaultMemoryManager;

	// Create the context structure.
	ctx = (struct _cmsContext_struct*)_cmsMalloc(&fakeContext, sizeof(struct _cmsContext_struct));
	if(!ctx)
		return NULL; // Something very wrong happened!

	// Init the structure and the memory manager
	memzero(ctx, sizeof(struct _cmsContext_struct));
	// Keep memory manager
	memcpy(&ctx->DefaultMemoryManager, &fakeContext.DefaultMemoryManager, sizeof(_cmsMemPluginChunk));
	// Maintain the linked list (with proper locking)
	_cmsEnterCriticalSectionPrimitive(&_cmsContextPoolHeadMutex);
	ctx->Next = _cmsContextPoolHead;
	_cmsContextPoolHead = ctx;
	_cmsLeaveCriticalSectionPrimitive(&_cmsContextPoolHeadMutex);

	ctx->chunks[UserPtr]     = UserData;
	ctx->chunks[MemPlugin]   = &ctx->DefaultMemoryManager;

	// Now we can allocate the pool by using default memory manager
	ctx->MemPool = _cmsCreateSubAlloc(ctx, 22 * sizeof(void *)); // default size about 22 pointers
	if(ctx->MemPool == NULL) {
		cmsDeleteContext(ctx);
		return NULL;
	}

	_cmsAllocLogErrorChunk(ctx, NULL);
	_cmsAllocAlarmCodesChunk(ctx, NULL);
	_cmsAllocAdaptationStateChunk(ctx, NULL);
	_cmsAllocMemPluginChunk(ctx, NULL);
	_cmsAllocInterpPluginChunk(ctx, NULL);
	_cmsAllocCurvesPluginChunk(ctx, NULL);
	_cmsAllocFormattersPluginChunk(ctx, NULL);
	_cmsAllocTagTypePluginChunk(ctx, NULL);
	_cmsAllocMPETypePluginChunk(ctx, NULL);
	_cmsAllocTagPluginChunk(ctx, NULL);
	_cmsAllocIntentsPluginChunk(ctx, NULL);
	_cmsAllocOptimizationPluginChunk(ctx, NULL);
	_cmsAllocTransformPluginChunk(ctx, NULL);
	_cmsAllocMutexPluginChunk(ctx, NULL);

	// Setup the plug-ins
	if(!cmsPluginTHR(ctx, Plugin)) {
		cmsDeleteContext(ctx);
		return NULL;
	}

	return (cmsContext)ctx;
}

// Duplicates a context with all associated plug-ins.
// Caller may specify an optional pointer to user-defined
// data that will be forwarded to plug-ins and logger.
cmsContext CMSEXPORT cmsDupContext(cmsContext ContextID, void * NewUserData)
{
	int i;
	const struct _cmsContext_struct* src = _cmsGetContext(ContextID);
	void * userData = (NewUserData) ? NewUserData : src->chunks[UserPtr];
	struct _cmsContext_struct* ctx = (struct _cmsContext_struct*)_cmsMalloc(ContextID, sizeof(struct _cmsContext_struct));
	if(!ctx)
		return NULL; // Something very wrong happened
	// Setup default memory allocators
	memcpy(&ctx->DefaultMemoryManager, &src->DefaultMemoryManager, sizeof(ctx->DefaultMemoryManager));
	// Maintain the linked list
	_cmsEnterCriticalSectionPrimitive(&_cmsContextPoolHeadMutex);
	ctx->Next = _cmsContextPoolHead;
	_cmsContextPoolHead = ctx;
	_cmsLeaveCriticalSectionPrimitive(&_cmsContextPoolHeadMutex);

	ctx->chunks[UserPtr]    = userData;
	ctx->chunks[MemPlugin]  = &ctx->DefaultMemoryManager;

	ctx->MemPool = _cmsCreateSubAlloc(ctx, 22 * sizeof(void *));
	if(ctx->MemPool == NULL) {
		cmsDeleteContext(ctx);
		return NULL;
	}

	// Allocate all required chunks.
	_cmsAllocLogErrorChunk(ctx, src);
	_cmsAllocAlarmCodesChunk(ctx, src);
	_cmsAllocAdaptationStateChunk(ctx, src);
	_cmsAllocMemPluginChunk(ctx, src);
	_cmsAllocInterpPluginChunk(ctx, src);
	_cmsAllocCurvesPluginChunk(ctx, src);
	_cmsAllocFormattersPluginChunk(ctx, src);
	_cmsAllocTagTypePluginChunk(ctx, src);
	_cmsAllocMPETypePluginChunk(ctx, src);
	_cmsAllocTagPluginChunk(ctx, src);
	_cmsAllocIntentsPluginChunk(ctx, src);
	_cmsAllocOptimizationPluginChunk(ctx, src);
	_cmsAllocTransformPluginChunk(ctx, src);
	_cmsAllocMutexPluginChunk(ctx, src);

	// Make sure no one failed
	for(i = Logger; i < MemoryClientMax; i++) {
		if(src->chunks[i] == NULL) {
			cmsDeleteContext((cmsContext)ctx);
			return NULL;
		}
	}

	return (cmsContext)ctx;
}

// Frees any resources associated with the given context,
// and destroys the context placeholder.
// The ContextID can no longer be used in any THR operation.
void CMSEXPORT cmsDeleteContext(cmsContext ContextID)
{
	if(ContextID) {
		struct _cmsContext_struct* ctx = (struct _cmsContext_struct*)ContextID;
		struct _cmsContext_struct fakeContext;
		struct _cmsContext_struct* prev;
		memcpy(&fakeContext.DefaultMemoryManager, &ctx->DefaultMemoryManager, sizeof(ctx->DefaultMemoryManager));
		fakeContext.chunks[UserPtr]     = ctx->chunks[UserPtr];
		fakeContext.chunks[MemPlugin]   = &fakeContext.DefaultMemoryManager;
		// Get rid of plugins
		cmsUnregisterPluginsTHR(ContextID);
		// Since all memory is allocated in the private pool, all what we need to do is destroy the pool
		if(ctx->MemPool)
			_cmsSubAllocDestroy(ctx->MemPool);
		ctx->MemPool = NULL;
		// Maintain list
		_cmsEnterCriticalSectionPrimitive(&_cmsContextPoolHeadMutex);
		if(_cmsContextPoolHead == ctx) {
			_cmsContextPoolHead = ctx->Next;
		}
		else {
			// Search for previous
			for(prev = _cmsContextPoolHead; prev; prev = prev->Next) {
				if(prev->Next == ctx) {
					prev->Next = ctx->Next;
					break;
				}
			}
		}
		_cmsLeaveCriticalSectionPrimitive(&_cmsContextPoolHeadMutex);
		// free the memory block itself
		_cmsFree(&fakeContext, ctx);
	}
}

// Returns the user data associated to the given ContextID, or NULL if no user data was attached on context creation
void * CMSEXPORT cmsGetContextUserData(cmsContext ContextID)
{
	return _cmsContextGetClientChunk(ContextID, UserPtr);
}
