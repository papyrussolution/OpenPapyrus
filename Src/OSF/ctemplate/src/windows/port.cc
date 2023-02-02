/* Copyright (c) 2007, Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 * ---
 * Author: Craig Silverstein
 */
#include <ctemplate-internal.h>
#pragma hdrstop
#ifndef _WIN32
	#error You should only be including windows/port.cc in a windows environment!
#endif
#include "port.h"

// These call the windows _vsnprintf, but always NUL-terminate.
#if !defined(__MINGW32__) && !defined(__MINGW64__) && (!defined(_MSC_VER) || _MSC_VER < 1900)  /* mingw already defines
                                                                                                  */
int safe_vsnprintf(char * str, size_t size, const char * format, va_list ap) 
{
	if(!size)   // not even room for a \0?
		return -1; // not what C99 says to do, but what windows does
	str[size-1] = '\0';
	return _vsnprintf(str, size-1, format, ap);
}

int snprintf(char * str, size_t size, const char * format, ...) 
{
	int r;
	va_list ap;
	va_start(ap, format);
	r = vsnprintf(str, size, format, ap);
	va_end(ap);
	return r;
}

#endif  /* #if !defined(__MINGW32__) && !defined(__MINGW64__) */

#ifdef __cplusplus
#include <string>
#include <vector>
#include <ctemplate/template_pathops.h>

using std::string;
using std::vector;

namespace ctemplate {
// defined (for unix) in template_test_utils.cc
string TmpFile(const char* basename) 
{
	char tmppath_buffer[1024];
	int tmppath_len = GetTempPathA(sizeof(tmppath_buffer), tmppath_buffer);
	if(tmppath_len <= 0 || tmppath_len >= sizeof(tmppath_buffer)) {
		return basename; // an error, so just bail on tmppath
	}
	assert(tmppath_buffer[tmppath_len - 1] == '\\'); // API guarantees it
	return string(tmppath_buffer) + basename;
}

// A replacement for template_unittest.cc:CleanTestDir()
void CreateOrCleanTestDir(const string& dirname) 
{
	string glob(PathJoin(dirname, "*"));
	WIN32_FIND_DATAA found; // that final A is for Ansi (as opposed to Unicode)
	HANDLE hFind = FindFirstFileA(glob.c_str(), &found); // A is for Ansi
	if(hFind == INVALID_HANDLE_VALUE) { // directory doesn't exist or some such
		_mkdir(dirname.c_str());
		hFind = FindFirstFileA(glob.c_str(), &found); // Try again...
	}
	if(hFind != INVALID_HANDLE_VALUE) {
		do {
			if(strstr(found.cFileName, "template"))
				_unlink(PathJoin(dirname, found.cFileName).c_str());
		} while(FindNextFileA(hFind, &found) != FALSE); // A is for Ansi
		FindClose(hFind);
	}
}
}

void GetNamelist(const char* testdata_dir, vector<string>* namelist) 
{
	string glob(ctemplate::PathJoin(testdata_dir, "template_unittest_test*"));
	WIN32_FIND_DATAA found; // that final A is for Ansi (as opposed to Unicode)
	HANDLE hFind = FindFirstFileA(glob.c_str(), &found);
	if(hFind == INVALID_HANDLE_VALUE) // no files matching the glob, probably
		return; // if we don't find any files, nothing to add to namelist
	do {
		namelist->push_back(found.cFileName);
	} while(FindNextFileA(hFind, &found) != FALSE); // A is for Ansi
	FindClose(hFind);
}

#endif  /* __cplusplus */
