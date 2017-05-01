// SLCONFIG.H
// Copyright (c) Sobolev A. 1995, 1996, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2016, 2017
//
#ifndef __SLCONFIG_H
#define __SLCONFIG_H
//
// Справочная информация по значениям _MSC_VER
// MSVC++ 5.0  _MSC_VER == 1100
// MSVC++ 6.0  _MSC_VER == 1200
// MSVC++ 7.0  _MSC_VER == 1300
// MSVC++ 7.1  _MSC_VER == 1310 (Visual Studio 2003)
// MSVC++ 8.0  _MSC_VER == 1400 (Visual Studio 2005)
// MSVC++ 9.0  _MSC_VER == 1500 (Visual Studio 2008)
// MSVC++ 10.0 _MSC_VER == 1600 (Visual Studio 2010)
// MSVC++ 11.0 _MSC_VER == 1700 (Visual Studio 2012)
// MSVC++ 12.0 _MSC_VER == 1800 (Visual Studio 2013)
// MSVC++ 14.0 _MSC_VER == 1900 (Visual Studio 2015)
//
#if (_MSC_VER < 1900)
	#ifndef _WIN32_WINNT
		#define _WIN32_WINNT 0x0501
	#endif
	#ifndef NTDDI_VERSION
		#define NTDDI_VERSION NTDDI_WINXP
	#endif
#else
	#ifndef _WIN32_WINNT
		#define _WIN32_WINNT 0x0501 // @v9.1.12 0x0600-->0x0501
	#endif
	#ifndef NTDDI_VERSION
		#define NTDDI_VERSION NTDDI_WIN7
	#endif
#endif
#define _CRT_SECURE_NO_WARNINGS // @v9.1.11

/* Remove comment if clarion date format is needed
#define USE_DF_CLARION */
#define DEFAULT_BIN_DATE_FORMAT   DF_BTRIEVE
#define DEFAULT_TXT_DATE_FORMAT   DATF_GERMAN

#define DEFAULT_OVERFLOW_SYMB     '#'
#define DEFAULT_PASSWORD_SYMB     '*'

/* Maximum size of data types, processing by STYPE mechanism */
#define MAXSSIZE                 255

/* Remove comment if complex types are needed (see STYPE.H) */
#define COMPLEX_TYPES

/* Remove comment if ObjStore streaming is needed
#define OBJSTORE_STREAM */

/* Comment if 'typedef [un]signed [char|short|long] [u]int[8|16|32]' not needed */
#define DECL_INTX
//
// Требуется для MSVS 10.0 дабы он не выдавал замечания на использование "небезопасных" функций.
//
#define _CRT_SECURE_NO_WARNINGS
//
// Если !0, то некоторые низкоуровневые функции заменяются на соответствующие функции из ASMLIB
// Замена осуществляется в slib.h посредством #define
//
// После включения библиотеки в релиз Papyrus v7.0.10 обнаружились непонятные эффекты.
// В релизе 7.0.12 библиотека отключена. Если эффекты исчезнут, то придется писать
// нагрузочные тесты для проверки всех аспектов использования данной библиотеки.
//
#define USE_ASMLIB       1
//
//
//
#define SALLOC_STD       0
#define SALLOC_NEDMALLOC 1

#define SALLOCATOR SALLOC_STD
//
// Descr: Если следующее определение != 0, то весь проект компилируется с включенными
//   кодами автоматического тестирования.
//
#define SLTEST_RUNNING   0

#endif /* __SLCONFIG_H */
