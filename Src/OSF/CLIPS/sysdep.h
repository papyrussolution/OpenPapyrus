   /*******************************************************/
   /*      "C" Language Integrated Production System      */
   /*                                                     */
   /*             CLIPS Version 6.24  06/05/06            */
   /*                                                     */
   /*            SYSTEM DEPENDENT HEADER FILE             */
   /*******************************************************/

/*************************************************************/
/* Purpose: Isolation of system dependent routines.          */
/*                                                           */
/* Principal Programmer(s):                                  */
/*      Gary D. Riley                                        */
/*                                                           */
/* Contributing Programmer(s):                               */
/*                                                           */
/* Revision History:                                         */
/*                                                           */
/*      6.24: Support for run-time programs directly passing */
/*            the hash tables for initialization.            */
/*                                                           */
/*            Added BeforeOpenFunction and AfterOpenFunction */
/*            hooks.                                         */
/*                                                           */
/*            Added environment parameter to GenClose.       */
/*            Added environment parameter to GenOpen.        */
/*                                                           */
/*************************************************************/

#ifndef _H_sysdep
#define _H_sysdep

#ifndef _H_symbol
#include "symbol.h"
#endif

#ifndef _STDIO_INCLUDED_
#define _STDIO_INCLUDED_
#include <stdio.h>
#endif

#if IBM_TBC || IBM_MSC || IBM_ICB
#include <dos.h>
#endif

#ifdef LOCALE
#undef LOCALE
#endif

#ifdef _SYSDEP_SOURCE_
#define LOCALE
#else
#define LOCALE extern
#endif

   LOCALE void                        InitializeEnvironment(void);
   LOCALE void                        EnvInitializeEnvironment(void *,struct symbolHashNode **,struct floatHashNode **,
                                                               struct integerHashNode **,struct bitMapHashNode **);
   LOCALE void                        SetRedrawFunction(void *,void (*)(void *));
   LOCALE void                        SetPauseEnvFunction(void *,void (*)(void *));
   LOCALE void                        SetContinueEnvFunction(void *,void (*)(void *,int));
   LOCALE void                        (*GetRedrawFunction(void *))(void *);
   LOCALE void                        (*GetPauseEnvFunction(void *))(void *);
   LOCALE void                        (*GetContinueEnvFunction(void *))(void *,int);
   LOCALE void                        RerouteStdin(void *,int,char *[]);
   LOCALE double                      gentime(void);
   LOCALE void                        gensystem(void *theEnv);
   LOCALE void                        VMSSystem(char *);
   LOCALE int                         GenOpenReadBinary(void *,char *,char *);
   LOCALE void                        GetSeekCurBinary(void *,long);
   LOCALE void                        GetSeekSetBinary(void *,long);
   LOCALE void                        GenTellBinary(void *,long *);
   LOCALE void                        GenCloseBinary(void *);
   LOCALE void                        GenReadBinary(void *,void *,unsigned long);
   LOCALE FILE                       *GenOpen(void *,char *,char *);
   LOCALE int                         GenClose(void *,FILE *);
   LOCALE void                        genexit(int);
   LOCALE int                         genrand(void);
   LOCALE void                        genseed(int);
   LOCALE int                         genremove(char *);
   LOCALE int                         genrename(char *,char *);
   LOCALE char                       *gengetcwd(char *,int);
   LOCALE void                        GenWrite(void *,unsigned long,FILE *);
   LOCALE int                       (*EnvSetBeforeOpenFunction(void *,int (*)(void *)))(void *);
   LOCALE int                       (*EnvSetAfterOpenFunction(void *,int (*)(void *)))(void *);

#endif





