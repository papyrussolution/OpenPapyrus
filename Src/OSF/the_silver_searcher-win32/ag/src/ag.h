// AG.H
//
#ifndef __AG_H
#define __AG_H

#include <slib.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdarg.h>
#include <fcntl.h>
#include <pcre.h>
//#include "config.h"
#define HAVE_ZLIB_H
#define HAVE_LZMA_H
#define HAVE_PTHREAD_H
#define HAVE_VASPRINTF
//
#ifdef HAVE_SYS_CPUSET_H
	#include <sys/cpuset.h>
#endif
#ifdef HAVE_PTHREAD_H
	#include <pthread.h>
#endif
#if defined(HAVE_PTHREAD_SETAFFINITY_NP) && defined(__FreeBSD__)
	#include <pthread_np.h>
#endif
#ifdef __FreeBSD__
	#include <sys/endian.h>
#endif
#ifdef __CYGWIN__
	typedef _off64_t off64_t;
#endif
#ifdef HAVE_ERR_H
	#include <err.h>
#endif
#ifdef HAVE_ZLIB_H
	#include <zlib.h>
#endif
#ifdef HAVE_LZMA_H
	#include <..\osf\liblzma\api\lzma.h>
#endif
#include <getopt.h>
#include "uthash.h"
//#include "ignore.h"
struct ignores {
	char ** extensions; /* File extensions to ignore */
	size_t extensions_len;
	char ** names; /* Non-regex ignore lines. Sorted so we can binary search them. */
	size_t names_len;
	char ** slash_names; /* Same but starts with a slash */
	size_t slash_names_len;
	char ** regexes; /* For patterns that need fnmatch */
	size_t regexes_len;
	char ** invert_regexes; /* For "!" patterns */
	size_t invert_regexes_len;
	char ** slash_regexes;
	size_t slash_regexes_len;
	const char * dirname;
	size_t dirname_len;
	char * abs_path;
	size_t abs_path_len;
	struct ignores * parent;
};

typedef struct ignores ignores;

extern ignores * root_ignores;
extern const char * evil_hardcoded_ignore_files[];
extern const char * ignore_pattern_files[];

ignores * init_ignore(ignores * parent, const char * dirname, const size_t dirname_len);
void cleanup_ignore(ignores * ig);
void add_ignore_pattern(ignores * ig, const char * pattern);
void load_ignore_patterns(ignores * ig, const char * path);
int filename_filter(const char * path, const struct dirent * dir, void * baton);
int is_empty(ignores * ig);
//
//#include "log.h"
extern pthread_mutex_t print_mtx;

enum log_level {
	LOG_LEVEL_DEBUG = 10,
	LOG_LEVEL_MSG = 20,
	LOG_LEVEL_WARN = 30,
	LOG_LEVEL_ERR = 40,
	LOG_LEVEL_NONE = 100
};

void set_log_level(enum log_level threshold);
void log_debug(const char * fmt, ...);
void log_msg(const char * fmt, ...);
void log_warn(const char * fmt, ...);
void log_err(const char * fmt, ...);
void vplog(const uint level, const char * fmt, va_list args);
void plog(const uint level, const char * fmt, ...);
//
//#include "options.h"
#define DEFAULT_AFTER_LEN 2
#define DEFAULT_BEFORE_LEN 2
#define DEFAULT_CONTEXT_LEN 2
#define DEFAULT_MAX_SEARCH_DEPTH 25
enum case_behavior {
	CASE_DEFAULT, /* Changes to CASE_SMART at the end of option parsing */
	CASE_SENSITIVE,
	CASE_INSENSITIVE,
	CASE_SMART,
	CASE_SENSITIVE_RETRY_INSENSITIVE /* for future use */
};

enum path_print_behavior {
	PATH_PRINT_DEFAULT,       /* PRINT_TOP if > 1 file being searched, else PRINT_NOTHING */
	PATH_PRINT_DEFAULT_EACH_LINE, /* PRINT_EACH_LINE if > 1 file being searched, else PRINT_NOTHING */
	PATH_PRINT_TOP,
	PATH_PRINT_EACH_LINE,
	PATH_PRINT_NOTHING
};

typedef struct {
	int ackmate;
	pcre * ackmate_dir_filter;
	pcre_extra * ackmate_dir_filter_extra;
	size_t after;
	size_t before;
	enum case_behavior casing;
	const char * file_search_string;
	int match_files;
	pcre * file_search_regex;
	pcre_extra * file_search_regex_extra;
	int color;
	char * color_line_number;
	char * color_match;
	char * color_path;
	int color_win_ansi;
	int column;
	int context;
	int follow_symlinks;
	int invert_match;
	int literal;
	int literal_starts_wordchar;
	int literal_ends_wordchar;
	size_t max_matches_per_file;
	int max_search_depth;
	int mmap;
	int multiline;
	int one_dev;
	int only_matching;
	char path_sep;
	int path_to_ignore;
	int print_break;
	int print_count;
	int print_filename_only;
	int print_nonmatching_files;
	int print_path;
	int print_all_paths;
	int print_line_numbers;
	int print_long_lines; /* TODO: support this in print.c */
	int passthrough;
	pcre * re;
	pcre_extra * re_extra;
	int recurse_dirs;
	int search_all_files;
	int skip_vcs_ignores;
	int search_binary_files;
	int search_zip_files;
	int search_hidden_files;
	int search_stream; /* true if tail -F blah | ag */
	int stats;
	size_t stream_line_num; /* This should totally not be in here */
	int match_found; /* This should totally not be in here */
	ino_t stdout_inode;
	char * query;
	int query_len;
	char * pager;
	int paths_len;
	int parallel;
	int use_thread_affinity;
	int vimgrep;
	size_t width;
	int word_regexp;
	int workers;
} cli_options;

/* global options. parse_options gives it sane values, everything else reads from it */
extern cli_options opts;

typedef struct option option_t;

void usage(void);
void print_version(void);
void init_options(void);
void parse_options(int argc, char ** argv, char ** base_paths[], char ** paths[]);
void cleanup_options(void);
//
//#include "util.h"
extern FILE * out_fd;
#define H_SIZE (64 * 1024)
#ifdef __clang__
	#define NO_SANITIZE_ALIGNMENT __attribute__((no_sanitize("alignment")))
#else
	#define NO_SANITIZE_ALIGNMENT
#endif

void * FASTCALL ag_malloc(size_t size);
void * FASTCALL ag_realloc(void * ptr, size_t size);
void * FASTCALL ag_calloc(size_t nelem, size_t elsize);
char * FASTCALL ag_strdup(const char * s);
char * FASTCALL ag_strndup(const char * s, size_t size);

typedef struct {
	size_t start; /* Byte at which the match starts */
	size_t end; /* and where it ends */
} match_t;

typedef struct {
	size_t total_bytes;
	size_t total_files;
	size_t total_matches;
	size_t total_file_matches;
	struct timeval time_start;
	struct timeval time_end;
} ag_stats;

extern ag_stats stats;
//
// Union to translate between chars and words without violating strict aliasing
//
typedef union {
	char as_chars[sizeof(uint16_t)];
	uint16_t as_word;
} word_t;

void FASTCALL free_strings(char ** strs, const size_t strs_len);
void generate_alpha_skip(const char * find, size_t f_len, size_t skip_lookup[], const int case_sensitive);
int is_prefix(const char * s, const size_t s_len, const size_t pos, const int case_sensitive);
size_t suffix_len(const char * s, const size_t s_len, const size_t pos, const int case_sensitive);
void generate_find_skip(const char * find, const size_t f_len, size_t ** skip_lookup, const int case_sensitive);
void generate_hash(const char * find, const size_t f_len, uint8 * H, const int case_sensitive);

// max is already defined on spec-violating compilers such as MinGW 
//size_t ag_max(size_t a, size_t b);
//size_t ag_min(size_t a, size_t b);

const char * boyer_moore_strnstr(const char * s, const char * find, const size_t s_len, const size_t f_len,
    const size_t alpha_skip_lookup[], const size_t * find_skip_lookup, const int case_insensitive);
const char * hash_strnstr(const char * s, const char * find, const size_t s_len, const size_t f_len, uint8 * h_table, const int case_sensitive);
size_t invert_matches(const char * buf, const size_t buf_len, match_t matches[], size_t matches_len);
void realloc_matches(match_t ** matches, size_t * matches_size, size_t matches_len);
void compile_study(pcre ** re, pcre_extra ** re_extra, char * q, const int pcre_opts, const int study_opts);
int is_binary(const void * buf, const size_t buf_len);
int is_regex(const char * query);
int is_fnmatch(const char * filename);
int binary_search(const char * needle, char ** haystack, int start, int end);
void init_wordchar_table(void);
int is_wordchar(char ch);
int is_lowercase(const char * s);
int is_directory(const char * path, const struct dirent * d);
int is_symlink(const char * path, const struct dirent * d);
int is_named_pipe(const char * path, const struct dirent * d);
void die(const char * fmt, ...);
void ag_asprintf(char ** ret, const char * fmt, ...);
ssize_t buf_getline(const char ** line, const char * buf, const size_t buf_len, const size_t buf_offset);

#ifndef HAVE_FGETLN
	char * fgetln(FILE * fp, size_t * lenp);
#endif
#ifndef HAVE_GETLINE
	ssize_t getline(char ** lineptr, size_t * n, FILE * stream);
#endif
#ifndef HAVE_REALPATH
	char * realpath(const char * path, char * resolved_path);
#endif
#ifndef HAVE_STRLCPY
	size_t strlcpy(char * dest, const char * src, size_t size);
#endif
#ifndef HAVE_VASPRINTF
	int vasprintf(char ** ret, const char * fmt, va_list args);
#endif
//
//#include "print.h"
void print_init_context(void);
void print_cleanup_context(void);
void print_context_append(const char *line, size_t len);
void print_trailing_context(const char *path, const char *buf, size_t n);
void print_path(const char *path, const char sep);
void print_path_count(const char *path, const char sep, const size_t count);
void print_line(const char *buf, size_t buf_pos, size_t prev_line_offset);
void print_binary_file_matches(const char *path);
void print_file_matches(const char *path, const char *buf, const size_t buf_len, const match_t matches[], const size_t matches_len);
void print_line_number(size_t line, const char sep);
void print_column_number(const match_t matches[], size_t last_printed_match, size_t prev_line_offset, const char sep);
void print_file_separator(void);
const char *normalize_path(const char *path);
#ifdef _WIN32
	void windows_use_ansi(int use_ansi);
	int fprintf_w32(FILE *fp, const char *format, ...);
#endif
//
//#include "search.h"
#ifdef _WIN32
//#include <windows.h>
#else
	#include <sys/mman.h>
#endif

extern size_t alpha_skip_lookup[256];
extern size_t * find_skip_lookup;
extern uint8 h_table[H_SIZE] /* @sobolev __attribute__((aligned(64)))*/;

struct work_queue_t {
	char * path;
	struct work_queue_t * next;
};

typedef struct work_queue_t work_queue_t;

extern work_queue_t * work_queue;
extern work_queue_t * work_queue_tail;
extern int done_adding_files;
extern pthread_cond_t files_ready;
extern pthread_mutex_t stats_mtx;
extern pthread_mutex_t work_queue_mtx;

/* For symlink loop detection */
#define SYMLOOP_ERROR (-1)
#define SYMLOOP_OK (0)
#define SYMLOOP_LOOP (1)

typedef struct {
	dev_t dev;
	ino_t ino;
} dirkey_t;

typedef struct {
	dirkey_t key;
	UT_hash_handle hh;
} symdir_t;

extern symdir_t * symhash;

ssize_t search_buf(const char * buf, const size_t buf_len, const char * dir_full_path);
ssize_t search_stream(FILE * stream, const char * path);
void search_file(const char * file_full_path);
void * search_file_worker(void * i);
void search_dir(ignores * ig, const char * base_path, const char * path, const int depth, dev_t original_dev);
//
//#include "scandir.h"
typedef struct {
	const ignores *ig;
	const char *base_path;
	size_t base_path_len;
	const char *path_start;
} scandir_baton_t;

typedef int (*filter_fp)(const char *path, const struct dirent *, void *);
int ag_scandir(const char *dirname, struct dirent ***namelist, filter_fp filter, void *baton);
//
//#include "lang.h"
#define MAX_EXTENSIONS 12
#define SINGLE_EXT_LEN 20

typedef struct {
	const char * name;
	const char * extensions[MAX_EXTENSIONS];
} lang_spec_t;

// (become static in options.c) extern const lang_spec_t langs[];
// 
// Return the language count.
// 
size_t get_lang_count(void);
// 
// Convert a NULL-terminated array of language extensions
// into a regular expression of the form \.(extension1|extension2...)$
// Caller is responsible for freeing the returned string.
// 
char * make_lang_regex(char * ext_array, size_t num_exts);
// 
// Combine multiple file type extensions into one array.
// The combined result is returned through *exts*;
// * exts* is one-dimension array, which can contain up to 100 extensions;
// The number of extensions that *exts* actually contain is returned.
// 
size_t combine_file_extensions(size_t * extension_index, size_t len, char ** exts);
//
//#include "decompress.h"
typedef enum {
	AG_NO_COMPRESSION,
	AG_GZIP,
	AG_COMPRESS,
	AG_ZIP,
	AG_XZ,
} ag_compression_type;

ag_compression_type is_zipped(const void * buf, const int buf_len);
void * decompress(const ag_compression_type zip_type, const void * buf, const int buf_len, const char * dir_full_path, int * new_buf_len);
#if HAVE_FOPENCOOKIE
	FILE * decompress_open(int fd, const char * mode, ag_compression_type ctype);
#endif
//
#endif // } __AG_H
