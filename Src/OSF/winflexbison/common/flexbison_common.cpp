// flexbison_common.cpp
//
#include <flexbison_common.h>
#pragma hdrstop
#include "concat-filename.h"
#include "filename.h"
#include "path-join.h"
//
// EXITFAIL.C Failure exit status
//
int volatile exit_failure = EXIT_FAILURE;
//
// APP_PATH.C
//
#define APP_PATH_LEN  1024
static char app_path[APP_PATH_LEN+1];

const char * get_app_path()
{
	DWORD res = GetModuleFileNameA(NULL, app_path, APP_PATH_LEN);
	if(res == 0 || res == APP_PATH_LEN)
		return 0;
	app_path[APP_PATH_LEN] = '\0';
	return app_path;
}
//
// BASENAME-LGPL.C -- return the last element in a file name
// 
// Return the address of the last file name component of NAME.  If
// NAME has no relative file name components because it is a file
// system root, return the empty string.
// 
char * last_component(char const * name)
{
	char const * base = name + FILE_SYSTEM_PREFIX_LEN(name);
	char const * p;
	bool saw_slash = false;
	while(ISSLASH(*base))
		base++;
	for(p = base; *p; p++) {
		if(ISSLASH(*p))
			saw_slash = true;
		else if(saw_slash) {
			base = p;
			saw_slash = false;
		}
	}
	return (char *)base;
}
// 
// Return the length of the basename NAME.  Typically NAME is the
// value returned by base_name or last_component.  Act like strlen
// (NAME), except omit all trailing slashes.
// 
size_t base_len(char const * name)
{
	size_t len;
	size_t prefix_len = FILE_SYSTEM_PREFIX_LEN(name);
	for(len = strlen(name); 1 < len && ISSLASH(name[len - 1]); len--)
		continue;
	if(DOUBLE_SLASH_IS_DISTINCT_ROOT && len == 1 && ISSLASH(name[0]) && ISSLASH(name[1]) && !name[2])
		return 2;
	if(FILE_SYSTEM_DRIVE_PREFIX_CAN_BE_RELATIVE && prefix_len && len == prefix_len && ISSLASH(name[prefix_len]))
		return prefix_len + 1;
	return len;
}
//
// CONCAT-FILENAME.C Construct a full filename from a directory and a relative filename.
// Written by Bruno Haible <haible@clisp.cons.org>
//
char * _stpcpy(char * yydest, const char * yysrc)
{
	char * yyd = yydest;
	const char * yys = yysrc;
	while((*yyd++ = *yys++) != '\0')
		continue;
	return yyd - 1;
}
// 
// Concatenate a directory filename, a relative filename and an optional
// suffix.  The directory may end with the directory separator.  The second
// argument may not start with the directory separator (it is relative).
// Return a freshly allocated filename.  Return NULL and set errno
// upon memory allocation failure. 
// 
char * concatenated_filename(const char * directory, const char * filename, const char * suffix)
{
	char * result;
	char * p;
	if(sstreq(directory, ".")) {
		// No need to prepend the directory
		result = (char *)SAlloc::M(strlen(filename) + (suffix != NULL ? strlen(suffix) : 0) + 1);
		if(result == NULL)
			return NULL; /* errno is set here */
		p = result;
	}
	else {
		size_t directory_len = strlen(directory);
		int need_slash = ((int)(directory_len) > FILE_SYSTEM_PREFIX_LEN(directory) && !ISSLASH(directory[directory_len-1]));
		result = (char *)SAlloc::M(directory_len + need_slash + strlen(filename) + (suffix != NULL ? strlen(suffix) : 0) + 1);
		if(result == NULL)
			return NULL; /* errno is set here */
		memcpy(result, directory, directory_len);
		p = result + directory_len;
		if(need_slash)
			*p++ = '/';
	}
	p = _stpcpy(p, filename);
	if(suffix)
		_stpcpy(p, suffix);
	return result;
}
//
// DIRNAME.C Return all but the last element in a file name
// 
// Just like mdir_name (dirname-lgpl.c), except, rather than
// returning NULL upon malloc failure, here, we report the "memory exhausted" condition and exit.
// 
char * dir_name(char const * file)
{
	char * result = mdir_name(file);
	if(!result)
		xalloc_die();
	return result;
}
//
// DIRNAME-LGPL.C Return all but the last element in a file name
//
// 
// Return the length of the prefix of FILE that will be used by
// dir_name.  If FILE is in the working directory, this returns zero
// even though `dir_name (FILE)' will return ".".  Works properly even
// if there are trailing slashes (by effectively ignoring them). 
// 
size_t dir_len(char const * file)
{
	size_t prefix_length = FILE_SYSTEM_PREFIX_LEN(file);
	size_t length;
	// Advance prefix_length beyond important leading slashes. 
	prefix_length += (prefix_length != 0 ? (FILE_SYSTEM_DRIVE_PREFIX_CAN_BE_RELATIVE && ISSLASH(file[prefix_length])) : (ISSLASH(file[0])
	    ? ((DOUBLE_SLASH_IS_DISTINCT_ROOT && ISSLASH(file[1]) && !ISSLASH(file[2]) ? 2 : 1)) : 0));
	// Strip the basename and any redundant slashes before it. 
	for(length = last_component(file) - file; prefix_length < length; length--)
		if(!ISSLASH(file[length - 1]))
			break;
	return length;
}
// 
// In general, we can't use the builtin `dirname' function if available,
// since it has different meanings in different environments.
// In some environments the builtin `dirname' modifies its argument.
// 
// Return the leading directories part of FILE, allocated with malloc.
// Works properly even if there are trailing slashes (by effectively
// ignoring them).  Return NULL on failure.
// 
// If lstat (FILE) would succeed, then { chdir (dir_name (FILE));
// lstat (base_name (FILE)); } will access the same file.  Likewise,
// if the sequence { chdir (dir_name (FILE));
// rename (base_name (FILE), "foo"); } succeeds, you have renamed FILE
// to "foo" in the same directory FILE was in.  */
// 
char * mdir_name(char const * file)
{
	size_t length = dir_len(file);
	bool append_dot = (length == 0 || (FILE_SYSTEM_DRIVE_PREFIX_CAN_BE_RELATIVE && length == FILE_SYSTEM_PREFIX_LEN(file) && file[2] != '\0' && !ISSLASH(file[2])));
	char * dir = (char *)SAlloc::M(length + append_dot + 1);
	if(dir) {
		memcpy(dir, file, length);
		if(append_dot)
			dir[length++] = '.';
		dir[length] = '\0';
	}
	return dir;
}
//
// GET-ERRNO.C Get and set errno.
// Written by Paul Eggert
//
// Get and set errno.  A source file that needs to set or get errno,
// but doesn't need to test for specific errno values, can use these
// functions to avoid namespace pollution.  For example, a file that
// defines EQUAL should not include <errno.h>, since <errno.h> might
// define EQUAL; such a file can include <get-errno.h> instead.
// 
int    get_errno(void) { return errno; }
void   set_errno(int e) { errno = e; }
//
// GETLINE.C Implementation of replacement getline function.
// Written by Simon Josefsson
//
extern ssize_t getdelim(char ** lineptr, size_t * n, int delimiter, FILE * fp);

ssize_t getline(char ** lineptr, size_t * n, FILE * stream)
{
	return getdelim(lineptr, n, '\n', stream);
}
//
// PATH-JOIN.C Concatenate path components
// Written by Akim Demaille <akim@lrde.epita.fr>
//
char * xpath_join(const char * path1, const char * path2)
{
	if(!path2 || !*path2)
		return xstrdup(path1);
	else if(IS_ABSOLUTE_PATH(path2))
		return xstrdup(path2);
	else
		return xconcatenated_filename(path1, path2, NULL);
}
//
// XALLOC-DIE.C Report a memory allocation failure and exit
//
#undef _ // @sobolev
#define _(msgid) gettext(msgid)

void xalloc_die(void)
{
	error(exit_failure, 0, "%s", _("memory exhausted"));
	// The `noreturn' cannot be given to error, since it may return if
	// its first argument is 0.  To help compilers understand the
	// xalloc_die does not return, call abort.  Also, the abort is a
	// safety feature if exit_failure is 0 (which shouldn't happen). 
	abort();
}
#undef _ // @sobolev
//
// XTIME.C
//
#define XTIME_INLINE _GL_EXTERN_INLINE
#include "xtime.h"
#undef XTIME_INLINE
//
// TIMESPEC.C
//
#define _GL_TIMESPEC_INLINE _GL_EXTERN_INLINE
#include "timespec.h"
#undef _GL_TIMESPEC_INLINE
//
// MBFILE.C
//
#define MBFILE_INLINE _GL_EXTERN_INLINE
#include "mbfile.h"
#undef MBFILE_INLINE
//
// MBCHAR.C
//
#define MBCHAR_INLINE _GL_EXTERN_INLINE
#include "mbchar.h"

#if IS_BASIC_ASCII
	// Bit table of characters in the ISO C "basic character set"
	const uint is_basic_table[UCHAR_MAX / 32 + 1] =
	{
		0x00001a00,     /* '\t' '\v' '\f' */
		0xffffffef,     /* ' '...'#' '%'...'?' */
		0xfffffffe,     /* 'A'...'Z' '[' '\\' ']' '^' '_' */
		0x7ffffffe      /* 'a'...'z' '{' '|' '}' '~' */
		// The remaining bits are 0
	};
#endif
#undef MBCHAR_INLINE
//
//
//
