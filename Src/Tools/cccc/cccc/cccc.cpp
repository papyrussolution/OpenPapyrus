/*
 * A n t l r  T r a n s l a t i o n  H e a d e r
 *
 * Terence Parr, Will Cohen, and Hank Dietz: 1989-1994
 * Purdue University Electrical Engineering
 * With AHPCRC, University of Minnesota
 * ANTLR Version 1.33
 */
#include <stdio.h>
#define ANTLR_VERSION	133
#include "Ctokens.h"

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
#include "AParser.h"
#include "CParser.h"
#include "DLexerBase.h"
#include "ATokPtr.h"
#ifndef PURIFY
#define PURIFY(r,s)
#endif

void
CParser::start(void)
{
	zzRULE;
	string fileScope;   
	tracein("start");
	if ( (LA(1)==Eof) ) {
		end_of_file();
	}
	else {
		if ( (setwd1[LA(1)]&0x1) ) {
			link_item( fileScope );
			start();
		}
		else {FAIL(1,err1,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
	}
	traceout("start");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd1, 0x2);
	traceout("start");
}

void
CParser::link_item( string& scope )
{
	zzRULE;
	zzGUESS_BLOCK
	tracein("link_item");
	zzGUESS
	if ( !zzrv && (LA(1)==EXTERN) && (LA(2)==STRINGCONST) ) {
		{
			zzmatch(EXTERN); labase++;
			 consume();
			zzmatch(STRINGCONST); labase++;
			 consume();
			zzmatch(LBRACE); labase++;
			 consume();
		}
		zzGUESS_DONE
		extern_linkage_block();
	}
	else {
		if ( !zzrv ) zzGUESS_DONE;
		if ( (LA(1)==NAMESPACE) ) {
			namespace_block();
		}
		else {
			if ( !zzrv ) zzGUESS_DONE;
			if ( (LA(1)==USING)
 ) {
				using_statement();
			}
			else {
				if ( !zzrv ) zzGUESS_DONE;
				if ( (setwd1[LA(1)]&0x4) && (setwd1[LA(2)]&0x8) ) {
					linkage_qualifiers();
					definition_or_declaration( scope );
				}
				else {FAIL(2,err2,err3,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
			}
		}
	}
	traceout("link_item");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd1, 0x10);
	traceout("link_item");
}

void
CParser::end_of_file(void)
{
	zzRULE;
	ANTLRTokenPtr eof=NULL;
	tracein("end_of_file");
	zzmatch(Eof); labase++;
	
	if ( !guessing ) {
		eof = (ANTLRTokenPtr)LT(1);
}
	
	if ( !guessing ) {
	
	ps->record_other_extent(1, eof->getLine(),"<file scope items>");
	}
 consume();
	traceout("end_of_file");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd1, 0x20);
	traceout("end_of_file");
}

void
CParser::definition_or_declaration( string& scope )
{
	zzRULE;
	zzGUESS_BLOCK
	
	// get ready in case we need to resynchronize...
	ANTLRTokenPtr initial_token=LT(1);
	int startLine=LT(1)->getLine();
	string initial_text=pu->lookahead_text(3);
	tracein("definition_or_declaration");
	if ( (LA(1)==TYPEDEF) ) {
		typedef_definition();
	}
	else {
		zzGUESS
		if ( !zzrv && (setwd1[LA(1)]&0x40) && (setwd1[LA(2)]&0x80) ) {
			{
				explicit_template_instantiation();
			}
			zzGUESS_DONE
			{
				explicit_template_instantiation();
			}
		}
		else {
			if ( !zzrv ) zzGUESS_DONE;
			zzGUESS
			if ( !zzrv && (setwd2[LA(1)]&0x1) && (setwd2[LA(2)]&0x2) ) {
				{
					scoped_member_name();
					zzmatch(SEMICOLON); labase++;
					 consume();
				}
				zzGUESS_DONE
				{
					scoped_member_name();
					zzmatch(SEMICOLON); labase++;
					 consume();
				}
			}
			else {
				if ( !zzrv ) zzGUESS_DONE;
				zzGUESS
				if ( !zzrv && (setwd2[LA(1)]&0x4) && 
(setwd2[LA(2)]&0x8) ) {
					{
						scoped_member_name();
						zzmatch(LPAREN); labase++;
						 consume();
					}
					zzGUESS_DONE
					method_declaration_or_definition_with_implicit_type( scope );
				}
				else {
					if ( !zzrv ) zzGUESS_DONE;
					zzGUESS
					if ( !zzrv && (setwd2[LA(1)]&0x10) && (setwd2[LA(2)]&0x20) ) {
						{
							type( d1,d2,d3 );
							scoped_member_name();
							zzmatch(LPAREN); labase++;
							 consume();
						}
						zzGUESS_DONE
						method_declaration_or_definition_with_explicit_type( scope );
					}
					else {
						if ( !zzrv ) zzGUESS_DONE;
						zzGUESS
						if ( !zzrv && (setwd2[LA(1)]&0x40) && (setwd2[LA(2)]&0x80) ) {
							{
								type( d1,d2,d3 );
								scoped_member_name();
							}
							zzGUESS_DONE
							instance_declaration( scope );
						}
						else {
							if ( !zzrv ) zzGUESS_DONE;
							zzGUESS
							if ( !zzrv && (setwd3[LA(1)]&0x1) && (setwd3[LA(2)]&0x2) ) {
								{
									type( d1,d2,d3 );
									zzmatch(LPAREN); labase++;
									 consume();
									zzmatch(ASTERISK); labase++;
									 consume();
									scoped_member_name();
									zzmatch(RPAREN); labase++;
									 consume();
								}
								zzGUESS_DONE
								instance_declaration( scope );
							}
							else {
								if ( !zzrv ) zzGUESS_DONE;
								if ( (setwd3[LA(1)]&0x4) && (setwd3[LA(2)]&0x8) ) {
									class_declaration_or_definition( scope );
								}
								else {
									if ( !zzrv ) zzGUESS_DONE;
									if ( (LA(1)==UNION)
 ) {
										union_definition();
									}
									else {
										if ( !zzrv ) zzGUESS_DONE;
										if ( (LA(1)==ENUM) ) {
											enum_definition();
										}
										else {
											if ( !zzrv ) zzGUESS_DONE;
											if ( (LA(1)==SEMICOLON) ) {
												zzmatch(SEMICOLON); labase++;
												 consume();
											}
											else {FAIL(2,err4,err5,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
	traceout("definition_or_declaration");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	
	{
		ANTLRTokenPtr resync_token;
		int resync_nesting=mytoken(initial_token)->getNestingLevel();
		pu->resynchronize(resync_nesting,RESYNCHRONISATION_set,resync_token);
		
  cerr << "Syntax error: parser failed to handle "
		<< initial_text << "..." << resync_token->getText()
		<< " on lines " << initial_token->getLine() 
		<< " to " << resync_token->getLine() << endl;
		
  // record the rejected extent in the database
		int endLine=LT(1)->getLine();
		ps->record_other_extent(startLine,endLine,initial_text);
	}
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd3, 0x10);
	traceout("definition_or_declaration");
}

void
CParser::resync_tokens(void)
{
	zzRULE;
	tracein("resync_tokens");
	if ( (LA(1)==RBRACE) ) {
		zzmatch(RBRACE); labase++;
		 consume();
	}
	else {
		if ( (LA(1)==SEMICOLON) ) {
			zzmatch(SEMICOLON); labase++;
			 consume();
		}
		else {FAIL(1,err6,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
	}
	traceout("resync_tokens");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd3, 0x20);
	traceout("resync_tokens");
}

void
CParser::extern_linkage_block(void)
{
	zzRULE;
	string dummy;   
	tracein("extern_linkage_block");
	zzmatch(EXTERN); labase++;
	 consume();
	zzmatch(STRINGCONST); labase++;
	 consume();
	zzmatch(LBRACE); labase++;
	 consume();
	{
		while ( (setwd3[LA(1)]&0x40)
 ) {
			link_item( dummy );
		}
	}
	zzmatch(RBRACE); labase++;
	 consume();
	traceout("extern_linkage_block");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd3, 0x80);
	traceout("extern_linkage_block");
}

void
CParser::namespace_block(void)
{
	zzRULE;
	string dummy;   
	tracein("namespace_block");
	zzmatch(NAMESPACE); labase++;
	 consume();
	{
		if ( (LA(1)==IDENTIFIER) ) {
			zzmatch(IDENTIFIER); labase++;
			 consume();
		}
	}
	zzmatch(LBRACE); labase++;
	 consume();
	{
		while ( (setwd4[LA(1)]&0x1) ) {
			link_item( dummy );
		}
	}
	zzmatch(RBRACE); labase++;
	 consume();
	zzmatch(SEMICOLON); labase++;
	 consume();
	traceout("namespace_block");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd4, 0x2);
	traceout("namespace_block");
}

void
CParser::using_statement(void)
{
	zzRULE;
	tracein("using_statement");
	zzmatch(USING); labase++;
	 consume();
	{
		if ( (LA(1)==NAMESPACE) ) {
			zzmatch(NAMESPACE); labase++;
			 consume();
		}
	}
	scoped_member_name();
	zzmatch(SEMICOLON); labase++;
	 consume();
	traceout("using_statement");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd4, 0x4);
	traceout("using_statement");
}

void
CParser::explicit_template_instantiation(void)
{
	zzRULE;
	tracein("explicit_template_instantiation");
	scoped_member_name();
	angle_block();
	zzmatch(SEMICOLON); labase++;
	 consume();
	traceout("explicit_template_instantiation");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd4, 0x8);
	traceout("explicit_template_instantiation");
}

void
CParser::class_declaration_or_definition( string& scope )
{
	zzRULE;
	ANTLRTokenPtr sfx=NULL;
	
	int startLine=LT(1)->getLine(); 
	bool is_definition; 
	string modname,modtype;
	tracein("class_declaration_or_definition");
	class_prefix( modname,modtype );
	class_suffix( is_definition,modname );
	if ( !guessing ) {
	
	int endLine=LT(1)->getLine(); 
	if(is_definition==false)
	{           	
		ps->record_module_extent(startLine,endLine,modname,modtype,
		"declaration",utDECLARATION);
	}
	else
	{
	ps->record_module_extent(startLine,endLine,modname,modtype,
	"definition",utDEFINITION);
}
	}
	traceout("class_declaration_or_definition");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd4, 0x10);
	traceout("class_declaration_or_definition");
}

void
CParser::class_suffix( bool& is_definition,string& scope )
{
	zzRULE;
	tracein("class_suffix");
	if ( (LA(1)==SEMICOLON) ) {
		zzmatch(SEMICOLON); labase++;
		
		if ( !guessing ) {
		is_definition=false;   
		}
 consume();
	}
	else {
		if ( (setwd4[LA(1)]&0x20)
 ) {
			{
				if ( (LA(1)==COLON) ) {
					inheritance_list( scope );
				}
			}
			class_block( scope );
			class_suffix_trailer();
			if ( !guessing ) {
			is_definition=true;   
			}
		}
		else {FAIL(1,err7,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
	}
	traceout("class_suffix");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd4, 0x40);
	traceout("class_suffix");
}

void
CParser::class_suffix_trailer(void)
{
	zzRULE;
	tracein("class_suffix_trailer");
	opt_instance_list();
	zzmatch(SEMICOLON); labase++;
	 consume();
	traceout("class_suffix_trailer");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd4, 0x80);
	traceout("class_suffix_trailer");
}

void
CParser::opt_instance_list(void)
{
	zzRULE;
	tracein("opt_instance_list");
	if ( (LA(1)==IDENTIFIER) ) {
		zzmatch(IDENTIFIER); labase++;
		 consume();
		{
			while ( (LA(1)==COMMA) ) {
				zzmatch(COMMA); labase++;
				 consume();
				zzmatch(IDENTIFIER); labase++;
				 consume();
			}
		}
	}
	else {
		if ( (LA(1)==SEMICOLON) ) {
		}
		else {FAIL(1,err8,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
	}
	traceout("opt_instance_list");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd5, 0x1);
	traceout("opt_instance_list");
}

void
CParser::union_definition(void)
{
	zzRULE;
	zzGUESS_BLOCK
	tracein("union_definition");
	zzGUESS
	if ( !zzrv && (LA(1)==UNION) && 
(LA(2)==LBRACE) ) {
		{
			anonymous_union_definition();
		}
		zzGUESS_DONE
		{
			anonymous_union_definition();
		}
	}
	else {
		if ( !zzrv ) zzGUESS_DONE;
		if ( (LA(1)==UNION) && (LA(2)==IDENTIFIER) ) {
			named_union_definition();
		}
		else {FAIL(2,err9,err10,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
	}
	traceout("union_definition");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd5, 0x2);
	traceout("union_definition");
}

void
CParser::anonymous_union_definition(void)
{
	zzRULE;
	tracein("anonymous_union_definition");
	zzmatch(UNION); labase++;
	 consume();
	brace_block();
	opt_instance_list();
	zzmatch(SEMICOLON); labase++;
	 consume();
	traceout("anonymous_union_definition");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd5, 0x4);
	traceout("anonymous_union_definition");
}

void
CParser::named_union_definition(void)
{
	zzRULE;
	ANTLRTokenPtr id=NULL;
	int startLine=LT(1)->getLine();   
	tracein("named_union_definition");
	zzmatch(UNION); labase++;
	 consume();
	zzmatch(IDENTIFIER); labase++;
	
	if ( !guessing ) {
		id = (ANTLRTokenPtr)LT(1);
}
	 consume();
	brace_block();
	zzmatch(SEMICOLON); labase++;
	
	if ( !guessing ) {
	
	ps->record_module_extent(startLine,startLine,
	id->getText(),"union",
	"definition",utDEFINITION);
	}
 consume();
	traceout("named_union_definition");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd5, 0x8);
	traceout("named_union_definition");
}

void
CParser::enum_definition(void)
{
	zzRULE;
	zzGUESS_BLOCK
	tracein("enum_definition");
	zzGUESS
	if ( !zzrv && (LA(1)==ENUM) && (LA(2)==LBRACE) ) {
		{
			anonymous_enum_definition();
		}
		zzGUESS_DONE
		{
			anonymous_enum_definition();
		}
	}
	else {
		if ( !zzrv ) zzGUESS_DONE;
		if ( (LA(1)==ENUM) && (LA(2)==IDENTIFIER) ) {
			named_enum_definition();
		}
		else {FAIL(2,err11,err12,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
	}
	traceout("enum_definition");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd5, 0x10);
	traceout("enum_definition");
}

void
CParser::anonymous_enum_definition(void)
{
	zzRULE;
	tracein("anonymous_enum_definition");
	zzmatch(ENUM); labase++;
	 consume();
	brace_block();
	opt_instance_list();
	zzmatch(SEMICOLON); labase++;
	 consume();
	traceout("anonymous_enum_definition");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd5, 0x20);
	traceout("anonymous_enum_definition");
}

void
CParser::named_enum_definition(void)
{
	zzRULE;
	ANTLRTokenPtr id=NULL;
	int startLine=LT(1)->getLine();   
	tracein("named_enum_definition");
	zzmatch(ENUM); labase++;
	 consume();
	zzmatch(IDENTIFIER); labase++;
	
	if ( !guessing ) {
		id = (ANTLRTokenPtr)LT(1);
}
	 consume();
	brace_block();
	zzmatch(SEMICOLON); labase++;
	
	if ( !guessing ) {
	
	ps->record_module_extent(startLine,startLine,
	id->getText(),"enum",
	"definition",utDEFINITION);
	}
 consume();
	traceout("named_enum_definition");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd5, 0x40);
	traceout("named_enum_definition");
}

void
CParser::instance_declaration( string& scopeName )
{
	zzRULE;
	int startLine=LT(1)->getLine(); string cvQuals,typeName,varName,indir;   
	tracein("instance_declaration");
	{
		while ( (setwd5[LA(1)]&0x80) ) {
			cv_qualifier( cvQuals );
		}
	}
	{
		if ( (LA(1)==STATIC)
 ) {
			zzmatch(STATIC); labase++;
			 consume();
		}
	}
	type_name( typeName );
	instance_item( indir,varName );
	{
		while ( (LA(1)==COMMA) ) {
			zzmatch(COMMA); labase++;
			 consume();
			instance_item( d1,d2 );
		}
	}
	zzmatch(SEMICOLON); labase++;
	
	if ( !guessing ) {
	
	if(indir.size()!=0)
	{
		ps->record_userel_extent(startLine,startLine,
		scopeName,varName,typeName,
		"has by reference",
		ps->get_visibility(),
		utHASBYREF);
	} 
	else 
	{
	ps->record_userel_extent(startLine,startLine,
	scopeName,"",typeName,
	"has by value", 
	ps->get_visibility(),
	utHASBYVAL);
}
	}
 consume();
	traceout("instance_declaration");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd6, 0x1);
	traceout("instance_declaration");
}

void
CParser::class_block( string& scope )
{
	zzRULE;
	
	int saved_visibility=ps->get_flag(psfVISIBILITY);
	tracein("class_block");
	zzmatch(LBRACE); labase++;
	 consume();
	class_block_item_list( scope );
	zzmatch(RBRACE); labase++;
	
	if ( !guessing ) {
	ps->set_flag(psfVISIBILITY,saved_visibility);   
	}
 consume();
	traceout("class_block");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd6, 0x2);
	traceout("class_block");
}

void
CParser::class_block_item_list( string& scope )
{
	zzRULE;
	zzGUESS_BLOCK
	tracein("class_block_item_list");
	zzGUESS
	if ( !zzrv && (setwd6[LA(1)]&0x4) ) {
		{
			class_block_item( scope );
		}
		zzGUESS_DONE
		class_block_item( scope );
		class_block_item_list( scope );
	}
	else {
		if ( !zzrv ) zzGUESS_DONE;
		if ( (LA(1)==RBRACE) ) {
		}
		else {FAIL(1,err13,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
	}
	traceout("class_block_item_list");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd6, 0x8);
	traceout("class_block_item_list");
}

void
CParser::class_block_item( string& scope )
{
	zzRULE;
	zzGUESS_BLOCK
	tracein("class_block_item");
	zzGUESS
	if ( !zzrv && (setwd6[LA(1)]&0x10) ) {
		{
			access_modifier();
		}
		zzGUESS_DONE
		{
			access_modifier();
		}
	}
	else {
		if ( !zzrv ) zzGUESS_DONE;
		if ( (setwd6[LA(1)]&0x20)
 ) {
			class_item_qualifier_list();
			definition_or_declaration( scope );
		}
		else {FAIL(1,err14,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
	}
	traceout("class_block_item");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd6, 0x40);
	traceout("class_block_item");
}

void
CParser::class_item_qualifier_list(void)
{
	zzRULE;
	zzGUESS_BLOCK
	tracein("class_item_qualifier_list");
	zzGUESS
	if ( !zzrv && (setwd6[LA(1)]&0x80) && (setwd7[LA(2)]&0x1) ) {
		{
			class_item_qualifier();
		}
		zzGUESS_DONE
		class_item_qualifier();
		class_item_qualifier_list();
	}
	else {
		if ( !zzrv ) zzGUESS_DONE;
		if ( (setwd7[LA(1)]&0x2) && (setwd7[LA(2)]&0x4) ) {
		}
		else {FAIL(2,err15,err16,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
	}
	traceout("class_item_qualifier_list");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd7, 0x8);
	traceout("class_item_qualifier_list");
}

void
CParser::class_item_qualifier(void)
{
	zzRULE;
	tracein("class_item_qualifier");
	if ( (LA(1)==FRIEND) ) {
		zzmatch(FRIEND); labase++;
		 consume();
	}
	else {
		if ( (LA(1)==VIRTUAL) ) {
			zzmatch(VIRTUAL); labase++;
			
			if ( !guessing ) {
			ps->set_flag(psfVIRTUAL,abTRUE);   
			}
 consume();
		}
		else {
			if ( (LA(1)==STATIC)
 ) {
				zzmatch(STATIC); labase++;
				
				if ( !guessing ) {
				ps->set_flag(psfSTATIC,abTRUE);   
				}
 consume();
			}
			else {
				if ( (LA(1)==INLINE) ) {
					zzmatch(INLINE); labase++;
					 consume();
				}
				else {FAIL(1,err17,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
			}
		}
	}
	traceout("class_item_qualifier");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd7, 0x10);
	traceout("class_item_qualifier");
}

void
CParser::access_modifier(void)
{
	zzRULE;
	tracein("access_modifier");
	access_key();
	zzmatch(COLON); labase++;
	 consume();
	traceout("access_modifier");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd7, 0x20);
	traceout("access_modifier");
}

void
CParser::method_declaration_or_definition_with_implicit_type( string& implicitScope )
{
	zzRULE;
	
	int startLine=LT(1)->getLine(); bool is_definition; 
	string returnType,scope=implicitScope,methodName, paramList;
	tracein("method_declaration_or_definition_with_implicit_type");
	method_signature( scope,methodName,paramList );
	method_suffix( is_definition );
	if ( !guessing ) {
	
	int endLine=LT(1)->getLine();
	if(is_definition==false)
	{
		ps->record_function_extent(startLine,endLine,
		returnType,scope,	
		methodName,paramList,
		"declaration",
		ps->get_visibility(), 
		utDECLARATION);
	}
	else
	{
	ps->record_function_extent(startLine,endLine,
	returnType,scope,
	methodName,paramList,
	"definition",
	ps->get_visibility(), 
	utDEFINITION);
}
	}
	traceout("method_declaration_or_definition_with_implicit_type");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd7, 0x40);
	traceout("method_declaration_or_definition_with_implicit_type");
}

void
CParser::method_declaration_or_definition_with_explicit_type( string &scope )
{
	zzRULE;
	string cvQualifiers,typeName,indirMods;   
	tracein("method_declaration_or_definition_with_explicit_type");
	type( cvQualifiers,typeName,indirMods );
	method_declaration_or_definition_with_implicit_type( scope );
	traceout("method_declaration_or_definition_with_explicit_type");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd7, 0x80);
	traceout("method_declaration_or_definition_with_explicit_type");
}

void
CParser::method_suffix( bool& is_definition )
{
	zzRULE;
	tracein("method_suffix");
	if ( (LA(1)==SEMICOLON) ) {
		zzmatch(SEMICOLON); labase++;
		
		if ( !guessing ) {
		is_definition=false;   
		}
 consume();
	}
	else {
		if ( (LA(1)==ASSIGN_OP) ) {
			zzmatch(ASSIGN_OP); labase++;
			 consume();
			zzmatch(OCT_NUM); labase++;
			 consume();
			zzmatch(SEMICOLON); labase++;
			
			if ( !guessing ) {
			is_definition=false;   
			}
 consume();
		}
		else {
			if ( (LA(1)==COLON) ) {
				ctor_init_list();
				brace_block();
				if ( !guessing ) {
				is_definition=true;   
				}
			}
			else {
				if ( (LA(1)==LBRACE)
 ) {
					brace_block();
					if ( !guessing ) {
					is_definition=true;   
					}
				}
				else {FAIL(1,err18,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
			}
		}
	}
	traceout("method_suffix");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd8, 0x1);
	traceout("method_suffix");
}

void
CParser::method_signature( string& scope, string& methodName, string& paramList )
{
	zzRULE;
	
	int startLine=LT(1)->getLine(); 
	tracein("method_signature");
	scoped_identifier( scope,methodName );
	param_list( scope,paramList );
	opt_const_modifier();
	if ( !guessing ) {
	
	}
	traceout("method_signature");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd8, 0x2);
	traceout("method_signature");
}

void
CParser::type( string& cvQualifiers, string& typeName, string& indirMods )
{
	zzRULE;
	tracein("type");
	{
		while ( (setwd8[LA(1)]&0x4) ) {
			cv_qualifier( cvQualifiers );
		}
	}
	{
		if ( (LA(1)==STATIC) ) {
			zzmatch(STATIC); labase++;
			 consume();
		}
	}
	type_name( typeName );
	indirection_modifiers( indirMods );
	traceout("type");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd8, 0x8);
	traceout("type");
}

void
CParser::cv_qualifier( string& cvQualifiers )
{
	zzRULE;
	string nextTokenText=LT(1)->getText();   
	tracein("cv_qualifier");
	{
		if ( (LA(1)==KW_CONST) ) {
			zzmatch(KW_CONST); labase++;
			 consume();
		}
		else {
			if ( (LA(1)==MUTABLE) ) {
				zzmatch(MUTABLE); labase++;
				 consume();
			}
			else {
				if ( (LA(1)==VOLATILE)
 ) {
					zzmatch(VOLATILE); labase++;
					 consume();
				}
				else {
					if ( (LA(1)==REGISTER) ) {
						zzmatch(REGISTER); labase++;
						 consume();
					}
					else {FAIL(1,err19,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
				}
			}
		}
	}
	if ( !guessing ) {
	cvQualifiers=typeCombine(cvQualifiers,nextTokenText,"");   
	}
	traceout("cv_qualifier");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd8, 0x10);
	traceout("cv_qualifier");
}

void
CParser::type_name( string& typeName )
{
	zzRULE;
	tracein("type_name");
	if ( (setwd8[LA(1)]&0x20) ) {
		builtin_type( typeName );
	}
	else {
		if ( (setwd8[LA(1)]&0x40) ) {
			user_type( typeName );
		}
		else {FAIL(1,err20,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
	}
	traceout("type_name");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd8, 0x80);
	traceout("type_name");
}

void
CParser::indirection_modifiers( string& indirMods )
{
	zzRULE;
	zzGUESS_BLOCK
	tracein("indirection_modifiers");
	zzGUESS
	if ( !zzrv && (setwd9[LA(1)]&0x1) && (setwd9[LA(2)]&0x2) ) {
		{
			indirection_modifier( indirMods );
		}
		zzGUESS_DONE
		indirection_modifier( indirMods );
		indirection_modifiers( indirMods );
	}
	else {
		if ( !zzrv ) zzGUESS_DONE;
		if ( (setwd9[LA(1)]&0x4) && 
(setwd9[LA(2)]&0x8) ) {
		}
		else {FAIL(2,err21,err22,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
	}
	traceout("indirection_modifiers");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd9, 0x10);
	traceout("indirection_modifiers");
}

void
CParser::indirection_modifier( string& indirMods )
{
	zzRULE;
	zzGUESS_BLOCK
	tracein("indirection_modifier");
	zzGUESS
	if ( !zzrv && (LA(1)==KW_CONST) ) {
		{
			zzmatch(KW_CONST); labase++;
			 consume();
			zzmatch(ASTERISK); labase++;
			 consume();
		}
		zzGUESS_DONE
		{
			zzmatch(KW_CONST); labase++;
			 consume();
			zzmatch(ASTERISK); labase++;
			 consume();
		}
		if ( !guessing ) {
		indirMods+="const*";   
		}
	}
	else {
		if ( !zzrv ) zzGUESS_DONE;
		if ( (LA(1)==ASTERISK) ) {
			zzmatch(ASTERISK); labase++;
			
			if ( !guessing ) {
			indirMods+="*";   
			}
 consume();
		}
		else {
			if ( !zzrv ) zzGUESS_DONE;
			if ( (LA(1)==AMPERSAND) ) {
				zzmatch(AMPERSAND); labase++;
				
				if ( !guessing ) {
				indirMods+="&";   
				}
 consume();
			}
			else {FAIL(1,err23,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
		}
	}
	traceout("indirection_modifier");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd9, 0x20);
	traceout("indirection_modifier");
}

void
CParser::builtin_type( string& typeName )
{
	zzRULE;
	tracein("builtin_type");
	{
		int zzcnt=1;
		do {
			type_keyword( typeName );
		} while ( (setwd9[LA(1)]&0x40) && (setwd9[LA(2)]&0x80) );
	}
	traceout("builtin_type");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd10, 0x1);
	traceout("builtin_type");
}

void
CParser::type_keyword( string& typeName )
{
	zzRULE;
	string tokenText=LT(1)->getText();   
	tracein("type_keyword");
	{
		if ( (LA(1)==KW_VOID)
 ) {
			zzmatch(KW_VOID); labase++;
			 consume();
		}
		else {
			if ( (LA(1)==KW_BOOL) ) {
				zzmatch(KW_BOOL); labase++;
				 consume();
			}
			else {
				if ( (LA(1)==KW_CHAR) ) {
					zzmatch(KW_CHAR); labase++;
					 consume();
				}
				else {
					if ( (LA(1)==KW_INT) ) {
						zzmatch(KW_INT); labase++;
						 consume();
					}
					else {
						if ( (LA(1)==KW_FLOAT) ) {
							zzmatch(KW_FLOAT); labase++;
							 consume();
						}
						else {
							if ( (LA(1)==KW_DOUBLE)
 ) {
								zzmatch(KW_DOUBLE); labase++;
								 consume();
							}
							else {
								if ( (LA(1)==KW_SHORT) ) {
									zzmatch(KW_SHORT); labase++;
									 consume();
								}
								else {
									if ( (LA(1)==KW_LONG) ) {
										zzmatch(KW_LONG); labase++;
										 consume();
									}
									else {
										if ( (LA(1)==UNSIGNED) ) {
											zzmatch(UNSIGNED); labase++;
											 consume();
										}
										else {
											if ( (LA(1)==SIGNED) ) {
												zzmatch(SIGNED); labase++;
												 consume();
											}
											else {FAIL(1,err24,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
	if ( !guessing ) {
	
	// We only really care about the type name so that we 
	// can count relationships between classes, so what we store
	// here is a bit arbitrary.  We choose to represent the type
	// of composed builtin types such as 'unsigned char' using
	// only the last keyword (i.e. 'char' in this case), so that 
	// we don't have to have an enormous supression list of different
	// variants like 'long long unsigned int'.
	typeName=tokenText; 
	}
	traceout("type_keyword");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd10, 0x2);
	traceout("type_keyword");
}

void
CParser::user_type( string& typeName )
{
	zzRULE;
	string scope,name;   
	tracein("user_type");
	{
		if ( (setwd10[LA(1)]&0x4)
 ) {
			class_key( d1 );
		}
	}
	scoped_identifier( scope,name );
	if ( !guessing ) {
	typeName=pu->scopeCombine(scope,name);   
	}
	traceout("user_type");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd10, 0x8);
	traceout("user_type");
}

void
CParser::scoped_member_name(void)
{
	zzRULE;
	string dummy1, dummy2;   
	tracein("scoped_member_name");
	scoped_identifier( dummy1,dummy2 );
	traceout("scoped_member_name");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd10, 0x10);
	traceout("scoped_member_name");
}

void
CParser::scoped_identifier( string& scope, string& name )
{
	zzRULE;
	zzGUESS_BLOCK
	tracein("scoped_identifier");
	zzGUESS
	if ( !zzrv && (LA(1)==IDENTIFIER) && (setwd10[LA(2)]&0x20) ) {
		{
			explicit_scope_spec( scope );
		}
		zzGUESS_DONE
		explicit_scope_spec( scope );
		scoped_identifier( scope,name );
	}
	else {
		if ( !zzrv ) zzGUESS_DONE;
		if ( (setwd10[LA(1)]&0x40) && (setwd10[LA(2)]&0x80) ) {
			unscoped_member_name( name );
		}
		else {FAIL(2,err25,err26,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
	}
	traceout("scoped_identifier");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd11, 0x1);
	traceout("scoped_identifier");
}

void
CParser::explicit_scope_spec( string& scope )
{
	zzRULE;
	ANTLRTokenPtr cl=NULL;
	tracein("explicit_scope_spec");
	zzmatch(IDENTIFIER); labase++;
	
	if ( !guessing ) {
		cl = (ANTLRTokenPtr)LT(1);
}
	 consume();
	{
		if ( (LA(1)==LESSTHAN) ) {
			angle_block();
		}
	}
	zzmatch(COLONCOLON); labase++;
	
	if ( !guessing ) {
	
	scope=cl->getText();
	ps->set_flag(vDONTKNOW);
	}
 consume();
	traceout("explicit_scope_spec");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd11, 0x2);
	traceout("explicit_scope_spec");
}

void
CParser::unscoped_member_name( string& name )
{
	zzRULE;
	ANTLRTokenPtr id1=NULL, id2=NULL;
	tracein("unscoped_member_name");
	{
		zzGUESS_BLOCK
		zzGUESS
		if ( !zzrv && (LA(1)==IDENTIFIER) && (LA(2)==LESSTHAN) ) {
			{
				zzmatch(IDENTIFIER); labase++;
				
				if ( !guessing ) {
								id1 = (ANTLRTokenPtr)LT(1);
}
				 consume();
				angle_block();
			}
			zzGUESS_DONE
			{
				zzmatch(IDENTIFIER); labase++;
				
				if ( !guessing ) {
								id1 = (ANTLRTokenPtr)LT(1);
}
				 consume();
				angle_block();
			}
			if ( !guessing ) {
			name= id1->getText();   
			}
		}
		else {
			if ( !zzrv ) zzGUESS_DONE;
			if ( (LA(1)==IDENTIFIER) && 
(setwd11[LA(2)]&0x4) ) {
				zzmatch(IDENTIFIER); labase++;
				
				if ( !guessing ) {
								id2 = (ANTLRTokenPtr)LT(1);
}
				
				if ( !guessing ) {
				name= id2->getText();   
				}
 consume();
			}
			else {
				if ( !zzrv ) zzGUESS_DONE;
				if ( (LA(1)==TILDA) ) {
					dtor_member_name( name );
				}
				else {
					if ( !zzrv ) zzGUESS_DONE;
					if ( (LA(1)==OPERATOR) ) {
						operator_member_name( name );
					}
					else {FAIL(2,err27,err28,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
				}
			}
		}
	}
	traceout("unscoped_member_name");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd11, 0x8);
	traceout("unscoped_member_name");
}

void
CParser::dtor_member_name( string& name )
{
	zzRULE;
	ANTLRTokenPtr id=NULL;
	tracein("dtor_member_name");
	zzmatch(TILDA); labase++;
	 consume();
	zzmatch(IDENTIFIER); labase++;
	
	if ( !guessing ) {
		id = (ANTLRTokenPtr)LT(1);
}
	
	if ( !guessing ) {
	
	name="~";
	name+= id->getText();
	}
 consume();
	traceout("dtor_member_name");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd11, 0x10);
	traceout("dtor_member_name");
}

void
CParser::operator_member_name( string& name )
{
	zzRULE;
	string operatorIdentifier;   
	tracein("operator_member_name");
	zzmatch(OPERATOR); labase++;
	 consume();
	operator_identifier( operatorIdentifier );
	if ( !guessing ) {
	
	name+="operator ";
	name+=operatorIdentifier; 
	}
	traceout("operator_member_name");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd11, 0x20);
	traceout("operator_member_name");
}

void
CParser::operator_identifier( string& opname )
{
	zzRULE;
	zzGUESS_BLOCK
	
	string cv,name,indir;  
	opname=LT(1)->getText();
	tracein("operator_identifier");
	zzGUESS
	if ( !zzrv && (setwd11[LA(1)]&0x40) && (setwd11[LA(2)]&0x80) ) {
		{
			op();
		}
		zzGUESS_DONE
		{
			op();
		}
	}
	else {
		if ( !zzrv ) zzGUESS_DONE;
		zzGUESS
		if ( !zzrv && (setwd12[LA(1)]&0x1) && (LA(2)==LBRACK) ) {
			{
				new_or_delete();
				zzmatch(LBRACK); labase++;
				 consume();
				zzmatch(RBRACK); labase++;
				 consume();
			}
			zzGUESS_DONE
			{
				new_or_delete();
				zzmatch(LBRACK); labase++;
				 consume();
				zzmatch(RBRACK); labase++;
				 consume();
			}
			if ( !guessing ) {
			opname+="[]";   
			}
		}
		else {
			if ( !zzrv ) zzGUESS_DONE;
			if ( (setwd12[LA(1)]&0x2) && 
(setwd12[LA(2)]&0x4) ) {
				new_or_delete();
			}
			else {
				if ( !zzrv ) zzGUESS_DONE;
				if ( (setwd12[LA(1)]&0x8) && (setwd12[LA(2)]&0x10) ) {
					type( cv,name,indir );
					if ( !guessing ) {
					opname=name+indir;   
					}
				}
				else {
					if ( !zzrv ) zzGUESS_DONE;
					if ( (LA(1)==LPAREN) ) {
						zzmatch(LPAREN); labase++;
						 consume();
						zzmatch(RPAREN); labase++;
						
						if ( !guessing ) {
						opname="()";   
						}
 consume();
					}
					else {
						if ( !zzrv ) zzGUESS_DONE;
						if ( (LA(1)==LBRACK) ) {
							zzmatch(LBRACK); labase++;
							 consume();
							zzmatch(RBRACK); labase++;
							
							if ( !guessing ) {
							opname="[]";   
							}
 consume();
						}
						else {FAIL(2,err29,err30,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
					}
				}
			}
		}
	}
	traceout("operator_identifier");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd12, 0x20);
	traceout("operator_identifier");
}

void
CParser::new_or_delete(void)
{
	zzRULE;
	tracein("new_or_delete");
	if ( (LA(1)==NEW) ) {
		zzmatch(NEW); labase++;
		 consume();
	}
	else {
		if ( (LA(1)==DELETE)
 ) {
			zzmatch(DELETE); labase++;
			 consume();
		}
		else {FAIL(1,err31,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
	}
	traceout("new_or_delete");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd12, 0x40);
	traceout("new_or_delete");
}

void
CParser::param_list( string& scope, string& params )
{
	zzRULE;
	zzGUESS_BLOCK
	
	int startLine=LT(1)->getLine();
	string param_items;
	tracein("param_list");
	zzGUESS
	if ( !zzrv && (LA(1)==LPAREN) && (setwd12[LA(2)]&0x80) ) {
		{
			zzmatch(LPAREN); labase++;
			 consume();
			param_list_items( scope,param_items );
			zzmatch(RPAREN); labase++;
			 consume();
		}
		zzGUESS_DONE
		{
			zzmatch(LPAREN); labase++;
			 consume();
			param_list_items( scope,param_items );
			zzmatch(RPAREN); labase++;
			 consume();
		}
		if ( !guessing ) {
		
		params="(";
		params+=param_items;
		params+=")";
		}
	}
	else {
		if ( !zzrv ) zzGUESS_DONE;
		if ( (LA(1)==LPAREN) && (setwd13[LA(2)]&0x1) ) {
			paren_block();
			if ( !guessing ) {
			
			params="(...)";
			}
		}
		else {FAIL(2,err32,err33,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
	}
	traceout("param_list");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd13, 0x2);
	traceout("param_list");
}

void
CParser::param_list_items( string& scope, string& items )
{
	zzRULE;
	tracein("param_list_items");
	if ( (LA(1)==RPAREN) ) {
	}
	else {
		if ( (setwd13[LA(1)]&0x4) ) {
			param_item( scope,items );
			more_param_items( scope,items );
		}
		else {FAIL(1,err34,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
	}
	traceout("param_list_items");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd13, 0x8);
	traceout("param_list_items");
}

void
CParser::more_param_items( string& scope, string& items )
{
	zzRULE;
	string next_item, further_items;   
	tracein("more_param_items");
	if ( (LA(1)==RPAREN)
 ) {
	}
	else {
		if ( (LA(1)==COMMA) ) {
			zzmatch(COMMA); labase++;
			 consume();
			param_item( scope,next_item );
			more_param_items( scope,further_items );
			if ( !guessing ) {
			
			items+=",";
			items+=next_item;
			items+=further_items;
			}
		}
		else {FAIL(1,err35,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
	}
	traceout("more_param_items");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd13, 0x10);
	traceout("more_param_items");
}

void
CParser::param_item( string& scope, string& item )
{
	zzRULE;
	tracein("param_item");
	param_type( scope,item );
	param_spec();
	traceout("param_item");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd13, 0x20);
	traceout("param_item");
}

void
CParser::param_type( string& scope, string& typeName )
{
	zzRULE;
	
	string cvmods, name, indir;  
	int startLine=LT(1)->getLine();
	tracein("param_type");
	type( cvmods,name,indir );
	if ( !guessing ) {
	
	// we distinguish between value & reference by
	// looking at the length of the string of indirection
	// operator associated with the last recognised type
	if(indir.size()!=0)
	{
		ps->record_userel_extent(startLine,startLine,
		scope,"",name,
		"pass by reference",
		ps->get_visibility(),
		utPARBYREF);
	} 
	else 
	{
	ps->record_userel_extent(startLine,startLine,
	scope,"",name,
	"pass by value",
	ps->get_visibility(),
	utPARBYVAL);
} 
typeName=typeCombine(cvmods,name,indir);
	}
	traceout("param_type");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd13, 0x40);
	traceout("param_type");
}

void
CParser::param_spec(void)
{
	zzRULE;
	tracein("param_spec");
	{
		if ( (LA(1)==IDENTIFIER) ) {
			zzmatch(IDENTIFIER); labase++;
			 consume();
		}
	}
	{
		if ( (LA(1)==ASSIGN_OP) ) {
			zzmatch(ASSIGN_OP); labase++;
			 consume();
			literal();
		}
	}
	traceout("param_spec");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd13, 0x80);
	traceout("param_spec");
}

void
CParser::knr_param_decl_list(void)
{
	zzRULE;
	tracein("knr_param_decl_list");
	if ( (LA(1)==Eof) ) {
	}
	else {
		if ( (setwd14[LA(1)]&0x1)
 ) {
			instance_declaration( d1 );
			knr_param_decl_list();
		}
		else {FAIL(1,err36,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
	}
	traceout("knr_param_decl_list");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	
	cerr << "failed knr_param_decl_list for token " 
	<< static_cast<int>(LT(1)->getType()) << ' ' 
	<< LT(1)->getText() << endl;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd14, 0x2);
	traceout("knr_param_decl_list");
}

void
CParser::opt_const_modifier(void)
{
	zzRULE;
	
	ps->set_flag(psfCONST,abFALSE);
	tracein("opt_const_modifier");
	if ( (setwd14[LA(1)]&0x4) ) {
	}
	else {
		if ( (LA(1)==KW_CONST) ) {
			zzmatch(KW_CONST); labase++;
			
			if ( !guessing ) {
			
			ps->set_flag(psfCONST,abTRUE);
			}
 consume();
		}
		else {FAIL(1,err37,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
	}
	traceout("opt_const_modifier");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	
	// fail action for opt_const_modifier
	// I can't see how we can fail this, but we seem to manage
	cerr << "failed opt_const_modifier for token " 
	<< static_cast<int>(LT(1)->getType()) << ' ' 
	<< LT(1)->getText() << endl;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd14, 0x8);
	traceout("opt_const_modifier");
}

void
CParser::typedef_definition(void)
{
	zzRULE;
	zzGUESS_BLOCK
	string dummy;   
	tracein("typedef_definition");
	zzGUESS
	if ( !zzrv && (LA(1)==TYPEDEF) && (setwd14[LA(2)]&0x10) ) {
		{
			fptr_typedef_definition();
		}
		zzGUESS_DONE
		{
			fptr_typedef_definition();
		}
	}
	else {
		if ( !zzrv ) zzGUESS_DONE;
		zzGUESS
		if ( !zzrv && (LA(1)==TYPEDEF) && (setwd14[LA(2)]&0x20) ) {
			{
				zzmatch(TYPEDEF); labase++;
				 consume();
				class_key( dummy );
			}
			zzGUESS_DONE
			struct_typedef_definition();
		}
		else {
			if ( !zzrv ) zzGUESS_DONE;
			if ( (LA(1)==TYPEDEF) && 
(setwd14[LA(2)]&0x40) ) {
				simple_typedef_definition();
			}
			else {FAIL(2,err38,err39,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
		}
	}
	traceout("typedef_definition");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd14, 0x80);
	traceout("typedef_definition");
}

void
CParser::fptr_typedef_definition(void)
{
	zzRULE;
	tracein("fptr_typedef_definition");
	zzmatch(TYPEDEF); labase++;
	 consume();
	type( d1,d2,d3 );
	fptr_type_alias();
	zzmatch(SEMICOLON); labase++;
	 consume();
	traceout("fptr_typedef_definition");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd15, 0x1);
	traceout("fptr_typedef_definition");
}

void
CParser::struct_typedef_definition(void)
{
	zzRULE;
	string dummy;   
	tracein("struct_typedef_definition");
	zzmatch(TYPEDEF); labase++;
	 consume();
	class_key( dummy );
	identifier_opt();
	brace_block();
	tag_list_opt();
	zzmatch(SEMICOLON); labase++;
	 consume();
	traceout("struct_typedef_definition");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd15, 0x2);
	traceout("struct_typedef_definition");
}

void
CParser::simple_typedef_definition(void)
{
	zzRULE;
	tracein("simple_typedef_definition");
	zzmatch(TYPEDEF); labase++;
	 consume();
	type( d1,d2,d3 );
	simple_type_alias();
	zzmatch(SEMICOLON); labase++;
	 consume();
	traceout("simple_typedef_definition");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd15, 0x4);
	traceout("simple_typedef_definition");
}

void
CParser::identifier_opt(void)
{
	zzRULE;
	tracein("identifier_opt");
	if ( (LA(1)==IDENTIFIER) ) {
		zzmatch(IDENTIFIER); labase++;
		 consume();
	}
	else {
		if ( (LA(1)==LBRACE) ) {
		}
		else {FAIL(1,err40,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
	}
	traceout("identifier_opt");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd15, 0x8);
	traceout("identifier_opt");
}

void
CParser::tag_list_opt(void)
{
	zzRULE;
	tracein("tag_list_opt");
	if ( (setwd15[LA(1)]&0x10) ) {
		tag();
		{
			while ( (LA(1)==COMMA) ) {
				zzmatch(COMMA); labase++;
				 consume();
				tag();
			}
		}
	}
	else {
		if ( (LA(1)==SEMICOLON)
 ) {
		}
		else {FAIL(1,err41,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
	}
	traceout("tag_list_opt");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd15, 0x20);
	traceout("tag_list_opt");
}

void
CParser::tag(void)
{
	zzRULE;
	tracein("tag");
	{
		while ( (LA(1)==ASTERISK) ) {
			zzmatch(ASTERISK); labase++;
			 consume();
		}
	}
	zzmatch(IDENTIFIER); labase++;
	 consume();
	traceout("tag");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd15, 0x40);
	traceout("tag");
}

void
CParser::simple_type_alias(void)
{
	zzRULE;
	ANTLRTokenPtr id=NULL;
	tracein("simple_type_alias");
	zzmatch(IDENTIFIER); labase++;
	
	if ( !guessing ) {
		id = (ANTLRTokenPtr)LT(1);
}
	 consume();
	traceout("simple_type_alias");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd15, 0x80);
	traceout("simple_type_alias");
}

void
CParser::fptr_type_alias(void)
{
	zzRULE;
	tracein("fptr_type_alias");
	zzmatch(LPAREN); labase++;
	 consume();
	zzmatch(ASTERISK); labase++;
	 consume();
	scoped_identifier( d1,d2 );
	zzmatch(RPAREN); labase++;
	 consume();
	paren_block();
	traceout("fptr_type_alias");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd16, 0x1);
	traceout("fptr_type_alias");
}

void
CParser::class_or_method_declaration_or_definition( string& scope )
{
	zzRULE;
	zzGUESS_BLOCK
	string dummy;   
	tracein("class_or_method_declaration_or_definition");
	zzGUESS
	if ( !zzrv && (setwd16[LA(1)]&0x2) && (setwd16[LA(2)]&0x4) ) {
		{
			class_key( dummy );
		}
		zzGUESS_DONE
		class_declaration_or_definition( scope );
	}
	else {
		if ( !zzrv ) zzGUESS_DONE;
		zzGUESS
		if ( !zzrv && (setwd16[LA(1)]&0x8) && (setwd16[LA(2)]&0x10) ) {
			{
				scoped_member_name();
				zzmatch(LPAREN); labase++;
				 consume();
			}
			zzGUESS_DONE
			method_declaration_or_definition_with_implicit_type( scope );
		}
		else {
			if ( !zzrv ) zzGUESS_DONE;
			if ( (setwd16[LA(1)]&0x20) && (setwd16[LA(2)]&0x40) ) {
				method_declaration_or_definition_with_explicit_type( scope );
			}
			else {FAIL(2,err42,err43,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
		}
	}
	traceout("class_or_method_declaration_or_definition");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd16, 0x80);
	traceout("class_or_method_declaration_or_definition");
}

void
CParser::class_prefix( string& modname, string& modtype )
{
	zzRULE;
	tracein("class_prefix");
	if ( (setwd17[LA(1)]&0x1) && 
(setwd17[LA(2)]&0x2) ) {
		class_key( modtype );
		scoped_identifier( d1,modname );
		{
			if ( (LA(1)==LESSTHAN) ) {
				angle_block();
			}
		}
	}
	else {
		if ( (setwd17[LA(1)]&0x4) && (setwd17[LA(2)]&0x8) ) {
			class_key( modtype );
		}
		else {FAIL(2,err44,err45,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
	}
	traceout("class_prefix");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd17, 0x10);
	traceout("class_prefix");
}

void
CParser::inheritance_list( string& childName )
{
	zzRULE;
	ps->set_flag(vPRIVATE);   
	tracein("inheritance_list");
	zzmatch(COLON); labase++;
	 consume();
	inheritance_item_list( childName );
	traceout("inheritance_list");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd17, 0x20);
	traceout("inheritance_list");
}

void
CParser::inheritance_item_list( string& childName )
{
	zzRULE;
	tracein("inheritance_item_list");
	inheritance_item( childName );
	{
		while ( (LA(1)==COMMA) ) {
			zzmatch(COMMA); labase++;
			 consume();
			inheritance_item( childName );
		}
	}
	traceout("inheritance_item_list");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd17, 0x40);
	traceout("inheritance_item_list");
}

void
CParser::inheritance_access_key(void)
{
	zzRULE;
	tracein("inheritance_access_key");
	if ( (LA(1)==VIRTUAL) ) {
		zzmatch(VIRTUAL); labase++;
		 consume();
		{
			if ( (setwd17[LA(1)]&0x80)
 ) {
				access_key();
			}
		}
	}
	else {
		if ( (setwd18[LA(1)]&0x1) ) {
			access_key();
			{
				if ( (LA(1)==VIRTUAL) ) {
					zzmatch(VIRTUAL); labase++;
					 consume();
				}
			}
		}
		else {
			if ( (setwd18[LA(1)]&0x2) ) {
			}
			else {FAIL(1,err46,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
		}
	}
	traceout("inheritance_access_key");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd18, 0x4);
	traceout("inheritance_access_key");
}

void
CParser::inheritance_item( string& childName )
{
	zzRULE;
	
	string parent_scope,parent_name; 
	int startLine=LT(1)->getLine(); 
	tracein("inheritance_item");
	inheritance_access_key();
	type_name( parent_name );
	if ( !guessing ) {
	
	int endLine=LT(1)->getLine();
	ps->record_userel_extent(startLine,endLine,
	childName,"",parent_name,
	"inheritance",
	ps->get_visibility(),
	utINHERITS);
	}
	traceout("inheritance_item");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd18, 0x8);
	traceout("inheritance_item");
}

void
CParser::class_key( string& modtype )
{
	zzRULE;
	modtype=LT(1)->getText();   
	tracein("class_key");
	if ( (LA(1)==CLASS) ) {
		zzmatch(CLASS); labase++;
		
		if ( !guessing ) {
		ps->set_flag(vPRIVATE);   
		}
 consume();
	}
	else {
		if ( (LA(1)==STRUCT)
 ) {
			zzmatch(STRUCT); labase++;
			
			if ( !guessing ) {
			ps->set_flag(vPUBLIC);   
			}
 consume();
		}
		else {FAIL(1,err47,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
	}
	traceout("class_key");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd18, 0x10);
	traceout("class_key");
}

void
CParser::access_key(void)
{
	zzRULE;
	tracein("access_key");
	if ( (LA(1)==PUBLIC) ) {
		zzmatch(PUBLIC); labase++;
		
		if ( !guessing ) {
		
		ps->set_flag(vPUBLIC); 
		}
 consume();
	}
	else {
		if ( (LA(1)==PRIVATE) ) {
			zzmatch(PRIVATE); labase++;
			
			if ( !guessing ) {
			
			ps->set_flag(vPRIVATE); 
			}
 consume();
		}
		else {
			if ( (LA(1)==PROTECTED) ) {
				zzmatch(PROTECTED); labase++;
				
				if ( !guessing ) {
				
				ps->set_flag(vPROTECTED); 
				}
 consume();
			}
			else {FAIL(1,err48,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
		}
	}
	traceout("access_key");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd18, 0x20);
	traceout("access_key");
}

void
CParser::ctor_init_list(void)
{
	zzRULE;
	tracein("ctor_init_list");
	zzmatch(COLON); labase++;
	 consume();
	ctor_init_item_list();
	traceout("ctor_init_list");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd18, 0x40);
	traceout("ctor_init_list");
}

void
CParser::ctor_init_item_list(void)
{
	zzRULE;
	zzGUESS_BLOCK
	tracein("ctor_init_item_list");
	zzGUESS
	if ( !zzrv && (setwd18[LA(1)]&0x80) && (setwd19[LA(2)]&0x1) ) {
		{
			ctor_init_item();
			zzmatch(COMMA); labase++;
			 consume();
		}
		zzGUESS_DONE
		ctor_init_item();
		zzmatch(COMMA); labase++;
		 consume();
		ctor_init_item_list();
	}
	else {
		if ( !zzrv ) zzGUESS_DONE;
		if ( (setwd19[LA(1)]&0x2) && 
(setwd19[LA(2)]&0x4) ) {
			ctor_init_item();
		}
		else {FAIL(2,err49,err50,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
	}
	traceout("ctor_init_item_list");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd19, 0x8);
	traceout("ctor_init_item_list");
}

void
CParser::ctor_init_item(void)
{
	zzRULE;
	tracein("ctor_init_item");
	instance_item( d1,d2 );
	traceout("ctor_init_item");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd19, 0x10);
	traceout("ctor_init_item");
}

void
CParser::linkage_qualifiers(void)
{
	zzRULE;
	zzGUESS_BLOCK
	tracein("linkage_qualifiers");
	zzGUESS
	if ( !zzrv && (setwd19[LA(1)]&0x20) && (setwd19[LA(2)]&0x40) ) {
		{
			linkage_qualifier();
		}
		zzGUESS_DONE
		linkage_qualifier();
		linkage_qualifiers();
	}
	else {
		if ( !zzrv ) zzGUESS_DONE;
		if ( (setwd19[LA(1)]&0x80) && (setwd20[LA(2)]&0x1) ) {
		}
		else {FAIL(2,err51,err52,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
	}
	traceout("linkage_qualifiers");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd20, 0x2);
	traceout("linkage_qualifiers");
}

void
CParser::linkage_qualifier(void)
{
	zzRULE;
	zzGUESS_BLOCK
	tracein("linkage_qualifier");
	if ( (LA(1)==STATIC) ) {
		zzmatch(STATIC); labase++;
		
		if ( !guessing ) {
		ps->set_flag(psfSTATIC,abTRUE);   
		}
 consume();
	}
	else {
		zzGUESS
		if ( !zzrv && (LA(1)==EXTERN) && (LA(2)==STRINGCONST) ) {
			{
				zzmatch(EXTERN); labase++;
				 consume();
				zzmatch(STRINGCONST); labase++;
				 consume();
			}
			zzGUESS_DONE
			{
				zzmatch(EXTERN); labase++;
				 consume();
				zzmatch(STRINGCONST); labase++;
				 consume();
			}
		}
		else {
			if ( !zzrv ) zzGUESS_DONE;
			if ( (LA(1)==EXTERN) && 
(setwd20[LA(2)]&0x4) ) {
				zzmatch(EXTERN); labase++;
				 consume();
			}
			else {
				if ( !zzrv ) zzGUESS_DONE;
				if ( (LA(1)==INLINE) ) {
					zzmatch(INLINE); labase++;
					 consume();
				}
				else {
					if ( !zzrv ) zzGUESS_DONE;
					if ( (LA(1)==TEMPLATE) ) {
						zzmatch(TEMPLATE); labase++;
						 consume();
						{
							if ( (LA(1)==LESSTHAN) ) {
								angle_block();
							}
						}
					}
					else {FAIL(2,err53,err54,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
				}
			}
		}
	}
	traceout("linkage_qualifier");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd20, 0x8);
	traceout("linkage_qualifier");
}

void
CParser::identifier_or_brace_block_or_both(void)
{
	zzRULE;
	tracein("identifier_or_brace_block_or_both");
	zzmatch(IDENTIFIER); labase++;
	 consume();
	opt_brace_block();
	traceout("identifier_or_brace_block_or_both");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd20, 0x10);
	traceout("identifier_or_brace_block_or_both");
}

void
CParser::opt_brace_block(void)
{
	zzRULE;
	zzGUESS_BLOCK
	tracein("opt_brace_block");
	zzGUESS
	if ( !zzrv && (LA(1)==LBRACE) ) {
		{
			brace_block();
		}
		zzGUESS_DONE
		{
			brace_block();
		}
	}
	else {
		if ( !zzrv ) zzGUESS_DONE;
		if ( (LA(1)==Eof)
 ) {
		}
		else {FAIL(1,err55,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
	}
	traceout("opt_brace_block");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd20, 0x20);
	traceout("opt_brace_block");
}

void
CParser::instance_item( string& indir,string& name )
{
	zzRULE;
	tracein("instance_item");
	item_specifier( indir,name );
	brack_list();
	opt_initializer();
	traceout("instance_item");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd20, 0x40);
	traceout("instance_item");
}

void
CParser::item_specifier( string& indir,string& name )
{
	zzRULE;
	tracein("item_specifier");
	if ( (LA(1)==LPAREN) ) {
		zzmatch(LPAREN); labase++;
		 consume();
		zzmatch(ASTERISK); labase++;
		 consume();
		scoped_member_name();
		zzmatch(RPAREN); labase++;
		 consume();
		paren_block();
	}
	else {
		if ( (setwd20[LA(1)]&0x80) ) {
			{
				while ( (setwd21[LA(1)]&0x1) ) {
					indirection_modifier( indir );
				}
			}
			scoped_identifier( d1,name );
		}
		else {FAIL(1,err56,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
	}
	traceout("item_specifier");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd21, 0x2);
	traceout("item_specifier");
}

void
CParser::opt_initializer(void)
{
	zzRULE;
	zzGUESS_BLOCK
	string dummy;   
	tracein("opt_initializer");
	if ( (LA(1)==ASSIGN_OP) ) {
		zzmatch(ASSIGN_OP); labase++;
		 consume();
		init_expr();
	}
	else {
		zzGUESS
		if ( !zzrv && (LA(1)==LPAREN) && 
(LA(2)==RPAREN) ) {
			{
				zzmatch(LPAREN); labase++;
				 consume();
				zzmatch(RPAREN); labase++;
				 consume();
			}
			zzGUESS_DONE
			{
				zzmatch(LPAREN); labase++;
				 consume();
				zzmatch(RPAREN); labase++;
				 consume();
			}
		}
		else {
			if ( !zzrv ) zzGUESS_DONE;
			if ( (LA(1)==LPAREN) && (setwd21[LA(2)]&0x4) ) {
				zzmatch(LPAREN); labase++;
				 consume();
				init_expr();
				zzmatch(RPAREN); labase++;
				 consume();
			}
			else {
				if ( !zzrv ) zzGUESS_DONE;
				if ( (setwd21[LA(1)]&0x8) ) {
				}
				else {FAIL(2,err57,err58,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
			}
		}
	}
	traceout("opt_initializer");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd21, 0x10);
	traceout("opt_initializer");
}

void
CParser::init_expr(void)
{
	zzRULE;
	zzGUESS_BLOCK
	tracein("init_expr");
	zzGUESS
	if ( !zzrv && (setwd21[LA(1)]&0x20) && (setwd21[LA(2)]&0x40) ) {
		{
			init_expr_item();
			op();
		}
		zzGUESS_DONE
		init_expr_item();
		op();
		init_expr();
	}
	else {
		if ( !zzrv ) zzGUESS_DONE;
		if ( (setwd21[LA(1)]&0x80) && (setwd22[LA(2)]&0x1) ) {
			init_expr_item();
		}
		else {FAIL(2,err59,err60,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
	}
	traceout("init_expr");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd22, 0x2);
	traceout("init_expr");
}

void
CParser::init_expr_item(void)
{
	zzRULE;
	zzGUESS_BLOCK
	tracein("init_expr_item");
	if ( (LA(1)==SIZEOF)
 ) {
		zzmatch(SIZEOF); labase++;
		 consume();
		paren_block();
	}
	else {
		if ( (LA(1)==LPAREN) ) {
			paren_block();
		}
		else {
			if ( (LA(1)==LBRACE) ) {
				brace_block();
			}
			else {
				zzGUESS
				if ( !zzrv && (LA(1)==IDENTIFIER) && (LA(2)==LPAREN) ) {
					{
						zzmatch(IDENTIFIER); labase++;
						 consume();
						paren_block();
					}
					zzGUESS_DONE
					{
						zzmatch(IDENTIFIER); labase++;
						 consume();
						paren_block();
					}
				}
				else {
					if ( !zzrv ) zzGUESS_DONE;
					if ( (setwd22[LA(1)]&0x4) ) {
						cast_keyword();
						angle_block();
						paren_block();
					}
					else {
						if ( !zzrv ) zzGUESS_DONE;
						if ( (setwd22[LA(1)]&0x8) && 
(setwd22[LA(2)]&0x10) ) {
							constant();
						}
						else {FAIL(2,err61,err62,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
					}
				}
			}
		}
	}
	traceout("init_expr_item");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd22, 0x20);
	traceout("init_expr_item");
}

void
CParser::cast_keyword(void)
{
	zzRULE;
	tracein("cast_keyword");
	if ( (LA(1)==STATIC_CAST) ) {
		zzmatch(STATIC_CAST); labase++;
		 consume();
	}
	else {
		if ( (LA(1)==CONST_CAST) ) {
			zzmatch(CONST_CAST); labase++;
			 consume();
		}
		else {
			if ( (LA(1)==REINTERPRET_CAST) ) {
				zzmatch(REINTERPRET_CAST); labase++;
				 consume();
			}
			else {FAIL(1,err63,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
		}
	}
	traceout("cast_keyword");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd22, 0x40);
	traceout("cast_keyword");
}

void
CParser::init_value(void)
{
	zzRULE;
	tracein("init_value");
	if ( (setwd22[LA(1)]&0x80) ) {
		constant();
	}
	else {
		if ( (LA(1)==LBRACE)
 ) {
			brace_block();
		}
		else {
			if ( (LA(1)==LPAREN) ) {
				paren_block();
			}
			else {FAIL(1,err64,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
		}
	}
	traceout("init_value");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd23, 0x1);
	traceout("init_value");
}

void
CParser::keyword(void)
{
	zzRULE;
	tracein("keyword");
	if ( (LA(1)==ASM) ) {
		zzmatch(ASM); labase++;
		 consume();
	}
	else {
		if ( (LA(1)==AUTO) ) {
			zzmatch(AUTO); labase++;
			 consume();
		}
		else {
			if ( (LA(1)==KW_BOOL) ) {
				zzmatch(KW_BOOL); labase++;
				 consume();
			}
			else {
				if ( (LA(1)==BREAK)
 ) {
					zzmatch(BREAK); labase++;
					 consume();
				}
				else {
					if ( (LA(1)==CASE) ) {
						zzmatch(CASE); labase++;
						 consume();
					}
					else {
						if ( (LA(1)==CATCH) ) {
							zzmatch(CATCH); labase++;
							 consume();
						}
						else {
							if ( (LA(1)==KW_CHAR) ) {
								zzmatch(KW_CHAR); labase++;
								 consume();
							}
							else {
								if ( (LA(1)==CLASS) ) {
									zzmatch(CLASS); labase++;
									 consume();
								}
								else {
									if ( (LA(1)==KW_CONST)
 ) {
										zzmatch(KW_CONST); labase++;
										 consume();
									}
									else {
										if ( (LA(1)==CONTINUE) ) {
											zzmatch(CONTINUE); labase++;
											 consume();
										}
										else {
											if ( (LA(1)==DEFAULT) ) {
												zzmatch(DEFAULT); labase++;
												 consume();
											}
											else {
												if ( (LA(1)==DELETE) ) {
													zzmatch(DELETE); labase++;
													 consume();
												}
												else {
													if ( (LA(1)==KW_DOUBLE) ) {
														zzmatch(KW_DOUBLE); labase++;
														 consume();
													}
													else {
														if ( (LA(1)==DO)
 ) {
															zzmatch(DO); labase++;
															 consume();
														}
														else {
															if ( (LA(1)==DYNAMIC_CAST) ) {
																zzmatch(DYNAMIC_CAST); labase++;
																 consume();
															}
															else {
																if ( (LA(1)==ELSE) ) {
																	zzmatch(ELSE); labase++;
																	 consume();
																}
																else {
																	if ( (LA(1)==ENUM) ) {
																		zzmatch(ENUM); labase++;
																		 consume();
																	}
																	else {
																		if ( (LA(1)==EXTERN) ) {
																			zzmatch(EXTERN); labase++;
																			 consume();
																		}
																		else {
																			if ( (LA(1)==EXPLICIT)
 ) {
																				zzmatch(EXPLICIT); labase++;
																				 consume();
																			}
																			else {
																				if ( (LA(1)==BFALSE) ) {
																					zzmatch(BFALSE); labase++;
																					 consume();
																				}
																				else {
																					if ( (LA(1)==KW_FLOAT) ) {
																						zzmatch(KW_FLOAT); labase++;
																						 consume();
																					}
																					else {
																						if ( (LA(1)==FOR) ) {
																							zzmatch(FOR); labase++;
																							 consume();
																						}
																						else {
																							if ( (LA(1)==FRIEND) ) {
																								zzmatch(FRIEND); labase++;
																								 consume();
																							}
																							else {
																								if ( (LA(1)==GOTO)
 ) {
																									zzmatch(GOTO); labase++;
																									 consume();
																								}
																								else {
																									if ( (LA(1)==IF) ) {
																										zzmatch(IF); labase++;
																										 consume();
																									}
																									else {
																										if ( (LA(1)==INLINE) ) {
																											zzmatch(INLINE); labase++;
																											 consume();
																										}
																										else {
																											if ( (LA(1)==KW_INT) ) {
																												zzmatch(KW_INT); labase++;
																												 consume();
																											}
																											else {
																												if ( (LA(1)==KW_LONG) ) {
																													zzmatch(KW_LONG); labase++;
																													 consume();
																												}
																												else {
																													if ( (LA(1)==NEW)
 ) {
																														zzmatch(NEW); labase++;
																														 consume();
																													}
																													else {
																														if ( (LA(1)==OPERATOR) ) {
																															zzmatch(OPERATOR); labase++;
																															 consume();
																														}
																														else {
																															if ( (LA(1)==PRIVATE) ) {
																																zzmatch(PRIVATE); labase++;
																																 consume();
																															}
																															else {
																																if ( (LA(1)==PROTECTED) ) {
																																	zzmatch(PROTECTED); labase++;
																																	 consume();
																																}
																																else {
																																	if ( (LA(1)==PUBLIC) ) {
																																		zzmatch(PUBLIC); labase++;
																																		 consume();
																																	}
																																	else {
																																		if ( (LA(1)==REGISTER)
 ) {
																																			zzmatch(REGISTER); labase++;
																																			 consume();
																																		}
																																		else {
																																			if ( (LA(1)==REINTERPRET_CAST) ) {
																																				zzmatch(REINTERPRET_CAST); labase++;
																																				 consume();
																																			}
																																			else {
																																				if ( (LA(1)==RETURN) ) {
																																					zzmatch(RETURN); labase++;
																																					 consume();
																																				}
																																				else {
																																					if ( (LA(1)==KW_SHORT) ) {
																																						zzmatch(KW_SHORT); labase++;
																																						 consume();
																																					}
																																					else {
																																						if ( (LA(1)==SIGNED) ) {
																																							zzmatch(SIGNED); labase++;
																																							 consume();
																																						}
																																						else {
																																							if ( (LA(1)==SIZEOF)
 ) {
																																								zzmatch(SIZEOF); labase++;
																																								 consume();
																																							}
																																							else {
																																								if ( (LA(1)==STATIC) ) {
																																									zzmatch(STATIC); labase++;
																																									 consume();
																																								}
																																								else {
																																									if ( (LA(1)==STATIC_CAST) ) {
																																										zzmatch(STATIC_CAST); labase++;
																																										 consume();
																																									}
																																									else {
																																										if ( (LA(1)==STRUCT) ) {
																																											zzmatch(STRUCT); labase++;
																																											 consume();
																																										}
																																										else {
																																											if ( (LA(1)==SWITCH) ) {
																																												zzmatch(SWITCH); labase++;
																																												 consume();
																																											}
																																											else {
																																												if ( (LA(1)==TEMPLATE)
 ) {
																																													zzmatch(TEMPLATE); labase++;
																																													 consume();
																																												}
																																												else {
																																													if ( (LA(1)==KW_THIS) ) {
																																														zzmatch(KW_THIS); labase++;
																																														 consume();
																																													}
																																													else {
																																														if ( (LA(1)==THROW) ) {
																																															zzmatch(THROW); labase++;
																																															 consume();
																																														}
																																														else {
																																															if ( (LA(1)==BTRUE) ) {
																																																zzmatch(BTRUE); labase++;
																																																 consume();
																																															}
																																															else {
																																																if ( (LA(1)==TRY) ) {
																																																	zzmatch(TRY); labase++;
																																																	 consume();
																																																}
																																																else {
																																																	if ( (LA(1)==TYPEDEF)
 ) {
																																																		zzmatch(TYPEDEF); labase++;
																																																		 consume();
																																																	}
																																																	else {
																																																		if ( (LA(1)==UNION) ) {
																																																			zzmatch(UNION); labase++;
																																																			 consume();
																																																		}
																																																		else {
																																																			if ( (LA(1)==UNSIGNED) ) {
																																																				zzmatch(UNSIGNED); labase++;
																																																				 consume();
																																																			}
																																																			else {
																																																				if ( (LA(1)==VIRTUAL) ) {
																																																					zzmatch(VIRTUAL); labase++;
																																																					 consume();
																																																				}
																																																				else {
																																																					if ( (LA(1)==KW_VOID) ) {
																																																						zzmatch(KW_VOID); labase++;
																																																						 consume();
																																																					}
																																																					else {
																																																						if ( (LA(1)==VOLATILE)
 ) {
																																																							zzmatch(VOLATILE); labase++;
																																																							 consume();
																																																						}
																																																						else {
																																																							if ( (LA(1)==WHILE) ) {
																																																								zzmatch(WHILE); labase++;
																																																								 consume();
																																																							}
																																																							else {FAIL(1,err65,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
																																																						}
																																																					}
																																																				}
																																																			}
																																																		}
																																																	}
																																																}
																																															}
																																														}
																																													}
																																												}
																																											}
																																										}
																																									}
																																								}
																																							}
																																						}
																																					}
																																				}
																																			}
																																		}
																																	}
																																}
																															}
																														}
																													}
																												}
																											}
																										}
																									}
																								}
																							}
																						}
																					}
																				}
																			}
																		}
																	}
																}
															}
														}
													}
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
	traceout("keyword");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd23, 0x2);
	traceout("keyword");
}

void
CParser::op(void)
{
	zzRULE;
	tracein("op");
	if ( (setwd23[LA(1)]&0x4) ) {
		zzsetmatch(EQUAL_OP_set); labase++;
		 consume();
	}
	else {
		if ( (LA(1)==ASSIGN_OP) ) {
			zzmatch(ASSIGN_OP); labase++;
			 consume();
		}
		else {
			if ( (setwd23[LA(1)]&0x8) ) {
				zzsetmatch(OP_ASSIGN_OP_set); labase++;
				 consume();
			}
			else {
				if ( (setwd23[LA(1)]&0x10)
 ) {
					zzsetmatch(SHIFT_OP_set); labase++;
					 consume();
				}
				else {
					if ( (setwd23[LA(1)]&0x20) ) {
						zzsetmatch(REL_OP_set); labase++;
						 consume();
					}
					else {
						if ( (LA(1)==ASTERISK) ) {
							zzmatch(ASTERISK); labase++;
							 consume();
						}
						else {
							if ( (setwd23[LA(1)]&0x40) ) {
								zzsetmatch(DIV_OP_set); labase++;
								 consume();
							}
							else {
								if ( (setwd23[LA(1)]&0x80) ) {
									zzsetmatch(PM_OP_set); labase++;
									 consume();
								}
								else {
									if ( (setwd24[LA(1)]&0x1)
 ) {
										zzsetmatch(INCR_OP_set); labase++;
										 consume();
									}
									else {
										if ( (setwd24[LA(1)]&0x2) ) {
											zzsetmatch(ADD_OP_set); labase++;
											 consume();
										}
										else {
											if ( (LA(1)==QUERY_OP) ) {
												zzmatch(QUERY_OP); labase++;
												 consume();
											}
											else {
												if ( (LA(1)==LOGICAL_AND_OP) ) {
													zzmatch(LOGICAL_AND_OP); labase++;
													 consume();
												}
												else {
													if ( (LA(1)==LOGICAL_OR_OP) ) {
														zzmatch(LOGICAL_OR_OP); labase++;
														 consume();
													}
													else {
														if ( (LA(1)==LOGICAL_NOT_OP)
 ) {
															zzmatch(LOGICAL_NOT_OP); labase++;
															 consume();
														}
														else {
															if ( (setwd24[LA(1)]&0x4) ) {
																zzsetmatch(BITWISE_OP_set); labase++;
																 consume();
															}
															else {
																if ( (LA(1)==COLONCOLON) ) {
																	zzmatch(COLONCOLON); labase++;
																	 consume();
																}
																else {
																	if ( (LA(1)==COLON) ) {
																		zzmatch(COLON); labase++;
																		 consume();
																	}
																	else {
																		if ( (LA(1)==PERIOD) ) {
																			zzmatch(PERIOD); labase++;
																			 consume();
																		}
																		else {
																			if ( (LA(1)==ARROW)
 ) {
																				zzmatch(ARROW); labase++;
																				 consume();
																			}
																			else {
																				if ( (LA(1)==COMMA) ) {
																					zzmatch(COMMA); labase++;
																					 consume();
																				}
																				else {FAIL(1,err75,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
																			}
																		}
																	}
																}
															}
														}
													}
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
	traceout("op");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd24, 0x8);
	traceout("op");
}

void
CParser::constant(void)
{
	zzRULE;
	tracein("constant");
	if ( (setwd24[LA(1)]&0x10) ) {
		literal();
	}
	else {
		if ( (LA(1)==IDENTIFIER) ) {
			zzmatch(IDENTIFIER); labase++;
			 consume();
		}
		else {FAIL(1,err76,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
	}
	traceout("constant");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd24, 0x20);
	traceout("constant");
}

void
CParser::literal(void)
{
	zzRULE;
	tracein("literal");
	if ( (LA(1)==STRINGCONST) ) {
		string_literal();
	}
	else {
		if ( (LA(1)==CHARCONST)
 ) {
			zzmatch(CHARCONST); labase++;
			 consume();
		}
		else {
			if ( (LA(1)==FNUM) ) {
				zzmatch(FNUM); labase++;
				 consume();
			}
			else {
				if ( (LA(1)==OCT_NUM) ) {
					zzmatch(OCT_NUM); labase++;
					 consume();
				}
				else {
					if ( (LA(1)==L_OCT_NUM) ) {
						zzmatch(L_OCT_NUM); labase++;
						 consume();
					}
					else {
						if ( (LA(1)==HEX_NUM) ) {
							zzmatch(HEX_NUM); labase++;
							 consume();
						}
						else {
							if ( (LA(1)==L_HEX_NUM)
 ) {
								zzmatch(L_HEX_NUM); labase++;
								 consume();
							}
							else {
								if ( (LA(1)==INT_NUM) ) {
									zzmatch(INT_NUM); labase++;
									 consume();
								}
								else {
									if ( (LA(1)==L_INT_NUM) ) {
										zzmatch(L_INT_NUM); labase++;
										 consume();
									}
									else {
										if ( (LA(1)==BTRUE) ) {
											zzmatch(BTRUE); labase++;
											 consume();
										}
										else {
											if ( (LA(1)==BFALSE) ) {
												zzmatch(BFALSE); labase++;
												 consume();
											}
											else {
												if ( (setwd24[LA(1)]&0x40)
 ) {
													zzsetmatch(ADD_OP_set); labase++;
													 consume();
													literal();
												}
												else {FAIL(1,err77,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
	traceout("literal");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd24, 0x80);
	traceout("literal");
}

void
CParser::string_literal(void)
{
	zzRULE;
	zzGUESS_BLOCK
	tracein("string_literal");
	zzGUESS
	if ( !zzrv && (LA(1)==STRINGCONST) && (LA(2)==STRINGCONST) ) {
		{
			zzmatch(STRINGCONST); labase++;
			 consume();
			zzmatch(STRINGCONST); labase++;
			 consume();
		}
		zzGUESS_DONE
		zzmatch(STRINGCONST); labase++;
		 consume();
		string_literal();
	}
	else {
		if ( !zzrv ) zzGUESS_DONE;
		if ( (LA(1)==STRINGCONST) && (setwd25[LA(2)]&0x1) ) {
			zzmatch(STRINGCONST); labase++;
			 consume();
		}
		else {FAIL(2,err78,err79,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
	}
	traceout("string_literal");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd25, 0x2);
	traceout("string_literal");
}

void
CParser::block(void)
{
	zzRULE;
	tracein("block");
	if ( (LA(1)==LBRACE) ) {
		brace_block();
	}
	else {
		if ( (LA(1)==LBRACK) ) {
			brack_block();
		}
		else {
			if ( (LA(1)==LPAREN)
 ) {
				paren_block();
			}
			else {FAIL(1,err80,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
		}
	}
	traceout("block");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd25, 0x4);
	traceout("block");
}

void
CParser::balanced(void)
{
	zzRULE;
	tracein("balanced");
	if ( (setwd25[LA(1)]&0x8) ) {
		scoped();
	}
	else {
		if ( (setwd25[LA(1)]&0x10) ) {
			block();
		}
		else {
			if ( (LA(1)==SEMICOLON) ) {
				zzmatch(SEMICOLON); labase++;
				 consume();
			}
			else {FAIL(1,err81,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
		}
	}
	traceout("balanced");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd25, 0x20);
	traceout("balanced");
}

void
CParser::balanced_list(void)
{
	zzRULE;
	zzGUESS_BLOCK
	tracein("balanced_list");
	zzGUESS
	if ( !zzrv && (setwd25[LA(1)]&0x40) ) {
		{
			balanced();
		}
		zzGUESS_DONE
		balanced();
		balanced_list();
	}
	else {
		if ( !zzrv ) zzGUESS_DONE;
		if ( (setwd25[LA(1)]&0x80)
 ) {
		}
		else {FAIL(1,err82,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
	}
	traceout("balanced_list");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd26, 0x1);
	traceout("balanced_list");
}

void
CParser::nested_token_list(  int nl  )
{
	zzRULE;
	tracein("nested_token_list");
	if ( (setwd26[LA(1)]&0x2) ) {
		nested_token( nl );
		nested_token_list( nl );
	}
	else {
		if ( (LA(1)==Eof) ) {
		}
		else {FAIL(1,err83,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
	}
	traceout("nested_token_list");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd26, 0x4);
	traceout("nested_token_list");
}

void
CParser::nested_token(  int nl  )
{
	zzRULE;
	ANTLRTokenPtr tok=NULL;
	ANTLRTokenPtr la_ptr=LT(1);   
	tracein("nested_token");
	if (!((la_ptr!=0) && (mytoken(la_ptr)->getNestingLevel() > nl) )) {zzfailed_pred("   (la_ptr!=0) && (mytoken(la_ptr)->getNestingLevel() > nl) ");}
	zzsetmatch(WildCard_set); labase++;
	
	if ( !guessing ) {
		tok = (ANTLRTokenPtr)LT(1);
}
	 consume();
	traceout("nested_token");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd26, 0x8);
	traceout("nested_token");
}

void
CParser::scoped(void)
{
	zzRULE;
	zzGUESS_BLOCK
	tracein("scoped");
	zzGUESS
	if ( !zzrv && (setwd26[LA(1)]&0x10) && (setwd26[LA(2)]&0x20) ) {
		{
			keyword();
		}
		zzGUESS_DONE
		{
			keyword();
		}
	}
	else {
		if ( !zzrv ) zzGUESS_DONE;
		zzGUESS
		if ( !zzrv && (setwd26[LA(1)]&0x40) && (setwd26[LA(2)]&0x80) ) {
			{
				op();
			}
			zzGUESS_DONE
			{
				op();
			}
		}
		else {
			if ( !zzrv ) zzGUESS_DONE;
			if ( (LA(1)==IDENTIFIER)
 ) {
				zzmatch(IDENTIFIER); labase++;
				 consume();
			}
			else {
				if ( !zzrv ) zzGUESS_DONE;
				if ( (setwd27[LA(1)]&0x1) && (setwd27[LA(2)]&0x2) ) {
					literal();
				}
				else {FAIL(2,err85,err86,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
			}
		}
	}
	traceout("scoped");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd27, 0x4);
	traceout("scoped");
}

void
CParser::brace_block(void)
{
	zzRULE;
	int brace_level=MY_TOK(LT(1))->getNestingLevel();   
	tracein("brace_block");
	zzmatch(LBRACE); labase++;
	 consume();
	skip_until_matching_rbrace( brace_level );
	traceout("brace_block");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd27, 0x8);
	traceout("brace_block");
}

void
CParser::skip_until_matching_rbrace(  int brace_level  )
{
	zzRULE;
	
	// this is an init action, so it should be executed unconditionally
	// when we try to match this rule
	while(MY_TOK(LT(1))->getNestingLevel()>=brace_level)
	{
		if(LT(1)->getType()==Eof)
		{
			// We have reached the end of file with unbalanced {} nesting.
			// Presumably somebody stuffed up.  Maybe a preprocessor problem.
			// Anyway, get out of this rule.  Expect a syntax error RSN.
			break;
		}
		else
		{
			consume();
		}
	}
	tracein("skip_until_matching_rbrace");
	zzmatch(RBRACE); labase++;
	 consume();
	traceout("skip_until_matching_rbrace");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd27, 0x10);
	traceout("skip_until_matching_rbrace");
}

void
CParser::paren_block(void)
{
	zzRULE;
	tracein("paren_block");
	zzmatch(LPAREN); labase++;
	 consume();
	balanced_list();
	zzmatch(RPAREN); labase++;
	 consume();
	traceout("paren_block");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd27, 0x20);
	traceout("paren_block");
}

void
CParser::brack_block(void)
{
	zzRULE;
	tracein("brack_block");
	zzmatch(LBRACK); labase++;
	 consume();
	balanced_list();
	zzmatch(RBRACK); labase++;
	 consume();
	traceout("brack_block");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd27, 0x40);
	traceout("brack_block");
}

void
CParser::brack_list(void)
{
	zzRULE;
	zzGUESS_BLOCK
	tracein("brack_list");
	zzGUESS
	if ( !zzrv && (LA(1)==LBRACK) ) {
		{
			brack_block();
		}
		zzGUESS_DONE
		brack_block();
		brack_list();
	}
	else {
		if ( !zzrv ) zzGUESS_DONE;
		if ( (setwd27[LA(1)]&0x80) ) {
		}
		else {FAIL(1,err87,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
	}
	traceout("brack_list");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd28, 0x1);
	traceout("brack_list");
}

void
CParser::angle_balanced_list(void)
{
	zzRULE;
	tracein("angle_balanced_list");
	if ( (LA(1)==LESSTHAN) && (setwd28[LA(2)]&0x2)&&(LT(1)->getType() == LESSTHAN ) ) {
		if (!(LT(1)->getType() == LESSTHAN )) {zzfailed_pred("   LT(1)->getType() == LESSTHAN ");}
		angle_block();
		angle_balanced_list();
	}
	else {
		if ( (LA(1)==GREATERTHAN)
 ) {
			if (!(LT(1)->getType() == GREATERTHAN )) {zzfailed_pred("   LT(1)->getType() == GREATERTHAN ");}
		}
		else {
			if ( (setwd28[LA(1)]&0x4) && (setwd28[LA(2)]&0x8) ) {
				zzsetmatch(err88); labase++;
				 consume();
				angle_balanced_list();
			}
			else {FAIL(2,err89,err90,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
		}
	}
	traceout("angle_balanced_list");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd28, 0x10);
	traceout("angle_balanced_list");
}

void
CParser::angle_block(void)
{
	zzRULE;
	tracein("angle_block");
	zzmatch(LESSTHAN); labase++;
	 consume();
	angle_balanced_list();
	zzmatch(GREATERTHAN); labase++;
	 consume();
	traceout("angle_block");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd28, 0x20);
	traceout("angle_block");
}
