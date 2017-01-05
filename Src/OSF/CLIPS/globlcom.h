   /*******************************************************/
   /*      "C" Language Integrated Production System      */
   /*                                                     */
   /*             CLIPS Version 6.24  06/05/06            */
   /*                                                     */
   /*            DEFGLOBAL COMMANDS HEADER FILE           */
   /*******************************************************/

/*************************************************************/
/* Purpose:                                                  */
/*                                                           */
/* Principal Programmer(s):                                  */
/*      Gary D. Riley                                        */
/*                                                           */
/* Contributing Programmer(s):                               */
/*      Brian L. Donnell                                     */
/*                                                           */
/* Revision History:                                         */
/*                                                           */
/*      6.24: Renamed BOOLEAN macro type to intBool.         */
/*                                                           */
/*************************************************************/

#ifndef _H_globlcom
#define _H_globlcom

#ifdef LOCALE
#undef LOCALE
#endif

#ifdef _GLOBLCOM_SOURCE_
#define LOCALE
#else
#define LOCALE extern
#endif

#if ENVIRONMENT_API_ONLY
#define GetResetGlobals(theEnv) EnvGetResetGlobals(theEnv)
#define SetResetGlobals(theEnv,a) EnvSetResetGlobals(theEnv,a)
#define ShowDefglobals(theEnv,a,b) EnvShowDefglobals(theEnv,a,b)
#else
#define GetResetGlobals() EnvGetResetGlobals(GetCurrentEnvironment())
#define SetResetGlobals(a) EnvSetResetGlobals(GetCurrentEnvironment(),a)
#define ShowDefglobals(a,b) EnvShowDefglobals(GetCurrentEnvironment(),a,b)
#endif

   LOCALE void                           DefglobalCommandDefinitions(void *);
   LOCALE int                            SetResetGlobalsCommand(void *);
   LOCALE intBool                        EnvSetResetGlobals(void *,int);
   LOCALE int                            GetResetGlobalsCommand(void *);
   LOCALE intBool                        EnvGetResetGlobals(void *);
   LOCALE void                           ShowDefglobalsCommand(void *);
   LOCALE void                           EnvShowDefglobals(void *,char *,void *);

#endif

