/*******************************************************/
/*      "C" Language Integrated Production System      */
/*                                                     */
/*             CLIPS Version 6.24  06/05/06            */
/*                                                     */
/*                   UTILITY MODULE                    */
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
/*      Brian Donnell                                        */
/*                                                           */
/* Revision History:                                         */
/*                                                           */
/*      6.24: Renamed BOOLEAN macro type to intBool.         */
/*                                                           */
/*************************************************************/

#define _UTILITY_SOURCE_

#include "setup.h"

#include <ctype.h>
#include <stdlib.h>

#include <stdio.h>
#define _STDIO_INCLUDED_
#include <string.h>

#include "envrnmnt.h"
#include "evaluatn.h"
#include "facthsh.h"
#include "memalloc.h"
#include "multifld.h"
#include "prntutil.h"

#include "utility.h"

#define MAX_EPHEMERAL_COUNT 1000L
#define MAX_EPHEMERAL_SIZE 10240L
#define COUNT_INCREMENT 1000L
#define SIZE_INCREMENT 10240L

/***************************************/
/* LOCAL INTERNAL FUNCTION DEFINITIONS */
/***************************************/

static intBool                 AddCPFunction(void *, char *, void (*)(void *), int, struct cleanupFunction **, intBool);
static intBool                 RemoveCPFunction(void *, char *, struct cleanupFunction **);
static void                    DeallocateUtilityData(void *);

/************************************************/
/* InitializeUtilityData: Allocates environment */
/*    data for utility routines.                */
/************************************************/
globle void InitializeUtilityData(
	void * theEnv)
{
	AllocateEnvironmentData(theEnv, UTILITY_DATA, sizeof(struct utilityData), DeallocateUtilityData);

	UtilityData(theEnv)->GarbageCollectionLocks = 0;
	UtilityData(theEnv)->GarbageCollectionHeuristicsEnabled = TRUE;
	UtilityData(theEnv)->PeriodicFunctionsEnabled = TRUE;
	UtilityData(theEnv)->YieldFunctionEnabled = TRUE;

	UtilityData(theEnv)->CurrentEphemeralCountMax = MAX_EPHEMERAL_COUNT;
	UtilityData(theEnv)->CurrentEphemeralSizeMax = MAX_EPHEMERAL_SIZE;
	UtilityData(theEnv)->LastEvaluationDepth = -1;
}

/**************************************************/
/* DeallocateUtilityData: Deallocates environment */
/*    data for utility routines.                  */
/**************************************************/
static void DeallocateUtilityData(
	void * theEnv)
{
	struct cleanupFunction * tmpPtr, * nextPtr;

	tmpPtr = UtilityData(theEnv)->ListOfPeriodicFunctions;
	while(tmpPtr != NULL) {
		nextPtr = tmpPtr->next;
		rtn_struct(theEnv, cleanupFunction, tmpPtr);
		tmpPtr = nextPtr;
	}
	tmpPtr = UtilityData(theEnv)->ListOfCleanupFunctions;
	while(tmpPtr != NULL) {
		nextPtr = tmpPtr->next;
		rtn_struct(theEnv, cleanupFunction, tmpPtr);
		tmpPtr = nextPtr;
	}
}

/*************************************************************/
/* PeriodicCleanup: Returns garbage created during execution */
/*   that has not been returned to the memory pool yet. The  */
/*   cleanup is normally deferred so that an executing rule  */
/*   can still access these data structures. Always calls a  */
/*   series of functions that should be called periodically. */
/*   Usually used by interfaces to update displays.          */
/*************************************************************/
globle void PeriodicCleanup(
	void * theEnv,
	intBool cleanupAllDepths,
	intBool useHeuristics)
{
	int oldDepth = -1;
	struct cleanupFunction * cleanupPtr, * periodPtr;

	/*===================================*/
	/* Don't use heuristics if disabled. */
	/*===================================*/
	if(!UtilityData(theEnv)->GarbageCollectionHeuristicsEnabled) {
		useHeuristics = FALSE;
	}
	/*=============================================*/
	/* Call functions for handling periodic tasks. */
	/*=============================================*/
	if(UtilityData(theEnv)->PeriodicFunctionsEnabled) {
		for(periodPtr = UtilityData(theEnv)->ListOfPeriodicFunctions;
		    periodPtr != NULL;
		    periodPtr = periodPtr->next) {
			if(periodPtr->environmentAware) {
				(*periodPtr->ip)(theEnv);
			}
			else {(*(void (*)(void))periodPtr->ip)(); }
		}
	}
	/*===================================================*/
	/* If the last level we performed cleanup was deeper */
	/* than the current level, reset the values used by  */
	/* the heuristics to determine if garbage collection */
	/* should be performed. If the heuristic values had  */
	/* to be incremented because there was no garbage    */
	/* that could be cleaned up, we don't want to keep   */
	/* those same high values permanently so we reset    */
	/* them when we go back to a lower evaluation depth. */
	/*===================================================*/
	if(UtilityData(theEnv)->LastEvaluationDepth > EvaluationData(theEnv)->CurrentEvaluationDepth) {
		UtilityData(theEnv)->LastEvaluationDepth = EvaluationData(theEnv)->CurrentEvaluationDepth;
		UtilityData(theEnv)->CurrentEphemeralCountMax = MAX_EPHEMERAL_COUNT;
		UtilityData(theEnv)->CurrentEphemeralSizeMax = MAX_EPHEMERAL_SIZE;
	}
	/*======================================================*/
	/* If we're using heuristics to determine if garbage    */
	/* collection to occur, then check to see if enough     */
	/* garbage has been created to make cleanup worthwhile. */
	/*======================================================*/
	if(UtilityData(theEnv)->GarbageCollectionLocks > 0) return;
	if(useHeuristics &&
	   (UtilityData(theEnv)->EphemeralItemCount < UtilityData(theEnv)->CurrentEphemeralCountMax) &&
	   (UtilityData(theEnv)->EphemeralItemSize < UtilityData(theEnv)->CurrentEphemeralSizeMax)) {
		return;
	}
	/*==========================================================*/
	/* If cleanup is being performed at all depths, rather than */
	/* just the current evaluation depth, then temporarily set  */
	/* the evaluation depth to a level that will force cleanup  */
	/* at all depths.                                           */
	/*==========================================================*/
	if(cleanupAllDepths) {
		oldDepth = EvaluationData(theEnv)->CurrentEvaluationDepth;
		EvaluationData(theEnv)->CurrentEvaluationDepth = -1;
	}
	/*=============================================*/
	/* Free up multifield values no longer in use. */
	/*=============================================*/

	FlushMultifields(theEnv);

	/*=====================================*/
	/* Call the list of cleanup functions. */
	/*=====================================*/

	for(cleanupPtr = UtilityData(theEnv)->ListOfCleanupFunctions;
	    cleanupPtr != NULL;
	    cleanupPtr = cleanupPtr->next) {
		if(cleanupPtr->environmentAware) {
			(*cleanupPtr->ip)(theEnv);
		}
		else {(*(void (*)(void))cleanupPtr->ip)(); }
	}
	/*================================================*/
	/* Free up atomic values that are no longer used. */
	/*================================================*/

	RemoveEphemeralAtoms(theEnv);

	/*=========================================*/
	/* Restore the evaluation depth if cleanup */
	/* was performed on all depths.            */
	/*=========================================*/
	if(cleanupAllDepths) EvaluationData(theEnv)->CurrentEvaluationDepth = oldDepth;
	/*============================================================*/
	/* If very little memory was freed up, then increment the     */
	/* values used by the heuristics so that we don't continually */
	/* try to free up memory that isn't being released.           */
	/*============================================================*/
	if((UtilityData(theEnv)->EphemeralItemCount+COUNT_INCREMENT) > UtilityData(theEnv)->CurrentEphemeralCountMax) {
		UtilityData(theEnv)->CurrentEphemeralCountMax = UtilityData(theEnv)->EphemeralItemCount+COUNT_INCREMENT;
	}
	if((UtilityData(theEnv)->EphemeralItemSize+SIZE_INCREMENT) > UtilityData(theEnv)->CurrentEphemeralSizeMax) {
		UtilityData(theEnv)->CurrentEphemeralSizeMax = UtilityData(theEnv)->EphemeralItemSize+SIZE_INCREMENT;
	}
	/*===============================================================*/
	/* Remember the evaluation depth at which garbage collection was */
	/* last performed. This information is used for resetting the    */
	/* ephemeral count and size numbers used by the heuristics.      */
	/*===============================================================*/

	UtilityData(theEnv)->LastEvaluationDepth = EvaluationData(theEnv)->CurrentEvaluationDepth;
}

/***************************************************/
/* AddCleanupFunction: Adds a function to the list */
/*   of functions called to perform cleanup such   */
/*   as returning free memory to the memory pool.  */
/***************************************************/
globle intBool AddCleanupFunction(
	void * theEnv,
	char * name,
	void (* theFunction)(void *),
	int priority)
{
	return AddCPFunction(theEnv, name, theFunction, priority, &UtilityData(theEnv)->ListOfCleanupFunctions, TRUE);
}

#if (!ENVIRONMENT_API_ONLY) && ALLOW_ENVIRONMENT_GLOBALS
/****************************************************/
/* AddPeriodicFunction: Adds a function to the list */
/*   of functions called to handle periodic tasks.  */
/****************************************************/
globle intBool AddPeriodicFunction(
	char * name,
	void (* theFunction)(void),
	int priority)
{
	void * theEnv;

	theEnv = GetCurrentEnvironment();

	return AddCPFunction(theEnv, name, (void (*)(void *))theFunction, priority,
		&UtilityData(theEnv)->ListOfPeriodicFunctions, FALSE);
}

#endif

/*******************************************************/
/* EnvAddPeriodicFunction: Adds a function to the list */
/*   of functions called to handle periodic tasks.     */
/*******************************************************/
globle intBool EnvAddPeriodicFunction(
	void * theEnv,
	char * name,
	void (* theFunction)(void *),
	int priority)
{
	return AddCPFunction(theEnv, name, theFunction, priority, &UtilityData(theEnv)->ListOfPeriodicFunctions, TRUE);
}

/**********************************/
/* AddCPFunction: Adds a function */
/*   to a list of functions.      */
/**********************************/
static intBool AddCPFunction(
	void * theEnv,
	char * name,
	void (* theFunction)(void *),
	int priority,
	struct cleanupFunction ** head,
	intBool environmentAware)
{
	struct cleanupFunction * newPtr, * currentPtr, * lastPtr = NULL;

	newPtr = get_struct(theEnv, cleanupFunction);

	newPtr->name = name;
	newPtr->ip = theFunction;
	newPtr->priority = priority;
	newPtr->environmentAware = (short)environmentAware;
	if(*head == NULL) {
		newPtr->next = NULL;
		*head = newPtr;
		return 1;
	}
	currentPtr = *head;
	while((currentPtr != NULL) ? (priority < currentPtr->priority) : FALSE) {
		lastPtr = currentPtr;
		currentPtr = currentPtr->next;
	}
	if(lastPtr == NULL) {
		newPtr->next = *head;
		*head = newPtr;
	}
	else {
		newPtr->next = currentPtr;
		lastPtr->next = newPtr;
	}
	return 1;
}

/*******************************************************/
/* RemoveCleanupFunction: Removes a function from the  */
/*   list of functions called to perform cleanup such  */
/*   as returning free memory to the memory pool.      */
/*******************************************************/
globle intBool RemoveCleanupFunction(
	void * theEnv,
	char * name)
{
	return RemoveCPFunction(theEnv, name, &UtilityData(theEnv)->ListOfCleanupFunctions);
}

/**********************************************************/
/* EnvRemovePeriodicFunction: Removes a function from the */
/*   list of functions called to handle periodic tasks.   */
/**********************************************************/
globle intBool EnvRemovePeriodicFunction(
	void * theEnv,
	char * name)
{
	return RemoveCPFunction(theEnv, name, &UtilityData(theEnv)->ListOfPeriodicFunctions);
}

/****************************************/
/* RemoveCPFunction: Removes a function */
/*   from a list of functions.          */
/****************************************/
static intBool RemoveCPFunction(
	void * theEnv,
	char * name,
	struct cleanupFunction ** head)
{
	struct cleanupFunction * currentPtr, * lastPtr;

	lastPtr = NULL;
	currentPtr = *head;

	while(currentPtr != NULL) {
		if(strcmp(name, currentPtr->name) == 0) {
			if(lastPtr == NULL) {
				*head = currentPtr->next;
			}
			else {lastPtr->next = currentPtr->next; }
			rtn_struct(theEnv, cleanupFunction, currentPtr);
			return TRUE;
		}
		lastPtr = currentPtr;
		currentPtr = currentPtr->next;
	}
	return FALSE;
}

/*****************************************************/
/* StringPrintForm: Generates printed representation */
/*   of a string. Replaces / with // and " with /".  */
/*****************************************************/
globle char * StringPrintForm(
	void * theEnv,
	char * str)
{
	int i = 0, pos = 0;
	unsigned max = 0;
	char * theString = NULL;
	void * thePtr;

	theString = ExpandStringWithChar(theEnv, '"', theString, &pos, &max, max+80);
	while(str[i] != EOS) {
		if((str[i] == '"') || (str[i] == '\\')) {
			theString = ExpandStringWithChar(theEnv, '\\', theString, &pos, &max, max+80);
			theString = ExpandStringWithChar(theEnv, str[i], theString, &pos, &max, max+80);
		}
		else {theString = ExpandStringWithChar(theEnv, str[i], theString, &pos, &max, max+80); }
		i++;
	}
	theString = ExpandStringWithChar(theEnv, '"', theString, &pos, &max, max+80);

	thePtr = EnvAddSymbol(theEnv, theString);
	rm(theEnv, theString, max);
	return ValueToString(thePtr);
}

/***********************************************************/
/* AppendStrings: Appends two strings together. The string */
/*   created is added to the SymbolTable, so it is not     */
/*   necessary to deallocate the string returned.          */
/***********************************************************/
globle char * AppendStrings(
	void * theEnv,
	char * str1,
	char * str2)
{
	int pos = 0;
	unsigned max = 0;
	char * theString = NULL;
	void * thePtr;

	theString = AppendToString(theEnv, str1, theString, &pos, &max);
	theString = AppendToString(theEnv, str2, theString, &pos, &max);

	thePtr = EnvAddSymbol(theEnv, theString);
	rm(theEnv, theString, max);
	return ValueToString(thePtr);
}

/******************************************************/
/* AppendToString: Appends a string to another string */
/*   (expanding the other string if necessary).       */
/******************************************************/
globle char * AppendToString(
	void * theEnv,
	char * appendStr,
	char * oldStr,
	int * oldPos,
	unsigned * oldMax)
{
	size_t length;

	/*=========================================*/
	/* Expand the old string so it can contain */
	/* the new string (if necessary).          */
	/*=========================================*/

	length = strlen(appendStr);
	if(length+*oldPos+1 > *oldMax) {
		oldStr = (char *)genrealloc(theEnv, oldStr, (unsigned)*oldMax, (unsigned)length+*oldPos+1);
		*oldMax = length+*oldPos+1;
	}
	/*==============================================================*/
	/* Return NULL if the old string was not successfully expanded. */
	/*==============================================================*/
	if(oldStr == NULL) {
		return NULL;
	}
	/*===============================================*/
	/* Append the new string to the expanded string. */
	/*===============================================*/

	strcpy(&oldStr[*oldPos], appendStr);
	*oldPos += (int)length;

	/*============================================================*/
	/* Return the expanded string containing the appended string. */
	/*============================================================*/

	return oldStr;
}

/*******************************************************/
/* AppendNToString: Appends a string to another string */
/*   (expanding the other string if necessary). Only a */
/*   specified number of characters are appended from  */
/*   the string.                                       */
/*******************************************************/
globle char * AppendNToString(
	void * theEnv,
	char * appendStr,
	char * oldStr,
	unsigned length,
	int * oldPos,
	unsigned * oldMax)
{
	unsigned lengthWithEOS;

	/*====================================*/
	/* Determine the number of characters */
	/* to be appended from the string.    */
	/*====================================*/
	if(appendStr[length-1] != '\0') lengthWithEOS = length+1;
	else lengthWithEOS = length;
	/*=========================================*/
	/* Expand the old string so it can contain */
	/* the new string (if necessary).          */
	/*=========================================*/
	if(lengthWithEOS+*oldPos > *oldMax) {
		oldStr = (char *)genrealloc(theEnv, oldStr, (unsigned)*oldMax, (unsigned)*oldPos+lengthWithEOS);
		*oldMax = (unsigned)*oldPos+lengthWithEOS;
	}
	/*==============================================================*/
	/* Return NULL if the old string was not successfully expanded. */
	/*==============================================================*/
	if(oldStr == NULL) {
		return NULL;
	}
	/*==================================*/
	/* Append N characters from the new */
	/* string to the expanded string.   */
	/*==================================*/

	strncpy(&oldStr[*oldPos], appendStr, (STD_SIZE)length);
	*oldPos += (int)(lengthWithEOS-1);
	oldStr[*oldPos] = '\0';

	/*============================================================*/
	/* Return the expanded string containing the appended string. */
	/*============================================================*/

	return oldStr;
}

/*******************************************************/
/* ExpandStringWithChar: Adds a character to a string, */
/*   reallocating space for the string if it needs to  */
/*   be enlarged. The backspace character causes the   */
/*   size of the string to reduced if it is "added" to */
/*   the string.                                       */
/*******************************************************/
globle char * ExpandStringWithChar(
	void * theEnv,
	int inchar,
	char * str,
	int * pos,
	unsigned * max,
	unsigned newSize)
{
	if((*pos+1) >= (int)*max) {
		str = (char *)genrealloc(theEnv, str, *max, newSize);
		*max = newSize;
	}
	if(inchar != '\b') {
		str[*pos] = (char)inchar;
		(*pos)++;
		str[*pos] = '\0';
	}
	else {
		if(*pos > 0) (*pos)--;
		str[*pos] = '\0';
	}
	return str;
}

/*****************************************************************/
/* AddFunctionToCallList: Adds a function to a list of functions */
/*   which are called to perform certain operations (e.g. clear, */
/*   reset, and bload functions).                                */
/*****************************************************************/
globle struct callFunctionItem * AddFunctionToCallList(
	void * theEnv,
	char * name,
	int priority,
	void (* func)(void *),
	struct callFunctionItem * head,
	intBool environmentAware)
{
	struct callFunctionItem * newPtr, * currentPtr, * lastPtr = NULL;

	newPtr = get_struct(theEnv, callFunctionItem);

	newPtr->name = name;
	newPtr->func = func;
	newPtr->priority = priority;
	newPtr->environmentAware = (short)environmentAware;
	if(head == NULL) {
		newPtr->next = NULL;
		return newPtr;
	}
	currentPtr = head;
	while((currentPtr != NULL) ? (priority < currentPtr->priority) : FALSE) {
		lastPtr = currentPtr;
		currentPtr = currentPtr->next;
	}
	if(lastPtr == NULL) {
		newPtr->next = head;
		head = newPtr;
	}
	else {
		newPtr->next = currentPtr;
		lastPtr->next = newPtr;
	}
	return head;
}

/*****************************************************************/
/* RemoveFunctionFromCallList: Removes a function from a list of */
/*   functions which are called to perform certain operations    */
/*   (e.g. clear, reset, and bload functions).                   */
/*****************************************************************/
globle struct callFunctionItem * RemoveFunctionFromCallList(
	void * theEnv,
	char * name,
	struct callFunctionItem * head,
	int * found)
{
	struct callFunctionItem * currentPtr, * lastPtr;

	*found = FALSE;
	lastPtr = NULL;
	currentPtr = head;

	while(currentPtr != NULL) {
		if(strcmp(name, currentPtr->name) == 0) {
			*found = TRUE;
			if(lastPtr == NULL) {
				head = currentPtr->next;
			}
			else {lastPtr->next = currentPtr->next; }
			rtn_struct(theEnv, callFunctionItem, currentPtr);
			return head;
		}
		lastPtr = currentPtr;
		currentPtr = currentPtr->next;
	}
	return head;
}

/**************************************************************/
/* DeallocateCallList: Removes all functions from a list of   */
/*   functions which are called to perform certain operations */
/*   (e.g. clear, reset, and bload functions).                */
/**************************************************************/
globle void DeallocateCallList(
	void * theEnv,
	struct callFunctionItem * theList)
{
	struct callFunctionItem * tmpPtr, * nextPtr;

	tmpPtr = theList;
	while(tmpPtr != NULL) {
		nextPtr = tmpPtr->next;
		rtn_struct(theEnv, callFunctionItem, tmpPtr);
		tmpPtr = nextPtr;
	}
}

/*****************************************/
/* ItemHashValue: Returns the hash value */
/*   for the specified value.            */
/*****************************************/
globle unsigned ItemHashValue(
	void * theEnv,
	unsigned short theType,
	void * theValue,
	unsigned theRange)
{
	switch(theType) {
	    case FLOAT:
		return HashFloat(ValueToDouble(theValue), theRange);

	    case INTEGER:
		return HashInteger(ValueToLong(theValue), theRange);

	    case SYMBOL:
	    case STRING:
#if OBJECT_SYSTEM
	    case INSTANCE_NAME:
#endif
		return HashSymbol(ValueToString(theValue), theRange);

	    case MULTIFIELD:
		return HashMultifield((struct multifield *)theValue, theRange);

#if DEFTEMPLATE_CONSTRUCT
	    case FACT_ADDRESS:
		return HashFact((struct fact *)theValue)%theRange;
#endif

	    case EXTERNAL_ADDRESS:
#if OBJECT_SYSTEM
	    case INSTANCE_ADDRESS:
#endif
		return ((unsigned)theValue)%theRange;
	}
	SystemError(theEnv, "UTILITY", 1);
	return 0;
}

/********************************************/
/* YieldTime: Yields time to a user-defined */
/*   function. Intended to allow foreground */
/*   application responsiveness when CLIPS  */
/*   is running in the background.          */
/********************************************/
void YieldTime(
	void * theEnv)
{
	if((UtilityData(theEnv)->YieldTimeFunction != NULL) && UtilityData(theEnv)->YieldFunctionEnabled) {
		(*UtilityData(theEnv)->YieldTimeFunction)();
	}
}

/********************************************/
/* SetGarbageCollectionHeuristics:         */
/********************************************/
short SetGarbageCollectionHeuristics(
	void * theEnv,
	short newValue)
{
	short oldValue;

	oldValue = UtilityData(theEnv)->GarbageCollectionHeuristicsEnabled;

	UtilityData(theEnv)->GarbageCollectionHeuristicsEnabled = newValue;

	return oldValue;
}

/**********************************************/
/* EnvIncrementGCLocks: Increments the number */
/*   of garbage collection locks.             */
/**********************************************/
globle void EnvIncrementGCLocks(
	void * theEnv)
{
	UtilityData(theEnv)->GarbageCollectionLocks++;
}

/**********************************************/
/* EnvDecrementGCLocks: Decrements the number */
/*   of garbage collection locks.             */
/**********************************************/
globle void EnvDecrementGCLocks(
	void * theEnv)
{
	if(UtilityData(theEnv)->GarbageCollectionLocks > 0) {
		UtilityData(theEnv)->GarbageCollectionLocks--;
	}
}

/********************************************/
/* EnablePeriodicFunctions:         */
/********************************************/
short EnablePeriodicFunctions(
	void * theEnv,
	short value)
{
	short oldValue;

	oldValue = UtilityData(theEnv)->PeriodicFunctionsEnabled;

	UtilityData(theEnv)->PeriodicFunctionsEnabled = value;

	return oldValue;
}

/********************************************/
/* EnableYieldFunction:         */
/********************************************/
short EnableYieldFunction(
	void * theEnv,
	short value)
{
	short oldValue;

	oldValue = UtilityData(theEnv)->YieldFunctionEnabled;

	UtilityData(theEnv)->YieldFunctionEnabled = value;

	return oldValue;
}

