/*
 * CParser: P a r s e r  H e a d e r 
 *
 * Generated from: cccc.g
 *
 * Terence Parr, Russell Quong, Will Cohen, and Hank Dietz: 1989-1995
 * Parr Research Corporation
 * with Purdue University Electrical Engineering
 * with AHPCRC, University of Minnesota
 * ANTLR Version 1.33
 */

#ifndef CParser_h
#define CParser_h
#include "AParser.h"


#define zzTRACE_RULES
#include "AParser.h"
#include "cccc.h"
#include "cccc_utl.h"
#include "cccc_opt.h"

  // the objects which PCCTS creates for ASTs as the #0 variable etc
// have type "pointer to ASTBase", which means they need to be cast
// to a pointer to my variant of AST if I want to call my AST 
// methods on them
#define MY_AST(X) ( (AST*) X)

  // we have a global variable member for the language of the parse so
// that we can supply the names of dialects (ansi_c, ansi_c++, mfc_c++ etc)
// for contexts where we wish to apply dialect-specific lexing or parsing
// rules
extern string parse_language;
class CParser : public ANTLRParser {
protected:
	static ANTLRChar *_token_tbl[];
private:


ParseStore* ps;
ParseUtility* pu;

void tracein(const char *rulename)  { pu->tracein(rulename,guessing,LT(1)); }
void traceout(const char *rulename)  { pu->traceout(rulename,guessing,LT(1)); }
void syn(
_ANTLRTokenPtr tok, ANTLRChar *egroup, SetWordType *eset,
ANTLRTokenType etok, int k) 
{ 
	pu->syn(tok,egroup,eset,etok,k);
}

string typeCombine(const string& modifiers, const string& name, const string& indir)
{
	string retval;
	if(modifiers.size()>0)
	{
		retval=modifiers+" "+name;
	}
	else
	{
		retval=name;
	}
	if(indir.size()>0)
	{
		retval+=" ";
		retval+=indir;
	}
	return retval;
}

// Many of the rules below accept string parameters to 
// allow upward passing of attributes.
// Where the calling context does not need to receive
// the attributes, it can use the dummy values defined
// here to save allocating a string locally to satisfy the
// parameter list.
string d1,d2,d3;

public:

void init(const string& filename, const string& language)
{ 
	pu=ParseUtility::currentInstance();
	ps=ParseStore::currentInstance();
	
	ANTLRParser::init();
	parse_language=language;
}

  
	static SetWordType err1[36];
	static SetWordType err2[36];
	static SetWordType err3[36];
	static SetWordType setwd1[276];
	static SetWordType setwd2[276];
	static SetWordType err4[36];
	static SetWordType err5[36];
	static SetWordType err6[36];
	static SetWordType setwd3[276];
	static SetWordType err7[36];
	static SetWordType err8[36];
	static SetWordType setwd4[276];
	static SetWordType err9[36];
	static SetWordType err10[36];
	static SetWordType err11[36];
	static SetWordType err12[36];
	static SetWordType setwd5[276];
	static SetWordType err13[36];
	static SetWordType err14[36];
	static SetWordType setwd6[276];
	static SetWordType err15[36];
	static SetWordType err16[36];
	static SetWordType err17[36];
	static SetWordType err18[36];
	static SetWordType setwd7[276];
	static SetWordType err19[36];
	static SetWordType err20[36];
	static SetWordType setwd8[276];
	static SetWordType err21[36];
	static SetWordType err22[36];
	static SetWordType err23[36];
	static SetWordType setwd9[276];
	static SetWordType err24[36];
	static SetWordType err25[36];
	static SetWordType err26[36];
	static SetWordType setwd10[276];
	static SetWordType err27[36];
	static SetWordType err28[36];
	static SetWordType setwd11[276];
	static SetWordType err29[36];
	static SetWordType err30[36];
	static SetWordType err31[36];
	static SetWordType setwd12[276];
	static SetWordType err32[36];
	static SetWordType err33[36];
	static SetWordType err34[36];
	static SetWordType err35[36];
	static SetWordType setwd13[276];
	static SetWordType err36[36];
	static SetWordType err37[36];
	static SetWordType err38[36];
	static SetWordType err39[36];
	static SetWordType setwd14[276];
	static SetWordType err40[36];
	static SetWordType err41[36];
	static SetWordType setwd15[276];
	static SetWordType err42[36];
	static SetWordType err43[36];
	static SetWordType setwd16[276];
	static SetWordType err44[36];
	static SetWordType err45[36];
	static SetWordType setwd17[276];
	static SetWordType err46[36];
	static SetWordType err47[36];
	static SetWordType err48[36];
	static SetWordType setwd18[276];
	static SetWordType err49[36];
	static SetWordType err50[36];
	static SetWordType setwd19[276];
	static SetWordType err51[36];
	static SetWordType err52[36];
	static SetWordType err53[36];
	static SetWordType err54[36];
	static SetWordType err55[36];
	static SetWordType setwd20[276];
	static SetWordType err56[36];
	static SetWordType err57[36];
	static SetWordType err58[36];
	static SetWordType setwd21[276];
	static SetWordType err59[36];
	static SetWordType err60[36];
	static SetWordType err61[36];
	static SetWordType err62[36];
	static SetWordType err63[36];
	static SetWordType err64[36];
	static SetWordType setwd22[276];
	static SetWordType err65[36];
	static SetWordType EQUAL_OP_set[36];
	static SetWordType OP_ASSIGN_OP_set[36];
	static SetWordType SHIFT_OP_set[36];
	static SetWordType REL_OP_set[36];
	static SetWordType DIV_OP_set[36];
	static SetWordType PM_OP_set[36];
	static SetWordType setwd23[276];
	static SetWordType INCR_OP_set[36];
	static SetWordType ADD_OP_set[36];
	static SetWordType BITWISE_OP_set[36];
	static SetWordType err75[36];
	static SetWordType err76[36];
	static SetWordType err77[36];
	static SetWordType setwd24[276];
	static SetWordType err78[36];
	static SetWordType err79[36];
	static SetWordType err80[36];
	static SetWordType err81[36];
	static SetWordType err82[36];
	static SetWordType setwd25[276];
	static SetWordType err83[36];
	static SetWordType WildCard_set[36];
	static SetWordType setwd26[276];
	static SetWordType err85[36];
	static SetWordType err86[36];
	static SetWordType err87[36];
	static SetWordType setwd27[276];
	static SetWordType err88[36];
	static SetWordType err89[36];
	static SetWordType err90[36];
	static SetWordType setwd28[276];
private:
	void zzdflthandlers( int _signal, int *_retsignal );

public:
	CParser(ANTLRTokenBuffer *input);
	void start(void);
	void link_item( string& scope );
	void end_of_file(void);
	void definition_or_declaration( string& scope );
	void resync_tokens(void);
	void extern_linkage_block(void);
	void namespace_block(void);
	void using_statement(void);
	void explicit_template_instantiation(void);
	void class_declaration_or_definition( string& scope );
	void class_suffix( bool& is_definition,string& scope );
	void class_suffix_trailer(void);
	void opt_instance_list(void);
	void union_definition(void);
	void anonymous_union_definition(void);
	void named_union_definition(void);
	void enum_definition(void);
	void anonymous_enum_definition(void);
	void named_enum_definition(void);
	void instance_declaration( string& scopeName );
	void class_block( string& scope );
	void class_block_item_list( string& scope );
	void class_block_item( string& scope );
	void class_item_qualifier_list(void);
	void class_item_qualifier(void);
	void access_modifier(void);
	void method_declaration_or_definition_with_implicit_type( string& implicitScope );
	void method_declaration_or_definition_with_explicit_type( string &scope );
	void method_suffix( bool& is_definition );
	void method_signature( string& scope, string& methodName, string& paramList );
	void type( string& cvQualifiers, string& typeName, string& indirMods );
	void cv_qualifier( string& cvQualifiers );
	void type_name( string& typeName );
	void indirection_modifiers( string& indirMods );
	void indirection_modifier( string& indirMods );
	void builtin_type( string& typeName );
	void type_keyword( string& typeName );
	void user_type( string& typeName );
	void scoped_member_name(void);
	void scoped_identifier( string& scope, string& name );
	void explicit_scope_spec( string& scope );
	void unscoped_member_name( string& name );
	void dtor_member_name( string& name );
	void operator_member_name( string& name );
	void operator_identifier( string& opname );
	void new_or_delete(void);
	void param_list( string& scope, string& params );
	void param_list_items( string& scope, string& items );
	void more_param_items( string& scope, string& items );
	void param_item( string& scope, string& item );
	void param_type( string& scope, string& typeName );
	void param_spec(void);
	void knr_param_decl_list(void);
	void opt_const_modifier(void);
	void typedef_definition(void);
	void fptr_typedef_definition(void);
	void struct_typedef_definition(void);
	void simple_typedef_definition(void);
	void identifier_opt(void);
	void tag_list_opt(void);
	void tag(void);
	void simple_type_alias(void);
	void fptr_type_alias(void);
	void class_or_method_declaration_or_definition( string& scope );
	void class_prefix( string& modname, string& modtype );
	void inheritance_list( string& childName );
	void inheritance_item_list( string& childName );
	void inheritance_access_key(void);
	void inheritance_item( string& childName );
	void class_key( string& modtype );
	void access_key(void);
	void ctor_init_list(void);
	void ctor_init_item_list(void);
	void ctor_init_item(void);
	void linkage_qualifiers(void);
	void linkage_qualifier(void);
	void identifier_or_brace_block_or_both(void);
	void opt_brace_block(void);
	void instance_item( string& indir,string& name );
	void item_specifier( string& indir,string& name );
	void opt_initializer(void);
	void init_expr(void);
	void init_expr_item(void);
	void cast_keyword(void);
	void init_value(void);
	void keyword(void);
	void op(void);
	void constant(void);
	void literal(void);
	void string_literal(void);
	void block(void);
	void balanced(void);
	void balanced_list(void);
	void nested_token_list(  int nl  );
	void nested_token(  int nl  );
	void scoped(void);
	void brace_block(void);
	void skip_until_matching_rbrace(  int brace_level  );
	void paren_block(void);
	void brack_block(void);
	void brack_list(void);
	void angle_balanced_list(void);
	void angle_block(void);
	static SetWordType RESYNCHRONISATION_set[36];
};

#endif /* CParser_h */
