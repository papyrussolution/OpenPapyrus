// GETOPT.H
// Declarations for getopt.
// Copyright (C) 1989-1994, 1996-1999, 2001 Free Software Foundation, Inc.
// This file is part of the GNU C Library.
// 
// The GNU C Library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
// 
// The GNU C Library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public
// License along with the GNU C Library; if not, write to the Free
// Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
// 
// Adopted to Papyrus project by A.Sobolev 2021
// The replacement of multiple different variants coming from outer libraries
//
#ifndef __GETOPT_H // {
#define __GETOPT_H
//
// Names for the values of the `has_arg' field of `struct option'
//
#define no_argument       0
#define required_argument 1
#define optional_argument 2

struct option {
	const  char * name;
	int    has_arg; // no_argument || required_argument || optional_argument
	int *  flag;
	int    val;
};

#ifdef	__cplusplus
//extern "C" {
#endif
	// 
	// For communication from `getopt' to the caller.
	// When `getopt' finds an option that takes an argument, the argument value is returned here.
	// Also, when `ordering' is RETURN_IN_ORDER, each non-option ARGV-element is returned here.
	// 
	extern char * optarg;
	// 
	// Index in ARGV of the next element to be scanned.
	// This is used for communication to and from the caller
	// and for communication between successive calls to `getopt'.
	// 
	// On entry to `getopt', zero means this is the first call; initialize.
	// 
	// When `getopt' returns -1, this is the index of the first of the
	// non-option elements that the caller should itself scan.
	// 
	// Otherwise, `optind' communicates from one call to the next
	// how much of ARGV has been scanned so far.  */
	// 
	extern int optind;
	// 
	// Callers store zero here to inhibit the error message `getopt' prints for unrecognized options.
	// 
	extern int opterr;
	// 
	// Set to an option character which was unrecognized.
	// 
	extern int optopt;
	extern int optreset; // @? (\openjpeg\src\bin\jp3d\getopt.h)

	// Many other libraries have conflicting prototypes for getopt, with
	// differences in the consts, in stdlib.h.  To avoid compilation
	// errors, only prototype getopt for the GNU C library.
	extern int getopt(int argc, char * const * ppArgv, const char * pShortOpts);
	extern int getopt_long(int argc, char *const * argv, const char *__shortopts, const struct option * __longopts, int * __longind);
	extern int getopt_long_only(int argc, char *const * argv, const char *__shortopts, const struct option * __longopts, int * __longind);
	extern void getopt_init(); // @? (\winflexbison\common\getopt.h)

	// Internal only.  Users should not call this directly. 
	extern int _getopt_internal(int argc, char * const * argv, const char * shortopts, const struct option * longopts, int * longind, int long_only);

#ifdef	__cplusplus
//}
#endif

#endif // } __GETOPT_H