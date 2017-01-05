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
/*
 * 
 * A coarse grammar for C++
 * This file is designed to be processed by the PCCTS utilities ANTLR and 
 * DLG to generate a scanner and parser for the cccc metric analyser project
 *
 * by Tim Littlefair, 1995-2001
 */

#header<<
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
>>

#lexaction
<<
#include <cassert>

// Need to see definition of CLexer class before defining the 
// next actions.
// This will mean this file is included twice in CLexer.cpp.  This
// should do no harm...
#include "CLexer.h"


// The lexical analyser passes some information through to
// the parse store to be integrated with parse information before
// recording in the database.
inline void IncrementCount(LexicalCount tc) 
{
  assert(ParseStore::currentInstance()!=NULL);
  ParseStore::currentInstance()->IncrementCount(tc);
}

inline void endOfLine(CLexer &lexer)
{
  assert(ParseStore::currentInstance()!=NULL);
  ParseStore::currentInstance()->endOfLine(lexer.line());
  lexer.newline();
  lexer.skip();
}

>>

#token Eof "@" << replstr("<EOF>"); >>

#token WHITESPACE "[\ \t\r]+" << skip(); >>

// handle newlines
#token DOS_NL		"\r\n"  << endOfLine(*this); >>
#token MAC_NL		"\r"	<< endOfLine(*this); >>
#token UNIX_NL		"\n"    << endOfLine(*this); >>


/* preprocessor constructs - comments, #defines etc */

#token HASH "#" << mode(PREPROC); more(); >>
#lexclass PREPROC
#token DOS_P_EOL "\r\n" << mode(START); endOfLine(*this); >>
#token MAC_P_EOL "\r" << mode(START); endOfLine(*this); >>
#token UNIX_P_EOL "\n" << mode(START); endOfLine(*this); >>
#token P_LINECONT "\\\n" << ; skip(); >>
#token P_ANYTHING "~[\n]" << ; more(); >>
#token P_COMMULTI "/\*" << mode(COMMENT_MULTI); more(); skip(); >>
#lexclass START

/*
** The object oriented design editor Rational Rose/C++ generates source
** inserting directives into source files disguised as comments so that
** C++ source contributed by Rational Rose, which Rose is at liberty to
** change or overwrite can be distinguished from source code written by 
** the programmer, which Rose will do its best to preserve.
** We do not count these as user comments.
*/
#token RR_DIRECTIVE "//##" << mode(RR); skip(); >>
#lexclass RR
#token RR_ANYTHING "~[\n]" << skip(); >>
#token RR_END "\n" << mode(START); endOfLine(*this); >>
#lexclass START

/*
** for some reason, the string //#define gives an invalid token error
** with //# quoted as the token name, the 'd' lost in space and 'efine'
** quoted as the next token.  For the time being we will just try to trap
** this here (NB we _do_ count this as a user comment)
*/
#token COMDEF "//#" << mode(COMMENT_LINE); skip(); >>
#token COMLINE "//" << mode(COMMENT_LINE); skip(); >>
#lexclass COMMENT_LINE
#token COMLINE_END  "\n" << 
  IncrementCount(tcCOMLINES); 
  endOfLine(*this);
  mode(START); 
>>
#token COMLINE_ANYTHING "~[\n]" << skip(); >>
#lexclass START

#token COMMULTI "/\*" << mode(COMMENT_MULTI); skip(); >>
#lexclass COMMENT_MULTI
#token COMMULTI_END "\*/" << 
  IncrementCount(tcCOMLINES); 
  mode(START); 
  skip(); 
>>
#token COMMULTI_EOL "\n" << IncrementCount(tcCOMLINES); endOfLine(*this); >>
#token COMMULTI_ANYTHING "~[\n]" << skip(); >>
#lexclass START

#token STRINGSTART "\"" << mode(CONST_STRING); skip(); >>
#lexclass CONST_STRING
#token STRINGCONST "\"" << mode(START); >>
// thanks to Lynn Wilson for pointing out the need for a simple fix 
// to handle escaped newlines within string constants
#token LYNNS_FIX "\\\n"  <<  endOfLine(*this); >>   
#token ESCAPED_DQUOTE "\\\"" << skip(); >>
#token ESCAPED_OTHER "\\~[\"]" << skip(); >>
#token S_ANYTHING "~[\"]" << skip(); >>
#lexclass START

#token CHARSTART "\'" << mode(CONST_CHAR); skip(); >>
#lexclass CONST_CHAR
#token CHARCONST "'" << replstr("'.'"); mode(START); >>
#token CH_ANYTHING "~[']" << skip(); >>
#lexclass START


#token LBRACE "\{" << ANTLRToken::IncrementNesting(); >>
#token RBRACE "\}" << ANTLRToken::DecrementNesting(); >>
#token LPAREN "\(" << ; >>
#token RPAREN "\)" << ; >>
#token LBRACK "\[" << ; >>
#token RBRACK "\]" << ; >>


/* keywords */

// Some of the keywords below have recently acquired the prefix
// KW_.  This is to avoid clashes between items in the token type
// enumeration generated by PCCTS and predefined datatypes in Windows.
// (Problem first seen under Borland C++ Builder v3).

#token ASM "asm" << ; >>
#token AUTO "auto" << ; >>
#token BREAK "break" << IncrementCount(tcMCCABES_VG); >>
#token CASE "case" << ; >>
#token CATCH "catch" << ; >>
#token KW_CHAR "char" << ; >>
#token CLASS "class" << ; >>
#token KW_CONST "const" << ; >>
#token CONTINUE "continue" << ; >>
#token DEFAULT "default" << ; >>
#token DELETE "delete" << ; >>
#token DO "do" << ; >>
#token KW_DOUBLE "double" << ; >> 
#token ELSE "else" << ; >>
#token ENUM "enum" << ; >>
#token EXTERN "extern" << ; >>
#token KW_FLOAT "float" << ; >>
#token FOR "for" << IncrementCount(tcMCCABES_VG); >>
#token FRIEND "friend" << ; >>
#token GOTO "goto" << ; >>
#token IF "if" << IncrementCount(tcMCCABES_VG); >>
#token INLINE "inline" << ; >>
#token KW_INT "int" << ; >>
#token KW_LONG "long" << ; >>
#token NEW "new" << ; >>
#token OPERATOR "operator" << ; >>
#token PRIVATE "private" << ; >>
#token PROTECTED "protected" << ; >>
#token PUBLIC "public" << ; >>
#token REGISTER "register" << ; >>
#token RETURN "return" << IncrementCount(tcMCCABES_VG); >>
#token KW_SHORT "short" << ; >>
#token SIGNED "signed" << ; >>
#token SIZEOF "sizeof" << ; >>
#token STATIC "static" << ; >>
#token STRUCT "struct" << ; >>
#token SWITCH "switch" << IncrementCount(tcMCCABES_VG); >>
#token TEMPLATE "template" << ; >>
#token KW_THIS "this" << ; >>
#token THROW "throw" << ; >>
#token TRY "try" << ; >>
#token TYPEDEF "typedef" << ; >>
#token UNION "union" << ; >>
#token UNSIGNED "unsigned" << ; >>
#token VIRTUAL "virtual" << ; >>
#token KW_VOID "void" << ; >>
#token VOLATILE "volatile" << ; >>
#token WHILE "while" << IncrementCount(tcMCCABES_VG); >>

/* operators */

#token "==" << ; >>
#token "!=" << ; >>
#tokclass EQUAL_OP { "==" "!=" }

#token ASSIGN_OP "=" << ; >>

#token "\*=" << ; >> 
#token "\/=" << ; >> 
#token "%=" << ; >>
#token "\+=" << ; >>
#token "\-=" << ; >>
#token "\>\>=" << ; >>
#token "\<\<=" << ; >>
#token "&=" << ; >>
#token "\^=" << ; >>
#token "\|=" << ; >>
#tokclass OP_ASSIGN_OP 
	{ "\*=" "\/=" "%=" "\+=" "\-=" "\>\>=" "\<\<=" "&=" "\^=" "\|="	}
 
#token "\>\>" << ; >>
#token "\<\<" << ; >>
#tokclass SHIFT_OP { "\>\>" "\<\<" } 

#token GREATERTHAN "\>"  << ; >>
#token LESSTHAN "\<"  << ; >>
#token GREATEREQUAL "\>="  << ; >>
#token LESSEQUAL "\<="  << ; >>
#tokclass REL_OP { "\>" "\<" "\>=" "\<=" }
 
#token ASTERISK "\*" << ; >>

#token "\/" << ; >>
#token "%"  << ; >>
#tokclass DIV_OP { "\/" "%" } 

#token "\.\*" << ; >>
#token "\-\>\*" << ; >>
#tokclass PM_OP { "\.\*" "\->\*" } 

#token "\+\+" << ; >>
#token "\-\-" << ; >>
#tokclass INCR_OP { "\+\+" "\-\-" } 

#token "\+" << ; >>
#token "\-" << ; >>
#tokclass ADD_OP { "\+" "\-" } 

#token LOGICAL_AND_OP "&&" << IncrementCount(tcMCCABES_VG); >>
#token LOGICAL_OR_OP "\|\|" << IncrementCount(tcMCCABES_VG); >>
#token LOGICAL_NOT_OP "!" << ; >>
#token QUERY_OP "?" << IncrementCount(tcMCCABES_VG); >>

#token AMPERSAND "&" << ; >>
#token PIPE "\|" << ; >>
#token TILDA "\~" << ; >>
#tokclass BITWISE_OP { "&" "\|" "\~" } 

#token COLONCOLON "::" << ; >>
#token ARROW "\-\>" << ; >>
#token COLON ":" << ; >>
#token PERIOD "\." << ; >>
#token COMMA "," << ; >>
#token SEMICOLON ";" << ; >>

/*
** draft ANSI C++ keywords 
**
** this grammar is intended to look forward to ANSI C++, so we at least
** try to lex up the new keywords (taken from the revised appendix to the
** ARM, which claims to cover decisions up to February 1995)
**
** namespaces will be treated as a kind of module, along with class and struct
**
** most of the other keywords will make little difference except
** that they will not be recognized as identifiers
** in general, anyone who is using these keywords as identifiers today, 
** probably shouldn't be
** 
** most of the rest of the new keywords occur in contexts where we aren't
** looking too hard (e.g. in procedural code rather than declaration or
** definition signatures)
*/
#token NAMESPACE "namespace" << ; >>
#token USING "using" << ; >>
#token AND "and" << ; >>
#token AND_EQ "and_eq" << ; >>
#token BITAND "bitand" << ; >>
#token BITOR "bitor" << ; >>
#token COMPL "compl" << ; >>
#token NOT "not" << ; >>
#token OR "or" << ; >>
#token OR_EQ "or_eq" << ; >>
#token XOR "xor" << ; >>
#token XOR_EQ "xor_eq" << ; >>
#token KW_BOOL "bool" << ; >>
#token BTRUE "true" << ; >>
#token BFALSE "false" << ; >>
#token STATIC_CAST "static_cast" << ; >>
#token REINTERPRET_CAST "reinterpret_cast" << ; >>
#token CONST_CAST "const_cast" << ; >>
#token DYNAMIC_CAST "dynamic_cast" << ; >>
#token TYPEID "typeid" << ; >>

// The next definition reserves a member of the token type enumeration for
// later use.  We do not expect to match the associated regular expression.
#token IMPLEMENTATION_KEYWORD "XXXXXXXXXXXXXXXXXXXXXXXXXX" << ; >>

 
/* identifiers */
#token IDENTIFIER "[A-Za-z_][A-Za-z_0-9]*" 
<<
	// Check whether there are any dialect-specific rules 
	// about the current token.
	std::string treatment = 
		CCCC_Options::dialectKeywordPolicy(parse_language,lextext());

	std::string toktext=lextext();
        if( treatment == "ignore" )
	{
	    skip();
	}
	// Ultimately, the next two cases will need to be handled 
	// using a #lexclass or something similar, for the moment
	// we just try to skip the tokens themselves.
	else if ( treatment == "start_skipping" )
	{
	    skip();
	}	
	else if ( treatment == "stop_skipping" ) 
	{
	    skip();
	}
>>

	
/* Literal elements - integer and float numbers and strings */

#lexclass START
#token OCT_NUM "[0][0-7]*"
#token L_OCT_NUM "[0][0-7]*[Ll]"
#token INT_NUM "[1-9][0-9]*"
#token L_INT_NUM "[1-9][0-9]*[Ll]"
#token HEX_NUM "[0][Xx][0-9A-Fa-f]+"
#token L_HEX_NUM "[0][Xx][0-9A-Fa-f]+[Ll]"
//#token FNUM "([1-9][0-9]*{.[0-9]*} | {[0]}.[0-9]+ | 0. ) {[Ee]{[\+\-]}[0-9]+}"
#token FNUM "([0-9]+.[0-9]+ | [0-9]+. | .[0-9]+ ) {[Ee]{[\+\-]}[0-9]+}"
#token ANYTHING "~[\{\}\[\]\(\)@]" << skip(); >>

// it is not obvious from the PCCTS documentation that it a token class
// may contain things already declared as tokens, but it does not appear
// to cause problems
#tokclass RESYNCHRONISATION { "\}" ";" } 

class CParser {

<<

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

>>


/* 
** the start symbol for the grammar:
*/

start : << string fileScope; >>
          end_of_file
        | link_item[fileScope] start
	; 

link_item[string& scope] : 
          (EXTERN STRINGCONST LBRACE)? 
	  extern_linkage_block
	| namespace_block
        | using_statement
	| linkage_qualifiers definition_or_declaration[scope]
	;

end_of_file : eof:Eof
<<
  ps->record_other_extent(1,$eof->getLine(),"<file scope items>");
>>
        ;


definition_or_declaration[string& scope] : 
	<<
	  // get ready in case we need to resynchronize...
          ANTLRTokenPtr initial_token=LT(1);
          int startLine=LT(1)->getLine();
	  string initial_text=pu->lookahead_text(3);
	>>
	  typedef_definition
	| ( explicit_template_instantiation )?
	  | ( scoped_member_name SEMICOLON )? // e.g. 'friend CCCC_Project;'
	| ( scoped_member_name LPAREN )? 
	  method_declaration_or_definition_with_implicit_type[scope]
	| (type[d1,d2,d3] scoped_member_name LPAREN )? 
	  method_declaration_or_definition_with_explicit_type[scope]
	| (type[d1,d2,d3] scoped_member_name )?
	  instance_declaration[scope]
	| (type[d1,d2,d3] LPAREN ASTERISK scoped_member_name RPAREN)?
	  instance_declaration[scope]
	| class_declaration_or_definition[scope]
	| union_definition
	| enum_definition
	  // suggested by Kenneth H. Cox to deal with coders who put
	  // a semicolon after the '}' at the end of an inline method
	  // definition
	| SEMICOLON             
	;
<<
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
>>
    
// the following rule is included to cause generation of a token class
// which will be passed to the resynchronise() function
// it is not intended to be matched within the grammar
resync_tokens : 
	  RBRACE
	| SEMICOLON
	;


extern_linkage_block : << string dummy; >>
	  EXTERN STRINGCONST LBRACE (link_item[dummy])* RBRACE
	;

namespace_block : << string dummy; >>
	  NAMESPACE { IDENTIFIER } LBRACE (link_item[dummy])* RBRACE SEMICOLON
	;

using_statement :
	  USING { NAMESPACE } scoped_member_name SEMICOLON
	;

explicit_template_instantiation :
          scoped_member_name angle_block SEMICOLON
        ;

class_declaration_or_definition[string& scope] : 
<< 
    int startLine=LT(1)->getLine(); 
    bool is_definition; 
    string modname,modtype;
>> 
	  class_prefix[modname,modtype] sfx:class_suffix[is_definition,modname]
<<
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
>>
	;

class_suffix[bool& is_definition,string& scope] :
          SEMICOLON
<< is_definition=false; >> 
        | { inheritance_list[scope] } class_block[scope] class_suffix_trailer 
<< is_definition=true; >>  
        ;

// version 3.pre42 
// Attempting to add support for C-style anonymous struct declarations,
// where there is no class name between the keyword (usually 'struct') 
// and the start of the definition block, but there are a list of instances
// of the anonymous class declared between the end of the definition block
// and the semicolon.
class_suffix_trailer :
	   opt_instance_list SEMICOLON
	 ;

opt_instance_list :
		  IDENTIFIER ( COMMA IDENTIFIER )*
		| /* empty */
		;

union_definition : 
	  (anonymous_union_definition)?
	| named_union_definition
	;

anonymous_union_definition :
	  UNION brace_block opt_instance_list SEMICOLON
	;

named_union_definition :	
<< int startLine=LT(1)->getLine(); >>
	    UNION id:IDENTIFIER brace_block SEMICOLON
<<
    ps->record_module_extent(startLine,startLine,
			     $id->getText(),"union",
			     "definition",utDEFINITION);
>>
	;


enum_definition : 
	 ( anonymous_enum_definition )?
	 | named_enum_definition
	 ;

anonymous_enum_definition :
	    ENUM brace_block opt_instance_list SEMICOLON
	  ;

named_enum_definition :
<<int startLine=LT(1)->getLine(); >>
	  ENUM id:IDENTIFIER brace_block SEMICOLON
	<<
		ps->record_module_extent(startLine,startLine,
					$id->getText(),"enum",
					 "definition",utDEFINITION);
	>>
	;

/*
** instance_declaration embraces declarations which initialise the variable
** as the same declaration may initialise some variables but not others,
** so we do not have a separate instance_definition rule
*/
instance_declaration[string& scopeName] : 
<< int startLine=LT(1)->getLine(); string cvQuals,typeName,varName,indir; >>
	  (cv_qualifier[cvQuals])* { STATIC }  type_name[typeName]
	instance_item[indir,varName] ( COMMA instance_item[d1,d2] )* SEMICOLON
<<
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
>> 
;

class_block [string& scope]:
	  << 
	      int saved_visibility=ps->get_flag(psfVISIBILITY);
	  >>
	  LBRACE class_block_item_list[scope] RBRACE
	  << ps->set_flag(psfVISIBILITY,saved_visibility); >>
	;

class_block_item_list[string& scope] :
	  (class_block_item[scope])? class_block_item[scope] class_block_item_list[scope]
	| /* empty */
	;

class_block_item[string& scope] : 
	  ( access_modifier )?
	| class_item_qualifier_list definition_or_declaration[scope]
	;

class_item_qualifier_list! :
      (class_item_qualifier)? class_item_qualifier class_item_qualifier_list
    | /* empty */
	;

class_item_qualifier :
      FRIEND 
    | VIRTUAL << ps->set_flag(psfVIRTUAL,abTRUE); >> 
    | STATIC << ps->set_flag(psfSTATIC,abTRUE); >>
    | INLINE
	;	
    
access_modifier! :
	  access_key COLON
	;

method_declaration_or_definition_with_implicit_type[string& implicitScope] : 
<< 
  int startLine=LT(1)->getLine(); bool is_definition; 
  string returnType,scope=implicitScope,methodName, paramList;
>>
	  method_signature[scope,methodName,paramList] 
          method_suffix[is_definition]
	<<
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
	>>
	;

method_declaration_or_definition_with_explicit_type[string &scope] : 
<< string cvQualifiers,typeName,indirMods; >> 
type[cvQualifiers,typeName,indirMods]
method_declaration_or_definition_with_implicit_type[scope] 
;

// It has become increasingly obvious that one of the major
// reasons for people in the field to use CCCC is to assess
// legacy code, which is often not pure ANSI C.
// One common idiom which has been reported is hybrid
// Kernighan&Ritche/ANSI C with the preprocessor used
// to offer either compiler sight of their appropriate 
// kind of signature.  CCCC runs on code which has not
// been preprocessed and ignores the preprocessor 
// directives, so this idiom looks to it as if the
// function has two signatures, one after another,
// followed by the function's implementation.
// Attempts have been made to support this idiom but
// they are disabled at present.

// Once we have seen the signature of a function, there are a
// number of ways the item can end.
method_suffix[bool& is_definition] :
           // a simple declaration
           SEMICOLON << is_definition=false; >> 
           // declaration of a pure virtual function
           // NB the value zero may look like a normal integer, but to the
           // lexer, it is seen as an octal literal 
         | ASSIGN_OP OCT_NUM SEMICOLON << is_definition=false; >>
           // definition of a constructor with an initializer list
	 | ctor_init_list brace_block << is_definition=true; >>
           // definition of any other kind of method 
         | brace_block << is_definition=true; >>
	 ;

method_signature[string& scope, string& methodName, string& paramList] : 
<< 
    int startLine=LT(1)->getLine(); 
>>
         scoped_identifier[scope,methodName] param_list[scope,paramList] opt_const_modifier 
	 << 
	 >>
	 ;

type[string& cvQualifiers, string& typeName, string& indirMods] :
           (cv_qualifier[cvQualifiers])* { STATIC }
           type_name[typeName] 
           indirection_modifiers[indirMods]
         ;

cv_qualifier[string& cvQualifiers] : 
<< string nextTokenText=LT(1)->getText(); >>
           (
	     KW_CONST 
           | MUTABLE
           | VOLATILE
           | REGISTER
	   )
<< cvQualifiers=typeCombine(cvQualifiers,nextTokenText,""); >>
	   ;

type_name[string& typeName] :
             builtin_type[typeName]
           | user_type[typeName]
           ;
 
indirection_modifiers[string& indirMods] :
             (indirection_modifier[indirMods])? 
             indirection_modifier[indirMods] indirection_modifiers[indirMods] 
           | /* empty */
           ;

indirection_modifier[string& indirMods] :
             (KW_CONST ASTERISK)? << indirMods+="const*"; >>
           | ASTERISK       << indirMods+="*"; >>
           | AMPERSAND      << indirMods+="&"; >>
;

builtin_type[string& typeName] :
         (type_keyword[typeName])+
       ;

type_keyword[string& typeName] : 
<< string tokenText=LT(1)->getText(); >>
        (  
	  KW_VOID
	| KW_BOOL
	| KW_CHAR | KW_INT 
	| KW_FLOAT | KW_DOUBLE 
	| KW_SHORT | KW_LONG | UNSIGNED | SIGNED 
	)
	<< 
	// We only really care about the type name so that we 
	// can count relationships between classes, so what we store
	// here is a bit arbitrary.  We choose to represent the type
	// of composed builtin types such as 'unsigned char' using
	// only the last keyword (i.e. 'char' in this case), so that 
	// we don't have to have an enormous supression list of different
	// variants like 'long long unsigned int'.
	typeName=tokenText; 
	>>
	;

user_type[string& typeName] : << string scope,name; >>
        { class_key[d1] } scoped_identifier[scope,name]
        << typeName=pu->scopeCombine(scope,name); >>
        ;

scoped_member_name: << string dummy1, dummy2; >>
           scoped_identifier[dummy1,dummy2]
	 ; 

scoped_identifier[string& scope, string& name] :
	   ( explicit_scope_spec[scope] )?
	   explicit_scope_spec[scope] scoped_identifier[scope,name] 
	 | unscoped_member_name[name]
	 ;

explicit_scope_spec[string& scope] :
	   cl:IDENTIFIER {angle_block} COLONCOLON
<<
   scope=cl->getText();
   ps->set_flag(vDONTKNOW);
>>
	 ;

unscoped_member_name[string& name] :
	 ( 
	   (id1:IDENTIFIER angle_block)? << name=$id1->getText(); >>
	 | id2:IDENTIFIER << name=$id2->getText(); >>
	 | dtor_member_name[name] 
	 | operator_member_name[name] 
	 )
	 ;

 dtor_member_name[string& name] :
	   TILDA id:IDENTIFIER
         << 
            name="~";
            name+=$id->getText();
         >>

	 ;

operator_member_name[string& name] : << string operatorIdentifier; >>
           OPERATOR operator_identifier[operatorIdentifier]
	   << 
		name+="operator ";
		name+=operatorIdentifier; 
	   >>
	 ;

operator_identifier[string& opname] : 
<< 
	string cv,name,indir;  
	opname=LT(1)->getText();
>>
	   (op)?
         | (new_or_delete LBRACK RBRACK)? << opname+="[]"; >>  
	 | new_or_delete
	 | type[cv,name,indir] << opname=name+indir; >>
	 | LPAREN RPAREN << opname="()"; >>
	 | LBRACK RBRACK << opname="[]"; >>
	 ;

new_or_delete :
	   NEW
	 | DELETE
	 ;

param_list[string& scope, string& params] :  
	 << 
           int startLine=LT(1)->getLine();
           string param_items;
	 >>
	   ( LPAREN param_list_items[scope,param_items] RPAREN )?
	 <<
	    params="(";
            params+=param_items;
            params+=")";
	 >>
	 | paren_block 
	 <<
	    params="(...)";
	 >>
	 ;

param_list_items [string& scope, string& items] :
           /* empty */
         | param_item[scope,items] more_param_items[scope,items]
         ;

more_param_items[string& scope, string& items] : 
<< string next_item, further_items; >>
           /* empty */
         | COMMA param_item[scope,next_item] more_param_items[scope,further_items]
	 <<
	     items+=",";
             items+=next_item;
             items+=further_items;
         >>
         ;

param_item[string& scope, string& item] : 
	   param_type[scope,item] param_spec
	 ;

param_type[string& scope, string& typeName] : 
<< 
    string cvmods, name, indir;  
    int startLine=LT(1)->getLine();
>>
	   type[cvmods,name,indir] 
	 <<
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
	 >>
	 ;

 param_spec :
	   { IDENTIFIER } { "=" literal }
	 ;

knr_param_decl_list :
          /* empty */
        | instance_declaration[d1] knr_param_decl_list
	;
<<
        cerr << "failed knr_param_decl_list for token " 
             << static_cast<int>(LT(1)->getType()) << ' ' 
             << LT(1)->getText() << endl;
>>

opt_const_modifier :
          /* empty */
	  <<
		ps->set_flag(psfCONST,abFALSE);
          >>
        | KW_CONST 
	  <<
		ps->set_flag(psfCONST,abTRUE);
	  >>
	;
<<
	// fail action for opt_const_modifier
	// I can't see how we can fail this, but we seem to manage
	cerr << "failed opt_const_modifier for token " 
  	     << static_cast<int>(LT(1)->getType()) << ' ' 
	     << LT(1)->getText() << endl;
>>

typedef_definition : << string dummy; >>
	   ( fptr_typedef_definition )?
	|  ( TYPEDEF class_key[dummy])? struct_typedef_definition 
	|  simple_typedef_definition
	;

fptr_typedef_definition :  
	   TYPEDEF type[d1,d2,d3] fptr_type_alias SEMICOLON 
	;

// This rule added in response to a report sent by Tennis Smith of
// Cisco.
// It supports the old C-style idiom where a struct is defined within
// a typedef (either with or without a name).
// There's a lot of this code out there...  
struct_typedef_definition : << string dummy; >>
	   TYPEDEF class_key[dummy] identifier_opt brace_block tag_list_opt SEMICOLON
	;

simple_typedef_definition :  
	  TYPEDEF type[d1,d2,d3] simple_type_alias SEMICOLON
	;

identifier_opt :
	  IDENTIFIER
	| /* empty */
	;

tag_list_opt :
	  tag ( COMMA tag )*
	| /* empty */
	;

tag :
	    ( ASTERISK )* IDENTIFIER
	  ;

simple_type_alias :
	  id:IDENTIFIER
	;

fptr_type_alias :
	  LPAREN ASTERISK scoped_identifier[d1,d2] RPAREN paren_block 
	;

class_or_method_declaration_or_definition[string& scope] : << string dummy; >>
	  (class_key[dummy])? class_declaration_or_definition[scope] 
	| (scoped_member_name LPAREN)?
	  method_declaration_or_definition_with_implicit_type[scope]
	| method_declaration_or_definition_with_explicit_type[scope] 
	;

class_prefix[string& modname, string& modtype] :
    class_key[modtype] scoped_identifier[d1,modname] { angle_block }
	// or perhaps an anonymous type...
	| class_key[modtype]
	;

inheritance_list [string& childName] :
	  << ps->set_flag(vPRIVATE); >>
	  COLON inheritance_item_list[childName] 
	;

inheritance_item_list[string& childName] :
	  inheritance_item[childName] ( COMMA inheritance_item[childName])*
	;

inheritance_access_key :
      VIRTUAL { access_key }
    | access_key { VIRTUAL }
    | /* empty */
    ;

inheritance_item[string& childName]  : 
<< 
    string parent_scope,parent_name; 
    int startLine=LT(1)->getLine(); 
>>
	  inheritance_access_key  type_name[parent_name]
	<<
	        int endLine=LT(1)->getLine();
		ps->record_userel_extent(startLine,endLine,
					 childName,"",parent_name,
					 "inheritance",
					 ps->get_visibility(),
					 utINHERITS);
	>>
	;
	
class_key[string& modtype] : << modtype=LT(1)->getText(); >>
	    CLASS << ps->set_flag(vPRIVATE); >>
	  | STRUCT << ps->set_flag(vPUBLIC); >>
	;

access_key :
	  PUBLIC! 
	  << 
		ps->set_flag(vPUBLIC); 
	  >>
        | PRIVATE! 
	  << 
		ps->set_flag(vPRIVATE); 
	  >>
        | PROTECTED! 
	  << 
		ps->set_flag(vPROTECTED); 
	  >>
	;

ctor_init_list : 
	  COLON ctor_init_item_list
	;

ctor_init_item_list :
	  ( ctor_init_item COMMA )? ctor_init_item COMMA ctor_init_item_list
	| ctor_init_item
	;

ctor_init_item :
	  instance_item[d1,d2]
	;

linkage_qualifiers:
      (linkage_qualifier)? linkage_qualifier linkage_qualifiers
	| /* empty */
	;

linkage_qualifier :
      STATIC            << ps->set_flag(psfSTATIC,abTRUE); >>
	| ( EXTERN STRINGCONST )?
    | EXTERN            
	| INLINE
	| TEMPLATE { angle_block }
	;

identifier_or_brace_block_or_both :
	  IDENTIFIER opt_brace_block 
	;

opt_brace_block :
	 ( brace_block )?
	| /* empty */
	;

instance_item[string& indir,string& name] :
	  item_specifier[indir,name] brack_list opt_initializer
	;

item_specifier[string& indir,string& name] : 
          LPAREN ASTERISK scoped_member_name RPAREN paren_block
        | (indirection_modifier[indir])* scoped_identifier[d1,name]
	;

opt_initializer: << string dummy; >>
	  "=" init_expr
	| ( LPAREN RPAREN )?
	| LPAREN init_expr RPAREN
	| /* empty */
	;

init_expr :
	  ( init_expr_item op )? init_expr_item op init_expr 
	|  init_expr_item
	;

init_expr_item :
	  SIZEOF paren_block
	| paren_block
	| brace_block
	| (IDENTIFIER paren_block)?
	| cast_keyword angle_block paren_block
	| constant
	;

cast_keyword :
	  STATIC_CAST | CONST_CAST | REINTERPRET_CAST
	;

init_value:
	  constant
	| brace_block
	| paren_block
	;

keyword :
	  ASM | AUTO | KW_BOOL | BREAK | CASE | CATCH | KW_CHAR | CLASS | KW_CONST 
	| CONTINUE | DEFAULT | DELETE | KW_DOUBLE | DO | DYNAMIC_CAST | ELSE 
        | ENUM | EXTERN | EXPLICIT
	| BFALSE | KW_FLOAT | FOR | FRIEND | GOTO | IF | INLINE | KW_INT | KW_LONG | NEW 
	| OPERATOR | PRIVATE | PROTECTED | PUBLIC | REGISTER 
        | REINTERPRET_CAST | RETURN 
	| KW_SHORT | SIGNED | SIZEOF | STATIC | STATIC_CAST | STRUCT 
        | SWITCH | TEMPLATE 
	| KW_THIS | THROW | BTRUE | TRY | TYPEDEF | UNION | UNSIGNED | VIRTUAL 
	| KW_VOID | VOLATILE | WHILE 
	;

op :
	  EQUAL_OP | ASSIGN_OP | OP_ASSIGN_OP | SHIFT_OP | REL_OP 
	| ASTERISK | DIV_OP | PM_OP | INCR_OP | ADD_OP | QUERY_OP
	| LOGICAL_AND_OP | LOGICAL_OR_OP | LOGICAL_NOT_OP | BITWISE_OP 
	| COLONCOLON | COLON | PERIOD | ARROW | COMMA 
	;

constant :
	  literal
	| IDENTIFIER
	;

literal :
	  string_literal 
	| CHARCONST | FNUM
	| OCT_NUM | L_OCT_NUM | HEX_NUM | L_HEX_NUM | INT_NUM | L_INT_NUM
	| BTRUE | BFALSE
	| ADD_OP literal
	;

string_literal :
	  (STRINGCONST STRINGCONST)? STRINGCONST string_literal
	| STRINGCONST
	;

block :
	  brace_block
	| brack_block
	| paren_block
	;

balanced :
	  scoped 
	| block
	| SEMICOLON
	;

balanced_list :
	  ( balanced )? balanced balanced_list
	| /* empty */
	;

nested_token_list [ int nl ] :
	  nested_token[nl] nested_token_list[nl]
	| /* empty */
	;

nested_token [ int nl ] : << ANTLRTokenPtr la_ptr=LT(1); >>
	<< (la_ptr!=0) && (mytoken(la_ptr)->getNestingLevel() > nl) >>?
	  tok:.
	;


scoped :
	  ( keyword )?
	| (op)?
	| IDENTIFIER
	| literal
	;

brace_block : << int brace_level=MY_TOK(LT(1))->getNestingLevel(); >> 
	  LBRACE skip_until_matching_rbrace[brace_level]
	;

skip_until_matching_rbrace [ int brace_level ] :
<< 
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
>>
	RBRACE
	; 

paren_block :
	  LPAREN balanced_list! RPAREN 
	;

brack_block :
	  LBRACK balanced_list! RBRACK 
	;

brack_list :
          ( brack_block )? brack_block brack_list
        | /* empty */
        ;

angle_balanced_list :
          << LT(1)->getType() == LESSTHAN >>? angle_block angle_balanced_list
        | << LT(1)->getType() == GREATERTHAN >>? 
          /* empty, the token will be matched in parent rule angle_block */
        | ~GREATERTHAN angle_balanced_list /* consume one token & recurse */
	;

angle_block : 
	  LESSTHAN! angle_balanced_list! GREATERTHAN!
	;
}


#token TOKENTYPE_MAX

// -*-c++-*- (Emacs mode request)



