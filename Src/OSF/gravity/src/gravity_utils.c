//  gravity_utils.c
//  gravity
//
//  Created by Marco Bambini on 29/08/14.
//  Copyright (c) 2014 CreoLabs. All rights reserved.
//
#include <gravity_.h>
#pragma hdrstop

#define SWP(x, y) (x ^= y, y ^= x, x ^= y)

// MARK: Timer -

nanotime_t nanotime() 
{
	nanotime_t value;
    #if defined(_WIN32)
		static LARGE_INTEGER win_frequency;
		QueryPerformanceFrequency(&win_frequency);
		LARGE_INTEGER t;
		if(!QueryPerformanceCounter(&t)) 
			return 0;
		value = (t.QuadPart / win_frequency.QuadPart) * SlConst::OneBillion;
		value += (t.QuadPart % win_frequency.QuadPart) * SlConst::OneBillion / win_frequency.QuadPart;
    #elif defined(__MACH__)
		mach_timebase_info_data_t info;
		nanotime_t t = mach_absolute_time();
		kern_return_t r = mach_timebase_info(&info);
		if(r) return 0;
		value = (t / info.denom) * info.numer;
		value += (t % info.denom) * info.numer / info.denom;
    #elif defined(__linux)
		struct timespec ts;
		int r = clock_gettime(CLOCK_MONOTONIC, &ts);
		if(r) return 0;
		value = ts.tv_sec * (nanotime_t)SlConst::OneBillion + ts.tv_nsec;
    #else
		struct timeval tv;
		int r = gettimeofday(&tv, 0);
		if(r) 
			return 0;
		value = tv.tv_sec * (nanotime_t)SlConst::OneBillion + tv.tv_usec * 1000;
    #endif
	return value;
}

double microtime(nanotime_t tstart, nanotime_t tend) 
{
	nanotime_t t = tend - tstart;
	return ((double)t / 1000.0f);
}

double millitime(nanotime_t tstart, nanotime_t tend) 
{
	nanotime_t t = tend - tstart;
	return ((double)t / 1000000.0f);
}

// MARK: - Console Functions -

#ifdef WIN32
// getline is a POSIX function not available in C on Windows (only C++)
static ssize_t getline(char ** lineptr, size_t * n, FILE * stream) 
{
	// to be implemented on Windows
	// Never use gets: it offers no protections against a buffer overflow vulnerability.
	// see http://stackoverflow.com/questions/3302255/c-scanf-vs-gets-vs-fgets
	// we should implement something like ggets here
	// http://web.archive.org/web/20080525133110/http://cbfalconer.home.att.net/download/

	return -1;
}
#endif

char * readline(char * prompt, int * length) 
{
	char * line = NULL;
	size_t size = 0;
	printf("%s", prompt);
	fflush(stdout);
	ssize_t nread = getline(&line, &size, stdin);
	if(nread == -1 || feof(stdin)) return NULL;
	*length = (int)nread;
	return line;
}

// MARK: - I/O Functions -

uint64_t file_size(const char * path) 
{
#ifdef WIN32
	WIN32_FILE_ATTRIBUTE_DATA fileInfo;
	if(GetFileAttributesExA(path, GetFileExInfoStandard, (void *)&fileInfo) == 0) return -1;
		return (uint64_t)(((__int64)fileInfo.nFileSizeHigh) << 32 ) + fileInfo.nFileSizeLow;
#else
	struct stat sb;
	if(stat(path, &sb) > 0) 
		return -1;
	return (uint64_t)sb.st_size;
#endif
}

const char * file_read(const char * path, size_t * len) 
{
	int fd = 0;
	size_t fsize2 = 0;
	char * buffer = NULL;
	off_t fsize = (off_t)file_size(path);
	if(fsize < 0) 
		goto abort_read;
	fd = _open(path, O_RDONLY);
	if(fd < 0) 
		goto abort_read;
	buffer = (char *)mem_alloc(NULL, (size_t)fsize + 1);
	if(!buffer) 
		goto abort_read;
	buffer[fsize] = 0;
	fsize2 = _read(fd, buffer, (size_t)fsize);
	if(fsize2 == -1) 
		goto abort_read;
	ASSIGN_PTR(len, fsize2);
	_close(fd);
	return (const char *)buffer;
abort_read:
	mem_free((void *)buffer);
	if(fd >= 0) 
		_close(fd);
	return NULL;
}

/* @sobolev (replaced with SLIB fileExists(const char *)) bool file_exists(const char * path) 
{
#ifdef WIN32
	const DWORD attributes = GetFileAttributesA(path);
	// special directory case to drive the network path check
	const BOOL is_directory = (attributes == INVALID_FILE_ATTRIBUTES) ? (GetLastError() == ERROR_BAD_NETPATH) : (FILE_ATTRIBUTE_DIRECTORY & attributes);
	if(is_directory) {
		if(PathIsNetworkPathA(path)) 
			return true;
		else if(PathIsUNCA(path)) 
			return true;
	}
	if(PathFileExistsA(path) == 1) 
		return true;
#else
	if(access(path, F_OK)==0) 
		return true;
#endif
	return false;
}*/

const char * file_buildpath(const char * filename, const char * dirpath) 
{
//    #ifdef WIN32
//    PathCombineA(result, filename, dirpath);
//    #else
	size_t len1 = strlen(filename);
	size_t len2 = strlen(dirpath);
	size_t len = len1+len2+2;
	char * full_path = (char *)mem_alloc(NULL, len);
	if(!full_path) 
		return NULL;
	if((len2) && (dirpath[len2-1] != '/'))
		snprintf(full_path, len, "%s/%s", dirpath, filename);
	else
		snprintf(full_path, len, "%s%s", dirpath, filename);
//    #endif
	return (const char *)full_path;
}

bool file_write(const char * path, const char * buffer, size_t len) 
{
	// RW for owner, R for group, R for others
    #ifdef _WIN32
		mode_t mode = _S_IWRITE;
    #else
		mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    #endif
	int fd = _open(path, O_WRONLY | O_CREAT | O_TRUNC, mode);
	if(fd < 0) 
		return false;
	ssize_t nwrite = (ssize_t)_write(fd, buffer, len);
	_close(fd);
	return (nwrite == len);
}

// MARK: - Directory Functions -

bool is_directory(const char * path) 
{
    #ifdef WIN32
		DWORD dwAttrs = GetFileAttributesA(path);
		if(dwAttrs == INVALID_FILE_ATTRIBUTES) return false;
		if(dwAttrs & FILE_ATTRIBUTE_DIRECTORY) return true;
    #else
		struct stat buf;
		if(lstat(path, &buf) < 0) return false;
		if(S_ISDIR(buf.st_mode)) return true;
    #endif
	return false;
}

DIRREF directory_init(const char * dirpath) 
{
	#ifdef WIN32
		WIN32_FIND_DATAW findData;
		WCHAR path[MAX_PATH];
		WCHAR dirpathW[MAX_PATH];
		HANDLE hFind;
		(void)hFind;
		// convert dirpath to dirpathW
		MultiByteToWideChar(CP_UTF8, 0, dirpath, -1, dirpathW, MAX_PATH);
		// in this way I can be sure that the first file returned (and lost) is .
		PathCombineW(path, dirpathW, L"*");
		// if the path points to a symbolic link, the WIN32_FIND_DATA buffer contains
		// information about the symbolic link, not the target
		return FindFirstFileW(path, &findData);
	#else
		return opendir(dirpath);
	#endif
}

const char * directory_read(DIRREF ref, char * out) 
{
	if(ref) {
		while(1) {
			#ifdef WIN32
				WIN32_FIND_DATAA findData;
				if(FindNextFileA(ref, &findData) == 0) {
					FindClose(ref);
					return NULL;
				}
				if(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) 
					continue;
				if(findData.cFileName[0] == '\0') 
					continue;
				if(findData.cFileName[0] == '.') 
					continue;
				// cFileName from WIN32_FIND_DATAA is a fixed size array, and findData is local
				// This line of code is under the assumption that `out` is at least MAX_PATH in size!
				return !out ? NULL : static_cast<char *>(memcpy(out, findData.cFileName, sizeof(findData.cFileName)));
			#else
				struct dirent * d;
				if((d = readdir(ref)) == NULL) {
					closedir(ref);
					return NULL;
				}
				if(d->d_name[0] == '\0') 
					continue;
				if(d->d_name[0] == '.') 
					continue;
				return (const char *)d->d_name;
			#endif
		}
	}
	return NULL;
}

// MARK: - String Functions -

int string_nocasencmp(const char * s1, const char * s2, size_t n) 
{
	while(n > 0 && tolower((uchar)*s1) == tolower((uchar)*s2)) {
		if(*s1 == '\0') 
			return 0;
		s1++;
		s2++;
		n--;
	}
	if(!n) 
		return 0;
	return tolower((uchar)*s1) - tolower((uchar)*s2);
}

int FASTCALL string_casencmp(const char * s1, const char * s2, size_t n) 
{
	while(n > 0 && ((uchar)*s1) == ((uchar)*s2)) {
		if(*s1 == '\0') 
			return 0;
		s1++;
		s2++;
		n--;
	}
	if(!n) 
		return 0;
	return ((uchar)*s1) - ((uchar)*s2);
}

/*int FASTCALL string_cmp(const char * s1, const char * s2) 
{
	if(!s1) return 1;
	return strcmp(s1, s2);
}*/
/*const char * string_dup_(const char * s1) 
{
	size_t len = (size_t)strlen(s1);
	char    * s = (char *)mem_alloc(NULL, len + 1);
	if(!s) return NULL;
	memcpy(s, s1, len);
	return s;
}*/

const char * string_ndup(const char * s1, size_t n) 
{
	char * s = static_cast<char *>(mem_alloc(NULL, n + 1));
	if(s)
		memcpy(s, s1, n);
	return s;
}

// From: http://stackoverflow.com/questions/198199/how-do-you-reverse-a-string-in-place-in-c-or-c
void string_reverse(char * p) 
{
	char * q = p;
	while(q && *q) 
		++q; /* find eos */
	for(--q; p < q; ++p, --q) 
		SWP(*p, *q);
}

// @sobolev (replaced with sstrlen) uint32 string_size(const char * p) { return p ? (uint32)strlen(p) : 0; }

// From: https://opensource.apple.com/source/Libc/Libc-339/string/FreeBSD/strnstr.c
char * string_strnstr(const char * s, const char * find, size_t slen) 
{
	char c, sc;
	size_t len;
	if((c = *find++) != '\0') {
		len = strlen(find);
		do {
			do {
				if((sc = *s++) == '\0' || slen-- < 1)
					return NULL;
			} while(sc != c);
			if(len > slen)
				return NULL;
		} while(strncmp(s, find, len) != 0);
		s--;
	}
	return ((char *)s);
}

// From: https://creativeandcritical.net/str-replace-c
char * string_replace(const char * str, const char * from, const char * to, size_t * rlen) 
{
	// cache related settings
	size_t cache_sz_inc = 16;
	const size_t cache_sz_inc_factor = 3;
	const size_t cache_sz_inc_max = 1048576;
	char * pret, * ret = NULL;
	const char * pstr2, * pstr = str;
	size_t i, count = 0;
	uintptr_t * pos_cache_tmp, * pos_cache = NULL;
	size_t cache_sz = 0;
	size_t cpylen, orglen, retlen = 0, tolen = 0, fromlen = strlen(from);
	ASSIGN_PTR(rlen, 0);
	// find all matches and cache their positions
	while((pstr2 = strstr(pstr, from)) != NULL) {
		++count;
		// Increase the cache size when necessary
		if(cache_sz < count) {
			cache_sz += cache_sz_inc;
			pos_cache_tmp = static_cast<uintptr_t *>(mem_realloc(NULL, pos_cache, sizeof(*pos_cache) * cache_sz));
			if(pos_cache_tmp == NULL) {
				goto end_repl_str;
			}
			else {
				pos_cache = pos_cache_tmp;
			}
			cache_sz_inc *= cache_sz_inc_factor;
			if(cache_sz_inc > cache_sz_inc_max) {
				cache_sz_inc = cache_sz_inc_max;
			}
		}
		pos_cache[count-1] = pstr2 - str;
		pstr = pstr2 + fromlen;
	}
	orglen = pstr - str + strlen(pstr);
	// allocate memory for the post-replacement string
	if(count > 0) {
		tolen = strlen(to);
		retlen = orglen + (tolen - fromlen) * count;
	}
	else {
		retlen = orglen;
	}
	ret = (char *)mem_alloc(NULL, retlen + 1);
	if(ret == NULL) {
		goto end_repl_str;
	}

	if(!count) {
		// if no matches, then just duplicate the string
		strcpy(ret, str);
	}
	else {
		//therwise, duplicate the string while performing the replacements using the position cache
		pret = ret;
		memcpy(pret, str, pos_cache[0]);
		pret += pos_cache[0];
		for(i = 0; i < count; ++i) {
			memcpy(pret, to, tolen);
			pret += tolen;
			pstr = str + pos_cache[i] + fromlen;
			cpylen = (i == count-1 ? orglen : pos_cache[i+1]) - pos_cache[i] - fromlen;
			memcpy(pret, pstr, cpylen);
			pret += cpylen;
		}
		ret[retlen] = '\0';
	}

end_repl_str:
	// free the cache and return the post-replacement string which will be NULL in the event of an error
	mem_free(pos_cache);
	if(rlen && ret) 
		*rlen = retlen;
	return ret;
}

// MARK: - UTF-8 Functions -

/*
    Based on: https://github.com/Stepets/utf8.lua/blob/master/utf8.lua
    ABNF from RFC 3629

    UTF8-octets = *( UTF8-char )
    UTF8-char   = UTF8-1 / UTF8-2 / UTF8-3 / UTF8-4
    UTF8-1      = %x00-7F
    UTF8-2      = %xC2-DF UTF8-tail
    UTF8-3      = %xE0 %xA0-BF UTF8-tail / %xE1-EC 2( UTF8-tail ) /
                  %xED %x80-9F UTF8-tail / %xEE-EF 2( UTF8-tail )
    UTF8-4      = %xF0 %x90-BF 2( UTF8-tail ) / %xF1-F3 3( UTF8-tail ) /
                  %xF4 %x80-8F 2( UTF8-tail )
    UTF8-tail   = %x80-BF

 */

/*inline*/ uint32 FASTCALL utf8_charbytes(const char * s, uint32 i) 
{
	uchar c = s[i];
	// determine bytes needed for character, based on RFC 3629
	if((c > 0) && (c <= 127)) return 1;
	if((c >= 194) && (c <= 223)) return 2;
	if((c >= 224) && (c <= 239)) return 3;
	if((c >= 240) && (c <= 244)) return 4;
	// means error
	return 0;
}

uint32 utf8_nbytes(uint32 n) 
{
	if(n <= 0x7f) return 1;     // 127
	if(n <= 0x7ff) return 2;    // 2047
	if(n <= 0xffff) return 3;   // 65535
	if(n <= 0x10ffff) return 4; // 1114111
	return 0;
}

// from: https://github.com/munificent/wren/blob/master/src/vm/wren_utils.c
uint32 utf8_encode(char * buffer, uint32 value) 
{
	char * bytes = buffer;
	if(value <= 0x7f) {
		// single byte (i.e. fits in ASCII).
		*bytes = value & 0x7f;
		return 1;
	}
	if(value <= 0x7ff) {
		// two byte sequence: 110xxxxx 10xxxxxx.
		*bytes = static_cast<char>(0xc0 | ((value & 0x7c0) >> 6));
		++bytes;
		*bytes = 0x80 | (value & 0x3f);
		return 2;
	}

	if(value <= 0xffff) {
		// three byte sequence: 1110xxxx 10xxxxxx 10xxxxxx.
		*bytes = 0xe0 | ((value & 0xf000) >> 12);
		++bytes;
		*bytes = static_cast<char>(0x80 | ((value & 0xfc0) >> 6));
		++bytes;
		*bytes = 0x80 | (value & 0x3f);
		return 3;
	}

	if(value <= 0x10ffff) {
		// four byte sequence: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx.
		*bytes = static_cast<char>(0xf0 | ((value & 0x1c0000) >> 18));
		++bytes;
		*bytes = static_cast<char>(0x80 | ((value & 0x3f000) >> 12));
		++bytes;
		*bytes = static_cast<char>(0x80 | ((value & 0xfc0) >> 6));
		++bytes;
		*bytes = 0x80 | (value & 0x3f);
		return 4;
	}

	return 0;
}

uint32 FASTCALL utf8_len(const char * s, uint32 nbytes) 
{
	uint32 len = 0;
	if(s) {
		SETIFZ(nbytes, (uint32)strlen(s));
		for(uint32 pos = 0; pos < nbytes;) {
			++len;
			uint32 n = utf8_charbytes(s, pos);
			if(!n) 
				return 0; // means error
			pos += n;
		}
	}
	return len;
}

// From: http://stackoverflow.com/questions/198199/how-do-you-reverse-a-string-in-place-in-c-or-c
bool utf8_reverse(char * p) 
{
	char * q = p;
	string_reverse(p);
	// now fix bass-ackwards UTF chars.
	while(q && *q) ++q; // find eos
	while(p < --q)
		switch( (*q & 0xF0) >> 4) {
			case 0xF: /* U+010000-U+10FFFF: four bytes. */
			    if(q-p < 4) return false;
			    SWP(*(q-0), *(q-3));
			    SWP(*(q-1), *(q-2));
			    q -= 3;
			    break;
			case 0xE: /* U+000800-U+00FFFF: three bytes. */
			    if(q-p < 3) return false;
			    SWP(*(q-0), *(q-2));
			    q -= 2;
			    break;
			case 0xC: // @fallthrough
			case 0xD: /* U+000080-U+0007FF: two bytes. */
			    if(q-p < 1) return false;
			    SWP(*(q-0), *(q-1));
			    q--;
			    break;
		}
	return true;
}

// MARK: - Math -

// From: http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2Float
// WARNING: this function returns 0 if n is greater than 2^31
uint32 power_of2_ceil(uint32 n) 
{
	n--;
	n |= n >> 1;
	n |= n >> 2;
	n |= n >> 4;
	n |= n >> 8;
	n |= n >> 16;
	n++;
	return n;
}

int64_t number_from_hex(const char * s, uint32 len) 
{
	// LLONG_MIN  = -9223372036854775808
	// LLONG_MAX  = +9223372036854775807
	// HEX(9223372036854775808) = 346DC5D638865

	// sanity check on len in order to workaround an address sanityzer error
	return (len > 24) ? 0 : (int64_t)strtoll(s, NULL, 16);
}

int64_t number_from_oct(const char * s, uint32 len) 
{
	// LLONG_MIN  = -9223372036854775808
	// LLONG_MAX  = +9223372036854775807
	// OCT(9223372036854775808) = 32155613530704145

	// sanity check on len in order to workaround an address sanityzer error
	return (len > 24) ? 0 : (int64_t)strtoll(s, NULL, 8);
}

int64_t number_from_bin(const char * s, uint32 len) 
{
	// LLONG_MIN  = -9223372036854775808
	// LLONG_MAX  = +9223372036854775807
	// BIN(9223372036854775808) = 11010001101101110001011101011000111000100001100101

	// sanity check on len
	if(len > 64) 
		return 0;
	int64_t value = 0;
	for(uint32 i = 0; i < len; ++i) {
		int c = s[i];
		value = (value << 1) + (c - '0');
	}
	return value;
}
/*
        if(!(condition)) { \
            slfprintf_stderr("[%s:%d] Assert failed in %s(): %s\n", __FILE__, __LINE__, __func__, message); \
            abort(); \
        } \
*/
void Gravity_Implement_DebugAssert(const char * pFile, int line, const char * pFunc, const char * pMessage)
{
    slfprintf_stderr("[%s:%d] Assert failed in %s(): %s\n", pFile, line, pFunc, pMessage);
    abort();
}
