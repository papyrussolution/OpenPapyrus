   /*******************************************************/
   /*      "C" Language Integrated Production System      */
   /*                                                     */
   /*             CLIPS Version 6.24  06/05/06            */
   /*                                                     */
   /*            MEMORY ALLOCATION HEADER FILE            */
   /*******************************************************/

/*************************************************************/
/* Purpose: Memory allocation routines.                      */
/*                                                           */
/* Principal Programmer(s):                                  */
/*      Gary D. Riley                                        */
/*                                                           */
/* Contributing Programmer(s):                               */
/*                                                           */
/* Revision History:                                         */
/*                                                           */
/*      6.24: Renamed BOOLEAN macro type to intBool.         */
/*                                                           */
/*************************************************************/

#ifndef _H_memalloc

#include <string.h>

#define _H_memalloc

struct chunkInfo;
struct blockInfo;
struct memoryPtr;
struct longMemoryPtr;

#define MEM_TABLE_SIZE 500

#ifdef LOCALE
#undef LOCALE
#endif

#ifdef _MEMORY_SOURCE_
#define LOCALE
#else
#define LOCALE extern
#endif

struct chunkInfo
  {
   struct chunkInfo *prevChunk;
   struct chunkInfo *nextFree;
   struct chunkInfo *lastFree;
   long int size;
  };

struct blockInfo
  {
   struct blockInfo *nextBlock;
   struct blockInfo *prevBlock;
   struct chunkInfo *nextFree;
   long int size;
  };

struct memoryPtr
  {
   struct memoryPtr *next;
  };

struct longMemoryPtr
  {
   struct longMemoryPtr *prev;
   struct longMemoryPtr *next;
   long size;
  };

#define get_struct(theEnv,type) \
  ((MemoryData(theEnv)->MemoryTable[sizeof(struct type)] == NULL) ? \
   ((struct type *) genalloc(theEnv,(unsigned) sizeof(struct type))) :\
   ((MemoryData(theEnv)->TempMemoryPtr = MemoryData(theEnv)->MemoryTable[sizeof(struct type)]),\
    MemoryData(theEnv)->MemoryTable[sizeof(struct type)] = MemoryData(theEnv)->TempMemoryPtr->next,\
    ((struct type *) MemoryData(theEnv)->TempMemoryPtr)))

#define rtn_struct(theEnv,type,struct_ptr) \
  (MemoryData(theEnv)->TempMemoryPtr = (struct memoryPtr *) struct_ptr,\
   MemoryData(theEnv)->TempMemoryPtr->next = MemoryData(theEnv)->MemoryTable[sizeof(struct type)], \
   MemoryData(theEnv)->MemoryTable[sizeof(struct type)] = MemoryData(theEnv)->TempMemoryPtr)

#define rtn_sized_struct(theEnv,size,struct_ptr) \
  (MemoryData(theEnv)->TempMemoryPtr = (struct memoryPtr *) struct_ptr,\
   MemoryData(theEnv)->TempMemoryPtr->next = MemoryData(theEnv)->MemoryTable[size], \
   MemoryData(theEnv)->MemoryTable[size] = MemoryData(theEnv)->TempMemoryPtr)

#define get_var_struct(theEnv,type,vsize) \
  ((((sizeof(struct type) + vsize) <  MEM_TABLE_SIZE) ? \
    (MemoryData(theEnv)->MemoryTable[sizeof(struct type) + vsize] == NULL) : 1) ? \
   ((struct type *) genalloc(theEnv,(unsigned) (sizeof(struct type) + vsize))) :\
   ((MemoryData(theEnv)->TempMemoryPtr = MemoryData(theEnv)->MemoryTable[sizeof(struct type) + vsize]),\
    MemoryData(theEnv)->MemoryTable[sizeof(struct type) + vsize] = MemoryData(theEnv)->TempMemoryPtr->next,\
    ((struct type *) MemoryData(theEnv)->TempMemoryPtr)))

#define rtn_var_struct(theEnv,type,vsize,struct_ptr) \
  (MemoryData(theEnv)->TempSize = sizeof(struct type) + vsize, \
   ((MemoryData(theEnv)->TempSize < MEM_TABLE_SIZE) ? \
    (MemoryData(theEnv)->TempMemoryPtr = (struct memoryPtr *) struct_ptr,\
     MemoryData(theEnv)->TempMemoryPtr->next = MemoryData(theEnv)->MemoryTable[MemoryData(theEnv)->TempSize], \
     MemoryData(theEnv)->MemoryTable[MemoryData(theEnv)->TempSize] =  MemoryData(theEnv)->TempMemoryPtr) : \
    (genfree(theEnv,(void *) struct_ptr,(unsigned) MemoryData(theEnv)->TempSize),(struct memoryPtr *) struct_ptr)))

#define get_var_struct2(theEnv,type,vsize) \
  ((((sizeof(struct type) + vsize) <  (unsigned long) MEM_TABLE_SIZE) ? \
    (MemoryData(theEnv)->MemoryTable[sizeof(struct type) + vsize] == NULL) : 1) ? \
   ((struct type *) gm3(theEnv,(long) (sizeof(struct type) + vsize))) :\
   ((MemoryData(theEnv)->TempMemoryPtr = MemoryData(theEnv)->MemoryTable[sizeof(struct type) + vsize]),\
    MemoryData(theEnv)->MemoryTable[sizeof(struct type) + vsize] = MemoryData(theEnv)->TempMemoryPtr->next,\
    ((struct type *) MemoryData(theEnv)->TempMemoryPtr)))

#define rtn_var_struct2(theEnv,type,vsize,struct_ptr) \
  (MemoryData(theEnv)->TempSize2 = sizeof(struct type) + vsize, \
   (((MemoryData(theEnv)->TempSize2 <  (unsigned long) MEM_TABLE_SIZE) ? \
     (MemoryData(theEnv)->TempMemoryPtr = (struct memoryPtr *) struct_ptr,\
      MemoryData(theEnv)->TempMemoryPtr->next = MemoryData(theEnv)->MemoryTable[MemoryData(theEnv)->TempSize2], \
      MemoryData(theEnv)->MemoryTable[MemoryData(theEnv)->TempSize2] =  MemoryData(theEnv)->TempMemoryPtr) : \
     (rm3(theEnv,(void *) struct_ptr,(long) (sizeof(struct type) + vsize)),(struct memoryPtr *) struct_ptr))))

#define GenCopyMemory(type,cnt,dst,src) \
   memcpy((void *) (dst),(void *) (src),sizeof(type) * (size_t) (cnt))

#define MEMORY_DATA 59

struct memoryData
  { 
   long int MemoryAmount;
   long int MemoryCalls;
   intBool ConserveMemory;
   int (*OutOfMemoryFunction)(void *,unsigned long);
#if BLOCK_MEMORY
   struct longMemoryPtr *TopLongMemoryPtr;
   struct blockInfo *TopMemoryBlock;
   int BlockInfoSize;
   int ChunkInfoSize;
   int BlockMemoryInitialized;
#endif
   struct memoryPtr *TempMemoryPtr;
   struct memoryPtr **MemoryTable;
   unsigned int TempSize;
   unsigned long TempSize2;
  };

#define MemoryData(theEnv) ((struct memoryData *) GetEnvironmentData(theEnv,MEMORY_DATA))

#if ENVIRONMENT_API_ONLY
#define GetConserveMemory(theEnv) EnvGetConserveMemory(theEnv)
#define MemRequests(theEnv) EnvMemRequests(theEnv)
#define MemUsed(theEnv) EnvMemUsed(theEnv)
#define ReleaseMem(theEnv,a,b) EnvReleaseMem(theEnv,a,b)
#define SetConserveMemory(theEnv,a) EnvSetConserveMemory(theEnv,a)
#define SetOutOfMemoryFunction(theEnv,a) EnvSetOutOfMemoryFunction(theEnv,a)
#else
#define GetConserveMemory() EnvGetConserveMemory(GetCurrentEnvironment())
#define MemRequests() EnvMemRequests(GetCurrentEnvironment())
#define MemUsed() EnvMemUsed(GetCurrentEnvironment())
#define ReleaseMem(a,b) EnvReleaseMem(GetCurrentEnvironment(),a,b)
#define SetConserveMemory(a) EnvSetConserveMemory(GetCurrentEnvironment(),a)
#define SetOutOfMemoryFunction(a) EnvSetOutOfMemoryFunction(GetCurrentEnvironment(),a)
#endif

   LOCALE void                           InitializeMemory(void *);
   LOCALE void                          *genalloc(void *,unsigned int);
   LOCALE int                            DefaultOutOfMemoryFunction(void *,unsigned long);
   LOCALE int                          (*EnvSetOutOfMemoryFunction(void *,int (*)(void *,unsigned long)))(void *,unsigned long);
   LOCALE int                            genfree(void *,void *,unsigned int);
   LOCALE void                          *genrealloc(void *,void *,unsigned int,unsigned int);
   LOCALE long                           EnvMemUsed(void *);
   LOCALE long                           EnvMemRequests(void *);
   LOCALE long                           UpdateMemoryUsed(void *,long int);
   LOCALE long                           UpdateMemoryRequests(void *,long int);
   LOCALE long                           EnvReleaseMem(void *,long,int);
   LOCALE void                          *gm1(void *,int);
   LOCALE void                          *gm2(void *,unsigned int);
   LOCALE void                          *gm3(void *,long);
   LOCALE int                            rm(void *,void *,unsigned);
   LOCALE int                            rm3(void *,void *,long);
   LOCALE unsigned long                  PoolSize(void *);
   LOCALE unsigned long                  ActualPoolSize(void *);
   LOCALE void                          *RequestChunk(void *,unsigned int);
   LOCALE int                            ReturnChunk(void *,void *,unsigned int);
   LOCALE void                          *genlongalloc(void *,unsigned long);
   LOCALE int                            genlongfree(void *,void *,unsigned long);
   LOCALE intBool                        EnvSetConserveMemory(void *,intBool);
   LOCALE intBool                        EnvGetConserveMemory(void *);
   LOCALE void                           genmemcpy(char *,char *,unsigned long);
   LOCALE void                           ReturnAllBlocks(void *);

#endif






