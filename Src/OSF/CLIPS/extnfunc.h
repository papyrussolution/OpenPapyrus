   /*******************************************************/
   /*      "C" Language Integrated Production System      */
   /*                                                     */
   /*             CLIPS Version 6.24  06/05/06            */
   /*                                                     */
   /*            EXTERNAL FUNCTIONS HEADER FILE           */
   /*******************************************************/

/*************************************************************/
/* Purpose: Routines for adding new user or system defined   */
/*   functions.                                              */
/*                                                           */
/* Principal Programmer(s):                                  */
/*      Gary D. Riley                                        */
/*                                                           */
/* Contributing Programmer(s):                               */
/*                                                           */
/* Revision History:                                         */
/*                                                           */
/*      6.24: Renamed BOOLEAN macro type to intBool.         */
/*                                                           */
/*************************************************************/

#ifndef _H_extnfunc

#define _H_extnfunc

#ifndef _H_symbol
#include "symbol.h"
#endif
#ifndef _H_expressn
#include "expressn.h"
#endif

#include "userdata.h"

struct FunctionDefinition
  {
   struct symbolHashNode *callFunctionName;
   char *actualFunctionName;
   char returnValueType;
   int (*functionPointer)(void);
   struct expr *(*parser)(void *,struct expr *,char *);
   char *restrictions;
   short int overloadable;
   short int sequenceuseok;
   short int environmentAware;
   short int bsaveIndex;
   struct FunctionDefinition *next;
   struct userData *usrData;
  };

#define ValueFunctionType(target) (((struct FunctionDefinition *) target)->returnValueType)
#define ExpressionFunctionType(target) (((struct FunctionDefinition *) ((target)->value))->returnValueType)
#define ExpressionFunctionPointer(target) (((struct FunctionDefinition *) ((target)->value))->functionPointer)
#define ExpressionFunctionCallName(target) (((struct FunctionDefinition *) ((target)->value))->callFunctionName)
#define ExpressionFunctionRealName(target) (((struct FunctionDefinition *) ((target)->value))->actualFunctionName)

#define PTIF (int (*)(void))
#define PTIEF (int (*)(void *))

/*==================*/
/* ENVIRONMENT DATA */
/*==================*/

#define EXTERNAL_FUNCTION_DATA 50

struct externalFunctionData
  { 
   struct FunctionDefinition *ListOfFunctions;
   struct FunctionHash **FunctionHashtable;
  };

#define ExternalFunctionData(theEnv) ((struct externalFunctionData *) GetEnvironmentData(theEnv,EXTERNAL_FUNCTION_DATA))

#ifdef LOCALE
#undef LOCALE
#endif

#ifdef _EXTNFUNC_SOURCE_
#define LOCALE
#else
#define LOCALE extern
#endif

#ifdef LOCALE
struct FunctionHash
  {
   struct FunctionDefinition *fdPtr;
   struct FunctionHash *next;
  };

#define SIZE_FUNCTION_HASH 517
#endif

#if ENVIRONMENT_API_ONLY
#define DefineFunction(theEnv,a,b,c,d) EnvDefineFunction(theEnv,a,b,c,d)
#define DefineFunction2(theEnv,a,b,c,d,e) EnvDefineFunction2(theEnv,a,b,c,d,e)
#else
   LOCALE int                            DefineFunction(char *,int,int (*)(void),char *);
   LOCALE int                            DefineFunction2(char *,int,int (*)(void),char *,char *);
#endif

   LOCALE void                           InitializeExternalFunctionData(void *);
   LOCALE int                            EnvDefineFunction(void *,char *,int,
                                                           int (*)(void *),char *);
   LOCALE int                            EnvDefineFunction2(void *,char *,int,
                                                            int (*)(void *),char *,char *);
   LOCALE int                            DefineFunction3(void *,char *,int,
                                                         int (*)(void *),char *,char *,intBool);
   LOCALE int                            AddFunctionParser(void *,char *,
                                                           struct expr *(*)( void *,struct expr *,char *));
   LOCALE int                            RemoveFunctionParser(void *,char *);
   LOCALE int                            FuncSeqOvlFlags(void *,char *,int,int);
   LOCALE struct FunctionDefinition     *GetFunctionList(void *);
   LOCALE void                           InstallFunctionList(void *,struct FunctionDefinition *);
   LOCALE struct FunctionDefinition     *FindFunction(void *,char *);
   LOCALE int                            GetNthRestriction(struct FunctionDefinition *,int);
   LOCALE char                          *GetArgumentTypeName(int);
   LOCALE int                            UndefineFunction(void *,char *);
   LOCALE int                            GetMinimumArgs(struct FunctionDefinition *);
   LOCALE int                            GetMaximumArgs(struct FunctionDefinition *);

#endif



