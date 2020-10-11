// SLPORT.H
// Copyright (c) A.Sobolev 2020
// @codepage UTF-8
// Ётот заголовочный файл призван унифицировать большинство макроопределений и деклараций, реализующих портируемость
// компил€ции между платформами и компил€торами.
// ‘актически, € буду здесь собирать эклектические выжимки из разных библиотек, унифициру€ их как только могу.
//
// ƒа, € знаю, что не первый пытаюсь это сделать, и предполагаю что выйдет плохо, но мен€ замучали дублированные в определени€,
// и невозможность скомпилировать сторонние библиотеки, которые мне нужны.
//
#ifndef __SLPORT_H // {
#define __SLPORT_H
#include <cxx_detect.h> // @v10.9.0
//
// »дентификаци€ разр€дности компилируемого кода
// 
#if defined(_WIN32)
	#if defined(_WIN64)
		#define __SL_PLATFORM_BIT 64
	#else
		#define __SL_PLATFORM_BIT 32
	#endif
#elif __GNUC__
	#if __x86_64__ || __ppc64__
		#define __SL_PLATFORM_BIT 64
	#else
		#define __SL_PLATFORM_BIT 32
	#endif
#endif
#ifndef SIZEOF_OFF_T
	#if defined(__VMS) && !defined(__VAX)
		#if defined(_LARGEFILE)
			#define SIZEOF_OFF_T 8
		#endif
	#elif defined(__OS400__) && defined(__ILEC400__)
		#if defined(_LARGE_FILES)
			#define SIZEOF_OFF_T 8
		#endif
	#elif defined(__MVS__) && defined(__IBMC__)
		#if defined(_LP64) || defined(_LARGE_FILES)
			#define SIZEOF_OFF_T 8
		#endif
	#elif defined(__370__) && defined(__IBMC__)
		#if defined(_LP64) || defined(_LARGE_FILES)
			#define SIZEOF_OFF_T 8
		#endif
	#endif
	#ifndef SIZEOF_OFF_T
		#define SIZEOF_OFF_T 4
	#endif
#endif
#ifndef SIZEOF_SIZE_T
	#if __SL_PLATFORM_BIT==64
		#define SIZEOF_SIZE_T 8
	#else
		#define SIZEOF_SIZE_T 4
	#endif
#endif
#if defined(_WIN32) || defined(_WIN64)
	//typedef long pid_t;
	// @v10.8.5 #define getpid GetCurrentProcessId
	// @v10.8.5 #define chdir  SetCurrentDirectory
	#ifndef STDIN_FILENO
		#define STDIN_FILENO (_fileno(stdin))
	#endif
	#ifndef STDOUT_FILENO
		#define STDOUT_FILENO (_fileno(stdout))
	#endif
	#ifdef _WIN64
		#define fseeko      _fseeki64
		#define ftello      _ftelli64
	#else
		#define fseeko      fseek
		#define ftello      ftell
	#endif
	// @variant: #define fseeko(s, o, w)	(fseek((s), static_cast<long>(o), (w)))
	// @variant: #define ftello(s)	(static_cast<long>(ftell((s))))
	#define strcasecmp  _stricmp
	#define strncasecmp _strnicmp
#endif
#endif // } __SLPORT_H