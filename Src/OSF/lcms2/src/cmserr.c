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

// This function is here to help applications to prevent mixing lcms versions on header and shared objects.
int CMSEXPORT cmsGetEncodedCMMversion()
{
	return LCMS_VERSION;
}

// I am so tired about incompatibilities on those functions that here are some replacements
// that hopefully would be fully portable.

// compare two strings ignoring case
int CMSEXPORT cmsstrcasecmp(const char * s1, const char * s2)
{
	const uchar * us1 = (const uchar *)s1;
	const uchar * us2 = (const uchar *)s2;
	while(toupper(*us1) == toupper(*us2++))
		if(*us1++ == '\0')
			return 0;
	return (toupper(*us1) - toupper(*--us2));
}

// long int because C99 specifies ftell in such way (7.19.9.2)
long CMSEXPORT cmsfilelength(FILE* f)
{
	long n;
	long p = ftell(f); // register current file position
	if(p == -1L)
		return -1L;
	if(fseek(f, 0, SEEK_END) != 0) {
		return -1L;
	}
	n = ftell(f);
	fseek(f, p, SEEK_SET); // file position restored
	return n;
}

// Memory handling ------------------------------------------------------------------
//
// This is the interface to low-level memory management routines. By default a simple
// wrapping to malloc/free/realloc is provided, although there is a limit on the max
// amount of memoy that can be reclaimed. This is mostly as a safety feature to prevent
// bogus or evil code to allocate huge blocks that otherwise lcms would never need.

#define MAX_MEMORY_FOR_ALLOC  ((uint32)(1024U*1024U*512U))

// User may override this behaviour by using a memory plug-in, which basically replaces
// the default memory management functions. In this case, no check is performed and it
// is up to the plug-in writter to keep in the safe side. There are only three functions
// required to be implemented: malloc, realloc and free, although the user may want to
// replace the optional mallocZero, calloc and dup as well.

boolint _cmsRegisterMemHandlerPlugin(cmsContext ContextID, cmsPluginBase* Plugin);
//
// This is the default memory allocation function. It does a very coarse
// check of amount of memory, just to prevent exploits
//
static void * _cmsMallocDefaultFn(cmsContext ContextID, uint32 size)
{
	if(size > MAX_MEMORY_FOR_ALLOC) 
		return NULL; // Never allow over maximum
	return (void *)SAlloc::M(size);
	CXX_UNUSED(ContextID);
}

// Generic allocate & zero
static void * _cmsMallocZeroDefaultFn(cmsContext ContextID, uint32 size)
{
	void * pt = _cmsMalloc(ContextID, size);
	if(pt == NULL) 
		return NULL;
	memzero(pt, size);
	return pt;
}

// The default free function. The only check proformed is against NULL pointers
static void _cmsFreeDefaultFn(cmsContext ContextID, void * Ptr)
{
	// free(NULL) is defined a no-op by C99, therefore it is safe to
	// avoid the check, but it is here just in case...
	if(Ptr) 
		SAlloc::F(Ptr);
	CXX_UNUSED(ContextID);
}

// The default realloc function. Again it checks for exploits. If Ptr is NULL,
// realloc behaves the same way as malloc and allocates a new block of size bytes.
static void * _cmsReallocDefaultFn(cmsContext ContextID, void * Ptr, uint32 size)
{
	if(size > MAX_MEMORY_FOR_ALLOC) 
		return NULL; // Never realloc over 512Mb
	return SAlloc::R(Ptr, size);
	CXX_UNUSED(ContextID);
}

// The default calloc function. Allocates an array of num elements, each one of size bytes
// all memory is initialized to zero.
static void * _cmsCallocDefaultFn(cmsContext ContextID, uint32 num, uint32 size)
{
	uint32 Total = num * size;
	// Preserve calloc behaviour
	if(Total == 0) return NULL;
	// Safe check for overflow.
	if(num >= UINT_MAX / size) return NULL;
	// Check for overflow
	if(Total < num || Total < size) {
		return NULL;
	}
	if(Total > MAX_MEMORY_FOR_ALLOC) return NULL; // Never alloc over 512Mb
	return _cmsMallocZero(ContextID, Total);
}

// Generic block duplication
static void * _cmsDupDefaultFn(cmsContext ContextID, const void * Org, uint32 size)
{
	void * mem;
	if(size > MAX_MEMORY_FOR_ALLOC) return NULL; // Never dup over 512Mb
	mem = _cmsMalloc(ContextID, size);
	if(mem && Org)
		memmove(mem, Org, size);
	return mem;
}

// Pointers to memory manager functions in Context0
_cmsMemPluginChunkType _cmsMemPluginChunk = { _cmsMallocDefaultFn, _cmsMallocZeroDefaultFn, _cmsFreeDefaultFn, _cmsReallocDefaultFn, _cmsCallocDefaultFn,    _cmsDupDefaultFn};

// Reset and duplicate memory manager
void _cmsAllocMemPluginChunk(struct _cmsContext_struct* ctx, const struct _cmsContext_struct* src)
{
	assert(ctx);
	if(src) {
		// Duplicate
		ctx->chunks[MemPlugin] = _cmsSubAllocDup(ctx->MemPool, src->chunks[MemPlugin], sizeof(_cmsMemPluginChunkType));
	}
	else {
		// To reset it, we use the default allocators, which cannot be overridden
		ctx->chunks[MemPlugin] = &ctx->DefaultMemoryManager;
	}
}

// Auxiliary to fill memory management functions from plugin (or context 0 defaults)
void _cmsInstallAllocFunctions(cmsPluginMemHandler* Plugin, _cmsMemPluginChunkType* ptr)
{
	if(Plugin == NULL) {
		memcpy(ptr, &_cmsMemPluginChunk, sizeof(_cmsMemPluginChunk));
	}
	else {
		ptr->MallocPtr  = Plugin->MallocPtr;
		ptr->FreePtr    = Plugin->FreePtr;
		ptr->ReallocPtr = Plugin->ReallocPtr;
		// Make sure we revert to defaults
		ptr->MallocZeroPtr = _cmsMallocZeroDefaultFn;
		ptr->CallocPtr    = _cmsCallocDefaultFn;
		ptr->DupPtr       = _cmsDupDefaultFn;
		if(Plugin->MallocZeroPtr) ptr->MallocZeroPtr = Plugin->MallocZeroPtr;
		if(Plugin->CallocPtr) ptr->CallocPtr     = Plugin->CallocPtr;
		if(Plugin->DupPtr) ptr->DupPtr        = Plugin->DupPtr;
	}
}

// Plug-in replacement entry
boolint _cmsRegisterMemHandlerPlugin(cmsContext ContextID, cmsPluginBase * Data)
{
	cmsPluginMemHandler* Plugin = (cmsPluginMemHandler*)Data;
	_cmsMemPluginChunkType* ptr;
	// NULL forces to reset to defaults. In this special case, the defaults are stored in the context structure.
	// Remaining plug-ins does NOT have any copy in the context structure, but this is somehow special as the
	// context internal data should be malloce'd by using those functions.
	if(!Data) {
		struct _cmsContext_struct* ctx = (struct _cmsContext_struct*)ContextID;
		// Return to the default allocators
		if(ContextID) {
			ctx->chunks[MemPlugin] = (void *)&ctx->DefaultMemoryManager;
		}
		return TRUE;
	}
	// Check for required callbacks
	if(Plugin->MallocPtr == NULL || Plugin->FreePtr == NULL || Plugin->ReallocPtr == NULL) 
		return FALSE;
	// Set replacement functions
	ptr = (_cmsMemPluginChunkType*)_cmsContextGetClientChunk(ContextID, MemPlugin);
	if(!ptr)
		return FALSE;
	_cmsInstallAllocFunctions(Plugin, ptr);
	return TRUE;
}

// Generic allocate
void * CMSEXPORT _cmsMalloc(cmsContext ContextID, uint32 size)
{
	_cmsMemPluginChunkType* ptr = (_cmsMemPluginChunkType*)_cmsContextGetClientChunk(ContextID, MemPlugin);
	return ptr->MallocPtr(ContextID, size);
}

// Generic allocate & zero
void * CMSEXPORT _cmsMallocZero(cmsContext ContextID, uint32 size)
{
	_cmsMemPluginChunkType* ptr = (_cmsMemPluginChunkType*)_cmsContextGetClientChunk(ContextID, MemPlugin);
	return ptr->MallocZeroPtr(ContextID, size);
}

// Generic calloc
void * CMSEXPORT _cmsCalloc(cmsContext ContextID, uint32 num, uint32 size)
{
	_cmsMemPluginChunkType* ptr = (_cmsMemPluginChunkType*)_cmsContextGetClientChunk(ContextID, MemPlugin);
	return ptr->CallocPtr(ContextID, num, size);
}

// Generic reallocate
void * CMSEXPORT _cmsRealloc(cmsContext ContextID, void * Ptr, uint32 size)
{
	_cmsMemPluginChunkType* ptr = (_cmsMemPluginChunkType*)_cmsContextGetClientChunk(ContextID, MemPlugin);
	return ptr->ReallocPtr(ContextID, Ptr, size);
}

// Generic free memory
void CMSEXPORT _cmsFree(cmsContext ContextID, void * Ptr)
{
	if(Ptr) {
		_cmsMemPluginChunkType* ptr = (_cmsMemPluginChunkType*)_cmsContextGetClientChunk(ContextID, MemPlugin);
		ptr->FreePtr(ContextID, Ptr);
	}
}

// Generic block duplication
void * CMSEXPORT _cmsDupMem(cmsContext ContextID, const void * Org, uint32 size)
{
	_cmsMemPluginChunkType* ptr = (_cmsMemPluginChunkType*)_cmsContextGetClientChunk(ContextID, MemPlugin);
	return ptr->DupPtr(ContextID, Org, size);
}
//
// Sub allocation takes care of many pointers of small size. The memory allocated in
// this way have be freed at once. Next function allocates a single chunk for linked list
// I prefer this method over realloc due to the big inpact on xput realloc may have if
// memory is being swapped to disk. This approach is safer (although that may not be true on all platforms)
static _cmsSubAllocator_chunk* _cmsCreateSubAllocChunk(cmsContext ContextID, uint32 Initial)
{
	_cmsSubAllocator_chunk* chunk;
	// 20K by default
	SETIFZQ(Initial, 20*1024);
	// Create the container
	chunk = (_cmsSubAllocator_chunk*)_cmsMallocZero(ContextID, sizeof(_cmsSubAllocator_chunk));
	if(chunk) {
		// Initialize values
		chunk->Block = (uint8 *)_cmsMalloc(ContextID, Initial);
		if(chunk->Block == NULL) {
			// Something went wrong
			_cmsFree(ContextID, chunk);
			return NULL;
		}
		chunk->BlockSize = Initial;
		chunk->Used      = 0;
		chunk->next      = NULL;
	}
	return chunk;
}

// The suballocated is nothing but a pointer to the first element in the list. We also keep
// the thread ID in this structure.
_cmsSubAllocator* _cmsCreateSubAlloc(cmsContext ContextID, uint32 Initial)
{
	// Create the container
	_cmsSubAllocator* sub = (_cmsSubAllocator*)_cmsMallocZero(ContextID, sizeof(_cmsSubAllocator));
	if(sub) {
		sub->ContextID = ContextID;
		sub->h = _cmsCreateSubAllocChunk(ContextID, Initial);
		if(sub->h == NULL) {
			_cmsFree(ContextID, sub);
			return NULL;
		}
	}
	return sub;
}

// Get rid of whole linked list
void _cmsSubAllocDestroy(_cmsSubAllocator* sub)
{
	if(sub) {
		_cmsSubAllocator_chunk * chunk, * n;
		for(chunk = sub->h; chunk; chunk = n) {
			n = chunk->next;
			_cmsFree(sub->ContextID, chunk->Block);
			_cmsFree(sub->ContextID, chunk);
		}
		_cmsFree(sub->ContextID, sub); // Free the header
	}
}

// Get a pointer to small memory block.
void *  _cmsSubAlloc(_cmsSubAllocator* sub, uint32 size)
{
	uint32 Free = sub->h->BlockSize - sub->h->Used;
	uint8 * ptr;
	size = _cmsALIGNMEM(size);
	// Check for memory. If there is no room, allocate a new chunk of double memory size.
	if(size > Free) {
		_cmsSubAllocator_chunk* chunk;
		uint32 newSize = sub->h->BlockSize * 2;
		if(newSize < size) newSize = size;
		chunk = _cmsCreateSubAllocChunk(sub->ContextID, newSize);
		if(chunk == NULL) return NULL;
		// Link list
		chunk->next = sub->h;
		sub->h    = chunk;
	}
	ptr =  sub->h->Block + sub->h->Used;
	sub->h->Used += size;
	return (void *)ptr;
}

// Duplicate in pool
void * _cmsSubAllocDup(_cmsSubAllocator* s, const void * ptr, uint32 size)
{
	void * NewPtr;
	// Dup of null pointer is also NULL
	if(!ptr)
		return NULL;
	NewPtr = _cmsSubAlloc(s, size);
	if(ptr && NewPtr) {
		memcpy(NewPtr, ptr, size);
	}
	return NewPtr;
}

// Error logging ******************************************************************

// There is no error handling at all. When a function fails, it returns proper value.
// For example, all create functions does return NULL on failure. Other return FALSE
// It may be interesting, for the developer, to know why the function is failing.
// for that reason, lcms2 does offer a logging function. This function does receive
// a ENGLISH string with some clues on what is going wrong. You can show this
// info to the end user, or just create some sort of log.
// The logging function should NOT terminate the program, as this obviously can leave
// resources. It is the programmer's responsibility to check each function return code
// to make sure it didn't fail.

// Error messages are limited to MAX_ERROR_MESSAGE_LEN

#define MAX_ERROR_MESSAGE_LEN   1024

// The default error logger does nothing.
static void DefaultLogErrorHandlerFunction(FILE * fOut, cmsContext ContextID, uint32 ErrorCode, const char * Text)
{
	// slfprintf_stderr("[lcms]: %s\n", Text);
	// fflush(stderr);
	CXX_UNUSED(fOut); // @v11.7.6
	CXX_UNUSED(ContextID);
	CXX_UNUSED(ErrorCode);
	CXX_UNUSED(Text);
}

// Context0 storage, which is global
_cmsLogErrorChunkType _cmsLogErrorChunk = { DefaultLogErrorHandlerFunction };

// Allocates and inits error logger container for a given context. If src is NULL, only initializes the value
// to the default. Otherwise, it duplicates the value. The interface is standard across all context clients
void _cmsAllocLogErrorChunk(struct _cmsContext_struct* ctx, const struct _cmsContext_struct* src)
{
	static _cmsLogErrorChunkType LogErrorChunk = { DefaultLogErrorHandlerFunction };
	void * from = src ? src->chunks[Logger] : &LogErrorChunk;
	ctx->chunks[Logger] = _cmsSubAllocDup(ctx->MemPool, from, sizeof(_cmsLogErrorChunkType));
}

// Change log error, context based
void CMSEXPORT cmsSetLogErrorHandlerTHR(cmsContext ContextID, cmsLogErrorHandlerFunction Fn)
{
	_cmsLogErrorChunkType * lhg = (_cmsLogErrorChunkType*)_cmsContextGetClientChunk(ContextID, Logger);
	if(lhg)
		lhg->LogErrorHandler = Fn ? Fn : DefaultLogErrorHandlerFunction;
}

// Change log error, legacy
void CMSEXPORT cmsSetLogErrorHandler(cmsLogErrorHandlerFunction Fn)
{
	cmsSetLogErrorHandlerTHR(NULL, Fn);
}

// Log an error
// ErrorText is a text holding an english description of error.
void CMSEXPORT cmsSignalError(cmsContext ContextID, uint32 ErrorCode, const char * ErrorText, ...)
{
	va_list args;
	char Buffer[MAX_ERROR_MESSAGE_LEN];
	_cmsLogErrorChunkType* lhg;
	va_start(args, ErrorText);
	vsnprintf(Buffer, MAX_ERROR_MESSAGE_LEN-1, ErrorText, args);
	va_end(args);
	// Check for the context, if specified go there. If not, go for the global
	lhg = (_cmsLogErrorChunkType*)_cmsContextGetClientChunk(ContextID, Logger);
	if(lhg->LogErrorHandler) {
		lhg->LogErrorHandler(stderr, ContextID, ErrorCode, Buffer);
	}
}

// Utility function to print signatures
void _cmsTagSignature2String(char String[5], cmsTagSignature sig)
{
	// Convert to big endian
	uint32 be = _cmsAdjustEndianess32((uint32)sig);
	// Move chars
	memmove(String, &be, 4);
	// Make sure of terminator
	String[4] = 0;
}

static void * defMtxCreate(cmsContext id)
{
	_cmsMutex* ptr_mutex = (_cmsMutex*)_cmsMalloc(id, sizeof(_cmsMutex));
	_cmsInitMutexPrimitive(ptr_mutex);
	return (void *)ptr_mutex;
}

static void defMtxDestroy(cmsContext id, void * mtx)
{
	_cmsDestroyMutexPrimitive((_cmsMutex*)mtx);
	_cmsFree(id, mtx);
}

static boolint defMtxLock(cmsContext id, void * mtx)
{
	CXX_UNUSED(id);
	return _cmsLockPrimitive((_cmsMutex*)mtx) == 0;
}

static void defMtxUnlock(cmsContext id, void * mtx)
{
	CXX_UNUSED(id);
	_cmsUnlockPrimitive((_cmsMutex*)mtx);
}

// Pointers to memory manager functions in Context0
_cmsMutexPluginChunkType _cmsMutexPluginChunk = { defMtxCreate, defMtxDestroy, defMtxLock, defMtxUnlock };

// Allocate and init mutex container.
void _cmsAllocMutexPluginChunk(struct _cmsContext_struct* ctx, const struct _cmsContext_struct* src)
{
	static _cmsMutexPluginChunkType MutexChunk = {defMtxCreate, defMtxDestroy, defMtxLock, defMtxUnlock };
	void * from = src ? src->chunks[MutexPlugin] : &MutexChunk;
	ctx->chunks[MutexPlugin] = _cmsSubAllocDup(ctx->MemPool, from, sizeof(_cmsMutexPluginChunkType));
}

// Register new ways to transform
boolint _cmsRegisterMutexPlugin(cmsContext ContextID, cmsPluginBase* Data)
{
	cmsPluginMutex* Plugin = (cmsPluginMutex*)Data;
	_cmsMutexPluginChunkType* ctx = (_cmsMutexPluginChunkType*)_cmsContextGetClientChunk(ContextID, MutexPlugin);
	if(!Data) {
		// No lock routines
		ctx->CreateMutexPtr = NULL;
		ctx->DestroyMutexPtr = NULL;
		ctx->LockMutexPtr = NULL;
		ctx->UnlockMutexPtr = NULL;
		return TRUE;
	}
	// Factory callback is required
	if(!Plugin->CreateMutexPtr || !Plugin->DestroyMutexPtr || !Plugin->LockMutexPtr || !Plugin->UnlockMutexPtr) 
		return FALSE;
	ctx->CreateMutexPtr  = Plugin->CreateMutexPtr;
	ctx->DestroyMutexPtr = Plugin->DestroyMutexPtr;
	ctx->LockMutexPtr   = Plugin->LockMutexPtr;
	ctx->UnlockMutexPtr = Plugin->UnlockMutexPtr;
	// All is ok
	return TRUE;
}

// Generic Mutex fns
void * CMSEXPORT _cmsCreateMutex(cmsContext ContextID)
{
	_cmsMutexPluginChunkType* ptr = (_cmsMutexPluginChunkType*)_cmsContextGetClientChunk(ContextID, MutexPlugin);
	return ptr->CreateMutexPtr ? ptr->CreateMutexPtr(ContextID) : NULL;
}

void CMSEXPORT _cmsDestroyMutex(cmsContext ContextID, void * mtx)
{
	_cmsMutexPluginChunkType* ptr = (_cmsMutexPluginChunkType*)_cmsContextGetClientChunk(ContextID, MutexPlugin);
	if(ptr->DestroyMutexPtr)
		ptr->DestroyMutexPtr(ContextID, mtx);
}

boolint CMSEXPORT _cmsLockMutex(cmsContext ContextID, void * mtx)
{
	_cmsMutexPluginChunkType* ptr = (_cmsMutexPluginChunkType*)_cmsContextGetClientChunk(ContextID, MutexPlugin);
	return ptr->LockMutexPtr ? ptr->LockMutexPtr(ContextID, mtx) : TRUE;
}

void CMSEXPORT _cmsUnlockMutex(cmsContext ContextID, void * mtx)
{
	_cmsMutexPluginChunkType* ptr = (_cmsMutexPluginChunkType*)_cmsContextGetClientChunk(ContextID, MutexPlugin);
	if(ptr->UnlockMutexPtr)
		ptr->UnlockMutexPtr(ContextID, mtx);
}
