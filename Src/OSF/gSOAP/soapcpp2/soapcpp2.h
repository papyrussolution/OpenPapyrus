/*
        soapcpp2.h

        Common declarations.

   --------------------------------------------------------------------------------
   gSOAP XML Web services tools
   Copyright (C) 2000-2012, Robert van Engelen, Genivia Inc. All Rights Reserved.
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
#ifndef __SOAPCPP2_H
#define __SOAPCPP2_H
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "error2.h"

#ifndef VERSION
 #define VERSION "2.8.8" /* Current version */
 #define GSOAP_VERSION 20808
#endif
#ifdef WIN32
 #pragma warning(disable : 4996)
 #ifndef WITH_BISON
  #define WITH_BISON
 #endif
#endif

/* #define DEBUG */ /* uncomment to debug */

#ifdef DEBUG
 #define        check(expr, msg) (void)((expr) ? 0 : (progerror(msg, __FILE__, __LINE__), 0))
 #define DBGLOG(DBGCMD) { DBGCMD; }
#else
 #define check(expr, msg) (void)(expr)
 #define DBGLOG(DBGCMD)
#endif

#ifdef WIN32
 #ifdef WITH_BISON
  #ifdef WIN32_WITHOUT_SOLARIS_FLEX
   #define yyparse soapcpp2parse
   #define yylex soapcpp2lex
   #define yyerror soapcpp2error
   #define yylval soapcpp2lval
   #define yychar soapcpp2char
   #define yydebug soapcpp2debug
   #define yynerrs soapcpp2nerrs
   #define yylineno soapcpp2lineno
   #define yytext soapcpp2text
   #define yyin soapcpp2in
   #define yywrap soapcpp2wrap
  #endif
 #endif
#endif

#ifdef WIN32
	#define SOAP_PATHCAT "\\"
	#define SOAP_PATHSEP ";"
	#define LONG64 __int64
#else
	#define SOAP_PATHCAT "/"
	#define SOAP_PATHSEP ":"
	#define LONG64 long long
#endif
#if defined(WIN32)
	#define SOAP_LONG_FORMAT "%I64d"
	#define SOAP_ULONG_FORMAT "%I64u"
	#define SOAP_XLONG_FORMAT "%I64x"
#elif defined(TRU64)
	#define SOAP_LONG_FORMAT "%ld"
	#define SOAP_ULONG_FORMAT "%lu"
	#define SOAP_XLONG_FORMAT "%lx"
#endif
#ifndef SOAP_LONG_FORMAT
	#define SOAP_LONG_FORMAT "%lld"        /* printf format for 64 bit ints */
#endif
#ifndef SOAP_ULONG_FORMAT
	#define SOAP_ULONG_FORMAT "%llu"       /* printf format for unsigned 64 bit ints */
#endif
#ifndef SOAP_XLONG_FORMAT
	#define SOAP_XLONG_FORMAT "%llx"       /* printf format for unsigned 64 bit hex ints */
#endif

extern int yylineno;
typedef enum Bool { 
	False, 
	True 
} Bool;

typedef int Token;

typedef enum tagType {       
	Tnone,
	Tvoid,          /* primitive types */
	Tchar,
	Twchar,
	Tshort,
	Tint,
	Tlong,
	Tllong,
	Tfloat,
	Tdouble,
	Tldouble,
	Tuchar,
	Tushort,
	Tuint,
	Tulong,
	Tullong,
	Ttime,
	Tenum,
	Tclass,         /* compound types */
	Tstruct,
	Tunion,
	Tpointer,
	Treference,
	Tarray,
	Ttemplate,
	Tfun
} Type;

#define TYPES (Tfun+1)  /* number of type (operators) enumerated above */

typedef enum tagStorage {       
	Snone           = 0,
	Sauto           = 0x000001,
	Sregister       = 0x000002,
	Sstatic         = 0x000004,
	Sextern         = 0x000008,
	Stypedef        = 0x000010,
	Svirtual        = 0x000020,
	Sconst          = 0x000040,
	Sconstobj       = 0x000080,
	Sconstptr       = 0x000100,
	Sfriend         = 0x000200,
	Sinline         = 0x000400,
	Sabstract       = 0x000800,
	SmustUnderstand = 0x001000,
	Sreturn         = 0x002000,
	Sattribute      = 0x004000,
	Sspecial        = 0x008000,
	Sexplicit       = 0x010000,
	Sprivate        = 0x020000,
	Sprotected      = 0x040000
} Storage;

typedef enum Level { INTERNAL, GLOBAL, PARAM, LOCAL } Level;

#define mknone()        mktype(Tnone,     NULL, 0)
#define mkvoid()        mktype(Tvoid,     NULL, 0)
#define mkbool()        mktype(Tenum,     booltable, 4)
#define mkchar()        mktype(Tchar,     NULL, 1)
#define mkwchart()      mktype(Twchar,    NULL, 4)
#define mkshort()       mktype(Tshort,    NULL, 2)
#define mkint()         mktype(Tint,      NULL, 4)
#define mklong()        mktype(Tlong,     NULL, 8)
#define mkllong()       mktype(Tllong,    NULL, 8)
#define mkfloat()       mktype(Tfloat,    NULL, 4)
#define mkdouble()      mktype(Tdouble,   NULL, 8)
#define mkldouble()     mktype(Tldouble,  NULL, 16)     /* long double */
#define mkuchar()       mktype(Tuchar,    NULL, 1)      /* unsigned char */
#define mkushort()      mktype(Tushort,   NULL, 2)      /* unsigned short */
#define mkuint()        mktype(Tuint,     NULL, 4)      /* unsigned int */
#define mkulong()       mktype(Tulong,    NULL, 8)      /* unsigned long */
#define mkullong()      mktype(Tullong,   NULL, 8)      /* unsigned long */
#define mktimet()       mktype(Ttime,     NULL, 4)
#define mkenum(t)       mktype(Tenum,     t,    4)
#define mkmask(t)       mktype(Tenum,     t,    8)
#define mkpointer(t)    mktype(Tpointer,  t,    4)
#define mkreference(t)  mktype(Treference, t,    4)
#define mkclass(t, w)   mktype(Tclass,    t,    w)
#define mkstruct(t, w)  mktype(Tstruct,   t,    w)
#define mkunion(t, w)   mktype(Tunion,    t,    w)
#define mkarray(t, w)   mktype(Tarray,    t,    w)
#define mkfun(t)        mktype(Tfun,      t,    0)
#define mkstring()      mkpointer(mkchar())

#define MINLONG64 (0x8000000000000000LL)
#define MAXLONG64 (0x7FFFFFFFFFFFFFFFLL)

typedef struct tagSymbol {       
	char * name;
	Token token;
	struct tagSymbol * next; 
} Symbol;

Symbol * install(const char *, Token), *lookup(const char *), *gensym(const char *), *gensymidx(const char *, int);

typedef struct tagTnode {       
	Type type;
	void * ref;
	Symbol * id;    /* struct/class/union/enum name */
	Symbol * base;  /* base class name */
	Symbol * sym;   /* typedef name */
	struct tagEntry * response; /* funcs only: points to response struct */
	int width;
	int transient;
	const char * imported;
	struct tagTnode * next;
	Bool generated;
	Bool classed;           /* class qualified */
	Bool wsdl;
	int num;
	char * pattern;
	LONG64 minLength;
	LONG64 maxLength; 
} Tnode;

typedef union tagValue {
	LONG64 i;
	double r;
	const char * s;
} Value;

typedef struct tagIDinfo {
	Tnode * typ;
	Storage sto;
	Bool hasval;            /* if true, identifier is constant */
	Value val;              /* ... with this value */
	int offset;
	LONG64 minOccurs;
	LONG64 maxOccurs;
} IDinfo;

typedef struct tagEntry {
	Symbol * sym;
	Symbol * tag;
	IDinfo info;
	Level level;
	int lineno;
	struct tagEntry * next;
} Entry;

typedef struct tagTable {
	Symbol * sym;
	Level level;
	Entry * list;
	struct tagTable * prev;
} Table;

typedef struct tagFNinfo {
	Tnode * ret;
	Table * args;
} FNinfo;

typedef struct tagNode {
	Tnode * typ;
	Storage sto;
	Bool hasval;            /* if true, this node has a constant value */
	Value val;              /* ... this is the value */
	LONG64 minOccurs;
	LONG64 maxOccurs;
	char * pattern;
	LONG64 minLength;
	LONG64 maxLength;
} Node;

#define ACTION                  0x00
#define REQUEST_ACTION          0x01
#define RESPONSE_ACTION         0x02
#define FAULT_ACTION            0x04
#define HDRIN                   0x10
#define HDROUT                  0x20
#define MIMEIN                  0x40
#define MIMEOUT                 0x80
#define COMMENT                 0x100
#define ENCODING                0x200
#define RESPONSE_ENCODING       0x400
#define STYLE                   0x800
#define FAULT                   0x1000

typedef struct Data {
	struct Data * next;
	char * name;
	char * text; 
} Data;

typedef struct Method {       
	struct Method * next;
	char * name;
	short mess; /* see #defines above */
	char * part; 
} Method;

typedef struct Service {       
	struct Service * next;
	char * ns;
	char * name;
	char * porttype;
	char * portname;
	char * binding;
	char * definitions;
	char * transport;
	char * URL;
	char * executable;
	char * import;
	char * URI;
	char * URI2;
	char * WSDL;
	char * style;
	char * encoding;
	int xsi_type;
	char * elementForm;
	char * attributeForm;
	char * documentation;
	struct Method * list;
	struct Data * data; 
} Service;

typedef struct Pragma {
	struct Pragma * next;
	char * pragma; 
} Pragma;

Entry * enter(Table *, Symbol *);
Entry  * entry(Table*, Symbol *);
Entry  * reenter(Table*, Symbol *);
Entry  * enumentry(Symbol *);
int merge(Table *, Table *);
Table * mktable(Table *);
Tnode * mkmethod(Tnode *, Table *);
char * emalloc(size_t);
Tnode * mktype(Type, void *, int);
Tnode * mksymtype(Tnode *, Symbol *);
Tnode * mktemplate(Tnode *, Symbol *);
void compile(Table *);
void freetable(Table *);
Entry * unlinklast(Table *);

int is_transient(const Tnode *);
int is_response(Tnode *);
extern Table * typetable;
extern Table * enumtable;
extern Table * classtable;
extern Table * booltable;
extern Table * templatetable;
extern FILE * fmsg;
extern int aflag;
extern int bflag;
extern int vflag;
extern int wflag;
extern int cflag;
extern int Cflag;
extern int eflag;
extern unsigned long fflag;
extern int iflag;
extern int jflag;
extern int kflag;
extern int mflag;
extern int nflag;
extern int nflag;
extern int lflag;
extern int Lflag;
extern int sflag;
extern int Sflag;
extern int Tflag;
extern int tflag;
extern int uflag;
extern int xflag;
extern int yflag;
extern int zflag;
extern char dirpath[1024];
extern char filename[1024];
extern char * prefix;
extern char * importpath;
extern int custom_header;
extern int custom_fault;
extern Pragma * pragmas;
extern Service * services;
extern char * namespaceid;
extern int transient;
extern int imports;
extern char * imported;
extern int typeNO;

extern char * envURI;
extern char * encURI;
extern char * rpcURI;
extern char * xsiURI;
extern char * xsdURI;

#endif // __SOAPCPP2_H
