   /*******************************************************/
   /*      "C" Language Integrated Production System      */
   /*                                                     */
   /*             CLIPS Version 6.21  06/15/03            */
   /*                                                     */
   /*               FACT COMMANDS HEADER FILE             */
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
/*************************************************************/

#ifndef _H_factcom
#define _H_factcom

#ifndef _H_evaluatn
#include "evaluatn.h"
#endif

#ifdef LOCALE
#undef LOCALE
#endif

#ifdef _FACTCOM_SOURCE_
#define LOCALE
#else
#define LOCALE extern
#endif

#if ENVIRONMENT_API_ONLY
#define Facts(theEnv,a,b,c,d,e) EnvFacts(theEnv,a,b,c,d,e)
#define LoadFacts(theEnv,a) EnvLoadFacts(theEnv,a)
#define SaveFacts(theEnv,a,b,c) EnvSaveFacts(theEnv,a,b,c)
#define LoadFactsFromString(theEnv,a,b) EnvLoadFactsFromString(theEnv,a,b)
#else
#define Facts(a,b,c,d,e) EnvFacts(GetCurrentEnvironment(),a,b,c,d,e)
#define LoadFacts(a) EnvLoadFacts(GetCurrentEnvironment(),a)
#define SaveFacts(a,b,c) EnvSaveFacts(GetCurrentEnvironment(),a,b,c)
#define LoadFactsFromString(a,b) EnvLoadFactsFromString(GetCurrentEnvironment(),a,b)
#endif

   LOCALE void                           FactCommandDefinitions(void *);
   LOCALE void                           AssertCommand(void *,DATA_OBJECT_PTR);
   LOCALE void                           RetractCommand(void *);
   LOCALE void                           AssertStringFunction(void *,DATA_OBJECT_PTR);
   LOCALE void                           FactsCommand(void *);
   LOCALE void                           EnvFacts(void *,char *,void *,long,long,long);
   LOCALE int                            SetFactDuplicationCommand(void *);
   LOCALE int                            GetFactDuplicationCommand(void *);
   LOCALE int                            SaveFactsCommand(void *);
   LOCALE int                            LoadFactsCommand(void *);
   LOCALE int                            EnvSaveFacts(void *,char *,int,struct expr *);
   LOCALE int                            EnvLoadFacts(void *,char *);
   LOCALE int                            EnvLoadFactsFromString(void *,char *,int);
   LOCALE long int                       FactIndexFunction(void *);

#endif


