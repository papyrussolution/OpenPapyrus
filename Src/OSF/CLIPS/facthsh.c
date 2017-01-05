   /*******************************************************/
   /*      "C" Language Integrated Production System      */
   /*                                                     */
   /*             CLIPS Version 6.24  05/17/06            */
   /*                                                     */
   /*                 FACT HASHING MODULE                 */
   /*******************************************************/

/*************************************************************/
/* Purpose: Provides routines for maintaining a fact hash    */
/*   table so that duplication of facts can quickly be       */
/*   determined.                                             */
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
/*************************************************************/

#define _FACTHSH_SOURCE_

#include <stdio.h>
#define _STDIO_INCLUDED_
#include <stdlib.h>

#include "setup.h"

#if DEFTEMPLATE_CONSTRUCT

#include "constant.h"
#include "memalloc.h"
#include "router.h"
#include "envrnmnt.h"

#if DEFRULE_CONSTRUCT
#include "lgcldpnd.h"
#endif

#include "facthsh.h"

/***************************************/
/* LOCAL INTERNAL FUNCTION DEFINITIONS */
/***************************************/

   static struct fact            *FactExists(void *,struct fact *,int);

/************************************************/
/* HashFact: Returns the hash value for a fact. */
/************************************************/
int HashFact(
  struct fact *theFact)
  {
   int count = 0;
   int hashValue;

   /*============================================*/
   /* Get a hash value for the deftemplate name. */
   /*============================================*/

   count += (int) HashSymbol(ValueToString(theFact->whichDeftemplate->header.name),
                       SIZE_FACT_HASH);

   /*=================================================*/
   /* Add in the hash value for the rest of the fact. */
   /*=================================================*/

   count += (int) HashMultifield(&theFact->theProposition,SIZE_FACT_HASH);

   /*================================*/
   /* Make sure the hash value falls */
   /* in the appropriate range.      */
   /*================================*/

   hashValue = (int) (count % SIZE_FACT_HASH);
   if (hashValue < 0) hashValue = - hashValue;

   /*========================*/
   /* Return the hash value. */
   /*========================*/

   return(hashValue);
  }

/**********************************************/
/* FactExists: Determines if a specified fact */
/*   already exists in the fact hash table.   */
/**********************************************/
static struct fact *FactExists(
  void *theEnv,
  struct fact *theFact,
  int hashValue)
  {
   struct factHashEntry *theFactHash;

   for (theFactHash = FactData(theEnv)->FactHashTable[hashValue];
        theFactHash != NULL;
        theFactHash = theFactHash->next)
     {
      if ((theFact->whichDeftemplate == theFactHash->theFact->whichDeftemplate) ?
          MultifieldsEqual(&theFact->theProposition,
                           &theFactHash->theFact->theProposition) : FALSE)
        { return(theFactHash->theFact); }
     }

   return(NULL);
  }

/************************************************************/
/* AddHashedFact: Adds a fact entry to the fact hash table. */
/************************************************************/
globle void AddHashedFact(
  void *theEnv,
  struct fact *theFact,
  int hashValue)
  {
   struct factHashEntry *newhash, *temp;

   newhash = get_struct(theEnv,factHashEntry);
   newhash->theFact = theFact;

   temp = FactData(theEnv)->FactHashTable[hashValue];
   FactData(theEnv)->FactHashTable[hashValue] = newhash;
   newhash->next = temp;
  }

/******************************************/
/* RemoveHashedFact: Removes a fact entry */
/*   from the fact hash table.            */
/******************************************/
globle intBool RemoveHashedFact(
  void *theEnv,
  struct fact *theFact)
  {
   int hashValue;
   struct factHashEntry *hptr, *prev;

   hashValue = HashFact(theFact);

   for (hptr = FactData(theEnv)->FactHashTable[hashValue], prev = NULL;
        hptr != NULL;
        hptr = hptr->next)
     {
      if (hptr->theFact == theFact)
        {
         if (prev == NULL)
           {
            FactData(theEnv)->FactHashTable[hashValue] = hptr->next;
            rtn_struct(theEnv,factHashEntry,hptr);
            return(1);
           }
         else
           {
            prev->next = hptr->next;
            rtn_struct(theEnv,factHashEntry,hptr);
            return(1);
           }
        }
      prev = hptr;
     }

   return(0);
  }

/*****************************************************/
/* HandleFactDuplication: Determines if a fact to be */
/*   added to the fact-list is a duplicate entry and */
/*   takes appropriate action based on the current   */
/*   setting of the fact-duplication flag.           */
/*****************************************************/
globle int HandleFactDuplication(
  void *theEnv,
  void *theFact)
  {
   struct fact *tempPtr;
   int hashValue;

   hashValue = HashFact((struct fact *) theFact);

   if (FactData(theEnv)->FactDuplication) return(hashValue);

   tempPtr = FactExists(theEnv,(struct fact *) theFact,hashValue);
   if (tempPtr == NULL) return(hashValue);

   ReturnFact(theEnv,(struct fact *) theFact);
#if DEFRULE_CONSTRUCT
   AddLogicalDependencies(theEnv,(struct patternEntity *) tempPtr,TRUE);
#endif
   return(-1);
  }

/*******************************************/
/* EnvGetFactDuplication: C access routine */
/*   for the get-fact-duplication command. */
/*******************************************/
globle intBool EnvGetFactDuplication(
  void *theEnv)
  {   
   return(FactData(theEnv)->FactDuplication); 
  }

/*******************************************/
/* EnvSetFactDuplication: C access routine */
/*   for the set-fact-duplication command. */
/*******************************************/
globle intBool EnvSetFactDuplication(
  void *theEnv,
  int value)
  {
   int ov;

   ov = FactData(theEnv)->FactDuplication;
   FactData(theEnv)->FactDuplication = value;
   return(ov);
  }

/**************************************************/
/* InitializeFactHashTable: Initializes the table */
/*   entries in the fact hash table to NULL.      */
/**************************************************/
globle void InitializeFactHashTable(
   void *theEnv)
   {
    int i;

    FactData(theEnv)->FactHashTable = (struct factHashEntry **)
                    gm3(theEnv,sizeof (struct factHashEntry *) * SIZE_FACT_HASH);

    if (FactData(theEnv)->FactHashTable == NULL) EnvExitRouter(theEnv,EXIT_FAILURE);

    for (i = 0; i < SIZE_FACT_HASH; i++) FactData(theEnv)->FactHashTable[i] = NULL;
   }

#if DEVELOPER

/*****************************************************/
/* ShowFactHashTable: Displays the number of entries */
/*   in each slot of the fact hash table.            */
/*****************************************************/
globle void ShowFactHashTable(
   void *theEnv)
   {
    int i, count;
    struct factHashEntry *theEntry;
    char buffer[20];

    for (i = 0; i < SIZE_FACT_HASH; i++)
      {
       for (theEntry =  FactData(theEnv)->FactHashTable[i], count = 0;
            theEntry != NULL;
            theEntry = theEntry->next)
         { count++; }

       if (count != 0)
         {
          sprintf(buffer,"%4d: %4d\n",i,count);
          EnvPrintRouter(theEnv,WDISPLAY,buffer);
         }
      }
   }

#endif /* DEVELOPER */

#endif /* DEFTEMPLATE_CONSTRUCT */

