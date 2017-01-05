   /*******************************************************/
   /*      "C" Language Integrated Production System      */
   /*                                                     */
   /*             CLIPS Version 6.24  05/17/06            */
   /*                                                     */
   /*                  RULE BUILD MODULE                  */
   /*******************************************************/

/*************************************************************/
/* Purpose: Provides routines to ntegrates a set of pattern  */
/*   and join tests associated with a rule into the pattern  */
/*   and join networks. The joins are integrated into the    */
/*   join network by routines in this module. The pattern    */
/*   is integrated by calling the external routine           */
/*   associated with the pattern parser that originally      */
/*   parsed the pattern.                                     */
/*                                                           */
/* Principal Programmer(s):                                  */
/*      Gary D. Riley                                        */
/*                                                           */
/* Contributing Programmer(s):                               */
/*                                                           */
/* Revision History:                                         */
/*                                                           */
/*      6.24: Removed INCREMENTAL_RESET compilation flag.    */
/*                                                           */
/*            Corrected code to remove compiler warnings.    */
/*                                                           */
/*************************************************************/

#define _RULEBLD_SOURCE_

#include "setup.h"

#if DEFRULE_CONSTRUCT && (! RUN_TIME) && (! BLOAD_ONLY)

#include <stdio.h>
#define _STDIO_INCLUDED_
#include <stdlib.h>

#include "constant.h"
#include "envrnmnt.h"
#include "constrct.h"
#include "drive.h"
#include "incrrset.h"
#include "memalloc.h"
#include "pattern.h"
#include "reteutil.h"
#include "router.h"
#include "rulebld.h"
#include "watch.h"

/***************************************/
/* LOCAL INTERNAL FUNCTION DEFINITIONS */
/***************************************/

   static struct joinNode        *FindShareableJoin(struct joinNode *,void *,unsigned,unsigned,int,
                                                    struct expr *,
                                                    int,int,int,struct joinNode **);
   static int                     TestJoinForReuse(struct joinNode *,unsigned,unsigned,int,
                                                   struct expr *,
                                                   int,int,int,struct joinNode **);
   static struct joinNode        *CreateNewJoin(void *,struct expr *,
                                                struct joinNode *,void *,int,int);
   static void                    AttachTestCEsToPatternCEs(void *,struct lhsParseNode *);

/****************************************************************/
/* ConstructJoins: Integrates a set of pattern and join tests   */
/*   associated with a rule into the pattern and join networks. */
/****************************************************************/
globle struct joinNode *ConstructJoins(
  void *theEnv,
  int logicalJoin,
  struct lhsParseNode *theLHS)
  {
   struct joinNode *lastJoin = NULL;
   struct patternNodeHeader *lastPattern;
   unsigned firstJoin = TRUE;
   int tryToReuse = TRUE;
   struct joinNode *listOfJoins;
   struct joinNode *oldJoin;
   int joinNumber = 1;
   int isLogical;
   struct joinNode *nandReconnect[32];
   int currentDepth = 1;
   int lastIteration = FALSE;
   int rhsType;
   int endDepth;

   /*===================================================*/
   /* Remove any test CEs from the LHS and attach their */
   /* expression to the closest preceeding non-negated  */
   /* join at the same not/and depth.                   */
   /*===================================================*/

   AttachTestCEsToPatternCEs(theEnv,theLHS);

   /*=====================================================*/
   /* Process each pattern CE in the rule. At this point, */
   /* there should be no and/or/not/test CEs in the LHS.  */
   /*=====================================================*/

   while (theLHS != NULL)
     {
      if (theLHS->bottom == NULL) lastIteration = TRUE;

      /*==================================================*/
      /* If the pattern is the start of a new not/and CE, */
      /* then remember the join to reconnect to after the */
      /* join from the right is completed.                */
      /*==================================================*/

      while (theLHS->beginNandDepth > currentDepth)
        {
         nandReconnect[currentDepth-1] = lastJoin;
         currentDepth++;
        }

      /*============================================================*/
      /* Add the next pattern for this rule to the pattern network. */
      /*============================================================*/

      rhsType = theLHS->patternType->positionInArray;
      lastPattern = (*theLHS->patternType->addPatternFunction)(theEnv,theLHS);

      /*======================================================*/
      /* Determine if the join being added is a logical join. */
      /*======================================================*/

      if (joinNumber == logicalJoin) isLogical = TRUE;
      else isLogical = FALSE;

      /*===============================================*/
      /* Get the list of joins which could potentially */
      /* be reused in place of the join being added.   */
      /*===============================================*/

      if (firstJoin == TRUE)
        { listOfJoins = lastPattern->entryJoin; }
      else
        { listOfJoins = lastJoin->nextLevel; }

      /*=======================================================*/
      /* Determine if the next join to be added can be shared. */
      /*=======================================================*/

      endDepth = theLHS->endNandDepth;
      if ((tryToReuse == TRUE) &&
          ((oldJoin = FindShareableJoin(listOfJoins,(void *) lastPattern,firstJoin,
                                        theLHS->negated,isLogical,
                                        theLHS->networkTest,
                                        endDepth,currentDepth,
                                        lastIteration,nandReconnect)) != NULL) )
        {
#if DEBUGGING_FUNCTIONS
         if ((EnvGetWatchItem(theEnv,"compilations") == TRUE) && GetPrintWhileLoading(theEnv))
           { EnvPrintRouter(theEnv,WDIALOG,"=j"); }
#endif
         lastJoin = oldJoin;
        }
      else
        {
         tryToReuse = FALSE;
         lastJoin = CreateNewJoin(theEnv,theLHS->networkTest,
                                  lastJoin,lastPattern,
                                  FALSE,(int) theLHS->negated);
         lastJoin->rhsType = rhsType;
        }

      /*==========================================================*/
      /* Create any joins from the right needed to handle not/and */
      /* CE combinations and connect them to the join network.    */
      /*==========================================================*/

      while (endDepth < currentDepth)
        {
         currentDepth--;

         if (lastJoin->nextLevel == NULL) tryToReuse = FALSE;

         if (tryToReuse)
           {
#if DEBUGGING_FUNCTIONS
            if ((EnvGetWatchItem(theEnv,"compilations") == TRUE) && GetPrintWhileLoading(theEnv))
              { EnvPrintRouter(theEnv,WDIALOG,"=j"); }
#endif
            lastJoin = lastJoin->nextLevel;
           }
         else
           {
            lastJoin = CreateNewJoin(theEnv,NULL,nandReconnect[currentDepth-1],
                                     lastJoin,TRUE,FALSE);
           }
        }

      /*=======================================*/
      /* Move on to the next join to be added. */
      /*=======================================*/

      theLHS = theLHS->bottom;
      joinNumber++;
      firstJoin = FALSE;
     }

   /*===================================================*/
   /* If compilations are being watched, put a carriage */
   /* return after all of the =j's and +j's             */
   /*===================================================*/

#if DEBUGGING_FUNCTIONS
   if ((EnvGetWatchItem(theEnv,"compilations") == TRUE) && GetPrintWhileLoading(theEnv))
     { EnvPrintRouter(theEnv,WDIALOG,"\n"); }
#endif

   /*=============================*/
   /* Return the last join added. */
   /*=============================*/

   return(lastJoin);
  }

/****************************************************************/
/* AttachTestCEsToPatternCEs: Attaches the expressions found in */
/*   test CEs to the closest preceeding pattern CE that is not  */
/*   negated and is at the same not/and depth.                  */
/****************************************************************/
static void AttachTestCEsToPatternCEs(
  void *theEnv,
  struct lhsParseNode *theLHS)
  {
   struct lhsParseNode *lastNode = NULL, *trackNode, *tempNode;

   /*===============================================*/
   /* Look at each pattern on the rule's LHS to see */
   /* if any test CEs should be attached to it.     */
   /*===============================================*/

   while (theLHS != NULL)
     {
      /*==============================================*/
      /* If the pattern is negated, then don't bother */
      /* looking for any test CEs to attach to it.    */
      /*==============================================*/

      if (theLHS->negated)
        { trackNode = NULL; }
      else
        {
         lastNode = theLHS;
         trackNode = theLHS->bottom;
        }

      /*=================================================*/
      /* Check all of the patterns following the current */
      /* pattern to check for test CEs which can be      */
      /* attached to the current pattern.                */
      /*=================================================*/

      while (trackNode != NULL)
        {
         /*=======================================================*/
         /* Skip over any CEs that have a higher not/and depth or */
         /* are negated since any test CEs found within these CEs */
         /* would be attached to another pattern with the same    */
         /* depth, rather than the current pattern.               */
         /*=======================================================*/

         if ((trackNode->beginNandDepth != theLHS->beginNandDepth) ||
             (trackNode->negated))
           {
            lastNode = trackNode;
            trackNode = trackNode->bottom;
           }

         /*======================================================*/
         /* Once a non-negated pattern has been encounted at the */
         /* same not/and depth as the current pattern, then stop */
         /* because any test CEs following this pattern would be */
         /* attached to it rather than the current pattern.      */
         /*======================================================*/

         else if (trackNode->type == PATTERN_CE)
           { trackNode = NULL; }

         /*==================================================*/
         /* A test CE encountered at the same not/and depth  */
         /* can be added to the network test expressions for */
         /* the currentpattern.                              */
         /*==================================================*/

         else if (trackNode->type == TEST_CE)
           {
            theLHS->networkTest = CombineExpressions(theEnv,theLHS->networkTest,
                                                     trackNode->networkTest);
            trackNode->networkTest = NULL;
            tempNode = trackNode->bottom;
            trackNode->bottom = NULL;
            lastNode->bottom = tempNode;
            lastNode->endNandDepth = trackNode->endNandDepth;
            ReturnLHSParseNodes(theEnv,trackNode);
            trackNode = tempNode;
           }

         /*================================================*/
         /* If none of the previous conditions have been   */
         /* met, then there is an internal error.          */
         /*================================================*/

         else
           {
            SystemError(theEnv,"BUILD",1);
            EnvExitRouter(theEnv,EXIT_FAILURE);
           }
        }

      /*====================================*/
      /* Check the next pattern in the LHS. */
      /*====================================*/

      theLHS = theLHS->bottom;
     }
  }

/********************************************************************/
/* FindShareableJoin: Determines whether a join exists that can be  */
/*   reused for the join currently being added to the join network. */
/*   Returns a pointer to the join to be shared if one if found,    */
/*   otherwise returns a NULL pointer.                              */
/********************************************************************/
static struct joinNode *FindShareableJoin(
  struct joinNode *listOfJoins,
  void *rhsStruct,
  unsigned int firstJoin,
  unsigned int negatedRHS,
  int isLogical,
  struct expr *joinTest,
  int endDepth,
  int currentDepth,
  int lastIteration,
  struct joinNode **nandReconnect)
  {
   /*========================================*/
   /* Loop through all of the joins in the   */
   /* list of potential candiates for reuse. */
   /*========================================*/

   while (listOfJoins != NULL)
     {
      /*=========================================================*/
      /* If the join being tested for reuse is connected on the  */
      /* RHS to the end node of the pattern node associated with */
      /* the join to be added, then determine if the join can    */
      /* be reused. If so, return the join.                      */
      /*=========================================================*/

      if (listOfJoins->rightSideEntryStructure == rhsStruct)
        {
         if (TestJoinForReuse(listOfJoins,firstJoin,negatedRHS,isLogical,
                              joinTest,endDepth,currentDepth,
                              lastIteration,nandReconnect))
           { return(listOfJoins); }
        }

      /*====================================================*/
      /* Move on to the next potential candidate. Note that */
      /* the rightMatchNode link is used for traversing     */
      /* through the candidates for the first join of a     */
      /* rule and that rightDriveNode link is used for      */
      /* traversing through the candidates for subsequent   */
      /* joins of a rule.                                   */
      /*====================================================*/

      if (firstJoin)
        { listOfJoins = listOfJoins->rightMatchNode; }
      else
        { listOfJoins = listOfJoins->rightDriveNode; }
     }

   /*================================*/
   /* Return a NULL pointer, since a */
   /* reusable join was not found.   */
   /*================================*/

   return(NULL);
  }

/**************************************************************/
/* TestJoinForReuse: Determines if the specified join can be  */
/*   shared with a join being added for a rule being defined. */
/*   Returns TRUE if the join can be shared, otherwise FALSE. */
/**************************************************************/
static int TestJoinForReuse(
  struct joinNode *testJoin,
  unsigned firstJoin,
  unsigned negatedRHS,
  int isLogical,
  struct expr *joinTest,
  int endDepth,
  int currentDepth,
  int lastIteration,
  struct joinNode **nandReconnect)
  {
   /*==================================================*/
   /* The first join of a rule may only be shared with */
   /* a join that has its firstJoin field set to TRUE. */
   /*==================================================*/

   if (testJoin->firstJoin != firstJoin) return(FALSE);

   /*========================================================*/
   /* A join connected to a not CE may only be shared with a */
   /* join that has its patternIsNegated field set to TRUE.  */
   /*========================================================*/

   if (testJoin->patternIsNegated != negatedRHS) return(FALSE);

   /*==========================================================*/
   /* If the join added is associated with a logical CE, then  */
   /* either the join to be shared must be associated with a   */
   /* logical CE or the beta memory must be empty (since       */
   /* joins associate an extra field with each partial match). */
   /*==========================================================*/

   if ((isLogical == TRUE) &&
       (testJoin->logicalJoin == FALSE) &&
       (testJoin->beta != NULL))
     { return(FALSE); }

   /*===============================================================*/
   /* The expression associated with the join must be identical to  */
   /* the networkTest expression stored with the join to be shared. */
   /*===============================================================*/

   if (IdenticalExpression(testJoin->networkTest,joinTest) != TRUE)
     { return(FALSE); }

   /*==============================================================*/
   /* If the join being added enters another join from the right,  */
   /* then the series of "joins from the right" for the join being */
   /* added must match the series of "joins from the right" for    */
   /* the join being tested for reuse (i.e. the LHS connections    */
   /* from other joins must be identical for each of the joins in  */
   /* the series of "joins from the right."                        */
   /*==============================================================*/

   for (; endDepth < currentDepth; currentDepth--)
     {
      testJoin = testJoin->nextLevel;
      if (testJoin == NULL) return(FALSE);

      if (testJoin->joinFromTheRight == FALSE)
        { return(FALSE); }
      else if (nandReconnect[currentDepth-2] != testJoin->lastLevel)
        { return(FALSE); }
     }

   /*=============================================================*/
   /* The last join of a rule cannot be shared with the last join */
   /* of another rule. A join cannot be used as the last join of  */
   /* a rule if it already has partial matches in its beta memory */
   /* (because of the extra slot used to point at activations).   */
   /*=============================================================*/

   if (lastIteration)
     {
      if (testJoin->ruleToActivate != NULL) return(FALSE);

      if (testJoin->beta != NULL) return(FALSE);
     }

   /*===========================================================================*/
   /* A join cannot be shared if it is not the last join for a rule and shares  */
   /* part, but not all, of a series of joins connected to other joins from the */
   /* right. This is because the data structure for joins can only point to     */
   /* either a single join that is entered from the right or a series of joins  */
   /* that are entered from the left, but not both. (The last join of a rule    */
   /* does not require any links to other joins so it can be shared).           */
   /*===========================================================================*/

   if ((! lastIteration) && (testJoin->nextLevel != NULL))
     {
      if (testJoin->nextLevel->joinFromTheRight == TRUE)
        {
         if (((struct joinNode *) testJoin->nextLevel->rightSideEntryStructure) == testJoin)
           { return(FALSE); }
        }
     }

   /*=============================================*/
   /* The join can be shared since all conditions */
   /* for sharing have been satisfied.            */
   /*=============================================*/

   return(TRUE);
  }

/*************************************************************************/
/* CreateNewJoin: Creates a new join and links it into the join network. */
/*************************************************************************/
static struct joinNode *CreateNewJoin(
  void *theEnv,
  struct expr *joinTest,
  struct joinNode *lhsEntryStruct,
  void *rhsEntryStruct,
  int joinFromTheRight,
  int negatedRHSPattern)
  {
   struct joinNode *newJoin;

   /*===============================================*/
   /* If compilations are being watch, print +j to  */
   /* indicate that a new join has been created for */
   /* this pattern of the rule (i.e. a join could   */
   /* not be shared with another rule.              */
   /*===============================================*/

#if DEBUGGING_FUNCTIONS
   if ((EnvGetWatchItem(theEnv,"compilations") == TRUE) && GetPrintWhileLoading(theEnv))
     { EnvPrintRouter(theEnv,WDIALOG,"+j"); }
#endif

   /*========================================================*/
   /* Create the new join and initialize some of its values. */
   /*========================================================*/

   newJoin = get_struct(theEnv,joinNode);
   newJoin->beta = NULL;
   newJoin->nextLevel = NULL;
   newJoin->joinFromTheRight = joinFromTheRight;
   newJoin->patternIsNegated = negatedRHSPattern;
   newJoin->initialize = EnvGetIncrementalReset(theEnv);
   newJoin->logicalJoin = FALSE;
   newJoin->ruleToActivate = NULL;

   /*==============================================*/
   /* Install the expressions used to determine    */
   /* if a partial match satisfies the constraints */
   /* associated with this join.                   */
   /*==============================================*/

   newJoin->networkTest = AddHashedExpression(theEnv,joinTest);

   /*============================================================*/
   /* Initialize the values associated with the LHS of the join. */
   /*============================================================*/

   newJoin->lastLevel = lhsEntryStruct;

   if (lhsEntryStruct == NULL)
     {
      newJoin->firstJoin = TRUE;
      newJoin->depth = 1;
      newJoin->rightDriveNode = NULL;
     }
   else
     {
      newJoin->firstJoin = FALSE;
      newJoin->depth = lhsEntryStruct->depth;
      newJoin->depth++; /* To work around Sparcworks C compiler bug */
      newJoin->rightDriveNode = lhsEntryStruct->nextLevel;
      lhsEntryStruct->nextLevel = newJoin;
     }

   /*=======================================================*/
   /* Initialize the pointer values associated with the RHS */
   /* of the join (both for the new join and the join or    */
   /* pattern which enters this join from the right.        */
   /*=======================================================*/

   newJoin->rightSideEntryStructure = rhsEntryStruct;

   if (joinFromTheRight)
     {
      newJoin->rightMatchNode = NULL;
      ((struct joinNode *) rhsEntryStruct)->nextLevel = newJoin;
     }
   else
     {
      newJoin->rightMatchNode = ((struct patternNodeHeader *) rhsEntryStruct)->entryJoin;
      ((struct patternNodeHeader *) rhsEntryStruct)->entryJoin = newJoin;
     }

   /*================================*/
   /* Return the newly created join. */
   /*================================*/

   return(newJoin);
  }

#endif



