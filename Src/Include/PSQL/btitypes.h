#ifndef _BTITYPES_H_INCLUDED
/*************************************************************************
**
**  Copyright 1982-2001 Pervasive Software Inc. All Rights Reserved
**
*************************************************************************/
/*************************************************************************
   BTITYPES.H
     This module contains platform-independent data types used by the
     Btrieve and Scalable SQL C/C++ Interface for MS Windows, OS2, DOS,
     Extended DOS, NetWare NLM, Windows 9x/Me, MS Windows NT/2000, and UNIX.

     You must define one of the following to your compiler in order to
     compile for a particular platform:

        BTI_DOS     - DOS Real Mode                           16-bit Apps
        BTI_DOS_32R - Extended DOS with Rational + bstub.exe  32-bit Apps
        BTI_DOS_32P - Extended DOS with Phar Lap 6.0          32-bit Apps
        BTI_DOS_32B - Extended DOS with Borland PowerPack     32-bit Apps
        BTI_DOS_16B - Extended DOS with Borland PowerPack     16-bit Apps
        BTI_WIN     - MS WINDOWS                              16-bit Apps
        BTI_WIN_32  - Windows NT and Windows 95               32-bit Apps
        BTI_OS2     - OS2                                     16-bit Apps
        BTI_OS2_32  - OS2                                     32-bit Apps
        BTI_NLM     - NetWare NLM                             32-bit Apps
        BTI_SQL     - For 32-Bit OS2 SSQL Apps
        BTI_SOL     - Solaris                                 32-bit Apps
        BTI_HPUX    - HP Unix
        BTI_AIX     - IBM Aix
        BTI_IRIX    - Irix
        BTI_DEC_UNIX  - DEC Unix


     If you do not specify one of the above switches to your compiler,
     an error directive in btitypes.h will halt the compilation.
*************************************************************************/
#if !defined(BTI_WIN)     && !defined(BTI_OS2)     && !defined(BTI_DOS) && \
    !defined(BTI_NLM)     && !defined(BTI_DOS_32R) && !defined(BTI_DOS_32P) && \
    !defined(BTI_DOS_32B) && !defined(BTI_WIN_32)  && !defined(BTI_OS2_32) && \
    !defined(BTI_DOS_16B) && !defined(BTI_SOL)     && !defined(BTI_HPUX) && \
    !defined(BTI_AIX)     && !defined(BTI_IRIX)    && !defined(BTI_DEC_UNIX) && \
    !defined(BTI_LINUX)
#error You must define one of the following: BTI_WIN, BTI_OS2, BTI_DOS, BTI_NLM, BTI_DOS_32R, BTI_DOS_32P, BTI_DOS_32B, BTI_WIN_32, BTI_OS2_32, BTI_DOS_16B, BTI_SOL, BTI_HPUX, BTI_AIX, BTI_IRIX, BTI_DEC_UNIX, BTI_LINUX
#endif

   /* ONLY FOR NLM APPS THAT PASS PARAMETERS IN REGISTERS */
   #if defined (__WATCOMC__) && defined (BTI_NLM)
      /***********************************************************************
         Declare one of the  following cdecl pragmas IF AND ONLY IF your
         NLM application is using register-based parameter-passing.  Most
         users will not have to modify this file.
      ***********************************************************************/
      /********************* WATCOM Compiler Version <= 8.5 ******************
       #define USING_REGISTERS
       #pragma aux cdecl "*" parm caller []\
               value struct float struct routine [eax]\
               modify [eax ecx edx fs gs 8087];
      ***********************************************************************/
      /********************* WATCOM Compiler Version >= 9.01 *****************
      #define USING_REGISTERS
      #pragma aux cdecl "*" parm caller []\
              modify [eax ecx edx gs 8087];
      ***********************************************************************/
   #endif

  /* Platform-Dependent Definitions for BTI_FAR and BTI_PASCAL */
   #if defined(BTI_WIN)
      #define BTI_FAR       far
      #define BTI_PASCAL    pascal
   #endif

   #if defined(BTI_WIN_32)
     #include <windows.h>
     #define BTI_FAR
     #define BTI_PASCAL APIENTRY
   #endif

   #if defined(BTI_NLM) || defined(BTI_DOS_32R) || defined(BTI_DOS_32P)
      #define BTI_FAR
      #define BTI_PASCAL
   #endif

   #if defined(BTI_DOS_32B)
      #define BTI_FAR
      #define BTI_PASCAL pascal
   #endif

   #if defined(BTI_DOS_16B)
      #define BTI_FAR       far
      #define BTI_PASCAL    pascal
   #endif

   #if defined(BTI_OS2)
      #define BTI_FAR       far
      #define BTI_PASCAL    pascal
   #endif

   #if defined(BTI_DOS)
      #define BTI_FAR       far
      #define BTI_PASCAL
   #endif

   #if defined(BTI_OS2_32)
      #define BTI_FAR
      #define BTI_PASCAL    pascal
      #define INCL_BASE
      #include <os2.h>
   #endif

   #if defined( BTI_OS2 ) || defined( BTI_OS2_32 )
   typedef unsigned short int BTI_SIZE;
   #else
   #include <stdlib.h>
   typedef size_t BTI_SIZE;
   #endif

   #if defined(BTI_SOL) || defined(BTI_HPUX) || defined(BTI_AIX) || \
       defined(BTI_IRIX) || defined(BTI_DEC_UNIX) || defined(BTI_LINUX)
      #define BTI_FAR
      #define BTI_PASCAL
   #endif

   /* Platform-Independent Data Types */
   typedef long                  BTI_LONG;
   typedef unsigned long         BTI_ULONG;
   typedef short int             BTI_SINT;
   typedef unsigned short        BTI_WORD;
   typedef int                   BTI_INT;
   typedef char                  BTI_CHAR;
   typedef unsigned char         BTI_BYTE;
   typedef void                  BTI_VOID;
   typedef BTI_CHAR     BTI_FAR* BTI_CHAR_PTR;
   typedef BTI_BYTE     BTI_FAR* BTI_BUFFER_PTR;
   typedef BTI_SINT     BTI_FAR* BTI_SINT_PTR;
   typedef BTI_INT      BTI_FAR* BTI_INT_PTR;
   typedef BTI_WORD     BTI_FAR* BTI_WORD_PTR;
   typedef BTI_LONG     BTI_FAR* BTI_LONG_PTR;
   typedef BTI_ULONG    BTI_FAR* BTI_ULONG_PTR;
   typedef BTI_VOID     BTI_FAR* BTI_VOID_PTR;
   typedef BTI_SIZE     BTI_FAR* BTI_SIZE_PTR;

   /***********************************************************************
      Currently, OS2 32-bit Scalable SQL applications must be passed to a
      16-bit entry point.   BTI_SQL is temporary until there is a 32-bit
      entry point for Scalable SQL applications on OS2.
   ************************************************************************/
   #if defined( BTI_OS2_32 ) && defined( BTI_SQL )
   #include <os2def.h>
   #if defined( __BORLANDC__ )
   #define BTI_CHAR_PTR    BTI_CHAR  FAR16PTR
   #define BTI_BUFFER_PTR  BTI_BYTE  FAR16PTR
   #define BTI_SINT_PTR    BTI_SINT  FAR16PTR
   #define BTI_INT_PTR     BTI_INT   FAR16PTR
   #define BTI_WORD_PTR    BTI_WORD  FAR16PTR
   #define BTI_LONG_PTR    BTI_LONG  FAR16PTR
   #define BTI_ULONG_PTR   BTI_ULONG FAR16PTR
   #define BTI_VOID_PTR    BTI_VOID  FAR16PTR
   #define BTI_SIZE_PTR    BTI_SIZE  FAR16PTR
   #else   /* IBM Visual Age Compiler, others */
   #define BTI_CHAR_PTR    BTI_CHAR  * _Seg16
   #define BTI_BUFFER_PTR  BTI_BYTE  * _Seg16
   #define BTI_SINT_PTR    BTI_SINT  * _Seg16
   #define BTI_INT_PTR     BTI_INT   * _Seg16
   #define BTI_WORD_PTR    BTI_WORD  * _Seg16
   #define BTI_LONG_PTR    BTI_LONG  * _Seg16
   #define BTI_ULONG_PTR   BTI_ULONG * _Seg16
   #define BTI_VOID_PTR    BTI_VOID  * _Seg16
   #define BTI_SIZE_PTR    BTI_SIZE  * _Seg16
   #endif
   #endif

   #if defined(USING_REGISTERS)
   #define BTI_API extern BTI_SINT BTI_FAR BTI_PASCAL cdecl
   #endif

   #if !defined(USING_REGISTERS)  && !defined( BTI_OS2_32 ) && \
       !defined( BTI_WIN_32 )
   #define BTI_API extern BTI_SINT BTI_FAR BTI_PASCAL
   #endif

   #if defined( BTI_OS2_32 )
   #if defined( BTI_SQL )
   #define BTI_API BTI_SINT APIENTRY16
   #else
   #define BTI_API BTI_SINT APIENTRY
   #endif
   #endif

   #if defined( BTI_WIN_32 )
   #include <windef.h>
   #define BTI_API extern BTI_SINT BTI_FAR WINAPI
   #endif

   /* Callback Functions Data Types for MS Windows Applications */
   #if defined(BTI_WIN) || defined(BTI_WIN_32)

   typedef struct
   {
      BTI_WORD iSessionID;
   } NWSQL_YIELD_T;

   typedef struct
   {
      BTI_WORD  iOpCode;
      BTI_BYTE  bClientIDlastFour[ 4 ];
   } BTRV_YIELD_T;

   typedef struct
   {
      BTI_WORD type;
      BTI_WORD size;
      union
      {
          NWSQL_YIELD_T sYield;
          BTRV_YIELD_T  bYield;
      } u;
   } BTI_CB_INFO_T;

   typedef BTI_SINT ( BTI_FAR BTI_PASCAL *BTI_CB_FUNC_PTR_T ) (
                                        BTI_CB_INFO_T BTI_FAR *bCallbackInfo,
                                        BTI_VOID_PTR           bUserData );

   #endif

#define _BTITYPES_H_INCLUDED
#endif
