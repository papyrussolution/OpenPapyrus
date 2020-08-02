// SLPORT.H
// Copyright (c) A.Sobolev 2020
//
#ifndef __SLPORT_H // {
#define __SLPORT_H
#if defined(_WIN32) || defined(_WIN64)
	//typedef long pid_t;
	#define	getpid GetCurrentProcessId
	#define chdir SetCurrentDirectory
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
	#define strcasecmp  _stricmp
	#define strncasecmp _strnicmp
#endif
#endif // } __SLPORT_H