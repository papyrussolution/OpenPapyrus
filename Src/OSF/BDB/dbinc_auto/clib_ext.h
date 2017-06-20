/* DO NOT EDIT: automatically built by dist/s_include. */
#ifndef	_clib_ext_h_
#define	_clib_ext_h_

#if defined(__cplusplus)
extern "C" {
#endif

#ifndef HAVE_ATOI
	int atoi(const char *);
#endif
#ifndef HAVE_ATOL
	long atol(const char *);
#endif
#ifndef HAVE_BSEARCH
	void *bsearch(const void *, const void *, size_t, size_t, int (*)(const void *, const void *));
#endif
#ifndef HAVE_GETCWD
	char *getcwd(char *, size_t);
#endif
#ifndef HAVE_GETOPT
	int getopt(int, char * const *, const char *);
#endif
#ifndef HAVE_ISALPHA
	int isalpha(int);
#endif
#ifndef HAVE_ISDIGIT
	int isdigit(int);
#endif
#ifndef HAVE_ISPRINT
	int isprint(int);
#endif
#ifndef HAVE_ISSPACE
	int isspace(int);
#endif
#ifndef HAVE_MEMCMP
	int memcmp(const void *, const void *, size_t);
#endif
#ifndef HAVE_MEMCPY
	void *memcpy(void *, const void *, size_t);
#endif
#ifndef HAVE_MEMMOVE
	void *memmove(void *, const void *, size_t);
#endif
#ifndef HAVE_PRINTF
	int printf(const char *, ...);
#endif
#ifndef HAVE_PRINTF
	int fprintf(FILE *, const char *, ...);
#endif
#ifndef HAVE_PRINTF
	int vfprintf(FILE *, const char *, va_list);
#endif
#ifndef HAVE_QSORT
	void qsort(void *, size_t, size_t, int(*)(const void *, const void *));
#endif
#ifndef HAVE_RAISE
	int raise(int);
#endif
#ifndef HAVE_RAND
	int rand();
	void srand(unsigned int);
#endif
#ifndef HAVE_SNPRINTF
	int snprintf(char *, size_t, const char *, ...);
#endif
#ifndef HAVE_VSNPRINTF
	int vsnprintf(char *, size_t, const char *, va_list);
#endif
#ifndef HAVE_STRCASECMP
	int strcasecmp(const char *, const char *);
#endif
#ifndef HAVE_STRCASECMP
	int strncasecmp(const char *, const char *, size_t);
#endif
#ifndef HAVE_STRCAT
	char *strcat(char *, const char *);
#endif
#ifndef HAVE_STRCHR
	char *strchr(const char *,  int);
#endif
#ifndef HAVE_STRDUP
	char *strdup(const char *);
#endif
#ifndef HAVE_STRERROR
	char *strerror(int);
#endif
#ifndef HAVE_STRNCAT
	char *strncat(char *, const char *, size_t);
#endif
#ifndef HAVE_STRNCMP
	int strncmp(const char *, const char *, size_t);
#endif
#ifndef HAVE_STRRCHR
	char *strrchr(const char *, int);
#endif
#ifndef HAVE_STRSEP
	char *strsep(char **, const char *);
#endif
#ifndef HAVE_STRTOL
	long strtol(const char *, char **, int);
#endif
#ifndef HAVE_STRTOUL
	ulong strtoul(const char *, char **, int);
#endif
#ifndef HAVE_TIME
	__time64_t time(__time64_t *);
#endif

#if defined(__cplusplus)
}
#endif
#endif /* !_clib_ext_h_ */
