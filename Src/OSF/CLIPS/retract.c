   /*******************************************************/
   /*      "C" Language Integrated Production System      */
   /*                                                     */
   /*             CLIPS Version 6.24  05/17/06            */
   /*                                                     */
   /*                   RETRACT MODULE                    */
   /*******************************************************/

/*************************************************************/
/* Purpose:  Handles join network activity associated with   */
/*   with the removal of a data entity such as a fact or     */
/*   instance.                                               */
/*                                                           */
/* Principal Programmer(s):                                  */
/*      Gary D. Riley                                        */
/*                                                           */
/* Contributing Programmer(s):                               */
/*                                                           */
/* Revision History:                                         */
/*                                                           */
/*      6.24: Removed LOGICAL_DEPENDENCIES compilation flag. */
/*                                                           */
/*            Renamed BOOLEAN macro type to intBool.         */
/*                                                           */
/*            Rule with exists CE has incorrect activation.  */
/*            DR0867                                         */
/*                                                           */
/*************************************************************/

#define _RETRACT_SOURCE_

#include <stdio.h>
#define _STDIO_INCLUDED_
#include <stdlib.h>

#include "setup.h"

#if DEFRULE_CONSTRUCT

#include "agenda.h"
#include "argacces.h"
#include "constant.h"
#include "drive.h"
#include "engine.h"
#include "envrnmnt.h"
#include "lgcldpnd.h"
#include "match.h"
#include "memalloc.h"
#include "network.h"
#include "reteutil.h"
#include "router.h"
#include "symbol.h"

#include "retract.h"

/***************************************/
/* LOCAL INTERNAL FUNCTION DEFINITIONS */
/***************************************/

   static struct partialMatch    *RemovePartialMatches(void *,struct alphaMatch *,
                                                      struct partialMatch *,
                                                      struct partialMatch **,int,
                                                      struct partialMatch **);
   static void                    DeletePartialMatches(void *,struct partialMatch *,int);
   static void                    ReturnMarkers(void *,struct multifieldMarker *);
   static void                    DriveRetractions(void *);
   static intBool                 FindNextConflictingAlphaMatch(void *,struct partialMatch *,
                                                                struct partialMatch *,
                                                                struct joinNode *);
   static intBool                 PartialMatchDefunct(void *,struct partialMatch *);

/************************************************************/
/* NetworkRetract:  Retracts a data entity (such as a fact  */
/*   or instance) from the pattern and join networks given  */
/*   a pointer to the list of patterns which the data       */
/*   entity matched. The data entity is first removed from  */
/*   the join network through patterns not directly         */
/*   enclosed within a not CE and then through patterns     */
/*   enclosed by a not CE. Any new partial matches created  */
/*   by the removal are then filtered through the join      */
/*   network. This ordering prevents partial matches from   */
/*   being generated that contain the data entity which was */
/*   removed.                                               */
/************************************************************/
globle void NetworkRetract(
  void *theEnv,
  struct patternMatch *listOfMatchedPatterns)
  {
   struct patternMatch *tempMatch;
   struct partialMatch *deletedMatches, *theLast;
   struct joinNode *joinPtr;

   /*===============================*/
   /* Remember the beginning of the */
   /* list of matched patterns.     */
   /*===============================*/

   tempMatch = listOfMatchedPatterns;

   /*============================================*/
   /* Remove the data entity from all joins that */
   /* aren't directly enclosed by a not CE.      */
   /*============================================*/

   for (;
        listOfMatchedPatterns != NULL;
        listOfMatchedPatterns = listOfMatchedPatterns->next)
     {
      /*====================================*/
      /* Loop through the list of all joins */
      /* attached to this pattern.          */
      /*====================================*/

      for (joinPtr = listOfMatchedPatterns->matchingPattern->entryJoin;
           joinPtr != NULL;
           joinPtr = joinPtr->rightMatchNode)
        {
         if (joinPtr->patternIsNegated == FALSE)
           { PosEntryRetract(theEnv,joinPtr,
                             listOfMatchedPatterns->theMatch->binds[0].gm.theMatch,
                             listOfMatchedPatterns->theMatch,
                             (int) joinPtr->depth - 1,listOfMatchedPatterns->theMatch->binds[0].gm.theMatch->matchingItem); }
        }
     }

   /*============================================*/
   /* Remove the data entity from all joins that */
   /* are directly enclosed by a not CE.         */
   /*============================================*/

   listOfMatchedPatterns = tempMatch;
   while (listOfMatchedPatterns != NULL)
     {
      /*====================================*/
      /* Loop through the list of all joins */
      /* attached to this pattern.          */
      /*====================================*/

      for (joinPtr = listOfMatchedPatterns->matchingPattern->entryJoin;
           joinPtr != NULL;
           joinPtr = joinPtr->rightMatchNode)
        {
         if (joinPtr->patternIsNegated == TRUE)
           {
            if (joinPtr->firstJoin == TRUE)
              {
               SystemError(theEnv,"RETRACT",3);
               EnvExitRouter(theEnv,EXIT_FAILURE);
              }
            else
              { 
               NegEntryRetract(theEnv,joinPtr,listOfMatchedPatterns->theMatch,listOfMatchedPatterns->theMatch->binds[0].gm.theMatch->matchingItem); 
              }
           }
        }

      /*===================================================*/
      /* Remove from the alpha memory of the pattern node. */
      /*===================================================*/

      theLast = NULL;
      listOfMatchedPatterns->matchingPattern->alphaMemory =
      RemovePartialMatches(theEnv,listOfMatchedPatterns->theMatch->binds[0].gm.theMatch,
                                listOfMatchedPatterns->matchingPattern->alphaMemory,
                                &deletedMatches,0,&theLast);
      listOfMatchedPatterns->matchingPattern->endOfQueue = theLast;

      DeletePartialMatches(theEnv,deletedMatches,0);

      tempMatch = listOfMatchedPatterns->next;
      rtn_struct(theEnv,patternMatch,listOfMatchedPatterns);
      listOfMatchedPatterns = tempMatch;
     }

   /*=========================================*/
   /* Filter new partial matches generated by */
   /* retraction through the join network.    */
   /*=========================================*/

   DriveRetractions(theEnv);
  }

/***************************************************************/
/* PosEntryRetract:  Handles retract for a join of a rule with */
/*    a positive pattern when the retraction is starting from  */
/*    the RHS of that join (empty or positive LHS entry,       */
/*    positive RHS entry), or the LHS of that join (positive   */
/*    LHS entry, negative or positive RHS entry).              */
/***************************************************************/
globle void PosEntryRetract(
  void *theEnv,
  struct joinNode *join,
  struct alphaMatch *theAlphaNode,
  struct partialMatch *theMatch,
  int position,
  void *duringRetract)
  {
   struct partialMatch *deletedMatches;
   struct joinNode *joinPtr;
   struct partialMatch *theLast;

   while (join != NULL)
     {
      /*=========================================*/
      /* Remove the bindings from this join that */
      /* contain the fact to be retracted.       */
      /*=========================================*/

      if (join->beta == NULL) return; /* optimize */

      join->beta = RemovePartialMatches(theEnv,theAlphaNode,join->beta,&deletedMatches,
                                        position,&theLast);

      /*===================================================*/
      /* If no facts were deleted at this join, then there */
      /* is no need to check joins at a lower level.       */
      /*===================================================*/

      if (deletedMatches == NULL) return;

      /*==================================================*/
      /* If there is more than one join below this join,  */
      /* then recursively remove fact bindings from all   */
      /* but one of the lower joins.  Remove the bindings */
      /* from the other join through this loop.           */
      /*==================================================*/

      joinPtr = join->nextLevel;
      if (joinPtr == NULL)
        {
         DeletePartialMatches(theEnv,deletedMatches,1);
         return;
        }

      if (((struct joinNode *) (joinPtr->rightSideEntryStructure)) == join)
        {
         theMatch = deletedMatches;
         while (theMatch != NULL)
           {
            NegEntryRetract(theEnv,joinPtr,theMatch,duringRetract);
            theMatch = theMatch->next;
           }

         DeletePartialMatches(theEnv,deletedMatches,1);
         return;
        }

      DeletePartialMatches(theEnv,deletedMatches,1);
      while (joinPtr->rightDriveNode != NULL)
        {
         PosEntryRetract(theEnv,joinPtr,theAlphaNode,theMatch,position,duringRetract);
         joinPtr = joinPtr->rightDriveNode;
        }

      join = joinPtr;
     }
  }

/*****************************************************************/
/* NegEntryRetract:  Handles retract for a join of a rule with a */
/*    not CE when the retraction is process from the RHS of that */
/*    join.                                                      */
/*****************************************************************/
void NegEntryRetract(
  void *theEnv,
  struct joinNode *theJoin,
  struct partialMatch *theMatch,
  void *duringRetract)
  {
   struct partialMatch *theLHS;
   int result;
   struct rdriveinfo *tempDR;
   struct alphaMatch *tempAlpha;
   struct joinNode *listOfJoins;

   /*===============================================*/
   /* Loop through all LHS partial matches checking */
   /* for sets that satisfied the join expression.  */
   /*===============================================*/

   for (theLHS = theJoin->beta; theLHS != NULL; theLHS = theLHS->next)
     {
      /*===========================================================*/
      /* Don't bother checking partial matches that are satisfied. */
      /* We're looking for joins from which the removal of a       */
      /* partial match would satisfy the join.                     */
      /*===========================================================*/

      if (theLHS->counterf == FALSE) continue;

      /*==================================================*/
      /* If the partial match being removed isn't the one */
      /* preventing the LHS partial match from being      */
      /* satisifed, then don't bother processing it.      */
      /*==================================================*/

      if (theLHS->binds[theLHS->bcount - 1].gm.theValue != (void *) theMatch) continue;

      /*======================================================*/
      /* Try to find another RHS partial match which prevents */
      /* the LHS partial match from being satisifed.          */
      /*======================================================*/

      theLHS->binds[theLHS->bcount - 1].gm.theValue = NULL;
      result = FindNextConflictingAlphaMatch(theEnv,theLHS,theMatch->next,theJoin);

      /*=========================================================*/
      /* If the LHS partial match now has no RHS partial matches */
      /* that conflict with it, then it satisfies the conditions */
      /* of the RHS not CE. Create a partial match and send it   */
      /* to the joins below.                                     */
      /*=========================================================*/

      if (result == FALSE)
        {
         /*===============================*/
         /* Create the new partial match. */
         /*===============================*/

         theLHS->counterf = FALSE;
         tempAlpha = get_struct(theEnv,alphaMatch);
         tempAlpha->next = NULL;
         tempAlpha->matchingItem = NULL;
         tempAlpha->markers = NULL;
         theLHS->binds[theLHS->bcount - 1].gm.theMatch = tempAlpha;

         /*==============================================*/
         /* If partial matches from this join correspond */
         /* to a rule activation, then add an activation */
         /* to the agenda.                               */
         /*==============================================*/

         if (theJoin->ruleToActivate != NULL)
           { AddActivation(theEnv,theJoin->ruleToActivate,theLHS); }

         /*=======================================================*/
         /* Send the partial match to the list of joins following */
         /* this join. If we're in the middle of a retract, add   */
         /* the partial match to the list of join activities that */
         /* need to be processed later. If we're doing an assert, */
         /* then the join activity can be processed immediately.  */
         /*=======================================================*/

         listOfJoins = theJoin->nextLevel;
         if (listOfJoins != NULL)
           {
            if (((struct joinNode *) (listOfJoins->rightSideEntryStructure)) == theJoin)
              { NetworkAssert(theEnv,theLHS,listOfJoins,RHS); }
            else
              {
               if (duringRetract != NULL)
                 {
                  if (FindEntityInPartialMatch((struct patternEntity *) duringRetract,theLHS) == FALSE)
                    {
                     tempDR = get_struct(theEnv,rdriveinfo);
                     tempDR->link = theLHS;
                     tempDR->jlist = theJoin->nextLevel;
                     tempDR->next = EngineData(theEnv)->DriveRetractionList;
                     EngineData(theEnv)->DriveRetractionList = tempDR;
                    }
                 }
               else while (listOfJoins != NULL)
                 {
                  NetworkAssert(theEnv,theLHS,listOfJoins,LHS);
                  listOfJoins = listOfJoins->rightDriveNode;
                 }
              }
           }
        }
     }
  }

/**************************************************************/
/* FindNextConflictingAlphaMatch: Finds the next conflicting  */
/*   partial match in the alpha memory of a join (or the beta */
/*   memory of a join from the right) that prevents a partial */
/*   match in the beta memory of the join from being          */
/*   satisfied.                                               */
/**************************************************************/
static intBool FindNextConflictingAlphaMatch(
  void *theEnv,
  struct partialMatch *theBind,
  struct partialMatch *possibleConflicts,
  struct joinNode *theJoin)
  {
   int i, result;

   /*=====================================================*/
   /* If we're dealing with a join from the right, then   */
   /* we need to check the entire beta memory of the join */
   /* from the right (a join doesn't have an end of queue */
   /* pointer like a pattern data structure has).         */
   /*=====================================================*/

   if (theJoin->joinFromTheRight)
     { possibleConflicts = ((struct joinNode *) theJoin->rightSideEntryStructure)->beta; }

   /*====================================*/
   /* Check each of the possible partial */
   /* matches which could conflict.      */
   /*====================================*/

   for (;
        possibleConflicts != NULL;
        possibleConflicts = possibleConflicts->next)
     {
      /*=====================================*/
      /* Initially indicate that the partial */
      /* match doesn't conflict.             */
      /*=====================================*/

      result = FALSE;

      /*====================================================*/
      /* A partial match with the counterf flag set is not  */
      /* yet a "real" partial match, so ignore it. When the */
      /* counterf flag is set that means that the partial   */
      /* match is associated with a not CE that has a data  */
      /* entity preventing it from being satsified.         */
      /*====================================================*/

      if (possibleConflicts->counterf)
        { /* Do Nothing */ }

       /*======================================================*/
       /* 6.05 Bug Fix. It is possible that a pattern entity   */
       /* (e.g., instance) in a partial match is 'out of date' */
       /* with respect to the lazy evaluation scheme use by    */
       /* negated patterns. In other words, the object may     */
       /* have changed since it was last pushed through the    */
       /* network, and thus the partial match may be invalid.  */
       /* If so, the partial match must be ignored here.       */
       /*======================================================*/

      else if (PartialMatchDefunct(theEnv,possibleConflicts))
        { /* Do Nothing */ }

      /*==================================================*/
      /* If the join doesn't have a network expression to */
      /* be evaluated, then partial match conflicts. If   */
      /* the partial match is retrieved from a join from  */
      /* the right, the RHS partial match must correspond */
      /* to the partial match in the beta memory of the   */
      /* join being examined (in a join associated with a */
      /* not CE, each partial match in the beta memory of */
      /* the join corresponds uniquely to a partial match */
      /* in either the alpha memory from the RHS or in    */
      /* the beta memory of a join from the right).       */
      /*==================================================*/

      else if (theJoin->networkTest == NULL)
        {
         result = TRUE;
         if (theJoin->joinFromTheRight)
           {
            for (i = 0; i < (int) (theBind->bcount - 1); i++)
              {
               if (possibleConflicts->binds[i].gm.theMatch != theBind->binds[i].gm.theMatch)
                 {
                  result = FALSE;
                  break;
                 }
              }
           }
        }

      /*=================================================*/
      /* Otherwise, if the join has a network expression */
      /* to evaluate, then evaluate it.                  */
      /*=================================================*/

      else
        {
         result = EvaluateJoinExpression(theEnv,theJoin->networkTest,theBind,
                                         possibleConflicts,theJoin);
         if (EvaluationData(theEnv)->EvaluationError)
           {
            result = TRUE;
            EvaluationData(theEnv)->EvaluationError = FALSE;
           }
        }

      /*==============================================*/
      /* If the network expression evaluated to TRUE, */
      /* then partial match being examined conflicts. */
      /* Point the beta memory partial match to the   */
      /* conflicting partial match and return TRUE to */
      /* indicate a conflict was found.               */
      /*==============================================*/

      if (result != FALSE)
        {
         theBind->binds[theBind->bcount - 1].gm.theValue = (void *) possibleConflicts;
         return(TRUE);
        }
     }

   /*========================*/
   /* No conflict was found. */
   /*========================*/

   return(FALSE);
  }

/***********************************************************/
/* PartialMatchDefunct: Determines if any pattern entities */
/*   contained within the partial match have changed since */
/*   this partial match was generated. Assumes counterf is */
/*   FALSE.                                                */
/***********************************************************/
static intBool PartialMatchDefunct(
  void *theEnv,
  struct partialMatch *thePM)
  {
   register unsigned i;
   register struct patternEntity * thePE;

   for (i = 0 ; i < thePM->bcount ; i++)
     {
      thePE = thePM->binds[i].gm.theMatch->matchingItem;
      if (thePE && thePE->theInfo->synchronized &&
          !(*thePE->theInfo->synchronized)(theEnv,thePE))
        return(TRUE);
     }
   return(FALSE);
  }

/*************************************************************/
/* RemovePartialMatches: Searches through a list of partial  */
/*   matches and removes any partial match that contains the */
/*   specified data entity.                                  */
/*************************************************************/
static struct partialMatch *RemovePartialMatches(
  void *theEnv,
  struct alphaMatch *theAlphaNode,
  struct partialMatch *listOfPMs,
  struct partialMatch **deleteHead,
  int position,
  struct partialMatch **returnLast)
  {
   struct partialMatch *head, *lastPM, *nextPM;
   struct partialMatch *lastDelete = NULL;
   
   /*====================================================*/
   /* Initialize pointers used for creating the new list */
   /* of partial matches and the list of partial matches */
   /* to be deleted.                                     */
   /*====================================================*/

   head = listOfPMs;
   lastPM = listOfPMs;
   *deleteHead = NULL;

   /*==========================================*/
   /* Loop through each of the partial matches */
   /* and determine if it needs to be deleted. */
   /*==========================================*/

   while (listOfPMs != NULL)
     {
      if ((listOfPMs->counterf == TRUE) && (position == ((int) (listOfPMs->bcount - 1))))
        {
         lastPM = listOfPMs;
         listOfPMs = listOfPMs->next;
        }

      /*=====================================================*/
      /* Otherwise, if the specified position in the partial */
      /* match contains the specified data entity, then      */
      /* remove the partial match from the list and add it   */
      /* to a deletion list.                                 */
      /*=====================================================*/

      else if (listOfPMs->binds[position].gm.theMatch == theAlphaNode)
        {
         /*===================================================*/
         /* If the partial match has an activation associated */
         /* with it, then return the activation.              */
         /*===================================================*/

         if ((listOfPMs->activationf) ?
             (listOfPMs->binds[listOfPMs->bcount].gm.theValue != NULL) : FALSE)
           { RemoveActivation(theEnv,(struct activation *) listOfPMs->binds[listOfPMs->bcount].gm.theValue,TRUE,TRUE); }

         /*==================================================*/
         /* If the partial match is at the head of the list  */
         /* of matches, then use the following deletion code */
         /* for the head of the list.                        */
         /*==================================================*/

         if (listOfPMs == head)
           {
            /*===================================*/
            /* Remember the new beginning of the */
            /* new list of partial matches.      */
            /*===================================*/

            nextPM = listOfPMs->next;

            /*=============================================*/
            /* Add the partial match to the deletion list. */
            /*=============================================*/

            if (*deleteHead == NULL)
              { *deleteHead = listOfPMs; }
            else
              { lastDelete->next = listOfPMs; }

            listOfPMs->next = NULL;
            lastDelete = listOfPMs;

            /*================================================*/
            /* Update the head and tail pointers for the new  */
            /* list of partial matches as well as the pointer */
            /* to the next partial match to be examined.      */
            /*================================================*/

            listOfPMs = nextPM;
            head = listOfPMs;
            lastPM = head;
           }

         /*======================================*/
         /* Otherwise, use the following code to */
         /* delete the partial match.            */
         /*======================================*/

         else
           {
            /*========================================*/
            /* Detach the partial match being deleted */
            /* from the new list of partial matches.  */
            /*========================================*/

            lastPM->next = listOfPMs->next;

            /*=============================================*/
            /* Add the partial match to the deletion list. */
            /*=============================================*/

            if (*deleteHead == NULL)
              { *deleteHead = listOfPMs; }
            else
              { lastDelete->next = listOfPMs; }

            listOfPMs->next = NULL;
            lastDelete = listOfPMs;

            /*=============================*/
            /* Move on to the next partial */
            /* match to be examined.       */
            /*=============================*/

            listOfPMs = lastPM->next;
           }
        }

      /*==============================================*/
      /* Otherwise, the partial match should be added */
      /* to the new list of partial matches.          */
      /*==============================================*/

      else
        {
         lastPM = listOfPMs;
         listOfPMs = listOfPMs->next;
        }
     }

   /*===============================================*/
   /* Return the last partial match in the new list */
   /* of partial matches via one of the function's  */
   /* parameters.                                   */
   /*===============================================*/

   *returnLast = lastPM;

   /*=====================================================*/
   /* Return the head of the new list of partial matches. */
   /*=====================================================*/

   return(head);
  }

/***************************************************/
/* DeletePartialMatches: Returns a list of partial */
/*   matches to the pool of free memory.           */
/***************************************************/
static void DeletePartialMatches(
  void *theEnv,
  struct partialMatch *listOfPMs,
  int betaDelete)
  {
   struct partialMatch *nextPM;

   while (listOfPMs != NULL)
     {
      /*============================================*/
      /* Remember the next partial match to delete. */
      /*============================================*/

      nextPM = listOfPMs->next;

      /*================================================*/
      /* Remove the links between the partial match and */
      /* any data entities that it is attached to as a  */
      /* result of a logical CE.                        */
      /*================================================*/

      if (listOfPMs->dependentsf) RemoveLogicalSupport(theEnv,listOfPMs);

      /*==========================================================*/
      /* If the partial match is being deleted from a beta memory */
      /* and the partial match isn't associated with a satisfied  */
      /* not CE, then it can be immediately returned to the pool  */
      /* of free memory. Otherwise, it's could be in use (either  */
      /* to retrieve variables from the LHS or by the activation  */
      /* of the rule). Since a not CE creates a "pseudo" data     */
      /* entity, the beta partial match which stores this pseudo  */
      /* data entity can not be deleted immediately (for the same */
      /* reason an alpha memory partial match can't be deleted    */
      /* immediately).                                            */
      /*==========================================================*/

      if (betaDelete &&
          ((listOfPMs->notOriginf == FALSE) || (listOfPMs->counterf)))
        { ReturnPartialMatch(theEnv,listOfPMs); }
      else
        {
         listOfPMs->next = EngineData(theEnv)->GarbagePartialMatches;
         EngineData(theEnv)->GarbagePartialMatches = listOfPMs;
        }

      /*====================================*/
      /* Move on to the next partial match. */
      /*====================================*/

      listOfPMs = nextPM;
     }
  }

/**************************************************************/
/* ReturnPartialMatch: Returns the data structures associated */
/*   with a partial match to the pool of free memory.         */
/**************************************************************/
globle void ReturnPartialMatch(
  void *theEnv,
  struct partialMatch *waste)
  {
   /*==============================================*/
   /* If the partial match is in use, then put it  */
   /* on a garbage list to be processed later when */
   /* the partial match is not in use.             */
   /*==============================================*/

   if (waste->busy)
     {
      waste->next = EngineData(theEnv)->GarbagePartialMatches;
      EngineData(theEnv)->GarbagePartialMatches = waste;
      return;
     }

   /*======================================================*/
   /* If we're dealing with an alpha memory partial match, */
   /* then return the multifield markers associated with   */
   /* the partial match (if any) along with the alphaMatch */
   /* data structure.                                      */
   /*======================================================*/

   if (waste->betaMemory == FALSE)
     {
      if (waste->binds[0].gm.theMatch->markers != NULL)
        { ReturnMarkers(theEnv,waste->binds[0].gm.theMatch->markers); }
      rm(theEnv,waste->binds[0].gm.theMatch,(int) sizeof(struct alphaMatch));
     }

   /*=================================================*/
   /* Remove any links between the partial match and  */
   /* a data entity that were created with the use of */
   /* the logical CE.                                 */
   /*=================================================*/

   if (waste->dependentsf) RemovePMDependencies(theEnv,waste);

   /*======================================================*/
   /* Return the partial match to the pool of free memory. */
   /*======================================================*/

   rtn_var_struct(theEnv,partialMatch,(int) sizeof(struct genericMatch *) *
                  (waste->bcount + waste->activationf + waste->dependentsf - 1),
                  waste);
  }

/***************************************************************/
/* DestroyPartialMatch: Returns the data structures associated */
/*   with a partial match to the pool of free memory.          */
/***************************************************************/
globle void DestroyPartialMatch(
  void *theEnv,
  struct partialMatch *waste)
  {
   /*======================================================*/
   /* If we're dealing with an alpha memory partial match, */
   /* then return the multifield markers associated with   */
   /* the partial match (if any) along with the alphaMatch */
   /* data structure.                                      */
   /*======================================================*/

   if (waste->betaMemory == FALSE)
     {
      if (waste->binds[0].gm.theMatch->markers != NULL)
        { ReturnMarkers(theEnv,waste->binds[0].gm.theMatch->markers); }
      rm(theEnv,waste->binds[0].gm.theMatch,(int) sizeof(struct alphaMatch));
     }
     
   /*================================================*/
   /* Remove the alpha match used to represent a not */
   /* CE match in a beta memory partial match.       */
   /*================================================*/
   
   if ((waste->notOriginf) && (waste->counterf == FALSE))
     {
      if (waste->binds[waste->bcount - 1].gm.theMatch != NULL)
        {
         rtn_struct(theEnv,alphaMatch,
                    waste->binds[waste->bcount - 1].gm.theMatch);
        }
     }

   /*=================================================*/
   /* Remove any links between the partial match and  */
   /* a data entity that were created with the use of */
   /* the logical CE.                                 */
   /*=================================================*/

   if (waste->dependentsf) DestroyPMDependencies(theEnv,waste);

   /*======================================================*/
   /* Return the partial match to the pool of free memory. */
   /*======================================================*/

   rtn_var_struct(theEnv,partialMatch,(int) sizeof(struct genericMatch *) *
                  (waste->bcount + waste->activationf + waste->dependentsf - 1),
                  waste);
  }

/******************************************************/
/* ReturnMarkers: Returns a linked list of multifield */
/*   markers associated with a data entity matching a */
/*   pattern to the pool of free memory.              */
/******************************************************/
static void ReturnMarkers(
  void *theEnv,
  struct multifieldMarker *waste)
  {
   struct multifieldMarker *temp;

   while (waste != NULL)
     {
      temp = waste->next;
      rtn_struct(theEnv,multifieldMarker,waste);
      waste = temp;
     }
  }

/*************************************************/
/* DriveRetractions: Filters the list of partial */
/*   matches created as a result of removing a   */
/*   data entity through the join network.       */
/*************************************************/
static void DriveRetractions(
  void *theEnv)
  {
   struct rdriveinfo *tempDR;
   struct joinNode *joinPtr;

   while (EngineData(theEnv)->DriveRetractionList != NULL)
     {
      for (joinPtr = EngineData(theEnv)->DriveRetractionList->jlist;
           joinPtr != NULL;
           joinPtr = joinPtr->rightDriveNode)
        { NetworkAssert(theEnv,EngineData(theEnv)->DriveRetractionList->link,joinPtr,LHS); }

      tempDR = EngineData(theEnv)->DriveRetractionList->next;
      rtn_struct(theEnv,rdriveinfo,EngineData(theEnv)->DriveRetractionList);
      EngineData(theEnv)->DriveRetractionList = tempDR;
     }
  }

/*************************************************/
/* RetractCheckDriveRetractions:       */
/*************************************************/
globle void RetractCheckDriveRetractions(  /* GDR 111599 #834 Begin */
  void *theEnv,
  struct alphaMatch *theAlphaNode,
  int position)
  {
   struct rdriveinfo *tempDR, *theDR, *lastDR = NULL;

   theDR = EngineData(theEnv)->DriveRetractionList;
   while (theDR != NULL)
     {
      if ((position < (int) theDR->link->bcount) &&
          (theDR->link->binds[position].gm.theMatch == theAlphaNode))
        {
         tempDR = theDR->next;
         rtn_struct(theEnv,rdriveinfo,theDR);
         if (lastDR == NULL)
           { EngineData(theEnv)->DriveRetractionList = tempDR; }
         else
           { lastDR->next = tempDR; }
         theDR = tempDR;
        }
      else
        {
         lastDR = theDR;
         theDR = theDR->next;
        }
     }
  }                                        /* GDR 111599 #834 End */
  
/*************************************************************/
/* FlushGarbagePartialMatches:  Returns partial matches and  */
/*   associated structures that were removed as part of a    */
/*   retraction. It is necessary to postpone returning these */
/*   structures to memory because RHS actions retrieve their */
/*   variable bindings directly from the fact and instance   */
/*   data structures through the alpha memory bindings.      */
/*************************************************************/
globle void FlushGarbagePartialMatches(
  void *theEnv)
  {
   struct partialMatch *pmPtr;
   struct alphaMatch *amPtr;

   /*===================================================*/
   /* Return the garbage partial matches collected from */
   /* the alpha memories of the pattern networks.       */
   /*===================================================*/

   while (EngineData(theEnv)->GarbageAlphaMatches != NULL)
     {
      amPtr = EngineData(theEnv)->GarbageAlphaMatches->next;
      rtn_struct(theEnv,alphaMatch,EngineData(theEnv)->GarbageAlphaMatches);
      EngineData(theEnv)->GarbageAlphaMatches = amPtr;
     }

   /*==============================================*/
   /* Return the garbage partial matches collected */
   /* from the beta memories of the join networks. */
   /*==============================================*/

   while (EngineData(theEnv)->GarbagePartialMatches != NULL)
     {
      /*=====================================================*/
      /* Remember the next garbage partial match to process. */
      /*=====================================================*/

      pmPtr = EngineData(theEnv)->GarbagePartialMatches->next;

      /*=======================================================*/
      /* If a "pseudo" data entity was created for the partial */
      /* match (i.e. a not CE was satisfied), then dispose of  */
      /* the pseudo data entity.                               */
      /*=======================================================*/

      if ((EngineData(theEnv)->GarbagePartialMatches->notOriginf) &&
          (EngineData(theEnv)->GarbagePartialMatches->counterf == FALSE))
        {
         if (EngineData(theEnv)->GarbagePartialMatches->binds[EngineData(theEnv)->GarbagePartialMatches->bcount - 1].gm.theMatch != NULL)
           {
            rtn_struct(theEnv,alphaMatch,
                       EngineData(theEnv)->GarbagePartialMatches->binds[EngineData(theEnv)->GarbagePartialMatches->bcount - 1].gm.theMatch);
           }
        }

      /*============================================*/
      /* Dispose of the garbage partial match being */
      /* examined and move on to the next one.      */
      /*============================================*/

      EngineData(theEnv)->GarbagePartialMatches->busy = FALSE;
      ReturnPartialMatch(theEnv,EngineData(theEnv)->GarbagePartialMatches);
      EngineData(theEnv)->GarbagePartialMatches = pmPtr;
     }
  }

#endif /* DEFRULE_CONSTRUCT */

