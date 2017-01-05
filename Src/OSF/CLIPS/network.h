   /*******************************************************/
   /*      "C" Language Integrated Production System      */
   /*                                                     */
   /*             CLIPS Version 6.20  01/31/02            */
   /*                                                     */
   /*                 NETWORK HEADER FILE                 */
   /*******************************************************/

/*************************************************************/
/* Purpose:                                                  */
/*                                                           */
/* Principal Programmer(s):                                  */
/*      Gary D. Riley                                        */
/*                                                           */
/* Contributing Programmer(s):                               */
/*                                                           */
/* Revision History:                                         */
/*                                                           */
/*************************************************************/

#ifndef _H_network

#define _H_network

struct patternNodeHeader;
struct joinNode;

#ifndef _H_match
#include "match.h"
#endif

struct patternNodeHeader
  {
   struct partialMatch *alphaMemory;
   struct partialMatch *endOfQueue;
   struct joinNode *entryJoin;
   unsigned int singlefieldNode : 1;
   unsigned int multifieldNode : 1;
   unsigned int stopNode : 1;
   unsigned int initialize : 1;
   unsigned int marked : 1;
   unsigned int beginSlot : 1;
   unsigned int endSlot : 1;
  };

#ifndef _H_expressn
#include "expressn.h"
#endif

#ifndef _H_ruledef
#include "ruledef.h"
#endif

struct joinNode
  {
   unsigned int firstJoin : 1;
   unsigned int logicalJoin : 1;
   unsigned int joinFromTheRight : 1;
   unsigned int patternIsNegated : 1;
   unsigned int initialize : 1;
   unsigned int marked : 1;
   unsigned int rhsType : 3;
   unsigned int depth : 7;
   long bsaveID;
   struct partialMatch *beta;
   struct expr *networkTest;
   void *rightSideEntryStructure;
   struct joinNode *nextLevel;
   struct joinNode *lastLevel;
   struct joinNode *rightDriveNode;
   struct joinNode *rightMatchNode;
   struct defrule *ruleToActivate;
  };

#endif




