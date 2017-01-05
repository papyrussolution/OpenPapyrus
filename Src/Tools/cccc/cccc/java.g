// java.g for CCCC v 3.X
// by Tim Littlefair, November 1999
// based on the java.g for ANTLR2 from www.antlr.org
// credits from that version follow:

/** Java 1.1 Recognizer Grammar
 *
 * Contributing authors:
 *		John Mitchell		johnm@non.net
 *		Terence Parr		parrt@magelang.com
 *		John Lilley			jlilley@empathy.com
 *		Scott Stanchfield	thetick@magelang.com
 *		Markus Mohnen       mohnen@informatik.rwth-aachen.de
 *		Peter Williams		pwilliams@netdynamics.com
 *
 * Version 1.00 December 9, 1997 -- initial release
 * Version 1.01 December 10, 1997
 *		fixed bug in octal def (0..7 not 0..8)
 * Version 1.10 August 1998 (parrt)
 *		added tree construction
 *		fixed definition of WS,comments for mac,pc,unix newlines
 *		added unary plus
 * Version 1.11 (Nov 20, 1998)
 *		Added "shutup" option to turn off last ambig warning.
 *		Fixed inner class def to allow named class defs as statements
 *		synchronized requires compound not simple statement
 *		add [] after builtInType DOT class in primaryExpression
 *		"const" is reserved but not valid..removed from modifiers
 *
 * Version 1.12 (Feb 2, 1999)
 *		Changed LITERAL_xxx to xxx in tree grammar.
 *		Updated java.g to use tokens {...} now for 2.6.0 (new feature).
 *
 * Version 1.13 (Apr 23, 1999)
 *		Didn't have (stat)? for else clause in tree parser.
 *		Didn't gen ASTs for interface extends.  Updated tree parser too.
 *		Updated to 2.6.0.
 * Version 1.14 (Jun 20, 1999)
 *		Allowed final/abstract on local classes.
 *		Removed local interfaces from methods
 *		Put instanceof precedence where it belongs...in relationalExpr
 *			It also had expr not type as arg; fixed it.
 *		Missing ! on SEMI in classBlock
 *		fixed: (expr) + "string" was parsed incorrectly (+ as unary plus).
 *		fixed: didn't like Object[].class in parser or tree parser
 *
 * This grammar is in the PUBLIC DOMAIN
 * 
 * BUGS
 *
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
>>


#lexaction
<<
#include <cassert>

// Need to see definition of JLexer class before defining the 
// next actions.
// This will mean this file is included twice in JLexer.cpp.  This
// should do no harm...
#include "JLexer.h"

// The lexical analyser passes some information through to
// the parse store to be integrated with parse information before

inline void IncrementCount(LexicalCount tc) 
{
  assert(ParseStore::currentInstance()!=NULL);
  ParseStore::currentInstance()->IncrementCount(tc);
}

inline void endOfLine(JLexer &lexer)
{
  assert(ParseStore::currentInstance()!=NULL);
  ParseStore::currentInstance()->endOfLine(lexer.line());
  lexer.newline();
  lexer.skip();
}

inline void endOfCommentLine(JLexer &lexer)
{
  IncrementCount(tcCOMLINES); 
  endOfLine(lexer);
}

>>

#token Eof "@" << replstr("<EOF>"); >>

// OPERATORS
#token QUESTION		"?"		<< IncrementCount(tcMCCABES_VG);>>
#token LPAREN		"\("	<<;>>
#token RPAREN		"\)"	<<;>>
#token LBRACK		"\["	<<;>>
#token RBRACK		"\]"	<<;>>
#token LCURLY		"\{"	<<;>>
#token RCURLY		"\}"	<<;>>
#token COLON		":"	<<;>>
#token COMMA		","	<<;>>
#token DOT		"\."	<<;>>
#token ASSIGN		"="	<<;>>
#token EQUAL		"=="	<<;>>
#token LNOT		"!"	<<;>>
#token BNOT		"\~"	<<;>>
#token NOT_EQUAL	"!="	<<;>>
#token DIV		"/"	<<;>>
#token DIV_ASSIGN	"/="	<<;>>
#token PLUS		"\+"	<<;>>
#token PLUS_ASSIGN	"\+="	<<;>>
#token INC		"\+\+"	<<;>>
#token MINUS		"\-"	<<;>>
#token MINUS_ASSIGN	"\-="	<<;>>
#token DEC		"\-\-"	<<;>>
#token STAR		"\*"	<<;>>
#token STAR_ASSIGN	"\*="	<<;>>
#token MOD		"%"	<<;>>
#token MOD_ASSIGN	"%="	<<;>>
#token SR		"\>\>"	<<;>>
#token SR_ASSIGN	"\>\>="	<<;>>
#token BSR		"\>\>\>"	<<;>>
#token BSR_ASSIGN	"\>\>\>="	<<;>>
#token GE		"\>="	<<;>>
#token GT		"\>"	<<;>>
#token SL		"\<\<"	<<;>>
#token SL_ASSIGN	"\<\<="	<<;>>
#token LE		"\<="	<<;>>
#token LESSTHAN		"\<"	<<;>> // LT is reserved for PCCTS support code use

// Boolean operators
// I'm not sure whether the Java VM definition 
// mandates short circuit evaluation or not.  I assume
// it does for logical or and logical and, which means 
// that expressions involving these operators are choice points
// in terms of what code gets executed, hence MVG increases.  
// In theory, bitwise operators could be shortcircuited if
// and only if the first operand examined forces the result
// (i.e. 1 | expression or 0 & expression).  Again, I'm
// not sure what the JVM definition says, but I am assuming
// that this does not represent a switch of flow of control,
// hence no increment for MVG.   
// Bitwise exclusive or always requires both operands to be 
// evaluated, hence it does not increment MVG.
// Could someone please email me with chapter and verse if
// I am wrong in my assumptions here. 
// tim_littlefair@hotmail.com
#token BXOR		"\^"	<<;>>
#token BXOR_ASSIGN	"\^="	<<;>>
#token BOR		"\|"	<< ; >>
#token BOR_ASSIGN	"\|="	<< ; >>
#token LOR		"\|\|"	<< IncrementCount(tcMCCABES_VG); >>
#token BAND		"&"	<< ; >>
#token BAND_ASSIGN	"&="	<<;>>
#token LAND		"&&"	<< IncrementCount(tcMCCABES_VG);>>
#token SEMI		";"	<<;>>

 
// Whitespace -- ignored
#token WS		"[\ \t]" << skip(); >>

// handle newlines
#token DOS_NL		"\r\n"  << endOfLine(*this); >>
#token MAC_NL		"\r"	<< endOfLine(*this); >>
#token UNIX_NL		"\n"    << endOfLine(*this); >>


// The handling of single and multiline comments and string 
// and character literals below is copied from my C++ grammar
// and may well be less accurate than the treatment in 
// TJP's base Java grammar.
#token COMLINE "//" << mode(COMMENT_LINE); skip(); >>
#lexclass COMMENT_LINE
#token DOS_COMLINE_END  "\r\n" << endOfCommentLine(*this); mode(START); >>
#token MAC_COMLINE_END  "\r" << endOfCommentLine(*this); mode(START); >>
#token UNIX_COMLINE_END  "\n" << endOfCommentLine(*this); mode(START); >>
#token COMLINE_ANYTHING "~[\n]" << skip(); >>
#lexclass START

#token COMMULTI "/\*" << mode(COMMENT_MULTI); skip(); >>
#lexclass COMMENT_MULTI
#token COMMULTI_END "\*/" << 
  IncrementCount(tcCOMLINES); 
  mode(START); 
  skip(); 
>>
#token DOS_COMMULTI_EOL "\r\n" << 
	endOfCommentLine(*this);
>>
#token MAC_COMMULTI_EOL "\r" << 
	endOfCommentLine(*this);
>>
#token UNIX_COMMULTI_EOL "\n" << 
	endOfCommentLine(*this);
>>

#token COMMULTI_ANYTHING "~[\n]" << skip(); >>
#lexclass START

#token STRINGSTART "\"" << mode(CONST_STRING); skip(); >>
#lexclass CONST_STRING
#token STRINGCONST "\"" << mode(START); >>
// thanks to Lynn Wilson for pointing out the need for a simple fix 
// to handle escaped newlines within string constants
#token LYNNS_FIX "\\\n"  <<  endOfLine(*this); >> 
// We also need to handle escaped double quotes
#token ESCAPED_DQUOTE "\\\"" << skip(); >>
#token ESCAPED_OTHER "\\~[\"]" << skip(); >>
#token S_ANYTHING "~[\"]" << skip(); >>
#lexclass START

#token CHARSTART "\'" << mode(CONST_CHAR); skip(); >>
#lexclass CONST_CHAR
#token CHARCONST "'" << replstr("'.'"); mode(START); >>
#token CH_ANYTHING "~[']" << skip(); >>
#lexclass START

// keywords
#token	ABSTRACT	"abstract"		<<;>>
#token	KW_BOOLEAN		"boolean"		<<;>>
#token	BREAK		"break"		<< IncrementCount(tcMCCABES_VG);>>
#token	KW_BYTE		"byte"		<<;>>
#token	CATCH		"catch"		<< /* IncrementCount(tcMCCABES_VG) ? */ ;>>
#token  DEFAULT         "default"       <<;>>
#token	KW_CHAR		"char"		<<;>>
#token	CLASS		"class"		<<;>>
#token	KW_CONST		"const"		<<;>>
#token	CONTINUE	"continue"		<< IncrementCount(tcMCCABES_VG); >>
#token	DO			"do"		<<;>>
#token	KW_DOUBLE		"double"		<<;>>
#token	ELSE		"else"		<<;>>
#token	EXTENDS		"extends"		<<;>>
#token	BFALSE		"false"		<<;>>
#token	FINAL		"final"		<<;>>
#token	FINALLY		"finally"		<<;>>
#token	KW_FLOAT		"float"		<<;>>
#token	FOR			"for"		<< IncrementCount(tcMCCABES_VG); >>
#token	IF			"if"		<< IncrementCount(tcMCCABES_VG); >>
#token	IMPLEMENTS	"implements"		<<;>>
#token	IMPORT		"import"		<<;>>
#token	INSTANCEOF	"instanceof"		<<;>>
#token	KW_INT			"int"		<<;>>
#token	INTERFACE	"interface"		<<;>>
#token	KW_LONG		"long"		<<;>>
#token	NATIVE		"native"		<<;>>
#token	NEW			"new"		<<;>>
#token	PNULL		"null"		<<;>>
#token	PACKAGE		"package"		<<;>>
#token	PRIVATE		"private"		<<;>>
#token	PROTECTED	"protected"		<<;>>
#token	PUBLIC		"public"		<<;>>
#token	RETURN		"return"		<<;>>
#token	KW_SHORT		"short"		<<;>>
#token	SHUTUP		"shutup"		<<;>>
#token	STATIC		"static"		<<;>>
#token	SUPER		"super"		<<;>>
#token	SWITCH		"switch"		<< IncrementCount(tcMCCABES_VG); >>
#token	SYNCHRONIZED	"synchronized"		<<;>>
#token	THINGS		"things"		<<;>>
#token	KW_THIS		"this"		<<;>>
#token	THREADSAFE	"threadsafe"		<<;>>
#token	THROW		"throw"		<<;>>
#token	THROWS		"throws"		<<;>>
#token	TRANSIENT	"transient"		<<;>>
#token	BTRUE		"true"		<<;>>
#token	TRY			"try"		<<;>>
#token	KW_VOID		"void"		<<;>>
#token	VOLATILE	"volatile"		<<;>>
#token	WHILE		"while"		<< IncrementCount(tcMCCABES_VG); >>
#token  CASE		"case"		<< IncrementCount(tcMCCABES_VG); >>

// an identifier.  

// TJP & co's original grammar for ANTLR 2.0 sets a state in the
// lexer to test for keyword literals defined in grammar rules before
// matching regular expressions defined as named tokens.
// Under PCCTS 1.33MRxx this won't work, we have to move all of the
// literals before the IDENT rule and give them names.
// Personally, I don't mind this.
#token IDENT 	"[a-zA-Z_$]([a-zA-Z_$0-9])*"	<<;>>


// a numeric literal
#token NUM_INT1	"([0-9])*\.([0-9])+{[eE]{[\+\-]}([0-9])+}{[fFdD]}" <<;>>
#token NUM_INT2	"0([0-7])*{[lL]}" <<;>>
#token NUM_INT3 "0[xX]([0-9a-fA-F])*{[lL]}" <<;>>
#token NUM_INT4	"[1-9]([0-9])*{[lL]}" <<;>>

// Option 1 above does not cover a floating point value with an f at
// the end but no decimal point
#token NUM_INT1A "([0-9])+{[eE]{[\+\-]}([0-9])+}{[fFdD]}" <<;>>

// Nor does it cover a floating point value with a decimal point but 
// no following mantissa.
#token NUM_INT1B "([0-9])+\.{[eE]{[\+\-]}([0-9])+}{[fFdD]}" <<;>>


class JParser
{
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

// we have a data member for the language of the parse so
// that we can supply the names of dialects (ansi_c, ansi_c++, mfc_c++ etc)
// for contexts where we wish to apply dialect-specific parsing
// rules
string parse_language;

// Many of the rules below accept string parameters to 
// allow upward passing of attributes.
// Where the calling context does not need to receive
// the attributes, it can use the dummy values defined
// here to save allocating a string locally to satisfy the
// parameter list.
string d1,d2,d3;
bool db;

public:

void init(const string& filename, const string& language)
{ 
	pu=ParseUtility::currentInstance();
	ps=ParseStore::currentInstance();

	ANTLRParser::init();
	parse_language=language;
}

>>

// Compilation Unit: In Java, this is a single file.  This is the start
//   rule for this parser
compilationUnit
	:	
<< 
	// We want to keep track of the scope we are operating in
	string scope; 
>> 
		// A compilation unit starts with an optional package definition
	(	packageDefinition[scope]
		|	/* nothing */
	)

		// Next we have a series of zero or more import statements
	( importDefinition )*

		// Wrapping things up with any number of class or interface
		//    definitions
	( typeDefinition[scope] )*

	eof:Eof
	<< 
		ps->record_other_extent(1,$eof->getLine(),"<file scope items>");
	>>
	;


// Package statement: "package" followed by an identifier.
packageDefinition[string& scope]
	:	PACKAGE identifier2[scope] SEMI
	;


// Import statement: import followed by a package or class name
importDefinition
	:	IMPORT identifierStar SEMI
	;

// A type definition in a file is either a class or interface definition.
typeDefinition[string& parentScope]
	:	<< Visibility v; >>
		m:modifiers[v]
		( classDefinition[parentScope]
		| interfaceDefinition[parentScope]
		)
	|	SEMI
	;

/** A declaration is the creation of a reference or primitive-type variable
 *  Create a separate Type/Var tree for each var in the var list.
 */
declaration
	:	<<
			Visibility v=vDONTKNOW;
			int startLine=LT(1)->getLine();
		>>
		modifiers[v] typeSpec variableDefinitions[d1,d2,db,v,startLine]
	;

// A list of zero or more modifiers.  We could have used (modifier)* in
//   place of a call to modifiers, but I thought it was a good idea to keep
//   this rule separate so they can easily be collected in a Vector if
//   someone so desires
modifiers[Visibility& v]
	:	( modifier[v] )*
	;


// A type specification is a type name with possible brackets afterwards
//   (which would make it an array type).
typeSpec
	: classTypeSpec
	| builtInTypeSpec
	;

// A class type specification is a class type with possible brackets afterwards
//   (which would make it an array type).
classTypeSpec
	:	identifier (lb:LBRACK RBRACK)*
	;

// A builtin type specification is a builtin type with possible brackets
// afterwards (which would make it an array type).
builtInTypeSpec
	:	builtInType (lb:LBRACK RBRACK)*
	;

// A type name. which is either a (possibly qualified) class name or
//   a primitive (builtin) type
type
	:	identifier
	|	builtInType
	;

// The primitive types.
builtInType
	:	KW_VOID
	|	KW_BOOLEAN
	|	KW_BYTE
	|	KW_CHAR
	|	KW_SHORT
	|	KW_INT
	|	KW_FLOAT
	|	KW_LONG
	|	KW_DOUBLE
	;

// A (possibly-qualified) java identifier.  We start with the first IDENT
//   and expand its name by adding dots and following IDENTS
identifier
	:	IDENT  ( DOT IDENT )*
	;

// We replicate typeSpec, classTypeSpec, builtIntTypeSpec and identifier 
// with variants which receive and set a reference to a string
// describing the builtin type. typeSpec also has a boolean indicating 
// whether the described type is built in or not.
typeSpec2[string& typeString, bool& isBuiltIn]
	: classTypeSpec2[typeString] << isBuiltIn=false; >>
	| builtInTypeSpec2[typeString] << isBuiltIn=true; >>
	;

classTypeSpec2[string& typeString]
	:	
		identifier2[typeString] 
		(lb:LBRACK RBRACK << typeString.append("[]"); >> )*
	;

builtInTypeSpec2[string& typeString]
	:	
		<< ANTLRTokenPtr typeToken=LT(1); >>
		builtInType << typeString=typeToken->getText(); >> 
		(LBRACK RBRACK << typeString.append("[]"); >> )*
	;

identifier2[string& scope]
	:	id1:IDENT << scope=pu->scopeCombine(scope,$id1->getText()); >>
		( 
			DOT id2:IDENT 
			<< scope=pu->scopeCombine(scope,$id2->getText()); >>
		)* 
	;
identifierStar
	:	(IDENT (DOT IDENT)* DOT STAR)?
	|	IDENT (DOT IDENT)*
	;


// modifiers for Java classes, interfaces, class/instance vars and methods
modifier[Visibility& v]
	:	PRIVATE << v=vPRIVATE; >> 
	|	PUBLIC << v=vPUBLIC; >>
	|	PROTECTED << v=vPROTECTED; >>
	|	STATIC
	|	TRANSIENT
	|	FINAL
	|	ABSTRACT
	|	NATIVE
	|	THREADSAFE
	|	SYNCHRONIZED
//	|	KW_CONST			// reserved word; leave out
	|	VOLATILE
	;


// Definition of a Java class
classDefinition[string& parentScope]
	:	
<<	
	int startLine=LT(1)->getLine(); 
	string className;
>>
		// the name of the class
		"class" id:IDENT 
<<
		className=$id->getText(); 
>>
		// it _might_ have a superclass...
		sc:superClassClause[className]

		// it might implement some interfaces...
		ic:implementsClause[className]
		// now parse the body of the class
		cb:classBlock[className]
<<
	int endLine=LT(1)->getLine();
	ps->record_module_extent(startLine,endLine,className,"class",
		"definition",utDEFINITION);
>>
	;

superClassClause[const string& className]
	:	<< 
			int startLine=LT(1)->getLine(); 
			string parentName;
		>>
		{ 
			EXTENDS identifier2[parentName] 
			<<
				ps->record_userel_extent(
					startLine,startLine,
					className,"",parentName,"extends",
					// no private inheritance in Java
					vPUBLIC, utINHERITS);
			>>
					
		}
	;

// Definition of a Java Interface
interfaceDefinition[const string& parentScope]
	:	<< string interfaceName; >>
		INTERFACE id:IDENT 
		<< 
			interfaceName=$id->getText();
		>>
		// it might extend some other interfaces
		ie:interfaceExtends[interfaceName]
		// now parse the body of the interface (looks like a class...)
		cb:classBlock[interfaceName]
	;


// This is the body of a class.  You can have fields and extra semicolons,
// That's about it (until you see what a field is...)
classBlock[const string& className]
	:	LCURLY
			( field[className] | SEMI )*
		RCURLY
	;

// An interface can extend several other interfaces...
interfaceExtends[const string& className]
	:	<< 
			int startLine=LT(1)->getLine(); 
			string parentName;
		>>
	{ 
		EXTENDS identifier2[parentName] 
		moreSuperclassNames[className,"extends"]
		<<
				ps->record_userel_extent(
					startLine,startLine,
					className,"",parentName,"extends",
					// no private inheritance in Java
					vPUBLIC, utINHERITS);
		>>
	}
	;

// A class can implement several interfaces...
implementsClause[const std::string& className]
	:	<< 
			int startLine=LT(1)->getLine(); 
			string parentName;
		>>
	{ 
		IMPLEMENTS identifier2[parentName] 
		moreSuperclassNames[className,"implements"]
	}
		<<
				ps->record_userel_extent(
					startLine,startLine,
					className,"",parentName,"implements",
					// no private inheritance in Java
					vPUBLIC, utINHERITS);
		>>
	;

moreSuperclassNames[const string& className, const string& inheritType]
	:	<< 
			int startLine=LT(1)->getLine(); 
			string parentName;
		>>
	(
		COMMA identifier2[parentName]
		<<
				ps->record_userel_extent(
					startLine,startLine,
					className,"",parentName,inheritType,
					// no private inheritance in Java
					vPUBLIC, utINHERITS);
		>>
	)*
	;
			
// Now the various things that can be defined inside a class or interface...
// Note that not all of these are really valid in an interface (constructors,
// for example), and if this grammar were used for a compiler there would
// need to be some semantic checks to make sure we're doing the right thing...
field[const string& className]
	:
<< 
	string scope=className, typeName; 
	bool isBuiltIn;
	Visibility v=vDONTKNOW;
	int startLine=LT(1)->getLine(); 
>>
	// method, constructor, or variable declaration
		mods:modifiers[v]
		(	
			// constructor
			ctorDefinition[className,v] 
		|	cd:classDefinition[scope]             // inner class
		|	id:interfaceDefinition[scope]         // inner interface
			// method or variable declaration(s)
		|	t:typeSpec2[typeName,isBuiltIn] 
			typedDeclaration[className,typeName,isBuiltIn,v,startLine]
		)

    // "static { ... }" class initializer
	|	STATIC s3:compoundStatement

    // "{ ... }" instance initializer
	|	compoundStatement
	;

typedDeclaration
	[const string& className,const string& typeName,
	 bool isBuiltIn, Visibility v, int startLine] 
	:
		    methodDefinition[className,typeName,isBuiltIn,v,startLine]
		|	variableDefinitions[className,typeName,isBuiltIn,v,startLine] SEMI
	;

// The next rule covers that part of a method definition which
// occurs after the return type is recognized.
// The return type is recognized separately as a left-factoring
// because it can equally well be the type of a variable definition.
methodDefinition
	[const string& className, const string& returnType, bool rtIsBuiltIn,
	Visibility v, int startLine] 
	:
		<< 
			string paramList; 
			bool isDefinition=false;
		>>
		// the name of the method
		id:IDENT   
		// parse the formal parameter declarations.
		LPAREN param:parameterDeclarationList[paramList,className,v] RPAREN
		rt:returnTypeBrackersOnEndOfMethodHead
		// get the list of exceptions that this method is declared to throw
		{ tc:throwsClause }
		( 
		  s2:compoundStatement << isDefinition=true; >> 
		| SEMI 
		)
	  <<
			int endLine=LT(1)->getLine();
			string methodName=$id->getText();
          	if(isDefinition==false)
          	{
	      		ps->record_function_extent(startLine,endLine,
					 returnType,className,	
					 methodName,paramList,
					 "declaration",v,
					 utDECLARATION);
	  		}
          	else
          	{
	      		ps->record_function_extent(startLine,endLine,
					 returnType,className,
					 methodName,paramList,
					 "definition",v,
					 utDEFINITION);
          	}

			// the return value is effectively an anonymous 
			// parameter...
			if(rtIsBuiltIn==false)
			{
				// In Java, all parameter passing are effectively by
				// reference (I think).
				ps->record_userel_extent(startLine,startLine,
					className,"",returnType,
					"return type",v,utPARBYREF);
			}

	>>
	;

variableDefinitions
	[const string& className, const string& returnType, bool rtIsBuiltIn,
	Visibility v, int startLine] 
	:	variableDeclarator
		(	COMMA
			variableDeclarator
		)*
	<<
		if(rtIsBuiltIn==false)
		{
			// In Java, all containment relationships are effectively by
			// reference (I think).
			ps->record_userel_extent(startLine,startLine,
				className,"",returnType,
				"member variable",v,utHASBYREF);
		}
	>>
	;

/** Declaration of a variable.  This can be a class/instance variable,
 *   or a local variable in a method
 * It can also include possible initialization.
 */
variableDeclarator
	:	id:IDENT d:declaratorBrackets v:varInitializer
	;

declaratorBrackets
	:	
		(lb:LBRACK RBRACK)*
	;

varInitializer
	:	{ ASSIGN initializer }
	;

// This is an initializer used to set up an array.
arrayInitializer
	:	lc:LCURLY
			{
				initializer
				(
					// CONFLICT: does a COMMA after an initializer start a new
					//           initializer or start the option ',' at end?
					//           ANTLR generates proper code by matching
					//			 the comma as soon as possible.
					COMMA initializer
				)*
				{COMMA}
			}
		RCURLY
	;


// The two "things" that can initialize an array element are an expression
//   and another (nested) array initializer.
initializer
	:	expression
	|	arrayInitializer
	;

// This is the header of a method.  It includes the name and parameters
//   for the method.
//   This also watches for a list of exception classes in a "throws" clause.
ctorDefinition[const string& className, Visibility v]
	:
	<< 
                // The following assertion seemed like a good idea
                // at the time, but it fails for nested classes.
                // assert(className==LT(1)->getText());
		int startLine=LT(1)->getLine();
		string paramList;
	>>
	// constructors are the only methods allowed to occur without
	// return types, and they must have definitions.  This makes
	// life fairly easy
	ctorHead[paramList,className,v] compoundStatement
	<<
		int endLine=LT(1)->getLine();
		ps->record_function_extent(startLine,endLine,
		 "",className,className,paramList,
		 "definition",v,utDEFINITION);
	>>	
	;			

ctorHead[string& paramList,const string& className, Visibility v]
	:
	IDENT  // the name of the method

		// parse the formal parameter declarations.
		LPAREN parameterDeclarationList[paramList,className,v] RPAREN

		// get the list of exceptions that this method is declared to throw
		{ throwsClause }
	;

// This is a list of exception classes that the method is declared to throw
throwsClause
	:	THROWS identifier ( COMMA identifier )*
	;


returnTypeBrackersOnEndOfMethodHead
	:	(LBRACK RBRACK)*
	;

// A list of formal parameters
parameterDeclarationList
	[
		// a string in which we accumulate a string of all parameters
		string& paramList, 
		// the name of the owner class for the method
	 	const string& clientName, 
		// visibility of the method
	 	const Visibility& v      
	]
	:	<< string paramItem; >>
		<< paramList+="("; >>
		{ 
			parameterDeclaration[paramList,clientName,v,true]
			( 
				COMMA << paramList+=","; >> 
				parameterDeclaration[paramList,clientName,v,true] 
			)* 
		}
		<< paramList+=")"; >> 
	;

// A formal parameter.
parameterDeclaration
	[
		// a string in which we accumulate a string of all parameters
		string& paramList, 
		// the name of the owner class for the method
	 	const string& clientName, 
		// visibility of the method
	 	const Visibility& v,      
		// is this rule being used from a parameter list (in which case
		// we want to do record_userel_extent) or from a syntacticly similar
		// catch statement, which is buried in the implementation of a method 
		// and does not interest us
		bool inParameterList
	]
	:	<< 
			string mod, typeName, bracks; bool isBuiltIn; 
			int startLine=LT(1)->getLine();
		>>
		pm:parameterModifier[mod] t:typeSpec2[typeName,isBuiltIn] id:IDENT
		pd:parameterDeclaratorBrackets[bracks]
		<< 
			if(inParameterList)
			{
				paramList+=(mod+typeName+bracks); 

				if(isBuiltIn==false)
				{
					// In Java, all parameter passing is effectively by
					// reference (I think).
					ps->record_userel_extent(startLine,startLine,
						clientName,"",typeName,
						"normal parameter",v,utPARBYREF);
				}
			}
		>>
	;

parameterDeclaratorBrackets[string& bracks]
	:	(LBRACK RBRACK << bracks+="[]"; >> )*
	;

parameterModifier[string& paramList]
	:	{ f:FINAL << paramList+=$f->getText(); >> }
	;

// Compound statement.  This is used in many contexts:
//   Inside a class definition prefixed with "static":
//      it is a class initializer
//   Inside a class definition without "static":
//      it is an instance initializer
//   As the body of a method
//   As a completely indepdent braced block of code inside a method
//      it starts a new scope for variable definitions

compoundStatement
	:	LCURLY 
			// include the (possibly-empty) list of statements
			(statement)*
		RCURLY
	;


statement
	// A list of statements in curly braces -- start a new scope!
	:
<< string scope; >>
	  ifStatement
	| forStatement
	| whileStatement
	| doWhileStatement
	| breakStatement
	| continueStatement
	| returnStatement
	| switchStatement
	| throwStatement
	| tryBlock
	| syncStatement
	| emptyStatement
	| compoundStatement

	// class definition
	|	classDefinition[scope]

	// final class definition
	|	FINAL classDefinition[scope]

	// abstract class definition
	|	ABSTRACT classDefinition[scope]

	// declarations are ambiguous with "ID DOT" relative to expression
	// statements.  Must backtrack to be sure.  Could use a semantic
	// predicate to test symbol table to see what the type was coming
	// up, but that's pretty hard without a symbol table ;)
	|	(declaration)? declaration SEMI

	// An expression statement.  This could be a method call,
	// assignment statement, or any other expression evaluated for
	// side-effects.
	|	expression SEMI

	// Attach a label to the front of a statement
	|	IDENT c:COLON statement
	;
	
	// If-else statement
ifStatement :
	IF LPAREN expression RPAREN statement
	optElseClause
	;

	// For statement
forStatement :
	FOR
		LPAREN
			forInit SEMI   // initializer
			forCond	SEMI   // condition test
			forIter         // updater
		RPAREN
		statement                     // statement to loop over
	;

	// While statement
whileStatement :
	WHILE LPAREN expression RPAREN statement
	;

	// do-while statement
doWhileStatement :
	DO statement "while" LPAREN expression RPAREN SEMI
	;

	// get out of a loop (or switch)
breakStatement :
	BREAK { IDENT } SEMI
	;

	// do next iteration of a loop
continueStatement :
	CONTINUE { IDENT } SEMI
	;

	// Return an expression
returnStatement :
	RETURN { expression } SEMI
	;

	// switch/case statement
switchStatement :
	SWITCH LPAREN expression RPAREN LCURLY
		( casesGroup )*
	RCURLY
	;


	// throw an exception
throwStatement :
	THROW expression SEMI
	;

	// synchronize a statement
syncStatement :
	SYNCHRONIZED LPAREN expression RPAREN compoundStatement
	;

	// empty statement
emptyStatement :
	SEMI 
	;

optElseClause
	:	(ELSE)? ELSE statement
	| 	/* empty */
	;

casesGroup
	:	cases caseSList
	;

cases	
	:	aCase optMoreCases
	;

optMoreCases
	:	(CASE | DEFAULT)? aCase optMoreCases
	|	/* empty */
	; 
	
aCase
	:	DEFAULT COLON
	|	CASE expression COLON
	;

caseSList
	:	(statement)*
	;

// The initializer for a for loop
forInit
		// if it looks like a declaration, it is
	:	(declaration)?
		// otherwise it could be an expression list...
	|	expressionList
	|	// or it could be empty
	;

forCond
	:	{ expression }
	;

forIter
	:	{ expressionList }
	;

// an exception handler try/catch block
tryBlock
	:	TRY compoundStatement
		(handler)*
		{ FINALLY compoundStatement }
	;


// an exception handler
handler
	:	<< Visibility dv;  >> 
		CATCH LPAREN parameterDeclaration[d1,d2,dv,false]  RPAREN compoundStatement
	;


// expressions
// Note that most of these expressions follow the pattern
//   thisLevelExpression :
//       nextHigherPrecedenceExpression
//           (OPERATOR nextHigherPrecedenceExpression)*
// which is a standard recursive definition for a parsing an expression.
// The operators in java have the following precedences:
//    lowest  (13)  = *= /= %= += -= <<= >>= >>>= &= ^= |=
//            (12)  ?:
//            (11)  ||
//            (10)  &&
//            ( 9)  |
//            ( 8)  ^
//            ( 7)  &
//            ( 6)  == !=
//            ( 5)  < <= > >=
//            ( 4)  << >>
//            ( 3)  +(binary) -(binary)
//            ( 2)  * / %
//            ( 1)  ++ -- +(unary) -(unary)  ~  !  (type)
//                  []   () (method call)  . (dot -- identifier qualification)
//                  new   ()  (explicit parenthesis)
//
// the last two are not usually on a precedence chart; I put them in
// to point out that new has a higher precedence than '.', so you
// can validy use
//     new Frame().show()
// 
// Note that the above precedence levels map to the rules below...
// Once you have a precedence chart, writing the appropriate rules as below
//   is usually very straightfoward

// the mother of all expressions
expression
	:	assignmentExpression
	;


// This is a list of expressions.
expressionList
	:	expression (COMMA expression)*
	;


// assignment expression (level 13)
assignmentExpression
	:	conditionalExpression
		{
			(	ASSIGN
			|   PLUS_ASSIGN
			|   MINUS_ASSIGN
			|   STAR_ASSIGN
			|   DIV_ASSIGN
			|   MOD_ASSIGN
			|   SR_ASSIGN
			|   BSR_ASSIGN
			|   SL_ASSIGN
			|   BAND_ASSIGN
			|   BXOR_ASSIGN
			|   BOR_ASSIGN
            		)
			assignmentExpression
		}
	;


// conditional test (level 12)
conditionalExpression
	:	logicalOrExpression
		{ QUESTION conditionalExpression COLON conditionalExpression }
	;


// logical or (||)  (level 11)
logicalOrExpression
	:	logicalAndExpression (LOR logicalAndExpression)*
	;


// logical and (&&)  (level 10)
logicalAndExpression
	:	inclusiveOrExpression (LAND inclusiveOrExpression)*
	;


// bitwise or non-short-circuiting or (|)  (level 9)
inclusiveOrExpression
	:	exclusiveOrExpression (BOR exclusiveOrExpression)*
	;


// exclusive or (^)  (level 8)
exclusiveOrExpression
	:	andExpression (BXOR andExpression)*
	;


// bitwise or non-short-circuiting and (&)  (level 7)
andExpression
	:	equalityExpression (BAND equalityExpression)*
	;


// equality/inequality (==/!=) (level 6)
equalityExpression
	:	relationalExpression relationalPredicate
	;

relationalPredicate 
	:	INSTANCEOF typeSpec
	|	((NOT_EQUAL | EQUAL) relationalExpression)*
	;


// boolean relational expressions (level 5)
relationalExpression
	:	shiftExpression
		(	(	LESSTHAN
			|	GT
			|	LE
			|	GE
			)
			shiftExpression
		)*
	;

// bit shift expressions (level 4)
shiftExpression
	:	additiveExpression ((SL | SR | BSR) additiveExpression)*
	;


// binary addition/subtraction (level 3)
additiveExpression
	:	multiplicativeExpression ((PLUS | MINUS) multiplicativeExpression)*
	;


// multiplication/division/modulo (level 2)
multiplicativeExpression
	:	unaryExpression ((STAR | DIV | MOD ) unaryExpression)*
	;

unaryExpression
	:	INC unaryExpression
	|	DEC unaryExpression
	|	MINUS unaryExpression
	|	PLUS unaryExpression
	|	unaryExpressionNotPlusMinus
	;

unaryExpressionNotPlusMinus
	:	BNOT unaryExpression
	|	LNOT unaryExpression
 
		// If typecast is built in type, must be numeric operand
		// Also, no reason to backtrack if type keyword like int, float...
	|	(LPAREN builtInTypeSpec RPAREN unaryExpression)?

		// Have to backtrack to see if operator follows.  If no operator
		// follows, it's a typecast.  No semantic checking needed to parse.
		// if it _looks_ like a cast, it _is_ a cast; else it's a "(expr)"
	|	(LPAREN classTypeSpec RPAREN unaryExpressionNotPlusMinus)?

	|	postfixExpression
	;

// qualified names, array expressions, method invocation, post inc/dec
postfixExpression
	:	primaryExpression // start with a primary

		(	// qualified id (id.id.id.id...) -- build the name
			DOT ( IDENT
				| KW_THIS
				| CLASS
				| newExpression
				| SUPER LPAREN { expressionList } RPAREN
				)
			// the above line needs a semantic check to make sure "class"
			//   is the _last_ qualifier.

			// allow ClassName[].class
		|	( LBRACK RBRACK )+
			DOT CLASS

			// an array indexing operation
		|	LBRACK expression RBRACK

			// method invocation
			// The next line is not strictly proper; it allows x(3)(4) or
			//  x[2](4) which are not valid in Java.  If this grammar were used
			//  to validate a Java program a semantic check would be needed, or
			//   this rule would get really ugly...
		|	LPAREN argList RPAREN
		)*

		// possibly add on a post-increment or post-decrement.
		// allows INC/DEC on too much, but semantics can check
		(	INC
	 	|	DEC
		|	// nothing
		)

		// look for int.class and int[].class
	|	builtInType 
		( LBRACK RBRACK )*
		DOT CLASS
	;

// the basic element of an expression
primaryExpression
	:	IDENT
	|	newExpression
	|	constant
	|	SUPER
	|	BTRUE
	|	BFALSE
	|	KW_THIS
	|	PNULL
	|	LPAREN assignmentExpression RPAREN
	;

/** object instantiation.
 *  Trees are built as illustrated by the following input/tree pairs:
 *  
 *  new T()
 *  
 *  new
 *   |
 *   T --  ELIST
 *           |
 *          arg1 -- arg2 -- .. -- argn
 *  
 *  new int[]
 *
 *  new
 *   |
 *  int -- ARRAY_DECLARATOR
 *  
 *  new int[] {1,2}
 *
 *  new
 *   |
 *  int -- ARRAY_DECLARATOR -- ARRAY_INIT
 *                                  |
 *                                EXPR -- EXPR
 *                                  |      |
 *                                  1      2
 *  
 *  new int[3]
 *  new
 *   |
 *  int -- ARRAY_DECLARATOR
 *                |
 *              EXPR
 *                |
 *                3
 *  
 *  new int[1][2]
 *  
 *  new
 *   |
 *  int -- ARRAY_DECLARATOR
 *               |
 *         ARRAY_DECLARATOR -- EXPR
 *               |              |
 *             EXPR             1
 *               |
 *               2
 *  
 */
newExpression
	:	"new" type
		(	LPAREN argList RPAREN {classBlock[d1]}

			//java 1.1
			// Note: This will allow bad constructs like
			//    new int[4][][3] {exp,exp}.
			//    There needs to be a semantic check here...
			// to make sure:
			//   a) [ expr ] and [ ] are not mixed
			//   b) [ expr ] and an init are not used together

		|	newArrayDeclarator {arrayInitializer}
		)
	;

argList
	:	(	expressionList
		|	/*nothing*/
		)
	;

newArrayDeclarator
	// CONFLICT:
	// newExpression is a primaryExpression which can be
	// followed by an array index reference.  This is ok,
	// as the generated code will stay in this loop as
	// long as it sees an LBRACK (proper behavior)
	:	(LBRACK { expression } RBRACK LBRACK)?
		LBRACK { expression } RBRACK newArrayDeclarator
	|	LBRACK { expression } RBRACK
	;


constant
        :       NUM_INT1
        |       NUM_INT1A
	| 	NUM_INT1B
	|	NUM_INT2
	|	NUM_INT3
	|	NUM_INT4
	|	CHARCONST
	|	STRINGCONST
	|	NUM_FLOAT
	;
}

