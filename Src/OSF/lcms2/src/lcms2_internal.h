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
#ifndef _lcms_internal_H

#include <slib.h>
// Include plug-in foundation
#ifndef _lcms_plugin_H
	#include "lcms2_plugin.h"
#endif

// Some needed constants
//#ifndef M_PI
	//#define M_PI        3.14159265358979323846
//#endif
#ifndef M_LOG10E
	#define M_LOG10E    0.434294481903251827651
#endif
// BorlandC 5.5, VC2003 are broken on that
#if defined(__BORLANDC__) || (defined(_MSC_VER) && (_MSC_VER < 1400)) // 1400 == VC++ 8.0
	#define sinf(x) (float)sin((float)x)
	#define sqrtf(x) (float)sqrt((float)x)
#endif
// Alignment of ICC file format uses 4 bytes (uint32)
#define _cmsALIGNLONG(x) (((x)+(sizeof(uint32)-1)) & ~(sizeof(uint32)-1))

// Alignment to memory pointer

// (Ultra)SPARC with gcc requires ptr alignment of 8 bytes
// even though sizeof(void *) is only four: for greatest flexibility
// allow the build to specify ptr alignment.
#ifndef CMS_PTR_ALIGNMENT
	#define CMS_PTR_ALIGNMENT sizeof(void *)
#endif

#define _cmsALIGNMEM(x)  (((x)+(CMS_PTR_ALIGNMENT - 1)) & ~(CMS_PTR_ALIGNMENT - 1))

// Maximum encodeable values in floating point
#define MAX_ENCODEABLE_XYZ  (1.0 + 32767.0/32768.0)
#define MIN_ENCODEABLE_ab2  (-128.0)
#define MAX_ENCODEABLE_ab2  ((65535.0/256.0) - 128.0)
#define MIN_ENCODEABLE_ab4  (-128.0)
#define MAX_ENCODEABLE_ab4  (127.0)

#define MAX_STAGE_CHANNELS  128 // Maximum of channels for internal pipeline evaluation
// @sobolev #define cmsUNUSED_PARAMETER_Removed(x) ((void)x) // Unused parameter warning suppression

// The specification for "inline" is section 6.7.4 of the C99 standard (ISO/IEC 9899:1999).
// unfortunately VisualC++ does not conform that
#if defined(_MSC_VER) || defined(__BORLANDC__)
	#define cmsINLINE __inline
#else
	#define cmsINLINE static inline
#endif
// Allow signed overflow, we know this is harmless in this particular context
#if defined(__clang__)
	#define CMS_NO_SANITIZE __attribute__((no_sanitize("signed-integer-overflow")))
#else
	#define CMS_NO_SANITIZE
#endif
// Other replacement functions
#ifdef _MSC_VER
	#ifndef snprintf
		#define snprintf  _snprintf
	#endif
	#ifndef vsnprintf
		#define vsnprintf  _vsnprintf
	#endif
/// Properly define some macros to accommodate
/// older MSVC versions.
#if defined(_MSC_VER) && _MSC_VER <= 1700
	//#include <float.h>
	#define isnan _isnan
	#define isinf(x) (!_finite((x)))
#endif

#if !defined(_MSC_VER) && (defined(__STDC_VERSION__) && __STDC_VERSION__ < 199901L)
	#if !defined(isinf)
		#define isinf(x) (!finite((x)))
	#endif
#endif
#endif
// A fast way to convert from/to 16 <-> 8 bits
#define FROM_8_TO_16(rgb) (uint16)((((uint16)(rgb)) << 8)|(rgb))
#define FROM_16_TO_8(rgb) (uint8)((((uint32)(rgb) * 65281U + 8388608U) >> 24) & 0xFFU)

// Code analysis is broken on asserts
#ifdef _MSC_VER
#if(_MSC_VER >= 1500)
#define _cmsAssert(a) { assert((a)); __analysis_assume((a)); }
#else
#define _cmsAssert(a)   assert((a))
#endif
#else
#define _cmsAssert(a)   assert((a))
#endif

//---------------------------------------------------------------------------------

// Determinant lower than that are assumed zero (used on matrix invert)
#define MATRIX_DET_TOLERANCE    0.0001

//---------------------------------------------------------------------------------

// Fixed point
#define FIXED_TO_INT(x)         ((x)>>16)
#define FIXED_REST_TO_INT(x)    ((x)&0xFFFFU)
#define ROUND_FIXED_TO_INT(x)   (((x)+0x8000)>>16)

cmsINLINE cmsS15Fixed16Number _cmsToFixedDomain(int a) { return a + ((a + 0x7fff) / 0xffff); }
cmsINLINE int _cmsFromFixedDomain(cmsS15Fixed16Number a) { return a - ((a + 0x7fff) >> 16); }

// -----------------------------------------------------------------------------------------------------------

// Fast floor conversion logic. Thanks to Sree Kotay and Stuart Nixon
// note than this only works in the range ..-32767...+32767 because
// mantissa is interpreted as 15.16 fixed point.
// The union is to avoid pointer aliasing overoptimization.
cmsINLINE int _cmsQuickFloor(double val)
{
#ifdef CMS_DONT_USE_FAST_FLOOR
	return (int)floor(val);
#else
	const double _lcms_double2fixmagic = 68719476736.0 * 1.5; // 2^36 * 1.5, (52-16=36) uses limited
	                                                                    // precision to floor
	union {
		double val;
		int halves[2];
	} temp;

	temp.val = val + _lcms_double2fixmagic;

#ifdef CMS_USE_BIG_ENDIAN
	return temp.halves[1] >> 16;
#else
	return temp.halves[0] >> 16;
#endif
#endif
}

// Fast floor restricted to 0..65535.0
cmsINLINE uint16 _cmsQuickFloorWord(double d) { return (uint16)_cmsQuickFloor(d - 32767.0) + 32767U; }

// Floor to word, taking care of saturation
cmsINLINE uint16 _cmsQuickSaturateWord(double d)
{
	d += 0.5;
	if(d <= 0) return 0;
	if(d >= 65535.0) return 0xffff;
	return _cmsQuickFloorWord(d);
}

// Test bed entry points---------------------------------------------------------------
#define CMSCHECKPOINT CMSAPI

// Pthread support --------------------------------------------------------------------
#ifndef CMS_NO_PTHREADS

// This is the threading support. Unfortunately, it has to be platform-dependent because
// windows does not support pthreads.
#ifdef CMS_IS_WINDOWS_

// @sobolev #define WIN32_LEAN_AND_MEAN 1
// @sobolev #include <windows.h>

// The locking scheme in LCMS requires a single 'top level' mutex
// to work. This is actually implemented on Windows as a
// CriticalSection, because they are lighter weight. With
// pthreads, this is statically inited. Unfortunately, windows
// can't officially statically init critical sections.
//
// We can work around this in 2 ways.
//
// 1) We can use a proper mutex purely to protect the init
// of the CriticalSection. This in turns requires us to protect
// the Mutex creation, which we can do using the snappily
// named InterlockedCompareExchangePointer API (present on
// windows XP and above).
//
// 2) In cases where we want to work on pre-Windows XP, we
// can use an even more horrible hack described below.
//
// So why wouldn't we always use 2)? Because not calling
// the init function for a critical section means it fails
// testing with ApplicationVerifier (and presumably similar
// tools).
//
// We therefore default to 1, and people who want to be able
// to run on pre-Windows XP boxes can build with:
//     CMS_RELY_ON_WINDOWS_STATIC_MUTEX_INIT
// defined. This is automatically set for builds using
// versions of MSVC that don't have this API available.
//
// From: http://locklessinc.com/articles/pthreads_on_windows/
// The pthreads API has an initialization macro that has no correspondence to anything in
// the windows API. By investigating the internal definition of the critical section type,
// one may work out how to initialize one without calling InitializeCriticalSection().
// The trick here is that InitializeCriticalSection() is not allowed to fail. It tries
// to allocate a critical section debug object, but if no memory is available, it sets
// the pointer to a specific value. (One would expect that value to be NULL, but it is
// actually (void *)-1 for some reason.) Thus we can use this special value for that
// pointer, and the critical section code will work.

// The other important part of the critical section type to initialize is the number
// of waiters. This controls whether or not the mutex is locked. Fortunately, this
// part of the critical section is unlikely to change. Apparently, many programs
// already test critical sections to see if they are locked using this value, so
// Microsoft felt that it was necessary to keep it set at -1 for an unlocked critical
// section, even when they changed the underlying algorithm to be more scalable.
// The final parts of the critical section object are unimportant, and can be set
// to zero for their defaults. This yields to an initialization macro:

typedef CRITICAL_SECTION _cmsMutex;

#ifdef _MSC_VER
#if(_MSC_VER >= 1800)
#          pragma warning(disable : 26135)
#endif
#endif

#ifndef CMS_RELY_ON_WINDOWS_STATIC_MUTEX_INIT
// If we are building with a version of MSVC smaller
// than 1400 (i.e. before VS2005) then we don't have
// the InterlockedCompareExchangePointer API, so use
// the old version.
#ifdef _MSC_VER
#if _MSC_VER < 1400
#define CMS_RELY_ON_WINDOWS_STATIC_MUTEX_INIT
#endif
#endif
#endif

#ifdef CMS_RELY_ON_WINDOWS_STATIC_MUTEX_INIT
#define CMS_MUTEX_INITIALIZER {(PRTL_CRITICAL_SECTION_DEBUG)-1, -1, 0, 0, 0, 0}
#else
#define CMS_MUTEX_INITIALIZER {(PRTL_CRITICAL_SECTION_DEBUG)NULL, -1, 0, 0, 0, 0}
#endif

cmsINLINE int _cmsLockPrimitive(_cmsMutex * m)
{
	EnterCriticalSection(m);
	return 0;
}

cmsINLINE int _cmsUnlockPrimitive(_cmsMutex * m)
{
	LeaveCriticalSection(m);
	return 0;
}

cmsINLINE int _cmsInitMutexPrimitive(_cmsMutex * m)
{
	InitializeCriticalSection(m);
	return 0;
}

cmsINLINE int _cmsDestroyMutexPrimitive(_cmsMutex * m)
{
	DeleteCriticalSection(m);
	return 0;
}

cmsINLINE int _cmsEnterCriticalSectionPrimitive(_cmsMutex * m)
{
	EnterCriticalSection(m);
	return 0;
}

cmsINLINE int _cmsLeaveCriticalSectionPrimitive(_cmsMutex * m)
{
	LeaveCriticalSection(m);
	return 0;
}

#else

// Rest of the wide world
#include <pthread.h>

#define CMS_MUTEX_INITIALIZER PTHREAD_MUTEX_INITIALIZER
typedef pthread_mutex_t _cmsMutex;

cmsINLINE int _cmsLockPrimitive(_cmsMutex * m) { return pthread_mutex_lock(m); }
cmsINLINE int _cmsUnlockPrimitive(_cmsMutex * m) { return pthread_mutex_unlock(m); }
cmsINLINE int _cmsInitMutexPrimitive(_cmsMutex * m) { return pthread_mutex_init(m, NULL); }
cmsINLINE int _cmsDestroyMutexPrimitive(_cmsMutex * m) { return pthread_mutex_destroy(m); }
cmsINLINE int _cmsEnterCriticalSectionPrimitive(_cmsMutex * m) { return pthread_mutex_lock(m); }
cmsINLINE int _cmsLeaveCriticalSectionPrimitive(_cmsMutex * m) { return pthread_mutex_unlock(m); }

#endif
#else

#define CMS_MUTEX_INITIALIZER 0
typedef int _cmsMutex;

cmsINLINE int _cmsLockPrimitive(_cmsMutex * m)
{
	CXX_UNUSED(m);
	return 0;
}

cmsINLINE int _cmsUnlockPrimitive(_cmsMutex * m)
{
	CXX_UNUSED(m);
	return 0;
}

cmsINLINE int _cmsInitMutexPrimitive(_cmsMutex * m)
{
	CXX_UNUSED(m);
	return 0;
}

cmsINLINE int _cmsDestroyMutexPrimitive(_cmsMutex * m)
{
	CXX_UNUSED(m);
	return 0;
}

cmsINLINE int _cmsEnterCriticalSectionPrimitive(_cmsMutex * m)
{
	CXX_UNUSED(m);
	return 0;
}

cmsINLINE int _cmsLeaveCriticalSectionPrimitive(_cmsMutex * m)
{
	CXX_UNUSED(m);
	return 0;
}

#endif

// Plug-In registration ---------------------------------------------------------------

// Specialized function for plug-in memory management. No pairing free() since whole pool is freed at once.
void * _cmsPluginMalloc(cmsContext ContextID, uint32 size);
// Memory management
boolint _cmsRegisterMemHandlerPlugin(cmsContext ContextID, cmsPluginBase* Plugin);
// Interpolation
boolint _cmsRegisterInterpPlugin(cmsContext ContextID, cmsPluginBase* Plugin);
// Parametric curves
boolint _cmsRegisterParametricCurvesPlugin(cmsContext ContextID, cmsPluginBase* Plugin);
// Formatters management
boolint _cmsRegisterFormattersPlugin(cmsContext ContextID, cmsPluginBase* Plugin);
// Tag type management
boolint _cmsRegisterTagTypePlugin(cmsContext ContextID, cmsPluginBase* Plugin);
// Tag management
boolint _cmsRegisterTagPlugin(cmsContext ContextID, cmsPluginBase* Plugin);
// Intent management
boolint _cmsRegisterRenderingIntentPlugin(cmsContext ContextID, cmsPluginBase* Plugin);
// Multi Process elements
boolint _cmsRegisterMultiProcessElementPlugin(cmsContext ContextID, cmsPluginBase* Plugin);
// Optimization
boolint _cmsRegisterOptimizationPlugin(cmsContext ContextID, cmsPluginBase* Plugin);
// Transform
boolint _cmsRegisterTransformPlugin(cmsContext ContextID, cmsPluginBase* Plugin);
// Mutex
boolint _cmsRegisterMutexPlugin(cmsContext ContextID, cmsPluginBase* Plugin);

// ---------------------------------------------------------------------------------------------------------

// Suballocators.
typedef struct _cmsSubAllocator_chunk_st {
	uint8 * Block;
	uint32 BlockSize;
	uint32 Used;
	struct _cmsSubAllocator_chunk_st* next;
} _cmsSubAllocator_chunk;

typedef struct {
	cmsContext ContextID;
	_cmsSubAllocator_chunk* h;
} _cmsSubAllocator;

_cmsSubAllocator* _cmsCreateSubAlloc(cmsContext ContextID, uint32 Initial);
void              _cmsSubAllocDestroy(_cmsSubAllocator* s);
void *  _cmsSubAlloc(_cmsSubAllocator* s, uint32 size);
void *  _cmsSubAllocDup(_cmsSubAllocator* s, const void * ptr, uint32 size);

// ----------------------------------------------------------------------------------

// The context clients.
typedef enum {
	UserPtr,        // User-defined pointer
	Logger,
	AlarmCodesContext,
	AdaptationStateContext,
	MemPlugin,
	InterpPlugin,
	CurvesPlugin,
	FormattersPlugin,
	TagTypePlugin,
	TagPlugin,
	IntentPlugin,
	MPEPlugin,
	OptimizationPlugin,
	TransformPlugin,
	MutexPlugin,

	// Last in list
	MemoryClientMax
} _cmsMemoryClient;

// Container for memory management plug-in.
typedef struct {
	_cmsMallocFnPtrType MallocPtr;
	_cmsMalloZerocFnPtrType MallocZeroPtr;
	_cmsFreeFnPtrType FreePtr;
	_cmsReallocFnPtrType ReallocPtr;
	_cmsCallocFnPtrType CallocPtr;
	_cmsDupFnPtrType DupPtr;
} _cmsMemPluginChunkType;

// Copy memory management function pointers from plug-in to chunk, taking care of missing routines
void  _cmsInstallAllocFunctions(cmsPluginMemHandler* Plugin, _cmsMemPluginChunkType* ptr);

// Internal structure for context
struct _cmsContext_struct {
	struct _cmsContext_struct* Next; // Points to next context in the new style
	_cmsSubAllocator* MemPool;    // The memory pool that stores context data

	void * chunks[MemoryClientMax]; // array of pointers to client chunks. Memory itself is hold in the suballocator.
	                               // If NULL, then it reverts to global Context0

	_cmsMemPluginChunkType DefaultMemoryManager; // The allocators used for creating the context itself. Cannot be
	                                             // overridden
};

// Returns a pointer to a valid context structure, including the global one if id is zero.
// Verifies the magic number.
struct _cmsContext_struct * FASTCALL _cmsGetContext(cmsContext ContextID);

// Returns the block assigned to the specific zone.
void * FASTCALL _cmsContextGetClientChunk(cmsContext id, _cmsMemoryClient mc);

// Chunks of context memory by plug-in client -------------------------------------------------------

// Those structures encapsulates all variables needed by the several context clients (mostly plug-ins)

// Container for error logger -- not a plug-in
typedef struct {
	cmsLogErrorHandlerFunction LogErrorHandler; // Set to NULL for Context0 fallback
} _cmsLogErrorChunkType;

// The global Context0 storage for error logger
extern _cmsLogErrorChunkType _cmsLogErrorChunk;

// Allocate and init error logger container.
void _cmsAllocLogErrorChunk(struct _cmsContext_struct* ctx,
    const struct _cmsContext_struct* src);

// Container for alarm codes -- not a plug-in
typedef struct {
	uint16 AlarmCodes[cmsMAXCHANNELS];
} _cmsAlarmCodesChunkType;

// The global Context0 storage for alarm codes
extern _cmsAlarmCodesChunkType _cmsAlarmCodesChunk;

// Allocate and init alarm codes container.
void _cmsAllocAlarmCodesChunk(struct _cmsContext_struct* ctx,
    const struct _cmsContext_struct* src);

// Container for adaptation state -- not a plug-in
typedef struct {
	double AdaptationState;
} _cmsAdaptationStateChunkType;

// The global Context0 storage for adaptation state
extern _cmsAdaptationStateChunkType _cmsAdaptationStateChunk;
// Allocate and init adaptation state container.
void _cmsAllocAdaptationStateChunk(struct _cmsContext_struct* ctx, const struct _cmsContext_struct* src);
// The global Context0 storage for memory management
extern _cmsMemPluginChunkType _cmsMemPluginChunk;
// Allocate and init memory management container.
void _cmsAllocMemPluginChunk(struct _cmsContext_struct* ctx, const struct _cmsContext_struct* src);

// Container for interpolation plug-in
typedef struct {
	cmsInterpFnFactory Interpolators;
} _cmsInterpPluginChunkType;

// The global Context0 storage for interpolation plug-in
extern _cmsInterpPluginChunkType _cmsInterpPluginChunk;
// Allocate and init interpolation container.
void _cmsAllocInterpPluginChunk(struct _cmsContext_struct* ctx, const struct _cmsContext_struct* src);

// Container for parametric curves plug-in
typedef struct {
	struct _cmsParametricCurvesCollection_st* ParametricCurves;
} _cmsCurvesPluginChunkType;

// The global Context0 storage for tone curves plug-in
extern _cmsCurvesPluginChunkType _cmsCurvesPluginChunk;

// Allocate and init parametric curves container.
void _cmsAllocCurvesPluginChunk(struct _cmsContext_struct* ctx, const struct _cmsContext_struct* src);

// Container for formatters plug-in
typedef struct {
	struct _cms_formatters_factory_list* FactoryList;
} _cmsFormattersPluginChunkType;

// The global Context0 storage for formatters plug-in
extern _cmsFormattersPluginChunkType _cmsFormattersPluginChunk;

// Allocate and init formatters container.
void _cmsAllocFormattersPluginChunk(struct _cmsContext_struct* ctx, const struct _cmsContext_struct* src);

// This chunk type is shared by TagType plug-in and MPE Plug-in
typedef struct {
	struct _cmsTagTypeLinkedList_st* TagTypes;
} _cmsTagTypePluginChunkType;

extern _cmsTagTypePluginChunkType _cmsTagTypePluginChunk; // The global Context0 storage for tag types plug-in
extern _cmsTagTypePluginChunkType _cmsMPETypePluginChunk; // The global Context0 storage for mult process elements plug-in
// Allocate and init Tag types container.
void _cmsAllocTagTypePluginChunk(struct _cmsContext_struct* ctx, const struct _cmsContext_struct* src);
// Allocate and init MPE container.
void _cmsAllocMPETypePluginChunk(struct _cmsContext_struct* ctx, const struct _cmsContext_struct* src);
// Container for tag plug-in
typedef struct {
	struct _cmsTagLinkedList_st* Tag;
} _cmsTagPluginChunkType;

extern _cmsTagPluginChunkType _cmsTagPluginChunk; // The global Context0 storage for tag plug-in

// Allocate and init Tag container.
void _cmsAllocTagPluginChunk(struct _cmsContext_struct* ctx, const struct _cmsContext_struct* src);

// Container for intents plug-in
typedef struct {
	struct _cms_intents_list* Intents;
} _cmsIntentsPluginChunkType;

extern _cmsIntentsPluginChunkType _cmsIntentsPluginChunk; // The global Context0 storage for intents plug-in

// Allocate and init intents container.
void _cmsAllocIntentsPluginChunk(struct _cmsContext_struct* ctx, const struct _cmsContext_struct* src);

// Container for optimization plug-in
typedef struct {
	struct _cmsOptimizationCollection_st* OptimizationCollection;
} _cmsOptimizationPluginChunkType;

extern _cmsOptimizationPluginChunkType _cmsOptimizationPluginChunk; // The global Context0 storage for optimizers plug-in

// Allocate and init optimizers container.
void _cmsAllocOptimizationPluginChunk(struct _cmsContext_struct* ctx, const struct _cmsContext_struct* src);

// Container for transform plug-in
typedef struct {
	struct _cmsTransformCollection_st* TransformCollection;
} _cmsTransformPluginChunkType;

extern _cmsTransformPluginChunkType _cmsTransformPluginChunk; // The global Context0 storage for full-transform replacement plug-in

// Allocate and init transform container.
void _cmsAllocTransformPluginChunk(struct _cmsContext_struct* ctx, const struct _cmsContext_struct* src);

// Container for mutex plug-in
typedef struct {
	_cmsCreateMutexFnPtrType CreateMutexPtr;
	_cmsDestroyMutexFnPtrType DestroyMutexPtr;
	_cmsLockMutexFnPtrType LockMutexPtr;
	_cmsUnlockMutexFnPtrType UnlockMutexPtr;
} _cmsMutexPluginChunkType;

extern _cmsMutexPluginChunkType _cmsMutexPluginChunk; // The global Context0 storage for mutex plug-in

// Allocate and init mutex container.
void _cmsAllocMutexPluginChunk(struct _cmsContext_struct* ctx, const struct _cmsContext_struct* src);

// ----------------------------------------------------------------------------------
// MLU internal representation
typedef struct {
	uint16 Language;
	uint16 Country;
	uint32 StrW;   // Offset to current unicode string
	uint32 Len;    // Length in bytes
} _cmsMLUentry;

struct _cms_MLU_struct {
	cmsContext ContextID;

	// The directory
	uint32 AllocatedEntries;
	uint32 UsedEntries;
	_cmsMLUentry* Entries; // Array of pointers to strings allocated in MemPool

	// The Pool
	uint32 PoolSize; // The maximum allocated size
	uint32 PoolUsed; // The used size
	void *  MemPool;        // Pointer to begin of memory pool
};

// Named color list internal representation
typedef struct {
	char Name[cmsMAX_PATH];
	uint16 PCS[3];
	uint16 DeviceColorant[cmsMAXCHANNELS];
} _cmsNAMEDCOLOR;

struct _cms_NAMEDCOLORLIST_struct {
	uint32 nColors;
	uint32 Allocated;
	uint32 ColorantCount;
	char Prefix[33];  // Prefix and suffix are defined to be 32 characters at most
	char Suffix[33];
	_cmsNAMEDCOLOR* List;
	cmsContext ContextID;
};

// ----------------------------------------------------------------------------------

// This is the internal struct holding profile details.

// Maximum supported tags in a profile
#define MAX_TABLE_TAG       100

typedef struct _cms_iccprofile_struct {
	cmsIOHANDLER*            IOhandler; // I/O handler
	cmsContext ContextID; // The thread ID
	struct tm Created; // Creation time
	// Only most important items found in ICC profiles
	uint32 Version;
	cmsProfileClassSignature DeviceClass;
	cmsColorSpaceSignature ColorSpace;
	cmsColorSpaceSignature PCS;
	uint32 RenderingIntent;
	uint32 flags;
	uint32 manufacturer, model;
	uint64 attributes;
	uint32 creator;
	cmsProfileID ProfileID;
	// Dictionary
	uint32 TagCount;
	cmsTagSignature TagNames[MAX_TABLE_TAG];
	cmsTagSignature TagLinked[MAX_TABLE_TAG];                // The tag to which is linked (0=none)
	uint32 TagSizes[MAX_TABLE_TAG];                 // Size on disk
	uint32 TagOffsets[MAX_TABLE_TAG];
	boolint TagSaveAsRaw[MAX_TABLE_TAG];                     // True to write uncooked
	void * TagPtrs[MAX_TABLE_TAG];
	cmsTagTypeHandler* TagTypeHandlers[MAX_TABLE_TAG]; // Same structure may be serialized on different types
		// depending on profile version, so we keep track of the type handler for each tag in the list.
	// Special
	boolint IsWrite;
	// Keep a mutex for cmsReadTag -- Note that this only works if the user includes a mutex plugin
	void * UsrMutex;
} _cmsICCPROFILE;

// IO helpers for profiles
boolint _cmsReadHeader(_cmsICCPROFILE* Icc);
boolint _cmsWriteHeader(_cmsICCPROFILE* Icc, uint32 UsedSpace);
int                  _cmsSearchTag(_cmsICCPROFILE* Icc, cmsTagSignature sig, boolint lFollowLinks);

// Tag types
cmsTagTypeHandler*   _cmsGetTagTypeHandler(cmsContext ContextID, cmsTagTypeSignature sig);
cmsTagTypeSignature  _cmsGetTagTrueType(cmsHPROFILE hProfile, cmsTagSignature sig);
cmsTagDescriptor*    _cmsGetTagDescriptor(cmsContext ContextID, cmsTagSignature sig);

// Error logging
// ---------------------------------------------------------------------------------------------------------

void                 _cmsTagSignature2String(char String[5], cmsTagSignature sig);

// Interpolation
// ---------------------------------------------------------------------------------------------------------

CMSCHECKPOINT cmsInterpParams* CMSEXPORT _cmsComputeInterpParams(cmsContext ContextID, uint32 nSamples, uint32 InputChan, uint32 OutputChan, const void * Table, uint32 dwFlags);
cmsInterpParams * _cmsComputeInterpParamsEx(cmsContext ContextID, const uint32 nSamples[], uint32 InputChan, uint32 OutputChan, const void * Table, uint32 dwFlags);
CMSCHECKPOINT void CMSEXPORT _cmsFreeInterpParams(cmsInterpParams* p);
boolint _cmsSetInterpolationRoutine(cmsContext ContextID, cmsInterpParams* p);

// Curves
// ----------------------------------------------------------------------------------------------------------------

// This struct holds information about a segment, plus a pointer to the function that implements the evaluation.
// In the case of table-based, Eval pointer is set to NULL

// The gamma function main structure
struct _cms_curve_struct {
	cmsInterpParams*  InterpParams;// Private optimizations for interpolation
	uint32 nSegments;   // Number of segments in the curve. Zero for a 16-bit based tables
	cmsCurveSegment*  Segments;  // The segments
	cmsInterpParams** SegInterp; // Array of private optimizations for interpolation in table-based segments
	cmsParametricCurveEvaluator* Evals; // Evaluators (one per segment)
	// 16 bit Table-based representation follows
	uint32 nEntries;     // Number of table elements
	uint16*   Table16;   // The table itself.
};

//  Pipelines & Stages ---------------------------------------------------------------------------------------------

// A single stage
struct _cmsStage_struct {
	cmsContext ContextID;
	cmsStageSignature Type;         // Identifies the stage
	cmsStageSignature Implements;   // Identifies the *function* of the stage (for optimizations)
	uint32 InputChannels;  // Input channels -- for optimization purposes
	uint32 OutputChannels; // Output channels -- for optimization purposes
	_cmsStageEvalFn EvalPtr;        // Points to fn that evaluates the stage (always in floating point)
	_cmsStageDupElemFn DupElemPtr;  // Points to a fn that duplicates the *data* of the stage
	_cmsStageFreeElemFn FreePtr;    // Points to a fn that sets the *data* of the stage free
	// A generic pointer to whatever memory needed by the stage
	void *    Data;
	// Maintains linked list (used internally)
	struct _cmsStage_struct* Next;
};

// Special Stages (cannot be saved)
CMSCHECKPOINT cmsStage * CMSEXPORT _cmsStageAllocLab2XYZ(cmsContext ContextID);
CMSCHECKPOINT cmsStage * CMSEXPORT _cmsStageAllocXYZ2Lab(cmsContext ContextID);
cmsStage * _cmsStageAllocLabPrelin(cmsContext ContextID);
CMSCHECKPOINT cmsStage * CMSEXPORT _cmsStageAllocLabV2ToV4(cmsContext ContextID);
cmsStage * _cmsStageAllocLabV2ToV4curves(cmsContext ContextID);
CMSCHECKPOINT cmsStage * CMSEXPORT _cmsStageAllocLabV4ToV2(cmsContext ContextID);
CMSCHECKPOINT cmsStage * CMSEXPORT _cmsStageAllocNamedColor(cmsNAMEDCOLORLIST* NamedColorList, boolint UsePCS);
CMSCHECKPOINT cmsStage * CMSEXPORT _cmsStageAllocIdentityCurves(cmsContext ContextID, uint32 nChannels);
CMSCHECKPOINT cmsStage * CMSEXPORT _cmsStageAllocIdentityCLut(cmsContext ContextID, uint32 nChan);
cmsStage * _cmsStageNormalizeFromLabFloat(cmsContext ContextID);
cmsStage * _cmsStageNormalizeFromXyzFloat(cmsContext ContextID);
cmsStage * _cmsStageNormalizeToLabFloat(cmsContext ContextID);
cmsStage * _cmsStageNormalizeToXyzFloat(cmsContext ContextID);
cmsStage * _cmsStageClipNegatives(cmsContext ContextID, uint32 nChannels);

// For curve set only
cmsToneCurve **     _cmsStageGetPtrToCurveSet(const cmsStage * mpe);

// Pipeline Evaluator (in floating point)
typedef void (* _cmsPipelineEvalFloatFn)(const float In[], float Out[], const void * Data);

struct _cmsPipeline_struct {
	cmsStage * Elements;                            // Points to elements chain
	uint32 InputChannels, OutputChannels;
	// Data & evaluators
	void * Data;
	_cmsOPTeval16Fn Eval16Fn;
	_cmsPipelineEvalFloatFn EvalFloatFn;
	_cmsFreeUserDataFn FreeDataFn;
	_cmsDupUserDataFn DupDataFn;
	cmsContext ContextID;        // Environment
	boolint SaveAs8Bits;         // Implementation-specific: save as 8 bits if possible
};

// LUT reading & creation -------------------------------------------------------------------------------------------

// Read tags using low-level function, provide necessary glue code to adapt versions, etc. All those return a brand new
// copy
// of the LUTS, since ownership of original is up to the profile. The user should free allocated resources.

CMSCHECKPOINT cmsPipeline * CMSEXPORT _cmsReadInputLUT(cmsHPROFILE hProfile, uint32 Intent);
CMSCHECKPOINT cmsPipeline * CMSEXPORT _cmsReadOutputLUT(cmsHPROFILE hProfile, uint32 Intent);
CMSCHECKPOINT cmsPipeline * CMSEXPORT _cmsReadDevicelinkLUT(cmsHPROFILE hProfile, uint32 Intent);

// Special values
boolint _cmsReadMediaWhitePoint(cmsCIEXYZ* Dest, cmsHPROFILE hProfile);
boolint _cmsReadCHAD(cmsMAT3* Dest, cmsHPROFILE hProfile);

// Profile linker --------------------------------------------------------------------------------------------------

// Link several profiles to obtain a single LUT modelling the whole color transform. Intents, Black point
// compensation and Adaptation parameters may vary across profiles. BPC and Adaptation refers to the PCS
// after the profile. I.e, BPC[0] refers to connexion between profile(0) and profile(1)
cmsPipeline * _cmsLinkProfiles(cmsContext ContextID,
    uint32 nProfiles,
    uint32 TheIntents[],
    cmsHPROFILE hProfiles[],
    boolint BPC[],
    double AdaptationStates[],
    uint32 dwFlags);

// Sequence --------------------------------------------------------------------------------------------------------

cmsSEQ* _cmsReadProfileSequence(cmsHPROFILE hProfile);
boolint _cmsWriteProfileSequence(cmsHPROFILE hProfile, const cmsSEQ* seq);
cmsSEQ* _cmsCompileProfileSequence(cmsContext ContextID, uint32 nProfiles, cmsHPROFILE hProfiles[]);

// LUT optimization ------------------------------------------------------------------------------------------------

CMSCHECKPOINT uint16 CMSEXPORT _cmsQuantizeVal(double i, uint32 MaxSamples);

uint32  _cmsReasonableGridpointsByColorspace(cmsColorSpaceSignature Colorspace, uint32 dwFlags);

boolint _cmsEndPointsBySpace(cmsColorSpaceSignature Space, uint16 ** White, uint16 ** Black, uint32 * nOutputs);
boolint _cmsOptimizePipeline(cmsContext ContextID, cmsPipeline **    Lut, uint32 Intent, uint32* InputFormat, uint32* OutputFormat, uint32* dwFlags);

// Hi level LUT building ----------------------------------------------------------------------------------------------

cmsPipeline * _cmsCreateGamutCheckPipeline(cmsContext ContextID, cmsHPROFILE hProfiles[], boolint BPC[], uint32 Intents[], double AdaptationStates[], uint32 nGamutPCSposition, cmsHPROFILE hGamut);

// Formatters
// ------------------------------------------------------------------------------------------------------------

#define cmsFLAGS_CAN_CHANGE_FORMATTER     0x02000000   // Allow change buffer format

boolint _cmsFormatterIsFloat(uint32 Type);
boolint _cmsFormatterIs8bit(uint32 Type);
CMSCHECKPOINT cmsFormatter CMSEXPORT _cmsGetFormatter(cmsContext ContextID, uint32 Type/*Specific type, i.e. TYPE_RGB_8*/, cmsFormatterDirection Dir, uint32 dwFlags);

#ifndef CMS_NO_HALF_SUPPORT

// Half float
CMSCHECKPOINT float CMSEXPORT _cmsHalf2Float(uint16 h);
CMSCHECKPOINT uint16 CMSEXPORT _cmsFloat2Half(float flt);

#endif

// Transform logic
// ------------------------------------------------------------------------------------------------------

struct _cmstransform_struct;

typedef struct {
	// 1-pixel cache (16 bits only)
	uint16 CacheIn[cmsMAXCHANNELS];
	uint16 CacheOut[cmsMAXCHANNELS];
} _cmsCACHE;

// Transformation
typedef struct _cmstransform_struct {
	uint32 InputFormat, OutputFormat; // Keep formats for further reference
	_cmsTransform2Fn xform; // Points to transform code
	// Formatters, cannot be embedded into LUT because cache
	cmsFormatter16 FromInput;
	cmsFormatter16 ToOutput;
	cmsFormatterFloat FromInputFloat;
	cmsFormatterFloat ToOutputFloat;
	_cmsCACHE Cache; // 1-pixel cache seed for zero as input (16 bits, read only)
	cmsPipeline * Lut; // A Pipeline holding the full (optimized) transform
	cmsPipeline * GamutCheck; // A Pipeline holding the gamut check. It goes from the input space to bilevel
	// Colorant tables
	cmsNAMEDCOLORLIST* InputColorant;   // Input Colorant table
	cmsNAMEDCOLORLIST* OutputColorant;  // Colorant table (for n chans > CMYK)
	// Informational only
	cmsColorSpaceSignature EntryColorSpace;
	cmsColorSpaceSignature ExitColorSpace;
	// White points (informative only)
	cmsCIEXYZ EntryWhitePoint;
	cmsCIEXYZ ExitWhitePoint;
	cmsSEQ* Sequence; // Profiles used to create the transform
	uint32 dwOriginalFlags;
	double AdaptationState;
	uint32 RenderingIntent; // The intent of this transform. That is usually the last intent in the profilechain, but may differ
	cmsContext ContextID; // An id that uniquely identifies the running context. May be null.
	// A user-defined pointer that can be used to store data for transform plug-ins
	void * UserData;
	_cmsFreeUserDataFn FreeUserData;
	_cmsTransformFn OldXform; // A way to provide backwards compatibility with full xform plugins
} _cmsTRANSFORM;

// Copies extra channels from input to output if the original flags in the transform structure
// instructs to do so. This function is called on all standard transform functions.
void _cmsHandleExtraChannels(_cmsTRANSFORM* p, const void * in, void * out, uint32 PixelsPerLine, uint32 LineCount, const cmsStride* Stride);

// -----------------------------------------------------------------------------------------------------------------------

cmsHTRANSFORM _cmsChain2Lab(cmsContext ContextID, uint32 nProfiles, uint32 InputFormat, uint32 OutputFormat, const uint32 Intents[],
    const cmsHPROFILE hProfiles[], const boolint BPC[], const double AdaptationStates[], uint32 dwFlags);
cmsToneCurve * _cmsBuildKToneCurve(cmsContext ContextID, uint32 nPoints, uint32 nProfiles, const uint32 Intents[], const cmsHPROFILE hProfiles[], 
	const boolint BPC[], const double AdaptationStates[], uint32 dwFlags);
boolint _cmsAdaptationMatrix(cmsMAT3* r, const cmsMAT3* ConeMatrix, const cmsCIEXYZ* FromIll, const cmsCIEXYZ* ToIll);
boolint _cmsBuildRGB2XYZtransferMatrix(cmsMAT3* r, const cmsCIExyY* WhitePoint, const cmsCIExyYTRIPLE* Primaries);

#define _lcms_internal_H
#endif
