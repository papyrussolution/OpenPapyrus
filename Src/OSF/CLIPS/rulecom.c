/*******************************************************/
/*      "C" Language Integrated Production System      */
/*                                                     */
/*             CLIPS Version 6.24  05/17/06            */
/*                                                     */
/*                RULE COMMANDS MODULE                 */
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
/*                                                           */
/* Revision History:                                         */
/*                                                           */
/*      6.24: Removed CONFLICT_RESOLUTION_STRATEGIES         */
/*            INCREMENTAL_RESET, and LOGICAL_DEPENDENCIES    */
/*            compilation flags.                             */
/*                                                           */
/*            Renamed BOOLEAN macro type to intBool.         */
/*                                                           */
/*************************************************************/

#define _RULECOM_SOURCE_

#include <stdio.h>
#define _STDIO_INCLUDED_
#include <string.h>

#include "setup.h"

#if DEFRULE_CONSTRUCT

 #include "argacces.h"
 #include "constant.h"
 #include "constrct.h"
 #include "crstrtgy.h"
 #include "engine.h"
 #include "envrnmnt.h"
 #include "evaluatn.h"
 #include "extnfunc.h"
 #include "incrrset.h"
 #include "lgcldpnd.h"
 #include "memalloc.h"
 #include "pattern.h"
 #include "reteutil.h"
 #include "router.h"
 #include "ruledlt.h"
 #include "watch.h"

 #if BLOAD || BLOAD_AND_BSAVE || BLOAD_ONLY
  #include "rulebin.h"
 #endif

 #include "rulecom.h"

/***************************************/
/* LOCAL INTERNAL FUNCTION DEFINITIONS */
/***************************************/

 #if DEVELOPER
static void                    ShowJoins(void *, void *);
 #endif

/****************************************************************/
/* DefruleCommands: Initializes defrule commands and functions. */
/****************************************************************/
globle void DefruleCommands(
	void * theEnv)
{
 #if !RUN_TIME
	EnvDefineFunction2(theEnv, "run", 'v', PTIEF RunCommand, "RunCommand", "*1i");
	EnvDefineFunction2(theEnv, "halt", 'v', PTIEF HaltCommand, "HaltCommand", "00");
	EnvDefineFunction2(theEnv, "focus", 'b', PTIEF FocusCommand, "FocusCommand", "1*w");
	EnvDefineFunction2(theEnv, "clear-focus-stack", 'v', PTIEF ClearFocusStackCommand,
		"ClearFocusStackCommand", "00");
	EnvDefineFunction2(theEnv, "get-focus-stack", 'm', PTIEF GetFocusStackFunction,
		"GetFocusStackFunction", "00");
	EnvDefineFunction2(theEnv, "pop-focus", 'w', PTIEF PopFocusFunction,
		"PopFocusFunction", "00");
	EnvDefineFunction2(theEnv, "get-focus", 'w', PTIEF GetFocusFunction,
		"GetFocusFunction", "00");
  #if DEBUGGING_FUNCTIONS
	EnvDefineFunction2(theEnv, "set-break", 'v', PTIEF SetBreakCommand,
		"SetBreakCommand", "11w");
	EnvDefineFunction2(theEnv, "remove-break", 'v', PTIEF RemoveBreakCommand,
		"RemoveBreakCommand", "*1w");
	EnvDefineFunction2(theEnv, "show-breaks", 'v', PTIEF ShowBreaksCommand,
		"ShowBreaksCommand", "01w");
	EnvDefineFunction2(theEnv, "matches", 'v', PTIEF MatchesCommand, "MatchesCommand", "11w");
	EnvDefineFunction2(theEnv, "list-focus-stack", 'v', PTIEF ListFocusStackCommand,
		"ListFocusStackCommand", "00");
	EnvDefineFunction2(theEnv, "dependencies", 'v', PTIEF DependenciesCommand,
		"DependenciesCommand", "11h");
	EnvDefineFunction2(theEnv, "dependents",   'v', PTIEF DependentsCommand,
		"DependentsCommand", "11h");
  #endif /* DEBUGGING_FUNCTIONS */

	EnvDefineFunction2(theEnv, "get-incremental-reset", 'b',
		GetIncrementalResetCommand, "GetIncrementalResetCommand", "00");
	EnvDefineFunction2(theEnv, "set-incremental-reset", 'b',
		SetIncrementalResetCommand, "SetIncrementalResetCommand", "11");

	EnvDefineFunction2(theEnv, "get-strategy", 'w', PTIEF GetStrategyCommand,  "GetStrategyCommand", "00");
	EnvDefineFunction2(theEnv, "set-strategy", 'w', PTIEF SetStrategyCommand,  "SetStrategyCommand", "11w");

  #if DEVELOPER && (!BLOAD_ONLY)
	EnvDefineFunction2(theEnv, "rule-complexity", 'l', PTIEF RuleComplexityCommand, "RuleComplexityCommand", "11w");
	EnvDefineFunction2(theEnv, "show-joins",   'v', PTIEF ShowJoinsCommand,    "ShowJoinsCommand", "11w");
   #if DEBUGGING_FUNCTIONS
	//AddWatchItem(theEnv,"rule-analysis",0,&DefruleData(theEnv)->WatchRuleAnalysis,0,NULL,NULL);
   #endif
  #endif /* DEVELOPER && (! BLOAD_ONLY) */

 #else
  #if MAC_MCW || IBM_MCW || MAC_XCD
   #pragma unused(theEnv)
  #endif
 #endif /* ! RUN_TIME */
}

 #if DEBUGGING_FUNCTIONS

/****************************************/
/* MatchesCommand: H/L access routine   */
/*   for the matches command.           */
/****************************************/
globle void MatchesCommand(
	void * theEnv)
{
	char * ruleName;
	void * rulePtr;

	ruleName = GetConstructName(theEnv, "matches", "rule name");
	if(ruleName == NULL) return;
	rulePtr = EnvFindDefrule(theEnv, ruleName);
	if(rulePtr == NULL) {
		CantFindItemErrorMessage(theEnv, "defrule", ruleName);
		return;
	}
	EnvMatches(theEnv, rulePtr);
}

/********************************/
/* EnvMatches: C access routine */
/*   for the matches command.   */
/********************************/
globle intBool EnvMatches(
	void * theEnv,
	void * theRule)
{
	struct defrule * rulePtr, * tmpPtr;
	struct partialMatch * listOfMatches, ** theStorage;
	struct joinNode * theJoin, * lastJoin;
	int i, depth;
	ACTIVATION * agendaPtr;
	int flag;
	int matchesDisplayed;

	/*=================================================*/
	/* Loop through each of the disjuncts for the rule */
	/*=================================================*/

	for(rulePtr = (struct defrule *)theRule, tmpPtr = rulePtr;
	    rulePtr != NULL;
	    rulePtr = rulePtr->disjunct) {
		/*======================================*/
		/* Determine the last join in the rule. */
		/*======================================*/

		lastJoin = rulePtr->lastJoin;

		/*===================================*/
		/* Determine the number of patterns. */
		/*===================================*/

		depth = GetPatternNumberFromJoin(lastJoin);

		/*=========================================*/
		/* Store the alpha memory partial matches. */
		/*=========================================*/

		theStorage = (struct partialMatch **)
		             genalloc(theEnv, (unsigned)(depth*sizeof(struct partialMatch)));

		theJoin = lastJoin;
		i = depth-1;
		while(theJoin != NULL) {
			if(theJoin->joinFromTheRight) {
				theJoin = (struct joinNode *)theJoin->rightSideEntryStructure;
			}
			else {
				theStorage[i] =
				        ((struct patternNodeHeader *)theJoin->rightSideEntryStructure)->alphaMemory;
				i--;
				theJoin = theJoin->lastLevel;
			}
		}
		/*========================================*/
		/* List the alpha memory partial matches. */
		/*========================================*/

		for(i = 0; i < depth; i++) {
			if(GetHaltExecution(theEnv) == TRUE) {
				genfree(theEnv, theStorage, (unsigned)(depth*sizeof(struct partialMatch)));
				return TRUE;
			}
			EnvPrintRouter(theEnv, WDISPLAY, "Matches for Pattern ");
			PrintLongInteger(theEnv, WDISPLAY, (long int)i+1);
			EnvPrintRouter(theEnv, WDISPLAY, "\n");

			listOfMatches = theStorage[i];
			if(listOfMatches == NULL) EnvPrintRouter(theEnv, WDISPLAY, " None\n");
			while(listOfMatches != NULL) {
				if(GetHaltExecution(theEnv) == TRUE) {
					genfree(theEnv, theStorage, (unsigned)(depth*sizeof(struct partialMatch)));
					return TRUE;
				}
				PrintPartialMatch(theEnv, WDISPLAY, listOfMatches);
				EnvPrintRouter(theEnv, WDISPLAY, "\n");
				listOfMatches = listOfMatches->next;
			}
		}
		genfree(theEnv, theStorage, (unsigned)(depth*sizeof(struct partialMatch)));

		/*========================================*/
		/* Store the beta memory partial matches. */
		/*========================================*/

		depth = lastJoin->depth;
		theStorage = (struct partialMatch **)genalloc(theEnv, (unsigned)(depth*sizeof(struct partialMatch)));

		theJoin = lastJoin;
		for(i = depth-1; i >= 0; i--) {
			theStorage[i] = theJoin->beta;
			theJoin = theJoin->lastLevel;
		}
		/*=======================================*/
		/* List the beta memory partial matches. */
		/*=======================================*/

		for(i = 1; i < depth; i++) {
			if(GetHaltExecution(theEnv) == TRUE) {
				genfree(theEnv, theStorage, (unsigned)(depth*sizeof(struct partialMatch)));
				return TRUE;
			}
			matchesDisplayed = 0;
			EnvPrintRouter(theEnv, WDISPLAY, "Partial matches for CEs 1 - ");
			PrintLongInteger(theEnv, WDISPLAY, (long int)i+1);
			EnvPrintRouter(theEnv, WDISPLAY, "\n");
			listOfMatches = theStorage[i];

			while(listOfMatches != NULL) {
				if(GetHaltExecution(theEnv) == TRUE) {
					genfree(theEnv, theStorage, (unsigned)(depth*sizeof(struct partialMatch)));
					return TRUE;
				}
				if(listOfMatches->counterf == FALSE) {
					matchesDisplayed++;
					PrintPartialMatch(theEnv, WDISPLAY, listOfMatches);
					EnvPrintRouter(theEnv, WDISPLAY, "\n");
				}
				listOfMatches = listOfMatches->next;
			}
			if(matchesDisplayed == 0) {
				EnvPrintRouter(theEnv, WDISPLAY, " None\n");
			}
		}
		genfree(theEnv, theStorage, (unsigned)(depth*sizeof(struct partialMatch)));
	}
	/*===================*/
	/* List activations. */
	/*===================*/

	rulePtr = tmpPtr;
	EnvPrintRouter(theEnv, WDISPLAY, "Activations\n");
	flag = 1;
	for(agendaPtr = (struct activation *)EnvGetNextActivation(theEnv, NULL);
	    agendaPtr != NULL;
	    agendaPtr = (struct activation *)EnvGetNextActivation(theEnv, agendaPtr)) {
		if(GetHaltExecution(theEnv) == TRUE) return TRUE;
		if(((struct activation *)agendaPtr)->theRule->header.name == rulePtr->header.name) {
			flag = 0;
			PrintPartialMatch(theEnv, WDISPLAY, GetActivationBasis(agendaPtr));
			EnvPrintRouter(theEnv, WDISPLAY, "\n");
		}
	}
	if(flag) EnvPrintRouter(theEnv, WDISPLAY, " None\n");
	return TRUE;
}

 #endif /* DEBUGGING_FUNCTIONS */

 #if DEVELOPER
/***********************************************/
/* RuleComplexityCommand: H/L access routine   */
/*   for the rule-complexity function.         */
/***********************************************/
globle long RuleComplexityCommand(
	void * theEnv)
{
	char * ruleName;
	struct defrule * rulePtr;

	ruleName = GetConstructName(theEnv, "rule-complexity", "rule name");
	if(ruleName == NULL) return -1;
	rulePtr = (struct defrule *)EnvFindDefrule(theEnv, ruleName);
	if(rulePtr == NULL) {
		CantFindItemErrorMessage(theEnv, "defrule", ruleName);
		return -1;
	}
	return rulePtr->complexity;
}

/******************************************/
/* ShowJoinsCommand: H/L access routine   */
/*   for the show-joins command.          */
/******************************************/
globle void ShowJoinsCommand(void * theEnv)
{
	void * rulePtr;
	char * ruleName = GetConstructName(theEnv, "show-joins", "rule name");
	if(ruleName == NULL) 
		return;
	rulePtr = EnvFindDefrule(theEnv, ruleName);
	if(rulePtr == NULL) {
		CantFindItemErrorMessage(theEnv, "defrule", ruleName);
		return;
	}
	ShowJoins(theEnv, rulePtr);
}

/*********************************/
/* ShowJoins: C access routine   */
/*   for the show-joins command. */
/*********************************/
static void ShowJoins(
	void * theEnv,
	void * theRule)
{
	struct defrule * rulePtr;
	struct joinNode * theJoin;
	struct joinNode * joinList[MAXIMUM_NUMBER_OF_PATTERNS];
	int numberOfJoins;

	rulePtr = (struct defrule *)theRule;

	/*=================================================*/
	/* Loop through each of the disjuncts for the rule */
	/*=================================================*/

	while(rulePtr != NULL) {
		/*=====================================*/
		/* Determine the number of join nodes. */
		/*=====================================*/

		numberOfJoins = -1;
		theJoin = rulePtr->lastJoin;
		while(theJoin != NULL) {
			if(theJoin->joinFromTheRight) {
				theJoin = (struct joinNode *)theJoin->rightSideEntryStructure;
			}
			else {
				numberOfJoins++;
				joinList[numberOfJoins] = theJoin;
				theJoin = theJoin->lastLevel;
			}
		}
		/*====================*/
		/* Display the joins. */
		/*====================*/

		while(numberOfJoins >= 0) {
			char buffer[20];
			sprintf(buffer, "%2d%c%c: ", (int)joinList[numberOfJoins]->depth,
				(joinList[numberOfJoins]->patternIsNegated) ? 'n' : ' ',
				(joinList[numberOfJoins]->logicalJoin) ? 'l' : ' ');
			EnvPrintRouter(theEnv, WDISPLAY, buffer);
			PrintExpression(theEnv, WDISPLAY, joinList[numberOfJoins]->networkTest);
			EnvPrintRouter(theEnv, WDISPLAY, "\n");
			numberOfJoins--;
		}
		;

		/*===============================*/
		/* Proceed to the next disjunct. */
		/*===============================*/

		rulePtr = rulePtr->disjunct;
		if(rulePtr != NULL) EnvPrintRouter(theEnv, WDISPLAY, "\n");
	}
}

 #endif /* DEVELOPER */

#endif /* DEFRULE_CONSTRUCT */

