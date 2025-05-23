//
//
#include "mupdf/fitz.h"
#pragma hdrstop

#ifdef _WIN32

#ifdef _MSC_VER
#ifndef _WINRT
#ifndef SLIBINCLUDED // @sobolev

//#define DELTA_EPOCH_IN_MICROSECS 11644473600000000Ui64

int gettimeofday(struct timeval * tv, struct timezone * tz)
{
	FILETIME ft;
	unsigned __int64 tmpres = 0;
	if(tv) {
		GetSystemTimeAsFileTime(&ft);
		tmpres |= ft.dwHighDateTime;
		tmpres <<= 32;
		tmpres |= ft.dwLowDateTime;
		tmpres /= 10; /*convert into microseconds*/
		/*converting file time to unix epoch*/
		tmpres -= SlConst::Epoch1600_1970_Offs_Mks;;
		tv->tv_sec = (long)(tmpres / 1000000UL);
		tv->tv_usec = (long)(tmpres % 1000000UL);
	}
	return 0;
}
#endif SLIBINCLUDED
#endif /* !_WINRT */
#endif /* _MSC_VER */

char * fz_utf8_from_wchar(const wchar_t * s)
{
	const wchar_t * src = s;
	char * d;
	char * dst;
	int len = 1;
	while(*src) {
		len += fz_runelen(*src++);
	}
	d = (char *)Memento_label(SAlloc::M(len), "utf8_from_wchar");
	if(d) {
		dst = d;
		src = s;
		while(*src) {
			dst += fz_runetochar(dst, *src++);
		}
		*dst = 0;
	}
	return d;
}

wchar_t * fz_wchar_from_utf8(const char * s)
{
	wchar_t * d, * r;
	int c;
	r = d = (wchar_t *)SAlloc::M((strlen(s) + 1) * sizeof(wchar_t));
	if(!r)
		return NULL;
	while(*s) {
		s += fz_chartorune(&c, s);
		/* Truncating c to a wchar_t can be problematic if c
		 * is 0x10000. */
		if(c >= 0x10000)
			c = FZ_REPLACEMENT_CHARACTER;
		*d++ = c;
	}
	*d = 0;
	return r;
}

void * fz_fopen_utf8(const char * name, const char * mode)
{
	wchar_t * wmode;
	FILE * file = NULL;
	wchar_t * wname = fz_wchar_from_utf8(name);
	if(wname) {
		wmode = fz_wchar_from_utf8(mode);
		if(wmode == NULL) {
			SAlloc::F(wname);
			return NULL;
		}
		file = _wfopen(wname, wmode);
		SAlloc::F(wname);
		SAlloc::F(wmode);
	}
	return file;
}

int fz_remove_utf8(const char * name)
{
	int n;
	wchar_t * wname = fz_wchar_from_utf8(name);
	if(wname == NULL) {
		errno = ENOMEM;
		return -1;
	}
	n = _wremove(wname);
	SAlloc::F(wname);
	return n;
}

char ** fz_argv_from_wargv(int argc, const wchar_t ** wargv)
{
	int i;
	char ** argv = (char **)Memento_label(SAlloc::C(argc, sizeof(char *)), "fz_argv");
	if(argv == NULL) {
		slfprintf_stderr("Out of memory while processing command line args!\n");
		exit(1);
	}
	for(i = 0; i < argc; i++) {
		argv[i] = Memento_label(fz_utf8_from_wchar(wargv[i]), "fz_arg");
		if(argv[i] == NULL) {
			slfprintf_stderr("Out of memory while processing command line args!\n");
			exit(1);
		}
	}
	return argv;
}

void fz_free_argv(int argc, char ** argv)
{
	for(int i = 0; i < argc; i++)
		SAlloc::F(argv[i]);
	SAlloc::F(argv);
}

#else

int fz_time_dummy;

#endif /* _WIN32 */
