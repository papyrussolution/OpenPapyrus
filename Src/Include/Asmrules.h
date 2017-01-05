/*------------------------------------------------------------------------
 * filename - asmrules.h
 *
 *      Rules & structures useful for in-line assembler
 *-----------------------------------------------------------------------*/

/*[]------------------------------------------------------------[]*/
/*|                                                              |*/
/*|     C/C++ Run Time Library - Version 4.0                     |*/
/*|                                                              |*/
/*|                                                              |*/
/*|     Copyright (c) 1987, 1991 by Borland International        |*/
/*|     All Rights Reserved.                                     |*/
/*|                                                              |*/
/*[]------------------------------------------------------------[]*/

#ifdef __BORLANDC__
#	if !defined(_Windows) && !defined(__WIN32__)
#		pragma inline
#	endif
#	pragma warn -asm
#endif

/****
#define  LPROG  (4 == sizeof (void (*) (void)))
#define  LDATA  (4 == sizeof (void *))
****/
#if defined(__LARGE__) || defined(__HUGE__) || defined(__COMPACT__)
#	define LDATA 1
#else
#	define LDATA 0
#endif

#if defined(__LARGE__) || defined(__HUGE__) || defined(__MEDIUM__)
#	define LPROG 1
#else
#	define LPROG 0
#endif

#define  W0(ea) (word ptr (ea))
#define  W1(ea) (word ptr (ea) [2])

#define  BY0(ea) (byte ptr (ea))
#define  BY1(ea) (byte ptr (ea) [1])

#define  FLOAT(ea)  (dword ptr (ea))
#define  DOUBLE(ea) (qword ptr (ea))
#define  LONGDOUBLE(ea) (tbyte ptr (ea))

#define _SimLocalCall_  asm     db      0E8h, 0, 0      /* call $       */
/*      jmp     toLocalProc                             NEVER USE "SHORT" ! */

#define  RETNEAR asm  db   0C3h

#if LDATA
#   define  LES_  LES
#   define  ES_   ES:
#   define  SS_   SS:
#   define  DPTR_(ea)  (dword ptr (ea))
#   define  dPtrSize    4
#   define  pushDS_     asm     push    DS
#   define  LDS_        LDS
#   define  popDS_      asm     pop     DS
#else
#   define  LES_  mov
#   define  ES_
#   define  SS_
#   define  DPTR_(ea)  (word ptr (ea))
#   define  dPtrSize    2
#   define  pushDS_
#   define  LDS_        mov
#   define  popDS_
#endif

#if LPROG
#   define  CPTR_(ea)  (dword ptr (ea))
#   define  EXTPROC(x)  (far ptr (x))
#   define  cPtrSize    4
#else
#   define  CPTR_(ea)  (word ptr (ea))
#   define  EXTPROC(x)  (x)
#   define  cPtrSize    2
#endif

/* Use these macros inside of a function to force an si or di save/restore */
#define SaveSI                  asm     __savesi equ si
#define SaveDI                  asm     __savedi equ di

/* Use these macros when you want to return a value in DX:AX and
   avoid a warning messages being generated.  Ex: return( MK_LONG ); */
#define MK_LONG   (long)((void _seg *)(_DX) + (void near *)(_AX))
#define MK_ULONG  (unsigned long)((void _seg *)(_DX) + (void near *)(_AX))
