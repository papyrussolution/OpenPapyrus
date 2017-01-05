   /*******************************************************/
   /*      "C" Language Integrated Production System      */
   /*                                                     */
   /*             CLIPS Version 6.24  06/05/06            */
   /*                                                     */
   /*                   DEFRULE MODULE                    */
   /*******************************************************/

/*************************************************************/
/* Purpose: Defines basic defrule primitive functions such   */
/*   as allocating and deallocating, traversing, and finding */
/*   defrule data structures.                                */
/*                                                           */
/* Principal Programmer(s):                                  */
/*      Gary D. Riley                                        */
/*                                                           */
/* Contributing Programmer(s):                               */
/*      Brian L. Donnell                                     */
/*                                                           */
/* Revision History:                                         */
/*      6.24: Removed CONFLICT_RESOLUTION_STRATEGIES         */
/*            compilation flag.                              */
/*                                                           */
/*            Renamed BOOLEAN macro type to intBool.         */
/*                                                           */
/*            Corrected code to remove run-time program      */
/*            compiler warnings.                             */
/*                                                           */
/*************************************************************/

#define _RULEDEF_SOURCE_

#include "setup.h"

#if DEFRULE_CONSTRUCT

#include <stdio.h>
#define _STDIO_INCLUDED_

#include "agenda.h"
#include "drive.h"
#include "engine.h"
#include "envrnmnt.h"
#include "memalloc.h"
#include "pattern.h"
#include "retract.h"
#include "rulebsc.h"
#include "rulecom.h"
#include "rulepsr.h"
#include "ruledlt.h"

#if BLOAD || BLOAD_AND_BSAVE || BLOAD_ONLY
#include "bload.h"
#include "rulebin.h"
#endif

#if CONSTRUCT_COMPILER && (! RUN_TIME)
#include "rulecmp.h"
#endif

#include "ruledef.h"

/***************************************/
/* LOCAL INTERNAL FUNCTION DEFINITIONS */
/***************************************/

   static void                   *AllocateModule(void *);
   static void                    ReturnModule(void *,void *);
   static void                    InitializeDefruleModules(void *);
   static void                    DeallocateDefruleData(void *);
   static void                    DestroyDefruleAction(void *,struct constructHeader *,void *);

/**********************************************************/
/* InitializeDefrules: Initializes the defrule construct. */
/**********************************************************/
globle void InitializeDefrules(
  void *theEnv)
  {   
   AllocateEnvironmentData(theEnv,DEFRULE_DATA,sizeof(struct defruleData),DeallocateDefruleData);

   InitializeEngine(theEnv);
   InitializeAgenda(theEnv);
   InitializePatterns(theEnv);
   InitializeDefruleModules(theEnv);

   AddReservedPatternSymbol(theEnv,"and",NULL);
   AddReservedPatternSymbol(theEnv,"not",NULL);
   AddReservedPatternSymbol(theEnv,"or",NULL);
   AddReservedPatternSymbol(theEnv,"test",NULL);
   AddReservedPatternSymbol(theEnv,"logical",NULL);
   AddReservedPatternSymbol(theEnv,"exists",NULL);
   AddReservedPatternSymbol(theEnv,"forall",NULL);

   DefruleBasicCommands(theEnv);

   DefruleCommands(theEnv);

   DefruleData(theEnv)->DefruleConstruct =
      AddConstruct(theEnv,"defrule","defrules",
                   ParseDefrule,EnvFindDefrule,
                   GetConstructNamePointer,GetConstructPPForm,
                   GetConstructModuleItem,EnvGetNextDefrule,SetNextConstruct,
                   EnvIsDefruleDeletable,EnvUndefrule,ReturnDefrule);
  }
  
/***************************************************/
/* DeallocateDefruleData: Deallocates environment */
/*    data for the deffacts construct.             */
/***************************************************/
static void DeallocateDefruleData(
  void *theEnv)
  {
   struct defruleModule *theModuleItem;
   void *theModule;
   struct activation *theActivation, *tmpActivation;

#if BLOAD || BLOAD_AND_BSAVE
   if (Bloaded(theEnv)) return;
#endif
   
   DoForAllConstructs(theEnv,DestroyDefruleAction,DefruleData(theEnv)->DefruleModuleIndex,FALSE,NULL);

   for (theModule = EnvGetNextDefmodule(theEnv,NULL);
        theModule != NULL;
        theModule = EnvGetNextDefmodule(theEnv,theModule))
     {
      theModuleItem = (struct defruleModule *)
                      GetModuleItem(theEnv,(struct defmodule *) theModule,
                                    DefruleData(theEnv)->DefruleModuleIndex);
                                    
      theActivation = theModuleItem->agenda;
      while (theActivation != NULL)
        {
         tmpActivation = theActivation->next;
         
         if (theActivation->sortedBasis != NULL)
           { DestroyPartialMatch(theEnv,theActivation->sortedBasis); }

         rtn_struct(theEnv,activation,theActivation);
         
         theActivation = tmpActivation;
        }

#if ! RUN_TIME                                    
      rtn_struct(theEnv,defruleModule,theModuleItem);
#endif
     }
  }
  
/********************************************************/
/* DestroyDefruleAction: Action used to remove defrules */
/*   as a result of DestroyEnvironment.                 */
/********************************************************/
#if IBM_TBC
#pragma argsused
#endif
static void DestroyDefruleAction(
  void *theEnv,
  struct constructHeader *theConstruct,
  void *buffer)
  {
#if MAC_MCW || IBM_MCW || MAC_XCD
#pragma unused(buffer)
#endif
   struct defrule *theDefrule = (struct defrule *) theConstruct;
   
   DestroyDefrule(theEnv,theDefrule);
  }

/*****************************************************/
/* InitializeDefruleModules: Initializes the defrule */
/*   construct for use with the defmodule construct. */
/*****************************************************/
static void InitializeDefruleModules(
  void *theEnv)
  {
   DefruleData(theEnv)->DefruleModuleIndex = RegisterModuleItem(theEnv,"defrule",
                                    AllocateModule,
                                    ReturnModule,
#if BLOAD_AND_BSAVE || BLOAD || BLOAD_ONLY
                                    BloadDefruleModuleReference,
#else
                                    NULL,
#endif
#if CONSTRUCT_COMPILER && (! RUN_TIME)
                                    DefruleCModuleReference,
#else
                                    NULL,
#endif
                                    EnvFindDefrule);
  }

/***********************************************/
/* AllocateModule: Allocates a defrule module. */
/***********************************************/
static void *AllocateModule(
  void *theEnv)
  {
   struct defruleModule *theItem;

   theItem = get_struct(theEnv,defruleModule);
   theItem->agenda = NULL;
   return((void *) theItem);
  }

/*********************************************/
/* ReturnModule: Deallocates a defrule module. */
/*********************************************/
static void ReturnModule(
  void *theEnv,
  void *theItem)
  {
   FreeConstructHeaderModule(theEnv,(struct defmoduleItemHeader *) theItem,DefruleData(theEnv)->DefruleConstruct);
   rtn_struct(theEnv,defruleModule,theItem);
  }

/************************************************************/
/* GetDefruleModuleItem: Returns a pointer to the defmodule */
/*  item for the specified defrule or defmodule.            */
/************************************************************/
globle struct defruleModule *GetDefruleModuleItem(
  void *theEnv,
  struct defmodule *theModule)
  {   
   return((struct defruleModule *) GetConstructModuleItemByIndex(theEnv,theModule,DefruleData(theEnv)->DefruleModuleIndex)); 
  }

/*******************************************************************/
/* EnvFindDefrule: Searches for a defrule in the list of defrules. */
/*   Returns a pointer to the defrule if found, otherwise NULL.    */
/*******************************************************************/
globle void *EnvFindDefrule(
  void *theEnv,
  char *defruleName)
  {   
   return(FindNamedConstruct(theEnv,defruleName,DefruleData(theEnv)->DefruleConstruct)); 
  }

/************************************************************/
/* EnvGetNextDefrule: If passed a NULL pointer, returns the */
/*   first defrule in the ListOfDefrules. Otherwise returns */
/*   the next defrule following the defrule passed as an    */
/*   argument.                                              */
/************************************************************/
globle void *EnvGetNextDefrule(
  void *theEnv,
  void *defrulePtr)
  {   
   return((void *) GetNextConstructItem(theEnv,(struct constructHeader *) defrulePtr,DefruleData(theEnv)->DefruleModuleIndex)); 
  }

/*******************************************************/
/* EnvIsDefruleDeletable: Returns TRUE if a particular */
/*   defrule can be deleted, otherwise returns FALSE.  */
/*******************************************************/
globle intBool EnvIsDefruleDeletable(
  void *theEnv,
  void *vTheDefrule)
  {
   struct defrule *theDefrule;

   if (! ConstructsDeletable(theEnv))
     { return FALSE; }

   for (theDefrule = (struct defrule *) vTheDefrule;
        theDefrule != NULL;
        theDefrule = theDefrule->disjunct)
     { if (theDefrule->executing) return(FALSE); }

   if (EngineData(theEnv)->JoinOperationInProgress) return(FALSE);

   return(TRUE);
  }

#endif /* DEFRULE_CONSTRUCT */


