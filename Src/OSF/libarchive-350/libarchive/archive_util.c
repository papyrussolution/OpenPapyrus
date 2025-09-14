/*-
 * Copyright (c) 2009-2012,2014 Michihiro NAKAJIMA
 * Copyright (c) 2003-2007 Tim Kientzle
 * All rights reserved.
 */
#include "archive_platform.h"
#pragma hdrstop
__FBSDID("$FreeBSD: head/lib/libarchive/archive_util.c 201098 2009-12-28 02:58:14Z kientzle $");

#if defined(HAVE_WINCRYPT_H) && !defined(__CYGWIN__)
#include <wincrypt.h>
#endif
#ifdef HAVE_BZLIB_H
	#include <..\slib\bzip2\bzlib.h>
#endif
#ifdef HAVE_LZ4_H
	#include <..\slib\lz4\lz4.h>
#endif
#include "archive_random_private.h"

#ifndef O_CLOEXEC
	#define O_CLOEXEC       0
#endif

static int archive_utility_string_sort_helper(char **, uint);

/* Generic initialization of 'Archive' objects. */
int __archive_clean(Archive * a)
{
	archive_string_conversion_free(a);
	return ARCHIVE_OK;
}

int archive_version_number(void) { return (ARCHIVE_VERSION_NUMBER); }
const char * archive_version_string(void) { return (ARCHIVE_VERSION_STRING); }
int archive_errno(Archive * a) { return (a->archive_error_number); }
const char * archive_error_string(Archive * a) { return (a && !isempty(a->error)) ? a->error : NULL; }
int archive_file_count(Archive * a) { return (a->file_count); }
int archive_format(Archive * a) { return (a->archive_format); }
const char * archive_format_name(Archive * a) { return (a->archive_format_name); }
int archive_compression(Archive * a) { return archive_filter_code(a, 0); }
const char * archive_compression_name(Archive * a) { return archive_filter_name(a, 0); }
/*
 * Return a count of the number of compressed bytes processed.
 */
int64 archive_position_compressed(Archive * a) { return archive_filter_bytes(a, -1); }
/*
 * Return a count of the number of uncompressed bytes processed.
 */
int64 archive_position_uncompressed(Archive * a) { return archive_filter_bytes(a, 0); }

void archive_clear_error(Archive * a)
{
	archive_string_empty(&a->error_string);
	a->error = NULL;
	a->archive_error_number = 0;
}

void archive_set_error(Archive * a, int error_number, const char * fmt, ...)
{
	va_list ap;
	a->archive_error_number = error_number;
	if(fmt == NULL)
		a->error = NULL;
	else {
		archive_string_empty(&(a->error_string));
		va_start(ap, fmt);
		archive_string_vsprintf(&(a->error_string), fmt, ap);
		va_end(ap);
		a->error = a->error_string.s;
	}
}

void archive_copy_error(Archive * dest, Archive * src)
{
	dest->archive_error_number = src->archive_error_number;
	archive_string_copy(&dest->error_string, &src->error_string);
	dest->error = dest->error_string.s;
}

void FASTCALL __archive_errx(int retvalue, const char * msg)
{
	static const char msg1[] = "Fatal Internal Error in libarchive: ";
	size_t s = write(2, msg1, strlen(msg1));
	(void)s; /*unused*/
	s = write(2, msg, strlen(msg));
	(void)s; /*unused*/
	s = write(2, "\n", 1);
	(void)s; /*unused*/
	exit(retvalue);
}
/*
 * Create a temporary file
 */
#if defined(_WIN32) && !defined(__CYGWIN__)
/*
 * Do not use Windows tmpfile() function.
 * It will make a temporary file under the root directory
 * and it'll cause permission error if a user who is
 * non-Administrator creates temporary files.
 * Also Windows version of mktemp family including _mktemp_s
 * are not secure.
 */
static int __archive_mktempx(const char * tmpdir, wchar_t * pTemplate)
{
	static const wchar_t prefix[] = L"libarchive_";
	static const wchar_t suffix[] = L"XXXXXXXXXX";
	static const wchar_t num[] = {
		L'0', L'1', L'2', L'3', L'4', L'5', L'6', L'7',
		L'8', L'9', L'A', L'B', L'C', L'D', L'E', L'F',
		L'G', L'H', L'I', L'J', L'K', L'L', L'M', L'N',
		L'O', L'P', L'Q', L'R', L'S', L'T', L'U', L'V',
		L'W', L'X', L'Y', L'Z', L'a', L'b', L'c', L'd',
		L'e', L'f', L'g', L'h', L'i', L'j', L'k', L'l',
		L'm', L'n', L'o', L'p', L'q', L'r', L's', L't',
		L'u', L'v', L'w', L'x', L'y', L'z'
	};
	archive_wstring temp_name;
	DWORD attr;
	wchar_t * xp, * ep;
	HCRYPTPROV hProv = (HCRYPTPROV)NULL;
	int fd = -1;
	wchar_t * ws = NULL;
	if(pTemplate == NULL) {
		archive_string_init(&temp_name);
		/* Get a temporary directory. */
		if(tmpdir == NULL) {
			wchar_t * tmp;
			size_t l = GetTempPathW(0, NULL);
			if(l == 0) {
				la_dosmaperr(GetLastError());
				goto exit_tmpfile;
			}
			tmp = (wchar_t *)SAlloc::M(l*sizeof(wchar_t));
			if(!tmp) {
				errno = ENOMEM;
				goto exit_tmpfile;
			}
			GetTempPathW((DWORD)l, tmp);
			archive_wstrcpy(&temp_name, tmp);
			SAlloc::F(tmp);
		}
		else {
			if(archive_wstring_append_from_mbs(&temp_name, tmpdir, strlen(tmpdir)) < 0)
				goto exit_tmpfile;
			if(temp_name.s[temp_name.length-1] != L'/')
				archive_wstrappend_wchar(&temp_name, L'/');
		}
		/* Check if temp_name is a directory. */
		attr = GetFileAttributesW(temp_name.s);
		if(attr == (DWORD)-1) {
			if(GetLastError() != ERROR_FILE_NOT_FOUND) {
				la_dosmaperr(GetLastError());
				goto exit_tmpfile;
			}
			ws = __la_win_permissive_name_w(temp_name.s);
			if(ws == NULL) {
				errno = EINVAL;
				goto exit_tmpfile;
			}
			attr = GetFileAttributesW(ws);
			if(attr == (DWORD)-1) {
				la_dosmaperr(GetLastError());
				goto exit_tmpfile;
			}
		}
		if(!(attr & FILE_ATTRIBUTE_DIRECTORY)) {
			errno = ENOTDIR;
			goto exit_tmpfile;
		}
		// 
		// Create a temporary file.
		// 
		archive_wstrcat(&temp_name, prefix);
		archive_wstrcat(&temp_name, suffix);
		ep = temp_name.s + archive_strlen(&temp_name);
		xp = ep - wcslen(suffix);
		pTemplate = temp_name.s;
	}
	else {
		xp = wcschr(pTemplate, L'X');
		if(xp == NULL)  /* No X, programming error */
			abort();
		for(ep = xp; *ep == L'X'; ep++)
			continue;
		if(*ep) /* X followed by non X, programming error */
			abort();
	}
	if(!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
		la_dosmaperr(GetLastError());
		goto exit_tmpfile;
	}
	for(;;) {
		HANDLE h;
		// Generate a random file name through CryptGenRandom()
		wchar_t * p = xp;
		if(!CryptGenRandom(hProv, (DWORD)(ep - p)*sizeof(wchar_t), (BYTE*)p)) {
			la_dosmaperr(GetLastError());
			goto exit_tmpfile;
		}
		for(; p < ep; p++)
			*p = num[((DWORD)*p) % SIZEOFARRAY(num)];
		SAlloc::F(ws);
		ws = __la_win_permissive_name_w(pTemplate);
		if(ws == NULL) {
			errno = EINVAL;
			goto exit_tmpfile;
		}
		if(pTemplate == temp_name.s) {
			attr = FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE;
		}
		else {
			/* mkstemp */
			attr = FILE_ATTRIBUTE_NORMAL;
		}
		h = CreateFileW(ws, GENERIC_READ|GENERIC_WRITE|DELETE, 0/* Not share */, NULL, CREATE_NEW/* Create a new file only */, attr, NULL);
		if(h == INVALID_HANDLE_VALUE) {
			// The same file already exists. retry with a new filename. 
			if(GetLastError() == ERROR_FILE_EXISTS)
				continue;
			/* Otherwise, fail creation temporary file. */
			la_dosmaperr(GetLastError());
			goto exit_tmpfile;
		}
		fd = _open_osfhandle((intptr_t)h, _O_BINARY | _O_RDWR);
		if(fd == -1) {
			la_dosmaperr(GetLastError());
			CloseHandle(h);
			goto exit_tmpfile;
		}
		else
			break; /* success! */
	}
exit_tmpfile:
	if(hProv != (HCRYPTPROV)NULL)
		CryptReleaseContext(hProv, 0);
	SAlloc::F(ws);
	if(pTemplate == temp_name.s)
		archive_wstring_free(&temp_name);
	return (fd);
}

int __archive_mktemp(const char * tmpdir) { return __archive_mktempx(tmpdir, NULL); }
int __archive_mkstemp(wchar_t * pTemplate) { return __archive_mktempx(NULL, pTemplate); }

#else

static int get_tempdir(archive_string * temppath)
{
	const char * tmp = getenv("TMPDIR");
	if(!tmp)
#ifdef _PATH_TMP
		tmp = _PATH_TMP;
#else
		tmp = "/tmp";
#endif
	archive_strcpy(temppath, tmp);
	if(temppath->s[temppath->length-1] != '/')
		archive_strappend_char(temppath, '/');
	return ARCHIVE_OK;
}

#if defined(HAVE_MKSTEMP)
/*
 * We can use mkstemp().
 */
int __archive_mktemp(const char * tmpdir)
{
	archive_string temp_name;
	int fd = -1;
	archive_string_init(&temp_name);
	if(tmpdir == NULL) {
		if(get_tempdir(&temp_name) != ARCHIVE_OK)
			goto exit_tmpfile;
	}
	else {
		archive_strcpy(&temp_name, tmpdir);
		if(temp_name.s[temp_name.length-1] != '/')
			archive_strappend_char(&temp_name, '/');
	}
#ifdef O_TMPFILE
	fd = open(temp_name.s, O_RDWR|O_CLOEXEC|O_TMPFILE|O_EXCL, 0600);
	if(fd >= 0)
		goto exit_tmpfile;
#endif
	archive_strcat(&temp_name, "libarchive_XXXXXX");
	fd = mkstemp(temp_name.s);
	if(fd < 0)
		goto exit_tmpfile;
	__archive_ensure_cloexec_flag(fd);
	unlink(temp_name.s);
exit_tmpfile:
	archive_string_free(&temp_name);
	return (fd);
}

int __archive_mkstemp(char * pTemplate)
{
	int fd = mkstemp(pTemplate);
	if(fd >= 0)
		__archive_ensure_cloexec_flag(fd);
	return (fd);
}

#else /* !HAVE_MKSTEMP */
/*
 * We use a private routine.
 */
static int __archive_mktempx(const char * tmpdir, char * pTemplate)
{
	static const char num[] = {
		'0', '1', '2', '3', '4', '5', '6', '7',
		'8', '9', 'A', 'B', 'C', 'D', 'E', 'F',
		'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
		'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V',
		'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd',
		'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l',
		'm', 'n', 'o', 'p', 'q', 'r', 's', 't',
		'u', 'v', 'w', 'x', 'y', 'z'
	};
	archive_string temp_name;
	struct stat st;
	char * tp, * ep;
	int fd = -1;
	if(pTemplate == NULL) {
		archive_string_init(&temp_name);
		if(tmpdir == NULL) {
			if(get_tempdir(&temp_name) != ARCHIVE_OK)
				goto exit_tmpfile;
		}
		else
			archive_strcpy(&temp_name, tmpdir);
		if(temp_name.s[temp_name.length-1] == '/') {
			temp_name.s[temp_name.length-1] = '\0';
			temp_name.length--;
		}
		if(la_stat(temp_name.s, &st) < 0)
			goto exit_tmpfile;
		if(!S_ISDIR(st.st_mode)) {
			errno = ENOTDIR;
			goto exit_tmpfile;
		}
		archive_strcat(&temp_name, "/libarchive_");
		tp = temp_name.s + archive_strlen(&temp_name);
		archive_strcat(&temp_name, "XXXXXXXXXX");
		ep = temp_name.s + archive_strlen(&temp_name);
		pTemplate = temp_name.s;
	}
	else {
		tp = sstrchr(pTemplate, 'X');
		if(tp == NULL)  /* No X, programming error */
			abort();
		for(ep = tp; *ep == 'X'; ep++)
			continue;
		if(*ep)         /* X followed by non X, programming error */
			abort();
	}
	do {
		char * p = tp;
		archive_random(p, ep - p);
		while(p < ep) {
			int d = *((uchar *)p) % sizeof(num);
			*p++ = num[d];
		}
		fd = open(pTemplate, O_CREAT | O_EXCL | O_RDWR | O_CLOEXEC, 0600);
	} while(fd < 0 && errno == EEXIST);
	if(fd < 0)
		goto exit_tmpfile;
	__archive_ensure_cloexec_flag(fd);
	if(pTemplate == temp_name.s)
		unlink(temp_name.s);
exit_tmpfile:
	if(pTemplate == temp_name.s)
		archive_string_free(&temp_name);
	return (fd);
}

int __archive_mktemp(const char * tmpdir)
{
	return __archive_mktempx(tmpdir, NULL);
}

int __archive_mkstemp(char * pTemplate)
{
	return __archive_mktempx(NULL, pTemplate);
}

#endif /* !HAVE_MKSTEMP */
#endif /* !_WIN32 || __CYGWIN__ */

/*
 * Set FD_CLOEXEC flag to a file descriptor if it is not set.
 * We have to set the flag if the platform does not provide O_CLOEXEC
 * or F_DUPFD_CLOEXEC flags.
 *
 * Note: This function is absolutely called after creating a new file
 * descriptor even if the platform seemingly provides O_CLOEXEC or
 * F_DUPFD_CLOEXEC macros because it is possible that the platform
 * merely declares those macros, especially Linux 2.6.18 - 2.6.24 do it.
 */
void __archive_ensure_cloexec_flag(int fd)
{
#if defined(_WIN32) && !defined(__CYGWIN__)
	CXX_UNUSED(fd);
#else
	int flags;
	if(fd >= 0) {
		flags = fcntl(fd, F_GETFD);
		if(flags != -1 && (flags & FD_CLOEXEC) == 0)
			fcntl(fd, F_SETFD, flags | FD_CLOEXEC);
	}
#endif
}

/*
 * Utility function to sort a group of strings using quicksort.
 */
static int archive_utility_string_sort_helper(char ** strings, uint n)
{
	uint i, lesser_count, greater_count;
	char ** lesser, ** greater, ** tmp, * pivot;
	int retval1, retval2;
	/* A list of 0 or 1 elements is already sorted */
	if(n <= 1)
		return ARCHIVE_OK;
	lesser_count = greater_count = 0;
	lesser = greater = NULL;
	pivot = strings[0];
	for(i = 1; i < n; i++) {
		if(strcmp(strings[i], pivot) < 0) {
			lesser_count++;
			tmp = (char**)SAlloc::R(lesser,
				lesser_count * sizeof(char *));
			if(!tmp) {
				SAlloc::F(greater);
				SAlloc::F(lesser);
				return ARCHIVE_FATAL;
			}
			lesser = tmp;
			lesser[lesser_count - 1] = strings[i];
		}
		else {
			greater_count++;
			tmp = (char**)SAlloc::R(greater,
				greater_count * sizeof(char *));
			if(!tmp) {
				SAlloc::F(greater);
				SAlloc::F(lesser);
				return ARCHIVE_FATAL;
			}
			greater = tmp;
			greater[greater_count - 1] = strings[i];
		}
	}
	/* quicksort(lesser) */
	retval1 = archive_utility_string_sort_helper(lesser, lesser_count);
	for(i = 0; i < lesser_count; i++)
		strings[i] = lesser[i];
	SAlloc::F(lesser);
	/* pivot */
	strings[lesser_count] = pivot;
	/* quicksort(greater) */
	retval2 = archive_utility_string_sort_helper(greater, greater_count);
	for(i = 0; i < greater_count; i++)
		strings[lesser_count + 1 + i] = greater[i];
	SAlloc::F(greater);
	return (retval1 < retval2) ? retval1 : retval2;
}

int archive_utility_string_sort(char ** strings)
{
	uint size = 0;
	while(strings[size])
		size++;
	return archive_utility_string_sort_helper(strings, size);
}
