/*******************************************************/
/*      "C" Language Integrated Production System      */
/*                                                     */
/*             CLIPS Version 6.24  06/05/06            */
/*                                                     */
/*                  WATCH HEADER FILE                  */
/*******************************************************/

/*************************************************************/
/* Purpose: Support functions for the watch and unwatch      */
/*   commands.                                               */
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
/*            Added EnvSetWatchItem function.                */
/*                                                           */
/*************************************************************/

#ifndef _H_watch
#define _H_watch

#ifndef _H_expressn
 #include "expressn.h"
#endif

#define WATCH_DATA 54

struct watchItem {
	char * name;
	unsigned * flag;
	int code, priority;
	unsigned (* accessFunc)(void *, int, unsigned, struct expr *);
	unsigned (* printFunc)(void *, char *, int, struct expr *);
	struct watchItem * next;
};

struct watchData {
	struct watchItem * ListOfWatchItems;
};

#define WatchData(theEnv) ((struct watchData *)GetEnvironmentData(theEnv, WATCH_DATA))

#ifdef LOCALE
 #undef LOCALE
#endif

#ifdef _WATCH_SOURCE_
 #define LOCALE
#else
 #define LOCALE extern
#endif

#if ENVIRONMENT_API_ONLY
 #define GetWatchItem(theEnv, a) EnvGetWatchItem(theEnv, a)
 #define SetWatchItem(theEnv, a, b) EnvSetWatchItem(theEnv, a, b)
 #define Watch(theEnv, a) EnvWatch(theEnv, a)
 #define Unwatch(theEnv, a) EnvUnwatch(theEnv, a)
#else
 #define GetWatchItem(a) EnvGetWatchItem(GetCurrentEnvironment(), a)
 #define SetWatchItem(a, b) EnvSetWatchItem(GetCurrentEnvironment(), a, b)
 #define Watch(a) EnvWatch(GetCurrentEnvironment(), a)
 #define Unwatch(a) EnvUnwatch(GetCurrentEnvironment(), a)
#endif

LOCALE void InitializeWatchData(void *);
LOCALE int  EnvSetWatchItem(void *, char *, unsigned, struct expr *);
LOCALE int  EnvGetWatchItem(void *, char *);
LOCALE intBool AddWatchItem(void *, char *, int, unsigned *, int, unsigned (*)(void *, int, unsigned, struct expr *),
	unsigned (*)(void *, char *, int, struct expr *));
LOCALE char * GetNthWatchName(void *, int);
LOCALE int  GetNthWatchValue(void *, int);
LOCALE void WatchCommand(void *);
LOCALE void UnwatchCommand(void *);
LOCALE void ListWatchItemsCommand(void *);
LOCALE void WatchFunctionDefinitions(void *);
LOCALE intBool EnvWatch(void *, char *);
LOCALE intBool EnvUnwatch(void *, char *);
LOCALE int GetWatchItemCommand(void *);

#endif

