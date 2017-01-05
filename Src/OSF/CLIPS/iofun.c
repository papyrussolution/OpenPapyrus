   /*******************************************************/
   /*      "C" Language Integrated Production System      */
   /*                                                     */
   /*             CLIPS Version 6.24  07/01/05            */
   /*                                                     */
   /*                 I/O FUNCTIONS MODULE                */
   /*******************************************************/

/*************************************************************/
/* Purpose: Contains the code for several I/O functions      */
/*   including printout, read, open, close, remove, rename,  */
/*   format, and readline.                                   */
/*                                                           */
/* Principal Programmer(s):                                  */
/*      Brian L. Donnell                                     */
/*      Gary D. Riley                                        */
/*      Bebe Ly                                              */
/*                                                           */
/* Contributing Programmer(s):                               */
/*                                                           */
/* Revision History:                                         */
/*      6.24: Added the get-char, set-locale, and            */
/*            read-number functions.                         */
/*                                                           */
/*            Modified printing of floats in the format      */
/*            function to use the locale from the set-locale */
/*            function.                                      */
/*                                                           */
/*            Moved IllegalLogicalNameMessage function to    */
/*            argacces.c.                                    */
/*                                                           */
/*************************************************************/

#define _IOFUN_SOURCE_

#include "setup.h"

#if EXT_IO
#include <locale.h>
#include <stdlib.h>
#include <ctype.h>
#endif

#include <stdio.h>
#define _STDIO_INCLUDED_
#include <string.h>

#include "envrnmnt.h"
#include "router.h"
#include "strngrtr.h"
#include "filertr.h"
#include "argacces.h"
#include "extnfunc.h"
#include "scanner.h"
#include "constant.h"
#include "memalloc.h"
#include "commline.h"
#include "sysdep.h"
#include "utility.h"

#include "iofun.h"

/***************/
/* DEFINITIONS */
/***************/

#define FORMAT_MAX 512
#define FLAG_MAX    80

/********************/
/* ENVIRONMENT DATA */
/********************/

#define IO_FUNCTION_DATA 64

struct IOFunctionData
  { 
   void *locale;
  };

#define IOFunctionData(theEnv) ((struct IOFunctionData *) GetEnvironmentData(theEnv,IO_FUNCTION_DATA))

/****************************************/
/* LOCAL INTERNAL FUNCTION DEFINITIONS  */
/****************************************/

#if BASIC_IO
   static void             ReadTokenFromStdin(void *,struct token *);
#endif
#if EXT_IO
   static char            *ControlStringCheck(void *,int);
   static char             FindFormatFlag(char *,unsigned *,char *,int *);
   static char            *PrintFormatFlag(void *,char *,int,int,int);
   static char            *FillBuffer(void *,char *,int *,unsigned *);
   static void             ReadNumber(void *,char *,struct token *,int);
#endif

/**************************************/
/* IOFunctionDefinitions: Initializes */
/*   the I/O functions.               */
/**************************************/
globle void IOFunctionDefinitions(
  void *theEnv)
  {
   AllocateEnvironmentData(theEnv,IO_FUNCTION_DATA,sizeof(struct IOFunctionData),NULL);
   
   IOFunctionData(theEnv)->locale = (SYMBOL_HN *) EnvAddSymbol(theEnv,setlocale(LC_ALL,NULL));
   IncrementSymbolCount(IOFunctionData(theEnv)->locale);

#if ! RUN_TIME
#if BASIC_IO
   EnvDefineFunction2(theEnv,"printout",   'v', PTIEF PrintoutFunction, "PrintoutFunction", "1*");
   EnvDefineFunction2(theEnv,"read",       'u', PTIEF ReadFunction,  "ReadFunction", "*1");
   EnvDefineFunction2(theEnv,"open",       'b', OpenFunction,  "OpenFunction", "23*k");
   EnvDefineFunction2(theEnv,"close",      'b', CloseFunction, "CloseFunction", "*1");
   EnvDefineFunction2(theEnv,"get-char",   'i', GetCharFunction, "GetCharFunction", "*1");
#endif

#if EXT_IO
   EnvDefineFunction2(theEnv,"remove",   'b', RemoveFunction,  "RemoveFunction", "11k");
   EnvDefineFunction2(theEnv,"rename",   'b', RenameFunction, "RenameFunction", "22k");
   EnvDefineFunction2(theEnv,"format",   's', PTIEF FormatFunction, "FormatFunction", "2**us");
   EnvDefineFunction2(theEnv,"readline", 'k', PTIEF ReadlineFunction, "ReadlineFunction", "*1");
   EnvDefineFunction2(theEnv,"set-locale", 'u', PTIEF SetLocaleFunction,  "SetLocaleFunction", "*1");
   EnvDefineFunction2(theEnv,"read-number",       'u', PTIEF ReadNumberFunction,  "ReadNumberFunction", "*1");
#endif
#else
#if MAC_MCW || IBM_MCW || MAC_XCD
#pragma unused(theEnv)
#endif
#endif
  }

#if BASIC_IO

/******************************************/
/* PrintoutFunction: H/L access routine   */
/*   for the printout function.           */
/******************************************/
globle void PrintoutFunction(
  void *theEnv)
  {
   char *dummyid;
   int i, argCount;
   DATA_OBJECT theArgument;

   /*=======================================================*/
   /* The printout function requires at least one argument. */
   /*=======================================================*/

   if ((argCount = EnvArgCountCheck(theEnv,"printout",AT_LEAST,1)) == -1) return;

   /*=====================================================*/
   /* Get the logical name to which output is to be sent. */
   /*=====================================================*/

   dummyid = GetLogicalName(theEnv,1,"stdout");
   if (dummyid == NULL)
     {
      IllegalLogicalNameMessage(theEnv,"printout");
      SetHaltExecution(theEnv,TRUE);
      SetEvaluationError(theEnv,TRUE);
      return;
     }

   /*============================================================*/
   /* Determine if any router recognizes the output destination. */
   /*============================================================*/

   if (strcmp(dummyid,"nil") == 0)
     { return; }
   else if (QueryRouters(theEnv,dummyid) == FALSE)
     {
      UnrecognizedRouterMessage(theEnv,dummyid);
      return;
     }

   /*===============================================*/
   /* Print each of the arguments sent to printout. */
   /*===============================================*/

   for (i = 2; i <= argCount; i++)
     {
      EnvRtnUnknown(theEnv,i,&theArgument);
      if (EvaluationData(theEnv)->HaltExecution) break;

      switch(GetType(theArgument))
        {
         case SYMBOL:
           if (strcmp(DOToString(theArgument),"crlf") == 0)
             { EnvPrintRouter(theEnv,dummyid,"\n"); }
           else if (strcmp(DOToString(theArgument),"tab") == 0)
             { EnvPrintRouter(theEnv,dummyid,"\t"); }
           else if (strcmp(DOToString(theArgument),"vtab") == 0)
             { EnvPrintRouter(theEnv,dummyid,"\v"); }
           else if (strcmp(DOToString(theArgument),"ff") == 0)
             { EnvPrintRouter(theEnv,dummyid,"\f"); }
           else if (strcmp(DOToString(theArgument),"t") == 0)
             { EnvPrintRouter(theEnv,dummyid,"\n"); }
           else
             { EnvPrintRouter(theEnv,dummyid,DOToString(theArgument)); }
           break;

         case STRING:
           EnvPrintRouter(theEnv,dummyid,DOToString(theArgument));
           break;

         default:
           PrintDataObject(theEnv,dummyid,&theArgument);
           break;
        }
     }
  }

/*************************************************************/
/* ReadFunction: H/L access routine for the read function.   */
/*************************************************************/
globle void ReadFunction(
  void *theEnv,
  DATA_OBJECT_PTR returnValue)
  {
   struct token theToken;
   int numberOfArguments;
   char *logicalName = NULL;

   /*===============================================*/
   /* Check for an appropriate number of arguments. */
   /*===============================================*/

   if ((numberOfArguments = EnvArgCountCheck(theEnv,"read",NO_MORE_THAN,1)) == -1)
     {
      returnValue->type = STRING;
      returnValue->value = (void *) EnvAddSymbol(theEnv,"*** READ ERROR ***");
      return;
     }

   /*======================================================*/
   /* Determine the logical name from which input is read. */
   /*======================================================*/

   if (numberOfArguments == 0)
     { logicalName = "stdin"; }
   else if (numberOfArguments == 1)
     {
      logicalName = GetLogicalName(theEnv,1,"stdin");
      if (logicalName == NULL)
        {
         IllegalLogicalNameMessage(theEnv,"read");
         SetHaltExecution(theEnv,TRUE);
         SetEvaluationError(theEnv,TRUE);
         returnValue->type = STRING;
         returnValue->value = (void *) EnvAddSymbol(theEnv,"*** READ ERROR ***");
         return;
        }
     }

   /*============================================*/
   /* Check to see that the logical name exists. */
   /*============================================*/

   if (QueryRouters(theEnv,logicalName) == FALSE)
     {
      UnrecognizedRouterMessage(theEnv,logicalName);
      SetHaltExecution(theEnv,TRUE);
      SetEvaluationError(theEnv,TRUE);
      returnValue->type = STRING;
      returnValue->value = (void *) EnvAddSymbol(theEnv,"*** READ ERROR ***");
      return;
     }

   /*=======================================*/
   /* Collect input into string if the read */
   /* source is stdin, else just get token. */
   /*=======================================*/

   if (strcmp(logicalName,"stdin") == 0)
     { ReadTokenFromStdin(theEnv,&theToken); }
   else
     { GetToken(theEnv,logicalName,&theToken); }

   RouterData(theEnv)->CommandBufferInputCount = -1;

   /*====================================================*/
   /* Copy the token to the return value data structure. */
   /*====================================================*/

   returnValue->type = theToken.type;
   if ((theToken.type == FLOAT) || (theToken.type == STRING) ||
#if OBJECT_SYSTEM
       (theToken.type == INSTANCE_NAME) ||
#endif
       (theToken.type == SYMBOL) || (theToken.type == INTEGER))
     { returnValue->value = theToken.value; }
   else if (theToken.type == STOP)
     {
      returnValue->type = SYMBOL;
      returnValue->value = (void *) EnvAddSymbol(theEnv,"EOF");
     }
   else if (theToken.type == UNKNOWN_VALUE)
     {
      returnValue->type = STRING;
      returnValue->value = (void *) EnvAddSymbol(theEnv,"*** READ ERROR ***");
     }
   else
     {
      returnValue->type = STRING;
      returnValue->value = (void *) EnvAddSymbol(theEnv,theToken.printForm);
     }

   return;
  }

/********************************************************/
/* ReadTokenFromStdin: Special routine used by the read */
/*   function to read a token from standard input.      */
/********************************************************/
static void ReadTokenFromStdin(
  void *theEnv,
  struct token *theToken)
  {
   char *inputString;
   unsigned inputStringSize;
   int inchar;

   /*=============================================*/
   /* Continue processing until a token is found. */
   /*=============================================*/

   theToken->type = STOP;
   while (theToken->type == STOP)
     {
      /*===========================================*/
      /* Initialize the variables used for storing */
      /* the characters retrieved from stdin.      */
      /*===========================================*/

      inputString = NULL;
      RouterData(theEnv)->CommandBufferInputCount = 0;
      inputStringSize = 0;
      inchar = EnvGetcRouter(theEnv,"stdin");

      /*========================================================*/
      /* Continue reading characters until a carriage return is */
      /* entered or the user halts execution (usually with      */
      /* control-c). Waiting for the carriage return prevents   */
      /* the input from being prematurely parsed (such as when  */
      /* a space is entered after a symbol has been typed).     */
      /*========================================================*/

      while ((inchar != '\n') && (inchar != '\r') && (inchar != EOF) &&
             (! GetHaltExecution(theEnv)))
        {
         inputString = ExpandStringWithChar(theEnv,inchar,inputString,&RouterData(theEnv)->CommandBufferInputCount,
                                            &inputStringSize,inputStringSize + 80);
         inchar = EnvGetcRouter(theEnv,"stdin");
        }

      /*==================================================*/
      /* Open a string input source using the characters  */
      /* retrieved from stdin and extract the first token */
      /* contained in the string.                         */
      /*==================================================*/

      OpenStringSource(theEnv,"read",inputString,0);
      GetToken(theEnv,"read",theToken);
      CloseStringSource(theEnv,"read");
      if (inputStringSize > 0) rm(theEnv,inputString,inputStringSize);

      /*===========================================*/
      /* Pressing control-c (or comparable action) */
      /* aborts the read function.                 */
      /*===========================================*/

      if (GetHaltExecution(theEnv))
        {
         theToken->type = STRING;
         theToken->value = (void *) EnvAddSymbol(theEnv,"*** READ ERROR ***");
        }

      /*====================================================*/
      /* Return the EOF symbol if the end of file for stdin */
      /* has been encountered. This typically won't occur,  */
      /* but is possible (for example by pressing control-d */
      /* in the UNIX operating system).                     */
      /*====================================================*/

      if ((theToken->type == STOP) && (inchar == EOF))
        {
         theToken->type = SYMBOL;
         theToken->value = (void *) EnvAddSymbol(theEnv,"EOF");
        }
     }
  }

/*************************************************************/
/* OpenFunction: H/L access routine for the open function.   */
/*************************************************************/
globle int OpenFunction(
  void *theEnv)
  {
   int numberOfArguments;
   char *fileName, *logicalName, *accessMode = NULL;
   DATA_OBJECT theArgument;

   /*========================================*/
   /* Check for a valid number of arguments. */
   /*========================================*/

   if ((numberOfArguments = EnvArgRangeCheck(theEnv,"open",2,3)) == -1) return(0);

   /*====================*/
   /* Get the file name. */
   /*====================*/

   if ((fileName = GetFileName(theEnv,"open",1)) == NULL) return(0);

   /*=======================================*/
   /* Get the logical name to be associated */
   /* with the opened file.                 */
   /*=======================================*/

   logicalName = GetLogicalName(theEnv,2,NULL);
   if (logicalName == NULL)
     {
      SetHaltExecution(theEnv,TRUE);
      SetEvaluationError(theEnv,TRUE);
      IllegalLogicalNameMessage(theEnv,"open");
      return(0);
     }

   /*==================================*/
   /* Check to see if the logical name */
   /* is already in use.               */
   /*==================================*/

   if (FindFile(theEnv,logicalName))
     {
      SetHaltExecution(theEnv,TRUE);
      SetEvaluationError(theEnv,TRUE);
      PrintErrorID(theEnv,"IOFUN",2,FALSE);
      EnvPrintRouter(theEnv,WERROR,"Logical name ");
      EnvPrintRouter(theEnv,WERROR,logicalName);
      EnvPrintRouter(theEnv,WERROR," already in use.\n");
      return(0);
     }

   /*===========================*/
   /* Get the file access mode. */
   /*===========================*/

   if (numberOfArguments == 2)
     { accessMode = "r"; }
   else if (numberOfArguments == 3)
     {
      if (EnvArgTypeCheck(theEnv,"open",3,STRING,&theArgument) == FALSE) return(0);
      accessMode = DOToString(theArgument);
     }

   /*=====================================*/
   /* Check for a valid file access mode. */
   /*=====================================*/

   if ((strcmp(accessMode,"r") != 0) &&
       (strcmp(accessMode,"r+") != 0) &&
       (strcmp(accessMode,"w") != 0) &&
       (strcmp(accessMode,"a") != 0) &&
       (strcmp(accessMode,"wb") != 0))
     {
      SetHaltExecution(theEnv,TRUE);
      SetEvaluationError(theEnv,TRUE);
      ExpectedTypeError1(theEnv,"open",3,"string with value \"r\", \"r+\", \"w\", \"wb\", or \"a\"");
      return(0);
     }

   /*================================================*/
   /* Open the named file and associate it with the  */
   /* specified logical name. Return TRUE if the     */
   /* file was opened successfully, otherwise FALSE. */
   /*================================================*/

   return(OpenAFile(theEnv,fileName,accessMode,logicalName));
  }

/***************************************************************/
/* CloseFunction: H/L access routine for the close function.   */
/***************************************************************/
globle int CloseFunction(
  void *theEnv)
  {
   int numberOfArguments;
   char *logicalName;

   /*======================================*/
   /* Check for valid number of arguments. */
   /*======================================*/

   if ((numberOfArguments = EnvArgCountCheck(theEnv,"close",NO_MORE_THAN,1)) == -1) return(0);

   /*=====================================================*/
   /* If no arguments are specified, then close all files */
   /* opened with the open command. Return TRUE if all    */
   /* files were closed successfully, otherwise FALSE.    */
   /*=====================================================*/

   if (numberOfArguments == 0) return(CloseAllFiles(theEnv));

   /*================================*/
   /* Get the logical name argument. */
   /*================================*/

   logicalName = GetLogicalName(theEnv,1,NULL);
   if (logicalName == NULL)
     {
      IllegalLogicalNameMessage(theEnv,"close");
      SetHaltExecution(theEnv,TRUE);
      SetEvaluationError(theEnv,TRUE);
      return(0);
     }

   /*========================================================*/
   /* Close the file associated with the specified logical   */
   /* name. Return TRUE if the file was closed successfully, */
   /* otherwise false.                                       */
   /*========================================================*/

   return(CloseFile(theEnv,logicalName));
  }

/***************************************/
/* GetCharFunction: H/L access routine */
/*   for the get-char function.        */
/***************************************/
globle int GetCharFunction(
  void *theEnv)
  {
   int numberOfArguments;
   char *logicalName;

   if ((numberOfArguments = EnvArgCountCheck(theEnv,"get-char",NO_MORE_THAN,1)) == -1)
     { return(-1); }

   if (numberOfArguments == 0 )
     { logicalName = "stdin"; }
   else
     {
      logicalName = GetLogicalName(theEnv,1,"stdin");
      if (logicalName == NULL)
        {
         IllegalLogicalNameMessage(theEnv,"get-char");
         SetHaltExecution(theEnv,TRUE);
         SetEvaluationError(theEnv,TRUE);
         return(-1);
        }
     }

   if (QueryRouters(theEnv,logicalName) == FALSE)
     {
      UnrecognizedRouterMessage(theEnv,logicalName);
      SetHaltExecution(theEnv,TRUE);
      SetEvaluationError(theEnv,TRUE);
      return(-1);
     }

   return(EnvGetcRouter(theEnv,logicalName));
  }

#endif

#if EXT_IO

/****************************************/
/* RemoveFunction: H/L access routine   */
/*   for the remove function.           */
/****************************************/
globle int RemoveFunction(
  void *theEnv)
  {
   char *theFileName;

   /*======================================*/
   /* Check for valid number of arguments. */
   /*======================================*/

   if (EnvArgCountCheck(theEnv,"remove",EXACTLY,1) == -1) return(FALSE);

   /*====================*/
   /* Get the file name. */
   /*====================*/

   if ((theFileName = GetFileName(theEnv,"remove",1)) == NULL) return(FALSE);

   /*==============================================*/
   /* Remove the file. Return TRUE if the file was */
   /* sucessfully removed, otherwise FALSE.        */
   /*==============================================*/

   return(genremove(theFileName));
  }

/****************************************/
/* RenameFunction: H/L access routine   */
/*   for the rename function.           */
/****************************************/
globle int RenameFunction(
  void *theEnv)
  {
   char *oldFileName, *newFileName;

   /*========================================*/
   /* Check for a valid number of arguments. */
   /*========================================*/

   if (EnvArgCountCheck(theEnv,"rename",EXACTLY,2) == -1) return(FALSE);

   /*===========================*/
   /* Check for the file names. */
   /*===========================*/

   if ((oldFileName = GetFileName(theEnv,"rename",1)) == NULL) return(FALSE);
   if ((newFileName = GetFileName(theEnv,"rename",2)) == NULL) return(FALSE);

   /*==============================================*/
   /* Rename the file. Return TRUE if the file was */
   /* sucessfully renamed, otherwise FALSE.        */
   /*==============================================*/

   return(genrename(oldFileName,newFileName));
  }

/****************************************/
/* FormatFunction: H/L access routine   */
/*   for the format function.           */
/****************************************/
globle void *FormatFunction(
  void *theEnv)
  {
   int argCount;
   unsigned start_pos;
   char *formatString, *logicalName;
   char formatFlagType;
   int  f_cur_arg = 3;
   unsigned form_pos = 0;
   char buffer[FORMAT_MAX];
   char percentBuffer[FLAG_MAX];
   char *fstr = NULL;
   unsigned fmaxm = 0;
   int fpos = 0;
   void *hptr;
   int longFound;
   char *theString;

   /*======================================*/
   /* Set default return value for errors. */
   /*======================================*/

   hptr = EnvAddSymbol(theEnv,"");

   /*=========================================*/
   /* Format requires at least two arguments: */
   /* a logical name and a format string.     */
   /*=========================================*/

   if ((argCount = EnvArgCountCheck(theEnv,"format",AT_LEAST,2)) == -1)
     { return(hptr); }

   /*========================================*/
   /* First argument must be a logical name. */
   /*========================================*/

   if ((logicalName = GetLogicalName(theEnv,1,"stdout")) == NULL)
     {
      IllegalLogicalNameMessage(theEnv,"format");
      SetHaltExecution(theEnv,TRUE);
      SetEvaluationError(theEnv,TRUE);
      return(hptr);
     }

   if (strcmp(logicalName,"nil") == 0)
     { /* do nothing */ }
   else if (QueryRouters(theEnv,logicalName) == FALSE)
     {
      UnrecognizedRouterMessage(theEnv,logicalName);
      return(hptr);
     }

   /*=====================================================*/
   /* Second argument must be a string.  The appropriate  */
   /* number of arguments specified by the string must be */
   /* present in the argument list.                       */
   /*=====================================================*/

   if ((formatString = ControlStringCheck(theEnv,argCount)) == NULL)
     { return (hptr); }

   /*==============================================*/
   /* Locate a string of 80 character for scanning */
   /* sub_string from control_string               */
   /*==============================================*/

   /* Scanning and print the format */

   while (formatString[form_pos] != '\0')
     {
      if (formatString[form_pos] != '%')
        {
         start_pos = form_pos;
         while ((formatString[form_pos] != '%') &&
                (formatString[form_pos] != '\0') &&
                ((form_pos - start_pos) < FLAG_MAX))
           { form_pos++; }
         fstr = AppendNToString(theEnv,&formatString[start_pos],fstr,form_pos-start_pos,&fpos,&fmaxm);
        }
      else
        {
         start_pos = form_pos;
         form_pos++;
         formatFlagType = FindFormatFlag(formatString,&form_pos,buffer,&longFound);
         if (formatFlagType != ' ')
           {
            strncpy(percentBuffer,&formatString[start_pos],
                    (STD_SIZE) form_pos-start_pos);
            percentBuffer[form_pos-start_pos] = EOS;
            if ((! longFound) &&
                ((formatFlagType == 'd') || (formatFlagType == 'o') ||
                 (formatFlagType == 'u') || (formatFlagType == 'x')))
              {
               longFound = TRUE;
               percentBuffer[(form_pos-start_pos) - 1] = 'l';
               percentBuffer[form_pos-start_pos] = formatFlagType;
               percentBuffer[(form_pos-start_pos) + 1] = EOS;
              }

            if ((theString = PrintFormatFlag(theEnv,percentBuffer,f_cur_arg,formatFlagType,longFound)) == NULL)
              {
               if (fstr != NULL) rm(theEnv,fstr,fmaxm);
               return (hptr);
              }
            fstr = AppendToString(theEnv,theString,fstr,&fpos,&fmaxm);
            if (fstr == NULL) return(hptr);
            f_cur_arg++;
           }
         else
           {
            fstr = AppendToString(theEnv,buffer,fstr,&fpos,&fmaxm);
            if (fstr == NULL) return(hptr);
           }
        }
     }

   if (fstr != NULL)
     {
      hptr = EnvAddSymbol(theEnv,fstr);
      if (strcmp(logicalName,"nil") != 0) EnvPrintRouter(theEnv,logicalName,fstr);
      rm(theEnv,fstr,fmaxm);
     }
   else
     { hptr = EnvAddSymbol(theEnv,""); }

   return(hptr);
  }

/*********************************************************************/
/* ControlStringCheck:  Checks the 2nd parameter which is the format */
/*   control string to see if there are enough matching arguments.   */
/*********************************************************************/
static char *ControlStringCheck(
  void *theEnv,
  int argCount)
  {
   DATA_OBJECT t_ptr;
   char *str_array;
   char print_buff[10];
   int longFound;
   unsigned i;
   int per_count;

   if (EnvArgTypeCheck(theEnv,"format",2,STRING,&t_ptr) == FALSE) return(NULL);

   per_count = 0;
   str_array = ValueToString(t_ptr.value);
   for (i= 0 ; str_array[i] != '\0' ; )
     {
      if (str_array[i] == '%')
        {
         i++;
         if (FindFormatFlag(str_array,&i,print_buff,&longFound) != ' ')
           { per_count++; }
        }
      else
        { i++; }
     }

   if (per_count != (argCount - 2))
     {
      ExpectedCountError(theEnv,"format",EXACTLY,per_count+2);
      SetEvaluationError(theEnv,TRUE);
      return (NULL);
     }

   return(str_array);
  }

/***********************************************/
/* FindFormatFlag:  This function searches for */
/*   a format flag in the format string.       */
/***********************************************/
static char FindFormatFlag(
  char *formatString,
  unsigned *a,
  char *formatBuffer,
  int *longFound)
  {
   char inchar, formatFlagType;
   unsigned start_pos, copy_pos = 0;

   /*===========================================================*/
   /* Set return values to the default value. A blank character */
   /* indicates that no format flag was found which requires a  */
   /* parameter. The longFound flag indicates whether the       */
   /* character 'l' was used with the float or integer flag to  */
   /* indicate a double precision float or a long integer.      */
   /*===========================================================*/

   formatFlagType = ' ';
   *longFound = FALSE;

   /*=====================================================*/
   /* The format flags for carriage returns, line feeds,  */
   /* horizontal and vertical tabs, and the percent sign, */
   /* do not require a parameter.                         */
   /*=====================================================*/

   if (formatString[*a] == 'n')
     {
      sprintf(formatBuffer,"\n");
      (*a)++;
      return(formatFlagType);
     }
   else if (formatString[*a] == 'r')
     {
      sprintf(formatBuffer,"\r");
      (*a)++;
      return(formatFlagType);
     }
   else if (formatString[*a] == 't')
     {
      sprintf(formatBuffer,"\t");
      (*a)++;
      return(formatFlagType);
     }
   else if (formatString[*a] == 'v')
     {
      sprintf(formatBuffer,"\v");
      (*a)++;
      return(formatFlagType);
     }
   else if (formatString[*a] == '%')
     {
      sprintf(formatBuffer,"%%");
      (*a)++;
      return(formatFlagType);
     }

   /*======================================================*/
   /* Identify the format flag which requires a parameter. */
   /*======================================================*/

   start_pos = *a;
   formatBuffer[copy_pos] = '\0';
   while ((formatString[*a] != '%') &&
          (formatString[*a] != '\0') &&
          ((*a - start_pos) < FLAG_MAX))
     {
      inchar = formatString[*a];
      formatBuffer[copy_pos++] = inchar;
      formatBuffer[copy_pos] = '\0';
      if ( (inchar == 'd') ||
           (inchar == 'o') ||
           (inchar == 'x') ||
           (inchar == 'u') ||
           (inchar == 'c') ||
           (inchar == 's') ||
           (inchar == 'e') ||
           (inchar == 'f') ||
           (inchar == 'g') )
        {
         formatFlagType = inchar;
         if (formatString[(*a) - 1] == 'l')
           { *longFound = TRUE; }
         (*a)++;
         return(formatFlagType);
        }
      (*a)++;
     }

   return(formatFlagType);
  }

/**********************************************************************/
/* PrintFormatFlag:  Prints out part of the total format string along */
/*   with the argument for that part of the format string.            */
/**********************************************************************/
static char *PrintFormatFlag(
  void *theEnv,
  char *formatString,
  int whichArg,
  int formatType,
  int longFound)
  {
   DATA_OBJECT theResult;
   char *theString, *printBuffer;
   unsigned theLength;
   void *oldLocale;
      
   /*=================*/
   /* String argument */
   /*=================*/

   switch (formatType)
     {
      case 's':
        if (EnvArgTypeCheck(theEnv,"format",whichArg,SYMBOL_OR_STRING,&theResult) == FALSE) return(NULL);
        theLength = strlen(formatString) + strlen(ValueToString(theResult.value)) + 200;
        printBuffer = (char *) gm2(theEnv,(sizeof(char) * theLength));
        sprintf(printBuffer,formatString,ValueToString(theResult.value));
        break;

      case 'c':
        EnvRtnUnknown(theEnv,whichArg,&theResult);
        if ((GetType(theResult) == STRING) ||
            (GetType(theResult) == SYMBOL))
          {
           theLength = strlen(formatString) + 200;
           printBuffer = (char *) gm2(theEnv,(sizeof(char) * theLength));
           sprintf(printBuffer,formatString,(ValueToString(theResult.value))[0]);
          }
        else if (GetType(theResult) == INTEGER)
          {
           theLength = strlen(formatString) + 200;
           printBuffer = (char *) gm2(theEnv,(sizeof(char) * theLength));
           sprintf(printBuffer,formatString,(char) DOToLong(theResult));
          }
        else
          {
           ExpectedTypeError1(theEnv,"format",whichArg,"symbol, string, or integer");
           return(NULL);
          }
        break;

      case 'd':
      case 'x':
      case 'o':
      case 'u':
        if (EnvArgTypeCheck(theEnv,"format",whichArg,INTEGER_OR_FLOAT,&theResult) == FALSE) return(NULL);
        theLength = strlen(formatString) + 200;
        printBuffer = (char *) gm2(theEnv,(sizeof(char) * theLength));
        
        oldLocale = EnvAddSymbol(theEnv,setlocale(LC_NUMERIC,NULL));
        setlocale(LC_NUMERIC,ValueToString(IOFunctionData(theEnv)->locale));

        if (GetType(theResult) == FLOAT)
          {
           if (longFound)
             { sprintf(printBuffer,formatString,(long) ValueToDouble(theResult.value)); }
           else
             { sprintf(printBuffer,formatString,(int) ValueToDouble(theResult.value)); }
          }
        else
          {
           if (longFound)
             { sprintf(printBuffer,formatString,(long) ValueToLong(theResult.value)); }
           else
             { sprintf(printBuffer,formatString,(int) ValueToLong(theResult.value)); }
          }
          
        setlocale(LC_NUMERIC,ValueToString(oldLocale));
        break;

      case 'f':
      case 'g':
      case 'e':
        if (EnvArgTypeCheck(theEnv,"format",whichArg,INTEGER_OR_FLOAT,&theResult) == FALSE) return(NULL);
        theLength = strlen(formatString) + 200;
        printBuffer = (char *) gm2(theEnv,(sizeof(char) * theLength));

        oldLocale = EnvAddSymbol(theEnv,setlocale(LC_NUMERIC,NULL));
        
        setlocale(LC_NUMERIC,ValueToString(IOFunctionData(theEnv)->locale));

        if (GetType(theResult) == FLOAT)
          { sprintf(printBuffer,formatString,ValueToDouble(theResult.value)); }
        else
          { sprintf(printBuffer,formatString,(double) ValueToLong(theResult.value)); }
        
        setlocale(LC_NUMERIC,ValueToString(oldLocale));
        
        break;

      default:
         EnvPrintRouter(theEnv,WERROR," Error in format, the conversion character");
         EnvPrintRouter(theEnv,WERROR," for formatted output is not valid\n");
         return(FALSE);
     }

   theString = ValueToString(EnvAddSymbol(theEnv,printBuffer));
   rm(theEnv,printBuffer,sizeof(char) * theLength);
   return(theString);
  }

/******************************************/
/* ReadlineFunction: H/L access routine   */
/*   for the readline function.           */
/******************************************/
globle void ReadlineFunction(
  void *theEnv,
  DATA_OBJECT_PTR returnValue)
  {
   char *buffer;
   unsigned line_max = 0;
   int numberOfArguments;
   char *logicalName;

   returnValue->type = STRING;

   if ((numberOfArguments = EnvArgCountCheck(theEnv,"readline",NO_MORE_THAN,1)) == -1)
     {
      returnValue->value = (void *) EnvAddSymbol(theEnv,"*** READ ERROR ***");
      return;
     }

   if (numberOfArguments == 0 )
     { logicalName = "stdin"; }
   else
     {
      logicalName = GetLogicalName(theEnv,1,"stdin");
      if (logicalName == NULL)
        {
         IllegalLogicalNameMessage(theEnv,"readline");
         SetHaltExecution(theEnv,TRUE);
         SetEvaluationError(theEnv,TRUE);
         returnValue->value = (void *) EnvAddSymbol(theEnv,"*** READ ERROR ***");
         return;
        }
     }

   if (QueryRouters(theEnv,logicalName) == FALSE)
     {
      UnrecognizedRouterMessage(theEnv,logicalName);
      SetHaltExecution(theEnv,TRUE);
      SetEvaluationError(theEnv,TRUE);
      returnValue->value = (void *) EnvAddSymbol(theEnv,"*** READ ERROR ***");
      return;
     }

   RouterData(theEnv)->CommandBufferInputCount = 0;
   buffer = FillBuffer(theEnv,logicalName,&RouterData(theEnv)->CommandBufferInputCount,&line_max);
   RouterData(theEnv)->CommandBufferInputCount = -1;

   if (GetHaltExecution(theEnv))
     {
      returnValue->value = (void *) EnvAddSymbol(theEnv,"*** READ ERROR ***");
      if (buffer != NULL) rm(theEnv,buffer,(int) sizeof (char) * line_max);
      return;
     }

   if (buffer == NULL)
     {
      returnValue->value = (void *) EnvAddSymbol(theEnv,"EOF");
      returnValue->type = SYMBOL;
      return;
     }

   returnValue->value = (void *) EnvAddSymbol(theEnv,buffer);
   rm(theEnv,buffer,(int) sizeof (char) * line_max);
   return;
  }

/*************************************************************/
/* FillBuffer: Read characters from a specified logical name */
/*   and places them into a buffer until a carriage return   */
/*   or end-of-file character is read.                       */
/*************************************************************/
static char *FillBuffer(
  void *theEnv,
  char *logicalName,
  int *currentPosition,
  unsigned *maximumSize)
  {
   int c;
   char *buf = NULL;

   /*================================*/
   /* Read until end of line or eof. */
   /*================================*/

   c = EnvGetcRouter(theEnv,logicalName);

   if (c == EOF)
     { return(NULL); }

   /*==================================*/
   /* Grab characters until cr or eof. */
   /*==================================*/

   while ((c != '\n') && (c != '\r') && (c != EOF) &&
          (! GetHaltExecution(theEnv)))
     {
      buf = ExpandStringWithChar(theEnv,c,buf,currentPosition,maximumSize,*maximumSize+80);
      c = EnvGetcRouter(theEnv,logicalName);
     }

   /*==================*/
   /* Add closing EOS. */
   /*==================*/

   buf = ExpandStringWithChar(theEnv,EOS,buf,currentPosition,maximumSize,*maximumSize+80);
   return (buf);
  }
  
/*****************************************/
/* SetLocaleFunction: H/L access routine */
/*   for the set-locale function.        */
/*****************************************/
globle void SetLocaleFunction(
  void *theEnv,
  DATA_OBJECT_PTR returnValue)
  {
   DATA_OBJECT theResult;
   int numArgs;
   
   /*======================================*/
   /* Check for valid number of arguments. */
   /*======================================*/
   
   if ((numArgs = EnvArgCountCheck(theEnv,"set-locale",NO_MORE_THAN,1)) == -1)
     {
      returnValue->type = SYMBOL;
      returnValue->value = EnvFalseSymbol(theEnv);
      return;
     }
     
   /*=================================*/
   /* If there are no arguments, just */
   /* return the current locale.      */
   /*=================================*/
   
   if (numArgs == 0)
     {
      returnValue->type = STRING;
      returnValue->value = IOFunctionData(theEnv)->locale;
      return;
     }

   /*=================*/
   /* Get the locale. */
   /*=================*/
   
   if (EnvArgTypeCheck(theEnv,"set-locale",1,STRING,&theResult) == FALSE)
     {
      returnValue->type = SYMBOL;
      returnValue->value = EnvFalseSymbol(theEnv);
      return;
     }
     
   /*=====================================*/
   /* Return the old value of the locale. */
   /*=====================================*/
   
   returnValue->type = STRING;
   returnValue->value = IOFunctionData(theEnv)->locale;
   
   /*======================================================*/
   /* Change the value of the locale to the one specified. */
   /*======================================================*/
   
   DecrementSymbolCount(theEnv,(struct symbolHashNode *) IOFunctionData(theEnv)->locale);
   IOFunctionData(theEnv)->locale = DOToPointer(theResult);
   IncrementSymbolCount(IOFunctionData(theEnv)->locale);
  }

/******************************************/
/* ReadNumberFunction: H/L access routine */
/*   for the read-number function.        */
/******************************************/
globle void ReadNumberFunction(
  void *theEnv,
  DATA_OBJECT_PTR returnValue)
  {
   struct token theToken;
   int numberOfArguments;
   char *logicalName = NULL;

   /*===============================================*/
   /* Check for an appropriate number of arguments. */
   /*===============================================*/

   if ((numberOfArguments = EnvArgCountCheck(theEnv,"read",NO_MORE_THAN,1)) == -1)
     {
      returnValue->type = STRING;
      returnValue->value = (void *) EnvAddSymbol(theEnv,"*** READ ERROR ***");
      return;
     }

   /*======================================================*/
   /* Determine the logical name from which input is read. */
   /*======================================================*/

   if (numberOfArguments == 0)
     { logicalName = "stdin"; }
   else if (numberOfArguments == 1)
     {
      logicalName = GetLogicalName(theEnv,1,"stdin");
      if (logicalName == NULL)
        {
         IllegalLogicalNameMessage(theEnv,"read");
         SetHaltExecution(theEnv,TRUE);
         SetEvaluationError(theEnv,TRUE);
         returnValue->type = STRING;
         returnValue->value = (void *) EnvAddSymbol(theEnv,"*** READ ERROR ***");
         return;
        }
     }

   /*============================================*/
   /* Check to see that the logical name exists. */
   /*============================================*/

   if (QueryRouters(theEnv,logicalName) == FALSE)
     {
      UnrecognizedRouterMessage(theEnv,logicalName);
      SetHaltExecution(theEnv,TRUE);
      SetEvaluationError(theEnv,TRUE);
      returnValue->type = STRING;
      returnValue->value = (void *) EnvAddSymbol(theEnv,"*** READ ERROR ***");
      return;
     }

   /*=======================================*/
   /* Collect input into string if the read */
   /* source is stdin, else just get token. */
   /*=======================================*/

   if (strcmp(logicalName,"stdin") == 0)
     { ReadNumber(theEnv,logicalName,&theToken,TRUE); }
   else
     { ReadNumber(theEnv,logicalName,&theToken,FALSE); }

   RouterData(theEnv)->CommandBufferInputCount = -1;

   /*====================================================*/
   /* Copy the token to the return value data structure. */
   /*====================================================*/

   returnValue->type = theToken.type;
   if ((theToken.type == FLOAT) || (theToken.type == STRING) ||
#if OBJECT_SYSTEM
       (theToken.type == INSTANCE_NAME) ||
#endif
       (theToken.type == SYMBOL) || (theToken.type == INTEGER))
     { returnValue->value = theToken.value; }
   else if (theToken.type == STOP)
     {
      returnValue->type = SYMBOL;
      returnValue->value = (void *) EnvAddSymbol(theEnv,"EOF");
     }
   else if (theToken.type == UNKNOWN_VALUE)
     {
      returnValue->type = STRING;
      returnValue->value = (void *) EnvAddSymbol(theEnv,"*** READ ERROR ***");
     }
   else
     {
      returnValue->type = STRING;
      returnValue->value = (void *) EnvAddSymbol(theEnv,theToken.printForm);
     }

   return;
  }
  
/********************************************/
/* ReadNumber: Special routine used by the  */
/*   read-number function to read a number. */
/********************************************/
static void ReadNumber(
  void *theEnv,
  char *logicalName,
  struct token *theToken,
  int isStdin)
  {
   char *inputString;
   char *charPtr = NULL;
   unsigned inputStringSize;
   int inchar;
   long theLong;
   double theDouble;
   void *oldLocale;

   theToken->type = STOP;

   /*===========================================*/
   /* Initialize the variables used for storing */
   /* the characters retrieved from stdin.      */
   /*===========================================*/

   inputString = NULL;
   RouterData(theEnv)->CommandBufferInputCount = 0;
   inputStringSize = 0;
   inchar = EnvGetcRouter(theEnv,logicalName);
            
   /*====================================*/
   /* Skip whitespace before any number. */
   /*====================================*/
      
   while (isspace(inchar) && (inchar != EOF) && 
          (! GetHaltExecution(theEnv)))
     { inchar = EnvGetcRouter(theEnv,logicalName); }

   /*=============================================================*/
   /* Continue reading characters until whitespace is found again */
   /* (for anything other than stdin) or a CR/LF (for stdin).     */
   /*=============================================================*/

   while ((((! isStdin) && (! isspace(inchar))) || 
          (isStdin && (inchar != '\n') && (inchar != '\r'))) &&
          (inchar != EOF) &&
          (! GetHaltExecution(theEnv)))
     {
      inputString = ExpandStringWithChar(theEnv,inchar,inputString,&RouterData(theEnv)->CommandBufferInputCount,
                                         &inputStringSize,inputStringSize + 80);
      inchar = EnvGetcRouter(theEnv,logicalName);
     }

   /*===========================================*/
   /* Pressing control-c (or comparable action) */
   /* aborts the read-number function.          */
   /*===========================================*/

   if (GetHaltExecution(theEnv))
     {
      theToken->type = STRING;
      theToken->value = (void *) EnvAddSymbol(theEnv,"*** READ ERROR ***");
      if (inputStringSize > 0) rm(theEnv,inputString,inputStringSize);
      return;
     }

   /*====================================================*/
   /* Return the EOF symbol if the end of file for stdin */
   /* has been encountered. This typically won't occur,  */
   /* but is possible (for example by pressing control-d */
   /* in the UNIX operating system).                     */
   /*====================================================*/

   if (inchar == EOF)
     {
      theToken->type = SYMBOL;
      theToken->value = (void *) EnvAddSymbol(theEnv,"EOF");
      if (inputStringSize > 0) rm(theEnv,inputString,inputStringSize);
      return;
     }

   /*==================================================*/
   /* Open a string input source using the characters  */
   /* retrieved from stdin and extract the first token */
   /* contained in the string.                         */
   /*==================================================*/
   
   /*=======================================*/
   /* Change the locale so that numbers are */
   /* converted using the localized format. */
   /*=======================================*/
   
   oldLocale = EnvAddSymbol(theEnv,setlocale(LC_NUMERIC,NULL));
   setlocale(LC_NUMERIC,ValueToString(IOFunctionData(theEnv)->locale));

   /*========================================*/
   /* Try to parse the number as a long. The */
   /* terminating character must either be   */
   /* white space or the string terminator.  */
   /*========================================*/

   theLong = strtol(inputString,&charPtr,10);
   if ((charPtr != inputString) && 
       (isspace(*charPtr) || (*charPtr == '\0')))
     {
      theToken->type = INTEGER;
      theToken->value = (void *) EnvAddLong(theEnv,theLong);
      if (inputStringSize > 0) rm(theEnv,inputString,inputStringSize);
      setlocale(LC_NUMERIC,ValueToString(oldLocale));
      return;
     }
     
   /*==========================================*/
   /* Try to parse the number as a double. The */
   /* terminating character must either be     */
   /* white space or the string terminator.    */
   /*==========================================*/

   theDouble = strtod(inputString,&charPtr);  
   if ((charPtr != inputString) && 
       (isspace(*charPtr) || (*charPtr == '\0')))
     {
      theToken->type = FLOAT;
      theToken->value = (void *) EnvAddDouble(theEnv,theDouble);
      if (inputStringSize > 0) rm(theEnv,inputString,inputStringSize);
      setlocale(LC_NUMERIC,ValueToString(oldLocale));
      return;
     }

   /*============================================*/
   /* Restore the "C" locale so that any parsing */
   /* of numbers uses the C format.              */
   /*============================================*/
   
   setlocale(LC_NUMERIC,ValueToString(oldLocale));

   /*=========================================*/
   /* Return "*** READ ERROR ***" to indicate */
   /* a number was not successfully parsed.   */
   /*=========================================*/
         
   theToken->type = STRING;
   theToken->value = (void *) EnvAddSymbol(theEnv,"*** READ ERROR ***");
  }

#endif

