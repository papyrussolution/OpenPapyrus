   /*******************************************************/
   /*      "C" Language Integrated Production System      */
   /*                                                     */
   /*               CLIPS Version 6.24  07/01/05          */
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
/*      6.24: Added allowed-classes slot facet.              */
/*                                                           */
/*            Renamed BOOLEAN macro type to intBool.         */
/*                                                           */
/*************************************************************/

#ifndef _H_classinf
#define _H_classinf

#ifndef _H_evaluatn
#include "evaluatn.h"
#endif

#ifdef LOCALE
#undef LOCALE
#endif

#ifdef _CLASSINF_SOURCE_
#define LOCALE
#else
#define LOCALE extern
#endif

#if ENVIRONMENT_API_ONLY
#define ClassAbstractP(theEnv,a) EnvClassAbstractP(theEnv,a)
#define ClassReactiveP(theEnv,a) EnvClassReactiveP(theEnv,a)
#define ClassSlots(theEnv,a,b,c) EnvClassSlots(theEnv,a,b,c)
#define ClassSubclasses(theEnv,a,b,c) EnvClassSubclasses(theEnv,a,b,c)
#define ClassSuperclasses(theEnv,a,b,c) EnvClassSuperclasses(theEnv,a,b,c)
#define SlotAllowedValues(theEnv,a,b,c) EnvSlotAllowedValues(theEnv,a,b,c)
#define SlotAllowedClasses(theEnv,a,b,c) EnvSlotAllowedClasses(theEnv,a,b,c)
#define SlotCardinality(theEnv,a,b,c) EnvSlotCardinality(theEnv,a,b,c)
#define SlotFacets(theEnv,a,b,c) EnvSlotFacets(theEnv,a,b,c)
#define SlotRange(theEnv,a,b,c) EnvSlotRange(theEnv,a,b,c)
#define SlotSources(theEnv,a,b,c) EnvSlotSources(theEnv,a,b,c)
#define SlotTypes(theEnv,a,b,c) EnvSlotTypes(theEnv,a,b,c)
#define GetDefmessageHandlerList(theEnv,a,b,c) EnvGetDefmessageHandlerList(theEnv,a,b,c)
#else
#define ClassAbstractP(a) EnvClassAbstractP(GetCurrentEnvironment(),a)
#define ClassReactiveP(a) EnvClassReactiveP(GetCurrentEnvironment(),a)
#define ClassSlots(a,b,c) EnvClassSlots(GetCurrentEnvironment(),a,b,c)
#define ClassSubclasses(a,b,c) EnvClassSubclasses(GetCurrentEnvironment(),a,b,c)
#define ClassSuperclasses(a,b,c) EnvClassSuperclasses(GetCurrentEnvironment(),a,b,c)
#define SlotAllowedValues(a,b,c) EnvSlotAllowedValues(GetCurrentEnvironment(),a,b,c)
#define SlotAllowedClasses(a,b,c) EnvSlotAllowedClasses(GetCurrentEnvironment(),a,b,c)
#define SlotCardinality(a,b,c) EnvSlotCardinality(GetCurrentEnvironment(),a,b,c)
#define SlotFacets(a,b,c) EnvSlotFacets(GetCurrentEnvironment(),a,b,c)
#define SlotRange(a,b,c) EnvSlotRange(GetCurrentEnvironment(),a,b,c)
#define SlotSources(a,b,c) EnvSlotSources(GetCurrentEnvironment(),a,b,c)
#define SlotTypes(a,b,c) EnvSlotTypes(GetCurrentEnvironment(),a,b,c)
#define GetDefmessageHandlerList(a,b,c) EnvGetDefmessageHandlerList(GetCurrentEnvironment(),a,b,c)
#endif

LOCALE intBool ClassAbstractPCommand(void *);
#if DEFRULE_CONSTRUCT
LOCALE intBool ClassReactivePCommand(void *);
#endif
LOCALE void *ClassInfoFnxArgs(void *,char *,int *);
LOCALE void ClassSlotsCommand(void *,DATA_OBJECT *);
LOCALE void ClassSuperclassesCommand(void *,DATA_OBJECT *);
LOCALE void ClassSubclassesCommand(void *,DATA_OBJECT *);
LOCALE void GetDefmessageHandlersListCmd(void *,DATA_OBJECT *);
LOCALE void SlotFacetsCommand(void *,DATA_OBJECT *);
LOCALE void SlotSourcesCommand(void *,DATA_OBJECT *);
LOCALE void SlotTypesCommand(void *,DATA_OBJECT *);
LOCALE void SlotAllowedValuesCommand(void *,DATA_OBJECT *);
LOCALE void SlotAllowedClassesCommand(void *,DATA_OBJECT *);
LOCALE void SlotRangeCommand(void *,DATA_OBJECT *);
LOCALE void SlotCardinalityCommand(void *,DATA_OBJECT *);
LOCALE intBool EnvClassAbstractP(void *,void *);
#if DEFRULE_CONSTRUCT
LOCALE intBool EnvClassReactiveP(void *,void *);
#endif
LOCALE void EnvClassSlots(void *,void *,DATA_OBJECT *,int);
LOCALE void EnvGetDefmessageHandlerList(void *,void *,DATA_OBJECT *,int);
LOCALE void EnvClassSuperclasses(void *,void *,DATA_OBJECT *,int);
LOCALE void EnvClassSubclasses(void *,void *,DATA_OBJECT *,int);
LOCALE void ClassSubclassAddresses(void *,void *,DATA_OBJECT *,int);
LOCALE void EnvSlotFacets(void *,void *,char *,DATA_OBJECT *);
LOCALE void EnvSlotSources(void *,void *,char *,DATA_OBJECT *);
LOCALE void EnvSlotTypes(void *,void *,char *,DATA_OBJECT *);
LOCALE void EnvSlotAllowedValues(void *,void *,char *,DATA_OBJECT *);
LOCALE void EnvSlotAllowedClasses(void *,void *,char *,DATA_OBJECT *);
LOCALE void EnvSlotRange(void *,void *,char *,DATA_OBJECT *);
LOCALE void EnvSlotCardinality(void *,void *,char *,DATA_OBJECT *);

#endif





