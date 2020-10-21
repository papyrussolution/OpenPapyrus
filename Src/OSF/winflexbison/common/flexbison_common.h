// FLEXBISON_COMMON.H
//
#ifndef __FLEXBISON_COMMON_H
#define __FLEXBISON_COMMON_H

#include <slib.h>
#include <io.h>
#include <stdbool.h>
#include <wctype.h>
#include <locale.h>
#include <process.h>
#if (defined _WIN32 || defined __WIN32__) && !defined __CYGWIN__
	#include <direct.h>
#endif
#if !_LIBC
	#include <flexbison_common_config.h>
#endif
#include "memchr2.h"
#include "xmemdup0.h"
#include "xstrndup.h"
#include "c-stack.h"
#include "xalloc.h"
#include "get-errno.h"
#include "error.h"
#include "xsize.h"
#include "m4.h"
#include <getopt.h>
#include "localcharset.h"
#include "gettext.h"
#include "dirname.h"
#include "filenamecat.h"
#include "tempname.h"
#include "tmpdir.h"
#include "xprintf.h"
#include "xvasprintf.h"
#include "vasnprintf.h"
#include "close-stream.h"
#include "quote.h"
#include "verify.h"
#include "argmatch.h"
#include "quotearg.h"
#include "obstack.h"
#include "relocatable.h"
#include "exitfail.h"
#include "unicodeio.h"
#include "cloexec.h"
#include "closein.h"
#include "clean-temp.h"

#endif // __FLEXBISON_COMMON_H
