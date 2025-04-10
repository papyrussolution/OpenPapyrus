/*
 * Summary: interface for the memory allocator
 * Description: provides interfaces for the memory allocator,
 *   including debugging capabilities.
 *
 * Copy: See Copyright for the status of this software.
 *
 * Author: Daniel Veillard
 */
#ifndef __DEBUG_MEMORY_ALLOC__
#define __DEBUG_MEMORY_ALLOC__
/**
 * DEBUG_MEMORY:
 *
 * DEBUG_MEMORY replaces the allocator with a collect and debug
 * shell to the libc allocator.
 * DEBUG_MEMORY should only be activated when debugging
 * libxml i.e. if libxml has been configured with --with-debug-mem too.
 */
/* #define DEBUG_MEMORY_FREED */
/* #define DEBUG_MEMORY_LOCATION */
#ifndef NDEBUG
	#ifndef DEBUG_MEMORY
		#define DEBUG_MEMORY
	#endif
#endif
/**
 * DEBUG_MEMORY_LOCATION:
 *
 * DEBUG_MEMORY_LOCATION should be activated only when debugging
 * libxml i.e. if libxml has been configured with --with-debug-mem too.
 */
#ifdef DEBUG_MEMORY_LOCATION
#endif
//#ifdef __cplusplus
	//extern "C" {
//#endif
/*
 * The XML memory wrapper support 4 basic overloadable functions.
 */
/**
 * xmlFreeFunc:
 * @mem: an already allocated block of memory
 *
 * Signature for a SAlloc::F() implementation.
 */
typedef void (* xmlFreeFunc)(void *mem);
/**
 * xmlMallocFunc:
 * @size:  the size requested in bytes
 *
 * Signature for a malloc() implementation.
 *
 * Returns a pointer to the newly allocated block or NULL in case of error.
 */
typedef void *(LIBXML_ATTR_ALLOC_SIZE(1) *xmlMallocFunc)(size_t size);

/**
 * xmlReallocFunc:
 * @mem: an already allocated block of memory
 * @size:  the new size requested in bytes
 *
 * Signature for a realloc() implementation.
 *
 * Returns a pointer to the newly reallocated block or NULL in case of error.
 */
typedef void *(* xmlReallocFunc)(void *mem, size_t size);

/**
 * xmlStrdupFunc:
 * @str: a zero terminated string
 *
 * Signature for an strdup() implementation.
 *
 * Returns the copy of the string or NULL in case of error.
 */
typedef char *(* xmlStrdupFunc)(const char *str);
/*
 * The 4 interfaces used for all memory handling within libxml.
// @sobolev LIBXML_DLL_IMPORT xmlFreeFunc xmlFree_;
LIBXML_DLL_IMPORT xmlMallocFunc xmlMalloc_;
LIBXML_DLL_IMPORT xmlMallocFunc xmlMallocAtomic_;
LIBXML_DLL_IMPORT xmlReallocFunc xmlRealloc_;
LIBXML_DLL_IMPORT xmlStrdupFunc xmlMemStrdup_Removed;
 */
/*
 * The way to overload the existing functions.
 * The xmlGc function have an extra entry for atomic block
 * allocations useful for garbage collected memory allocators
 */
//int xmlMemSetup(xmlFreeFunc freeFunc, xmlMallocFunc mallocFunc, xmlReallocFunc reallocFunc, xmlStrdupFunc strdupFunc);
//int xmlMemGet(xmlFreeFunc *freeFunc, xmlMallocFunc *mallocFunc, xmlReallocFunc *reallocFunc, xmlStrdupFunc *strdupFunc);
//int xmlGcMemSetup(xmlFreeFunc freeFunc, xmlMallocFunc mallocFunc, xmlMallocFunc mallocAtomicFunc, xmlReallocFunc reallocFunc, xmlStrdupFunc strdupFunc);
//int xmlGcMemGet(xmlFreeFunc *freeFunc, xmlMallocFunc *mallocFunc, xmlMallocFunc *mallocAtomicFunc, xmlReallocFunc *reallocFunc, xmlStrdupFunc *strdupFunc);
/*
 * Initialization of the memory layer.
 */
int xmlInitMemory();
/*
 * Cleanup of the memory layer.
 */
void xmlCleanupMemory();
/*
 * These are specific to the XML debug memory wrapper.
 */
int    xmlMemUsed();
int    xmlMemBlocks();
void   xmlMemDisplay(FILE *fp);
void   xmlMemDisplayLast(FILE *fp, long nbBytes);
void   xmlMemShow(FILE *fp, int nr);
void   xmlMemoryDump();
void * xmlMemMalloc(size_t size) LIBXML_ATTR_ALLOC_SIZE(1);
void * xmlMemRealloc(void *ptr,size_t size);
void   xmlMemFree(void *ptr);
char * xmlMemoryStrdup(const char *str);
void * xmlMallocLoc(size_t size, const char *file, int line) LIBXML_ATTR_ALLOC_SIZE(1);
void * xmlReallocLoc(void *ptr, size_t size, const char *file, int line);
void * xmlMallocAtomicLoc(size_t size, const char *file, int line) LIBXML_ATTR_ALLOC_SIZE(1);
char * xmlMemStrdupLoc(const char *str, const char *file, int line);

#ifdef DEBUG_MEMORY_LOCATION
/**
 * @size:  number of bytes to allocate
 *
 * Wrapper for the malloc() function used in the XML library.
 *
 * Returns the pointer to the allocated area or NULL in case of error.
 */
#define xmlMalloc_(size) xmlMallocLoc((size), __FILE__, __LINE__)
/**
 * xmlMallocAtomic:
 * @size:  number of bytes to allocate
 *
 * Wrapper for the malloc() function used in the XML library for allocation
 * of block not containing pointers to other areas.
 *
 * Returns the pointer to the allocated area or NULL in case of error.
 */
#define SAlloc::M(size) xmlMallocAtomicLoc((size), __FILE__, __LINE__)
/**
 * xmlRealloc:
 * @ptr:  pointer to the existing allocated area
 * @size:  number of bytes to allocate
 *
 * Wrapper for the realloc() function used in the XML library.
 *
 * Returns the pointer to the allocated area or NULL in case of error.
 */
#define SAlloc::R(ptr, size) xmlReallocLoc((ptr), (size), __FILE__, __LINE__)
/**
 * xmlMemStrdup:
 * @str:  pointer to the existing string
 *
 * Wrapper for the strdup() function, xmlStrdup() is usually preferred.
 *
 * Returns the pointer to the allocated area or NULL in case of error.
 */
//#define xmlMemStrdup_Removed(str) xmlMemStrdupLoc((str), __FILE__, __LINE__)
#endif /* DEBUG_MEMORY_LOCATION */
//#ifdef __cplusplus
//}
//#endif /* __cplusplus */
#endif  /* __DEBUG_MEMORY_ALLOC__ */

