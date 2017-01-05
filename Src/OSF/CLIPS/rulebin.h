   /*******************************************************/
   /*      "C" Language Integrated Production System      */
   /*                                                     */
   /*             CLIPS Version 6.20  01/31/02            */
   /*                                                     */
   /*           DEFRULE BSAVE/BLOAD HEADER FILE           */
   /*******************************************************/

/*************************************************************/
/* Purpose: Implements the binary save/load feature for the  */
/*    defrule construct.                                     */
/*                                                           */
/* Principal Programmer(s):                                  */
/*      Gary D. Riley                                        */
/*                                                           */
/* Contributing Programmer(s):                               */
/*      Brian L. Donnell                                     */
/*                                                           */
/* Revision History:                                         */
/*                                                           */
/*************************************************************/

#if (! RUN_TIME)
#ifndef _H_rulebin

#define _H_rulebin

#include "modulbin.h"
#include "cstrcbin.h"
#ifndef _H_network
#include "network.h"
#endif

struct bsaveDefrule
  {
   struct bsaveConstructHeader header;
   int salience;
   int localVarCnt;
   unsigned int complexity      : 12;
   unsigned int autoFocus       :  1;
   long dynamicSalience;
   long actions;
   long logicalJoin;
   long lastJoin;
   long disjunct;
  };

struct bsavePatternNodeHeader
  {
   long entryJoin;
   unsigned int singlefieldNode : 1;
   unsigned int multifieldNode : 1;
   unsigned int stopNode : 1;
   unsigned int blocked : 1;
   unsigned int initialize : 1;
   unsigned int marked : 1;
   unsigned int beginSlot : 1;
   unsigned int endSlot : 1;
  };

struct bsaveDefruleModule
  {
   struct bsaveDefmoduleItemHeader header;
  };

struct bsaveJoinNode
  {
   unsigned int firstJoin : 1;
   unsigned int logicalJoin : 1;
   unsigned int joinFromTheRight : 1;
   unsigned int patternIsNegated : 1;
   unsigned int rhsType : 3;
   unsigned int depth : 7;
   long networkTest;
   long rightSideEntryStructure;
   long nextLevel;
   long lastLevel;
   long rightDriveNode;
   long rightMatchNode;
   long ruleToActivate;
  };
  
#define RULEBIN_DATA 20

struct defruleBinaryData
  { 
   long NumberOfDefruleModules;
   long NumberOfDefrules;
   long NumberOfJoins;
   struct defruleModule *ModuleArray;
   struct defrule *DefruleArray;
   struct joinNode *JoinArray;
  };

#define DefruleBinaryData(theEnv) ((struct defruleBinaryData *) GetEnvironmentData(theEnv,RULEBIN_DATA))

#define BloadDefrulePointer(x,i) ((struct defrule *) (( i == -1L) ? NULL : &x[i]))
#define BsaveJoinIndex(joinPtr) ((joinPtr == NULL) ? -1L :  ((struct joinNode *) joinPtr)->bsaveID)
#define BloadJoinPointer(i) ((struct joinNode *) ((i == -1L) ? NULL : &DefruleBinaryData(theEnv)->JoinArray[i]))

#ifdef LOCALE
#undef LOCALE
#endif

#ifdef _RULEBIN_SOURCE_
#define LOCALE
#else
#define LOCALE extern
#endif

   LOCALE void                           DefruleBinarySetup(void *);
   LOCALE void                           UpdatePatternNodeHeader(void *,struct patternNodeHeader *,
                                                                 struct bsavePatternNodeHeader *);
   LOCALE void                           AssignBsavePatternHeaderValues(struct bsavePatternNodeHeader *,
                                                                 struct patternNodeHeader *);
   LOCALE void                          *BloadDefruleModuleReference(void *,int);

#endif
#endif




