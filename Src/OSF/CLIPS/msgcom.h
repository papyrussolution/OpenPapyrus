   /*******************************************************/
   /*      "C" Language Integrated Production System      */
   /*                                                     */
   /*               CLIPS Version 6.20  01/31/02          */
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
/*************************************************************/

#ifndef _H_msgcom
#define _H_msgcom

#ifndef _H_object
#include "object.h"
#endif

#ifndef _H_msgpass
#include "msgpass.h"
#endif

#define MESSAGE_HANDLER_DATA 32

struct messageHandlerData
  { 
   ENTITY_RECORD HandlerGetInfo;
   ENTITY_RECORD HandlerPutInfo;
   SYMBOL_HN *INIT_SYMBOL;
   SYMBOL_HN *DELETE_SYMBOL;
   SYMBOL_HN *CREATE_SYMBOL;
#if DEBUGGING_FUNCTIONS
   unsigned WatchHandlers;
   unsigned WatchMessages;
#endif
   char *hndquals[4];
   SYMBOL_HN *SELF_SYMBOL;
   SYMBOL_HN *CurrentMessageName;
   HANDLER_LINK *CurrentCore;
   HANDLER_LINK *TopOfCore;
   HANDLER_LINK *NextInCore;
  };

#define MessageHandlerData(theEnv) ((struct messageHandlerData *) GetEnvironmentData(theEnv,MESSAGE_HANDLER_DATA))


#ifdef LOCALE
#undef LOCALE
#endif

#ifdef _MSGCOM_SOURCE_
#define LOCALE
#else
#define LOCALE extern
#endif

#define INIT_STRING   "init"
#define DELETE_STRING "delete"
#define PRINT_STRING  "print"
#define CREATE_STRING "create"

#if ENVIRONMENT_API_ONLY
#define FindDefmessageHandler(theEnv,a,b,c) EnvFindDefmessageHandler(theEnv,a,b,c)
#define GetDefmessageHandlerName(theEnv,a,b) EnvGetDefmessageHandlerName(theEnv,a,b)
#define GetDefmessageHandlerPPForm(theEnv,a,b) EnvGetDefmessageHandlerPPForm(theEnv,a,b)
#define GetDefmessageHandlerType(theEnv,a,b) EnvGetDefmessageHandlerType(theEnv,a,b)
#define GetDefmessageHandlerWatch(theEnv,a,b) EnvGetDefmessageHandlerWatch(theEnv,a,b)
#define GetNextDefmessageHandler(theEnv,a,b) EnvGetNextDefmessageHandler(theEnv,a,b)
#define IsDefmessageHandlerDeletable(theEnv,a,b) EnvIsDefmessageHandlerDeletable(theEnv,a,b)
#define ListDefmessageHandlers(theEnv,a,b,c) EnvListDefmessageHandler(theEnv,a,b,c)
#define PreviewSend(theEnv,a,b,c) EnvPreviewSend(theEnv,a,b,c)
#define SetDefmessageHandlerWatch(theEnv,a,b,c) EnvSetDefmessageHandlerWatch(theEnv,a,b,c)
#define UndefmessageHandler(theEnv,a,b) EnvUndefmessageHandler(theEnv,a,b)
#else
#define FindDefmessageHandler(a,b,c) EnvFindDefmessageHandler(GetCurrentEnvironment(),a,b,c)
#define GetDefmessageHandlerName(a,b) EnvGetDefmessageHandlerName(GetCurrentEnvironment(),a,b)
#define GetDefmessageHandlerPPForm(a,b) EnvGetDefmessageHandlerPPForm(GetCurrentEnvironment(),a,b)
#define GetDefmessageHandlerType(a,b) EnvGetDefmessageHandlerType(GetCurrentEnvironment(),a,b)
#define GetDefmessageHandlerWatch(a,b) EnvGetDefmessageHandlerWatch(GetCurrentEnvironment(),a,b)
#define GetNextDefmessageHandler(a,b) EnvGetNextDefmessageHandler(GetCurrentEnvironment(),a,b)
#define IsDefmessageHandlerDeletable(a,b) EnvIsDefmessageHandlerDeletable(GetCurrentEnvironment(),a,b)
#define ListDefmessageHandlers(a,b,c) EnvListDefmessageHandlers(GetCurrentEnvironment(),a,b,c)
#define PreviewSend(a,b,c) EnvPreviewSend(GetCurrentEnvironment(),a,b,c)
#define SetDefmessageHandlerWatch(a,b,c) EnvSetDefmessageHandlerWatch(GetCurrentEnvironment(),a,b,c)
#define UndefmessageHandler(a,b) EnvUndefmessageHandler(GetCurrentEnvironment(),a,b)
#endif
   
   LOCALE void             SetupMessageHandlers(void *);
   LOCALE char            *EnvGetDefmessageHandlerName(void *,void *,unsigned);
   LOCALE char            *EnvGetDefmessageHandlerType(void *,void *,unsigned);
   LOCALE unsigned         EnvGetNextDefmessageHandler(void *,void *,unsigned);
   LOCALE HANDLER         *GetDefmessageHandlerPointer(void *,unsigned);
#if DEBUGGING_FUNCTIONS
   LOCALE unsigned         EnvGetDefmessageHandlerWatch(void *,void *,unsigned);
   LOCALE void             EnvSetDefmessageHandlerWatch(void *,int,void *,unsigned);
#endif
   LOCALE unsigned         EnvFindDefmessageHandler(void *,void *,char *,char *); 
   LOCALE int              EnvIsDefmessageHandlerDeletable(void *,void *,unsigned);
   LOCALE void             UndefmessageHandlerCommand(void *);
   LOCALE int              EnvUndefmessageHandler(void *,void *,unsigned);

#if DEBUGGING_FUNCTIONS
   LOCALE void             PPDefmessageHandlerCommand(void *);
   LOCALE void             ListDefmessageHandlersCommand(void *);
   LOCALE void             PreviewSendCommand(void *); 
   LOCALE char            *EnvGetDefmessageHandlerPPForm(void *,void *,unsigned);
   LOCALE void             EnvListDefmessageHandlers(void *,char *,void *,int);
   LOCALE void             EnvPreviewSend(void *,char *,void *,char *);
   LOCALE long             DisplayHandlersInLinks(void *,char *,PACKED_CLASS_LINKS *,unsigned);
#endif

#endif





