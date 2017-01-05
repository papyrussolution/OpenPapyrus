   /*******************************************************/
   /*      "C" Language Integrated Production System      */
   /*                                                     */
   /*             CLIPS Version 6.23  01/31/05            */
   /*                                                     */
   /*              DEFTEMPLATE PARSER MODULE              */
   /*******************************************************/

/*************************************************************/
/* Purpose: Parses the deftemplate construct.                */
/*                                                           */
/* Principal Programmer(s):                                  */
/*      Gary D. Riley                                        */
/*                                                           */
/* Contributing Programmer(s):                               */
/*                                                           */
/* Revision History:                                         */
/*      6.23: Added support for templates maintaining their  */
/*            own list of facts.                             */
/*                                                           */
/*************************************************************/

#define _TMPLTPSR_SOURCE_

#include "setup.h"

#if DEFTEMPLATE_CONSTRUCT

#include <stdio.h>
#define _STDIO_INCLUDED_
#include <string.h>

#include "constant.h"
#include "memalloc.h"
#include "symbol.h"
#include "scanner.h"
#include "exprnpsr.h"
#include "router.h"
#include "constrct.h"
#include "envrnmnt.h"
#include "factmngr.h"
#include "cstrnchk.h"
#include "cstrnpsr.h"
#include "cstrcpsr.h"
#if BLOAD || BLOAD_AND_BSAVE
#include "bload.h"
#endif
#include "default.h"
#include "pattern.h"
#include "watch.h"
#include "cstrnutl.h"

#include "tmpltdef.h"
#include "tmpltbsc.h"

#include "tmpltpsr.h"

/***************************************/
/* LOCAL INTERNAL FUNCTION DEFINITIONS */
/***************************************/

#if (! RUN_TIME) && (! BLOAD_ONLY)
   static struct templateSlot    *SlotDeclarations(void *,char *,struct token *);
   static struct templateSlot    *ParseSlot(void *,char *,struct token *,struct templateSlot *);
   static struct templateSlot    *DefinedSlots(void *,char *,SYMBOL_HN *,int,struct token *);
#endif

/*******************************************************/
/* ParseDeftemplate: Parses the deftemplate construct. */
/*******************************************************/
globle int ParseDeftemplate(
  void *theEnv,
  char *readSource)
  {
#if (MAC_MCW || IBM_MCW) && (RUN_TIME || BLOAD_ONLY)
#pragma unused(readSource)
#endif

#if (! RUN_TIME) && (! BLOAD_ONLY)
   SYMBOL_HN *deftemplateName;
   struct deftemplate *newDeftemplate;
   struct templateSlot *slots;
   struct token inputToken;

   /*================================================*/
   /* Initialize pretty print and error information. */
   /*================================================*/

   DeftemplateData(theEnv)->DeftemplateError = FALSE;
   SetPPBufferStatus(theEnv,ON);
   FlushPPBuffer(theEnv);
   SavePPBuffer(theEnv,"(deftemplate ");

   /*==============================================================*/
   /* Deftemplates can not be added when a binary image is loaded. */
   /*==============================================================*/

#if BLOAD || BLOAD_AND_BSAVE
   if ((Bloaded(theEnv) == TRUE) && (! ConstructData(theEnv)->CheckSyntaxMode))
     {
      CannotLoadWithBloadMessage(theEnv,"deftemplate");
      return(TRUE);
     }
#endif

   /*=======================================================*/
   /* Parse the name and comment fields of the deftemplate. */
   /*=======================================================*/

#if DEBUGGING_FUNCTIONS
   DeftemplateData(theEnv)->DeletedTemplateDebugFlags = 0;
#endif

   deftemplateName = GetConstructNameAndComment(theEnv,readSource,&inputToken,"deftemplate",
                                                EnvFindDeftemplate,EnvUndeftemplate,"%",
                                                TRUE,TRUE,TRUE);
   if (deftemplateName == NULL) return(TRUE);

   if (ReservedPatternSymbol(theEnv,ValueToString(deftemplateName),"deftemplate"))
     {
      ReservedPatternSymbolErrorMsg(theEnv,ValueToString(deftemplateName),"a deftemplate name");
      return(TRUE);
     }

   /*===========================================*/
   /* Parse the slot fields of the deftemplate. */
   /*===========================================*/

   slots = SlotDeclarations(theEnv,readSource,&inputToken);
   if (DeftemplateData(theEnv)->DeftemplateError == TRUE) return(TRUE);

   /*==============================================*/
   /* If we're only checking syntax, don't add the */
   /* successfully parsed deftemplate to the KB.   */
   /*==============================================*/

   if (ConstructData(theEnv)->CheckSyntaxMode)
     {
      ReturnSlots(theEnv,slots);
      return(FALSE);
     }

   /*=====================================*/
   /* Create a new deftemplate structure. */
   /*=====================================*/

   newDeftemplate = get_struct(theEnv,deftemplate);
   newDeftemplate->header.name =  deftemplateName;
   newDeftemplate->header.next = NULL;
   newDeftemplate->header.usrData = NULL;
   newDeftemplate->slotList = slots;
   newDeftemplate->implied = FALSE;
   newDeftemplate->numberOfSlots = 0;
   newDeftemplate->busyCount = 0;
   newDeftemplate->watch = 0;
   newDeftemplate->inScope = TRUE;
   newDeftemplate->patternNetwork = NULL;
   newDeftemplate->factList = NULL;
   newDeftemplate->lastFact = NULL;
   newDeftemplate->header.whichModule = (struct defmoduleItemHeader *)
                                        GetModuleItem(theEnv,NULL,DeftemplateData(theEnv)->DeftemplateModuleIndex);

   /*================================*/
   /* Determine the number of slots. */
   /*================================*/

   while (slots != NULL)
     {
      newDeftemplate->numberOfSlots++;
      slots = slots->next;
     }

   /*====================================*/
   /* Store pretty print representation. */
   /*====================================*/

   if (EnvGetConserveMemory(theEnv) == TRUE)
     { newDeftemplate->header.ppForm = NULL; }
   else
     { newDeftemplate->header.ppForm = CopyPPBuffer(theEnv); }

   /*=======================================================================*/
   /* If a template is redefined, then we want to restore its watch status. */
   /*=======================================================================*/

#if DEBUGGING_FUNCTIONS
   if ((BitwiseTest(DeftemplateData(theEnv)->DeletedTemplateDebugFlags,0)) || EnvGetWatchItem(theEnv,"facts"))
     { EnvSetDeftemplateWatch(theEnv,ON,(void *) newDeftemplate); }
#endif

   /*==============================================*/
   /* Add deftemplate to the list of deftemplates. */
   /*==============================================*/

   AddConstructToModule(&newDeftemplate->header);

   InstallDeftemplate(theEnv,newDeftemplate);

#else
#if MAC_MCW || IBM_MCW || MAC_XCD
#pragma unused(theEnv)
#endif
#endif

   return(FALSE);
  }

#if (! RUN_TIME) && (! BLOAD_ONLY)

/**************************************************************/
/* InstallDeftemplate: Increments all occurrences in the hash */
/*   table of symbols found in an deftemplate and adds it to  */
/*   the hash table.                                          */
/**************************************************************/
globle void InstallDeftemplate(
  void *theEnv,
  struct deftemplate *theDeftemplate)
  {
   struct templateSlot *slotPtr;
   struct expr *tempExpr;

   IncrementSymbolCount(theDeftemplate->header.name);

   for (slotPtr = theDeftemplate->slotList;
        slotPtr != NULL;
        slotPtr = slotPtr->next)
     {
      IncrementSymbolCount(slotPtr->slotName);
      tempExpr = AddHashedExpression(theEnv,slotPtr->defaultList);
      ReturnExpression(theEnv,slotPtr->defaultList);
      slotPtr->defaultList = tempExpr;
      slotPtr->constraints = AddConstraint(theEnv,slotPtr->constraints);
     }
  }

/********************************************************************/
/* SlotDeclarations: Parses the slot declarations of a deftemplate. */
/********************************************************************/
static struct templateSlot *SlotDeclarations(
  void *theEnv,
  char *readSource,
  struct token *inputToken)
  {
   struct templateSlot *newSlot, *slotList = NULL, *lastSlot = NULL;
   struct templateSlot *multiSlot = NULL;

   while (inputToken->type != RPAREN)
     {
      /*====================================================*/
      /* Slots begin with a '(' followed by a slot keyword. */
      /*====================================================*/

      if (inputToken->type != LPAREN)
        {
         SyntaxErrorMessage(theEnv,"deftemplate");
         ReturnSlots(theEnv,slotList);
         ReturnSlots(theEnv,multiSlot);
         DeftemplateData(theEnv)->DeftemplateError = TRUE;
         return(NULL);
        }

      GetToken(theEnv,readSource,inputToken);
      if (inputToken->type != SYMBOL)
        {
         SyntaxErrorMessage(theEnv,"deftemplate");
         ReturnSlots(theEnv,slotList);
         ReturnSlots(theEnv,multiSlot);
         DeftemplateData(theEnv)->DeftemplateError = TRUE;
         return(NULL);
        }

      /*=================*/
      /* Parse the slot. */
      /*=================*/

      newSlot = ParseSlot(theEnv,readSource,inputToken,slotList);
      if (DeftemplateData(theEnv)->DeftemplateError == TRUE)
        {
         ReturnSlots(theEnv,newSlot);
         ReturnSlots(theEnv,slotList);
         ReturnSlots(theEnv,multiSlot);
         return(NULL);
        }

      /*===========================================*/
      /* Attach the new slot to the list of slots. */
      /*===========================================*/

      if (newSlot != NULL)
        {
         if (lastSlot == NULL)
           { slotList = newSlot; }
         else
           { lastSlot->next = newSlot; }
         lastSlot = newSlot;
        }

      /*================================*/
      /* Check for closing parenthesis. */
      /*================================*/

      GetToken(theEnv,readSource,inputToken);
      if (inputToken->type != RPAREN)
        {
         PPBackup(theEnv);
         SavePPBuffer(theEnv,"\n   ");
         SavePPBuffer(theEnv,inputToken->printForm);
        }
     }

  SavePPBuffer(theEnv,"\n");

  /*=======================*/
  /* Return the slot list. */
  /*=======================*/

  return(slotList);
 }

/*****************************************************/
/* ParseSlot: Parses a single slot of a deftemplate. */
/*****************************************************/
static struct templateSlot *ParseSlot(
  void *theEnv,
  char *readSource,
  struct token *inputToken,
  struct templateSlot *slotList)
  {
   int parsingMultislot;
   SYMBOL_HN *slotName;
   struct templateSlot *newSlot;
   int rv;

   /*=====================================================*/
   /* Slots must  begin with keyword field or multifield. */
   /*=====================================================*/

   if ((strcmp(ValueToString(inputToken->value),"field") != 0) &&
       (strcmp(ValueToString(inputToken->value),"multifield") != 0) &&
       (strcmp(ValueToString(inputToken->value),"slot") != 0) &&
       (strcmp(ValueToString(inputToken->value),"multislot") != 0))
     {
      SyntaxErrorMessage(theEnv,"deftemplate");
      DeftemplateData(theEnv)->DeftemplateError = TRUE;
      return(NULL);
     }

   /*===============================================*/
   /* Determine if multifield slot is being parsed. */
   /*===============================================*/

   if ((strcmp(ValueToString(inputToken->value),"multifield") == 0) ||
       (strcmp(ValueToString(inputToken->value),"multislot") == 0))
     { parsingMultislot = TRUE; }
   else
     { parsingMultislot = FALSE; }

   /*========================================*/
   /* The name of the slot must be a symbol. */
   /*========================================*/

   SavePPBuffer(theEnv," ");
   GetToken(theEnv,readSource,inputToken);
   if (inputToken->type != SYMBOL)
     {
      SyntaxErrorMessage(theEnv,"deftemplate");
      DeftemplateData(theEnv)->DeftemplateError = TRUE;
      return(NULL);
     }

   slotName = (SYMBOL_HN *) inputToken->value;

   /*================================================*/
   /* Determine if the slot has already been parsed. */
   /*================================================*/

   while (slotList != NULL)
     {
      if (slotList->slotName == slotName)
        {
         AlreadyParsedErrorMessage(theEnv,"slot ",ValueToString(slotList->slotName));
         DeftemplateData(theEnv)->DeftemplateError = TRUE;
         return(NULL);
        }

      slotList = slotList->next;
     }

   /*===================================*/
   /* Parse the attributes of the slot. */
   /*===================================*/

   newSlot = DefinedSlots(theEnv,readSource,slotName,parsingMultislot,inputToken);
   if (newSlot == NULL)
     {
      DeftemplateData(theEnv)->DeftemplateError = TRUE;
      return(NULL);
     }

   /*=================================*/
   /* Check for slot conflict errors. */
   /*=================================*/

   if (CheckConstraintParseConflicts(theEnv,newSlot->constraints) == FALSE)
     {
      ReturnSlots(theEnv,newSlot);
      DeftemplateData(theEnv)->DeftemplateError = TRUE;
      return(NULL);
     }

   if ((newSlot->defaultPresent) || (newSlot->defaultDynamic))
     { rv = ConstraintCheckExpressionChain(theEnv,newSlot->defaultList,newSlot->constraints); }
   else
     { rv = NO_VIOLATION; }

   if ((rv != NO_VIOLATION) && EnvGetStaticConstraintChecking(theEnv))
     {
      char *temp;
      if (newSlot->defaultDynamic) temp = "the default-dynamic attribute";
      else temp = "the default attribute";
      ConstraintViolationErrorMessage(theEnv,"An expression",temp,FALSE,0,
                                      newSlot->slotName,0,rv,newSlot->constraints,TRUE);
      ReturnSlots(theEnv,newSlot);
      DeftemplateData(theEnv)->DeftemplateError = TRUE;
      return(NULL);
     }

   /*==================*/
   /* Return the slot. */
   /*==================*/

   return(newSlot);
  }

/**************************************************************/
/* DefinedSlots: Parses a field or multifield slot attribute. */
/**************************************************************/
static struct templateSlot *DefinedSlots(
  void *theEnv,
  char *readSource,
  SYMBOL_HN *slotName,
  int multifieldSlot,
  struct token *inputToken)
  {
   struct templateSlot *newSlot;
   struct expr *defaultList;
   int defaultFound = FALSE;
   int noneSpecified, deriveSpecified;
   CONSTRAINT_PARSE_RECORD parsedConstraints;

   /*===========================*/
   /* Build the slot container. */
   /*===========================*/

   newSlot = get_struct(theEnv,templateSlot);
   newSlot->slotName = slotName;
   newSlot->defaultList = NULL;
   newSlot->constraints = GetConstraintRecord(theEnv);
   if (multifieldSlot)
     { newSlot->constraints->multifieldsAllowed = TRUE; }
   newSlot->multislot = multifieldSlot;
   newSlot->noDefault = FALSE;
   newSlot->defaultPresent = FALSE;
   newSlot->defaultDynamic = FALSE;
   newSlot->next = NULL;

   /*========================================*/
   /* Parse the primitive slot if it exists. */
   /*========================================*/

   InitializeConstraintParseRecord(&parsedConstraints);
   GetToken(theEnv,readSource,inputToken);

   while (inputToken->type != RPAREN)
     {
      PPBackup(theEnv);
      SavePPBuffer(theEnv," ");
      SavePPBuffer(theEnv,inputToken->printForm);

      /*================================================*/
      /* Slot attributes begin with a left parenthesis. */
      /*================================================*/

      if (inputToken->type != LPAREN)
        {
         SyntaxErrorMessage(theEnv,"deftemplate");
         ReturnSlots(theEnv,newSlot);
         DeftemplateData(theEnv)->DeftemplateError = TRUE;
         return(NULL);
        }

      /*=============================================*/
      /* The name of the attribute must be a symbol. */
      /*=============================================*/

      GetToken(theEnv,readSource,inputToken);
      if (inputToken->type != SYMBOL)
        {
         SyntaxErrorMessage(theEnv,"deftemplate");
         ReturnSlots(theEnv,newSlot);
         DeftemplateData(theEnv)->DeftemplateError = TRUE;
         return(NULL);
        }

      /*================================================================*/
      /* Determine if the attribute is one of the standard constraints. */
      /*================================================================*/

      if (StandardConstraint(ValueToString(inputToken->value)))
        {
         if (ParseStandardConstraint(theEnv,readSource,(ValueToString(inputToken->value)),
                                     newSlot->constraints,&parsedConstraints,
                                     multifieldSlot) == FALSE)
           {
            DeftemplateData(theEnv)->DeftemplateError = TRUE;
            ReturnSlots(theEnv,newSlot);
            return(NULL);
           }
        }

      /*=================================================*/
      /* else if the attribute is the default attribute, */
      /* then get the default list for this slot.        */
      /*=================================================*/

      else if ((strcmp(ValueToString(inputToken->value),"default") == 0) ||
               (strcmp(ValueToString(inputToken->value),"default-dynamic") == 0))
        {
         /*======================================================*/
         /* Check to see if the default has already been parsed. */
         /*======================================================*/

         if (defaultFound)
           {
            AlreadyParsedErrorMessage(theEnv,"default attribute",NULL);
            DeftemplateData(theEnv)->DeftemplateError = TRUE;
            ReturnSlots(theEnv,newSlot);
            return(NULL);
           }

         newSlot->noDefault = FALSE;

         /*=====================================================*/
         /* Determine whether the default is dynamic or static. */
         /*=====================================================*/

         if (strcmp(ValueToString(inputToken->value),"default") == 0)
           {
            newSlot->defaultPresent = TRUE;
            newSlot->defaultDynamic = FALSE;
           }
         else
           {
            newSlot->defaultPresent = FALSE;
            newSlot->defaultDynamic = TRUE;
           }

         /*===================================*/
         /* Parse the list of default values. */
         /*===================================*/

         defaultList = ParseDefault(theEnv,readSource,multifieldSlot,(int) newSlot->defaultDynamic,
                                  TRUE,&noneSpecified,&deriveSpecified,&DeftemplateData(theEnv)->DeftemplateError);
         if (DeftemplateData(theEnv)->DeftemplateError == TRUE)
           {
            ReturnSlots(theEnv,newSlot);
            return(NULL);
           }

         /*==================================*/
         /* Store the default with the slot. */
         /*==================================*/

         defaultFound = TRUE;
         if (deriveSpecified) newSlot->defaultPresent = FALSE;
         else if (noneSpecified)
           {
            newSlot->noDefault = TRUE;
            newSlot->defaultPresent = FALSE;
           }
         newSlot->defaultList = defaultList;
        }

      /*============================================*/
      /* Otherwise the attribute is an invalid one. */
      /*============================================*/

      else
        {
         SyntaxErrorMessage(theEnv,"slot attributes");
         ReturnSlots(theEnv,newSlot);
         DeftemplateData(theEnv)->DeftemplateError = TRUE;
         return(NULL);
        }

      /*===================================*/
      /* Begin parsing the next attribute. */
      /*===================================*/

      GetToken(theEnv,readSource,inputToken);
     }

   /*============================*/
   /* Return the attribute list. */
   /*============================*/

   return(newSlot);
  }

#endif /* (! RUN_TIME) && (! BLOAD_ONLY) */

#endif /* DEFTEMPLATE_CONSTRUCT */


