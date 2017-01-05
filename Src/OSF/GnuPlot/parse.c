/* GNUPLOT - parse.c */

/*[
 * Copyright 1986 - 1993, 1998, 2004   Thomas Williams, Colin Kelley
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
#include <gnuplot.h>
#pragma hdrstop

//GpParser GpP;

//
// Internal prototypes:
//
//static void convert(t_value *, int);
static void extend_at();

//static void convert(t_value * val_ptr, int t_num)
void GpCommand::Convert(t_value * pValue, int t_num)
{
	*pValue = P_Token[t_num].l_val;
}

int GpCommand::IntExpression()
{
	return (int)RealExpression();
}

double GpCommand::RealExpression()
{
	t_value a;
	double result = ConstExpress(&a)->Real();
	gpfree_string(&a);
	return result;
}

//void parse_reset_after_error()
void GpParser::ParseResetAfterError()
{
	IsStringResultOnly = false;
	parse_recursion_level = 0;
}

/* JW 20051126:
 * Wrapper around const_express() called by GpC.TryToGetString().
 * Disallows top level + and - operators.
 * This enables things like set xtics ('-\pi' -pi, '-\pi/2' -pi/2.)
 */
t_value * GpParser::ConstStringExpress(t_value * valptr)
{
	IsStringResultOnly = true;
	ConstExpress(GpC, valptr);
	IsStringResultOnly = false;
	return (valptr);
}

t_value * GpParser::ConstExpress(GpCommand & rC, t_value * valptr)
{
	int    tkn = rC.CToken;
	if(rC.EndOfCommand())
		GpGg.IntError(rC, rC.CToken, "constant expression required");
	// div - no dummy variables in a constant expression
	rC.P_DummyFunc = NULL;
	GpGg.Ev.EvaluateAt(TempAt(), valptr); // run it and send answer back
	if(GpGg.Ev.undefined) {
		GpGg.IntError(GpC, tkn, "undefined value");
	}
	if(valptr->type == ARRAY) {
		// Make sure no one tries to free it later
		valptr->type = NOTDEFINED;
		GpGg.IntError(GpC, NO_CARET, "const_express: unsupported array operation");
	}
	return (valptr);
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
//char * string_or_express(AtType ** atptr)
char * GpParser::StringOrExpress(GpCommand & rC, AtType ** atptr)
{
	int i;
	bool has_dummies;
	static char * array_placeholder = "@@";
	static char * str = NULL;
	ZFREE(str);
	P_DfArray = NULL;
	ASSIGN_PTR(atptr, 0);
	if(rC.EndOfCommand())
		GpGg.IntError(rC, rC.CToken, "expression expected");
	// parsing for datablocks
	if(rC.Eq("$"))
		return rC.ParseDataBlockName();
	if(rC.IsString(rC.CToken) && (str = rC.TryToGetString()))
		return str;
	// If this is a bare array name for an existing array, store a pointer
	// for df_open() to use.  "@@" is a magic pseudo-filename passed to
	// df_open() that tells it to use the stored pointer.
	if(rC.TypeDdv(rC.CToken) == ARRAY && !rC.Eq(rC.CToken+1, "[")) {
		P_DfArray = GpGg.Ev.AddUdv(rC, rC.CToken++);
		return array_placeholder;
	}
	// parse expression
	TempAt();
	/* check if any dummy variables are used */
	has_dummies = false;
	for(i = 0; i < P_At->a_count; i++) {
		enum operators op_index = P_At->actions[i].Index;
		if(oneof4(op_index, PUSHD1, PUSHD2, PUSHD, SUM)) {
			has_dummies = true;
			break;
		}
	}
	if(!has_dummies) {
		// no dummy variables: evaluate expression 
		t_value val;
		GpGg.Ev.EvaluateAt(P_At, &val);
		if(!GpGg.Ev.undefined && val.type == STRING) {
			/* prevent empty string variable from treated as special file '' or "" */
			if(*val.v.string_val == '\0') {
				free(val.v.string_val);
				str = _strdup(" ");
			}
			else {
				str = val.v.string_val;
			}
		}
	}
	// prepare return
	ASSIGN_PTR(atptr, P_At);
	return str;
}
//
// build an action table and return its pointer, but keep a pointer in at
// so that we can free it later if the caller hasn't taken over management of this table.
//
// temp_at()
AtType * GpParser::TempAt()
{
	AtType::Destroy(P_At);
	P_At = (AtType *)malloc(sizeof(AtType));
	memzero(P_At, sizeof(*P_At)); // reset action table !!! 
	AtSize = MAX_AT_LEN;
	parse_recursion_level = 0;
	ParseExpression(GpC);
	return P_At;
}
//
// build an action table, put it in dynamic memory, and return its pointer
//
//AtType * perm_at()
AtType * GpParser::PermAt()
{
	TempAt();
	size_t len = sizeof(AtType) + (P_At->a_count - MAX_AT_LEN) * sizeof(AtEntry);
	AtType * at_ptr = (AtType*)gp_realloc(P_At, len, "perm_at");
	P_At = NULL; // invalidate at pointer
	return (at_ptr);
}
//
// Create an action table that describes a call to column("string"). 
// This is used by plot_option_using() to handle 'plot ... using "string"' 
//
AtType * create_call_column_at(char * string)
{
	AtType * at = (AtType *)malloc(sizeof(int) + 2*sizeof(AtEntry));
	at->a_count = 2;
	at->actions[0].Index = PUSHC;
	at->actions[0].arg.j_arg = 3;   /* FIXME - magic number! */
	at->actions[0].arg.v_arg.type = STRING;
	at->actions[0].arg.v_arg.v.string_val = string;
	at->actions[1].Index = COLUMN;
	at->actions[1].arg.j_arg = 0;
	return (at);
}

void GpParser::ExtendAt()
{
	size_t newsize = sizeof(AtType) + AtSize * sizeof(AtEntry);
	P_At = (AtType *)gp_realloc(P_At, newsize, "extend_at");
	AtSize += MAX_AT_LEN;
	FPRINTF((stderr, "Extending at size to %d\n", at_size));
}
//
// Add function number <sf_index> to the current action table
//
GpArgument * GpParser::AddAction(enum operators sf_index)
{
	if(P_At->a_count >= AtSize) {
		ExtendAt();
	}
	P_At->actions[P_At->a_count].Index = sf_index;
	return (&(P_At->actions[P_At->a_count++].arg));
}
//
// For external calls to parse_expressions()
// parse_recursion_level is expected to be 0
//
void GpParser::ParseExpression(GpCommand & rC)
{
	// full expressions
	if(!ParseAssignmentExpression(rC)) {
		parse_recursion_level++;
		AcceptLogicalOrExpression(rC);
		ParseConditionalExpression(rC);
		parse_recursion_level--;
	}
}

//static void accept_logical_OR_expression()
void GpParser::AcceptLogicalOrExpression(GpCommand & rC)
{
	// ? : expressions
	AcceptLogicalAndExpression(rC);
	ParseLogicalOrExpression(rC);
}

void GpParser::AcceptLogicalAndExpression(GpCommand & rC)
{
	AcceptInclusiveOrExpression(rC);
	ParseLogicalAndExpression(rC);
}

void GpParser::AcceptInclusiveOrExpression(GpCommand & rC)
{
	AcceptExclusiveOrExpression(rC);
	ParseInclusiveOrExpression(rC);
}

void GpParser::AcceptExclusiveOrExpression(GpCommand & rC)
{
	AcceptAndExpression(rC);
	ParseExclusiveOrExpression(rC);
}

void GpParser::AcceptAndExpression(GpCommand & rC)
{
	AcceptEqualityExpression(rC);
	ParseAndExpression(rC);
}

void GpParser::AcceptEqualityExpression(GpCommand & rC)
{
	AcceptRelationalExpression(rC);
	ParseEqualityExpression(rC);
}

//static void accept_relational_expression()
void GpParser::AcceptRelationalExpression(GpCommand & rC)
{
	AcceptBitshiftExpression(rC);
	ParseRelationalExpression(rC);
}

//static void accept_bitshift_expression()
void GpParser::AcceptBitshiftExpression(GpCommand & rC)
{
	AcceptAdditiveExpression(rC);
	ParseBitshiftExpression(rC);
}

void GpParser::AcceptAdditiveExpression(GpCommand & rC)
{
	AcceptMultiplicativeExpression(rC);
	ParseAdditiveExpression(rC);
}

void GpParser::AcceptMultiplicativeExpression(GpCommand & rC)
{
	ParseUnaryExpression(rC); // - things
	ParseMultiplicativeExpression(rC);// * / %
}

int GpParser::ParseAssignmentExpression(GpCommand & rC)
{
	// Check for assignment operator Var = <expr>
	if(rC.IsLetter(rC.CToken) && rC.Eq(rC.CToken+1, "=")) {
		/* push the variable name */
		GpArgument * foo = AddAction(PUSHC);
		char * varname = NULL;
		rC.MCapture(&varname, rC.CToken, rC.CToken);
		foo->v_arg.type = STRING;
		foo->v_arg.v.string_val = varname;
		/* push a dummy variable that would be the index if this were an array */
		/* FIXME: It would be nice to hide this from "show at" */
		foo = AddAction(PUSHC);
		foo->v_arg.type = NOTDEFINED;
		// push the expression whose value it will get
		rC.CToken += 2;
		ParseExpression(rC);
		// push the actual assignment operation
		AddAction(ASSIGN);
		return 1;
	}
	else {
		// Check for assignment to an array element Array[<expr>] = <expr>
		return ParseArrayAssignmentExpression(rC) ? 1 : 0;
	}
}
//
// If an array assignment is the first thing on a command line it is handled by
// the separate routine array_assignment().
// Here we catch assignments that are embedded in an expression.
// Examples:
//   print A[2] = foo
//   A[1] = A[2] = A[3] = 0
//
//static int parse_array_assignment_expression()
int GpParser::ParseArrayAssignmentExpression(GpCommand & rC)
{
	// Check for assignment to an array element Array[<expr>] = <expr>
	if(rC.IsLetter(rC.CToken) && rC.Eq(rC.CToken+1, "[")) {
		char * varname = NULL;
		GpArgument * foo;
		// Quick check for the most common false positives
		// i.e. other constructs that begin with "name["
		if(rC.Eq(rC.CToken+3, ":"))
			return 0;
		else if(rC.Eq(rC.CToken+3, "]") && !rC.Eq(rC.CToken+4, "="))
			return 0;
		else {
			// Save state of the action table and the command line
			const int save_action = P_At->a_count;
			const int save_token = rC.CToken;
			// push the array name
			rC.MCapture(&varname, rC.CToken, rC.CToken);
			foo = AddAction(PUSHC);
			foo->v_arg.type = STRING;
			foo->v_arg.v.string_val = varname;
			// push the index
			rC.CToken += 2;
			ParseExpression(rC);
			// If this wasn't really an array element assignment, back out
			if(!rC.Eq("]") || !rC.Eq(rC.CToken+1, "=")) {
				rC.CToken = save_token;
				P_At->a_count = save_action;
				free(varname);
				return 0;
			}
			else {
				// Now we evaluate the expression whose value it will get
				rC.CToken += 2;
				ParseExpression(rC);
				// push the actual assignment operation
				AddAction(ASSIGN);
				return 1;
			}
		}
	}
	else
		return 0;
}
//
// add action table entries for primary expressions, i.e. either a
// parenthesized expression, a variable name, a numeric constant, a
// function evaluation, a power operator or postfix '!' (factorial) expression
//
void GpParser::ParsePrimaryExpression(GpCommand & rC)
{
	if(rC.Eq("(")) {
		rC.CToken++;
		ParseExpression(rC);
		// Expressions may be separated by a comma
		while(rC.Eq(",")) {
			rC.CToken++;
			AddAction(POP);
			ParseExpression(rC);
		}
		if(!rC.Eq(")"))
			GpGg.IntError(rC, rC.CToken, "')' expected");
		rC.CToken++;
	}
	else if(rC.Eq("$")) {
		t_value a;
		rC.CToken++;
		if(rC.Eq("N")) { /* $N == pseudocolumn -3 means "last column" */
			rC.CToken++;
			a.SetInt(-3);
			AtHighestColumnUsed = -3;
		}
		else if(!rC.IsANumber(rC.CToken)) {
			GpGg.IntError(rC, rC.CToken, "Column number expected");
		}
		else {
			rC.Convert(&a, rC.CToken++);
			if(a.type != INTGR || a.v.int_val < 0)
				GpGg.IntError(rC, rC.CToken, "Positive integer expected");
			SETMAX(AtHighestColumnUsed, a.v.int_val);
		}
		AddAction(DOLLARS)->v_arg = a;
	}
	else if(rC.IsANumber(rC.CToken)) {
		GpArgument * foo = AddAction(PUSHC);
		rC.Convert(&(foo->v_arg), rC.CToken);
		rC.CToken++;
	}
	else if(rC.IsLetter(rC.CToken)) {
		// Found an identifier --- check whether its a function or a
		// variable by looking for the parentheses of a function argument list 
		if(rC.Eq(rC.CToken + 1, "(")) {
			enum operators whichfunc = (enum operators)GpGg.Ev.IsBuiltinFunction(rC, rC.CToken);
			t_value num_params;
			num_params.type = INTGR;
#if(1) // DEPRECATED 
			if(whichfunc && (strcmp(GpGg.Ev.ft[whichfunc].f_name, "defined")==0)) {
				// Deprecated syntax:   if(defined(foo)) ...  
				// New syntax:          if(exists("foo")) ... 
				UdvtEntry * udv = GpGg.Ev.AddUdv(rC, rC.CToken+2);
				GpArgument * foo = AddAction(PUSHC);
				foo->v_arg.type = INTGR;
				if(udv->udv_value.type == NOTDEFINED)
					foo->v_arg.v.int_val = 0;
				else
					foo->v_arg.v.int_val = 1;
				rC.CToken += 4; /* skip past "defined ( <foo> ) " */
				return;
			}
#endif
			if(whichfunc) {
				rC.CToken += 2; /* skip fnc name and '(' */
				ParseExpression(rC); /* parse fnc argument */
				num_params.v.int_val = 1;
				while(rC.Eq(",")) {
					rC.CToken++;
					num_params.v.int_val++;
					ParseExpression(rC);
				}
				if(!rC.Eq(")"))
					GpGg.IntError(rC, rC.CToken, "')' expected");
				rC.CToken++;
				// So far sprintf is the only built-in function 
				// with a variable number of arguments.         
				if(!strcmp(GpGg.Ev.ft[whichfunc].f_name, "sprintf"))
					AddAction(PUSHC)->v_arg = num_params;
				// v4 timecolumn only had 1 param; v5 has 2. Accept either 
				if(!strcmp(GpGg.Ev.ft[whichfunc].f_name, "timecolumn"))
					AddAction(PUSHC)->v_arg = num_params;
				// The column() function has side effects requiring special handling 
				if(!strcmp(GpGg.Ev.ft[whichfunc].f_name, "column")) {
					SetupColumnheaderParsing(P_At->actions[P_At->a_count-1]);
				}
				AddAction(whichfunc);
			}
			else {
				// it's a call to a user-defined function 
				enum operators call_type = (enum operators)CALL;
				int tok = rC.CToken;
				rC.CToken += 2; // skip func name and '('
				ParseExpression(rC);
				if(rC.Eq(",")) { // more than 1 argument? 
					num_params.v.int_val = 1;
					while(rC.Eq(",")) {
						num_params.v.int_val += 1;
						rC.CToken += 1;
						ParseExpression(rC);
					}
					AddAction(PUSHC)->v_arg = num_params;
					call_type = (enum operators)CALLN;
				}
				if(!rC.Eq(")"))
					GpGg.IntError(rC, rC.CToken, "')' expected");
				rC.CToken++;
				AddAction(call_type)->udf_arg = GpGg.Ev.AddUdf(rC, tok);
			}
		}
		else if(rC.Eq("sum") && rC.Eq(rC.CToken+1, "[")) {
			ParseSumExpression(rC);
			// dummy_func==NULL is a flag to say no dummy variables active 
		}
		else if(rC.P_DummyFunc) {
			if(rC.Eq(CDummyVar[0])) {
				rC.CToken++;
				AddAction(PUSHD1)->udf_arg = rC.P_DummyFunc;
				FitDummyVar[0]++;
			}
			else if(rC.Eq(CDummyVar[1])) {
				rC.CToken++;
				AddAction(PUSHD2)->udf_arg = rC.P_DummyFunc;
				FitDummyVar[1]++;
			}
			else {
				int i, param = 0;
				for(i = 2; i < MAX_NUM_VAR; i++) {
					if(rC.Eq(CDummyVar[i])) {
						t_value num_params;
						num_params.type = INTGR;
						num_params.v.int_val = i;
						param = 1;
						rC.CToken++;
						AddAction(PUSHC)->v_arg = num_params;
						AddAction(PUSHD)->udf_arg = rC.P_DummyFunc;
						FitDummyVar[i]++;
						break;
					}
				}
				if(!param) { /* defined variable */
					AddAction(PUSH)->udv_arg = GpGg.Ev.AddUdv(rC, rC.CToken);
					rC.CToken++;
				}
			}
			/* its a variable, with no dummies active - div */
		}
		else {
			AddAction(PUSH)->udv_arg = GpGg.Ev.AddUdv(rC, rC.CToken);
			rC.CToken++;
		}
	}
	/* end if letter */
	// Maybe it's a string constant
	else if(rC.IsString(rC.CToken)) {
		GpArgument * foo = AddAction(PUSHC);
		foo->v_arg.type = STRING;
		foo->v_arg.v.string_val = NULL;
		// this dynamically allocated string will be freed by free_at()
		rC.MQuoteCapture(&(foo->v_arg.v.string_val), rC.CToken, rC.CToken);
		rC.CToken++;
	}
	else {
		GpGg.IntError(rC, rC.CToken, "invalid expression ");
	}
	/* The remaining operators are postfixes and can be stacked, e.g. */
	/* Array[i]**2, so we may have to loop to catch all of them.      */
	while(true) {
		/* add action code for ! (factorial) operator */
		if(rC.Eq("!")) {
			rC.CToken++;
			AddAction(FACTORIAL);
		}
		/* add action code for ** operator */
		else if(rC.Eq("**")) {
			rC.CToken++;
			ParseUnaryExpression(rC);
			AddAction(POWER);
		}
		/* Parse and add actions for range specifier applying to previous entity.
		 * Currently the [beg:end] form is used to generate substrings, but could
		 * also be used to extract vector slices.  The [i] form is used to index
		 * arrays, but could also be a shorthand for extracting a single-character
		 * substring.
		 */
		else if(rC.Eq("[") && !rC.IsANumber(rC.CToken-1)) {
			/* handle '*' or empty start of range */
			if(rC.Eq(++rC.CToken, "*") || rC.Eq(":")) {
				GpArgument * empty = AddAction(PUSHC);
				empty->v_arg.type = INTGR;
				empty->v_arg.v.int_val = 1;
				if(rC.Eq("*"))
					rC.CToken++;
			}
			else
				ParseExpression(rC);
			// handle array indexing (single value in square brackets)
			if(rC.Eq("]")) {
				rC.CToken++;
				AddAction(INDEX);
				continue;
			}
			if(!rC.Eq(":"))
				GpGg.IntError(rC, rC.CToken, "':' expected");
			// handle '*' or empty end of range 
			if(rC.Eq(++rC.CToken, "*") || rC.Eq("]")) {
				GpArgument * empty = AddAction(PUSHC);
				empty->v_arg.type = INTGR;
				empty->v_arg.v.int_val = 65535; // should be INT_MAX 
				if(rC.Eq("*"))
					rC.CToken++;
			}
			else
				ParseExpression(rC);
			if(!rC.Eq("]"))
				GpGg.IntError(rC, rC.CToken, "']' expected");
			rC.CToken++;
			AddAction(RANGE);
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
// express() calls!. Access via index savepc1/savepc2, instead. */
//
//static void parse_conditional_expression()
void GpParser::ParseConditionalExpression(GpCommand & rC)
{
	// create action code for ? : expressions
	if(rC.Eq("?")) {
		int savepc1, savepc2;
		/* Fake same recursion level for alternatives
		 *   set xlabel a>b ? 'foo' : 'bar' -1, 1
		 * FIXME: This won't work:
		 *   set xlabel a-b>c ? 'foo' : 'bar'  offset -1, 1
		 */
		parse_recursion_level--;
		rC.CToken++;
		savepc1 = P_At->a_count;
		AddAction(JTERN);
		ParseExpression(rC);
		if(!rC.Eq(":"))
			GpGg.IntError(rC, rC.CToken, "expecting ':'");
		rC.CToken++;
		savepc2 = P_At->a_count;
		AddAction(JUMP);
		P_At->actions[savepc1].arg.j_arg = P_At->a_count - savepc1;
		ParseExpression(rC);
		P_At->actions[savepc2].arg.j_arg = P_At->a_count - savepc2;
		parse_recursion_level++;
	}
}

void GpParser::ParseLogicalOrExpression(GpCommand & rC)
{
	// create action codes for || operator
	while(rC.Eq("||")) {
		int savepc;
		rC.CToken++;
		savepc = P_At->a_count;
		AddAction(JUMPNZ); /* short-circuit if already true */
		AcceptLogicalAndExpression(rC);
		// offset for jump 
		P_At->actions[savepc].arg.j_arg = P_At->a_count - savepc;
		AddAction(BOOLE);
	}
}

void GpParser::ParseLogicalAndExpression(GpCommand & rC)
{
	// create action code for && operator
	while(rC.Eq("&&")) {
		int savepc;
		rC.CToken++;
		savepc = P_At->a_count;
		AddAction(JUMPZ); // short-circuit if already false 
		AcceptInclusiveOrExpression(rC);
		P_At->actions[savepc].arg.j_arg = P_At->a_count - savepc; // offset for jump 
		AddAction(BOOLE);
	}
}

void GpParser::ParseInclusiveOrExpression(GpCommand & rC)
{
	// create action code for | operator
	while(rC.Eq("|")) {
		rC.CToken++;
		AcceptExclusiveOrExpression(rC);
		AddAction(BOR);
	}
}

void GpParser::ParseExclusiveOrExpression(GpCommand & rC)
{
	// create action code for ^ operator
	while(rC.Eq("^")) {
		rC.CToken++;
		AcceptAndExpression(rC);
		AddAction(XOR);
	}
}

void GpParser::ParseAndExpression(GpCommand & rC)
{
	// create action code for & operator
	while(rC.Eq("&")) {
		rC.CToken++;
		AcceptEqualityExpression(rC);
		AddAction(BAND);
	}
}

//static void parse_equality_expression()
void GpParser::ParseEqualityExpression(GpCommand & rC)
{
	// create action codes for == and != numeric operators eq and ne string operators
	while(true) {
		if(rC.Eq("==")) {
			rC.CToken++;
			AcceptRelationalExpression(rC);
			AddAction(EQ);
		}
		else if(rC.Eq("!=")) {
			rC.CToken++;
			AcceptRelationalExpression(rC);
			AddAction(NE);
		}
		else if(rC.Eq("eq")) {
			rC.CToken++;
			AcceptRelationalExpression(rC);
			AddAction(EQS);
		}
		else if(rC.Eq("ne")) {
			rC.CToken++;
			AcceptRelationalExpression(rC);
			AddAction(NES);
		}
		else
			break;
	}
}

//static void parse_relational_expression()
void GpParser::ParseRelationalExpression(GpCommand & rC)
{
	// create action code for < > >= or <= operators
	while(true) {
		if(rC.Eq(">")) {
			rC.CToken++;
			AcceptBitshiftExpression(rC);
			AddAction(GT);
		}
		else if(rC.Eq("<")) {
			//  Workaround for * in syntax of range constraints
			if(IsScanningRangeInProgress && rC.Eq(rC.CToken+1, "*") ) {
				break;
			}
			rC.CToken++;
			AcceptBitshiftExpression(rC);
			AddAction(LT);
		}
		else if(rC.Eq(">=")) {
			rC.CToken++;
			AcceptBitshiftExpression(rC);
			AddAction(GE);
		}
		else if(rC.Eq("<=")) {
			rC.CToken++;
			AcceptBitshiftExpression(rC);
			AddAction(LE);
		}
		else
			break;
	}
}

//static void parse_bitshift_expression()
void GpParser::ParseBitshiftExpression(GpCommand & rC)
{
	// create action codes for << and >> operators
	while(true) {
		if(rC.Eq("<<")) {
			rC.CToken++;
			AcceptAdditiveExpression(rC);
			AddAction(LEFTSHIFT);
		}
		else if(rC.Eq(">>")) {
			rC.CToken++;
			AcceptAdditiveExpression(rC);
			AddAction(RIGHTSHIFT);
		}
		else
			break;
	}
}

void GpParser::ParseAdditiveExpression(GpCommand & rC)
{
	// create action codes for +, - and . operators 
	while(true) {
		if(rC.Eq(".")) {
			rC.CToken++;
			AcceptMultiplicativeExpression(rC);
			AddAction(CONCATENATE);
			// If only string results are wanted do not accept '-' or '+' at the top level.
		}
		else if(IsStringResultOnly && parse_recursion_level == 1) {
			break;
		}
		else if(rC.Eq("+")) {
			rC.CToken++;
			AcceptMultiplicativeExpression(rC);
			AddAction(PLUS);
		}
		else if(rC.Eq("-")) {
			rC.CToken++;
			AcceptMultiplicativeExpression(rC);
			AddAction(MINUS);
		}
		else
			break;
	}
}

void GpParser::ParseMultiplicativeExpression(GpCommand & rC)
{
	// add action code for * / and % operators
	while(true) {
		if(rC.Eq("*")) {
			rC.CToken++;
			ParseUnaryExpression(rC);
			AddAction(MULT);
		}
		else if(rC.Eq("/")) {
			rC.CToken++;
			ParseUnaryExpression(rC);
			AddAction(DIV);
		}
		else if(rC.Eq("%")) {
			rC.CToken++;
			ParseUnaryExpression(rC);
			AddAction(MOD);
		}
		else
			break;
	}
}

void GpParser::ParseUnaryExpression(GpCommand & rC)
{
	// add code for unary operators
	if(rC.Eq("!")) {
		rC.CToken++;
		ParseUnaryExpression(rC); // @recursion
		AddAction(LNOT);
	}
	else if(rC.Eq("~")) {
		rC.CToken++;
		ParseUnaryExpression(rC); // @recursion
		AddAction(BNOT);
	}
	else if(rC.Eq("-")) {
		AtEntry * previous;
		rC.CToken++;
		ParseUnaryExpression(rC); // @recursion
		// Collapse two operations PUSHC <pos-const> + UMINUS into a single operation PUSHC <neg-const>
		previous = &P_At->actions[P_At->a_count-1];
		if(previous->Index == PUSHC &&  previous->arg.v_arg.type == INTGR) {
			previous->arg.v_arg.v.int_val = -previous->arg.v_arg.v.int_val;
		}
		else if(previous->Index == PUSHC &&  previous->arg.v_arg.type == CMPLX) {
			previous->arg.v_arg.v.cmplx_val.real = -previous->arg.v_arg.v.cmplx_val.real;
			previous->arg.v_arg.v.cmplx_val.imag = -previous->arg.v_arg.v.cmplx_val.imag;
		}
		else
			AddAction(UMINUS);
	}
	else if(rC.Eq("+")) { /* unary + is no-op */
		rC.CToken++;
		ParseUnaryExpression(rC); // @recursion
	}
	else
		ParsePrimaryExpression(rC);
}
//
// Syntax: set link {x2|y2} {via <expression1> inverse <expression2>}
// Create action code tables for the functions linking primary and secondary axes.
// expression1 maps primary coordinates into the secondary GpCoordinate space.
// expression2 maps secondary coordinates into the primary GpCoordinate space.
//
void GpCommand::ParseLinkVia(UdftEntry * pUdf)
{
	// Caller left us pointing at "via" or "inverse"
	CToken++;
	int start_token = CToken;
	if(EndOfCommand())
		GpGg.IntError(*this, CToken, "Missing expression");
	// Save action table for the linkage mapping
	P_DummyFunc = pUdf;
	AtType::Destroy(pUdf->at);
	pUdf->at = P.PermAt();
	P_DummyFunc = NULL;
	// Save the mapping expression itself
	MCapture(&(pUdf->definition), start_token, CToken - 1);
}
//
// create action code for 'sum' expressions
//
void GpParser::ParseSumExpression(GpCommand & rC)
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
	GpArgument * arg;
	UdftEntry * udf;
	AtType * save_at;
	int save_at_size;
	int i;
	// Caller already checked for string "sum [" so skip both tokens 
	rC.CToken += 2;
	/* <var> */
	if(!rC.IsLetter(rC.CToken))
		GpGg.IntError(rC, rC.CToken, errormsg);
	// create a user defined variable and pass it to f_sum via PUSHC, since the
	// argument of f_sum is already used by the udf 
	rC.MCapture(&varname, rC.CToken, rC.CToken);
	GpGg.Ev.AddUdv(rC, rC.CToken);
	arg = AddAction(PUSHC);
	Gstring(&(arg->v_arg), varname);
	rC.CToken++;
	if(!rC.Eq("="))
		GpGg.IntError(rC, rC.CToken, errormsg);
	rC.CToken++;
	/* <start> */
	ParseExpression(rC);
	if(!rC.Eq(":"))
		GpGg.IntError(rC, rC.CToken, errormsg);
	rC.CToken++;
	/* <end> */
	ParseExpression(rC);
	if(!rC.Eq("]"))
		GpGg.IntError(rC, rC.CToken, errormsg);
	rC.CToken++;
	/* parse <expr> and convert it to a new action table. */
	/* modeled on code from temp_at(). */
	/* 1. save environment to restart parsing */
	save_at = P_At;
	save_at_size = AtSize;
	P_At = NULL;
	// 2. save action table in a user defined function 
	udf = (UdftEntry*)malloc(sizeof(UdftEntry));
	udf->next_udf = (UdftEntry*)NULL;
	udf->udf_name = NULL; // TODO maybe add a name and definition 
	udf->at = PermAt();
	udf->definition = NULL;
	udf->dummy_num = 0;
	for(i = 0; i < MAX_NUM_VAR; i++)
		udf->dummy_values[i].SetInt(0);
	// 3. restore environment 
	P_At = save_at;
	AtSize = save_at_size;
	// pass the udf to f_sum using the argument
	AddAction(SUM)->udf_arg = udf;
}
//
// find or add value and return pointer
//
//UdvtEntry * add_udv(int t_num)
UdvtEntry * GpEval::AddUdv(GpCommand & rC, int t_num)
{
	char varname[MAX_ID_LEN+1];
	rC.CopyStr(varname, t_num, MAX_ID_LEN);
	if(rC.P_Token[t_num].length > MAX_ID_LEN-1)
		int_warn(t_num, "truncating variable name that is too long");
	return AddUdvByName(varname);
}
//
// find or add function at index <t_num>, and return pointer
//
//UdftEntry * add_udf(int t_num)
UdftEntry * GpEval::AddUdf(GpCommand & rC, int t_num)
{
	UdftEntry ** udf_ptr = &first_udf;
	int i;
	while(*udf_ptr) {
		if(rC.Eq(t_num, (*udf_ptr)->udf_name))
			return (*udf_ptr);
		udf_ptr = &((*udf_ptr)->next_udf);
	}
	// get here => not found. udf_ptr points at first_udf or next_udf field of last udf
	if(IsBuiltinFunction(rC, t_num))
		int_warn(t_num, "Warning : udf shadowed by built-in function of the same name");
	/* create and return a new udf slot */
	*udf_ptr = (UdftEntry*)malloc(sizeof(UdftEntry));
	(*udf_ptr)->next_udf = (UdftEntry*)NULL;
	(*udf_ptr)->definition = NULL;
	(*udf_ptr)->at = NULL;
	(*udf_ptr)->udf_name = (char *)malloc(rC.TokenLen(t_num)+1);
	rC.CopyStr((*udf_ptr)->udf_name, t_num, rC.TokenLen(t_num)+1);
	for(i = 0; i < MAX_NUM_VAR; i++)
		(*udf_ptr)->dummy_values[i].SetInt(0);
	return (*udf_ptr);
}
//
// return standard function index or 0
//
//static int is_builtin_function(int t_num)
int GpEval::IsBuiltinFunction(GpCommand & rC, int t_num) const
{
	for(int i = (int)SF_START; ft[i].f_name != NULL; i++) {
		if(rC.Eq(t_num, ft[i].f_name))
			return (i);
	}
	return (0);
}
//
// Look for iterate-over-plot constructs, of the form
//    for [<var> = <start> : <end> { : <increment>}] ...
// If one (or more) is found, an iterator structure is allocated and filled
// and a pointer to that structure is returned.
// The pointer is NULL if no "for" statements are found.
//
//t_iterator * check_for_iteration()
t_iterator * GpCommand::CheckForIteration()
{
	char * errormsg = "Expecting iterator \tfor [<var> = <start> : <end> {: <incr>}]\n\t\t\tor\tfor [<var> in \"string of words\"]";
	int nesting_depth = 0;
	t_iterator * iter = NULL;
	t_iterator * this_iter = NULL;
	// Now checking for iteration parameters 
	// Nested "for" statements are supported, each one corresponds to a node of the linked list 
	while(Eq("for")) {
		UdvtEntry * iteration_udv = NULL;
		char * iteration_string = NULL;
		int iteration_start;
		int iteration_end;
		int iteration_increment = 1;
		int iteration_current;
		int iteration = 0;
		bool empty_iteration;
		bool just_once = false;
		CToken++;
		if(!Eq(CToken++, "[") || !IsLetter(CToken))
			GpGg.IntError(*this, CToken-1, errormsg);
		iteration_udv = GpGg.Ev.AddUdv(*this, CToken++);
		if(Eq("=")) {
			CToken++;
			iteration_start = IntExpression();
			if(!Eq(CToken++, ":"))
				GpGg.IntError(*this, CToken-1, errormsg);
			if(Eq("*")) {
				iteration_end = INT_MAX;
				CToken++;
			}
			else
				iteration_end = IntExpression();
			if(Eq(":")) {
				CToken++;
				iteration_increment = IntExpression();
				if(iteration_increment == 0)
					GpGg.IntError(*this, CToken-1, errormsg);
			}
			if(!Eq(CToken++, "]"))
				GpGg.IntError(*this, CToken-1, errormsg);
			gpfree_array(&(iteration_udv->udv_value));
			gpfree_string(&(iteration_udv->udv_value));
			iteration_udv->udv_value.SetInt(iteration_start);
		}
		else if(Eq(CToken++, "in")) {
			iteration_string = TryToGetString();
			if(!iteration_string)
				GpGg.IntError(*this, CToken-1, errormsg);
			if(!Eq(CToken++, "]"))
				GpGg.IntError(*this, CToken-1, errormsg);
			iteration_start = 1;
			iteration_end = GpGg.Ev.GpWords(iteration_string);
			gpfree_array(&(iteration_udv->udv_value));
			gpfree_string(&(iteration_udv->udv_value));
			Gstring(&(iteration_udv->udv_value), gp_word(iteration_string, 1));
		}
		else // Neither [i=B:E] or [s in "foo"] 
			GpGg.IntError(*this, CToken-1, errormsg);
		iteration_current = iteration_start;
		empty_iteration = false;
		if(iteration_udv && ((iteration_end > iteration_start && iteration_increment < 0) || (iteration_end < iteration_start && iteration_increment > 0))) {
			empty_iteration = true;
			FPRINTF((stderr, "Empty iteration\n"));
		}

		/* Allocating a node of the linked list nested iterations. */
		/* Iterating just once is the same as not iterating at all */
		/* so we skip building the node in that case.		   */
		if(iteration_start == iteration_end)
			just_once = true;
		if(iteration_start < iteration_end && iteration_end < iteration_start + iteration_increment)
			just_once = true;
		if(iteration_start > iteration_end && iteration_end > iteration_start + iteration_increment)
			just_once = true;
		if(!just_once) {
			this_iter = (t_iterator *)malloc(sizeof(t_iterator));
			this_iter->iteration_udv = iteration_udv;
			this_iter->iteration_string = iteration_string;
			this_iter->iteration_start = iteration_start;
			this_iter->iteration_end = iteration_end;
			this_iter->iteration_increment = iteration_increment;
			this_iter->iteration_current = iteration_current;
			this_iter->iteration = iteration;
			this_iter->done = false;
			this_iter->really_done = false;
			this_iter->empty_iteration = empty_iteration;
			this_iter->next = NULL;
			this_iter->prev = NULL;
			if(nesting_depth == 0) {
				// first "for" statement: this will be the listhead
				iter = this_iter;
			}
			else {
				// not the first "for" statement: attach the newly created node to the end of the list
				iter->prev->next = this_iter; /* iter->prev points to the last node of the list */
				this_iter->prev = iter->prev;
			}
			iter->prev = this_iter; /* a shortcut: making the list circular */
			// if one iteration in the chain is empty, the subchain of nested iterations is too
			SETIFZ(iter->empty_iteration, empty_iteration);
			nesting_depth++;
		}
	}
	return iter;
}
//
// Set up next iteration.
// Return true if there is one, false if we're done
//
bool next_iteration(t_iterator * pIter)
{
	bool condition = false;
	if(pIter && !pIter->empty_iteration) {
		// Support for nested iteration: we start with the innermost loop.
		t_iterator * p_iter = pIter->prev; /* linked to the last element of the list */
		if(p_iter) {
			while(!pIter->really_done && p_iter != pIter && p_iter->done) {
				p_iter->iteration_current = p_iter->iteration_start;
				p_iter->done = false;
				if(p_iter->iteration_string) {
					gpfree_string(&(p_iter->iteration_udv->udv_value));
					Gstring(&(p_iter->iteration_udv->udv_value), gp_word(p_iter->iteration_string, p_iter->iteration_current));
				}
				else {
					gpfree_string(&(p_iter->iteration_udv->udv_value));
					p_iter->iteration_udv->udv_value.SetInt(p_iter->iteration_current);
				}
				p_iter = p_iter->prev;
			}
			if(!p_iter->iteration_udv) {
				p_iter->iteration = 0;
			}
			else {
				pIter->iteration++;
				// don't increment if we're at the last iteration
				if(!pIter->really_done)
					p_iter->iteration_current += p_iter->iteration_increment;
				if(p_iter->iteration_string) {
					gpfree_string(&(p_iter->iteration_udv->udv_value));
					Gstring(&(p_iter->iteration_udv->udv_value), gp_word(p_iter->iteration_string, p_iter->iteration_current));
				}
				else {
					// This traps fatal user error of reassigning iteration variable to a string
					gpfree_string(&(p_iter->iteration_udv->udv_value));
					p_iter->iteration_udv->udv_value.SetInt(p_iter->iteration_current);
				}
				// Mar 2014 revised to avoid integer overflow
				if(p_iter->iteration_increment > 0 && p_iter->iteration_end - p_iter->iteration_current < p_iter->iteration_increment)
					p_iter->done = true;
				else if(p_iter->iteration_increment < 0 && p_iter->iteration_end - p_iter->iteration_current > p_iter->iteration_increment)
					p_iter->done = true;
				else
					p_iter->done = false;
				// We return false only if we're, um, really done
				p_iter = pIter;
				while(p_iter) {
					condition = condition || (!p_iter->done);
					p_iter = p_iter->next;
				}
				if(!condition) {
					if(!pIter->really_done) {
						pIter->really_done = true;
						condition = true;
					}
					else
						condition = false;
				}
			}
		}
	}
	return condition;
}

bool empty_iteration(t_iterator * iter)
{
	return iter ? iter->empty_iteration : false;
}

t_iterator * cleanup_iteration(t_iterator * iter)
{
	while(iter) {
		t_iterator * next = iter->next;
		free(iter->iteration_string);
		free(iter);
		iter = next;
	}
	return NULL;
}

void GpParser::CleanupSetIterator()
{
	P_SetIterator = cleanup_iteration(P_SetIterator);
}

void GpParser::CleanupPlotIterator()
{
	P_PlotIterator = cleanup_iteration(P_PlotIterator);
}

bool forever_iteration(t_iterator * iter)
{
	return iter ? (iter->iteration_end == INT_MAX) : false;
}
//
// The column() function requires special handling because
//   - It has side effects if reference to a column entry
//     requires matching it to the column header string.
//   - These side effects must be handled at the time the
//     expression is parsed rather than when it it evaluated.
//
//static void set_up_columnheader_parsing(AtEntry * previous)
void GpParser::SetupColumnheaderParsing(const AtEntry & rPrevious)
{
	// column("string") means we expect the first row of 
	// a data file to contain headers rather than data.  
	if(rPrevious.Index == PUSHC && rPrevious.arg.v_arg.type == STRING)
		Parse1stRowAsHeaders = true;
	// This allows plot ... using (column(<const>)) title columnhead
	if(rPrevious.Index == PUSHC && rPrevious.arg.v_arg.type == INTGR) {
		SETMAX(AtHighestColumnUsed, rPrevious.arg.v_arg.v.int_val);
	}
	// This attempts to catch plot ... using (column(<variable>))
	if(rPrevious.Index == PUSH) {
		const UdvtEntry * u = rPrevious.arg.udv_arg;
		if(u->udv_value.type == INTGR) {
			SETMAX(AtHighestColumnUsed, u->udv_value.v.int_val);
		}
#if(0) // Currently handled elsewhere, but could be done here instead
		if(u->udv_value.type == STRING) {
			Parse1stRowAsHeaders = true;
		}
#endif
	}
	// NOTE: There is no way to handle ... using (column(<general expression>))
}

