/*
    CCCC - C and C++ Code Counter
    Copyright (C) 1994-2005 Tim Littlefair (tim_littlefair@hotmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
*/

// ada95.g

// derived from the publicly available files grammar9x.y and lexer9x.l
// from Intermetrics, available from:
// somewhere on the internet

// the Intermetrics notices:

/******* A YACC grammar for Ada 9X *********************************/
/* Copyright (C) Intermetrics, Inc. 1994 Cambridge, MA  USA        */
/* Copying permitted if accompanied by this statement.             */
/* Derivative works are permitted if accompanied by this statement.*/
/* This grammar is thought to be correct as of May 1, 1994         */
/* but as usual there is *no warranty* to that effect.             */
/*******************************************************************/
/******* A "lex"-style lexer for Ada 9X ****************************/
/* Copyright (C) Intermetrics, Inc. 1994 Cambridge, MA  USA        */
/* Copying permitted if accompanied by this statement.             */
/* Derivative works are permitted if accompanied by this statement.*/
/* This lexer is known to be only approximately correct, but it is */
/* more than adequate for most uses (the lexing of apostrophe is   */
/* not as sophisticated as it needs to be to be "perfect").        */
/* As usual there is *no warranty* but we hope it is useful.       */
/*******************************************************************/
#header <<

#include "cccc.h"
#include "cccc_ast.h"
#include "cccc_utl.h"

// the objects which PCCTS creates for ASTs as the #0 variable etc
// have type "pointer to ASTBase", which means they need to be cast
// to a pointer to my variant of AST if I want to call my AST
// methods on them
#define MY_AST(X) ( (AST*) X)

>>

#lexaction		
<<
	void lex_error(int tok)
	{
		cerr << "Accepting lexical error token " << tok << endl;
	}
>>

#token Eof		"@"				<<replstr("<EOF>");>>

#token ADA_COMMENT	"\-\- ~[\n]* \n"	
<< 
    IncrementCount(tcCOMLINES);
    newline(); 
    skip(); 
>>


#token WHITESPACE	"[\ \r\t\f]"			<< skip(); >>
#token NEWLINE		"\n"				<< 
    if(ANTLRToken::bCodeLine!=0)
    {
	IncrementCount(tcCODELINES);
    }
    newline();
    skip(); 
>>

#token LPAREN			"\("		<<;>>
#token RPAREN			"\)"		<<;>>
#token LBRACK			"\["		<<;>>
#token RBRACK			"\]"		<<;>>
#token LBRACE			"\{"		<<;>>
#token RBRACE			"\}"		<<;>>
#token PIPE			"\|"		<<;>>
#token PLUS			"\+"		<<;>>
#token MINUS			"\-"		<<;>>
#token TIMES			"\*"		<<;>>
#token DIVIDE			"\/"		<<;>>


// Ada provides support for explicitly requesting short circuit evaluation
// on AND THEN and OR ELSE boolean expressions.   
// We need to count these as increments to the McCabe value, as they introduce
// new paths through the code.
// To do this we need to trick the lexer into doing lookahead for us

#token AND			"and"		<< mode(AND_RECOGNIZED); >>
#lexclass AND_RECOGNIZED
#token AND_WS 			"[\ \t\r\f]"	<< skip(); >>
#token AND_NL			"\n"		<< newline(); skip(); >>
#token THEN_AFTER_AND		"then"		
<< 
	IncrementCount(tcMCCABES_VG);
	mode(START); 
>>
#token AND_OTHER1		"~[]"	<< _endexpr-=1; mode(START); skip(); >>
#token AND_OTHER2		"t~[]"	<< _endexpr-=2; mode(START); skip(); >>
#token AND_OTHER3		"th~[]"	<< _endexpr-=3; mode(START); skip(); >>
#token AND_OTHER4		"the~[]" 
					<< _endexpr-=4; mode(START); skip(); >>
#lexclass START

#token OR			"or"		<< mode(OR_RECOGNIZED); >>
#lexclass OR_RECOGNIZED
#token OR_WS 			"[\ \t\r\f]"	<< skip(); >>
#token OR_NL			"\n"		<< newline(); skip(); >>
#token ELSE_AFTER_OR		"else"		
<< 
	IncrementCount(tcMCCABES_VG);
	mode(START); 
>>
#token OR_OTHER1		"~[]"	<< _endexpr-=1; mode(START); skip(); >>
#token OR_OTHER2		"e~[]"	<< _endexpr-=2; mode(START); skip(); >>
#token OR_OTHER3		"el~[]"	<< _endexpr-=3; mode(START); skip(); >>
#token OR_OTHER4		"els~[]" 
					<< _endexpr-=4; mode(START); skip(); >>
#lexclass START


// We want to use nesting levels to control resynchronisation strategies after
// unrecognized text.
// in C, C++ and Java, the useful nesting relation is simple: 
//    depth=number of "{" seen - number of "}" seen
// (this assumes that people do not use the preprocessor to hide or create
// more of one kind of brace than the other)
// in Ada, the useful nesting is BEGIN ... END, but there are some 
// complications as END tokens are used to match with tokens other than the
// BEGIN, including IS, DO, LOOP, SELECT, RECORD, IF and CASE.
// Of these:
//	IF, SELECT, LOOP and DO stand in positions similar to an opening 
//	BEGIN, before groups of statements, so we treat them as an alternate 
//	forms of nesting depth increase,
//	IS before a block of statements or declarations matches an END at the
//	end
// 	RECORD at the start of a group of declarations matches an END RECORD
// 	at the end.
// Unfortunately, IS is used quite widely, sometimes in contexts where 
// it does not need a matching END, sometimes where it does.  Detecting this
// in the lexer is not trivial
 
#token PERIOD			"."		<<;>>
#token DOT_DOT			".."		<<;>>
#token LT_LT			"\<\<"		<<;>>
#token BOX			"\<\>"		<<;>>
#token LT_EQ			"\<="		<<;>>
#token EXPON			"\*\*"		<<;>>
#token NE			"\/="		<<;>>
#token GT_GT			"/>/>"		<<;>>
#token GE			"\>="		<<;>>
#token IS_ASSIGNED		":="		<<;>>
#token RIGHT_SHAFT		"=\>"		<<;>>
#token ABORT			"abort"		<<;>>
#token ABS			"abs"		<<;>>
#token ABSTRACT			"abstract"	<<;>>
#token ACCEPT			"accept"	<<;>>
#token ACCESS			"access"	<<;>>
#token ALIASED			"aliased"	<<;>>
#token ALL			"all"		<<;>>
#token ARRAY			"array"		<<;>>
#token AT			"at"		<<;>>
#token BEGiN			"begin"		<<;>>
#token BODY			"body"		<<;>>

#token CASE			"case"		
<<
	IncrementCount(tcMCCABES_VG);
>>

#token CONSTANT			"constant"	<<;>>
#token DECLARE			"declare"	<<;>>
#token DELAY			"delay"		<<;>>
#token DELTA			"delta"		<<;>>
#token DIGITS			"digits"	<<;>>
#token DO			"do"		<<;>>
#token ELSE			"else"		<<;>>
#token ELSIF			"elsif"		
<<
	IncrementCount(tcMCCABES_VG);
>>

#token END			"end"		<<;>>
#token ENTRY			"entry"		<<;>>
#token EXCEPTION		"exception"	<<;>>
#token EXIT			"exit"		<<;>>
#token FOR			"for"		
<<
	IncrementCount(tcMCCABES_VG);
>>

#token FUNCTION			"function"	<<;>>
#token GENERIC			"generic"	<<;>>
#token GOTO			"goto"		<<;>>
#token IF			"if"		
<<
	IncrementCount(tcMCCABES_VG);
>>

#token IN			"in"		<<;>>
#token IS			"is"		<<;>>
#token LIMITED			"limited"	<<;>>
#token LOOP			"loop"		<<;>>
#token MOD			"mod"		<<;>>
#token NEW			"new"		<<;>>
#token NOT			"not"		<<;>>
#token NuLL			"null"		<<;>>
#token OF			"of"		<<;>>

#token OTHERS			"others"	<<;>>
#token OUT			"out"		<<;>>
#token PACKAGE			"package"	<<;>>
#token PRAGMA			"pragma"	<<;>>
#token PRIVATE			"private"	<<;>>
#token PROCEDURE		"procedure"	<<;>>
#token PROTECTED		"protected"	<<;>>
#token RAISE			"raise"		<<;>>
#token RANGE			"range"		<<;>>
#token RECORD			"record"	<<;>>
#token REM			"rem"		<<;>>
#token RENAMES			"renames"	<<;>>
#token REQUEUE			"requeue"	<<;>>
#token RETURN			"return"	<<;>>
#token REVERSE			"reverse"	<<;>>
#token SELECT			"select"	<<;>>
#token SEPARATE			"separate"	<<;>>
#token SUBTYPE			"subtype"	<<;>>
#token TAGGED			"tagged"	<<;>>
#token TASK			"task"		<<;>>
#token TERMINATE		"terminate"	<<;>>
#token THEN			"then"		<<;>>
#token TYPE			"type"		<<;>>
#token UNTIL			"until"		<<;>>
#token USE			"use"		<<;>>
#token WHEN			"when"		<<;>>
#token WHILE			"while"		
<<
	IncrementCount(tcMCCABES_VG);
>>

#token WITH			"with"		<<;>>
#token XOR			"xor"		<<;>>

#token IDENTIFIER	"[a-zA-Z][_a-zA-Z0-9]*"		<<;>>
#token CHAR_STRING 	"\"~[\"]*\""	  		<<;>>

// there is a very nasty lexer ambiguity between single quote as the start of
// a character literal, and single quote as the modifier for an attribute
// to resolve this, we use the lexclass processing below

#token TIC 	"'"  	<< mode(TIC_SEEN); >>
#lexclass TIC_SEEN
#token TIC_CHAR_TIC	"~[]'"	<< mode(START); >>
#token TIC_LPAREN "\("	<< mode(START); return LPAREN; >>
#token TIC_IDENTIFIER	"[a-zA-Z][_a-zA-Z0-9]+"		
			<< mode(START); return IDENTIFIER; >>
#token TIC_SHORT_ID	"[a-zA-Z]" << mode(START); return IDENTIFIER; >>
#token TIC_ERROR	"~[]"	<< mode(START); lex_error(TIC_ERROR); >>
#lexclass START

/* 
these are the lex patterns which build up into DECIMAL_LITERAL and 
BASED_LITERAL which are the two forms of numeric literal used in the 
original lex/yacc grammar

I've implemented them as rules, based on component token patterns as below 

EXTENDED_DIGIT          [0-9a-zA-Z]
INTEGER                 ({DIGIT}(_?{DIGIT})*)
EXPONENT                ([eE](\+?|-){INTEGER})
DECIMAL_LITERAL         {INTEGER}(\.?{INTEGER})?{EXPONENT}?
BASE                    {INTEGER}
BASED_INTEGER           {EXTENDED_DIGIT}(_?{EXTENDED_DIGIT})*
BASED_LITERAL           {BASE}#{BASED_INTEGER}(\.{BASED_INTEGER})?#{EXPONENT}?
*/

#token DECIMAL_STRING 	"[0-9_]+"	<< ; >>
#token DECIMAL_EXPON	"[Ee]{[\+\-]}"	<< ; >>
#token BASED_INDIC_BEG	"#"		<< mode(BASED); >>

#lexclass BASED
#token BASED_STRING	"[_0-9A-Za-z]+"	<< ; >>
#token BASED_POINT	"."		<< ; >>
#token BASED_INDIC_END	"#"		<< mode(START); >>
// we don't expect to see anything except the tokens above while in 
// this mode, but just in case...
#token BASED_ERROR	"~[]"	<< mode(START); lex_error(BASED_ERROR); >>

#lexclass START
#tokclass RESYNCHRONISATION { ";" } 

class AdaPrser {
<<
	ParseUtility ps;

#define TOKEN_INFO "\t" << LT(1)->getText() << " on line " << LT(1)->getLine()

void tracein(char *rulename) { ps.tracein(rulename,guessing,LT(1)); }
void traceout(char *rulename) { ps.traceout(rulename,guessing,LT(1)); }
void syn(
  _ANTLRTokenPtr tok, ANTLRChar *egroup, SetWordType *eset,
  ANTLRTokenType etok, int k
) 
{ 
	if(DebugMask&PARSER) { ps.syn(tok); } 
}


/* Parser Members */
  string parse_language;

public:
  void init(const string& filename, const string& language)
  {
    ps.reset(this);
    ps.set_string(pssFILE, filename);
    parse_language=language;
    ANTLRParser::init();
  }

>>

goal_symbol : compilation Eof
	;

// numeric literal handling
numeric_literal :
	  decimal_literal
	| based_literal
	;

decimal_literal :
	  DECIMAL_STRING { decimal_fractional_part } { decimal_exponent }
	| decimal_fractional_part { decimal_exponent }
	;

decimal_fractional_part :
	  PERIOD DECIMAL_STRING
	;	

decimal_exponent :
	  DECIMAL_EXPON DECIMAL_STRING
	;

based_literal :
	  DECIMAL_STRING based_number { decimal_exponent }
	;

based_number :
	  BASED_INDIC_BEG based_float BASED_INDIC_END
	;

based_float :
	  BASED_STRING { BASED_POINT BASED_STRING } 
	| BASED_POINT BASED_STRING
	; 	

error : 
	  << (LT(1)->getType() == 9999) >>? "!!!!!!" ";"
	;


pragma! : PRAGMA simple_name { LPAREN pragma_arg_s RPAREN } ";"
	;

pragma_arg_s : 
	  pragma_arg ( "," pragma_arg )*
	| /* empty */
	;

pragma_arg : 
	  (simple_name RIGHT_SHAFT)? simple_name RIGHT_SHAFT expression
	| expression
	;

pragma_s : 
	  ( pragma )* 
	;


/* recast for LL parsing */
def_id_s : def_id ( "," def_id )*
	;

def_id  : IDENTIFIER
	;

object_qualifier_opt :
	  ( ALIASED CONSTANT )?
	| ALIASED
	| CONSTANT
	| /* empty */
	;

object_subtype_def : 
	  subtype_ind
	| array_type
	;

init_opt! :
	  IS_ASSIGNED expression
	| /* empty */
	;

object_or_number_decl : 
	  def_id_s ":" object_or_number_decl_completion
	;

object_or_number_decl_completion : 
	  CONSTANT IS_ASSIGNED expression ";"
	| object_qualifier_opt object_subtype_def init_opt ";"
	;

type_decl : TYPE IDENTIFIER discrim_part_opt type_completion ";"
	;

discrim_part_opt :
	  discrim_part
	| LPAREN BOX RPAREN
	| /* empty */
	;

type_completion :
	  IS type_def
	| /* empty */
	;

type_def : 
	  (private_type)? 
	| enumeration_type  
	| integer_type 
	| real_type 
	| array_type 
	| record_type 
	| access_type 
	| derived_type 
	;

subtype_decl : SUBTYPE IDENTIFIER IS subtype_ind ";"
	;

subtype_ind : 
	  ( name constraint )?
	| name
	;

constraint : 
	  range_constraint
	| decimal_digits_constraint
	;

decimal_digits_constraint : DIGITS expression range_constr_opt
	;

derived_type : 
	  { ABSTRACT } NEW subtype_ind subtype_opt_with_extension
	;

subtype_opt_with_extension :
	  WITH PRIVATE
	| WITH record_def
	| /* empty */
	;

range_constraint : RANGE range
	;

range :
	  ( name TIC RANGE LPAREN expression RPAREN )?
	| ( name TIC RANGE )?
	| simple_expression DOT_DOT simple_expression
	;

enumeration_type : LPAREN enum_id_s RPAREN
	;

enum_id_s : enum_id ( "," enum_id )*
	;

enum_id : IDENTIFIER
	| char_lit
	;

char_lit : TIC TIC_CHAR_TIC 
	;

integer_type : range_spec
	| MOD expression
	;
	

range_spec : range_constraint
	;

range_spec_opt :
	  range_spec
	| /* empty */
	;

real_type : 
	  float_type
	| fixed_type
	;

float_type : DIGITS expression range_spec_opt
	;

fixed_type : DELTA expression fixed_type_suffix
	;

fixed_type_suffix :
	  range_spec
	| DIGITS expression range_spec_opt
	;

array_type : 
	  ARRAY LPAREN array_index_spec RPAREN OF component_subtype_def
	;

array_index_spec :
	  ( iter_discrete_range_s )?
	| index_s 
	;

component_subtype_def : { ALIASED } subtype_ind
	;

index_s : index ( "," index )*
	;

index : name RANGE BOX
	;

iter_index_constraint : LPAREN iter_discrete_range_s RPAREN
	;

iter_discrete_range_s : discrete_range ( "," discrete_range )*
	;

discrete_range : 
	| ( name range_suffix )?
	| ( simple_expression DOT_DOT )? 
	    simple_expression DOT_DOT simple_expression
	| name
	;

range_constr_opt :
	| range_constraint
	;

record_type : tagged_opt limited_opt record_def
	;

record_def : RECORD pragma_s comp_list END RECORD
	| NuLL RECORD
	;

tagged_opt :
	| TAGGED
	| ABSTRACT TAGGED
	;

comp_list : comp_decl_s variant_part_opt
	| variant_part pragma_s
	| NuLL ";" pragma_s
	;

comp_decl_s : 
	  comp_decl comp_decl_or_pragma_list
	;

comp_decl_or_pragma_list :
	  (comp_decl)? comp_decl comp_decl_s
	| (PRAGMA)? pragma comp_decl_s
	| empty
	;

variant_part_opt : pragma_s (variant_part pragma_s)*
	;

comp_decl : def_id_s ":" component_subtype_def init_opt ";"
	| error ";"
	;

discrim_part : LPAREN discrim_spec_s RPAREN
	;

discrim_spec_s : discrim_spec ( ";" discrim_spec )*
	;

discrim_spec : def_id_s ":" { ACCESS } mark init_opt
	| error
	;

variant_part : CASE simple_name IS pragma_s variant_s END CASE ";"
	;

variant_s : ( variant )*
	;

variant : WHEN choice_s RIGHT_SHAFT pragma_s comp_list
	;

choice_s : choice ( PIPE choice )*
	;

choice : 
	  ( discrete_with_range )?
	| expression
	| OTHERS
	;

discrete_with_range : 
	  ( name RANGE)? name range_suffix 
	| ( name TIC RANGE )? name range_suffix
	| simple_expression DOT_DOT simple_expression
	;

range_suffix :
	  range_constraint
	| TIC RANGE { LPAREN expression RPAREN }
	;

access_type : ACCESS subtype_ind
	| ACCESS CONSTANT subtype_ind
	| ACCESS ALL subtype_ind
	| ACCESS { PROTECTED } proc_func_spec 
	;

proc_func_spec :
	  PROCEDURE formal_part_opt
  	| FUNCTION formal_part_opt RETURN mark
	;

decl_part : decl_item_or_body_s1
	;

decl_item_s :
	  (END|PRIVATE)? empty
	| decl_item_or_body decl_item_s
	;

empty : /* nothing */ 
	;

decl_item_or_body_s1 : decl_item_or_body decl_item_s 
	;

decl_item_or_body :
	<<
	// prepare in case we need to resynchronize
	ANTLRTokenPtr initial_token=LT(1);
	string initial_text=ps.lookahead_text(3);
	>>
	  ( generic_decl )?
	| ( rename_decl )?
	| type_decl 
	| prot_decl
	| subtype_decl
	| pkg_decl
	| task_decl_or_body
	| subprog_decl_or_body 
	| use_clause
	| rep_spec
	| pragma
	| pkg_body
	| prot_body
	| (exception_decl)?
	| object_or_number_decl 
	;
	<<
{
  ANTLRTokenPtr resync_token;
  int resync_nesting=mytoken(initial_token)->getNestingLevel();
  ps.resynchronize(resync_nesting,RESYNCHRONISATION_set,resync_token);

  cerr << "Syntax error: parser failed to handle "
       << initial_text << "..." << resync_token->getText()
	<< " on lines " << initial_token->getLine() 
	<< " to " << resync_token->getLine() << endl;

  // now we build an AST representing the rejected area...
  initial_token->setText(initial_text.c_str());
  AST *rejected_ast=new AST(initial_token);
  AST *rejected_ast_end=new AST(resync_token);
  rejected_ast->setRight(rejected_ast_end);
  ps.record_rejected_extent(rejected_ast);
  
  // we only delete the root tree - it deletes the other one
  delete rejected_ast;
}
	>>

name : 
	  simple_name name_extension_list
	| operator_symbol
	;

name_extension_list :
	  (LPAREN|TIC|PERIOD)? extension_item name_extension_list
	| /* empty */
	;

extension_item :
	  index_extension
	| selection_extension
	| attribute_extension
	;


mark : simple_name ( mark_extension )*
	;

mark_extension :
	  TIC! attribute_id!
	| PERIOD simple_name
	;

simple_name : IDENTIFIER
	;

compound_name : simple_name ( PERIOD simple_name )*
	;

compound_member_name :
	   first_name_element extra_name_elements
	;

first_name_element :
	   nm:IDENTIFIER
	<< ps.set_string(pssMEMBER,$nm->getText()); >>
	;

extra_name_elements :
	  /* empty */
	| PERIOD nm:IDENTIFIER extra_name_elements
	<<
	    ps.set_string(pssMODULE,ps.get_string(pssMEMBER));
	    ps.set_string(pssMEMBER,$nm->getText());
	>>
	; 

c_name_list : compound_name ( "," compound_name )*
	;

used_char : CHAR_LIT
	;

operator_symbol : CHAR_STRING
	;

index_extension :
	  LPAREN  value_s RPAREN
	;

value_s : value ( "," value )*
	;

value : 
	  ( discrete_with_range )?
	| choice_s opt_assoc
	| error
	;

opt_assoc :
	  (RIGHT_SHAFT)? RIGHT_SHAFT expression
	| /* empty */
	; 

selection_extension :
	  PERIOD simple_name
	| PERIOD used_char
	| PERIOD operator_symbol
	| PERIOD ALL
	;

attribute_extension :
	  TIC attribute_id
	;

attribute_id : IDENTIFIER
	| DIGITS
	| DELTA
	| ACCESS
	;

literal : numeric_literal
	| used_char
	| NuLL
	;

aggregate : LPAREN aggregate_contents RPAREN
	;

aggregate_contents :
	  ( value_s_2 )? 
	| (choice_s RIGHT_SHAFT)? comp_assoc 
	| expression WITH aggregate_with_operand
	| NuLL RECORD
	;

aggregate_with_operand :
	  value_s 
	| NuLL RECORD
	;

value_s_2 : value "," value ( "," value )*
	;

comp_assoc : choice_s RIGHT_SHAFT expression
	;

expression : relation expression_extension
	;

expression_extension :
	  (logical)? logical expression
	| (short_circuit)? short_circuit expression
	| /* empty */
	;

logical : AND
	| OR
	| XOR
	;

short_circuit : AND THEN_AFTER_AND
	| OR ELSE_AFTER_OR
	;

relation : simple_expression relation_extension
	;

relation_extension :
	  ( membership range )?
	| membership name
	| ( relational simple_expression )*
	;

relational : "="
	| NE
	| "<"
	| LT_EQ
	| ">"
	| GE
	;

membership : IN
	| NOT IN
	;

simple_expression : 
	  (unary)? unary term
	| term ( adding term )*
	;

unary   : PLUS
	| MINUS
	;

adding  : PLUS
	| MINUS
	| "&"
	;

term    : factor ( multiplying factor )*
	;

multiplying : TIMES
	| DIVIDE
	| MOD
	| REM
	;

factor : 
	  NOT primary
	| ABS primary
	| primary ( EXPON primary )*
	;

primary : literal
	| (qualified)?
	| name
	| allocator
	| parenthesized_primary
	;

parenthesized_primary : LPAREN paren_primary_contents RPAREN
	;

// in the original grammar, the options here are value_s_2 (i.e. at least
// two patterns matching value, separated by commas) or expression
// as expression matches value, this creates an ambiguity, so we relax the
// rule a little...
paren_primary_contents :
	  value ( "," value )*
	;

qualified : name TIC parenthesized_primary
	;

allocator : 
	  ( NEW qualified )?
	| NEW name
	;

statement_s :  
	  (statement)? statement statement_s
	| /* empty */
	;

statement : unlabeled
	| label statement
	;

unlabeled : simple_stmt
	| compound_stmt
	| pragma
	;

simple_stmt : null_stmt
	| (assign_stmt)?
	| exit_stmt
	| return_stmt
	| goto_stmt
	| (procedure_call)?
	| delay_stmt
	| abort_stmt
	| raise_stmt
	| code_stmt
	| requeue_stmt
	| error ";"
	;

compound_stmt : if_stmt
	| case_stmt
	| ( loop_stmt )?
	| block
	| accept_stmt
	| select_stmt
	;

label : LT_LT IDENTIFIER GT_GT
	;

null_stmt : NuLL ";"
	;

assign_stmt : name IS_ASSIGNED expression ";"
	;

if_stmt : IF cond_clause_s else_opt END IF ";"
	;

cond_clause_s : cond_clause ( ELSIF cond_clause )*
	;

cond_clause : cond_part statement_s
	;

cond_part : condition THEN
	;

condition : expression
	;

else_opt :
	| ELSE statement_s
	;

case_stmt : case_hdr pragma_s alternative_s END CASE ";"
	;

case_hdr : CASE expression IS
	;

alternative_s :
	| alternative alternative_s
	;

alternative : WHEN choice_s RIGHT_SHAFT statement_s
	;

loop_stmt : label_opt iteration basic_loop id_opt ";"
	;

label_opt :
	| IDENTIFIER ":"
	;

iteration :
	| WHILE condition
	| iter_part reverse_opt discrete_range
	;

iter_part : FOR IDENTIFIER IN
	;

reverse_opt :
	| REVERSE
	;

basic_loop : LOOP statement_s END LOOP
	;

id_opt : 
	| func_designator
	;

block : label_opt block_decl block_body END id_opt ";"
	;

block_decl :
	| DECLARE decl_part
	;

block_body : BEGiN handled_stmt_s
	;

handled_stmt_s : statement_s except_handler_part_opt 
	; 

except_handler_part_opt :
	| except_handler_part
	;

exit_stmt : EXIT name_opt when_opt ";"
	;

name_opt :
	| name
	;

when_opt :
	| WHEN condition
	;

return_stmt : RETURN ";"
	| RETURN expression ";"
	;

goto_stmt : GOTO name ";"
	;

subprog_decl_or_body : 
<< 
	string sp_str;
	UseType ut;
>>
	  subprog_spec
	    subprog_decl_or_body_completion>[ut]
<<
	ps.record_function_extent(MY_AST(#0),ut);
>>
	;

subprog_decl_or_body_completion > [UseType _retv] :
	  (
		  IS SEPARATE ";" 	
	 	| ";" 			
	  )
	<< 
		_retv=utDECLARATION; 
		ps.set_string(pssDESCRIPTION,"declaration");
	>>
	| 
	  (
		  IS ABSTRACT ";" 
		| IS decl_part block_body END id_opt ";" 	
        	| IS generic_inst 	
	  )
	<< 
		_retv=utDEFINITION; 
		ps.set_string(pssDESCRIPTION,"definition");
	>>
	;

subprog_spec : 
	  PROCEDURE proc_designator formal_part_opt
	| FUNCTION func_designator formal_part_opt return_part_opt
	;

proc_designator : << string final_name; >> 
	  compound_member_name
	;

func_designator : << string final_name; >> 
	  proc_designator
 	| opname:CHAR_STRING   
	<< 
		final_name=$opname->getText(); 
		ps.set_string(pssMEMBER,final_name); 
	>>
	;

formal_part_opt : 
	| formal_part
	;

return_part :
	  RETURN n:name 
	<<
		ps.set_string(pssUTYPE,MY_AST(#n)->last_token()->getText());
		ps.set_string(pssDESCRIPTION,"returns by value");
		ps.record_userel_extent(MY_AST(#n),utPARBYVAL);
	>>
	;
	
return_part_opt :
	  return_part
	| /* empty */
	;	

formal_part : LPAREN param_s RPAREN
	;


param_s : param ( ";" param )*
	;

param : << UseType ut; >>
	  def_id_s ":" param_mode>[ut] mk:mark init_opt
	<<
		ps.set_string(pssUTYPE,MY_AST(#mk)->last_token()->getText());
		ps.set_string(pssDESCRIPTION,"parameter");
		ps.record_userel_extent(MY_AST(#mk),ut);
	>>
	;

param_mode >[UseType ut]: << $ut=utPARBYVAL; >>
	| IN
	| OUT
	| IN OUT
	| ACCESS << $ut=utPARBYREF; >>
	;


procedure_call : name ";"
	;

pkg_decl : 
	  normal_or_instance_pkg_spec ";"
	;

pkg_spec :
	  normal_or_instance_pkg_spec
	;

normal_or_instance_pkg_spec :
	  PACKAGE cn:compound_name 
	<<
	     ps.set_string(
		pssMODULE,MY_AST(#cn)->last_token()->getText());
	     ps.set_string(pssMODTYPE,"Ada package");
	>>
	  normal_or_instance_pkg_spec_completion
	;

normal_or_instance_pkg_spec_completion :
	  RENAMES name 
<<
	    ps.set_string(pssDESCRIPTION,"renames definition");
	    ps.record_module_extent(MY_AST(#0), utDEFINITION);
	>>
	| IS generic_inst
	<<
	    ps.set_string(pssDESCRIPTION,"instanced declaration");
	    ps.record_module_extent(MY_AST(#0), utDECLARATION);
	>>
	| IS decl_item_s private_part END c_id_opt
	<<
	    ps.set_string(pssDESCRIPTION,"declaration");
	    ps.record_module_extent(MY_AST(#0), utDECLARATION);
	>>
	;

private_part :
	   << LT(1)->getType() == END >>? /* empty */
	|  << LT(1)->getType() == PRIVATE >>? PRIVATE decl_item_s
	;

c_id_opt : 
	| compound_name
	;

pkg_body : 
	  PACKAGE BODY cn:compound_name 
	<<
	     ps.set_string(
		pssMODULE,MY_AST(#cn)->last_token()->getText());
	     ps.set_string(pssMODTYPE,"Ada package");
	>>
	  IS pkg_body_or_separate ";"
	;

pkg_body_or_separate :
	  SEPARATE
	| decl_part body_opt END c_id_opt 
	;

body_opt :
	| block_body
	;

private_type : tagged_opt limited_opt PRIVATE
	;

limited_opt :
	| LIMITED
	;

use_clause : USE name_s ";"
	| USE TYPE name_s ";"
	;

name_s : name ( "," name )*
	;

rename_decl : 
	  (rename_unit)?
	| def_id_s ":" rename_completion
	;

rename_completion :
	  object_qualifier_opt subtype_ind renames ";"
	| EXCEPTION renames ";"
	;

rename_unit : PACKAGE compound_name renames ";"
	| subprog_spec renames ";"
	| generic_formal_part generic_renames_completion
	;

generic_renames_completion :
	  PACKAGE compound_name renames ";"
	| subprog_spec renames ";"
	;

renames : RENAMES name
	;

task_decl_or_body : task_spec ";"
	;

task_spec : 
	  TASK TYPE simple_name discrim_part_opt task_def
	| TASK BODY simple_name IS body_or_separate 
	| TASK simple_name task_def
	;

body_or_separate :
	  SEPARATE
	| decl_part block_body END id_opt ";"
	;

task_def :
	| IS entry_decl_s rep_spec_s task_private_opt END id_opt
	;

task_private_opt :
	| PRIVATE entry_decl_s rep_spec_s
	;

prot_decl : prot_spec ";"
	;

prot_spec : PROTECTED prot_item
	;

prot_item :
	  IDENTIFIER prot_def
	| TYPE simple_name discrim_part_opt prot_def
	;


prot_def : IS prot_op_decl_s prot_private_opt END id_opt
	;

prot_private_opt :
	| PRIVATE prot_elem_decl_s 
	;

prot_op_decl_s : 
	| prot_op_decl prot_op_decl_s
	;

prot_op_decl : entry_decl
	| subprog_spec ";"
	| rep_spec
	| pragma
	;

prot_elem_decl_s : 
	| prot_elem_decl prot_elem_decl_s
	;

prot_elem_decl : prot_op_decl | comp_decl 
	;

prot_body : PROTECTED BODY simple_name IS prot_separate_or_body ";"
	;

prot_separate_or_body :
	  SEPARATE
	| prot_op_body_s END id_opt 
	;

prot_op_body_s : pragma_s ( prot_op_body pragma_s )*
	;

prot_op_body : entry_body
	| subprog_decl_or_body
	;

entry_decl_s : pragma_s ( entry_decl pragma_s )*
	;

entry_decl : ENTRY IDENTIFIER entry_decl_completion
	;

entry_decl_completion :
 	  ( paren_discrete_range )? paren_discrete_range formal_part_opt ";"
	| formal_part_opt ";"
	;

paren_discrete_range :
	  LPAREN discrete_range RPAREN 
	;

entry_body : ENTRY IDENTIFIER { LPAREN iter_part discrete_range RPAREN } 
		formal_part_opt WHEN condition entry_body_part
	;

entry_body_part : ";"
	| IS decl_part block_body END id_opt ";"
	;

rep_spec_s : ( rep_spec pragma_s )*
	;

entry_call : procedure_call
	;

accept_stmt : accept_hdr  accept_completion
	;

accept_completion :
	  ";"
	| DO handled_stmt_s END id_opt ";"
	;

accept_hdr : ACCEPT entry_name formal_part_opt
	;

entry_name : simple_name paren_expression_list
	;

paren_expression_list :
	 ( LPAREN )? LPAREN expression RPAREN paren_expression_list
	| /* empty */
	;

delay_stmt : DELAY expression ";"
	| DELAY UNTIL expression ";"
	;

select_stmt : SELECT select_completion
	;

select_completion :
          ( select_wait_completion )?
	| ( async_select_completion )?
	| entry_completion
	;

select_wait_completion : 
	  guarded_select_alt or_select else_opt END SELECT ";"
	;

guarded_select_alt : select_alt
	| WHEN condition RIGHT_SHAFT select_alt
	;

or_select : ( OR guarded_select_alt )*
	;

select_alt : accept_stmt stmts_opt
	| delay_stmt stmts_opt
	| TERMINATE ";"
	;

delay_or_entry_alt : 
          delay_stmt stmts_opt
	| entry_call stmts_opt
	;

async_select_completion : 
	  delay_or_entry_alt THEN ABORT statement_s END SELECT ";"
	;

entry_completion : entry_call stmts_opt entry_completion_tail
	;

entry_completion_tail :
	  OR delay_stmt stmts_opt END SELECT ";"
	| ELSE statement_s END SELECT ";"
	;

stmts_opt :
	statement_s | /* empty */
	;

abort_stmt : ABORT name_s ";"
	;

compilation : 
	| comp_unit compilation
	;

comp_unit :
	<< 
		ParseUtility saved_ps=ps; 
	>> 
	  pragma_s cs:opt_context_spec private_opt unit
	<<
{
  ps.set_flag(vPUBLIC);
  ps.set_string(pssDESCRIPTION,"with relationship");
  while(cs != NULL)
  {
    if( 
      (MY_AST(#cs)->token.getType() == IDENTIFIER) &&
      (MY_AST(#cs->right())->token.getType() != PERIOD)
      )
    {
      ps.set_string(pssUTYPE,MY_AST(#cs)->token.getText());
      ps.record_userel_extent(MY_AST(#cs),utWITH);
      ps.set_flag(vPUBLIC);
    }
    else if(MY_AST(#cs)->token.getType() == PRIVATE)
    {
      ps.set_flag(vPRIVATE);
    }
    #cs=MY_AST(#cs->right());
  }
  ps=saved_ps; 
}
	>>
	;

private_opt :
	| PRIVATE
	;

opt_context_spec : 
	  { context_spec }
	;

context_spec : 
       	  with_clause use_clause_opt pragma_with_list
	;

pragma_with_list :
	  (pragma_with_item)? pragma_with_item pragma_with_list
	| /* empty */
	;

pragma_with_item :
	  pragma 
	| with_clause use_clause_opt
	;

with_clause : WITH! c_name_list ";"!
	;

use_clause_opt! :
	  ( use_clause )? use_clause use_clause_opt 
	| /* empty */
	;

unit : pkg_decl
	| pkg_body
	| subprog_decl_or_body
	| subunit
	| generic_decl
//	| rename_unit
	;

subunit : SEPARATE LPAREN compound_name RPAREN
	      subunit_body
	;

subunit_body : subprog_decl_or_body
	| pkg_body
	| task_decl_or_body
	| prot_body
	;

exception_decl : def_id_s ":" exception_spec
	;

exception_spec :
	  EXCEPTION ";"
	| EXCEPTION RENAMES name ";"
	;

except_handler_part : EXCEPTION ( exception_handler )* 
	;

exception_handler : 
	WHEN { IDENTIFIER ":" } except_choice_s RIGHT_SHAFT statement_s
	;

except_choice_s : 
	  except_choice more_except_choices
	;

more_except_choices :
	  (PIPE)? PIPE except_choice_s 
	| /* empty */
	;

except_choice : name
	| OTHERS
	;

raise_stmt : RAISE name_opt ";"
	;

requeue_stmt : 
	  REQUEUE name { WITH ABORT } ";"
	;

generic_decl : 
	  generic_formal_part generic_decl_completion 
	;

generic_decl_completion :
	  subprog_spec ";"
	| pkg_spec ";"
	;

/* recast for LL parsing */
generic_formal_part : GENERIC
	| generic_formal generic_formal_part
	;

generic_formal : param ";"
	| TYPE simple_name generic_discrim_part_opt IS generic_type_def ";"
	| WITH PROCEDURE simple_name 
	    formal_part_opt subp_default ";"
	| WITH FUNCTION func_designator 
	    formal_part_opt return_part subp_default ";"
	| WITH PACKAGE simple_name IS NEW name { LPAREN BOX RPAREN } ";"
	| use_clause
	;

generic_discrim_part_opt :
	| discrim_part
	| LPAREN BOX RPAREN
	;

subp_default :
	| IS name
	| IS BOX
	;

generic_type_def : LPAREN BOX RPAREN
	| RANGE BOX
	| MOD BOX
	| ( DELTA BOX DIGITS BOX )?
	| DELTA BOX
	| DIGITS BOX
	| array_type
	| access_type
	| private_type
	| generic_derived_type
	;

generic_derived_type : 
	  NEW subtype_ind { WITH PRIVATE }
	| ABSTRACT NEW subtype_ind WITH PRIVATE
	;

generic_inst : NEW name
	;

rep_spec : 
	  FOR mark USE rep_def
	;

rep_def : 
	  expression ";"
        | RECORD align_opt comp_loc_s END RECORD ";"
	| AT expression ";"
	;

align_opt :
	| AT MOD expression ";"
	;

comp_loc_s :
	| mark AT expression RANGE range ";" comp_loc_s
	;

code_stmt : qualified ";"
	;


}
