#ifndef _BTRAPI_H_INCLUDED
/*************************************************************************
**
**  Copyright 1982-2001 Pervasive Software Inc. All Rights Reserved
**
*************************************************************************/
/***************************************************************************
 BTRAPI.H
    This file contains prototypes for the DOS, Extended DOS, MS Windows,
    MS Windows NT, MS Windows 95, OS2, NLM, and Unix Btrieve call.   The
    calling application may be C or C++.

    You must define one of the following to your compiler in order to
    compile for a particular platform:

        BTI_DOS     - DOS                                     16-bit Apps
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
        BTI_LINUX   - Linux

    If you do not specify one of the above switches to your compiler,
    an error directive will halt the compilation.

    Modules are:
      btrapi.c     - compile and link with all apps except NLMs
      btrsamp.c    - sample program which can be compiled for any platform
      btitypes.h   - platform-independent data types
      blobhdr.h    - used by apps which use BTI_DOS_32B or BTI_DOS_32P
      btrapi.h     - included by all apps
      btrconst.h   - Btrieve constants
      bmemcopy.obj - used by apps which use BTI_DOS_32B or BTI_DOS_32P
      w3btrv7.lib  - used by 32-bit apps; versions are provided for
                     Microsoft Visual C++ apps ("COFF" format) and
                     Borland, Watcom, and other compilers ( see NT note
                     below )

    Except for DOS or Extended DOS, the Btrieve application developer will
    also need either an import library or linker import statements to
    identify the imported Btrieve functions to the linker.  The imported
    functions are:

      MS Windows
      ----------
      W1BTRV7.LIB  - WBTRVINIT, WBRQSHELLINIT, WBTRVSTOP, BTRCALL,
                     BTRCALLID, BTRCALLBACK, RQSHELLINIT
      NT and WIN95
      -------------
      W3BTRV7.LIB - WBTRVINIT, WBRQSHELLINIT, WBTRVSTOP, BTRCALL,
                    BTRCALLID, BTRCALLBACK, RQSHELLINIT ( see NT note below )

      OS2
      ---
      O3BTRV7.LIB - BTRVINIT,  BRQSHELLINIT,  BTRVSTOP,  BTRCALL

      NLM
      ---
      Use linker import statements - btrv, btrvID

      Unix
      -------
      RDA - Tbd

    You will need to compile and link 'btrapi.c' if you call any of
    the following functions from the indicated platforms:
          BTRV .......... MS Windows, OS2, DOS
          BTRVID ........ MS Windows, OS2, DOS
          RQSHELLINIT ... MS Windows

    For 32-Bit DOS Rational Applications Using Bstub
    ------------------------------------------------
    If you are using the Rational Systems DOS Extender, DOS4G.EXE, along
    with BSTUB.EXE, you will need to define the plaform switch,
    BTI_DOS_32R to your compiler.  You also must link your application
    using the Rational linker, GLU.EXE.  You MUST have a '.def' file
    similar to the following which is used to create a 32-bit btrsamp.exe:

         btrsamp.def
         -----------
            LIBRARY btrsamp.dll
            DATA NONSHARED
            EXPORTS
               __ImportedFunctions_

    The module, btrapi.c, requires '__ImportedFunctions_' as shown in
    the above btrsamp.def.  This allows btrapi.c to call a function
    in Btrieve without using int386.

    Depending on your compiler's naming conventions, you may need to
    add a leading underscore to '_ImportedFunctions' at 3 locations in
    btrapi.c.  For example, when you compile with WATCOM using register-based
    parameter-passing, no change to '_ImportedFunctions' is needed.  But
    if you compile using stack-based parameter-passing, you will need to
    change '_ImportedFunctions' to '__ImportedFunctions' in btrapi.c.  We
    could have added compiler predefined macros to btrapi.c to handle
    this naming problem for you, except we were not assured of the
    predefined macros being present in all cases.

    The following is the GLU '.lnk' file for sqlsamp.exe using WATCOM
    libraries:

          btrsamp.lnk
          -----------
            btrsamp.obj
            btrapi.obj
            -format lin -stack 40000 -dbg -stub bstub.exe
            e(btrsamp)
            m(btrsamp)
            dos32wc.lib
            clib3r.lib
            math3r.lib
            math387r.lib
            emu387.lib

    You invoke GLU.EXE as follows:

            glu -deffile btrsamp.def @btrsamp.lnk


    For 32-Bit Borland PowerPack and Phar Lap DOS Applications
    -----------------------------------------------------------
    You must link 'bmemcopy.obj' with your application.

   For 32-Bit NT, Win95, and Win32s Applications
   ---------------------------------------------
   - If using WATCOM, you must use version 10.5 or later.  Earlier versions
     do not understand the format of the import library, w3btrv7.lib.

   - If you are using a compiler that complains that w3btrv7.lib has
     an invalid format, you will need to get a version of that compiler that
     understands the 'coff' format used by Microsoft Visual C++.


***************************************************************************/
#if !defined(BTI_WIN)     && !defined(BTI_OS2)     && !defined(BTI_DOS) && \
    !defined(BTI_NLM)     && !defined(BTI_DOS_32R) && !defined(BTI_DOS_32P) && \
    !defined(BTI_DOS_32B) && !defined(BTI_WIN_32)  && !defined(BTI_OS2_32) && \
    !defined(BTI_DOS_16B) && !defined(BTI_SOL)     && !defined(BTI_HPUX) && \
    !defined(BTI_LINUX)
#error You must define one of the following: BTI_WIN, BTI_OS2, BTI_DOS, BTI_NLM, BTI_DOS_32R, BTI_DOS_32P, BTI_DOS_32B, BTI_WIN_32, BTI_OS2_32, BTI_DOS_16B, BTI_SOL, BTI_HPUX, BTI_AIX, BTI_IRIX, BTI_DEC_UNIX, BTI_LINUX
#endif

#include <btitypes.h>

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************
   PLATFORM-INDEPENDENT FUNCTIONS
     These APIs are the same on all platforms for which they have
     an implementation.  We recommend that you use only these two
     functions with Btrieve 6.x client components, and then issue the
     B_STOP operation prior to exiting your application.
****************************************************************************/
BTI_API BTRV(
           BTI_WORD     operation,
           BTI_VOID_PTR posBlock,
           BTI_VOID_PTR dataBuffer,
           BTI_WORD_PTR dataLength,
           BTI_VOID_PTR keyBuffer,
           BTI_SINT     keyNumber );

BTI_API BTRVID(
           BTI_WORD       operation,
           BTI_VOID_PTR   posBlock,
           BTI_VOID_PTR   dataBuffer,
           BTI_WORD_PTR   dataLength,
           BTI_VOID_PTR   keyBuffer,
           BTI_SINT       keyNumber,
           BTI_BUFFER_PTR clientID );

/***************************************************************************
   PLATFORM-SPECIFIC FUNCTIONS
      These APIs are specific to the indicated platform.  With the
      exception of BTRCALLBACK, we recommend that you use either
      BTRV or BTRVID, shown above.  Slight performance gains can be
      achieved by using BTRCALL or BTRCALLID.
****************************************************************************/
#if defined( BTI_NLM )
BTI_API btrv(
           BTI_WORD     operation,
           BTI_VOID_PTR posBlock,
           BTI_VOID_PTR dataBuffer,
           BTI_WORD_PTR dataLength,
           BTI_VOID_PTR keyBuffer,
           BTI_SINT     keyNumber );
#define BTRV( a, b, c, d, e, f )  btrv( a, b, c, d, e, f )

BTI_API btrvID(
           BTI_WORD       operation,
           BTI_VOID_PTR   posBlock,
           BTI_VOID_PTR   dataBuffer,
           BTI_WORD_PTR   dataLength,
           BTI_VOID_PTR   keyBuffer,
           BTI_SINT       keyNumber,
           BTI_BUFFER_PTR clientID );
#define BTRVID( a, b, c, d, e, f, g )  btrvID( a, b, c, d, e, f, g )
#endif

#if defined( BTI_WIN ) || defined( BTI_OS2 )
BTI_API BTRCALL(
           BTI_WORD     operation,
           BTI_VOID_PTR posBlock,
           BTI_VOID_PTR dataBuffer,
           BTI_WORD_PTR dataLength,
           BTI_VOID_PTR keyBuffer,
           BTI_BYTE     keyLength,
           BTI_CHAR     ckeynum );

BTI_API BTRCALLID(
           BTI_WORD       operation,
           BTI_VOID_PTR   posBlock,
           BTI_VOID_PTR   dataBuffer,
           BTI_WORD_PTR   dataLength,
           BTI_VOID_PTR   keyBuffer,
           BTI_BYTE       keyLength,
           BTI_CHAR       ckeynum,
           BTI_BUFFER_PTR clientID );
#endif

#if defined( BTI_WIN_32) || defined( BTI_OS2_32 ) || defined( BTI_SOL ) || \
    defined( BTI_HPUX) || defined( BTI_AIX ) || defined( BTI_IRIX ) || \
    defined( BTI_DEC_UNIX)
#if defined( BTI_OS2_32 )
#define BTRCALL   BTRCALL32
#define BTRCALLID BTRCALLID32
#endif
BTI_API BTRCALL(
           BTI_WORD      operation,
           BTI_VOID_PTR  posBlock,
           BTI_VOID_PTR  dataBuffer,
           BTI_ULONG_PTR dataLength,
           BTI_VOID_PTR  keyBuffer,
           BTI_BYTE      keyLength,
           BTI_CHAR      ckeynum );

BTI_API BTRCALLID(
           BTI_WORD       operation,
           BTI_VOID_PTR   posBlock,
           BTI_VOID_PTR   dataBuffer,
           BTI_ULONG_PTR  dataLength,
           BTI_VOID_PTR   keyBuffer,
           BTI_BYTE       keyLength,
           BTI_CHAR       ckeynum,
           BTI_BUFFER_PTR clientID );
#endif

#if defined( BTI_WIN ) || defined( BTI_WIN_32 )
BTI_API BTRCallback(
           BTI_WORD                   iAction,
           BTI_WORD                   iOption,
           BTI_CB_FUNC_PTR_T          fCallBackFunction,
           BTI_CB_FUNC_PTR_T BTI_FAR *fPreviousCallBackFunction,
           BTI_VOID_PTR               bUserData,
           BTI_VOID_PTR BTI_FAR      *bPreviousUserData,
           BTI_BUFFER_PTR             bClientID );
#endif


/***************************************************************************
   HISTORICAL FUNCTIONS
      These APIs were needed prior to Btrieve 6.x client
      components.  Older applications may still call these functions,
      and the later Btrieve client components will do the appropriate
      thing, depending on the platform.  New applications using the
      later client components do NOT have to call these functions.
****************************************************************************/
#if defined( BTI_WIN ) || defined( BTI_WIN_32 )
#define BTRVINIT WBTRVINIT
#define BTRVSTOP WBTRVSTOP
BTI_API WBTRVINIT( BTI_CHAR_PTR option );
BTI_API WBRQSHELLINIT( BTI_CHAR_PTR option );
BTI_API WBTRVSTOP( BTI_VOID );
BTI_API RQSHELLINIT( BTI_CHAR_PTR options );
#endif

#if defined( BTI_OS2 )
BTI_API BTRVINIT( BTI_CHAR_PTR options );
BTI_API BTRVSTOP( BTI_VOID );
BTI_API BRQSHELLINIT( BTI_VOID );
#endif

#if defined( BTI_OS2_32 )
BTI_API BTRVINIT32( BTI_CHAR_PTR options );
BTI_API BTRVSTOP32( BTI_VOID );
BTI_API BRQSHELLINIT32( BTI_VOID );
#endif

#ifdef __cplusplus
}
#endif

#define _BTRAPI_H_INCLUDED
#endif
