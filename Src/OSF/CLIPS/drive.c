   /*******************************************************/
   /*      "C" Language Integrated Production System      */
   /*                                                     */
   /*             CLIPS Version 6.24  05/17/06            */
   /*                                                     */
   /*                    DRIVE MODULE                     */
   /*******************************************************/

/*************************************************************/
/* Purpose: Handles join network activity associated with    */
/*   with the addition of a data entity such as a fact or    */
/*   instance.                                               */
/*                                                           */
/* Principal Programmer(s):                                  */
/*      Gary D. Riley                                        */
/*                                                           */
/* Contributing Programmer(s):                               */
/*                                                           */
/* Revision History:                                         */
/*      6.23: Correction for FalseSymbol/TrueSymbol. DR0859  */
/*                                                           */
/*      6.24: Removed INCREMENTAL_RESET and                  */
/*            LOGICAL_DEPENDENCIES compilation flags.        */
/*                                                           */
/*            Renamed BOOLEAN macro type to intBool.         */
/*                                                           */
/*            Rule with exists CE has incorrect activation.  */
/*            DR0867                                         */
/*                                                           */
/*************************************************************/

#define _DRIVE_SOURCE_

#include <stdio.h>
#define _STDIO_INCLUDED_
#include <stdlib.h>

#include "setup.h"

#if DEFRULE_CONSTRUCT

#include "agenda.h"
#include "constant.h"
#include "engine.h"
#include "envrnmnt.h"
#include "memalloc.h"
#include "prntutil.h"
#include "reteutil.h"
#include "retract.h"
#include "router.h"
#include "lgcldpnd.h"
#include "incrrset.h"

#include "drive.h"

/***************************************/
/* LOCAL INTERNAL FUNCTION DEFINITIONS */
/***************************************/

   static void                    PPDrive(void *,struct partialMatch *,struct partialMatch *,struct joinNode *);
   static void                    PNRDrive(void *,struct joinNode *,struct partialMatch *,
                                           struct partialMatch *);
   static void                    EmptyDrive(void *,struct joinNode *,struct partialMatch *);
   static void                    JoinNetErrorMessage(void *,struct joinNode *);

/************************************************/
/* NetworkAssert: Primary routine for filtering */
/*   a partial match through the join network.  */
/************************************************/
globle void NetworkAssert(
  void *theEnv,
  struct partialMatch *binds,
  struct joinNode *join,
  int enterDirection)
  {
   struct partialMatch *lhsBinds = NULL, *rhsBinds = NULL;
   struct partialMatch *comparePMs = NULL, *newBinds;
   int exprResult;

   /*=========================================================*/
   /* If an incremental reset is being performed and the join */
   /* is not part of the network to be reset, then return.    */
   /*=========================================================*/

#if (! BLOAD_ONLY) && (! RUN_TIME)
   if (EngineData(theEnv)->IncrementalResetInProgress && (join->initialize == FALSE)) return;
#endif

   /*=========================================================*/
   /* If the associated LHS pattern is a not CE or the join   */
   /* is a nand join, then we need an additional field in the */
   /* partial match to keep track of the pseudo fact if one   */
   /* is created. The partial match is automatically stored   */
   /* in the beta memory and the counterf slot is used to     */
   /* determine if it is an actual partial match. If counterf */
   /* is TRUE, there are one or more fact or instances        */
   /* keeping the not or nand join from being satisfied.      */
   /*=========================================================*/

   if ((enterDirection == LHS) &&
       ((join->patternIsNegated) || (join->joinFromTheRight)))
     {
      newBinds = AddSingleMatch(theEnv,binds,NULL,
                                (join->ruleToActivate == NULL) ? 0 : 1,
                                (int) join->logicalJoin);
      newBinds->notOriginf = TRUE;
      newBinds->counterf = TRUE;
      binds = newBinds;
      binds->next = join->beta;
      join->beta = binds;
     }

   /*==================================================*/
   /* Use a special routine if this is the first join. */
   /*==================================================*/

   if (join->firstJoin)
     {
      EmptyDrive(theEnv,join,binds);
      return;
     }

   /*==================================================*/
   /* Initialize some variables used to indicate which */
   /* side is being compared to the new partial match. */
   /*==================================================*/

   if (enterDirection == LHS)
     {
      if (join->joinFromTheRight)
        { comparePMs = ((struct joinNode *) join->rightSideEntryStructure)->beta;}
      else
        { comparePMs = ((struct patternNodeHeader *) join->rightSideEntryStructure)->alphaMemory; }
      lhsBinds = binds;
     }
   else if (enterDirection == RHS)
     {
      if (join->patternIsNegated || join->joinFromTheRight)
        { comparePMs = join->beta; }
      else
        { comparePMs = join->lastLevel->beta; }
      rhsBinds = binds;
     }
   else
     {
      SystemError(theEnv,"DRIVE",1);
      EnvExitRouter(theEnv,EXIT_FAILURE);
     }

   /*===================================================*/
   /* Compare each set of binds on the opposite side of */
   /* the join with the set of binds that entered this  */
   /* join. If the binds don't mismatch, then perform   */
   /* the appropriate action for the logic of the join. */
   /*===================================================*/

   while (comparePMs != NULL)
     {
      /*===========================================================*/
      /* Initialize some variables pointing to the partial matches */
      /* in the LHS and RHS of the join. In addition, check for    */
      /* certain conditions under which the partial match can be   */
      /* skipped since it's not a "real" partial match.            */
      /*===========================================================*/

      if (enterDirection == RHS)
        {
         lhsBinds = comparePMs;

         /*=====================================================*/
         /* The partial matches entering from the LHS of a join */
         /* are stored in the beta memory of the previous join  */
         /* (unless the current join is a join from the right   */
         /* or is attached to a not CE). If the previous join   */
         /* is a join from the right or associated with a not   */
         /* CE, then some of its partial matches in its beta    */
         /* memory will not be "real" partial matches. That is, */
         /* there may be a partial match in the alpha memory    */
         /* that prevents the partial match from satisfying the */
         /* join's conditions. If this is the case, then the    */
         /* counterf flag in the partial match will be set to   */
         /* TRUE and in this case, we move on to the next       */
         /* partial match to be checked.                        */
         /*=====================================================*/

         if (lhsBinds->counterf &&
             (join->patternIsNegated == FALSE) &&
             (join->joinFromTheRight == FALSE))
           {
            comparePMs = comparePMs->next;
            continue;
           }

        /*==================================================*/
        /* If the join is associated with a not CE or has a */
        /* join from the right, then the LHS partial match  */
        /* currently being checked may already have a       */
        /* partial match from the alpha memory preventing   */
        /* it from being satisfied. If this is the case,    */
        /* then move on to the next partial match in the    */
        /* beta memory of the join.                         */
        /*==================================================*/

        if ((join->patternIsNegated || join->joinFromTheRight) &&
            (lhsBinds->counterf))
          {
           comparePMs = comparePMs->next;
           continue;
          }
        }
      else
        { rhsBinds = comparePMs; }

      /*========================================================*/
      /* If the join has no expression associated with it, then */
      /* the new partial match derived from the LHS and RHS     */
      /* partial matches is valid. In the event that the join   */
      /* is a join from the right, it must also be checked that */
      /* the RHS partial match is the same partial match that   */
      /* the LHS partial match was generated from. Each LHS     */
      /* partial match in a join from the right corresponds     */
      /* uniquely to a partial match from the RHS of the join.  */
      /* To determine whether the LHS partial match is the one  */
      /* associated with the RHS partial match, we compare the  */
      /* the entity addresses found in the partial matches to   */
      /* make sure they're equal.                               */
      /*========================================================*/

      if (join->networkTest == NULL)
        {
         exprResult = TRUE;
         if (join->joinFromTheRight)
           {
            int i;

            for (i = 0; i < (int) (lhsBinds->bcount - 1); i++)
              {
               if (lhsBinds->binds[i].gm.theMatch != rhsBinds->binds[i].gm.theMatch)
                 {
                  exprResult = FALSE;
                  break;
                 }
              }
           }
        }

      /*=========================================================*/
      /* If the join has an expression associated with it, then  */
      /* evaluate the expression to determine if the new partial */
      /* match derived from the LHS and RHS partial matches is   */
      /* valid (i.e. variable bindings are consistent and        */
      /* predicate expressions evaluate to TRUE).                */
      /*=========================================================*/

      else
        {
         exprResult = EvaluateJoinExpression(theEnv,join->networkTest,lhsBinds,rhsBinds,join);
         if (EvaluationData(theEnv)->EvaluationError)
           {
            if (join->patternIsNegated) exprResult = TRUE;
            SetEvaluationError(theEnv,FALSE);
           }
        }

      /*====================================================*/
      /* If the join expression evaluated to TRUE (i.e.     */
      /* there were no conflicts between variable bindings, */
      /* all tests were satisfied, etc.), then perform the  */
      /* appropriate action given the logic of this join.   */
      /*====================================================*/

      if (exprResult != FALSE)
        {
         /*==============================================*/
         /* Use the PPDrive routine when the join isn't  */
         /* associated with a not CE and it doesn't have */
         /* a join from the right.                       */
         /*==============================================*/

         if ((join->patternIsNegated == FALSE) &&
             (join->joinFromTheRight == FALSE))
           { PPDrive(theEnv,lhsBinds,rhsBinds,join); }

         /*=====================================================*/
         /* Use the PNRDrive routine when the new partial match */
         /* enters from the RHS of the join and the join either */
         /* is associated with a not CE or has a join from the  */
         /* right.                                              */
         /*=====================================================*/

         else if (enterDirection == RHS)
           { PNRDrive(theEnv,join,comparePMs,rhsBinds); }

         /*===========================================================*/
         /* If the new partial match entered from the LHS of the join */
         /* and the join is either associated with a not CE or the    */
         /* join has a join from the right, then mark the LHS partial */
         /* match indicating that there is a RHS partial match        */
         /* preventing this join from being satisfied. Once this has  */
         /* happened, the other RHS partial matches don't have to be  */
         /* tested since it only takes one partial match to prevent   */
         /* the LHS from being satisfied.                             */
         /*===========================================================*/

         else if (enterDirection == LHS)
           {
            binds->binds[binds->bcount - 1].gm.theValue = (void *) rhsBinds;
            comparePMs = NULL;
            continue;
           }
        }

      /*====================================*/
      /* Move on to the next partial match. */
      /*====================================*/

      comparePMs = comparePMs->next;
     }

   /*==================================================================*/
   /* If a join with an associated not CE or join from the right was   */
   /* entered from the LHS side of the join, and the join expression   */
   /* failed for all sets of matches for the new bindings on the LHS   */
   /* side (there was no RHS partial match preventing the LHS partial  */
   /* match from being satisfied), then the LHS partial match appended */
   /* with an pseudo-fact that represents the instance of the not      */
   /* pattern or join from the right that was satisfied should be sent */
   /* to the joins below this join.                                    */
   /*==================================================================*/

   if ((join->patternIsNegated || join->joinFromTheRight) &&
       (enterDirection == LHS) &&
       (binds->binds[binds->bcount - 1].gm.theValue == NULL))
     { PNLDrive(theEnv,join,binds); }

   return;
  }

/*******************************************************/
/* EvaluateJoinExpression: Evaluates join expressions. */
/*   Performs a faster evaluation for join expressions */
/*   than if EvaluateExpression was used directly.     */
/*******************************************************/
globle intBool EvaluateJoinExpression(
  void *theEnv,
  struct expr *joinExpr,
  struct partialMatch *lbinds,
  struct partialMatch *rbinds,
  struct joinNode *joinPtr)
  {
   DATA_OBJECT theResult;
   int andLogic, result = TRUE;
   struct partialMatch *oldLHSBinds;
   struct partialMatch *oldRHSBinds;
   struct joinNode *oldJoin;

   /*======================================*/
   /* A NULL expression evaluates to TRUE. */
   /*======================================*/

   if (joinExpr == NULL) return(TRUE);

   /*=========================================*/
   /* Initialize some of the global variables */
   /* used when evaluating expressions.       */
   /*=========================================*/

   oldLHSBinds = EngineData(theEnv)->GlobalLHSBinds;
   oldRHSBinds = EngineData(theEnv)->GlobalRHSBinds;
   oldJoin = EngineData(theEnv)->GlobalJoin;
   EngineData(theEnv)->GlobalLHSBinds = lbinds;
   EngineData(theEnv)->GlobalRHSBinds = rbinds;
   EngineData(theEnv)->GlobalJoin = joinPtr;

   /*=====================================================*/
   /* Partial matches stored in joins that are associated */
   /* with a not CE contain an additional slot shouldn't  */
   /* be considered when evaluating expressions. Since    */
   /* joins that have joins from the right don't have any */
   /* expression, we don't have to do this for partial    */
   /* matches contained in these joins.                   */
   /*=====================================================*/

   if (joinPtr->patternIsNegated) lbinds->bcount--;

   /*====================================================*/
   /* Initialize some variables which allow this routine */
   /* to avoid calling the "and" and "or" functions if   */
   /* they are the first part of the expression to be    */
   /* evaluated. Most of the join expressions do not use */
   /* deeply nested and/or functions so this technique   */
   /* speeds up evaluation.                              */
   /*====================================================*/

   if (joinExpr->value == ExpressionData(theEnv)->PTR_AND)
     {
      andLogic = TRUE;
      joinExpr = joinExpr->argList;
     }
   else if (joinExpr->value == ExpressionData(theEnv)->PTR_OR)
     {
      andLogic = FALSE;
      joinExpr = joinExpr->argList;
     }
   else
     { andLogic = TRUE; }

   /*=========================================*/
   /* Evaluate each of the expressions linked */
   /* together in the join expression.        */
   /*=========================================*/

   while (joinExpr != NULL)
     {
      /*================================*/
      /* Evaluate a primitive function. */
      /*================================*/

      if ((EvaluationData(theEnv)->PrimitivesArray[joinExpr->type] == NULL) ?
          FALSE :
          EvaluationData(theEnv)->PrimitivesArray[joinExpr->type]->evaluateFunction != NULL)
        {
         struct expr *oldArgument;

         oldArgument = EvaluationData(theEnv)->CurrentExpression;
         EvaluationData(theEnv)->CurrentExpression = joinExpr;
         result = (*EvaluationData(theEnv)->PrimitivesArray[joinExpr->type]->evaluateFunction)(theEnv,joinExpr->value,&theResult);
         EvaluationData(theEnv)->CurrentExpression = oldArgument;
        }

      /*=============================*/
      /* Evaluate the "or" function. */
      /*=============================*/

      else if (joinExpr->value == ExpressionData(theEnv)->PTR_OR)
        {
         result = FALSE;
         if (EvaluateJoinExpression(theEnv,joinExpr,lbinds,rbinds,joinPtr) == TRUE)
           {
            if (EvaluationData(theEnv)->EvaluationError)
              {
               if (joinPtr->patternIsNegated) lbinds->bcount++;
               EngineData(theEnv)->GlobalLHSBinds = oldLHSBinds;
               EngineData(theEnv)->GlobalRHSBinds = oldRHSBinds;
               EngineData(theEnv)->GlobalJoin = oldJoin;
               return(FALSE);
              }
            result = TRUE;
           }
         else if (EvaluationData(theEnv)->EvaluationError)
           {
            if (joinPtr->patternIsNegated) lbinds->bcount++;
            EngineData(theEnv)->GlobalLHSBinds = oldLHSBinds;
            EngineData(theEnv)->GlobalRHSBinds = oldRHSBinds;
            EngineData(theEnv)->GlobalJoin = oldJoin;
            return(FALSE);
           }
        }

      /*==============================*/
      /* Evaluate the "and" function. */
      /*==============================*/

      else if (joinExpr->value == ExpressionData(theEnv)->PTR_AND)
        {
         result = TRUE;
         if (EvaluateJoinExpression(theEnv,joinExpr,lbinds,rbinds,joinPtr) == FALSE)
           {
            if (EvaluationData(theEnv)->EvaluationError)
              {
               if (joinPtr->patternIsNegated) lbinds->bcount++;
               EngineData(theEnv)->GlobalLHSBinds = oldLHSBinds;
               EngineData(theEnv)->GlobalRHSBinds = oldRHSBinds;
               EngineData(theEnv)->GlobalJoin = oldJoin;
               return(FALSE);
              }
            result = FALSE;
           }
         else if (EvaluationData(theEnv)->EvaluationError)
           {
            if (joinPtr->patternIsNegated) lbinds->bcount++;
            EngineData(theEnv)->GlobalLHSBinds = oldLHSBinds;
            EngineData(theEnv)->GlobalRHSBinds = oldRHSBinds;
            EngineData(theEnv)->GlobalJoin = oldJoin;
            return(FALSE);
           }
        }

      /*==========================================================*/
      /* Evaluate all other expressions using EvaluateExpression. */
      /*==========================================================*/

      else
        {
         EvaluateExpression(theEnv,joinExpr,&theResult);

         if (EvaluationData(theEnv)->EvaluationError)
           {
            JoinNetErrorMessage(theEnv,joinPtr);
            if (joinPtr->patternIsNegated) lbinds->bcount++;
            EngineData(theEnv)->GlobalLHSBinds = oldLHSBinds;
            EngineData(theEnv)->GlobalRHSBinds = oldRHSBinds;
            EngineData(theEnv)->GlobalJoin = oldJoin;
            return(FALSE);
           }

         if ((theResult.value == EnvFalseSymbol(theEnv)) && (theResult.type == SYMBOL))
           { result = FALSE; }
         else
           { result = TRUE; }
        }

      /*====================================*/
      /* Handle the short cut evaluation of */
      /* the "and" and "or" functions.      */
      /*====================================*/

      if ((andLogic == TRUE) && (result == FALSE))
        {
         if (joinPtr->patternIsNegated) lbinds->bcount++;
         EngineData(theEnv)->GlobalLHSBinds = oldLHSBinds;
         EngineData(theEnv)->GlobalRHSBinds = oldRHSBinds;
         EngineData(theEnv)->GlobalJoin = oldJoin;
         return(FALSE);
        }
      else if ((andLogic == FALSE) && (result == TRUE))
        {
         if (joinPtr->patternIsNegated) lbinds->bcount++;
         EngineData(theEnv)->GlobalLHSBinds = oldLHSBinds;
         EngineData(theEnv)->GlobalRHSBinds = oldRHSBinds;
         EngineData(theEnv)->GlobalJoin = oldJoin;
         return(TRUE);
        }

      /*==============================================*/
      /* Move to the next expression to be evaluated. */
      /*==============================================*/

      joinExpr = joinExpr->nextArg;
     }

   /*=======================================*/
   /* Restore some of the global variables. */
   /*=======================================*/

   EngineData(theEnv)->GlobalLHSBinds = oldLHSBinds;
   EngineData(theEnv)->GlobalRHSBinds = oldRHSBinds;
   EngineData(theEnv)->GlobalJoin = oldJoin;

   /*=====================================*/
   /* Restore the count value for the LHS */
   /* binds if it had to be modified.     */
   /*=====================================*/

   if (joinPtr->patternIsNegated) lbinds->bcount++;

   /*=================================================*/
   /* Return the result of evaluating the expression. */
   /*=================================================*/

   return(result);
  }

/*******************************************************************/
/* PPDrive: Handles the merging of an alpha memory partial match   */
/*   with a beta memory partial match for a join that has positive */
/*   LHS entry and positive RHS entry. The partial matches being   */
/*   merged have previously been checked to determine that they    */
/*   satisify the constraints for the join. Once merged, the new   */
/*   partial match is sent to each child join of the join from     */
/*   which the merge took place.                                   */
/*******************************************************************/
static void PPDrive(
  void *theEnv,
  struct partialMatch *lhsBinds,
  struct partialMatch *rhsBinds,
  struct joinNode *join)
  {
   struct partialMatch *linker;
   struct joinNode *listOfJoins;
   
   /*==================================================*/
   /* Merge the alpha and beta memory partial matches. */
   /*==================================================*/

   linker = MergePartialMatches(theEnv,lhsBinds,rhsBinds,
                                (join->ruleToActivate == NULL) ? 0 : 1,
                                (int) join->logicalJoin);

   /*=======================================================*/
   /* Add the partial match to the beta memory of the join. */
   /*=======================================================*/

   linker->next = join->beta;
   join->beta = linker;

   /*====================================================*/
   /* Activate the rule satisfied by this partial match. */
   /*====================================================*/

   if (join->ruleToActivate != NULL) AddActivation(theEnv,join->ruleToActivate,linker);

   /*================================================*/
   /* Send the new partial match to all child joins. */
   /*================================================*/

   listOfJoins = join->nextLevel;
   if (listOfJoins != NULL)
     {
      if (((struct joinNode *) (listOfJoins->rightSideEntryStructure)) == join)
        { NetworkAssert(theEnv,linker,listOfJoins,RHS); }
      else while (listOfJoins != NULL)
        {
         NetworkAssert(theEnv,linker,listOfJoins,LHS);
         listOfJoins = listOfJoins->rightDriveNode;
        }
     }

   return;
  }

/**********************************************************************/
/* PNRDrive: Handles the entry of a partial match from the RHS of a   */
/*   join that has positive LHS entry and negative RHS entry (meaning */
/*   the conditional element associated with this join is a not       */
/*   conditional element). Entry of the alpha memory partial match    */
/*   will cause the counterf value of the associated beta memory      */
/*   partial match to be set. This in turn may cause partial matches  */
/*   associated with the beta memory partial match to be removed from */
/*   the network.                                                     */
/**********************************************************************/
static void PNRDrive(
  void *theEnv,
  struct joinNode *join,
  struct partialMatch *lhsBinds,
  struct partialMatch *rhsBinds)
  {
   struct joinNode *listOfJoins;

   /*==================================================*/
   /* If the partial match already has a partial match */
   /* in the alpha memory which prevents it from being */
   /* satisfied, then don't do anything.               */
   /*==================================================*/

   if (lhsBinds->counterf == TRUE) return;

   /*=================================================*/
   /* Set the counterf flag to indicate that an alpha */
   /* memory partial match is preventing the beta     */
   /* memory partial match from being satisfied.      */
   /*=================================================*/

   lhsBinds->counterf = TRUE;

   /*===================================================================*/
   /* If the partial match caused an activation, remove the activation. */
   /*===================================================================*/

   if ((lhsBinds->activationf) ?
       (lhsBinds->binds[lhsBinds->bcount].gm.theValue != NULL) : FALSE)
     { RemoveActivation(theEnv,(struct activation *) lhsBinds->binds[lhsBinds->bcount].gm.theValue,TRUE,TRUE); }

   /*===========================================================*/
   /* The counterf flag was FALSE. This means that a pointer to */
   /* the pseudo-fact matching the not CE is stored directly in */
   /* the partial match. Determine the ID of this pseudo-fact   */
   /* and remove all partial matches from descendent joins that */
   /* contain the ID.                                           */
   /*===========================================================*/

   if (join->joinFromTheRight) /* GDR 111599 #834 Begin */
     {
      RetractCheckDriveRetractions(theEnv,lhsBinds->binds[lhsBinds->bcount - 1].gm.theMatch,
                                   (int) join->depth-1);
     }                         /* GDR 111599 #834 End */
     
   listOfJoins = join->nextLevel;
   if (listOfJoins != NULL)
     {
      if (((struct joinNode *) (listOfJoins->rightSideEntryStructure)) == join)
        { NegEntryRetract(theEnv,listOfJoins,lhsBinds,NULL); }
      else while (listOfJoins != NULL)
        {

         PosEntryRetract(theEnv,listOfJoins,
                         lhsBinds->binds[lhsBinds->bcount - 1].gm.theMatch,
                         lhsBinds,(int) join->depth-1,NULL);
         listOfJoins = listOfJoins->rightDriveNode;
        }
     }

   /*=========================================================================*/
   /* Remove any logical dependency links associated with this partial match. */
   /*=========================================================================*/

   if (lhsBinds->dependentsf) RemoveLogicalSupport(theEnv,lhsBinds);

   /*========================================*/
   /* Put the pseudo-fact on a garbage list. */
   /*========================================*/

   lhsBinds->binds[lhsBinds->bcount - 1].gm.theMatch->next = EngineData(theEnv)->GarbageAlphaMatches;
   EngineData(theEnv)->GarbageAlphaMatches = lhsBinds->binds[lhsBinds->bcount - 1].gm.theMatch;

   /*========================================================*/
   /* Store the partial match from the alpha memory that is  */
   /* preventing the LHS partial match from being satisfied. */
   /*========================================================*/

   lhsBinds->binds[lhsBinds->bcount - 1].gm.theValue = (void *) rhsBinds;
  }

/********************************************************************/
/* PNLDrive: Handles the entry of a partial match from the LHS of a */
/*   join that has positive LHS entry and negative RHS entry        */
/*   (meaning the conditional element associated with this join is  */
/*   a not conditional element). An new partial match is created by */
/*   combining the match from the beta memory with a "pseudo"       */
/*   partial match corresponding to the facts which didn't match    */
/*   the not CE. Once merged, the new partial match is sent to each */
/*   child join of the join from which the merge took place.        */
/********************************************************************/
globle void PNLDrive(
  void *theEnv,
  struct joinNode *join,
  struct partialMatch *binds)
  {
   struct joinNode *listOfJoins;
   struct alphaMatch *tempAlpha;

    /*=======================================================*/
    /* Create a pseudo-fact representing the facts which did */
    /* not match the not CE associated with this join.       */
    /*=======================================================*/

    tempAlpha = get_struct(theEnv,alphaMatch);
    tempAlpha->next = NULL;
    tempAlpha->matchingItem = NULL;
    tempAlpha->markers = NULL;

    /*===============================================*/
    /* Store the pointer to the pseudo-fact directly */
    /* in the beta memory partial match.             */
    /*===============================================*/

    binds->counterf = FALSE;
    binds->binds[binds->bcount - 1].gm.theMatch = tempAlpha;

   /*====================================================*/
   /* Activate the rule satisfied by this partial match. */
   /*====================================================*/

   if (join->ruleToActivate != NULL) AddActivation(theEnv,join->ruleToActivate,binds);

    /*========================================================*/
    /* Send the merged partial match to all descendent joins. */
    /*========================================================*/

    listOfJoins = join->nextLevel;
    if (listOfJoins != NULL)
      {
       if (((struct joinNode *) (listOfJoins->rightSideEntryStructure)) == join)
         { NetworkAssert(theEnv,binds,listOfJoins,RHS); }
       else while (listOfJoins != NULL)
         {
          NetworkAssert(theEnv,binds,listOfJoins,LHS);
          listOfJoins = listOfJoins->rightDriveNode;
         }
      }
   }

/***************************************************************/
/* EmptyDrive: Handles the entry of a alpha memory partial     */
/*   match from the RHS of a join that is the first join of    */
/*   a rule (i.e. a join that cannot be entered from the LHS). */
/***************************************************************/
static void EmptyDrive(
  void *theEnv,
  struct joinNode *join,
  struct partialMatch *rhsBinds)
  {
   struct partialMatch *linker;
   struct joinNode *listOfJoins;
   int joinExpr;

   /*======================================================*/
   /* Determine if the alpha memory partial match satifies */
   /* the join expression. If it doesn't then no further   */
   /* action is taken.                                     */
   /*======================================================*/

   if (join->networkTest != NULL)
     {
      joinExpr = EvaluateJoinExpression(theEnv,join->networkTest,NULL,rhsBinds,join);
      EvaluationData(theEnv)->EvaluationError = FALSE;
      if (joinExpr == FALSE) return;
     }

   /*===========================================================*/
   /* The first join of a rule cannot be connected to a NOT CE. */
   /*===========================================================*/

   if (join->patternIsNegated == TRUE)
     {
      SystemError(theEnv,"DRIVE",2);
      EnvExitRouter(theEnv,EXIT_FAILURE);
     }

   /*=========================================================*/
   /* If the join's RHS entry is associated with a pattern CE */
   /* (positive entry), then copy the alpha memory partial    */
   /* match and send it to all child joins.                   */
   /*=========================================================*/

   linker = CopyPartialMatch(theEnv,rhsBinds,
                             (join->ruleToActivate == NULL) ? 0 : 1,
                             (int) join->logicalJoin);

   /*=======================================================*/
   /* Add the partial match to the beta memory of the join. */
   /*=======================================================*/

   linker->next = join->beta;
   join->beta = linker;

   /*====================================================*/
   /* Activate the rule satisfied by this partial match. */
   /*====================================================*/

   if (join->ruleToActivate != NULL) AddActivation(theEnv,join->ruleToActivate,linker);

   /*============================================*/
   /* Send the partial match to all child joins. */
   /*============================================*/

   listOfJoins = join->nextLevel;
   while (listOfJoins != NULL)
     {
      NetworkAssert(theEnv,linker,listOfJoins,LHS);
      listOfJoins = listOfJoins->rightDriveNode;
     }
  }

/********************************************************************/
/* JoinNetErrorMessage: Prints an informational message indicating  */
/*   which join of a rule generated an error when a join expression */
/*   was being evaluated.                                           */
/********************************************************************/
static void JoinNetErrorMessage(
  void *theEnv,
  struct joinNode *joinPtr)
  {
   char buffer[60];

   PrintErrorID(theEnv,"DRIVE",1,TRUE);
   EnvPrintRouter(theEnv,WERROR,"This error occurred in the join network\n");

   sprintf(buffer,"   Problem resides in join #%d in rule(s):\n",joinPtr->depth);
   EnvPrintRouter(theEnv,WERROR,buffer);

   TraceErrorToRule(theEnv,joinPtr,"      ");
   EnvPrintRouter(theEnv,WERROR,"\n");
  }

#endif /* DEFRULE_CONSTRUCT */
