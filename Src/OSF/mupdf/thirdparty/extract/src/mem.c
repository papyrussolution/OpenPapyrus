#include "alloc.h"
#include "mem.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>


void extract_bzero(void *b, size_t len)
{
    memset(b, 0, len);
}

int extract_vasprintf(char** out, const char* format, va_list va)
{
    int n;
    int n2;
    va_list va2;
    va_copy(va2, va);
    n = vsnprintf(NULL, 0, format, va);
    if (n < 0) return n;
    if (extract_malloc(out, n + 1)) return -1;
    n2 = vsnprintf(*out, n + 1, format, va2);
    va_end(va2);
    assert(n2 == n);
    return n2;
}


int extract_asprintf(char** out, const char* format, ...)
{
    va_list va;
    int     ret;
    va_start(va, format);
    ret = extract_vasprintf(out, format, va);
    va_end(va);
    return ret;
}

int extract_strdup(const char* s, char** o_out)
{
    size_t l = strlen(s) + 1;
    if (extract_malloc(o_out, l)) return -1;
    memcpy(*o_out, s, l);
    return 0;
}
