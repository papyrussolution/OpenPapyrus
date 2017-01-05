#ifndef __CODE_WRITER_H__
#define __CODE_WRITER_H__


#include "CallList.h"

void codeWriteStubHeader(struct CallList *cl, const char* filename);
void codeWriteStubSource(struct CallList *cl, const char* filename);


#endif

