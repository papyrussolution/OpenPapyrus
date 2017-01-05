   /*******************************************************/
   /*      "C" Language Integrated Production System      */
   /*                                                     */
   /*             CLIPS Version 6.20  01/31/02            */
   /*                                                     */
   /*                   GENERATE MODULE                   */
   /*******************************************************/

/*************************************************************/
/* Purpose: Provides routines for converting field           */
/*   constraints to expressions which can be used            */
/*   in the pattern and join networks.                       */
/*                                                           */
/* Principal Programmer(s):                                  */
/*      Gary D. Riley                                        */
/*                                                           */
/* Contributing Programmer(s):                               */
/*                                                           */
/* Revision History:                                         */
/*                                                           */
/*************************************************************/

#define _GENERATE_SOURCE_

#include <stdio.h>
#define _STDIO_INCLUDED_
#include <stdlib.h>

#include "setup.h"

#if (! RUN_TIME) && (! BLOAD_ONLY) && DEFRULE_CONSTRUCT

#include "constant.h"
#include "envrnmnt.h"
#include "memalloc.h"
#include "symbol.h"
#include "exprnpsr.h"
#include "argacces.h"
#include "extnfunc.h"
#include "router.h"
#include "ruledef.h"
#include "pattern.h"
#include "generate.h"

#if DEFGLOBAL_CONSTRUCT
#include "globlpsr.h"
#endif

/***************************************/
/* LOCAL INTERNAL FUNCTION DEFINITIONS */
/***************************************/

   static void                    ExtractAnds(void *,struct lhsParseNode *,int,
                                              struct expr **,struct expr **);
   static void                    ExtractFieldTest(void *,struct lhsParseNode *,int,
                                                   struct expr **,struct expr **);
   static struct expr            *GetfieldReplace(void *,struct lhsParseNode *);
   static struct expr            *GenPNConstant(void *,struct lhsParseNode *);
   static struct expr            *GenJNConstant(void *,struct lhsParseNode *);
   static struct expr            *GenJNColon(void *,struct lhsParseNode *);
   static struct expr            *GenPNColon(void *,struct lhsParseNode *);
   static struct expr            *GenJNEq(void *,struct lhsParseNode *);
   static struct expr            *GenPNEq(void *,struct lhsParseNode *);
   static struct expr            *GenJNVariableComparison(void *,struct lhsParseNode *,
                                                          struct lhsParseNode *);
   static struct expr            *GenPNVariableComparison(void *,struct lhsParseNode *,
                                                          struct lhsParseNode *);
   static int                     AllVariablesInPattern(struct lhsParseNode *,
                                                        int);
   static int                     AllVariablesInExpression(struct lhsParseNode *,
                                                           int);

/*******************************************************/
/* FieldConversion: Generates join and pattern network */
/*   expressions for a field constraint.               */
/*******************************************************/
globle void FieldConversion(
  void *theEnv,
  struct lhsParseNode *theField,
  struct lhsParseNode *thePattern)
  {
   int testInPatternNetwork = TRUE;
   struct lhsParseNode *patternPtr;
   struct expr *headOfPNExpression, *headOfJNExpression;
   struct expr *lastPNExpression, *lastJNExpression;
   struct expr *tempExpression;
   struct expr *patternNetTest = NULL;
   struct expr *joinNetTest = NULL;

   /*==================================================*/
   /* Consider a NULL pointer to be an internal error. */
   /*==================================================*/

   if (theField == NULL)
     {
      SystemError(theEnv,"ANALYSIS",3);
      EnvExitRouter(theEnv,EXIT_FAILURE);
     }

   /*========================================================*/
   /* Determine if constant testing must be performed in the */
   /* join network. Only possible when a field contains an   */
   /* or ('|') and references are made to variables outside  */
   /* the pattern.                                           */
   /*========================================================*/

   if (theField->bottom != NULL)
     {
      if (theField->bottom->bottom != NULL)
        { testInPatternNetwork = AllVariablesInPattern(theField->bottom,theField->pattern); }
     }

   /*=============================================================*/
   /* Extract pattern and join network expressions. Loop through  */
   /* the or'ed constraints of the field, extracting pattern and  */
   /* join network expressions and adding them to a running list. */
   /*=============================================================*/

   headOfPNExpression = lastPNExpression = NULL;
   headOfJNExpression = lastJNExpression = NULL;

   for (patternPtr = theField->bottom;
        patternPtr != NULL;
        patternPtr = patternPtr->bottom)
     {
      /*=============================================*/
      /* Extract pattern and join network tests from */
      /* the or'ed constraint being examined.        */
      /*=============================================*/

      ExtractAnds(theEnv,patternPtr,testInPatternNetwork,&patternNetTest,&joinNetTest);

      /*=====================================================*/
      /* Add the new pattern network expressions to the list */
      /* of pattern network expressions being constructed.   */
      /*=====================================================*/

      if (patternNetTest != NULL)
        {
         if (lastPNExpression == NULL)
           { headOfPNExpression = patternNetTest; }
         else
           { lastPNExpression->nextArg = patternNetTest; }
         lastPNExpression = patternNetTest;
        }

      /*==================================================*/
      /* Add the new join network expressions to the list */
      /* of join network expressions being constructed.   */
      /*==================================================*/

      if (joinNetTest != NULL)
        {
         if (lastJNExpression == NULL)
           { headOfJNExpression = joinNetTest; }
         else
           { lastJNExpression->nextArg = joinNetTest; }
         lastJNExpression = joinNetTest;
        }
     }

   /*==========================================================*/
   /* If there was more than one expression generated from the */
   /* or'ed field constraints for the pattern network, then    */
   /* enclose the expressions within an "or" function call.    */
   /*==========================================================*/

   if ((headOfPNExpression != NULL) ? (headOfPNExpression->nextArg != NULL) : FALSE)
     {
      tempExpression = GenConstant(theEnv,FCALL,ExpressionData(theEnv)->PTR_OR);
      tempExpression->argList = headOfPNExpression;
      headOfPNExpression = tempExpression;
     }

   /*==========================================================*/
   /* If there was more than one expression generated from the */
   /* or'ed field constraints for the join network, then       */
   /* enclose the expressions within an "or" function call.    */
   /*==========================================================*/

   if ((headOfJNExpression != NULL) ? (headOfJNExpression->nextArg != NULL) : FALSE)
     {
      tempExpression = GenConstant(theEnv,FCALL,ExpressionData(theEnv)->PTR_OR);
      tempExpression->argList = headOfJNExpression;
      headOfJNExpression = tempExpression;
     }

   /*===============================================================*/
   /* If the field constraint binds a variable that was previously  */
   /* bound somewhere in the LHS of the rule, then generate an      */
   /* expression to compare this binding occurrence of the variable */
   /* to the previous binding occurrence.                           */
   /*===============================================================*/

   if (((theField->type == MF_VARIABLE) || (theField->type == SF_VARIABLE)) &&
       (theField->referringNode != NULL))
     {
      /*================================================================*/
      /* If the previous variable reference is within the same pattern, */
      /* then the variable comparison can occur in the pattern network. */
      /*================================================================*/

      if (theField->referringNode->pattern == theField->pattern)
        {
         tempExpression = GenPNVariableComparison(theEnv,theField,theField->referringNode);
         headOfPNExpression = CombineExpressions(theEnv,tempExpression,headOfPNExpression);
        }

      /*====================================*/
      /* Otherwise, the variable comparison */
      /* must occur in the join network.    */
      /*====================================*/

      else if (theField->referringNode->pattern > 0)
        {
         tempExpression = GenJNVariableComparison(theEnv,theField,theField->referringNode);
         headOfJNExpression = CombineExpressions(theEnv,tempExpression,headOfJNExpression);
        }
     }

   /*======================================================*/
   /* Attach the pattern network expressions to the field. */
   /*======================================================*/

   theField->networkTest = headOfPNExpression;

   /*=====================================================*/
   /* Attach the join network expressions to the pattern. */
   /*=====================================================*/

   thePattern->networkTest = CombineExpressions(theEnv,thePattern->networkTest,headOfJNExpression);
  }

/****************************************************************************/
/* ExtractAnds: Loops through a single set of subfields bound together by   */
/*   an & connective constraint in a field and generates expressions needed */
/*   for testing conditions in the pattern and join network.                */
/****************************************************************************/
static void ExtractAnds(
  void *theEnv,
  struct lhsParseNode *andField,
  int testInPatternNetwork,
  struct expr **patternNetTest,
  struct expr **joinNetTest)
  {
   struct expr *newPNTest, *newJNTest;

   /*=================================================*/
   /* Before starting, the subfield has no pattern or */
   /* join network expressions associated with it.    */
   /*=================================================*/

   *patternNetTest = NULL;
   *joinNetTest = NULL;

   /*=========================================*/
   /* Loop through each of the subfields tied */
   /* together by the & constraint.           */
   /*=========================================*/

   for (;
        andField != NULL;
        andField = andField->right)
     {
      /*======================================*/
      /* Extract the pattern and join network */
      /* expressions from the subfield.       */
      /*======================================*/

      ExtractFieldTest(theEnv,andField,testInPatternNetwork,&newPNTest,&newJNTest);

      /*=================================================*/
      /* Add the new expressions to the list of pattern  */
      /* and join network expressions being constructed. */
      /*=================================================*/

      *patternNetTest = CombineExpressions(theEnv,*patternNetTest,newPNTest);
      *joinNetTest = CombineExpressions(theEnv,*joinNetTest,newJNTest);
     }
  }

/************************************************************************/
/* ExtractFieldTest: Generates the pattern or join network expression   */
/*   associated with the basic field constraints: constants, predicate, */
/*   return value, and variable constraints. Based on the context in    */
/*   which a constraint is used, some constraints may be tested in the  */
/*   pattern network while other constraints must be tested in the join */
/*   network. Constraints which refer to variables in other patterns    */
/*   must be tested in the join network. The predicate constraint       */
/*   associated with a test CE is tested in the join network (even if   */
/*   all the variables it refers to are contained in the previous       */
/*   pattern CE). If one of the or'ed constraints in a field refers to  */
/*   a binding occurrence of a variable in another pattern, then the    */
/*   other constraints in the field must be tested in the join network  */
/*   (this is how some constant constraint tests must occasionally be   */
/*   performed in the join network).                                    */
/************************************************************************/
static void ExtractFieldTest(
  void *theEnv,
  struct lhsParseNode *theField,
  int testInPatternNetwork,
  struct expr **patternNetTest,
  struct expr **joinNetTest)
  {
   *patternNetTest = NULL;
   *joinNetTest = NULL;

   /*==========================================================*/
   /* Generate a network expression for a constant constraint. */
   /*==========================================================*/

   if ((theField->type == STRING) || (theField->type == SYMBOL) ||
#if OBJECT_SYSTEM
       (theField->type == INSTANCE_NAME) ||
#endif
       (theField->type == FLOAT) || (theField->type == INTEGER))
     {
      if (testInPatternNetwork == TRUE)
        { *patternNetTest = GenPNConstant(theEnv,theField); }
      else
        { *joinNetTest = GenJNConstant(theEnv,theField); }
     }

   /*===========================================================*/
   /* Generate a network expression for a predicate constraint. */
   /*===========================================================*/

   else if (theField->type == PREDICATE_CONSTRAINT)
     {
      if ((testInPatternNetwork == TRUE) &&
          (AllVariablesInExpression(theField->expression,theField->pattern) == TRUE))
        { *patternNetTest = GenPNColon(theEnv,theField); }
      else
        { *joinNetTest = GenJNColon(theEnv,theField); }
     }

   /*==============================================================*/
   /* Generate a network expression for a return value constraint. */
   /*==============================================================*/

   else if (theField->type == RETURN_VALUE_CONSTRAINT)
     {
      if ((testInPatternNetwork == TRUE) &&
          (AllVariablesInExpression(theField->expression,theField->pattern) == TRUE))
        { *patternNetTest = GenPNEq(theEnv,theField); }
      else
        { *joinNetTest = GenJNEq(theEnv,theField); }
     }

   /*=====================================================================*/
   /* Generate a network expression for a variable comparison constraint. */
   /*=====================================================================*/

   else if ((theField->type == SF_VARIABLE) || (theField->type == MF_VARIABLE))
     {
      if ((testInPatternNetwork == TRUE) &&
          ((theField->referringNode != NULL) ?
           (theField->referringNode->pattern == theField->pattern) :
           FALSE))
        { *patternNetTest = GenPNVariableComparison(theEnv,theField,theField->referringNode); }
      else
        { *joinNetTest = GenJNVariableComparison(theEnv,theField,theField->referringNode); }
     }
  }

/*********************************************************/
/* GenPNConstant: Generates an expression for use in the */
/*  pattern network of a data entity (such as a fact or  */
/*  instance). The expression generated is for comparing */
/*  a constant value against a specified slot/field in   */
/*  the data entity for equality or inequality.          */
/*********************************************************/
static struct expr *GenPNConstant(
  void *theEnv,
  struct lhsParseNode *theField)
  {
   struct expr *top;

   /*===============================================*/
   /* If the pattern parser is capable of creating  */
   /* a specialized test, then call the function to */
   /* generate the pattern network test and return  */
   /* the expression generated.                     */
   /*===============================================*/

   if (theField->patternType->genPNConstantFunction != NULL)
     { return (*theField->patternType->genPNConstantFunction)(theEnv,theField); }

   /*===================================================*/
   /* Otherwise, generate a test which uses the eq/neq  */
   /* function to compare the pattern field/slot to the */
   /* constant and then return the expression.          */
   /*===================================================*/

   if (theField->negated)
     { top = GenConstant(theEnv,FCALL,ExpressionData(theEnv)->PTR_NEQ); }
   else
     { top = GenConstant(theEnv,FCALL,ExpressionData(theEnv)->PTR_EQ); }

   top->argList = (*theField->patternType->genGetPNValueFunction)(theEnv,theField);
   top->argList->nextArg = GenConstant(theEnv,theField->type,theField->value);

   return(top);
  }

/************************************************************/
/* GenJNConstant: Generates an expression for use in the    */
/*  join network. The expression generated is for comparing */
/*  a constant value against a specified slot/field in the  */
/*  data entity for equality or inequality.                 */
/************************************************************/
static struct expr *GenJNConstant(
  void *theEnv,
  struct lhsParseNode *theField)
  {
   struct expr *top;

   /*===============================================*/
   /* If the pattern parser is capable of creating  */
   /* a specialized test, then call the function to */
   /* generate the join network test and return the */
   /* expression generated.                         */
   /*===============================================*/

   if (theField->patternType->genJNConstantFunction != NULL)
     { return (*theField->patternType->genJNConstantFunction)(theEnv,theField); }

   /*===================================================*/
   /* Otherwise, generate a test which uses the eq/neq  */
   /* function to compare the pattern field/slot to the */
   /* constant and then return the expression.          */
   /*===================================================*/

   if (theField->negated)
     { top = GenConstant(theEnv,FCALL,ExpressionData(theEnv)->PTR_NEQ); }
   else
     { top = GenConstant(theEnv,FCALL,ExpressionData(theEnv)->PTR_EQ); }

   top->argList = (*theField->patternType->genGetJNValueFunction)(theEnv,theField);
   top->argList->nextArg = GenConstant(theEnv,theField->type,theField->value);

   return(top);
  }

/******************************************************/
/* GenJNColon: Generates an expression for use in the */
/*  join network. The expression generated is for a   */
/*  predicate field constraint (the : constraint).    */
/******************************************************/
static struct expr *GenJNColon(
  void *theEnv,
  struct lhsParseNode *theField)
  {
   struct expr *top, *conversion;

   /*==================================================*/
   /* Replace variables with function calls to extract */
   /* the appropriate value from the data entity.      */
   /*==================================================*/

   conversion = GetvarReplace(theEnv,theField->expression);

   /*================================================*/
   /* If the predicate constraint is negated by a ~, */
   /* then wrap a "not" function call around the     */
   /* expression before returning it. Otherwise,     */
   /* just return the expression.                    */
   /*================================================*/

   if (theField->negated)
     {
      top = GenConstant(theEnv,FCALL,ExpressionData(theEnv)->PTR_NOT);
      top->argList = conversion;
     }
   else
     { top = conversion; }

   return(top);
  }

/******************************************************/
/* GenPNColon: Generates an expression for use in the */
/*  pattern network. The expression generated is for  */
/*  a predicate field constraint (the : constraint).  */
/******************************************************/
static struct expr *GenPNColon(
  void *theEnv,
  struct lhsParseNode *theField)
  {
   struct expr *top, *conversion;

   /*==================================================*/
   /* Replace variables with function calls to extract */
   /* the appropriate value from the data entity.      */
   /*==================================================*/

   conversion = GetfieldReplace(theEnv,theField->expression);

   /*================================================*/
   /* If the predicate constraint is negated by a ~, */
   /* then wrap a "not" function call around the     */
   /* expression before returning it. Otherwise,     */
   /* just return the expression.                    */
   /*================================================*/

   if (theField->negated)
     {
      top = GenConstant(theEnv,FCALL,ExpressionData(theEnv)->PTR_NOT);
      top->argList = conversion;
     }
   else
     { top = conversion; }

   return(top);
  }

/******************************************************/
/* GenJNEq: Generates an expression for use in the    */
/*  join network. The expression generated is for a   */
/*  return value field constraint (the = constraint). */
/******************************************************/
static struct expr *GenJNEq(
  void *theEnv,
  struct lhsParseNode *theField)
  {
   struct expr *top, *conversion;

   /*==================================================*/
   /* Replace variables with function calls to extract */
   /* the appropriate value from the data entity.      */
   /*==================================================*/

   conversion = GetvarReplace(theEnv,theField->expression);

   /*============================================================*/
   /* If the return value constraint is negated by a ~, then use */
   /* the neq function to compare the value of the field to the  */
   /* value returned by the function call. Otherwise, use eq to  */
   /* compare the two values.                                    */
   /*============================================================*/

   if (theField->negated)
     { top = GenConstant(theEnv,FCALL,ExpressionData(theEnv)->PTR_NEQ); }
   else
     { top = GenConstant(theEnv,FCALL,ExpressionData(theEnv)->PTR_EQ); }

   top->argList = (*theField->patternType->genGetJNValueFunction)(theEnv,theField);
   top->argList->nextArg = conversion;

   return(top);
  }

/*******************************************************/
/* GenPNEq: Generates an expression for use in the     */
/*  pattern network. The expression generated is for a */
/*  return value field constraint (the = constraint).  */
/*******************************************************/
static struct expr *GenPNEq(
  void *theEnv,
  struct lhsParseNode *theField)
  {
   struct expr *top, *conversion;

   /*==================================================*/
   /* Replace variables with function calls to extract */
   /* the appropriate value from the data entity.      */
   /*==================================================*/

   conversion = GetfieldReplace(theEnv,theField->expression);

   /*============================================================*/
   /* If the return value constraint is negated by a ~, then use */
   /* the neq function to compare the value of the field to the  */
   /* value returned by the function call. Otherwise, use eq to  */
   /* compare the two values.                                    */
   /*============================================================*/

   if (theField->negated)
     { top = GenConstant(theEnv,FCALL,ExpressionData(theEnv)->PTR_NEQ); }
   else
     { top = GenConstant(theEnv,FCALL,ExpressionData(theEnv)->PTR_EQ); }

   top->argList = (*theField->patternType->genGetPNValueFunction)(theEnv,theField);
   top->argList->nextArg = conversion;

   return(top);
  }

/******************************************************************/
/* GetvarReplace: Replaces occurences of variables in expressions */
/*   with function calls that will extract the variable's value   */
/*   from a partial match (i.e. from information stored in the    */
/*   join network or the activation of the rule).                 */
/******************************************************************/
globle struct expr *GetvarReplace(
  void *theEnv,
  struct lhsParseNode *nodeList)
  {
   struct expr *newList;

   /*====================================*/
   /* Return NULL for a NULL pointer     */
   /* (i.e. nothing has to be replaced). */
   /*====================================*/

   if (nodeList == NULL) return(NULL);

   /*=====================================================*/
   /* Create an expression data structure and recursively */
   /* replace variables in its argument list and next     */
   /* argument links.                                     */
   /*=====================================================*/

   newList = get_struct(theEnv,expr);
   newList->type = nodeList->type;
   newList->value = nodeList->value;
   newList->nextArg = GetvarReplace(theEnv,nodeList->right);
   newList->argList = GetvarReplace(theEnv,nodeList->bottom);

   /*=========================================================*/
   /* If the present node being examined is either a local or */
   /* global variable, then replace it with a function call   */
   /* that will return the variable's value.                  */
   /*=========================================================*/

   if ((nodeList->type == SF_VARIABLE) || (nodeList->type == MF_VARIABLE))
     {

      (*nodeList->referringNode->patternType->replaceGetJNValueFunction)
        (theEnv,newList,nodeList->referringNode);
     }
#if DEFGLOBAL_CONSTRUCT
   else if (newList->type == GBL_VARIABLE)
     { ReplaceGlobalVariable(theEnv,newList); }
#endif

   /*====================================================*/
   /* Return the expression with its variables replaced. */
   /*====================================================*/

   return(newList);
  }

/**********************************************************************/
/* GetfieldReplace: Replaces occurences of variables in expressions   */
/*   with function calls that will extract the variable's value       */
/*   given a pointer to the data entity that contains the value (i.e. */
/*   from information stored in the pattern network).                 */
/**********************************************************************/
static struct expr *GetfieldReplace(
  void *theEnv,
  struct lhsParseNode *nodeList)
  {
   struct expr *newList;

   /*====================================*/
   /* Return NULL for a NULL pointer     */
   /* (i.e. nothing has to be replaced). */
   /*====================================*/

   if (nodeList == NULL) return(NULL);

   /*=====================================================*/
   /* Create an expression data structure and recursively */
   /* replace variables in its argument list and next     */
   /* argument links.                                     */
   /*=====================================================*/

   newList = get_struct(theEnv,expr);
   newList->type = nodeList->type;
   newList->value = nodeList->value;
   newList->nextArg = GetfieldReplace(theEnv,nodeList->right);
   newList->argList = GetfieldReplace(theEnv,nodeList->bottom);

   /*=========================================================*/
   /* If the present node being examined is either a local or */
   /* global variable, then replace it with a function call   */
   /* that will return the variable's value.                  */
   /*=========================================================*/

   if ((nodeList->type == SF_VARIABLE) || (nodeList->type == MF_VARIABLE))
     {
      (*nodeList->referringNode->patternType->replaceGetPNValueFunction)
         (theEnv,newList,nodeList->referringNode);
     }
#if DEFGLOBAL_CONSTRUCT
   else if (newList->type == GBL_VARIABLE)
     { ReplaceGlobalVariable(theEnv,newList); }
#endif

   /*====================================================*/
   /* Return the expression with its variables replaced. */
   /*====================================================*/

   return(newList);
  }

/**************************************************************/
/* GenJNVariableComparison: Generates a join network test for */
/*   comparing two variables found in different patterns.     */
/**************************************************************/
static struct expr *GenJNVariableComparison(
  void *theEnv,
  struct lhsParseNode *selfNode,
  struct lhsParseNode *referringNode)
  {
   struct expr *top;

   /*========================================================*/
   /* If either pattern is missing a function for generating */
   /* the appropriate test, then no test is generated.       */
   /*========================================================*/

   if ((selfNode->patternType->genCompareJNValuesFunction == NULL) ||
       (referringNode->patternType->genCompareJNValuesFunction == NULL))
     { return(NULL); }

   /*=====================================================*/
   /* If both patterns are of the same type, then use the */
   /* special function for generating the join test.      */
   /*=====================================================*/

   if (selfNode->patternType->genCompareJNValuesFunction ==
       referringNode->patternType->genCompareJNValuesFunction)

     {
      return (*selfNode->patternType->genCompareJNValuesFunction)(theEnv,selfNode,
                                                                  referringNode);
     }

   /*===========================================================*/
   /* If the patterns are of different types, then generate a   */
   /* join test by using the eq/neq function with its arguments */
   /* being function calls to retrieve the appropriate values   */
   /* from the patterns.                                        */
   /*===========================================================*/

   if (selfNode->negated) top = GenConstant(theEnv,FCALL,ExpressionData(theEnv)->PTR_NEQ);
   else top = GenConstant(theEnv,FCALL,ExpressionData(theEnv)->PTR_EQ);

   top->argList = (*selfNode->patternType->genGetJNValueFunction)(theEnv,selfNode);
   top->argList->nextArg = (*referringNode->patternType->genGetJNValueFunction)(theEnv,referringNode);

   return(top);
  }

/*************************************************************/
/* GenPNVariableComparison: Generates a pattern network test */
/*   for comparing two variables found in the same pattern.  */
/*************************************************************/
static struct expr *GenPNVariableComparison(
  void *theEnv,
  struct lhsParseNode *selfNode,
  struct lhsParseNode *referringNode)
  {
   if (selfNode->patternType->genComparePNValuesFunction != NULL)
     {
      return (*selfNode->patternType->genComparePNValuesFunction)(theEnv,selfNode,referringNode);
     }

   return(NULL);
  }

/************************************************************/
/* AllVariablesInPattern: Determines if all of the variable */
/*   references in a field constraint can be referenced     */
/*   within thepattern in which the field is contained.     */
/************************************************************/
static int AllVariablesInPattern(
  struct lhsParseNode *orField,
  int pattern)
  {
   struct lhsParseNode *andField;

   /*=========================================*/
   /* Loop through each of the | constraints. */
   /*=========================================*/

   for (;
        orField != NULL;
        orField = orField->bottom)
     {
      /*=========================================*/
      /* Loop through each of the & constraints. */
      /*=========================================*/

      for (andField = orField;
           andField != NULL;
           andField = andField->right)
        {
         /*========================================================*/
         /* If a variable is found, make sure the pattern in which */
         /* the variable was previously bound is the same as the   */
         /* pattern being checked.                                 */
         /*========================================================*/

         if ((andField->type == SF_VARIABLE) || (andField->type == MF_VARIABLE))
           { if (andField->referringNode->pattern != pattern) return(FALSE); }

         /*========================================================*/
         /* Check predicate and return value constraints to see    */
         /* that all variables can be referenced from the pattern. */
         /*========================================================*/

         else if ((andField->type == PREDICATE_CONSTRAINT) ||
                  (andField->type == RETURN_VALUE_CONSTRAINT))
           {
            if (AllVariablesInExpression(andField->expression,pattern) == FALSE)
              { return(FALSE); }
           }
        }
     }

   /*=====================================*/
   /* All variables in the field can be   */
   /* referenced from within the pattern. */
   /*=====================================*/

   return(TRUE);
  }

/**************************************************************************/
/* AllVariablesInExpression: Determines if all of the variable references */
/*   in an expression can be referenced within the pattern in which the   */
/*   expression is contained.                                             */
/**************************************************************************/
static int AllVariablesInExpression(
  struct lhsParseNode *theExpression,
  int pattern)
  {
   /*==========================================*/
   /* Check all expressions in the right link. */
   /*==========================================*/

   for (;
        theExpression != NULL;
        theExpression = theExpression->right)
     {
      /*========================================================*/
      /* If a variable is found, make sure the pattern in which */
      /* the variable is bound is the same as the pattern being */
      /* checked.                                               */
      /*========================================================*/

      if ((theExpression->type == SF_VARIABLE) ||
          (theExpression->type == MF_VARIABLE))
        { if (theExpression->referringNode->pattern != pattern) return(FALSE); }

      /*=======================================================*/
      /* Recursively check all expressions in the bottom link. */
      /*=======================================================*/

      if (AllVariablesInExpression(theExpression->bottom,pattern) == FALSE)
        { return(FALSE); }
     }

   /*========================================*/
   /* All variables in the expression can be */
   /* referenced from within the pattern.    */
   /*========================================*/

   return(TRUE);
  }

#endif /* (! RUN_TIME) && (! BLOAD_ONLY) && DEFRULE_CONSTRUCT */


