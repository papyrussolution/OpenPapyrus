
/* flexdef - definitions file for flex */

/*  Copyright (c) 1990 The Regents of the University of California. */
/*  All rights reserved. */

/*  This code is derived from software contributed to Berkeley by */
/*  Vern Paxson. */

/*  The United States Government has rights in this work pursuant */
/*  to contract no. DE-AC03-76SF00098 between the United States */
/*  Department of Energy and the University of California. */

/*  This file is part of flex. */

/*  Redistribution and use in source and binary forms, with or without */
/*  modification, are permitted provided that the following conditions */
/*  are met: */

/*  1. Redistributions of source code must retain the above copyright */
/*     notice, this list of conditions and the following disclaimer. */
/*  2. Redistributions in binary form must reproduce the above copyright */
/*     notice, this list of conditions and the following disclaimer in the */
/*     documentation and/or other materials provided with the distribution. */

/*  Neither the name of the University nor the names of its contributors */
/*  may be used to endorse or promote products derived from this software */
/*  without specific prior written permission. */

/*  THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR */
/*  IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED */
/*  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR */
/*  PURPOSE. */

#ifndef FLEXDEF_H
#define FLEXDEF_H 1

#ifdef HAVE_CONFIG_H
	#include <config.h>
#endif
#include <slib.h>
// For Visual Studio 2013 we need this workaround
#ifdef _MSC_VER
	#if _MSC_VER < 1900
		#define snprintf _snprintf
	#endif
#endif
#include <setjmp.h>
#include <process.h>
//#include <libgen.h> /* for XPG version of basename(3) */
//#include <unistd.h> /* Required: dup() and dup2() in <unistd.h> */
#ifdef HAVE_NETINET_IN_H
	#include <netinet/in.h>
#endif
#ifdef HAVE_SYS_PARAMS_H
	#include <sys/params.h>
#endif
/* Required: stat() in <sys/stat.h> */
	#include <sys/stat.h>
/* Required: wait() in <sys/wait.h> */
//#include <sys/wait.h>
#include <stdbool.h>
#include <stdarg.h>
#include <regex.h> /* Required: regcomp(), regexec() and regerror() in <regex.h> */
//#include <strings.h> /* Required: strcasecmp() in <strings.h> */
//#include "flexint.h"
	// C99 systems have <inttypes.h>. Non-C99 systems may or may not
	#if defined (__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
		// C99 says to define __STDC_LIMIT_MACROS before including stdint.h,
		// if you want the limit (max/min) macros for int types. 
		#ifndef __STDC_LIMIT_MACROS
			#define __STDC_LIMIT_MACROS 1
		#endif

		#include <inttypes.h>
		typedef int8_t flex_int8_t;
		typedef uint8_t flex_uint8_t;
		typedef int16_t flex_int16_t;
		typedef uint16_t flex_uint16_t;
		typedef int32_t flex_int32_t;
		typedef uint32_t flex_uint32_t;
	#else
		typedef signed char flex_int8_t;
		typedef short int flex_int16_t;
		typedef int flex_int32_t;
		typedef uchar flex_uint8_t; 
		typedef unsigned short int flex_uint16_t;
		typedef unsigned int flex_uint32_t;

		// Limits of integral types
		#ifndef INT8_MIN
			#define INT8_MIN               (-128)
		#endif
		#ifndef INT16_MIN
			#define INT16_MIN              (-32767-1)
		#endif
		#ifndef INT32_MIN
			#define INT32_MIN              (-2147483647-1)
		#endif
		#ifndef INT8_MAX
			#define INT8_MAX               (127)
		#endif
		#ifndef INT16_MAX
			#define INT16_MAX              (32767)
		#endif
		#ifndef INT32_MAX
			#define INT32_MAX              (2147483647)
		#endif
		#ifndef UINT8_MAX
			#define UINT8_MAX              (255U)
		#endif
		#ifndef UINT16_MAX
			#define UINT16_MAX             (65535U)
		#endif
		#ifndef UINT32_MAX
			#define UINT32_MAX             (4294967295U)
		#endif
		#ifndef SIZE_MAX
			#define SIZE_MAX               (~(size_t)0)
		#endif
	#endif /* ! C99 */
//
/* We use gettext. So, when we write strings which should be translated, we mark them with _() */
#ifdef ENABLE_NLS
	#ifdef HAVE_LOCALE_H
		#include <locale.h>
	#endif /* HAVE_LOCALE_H */
	#include "gettext.h"
	#define _(String) gettext (String)
#else
	#define _(STRING) STRING
#endif /* ENABLE_NLS */
#define CSIZE 256 // Always be prepared to generate an 8-bit scanner.
// Size of input alphabet - should be size of ASCII set. 
#ifndef DEFAULT_CSIZE
	#define DEFAULT_CSIZE 128
#endif
#define MAXLINE 2048 // Maximum line length we'll have to deal with. 
#ifndef ABS
	#define ABS(x) ((x) < 0 ? -(x) : (x))
#endif
#define is_power_of_2(n) ((n) > 0 && ((n) & ((n) - 1)) == 0) // Whether an integer is a power of two 
#define unspecified -1

/* Special chk[] values marking the slots taking by end-of-buffer and action numbers. */
#define EOB_POSITION -1
#define ACTION_POSITION -2
#define NUMDATAITEMS 10 // Number of data items per line for -f output.
#define NUMDATALINES 10 // Number of lines of data in -f output before inserting a blank line for readability. 
#define TRANS_STRUCT_PRINT_LENGTH 14 // transition_struct_out() definitions. 

/* Returns true if an nfa state has an epsilon out-transition slot
 * that can be used.  This definition is currently not used.
 */
#define FREE_EPSILON(state) (transchar[state] == SYM_EPSILON && trans2[state] == NO_TRANSITION && finalst[state] != state)

/* Returns true if an nfa state has an epsilon out-transition character
 * and both slots are free
 */
#define SUPER_FREE_EPSILON(state) (transchar[state] == SYM_EPSILON && trans1[state] == NO_TRANSITION)

/* Maximum number of NFA states that can comprise a DFA state.  It's real
 * big because if there's a lot of rules, the initial state will have a
 * huge epsilon closure.
 */
#define INITIAL_MAX_DFA_SIZE 750
#define MAX_DFA_SIZE_INCREMENT 750

/* A note on the following masks.  They are used to mark accepting numbers
 * as being special.  As such, they implicitly limit the number of accepting
 * numbers (i.e., rules) because if there are too many rules the rule numbers
 * will overload the mask bits.  Fortunately, this limit is \large/ (0x2000 ==
 * 8192) so unlikely to actually cause any problems.  A check is made in
 * new_rule() to ensure that this limit is not reached.
 */
#define YY_TRAILING_MASK 0x2000 /* Mask to mark a trailing context accepting number. */
#define YY_TRAILING_HEAD_MASK 0x4000 /* Mask to mark the accepting number of the "head" of a trailing context rule. */
#define MAX_RULE (YY_TRAILING_MASK - 1) /* Maximum number of rules, as outlined in the above note. */

/* NIL must be 0.  If not, its special meaning when making equivalence classes
 * (it marks the representative of a given e.c.) will be unidentifiable.
 */
#define NIL 0

#define JAM -1			/* to mark a missing DFA transition */
#define NO_TRANSITION NIL
#define UNIQUE -1		/* marks a symbol as an e.c. representative */
#define INFINITE_REPEAT -1		/* for x{5,} constructions */
#define INITIAL_MAX_CCLS 100	/* max number of unique character classes */
#define MAX_CCLS_INCREMENT 100

/* Size of table holding members of character classes. */
#define INITIAL_MAX_CCL_TBL_SIZE 500
#define MAX_CCL_TBL_SIZE_INCREMENT 250
#define INITIAL_MAX_RULES 100	/* default maximum number of rules */
#define MAX_RULES_INCREMENT 100
#define INITIAL_MNS 2000	/* default maximum number of nfa states */
#define MNS_INCREMENT 1000	/* amount to bump above by if it's not enough */
#define INITIAL_MAX_DFAS 1000	/* default maximum number of dfa states */
#define MAX_DFAS_INCREMENT 1000
#define JAMSTATE -32766		/* marks a reference to the state that always jams */

/* Maximum number of NFA states. */
#define MAXIMUM_MNS 31999
#define MAXIMUM_MNS_LONG 1999999999

/* Enough so that if it's subtracted from an NFA state number, the result
 * is guaranteed to be negative.
 */
#define MARKER_DIFFERENCE (maximum_mns+2)

/* Maximum number of nxt/chk pairs for non-templates. */
#define INITIAL_MAX_XPAIRS 2000
#define MAX_XPAIRS_INCREMENT 2000

/* Maximum number of nxt/chk pairs needed for templates. */
#define INITIAL_MAX_TEMPLATE_XPAIRS 2500
#define MAX_TEMPLATE_XPAIRS_INCREMENT 2500
#define SYM_EPSILON (CSIZE + 1)	/* to mark transitions on the symbol epsilon */
#define INITIAL_MAX_SCS 40	/* maximum number of start conditions */
#define MAX_SCS_INCREMENT 40	/* amount to bump by if it's not enough */
#define ONE_STACK_SIZE 500	/* stack of states with only one out-transition */
#define SAME_TRANS -1		/* transition is the same as "default" entry for state */

/* The following percentages are used to tune table compression:

 * The percentage the number of out-transitions a state must be of the
 * number of equivalence classes in order to be considered for table
 * compaction by using protos.
 */
#define PROTO_SIZE_PERCENTAGE 15

/* The percentage the number of homogeneous out-transitions of a state
 * must be of the number of total out-transitions of the state in order
 * that the state's transition table is first compared with a potential
 * template of the most common out-transition instead of with the first
 * proto in the proto queue.
 */
#define CHECK_COM_PERCENTAGE 50

/* The percentage the number of differences between a state's transition
 * table and the proto it was first compared with must be of the total
 * number of out-transitions of the state in order to keep the first
 * proto as a good match and not search any further.
 */
#define FIRST_MATCH_DIFF_PERCENTAGE 10

/* The percentage the number of differences between a state's transition
 * table and the most similar proto must be of the state's total number
 * of out-transitions to use the proto as an acceptable close match.
 */
#define ACCEPTABLE_DIFF_PERCENTAGE 50

/* The percentage the number of homogeneous out-transitions of a state
 * must be of the number of total out-transitions of the state in order
 * to consider making a template from the state.
 */
#define TEMPLATE_SAME_PERCENTAGE 60
/* The percentage the number of differences between a state's transition
 * table and the most similar proto must be of the state's total number
 * of out-transitions to create a new proto from the state.
 */
#define NEW_PROTO_DIFF_PERCENTAGE 20

/* The percentage the total number of out-transitions of a state must be
 * of the number of equivalence classes in order to consider trying to
 * fit the transition table into "holes" inside the nxt/chk table.
 */
#define INTERIOR_FIT_PERCENTAGE 15

/* Size of region set aside to cache the complete transition table of
 * protos on the proto queue to enable quick comparisons.
 */
#define PROT_SAVE_SIZE 2000
#define MSP 50			/* maximum number of saved protos (protos on the proto queue) */
/* Maximum number of out-transitions a state can have that we'll rummage
 * around through the interior of the internal fast table looking for a
 * spot for it.
 */
#define MAX_XTIONS_FULL_INTERIOR_FIT 4
#define MAX_ASSOC_RULES 100 // Maximum number of rules which will be reported as being associated with a DFA state.

/* Number that, if used to subscript an array, has a good chance of producing
 * an error; should be small enough to fit into a short.
 */
#define BAD_SUBSCRIPT -32767
/* Absolute value of largest number that can be stored in a short, with a
 * bit of slop thrown in for general paranoia.
 */
#define MAX_SHORT 32700

/* Declarations for global variables. */

/* Variables for flags:
 * printstats - if true (-v), dump statistics
 * syntaxerror - true if a syntax error has been found
 * eofseen - true if we've seen an eof in the input file
 * ddebug - if true (-d), make a "debug" scanner
 * trace - if true (-T), trace processing
 * nowarn - if true (-w), do not generate warnings
 * spprdflt - if true (-s), suppress the default rule
 * interactive - if true (-I), generate an interactive scanner
 * lex_compat - if true (-l), maximize compatibility with AT&T lex
 * posix_compat - if true (-X), maximize compatibility with POSIX lex
 * do_yylineno - if true, generate code to maintain yylineno
 * useecs - if true (-Ce flag), use equivalence classes
 * fulltbl - if true (-Cf flag), don't compress the DFA state table
 * usemecs - if true (-Cm flag), use meta-equivalence classes
 * fullspd - if true (-F flag), use Jacobson method of table representation
 * gen_line_dirs - if true (i.e., no -L flag), generate #line directives
 * performance_report - if > 0 (i.e., -p flag), generate a report relating
 *   to scanner performance; if > 1 (-p -p), report on minor performance
 *   problems, too
 * backing_up_report - if true (i.e., -b flag), generate "lex.backup" file
 *   listing backing-up states
 * C_plus_plus - if true (i.e., -+ flag), generate a C++ scanner class;
 *   otherwise, a standard C scanner
 * reentrant - if true (-R), generate a reentrant C scanner.
 * bison_bridge_lval - if true (--bison-bridge), bison pure calling convention.
 * bison_bridge_lloc - if true (--bison-locations), bison yylloc.
 * long_align - if true (-Ca flag), favor long-word alignment.
 * use_read - if true (-f, -F, or -Cr) then use read() for scanner input;
 *   otherwise, use fread().
 * yytext_is_array - if true (i.e., %array directive), then declare
 *   yytext as a array instead of a character pointer.  Nice and inefficient.
 * do_yywrap - do yywrap() processing on EOF.  If false, EOF treated as
 *   "no more files".
 * csize - size of character set for the scanner we're generating;
 *   128 for 7-bit chars and 256 for 8-bit
 * yymore_used - if true, yymore() is used in input rules
 * reject - if true, generate back-up tables for REJECT macro
 * real_reject - if true, scanner really uses REJECT (as opposed to just
 *   having "reject" set for variable trailing context)
 * continued_action - true if this rule's action is to "fall through" to
 *   the next rule's action (i.e., the '|' action)
 * in_rule - true if we're inside an individual rule, false if not.
 * yymore_really_used - whether to treat yymore() as really used, regardless
 *   of what we think based on references to it in the user's actions.
 * reject_really_used - same for REJECT
 * trace_hex - use hexadecimal numbers in trace/debug outputs instead of octals
 */
extern int printstats;
extern int syntaxerror;
extern int eofseen;
extern int ddebug;
extern int trace;
extern int nowarn;
extern int spprdflt;
extern int interactive;
extern int lex_compat;
extern int posix_compat;
extern int do_yylineno;
extern int useecs;
extern int fulltbl;
extern int usemecs;
extern int fullspd;
extern int gen_line_dirs;
extern int performance_report;
extern int backing_up_report;
extern int reentrant;
extern int bison_bridge_lval;
extern int bison_bridge_lloc;
extern int C_plus_plus;
extern int long_align;
extern int use_read;
extern int yytext_is_array;
extern int do_yywrap;
extern int csize;
extern int yymore_used;
extern int reject;
extern int real_reject;
extern int continued_action;
extern int in_rule;
extern int yymore_really_used;
extern int reject_really_used;
extern int trace_hex;

/* Variables used in the flex input routines:
 * datapos - characters on current output line
 * dataline - number of contiguous lines of data in current data
 * 	statement.  Used to generate readable -f output
 * linenum - current input line number
 * skelfile - the skeleton file
 * skel - compiled-in skeleton array
 * skel_ind - index into "skel" array, if skelfile is nil
 * yyin - input file
 * backing_up_file - file to summarize backing-up states to
 * infilename - name of input file
 * outfilename - name of output file
 * headerfilename - name of the .h file to generate
 * did_outfilename - whether outfilename was explicitly set
 * prefix - the prefix used for externally visible names ("yy" by default)
 * yyclass - yyFlexLexer subclass to use for YY_DECL
 * do_stdinit - whether to initialize yyin/yyout to stdin/stdout
 * use_stdout - the -t flag
 * input_files - array holding names of input files
 * num_input_files - size of input_files array
 * program_name - name with which program was invoked
 *
 * action_array - array to hold the rule actions
 * action_size - size of action_array
 * defs1_offset - index where the user's section 1 definitions start
 *	in action_array
 * prolog_offset - index where the prolog starts in action_array
 * action_offset - index where the non-prolog starts in action_array
 * action_index - index where the next action should go, with respect
 * 	to "action_array"
 */
extern int datapos;
extern int dataline;
extern int linenum;
extern FILE * skelfile;
extern FILE * backing_up_file;
extern const char * skel[];
extern int skel_ind;
extern char * infilename;
extern char * outfilename;
extern char * headerfilename;
extern int did_outfilename;
extern char * prefix;
extern char * yyclass;
extern char * extra_type;
extern int do_stdinit;
extern int use_stdout;
extern char **input_files;
extern int num_input_files;
extern char *program_name;
extern char *action_array;
extern int action_size;
extern int defs1_offset;
extern int prolog_offset;
extern int action_offset;
extern int action_index;

/* Variables for stack of states having only one out-transition:
 * onestate - state number
 * onesym - transition symbol
 * onenext - target state
 * onedef - default base entry
 * onesp - stack pointer
 */
extern int onestate[ONE_STACK_SIZE], onesym[ONE_STACK_SIZE];
extern int onenext[ONE_STACK_SIZE], onedef[ONE_STACK_SIZE], onesp;

/* Variables for nfa machine data:
 * maximum_mns - maximal number of NFA states supported by tables
 * current_mns - current maximum on number of NFA states
 * num_rules - number of the last accepting state; also is number of
 * 	rules created so far
 * num_eof_rules - number of <<EOF>> rules
 * default_rule - number of the default rule
 * current_max_rules - current maximum number of rules
 * lastnfa - last nfa state number created
 * firstst - physically the first state of a fragment
 * lastst - last physical state of fragment
 * finalst - last logical state of fragment
 * transchar - transition character
 * trans1 - transition state
 * trans2 - 2nd transition state for epsilons
 * accptnum - accepting number
 * assoc_rule - rule associated with this NFA state (or 0 if none)
 * state_type - a STATE_xxx type identifying whether the state is part
 * 	of a normal rule, the leading state in a trailing context
 * 	rule (i.e., the state which marks the transition from
 * 	recognizing the text-to-be-matched to the beginning of
 * 	the trailing context), or a subsequent state in a trailing
 * 	context rule
 * rule_type - a RULE_xxx type identifying whether this a ho-hum
 * 	normal rule or one which has variable head & trailing
 * 	context
 * rule_linenum - line number associated with rule
 * rule_useful - true if we've determined that the rule can be matched
 * rule_has_nl - true if rule could possibly match a newline
 * ccl_has_nl - true if current ccl could match a newline
 * nlch - default eol char
 */

extern int maximum_mns, current_mns, current_max_rules;
extern int num_rules, num_eof_rules, default_rule, lastnfa;
extern int *firstst, *lastst, *finalst, *transchar, *trans1, *trans2;
extern int *accptnum, *assoc_rule, *state_type;
extern int *rule_type, *rule_linenum, *rule_useful;
extern bool *rule_has_nl, *ccl_has_nl;
extern int nlch;

/* Different types of states; values are useful as masks, as well, for
 * routines like check_trailing_context().
 */
#define STATE_NORMAL 0x1
#define STATE_TRAILING_CONTEXT 0x2

/* Global holding current type of state we're making. */

extern int current_state_type;

/* Different types of rules. */
#define RULE_NORMAL 0
#define RULE_VARIABLE 1

/* True if the input rules include a rule with both variable-length head
 * and trailing context, false otherwise.
 */
extern int variable_trailing_context_rules;

/* Variables for protos:
 * numtemps - number of templates created
 * numprots - number of protos created
 * protprev - backlink to a more-recently used proto
 * protnext - forward link to a less-recently used proto
 * prottbl - base/def table entry for proto
 * protcomst - common state of proto
 * firstprot - number of the most recently used proto
 * lastprot - number of the least recently used proto
 * protsave contains the entire state array for protos
 */

extern int numtemps, numprots, protprev[MSP], protnext[MSP], prottbl[MSP];
extern int protcomst[MSP], firstprot, lastprot, protsave[PROT_SAVE_SIZE];

/* Variables for managing equivalence classes:
 * numecs - number of equivalence classes
 * nextecm - forward link of Equivalence Class members
 * ecgroup - class number or backward link of EC members
 * nummecs - number of meta-equivalence classes (used to compress
 *   templates)
 * tecfwd - forward link of meta-equivalence classes members
 * tecbck - backward link of MEC's
 */

/* Reserve enough room in the equivalence class arrays so that we
 * can use the CSIZE'th element to hold equivalence class information
 * for the NUL character.  Later we'll move this information into
 * the 0th element.
 */
extern int numecs, nextecm[CSIZE + 1], ecgroup[CSIZE + 1], nummecs;

/* Meta-equivalence classes are indexed starting at 1, so it's possible
 * that they will require positions from 1 .. CSIZE, i.e., CSIZE + 1
 * slots total (since the arrays are 0-based).  nextecm[] and ecgroup[]
 * don't require the extra position since they're indexed from 1 .. CSIZE - 1.
 */
extern int tecfwd[CSIZE + 1], tecbck[CSIZE + 1];

/* Variables for start conditions:
 * lastsc - last start condition created
 * current_max_scs - current limit on number of start conditions
 * scset - set of rules active in start condition
 * scbol - set of rules active only at the beginning of line in a s.c.
 * scxclu - true if start condition is exclusive
 * sceof - true if start condition has EOF rule
 * scname - start condition name
 */
extern int lastsc, *scset, *scbol, *scxclu, *sceof;
extern int current_max_scs;
extern char **scname;

/* Variables for dfa machine data:
 * current_max_dfa_size - current maximum number of NFA states in DFA
 * current_max_xpairs - current maximum number of non-template xtion pairs
 * current_max_template_xpairs - current maximum number of template pairs
 * current_max_dfas - current maximum number DFA states
 * lastdfa - last dfa state number created
 * nxt - state to enter upon reading character
 * chk - check value to see if "nxt" applies
 * tnxt - internal nxt table for templates
 * base - offset into "nxt" for given state
 * def - where to go if "chk" disallows "nxt" entry
 * nultrans - NUL transition for each state
 * NUL_ec - equivalence class of the NUL character
 * tblend - last "nxt/chk" table entry being used
 * firstfree - first empty entry in "nxt/chk" table
 * dss - nfa state set for each dfa
 * dfasiz - size of nfa state set for each dfa
 * dfaacc - accepting set for each dfa state (if using REJECT), or accepting
 *	number, if not
 * accsiz - size of accepting set for each dfa state
 * dhash - dfa state hash value
 * numas - number of DFA accepting states created; note that this
 *	is not necessarily the same value as num_rules, which is the analogous
 *	value for the NFA
 * numsnpairs - number of state/nextstate transition pairs
 * jambase - position in base/def where the default jam table starts
 * jamstate - state number corresponding to "jam" state
 * end_of_buffer_state - end-of-buffer dfa state number
 */
extern int current_max_dfa_size;
extern int current_max_xpairs;
extern int current_max_template_xpairs;
extern int current_max_dfas;
extern int lastdfa;
extern int * nxt;
extern int * chk;
extern int * tnxt;
extern int * base;
extern int * def;
extern int * nultrans;
extern int NUL_ec;
extern int tblend;
extern int firstfree;
extern int **dss;
extern int *dfasiz;

union dfaacc_union {
	int  * dfaacc_set;
	int    dfaacc_state;
};

extern union dfaacc_union * dfaacc;
extern int * accsiz;
extern int * dhash;
extern int numas;
extern int numsnpairs;
extern int jambase;
extern int jamstate;
extern int end_of_buffer_state;

/* Variables for ccl information:
 * lastccl - ccl index of the last created ccl
 * current_maxccls - current limit on the maximum number of unique ccl's
 * cclmap - maps a ccl index to its set pointer
 * ccllen - gives the length of a ccl
 * cclng - true for a given ccl if the ccl is negated
 * cclreuse - counts how many times a ccl is re-used
 * current_max_ccl_tbl_size - current limit on number of characters needed
 *	to represent the unique ccl's
 * ccltbl - holds the characters in each ccl - indexed by cclmap
 */
extern int lastccl;
extern int * cclmap;
extern int * ccllen;
extern int * cclng;
extern int cclreuse;
extern int current_maxccls;
extern int current_max_ccl_tbl_size;
extern uchar * ccltbl;

/* Variables for miscellaneous information:
 * nmstr - last NAME scanned by the scanner
 * sectnum - section number currently being parsed
 * nummt - number of empty nxt/chk table entries
 * hshcol - number of hash collisions detected by snstods
 * dfaeql - number of times a newly created dfa was equal to an old one
 * numeps - number of epsilon NFA states created
 * eps2 - number of epsilon states which have 2 out-transitions
 * num_reallocs - number of times it was necessary to SAlloc::R() a group
 *	  of arrays
 * tmpuses - number of DFA states that chain to templates
 * totnst - total number of NFA states used to make DFA states
 * peakpairs - peak number of transition pairs we had to store internally
 * numuniq - number of unique transitions
 * numdup - number of duplicate transitions
 * hshsave - number of hash collisions saved by checking number of states
 * num_backing_up - number of DFA states requiring backing up
 * bol_needed - whether scanner needs beginning-of-line recognition
 */
extern char nmstr[MAXLINE];
extern int sectnum;
extern int nummt;
extern int hshcol;
extern int dfaeql;
extern int numeps;
extern int eps2;
extern int num_reallocs;
extern int tmpuses;
extern int totnst;
extern int peakpairs;
extern int numuniq;
extern int numdup;
extern int hshsave;
extern int num_backing_up;
extern int bol_needed;

#ifndef HAVE_REALLOCARRAY
	void * reallocarray(void *, size_t, size_t);
#endif
void   * allocate_array(int, size_t);
void   * reallocate_array(void *, int, size_t);

#define allocate_integer_array(size) allocate_array(size, sizeof(int))
#define reallocate_integer_array(array,size) reallocate_array((void *) array, size, sizeof(int))
#define allocate_bool_array(size) allocate_array(size, sizeof(bool))
#define reallocate_bool_array(array,size) reallocate_array((void *) array, size, sizeof(bool))
#define allocate_int_ptr_array(size) allocate_array(size, sizeof(int *))
#define allocate_char_ptr_array(size) allocate_array(size, sizeof(char *))
#define allocate_dfaacc_union(size) allocate_array(size, sizeof(union dfaacc_union))
#define reallocate_int_ptr_array(array,size) reallocate_array((void *) array, size, sizeof(int *))
#define reallocate_char_ptr_array(array,size) reallocate_array((void *) array, size, sizeof(char *))
#define reallocate_dfaacc_union(array, size) reallocate_array((void *) array, size, sizeof(union dfaacc_union))
#define allocate_character_array(size) allocate_array( size, sizeof(char))
#define reallocate_character_array(array,size) reallocate_array((void *) array, size, sizeof(char))
#define allocate_Character_array(size) allocate_array(size, sizeof(uchar))
#define reallocate_Character_array(array,size) reallocate_array((void *) array, size, sizeof(uchar))

/* External functions that are cross-referenced among the flex source files. */

/* from file ccl.c */
extern void ccladd(int, int);	/* add a single character to a ccl */
extern int cclinit();	/* make an empty ccl */
extern void cclnegate(int);	/* negate a ccl */
extern int ccl_set_diff (int a, int b); /* set difference of two ccls. */
extern int ccl_set_union (int a, int b); /* set union of two ccls. */
extern void list_character_set(FILE *, int[]); /* List the members of a set of characters in CCL form. */

/* from file dfa.c */
extern void check_for_backing_up(int, int[]); /* Check a DFA state for backing up. */
extern void check_trailing_context(int *, int, int *, int); /* Check to see if NFA state set constitutes "dangerous" trailing context. */
extern int *epsclosure(int *, int *, int[], int *, int *); /* Construct the epsilon closure of a set of ndfa states. */
extern void increase_max_dfas(); /* Increase the maximum number of dfas. */
extern void ntod();	/* convert a ndfa to a dfa */
extern int snstods(int[], int, int[], int, int, int *); /* Converts a set of ndfa states into a dfa state. */

/* from file ecs.c */
extern void ccl2ecl(); /* Convert character classes to set of equivalence classes. */
extern int cre8ecs(int[], int[], int); /* Associate equivalence class numbers with class members. */
extern void mkeccl(uchar[], int, int[], int[], int, int); /* Update equivalence classes based on character class transitions. */
extern void mkechar(int, int[], int[]); /* Create equivalence class for single character. */

/* from file gen.c */
extern void do_indent();	/* indent to the current level */
extern void gen_backing_up(); /* Generate the code to keep backing-up information. */
extern void gen_bu_action(); /* Generate the code to perform the backing up. */
extern void genctbl(); /* Generate full speed compressed transition table. */
extern void gen_find_action(); /* Generate the code to find the action number. */
extern void genftbl();	/* generate full transition table */
extern void gen_next_compressed_state(char *); /* Generate the code to find the next compressed-table state. */
extern void gen_next_match(); /* Generate the code to find the next match. */
extern void gen_next_state(int); /* Generate the code to find the next state. */
extern void gen_NUL_trans(); /* Generate the code to make a NUL transition. */
extern void gen_start_state(); /* Generate the code to find the start state. */
extern void gentabs(); /* Generate data statements for the transition tables. */
extern void indent_put2s(const char *, const char *); /* Write out a formatted string at the current indentation level. */
extern void indent_puts(const char *); /* Write out a string + newline at the current indentation level. */
extern void make_tables();	/* generate transition tables */


/* from file main.c */
extern void check_options();
extern void flexend(int);
extern void usage();

/* from file misc.c */
extern void action_define(const char *defname, int value); /* Add a #define to the action file. */
extern void add_action(const char *new_text); /* Add the given text to the stored actions. */
extern int all_lower(char *); /* True if a string is all lower case. */
extern int all_upper(char *); /* True if a string is all upper case. */
extern int intcmp(const void *, const void *); /* Compare two integers for use by qsort. */
extern void check_char(int c); /* Check a character to make sure it's in the expected range. */
extern uchar clower(int); /* Replace upper-case letter to lower-case. */
extern char *xstrdup(const char *); /* strdup() that fails fatally on allocation failures. */
extern int cclcmp(const void *, const void *); /* Compare two characters for use by qsort with '\0' sorting last. */
extern void dataend(); /* Finish up a block of data declarations. */
extern void dataflush(); /* Flush generated data statements. */
extern void flexerror(const char *); /* Report an error message and terminate. */
extern void flexfatal(const char *); /* Report a fatal error message and terminate. */

/* Report a fatal error with a pinpoint, and terminate */
#if HAVE_DECL___FUNC__
#define flex_die(msg) \
    do { \
        fprintf (stderr, _("%s: fatal internal error at %s:%d (%s): %s\n"), program_name, __FILE__, (int)__LINE__, __func__,msg);\
        FLEX_EXIT(1);\
    }while(0)
#else /* ! HAVE_DECL___FUNC__ */
#define flex_die(msg) \
    do { \
        fprintf(stderr, _("%s: fatal internal error at %s:%d %s\n"), program_name, __FILE__, (int)__LINE__, msg);\
        FLEX_EXIT(1);\
    }while(0)
#endif /* ! HAVE_DECL___func__ */

/* Report an error message formatted  */
extern void lerr(const char *, ...)
#if defined(__GNUC__) && __GNUC__ >= 3
    __attribute__((__format__(__printf__, 1, 2)))
#endif
;

/* Like lerr, but also exit after displaying message. */
extern void lerr_fatal(const char *, ...)
#if defined(__GNUC__) && __GNUC__ >= 3
    __attribute__((__format__(__printf__, 1, 2)))
#endif
;

extern void line_directive_out(FILE *, int); /* Spit out a "#line" statement. */
extern void mark_defs1(); /* Mark the current position in the action array as the end of the section 1 user defs. */
extern void mark_prolog(); /* Mark the current position in the action array as the end of the prolog. */
extern void mk2data(int); /* Generate a data statment for a two-dimensional array. */
extern void mkdata(int);	/* generate a data statement */
extern int myctoi(const char *); /* Return the integer represented by a string of digits. */
extern uchar myesc(uchar[]); /* Return character corresponding to escape sequence. */

/* Output a (possibly-formatted) string to the generated scanner. */
extern void out(const char *);
extern void out_dec(const char *, int);
extern void out_dec2(const char *, int, int);
extern void out_hex(const char *, uint);
extern void out_str(const char *, const char *);
extern void out_str3(const char *, const char *, const char *, const char *);
extern void out_str_dec(const char *, const char *, int);
extern void outc(int);
extern void outn(const char *);
extern void out_m4_define(const char* def, const char* val);
extern char *readable_form(int); /* Return a printable version of the given character, which might be 8-bit. */
extern void skelout(); /* Write out one section of the skeleton file. */
extern void transition_struct_out(int, int); /* Output a yy_trans_info structure. */
extern void *yy_flex_xmalloc(int); /* Only needed when using certain broken versions of bison to build parse.c. */

/* from file nfa.c */
extern void add_accept(int, int); /* Add an accepting state to a machine. */
extern int copysingl(int, int); /* Make a given number of copies of a singleton machine. */
extern void dumpnfa(int); /* Debugging routine to write out an nfa. */
extern void finish_rule(int, int, int, int, int); /* Finish up the processing for a rule. */
extern int link_machines(int, int); /* Connect two machines together. */
/* Mark each "beginning" state in a machine as being a "normal" (i.e.,
 * not trailing context associated) state.
 */
extern void mark_beginning_as_normal(int);
extern int mkbranch(int, int); /* Make a machine that branches to two machines. */
extern int mkclos(int);	/* convert a machine into a closure */
extern int mkopt(int);	/* make a machine optional */
extern int mkor(int, int); /* Make a machine that matches either one of two machines. */
extern int mkposcl(int); /* Convert a machine into a positive closure. */
extern int mkrep(int, int, int);	/* make a replicated machine */
extern int mkstate(int); /* Create a state with a transition on a given symbol. */
extern void new_rule();	/* initialize for a new rule */

/* from file parse.y */
extern void build_eof_action(); /* Build the "<<EOF>>" action for the active start conditions. */
extern void format_pinpoint_message(const char *, const char *); /* Write out a message formatted with one string, pinpointing its location. */
extern void pinpoint_message(const char *); /* Write out a message, pinpointing its location. */
extern void line_warning(const char *, int); /* Write out a warning, pinpointing it at the given line. */
extern void line_pinpoint(const char *, int); /* Write out a message, pinpointing it at the given line. */
extern void format_synerr(const char *, const char *); /* Report a formatted syntax error. */
extern void synerr(const char *);	/* report a syntax error */
extern void format_warn(const char *, const char *);
extern void lwarn(const char *);	/* report a warning */
extern void yyerror(const char *);	/* report a parse error */
extern int yyparse();		/* the YACC parser */

/* from file scan.l */
extern int flexscan(); /* The Flex-generated scanner for flex. */
extern void set_input_file(char *); /* Open the given file (if NULL, stdin) for scanning. */

/* from file sym.c */
extern void cclinstal(char[], int); /* Save the text of a character class. */
extern int ccllookup(char[]); /* Lookup the number associated with character class. */
extern void ndinstal(const char *, char[]);	/* install a name definition */
extern char *ndlookup(const char *);	/* lookup a name definition */
extern void scextend(); /* Increase maximum number of SC's. */
extern void scinstal(const char *, int);	/* make a start condition */
extern int sclookup(const char *); /* Lookup the number associated with a start condition. */

/* from file tblcmp.c */
extern void bldtbl(int[], int, int, int, int); /* Build table entries for dfa state. */
extern void cmptmps();	/* compress template table entries */
extern void expand_nxt_chk();	/* increase nxt/chk arrays */
extern int find_table_space(int *, int); /* Finds a space in the table for a state to be placed. */
extern void inittbl();	/* initialize transition tables */

extern void mkdeftbl(); /* Make the default, "jam" table entries. */
extern void mk1tbl(int, int, int, int); /* Create table entries for a state (or state fragment) which has only one out-transition. */
extern void place_state(int *, int, int); /* Place a state into full speed transition table. */
extern void stack1(int, int, int, int); /* Save states with only one out-transition to be processed later. */

/* from file yylex.c */
extern int yylex();

/* A growable array. See buf.c. */
struct Buf {
	void   *elts;		/* elements. */
	int     nelts;		/* number of elements. */
	size_t  elt_size;	/* in bytes. */
	int     nmax;		/* max capacity of elements. */
};

extern void buf_init(struct Buf * buf, size_t elem_size);
extern void buf_destroy(struct Buf * buf);
extern struct Buf *buf_append(struct Buf * buf, const void *ptr, int n_elem);
extern struct Buf *buf_concat(struct Buf* dest, const struct Buf* src);
extern struct Buf *buf_strappend(struct Buf *, const char *str);
extern struct Buf *buf_strnappend(struct Buf *, const char *str, int nchars);
extern struct Buf *buf_strdefine(struct Buf * buf, const char *str, const char *def);
extern struct Buf *buf_prints(struct Buf *buf, const char *fmt, const char* s);
extern struct Buf *buf_m4_define(struct Buf *buf, const char* def, const char* val);
extern struct Buf *buf_m4_undefine(struct Buf *buf, const char* def);
extern struct Buf *buf_print_strings(struct Buf * buf, FILE* out);
extern struct Buf *buf_linedir(struct Buf *buf, const char* filename, int lineno);

extern struct Buf userdef_buf; /* a string buffer for #define's generated by user-options on cmd line. */
extern struct Buf defs_buf;    /* a char* buffer to save #define'd some symbols generated by flex. */
extern struct Buf yydmap_buf;  /* a string buffer to hold yydmap elements */
extern struct Buf m4defs_buf;  /* Holds m4 definitions. */
extern struct Buf top_buf;     /* contains %top code. String buffer. */
extern bool no_section3_escape; /* True if the undocumented option --unsafe-no-m4-sect3-escape was passed */

/* For blocking out code from the header file. */
#define OUT_BEGIN_CODE() outn("m4_ifdef( [[M4_YY_IN_HEADER]],,[[m4_dnl")
#define OUT_END_CODE()   outn("]])")

/* For setjmp/longjmp (instead of calling exit(2)). Linkage in main.c */
extern jmp_buf flex_main_jmp_buf;

//#define FLEX_EXIT(status) longjmp(flex_main_jmp_buf,(status)+1)
#define FLEX_EXIT(status) exit(status)

#ifndef SLIBINCLUDED
	// Removes all \n and \r chars from tail of str. returns str. 
	extern char * chomp(char *str);
#endif

/* ctype functions forced to return boolean */
#define b_isalnum(c) (isalnum(c)?true:false)
#define b_isalpha(c) (isalpha(c)?true:false)
#define b_isascii(c) (isascii(c)?true:false)
#define b_isblank(c) (isblank(c)?true:false)
#define b_iscntrl(c) (iscntrl(c)?true:false)
#define b_isdigit(c) (isdigit(c)?true:false)
#define b_isgraph(c) (isgraph(c)?true:false)
#define b_islower(c) (islower(c)?true:false)
#define b_isprint(c) (isprint(c)?true:false)
#define b_ispunct(c) (ispunct(c)?true:false)
#define b_isspace(c) (isspace(c)?true:false)
#define b_isupper(c) (isupper(c)?true:false)
#define b_isxdigit(c) (isxdigit(c)?true:false)

bool has_case(int c); /* return true if char is uppercase or lowercase. */
int reverse_case(int c); /* Change case of character if possible. */
bool range_covers_case (int c1, int c2); /* return false if [c1-c2] is ambiguous for a caseless scanner. */
/*
 *  From "filter.c"
 */

/** A single stdio filter to execute.
 *  The filter may be external, such as "sed", or it
 *  may be internal, as a function call.
 */
struct filter {
	int    (*filter_func)(struct filter*); /**< internal filter function */
	void * extra;         /**< extra data passed to filter_func */
	int     argc;         /**< arg count */
	const char ** argv;   /**< arg vector, \0-terminated */
	FILE* in_file;
	FILE* out_file;
	struct filter * next; /**< next filter or NULL */
};

/* output filter chain */
extern struct filter * output_chain;
extern struct filter *filter_create_ext (struct filter * chain, const char *cmd, ...);
struct filter *filter_create_int(struct filter *chain, int (*filter_func) (struct filter *), void *extra);
extern bool filter_apply_chain(struct filter * chain, FILE* in_file, FILE* out_file);
extern int filter_truncate(struct filter * chain, int max_len);
extern int filter_tee_header(struct filter *chain);
extern int filter_fix_linedirs(struct filter *chain);
extern int filter_m4_p(struct filter *chain);

extern char* add_tmp_dir(const char* tmp_file_name);
extern FILE* mkstempFILE(char *tmpl, const char *mode);
extern void unlinktemp();
/*
 * From "regex.c"
 */
extern regex_t regex_linedir;
extern regex_t regex_blank_line;

bool flex_init_regex();
void flex_regcomp(regex_t *preg, const char *regex, int cflags);
char * regmatch_dup(regmatch_t * m, const char *src);
char * regmatch_cpy(regmatch_t * m, char *dest, const char *src);
int regmatch_len(regmatch_t * m);
int regmatch_strtol(regmatch_t * m, const char *src, char **endptr, int base);
bool regmatch_empty(regmatch_t * m);

/* From "scanflags.h" */
typedef uint scanflags_t;
extern scanflags_t* _sf_stk;
extern size_t _sf_top_ix, _sf_max; /**< stack of scanner flags. */
#define _SF_CASE_INS   ((scanflags_t)0x0001)
#define _SF_DOT_ALL    ((scanflags_t)0x0002)
#define _SF_SKIP_WS    ((scanflags_t)0x0004)
#define sf_top()           (_sf_stk[_sf_top_ix])
#define sf_case_ins()      (sf_top() & _SF_CASE_INS)
#define sf_dot_all()       (sf_top() & _SF_DOT_ALL)
#define sf_skip_ws()       (sf_top() & _SF_SKIP_WS)
#define sf_set_case_ins(X)      ((X) ? (sf_top() |= _SF_CASE_INS) : (sf_top() &= ~_SF_CASE_INS))
#define sf_set_dot_all(X)       ((X) ? (sf_top() |= _SF_DOT_ALL)  : (sf_top() &= ~_SF_DOT_ALL))
#define sf_set_skip_ws(X)       ((X) ? (sf_top() |= _SF_SKIP_WS)  : (sf_top() &= ~_SF_SKIP_WS))
extern void sf_init();
extern void sf_push();
extern void sf_pop();

//#include "tables.h"
//#include "scanopt.h"
	#ifndef NO_SCANOPT_USAGE
		#ifdef HAVE_NCURSES_H
			#include <ncurses.h> // Used by scanopt_usage for pretty-printing
		#endif
	#endif
	#ifdef __cplusplus
	extern  "C" {
	#endif
	//
	// Error codes.
	//
	enum scanopt_err_t {
		SCANOPT_ERR_OPT_UNRECOGNIZED = -1, // Unrecognized option
		SCANOPT_ERR_OPT_AMBIGUOUS = -2,    // It matched more than one option name
		SCANOPT_ERR_ARG_NOT_FOUND = -3,    // The required arg was not found
		SCANOPT_ERR_ARG_NOT_ALLOWED = -4   // Option does not take an argument
	};
	//
	// flags passed to scanopt_init 
	//
	enum scanopt_flag_t {
		SCANOPT_NO_ERR_MSG = 0x01 /* Suppress printing to stderr. */
	};

	/* Specification for a single option. */
	struct optspec_t {
		const char * opt_fmt; /* e.g., "--foo=FILE", "-f FILE", "-n [NUM]" */
		int r_val;            /* Value to be returned by scanopt_ex(). */
		const char * desc;    /* Brief description of this option, or NULL. */
	};

	typedef struct optspec_t optspec_t;
	typedef void * scanopt_t; // Used internally by scanopt() to maintain state. Never modify these value directly

	/* Initializes scanner and checks option list for errors.
	 * Parameters:
	 *   options - Array of options.
	 *   argc    - Same as passed to main().
	 *   argv    - Same as passed to main(). First element is skipped.
	 *   flags   - Control behavior.
	 * Return:  A malloc'd pointer .
	 */
	scanopt_t * scanopt_init(const optspec_t * options, int argc, char ** argv, int flags);

	// Frees memory used by scanner. Always returns 0.
	int scanopt_destroy(scanopt_t * scanner);

	#ifndef NO_SCANOPT_USAGE
		/* Prints a usage message based on contents of optlist.
		 * Parameters:
		 *   scanner  - The scanner, already initialized with scanopt_init().
		 *   fp       - The file stream to write to.
		 *   usage    - Text to be prepended to option list. May be NULL.
		 * Return:  Always returns 0 (zero).
		 */
		int scanopt_usage(scanopt_t * scanner, FILE * fp, const char * usage);
	#endif

	/* Scans command-line options in argv[].
	 * Parameters:
	 *   scanner  - The scanner, already initialized with scanopt_init().
	 *   optarg   - Return argument, may be NULL. On success, it points to start of an argument.
	 *   optindex - Return argument, may be NULL.
	 *              On success or failure, it is the index of this option.
	 *              If return is zero, then optindex is the NEXT valid option index.
	 *
	 * Return:  > 0 on success. Return value is from optspec_t->rval.
	 *         == 0 if at end of options.
	 *          < 0 on error (return value is an error code).
	 *
	 */
	int scanopt(scanopt_t * scanner, char ** optarg, int * optindex);

	#ifdef __cplusplus
	}
	#endif
//
//#include "options.h"
	extern optspec_t flexopts[]; // @global

	enum flexopt_flag_t {
		// Use positive integers only, since they are return codes for scanopt.
		// Order is not important.
		OPT_7BIT = 1,
		OPT_8BIT,
		OPT_ALIGN,
		OPT_ALWAYS_INTERACTIVE,
		OPT_ARRAY,
		OPT_BACKUP,
		OPT_BATCH,
		OPT_BISON_BRIDGE,
		OPT_BISON_BRIDGE_LOCATIONS,
		OPT_CASE_INSENSITIVE,
		OPT_COMPRESSION,
		OPT_CPLUSPLUS,
		OPT_DEBUG,
		OPT_DEFAULT,
		OPT_DONOTHING,
		OPT_ECS,
		OPT_FAST,
		OPT_FULL,
		OPT_HEADER_FILE,
		OPT_HELP,
		OPT_HEX,
		OPT_INTERACTIVE,
		OPT_LEX_COMPAT,
		OPT_POSIX_COMPAT,
		OPT_MAIN,
		OPT_META_ECS,
		OPT_NEVER_INTERACTIVE,
		OPT_NO_ALIGN,
		OPT_NO_DEBUG,
		OPT_NO_DEFAULT,
		OPT_NO_ECS,
		OPT_NO_LINE,
		OPT_NO_MAIN,
		OPT_NO_META_ECS,
		OPT_NO_REENTRANT,
		OPT_NO_REJECT,
		OPT_NO_STDINIT,
		OPT_NO_UNPUT,
		OPT_NO_WARN,
		OPT_NO_YYGET_EXTRA,
		OPT_NO_YYGET_IN,
		OPT_NO_YYGET_LENG,
		OPT_NO_YYGET_LINENO,
		OPT_NO_YYGET_LLOC,
		OPT_NO_YYGET_LVAL,
		OPT_NO_YYGET_OUT,
		OPT_NO_YYGET_TEXT,
		OPT_NO_YYLINENO,
		OPT_NO_YYMORE,
		OPT_NO_YYSET_EXTRA,
		OPT_NO_YYSET_IN,
		OPT_NO_YYSET_LINENO,
		OPT_NO_YYSET_LLOC,
		OPT_NO_YYSET_LVAL,
		OPT_NO_YYSET_OUT,
		OPT_NO_YYWRAP,
		OPT_NO_YY_POP_STATE,
		OPT_NO_YY_PUSH_STATE,
		OPT_NO_YY_SCAN_BUFFER,
		OPT_NO_YY_SCAN_BYTES,
		OPT_NO_YY_SCAN_STRING,
		OPT_NO_YY_TOP_STATE,
		OPT_OUTFILE,
		OPT_PERF_REPORT,
		OPT_POINTER,
		OPT_PREFIX,
		OPT_PREPROCDEFINE,
		OPT_PREPROC_LEVEL,
		OPT_READ,
		OPT_REENTRANT,
		OPT_REJECT,
		OPT_SKEL,
		OPT_STACK,
		OPT_STDINIT,
		OPT_STDOUT,
		OPT_TABLES_FILE,
		OPT_TABLES_VERIFY,
		OPT_TRACE,
		OPT_NO_UNISTD_H,
		OPT_VERBOSE,
		OPT_VERSION,
		OPT_WARN,
		OPT_YYCLASS,
		OPT_YYLINENO,
		OPT_YYMORE,
		OPT_YYWRAP,
		OPT_WIN_COMPAT,
		OPT_NO_SECT3_ESCAPE,
	};
//
#endif /* not defined FLEXDEF_H */
