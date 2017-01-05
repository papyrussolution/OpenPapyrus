   /*******************************************************/
   /*      "C" Language Integrated Production System      */
   /*                                                     */
   /*             CLIPS Version 6.24  06/05/06            */
   /*                                                     */
   /*              RETE UTILITY HEADER FILE               */
   /*******************************************************/

/*************************************************************/
/* Purpose: Provides a set of utility functions useful to    */
/*   other modules.                                          */
/*                                                           */
/* Principal Programmer(s):                                  */
/*      Gary D. Riley                                        */
/*                                                           */
/* Contributing Programmer(s):                               */
/*                                                           */
/* Revision History:                                         */
/*                                                           */
/*      6.24: Rule with exists CE has incorrect activation.  */
/*            DR0867                                         */
/*                                                           */
/*************************************************************/

#ifndef _H_reteutil
#define _H_reteutil

#ifndef _H_evaluatn
#include "evaluatn.h"
#endif
#ifndef _H_match
#include "match.h"
#endif
#ifndef _H_network
#include "network.h"
#endif

#ifdef LOCALE
#undef LOCALE
#endif

#ifdef _RETEUTIL_SOURCE_
#define LOCALE
#else
#define LOCALE extern
#endif

   LOCALE void                           PrintPartialMatch(void *,char *,struct partialMatch *);
   LOCALE struct partialMatch           *CopyPartialMatch(void *,struct partialMatch *,int,int);
   LOCALE struct partialMatch           *MergePartialMatches(void *,struct partialMatch *,struct partialMatch *,int,int);
   LOCALE struct partialMatch           *AddSingleMatch(void *,struct partialMatch *,struct alphaMatch *,int,int);
   LOCALE struct partialMatch           *NewPseudoFactPartialMatch(void *);
   LOCALE long int                       IncrementPseudoFactIndex(void);
   LOCALE void                           FlushAlphaBetaMemory(void *,struct partialMatch *);
   LOCALE void                           DestroyAlphaBetaMemory(void *,struct partialMatch *);
   LOCALE int                            GetPatternNumberFromJoin(struct joinNode *);
   LOCALE void                           PrimeJoin(struct joinNode *);
   LOCALE struct multifieldMarker       *CopyMultifieldMarkers(void *,struct multifieldMarker *);
   LOCALE struct partialMatch           *CreateAlphaMatch(void *,void *,struct multifieldMarker *,
                                                       struct patternNodeHeader *);
   LOCALE void                           TraceErrorToRule(void *,struct joinNode *,char *);
   LOCALE void                           InitializePatternHeader(void *,struct patternNodeHeader *);
   LOCALE void                           MarkRuleNetwork(void *,int);
   LOCALE void                           TagRuleNetwork(void *,long *,long *,long *);
   LOCALE int                            FindEntityInPartialMatch(struct patternEntity *,struct partialMatch *);

#endif




