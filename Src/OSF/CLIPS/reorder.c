   /*******************************************************/
   /*      "C" Language Integrated Production System      */
   /*                                                     */
   /*             CLIPS Version 6.21  06/15/03            */
   /*                                                     */
   /*                    REORDER MODULE                   */
   /*******************************************************/

/*************************************************************/
/* Purpose: Provides routines necessary for converting the   */
/*   the LHS of a rule into an appropriate form suitable for */
/*   the KB Rete topology. This includes transforming the    */
/*   LHS so there is at most one "or" CE (and this is the    */
/*   first CE of the LHS if it is used), adding initial      */
/*   patterns to the LHS (if no LHS is specified or a "test" */
/*   or "not" CE is the first pattern within an "and" CE),   */
/*   removing redundant CEs, and determining appropriate     */
/*   information on nesting for implementing joins from the  */
/*   right.                                                  */
/*                                                           */
/* Principal Programmer(s):                                  */
/*      Gary D. Riley                                        */
/*                                                           */
/* Contributing Programmer(s):                               */
/*                                                           */
/* Revision History:                                         */
/*                                                           */
/*************************************************************/

#define _REORDER_SOURCE_

#include "setup.h"

#if (! RUN_TIME) && (! BLOAD_ONLY) && DEFRULE_CONSTRUCT

#include <stdio.h>
#define _STDIO_INCLUDED_

#include "cstrnutl.h"
#include "envrnmnt.h"
#include "extnfunc.h"
#include "memalloc.h"
#include "pattern.h"
#include "prntutil.h"
#include "router.h"
#include "rulelhs.h"

#include "reorder.h"

/***************************************/
/* LOCAL INTERNAL FUNCTION DEFINITIONS */
/***************************************/

   static struct lhsParseNode    *ReverseAndOr(void *,struct lhsParseNode *,struct lhsParseNode *,int);
   static struct lhsParseNode    *PerformReorder1(void *,struct lhsParseNode *,int *);
   static struct lhsParseNode    *PerformReorder2(void *,struct lhsParseNode *,int *);
   static struct lhsParseNode    *CompressCEs(void *,struct lhsParseNode *,int *);
   static void                    IncrementNandDepth(void *,struct lhsParseNode *,int);
   static struct lhsParseNode    *CreateInitialPattern(void *,struct patternParser *);
   static struct lhsParseNode    *ReorderDriver(void *,struct lhsParseNode *,int *,int);
   static void                    AddRemainingInitialPatterns(void *,struct lhsParseNode *,struct patternParser *);
   static void                    PrintNodes(void *,char *,struct lhsParseNode *);
   static struct lhsParseNode    *AssignPatternIndices(struct lhsParseNode *,short);
   static void                    PropagateIndexSlotPatternValues(struct lhsParseNode *,
                                                                  short,short,
                                                                  struct symbolHashNode *,
                                                                  short);

/********************************************/
/* ReorderPatterns: Reorders a group of CEs */
/*   to accommodate KB Rete topology.       */
/********************************************/
globle struct lhsParseNode *ReorderPatterns(
  void *theEnv,
  struct lhsParseNode *theLHS,
  int *anyChange)
  {
   struct lhsParseNode *newLHS, *patternPtr, *tempLHS, *lastLHS;
   unsigned int whichCE;

   /*===========================================================*/
   /* The LHS of a rule is enclosed within an implied "and" CE. */
   /*===========================================================*/

   newLHS = GetLHSParseNode(theEnv);
   newLHS->type = AND_CE;

   /*=============================================*/
   /* If the LHS of the rule was left unspecified */
   /* (e.g., (defrule x => ...)), then add an     */
   /* initial fact or instance pattern to the LHS */
   /* of the rule. Otherwise, attach the user     */
   /* specified LHS to the implied "and" Ce.      */
   /*=============================================*/

   if (theLHS == NULL) newLHS->right = CreateInitialPattern(theEnv,NULL);
   else newLHS->right = theLHS;

   /*=======================================================*/
   /* Reorder the patterns to support the KB Rete topology. */
   /*=======================================================*/

   newLHS = ReorderDriver(theEnv,newLHS,anyChange,1);
   newLHS = ReorderDriver(theEnv,newLHS,anyChange,2);

   /*===========================================*/
   /* The top level and CE may have disappeared */
   /* as a result of pattern compression.       */
   /*===========================================*/

   if (newLHS->type == OR_CE)
     {
      for (tempLHS = newLHS->right, lastLHS = NULL;
           tempLHS != NULL;
           lastLHS = tempLHS, tempLHS = tempLHS->bottom)
        {
         if (tempLHS->type != AND_CE)
           {
            theLHS = GetLHSParseNode(theEnv);
            theLHS->type = AND_CE;
            theLHS->right = tempLHS;
            theLHS->bottom = tempLHS->bottom;
            tempLHS->bottom = NULL;
            if (lastLHS == NULL)
              { newLHS->right = theLHS; }
            else
              { lastLHS->bottom = theLHS; }
            tempLHS = theLHS;
           }
        }
     }
   else if (newLHS->type != AND_CE)
     {
      theLHS = newLHS;
      newLHS = GetLHSParseNode(theEnv);
      newLHS->type = AND_CE;
      newLHS->right = theLHS;
     }

   /*=====================================================*/
   /* Add initial patterns where needed (such as before a */
   /* "test" CE or "not" CE which is the first CE within  */
   /* an "and" CE).                                       */
   /*=====================================================*/

   AddInitialPatterns(theEnv,newLHS);

   /*===========================================================*/
   /* Number the user specified patterns. Patterns added while  */
   /* analyzing the rule (such as placing initial-fact patterns */
   /* before not CEs) are not numbered so that there is no      */
   /* confusion when an error message refers to a CE. Also      */
   /* propagate field and slot values throughout each pattern.  */
   /*===========================================================*/

   if (newLHS->type == OR_CE) theLHS = newLHS->right;
   else theLHS = newLHS;

   for (;
        theLHS != NULL;
        theLHS = theLHS->bottom)
     {
      whichCE = 1;
      for (patternPtr = theLHS->right;
           patternPtr != NULL;
           patternPtr = patternPtr->bottom)
        { if (patternPtr->userCE) patternPtr->whichCE = whichCE++; }

      AssignPatternIndices(theLHS->right,1);
     }

   /*===========================*/
   /* Return the processed LHS. */
   /*===========================*/

   return(newLHS);
  }

/******************************************/
/* ReorderDriver: Reorders a group of CEs */
/*   to accommodate KB Rete topology.     */
/******************************************/
static struct lhsParseNode *ReorderDriver(
  void *theEnv,
  struct lhsParseNode *theLHS,
  int *anyChange,
  int pass)
  {
   struct lhsParseNode *argPtr;
   struct lhsParseNode *before, *save;
   int change, newChange;
   *anyChange = FALSE;

   /*===================================*/
   /* Continue processing the LHS until */
   /* no more changes have been made.   */
   /*===================================*/

   change = TRUE;
   while (change)
     {
      /*==================================*/
      /* No change yet on this iteration. */
      /*==================================*/

      change = FALSE;

      /*=======================================*/
      /* Reorder the current level of the LHS. */
      /*=======================================*/

      if ((theLHS->type == AND_CE) ||
          (theLHS->type == NOT_CE) ||
          (theLHS->type == OR_CE))
        {
         if (pass == 1) theLHS = PerformReorder1(theEnv,theLHS,&newChange);
         else theLHS = PerformReorder2(theEnv,theLHS,&newChange);

         if (newChange)
           {
            *anyChange = TRUE;
            change = TRUE;
           }

         theLHS = CompressCEs(theEnv,theLHS,&newChange);

         if (newChange)
           {
            *anyChange = TRUE;
            change = TRUE;
           }
        }

      /*=====================================================*/
      /* Recursively reorder CEs at lower levels in the LHS. */
      /*=====================================================*/

      before = NULL;
      argPtr = theLHS->right;

      while (argPtr != NULL)
        {
         /*==================================*/
         /* Remember the next CE to reorder. */
         /*==================================*/

         save = argPtr->bottom;

         /*============================================*/
         /* Reorder the current CE at the lower level. */
         /*============================================*/

         if ((argPtr->type == AND_CE) ||
             (argPtr->type == NOT_CE) ||
             (argPtr->type == OR_CE))
           {
            if (before == NULL)
              {
               argPtr->bottom = NULL;
               theLHS->right = ReorderDriver(theEnv,argPtr,&newChange,pass);
               theLHS->right->bottom = save;
               before = theLHS->right;
              }
            else
              {
               argPtr->bottom = NULL;
               before->bottom = ReorderDriver(theEnv,argPtr,&newChange,pass);
               before->bottom->bottom = save;
               before = before->bottom;
              }

            if (newChange)
              {
               *anyChange = TRUE;
               change = TRUE;
              }
           }
         else
           { before = argPtr; }

         /*====================================*/
         /* Move on to the next CE to reorder. */
         /*====================================*/

         argPtr = save;
        }
     }

   /*===========================*/
   /* Return the reordered LHS. */
   /*===========================*/

   return(theLHS);
  }

/****************************************************************/
/* AddInitialPatterns: Add initial patterns to CEs where needed */
/*   (such as before a "test" CE or "not" CE which is the first */
/*   CE within an "and" CE).                                    */
/****************************************************************/
globle void AddInitialPatterns(
  void *theEnv,
  struct lhsParseNode *theLHS)
  {
   struct lhsParseNode *thePattern;
   struct patternParser *lastType;

   /*====================================================*/
   /* If there are multiple disjuncts for the rule, then */
   /* add initial patterns to each disjunct separately.  */
   /*====================================================*/

   if (theLHS->type == OR_CE)
     {
      for (thePattern = theLHS->right;
           thePattern != NULL;
           thePattern = thePattern->bottom)
        { AddInitialPatterns(theEnv,thePattern); }

      return;
     }

   /*======================================================*/
   /* Determine what the default pattern type for the rule */
   /* should be (in case the rule begins with a test CE).  */
   /* The default pattern type is the type of the data     */
   /* entity associated with the first pattern CE found in */
   /* the LHS of the rule.                                 */
   /*======================================================*/

   for (lastType = NULL, thePattern = theLHS->right;
        thePattern != NULL;
        thePattern = thePattern->bottom)
     {
      if (thePattern->type == PATTERN_CE)
        {
         lastType = thePattern->patternType;
         break;
        }
     }

   /*===================================================*/
   /* Add an initial pattern to the first CE of a rule  */
   /* if the CE is a "test" CE, "not" CE or a join from */
   /* the right.                                        */
   /*===================================================*/

   if ((theLHS->right->negated) ||
       (theLHS->right->type == TEST_CE) ||
       (theLHS->right->beginNandDepth > 1))
     {
      thePattern = CreateInitialPattern(theEnv,lastType);

      thePattern->logical = (theLHS->logical || theLHS->right->logical);
      thePattern->bottom = theLHS->right;
      theLHS->right = thePattern;
     }

   /*================================*/
   /* Handle the remaining patterns. */
   /*================================*/

   AddRemainingInitialPatterns(theEnv,theLHS->right,lastType);
  }

/***********************************************************/
/* PerformReorder1: Reorders a group of CEs to accommodate */
/*   KB Rete topology. The first pass of this function     */
/*   transforms or CEs into equivalent forms.              */
/***********************************************************/
static struct lhsParseNode *PerformReorder1(
  void *theEnv,
  struct lhsParseNode *theLHS,
  int *newChange)
  {
   struct lhsParseNode *argPtr, *lastArg, *nextArg;
   struct lhsParseNode *tempArg, *newNode;
   int count;
   int change;

   /*======================================================*/
   /* Loop through the CEs as long as changes can be made. */
   /*======================================================*/

   change = TRUE;
   *newChange = FALSE;

   while (change)
     {
      change = FALSE;
      count = 1;
      lastArg = NULL;

      for (argPtr = theLHS->right;
           argPtr != NULL;)
        {
         /*=============================================================*/
         /* Convert and/or CE combinations into or/and CE combinations. */
         /*=============================================================*/

         if ((theLHS->type == AND_CE) && (argPtr->type == OR_CE))
           {
            theLHS = ReverseAndOr(theEnv,theLHS,argPtr->right,count);

            change = TRUE;
            *newChange = TRUE;
            break;
           }

         /*==============================================================*/
         /* Convert not/or CE combinations into and/not CE combinations. */
         /*==============================================================*/

         else if ((theLHS->type == NOT_CE) && (argPtr->type == OR_CE))
           {
            change = TRUE;
            *newChange = TRUE;

            tempArg = argPtr->right;

            argPtr->right = NULL;
            argPtr->bottom = NULL;
            ReturnLHSParseNodes(theEnv,argPtr);
            theLHS->type = AND_CE;
            theLHS->right = tempArg;

            while (tempArg != NULL)
              {
               newNode = GetLHSParseNode(theEnv);
               CopyLHSParseNode(theEnv,newNode,tempArg,FALSE);
               newNode->right = tempArg->right;
               newNode->bottom = NULL;

               tempArg->type = NOT_CE;
               tempArg->negated = FALSE;
               tempArg->logical = FALSE;
               tempArg->value = NULL;
               tempArg->expression = NULL;
               tempArg->right = newNode;

               tempArg = tempArg->bottom;
              }

            break;
           }

         /*=====================================*/
         /* Remove duplication of or CEs within */
         /* or CEs and and CEs within and CEs.  */
         /*=====================================*/

         else if (((theLHS->type == OR_CE) && (argPtr->type == OR_CE)) ||
                  ((theLHS->type == AND_CE) && (argPtr->type == AND_CE)))
           {
            if (argPtr->logical) theLHS->logical = TRUE;

            change = TRUE;
            *newChange = TRUE;
            tempArg = argPtr->right;
            nextArg = argPtr->bottom;
            argPtr->right = NULL;
            argPtr->bottom = NULL;
            ReturnLHSParseNodes(theEnv,argPtr);

            if (lastArg == NULL)
              { theLHS->right = tempArg; }
            else
              { lastArg->bottom = tempArg; }

            argPtr = tempArg;
            while (tempArg->bottom != NULL) tempArg = tempArg->bottom;
            tempArg->bottom = nextArg;
           }

         /*===================================================*/
         /* If no changes are needed, move on to the next CE. */
         /*===================================================*/

         else
           {
            count++;
            lastArg = argPtr;
            argPtr = argPtr->bottom;
           }
        }
     }

   /*===========================*/
   /* Return the reordered LHS. */
   /*===========================*/

   return(theLHS);
  }

/***********************************************************/
/* PerformReorder2: Reorders a group of CEs to accommodate */
/*   KB Rete topology. The second pass performs all other  */
/*   transformations not associated with the or CE.        */
/***********************************************************/
static struct lhsParseNode *PerformReorder2(
  void *theEnv,
  struct lhsParseNode *theLHS,
  int *newChange)
  {
   struct lhsParseNode *argPtr;
   int change;

   /*======================================================*/
   /* Loop through the CEs as long as changes can be made. */
   /*======================================================*/

   change = TRUE;
   *newChange = FALSE;

   while (change)
     {
      change = FALSE;

      for (argPtr = theLHS->right;
           argPtr != NULL;)
        {
         /*======================================*/
         /* Replace not CEs containing a pattern */
         /* CE with a negated pattern CE.        */
         /*======================================*/

         if ((theLHS->type == NOT_CE) && (argPtr->type == PATTERN_CE))
           {
            change = TRUE;
            *newChange = TRUE;

            CopyLHSParseNode(theEnv,theLHS,argPtr,FALSE);

            theLHS->negated = TRUE;
            theLHS->right = argPtr->right;

            argPtr->networkTest = NULL;
            argPtr->expression = NULL;
            argPtr->userData = NULL;
            argPtr->right = NULL;
            argPtr->bottom = NULL;
            ReturnLHSParseNodes(theEnv,argPtr);
            break;
           }

         /*============================================================*/
         /* Replace "and" and "not" CEs contained within a not CE with */
         /* just the and CE, but increment the nand depths of the      */
         /* pattern contained within.                                  */
         /*============================================================*/

         else if ((theLHS->type == NOT_CE) &&
                  ((argPtr->type == AND_CE) ||  (argPtr->type == NOT_CE)))
           {
            change = TRUE;
            *newChange = TRUE;

            theLHS->type = argPtr->type;

            theLHS->negated = argPtr->negated;
            theLHS->value = argPtr->value;
            theLHS->logical = argPtr->logical;
            theLHS->right = argPtr->right;
            argPtr->right = NULL;
            argPtr->bottom = NULL;
            ReturnLHSParseNodes(theEnv,argPtr);

            IncrementNandDepth(theEnv,theLHS->right,TRUE);
            break;
           }

         /*===================================================*/
         /* If no changes are needed, move on to the next CE. */
         /*===================================================*/

         else
           {
            argPtr = argPtr->bottom;
           }
        }
     }

   /*===========================*/
   /* Return the reordered LHS. */
   /*===========================*/

   return(theLHS);
  }

/**************************************************/
/* ReverseAndOr: Switches and/or CEs into         */
/*   equivalent or/and CEs. For example:          */
/*                                                */
/*     (and (or a b) (or c d))                    */
/*                                                */
/*   would be converted to                        */
/*                                                */
/*     (or (and a (or c d)) (and b (or c d))),    */
/*                                                */
/*   if the "or" CE being expanded was (or a b).  */
/**************************************************/
static struct lhsParseNode *ReverseAndOr(
  void *theEnv,
  struct lhsParseNode *listOfCEs,
  struct lhsParseNode *orCE,
  int orPosition)
  {
   int count;
   struct lhsParseNode *listOfExpandedOrCEs = NULL;
   struct lhsParseNode *lastExpandedOrCE = NULL;
   struct lhsParseNode *copyOfCEs, *replaceCE;

   /*========================================================*/
   /* Loop through each of the CEs contained within the "or" */
   /* CE that is being expanded into the enclosing "and" CE. */
   /*========================================================*/

   while (orCE != NULL)
     {
      /*===============================*/
      /* Make a copy of the and/or CE. */
      /*===============================*/

      copyOfCEs = CopyLHSParseNodes(theEnv,listOfCEs);

      /*====================================================*/
      /* Get a pointer to the "or" CE being expanded in the */
      /* copy just made based on the position of the "or"   */
      /* CE in the original and/or CE (e.g., 1st, 2nd).     */
      /*====================================================*/

      for (count = 1, replaceCE = copyOfCEs->right;
           count != orPosition;
           count++, replaceCE = replaceCE->bottom)
        { /* Do Nothing*/ }

      /*===================================================*/
      /* Delete the contents of the "or" CE being expanded */
      /* in the copy of the and/or CE. From the example    */
      /* above, (and (or a b) (or c d)) would be replaced  */
      /* with (and (or) (or c d)). Note that the "or" CE   */
      /* is still left as a placeholder.                   */
      /*===================================================*/

      ReturnLHSParseNodes(theEnv,replaceCE->right);

      /*======================================================*/
      /* Copy the current CE being examined in the "or" CE to */
      /* the placeholder left in the and/or CE. From the      */
      /* example above, (and (or) (or c d)) would be replaced */
      /* with (and a (or c d)) if the "a" pattern from the    */
      /* "or" CE was being examined or (and b (or c d)) if    */
      /* the "b" pattern from the "or" CE was being examined. */
      /*======================================================*/

      CopyLHSParseNode(theEnv,replaceCE,orCE,TRUE);
      replaceCE->right = CopyLHSParseNodes(theEnv,orCE->right);

      /*====================================*/
      /* Add the newly expanded "and" CE to */
      /* the list of CEs already expanded.  */
      /*====================================*/

      if (lastExpandedOrCE == NULL)
        {
         listOfExpandedOrCEs = copyOfCEs;
         copyOfCEs->bottom = NULL;
         lastExpandedOrCE = copyOfCEs;
        }
      else
        {
         lastExpandedOrCE->bottom = copyOfCEs;
         copyOfCEs->bottom = NULL;
         lastExpandedOrCE = copyOfCEs;
        }

      /*=======================================================*/
      /* Move on to the next CE in the "or" CE being expanded. */
      /*=======================================================*/

      orCE = orCE->bottom;
     }

   /*=====================================================*/
   /* Release the original and/or CE list to free memory. */
   /*=====================================================*/

   ReturnLHSParseNodes(theEnv,listOfCEs);

   /*================================================*/
   /* Wrap an or CE around the list of expanded CEs. */
   /*================================================*/

   copyOfCEs = GetLHSParseNode(theEnv);
   copyOfCEs->type = OR_CE;
   copyOfCEs->right = listOfExpandedOrCEs;

   /*================================*/
   /* Return the newly expanded CEs. */
   /*================================*/

   return(copyOfCEs);
  }

/***********************************************************/
/* CompressCEs:              */
/***********************************************************/
static struct lhsParseNode *CompressCEs(
  void *theEnv,
  struct lhsParseNode *theLHS,
  int *newChange)
  {
   struct lhsParseNode *argPtr, *lastArg, *nextArg;
   struct lhsParseNode *tempArg;
   int change;
   struct expr *e1, *e2;

   /*======================================================*/
   /* Loop through the CEs as long as changes can be made. */
   /*======================================================*/

   change = TRUE;
   *newChange = FALSE;

   while (change)
     {
      change = FALSE;
      lastArg = NULL;

      for (argPtr = theLHS->right;
           argPtr != NULL;)
        {
         /*=====================================*/
         /* Remove duplication of or CEs within */
         /* or CEs and and CEs within and CEs.  */
         /*=====================================*/

         if (((theLHS->type == OR_CE) && (argPtr->type == OR_CE)) ||
             ((theLHS->type == AND_CE) && (argPtr->type == AND_CE)))
           {
            if (argPtr->logical) theLHS->logical = TRUE;

            change = TRUE;
            *newChange = TRUE;
            tempArg = argPtr->right;
            nextArg = argPtr->bottom;
            argPtr->right = NULL;
            argPtr->bottom = NULL;
            ReturnLHSParseNodes(theEnv,argPtr);

            if (lastArg == NULL)
              { theLHS->right = tempArg; }
            else
              { lastArg->bottom = tempArg; }

            argPtr = tempArg;
            while (tempArg->bottom != NULL) tempArg = tempArg->bottom;
            tempArg->bottom = nextArg;
           }

         /*=======================================================*/
         /* Replace not CEs containing a test CE with just a test */
         /* CE with the original test CE condition negated.       */
         /*=======================================================*/

         else if ((theLHS->type == NOT_CE) && (argPtr->type == TEST_CE))
           {
            change = TRUE;
            *newChange = TRUE;

            e1 = GenConstant(theEnv,FCALL,ExpressionData(theEnv)->PTR_NOT);
            e2 = LHSParseNodesToExpression(theEnv,argPtr->expression);
            e1->arg_list = e2;

            CopyLHSParseNode(theEnv,theLHS,argPtr,TRUE);

            ReturnLHSParseNodes(theEnv,argPtr);
            ReturnLHSParseNodes(theEnv,theLHS->expression);

            theLHS->expression = ExpressionToLHSParseNodes(theEnv,e1);
            theLHS->right = NULL;
            ReturnExpression(theEnv,e1);

            break;
           }

         /*==============================*/
         /* Two adjacent test CEs within */
         /* an and CE can be combined.   */
         /*==============================*/

         else if ((theLHS->type == AND_CE) && (argPtr->type == TEST_CE) &&
                  ((argPtr->bottom != NULL) ? argPtr->bottom->type == TEST_CE :
                                              FALSE) &&
                   (argPtr->beginNandDepth == argPtr->endNandDepth) &&
                   (argPtr->endNandDepth == argPtr->bottom->beginNandDepth))
           {
            change = TRUE;
            *newChange = TRUE;

            e1 = LHSParseNodesToExpression(theEnv,argPtr->expression);
            e2 = LHSParseNodesToExpression(theEnv,argPtr->bottom->expression);
            e1 = CombineExpressions(theEnv,e1,e2);

            ReturnLHSParseNodes(theEnv,argPtr->expression);
            argPtr->expression = ExpressionToLHSParseNodes(theEnv,e1);
            ReturnExpression(theEnv,e1);

            tempArg = argPtr->bottom;
            argPtr->bottom = tempArg->bottom;
            tempArg->bottom = NULL;

            ReturnLHSParseNodes(theEnv,tempArg);
           }

         /*=====================================*/
         /* Replace and CEs containing a single */
         /* test CE with just a test CE.        */
         /*=====================================*/

         else if ((theLHS->type == AND_CE) && (argPtr->type == TEST_CE) &&
                  (theLHS->right == argPtr) && (argPtr->bottom == NULL))
           {
            change = TRUE;
            *newChange = TRUE;

            CopyLHSParseNode(theEnv,theLHS,argPtr,TRUE);
            theLHS->right = NULL;
            ReturnLHSParseNodes(theEnv,argPtr);
            break;
           }

         /*===================================================*/
         /* If no changes are needed, move on to the next CE. */
         /*===================================================*/

         else
           {
            lastArg = argPtr;
            argPtr = argPtr->bottom;
           }
        }
     }

   /*===========================*/
   /* Return the reordered LHS. */
   /*===========================*/

   return(theLHS);
  }

/*********************************************************************/
/* CopyLHSParseNodes: Copies a linked group of conditional elements. */
/*********************************************************************/
globle struct lhsParseNode *CopyLHSParseNodes(
  void *theEnv,
  struct lhsParseNode *listOfCEs)
  {
   struct lhsParseNode *newList;

   if (listOfCEs == NULL)
     { return(NULL); }

   newList = get_struct(theEnv,lhsParseNode);
   CopyLHSParseNode(theEnv,newList,listOfCEs,TRUE);

   newList->right = CopyLHSParseNodes(theEnv,listOfCEs->right);
   newList->bottom = CopyLHSParseNodes(theEnv,listOfCEs->bottom);

   return(newList);
  }

/**********************************************************/
/* CopyLHSParseNode: Copies a single conditional element. */
/**********************************************************/
globle void CopyLHSParseNode(
  void *theEnv,
  struct lhsParseNode *dest,
  struct lhsParseNode *src,
  int duplicate)
  {
   dest->type = src->type;
   dest->value = src->value;
   dest->negated = src->negated;
   dest->bindingVariable = src->bindingVariable;
   dest->withinMultifieldSlot = src->withinMultifieldSlot;
   dest->multifieldSlot = src->multifieldSlot;
   dest->multiFieldsBefore = src->multiFieldsBefore;
   dest->multiFieldsAfter = src->multiFieldsAfter;
   dest->singleFieldsBefore = src->singleFieldsBefore;
   dest->singleFieldsAfter = src->singleFieldsAfter;
   dest->logical = src->logical;
   dest->userCE = src->userCE;
   dest->referringNode = src->referringNode;
   dest->patternType = src->patternType;
   dest->pattern = src->pattern;
   dest->index = src->index;
   dest->slot = src->slot;
   dest->slotNumber = src->slotNumber;
   dest->beginNandDepth = src->beginNandDepth;
   dest->endNandDepth = src->endNandDepth;

   /*==========================================================*/
   /* The duplicate flag controls whether pointers to existing */
   /* data structures are used when copying some slots or if   */
   /* new copies of the data structures are made.              */
   /*==========================================================*/

   if (duplicate)
     {
      dest->networkTest = CopyExpression(theEnv,src->networkTest);
      if (src->userData == NULL)
        { dest->userData = NULL; }
      else if (src->patternType->copyUserDataFunction == NULL)
        { dest->userData = src->userData; }
      else
        { dest->userData = (*src->patternType->copyUserDataFunction)(theEnv,src->userData); }
      dest->expression = CopyLHSParseNodes(theEnv,src->expression);
      dest->constraints = CopyConstraintRecord(theEnv,src->constraints);
      if (dest->constraints != NULL) dest->derivedConstraints = TRUE;
      else dest->derivedConstraints = FALSE;
     }
   else
     {
      dest->networkTest = src->networkTest;
      dest->userData = src->userData;
      dest->expression = src->expression;
      dest->derivedConstraints = FALSE;
      dest->constraints = src->constraints;
     }
  }

/****************************************************/
/* GetLHSParseNode: Creates an empty node structure */
/*   used for building conditional elements.        */
/****************************************************/
globle struct lhsParseNode *GetLHSParseNode(
  void *theEnv)
  {
   struct lhsParseNode *newNode;

   newNode = get_struct(theEnv,lhsParseNode);
   newNode->type = UNKNOWN_VALUE;
   newNode->value = NULL;
   newNode->negated = FALSE;
   newNode->bindingVariable = FALSE;
   newNode->withinMultifieldSlot = FALSE;
   newNode->multifieldSlot = FALSE;
   newNode->multiFieldsBefore = 0;
   newNode->multiFieldsAfter = 0;
   newNode->singleFieldsBefore = 0;
   newNode->singleFieldsAfter = 0;
   newNode->logical = FALSE;
   newNode->derivedConstraints = FALSE;
   newNode->userCE = TRUE;
   newNode->constraints = NULL;
   newNode->referringNode = NULL;
   newNode->patternType = NULL;
   newNode->pattern = -1;
   newNode->index = -1;
   newNode->slot = NULL;
   newNode->slotNumber = -1;
   newNode->beginNandDepth = 1;
   newNode->endNandDepth = 1;
   newNode->userData = NULL;
   newNode->networkTest = NULL;
   newNode->expression = NULL;
   newNode->right = NULL;
   newNode->bottom = NULL;

   return(newNode);
  }

/********************************************************/
/* ReturnLHSParseNodes:  Returns a multiply linked list */
/*   of lhsParseNode structures to the memory manager.  */
/********************************************************/
globle void ReturnLHSParseNodes(
  void *theEnv,
  struct lhsParseNode *waste)
  {
   if (waste != NULL)
     {
      ReturnExpression(theEnv,waste->networkTest);
      ReturnLHSParseNodes(theEnv,waste->right);
      ReturnLHSParseNodes(theEnv,waste->bottom);
      ReturnLHSParseNodes(theEnv,waste->expression);
      if (waste->derivedConstraints) RemoveConstraint(theEnv,waste->constraints);
      if ((waste->userData != NULL) &&
          (waste->patternType->returnUserDataFunction != NULL))
        { (*waste->patternType->returnUserDataFunction)(theEnv,waste->userData); }
      rtn_struct(theEnv,lhsParseNode,waste);
     }
  }

/********************************************************/
/* ExpressionToLHSParseNodes: Copies an expression into */
/*   the equivalent lhsParseNode data structures.       */
/********************************************************/
globle struct lhsParseNode *ExpressionToLHSParseNodes(
  void *theEnv,
  struct expr *expressionList)
  {
   struct lhsParseNode *newList, *theList;
   struct FunctionDefinition *theFunction;
   int i, theRestriction;

   /*===========================================*/
   /* A NULL expression requires no conversion. */
   /*===========================================*/

   if (expressionList == NULL) return(NULL);

   /*====================================*/
   /* Recursively convert the expression */
   /* to lhsParseNode data structures.   */
   /*====================================*/

   newList = GetLHSParseNode(theEnv);
   newList->type = expressionList->type;
   newList->value = expressionList->value;
   newList->right = ExpressionToLHSParseNodes(theEnv,expressionList->nextArg);
   newList->bottom = ExpressionToLHSParseNodes(theEnv,expressionList->argList);

   /*==================================================*/
   /* If the expression is a function call, then store */
   /* the constraint information for the functions     */
   /* arguments in the lshParseNode data structures.   */
   /*==================================================*/

   if (newList->type != FCALL) return(newList);

   theFunction = (struct FunctionDefinition *) newList->value;
   for (theList = newList->bottom, i = 1;
        theList != NULL;
        theList = theList->right, i++)
     {
      if (theList->type == SF_VARIABLE)
        {
         theRestriction = GetNthRestriction(theFunction,i);
         theList->constraints = ArgumentTypeToConstraintRecord(theEnv,theRestriction);
         theList->derivedConstraints = TRUE;
        }
     }

   /*==================================*/
   /* Return the converted expression. */
   /*==================================*/

   return(newList);
  }

/******************************************************************/
/* LHSParseNodesToExpression: Copies lhsParseNode data structures */
/*   into the equivalent expression data structures.              */
/******************************************************************/
globle struct expr *LHSParseNodesToExpression(
  void *theEnv,
  struct lhsParseNode *nodeList)
  {
   struct expr *newList;

   if (nodeList == NULL)
     { return(NULL); }

   newList = get_struct(theEnv,expr);
   newList->type = nodeList->type;
   newList->value = nodeList->value;
   newList->nextArg = LHSParseNodesToExpression(theEnv,nodeList->right);
   newList->argList = LHSParseNodesToExpression(theEnv,nodeList->bottom);

   return(newList);
  }

/************************************************************/
/* IncrementNandDepth: Increments the nand depth of a group */
/*   of CEs. The nand depth is used to indicate the nesting */
/*   of not/and or not/not CEs which are implemented using  */
/*   joins from the right. A single pattern within a "not"  */
/*   CE does not require a join from the right and its nand */
/*   depth is normally not increased (except when it's      */
/*   within a not/and or not/not CE. The begin nand depth   */
/*   indicates the current nesting for a CE. The end nand   */
/*   depth indicates the nand depth in the following CE     */
/*   (assuming that the next CE is not the beginning of a   */
/*   new group of nand CEs). All but the last CE in a nand  */
/*   group should have the same begin and end nand depths.  */
/*   Since a single CE can be the last CE of several nand   */
/*   groups, it is possible to have an end nand depth that  */
/*   is more than 1 less than the begin nand depth of the   */
/*   CE.                                                    */
/************************************************************/
static void IncrementNandDepth(
  void *theEnv,
  struct lhsParseNode *theLHS,
  int lastCE)
  {
   /*======================================*/
   /* Loop through each CE in the group of */
   /* CEs having its nand depth increased. */
   /*======================================*/

   for (;
        theLHS != NULL;
        theLHS = theLHS->bottom)
     {
      /*=========================================================*/
      /* Increment the begin nand depth of pattern and test CEs. */
      /* The last CE in the original list doesn't have its end   */
      /* nand depth incremented. All other last CEs in other CEs */
      /* entered recursively do have their end depth incremented */
      /* (unless the last CE in the recursively entered CE is    */
      /* the same last CE as contained in the original group     */
      /* when this function was first entered).                  */
      /*=========================================================*/

      if ((theLHS->type == PATTERN_CE) || (theLHS->type == TEST_CE))
        {
         theLHS->beginNandDepth++;

         if (lastCE == FALSE) theLHS->endNandDepth++;
         else if (theLHS->bottom != NULL) theLHS->endNandDepth++;
        }

      /*==============================================*/
      /* Recursively increase the nand depth of other */
      /* CEs contained within the CE having its nand  */
      /* depth increased.                             */
      /*==============================================*/

      else if ((theLHS->type == AND_CE) || (theLHS->type == NOT_CE))
        {
         IncrementNandDepth(theEnv,theLHS->right,
                            (lastCE ? (theLHS->bottom == NULL) : FALSE));
        }

      /*=====================================*/
      /* All or CEs should have been removed */
      /* from the LHS at this point.         */
      /*=====================================*/

      else if (theLHS->type == OR_CE)
        { SystemError(theEnv,"REORDER",1); }
     }
  }

/***********************************************************/
/* CreateInitialPattern: Creates a default pattern used in */
/*  the LHS of a rule under certain cirmustances (such as  */
/*  when a "not" or "test" CE is the first CE in an "and"  */
/*  CE or when no CEs are specified in the LHS of a rule.  */
/***********************************************************/
static struct lhsParseNode *CreateInitialPattern(
  void *theEnv,
  struct patternParser *theParser)
  {
   struct lhsParseNode *topNode;

   /*==========================================*/
   /* Create the top most node of the pattern. */
   /*==========================================*/

   topNode = GetLHSParseNode(theEnv);
   topNode->type = PATTERN_CE;
   topNode->userCE = FALSE;
   topNode->bottom = NULL;

   /*==============================================*/
   /* If a pattern type was not supplied, then try */
   /* to use the (initial-fact) fact pattern.      */
   /*==============================================*/

   if (theParser == NULL)
     { theParser = FindPatternParser(theEnv,"facts"); }

   /*====================================================*/
   /* If a pattern type was supplied or the initial fact */
   /* pattern is available, create the initial pattern   */
   /* with one of those in the given order.              */
   /*====================================================*/

   if (theParser != NULL)
     {
      topNode->right = (*theParser->initialPatternFunction)(theEnv);
      PropagatePatternType(topNode,theParser);
      return(topNode);
     }

   /*=============================================*/
   /* If neither a pattern type was supplied nor  */
   /* the initial fact pattern was available, use */
   /* any available initial pattern.              */
   /*=============================================*/

   for (theParser = PatternData(theEnv)->ListOfPatternParsers;
        theParser != NULL;
        theParser = theParser->next)
     {
      if (theParser->initialPatternFunction != NULL)
        {
         topNode->right = (*theParser->initialPatternFunction)(theEnv);
         PropagatePatternType(topNode,theParser);
         return(topNode);
        }
     }

   /*===========================================*/
   /* There must be at least one pattern parser */
   /* capable of creating an initial pattern.   */
   /*===========================================*/

   SystemError(theEnv,"REORDER",2);

   return(NULL);
  }

/*****************************************************************/
/* AddRemainingInitialPatterns: Finishes adding initial patterns */
/*   where needed on the LHS of a rule. Assumes that an initial  */
/*   pattern has been added to the beginning of the rule if one  */
/*   was needed.                                                 */
/*****************************************************************/
static void AddRemainingInitialPatterns(
  void *theEnv,
  struct lhsParseNode *theLHS,
  struct patternParser *defaultType)
  {
   struct lhsParseNode *trackNode, *tempNode, *lastNode;

   /*====================================================*/
   /* Set the mark flag for each CE in the LHS to FALSE. */
   /*====================================================*/

   for (trackNode = theLHS; trackNode != NULL; trackNode = trackNode->bottom)
     { trackNode->marked = FALSE; }

   /*==================================*/
   /* Loop through each CE in the LHS. */
   /*==================================*/

   lastNode = NULL;
   while (theLHS != NULL)
     {
      /*===================================*/
      /* A "not" CE will not propagate its */
      /* pattern type to following CEs.    */
      /*===================================*/

      if ((theLHS->negated) && (theLHS->marked))
        { trackNode = NULL; }

      /*==================================================*/
      /* A "test" or "not" CE was found that has not been */
      /* marked. Add an initial pattern before the CE.    */
      /*==================================================*/

      else if (((theLHS->type == TEST_CE) || (theLHS->negated)) &&
               (theLHS->marked == FALSE))
        {
         if (theLHS->negated) tempNode = CreateInitialPattern(theEnv,theLHS->patternType);
         else tempNode = CreateInitialPattern(theEnv,defaultType);

         tempNode->logical = theLHS->logical;
         tempNode->beginNandDepth = theLHS->beginNandDepth;
         tempNode->endNandDepth = theLHS->beginNandDepth;

         if (lastNode == NULL)
           { SystemError(theEnv,"REORDER",3); }

         lastNode->bottom = tempNode;
         tempNode->bottom = theLHS;
         theLHS = tempNode;

         trackNode = theLHS->bottom;
        }

      /*===================================================*/
      /* If a pattern CE is found, then propagate its type */
      /* to following test CEs in the same lexical scope.  */
      /*===================================================*/

      else
        { trackNode = theLHS->bottom; }

      /*=======================================================*/
      /* Mark the pattern type of a "test" or "not" CE as the  */
      /* same type as the last pattern CE found that is within */
      /* the same lexical scope as the "test" or "not" CE.     */
      /*=======================================================*/

      for (;
           trackNode != NULL;
           trackNode = trackNode->bottom)
        {
         /*=====================================*/
         /* If the CE isn't in the same lexical */
         /* scope, move on to the next CE.      */
         /*=====================================*/

         if (trackNode->beginNandDepth != theLHS->beginNandDepth)
           { continue; }

         /*=======================================================*/
         /* Mark a negated CE that is in the same lexical scope.  */
         /* This signifies that there is a preceeding non-negated */
         /* pattern and thus no need for an initial pattern to be */
         /* placed before this CE.                                */
         /*=======================================================*/

         else if (trackNode->negated)
           { trackNode->marked = TRUE; }

         /*====================================================*/
         /* If another non-negated pattern in the same lexical */
         /* scope if found, stop propagation of the current    */
         /* pattern type.                                      */
         /*====================================================*/

         else if (trackNode->type == PATTERN_CE)
           { break; }

         /*====================================================*/
         /* A "test" CE in the same lexical scope is marked to */
         /* indicate that it has a non-negated pattern that it */
         /* can be attached to.                                */
         /*====================================================*/

         else if (trackNode->type == TEST_CE)
           {
            trackNode->marked = TRUE;
            trackNode->patternType = theLHS->patternType;
           }
        }

      /*====================================*/
      /* Move on to the next CE in the LHS. */
      /*====================================*/

      lastNode = theLHS;
      theLHS = theLHS->bottom;
     }
  }

/**********************************************/
/* PrintNodes: Debugging routine which prints */
/*   the representation of a CE.              */
/**********************************************/
static void PrintNodes(
  void *theEnv,
  char *fileid,
  struct lhsParseNode *theNode)
  {
   if (theNode == NULL) return;

   while (theNode != NULL)
     {
      switch (theNode->type)
        {
         case PATTERN_CE:
           EnvPrintRouter(theEnv,fileid,"(");
           if (theNode->negated) EnvPrintRouter(theEnv,fileid,"n");
           if (theNode->logical) EnvPrintRouter(theEnv,fileid,"l");
           PrintLongInteger(theEnv,fileid,(long) theNode->beginNandDepth);
           EnvPrintRouter(theEnv,fileid,"-");
           PrintLongInteger(theEnv,fileid,(long) theNode->endNandDepth);
           EnvPrintRouter(theEnv,fileid," ");
           EnvPrintRouter(theEnv,fileid,ValueToString(theNode->right->bottom->value));
           EnvPrintRouter(theEnv,fileid,")");
           break;

         case TEST_CE:
           EnvPrintRouter(theEnv,fileid,"(test ");
           PrintLongInteger(theEnv,fileid,(long) theNode->beginNandDepth);
           EnvPrintRouter(theEnv,fileid,"-");
           PrintLongInteger(theEnv,fileid,(long) theNode->endNandDepth);
           EnvPrintRouter(theEnv,fileid,")");
           break;

         case NOT_CE:
           if (theNode->logical) EnvPrintRouter(theEnv,fileid,"(lnot ");
           else EnvPrintRouter(theEnv,fileid,"(not ");;
           PrintNodes(theEnv,fileid,theNode->right);
           EnvPrintRouter(theEnv,fileid,")");
           break;

         case OR_CE:
           if (theNode->logical) EnvPrintRouter(theEnv,fileid,"(lor ");
           else EnvPrintRouter(theEnv,fileid,"(or ");
           PrintNodes(theEnv,fileid,theNode->right);
           EnvPrintRouter(theEnv,fileid,")");
           break;

         case AND_CE:
           if (theNode->logical) EnvPrintRouter(theEnv,fileid,"(land ");
           else EnvPrintRouter(theEnv,fileid,"(and ");
           PrintNodes(theEnv,fileid,theNode->right);
           EnvPrintRouter(theEnv,fileid,")");
           break;

         default:
           EnvPrintRouter(theEnv,fileid,"(unknown)");
           break;

        }

      theNode = theNode->bottom;
      if (theNode != NULL) EnvPrintRouter(theEnv,fileid," ");
     }

   return;
  }

/*************************************************************/
/* AssignPatternIndices: For each pattern CE in the LHS of a */
/*   rule, determines the pattern index for the CE. A simple */
/*   1 to N numbering can't be used since a join from the    */
/*   right only counts as a single CE to other CEs outside   */
/*   the lexical scope of the join from the right. For       */
/*   example, the patterns in the following LHS              */
/*                                                           */
/*     (a) (not (b) (c) (d)) (e)                             */
/*                                                           */
/*   would be assigned the following numbers: a-1, b-2, c-3, */
/*   d-4, and e-3.                                           */
/*************************************************************/
static struct lhsParseNode *AssignPatternIndices(
  struct lhsParseNode *theLHS,
  short startIndex)
  {
   int depth;
   struct lhsParseNode *theField;

   depth = theLHS->beginNandDepth;

   /*====================================*/
   /* Loop through the CEs at this level */
   /* assigning each CE a pattern index. */
   /*====================================*/

   while (theLHS != NULL)
     {
      /*============================================================*/
      /* If we're entering a group of CEs requiring a join from the */
      /* right, compute the pattern indices for that group and then */
      /* proceed with the next CE in this group. A join from the    */
      /* right only counts as one CE on this level regardless of    */
      /* the number of CEs it contains. If the end of this level is */
      /* encountered while processing the join from right, then     */
      /* return to the previous level.                              */
      /*============================================================*/

      if (theLHS->beginNandDepth > depth)
        {
         theLHS = AssignPatternIndices(theLHS,startIndex);
         if (theLHS->endNandDepth < depth) return(theLHS);
         startIndex++;
        }

      /*=====================================================*/
      /* A test CE is not assigned a pattern index, however, */
      /* if it is the last CE at the end of this level, then */
      /* return to the previous level.                       */
      /*=====================================================*/

      else if (theLHS->type == TEST_CE)
        { if (theLHS->endNandDepth < depth) return(theLHS); }

      /*==========================================================*/
      /* The fields of a pattern CE need to be assigned a pattern */
      /* index, field index, and/or slot names. If this CE is the */
      /* last CE at the end of this level, then return to the     */
      /* previous level.                                          */
      /*==========================================================*/

      else if (theLHS->type == PATTERN_CE)
        {
         theLHS->pattern = startIndex;
         for (theField = theLHS->right; theField != NULL; theField = theField->right)
           {
            theField->pattern = startIndex;
            PropagateIndexSlotPatternValues(theField,theField->pattern,
                                            theField->index,theField->slot,
                                            theField->slotNumber);
           }

         if (theLHS->endNandDepth < depth) return(theLHS);
         startIndex++;
        }

      /*=========================*/
      /* Move on to the next CE. */
      /*=========================*/

      theLHS = theLHS->bottom;
     }

   /*========================================*/
   /* There are no more CEs left to process. */
   /* Return to the previous level.          */
   /*========================================*/

   return(NULL);
  }

/***********************************************************/
/* PropagateIndexSlotPatternValues: Assigns pattern, field */
/*   and slot identifiers to a field in a pattern.         */
/***********************************************************/
static void PropagateIndexSlotPatternValues(
  struct lhsParseNode *theField,
  short thePattern,
  short theIndex,
  struct symbolHashNode *theSlot,
  short theSlotNumber)
  {
   struct lhsParseNode *tmpNode, *andField;

   /*=============================================*/
   /* A NULL field does not have to be processed. */
   /*=============================================*/

   if (theField == NULL) return;

   /*=====================================================*/
   /* Assign the appropriate identifiers for a multifield */
   /* slot by calling this routine recursively.           */
   /*=====================================================*/

   if (theField->multifieldSlot)
     {
      theField->pattern = thePattern;
      if (theIndex > 0) theField->index = theIndex;
      theField->slot = theSlot;
      theField->slotNumber = theSlotNumber;

      for (tmpNode = theField->bottom;
           tmpNode != NULL;
           tmpNode = tmpNode->right)
        {
         tmpNode->pattern = thePattern;
         tmpNode->slot = theSlot;
         PropagateIndexSlotPatternValues(tmpNode,thePattern,tmpNode->index,
                                         theSlot,theSlotNumber);
        }

      return;
     }

   /*=======================================================*/
   /* Loop through each of the or'ed constraints (connected */
   /* by a |) in this field of the pattern.                 */
   /*=======================================================*/

   for (theField = theField->bottom;
        theField != NULL;
        theField = theField->bottom)
     {
      /*===========================================================*/
      /* Loop through each of the and'ed constraints (connected by */
      /* a &) in this field of the pattern. Assign the pattern,    */
      /* field, and slot identifiers.                              */
      /*===========================================================*/

      for (andField = theField; andField != NULL; andField = andField->right)
        {
         andField->pattern = thePattern;
         if (theIndex > 0) andField->index = theIndex;
         andField->slot = theSlot;
         andField->slotNumber = theSlotNumber;
        }
     }
   }

#endif

