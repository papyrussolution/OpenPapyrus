   /*******************************************************/
   /*      "C" Language Integrated Production System      */
   /*                                                     */
   /*             CLIPS Version 6.24  05/17/06            */
   /*                                                     */
   /*                 RULE DELETION MODULE                */
   /*******************************************************/

/*************************************************************/
/* Purpose: Provides routines for deleting a rule including  */
/*   freeing the defrule data structures and removing the    */
/*   appropriate joins from the join network.                */
/*                                                           */
/* Principal Programmer(s):                                  */
/*      Gary D. Riley                                        */
/*                                                           */
/* Contributing Programmer(s):                               */
/*                                                           */
/* Revision History:                                         */
/*                                                           */
/*      6.24: Removed DYNAMIC_SALIENCE compilation flag.     */
/*                                                           */
/*            Renamed BOOLEAN macro type to intBool.         */
/*                                                           */
/*************************************************************/

#define _RULEDLT_SOURCE_

#include "setup.h"

#if DEFRULE_CONSTRUCT

#include <stdio.h>
#define _STDIO_INCLUDED_
#include <string.h>

#include "memalloc.h"
#include "engine.h"
#include "envrnmnt.h"
#include "reteutil.h"
#include "pattern.h"
#include "agenda.h"
#include "drive.h"
#include "retract.h"
#include "constrct.h"

#if BLOAD || BLOAD_ONLY || BLOAD_AND_BSAVE
#include "bload.h"
#endif

#include "ruledlt.h"

/***************************************/
/* LOCAL INTERNAL FUNCTION DEFINITIONS */
/***************************************/

#if (! RUN_TIME) && (! BLOAD_ONLY)
   static void                    RemoveIntranetworkLink(void *,struct joinNode *);
#endif
   static void                    DetachJoins(void *,struct defrule *,intBool);

/**********************************************************************/
/* ReturnDefrule: Returns a defrule data structure and its associated */
/*   data structures to the memory manager. Note that the first       */
/*   disjunct of a rule is the only disjunct which allocates storage  */
/*   for the rule's dynamic salience and pretty print form (so these  */
/*   are only deallocated for the first disjunct).                    */
/**********************************************************************/
globle void ReturnDefrule(
  void *theEnv,
  void *vWaste)
  {
#if (MAC_MCW || IBM_MCW) && (RUN_TIME || BLOAD_ONLY)
#pragma unused(theEnv,vWaste)
#endif

#if (! RUN_TIME) && (! BLOAD_ONLY)
   struct defrule *waste = (struct defrule *) vWaste;
   int first = TRUE;
   struct defrule *nextPtr;

   if (waste == NULL) return;

   /*======================================*/
   /* If a rule is redefined, then we want */
   /* to save its breakpoint status.       */
   /*======================================*/

#if DEBUGGING_FUNCTIONS
   DefruleData(theEnv)->DeletedRuleDebugFlags = 0;
   if (waste->afterBreakpoint) BitwiseSet(DefruleData(theEnv)->DeletedRuleDebugFlags,0);
   if (waste->watchActivation) BitwiseSet(DefruleData(theEnv)->DeletedRuleDebugFlags,1);
   if (waste->watchFiring) BitwiseSet(DefruleData(theEnv)->DeletedRuleDebugFlags,2);
#endif

   /*================================*/
   /* Clear the agenda of all the    */
   /* activations added by the rule. */
   /*================================*/

   ClearRuleFromAgenda(theEnv,waste);

   /*======================*/
   /* Get rid of the rule. */
   /*======================*/

   while (waste != NULL)
     {
      /*================================================*/
      /* Remove the rule's joins from the join network. */
      /*================================================*/

      DetachJoins(theEnv,waste,FALSE);

      /*=============================================*/
      /* If this is the first disjunct, get rid of   */
      /* the dynamic salience and pretty print form. */
      /*=============================================*/

      if (first)
        {
         if (waste->dynamicSalience != NULL)
          {
           ExpressionDeinstall(theEnv,waste->dynamicSalience);
           ReturnPackedExpression(theEnv,waste->dynamicSalience);
           waste->dynamicSalience = NULL;
          }
         if (waste->header.ppForm != NULL)
           {
            rm(theEnv,waste->header.ppForm,strlen(waste->header.ppForm) + 1);
            waste->header.ppForm = NULL;
           }

         first = FALSE;
        }

      /*===========================*/
      /* Get rid of any user data. */
      /*===========================*/
      
      if (waste->header.usrData != NULL)
        { ClearUserDataList(theEnv,waste->header.usrData); }
        
      /*===========================================*/
      /* Decrement the count for the defrule name. */
      /*===========================================*/

      DecrementSymbolCount(theEnv,waste->header.name);

      /*========================================*/
      /* Get rid of the the rule's RHS actions. */
      /*========================================*/

      if (waste->actions != NULL)
        {
         ExpressionDeinstall(theEnv,waste->actions);
         ReturnPackedExpression(theEnv,waste->actions);
        }

      /*===============================*/
      /* Move on to the next disjunct. */
      /*===============================*/

      nextPtr = waste->disjunct;
      rtn_struct(theEnv,defrule,waste);
      waste = nextPtr;
     }

   /*==========================*/
   /* Free up partial matches. */
   /*==========================*/

   if (EngineData(theEnv)->ExecutingRule == NULL) FlushGarbagePartialMatches(theEnv);
#endif
  }

/********************************************************/
/* DestroyDefrule: Action used to remove defrules       */
/*   as a result of DestroyEnvironment.                 */
/********************************************************/
globle void DestroyDefrule(
  void *theEnv,
  void *vTheDefrule)
  {
   struct defrule *theDefrule = (struct defrule *) vTheDefrule;
   struct defrule *nextDisjunct;
   int first = TRUE;
   
   if (theDefrule == NULL) return;
   
   while (theDefrule != NULL)
     {
      DetachJoins(theEnv,theDefrule,TRUE);

      if (first)
        {
#if (! BLOAD_ONLY) && (! RUN_TIME)
         if (theDefrule->dynamicSalience != NULL)
           { ReturnPackedExpression(theEnv,theDefrule->dynamicSalience); }

         if (theDefrule->header.ppForm != NULL)
           { rm(theEnv,theDefrule->header.ppForm,strlen(theDefrule->header.ppForm) + 1); }
#endif

         first = FALSE;
        }
     
      if (theDefrule->header.usrData != NULL)
        { ClearUserDataList(theEnv,theDefrule->header.usrData); }
        
#if (! BLOAD_ONLY) && (! RUN_TIME)
      if (theDefrule->actions != NULL)
        { ReturnPackedExpression(theEnv,theDefrule->actions); }
#endif
     
      nextDisjunct = theDefrule->disjunct;
      
#if (! BLOAD_ONLY) && (! RUN_TIME)
      rtn_struct(theEnv,defrule,theDefrule);
#endif

      theDefrule = nextDisjunct;
     }
  }

/**********************************************************************/
/* DetachJoins: Removes a join node and all of its parent nodes from  */
/*   the join network. Nodes are only removed if they are no required */
/*   by other rules (the same join can be shared by multiple rules).  */
/*   Any partial matches associated with the join are also removed.   */
/*   A rule's joins are typically removed by removing the bottom most */
/*   join used by the rule and then removing any parent joins which   */
/*   are not shared by other rules.                                   */
/**********************************************************************/
static void DetachJoins(
  void *theEnv,
  struct defrule *theRule,
  intBool destroy)
  {
   struct joinNode *join;
   struct joinNode *prevJoin;
   struct joinNode *joinPtr, *lastJoin, *rightJoin;

   /*==================================*/
   /* Find the last join for the rule. */
   /*==================================*/

   join = theRule->lastJoin;
   theRule->lastJoin = NULL;
   if (join == NULL) return;

   /*===================================================*/
   /* Remove the activation link from the last join. If */
   /* there are joins below this join, then all of the  */
   /* joins for this rule were shared with another rule */
   /* and thus no joins can be deleted.                 */
   /*===================================================*/

   join->ruleToActivate = NULL;
   if (join->nextLevel != NULL) return;

   /*===========================*/
   /* Begin removing the joins. */
   /*===========================*/

   while (join != NULL)
     {
      /*==========================================================*/
      /* Remember the join "above" this join (the one that enters */
      /* from the left). If the join is entered from the right by */
      /* another join, remember the right entering join as well.  */
      /*==========================================================*/

      prevJoin = join->lastLevel;
      if (join->joinFromTheRight)
        { rightJoin = (struct joinNode *) join->rightSideEntryStructure; }
      else
        { rightJoin = NULL; }

      /*=================================================*/
      /* If the join was attached to a pattern, remove   */
      /* any structures associated with the pattern that */
      /* are no longer needed.                           */
      /*=================================================*/
      
#if (! RUN_TIME) && (! BLOAD_ONLY)
      if (! destroy)
        {
         if ((join->rightSideEntryStructure != NULL) && (join->joinFromTheRight == FALSE))
           { RemoveIntranetworkLink(theEnv,join); }
        }
#endif
        
      /*======================================*/
      /* Remove any partial matches contained */
      /* in the beta memory of the join.      */
      /*======================================*/
      
      if (destroy)
        { DestroyAlphaBetaMemory(theEnv,join->beta); }
      else
        { FlushAlphaBetaMemory(theEnv,join->beta); }
      join->beta = NULL;

      /*===================================*/
      /* Remove the expressions associated */
      /* with the join.                    */
      /*===================================*/
      
#if (! RUN_TIME) && (! BLOAD_ONLY)
      if (! destroy)
        { RemoveHashedExpression(theEnv,join->networkTest); }
#endif

      /*==================================================*/
      /* Remove the link to the join from the join above. */
      /*==================================================*/

      if (prevJoin == NULL)
        {
#if (! RUN_TIME) && (! BLOAD_ONLY)
         rtn_struct(theEnv,joinNode,join);
#endif
         return;
        }

      lastJoin = NULL;
      joinPtr = prevJoin->nextLevel;
      while (joinPtr != NULL)
        {
         if (joinPtr == join)
           {
            if (lastJoin == NULL)
              { prevJoin->nextLevel = joinPtr->rightDriveNode; }
            else
              { lastJoin->rightDriveNode = joinPtr->rightDriveNode; }

            joinPtr = NULL;
           }
         else
           {
            lastJoin = joinPtr;
            joinPtr = joinPtr->rightDriveNode;
           }
         }

      /*==================*/
      /* Delete the join. */
      /*==================*/

#if (! RUN_TIME) && (! BLOAD_ONLY)
      rtn_struct(theEnv,joinNode,join);
#endif

      /*==========================================*/
      /* Remove the right join link if it exists. */
      /*==========================================*/

      if (rightJoin != NULL)
        {
         rightJoin->nextLevel = NULL;
         prevJoin = rightJoin;
        }

      /*===========================================================*/
      /* Move on to the next join to be removed. All the joins of  */
      /* a rule can be deleted by following the right joins links  */
      /* (when these links exist) and then following the left join */
      /* links. This works because if join A enters join B from    */
      /* the right, the right/left links of join A eventually lead */
      /* to the join which enters join B from the left.            */
      /*===========================================================*/

      if (prevJoin->ruleToActivate != NULL)
        { join = NULL; }
      else if (prevJoin->nextLevel == NULL)
        { join = prevJoin; }
      else
        { join = NULL; }
     }
  }

#if (! RUN_TIME) && (! BLOAD_ONLY)

/***********************************************************************/
/* RemoveIntranetworkLink: Removes the link between a join node in the */
/*   join network and its corresponding pattern node in the pattern    */
/*   network. If the pattern node is then no longer associated with    */
/*   any other joins, it is removed using the function DetachPattern.  */
/***********************************************************************/
static void RemoveIntranetworkLink(
  void *theEnv,
  struct joinNode *join)
  {
   struct patternNodeHeader *patternPtr;
   struct joinNode *joinPtr, *lastJoin;

   /*================================================*/
   /* Determine the pattern that enters this join.   */
   /* Determine the list of joins which this pattern */
   /* enters from the right.                         */
   /*================================================*/

   patternPtr = (struct patternNodeHeader *) join->rightSideEntryStructure;
   joinPtr = patternPtr->entryJoin;
   lastJoin = NULL;

   /*=================================================*/
   /* Loop through the list of joins that the pattern */
   /* enters until the join being removed is found.   */
   /* Remove this join from the list.                 */
   /*=================================================*/

   while (joinPtr != NULL)
     {
      if (joinPtr == join)
        {
         if (lastJoin == NULL)
           { patternPtr->entryJoin = joinPtr->rightMatchNode; }
         else
           { lastJoin->rightMatchNode = joinPtr->rightMatchNode; }

         joinPtr = NULL;
        }
      else
        {
         lastJoin = joinPtr;
         joinPtr = joinPtr->rightMatchNode;
        }
     }

   /*===================================================*/
   /* If the terminal node of the pattern doesn't point */
   /* to any joins, then start removing the pattern.    */
   /*===================================================*/

   if (patternPtr->entryJoin == NULL)
     { DetachPattern(theEnv,(int) join->rhsType,patternPtr); }
  }

#endif /* (! RUN_TIME) && (! BLOAD_ONLY) */

#endif /* DEFRULE_CONSTRUCT */



