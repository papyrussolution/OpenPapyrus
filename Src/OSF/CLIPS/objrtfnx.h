   /*******************************************************/
   /*      "C" Language Integrated Production System      */
   /*                                                     */
   /*               CLIPS Version 6.24  05/17/06          */
   /*                                                     */
   /*                                                     */
   /*******************************************************/

/*************************************************************/
/* Purpose:                                                  */
/*                                                           */
/* Principal Programmer(s):                                  */
/*      Brian L. Donnell                                     */
/*                                                           */
/* Contributing Programmer(s):                               */
/*                                                           */
/* Revision History:                                         */
/*                                                           */
/*      6.24: Converted INSTANCE_PATTERN_MATCHING to         */
/*            DEFRULE_CONSTRUCT.                             */
/*                                                           */
/*            Renamed BOOLEAN macro type to intBool.         */
/*                                                           */
/*************************************************************/

#ifndef _H_objrtfnx
#define _H_objrtfnx

#if DEFRULE_CONSTRUCT && OBJECT_SYSTEM

#ifndef _H_conscomp
#include "conscomp.h"
#endif
#ifndef _H_evaluatn
#include "evaluatn.h"
#endif
#ifndef _H_object
#include "object.h"
#endif
#ifndef _H_match
#include "match.h"
#endif
#ifndef _H_objrtmch
#include "objrtmch.h"
#endif

struct ObjectMatchVar1
  {
   unsigned short whichSlot;
   unsigned short whichPattern;
   unsigned short whichField;
   unsigned objectAddress : 1;
   unsigned allFields     : 1;
  };

struct ObjectMatchVar2
  {
   unsigned short whichSlot;
   unsigned short whichPattern;
   unsigned fromBeginning   : 1;
   unsigned beginningOffset : 7;
   unsigned fromEnd         : 1;
   unsigned endOffset       : 7;
  };

struct ObjectMatchLength
  {
   unsigned minLength : 15;
   unsigned exactly   : 1;
  };

struct ObjectCmpPNConstant
  {
   unsigned offset        : 7;
   unsigned pass          : 1;
   unsigned fail          : 1;
   unsigned general       : 1;
   unsigned fromBeginning : 1;
  };

struct ObjectCmpPNSingleSlotVars1
  {
   unsigned short firstSlot;
   unsigned short secondSlot;
   unsigned pass       : 1;
   unsigned fail       : 1;
  };

struct ObjectCmpPNSingleSlotVars2
  {
   unsigned short firstSlot;
   unsigned short secondSlot;
   unsigned pass          : 1;
   unsigned fail          : 1;
   unsigned offset        : 7;
   unsigned fromBeginning : 1;
  };

struct ObjectCmpPNSingleSlotVars3
  {
   unsigned short firstSlot;
   unsigned short secondSlot;
   unsigned pass                : 1;
   unsigned fail                : 1;
   unsigned firstOffset         : 7;
   unsigned firstFromBeginning  : 1;
   unsigned secondOffset        : 7;
   unsigned secondFromBeginning : 1;
  };

struct ObjectCmpJoinSingleSlotVars1
  {
   unsigned short firstSlot;
   unsigned short secondSlot;
   unsigned short firstPattern;
   unsigned short secondPattern;
   unsigned pass          : 1;
   unsigned fail          : 1;
  };

struct ObjectCmpJoinSingleSlotVars2
  {
   unsigned short firstSlot;
   unsigned short secondSlot;
   unsigned short firstPattern;
   unsigned short secondPattern;
   unsigned pass          : 1;
   unsigned fromBeginning : 1;
   unsigned offset        : 7;
   unsigned fail          : 1;
  };

struct ObjectCmpJoinSingleSlotVars3
  {
   unsigned short firstSlot;
   unsigned short secondSlot;
   unsigned short firstPattern;
   unsigned short secondPattern;
   unsigned pass                : 1;
   unsigned fail                : 1;
   unsigned firstOffset         : 7;
   unsigned firstFromBeginning  : 1;
   unsigned secondOffset        : 7;
   unsigned secondFromBeginning : 1;
  };

#define OBJECT_RETE_DATA 35

struct objectReteData
  { 
   INSTANCE_TYPE *CurrentPatternObject;
   INSTANCE_SLOT *CurrentPatternObjectSlot;
   unsigned CurrentObjectSlotLength;
   struct multifieldMarker *CurrentPatternObjectMarks;
   struct entityRecord ObjectGVInfo1;  
   struct entityRecord ObjectGVInfo2;
   struct entityRecord ObjectGVPNInfo1;
   struct entityRecord ObjectGVPNInfo2;
   struct entityRecord ObjectCmpConstantInfo; 
   struct entityRecord LengthTestInfo; 
   struct entityRecord PNSimpleCompareInfo1; 
   struct entityRecord PNSimpleCompareInfo2; 
   struct entityRecord PNSimpleCompareInfo3; 
   struct entityRecord JNSimpleCompareInfo1; 
   struct entityRecord JNSimpleCompareInfo2; 
   struct entityRecord JNSimpleCompareInfo3; 
   OBJECT_MATCH_ACTION *ObjectMatchActionQueue;
   OBJECT_PATTERN_NODE *ObjectPatternNetworkPointer;
   OBJECT_ALPHA_NODE *ObjectPatternNetworkTerminalPointer;
   intBool DelayObjectPatternMatching;
   unsigned long CurrentObjectMatchTimeTag;
   long UseEntityTimeTag;
#if DEFRULE_CONSTRUCT && OBJECT_SYSTEM && CONSTRUCT_COMPILER && (! RUN_TIME)
   struct CodeGeneratorItem *ObjectPatternCodeItem;
#endif
  };

#define ObjectReteData(theEnv) ((struct objectReteData *) GetEnvironmentData(theEnv,OBJECT_RETE_DATA))


#ifdef LOCALE
#undef LOCALE
#endif

#ifdef _OBJRTFNX_SOURCE_
#define LOCALE
#else
#define LOCALE extern
#endif

LOCALE void InstallObjectPrimitives(void *);
LOCALE intBool ObjectCmpConstantFunction(void *,void *,DATA_OBJECT *);

#endif

#endif






