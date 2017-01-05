/*
 * $Id: eval.h,v 1.49 2016/03/04 04:58:03 sfeam Exp $
 */

/* GNUPLOT - eval.h */

/*[
 * Copyright 1999, 2004   Thomas Williams, Colin Kelley
 *
 * Permission to use, copy, and distribute this software and its
 * documentation for any purpose with or without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.
 *
 * Permission to modify the software is granted, but not the right to
 * distribute the complete modified source code.  Modifications are to
 * be distributed as patches to the released version.  Permission to
 * distribute binaries produced by compiling modified sources is granted,
 * provided you
 *   1. distribute the corresponding source modifications from the
 *    released version in the form of a patch file along with the binaries,
 *   2. add special version identification to distinguish your version
 *    in addition to the base release version number,
 *   3. provide your name and address as the primary contact for the
 *    support of your modified version, and
 *   4. retain our contact information in regard to use of the base
 *    software.
 * Permission to distribute the released version of the source code along
 * with corresponding source modifications in the form of a patch file is
 * granted with same provisions 2 through 4 for binary distributions.
 *
 * This software is provided "as is" without express or implied warranty
 * to the extent permitted by applicable law.
]*/

#ifndef GNUPLOT_EVAL_H
# define GNUPLOT_EVAL_H

/* #if... / #include / #define collection: */

#include "syscfg.h"
#include "gp_types.h"

//#include <stdio.h>		/* for FILE* */

#define STACK_DEPTH 250		/* maximum size of the execution stack */
#define MAX_AT_LEN 150		/* max number of entries in action table */

/* These are used by add_action() to index the subroutine list ft[] in eval.c */
enum operators {
    /* keep this in line with table in eval.c */
    PUSH, 
	PUSHC, 
	PUSHD1, 
	PUSHD2, 
	PUSHD, 
	POP,
    CALL, 
	CALLN, 
	SUM, 
	LNOT, 
	BNOT, 
	UMINUS,
    LOR, 
	LAND, 
	BOR, 
	XOR, 
	BAND, 
	EQ, 
	NE, 
	GT, 
	LT, 
	GE, 
	LE, 
    LEFTSHIFT, 
	RIGHTSHIFT, 
	PLUS, 
	MINUS,
    MULT, 
	DIV, 
	MOD, 
	POWER, 
	FACTORIAL, 
	BOOLE,
    DOLLARS,
    CONCATENATE, 
	EQS, 
	NES, 
	RANGE, 
	INDEX,
    ASSIGN,
    /* only jump operators go between jump and sf_start, for is_jump() */
    JUMP, 
	JUMPZ, 
	JUMPNZ, 
	JTERN, 
	SF_START,
    /* External function call */
#ifdef HAVE_EXTERNAL_FUNCTIONS
    CALLE,
#endif
    // functions specific to using spec 
    COLUMN, STRINGCOLUMN
};

#define is_jump(operator) ((operator) >=(int)JUMP && (operator) <(int)SF_START)

struct UdftEntry;
struct UdvtEntry;
//
// p-code argument 
//
union GpArgument {
	int    j_arg;   // offset for jump 
	t_value v_arg;  // constant value 
	UdvtEntry * udv_arg; // pointer to dummy variable 
	UdftEntry * udf_arg; // pointer to udf to execute 
#ifdef HAVE_EXTERNAL_FUNCTIONS
	struct exft_entry *exf_arg; /* pointer to external function */
#endif
};
//
// action table entry 
//
struct AtEntry {
    enum operators index;	/* index of p-code function */
    GpArgument arg;
};

struct AtType {
    int a_count; /* count of entries in .actions[] */
	AtEntry actions[MAX_AT_LEN]; /* will usually be less than MAX_AT_LEN is malloc()'d copy */
};
//
// user-defined function table entry 
//
struct UdftEntry {
	UdftEntry()
	{
		THISZERO();
	}
    UdftEntry * next_udf; // pointer to next udf in linked list 
    char * udf_name;   // name of this function entry 
    AtType * at;       // pointer to action table to execute 
    char * definition; // definition of function as typed 
    int    dummy_num;  // required number of input variables 
    t_value dummy_values[MAX_NUM_VAR]; // current value of dummy variables 
};
//
// user-defined variable table entry
//
struct UdvtEntry {
	void   Init(UdvtEntry * pNext, char * pName, DATA_TYPES typ)
	{
		next_udv = pNext;
		udv_name = pName;
		udv_value.Init(typ);
	}
    UdvtEntry *next_udv; // pointer to next value in linked list
    char * udv_name;   // name of this value entry */
    t_value udv_value; // value it has
};
//
// This type definition has to come after GpArgument has been declared.
//
typedef void (*FUNC_PTR)(GpArgument *arg);
//
// standard/internal function table entry
//
struct ft_entry {
    const char *f_name;		/* pointer to name of this function */
    FUNC_PTR func;		/* address of function to call */
};

class GpEval {
public:
	GpEval()
	{
		first_udv = &udv_pi;
		first_udf = NULL;
		udv_pi.Init(0, "pi", INTGR);
		udv_NaN = 0;
		udv_user_head= 0;
		undefined= false;
	}
	void   ClearUdfList();
	UdvtEntry * AddUdvByName(const char * key);
	UdvtEntry * GetUdvByName(const char * key) const;
	void   DelUdvByName(const char * key, bool wildcard);
	static RETSIGTYPE Fpe(int an_int);
	void   EvaluateAt(AtType * at_ptr, t_value * val_ptr);
	void   FillGpValString(char * var, const char * stringvalue);
	void   FillGpValInteger(char * var, int value);
	void   FillGpValFloat(char * var, double value);
	void   FillGpValComplex(char * var, double areal, double aimag);
	void   UpdateGpValVariables(int context);

	static const ft_entry ft[]; /* The table of builtin functions */
	UdftEntry *first_udf; /* user-def'd functions */
	UdvtEntry *first_udv; /* user-def'd variables */
	UdvtEntry udv_pi; /* 'pi' variable */
	UdvtEntry *udv_NaN; /* 'NaN' variable */
	UdvtEntry **udv_user_head; /* first udv that can be deleted */
	bool undefined;
private:
	void   UpdatePlotBounds();
};

extern GpEval GpE; // @global

double gp_exp(double x);
/* HBB 20010726: Moved these here, from util.h. */
double real(const t_value *);
int    real_int(const t_value *val);
double imag(const t_value *);
double magnitude(const t_value *);
double angle(const t_value *);
t_value * Gcomplex(t_value *, double, double);
t_value * Ginteger(t_value *, int);
t_value * Gstring(t_value *, char *);
t_value * pop_or_convert_from_string(t_value *);
t_value * gpfree_string(t_value *a);
void gpfree_array(t_value *a);
void reset_stack();
void check_stack();
bool more_on_stack();
t_value * pop(t_value *x);
void push(t_value *x);
void int_check(t_value * v);
void f_bool(GpArgument *x);
void f_jump(GpArgument *x);
void f_jumpz(GpArgument *x);
void f_jumpnz(GpArgument *x);
void f_jtern(GpArgument *x);
void execute_at(AtType *at_ptr);
//void evaluate_at(AtType *at_ptr, t_value *val_ptr);
void free_at(AtType *at_ptr);
//UdvtEntry * GpE.AddUdvByName(const char *key);
//UdvtEntry * get_udv_by_name(const char *key);
//void del_udv_by_name(const char * key, bool isWildcard);
//void clear_udf_list();
// update GPVAL_ variables available to user 
//void update_gpval_variables(int from_plot_command);
// note: the routines below work for any variable name, not just those beginning GPVAL_ 
//void fill_gpval_string(char *var, const char *value);
//void fill_gpval_integer(char *var, int value);
//void fill_gpval_float(char *var, double value);
//void fill_gpval_complex(char *var, double areal, double aimag);
// C-callable versions of internal gnuplot functions word() and words()
char * gp_word(char *string, int i);
int gp_words(char *string);

#endif /* GNUPLOT_EVAL_H */
