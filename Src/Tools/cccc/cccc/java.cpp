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
#include "Jtokens.h"

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
#include "AParser.h"
#include "JParser.h"
#include "DLexerBase.h"
#include "ATokPtr.h"
#ifndef PURIFY
#define PURIFY(r,s)
#endif

void
JParser::compilationUnit(void)
{
	zzRULE;
	ANTLRTokenPtr eof=NULL;
	
	// We want to keep track of the scope we are operating in
	string scope; 
	tracein("compilationUnit");
	{
		if ( (LA(1)==PACKAGE) ) {
			packageDefinition( scope );
		}
		else {
			if ( (setwd1[LA(1)]&0x1) ) {
			}
			else {FAIL(1,err1,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
		}
	}
	{
		while ( (LA(1)==IMPORT) ) {
			importDefinition();
		}
	}
	{
		while ( (setwd1[LA(1)]&0x2) ) {
			typeDefinition( scope );
		}
	}
	zzmatch(Eof); labase++;
	
	if ( !guessing ) {
		eof = (ANTLRTokenPtr)LT(1);
}
	
	if ( !guessing ) {
	
	ps->record_other_extent(1, eof->getLine(),"<file scope items>");
	}
 consume();
	traceout("compilationUnit");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd1, 0x4);
	traceout("compilationUnit");
}

void
JParser::packageDefinition( string& scope )
{
	zzRULE;
	tracein("packageDefinition");
	zzmatch(PACKAGE); labase++;
	 consume();
	identifier2( scope );
	zzmatch(SEMI); labase++;
	 consume();
	traceout("packageDefinition");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd1, 0x8);
	traceout("packageDefinition");
}

void
JParser::importDefinition(void)
{
	zzRULE;
	tracein("importDefinition");
	zzmatch(IMPORT); labase++;
	 consume();
	identifierStar();
	zzmatch(SEMI); labase++;
	 consume();
	traceout("importDefinition");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd1, 0x10);
	traceout("importDefinition");
}

void
JParser::typeDefinition( string& parentScope )
{
	zzRULE;
	ANTLRTokenPtr m=NULL;
	Visibility v;   
	tracein("typeDefinition");
	if ( (setwd1[LA(1)]&0x20)
 ) {
		modifiers( v );
		{
			if ( (LA(1)==CLASS) ) {
				classDefinition( parentScope );
			}
			else {
				if ( (LA(1)==INTERFACE) ) {
					interfaceDefinition( parentScope );
				}
				else {FAIL(1,err2,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
			}
		}
	}
	else {
		if ( (LA(1)==SEMI) ) {
			zzmatch(SEMI); labase++;
			 consume();
		}
		else {FAIL(1,err3,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
	}
	traceout("typeDefinition");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd1, 0x40);
	traceout("typeDefinition");
}

void
JParser::declaration(void)
{
	zzRULE;
	
	Visibility v=vDONTKNOW;
	int startLine=LT(1)->getLine();
	tracein("declaration");
	modifiers( v );
	typeSpec();
	variableDefinitions( d1,d2,db,v,startLine );
	traceout("declaration");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd1, 0x80);
	traceout("declaration");
}

void
JParser::modifiers( Visibility& v )
{
	zzRULE;
	tracein("modifiers");
	{
		while ( (setwd2[LA(1)]&0x1) ) {
			modifier( v );
		}
	}
	traceout("modifiers");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd2, 0x2);
	traceout("modifiers");
}

void
JParser::typeSpec(void)
{
	zzRULE;
	tracein("typeSpec");
	if ( (LA(1)==IDENT)
 ) {
		classTypeSpec();
	}
	else {
		if ( (setwd2[LA(1)]&0x4) ) {
			builtInTypeSpec();
		}
		else {FAIL(1,err4,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
	}
	traceout("typeSpec");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd2, 0x8);
	traceout("typeSpec");
}

void
JParser::classTypeSpec(void)
{
	zzRULE;
	ANTLRTokenPtr lb=NULL;
	tracein("classTypeSpec");
	identifier();
	{
		while ( (LA(1)==LBRACK) ) {
			zzmatch(LBRACK); labase++;
			
			if ( !guessing ) {
						lb = (ANTLRTokenPtr)LT(1);
}
			 consume();
			zzmatch(RBRACK); labase++;
			 consume();
		}
	}
	traceout("classTypeSpec");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd2, 0x10);
	traceout("classTypeSpec");
}

void
JParser::builtInTypeSpec(void)
{
	zzRULE;
	ANTLRTokenPtr lb=NULL;
	tracein("builtInTypeSpec");
	builtInType();
	{
		while ( (LA(1)==LBRACK) ) {
			zzmatch(LBRACK); labase++;
			
			if ( !guessing ) {
						lb = (ANTLRTokenPtr)LT(1);
}
			 consume();
			zzmatch(RBRACK); labase++;
			 consume();
		}
	}
	traceout("builtInTypeSpec");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd2, 0x20);
	traceout("builtInTypeSpec");
}

void
JParser::type(void)
{
	zzRULE;
	tracein("type");
	if ( (LA(1)==IDENT) ) {
		identifier();
	}
	else {
		if ( (setwd2[LA(1)]&0x40)
 ) {
			builtInType();
		}
		else {FAIL(1,err5,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
	}
	traceout("type");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd2, 0x80);
	traceout("type");
}

void
JParser::builtInType(void)
{
	zzRULE;
	tracein("builtInType");
	if ( (LA(1)==KW_VOID) ) {
		zzmatch(KW_VOID); labase++;
		 consume();
	}
	else {
		if ( (LA(1)==KW_BOOLEAN) ) {
			zzmatch(KW_BOOLEAN); labase++;
			 consume();
		}
		else {
			if ( (LA(1)==KW_BYTE) ) {
				zzmatch(KW_BYTE); labase++;
				 consume();
			}
			else {
				if ( (LA(1)==KW_CHAR) ) {
					zzmatch(KW_CHAR); labase++;
					 consume();
				}
				else {
					if ( (LA(1)==KW_SHORT)
 ) {
						zzmatch(KW_SHORT); labase++;
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
								if ( (LA(1)==KW_LONG) ) {
									zzmatch(KW_LONG); labase++;
									 consume();
								}
								else {
									if ( (LA(1)==KW_DOUBLE) ) {
										zzmatch(KW_DOUBLE); labase++;
										 consume();
									}
									else {FAIL(1,err6,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
								}
							}
						}
					}
				}
			}
		}
	}
	traceout("builtInType");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd3, 0x1);
	traceout("builtInType");
}

void
JParser::identifier(void)
{
	zzRULE;
	tracein("identifier");
	zzmatch(IDENT); labase++;
	 consume();
	{
		while ( (LA(1)==DOT)
 ) {
			zzmatch(DOT); labase++;
			 consume();
			zzmatch(IDENT); labase++;
			 consume();
		}
	}
	traceout("identifier");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd3, 0x2);
	traceout("identifier");
}

void
JParser::typeSpec2( string& typeString, bool& isBuiltIn )
{
	zzRULE;
	tracein("typeSpec2");
	if ( (LA(1)==IDENT) ) {
		classTypeSpec2( typeString );
		if ( !guessing ) {
		isBuiltIn=false;   
		}
	}
	else {
		if ( (setwd3[LA(1)]&0x4) ) {
			builtInTypeSpec2( typeString );
			if ( !guessing ) {
			isBuiltIn=true;   
			}
		}
		else {FAIL(1,err7,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
	}
	traceout("typeSpec2");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd3, 0x8);
	traceout("typeSpec2");
}

void
JParser::classTypeSpec2( string& typeString )
{
	zzRULE;
	ANTLRTokenPtr lb=NULL;
	tracein("classTypeSpec2");
	identifier2( typeString );
	{
		while ( (LA(1)==LBRACK) ) {
			zzmatch(LBRACK); labase++;
			
			if ( !guessing ) {
						lb = (ANTLRTokenPtr)LT(1);
}
			 consume();
			zzmatch(RBRACK); labase++;
			
			if ( !guessing ) {
			typeString.append("[]");   
			}
 consume();
		}
	}
	traceout("classTypeSpec2");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd3, 0x10);
	traceout("classTypeSpec2");
}

void
JParser::builtInTypeSpec2( string& typeString )
{
	zzRULE;
	ANTLRTokenPtr typeToken=LT(1);   
	tracein("builtInTypeSpec2");
	builtInType();
	if ( !guessing ) {
	typeString=typeToken->getText();   
	}
	{
		while ( (LA(1)==LBRACK) ) {
			zzmatch(LBRACK); labase++;
			 consume();
			zzmatch(RBRACK); labase++;
			
			if ( !guessing ) {
			typeString.append("[]");   
			}
 consume();
		}
	}
	traceout("builtInTypeSpec2");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd3, 0x20);
	traceout("builtInTypeSpec2");
}

void
JParser::identifier2( string& scope )
{
	zzRULE;
	ANTLRTokenPtr id1=NULL, id2=NULL;
	tracein("identifier2");
	zzmatch(IDENT); labase++;
	
	if ( !guessing ) {
		id1 = (ANTLRTokenPtr)LT(1);
}
	
	if ( !guessing ) {
	scope=pu->scopeCombine(scope, id1->getText());   
	}
 consume();
	{
		while ( (LA(1)==DOT)
 ) {
			zzmatch(DOT); labase++;
			 consume();
			zzmatch(IDENT); labase++;
			
			if ( !guessing ) {
						id2 = (ANTLRTokenPtr)LT(1);
}
			
			if ( !guessing ) {
			scope=pu->scopeCombine(scope, id2->getText());   
			}
 consume();
		}
	}
	traceout("identifier2");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd3, 0x40);
	traceout("identifier2");
}

void
JParser::identifierStar(void)
{
	zzRULE;
	zzGUESS_BLOCK
	tracein("identifierStar");
	zzGUESS
	if ( !zzrv && (LA(1)==IDENT) && (LA(2)==DOT) ) {
		{
			zzmatch(IDENT); labase++;
			 consume();
			{
				while ( (LA(1)==DOT) && (LA(2)==IDENT) ) {
					zzmatch(DOT); labase++;
					 consume();
					zzmatch(IDENT); labase++;
					 consume();
				}
			}
			zzmatch(DOT); labase++;
			 consume();
			zzmatch(STAR); labase++;
			 consume();
		}
		zzGUESS_DONE
		{
			zzmatch(IDENT); labase++;
			 consume();
			{
				while ( (LA(1)==DOT) && (LA(2)==IDENT) ) {
					zzmatch(DOT); labase++;
					 consume();
					zzmatch(IDENT); labase++;
					 consume();
				}
			}
			zzmatch(DOT); labase++;
			 consume();
			zzmatch(STAR); labase++;
			 consume();
		}
	}
	else {
		if ( !zzrv ) zzGUESS_DONE;
		if ( (LA(1)==IDENT) && (setwd3[LA(2)]&0x80) ) {
			zzmatch(IDENT); labase++;
			 consume();
			{
				while ( (LA(1)==DOT)
 ) {
					zzmatch(DOT); labase++;
					 consume();
					zzmatch(IDENT); labase++;
					 consume();
				}
			}
		}
		else {FAIL(2,err8,err9,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
	}
	traceout("identifierStar");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd4, 0x1);
	traceout("identifierStar");
}

void
JParser::modifier( Visibility& v )
{
	zzRULE;
	tracein("modifier");
	if ( (LA(1)==PRIVATE) ) {
		zzmatch(PRIVATE); labase++;
		
		if ( !guessing ) {
		v=vPRIVATE;   
		}
 consume();
	}
	else {
		if ( (LA(1)==PUBLIC) ) {
			zzmatch(PUBLIC); labase++;
			
			if ( !guessing ) {
			v=vPUBLIC;   
			}
 consume();
		}
		else {
			if ( (LA(1)==PROTECTED) ) {
				zzmatch(PROTECTED); labase++;
				
				if ( !guessing ) {
				v=vPROTECTED;   
				}
 consume();
			}
			else {
				if ( (LA(1)==STATIC) ) {
					zzmatch(STATIC); labase++;
					 consume();
				}
				else {
					if ( (LA(1)==TRANSIENT)
 ) {
						zzmatch(TRANSIENT); labase++;
						 consume();
					}
					else {
						if ( (LA(1)==FINAL) ) {
							zzmatch(FINAL); labase++;
							 consume();
						}
						else {
							if ( (LA(1)==ABSTRACT) ) {
								zzmatch(ABSTRACT); labase++;
								 consume();
							}
							else {
								if ( (LA(1)==NATIVE) ) {
									zzmatch(NATIVE); labase++;
									 consume();
								}
								else {
									if ( (LA(1)==THREADSAFE) ) {
										zzmatch(THREADSAFE); labase++;
										 consume();
									}
									else {
										if ( (LA(1)==SYNCHRONIZED)
 ) {
											zzmatch(SYNCHRONIZED); labase++;
											 consume();
										}
										else {
											if ( (LA(1)==VOLATILE) ) {
												zzmatch(VOLATILE); labase++;
												 consume();
											}
											else {FAIL(1,err10,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
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
	traceout("modifier");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd4, 0x2);
	traceout("modifier");
}

void
JParser::classDefinition( string& parentScope )
{
	zzRULE;
	ANTLRTokenPtr id=NULL, sc=NULL, ic=NULL, cb=NULL;
	
	int startLine=LT(1)->getLine(); 
	string className;
	tracein("classDefinition");
	zzmatch(CLASS); labase++;
	 consume();
	zzmatch(IDENT); labase++;
	
	if ( !guessing ) {
		id = (ANTLRTokenPtr)LT(1);
}
	
	if ( !guessing ) {
	
	className= id->getText(); 
	}
 consume();
	superClassClause( className );
	implementsClause( className );
	classBlock( className );
	if ( !guessing ) {
	
	int endLine=LT(1)->getLine();
	ps->record_module_extent(startLine,endLine,className,"class",
	"definition",utDEFINITION);
	}
	traceout("classDefinition");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd4, 0x4);
	traceout("classDefinition");
}

void
JParser::superClassClause( const string& className )
{
	zzRULE;
	
	int startLine=LT(1)->getLine(); 
	string parentName;
	tracein("superClassClause");
	{
		if ( (LA(1)==EXTENDS) ) {
			zzmatch(EXTENDS); labase++;
			 consume();
			identifier2( parentName );
			if ( !guessing ) {
			
			ps->record_userel_extent(
			startLine,startLine,
			className,"",parentName,"extends",
			// no private inheritance in Java
			vPUBLIC, utINHERITS);
			}
		}
	}
	traceout("superClassClause");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd4, 0x8);
	traceout("superClassClause");
}

void
JParser::interfaceDefinition( const string& parentScope )
{
	zzRULE;
	ANTLRTokenPtr id=NULL, ie=NULL, cb=NULL;
	string interfaceName;   
	tracein("interfaceDefinition");
	zzmatch(INTERFACE); labase++;
	 consume();
	zzmatch(IDENT); labase++;
	
	if ( !guessing ) {
		id = (ANTLRTokenPtr)LT(1);
}
	
	if ( !guessing ) {
	
	interfaceName= id->getText();
	}
 consume();
	interfaceExtends( interfaceName );
	classBlock( interfaceName );
	traceout("interfaceDefinition");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd4, 0x10);
	traceout("interfaceDefinition");
}

void
JParser::classBlock( const string& className )
{
	zzRULE;
	tracein("classBlock");
	zzmatch(LCURLY); labase++;
	 consume();
	{
		while ( 1 ) {
			if ( !((setwd4[LA(1)]&0x20))) break;
			if ( (setwd4[LA(1)]&0x40) ) {
				field( className );
			}
			else {
				if ( (LA(1)==SEMI)
 ) {
					zzmatch(SEMI); labase++;
					 consume();
				}
			}
		}
	}
	zzmatch(RCURLY); labase++;
	 consume();
	traceout("classBlock");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd4, 0x80);
	traceout("classBlock");
}

void
JParser::interfaceExtends( const string& className )
{
	zzRULE;
	
	int startLine=LT(1)->getLine(); 
	string parentName;
	tracein("interfaceExtends");
	{
		if ( (LA(1)==EXTENDS) ) {
			zzmatch(EXTENDS); labase++;
			 consume();
			identifier2( parentName );
			moreSuperclassNames( className,"extends" );
			if ( !guessing ) {
			
			ps->record_userel_extent(
			startLine,startLine,
			className,"",parentName,"extends",
			// no private inheritance in Java
			vPUBLIC, utINHERITS);
			}
		}
	}
	traceout("interfaceExtends");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd5, 0x1);
	traceout("interfaceExtends");
}

void
JParser::implementsClause( const std::string& className )
{
	zzRULE;
	
	int startLine=LT(1)->getLine(); 
	string parentName;
	tracein("implementsClause");
	{
		if ( (LA(1)==IMPLEMENTS) ) {
			zzmatch(IMPLEMENTS); labase++;
			 consume();
			identifier2( parentName );
			moreSuperclassNames( className,"implements" );
		}
	}
	if ( !guessing ) {
	
	ps->record_userel_extent(
	startLine,startLine,
	className,"",parentName,"implements",
	// no private inheritance in Java
	vPUBLIC, utINHERITS);
	}
	traceout("implementsClause");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd5, 0x2);
	traceout("implementsClause");
}

void
JParser::moreSuperclassNames( const string& className, const string& inheritType )
{
	zzRULE;
	
	int startLine=LT(1)->getLine(); 
	string parentName;
	tracein("moreSuperclassNames");
	{
		while ( (LA(1)==COMMA) ) {
			zzmatch(COMMA); labase++;
			 consume();
			identifier2( parentName );
			if ( !guessing ) {
			
			ps->record_userel_extent(
			startLine,startLine,
			className,"",parentName,inheritType,
			// no private inheritance in Java
			vPUBLIC, utINHERITS);
			}
		}
	}
	traceout("moreSuperclassNames");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd5, 0x4);
	traceout("moreSuperclassNames");
}

void
JParser::field( const string& className )
{
	zzRULE;
	ANTLRTokenPtr mods=NULL, cd=NULL, id=NULL, t=NULL, s3=NULL;
	
	string scope=className, typeName; 
	bool isBuiltIn;
	Visibility v=vDONTKNOW;
	int startLine=LT(1)->getLine(); 
	tracein("field");
	if ( (setwd5[LA(1)]&0x8) && (setwd5[LA(2)]&0x10) && !((LA(1)==STATIC&&
(LA(2)==LCURLY))) ) {
		modifiers( v );
		{
			if ( (LA(1)==IDENT) && (LA(2)==LPAREN) ) {
				ctorDefinition( className,v );
			}
			else {
				if ( (LA(1)==CLASS) ) {
					classDefinition( scope );
				}
				else {
					if ( (LA(1)==INTERFACE) ) {
						interfaceDefinition( scope );
					}
					else {
						if ( (setwd5[LA(1)]&0x20) && (setwd5[LA(2)]&0x40) ) {
							typeSpec2( typeName,isBuiltIn );
							typedDeclaration( className,typeName,isBuiltIn,v,startLine );
						}
						else {FAIL(2,err11,err12,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
					}
				}
			}
		}
	}
	else {
		if ( (LA(1)==STATIC) && 
(LA(2)==LCURLY) ) {
			zzmatch(STATIC); labase++;
			 consume();
			compoundStatement();
		}
		else {
			if ( (LA(1)==LCURLY) ) {
				compoundStatement();
			}
			else {FAIL(2,err13,err14,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
		}
	}
	traceout("field");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd5, 0x80);
	traceout("field");
}

void
JParser::typedDeclaration( const string& className,const string& typeName,
	 bool isBuiltIn, Visibility v, int startLine )
{
	zzRULE;
	tracein("typedDeclaration");
	if ( (LA(1)==IDENT) && (LA(2)==LPAREN) ) {
		methodDefinition( className,typeName,isBuiltIn,v,startLine );
	}
	else {
		if ( (LA(1)==IDENT) && (setwd6[LA(2)]&0x1) ) {
			variableDefinitions( className,typeName,isBuiltIn,v,startLine );
			zzmatch(SEMI); labase++;
			 consume();
		}
		else {FAIL(2,err15,err16,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
	}
	traceout("typedDeclaration");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd6, 0x2);
	traceout("typedDeclaration");
}

void
JParser::methodDefinition( const string& className, const string& returnType, bool rtIsBuiltIn,
	Visibility v, int startLine )
{
	zzRULE;
	ANTLRTokenPtr id=NULL, param=NULL, rt=NULL, tc=NULL, s2=NULL;
	
	string paramList; 
	bool isDefinition=false;
	tracein("methodDefinition");
	zzmatch(IDENT); labase++;
	
	if ( !guessing ) {
		id = (ANTLRTokenPtr)LT(1);
}
	 consume();
	zzmatch(LPAREN); labase++;
	 consume();
	parameterDeclarationList( paramList,className,v );
	zzmatch(RPAREN); labase++;
	 consume();
	returnTypeBrackersOnEndOfMethodHead();
	{
		if ( (LA(1)==THROWS) ) {
			throwsClause();
		}
	}
	{
		if ( (LA(1)==LCURLY)
 ) {
			compoundStatement();
			if ( !guessing ) {
			isDefinition=true;   
			}
		}
		else {
			if ( (LA(1)==SEMI) ) {
				zzmatch(SEMI); labase++;
				 consume();
			}
			else {FAIL(1,err17,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
		}
	}
	if ( !guessing ) {
	
	int endLine=LT(1)->getLine();
	string methodName= id->getText();
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

	  
	}
	traceout("methodDefinition");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd6, 0x4);
	traceout("methodDefinition");
}

void
JParser::variableDefinitions( const string& className, const string& returnType, bool rtIsBuiltIn,
	Visibility v, int startLine )
{
	zzRULE;
	tracein("variableDefinitions");
	variableDeclarator();
	{
		while ( (LA(1)==COMMA) ) {
			zzmatch(COMMA); labase++;
			 consume();
			variableDeclarator();
		}
	}
	if ( !guessing ) {
	
	if(rtIsBuiltIn==false)
	{
		// In Java, all containment relationships are effectively by
		// reference (I think).
		ps->record_userel_extent(startLine,startLine,
		className,"",returnType,
		"member variable",v,utHASBYREF);
	}
	}
	traceout("variableDefinitions");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd6, 0x8);
	traceout("variableDefinitions");
}

void
JParser::variableDeclarator(void)
{
	zzRULE;
	ANTLRTokenPtr id=NULL, d=NULL, v=NULL;
	tracein("variableDeclarator");
	zzmatch(IDENT); labase++;
	
	if ( !guessing ) {
		id = (ANTLRTokenPtr)LT(1);
}
	 consume();
	declaratorBrackets();
	varInitializer();
	traceout("variableDeclarator");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd6, 0x10);
	traceout("variableDeclarator");
}

void
JParser::declaratorBrackets(void)
{
	zzRULE;
	ANTLRTokenPtr lb=NULL;
	tracein("declaratorBrackets");
	{
		while ( (LA(1)==LBRACK) ) {
			zzmatch(LBRACK); labase++;
			
			if ( !guessing ) {
						lb = (ANTLRTokenPtr)LT(1);
}
			 consume();
			zzmatch(RBRACK); labase++;
			 consume();
		}
	}
	traceout("declaratorBrackets");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd6, 0x20);
	traceout("declaratorBrackets");
}

void
JParser::varInitializer(void)
{
	zzRULE;
	tracein("varInitializer");
	{
		if ( (LA(1)==ASSIGN) ) {
			zzmatch(ASSIGN); labase++;
			 consume();
			initializer();
		}
	}
	traceout("varInitializer");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd6, 0x40);
	traceout("varInitializer");
}

void
JParser::arrayInitializer(void)
{
	zzRULE;
	ANTLRTokenPtr lc=NULL;
	tracein("arrayInitializer");
	zzmatch(LCURLY); labase++;
	
	if ( !guessing ) {
		lc = (ANTLRTokenPtr)LT(1);
}
	 consume();
	{
		if ( (setwd6[LA(1)]&0x80)
 ) {
			initializer();
			{
				while ( (LA(1)==COMMA) && (setwd7[LA(2)]&0x1) ) {
					zzmatch(COMMA); labase++;
					 consume();
					initializer();
				}
			}
			{
				if ( (LA(1)==COMMA) ) {
					zzmatch(COMMA); labase++;
					 consume();
				}
			}
		}
	}
	zzmatch(RCURLY); labase++;
	 consume();
	traceout("arrayInitializer");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd7, 0x2);
	traceout("arrayInitializer");
}

void
JParser::initializer(void)
{
	zzRULE;
	tracein("initializer");
	if ( (setwd7[LA(1)]&0x4) ) {
		expression();
	}
	else {
		if ( (LA(1)==LCURLY) ) {
			arrayInitializer();
		}
		else {FAIL(1,err18,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
	}
	traceout("initializer");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd7, 0x8);
	traceout("initializer");
}

void
JParser::ctorDefinition( const string& className, Visibility v )
{
	zzRULE;
	
	// The following assertion seemed like a good idea
	// at the time, but it fails for nested classes.
	// assert(className==LT(1)->getText());
	int startLine=LT(1)->getLine();
	string paramList;
	tracein("ctorDefinition");
	ctorHead( paramList,className,v );
	compoundStatement();
	if ( !guessing ) {
	
	int endLine=LT(1)->getLine();
	ps->record_function_extent(startLine,endLine,
	"",className,className,paramList,
	"definition",v,utDEFINITION);
	}
	traceout("ctorDefinition");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd7, 0x10);
	traceout("ctorDefinition");
}

void
JParser::ctorHead( string& paramList,const string& className, Visibility v )
{
	zzRULE;
	tracein("ctorHead");
	zzmatch(IDENT); labase++;
	 consume();
	zzmatch(LPAREN); labase++;
	 consume();
	parameterDeclarationList( paramList,className,v );
	zzmatch(RPAREN); labase++;
	 consume();
	{
		if ( (LA(1)==THROWS)
 ) {
			throwsClause();
		}
	}
	traceout("ctorHead");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd7, 0x20);
	traceout("ctorHead");
}

void
JParser::throwsClause(void)
{
	zzRULE;
	tracein("throwsClause");
	zzmatch(THROWS); labase++;
	 consume();
	identifier();
	{
		while ( (LA(1)==COMMA) ) {
			zzmatch(COMMA); labase++;
			 consume();
			identifier();
		}
	}
	traceout("throwsClause");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd7, 0x40);
	traceout("throwsClause");
}

void
JParser::returnTypeBrackersOnEndOfMethodHead(void)
{
	zzRULE;
	tracein("returnTypeBrackersOnEndOfMethodHead");
	{
		while ( (LA(1)==LBRACK) ) {
			zzmatch(LBRACK); labase++;
			 consume();
			zzmatch(RBRACK); labase++;
			 consume();
		}
	}
	traceout("returnTypeBrackersOnEndOfMethodHead");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd7, 0x80);
	traceout("returnTypeBrackersOnEndOfMethodHead");
}

void
JParser::parameterDeclarationList( 
		// a string in which we accumulate a string of all parameters
		string& paramList, 
		// the name of the owner class for the method
	 	const string& clientName, 
		// visibility of the method
	 	const Visibility& v      
	 )
{
	zzRULE;
	string paramItem;   
	tracein("parameterDeclarationList");
	if ( !guessing ) {
	paramList+="(";   
	}
	{
		if ( (setwd8[LA(1)]&0x1) ) {
			parameterDeclaration( paramList,clientName,v,true );
			{
				while ( (LA(1)==COMMA) ) {
					zzmatch(COMMA); labase++;
					
					if ( !guessing ) {
					paramList+=",";   
					}
 consume();
					parameterDeclaration( paramList,clientName,v,true );
				}
			}
		}
	}
	if ( !guessing ) {
	paramList+=")";   
	}
	traceout("parameterDeclarationList");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd8, 0x2);
	traceout("parameterDeclarationList");
}

void
JParser::parameterDeclaration( 
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
	 )
{
	zzRULE;
	ANTLRTokenPtr pm=NULL, t=NULL, id=NULL, pd=NULL;
	
	string mod, typeName, bracks; bool isBuiltIn; 
	int startLine=LT(1)->getLine();
	tracein("parameterDeclaration");
	parameterModifier( mod );
	typeSpec2( typeName,isBuiltIn );
	zzmatch(IDENT); labase++;
	
	if ( !guessing ) {
		id = (ANTLRTokenPtr)LT(1);
}
	 consume();
	parameterDeclaratorBrackets( bracks );
	if ( !guessing ) {
	
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
	}
	traceout("parameterDeclaration");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd8, 0x4);
	traceout("parameterDeclaration");
}

void
JParser::parameterDeclaratorBrackets( string& bracks )
{
	zzRULE;
	tracein("parameterDeclaratorBrackets");
	{
		while ( (LA(1)==LBRACK)
 ) {
			zzmatch(LBRACK); labase++;
			 consume();
			zzmatch(RBRACK); labase++;
			
			if ( !guessing ) {
			bracks+="[]";   
			}
 consume();
		}
	}
	traceout("parameterDeclaratorBrackets");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd8, 0x8);
	traceout("parameterDeclaratorBrackets");
}

void
JParser::parameterModifier( string& paramList )
{
	zzRULE;
	ANTLRTokenPtr f=NULL;
	tracein("parameterModifier");
	{
		if ( (LA(1)==FINAL) ) {
			zzmatch(FINAL); labase++;
			
			if ( !guessing ) {
						f = (ANTLRTokenPtr)LT(1);
}
			
			if ( !guessing ) {
			paramList+= f->getText();   
			}
 consume();
		}
	}
	traceout("parameterModifier");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd8, 0x10);
	traceout("parameterModifier");
}

void
JParser::compoundStatement(void)
{
	zzRULE;
	tracein("compoundStatement");
	zzmatch(LCURLY); labase++;
	 consume();
	{
		while ( (setwd8[LA(1)]&0x20) ) {
			statement();
		}
	}
	zzmatch(RCURLY); labase++;
	 consume();
	traceout("compoundStatement");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd8, 0x40);
	traceout("compoundStatement");
}

void
JParser::statement(void)
{
	zzRULE;
	ANTLRTokenPtr c=NULL;
	zzGUESS_BLOCK
	string scope;   
	tracein("statement");
	if ( (LA(1)==IF) ) {
		ifStatement();
	}
	else {
		if ( (LA(1)==FOR) ) {
			forStatement();
		}
		else {
			if ( (LA(1)==WHILE)
 ) {
				whileStatement();
			}
			else {
				if ( (LA(1)==DO) ) {
					doWhileStatement();
				}
				else {
					if ( (LA(1)==BREAK) ) {
						breakStatement();
					}
					else {
						if ( (LA(1)==CONTINUE) ) {
							continueStatement();
						}
						else {
							if ( (LA(1)==RETURN) ) {
								returnStatement();
							}
							else {
								if ( (LA(1)==SWITCH)
 ) {
									switchStatement();
								}
								else {
									if ( (LA(1)==THROW) ) {
										throwStatement();
									}
									else {
										if ( (LA(1)==TRY) ) {
											tryBlock();
										}
										else {
											if ( (LA(1)==SYNCHRONIZED) && (LA(2)==LPAREN) ) {
												syncStatement();
											}
											else {
												if ( (LA(1)==SEMI) ) {
													emptyStatement();
												}
												else {
													if ( (LA(1)==LCURLY)
 ) {
														compoundStatement();
													}
													else {
														if ( (LA(1)==CLASS) ) {
															classDefinition( scope );
														}
														else {
															if ( (LA(1)==FINAL) && (LA(2)==CLASS) ) {
																zzmatch(FINAL); labase++;
																 consume();
																classDefinition( scope );
															}
															else {
																if ( (LA(1)==ABSTRACT) && (LA(2)==CLASS) ) {
																	zzmatch(ABSTRACT); labase++;
																	 consume();
																	classDefinition( scope );
																}
																else {
																	zzGUESS
																	if ( !zzrv && (setwd8[LA(1)]&0x80) && (setwd9[LA(2)]&0x1) ) {
																		{
																			declaration();
																		}
																		zzGUESS_DONE
																		declaration();
																		zzmatch(SEMI); labase++;
																		 consume();
																	}
																	else {
																		if ( !zzrv ) zzGUESS_DONE;
																		if ( (setwd9[LA(1)]&0x2) && 
(setwd9[LA(2)]&0x4) && !((LA(1)==IDENT&&(LA(2)==COLON))) ) {
																			expression();
																			zzmatch(SEMI); labase++;
																			 consume();
																		}
																		else {
																			if ( !zzrv ) zzGUESS_DONE;
																			if ( (LA(1)==IDENT) && (LA(2)==COLON) ) {
																				zzmatch(IDENT); labase++;
																				 consume();
																				zzmatch(COLON); labase++;
																				
																				if ( !guessing ) {
																																								c = (ANTLRTokenPtr)LT(1);
}
																				 consume();
																				statement();
																			}
																			else {FAIL(2,err19,err20,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
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
	traceout("statement");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd9, 0x8);
	traceout("statement");
}

void
JParser::ifStatement(void)
{
	zzRULE;
	tracein("ifStatement");
	zzmatch(IF); labase++;
	 consume();
	zzmatch(LPAREN); labase++;
	 consume();
	expression();
	zzmatch(RPAREN); labase++;
	 consume();
	statement();
	optElseClause();
	traceout("ifStatement");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd9, 0x10);
	traceout("ifStatement");
}

void
JParser::forStatement(void)
{
	zzRULE;
	tracein("forStatement");
	zzmatch(FOR); labase++;
	 consume();
	zzmatch(LPAREN); labase++;
	 consume();
	forInit();
	zzmatch(SEMI); labase++;
	 consume();
	forCond();
	zzmatch(SEMI); labase++;
	 consume();
	forIter();
	zzmatch(RPAREN); labase++;
	 consume();
	statement();
	traceout("forStatement");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd9, 0x20);
	traceout("forStatement");
}

void
JParser::whileStatement(void)
{
	zzRULE;
	tracein("whileStatement");
	zzmatch(WHILE); labase++;
	 consume();
	zzmatch(LPAREN); labase++;
	 consume();
	expression();
	zzmatch(RPAREN); labase++;
	 consume();
	statement();
	traceout("whileStatement");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd9, 0x40);
	traceout("whileStatement");
}

void
JParser::doWhileStatement(void)
{
	zzRULE;
	tracein("doWhileStatement");
	zzmatch(DO); labase++;
	 consume();
	statement();
	zzmatch(WHILE); labase++;
	 consume();
	zzmatch(LPAREN); labase++;
	 consume();
	expression();
	zzmatch(RPAREN); labase++;
	 consume();
	zzmatch(SEMI); labase++;
	 consume();
	traceout("doWhileStatement");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd9, 0x80);
	traceout("doWhileStatement");
}

void
JParser::breakStatement(void)
{
	zzRULE;
	tracein("breakStatement");
	zzmatch(BREAK); labase++;
	 consume();
	{
		if ( (LA(1)==IDENT) ) {
			zzmatch(IDENT); labase++;
			 consume();
		}
	}
	zzmatch(SEMI); labase++;
	 consume();
	traceout("breakStatement");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd10, 0x1);
	traceout("breakStatement");
}

void
JParser::continueStatement(void)
{
	zzRULE;
	tracein("continueStatement");
	zzmatch(CONTINUE); labase++;
	 consume();
	{
		if ( (LA(1)==IDENT) ) {
			zzmatch(IDENT); labase++;
			 consume();
		}
	}
	zzmatch(SEMI); labase++;
	 consume();
	traceout("continueStatement");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd10, 0x2);
	traceout("continueStatement");
}

void
JParser::returnStatement(void)
{
	zzRULE;
	tracein("returnStatement");
	zzmatch(RETURN); labase++;
	 consume();
	{
		if ( (setwd10[LA(1)]&0x4)
 ) {
			expression();
		}
	}
	zzmatch(SEMI); labase++;
	 consume();
	traceout("returnStatement");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd10, 0x8);
	traceout("returnStatement");
}

void
JParser::switchStatement(void)
{
	zzRULE;
	tracein("switchStatement");
	zzmatch(SWITCH); labase++;
	 consume();
	zzmatch(LPAREN); labase++;
	 consume();
	expression();
	zzmatch(RPAREN); labase++;
	 consume();
	zzmatch(LCURLY); labase++;
	 consume();
	{
		while ( (setwd10[LA(1)]&0x10) ) {
			casesGroup();
		}
	}
	zzmatch(RCURLY); labase++;
	 consume();
	traceout("switchStatement");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd10, 0x20);
	traceout("switchStatement");
}

void
JParser::throwStatement(void)
{
	zzRULE;
	tracein("throwStatement");
	zzmatch(THROW); labase++;
	 consume();
	expression();
	zzmatch(SEMI); labase++;
	 consume();
	traceout("throwStatement");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd10, 0x40);
	traceout("throwStatement");
}

void
JParser::syncStatement(void)
{
	zzRULE;
	tracein("syncStatement");
	zzmatch(SYNCHRONIZED); labase++;
	 consume();
	zzmatch(LPAREN); labase++;
	 consume();
	expression();
	zzmatch(RPAREN); labase++;
	 consume();
	compoundStatement();
	traceout("syncStatement");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd10, 0x80);
	traceout("syncStatement");
}

void
JParser::emptyStatement(void)
{
	zzRULE;
	tracein("emptyStatement");
	zzmatch(SEMI); labase++;
	 consume();
	traceout("emptyStatement");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd11, 0x1);
	traceout("emptyStatement");
}

void
JParser::optElseClause(void)
{
	zzRULE;
	zzGUESS_BLOCK
	tracein("optElseClause");
	zzGUESS
	if ( !zzrv && (LA(1)==ELSE) && (setwd11[LA(2)]&0x2) ) {
		{
			zzmatch(ELSE); labase++;
			 consume();
		}
		zzGUESS_DONE
		zzmatch(ELSE); labase++;
		 consume();
		statement();
	}
	else {
		if ( !zzrv ) zzGUESS_DONE;
		if ( (setwd11[LA(1)]&0x4) && (setwd11[LA(2)]&0x8) ) {
		}
		else {FAIL(2,err21,err22,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
	}
	traceout("optElseClause");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd11, 0x10);
	traceout("optElseClause");
}

void
JParser::casesGroup(void)
{
	zzRULE;
	tracein("casesGroup");
	cases();
	caseSList();
	traceout("casesGroup");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd11, 0x20);
	traceout("casesGroup");
}

void
JParser::cases(void)
{
	zzRULE;
	tracein("cases");
	aCase();
	optMoreCases();
	traceout("cases");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd11, 0x40);
	traceout("cases");
}

void
JParser::optMoreCases(void)
{
	zzRULE;
	zzGUESS_BLOCK
	tracein("optMoreCases");
	zzGUESS
	if ( !zzrv && (setwd11[LA(1)]&0x80) && (setwd12[LA(2)]&0x1) ) {
		{
			if ( (LA(1)==CASE)
 ) {
				zzmatch(CASE); labase++;
				 consume();
			}
			else {
				if ( (LA(1)==DEFAULT) ) {
					zzmatch(DEFAULT); labase++;
					 consume();
				}
				else {FAIL(1,err23,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
			}
		}
		zzGUESS_DONE
		aCase();
		optMoreCases();
	}
	else {
		if ( !zzrv ) zzGUESS_DONE;
		if ( (setwd12[LA(1)]&0x2) && (setwd12[LA(2)]&0x4) ) {
		}
		else {FAIL(2,err24,err25,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
	}
	traceout("optMoreCases");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd12, 0x8);
	traceout("optMoreCases");
}

void
JParser::aCase(void)
{
	zzRULE;
	tracein("aCase");
	if ( (LA(1)==DEFAULT) ) {
		zzmatch(DEFAULT); labase++;
		 consume();
		zzmatch(COLON); labase++;
		 consume();
	}
	else {
		if ( (LA(1)==CASE) ) {
			zzmatch(CASE); labase++;
			 consume();
			expression();
			zzmatch(COLON); labase++;
			 consume();
		}
		else {FAIL(1,err26,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
	}
	traceout("aCase");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd12, 0x10);
	traceout("aCase");
}

void
JParser::caseSList(void)
{
	zzRULE;
	tracein("caseSList");
	{
		while ( (setwd12[LA(1)]&0x20)
 ) {
			statement();
		}
	}
	traceout("caseSList");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd12, 0x40);
	traceout("caseSList");
}

void
JParser::forInit(void)
{
	zzRULE;
	zzGUESS_BLOCK
	tracein("forInit");
	zzGUESS
	if ( !zzrv && (setwd12[LA(1)]&0x80) && (setwd13[LA(2)]&0x1) ) {
		{
			declaration();
		}
		zzGUESS_DONE
		{
			declaration();
		}
	}
	else {
		if ( !zzrv ) zzGUESS_DONE;
		if ( (setwd13[LA(1)]&0x2) && (setwd13[LA(2)]&0x4) ) {
			expressionList();
		}
		else {
			if ( !zzrv ) zzGUESS_DONE;
			if ( (LA(1)==SEMI) ) {
			}
			else {FAIL(2,err27,err28,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
		}
	}
	traceout("forInit");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd13, 0x8);
	traceout("forInit");
}

void
JParser::forCond(void)
{
	zzRULE;
	tracein("forCond");
	{
		if ( (setwd13[LA(1)]&0x10) ) {
			expression();
		}
	}
	traceout("forCond");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd13, 0x20);
	traceout("forCond");
}

void
JParser::forIter(void)
{
	zzRULE;
	tracein("forIter");
	{
		if ( (setwd13[LA(1)]&0x40)
 ) {
			expressionList();
		}
	}
	traceout("forIter");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd13, 0x80);
	traceout("forIter");
}

void
JParser::tryBlock(void)
{
	zzRULE;
	tracein("tryBlock");
	zzmatch(TRY); labase++;
	 consume();
	compoundStatement();
	{
		while ( (LA(1)==CATCH) ) {
			handler();
		}
	}
	{
		if ( (LA(1)==FINALLY) ) {
			zzmatch(FINALLY); labase++;
			 consume();
			compoundStatement();
		}
	}
	traceout("tryBlock");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd14, 0x1);
	traceout("tryBlock");
}

void
JParser::handler(void)
{
	zzRULE;
	Visibility dv;    
	tracein("handler");
	zzmatch(CATCH); labase++;
	 consume();
	zzmatch(LPAREN); labase++;
	 consume();
	parameterDeclaration( d1,d2,dv,false );
	zzmatch(RPAREN); labase++;
	 consume();
	compoundStatement();
	traceout("handler");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd14, 0x2);
	traceout("handler");
}

void
JParser::expression(void)
{
	zzRULE;
	tracein("expression");
	assignmentExpression();
	traceout("expression");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd14, 0x4);
	traceout("expression");
}

void
JParser::expressionList(void)
{
	zzRULE;
	tracein("expressionList");
	expression();
	{
		while ( (LA(1)==COMMA) ) {
			zzmatch(COMMA); labase++;
			 consume();
			expression();
		}
	}
	traceout("expressionList");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd14, 0x8);
	traceout("expressionList");
}

void
JParser::assignmentExpression(void)
{
	zzRULE;
	tracein("assignmentExpression");
	conditionalExpression();
	{
		if ( (setwd14[LA(1)]&0x10) ) {
			{
				if ( (LA(1)==ASSIGN)
 ) {
					zzmatch(ASSIGN); labase++;
					 consume();
				}
				else {
					if ( (LA(1)==PLUS_ASSIGN) ) {
						zzmatch(PLUS_ASSIGN); labase++;
						 consume();
					}
					else {
						if ( (LA(1)==MINUS_ASSIGN) ) {
							zzmatch(MINUS_ASSIGN); labase++;
							 consume();
						}
						else {
							if ( (LA(1)==STAR_ASSIGN) ) {
								zzmatch(STAR_ASSIGN); labase++;
								 consume();
							}
							else {
								if ( (LA(1)==DIV_ASSIGN) ) {
									zzmatch(DIV_ASSIGN); labase++;
									 consume();
								}
								else {
									if ( (LA(1)==MOD_ASSIGN)
 ) {
										zzmatch(MOD_ASSIGN); labase++;
										 consume();
									}
									else {
										if ( (LA(1)==SR_ASSIGN) ) {
											zzmatch(SR_ASSIGN); labase++;
											 consume();
										}
										else {
											if ( (LA(1)==BSR_ASSIGN) ) {
												zzmatch(BSR_ASSIGN); labase++;
												 consume();
											}
											else {
												if ( (LA(1)==SL_ASSIGN) ) {
													zzmatch(SL_ASSIGN); labase++;
													 consume();
												}
												else {
													if ( (LA(1)==BAND_ASSIGN) ) {
														zzmatch(BAND_ASSIGN); labase++;
														 consume();
													}
													else {
														if ( (LA(1)==BXOR_ASSIGN)
 ) {
															zzmatch(BXOR_ASSIGN); labase++;
															 consume();
														}
														else {
															if ( (LA(1)==BOR_ASSIGN) ) {
																zzmatch(BOR_ASSIGN); labase++;
																 consume();
															}
															else {FAIL(1,err29,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
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
			assignmentExpression();
		}
	}
	traceout("assignmentExpression");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd14, 0x20);
	traceout("assignmentExpression");
}

void
JParser::conditionalExpression(void)
{
	zzRULE;
	tracein("conditionalExpression");
	logicalOrExpression();
	{
		if ( (LA(1)==QUESTION) ) {
			zzmatch(QUESTION); labase++;
			 consume();
			conditionalExpression();
			zzmatch(COLON); labase++;
			 consume();
			conditionalExpression();
		}
	}
	traceout("conditionalExpression");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd14, 0x40);
	traceout("conditionalExpression");
}

void
JParser::logicalOrExpression(void)
{
	zzRULE;
	tracein("logicalOrExpression");
	logicalAndExpression();
	{
		while ( (LA(1)==LOR) ) {
			zzmatch(LOR); labase++;
			 consume();
			logicalAndExpression();
		}
	}
	traceout("logicalOrExpression");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd14, 0x80);
	traceout("logicalOrExpression");
}

void
JParser::logicalAndExpression(void)
{
	zzRULE;
	tracein("logicalAndExpression");
	inclusiveOrExpression();
	{
		while ( (LA(1)==LAND) ) {
			zzmatch(LAND); labase++;
			 consume();
			inclusiveOrExpression();
		}
	}
	traceout("logicalAndExpression");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd15, 0x1);
	traceout("logicalAndExpression");
}

void
JParser::inclusiveOrExpression(void)
{
	zzRULE;
	tracein("inclusiveOrExpression");
	exclusiveOrExpression();
	{
		while ( (LA(1)==BOR)
 ) {
			zzmatch(BOR); labase++;
			 consume();
			exclusiveOrExpression();
		}
	}
	traceout("inclusiveOrExpression");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd15, 0x2);
	traceout("inclusiveOrExpression");
}

void
JParser::exclusiveOrExpression(void)
{
	zzRULE;
	tracein("exclusiveOrExpression");
	andExpression();
	{
		while ( (LA(1)==BXOR) ) {
			zzmatch(BXOR); labase++;
			 consume();
			andExpression();
		}
	}
	traceout("exclusiveOrExpression");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd15, 0x4);
	traceout("exclusiveOrExpression");
}

void
JParser::andExpression(void)
{
	zzRULE;
	tracein("andExpression");
	equalityExpression();
	{
		while ( (LA(1)==BAND) ) {
			zzmatch(BAND); labase++;
			 consume();
			equalityExpression();
		}
	}
	traceout("andExpression");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd15, 0x8);
	traceout("andExpression");
}

void
JParser::equalityExpression(void)
{
	zzRULE;
	tracein("equalityExpression");
	relationalExpression();
	relationalPredicate();
	traceout("equalityExpression");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd15, 0x10);
	traceout("equalityExpression");
}

void
JParser::relationalPredicate(void)
{
	zzRULE;
	tracein("relationalPredicate");
	if ( (LA(1)==INSTANCEOF) ) {
		zzmatch(INSTANCEOF); labase++;
		 consume();
		typeSpec();
	}
	else {
		if ( (setwd15[LA(1)]&0x20) ) {
			{
				while ( (setwd15[LA(1)]&0x40)
 ) {
					{
						if ( (LA(1)==NOT_EQUAL) ) {
							zzmatch(NOT_EQUAL); labase++;
							 consume();
						}
						else {
							if ( (LA(1)==EQUAL) ) {
								zzmatch(EQUAL); labase++;
								 consume();
							}
							else {FAIL(1,err30,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
						}
					}
					relationalExpression();
				}
			}
		}
		else {FAIL(1,err31,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
	}
	traceout("relationalPredicate");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd15, 0x80);
	traceout("relationalPredicate");
}

void
JParser::relationalExpression(void)
{
	zzRULE;
	tracein("relationalExpression");
	shiftExpression();
	{
		while ( (setwd16[LA(1)]&0x1) ) {
			{
				if ( (LA(1)==LESSTHAN) ) {
					zzmatch(LESSTHAN); labase++;
					 consume();
				}
				else {
					if ( (LA(1)==GT)
 ) {
						zzmatch(GT); labase++;
						 consume();
					}
					else {
						if ( (LA(1)==LE) ) {
							zzmatch(LE); labase++;
							 consume();
						}
						else {
							if ( (LA(1)==GE) ) {
								zzmatch(GE); labase++;
								 consume();
							}
							else {FAIL(1,err32,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
						}
					}
				}
			}
			shiftExpression();
		}
	}
	traceout("relationalExpression");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd16, 0x2);
	traceout("relationalExpression");
}

void
JParser::shiftExpression(void)
{
	zzRULE;
	tracein("shiftExpression");
	additiveExpression();
	{
		while ( (setwd16[LA(1)]&0x4) ) {
			{
				if ( (LA(1)==SL) ) {
					zzmatch(SL); labase++;
					 consume();
				}
				else {
					if ( (LA(1)==SR)
 ) {
						zzmatch(SR); labase++;
						 consume();
					}
					else {
						if ( (LA(1)==BSR) ) {
							zzmatch(BSR); labase++;
							 consume();
						}
						else {FAIL(1,err33,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
					}
				}
			}
			additiveExpression();
		}
	}
	traceout("shiftExpression");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd16, 0x8);
	traceout("shiftExpression");
}

void
JParser::additiveExpression(void)
{
	zzRULE;
	tracein("additiveExpression");
	multiplicativeExpression();
	{
		while ( (setwd16[LA(1)]&0x10) ) {
			{
				if ( (LA(1)==PLUS) ) {
					zzmatch(PLUS); labase++;
					 consume();
				}
				else {
					if ( (LA(1)==MINUS) ) {
						zzmatch(MINUS); labase++;
						 consume();
					}
					else {FAIL(1,err34,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
				}
			}
			multiplicativeExpression();
		}
	}
	traceout("additiveExpression");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd16, 0x20);
	traceout("additiveExpression");
}

void
JParser::multiplicativeExpression(void)
{
	zzRULE;
	tracein("multiplicativeExpression");
	unaryExpression();
	{
		while ( (setwd16[LA(1)]&0x40)
 ) {
			{
				if ( (LA(1)==STAR) ) {
					zzmatch(STAR); labase++;
					 consume();
				}
				else {
					if ( (LA(1)==DIV) ) {
						zzmatch(DIV); labase++;
						 consume();
					}
					else {
						if ( (LA(1)==MOD) ) {
							zzmatch(MOD); labase++;
							 consume();
						}
						else {FAIL(1,err35,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
					}
				}
			}
			unaryExpression();
		}
	}
	traceout("multiplicativeExpression");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd16, 0x80);
	traceout("multiplicativeExpression");
}

void
JParser::unaryExpression(void)
{
	zzRULE;
	tracein("unaryExpression");
	if ( (LA(1)==INC) ) {
		zzmatch(INC); labase++;
		 consume();
		unaryExpression();
	}
	else {
		if ( (LA(1)==DEC)
 ) {
			zzmatch(DEC); labase++;
			 consume();
			unaryExpression();
		}
		else {
			if ( (LA(1)==MINUS) ) {
				zzmatch(MINUS); labase++;
				 consume();
				unaryExpression();
			}
			else {
				if ( (LA(1)==PLUS) ) {
					zzmatch(PLUS); labase++;
					 consume();
					unaryExpression();
				}
				else {
					if ( (setwd17[LA(1)]&0x1) ) {
						unaryExpressionNotPlusMinus();
					}
					else {FAIL(1,err36,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
				}
			}
		}
	}
	traceout("unaryExpression");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd17, 0x2);
	traceout("unaryExpression");
}

void
JParser::unaryExpressionNotPlusMinus(void)
{
	zzRULE;
	zzGUESS_BLOCK
	tracein("unaryExpressionNotPlusMinus");
	if ( (LA(1)==BNOT) ) {
		zzmatch(BNOT); labase++;
		 consume();
		unaryExpression();
	}
	else {
		if ( (LA(1)==LNOT)
 ) {
			zzmatch(LNOT); labase++;
			 consume();
			unaryExpression();
		}
		else {
			zzGUESS
			if ( !zzrv && (LA(1)==LPAREN) && (setwd17[LA(2)]&0x4) ) {
				{
					zzmatch(LPAREN); labase++;
					 consume();
					builtInTypeSpec();
					zzmatch(RPAREN); labase++;
					 consume();
					unaryExpression();
				}
				zzGUESS_DONE
				{
					zzmatch(LPAREN); labase++;
					 consume();
					builtInTypeSpec();
					zzmatch(RPAREN); labase++;
					 consume();
					unaryExpression();
				}
			}
			else {
				if ( !zzrv ) zzGUESS_DONE;
				zzGUESS
				if ( !zzrv && (LA(1)==LPAREN) && (LA(2)==IDENT) ) {
					{
						zzmatch(LPAREN); labase++;
						 consume();
						classTypeSpec();
						zzmatch(RPAREN); labase++;
						 consume();
						unaryExpressionNotPlusMinus();
					}
					zzGUESS_DONE
					{
						zzmatch(LPAREN); labase++;
						 consume();
						classTypeSpec();
						zzmatch(RPAREN); labase++;
						 consume();
						unaryExpressionNotPlusMinus();
					}
				}
				else {
					if ( !zzrv ) zzGUESS_DONE;
					if ( (setwd17[LA(1)]&0x8) && (setwd17[LA(2)]&0x10) ) {
						postfixExpression();
					}
					else {FAIL(2,err37,err38,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
				}
			}
		}
	}
	traceout("unaryExpressionNotPlusMinus");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd17, 0x20);
	traceout("unaryExpressionNotPlusMinus");
}

void
JParser::postfixExpression(void)
{
	zzRULE;
	tracein("postfixExpression");
	if ( (setwd17[LA(1)]&0x40) ) {
		primaryExpression();
		{
			while ( 1 ) {
				if ( !((setwd17[LA(1)]&0x80)
)) break;
				if ( (LA(1)==DOT) ) {
					zzmatch(DOT); labase++;
					 consume();
					{
						if ( (LA(1)==IDENT) ) {
							zzmatch(IDENT); labase++;
							 consume();
						}
						else {
							if ( (LA(1)==KW_THIS) ) {
								zzmatch(KW_THIS); labase++;
								 consume();
							}
							else {
								if ( (LA(1)==CLASS) ) {
									zzmatch(CLASS); labase++;
									 consume();
								}
								else {
									if ( (LA(1)==NEW)
 ) {
										newExpression();
									}
									else {
										if ( (LA(1)==SUPER) ) {
											zzmatch(SUPER); labase++;
											 consume();
											zzmatch(LPAREN); labase++;
											 consume();
											{
												if ( (setwd18[LA(1)]&0x1) ) {
													expressionList();
												}
											}
											zzmatch(RPAREN); labase++;
											 consume();
										}
										else {FAIL(1,err39,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
									}
								}
							}
						}
					}
				}
				else {
					if ( (LA(1)==LBRACK) && (LA(2)==RBRACK) ) {
						{
							int zzcnt=1;
							do {
								zzmatch(LBRACK); labase++;
								 consume();
								zzmatch(RBRACK); labase++;
								 consume();
							} while ( (LA(1)==LBRACK) );
						}
						zzmatch(DOT); labase++;
						 consume();
						zzmatch(CLASS); labase++;
						 consume();
					}
					else {
						if ( (LA(1)==LBRACK) && 
(setwd18[LA(2)]&0x2) ) {
							zzmatch(LBRACK); labase++;
							 consume();
							expression();
							zzmatch(RBRACK); labase++;
							 consume();
						}
						else {
							if ( (LA(1)==LPAREN) ) {
								zzmatch(LPAREN); labase++;
								 consume();
								argList();
								zzmatch(RPAREN); labase++;
								 consume();
							}
						}
					}
				}
			}
		}
		{
			if ( (LA(1)==INC) ) {
				zzmatch(INC); labase++;
				 consume();
			}
			else {
				if ( (LA(1)==DEC) ) {
					zzmatch(DEC); labase++;
					 consume();
				}
				else {
					if ( (setwd18[LA(1)]&0x4) ) {
					}
					else {FAIL(1,err40,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
				}
			}
		}
	}
	else {
		if ( (setwd18[LA(1)]&0x8)
 ) {
			builtInType();
			{
				while ( (LA(1)==LBRACK) ) {
					zzmatch(LBRACK); labase++;
					 consume();
					zzmatch(RBRACK); labase++;
					 consume();
				}
			}
			zzmatch(DOT); labase++;
			 consume();
			zzmatch(CLASS); labase++;
			 consume();
		}
		else {FAIL(1,err41,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
	}
	traceout("postfixExpression");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd18, 0x10);
	traceout("postfixExpression");
}

void
JParser::primaryExpression(void)
{
	zzRULE;
	tracein("primaryExpression");
	if ( (LA(1)==IDENT) ) {
		zzmatch(IDENT); labase++;
		 consume();
	}
	else {
		if ( (LA(1)==NEW) ) {
			newExpression();
		}
		else {
			if ( (setwd18[LA(1)]&0x20) ) {
				constant();
			}
			else {
				if ( (LA(1)==SUPER)
 ) {
					zzmatch(SUPER); labase++;
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
							if ( (LA(1)==KW_THIS) ) {
								zzmatch(KW_THIS); labase++;
								 consume();
							}
							else {
								if ( (LA(1)==PNULL) ) {
									zzmatch(PNULL); labase++;
									 consume();
								}
								else {
									if ( (LA(1)==LPAREN)
 ) {
										zzmatch(LPAREN); labase++;
										 consume();
										assignmentExpression();
										zzmatch(RPAREN); labase++;
										 consume();
									}
									else {FAIL(1,err42,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
								}
							}
						}
					}
				}
			}
		}
	}
	traceout("primaryExpression");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd18, 0x40);
	traceout("primaryExpression");
}

void
JParser::newExpression(void)
{
	zzRULE;
	tracein("newExpression");
	zzmatch(NEW); labase++;
	 consume();
	type();
	{
		if ( (LA(1)==LPAREN) ) {
			zzmatch(LPAREN); labase++;
			 consume();
			argList();
			zzmatch(RPAREN); labase++;
			 consume();
			{
				if ( (LA(1)==LCURLY) ) {
					classBlock( d1 );
				}
			}
		}
		else {
			if ( (LA(1)==LBRACK) ) {
				newArrayDeclarator();
				{
					if ( (LA(1)==LCURLY) ) {
						arrayInitializer();
					}
				}
			}
			else {FAIL(1,err43,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
		}
	}
	traceout("newExpression");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd18, 0x80);
	traceout("newExpression");
}

void
JParser::argList(void)
{
	zzRULE;
	tracein("argList");
	{
		if ( (setwd19[LA(1)]&0x1)
 ) {
			expressionList();
		}
		else {
			if ( (LA(1)==RPAREN) ) {
			}
			else {FAIL(1,err44,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
		}
	}
	traceout("argList");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd19, 0x2);
	traceout("argList");
}

void
JParser::newArrayDeclarator(void)
{
	zzRULE;
	zzGUESS_BLOCK
	tracein("newArrayDeclarator");
	zzGUESS
	if ( !zzrv && (LA(1)==LBRACK) && (setwd19[LA(2)]&0x4) ) {
		{
			zzmatch(LBRACK); labase++;
			 consume();
			{
				if ( (setwd19[LA(1)]&0x8) ) {
					expression();
				}
			}
			zzmatch(RBRACK); labase++;
			 consume();
			zzmatch(LBRACK); labase++;
			 consume();
		}
		zzGUESS_DONE
		zzmatch(LBRACK); labase++;
		 consume();
		{
			if ( (setwd19[LA(1)]&0x10) ) {
				expression();
			}
		}
		zzmatch(RBRACK); labase++;
		 consume();
		newArrayDeclarator();
	}
	else {
		if ( !zzrv ) zzGUESS_DONE;
		if ( (LA(1)==LBRACK) && 
(setwd19[LA(2)]&0x20) ) {
			zzmatch(LBRACK); labase++;
			 consume();
			{
				if ( (setwd19[LA(1)]&0x40) ) {
					expression();
				}
			}
			zzmatch(RBRACK); labase++;
			 consume();
		}
		else {FAIL(2,err45,err46,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
	}
	traceout("newArrayDeclarator");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd19, 0x80);
	traceout("newArrayDeclarator");
}

void
JParser::constant(void)
{
	zzRULE;
	tracein("constant");
	if ( (LA(1)==NUM_INT1) ) {
		zzmatch(NUM_INT1); labase++;
		 consume();
	}
	else {
		if ( (LA(1)==NUM_INT1A) ) {
			zzmatch(NUM_INT1A); labase++;
			 consume();
		}
		else {
			if ( (LA(1)==NUM_INT1B) ) {
				zzmatch(NUM_INT1B); labase++;
				 consume();
			}
			else {
				if ( (LA(1)==NUM_INT2)
 ) {
					zzmatch(NUM_INT2); labase++;
					 consume();
				}
				else {
					if ( (LA(1)==NUM_INT3) ) {
						zzmatch(NUM_INT3); labase++;
						 consume();
					}
					else {
						if ( (LA(1)==NUM_INT4) ) {
							zzmatch(NUM_INT4); labase++;
							 consume();
						}
						else {
							if ( (LA(1)==CHARCONST) ) {
								zzmatch(CHARCONST); labase++;
								 consume();
							}
							else {
								if ( (LA(1)==STRINGCONST) ) {
									zzmatch(STRINGCONST); labase++;
									 consume();
								}
								else {
									if ( (LA(1)==NUM_FLOAT)
 ) {
										zzmatch(NUM_FLOAT); labase++;
										 consume();
									}
									else {FAIL(1,err47,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
								}
							}
						}
					}
				}
			}
		}
	}
	traceout("constant");
	return;
fail:
	if ( guessing ) zzGUESS_FAIL;
	syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
	resynch(setwd20, 0x1);
	traceout("constant");
}
