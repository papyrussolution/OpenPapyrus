   /*******************************************************/
   /*      "C" Language Integrated Production System      */
   /*                                                     */
   /*             CLIPS Version 6.24  06/05/06            */
   /*                                                     */
   /*                 FACT HASHING MODULE                 */
   /*******************************************************/

/*************************************************************/
/* Purpose:                                                  */
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

#ifndef _H_facthsh

#define _H_facthsh

struct factHashEntry;

#ifndef _H_factmngr
#include "factmngr.h"
#endif

struct factHashEntry
  {
   struct fact *theFact;
   struct factHashEntry *next;
  };

#define SIZE_FACT_HASH  7717

#ifdef LOCALE
#undef LOCALE
#endif
#ifdef _FACTHSH_SOURCE_
#define LOCALE
#else
#define LOCALE extern
#endif

#if ENVIRONMENT_API_ONLY
#define GetFactDuplication(theEnv) EnvGetFactDuplication(theEnv)
#define SetFactDuplication(theEnv,a) EnvSetFactDuplication(theEnv,a)
#else
#define GetFactDuplication() EnvGetFactDuplication(GetCurrentEnvironment())
#define SetFactDuplication(a) EnvSetFactDuplication(GetCurrentEnvironment(),a)
#endif

   LOCALE void                           AddHashedFact(void *,struct fact *,int);
   LOCALE intBool                        RemoveHashedFact(void *,struct fact *);
   LOCALE int                            HandleFactDuplication(void *,void *);
   LOCALE intBool                        EnvGetFactDuplication(void *);
   LOCALE intBool                        EnvSetFactDuplication(void *,int);
   LOCALE void                           InitializeFactHashTable(void *);
   LOCALE void                           ShowFactHashTable(void *);
   LOCALE int                            HashFact(struct fact *);

#endif


