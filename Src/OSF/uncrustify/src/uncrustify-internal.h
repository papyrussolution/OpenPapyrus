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
//#include <regex>
//#include <unordered_map>
//#include <ctime>
//#include <set>
//#include <cstdlib>
//#include <fstream>
//#include <cctype>  // to get std::tolower
//#include <cstdarg> // to get va_start, va_end
//#include <limits>
//#include <cassert>
//#include <deque>
//#include <string>
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
#include "enum_flags.h"
#include "option.h"
#include "generated/uncrustify_version.h"
#include "generated/option_enum.h"
#include "options.h"
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
//#include "align.h"
void align_all();
//
//#include "align_asm_colon.h"
// 
// Aligns asm declarations on the colon asm volatile (
//    "xxx"
//    : "x"(h),
//      "y"(l),
//    : "z"(h)
//    );
// 
void align_asm_colon();
//
//#include "align_assign.h"
// 
// Aligns all assignment operators on the same level as first, starting with first.
// For variable definitions, only consider the '=' for the first variable.
// Otherwise, only look at the first '=' on the line.
// @param first  chunk pointing to the first assignment
// 
Chunk * align_assign(Chunk *first, size_t span, size_t thresh, size_t *p_nl_count);
//
//#include "align_braced_init_list.h"
// 
// Aligns all braced init list operators on the same level as first, starting with first.
// @param first  chunk pointing to the first braced init list
//
Chunk * align_braced_init_list(Chunk *first, size_t span, size_t thresh, size_t *p_nl_count);
//
//#include "align_eigen_comma_init.h"
//
// Descr: Align comma-separated expressions following left shift operator '<<'
//
void align_eigen_comma_init();
//
//#include "align_func_params.h"
void   align_func_params();
Chunk * align_func_param(Chunk *start);
//
//#include "align_func_proto.h"
//
// Descr: Aligns all function prototypes in the file.
//
void align_func_proto(size_t span);
//
//#include "align_init_brace.h"
// 
// Generically aligns on '=', '{', '(' and item after ','
// It scans the first line and picks up the location of those tags.
// It then scans subsequent lines and adjusts the column.
// Finally it does a second pass to align everything.
// 
// Aligns all the '=' signs in structure assignments.
// a = {
//    .a    = 1;
//    .type = fast;
// };
// 
// And aligns on '{', numbers, strings, words.
// colors[] = {
//    {"red",   {255, 0,   0}}, {"blue",   {  0, 255, 0}},
//    {"green", {  0, 0, 255}}, {"purple", {255, 255, 0}},
// };
// 
// For the C99 indexed array assignment, the leading []= is skipped (no aligning)
// struct foo_t bars[] =
// {
//    [0] = { .name = "bar",
//            .age  = 21 },
//    [1] = { .name = "barley",
//            .age  = 55 },
// };
// 
// NOTE: this assumes that spacing is at the minimum correct spacing (ie force)
//       if it isn't, some extra spaces will be inserted.
// 
// @param start   Points to the open brace chunk
// 
void align_init_brace(Chunk *start);
//
//#include "align_left_shift.h"
//
// Descr: Align left shift operators '<<' (CT_SHIFT)
//
void align_left_shift();
//
//#include "align_oc_decl_colon.h"
// 
// Descr: Aligns OC declarations on the colon
//   -(void) doSomething: (NSString*) param1 with: (NSString*) param2
// 
void align_oc_decl_colon();
//
//#include "align_oc_msg_colons.h"
//
// Descr: Aligns OC messages
//
void align_oc_msg_colons();
//
//#include "align_oc_msg_spec.h"
//
// Descr: Aligns all function prototypes in the file.
//
void align_oc_msg_spec(size_t span);
//
//#include "align_preprocessor.h"
//
// Descr: Scans the whole file for #defines. Aligns all within X lines of each other
//
void align_preprocessor();
//
//#include "ChunkStack.h"
class ChunkStack {
public:
	struct Entry {
		Entry() : m_seqnum(0), m_pc(Chunk::NullChunkPtr)
		{
		}
		Entry(const Entry &ref) : m_seqnum(ref.m_seqnum), m_pc(ref.m_pc)
		{
		}
		Entry(size_t sn, Chunk *pc) : m_seqnum(sn), m_pc(pc)
		{
		}
		size_t m_seqnum;
		Chunk  * m_pc;
	};
protected:
	std::deque<Entry> m_cse;
	size_t m_seqnum;       //! current sequence number
public:
	ChunkStack() : m_seqnum(0)
	{
	}
	ChunkStack(const ChunkStack &cs)
	{
		Set(cs);
	}
	virtual ~ChunkStack()
	{
	}
	void Set(const ChunkStack &cs);
	void Push_Back(Chunk * pc)
	{
		Push_Back(pc, ++m_seqnum);
	}
	bool Empty() const { return (m_cse.empty()); }
	size_t Len() const { return (m_cse.size()); }
	const Entry *Top() const;
	const Entry *Get(size_t idx) const;
	Chunk *GetChunk(size_t idx) const;
	Chunk *Pop_Back();
	void Push_Back(Chunk * pc, size_t seqnum);
	Chunk *Pop_Front();
	void Reset()
	{
		m_cse.clear();
	}
	/**
	 * Mark an entry to be removed by Collapse()
	 * @param idx  The item to remove
	 */
	void Zap(size_t idx);
	//! Compresses down the stack by removing dead entries
	void Collapse();
};
//
//#include "align_same_func_call_params.h"
void align_params(Chunk * start, std::deque<Chunk *> &chunks);
void align_same_func_call_params();
//
//#include "align_stack.h"
class AlignStack {
public:
	enum StarStyle {
		SS_IGNORE, //! don't look for prev stars
		SS_INCLUDE, //! include prev * before add
		SS_DANGLE //! include prev * after add
	};

	ChunkStack m_aligned; //! contains the tokens that are aligned
	ChunkStack m_skipped; //! contains the tokens sent to Add()
	size_t m_max_col;
	size_t m_min_col;
	size_t m_span;
	size_t m_thresh;
	size_t m_seqnum;
	size_t m_nl_seqnum;
	size_t m_gap;
	bool m_right_align;
	bool m_absolute_thresh;
	StarStyle m_star_style;
	StarStyle m_amp_style;
	bool m_skip_first;  //! do not include the first item if it causes it to be indented
	size_t stackID;     //! for debugging purposes only

	AlignStack() : m_max_col(0), m_min_col(0), m_span(0), m_thresh(0), m_seqnum(0), m_nl_seqnum(0), m_gap(0), m_right_align(false), m_absolute_thresh(false), 
		m_star_style(SS_IGNORE), m_amp_style(SS_IGNORE), m_skip_first(false), stackID(std::numeric_limits<std::size_t>::max()) // under linux 64 bits: 18446744073709551615
		, m_last_added(0)
	{
	}
	AlignStack(const AlignStack &ref) : m_aligned(ref.m_aligned), m_skipped(ref.m_skipped), m_max_col(ref.m_max_col), m_min_col(ref.m_min_col), m_span(ref.m_span), 
		m_thresh(ref.m_thresh), m_seqnum(ref.m_seqnum), m_nl_seqnum(ref.m_nl_seqnum), m_gap(ref.m_gap), m_right_align(ref.m_right_align), m_absolute_thresh(ref.m_absolute_thresh), 
		m_star_style(ref.m_star_style), m_amp_style(ref.m_amp_style), m_skip_first(ref.m_skip_first), m_last_added(ref.m_last_added)
	{
	}
	~AlignStack()
	{
	}
	/**
	 * Resets the two ChunkLists and zeroes local vars.
	 *
	 * @param span       The row span limit
	 * @param threshold  The column threshold
	 */
	void Start(size_t span, int threshold = 0);
	/**
	 * Adds an entry to the appropriate stack.
	 *
	 * @param pc      the chunk
	 * @param seqnum  optional sequence number (0=assign one)
	 */
	void Add(Chunk * pc, size_t seqnum = 0);
	//! Adds some newline and calls Flush() if needed
	void NewLines(size_t cnt);
	/**
	 * Aligns all the stuff in m_aligned.
	 * Re-adds 'newer' items in m_skipped.
	 */
	void Flush();
	//! Resets the stack, discarding anything that was previously added
	void Reset();
	//! Aligns everything else and resets the lists.
	void End();
	//! the size of the lists.
	size_t Len();
	//! for debugging purpose only
	void Debug();
	const char * get_StarStyle_name(StarStyle star_style);
protected:
	size_t m_last_added; //! 0=none, 1=aligned, 2=skipped
	ChunkStack m_scratch; //! used in ReAddSkipped()
	//! Calls Add on all the skipped items
	void ReAddSkipped();
};

#define WITH_STACKID_DEBUG                                                                                  \
	if(stackID == std::numeric_limits<std::size_t>::max()) {                                                \
		fprintf(stderr, "AlignStack::%s(%d): the stack is not ready, Start is missed\n", __func__, __LINE__); \
		log_flush(true);                                                                                      \
		exit(EX_SOFTWARE);                                                                                    \
	}                                                                                                        \
	else {                                                                                                   \
		LOG_FMT(LAS, "AlignStack::%s(%d): stackID is %zu\n", __func__, __LINE__, stackID);                    \
	}
//
//#include "align_struct_initializers.h"
//
// Descr: Aligns stuff inside a multi-line "= { ... }" sequence.
//
void align_struct_initializers();
//
//#include "align_trailing_comments.h"
enum class comment_align_e : unsigned int {
	REGULAR,
	BRACE,
	ENDIF,
};
// 
// For a series of lines ending in a comment, align them. The series ends when more than align_right_cmt_span newlines are found.
// 
// Interesting info:
//  - least physically allowed column
//  - intended column
//  - least original cmt column
// 
// min_col is the minimum allowed column (based on prev token col/size) cmt_col less than
// @param start   Start point
// @return        pointer the last item looked at
// 
Chunk *align_trailing_comments(Chunk * start);
comment_align_e get_comment_align_type(Chunk * cmt);
void align_stack(ChunkStack &cs, size_t col, bool align_single, log_sev_t sev);
void align_right_comments();
//
//#include "align_typedefs.h"
// 
// Aligns simple typedefs that are contained on a single line each.
// This should be called after the typedef target is marked as a type.
// 
// typedef int        foo_t;
// typedef char       bar_t;
// typedef const char cc_t;
// 
void align_typedefs(size_t span);
//
//#include "align_var_def_brace.h"
// 
// Descr: Scan everything at the current level until the close brace and find the
//   variable def align column.  Also aligns bit-colons, but that assumes that
//   bit-types are the same! But that should always be the case...
// 
Chunk *align_var_def_brace(Chunk *pc, size_t span, size_t *nl_count);
//
//#include "align_add.h"
void align_add(ChunkStack &cs, Chunk *pc, size_t &max_col);
//
//#include "quick_align_again.h"
void quick_align_again();
//
//#include "indent.h"
// 
// Change the top-level indentation only by changing the column member in the chunk structures. The level indicator must
// already be set.
// 
void indent_text();
// 
// Indent the preprocessor stuff from column 1. FIXME: This is broken if there is a comment or escaped newline between
// '#' and 'define'.
// 
void indent_preproc();
// 
// @param pc      chunk at the start of the line
// @param column  desired column
// 
void indent_to_column(Chunk * pc, size_t column);
// 
// Same as indent_to_column, except we can move both ways
// @param pc      chunk at the start of the line
// @param column  desired column
// 
void align_to_column(Chunk * pc, size_t column);
// Scan to see if the whole file is covered by one #ifdef
bool ifdef_over_whole_file();
// 
// Changes the initial indent for a line to the given column
// @param pc      The chunk at the start of the line
// @param column  The desired column
// 
void reindent_line(Chunk * pc, size_t column);
//
//#include "align_tools.h"
// 
// @brief return the chunk the follows after a C array
// 
// The provided chunk is considered an array if it is an opening square (CT_SQUARE_OPEN) and the matching close is followed by an equal sign '='
// 
// Example:                  array[25] = 12;
//                               /|\     /|\
//                                |       |
// provided chunk has to point to [       |
// returned chunk points to              12
// 
// @param chunk  chunk to operate on
// @return the chunk after the '=' if the check succeeds
// @return Chunk::NullChunkPtr in all other cases
// 
Chunk * skip_c99_array(Chunk * sq_open);
// 
// Scans a line for stuff to align on.
// 
// We trigger on BRACE_OPEN, FPAREN_OPEN, ASSIGN, and COMMA. We want to align the NEXT item.
// 
Chunk * scan_ib_line(Chunk * start);
void   ib_shift_out(size_t idx, size_t num);
Chunk * step_back_over_member(Chunk * pc);
//
//#include "align_log_al.h"
void align_log_al(log_sev_t sev, size_t line);
//
//#include "align_tab_column.h"
// 
// Advances to the next tab stop if not currently on one.
// @param col  The current column
// @return the next tabstop column
// 
size_t align_tab_column(size_t col);
//
//#include "align_nl_cont.h"
// 
// For a series of lines ending in backslash-newline, align them.
// The series ends when a newline or multi-line C comment is encountered.
// @param start   Start point
// @return pointer the last item looked at (null chunk/newline/comment)
// 
Chunk *align_nl_cont(Chunk *start);
// 
// Aligns all backslash-newline combos in the file.
// This should be done LAST.
// 
void align_backslash_newline();
//
#include "space.h"
#include "prototypes.h"
#include "unc_tools.h" // to get stackID and get_A_Number()
#include "args.h"
//#include "unc_ctype.h"
//! Test anything EOF (-1) to 0-255
int unc_fix_ctype(int ch);
//! check if a character is a space
int unc_isspace(int ch);
//! check if a character is a printing character
int unc_isprint(int ch);
//! check if a character is an alphabetic character (a letter).
int unc_isalpha(int ch);
//! check if a character is an alphanumeric character.
int unc_isalnum(int ch);
//! convert a character to upper case
int unc_toupper(int ch);
//! convert a character to lower case
int unc_tolower(int ch);
//! check if a character is a hexadecimal digit
int unc_isxdigit(int ch);
//! check if a character is a decimal digit
int unc_isdigit(int ch);
//! check if a character is upper case
int unc_isupper(int ch);
//
#include "backup.h"
#include "parsing_frame.h" // necessary to not sort
//#include "brace_cleanup.h"
// 
// Scans through the whole list and does stuff.
// It has to do some tricks to parse preprocessors.
// 
void brace_cleanup();
//
//#include "flag_parens.h"
// 
// Flags everything from the open paren to the close paren.
// 
// @param po          Pointer to the open parenthesis
// @param flags       flags to add
// @param opentype
// @param parenttype
// @param parent_all
// @return The token after the close paren
// 
Chunk * flag_parens(Chunk * po, PcfFlags flags, E_Token opentype, E_Token parenttype, bool parent_all);
//
#include "keywords.h"
#include "lang_pawn.h"
#include "parsing_frame_stack.h"
#include "braces.h"
//#include "calculate_closing_brace_position.h"
Chunk * calculate_closing_brace_position(const Chunk * cl_colon, Chunk * pc);
//
#include "combine_tools.h"
#include "newlines.h"
//#include "change_int_types.h"
// 
// Descr: Add or remove redundant 'int' keyword of integer types
// 
void change_int_types();
//
//#include "width.h"
// 
// Step forward until a token goes beyond the limit and then call split_line() to split the line at or before that
// point.
// 
void do_code_width();
//
//#include "universalindentgui.h"
void print_universal_indent_cfg(FILE * pfile);
//
#include "error_types.h"
//#include "unicode.h"
void write_bom();
// 
// @param ch the 31-bit char value
// 
void write_char(int ch);
void write_string(const UncText &text);
// Figure out the encoding and convert to an int sequence
bool decode_unicode(const std::vector<UINT8> &in_data, std::deque<int> &out_data, char_encoding_e &enc, bool &has_bom);
void encode_utf8(int ch, std::vector<UINT8> &res);
//
#include "combine.h"
//#include "compat.h"
bool unc_getenv(const char * name, std::string &str);
bool unc_homedir(std::string &home);
// 
// Descr: even if we prefer the format %zu, we have to change to %lu to be runable under Windows
// 
void convert_log_zu2lu(char * buf);
//
//#include "detect.h"
//
// Call all the detect_xxxx() functions
//
void detect_options();
//
//#include "enum_cleanup.h"
// 
// Descr: Scans through the whole list and does stuff.
// works on the last comma within enum
// 
void enum_cleanup();
//
//#include "mark_question_colon.h"
void mark_question_colon();
//
#include "output.h"
//#include "parameter_pack_cleanup.h"
void parameter_pack_cleanup();
//
//#include "parens.h"
//
// add parenthesis
//
void do_parens();
void do_parens_assign();
void do_parens_return();
//
//#include "parent_for_pp.h"
//
// Descr: mark the parent
//
void do_parent_for_pp();
//
//#include "remove_duplicate_include.h"
// Descr: Remove duplicate include
// 
void remove_duplicate_include();
//
//#include "remove_extra_returns.h"
// 
// Descr: Remove unnecessary returns that is remove 'return;' that appears as the last statement in a function
// 
void remove_extra_returns();
//
//#include "rewrite_infinite_loops.h"
// 
// Descr: Rewrite infinite loops in a consistent syntax
// 
void rewrite_infinite_loops();
//
//#include "semicolons.h"
// 
// Removes superfluous semicolons:
//  - after brace close whose parent is IF, ELSE, SWITCH, WHILE, FOR, NAMESPACE
//  - after another semicolon where parent is not FOR
//  - (D) after brace close whose parent is ENUM/STRUCT/UNION
//  - (Java) after brace close whose parent is SYNCHRONIZED
//  - after an open brace
//  - when not in a #DEFINE
// 
void remove_extra_semicolons();
//
//#include "sorting.h"
// 
// alphabetically sort the #include or #import statements of a file
// 
// @todo better use a chunk pointer parameter instead of a global variable
// 
void sort_imports();
//
#include "tokenize.h"
//#include "tokenize_cleanup.h"
// 
// @brief clean up tokens
// 
// Change certain token types based on simple sequence. Example: change '[' + ']' to '[]'
// Note that level info is not yet available, so it is OK to do all processing that doesn't need to know level info.
// (that's very little!)
// 
void tokenize_cleanup();
void tokenize_trailing_return_types();
void split_off_angle_close(Chunk * pc);
//
#include "check_template.h"
#include "combine_skip.h"
//#include "flag_braced_init_list.h"
// 
// Descr: Detect a cpp braced init list
// 
bool detect_cpp_braced_init_list(Chunk * pc, Chunk * next);
// 
// Descr: Flags the opening and closing braces of an expression deemed to be a cpp braced initializer list; a call to
// detect_cpp_braced_init_list() should first be made prior to calling this function
// 
void flag_cpp_braced_init_list(Chunk * pc, Chunk * next);
//
//#include "flag_decltype.h"
// 
// Descr: Flags all chunks within a cpp decltype expression from the opening brace to the closing brace
// @return Returns true if expression is a valid decltype expression
// 
bool flag_cpp_decltype(Chunk * pc);
//
#include "punctuators.h"
#include "add_space_table.h"
//#include "options_for_QT.h"
// TODO can we avoid those extern variables?
extern bool QT_SIGNAL_SLOT_found;
extern size_t QT_SIGNAL_SLOT_level;
extern bool restoreValues;
void save_set_options_for_QT(size_t level);
void restore_options_for_QT();
//
//#include "pragma_cleanup.h"
// cleanup the pagma line(s)
void pragma_cleanup();
//
#include "ListManager.h"
#include "combine_fix_mark.h"
//#include "EnumStructUnionParser.h"
// 
// Class EnumStructUnionParser : This class facilitates the parsing and interpretation of ALL instances of the class,
// enum, union, and struct keywords, including user-defined types with a body {} and any trailing inline variable
// declarations that may follow the definition (as permitted by the coding language in question). The class also
// interprets variable declarations preceded by one of those keywords, as well as any C/C++ forward declarations
// 
class EnumStructUnionParser {
public:
	EnumStructUnionParser();
	~EnumStructUnionParser();
private:
	/**
	 * Analyzes all identifiers (marked as CT_WORD) between the starting and ending chunks and changes CT_WORD to
	 *one of CT_TYPE, CT_MACRO_FUNC_CALL, etc. and sets flags (PCF_VAR_1ST, PCF_VAR_1ST_DEF, PCF_VAR_INLINE, etc.)
	 *for variable identifiers accordingly. Flags C++ forward declarations as PCF_INCOMPLETE
	 */
	void analyze_identifiers();
	/**
	 * Returns true if a pair of braces were both detected AND determined to be part of a class/enum/struct/union
	 *body
	 */
	bool body_detected() const;
	/**
	 * Returns true if comma-separated values were detected during parsing
	 */
	bool comma_separated_values_detected() const;
	/**
	 * Returns true if an enumerated integral type was detected during parsing
	 */
	bool enum_base_detected() const;
	/**
	 * Returns the end chunk of a class/enum/struct/union body, if detected during parsing
	 */
	Chunk *get_body_end() const;
	/**
	 * Returns the starting chunk of a class/enum/struct/union body, if detected during parsing
	 */
	Chunk *get_body_start() const;
	/**
	 * Returns the starting chunk associated with an enumerated type's base specifier statement, if detected during
	 *parsing
	 */
	Chunk *get_enum_base_start() const;
	/**
	 * Returns the first comma encountered at the level of the starting chunk, if detected during parsing
	 */
	Chunk *get_first_top_level_comma() const;
	/**
	 * Returns the ending chunk associated with an class/struct inheritance list, if detected during parsing
	 */
	Chunk *get_inheritance_end() const;
	/**
	 * Returns the starting chunk associated with an class/struct inheritance list, if detected during parsing
	 */
	Chunk *get_inheritance_start() const;
	/**
	 * Returns a numerically-indexed map of all question operators encountered during parsing
	 */
	std::map<std::size_t, Chunk *> get_question_operators() const;
	/**
	 * Returns the end chunk associated with a template parameter list, if detected during parsing
	 */
	Chunk *get_template_end() const;
	/**
	 * Return the starting chunk associated with a template parameter list, if detected during parsing
	 */
	Chunk *get_template_start() const;
	/**
	 * Returns a numerically-indexed map of all top-level commas encountered during parsing
	 */
	std::map<std::size_t, Chunk *> get_top_level_commas() const;
	/**
	 * Return the starting chunk associated with a where clause, if detected during parsing
	 */
	Chunk *get_where_end() const;
	/**
	 * Return the starting chunk associated with a where clause, if detected during parsing
	 */
	Chunk *get_where_start() const;
	/**
	 * Returns true if an inheritance list associated with a class or struct was discovered during parsing
	 */
	bool inheritance_detected() const;
public:
	/**
	 * Performs object initialization prior to parsing
	 */
	void initialize(Chunk * pc);
private:
	/**
	 * Returns true if the chunk under test represents a potential end chunk past which further parsing is not
	 *likely warranted
	 */
	bool is_potential_end_chunk(Chunk * pc) const;
	/**
	 * Returns true if the chunk under test is deemed to be located within a conditional/ternary statement
	 */
	bool is_within_conditional(Chunk * pc) const;
	/**
	 * Returns true if the chunk under test is deemed to be located within an inheritance list
	 */
	bool is_within_inheritance_list(Chunk * pc) const;
	/**
	 * Returns true if the chunk under test is deemed to be located within a where clause
	 */
	bool is_within_where_clause(Chunk * pc) const;
	/**
	 * Marks all base classes that appear as part of an inheritance list
	 */
	void mark_base_classes(Chunk * pc);
	/**
	 * Marks pairs of braces associated with the body of a class/enum/struct/union, and additionally calls a
	 *separate routine to mark any base classes for that may precede the opening brace
	 */
	void mark_braces(Chunk * start);
	/**
	 * Marks the beginning chunk of an inheritance list
	 */
	void mark_class_colon(Chunk * colon);
	/**
	 * Mark a colon as a conditional
	 */
	void mark_conditional_colon(Chunk * colon);
	/**
	 * Mark any struct/class constructor declarations/definitions
	 */
	void mark_constructors();
	/**
	 * Marks the beginning chunk of an enumerated integral type specification
	 */
	void mark_enum_integral_type(Chunk * colon);
	/**
	 * Scan chunks outside the definition body and mark lvalues accordingly
	 */
	void mark_extracorporeal_lvalues();
	/**
	 * Mark nested name specifiers preceding qualified identifiers
	 */
	void mark_nested_name_specifiers(Chunk * pc);
	/**
	 * Marks pointer operators preceding a variable identifier
	 */
	void mark_pointer_types(Chunk * pc);
	/**
	 * Marks the beginning and ending chunks associated with a template (templates may appear after the identifier
	 *type name as part of a class specialization)
	 */
	void mark_template(Chunk * start) const;
	/**
	 * Marks the arguments within a template argument list bounded by the starting and ending chunks
	 */
	void mark_template_args(Chunk * start, Chunk * end) const;
	/**
	 * Marks the type identifier associated with the class/enum/struct/union, if not anonymously defined
	 */
	void mark_type(Chunk * pc);
	/**
	 * Marks all variable identifiers associated with the class/enum/struct/union
	 */
	void mark_variable(Chunk * variable, PcfFlags flags);
	/**
	 * Marks all chunks belonging to a c# where clause
	 */
	void mark_where_clause(Chunk * where);
	/**
	 * Marks the beginning of a where clause
	 */
	void mark_where_colon(Chunk * colon);
public:
	/**
	 * Parses the class/enum/struct/union and all associated chunks
	 */
	void parse(Chunk * pc);
private:
	/**
	 * Parses closing and opening angle brackets
	 */
	Chunk *parse_angles(Chunk * angle_open);
	/**
	 * Parses closing and opening braces
	 */
	Chunk *parse_braces(Chunk * brace_open);
	/**
	 * Parses a single colon, which may precede an inheritance list or enumerated integral type specification
	 */
	void parse_colon(Chunk * colon);
	/**
	 * Parses a double colon, which may indicate a scope resolution chain
	 */
	Chunk *parse_double_colon(Chunk * double_colon);
	/**
	 * Returns the parsing error status
	 */
	bool parse_error_detected() const;
	/**
	 * Sets the parsing error status
	 */
	void parse_error_detected(bool status);
	/**
	 * Records all question operators encountered during parsing
	 */
	void record_question_operator(Chunk * question);
	/**
	 * Records a comma chunk given one the following conditions are satisfied:
	 * 1) it is encountered at the level of the starting chunk 2) it is not part of a right-hand side assignment 3)
	 *it is not part of an inheritance list 4) it is not part of a conditional/ternary expression
	 */
	void record_top_level_comma(Chunk * comma);
	/**
	 * Adjusts the end chunk returned by the try_find_end_chunk() function for any potential trailing inline
	 *variable declarations that may follow the body of a class/enum/struct/union definition
	 */
	Chunk *refine_end_chunk(Chunk * pc);
	/**
	 * Sets the chunk associated with the end of a class/enum/struct/union body
	 */
	void set_body_end(Chunk * body_end);
	/**
	 * Sets the chunk associated with the start of a class/enum/struct/union body
	 */
	void set_body_start(Chunk * body_start);
	/**
	 * Sets the chunk associated with the start of an enumerated integral base type specification
	 */
	void set_enum_base_start(Chunk * enum_base_start);
	/**
	 * Sets the chunk associated with the start of an inheritance list
	 */
	void set_inheritance_start(Chunk * inheritance_start);
	/**
	 * Sets the chunk associated with the end of a template
	 */
	void set_template_end(Chunk * template_end);
	/**
	 * Sets the chunk associated with the start of a template
	 */
	void set_template_start(Chunk * template_start);
	/**
	 * Return the ending chunk associated with a where clause, if detected during parsing
	 */
	void set_where_end(Chunk * where_end);
	/**
	 * Return the starting chunk associated with a where clause, if detected during parsing
	 */
	void set_where_start(Chunk * where_start);
	/**
	 * Returns true if a template was detected during parsing
	 */
	bool template_detected() const;
	/**
	 * Attempts to find the last chunk associated with the class/enum/struct/union
	 */
	Chunk *try_find_end_chunk(Chunk * pc);
	/**
	 * Attempts to identify any function-like macro calls which may precede the actual type identifier
	 */
	void try_post_identify_macro_calls();
	/**
	 * Attempts to find the identifier type name (if not anonymously-defined) post variable identifier
	 *interpretation
	 */
	void try_post_identify_type();
	/**
	 * Attempts to find the identifier type name prior to variable identifier interpretation
	 */
	bool try_pre_identify_type();
	/**
	 * Returns true if a corresponding type was identified for the class/enum/struct/union
	 */
	bool type_identified() const;
	/**
	 * Returns true if a where clause was detected during parsing
	 */
	bool where_clause_detected() const;
	std::map<E_Token, std::map<std::size_t, Chunk *> > m_chunk_map; // Map of token-type, chunk pairs
	Chunk * m_end; // Indicates the last chunk associated with the class/enum/struct/union keyword
	bool m_parse_error; // Indicates whether or not a parse error has occurred
	Chunk * m_start; // Stores a pointer to the class/enum/struct/union keyword chunk with which the parse() routine was invoked
	Chunk * m_type; // Stores a pointer to the type identifier associated with the class/enum/struct/union, if not anonymously defined
};
//
//#include "cs_top_is_question.h"
bool cs_top_is_question(ChunkStack &cs, size_t level);
//
//#include "combine_labels.h"
// 
// Descr: Examines the whole file and changes CT_COLON to CT_Q_COLON, CT_LABEL_COLON, or CT_CASE_COLON. It also changes the
// CT_WORD before CT_LABEL_COLON into CT_LABEL.
// 
void combine_labels();
//
#endif // __UNCRUSTIFY_INTERNAL_H
