#ifndef VDOS_SUPPORT_H
#define VDOS_SUPPORT_H

#include <string.h>
#include <string>
#include <ctype.h>
#include "vDos.h"

char *lTrim(char *str);
char *rTrim(char *str);
char *rSpTrim(char *str);
char *lrTrim(char * str);
void upcase(char * str);

extern Bit16u cpMap[];

int Unicode2Ascii(Bit16u *unicode, Bit8u *ascii, int length);

#endif
