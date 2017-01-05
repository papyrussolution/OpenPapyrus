/*******************************************************/
/*      "C" Language Integrated Production System      */
/*                                                     */
/*             CLIPS Version 6.24  06/05/06            */
/*                                                     */
/*                 UTILITY HEADER FILE                 */
/*******************************************************/

/*************************************************************/
/* Purpose: Provides a set of utility functions useful to    */
/*   other modules. Primarily these are the functions for    */
/*   handling periodic garbage collection and appending      */
/*   string data.                                            */
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

#ifndef _H_utility
#define _H_utility

#ifdef LOCALE
 #undef LOCALE
#endif

struct cleanupFunction {
	char * name;
	void (* ip)(void *);
	int priority;
	struct cleanupFunction * next;
	short int environmentAware;
};

struct callFunctionItem {
	char * name;
	void (* func)(void *);
	int priority;
	struct callFunctionItem * next;
	short int environmentAware;
};

#define UTILITY_DATA 55

struct utilityData {
	struct cleanupFunction * ListOfCleanupFunctions;
	struct cleanupFunction * ListOfPeriodicFunctions;
	short GarbageCollectionLocks;
	short GarbageCollectionHeuristicsEnabled;
	short PeriodicFunctionsEnabled;
	short YieldFunctionEnabled;
	unsigned long EphemeralItemCount;
	unsigned long EphemeralItemSize;
	unsigned long CurrentEphemeralCountMax;
	unsigned long CurrentEphemeralSizeMax;
	void (* YieldTimeFunction)(void);
	int LastEvaluationDepth;
};

#define UtilityData(theEnv) ((struct utilityData *)GetEnvironmentData(theEnv, UTILITY_DATA))

#ifdef _UTILITY_SOURCE_
 #define LOCALE
#else
 #define LOCALE extern
#endif

#if ENVIRONMENT_API_ONLY
 #define DecrementGCLocks(theEnv) EnvDecrementGCLocks(theEnv)
 #define IncrementGCLocks(theEnv) EnvIncrementGCLocks(theEnv)
 #define RemovePeriodicFunction(theEnv, a) EnvRemovePeriodicFunction(theEnv, a)
#else
 #define DecrementGCLocks() EnvDecrementGCLocks(GetCurrentEnvironment())
 #define IncrementGCLocks() EnvIncrementGCLocks(GetCurrentEnvironment())
 #define RemovePeriodicFunction(a) EnvRemovePeriodicFunction(GetCurrentEnvironment(), a)
#endif

LOCALE void                           InitializeUtilityData(void *);
LOCALE void PeriodicCleanup(void *, intBool, intBool);
LOCALE intBool AddCleanupFunction(void *, char *, void (*)(void *), int);
LOCALE intBool EnvAddPeriodicFunction(void *, char *, void (*)(void *), int);
LOCALE intBool AddPeriodicFunction(char *, void (*)(void), int);
LOCALE intBool                        RemoveCleanupFunction(void *, char *);
LOCALE intBool                        EnvRemovePeriodicFunction(void *, char *);
LOCALE char * AppendStrings(void *, char *, char *);
LOCALE char * StringPrintForm(void *, char *);
LOCALE char * AppendToString(void *, char *, char *, int *, unsigned *);
LOCALE char * AppendNToString(void *, char *, char *, unsigned, int *, unsigned *);
LOCALE char * ExpandStringWithChar(void *, int, char *, int *, unsigned *, unsigned);
LOCALE struct callFunctionItem * AddFunctionToCallList(void *, char *, int, void (*)(void *),
                                                       struct callFunctionItem *, intBool);
LOCALE struct callFunctionItem * RemoveFunctionFromCallList(void *, char *, struct callFunctionItem *, int *);
LOCALE void                           DeallocateCallList(void *, struct callFunctionItem *);
LOCALE unsigned                       ItemHashValue(void *, unsigned short, void *, unsigned);
LOCALE void                           YieldTime(void *);
LOCALE short                          SetGarbageCollectionHeuristics(void *, short);
LOCALE void                           EnvIncrementGCLocks(void *);
LOCALE void                           EnvDecrementGCLocks(void *);
LOCALE short                          EnablePeriodicFunctions(void *, short);
LOCALE short                          EnableYieldFunction(void *, short);

#endif

