// uncrustify-internal.h
//
#ifndef __UNCRUSTIFY_INTERNAL_H
#define __UNCRUSTIFY_INTERNAL_H

#define SLIB_INCLUDE_CPPSTDLIBS
#include <SLIB.H>
//#include <deque>
//#include <string>
//#include <vector>
//#include <type_traits>
//#include <cstdio>      // FILE
//#include <cstring>     // memset()
//#include <stdexcept>            // to get std::logic_error
#include <bitset>
//#include <cerrno>
//#include <fcntl.h>
//#include <map>
#ifdef HAVE_UNISTD_H
	#include <unistd.h>
#endif
#ifdef HAVE_SYS_STAT_H
	#include <sys/stat.h>
#endif
#ifdef HAVE_STRINGS_H
	#include <strings.h>    // provides strcasecmp()
#endif
#ifdef HAVE_UTIME_H
	#include <time.h>
#endif

#include "options.h"
#include "enum_flags.h"
#include "base_types.h"
#include "log_levels.h"
#include "logmask.h"
#include "logger.h"
#include "pcf_flags.h"
#include "token_enum.h"    // E_Token
#include "unc_text.h"
#include "uncrustify_limits.h"
//#include <assert.h>
//#include <cstddef>      // do get the definition of size_t
#ifdef HAVE_UTIME_H
	#include <utime.h>
#endif
#include "uncrustify_types.h"
// necessary to not sort it
#include "char_table.h"
#include "language_tools.h"
#include "chunk.h"
#include "uncrustify.h"
#include "log_rules.h"
#include "align.h"
#include "align_asm_colon.h"
#include "align_assign.h"
#include "align_braced_init_list.h"
#include "align_eigen_comma_init.h"
#include "align_func_params.h"
#include "align_func_proto.h"
#include "align_init_brace.h"
#include "align_left_shift.h"
#include "align_oc_decl_colon.h"
#include "align_oc_msg_colons.h"
#include "align_oc_msg_spec.h"
#include "align_preprocessor.h"
#include "align_same_func_call_params.h"
#include "align_stack.h"
#include "align_struct_initializers.h"
#include "align_trailing_comments.h"
#include "align_typedefs.h"
#include "align_var_def_brace.h"
#include "align_add.h"
#include "quick_align_again.h"
#include "indent.h"
#include "align_tools.h"
#include "align_log_al.h"
#include "align_tab_column.h"
#include "align_nl_cont.h"
#include "space.h"
#include "unc_tools.h" // to get stackID and get_A_Number()
#include "prototypes.h"
#include "args.h"
#include "unc_ctype.h"
#include "backup.h"
#include "brace_cleanup.h"
#include "flag_parens.h"
#include "keywords.h"
#include "lang_pawn.h"
#include "parsing_frame_stack.h"
#include "braces.h"
#include "calculate_closing_brace_position.h"
#include "combine_tools.h"
#include "newlines.h"
#include "change_int_types.h"
#include "width.h"
#include "universalindentgui.h"
#include "error_types.h"
#include "generated/uncrustify_version.h"
#include "unicode.h"
#include "combine.h"
#include "compat.h"
#include "detect.h"
#include "enum_cleanup.h"
#include "mark_question_colon.h"
#include "output.h"
#include "parameter_pack_cleanup.h"
#include "parens.h"
#include "parent_for_pp.h"
#include "remove_duplicate_include.h"
#include "remove_extra_returns.h"
#include "rewrite_infinite_loops.h"
#include "semicolons.h"
#include "sorting.h"
#include "tokenize.h"
#include "tokenize_cleanup.h"
#include "check_template.h"
#include "combine_skip.h"
#include "flag_braced_init_list.h"
#include "flag_decltype.h"
#include "punctuators.h"

#endif // __UNCRUSTIFY_INTERNAL_H
