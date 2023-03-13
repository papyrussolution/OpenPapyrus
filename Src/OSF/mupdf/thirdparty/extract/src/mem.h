#ifndef EXTRACT_MEM_H
#define EXTRACT_MEM_H

#include <stdarg.h>
#include <string.h>

void extract_bzero(void *b, size_t len);

int extract_vasprintf(char** out, const char* format, va_list va);
int extract_asprintf(char** out, const char* format, ...);

int extract_strdup(const char* s, char** o_out);

#endif
