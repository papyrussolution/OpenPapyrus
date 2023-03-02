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
#include "ChunkStack.h"
#include "align_same_func_call_params.h"
#include "align_stack.h"
#include "align_struct_initializers.h"
#include "align_trailing_comments.h"
#include "align_typedefs.h"
#include "align_var_def_brace.h"
#include "align_add.h"
//#include "quick_align_again.h"
void quick_align_again();
//
#include "indent.h"
#include "align_tools.h"
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
#include "align_nl_cont.h"
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
