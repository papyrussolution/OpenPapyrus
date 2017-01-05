/*
 * $Id: parse.h,v 1.32 2016/02/08 00:51:11 sfeam Exp $
 */

/* GNUPLOT - parse.h */

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

#ifndef PARSE_H
#define PARSE_H

#include "syscfg.h"
#include "gp_types.h"
#include "eval.h"

struct t_iterator {
	t_iterator * next;  // doubly linked list
	t_iterator * prev;
	UdvtEntry *iteration_udv;
	char *iteration_string;
	int iteration_start;
	int iteration_end;
	int iteration_increment;
	int iteration_current;
	int iteration;
	bool done;
	bool really_done;
	bool empty_iteration;
};

class GpParser {
public:
	GpParser()
	{
		for(uint i = 0; i < MAX_NUM_VAR; i++) {
			SetDummyVar[i][0] = 0;
		}
		STRNSCPY(SetDummyVar[0], "x");
		STRNSCPY(SetDummyVar[1], "y");
		MEMSZERO(FitDummyVar);
		MEMSZERO(CDummyVar);
		AtHighestColumnUsed = 0;
		P_DfArray = 0;
		P_PlotIterator = 0;
		P_SetIterator = 0;
		IsScanningRangeInProgress = false;
		Parse1stRowAsHeaders = false;
		IsStringResultOnly = false;

		parse_recursion_level = 0;
		at = NULL;
		at_size = 0;
	}
	t_value * ConstExpress(t_value * valptr);
	t_value * ConstStringExpress(t_value * valptr);
	char * StringOrExpress(AtType ** atptr);
	AtType * PermAt();
	AtType * TempAt();
	void   ExtendAt();
	GpArgument * AddAction(enum operators sf_index);

	void   CleanupSetIterator();
	void   CleanupPlotIterator();

	void   ParseExpression();
	void   ParsePrimaryExpression();
	void   ParseUnaryExpression();
	void   ParseMultiplicativeExpression();
	int    ParseAssignmentExpression();
	int    ParseArrayAssignmentExpression();
	void   ParseAdditiveExpression();
	void   ParseBitshiftExpression();
	void   ParseRelationalExpression();
	void   ParseEqualityExpression();
	void   ParseConditionalExpression();
	void   ParseLogicalOrExpression();
	void   ParseLogicalAndExpression();
	void   ParseExclusiveOrExpression();
	void   ParseInclusiveOrExpression();
	void   ParseAndExpression();
	void   ParseSumExpression();

	void   ParseLinkVia(UdftEntry * udf);
	void   ParseResetAfterError();

	void   AcceptMultiplicativeExpression();
	void   AcceptAdditiveExpression();
	void   AcceptBitshiftExpression();
	void   AcceptRelationalExpression();
	void   AcceptEqualityExpression();
	void   AcceptLogicalOrExpression();
	void   AcceptLogicalAndExpression();
	void   AcceptAndExpression();
	void   AcceptExclusiveOrExpression();
	void   AcceptInclusiveOrExpression();

	void   SetupColumnheaderParsing(AtEntry * previous);
	//
	char   SetDummyVar[MAX_NUM_VAR][MAX_ID_LEN+1]; // The choice of dummy variables, as set by 'set dummy', 
		// 'set polar' and 'set parametric'
	int    FitDummyVar[MAX_NUM_VAR]; // Dummy variables referenced by name in a fit command 
		// Sep 2014 (DEBUG) used to deduce how many independent variables 
	// the currently used 'dummy' variables. Usually a copy of
	// GpP.SetDummyVar, but may be changed by the '(s)plot' command
	// containing an explicit range (--> 'plot [phi=0..pi]') 
	char   CDummyVar[MAX_NUM_VAR][MAX_ID_LEN+1];
	int    AtHighestColumnUsed; // This is used by plot_option_using() 
	UdvtEntry * P_DfArray; // This is used by df_open() and df_readascii() 
	t_iterator * P_PlotIterator; // Used for plot and splot
	t_iterator * P_SetIterator;  // Used by set/unset commands 
	bool IsScanningRangeInProgress;
	bool Parse1stRowAsHeaders; // This is checked by df_readascii()
	bool IsStringResultOnly;
private:
	int    parse_recursion_level;
	AtType * at;
	int    at_size;
};

extern GpParser GpP;
/*
//
// externally usable types defined by parse.h 
//
// exported variables of parse.c 
//
extern bool scanning_range_in_progress;
// The choice of dummy variables, as set by 'set dummy', 'set polar' and 'set parametric'
extern char GpP.SetDummyVar[MAX_NUM_VAR][MAX_ID_LEN+1];
// Dummy variables referenced by name in a fit command 
// Sep 2014 (DEBUG) used to deduce how many independent variables 
extern int  GpP.FitDummyVar[MAX_NUM_VAR];
// the currently used 'dummy' variables. Usually a copy of
// GpP.SetDummyVar, but may be changed by the '(s)plot' command
// containing an explicit range (--> 'plot [phi=0..pi]') 
extern char GpP.CDummyVar[MAX_NUM_VAR][MAX_ID_LEN+1];
extern int GpP.AtHighestColumnUsed; // This is used by plot_option_using() 
extern bool GpP.Parse1stRowAsHeaders; // This is checked by df_readascii()
extern UdvtEntry *GpP.P_DfArray; // This is used by df_open() and df_readascii() 
//
// Protection mechanism for trying to parse a string followed by a + or - sign.
// Also suppresses an undefined variable message if an unrecognized token
// is encountered during GpC.TryToGetString().
//
extern bool GpP.IsStringResultOnly;
extern t_iterator * GpP.P_PlotIterator; // Used for plot and splot
extern t_iterator * GpP.P_SetIterator;  // Used by set/unset commands 
*/
//
// Prototypes of exported functions in parse.c
//
int    int_expression();
double real_expression();
//void   parse_reset_after_error();
//t_value * const_string_express(t_value *valptr);
//t_value * const_express(t_value *valptr);
//char * string_or_express(AtType **atptr);
//AtType * temp_at();
//AtType * perm_at();
AtType * create_call_column_at(char *);
UdvtEntry * add_udv(int t_num);
UdftEntry * add_udf(int t_num);
void   cleanup_udvlist();
//
// These are used by the iteration code 
//
t_iterator * check_for_iteration();
bool next_iteration(t_iterator *);
bool empty_iteration(t_iterator *);
bool forever_iteration(t_iterator *);
t_iterator * cleanup_iteration(t_iterator *);
//void parse_link_via(UdftEntry *);

#endif /* PARSE_H */
