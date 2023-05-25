// SLCONFIG.H
// Copyright (c) Sobolev A. 1995, 1996, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2016, 2017, 2018, 2019, 2020, 2021, 2022, 2023
// @codepage UTF-8
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
// Visual Studio 2017 RTW (15.0) 	1910
// Visual Studio 2017 версия 15.3 	1911
// Visual Studio 2017 версии 15.5 	1912
// Visual Studio 2017 версии 15.6 	1913
// Visual Studio 2017 версии 15.7 	1914
// Visual Studio 2017 версии 15.8 	1915
// Visual Studio 2017 версии 15.9 	1916
// Visual Studio 2019 версии 16.11.2 (update 11) 1929
// Visual Studio 2019 версии 16.9.2  (update  9) 1928
// Visual Studio 2019 версии 16.8.2  (update  8) 1928
// Visual Studio 2019 версии 16.8.1  (update  8) 1928
// Visual Studio 2019 версии 16.7    (update  7) 1927
// Visual Studio 2019 версии 16.6.2  (update  6) 1926
// Visual Studio 2019 версии 16.5.1  (update  5) 1925
// Visual Studio 2019 версии 16.4.0  (update  4) 1924
// Visual Studio 2019 версии 16.3.2  (update  3) 1923
// Visual Studio 2019 версии 16.2.3  (update  2) 1922
// Visual Studio 2019 версии 16.1.2  (update  1) 1921
// Visual Studio 2019 версии 16.0.0              1920
//
// _WIN32_WINNT:
// _WIN32_WINNT_NT4                    0x0400 // Windows NT 4.0
// _WIN32_WINNT_WIN2K                  0x0500 // Windows 2000
// _WIN32_WINNT_WINXP                  0x0501 // Windows XP
// _WIN32_WINNT_WS03                   0x0502 // Windows Server 2003
// _WIN32_WINNT_WIN6                   0x0600 // Windows Vista
// _WIN32_WINNT_VISTA                  0x0600 // Windows Vista
// _WIN32_WINNT_WS08                   0x0600 // Windows Server 2008
// _WIN32_WINNT_LONGHORN               0x0600 // Windows Vista
// _WIN32_WINNT_WIN7                   0x0601 // Windows 7
// _WIN32_WINNT_WIN8                   0x0602 // Windows 8
// _WIN32_WINNT_WINBLUE                0x0603 // Windows 8.1
// _WIN32_WINNT_WINTHRESHOLD           0x0A00 // Windows 10
// _WIN32_WINNT_WIN10                  0x0A00 // Windows 10
//
#if(_MSC_VER < 1900)
	#ifndef _WIN32_WINNT
		#define _WIN32_WINNT 0x0501
	#endif
	#ifndef NTDDI_VERSION
		#define NTDDI_VERSION NTDDI_WINXP
	#endif
#else
	#ifndef _WIN32_WINNT
		#ifdef NGINX_SLIB
			#define _WIN32_WINNT 0x0600
		#else
			// @v10.7.11 #define _WIN32_WINNT 0x0501 // @v9.1.12 0x0600-->0x0501
			#define _WIN32_WINNT 0x0600 // @v10.7.11
			//#define _WIN32_WINNT 0x0603 // @v10.9.9
		#endif
	#endif
	#ifndef NTDDI_VERSION
		#define NTDDI_VERSION NTDDI_WIN8 // @v11.6.7 NTDDI_WIN7-->NTDDI_WIN8
	#endif
	#define _CRT_SECURE_NO_WARNINGS
	// @v10.9.10 {
	#if(_MSC_VER >= 1928)
		#define WINDOWS_IGNORE_PACKING_MISMATCH
	#endif
	// } @v10.9.10 
#endif
#define __STDC_LIMIT_MACROS // @v10.5.4

/* Remove comment if clarion date format is needed
#define USE_DF_CLARION */
// @v11.7.4 (replaced with SlConst::DefaultBinDateFormat) #define DEFAULT_BIN_DATE_FORMAT   DF_BTRIEVE
// @v11.7.4 (replaced with SlConst::DefaultTxtDateFormat) #define DEFAULT_TXT_DATE_FORMAT   DATF_GERMAN
// @v11.7.4 (replaced with SlConst::DefaultOverflowSymb) #define DEFAULT_OVERFLOW_SYMB     '#'
// @v11.7.4 (replaced with SlConst::DefaultPasswordSymb) #define DEFAULT_PASSWORD_SYMB     '*'

// #define OBJSTORE_STREAM // Remove comment if ObjStore streaming is needed
//
// Если !0, то некоторые низкоуровневые функции заменяются на соответствующие функции из ASMLIB
// Замена осуществляется в slib.h посредством #define
//
// После включения библиотеки в релиз Papyrus v7.0.10 обнаружились непонятные эффекты.
// В релизе 7.0.12 библиотека отключена. Если эффекты исчезнут, то придется писать
// нагрузочные тесты для проверки всех аспектов использования данной библиотеки.
//
#define USE_ASMLIB       1
#define USE_SSE2         1 // @v9.8.4 used by pixman. @attention: Эта опция увеличивает размер exe-файла на ~2Mb
//
//
//
#define SALLOC_STD       0
#define SALLOC_NEDMALLOC 1
#define USE_OPENSSL_STATIC
#define SALLOCATOR       SALLOC_STD
#define XXH_STATIC_LINKING_ONLY // @v10.5.6 for xxhash
//
// @v9.8.1 Если !0, то ведется отладочная трассировка внутренних блокировок.
// Актуально только для сервера.
//
#ifdef _PPSERVER
	#define SLTRACELOCKSTACK 1 // @v9.8.3 С помощью трассировки аномалии идентифицировать пока не удалось. До поры-времени отключаем.
		// @v9.8.8 Опять включили.
#else
	#define SLTRACELOCKSTACK 0
#endif
//
// @v10.8.0
// Descr: Если SLGATHERALLOCSTATISTICS != 0, то функции распределения памяти собирают статистику по количеству запросов
//   на распределение и перераспределение. 
// Attention: Включать можно только в отладочном режиме поскольку побочные эффекты в виде замедления работы и вероятных сбоев ожидаются значительные.
//
#define SLGATHERALLOCSTATISTICS 0 // !Не включать - убьет!
#define __STDC_FORMAT_MACROS      // @v10.9.3
//
// Descr: Если следующее определение != 0, то весь проект компилируется с включенными
//   кодами автоматического тестирования.
//
#define SLTEST_RUNNING          0

#endif /* __SLCONFIG_H */
