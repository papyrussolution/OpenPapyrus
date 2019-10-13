/*
	soapcpp2_yacc.y

	Yacc/Bison grammar.

	Build notes:

	1. Bison 1.6 is known to crash on Win32 systems if YYINITDEPTH is too
	small Compile with -DYYINITDEPTH=5000

	2. This grammar has one shift/reduce conflict related to the use of a
	class declaration with a base class (e.g. class Y : public X) and the
	use of a maxOccurs (class Y :10). Internally the conflict is resolved
	in favor of a shift by Bison/Yacc, which leads to the correct parsing
	behavior. Therefore, the warning can be ignored. If this leads to an
	error, then please enable the following directive (around line 121):

%expect 1 // Bison: ignore one shift/reduce conflict

--------------------------------------------------------------------------------
gSOAP XML Web services tools
Copyright (C) 2000-2011, Robert van Engelen, Genivia Inc. All Rights Reserved.
This part of the software is released under ONE of the following licenses:
GPL OR Genivia's license for commercial use.
--------------------------------------------------------------------------------
GPL license.

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation; either version 2 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program; if not, write to the Free Software Foundation, Inc., 59 Temple
Place, Suite 330, Boston, MA 02111-1307 USA

Author contact information:
engelen@genivia.com / engelen@acm.org

This program is released under the GPL with the additional exemption that
compiling, linking, and/or using OpenSSL is allowed.
--------------------------------------------------------------------------------
A commercial use license is available from Genivia, Inc., contact@genivia.com
--------------------------------------------------------------------------------
*/

%{

#include "soapcpp2.h"

#ifdef WIN32
	/* @sobolev
	#ifndef __STDC__
		#define __STDC__
	#endif
	*/
	#define YYINCLUDED_STDLIB_H
	#ifdef WIN32_WITHOUT_SOLARIS_FLEX
		extern int soapcpp2lex(void);
	#else
		extern int yylex(void);
	#endif
#else
	extern int yylex(void);
#endif

extern int is_XML(Tnode*);

#define MAXNEST 16	/* max. nesting depth of scopes */

struct Scope {
	Table	*table;
	Entry	*entry;
	Node	node;
	LONG64	val;
	int	offset;
	Bool	grow;	/* true if offset grows with declarations */
	Bool	mask;	/* true if enum is mask */
} stack[MAXNEST],	/* stack of tables and offsets */
	*sp;		/* current scope stack pointer */

Table * classtable = (Table*)0;
Table * enumtable = (Table*)0;
Table * typetable = (Table*)0;
Table * booltable = (Table*)0;
Table * templatetable = (Table*)0;

char * namespaceid = NULL;
int	transient = 0;
int	permission = 0;
int	custom_header = 1;
int	custom_fault = 1;
Pragma	*pragmas = NULL;
Tnode	*qname = NULL;
Tnode	*xml = NULL;

/* function prototypes for support routine section */
static Entry	*undefined(Symbol*);
static Tnode	*mgtype(Tnode*, Tnode*);
static Node	op(const char*, Node, Node), iop(const char*, Node, Node), relop(const char*, Node, Node);
static void	mkscope(Table*, int), enterscope(Table*, int), exitscope(void);
static int	integer(Tnode*), real(Tnode*), numeric(Tnode*);
static void	add_soap(void), add_XML(void), add_qname(void), add_header(Table*), add_fault(Table*), add_response(Entry*, Entry*), add_result(Tnode*);

char * c_storage(/*Storage*/int);
char * c_type(Tnode*);
char * c_ident(Tnode*);
int    is_primitive_or_string(Tnode*);
int    is_stdstr(Tnode*);
int    is_binary(Tnode*);
int    is_external(Tnode*);
int    is_mutable(Tnode*);

/* Temporaries used in semantic rules */
int	i;
char	*s, *s1, *s2;
Symbol	*sym;
Entry	*p, *q;
Tnode	*t;
Node	tmp, c;
Pragma	**pp;

int    yylineno; // @sobolev
//int    yywrap;   // @sobolev

%}

/* We expect one shift-reduce conflict, see build notes in the header above */
/* %expect 1 */ /* directive is not compatible with Yacc */
/* If Yacc complains then remove the line above to allow Yacc to proceed */

%union {
	Symbol	*sym;
	LONG64	i;
	double	r;
	char	c;
	char	*s;
	Tnode	*typ;
	Storage	sto;
	Node	rec;
	Entry	*e;
}

/* pragmas */
%token	<s> PRAGMA
/* keywords */
%token	<sym> AUTO     DOUBLE  INT       STRUCT
%token	<sym> BREAK    ELSE    LONG      SWITCH
%token	<sym> CASE     ENUM    REGISTER  TYPEDEF
%token	<sym> CHAR     EXTERN  RETURN    UNION
%token	<sym> CONST    FLOAT   SHORT     UNSIGNED
%token	<sym> CONTINUE FOR     SIGNED    VOID
%token	<sym> DEFAULT  GOTO    SIZEOF    VOLATILE
%token	<sym> DO       IF      STATIC    WHILE
%token	<sym> CLASS    PRIVATE PROTECTED PUBLIC
%token	<sym> VIRTUAL  INLINE  OPERATOR  LLONG
%token	<sym> BOOL     CFALSE  CTRUE	 WCHAR
%token	<sym> TIME     USING   NAMESPACE ULLONG
%token	<sym> MUSTUNDERSTAND   SIZE      FRIEND
%token	<sym> TEMPLATE EXPLICIT		 TYPENAME
%token	<sym> RESTRICT null
%token	<sym> UCHAR    USHORT  UINT      ULONG
/* */
%token	NONE
/* identifiers (TYPE = typedef identifier) */
%token	<sym> ID TAG LAB TYPE
/* constants */
%token	<i> LNG
%token	<r> DBL
%token	<c> CHR
%token	<s> STR
/* types and related */
%type	<typ> type
%type	<sto> store virtual constobj abstract
%type	<e> fname struct class base enum
%type	<sym> id tag arg name
%type	<s> patt
%type	<i> cint
/* expressions and statements */
%type	<rec> expr cexp oexp obex aexp abex rexp lexp pexp init spec tspec ptrs array arrayck texp qexp occurs
/* terminals */
%left	','
%right	'=' PA NA TA DA MA AA XA OA LA RA  /* += -= *= /= %= &= ^= |= <<= >>= */
%right	'?'
%right	':'
%left	OR		/* || */
%left	AN		/* && */
%left	'|'
%left	'^'
%left	'&'
%left	EQ NE		/* == != */
%left	'<' LE '>' GE	/* <= >= */
%left	LS RS		/* << >> */
%left	'+' '-'
%left	'*' '/' '%'
%left	AR		/* -> */
%token	PP NN		/* ++ -- */

%%

/******************************************************************************\

	Program syntax

\******************************************************************************/

prog	: s1 exts	{ if (lflag)
    			  {	custom_header = 0;
    			  	custom_fault = 0;
			  }
			  else
			  {	add_header(sp->table);
			  	add_fault(sp->table);
			  }
			  compile(sp->table);
			  freetable(classtable);
			  freetable(enumtable);
			  freetable(typetable);
			  freetable(booltable);
			  freetable(templatetable);
			}
	;
s1	: /* empty */	{ classtable = mktable((Table*)0);
			  enumtable = mktable((Table*)0);
			  typetable = mktable((Table*)0);
			  booltable = mktable((Table*)0);
			  templatetable = mktable((Table*)0);
			  p = enter(booltable, lookup("false"));
			  p->info.typ = mkint();
			  p->info.val.i = 0;
			  p = enter(booltable, lookup("true"));
			  p->info.typ = mkint();
			  p->info.val.i = 1;
			  mkscope(mktable(mktable((Table*)0)), 0);
			}
	;
exts	: NAMESPACE ID '{' exts1 '}'
			{ namespaceid = $2->name; }
	| exts1		{ }
	;
exts1	: /* empty */	{ add_soap();
			  add_qname();
			  add_XML();
			}
	| exts1 ext	{ }
	;
ext	: dclrs ';'	{ }
	| pragma	{ }
	| error ';'	{ synerror("input before ; skipped");
			  while (sp > stack)
			  {	freetable(sp->table);
			  	exitscope();
			  }
			  yyerrok;
			}
	| t1		{ }
	| t2		{ }
	;
pragma	: PRAGMA	{ if ($1[1] >= 'a' && $1[1] <= 'z')
			  {	for (pp = &pragmas; *pp; pp = &(*pp)->next)
			          ;
				*pp = (Pragma*)emalloc(sizeof(Pragma));
				(*pp)->pragma = (char *)emalloc(strlen($1)+1);
				strcpy((*pp)->pragma, $1);
				(*pp)->next = NULL;
			  }
			  else if ((i = atoi($1+2)) > 0)
				yylineno = i;
			  else
			  {	sprintf(errbuf, "directive '%s' ignored (use #import to import files and/or use option -i)", $1);
			  	semwarn(errbuf);
			  }
			}
	;

/******************************************************************************\

	Declarations

\******************************************************************************/

decls	: /* empty */	{ transient &= ~6;
			  permission = 0;
			}
	| dclrs ';' decls
			{ }
	| PRIVATE ':' t3 decls
			{ }
	| PROTECTED ':' t4 decls
			{ }
	| PUBLIC ':' t5 decls
			{ }
	| t1 decls t2 decls
			{ }
	| error ';' 	{ synerror("declaration expected"); yyerrok; }
	;
t1	: '['		{ transient |= 1;
			}
	;
t2	: ']'		{ transient &= ~1;
			}
	;
t3	:		{ permission = Sprivate;
			}
	;
t4	:		{ permission = Sprotected;
			}
	;
t5	:		{ permission = 0;
			}
	;
dclrs	: spec		{ }
	| spec dclr	{ }
	| spec fdclr func
			{ }
	| constr func	{ }
	| destr func	{ }
	| dclrs ',' dclr{ }
	| dclrs ',' fdclr func
			{ }
	;
dclr	: ptrs ID arrayck tag occurs init
			{ if (($3.sto & Stypedef) && sp->table->level == GLOBAL)
			  {	if (($3.typ->type != Tstruct && $3.typ->type != Tunion && $3.typ->type != Tenum) || strcmp($2->name, $3.typ->id->name))
				{	p = enter(typetable, $2);
					p->info.typ = mksymtype($3.typ, $2);
			  		if ($3.sto & Sextern)
						p->info.typ->transient = -1;
					else
						p->info.typ->transient = $3.typ->transient;
			  		p->info.sto = $3.sto;
					p->info.typ->pattern = $5.pattern;
					p->info.typ->minLength = $5.minLength;
					p->info.typ->maxLength = $5.maxLength;
				}
				$2->token = TYPE;
			  }
			  else
			  {	p = enter(sp->table, $2);
			  	p->tag = $4;
			  	p->info.typ = $3.typ;
			  	p->info.sto = (Storage)((int)$3.sto | permission);
				if ($6.hasval)
				{	p->info.hasval = True;
					switch ($3.typ->type)
					{	case Tchar:
						case Tuchar:
						case Tshort:
						case Tushort:
						case Tint:
						case Tuint:
						case Tlong:
						case Tulong:
						case Tllong:
						case Tullong:
						case Tenum:
						case Ttime:
							if ($6.typ->type == Tint || $6.typ->type == Tchar || $6.typ->type == Tenum)
								sp->val = p->info.val.i = $6.val.i;
							else
							{	semerror("type error in initialization constant");
								p->info.hasval = False;
							}
							break;
						case Tfloat:
						case Tdouble:
						case Tldouble:
							if ($6.typ->type == Tfloat || $6.typ->type == Tdouble || $6.typ->type == Tldouble)
								p->info.val.r = $6.val.r;
							else if ($6.typ->type == Tint)
								p->info.val.r = (double)$6.val.i;
							else
							{	semerror("type error in initialization constant");
								p->info.hasval = False;
							}
							break;
						default:
							if ($3.typ->type == Tpointer
							 && ((Tnode*)$3.typ->ref)->type == Tchar
							 && $6.typ->type == Tpointer
							 && ((Tnode*)$6.typ->ref)->type == Tchar)
								p->info.val.s = $6.val.s;
							else if (bflag
							 && $3.typ->type == Tarray
							 && ((Tnode*)$3.typ->ref)->type == Tchar
							 && $6.typ->type == Tpointer
							 && ((Tnode*)$6.typ->ref)->type == Tchar)
							{	if ($3.typ->width / ((Tnode*)$3.typ->ref)->width - 1 < strlen($6.val.s))
								{	semerror("char[] initialization constant too long");
									p->info.val.s = "";
								}

								else
									p->info.val.s = $6.val.s;
							}
							else if ($3.typ->type == Tpointer
							      && ((Tnode*)$3.typ->ref)->id == lookup("std::string"))
							      	p->info.val.s = $6.val.s;
							else if ($3.typ->id == lookup("std::string"))
							      	p->info.val.s = $6.val.s;
							else if ($3.typ->type == Tpointer
							      && $6.typ->type == Tint
							      && $6.val.i == 0)
								p->info.val.i = 0;
							else
							{	semerror("type error in initialization constant");
								p->info.hasval = False;
							}
							break;
					}
				}
				else
					p->info.val.i = sp->val;
			        if ($5.minOccurs < 0)
			        {	if (($3.sto & Sattribute) || $3.typ->type == Tpointer || $3.typ->type == Ttemplate || !strncmp($2->name, "__size", 6))
			        		p->info.minOccurs = 0;
			        	else
			        		p->info.minOccurs = 1;
				}
				else
					p->info.minOccurs = $5.minOccurs;
				p->info.maxOccurs = $5.maxOccurs;
				if (sp->mask)
					sp->val <<= 1;
				else
					sp->val++;
			  	p->info.offset = sp->offset;
				if ($3.sto & Sextern)
					p->level = GLOBAL;
				else if ($3.sto & Stypedef)
					;
			  	else if (sp->grow)
					sp->offset += p->info.typ->width;
				else if (p->info.typ->width > sp->offset)
					sp->offset = p->info.typ->width;
			  }
			  sp->entry = p;
			}
	;
fdclr	: ptrs name	{ if ($1.sto & Stypedef)
			  {	sprintf(errbuf, "invalid typedef qualifier for '%s'", $2->name);
				semwarn(errbuf);
			  }
			  p = enter(sp->table, $2);
			  p->info.typ = $1.typ;
			  p->info.sto = $1.sto;
			  p->info.hasval = False;
			  p->info.offset = sp->offset;
			  if (sp->grow)
				sp->offset += p->info.typ->width;
			  else if (p->info.typ->width > sp->offset)
				sp->offset = p->info.typ->width;
			  sp->entry = p;
			}
	;
id	: ID		{ $$ = $1; }
	| TYPE		{ $$ = $1; }
	;
name	: ID		{ $$ = $1; }
	| OPERATOR '!'	{ $$ = lookup("operator!"); }
	| OPERATOR '~'	{ $$ = lookup("operator~"); }
	| OPERATOR '='	{ $$ = lookup("operator="); }
	| OPERATOR PA	{ $$ = lookup("operator+="); }
	| OPERATOR NA	{ $$ = lookup("operator-="); }
	| OPERATOR TA	{ $$ = lookup("operator*="); }
	| OPERATOR DA	{ $$ = lookup("operator/="); }
	| OPERATOR MA	{ $$ = lookup("operator%="); }
	| OPERATOR AA	{ $$ = lookup("operator&="); }
	| OPERATOR XA	{ $$ = lookup("operator^="); }
	| OPERATOR OA	{ $$ = lookup("operator|="); }
	| OPERATOR LA	{ $$ = lookup("operator<<="); }
	| OPERATOR RA	{ $$ = lookup("operator>>="); }
	| OPERATOR OR	{ $$ = lookup("operator||"); }
	| OPERATOR AN	{ $$ = lookup("operator&&"); }
	| OPERATOR '|'	{ $$ = lookup("operator|"); }
	| OPERATOR '^'	{ $$ = lookup("operator^"); }
	| OPERATOR '&'	{ $$ = lookup("operator&"); }
	| OPERATOR EQ	{ $$ = lookup("operator=="); }
	| OPERATOR NE	{ $$ = lookup("operator!="); }
	| OPERATOR '<'	{ $$ = lookup("operator<"); }
	| OPERATOR LE	{ $$ = lookup("operator<="); }
	| OPERATOR '>'	{ $$ = lookup("operator>"); }
	| OPERATOR GE	{ $$ = lookup("operator>="); }
	| OPERATOR LS	{ $$ = lookup("operator<<"); }
	| OPERATOR RS	{ $$ = lookup("operator>>"); }
	| OPERATOR '+'	{ $$ = lookup("operator+"); }
	| OPERATOR '-'	{ $$ = lookup("operator-"); }
	| OPERATOR '*'	{ $$ = lookup("operator*"); }
	| OPERATOR '/'	{ $$ = lookup("operator/"); }
	| OPERATOR '%'	{ $$ = lookup("operator%"); }
	| OPERATOR PP	{ $$ = lookup("operator++"); }
	| OPERATOR NN	{ $$ = lookup("operator--"); }
	| OPERATOR AR   { $$ = lookup("operator->"); }
	| OPERATOR'['']'{ $$ = lookup("operator[]"); }
	| OPERATOR'('')'{ $$ = lookup("operator()"); }
	| OPERATOR texp { s1 = c_storage($2.sto);
			  s2 = c_type($2.typ);
			  s = (char *)emalloc(strlen(s1) + strlen(s2) + 10);
			  strcpy(s, "operator ");
			  strcat(s, s1);
			  strcat(s, s2);
			  $$ = lookup(s);
			  if (!$$)
				$$ = install(s, ID);
			}
	;
constr	: TYPE		{ if (!(p = entry(classtable, $1)))
			  	semerror("invalid constructor");
			  sp->entry = enter(sp->table, $1);
			  sp->entry->info.typ = mknone();
			  sp->entry->info.sto = Snone;
			  sp->entry->info.offset = sp->offset;
			  sp->node.typ = mkvoid();
			  sp->node.sto = Snone;
			}
	;
destr	: virtual '~' TYPE
			{ if (!(p = entry(classtable, $3)))
			  	semerror("invalid destructor");
			  s = (char *)emalloc(strlen($3->name) + 2);
			  strcpy(s, "~");
			  strcat(s, $3->name);
			  sym = lookup(s);
			  if (!sym)
				sym = install(s, ID);
			  sp->entry = enter(sp->table, sym);
			  sp->entry->info.typ = mknone();
			  sp->entry->info.sto = $1;
			  sp->entry->info.offset = sp->offset;
			  sp->node.typ = mkvoid();
			  sp->node.sto = Snone;
			}
	;
func	: fname '(' s6 fargso ')' constobj abstract
			{ if ($1->level == GLOBAL)
			  {	if (!($1->info.sto & Sextern) && sp->entry && sp->entry->info.typ->type == Tpointer && ((Tnode*)sp->entry->info.typ->ref)->type == Tchar)
			  	{	sprintf(errbuf, "last output parameter of remote method function prototype '%s' is a pointer to a char which will only return one byte: use char** instead to return a string", $1->sym->name);
					semwarn(errbuf);
				}
				if ($1->info.sto & Sextern)
				 	$1->info.typ = mkmethod($1->info.typ, sp->table);
			  	else if (sp->entry && (sp->entry->info.typ->type == Tpointer || sp->entry->info.typ->type == Treference || sp->entry->info.typ->type == Tarray || is_transient(sp->entry->info.typ)))
				{	if ($1->info.typ->type == Tint)
					{	sp->entry->info.sto = (Storage)((int)sp->entry->info.sto | (int)Sreturn);
						$1->info.typ = mkfun(sp->entry);
						$1->info.typ->id = $1->sym;
						if (!is_transient(sp->entry->info.typ))
						{	if (!is_response(sp->entry->info.typ))
							{	if (!is_XML(sp->entry->info.typ))
									add_response($1, sp->entry);
							}
							else
								add_result(sp->entry->info.typ);
						}
					}
					else
					{	sprintf(errbuf, "return type of remote method function prototype '%s' must be integer", $1->sym->name);
						semerror(errbuf);
					}
				}
			  	else
			  	{	sprintf(errbuf, "last output parameter of remote method function prototype '%s' is a return parameter and must be a pointer or reference, or use %s(void) for no return parameter", $1->sym->name, $1->sym->name);
					semerror(errbuf);
			  	}
				if (!($1->info.sto & Sextern))
			  	{	unlinklast(sp->table);
			  		if ((p = entry(classtable, $1->sym)))
					{	if (p->info.typ->ref)
						{	sprintf(errbuf, "remote method name clash: struct/class '%s' already declared at line %d", $1->sym->name, p->lineno);
							semerror(errbuf);
						}
						else
						{	p->info.typ->ref = sp->table;
							p->info.typ->width = sp->offset;
						}
					}
			  		else
			  		{	p = enter(classtable, $1->sym);
						p->info.typ = mkstruct(sp->table, sp->offset);
						p->info.typ->id = $1->sym;
			  		}
			  	}
			  }
			  else if ($1->level == INTERNAL)
			  {	$1->info.typ = mkmethod($1->info.typ, sp->table);
				$1->info.sto = (Storage)((int)$1->info.sto | (int)$6 | (int)$7);
			  	transient &= ~1;
			  }
			  exitscope();
			}
	;
fname	:		{ $$ = sp->entry; }
	;
fargso	: /* empty */	{ }
	| fargs		{ }
	;
fargs	: farg		{ }
	| farg ',' fargs{ }
	;
farg	: tspec ptrs arg arrayck occurs init
			{ if ($4.sto & Stypedef)
			  	semwarn("typedef in function argument");
			  p = enter(sp->table, $3);
			  p->info.typ = $4.typ;
			  p->info.sto = $4.sto;
			  if ($5.minOccurs < 0)
			  {	if (($4.sto & Sattribute) || $4.typ->type == Tpointer)
			        	p->info.minOccurs = 0;
			       	else
			        	p->info.minOccurs = 1;
			  }
			  else
				p->info.minOccurs = $5.minOccurs;
			  p->info.maxOccurs = $5.maxOccurs;
			  if ($6.hasval)
			  {	p->info.hasval = True;
				switch ($4.typ->type)
				{	case Tchar:
					case Tuchar:
					case Tshort:
					case Tushort:
					case Tint:
					case Tuint:
					case Tlong:
					case Tulong:
					case Tenum:
					case Ttime:
						if ($6.typ->type == Tint || $6.typ->type == Tchar || $6.typ->type == Tenum)
							sp->val = p->info.val.i = $6.val.i;
						else
						{	semerror("type error in initialization constant");
							p->info.hasval = False;
						}
						break;
					case Tfloat:
					case Tdouble:
					case Tldouble:
						if ($6.typ->type == Tfloat || $6.typ->type == Tdouble || $6.typ->type == Tldouble)
							p->info.val.r = $6.val.r;
						else if ($6.typ->type == Tint)
							p->info.val.r = (double)$6.val.i;
						else
						{	semerror("type error in initialization constant");
							p->info.hasval = False;
						}
						break;
					default:
						if ($4.typ->type == Tpointer
						 && ((Tnode*)$4.typ->ref)->type == Tchar
						 && $6.typ->type == Tpointer
						 && ((Tnode*)$6.typ->ref)->type == Tchar)
							p->info.val.s = $6.val.s;
						else if ($4.typ->type == Tpointer
						      && ((Tnode*)$4.typ->ref)->id == lookup("std::string"))
						      	p->info.val.s = $6.val.s;
						else if ($4.typ->id == lookup("std::string"))
						      	p->info.val.s = $6.val.s;
						else if ($4.typ->type == Tpointer
						      && $6.typ->type == Tint
						      && $6.val.i == 0)
							p->info.val.i = 0;
						else
						{	semerror("type error in initialization constant");
							p->info.hasval = False;
						}
						break;
				}
			  }
			  p->info.offset = sp->offset;
			  if ($4.sto & Sextern)
				p->level = GLOBAL;
			  else if (sp->grow)
				sp->offset += p->info.typ->width;
			  else if (p->info.typ->width > sp->offset)
				sp->offset = p->info.typ->width;
			  sp->entry = p;
			}
	;
arg	: /* empty */	{ if (sp->table->level != PARAM)
			    $$ = gensymidx("param", (int)++sp->val);
			  else if (eflag)
				$$ = gensymidx("_param", (int)++sp->val);
			  else
				$$ = gensym("_param");
			}
	| ID		{ if (vflag != 1 && *$1->name == '_' && sp->table->level == GLOBAL)
			  { sprintf(errbuf, "SOAP 1.2 does not support anonymous parameters '%s'", $1->name);
			    semwarn(errbuf);
			  }
			  $$ = $1;
			}
	;

/******************************************************************************\

	Type specification

\******************************************************************************/

/* texp : type expression (subset of C) */
texp	: tspec ptrs array
			{ $$ = $3; }
	| tspec ptrs ID array
			{ $$ = $4; }
	;
spec	: /*empty */	{ $$.typ = mkint();
			  $$.sto = Snone;
			  sp->node = $$;
			}
	| store spec	{ $$.typ = $2.typ;
			  $$.sto = (Storage)((int)$1 | (int)$2.sto);
			  if (($$.sto & Sattribute) && !is_primitive_or_string($2.typ) && !is_stdstr($2.typ) && !is_binary($2.typ) && !is_external($2.typ))
			  {	semwarn("invalid attribute type");
			  	$$.sto = (Storage)((int)$$.sto & ~Sattribute);
			  }
			  sp->node = $$;
			  if ($1 & Sextern)
				transient = 0;
			}
	| type spec	{ if ($1->type == Tint)
				switch ($2.typ->type)
				{ case Tchar:	$$.typ = $2.typ; break;
				  case Tshort:	$$.typ = $2.typ; break;
				  case Tint:	$$.typ = $1; break;
				  case Tlong:	$$.typ = $2.typ; break;
				  case Tllong:	$$.typ = $2.typ; break;
				  default:	semwarn("illegal use of 'signed'");
						$$.typ = $2.typ;
				}
			  else if ($1->type == Tuint)
				switch ($2.typ->type)
				{ case Tchar:	$$.typ = mkuchar(); break;
				  case Tshort:	$$.typ = mkushort(); break;
				  case Tint:	$$.typ = $1; break;
				  case Tlong:	$$.typ = mkulong(); break;
				  case Tllong:	$$.typ = mkullong(); break;
				  default:	semwarn("illegal use of 'unsigned'");
						$$.typ = $2.typ;
				}
			  else if ($1->type == Tlong)
				switch ($2.typ->type)
				{ case Tint:	$$.typ = $1; break;
				  case Tlong:	$$.typ = mkllong(); break;
				  case Tuint:	$$.typ = mkulong(); break;
				  case Tulong:	$$.typ = mkullong(); break;
				  case Tdouble:	$$.typ = mkldouble(); break;
				  default:	semwarn("illegal use of 'long'");
						$$.typ = $2.typ;
				}
			  else if ($1->type == Tulong)
				switch ($2.typ->type)
				{ case Tint:	$$.typ = $1; break;
				  case Tlong:	$$.typ = mkullong(); break;
				  case Tuint:	$$.typ = $1; break;
				  case Tulong:	$$.typ = mkullong(); break;
				  default:	semwarn("illegal use of 'long'");
						$$.typ = $2.typ;
				}
			  else if ($2.typ->type == Tint)
				$$.typ = $1;
			  else
			  	semwarn("invalid type (missing ';' or type name used as non-type identifier?)");
			  $$.sto = $2.sto;
			  sp->node = $$;
			}
	;
tspec	: store		{ $$.typ = mkint();
			  $$.sto = $1;
			  sp->node = $$;
			  if ($1 & Sextern)
				transient = 0;
			}
	| type		{ $$.typ = $1;
			  $$.sto = Snone;
			  sp->node = $$;
			}
	| store tspec	{ $$.typ = $2.typ;
			  $$.sto = (Storage)((int)$1 | (int)$2.sto);
			  if (($$.sto & Sattribute) && !is_primitive_or_string($2.typ) && !is_stdstr($2.typ) && !is_binary($2.typ) && !is_external($2.typ))
			  {	semwarn("invalid attribute type");
			  	$$.sto = (Storage)((int)$$.sto & ~Sattribute);
			  }
			  sp->node = $$;
			  if ($1 & Sextern)
				transient = 0;
			}
	| type tspec	{ if ($1->type == Tint)
				switch ($2.typ->type)
				{ case Tchar:	$$.typ = $2.typ; break;
				  case Tshort:	$$.typ = $2.typ; break;
				  case Tint:	$$.typ = $1; break;
				  case Tlong:	$$.typ = $2.typ; break;
				  case Tllong:	$$.typ = $2.typ; break;
				  default:	semwarn("illegal use of 'signed'");
						$$.typ = $2.typ;
				}
			  else if ($1->type == Tuint)
				switch ($2.typ->type)
				{ case Tchar:	$$.typ = mkuchar(); break;
				  case Tshort:	$$.typ = mkushort(); break;
				  case Tint:	$$.typ = $1; break;
				  case Tlong:	$$.typ = mkulong(); break;
				  case Tllong:	$$.typ = mkullong(); break;
				  default:	semwarn("illegal use of 'unsigned'");
						$$.typ = $2.typ;
				}
			  else if ($1->type == Tlong)
				switch ($2.typ->type)
				{ case Tint:	$$.typ = $1; break;
				  case Tlong:	$$.typ = mkllong(); break;
				  case Tuint:	$$.typ = mkulong(); break;
				  case Tulong:	$$.typ = mkullong(); break;
				  case Tdouble:	$$.typ = mkldouble(); break;
				  default:	semwarn("illegal use of 'long'");
						$$.typ = $2.typ;
				}
			  else if ($1->type == Tulong)
				switch ($2.typ->type)
				{ case Tint:	$$.typ = $1; break;
				  case Tlong:	$$.typ = mkullong(); break;
				  case Tuint:	$$.typ = $1; break;
				  case Tulong:	$$.typ = mkullong(); break;
				  default:	semwarn("illegal use of 'long'");
						$$.typ = $2.typ;
				}
			  else if ($2.typ->type == Tint)
				$$.typ = $1;
			  else
			  	semwarn("invalid type");
			  $$.sto = $2.sto;
			  sp->node = $$;
			}
	;
type	: VOID { $$ = mkvoid(); }
	| BOOL		{ $$ = mkbool(); }
	| CHAR		{ $$ = mkchar(); }
	| WCHAR		{ $$ = mkwchart(); }
	| SHORT		{ $$ = mkshort(); }
	| INT		{ $$ = mkint(); }
	| LONG		{ $$ = mklong(); }
	| LLONG		{ $$ = mkllong(); }
	| ULLONG	{ $$ = mkullong(); }
	| SIZE		{ $$ = mkulong(); }
	| FLOAT		{ $$ = mkfloat(); }
	| DOUBLE	{ $$ = mkdouble(); }
	| SIGNED	{ $$ = mkint(); }
	| UNSIGNED	{ $$ = mkuint(); }
	| UCHAR		{ $$ = mkuchar(); }
	| USHORT	{ $$ = mkushort(); }
	| UINT		{ $$ = mkuint(); }
	| ULONG		{ $$ = mkulong(); }
	| TIME		{ $$ = mktimet(); }
	| TEMPLATE '<' tname id '>' CLASS id
			{ if (!(p = entry(templatetable, $7)))
			  {	p = enter(templatetable, $7);
			  	p->info.typ = mktemplate(NULL, $7);
			  	$7->token = TYPE;
			  }
			  $$ = p->info.typ;
			}
	| CLASS '{' s2 decls '}'
			{ sym = gensym("_Struct");
			  sprintf(errbuf, "anonymous class will be named '%s'", sym->name);
			  semwarn(errbuf);
			  if ((p = entry(classtable, sym)))
			  {	if (p->info.typ->ref || p->info.typ->type != Tclass)
				{	sprintf(errbuf, "class '%s' already declared at line %d", sym->name, p->lineno);
					semerror(errbuf);
				}
			  }
			  else
			  {	p = enter(classtable, sym);
				p->info.typ = mkclass((Table*)0, 0);
			  }
			  sym->token = TYPE;
			  sp->table->sym = sym;
			  p->info.typ->ref = sp->table;
			  p->info.typ->width = sp->offset;
			  p->info.typ->id = sym;
			  $$ = p->info.typ;
			  exitscope();
			}
	| class '{' s2 decls '}'
			{ p = reenter(classtable, $1->sym);
			  sp->table->sym = p->sym;
			  p->info.typ->ref = sp->table;
			  p->info.typ->width = sp->offset;
			  p->info.typ->id = p->sym;
			  if (p->info.typ->base)
			  	sp->table->prev = (Table*)entry(classtable, p->info.typ->base)->info.typ->ref;
			  $$ = p->info.typ;
			  exitscope();
			}
	| class ':' base '{' s2 decls '}'
			{ p = reenter(classtable, $1->sym);
			  sp->table->sym = p->sym;
			  if (!$3)
				semerror("invalid base class");
			  else
			  {	sp->table->prev = (Table*)$3->info.typ->ref;
				if (!sp->table->prev && !$3->info.typ->transient)
				{	sprintf(errbuf, "class '%s' has incomplete type", $3->sym->name);
					semerror(errbuf);
				}
			  	p->info.typ->base = $3->info.typ->id;
			  }
			  p->info.typ->ref = sp->table;
			  p->info.typ->width = sp->offset;
			  p->info.typ->id = p->sym;
			  $$ = p->info.typ;
			  exitscope();
			}
	| class		{ $1->info.typ->id = $1->sym;
			  $$ = $1->info.typ;
			}
	| class ':' base
			{ if (!$3)
				semerror("invalid base class");
			  else
			  {	if (!$3->info.typ->ref && !$3->info.typ->transient)
				{	sprintf(errbuf, "class '%s' has incomplete type", $3->sym->name);
					semerror(errbuf);
				}
			  	$1->info.typ->base = $3->info.typ->id;
			  }
			  $1->info.typ->id = $1->sym;
			  $$ = $1->info.typ;
			}
	| STRUCT '{' s2 decls '}'
			{ sym = gensym("_Struct");
			  sprintf(errbuf, "anonymous struct will be named '%s'", sym->name);
			  semwarn(errbuf);
			  if ((p = entry(classtable, sym)))
			  {	if (p->info.typ->ref || p->info.typ->type != Tstruct)
				{	sprintf(errbuf, "struct '%s' already declared at line %d", sym->name, p->lineno);
					semerror(errbuf);
				}
				else
				{	p->info.typ->ref = sp->table;
					p->info.typ->width = sp->offset;
				}
			  }
			  else
			  {	p = enter(classtable, sym);
				p->info.typ = mkstruct(sp->table, sp->offset);
			  }
			  p->info.typ->id = sym;
			  $$ = p->info.typ;
			  exitscope();
			}
	| struct '{' s2 decls '}'
			{ if ((p = entry(classtable, $1->sym)) && p->info.typ->ref)
			  {	if (is_mutable(p->info.typ))
			  	{	if (merge((Table*)p->info.typ->ref, sp->table))
					{	sprintf(errbuf, "member name clash in struct '%s' declared at line %d", $1->sym->name, p->lineno);
						semerror(errbuf);
					}
			  		p->info.typ->width += sp->offset;
				}
			  }
			  else
			  {	p = reenter(classtable, $1->sym);
			  	p->info.typ->ref = sp->table;
			  	p->info.typ->width = sp->offset;
			  	p->info.typ->id = p->sym;
			  }
			  $$ = p->info.typ;
			  exitscope();
			}
	| STRUCT ID	{ if ((p = entry(classtable, $2)))
			  {	if (p->info.typ->type == Tstruct)
			  		$$ = p->info.typ;
			  	else
				{	sprintf(errbuf, "'struct %s' redeclaration (line %d)", $2->name, p->lineno);
			  		semerror(errbuf);
			  		$$ = mkint();
				}
			  }
			  else
			  {	p = enter(classtable, $2);
			  	$$ = p->info.typ = mkstruct((Table*)0, 0);
				p->info.typ->id = $2;
			  }
			}
	| STRUCT TYPE	{ if ((p = entry(classtable, $2)))
			  {	if (p->info.typ->type == Tstruct)
					$$ = p->info.typ;
			  	else
				{	sprintf(errbuf, "'struct %s' redeclaration (line %d)", $2->name, p->lineno);
			  		semerror(errbuf);
			  		$$ = mkint();
				}
			  }
			  else
			  {	p = enter(classtable, $2);
			  	$$ = p->info.typ = mkstruct((Table*)0, 0);
				p->info.typ->id = $2;
			  }
			}
	| UNION '{' s3 decls '}'
			{ sym = gensym("_Union");
			  sprintf(errbuf, "anonymous union will be named '%s'", sym->name);
			  semwarn(errbuf);
			  $$ = mkunion(sp->table, sp->offset);
			  if ((p = entry(classtable, sym)))
			  {	if (p->info.typ->ref)
				{	sprintf(errbuf, "union or struct '%s' already declared at line %d", sym->name, p->lineno);
					semerror(errbuf);
				}
				else
				{	p->info.typ->ref = sp->table;
					p->info.typ->width = sp->offset;
				}
			  }
			  else
			  {	p = enter(classtable, sym);
				p->info.typ = mkunion(sp->table, sp->offset);
			  }
			  p->info.typ->id = sym;
			  $$ = p->info.typ;
			  exitscope();
			}
	| UNION id '{' s3 decls '}'
			{ if ((p = entry(classtable, $2)))
			  {	if (p->info.typ->ref || p->info.typ->type != Tunion)
			  	{	sprintf(errbuf, "union '%s' already declared at line %d", $2->name, p->lineno);
					semerror(errbuf);
				}
				else
				{	p = reenter(classtable, $2);
					p->info.typ->ref = sp->table;
					p->info.typ->width = sp->offset;
				}
			  }
			  else
			  {	p = enter(classtable, $2);
				p->info.typ = mkunion(sp->table, sp->offset);
			  }
			  p->info.typ->id = $2;
			  $$ = p->info.typ;
			  exitscope();
			}
	| UNION ID	{ if ((p = entry(classtable, $2)))
			  {	if (p->info.typ->type == Tunion)
					$$ = p->info.typ;
			  	else
				{	sprintf(errbuf, "'union %s' redeclaration (line %d)", $2->name, p->lineno);
			  		semerror(errbuf);
			  		$$ = mkint();
				}
			  }
			  else
			  {	p = enter(classtable, $2);
			  	$$ = p->info.typ = mkunion((Table*) 0, 0);
				p->info.typ->id = $2;
			  }
			}
	| UNION TYPE	{ if ((p = entry(classtable, $2)))
			  {	if (p->info.typ->type == Tunion)
					$$ = p->info.typ;
			  	else
				{	sprintf(errbuf, "'union %s' redeclaration (line %d)", $2->name, p->lineno);
			  		semerror(errbuf);
			  		$$ = mkint();
				}
			  }
			  else
			  {	p = enter(classtable, $2);
			  	$$ = p->info.typ = mkunion((Table*) 0, 0);
				p->info.typ->id = $2;
			  }
			}
	| ENUM '{' s2 dclrs s5 '}'
			{ sym = gensym("_Enum");
			  sprintf(errbuf, "anonymous enum will be named '%s'", sym->name);
			  semwarn(errbuf);
			  if ((p = entry(enumtable, sym)))
			  {	if (p->info.typ->ref)
				{	sprintf(errbuf, "enum '%s' already declared at line %d", sym->name, p->lineno);
					semerror(errbuf);
				}
				else
				{	p->info.typ->ref = sp->table;
					p->info.typ->width = 4; /* 4 = enum */
				}
			  }
			  else
			  {	p = enter(enumtable, sym);
				p->info.typ = mkenum(sp->table);
			  }
			  p->info.typ->id = sym;
			  $$ = p->info.typ;
			  exitscope();
			}
	| enum '{' s2 dclrs s5 '}'
			{ if ((p = entry(enumtable, $1->sym)))
			  {	if (p->info.typ->ref)
				{	sprintf(errbuf, "enum '%s' already declared at line %d", $1->sym->name, p->lineno);
					semerror(errbuf);
				}
				else
				{	p->info.typ->ref = sp->table;
					p->info.typ->width = 4; /* 4 = enum */
				}
			  }
			  else
			  {	p = enter(enumtable, $1->sym);
				p->info.typ = mkenum(sp->table);
			  }
			  p->info.typ->id = $1->sym;
			  $$ = p->info.typ;
			  exitscope();
			}
	| ENUM '*' id '{' s4 dclrs s5 '}'
			{ if ((p = entry(enumtable, $3)))
			  {	if (p->info.typ->ref)
				{	sprintf(errbuf, "enum '%s' already declared at line %d", $3->name, p->lineno);
					semerror(errbuf);
				}
				else
				{	p->info.typ->ref = sp->table;
					p->info.typ->width = 8; /* 8 = mask */
				}
			  }
			  else
			  {	p = enter(enumtable, $3);
				p->info.typ = mkmask(sp->table);
			  }
			  p->info.typ->id = $3;
			  $$ = p->info.typ;
			  exitscope();
			}
	| ENUM ID	{ if ((p = entry(enumtable, $2)))
			  	$$ = p->info.typ;
			  else
			  {	p = enter(enumtable, $2);
			  	$$ = p->info.typ = mkenum((Table*)0);
				p->info.typ->id = $2;
			  }
			}
	| ENUM TYPE	{ if ((p = entry(enumtable, $2)))
				$$ = p->info.typ;
			  else
			  {	p = enter(enumtable, $2);
			  	$$ = p->info.typ = mkenum((Table*)0);
				p->info.typ->id = $2;
			  }
			}
	| TYPE		{ if ((p = entry(typetable, $1)))
				$$ = p->info.typ;
			  else if ((p = entry(classtable, $1)))
			  	$$ = p->info.typ;
			  else if ((p = entry(enumtable, $1)))
			  	$$ = p->info.typ;
			  else if ($1 == lookup("std::string") || $1 == lookup("std::wstring"))
			  {	p = enter(classtable, $1);
				$$ = p->info.typ = mkclass((Table*)0, 0);
			  	p->info.typ->id = $1;
			  	p->info.typ->transient = -2;
			  }
			  else
			  {	sprintf(errbuf, "unknown type '%s'", $1->name);
				semerror(errbuf);
				$$ = mkint();
			  }
			}
	| TYPE '<' texp '>'
			{ if ((p = entry(templatetable, $1)))
				$$ = mktemplate($3.typ, $1);
			  else
			  {	sprintf(errbuf, "invalid template '%s'", $1->name);
				semerror(errbuf);
				$$ = mkint();
			  }
			}
	| CLASS error '}'
			{ synerror("malformed class definition (use spacing around ':' to separate derived : base)");
			  yyerrok;
			  $$ = mkint();
			}
	| STRUCT error '}'
			{ synerror("malformed struct definition");
			  yyerrok;
			  $$ = mkint();
			}
	| UNION error '}'
			{ synerror("malformed union definition");
			  yyerrok;
			  $$ = mkint();
			}
	| ENUM error '}'
			{ synerror("malformed enum definition");
			  yyerrok;
			  $$ = mkint();
			}
	;
struct	: STRUCT id	{ if ((p = entry(classtable, $2)))
			  {	if (p->info.typ->ref)
			   	{	if (!is_mutable(p->info.typ))
					{	sprintf(errbuf, "struct '%s' already declared at line %d", $2->name, p->lineno);
						semerror(errbuf);
					}
				}
				else
					p = reenter(classtable, $2);
			  }
			  else
			  {	p = enter(classtable, $2);
				p->info.typ = mkstruct((Table*)0, 0);
			  }
			  $$ = p;
			}
	;
class	: CLASS id	{ if ((p = entry(classtable, $2)))
			  {	if (p->info.typ->ref)
			   	{	if (!is_mutable(p->info.typ))
					{	sprintf(errbuf, "class '%s' already declared at line %d", $2->name, p->lineno);
						semerror(errbuf);
					}
				}
				else
					p = reenter(classtable, $2);
			  }
			  else
			  {	p = enter(classtable, $2);
				p->info.typ = mkclass((Table*)0, 0);
				p->info.typ->id = p->sym;
			  }
			  $2->token = TYPE;
			  $$ = p;
			}
	;
enum	: ENUM id	{ if ((p = entry(enumtable, $2)))
			  {	if (p->info.typ->ref)
				{	sprintf(errbuf, "enum '%s' already declared at line %d", $2->name, p->lineno);
					semerror(errbuf);
				}
				/*
				else
					p = reenter(classtable, $2);
			  	*/
			  }
			  else
			  {	p = enter(enumtable, $2);
				p->info.typ = mkenum(0);
			  }
			  $$ = p;
			}
	;
tname	: CLASS		{ }
	| TYPENAME	{ }
	;
base	: PROTECTED base{ $$ = $2; }
	| PRIVATE base	{ $$ = $2; }
	| PUBLIC base	{ $$ = $2; }
	| TYPE		{ $$ = entry(classtable, $1);
			  if (!$$)
			  {	p = entry(typetable, $1);
			  	if (p && (p->info.typ->type == Tclass || p->info.typ->type == Tstruct))
					$$ = p;
			  }
			}
	| STRUCT ID	{ $$ = entry(classtable, $2); }
	;
s2	: /* empty */	{ if (transient == -2)
			  	transient = 0;
			  permission = 0;
			  enterscope(mktable(NULL), 0);
			  sp->entry = NULL;
			}
	;
s3	: /* empty */	{ if (transient == -2)
			  	transient = 0;
			  permission = 0;
			  enterscope(mktable(NULL), 0);
			  sp->entry = NULL;
			  sp->grow = False;
			}
	;
s4	: /* empty */	{ enterscope(mktable(NULL), 0);
			  sp->entry = NULL;
			  sp->mask = True;
			  sp->val = 1;
			}
	;
s5	: /* empty */	{ }
	| ','		{ }
	;
s6	: /* empty */	{ if (sp->table->level == INTERNAL)
			  	transient |= 1;
			  permission = 0;
			  enterscope(mktable(NULL), 0);
			  sp->entry = NULL;
			  sp->table->level = PARAM;
			}
	;
store	: AUTO		{ $$ = Sauto; }
	| REGISTER	{ $$ = Sregister; }
	| STATIC	{ $$ = Sstatic; }
	| EXPLICIT	{ $$ = Sexplicit; }
	| EXTERN	{ $$ = Sextern; transient = 1; }
	| TYPEDEF	{ $$ = Stypedef; }
	| VIRTUAL	{ $$ = Svirtual; }
	| CONST     { $$ = Sconst; }
	| FRIEND	{ $$ = Sfriend; }
	| INLINE	{ $$ = Sinline; }
	| MUSTUNDERSTAND{ $$ = SmustUnderstand; }
	| RETURN	{ $$ = Sreturn; }
	| '@'		{ $$ = Sattribute;
			  if (eflag)
			   	semwarn("SOAP RPC encoding does not support XML attributes");
			}
	| '$'		{ $$ = Sspecial; }
	| VOLATILE	{ $$ = Sextern; transient = -2; }
	;
constobj: /* empty */	{ $$ = Snone; }
	| CONST		{ $$ = Sconstobj; }
	;
abstract: /* empty */	{ $$ = Snone; }
	| '=' LNG	{ $$ = Sabstract; }
	;
virtual : /* empty */	{ $$ = Snone; }
	| VIRTUAL	{ $$ = Svirtual; }
	;
ptrs	: /* empty */	{ $$ = tmp = sp->node; }
	| ptrs '*'	{ /* handle const pointers, such as const char* */
			  if (/*tmp.typ->type == Tchar &&*/ (tmp.sto & Sconst))
			  	tmp.sto = (Storage)(((int)tmp.sto & ~Sconst) | Sconstptr);
			  tmp.typ = mkpointer(tmp.typ);
			  tmp.typ->transient = transient;
			  $$ = tmp;
			}
	| ptrs '&'	{ tmp.typ = mkreference(tmp.typ);
			  tmp.typ->transient = transient;
			  $$ = tmp;
			}
	;
array	: /* empty */ 	{ $$ = tmp;	/* tmp is inherited */
			}
	| '[' cexp ']' array
			{ if (!bflag && $4.typ->type == Tchar)
			  {	sprintf(errbuf, "char[" SOAP_LONG_FORMAT "] will be serialized as an array of " SOAP_LONG_FORMAT " bytes: use soapcpp2 option -b to enable char[] string serialization or use char* for strings", $2.val.i, $2.val.i);
			  	semwarn(errbuf);
			  }
			  if ($2.hasval && $2.typ->type == Tint && $2.val.i > 0 && $4.typ->width > 0)
				$$.typ = mkarray($4.typ, (int) $2.val.i * $4.typ->width);
			  else
			  {	$$.typ = mkarray($4.typ, 0);
			  	semerror("undetermined array size");
			  }
			  $$.sto = $4.sto;
			}
	| '[' ']' array	{ $$.typ = mkpointer($3.typ); /* zero size array = pointer */
			  $$.sto = $3.sto;
			}
	;
arrayck	: array		{ if ($1.typ->type == Tstruct || $1.typ->type == Tclass)
				if (!$1.typ->ref && !$1.typ->transient && !($1.sto & Stypedef))
			   	{	sprintf(errbuf, "struct/class '%s' has incomplete type", $1.typ->id->name);
					semerror(errbuf);
				}
			  $$ = $1;
			}
	;
init	: /* empty */   { $$.hasval = False; }
	| '=' cexp      { if ($2.hasval)
			  {	$$.typ = $2.typ;
				$$.hasval = True;
				$$.val = $2.val;
			  }
			  else
			  {	$$.hasval = False;
				semerror("initialization expression not constant");
			  }
			}
	;
tag	: /* empty */	{ $$ = NULL; }
	| TAG		{ $$ = $1; }
	;
occurs	: patt
			{ $$.minOccurs = -1;
			  $$.maxOccurs = 1;
			  $$.minLength = MINLONG64;
			  $$.maxLength = MAXLONG64;
			  $$.pattern = $1;
			}
	| patt cint
			{ $$.minOccurs = (LONG64)$2;
			  $$.maxOccurs = 1;
			  $$.minLength = (LONG64)$2;
			  $$.maxLength = MAXLONG64;
			  $$.pattern = $1;
			}
	| patt cint ':'
			{ $$.minOccurs = (LONG64)$2;
			  $$.maxOccurs = 1;
			  $$.minLength = (LONG64)$2;
			  $$.maxLength = MAXLONG64;
			  $$.pattern = $1;
			}
	| patt cint ':' cint
			{ $$.minOccurs = (LONG64)$2;
			  $$.maxOccurs = (LONG64)$4;
			  $$.minLength = (LONG64)$2;
			  $$.maxLength = (LONG64)$4;
			  $$.pattern = $1;
			}
	| patt ':' cint
			{ $$.minOccurs = -1;
			  $$.maxOccurs = (LONG64)$3;
			  $$.minLength = MINLONG64;
			  $$.maxLength = (LONG64)$3;
			  $$.pattern = $1;
			}
	;
patt	: /* empty */	{ $$ = NULL; }
	| STR		{ $$ = $1; }
	;
cint	: LNG		{ $$ = $1; }
	| '+' LNG	{ $$ = $2; }
	| '-' LNG	{ $$ = -$2; }
	;

/******************************************************************************\

	Expressions

\******************************************************************************/

expr	: expr ',' expr	{ $$ = $3; }
	| cexp		{ $$ = $1; }
	;
/* cexp : conditional expression */
cexp	: obex '?' qexp ':' cexp
			{ $$.typ = $3.typ;
			  $$.sto = Snone;
			  $$.hasval = False;
			}
	| oexp
	;
/* qexp : true-branch of ? : conditional expression */
qexp	: expr		{ $$ = $1; }
	;
/* oexp : or-expression */
oexp	: obex OR aexp	{ $$.hasval = False;
			  $$.typ = mkint();
			}
	| aexp		{ $$ = $1; }
	;
obex	: oexp		{ $$ = $1; }
	;
/* aexp : and-expression */
aexp	: abex AN rexp	{ $$.hasval = False;
			  $$.typ = mkint();
			}
	| rexp		{ $$ = $1; }
	;
abex	: aexp		{ $$ = $1; }
	;
/* rexp : relational expression */
rexp	: rexp '|' rexp	{ $$ = iop("|", $1, $3); }
	| rexp '^' rexp	{ $$ = iop("^", $1, $3); }
	| rexp '&' rexp	{ $$ = iop("&", $1, $3); }
	| rexp EQ  rexp	{ $$ = relop("==", $1, $3); }
	| rexp NE  rexp	{ $$ = relop("!=", $1, $3); }
	| rexp '<' rexp	{ $$ = relop("<", $1, $3); }
	| rexp LE  rexp	{ $$ = relop("<=", $1, $3); }
	| rexp '>' rexp	{ $$ = relop(">", $1, $3); }
	| rexp GE  rexp	{ $$ = relop(">=", $1, $3); }
	| rexp LS  rexp	{ $$ = iop("<<", $1, $3); }
	| rexp RS  rexp	{ $$ = iop(">>", $1, $3); }
	| rexp '+' rexp	{ $$ = op("+", $1, $3); }
	| rexp '-' rexp	{ $$ = op("-", $1, $3); }
	| rexp '*' rexp	{ $$ = op("*", $1, $3); }
	| rexp '/' rexp	{ $$ = op("/", $1, $3); }
	| rexp '%' rexp	{ $$ = iop("%", $1, $3); }
	| lexp		{ $$ = $1; }
	;
/* lexp : lvalue kind of expression with optional prefix contructs */
lexp	: '!' lexp	{ if ($2.hasval)
				$$.val.i = !$2.val.i;
			  $$.typ = $2.typ;
			  $$.hasval = $2.hasval;
			}
	| '~' lexp	{ if ($2.hasval)
				$$.val.i = ~$2.val.i;
			  $$.typ = $2.typ;
			  $$.hasval = $2.hasval;
			}
	| '-' lexp	{ if ($2.hasval) {
				if (integer($2.typ))
					$$.val.i = -$2.val.i;
				else if (real($2.typ))
					$$.val.r = -$2.val.r;
				else	typerror("string?");
			  }
			  $$.typ = $2.typ;
			  $$.hasval = $2.hasval;
			}
	| '+' lexp	{ $$ = $2; }
	| '*' lexp	{ if ($2.typ->type == Tpointer) {
			  	$$.typ = (Tnode*)$2.typ->ref;
			  } else
			  	typerror("dereference of non-pointer type");
			  $$.sto = Snone;
			  $$.hasval = False;
			}
	| '&' lexp	{ $$.typ = mkpointer($2.typ);
			  $$.sto = Snone;
			  $$.hasval = False;
			}
	| SIZEOF '(' texp ')'
			{ $$.hasval = True;
			  $$.typ = mkint();
			  $$.val.i = $3.typ->width;
			}
	| pexp		{ $$ = $1; }
	;
/* pexp : primitive expression with optional postfix constructs */
pexp	: '(' expr ')'	{ $$ = $2; }
	| ID		{ if ((p = enumentry($1)) == (Entry*) 0)
				p = undefined($1);
			  else
			  	$$.hasval = True;
			  $$.typ = p->info.typ;
			  $$.val = p->info.val;
			}
	| LNG		{ $$.typ = mkint();
			  $$.hasval = True;
			  $$.val.i = $1;
			}
	| null		{ $$.typ = mkint();
			  $$.hasval = True;
			  $$.val.i = 0;
			}
	| DBL		{ $$.typ = mkfloat();
			  $$.hasval = True;
			  $$.val.r = $1;
			}
	| CHR		{ $$.typ = mkchar();
			  $$.hasval = True;
			  $$.val.i = $1;
			}
	| STR		{ $$.typ = mkstring();
			  $$.hasval = True;
			  $$.val.s = $1;
			}
	| CFALSE	{ $$.typ = mkbool();
			  $$.hasval = True;
			  $$.val.i = 0;
			}
	| CTRUE		{ $$.typ = mkbool();
			  $$.hasval = True;
			  $$.val.i = 1;
			}
	;

%%

/*
 * ???
 */
extern "C" int yywrap()
{	
	return 1;
} 

/******************************************************************************\

	Support routines

\******************************************************************************/

static Node op(const char *op, Node p, Node q)
{	
	Node	r;
	Tnode	*typ;
	r.typ = p.typ;
	r.sto = Snone;
	if (p.hasval && q.hasval) {
		if (integer(p.typ) && integer(q.typ))
			switch (op[0]) {
			case '|':	r.val.i = p.val.i |  q.val.i; break;
			case '^':	r.val.i = p.val.i ^  q.val.i; break;
			case '&':	r.val.i = p.val.i &  q.val.i; break;
			case '<':	r.val.i = p.val.i << q.val.i; break;
			case '>':	r.val.i = p.val.i >> q.val.i; break;
			case '+':	r.val.i = p.val.i +  q.val.i; break;
			case '-':	r.val.i = p.val.i -  q.val.i; break;
			case '*':	r.val.i = p.val.i *  q.val.i; break;
			case '/':	r.val.i = p.val.i /  q.val.i; break;
			case '%':	r.val.i = p.val.i %  q.val.i; break;
			default:	typerror(op);
			}
		else if (real(p.typ) && real(q.typ))
			switch (op[0]) {
			case '+':	r.val.r = p.val.r + q.val.r; break;
			case '-':	r.val.r = p.val.r - q.val.r; break;
			case '*':	r.val.r = p.val.r * q.val.r; break;
			case '/':	r.val.r = p.val.r / q.val.r; break;
			default:	typerror(op);
			}
		else	semerror("illegal constant operation");
		r.hasval = True;
	} else {
		typ = mgtype(p.typ, q.typ);
		r.hasval = False;
	}
	return r;
}

static Node
iop(const char *iop, Node p, Node q)
{	if (integer(p.typ) && integer(q.typ))
		return op(iop, p, q);
	typerror("integer operands only");
	return p;
}

static Node
relop(const char *op, Node p, Node q)
{	Node	r;
	Tnode	*typ;
	r.typ = mkint();
	r.sto = Snone;
	r.hasval = False;
	if (p.typ->type != Tpointer || p.typ != q.typ)
		typ = mgtype(p.typ, q.typ);
	return r;
}

/******************************************************************************\

	Scope management

\******************************************************************************/

/*
mkscope - initialize scope stack with a new table and offset
*/
static void
mkscope(Table *table, int offset)
{	sp = stack-1;
	enterscope(table, offset);
}

/*
enterscope - enter a new scope by pushing a new table and offset on the stack
*/
static void
enterscope(Table *table, int offset)
{	if (++sp == stack+MAXNEST)
		execerror("maximum scope depth exceeded");
	sp->table = table;
	sp->val = 0;
	sp->offset = offset;
	sp->grow = True;	/* by default, offset grows */
	sp->mask = False;
}

/*
exitscope - exit a scope by popping the table and offset from the stack
*/
static void
exitscope(void)
{	check(sp-- != stack, "exitscope() has no matching enterscope()");
}

/******************************************************************************\

	Undefined symbol

\******************************************************************************/

static Entry*
undefined(Symbol *sym)
{	Entry	*p;
	sprintf(errbuf, "undefined identifier '%s'", sym->name);
	semwarn(errbuf);
	p = enter(sp->table, sym);
	p->level = GLOBAL;
	p->info.typ = mkint();
	p->info.sto = Sextern;
	p->info.hasval = False;
	return p;
}

/*
mgtype - return most general type among two numerical types
*/
Tnode*
mgtype(Tnode *typ1, Tnode *typ2)
{	if (numeric(typ1) && numeric(typ2)) {
		if (typ1->type < typ2->type)
			return typ2;
	} else	typerror("non-numeric type");
	return typ1;
}

/******************************************************************************\

	Type checks

\******************************************************************************/

static int
integer(Tnode *typ)
{	switch (typ->type) {
	case Tchar:
	case Tshort:
	case Tint:
	case Tlong:	return True;
	default:	break;
	}
	return False;
}

static int
real(Tnode *typ)
{	switch (typ->type) {
	case Tfloat:
	case Tdouble:
	case Tldouble:	return True;
	default:	break;
	}
	return False;
}

static int
numeric(Tnode *typ)
{	return integer(typ) || real(typ);
}

static void
add_fault(Table *gt)
{ Table *t;
  Entry *p1, *p2, *p3, *p4;
  Symbol *s1, *s2, *s3, *s4;
  imported = NULL;
  s1 = lookup("SOAP_ENV__Code");
  p1 = entry(classtable, s1);
  if (!p1 || !p1->info.typ->ref)
  { t = mktable((Table*)0);
    if (!p1)
    { p1 = enter(classtable, s1);
      p1->info.typ = mkstruct(t, 3*4);
      p1->info.typ->id = s1;
    }
    else
      p1->info.typ->ref = t;
    p2 = enter(t, lookup("SOAP_ENV__Value"));
    p2->info.typ = qname;
    p2->info.minOccurs = 0;
    p2 = enter(t, lookup("SOAP_ENV__Subcode"));
    p2->info.typ = mkpointer(p1->info.typ);
    p2->info.minOccurs = 0;
  }
  s2 = lookup("SOAP_ENV__Detail");
  p2 = entry(classtable, s2);
  if (!p2 || !p2->info.typ->ref)
  { t = mktable((Table*)0);
    if (!p2)
    { p2 = enter(classtable, s2);
      p2->info.typ = mkstruct(t, 3*4);
      p2->info.typ->id = s2;
    }
    else
      p2->info.typ->ref = t;
    p3 = enter(t, lookup("__any"));
    p3->info.typ = xml;
    p3->info.minOccurs = 0;
    p3 = enter(t, lookup("__type"));
    p3->info.typ = mkint();
    p3->info.minOccurs = 0;
    p3 = enter(t, lookup("fault"));
    p3->info.typ = mkpointer(mkvoid());
    p3->info.minOccurs = 0;
    custom_fault = 0;
  }
  s4 = lookup("SOAP_ENV__Reason");
  p4 = entry(classtable, s4);
  if (!p4 || !p4->info.typ->ref)
  { t = mktable((Table*)0);
    if (!p4)
    { p4 = enter(classtable, s4);
      p4->info.typ = mkstruct(t, 4);
      p4->info.typ->id = s4;
    }
    else
      p4->info.typ->ref = t;
    p3 = enter(t, lookup("SOAP_ENV__Text"));
    p3->info.typ = mkstring();
    p3->info.minOccurs = 0;
  }
  s3 = lookup("SOAP_ENV__Fault");
  p3 = entry(classtable, s3);
  if (!p3 || !p3->info.typ->ref)
  { t = mktable(NULL);
    if (!p3)
    { p3 = enter(classtable, s3);
      p3->info.typ = mkstruct(t, 9*4);
      p3->info.typ->id = s3;
    }
    else
      p3->info.typ->ref = t;
    p3 = enter(t, lookup("faultcode"));
    p3->info.typ = qname;
    p3->info.minOccurs = 0;
    p3 = enter(t, lookup("faultstring"));
    p3->info.typ = mkstring();
    p3->info.minOccurs = 0;
    p3 = enter(t, lookup("faultactor"));
    p3->info.typ = mkstring();
    p3->info.minOccurs = 0;
    p3 = enter(t, lookup("detail"));
    p3->info.typ = mkpointer(p2->info.typ);
    p3->info.minOccurs = 0;
    p3 = enter(t, s1);
    p3->info.typ = mkpointer(p1->info.typ);
    p3->info.minOccurs = 0;
    p3 = enter(t, s4);
    p3->info.typ = mkpointer(p4->info.typ);
    p3->info.minOccurs = 0;
    p3 = enter(t, lookup("SOAP_ENV__Node"));
    p3->info.typ = mkstring();
    p3->info.minOccurs = 0;
    p3 = enter(t, lookup("SOAP_ENV__Role"));
    p3->info.typ = mkstring();
    p3->info.minOccurs = 0;
    p3 = enter(t, lookup("SOAP_ENV__Detail"));
    p3->info.typ = mkpointer(p2->info.typ);
    p3->info.minOccurs = 0;
  }
}

static void
add_soap(void)
{ Symbol *s = lookup("soap");
  p = enter(classtable, s);
  p->info.typ = mkstruct(NULL, 0);
  p->info.typ->transient = -2;
  p->info.typ->id = s;
}

static void
add_XML(void)
{ Symbol *s = lookup("_XML");
  s->token = TYPE;
  p = enter(typetable, s);
  xml = p->info.typ = mksymtype(mkstring(), s);
  p->info.sto = Stypedef;
}

static void
add_qname(void)
{ Symbol *s = lookup("_QName");
  s->token = TYPE;
  p = enter(typetable, s);
  qname = p->info.typ = mksymtype(mkstring(), s);
  p->info.sto = Stypedef;
}

static void
add_header(Table *gt)
{ Table *t;
  Entry *p;
  Symbol *s = lookup("SOAP_ENV__Header");
  imported = NULL;
  p = entry(classtable, s);
  if (!p || !p->info.typ->ref)
  { t = mktable((Table*)0);
    if (!p)
      p = enter(classtable, s);
    p->info.typ = mkstruct(t, 0);
    p->info.typ->id = s;
    custom_header = 0;
  }
}

static void
add_response(Entry *fun, Entry *ret)
{ Table *t;
  Entry *p, *q;
  Symbol *s;
  size_t n = strlen(fun->sym->name);
  char *r = (char *)emalloc(n+9);
  strcpy(r, fun->sym->name);
  strcat(r, "Response");
  if (!(s = lookup(r)))
    s = install(r, ID);
  free(r);
  t = mktable((Table*)0);
  q = enter(t, ret->sym);
  q->info = ret->info;
  if (q->info.typ->type == Treference)
    q->info.typ = (Tnode*)q->info.typ->ref;
  p = enter(classtable, s);
  p->info.typ = mkstruct(t, 4);
  p->info.typ->id = s;
  fun->info.typ->response = p;
}

static void
add_result(Tnode *typ)
{ Entry *p;
  if (!typ->ref || !((Tnode*)typ->ref)->ref)
  { semwarn("response struct/class must be declared before used in function prototype");
    return;
  }
  for (p = ((Table*)((Tnode*)typ->ref)->ref)->list; p; p = p->next)
    if (p->info.sto & Sreturn)
      return;
  for (p = ((Table*)((Tnode*)typ->ref)->ref)->list; p; p = p->next)
  { if (p->info.typ->type != Tfun && !(p->info.sto & Sattribute) && !is_transient(p->info.typ) && !(p->info.sto & (Sprivate|Sprotected)))
      p->info.sto = (Storage)((int)p->info.sto | (int)Sreturn);
      return;
  }
}
