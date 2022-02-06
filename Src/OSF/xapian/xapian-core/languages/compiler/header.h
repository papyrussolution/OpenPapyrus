//
//
#include <stdio.h>

#define SNOWBALL_VERSION "2.0.0"

typedef uchar byte;
typedef ushort symbol;

//#define true 1
//#define false 0

#define MALLOC check_malloc
#define FREE check_free

#define NEW(type, p) struct type * p = (struct type *)MALLOC(sizeof(struct type))
#define NEWVEC(type, p, n) struct type * p = (struct type *)MALLOC(sizeof(struct type) * (n))

#define SIZE(p)     ((int*)(p))[-1]
#define CAPACITY(p) ((int*)(p))[-2]

extern symbol * create_b(int n);
extern void report_b(FILE * out, const symbol * p);
extern void lose_b(symbol * p);
extern symbol * increase_capacity(symbol * p, int n);
extern symbol * move_to_b(symbol * p, int n, const symbol * q);
extern symbol * add_to_b(symbol * p, int n, const symbol * q);
extern symbol * copy_b(const symbol * p);
extern char * b_to_s(const symbol * p);
extern symbol * FASTCALL add_s_to_b(symbol * p, const char * s);

#define MOVE_TO_B(B, LIT) move_to_b(B, sizeof(LIT) / sizeof(LIT[0]), LIT)

struct str; // defined in space.c 
struct Analyser;

extern struct str * str_new(void);
extern void str_delete(struct str * str);
extern void str_append(struct str * str, struct str * add);
extern void str_append_ch(struct str * str, char add);
extern void str_append_b(struct str * str, const symbol * q);
extern void str_append_string(struct str * str, const char * s);
extern void str_append_int(struct str * str, int i);
extern void str_clear(struct str * str);
extern void str_assign(struct str * str, char * s);
extern struct str * str_copy(struct str * old);
extern symbol * str_data(struct str * str);
extern int str_len(struct str * str);
extern int str_back(struct str * str);
extern int get_utf8(const symbol * p, int * slot);
extern int put_utf8(int ch, symbol * p);
extern void output_str(FILE * outfile, struct str * str);

typedef enum { 
	ENC_SINGLEBYTE, 
	ENC_UTF8, 
	ENC_WIDECHARS 
} enc;

struct MPair {
	MPair * next;
	symbol * name;
	symbol * value;
};
//
// Input must be a prefix of Tokeniser. 
//
struct Input {
	Input * P_Next;
	symbol * P_Symb;
	int    C;
	const char * P_FileName;
	bool   FileNeedsFreeing;
	int    LineNumber;
};

struct Include {
	Include * next;
	symbol * b;
};

enum token_codes {
#include "syswords2.h"
	c_mathassign,
	c_name,
	c_number,
	c_literalstring,
	c_neg,
	c_call,
	c_grouping,
	c_booltest,
	NUM_TOKEN_CODES
};

enum uplus_modes {
	UPLUS_NONE,
	UPLUS_DEFINED,
	UPLUS_UNICODE
};
//
// Input must be a prefix of Tokeniser
//
struct Tokeniser {
	static int HexToNum(int ch);
	Analyser * CreateAnalyser();
	void Error(const char * s1, int n, symbol * pSymb, const char * s2);
	void Error1(const char * s);
	void Error2(const char * s);
	int  NextChar();
	int  WhiteSpace(int ch);
	int  NextRealChar();
	void ReadChars();
	const symbol * FindInM(int n, const symbol * p) const;
	void ConvertNumericString(symbol * pSymb, int base);
	int  NextToken();
	int  ReadToken();
	//
	Input  * P_Next;
	symbol * P_Symb;
	int    C;
	const char * P_FileName;
	bool   FileNeedsFreeing;
	int    LineNumber;
	bool   TokenHeld;
	symbol * P_b;
	symbol * P_b2;
	int    Number;
	int    m_start;
	int    m_end;
	MPair * m_pairs;
	int    GetDepth;
	int    ErrCount;
	int    Token;
	int    PreviousToken;
	enc    Encoding;
	int    Omission;
	Include * P_Includes;
	// Mode in which U+ has been used:
	// UPLUS_NONE - not used yet
	// UPLUS_DEFINED - stringdef U+xxxx ....
	// UPLUS_UNICODE - {U+xxxx} used with implicit meaning
	int    UPlusMode;
	char   TokenDisabled[NUM_TOKEN_CODES];
};

extern symbol * get_input(const char * filename);
extern Tokeniser * create_tokeniser(symbol * b, const char * file);
//extern int read_token(Tokeniser * t);
extern const char * name_of_token(int code);
extern void disable_token(Tokeniser * t, int code);
extern void close_tokeniser(Tokeniser * t);

extern int space_count;
extern void * check_malloc(int n);
extern void check_free(void * p);

struct Node;
struct Grouping;
struct Options;

struct Name {
	Name * next;
	symbol * b;
	int type;               /* t_string etc */
	int mode;               /*    )_  for routines, externals */
	Node * definition; /*    )                           */
	int count;              /* 0, 1, 2 for each type */
	int among_func_count;   /* 1, 2, 3 for routines called by among */
	Grouping * grouping; /* for grouping names */
	byte referenced;
	byte used_in_among;     /* Function used in among? */
	byte value_used;        /* (For variables) is its value ever used? */
	byte initialised;       /* (For variables) is it ever initialised? */
	byte used_in_definition; /* (grouping) used in grouping definition? */
	const Node * P_Used; // First use, or NULL if not used 
	Name * local_to; // Local to one routine/external 
	int declaration_line_number;/* Line number of declaration */
};

struct LiteralString {
	LiteralString * next;
	symbol * b;
};

struct AmongVec {
	symbol * b;         // the string giving the case 
	int    size;        // - and its size 
	Node * action;      // the corresponding action 
	int    i;           // the AmongVec index of the longest substring of b 
	int    result;      // the numeric result for the case 
	int    line_number; // for diagnostics and stable sorting 
	Name * function;
};

struct Among {
	Among * P_Next;
	AmongVec * P_Vec; // pointer to the AmongVec
	int number;           /* amongs are numbered 0, 1, 2 ... */
	int LiteralStringCount; // in this among
	int CommandCount;       // in this among
	int NoCommandCount;     // number of "no command" entries in this among 
	int function_count;   /* in this among */
	int amongvar_needed;  /* do we need to set among_var? */
	Node * starter; /* i.e. among( (starter) 'string' ... ) */
	Node * substring; /* i.e. substring ... among ( ... ) */
	Node ** commands; /* array with command_count entries */
};

struct Grouping {
	Grouping * next;
	symbol * b;           /* the characters of this group */
	int largest_ch;       /* character with max code */
	int smallest_ch;      /* character with min code */
	Name * name;   /* so g->name->grouping == g */
	int line_number;
};

struct Node {
	void   Print_(int n, const char * s) const;
	Node * P_Next;
	Node * P_Left;
	Node * P_Aux; // used in setlimit 
	Among * among; /* used in among */
	Node * P_Right;
	int type;
	int mode;
	Node * AE;
	Name * name;
	symbol * P_LiteralString;
	int number;
	int line_number;
	int AmongVarNeeded; // used in routine definitions
};

enum name_types {
	t_size = 6,
	t_string = 0, 
	t_boolean = 1, 
	t_integer = 2, 
	t_routine = 3, 
	t_external = 4,
	t_grouping = 5
/*  If this list is extended, adjust wvn in generator.c  */
};

/*  In name_count[i] below, remember that
    type   is
    ----+----
      0 |  string
      1 |  boolean
      2 |  integer
      3 |  routine
      4 |  external
      5 |  grouping
 */
struct Analyser {
	symbol * NewLiteralString();
	Node * ReadLiteralString();
	Node * NewNode(int type);
	Name * LookForName();
	Name * FindName();
	void   CountError();

	Tokeniser * P_Tokeniser;
	Node * P_Nodes;
	Name * P_Names;
	LiteralString * literalstrings;
	int    Mode;
	bool   Modifyable;      /* false inside reverse(...) */
	Node * P_Program;
	Node * P_ProgramEnd;
	int    NameCount[t_size]; // name_count[i] counts the number of names of type i 
	Among * P_Amongs;
	Among * P_AmongsEnd;
	int    AmongCount;
	int    AmongVarNeeded; // used in reading routine definitions
	Grouping * P_Groupings;
	Grouping * P_GroupingsEnd;
	Node * P_Substring; // pending 'substring' in current routine definition 
	enc    Encoding;
	bool   IntLimitsUsed; // are maxint or minint used?
};

enum analyser_modes {
	m_forward = 0, 
	m_backward /*, m_integer */
};

extern void print_program(Analyser * a);
//extern Analyser * create_analyser(Tokeniser * t);
extern void close_analyser(Analyser * a);
extern void read_program(Analyser * a);

struct Generator {
	int NewLabel() 
	{
		return NextLabel++;
	}
	Analyser * analyser;
	const Options * P_Options;
	int unreachable; // 0 if code can be reached, 1 if current code is unreachable. 
	int var_number;        /* Number of next variable to use. */
	struct str * outbuf;   /* temporary str to store output */
	struct str * declarations; /* str storing variable declarations */
	int NextLabel;
#ifndef DISABLE_PYTHON
	int max_label;
#endif
	int margin;
	/* if > 0, keep_count to restore in case of a failure;
	 * if < 0, the negated keep_count for the limit to restore in case of
	 * failure. */
	int failure_keep_count;
#if !defined(DISABLE_JAVA) && !defined(DISABLE_JS) && !defined(DISABLE_PYTHON) && !defined(DISABLE_CSHARP)
	struct str * failure_str; /* This is used by some generators instead of failure_keep_count */
#endif
	int label_used; /* Keep track of whether the failure label is used. */
	int failure_label;
	int debug_count;
	int copy_from_count; /* count of calls to copy_from() */
	const char * S[10]; /* strings */
	symbol * B[10];  /* blocks */
	int I[10];       /* integers */
	const Name * V[5]; // variables
	symbol * L[5];   /* literals, used in formatted write */
	int line_count;  /* counts number of lines output */
	int line_labelled; /* in ISO C, will need extra ';' if it is a block end */
	int literalstring_count;
	int keep_count;  // used to number keep/restore pairs to avoid compiler warnings about shadowed variables 
};
//
// Special values for failure_label in Generator. 
//
enum special_labels {
	x_return = -1
};

struct Options {
	// for the command line: 
	const char * output_file;
	const char * name;
	mutable FILE * output_src;
	mutable FILE * output_h;
	byte   syntax_tree;
	byte   comments;
	enc    encoding;
	enum { 
		LANG_JAVA, 
		LANG_C, 
		LANG_CPLUSPLUS, 
		LANG_CSHARP, 
		LANG_PASCAL, 
		LANG_PYTHON, 
		LANG_JAVASCRIPT, 
		LANG_RUST, 
		LANG_GO 
	} make_lang;

	const char * externals_prefix;
	const char * variables_prefix;
	const char * runtime_path;
	const char * parent_class_name;
	const char * package;
	const char * go_snowball_runtime;
	const char * string_class;
	const char * among_class;
	Include * includes;
	Include * includes_end;
};
//
// Generator functions common to several backends.
//
extern Generator * create_generator(Analyser * a, const Options * o);
extern void close_generator(Generator * g);
extern void write_char(Generator * g, int ch);
extern void write_newline(Generator * g);
extern void write_string(Generator * g, const char * s);
extern void write_int(Generator * g, int i);
extern void write_b(Generator * g, const symbol * b);
extern void write_str(Generator * g, struct str * str);
extern void write_comment_content(Generator * g, const Node * p);
extern void write_generated_comment_content(Generator * g);
extern void write_start_comment(Generator * g, const char * comment_start, const char * comment_end);
extern int K_needed(Generator * g, Node * p);
extern int repeat_restore(Generator * g, Node * p);
extern void generate_program_c(Generator * g); /* Generator for C code. */
#ifndef DISABLE_JAVA
	extern void generate_program_java(Generator * g); /* Generator for Java code. */
#endif
#ifndef DISABLE_CSHARP
	extern void generate_program_csharp(Generator * g); /* Generator for C# code. */
#endif
#ifndef DISABLE_PASCAL
	extern void generate_program_pascal(Generator * g);
#endif
#ifndef DISABLE_PYTHON
	extern void generate_program_python(Generator * g); /* Generator for Python code. */
#endif
#ifndef DISABLE_JS
	extern void generate_program_js(Generator * g);
#endif
#ifndef DISABLE_RUST
	extern void generate_program_rust(Generator * g);
#endif
#ifndef DISABLE_GO
	extern void generate_program_go(Generator * g);
#endif
