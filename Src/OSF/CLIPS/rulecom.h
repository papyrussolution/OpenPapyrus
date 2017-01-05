   /*******************************************************/
   /*      "C" Language Integrated Production System      */
   /*                                                     */
   /*             CLIPS Version 6.24  06/05/06            */
   /*                                                     */
   /*             DEFRULE COMMANDS HEADER FILE            */
   /*******************************************************/

/*************************************************************/
/* Purpose: Provides the matches command. Also provides the  */
/*   the developer commands show-joins and rule-complexity.  */
/*   Also provides the initialization routine which          */
/*   registers rule commands found in other modules.         */
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

#ifndef _H_rulecom
#define _H_rulecom

#ifndef _H_evaluatn
#include "evaluatn.h"
#endif

#ifdef LOCALE
#undef LOCALE
#endif

#ifdef _RULECOM_SOURCE_
#define LOCALE
#else
#define LOCALE extern
#endif

#if ENVIRONMENT_API_ONLY
#define Matches(theEnv,a) EnvMatches(theEnv,a)
#else
#define Matches(a) EnvMatches(GetCurrentEnvironment(),a)
#endif

   LOCALE intBool                        EnvMatches(void *,void *);
   LOCALE void                           DefruleCommands(void *);
   LOCALE void                           MatchesCommand(void *);
#if DEVELOPER
   LOCALE void                           ShowJoinsCommand(void *);
   LOCALE long                           RuleComplexityCommand(void *);
#endif

#endif


