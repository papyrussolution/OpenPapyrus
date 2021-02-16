// oniggnu.h - Oniguruma (regular expression library)
// Copyright (c) 2002-2019  K.Kosako  All rights reserved.
//
#ifndef ONIGGNU_H
#define ONIGGNU_H

#include "oniguruma.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
	RE_MBCTYPE_ASCII = 0,
	RE_MBCTYPE_EUC   = 1,
	RE_MBCTYPE_SJIS  = 2,
	RE_MBCTYPE_UTF8  = 3
};

// GNU regex options 
#ifndef RE_NREGS
	#define RE_NREGS                ONIG_NREGION
#endif

#define RE_OPTION_IGNORECASE         ONIG_OPTION_IGNORECASE
#define RE_OPTION_EXTENDED           ONIG_OPTION_EXTEND
#define RE_OPTION_MULTILINE          ONIG_OPTION_MULTILINE
#define RE_OPTION_SINGLELINE         ONIG_OPTION_SINGLELINE
#define RE_OPTION_LONGEST            ONIG_OPTION_FIND_LONGEST
#define RE_OPTION_POSIXLINE         (RE_OPTION_MULTILINE|RE_OPTION_SINGLELINE)
#define RE_OPTION_FIND_NOT_EMPTY     ONIG_OPTION_FIND_NOT_EMPTY
#define RE_OPTION_NEGATE_SINGLELINE  ONIG_OPTION_NEGATE_SINGLELINE
#define RE_OPTION_DONT_CAPTURE_GROUP ONIG_OPTION_DONT_CAPTURE_GROUP
#define RE_OPTION_CAPTURE_GROUP      ONIG_OPTION_CAPTURE_GROUP

ONIG_EXTERN void re_mbcinit(int);
ONIG_EXTERN int  re_compile_pattern(const char*, int, struct re_pattern_buffer*, char* err_buf);
ONIG_EXTERN int  re_recompile_pattern(const char*, int, struct re_pattern_buffer*, char* err_buf);
ONIG_EXTERN void re_free_pattern(struct re_pattern_buffer*);
ONIG_EXTERN int  re_adjust_startpos(struct re_pattern_buffer*, const char*, int, int, int);
ONIG_EXTERN int  re_search(struct re_pattern_buffer*, const char*, int, int, int, struct re_registers*);
ONIG_EXTERN int  re_match(struct re_pattern_buffer*, const char *, int, int, struct re_registers*);
ONIG_EXTERN void re_set_casetable(const char*);
ONIG_EXTERN void re_free_registers(struct re_registers*);
ONIG_EXTERN int  re_alloc_pattern(struct re_pattern_buffer**);  /* added */

#ifdef __cplusplus
}
#endif

#endif /* ONIGGNU_H */
