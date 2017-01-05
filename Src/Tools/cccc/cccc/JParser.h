/*
 * JParser: P a r s e r  H e a d e r 
 *
 * Generated from: java.g
 *
 * Terence Parr, Russell Quong, Will Cohen, and Hank Dietz: 1989-1995
 * Parr Research Corporation
 * with Purdue University Electrical Engineering
 * with AHPCRC, University of Minnesota
 * ANTLR Version 1.33
 */

#ifndef JParser_h
#define JParser_h
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
class JParser : public ANTLRParser {
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

  
	static SetWordType err1[28];
	static SetWordType err2[28];
	static SetWordType err3[28];
	static SetWordType setwd1[221];
	static SetWordType err4[28];
	static SetWordType err5[28];
	static SetWordType err6[28];
	static SetWordType setwd2[221];
	static SetWordType err7[28];
	static SetWordType err8[28];
	static SetWordType err9[28];
	static SetWordType setwd3[221];
	static SetWordType err10[28];
	static SetWordType setwd4[221];
	static SetWordType err11[28];
	static SetWordType err12[28];
	static SetWordType err13[28];
	static SetWordType err14[28];
	static SetWordType setwd5[221];
	static SetWordType err15[28];
	static SetWordType err16[28];
	static SetWordType err17[28];
	static SetWordType setwd6[221];
	static SetWordType err18[28];
	static SetWordType setwd7[221];
	static SetWordType setwd8[221];
	static SetWordType err19[28];
	static SetWordType err20[28];
	static SetWordType setwd9[221];
	static SetWordType setwd10[221];
	static SetWordType err21[28];
	static SetWordType err22[28];
	static SetWordType setwd11[221];
	static SetWordType err23[28];
	static SetWordType err24[28];
	static SetWordType err25[28];
	static SetWordType err26[28];
	static SetWordType setwd12[221];
	static SetWordType err27[28];
	static SetWordType err28[28];
	static SetWordType setwd13[221];
	static SetWordType err29[28];
	static SetWordType setwd14[221];
	static SetWordType err30[28];
	static SetWordType err31[28];
	static SetWordType setwd15[221];
	static SetWordType err32[28];
	static SetWordType err33[28];
	static SetWordType err34[28];
	static SetWordType err35[28];
	static SetWordType setwd16[221];
	static SetWordType err36[28];
	static SetWordType err37[28];
	static SetWordType err38[28];
	static SetWordType setwd17[221];
	static SetWordType err39[28];
	static SetWordType err40[28];
	static SetWordType err41[28];
	static SetWordType err42[28];
	static SetWordType err43[28];
	static SetWordType setwd18[221];
	static SetWordType err44[28];
	static SetWordType err45[28];
	static SetWordType err46[28];
	static SetWordType err47[28];
	static SetWordType setwd19[221];
	static SetWordType setwd20[221];
private:
	void zzdflthandlers( int _signal, int *_retsignal );

public:
	JParser(ANTLRTokenBuffer *input);
	void compilationUnit(void);
	void packageDefinition( string& scope );
	void importDefinition(void);
	void typeDefinition( string& parentScope );
	void declaration(void);
	void modifiers( Visibility& v );
	void typeSpec(void);
	void classTypeSpec(void);
	void builtInTypeSpec(void);
	void type(void);
	void builtInType(void);
	void identifier(void);
	void typeSpec2( string& typeString, bool& isBuiltIn );
	void classTypeSpec2( string& typeString );
	void builtInTypeSpec2( string& typeString );
	void identifier2( string& scope );
	void identifierStar(void);
	void modifier( Visibility& v );
	void classDefinition( string& parentScope );
	void superClassClause( const string& className );
	void interfaceDefinition( const string& parentScope );
	void classBlock( const string& className );
	void interfaceExtends( const string& className );
	void implementsClause( const std::string& className );
	void moreSuperclassNames( const string& className, const string& inheritType );
	void field( const string& className );
	void typedDeclaration( const string& className,const string& typeName,
	 bool isBuiltIn, Visibility v, int startLine );
	void methodDefinition( const string& className, const string& returnType, bool rtIsBuiltIn,
	Visibility v, int startLine );
	void variableDefinitions( const string& className, const string& returnType, bool rtIsBuiltIn,
	Visibility v, int startLine );
	void variableDeclarator(void);
	void declaratorBrackets(void);
	void varInitializer(void);
	void arrayInitializer(void);
	void initializer(void);
	void ctorDefinition( const string& className, Visibility v );
	void ctorHead( string& paramList,const string& className, Visibility v );
	void throwsClause(void);
	void returnTypeBrackersOnEndOfMethodHead(void);
	void parameterDeclarationList( 
		// a string in which we accumulate a string of all parameters
		string& paramList, 
		// the name of the owner class for the method
	 	const string& clientName, 
		// visibility of the method
	 	const Visibility& v      
	 );
	void parameterDeclaration( 
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
	 );
	void parameterDeclaratorBrackets( string& bracks );
	void parameterModifier( string& paramList );
	void compoundStatement(void);
	void statement(void);
	void ifStatement(void);
	void forStatement(void);
	void whileStatement(void);
	void doWhileStatement(void);
	void breakStatement(void);
	void continueStatement(void);
	void returnStatement(void);
	void switchStatement(void);
	void throwStatement(void);
	void syncStatement(void);
	void emptyStatement(void);
	void optElseClause(void);
	void casesGroup(void);
	void cases(void);
	void optMoreCases(void);
	void aCase(void);
	void caseSList(void);
	void forInit(void);
	void forCond(void);
	void forIter(void);
	void tryBlock(void);
	void handler(void);
	void expression(void);
	void expressionList(void);
	void assignmentExpression(void);
	void conditionalExpression(void);
	void logicalOrExpression(void);
	void logicalAndExpression(void);
	void inclusiveOrExpression(void);
	void exclusiveOrExpression(void);
	void andExpression(void);
	void equalityExpression(void);
	void relationalPredicate(void);
	void relationalExpression(void);
	void shiftExpression(void);
	void additiveExpression(void);
	void multiplicativeExpression(void);
	void unaryExpression(void);
	void unaryExpressionNotPlusMinus(void);
	void postfixExpression(void);
	void primaryExpression(void);
	void newExpression(void);
	void argList(void);
	void newArrayDeclarator(void);
	void constant(void);
};

#endif /* JParser_h */
