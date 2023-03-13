#ifndef FITZ_Z_IMP_H
#define FITZ_Z_IMP_H

#include <zlib.h>

void *fz_zlib_alloc(void *ctx, uint items, uint size);
void fz_zlib_free(void *ctx, void *ptr);

#endif
