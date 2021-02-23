// GNUPLOT - parse.c 
// Copyright 1986 - 1993, 1998, 2004   Thomas Williams, Colin Kelley
//
#include <gnuplot.h>
#pragma hdrstop
// 
// Protection mechanism for trying to parse a string followed by a + or - sign.
// Also suppresses an undefined variable message if an unrecognized token
// is encountered during try_to_get_string().
// 
bool string_result_only = FALSE;
static int parse_recursion_level;
// Exported globals: the current 'dummy' variable names 
char c_dummy_var[MAX_NUM_VAR][MAX_ID_LEN+1];
char set_dummy_var[MAX_NUM_VAR][MAX_ID_LEN+1] = { "x", "y" };
int fit_dummy_var[MAX_NUM_VAR];
bool scanning_range_in_progress = FALSE;
int at_highest_column_used = -1; /* This is used by plot_option_using() */
bool parse_1st_row_as_headers = FALSE; /* This is checked by df_readascii() */
udvt_entry * df_array = NULL; /* This is used by df_open() and df_readascii() */
/* Iteration structures used for bookkeeping */
GpIterator * plot_iterator = NULL;
GpIterator * set_iterator = NULL;
//
// Internal prototypes: 
//
static void extend_at();
static union argument * add_action(enum operators sf_index);
static void set_up_columnheader_parsing(struct at_entry * previous);
static bool no_iteration(GpIterator *);
//
// Internal variables: 
//
static at_type * P_At/*at*/ = NULL; // @global
static int AtSize = 0;

//static void convert(GpValue * pVal, int t_num)
void GnuPlot::Convert(GpValue * pVal, int t_num) const
{
	*pVal = Pgm.P_Token[t_num].l_val;
}

//intgr_t int_expression()
intgr_t GnuPlot::IntExpression()
{
	return (intgr_t)RealExpression();
}

//double GPO.RealExpression()
double GnuPlot::RealExpression()
{
	GpValue a;
	double result = real(ConstExpress(&a));
	a.Destroy();
	return result;
}

float GnuPlot::FloatExpression()
{
	GpValue a;
	double result = real(ConstExpress(&a));
	a.Destroy();
	return static_cast<float>(result);
}

void parse_reset_after_error()
{
	string_result_only = FALSE;
	parse_recursion_level = 0;
}
// 
// JW 20051126:
// Wrapper around const_express() called by try_to_get_string().
// Disallows top level + and - operators.
// This enables things like set xtics ('-\pi' -pi, '-\pi/2' -pi/2.)
// 
//GpValue * const_string_express(GpValue * pVal)
GpValue * GnuPlot::ConstStringExpress(GpValue * pVal)
{
	string_result_only = TRUE;
	ConstExpress(pVal);
	string_result_only = FALSE;
	return pVal;
}

//GpValue * const_express(GpValue * pVal)
GpValue * FASTCALL GnuPlot::ConstExpress(GpValue * pVal)
{
	int tkn = Pgm.GetCurTokenIdx();
	if(Pgm.EndOfCommand())
		IntErrorCurToken("constant expression required");
	// div - no dummy variables in a constant expression 
	dummy_func = NULL;
	EvaluateAt(TempAt(), pVal); // run it and send answer back 
	if(Ev.IsUndefined_)
		IntError(tkn, "undefined value");
	return pVal;
}
// 
// Used by plot2d/plot3d/stats/fit:
// Parse an expression that may return a filename string, a datablock name,
// a constant, or a dummy function using dummy variables x, y, ...
// If any dummy variables are present, set (*atptr) to point to an action table
// corresponding to the parsed expression, and return NULL.
// Otherwise evaluate the expression and return a string if there is one.
// The return value "str" and "*atptr" both point to locally-managed memory,
// which must not be freed by the caller!
// 
//char * string_or_express(at_type ** ppAt)
char * GnuPlot::StringOrExpress(at_type ** ppAt)
{
	int i;
	bool has_dummies;
	static char * array_placeholder = "@@";
	static char * str = NULL;
	ZFREE(str);
	df_array = NULL;
	ASSIGN_PTR(ppAt, NULL);
	if(Pgm.EndOfCommand())
		IntErrorCurToken("expression expected");
	// parsing for datablocks 
	if(Pgm.EqualsCur("$"))
		return Pgm.ParseDatablockName();
	// special keywords 
	if(Pgm.EqualsCur("keyentry"))
		return NULL;
	if(Pgm.IsString(Pgm.GetCurTokenIdx()) && (str = TryToGetString()))
		return str;
	// If this is a bare array name for an existing array, store a pointer
	// for df_open() to use.  "@@" is a magic pseudo-filename passed to
	// df_open() that tells it to use the stored pointer.
	if(Pgm.TypeUdv(Pgm.GetCurTokenIdx()) == ARRAY && !Pgm.EqualsNext("[")) {
		df_array = AddUdv(Pgm.GetCurTokenIdx());
		Pgm.Shift();
		return array_placeholder;
	}
	// parse expression 
	TempAt();
	// check if any dummy variables are used 
	has_dummies = FALSE;
	for(i = 0; i < P_At->a_count; i++) {
		enum operators op_index = P_At->actions[i].index;
		if(oneof4(op_index, PUSHD1, PUSHD2, PUSHD, SUM)) {
			has_dummies = TRUE;
			break;
		}
	}
	if(!has_dummies) {
		// no dummy variables: evaluate expression 
		GpValue val;
		EvaluateAt(P_At, &val);
		if(!Ev.IsUndefined_ && val.type == STRING) {
			// prevent empty string variable from treated as special file '' or "" 
			if(*val.v.string_val == '\0') {
				SAlloc::F(val.v.string_val);
				str = sstrdup(" ");
			}
			else
				str = val.v.string_val;
		}
	}
	// prepare return 
	ASSIGN_PTR(ppAt, P_At);
	return str;
}
//
// build an action table and return its pointer, but keep a pointer in at
// so that we can free it later if the caller hasn't taken over management of this table.
//
//at_type * temp_at()
at_type * GnuPlot::TempAt()
{
	if(P_At)
		free_at(P_At);
	P_At = (at_type *)SAlloc::M(sizeof(struct at_type));
	memzero(P_At, sizeof(*P_At));     /* reset action table !!! */
	AtSize = MAX_AT_LEN;
	parse_recursion_level = 0;
	ParseExpression();
	return (P_At);
}
//
// build an action table, put it in dynamic memory, and return its pointer 
//
//at_type * perm_at()
at_type * GnuPlot::PermAt()
{
	TempAt();
	size_t len = sizeof(at_type) + (P_At->a_count - MAX_AT_LEN) * sizeof(struct at_entry);
	at_type * at_ptr = (at_type *)SAlloc::R(P_At, len);
	P_At = NULL; // invalidate at pointer 
	return (at_ptr);
}

/* Create an action table that describes a call to column("string"). */
/* This is used by plot_option_using() to handle 'plot ... using "string"' */
struct at_type * create_call_column_at(char * string)                  
{
	at_type * at = (at_type *)SAlloc::M(sizeof(int) + 2*sizeof(at_entry));
	at->a_count = 2;
	at->actions[0].index = PUSHC;
	at->actions[0].arg.j_arg = 3;   /* FIXME - magic number! */
	at->actions[0].arg.v_arg.type = STRING;
	at->actions[0].arg.v_arg.v.string_val = string;
	at->actions[1].index = COLUMN;
	at->actions[1].arg.j_arg = 0;
	return (at);
}
// 
// Create an action table that describes a call to columnhead(-1). 
// This is substituted for the bare keyword "colummhead" 
// 
at_type * create_call_columnhead()                 
{
	at_type * p_at = (at_type *)SAlloc::M(sizeof(int) + 2*sizeof(at_entry));
	p_at->a_count = 2;
	p_at->actions[0].index = PUSHC;
	p_at->actions[0].arg.j_arg = 3;   /* FIXME - magic number! */
	p_at->actions[0].arg.v_arg.type = INTGR;
	p_at->actions[0].arg.v_arg.v.int_val = -1;
	p_at->actions[1].index = COLUMNHEAD;
	p_at->actions[1].arg.j_arg = 0;
	return (p_at);
}

static void extend_at()
{
	size_t newsize = sizeof(at_type) + AtSize * sizeof(at_entry);
	P_At = (at_type *)SAlloc::R(P_At, newsize);
	AtSize += MAX_AT_LEN;
	FPRINTF((stderr, "Extending at size to %d\n", AtSize));
}
//
// Add function number <sf_index> to the current action table 
//
static union argument * add_action(enum operators sf_index)                        
{
	if(P_At->a_count >= AtSize) {
		extend_at();
	}
	P_At->actions[P_At->a_count].index = sf_index;
	return (&(P_At->actions[P_At->a_count++].arg));
}
// 
// For external calls to parse_expressions()
// parse_recursion_level is expected to be 0 
// 
//static void parse_expression()
void GnuPlot::ParseExpression()
{                               /* full expressions */
	if(ParseAssignmentExpression())
		return;
	parse_recursion_level++;
	AcceptLogicalOrExpression();
	ParseConditionalExpression();
	parse_recursion_level--;
}

//static void accept_logical_OR_expression()
void GnuPlot::AcceptLogicalOrExpression()
{                               /* ? : expressions */
	AcceptLogicalAndExpression();
	ParseLogicalOrExpression();
}

//static void accept_logical_AND_expression()
void GnuPlot::AcceptLogicalAndExpression()
{
	AcceptInclusiveOrExpression();
	ParseLogicalAndExpression();
}

//static void accept_inclusive_OR_expression()
void GnuPlot::AcceptInclusiveOrExpression()
{
	AcceptExclusiveOrExpression();
	ParseInclusiveOrExpression();
}

//static void accept_exclusive_OR_expression()
void GnuPlot::AcceptExclusiveOrExpression()
{
	AcceptAndExpression();
	ParseExclusiveOrExpression();
}

//static void accept_AND_expression()
void GnuPlot::AcceptAndExpression()
{
	AcceptEqualityExpression();
	ParseAndExpression();
}

//static void accept_equality_expression()
void GnuPlot::AcceptEqualityExpression()
{
	AcceptRelationalExpression();
	ParseEqualityExpression();
}

//static void accept_relational_expression()
void GnuPlot::AcceptRelationalExpression()
{
	AcceptBitshiftExpression();
	ParseRelationalExpression();
}

//static void accept_bitshift_expression()
void GnuPlot::AcceptBitshiftExpression()
{
	AcceptAdditiveExpression();
	ParseBitshiftExpression();
}

//static void accept_additive_expression()
void GnuPlot::AcceptAdditiveExpression()
{
	AcceptMultiplicativeExpression();
	ParseAdditiveExpression();
}

//static void accept_multiplicative_expression()
void GnuPlot::AcceptMultiplicativeExpression()
{
	ParseUnaryExpression(); /* - things */
	ParseMultiplicativeExpression(); /* * / % */
}

//static int parse_assignment_expression()
int GnuPlot::ParseAssignmentExpression()
{
	// Check for assignment operator Var = <expr> 
	if(Pgm.IsLetter(Pgm.GetCurTokenIdx()) && Pgm.Equals(Pgm.GetCurTokenIdx()+1, "=")) {
		// push the variable name 
		union argument * foo = add_action(PUSHC);
		char * varname = NULL;
		Pgm.MCapture(&varname, Pgm.GetCurTokenIdx(), Pgm.GetCurTokenIdx());
		foo->v_arg.type = STRING;
		foo->v_arg.v.string_val = varname;
		// push a dummy variable that would be the index if this were an array 
		// FIXME: It would be nice to hide this from "show at" 
		foo = add_action(PUSHC);
		foo->v_arg.SetNotDefined();
		// push the expression whose value it will get 
		Pgm.Shift();
		Pgm.Shift();
		ParseExpression();
		// push the actual assignment operation 
		add_action(ASSIGN);
		return 1;
	}
	// Check for assignment to an array element Array[<expr>] = <expr> 
	if(ParseArrayAssignmentExpression())
		return 1;
	return 0;
}
/*
 * If an array assignment is the first thing on a command line it is handled by
 * the separate routine array_assignment().
 * Here we catch assignments that are embedded in an expression.
 * Examples:
 *	print A[2] = foo
 *	A[1] = A[2] = A[3] = 0
 */
//static int parse_array_assignment_expression()
int GnuPlot::ParseArrayAssignmentExpression()
{
	// Check for assignment to an array element Array[<expr>] = <expr> 
	if(Pgm.IsLetter(Pgm.GetCurTokenIdx()) && Pgm.EqualsNext("[")) {
		char * varname = NULL;
		union argument * foo;
		int save_action, save_token;
		/* Quick check for the most common false positives */
		/* i.e. other constructs that begin with "name["   */
		/* FIXME: quicker than the full test below, but do we care? */
		if(Pgm.Equals(Pgm.GetCurTokenIdx()+3, "]") && !Pgm.Equals(Pgm.GetCurTokenIdx()+4, "="))
			return 0;
		if(Pgm.Equals(Pgm.GetCurTokenIdx()+3, ":")) /* substring s[foo:baz] */
			return 0;
		// Is this really a known array name? 
		if(Pgm.TypeUdv(Pgm.GetCurTokenIdx()) != ARRAY)
			return 0;
		// Save state of the action table and the command line 
		save_action = P_At->a_count;
		save_token = Pgm.GetCurTokenIdx();
		// push the array name 
		Pgm.MCapture(&varname, Pgm.GetCurTokenIdx(), Pgm.GetCurTokenIdx());
		foo = add_action(PUSHC);
		foo->v_arg.type = STRING;
		foo->v_arg.v.string_val = varname;
		// push the index 
		Pgm.Shift();
		Pgm.Shift();
		ParseExpression();
		/* If this wasn't really an array element assignment, back out. */
		/* NB: Depending on what we just parsed, this may leak memory.  */
		if(!Pgm.EqualsCur("]") || !Pgm.EqualsNext("=")) {
			Pgm.SetTokenIdx(save_token);
			P_At->a_count = save_action;
			SAlloc::F(varname);
			return 0;
		}
		// Now we evaluate the expression whose value it will get 
		Pgm.Shift();
		Pgm.Shift();
		ParseExpression();
		// push the actual assignment operation 
		add_action(ASSIGN);
		return 1;
	}
	return 0;
}
// 
// add action table entries for primary expressions, i.e. either a
// parenthesized expression, a variable name, a numeric constant, a
// function evaluation, a power operator or postfix '!' (factorial)
// expression.
// Sep 2016 cardinality expression |Array| 
// 
//static void parse_primary_expression()
void GnuPlot::ParsePrimaryExpression()
{
	if(Pgm.EqualsCur("(")) {
		Pgm.Shift();
		ParseExpression();
		// Expressions may be separated by a comma 
		while(Pgm.EqualsCur(",")) {
			Pgm.Shift();
			add_action(POP);
			ParseExpression();
		}
		if(!Pgm.EqualsCur(")"))
			IntErrorCurToken("')' expected");
		Pgm.Shift();
	}
	else if(Pgm.EqualsCur("$")) {
		GpValue a;
		Pgm.Shift();
		if(!Pgm.IsANumber(Pgm.GetCurTokenIdx())) {
			if(Pgm.EqualsNext("[")) {
				udvt_entry * datablock_udv;
				Pgm.Rollback();
				datablock_udv = Ev.GetUdvByName(Pgm.ParseDatablockName());
				if(!datablock_udv)
					IntError(Pgm.GetCurTokenIdx()-2, "No such datablock");
				add_action(PUSH)->udv_arg = datablock_udv;
			}
			else
				IntErrorCurToken("Column number or datablock line expected");
		}
		else {
			Convert(&a, Pgm.GetCurTokenIdx());
			Pgm.Shift();
			if(a.type != INTGR || a.v.int_val < 0)
				IntErrorCurToken("Positive integer expected");
			if(at_highest_column_used < a.v.int_val)
				at_highest_column_used = a.v.int_val;
			add_action(DOLLARS)->v_arg = a;
		}
	}
	else if(Pgm.EqualsCur("|")) {
		udvt_entry * udv;
		Pgm.Shift();
		if(Pgm.EqualsCur("$")) {
			udv = Ev.GetUdvByName(Pgm.ParseDatablockName());
			if(!udv)
				IntError(Pgm.GetPrevTokenIdx(), "no such datablock");
		}
		else {
			udv = AddUdv(Pgm.GetCurTokenIdx());
			Pgm.Shift();
			if(udv->udv_value.type != ARRAY)
				IntError(Pgm.GetPrevTokenIdx(), "not an array");
		}
		add_action(PUSH)->udv_arg = udv;
		if(!Pgm.EqualsCur("|"))
			IntErrorCurToken("'|' expected");
		Pgm.Shift();
		add_action(CARDINALITY);
	}
	else if(Pgm.IsANumber(Pgm.GetCurTokenIdx())) {
		union argument * foo = add_action(PUSHC);
		Convert(&(foo->v_arg), Pgm.GetCurTokenIdx());
		Pgm.Shift();
	}
	else if(Pgm.IsLetter(Pgm.GetCurTokenIdx())) {
		// Found an identifier --- check whether its a function or a
		// variable by looking for the parentheses of a function argument list 
		if(Pgm.EqualsNext("(")) {
			enum operators whichfunc = (enum operators)IsBuiltinFunction(Pgm.GetCurTokenIdx());
			GpValue num_params;
			num_params.type = INTGR;
			if(whichfunc) {
				/* skip fnc name and '(' */
				Pgm.Shift();
				Pgm.Shift();
				ParseExpression(); // parse fnc argument 
				num_params.v.int_val = 1;
				while(Pgm.EqualsCur(",")) {
					Pgm.Shift();
					num_params.v.int_val++;
					ParseExpression();
				}
				if(!Pgm.EqualsCur(")"))
					IntErrorCurToken("')' expected");
				Pgm.Shift();
				// The sprintf built-in function has a variable number of arguments 
				if(!strcmp(_FuncTab2[whichfunc].P_Name, "sprintf"))
					add_action(PUSHC)->v_arg = num_params;
				// v4 timecolumn only had 1 param; v5 has 2. Accept either 
				if(!strcmp(_FuncTab2[whichfunc].P_Name, "timecolumn"))
					add_action(PUSHC)->v_arg = num_params;
				// The column() function has side effects requiring special handling 
				if(!strcmp(_FuncTab2[whichfunc].P_Name, "column")) {
					set_up_columnheader_parsing(&(P_At->actions[P_At->a_count-1]) );
				}
				add_action(whichfunc);
			}
			else {
				// it's a call to a user-defined function 
				enum operators call_type = (enum operators)CALL;
				int tok = Pgm.GetCurTokenIdx();
				// skip func name and '(' 
				Pgm.Shift();
				Pgm.Shift();
				ParseExpression();
				if(Pgm.EqualsCur(",")) { /* more than 1 argument? */
					num_params.v.int_val = 1;
					while(Pgm.EqualsCur(",")) {
						num_params.v.int_val += 1;
						Pgm.Shift();
						ParseExpression();
					}
					add_action(PUSHC)->v_arg = num_params;
					call_type = (enum operators)CALLN;
				}
				if(!Pgm.EqualsCur(")"))
					IntErrorCurToken("')' expected");
				Pgm.Shift();
				add_action(call_type)->udf_arg = AddUdf(tok);
			}
		}
		else if(Pgm.EqualsCur("sum") && Pgm.EqualsNext("[")) {
			ParseSumExpression();
			// dummy_func==NULL is a flag to say no dummy variables active 
		}
		else if(dummy_func) {
			if(Pgm.EqualsCur(c_dummy_var[0])) {
				Pgm.Shift();
				add_action(PUSHD1)->udf_arg = dummy_func;
				fit_dummy_var[0]++;
			}
			else if(Pgm.EqualsCur(c_dummy_var[1])) {
				Pgm.Shift();
				add_action(PUSHD2)->udf_arg = dummy_func;
				fit_dummy_var[1]++;
			}
			else {
				int param = 0;
				for(int i = 2; i < MAX_NUM_VAR; i++) {
					if(Pgm.EqualsCur(c_dummy_var[i])) {
						GpValue num_params;
						num_params.type = INTGR;
						num_params.v.int_val = i;
						param = 1;
						Pgm.Shift();
						add_action(PUSHC)->v_arg = num_params;
						add_action(PUSHD)->udf_arg = dummy_func;
						fit_dummy_var[i]++;
						break;
					}
				}
				if(!param) { // defined variable 
					add_action(PUSH)->udv_arg = AddUdv(Pgm.GetCurTokenIdx());
					Pgm.Shift();
				}
			}
			// its a variable, with no dummies active - div 
		}
		else {
			add_action(PUSH)->udv_arg = AddUdv(Pgm.GetCurTokenIdx());
			Pgm.Shift();
		}
	}
	/* end if letter */
	// Maybe it's a string constant 
	else if(Pgm.IsString(Pgm.GetCurTokenIdx())) {
		union argument * foo = add_action(PUSHC);
		foo->v_arg.type = STRING;
		foo->v_arg.v.string_val = NULL;
		// this dynamically allocated string will be freed by free_at() 
		Pgm.MQuoteCapture(&(foo->v_arg.v.string_val), Pgm.GetCurTokenIdx(), Pgm.GetCurTokenIdx());
		Pgm.Shift();
	}
	else {
		IntErrorCurToken("invalid expression ");
	}
	// The remaining operators are postfixes and can be stacked, e.g. 
	// Array[i]**2, so we may have to loop to catch all of them.      
	while(TRUE) {
		// add action code for ! (factorial) operator 
		if(Pgm.EqualsCur("!")) {
			Pgm.Shift();
			add_action(FACTORIAL);
		}
		// add action code for ** operator 
		else if(Pgm.EqualsCur("**")) {
			Pgm.Shift();
			ParseUnaryExpression();
			add_action(POWER);
		}
		// Parse and add actions for range specifier applying to previous entity.
		// Currently the [beg:end] form is used to generate substrings, but could
		// also be used to extract vector slices.  The [i] form is used to index
		// arrays, but could also be a shorthand for extracting a single-character substring.
		else if(Pgm.EqualsCur("[") && !Pgm.IsANumber(Pgm.GetPrevTokenIdx())) {
			// handle '*' or empty start of range 
			Pgm.Shift();
			if(Pgm.EqualsCur("*") || Pgm.EqualsCur(":")) {
				union argument * empty = add_action(PUSHC);
				empty->v_arg.type = INTGR;
				empty->v_arg.v.int_val = 1;
				if(Pgm.EqualsCur("*"))
					Pgm.Shift();
			}
			else
				ParseExpression();
			// handle array indexing (single value in square brackets) 
			if(Pgm.EqualsCur("]")) {
				Pgm.Shift();
				add_action(INDEX);
				continue;
			}
			if(!Pgm.EqualsCur(":"))
				IntErrorCurToken("':' expected");
			// handle '*' or empty end of range 
			if(Pgm.Equals(++Pgm.CToken, "*") || Pgm.EqualsCur("]")) {
				union argument * empty = add_action(PUSHC);
				empty->v_arg.type = INTGR;
				empty->v_arg.v.int_val = 65535; /* should be INT_MAX */
				if(Pgm.EqualsCur("*"))
					Pgm.Shift();
			}
			else
				ParseExpression();
			if(!Pgm.EqualsCur("]"))
				IntErrorCurToken("']' expected");
			Pgm.Shift();
			add_action(RANGE);
			// Whatever this is, it isn't another postfix operator 
		}
		else {
			break;
		}
	}
}
// 
// HBB 20010309: Here and below: can't store pointers into the middle
// of at->actions[]. That array may be realloc()ed by add_action() or
// express() calls!. Access via index savepc1/savepc2, instead. 
// 
//static void parse_conditional_expression()
void GnuPlot::ParseConditionalExpression()
{
	// create action code for ? : expressions 
	if(Pgm.EqualsCur("?")) {
		int savepc1, savepc2;
		/* Fake same recursion level for alternatives
		 *   set xlabel a>b ? 'foo' : 'bar' -1, 1
		 * FIXME: This won't work:
		 *   set xlabel a-b>c ? 'foo' : 'bar'  offset -1, 1
		 */
		parse_recursion_level--;
		Pgm.Shift();
		savepc1 = P_At->a_count;
		add_action(JTERN);
		ParseExpression();
		if(!Pgm.EqualsCur(":"))
			IntErrorCurToken("expecting ':'");
		Pgm.Shift();
		savepc2 = P_At->a_count;
		add_action(JUMP);
		P_At->actions[savepc1].arg.j_arg = P_At->a_count - savepc1;
		ParseExpression();
		P_At->actions[savepc2].arg.j_arg = P_At->a_count - savepc2;
		parse_recursion_level++;
	}
}

//static void parse_logical_OR_expression()
void GnuPlot::ParseLogicalOrExpression()
{
	// create action codes for || operator 
	while(Pgm.EqualsCur("||")) {
		int savepc;
		Pgm.Shift();
		savepc = P_At->a_count;
		add_action(JUMPNZ); /* short-circuit if already TRUE */
		AcceptLogicalAndExpression();
		// offset for jump 
		P_At->actions[savepc].arg.j_arg = P_At->a_count - savepc;
		add_action(BOOLE);
	}
}

//static void parse_logical_AND_expression()
void GnuPlot::ParseLogicalAndExpression()
{
	// create action code for && operator 
	while(Pgm.EqualsCur("&&")) {
		int savepc;
		Pgm.Shift();
		savepc = P_At->a_count;
		add_action(JUMPZ); /* short-circuit if already FALSE */
		AcceptInclusiveOrExpression();
		P_At->actions[savepc].arg.j_arg = P_At->a_count - savepc; /* offset for jump */
		add_action(BOOLE);
	}
}

//static void parse_inclusive_OR_expression()
void GnuPlot::ParseInclusiveOrExpression()
{
	// create action code for | operator 
	while(Pgm.EqualsCur("|")) {
		Pgm.Shift();
		AcceptExclusiveOrExpression();
		add_action(BOR);
	}
}

//static void parse_exclusive_OR_expression()
void GnuPlot::ParseExclusiveOrExpression()
{
	// create action code for ^ operator 
	while(Pgm.EqualsCur("^")) {
		Pgm.Shift();
		AcceptAndExpression();
		add_action(XOR);
	}
}

//static void parse_AND_expression()
void GnuPlot::ParseAndExpression()
{
	// create action code for & operator 
	while(Pgm.EqualsCur("&")) {
		Pgm.Shift();
		AcceptEqualityExpression();
		add_action(BAND);
	}
}

//static void parse_equality_expression()
void GnuPlot::ParseEqualityExpression()
{
	// create action codes for == and != numeric operators eq and ne string operators 
	while(TRUE) {
		if(Pgm.EqualsCur("==")) {
			Pgm.Shift();
			AcceptRelationalExpression();
			add_action(EQ);
		}
		else if(Pgm.EqualsCur("!=")) {
			Pgm.Shift();
			AcceptRelationalExpression();
			add_action(NE);
		}
		else if(Pgm.EqualsCur("eq")) {
			Pgm.Shift();
			AcceptRelationalExpression();
			add_action(EQS);
		}
		else if(Pgm.EqualsCur("ne")) {
			Pgm.Shift();
			AcceptRelationalExpression();
			add_action(NES);
		}
		else
			break;
	}
}

//static void parse_relational_expression()
void GnuPlot::ParseRelationalExpression()
{
	// create action code for < > >= or <= operators 
	while(TRUE) {
		if(Pgm.EqualsCur(">")) {
			Pgm.Shift();
			AcceptBitshiftExpression();
			add_action(GT);
		}
		else if(Pgm.EqualsCur("<")) {
			// Workaround for * in syntax of range constraints  
			if(scanning_range_in_progress && Pgm.EqualsNext("*") ) {
				break;
			}
			Pgm.Shift();
			AcceptBitshiftExpression();
			add_action(LT);
		}
		else if(Pgm.EqualsCur(">=")) {
			Pgm.Shift();
			AcceptBitshiftExpression();
			add_action(GE);
		}
		else if(Pgm.EqualsCur("<=")) {
			Pgm.Shift();
			AcceptBitshiftExpression();
			add_action(LE);
		}
		else
			break;
	}
}

//static void parse_bitshift_expression()
void GnuPlot::ParseBitshiftExpression()
{
	// create action codes for << and >> operators 
	while(TRUE) {
		if(Pgm.EqualsCur("<<")) {
			Pgm.Shift();
			AcceptAdditiveExpression();
			add_action(LEFTSHIFT);
		}
		else if(Pgm.EqualsCur(">>")) {
			Pgm.Shift();
			AcceptAdditiveExpression();
			add_action(RIGHTSHIFT);
		}
		else
			break;
	}
}

//static void parse_additive_expression()
void GnuPlot::ParseAdditiveExpression()
{
	// create action codes for +, - and . operators 
	while(TRUE) {
		if(Pgm.EqualsCur(".")) {
			Pgm.Shift();
			AcceptMultiplicativeExpression();
			add_action(CONCATENATE);
			// If only string results are wanted do not accept '-' or '+' at the top level. 
		}
		else if(string_result_only && parse_recursion_level == 1) {
			break;
		}
		else if(Pgm.EqualsCur("+")) {
			Pgm.Shift();
			AcceptMultiplicativeExpression();
			add_action(PLUS);
		}
		else if(Pgm.EqualsCur("-")) {
			Pgm.Shift();
			AcceptMultiplicativeExpression();
			add_action(MINUS);
		}
		else
			break;
	}
}

//static void parse_multiplicative_expression()
void GnuPlot::ParseMultiplicativeExpression()
{
	// add action code for * / and % operators 
	while(TRUE) {
		if(Pgm.EqualsCur("*")) {
			Pgm.Shift();
			ParseUnaryExpression();
			add_action(MULT);
		}
		else if(Pgm.EqualsCur("/")) {
			Pgm.Shift();
			ParseUnaryExpression();
			add_action(DIV);
		}
		else if(Pgm.EqualsCur("%")) {
			Pgm.Shift();
			ParseUnaryExpression();
			add_action(MOD);
		}
		else
			break;
	}
}

//static void parse_unary_expression()
void GnuPlot::ParseUnaryExpression()
{
	// add code for unary operators 
	if(Pgm.EqualsCur("!")) {
		Pgm.Shift();
		ParseUnaryExpression(); // @recursion
		add_action(LNOT);
	}
	else if(Pgm.EqualsCur("~")) {
		Pgm.Shift();
		ParseUnaryExpression(); // @recursion
		add_action(BNOT);
	}
	else if(Pgm.EqualsCur("-")) {
		at_entry * previous;
		Pgm.Shift();
		ParseUnaryExpression(); // @recursion
		// Collapse two operations PUSHC <pos-const> + UMINUS
		// into a single operation PUSHC <neg-const>
		previous = &(P_At->actions[P_At->a_count-1]);
		if(previous->index == PUSHC && previous->arg.v_arg.type == INTGR) {
			previous->arg.v_arg.v.int_val = -previous->arg.v_arg.v.int_val;
		}
		else if(previous->index == PUSHC && previous->arg.v_arg.type == CMPLX) {
			previous->arg.v_arg.v.cmplx_val.real = -previous->arg.v_arg.v.cmplx_val.real;
			previous->arg.v_arg.v.cmplx_val.imag = -previous->arg.v_arg.v.cmplx_val.imag;
		}
		else
			add_action(UMINUS);
	}
	else if(Pgm.EqualsCur("+")) { /* unary + is no-op */
		Pgm.Shift();
		ParseUnaryExpression(); // @recursion
	}
	else
		ParsePrimaryExpression();
}
// 
// Syntax: set link {x2|y2} {via <expression1> inverse <expression2>}
// Create action code tables for the functions linking primary and secondary axes.
// expression1 maps primary coordinates into the secondary coordinate space.
// expression2 maps secondary coordinates into the primary coordinate space.
// 
//void parse_link_via(struct udft_entry * udf)
void GnuPlot::ParseLinkVia(udft_entry * pUdf)
{
	// Caller left us pointing at "via" or "inverse" 
	Pgm.Shift();
	int start_token = Pgm.GetCurTokenIdx();
	if(Pgm.EndOfCommand())
		IntErrorCurToken("Missing expression");
	// Save action table for the linkage mapping 
	dummy_func = pUdf;
	free_at(pUdf->at);
	pUdf->at = PermAt();
	dummy_func = NULL;
	// Save the mapping expression itself 
	Pgm.MCapture(&(pUdf->definition), start_token, Pgm.GetPrevTokenIdx());
}
//
// create action code for 'sum' expressions 
//
//static void parse_sum_expression()
void GnuPlot::ParseSumExpression()
{
	/* sum [<var>=<range>] <expr>
	 * - Pass a udf to f_sum (with action code (for <expr>) that is not added
	 *   to the global action table).
	 * - f_sum uses a newly created udv (<var>) to pass the current value of
	 *   <var> to <expr> (resp. its ac).
	 * - The original idea was to treat <expr> as function f(<var>), but there
	 *   was the following problem: Consider 'g(x) = sum [k=1:4] f(k)'. There
	 *   are two dummy variables 'x' and 'k' from different functions 'g' and
	 *   'f' which would require changing the parsing of dummy variables.
	 */
	char * errormsg = "Expecting 'sum [<var> = <start>:<end>] <expression>'\n";
	char * varname = NULL;
	union argument * arg;
	udft_entry * udf;
	at_type * save_at;
	int save_at_size;
	int i;
	// Caller already checked for string "sum [" so skip both tokens 
	Pgm.Shift();
	Pgm.Shift();
	// <var> 
	if(!Pgm.IsLetter(Pgm.GetCurTokenIdx()))
		IntErrorCurToken(errormsg);
	// create a user defined variable and pass it to f_sum via PUSHC, since the
	// argument of f_sum is already used by the udf 
	Pgm.MCapture(&varname, Pgm.GetCurTokenIdx(), Pgm.GetCurTokenIdx());
	AddUdv(Pgm.GetCurTokenIdx());
	arg = add_action(PUSHC);
	Gstring(&(arg->v_arg), varname);
	Pgm.Shift();
	if(!Pgm.EqualsCur("="))
		IntErrorCurToken(errormsg);
	Pgm.Shift();
	// <start> 
	ParseExpression();
	if(!Pgm.EqualsCur(":"))
		IntErrorCurToken(errormsg);
	Pgm.Shift();
	// <end> 
	ParseExpression();
	if(!Pgm.EqualsCur("]"))
		IntErrorCurToken(errormsg);
	Pgm.Shift();
	// parse <expr> and convert it to a new action table. 
	// modeled on code from temp_at(). 
	// 1. save environment to restart parsing 
	save_at = P_At;
	save_at_size = AtSize;
	P_At = NULL;
	// 2. save action table in a user defined function 
	udf = (udft_entry *)SAlloc::M(sizeof(struct udft_entry));
	udf->next_udf = (udft_entry *)NULL;
	udf->udf_name = NULL; /* TODO maybe add a name and definition */
	udf->at = PermAt();
	udf->definition = NULL;
	udf->dummy_num = 0;
	for(i = 0; i < MAX_NUM_VAR; i++)
		Ginteger(&(udf->dummy_values[i]), 0);
	// 3. restore environment 
	P_At = save_at;
	AtSize = save_at_size;
	// pass the udf to f_sum using the argument 
	add_action(SUM)->udf_arg = udf;
}
//
// find or add value and return pointer 
//
//udvt_entry * add_udv(int t_num)
udvt_entry * GnuPlot::AddUdv(int t_num)
{
	char varname[MAX_ID_LEN+1];
	Pgm.CopyStr(varname, t_num, MAX_ID_LEN);
	if(Pgm.P_Token[t_num].length > MAX_ID_LEN-1)
		IntWarn(t_num, "truncating variable name that is too long");
	return Ev.AddUdvByName(varname);
}
//
// find or add function at index <t_num>, and return pointer 
//
//udft_entry * add_udf(int t_num)
udft_entry * GnuPlot::AddUdf(int t_num)
{
	udft_entry ** udf_ptr = &Ev.P_FirstUdf;
	int i;
	while(*udf_ptr) {
		if(Pgm.Equals(t_num, (*udf_ptr)->udf_name))
			return (*udf_ptr);
		udf_ptr = &((*udf_ptr)->next_udf);
	}
	// get here => not found. udf_ptr points at first_udf or next_udf field of last udf
	if(IsBuiltinFunction(t_num))
		IntWarn(t_num, "Warning : udf shadowed by built-in function of the same name");
	// create and return a new udf slot 
	*udf_ptr = (udft_entry *)SAlloc::M(sizeof(udft_entry));
	(*udf_ptr)->next_udf = 0;
	(*udf_ptr)->definition = NULL;
	(*udf_ptr)->at = NULL;
	(*udf_ptr)->udf_name = (char *)SAlloc::M(Pgm.TokenLen(t_num)+1);
	Pgm.CopyStr((*udf_ptr)->udf_name, t_num, Pgm.TokenLen(t_num)+1);
	for(i = 0; i < MAX_NUM_VAR; i++)
		Ginteger(&((*udf_ptr)->dummy_values[i]), 0);
	return (*udf_ptr);
}
//
// return standard function index or 0 
//
//int is_builtin_function(int t_num)
int GnuPlot::IsBuiltinFunction(int t_num) const
{
	for(int i = (int)SF_START; _FuncTab2[i].P_Name; i++) {
		if(Pgm.Equals(t_num, _FuncTab2[i].P_Name))
			return i;
	}
	return 0;
}
// 
// Test for the existence of a function without triggering errors
// Return values:
//   0  no such function is defined
//   -1  built-in function
//   1  user-defined function
// 
//int is_function(int t_num)
int GnuPlot::IsFunction(int t_num) const
{
	if(IsBuiltinFunction(t_num))
		return -1;
	else {
		for(const udft_entry * const * udf_ptr = &Ev.P_FirstUdf; *udf_ptr;) {
			if(Pgm.Equals(t_num, (*udf_ptr)->udf_name))
				return 1;
			udf_ptr = &((*udf_ptr)->next_udf);
		}
		return 0;
	}
}
// 
// Look for iterate-over-plot constructs, of the form
//   for [<var> = <start> : <end> { : <increment>}] ...
// If one (or more) is found, an iterator structure is allocated and filled
// and a pointer to that structure is returned.
// The pointer is NULL if no "for" statements are found.
// If the iteration limits are constants, store them as is.
// If they are given as expressions, store an action table for the expression.
// 
//GpIterator * check_for_iteration()
GpIterator * GnuPlot::CheckForIteration()
{
	const char * p_errormsg = "Expecting iterator \tfor [<var> = <start> : <end> {: <incr>}]\n\t\t\tor\tfor [<var> in \"string of words\"]";
	int nesting_depth = 0;
	GpIterator * iter = NULL;
	GpIterator * prev = NULL;
	GpIterator * this_iter = NULL;
	bool no_parent = FALSE;
	// Now checking for iteration parameters 
	// Nested "for" statements are supported, each one corresponds to a node of the linked list 
	while(Pgm.EqualsCur("for")) {
		udvt_entry * iteration_udv = NULL;
		GpValue original_udv_value;
		char * iteration_string = NULL;
		int iteration_start;
		int iteration_end;
		int iteration_increment = 1;
		int iteration_current;
		int iteration = 0;
		at_type * iteration_start_at = NULL;
		at_type * iteration_end_at = NULL;
		Pgm.Shift();
		if(!Pgm.EqualsCurShift("[") || !Pgm.IsLetter(Pgm.GetCurTokenIdx()))
			IntError(Pgm.GetPrevTokenIdx(), p_errormsg);
		iteration_udv = AddUdv(Pgm.GetCurTokenIdx());
		Pgm.Shift();
		original_udv_value = iteration_udv->udv_value;
		iteration_udv->udv_value.SetNotDefined();
		if(Pgm.EqualsCur("=")) {
			Pgm.Shift();
			if(Pgm.IsANumber(Pgm.GetCurTokenIdx()) && Pgm.EqualsNext(":")) {
				// Save the constant value only 
				iteration_start = IntExpression();
			}
			else {
				// Save the expression as well as the value 
				GpValue v;
				iteration_start_at = PermAt();
				if(no_parent) {
					iteration_start = 0;
				}
				else {
					EvaluateAt(iteration_start_at, &v);
					iteration_start = static_cast<int>(real(&v));
				}
			}
			if(!Pgm.EqualsCurShift(":"))
				IntError(Pgm.GetPrevTokenIdx(), p_errormsg);
			if(Pgm.EqualsCur("*")) {
				iteration_end = INT_MAX;
				Pgm.Shift();
			}
			else if(Pgm.IsANumber(Pgm.GetCurTokenIdx()) && (Pgm.EqualsNext(":") || Pgm.EqualsNext("]"))) {
				// Save the constant value only 
				iteration_end = IntExpression();
			}
			else {
				// Save the expression as well as the value 
				GpValue v;
				iteration_end_at = PermAt();
				if(no_parent) {
					iteration_end = 0;
				}
				else {
					EvaluateAt(iteration_end_at, &v);
					iteration_end = static_cast<int>(real(&v));
				}
			}
			if(Pgm.EqualsCur(":")) {
				Pgm.Shift();
				iteration_increment = IntExpression();
				if(iteration_increment == 0)
					IntError(Pgm.GetPrevTokenIdx(), p_errormsg);
			}
			if(!Pgm.EqualsCurShift("]"))
				IntError(Pgm.GetPrevTokenIdx(), p_errormsg);
			iteration_udv->udv_value.Destroy();
			Ginteger(&(iteration_udv->udv_value), iteration_start);
		}
		else if(Pgm.EqualsCurShift("in")) {
			// Assume this is a string-valued expression. 
			// It might be worth treating a string constant as a special case 
			GpValue v;
			iteration_start_at = PermAt();
			EvaluateAt(iteration_start_at, &v);
			if(v.type != STRING)
				IntError(Pgm.GetPrevTokenIdx(), p_errormsg);
			if(!Pgm.EqualsCurShift("]"))
				IntError(Pgm.GetPrevTokenIdx(), p_errormsg);
			iteration_string = v.v.string_val;
			iteration_start = 1;
			iteration_end = Gp_Words(iteration_string);
			iteration_udv->udv_value.Destroy();
			Gstring(&(iteration_udv->udv_value), Gp_Word(iteration_string, 1));
		}
		else // Neither [i=B:E] or [s in "foo"] 
			IntError(Pgm.GetPrevTokenIdx(), p_errormsg);
		iteration_current = iteration_start;
		this_iter = (GpIterator *)SAlloc::M(sizeof(GpIterator));
		this_iter->original_udv_value = original_udv_value;
		this_iter->iteration_udv = iteration_udv;
		this_iter->iteration_string = iteration_string;
		this_iter->iteration_start = iteration_start;
		this_iter->iteration_end = iteration_end;
		this_iter->iteration_increment = iteration_increment;
		this_iter->iteration_current = iteration_current;
		this_iter->iteration = iteration;
		this_iter->start_at = iteration_start_at;
		this_iter->end_at = iteration_end_at;
		this_iter->next = NULL;
		if(nesting_depth == 0) {
			iter = this_iter; // first "for" statement: this will be the listhead 
		}
		else {
			prev->next = this_iter; // nested "for": attach newly created node to the end of the list 
		}
		prev = this_iter;
		// If some depth of a nested iteration evaluates to an empty range, the
		// evaluated limits of depths below it are moot (and possibly invalid).
		// This flag tells us to skip their evaluation to avoid irrelevant errors.
		if(no_iteration(this_iter)) {
			no_parent = TRUE;
			FPRINTF((stderr, "iteration at level %d is moot\n", nesting_depth));
		}
		nesting_depth++;
	}
	return iter;
}
// 
// Reevaluate the iteration limits
// (in case they are functions whose parameters have taken
// on a new value)
// 
//static void reevaluate_iteration_limits(GpIterator * iter)
void GnuPlot::ReevaluateIterationLimits(GpIterator * iter)
{
	if(iter->start_at) {
		GpValue v;
		EvaluateAt(iter->start_at, &v);
		if(iter->iteration_string) {
			// unnecessary if iteration string is a constant 
			SAlloc::F(iter->iteration_string);
			if(v.type != STRING)
				IntError(NO_CARET, "corrupt iteration string");
			iter->iteration_string = v.v.string_val;
			iter->iteration_start = 1;
			iter->iteration_end = Gp_Words(iter->iteration_string);
		}
		else
			iter->iteration_start = static_cast<int>(real(&v));
	}
	if(iter->end_at) {
		GpValue v;
		EvaluateAt(iter->end_at, &v);
		iter->iteration_end = static_cast<int>(real(&v));
	}
}
//
// Reset iteration at this level to start value.
// Any iteration levels underneath are reset also.
// 
//static void reset_iteration(GpIterator * iter)
void GnuPlot::ResetIteration(GpIterator * iter)
{
	if(iter) {
		ReevaluateIterationLimits(iter);
		iter->iteration = -1;
		iter->iteration_current = iter->iteration_start;
		if(iter->iteration_string) {
			gpfree_string(&(iter->iteration_udv->udv_value));
			Gstring(&(iter->iteration_udv->udv_value), Gp_Word(iter->iteration_string, iter->iteration_current));
		}
		else {
			// This traps fatal user error of reassigning iteration variable to a string 
			gpfree_string(&(iter->iteration_udv->udv_value));
			Ginteger(&(iter->iteration_udv->udv_value), iter->iteration_current);
		}
		ResetIteration(iter->next); // @recursion
	}
}
// 
// Increment the iteration position recursively.
// returns TRUE if the iteration is still in range
// returns FALSE if the incement put it past the end limit
// 
//bool next_iteration(GpIterator * iter)
bool GnuPlot::NextIteration(GpIterator * iter)
{
	// Once it goes out of range it will stay that way until reset 
	if(!iter || no_iteration(iter))
		return FALSE;
	// Give sub-iterations a chance to advance 
	if(NextIteration(iter->next)) {
		if(iter->iteration < 0)
			iter->iteration = 0;
		return TRUE;
	}
	// Increment at this level 
	if(iter->iteration < 0) {
		// Just reset, haven't used start value yet 
		iter->iteration = 0;
		if(!empty_iteration(iter))
			return TRUE;
	}
	else {
		iter->iteration++;
		iter->iteration_current += iter->iteration_increment;
	}
	if(iter->iteration_string) {
		gpfree_string(&(iter->iteration_udv->udv_value));
		Gstring(&(iter->iteration_udv->udv_value), Gp_Word(iter->iteration_string, iter->iteration_current));
	}
	else {
		// This traps fatal user error of reassigning iteration variable to a string 
		gpfree_string(&(iter->iteration_udv->udv_value));
		Ginteger(&(iter->iteration_udv->udv_value), iter->iteration_current);
	}
	// If this runs off the end, leave the value out-of-range and return FALSE 
	if(iter->iteration_increment > 0 && iter->iteration_end - iter->iteration_current < 0)
		return FALSE;
	if(iter->iteration_increment < 0 && iter->iteration_end - iter->iteration_current > 0)
		return FALSE;
	if(iter->next == NULL)
		return TRUE;
	// Reset sub-iterations, if any 
	ResetIteration(iter->next);
	// Go back to top or call self recursively 
	return NextIteration(iter);
}
/*
 * Returns TRUE if
 * - this really is an iteration and
 * - the top level iteration covers no usable range
 */
static bool no_iteration(GpIterator * iter)
{
	if(!iter)
		return FALSE;
	if((iter->iteration_end > iter->iteration_start && iter->iteration_increment < 0) || (iter->iteration_end < iter->iteration_start && iter->iteration_increment > 0)) {
		return TRUE;
	}
	return FALSE;
}

/*
 * Recursive test that no empty iteration exists in a nested set of iterations
 */
bool empty_iteration(GpIterator * iter)
{
	if(!iter)
		return FALSE;
	else if(no_iteration(iter))
		return TRUE;
	else
		return no_iteration(iter->next);
}

GpIterator * cleanup_iteration(GpIterator * iter)
{
	while(iter) {
		GpIterator * next = iter->next;
		gpfree_string(&(iter->iteration_udv->udv_value));
		iter->iteration_udv->udv_value = iter->original_udv_value;
		SAlloc::F(iter->iteration_string);
		free_at(iter->start_at);
		free_at(iter->end_at);
		SAlloc::F(iter);
		iter = next;
	}
	return NULL;
}

bool forever_iteration(GpIterator * iter)
{
	if(!iter)
		return FALSE;
	else
		return (iter->iteration_end == INT_MAX);
}

/* The column() function requires special handling because
 * - It has side effects if reference to a column entry
 *   requires matching it to the column header string.
 * - These side effects must be handled at the time the
 *   expression is parsed rather than when it it evaluated.
 */
static void set_up_columnheader_parsing(struct at_entry * previous)
{
	/* column("string") means we expect the first row of */
	/* a data file to contain headers rather than data.  */
	if(previous->index == PUSHC && previous->arg.v_arg.type == STRING)
		parse_1st_row_as_headers = TRUE;

	/* This allows plot ... using (column(<const>)) title columnhead */
	if(previous->index == PUSHC && previous->arg.v_arg.type == INTGR) {
		if(at_highest_column_used < previous->arg.v_arg.v.int_val)
			at_highest_column_used = previous->arg.v_arg.v.int_val;
	}
	/* This attempts to catch plot ... using (column(<variable>)) */
	if(previous->index == PUSH) {
		udvt_entry * u = previous->arg.udv_arg;
		if(u->udv_value.type == INTGR) {
			if(at_highest_column_used < u->udv_value.v.int_val)
				at_highest_column_used = u->udv_value.v.int_val;
		}
	}
	/* NOTE: There is no way to handle ... using (column(<general expression>)) */
}
