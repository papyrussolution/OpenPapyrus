/*
        symbol2.c

        Symbol table handling, type analysis, and code generation.

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
#include <slib.h>
#undef CONST // Используется как идент токена в синтаксическом анализе
#undef VOID  // Используется как идент токена в синтаксическом анализе
#include "soapcpp2.h"
#include "soapcpp2_yacc.tab.cpp.h"

extern const char * S_SoapEnvNs;
extern const char * S_SoapEncNs;

char * envURI = "http://schemas.xmlsoap.org/soap/envelope/";
char * encURI = "http://schemas.xmlsoap.org/soap/encoding/";
char * rpcURI = "http://www.w3.org/2003/05/soap-rpc";
char * xsiURI = "http://www.w3.org/2001/XMLSchema-instance";
char * xsdURI = "http://www.w3.org/2001/XMLSchema";
char * tmpURI = "http://tempuri.org";
static Symbol * symlist = (Symbol *)0;  /* pointer to linked list of symbols */
static Symbol * nslist = (Symbol *)0;   /* pointer to linked list of namespace prefix symbols */
static Tnode * Tptr[TYPES];
Service * services = NULL;
FILE * fout;
FILE * fhead;
FILE * fclient;
FILE * fserver;
FILE * fheader;
FILE * flib;
FILE * fmatlab;
FILE * fmheader;
int partnum = 0;
int typeNO = 1; /* unique no. assigned to all types */
static int is_anytype_flag = 0; /* anytype is used */
static int has_nsmap = 0;

int tagcmp(const char * s, const char * t);
int tagncmp(const char * s, const char * t, size_t n);
long minlen(Tnode * typ);
long maxlen(Tnode * typ);
int is_soap12(const char *);
int has_detail_string(void);
int has_Detail_string(void);
void needs_lang(Entry * e);
int is_mutable(Tnode * typ);
int is_header_or_fault(Tnode * typ);
int is_body(Tnode * typ);
int is_volatile(Tnode * typ);
int is_untyped(Tnode * typ);
int is_primclass(Tnode * typ);
int is_imported(Tnode * typ);
int is_template(Tnode * typ);
int is_mask(Tnode * typ);
int is_attachment(Tnode * typ);
int is_void(Tnode * typ);
int has_external(Tnode * typ);
int has_volatile(Tnode * typ);
int is_invisible(const char * name);
int is_invisible_empty(Tnode * p);
int is_eq_nons(const char * s, const char * t);
int is_eq(const char * s, const char * t);
int is_item(Entry * p);
const char * cstring(const char *);
const char * xstring(const char *);
/*
   install - add new symbol
*/
Symbol * install(const char * name, Token token)
{
	Symbol * p = reinterpret_cast<Symbol *>(emalloc(sizeof(Symbol)));
	p->name = emalloc(strlen(name)+1);
	strcpy(p->name, name);
	p->token = token;
	p->next = symlist;
	symlist = p;
	return p;
}
/*
   lookup - search for an identifier's name. If found, return pointer to symbol table entry. Return pointer 0 if not found.
 */
Symbol * lookup(const char * name)
{
	for(Symbol * p = symlist; p; p = p->next)
		if(sstreq(p->name, name))
			return p;
	return NULL;
}
/*
   gensymidx - generate new symbol from base name and index
 */
Symbol * gensymidx(const char * base, int idx)
{
	char buf[1024];
	sprintf(buf, "%s_%d", base, idx);
	Symbol * s = lookup(buf);
	return s ? s : install(buf, ID);
}
/*
   gensym - generate new symbol from base name
 */
Symbol * gensym(const char * base)
{
	static int num = 1;
	return gensymidx(base, num++);
}
/*
   mktable - make a new symbol table with a pointer to a previous table
 */
Table * mktable(Table * table)
{
	Table * p = reinterpret_cast<Table *>(emalloc(sizeof(Table)));
	p->sym = lookup("/*?*/");
	p->list = (Entry *)0;
	p->level = (table == (Table *)0) ? INTERNAL : (Level)(table->level+1);
	p->prev = table;
	return p;
}
/*
   mkmethod - make a new method by calling mktype
 */
Tnode * mkmethod(Tnode * ret, Table * args)
{
	FNinfo * fn = reinterpret_cast<FNinfo *>(emalloc(sizeof(FNinfo)));
	fn->ret = ret;
	fn->args = args;
	return mktype(Tfun, fn, 0);
}
/*
   freetable - free space by removing a table
 */
void freetable(Table * table)
{
	if(table != (Table *)0) {
		Entry * p, * q;
		for(p = table->list; p != (Entry *)0; p = q) {
			q = p->next;
			SAlloc::F(p);
		}
		SAlloc::F(table);
	}
}
/*
   unlinklast - unlink last entry added to table
 */
Entry * unlinklast(Table * table)
{
	Entry ** p, * q;
	if(table == (Table *)0)
		return (Entry *)0;
	for(p = &table->list; *p != (Entry *)0 && (*p)->next != (Entry *)0; p = &(*p)->next) 
		;
	q = *p;
	*p = (Entry *)0;
	return q;
}

/*
   enter - enter a symbol in a table. Error if already in the table
 */
Entry * enter(Table * table, Symbol * sym)
{
	Entry * p, * q = NULL;
again:
	for(p = table->list; p; q = p, p = p->next) {
		if(p->sym == sym && p->info.typ->type != Tfun) {
			char * s;
			sprintf(errbuf,
				"Duplicate declaration of '%s' (already declared at line %d), changing conflicting identifier name to new name '%s_'. Note: this problem may be caused by importing invalid XML schemas",
				sym->name, p->lineno, sym->name);
			semwarn(errbuf);
			s = static_cast<char *>(emalloc(strlen(sym->name)+2));
			strcpy(s, sym->name);
			strcat(s, "_");
			sym = lookup(s);
			SETIFZ(sym, install(s, ID));
			SAlloc::F(s);
			goto again;
		}
	}
	p = reinterpret_cast<Entry *>(emalloc(sizeof(Entry)));
	p->sym = sym;
	p->tag = NULL;
	p->info.typ = NULL;
	p->info.sto = Snone;
	p->info.hasval = False;
	p->info.minOccurs = 1;
	p->info.maxOccurs = 1;
	p->info.offset = 0;
	p->level = table->level;
	p->lineno = yylineno;
	p->next = NULL;
	if(!q)
		table->list = p;
	else
		q->next = p;
	return p;
}
/*
   entry - return pointer to table entry of a symbol
 */
Entry * entry(Table * table, Symbol * sym)
{
	for(Table * t = table; t; t = t->prev)
		for(Entry * p = t->list; p; p = p->next)
			if(p->sym == sym)
				return p;
	return NULL;
}
/*
   reenter - re-enter a symbol in a table.
 */
Entry * reenter(Table * table, Symbol * sym)
{
	Entry * p, * q = NULL;
	for(p = table->list; p; q = p, p = p->next)
		if(p->sym == sym)
			break;
	if(p && p->next) {
		if(q)
			q->next = p->next;
		else
			table->list = p->next;
		for(q = p->next; q->next; q = q->next)
			;
		q->next = p;
		p->next = NULL;
	}
	return p;
}
/*
   merge - append two tables if members are not duplicated
 */
int merge(Table * dest, Table * src)
{
	for(Entry * p = src->list; p; p = p->next) {
		Entry * q = entry(dest, p->sym);
		if(!q || q->info.typ != p->info.typ) {
			q = enter(dest, p->sym);
			q->info = p->info;
		}
	}
	return 0;
}

Entry * enumentry(Symbol * sym)
{
	for(Table * t = enumtable; t; t = t->prev) {
		for(Entry * p = t->list; p; p = p->next) {
			Entry * q = entry((Table *)p->info.typ->ref, sym);
			if(q)
				return q;
		}
	}
	return NULL;
}

char * soap_type(Tnode *);
char * c_storage(int/*Storage*/);
char * c_init(Entry *);
char * c_type(Tnode *);
char * c_type_id(Tnode *, char *);
char * xsi_type_cond(Tnode *, int);
char * xsi_type(Tnode *);
char * xsi_type_cond_u(Tnode *, int);
char * xsi_type_u(Tnode *);
const char * the_type(Tnode *);
char * wsdl_type(Tnode *, char *);
char * base_type(Tnode *, char *);
const char * xml_tag(Tnode *);
char * ns_qualifiedElement(Tnode *);
char * ns_qualifiedAttribute(Tnode *);
char * ns_convert(char *);
char * ns_add(char *, char *);
char * ns_remove(char *);
char * ns_remove1(char *);
char * ns_remove2(char *);
char * res_remove(char *);
char * ns_name(char *);
char * ns_cname(char *, char *);
char * ns_fname(char *);
int has_class(Tnode *);
int has_constructor(Tnode *);
int has_destructor(Tnode *);
int has_getter(Tnode *);
int has_setter(Tnode *);
int has_ns(Tnode *);
int has_ns_t(Tnode *);
int has_ns_eq(char *, char *);
char * strict_check(void);
char * ns_of(char *);
int eq_ns(char *, char *);
char * prefix_of(char *);
int has_offset(Tnode *);
int reflevel(Tnode * typ);
Tnode * reftype(Tnode * typ);
int is_response(Tnode *);
int is_XML(Tnode *);
int is_stdXML(Tnode * p);
Entry * get_response(Tnode *);
int is_primitive_or_string(Tnode *);
int is_primitive(Tnode *);
Entry * is_discriminant(Tnode *);
Entry * is_dynamic_array(Tnode *);
int is_transient(Tnode *);
int is_external(Tnode *);
int is_anyType(Tnode *);
int is_anyAttribute(Tnode *);
int is_binary(Tnode *);
int is_hexBinary(Tnode *);
int is_fixedstring(Tnode *);
int is_string(Tnode *);
int is_wstring(Tnode *);
int is_stdstring(Tnode *);
int is_stdwstring(Tnode *);
int is_stdstr(Tnode *);
int is_typedef(Tnode *);
int get_dimension(Tnode *);
char * has_soapref(Tnode *);
int is_document(const char *);
int is_literal(const char *);
int is_keyword(const char *);
int is_repetition(Entry *);
int is_choice(Entry *);
int is_sequence(Entry *);
int is_anytype(Entry *);
char * xsi_type_Tarray(Tnode *);
char * xsi_type_Darray(Tnode *);
void matlab_def_table(Table *);
void def_table(Table *);
void generate(Tnode *);
int no_of_var(Tnode *);
char * pointer_stuff(Tnode *);
void in_defs(Table *);
void in_defs2(Table *);
void in_defs3(Table *);
void out_defs(Table *);
void mark_defs(Table *);
void in_attach(Table *);
void out_attach(Table *);
void soap_serialize(Tnode *);
void soap_traverse(Tnode *);
void soap_default(Tnode *);
void soap_put(Tnode *);
void soap_out(Tnode *);
void soap_out_Darray(Tnode *);
void soap_get(Tnode *);
void soap_in(Tnode *);
void soap_in_Darray(Tnode *);
void soap_instantiate_class(Tnode *);
int get_Darraydims(Tnode * typ);
const char * nillable(Tnode * typ);
void soap_serve(Table *);
void generate_proto(Table *, Entry *);
//void generate_call(Table*, Entry*);
//void generate_server(Table*, Entry*);
void generate_header(Table *);
void get_namespace_prefixes(void);
void generate_schema(Table *);
void gen_schema(FILE *, Table *, char *, char *, int, int, char *, char *, char *, char *);
void gen_type_documentation(FILE * fd, Entry * type, char * ns);
int gen_member_documentation(FILE * fd, Symbol * type, Entry * member, char * ns);
void gen_schema_elements_attributes(FILE * fd, Table * t, char * ns, char * ns1, char * encoding, char * style);
void gen_schema_elements(FILE * fd, Tnode * p, char * ns, char * ns1);
int gen_schema_element(FILE * fd, Tnode * p, Entry * q, char * ns, char * ns1);
void gen_schema_attributes(FILE * fd, Tnode * p, char * ns, char * ns1);
void gen_wsdl(FILE *, Table *, char *, char *, char *, char *, char *, char *, char *);
void gen_nsmap(FILE *, Symbol *, char *);
void gen_proxy(FILE *, Table *, Symbol *, char *, char *, char *, char *, char *);
void gen_object(FILE *, Table *, Symbol *, char *, char *, char *, char *, char *);
void gen_proxy_header(FILE *, Table *, Symbol *, char *, char *, char *, char *, char *);
void gen_proxy_code(FILE *, Table *, Symbol *, char *, char *, char *, char *, char *);
void gen_object_header(FILE *, Table *, Symbol *, char *, char *, char *, char *, char *);
void gen_object_code(FILE *, Table *, Symbol *, char *, char *, char *, char *, char *);
void gen_method(FILE * fd, Table * table, Entry * method, int server);
void gen_params(FILE * fd, Table * params, Entry * result, int flag);
void gen_args(FILE * fd, Table * params, Entry * result, int flag);
void gen_call_method(FILE * fd, Table * table, Entry * method, char * name);
void gen_serve_method(FILE * fd, Table * table, Entry * param, char * name);
void gen_data(char *, Table *, char *, char *, char *, char *, char *, char *);
FILE * gen_env(char *, char *, int, Table *, char *, char *, char *, char *, char *, char *);
void gen_field(FILE *, int, Entry *, char *, char *, char *);
void gen_val(FILE *, int, Tnode *, char *, char *, char *);
void gen_atts(FILE *, int, Table *, char *);
char * get_mxClassID(Tnode *);
char * c_ident(Tnode *);

//char * t_ident(Tnode *);
//
// t_ident gives the name of a type in identifier format
//
static char * FASTCALL t_ident(Tnode * typ)
{
	char * p, * q;
	switch(typ->type) {
	    case Tnone: return "";
	    case Tvoid: return "void";
	    case Tchar: return "byte";
	    case Twchar: return "wchar";
	    case Tshort: return "short";
	    case Tint: return "int";
	    case Tlong: return "long";
	    case Tllong: return "LONG64";
	    case Tfloat: return "float";
	    case Tdouble: return "double";
	    case Tldouble: return "decimal";
	    case Tuchar: return "unsignedByte";
	    case Tushort: return "unsignedShort";
	    case Tuint: return "unsignedInt";
	    case Tulong: return "unsignedLong";
	    case Tullong: return "unsignedLONG64";
	    case Ttime: return "time";
	    case Tstruct:
	    case Tclass:
	    case Tunion:
	    case Tenum: return ((Table *)typ->ref == booltable) ? "bool" : res_remove(typ->id->name);
	    case Treference: return c_ident((Tnode *)typ->ref);
	    case Tpointer:
			if(is_string(typ))
				return "string";
			if(is_wstring(typ))
				return "wstring";
			p = static_cast<char *>(emalloc((10+strlen(q = c_ident((Tnode *)typ->ref)))*sizeof(char)));
			strcpy(p, "PointerTo");
			strcat(p, q);
			return p;
	    case Tarray:
			p = static_cast<char *>(emalloc((16+strlen(c_ident((Tnode *)typ->ref)))*sizeof(char)));
			if(((Tnode *)typ->ref)->width)
				sprintf(p, "Array%dOf%s", typ->width/((Tnode *)typ->ref)->width, c_ident((Tnode *)typ->ref));
			else
				sprintf(p, "ArrayOf%s", c_ident((Tnode *)typ->ref));
			return p;
	    case Ttemplate:
			if(typ->ref) {
				p = static_cast<char *>(emalloc((11+strlen(res_remove(typ->id->name))+strlen(q = c_ident((Tnode *)typ->ref)))*sizeof(char)));
				strcpy(p, res_remove(typ->id->name));
				strcat(p, "TemplateOf");
				strcat(p, q);
				return p;
			}
	    case Tfun: return "Function";
	}
	return "anyType";
}

//char * ident(char *);
static char * FASTCALL ident(char * name)
{
	char * s = strrchr(name, ':');
	return (s && *(s+1) && *(s-1) != ':') ? (s+1) : name;
}
/*
   mktype - make a (new) type with a reference to additional information and the
   width in bytes required to store objects of that type. A pointer to the
   type is returned which can be compared to check if types are identical.
 */
Tnode * mktype(Type type, void * ref, int width)
{
	Tnode * p;
	const int t = (transient != -2 || type > Ttime) ? transient : 0;
	if(type != Tstruct && type != Tclass && type != Tunion && (type != Tenum || ref)) {
		for(p = Tptr[type]; p; p = p->next) {
			if(p->ref == ref && p->sym == (Symbol *)0 && p->width == width && p->transient == t) {
				if(imported && !p->imported)
					p->imported = imported;
				return p;       /* type alrady exists in table */
			}
		}
	}
	p = reinterpret_cast<Tnode *>(emalloc(sizeof(Tnode))); // install new type 
	p->type = type;
	p->ref = ref;
	p->id = lookup("/*?*/");
	p->base = NULL;
	p->sym = (Symbol *)0;
	p->response = (Entry *)0;
	p->width = width;
	p->generated = False;
	p->classed = False;
	p->wsdl = False;
	p->next = Tptr[type];
	p->transient = t;
	p->imported = imported;
	p->pattern = NULL;
	p->minLength = MINLONG64;
	p->maxLength = MAXLONG64;
	p->num = typeNO++;
	Tptr[type] = p;
	DBGLOG(fprintf(stderr, "New type %s %s\n", c_type(p), p->imported));
	if(type == Tpointer && ((Tnode *)ref)->imported &&
	   (((Tnode *)ref)->type == Tenum || ((Tnode *)ref)->type == Tstruct || ((Tnode *)ref)->type == Tclass))
		p->imported = ((Tnode *)ref)->imported;
	else if(lflag && !is_transient(p) && (type == Tenum || type == Tstruct || type == Tclass))
		mkpointer(p);
	return p;
}

Tnode * mksymtype(Tnode * typ, Symbol * sym)
{
	Tnode * p = reinterpret_cast<Tnode *>(emalloc(sizeof(Tnode))); // install new type 
	p->type = typ->type;
	p->ref = typ->ref;
	p->id = (typ->id == lookup("/*?*/")) ? sym : typ->id;
	p->sym = sym;
	p->response = (Entry *)0;
	p->width = typ->width;
	p->generated = False;
	p->classed = True; /* copy of existing (generated) type */
	p->wsdl = False;
	p->next = Tptr[typ->type];
	p->transient = transient;
	p->imported = imported;
	p->pattern = NULL;
	p->minLength = MINLONG64;
	p->maxLength = MAXLONG64;
	p->num = typeNO++;
	Tptr[typ->type] = p;
	DBGLOG(fprintf(stderr, "New typedef %s %s\n", c_type(p), p->imported));
	return p;
}

Tnode * mktemplate(Tnode * typ, Symbol * id)
{
	Tnode * p;
	for(p = Tptr[Ttemplate]; p; p = p->next) {
		if(p->ref == typ && p->id == id && p->transient == transient) {
			if(imported && !p->imported)
				p->imported = imported;
			return p; // type alrady exists in table 
		}
	}
	p = reinterpret_cast<Tnode *>(emalloc(sizeof(Tnode))); // install new type 
	p->type = Ttemplate;
	p->ref = typ;
	p->id = id;
	p->sym = NULL;
	p->response = (Entry *)0;
	p->width = 0;
	p->generated = False;
	p->classed = False; /* copy of existing (generated) type */
	p->wsdl = False;
	p->next = Tptr[Ttemplate];
	p->transient = transient;
	p->imported = imported;
	p->pattern = NULL;
	p->minLength = MINLONG64;
	p->maxLength = MAXLONG64;
	p->num = typeNO++;
	Tptr[Ttemplate] = p;
	return p;
}

/*	DO NOT REMOVE OR ALTER (SEE LICENCE AGREEMENT AND COPYING.txt)	*/
void copyrightnote(FILE * fd, char * fn)
{
	fprintf(fd, "/* %s\n   Generated by gSOAP " VERSION " from %s\n\
\n\
Copyright(C) 2000-2012, Robert van Engelen, Genivia Inc. All Rights Reserved.\n\
The generated code is released under one of the following licenses:\n\
1) GPL or 2) Genivia's license for commercial use.\n\
This program is released under the GPL with the additional exemption that\n\
compiling, linking, and/or using OpenSSL is allowed.\n\
*/"                                                                                                                                                                                                                                                                                                                                                                                    ,
		fn,
		filename);
}

void banner(FILE * fd, const char * text)
{
	int i;
	fprintf(fd, "\n\n/");
	for(i = 0; i < 78; i++)
		fputc('*', fd);
	fprintf(fd, "\\\n *%76s*\n * %-75s*\n *%76s*\n\\", "", text, "");
	for(i = 0; i < 78; i++)
		fputc('*', fd);
	fprintf(fd, "/\n");
}

void identify(FILE * fd, char * fn)
{
	time_t t = time(NULL), * p = &t;
	char tmp[256];
	strftime(tmp, 256, "%Y-%m-%d %H:%M:%S GMT", gmtime(p));
	fprintf(fd, "\n\nSOAP_SOURCE_STAMP(\"@(#) %s ver " VERSION " %s\")\n", fn, tmp);
}

void compile(Table * table)
{
	Entry * p;
	Tnode * typ;
	Pragma * pragma;
	int classflag = 0;
	int found;
	int filenum;
	char * s;
	char base[1024];
	char soapStub[1024];
	char soapH[1024];
	char soapC[1024];
	char soapClient[1024];
	char soapServer[1024];
	char soapClientLib[1024];
	char soapServerLib[1024];
	char pathsoapStub[1024];
	char pathsoapH[1024];
	char pathsoapC[1024];
	char pathsoapClient[1024];
	char pathsoapServer[1024];
	char pathsoapClientLib[1024];
	char pathsoapServerLib[1024];
	char soapMatlab[1024];
	char pathsoapMatlab[1024];
	char soapMatlabHdr[1024];
	char pathsoapMatlabHdr[1024];
	found = 0;
	for(p = table->list; p; p = p->next)
		if(p->info.typ->type == Tfun && !(p->info.sto&Sextern))
			found = 1;
	if(!found)
		Sflag = Cflag = Lflag = 1;  /* no service operations were found */
	if(*dirpath)
		fprintf(fmsg, "Using project directory path: %s\n", dirpath);
	if(namespaceid) {
		prefix = namespaceid;
		fprintf(fmsg, "Using code namespace: %s\n", namespaceid);
	}
	strcpy(base, prefix);
	s = cflag ? ".c" : ".cpp";
	strcpy(soapMatlab, base);
	strcat(soapMatlab, "Matlab.c");
	strcpy(pathsoapMatlab, dirpath);
	strcat(pathsoapMatlab, soapMatlab );
	strcpy(soapMatlabHdr, base);
	strcat(soapMatlabHdr, "Matlab.h");
	strcpy(pathsoapMatlabHdr, dirpath);
	strcat(pathsoapMatlabHdr, soapMatlabHdr);
	strcpy(soapStub, base);
	strcat(soapStub, "Stub.h");
	strcpy(pathsoapStub, dirpath);
	strcat(pathsoapStub, soapStub);
	strcpy(soapH, base);
	strcat(soapH, "H.h");
	strcpy(pathsoapH, dirpath);
	strcat(pathsoapH, soapH);
	strcpy(soapC, base);
	if(fflag)
		strcat(soapC, "C_nnn");
	else
		strcat(soapC, "C");
	strcat(soapC, s);
	strcpy(pathsoapC, dirpath);
	strcat(pathsoapC, soapC);
	strcpy(soapClient, base);
	strcat(soapClient, "Client");
	strcat(soapClient, s);
	strcpy(pathsoapClient, dirpath);
	strcat(pathsoapClient, soapClient);
	strcpy(soapServer, base);
	strcat(soapServer, "Server");
	strcat(soapServer, s);
	strcpy(pathsoapServer, dirpath);
	strcat(pathsoapServer, soapServer);
	strcpy(soapClientLib, base);
	strcat(soapClientLib, "ClientLib");
	strcat(soapClientLib, s);
	strcpy(pathsoapClientLib, dirpath);
	strcat(pathsoapClientLib, soapClientLib);
	strcpy(soapServerLib, base);
	strcat(soapServerLib, "ServerLib");
	strcat(soapServerLib, s);
	strcpy(pathsoapServerLib, dirpath);
	strcat(pathsoapServerLib, soapServerLib);
	if(mflag) {
		fprintf(fmsg, "Saving %s Matlab definitions\n", pathsoapMatlab);
		fmatlab = fopen(pathsoapMatlab, "w");
		if(!fmatlab)
			execerror("Cannot write to file");
		copyrightnote(fmatlab, soapMatlab);
		fprintf(fmatlab, "\n#include \"%s\"\n", soapMatlabHdr);
		fprintf(fmsg, "Saving %s Matlab definitions\n", pathsoapMatlabHdr);
		fmheader = fopen(pathsoapMatlabHdr, "w");
		if(!fmheader)
			execerror("Cannot write to file");
		copyrightnote(fmheader, soapMatlabHdr);
		fprintf(fmheader, "\n#include \"mex.h\"\n#include \"%s\"\n", soapStub);
	}
	fprintf(fmsg, "Saving %s annotated copy of the input declarations\n", pathsoapStub);
	fheader = fopen(pathsoapStub, "w");
	if(!fheader)
		execerror("Cannot write to file");
	copyrightnote(fheader, soapStub);
	fprintf(fheader, "\n\n#ifndef %sStub_H\n\t#define %sStub_H", prefix, prefix);
	for(pragma = pragmas; pragma; pragma = pragma->next)
		fprintf(fheader, "\n%s", pragma->pragma);
	if(nflag)
		fprintf(fheader, "\n#ifndef WITH_NONAMESPACES\n\t#define WITH_NONAMESPACES\n#endif");
	if(namespaceid) {
		fprintf(fheader, "\n#ifndef WITH_NOGLOBAL\n\t#define WITH_NOGLOBAL\n#endif");
	}
	fprintf(fheader, "\n#include \"stdsoap2.h\"");
	fprintf(fheader, "\n#if GSOAP_VERSION != %d\n# error \"GSOAP VERSION MISMATCH IN GENERATED CODE: PLEASE REINSTALL PACKAGE\"\n#endif\n", GSOAP_VERSION);
	if(cflag)
		fprintf(fheader, "\n#ifdef __cplusplus\n\textern \"C\" {\n#endif");
	if(namespaceid)
		fprintf(fheader, "\n\nnamespace %s {", namespaceid);
	generate_header(table);
	generate_schema(table);
	if(!Sflag && !iflag && !jflag) {
		fprintf(fmsg, "Saving %s client calling stubs\n", pathsoapClient);
		fclient = fopen(pathsoapClient, "w");
		if(!fclient)
			execerror("Cannot write to file");
		copyrightnote(fclient, soapClient);
		fprintf(fclient, "\n\n#if defined(__BORLANDC__)");
		fprintf(fclient, "\n\t#pragma option push -w-8060");
		fprintf(fclient, "\n\t#pragma option push -w-8004");
		fprintf(fclient, "\n#endif");
		fprintf(fclient, "\n#include \"%sH.h\"", prefix);
		if(cflag)
			fprintf(fclient, "\n#ifdef __cplusplus\n\textern \"C\" {\n#endif");
		if(namespaceid)
			fprintf(fclient, "\n\nnamespace %s {", namespaceid);
		identify(fclient, soapClient);
		if(!Lflag) {
			flib = fopen(pathsoapClientLib, "w");
			if(!flib)
				execerror("Cannot write to file");
			copyrightnote(flib, soapClientLib);
			fprintf(fmsg, "Saving %s client stubs with serializers (use only for libs)\n",
				pathsoapClientLib);
			fprintf(flib, "\n\n/** Use this file in your project build instead of the two files %s and %s. This hides the serializer functions and avoids linking problems when linking multiple clients and servers. */\n",
				soapC, soapClient);
			fprintf(flib, "\n#ifndef WITH_NOGLOBAL\n#define WITH_NOGLOBAL\n#endif");
			fprintf(flib, "\n#define SOAP_FMAC3 static");
			fprintf(flib, "\n#include \"%s\"", soapC);
			fprintf(flib, "\n#include \"%s\"", soapClient);
			fprintf(flib, "\n\n/* End of %s */\n", soapClientLib);
			fclose(flib);
		}
	}
	if(!Cflag && !iflag && !jflag) {
		fprintf(fmsg, "Saving %s server request dispatcher\n", pathsoapServer);
		fserver = fopen(pathsoapServer, "w");
		if(!fserver)
			execerror("Cannot write to file");
		copyrightnote(fserver, soapServer);
		fprintf(fserver, "\n#if defined(__BORLANDC__)");
		fprintf(fserver, "\n\t#pragma option push -w-8060");
		fprintf(fserver, "\n\t#pragma option push -w-8004");
		fprintf(fserver, "\n#endif");
		fprintf(fserver, "\n#include \"%sH.h\"", prefix);
		if(cflag)
			fprintf(fserver, "\n#ifdef __cplusplus\n\textern \"C\" {\n#endif");
		if(namespaceid)
			fprintf(fserver, "\n\nnamespace %s {", namespaceid);
		identify(fserver, soapServer);
		if(!Lflag) {
			flib = fopen(pathsoapServerLib, "w");
			if(!flib)
				execerror("Cannot write to file");
			copyrightnote(flib, soapServerLib);
			fprintf(fmsg, "Saving %s server request dispatcher with serializers (use only for libs)\n",
				pathsoapServerLib);
			fprintf(flib, "\n\n/** Use this file in your project build instead of the two files %s and %s. This hides the serializer functions and avoids linking problems when linking multiple clients and servers. */\n",
				soapC, soapServer);
			fprintf(flib, "\n#ifndef WITH_NOGLOBAL\n\t#define WITH_NOGLOBAL\n#endif");
			fprintf(flib, "\n#define SOAP_FMAC3 static");
			fprintf(flib, "\n#include \"%s\"", soapC);
			fprintf(flib, "\n#include \"%s\"", soapServer);
			fprintf(flib, "\n\n/* End of %s */\n", soapServerLib);
			fclose(flib);
		}
	}
	if(!iflag && !jflag)
		soap_serve(table);
	fprintf(fmsg, "Saving %s interface declarations\n", pathsoapH);
	fhead = fopen(pathsoapH, "w");
	if(!fhead)
		execerror("Cannot write to file");
	copyrightnote(fhead, soapH);
	fprintf(fhead, "\n\n#ifndef %sH_H\n#define %sH_H", prefix, prefix);
	fprintf(fhead, "\n#include \"%s\"", soapStub);
	if(cflag)
		fprintf(fhead, "\n#ifdef __cplusplus\n\textern \"C\" {\n#endif");
	if(namespaceid)
		fprintf(fhead, "\n\nnamespace %s {", namespaceid);
	fprintf(fhead, "\n#ifndef WITH_NOIDREF");
	if(!cflag && !namespaceid)
		fprintf(fhead, "\n\n#ifdef __cplusplus\n\textern \"C\" {\n#endif");
	fprintf(fhead, "\nSOAP_FMAC3 void SOAP_FMAC4 soap_markelement(struct soap*, const void*, int);");
	fprintf(fhead, "\nSOAP_FMAC3 int SOAP_FMAC4 soap_putelement(struct soap*, const void*, const char*, int, int);");
	fprintf(fhead, "\nSOAP_FMAC3 void *SOAP_FMAC4 soap_getelement(struct soap*, int*);");
	if(!cflag && !namespaceid)
		fprintf(fhead, "\n\n#ifdef __cplusplus\n\t}\n#endif");
	fprintf(fhead, "\nSOAP_FMAC3 int FASTCALL soap_putindependent(struct soap*);");
	fprintf(fhead, "\nSOAP_FMAC3 int FASTCALL soap_getindependent(struct soap*);"); // @v9.8.8 SOAP_FMAC4-->FASTCALL
	fprintf(fhead, "\n#endif");
	fprintf(fhead, "\nSOAP_FMAC3 int FASTCALL soap_ignore_element(struct soap*);");

	classflag = 0;
	for(p = classtable->list; p; p = p->next) {
		if(p->info.typ->type == Tclass && p->info.typ->transient <= 0) {
			classflag = 1;
			break;
		}
	}
	if(classflag || Tptr[Ttemplate]) {
		if(cflag)
			semwarn("Option -c conflicts with the use of classes");
	}
	for(filenum = 1; partnum == 0; filenum++) {
		if(fflag) {
			char * t = strrchr(pathsoapC, '.');
			sprintf(t-3, "%03d", filenum);
			*t = '.';
			fprintf(fmsg, "Saving %s XML serializers (part %d)\n", pathsoapC, filenum);
			partnum = fflag; /* number of defs per file */
		}
		else {
			fprintf(fmsg, "Saving %s XML serializers\n", pathsoapC);
		      partnum = 1; 
		}
		fout = fopen(pathsoapC, "w");
		if(!fout)
			execerror("Cannot write to file");
		copyrightnote(fout, soapC);
		fprintf(fout, "\n\n#if defined(__BORLANDC__)");
		fprintf(fout, "\n#pragma option push -w-8060");
		fprintf(fout, "\n#pragma option push -w-8004");
		fprintf(fout, "\n#endif");

		fprintf(fout, "\n\n#include \"slib.h\""); // @v9.6.8
		fprintf(fout, "\n#include \"%sH.h\"", prefix);
		fprintf(fout, "\n#pragma hdrstop"); // @v9.6.8
		if(cflag)
			fprintf(fout, "\n\n#ifdef __cplusplus\nextern \"C\" {\n#endif");
		if(namespaceid)
			fprintf(fout, "\n\nnamespace %s {", namespaceid);
		identify(fout, soapC);
		if(filenum == 1) {
			if(!lflag) {
				fprintf(fout, "\n\n#ifndef WITH_NOGLOBAL");
				if((p = entry(classtable, lookup("SOAP_ENV__Header"))) && p->info.typ->type == Tstruct)
					fprintf(fout, "\n\nSOAP_FMAC3 void FASTCALL soap_serializeheader(struct soap * pSoap)\n{\n\tif(pSoap->header)\n\t\tsoap_serialize_SOAP_ENV__Header(pSoap, pSoap->header);\n}");
				else
					fprintf(fout, "\n\nSOAP_FMAC3 void FASTCALL soap_serializeheader(struct soap * pSoap)\n{\n\tif(pSoap->header)\n\t\tpSoap->header->soap_serialize(pSoap);\n}");
				fprintf(fout, "\n\nSOAP_FMAC3 int FASTCALL soap_putheader(struct soap * pSoap)\n{\n\tif(pSoap->header) {\n\t\tpSoap->part = SOAP_IN_HEADER;\n\t\tif(soap_out_SOAP_ENV__Header(pSoap, \"%s:Header\", 0, pSoap->header, NULL))\n\t\t\treturn pSoap->error;\n\t\tpSoap->part = SOAP_END_HEADER;\n\t}\n\treturn SOAP_OK;\n}", S_SoapEnvNs);
				fprintf(fout, "\n\nSOAP_FMAC3 int SOAP_FMAC4 soap_getheader(struct soap * pSoap)\n{\n\tpSoap->part = SOAP_IN_HEADER;\n\tpSoap->header = soap_in_SOAP_ENV__Header(pSoap, \"%s:Header\", NULL, NULL);\n\tpSoap->part = SOAP_END_HEADER;\n\treturn pSoap->header == NULL;\n}", S_SoapEnvNs);
				if(cflag) {
					fprintf(fout, "\n\nSOAP_FMAC3 void SOAP_FMAC4 soap_header(struct soap * pSoap)\n{\n\tif(!pSoap->header) {\n\t\tif((pSoap->header = static_cast<struct SOAP_ENV__Header *>(soap_malloc(pSoap, sizeof(struct SOAP_ENV__Header)))))\n\t\t\tsoap_default_SOAP_ENV__Header(pSoap, pSoap->header);\n\t}\n}");
				}
				else if((p = entry(classtable, lookup("SOAP_ENV__Fault"))) && p->info.typ->type == Tstruct) {
					fprintf(fout, "\n\nSOAP_FMAC3 void SOAP_FMAC4 soap_header(struct soap * pSoap)\n{\n\tif(!pSoap->header) {\n\t\tif((pSoap->header = soap_new_SOAP_ENV__Header(pSoap, -1)))\n\t\t\tsoap_default_SOAP_ENV__Header(pSoap, pSoap->header);\n\t}\n}");
				}
				else {
					fprintf(fout, "\n\nSOAP_FMAC3 void SOAP_FMAC4 soap_header(struct soap * pSoap)\n{\n\tif(!pSoap->header) {\n\t\tif((pSoap->header = soap_new_SOAP_ENV__Header(pSoap, -1)))\n\t\t\tpSoap->header->soap_default(pSoap);\n\t}\n}"); }
				if(cflag) {
					fprintf(fout, "\n\nSOAP_FMAC3 void FASTCALL soap_fault(struct soap * pSoap)\n{\n\tif(!pSoap->fault) {\n\t\tpSoap->fault = static_cast<struct SOAP_ENV__Fault *>(soap_malloc(pSoap, sizeof(struct SOAP_ENV__Fault)));\n\t\tif(!pSoap->fault)\n\t\t\treturn;\n\t\tsoap_default_SOAP_ENV__Fault(pSoap, pSoap->fault);\n\t}\n\tif(pSoap->version == 2 && !pSoap->fault->SOAP_ENV__Code) {\n\t\tpSoap->fault->SOAP_ENV__Code = (struct SOAP_ENV__Code*)soap_malloc(pSoap, sizeof(struct SOAP_ENV__Code));\n\t\tsoap_default_SOAP_ENV__Code(pSoap, pSoap->fault->SOAP_ENV__Code);\n\t}\n\tif(pSoap->version == 2 && !pSoap->fault->SOAP_ENV__Reason) {\n\t\tpSoap->fault->SOAP_ENV__Reason = static_cast<struct SOAP_ENV__Reason *>(soap_malloc(pSoap, sizeof(struct SOAP_ENV__Reason)));\n\t\tsoap_default_SOAP_ENV__Reason(pSoap, pSoap->fault->SOAP_ENV__Reason);\n\t}\n}");
					fprintf(fout, "\n\nSOAP_FMAC3 void SOAP_FMAC4 soap_serializefault(struct soap * pSoap)\n{\n\tif(pSoap->fault)\n\t\tsoap_serialize_SOAP_ENV__Fault(pSoap, pSoap->fault);\n}");
				}
				else if((p = entry(classtable, lookup("SOAP_ENV__Fault"))) && p->info.typ->type == Tstruct) {
					fprintf(fout, "\n\nSOAP_FMAC3 void FASTCALL soap_fault(struct soap * pSoap)\n{\n\tif(!pSoap->fault) {\n\t\tpSoap->fault = soap_new_SOAP_ENV__Fault(pSoap, -1);\n\t\tif(!pSoap->fault)\n\t\t\treturn;\n\t\tsoap_default_SOAP_ENV__Fault(pSoap, pSoap->fault);\n\t}\n\tif(pSoap->version == 2 && !pSoap->fault->SOAP_ENV__Code) {\n\t\tpSoap->fault->SOAP_ENV__Code = soap_new_SOAP_ENV__Code(pSoap, -1);\n\t\tsoap_default_SOAP_ENV__Code(pSoap, pSoap->fault->SOAP_ENV__Code);\n\t}\n\tif(pSoap->version == 2 && !pSoap->fault->SOAP_ENV__Reason) {\n\t\tpSoap->fault->SOAP_ENV__Reason = soap_new_SOAP_ENV__Reason(pSoap, -1);\n\t\tsoap_default_SOAP_ENV__Reason(pSoap, pSoap->fault->SOAP_ENV__Reason);\n\t}\n}");
					fprintf(fout, "\n\nSOAP_FMAC3 void SOAP_FMAC4 soap_serializefault(struct soap * pSoap)\n{\n\tsoap_fault(pSoap);\n\tif(pSoap->fault)\n\t\tsoap_serialize_SOAP_ENV__Fault(pSoap, pSoap->fault);\n}");
				}
				else {
					fprintf(fout, "\n\nSOAP_FMAC3 void FASTCALL soap_fault(struct soap * pSoap)\n{\n\tif(!pSoap->fault) {\n\t\tpSoap->fault = soap_new_SOAP_ENV__Fault(pSoap, -1);\n\t\tpSoap->fault->soap_default(pSoap);\n\t}\n\tif(pSoap->version == 2 && !pSoap->fault->SOAP_ENV__Code) {\n\t\tpSoap->fault->SOAP_ENV__Code = soap_new_SOAP_ENV__Code(pSoap, -1);\n\t\tsoap_default_SOAP_ENV__Code(pSoap, pSoap->fault->SOAP_ENV__Code);\n\t}\n\tif(pSoap->version == 2 && !pSoap->fault->SOAP_ENV__Reason) {\n\t\tpSoap->fault->SOAP_ENV__Reason = soap_new_SOAP_ENV__Reason(pSoap, -1);\n\t\tsoap_default_SOAP_ENV__Reason(pSoap, pSoap->fault->SOAP_ENV__Reason);\n\t}\n}");
					fprintf(fout, "\n\nSOAP_FMAC3 void SOAP_FMAC4 soap_serializefault(struct soap * pSoap)\n{\n\tsoap_fault(pSoap);\n\tif(pSoap->fault)\n\t\tpSoap->fault->soap_serialize(pSoap);\n}"); 
				}
				if((p = entry(classtable, lookup("SOAP_ENV__Fault"))) && p->info.typ->type == Tstruct) {
					//fprintf(fout, "\n\nSOAP_FMAC3 int SOAP_FMAC4 soap_putfault(struct soap *soap)\n{\n\tif(soap->fault)\n\t\treturn soap_put_SOAP_ENV__Fault(soap, soap->fault, \"%s:Fault\", NULL);\n\treturn SOAP_OK;\n}", S_SoapEnvNs);
					fprintf(fout, "\n\nSOAP_FMAC3 int SOAP_FMAC4 soap_putfault(struct soap *soap)\n{\n\treturn soap->fault ? soap_put_SOAP_ENV__Fault(soap, soap->fault, \"%s:Fault\", NULL) : SOAP_OK;\n}", S_SoapEnvNs);
					fprintf(fout, "\n\nSOAP_FMAC3 int SOAP_FMAC4 soap_getfault(struct soap *soap)\n{\n\treturn (soap->fault = soap_get_SOAP_ENV__Fault(soap, NULL, \"%s:Fault\", NULL)) == NULL;\n}", S_SoapEnvNs);
				}
				else {
					fprintf(fout, "\n\nSOAP_FMAC3 int SOAP_FMAC4 soap_putfault(struct soap *soap)\n{\n\tsoap_fault(soap);\n\tif(soap->fault)\n\t\treturn soap->fault->soap_put(soap, \"%s:Fault\", NULL);\n\treturn SOAP_EOM;\n}", S_SoapEnvNs);
					fprintf(fout, "\n\nSOAP_FMAC3 int SOAP_FMAC4 soap_getfault(struct soap *soap)\n{\n\tsoap_fault(soap);\n\tif(soap->fault)\n\t\treturn soap->fault->soap_get(soap, \"%s:Fault\", NULL) == NULL;\n\treturn SOAP_EOM;\n}", S_SoapEnvNs);
				}
				fprintf(fout, "\n\nSOAP_FMAC3 const char ** SOAP_FMAC4 soap_faultcode(struct soap *soap)\n{\n\tsoap_fault(soap);\n\tif(soap->version == 2 && soap->fault->SOAP_ENV__Code)\n\t\treturn (const char**)&soap->fault->SOAP_ENV__Code->SOAP_ENV__Value;\n\treturn (const char**)&soap->fault->faultcode;\n}");
				if(cflag)
					fprintf(fout, "\n\nSOAP_FMAC3 const char ** SOAP_FMAC4 soap_faultsubcode(struct soap *soap)\n{\n\tsoap_fault(soap);\n\tif(soap->version == 2) {\n\t\tif(!soap->fault->SOAP_ENV__Code->SOAP_ENV__Subcode) {\n\t\t\tsoap->fault->SOAP_ENV__Code->SOAP_ENV__Subcode = static_cast<struct SOAP_ENV__Code *>(soap_malloc(soap, sizeof(struct SOAP_ENV__Code)));\n\t\t\tsoap_default_SOAP_ENV__Code(soap, soap->fault->SOAP_ENV__Code->SOAP_ENV__Subcode);\n\t\t}\n\t\treturn (const char**)&soap->fault->SOAP_ENV__Code->SOAP_ENV__Subcode->SOAP_ENV__Value;\n\t}\n\treturn (const char**)&soap->fault->faultcode;\n}");
				else
					fprintf(fout, "\n\nSOAP_FMAC3 const char ** SOAP_FMAC4 soap_faultsubcode(struct soap *soap)\n{\n\tsoap_fault(soap);\n\tif(soap->version == 2) {\n\t\tif(!soap->fault->SOAP_ENV__Code->SOAP_ENV__Subcode) {\n\t\t\tsoap->fault->SOAP_ENV__Code->SOAP_ENV__Subcode = soap_new_SOAP_ENV__Code(soap, -1);\n\t\t\tsoap_default_SOAP_ENV__Code(soap, soap->fault->SOAP_ENV__Code->SOAP_ENV__Subcode);\n\t\t}\n\t\treturn (const char**)&soap->fault->SOAP_ENV__Code->SOAP_ENV__Subcode->SOAP_ENV__Value;\n\t}\n\treturn (const char**)&soap->fault->faultcode;\n}");
				fprintf(fout, "\n\nSOAP_FMAC3 const char * SOAP_FMAC4 soap_check_faultsubcode(struct soap *soap)\n{\n\tsoap_fault(soap);\n\tif(soap->version == 2) {\n\t\tif(soap->fault->SOAP_ENV__Code && soap->fault->SOAP_ENV__Code->SOAP_ENV__Subcode)\n\t\t\treturn soap->fault->SOAP_ENV__Code->SOAP_ENV__Subcode->SOAP_ENV__Value;\n\t\treturn NULL;\n\t}\n\treturn soap->fault->faultcode;\n}");
				fprintf(fout, "\n\nSOAP_FMAC3 const char ** SOAP_FMAC4 soap_faultstring(struct soap *soap)\n{\n\tsoap_fault(soap);\n\tif(soap->version == 2)\n\t\treturn (const char**)&soap->fault->SOAP_ENV__Reason->SOAP_ENV__Text;\n\treturn (const char**)&soap->fault->faultstring;\n}");
				fprintf(fout, "\n\nSOAP_FMAC3 const char ** SOAP_FMAC4 soap_faultdetail(struct soap *soap)\n{\n\tsoap_fault(soap);");
				if(has_detail_string())
					fprintf(fout, "\n\tif(soap->version == 1) {\n\t\tif(!soap->fault->detail) {\n\t\t\tsoap->fault->detail = static_cast<struct SOAP_ENV__Detail *>(soap_malloc(soap, sizeof(struct SOAP_ENV__Detail)));\n\t\t\tsoap_default_SOAP_ENV__Detail(soap, soap->fault->detail);\n\t\t}\n\t\treturn (const char**)&soap->fault->detail->__any;\n\t}");
				if(has_Detail_string()) {
					if(cflag)
						fprintf(fout, "\n\tif(!soap->fault->SOAP_ENV__Detail) {\n\t\tsoap->fault->SOAP_ENV__Detail = static_cast<struct SOAP_ENV__Detail *>(soap_malloc(soap, sizeof(struct SOAP_ENV__Detail)));\n\t\tsoap_default_SOAP_ENV__Detail(soap, soap->fault->SOAP_ENV__Detail);\n\t}\n\treturn (const char**)&soap->fault->SOAP_ENV__Detail->__any;\n}");
					else
						fprintf(fout, "\n\tif(!soap->fault->SOAP_ENV__Detail) {\n\t\tsoap->fault->SOAP_ENV__Detail = soap_new_SOAP_ENV__Detail(soap, -1);\n\t\tsoap_default_SOAP_ENV__Detail(soap, soap->fault->SOAP_ENV__Detail);\n\t}\n\treturn (const char**)&soap->fault->SOAP_ENV__Detail->__any;\n}");
				}
				if(!has_detail_string() && !has_Detail_string())
					fprintf(fout, "\n\treturn NULL;\n}");
				fprintf(fout, "\n\nSOAP_FMAC3 const char * SOAP_FMAC4 soap_check_faultdetail(struct soap *soap)\n{\n\tsoap_fault(soap);");
				if(has_Detail_string())
					fprintf(fout, "\n\tif(soap->version == 2 && soap->fault->SOAP_ENV__Detail)\n\t\treturn soap->fault->SOAP_ENV__Detail->__any;");
				if(has_detail_string())
					fprintf(fout, "\n\tif(soap->fault->detail)\n\t\treturn soap->fault->detail->__any;");
				fprintf(fout, "\n\treturn NULL;\n}");
				fprintf(fout, "\n\n#endif");
				fprintf(fout, "\n\n#ifndef WITH_NOIDREF");
				fprintf(fout, "\nSOAP_FMAC3 int FASTCALL soap_getindependent(struct soap *soap)\n{"); // @v9.8.8 SOAP_FMAC4-->FASTCALL
				fprintf(fout, "\n\tint t;\n\tif(soap->version == 1) {\n\t\tfor(;;) {\n\t\t\tif(!soap_getelement(soap, &t))\n\t\t\t\tif(soap->error || soap_ignore_element(soap))\n\t\t\t\t\tbreak;\n\t\t}\n\t}");
				fprintf(fout, "\n\tif(oneof2(soap->error, SOAP_NO_TAG, SOAP_EOF))");
				fprintf(fout, "\n\t\tsoap->error = SOAP_OK;");
				fprintf(fout, "\n\treturn soap->error;");
				fprintf(fout, "\n}\n#endif");
				fprintf(fout, "\n\n#ifndef WITH_NOIDREF");
				if(!cflag && !namespaceid)
					fprintf(fout, "\n\n#ifdef __cplusplus\nextern \"C\" {\n#endif");
				fprintf(fout, "\nSOAP_FMAC3 void * SOAP_FMAC4 soap_getelement(struct soap *soap, int *type)\n{\n\t(void)type;");
				fprintf(fout, "\n\tif(soap_peek_element(soap))\n\t\treturn NULL;");
				fprintf(fout, "\n\tif(!*soap->id || !(*type = soap_lookup_type(soap, soap->id)))\n\t\t*type = soap_lookup_type(soap, soap->href);");
				fprintf(fout, "\n\tswitch(*type) {");
				DBGLOG(fprintf(stderr, "\n Calling in_defs( )."));
				fflush(fout);
				in_defs(table);
				DBGLOG(fprintf(stderr, "\n Completed in_defs( )."));
				fprintf(fout, "\n\tdefault:\n\t{\tconst char *t = soap->type;\n\t\tif(!*t)\n\t\t\tt = soap->tag;");
				fflush(fout);
				in_defs2(table);
				fprintf(fout, "\n\t\tt = soap->tag;");
				in_defs3(table);
				fprintf(fout, "\n\t}\n\t}\n\tsoap->error = SOAP_TAG_MISMATCH;\n\treturn NULL;\n}");
				if(!cflag && !namespaceid)
					fprintf(fout, "\n\n#ifdef __cplusplus\n}\n#endif");
				fprintf(fout, "\n#endif");
				fprintf(fout, "\n\nSOAP_FMAC3 int FASTCALL soap_ignore_element(struct soap *soap)\n{");
				fprintf(fout, "\n\tif(!soap_peek_element(soap))");
				fprintf(fout, " {\n\t\tint t;");
				fprintf(fout, "\n\t\tDBGLOG(TEST, SOAP_MESSAGE(fdebug, \"Unexpected element '%%s' in input (level=%%u, %%d)\\n\", soap->tag, soap->level, soap->body));");
				fprintf(fout, "\n\t\tif(soap->mustUnderstand && !soap->other)");
				fprintf(fout, "\n\t\t\treturn soap->error = SOAP_MUSTUNDERSTAND;");
				fprintf(fout, "\n\t\tif(((soap->mode & SOAP_XML_STRICT) && soap->part != SOAP_IN_HEADER) || !soap_match_tag(soap, soap->tag, \"%s:\"))\n\t\t{\tDBGLOG(TEST, SOAP_MESSAGE(fdebug, \"REJECTING element '%%s'\\n\", soap->tag));\n\t\t\treturn soap->error = SOAP_TAG_MISMATCH;\n\t\t}", S_SoapEnvNs);
				fprintf(fout, "\n\t\tif(!*soap->id || !soap_getelement(soap, &t))");
				fprintf(fout, "\n\t\t{\tsoap->peeked = 0;");
				fprintf(fout, "\n\t\t\tsoap->error = soap->fignore ? soap->fignore(soap, soap->tag) : SOAP_OK;");
				fprintf(fout, "\n\t\t\tDBGLOG(TEST, if(!soap->error) SOAP_MESSAGE(fdebug, \"IGNORING element '%%s'\\n\", soap->tag));");
				fprintf(fout, "\n\t\t\tif(!soap->error && soap->body) {");
				fprintf(fout, "\n\t\t\t\tsoap->level++;");
				fprintf(fout, "\n\t\t\t\twhile(!soap_ignore_element(soap))");
				fprintf(fout, "\n\t\t\t\t\t;");
				fprintf(fout, "\n\t\t\t\tif(soap->error == SOAP_NO_TAG)");
				fprintf(fout, "\n\t\t\t\t\tsoap->error = soap_element_end_in(soap, NULL);");
				fprintf(fout, "\n\t\t\t}");
				fprintf(fout, "\n\t\t}");
				fprintf(fout, "\n\t}");
				fprintf(fout, "\n\treturn soap->error;");
				fprintf(fout, "\n}");
				fprintf(fout, "\n\n#ifndef WITH_NOIDREF");
				fprintf(fout, "\nSOAP_FMAC3 int FASTCALL soap_putindependent(struct soap *soap)\n{");
				fprintf(fout, "\n\tif(soap->version == 1 && soap->encodingStyle && !(soap->mode & (SOAP_XML_TREE | SOAP_XML_GRAPH)))");
				fprintf(fout, "\n\t\tfor(int i = 0; i < SOAP_PTRHASH; i++)");
				fprintf(fout, "\n\t\t\tfor(struct soap_plist * pp = soap->pht[i]; pp; pp = pp->next)");
				fprintf(fout, "\n\t\t\t\tif(pp->mark1 == 2 || pp->mark2 == 2)");
				fprintf(fout, "\n\t\t\t\t\tif(soap_putelement(soap, pp->ptr, \"id\", pp->id, pp->type))\n\t\t\t\t\t\treturn soap->error;");
				fprintf(fout, "\n\treturn SOAP_OK;\n}\n#endif");
				fprintf(fout, "\n\n#ifndef WITH_NOIDREF");
				if(!cflag && !namespaceid)
					fprintf(fout, "\n\n#ifdef __cplusplus\nextern \"C\" {\n#endif");
				fprintf(fout, "\nSOAP_FMAC3 int SOAP_FMAC4 soap_putelement(struct soap *soap, const void *ptr, const char *tag, int id, int type)\n{\n\t(void)tag;");
				fprintf(fout, "\n\tswitch(type) {");
				fflush(fout);
				out_defs(table);
				fprintf(fout, "\n\t}\n\treturn SOAP_OK;\n}");
				if(!cflag && !namespaceid)
					fprintf(fout, "\n\n#ifdef __cplusplus\n}\n#endif");
				fprintf(fout, "\n#endif");
				fprintf(fout, "\n\n#ifndef WITH_NOIDREF");
				if(!cflag && !namespaceid)
					fprintf(fout, "\n\n#ifdef __cplusplus\nextern \"C\" {\n#endif");
				if(is_anytype_flag) {
					fprintf(fout, "\nSOAP_FMAC3 void SOAP_FMAC4 soap_markelement(struct soap *soap, const void *ptr, int type)\n{");
					// @v9.8.9 fprintf(fout, "\n\t(void)soap; (void)ptr; (void)type; /* appease -Wall -Werror */");
					fprintf(fout, "\n\tswitch(type) {");
					fflush(fout);
					mark_defs(table);
					fprintf(fout, "\n\t}\n}");
				}
				else {
					fprintf(fout, "\nSOAP_FMAC3 void SOAP_FMAC4 soap_markelement(struct soap *soap, const void *ptr, int type)\n{");
					// @v9.8.9 fprintf(fout, "\n\t(void)soap; (void)ptr; (void)type; /* appease -Wall -Werror */");
					fprintf(fout, "\n}"); 
				}
				if(!cflag && !namespaceid)
					fprintf(fout, "\n\n#ifdef __cplusplus\n}\n#endif");
				fprintf(fout, "\n#endif");

			}
			if(!cflag) {
				fprintf(fhead, "\n\nSOAP_FMAC3 void * SOAP_FMAC4 %s_instantiate(struct soap*, int, const char*, const char*, size_t*);", prefix);
				fprintf(fout, "\n\nSOAP_FMAC3 void * SOAP_FMAC4 %s_instantiate(struct soap *soap, int t, const char *type, const char *arrayType, size_t *n)\n{\n\t(void)type;\n\tswitch(t) {", prefix);
				if(classtable) {
					for(p = classtable->list; p; p = p->next) {
						if((p->info.typ->type == Tclass || p->info.typ->type == Tstruct) && !is_transient(p->info.typ)) {
							if(is_header_or_fault(p->info.typ) || is_body(p->info.typ))
								fprintf(fout, "\n#ifndef WITH_NOGLOBAL");
							fprintf(fout, "\n\tcase %s: return soap_instantiate_%s(soap, -1, type, arrayType, n);", soap_type(p->info.typ), c_ident(p->info.typ));
							if(is_header_or_fault(p->info.typ) || is_body(p->info.typ))
								fprintf(fout, "\n#endif");
						}
					}
				}
				if(typetable) {
					for(p = typetable->list; p; p = p->next) {
						if((p->info.typ->type == Tclass || p->info.typ->type == Tstruct) && !is_transient(p->info.typ)) {
							if(is_header_or_fault(p->info.typ) || is_body(p->info.typ))
								fprintf(fout, "\n#ifndef WITH_NOGLOBAL");
							fprintf(fout, "\n\tcase %s: return soap_instantiate_%s(soap, -1, type, arrayType, n);", soap_type(p->info.typ), c_ident(p->info.typ));
							if(is_header_or_fault(p->info.typ) || is_body(p->info.typ))
								fprintf(fout, "\n#endif");
						}
					}
				}
				for(typ = Tptr[Ttemplate]; typ; typ = typ->next)
					if(typ->ref && !is_transient(typ))
						fprintf(fout, "\n\tcase %s: return soap_instantiate_%s(soap, -1, type, arrayType, n);", soap_type(typ), c_ident(typ));
				fprintf(fout, "\n\t}\n\treturn NULL;\n}");
				fprintf(fhead, "\nSOAP_FMAC3 int SOAP_FMAC4 %s_fdelete(struct soap_clist*);", prefix);
				fprintf(fout, "\n\nSOAP_FMAC3 int SOAP_FMAC4 %s_fdelete(struct soap_clist *p)", prefix);
				fprintf(fout, "\n{\n\tswitch(p->type) {");
				if(classtable) {
					for(p = classtable->list; p; p = p->next) {
						if((p->info.typ->type == Tclass || p->info.typ->type == Tstruct) && !is_transient(p->info.typ)) {
							if(is_header_or_fault(p->info.typ) || is_body(p->info.typ))
								fprintf(fout, "\n#ifndef WITH_NOGLOBAL");
							fprintf(fout, "\n\tcase %s:", soap_type(p->info.typ));
							fprintf(fout, "\n\t\tif(p->size < 0) SOAP_DELETE(static_cast<%s *>(p->ptr)); else SOAP_DELETE_ARRAY(static_cast<%s *>(p->ptr));\n\t\tbreak;",
								c_type(p->info.typ), c_type(p->info.typ));
							if(is_header_or_fault(p->info.typ) || is_body(p->info.typ))
								fprintf(fout, "\n#endif");
						}
					}
				}
				if(typetable) {
					for(p = typetable->list; p; p = p->next) {
						if(p->info.typ->type == Tclass || p->info.typ->type == Tstruct) { /* && is_external(p->info.typ)) */
							if(is_header_or_fault(p->info.typ) || is_body(p->info.typ))
								fprintf(fout, "\n#ifndef WITH_NOGLOBAL");
							fprintf(fout, "\n\tcase %s:", soap_type(p->info.typ));
							fprintf(fout, "\n\t\tif(p->size < 0) SOAP_DELETE(static_cast<%s *>(p->ptr)); else SOAP_DELETE_ARRAY(static_cast<%s *>(p->ptr));\n\t\tbreak;",
								c_type(p->info.typ), c_type(p->info.typ));
							if(is_header_or_fault(p->info.typ) || is_body(p->info.typ))
								fprintf(fout, "\n#endif");
						}
					}
				}
				for(typ = Tptr[Ttemplate]; typ; typ = typ->next) {
					if(typ->ref && !is_transient(typ)) {
						fprintf(fout, "\n\tcase %s:", soap_type(typ));
						fprintf(fout, "\n\t\tif(p->size < 0) SOAP_DELETE(static_cast<%s *>(p->ptr)); else SOAP_DELETE_ARRAY(static_cast<%s *>(p->ptr));\n\t\tbreak;", c_type(typ), c_type(typ));
					}
				}
				fprintf(fout, "\n\tdefault:\treturn SOAP_ERR;");
				fprintf(fout, "\n\t}\n\treturn SOAP_OK;");
				fprintf(fout, "\n}");
				fprintf(fhead, "\nSOAP_FMAC3 void* SOAP_FMAC4 soap_class_id_enter(struct soap*, const char*, void*, int, size_t, const char*, const char*);");
				if(!lflag) {
					fprintf(fout, "\n\nSOAP_FMAC3 void* SOAP_FMAC4 soap_class_id_enter(struct soap *soap, const char *id, void *p, int t, size_t n, const char *type, const char *arrayType)");
					fprintf(fout, "\n{\n\treturn soap_id_enter(soap, id, p, t, n, 0, type, arrayType, %s_instantiate);\n}", prefix);
				}
				if(Tptr[Ttemplate]) {
					fprintf(fhead, "\n\nSOAP_FMAC3 void* SOAP_FMAC4 soap_container_id_forward(struct soap*, const char*, void*, size_t, int, int, size_t, unsigned int);");
					if(!lflag) {
						fprintf(fout, "\n\nSOAP_FMAC3 void* SOAP_FMAC4 soap_container_id_forward(struct soap *soap, const char *href, void *p, size_t len, int st, int tt, size_t n, unsigned int k)");
						fprintf(fout, "\n{\n\treturn soap_id_forward(soap, href, p, len, st, tt, n, k, %s_container_insert);\n}", prefix);
					}
					fprintf(fhead, "\n\nSOAP_FMAC3 void SOAP_FMAC4 %s_container_insert(struct soap*, int, int, void*, size_t, const void*, size_t);", prefix);
					if(!lflag) {
						fprintf(fout, "\n\nSOAP_FMAC3 void SOAP_FMAC4 %s_container_insert(struct soap *soap, int st, int tt, void *p, size_t len, const void *q, size_t n)", prefix);
						fprintf(fout, "\n#ifdef WIN32\n#pragma warning(push)\n#pragma warning(disable:4065)\n#endif");
						fprintf(fout, "\n{\n\t(void)soap; (void)st; (void)p; (void)len; (void)q; (void)n; /* appease -Wall -Werror */");
						fprintf(fout, "\n\tswitch(tt) {");
						for(typ = Tptr[Ttemplate]; typ; typ = typ->next) {
							if(typ->ref && !is_transient(typ)) {
								fprintf(fout, "\n\tcase %s:", soap_type(typ));
								fprintf(fout, "\n\t\tDBGLOG(TEST, SOAP_MESSAGE(fdebug, \"Container %s_container_insert type=%%d in %%d location=%%p object=%%p len=%%lu\\n\", st, tt, p, q, (unsigned long)len));", prefix);
								if(sstreq(typ->id->name, "std::vector") || sstreq(typ->id->name, "std::deque"))
									fprintf(fout, "\n\t\t(*(%s)p)[len] = *(%s)q;", c_type_id(typ, "*"), c_type_id((Tnode *)typ->ref, "*"));
								else
									fprintf(fout, "\n\t\t((%s)p)->insert(((%s)p)->end(), *(%s)q);", c_type_id(typ, "*"), c_type_id(typ, "*"), c_type_id((Tnode *)typ->ref, "*"));
								fprintf(fout, "\n\t\tbreak;");
							}
						}
						fprintf(fout, "\n\tdefault:\n\t\tDBGLOG(TEST, SOAP_MESSAGE(fdebug, \"Could not insert type=%%d in %%d\\n\", st, tt));");
						fprintf(fout, "\n\t}");
						fprintf(fout, "\n#ifdef WIN32\n#pragma warning(pop)\n#endif");
						fprintf(fout, "\n}");
					}
				}
			}
		}
		def_table(table);
		if(namespaceid)
			fprintf(fout, "\n\n} // namespace %s\n", namespaceid);
		if(cflag)
			fprintf(fout, "\n\n#ifdef __cplusplus\n}\n#endif");
		fprintf(fout, "\n\n#if defined(__BORLANDC__)");
		fprintf(fout, "\n#pragma option pop");
		fprintf(fout, "\n#pragma option pop");
		fprintf(fout, "\n#endif");
		fprintf(fout, "\n\n/* End of %s */\n", soapC);
		fclose(fout);
	}
	if(namespaceid)
		fprintf(fhead, "\n\n} // namespace %s\n", namespaceid);
	if(cflag)
		fprintf(fhead, "\n\n#ifdef __cplusplus\n}\n#endif");
	fprintf(fhead, "\n\n#endif");
	fprintf(fhead, "\n\n/* End of %s */\n", soapH);
	fclose(fhead);
	if(namespaceid)
		fprintf(fheader, "\n\n} // namespace %s\n", namespaceid);
	if(cflag)
		fprintf(fheader, "\n\n#ifdef __cplusplus\n}\n#endif");
	fprintf(fheader, "\n\n#endif");
	fprintf(fheader, "\n\n/* End of %s */\n", soapStub);
	fclose(fheader);
	if(mflag) {
		DBGLOG(fprintf(stderr, "\n Calling matlab_def_table( )."));
		matlab_def_table(table);
		DBGLOG(fprintf(stderr, "\n Completed matlab_def_table( )."));
		fclose(fmatlab);
		fclose(fmheader);
	}
	if(!Sflag && !iflag && !jflag) {
		if(namespaceid)
			fprintf(fclient, "\n\n} // namespace %s\n", namespaceid);
		if(cflag)
			fprintf(fclient, "\n\n#ifdef __cplusplus\n}\n#endif");
		fprintf(fclient, "\n\n#if defined(__BORLANDC__)");
		fprintf(fclient, "\n#pragma option pop");
		fprintf(fclient, "\n#pragma option pop");
		fprintf(fclient, "\n#endif");
		fprintf(fclient, "\n\n/* End of %s */\n", soapClient);
		fclose(fclient);
	}
	if(!Cflag && !iflag && !jflag) {
		if(namespaceid)
			fprintf(fserver, "\n\n} // namespace %s\n", namespaceid);
		if(cflag)
			fprintf(fserver, "\n\n#ifdef __cplusplus\n}\n#endif");
		fprintf(fserver, "\n\n#if defined(__BORLANDC__)");
		fprintf(fserver, "\n#pragma option pop");
		fprintf(fserver, "\n#pragma option pop");
		fprintf(fserver, "\n#endif");
		fprintf(fserver, "\n\n/* End of %s */\n", soapServer);
		fclose(fserver);
	}
}

void gen_class(FILE * fd, Entry * p)
{
	Entry * Eptr;
	Tnode * typ = p->info.typ;
	char * x = xsi_type(typ);
	if(!x || !*x)
		x = wsdl_type(typ, "");
	typ->classed = True;
	if(is_header_or_fault(typ) || is_body(typ))
		fprintf(fd, "\n#ifndef WITH_NOGLOBAL");
	if(typ->ref) {
		fprintf(fd, "\n#ifndef %s", soap_type(typ));
		fprintf(fd, "\n#define %s (%d)\n", soap_type(typ), typ->num);
	}
	else
		fprintf(fd, "\n");
	if(is_volatile(typ))
		fprintf(fd, "#if 0 /* volatile type: do not declare here, declared elsewhere */\n");
	else if(is_transient(typ) && typ->ref)
		fprintf(fd, "/* Transient type: */\n");
	else if(is_invisible(typ->id->name) && typ->ref)
		fprintf(fd, "/* Operation wrapper: */\n");
	else if(is_hexBinary(typ))
		fprintf(fd, "/* hexBinary schema type: */\n");
	else if(is_binary(typ))
		fprintf(fd, "/* Base64 schema type: */\n");
	else if(is_discriminant(typ))
		fprintf(fd, "/* Choice: */\n");
	else if(is_dynamic_array(typ)) {
		Eptr = ((Table *)typ->ref)->list;
		if(has_ns(typ) || is_untyped(typ))
			fprintf(fd, "/* Sequence of %s schema type: */\n", x);
		else {
			if(!eflag) {
				sprintf(errbuf, "array '%s' is not compliant with WS-I Basic Profile 1.0a, reason: SOAP encoded array", c_type(typ));
				compliancewarn(errbuf);
			}
			fprintf(fd, "/* SOAP encoded array of %s schema type: */\n", x); 
		}
	}
	else if(is_primclass(typ))
		fprintf(fd, "/* Primitive %s schema type: */\n", x);
	else if(sstreq(typ->id->name, "SOAP_ENV__Header"))
		fprintf(fd, "/* SOAP Header: */\n");
	else if(sstreq(typ->id->name, "SOAP_ENV__Fault"))
		fprintf(fd, "/* SOAP Fault: */\n");
	else if(sstreq(typ->id->name, "SOAP_ENV__Code"))
		fprintf(fd, "/* SOAP Fault Code: */\n");
	else if(x && *x && typ->ref)
		fprintf(fd, "/* %s */\n", x);
	fflush(fd);
	if(typ->type == Tstruct) {
		DBGLOG(fprintf(stderr, "\nstruct %s\n", typ->id->name));
		if(typ->ref) {
			int permission = -1;
			fprintf(fd, "struct %s {\n", ident(typ->id->name));
			for(Eptr = ((Table *)typ->ref)->list; Eptr; Eptr = Eptr->next) {
				if(!cflag && permission != (Eptr->info.sto&(Sprivate|Sprotected))) {
					if(Eptr->info.sto&Sprivate)
						fprintf(fd, "private:");
					else if(Eptr->info.sto&Sprotected)
						fprintf(fd, "protected:");
					else
						fprintf(fd, "public:");
					permission = (Eptr->info.sto&(Sprivate|Sprotected));
				}
				if(cflag && Eptr->info.typ->type == Tfun)
					continue;
				if(cflag && (Eptr->info.sto&Stypedef))
					continue;
				fprintf(fd, "\n\t%s", c_storage(Eptr->info.sto));
				/*if(Eptr->info.typ->type == Tclass && !is_external(Eptr->info.typ) && Eptr->info.typ->classed == False || (Eptr->info.typ->type == Tpointer || Eptr->info.typ->type == Treference) && Eptr->info.typ->ref && ((Tnode*)Eptr->info.typ->ref)->type == Tclass && !is_external(Eptr->info.typ->ref) && ((Tnode*)Eptr->info.typ->ref)->classed == False)
				   fprintf(fd, "class ");
				 */
				if(Eptr->sym == typ->id && Eptr->info.typ->type == Tfun) /* a hack to emit constructor in a struct, where constructor has no return value */
					((FNinfo *)Eptr->info.typ->ref)->ret = mknone();
				fprintf(fd, "%s", c_type_id(Eptr->info.typ, Eptr->sym->name));
				if(Eptr->info.sto&Sconst)
					fprintf(fd, "%s;", c_init(Eptr));
				else if(Eptr->info.sto&Sconstobj)
					fprintf(fd, " const;");
				else
					fprintf(fd, ";");
				if(Eptr->info.sto&Sreturn)
					fprintf(fd, "\t/* SOAP 1.2 RPC return element (when namespace qualified) */");
				if(is_external(Eptr->info.typ))
					fprintf(fd, "\t/* external */");
				if(is_transient(Eptr->info.typ))
					fprintf(fd, "\t/* transient */");
				if(is_imported(Eptr->info.typ))
					fprintf(fd, "\t/* type imported from %s */", Eptr->info.typ->imported);
				if(Eptr->info.sto&Sattribute) {
					if(Eptr->info.minOccurs >= 1)
						fprintf(fd, "\t/* required attribute of type %s */", wsdl_type(Eptr->info.typ, ""));
					else
						fprintf(fd, "\t/* optional attribute of type %s */", wsdl_type(Eptr->info.typ, ""));
				}
				if(Eptr->info.sto&(Sconst|Sprivate|Sprotected))
					fprintf(fd, "\t/* not serialized */");
				else if(Eptr->info.sto&SmustUnderstand)
					fprintf(fd, "\t/* mustUnderstand */");
				else if(!is_dynamic_array(typ) && is_repetition(Eptr)) {
					if(Eptr->info.maxOccurs > 1)
						fprintf(fd, "\t/* sequence of " SOAP_LONG_FORMAT " to " SOAP_LONG_FORMAT " elements <%s> */",
							Eptr->info.minOccurs, Eptr->info.maxOccurs,
							ns_convert(Eptr->next->sym->name));
					else
						fprintf(fd, "\t/* sequence of elements <%s> */", ns_convert(Eptr->next->sym->name));
				}
				else if(is_anytype(Eptr))
					fprintf(fd, "\t/* any type of element <%s> (defined below) */", ns_convert(Eptr->next->sym->name));
				else if(is_choice(Eptr))
					fprintf(fd, "\t/* union discriminant (of union defined below) */");
				else if(Eptr->info.typ->type != Tfun && !(Eptr->info.sto&(Sconst|Sprivate|Sprotected)) &&
					!(Eptr->info.sto&Sattribute) && !is_transient(Eptr->info.typ) && !is_external(Eptr->info.typ) &&
				        strncmp(Eptr->sym->name, "__", 2)) {
					if(Eptr->info.maxOccurs > 1)
						fprintf(fd, "\t/* sequence of " SOAP_LONG_FORMAT " to " SOAP_LONG_FORMAT " elements of type %s */",
							Eptr->info.minOccurs, Eptr->info.maxOccurs, wsdl_type(Eptr->info.typ, ""));
					else if(Eptr->info.minOccurs >= 1)
						fprintf(fd, "\t/* required element of type %s */", wsdl_type(Eptr->info.typ, ""));
					else
						fprintf(fd, "\t/* optional element of type %s */", wsdl_type(Eptr->info.typ, ""));
				}
				if(!is_dynamic_array(typ) && !is_primclass(typ)) {
					if(!strncmp(Eptr->sym->name, "__size", 6))                                                   {
						if(!Eptr->next || Eptr->next->info.typ->type != Tpointer) {
							sprintf(errbuf, "Member field '%s' is not followed by a pointer member field in struct '%s'", Eptr->sym->name, typ->id->name);
							semwarn(errbuf);
						}
					}
					else if(!strncmp(Eptr->sym->name, "__type", 6)) {
						if(!Eptr->next || ((Eptr->next->info.typ->type != Tpointer || ((Tnode *)Eptr->next->info.typ->ref)->type != Tvoid)))                                                  {
							sprintf(errbuf, "Member field '%s' is not followed by a void pointer or union member field in struct '%s'",
								Eptr->sym->name, typ->id->name);
							semwarn(errbuf);
						}
					}
				}
			}
			if(!((Table *)typ->ref)->list) {
				if(cflag)
					fprintf(fd, "\n#ifdef WITH_NOEMPTYSTRUCT\n\tchar dummy;\t/* dummy member to enable compilation */\n#endif");
				else
					fprintf(fd, "\n#ifdef WITH_NOEMPTYSTRUCT\nprivate:\n\tchar dummy;\t/* dummy member to enable compilation */\n#endif");
			}
			fprintf(fd, "\n};");
		}
		else if(!is_transient(typ) && !is_external(typ) && !is_volatile(typ)) {
			sprintf(errbuf, "struct '%s' is empty", typ->id->name);
			semwarn(errbuf);
		}
	}
	else if(typ->type == Tclass) {
		DBGLOG(fprintf(stderr, "\nclass %s\n", typ->id->name));
		if(typ->ref) {
			int permission = -1;
			fprintf(fd, "class SOAP_CMAC %s", ident(typ->id->name));
			if(typ->base)
				fprintf(fd, " : public %s", ident(typ->base->name));
			fprintf(fd, " {\n");
			for(Eptr = ((Table *)typ->ref)->list; Eptr; Eptr = Eptr->next) {
				if(permission != (Eptr->info.sto&(Sprivate|Sprotected))) {
					if(Eptr->info.sto&Sprivate)
						fprintf(fd, "private:");
					else if(Eptr->info.sto&Sprotected)
						fprintf(fd, "protected:");
					else
						fprintf(fd, "public:");
					permission = (Eptr->info.sto&(Sprivate|Sprotected));
				}
				fprintf(fd, "\n\t%s", c_storage(Eptr->info.sto));
				/* if(Eptr->info.typ->type == Tclass && !is_external(Eptr->info.typ) && Eptr->info.typ->classed == False || (Eptr->info.typ->type == Tpointer || Eptr->info.typ->type == Treference) && Eptr->info.typ->ref && ((Tnode*)Eptr->info.typ->ref)->type == Tclass && !is_external(Eptr->info.typ->ref) && ((Tnode*)Eptr->info.typ->ref)->classed == False)
				   fprintf(fd, "class ");
				 */
				fprintf(fd, "%s", c_type_id(Eptr->info.typ, Eptr->sym->name));
				if(Eptr->info.sto&Sconstobj)
					fprintf(fd, " const");
				if(Eptr->info.sto&Sconst)
					fprintf(fd, "%s;", c_init(Eptr));
				else if(Eptr->info.sto&Sabstract)
					fprintf(fd, " = 0;");
				else
					fprintf(fd, ";");
				if(Eptr->info.sto&Sreturn)
					fprintf(fd, "\t/* SOAP 1.2 RPC return element (when namespace qualified) */");
				if(is_external(Eptr->info.typ))
					fprintf(fd, "\t/* external */");
				if(is_transient(Eptr->info.typ))
					fprintf(fd, "\t/* transient */");
				if(is_imported(Eptr->info.typ))
					fprintf(fd, "\t/* type imported from %s */", Eptr->info.typ->imported);
				if(Eptr->info.sto&Sattribute) {
					if(Eptr->info.minOccurs >= 1)
						fprintf(fd, "\t/* required attribute */");
					else
						fprintf(fd, "\t/* optional attribute */");
				}
				if(Eptr->info.sto&(Sconst|Sprivate|Sprotected))
					fprintf(fd, "\t/* not serialized */");
				else if(Eptr->info.sto&SmustUnderstand)
					fprintf(fd, "\t/* mustUnderstand */");
				else if(!is_dynamic_array(typ) && is_repetition(Eptr)) {
					if(Eptr->info.maxOccurs > 1)
						fprintf(fd, "\t/* sequence of " SOAP_LONG_FORMAT " to " SOAP_LONG_FORMAT " elements <%s> */",
							Eptr->info.minOccurs, Eptr->info.maxOccurs, ns_convert(Eptr->next->sym->name));
					else
						fprintf(fd, "\t/* sequence of elements <%s> */", ns_convert(Eptr->next->sym->name));
				}
				else if(is_anytype(Eptr))
					fprintf(fd, "\t/* any type of element <%s> (defined below) */", ns_convert(Eptr->next->sym->name));
				else if(is_choice(Eptr))
					fprintf(fd, "\t/* union discriminant (of union defined below) */");
				else if(Eptr->info.typ->type != Tfun && !(Eptr->info.sto&(Sconst|Sprivate|Sprotected)) &&
				        !(Eptr->info.sto&Sattribute) && !is_transient(Eptr->info.typ) && !is_external(Eptr->info.typ) &&
				        strncmp(Eptr->sym->name, "__", 2)) {
					if(Eptr->info.maxOccurs > 1)
						fprintf(fd, "\t/* sequence of " SOAP_LONG_FORMAT " to " SOAP_LONG_FORMAT " elements */", Eptr->info.minOccurs, Eptr->info.maxOccurs);
					else if(Eptr->info.minOccurs >= 1)
						fprintf(fd, "\t/* required element of type %s */", wsdl_type(Eptr->info.typ, ""));
					else
						fprintf(fd, "\t/* optional element of type %s */", wsdl_type(Eptr->info.typ, ""));
				}
				if(!is_dynamic_array(typ) && !is_primclass(typ)) {
					if(!strncmp(Eptr->sym->name, "__size", 6)) {
						if(!Eptr->next || Eptr->next->info.typ->type != Tpointer) {
							sprintf(errbuf, "Member field '%s' is not followed by a pointer member field in struct '%s'",
								Eptr->sym->name, typ->id->name);
							semwarn(errbuf);
						}
					}
					else if(!strncmp(Eptr->sym->name, "__type", 6)) {
						if(!Eptr->next || ((Eptr->next->info.typ->type != Tpointer || ((Tnode *)Eptr->next->info.typ->ref)->type != Tvoid))) {
							sprintf(errbuf, "Member field '%s' is not followed by a void pointer or union member field in struct '%s'",
								Eptr->sym->name, typ->id->name);
							semwarn(errbuf);
						}
					}
				}
			}
			if(!is_transient(typ) && !is_volatile(typ)) {
				fprintf(fd, "\npublic:\n\tvirtual int soap_type() const { return %d; } /* = unique id %s */", typ->num, soap_type(typ));
				fprintf(fd, "\n\tvirtual void soap_default(struct soap*);");
				fprintf(fd, "\n\tvirtual void soap_serialize(struct soap*) const;");
				if(kflag)
					fprintf(fd, "\n\tvirtual void soap_traverse(struct soap*, const char *s, soap_walker, soap_walker);");
				fprintf(fd, "\n\tvirtual int soap_put(struct soap*, const char*, const char*) const;");
				fprintf(fd, "\n\tvirtual int soap_out(struct soap*, const char*, int, const char*) const;");
				fprintf(fd, "\n\tvirtual void *soap_get(struct soap*, const char*, const char*);");
				fprintf(fd, "\n\tvirtual void *soap_in(struct soap*, const char*, const char*);");
				if(!has_constructor(typ)) { /* Table *t; Entry *p; int c = ':'; */
					fprintf(fd, "\n\t         %s()", ident(typ->id->name));
					/* Obsolete: moved to class::soap_default() */
#if 0
					t = (Table *)typ->ref;
					if(t) {
						for(p = t->list; p; p = p->next)        {
							if(!(p->info.sto&
							     Sconst))                                          {
								if(p->info.typ->type == Tpointer) {
									fprintf(fd, "%c %s(NULL)", c, ident(p->sym->name));
									c = ',';
								}
 #if 0
								else if(is_choice(p)) {
									fprintf(fd, "%c %s(0)", c, ident(p->sym->name));
									c = ',';
								}
								else if(p->info.typ->type == Tenum) {
									fprintf(fd, "%c %s((%s)0)", c, ident(p->sym->name), c_type(p->info.typ));
									c = ',';
								}
								else if(p->info.typ->type >= Tchar && p->info.typ->type < Tenum) {
									fprintf(fd, "%c %s(0)", c, ident(p->sym->name));
									c = ',';
								}
 #endif
							}
						}
					}
#endif
					fprintf(fd, " { %s::soap_default(NULL); }", ident(typ->id->name));
				}
				if(!has_destructor(typ))
					fprintf(fd, "\n\tvirtual ~%s() { }", ident(typ->id->name));
				/* the use of 'friend' causes problems linking static functions. Adding these friends could enable serializing protected/private members (which is not implemented)
				   fprintf(fd,"\n\tfriend %s *soap_instantiate_%s(struct soap*, int, const char*, const char*, size_t*);", typ->id->name, typ->id->name);
				   fprintf(fd,"\n\tfriend %s *soap_in_%s(struct soap*, const char*, %s*, const char*);", typ->id->name, typ->id->name, typ->id->name);
				   fprintf(fd,"\n\tfriend int soap_out_%s(struct soap*, const char*, int, const %s*, const char*);", typ->id->name, typ->id->name);
				 */
			}
			else if(!((Table *)typ->ref)->list)
				fprintf(
					fd,
					"\n#ifdef WITH_NOEMPTYSTRUCT\nprivate:\n\tchar dummy;\t/* dummy member to enable compilation */\n#endif");
			fprintf(fd, "\n};");
		}
		else if(!is_transient(typ) && !is_external(typ) && !is_volatile(typ)) {
			sprintf(errbuf, "class '%s' is empty", typ->id->name);
			semwarn(errbuf);
		}
	}
	else if(typ->type == Tunion) {
		int i = 1;
		if(typ->ref) {
			fprintf(fd, "union %s\n{", ident(typ->id->name));
			for(Eptr = ((Table *)typ->ref)->list; Eptr; Eptr = Eptr->next) {
				fprintf(fd, "\n#define SOAP_UNION_%s_%s\t(%d)", c_ident(typ), ident(Eptr->sym->name), i);
				i++;
				fprintf(fd, "\n\t%s", c_storage(Eptr->info.sto));
				fprintf(fd, "%s;", c_type_id(Eptr->info.typ, Eptr->sym->name));
				if(Eptr->info.sto&(Sconst|Sprivate|Sprotected))
					fprintf(fd, "\t/* const field cannot be deserialized */");
				if(is_external(Eptr->info.typ))
					fprintf(fd, "\t/* external */");
				if(is_transient(Eptr->info.typ))
					fprintf(fd, "\t/* transient */");
				if(Eptr->info.sto&Sattribute) {
					fprintf(fd, "\t/* attribute not allowed in union */");
					sprintf(errbuf, "union '%s' contains attribute declarations", typ->id->name);
					semwarn(errbuf);
				}
				if(Eptr->info.sto&SmustUnderstand)
					fprintf(fd, "\t/* mustUnderstand */");
			}
			fprintf(fd, "\n};");
		}
		else if(!is_transient(typ) && !is_external(typ) && !is_volatile(typ)) {
			sprintf(errbuf, "union '%s' is empty", typ->id->name);
			semwarn(errbuf);
		}
	}
	if(is_volatile(typ))
		fprintf(fd, "\n#endif");
	if((typ->type == Tstruct || typ->type == Tunion) && p->sym->token == TYPE)
		fprintf(fd, "\ntypedef %s %s;", c_type(typ), ident(p->sym->name));
	if(typ->ref)
		fprintf(fd, "\n#endif");
	if(is_header_or_fault(typ) || is_body(typ))
		fprintf(fd, "\n\n#endif");
	fflush(fd);
}

void generate_header(Table * t)
{
	Entry * p, * q;
	banner(fheader, "Enumerations");
	fflush(fheader);
	if(enumtable) {
		for(p = enumtable->list; p; p = p->next) {
			char * x;
			if(is_imported(p->info.typ) || (is_transient(p->info.typ) && !p->info.typ->ref))
				continue;
			x = xsi_type(p->info.typ);
			if(!x || !*x)
				x = wsdl_type(p->info.typ, "");
			fprintf(fheader, "\n#ifndef %s", soap_type(p->info.typ));
			fprintf(fheader, "\n\t#define %s (%d)", soap_type(p->info.typ), p->info.typ->num);
			if(is_volatile(p->info.typ))
				fprintf(fheader, "\n\t#if 0 /* volatile type: do not redeclare here */");
			if(is_mask(p->info.typ))
				fprintf(fheader, "\n/* Bitmask %s */", x);
			else
				fprintf(fheader, "\n/* %s */", x);
			fprintf(fheader, "\nenum %s {", ident(p->info.typ->id->name));
			if((Table *)p->info.typ->ref) {
				q = ((Table *)p->info.typ->ref)->list;
				if(q) {
					fprintf(fheader, "%s = " SOAP_LONG_FORMAT, ident(q->sym->name), q->info.val.i);
					for(q = q->next; q; q = q->next)
						fprintf(fheader, ", %s = " SOAP_LONG_FORMAT, ident(q->sym->name), q->info.val.i);
				}
			}
			fprintf(fheader, "};");
			if(p->sym->token == TYPE)
				fprintf(fheader, "\ntypedef %s %s;", c_type(p->info.typ), ident(p->sym->name));
			if(is_volatile(p->info.typ))
				fprintf(fheader, "\n\t#endif");
			fprintf(fheader, "\n#endif");
		}
	}
	banner(fheader, "Types with Custom Serializers");
	fflush(fheader);
	if(typetable)
		for(p = typetable->list; p; p = p->next) {
			if(is_external(p->info.typ) && !is_volatile(p->info.typ) && !is_imported(p->info.typ)) {
				fprintf(fheader, "\n#ifndef %s", soap_type(p->info.typ));
				fprintf(fheader, "\n\t#define %s (%d)", soap_type(p->info.typ), p->info.typ->num);
				fprintf(fheader, "\n\t%s%s;", c_storage(p->info.sto), c_type_id(p->info.typ, p->sym->name));
				fprintf(fheader, "\n#endif");
			}
		}
	if(typetable)
		for(p = typetable->list; p; p = p->next) {
			if(p->info.typ->type == Tclass && is_eq(p->info.typ->sym->name, "xsd__QName") && !is_external(p->info.typ) && !is_imported(p->info.typ))                                           {
				fprintf(fheader, "\n#ifndef %s", soap_type(p->info.typ));
				fprintf(fheader, "\n\t#define %s (%d)", soap_type(p->info.typ), p->info.typ->num);
				fprintf(fheader, "\n\t%sstd::string %s;", c_storage(p->info.sto), p->sym->name);
				fprintf(fheader, "\n#endif\n");
			}
		}
	banner(fheader, "Classes and Structs");
	fflush(fheader);
	if(classtable) {
		for(p = classtable->list; p; p = p->next) {
			if(!is_imported(p->info.typ))
				gen_class(fheader, p);
		}
	}
	banner(fheader, "Typedefs");
	fflush(fheader);
	if(typetable)
		for(p = typetable->list; p; p = p->next) {
			if(!is_primitive_or_string(p->info.typ) && !is_external(p->info.typ) && !is_XML(p->info.typ) &&
			   !is_transient(p->info.typ) && !has_ns_t(p->info.typ) && !is_imported(p->info.typ) &&
			   !is_template(p->info.typ))                                           {
				sprintf(errbuf, "typedef '%s' is not namespace qualified: schema definition for '%s' in WSDL file output may be invalid",
					p->sym->name, p->sym->name);
				semwarn(errbuf);
			}
			if(p->info.typ->type == Tclass && is_eq(p->info.typ->sym->name, "xsd__QName") && !is_external(p->info.typ) && !is_imported(p->info.typ))
				continue;
			if(!is_external(p->info.typ) && !is_imported(p->info.typ)) {
				fprintf(fheader, "\n#ifndef %s", soap_type(p->info.typ));
				fprintf(fheader, "\n#define %s (%d)", soap_type(p->info.typ), p->info.typ->num);
				fprintf(fheader, "\n%s%s;", c_storage(p->info.sto), c_type_id(p->info.typ, p->sym->name));
				fprintf(fheader, "\n#endif\n");
			}
		}
	banner(fheader, "Externals");
	fflush(fheader);
	if(t)
		for(p = t->list; p; p = p->next)
			if(p->info.typ->type != Tfun || p->info.sto&Sextern) {
				fprintf(fheader, "\n\nextern %s", c_storage(p->info.sto));
				fprintf(fheader, "%s;", c_type_id(p->info.typ, p->sym->name));
			}
	fflush(fheader);
}

void get_namespace_prefixes(void)
{
	Symbol * p, * q;
	int i, n;
	char * s, buf[256];
	if(nslist)
		return;
	for(p = symlist; p; p = p->next) {
		if(*p->name != '~') {
			s = p->name;
			while(*s == '_')
				s++;
			n = (int)(strlen(s)-2);
			for(i = 1; i < n; i++) {
				if(s[i] == ':' || (s[i-1] != '_' && s[i] == '_' && s[i+1] == '_' && s[i+2] && s[i+2] != '_') ||
				   (s[i-1] != '_' && (!strncmp(s+i, "___DOT", 6) || !strncmp(s+i, "___USCORE", 9) ||
					(!strncmp(s+i, "___x", 4) && ishex(s[i+4]) && ishex(s[i+5]) && ishex(s[i+6]) && ishex(s[i+7]))))) {
					if(s[i+1] == ':') {
						i++;
						continue;
					}
					strncpy(buf, s, i);
					buf[i] = '\0';
					if(sstreq(buf, "SOAP_ENV") || sstreq(buf, "SOAP_ENC") || sstreq(buf, "xsd") || sstreq(buf, "xsi") || sstreq(buf, "xml") || sstreq(buf, "std") || !strncmp(buf, "soap_", 5))
						goto nsnext;
					for(q = nslist; q; q = q->next)
						if(sstreq(q->name, buf))
							goto nsnext;
					q = (Symbol *)emalloc(sizeof(Symbol));
					q->name = static_cast<char *>(emalloc(i+1));
					strcpy(q->name, buf);
					q->name[i] = '\0';
					q->next = nslist;
					nslist = q;
					break;
				}
			}
		}
nsnext:
		;
	}
	q = (Symbol *)emalloc(sizeof(Symbol));
	q->name = "xsd";
	q->next = nslist;
	nslist = q;
	q = (Symbol *)emalloc(sizeof(Symbol));
	q->name = "xsi";
	q->next = nslist;
	nslist = q;
	q = (Symbol *)emalloc(sizeof(Symbol));
	q->name = (char *)S_SoapEncNs; // @badcast
	q->next = nslist;
	nslist = q;
	q = (Symbol *)emalloc(sizeof(Symbol));
	q->name = (char *)S_SoapEnvNs; // @badcast
	q->next = nslist;
	nslist = q;
}

void generate_schema(Table * t)
{
	Entry * p;
	Symbol * ns;
	char * name = NULL;
	char * URL = NULL;
	char * executable = NULL;
	char * URI = NULL;
	char * style = NULL;
	char * encoding = NULL;
	char * import = NULL;
	Service * sp = NULL;
	char buf[1024];
	FILE * fd;
	int flag = 0;
	get_namespace_prefixes();
	for(ns = nslist; ns; ns = ns->next) {
		if(sstreq(ns->name, S_SoapEnvNs) || sstreq(ns->name, S_SoapEncNs) || sstreq(ns->name, "xsi") || sstreq(ns->name, "xsd"))
			continue;
		name = NULL;
		URL = NULL;
		executable = NULL;
		URI = NULL;
		style = NULL;
		encoding = NULL;
		import = NULL;
		for(sp = services; sp; sp = sp->next) {
			if(!tagcmp(sp->ns, ns->name))                                        {
				name = ns_cname(sp->name, NULL);
				URL = sp->URL;
				executable = sp->executable;
				URI = sp->URI;
				style = sp->style;
				encoding = sp->encoding;
				import = sp->import;
				break;
			}
		}
		if(!URI) {
			URI = emalloc(strlen(tmpURI)+strlen(ns->name)+6);
			sprintf(URI, "%s/%s.xsd", tmpURI, ns_convert(ns->name));
		}
		if(is_document(style) && encoding && !*encoding) {
			semwarn("Cannot use document style with SOAP encoding");
			encoding = NULL;
		}
		if(!name)
			name = "Service";
		if(!URL)
			URL = "http://localhost:80";
		if(!import)
			flag = 1;
		if(t) {
			for(p = t->list; p; p = p->next)        {
				if(p->info.typ->type == Tfun && !(p->info.sto&Sextern) && has_ns_eq(ns->name, p->sym->name)) {
					if(name)
						fprintf(fmsg, "Using %s service name: %s\n", ns->name, name);
					if(style)
						fprintf(fmsg, "Using %s service style: %s\n", ns->name, style);
					else if(!eflag)
						fprintf(fmsg, "Using %s service style: document\n", ns->name);
					if(encoding && *encoding)
						fprintf(fmsg, "Using %s service encoding: %s\n", ns->name, encoding);
					else if(encoding && !*encoding)
						fprintf(fmsg, "Using %s service encoding: encoded\n", ns->name);
					else if(!eflag)
						fprintf(fmsg, "Using %s service encoding: literal\n", ns->name);
					if(URL)
						fprintf(fmsg, "Using %s service location: %s\n", ns->name, URL);
					if(executable)
						fprintf(fmsg, "Using %s service executable: %s\n", ns->name, executable);
					if(import)
						fprintf(fmsg, "Using %s schema import: %s\n", ns->name, import);
					else if(URI)
						fprintf(fmsg, "Using %s schema namespace: %s\n", ns->name, URI);
					if(sp && sp->name)
						sprintf(buf, "%s%s.wsdl", dirpath, ns_cname(name, NULL));
					else
						sprintf(buf, "%s%s.wsdl", dirpath, ns_cname(ns->name, NULL));
					if(!wflag && !import) {
						fprintf(fmsg, "Saving %s Web Service description\n", buf);
						fd = fopen(buf, "w");
						if(!fd)
							execerror("Cannot write WSDL file");
						gen_wsdl(fd, t, ns->name, name, URL, executable, URI, style, encoding);
						fclose(fd);
					}
					if(!cflag) {
						if(iflag || jflag) {
							char * sname = (sp && sp->name) ? sp->name : "";
							if(!Sflag) {
								char * name1 = ns_cname(sname, "Proxy");
								sprintf(buf, "%s%s%s.h", dirpath, prefix, name1);
								fprintf(fmsg, "Saving %s client proxy class\n", buf);
								fd = fopen(buf, "w");
								if(!fd)
									execerror("Cannot write proxy class file");
								sprintf(buf, "%s%s.h", prefix, name1);
								copyrightnote(fd, buf);
								gen_proxy_header(fd, t, ns, name1, URL, executable, URI, encoding);
								fclose(fd);
								sprintf(buf, "%s%s%s.cpp", dirpath, prefix, name1);
								fprintf(fmsg, "Saving %s client proxy class\n", buf);
								fd = fopen(buf, "w");
								if(!fd)
									execerror("Cannot write proxy class file");
								sprintf(buf, "%s%s.cpp", prefix, name1);
								copyrightnote(fd, buf);
								gen_proxy_code(fd, t, ns, name1, URL, executable, URI, encoding);
								fclose(fd);
							}
							if(!Cflag) {
								char * name1 = ns_cname(sname, "Service");
								sprintf(buf, "%s%s%s.h", dirpath, prefix, name1);
								fprintf(fmsg, "Saving %s service class\n", buf);
								fd = fopen(buf, "w");
								if(!fd)
									execerror("Cannot write service class file");
								sprintf(buf, "%s%s.h", prefix, name1);
								copyrightnote(fd, buf);
								gen_object_header(fd, t, ns, name1, URL, executable, URI, encoding);
								fclose(fd);
								sprintf(buf, "%s%s%s.cpp", dirpath, prefix, name1);
								fprintf(fmsg, "Saving %s service class\n", buf);
								fd = fopen(buf, "w");
								if(!fd)
									execerror("Cannot write service class file");
								sprintf(buf, "%s%s.cpp", prefix, name1);
								copyrightnote(fd, buf);
								gen_object_code(fd, t, ns, name1, URL, executable, URI, encoding);
								fclose(fd);
							}
						}
						else {
							if(!Sflag && sp && sp->name) {
							      sprintf(buf, "%s%s%s.h", dirpath, prefix, ns_cname(name, "Proxy"));
							      fprintf(fmsg, "Saving %s client proxy\n", buf);
							      fd = fopen(buf, "w");
							      if(!fd)
								      execerror("Cannot write proxy file");
							      sprintf(buf, "%s%s.h", prefix, ns_cname(name, "Proxy"));
							      copyrightnote(fd, buf);
							      gen_proxy(fd, t, ns, name, URL, executable, URI, encoding);
							      fclose(fd);
						      }
						      else if(!Sflag) {
							      sprintf(buf, "%s%s.h", dirpath, ns_cname(prefix, "Proxy"));
							      fprintf(fmsg, "Saving %s client proxy\n", buf);
							      fd = fopen(buf, "w");
							      if(!fd)
								      execerror("Cannot write proxy file");
							      sprintf(buf, "%s.h", ns_cname(prefix, "Proxy"));
							      copyrightnote(fd, buf);
							      gen_proxy(fd, t, ns, "Service", URL, executable, URI, encoding);
							      fclose(fd);
						      }
						      if(!Cflag && sp && sp->name) {
							      sprintf(buf, "%s%s%s.h", dirpath, prefix, ns_cname(name, "Object"));
							      fprintf(fmsg, "Saving %s server object\n", buf);
							      fd = fopen(buf, "w");
							      if(!fd)
								      execerror("Cannot write server object file");
							      sprintf(buf, "%s%s.h", prefix, ns_cname(name, "Object"));
							      copyrightnote(fd, buf);
							      gen_object(fd, t, ns, name, URL, executable, URI, encoding);
							      fclose(fd);
						      }
						      else if(!Cflag) {
							      sprintf(buf, "%s%s.h", dirpath, ns_cname(prefix, "Object"));
							      fprintf(fmsg, "Saving %s server object\n", buf);
							      fd = fopen(buf, "w");
							      if(!fd)
								      execerror("Cannot write server object file");
							      sprintf(buf, "%s.h", ns_cname(prefix, "Object"));
							      copyrightnote(fd, buf);
							      gen_object(fd, t, ns, "Service", URL, executable, URI, encoding);
							      fclose(fd);
						      }
						}
					}
					if(!xflag) {
						strcpy(buf, dirpath);
						if(sp && sp->name)
							strcat(buf, ns_fname(name));
						else
							strcat(buf, ns_fname(ns->name));
						strcat(buf, ".");
						gen_data(buf, t, ns->name, name, URL, executable, URI, encoding);
					}
					break;
				}
			}
			if(sp && sp->name) {
				has_nsmap = 1;
				if(nflag)
					sprintf(buf, "%s%s.nsmap", dirpath, prefix);
				else
					sprintf(buf, "%s%s.nsmap", dirpath, ns_cname(name, NULL));
				fprintf(fmsg, "Saving %s namespace mapping table\n", buf);
				fd = fopen(buf, "w");
				if(!fd)
					execerror("Cannot write nsmap file");
				fprintf(fd, "\n#include \"%sH.h\"", prefix);
				if(nflag)
					fprintf(fd, "\nSOAP_NMAC struct Namespace %s_namespaces[] =\n", prefix);
				else
					fprintf(fd, "\nSOAP_NMAC struct Namespace namespaces[] =\n");
				gen_nsmap(fd, ns, URI);
				fclose(fd);
				if(Tflag && !Cflag) {
					Entry * method;
					char soapTester[1024];
					char pathsoapTester[1024];
					char * name1 = NULL;
					Tflag = 0;
					strcpy(soapTester, prefix);
					strcat(soapTester, "Tester");
					strcat(soapTester, cflag ? ".c" : ".cpp");
					strcpy(pathsoapTester, dirpath);
					strcat(pathsoapTester, soapTester);
					fprintf(fmsg, "Saving %s server auto-test code\n", pathsoapTester);
					fd = fopen(pathsoapTester, "w");
					if(!fd)
						execerror("Cannot write to file");
					copyrightnote(fd, soapTester);
					fprintf(fd, "\n/*\n   Stand-alone server auto-test code:\n   Takes request from standard input or over TCP/IP socket and returns\nresponse to standard output or socket\n\n   Compile:\n   cc soapTester.c soapServer.c soapC.c stdsoap2.c\n\n   Command line usage with redirect over stdin/out:\n   > ./a.out < SomeTest.req.xml\n   > ./a.out 12288 < SomeTest.req.xml\n     Note: 12288 = SOAP_XML_INDENT | SOAP_XML_STRICT (see codes in stdsoap2.h)\n   Command line usage to start server at port 8080:\n   > a.out 12288 8080\n*/\n\n#include \"");
					if(iflag || jflag) {
						char * sname = (sp && sp->name) ? sp->name : "";
						name1 = ns_cname(sname, "Service");
						fprintf(fd, "%s%s%s.h\"\n\n#ifndef SOAP_DEFMAIN\n# define SOAP_DEFMAIN main\t/* redefine to use your own main() */\n#endif\n\nint SOAP_DEFMAIN(int argc, char **argv)\n{\n\t%s service(argc > 1 ? atoi(argv[1]) : 0);\n\tif(argc <= 2)\n\t\treturn service.serve();\n\treturn service.run(atoi(argv[2]));\n}\n",
							dirpath, prefix, name1, name1);
					}
					else
						fprintf(
							fd,
							"%s%s.nsmap\"\n\n#ifndef SOAP_DEFMAIN\n# define SOAP_DEFMAIN main\t/* redefine to use your own main() */\n#endif\n\nint SOAP_DEFMAIN(int argc, char **argv)\n{\n\tstruct soap *soap = soap_new1(argc > 1 ? atoi(argv[1]) : 0);\n\tif(argc <= 2)\n\t\treturn %s_serve(soap);\n\tif(soap_valid_socket(soap_bind(soap, NULL, atoi(argv[2]), 100)))\n\t\twhile(soap_valid_socket(soap_accept(soap)))\n\t\t{\t%s_serve(soap);\n\t\t\tsoap_destroy(soap);\n\t\t\tsoap_end(soap);\n\t\t}\n\tsoap_free(soap);\n\treturn 0;\n}\n",
							dirpath, nflag ? prefix : ns_cname(name,
								NULL), nflag ? prefix : "soap", nflag ? prefix : "soap");
					for(method = t->list; method; method = method->next) {
						if(method->info.typ->type == Tfun && !(method->info.sto&Sextern)) {
							Entry * p, * q = entry(t, method->sym);
							Table * r;
							if(q)
								p = (Entry *)q->info.typ->ref;
							else {
								fprintf(stderr, "Internal error: no table entry\n");
							      return; 
							}
							q = entry(classtable, method->sym);
							r = (Table *)q->info.typ->ref;
							if(iflag || jflag)
								fprintf(fd, "\n\n/** Auto-test server operation %s */\nint %s::%s(", method->sym->name, name1, ns_cname(method->sym->name, NULL));
							else
								fprintf(fd, "\n\n/** Auto-test server operation %s */\nint %s(struct soap *soap", method->sym->name, ident(method->sym->name));
							gen_params(fd, r, p, !iflag && !jflag);
							/* single param to single param echo */
							if(p && r && r->list && r->list->info.typ == p->info.typ)
								fprintf(fd, "\n{\n\t/* Echo request-response parameter */\n\t*%s = *%s;\n\treturn SOAP_OK;\n}\n",
									ident(p->sym->name), ident(r->list->sym->name));
							else if(p && r && r->list && p->info.typ->type == Tpointer && r->list->info.typ == (Tnode *)p->info.typ->ref)
								fprintf(fd, "\n{\n\t/* Echo request-response parameter */\n\t*%s = %s;\n\treturn SOAP_OK;\n}\n",
									ident(p->sym->name), ident(r->list->sym->name));
							else if(p && r && r->list && p->info.typ->type == Treference && r->list->info.typ == (Tnode *)p->info.typ->ref)
								fprintf(fd, "\n{\n\t/* Echo request-response parameter */\n\t%s = %s;\n\treturn SOAP_OK;\n}\n",
									ident(p->sym->name), ident(r->list->sym->name));
							else if(p && r && r->list && p->info.typ->type == Treference && r->list->info.typ->type == Tpointer &&
							        r->list->info.typ->ref == (Tnode *)p->info.typ->ref)
								fprintf(fd, "\n{\n\t/* Echo request-response parameter */\n\t%s = *%s;\n\treturn SOAP_OK;\n}\n", ident(p->sym->name), ident(r->list->sym->name));
							/* params to wrapped params echo */
							else {
								fprintf(fd, "\n{\n\t");
							      if(r && p && p->info.typ->ref && ((Tnode *)p->info.typ->ref)->ref &&
								 (((Tnode *)p->info.typ->ref)->type == Tstruct || ((Tnode *)p->info.typ->ref)->type == Tclass)) {
								      const char * s, * a;
								      int d = 1;
								      s = ident(p->sym->name);
								      if(p->info.typ->type == Treference)
									      a = ".";
								      else
									      a = "->";
								      for(p = ((Table *)((Tnode *)p->info.typ->ref)->ref)->list, q = r->list; p && q; p = p->next, q = q->next) {
									      if(p->info.typ == q->info.typ)
										      fprintf(fd, "\n\t%s%s%s = %s;", s, a, ident(p->sym->name), ident(q->sym->name));
									      else if(q->info.typ->type == Tpointer && p->info.typ == (Tnode *)q->info.typ->ref)
										      fprintf(fd, "\n\t%s%s%s = *%s;", s, a, ident(p->sym->name), ident(q->sym->name));
									      else
										      d = 0;
								      }
								      if(!d)
									      fprintf(fd, "\n\t/* Return incomplete response with default data values */");
							      }
							      fprintf(fd, "\n\treturn SOAP_OK;\n}\n"); }
							fflush(fd);
						}
					}
					fclose(fd);
				}
			}
		}
		if(!wflag && !import) {
			sprintf(buf, "%s%s.xsd", dirpath, ns_cname(ns->name, NULL));
			fprintf(fmsg, "Saving %s XML schema\n", buf);
			fd = fopen(buf, "w");
			if(!fd)
				execerror("Cannot write schema file");
			fprintf(fd, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
			if(t)
				for(p = t->list; p; p = p->next)
					if(p->info.typ->type == Tfun && !(p->info.sto&Sextern) && has_ns_eq(ns->name, p->sym->name)) {
						gen_schema(fd, t, ns->name, ns->name, 0, 1, URL, URI, style, encoding);
						break;
					}
			if(!t || !p)
				gen_schema(fd, t, ns->name, ns->name, 0, 0, URL, URI, style, encoding);
			fclose(fd);
		}
	}
	if(!has_nsmap && flag) {
		if(Tflag && !Cflag && !iflag && !jflag)
			fprintf(fmsg, "Warning: cannot save soapTester, need directive //gsoap service name\n");
		for(ns = nslist; ns; ns = ns->next)
			if(strcmp(ns->name, S_SoapEnvNs) && strcmp(ns->name, S_SoapEncNs) && strcmp(ns->name, "xsi") && strcmp(ns->name, "xsd"))
				break;
		if(nflag)
			sprintf(buf, "%s%s.nsmap", dirpath, prefix);
		else if(ns && ns->name)
			sprintf(buf, "%s%s.nsmap", dirpath, ns_cname(ns->name, NULL));
		else
			sprintf(buf, "%ssoap.nsmap", dirpath);
		fprintf(fmsg, "Saving %s namespace mapping table\n", buf);
		fd = fopen(buf, "w");
		if(!fd)
			execerror("Cannot write nsmap file");
		fprintf(fd, "\n#include \"%sH.h\"", prefix);
		if(nflag)
			fprintf(fd, "\nSOAP_NMAC struct Namespace %s_namespaces[] =\n", prefix);
		else
			fprintf(fd, "\nSOAP_NMAC struct Namespace namespaces[] =\n");
		gen_nsmap(fd, ns, URI);
		fclose(fd);
	}
}

int chkhdr(char * part)
{
	Entry * p = entry(classtable, lookup("SOAP_ENV__Header"));
	if(p)
		for(p = ((Table *)p->info.typ->ref)->list; p; p = p->next)
			if(has_ns_eq(NULL, p->sym->name) && (sstreq(part, p->sym->name) || is_eq_nons(part, p->sym->name)))
				return 1;
	sprintf(errbuf, "Cannot define method-header-part in WSDL: SOAP_ENV__Header \"%s\" member field is not qualified", part);
	semwarn(errbuf);
	return 0;
}

void gen_wsdl(FILE * fd, Table * t, char * ns, char * name, char * URL, char * executable, char * URI, char * style,
	char * encoding)
{
	Entry * p, * q, * r;
	Symbol * s;
	Service * sp, * sp2;
	Method * m;
	int mimein, mimeout;
	char * action, * comment, * method_style = NULL, * method_encoding = NULL, * method_response_encoding = NULL;
	char * portname;
	char * binding;
	fprintf(fd, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	for(sp = services; sp; sp = sp->next)
		if(!tagcmp(sp->ns, ns))
			break;
	if(sp && sp->definitions)
		fprintf(fd, "<definitions name=\"%s\"\n", sp->definitions);
	else
		fprintf(fd, "<definitions name=\"%s\"\n", name);
	if(sp && sp->WSDL)
		fprintf(fd, " targetNamespace=\"%s\"\n xmlns:tns=\"%s\"", sp->WSDL, sp->WSDL);
	else
		fprintf(fd, " targetNamespace=\"%s/%s.wsdl\"\n xmlns:tns=\"%s/%s.wsdl\"", URI, name, URI, name);
	if(sp && sp->binding)
		binding = ns_cname(sp->binding, NULL);
	else
		binding = name;
	if(sp && sp->portname)
		portname = ns_cname(sp->portname, NULL);
	else
		portname = name;
	for(s = nslist; s; s = s->next) {
		for(sp2 = services; sp2; sp2 = sp2->next)
			if(!tagcmp(sp2->ns, s->name) && sp2->URI)
				break;
		if(sp2)
			fprintf(fd, "\n xmlns:%s=\"%s\"", ns_convert(s->name), sp2->URI);
		else if(sstreq(s->name, S_SoapEnvNs))
			fprintf(fd, "\n xmlns:%s=\"%s\"", S_SoapEnvNs, envURI);
		else if(sstreq(s->name, S_SoapEncNs))
			fprintf(fd, "\n xmlns:%s=\"%s\"", S_SoapEncNs, encURI);
		else if(sstreq(s->name, "xsi"))
			fprintf(fd, "\n xmlns:xsi=\"%s\"", xsiURI);
		else if(sstreq(s->name, "xsd"))
			fprintf(fd, "\n xmlns:xsd=\"%s\"", xsdURI);
		else
			fprintf(fd, "\n xmlns:%s=\"%s/%s.xsd\"", ns_convert(s->name), tmpURI, ns_convert(s->name));
	}
	if(is_soap12(encoding))
		fprintf(fd, "\n xmlns:SOAP=\"http://schemas.xmlsoap.org/wsdl/soap12/\"");
	else
		fprintf(fd, "\n xmlns:SOAP=\"http://schemas.xmlsoap.org/wsdl/soap/\"");
	fprintf(fd, "\n xmlns:MIME=\"http://schemas.xmlsoap.org/wsdl/mime/\"");
	fprintf(fd, "\n xmlns:DIME=\"http://schemas.xmlsoap.org/ws/2002/04/dime/wsdl/\"");
	fprintf(fd, "\n xmlns:WSDL=\"http://schemas.xmlsoap.org/wsdl/\"");
	fprintf(fd, "\n xmlns=\"http://schemas.xmlsoap.org/wsdl/\">\n\n");
	fprintf(fd, "<types>\n\n");
	for(s = nslist; s; s = s->next)
		gen_schema(fd, t, ns, s->name, sstreq(s->name, ns), 1, URL, URI, style, encoding);
	fprintf(fd, "</types>\n\n");
	fflush(fd);
	if(t) {
		for(p = t->list; p; p = p->next) {
			if(p->info.typ->type == Tfun && !(p->info.sto&Sextern) && has_ns_eq(ns, p->sym->name)) {
				mimein = 0;
				mimeout = 0;
				comment = NULL;
				method_style = style;
				method_encoding = encoding;
				method_response_encoding = NULL;
				if(sp) {
					for(m = sp->list; m; m = m->next) {
						if(is_eq_nons(m->name, p->sym->name)) {
							if(m->mess&MIMEIN)
								mimein = 1;
							if(m->mess&MIMEOUT)
								mimeout = 1;
							if(m->mess == ENCODING)
								method_encoding = m->part;
							else if(m->mess == RESPONSE_ENCODING)
								method_response_encoding = m->part;
							else if(m->mess == STYLE)
								method_style = m->part;
							else if(m->mess == COMMENT)
								comment = m->part;
						}
					}
				}
				if(!method_response_encoding)
					method_response_encoding = method_encoding;
				if(get_response(p->info.typ))
					fprintf(fd, "<message name=\"%sRequest\">\n", ns_remove(p->sym->name));
				else
					fprintf(fd, "<message name=\"%s\">\n", ns_remove(p->sym->name));
				fflush(fd);
				if(is_document(method_style)) {
					if(is_invisible(p->sym->name)) {
						q = entry(classtable, p->sym);
						if(q) {
							q = ((Table *)q->info.typ->ref)->list;
							if(q) {
								if(is_invisible(q->sym->name)) {
									r = entry(classtable, q->sym);
									if(r) {
										r = ((Table *)r->info.typ->ref)->list;
										if(r) {
											fprintf(fd, " <part name=\"parameters\" element=\"%s\"", ns_add(r->sym->name, ns));
											if(gen_member_documentation(fd, p->sym, r, ns))
												fprintf(fd, " </part>\n");
										}
									}
								}
								else {
									fprintf(fd, " <part name=\"parameters\" element=\"%s\"", ns_add(q->sym->name, ns));
								      if(gen_member_documentation(fd, p->sym, q, ns))
									      fprintf(fd, " </part>\n"); }
							}
						}
					}
					else {fprintf(fd, " <part name=\"parameters\" element=\"%s\"", ns_add(p->sym->name, ns));
					      if(gen_member_documentation(fd, p->sym, p, ns))
						      fprintf(fd, " </part>\n"); }
				}
				else {q = entry(classtable, p->sym);
				      if(q)
					      for(q = ((Table *)q->info.typ->ref)->list; q; q = q->next) {
						      if(!is_transient(q->info.typ) && !(q->info.sto&Sattribute) &&
						         q->info.typ->type != Tfun && !is_repetition(q) &&
						         !is_anytype(q)) {
							      if(is_literal(method_encoding)) {
								      fprintf(fd, " <part name=\"%s\" element=\"%s\"", ns_remove(q->sym->name), ns_add(q->sym->name, ns));
								      if(gen_member_documentation(fd, p->sym, q, ns))
									      fprintf(fd, " </part>\n");
							      }
							      else if(is_XML(q->info.typ) || is_stdXML(q->info.typ))
								      fprintf(fd, " <part name=\"parameters\" type=\"xsd:anyType\"/>\n");
							      else {fprintf(fd, " <part name=\"%s\" type=\"%s\"", ns_remove(q->sym->name), wsdl_type(q->info.typ, ns));
								    if(gen_member_documentation(fd, p->sym, q, ns))
									    fprintf(fd, " </part>\n"); }
						      }
					      }
				}
				if(mimein)
					fprintf(fd, " <part name=\"attachments\" type=\"xsd:base64Binary\"/>\n");
				fprintf(fd, "</message>\n\n");
				fflush(fd);
				q = (Entry *)p->info.typ->ref;
				for(r = t->list; r; r = r->next)
					if(r != p && r->info.typ->type == Tfun && !(r->info.sto&Sextern) && q ==
					   (Entry *)r->info.typ->ref)
						q = NULL;
				if(q && is_transient(q->info.typ))
					;
				else if(q && !is_response(q->info.typ)) {
					fprintf(fd, "<message name=\"%sResponse\">\n", ns_remove(p->sym->name));
					if(is_document(method_style))
						fprintf(fd, " <part name=\"parameters\" element=\"%sResponse\"/>\n",
							ns_add(p->sym->name,
								ns));
					else if(is_literal(method_response_encoding)) {
						fprintf(fd, " <part name=\"%s\" element=\"%s\"", ns_remove(
								q->sym->name), ns_add(q->sym->name, ns));
						if(gen_member_documentation(fd, p->sym, q, ns))
							fprintf(fd, " </part>\n");
					}
					else if(is_XML((Tnode *)q->info.typ->ref) ||
					        is_stdXML((Tnode *)q->info.typ->ref))
						fprintf(fd, " <part name=\"parameters\" type=\"xsd:anyType\"/>\n");
					else {fprintf(fd, " <part name=\"%s\" type=\"%s\"", ns_remove(
							      q->sym->name), wsdl_type(q->info.typ, ns));
					      if(gen_member_documentation(fd, p->sym, q, ns))
						      fprintf(fd, " </part>\n"); }
					if(mimeout)
						fprintf(fd, " <part name=\"attachments\" type=\"xsd:base64Binary\"/>\n");
					fprintf(fd, "</message>\n\n");
				}
				else if(q && q->info.typ->wsdl == False) {
					q->info.typ->wsdl = True;
					fprintf(fd, "<message name=\"%s\">\n",
						ns_remove(((Tnode *)q->info.typ->ref)->id->name));
					if(is_document(method_style)) {
						if(has_ns_eq(NULL, ((Entry *)p->info.typ->ref)->sym->name))
							fprintf(fd, " <part name=\"parameters\" element=\"%s\"/>\n",
								ns_convert(
									((Entry *)p->info.typ->ref)->sym->name));
						else if(is_invisible(((Tnode *)q->info.typ->ref)->id->name)) {
							r = ((Table *)((Tnode *)q->info.typ->ref)->ref)->list;
							if(r) {
								fprintf(fd, " <part name=\"parameters\" element=\"%s\"",
									ns_add(r->sym->name,
										ns));
								if(gen_member_documentation(fd, p->sym, r, ns))
									fprintf(fd, " </part>\n");
							}
						}
						else {fprintf(fd, " <part name=\"parameters\" element=\"%s\"",
							      ns_convert(
								      ((Tnode *)q->info.typ->ref)->id->name));
						      if(gen_member_documentation(fd, p->sym, q, ns))
							      fprintf(fd, " </part>\n"); }
					}
					else {if(((Tnode *)q->info.typ->ref)->ref)      {
						      for(q = ((Table *)((Tnode *)q->info.typ->ref)->ref)->list; q;
						          q = q->next)                                            {
							      if(!is_transient(q->info.typ) &&
							         !(q->info.sto&Sattribute) && q->info.typ->type !=
							         Tfun && !is_repetition(q) &&
							         !is_anytype(q))
							      {
								      if(is_literal(method_response_encoding))
								      {
									      fprintf(
										      fd,
										      " <part name=\"%s\" element=\"%s\"",
										      ns_remove(q->sym->name),
										      ns_add(q->sym->name, ns));
									      if(gen_member_documentation(fd, p->sym, q,
											 ns))
										      fprintf(fd, " </part>\n");
								      }
								      else if(is_XML(q->info.typ) ||
								              is_stdXML(q->info.typ))
									      fprintf(
										      fd,
										      " <part name=\"parameters\" type=\"xsd:anyType\"/>\n");
								      else {fprintf(fd,
										    " <part name=\"%s\" type=\"%s\"",
										    ns_remove(
											    q->sym->name),
										    wsdl_type(q->info.typ, ns));
									    if(gen_member_documentation(fd, p->sym, q,
										       ns))
										    fprintf(fd, " </part>\n"); }
							      }
						      }
					      }
					}
					if(mimeout)
						fprintf(fd, " <part name=\"attachments\" type=\"xsd:base64Binary\"/>\n");
					fprintf(fd, "</message>\n\n");
				}
				fflush(fd);
			}
		}
		if(custom_header) {
			Table * r;
			fprintf(fd, "<message name=\"%sHeader\">\n", name);
			r = (Table *)entry(classtable, lookup("SOAP_ENV__Header"))->info.typ->ref;
			if(r) {
				for(q = r->list; q; q = q->next)        {
					if(!is_transient(q->info.typ) && !(q->info.sto&Sattribute) &&
					   q->info.typ->type != Tfun && !is_repetition(q) && !is_anytype(q))
						fprintf(fd, " <part name=\"%s\" element=\"%s\"/>\n",
							ns_remove(q->sym->name), ns_add(q->sym->name, ns));
				}
			}
			fprintf(fd, "</message>\n\n");
		}
		if(custom_fault) {
			Table * r;
			fprintf(fd, "<message name=\"%sFault\">\n", name);
			r = (Table *)entry(classtable, lookup("SOAP_ENV__Detail"))->info.typ->ref;
			if(r)
				for(q = r->list; q; q = q->next)
					if(!is_transient(q->info.typ) && !is_repetition(q) && !is_anytype(q) && !(q->info.sto&Sattribute) && q->info.typ->type != Tfun && has_ns_eq(NULL, q->sym->name)) {
						fprintf(fd, " <part name=\"%s\" element=\"%s\"", ns_remove(q->sym->name), ns_add(q->sym->name, ns));
						if(gen_member_documentation(fd, p->sym, q, ns))
							fprintf(fd, " </part>\n");
					}
			fprintf(fd, "</message>\n\n");
		}
		if(sp) {
			for(m = sp->list; m; m = m->next) {
				if(m->mess&FAULT && m->part) {
					Method * m2;
					int flag = 0;
					for(m2 = sp->list; m2 && m2 != m; m2 = m2->next)
						if(m2->mess&FAULT && sstreq(m2->part, m->part))
							flag = 1;
					if(!flag) {
						if(typetable)
							for(p = typetable->list; p; p = p->next)
								if((m->mess&FAULT) &&
								   is_eq(m->part, p->info.typ->sym->name))
									break;
						if(!p && classtable)
							for(p = classtable->list; p; p = p->next)
								if((m->mess&FAULT) &&
								   is_eq(m->part, p->info.typ->id->name))
									break;
						if(p) {
							fprintf(fd, "<message name=\"%sFault\">\n", ns_remove(m->part));
							fprintf(fd, " <part name=\"fault\" element=\"%s\"/>\n",
								ns_convert(
									m->part));
							fprintf(fd, "</message>\n\n");
							flag = 0;
							if(custom_fault) {
								Table * r;
								r = (Table *)entry(classtable, lookup("SOAP_ENV__Detail"))->info.typ->ref;
								if(r)
									for(q = r->list; q; q = q->next)
										if(!is_transient(q->info.typ) && !is_repetition(q) && !is_anytype(q) &&
										   !(q->info.sto&Sattribute) && q->info.typ->type != Tfun && (sstreq(q->sym->name, m->part) || sstreq(q->sym->name+1, m->part))) {
											flag = 1;
											break;
										}
							}
							if(!flag) {
								sprintf(errbuf,
									"//gsoap %s method-fault %s %s directive does not refer to a member of struct SOAP_ENV__Detail: suggest to define struct SOAP_ENV__Detail with member %s",
									sp->ns, m->name, m->part, m->part);
								semwarn(errbuf);
							}
						}
						else {
							sprintf(errbuf,
							      "//gsoap %s method-fault %s %s directive does not refer to struct/class or typedef: should globablly define fault %s as type (typedef or struct/class)",
							      sp->ns, m->name, m->part, m->part);
						      semwarn(errbuf); }
					}
				}
			}
		}
		fflush(fd);
		if(sp && sp->porttype)
			fprintf(fd, "<portType name=\"%s\">\n", sp->porttype);
		else
			fprintf(fd, "<portType name=\"%s\">\n", ns_cname(name, "PortType"));
		for(p = t->list; p; p = p->next) {
			if(p->info.typ->type == Tfun && !(p->info.sto&Sextern) &&
			   has_ns_eq(ns, p->sym->name))                                   {
				comment = NULL;
				if(sp)
					for(m = sp->list; m; m = m->next)
						if(m->mess == COMMENT && is_eq_nons(m->name, p->sym->name))
							comment = m->part;
				fprintf(fd, " <operation name=\"%s\">\n", ns_remove(p->sym->name));
				if(comment)
					fprintf(fd, "  <documentation>%s</documentation>\n", comment);
				else
					fprintf(fd,
						"  <documentation>Service definition of function %s</documentation>\n",
						p->sym->name);
				if(get_response(p->info.typ))
					fprintf(fd, "  <input message=\"tns:%sRequest\"/>\n", ns_remove(p->sym->name));
				else
					fprintf(fd, "  <input message=\"tns:%s\"/>\n", ns_remove(p->sym->name));
				q = (Entry *)p->info.typ->ref;
				if(q && is_transient(q->info.typ))
					;
				else if(q && !is_response(q->info.typ))
					fprintf(fd, "  <output message=\"tns:%sResponse\"/>\n", ns_remove(p->sym->name));
				else if(q)
					fprintf(fd, "  <output message=\"tns:%s\"/>\n",
						ns_remove(((Tnode *)q->info.typ->ref)->id->name));
				if(sp)
					for(m = sp->list; m; m = m->next)
						if((m->mess&FAULT) && is_eq_nons(m->name, p->sym->name))
							fprintf(fd, "  <fault name=\"%s\" message=\"tns:%sFault\"/>\n",
								ns_remove(
									m->part), ns_remove(m->part));
				fprintf(fd, " </operation>\n");
			}
		}
		fprintf(fd, "</portType>\n\n");
		fprintf(fd, "<binding name=\"%s\" ", binding);
		if(is_document(style))
			if(sp && sp->porttype)
				fprintf(fd, "type=\"tns:%s\">\n <SOAP:binding style=\"document\"", sp->porttype);
			else
				fprintf(fd, "type=\"tns:%s\">\n <SOAP:binding style=\"document\"",
					ns_cname(name, "PortType"));
		else
		if(sp && sp->porttype)
			fprintf(fd, "type=\"tns:%s\">\n <SOAP:binding style=\"rpc\"", sp->porttype);
		else
			fprintf(fd, "type=\"tns:%s\">\n <SOAP:binding style=\"rpc\"", ns_cname(name, "PortType"));
		if(sp && sp->transport)
			fprintf(fd, " transport=\"%s\"/>\n", sp->transport);
		else
			fprintf(fd, " transport=\"http://schemas.xmlsoap.org/soap/http\"/>\n");
		fflush(fd);
		for(p = t->list; p; p = p->next) {
			if(p->info.typ->type == Tfun && !(p->info.sto&Sextern) &&
			   has_ns_eq(ns, p->sym->name))                                   {
				action = "";
				mimein = 0;
				mimeout = 0;
				method_style = style;
				method_encoding = encoding;
				method_response_encoding = NULL;
				if(sp) {
					for(m = sp->list; m; m = m->next)         {
						if(is_eq_nons(m->name,
							   p->sym->name))                                            {
							if(m->mess&MIMEIN)
								mimein = 1;
							if(m->mess&MIMEOUT)
								mimeout = 1;
							if(m->mess == ENCODING)
								method_encoding = m->part;
							if(m->mess == RESPONSE_ENCODING)
								method_response_encoding = m->part;
							if(m->mess == STYLE)
								method_style = m->part;
							if(m->mess == ACTION || m->mess == REQUEST_ACTION)
								action = m->part;
							if(m->mess == RESPONSE_ACTION)
								action = m->part;
						}
					}
				}
				if(!method_response_encoding)
					method_response_encoding = method_encoding;
				fprintf(fd, " <operation name=\"%s\">\n", ns_remove(p->sym->name));
				if(is_document(style)) {
					if(is_document(method_style))                         {
						if(is_soap12(encoding) && !*action)
							fprintf(fd, "  <SOAP:operation/>\n");
						else if(*action == '"')
							fprintf(fd, "  <SOAP:operation soapAction=%s/>\n", action);
						else
							fprintf(fd, "  <SOAP:operation soapAction=\"%s\"/>\n", action);
					}
					else if(is_soap12(encoding) && !*action)
						fprintf(fd, "  <SOAP:operation style=\"rpc\"/>\n");
					else if(*action == '"')
						fprintf(fd, "  <SOAP:operation style=\"rpc\" soapAction=%s/>\n", action);
					else
						fprintf(fd, "  <SOAP:operation style=\"rpc\" soapAction=\"%s\"/>\n",
							action);
				}
				else {if(is_document(method_style))      {
					      if(is_soap12(encoding) && !*action)
						      fprintf(fd, "  <SOAP:operation style=\"document\"/>\n");
					      else if(*action == '"')
						      fprintf(fd,
							      "  <SOAP:operation style=\"document\" soapAction=%s/>\n",
							      action);
					      else
						      fprintf(
							      fd,
							      "  <SOAP:operation style=\"document\" soapAction=\"%s\"/>\n",
							      action);
				      }
				      else if(is_soap12(encoding) && !*action)
					      fprintf(fd, "  <SOAP:operation style=\"rpc\"/>\n");
				      else if(*action == '"')
					      fprintf(fd, "  <SOAP:operation style=\"rpc\" soapAction=%s/>\n", action);
				      else
					      fprintf(fd, "  <SOAP:operation style=\"rpc\" soapAction=\"%s\"/>\n",
						      action); }
				fprintf(fd, "  <input>\n");
				if(mimein)
					fprintf(fd, "   <MIME:multipartRelated>\n    <MIME:part>\n");
				q = entry(classtable, p->sym);
				if(is_literal(method_encoding) ||
				   (q && (q = (((Table *)q->info.typ->ref)->list)) && q && is_XML(q->info.typ))) {
					if(is_document(method_style))
						fprintf(fd, "     <SOAP:body parts=\"parameters\" use=\"literal\"/>\n");
					else
						fprintf(
							fd,
							"     <SOAP:body parts=\"parameters\" use=\"literal\" namespace=\"%s\"/>\n",
							URI);
				}
				else {if(encoding && *encoding)
					      fprintf(
						      fd,
						      "     <SOAP:body use=\"encoded\" namespace=\"%s\" encodingStyle=\"%s\"/>\n",
						      URI, encoding);
				      else if(method_encoding && *method_encoding)
					      fprintf(
						      fd,
						      "     <SOAP:body use=\"encoded\" namespace=\"%s\" encodingStyle=\"%s\"/>\n",
						      URI, method_encoding);
				      else
					      fprintf(
						      fd,
						      "     <SOAP:body use=\"encoded\" namespace=\"%s\" encodingStyle=\"%s\"/>\n",
						      URI, encURI);
				      if(!eflag) {
					      sprintf(
						      errbuf,
						      "operation '%s' is not compliant with WS-I Basic Profile 1.0a, reason: uses SOAP encoding",
						      p->sym->name);
					      compliancewarn(errbuf);
				      }
				}
				if(custom_header) {
					m = NULL;
					if(sp)
						for(m = sp->list; m; m = m->next)
							if(is_eq_nons(m->name, p->sym->name) && (m->mess&HDRIN)) {
								if(chkhdr(m->part))
									fprintf(
										fd,
										"     <SOAP:header use=\"literal\" message=\"tns:%sHeader\" part=\"%s\"/>\n",
										name, ns_remove(m->part));
							}
				}
				if(mimein) {
					if(sp)             {
						for(m = sp->list; m; m = m->next)                     {
							if(is_eq_nons(m->name, p->sym->name) && (m->mess&MIMEIN))
								fprintf(
									fd,
									"    </MIME:part>\n    <MIME:part>\n     <MIME:content part=\"attachments\" type=\"%s\"/>\n",
									m->part);
						}
					}
					fprintf(fd, "    </MIME:part>\n   </MIME:multipartRelated>\n");
				}
				fprintf(fd, "  </input>\n");
				q = (Entry *)p->info.typ->ref;
				if(!q || !q->info.typ->ref) {
					fprintf(fd, " </operation>\n");
					continue;
				}
				fprintf(fd, "  <output>\n");
				if(mimeout)
					fprintf(fd, "   <MIME:multipartRelated>\n    <MIME:part>\n");
				if(is_literal(method_response_encoding) || is_XML((Tnode *)q->info.typ->ref)) {
					if(is_document(method_style))
						fprintf(fd, "     <SOAP:body parts=\"parameters\" use=\"literal\"/>\n");
					else
						fprintf(
							fd,
							"     <SOAP:body parts=\"parameters\" use=\"literal\" namespace=\"%s\"/>\n",
							URI);
				}
				else if(encoding && *encoding)
					fprintf(
						fd,
						"     <SOAP:body use=\"encoded\" namespace=\"%s\" encodingStyle=\"%s\"/>\n",
						URI, encoding);
				else if(method_response_encoding && *method_response_encoding)
					fprintf(
						fd,
						"     <SOAP:body use=\"encoded\" namespace=\"%s\" encodingStyle=\"%s\"/>\n",
						URI, method_response_encoding);
				else
					fprintf(
						fd,
						"     <SOAP:body use=\"encoded\" namespace=\"%s\" encodingStyle=\"%s\"/>\n",
						URI, encURI);
				if(custom_header) {
					if(sp)
						for(m = sp->list; m; m = m->next)
							if(is_eq_nons(m->name, p->sym->name) && (m->mess&HDROUT)) {
								if(chkhdr(m->part))
									fprintf(
										fd,
										"     <SOAP:header use=\"literal\" message=\"tns:%sHeader\" part=\"%s\"/>\n",
										name, ns_remove(m->part));
							}
				}
				if(mimeout) {
					if(sp)              {
						for(m = sp->list; m; m = m->next)                      {
							if(is_eq_nons(m->name, p->sym->name) && (m->mess&MIMEOUT))
								fprintf(
									fd,
									"    </MIME:part>\n    <MIME:part>\n     <MIME:content part=\"attachments\" type=\"%s\"/>\n",
									m->part);
						}
					}
					fprintf(fd, "    </MIME:part>\n   </MIME:multipartRelated>\n");
				}
				fprintf(fd, "  </output>\n");
				if(sp)
					for(m = sp->list; m; m = m->next)
						if((m->mess&FAULT) && is_eq_nons(m->name, p->sym->name))
							fprintf(
								fd,
								"  <fault name=\"%s\">\n   <SOAP:fault name=\"%s\" use=\"literal\"/>\n  </fault>\n",
								ns_remove(m->part), ns_remove(m->part));
				fprintf(fd, " </operation>\n");
				fflush(fd);
			}
		}
		fprintf(fd, "</binding>\n\n");
	}
	fprintf(fd, "<service name=\"%s\">\n", name);
	if(sp && sp->documentation)
		fprintf(fd, " <documentation>%s</documentation>\n", sp->documentation);
	else
		fprintf(fd, " <documentation>gSOAP " VERSION " generated service definition</documentation>\n");
	if(executable)
		fprintf(fd, " <port name=\"%s\" binding=\"tns:%s\">\n  <SOAP:address location=\"%s/%s\"/>", portname, binding, URL, executable);
	else {const char * s, * t;
	      fprintf(fd, " <port name=\"%s\" binding=\"tns:%s\">", portname, binding);
	      for(s = URL; s; s = t) {
		      int n;
		      t = strchr(s, ' ');
		      if(t) {
			      n = (int)(t-s);
			      t++;
		      }
		      else
			      n = (int)strlen(s);
		      fprintf(fd, "\n  <SOAP:address location=\"%.*s\"/>", n, s);
	      }
	}
	fprintf(fd, "\n </port>\n</service>\n\n</definitions>\n");
}

char * default_value(Entry * e, const char * a)
{
	Entry * q;
	static char buf[1024];
	buf[0] = '\0';
	if(e->info.hasval)
		switch(e->info.typ->type) {
		    case Tchar:
		    case Twchar:
		    case Tuchar:
		    case Tshort:
		    case Tushort:
		    case Tint:
		    case Tuint:
		    case Tlong:
		    case Tllong:
		    case Tulong:
		    case Tullong:
			sprintf(buf, " %s=\"" SOAP_LONG_FORMAT "\"", a, e->info.val.i);
			break;
		    case Tfloat:
		    case Tdouble:
		    case Tldouble:
			sprintf(buf, " %s=\"%f\"", a, e->info.val.r);
			break;
		    case Ttime:
			break; /* should get value? */
		    case Tenum:
			for(q = ((Table *)e->info.typ->ref)->list; q; q = q->next)
				if(q->info.val.i == e->info.val.i) {
					sprintf(buf, " %s=\"%s\"", a, ns_convert(q->sym->name));
					break;
				}
			break;
		    default:
			if(e->info.val.s && strlen(e->info.val.s) < sizeof(buf)-12)
				sprintf(buf, " %s=\"%s\"", a, xstring(e->info.val.s));
			break;
		}
	return buf;
}

const char * nillable(Tnode * typ)
{
	if(typ->type == Tpointer)
		return "true";
	return "false";
}

void gen_schema(FILE * fd, Table * t, char * ns1, char * ns, int all, int wsdl, char * URL, char * URI, char * style,
	char * encoding)
{
	int i, d;
	char cbuf[4];
	Entry * p, * q, * r;
	Tnode * n;
	Symbol * s;
	Service * sp, * sp2;
	Method * m;
	int flag;
	if(sstreq(ns, S_SoapEnvNs) || sstreq(ns, S_SoapEncNs) || sstreq(ns, "xsi") || sstreq(ns, "xsd"))
		return;
	for(sp = services; sp; sp = sp->next)
		if(!tagcmp(sp->ns, ns) && sp->URI)
			break;
	if(sp && sp->import)
		return;
	fprintf(fd, " <schema ");
	if(sp)
		fprintf(fd, "targetNamespace=\"%s\"", sp->URI);
	else
		fprintf(fd, "targetNamespace=\"%s/%s.xsd\"", tmpURI, ns_convert(ns));
	for(s = nslist; s; s = s->next) {
		for(sp2 = services; sp2; sp2 = sp2->next)
			if(!tagcmp(sp2->ns, s->name) && sp2->URI)
				break;
		if(sp2)
			fprintf(fd, "\n  xmlns:%s=\"%s\"", ns_convert(s->name), sp2->URI);
		else if(sstreq(s->name, S_SoapEnvNs))
			fprintf(fd, "\n  xmlns:%s=\"%s\"", S_SoapEnvNs, envURI);
		else if(sstreq(s->name, S_SoapEncNs))
			fprintf(fd, "\n  xmlns:%s=\"%s\"", S_SoapEncNs, encURI);
		else if(sstreq(s->name, "xsi"))
			fprintf(fd, "\n  xmlns:xsi=\"%s\"", xsiURI);
		else if(sstreq(s->name, "xsd"))
			fprintf(fd, "\n  xmlns:xsd=\"%s\"", xsdURI);
		else
			fprintf(fd, "\n  xmlns:%s=\"%s/%s.xsd\"", ns_convert(s->name), tmpURI, ns_convert(s->name));
	}
	fprintf(fd, "\n  xmlns=\"%s\"\n", xsdURI);
	if(sp && (sp->elementForm || sp->attributeForm))
		fprintf(fd, "  elementFormDefault=\"%s\"\n  attributeFormDefault=\"%s\">\n",
			sp->elementForm ? sp->elementForm : "unqualified", sp->attributeForm ? sp->attributeForm : "unqualified");
	else if(style && sstreq(style, "document"))
		fprintf(fd, "  elementFormDefault=\"qualified\"\n  attributeFormDefault=\"unqualified\">\n");
	else
		fprintf(fd, "  elementFormDefault=\"unqualified\"\n  attributeFormDefault=\"unqualified\">\n");
	fflush(fd);
	flag = 0;
	for(s = nslist; s; s = s->next) {
		for(sp2 = services; sp2; sp2 = sp2->next)
			if(sp2 != sp && !tagcmp(sp2->ns, s->name) && sp2->URI)
				break;
		if(sp2) {
			fprintf(fd, "  <import namespace=\"%s\"", sp2->URI);
			if(sp2->import)
				fprintf(fd, " schemaLocation=\"%s\"", sp2->import);
			fprintf(fd, "/>\n");
			if(sstreq(sp2->URI, encURI))
				flag = 1;
		}
	}
	if(!flag)
		fprintf(fd, "  <import namespace=\"%s\"/>", encURI);
	fprintf(fd, "\n");
	fflush(fd);
	if(typetable) {
		for(p = typetable->list; p; p = p->next)                {
			if(p->info.typ->type != Ttemplate && !is_transient(p->info.typ) &&
			   !is_invisible(p->sym->name) &&
			   (!is_external(p->info.typ) ||
			    is_volatile(p->info.typ)) &&
			   ((has_ns_eq(ns, p->sym->name))))                                                          { /* typedefs that are used for SOAP Fault details */
				m = NULL;
				if(p->info.typ->type != Tstruct && p->info.typ->type != Tclass) {
					for(sp2 = services; sp2 && !m; sp2 =
					            sp2->next)
					{
						for(m = sp2->list; m; m =
						            m->next)
						{
							if((m->mess&FAULT) && m->part && is_eq(m->part, p->sym->name))
								break;
						}
					}
				}
				if(m) {
					if(!uflag)
						fprintf(fd, "  <!-- fault element -->\n");
					fprintf(fd, "  <element name=\"%s\" type=\"%s\">\n", ns_remove(p->sym->name), base_type(p->info.typ, ns1));
					gen_type_documentation(fd, p, ns);
					fprintf(fd, "  </element>\n");
					continue;
				}
				if(is_primitive_or_string(p->info.typ) || (p->info.typ->type == Tpointer && is_primitive_or_string((Tnode *)p->info.typ->ref))) {
					fprintf(fd, "  <simpleType name=\"%s\">", ns_remove(p->sym->name));
					gen_type_documentation(fd, p, ns);
					fprintf(fd, "   <restriction base=\"%s\">\n", base_type(p->info.typ, ns1));
					if(p->info.typ->pattern)
						fprintf(fd, "    <pattern value=\"%s\"/>\n", p->info.typ->pattern);
					if(is_primitive(p->info.typ) || (p->info.typ->type == Tpointer && is_primitive((Tnode *)p->info.typ->ref) &&
					    !is_string(p->info.typ) && !is_wstring(p->info.typ))) {
						if(p->info.typ->minLength != MINLONG64)
							fprintf(fd, "    <minInclusive value=\"" SOAP_LONG_FORMAT "\"/>\n",
								p->info.typ->minLength);
						if(p->info.typ->maxLength != MAXLONG64)
							fprintf(fd, "    <maxInclusive value=\"" SOAP_LONG_FORMAT "\"/>\n", p->info.typ->maxLength);
					}
					else {if(p->info.typ->maxLength > 0 && p->info.typ->minLength ==
						 p->info.typ->maxLength)
						      fprintf(fd, "    <length value=\"" SOAP_LONG_FORMAT "\"/>\n", p->info.typ->minLength);
					      else {if(p->info.typ->minLength > 0)
							    fprintf(fd, "    <minLength value=\"" SOAP_LONG_FORMAT "\"/>\n", p->info.typ->minLength);
						    if(p->info.typ->maxLength > 0)
							    fprintf(fd, "    <maxLength value=\"" SOAP_LONG_FORMAT "\"/>\n", p->info.typ->maxLength); }}
					fprintf(fd, "   </restriction>\n  </simpleType>\n");
				}
				else {fprintf(fd, "  <complexType name=\"%s\">", ns_remove(p->sym->name));
				      gen_type_documentation(fd, p, ns);
				      fprintf(fd, "   <complexContent>\n    <restriction base=\"%s\">\n", base_type(p->info.typ, ns1));
				      fprintf(fd, "    </restriction>\n   </complexContent>\n  </complexType>\n"); }
			}
		}
	}
	fflush(fd);
	if(enumtable) {
		for(p = enumtable->list; p; p = p->next)                {
			if(!is_transient(p->info.typ) && !is_invisible(p->sym->name) &&
			   ((!has_ns(p->info.typ) &&
			     all) ||
			    has_ns_eq(ns, p->sym->name)))                                                          {
				if(is_mask(p->info.typ))
				{
					fprintf(fd, "  <simpleType name=\"%s\">", wsdl_type(p->info.typ, NULL));
					gen_type_documentation(fd, p, ns);
					fprintf(fd, "   <list>\n");
					q = p;
					if((Table *)p->info.typ->ref) {
						for(q = ((Table *)p->info.typ->ref)->list; q; q = q->next)
							if(!has_ns_eq(NULL,
								   ns_remove1(((Table *)p->info.typ->ref)->list->sym->
									   name)))
								break;
					}
					if(q)
						fprintf(fd, "    <restriction base=\"xsd:string\">\n");
					else
						fprintf(fd, "    <restriction base=\"xsd:QName\">\n");
					if((Table *)p->info.typ->ref) {
						for(q = ((Table *)p->info.typ->ref)->list; q; q =
						            q->next)                               {
							fprintf(fd, "     <enumeration value=\"%s\"",
								ns_remove2(q->sym->name));
							if(gen_member_documentation(fd, p->sym, q, ns))
								fprintf(fd, "     </enumeration>");
							if(!uflag)
								fprintf(fd, "    <!-- = " SOAP_LONG_FORMAT " -->", q->info.val.i);
							fprintf(fd, "\n");
						}
					}
					fprintf(fd, "    </restriction>\n   </list>\n  </simpleType>\n");
				}
				else {fprintf(fd, "  <simpleType name=\"%s\">", wsdl_type(p->info.typ, NULL));
				      gen_type_documentation(fd, p, ns);
				      q = p;
				      if((Table *)p->info.typ->ref) {
					      for(q = ((Table *)p->info.typ->ref)->list; q; q = q->next)
						      if(!has_ns_eq(NULL,
								 ns_remove1(((Table *)p->info.typ->ref)->list->sym->
									 name)))
							      break;
				      }
				      if(q)
					      fprintf(fd, "   <restriction base=\"xsd:string\">\n");
				      else
					      fprintf(fd, "   <restriction base=\"xsd:QName\">\n");
				      if((Table *)p->info.typ->ref) {
					      for(q = ((Table *)p->info.typ->ref)->list; q; q =
					                  q->next)                               {
						      fprintf(fd, "    <enumeration value=\"%s\"",
							      ns_remove2(q->sym->name));
						      if(gen_member_documentation(fd, p->sym, q, ns))
							      fprintf(fd, "    </enumeration>");
						      if(!uflag)
							      fprintf(fd, "    <!-- = " SOAP_LONG_FORMAT " -->",
								      q->info.val.i);
						      fprintf(fd, "\n");
					      }
				      }
				      fprintf(fd, "   </restriction>\n  </simpleType>\n"); }
			}
		}
	}
	fflush(fd);
	if(classtable) {
		for(p = classtable->list; p; p = p->next)                 {
			if(is_transient(p->info.typ) || is_invisible(p->sym->name))
				continue;
			for(q = t->list; q; q = q->next)
				if(q->info.typ->type == Tfun && !(q->info.sto&Sextern) && p == get_response(q->info.typ))
					break;
			/* omit the auto-generated and user-defined response struct/class (when necessary) */
			if(!q)
				for(q = t->list; q; q = q->next)
					if(q->info.typ->type == Tfun && !(q->info.sto&Sextern) &&
					   !has_ns_eq(NULL, ((Entry *)q->info.typ->ref)->sym->name)) {
						r = entry(t, q->sym);
						if(r && r->info.typ->ref &&
						   is_response(((Entry *)r->info.typ->ref)->info.typ) && p->info.typ ==
						   (Tnode *)((Entry *)r->info.typ->ref)->info.typ->ref)
							break;
					}
			if(q)
				continue;
			/* classes that are used for SOAP Fault details */
			m = NULL;
			for(sp2 = services; sp2 && !m; sp2 = sp2->next)
				for(m = sp2->list; m; m = m->next)
					if((m->mess&FAULT) && m->part && is_eq(m->part, p->sym->name))
						break;
			if(m) {
				if((!has_ns(p->info.typ) && all) || has_ns_eq(ns, p->sym->name))        {
					if(!uflag)
						fprintf(fd, "  <!-- fault element and type -->\n");
					fprintf(fd, "  <element name=\"%s\" type=\"%s\">\n", ns_remove(
							p->sym->name), base_type(p->info.typ, ns1));
					gen_type_documentation(fd, p, ns);
					fprintf(fd, "  </element>\n");
				}
			}
			if(p->info.typ->ref && is_binary(p->info.typ)) {
				if((!has_ns(p->info.typ) &&
				    all) ||
				   has_ns_eq(ns, p->sym->name))                                                 {
					if(is_attachment(p->info.typ))
					{
						fprintf(fd, "  <complexType name=\"%s\">", ns_remove(p->sym->name));
						gen_type_documentation(fd, p, ns);
						fprintf(
							fd,
							"   <simpleContent>\n    <extension base=\"xsd:base64Binary\">\n");
						if(!eflag)
							fprintf(
								fd,
								"     <attribute name=\"href\" type=\"xsd:anyURI\" use=\"optional\"/>\n");
						gen_schema_attributes(fd, p->info.typ, ns, ns1);
						fprintf(fd, "    </extension>\n   </simpleContent>\n  </complexType>\n");
					}
					else {fprintf(fd, "  <simpleType name=\"%s\">", ns_remove(p->sym->name));
					      gen_type_documentation(fd, p, ns);
					      fprintf(fd, "   <restriction base=\"xsd:base64Binary\">\n");
					      if(p->info.typ->maxLength > 0 && p->info.typ->minLength ==
						 p->info.typ->maxLength)
						      fprintf(fd, "    <length value=\"" SOAP_LONG_FORMAT "\"/>\n",
							      p->info.typ->minLength);
					      else {if(p->info.typ->minLength > 0)
							    fprintf(fd,
								    "    <minLength value=\"" SOAP_LONG_FORMAT "\"/>\n",
								    p->info.typ->minLength);
						    if(p->info.typ->maxLength > 0)
							    fprintf(fd,
								    "    <maxLength value=\"" SOAP_LONG_FORMAT "\"/>\n",
								    p->info.typ->maxLength); }
					      fprintf(fd, "   </restriction>\n  </simpleType>\n"); }
				}
			}
			else if(p->info.typ->ref && !is_transient(p->info.typ) && is_primclass(p->info.typ)) {
				if((!has_ns(p->info.typ) &&
				    all) ||
				   has_ns_eq(ns,
					   p->sym->name))                                                                                       {
					q = ((Table *)p->info.typ->ref)->list;
					if(q && strncmp(q->sym->name, "xsd__anyType", 12)) {
						if(is_string(q->info.typ) || is_wstring(q->info.typ) ||
						   is_stdstring(q->info.typ) ||
						   is_stdwstring(q->info.typ))
						{
							fprintf(fd, "  <complexType name=\"%s\" mixed=\"true\">",
								ns_remove(
									p->sym->name));
							gen_type_documentation(fd, p, ns);
							fprintf(fd, "   <simpleContent>\n    <extension base=\"%s\">\n",
								wsdl_type(q->info.typ,
									ns1));
							gen_schema_attributes(fd, p->info.typ, ns, ns1);
							fprintf(
								fd,
								"    </extension>\n   </simpleContent>\n  </complexType>\n");
						}
						else if(is_primitive(q->info.typ)) {
							fprintf(fd, "  <complexType name=\"%s\">",
								ns_remove(p->sym->name));
							gen_type_documentation(fd, p, ns);
							fprintf(fd, "   <simpleContent>\n    <extension base=\"%s\">\n",
								wsdl_type(q->info.typ,
									ns1));
							gen_schema_attributes(fd, p->info.typ, ns, ns1);
							fprintf(
								fd,
								"    </extension>\n   </simpleContent>\n  </complexType>\n");
						}
						else {fprintf(fd, "  <complexType name=\"%s\">", ns_remove(p->sym->name));
						      gen_type_documentation(fd, p, ns);
						      fprintf(fd, "   <complexContent>\n    <extension base=\"%s\">\n",
							      wsdl_type(q->info.typ,
								      ns1));
						      gen_schema_attributes(fd, p->info.typ, ns, ns1);
						      fprintf(
							      fd,
							      "    </extension>\n   </complexContent>\n  </complexType>\n"); }
					}
				}
			}
			else if(p->info.typ->ref && !is_transient(p->info.typ)) {
				q = ((Table *)p->info.typ->ref)->list;
				if(entry(t, p->sym) && (!q || !is_XML(q->info.typ)))
					;
				else if(is_dynamic_array(p->info.typ)) {
					if(eflag || (!has_ns(p->info.typ) && !is_untyped(p->info.typ))) {
						if(all) {
							d = get_Darraydims(p->info.typ)-1;
							for(i = 0; i < d; i++)
								cbuf[i] = ',';
							cbuf[i] = '\0';
							if(q->info.maxOccurs == 1)
								fprintf(fd,
									"  <complexType name=\"%s\">\n   <complexContent>\n    <restriction base=\"%s:Array\">\n     <sequence>\n      <element name=\"%s\" type=\"%s\" minOccurs=\"0\" maxOccurs=\"unbounded\" nillable=\"%s\"/>\n     </sequence>\n     <attribute ref=\"SOAP-ENC:arrayType\" WSDL:arrayType=\"%s[%s]\"/>\n    </restriction>\n   </complexContent>\n  </complexType>\n",
									wsdl_type(p->info.typ, NULL), S_SoapEncNs, q->sym->name[5] ? ns_remove(q->sym->name+5) : "item", wsdl_type(q->info.typ, ns1),
									nillable((Tnode *)q->info.typ->ref), wsdl_type(q->info.typ, ns1), cbuf);
							else
								fprintf(fd,
									"  <complexType name=\"%s\">\n   <complexContent>\n    <restriction base=\"SOAP-ENC:Array\">\n     <sequence>\n      <element name=\"%s\" type=\"%s\" minOccurs=\"" SOAP_LONG_FORMAT
									"\" maxOccurs=\"" SOAP_LONG_FORMAT
									"\" nillable=\"%s\"/>\n     </sequence>\n     <attribute ref=\"SOAP-ENC:arrayType\" WSDL:arrayType=\"%s[%s]\"/>\n    </restriction>\n   </complexContent>\n  </complexType>\n",
									wsdl_type(p->info.typ, NULL), q->sym->name[5] ? ns_remove(q->sym->name+5) : "item",
									wsdl_type(q->info.typ, ns1), q->info.minOccurs,
									q->info.maxOccurs,
									nillable((Tnode *)q->info.typ->ref),
									wsdl_type(q->info.typ, ns1), cbuf);
						}
					}
					else if(p->info.typ->ref && ((Table *)p->info.typ->ref)->prev && !is_transient(entry(classtable,
								((Table *)p->info.typ->ref)->prev->sym)->info.typ) &&
					        strncmp(((Table *)p->info.typ->ref)->prev->sym->name, "xsd__anyType", 12)) {
						if(q->info.maxOccurs == 1)
						{
							fprintf(fd, "  <complexType name=\"%s\">", ns_remove(p->sym->name));
							gen_type_documentation(fd, p, ns);
							fprintf(fd,
								"   <complexContent>\n    <extension base=\"%s\">\n     <sequence>\n",
								ns_convert(((Table *)p->info.typ->ref)->prev->sym->name));
							fprintf(fd,
								"      <element name=\"%s\" type=\"%s\" minOccurs=\"0\" maxOccurs=\"unbounded\" nillable=\"true\"/>\n",
								q->sym->name[5] ? ns_remove(q->sym->name+5) : "item",
								wsdl_type(q->info.typ, ns1));
							fprintf(fd,
								"     </sequence>\n    </extension>\n   </complexContent>\n");
							gen_schema_attributes(fd, p->info.typ, ns, ns1);
							fprintf(fd, "  </complexType>\n");
						}
						else {
							fprintf(fd, "  <complexType name=\"%s\">", ns_remove(p->sym->name));
						      gen_type_documentation(fd, p, ns);
						      fprintf(fd,
							      "   <complexContent>\n    <extension base=\"%s\">\n     <sequence>\n",
							      ns_convert(((Table *)p->info.typ->ref)->prev->sym->name));
						      fprintf(fd,
							      "      <element name=\"%s\" type=\"%s\" minOccurs=\"" SOAP_LONG_FORMAT
							      "\" maxOccurs=\"" SOAP_LONG_FORMAT
							      "\" nillable=\"%s\"/>\n",
							      q->sym->name[5] ? ns_remove(q->sym->name+5) : "item",
							      wsdl_type(q->info.typ, ns1), q->info.minOccurs, q->info.maxOccurs,
							      nillable((Tnode *)q->info.typ->ref));
						      fprintf(fd,
							      "     </sequence>\n    </extension>\n   </complexContent>\n");
						      gen_schema_attributes(fd, p->info.typ, ns, ns1);
						      fprintf(fd, "  </complexType>\n"); }
					}
					else {if(q->info.maxOccurs == 1)      {
						      fprintf(fd, "  <complexType name=\"%s\">", ns_remove(p->sym->name));
						      gen_type_documentation(fd, p, ns);
						      fprintf(fd,
							      "   <sequence>\n    <element name=\"%s\" type=\"%s\" minOccurs=\"0\" maxOccurs=\"unbounded\" nillable=\"%s\"/>\n   </sequence>\n  </complexType>\n",
							      q->sym->name[5] ? ns_remove(q->sym->name+5) : "item",
							      wsdl_type(q->info.typ,
								      ns1), nillable((Tnode *)q->info.typ->ref));
					      }
					      else {fprintf(fd, "  <complexType name=\"%s\">", ns_remove(p->sym->name));
						    gen_type_documentation(fd, p, ns);
						    fprintf(
							    fd,
							    "   <sequence>\n    <element name=\"%s\" type=\"%s\" minOccurs=\"" SOAP_LONG_FORMAT
							    "\" maxOccurs=\"" SOAP_LONG_FORMAT
							    "\" nillable=\"%s\"/>\n   </sequence>\n  </complexType>\n",
							    q->sym->name[5] ? ns_remove(q->sym->name+5) : "item",
							    wsdl_type(q->info.typ,
								    ns1), q->info.minOccurs, q->info.maxOccurs,
							    nillable((Tnode *)q->info.typ->ref)); }}
				}
				else if(is_discriminant(p->info.typ) &&
				        ((!has_ns(p->info.typ) && all) || has_ns_eq(ns, p->sym->name))) {
					if(p->info.typ->ref)
					{
						fprintf(fd, "  <complexType name=\"%s\">\n", ns_remove(p->sym->name));
						gen_schema_elements(fd, p->info.typ, ns, ns1);
						fprintf(fd, "  </complexType>\n");
					}
				}
				else if(p->info.typ->type == Tstruct &&
				        ((!has_ns(p->info.typ) && all) || has_ns_eq(ns, p->sym->name))) {
					if(p->info.typ->ref)
					{
						fprintf(fd, "  <complexType name=\"%s\">", ns_remove(p->sym->name));
						gen_type_documentation(fd, p, ns);
						fprintf(fd, "   <sequence>\n");
						gen_schema_elements(fd, p->info.typ, ns, ns1);
						fprintf(fd, "   </sequence>\n");
						gen_schema_attributes(fd, p->info.typ, ns, ns1);
						fprintf(fd, "  </complexType>\n");
					}
				}
				else if(p->info.typ->type == Tclass &&
				        ((!has_ns(p->info.typ) && all) || has_ns_eq(ns, p->sym->name))) {
					if(p->info.typ->ref)
					{
						if(((Table *)p->info.typ->ref)->prev &&
						   !is_transient(entry(classtable,
								   ((Table *)p->info.typ->ref)->prev->sym)->info.typ)
						   &&
						   strncmp(((Table *)p->info.typ->ref)->prev->sym->name, "xsd__anyType",
							   12))
						{
							fprintf(fd, "  <complexType name=\"%s\">",
								ns_remove(p->sym->name));
							gen_type_documentation(fd, p, ns);
							fprintf(
								fd,
								"   <complexContent>\n    <extension base=\"%s\">\n     <sequence>\n",
								ns_convert(((Table *)p->info.typ->ref)->prev->sym->name));
							gen_schema_elements(fd, p->info.typ, ns, ns1);
							fprintf(
								fd,
								"     </sequence>\n    </extension>\n   </complexContent>\n");
							gen_schema_attributes(fd, p->info.typ, ns, ns1);
							fprintf(fd, "  </complexType>\n");
						}
						else {fprintf(fd, "  <complexType name=\"%s\">", ns_remove(p->sym->name));
						      gen_type_documentation(fd, p, ns);
						      fprintf(fd, "   <sequence>\n");
						      gen_schema_elements(fd, p->info.typ, ns, ns1);
						      fprintf(fd, "   </sequence>\n");
						      gen_schema_attributes(fd, p->info.typ, ns, ns1);
						      fprintf(fd, "  </complexType>\n"); }
					}
				}
			}
		}
	}
	fflush(fd);
	for(n = Tptr[Tarray]; n; n = n->next) {
		if(is_transient(n) || is_fixedstring(n))
			continue;
		if(1 /* wsdl */)
			fprintf(fd,
				"  <complexType name=\"%s\">\n   <complexContent>\n    <restriction base=\"%s:Array\">\n     <attribute ref=\"%s:arrayType\" WSDL:arrayType=\"%s[]\"/>\n    </restriction>\n   </complexContent>\n  </complexType>\n",
				c_ident(n), S_SoapEncNs, S_SoapEncNs, wsdl_type((Tnode *)n->ref, ns1));
		else
			fprintf(fd,
				"  <complexType name=\"%s\">\n   <complexContent>\n    <restriction base=\"%s:Array\">\n     <element name=\"item\" type=\"%s\" maxOccurs=\"unbounded\"/>\n    </restriction>\n   </complexContent>\n  </complexType>\n",
				c_ident(n), S_SoapEncNs, xsi_type((Tnode *)n->ref));
		fflush(fd);
	}
	gen_schema_elements_attributes(fd, t, ns, ns1, style, encoding);
	fprintf(fd, " </schema>\n\n");
}

void gen_schema_elements(FILE * fd, Tnode * p, char * ns, char * ns1)
{
	Entry * q;
	for(q = ((Table *)p->ref)->list; q; q = q->next)
		if(gen_schema_element(fd, p, q, ns, ns1))
			q = q->next;
}

int gen_schema_element(FILE * fd, Tnode * p, Entry * q, char * ns, char * ns1)
{
	char * s, * t;
	if(is_transient(q->info.typ) || (q->info.sto&Sattribute) || q->info.typ->type == Tfun || q->info.typ->type ==
	   Tunion)
		return 0;
	if(is_repetition(q)) {
		if(is_sequence(q->next))                       {
			fprintf(fd, "    <sequence minOccurs=\"0\" maxOccurs=\"unbounded\">\n");
			if(q->next->info.typ->ref)
				gen_schema_elements(fd, (Tnode *)q->next->info.typ->ref, ns, ns1);
			fprintf(fd, "    </sequence>\n");
			return 1;
		}
		t = ns_convert(q->next->sym->name);
		if(*t == '-')
			fprintf(
				fd,
				"     <any processContents=\"lax\" minOccurs=\"0\" maxOccurs=\"unbounded\"/><!-- %s -->\n",
				q->next->sym->name);
		else if((s =
		                 strchr(t+1,
					 ':')) &&
		        (!strchr(q->next->sym->name+1, ':') || !has_ns_eq(ns, q->next->sym->name))) {
			if(((Tnode *)q->next->info.typ->ref)->type == Tpointer)
				if(q->info.maxOccurs == 1)
					fprintf(
						fd,
						"     <element ref=\"%s\" minOccurs=\"" SOAP_LONG_FORMAT
						"\" maxOccurs=\"unbounded\"",
						t, q->info.minOccurs);
				else
					fprintf(
						fd,
						"     <element ref=\"%s\" minOccurs=\"" SOAP_LONG_FORMAT
						"\" maxOccurs=\"" SOAP_LONG_FORMAT
						"\"", t, q->info.minOccurs, q->info.maxOccurs);
			else if(q->info.maxOccurs == 1)
				fprintf(
					fd,
					"     <element ref=\"%s\" minOccurs=\"" SOAP_LONG_FORMAT
					"\" maxOccurs=\"unbounded\"",
					t, q->info.minOccurs);
			else
				fprintf(
					fd,
					"     <element ref=\"%s\" minOccurs=\"" SOAP_LONG_FORMAT
					"\" maxOccurs=\"" SOAP_LONG_FORMAT
					"\"", t, q->info.minOccurs, q->info.maxOccurs);
			if(gen_member_documentation(fd, p->id, q, ns))
				fprintf(fd, "     </element>\n");
		}
		else {const char * form = "";
		      if(!s) {
			      s = t;
			      if(*s == ':') {
				      s++;
				      form = " form=\"unqualified\"";
			      }
		      }
		      else {s++;
			    form = " form=\"qualified\""; }
		      if(((Tnode *)q->next->info.typ->ref)->type == Tpointer)
			      if(q->info.maxOccurs == 1)
				      fprintf(
					      fd,
					      "     <element name=\"%s\" type=\"%s\" minOccurs=\"" SOAP_LONG_FORMAT
					      "\" maxOccurs=\"unbounded\" nillable=\"true\"%s",
					      s, wsdl_type((Tnode *)q->next->info.typ->ref,
						      ns1), q->info.minOccurs, form);
			      else
				      fprintf(
					      fd,
					      "     <element name=\"%s\" type=\"%s\" minOccurs=\"" SOAP_LONG_FORMAT
					      "\" maxOccurs=\"" SOAP_LONG_FORMAT
					      "\" nillable=\"true\"%s", s, wsdl_type((Tnode *)q->next->info.typ->ref,
						      ns1), q->info.minOccurs, q->info.maxOccurs, form);
		      else if(q->info.maxOccurs == 1)
			      fprintf(
				      fd,
				      "     <element name=\"%s\" type=\"%s\" minOccurs=\"" SOAP_LONG_FORMAT
				      "\" maxOccurs=\"unbounded\"%s",
				      s, wsdl_type((Tnode *)q->next->info.typ->ref, ns1), q->info.minOccurs, form);
		      else
			      fprintf(
				      fd,
				      "     <element name=\"%s\" type=\"%s\" minOccurs=\"" SOAP_LONG_FORMAT
				      "\" maxOccurs=\"" SOAP_LONG_FORMAT
				      "\"%s", s, wsdl_type((Tnode *)q->next->info.typ->ref,
					      ns1), q->info.minOccurs, q->info.maxOccurs, form);
		      if(gen_member_documentation(fd, p->id, q, ns))
			      fprintf(fd, "     </element>\n"); }
		return 1;
	}
	else if(q->info.typ->type == Ttemplate ||
	        (q->info.typ->type == Tpointer &&
	         ((Tnode *)q->info.typ->ref)->type == Ttemplate) ||
	        (q->info.typ->type == Treference && ((Tnode *)q->info.typ->ref)->type == Ttemplate)) {
		t = ns_convert(q->sym->name);
		if(*t == '-')
			fprintf(
				fd,
				"     <any processContents=\"lax\" minOccurs=\"0\" maxOccurs=\"unbounded\"/><!-- %s -->\n",
				q->sym->name);
		else if((s = strchr(t+1, ':')) && (!strchr(q->sym->name+1, ':') || !has_ns_eq(ns, q->sym->name))) {
			if(((Tnode *)q->info.typ->ref)->type == Tpointer)
				if(q->info.maxOccurs == 1)
					fprintf(
						fd,
						"     <element ref=\"%s\" minOccurs=\"" SOAP_LONG_FORMAT
						"\" maxOccurs=\"unbounded\"",
						t, q->info.minOccurs);
				else
					fprintf(
						fd,
						"     <element ref=\"%s\" minOccurs=\"" SOAP_LONG_FORMAT
						"\" maxOccurs=\"" SOAP_LONG_FORMAT
						"\"", t, q->info.minOccurs, q->info.maxOccurs);
			else if(q->info.maxOccurs == 1)
				fprintf(
					fd,
					"     <element ref=\"%s\" minOccurs=\"" SOAP_LONG_FORMAT
					"\" maxOccurs=\"unbounded\"",
					t, q->info.minOccurs);
			else
				fprintf(
					fd,
					"     <element ref=\"%s\" minOccurs=\"" SOAP_LONG_FORMAT
					"\" maxOccurs=\"" SOAP_LONG_FORMAT
					"\"", t, q->info.minOccurs, q->info.maxOccurs);
			if(gen_member_documentation(fd, p->id, q, ns))
				fprintf(fd, "     </element>\n");
		}
		else {const char * form = "";
		      if(!s) {
			      s = t;
			      if(*s == ':') {
				      s++;
				      form = " form=\"unqualified\"";
			      }
		      }
		      else {s++;
			    form = " form=\"qualified\""; }
		      if(((Tnode *)q->info.typ->ref)->type == Tpointer)
			      if(q->info.maxOccurs == 1)
				      fprintf(
					      fd,
					      "     <element name=\"%s\" type=\"%s\" minOccurs=\"" SOAP_LONG_FORMAT
					      "\" maxOccurs=\"unbounded\" nillable=\"true\"%s",
					      s, wsdl_type((Tnode *)q->info.typ->ref, ns1), q->info.minOccurs, form);
			      else
				      fprintf(
					      fd,
					      "     <element name=\"%s\" type=\"%s\" minOccurs=\"" SOAP_LONG_FORMAT
					      "\" maxOccurs=\"" SOAP_LONG_FORMAT
					      "\" nillable=\"true\"%s", s, wsdl_type((Tnode *)q->info.typ->ref,
						      ns1), q->info.minOccurs, q->info.maxOccurs, form);
		      else if(q->info.maxOccurs == 1)
			      fprintf(
				      fd,
				      "     <element name=\"%s\" type=\"%s\" minOccurs=\"" SOAP_LONG_FORMAT
				      "\" maxOccurs=\"unbounded\"%s",
				      s, wsdl_type((Tnode *)q->info.typ->ref, ns1), q->info.minOccurs, form);
		      else
			      fprintf(
				      fd,
				      "     <element name=\"%s\" type=\"%s\" minOccurs=\"" SOAP_LONG_FORMAT
				      "\" maxOccurs=\"" SOAP_LONG_FORMAT
				      "\"%s", s, wsdl_type((Tnode *)q->info.typ->ref,
					      ns1), q->info.minOccurs, q->info.maxOccurs, form);
		      if(gen_member_documentation(fd, p->id, q, ns))
			      fprintf(fd, "     </element>\n"); }
	}
	else if(is_anytype(q)) { /* ... maybe need to show all possible types rather than xsd:anyType */
		fprintf(
			fd,
			"     <element name=\"%s\" type=\"xsd:anyType\" minOccurs=\"" SOAP_LONG_FORMAT
			"\" maxOccurs=\"" SOAP_LONG_FORMAT
			"\" nillable=\"true\"/>\n", ns_convert(
				q->next->sym->name), q->info.minOccurs, q->info.maxOccurs);
		return 1;
	}
	else if(is_choice(q)) {
		if(q->info.minOccurs == 0)
			fprintf(fd, "    <choice minOccurs=\"0\" maxOccurs=\"1\">\n");
		else
			fprintf(fd, "    <choice>\n");
		if(q->next->info.typ->ref)
			gen_schema_elements(fd, q->next->info.typ, ns, ns1);
		fprintf(fd, "    </choice>\n");
		return 1;
	}
	else if(is_sequence(q)) {
		if(q->info.minOccurs == 0)
			fprintf(fd, "    <sequence minOccurs=\"0\" maxOccurs=\"1\">\n");
		else
			fprintf(fd, "    <sequence>\n");
		if(q->info.typ->type == Tpointer)
			gen_schema_elements(fd, (Tnode *)q->info.typ->ref, ns, ns1);
		else if(q->info.typ->ref)
			gen_schema_elements(fd, q->info.typ, ns, ns1);
		fprintf(fd, "    </sequence>\n");
		return 0;
	}
	else {t = ns_convert(q->sym->name);
	      if(*t == '-')
		      fprintf(fd, "     <any processContents=\"lax\" minOccurs=\"0\" maxOccurs=\"1\"/><!-- %s -->\n",
			      q->sym->name);
	      else if((s = strchr(t+1, ':')) && (!strchr(q->sym->name+1, ':') || !has_ns_eq(ns, q->sym->name))) {
		      if(q->info.typ->type == Tpointer || q->info.typ->type == Tarray || is_dynamic_array(q->info.typ))
			      fprintf(
				      fd,
				      "     <element ref=\"%s\" minOccurs=\"" SOAP_LONG_FORMAT
				      "\" maxOccurs=\"" SOAP_LONG_FORMAT
				      "\"", t, q->info.minOccurs, q->info.maxOccurs);
		      else
			      fprintf(
				      fd,
				      "     <element ref=\"%s\" minOccurs=\"" SOAP_LONG_FORMAT
				      "\" maxOccurs=\"" SOAP_LONG_FORMAT
				      "\"", t, q->info.minOccurs, q->info.maxOccurs);
		      if(gen_member_documentation(fd, p->id, q, ns))
			      fprintf(fd, "     </element>\n");
	      }
	      else {const char * form = "";
		    if(!s) {
			    s = t;
			    if(*s == ':') {
				    s++;
				    form = " form=\"unqualified\"";
			    }
		    }
		    else {s++;
			  form = " form=\"qualified\""; }
		    if(q->info.typ->type == Tpointer || q->info.typ->type == Tarray || is_dynamic_array(q->info.typ))
			    fprintf(
				    fd,
				    "     <element name=\"%s\" type=\"%s\" minOccurs=\"" SOAP_LONG_FORMAT
				    "\" maxOccurs=\"" SOAP_LONG_FORMAT
				    "\" nillable=\"true\"%s%s", s, wsdl_type(q->info.typ,
					    ns1), q->info.minOccurs, q->info.maxOccurs, default_value(q,
					    "default"), form);
		    else
			    fprintf(
				    fd,
				    "     <element name=\"%s\" type=\"%s\" minOccurs=\"" SOAP_LONG_FORMAT
				    "\" maxOccurs=\"" SOAP_LONG_FORMAT
				    "\"%s%s", s, wsdl_type(q->info.typ,
					    ns1), q->info.minOccurs, q->info.maxOccurs, default_value(q,
					    "default"), form);
		    if(gen_member_documentation(fd, p->id, q, ns))
			    fprintf(fd, "     </element>\n"); }}
	fflush(fd);
	return 0;
}

void gen_schema_elements_attributes(FILE * fd, Table * t, char * ns, char * ns1, char * style, char * encoding)
{
	Entry * p, * q, * e;
	Table * r;
	Service * sp;
	Method * m;
	char * method_style, * method_encoding, * method_response_encoding;
	int all = sstreq(ns, ns1);
	r = mktable(NULL);
	for(p = classtable->list; p; p = p->next) {
		if(!p->info.typ->ref || /* is_invisible(p->info.typ->id->name) || */ is_transient(p->info.typ) ||
		   is_primclass(p->info.typ) || is_dynamic_array(p->info.typ))
			continue;
		for(q = ((Table *)p->info.typ->ref)->list; q; q = q->next) {
			if(!is_repetition(q) && !is_anytype(q) &&
			   (!strchr(q->sym->name+1,
				    ':') ||
			    !eq_ns(p->sym->name,
				    q->sym->name)) &&
			   has_ns_eq(ns,
				   q->sym->name) && !is_transient(q->info.typ) && q->info.typ->type !=
			   Tfun)                                                            {
				Service * sp2;
				Method * m;
				m = NULL;
				for(sp2 = services; sp2 && !m; sp2 = sp2->next)
					for(m = sp2->list; m; m = m->next)
						if((m->mess&FAULT) && m->part && is_eq(m->part, q->sym->name))
							break;
				if(m)
					continue;  /* already generated element for fault */
				e = entry(r, q->sym);
				if(e) {
					if((e->info.sto&Sattribute) != (q->info.sto&Sattribute) ||
					   reftype(e->info.typ) != reftype(q->info.typ))        {
						sprintf(
							errbuf,
							"Field '%s' of type '%s' at line %d has a type that does not correspond to the required unique type '%s' defined for elements '<%s>' in the WSDL namespace based on literal encoding: use SOAP RPC encoding or rename or use a namespace qualifier",
							q->sym->name, c_type(q->info.typ), q->lineno, c_type(
								e->info.typ), ns_convert(q->sym->name));
						semwarn(errbuf);
					}
				}
				else {if(q->info.sto&Sattribute)
					      fprintf(fd, "  <attribute name=\"%s\" type=\"%s\"/>\n",
						      ns_remove(q->sym->name), wsdl_type(q->info.typ, ns1));
				      else
					      fprintf(fd, "  <element name=\"%s\" type=\"%s\"/>\n",
						      ns_remove(q->sym->name), wsdl_type(q->info.typ, ns1));
				      e = enter(r, q->sym);
				      e->info = q->info; }
			}
		}
	}
	if(t && all) {
		for(p = t->list; p; p = p->next)               {
			if(p->info.typ->type == Tfun && !is_invisible(p->sym->name) && !(p->info.sto&Sextern) &&
			   has_ns_eq(ns, p->sym->name))                                                 {
				method_encoding = encoding;
				method_response_encoding = NULL;
				method_style = style;
				for(sp = services; sp; sp = sp->next) {
					if(!tagcmp(sp->ns, ns))                                        {
						for(m = sp->list; m; m =
						            m->next)
						{
							if(is_eq_nons(m->name,
								   p->sym->name))
							{
								if(m->mess == ENCODING)
									method_encoding = m->part;
								else if(m->mess == RESPONSE_ENCODING)
									method_response_encoding = m->part;
								else if(m->mess == STYLE)
									method_style = m->part;
							}
						}
					}
				}
				if(!eflag) {
					if(!method_response_encoding)
						method_response_encoding = method_encoding;
					q = entry(classtable, p->sym);
					if(q) {
						if(is_document(method_style))        {
							if(!uflag)
								fprintf(fd, "  <!-- operation request element -->\n");
							fprintf(
								fd,
								"  <element name=\"%s\">\n   <complexType>\n    <sequence>\n",
								ns_remove(p->sym->name));
							gen_schema_elements(fd, q->info.typ, ns, ns1);
							fprintf(fd, "    </sequence>\n");
							gen_schema_attributes(fd, q->info.typ, ns, ns1);
							fprintf(fd, "   </complexType>\n  </element>\n");
						}
						else if(is_literal(method_encoding)) {
							for(q = ((Table *)q->info.typ->ref)->list; q; q =
							            q->next)                                       {
								if(!is_repetition(q) && !is_anytype(q) &&
								   !has_ns_eq(NULL,
									   q->sym->name) &&
								   !is_transient(q->info.typ) &&
								   q->info.typ->type != Tfun &&
								   !(q->info.sto&
								     Sattribute))
								{
									e = entry(r, q->sym);
									if(e) {
										if((e->info.sto&Sattribute) !=
										   (q->info.sto&Sattribute)||
										   reftype(e->info.typ) !=
										   reftype(q->info.typ))        {
											sprintf(
												errbuf,
												"Parameter '%s' of type '%s' at line %d has a type that does not correspond to the required unique type '%s' defined for elements '<%s>' in the WSDL namespace based on literal encoding: use SOAP RPC encoding or rename or use a namespace qualifier",
												q->sym->name, c_type(
													q->info.typ),
												q->lineno,
												c_type(e->info.typ),
												ns_convert(q->sym->name));
											semwarn(errbuf);
										}
									}
									else {if(!uflag)
										      fprintf(
											      fd,
											      "  <!-- operation request element -->\n");
									      fprintf(
										      fd,
										      "  <element name=\"%s\" type=\"%s\"/>\n",
										      ns_remove(q->sym->name),
										      wsdl_type(q->info.typ, ns1));
									      e = enter(r, q->sym);
									      e->info = q->info; }
								}
							}
						}
						q = (Entry *)p->info.typ->ref;
						for(e = t->list; e; e = e->next)
							if(e != p && e->info.typ->type == Tfun &&
							   !(e->info.sto&Sextern) && q == (Entry *)e->info.typ->ref)
								q = NULL;
						if(q && !is_transient(q->info.typ)) {
							if(!is_response(q->info.typ))
							{
								if(is_document(method_style))
								{
									if(!uflag)
										fprintf(
											fd,
											"  <!-- operation response element -->\n");
									fprintf(
										fd,
										"  <element name=\"%sResponse\">\n   <complexType>\n",
										ns_remove(p->sym->name));
									fprintf(fd, "    <sequence>\n");
									gen_schema_element(fd, p->info.typ, q, ns, ns1);
									fprintf(fd, "    </sequence>\n");
									fprintf(fd, "   </complexType>\n  </element>\n");
								}
								else if(is_literal(method_response_encoding)) {
									e = entry(r, q->sym);
									if(e) {
										if((e->info.sto&Sattribute) !=
										   (q->info.sto&Sattribute)||
										   reftype(e->info.typ) !=
										   reftype(q->info.typ))        {
											sprintf(
												errbuf,
												"Qualified member field '%s' has a type that does not correspond to the unique type '%s' defined for elements '<%s>'",
												q->sym->name, c_type(
													q->info.typ),
												ns_convert(q->sym->name));
											semwarn(errbuf);
										}
									}
									else {if(!uflag)
										      fprintf(
											      fd,
											      "  <!-- operation response element -->\n");
									      fprintf(
										      fd,
										      "  <element name=\"%s\" type=\"%s\"/>\n",
										      ns_remove(q->sym->name),
										      wsdl_type(q->info.typ, ns1));
									      e = enter(r, q->sym);
									      e->info = q->info; }
								}
							}
							else if(((Tnode *)q->info.typ->ref)->ref) {
								if(is_document(method_style))
								{
									if(!has_ns_eq(NULL,
										   q->sym->name))
									{
										e = entry(
											r,
											((Tnode *)q->info.typ->ref)->id);
										if(!e) {
											if(!uflag)
												fprintf(
													fd,
													"  <!-- operation response element -->\n");
											fprintf(
												fd,
												"  <element name=\"%s\">\n   <complexType>\n",
												ns_remove(((Tnode *)q->
													   info.typ
													   ->ref)->id->
													name));
											fprintf(fd, "    <sequence>\n");
											gen_schema_elements(
												fd,
												(Tnode *)q->info.typ->
												ref, ns,
												ns1);
											fprintf(fd, "    </sequence>\n");
											gen_schema_attributes(
												fd,
												(Tnode *)q->info.typ->
												ref, ns,
												ns1);
											fprintf(
												fd,
												"   </complexType>\n  </element>\n");
											e =
											        enter(
													r,
													((Tnode *)q->
													 info.typ->ref)
													->id);
											e->info = q->info;
										}
									}
								}
								else if(is_literal(method_response_encoding)) {
									for(q =
									            ((Table *)((Tnode *)q->info.typ->
									                       ref)->ref)->
									            list;
									    q; q =
									            q->next)
									{
										if(!is_repetition(q) &&
										   !is_anytype(q) &&
										   !has_ns_eq(NULL,
											   q->sym->name) &&
										   !is_transient(q->info.typ) &&
										   q->info.typ->type != Tfun &&
										   !(q->info.sto&
										     Sattribute))
										{
											e = entry(r, q->sym);
											if(e) {
												if((e->info.sto&
												    Sattribute) !=
												   (q->info.sto&
												    Sattribute)||
												   reftype(e->info.typ)
												   !=
												   reftype(q->info.typ))
												{
													sprintf(
														errbuf,
														"Qualified member field '%s' has a type that does not correspond to the unique type '%s' defined for elements '<%s>'",
														q->sym
														->name,
														c_type(
															q
															->info.
															typ),
														ns_convert(
															q
															->
															sym
															->
															name));
													semwarn(errbuf);
												}
											}
											else {if(!uflag)
												      fprintf(
													      fd,
													      "  <!-- operation response element -->\n");
											      fprintf(
												      fd,
												      "  <element name=\"%s\" type=\"%s\"/>\n",
												      ns_remove(
													      q->sym->
													      name),
												      wsdl_type(q->info
													      .typ,
													      ns1));
											      e = enter(r, q->sym);
											      e->info = q->info; }
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
	if(t) {
		for(p = t->list; p; p = p->next)        {
			if(p->info.typ->type == Tfun && !(p->info.sto&Sextern) &&
			   !eflag)                                          {
				q = (Entry *)p->info.typ->ref;
				if(q && !is_transient(q->info.typ)) {
					if(is_response(q->info.typ))                                      {
						if(has_ns_eq(ns,
							   q->sym->name))
						{
							e = entry(r, q->sym);
							if(!e) {
								if(!uflag)
									fprintf(
										fd,
										"  <!-- operation response element -->\n");
								fprintf(fd, "  <element name=\"%s\" type=\"%s\"/>\n",
									ns_remove(
										q->sym->name),
									wsdl_type(q->info.typ, ns1));
								e = enter(r, q->sym);
								e->info = q->info;
							}
						}
					}
				}
			}
		}
	}
	freetable(r);
}

void gen_schema_attributes(FILE * fd, Tnode * p, char * ns, char * ns1)
{
	Entry * q;
	char * t, * s, * r;
	for(q = ((Table *)p->ref)->list; q; q = q->next) {
		if(q->info.sto&Sattribute)                                                  {
			r = default_value(q, "default");
			t = ns_convert(q->sym->name);
			if(*t == '-' || is_anyAttribute(q->info.typ))
				fprintf(fd, "     <anyAttribute processContents=\"lax\"/><!-- %s -->\n", q->sym->name);
			else if((s =
			                 strchr(t+1,
						 ':')) &&
			        (!strchr(q->sym->name+1, ':') || !has_ns_eq(ns, q->sym->name))) {
				if(r && *r)
					fprintf(fd, "     <attribute ref=\"%s\" use=\"default\"%s/>\n", t, r);
				else if(q->info.typ->type != Tpointer || q->info.minOccurs)
					fprintf(fd, "     <attribute ref=\"%s\" use=\"required\"/>\n", t);
				else if(q->info.maxOccurs == 0)
					fprintf(fd, "     <attribute ref=\"%s\" use=\"prohibited\"/>\n", t);
				else
					fprintf(fd, "     <attribute ref=\"%s\" use=\"optional\"/>\n", t);
			}
			else {const char * form = "";
			      if(!s) {
				      s = t;
				      if(*s == ':') {
					      s++;
					      form = " form=\"unqualified\"";
				      }
			      }
			      else {s++;
				    form = " form=\"qualified\""; }
			      if(r && *r)
				      fprintf(fd, "     <attribute name=\"%s\" type=\"%s\" use=\"default\"%s%s", s,
					      wsdl_type(q->info.typ,
						      ns1), r, form);
			      else if((q->info.typ->type != Tpointer && !is_stdstring(q->info.typ) &&
				       !is_stdwstring(q->info.typ)) || q->info.minOccurs)
				      fprintf(fd, "     <attribute name=\"%s\" type=\"%s\" use=\"required\"%s", s,
					      wsdl_type(q->info.typ,
						      ns1), form);
			      else if(q->info.maxOccurs == 0)
				      fprintf(fd, "     <attribute name=\"%s\" type=\"%s\" use=\"prohibited\"", s,
					      wsdl_type(q->info.typ,
						      ns1));
			      else
				      fprintf(fd, "     <attribute name=\"%s\" type=\"%s\" use=\"optional\"%s", s,
					      wsdl_type(q->info.typ,
						      ns1), form);
			      if(gen_member_documentation(fd, p->id, q, ns))
				      fprintf(fd, "     </attribute>\n"); }
			fflush(fd);
		}
	}
}

void gen_type_documentation(FILE * fd, Entry * type, char * ns)
{
	Service * sp;
	Data * d;
	if(!type->sym) {
		fprintf(fd, "\n");
		return;
	}
	for(sp = services; sp; sp = sp->next) {
		if(!tagcmp(sp->ns, ns))                                        {
			for(d = sp->data; d; d =
			            d->next)                                                                 {
				if(!strstr(d->name,
					   "::") &&
				   is_eq_nons(d->name,
					   type->sym->name))                                                                                                    {
					fprintf(
						fd,
						"\n   <annotation>\n    <documentation>%s</documentation>\n   </annotation>\n",
						d->text);
					return;
				}
			}
		}
	}
	if(!uflag)
		fprintf(fd, "<!-- %s -->\n", type->sym->name);
	fprintf(fd, "\n");
}

int gen_member_documentation(FILE * fd, Symbol * type, Entry * member, char * ns)
{
	Service * sp;
	Data * d;
	char * t;
	if(!type || !member->sym) {
		fprintf(fd, "/>\n");
		return 0;
	}
	t = ns_remove(type->name);
	for(sp = services; sp; sp = sp->next) {
		if(!tagcmp(sp->ns, ns)) {
			for(d = sp->data; d; d = d->next) {
				char * s = strstr(d->name, "::");
				if(s && !strncmp(t, d->name, s-d->name) && sstreq(s+2, member->sym->name)) {
					fprintf(fd, ">\n     <annotation>\n      <documentation>%s</documentation>\n     </annotation>\n", d->text);
					return 1;
				}
			}
		}
	}
	fprintf(fd, "/>");
	if(!uflag)
		fprintf(fd, "<!-- %s::%s -->", type->name, member->sym->name);
	fprintf(fd, "\n");
	return 0;
}

void gen_nsmap(FILE * fd, Symbol * ns, char * URI)
{
	Symbol * ns1;
	Service * sp;
	fprintf(fd, "{\n");
	for(ns1 = nslist; ns1; ns1 = ns1->next) {
		/* if(ns1 != ns) */
		for(sp = services; sp; sp = sp->next)
			if(!tagcmp(sp->ns, ns1->name) && sp->URI)
				break;
		if(sp) {
			if(sstreq(ns1->name, S_SoapEnvNs))
				fprintf(fd, "\t{\"%s\", \"%s\", \"%s\", NULL},\n", ns_convert(ns1->name), sp->URI, sp->URI2 ? sp->URI2 : envURI);
			else if(sstreq(ns1->name, S_SoapEncNs))
				fprintf(fd, "\t{\"%s\", \"%s\", \"%s\", NULL},\n", ns_convert(ns1->name), sp->URI, sp->URI2 ? sp->URI2 : encURI);
			else if(sp->URI2)
				fprintf(fd, "\t{\"%s\", \"%s\", \"%s\", NULL},\n", ns_convert(ns1->name), sp->URI, sp->URI2);
			else
				fprintf(fd, "\t{\"%s\", \"%s\", NULL, NULL},\n", ns_convert(ns1->name), sp->URI);
		}
		else if(sstreq(ns1->name, S_SoapEnvNs))
			if(is_soap12(NULL))
				fprintf(fd, "\t{\"%s\", \"%s\", \"http://schemas.xmlsoap.org/soap/envelope/\", NULL},\n", S_SoapEnvNs, envURI);
			else
				fprintf(fd, "\t{\"%s\", \"%s\", \"http://www.w3.org/*/soap-envelope\", NULL},\n", S_SoapEnvNs, envURI);
		else if(sstreq(ns1->name, S_SoapEncNs))
			if(is_soap12(NULL))
				fprintf(fd, "\t{\"%s\", \"%s\", \"http://schemas.xmlsoap.org/soap/encoding/\", NULL},\n", S_SoapEncNs, encURI);
			else
				fprintf(fd, "\t{\"%s\", \"%s\", \"http://www.w3.org/*/soap-encoding\", NULL},\n", S_SoapEncNs, encURI);
		else if(sstreq(ns1->name, "xsi"))
			fprintf(fd, "\t{\"xsi\", \"%s\", \"http://www.w3.org/*/XMLSchema-instance\", NULL},\n", xsiURI);
		else if(sstreq(ns1->name, "xsd"))
			fprintf(fd, "\t{\"xsd\", \"%s\", \"http://www.w3.org/*/XMLSchema\", NULL},\n", xsdURI);
		else
			fprintf(fd, "\t{\"%s\", \"%s/%s.xsd\", NULL, NULL},\n", ns_convert(ns1->name), tmpURI, ns_convert(ns1->name));
	}
	/* fprintf(fd, "\t{\"%s\", \"%s\"},\n", ns_convert(ns->name), URI); */
	fprintf(fd, "\t{NULL, NULL, NULL, NULL}\n};\n");
}

void gen_proxy(FILE * fd, Table * table, Symbol * ns, char * name, char * URL, char * executable, char * URI, char * encoding)
{
	Entry * p, * q, * r;
	Table * t, * output;
	Service * sp;
	int flag;
	char * name1;
	name1 = ns_cname(name, NULL);
	for(sp = services; sp; sp = sp->next)
		if(!tagcmp(sp->ns, ns->name))
			break;
	fprintf(fd, "\n\n#ifndef %s%sProxy_H\n#define %s%sProxy_H\n#include \"%sH.h\"", prefix, name1, prefix, name1, prefix);
	if(nflag)
		fprintf(fd, "\nextern SOAP_NMAC struct Namespace %s_namespaces[];", prefix);
	if(namespaceid)
		fprintf(fd, "\n\nnamespace %s {", namespaceid);
	fprintf(fd, "\nclass %s {\npublic:\n\t/// Runtime engine context allocated in constructor\n\tstruct soap *soap;\n\t/// Endpoint URL of service '%s' (change as needed)\n\tconst char *endpoint;\n\t/// Constructor allocates soap engine context, sets default endpoint URL, and sets namespace mapping table\n",
		name1, name);
	if(nflag)
		fprintf(fd, "\t%s() { soap = soap_new(); if(soap) soap->namespaces = %s_namespaces; endpoint = \"%s\"; };\n", name1, prefix, URL);
	else {
		fprintf(fd, "\t%s()\n\t{ soap = soap_new(); endpoint = \"%s\"; if(soap && !soap->namespaces) { static const struct Namespace namespaces[] = \n", name1, URL);
	      gen_nsmap(fd, ns, URI);
	      fprintf(fd, "\tsoap->namespaces = namespaces; } };\n"); 
	}
	fprintf(fd, "\t/// Destructor frees deserialized data and soap engine context\n\tvirtual ~%s() { if(soap) { soap_destroy(soap); soap_end(soap); soap_free(soap); } };\n",
		name1);
	fflush(fd);
	for(r = table->list; r; r = r->next)
		if(r->info.typ->type == Tfun && !(r->info.sto&Sextern) && has_ns_eq(ns->name, r->sym->name)) {
			p = entry(table, r->sym);
			if(p)
				q = (Entry *)p->info.typ->ref;
			else {
				fprintf(stderr, "Internal error: no table entry\n");
				return; 
			}
			p = entry(classtable, r->sym);
			if(!p) {
				fprintf(stderr, "Internal error: no parameter table entry\n");
				return;
			}
			output = (Table *)p->info.typ->ref;
			/*
			   if((s = strstr(r->sym->name, "__")))
			   s += 2;
			   else
			   s = r->sym->name;
			   fprintf(fd, "\tvirtual int %s(", s);
			 */
			fprintf(fd, "\t/// Invoke '%s' of service '%s' and return error code (or SOAP_OK)\n",
				ns_remove(r->sym->name), name);
			fprintf(fd, "\tvirtual int %s(", ident(r->sym->name));
			flag = 0;
			for(t = output; t; t = t->prev) {
				p = t->list;
				if(p) {
					fprintf(fd, "%s%s", c_storage(p->info.sto), c_type_id(p->info.typ, p->sym->name));
					for(p = p->next; p; p = p->next)
						fprintf(fd, ", %s%s", c_storage(p->info.sto), c_type_id(p->info.typ, p->sym->name));
					flag = 1;
				}
			}
			if(is_transient(q->info.typ))
				fprintf(fd, ") { return soap ? soap_send_%s(soap, endpoint, NULL", ident(r->sym->name));
			else if(flag)
				fprintf(fd, ", %s%s) { return soap ? soap_call_%s(soap, endpoint, NULL",
					c_storage(q->info.sto),
					c_type_id(q->info.typ, q->sym->name), ident(r->sym->name));
			else
				fprintf(fd, "%s%s) { return soap ? soap_call_%s(soap, endpoint, NULL",
					c_storage(q->info.sto),
					c_type_id(q->info.typ, q->sym->name), ident(r->sym->name));
			/* the action is now handled by the soap_call/soap_send operation when we pass NULL */
#if 0
			m = NULL;
			if(sp && (s = strstr(r->sym->name, "__")))
				for(m = sp->list; m; m = m->next)
					if(m->part && m->mess == ACTION && sstreq(m->name, s+2)) {
						if(*m->part == '"')
							fprintf(fd, "%s", m->part);
						else
							fprintf(fd, "\"%s\"", m->part);
						break;
					}
			if(!m)
				fprintf(fd, "NULL");
#endif
			for(t = output; t; t = t->prev)
				for(p = t->list; p; p = p->next)
					fprintf(fd, ", %s", ident(p->sym->name));
			if(is_transient(q->info.typ))
				fprintf(fd, ") : SOAP_EOM; };\n");
			else
				fprintf(fd, ", %s) : SOAP_EOM; };\n", ident(q->sym->name));
			fflush(fd);
		}
	fprintf(fd, "};");
	if(namespaceid)
		fprintf(fd, "\n\n} // namespace %s\n", namespaceid);
	fprintf(fd, "\n#endif\n");
}

void gen_object(FILE * fd, Table * table, Symbol * ns, char * name, char * URL, char * executable, char * URI, char * encoding)
{
	Entry * method;
	char * name1 = ns_cname(name, NULL);
	fprintf(fd, "\n\n#ifndef %s%sObject_H\n#define %s%sObject_H\n#include \"%sH.h\"", prefix, name1, prefix, name1, prefix);
	banner(fd, "Service Object");
	if(namespaceid)
		fprintf(fd, "\n\nnamespace %s {", namespaceid);
	fprintf(fd, "\nclass %sService : public soap {\npublic:", name1);
	fprintf(fd, "\n\t%sService()\n\t{ static const struct Namespace namespaces[] =\n", name1);
	gen_nsmap(fd, ns, URI);
	fprintf(fd, "\n\tthis->namespaces = namespaces; };");
	fprintf(fd, "\n\tvirtual ~%sService() { };", name1);
	fprintf(fd, "\n\t/// Bind service to port (returns master socket or SOAP_INVALID_SOCKET)");
	fprintf(fd, "\n\tvirtual\tSOAP_SOCKET bind(const char *host, int port, int backlog) { return soap_bind(this, host, port, backlog); };");
	fprintf(fd, "\n\t/// Accept next request (returns socket or SOAP_INVALID_SOCKET)");
	fprintf(fd, "\n\tvirtual\tSOAP_SOCKET accept() { return soap_accept(this); };");
	fprintf(fd, "\n\t/// Serve this request (returns error code or SOAP_OK)");
	if(nflag)
		fprintf(fd, "\n\tvirtual\tint serve() { return %s_serve(this); };", prefix);
	else
		fprintf(fd, "\n\tvirtual\tint serve() { return soap_serve(this); };");
	fprintf(fd, "\n};");
	banner(fd, "Service Operations (you should define these globally)");
	for(method = table->list; method; method = method->next) {
		if(method->info.typ->type == Tfun && !(method->info.sto&Sextern))                                                           {
			Entry * p, * q = entry(table, method->sym);
			Table * output;
			if(q)
				p = (Entry *)q->info.typ->ref;
			else {
				fprintf(stderr, "Internal error: no table entry\n");
				return; 
			}
			q = entry(classtable, method->sym);
			output = (Table *)q->info.typ->ref;
			fprintf(fd, "\n\nSOAP_FMAC5 int SOAP_FMAC6 %s(struct soap*", ident(method->sym->name));
			gen_params(fd, output, p, 1);
			fprintf(fd, ";");
		}
	}
	if(namespaceid)
		fprintf(fd, "\n\n} // namespace %s\n", namespaceid);
	fprintf(fd, "\n\n#endif\n");
}

void gen_proxy_header(FILE * fd, Table * table, Symbol * ns, char * name, char * URL, char * executable, char * URI, char * encoding)
{
	Entry * p, * method;
	Table * t;
	fprintf(fd, "\n\n#ifndef %s%s_H\n#define %s%s_H\n#include \"%sH.h\"", prefix, name, prefix, name, prefix);
	if(namespaceid)
		fprintf(fd, "\n\nnamespace %s {", namespaceid);
	if(iflag)
		fprintf(fd, "\n\nclass SOAP_CMAC %s : public soap\n{ public:", name);
	else
		fprintf(fd, "\n\nclass SOAP_CMAC %s\n{ public:", name);
	if(!iflag)
		fprintf(fd, "\n\tstruct soap *soap;\n\tbool own;");
	//fprintf(fd, "\n\t/// Endpoint URL of service '%s' (change as needed)", name);
	//fprintf(fd, "\n\tconst char *soap_endpoint;");
	fprintf(fd, "\n\tconst char *soap_endpoint;"); fprintf(fd, " // Endpoint URL of service '%s' (change as needed)", name);
	//fprintf(fd, "\n\t/// Constructor");
	//fprintf(fd, "\n\t%s();", name);
	fprintf(fd, "\n\t%s();", name); fprintf(fd, " // Constructor");
	if(iflag) {
		//fprintf(fd, "\n\t/// Construct from another engine state");
		//fprintf(fd, "\n\t%s(const struct soap&);", name);
		fprintf(fd, "\n\t%s(const struct soap&);", name); fprintf(fd, " // Construct from another engine state");
	}
	else {
		//fprintf(fd, "\n\t/// Constructor to use/share an engine state");
		//fprintf(fd, "\n\t%s(struct soap*);", name); 
		fprintf(fd, "\n\t%s(struct soap*);", name); fprintf(fd, " // Constructor to use/share an engine state");
	}
	fprintf(fd, "\n\t%s(const char *url);", name); fprintf(fd, " // Constructor with endpoint URL");
	fprintf(fd, "\n\t%s(soap_mode iomode);", name); fprintf(fd, " // Constructor with engine input+output mode control");
	fprintf(fd, "\n\t%s(const char *url, soap_mode iomode);", name); fprintf(fd, " // Constructor with URL and input+output mode control");
	fprintf(fd, "\n\t%s(soap_mode imode, soap_mode omode);", name); fprintf(fd, " // Constructor with engine input and output mode control");
	fprintf(fd, "\n\tvirtual\t~%s();", name); fprintf(fd, " // Destructor frees deserialized data");
	fprintf(fd, "\n\tvirtual\tvoid %s_init(soap_mode imode, soap_mode omode);", name); fprintf(fd, " // Initializer used by constructors");
	fprintf(fd, "\n\tvirtual\tvoid destroy();"); fprintf(fd, " // Delete all deserialized data (uses soap_destroy and soap_end)");
	fprintf(fd, "\n\tvirtual\tvoid reset();"); fprintf(fd, " // Delete all deserialized data and reset to default");
	fprintf(fd, "\n\tvirtual\tvoid soap_noheader();"); fprintf(fd, " // Disables and removes SOAP Header from message");
	if(!namespaceid) {
		p = entry(classtable, lookup("SOAP_ENV__Header"));
		if(p) {
			t = (Table *)p->info.typ->ref;
			if(t && t->list && !is_void(t->list->info.typ)) {
				fprintf(fd, "\n\t/// Put SOAP Header in message");
				fprintf(fd, "\n\tvirtual\tvoid soap_header(");
				gen_params(fd, t, NULL, 0);
				fprintf(fd, ";");
			}
		}
	}
	fprintf(fd, "\n\tvirtual\tconst SOAP_ENV__Header *soap_header();"); fprintf(fd, " // Get SOAP Header structure (NULL when absent)");
	fprintf(fd, "\n\tvirtual\tconst SOAP_ENV__Fault *soap_fault();"); fprintf(fd, " // Get SOAP Fault structure (NULL when absent)");
	fprintf(fd, "\n\tvirtual\tconst char *soap_fault_string();"); fprintf(fd, " // Get SOAP Fault string (NULL when absent)");
	fprintf(fd, "\n\tvirtual\tconst char *soap_fault_detail();"); fprintf(fd, " // Get SOAP Fault detail as string (NULL when absent)");
	fprintf(fd, "\n\tvirtual\tint soap_close_socket();"); fprintf(fd, " // Close connection (normally automatic, except for send_X ops)");
	fprintf(fd, "\n\tvirtual\tint soap_force_close_socket();"); fprintf(fd, " // Force close connection (can kill a thread blocked on IO)");
	fprintf(fd, "\n\tvirtual\tvoid soap_print_fault(FILE*);"); fprintf(fd, " // Print fault");
	fprintf(fd, "\n#ifndef WITH_LEAN\n\t/// Print fault to stream");
	fprintf(fd, "\n#ifndef WITH_COMPAT");
	fprintf(fd, "\n\tvirtual\tvoid soap_stream_fault(std::ostream&);");
	fprintf(fd, "\n#endif\n");
	fprintf(fd, "\n\t/// Put fault into buffer");
	fprintf(fd, "\n\tvirtual\tchar *soap_sprint_fault(char *buf, size_t len);\n#endif");
	for(method = table->list; method; method = method->next)
		if(method->info.typ->type == Tfun && !(method->info.sto&Sextern) && has_ns_eq(ns->name, method->sym->name))
			gen_method(fd, table, method, 0);
	fprintf(fd, "\n};");
	if(namespaceid)
		fprintf(fd, "\n\n} // namespace %s\n", namespaceid);
	fprintf(fd, "\n#endif\n");
}

void gen_proxy_code(FILE * fd, Table * table, Symbol * ns, char * name, char * URL, char * executable, char * URI, char * encoding)
{
	Entry * p, * method, * param;
	Table * t;
	char * soap = iflag ? "this" : "this->soap";
	fprintf(fd, "\n\n#include \"%s%s.h\"", prefix, name);
	if(namespaceid)
		fprintf(fd, "\n\nnamespace %s {", namespaceid);
	if(iflag) {
		fprintf(fd, "\n\n%s::%s()\n{\n\t%s_init(SOAP_IO_DEFAULT, SOAP_IO_DEFAULT);\n}", name, name, name);
		fprintf(fd, "\n\n%s::%s(const struct soap &_soap) : soap(_soap)\n{ }", name, name);
		fprintf(fd, "\n\n%s::%s(const char *url)\n{\n\t%s_init(SOAP_IO_DEFAULT, SOAP_IO_DEFAULT);\n\tsoap_endpoint = url;\n}", name, name, name);
		fprintf(fd, "\n\n%s::%s(soap_mode iomode)\n{\n\t%s_init(iomode, iomode);\n}", name, name, name);
		fprintf(fd, "\n\n%s::%s(const char *url, soap_mode iomode)\n{\n\t%s_init(iomode, iomode);\n\tsoap_endpoint = url;\n}", name, name, name);
		fprintf(fd, "\n\n%s::%s(soap_mode imode, soap_mode omode)\n{\n\t%s_init(imode, omode);\n}", name, name, name);
		fprintf(fd, "\n\n%s::~%s()\n{ }", name, name);
	}
	else {
		fprintf(fd, "\n\n%s::%s()\n{\n\tthis->soap = soap_new();\n\tthis->own = true;\n\t%s_init(SOAP_IO_DEFAULT, SOAP_IO_DEFAULT);\n}", name, name, name);
		fprintf(fd, "\n\n%s::%s(struct soap *_soap)\n{\n\tthis->soap = _soap;\n\tthis->own = false;\n\t%s_init(_soap->imode, _soap->omode);\n}", name, name, name);
		fprintf(fd, "\n\n%s::%s(const char *url)\n{\n\tthis->soap = soap_new();\n\tthis->own = true;\n\t%s_init(SOAP_IO_DEFAULT, SOAP_IO_DEFAULT);\n\tsoap_endpoint = url;\n}", name, name, name);
		fprintf(fd, "\n\n%s::%s(soap_mode iomode)\n{\n\tthis->soap = soap_new();\n\tthis->own = true;\n\t%s_init(iomode, iomode);\n}", name, name, name);
		fprintf(fd, "\n\n%s::%s(const char *url, soap_mode iomode)\n{\n\tthis->soap = soap_new();\n\tthis->own = true;\n\t%s_init(iomode, iomode);\n\tsoap_endpoint = url;\n}", name, name, name);
		fprintf(fd, "\n\n%s::%s(soap_mode imode, soap_mode omode)\n{\n\tthis->soap = soap_new();\n\tthis->own = true;\n\t%s_init(imode, omode);\n}", name, name, name);
		fprintf(fd, "\n\n%s::~%s()\n{\n\tif(this->own)\n\t\tsoap_free(this->soap);\n}", name, name); 
	}
	fprintf(fd, "\n\nvoid %s::%s_init(soap_mode imode, soap_mode omode)\n{\n\tsoap_imode(%s, imode);\n\tsoap_omode(%s, omode);\n\tsoap_endpoint = NULL;\n\tstatic const struct Namespace namespaces[] =\n", name, name, soap, soap);
	gen_nsmap(fd, ns, URI);
	fprintf(fd, "\tsoap_set_namespaces(%s, namespaces);\n}", soap);
	fprintf(fd, "\n\nvoid %s::destroy()\n{\n\tsoap_destroy(%s);\n\tsoap_end(%s);\n}", name, soap, soap);
	fprintf(fd, "\n\nvoid %s::reset()\n{\n\tdestroy();\n\tsoap_done(%s);\n\tsoap_init(%s);\n\t%s_init(SOAP_IO_DEFAULT, SOAP_IO_DEFAULT);\n}", name, soap, soap, name);
	fprintf(fd, "\n\nvoid %s::soap_noheader()\n{\n\t%s->header = NULL;\n}", name, soap);
	if(!namespaceid) {
		p = entry(classtable, lookup("SOAP_ENV__Header"));
		if(p) {
			t = (Table *)p->info.typ->ref;
			if(t && t->list && !is_void(t->list->info.typ)) {
				fprintf(fd, "\n\nvoid %s::soap_header(", name);
				gen_params(fd, t, NULL, 0);
				fprintf(fd, "\n{\n\t::soap_header(%s);", soap);
				for(param = t->list; param; param = param->next) {
					if(namespaceid)
						fprintf(fd, "\n\t((%s::SOAP_ENV__Header*)%s->header)->%s = %s;",
							namespaceid, soap, ident(param->sym->name), ident(param->sym->name));
					else
						fprintf(fd, "\n\t%s->header->%s = %s;", soap, ident(param->sym->name), ident(param->sym->name));
				}
				fprintf(fd, "\n}");
			}
		}
	}
	fprintf(fd, "\n\nconst SOAP_ENV__Header *%s::soap_header()\n{\n\treturn %s->header;\n}", name, soap);
	fprintf(fd, "\n\nconst SOAP_ENV__Fault *%s::soap_fault()\n{\n\treturn %s->fault;\n}", name, soap);
	fprintf(fd, "\n\nconst char *%s::soap_fault_string()\n{\n\treturn *soap_faultstring(%s);\n}", name, soap);
	fprintf(fd, "\n\nconst char *%s::soap_fault_detail()\n{\n\treturn *soap_faultdetail(%s);\n}", name, soap);
	fprintf(fd, "\n\nint %s::soap_close_socket()\n{\n\treturn soap_closesock(%s);\n}", name, soap);
	fprintf(fd, "\n\nint %s::soap_force_close_socket()\n{\n\treturn soap_force_closesock(%s);\n}", name, soap);
	fprintf(fd, "\n\nvoid %s::soap_print_fault(FILE *fd)\n{\n\t::soap_print_fault(%s, fd);\n}", name, soap);
	fprintf(fd, "\n\n#ifndef WITH_LEAN\n#ifndef WITH_COMPAT\nvoid %s::soap_stream_fault(std::ostream& os)\n{\n\t::soap_stream_fault(%s, os);\n}\n#endif", name, soap);
	fprintf(
		fd,
		"\n\nchar *%s::soap_sprint_fault(char *buf, size_t len)\n{\n\treturn ::soap_sprint_fault(%s, buf, len);\n}\n#endif",
		name, soap);
	for(method = table->list; method; method = method->next)
		if(method->info.typ->type == Tfun && !(method->info.sto&Sextern) && !is_imported(method->info.typ) &&
		   has_ns_eq(ns->name, method->sym->name))
			gen_call_method(fd, table, method, name);
	if(namespaceid)
		fprintf(fd, "\n\n} // namespace %s\n", namespaceid);
	fprintf(fd, "\n/* End of client proxy code */\n");
}

void gen_object_header(FILE * fd, Table * table, Symbol * ns, char * name, char * URL, char * executable, char * URI, char * encoding)
{
	Entry * p, * method;
	Table * t;
	fprintf(fd, "\n\n#ifndef %s%s_H\n#define %s%s_H\n#include \"%sH.h\"", prefix, name, prefix, name, prefix);
	if(namespaceid)
		fprintf(fd, "\n\nnamespace %s {", namespaceid);
	if(iflag)
		fprintf(fd, "\nclass SOAP_CMAC %s : public soap\n{ public:", name);
	else {
		fprintf(fd, "\nclass SOAP_CMAC %s\n{ public:", name);
		fprintf(fd, "\n\tstruct soap *soap;\n\tbool own;"); 
	}
	fprintf(fd, "\n\t/// Constructor");
	fprintf(fd, "\n\t%s();", name);
	if(iflag) {
		fprintf(fd, "\n\t/// Construct from another engine state");
		fprintf(fd, "\n\t%s(const struct soap&);", name);
	}
	else {
		fprintf(fd, "\n\t/// Constructor to use/share an engine state");
		fprintf(fd, "\n\t%s(struct soap*);", name); 
	}
	fprintf(fd, "\n\t/// Constructor with engine input+output mode control");
	fprintf(fd, "\n\t%s(soap_mode iomode);", name);
	fprintf(fd, "\n\t/// Constructor with engine input and output mode control");
	fprintf(fd, "\n\t%s(soap_mode imode, soap_mode omode);", name);
	fprintf(fd, "\n\t/// Destructor, also frees all deserialized data");
	fprintf(fd, "\n\tvirtual ~%s();", name);
	fprintf(fd, "\n\t/// Delete all deserialized data (uses soap_destroy and soap_end)");
	fprintf(fd, "\n\tvirtual\tvoid destroy();");
	fprintf(fd, "\n\t/// Delete all deserialized data and reset to defaults");
	fprintf(fd, "\n\tvirtual\tvoid reset();");
	fprintf(fd, "\n\t/// Initializer used by constructor");
	fprintf(fd, "\n\tvirtual\tvoid %s_init(soap_mode imode, soap_mode omode);", name);
	fprintf(fd, "\n\t/// Create a copy");
	fprintf(fd, "\n\tvirtual\t%s *copy() SOAP_PURE_VIRTUAL;", name);
	fprintf(fd, "\n\t/// Close connection (normally automatic)");
	fprintf(fd, "\n\tvirtual\tint soap_close_socket();");
	fprintf(fd, "\n\t/// Force close connection (can kill a thread blocked on IO)");
	fprintf(fd, "\n\tvirtual\tint soap_force_close_socket();");
	fprintf(fd, "\n\t/// Return sender-related fault to sender");
	fprintf(fd, "\n\tvirtual\tint soap_senderfault(const char *string, const char *detailXML);");
	fprintf(fd, "\n\t/// Return sender-related fault with SOAP 1.2 subcode to sender");
	fprintf(fd, "\n\tvirtual\tint soap_senderfault(const char *subcodeQName, const char *string, const char *detailXML);");
	fprintf(fd, "\n\t/// Return receiver-related fault to sender");
	fprintf(fd, "\n\tvirtual\tint soap_receiverfault(const char *string, const char *detailXML);");
	fprintf(fd, "\n\t/// Return receiver-related fault with SOAP 1.2 subcode to sender");
	fprintf(fd, "\n\tvirtual\tint soap_receiverfault(const char *subcodeQName, const char *string, const char *detailXML);");
	fprintf(fd, "\n\t/// Print fault");
	fprintf(fd, "\n\tvirtual\tvoid soap_print_fault(FILE*);");
	fprintf(fd, "\n#ifndef WITH_LEAN\n\t/// Print fault to stream");
	fprintf(fd, "\n#ifndef WITH_COMPAT");
	fprintf(fd, "\n\tvirtual\tvoid soap_stream_fault(std::ostream&);");
	fprintf(fd, "\n#endif");
	fprintf(fd, "\n\t/// Put fault into buffer");
	fprintf(fd, "\n\tvirtual\tchar *soap_sprint_fault(char *buf, size_t len);\n#endif");
	fprintf(fd, "\n\t/// Disables and removes SOAP Header from message");
	fprintf(fd, "\n\tvirtual\tvoid soap_noheader();");
	if(!namespaceid) {
		p = entry(classtable, lookup("SOAP_ENV__Header"));
		if(p) {
			t = (Table *)p->info.typ->ref;
			if(t && t->list && !is_void(t->list->info.typ)) {
				fprintf(fd, "\n\t/// Put SOAP Header in message");
				fprintf(fd, "\n\tvirtual\tvoid soap_header(");
				gen_params(fd, t, NULL, 0);
				fprintf(fd, ";");
			}
		}
	}
	fprintf(fd, "\n\t/// Get SOAP Header structure (NULL when absent)");
	fprintf(fd, "\n\tvirtual\tconst SOAP_ENV__Header *soap_header();");
	fprintf(fd, "\n\t/// Run simple single-thread iterative service on port until a connection error occurs (returns error code or SOAP_OK), use this->bind_flag = SO_REUSEADDR to rebind for a rerun");
	fprintf(fd, "\n\tvirtual\tint run(int port);");
	fprintf(fd, "\n\t/// Bind service to port (returns master socket or SOAP_INVALID_SOCKET)");
	fprintf(fd, "\n\tvirtual\tSOAP_SOCKET bind(const char *host, int port, int backlog);");
	fprintf(fd, "\n\t/// Accept next request (returns socket or SOAP_INVALID_SOCKET)");
	fprintf(fd, "\n\tvirtual\tSOAP_SOCKET accept();");
	fprintf(fd, "\n\t/// Serve this request (returns error code or SOAP_OK)");
	fprintf(fd, "\n\tvirtual\tint serve();");
	fprintf(fd, "\n\t/// Used by serve() to dispatch a request (returns error code or SOAP_OK)");
	fprintf(fd, "\n\tvirtual\tint dispatch();");
	fprintf(fd, "\n\n\t///\n\t/// Service operations (you should define these):\n\t/// Note: compile with -DWITH_PURE_VIRTUAL for pure virtual methods\n\t///");
	for(method = table->list; method; method = method->next)
		if(method->info.typ->type == Tfun && !(method->info.sto&Sextern) && has_ns_eq(ns->name, method->sym->name))
			gen_method(fd, table, method, 1);
	fprintf(fd, "\n};");
	if(namespaceid)
		fprintf(fd, "\n\n} // namespace %s\n", namespaceid);
	fprintf(fd, "\n#endif\n");
}

void gen_method(FILE * fd, Table * table, Entry * method, int server)
{
	Table * params;
	char * soap = iflag ? "this" : "this->soap";
	Entry * result = (Entry *)method->info.typ->ref;
	Entry * p = entry(classtable, method->sym);
	if(!p)
		execerror("no table entry");
	params = (Table *)p->info.typ->ref;
	if(server || !is_transient(result->info.typ)) {
		if(is_transient(result->info.typ))
			fprintf(fd, "\n\n\t/// Web service one-way operation '%s' (return error code, SOAP_OK (no response), or send_%s_empty_response())",
				ns_remove(method->sym->name), ns_remove(method->sym->name));
		else
			fprintf(fd, "\n\n\t/// Web service operation '%s' (returns error code or SOAP_OK)",
				ns_remove(method->sym->name));
		fprintf(fd, "\n\tvirtual\tint %s(", ns_cname(method->sym->name, NULL));
		gen_params(fd, params, result, 0);
		if(!server) {
			fprintf(fd, " { return %s(NULL, NULL", ns_cname(method->sym->name, NULL));
			gen_args(fd, params, result, 1);
			fprintf(fd, "; }");
			fprintf(fd, "\n\tvirtual\tint %s(const char *endpoint, const char *soap_action", ns_cname(method->sym->name, NULL));
			gen_params(fd, params, result, 1);
		}
		if(server)
			fprintf(fd, " SOAP_PURE_VIRTUAL;");
		else
			fprintf(fd, ";");
		if(is_transient(result->info.typ))
			fprintf(fd, "\n\tvirtual\tint send_%s_empty_response(int httpcode) { return soap_send_empty_response(%s, httpcode); }",
				ns_cname(method->sym->name, NULL), soap);
	}
	else {
		fprintf(fd, "\n\n\t/// Web service one-way send operation 'send_%s' (returns error code or SOAP_OK)", ns_remove(method->sym->name));
	      fprintf(fd, "\n\tvirtual\tint send_%s(", ns_cname(method->sym->name, NULL));
	      gen_params(fd, params, result, 0);
	      fprintf(fd, " { return send_%s(NULL, NULL", ns_cname(method->sym->name, NULL));
	      gen_args(fd, params, result, 1);
	      fprintf(fd, "; }");
	      fprintf(fd, "\n\tvirtual\tint send_%s(const char *endpoint, const char *soap_action", ns_cname(method->sym->name, NULL));
	      gen_params(fd, params, result, 1);
	      fprintf(fd, ";\n\t/// Web service one-way receive operation 'recv_%s' (returns error code or SOAP_OK)", ns_remove(method->sym->name));
	      fprintf(fd, ";\n\tvirtual\tint recv_%s(", ns_cname(method->sym->name, NULL));
	      fprintf(fd, "struct %s&);", ident(method->sym->name));
	      fprintf(fd, "\n\t/// Web service receive of HTTP Accept acknowledgment for one-way send operation 'send_%s' (returns error code or SOAP_OK)", ns_remove(method->sym->name));
	      fprintf(fd, "\n\tvirtual\tint recv_%s_empty_response() { return soap_recv_empty_response(%s); }", ns_cname(method->sym->name, NULL), soap);
	      fprintf(fd, "\n\t/// Web service one-way synchronous send operation '%s' with HTTP Accept/OK response receive (returns error code or SOAP_OK)", ns_remove(method->sym->name));
	      fprintf(fd, "\n\tvirtual\tint %s(", ns_cname(method->sym->name, NULL));
	      gen_params(fd, params, result, 0);
	      fprintf(fd, " { return %s(NULL, NULL", ns_cname(method->sym->name, NULL));
	      gen_args(fd, params, result, 1);
	      fprintf(fd, "; }");
	      fprintf(fd, "\n\tvirtual\tint %s(const char *endpoint, const char *soap_action", ns_cname(method->sym->name, NULL));
	      gen_params(fd, params, result, 1);
	      fprintf(fd, " { if(send_%s(endpoint, soap_action", ns_cname(method->sym->name, NULL));
	      gen_args(fd, params, result, 1);
	      fprintf(fd, " || soap_recv_empty_response(%s)) return %s->error; return SOAP_OK; }", soap, soap); 
	}
}

void gen_params(FILE * fd, Table * params, Entry * result, int flag)
{
	Entry * param;
	for(param = params->list; param; param = param->next)
		fprintf(fd, "%s%s%s", flag || param != params->list ? ", " : "", c_storage(param->info.sto), c_type_id(param->info.typ, param->sym->name));
	if(!result || is_transient(result->info.typ))
		fprintf(fd, ")");
	else
		fprintf(fd, "%s%s%s)", flag || params->list ? ", " : "", c_storage(result->info.sto), c_type_id(result->info.typ, result->sym->name));
}

void gen_args(FILE * fd, Table * params, Entry * result, int flag)
{
	Entry * param;
	for(param = params->list; param; param = param->next)
		fprintf(fd, "%s%s", flag || param != params->list ? ", " : "", param->sym->name);
	if(!result || is_transient(result->info.typ))
		fprintf(fd, ")");
	else
		fprintf(fd, "%s%s)", flag || params->list ? ", " : "", result->sym->name);
}

void gen_call_method(FILE * fd, Table * table, Entry * method, char * name)
{
	Service * sp;
	Method * m;
	const char * style, * encoding;
	const char * xtag, * xtyp;
	const char * action = NULL, * method_encoding = NULL, * method_response_encoding = NULL;
	Table * params;
	Entry * param, * result, * p, * response = NULL;
	result = (Entry *)method->info.typ->ref;
	p = entry(classtable, method->sym);
	if(!p)
		execerror("no table entry");
	params = (Table *)p->info.typ->ref;
	if(!is_response(result->info.typ) && !is_XML(result->info.typ))
		response = get_response(method->info.typ);
	if(name) {
		if(!is_transient(result->info.typ))
			fprintf(fd, "\n\nint %s::%s(const char *endpoint, const char *soap_action", name,
				ns_cname(method->sym->name, NULL));
		else
			fprintf(fd, "\n\nint %s::send_%s(const char *endpoint, const char *soap_action", name,
				ns_cname(method->sym->name, NULL));
		gen_params(fd, params, result, 1);
	}
	else if(!is_transient(result->info.typ)) {
		fprintf(fheader, "\n\nSOAP_FMAC5 int SOAP_FMAC6 soap_call_%s(struct soap *soap, const char *soap_endpoint, const char *soap_action", ident(method->sym->name));
		gen_params(fheader, params, result, 1);
		fprintf(fd, "\n\nSOAP_FMAC5 int SOAP_FMAC6 soap_call_%s(struct soap *soap, const char *soap_endpoint, const char *soap_action", ident(method->sym->name));
		gen_params(fd, params, result, 1);
	}
	else {
		fprintf(fheader, "\n\nSOAP_FMAC5 int SOAP_FMAC6 soap_send_%s(struct soap *soap, const char *soap_endpoint, const char *soap_action", ident(method->sym->name));
		gen_params(fheader, params, result, 1);
		fprintf(fd, "\n\nSOAP_FMAC5 int SOAP_FMAC6 soap_send_%s(struct soap *soap, const char *soap_endpoint, const char *soap_action", ident(method->sym->name));
		gen_params(fd, params, result, 1); 
	}
	if(name) {
		if(iflag)
			fprintf(fd, "\n{\n\tstruct soap *soap = this;\n");
		else
			fprintf(fd, "\n{\n\tstruct soap *soap = this->soap;\n");
	}
	else {
		fprintf(fheader, ";");
		fprintf(fd, "\n{"); 
	}
	fprintf(fd, "\tstruct %s soap_tmp_%s;", ident(method->sym->name), ident(method->sym->name));
	if(response)
		fprintf(fd, "\n\tstruct %s *soap_tmp_%s;", c_ident(response->info.typ), c_ident(response->info.typ));
	for(sp = services; sp; sp = sp->next) {
		if(has_ns_eq(sp->ns, method->sym->name)) {
			style = sp->style;
			encoding = sp->encoding;
			method_encoding = encoding;
			method_response_encoding = NULL;
			for(m = sp->list; m; m = m->next) {
				if(is_eq_nons(m->name, method->sym->name)) {
					if(m->mess == ACTION || m->mess == REQUEST_ACTION)
						action = m->part;
					else if(m->mess == ENCODING)
						method_encoding = m->part;
					else if(m->mess == RESPONSE_ENCODING)
						method_response_encoding = m->part;
				}
			}
			break;
		}
	}
	if(name)
		fprintf(fd, "\n\tif(endpoint)\n\t\tsoap_endpoint = endpoint;");
	if(sp && sp->URL) {
		// @v9.8.8 fprintf(fd, "\n\tif(!soap_endpoint)\n\t\tsoap_endpoint = \"%s\";", sp->URL);
		fprintf(fd, "\n\tSETIFZ(soap_endpoint, \"%s\");", sp->URL); // @v9.8.8
	}
	if(action) {
		// @v9.8.8 fprintf(fd, "\n\tif(!soap_action)\n\t\tsoap_action = ");
		fprintf(fd, "\n\tSETIFZ(soap_action, "); // @v9.8.8
		if(*action == '"')
			fprintf(fd, "%s);", action); // @v9.8.8 ;"-->);"
		else
			fprintf(fd, "\"%s\");", action); // @v9.8.8 ;"-->);"
	}
	if(!method_response_encoding)
		method_response_encoding = method_encoding;
	if(sp && sp->URI && method_encoding) {
		if(is_literal(method_encoding))
			fprintf(fd, "\n\tsoap->encodingStyle = NULL;");
		else if(method_encoding)
			fprintf(fd, "\n\tsoap->encodingStyle = \"%s\";", method_encoding);
	}
	else if(!eflag)
		fprintf(fd, "\n\tsoap->encodingStyle = NULL;");
	for(param = params->list; param; param = param->next) {
		if(param->info.typ->type == Tarray)
			fprintf(fd, "\n\tmemcpy(soap_tmp_%s.%s, %s, sizeof(%s));", ident(method->sym->name),
				ident(param->sym->name), ident(param->sym->name), c_type(param->info.typ));
		else
			fprintf(fd, "\n\tsoap_tmp_%s.%s = %s;", ident(method->sym->name), ident(
					param->sym->name), ident(param->sym->name));
	}
	fprintf(fd, "\n\tsoap_begin(soap);");
	fprintf(fd, "\n\tsoap_serializeheader(soap);");
	fprintf(fd, "\n\tsoap_serialize_%s(soap, &soap_tmp_%s);", ident(method->sym->name), ident(method->sym->name));
	fprintf(fd, "\n\tif(soap_begin_count(soap))\n\t\treturn soap->error;");
	fprintf(fd, "\n\tif(soap->mode & SOAP_IO_LENGTH)");
	fprintf(fd, " {\n\t\tif(soap_envelope_begin_out(soap)");
	fprintf(fd, "\n\t\t || soap_putheader(soap)");
	fprintf(fd, "\n\t\t || soap_body_begin_out(soap)");
	fprintf(fd, "\n\t\t || soap_put_%s(soap, &soap_tmp_%s, \"%s\", NULL)", ident(method->sym->name), ident(method->sym->name), ns_convert(method->sym->name));
	fprintf(fd, "\n\t\t || soap_body_end_out(soap)");
	fprintf(fd, "\n\t\t || soap_envelope_end_out(soap))");
	fprintf(fd, "\n\t\t\t return soap->error;");
	fprintf(fd, "\n\t}");
	fprintf(fd, "\n\tif(soap_end_count(soap))\n\t\treturn soap->error;");
	fprintf(fd, "\n\tif(soap_connect(soap, soap_endpoint, soap_action)");
	fprintf(fd, "\n\t || soap_envelope_begin_out(soap)");
	fprintf(fd, "\n\t || soap_putheader(soap)");
	fprintf(fd, "\n\t || soap_body_begin_out(soap)");
	fprintf(fd, "\n\t || soap_put_%s(soap, &soap_tmp_%s, \"%s\", NULL)", ident(method->sym->name), ident(method->sym->name), ns_convert(method->sym->name));
	fprintf(fd, "\n\t || soap_body_end_out(soap)");
	fprintf(fd, "\n\t || soap_envelope_end_out(soap)");
	fprintf(fd, "\n\t || soap_end_send(soap))");
	fprintf(fd, "\n\t\treturn soap_closesock(soap);");
	if(is_transient(result->info.typ)) {
		fprintf(fd, "\n\treturn SOAP_OK;\n}");
		if(name) {
			fprintf(fd, "\n\nint %s::recv_%s(", name, ns_cname(method->sym->name, NULL));
			fprintf(fd, "struct %s& tmp)", ident(method->sym->name));
			if(iflag)
				fprintf(fd, "\n{\n\tstruct soap *soap = this;\n");
			else
				fprintf(fd, "\n{\n\tstruct soap *soap = this->soap;\n");
			fprintf(fd, "\n\tstruct %s *%s = &tmp;", ident(method->sym->name), ident(result->sym->name));
		}
		else {fprintf(fheader, "\n\nSOAP_FMAC5 int SOAP_FMAC6 soap_recv_%s(struct soap *soap, ",
			      ident(method->sym->name));
		      fprintf(fd, "\n\nSOAP_FMAC5 int SOAP_FMAC6 soap_recv_%s(struct soap *soap, ",
			      ident(method->sym->name));
		      fprintf(fheader, "struct %s *%s);\n", ident(method->sym->name), ident(result->sym->name));
		      fprintf(fd, "struct %s *%s)\n{", ident(method->sym->name), ident(result->sym->name)); }
		fprintf(fd, "\n\tsoap_default_%s(soap, %s);", ident(method->sym->name), ident(result->sym->name));
		fprintf(fd, "\n\tsoap_begin(soap);");
	}
	else if(result->info.typ->type == Tarray)
		fprintf(fd, "\n\tsoap_default_%s(soap, %s);", c_ident(result->info.typ), ident(result->sym->name));
	else if(result->info.typ->type == Treference && ((Tnode *)result->info.typ->ref)->type == Tclass &&
	        !is_external((Tnode *)result->info.typ->ref) && !is_volatile((Tnode *)result->info.typ->ref))
		fprintf(fd, "\n\tif(!&%s)\n\t\treturn soap_closesock(soap);\n\t%s.soap_default(soap);",
			ident(result->sym->name), ident(result->sym->name));
	else if(((Tnode *)result->info.typ->ref)->type == Tclass && !is_external((Tnode *)result->info.typ->ref) &&
	        !is_volatile((Tnode *)result->info.typ->ref))
		fprintf(fd, "\n\tif(!%s)\n\t\treturn soap_closesock(soap);\n\t%s->soap_default(soap);",
			ident(result->sym->name), ident(result->sym->name));
	else if(result->info.typ->type == Treference && ((Tnode *)result->info.typ->ref)->type == Tpointer)
		fprintf(fd, "\n\t%s = NULL;", ident(result->sym->name));
	else if(((Tnode *)result->info.typ->ref)->type == Tpointer)
		fprintf(fd, "\n\tif(!%s)\n\t\treturn soap_closesock(soap);\n\t*%s = NULL;", ident(
				result->sym->name), ident(result->sym->name));
	else if(result->info.typ->type == Treference)
		fprintf(fd, "\n\tif(!&%s)\n\t\treturn soap_closesock(soap);\n\tsoap_default_%s(soap, &%s);",
			ident(result->sym->name), c_ident((Tnode *)result->info.typ->ref), ident(result->sym->name));
	else if(!is_void(result->info.typ))
		fprintf(fd, "\n\tif(!%s)\n\t\treturn soap_closesock(soap);\n\tsoap_default_%s(soap, %s);",
			ident(result->sym->name), c_ident((Tnode *)result->info.typ->ref), ident(result->sym->name));
	fprintf(fd, "\n\tif(soap_begin_recv(soap)");
	fprintf(fd, "\n\t || soap_envelope_begin_in(soap)");
	fprintf(fd, "\n\t || soap_recv_header(soap)");
	fprintf(fd, "\n\t || soap_body_begin_in(soap))");
	fprintf(fd, "\n\t\treturn soap_closesock(soap);");
	if(is_transient(result->info.typ)) {
		fprintf(fd, "\n\tsoap_get_%s(soap, %s, \"%s\", NULL);", ident(method->sym->name), ident(result->sym->name), ns_convert(method->sym->name));
		fprintf(fd, "\n\tif(soap->error == SOAP_TAG_MISMATCH && soap->level == 2)\n\t\tsoap->error = SOAP_NO_METHOD;");
		fprintf(fd, "\n\tif(soap->error");
		fprintf(fd, "\n\t || soap_body_end_in(soap)");
		fprintf(fd, "\n\t || soap_envelope_end_in(soap)");
		fprintf(fd, "\n\t || soap_end_recv(soap))");
		fprintf(fd, "\n\t\treturn soap_closesock(soap);");
		fprintf(fd, "\n\treturn soap_closesock(soap);\n}");
		fflush(fd);
		return;
	}
	/* With RPC encoded responses, try to parse the fault first */
	if(!is_literal(method_response_encoding)) {
		fprintf(fd, "\n\tif(soap_recv_fault(soap, 1))\n\t\treturn soap->error;");
		xtag = xtyp = "";
	}
	else if(has_ns_eq(NULL, result->sym->name)) {
		if(response)
			xtag = xml_tag(response->info.typ);
		else
			xtag = ns_convert(result->sym->name);
		xtyp = xsi_type(result->info.typ);
	}
	else {
		if(response)
	      xtag = xml_tag(response->info.typ);
      else
	      xtag = xml_tag(result->info.typ);
      xtyp = ""; 
	}
	if(response) {
		fprintf(fd, "\n\tsoap_tmp_%s = soap_get_%s(soap, NULL, \"%s\", \"%s\");", c_ident(response->info.typ), c_ident(response->info.typ), xtag, xtyp);
	}
	else if(result->info.typ->type == Treference && ((Tnode *)result->info.typ->ref)->type == Tclass &&
	        !is_external((Tnode *)result->info.typ->ref) && !is_volatile((Tnode *)result->info.typ->ref) &&
	        !is_dynamic_array((Tnode *)result->info.typ->ref))
		fprintf(fd, "\n\t%s.soap_get(soap, \"%s\", \"%s\");", ident(result->sym->name), xtag, xtyp);
	else if(result->info.typ->type == Tpointer && ((Tnode *)result->info.typ->ref)->type == Tclass &&
	        !is_external((Tnode *)result->info.typ->ref) && !is_volatile((Tnode *)result->info.typ->ref) &&
	        !is_dynamic_array((Tnode *)result->info.typ->ref))
		fprintf(fd, "\n\t%s->soap_get(soap, \"%s\", \"%s\");", ident(result->sym->name), xtag, xtyp);
	else if(result->info.typ->type == Treference && ((Tnode *)result->info.typ->ref)->type == Tstruct &&
	        !is_external((Tnode *)result->info.typ->ref) && !is_volatile((Tnode *)result->info.typ->ref) &&
	        !is_dynamic_array((Tnode *)result->info.typ->ref)) {
		fprintf(fd, "\n\tsoap_get_%s(soap, &%s, \"%s\", \"%s\");", c_ident((Tnode *)result->info.typ->ref), ident(result->sym->name), xtag, xtyp);
	}
	else if(result->info.typ->type == Tpointer && ((Tnode *)result->info.typ->ref)->type == Tstruct && !is_dynamic_array((Tnode *)result->info.typ->ref)) {
		fprintf(fd, "\n\tsoap_get_%s(soap, %s, \"%s\", \"%s\");", c_ident((Tnode *)result->info.typ->ref), ident(result->sym->name), xtag, xtyp);
	}
	else if(result->info.typ->type == Tpointer && is_XML((Tnode *)result->info.typ->ref) && is_string((Tnode *)result->info.typ->ref)) {
		fprintf(fd, "\n\tsoap_inliteral(soap, NULL, %s);", ident(result->sym->name));
	}
	else if(result->info.typ->type == Treference && is_XML((Tnode *)result->info.typ->ref) && is_string((Tnode *)result->info.typ->ref)) {
		fprintf(fd, "\n\tsoap_inliteral(soap, NULL, &%s);", ident(result->sym->name));
	}
	else if(result->info.typ->type == Tpointer && is_XML((Tnode *)result->info.typ->ref) && is_wstring((Tnode *)result->info.typ->ref)) {
		fprintf(fd, "\n\tsoap_inwliteral(soap, NULL, %s);", ident(result->sym->name));
	}
	else if(result->info.typ->type == Treference && is_XML((Tnode *)result->info.typ->ref) && is_wstring((Tnode *)result->info.typ->ref)) {
		fprintf(fd, "\n\tsoap_inwliteral(soap, NULL, &%s);", ident(result->sym->name));
	}
	else if(result->info.typ->type == Treference) {
		fprintf(fd, "\n\tsoap_get_%s(soap, &%s, \"%s\", \"%s\");", c_ident(result->info.typ), ident(result->sym->name), xtag, xtyp);
	}
	else {
		fprintf(fd, "\n\tsoap_get_%s(soap, %s, \"%s\", \"%s\");", c_ident(result->info.typ), ident(result->sym->name), xtag, xtyp); 
	}
	fflush(fd);
	fprintf(fd, "\n\tif(soap->error)\n\t\treturn soap_recv_fault(soap, 0);");
	fprintf(fd, "\n\tif(soap_body_end_in(soap)");
	fprintf(fd, "\n\t || soap_envelope_end_in(soap)");
	fprintf(fd, "\n\t || soap_end_recv(soap))");
	fprintf(fd, "\n\t\treturn soap_closesock(soap);");
	if(response) {
		if(result->info.typ->type == Tarray)
			fprintf(fd, "\n\tmemcpy(%s, soap_tmp_%s->%s, sizeof(%s));", ident(result->sym->name),
				c_ident(response->info.typ), ident(result->sym->name), ident(result->sym->name));
		else if(result->info.typ->type == Treference)
			fprintf(fd, "\n\t%s = soap_tmp_%s->%s;", ident(result->sym->name), c_ident(response->info.typ), ident(result->sym->name));
		else if(!is_external((Tnode *)result->info.typ->ref)) {
			fprintf(fd, "\n\tif(%s && soap_tmp_%s->%s)", ident(result->sym->name),
				c_ident(response->info.typ), ident(result->sym->name));
			fprintf(fd, "\n\t\t*%s = *soap_tmp_%s->%s;", ident(result->sym->name),
				c_ident(response->info.typ), ident(result->sym->name));
		}
	}
	fprintf(fd, "\n\treturn soap_closesock(soap);");
	fprintf(fd, "\n}");
	fflush(fd);
}

void gen_serve_method(FILE * fd, Table * table, Entry * param, char * name)
{
	Service * sp = NULL;
	char * style, * encoding;
	Entry * result, * p, * q, * pin, * pout, * response = NULL;
	Table * input;
	const char * xtag = 0;
	Method * m;
	char * method_encoding = NULL, * method_response_encoding = NULL;
	result = (Entry *)param->info.typ->ref;
	p = entry(classtable, param->sym);
	if(!p)
		execerror("no table entry");
	if(!is_response(result->info.typ) && !is_XML(result->info.typ))
		response = get_response(param->info.typ);
	q = entry(table, param->sym);
	if(!q)
		execerror("no table entry");
	pout = (Entry *)q->info.typ->ref;
	if(name) {
		if(iflag)
			fprintf(fd, "\n\nstatic int serve_%s(%s *soap)\n{", ident(param->sym->name), name);
		else
			fprintf(fd, "\n\nstatic int serve_%s(%s *service)\n{\n\tstruct soap *soap = service->soap;\n", ident(param->sym->name), name);
	}
	else {
		fprintf(fheader, "\n\nSOAP_FMAC5 int SOAP_FMAC6 soap_serve_%s(struct soap*);", ident(param->sym->name));
		fprintf(fd, "\n\nSOAP_FMAC5 int SOAP_FMAC6 soap_serve_%s(struct soap *soap)\n{", ident(param->sym->name)); 
	}
	fprintf(fd, "\tstruct %s soap_tmp_%s;", ident(param->sym->name), ident(param->sym->name));
	for(sp = services; sp; sp = sp->next)
		if(has_ns_eq(sp->ns, param->sym->name)) {
			style = sp->style;
			encoding = sp->encoding;
			method_encoding = encoding;
			method_response_encoding = NULL;
			for(m = sp->list; m; m = m->next) {
				if(is_eq_nons(m->name, param->sym->name))                                    {
					if(m->mess == ENCODING)
						method_encoding = m->part;
					else if(m->mess == RESPONSE_ENCODING)
						method_response_encoding = m->part;
				}
			}
			break;
		}
	if(!method_response_encoding)
		method_response_encoding = method_encoding;
	fflush(fd);
	if(!is_transient(pout->info.typ)) {
		if(pout->info.typ->type == Tarray && response) {
			fprintf(fd, "\n\tstruct %s soap_tmp_%s;", c_ident(response->info.typ), c_ident(response->info.typ));
			fprintf(fd, "\n\tsoap_default_%s(soap, &soap_tmp_%s);", c_ident(response->info.typ), c_ident(response->info.typ));
		}
		else if(pout->info.typ->type == Tpointer && !is_stdstring(pout->info.typ) && !is_stdwstring(pout->info.typ) && response) {
			fprintf(fd, "\n\tstruct %s soap_tmp_%s;", c_ident(response->info.typ), c_ident(response->info.typ));
			fprintf(fd, "\n\t%s soap_tmp_%s;", c_type((Tnode *)pout->info.typ->ref), c_ident((Tnode *)pout->info.typ->ref));
			fprintf(fd, "\n\tsoap_default_%s(soap, &soap_tmp_%s);", c_ident(response->info.typ), c_ident(response->info.typ));
			if(((Tnode *)pout->info.typ->ref)->type == Tclass && !is_external((Tnode *)pout->info.typ->ref) && !is_volatile((Tnode *)pout->info.typ->ref) &&
			   !is_typedef((Tnode *)pout->info.typ->ref))
				fprintf(fd, "\n\tsoap_tmp_%s.soap_default(soap);", c_ident((Tnode *)pout->info.typ->ref));
			else if(((Tnode *)pout->info.typ->ref)->type == Tpointer)
				fprintf(fd, "\n\tsoap_tmp_%s = NULL;", c_ident((Tnode *)pout->info.typ->ref));
			else
				fprintf(fd, "\n\tsoap_default_%s(soap, &soap_tmp_%s);",
					c_ident((Tnode *)pout->info.typ->ref), c_ident((Tnode *)pout->info.typ->ref));
			fprintf(fd, "\n\tsoap_tmp_%s.%s = &soap_tmp_%s;", c_ident(response->info.typ),
				ident(pout->sym->name), c_ident((Tnode *)pout->info.typ->ref));
		}
		else if(response) {
			fprintf(fd, "\n\tstruct %s soap_tmp_%s;", c_ident(response->info.typ), c_ident(response->info.typ));
			fprintf(fd, "\n\tsoap_default_%s(soap, &soap_tmp_%s);", c_ident(response->info.typ), c_ident(response->info.typ));
		}
		else if(((Tnode *)pout->info.typ->ref)->type == Tclass &&
		        !is_stdstring((Tnode *)pout->info.typ->ref) && !is_stdwstring((Tnode *)pout->info.typ->ref) &&
		        (is_external((Tnode *)pout->info.typ->ref) || is_volatile((Tnode *)pout->info.typ->ref) ||
		         is_typedef((Tnode *)pout->info.typ->ref)) &&
		        !is_dynamic_array((Tnode *)pout->info.typ->ref)) {
			fprintf(fd, "\n\t%s;", c_type_id((Tnode *)pout->info.typ->ref, pout->sym->name));
			fprintf(fd, "\n\tsoap_default_%s(soap, &%s);", c_ident((Tnode *)pout->info.typ->ref),
				ident(pout->sym->name));
		}
		else if(((Tnode *)pout->info.typ->ref)->type == Tclass &&
		        !is_stdstring((Tnode *)pout->info.typ->ref) && !is_stdwstring((Tnode *)pout->info.typ->ref) &&
		        !is_dynamic_array((Tnode *)pout->info.typ->ref)) {
			fprintf(fd, "\n\t%s;", c_type_id((Tnode *)pout->info.typ->ref, pout->sym->name));
			fprintf(fd, "\n\t%s.soap_default(soap);", ident(pout->sym->name));
		}
		else if(((Tnode *)pout->info.typ->ref)->type == Tstruct &&
		        !is_dynamic_array((Tnode *)pout->info.typ->ref)) {
			fprintf(fd, "\n\t%s;", c_type_id((Tnode *)pout->info.typ->ref, pout->sym->name));
			fprintf(fd, "\n\tsoap_default_%s(soap, &%s);", c_ident((Tnode *)pout->info.typ->ref),
				ident(pout->sym->name));
		}
		else {
			fprintf(fd, "\n\t%s soap_tmp_%s;", c_type((Tnode *)pout->info.typ->ref), c_ident((Tnode *)pout->info.typ->ref));
		      if(is_string((Tnode *)pout->info.typ->ref) || is_wstring((Tnode *)pout->info.typ->ref))
			      fprintf(fd, "\n\tsoap_tmp_%s = NULL;", c_ident((Tnode *)pout->info.typ->ref));
		      else
			      fprintf(fd, "\n\tsoap_default_%s(soap, &soap_tmp_%s);",
				      c_ident((Tnode *)pout->info.typ->ref), c_ident((Tnode *)pout->info.typ->ref)); 
		}
	}
	fprintf(fd, "\n\tsoap_default_%s(soap, &soap_tmp_%s);", ident(param->sym->name), ident(param->sym->name));
	fflush(fd);
	if(sp && sp->URI && method_response_encoding) {
		if(is_literal(method_response_encoding))
			fprintf(fd, "\n\tsoap->encodingStyle = NULL;");
		else if(sp->encoding)
			fprintf(fd, "\n\tsoap->encodingStyle = \"%s\";", sp->encoding);
		else if(method_response_encoding)
			fprintf(fd, "\n\tsoap->encodingStyle = \"%s\";", method_response_encoding);
		else if(!eflag)
			fprintf(fd, "\n\tsoap->encodingStyle = NULL;");
	}
	else if(!eflag)
		fprintf(fd, "\n\tsoap->encodingStyle = NULL;");
	q = entry(classtable, param->sym);
	if(!is_invisible_empty(q->info.typ)) {
		fprintf(fd, "\n\tif(!soap_get_%s(soap, &soap_tmp_%s, \"%s\", NULL))", ident(param->sym->name),
			ident(param->sym->name), ns_convert(param->sym->name));
		fprintf(fd, "\n\t\treturn soap->error;");
	}
	fprintf(fd, "\n\tif(soap_body_end_in(soap)");
	fprintf(fd, "\n\t || soap_envelope_end_in(soap)");
	fprintf(fd, "\n\t || soap_end_recv(soap))\n\t\treturn soap->error;");
	if(name) {
		if(iflag)
			fprintf(fd, "\n\tsoap->error = soap->%s(", ns_cname(param->sym->name, NULL));
		else
			fprintf(fd, "\n\tsoap->error = service->%s(", ns_cname(param->sym->name, NULL));
	}
	else
		fprintf(fd, "\n\tsoap->error = %s(soap", ident(param->sym->name));
	fflush(fd);
	input = (Table *)q->info.typ->ref;
	for(pin = input->list; pin; pin = pin->next)
		fprintf(fd, "%ssoap_tmp_%s.%s", !name || pin != input->list ? ", " : "", ident(param->sym->name),
			ident(pin->sym->name));
	if(is_transient(pout->info.typ))
		fprintf(fd, ");");
	else {
		if(!name || input->list)
		      fprintf(fd, ", ");
	      if(response)
		      fprintf(fd, "soap_tmp_%s.%s);", c_ident(response->info.typ), ident(pout->sym->name));
	      else if(pout->info.typ->type == Treference && (((Tnode *)pout->info.typ->ref)->type == Tstruct ||
		       ((Tnode *)pout->info.typ->ref)->type == Tclass) && !is_stdstring((Tnode *)pout->info.typ->ref) &&
		      !is_stdwstring((Tnode *)pout->info.typ->ref) && !is_dynamic_array((Tnode *)pout->info.typ->ref))
		      fprintf(fd, "%s);", ident(pout->sym->name));
	      else if((((Tnode *)pout->info.typ->ref)->type == Tstruct || ((Tnode *)pout->info.typ->ref)->type ==
		       Tclass) && !is_stdstring((Tnode *)pout->info.typ->ref) &&
		      !is_stdwstring((Tnode *)pout->info.typ->ref) && !is_dynamic_array((Tnode *)pout->info.typ->ref))
		      fprintf(fd, "&%s);", ident(pout->sym->name));
	      else if(pout->info.typ->type == Treference)
		      fprintf(fd, "soap_tmp_%s);", c_ident((Tnode *)pout->info.typ->ref));
	      else
		      fprintf(fd, "&soap_tmp_%s);", c_ident((Tnode *)pout->info.typ->ref)); }
	fprintf(fd, "\n\tif(soap->error)\n\t\treturn soap->error;");
	if(!is_transient(pout->info.typ)) {
		fprintf(fd, "\n\tsoap_serializeheader(soap);");
		if(pout->info.typ->type == Tarray && response)
			fprintf(fd, "\n\tsoap_serialize_%s(soap, &soap_tmp_%s);", c_ident(response->info.typ), c_ident(response->info.typ));
		else if(response)
			fprintf(fd, "\n\tsoap_serialize_%s(soap, &soap_tmp_%s);", c_ident(response->info.typ), c_ident(response->info.typ));
		else if(((Tnode *)pout->info.typ->ref)->type == Tclass &&
		        !is_stdstring((Tnode *)pout->info.typ->ref) && !is_stdwstring((Tnode *)pout->info.typ->ref) &&
		        (is_external((Tnode *)pout->info.typ->ref) || is_volatile((Tnode *)pout->info.typ->ref) ||
		         is_typedef((Tnode *)pout->info.typ->ref)) && !is_dynamic_array((Tnode *)pout->info.typ->ref))
			fprintf(fd, "\n\tsoap_serialize_%s(soap, &%s);", c_ident((Tnode *)pout->info.typ->ref),
				ident(pout->sym->name));
		else if(((Tnode *)pout->info.typ->ref)->type == Tclass &&
		        !is_stdstring((Tnode *)pout->info.typ->ref) && !is_stdwstring((Tnode *)pout->info.typ->ref) &&
		        !is_dynamic_array((Tnode *)pout->info.typ->ref))
			fprintf(fd, "\n\t%s.soap_serialize(soap);", ident(pout->sym->name));
		else if(((Tnode *)pout->info.typ->ref)->type == Tstruct &&
		        !is_dynamic_array((Tnode *)pout->info.typ->ref))
			fprintf(fd, "\n\tsoap_serialize_%s(soap, &%s);", c_ident((Tnode *)pout->info.typ->ref),
				ident(pout->sym->name));
		else if(!is_XML((Tnode *)pout->info.typ->ref))
			fprintf(fd, "\n\tsoap_serialize_%s(soap, &soap_tmp_%s);", c_ident(
					(Tnode *)pout->info.typ->ref), c_ident((Tnode *)pout->info.typ->ref));
		if(has_ns_eq(NULL, pout->sym->name))
			xtag = ns_convert(pout->sym->name);
		else
			xtag = xml_tag(pout->info.typ);
		fprintf(fd, "\n\tif(soap_begin_count(soap))\n\t\treturn soap->error;");
		fprintf(fd, "\n\tif(soap->mode & SOAP_IO_LENGTH)");
		fprintf(fd, " {\n\t\tif(soap_envelope_begin_out(soap)");
		fprintf(fd, "\n\t\t || soap_putheader(soap)");
		fprintf(fd, "\n\t\t || soap_body_begin_out(soap)");
		if(response)
			fprintf(fd, "\n\t\t || soap_put_%s(soap, &soap_tmp_%s, \"%s\", NULL)",
				c_ident(response->info.typ), c_ident(response->info.typ), xml_tag(response->info.typ));
		else if(((Tnode *)pout->info.typ->ref)->type == Tclass &&
		        !is_stdstring((Tnode *)pout->info.typ->ref) && !is_stdwstring((Tnode *)pout->info.typ->ref) &&
		        (is_external((Tnode *)pout->info.typ->ref) || is_volatile((Tnode *)pout->info.typ->ref) ||
		         is_typedef((Tnode *)pout->info.typ->ref)) && !is_dynamic_array((Tnode *)pout->info.typ->ref))
			fprintf(fd, "\n\t\t || soap_put_%s(soap, &%s, \"%s\", NULL)",
				c_ident((Tnode *)pout->info.typ->ref), ident(pout->sym->name),
				ns_convert(pout->sym->name));
		else if(((Tnode *)pout->info.typ->ref)->type == Tclass &&
		        !is_stdstring((Tnode *)pout->info.typ->ref) && !is_stdwstring((Tnode *)pout->info.typ->ref) &&
		        !is_dynamic_array((Tnode *)pout->info.typ->ref))
			fprintf(fd, "\n\t\t || %s.soap_put(soap, \"%s\", \"\")", ident(pout->sym->name), xtag);
		else if(((Tnode *)pout->info.typ->ref)->type == Tstruct &&
		        !is_dynamic_array((Tnode *)pout->info.typ->ref))
			fprintf(fd, "\n\t\t || soap_put_%s(soap, &%s, \"%s\", NULL)",
				c_ident((Tnode *)pout->info.typ->ref), ident(pout->sym->name), xtag);
		else if(is_XML((Tnode *)pout->info.typ->ref) && is_string((Tnode *)pout->info.typ->ref))
			fprintf(fd, "\n\t\t || soap_outliteral(soap, \"%s\", &soap_tmp_%s, NULL)",
				ns_convert(pout->sym->name), c_ident((Tnode *)pout->info.typ->ref));
		else if(is_XML((Tnode *)pout->info.typ->ref) && is_wstring((Tnode *)pout->info.typ->ref))
			fprintf(fd, "\n\t\t || soap_outwliteral(soap, \"%s\", &soap_tmp_%s, NULL)",
				ns_convert(pout->sym->name), c_ident((Tnode *)pout->info.typ->ref));
		else
			fprintf(fd, "\n\t\t || soap_put_%s(soap, &soap_tmp_%s, \"%s\", NULL)", c_ident(
					pout->info.typ), c_ident((Tnode *)pout->info.typ->ref),
				ns_convert(pout->sym->name));
		fprintf(fd, "\n\t\t || soap_body_end_out(soap)");
		fprintf(fd, "\n\t\t || soap_envelope_end_out(soap))");
		fprintf(fd, "\n\t\t\t return soap->error;");
		fprintf(fd, "\n\t};");
		fprintf(fd, "\n\tif(soap_end_count(soap)");
		fprintf(fd, "\n\t || soap_response(soap, SOAP_OK)");
		fprintf(fd, "\n\t || soap_envelope_begin_out(soap)");
		fprintf(fd, "\n\t || soap_putheader(soap)");
		fprintf(fd, "\n\t || soap_body_begin_out(soap)");
		if(response)
			fprintf(fd, "\n\t || soap_put_%s(soap, &soap_tmp_%s, \"%s\", NULL)", c_ident(response->info.typ), c_ident(response->info.typ), xml_tag(response->info.typ));
		else if(((Tnode *)pout->info.typ->ref)->type == Tclass &&
		        !is_stdstring((Tnode *)pout->info.typ->ref) && !is_stdwstring((Tnode *)pout->info.typ->ref) &&
		        (is_external((Tnode *)pout->info.typ->ref) || is_volatile((Tnode *)pout->info.typ->ref) ||
		         is_typedef((Tnode *)pout->info.typ->ref)) && !is_dynamic_array((Tnode *)pout->info.typ->ref))
			fprintf(fd, "\n\t || soap_put_%s(soap, &%s, \"%s\", NULL)",
				c_ident((Tnode *)pout->info.typ->ref), ident(pout->sym->name),
				ns_convert(pout->sym->name));
		else if(((Tnode *)pout->info.typ->ref)->type == Tclass &&
		        !is_stdstring((Tnode *)pout->info.typ->ref) && !is_stdwstring((Tnode *)pout->info.typ->ref) &&
		        !is_dynamic_array((Tnode *)pout->info.typ->ref))
			fprintf(fd, "\n\t || %s.soap_put(soap, \"%s\", \"\")", ident(pout->sym->name), xtag);
		else if(((Tnode *)pout->info.typ->ref)->type == Tstruct &&
		        !is_dynamic_array((Tnode *)pout->info.typ->ref))
			fprintf(fd, "\n\t || soap_put_%s(soap, &%s, \"%s\", NULL)",
				c_ident((Tnode *)pout->info.typ->ref), ident(pout->sym->name), xtag);
		else if(is_XML((Tnode *)pout->info.typ->ref) && is_string((Tnode *)pout->info.typ->ref))
			fprintf(fd, "\n\t || soap_outliteral(soap, \"%s\", &soap_tmp_%s, NULL)",
				ns_convert(pout->sym->name), c_ident((Tnode *)pout->info.typ->ref));
		else if(is_XML((Tnode *)pout->info.typ->ref) && is_wstring((Tnode *)pout->info.typ->ref))
			fprintf(fd, "\n\t || soap_outwliteral(soap, \"%s\", &soap_tmp_%s, NULL)",
				ns_convert(pout->sym->name), c_ident((Tnode *)pout->info.typ->ref));
		else
			fprintf(fd, "\n\t || soap_put_%s(soap, &soap_tmp_%s, \"%s\", NULL)", c_ident(pout->info.typ), c_ident((Tnode *)pout->info.typ->ref),
				ns_convert(pout->sym->name));
		fprintf(fd, "\n\t || soap_body_end_out(soap)");
		fprintf(fd, "\n\t || soap_envelope_end_out(soap)");
		fprintf(fd, "\n\t || soap_end_send(soap))");
		fprintf(fd, "\n\t\treturn soap->error;");
	}
	fprintf(fd, "\n\treturn soap_closesock(soap);");
	fprintf(fd, "\n}");
	fflush(fd);
}

void gen_object_code(FILE * fd, Table * table, Symbol * ns, char * name, char * URL, char * executable, char * URI,
	char * encoding)
{
	Entry * p, * method, * catch_method, * param;
	Table * t;
	char * soap;
	if(iflag)
		soap = "this";
	else
		soap = "this->soap";
	fprintf(fd, "\n\n#include \"%s%s.h\"", prefix, name);
	if(namespaceid)
		fprintf(fd, "\n\nnamespace %s {", namespaceid);
	if(iflag) {
		fprintf(fd, "\n\n%s::%s()\n{\n\t%s_init(SOAP_IO_DEFAULT, SOAP_IO_DEFAULT);\n}", name, name, name);
		fprintf(fd, "\n\n%s::%s(const struct soap &_soap) : soap(_soap)\n{ }", name, name);
		fprintf(fd, "\n\n%s::%s(soap_mode iomode)\n{\n\t%s_init(iomode, iomode);\n}", name, name, name);
		fprintf(fd, "\n\n%s::%s(soap_mode imode, soap_mode omode)\n{\n\t%s_init(imode, omode);\n}", name, name,
			name);
		fprintf(fd, "\n\n%s::~%s()\n{ }", name, name);
	}
	else {fprintf(
		      fd,
		      "\n\n%s::%s()\n{\n\tthis->soap = soap_new();\n\tthis->own = true;\n\t%s_init(SOAP_IO_DEFAULT, SOAP_IO_DEFAULT);\n}",
		      name, name, name);
	      fprintf(
		      fd,
		      "\n\n%s::%s(struct soap *_soap)\n{\n\tthis->soap = _soap;\n\tthis->own = false;\n\t%s_init(_soap->imode, _soap->omode);\n}",
		      name, name, name);
	      fprintf(
		      fd,
		      "\n\n%s::%s(soap_mode iomode)\n{\n\tthis->soap = soap_new();\n\tthis->own = true;\n\t%s_init(iomode, iomode);\n}",
		      name, name, name);
	      fprintf(
		      fd,
		      "\n\n%s::%s(soap_mode imode, soap_mode omode)\n{\n\tthis->soap = soap_new();\n\tthis->own = true;\n\t%s_init(imode, omode);\n}",
		      name, name, name);
	      fprintf(fd, "\n\n%s::~%s()\n{\n\tif(this->own)\n\t\tsoap_free(this->soap);\n}", name, name); }
	fprintf(
		fd,
		"\n\nvoid %s::%s_init(soap_mode imode, soap_mode omode)\n{\n\tsoap_imode(%s, imode);\n\tsoap_omode(%s, omode);\n\tstatic const struct Namespace namespaces[] =\n",
		name, name, soap, soap);
	gen_nsmap(fd, ns, URI);
	fprintf(fd, "\tsoap_set_namespaces(%s, namespaces);\n};", soap);
	fprintf(fd, "\n\nvoid %s::destroy()\n{\n\tsoap_destroy(%s);\n\tsoap_end(%s);\n}", name, soap, soap);
	fprintf(
		fd,
		"\n\nvoid %s::reset()\n{\n\tdestroy();\n\tsoap_done(%s);\n\tsoap_init(%s);\n\t%s_init(SOAP_IO_DEFAULT, SOAP_IO_DEFAULT);\n}",
		name, soap, soap, name);
	if(iflag)
		fprintf(
			fd,
			"\n\n#ifndef WITH_PURE_VIRTUAL\n%s *%s::copy()\n{\n\t%s *dup = SOAP_NEW_COPY(%s(*(struct soap*)%s));\n\treturn dup;\n}\n#endif",
			name, name, name, name, soap);
	else
		fprintf(
			fd,
			"\n\n#ifndef WITH_PURE_VIRTUAL\n%s *%s::copy()\n{\n\t%s *dup = SOAP_NEW_COPY(%s);\n\tif(dup)\n\t\tsoap_copy_context(dup->soap, this->soap);\n\treturn dup;\n}\n#endif",
			name, name, name, name);
	fprintf(fd, "\n\nint %s::soap_close_socket()\n{\n\treturn soap_closesock(%s);\n}", name, soap);
	fprintf(fd, "\n\nint %s::soap_force_close_socket()\n{\n\treturn soap_force_closesock(%s);\n}", name, soap);
	fprintf(
		fd,
		"\n\nint %s::soap_senderfault(const char *string, const char *detailXML)\n{\n\treturn ::soap_sender_fault(%s, string, detailXML);\n}",
		name, soap);
	fprintf(
		fd,
		"\n\nint %s::soap_senderfault(const char *subcodeQName, const char *string, const char *detailXML)\n{\n\treturn ::soap_sender_fault_subcode(%s, subcodeQName, string, detailXML);\n}",
		name, soap);
	fprintf(
		fd,
		"\n\nint %s::soap_receiverfault(const char *string, const char *detailXML)\n{\n\treturn ::soap_receiver_fault(%s, string, detailXML);\n}",
		name, soap);
	fprintf(
		fd,
		"\n\nint %s::soap_receiverfault(const char *subcodeQName, const char *string, const char *detailXML)\n{\n\treturn ::soap_receiver_fault_subcode(%s, subcodeQName, string, detailXML);\n}",
		name, soap);
	fprintf(fd, "\n\nvoid %s::soap_print_fault(FILE *fd)\n{\n\t::soap_print_fault(%s, fd);\n}", name, soap);
	fprintf(
		fd,
		"\n\n#ifndef WITH_LEAN\n#ifndef WITH_COMPAT\nvoid %s::soap_stream_fault(std::ostream& os)\n{\n\t::soap_stream_fault(%s, os);\n}\n#endif",
		name, soap);
	fprintf(
		fd,
		"\n\nchar *%s::soap_sprint_fault(char *buf, size_t len)\n{\n\treturn ::soap_sprint_fault(%s, buf, len);\n}\n#endif",
		name, soap);
	fprintf(fd, "\n\nvoid %s::soap_noheader()\n{\n\t%s->header = NULL;\n}", name, soap);
	if(!namespaceid) {
		p = entry(classtable, lookup("SOAP_ENV__Header"));
		if(p) {
			t = (Table *)p->info.typ->ref;
			if(t && t->list && !is_void(t->list->info.typ)) {
				fprintf(fd, "\n\nvoid %s::soap_header(", name);
				gen_params(fd, t, NULL, 0);
				fprintf(fd, "\n{\n\t::soap_header(%s);", soap);
				for(param = t->list; param; param = param->next) {
					if(namespaceid)
						fprintf(fd, "\n\t((%s::SOAP_ENV__Header*)%s->header)->%s = %s;",
							namespaceid, soap, ident(
								param->sym->name), ident(param->sym->name));
					else
						fprintf(fd, "\n\t%s->header->%s = %s;", soap, ident(
								param->sym->name), ident(param->sym->name));
				}
				fprintf(fd, "\n}");
			}
		}
	}
	fprintf(fd, "\n\nconst SOAP_ENV__Header *%s::soap_header()\n{\n\treturn %s->header;\n}", name, soap);
	fprintf(fd, "\n\nint %s::run(int port)\n{\n\tif(soap_valid_socket(bind(NULL, port, 100))) {\n\t\tfor(;;) {\n\t\t\tif(!soap_valid_socket(accept()) || serve())\n\t\t\t\treturn %s->error;\n\t\t\tsoap_destroy(%s);\n\t\t\tsoap_end(%s);\n\t\t}\n\t}\n\telse\n\t\treturn %s->error;\n\treturn SOAP_OK;\n}",
		name, soap, soap, soap, soap);
	fprintf(
		fd,
		"\n\nSOAP_SOCKET %s::bind(const char *host, int port, int backlog)\n{\n\treturn soap_bind(%s, host, port, backlog);\n}",
		name, soap);
	fprintf(fd, "\n\nSOAP_SOCKET %s::accept()\n{\n\treturn soap_accept(%s);\n}", name, soap);
	fprintf(fd, "\n\nint %s::serve()", name);
	fprintf(fd, "\n{\n#ifndef WITH_FASTCGI\n\tunsigned int k = %s->max_keep_alive;\n#endif\n\tdo {\n\t", soap);
	fprintf(fd,
		"\n\n#ifndef WITH_FASTCGI\n\t\tif(%s->max_keep_alive > 0 && !--k)\n\t\t\t%s->keep_alive = 0;\n#endif",
		soap,
		soap);
	fprintf(
		fd,
		"\n\n\t\tif(soap_begin_serve(%s))\n\t\t{\tif(%s->error >= SOAP_STOP)\n\t\t\t\tcontinue;\n\t\t\treturn %s->error;\n\t\t}",
		soap, soap, soap);
	fprintf(
		fd,
		"\n\t\tif(dispatch() || (%s->fserveloop && %s->fserveloop(%s)))\n\t\t{\n#ifdef WITH_FASTCGI\n\t\t\tsoap_send_fault(%s);\n#else\n\t\t\treturn soap_send_fault(%s);\n#endif\n\t\t}",
		soap, soap, soap, soap, soap);
	fprintf(
		fd,
		"\n\n#ifdef WITH_FASTCGI\n\t\tsoap_destroy(%s);\n\t\tsoap_end(%s);\n\t} while(1);\n#else\n\t} while(%s->keep_alive);\n#endif",
		soap, soap, soap);

	fprintf(fd, "\n\treturn SOAP_OK;");
	fprintf(fd, "\n}\n");
	for(method = table->list; method; method = method->next)
		if(method->info.typ->type == Tfun && !(method->info.sto&Sextern) &&
		   has_ns_eq(ns->name, method->sym->name))
			fprintf(fd, "\nstatic int serve_%s(%s*);", ident(method->sym->name), name);
	fprintf(fd, "\n\nint %s::dispatch()\n{", name);
	if(!iflag)
		fprintf(fd, "\t%s_init(this->soap->imode, this->soap->omode);\n", name);
	fprintf(fd, "\tsoap_peek_element(%s);", soap);
	catch_method = NULL;
	for(method = table->list; method; method = method->next) {
		char * action = NULL;
		if(method->info.typ->type == Tfun && !(method->info.sto&Sextern) &&
		   has_ns_eq(ns->name, method->sym->name)) {
			if(aflag)
			{
				Service * sp;
				for(sp = services; sp; sp = sp->next) {
					if(has_ns_eq(sp->ns,
						   method->sym->name))                                        {
						Method * m;
						for(m = sp->list; m; m = m->next) {
							if(is_eq_nons(m->name,
								   method->sym->name))
							{
								if(m->mess == ACTION || m->mess == REQUEST_ACTION)
									action = m->part;
							}
						}
					}
				}
			}
			if(is_invisible(method->sym->name)) {
				Entry * param = entry(classtable, method->sym);
				if(param) {
					param = ((Table *)param->info.typ->ref)->list;
					if(param) {
						if(action)            {
							if(*action == '"')
								fprintf(fd, "\n\tif((!%s->action && !soap_match_tag(%s, %s->tag, \"%s\")) || (%s->action && sstreq(%s->action, %s))",
									soap, soap, soap, ns_convert(param->sym->name), soap, soap, action);
							else
								fprintf(fd, "\n\tif((!%s->action && !soap_match_tag(%s, %s->tag, \"%s\")) || (%s->action && sstreq(%s->action, \"%s\"))",
									soap, soap, soap, ns_convert(param->sym->name), soap, soap, action);
						}
						else
							fprintf(fd, "\n\tif(!soap_match_tag(%s, %s->tag, \"%s\")", soap, soap, ns_convert(param->sym->name));
						fprintf(fd, ")\n\t\treturn serve_%s(this);", ident(method->sym->name));
					}
					else
						catch_method = method;
				}
				else
					catch_method = method;
			}
			else {if(action)      {
				      if(*action == '"')
					      fprintf(fd, "\n\tif((!%s->action && !soap_match_tag(%s, %s->tag, \"%s\")) || (%s->action && sstreq(%s->action, %s))",
						      soap, soap, soap, ns_convert(method->sym->name), soap, soap, action);
				      else
					      fprintf(fd, "\n\tif((!%s->action && !soap_match_tag(%s, %s->tag, \"%s\")) || (%s->action && sstreq(%s->action, \"%s\"))",
						      soap, soap, soap, ns_convert(method->sym->name), soap, soap, action);
			      }
			      else
				      fprintf(fd, "\n\tif(!soap_match_tag(%s, %s->tag, \"%s\")", soap, soap, ns_convert(method->sym->name));
			      fprintf(fd, ")\n\t\treturn serve_%s(this);", ident(method->sym->name)); }
		}
	}
	if(catch_method)
		fprintf(fd, "\n\treturn serve_%s(this);\n}", ident(catch_method->sym->name));
	else
		fprintf(fd, "\n\treturn %s->error = SOAP_NO_METHOD;\n}", soap);
	for(method = table->list; method; method = method->next)
		if(method->info.typ->type == Tfun && !(method->info.sto&Sextern) && !is_imported(method->info.typ) &&
		   has_ns_eq(ns->name, method->sym->name))
			gen_serve_method(fd, table, method, name);
	if(namespaceid)
		fprintf(fd, "\n\n} // namespace %s\n", namespaceid);
	fprintf(fd, "\n/* End of server object code */\n");
}

void gen_response_begin(FILE * fd, int n, char * s)
{
	if(!is_invisible(s))
		fprintf(fd, "%*s<%sResponse>\n", n, "", s);
}

void gen_response_end(FILE * fd, int n, char * s)
{
	if(!is_invisible(s))
		fprintf(fd, "%*s</%sResponse>\n", n, "", s);
}

void gen_element_begin(FILE * fd, int n, char * s, char * t)
{
	if(!is_invisible(s)) {
		if(tflag && t && *t)
			fprintf(fd, "%*s<%s xsi:type=\"%s\"", n, "", s, t);
		else
			fprintf(fd, "%*s<%s", n, "", s);
	}
}

void gen_element_end(FILE * fd, int n, char * s)
{
	if(!is_invisible(s))
		fprintf(fd, "%*s</%s>\n", n, "", s);
	else
		fprintf(fd, "\n");
}

void gen_data(char * buf, Table * t, char * ns, char * name, char * URL, char * executable, char * URI, char * encoding)
{
	Entry * p, * q, * r;
	FILE * fd;
	char * method_encoding = NULL;
	char * method_response_encoding = NULL;
	if(t) {
		for(p = t->list; p; p = p->next)
			if(p->info.typ->type == Tfun && !(p->info.sto&Sextern) && has_ns_eq(ns, p->sym->name)) {
				Service * sp;
				Method * m;
				char * nse = ns_qualifiedElement(p->info.typ);
				char * nsa = ns_qualifiedAttribute(p->info.typ);
				method_encoding = encoding;
				method_response_encoding = NULL;
				for(sp = services; sp; sp = sp->next) {
					if(!tagcmp(sp->ns, ns))                                        {
						for(m = sp->list; m; m =
						            m->next)
						{
							if(is_eq_nons(m->name,
								   p->sym->name))
							{
								if(m->mess == ENCODING)
									method_encoding = m->part;
								else if(m->mess == RESPONSE_ENCODING)
									method_response_encoding = m->part;
							}
						}
					}
				}
				if(!method_response_encoding)
					method_response_encoding = method_encoding;
				/* request */
				fd = gen_env(buf, ns_remove(p->sym->name), 0, t, ns, name, URL, executable, URI, method_encoding);
				if(!fd)
					return;
				q = entry(classtable, p->sym);
				if(yflag) {
					fprintf(fd, "%*s<!-- %s(...", 2, "", ident(p->sym->name));
					if(q) {
						Table * r = (Table *)q->info.typ->ref;
						while(r) {
							for(Entry * e = r->list; e; e = e->next)
								fprintf(fd, ", %s", c_type_id(e->info.typ, e->sym->name));
							r = r->prev;
						}
					}
					fprintf(fd, ", ...) -->\n");
				}
				gen_element_begin(fd, 2, ns_convert(p->sym->name), NULL);
				if(q) {
					if(!is_invisible(p->sym->name))        {
						gen_atts(fd, 2, (Table *)q->info.typ->ref, nsa);
						fprintf(fd, "\n");
					}
					for(q = ((Table *)q->info.typ->ref)->list; q; q = q->next)
						gen_field(fd, 3, q, nse, nsa, method_encoding);
				}
				gen_element_end(fd, 2, ns_convert(p->sym->name));
				fprintf(fd, " </%s:Body>\n</%s:Envelope>\n", S_SoapEnvNs, S_SoapEnvNs);
				fclose(fd);
				/* response */
				q = (Entry *)p->info.typ->ref;
				if(q && !is_transient(q->info.typ)) {
					fd = gen_env(buf, ns_remove(p->sym->name), 1, t, ns, name, URL, executable, URI, method_response_encoding);
					if(!fd)
						return;
					if(q && !is_response(q->info.typ))
						if(is_XML((Tnode *)q->info.typ->ref)) {
							gen_response_begin(fd, 2, ns_convert(p->sym->name));
							gen_response_end(fd, 2, ns_convert(p->sym->name));
						}
						else {
							gen_response_begin(fd, 2, ns_convert(p->sym->name));
						      gen_field(fd, 3, q, nse, nsa, method_response_encoding);
						      gen_response_end(fd, 2, ns_convert(p->sym->name)); 
						}
					else if(q && q->info.typ->ref && ((Tnode *)q->info.typ->ref)->ref) {
						char * xtag;
						nse = ns_qualifiedElement((Tnode *)q->info.typ->ref);
						nsa = ns_qualifiedAttribute((Tnode *)q->info.typ->ref);
						if(has_ns_eq(NULL, q->sym->name))
							xtag = q->sym->name;
						else
							xtag = ((Tnode *)q->info.typ->ref)->id->name;
						if(yflag)
							fprintf(fd, "%*s<!-- %s(..., %s) -->\n", 2, "", ident(p->sym->name), c_type_id(q->info.typ, q->sym->name));
						gen_element_begin(fd, 2, ns_add(xtag, nse), NULL);
						if(!is_invisible(xtag)) {
							gen_atts(fd, 2, (Table *)((Tnode *)q->info.typ->ref)->ref, nsa);
							fprintf(fd, "\n");
						}
						for(r = ((Table *)((Tnode *)q->info.typ->ref)->ref)->list; r; r = r->next)
							gen_field(fd, 3, r, nse, nsa, method_response_encoding);
						gen_element_end(fd, 2, ns_add(xtag, nse));
					}
					fflush(fd);
					fprintf(fd, " </%s:Body>\n</%s:Envelope>\n", S_SoapEnvNs, S_SoapEnvNs);
					fclose(fd);
				}
			}
	}
}

void gen_field(FILE * fd, int n, Entry * p, char * nse, char * nsa, char * encoding)
{
	Entry * q;
	char tmp[32];
	LONG64 i;
	int d;
	if(!(p->info.sto&(Sattribute|Sconst|Sprivate|Sprotected)) && !is_transient(p->info.typ) && p->info.typ->type !=
	   Tfun && strncmp(p->sym->name, "__size", 6) && strncmp(p->sym->name, "__type", 6) && !is_choice(p)) {
		if(is_soap12(encoding) && (p->info.sto&Sreturn) &&
		   (nse || has_ns_eq(NULL, p->sym->name)) && !is_literal(encoding))
			fprintf(fd, "%*s<SOAP-RPC:result xmlns:SOAP-RPC=\"%s\">%s</SOAP-RPC:result>\n", n, "", rpcURI, ns_add(p->sym->name, nse));
		if(is_XML(p->info.typ)) {
			if(yflag)
				fprintf(fd, "%*s<!-- %s -->\n", n, "", c_type_id(p->info.typ, p->sym->name));
			gen_element_begin(fd, n, ns_add(p->sym->name, nse), NULL);
			if(!is_invisible(p->sym->name))
				fprintf(fd, ">");
			else
				fprintf(fd, "%*s<!-- extensibility element(s) -->\n", n, "");
			gen_element_end(fd, n, ns_add(p->sym->name, nse));
		}
		else {
			if(!is_string(p->info.typ) && n >= 10 && p->info.minOccurs <= 0) { 
				/* Do not generate nil, since some tools don't accept it:
				if(!is_invisible(p->sym->name)) { gen_element_begin(fd, n, ns_add(p->sym->name, nse), NULL); fprintf(fd, " xsi:nil=\"true\"/>"); }
				*/
			      return;
		      }
		      else if(n >= 20) {
			      fprintf(fd, "%*s<!-- WARNING max depth exceeded: schema appears to incorrectly define infinitely large documents in recursion over mandatory elements with minOccurs>0 -->\n",
				      n, "");
			      return;
		      }
		      else if(is_fixedstring(p->info.typ)) {
			      if(yflag)
				      fprintf(fd, "%*s<!-- %s -->\n", n, "", c_type_id(p->info.typ, p->sym->name));
			      gen_element_begin(fd, n, ns_add(p->sym->name, nse), xsi_type(p->info.typ));
			      fprintf(fd, ">");
			      fflush(fd);
			      if(p->info.hasval)
				      fprintf(fd, "%s", xstring(p->info.val.s));
			      else
				      gen_val(fd, n, p->info.typ, nse, nsa, encoding);
		      }
		      else if(p->info.typ->type == Tarray) {
			      i = ((Tnode *)p->info.typ->ref)->width;
			      if(i) {
				      i = p->info.typ->width/i;
				      if(i > 4)
					      i = 2;
			      }
			      if(yflag)
				      fprintf(fd, "%*s<!-- %s -->\n", n, "", c_type_id(p->info.typ, p->sym->name));
			      gen_element_begin(fd, n, ns_add(p->sym->name, nse), "SOAP-ENC:Array");
			      fprintf(fd, " %s:arrayType=\"%s[" SOAP_LONG_FORMAT "]\">", S_SoapEncNs, xsi_type_Tarray(p->info.typ), i);
			      fflush(fd);
			      gen_val(fd, n, p->info.typ, nse, nsa, encoding);
		      }
		      else if(is_dynamic_array(p->info.typ) && !is_binary(p->info.typ)) {
			      if(!eflag && (has_ns(p->info.typ) || is_untyped(p->info.typ))) {
				      if(yflag)
					      fprintf(fd, "%*s<!-- %s -->\n", n, "", c_type_id(p->info.typ, p->sym->name));
				      gen_element_begin(fd, n, ns_add(p->sym->name, nse), xsi_type(p->info.typ));
				      gen_atts(fd, n, (Table *)p->info.typ->ref, nsa);
			      }
			      else {
					  d = get_Darraydims(p->info.typ);
				    if(d) {
					    for(i = 0; i < d-1; i++) {
						    tmp[2*i] = ',';
						    tmp[2*i+1] = '1';
					    }
					    tmp[2*d-2] = '\0';
				    }
				    else
					    *tmp = '\0';
				    if(yflag)
					    fprintf(fd, "%*s<!-- %s -->\n", n, "", c_type_id(p->info.typ, p->sym->name));
				    gen_element_begin(fd, n, ns_add(p->sym->name, nse), "SOAP-ENC:Array");
				    if(((Table *)p->info.typ->ref)->list->info.minOccurs > 0)
					    fprintf(fd, " %s:arrayType=\"%s[" SOAP_LONG_FORMAT "%s]\">",
						    S_SoapEncNs, wsdl_type(((Table *)p->info.typ->ref)->list->info.typ, ""), ((Table *)p->info.typ->ref)->list->info.minOccurs, tmp);
				    else
					    fprintf(fd, " %s:arrayType=\"%s[1%s]\">",
						    S_SoapEncNs, wsdl_type(((Table *)p->info.typ->ref)->list->info.typ, ""), tmp); 
				  }
			      fflush(fd);
			      gen_val(fd, n, p->info.typ, nse, nsa, encoding);
		      }
		      else if((p->info.typ->type == Tpointer || p->info.typ->type == Treference) && is_dynamic_array((Tnode *)p->info.typ->ref) &&
			      !is_binary((Tnode *)p->info.typ->ref)) {
			      if(!eflag && (has_ns((Tnode *)p->info.typ->ref) || is_untyped((Tnode *)p->info.typ->ref))) {
				      if(yflag)
					      fprintf(fd, "%*s<!-- %s -->\n", n, "", c_type_id(p->info.typ, p->sym->name));
				      gen_element_begin(fd, n, ns_add(p->sym->name, nse), xsi_type((Tnode *)p->info.typ->ref));
				      gen_atts(fd, n, (Table *)((Tnode *)p->info.typ->ref)->ref, nsa);
			      }
			      else {
					  d = get_Darraydims((Tnode *)p->info.typ->ref);
				    if(d) {
					    for(i = 0; i < d-1; i++)        {
						    tmp[2*i] = ',';
						    tmp[2*i+1] = '1';
					    }
					    tmp[2*d-2] = '\0';
				    }
				    else
					    *tmp = '\0';
				    if(yflag)
					    fprintf(fd, "%*s<!-- %s -->\n", n, "", c_type_id(p->info.typ, p->sym->name));
				    gen_element_begin(fd, n, ns_add(p->sym->name, nse), "SOAP-ENC:Array");
				    if((((Tnode *)p->info.typ->ref)->type == Tstruct || ((Tnode *)p->info.typ->ref)->type == Tclass) &&
				       ((Table *)((Tnode *)p->info.typ->ref)->ref)->list->info.minOccurs > 0)
					    fprintf(fd, " %s:arrayType=\"%s[" SOAP_LONG_FORMAT "%s]\">",
						    S_SoapEncNs, wsdl_type(((Table *)((Tnode *)p->info.typ->ref)->ref)->list->info.typ, ""),
						    ((Table *)((Tnode *)p->info.typ->ref)->ref)->list->info.minOccurs, tmp);
				    else
					    fprintf(fd, " %s:arrayType=\"%s[1%s]\">",
						    S_SoapEncNs, wsdl_type(((Table *)((Tnode *)p->info.typ->ref)->ref)->list->info.typ, ""), tmp); 
				  }
			      fflush(fd);
			      gen_val(fd, n, (Tnode *)p->info.typ->ref, nse, nsa, encoding);
		      }
		      else if(p->info.typ->type == Tstruct || p->info.typ->type == Tclass) { 
				/* 
				if(!is_primclass(p->info.typ)) { 
					char *nse1 = ns_qualifiedElement(p->info.typ);
					char *nsa1 = ns_qualifiedAttribute(p->info.typ);
					if(nse1)
						nse = nse1;
					if(nsa1)
						nsa = nsa1;
				}
				*/
			      if(!is_invisible(p->sym->name)) {
				      if(yflag)
					      fprintf(fd, "%*s<!-- %s -->\n", n, "", c_type_id(p->info.typ, p->sym->name));
				      gen_element_begin(fd, n, ns_add(p->sym->name, nse), xsi_type_u(p->info.typ));
				      gen_atts(fd, n, (Table *)p->info.typ->ref, nsa);
			      }
			      else if(is_anyType(p->info.typ))
				      fprintf(fd, "%*s<!-- extensibility element(s) -->\n", n, "");
		      }
		      else if((p->info.typ->type == Tpointer || p->info.typ->type == Treference) &&
			      (((Tnode *)p->info.typ->ref)->type == Tstruct || ((Tnode *)p->info.typ->ref)->type == Tclass)) { 
					/*
					if(!is_primclass(p->info.typ->ref)) { 
						char *nse1 = ns_qualifiedElement(p->info.typ->ref);
						char *nsa1 = ns_qualifiedAttribute(p->info.typ->ref);
						if(nse1)
							nse = nse1;
						if(nsa1)
							nsa = nsa1;
					}
					*/
			      if(!is_invisible(p->sym->name)) {
				      if(yflag)
					      fprintf(fd, "%*s<!-- %s -->\n", n, "", c_type_id(p->info.typ, p->sym->name));
				      gen_element_begin(fd, n, ns_add(p->sym->name, nse), xsi_type_u(p->info.typ));
				      gen_atts(fd, n, (Table *)((Tnode *)p->info.typ->ref)->ref, nsa);
			      }
			      else if(is_anyType(p->info.typ))
				      fprintf(fd, "%*s<!-- extensibility element(s) -->\n", n, "");
		      }
		      else if(p->info.typ->type != Tunion) {
			      if(!is_invisible(p->sym->name)) {
				      if(yflag)
					      fprintf(fd, "%*s<!-- %s -->\n", n, "", c_type_id(p->info.typ, p->sym->name));
				      gen_element_begin(fd, n, ns_add(p->sym->name, nse), xsi_type_u(p->info.typ));
				      if(p->info.typ->type == Ttemplate) {
					      if(((Tnode *)p->info.typ->ref)->type == Tpointer &&
					         (((Tnode *)((Tnode *)p->info.typ->ref)->ref)->type == Tclass || ((Tnode *)((Tnode *)p->info.typ->ref)->ref)->type == Tstruct))
						      gen_atts(fd, n, (Table *)((Tnode *)((Tnode *)p->info.typ->ref)->ref)->ref, nsa);
					      else if(((Tnode *)p->info.typ->ref)->type == Tclass || ((Tnode *)p->info.typ->ref)->type == Tstruct)
						      gen_atts(fd, n, (Table *)((Tnode *)p->info.typ->ref)->ref, nsa);
					      else
						      fprintf(fd, ">");
				      }
				      else
					      fprintf(fd, ">");
			      }
		      }
		      switch(p->info.typ->type) {
			  case Tchar:
			  case Tshort:
			  case Tint:
			  case Tlong:
			  case Tllong:
			  case Tuchar:
			  case Tushort:
			  case Tuint:
			  case Tulong:
			  case Tullong:
			      if(p->info.hasval)
				      fprintf(fd, SOAP_LONG_FORMAT, p->info.val.i);
			      else
				      fprintf(fd, "0");
			      break;
			  case Tfloat:
			  case Tdouble:
			  case Tldouble:
			      if(p->info.hasval)
				      fprintf(fd, "%f", p->info.val.r);
			      else
				      fprintf(fd, "0.0");
			      break;
			  case Ttime:
			  { char tmp[256];
			    time_t t = time(NULL), * p = &t;
			    strftime(tmp, 256, "%Y-%m-%dT%H:%M:%SZ", gmtime(p));
			    fprintf(fd, "%s", tmp); }
			                            break;
			  case Tenum:
			      if(p->info.hasval && p->info.typ->ref) {
				      for(q = ((Table *)p->info.typ->ref)->list; q; q = q->next)
					      if(p->info.val.i == q->info.val.i) {
						      fprintf(fd, "%s", ns_remove2(q->sym->name));
						      break;
					      }
			      }
			      else
				      gen_val(fd, n+1, p->info.typ, nse, nsa, encoding);
			      break;
			  case Tpointer:
			  case Treference:
			      if(is_string(p->info.typ) || is_wstring(p->info.typ)) {
				      if(p->info.hasval)
					      fprintf(fd, "%s", xstring(p->info.val.s));
				      else
					      gen_val(fd, n, p->info.typ, nse, nsa, encoding);
			      }
			      else
				      gen_val(fd, n, (Tnode *)p->info.typ->ref, nse, nsa, encoding);
			      break;
			  case Tclass:
			      if(is_stdstr(p->info.typ)) {
				      if(p->info.hasval)
					      fprintf(fd, "%s", xstring(p->info.val.s));
				      else
					      gen_val(fd, n, p->info.typ, nse, nsa, encoding);
				      break;
			      }
			  case Tstruct:
			      if(!is_dynamic_array(p->info.typ))
				      gen_val(fd, n, p->info.typ, nse, nsa, encoding);
			      break;
			  case Tunion:
			      gen_val(fd, n, p->info.typ, nse, nsa, encoding);
			      break;
			  case Ttemplate:
			      i = p->info.maxOccurs;
			      if(i <= 1 || i > 4)
				      i = p->info.minOccurs;
			      if(i <= 1)
				      i = 2;
			      do { /* a bit of a hack, I don't like the copy of the code above */
				      { 
						  gen_val(fd, n, p->info.typ, nse, nsa, encoding);
					if(i > 1) {
						gen_element_end(fd, 0, ns_add(p->sym->name, nse));
						if(!is_invisible(p->sym->name)) {
							if(yflag)
								fprintf(fd, "%*s<!-- %s -->\n", n, "", c_type_id(p->info.typ, p->sym->name));
							gen_element_begin(fd, n, ns_add(p->sym->name, nse), xsi_type_u(p->info.typ));
							if(p->info.typ->type == Ttemplate) {
								if(((Tnode *)p->info.typ->ref)->type == Tpointer &&
								   (((Tnode *)((Tnode *)p->info.typ->ref)->ref)->type == Tclass || ((Tnode *)((Tnode *)p->info.typ->ref)->ref)->type == Tstruct))
									gen_atts(fd, n, (Table *)((Tnode *)((Tnode *)p->info.typ->ref)->ref)->ref, nsa);
								else if(((Tnode *)p->info.typ->ref)->type == Tclass || ((Tnode *)p->info.typ->ref)->type == Tstruct)
									gen_atts(fd, n, (Table *)((Tnode *)p->info.typ->ref)->ref, nsa);
								else
									fprintf(fd, ">");
							}
							else
								fprintf(fd, ">");
						}
					}
					fflush(fd); }
			      } while(--i);
			      break;
			  default:
			      break;
		      }
		      if(p->info.typ->type != Tunion)
			      gen_element_end(fd, 0, ns_add(p->sym->name, nse));
		      fflush(fd); }
	}
}

void gen_atts(FILE * fd, int n, Table * t, char * nsa)
{
	static unsigned long idnum = 0;
	Entry * q, * r;
	Tnode * p;
	int i;
	for(; t; t = t->prev) {
		for(q = t->list; q; q = q->next) {
			if(q->info.sto&Sattribute && !is_invisible(q->sym->name) && q->info.maxOccurs !=
			   0)                                   {
				fprintf(fd, " %s=\"", ns_add(q->sym->name, nsa));
				if((q->info.typ->type == Tpointer ||
				    q->info.typ->type == Treference) && !is_string(q->info.typ))
					p = (Tnode *)q->info.typ->ref;
				else
					p = q->info.typ;
				if(is_eq(q->sym->name, "id"))
					fprintf(fd, "%lu", ++idnum);  /* id="#" should be unique */
				else
					switch(p->type) {
					    case Tchar:
					    case Tshort:
					    case Tint:
					    case Tlong:
					    case Tllong:
					    case Tuchar:
					    case Tushort:
					    case Tuint:
					    case Tulong:
					    case Tullong:
						if(q->info.hasval)
							fprintf(fd, SOAP_LONG_FORMAT, q->info.val.i);
						else
							fprintf(fd, "0");
						break;
					    case Tfloat:
					    case Tdouble:
					    case Tldouble:
						if(q->info.hasval)
							fprintf(fd, "%f", q->info.val.r);
						else
							fprintf(fd, "0.0");
						break;
					    case Ttime:
							{ 
								char tmp[256];
								time_t T = time(NULL);
								strftime(tmp, 256, "%Y-%m-%dT%H:%M:%SZ", gmtime(&T));
								fprintf(fd, "%s", tmp); 
							}
							break;
					    case Tenum:
						if(q->info.hasval && p->ref) {
							for(r = ((Table *)p->ref)->list; r; r = r->next) {
								if(r->info.val.i == q->info.val.i) {
									fprintf(fd, "%s", ns_remove2(r->sym->name));
									break;
								}
							}
						}
						else if(p->ref)
							fprintf(fd, "%s", ns_remove2((((Table *)p->ref)->list)->sym->name));
						else
							fprintf(fd, "0");
						break;
					    case Tpointer:
					    case Treference:
						if(is_string(p)) {
							if(q->info.hasval)
								fprintf(fd, "%s", xstring(q->info.val.s));
							else if(q->info.typ->minLength > 0 && q->info.typ->minLength < 10000)
								for(i = 0; i < (int)q->info.typ->minLength; i++)
									fprintf(fd, "X");
						}
						break;
					    case Tclass:
						if(is_stdstr(p)) {
							if(q->info.hasval)
								fprintf(fd, "%s", xstring(q->info.val.s));
							else if(q->info.typ->minLength > 0 && q->info.typ->minLength < 10000)
								for(i = 0; i < (int)q->info.typ->minLength; i++)
									fprintf(fd, "X");
						}
						break;
					    default:
						break;
					}
				if(yflag)
					fprintf(fd, " // %s //", c_type_id(q->info.typ, q->sym->name));
				fprintf(fd, "\"");
			}
		}
	}
	fprintf(fd, ">");
	fflush(fd);
}

void gen_val(FILE * fd, int n, Tnode * p, char * nse, char * nsa, char * encoding)
{
	Entry * q;
	LONG64 i;
	if(!is_transient(p) && p->type != Tfun && !is_XML(p)) {
		if(is_fixedstring(p)) {
			for(i = 0; i < p->width/((Tnode *)p->ref)->width-1; i++)
				fprintf(fd, "X");
		}
		else if(p->type == Tarray) {
			i = ((Tnode *)p->ref)->width;
			if(i) {
				i = p->width/i;
				if(i > 4)
					i = 2;
				fprintf(fd, "\n");
				for(; i > 0; i--) {
					fprintf(fd, "%*s<item>", n+1, "");
					gen_val(fd, n+1, (Tnode *)p->ref, nse, nsa, encoding);
					fprintf(fd, "</item>\n");
				}
				fprintf(fd, "%*s", n, "");
			}
		}
		else if(is_dynamic_array(p)) {
			if(!is_binary(p))                               {
				Table * t;
				fprintf(fd, "\n");
				for(t = (Table *)p->ref; t && !t->list; t = t->prev)
					;
				if(t)
					gen_field(fd, n+1, t->list, nse, nsa, encoding);
				fprintf(fd, "%*s", n, "");
			}
		}
		switch(p->type) {
		    case Tchar:
		    case Tshort:
		    case Tint:
		    case Tlong:
		    case Tllong:
		    case Tuchar:
		    case Tushort:
		    case Tuint:
		    case Tulong:
		    case Tullong:
			fprintf(fd, "0");
			break;
		    case Tfloat:
		    case Tdouble:
		    case Tldouble:
			fprintf(fd, "0.0");
			break;
		    case Ttime:
				{ 
					char tmp[256];
					time_t T = time(NULL);
					strftime(tmp, 256, "%Y-%m-%dT%H:%M:%SZ", gmtime(&T));
					fprintf(fd, "%s", tmp); 
				}
				break;
		    case Tenum:
			if(p->ref && (q = ((Table *)p->ref)->list))
				fprintf(fd, "%s", ns_remove2(q->sym->name));
			else
				fprintf(fd, "0");
			break;
		    case Tpointer:
		    case Treference:
			if(is_string(p) || is_wstring(p)) {
				if(p->minLength > 0 && p->minLength < 10000)
					for(i = 0; i < (int)p->minLength; i++)
						fprintf(fd, "X");
			}
			else
				gen_val(fd, n, (Tnode *)p->ref, nse, nsa, encoding);
			break;
		    case Tclass:
		    case Tstruct:
			nse = ns_qualifiedElement(p);
			nsa = ns_qualifiedAttribute(p);
			if(is_stdstr(p)) {
				if(p->minLength > 0 && p->minLength < 10000)
					for(i = 0; i < (int)p->minLength; i++)
						fprintf(fd, "X");
			}
			else if(is_primclass(p)) {
				Table * t;
				for(t = (Table *)p->ref; t; t = t->prev) {
					Entry * r = entry(classtable, t->sym);
					r = t->list;
					if(r && is_item(r)) {
						gen_val(fd, n, r->info.typ, nse, nsa, encoding);
						return;
					}
				}
			}
			else if(!is_dynamic_array(p) && p->ref) {
				Table * t;
				fprintf(fd, "\n");
				for(t = (Table *)p->ref; t; t = t->prev) {
					for(q = t->list; q; q = q->next) {
						if(is_repetition(q))
						{
							i = q->info.maxOccurs;
							if(i <= 1 || i > 4)
								i = q->info.minOccurs;
							if(i <= 1)
								i = 2;
							do
								gen_field(fd, n+1, q->next, nse, nsa, encoding);
							while(--i);
							q = q->next;
						}
						else
							gen_field(fd, n+1, q, nse, nsa, encoding);
					}
				}
				fprintf(fd, "%*s", n, "");
			}
			break;
		    case Tunion:
			if(((Table *)p->ref)->list)
				gen_field(fd, n, ((Table *)p->ref)->list, nse, nsa, encoding);
			break;
		    case Ttemplate:
			gen_val(fd, n, (Tnode *)p->ref, nse, nsa, encoding);
			break;
		    default:
			break;
		}
	}
}

void gen_header(FILE * fd, char * method, int response, char * encoding)
{
	if(custom_header) {
		Service * sp;
		Method * m = NULL;
		Entry * q;
		Table * r;
		if(yflag) {
			if(cflag)
				fprintf(fd, " <!-- struct SOAP_ENV__Header -->\n");
			else
				fprintf(fd, " <!-- SOAP_ENV__Header *soap::header -->\n");
		}
		fprintf(fd, " <%s:Header>\n", S_SoapEnvNs);
		q = entry(classtable, lookup("SOAP_ENV__Header"));
		if(q) {
			r = (Table *)q->info.typ->ref;
			if(r) {
				for(q = r->list; q; q = q->next) {
					if(!is_transient(q->info.typ) && !(q->info.sto&Sattribute) && q->info.typ->type != Tfun) {
						for(sp = services; sp; sp = sp->next)
							for(m = sp->list; m; m = m->next)
								if(is_eq(m->name, method) && (sstreq(m->part, q->sym->name) || is_eq_nons(m->part, q->sym->name)) &&
								   ((!response && (m->mess&HDRIN)) || (response && (m->mess&HDROUT)))) {
									gen_field(fd, 2, q, NULL, NULL, encoding);
									break;
								}
					}
				}
				fprintf(fd, " </%s:Header>\n", S_SoapEnvNs);
			}
		}
	}
}

FILE * gen_env(char * buf, char * method, int response, Table * t, char * ns, char * name, char * URL,
	char * executable, char * URI, char * encoding)
{
	Symbol * s;
	Service * sp = NULL;
	char tmp[1024];
	FILE * fd;
	strcpy(tmp, buf);
#ifdef __vms
	if(!response) {
		sprintf(strrchr(tmp, '.'), "_%s_req.xml", method);
		fprintf(fmsg, "Saving %s sample SOAP/XML request\n", tmp);
	}
	else {
		sprintf(strrchr(tmp, '.'), "_%s_res.xml", method);
		fprintf(fmsg, "Saving %s sample SOAP/XML response\n", tmp); 
	}
#else
	strcpy(strrchr(tmp, '.')+1, method);
	if(!response) {
		strcat(tmp, ".req.xml");
		fprintf(fmsg, "Saving %s sample SOAP/XML request\n", tmp);
	}
	else {
		strcat(tmp, ".res.xml");
		fprintf(fmsg, "Saving %s sample SOAP/XML response\n", tmp); 
	}
#endif
	fd = fopen(tmp, "w");
	if(!fd)
		execerror("Cannot write XML file");
	fprintf(fd, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	fprintf(fd, "<%s:Envelope", S_SoapEnvNs);
	for(s = nslist; s; s = s->next) {
		for(sp = services; sp; sp = sp->next)
			if(!tagcmp(sp->ns, s->name) && sp->URI)
				break;
		if(sp)
			fprintf(fd, "\n xmlns:%s=\"%s\"", ns_convert(s->name), sp->URI);
		else if(sstreq(s->name, S_SoapEnvNs))
			fprintf(fd, "\n xmlns:%s=\"%s\"", S_SoapEnvNs, envURI);
		else if(sstreq(s->name, S_SoapEncNs))
			fprintf(fd, "\n xmlns:%s=\"%s\"", S_SoapEncNs, encURI);
		else if(sstreq(s->name, "xsi"))
			fprintf(fd, "\n xmlns:xsi=\"%s\"", xsiURI);
		else if(sstreq(s->name, "xsd"))
			fprintf(fd, "\n xmlns:xsd=\"%s\"", xsdURI);
		else
			fprintf(fd, "\n xmlns:%s=\"%s/%s.xsd\"", ns_convert(s->name), tmpURI, ns_convert(s->name));
	}
	fprintf(fd, ">\n");
	gen_header(fd, method, response, encoding);
	fprintf(fd, " <%s:Body", S_SoapEnvNs);
	if(eflag && !encoding)
		fprintf(fd, " %s:encodingStyle=\"%s\"", S_SoapEnvNs, encURI);
	else if(encoding && !*encoding)
		fprintf(fd, " %s:encodingStyle=\"%s\"", S_SoapEnvNs, encURI);
	else if(encoding && strcmp(encoding, "literal"))
		fprintf(fd, " %s:encodingStyle=\"%s\"", S_SoapEnvNs, encoding);
	fprintf(fd, ">\n");
	return fd;
}

char * emalloc(size_t n)
{
	char * p;
	if((p = (char *)SAlloc::M(n)) == NULL)
		execerror("out of memory");
	return p;
}

void soap_serve(Table * table)
{
	Entry * method, * catch_method;
	if(!Cflag) {
		fprintf(fserver, "\n\nSOAP_FMAC5 int SOAP_FMAC6 %s_serve(struct soap *soap)", nflag ? prefix : "soap");
		fprintf(fserver, "\n{\n#ifndef WITH_FASTCGI\n\tunsigned int k = soap->max_keep_alive;\n#endif\n\tdo {\n\t");
		fprintf(fserver, "\n#ifndef WITH_FASTCGI\n\t\tif(soap->max_keep_alive > 0 && !--k)\n\t\t\tsoap->keep_alive = 0;\n#endif");
		fprintf(fserver, "\n\t\tif(soap_begin_serve(soap))\n\t\t{\tif(soap->error >= SOAP_STOP)\n\t\t\t\tcontinue;\n\t\t\treturn soap->error;\n\t\t}");
		fprintf(fserver, "\n\t\tif(%s_serve_request(soap) || (soap->fserveloop && soap->fserveloop(soap)))\n\t\t{\n#ifdef WITH_FASTCGI\n\t\t\tsoap_send_fault(soap);\n#else\n\t\t\treturn soap_send_fault(soap);\n#endif\n\t\t}",
			nflag ? prefix : "soap");
		fprintf(fserver, "\n\n#ifdef WITH_FASTCGI\n\t\tsoap_destroy(soap);\n\t\tsoap_end(soap);\n\t} while(1);\n#else\n\t} while(soap->keep_alive);\n#endif");
		fprintf(fserver, "\n\treturn SOAP_OK;");
		fprintf(fserver, "\n}");
		fprintf(fserver, "\n\n#ifndef WITH_NOSERVEREQUEST\nSOAP_FMAC5 int SOAP_FMAC6 %s_serve_request(struct soap *soap)\n{", nflag ? prefix : "soap");
		fprintf(fserver, "\n\tsoap_peek_element(soap);");
		catch_method = NULL;
		for(method = table->list; method; method = method->next) {
			char * action = NULL;
			if(method->info.typ->type == Tfun && !(method->info.sto&Sextern)) {
				if(aflag) {
					for(Service * sp = services; sp; sp = sp->next) {
						if(has_ns_eq(sp->ns, method->sym->name)) {
							Method * m;
							for(m = sp->list; m; m = m->next) {
								if(is_eq_nons(m->name, method->sym->name)) {
									if(m->mess == ACTION || m->mess == REQUEST_ACTION)
										action = m->part;
								}
							}
						}
					}
				}
				if(is_invisible(method->sym->name)) {
					Entry * param = entry(classtable, method->sym);
					if(param) {
						param = ((Table *)param->info.typ->ref)->list;
						if(param) {
							if(action)
								if(*action == '"')
									fprintf(fserver, "\n\tif((!soap->action && !soap_match_tag(soap, soap->tag, \"%s\")) || (soap->action && sstreq(soap->action, %s))",
										ns_convert(param->sym->name), action);
								else
									fprintf(fserver, "\n\tif((!soap->action && !soap_match_tag(soap, soap->tag, \"%s\")) || (soap->action && sstreq(soap->action, \"%s\"))",
										ns_convert(param->sym->name), action);
							else
								fprintf(fserver, "\n\tif(!soap_match_tag(soap, soap->tag, \"%s\")", ns_convert(param->sym->name));
							fprintf(fserver, ")\n\t\treturn soap_serve_%s(soap);", ident(method->sym->name));
						}
						else
							catch_method = method;
					}
					else
						catch_method = method;
				}
				else {
					if(action)
					      if(*action == '"')
						      fprintf(fserver,
							      "\n\tif((!soap->action && !soap_match_tag(soap, soap->tag, \"%s\")) || (soap->action && sstreq(soap->action, %s))",
							      ns_convert(method->sym->name), action);
					      else
						      fprintf(fserver,
							      "\n\tif((!soap->action && !soap_match_tag(soap, soap->tag, \"%s\")) || (soap->action && sstreq(soap->action, \"%s\"))",
							      ns_convert(method->sym->name), action);
				      else
					      fprintf(fserver, "\n\tif(!soap_match_tag(soap, soap->tag, \"%s\")", ns_convert(method->sym->name));
				      fprintf(fserver, ")\n\t\treturn soap_serve_%s(soap);", ident(method->sym->name)); }
			}
		}
		if(catch_method)
			fprintf(fserver, "\n\treturn soap_serve_%s(soap);", ident(catch_method->sym->name));
		else
			fprintf(fserver, "\n\treturn soap->error = SOAP_NO_METHOD;");
		fprintf(fserver, "\n}\n#endif");

		banner(fheader, "Server-Side Operations");
		for(method = table->list; method; method = method->next)
			if(method->info.typ->type == Tfun && !(method->info.sto&Sextern))
				generate_proto(table, method);
		banner(fheader, "Server-Side Skeletons to Invoke Service Operations");
		fprintf(fheader, "\nSOAP_FMAC5 int SOAP_FMAC6 %s_serve(struct soap*);", nflag ? prefix : "soap");
		fprintf(fheader, "\n\nSOAP_FMAC5 int SOAP_FMAC6 %s_serve_request(struct soap*);",
			nflag ? prefix : "soap");
		for(method = table->list; method; method = method->next)
			if(method->info.typ->type == Tfun && !(method->info.sto&Sextern) && !is_imported(method->info.typ))
				gen_serve_method(fserver, table, method, NULL);
	}
	if(!Sflag) {
		banner(fheader, "Client-Side Call Stubs");
		for(method = table->list; method; method = method->next)
			if(method->info.typ->type == Tfun && !(method->info.sto&Sextern) && !is_imported(method->info.typ))
				gen_call_method(fclient, table, method, NULL);
	}
}

void generate_proto(Table * table, Entry * param)
{
	Entry * q = entry(table, param->sym);
	if(q) {
		Entry * pout = (Entry *)q->info.typ->ref;

		q = entry(classtable, param->sym);
		Table * output = (Table *)q->info.typ->ref;
		fprintf(fheader, "\n\nSOAP_FMAC5 int SOAP_FMAC6 %s(struct soap*", ident(param->sym->name));
		gen_params(fheader, output, pout, 1);
		fprintf(fheader, ";");
	}
	else
		fprintf(stderr, "Internal error: no table entry\n");
}

int tagcmp(const char * s, const char * t)
{
	size_t i;
	size_t n = strlen(s);
	for(i = 0; i < n; i++) {
		int c = t[i];
		if(c == '_' && s[i] != '_')
			c = '-';
		if(s[i] > c)
			return 1;
		if(s[i] < c)
			return -1;
	}
	return -(t[i] != 0);
}

int tagncmp(const char * s, const char * t, size_t n)
{
	for(size_t i = 0; i < n; i++) {
		int c = t[i];
		if(c == '_' && s[i] != '_')
			c = '-';
		if(s[i] > c)
			return 1;
		if(s[i] < c)
			return -1;
	}
	return 0;
}

int is_qname(Tnode * p)
{
	if(p->sym && is_string(p) && (is_eq(p->sym->name, "xsd__QName") || is_eq(p->sym->name, "QName")))
		return 1;
	return p->id && is_string(p) && (is_eq(p->id->name, "xsd__QName") || is_eq(p->id->name, "QName"));
}

int is_stdqname(Tnode * p)
{
	if(p->sym && p->type == Tclass && is_volatile(p) && (is_eq(p->sym->name, "xsd__QName") || is_eq(p->sym->name, "QName")))
		return 1;
	return p->id && p->type == Tclass && is_volatile(p) && (is_eq(p->id->name, "xsd__QName") || is_eq(p->id->name, "QName"));
}

int is_XML(Tnode * p)
{
	return (p->sym && (is_string(p) || is_wstring(p)) && is_eq(p->sym->name, "XML")) || (oneof2(p->type, Tpointer, Treference) && is_XML((Tnode *)p->ref));
}

int is_stdXML(Tnode * p)
{
	return p->sym && (is_stdstring(p) || is_stdwstring(p)) && is_eq(p->sym->name, "XML");
}

int is_response(Tnode * p)
{
	return (p->type == Tpointer || p->type == Treference) && p->ref && has_ns((Tnode *)p->ref) &&
		((((Tnode *)p->ref)->type == Tstruct || ((Tnode *)p->ref)->type == Tclass) && !is_primclass((Tnode *)p->ref) &&
		!is_dynamic_array((Tnode *)p->ref) && !is_stdstring((Tnode *)p->ref) && !is_stdwstring((Tnode *)p->ref));
}

Entry * get_response(Tnode * p)
{
	return (p->type == Tfun) ? p->response : 0;
}

int is_unmatched(Symbol * sym)
{
	return sym->name[0] == '_' && sym->name[1] != '_' && strncmp(sym->name, "_DOT", 4) &&
		strncmp(sym->name, "_USCORE", 7) && (strncmp(sym->name, "_x", 2) || !ishex(sym->name[2]) || !ishex(sym->name[3]) || !ishex(sym->name[4]) || !ishex(sym->name[5]));
}

int is_invisible(const char * name)
{
	return name[0] == '-' || (name[0] == '_' && name[1] == '_' && strncmp(name, "__ptr", 5));
}

int is_invisible_empty(Tnode * p)
{
	if(p->type == Tstruct || p->type == Tclass)
		if(is_invisible(p->id->name))
			if(!p->ref || !((Table *)p->ref)->list)
				return 1;
	return 0;
}

int is_element(Tnode * typ)
{
	if(is_XML(typ) || is_stdXML(typ) || is_qname(typ) || is_stdqname(typ))
		return 0;
	if(typ->sym)
		return is_unmatched(typ->sym);
	if(typ->type == Tstruct || typ->type == Tclass)
		return is_unmatched(typ->id);
	return 0;
}

int is_untyped(Tnode * typ)
{
	Tnode * p;
	if(typ->sym)
		return is_unmatched(typ->sym);
	if(typ->type == Tpointer || typ->type == Treference || typ->type == Tarray)
		return is_untyped((Tnode *)typ->ref);
	if(typ->type == Tstruct || typ->type == Tclass) {
		if(is_dynamic_array(typ) && !has_ns(typ) && !is_binary(typ)) {
			p = (Tnode *)((Table *)typ->ref)->list->info.typ->ref;
			return is_untyped(p);
		}
		else
			return is_unmatched(typ->id);
	}
	return 0;
}

int is_primclass(Tnode * typ)
{
	Table * t;
	if(typ->type == Tstruct || typ->type == Tclass) {
		if(!is_dynamic_array(typ))                                                  {
			t = (Table *)typ->ref;
			while(t) {
				if(t->list && is_item(t->list))
					break;
				t = t->prev;
			}
			if(!t)
				return 0;
			t = (Table *)typ->ref;
			while(t) {
				Entry * p;
				for(p = t->list; p; p = p->next)
					if(!is_item(p))
						if(p->info.typ->type != Tfun && !is_transient(p->info.typ) && p->info.sto != Sattribute && p->info.sto != Sprivate && p->info.sto != Sprotected)
							return 0;
				t = t->prev;
			}
			return 1;
		}
	}
	else if(typ->type == Tpointer || typ->type == Treference)
		return is_primclass((Tnode *)typ->ref);
	return 0;
}

int is_mask(Tnode * typ)
{
	return typ->type == Tenum && typ->width == 8;
}

int is_void(Tnode * typ)
{
	if(!typ)
		return 1;
	else if(typ->type == Tvoid)
		return 1;
	else if(typ->type == Tpointer)
		return is_void((Tnode *)typ->ref);
	else if(typ->type == Treference)
		return is_void((Tnode *)typ->ref);
	else if(typ->type == Tarray)
		return is_void((Tnode *)typ->ref);
	else if(typ->type == Ttemplate)
		return is_void((Tnode *)typ->ref);
	else 
		return 0;
}

int is_transient(Tnode * typ)
{
	if(!typ)
		return 1;
	else if(typ->type == Tstruct && typ->id == lookup("soap"))
		return 1;
	else if(is_external(typ) || is_volatile(typ))
		return 0;
	else if(typ->transient)
		return 1;
	else {
		switch(typ->type) {
			case Tpointer:
			case Treference:
			case Tarray:
			case Ttemplate: return is_transient((Tnode *)typ->ref);
			case Tnone:
			case Tvoid: return 1;
			default: break;
		}
		return 0;
	}
}

int is_imported(Tnode * typ)
{
	return typ->imported != NULL;
}

int is_external(Tnode * typ)
{
	return typ->transient == -1;
}

int is_anyType(Tnode * typ)
{
	if(typ->type == Tpointer)
		return is_anyType((Tnode *)typ->ref);
	return is_external(typ) && typ->type == Tstruct && sstreq(typ->id->name, "soap_dom_element");
}

int is_anyAttribute(Tnode * typ)
{
	if(typ->type == Tpointer)
		return is_anyAttribute((Tnode *)typ->ref);
	return is_external(typ) && typ->type == Tstruct && sstreq(typ->id->name, "soap_dom_attribute");
}

int is_volatile(Tnode * typ)
{
	return typ->transient == -2;
}

int is_template(Tnode * p)
{
	return (p->type == Tpointer) ? is_template((Tnode *)p->ref) : p->type == Ttemplate;
}

int is_repetition(Entry * p)
{
	if(p)
		return p->next && p->next->info.typ->type == Tpointer && oneof2(p->info.typ->type, Tint, Tuint) && ((p->info.sto&Sspecial) || !strncmp(p->sym->name, "__size", 6));
	return 0;
}

int is_item(Entry * p)
{
	if(p)
		return sstreq(p->sym->name, "__item");
	return 0;
}

int is_choice(Entry * p)
{
	if(p)
		if(p->next && p->next->info.typ->type == Tunion && p->info.typ->type == Tint && ((p->info.sto&Sspecial) || !strncmp(p->sym->name, "__union", 7)))
			return 1;
	return 0;
}

int is_sequence(Entry * p)
{
	if(p) {
		Tnode * q = p->info.typ;
		if(q->type == Tpointer)
			q = (Tnode *)q->ref;
		if(q->type == Tstruct && is_invisible(p->sym->name) && is_invisible(q->id->name) && !is_transient(q))
			return 1;
	}
	return 0;
}

int is_anytype(Entry * p)
{
	if(p)
		if(p->next && p->next->info.typ->type == Tpointer && ((Tnode *)p->next->info.typ->ref)->type ==
		   Tvoid && p->info.typ->type == Tint && !strncmp(p->sym->name, "__type", 6)) {
			is_anytype_flag = 1;
			return 1;
		}
	return 0;
}

int is_keyword(const char * name)
{
	Symbol * s = lookup(name);
	if(s)
		return s->token != ID;
	return 0;
}

int has_ptr(Tnode * typ)
{
	Tnode * p;
	if(typ->type == Tpointer || typ->type == Treference)
		return 0;
	for(p = Tptr[Tpointer]; p; p = p->next)
		if((Tnode *)p->ref == typ && p->transient != 1)
			return 1;
	return 0;
}

int has_detail_string(void)
{
	Entry * p = entry(classtable, lookup("SOAP_ENV__Fault"));
	if(p && p->info.typ->ref && (p->info.typ->type == Tstruct || p->info.typ->type == Tclass)) {
		Entry * e = entry((Table *)p->info.typ->ref, lookup("detail"));
		if(e && e->info.typ->ref && e->info.typ->type == Tpointer && ((Tnode *)e->info.typ->ref)->type == Tstruct) {
			Entry * e2 = entry((Table *)((Tnode *)e->info.typ->ref)->ref, lookup("__any"));
			return e2 && is_string(e2->info.typ);
		}
	}
	return 0;
}

int has_Detail_string(void)
{
	Entry * p = entry(classtable, lookup("SOAP_ENV__Fault"));
	if(p && p->info.typ->ref && (p->info.typ->type == Tstruct || p->info.typ->type == Tclass)) {
		Entry * e = entry((Table *)p->info.typ->ref, lookup("SOAP_ENV__Detail"));
		if(e && e->info.typ->ref && e->info.typ->type == Tpointer && ((Tnode *)e->info.typ->ref)->type ==
		   Tstruct) {
			Entry * e2 = entry((Table *)((Tnode *)e->info.typ->ref)->ref, lookup("__any"));
			return e2 && is_string(e2->info.typ);
		}
	}
	return 0;
}

int has_class(Tnode * typ)
{
	Entry * p;
	if(typ->type == Tstruct && typ->ref) {
		for(p = ((Table *)typ->ref)->list; p; p = p->next)                                       {
			if(p->info.sto&Stypedef)
				continue;
			if(p->info.typ->type == Tclass || p->info.typ->type == Ttemplate)
				return 1;
			if(p->info.typ->type == Tstruct && has_class(p->info.typ))
				return 1;
		}
	}
	return 0;
}

int has_external(Tnode * typ)
{
	Entry * p;
	if((typ->type == Tstruct || typ->type == Tclass) && typ->ref) {
		for(p = ((Table *)typ->ref)->list; p; p =
		            p->next)                                                                {
			if(p->info.typ->type == Tstruct || p->info.typ->type ==
			   Tclass)
			{
				if(is_external(p->info.typ) || has_external(p->info.typ))
					return 1;
			}
		}
	}
	return 0;
}

int has_volatile(Tnode * typ)
{
	if(oneof2(typ->type, Tstruct, Tclass) && typ->ref) {
		for(Entry * p = ((Table *)typ->ref)->list; p; p = p->next) {
			if(p->info.typ->type == Tstruct || p->info.typ->type == Tclass) {
				if(is_volatile(p->info.typ) || has_volatile(p->info.typ))
					if(!is_stdstr(p->info.typ))
						return 1;
			}
		}
	}
	return 0;
}

int has_ns(Tnode * typ)
{
	return oneof3(typ->type, Tstruct, Tclass, Tenum) ? has_ns_eq(NULL, typ->id->name) : 0;
}

int has_ns_t(Tnode * typ)
{
	if(typ->sym) {
		char * s = strstr(typ->sym->name+1, "__");
		if(!s) {
			s = strchr(typ->sym->name, ':');
			if(s && s[1] == ':')
				s = NULL;
		}
		return s && s[1] && s[2] && (s[2] != '_' || (s[2] == '_' && s[3] == 'x' && ishex(s[4]) && ishex(s[5]) && ishex(s[6]) && ishex(s[7])) || !strncmp(s+2, "_DOT", 4) || !strncmp(s+2, "_USCORE", 7));
	}
	return has_ns(typ);
}

/* needs_lang adds xml:lang attribute to matching struct/class member name
   we should use an annotation for soapcpp2's input this in the future instead
   of a hard-coded member name */
void needs_lang(Entry * e)
{
	if(sstreq(e->sym->name, "SOAP_ENV__Text"))
		fprintf(fout, "\n\tif(soap->lang)\n\t\tsoap_set_attr(soap, \"xml:lang\", soap->lang, 1);");
}

int is_eq_nons(const char * s, const char * t)
{
	size_t n, m;
	const char * r;
	while(*s == '_' || *s == ':')
		s++;
	while(*t == '_' || *t == ':')
		t++;
	if(!*s || !*t)
		return 0;
	r = strstr(t, "__");
	if(r)
		t = r+2;
	for(n = strlen(s)-1; n && s[n] == '_'; n--)
		;
	for(m = strlen(t)-1; m && t[m] == '_'; m--)
		;
	if(n != m)
		return 0;
	return !strncmp(s, t, n+1);
}

int is_eq(const char * s, const char * t)
{
	size_t n, m;
	while(*s == '_' || *s == ':')
		s++;
	while(*t == '_' || *t == ':')
		t++;
	if(!*s || !*t)
		return 0;
	for(n = strlen(s)-1; n && s[n] == '_'; n--)
		;
	for(m = strlen(t)-1; m && t[m] == '_'; m--)
		;
	if(n != m)
		return 0;
	return !strncmp(s, t, n+1);
}

int has_ns_eq(char * ns, char * s)
{
	size_t n;
	while(*s == '_' || *s == ':')
		s++;
	if(!ns) {
		char * t = strstr(s+1, "__");
		if(!t ||
		   (t[2] == 'x' && ishex(t[3]) && ishex(t[4]) && ishex(t[5]) && ishex(t[6])) || !strncmp(t+2, "DOT", 3) || !strncmp(t+2, "USCORE", 6)) {
			t = strchr(s, ':');
			if(t && t[1] == ':')
				t = NULL;
		}
		return t && t[1] && t[2] && t[2] != '_';
	}
	if((n = strlen(ns)) < strlen(s))
		return ((s[n] == '_' && s[n+1] == '_') || (s[n] == ':' && s[n+1] != ':')) && !tagncmp(ns, s, n);
	return 0;
}

char * strict_check(void)
{
	if(sflag)
		return "";
	return "(soap->mode & SOAP_XML_STRICT) && ";
}

char * ns_of(char * name)
{
	Service * sp;
	for(sp = services; sp; sp = sp->next)
		if(has_ns_eq(sp->ns, name))
			break;
	if(sp)
		return sp->URI;
	return NULL;
}

int eq_ns(char * s, char * t)
{
	return ns_of(s) == ns_of(t);
}

char * prefix_of(char * s)
{
	char * t;
	while(*s == '_' || *s == ':')
		s++;
	t = strstr(s+1, "__");
	if(!t) {
		t = strchr(s, ':');
		if(t && t[1] == ':')
			t = NULL;
	}
	if(t && t[1] && t[2] && t[2] != '_') {
		char * r  = static_cast<char *>(emalloc(t-s+1));
		strncpy(r, s, t-s);
		r[t-s] = '\0';
		return r;
	}
	return s;
}

char * ns_add_overridden(Table * t, Entry * p, char * ns)
{
	Entry * q;
	Symbol * s = t->sym;
	if(s)
		do {
			for(q = t->list; q; q = q->next)
			    if(sstreq(q->sym->name, p->sym->name))
				    return ns_add(p->sym->name, ns ? prefix_of(t->sym->name) : NULL);
		} while((t = t->prev) != NULL);
	return ns_add(p->sym->name, ns);
}

char * c_ident(Tnode * typ)
{
	return (typ->sym && strcmp(typ->sym->name, "/*?*/")) ? res_remove(typ->sym->name) : t_ident(typ);
}

char * soap_type(Tnode * typ)
{
	char * s, * t = c_ident(typ);
	if(namespaceid) {
		s = static_cast<char *>(emalloc(strlen(t)+strlen(namespaceid)+12));
		strcpy(s, "SOAP_TYPE_");
		strcat(s, namespaceid);
		strcat(s, "_");
	}
	else {
		s = static_cast<char *>(emalloc(strlen(t)+11));
		strcpy(s, "SOAP_TYPE_"); 
	}
	strcat(s, t);
	return s;
}

void utf8(char ** t, long c)
{
	if(c < 0x0080)
		*(*t)++ = (char)c;
	else {
		if(c < 0x0800)
			*(*t)++ = (char)(0xC0|((c>>6)&0x1F));
		else {
			if(c < 0x010000)
				*(*t)++ = (char)(0xE0|((c>>12)&0x0F));
			else {
				if(c < 0x200000)
					*(*t)++ = (char)(0xF0|((c>>18)&0x07));
				else {
					if(c < 0x04000000)
						*(*t)++ = (char)(0xF8|((c>>24)&0x03));
					else {
						*(*t)++ = (char)(0xFC|((c>>30)&0x01));
						*(*t)++ = (char)(0x80|((c>>24)&0x3F)); 
					}
					*(*t)++ = (char)(0x80|((c>>18)&0x3F)); 
				}
				*(*t)++ = (char)(0x80|((c>>12)&0x3F)); 
			}
			*(*t)++ = (char)(0x80|((c>>6)&0x3F)); 
		}
		*(*t)++ = (char)(0x80|(c&0x3F)); 
	}
	*(*t) = '\0';
}

char * ns_convert(char * tag)
{
	char * t, * s;
	size_t i, n;
	if(*tag == '_') {
		if(!strncmp(tag, "__ptr", 5)) {
			if(tag[5])
				tag += 5;
			else
				tag = "item";
		}
		else if(strncmp(tag, "_DOT", 4) && strncmp(tag, "_USCORE", 7) && (strncmp(tag, "_x", 2) || !ishex(tag[2]) || !ishex(tag[3]) || !ishex(tag[4]) || !ishex(tag[5])))
			tag++;  /* skip leading _ */
	}
	for(n = strlen(tag); n > 0; n--) {
		if(tag[n-1] != '_')
			break;
	}
	s = t = static_cast<char *>(emalloc(n+1));
	for(i = 0; i < n; i++) {
		if(tag[i] == '_') {
			if(tag[i+1] == '_' && !(tag[i+2] == 'x' && ishex(tag[i+3]) && ishex(tag[i+4]) && ishex(tag[i+5]) && ishex(tag[i+6])))
				break;
			else if(!strncmp(tag+i, "_DOT", 4)) {
				*s++ = '.';
				i += 3;
			}
			else if(!strncmp(tag+i, "_USCORE", 7)) {
				*s++ = '_';
				i += 6;
			}
			else if(!strncmp(tag+i, "_x", 2) && ishex(tag[i+2]) && ishex(tag[i+3]) && ishex(tag[i+4]) && ishex(tag[i+5])) {
				char d[5];
				strncpy(d, tag+i+2, 4);
				d[4] = '\0';
				utf8(&s, strtoul(d, NULL, 16));
				i += 5;
			}
			else
				*s++ = '-';
		}
		else if(tag[i] == ':' && tag[i+1] == ':')
			break;
		else
			*s++ = tag[i];
	}
	if(i < n) {
		*s++ = ':';
		for(i += 2; i < n; i++) {
			if(tag[i] == '_') {
				if(!strncmp(tag+i, "_DOT", 4)) {
					*s++ = '.';
					i += 3;
				}
				else if(!strncmp(tag+i, "_USCORE", 7)) {
					*s++ = '_';
					i += 6;
				}
				else if(!strncmp(tag+i, "_x", 2) && ishex(tag[i+2]) && ishex(tag[i+3]) && ishex(tag[i+4]) && ishex(tag[i+5])) {
					char d[5];
					strncpy(d, tag+i+2, 4);
					d[4] = '\0';
					utf8(&s, strtoul(d, NULL, 16));
					i += 5;
				}
				else
					*s++ = '-';
			}
			else
				*s++ = tag[i];
		}
	}
	*s = '\0';
	return t;
}

char * res_remove(char * tag)
{
	char * s, * t;
	if(!(s = strchr(tag, ':')))
		return tag;
	s = emalloc(strlen(tag)+1);
	strcpy(s, tag);
	while((t = strchr(s, ':')))
		*t = '_';
	return s;
}

char * ns_qualifiedElement(Tnode * typ)
{
	Service * sp;
	char * s = NULL;
	if(typ->sym)
		s = prefix_of(typ->sym->name);
	if(!s && typ->id)
		s = prefix_of(typ->id->name);
	if(!s)
		return NULL;
	for(sp = services; sp; sp = sp->next) {
		if(sp->elementForm && !tagcmp(sp->ns, s))                                        {
			if(sstreq(sp->elementForm, "qualified"))
				return s;
			return NULL;
		}
	}
	for(sp = services; sp; sp = sp->next)
		if(!tagcmp(sp->ns, s))
			if(sp->style && sstreq(sp->style, "document"))
				return s;
	return NULL;
}

char * ns_qualifiedAttribute(Tnode * typ)
{
	Service * sp;
	char * s = NULL;
	if(typ->sym)
		s = prefix_of(typ->sym->name);
	if(!s && typ->id)
		s = prefix_of(typ->id->name);
	if(!s)
		return NULL;
	for(sp = services; sp; sp = sp->next) {
		if(sp->attributeForm && !tagcmp(sp->ns, s))                                        {
			if(sstreq(sp->attributeForm, "qualified"))
				return s;
			return NULL;
		}
	}
	return NULL;
}

char * ns_add(char * tag, char * ns)
{
	char * n, * t, * s = ns_convert(tag);
	if(*s == ':')
		return s+1;
	if(!ns || *s == '-' || (t = strchr(s, ':')))
		return s;
	n = ns_convert(ns);
	t = emalloc(strlen(n)+strlen(s)+2);
	strcpy(t, n);
	strcat(t, ":");
	strcat(t, s);
	return t;
}

char * ns_name(char * tag)
{
	char * t, * r, * s = tag;
	if(*s) {
		for(r = s+strlen(s)-1; r > s; r--)
			if(*r != '_')
				break;
		for(t = s+1; t < r; t++) {
			if(t[0] == '_' && t[1] == '_') {
				s = t+2;
				t++;
			}
			else if(t[0] == ':' && t[1] != ':') {
				s = t+1;
				t++;
			}
		}
	}
	return s;
}

char * ns_cname(char * tag, char * suffix)
{
	char * s, * t;
	size_t i, n;
	if(!tag)
		return NULL;
	t = ns_name(tag);
	n = strlen(t);
	if(suffix)
		s = emalloc(n+strlen(suffix)+2);
	else
		s = emalloc(n+2);
	for(i = 0; i < n; i++) {
		if(!isalnum(t[i]))
			s[i] = '_';
		else
			s[i] = t[i];
	}
	s[i] = '\0';
	if(suffix)
		strcat(s, suffix);
	if(is_keyword(t))
		strcat(s, "_");
	return s;
}

char * ns_fname(char * tag)
{
	size_t i;
	char * s = emalloc(strlen(tag)+1);
	strcpy(s, tag);
	for(i = 0; s[i]; i++)
		if(!isalnum(s[i]))
			s[i] = '_';
	return s;
}

char * ns_remove(char * tag)
{
	return ns_convert(ns_name(tag));
}

char * ns_remove1(char * tag)
{
	char * t, * s = tag;
	int n = 2;
	/* handle 'enum_xx__yy' generated by wsdl2h
	   if(!strncmp(s, "enum_", 5))
	   n = 1;
	 */
	if(*s) {
		for(t = s+1; *t && n; t++)
			if(t[0] == '_' && t[1] == '_') {
				s = t+2;
				t++;
				n--;
			}
		if(n || (s[0] == '_' && s[1] != 'x' && strncmp(s, "_USCORE", 7)) || !*s)
			s = tag;
	}
	return s;
}

char * ns_remove2(char * tag)
{
	return ns_convert(ns_remove1(tag));
}

char * xsi_type_cond(Tnode * typ, int flag)
{
	return flag ? xsi_type(typ) : "";
}

char * xsi_type_cond_u(Tnode * typ, int flag)
{
	return flag ? xsi_type_u(typ) : "";
}

char * xsi_type_u(Tnode * typ)
{
	Service * sp;
	char * s = NULL;
	if(tflag)
		return xsi_type(typ);
	if(typ->sym)
		s = prefix_of(typ->sym->name);
	if(!s && typ->id)
		s = prefix_of(typ->id->name);
	if(!s)
		return "";
	s = xsi_type(typ);
	for(sp = services; sp; sp = sp->next)
		if(sp->xsi_type && has_ns_eq(sp->ns, s))
			return s;
	return "";
}

char * xsi_type(Tnode * typ)
{
	if(!typ)
		return "NULL";
	if(is_dynamic_array(typ) && !has_ns(typ))
		return xsi_type_Darray(typ);
	if(typ->type == Tarray)
		return xsi_type_Tarray(typ);
	if(is_untyped(typ))
		return "";
	if(typ->sym) {
		if(!strncmp(typ->sym->name, "SOAP_ENV__", 10))
			return "";
		if(is_XML(typ))
			return "xsd:anyType";
		if(typ->type != Ttemplate)
			return ns_convert(typ->sym->name);
	}
	if(is_string(typ) || is_wstring(typ) || is_stdstring(typ) || is_stdwstring(typ))
		return "xsd:string";
	switch(typ->type) {
	    case Tchar: return "xsd:byte";
	    case Twchar: return "wchar";
	    case Tshort: return "xsd:short";
	    case Tint: return "xsd:int";
	    case Tlong: 
		case Tllong: return "xsd:long";
	    case Tfloat: return "xsd:float";
	    case Tdouble: return "xsd:double";
	    case Tldouble: return "xsd:decimal";
	    case Tuchar: return "xsd:unsignedByte";
	    case Tushort: return "xsd:unsignedShort";
	    case Tuint: return "xsd:unsignedInt";
	    case Tulong:
	    case Tullong: return "xsd:unsignedLong";
	    case Ttime: return "xsd:dateTime";
	    case Tpointer:
	    case Treference: return xsi_type((Tnode *)typ->ref);
	    case Tenum:
			if((Table *)typ->ref == booltable)
				return "xsd:boolean";
	    case Tstruct:
	    case Tclass:
			if(!strncmp(typ->id->name, "SOAP_ENV__", 10))
				return "";
			return ns_convert(typ->id->name);
	    case Ttemplate:
			if((Tnode *)typ->ref)
				return xsi_type((Tnode *)typ->ref);
			break;
	    default:
			break;
	}
	return "";
}

const char * xml_tag(Tnode * typ)
{
	if(!typ)
		return "NULL";
	if(typ->type == Tpointer || typ->type == Treference)
		return xml_tag((Tnode *)typ->ref);
	return typ->sym ? ns_convert(typ->sym->name) : the_type(typ);
}

char * wsdl_type(Tnode * typ, char * ns)
{
	if(!typ)
		return "NULL";
	if((is_qname(typ) || is_stdqname(typ)) && ns)
		return "xsd:QName";
	if(typ->sym) {
		if(is_XML(typ))
			return "xsd:anyType";
		else if(ns)
			return ns_convert(typ->sym->name);
		else
			return ns_remove(typ->sym->name);
	}
	return base_type(typ, ns);
}

char * base_type(Tnode * typ, char * ns)
{
	int d;
	char * s, * t;
	if(is_string(typ) || is_wstring(typ) || is_stdstring(typ) || is_stdwstring(typ)) {
		return ns ? "xsd:string" : "string";
	}
	if(is_dynamic_array(typ) && !is_binary(typ) && !has_ns(typ) && !is_untyped(typ)) {
		s = ns_remove(wsdl_type(((Table *)typ->ref)->list->info.typ, NULL));
		if(ns && *ns) {
			t = static_cast<char *>(emalloc(strlen(s)+strlen(ns_convert(ns))+13));
			strcpy(t, ns_convert(ns));
			strcat(t, ":");
			strcat(t, "ArrayOf");
		}
		else {
			t = static_cast<char *>(emalloc(strlen(s)+12));
			strcpy(t, "ArrayOf"); 
		}
		strcat(t, s);
		d = get_Darraydims(typ);
		if(d)
			sprintf(t+strlen(t), "%dD", d);
		return t;
	}
	switch(typ->type) {
	    case Tchar: return ns ? "xsd:byte" : "byte";
	    case Twchar: return ns ? "xsd:wchar" : "wchar";
	    case Tshort: return ns ? "xsd:short" : "short";
	    case Tint: return ns ? "xsd:int" : "int";
	    case Tlong:
	    case Tllong: return ns ? "xsd:long" : "long";
	    case Tfloat: return ns ? "xsd:float" : "float";
	    case Tdouble: return ns ? "xsd:double" : "double";
	    case Tldouble: return ns ? "xsd:decimal" : "decimal";
	    case Tuchar: return ns ? "xsd:unsignedByte" : "unsignedByte";
	    case Tushort: return ns ? "xsd:unsignedShort" : "unsignedShort";
	    case Tuint: return ns ? "xsd:unsignedInt" : "unsignedInt";
	    case Tulong:
	    case Tullong: return ns ? "xsd:unsignedLong" : "unsignedLong";
	    case Ttime: return ns ? "xsd:dateTime" : "dateTime";
	    case Tpointer:
	    case Treference: return wsdl_type((Tnode *)typ->ref, ns);
	    case Tarray:
		if(is_fixedstring(typ)) {
			if(typ->sym)
				return ns ? ns_convert(typ->sym->name) : ns_remove(typ->sym->name);
			else
				return ns ? "xsd:string" : "string";
		}
		if(ns && *ns) {
			s = static_cast<char *>(emalloc((strlen(ns_convert(ns))+strlen(c_ident(typ))+2)*sizeof(char)));
			strcpy(s, ns_convert(ns));
			strcat(s, ":");
			strcat(s, c_ident(typ));
			return s;
		}
		else
			return c_ident(typ);
	    case Tenum:
		if((Table *)typ->ref == booltable) {
			return ns ? "xsd:boolean" : "boolean";
		}
	    case Tstruct:
	    case Tclass:
		if(!has_ns(typ) && ns && *ns) {
			s = static_cast<char *>(emalloc((strlen(ns_convert(ns))+strlen(typ->id->name)+2)*sizeof(char)));
			strcpy(s, ns_convert(ns));
			strcat(s, ":");
			strcat(s, ns_convert(typ->id->name));
			return s;
		}
		else if(ns)
			return ns_convert(typ->id->name);
		else
			return ns_remove(typ->id->name);
	    case Tunion: return ns ? "xsd:choice" : "choice";
	    case Ttemplate:
			if((Tnode *)typ->ref)
				return wsdl_type((Tnode *)typ->ref, ns);
			break;
	    default:
			break;
	}
	return "";
}

const char * the_type(Tnode * typ)
{
	if(!typ)
		return "NULL";
	if(typ->type == Tarray || (is_dynamic_array(typ) && (eflag || (!has_ns(typ) && !is_untyped(typ)))))
		return "SOAP-ENC:Array";
	if(is_string(typ) || is_wstring(typ) || is_stdstring(typ) || is_stdwstring(typ))
		return "string";
	switch(typ->type) {
	    case Tchar: return "byte";
	    case Twchar: return "wchar";
	    case Tshort: return "short";
	    case Tint: return "int";
	    case Tlong: 
		case Tllong: return "long";
	    case Tfloat: return "float";
	    case Tdouble: return "double";
	    case Tldouble: return "decimal";
	    case Tuchar: return "unsignedByte";
	    case Tushort: return "unsignedShort";
	    case Tuint: return "unsignedInt";
	    case Tulong:
	    case Tullong: return "unsignedLong";
	    case Ttime: return "dateTime";
	    case Tpointer:
	    case Treference: return the_type((Tnode *)typ->ref);
	    case Tarray: return "SOAP-ENC:Array";
	    case Tenum: 
			if((Table *)typ->ref == booltable)
				return "boolean";
	    case Tstruct:
	    case Tclass:
			return ns_convert(typ->id->name);
		default:
			break;
	}
	return "";
}

/* c_type returns the type to be used in parameter declaration*/
char * c_type(Tnode * typ)
{
	char * p, * q, tempBuf[10];
	Tnode * temp;
	if(typ==0)
		return "NULL";
	switch(typ->type) {
	    case Tnone: return "";
	    case Tvoid: return "void";
	    case Tchar: return "char";
	    case Twchar: return "wchar_t";
	    case Tshort: return "short";
	    case Tint: return "int";
	    case Tlong: return "long";
	    case Tllong: return "LONG64";
	    case Tfloat: return "float";
	    case Tdouble: return "double";
	    case Tldouble: return "long double";
	    case Tuchar: return "uchar";
	    case Tushort: return "ushort";
	    case Tuint: return "uint";
	    case Tulong: return "ulong";
	    case Tullong: return "ULONG64";
	    case Ttime: return "time_t";
	    case Tstruct:
			p = static_cast<char *>(emalloc((8+strlen(ident(typ->id->name)))*sizeof(char)));
			strcpy(p, "struct ");
			strcat(p, ident(typ->id->name));
			break;
	    case Tclass:
			p = ident(typ->id->name);
			break;
	    case Tunion: 
			p = static_cast<char *>(emalloc((7+strlen(ident(typ->id->name)))*sizeof(char)));
			strcpy(p, "union ");
			strcat(p, ident(typ->id->name));
			break;
	    case Tenum:
			if((Table *)typ->ref == booltable)
				return "bool";
			p = static_cast<char *>(emalloc((6+strlen(ident(typ->id->name)))*sizeof(char)));
			strcpy(p, "enum ");
			strcat(p, ident(typ->id->name));
			break;
	    case Tpointer:
			p = c_type_id((Tnode *)typ->ref, "*");
			break;
	    case Treference:
			p = c_type_id((Tnode *)typ->ref, "&");
			break;
	    case Tarray:
			temp = typ;
			while(((Tnode *)(typ->ref))->type==Tarray) {
				typ = (Tnode *)typ->ref;
			}
			p = static_cast<char *>(emalloc((12+strlen(q = c_type((Tnode *)typ->ref)))*sizeof(char)));
			if(((Tnode *)typ->ref)->type == Tpointer)
				sprintf(p, "%s", c_type((Tnode *)typ->ref));
			else
				strcpy(p, q);
			typ = temp;
			while(typ->type==Tarray) {
				if(((Tnode *)typ->ref)->width) {
					sprintf(tempBuf, "[%d]", (typ->width/((Tnode *)typ->ref)->width));
					strcat(p, tempBuf);
				}
				typ = (Tnode *)typ->ref;
			}
			break;
	    case Ttemplate:
			if(typ->ref) {
				p = static_cast<char *>(emalloc((strlen(q = c_type((Tnode *)typ->ref))+strlen(ident(typ->id->name))+4)*sizeof(char)));
				strcpy(p, ident(typ->id->name));
				strcat(p, "<");
				strcat(p, q);
				strcat(p, " >");
				break;
			}
	    default: return "UnknownType";
	}
	return p;
}

char * c_storage(/*Storage*/int sto)
{
	char * p;
	static char buf[256];
	if(sto&Sconst) {
		p = c_storage(sto&~Sconst);
		strcat(p, "const ");
		return p;
	}
	if(sto&Sconstptr) {
		p = c_storage(sto&~Sconstptr);
		strcat(p, "const ");
		return p;
	}
	if(sto&Sauto) {
		p = c_storage(sto&~Sauto);
		strcat(p, "auto ");
		return p;
	}
	if(sto&Sregister) {
		p = c_storage(sto&~Sregister);
		strcat(p, "register ");
		return p;
	}
	if(sto&Sstatic) {
		p = c_storage(sto&~Sstatic);
		strcat(p, "static ");
		return p;
	}
	if(sto&Sexplicit) {
		p = c_storage(sto&~Sexplicit);
		strcat(p, "explicit ");
		return p;
	}
	if(sto&Sextern) {
		p = c_storage(sto&~Sextern);
		return p;
	}
	if(sto&Stypedef) {
		p = c_storage(sto&~Stypedef);
		strcat(p, "typedef ");
		return p;
	}
	if(sto&Svirtual) {
		p = c_storage(sto&~Svirtual);
		strcat(p, "virtual ");
		return p;
	}
	if(sto&Sfriend) {
		p = c_storage(sto&~Sfriend);
		strcat(p, "friend ");
		return p;
	}
	if(sto&Sinline) {
		p = c_storage(sto&~Sinline);
		strcat(p, "inline ");
		return p;
	}
	buf[0] = '\0';
	return buf;
}

char * c_init(Entry * e)
{
	static char buf[1024];
	buf[0] = '\0';
	if(e && e->info.hasval) {
		switch(e->info.typ->type) {
		    case Tchar:
		    case Twchar:
		    case Tuchar:
		    case Tshort:
		    case Tushort:
		    case Tint:
		    case Tuint:
		    case Tlong:
		    case Tllong:
		    case Tulong:
		    case Tullong:
		    case Ttime:
			sprintf(buf, " = " SOAP_LONG_FORMAT, e->info.val.i);
			break;
		    case Tfloat:
		    case Tdouble:
		    case Tldouble:
			sprintf(buf, " = %f", e->info.val.r);
			break;
		    case Tenum:
			sprintf(buf, " = (%s)" SOAP_LONG_FORMAT, c_type(e->info.typ), e->info.val.i);
			break;
		    default:
			if(e->info.val.s && strlen(e->info.val.s) < sizeof(buf)-6)
				sprintf(buf, " = (char*)\"%s\"", cstring(e->info.val.s));
			else if(e->info.typ->type == Tpointer)
				sprintf(buf, " = NULL");
			break;
		}
	}
	return buf;
}

/* c_type_id returns the arraytype to be used in parameter declaration
   Allows you to specify the identifier that acts acts as teh name of teh
   type of array */
char * c_type_id(Tnode * typ, char * name)
{
	char * id, * p, * q, tempBuf[10];
	Tnode * temp;
	Entry * e;
	if(!typ)
		return "NULL";
	id = ident(name);
	switch(typ->type) {
	    case Tnone:
			p = id;
			break;
	    case Tvoid:
			p = (char *)emalloc(6+strlen(id));
			strcpy(p, "void ");
			strcat(p, id);
			break;
	    case Tchar:
			p = (char *)emalloc(6+strlen(id));
			strcpy(p, "char ");
			strcat(p, id);
			break;
	    case Twchar:
			p = (char *)emalloc(9+strlen(id));
			strcpy(p, "wchar_t ");
			strcat(p, id);
			break;
	    case Tshort:
			p = (char *)emalloc(7+strlen(id));
			strcpy(p, "short ");
			strcat(p, id);
			break;
	    case Tint:
			p = (char *)emalloc(5+strlen(id));
			strcpy(p, "int ");
			strcat(p, id);
			break;
	    case Tlong:
			p = (char *)emalloc(6+strlen(id));
			strcpy(p, "long ");
			strcat(p, id);
			break;
	    case Tllong:
			p = (char *)emalloc(8+strlen(id));
			strcpy(p, "LONG64 ");
			strcat(p, id);
			break;
	    case Tfloat:
			p = (char *)emalloc(7+strlen(id));
			strcpy(p, "float ");
			strcat(p, id);
			break;
	    case Tdouble:
			p = (char *)emalloc(8+strlen(id));
			strcpy(p, "double ");
			strcat(p, id);
			break;
	    case Tldouble:
			p = (char *)emalloc(13+strlen(id));
			strcpy(p, "long double ");
			strcat(p, id);
			break;
	    case Tuchar:
			p = (char *)emalloc(15+strlen(id));
			strcpy(p, "uchar ");
			strcat(p, id);
			break;
	    case Tushort:
			p = (char *)emalloc(16+strlen(id));
			strcpy(p, "ushort ");
			strcat(p, id);
			break;
	    case Tuint:
			p = (char *)emalloc(14+strlen(id));
			strcpy(p, "uint ");
			strcat(p, id);
			break;
	    case Tulong:
			p = (char *)emalloc(15+strlen(id));
			strcpy(p, "ulong ");
			strcat(p, id);
			break;
	    case Tullong:
			p = (char *)emalloc(9+strlen(id));
			strcpy(p, "ULONG64 ");
			strcat(p, id);
			break;
	    case Ttime:
			p = (char *)emalloc(8+strlen(id));
			strcpy(p, "time_t ");
			strcat(p, id);
			break;
	    case Tstruct:
			p = (char *)emalloc((9+strlen(ident(typ->id->name))+strlen(id))*sizeof(char));
			strcpy(p, "struct ");
			strcat(p, ident(typ->id->name));
			strcat(p, " ");
			strcat(p, id);
			break;
	    case Tclass:
			if(!typ->classed && !is_imported(typ)) {
				p = (char *)emalloc((8+strlen(ident(typ->id->name))+strlen(id))*sizeof(char));
				strcpy(p, "class ");
				strcat(p, ident(typ->id->name));
				typ->classed = True;
			}
			else {
				p = (char *)emalloc((2+strlen(ident(typ->id->name))+strlen(id))*sizeof(char));
				strcpy(p, ident(typ->id->name)); 
			}
			strcat(p, " ");
			strcat(p, id);
			break;
	    case Tunion:
			p = (char *)emalloc((8+strlen(ident(typ->id->name))+strlen(id))*sizeof(char));
			strcpy(p, "union ");
			strcat(p, ident(typ->id->name));
			strcat(p, " ");
			strcat(p, id);
			break;
	    case Tenum:
			if((Table *)typ->ref == booltable) {
				p = (char *)emalloc((strlen(id)+6)*sizeof(char));
				strcpy(p, "bool ");
				strcat(p, id);
				return p;
			}
			p = (char *)emalloc((7+strlen(ident(typ->id->name))+strlen(id))*sizeof(char));
			strcpy(p, "enum ");
			strcat(p, ident(typ->id->name));
			strcat(p, " ");
			strcat(p, id);
			break;
	    case Tpointer:
			p = (char *)emalloc(strlen(id)+2);
			strcpy(p+1, id);
			p[0] = '*';
			p = c_type_id((Tnode *)typ->ref, p);
			break;
	    case Treference:
			p = (char *)emalloc(strlen(id)+2);
			strcpy(p+1, id);
			p[0] = '&';
			p = c_type_id((Tnode *)typ->ref, p);
			break;
	    case Tarray:
			temp = typ;
			while(((Tnode *)(typ->ref))->type==Tarray) {
				typ = (Tnode *)typ->ref;
			}
			p = (char *)emalloc((12+strlen(q = c_type_id((Tnode *)typ->ref, id)))*sizeof(char));
			strcpy(p, q);
			typ = temp;
			while(typ->type==Tarray) {
				if(((Tnode *)typ->ref)->width) {
					sprintf(tempBuf, "[%d]", (typ->width/((Tnode *)typ->ref)->width));
					strcat(p, tempBuf);
				}
				typ = (Tnode *)typ->ref;
			}
			/*if(((Tnode*) (typ->ref))->type==Tarray){
			sprintf(p,"%s [%d]",c_type((Tnode*)typ->ref),(typ->width / ((Tnode*) typ->ref)->width));
			}else
			sprintf(p,"%s a[%d]",c_type((Tnode*)typ->ref),(typ->width /((Tnode*) typ->ref)->width));*/
			break;
	    case Tfun:
			if(strncmp(id, "operator ", 9))
				q = c_type_id(((FNinfo *)typ->ref)->ret, id);
			else
				q = id;
			p = (char *)emalloc(1024);
			strcpy(p, q);
			strcat(p, "(");
			for(e = ((FNinfo *)typ->ref)->args->list; e; e = e->next) {
				strcat(p, c_storage(e->info.sto));
				if(e->info.typ->type != Tvoid) {
					strcat(p, c_type_id(e->info.typ, e->sym->name));
					strcat(p, c_init(e));
				}
				else
					strcat(p, "void");
				if(e->next)
					strcat(p, ", ");
			}
			strcat(p, ")");
			break;
	    case Ttemplate:
			if(typ->ref) {
				p = (char *)emalloc((strlen(q = c_type((Tnode *)typ->ref))+strlen(ident(typ->id->name))+strlen(id)+4)*sizeof(char));
				strcpy(p, ident(typ->id->name));
				strcat(p, "<");
				strcat(p, q);
				strcat(p, " >");
				strcat(p, id);
				break;
			}
	    default:
			return "UnknownType";
	}
	return p;
}

char * xsi_type_Tarray(Tnode * typ)
{
	Tnode * t;
	int cardinality;
	char * p, * s;
	t = (Tnode *)typ->ref;
	if(is_fixedstring(typ)) {
		if(typ->sym)
			return ns_convert(typ->sym->name);
		return "xsd:string";
	}
	cardinality = 1;
	while(t->type == Tarray || (is_dynamic_array(t) && !has_ns(t) && !is_untyped(typ))) {
		if(t->type == Tarray)
			t = (Tnode *)t->ref;
		else
			t = (Tnode *)((Table *)t->ref)->list->info.typ->ref;
		cardinality++;
	}
	s = xsi_type(t);
	if(!*s)
		s = wsdl_type(t, "");
	p = (char *)emalloc(strlen(s)+cardinality+3);
	strcpy(p, s);
	if(cardinality > 1) {
		strcat(p, "[");
		for(; cardinality > 2; cardinality--)
			strcat(p, ",");
		strcat(p, "]");
	}
	return p;
}

char * xsi_type_Darray(Tnode * typ)
{
	Tnode * t;
	Entry * q;
	int cardinality;
	char * p, * s;
	if(!typ->ref)
		return "";
	q = ((Table *)typ->ref)->list;
	while(q && q->info.typ->type == Tfun)
		q = q->next;
	t = (Tnode *)q->info.typ->ref;
	cardinality = 1;
	while(t->type == Tarray || (is_dynamic_array(t) && !has_ns(t) && !is_untyped(typ))) {
		if(t->type == Tarray)
			t = (Tnode *)t->ref;
		else {q = ((Table *)t->ref)->list;
		      while(q && q->info.typ->type == Tfun)
			      q = q->next;
		      t = (Tnode *)q->info.typ->ref; }
		cardinality++;
	}
	s = xsi_type(t);
	if(!*s)
		s = wsdl_type(t, "");
	p = (char *)emalloc(strlen(s)+cardinality*2+1);
	strcpy(p, s);
	if(cardinality > 1) {
		strcat(p, "[");
		for(; cardinality > 2; cardinality--)
			strcat(p, ",");
		strcat(p, "]");
	}
	return p;
}

void generate(Tnode * typ)
{
	if(kflag && is_XML(typ)) {
		soap_traverse(typ);
		return;
	}
	if(is_transient(typ) || typ->type == Twchar || is_XML(typ) || is_void(typ))
		return;
	if(lflag && typ->type == Tint && !typ->sym) {
		fprintf(fhead, "\n#ifndef %s", soap_type(typ));
		fprintf(fhead, "\n\t#define %s (%d)", soap_type(typ), typ->num);
		fprintf(fhead, "\n#endif");
		fprintf(fhead, "\nSOAP_FMAC1 void SOAP_FMAC2 soap_default_int(struct soap*, int*);");
		fprintf(fhead, "\nSOAP_FMAC1 int SOAP_FMAC2 soap_out_int(struct soap*, const char*, int, const int*, const char*);");
		fprintf(fhead, "\nSOAP_FMAC1 int* SOAP_FMAC2 soap_in_int(struct soap*, const char*, int*, const char*);");
		return; /* do not generate int serializers in libs */
	}
	else if(is_imported(typ) && (typ->type != Tint || typ->sym))
		return;
	if(is_typedef(typ) && is_element(typ))
		fprintf(fhead, "\n\n/* %s is a typedef element/attribute synonym for %s */", c_ident(typ), t_ident(typ));
	if(is_primitive(typ) || is_string(typ) || is_wstring(typ)) {
		if(is_external(typ) && namespaceid)                                                             {
			char * id = namespaceid;
			fprintf(fhead, "\n\n}");
			fprintf(fout, "\n\n}");
			namespaceid = NULL;
			fprintf(fhead, "\n#ifndef %s", soap_type(typ));
			fprintf(fhead, "\n\t#define %s (%d)", soap_type(typ), typ->num);
			fprintf(fhead, "\n#endif");
			namespaceid = id;
		}
		fprintf(fhead, "\n#ifndef %s", soap_type(typ));
		fprintf(fhead, "\n\t#define %s (%d)", soap_type(typ), typ->num);
		fprintf(fhead, "\n#endif");
		fflush(fhead);
		soap_default(typ);
		soap_serialize(typ);
		if(kflag)
			soap_traverse(typ);
		soap_out(typ);
		soap_in(typ);
		if(is_external(typ) && namespaceid) {
			fprintf(fhead, "\n\nnamespace %s {", namespaceid);
			fprintf(fout, "\n\nnamespace %s {", namespaceid);
		}
		soap_put(typ);
		soap_get(typ);
		return;
	}
	switch(typ->type) {
	    case Ttemplate:
	    case Tenum:
	    case Tpointer:
	    case Tarray:
	    case Tstruct:
	    case Tclass:
	    case Tunion:
		if(is_header_or_fault(typ) || is_body(typ)) {
			fprintf(fhead, "\n\n#ifndef WITH_NOGLOBAL");
			fprintf(fout, "\n\n#ifndef WITH_NOGLOBAL");
		}
		if(is_external(typ) && namespaceid) {
			char * id = namespaceid;
			fprintf(fhead, "\n\n}");
			fprintf(fout, "\n\n}");
			namespaceid = NULL;
			fprintf(fhead, "\n#ifndef %s", soap_type(typ));
			fprintf(fhead, "\n\t#define %s (%d)", soap_type(typ), typ->num);
			fprintf(fhead, "\n#endif");
			namespaceid = id;
		}
		fprintf(fhead, "\n#ifndef %s", soap_type(typ));
		fprintf(fhead, "\n\t#define %s (%d)", soap_type(typ), typ->num);
		fprintf(fhead, "\n#endif");
		fflush(fhead);
		soap_default(typ);
		soap_serialize(typ);
		if(kflag)
			soap_traverse(typ);
		soap_out(typ);
		soap_in(typ);
		if(is_external(typ) && namespaceid) {
			fprintf(fhead, "\n\nnamespace %s {", namespaceid);
			fprintf(fout, "\n\nnamespace %s {", namespaceid);
		}
		soap_put(typ);
		soap_get(typ);
		if(typ->type == Tstruct || typ->type == Tclass || typ->type == Ttemplate)
			soap_instantiate_class(typ);
		if(is_header_or_fault(typ) || is_body(typ)) {
			fprintf(fhead, "\n\n#endif");
			fprintf(fout, "\n\n#endif");
		}
		break;
	    default:
		break;
	}
}

void matlab_gen_sparseStruct(void)
{
	fprintf(fmheader, "\nstruct soapSparseArray{\n");
	fprintf(fmheader, "  int *ir;\n");
	fprintf(fmheader, "  int *jc;\n");
	fprintf(fmheader, "  double *pr;\n");
	fprintf(fmheader, "  int num_columns;\n");
	fprintf(fmheader, "  int num_rows;\n");
	fprintf(fmheader, "  int nzmax;\n");
	fprintf(fmheader, "};\n");
}

void matlab_c_to_mx_sparse(void)
{
	fprintf(fmheader, "\nmxArray* c_to_mx_soapSparseArray(struct soapSparseArray);\n");
	fprintf(fmatlab, "\nmxArray* c_to_mx_soapSparseArray(struct soapSparseArray a)\n");
	fprintf(fmatlab, "{\n");
	fprintf(fmatlab, "  mxArray *b;\n");
	fprintf(fmatlab, "  b = mxCreateSparse(a.num_rows, a.num_columns, a.nzmax, mxREAL);\n");
	fprintf(fmatlab, "  mxSetIr(b,a.ir);\n");
	fprintf(fmatlab, "  mxSetJc(b,a.jc);\n");
	fprintf(fmatlab, "  mxSetPr(b,a.pr);\n");
	fprintf(fmatlab, "  return b;\n");
	fprintf(fmatlab, "}\n");
}

void matlab_mx_to_c_sparse(void)
{
	fprintf(fmheader, "\nmxArray* mx_to_c_soapSparseArray(const mxArray *, struct soapSparseArray *);\n");
	fprintf(fmatlab, "\nmxArray* mx_to_c_soapSparseArray(const mxArray *a, struct soapSparseArray *b)\n");
	fprintf(fmatlab, "{\n");
	fprintf(fmatlab, "  if(!mxIsSparse(a))\n");
	fprintf(fmatlab, "    {\n");
	fprintf(fmatlab, "      mexErrMsgTxt(\"Input should be a sparse array.\");\n");
	fprintf(fmatlab, "    }\n");

	fprintf(fmatlab, "  /* Get the starting positions of the data in the sparse array. */  \n");
	fprintf(fmatlab, "  b->pr = mxGetPr(a);\n");
	fprintf(fmatlab, "  b->ir = mxGetIr(a);\n");
	fprintf(fmatlab, "  b->jc = mxGetJc(a);\n");
	fprintf(fmatlab, "  b->num_columns = mxGetN(a);\n");
	fprintf(fmatlab, "  b->num_rows = mxGetM(a);\n");
	fprintf(fmatlab, "  b->nzmax = mxGetNzmax(a);\n");
	fprintf(fmatlab, "}\n");
}

void matlab_mx_to_c_dynamicArray(Tnode * typ)
{
	int d;
	Entry * p;

	p = is_dynamic_array(typ);

	fprintf(fmatlab, "{\n");
	fprintf(fmatlab, "\tint i, numdims;\n");
	fprintf(fmatlab, "\tconst int *dims;\n");
	fprintf(fmatlab, "\tdouble *temp;\n");
	fprintf(fmatlab, "\tint size = 1;\n");
	fprintf(fmatlab, "\tint ret;\n");
	fprintf(fmatlab, "\tnumdims = mxGetNumberOfDimensions(a);\n");
	fprintf(fmatlab, "\tdims = mxGetDimensions(a);\n");

	d = get_Darraydims(typ);
	fprintf(fmatlab, "\tif(numdims != %d)\n", d);
	fprintf(fmatlab, "\t\tmexErrMsgTxt(\"Incompatible array specifications in C and mx.\");\n");

	/*
	   fprintf(fmatlab,"\tfor(i=0;i<numdims; i++) {\n");
	   fprintf(fmatlab,"\t  b->__size[i] = dims[i];\n");
	   fprintf(fmatlab,"\t}\n");
	 */
	if((((Tnode *)p->info.typ->ref)->type != Tchar) && (((Tnode *)p->info.typ->ref)->type != Tuchar)) {
		fprintf(fmatlab, "\ttemp = (double*)mxGetPr(a);\n");
		fprintf(fmatlab,
			"\tif(!temp)\n\t\tmexErrMsgTxt(\"mx_to_c_ArrayOfdouble: Pointer to data is NULL\");\n");
	}
	fprintf(fmatlab, "\tfor(i = 0; i < numdims; i++) {\n");
	fprintf(fmatlab, "\t\tif(b->__size[i] < dims[i])\n");
	fprintf(fmatlab, "\t\t\tmexErrMsgTxt(\"Incompatible array dimensions in C and mx.\");\n");
	fprintf(fmatlab, "\t\tsize *= dims[i];\n");
	fprintf(fmatlab, "\t}\n");
	if((((Tnode *)p->info.typ->ref)->type != Tchar) && (((Tnode *)p->info.typ->ref)->type != Tuchar)) {
		fprintf(fmatlab, "\tfor(i = 0; i < size; i++)\n");
		fprintf(fmatlab, "\t\tb->__ptr[i] = (%s)*temp++;\n", c_type((Tnode *)p->info.typ->ref));
	}
	else {
		fprintf(fmatlab, "\tret = mxGetString(a, b->__ptr, size + 1);\n");
		fprintf(fmatlab, "\tmexPrintf(\"ret = %%d, b->__ptr = %%s, size = %%d\", ret, b->__ptr, size);\n");
	}
	fprintf(fmatlab, "\n}\n");
	fflush(fmatlab);
}

void matlab_c_to_mx_dynamicArray(Tnode * typ)
{
	int d, i;
	Entry * p = is_dynamic_array(typ);
	fprintf(fmatlab, "{\n");
	fprintf(fmatlab, "\tmxArray *out;\n");
	fprintf(fmatlab, "\t%s;\n", c_type_id((Tnode *)p->info.typ->ref, "*temp"));
	d = get_Darraydims(typ);
	fprintf(fmatlab, "\tint i;\n");

	fprintf(fmatlab, "\tint ndim = %d, dims[%d] = {", d, d);
	for(i = 0; i < d; i++) {
		if(i==0)
			fprintf(fmatlab, "a.__size[%d]", i);
		else
			fprintf(fmatlab, ", a.__size[%d]", i);
	}
	fprintf(fmatlab, "};\n");

	fprintf(fmatlab, "\tint size = ");
	for(i = 0; i < d; i++) {
		if(i==0)
			fprintf(fmatlab, "dims[%d]", i);
		else
			fprintf(fmatlab, "*dims[%d]", i);
	}
	fprintf(fmatlab, ";\n");
	if((((Tnode *)p->info.typ->ref)->type != Tchar) && (((Tnode *)p->info.typ->ref)->type != Tuchar)) {
		fprintf(fmatlab, "\tout = mxCreateNumericArray(ndim, dims, %s, mxREAL);\n",
			get_mxClassID((Tnode *)p->info.typ->ref));
		fprintf(fmatlab, "\tif(!out)\n\t\tmexErrMsgTxt(\"Could not create mxArray.\");\n");
		fprintf(fmatlab, "\ttemp = (%s) mxGetPr(out);\n", c_type_id((Tnode *)p->info.typ->ref, "*"));
		fprintf(fmatlab, "\tif(!temp)\n\t\tmexErrMsgTxt(\"matlab_array_c_to_mx: Pointer to data is NULL\");\n");

		fprintf(fmatlab, "\tfor(i = 0; i < size; i++)\n");
		fprintf(fmatlab, "\t\t*temp++ = a.__ptr[i];\n");
	}
	else {
		fprintf(fmatlab, "\tout = mxCreateString(a.__ptr);\n");
		fprintf(fmatlab, "\tif(!out)\n\t\tmexErrMsgTxt(\"Could not create mxArray.\");\n");
	}
	fprintf(fmatlab, "\treturn out;\n}\n");
	fflush(fmatlab);
}

char * get_mxClassID(Tnode * typ)
{
	switch(typ->type) {
	    case Tdouble: return "mxDOUBLE_CLASS";
	    case Tfloat: return "mxSINGLE_CLASS";
	    case Tshort: return "mxINT16_CLASS";
	    case Tushort: return "mxUINT16_CLASS";
	    case Tint: return "mxINT32_CLASS";
	    case Tuint: return "mxUINT32_CLASS";
	    case Tlong: return "mxINT32_CLASS";
	    case Tulong: return "mxUINT32_CLASS";
	    case Tllong: return "mxINT64_CLASS";
	    case Tullong: return "mxUINT64_CLASS";
	    case Tchar: return "mxCHAR_CLASS";
	    case Tuchar: return "mxCHAR_CLASS";
	    default: return "";
	}
	;
}

/*Function not in use.*/
void matlab_array_c_to_mx(Tnode * typ)
{
	Tnode * temp;
	int d, i;

	fprintf(fmatlab, "{\n\tint rows, r, cols, c;\n");
	fprintf(fmatlab, "\tmxArray* out;\n");
	fprintf(fmatlab, "\tdouble* temp;\n");
	d = get_dimension(typ);
	fprintf(fmatlab, "\tint ndim = %d, dims[%d] = {", d, d);
	temp = typ;
	for(i = 0; i<d; i++) {
		if(i==0)
			fprintf(fmatlab, "%d", temp->width/((Tnode *)temp->ref)->width);
		else
			fprintf(fmatlab, ",%d", temp->width/((Tnode *)temp->ref)->width);
		temp = (Tnode *)typ->ref;
	}
	fprintf(fmatlab, "};\n");
	fprintf(fmatlab, "\tout = mxCreateNumericArray(ndim, dims, mxDOUBLE_CLASS, mxREAL);\n");
	fprintf(fmatlab, "\ttemp = (double *) mxGetPr(out);\n");
	fprintf(fmatlab, "\tif(!out)\n\t\tmexErrMsgTxt(\"Could not create mxArray.\");\n");
	fprintf(fmatlab, "\tif(!temp)\n\t\tmexErrMsgTxt(\"matlab_array_c_to_mx: Pointer to data is NULL\");\n");
	fprintf(fmatlab, "\trows = mxGetM(out);\n");
	fprintf(fmatlab, "\tif(!rows)\n\t\tmexErrMsgTxt(\"matlab_array_c_to_mx: Data has zero rows\");\n");
	fprintf(fmatlab, "\tcols = mxGetN(out);\n");
	fprintf(fmatlab, "\tif(!cols)\n\t\tmexErrMsgTxt(\"matlab_array_c_to_mx: Data has zero columns\");\n");
	fprintf(fmatlab, "\tfor(c = 0; c < cols; c++)\n");
	fprintf(fmatlab, "\t\tfor(r = 0; r < rows; r++)\n");
	fprintf(fmatlab, "\t\t\t*temp++ = z->a[r][c];\n");
	fprintf(fmatlab, "\treturn out;\n}\n");
	fflush(fmatlab);
}

void matlab_c_to_mx_pointer(Tnode * typ)
{
	if(typ->ref) {
		fprintf(fmheader, "\nmxArray* c_to_mx_%s(%s);\n", c_ident(typ), c_type_id(typ, ""));
		fprintf(fmatlab, "\nmxArray* c_to_mx_%s(%s)\n", c_ident(typ), c_type_id(typ, "a"));
		fprintf(fmatlab, "{\n");
		fprintf(fmatlab, "\tmxArray  *fout;\n");
		fprintf(fmatlab, "\tfout = c_to_mx_%s(*a);\n", c_ident((Tnode *)typ->ref));
		fprintf(fmatlab, "\treturn fout;\n");
		fprintf(fmatlab, "}\n");
	}
}

void matlab_mx_to_c_pointer(Tnode * typ)
{
	if(typ->ref) {
		fprintf(fmheader, "\nvoid mx_to_c_%s(const mxArray*,%s);\n", c_ident(typ), c_type_id(typ, "*"));
		fprintf(fmatlab, "\nvoid mx_to_c_%s(const mxArray* a,%s)\n", c_ident(typ), c_type_id(typ, "*b"));
		fprintf(fmatlab, "{\n\tmx_to_c_%s(a,*b);\n", c_ident((Tnode *)typ->ref));
		fprintf(fmatlab, "\n}\n");
	}
}

void func2(Tnode * typ)
{
	Table * table, * t;
	Entry * p;
	fprintf(fmatlab, "\tif(!mxIsStruct(a))\n\t\tmexErrMsgTxt(\"Input must be a structure.\");\n");
	table = (Table *)typ->ref;
	for(t = table; t != (Table *)0; t = t->prev) {
		for(p = t->list; p != (Entry *)0; p = p->next) {
			if(p->info.typ->type != Tfun && !is_void(p->info.typ) && !is_XML(p->info.typ)) {
				fprintf(fmatlab, "\t{mxArray *tmp = mxGetField(a,0,\"%s\");\n", ident(p->sym->name));
				fprintf(fmatlab, "\tif(!tmp) {\n");
				fprintf(fmatlab, "\t\tmexErrMsgTxt(\"Above member field is empty!\");\n\t}\n");
				fprintf(fmatlab, "\tmx_to_c_%s(tmp,&(b->%s));}\n", c_ident(p->info.typ), ident(p->sym->name));
			}
		}
	}
}

void matlab_mx_to_c_struct(Tnode * typ)
{
	if(!typ->ref)
		return;
	if(is_dynamic_array(typ)) {
		fprintf(fmheader, "\nvoid mx_to_c_%s(const mxArray*, %s);\n", c_ident(typ), c_type_id(typ, "*"));
		fprintf(fmatlab, "\nvoid mx_to_c_%s(const mxArray* a, %s)\n", c_ident(typ), c_type_id(typ, "*b"));
		matlab_mx_to_c_dynamicArray(typ);
		return;
	}
	else if(strstr(c_type_id(typ, ""), "soapSparseArray")) {
		return;
	}
	fprintf(fmheader, "\nvoid mx_to_c_%s(const mxArray*, %s);\n", c_ident(typ), c_type_id(typ, "*"));
	fprintf(fmatlab, "\nvoid mx_to_c_%s(const mxArray* a, %s)\n", c_ident(typ), c_type_id(typ, "*b"));
	fprintf(fmatlab, "{\n");

	func2(typ);
	fprintf(fmatlab, "\n}\n");

	return;
}

void matlab_c_to_mx_struct(Tnode * typ)
{
	Table * table, * t;
	Entry * p;
	int number_of_fields = 0;
	if(!typ->ref)
		return;
	if(is_dynamic_array(typ)) {
		fprintf(fmheader, "\nmxArray* c_to_mx_%s(%s);\n", c_ident(typ), c_type_id(typ, ""));
		fprintf(fmatlab, "\nmxArray* c_to_mx_%s(%s)\n", c_ident(typ), c_type_id(typ, "a"));
		matlab_c_to_mx_dynamicArray(typ);
		return;
	}
	else if(strstr(c_type_id(typ, ""), "soapSparseArray")) {
		return;
	}
	fprintf(fmheader, "\nmxArray* c_to_mx_%s(%s);\n", c_ident(typ), c_type_id(typ, ""));
	fprintf(fmatlab, "\nmxArray* c_to_mx_%s(%s)\n", c_ident(typ), c_type_id(typ, "a"));
	table = (Table *)typ->ref;
	fprintf(fmatlab, "{\n\tconst char* fnames[] = {");
	for(t = table; t != (Table *)0; t = t->prev) {
		for(p = t->list; p != (Entry *)0; p = p->next) {
			if(p->info.typ->type != Tfun && !is_void(p->info.typ) && !is_XML(p->info.typ)) {
				if(number_of_fields)
					fprintf(fmatlab, ",\"%s\"", ident(p->sym->name));
				else
					fprintf(fmatlab, "\"%s\"", ident(p->sym->name));
				number_of_fields++;
			}
		}
	}
	fprintf(fmatlab, "}; /* pointers to member field names */\n");

	fprintf(
		fmatlab,
		"\tint rows = 1, cols = 1;\n\tint index = 0;\n\tint number_of_fields = %d;\n\tmxArray *struct_array_ptr;\n",
		number_of_fields);
	fprintf(fmatlab, "\t/* Create a 1x1 struct matrix for output  */\n");
	fprintf(
		fmatlab,
		"\tstruct_array_ptr = mxCreateStructMatrix(rows, cols, number_of_fields, fnames);\n\tmexPrintf(\"6\");\n\tif(struct_array_ptr == NULL) {\n\t\tmexPrintf(\"COULDNT CREATE A MATRIX\");}\n\tmexPrintf(\"7\");\n");

	for(t = table; t != (Table *)0; t = t->prev) {
		for(p = t->list; p != (Entry *)0; p = p->next) {
			if(p->info.typ->type != Tfun && !is_void(p->info.typ) && !is_XML(p->info.typ)) {
				fprintf(fmatlab, "\t{mxArray *fout = c_to_mx_%s(a.%s);\n", c_ident(p->info.typ),
					ident(p->sym->name));
				fprintf(fmatlab, "\tmxSetField(struct_array_ptr, index,\"%s\" , fout);}\n",
					ident(p->sym->name));
			}
		}
	}
	fprintf(fmatlab, "\treturn struct_array_ptr;\n}\n");
	return;
}

void matlab_c_to_mx_primitive(Tnode * typ)
{
	fprintf(fmheader, "\nmxArray* c_to_mx_%s(%s);", c_ident(typ), c_type_id(typ, ""));
	fprintf(fmatlab, "\nmxArray* c_to_mx_%s(%s)\n", c_ident(typ), c_type_id(typ, "a"));

	fprintf(fmatlab, "{\n\tmxArray  *fout;\n");
	if((typ->type == Tchar) || (typ->type == Tuchar)) {
		fprintf(fmatlab, "\tchar buf[2];\n");
		fprintf(fmatlab, "\tbuf[0] = a;\n");
		fprintf(fmatlab, "\tbuf[1] = \'\\0\';\n");
		fprintf(fmatlab, "\tfout = mxCreateString(buf);\n");
		fprintf(fmatlab, "\tif(!fout)\n");
		fprintf(fmatlab, "\t\tmexErrMsgTxt(\"Could not create mxArray.\");\n");
	}
	else {
		fprintf(fmatlab, "\tint ndim = 1, dims[1] = {1};\n");
		fprintf(fmatlab, "\tfout = mxCreateNumericArray(ndim, dims, %s, mxREAL);\n", get_mxClassID(typ));
		fprintf(fmatlab, "\t%s = (%s)mxGetPr(fout);\n", c_type_id(typ, "*temp"), c_type_id(typ, "*"));
		fprintf(fmatlab, "\tif(!fout)\n");
		fprintf(fmatlab, "\t\tmexErrMsgTxt(\"Could not create mxArray.\");\n");
		fprintf(fmatlab, "\tif(!temp) \n");
		fprintf(fmatlab, "\t\tmexErrMsgTxt(\"matlab_array_c_to_mx: Pointer to data is NULL\");\n");
		fprintf(fmatlab, "\t*temp++= a;\n");
	}
	fprintf(fmatlab, "\treturn fout;\n}\n");
}

void matlab_mx_to_c_primitive(Tnode * typ)
{
	fprintf(fmheader, "\nvoid mx_to_c_%s(const mxArray *, %s);\n", c_ident(typ), c_type_id(typ, "*"));
	fprintf(fmatlab, "\nvoid mx_to_c_%s(const mxArray *a, %s)\n", c_ident(typ), c_type_id(typ, "*b"));
	if((typ->type == Tchar) || (typ->type == Tuchar)) {
		fprintf(fmatlab, "{\n\tint ret;\n");
		fprintf(fmatlab, "\tchar buf[2];\n");
		fprintf(fmatlab, "\tret = mxGetString(a, buf, 2);\n");
		fprintf(fmatlab, "\tmexPrintf(\"ret = %%d, buf = %%s\", ret, buf);\n");
		fprintf(fmatlab, "\t*b = buf[0];\n");
	}
	else {
		fprintf(fmatlab, "{\n\tdouble* data = (double*)mxGetData(a);\n");
		fprintf(fmatlab, "\t*b = (%s)*data;\n", c_type(typ));
	}
	fprintf(fmatlab, "\n}\n");
}

void matlab_out_generate(Tnode * typ)
{
	if(is_transient(typ) || typ->type == Twchar || is_XML(typ))
		return;
	/*
	   typeNO++;
	   if(typeNO>=1024)
	   execerror("Too many user-defined data types");
	 */
	if(is_primitive(typ)) {
		matlab_c_to_mx_primitive(typ);
		matlab_mx_to_c_primitive(typ);
		return;
	}
	switch(typ->type) {
	    case Tstruct:
		matlab_c_to_mx_struct(typ);
		matlab_mx_to_c_struct(typ);
		break;
	    case Tpointer:
		matlab_c_to_mx_pointer(typ);
		matlab_mx_to_c_pointer(typ);
		break;
	    case Tarray:
		break;
	    default: break;
	}
}

/*his function is called first it first generates all routines
   and then in the second pass calls all routines to generate
   matlab_out for the table*/

void func1(Table * table, Entry * param)
{
	Entry * q, * pout, * response = NULL;
	q = entry(table, param->sym);
	if(q)
		pout = (Entry *)q->info.typ->ref;
	else {
		fprintf(stderr, "Internal error: no table entry\n");
	      return; 
	}
	q = entry(classtable, param->sym);
	if(!is_response(pout->info.typ)) {
		response = get_response(param->info.typ);
	}
	fprintf(fmheader, "\n\toutside loop struct %s soap_tmp_%s;", param->sym->name, param->sym->name);
	if(!is_response(pout->info.typ) && response) {
		fprintf(fmheader, "\n\tif..inside loop struct %s *soap_tmp_%s;", c_ident(response->info.typ),
			c_ident(response->info.typ));
	}
	fflush(fmheader);
}

void matlab_def_table(Table * table)
{
	Entry * q, * pout, * e, * response = NULL;
	int i;
	Tnode * p;

	/*  for(q1 = table->list; q1 != (Entry*) 0; q1 = q1->next)
	   if(q1->info.typ->type==Tfun)
	    func1(table, q1);
	 */

	/* Sparse matrix code will be present by default */
	matlab_gen_sparseStruct();
	matlab_c_to_mx_sparse();
	matlab_mx_to_c_sparse();

	for(i = 0; i<TYPES; i++)
		for(p = Tptr[i]; p!=(Tnode *)0; p = p->next) {
			/* This is generated for everything declared in the ".h" file. To make
			   sure that it doesnt get generated for functions do a comparison with
			   p->sym->name, so that its not generated for functions.
			 */
			if(is_XML(p))
				continue;
			if(strstr(c_ident(p), "SOAP_ENV_") != NULL)
				continue;
			for(q = table->list; q != (Entry *)0; q = q->next) {
				if(strcmp(c_ident(p), q->sym->name) == 0)
					break;
				e = entry(table, q->sym);
				if(e)
					pout = (Entry *)e->info.typ->ref;
				else {
					fprintf(stderr, "Internal error: no table entry\n");
				      return; 
				}
				if(!is_response(pout->info.typ)) {
					response = get_response(q->info.typ);
				}
				if(!is_response(pout->info.typ) && response) {
					if(strcmp(c_ident(p), c_ident(response->info.typ)) == 0)
						break;
				}
			}
			if(q == (Entry *)0)
				matlab_out_generate(p);
		}
}

void def_table(Table * table)
{
	for(int i = 0; i < TYPES; i++) {
		for(Tnode * p = Tptr[i]; p; p = p->next) {
			if(!p->generated && !is_transient(p) && p->type != Twchar && !is_void(p)) {
				p->generated = True;
				generate(p);
				if(fflag)
					if(--partnum == 0)
						return;
			}
		}
	}
}

int no_of_var(Tnode * typ)
{
	Entry * p;
	Table * t;
	int i = 0;
	if(typ->type==Tstruct || typ->type==Tclass) {
		t = (Table *)typ->ref;
		for(p = t->list; p != (Entry *)0; p = p->next) {
			if(p->info.typ->type==Tpointer)
				i++;
		}
	}
	if((((Tnode *)(typ->ref))->type==Tstruct) ||
	   (((Tnode *)(typ->ref))->type==Tclass) ) {
		t = (Table *)((Tnode *)(typ->ref))->ref;
		for(p = t->list; p != (Entry *)0; p = p->next) {
			if(p->info.typ->type==Tpointer)
				i++;
		}
	}
	return i;
}

void in_defs(Table * table)
{
	int i;
	Tnode * p;
	for(i = 0; i < TYPES; i++) {
		for(p = Tptr[i]; p; p = p->next) {
			if(!is_element(p) && !is_transient(p) && p->type != Twchar && p->type != Tfun && p->type !=
			   Treference && p->type != Tunion && !is_XML(p) && !is_header_or_fault(p) && !is_body(p) && !is_template(p))                                                               {
				char * s = xsi_type(p);
				if(!*s)
					s = wsdl_type(p, "");
				if(*s == '-')
					continue;
				if(is_string(p))
					fprintf(fout, "\n\tcase %s:\n\t{\n\t\tchar ** s = soap_in_%s(soap, 0, 0, \"%s\");\n\t\treturn s ? *s : NULL;\n\t}", soap_type(p), c_ident(p), s);
				else if(is_wstring(p))
					fprintf(fout, "\n\tcase %s:\n\t{\n\t\twchar_t ** s = soap_in_%s(soap, 0, 0, \"%s\");\n\t\treturn s ? *s : NULL;\n\t}", soap_type(p), c_ident(p), s);
				else
					fprintf(fout, "\n\tcase %s: return soap_in_%s(soap, 0, 0, \"%s\");", soap_type(p), c_ident(p), s);
			}
		}
	}
}

void in_defs2(Table * table)
{
	int i, j;
	Tnode * p;
	char * s;
	for(i = 0; i < TYPES; i++) { /* make sure (wrapper) classes are checked first */
		if(i == 0)
			j = Tclass;
		else if(i == Tclass)
			continue;
		else
			j = i;
		for(p = Tptr[j]; p; p = p->next) {
			if(!is_element(p) && ((!is_transient(p) && !is_template(p) && p->type != Twchar && p->type != Tfun && p->type !=
			     Tpointer && p->type != Treference && p->type != Tunion && !is_XML(p) &&
			     !is_header_or_fault(p) && !is_body(p)) || (is_string(p) && !is_XML(p)))) {
				s = xsi_type(p);
				if(!*s)
					s = wsdl_type(p, "");
				if(*s == '-')
					continue;
				if(*s) {
					if(is_dynamic_array(p) && !is_binary(p) && !has_ns(p) && !is_untyped(p))
						fprintf(fout, "\n\t\tif(*soap->arrayType && !soap_match_array(soap, \"%s\")) {\t*type = %s;\n\t\t\treturn soap_in_%s(soap, 0, 0, 0); }", s, soap_type(p), c_ident(p));
					else if(is_string(p))
						fprintf(fout, "\n\t\tif(!soap_match_tag(soap, t, \"%s\")) {\n\t\t\t*type = %s;\n\t\t\tchar ** s = soap_in_%s(soap, 0, 0, 0);\n\t\t\treturn s ? *s : NULL;\n\t\t}", s, soap_type(p), c_ident(p));
					else if(is_wstring(p))
						fprintf(fout, "\n\t\tif(!soap_match_tag(soap, t, \"%s\")) {\n\t\t\t*type = %s;\n\t\t\twchar_t ** s = soap_in_%s(soap, 0, 0, 0);\n\t\t\treturn s ? *s : NULL;\n\t\t}", s, soap_type(p), c_ident(p));
					else
						fprintf(fout, "\n\t\tif(!soap_match_tag(soap, t, \"%s\")) { *type = %s; return soap_in_%s(soap, 0, 0, 0); }", s, soap_type(p), c_ident(p));
				}
			}
		}
	}
}

void in_defs3(Table * table)
{
	char * s;
	for(int i = 0; i < TYPES; i++) {
		for(Tnode * p = Tptr[i]; p; p = p->next) {
			if(is_element(p) && ((!is_transient(p) && !is_template(p) && p->type != Twchar && p->type != Tfun && p->type !=
			     Tpointer && p->type != Treference && p->type != Tunion && !is_XML(p) && !is_header_or_fault(p) &&
			     !is_body(p)) || (is_string(p) && !is_XML(p)))) {
				s = xsi_type(p);
				if(!*s)
					s = wsdl_type(p, "");
				if(*s == '-')
					continue;
				if(*s) {
					if(is_dynamic_array(p) && !is_binary(p) && !has_ns(p) && !is_untyped(p))
						fprintf(fout, "\n\t\tif(*soap->arrayType && !soap_match_array(soap, \"%s\"))\n\t\t{\t*type = %s;\n\t\t\treturn soap_in_%s(soap, 0, 0, 0);\n\t\t}",
							s, soap_type(p), c_ident(p));
					else if(is_string(p))
						fprintf(fout, "\n\t\tif(!soap_match_tag(soap, t, \"%s\"))\n\t\t{\tchar **s;\n\t\t\t*type = %s;\n\t\t\ts = soap_in_%s(soap, 0, 0, 0);\n\t\t\treturn s ? *s : NULL;\n\t\t}",
							s, soap_type(p), c_ident(p));
					else if(is_wstring(p))
						fprintf(fout, "\n\t\tif(!soap_match_tag(soap, t, \"%s\"))\n\t\t{\twchar_t **s;\n\t\t\t*type = %s;\n\t\t\ts = soap_in_%s(soap, 0, 0, 0);\n\t\t\treturn s ? *s : NULL;\n\t\t}",
							s, soap_type(p), c_ident(p));
					else
						fprintf(fout, "\n\t\tif(!soap_match_tag(soap, t, \"%s\")) { *type = %s; return soap_in_%s(soap, 0, 0, 0); }", s, soap_type(p), c_ident(p));
				}
			}
		}
	}
}

void out_defs(Table * table)
{
	char * s;
	for(int i = 0; i < TYPES; i++) {
		for(Tnode * p = Tptr[i]; p; p = p->next) {
			if(is_transient(p) || is_template(p) || is_XML(p) || is_header_or_fault(p) || is_body(p))
				continue;
			if(is_element(p)) {
				s = wsdl_type(p, "");
				if(*s == '-')
					continue;
				if(p->type == Tarray)
					fprintf(fout, "\n\tcase %s: return soap_out_%s(soap, \"%s\", id, static_cast<%s>(ptr), NULL);", soap_type(p), c_ident(p), s, c_type_id((Tnode *)p->ref, "(*)"));
				else if(p->type == Tclass && !is_external(p) && !is_volatile(p) && !is_typedef(p))
					fprintf(fout, "\n\tcase %s: return static_cast<const %s>(ptr)->soap_out(soap, \"%s\", id, NULL);", soap_type(p), c_type_id(p, "*"), s);
				else if(is_string(p))
					fprintf(fout, "\n\tcase %s: return soap_out_string(soap, \"%s\", id, static_cast<char * const *>(&ptr), NULL);", soap_type(p), s);
				else if(is_wstring(p))
					fprintf(fout, "\n\tcase %s: return soap_out_wstring(soap, \"%s\", id, static_cast<wchar_t * const *>(&ptr), NULL);", soap_type(p), s);
				else if(p->type == Tpointer)
					fprintf(fout, "\n\tcase %s: return soap_out_%s(soap, \"%s\", id, static_cast<%s>(ptr), NULL);", soap_type(p), c_ident(p), s, c_type_id(p, "const*"));
				else if(p->type != Tnone && p->type != Ttemplate && p->type != Twchar && !is_void(p) && p->type != Tfun && p->type != Treference && p->type != Tunion)
					fprintf(fout, "\n\tcase %s: return soap_out_%s(soap, \"%s\", id, static_cast<const %s>(ptr), NULL);", soap_type(p), c_ident(p), s, c_type_id(p, "*"));
			}
			else {
				s = xsi_type(p);
			      if(!*s)
				      s = wsdl_type(p, "");
			      if(*s == '-')
				      continue;
			      if(p->type == Tarray)
				      fprintf(fout, "\n\tcase %s: return soap_out_%s(soap, tag, id, static_cast<%s>(ptr), \"%s\");",
					      soap_type(p), c_ident(p), c_type_id((Tnode *)p->ref, "(*)"), s);
			      else if(p->type == Tclass && !is_external(p) && !is_volatile(p) && !is_typedef(p))
				      fprintf(fout, "\n\tcase %s: return static_cast<const %s>(ptr)->soap_out(soap, tag, id, \"%s\");", soap_type(p), c_type_id(p, "*"), s);
			      else if(is_string(p))
				      fprintf(fout, "\n\tcase %s: return soap_out_string(soap, tag, id, (char * const *)(&ptr), \"%s\");", soap_type(p), s);
			      else if(is_wstring(p))
				      fprintf(fout, "\n\tcase %s: return soap_out_wstring(soap, tag, id, (wchar_t * const *)(&ptr), \"%s\");", soap_type(p), s);
			      else if(p->type == Tpointer)
				      fprintf(fout, "\n\tcase %s: return soap_out_%s(soap, tag, id, static_cast<%s>(ptr), \"%s\");",
					      soap_type(p), c_ident(p), c_type_id(p, "const*"), s);
			      else if(p->type != Tnone && p->type != Ttemplate && p->type != Twchar && !is_void(p) &&
				      p->type != Tfun && p->type != Treference && p->type != Tunion)
				      fprintf(fout, "\n\tcase %s: return soap_out_%s(soap, tag, id, static_cast<const %s>(ptr), \"%s\");",
					      soap_type(p), c_ident(p), c_type_id(p, "*"), s); 
			}
		}
	}
}

void mark_defs(Table * table)
{
	for(int i = 0; i < TYPES; i++) {
		for(Tnode * p = Tptr[i]; p; p = p->next) {
			if(is_transient(p) || is_template(p) || is_XML(p) || is_header_or_fault(p) || is_body(p) || is_void(p))
				continue;
			if(p->type == Tarray)
				fprintf(fout, "\n\tcase %s: soap_serialize_%s(soap, static_cast<%s>(ptr)); break;", soap_type(p), c_ident(p), c_type_id((Tnode *)p->ref, "(*)"));
			else if(p->type == Tclass && !is_external(p) && !is_volatile(p) && !is_typedef(p)) {
				//fprintf(fout, "\n\tcase %s: ((%s)ptr)->soap_serialize(soap); break;", soap_type(p), c_type_id(p, "*"));
				fprintf(fout, "\n\tcase %s: static_cast<const %s>(ptr)->soap_serialize(soap); break;", soap_type(p), c_type_id(p, "*"));
			}
			else if(is_string(p))
				fprintf(fout, "\n\tcase %s: soap_serialize_string(soap, (char*const*)&ptr); break;", soap_type(p));
			else if(is_wstring(p))
				fprintf(fout, "\n\tcase %s: soap_serialize_wstring(soap, (wchar_t*const*)&ptr); break;", soap_type(p));
			else if(p->type == Tpointer)
				fprintf(fout, "\n\tcase %s: soap_serialize_%s(soap, static_cast<%s>(ptr)); break;", soap_type(p), c_ident(p), c_type_id(p, "const*"));
			else if(p->type == Ttemplate && p->ref)
				fprintf(fout, "\n\tcase %s: soap_serialize_%s(soap, static_cast<const %s>(ptr)); break;", soap_type(p), c_ident(p), c_type_id(p, "*"));
			else if(!is_primitive(p) && p->type != Tnone && p->type != Ttemplate && !is_void(p) && p->type != Tfun && p->type != Treference && p->type != Tunion)
				fprintf(fout, "\n\tcase %s: soap_serialize_%s(soap, static_cast<const %s>(ptr)); break;", soap_type(p), c_ident(p), c_type_id(p, "*"));
		}
	}
}

void in_attach(Table * table)
{
	for(int i = 0; i < TYPES; i++) {
		for(Tnode * p = Tptr[i]; p; p = p->next) {
			if(is_attachment(p)) {
				if(p->type == Tclass)
					fprintf(fout,
						"\n\t\tcase %s:\n\t\t{\t%s a;\n\t\t\ta = (%s)soap_class_id_enter(soap, soap->dime.id, NULL, %s, sizeof(%s), NULL, NULL);\n\t\t\tif(a)\n\t\t\t{\ta->__ptr = (uchar *)soap->dime.ptr;\n\t\t\t\ta->__size = soap->dime.size;\n\t\t\t\ta->id = (char*)soap->dime.id;\n\t\t\t\ta->type = (char*)soap->dime.type;\n\t\t\t\ta->options = (char*)soap->dime.options;\n\t\t\t}\n\t\t\telse\n\t\t\t\treturn soap->error;\n\t\t\tbreak;\n\t\t}",
						soap_type(p), c_type_id(p, "*"), c_type_id(p, "*"), soap_type(p), c_type(p));
				else
					fprintf(fout,
						"\n\t\tcase %s:\n\t\t{\t%s a;\n\t\t\ta = (%s)soap_id_enter(soap, soap->dime.id, NULL, %s, sizeof(%s), 0, NULL, NULL, NULL);\n\t\t\tif(!a)\n\t\t\t\treturn soap->error;\n\t\t\ta->__ptr = (uchar *)soap->dime.ptr;\n\t\t\ta->__size = soap->dime.size;\n\t\t\ta->id = (char*)soap->dime.id;\n\t\t\ta->type = (char*)soap->dime.type;\n\t\t\ta->options = (char*)soap->dime.options;\n\t\t\tbreak;\n\t\t}",
						soap_type(p), c_type_id(p, "*"), c_type_id(p, "*"), soap_type(
							p), c_type(p));
			}
			else if(is_binary(p) && !is_transient(p)) {
				if(p->type == Tclass)
					fprintf(fout,
						"\n\t\tcase %s:\n\t\t{\t%s a;\n\t\t\ta = (%s)soap_class_id_enter(soap, soap->dime.id, NULL, %s, sizeof(%s), NULL, NULL);\n\t\t\tif(!a)\n\t\t\t\treturn soap->error;\n\t\t\ta->__ptr = (uchar *)soap->dime.ptr;\n\t\t\ta->__size = soap->dime.size;\n\t\t\tbreak;\n\t\t}",
						soap_type(p), c_type_id(p, "*"), c_type_id(p, "*"), soap_type(p), c_type(p));
				else
					fprintf(fout,
						"\n\t\tcase %s:\n\t\t{\t%s a;\n\t\t\ta = (%s)soap_id_enter(soap, soap->dime.id, NULL, %s, sizeof(%s), 0, NULL, NULL, NULL);\n\t\t\tif(!a)\n\t\t\t\treturn soap->error;\n\t\t\ta->__ptr = (uchar *)soap->dime.ptr;\n\t\t\ta->__size = soap->dime.size;\n\t\t\tbreak;\n\t\t}",
						soap_type(p), c_type_id(p, "*"), c_type_id(p, "*"), soap_type(p), c_type(p));
			}
		}
	}
}

void soap_instantiate_class(Tnode * typ)
{
	Table * Tptr;
	Entry * Eptr;
	int derclass = 0;
	char * s;
	if(cflag)
		return;
	fprintf(fhead, "\n\n#define soap_new_%s(soap, n) soap_instantiate_%s(soap, n, NULL, NULL, NULL)\n", c_ident(typ), c_ident(typ));
	fprintf(fhead, "\n\n#define soap_delete_%s(soap, p) soap_delete(soap, p)\n", c_ident(typ));
	if(typ->type != Tclass || !typ->sym || !is_eq(typ->sym->name, "xsd__QName") || is_imported(typ))
		if(is_typedef(typ) && !is_external(typ)) {
			fprintf(fhead, "\n\n#define soap_instantiate_%s soap_instantiate_%s\n", c_ident(typ), t_ident(typ));
			fprintf(fhead, "\n\n#define soap_copy_%s soap_copy_%s", c_ident(typ), t_ident(typ));
			return;
		}
	fprintf(fhead, "\nSOAP_FMAC1 %s * FASTCALL soap_instantiate_%s(struct soap*, int, const char*, const char*, size_t*);", c_type(typ), c_ident(typ));
	fprintf(fout, "\n\nSOAP_FMAC1 %s * FASTCALL soap_instantiate_%s(struct soap *soap, int n, const char *type, const char *arrayType, size_t *size)", c_type(typ), c_ident(typ));
	fprintf(fout, "\n{");
	// @v9.8.9 fprintf(fout, "\n\t(void)type; (void)arrayType; /* appease -Wall -Werror */");
	fprintf(fout, "\n\tDBGLOG(TEST, SOAP_MESSAGE(fdebug, \"soap_instantiate_%s(%%d, %%s, %%s)\\n\", n, type?type:\"\", arrayType?arrayType:\"\"));", c_ident(typ));
	fprintf(fout, "\n\tstruct soap_clist *cp = soap_link(soap, NULL, %s, n, %s_fdelete);", soap_type(typ), prefix);
	fprintf(fout, "\n\tif(!cp)\n\t\treturn NULL;");
	for(Eptr = classtable->list; Eptr; Eptr = Eptr->next) {
		Tptr = ((Table *)Eptr->info.typ->ref);
		if(Tptr == ((Table *)typ->ref)) {
			continue;
		}
		derclass = 0;
		while(Tptr) {
			if(Tptr == (Table *)typ->ref) {
				derclass = 1;
			}
			Tptr = Tptr->prev;
		}
		if(derclass == 1 && !is_transient(Eptr->info.typ)) {
			if(is_dynamic_array(Eptr->info.typ) && !is_binary(Eptr->info.typ) && !has_ns(Eptr->info.typ) && !is_untyped(Eptr->info.typ))
				fprintf(fout, "\n\tif(arrayType && !soap_match_tag(soap, arrayType, \"%s\"))", xsi_type(Eptr->info.typ));
			else
				fprintf(fout, "\n\tif(type && !soap_match_tag(soap, type, \"%s\"))", the_type(Eptr->info.typ));
			fprintf(fout, " {\n\t\tcp->type = %s;", soap_type(Eptr->info.typ));
			fprintf(fout, "\n\t\tif(n < 0) {");
			fprintf(fout, "\t\t\tcp->ptr = SOAP_NEW(%s);", c_type(Eptr->info.typ)); // @v10.2.12 (void*)SOAP_NEW-->SOAP_NEW
			fprintf(fout, "\n\t\t\tif(!cp->ptr) {\n\t\t\t\tsoap->error = SOAP_EOM;\n\t\t\t\treturn NULL;\n\t\t\t}");
			//fprintf(fout, "\n\t\t\tif(size)\n\t\t\t\t*size = sizeof(%s);", c_type(Eptr->info.typ));
			fprintf(fout, "\n\t\t\tASSIGN_PTR(size, sizeof(%s));", c_type(Eptr->info.typ));
			if((s = has_soapref(Eptr->info.typ))) {
				//fprintf(fout, "\n\t\t\t((%s*)cp->ptr)->%s = soap;", c_type(Eptr->info.typ), s);
				fprintf(fout, "\n\t\t\tstatic_cast<%s *>(cp->ptr)->%s = soap;", c_type(Eptr->info.typ), s);
			}
			fprintf(fout, "\n\t\t}\n\t\telse {");
			fprintf(fout, "\n\t\t\tcp->ptr = SOAP_NEW(%s[n]);", c_type(Eptr->info.typ)); // @v10.2.12 (void*)SOAP_NEW-->SOAP_NEW
			//fprintf(fout, "\n\t\t\tif(size)\n\t\t\t\t*size = n * sizeof(%s);", c_type(Eptr->info.typ));
			fprintf(fout, "\n\t\t\tASSIGN_PTR(size, n * sizeof(%s));", c_type(Eptr->info.typ));
			if(s) {
				//fprintf(fout, "\n\t\t\tfor(int i = 0; i < n; i++)\n\t\t\t\t((%s*)cp->ptr)[i].%s = soap;", c_type(Eptr->info.typ), s);
				fprintf(fout, "\n\t\t\tfor(int i = 0; i < n; i++)\n\t\t\t\tstatic_cast<%s *>(cp->ptr)[i].%s = soap;", c_type(Eptr->info.typ), s);
			}
			fprintf(fout, "\n\t\t}");
			fprintf(fout, "\n\tDBGLOG(TEST, SOAP_MESSAGE(fdebug, \"Instantiated location=%%p\\n\", cp->ptr));");
			fprintf(fout, "\n\t\treturn (%s*)cp->ptr;", c_type(Eptr->info.typ));
			fprintf(fout, "\n\t}");
			derclass = 0;
		}
	}
	fprintf(fout, "\n\tif(n < 0)");
	fprintf(fout, " {\n\t\tcp->ptr = SOAP_NEW(%s);", c_type(typ)); // @v10.2.12 (void*)SOAP_NEW-->SOAP_NEW
	//fprintf(fout, "\n\t\tif(size)\n\t\t\t*size = sizeof(%s);", c_type(typ));
	fprintf(fout, "\n\t\tASSIGN_PTR(size, sizeof(%s));", c_type(typ));
	if((s = has_soapref(typ))) {
		//fprintf(fout, "\n\t\t((%s*)cp->ptr)->%s = soap;", c_type(typ), s);
		fprintf(fout, "\n\t\tstatic_cast<%s *>(cp->ptr)->%s = soap;", c_type(typ), s);
	}
	fprintf(fout, "\n\t}\n\telse");
	fprintf(fout, " {\n\t\tcp->ptr = SOAP_NEW(%s[n]);", c_type(typ)); // @v10.2.12 (void*)SOAP_NEW-->SOAP_NEW
	fprintf(fout, "\n\t\tif(!cp->ptr) {\n\t\t\tsoap->error = SOAP_EOM;\n\t\t\treturn NULL;\n\t\t}");
	//fprintf(fout, "\n\t\tif(size)\n\t\t\t*size = n * sizeof(%s);", c_type(typ));
	fprintf(fout, "\n\t\tASSIGN_PTR(size, n * sizeof(%s));", c_type(typ));
	if(s) {
		//fprintf(fout, "\n\t\tfor(int i = 0; i < n; i++)\n\t\t\t((%s*)cp->ptr)[i].%s = soap;", c_type(typ), s);
		fprintf(fout, "\n\t\tfor(int i = 0; i < n; i++)\n\t\t\tstatic_cast<%s *>(cp->ptr)[i].%s = soap;", c_type(typ), s);
	}
	fprintf(fout, "\n\t}");
	fprintf(fout, "\n\tDBGLOG(TEST, SOAP_MESSAGE(fdebug, \"Instantiated location=%%p\\n\", cp->ptr));");
	fprintf(fout, "\n\treturn static_cast<%s *>(cp->ptr);", c_type(typ));
	fprintf(fout, "\n}");
	/* extern "C" causes C++ namespace linking issues */
	/* fprintf(fhead,"\n#ifdef __cplusplus\nextern \"C\" {\n#endif"); */
	fprintf(fhead, "\nSOAP_FMAC3 void SOAP_FMAC4 soap_copy_%s(struct soap*, int, int, void*, size_t, const void*, size_t);", c_ident(typ));
	/* fprintf(fhead,"\n#ifdef __cplusplus\n}\n#endif"); */
	/* fprintf(fout,"\n\n#ifdef __cplusplus\nextern \"C\" {\n#endif"); */
	fprintf(fout, "\n\nSOAP_FMAC3 void SOAP_FMAC4 soap_copy_%s(struct soap *soap, int st, int tt, void *p, size_t len, const void *q, size_t n)", c_ident(typ));
	fprintf(fout, "\n{\n\t(void)soap; (void)tt; (void)st; (void)len; (void)n; /* appease -Wall -Werror */");
	fprintf(fout, "\n\tDBGLOG(TEST, SOAP_MESSAGE(fdebug, \"Copying %s %%p -> %%p\\n\", q, p));", c_type(typ));
	fprintf(fout, "\n\t*static_cast<%s *>(p) = *static_cast<const %s *>(q);\n}", c_type(typ), c_type(typ));
	/* fprintf(fout,"\n#ifdef __cplusplus\n}\n#endif"); */
}

int get_dimension(Tnode * typ)
{
	return (((Tnode *)typ->ref)->width) ? typ->width/((Tnode *)typ->ref)->width : 0;
}

void soap_serialize(Tnode * typ)
{
	int d;
	Table * table, * t;
	Entry * p;
	Tnode * temp;
	int cardinality;
	if(is_primitive(typ))
		return;
	if(typ->type != Tclass ||
	   !(typ->sym &&
	     (is_stdstring(typ) ||
	      is_stdwstring(typ)) && is_eq(typ->sym->name, "xsd__QName")) || is_external(typ) || is_imported(typ))
		if(is_typedef(typ) && !is_external(typ)) {
			if(typ->type == Tclass && !is_stdstring(typ) && !is_stdwstring(typ) && !is_volatile(typ))
				fprintf(fhead, "\n\n#define soap_serialize_%s(soap, a) (a)->soap_serialize(soap)\n", c_ident(typ));
			else if(typ->type == Tclass && is_eq(typ->sym->name, "xsd__QName"))
				fprintf(fhead, "\n\n#define soap_serialize_%s(soap, a) soap_serialize_std__string(soap, a)\n", c_ident(typ));
			else
				fprintf(fhead, "\n\n#define soap_serialize_%s(soap, a) soap_serialize_%s(soap, a)\n", c_ident(typ), t_ident(typ));
			return;
		}
	if((p = is_dynamic_array(typ))) {
		if(typ->type == Tclass && !is_volatile(typ)) {
			if(is_external(typ))
				return;
			fprintf(fout, "\n\nvoid %s::soap_serialize(struct soap *soap) const\n{", c_ident(typ));
			if(is_binary(typ)) {
				if(is_attachment(typ)) {
					fprintf(fout, "\n\tif(this->__ptr && !soap_array_reference(soap, this, (struct soap_array*)&this->__ptr, 1, %s))",
						soap_type(typ));
					fprintf(fout, "\n\t\tif(this->id || this->type)\n\t\t\tsoap->mode |= SOAP_ENC_DIME;\n}");
				}
				else
					fprintf(fout, "\n\tif(this->__ptr)\n\t\tsoap_array_reference(soap, this, (struct soap_array*)&this->%s, 1, %s);\n}",
						ident(p->sym->name), soap_type(typ));
				fflush(fout);
				return;
			}
			else {
				d = get_Darraydims(typ);
				if(d) {
					fprintf(fout, "\n\tif(this->%s && !soap_array_reference(soap, this, (struct soap_array*)&this->%s, %d, %s))",
						ident(p->sym->name), ident(p->sym->name), d, soap_type(typ));
					fprintf(fout, "\n\t\tfor(int i = 0; i < soap_size(this->__size, %d); i++)", d);
				}
				else {
					fprintf(fout, "\n\tif(this->%s && !soap_array_reference(soap, this, (struct soap_array*)&this->%s, 1, %s))",
					      ident(p->sym->name), ident(p->sym->name), soap_type(typ));
				      fprintf(fout, "\n\t\tfor(int i = 0; i < this->__size; i++)"); 
				}
				fprintf(fout, "\n\t\t{");
				if(has_ptr((Tnode *)p->info.typ->ref))
					fprintf(fout, "\tsoap_embedded(soap, this->%s + i, %s);", ident(p->sym->name), soap_type((Tnode *)p->info.typ->ref));
				if(((Tnode *)p->info.typ->ref)->type == Tclass && !is_XML((Tnode *)p->info.typ->ref) &&
				   !is_external((Tnode *)p->info.typ->ref) && !is_volatile((Tnode *)p->info.typ->ref) &&
				   !is_typedef((Tnode *)p->info.typ->ref))
					fprintf(fout, "\n\t\t\tthis->%s[i].soap_serialize(soap);", ident(p->sym->name));
				else if(!is_XML((Tnode *)p->info.typ->ref) &&!is_primitive((Tnode *)p->info.typ->ref))
					fprintf(fout, "\n\t\t\tsoap_serialize_%s(soap, this->%s + i);",
						c_ident((Tnode *)p->info.typ->ref), ident(p->sym->name));
				fprintf(fout, "\n\t\t}\n}");
				return;
			}
		}
		else {
			if(is_external(typ)) {
			      fprintf(fhead, "\nSOAP_FMAC1 void SOAP_FMAC2 soap_serialize_%s(struct soap*, %s);", c_ident(typ), c_type_id(typ, "const*"));
			      return;
		      }
		      fprintf(fhead, "\nSOAP_FMAC3 void SOAP_FMAC4 soap_serialize_%s(struct soap*, %s);", c_ident(typ), c_type_id(typ, "const*"));
		      fprintf(fout, "\n\nSOAP_FMAC3 void SOAP_FMAC4 soap_serialize_%s(struct soap *soap, %s)\n{", c_ident(typ), c_type_id(typ, "const*a"));
		      if(is_binary(typ)) {
			      if(is_attachment(typ)) {
				      fprintf(fout, "\n\tif(a->%s && !soap_array_reference(soap, a, (struct soap_array*)&a->%s, 1, %s))",
					      ident(p->sym->name), ident(p->sym->name), soap_type(typ));
				      fprintf(fout, "\n\t\tif(a->id || a->type)\n\t\t\tsoap->mode |= SOAP_ENC_DIME;\n}");
			      }
			      else
				      fprintf(fout, "\n\tif(a->%s)\n\t\tsoap_array_reference(soap, a, (struct soap_array*)&a->%s, 1, %s);\n}",
					      ident(p->sym->name), ident(p->sym->name), soap_type(typ));
			      fflush(fout);
			      return;
		      }
		      else {
			      //fprintf(fout, "\n\tint i;");
			      d = get_Darraydims(typ);
			      if(d) {
				      fprintf(fout, "\n\tif(a->%s && !soap_array_reference(soap, a, (struct soap_array*)&a->%s, %d, %s))",
					      ident(p->sym->name), ident(p->sym->name), d, soap_type(typ));
				      fprintf(fout, "\n\t\tfor(int i = 0; i < soap_size(a->__size, %d); i++)", d);
			      }
			      else {
					  fprintf(fout, "\n\tif(a->%s && !soap_array_reference(soap, a, (struct soap_array*)&a->%s, 1, %s))", ident(p->sym->name), ident(p->sym->name), soap_type(typ));
						fprintf(fout, "\n\t\tfor(int i = 0; i < a->__size; i++)"); 
				  }
			      fprintf(fout, "\n\t\t{");
			      if(has_ptr((Tnode *)p->info.typ->ref))
				      fprintf(fout, "\tsoap_embedded(soap, a->%s + i, %s);", ident(p->sym->name), soap_type((Tnode *)p->info.typ->ref));
			      if(((Tnode *)p->info.typ->ref)->type == Tclass && !is_XML((Tnode *)p->info.typ->ref) &&
			         !is_external((Tnode *)p->info.typ->ref) && !is_volatile((Tnode *)p->info.typ->ref) &&
			         !is_typedef((Tnode *)p->info.typ->ref))
				      fprintf(fout, "\n\t\t\ta->%s[i].soap_serialize(soap);", ident(p->sym->name));
			      else if(!is_XML((Tnode *)p->info.typ->ref) && !is_primitive((Tnode *)p->info.typ->ref))
				      fprintf(fout, "\n\t\t\tsoap_serialize_%s(soap, a->%s + i);", c_ident((Tnode *)p->info.typ->ref), ident(p->sym->name));
			      fprintf(fout, "\n\t\t}\n}");
			      fflush(fout);
			      return;
		      }
		}
	}
	if(is_stdstring(typ) || is_stdwstring(typ)) {
		fprintf(fhead, "\nSOAP_FMAC3 void SOAP_FMAC4 soap_serialize_%s(struct soap*, const %s);", c_ident(typ), c_type_id(typ, "*"));
		fprintf(fout, "\n\nSOAP_FMAC3 void SOAP_FMAC4 soap_serialize_%s(struct soap *soap, const %s)\n{\n\t(void)soap; (void)a; /* appease -Wall -Werror */\n}",
			c_ident(typ), c_type_id(typ, "*a"));
		return;
	}
	switch(typ->type) {
	    case Tclass:
		if(!is_volatile(typ)) {
			if(is_external(typ)) {
				fprintf(fhead, "\nSOAP_FMAC1 void SOAP_FMAC2 soap_serialize_%s(struct soap*, const %s);", c_ident(typ), c_type_id(typ, "*"));
				return;
			}
			table = (Table *)typ->ref;
			fprintf(fout, "\n\nvoid %s::soap_serialize(struct soap *soap) const\n{", ident(typ->id->name));
			// @v9.8.9 fprintf(fout, "\n\t(void)soap; /* appease -Wall -Werror */");
			for(t = table; t; t = t->prev) {
				for(p = t->list; p; p = p->next) {
					if(p->info.sto&(Sconst|Sprivate|Sprotected))
						fprintf(fout, "\n\t/* non-serializable %s skipped */", ident(p->sym->name));
					else if(is_transient(p->info.typ))
						fprintf(fout, "\n\t/* transient %s skipped */", ident(p->sym->name));
					else if(p->info.sto&Sattribute)
						;
					else if(is_repetition(p)) {
						if(!is_XML(p->next->info.typ)) {
							fprintf(fout, "\n\tif(this->%s::%s)", ident(t->sym->name), ident(p->next->sym->name));
							fprintf(fout, " {\n\t\tfor(int i = 0; i < this->%s::%s; i++) {", ident(t->sym->name), ident(p->sym->name));
							if(!is_invisible(p->next->sym->name))
								if(has_ptr((Tnode *)p->next->info.typ->ref))
									fprintf(fout, "\n\t\t\tsoap_embedded(soap, this->%s::%s + i, %s);",
										ident(t->sym->name), ident(p->next->sym->name), soap_type((Tnode *)p->next->info.typ->ref));
							if(((Tnode *)p->next->info.typ->ref)->type == Tclass && !is_external((Tnode *)p->next->info.typ->ref) &&
							   !is_volatile((Tnode *)p->next->info.typ->ref) && !is_typedef((Tnode *)p->next->info.typ->ref))
								fprintf(fout, "\n\t\t\tthis->%s::%s[i].soap_serialize(soap);", ident(t->sym->name), ident(p->next->sym->name));
							else if(!is_primitive((Tnode *)p->next->info.typ->ref))
								fprintf(fout, "\n\t\t\tsoap_serialize_%s(soap, this->%s::%s + i);",
									c_ident((Tnode *)p->next->info.typ->ref), ident(t->sym->name), ident(p->next->sym->name));
							fprintf(fout, "\n\t\t}\n\t}");
						}
						p = p->next;
					}
					else if(is_anytype(p)) {
						fprintf(fout, "\n\tsoap_markelement(soap, this->%s, this->%s);",
							ident(p->next->sym->name), ident(p->sym->name));
						p = p->next;
					}
					else if(is_choice(p)) {
						fprintf(fout, "\n\tsoap_serialize_%s(soap, this->%s::%s, &this->%s::%s);",
							c_ident(p->next->info.typ), ident(t->sym->name), ident(p->sym->name), ident(t->sym->name), ident(p->next->sym->name));
						p = p->next;
					}
					else if(p->info.typ->type==Tarray) {
						if(has_ptr(p->info.typ))
							fprintf(fout, "\n\tsoap_embedded(soap, this->%s::%s, %s);", ident(t->sym->name), ident(p->sym->name), soap_type(p->info.typ));
						fprintf(fout, "\n\tsoap_serialize_%s(soap, this->%s::%s);", c_ident(p->info.typ), ident(t->sym->name), ident(p->sym->name));
					}
					else if(p->info.typ->type==Tclass && !is_external(p->info.typ) && !is_volatile(p->info.typ) && !is_typedef(p->info.typ)) {
						if(has_ptr(p->info.typ))
							fprintf(fout, "\n\tsoap_embedded(soap, &this->%s::%s, %s);",
								ident(t->sym->name), ident(p->sym->name), soap_type(p->info.typ));
						fprintf(fout, "\n\tthis->%s::%s.soap_serialize(soap);", ident(t->sym->name), ident(p->sym->name));
					}
					else if(p->info.typ->type != Tfun && !is_void(p->info.typ) && !is_XML(p->info.typ)) {
						if(!is_template(p->info.typ))
							if(has_ptr(p->info.typ))
								fprintf(fout, "\n\tsoap_embedded(soap, &this->%s::%s, %s);", ident(t->sym->name), ident(p->sym->name),
									soap_type(p->info.typ));
						if(!is_primitive(p->info.typ))
							fprintf(fout, "\n\tsoap_serialize_%s(soap, &this->%s::%s);",
								c_ident(p->info.typ), ident(t->sym->name), ident(p->sym->name));
					}
				}
			}
			fprintf(fout, "\n}");
			break;
		}
	    case Tstruct:
		if(is_external(typ) && !is_volatile(typ)) {
			fprintf(fhead, "\nSOAP_FMAC1 void SOAP_FMAC2 soap_serialize_%s(struct soap*, const %s);",
				c_ident(typ), c_type_id(typ, "*"));
			return;
		}
		fprintf(fhead, "\nSOAP_FMAC3 void SOAP_FMAC4 soap_serialize_%s(struct soap*, const %s);", c_ident(
				typ), c_type_id(typ, "*"));
		if(!typ->ref)
			return;
		fprintf(fout, "\n\nSOAP_FMAC3 void SOAP_FMAC4 soap_serialize_%s(struct soap *soap, const %s)\n{",
			c_ident(typ), c_type_id(typ, "*a"));
		/* DYNAMIC ARRAY */
		// @v9.8.9 fprintf(fout, "\n\t(void)soap; (void)a; /* appease -Wall -Werror */");
		table = (Table *)typ->ref;
		for(t = table; t; t = t->prev) {
			for(p = t->list; p; p = p->next)                                 {
				if(p->info.sto&(Sconst|Sprivate|Sprotected))
					fprintf(fout, "\n\t/* non-serializable %s skipped */", ident(p->sym->name));
				else if(is_transient(p->info.typ))
					fprintf(fout, "\n\t/* transient %s skipped */", ident(p->sym->name));
				else if(p->info.sto&Sattribute)
					;
				else if(is_repetition(p)) {
					if(!is_XML(p->next->info.typ)) {
						fprintf(fout, "\n\tif(a->%s)", ident(p->next->sym->name));
						fprintf(fout, " {\n\t\tint i;\n\t\tfor(i = 0; i < a->%s; i++)\n\t\t{", ident(p->sym->name));
						if(!is_invisible(p->next->sym->name))
							if(has_ptr((Tnode *)p->next->info.typ->ref))
								fprintf(fout, "\n\t\t\tsoap_embedded(soap, a->%s + i, %s);", ident(p->next->sym->name), soap_type((Tnode *)p->next->info.typ->ref));
						if(((Tnode *)p->next->info.typ->ref)->type == Tclass && !is_external((Tnode *)p->next->info.typ->ref) &&
						   !is_volatile((Tnode *)p->next->info.typ->ref) && !is_typedef((Tnode *)p->next->info.typ->ref))
							fprintf(fout, "\n\t\t\ta->%s[i].soap_serialize(soap);", ident(p->next->sym->name));
						else if(!is_primitive((Tnode *)p->next->info.typ->ref))
							fprintf(fout, "\n\t\t\tsoap_serialize_%s(soap, a->%s + i);", c_ident((Tnode *)p->next->info.typ->ref), ident(p->next->sym->name));
						fprintf(fout, "\n\t\t}\n\t}");
					}
					p = p->next;
				}
				else if(is_anytype(p)) {
					fprintf(fout, "\n\tsoap_markelement(soap, a->%s, a->%s);", ident(p->next->sym->name), ident(p->sym->name));
					p = p->next;
				}
				else if(is_choice(p)) {
					fprintf(fout, "\n\tsoap_serialize_%s(soap, a->%s, &a->%s);",
						c_ident(p->next->info.typ), ident(p->sym->name), ident(p->next->sym->name));
					p = p->next;
				}
				else if(p->info.typ->type==Tarray) {
					if(has_ptr(p->info.typ))
						fprintf(fout, "\n\tsoap_embedded(soap, a->%s, %s);", ident(p->sym->name), soap_type(p->info.typ));
					fprintf(fout, "\n\tsoap_serialize_%s(soap, a->%s);", c_ident(p->info.typ), ident(p->sym->name));
				}
				else if(p->info.typ->type == Tclass && !is_external(p->info.typ) && !is_volatile(p->info.typ) && !is_typedef(p->info.typ)) {
					if(has_ptr(p->info.typ))
						fprintf(fout, "\n\tsoap_embedded(soap, &a->%s, %s);", ident(p->sym->name), soap_type(p->info.typ));
					fprintf(fout, "\n\ta->%s.soap_serialize(soap);", ident(p->sym->name));
				}
				else if(p->info.typ->type != Tfun && !is_void(p->info.typ) && !is_XML(p->info.typ)) {
					if(!is_template(p->info.typ))
						if(has_ptr(p->info.typ))
							fprintf(fout, "\n\tsoap_embedded(soap, &a->%s, %s);", ident(p->sym->name), soap_type(p->info.typ));
					if(!is_primitive(p->info.typ))
						fprintf(fout, "\n\tsoap_serialize_%s(soap, &a->%s);", c_ident(p->info.typ), ident(p->sym->name));
				}
			}
		}
		fprintf(fout, "\n}");
		break;

	    case Tunion:
		if(is_external(typ) && !is_volatile(typ)) {
			fprintf(fhead, "\nSOAP_FMAC1 void SOAP_FMAC2 soap_serialize_%s(struct soap*, int, const %s);", c_ident(typ), c_type_id(typ, "*"));
			return;
		}
		table = (Table *)typ->ref;
		fprintf(fhead, "\nSOAP_FMAC3 void SOAP_FMAC4 soap_serialize_%s(struct soap*, int, const %s);",
			c_ident(typ), c_type_id(typ, "*"));
		fprintf(fout, "\n\nSOAP_FMAC3 void SOAP_FMAC4 soap_serialize_%s(struct soap *soap, int choice, const %s)\n{",
			c_ident(typ), c_type_id(typ, "*a"));
		// @v9.8.9 fprintf(fout, "\n\t(void)soap; (void)a; /* appease -Wall -Werror */");
		fprintf(fout, "\n\tswitch(choice) {\n\t");
		for(t = table; t; t = t->prev) {
			for(p = t->list; p; p = p->next) {
				if(p->info.sto&(Sconst|Sprivate|Sprotected))
					fprintf(fout, "\n\t/* non-serializable %s skipped */", ident(p->sym->name));
				else if(is_transient(p->info.typ))
					fprintf(fout, "\n\t/* transient %s skipped */", ident(p->sym->name));
				else if(p->info.sto&Sattribute)
					;
				else if(is_repetition(p))
					;
				else if(is_anytype(p))
					;
				else if(p->info.typ->type==Tarray) {
					fprintf(fout, "\n\tcase SOAP_UNION_%s_%s:", c_ident(typ), ident(p->sym->name));
					if(has_ptr(p->info.typ))
						fprintf(fout, "\n\t\tsoap_embedded(soap, a->%s, %s);", ident(p->sym->name), soap_type(p->info.typ));
					fprintf(fout, "\n\t\tsoap_serialize_%s(soap, a->%s);", c_ident(p->info.typ), ident(p->sym->name));
					fprintf(fout, "\n\t\tbreak;");
				}
				else if(p->info.typ->type == Tclass && !is_external(p->info.typ) && !is_volatile(p->info.typ) && !is_typedef(p->info.typ)) {
					fprintf(fout, "\n\tcase SOAP_UNION_%s_%s:", c_ident(typ), ident(p->sym->name));
					if(has_ptr(p->info.typ))
						fprintf(fout, "\n\t\tsoap_embedded(soap, &a->%s, %s);", ident(p->sym->name), soap_type(p->info.typ));
					fprintf(fout, "\n\t\ta->%s.soap_serialize(soap);", ident(p->sym->name));
					fprintf(fout, "\n\t\tbreak;");
				}
				else if(p->info.typ->type != Tfun && !is_void(p->info.typ) && !is_XML(p->info.typ)) {
					fprintf(fout, "\n\tcase SOAP_UNION_%s_%s:", c_ident(typ), ident(p->sym->name));
					if(has_ptr(p->info.typ))
						fprintf(fout, "\n\t\tsoap_embedded(soap, &a->%s, %s);", ident(p->sym->name), soap_type(p->info.typ));
					if(!is_primitive(p->info.typ))
						fprintf(fout, "\n\t\tsoap_serialize_%s(soap, &a->%s);", c_ident(p->info.typ), ident(p->sym->name));
					fprintf(fout, "\n\t\tbreak;");
				}
			}
		}
		fprintf(fout, "\n\tdefault:\n\t\tbreak;\n\t}\n}");
		break;
	    case Tpointer:
		if(((Tnode *)typ->ref)->type == Tclass && !is_external((Tnode *)typ->ref) &&
		   !is_volatile((Tnode *)typ->ref) && !is_typedef((Tnode *)typ->ref)) {
			if(is_external(typ))
			{
				fprintf(fhead, "\nSOAP_FMAC1 void SOAP_FMAC2 soap_serialize_%s(struct soap*, %s);", c_ident(typ), c_type_id(typ, "const*"));
				return;
			}
			fprintf(fhead, "\nSOAP_FMAC3 void SOAP_FMAC4 soap_serialize_%s(struct soap*, %s);", c_ident(typ), c_type_id(typ, "const*"));
			fprintf(fout, "\n\nSOAP_FMAC3 void SOAP_FMAC4 soap_serialize_%s(struct soap *soap, %s)\n{", c_ident(typ), c_type_id(typ, "const*a"));
			p = is_dynamic_array((Tnode *)typ->ref);
			if(p) {
				d = get_Darraydims((Tnode *)typ->ref);
				if(d)
					fprintf(fout, "\n\tif(*a)");
				else
					fprintf(fout, "\n\tif(*a)");
			}
			else
				fprintf(fout, "\n\tif(!soap_reference(soap, *a, %s))", soap_type((Tnode *)typ->ref));
			fprintf(fout, "\n\t\t(*a)->soap_serialize(soap);\n}");
			break;
		}
		else {
			if(is_external(typ)) {
				fprintf(fhead, "\nSOAP_FMAC1 void SOAP_FMAC2 soap_serialize_%s(struct soap*, %s);", c_ident(typ), c_type_id(typ, "const*"));
				return;
			}
			fprintf(fhead, "\nSOAP_FMAC3 void SOAP_FMAC4 soap_serialize_%s(struct soap*, %s);", c_ident(typ), c_type_id(typ, "const*"));
			fprintf(fout, "\n\nSOAP_FMAC3 void SOAP_FMAC4 soap_serialize_%s(struct soap *soap, %s)\n{", c_ident(typ), c_type_id(typ, "const*a"));
			if(is_string(typ) || is_wstring(typ))
				fprintf(fout, "\n\tsoap_reference(soap, *a, %s);\n}", soap_type(typ));
			else if(is_primitive((Tnode *)typ->ref))
				fprintf(fout, "\n\tsoap_reference(soap, *a, %s);\n}", soap_type((Tnode *)typ->ref));
			else if((p = is_dynamic_array((Tnode *)typ->ref)) != NULL) {
				d = get_Darraydims((Tnode *)typ->ref);
				if(d)
					fprintf(fout, "\n\tif(*a)");
				else
					fprintf(fout, "\n\tif(*a)");
				fprintf(fout, "\n\t\tsoap_serialize_%s(soap, *a);\n}", c_ident((Tnode *)typ->ref));
			}
			else {fprintf(fout, "\n\tif(!soap_reference(soap, *a, %s))", soap_type((Tnode *)typ->ref));
			      fprintf(fout, "\n\t\tsoap_serialize_%s(soap, *a);\n}", c_ident((Tnode *)typ->ref)); }
			break;
		}
	    case Tarray:
		if(is_external(typ)) {
			fprintf(fhead, "\nSOAP_FMAC1 void SOAP_FMAC2 soap_serialize_%s(struct soap*, %s);", c_ident(
					typ), c_type_id(typ, "const"));
			return;
		}
		fprintf(fhead, "\nSOAP_FMAC3 void SOAP_FMAC4 soap_serialize_%s(struct soap*, %s);", c_ident(typ), c_type_id(typ, "const"));
		fprintf(fout, "\n\nSOAP_FMAC3 void SOAP_FMAC4 soap_serialize_%s(struct soap *soap, %s)", c_ident(typ), c_type_id(typ, "const a"));
		if(is_primitive((Tnode *)typ->ref)) {
			fprintf(fout, "\n{");
			// @v9.8.9 fprintf(fout, "\n\t(void)soap; (void)a; /* appease -Wall -Werror */");
		}
		else {
			//fprintf(fout, "\n{\n\tint i;");
			//fprintf(fout, "\n\tfor(i = 0; i < %d; i++)", get_dimension(typ));
			fprintf(fout, "\n{");
			fprintf(fout, "\n\tfor(int i = 0; i < %d; i++)", get_dimension(typ));
		      temp = (Tnode *)typ->ref;;
		      cardinality = 1;
		      while(temp->type == Tarray) {
			      temp = (Tnode *)temp->ref;
			      cardinality++;
		      }
		      fprintf(fout, " {\n\t");
		      if(has_ptr((Tnode *)typ->ref)) {
			      fprintf(fout, "\tsoap_embedded(soap, a");
			      if(cardinality > 1)
				      fprintf(fout, "[i]");
			      else
				      fprintf(fout, "+i");
			      fprintf(fout, ", %s);", soap_type((Tnode *)typ->ref));
		      }
		      if(((Tnode *)typ->ref)->type == Tclass && !is_external((Tnode *)typ->ref) && !is_volatile((Tnode *)typ->ref) && !is_typedef((Tnode *)typ->ref)) {
			      fprintf(fout, "\n\ta[i].soap_serialize(soap)");
		      }
		      else if(!is_primitive((Tnode *)typ->ref)) {
			      fprintf(fout, "\n\tsoap_serialize_%s(soap, a", c_ident((Tnode *)typ->ref));
			      if(cardinality > 1) {
				      fprintf(fout, "[i])");
			      }
			      else {
				      fprintf(fout, "+i)");
			      }
		      }
		      fprintf(fout, ";\n\t}"); }
		fprintf(fout, "\n}");
		break;
	    case Ttemplate:
		if(is_external(typ)) {
			fprintf(fhead, "\nSOAP_FMAC1 void SOAP_FMAC2 soap_serialize_%s(struct soap*, const %s);", c_ident(typ), c_type_id(typ, "*"));
			return;
		}
		fprintf(fhead, "\nSOAP_FMAC3 void SOAP_FMAC4 soap_serialize_%s(struct soap*, const %s);", c_ident(typ), c_type_id(typ, "*"));
		temp = (Tnode *)typ->ref;
		if(!temp)
			return;
		fprintf(fout, "\n\nSOAP_FMAC3 void SOAP_FMAC4 soap_serialize_%s(struct soap *soap, const %s)\n{", c_ident(typ), c_type_id(typ, "*a"));
		if(!is_primitive(temp) && !is_XML(temp) && temp->type != Tfun && !is_void(temp)) {
			fprintf(fout, "\n\tfor(%s::const_iterator i = a->begin(); i != a->end(); ++i)", c_type(typ));
			if(temp->type==Tclass && !is_external(temp) && !is_volatile(temp) && !is_typedef(temp))
				fprintf(fout, "\n\t\t(*i).soap_serialize(soap);");
			else
				fprintf(fout, "\n\t\tsoap_serialize_%s(soap, &(*i));", c_ident(temp));
		}
		fprintf(fout, "\n}");
	    default:     break;
	}
}

void soap_default(Tnode * typ)
{
	int i, d;
	Table * table, * t;
	Entry * p;
	Tnode * temp;
	char * s;
	int cardinality;
	if(typ->type == Tpointer && !is_string(typ))
		return;
	if(typ->type != Tclass || !(typ->sym && (is_stdstring(typ) ||
	      is_stdwstring(typ)) && is_eq(typ->sym->name, "xsd__QName")) || is_external(typ) || is_imported(typ)) {
		if(is_typedef(typ) && !is_external(typ)) {
			if(typ->type == Tclass && !is_stdstring(typ) && !is_stdwstring(typ) && !is_volatile(typ))
				fprintf(fhead, "\n\n#define soap_default_%s(soap, a) (a)->%s::soap_default(soap)\n", c_ident(typ), t_ident(typ));
			else if(typ->type == Tclass && is_eq(typ->sym->name, "xsd__QName"))
				fprintf(fhead, "\n\n#define soap_default_%s(soap, a) soap_default_std__string(soap, a)\n", c_ident(typ));
			else
				fprintf(fhead, "\n\n#define soap_default_%s(soap, a) soap_default_%s(soap, a)\n", c_ident(typ), t_ident(typ));
			return;
		}
	}
	p = is_dynamic_array(typ);
	if(p) {
		if(typ->type == Tclass && !is_volatile(typ))        {
			if(is_external(typ))
				return;
			fprintf(fout, "\n\nvoid %s::soap_default(struct soap *soap)\n{", c_ident(typ));
			if((s = has_soapref(typ)))
				fprintf(fout, "\n\tthis->%s = soap;", s);
			else {
				// @v9.8.9 fprintf(fout, "\n\t(void)soap; /* appease -Wall -Werror */");
			}
			d = get_Darraydims(typ);
			if(d) {
				fprintf(fout, "\n\tthis->%s = NULL;", ident(p->sym->name));
				for(i = 0; i < d; i++) {
					fprintf(fout, "\n\tthis->__size[%d] = 0;", i);
					if(has_offset(typ) && (((Table *)typ->ref)->list->next->next->info.sto&Sconst) == 0)
						fprintf(fout, "\n\tthis->__offset[%d] = 0;", i);
				}
			}
			else {fprintf(fout, "\n\tthis->__size = 0;\n\tthis->%s = NULL;", ident(p->sym->name));
			      if(has_offset(typ) && (((Table *)typ->ref)->list->next->next->info.sto&Sconst) == 0)
				      fprintf(fout, "\n\tthis->__offset = 0;"); }
			if(is_attachment(typ))
				fprintf(fout, "\n\tthis->id = NULL;\n\tthis->type = NULL;\n\tthis->options = NULL;");
			fprintf(fout, "\n}");
		}
		else {
			if(is_external(typ)) {
			      fprintf(fhead, "\nSOAP_FMAC1 void SOAP_FMAC2 soap_default_%s(struct soap*, %s);", c_ident(typ), c_type_id(typ, "*"));
			      return;
		      }
		      fprintf(fhead, "\nSOAP_FMAC3 void /*SOAP_FMAC4*/FASTCALL soap_default_%s(struct soap*, %s);", c_ident(typ), c_type_id(typ, "*"));
		      fprintf(fout, "\n\nSOAP_FMAC3 void /*SOAP_FMAC4*/FASTCALL soap_default_%s(struct soap *soap, %s)\n{\n\t(void)soap;", c_ident(typ), c_type_id(typ, "*a"));
		      if((s = has_soapref(typ)))
			      fprintf(fout, "\n\ta->%s = soap;", s);
			  else {
			      // @v9.8.9 fprintf(fout, "\n\t(void)soap; /* appease -Wall -Werror */");
			  }
		      d = get_Darraydims(typ);
		      if(d) {
			      fprintf(fout, "\n\ta->%s = NULL;", ident(p->sym->name));
			      for(i = 0; i < d; i++) {
				      fprintf(fout, "\n\ta->__size[%d] = 0;", i);
				      if(has_offset(typ) && (((Table *)typ->ref)->list->next->next->info.sto&Sconst) ==
				         0)
					      fprintf(fout, "\n\ta->__offset[%d] = 0;", i);
			      }
		      }
		      else {fprintf(fout, "\n\ta->__size = 0;\n\ta->%s = NULL;", ident(p->sym->name));
			    if(has_offset(typ) && (((Table *)typ->ref)->list->next->next->info.sto&Sconst) == 0)
				    fprintf(fout, "\n\ta->__offset = 0;"); }
		      if(is_attachment(typ))
			      fprintf(fout, "\n\ta->id = NULL;\n\ta->type = NULL;\n\ta->options = NULL;");
		      fprintf(fout, "\n}"); }
		fflush(fout);
		return;
	}
	if(is_primitive(typ) || is_string(typ)) {
		if(is_external(typ))                                          {
			fprintf(fhead, "\nSOAP_FMAC1 void SOAP_FMAC2 soap_default_%s(struct soap*, %s);", c_ident(typ), c_type_id(typ, "*"));
			return;
		}
		fprintf(fhead, "\nSOAP_FMAC3 void /*SOAP_FMAC4*/FASTCALL soap_default_%s(struct soap*, %s);", c_ident(typ), c_type_id(typ, "*"));
		fprintf(fout,
			"\n\nSOAP_FMAC3 void /*SOAP_FMAC4*/FASTCALL soap_default_%s(struct soap *soap, %s)\n{\n\t(void)soap; /* appease -Wall -Werror */\n#ifdef SOAP_DEFAULT_%s\n\t*a = SOAP_DEFAULT_%s;\n#else\n\t*a = (%s)0;\n#endif\n}",
			c_ident(typ), c_type_id(typ, "*a"), c_ident(typ), c_ident(typ), c_type(typ));
		return;
	}
	if(is_fixedstring(typ)) {
		fprintf(fhead, "\nSOAP_FMAC3 void /*SOAP_FMAC4*/FASTCALL soap_default_%s(struct soap*, char[]);", c_ident(typ));
		fprintf(fout,  "\n\nSOAP_FMAC3 void /*SOAP_FMAC4*/FASTCALL soap_default_%s(struct soap *soap, char a[])\n{\n\t(void)soap; /* appease -Wall -Werror */\n\ta[0] = '\\0';\n}",
			c_ident(typ));
		return;
	}
	if(is_stdstring(typ) || is_stdwstring(typ)) {
		fprintf(fhead, "\nSOAP_FMAC3 void /*SOAP_FMAC4*/FASTCALL soap_default_%s(struct soap*, %s);", c_ident(typ), c_type_id(typ, "*"));
		fprintf(fout,  "\n\nSOAP_FMAC3 void /*SOAP_FMAC4*/FASTCALL soap_default_%s(struct soap *soap, %s)\n{\n\t(void)soap; /* appease -Wall -Werror */\n\tp->erase();\n}",
			c_ident(typ), c_type_id(typ, "*p"));
		return;
	}
	switch(typ->type) {
	    case Tclass:
		/* CLASS */
		if(!is_volatile(typ)) {
			if(is_external(typ)) {
				fprintf(fhead, "\nSOAP_FMAC1 void SOAP_FMAC2 soap_default_%s(struct soap*, %s);", c_ident(typ), c_type_id(typ, "*"));
				return;
			}
			table = (Table *)typ->ref;
			fprintf(fout, "\n\nvoid %s::soap_default(struct soap *soap)\n{", ident(typ->id->name));
			if((s = has_soapref(typ)))
				fprintf(fout, "\n\tthis->%s = soap;", s);
			else {
				// @v9.8.9 fprintf(fout, "\n\t(void)soap; /* appease -Wall -Werror */");
			}
			fflush(fout);
			for(t = table; t; t = t->prev) {
				for(p = t->list; p; p = p->next) {
					if(p->info.typ->type == Tfun)
						continue;
					if(p->info.sto&Sconst)
						fprintf(fout, "\n\t/* const %s skipped */", ident(p->sym->name));
					else if(is_choice(p)) {
						fprintf(fout, "\n\tthis->%s::%s = 0;", ident(t->sym->name), ident(p->sym->name));
						p = p->next;
					}
					else if(is_repetition(p) || is_anytype(p)) {
						fprintf(fout, "\n\tthis->%s::%s = 0;\n\tthis->%s::%s = NULL;",
							ident(t->sym->name), ident(p->sym->name), ident(t->sym->name), ident(p->next->sym->name));
						p = p->next;
					}
					else {
						if(is_fixedstring(p->info.typ)) {
							if(p->info.hasval)
								fprintf(fout, "\n\tstrcpy(this->%s::%s, \"%s\");", ident(t->sym->name), ident(p->sym->name), cstring(p->info.val.s));
							else
								fprintf(fout, "\n\tthis->%s::%s[0] = '\\0';", ident(t->sym->name), ident(p->sym->name));
						}
						else if(p->info.typ->type == Tarray) {
							fprintf(fout, "\n\tsoap_default_%s(soap, this->%s::%s);", c_ident(p->info.typ), ident(t->sym->name), ident(p->sym->name));
						}
						else if(p->info.typ->type == Tclass && !is_external(p->info.typ) &&
						        !is_volatile(p->info.typ) && !is_typedef(p->info.typ) && !is_transient(p->info.typ))
							fprintf(fout, "\n\tthis->%s::%s.%s::soap_default(soap);", ident(t->sym->name), ident(p->sym->name), c_ident(p->info.typ));
						else if(p->info.hasval) {
							if(p->info.typ->type == Tpointer && is_stdstring((Tnode *)p->info.typ->ref))
								fprintf( fout, "\n\tstatic std::string soap_tmp_%s(\"%s\");\n\tthis->%s::%s = &soap_tmp_%s;",
									ident(p->sym->name), p->info.val.s, ident(t->sym->name), ident(p->sym->name), ident(p->sym->name));
							else
								fprintf(fout, "\n\tthis->%s::%s%s;", ident(t->sym->name), ident(p->sym->name), c_init(p));
						}
						else if(is_transient(p->info.typ) || is_void(p->info.typ))
							fprintf(fout, "\n\t/* transient %s skipped */", ident(p->sym->name));
						else if(p->info.typ->type == Tpointer && (!is_string(p->info.typ) || is_XML(p->info.typ)))
							fprintf(fout, "\n\tthis->%s::%s = NULL;", ident(t->sym->name), ident(p->sym->name));
						else if(p->info.sto&(Sprivate|Sprotected)) {
							if(p->info.typ->type == Tpointer)
								fprintf(fout, "\n\tthis->%s::%s = NULL;", ident(t->sym->name), ident(p->sym->name));
							else if(p->info.typ->type >= Tchar && p->info.typ->type < Tenum)
								fprintf(fout, "\n\tthis->%s::%s = 0;", ident(t->sym->name), ident(p->sym->name));
							else if(p->info.typ->type == Tenum)
								fprintf(fout, "\n\tthis->%s::%s = (%s)0;", ident(t->sym->name), ident(p->sym->name), c_type(p->info.typ));
							else
								fprintf(fout, "\n\t/* private/protected %s skipped */", ident(p->sym->name));
						}
						else
							fprintf(fout, "\n\tsoap_default_%s(soap, &this->%s::%s);", c_ident(p->info.typ), ident(t->sym->name), ident(p->sym->name));
					}
				}
			}
			fprintf(fout, "\n}");
			fflush(fout);
			break;
		}
	    case Tstruct:
		table = (Table *)typ->ref;
		if(is_external(typ) && !is_volatile(typ)) {
			fprintf(fhead, "\nSOAP_FMAC1 void SOAP_FMAC2 soap_default_%s(struct soap*, %s);", c_ident(typ), c_type_id(typ, "*"));
			return;
		}
		fprintf(fhead, "\nSOAP_FMAC3 void /*SOAP_FMAC4*/FASTCALL soap_default_%s(struct soap*, %s);", c_ident(typ), c_type_id(typ, "*"));
		fprintf(fout, "\n\nSOAP_FMAC3 void /*SOAP_FMAC4*/FASTCALL soap_default_%s(struct soap *soap, %s)\n{", c_ident(typ), c_type_id(typ, "*a"));
		fflush(fout);
		if((s = has_soapref(typ)))
			fprintf(fout, "\n\ta->%s = soap;", s);
		else {
			// @v9.8.9 fprintf(fout, "\n\t(void)soap; (void)a; /* appease -Wall -Werror */");
		}
		for(t = table; t; t = t->prev) {
			for(p = t->list; p; p = p->next)                                 {
				if(p->info.typ->type == Tfun)
					continue;
				if(p->info.sto&Sconst)
					fprintf(fout, "\n\t/* const %s skipped */", ident(p->sym->name));
				else if(p->info.sto&(Sprivate|Sprotected))
					fprintf(fout, "\n\t/* private/protected %s skipped */", ident(p->sym->name));
				else if(is_choice(p)) {
					fprintf(fout, "\n\ta->%s = 0;", ident(p->sym->name));
					p = p->next;
				}
				else if(is_repetition(p) || is_anytype(p)) {
					fprintf(fout, "\n\ta->%s = 0;\n\ta->%s = NULL;", ident(p->sym->name),
						ident(p->next->sym->name));
					p = p->next;
				}
				else {
					if(is_fixedstring(p->info.typ)) {
						if(p->info.hasval)
							fprintf(fout, "\n\tstrcpy(a->%s, \"%s\");", ident(p->sym->name), cstring(p->info.val.s));
						else
							fprintf(fout, "\n\ta->%s[0] = '\\0';", ident(p->sym->name));
					}
					else if(p->info.typ->type==Tarray)
						fprintf(fout, "\n\tsoap_default_%s(soap, a->%s);", c_ident(p->info.typ), ident(p->sym->name));
					else if(p->info.typ->type == Tclass && !is_external(p->info.typ) && !is_volatile(p->info.typ) && !is_typedef(p->info.typ) && !is_transient(p->info.typ))
						fprintf(fout, "\n\ta->%s.%s::soap_default(soap);", ident(p->sym->name), c_ident(p->info.typ));
					else if(p->info.hasval) {
						if(p->info.typ->type == Tpointer && is_stdstring((Tnode *)p->info.typ->ref))
							fprintf(fout, "\n\tstatic std::string soap_tmp_%s(\"%s\");\n\ta->%s = &soap_tmp_%s;",
								ident(p->sym->name), p->info.val.s, ident(p->sym->name), ident(p->sym->name));
						else
							fprintf(fout, "\n\ta->%s%s;", ident(p->sym->name), c_init(p));
					}
					else if(is_transient(p->info.typ) || is_void(p->info.typ))
						fprintf(fout, "\n\t/* transient %s skipped */", ident(p->sym->name));
					else if(p->info.typ->type == Tpointer && (!is_string(p->info.typ) || is_XML(p->info.typ)))
						fprintf(fout, "\n\ta->%s = NULL;", ident(p->sym->name));
					else
						fprintf(fout, "\n\tsoap_default_%s(soap, &a->%s);", c_ident(p->info.typ), ident(p->sym->name));
				}
			}
		}
		fprintf(fout, "\n}");
		fflush(fout);
		break;
	    case Tarray:
		if(is_external(typ)) {
			fprintf(fhead, "\nSOAP_FMAC1 void SOAP_FMAC2 soap_default_%s(struct soap*, %s);", c_ident(typ), c_type(typ));
			return;
		}
		fprintf(fhead, "\nSOAP_FMAC3 void /*SOAP_FMAC4*/FASTCALL soap_default_%s(struct soap*, %s);", c_ident(typ), c_type(typ));
		fprintf(fout, "\n\nSOAP_FMAC3 void /*SOAP_FMAC4*/FASTCALL soap_default_%s(struct soap *soap, %s)\n{", c_ident(typ), c_type_id(typ, "a"));
		fprintf(fout, "\n\tint i;");
		// @v9.8.9 fprintf(fout, "\n\t(void)soap; /* appease -Wall -Werror */");
		fprintf(fout, "\n\tfor(i = 0; i < %d; i++)", get_dimension(typ));
		temp = (Tnode *)typ->ref;
		cardinality = 1;
		while(temp->type==Tarray) {
			temp = (Tnode *)temp->ref;
			cardinality++;
		}
		if(((Tnode *)typ->ref)->type == Tclass && !is_external((Tnode *)typ->ref) &&
		   !is_volatile((Tnode *)typ->ref)) {
			if(cardinality>1)
				fprintf(fout, "a[i].%s::soap_default(soap)", t_ident((Tnode *)typ->ref));
			else
				fprintf(fout, "(a+i)->soap_default(soap)");
		}
		else if(((Tnode *)typ->ref)->type == Tpointer)
			fprintf(fout, "\n\ta[i] = NULL");
		else {
			fprintf(fout, "\n\tsoap_default_%s(soap, a", c_ident((Tnode *)typ->ref));
			if(cardinality>1)
				fprintf(fout, "[i])");
			else
				fprintf(fout, "+i)");
		}
		fprintf(fout, ";\n}");
		break;

	    case Ttemplate:
		if(is_external(typ)) {
			fprintf(fhead, "\nSOAP_FMAC1 void SOAP_FMAC2 soap_default_%s(struct soap*, %s);", c_ident(typ), c_type_id(typ, "*"));
			return;
		}
		fprintf(fhead, "\nSOAP_FMAC3 void /*SOAP_FMAC4*/FASTCALL soap_default_%s(struct soap*, %s);", c_ident(typ), c_type_id(typ, "*"));
		fprintf(fout, "\n\nSOAP_FMAC3 void /*SOAP_FMAC4*/FASTCALL soap_default_%s(struct soap *soap, %s)\n{", c_ident(typ), c_type_id(typ, "*p"));
		fprintf(fout, "\n\tp->clear();");
		fprintf(fout, "\n}");
		fflush(fout);
		break;
	    default: break;
	}
}

void soap_traverse(Tnode * typ)
{
	int d;
	Table * table, * t;
	Entry * p;
	Tnode * temp;
	int cardinality;
	if(is_primitive_or_string(typ) || is_fixedstring(typ)) {
		fprintf(fhead, "\nSOAP_FMAC3 void SOAP_FMAC4 soap_traverse_%s(struct soap*, %s, const char *s, soap_walker p, soap_walker q);",
			c_ident(typ), c_type_id(typ, "*"));
		fprintf(fout, "\n\nSOAP_FMAC3 void SOAP_FMAC4 soap_traverse_%s(struct soap *soap, %s, const char *s, soap_walker p, soap_walker q)\n{\n\t(void)soap; (void)q; /* appease -Wall -Werror */",
			c_ident(typ), c_type_id(typ, "*a"));
		fprintf(fout, "\n\tif(p) p(soap, (void*)a, %s, s, \"%s\");", soap_type(typ), c_type(typ));
		fprintf(fout, "\n\tif(q) q(soap, (void*)a, %s, s, \"%s\");\n}", soap_type(typ), c_type(typ));
		return;
	}
	if(typ->type != Tclass || !(typ->sym && (is_stdstring(typ) || is_stdwstring(typ)) && is_eq(typ->sym->name, "xsd__QName")) || is_external(typ) || is_imported(typ))
		if(is_typedef(typ) && !is_external(typ)) {
			if(typ->type == Tclass && !is_stdstring(typ) && !is_stdwstring(typ) && !is_volatile(typ))
				fprintf(fhead, "\n\n#define soap_traverse_%s(soap, a, s, p, q) (a)->soap_traverse(soap, s, p, q)\n",
					c_ident(typ));
			else if(typ->type == Tclass && is_eq(typ->sym->name, "xsd__QName"))
				fprintf(fhead, "\n\n#define soap_traverse_%s(soap, a, s, p, q) soap_traverse_std__string(soap, a, s, p, q)\n",
					c_ident(typ));
			else
				fprintf(fhead, "\n\n#define soap_traverse_%s(soap, a, s, p, q) soap_traverse_%s(soap, a, s, p, q)\n",
					c_ident(typ), t_ident(typ));
			return;
		}
	if(is_XML(typ)) {
		fprintf(fhead, "\n\n#define soap_traverse_%s(soap, a, s, p, q) soap_traverse_%s(soap, a, s, p, q)\n", c_ident(typ), t_ident(typ));
		return;
	}
	if((p = is_dynamic_array(typ))) {
		if(typ->type == Tclass && !is_volatile(typ)) {
			if(is_external(typ))
				return;
			fprintf(fout, "\n\nvoid %s::soap_traverse(struct soap *soap, const char *s, soap_walker p, soap_walker q)\n{", c_ident(typ));
			if(is_binary(typ)) {
				fprintf(fout, "\n\tif(this->%s && !soap_array_reference(soap, this, (struct soap_array*)&this->%s, 1, %s)) {\n\t",
					ident(p->sym->name), ident(p->sym->name), soap_type(typ));
				fprintf(fout, "\n\t\tif(p) p(soap, (void*)this, %s, s, \"%s\");", soap_type(typ), c_type(typ));
				fprintf(fout, "\n\t\tif(p) p(soap, (void*)this->%s, 0, \"%s\", NULL);", ident(p->sym->name), p->sym->name);
				if(is_attachment(typ)) {
					fprintf(fout, "\n\t\tif(this->id || this->type)\n\t\t\tsoap->mode |= SOAP_ENC_DIME;\n}");
					fprintf(fout, "\n\t\tif(p) p(soap, (void*)this->id, SOAP_TYPE_string, \"id\", NULL);");
					fprintf(fout, "\n\t\tif(q) q(soap, (void*)this->id, SOAP_TYPE_string, \"id\", NULL);");
					fprintf(fout, "\n\t\tif(p) p(soap, (void*)this->type, SOAP_TYPE_string, \"type\", NULL);");
					fprintf(fout, "\n\t\tif(q) q(soap, (void*)this->type, SOAP_TYPE_string, \"type\", NULL);");
					fprintf(fout, "\n\t\tif(p) p(soap, (void*)this->options, 0, \"options\", NULL);");
					fprintf(fout, "\n\t\tif(q) q(soap, (void*)this->options, 0, \"options\", NULL);");
				}
				fprintf(fout, "\n\t\tif(q) q(soap, (void*)this->%s, 0, \"%s\", NULL);",
					ident(p->sym->name), p->sym->name);
				fprintf(fout, "\n\t\tif(q) q(soap, (void*)this, %s, s, \"%s\");\n\t}\n}",
					soap_type(typ), c_type(typ));
				fflush(fout);
				return;
			}
			else {
				d = get_Darraydims(typ);
				if(d) {
					fprintf(fout, "\n\tif(this->%s && !soap_array_reference(soap, this, (struct soap_array*)&this->%s, %d, %s)) {\n\t",
						ident(p->sym->name), ident(p->sym->name), d, soap_type(typ));
					fprintf(fout, "\n\t\tif(p) p(soap, (void*)this, %s, s, \"%s\");",
						soap_type(typ), c_type(typ));
					fprintf(fout, "\n\t\tfor(int i = 0; i < soap_size(this->__size, %d); i++)", d);
				}
				else {
					fprintf(fout, "\n\tif(this->%s && !soap_array_reference(soap, this, (struct soap_array*)&this->%s, 1, %s)) {\n\t",
					      ident(p->sym->name), ident(p->sym->name), soap_type(typ));
				      fprintf(fout, "\n\t\tif(p) p(soap, (void*)this, %s, s, \"%s\");", soap_type(typ), c_type(typ));
				      fprintf(fout, "\n\t\tfor(int i = 0; i < this->__size; i++)"); 
				}
				fprintf(fout, "\n\t\t{");
				if(has_ptr((Tnode *)p->info.typ->ref))
					fprintf(fout, "\tsoap_embedded(soap, this->%s + i, %s);", ident(
							p->sym->name), soap_type((Tnode *)p->info.typ->ref));
				if(((Tnode *)p->info.typ->ref)->type == Tclass && !is_external((Tnode *)p->info.typ->ref) &&
				   !is_volatile((Tnode *)p->info.typ->ref) && !is_typedef((Tnode *)p->info.typ->ref))
					fprintf(fout, "\n\t\t\tthis->%s[i].soap_traverse(soap, \"%s\", p, q);", ident(p->sym->name), p->sym->name);
				else
					fprintf(fout, "\n\t\t\tsoap_traverse_%s(soap, this->%s + i, \"%s\", p, q);",
						c_ident((Tnode *)p->info.typ->ref), ident(p->sym->name), ident(p->sym->name));
				fprintf(fout, "\n\t\t}\n\t\tif(q) q(soap, (void*)this, %s, s, \"%s\");", soap_type(
						typ), c_type(typ));
				fprintf(fout, "\n\t}\n}");
				return;
			}
		}
		else {
			if(is_external(typ)) {
			      fprintf(fhead, "\nSOAP_FMAC1 void SOAP_FMAC2 soap_traverse_%s(struct soap*, %s, const char *s, soap_walker p, soap_walker q);",
				      c_ident(typ), c_type_id(typ, "*"));
			      return;
		      }
		      fprintf(fhead, "\nSOAP_FMAC3 void SOAP_FMAC4 soap_traverse_%s(struct soap*, %s, const char *s, soap_walker p, soap_walker q);",
			      c_ident(typ), c_type_id(typ, "*"));
		      fprintf( fout, "\n\nSOAP_FMAC3 void SOAP_FMAC4 soap_traverse_%s(struct soap *soap, %s, const char *s, soap_walker p, soap_walker q)\n{",
			      c_ident(typ), c_type_id(typ, "*a"));
		      if(is_binary(typ)) {
			      fprintf(fout, "\n\tif(a->%s && !soap_array_reference(soap, a, (struct soap_array*)&a->%s, 1, %s)) {\n\t",
				      ident(p->sym->name), ident(p->sym->name), soap_type(typ));
			      fprintf(fout, "\n\t\tif(p) p(soap, (void*)a, %s, s, \"%s\");", soap_type(typ), c_type(typ));
			      fprintf(fout, "\n\t\tif(p) p(soap, (void*)a->%s, 0, \"%s\", NULL);", ident(p->sym->name), p->sym->name);
			      if(is_attachment(typ)) {
				      fprintf(fout, "\n\t\tif(a->id || a->type)\n\t\t\tsoap->mode |= SOAP_ENC_DIME;\n}");
				      fprintf(fout, "\n\t\tif(p) p(soap, (void*)a->id, SOAP_TYPE_string, \"id\", NULL);");
				      fprintf(fout, "\n\t\tif(q) q(soap, (void*)a->id, SOAP_TYPE_string, \"id\", NULL);");
				      fprintf(fout, "\n\t\tif(p) p(soap, (void*)a->type, SOAP_TYPE_string, \"type\", NULL);");
				      fprintf(fout, "\n\t\tif(q) q(soap, (void*)a->type, SOAP_TYPE_string, \"type\", NULL);");
				      fprintf(fout, "\n\t\tif(p) p(soap, (void*)a->options, 0, \"options\", NULL);");
				      fprintf(fout, "\n\t\tif(q) q(soap, (void*)a->options, 0, \"options\", NULL);");
			      }
			      fprintf(fout, "\n\t\tif(q) q(soap, (void*)a->%s, 0, \"%s\", NULL);", ident(p->sym->name), p->sym->name);
			      fprintf(fout, "\n\t\tif(q) q(soap, (void*)a, %s, s, \"%s\");\n\t}\n}", soap_type(typ), c_type(typ));
			      fflush(fout);
			      return;
		      }
		      else {
			      fprintf(fout, "\n\tint i;");
			      d = get_Darraydims(typ);
			      if(d) {
				      fprintf(fout, "\n\tif(a->%s && !soap_array_reference(soap, a, (struct soap_array*)&a->%s, %d, %s)) {\n\t",
					      ident(p->sym->name), ident(p->sym->name), d, soap_type(typ));
				      fprintf(fout, "\n\t\tif(p) p(soap, (void*)a, %s, s, \"%s\");", soap_type(typ), c_type(typ));
				      fprintf(fout, "\n\t\tfor(i = 0; i < soap_size(a->__size, %d); i++)", d);
			      }
			      else {
					  fprintf(fout, "\n\tif(a->%s && !soap_array_reference(soap, a, (struct soap_array*)&a->%s, 1, %s)) {\n\t",
					    ident(p->sym->name), ident(p->sym->name), soap_type(typ));
				    fprintf(fout, "\n\t\tif(p) p(soap, (void*)a, %s, s, \"%s\");", soap_type(typ), c_type(typ));
				    fprintf(fout, "\n\t\tfor(i = 0; i < a->__size; i++)"); }
			      fprintf(fout, "\n\t\t{");
			      if(has_ptr((Tnode *)p->info.typ->ref))
				      fprintf(fout, "\tsoap_embedded(soap, a->%s + i, %s);", ident(p->sym->name), soap_type((Tnode *)p->info.typ->ref));
			      if(((Tnode *)p->info.typ->ref)->type == Tclass &&
			         !is_external((Tnode *)p->info.typ->ref) && !is_volatile((Tnode *)p->info.typ->ref) &&
			         !is_typedef((Tnode *)p->info.typ->ref))
				      fprintf(fout, "\n\t\t\ta->%s[i].soap_traverse(soap, \"%s\", p, q);", ident(p->sym->name), p->sym->name);
			      else
				      fprintf(fout, "\n\t\t\tsoap_traverse_%s(soap, a->%s + i, \"%s\", p, q);",
					      c_ident((Tnode *)p->info.typ->ref), ident(p->sym->name), p->sym->name);
			      fprintf(fout, "\n\t\t}\n\t\tif(q) q(soap, (void*)a, %s, s, \"%s\");", soap_type(typ), c_type(typ));
			      fprintf(fout, "\n\t}\n}");
			      fflush(fout);
			      return;
		      }
		}
	}
	switch(typ->type) {
	    case Tclass:
		if(!is_volatile(typ)) {
			if(is_external(typ)) {
				fprintf(fhead, "\nSOAP_FMAC1 void SOAP_FMAC2 soap_traverse_%s(struct soap*, %s, const char *s, soap_walker p, soap_walker q);",
					c_ident(typ), c_type_id(typ, "*"));
				return;
			}
			table = (Table *)typ->ref;
			fprintf(fout, "\n\nvoid %s::soap_traverse(struct soap *soap, const char *s, soap_walker p, soap_walker q)\n{",
				ident(typ->id->name));
			// @v9.8.9 fprintf(fout, "\n\t(void)soap; /* appease -Wall -Werror */");
			fprintf(fout, "\n\tif(p) p(soap, (void*)this, %s, s, \"%s\");", soap_type(typ), c_type(typ));
			for(t = table; t; t = t->prev) {
				for(p = t->list; p; p = p->next) {
					if(p->info.sto&(Sconst|Sprivate|Sprotected))
						fprintf(fout, "\n\t/* non-serializable %s skipped */", ident(p->sym->name));
					else if(is_transient(p->info.typ))
						fprintf(fout, "\n\t/* transient %s skipped */", ident(p->sym->name));
					else if(p->info.sto&Sattribute)
						;
					else if(is_repetition(p)) {
						fprintf(fout, "\n\tif(this->%s::%s)", ident(t->sym->name), ident(p->next->sym->name));
						fprintf(fout, " {\n\t\tint i;\n\t\tfor(i = 0; i < this->%s::%s; i++)\n\t\t{", ident(t->sym->name), ident(p->sym->name));
						if(!is_invisible(p->next->sym->name))
							if(has_ptr((Tnode *)p->next->info.typ->ref))
								fprintf(fout, "\n\t\t\tsoap_embedded(soap, this->%s::%s + i, %s);", ident(t->sym->name), ident(p->next->sym->name),
									soap_type((Tnode *)p->next->info.typ->ref));
						if(((Tnode *)p->next->info.typ->ref)->type == Tclass && !is_external((Tnode *)p->next->info.typ->ref) &&
						   !is_volatile((Tnode *)p->next->info.typ->ref) && !is_typedef((Tnode *)p->next->info.typ->ref))
							fprintf(fout, "\n\t\t\tthis->%s::%s[i].soap_traverse(soap, \"%s\", p, q);", ident(t->sym->name), ident(p->next->sym->name), p->next->sym->name);
						else
							fprintf(fout, "\n\t\t\tsoap_traverse_%s(soap, this->%s::%s + i, \"%s\", p, q);",
								c_ident((Tnode *)p->next->info.typ->ref), ident(t->sym->name), ident(p->next->sym->name), p->next->sym->name);
						fprintf(fout, "\n\t\t}\n\t}");
						p = p->next;
					}
					else if(is_anytype(p)) {
						p = p->next;
					}
					else if(is_choice(p)) {
						fprintf(fout, "\n\tsoap_traverse_%s(soap, this->%s::%s, &this->%s::%s, \"%s\", p, q);",
							c_ident(p->next->info.typ), ident(t->sym->name), ident(p->sym->name), ident(t->sym->name), ident(p->next->sym->name), p->next->sym->name);
						p = p->next;
					}
					else if(p->info.typ->type==Tarray) {
						if(has_ptr(p->info.typ))
							fprintf(fout, "\n\tsoap_embedded(soap, this->%s::%s, %s);",
								ident(t->sym->name), ident(p->sym->name), soap_type(p->info.typ));
						fprintf(fout, "\n\tsoap_traverse_%s(soap, this->%s::%s, \"%s\", p, q);",
							c_ident(p->info.typ), ident(t->sym->name), ident(p->sym->name), p->sym->name);
					}
					else if(p->info.typ->type==Tclass && !is_external(p->info.typ) && !is_volatile(p->info.typ) && !is_typedef(p->info.typ)) {
						if(has_ptr(p->info.typ))
							fprintf(fout, "\n\tsoap_embedded(soap, &this->%s::%s, %s);",
								ident(t->sym->name), ident(p->sym->name), soap_type(p->info.typ));
						fprintf(fout, "\n\tthis->%s::%s.soap_traverse(soap, \"%s\", p, q);", ident(t->sym->name), ident(p->sym->name), p->sym->name);
					}
					else if(p->info.typ->type != Tfun && !is_void(p->info.typ)) {
						if(!is_template(p->info.typ))
							if(has_ptr(p->info.typ))
								fprintf(fout, "\n\tsoap_embedded(soap, &this->%s::%s, %s);",
									ident(t->sym->name), ident(p->sym->name), soap_type(p->info.typ));
						fprintf(fout, "\n\tsoap_traverse_%s(soap, &this->%s::%s, \"%s\", p, q);",
							c_ident(p->info.typ), ident(t->sym->name), ident(p->sym->name), p->sym->name);
					}
				}
			}
			fprintf(fout, "\n\tif(q) q(soap, (void*)this, %s, s, \"%s\");", soap_type(typ), c_type(typ));
			fprintf(fout, "\n}");
			break;
		}
	    case Tstruct:
		if(is_external(typ) && !is_volatile(typ)) {
			fprintf(fhead, "\nSOAP_FMAC1 void SOAP_FMAC2 soap_traverse_%s(struct soap*, %s, const char *s, soap_walker p, soap_walker q);",
				c_ident(typ), c_type_id(typ, "*"));
			return;
		}
		fprintf(fhead, "\nSOAP_FMAC3 void SOAP_FMAC4 soap_traverse_%s(struct soap*, %s, const char *s, soap_walker p, soap_walker q);",
			c_ident(typ), c_type_id(typ, "*"));
		if(!typ->ref)
			return;
		fprintf(fout, "\n\nSOAP_FMAC3 void SOAP_FMAC4 soap_traverse_%s(struct soap *soap, %s, const char *s, soap_walker p, soap_walker q)\n{",
			c_ident(typ), c_type_id(typ, "*a"));
		// @v9.8.9 fprintf(fout, "\n\t(void)soap; (void)a; /* appease -Wall -Werror */");
		fprintf(fout, "\n\tif(p) p(soap, (void*)a, %s, s, \"%s\");", soap_type(typ), c_type(typ));
		table = (Table *)typ->ref;
		for(t = table; t; t = t->prev) {
			for(p = t->list; p; p = p->next) {
				if(p->info.sto&(Sconst|Sprivate|Sprotected))
					fprintf(fout, "\n\t/* non-serializable %s skipped */", ident(p->sym->name));
				else if(is_transient(p->info.typ))
					fprintf(fout, "\n\t/* transient %s skipped */", ident(p->sym->name));
				else if(p->info.sto&Sattribute)
					;
				else if(is_repetition(p)) {
					fprintf(fout, "\n\tif(a->%s)", ident(p->next->sym->name));
					fprintf(fout, " {\n\t\tint i;\n\t\tfor(i = 0; i < a->%s; i++)\n\t\t{", ident(p->sym->name));
					if(!is_invisible(p->next->sym->name))
						if(has_ptr((Tnode *)p->next->info.typ->ref))
							fprintf(fout, "\n\t\t\tsoap_embedded(soap, a->%s + i, %s);", ident(p->next->sym->name),
								soap_type((Tnode *)p->next->info.typ->ref));
					if(((Tnode *)p->next->info.typ->ref)->type == Tclass && !is_external((Tnode *)p->next->info.typ->ref) &&
					   !is_volatile((Tnode *)p->next->info.typ->ref) && !is_typedef((Tnode *)p->next->info.typ->ref))
						fprintf(fout, "\n\t\t\ta->%s[i].soap_traverse(soap, \"%s\", p, q);", ident(p->next->sym->name), p->next->sym->name);
					else
						fprintf(fout, "\n\t\t\tsoap_traverse_%s(soap, a->%s + i, \"%s\", p, q);",
							c_ident((Tnode *)p->next->info.typ->ref), ident(p->next->sym->name), p->next->sym->name);
					fprintf(fout, "\n\t\t}\n\t}");
					p = p->next;
				}
				else if(is_anytype(p)) {
					p = p->next;
				}
				else if(is_choice(p)) {
					fprintf(fout, "\n\tsoap_traverse_%s(soap, a->%s, &a->%s, \"%s\", p, q);",
						c_ident(p->next->info.typ), ident(p->sym->name), ident(p->next->sym->name), p->next->sym->name);
					p = p->next;
				}
				else if(p->info.typ->type==Tarray) {
					if(has_ptr(p->info.typ))
						fprintf(fout, "\n\tsoap_embedded(soap, a->%s, %s);", ident(p->sym->name), soap_type(p->info.typ));
					fprintf(fout, "\n\tsoap_traverse_%s(soap, a->%s, \"%s\", p, q);",
						c_ident(p->info.typ), ident(p->sym->name), p->sym->name);
				}
				else if(p->info.typ->type == Tclass && !is_external(p->info.typ) && !is_volatile(p->info.typ) && !is_typedef(p->info.typ)) {
					if(has_ptr(p->info.typ))
						fprintf(fout, "\n\tsoap_embedded(soap, &a->%s, %s);", ident(p->sym->name), soap_type(p->info.typ));
					fprintf(fout, "\n\ta->%s.soap_traverse(soap, \"%s\", p, q);", ident(p->sym->name), p->sym->name);
				}
				else if(p->info.typ->type != Tfun && !is_void(p->info.typ)) {
					if(!is_template(p->info.typ))
						if(has_ptr(p->info.typ))
							fprintf(fout, "\n\tsoap_embedded(soap, &a->%s, %s);", ident(p->sym->name), soap_type(p->info.typ));
					fprintf(fout, "\n\tsoap_traverse_%s(soap, &a->%s, \"%s\", p, q);",
						c_ident(p->info.typ), ident(p->sym->name), p->sym->name);
				}
			}
		}
		fprintf(fout, "\n\tif(q) q(soap, (void*)a, %s, s, \"%s\");", soap_type(typ), c_type(typ));
		fprintf(fout, "\n}");
		break;
	    case Tunion:
		if(is_external(typ) && !is_volatile(typ)) {
			fprintf(fhead, "\nSOAP_FMAC1 void SOAP_FMAC2 soap_traverse_%s(struct soap*, int, %s, const char *s, soap_walker p, soap_walker q);",
				c_ident(typ), c_type_id(typ, "*"));
			return;
		}
		table = (Table *)typ->ref;
		fprintf(fhead, "\nSOAP_FMAC3 void SOAP_FMAC4 soap_traverse_%s(struct soap*, int, %s, const char *s, soap_walker p, soap_walker q);",
			c_ident(typ), c_type_id(typ, "*"));
		fprintf(fout, "\n\nSOAP_FMAC3 void SOAP_FMAC4 soap_traverse_%s(struct soap *soap, int choice, %s, const char *s, soap_walker p, soap_walker q)\n{",
			c_ident(typ), c_type_id(typ, "*a"));
		// @v9.8.9 fprintf(fout, "\n\t(void)soap; (void)a; /* appease -Wall -Werror */");
		fprintf(fout, "\n\tif(p) p(soap, (void*)a, %s, s, \"%s\");", soap_type(typ), c_type(typ));
		fprintf(fout, "\n\tswitch(choice) {\n\t");
		for(t = table; t; t = t->prev) {
			for(p = t->list; p; p = p->next)                                 {
				if(p->info.sto&(Sconst|Sprivate|Sprotected))
					fprintf(fout, "\n\t/* non-serializable %s skipped */", ident(p->sym->name));
				else if(is_transient(p->info.typ))
					fprintf(fout, "\n\t/* transient %s skipped */", ident(p->sym->name));
				else if(p->info.sto&Sattribute)
					;
				else if(is_repetition(p))
					;
				else if(is_anytype(p))
					;
				else if(p->info.typ->type==Tarray) {
					fprintf(fout, "\n\tcase SOAP_UNION_%s_%s:", c_ident(typ), ident(p->sym->name));
					if(has_ptr(p->info.typ))
						fprintf(fout, "\n\t\tsoap_embedded(soap, a->%s, %s);", ident(p->sym->name), soap_type(p->info.typ));
					fprintf(fout, "\n\t\tsoap_traverse_%s(soap, a->%s, \"%s\", p, q);", c_ident(p->info.typ), ident(p->sym->name), p->sym->name);
					fprintf(fout, "\n\t\tbreak;");
				}
				else if(p->info.typ->type == Tclass && !is_external(p->info.typ) && !is_volatile(p->info.typ) && !is_typedef(p->info.typ)) {
					fprintf(fout, "\n\tcase SOAP_UNION_%s_%s:", c_ident(typ), ident(p->sym->name));
					if(has_ptr(p->info.typ))
						fprintf(fout, "\n\t\tsoap_embedded(soap, &a->%s, %s);", ident(p->sym->name), soap_type(p->info.typ));
					fprintf(fout, "\n\t\ta->%s.soap_traverse(soap, \"%s\", p, q);",
						ident(p->sym->name), p->sym->name);
					fprintf(fout, "\n\t\tbreak;");
				}
				else if(p->info.typ->type != Tfun && !is_void(p->info.typ)) {
					fprintf(fout, "\n\tcase SOAP_UNION_%s_%s:", c_ident(typ), ident(p->sym->name));
					if(has_ptr(p->info.typ))
						fprintf(fout, "\n\t\tsoap_embedded(soap, &a->%s, %s);", ident(p->sym->name), soap_type(p->info.typ));
					fprintf(fout, "\n\t\tsoap_traverse_%s(soap, &a->%s, \"%s\", p, q);", c_ident(p->info.typ), ident(p->sym->name), p->sym->name);
					fprintf(fout, "\n\t\tbreak;");
				}
			}
		}
		fprintf(fout, "\n\tdefault:\n\t\tbreak;\n\t}");
		fprintf(fout, "\n\tif(q) q(soap, (void*)a, %s, s, \"%s\");", soap_type(typ), c_type(typ));
		fprintf(fout, "\n}");
		break;
	    case Tpointer:
		if(((Tnode *)typ->ref)->type == Tclass && !is_external((Tnode *)typ->ref) && !is_volatile((Tnode *)typ->ref) && !is_typedef((Tnode *)typ->ref)) {
			if(is_external(typ)) {
				fprintf(fhead, "\nSOAP_FMAC1 void SOAP_FMAC2 soap_traverse_%s(struct soap*, %s, const char *s, soap_walker p, soap_walker q);",
					c_ident(typ), c_type_id(typ, "*"));
				return;
			}
			fprintf(fhead, "\nSOAP_FMAC3 void SOAP_FMAC4 soap_traverse_%s(struct soap*, %s, const char *s, soap_walker p, soap_walker q);",
				c_ident(typ), c_type_id(typ, "*"));
			fprintf(fout, "\n\nSOAP_FMAC3 void SOAP_FMAC4 soap_traverse_%s(struct soap *soap, %s, const char *s, soap_walker p, soap_walker q)\n{",
				c_ident(typ), c_type_id(typ, "*a"));
			p = is_dynamic_array((Tnode *)typ->ref);
			if(p) {
				d = get_Darraydims((Tnode *)typ->ref);
				if(d)
					fprintf(fout, "\n\tif(*a)");
				else
					fprintf(fout, "\n\tif(*a)");
			}
			else
				fprintf(fout, "\n\tif(!soap_reference(soap, *a, %s))", soap_type((Tnode *)typ->ref));
			fprintf(fout, "\n\t\t(*a)->soap_traverse(soap, s, p, q);\n}");
			break;
		}
		else {
			if(is_external(typ)) {
				fprintf(fhead, "\nSOAP_FMAC1 void SOAP_FMAC2 soap_traverse_%s(struct soap*, %s, const char *s, soap_walker p, soap_walker q);",
					c_ident(typ), c_type_id(typ, "*"));
				return;
			}
			fprintf(fhead, "\nSOAP_FMAC3 void SOAP_FMAC4 soap_traverse_%s(struct soap*, %s, const char *s, soap_walker p, soap_walker q);",
				c_ident(typ), c_type_id(typ, "*"));
			fprintf(fout, "\n\nSOAP_FMAC3 void SOAP_FMAC4 soap_traverse_%s(struct soap *soap, %s, const char *s, soap_walker p, soap_walker q)\n{",
				c_ident(typ), c_type_id(typ, "*a"));
			if(is_primitive((Tnode *)typ->ref)) {
				fprintf(fout, "\n\tif(!soap_reference(soap, *a, %s)) {\n\t", soap_type(typ));
				fprintf(fout, "\n\t\tif(p) p(soap, (void*)*a, %s, s, \"%s\");", soap_type((Tnode *)typ->ref), c_type((Tnode *)typ->ref));
				fprintf(fout, "\n\t\tif(q) q(soap, (void*)*a, %s, s, \"%s\");\n\t}\n}", soap_type((Tnode *)typ->ref), c_type((Tnode *)typ->ref));
			}
			else if((p = is_dynamic_array((Tnode *)typ->ref)) != NULL) {
				d = get_Darraydims((Tnode *)typ->ref);
				if(d)
					fprintf(fout, "\n\tif(*a)");
				else
					fprintf(fout, "\n\tif(*a)");
				fprintf(fout, "\n\t\tsoap_traverse_%s(soap, *a, s, p, q);\n}", c_ident((Tnode *)typ->ref));
			}
			else {
				fprintf(fout, "\n\tif(!soap_reference(soap, *a, %s))", soap_type((Tnode *)typ->ref));
			      fprintf(fout, "\n\t\tsoap_traverse_%s(soap, *a, s, p, q);\n}", c_ident((Tnode *)typ->ref)); 
			}
			break;
		}
	    case Tarray:
		if(is_external(typ)) {
			fprintf(fhead, "\nSOAP_FMAC1 void SOAP_FMAC2 soap_traverse_%s(struct soap*, %s, const char *s, soap_walker p, soap_walker q);",
				c_ident(typ), c_type(typ));
			return;
		}
		fprintf(fhead, "\nSOAP_FMAC3 void SOAP_FMAC4 soap_traverse_%s(struct soap*, %s, const char *s, soap_walker p, soap_walker q);",
			c_ident(typ), c_type(typ));
		fprintf(fout, "\n\nSOAP_FMAC3 void SOAP_FMAC4 soap_traverse_%s(struct soap *soap, %s, const char *s, soap_walker p, soap_walker q)",
			c_ident(typ), c_type_id(typ, "a"));
		if(is_primitive((Tnode *)typ->ref)) {
			fprintf(fout, "\n{");
			// @v9.8.9 fprintf(fout, "\n\t(void)soap; (void)a; /* appease -Wall -Werror */");
			fprintf(fout, "\n\tif(p) p(soap, (void*)a, %s, s, \"%s\");", soap_type(typ), c_type(typ));
		}
		else {
			fprintf(fout, "\n{\n\tint i;");
		      fprintf(fout, "\n\tif(p) p(soap, (void*)a, %s, s, \"%s\");", soap_type(typ), c_type(typ));
		      fprintf(fout, "\n\tfor(i = 0; i < %d; i++)", get_dimension(typ));
		      temp = (Tnode *)typ->ref;;
		      cardinality = 1;
		      while(temp->type==Tarray) {
			      temp = (Tnode *)temp->ref;
			      cardinality++;
		      }
		      fprintf(fout, " {\n\t");
		      if(has_ptr((Tnode *)typ->ref)) {
			      fprintf(fout, "\tsoap_embedded(soap, a");
			      if(cardinality > 1)
				      fprintf(fout, "[i]");
			      else
				      fprintf(fout, "+i");
			      fprintf(fout, ", %s);", soap_type((Tnode *)typ->ref));
		      }
		      if(((Tnode *)typ->ref)->type == Tclass && !is_external((Tnode *)typ->ref) && !is_volatile((Tnode *)typ->ref) && !is_typedef((Tnode *)typ->ref)) {
			      fprintf(fout, "\n\ta[i].soap_traverse(soap, s, p, q)");
		      }
		      else if(!is_primitive((Tnode *)typ->ref)) {
			      fprintf(fout, "\n\tsoap_traverse_%s(soap, a", c_ident((Tnode *)typ->ref));
			      if(cardinality > 1)
				      fprintf(fout, "[i], s, p, q)");
			      else
				      fprintf(fout, "+i, s, p, q)");
		      }
		      fprintf(fout, ";\n\t}"); }
		fprintf(fout, "\n\tif(q) q(soap, (void*)a, %s, s, \"%s\");", soap_type(typ), c_type(typ));
		fprintf(fout, "\n}");
		break;
	    case Ttemplate:
		if(is_external(typ)) {
			fprintf(fhead, "\nSOAP_FMAC1 void SOAP_FMAC2 soap_traverse_%s(struct soap*, %s, const char *s, soap_walker p, soap_walker q);",
				c_ident(typ), c_type_id(typ, "*"));
			return;
		}
		fprintf(fhead, "\nSOAP_FMAC3 void SOAP_FMAC4 soap_traverse_%s(struct soap*, %s, const char *s, soap_walker p, soap_walker q);",
			c_ident(typ), c_type_id(typ, "*"));
		temp = (Tnode *)typ->ref;
		if(!temp)
			return;
		fprintf(fout, "\n\nSOAP_FMAC3 void SOAP_FMAC4 soap_traverse_%s(struct soap *soap, %s, const char *s, soap_walker p, soap_walker q)\n{",
			c_ident(typ), c_type_id(typ, "*a"));
		if(!is_primitive(temp) && temp->type != Tfun && !is_void(temp)) {
			fprintf(fout, "\n\tif(p) p(soap, (void*)a, %s, s, \"%s\");", soap_type(typ), c_type(typ));
			fprintf(fout, "\n\tfor(%s::iterator i = a->begin(); i != a->end(); ++i)", c_type(typ));
			if(temp->type==Tclass && !is_external(temp) && !is_volatile(temp) && !is_typedef(temp))
				fprintf(fout, "\n\t\t(*i).soap_traverse(soap, s, p, q);");
			else
				fprintf(fout, "\n\t\tsoap_traverse_%s(soap, &(*i), s, p, q);", c_ident(temp));
			fprintf(fout, "\n\tif(q) q(soap, (void*)a, %s, s, \"%s\");", soap_type(typ), c_type(typ));
		}
		fprintf(fout, "\n}");
	    default:     break;
	}
}

void soap_put(Tnode * typ)
{
	int d;
	Entry * p;
	char * ci = c_ident(typ);
	char * ct = c_type(typ);
	char * ctp = c_type_id(typ, "*");
	char * ctpa = c_type_id(typ, "*a");
	char * ctc = c_type_id(typ, "const");
	char * ctca = c_type_id(typ, "const a");
	char * ctcp = c_type_id(typ, "const*");
	char * ctcpa = c_type_id(typ, "const*a");
	if(typ->type == Ttemplate || typ->type == Tunion)
		return;
	if(typ->type == Tclass)
		fprintf(fhead, "\n#ifndef soap_write_%s\n\t#define soap_write_%s(soap, data) ( soap_begin_send(soap) || ((data)->soap_serialize(soap), 0) || (data)->soap_put(soap, \"%s\", NULL) || soap_end_send(soap) )\n#endif\n",
			c_ident(typ), c_ident(typ), xml_tag(typ));
	else if(is_primitive(typ)) {
		if(!is_external(typ) && namespaceid)
			fprintf(fhead, "\n#ifndef soap_write_%s\n\t#define soap_write_%s(soap, data) ( soap_begin_send(soap) || %s::soap_put_%s(soap, data, \"%s\", NULL) || soap_end_send(soap) )\n#endif\n",
				c_ident(typ), c_ident(typ), namespaceid, c_ident(typ), xml_tag(typ));
		else
			fprintf(fhead, "\n#ifndef soap_write_%s\n#define soap_write_%s(soap, data) ( soap_begin_send(soap) || soap_put_%s(soap, data, \"%s\", NULL) || soap_end_send(soap) )\n#endif\n",
				c_ident(typ), c_ident(typ), c_ident(typ), xml_tag(typ));
	}
	else if(typ->type != Treference) {
		if(!is_external(typ) && namespaceid)
			fprintf(fhead, "\n#ifndef soap_write_%s\n\t#define soap_write_%s(soap, data) ( soap_begin_send(soap) || (%s::soap_serialize_%s(soap, data), 0) || %s::soap_put_%s(soap, data, \"%s\", NULL) || soap_end_send(soap) )\n#endif\n",
				c_ident(typ), c_ident(typ), namespaceid, c_ident(typ), namespaceid, c_ident(typ), xml_tag(typ));
		else
			fprintf(fhead, "\n#ifndef soap_write_%s\n\t#define soap_write_%s(soap, data) ( soap_begin_send(soap) || (soap_serialize_%s(soap, data), 0) || soap_put_%s(soap, data, \"%s\", NULL) || soap_end_send(soap) )\n#endif\n",
				c_ident(typ), c_ident(typ), c_ident(typ), c_ident(typ), xml_tag(typ));
	}
	if(is_typedef(typ) && is_element(typ)) {
		fprintf(fhead, "\n\n#define soap_put_%s soap_put_%s\n", c_ident(typ), t_ident(typ));
		return;
	}
	if(typ->type == Tarray) {
		fprintf(fhead, "\n\nSOAP_FMAC3 int /*SOAP_FMAC4*/FASTCALL soap_put_%s(struct soap*, %s, const char*, const char*);", ci, ctc);
		fprintf(fout, "\n\nSOAP_FMAC3 int /*SOAP_FMAC4*/FASTCALL soap_put_%s(struct soap *soap, %s, const char *tag, const char *type)\n{", ci, ctca);
	}
	else if(typ->type == Tclass && !is_external(typ) && !is_volatile(typ) && !is_typedef(typ))
		fprintf(fout, "\n\nint %s::soap_put(struct soap *soap, const char *tag, const  char *type) const\n{", ct);
	else if(typ->type == Tpointer) {
		fprintf(fhead, "\nSOAP_FMAC3 int /*SOAP_FMAC4*/FASTCALL soap_put_%s(struct soap*, %s, const char*, const char*);", ci, ctcp);
		fprintf(fout, "\n\nSOAP_FMAC3 int /*SOAP_FMAC4*/FASTCALL soap_put_%s(struct soap *soap, %s, const char *tag, const char *type)\n{", ci, ctcpa);
	}
	else {
		fprintf(fhead, "\n\nSOAP_FMAC3 int /*SOAP_FMAC4*/FASTCALL soap_put_%s(struct soap*, const %s, const char*, const char*);", ci, ctp);
		fprintf(fout, "\n\nSOAP_FMAC3 int /*SOAP_FMAC4*/FASTCALL soap_put_%s(struct soap *soap, const %s, const char *tag, const char *type)\n{", ci, ctpa); 
	}
	fflush(fout);
	fprintf(fout, "\n\tint local_id = "); // @v10.3.0 id-->local_id
	if(is_invisible(typ->id->name))
		fprintf(fout, "0;");
	else if((p = is_dynamic_array(typ)) != NULL) {
		d = get_Darraydims(typ);
		if(typ->type == Tclass && !is_external(typ) && !is_volatile(typ) && !is_typedef(typ)) {
			if(d) {
				//fprintf(fout, "soap_embed(soap, (void*)this, (struct soap_array*)&this->%s, %d, tag, %s);", ident(p->sym->name), d, soap_type(typ));
				fprintf(fout, "soap_embed(soap, this, (struct soap_array*)&this->%s, %d, tag, %s);", ident(p->sym->name), d, soap_type(typ));
			}
			else {
				//fprintf(fout, "soap_embed(soap, (void*)this, (struct soap_array*)&this->%s, 1, tag, %s);", ident(p->sym->name), soap_type(typ));
				fprintf(fout, "soap_embed(soap, this, (struct soap_array*)&this->%s, 1, tag, %s);", ident(p->sym->name), soap_type(typ));
			}
		}
		else if(d) {
			//fprintf(fout, "soap_embed(soap, (void*)a, (struct soap_array*)&a->%s, %d, tag, %s);", ident(p->sym->name), d, soap_type(typ));
			fprintf(fout, "soap_embed(soap, a, (struct soap_array*)&a->%s, %d, tag, %s);", ident(p->sym->name), d, soap_type(typ));
		}
		else {
			//fprintf(fout, "soap_embed(soap, (void*)a, (struct soap_array*)&a->%s, 1, tag, %s);", ident(p->sym->name), soap_type(typ));
			fprintf(fout, "soap_embed(soap, a, (struct soap_array*)&a->%s, 1, tag, %s);", ident(p->sym->name), soap_type(typ));
		}
	}
	else if(typ->type == Tclass && !is_external(typ) && !is_volatile(typ) && !is_typedef(typ)) {
		//fprintf(fout, "soap_embed(soap, (void*)this, NULL, 0, tag, %s);", soap_type(typ));
		fprintf(fout, "soap_embed(soap, this, NULL, 0, tag, %s);", soap_type(typ));
	}
	else {
		//fprintf(fout, "soap_embed(soap, (void*)a, NULL, 0, tag, %s);", soap_type(typ));
		fprintf(fout, "soap_embed(soap, a, NULL, 0, tag, %s);", soap_type(typ));
	}
	//
	/*
	if(typ->type == Tclass && !is_external(typ) && !is_volatile(typ) && !is_typedef(typ))
		fprintf(fout, "\n\tif(this->soap_out(soap, tag?tag:\"%s\", id, type))\n\t\treturn soap->error;", xml_tag(typ));
	else
		fprintf(fout, "\n\tif(soap_out_%s(soap, tag?tag:\"%s\", id, a, type))\n\t\treturn soap->error;", ci, xml_tag(typ));
	if(!is_invisible(typ->id->name))
		fprintf(fout, "\n\treturn soap_putindependent(soap);\n}");
	else
		fprintf(fout, "\n\treturn SOAP_OK;\n}");
	*/
	//
	fprintf(fout, "\n\treturn ");
	if(typ->type == Tclass && !is_external(typ) && !is_volatile(typ) && !is_typedef(typ))
		fprintf(fout, "this->soap_out(soap, tag?tag:\"%s\", local_id, type) ? soap->error : ", xml_tag(typ)); // @v10.3.0 id-->local_id
	else
		fprintf(fout, "soap_out_%s(soap, tag?tag:\"%s\", local_id, a, type) ? soap->error : ", ci, xml_tag(typ)); // @v10.3.0 id-->local_id
	if(!is_invisible(typ->id->name))
		fprintf(fout, "soap_putindependent(soap);\n}");
	else
		fprintf(fout, "SOAP_OK;\n}");
	//
	fflush(fout);
}

Entry * is_dynamic_array(Tnode * typ)
{
	Entry * p;
	Table * t;
	if((typ->type == Tstruct || typ->type == Tclass) && typ->ref) {
		for(t = (Table *)typ->ref; t; t = t->prev) {
			p = t->list;
			while(p && p->info.typ->type == Tfun)
				p = p->next;
			if(p && p->info.typ->type == Tpointer && !strncmp(ident(p->sym->name), "__ptr", 5))
				if(p->next && (p->next->info.typ->type == Tint || p->next->info.typ->type == Tulong || (p->next->info.typ->type == Tarray && 
					(((Tnode *)p->next->info.typ->ref)->type == Tint || ((Tnode *)p->next->info.typ->ref)->type == Tuint))) &&
				   sstreq(ident(p->next->sym->name), "__size"))
					return p;
		}
	}
	return 0;
}

Entry * is_discriminant(Tnode * typ)
{
	if(oneof2(typ->type, Tstruct, Tclass) && typ->ref) {
		Table * t = (Table *)typ->ref;
		/* only if this struct/class has a union and is not derived */
		if(t && !t->prev) {
			Entry * p = t->list;
			if(p && p->info.typ->type == Tint && ((p->info.sto&Sspecial) || !strncmp(ident(p->sym->name), "__union", 7)))
				if(p->next && p->next->info.typ->type == Tunion) {
					for(Entry * q = p->next->next; q; q = q->next) {
						if(q->info.typ->type != Tfun && !is_void(q->info.typ) && !is_transient(q->info.typ))
							return NULL;
					}
					return p;
				}
		}
	}
	return NULL;
}

int get_Darraydims(Tnode * typ)
{
	if(oneof2(typ->type, Tstruct, Tclass) && typ->ref) {
		for(Table * t = (Table *)typ->ref; t; t = t->prev) {
			Entry * p = t->list;
			while(p && p->info.typ->type == Tfun)
				p = p->next;
			if(p && p->info.typ->type == Tpointer && !strncmp(ident(p->sym->name), "__ptr", 5))
				if(p->next && p->next->info.typ->type == Tarray && (((Tnode *)p->next->info.typ->ref)->type == Tint ||
				    ((Tnode *)p->next->info.typ->ref)->type == Tuint) && sstreq(ident(p->next->sym->name), "__size"))
					return get_dimension(p->next->info.typ);
		}
	}
	return 0;
}

int has_offset(Tnode * typ)
{
	if(oneof2(typ->type, Tstruct, Tclass)) {
		for(Table * t = (Table *)typ->ref; t; t = t->prev) {
			for(Entry * p = t->list; p; p = p->next) {
				if((p->info.typ->type == Tint || (p->info.typ->type == Tarray && ((Tnode *)p->info.typ->ref)->type == Tint)) && sstreq(ident(p->sym->name), "__offset"))
					return 1;
			}
		}
	}
	return 0;
}

int is_boolean(Tnode * typ)
{
	if(typ->type == Tenum) {
		if((Table *)typ->ref == booltable)
			return 1;
		else {
			size_t n = strlen(ident(typ->id->name));
			return n >= 7 && is_eq(ident(typ->id->name)+n-7, "boolean"); 
		}
	}
	return 0;
}

int is_hexBinary(Tnode * typ)
{
	Entry * p;
	Table * t;
	size_t n = strlen(ident(typ->id->name));
	if((typ->type == Tstruct || typ->type == Tclass) && n >= 9 && is_eq(ident(typ->id->name)+n-9, "hexBinary")) {
		for(t = (Table *)typ->ref; t; t = t->prev) {
			p = t->list;
			while(p && p->info.typ->type == Tfun)
				p = p->next;
			if(p && p->info.typ->type == Tpointer && ((Tnode *)p->info.typ->ref)->type == Tuchar && sstreq(ident(p->sym->name), "__ptr")) {
				p = p->next;
				return p && (p->info.typ->type == Tint || p->info.typ->type == Tuint) && sstreq(ident(p->sym->name), "__size");
			}
		}
	}
	return 0;
}

int is_binary(Tnode * typ)
{
	Entry * p;
	Table * t;
	if(!has_ns(typ) && !is_element(typ))
		return 0;
	if(typ->type == Tstruct || typ->type == Tclass) {
		for(t = (Table *)typ->ref; t; t = t->prev) {
			p = t->list;
			while(p && p->info.typ->type == Tfun)
				p = p->next;
			if(p && p->info.typ->type == Tpointer && ((Tnode *)p->info.typ->ref)->type == Tuchar && sstreq(ident(p->sym->name), "__ptr")) {
				p = p->next;
				return p && (p->info.typ->type == Tint || p->info.typ->type == Tuint) && sstreq(ident(p->sym->name), "__size");
			}
		}
	}
	return 0;
}

int is_attachment(Tnode * typ)
{
	Entry * p;
	Table * t;
	if(!is_binary(typ) || is_transient(typ))
		return 0;
	for(t = (Table *)typ->ref; t; t = t->prev) {
		for(p = t->list; p; p = p->next) {
			if(is_string(p->info.typ) && sstreq(p->sym->name, "id")) {
				p = p->next;
				if(!p || !is_string(p->info.typ) || strcmp(p->sym->name, "type"))
					break;
				p = p->next;
				if(!p || !is_string(p->info.typ) || strcmp(p->sym->name, "options"))
					break;
				return 1;
			}
		}
	}
	return 0;
}

int is_mutable(Tnode * typ)
{
	return is_header_or_fault(typ);
}

int is_header_or_fault(Tnode * typ)
{
	if(typ->type == Tpointer || typ->type == Treference)
		return is_header_or_fault((Tnode *)typ->ref);
	return (typ->type == Tstruct || typ->type == Tclass) && (sstreq(ident(typ->id->name), "SOAP_ENV__Header") ||
		sstreq(ident(typ->id->name), "SOAP_ENV__Fault") || sstreq(ident(typ->id->name), "SOAP_ENV__Code") || 
		sstreq(ident(typ->id->name), "SOAP_ENV__Detail") || sstreq(ident(typ->id->name), "SOAP_ENV__Reason"));
}

int is_body(Tnode * typ)
{
	return oneof2(typ->type, Tpointer, Treference) ? is_body((Tnode *)typ->ref) : (oneof2(typ->type, Tstruct, Tclass) && sstreq(ident(typ->id->name), "SOAP_ENV__Body"));
}

long minlen(Tnode * typ)
{
	return (typ->minLength < 0 || (typ->maxLength>>31) != 0) ? 0 : (long)typ->minLength;
}

long maxlen(Tnode * typ)
{
	return (typ->maxLength < 0 || (typ->maxLength>>31) != 0) ? -1 : (long)typ->maxLength;
}

int is_soap12(const char * enc)
{
	return sstreq(envURI, "http://www.w3.org/2003/05/soap-envelope") || (enc && sstreq(enc, "http://www.w3.org/2003/05/soap-encoding"));
}

int is_document(const char * style)
{
	return (!eflag && !style) || (style && sstreq(style, "document"));
}

int is_literal(const char * encoding)
{
	return (!eflag && !encoding) || (encoding && sstreq(encoding, "literal"));
}

char * has_soapref(Tnode * typ)
{
	Entry * p;
	Table * t;
	if(typ->type == Tstruct || typ->type == Tclass) {
		for(t = (Table *)typ->ref; t; t = t->prev)                                                  {
			for(p = t->list; p; p = p->next)
				if(p->info.typ->type == Tpointer && ((Tnode *)p->info.typ->ref)->type == Tstruct &&
				   ((Tnode *)p->info.typ->ref)->id == lookup("soap"))
					return ident(p->sym->name);
		}
	}
	return NULL;
}

int has_constructor(Tnode * typ)
{
	if(typ->type == Tclass) {
		for(Table * t = (Table *)typ->ref; t; t = t->prev) {
			for(Entry * p = t->list; p; p = p->next) {
				if(p->info.typ->type == Tfun && sstreq(p->sym->name, typ->id->name) && ((FNinfo *)p->info.typ->ref)->ret->type == Tnone) {
					Entry * q = ((FNinfo *)p->info.typ->ref)->args->list;
					if(!q)
						return 1;
				}
			}
		}
	}
	return 0;
}

int has_destructor(Tnode * typ)
{
	if(typ->type == Tclass) {
		for(Table * t = (Table *)typ->ref; t; t = t->prev) {
			for(Entry * p = t->list; p; p = p->next)
				if(p->info.typ->type == Tfun && *p->sym->name == '~')
					return 1;
		}
	}
	return 0;
}

int has_getter(Tnode * typ)
{
	Entry * p, * q;
	Table * t;
	if(typ->type == Tclass)
		for(t = (Table *)typ->ref; t; t = t->prev)
			for(p = t->list; p; p = p->next)
				if(p->info.typ->type == Tfun && sstreq(p->sym->name, "get") && ((FNinfo *)p->info.typ->ref)->ret->type == Tint) {
					q = ((FNinfo *)p->info.typ->ref)->args->list;
					if(q && q->info.typ->type == Tpointer && ((Tnode *)q->info.typ->ref)->type ==
					   Tstruct && ((Tnode *)q->info.typ->ref)->id == lookup("soap"))
						return 1;
				}
	return 0;
}

int has_setter(Tnode * typ)
{
	Entry * p, * q;
	Table * t;
	if(typ->type == Tclass)
		for(t = (Table *)typ->ref; t; t = t->prev)
			for(p = t->list; p; p = p->next)
				if(p->info.typ->type == Tfun && sstreq(p->sym->name, "set") && ((FNinfo *)p->info.typ->ref)->ret->type == Tint) {
					q = ((FNinfo *)p->info.typ->ref)->args->list;
					if(q && q->info.typ->type == Tpointer && ((Tnode *)q->info.typ->ref)->type ==
					   Tstruct && ((Tnode *)q->info.typ->ref)->id == lookup("soap"))
						return 1;
				}
	return 0;
}

int is_primitive_or_string(Tnode * typ)
{
	return is_primitive(typ) || is_string(typ) || is_wstring(typ) || is_stdstring(typ) || is_stdwstring(typ) ||
	       is_qname(typ) || is_stdqname(typ);
}

int is_primitive(Tnode * typ)
{
	return typ->type <= Tenum;
}

int is_string(Tnode * typ)
{
	return typ->type == Tpointer && ((Tnode *)typ->ref)->type == Tchar && !((Tnode *)typ->ref)->sym;
}

int is_fixedstring(Tnode * typ)
{
	return bflag && typ->type == Tarray && ((Tnode *)typ->ref)->type == Tchar;
}

int is_wstring(Tnode * typ)
{
	return typ->type == Tpointer && ((Tnode *)typ->ref)->type == Twchar && !((Tnode *)typ->ref)->sym;
}

int is_stdstring(Tnode * typ)
{
	return typ->type == Tclass && typ->id == lookup("std::string");
}

int is_stdwstring(Tnode * typ)
{
	return typ->type == Tclass && typ->id == lookup("std::wstring");
}

int is_stdstr(Tnode * typ)
{
	if(typ->type == Tpointer)
		return is_stdstring((Tnode *)typ->ref) || is_stdwstring((Tnode *)typ->ref);
	return is_stdstring(typ) || is_stdwstring(typ);
}

int is_typedef(Tnode * typ)
{
	return typ->sym && !is_transient(typ);
}

int reflevel(Tnode * typ)
{
	int level;
	for(level = 0; typ->type == Tpointer; level++)
		typ = (Tnode *)typ->ref;
	return level;
}

Tnode * reftype(Tnode * typ)
{
	while((typ->type == Tpointer && !is_string(typ) && !is_wstring(typ)) || typ->type == Treference)
		typ = (Tnode *)typ->ref;
	return typ;
}

void soap_set_attr(Entry * p, char * obj, char * name, char * tag)
{
	Tnode * typ = p->info.typ;
	int flag = (p->info.minOccurs == 0);
	if(is_external(typ) && !is_anyAttribute(typ))
		fprintf(fout, "\n\tsoap_set_attr(soap, \"%s\", soap_%s2s(soap, %s->%s), 1);", tag, c_ident(typ), obj, name);
	else if(is_qname(typ))
		fprintf(fout, "\n\tif(%s->%s)\n\t\tsoap_set_attr(soap, \"%s\", soap_QName2s(soap, %s->%s), 1);", obj, name, tag, obj, name);
	else if(is_string(typ))
		fprintf(fout, "\n\tif(%s->%s)\n\t\tsoap_set_attr(soap, \"%s\", %s->%s, 1);", obj, name, tag, obj, name);
	else if(is_wstring(typ))
		fprintf(fout, "\n\tif(%s->%s)\n\t\tsoap_set_attr(soap, \"%s\", soap_wchar2s(soap, %s->%s), 2);", obj, name, tag, obj, name);
	else if(is_stdqname(typ))
		fprintf(fout, "\n\tif(!%s->%s.empty())\n\t\tsoap_set_attr(soap, \"%s\", soap_QName2s(soap, %s->%s.c_str()), 1);", obj, name, tag, obj, name);
	else if(is_stdstring(typ)) {
		if(flag)
			fprintf(fout, "\n\tif(!%s->%s.empty())", obj, name);
		fprintf(fout, "\n\tsoap_set_attr(soap, \"%s\", %s->%s.c_str(), 1);", tag, obj, name);
	}
	else if(is_stdwstring(typ)) {
		if(flag)
			fprintf(fout, "\n\tif(!%s->%s.empty())", obj, name);
		fprintf(fout, "\n\tsoap_set_attr(soap, \"%s\", soap_wchar2s(soap, %s->%s.c_str()), 2);", tag, obj, name);
	}
	else if(typ->type == Tllong || typ->type == Tullong)
		fprintf(fout, "\n\tsoap_set_attr(soap, \"%s\", soap_%s2s(soap, %s->%s), 1);", tag, c_type(typ), obj, name);
	else if(typ->type == Tenum)
		fprintf(fout, "\n\tsoap_set_attr(soap, \"%s\", soap_%s2s(soap, %s->%s), 1);", tag, c_ident(typ), obj, name);
	else if(typ->type == Tpointer) {
		Tnode * ptr = (Tnode *)typ->ref;
		fprintf(fout, "\n\tif(%s->%s)", obj, name);
		if(is_external(ptr) && !is_anyAttribute(ptr))
			fprintf(fout, "\n\t\tsoap_set_attr(soap, \"%s\", soap_%s2s(soap, *%s->%s), 1);", tag, c_ident(ptr), obj, name);
		else if(is_qname(ptr))
			fprintf(fout, "\n\t\tif(*%s->%s)\n\t\t\tsoap_set_attr(soap, \"%s\", soap_QName2s(soap, *%s->%s), 1);",
				obj, name, tag, obj, name);
		else if(is_string(ptr))
			fprintf(fout, "\n\t\tif(*%s->%s)\n\t\t\tsoap_set_attr(soap, \"%s\", *%s->%s, 1);", obj, name, tag, obj, name);
		else if(ptr->type == Tllong || ptr->type == Tullong)
			fprintf(fout, "\n\t\tsoap_set_attr(soap, \"%s\", soap_%s2s(soap, *%s->%s), 1);", tag, c_type(ptr), obj, name);
		else if(ptr->type == Tenum)
			fprintf(fout, "\n\t\tsoap_set_attr(soap, \"%s\", soap_%s2s(soap, *%s->%s), 1);", tag, c_ident(ptr), obj, name);
		else if(is_stdqname(ptr))
			fprintf(fout, "\n\t\tsoap_set_attr(soap, \"%s\", soap_QName2s(soap, %s->%s->c_str()), 1);", tag, obj, name);
		else if(is_stdstring(ptr))
			fprintf(fout, "\n\t\tsoap_set_attr(soap, \"%s\", %s->%s->c_str(), 1);", tag, obj, name);
		else if(is_stdwstring(ptr))
			fprintf(fout, "\n\t\tsoap_set_attr(soap, \"%s\", soap_wchar2s(soap, %s->%s->c_str()), 2);", tag, obj, name);
		else if(is_primitive(ptr))
			fprintf(fout, "\n\t\tsoap_set_attr(soap, \"%s\", soap_%s2s(soap, *%s->%s), 1);", tag, the_type(ptr), obj, name);
		else if(is_hexBinary(ptr))
			fprintf(fout, "\n\t\tif(%s->%s->__ptr)\n\t\t\tsoap_set_attr(soap, \"%s\", soap_s2hex(soap, %s->%s->__ptr, NULL, %s->%s->__size), 1);",
				obj, name, tag, obj, name, obj, name);
		else if(is_binary(ptr))
			fprintf(fout, "\n\t\tif(%s->%s->__ptr)\n\t\t\tsoap_set_attr(soap, \"%s\", soap_s2base64(soap, %s->%s->__ptr, NULL, %s->%s->__size), 1);",
				obj, name, tag, obj, name, obj, name);
		else if(is_anyAttribute(ptr))
			fprintf(fout, "\n\t\tif(soap_out_%s(soap, \"%s\", -1, %s->%s, \"%s\"))\n\t\t\treturn soap->error;",
				c_ident(ptr), tag, obj, name, xsi_type_u(ptr));
		else {
			sprintf(errbuf, "Field '%s' cannot be serialized as an XML attribute", name);
			semwarn(errbuf); 
		}
	}
	else if(is_primitive(typ))
		fprintf(fout, "\n\tsoap_set_attr(soap, \"%s\", soap_%s2s(soap, %s->%s), 1);", tag, the_type(typ), obj, name);
	else if(is_hexBinary(typ))
		fprintf(fout, "\n\tif(%s->%s.__ptr)\n\t\tsoap_set_attr(soap, \"%s\", soap_s2hex(soap, %s->%s.__ptr, NULL, %s->%s.__size), 1);", obj, name, tag, obj, name, obj, name);
	else if(is_binary(typ))
		fprintf(fout, "\n\tif(%s->%s.__ptr)\n\t\tsoap_set_attr(soap, \"%s\", soap_s2base64(soap, %s->%s.__ptr, NULL, %s->%s.__size), 1);", obj, name, tag, obj, name, obj, name);
	else if(is_anyAttribute(typ))
		fprintf(fout, "\n\tif(soap_out_%s(soap, \"%s\", -1, &%s->%s, \"%s\"))\n\t\treturn soap->error;", c_ident(typ), tag, obj, name, xsi_type_u(typ));
	else {
		sprintf(errbuf, "Field '%s' cannot be serialized as an XML attribute", name);
		semwarn(errbuf); 
	}
}

void soap_attr_value(Entry * p, char * obj, char * name, char * tag)
{
	int flag = 0;
	Tnode * typ = p->info.typ;
	if(p->info.maxOccurs == 0)
		flag = 2;  /* prohibited */
	else if(p->info.minOccurs >= 1 && !p->info.hasval)
		flag = 1;  /* required */
	if(is_external(typ) && !is_anyAttribute(typ))
		fprintf(fout, "\n\tif(soap_s2%s(soap, soap_attr_value(soap, \"%s\", %d), &%s->%s))\n\t\treturn NULL;",
			c_ident(typ), tag, flag, obj, name);
	else if(typ->type == Tllong || typ->type == Tullong)
		fprintf(fout, "\n\tif(soap_s2%s(soap, soap_attr_value(soap, \"%s\", %d), &%s->%s))\n\t\treturn NULL;",
			c_type(typ), tag, flag, obj, name);
	else if(typ->type == Tenum)
		fprintf(fout, "\n\tif(soap_s2%s(soap, soap_attr_value(soap, \"%s\", %d), &%s->%s))\n\t\treturn NULL;",
			c_ident(typ), tag, flag, obj, name);
	else if(is_qname(typ))
		fprintf(fout,
			"\n\tif(soap_s2QName(soap, soap_attr_value(soap, \"%s\", %d), &%s->%s, %ld, %ld))\n\t\treturn NULL;",
			tag, flag, obj, name, minlen(typ), maxlen(typ));
	else if(is_string(typ))
		fprintf(fout,
			"\n\tif(soap_s2string(soap, soap_attr_value(soap, \"%s\", %d), &%s->%s, %ld, %ld))\n\t\treturn NULL;",
			tag, flag, obj, name, minlen(typ), maxlen(typ));
	else if(is_wstring(typ))
		fprintf(fout,
			"\n\tif(soap_s2wchar(soap, soap_attr_value(soap, \"%s\", %d), &%s->%s, %ld, %ld))\n\t\treturn NULL;",
			tag, flag, obj, name, minlen(typ), maxlen(typ));
	else if(is_stdqname(typ))
		fprintf(fout,
			"\n\t{\tconst char *t = soap_attr_value(soap, \"%s\", %d);\n\t\tif(t)\n\t\t{\tchar *s;\n\t\t\tif(soap_s2QName(soap, t, &s, %ld, %ld))\n\t\t\t\treturn NULL;\n\t\t\t%s->%s.assign(s);\n\t\t}\n\t\telse if(soap->error)\n\t\t\treturn NULL;\n\t}",
			tag, flag, minlen(typ), maxlen(typ), obj, name);
	else if(is_stdstring(typ))
		fprintf(fout, "\n\t{\tconst char *t = soap_attr_value(soap, \"%s\", %d);\n\t\tif(t)\n\t\t{\tchar *s;\n\t\t\tif(soap_s2string(soap, t, &s, %ld, %ld))\n\t\t\t\treturn NULL;\n\t\t\t%s->%s.assign(s);\n\t\t}\n\t\telse if(soap->error)\n\t\t\treturn NULL;\n\t}",
			tag, flag, minlen(typ), maxlen(typ), obj, name);
	else if(is_stdwstring(typ))
		fprintf(fout, "\n\t{\tconst char *t = soap_attr_value(soap, \"%s\", %d);\n\t\tif(t)\n\t\t{\twchar_t *s;\n\t\t\tif(soap_s2wchar(soap, t, &s, %ld, %ld))\n\t\t\t\treturn NULL;\n\t\t\t%s->%s.assign(s);\n\t\t}\n\t\telse if(soap->error)\n\t\t\treturn NULL;\n\t}",
			tag, flag, minlen(typ), maxlen(typ), obj, name);
	else if(typ->type == Tpointer) {
		Tnode * ptr = (Tnode *)typ->ref;
		if(!is_anyAttribute(ptr))
			fprintf(fout, "\n\t{\tconst char *t = soap_attr_value(soap, \"%s\", %d);\n\t\tif(t)\n\t\t{", tag, flag);
		if(!is_stdstring(ptr))
			fprintf(fout, "\n\t\t\tif(!(%s->%s = static_cast<%s>(soap_malloc(soap, sizeof(%s)))))\n\t\t\t{\tsoap->error = SOAP_EOM;\n\t\t\t\treturn NULL;\n\t\t\t}",
				obj, name, c_type(typ), c_type(ptr));
		if(is_external(ptr) && !is_anyAttribute(ptr))
			fprintf(fout, "\n\t\t\tif(soap_s2%s(soap, t, %s->%s))\n\t\t\t\treturn NULL;", c_ident(ptr), obj, name);
		else if(ptr->type == Tllong || ptr->type == Tullong)
			fprintf(fout, "\n\t\t\tif(soap_s2%s(soap, t, %s->%s))\n\t\t\treturn NULL;", c_type(ptr), obj, name);
		else if(ptr->type == Tenum)
			fprintf(fout, "\n\t\t\tif(soap_s2%s(soap, t, %s->%s))\n\t\t\treturn NULL;", c_ident(ptr), obj, name);
		else if(is_qname(ptr))
			fprintf(fout, "\n\t\t\tif(soap_s2QName(soap, t, %s->%s, %ld, %ld))\n\t\t\t\treturn NULL;", obj, name, minlen(ptr), maxlen(ptr));
		else if(is_string(ptr))
			fprintf(fout, "\n\t\t\tif(soap_s2string(soap, t, %s->%s, %ld, %ld))\n\t\t\t\treturn NULL;", obj, name, minlen(ptr), maxlen(ptr));
		else if(is_stdqname(ptr))
			fprintf(fout, "\n\t\t\tchar *s = NULL;\n\t\t\tif(soap_s2QName(soap, t, &s, %ld, %ld))\n\t\t\t\treturn NULL;\n\t\t\tif(s)\n\t\t\t{\t%s->%s = soap_new_std__string(soap, -1);\n\t\t\t\t%s->%s->assign(s);\n\t\t\t}",
				minlen(ptr), maxlen(ptr), obj, name, obj, name);
		else if(is_stdstring(ptr))
			fprintf(fout, "\n\t\t\tchar *s = NULL;\n\t\t\tif(soap_s2string(soap, t, &s, %ld, %ld))\n\t\t\t\treturn NULL;\n\t\t\tif(s)\n\t\t\t{\t%s->%s = soap_new_std__string(soap, -1);\n\t\t\t\t%s->%s->assign(s);\n\t\t\t}",
				minlen(ptr), maxlen(ptr), obj, name, obj, name);
		else if(is_stdwstring(ptr))
			fprintf(fout, "\n\t\t\twchar_t *s = NULL;\n\t\t\tif(soap_s2wchar(soap, t, &s, %ld, %ld))\n\t\t\t\treturn NULL;\n\t\t\tif(s)\n\t\t\t{\t%s->%s = soap_new_std__wstring(soap, -1);\n\t\t\t\t%s->%s->assign(s);\n\t\t\t}",
				minlen(ptr), maxlen(ptr), obj, name, obj, name);
		else if(is_hexBinary(ptr))
			fprintf(fout, "\n\t\t\tif(!(%s->%s->__ptr = (uchar *)soap_hex2s(soap, soap_attr_value(soap, \"%s\", %d), NULL, 0, &%s->%s->__size)))\n\t\t\t\treturn NULL;",
				obj, name, tag, flag, obj, name);
		else if(is_binary(ptr))
			fprintf(fout, "\n\t\t\tif(!(%s->%s->__ptr = (uchar *)soap_base642s(soap, soap_attr_value(soap, \"%s\", %d), NULL, 0, &%s->%s->__size)))\n\t\t\t\treturn NULL;",
				obj, name, tag, flag, obj, name);
		else if(is_anyAttribute(ptr))
			fprintf(fout, "\n\t\t\t%s->%s = soap_in_%s(soap, \"%s\", %s->%s, \"%s\");", obj, name,
				c_ident(ptr), tag, obj, name, xsi_type(ptr));
		else
			fprintf(fout, "\n\t\t\tif(soap_s2%s(soap, t, %s->%s))\n\t\t\t\treturn NULL;", the_type(ptr), obj, name);
		if(!is_anyAttribute(ptr))
			fprintf(fout, "\n\t\t}\n\t\telse if(soap->error)\n\t\t\treturn NULL;\n\t}");
	}
	else if(is_hexBinary(typ))
		fprintf(fout,
			"\n\tif(!(%s->%s.__ptr = (uchar *)soap_hex2s(soap, soap_attr_value(soap, \"%s\", %d), NULL, 0, &%s->%s.__size)))\n\t\treturn NULL;",
			obj, name, tag, flag, obj, name);
	else if(is_binary(typ))
		fprintf(fout,
			"\n\tif(!(%s->%s.__ptr = (uchar *)soap_base642s(soap, soap_attr_value(soap, \"%s\", %d), NULL, 0, &%s->%s.__size)))\n\t\treturn NULL;",
			obj, name, tag, flag, obj, name);
	else if(is_anyAttribute(typ))
		fprintf(fout, "\n\tsoap_in_%s(soap, \"%s\", &%s->%s, \"%s\");", c_ident(typ), tag, obj, name, xsi_type(typ));
	else if(is_primitive(typ))
		fprintf(fout, "\n\tif(soap_s2%s(soap, soap_attr_value(soap, \"%s\", %d), &%s->%s))\n\t\treturn NULL;",
			the_type(typ), tag, flag, obj, name);
	if(typ->type == Tpointer) {
		if(!is_string(typ) && !is_wstring(typ) && !is_stdstr((Tnode *)typ->ref))                            {
			Tnode * ptr = (Tnode *)typ->ref;
			if(!is_string(ptr) && !is_wstring(ptr)) {
				if(ptr->minLength != MINLONG64 && (ptr->minLength > 0 || ptr->type < Tuchar || ptr->type > Tullong))
					fprintf(fout, "\n\tif(%s->%s && *%s->%s < " SOAP_LONG_FORMAT ") {\n\t\tsoap->error = SOAP_LENGTH;\n\t\treturn NULL;\n\t}",
						obj, name, obj, name, ptr->minLength);
				if(ptr->maxLength != MAXLONG64)
					fprintf(fout,
						"\n\tif(%s->%s && *%s->%s > " SOAP_LONG_FORMAT ") {\n\t\tsoap->error = SOAP_LENGTH;\n\t\treturn NULL;\n\t}",
						obj, name, obj, name, ptr->maxLength);
			}
		}
	}
	else if(!is_stdstr(typ)) {
		if(typ->minLength != MINLONG64 && (typ->minLength > 0 || typ->type < Tuchar || typ->type > Tullong))
			fprintf(fout, "\n\tif(%s->%s < " SOAP_LONG_FORMAT ") {\n\t\tsoap->error = SOAP_LENGTH;\n\t\treturn NULL;\n\t}",
				obj, name, typ->minLength);
		if(typ->maxLength != MAXLONG64)
			fprintf(fout, "\n\tif(%s->%s > " SOAP_LONG_FORMAT ") {\n\t\tsoap->error = SOAP_LENGTH;\n\t\treturn NULL;\n\t}",
				obj, name, typ->maxLength);
	}
}

char * ptr_cast(Table * t, char * name)
{
	char * s = emalloc(strlen(t->sym->name)+strlen(name)+6);
	sprintf(s, "((%s*)%s)", t->sym->name, name);
	return s;
}

void soap_out(Tnode * typ)
{
	Table * table, * t;
	Entry * p = NULL;
	int cardinality, i, j, d;
	Tnode * n;
	char * nse = ns_qualifiedElement(typ);
	char * nsa = ns_qualifiedAttribute(typ);
	if(is_dynamic_array(typ)) {
		soap_out_Darray(typ);
		return;
	}
	if(is_external(typ))
		fprintf(fhead, "\n\nSOAP_FMAC3S const char* SOAP_FMAC4S soap_%s2s(struct soap*, %s);", c_ident(typ), c_type(typ));
	if(is_typedef(typ) && is_element(typ)) {
		fprintf(fhead, "\n\n#define soap_out_%s soap_out_%s\n", c_ident(typ), t_ident(typ));
		return;
	}
	if(is_primitive(typ) && typ->type != Tenum) {
		if(is_external(typ))                                              {
			fprintf(fhead, "\nSOAP_FMAC1 int SOAP_FMAC2 soap_out_%s(struct soap*, const char*, int, const %s, const char*);",
				c_ident(typ), c_type_id(typ, "*"));
			return;
		}
		fprintf(fhead, "\nSOAP_FMAC3 int FASTCALL soap_out_%s(struct soap*, const char*, int, const %s, const char*);",
			c_ident(typ), c_type_id(typ, "*"));
		fprintf(fout, "\n\nSOAP_FMAC3 int FASTCALL soap_out_%s(struct soap *soap, const char *tag, int id, const %s, const char *type)\n{\n\t(void)soap; (void)type; (void)tag; (void)id;",
			c_ident(typ), c_type_id(typ, "*a"));
		if(typ->type == Tllong || typ->type == Tullong)
			fprintf(fout, "\n\treturn soap_out%s(soap, tag, id, a, type, %s);\n}", c_type(typ), soap_type(typ));
		else
			fprintf(fout, "\n\treturn soap_out%s(soap, tag, id, a, type, %s);\n}", the_type(typ), soap_type(typ));
		return;
	}
	if(is_fixedstring(typ)) {
		fprintf(fhead, "\nSOAP_FMAC3 int FASTCALL soap_out_%s(struct soap*, const char*, int, const char[], const char*);", c_ident(typ));
		fprintf(fout, "\n\nSOAP_FMAC3 int FASTCALL soap_out_%s(struct soap *soap, const char *tag, int id, const char a[], const char *type)\n{", c_ident(typ));
		fprintf(fout, "\n\treturn soap_outstring(soap, tag, id, (char*const*)&a, type, %s);\n}", soap_type(typ));
		return;
	}
	if(is_string(typ)) {
		if(is_external(typ)) {
			fprintf(fhead, "\nSOAP_FMAC1 int SOAP_FMAC2 soap_out_%s(struct soap*, const char*, int, char*const*, const char*);", c_ident(typ));
			return;
		}
		fprintf(fhead, "\nSOAP_FMAC3 int FASTCALL soap_out_%s(struct soap*, const char*, int, char*const*, const char*);", c_ident(typ));
		fprintf(fout, "\n\nSOAP_FMAC3 int FASTCALL soap_out_%s(struct soap *soap, const char *tag, int id, char *const*a, const char *type)\n{",
			c_ident(typ));
		fprintf(fout, "\n\treturn soap_outstring(soap, tag, id, a, type, %s);\n}", soap_type(typ));
		return;
	}
	if(is_wstring(typ)) {
		if(is_external(typ)) {
			fprintf(fhead, "\nSOAP_FMAC1 int SOAP_FMAC2 soap_out_%s(struct soap*, const char*, int, wchar_t*const*, const char*);",
				c_ident(typ));
			return;
		}
		fprintf(fhead, "\nSOAP_FMAC3 int FASTCALL soap_out_%s(struct soap*, const char*, int, wchar_t*const*, const char*);",
			c_ident(typ));
		fprintf(fout, "\n\nSOAP_FMAC3 int FASTCALL soap_out_%s(struct soap *soap, const char *tag, int id, wchar_t *const*a, const char *type)\n{",
			c_ident(typ));
		fprintf(fout, "\n\treturn soap_outwstring(soap, tag, id, a, type, %s);\n}", soap_type(typ));
		return;
	}
	if(is_stdstring(typ)) {
		if(is_external(typ)) {
			fprintf(fhead, "\nSOAP_FMAC1 int SOAP_FMAC2 soap_out_%s(struct soap*, const char*, int, const std::string*, const char*);",
				c_ident(typ));
			return;
		}
		fprintf(fhead, "\nSOAP_FMAC3 int FASTCALL soap_out_%s(struct soap*, const char*, int, const std::string*, const char*);",
			c_ident(typ));
		if(is_stdXML(typ))
			fprintf(fout, "\nSOAP_FMAC3 int FASTCALL soap_out_%s(struct soap *soap, const char *tag, int id, const std::string *s, const char *type)\n{\n\tconst char *t = s->c_str();\n\treturn soap_outliteral(soap, tag, (char*const*)&t, type);\n}",
				c_ident(typ));
		else
			fprintf(fout, "\nSOAP_FMAC3 int FASTCALL soap_out_%s(struct soap *soap, const char *tag, int id, const std::string *s, const char *type)\n{\n\tif((soap->mode & SOAP_C_NILSTRING) && s->empty())\n\t\treturn soap_element_null(soap, tag, id, type);\n\tif(soap_element_begin_out(soap, tag, soap_embedded_id(soap, id, s, %s), type) || soap_string_out(soap, s->c_str(), 0) || soap_element_end_out(soap, tag))\n\t\treturn soap->error;\n\treturn SOAP_OK;\n}",
				c_ident(typ), soap_type(typ));
		return;
	}
	if(is_stdwstring(typ)) {
		if(is_external(typ)) {
			fprintf(fhead, "\nSOAP_FMAC1 int SOAP_FMAC2 soap_out_%s(struct soap*, const char*, int, const std::wstring*, const char*);",
				c_ident(typ));
			return;
		}
		fprintf(fhead, "\nSOAP_FMAC3 int FASTCALL soap_out_%s(struct soap*, const char*, int, const std::wstring*, const char*);",
			c_ident(typ));
		if(is_stdXML(typ))
			fprintf(fout, "\nSOAP_FMAC3 int FASTCALL soap_out_%s(struct soap *soap, const char *tag, int id, const std::wstring *s, const char *type)\n{\n\tconst wchar_t *t = s->c_str();\n\treturn soap_outwliteral(soap, tag, (wchar_t*const*)&t, type);\n}",
				c_ident(typ));
		else
			fprintf(fout, "\nSOAP_FMAC3 int FASTCALL soap_out_%s(struct soap *soap, const char *tag, int id, const std::wstring *s, const char *type)\n{\n\tif((soap->mode & SOAP_C_NILSTRING) && s->empty())\n\t\treturn soap_element_null(soap, tag, id, type);\n\tif(soap_element_begin_out(soap, tag, soap_embedded_id(soap, id, s, %s), type) || soap_wstring_out(soap, s->c_str(), 0) || soap_element_end_out(soap, tag))\n\t\treturn soap->error;\n\treturn SOAP_OK;\n}",
				c_ident(typ), soap_type(typ));
		return;
	}
	switch(typ->type) {
	    case Tstruct:
		table = (Table *)typ->ref;
		if(is_external(typ)) {
			fprintf(fhead, "\nSOAP_FMAC1 int SOAP_FMAC2 soap_out_%s(struct soap*, const char*, int, const %s, const char*);",
				c_ident(typ), c_type_id(typ, "*"));
			return;
		}
		fprintf(fhead, "\nSOAP_FMAC3 int FASTCALL soap_out_%s(struct soap*, const char*, int, const %s, const char*);",
			c_ident(typ), c_type_id(typ, "*"));
		fprintf(fout, "\n\nSOAP_FMAC3 int FASTCALL soap_out_%s(struct soap *soap, const char *tag, int id, const %s, const char *type)\n{",
			c_ident(typ), c_type_id(typ, "*a"));
		for(t = table; t; t = t->prev) {
			for(p = t->list; p; p = p->next) {
				if(is_repetition(p))
					p = p->next;
				else if(p->info.sto&Sattribute)
					soap_set_attr(p, "a", ident(p->sym->name), ns_add(p->sym->name, nsa));
				else if(is_qname(p->info.typ))
					fprintf(fout, "\n\tconst char *soap_tmp_%s = soap_QName2s(soap, a->%s);", ident(p->sym->name), ident(p->sym->name));
				else if(is_stdqname(p->info.typ))
					fprintf(fout, "\n\tstd::string soap_tmp_%s(soap_QName2s(soap, a->%s.c_str()));",
						ident(p->sym->name), ident(p->sym->name));
				else if(p->info.typ->type == Tpointer && is_qname((Tnode *)p->info.typ->ref))
					fprintf(fout, "\n\tconst char *soap_tmp_%s = a->%s ? soap_QName2s(soap, *a->%s) : NULL;",
						ident(p->sym->name), ident(p->sym->name), ident(p->sym->name));
				else if(p->info.typ->type == Tpointer && is_stdqname((Tnode *)p->info.typ->ref))
					fprintf(fout, "\n\tstd::string soap_temp_%s(a->%s ? soap_QName2s(soap, a->%s->c_str()) : \"\"), *soap_tmp_%s = a->%s ? &soap_temp_%s : NULL;",
						ident(p->sym->name), ident(p->sym->name), ident(p->sym->name), ident(p->sym->name), ident(p->sym->name), ident(p->sym->name));
			}
		}
		fprintf(fout, "\n\t(void)soap; (void)tag; (void)id; (void)type;");
		if(is_primclass(typ)) {
			for(table = (Table *)typ->ref; table; table = table->prev) {
				p = table->list;
				if(p && is_item(p))
					break;
			}
			if((p->info.sto&SmustUnderstand) && !(p->info.sto&(Sconst|Sprivate|Sprotected)) &&
			   !(p->info.sto&Sattribute) && !is_transient(p->info.typ) && !is_void(p->info.typ) &&
			   p->info.typ->type != Tfun)
				fprintf(fout, "\n\tsoap->mustUnderstand = 1;");
			if(p->info.typ->type==Tarray)
				fprintf(fout, "\n\treturn soap_out_%s(soap, tag, id, a->%s, \"%s\");",
					c_ident(p->info.typ), ident(p->sym->name), xsi_type_u(typ));
			else if(p->info.typ->type==Tclass && !is_external(p->info.typ) && !is_volatile(p->info.typ) &&
			        !is_typedef(p->info.typ))
				fprintf(fout, "\n\treturn a->%s.soap_out(soap, tag, id, \"%s\");", ident(
						p->sym->name), xsi_type_u(typ));
			else if(is_qname(p->info.typ))
				fprintf(fout,
					"\n\treturn soap_out_%s(soap, tag, id, (char*const*)&soap_tmp_%s, \"%s\");",
					c_ident(
						p->info.typ), ident(p->sym->name), xsi_type_u(typ));
			else if(is_stdqname(p->info.typ))
				fprintf(fout, "\n\treturn soap_out_%s(soap, tag, id, &soap_tmp_%s, \"%s\");",
					c_ident(p->info.typ), ident(p->sym->name), xsi_type_u(typ));
			else if(p->info.typ->type == Tpointer && is_qname((Tnode *)p->info.typ->ref))
				fprintf(fout,
					"\n\treturn soap_out_%s(soap, tag, id, (char*const*)soap_tmp_%s, \"%s\");",
					c_ident(
						(Tnode *)p->info.typ->ref), ident(p->sym->name), xsi_type_u(typ));
			else if(p->info.typ->type == Tpointer && is_stdqname((Tnode *)p->info.typ->ref))
				fprintf(fout, "\n\treturn soap_out_%s(soap, tag, id, &soap_tmp_%s, \"%s\");",
					c_ident(p->info.typ), ident(p->sym->name), xsi_type_u(typ));
			else if(is_XML(p->info.typ) && is_string(p->info.typ))
				fprintf(fout, "\n\treturn soap_outliteral(soap, tag, &a->%s, NULL);",
					ident(p->sym->name));
			else if(is_XML(p->info.typ) && is_wstring(p->info.typ))
				fprintf(fout, "\n\treturn soap_outwliteral(soap, tag, &a->%s, NULL);",
					ident(p->sym->name));
			else if(p->info.typ->type != Tfun && !is_void(p->info.typ))
				fprintf(fout, "\n\treturn soap_out_%s(soap, tag, id, &a->%s, \"%s\");",
					c_ident(p->info.typ), ident(p->sym->name), xsi_type_u(typ));
			else
				fprintf(fout, "\n\treturn SOAP_OK;");
			fprintf(fout, "\n}");
		}
		else {
			if(!is_invisible(typ->id->name))
			      fprintf(fout, "\n\tif(soap_element_begin_out(soap, tag, soap_embedded_id(soap, id, a, %s), type))\n\t\treturn soap->error;",
				      soap_type(typ));
		      fflush(fout);
		      for(t = table; t; t = t->prev) {
			      for(p = t->list; p; p = p->next) {
				      if(p->info.sto&Sreturn) {
					      if(nse || has_ns_eq(NULL, p->sym->name)) {
						      if(p->info.typ->type == Tpointer)
							      fprintf(fout, "\n\tif(a->%s)\n\t\tsoap_element_result(soap, \"%s\");", ident(p->sym->name), ns_add(p->sym->name, nse));
						      else
							      fprintf(fout, "\n\tsoap_element_result(soap, \"%s\");", ns_add(p->sym->name, nse));
					      }
				      }
				      if((p->info.sto&SmustUnderstand) && !(p->info.sto&(Sconst|Sprivate|Sprotected)) && !is_transient(p->info.typ) &&
				         !is_void(p->info.typ) && p->info.typ->type != Tfun)
					      fprintf(fout, "\n\tsoap->mustUnderstand = 1;");
				      needs_lang(p);
				      if(p->info.sto&(Sconst|Sprivate|Sprotected))
					      fprintf(fout, "\n\t/* non-serializable %s skipped */", ident(p->sym->name));
				      else if(is_transient(p->info.typ))
					      fprintf(fout, "\n\t/* transient %s skipped */", ident(p->sym->name));
				      else if(p->info.sto&Sattribute)
					      ;
				      else if(is_repetition(p)) {
					      fprintf(fout, "\n\tif(a->%s)", ident(p->next->sym->name));
					      fprintf(fout, " {\n\t\tint i;\n\t\tfor(i = 0; i < a->%s; i++)", ident(p->sym->name));
					      if(((Tnode *)p->next->info.typ->ref)->type == Tclass && !is_external((Tnode *)p->next->info.typ->ref) && !is_volatile((Tnode *)p->next->info.typ->ref) && !is_typedef((Tnode *)p->next->info.typ->ref))
						      fprintf(fout, "\n\t\t\tif(a->%s[i].soap_out(soap, \"%s\", -1, \"%s\"))\n\t\t\t\treturn soap->error;",
							      ident(p->next->sym->name), ns_add(p->next->sym->name, nse),
							      xsi_type_cond_u((Tnode *)p->next->info.typ->ref, !has_ns_eq(NULL, p->next->sym->name)));
					      else if(is_qname((Tnode *)p->next->info.typ->ref))
						      fprintf(fout,
							      "\n\t\t{\tconst char *soap_tmp_%s = soap_QName2s(soap, a->%s[i]);\n\t\t\tif(soap_out_%s(soap, \"%s\", -1, (char*const*)&soap_tmp_%s, \"%s\"))\n\t\t\t\treturn soap->error;\n\t\t}",
							      ident(p->next->sym->name), ident(p->next->sym->name), c_ident((Tnode *)p->next->info.typ->ref),
							      ns_add(p->next->sym->name, nse), ident(p->next->sym->name),
							      xsi_type_cond_u((Tnode *)p->next->info.typ->ref, !has_ns_eq(NULL, p->next->sym->name)));
					      else if(is_XML((Tnode *)p->next->info.typ->ref) && is_string((Tnode *)p->next->info.typ->ref))
						      fprintf(fout, "\n\t\t\tsoap_outliteral(soap, \"%s\", a->%s + i, NULL);",
							      ns_add(p->next->sym->name, nse), ident(p->next->sym->name));
					      else if(is_XML((Tnode *)p->next->info.typ->ref) && is_wstring((Tnode *)p->next->info.typ->ref))
						      fprintf(fout, "\n\t\t\tsoap_outwliteral(soap, \"%s\", a->%s + i, NULL);",
							      ns_add(p->next->sym->name, nse), ident(p->next->sym->name));
					      else
						      fprintf(fout, "\n\t\t\tif(soap_out_%s(soap, \"%s\", -1, a->%s + i, \"%s\"))\n\t\t\t\treturn soap->error;",
							      c_ident((Tnode *)p->next->info.typ->ref),
							      ns_add(p->next->sym->name, nse), ident(p->next->sym->name),
							      xsi_type_cond_u((Tnode *)p->next->info.typ->ref, !has_ns_eq(NULL, p->next->sym->name)));
					      fprintf(fout, "\n\t}");
					      p = p->next;
				      }
				      else if(is_anytype(p)) {
					      fprintf(fout, "\n\tif(soap_putelement(soap, a->%s, \"%s\", -1, a->%s))\n\t\treturn soap->error;",
						      ident(p->next->sym->name), ns_add(p->next->sym->name, nse), ident(p->sym->name));
					      p = p->next;
				      }
				      else if(is_choice(p)) {
					      fprintf(fout, "\n\tif(soap_out_%s(soap, a->%s, &a->%s))\n\t\treturn soap->error;",
						      c_ident(p->next->info.typ), ident(p->sym->name), ident(p->next->sym->name));
					      p = p->next;
				      }
				      else if(p->info.typ->type==Tarray)
					      fprintf(fout, "\n\tsoap_out_%s(soap, \"%s\", -1, a->%s, \"%s\");",
						      c_ident(p->info.typ), ns_add(p->sym->name, nse), ident(p->sym->name),
						      xsi_type_cond_u(p->info.typ, !has_ns_eq(NULL, p->sym->name)));
				      else if(p->info.typ->type==Tclass && !is_external(p->info.typ) && !is_volatile(p->info.typ) && !is_typedef(p->info.typ))
					      fprintf(fout, "\n\tif(a->%s.soap_out(soap, \"%s\", -1, \"%s\"))\n\t\treturn soap->error;",
						      ident(p->sym->name), ns_add(p->sym->name, nse),
						      xsi_type_cond_u(p->info.typ, !has_ns_eq(NULL, p->sym->name)));
				      else if(is_qname(p->info.typ))
					      fprintf(fout, "\n\tif(soap_out_%s(soap, \"%s\", -1, (char*const*)&soap_tmp_%s, \"%s\"))\n\t\treturn soap->error;",
						      c_ident(p->info.typ), ns_add(p->sym->name, nse), ident(
							      p->sym->name),
						      xsi_type_cond_u(p->info.typ, !has_ns_eq(NULL, p->sym->name)));
				      else if(is_stdqname(p->info.typ))
					      fprintf(fout, "\n\tif(soap_out_%s(soap, \"%s\", -1, &soap_tmp_%s, \"%s\"))\n\t\treturn soap->error;",
						      c_ident(p->info.typ), ns_add(p->sym->name, nse), ident(p->sym->name),
						      xsi_type_cond_u(p->info.typ, !has_ns_eq(NULL, p->sym->name)));
				      else if(p->info.typ->type == Tpointer && is_qname((Tnode *)p->info.typ->ref))
					      fprintf(fout, "\n\tif(soap_out_%s(soap, \"%s\", -1, (char*const*)&soap_tmp_%s, \"%s\"))\n\t\treturn soap->error;",
						      c_ident((Tnode *)p->info.typ->ref), ns_add(p->sym->name, nse), ident(p->sym->name),
						      xsi_type_cond_u(p->info.typ, !has_ns_eq(NULL, p->sym->name)));
				      else if(p->info.typ->type == Tpointer && is_stdqname((Tnode *)p->info.typ->ref))
					      fprintf(fout, "\n\tif(soap_out_%s(soap, \"%s\", -1, &soap_tmp_%s, \"%s\"))\n\t\treturn soap->error;",
						      c_ident(p->info.typ), ns_add(p->sym->name, nse), ident(p->sym->name),
						      xsi_type_cond_u(p->info.typ, !has_ns_eq(NULL, p->sym->name)));
				      else if(is_XML(p->info.typ) && is_string(p->info.typ))
					      fprintf(fout, "\n\tsoap_outliteral(soap, \"%s\", &a->%s, NULL);", ns_add(p->sym->name, nse), ident(p->sym->name));
				      else if(is_XML(p->info.typ) && is_wstring(p->info.typ))
					      fprintf(fout, "\n\tsoap_outwliteral(soap, \"%s\", &a->%s, NULL);", ns_add(p->sym->name, nse), ident(p->sym->name));
				      else if(p->info.typ->type == Tpointer && !is_void(p->info.typ) && p->info.minOccurs > 0)
					      fprintf(fout, "\n\tif(a->%s) {\n\t\tif(soap_out_%s(soap, \"%s\", -1, &a->%s, \"%s\"))\n\t\t\treturn soap->error;\n\t}\n\telse if(soap_element_nil(soap, \"%s\"))\n\t\treturn soap->error;",
						      ident(p->sym->name), c_ident(p->info.typ), ns_add(p->sym->name, nse), ident(p->sym->name),
						      xsi_type_cond_u(p->info.typ, !has_ns_eq(NULL, p->sym->name)), ns_add(p->sym->name, nse));
				      else if(p->info.typ->type != Tfun && !is_void(p->info.typ))
					      fprintf(fout, "\n\tif(soap_out_%s(soap, \"%s\", -1, &a->%s, \"%s\"))\n\t\treturn soap->error;",
						      c_ident(p->info.typ), ns_add(p->sym->name, nse), ident(p->sym->name),
						      xsi_type_cond_u(p->info.typ, !has_ns_eq(NULL, p->sym->name)));
			      }
		      }
		      if(!is_invisible(typ->id->name))
			      fprintf(fout, "\n\treturn soap_element_end_out(soap, tag);\n}");
		      else
			      fprintf(fout, "\n\treturn SOAP_OK;\n}"); }
		fflush(fout);
		break;

	    case Tclass:
		table = (Table *)typ->ref;
		if(!is_volatile(typ) && !is_typedef(typ)) {
			if(is_external(typ)) {
				fprintf(fhead, "\nSOAP_FMAC1 int SOAP_FMAC2 soap_out_%s(struct soap*, const char*, int, const %s, const char*);",
					c_ident(typ), c_type_id(typ, "*"));
				return;
			}
			fprintf(fout, "\n\nint %s::soap_out(struct soap *soap, const char *tag, int id, const char *type) const", ident(typ->id->name));
			fprintf(fout, "\n\t{ return soap_out_%s(soap, tag, id, this, type); }", c_ident(typ));
		}
		fprintf(fhead, "\n\nSOAP_FMAC3 int FASTCALL soap_out_%s(struct soap*, const char*, int, const %s, const char*);",
			c_ident(typ), c_type_id(typ, "*"));
		fprintf(fout, "\n\nSOAP_FMAC3 int FASTCALL soap_out_%s(struct soap *soap, const char *tag, int id, const %s, const char *type)\n{",
			c_ident(typ), c_type_id(typ, "*a"));
		fflush(fout);
		if(has_setter(typ))
			fprintf(fout, "\n\t((%s)a)->set(soap);", c_type_id(typ, "*"));
		for(t = table; t; t = t->prev) {
			Entry * e = entry(classtable, t->sym);
			char * nsa1 = e ? ns_qualifiedAttribute(e->info.typ) : nsa;
			for(p = t->list; p; p = p->next) {
				if(is_repetition(p))
					p = p->next;
				else if(p->info.sto&Sattribute)
					soap_set_attr(p, ptr_cast(t, "a"), ident(p->sym->name), ns_add(p->sym->name, nsa1));
				else if(is_qname(p->info.typ))
					fprintf(fout, "\n\tconst char *soap_tmp_%s = soap_QName2s(soap, a->%s);",
						ident(p->sym->name), ident(p->sym->name));
				else if(is_stdqname(p->info.typ))
					fprintf(fout, "\n\tstd::string soap_tmp_%s(soap_QName2s(soap, a->%s.c_str()));",
						ident(p->sym->name), ident(p->sym->name));
				else if(p->info.typ->type == Tpointer && is_qname((Tnode *)p->info.typ->ref))
					fprintf(fout,
						"\n\tconst char *soap_tmp_%s = a->%s ? soap_QName2s(soap, *a->%s) : NULL;",
						ident(p->sym->name), ident(p->sym->name), ident(p->sym->name));
				else if(p->info.typ->type == Tpointer && is_stdqname((Tnode *)p->info.typ->ref))
					fprintf(fout,
						"\n\tstd::string soap_temp_%s(a->%s ? soap_QName2s(soap, a->%s->c_str()) : \"\"), *soap_tmp_%s = a->%s ? &soap_temp_%s : NULL;",
						ident(p->sym->name), ident(p->sym->name), ident(p->sym->name),
						ident(p->sym->name), ident(p->sym->name), ident(p->sym->name));
			}
		}
		if(is_primclass(typ)) {
			for(t = table; t; t = t->prev) {
				p = t->list;
				if(p && is_item(p))
					break;
			}
			if((p->info.sto&SmustUnderstand) && !(p->info.sto&(Sconst|Sprivate|Sprotected)) &&
			   !(p->info.sto&Sattribute) && !is_transient(p->info.typ) && !is_void(p->info.typ) &&
			   p->info.typ->type != Tfun)
				fprintf(fout, "\n\tsoap->mustUnderstand = 1;");
			if(table->prev) {
				if(is_XML(p->info.typ) && is_string(p->info.typ))
					fprintf(fout, "\n\treturn soap_outliteral(soap, tag, &(a->%s::%s), \"%s\");",
						ident(t->sym->name), ident(p->sym->name), xsi_type(typ));
				else if(is_XML(p->info.typ) && is_wstring(p->info.typ))
					fprintf(fout, "\n\treturn soap_outwliteral(soap, tag, &(a->%s::%s), \"%s\");",
						ident(t->sym->name), ident(p->sym->name), xsi_type(typ));
				else if(p->info.typ->type==Tarray)
					fprintf(fout, "\n\treturn soap_out_%s(soap, tag, id, a->%s::%s, \"%s\");",
						c_ident(p->info.typ), ident(t->sym->name), ident(p->sym->name), xsi_type(typ));
				else if(p->info.typ->type==Tclass && !is_external(p->info.typ) &&
				        !is_volatile(p->info.typ) && !is_typedef(p->info.typ))
					fprintf(fout, "\n\treturn (a->%s::%s).soap_out(soap, tag, id, \"%s\");",
						ident(t->sym->name), ident(p->sym->name), xsi_type(typ));
				else if(is_qname(p->info.typ))
					fprintf(fout,
						"\n\treturn soap_out_%s(soap, tag, id, (char*const*)&soap_tmp_%s, \"%s\");",
						c_ident(p->info.typ), ident(p->sym->name), xsi_type(typ));
				else if(is_stdqname(p->info.typ))
					fprintf(fout, "\n\treturn soap_out_%s(soap, tag, id, &soap_tmp_%s, \"%s\");",
						c_ident(p->info.typ), ident(p->sym->name), xsi_type(typ));
				else if(p->info.typ->type == Tpointer && is_qname((Tnode *)p->info.typ->ref))
					fprintf(fout,
						"\n\treturn soap_out_%s(soap, tag, id, (char*const*)&soap_tmp_%s, \"%s\");",
						c_ident((Tnode *)p->info.typ->ref), ident(p->sym->name), xsi_type_u(typ));
				else if(p->info.typ->type == Tpointer && is_stdqname((Tnode *)p->info.typ->ref))
					fprintf(fout, "\n\treturn soap_out_%s(soap, tag, id, &soap_tmp_%s, \"%s\");",
						c_ident(p->info.typ), ident(p->sym->name), xsi_type_u(typ));
				else if(p->info.typ->type != Tfun && !is_void(p->info.typ))
					fprintf(fout, "\n\treturn soap_out_%s(soap, tag, id, &(a->%s::%s), \"%s\");",
						c_ident(p->info.typ), ident(t->sym->name), ident(p->sym->name), xsi_type(typ));
				else
					fprintf(fout, "\n\treturn SOAP_OK;");
			}
			else {
				if(is_XML(p->info.typ) && is_string(p->info.typ))
				      fprintf(fout, "\n\treturn soap_outliteral(soap, tag, &(a->%s::%s), NULL);",
					      ident(t->sym->name), ident(p->sym->name));
			      else if(is_XML(p->info.typ) && is_wstring(p->info.typ))
				      fprintf(fout, "\n\treturn soap_outwliteral(soap, tag, &(a->%s::%s), NULL);",
					      ident(t->sym->name), ident(p->sym->name));
			      else if(p->info.typ->type==Tarray)
				      fprintf(fout, "\n\treturn soap_out_%s(soap, tag, id, a->%s::%s, \"%s\");",
					      c_ident(p->info.typ), ident(t->sym->name), ident(p->sym->name), xsi_type_u(typ));
			      else if(p->info.typ->type==Tclass && !is_external(p->info.typ) &&
				      !is_volatile(p->info.typ) && !is_typedef(p->info.typ))
				      fprintf(fout, "\n\treturn (a->%s::%s).soap_out(soap, tag, id, \"%s\");",
					      ident(t->sym->name), ident(p->sym->name), xsi_type_u(typ));
			      else if(is_qname(p->info.typ))
				      fprintf(fout,
					      "\n\treturn soap_out_%s(soap, tag, id, (char*const*)&soap_tmp_%s, \"%s\");",
					      c_ident(p->info.typ), ident(p->sym->name), xsi_type_u(typ));
			      else if(is_stdqname(p->info.typ))
				      fprintf(fout, "\n\treturn soap_out_%s(soap, tag, id, &soap_tmp_%s, \"%s\");",
					      c_ident(p->info.typ), ident(p->sym->name), xsi_type_u(typ));
			      else if(p->info.typ->type == Tpointer && is_qname((Tnode *)p->info.typ->ref))
				      fprintf(fout,
					      "\n\treturn soap_out_%s(soap, tag, id, (char*const*)&soap_tmp_%s, \"%s\");",
					      c_ident((Tnode *)p->info.typ->ref), ident(p->sym->name), xsi_type_u(typ));
			      else if(p->info.typ->type == Tpointer && is_stdqname((Tnode *)p->info.typ->ref))
				      fprintf(fout, "\n\treturn soap_out_%s(soap, tag, id, &soap_tmp_%s, \"%s\");",
					      c_ident((Tnode *)p->info.typ->ref), ident(p->sym->name), xsi_type_u(typ));
			      else if(p->info.typ->type != Tfun && !is_void(p->info.typ))
				      fprintf(fout, "\n\treturn soap_out_%s(soap, tag, id, &a->%s::%s, \"%s\");",
					      c_ident(p->info.typ), ident(t->sym->name), ident(p->sym->name), xsi_type_u(typ));
			      else
				      fprintf(fout, "\n\treturn SOAP_OK;"); 
			}
			fprintf(fout, "\n}");
		}
		else {
			if(!is_invisible(typ->id->name)) {
			      if(table && table->prev)
				      fprintf(fout,
					      "\n\tif(soap_element_begin_out(soap, tag, soap_embedded_id(soap, id, a, %s), \"%s\"))\n\t\treturn soap->error;",
					      soap_type(typ), xsi_type(typ));
			      else
				      fprintf(fout,
					      "\n\tif(soap_element_begin_out(soap, tag, soap_embedded_id(soap, id, a, %s), type))\n\t\treturn soap->error;",
					      soap_type(typ));
		      }
		      fflush(fout);

		      i = 0;
			/* Get the depth of the inheritance hierarchy */
		      for(t = table; t; t = t->prev)
			      i++;
			/* Call routines to output the member data of the class */
			/* Data members of the Base Classes are outputed first
			 followed by the data members of the Derived classes.
			 Overridden data members are output twice once for the base class
			 they are defined in and once for the derived class that overwrites
			 them */

		      for(; i > 0; i--) {
			      Entry * e;
			      char * nse1;
			      t = table;
			      for(j = 0; j< i-1; j++)
				      t = t->prev;
			      e = entry(classtable, t->sym);
			      nse1 = e ? ns_qualifiedElement(e->info.typ) : nse;
			      for(p = t->list; p != (Entry *)0; p = p->next) {
				      if(p->info.sto&Sreturn) {
					      if(nse1 || has_ns_eq(NULL, p->sym->name)) {
						      if(p->info.typ->type == Tpointer)
							      fprintf(fout, "\n\tif(a->%s)\n\t\tsoap_element_result(soap, \"%s\");",
								      ident(p->sym->name), ns_add(p->sym->name, nse1));
						      else
							      fprintf(fout, "\n\tsoap_element_result(soap, \"%s\");", ns_add(p->sym->name, nse1));
					      }
				      }
				      if((p->info.sto&SmustUnderstand) && !(p->info.sto&(Sconst|Sprivate|Sprotected)) && !(p->info.sto&Sattribute) &&
				         !is_transient(p->info.typ) && !is_void(p->info.typ) && p->info.typ->type != Tfun)
					      fprintf(fout, "\n\tsoap->mustUnderstand = 1;");
				      needs_lang(p);
				      if(is_item(p))
					      ;
				      else if(p->info.sto&(Sconst|Sprivate|Sprotected))
					      fprintf(fout, "\n\t/* non-serializable %s skipped */", ident(p->sym->name));
				      else if(is_transient(p->info.typ))
					      fprintf(fout, "\n\t/* transient %s skipped */", ident(p->sym->name));
				      else if(p->info.sto&Sattribute)
					      ;
				      else if(is_repetition(p)) {
					      fprintf(fout, "\n\tif(a->%s::%s)", ident(t->sym->name), ident(p->next->sym->name));
					      fprintf(fout, " {\n\t\tint i;\n\t\tfor(i = 0; i < a->%s::%s; i++)", ident(t->sym->name), ident(p->sym->name));
					      if(((Tnode *)p->next->info.typ->ref)->type == Tclass && !is_external((Tnode *)p->next->info.typ->ref) &&
					         !is_volatile((Tnode *)p->next->info.typ->ref) && !is_typedef((Tnode *)p->next->info.typ->ref))
						      fprintf(fout,
							      "\n\t\t\tif(a->%s::%s[i].soap_out(soap, \"%s\", -1, \"%s\"))\n\t\t\t\treturn soap->error;",
							      ident(t->sym->name), ident(p->next->sym->name), ns_add_overridden(t, p->next, nse1),
							      xsi_type_cond_u((Tnode *)p->next->info.typ->ref, !has_ns_eq(NULL, p->next->sym->name)));
					      else if(is_qname((Tnode *)p->next->info.typ->ref))
						      fprintf(fout,
							      "\n\t\t{\tconst char *soap_tmp_%s = soap_QName2s(soap, a->%s[i]);\n\t\t\tif(soap_out_%s(soap, \"%s\", -1, (char*const*)&soap_tmp_%s, \"%s\"))\n\t\t\t\treturn soap->error;\n\t\t}",
							      ident(p->next->sym->name), ident(p->next->sym->name),
							      c_ident((Tnode *)p->next->info.typ->ref),
							      ns_add(p->next->sym->name, nse1), ident(p->next->sym->name),
							      xsi_type_cond_u((Tnode *)p->next->info.typ->ref, !has_ns_eq(NULL, p->next->sym->name)));
					      else if(is_XML((Tnode *)p->next->info.typ->ref) && is_string((Tnode *)p->next->info.typ->ref))
						      fprintf(fout, "\n\t\t\tsoap_outliteral(soap, \"%s\", a->%s::%s + i, NULL);",
							      ns_add(p->next->sym->name, nse1), ident(t->sym->name), ident(p->next->sym->name));
					      else if(is_XML((Tnode *)p->next->info.typ->ref) && is_wstring((Tnode *)p->next->info.typ->ref))
						      fprintf(fout, "\n\t\t\tsoap_outwliteral(soap, \"%s\", a->%s::%s + i, NULL);",
							      ns_add(p->next->sym->name, nse1), ident(t->sym->name), ident(p->next->sym->name));
					      else
						      fprintf(fout, "\n\t\t\tif(soap_out_%s(soap, \"%s\", -1, a->%s::%s + i, \"%s\"))\n\t\t\t\treturn soap->error;",
							      c_ident((Tnode *)p->next->info.typ->ref),
							      ns_add_overridden(t, p->next, nse1), ident(t->sym->name), ident(p->next->sym->name),
							      xsi_type_cond_u((Tnode *)p->next->info.typ->ref, !has_ns_eq(NULL, p->next->sym->name)));
					      fprintf(fout, "\n\t}");
					      p = p->next;
				      }
				      else if(is_anytype(p)) {
					      fprintf(fout,
						      "\n\tif(soap_putelement(soap, a->%s::%s, \"%s\", -1, a->%s::%s))\n\t\treturn soap->error;",
						      ident(t->sym->name), ident(p->next->sym->name),
						      ns_add(p->sym->name,
							      nse1), ident(t->sym->name), ident(p->sym->name));
					      p = p->next;
				      }
				      else if(is_choice(p)) {
					      fprintf(fout,
						      "\n\tif(soap_out_%s(soap, a->%s::%s, &a->%s::%s))\n\t\treturn soap->error;",
						      c_ident(p->next->info.typ), ident(t->sym->name), ident(p->sym->name), ident(t->sym->name),
						      ident(p->next->sym->name));
					      p = p->next;
				      }
				      else if(p->info.typ->type==Tarray)
					      fprintf(fout, "\n\tsoap_out_%s(soap, \"%s\", -1, a->%s::%s, \"%s\");",
						      c_ident(p->info.typ),
						      ns_add_overridden(t, p, nse1), ident(t->sym->name), ident(p->sym->name),
						      xsi_type_cond_u(p->info.typ, !has_ns_eq(NULL, p->sym->name)));
				      else if(p->info.typ->type==Tclass && !is_external(p->info.typ) &&
				              !is_volatile(p->info.typ) && !is_typedef(p->info.typ))
					      fprintf(fout,
						      "\n\tif((a->%s::%s).soap_out(soap, \"%s\", -1, \"%s\"))\n\t\treturn soap->error;",
						      ident(t->sym->name), ident(p->sym->name), ns_add_overridden(t, p, nse1),
						      xsi_type_cond_u(p->info.typ, !has_ns_eq(NULL, p->sym->name)));
				      else if(is_qname(p->info.typ))
					      fprintf(fout,
						      "\n\tif(soap_out_%s(soap, \"%s\", -1, (char*const*)&soap_tmp_%s, \"%s\"))\n\t\treturn soap->error;",
						      c_ident(p->info.typ), ns_add_overridden(t, p, nse1), ident(
							      p->sym->name),
						      xsi_type_cond_u(p->info.typ, !has_ns_eq(NULL, p->sym->name)));
				      else if(is_stdqname(p->info.typ))
					      fprintf(fout,
						      "\n\tif(soap_out_%s(soap, \"%s\", -1, &soap_tmp_%s, \"%s\"))\n\t\treturn soap->error;",
						      c_ident(p->info.typ), ns_add_overridden(t, p, nse1), ident(
							      p->sym->name),
						      xsi_type_cond_u(p->info.typ, !has_ns_eq(NULL, p->sym->name)));
				      else if(p->info.typ->type == Tpointer && is_qname((Tnode *)p->info.typ->ref))
					      fprintf(fout,
						      "\n\tif(soap_out_%s(soap, \"%s\", -1, (char*const*)&soap_tmp_%s, \"%s\"))\n\t\treturn soap->error;",
						      c_ident((Tnode *)p->info.typ->ref),
						      ns_add(p->sym->name,
							      nse1), ident(p->sym->name),
						      xsi_type_cond_u(p->info.typ, !has_ns_eq(NULL, p->sym->name)));
				      else if(p->info.typ->type == Tpointer && is_stdqname((Tnode *)p->info.typ->ref))
					      fprintf(fout,
						      "\n\tif(soap_out_%s(soap, \"%s\", -1, &soap_tmp_%s, \"%s\"))\n\t\treturn soap->error;",
						      c_ident(p->info.typ), ns_add(p->sym->name, nse1), ident(p->sym->name),
						      xsi_type_cond_u(p->info.typ, !has_ns_eq(NULL, p->sym->name)));
				      else if(is_XML(p->info.typ) && is_string(p->info.typ))
					      fprintf(fout, "\n\tsoap_outliteral(soap, \"%s\", &(a->%s::%s), NULL);",
						      ns_add_overridden(t, p, nse1), ident(t->sym->name), ident(p->sym->name));
				      else if(is_XML(p->info.typ) && is_wstring(p->info.typ))
					      fprintf(fout, "\n\tsoap_outwliteral(soap, \"%s\", &(a->%s::%s), NULL);",
						      ns_add_overridden(t, p, nse1), ident(t->sym->name), ident(p->sym->name));
				      else if(p->info.typ->type == Tpointer && !is_void(p->info.typ) && p->info.minOccurs > 0)
					      fprintf(fout,
						      "\n\tif(a->%s::%s) {\n\t\tif(soap_out_%s(soap, \"%s\", -1, &a->%s::%s, \"%s\"))\n\t\t\treturn soap->error;\n\t}\n\telse if(soap_element_nil(soap, \"%s\"))\n\t\treturn soap->error;",
						      ident(t->sym->name), ident(p->sym->name), c_ident(p->info.typ),
						      ns_add_overridden(t, p, nse1), ident(t->sym->name), ident(p->sym->name),
						      xsi_type_cond_u(p->info.typ, !has_ns_eq(NULL, p->sym->name)), ns_add_overridden(t, p, nse1));
				      else if(p->info.typ->type != Tfun && !is_void(p->info.typ))
					      fprintf(fout,
						      "\n\tif(soap_out_%s(soap, \"%s\", -1, &(a->%s::%s), \"%s\"))\n\t\treturn soap->error;",
						      c_ident(p->info.typ), ns_add_overridden(t, p, nse1), ident(t->sym->name), ident(p->sym->name),
						      xsi_type_cond_u(p->info.typ, !has_ns_eq(NULL, p->sym->name)));
				      fflush(fout);
			      }
		      }
		      if(!is_invisible(typ->id->name))
			      fprintf(fout, "\n\treturn soap_element_end_out(soap, tag);\n}");
		      else
			      fprintf(fout, "\n\treturn SOAP_OK;\n}"); }
		fflush(fout);
		break;

	    case Tunion:
		if(is_external(typ)) {
			fprintf(fhead, "\nSOAP_FMAC1 int SOAP_FMAC2 soap_out_%s(struct soap*, int, const %s);",
				c_ident(typ), c_type_id(typ, "*"));
			return;
		}
		fprintf(fhead, "\nSOAP_FMAC3 int FASTCALL soap_out_%s(struct soap*, int, const %s);", c_ident(typ), c_type_id(typ, "*"));
		fprintf(fout, "\n\nSOAP_FMAC3 int FASTCALL soap_out_%s(struct soap *soap, int choice, const %s)\n{", c_ident(typ), c_type_id(typ, "*a"));
		table = (Table *)typ->ref;
		fprintf(fout, "\n\tswitch(choice) {\n\t");
		for(p = table->list; p; p = p->next) {
			if(p->info.sto&(Sconst|Sprivate|Sprotected))
				fprintf(fout, "\n\t/* non-serializable %s skipped */", ident(p->sym->name));
			else if(is_transient(p->info.typ))
				fprintf(fout, "\n\t/* transient %s skipped */", ident(p->sym->name));
			else if(p->info.sto&Sattribute)
				;
			else if(is_repetition(p))
				;
			else if(is_anytype(p))
				;
			else if(p->info.typ->type == Tarray) {
				fprintf(fout, "\n\tcase SOAP_UNION_%s_%s:", c_ident(typ), ident(p->sym->name));
				fprintf(fout, "\n\t\treturn soap_out_%s(soap, \"%s\", -1, a->%s, \"%s\");",
					c_ident(p->info.typ), ns_add(p->sym->name, nse), ident(p->sym->name), xsi_type_cond_u(p->info.typ, !has_ns_eq(NULL, p->sym->name)));
			}
			else if(p->info.typ->type == Tclass && !is_external(p->info.typ) && !is_volatile(p->info.typ) && !is_typedef(p->info.typ)) {
				fprintf(fout, "\n\tcase SOAP_UNION_%s_%s:", c_ident(typ), ident(p->sym->name));
				fprintf(fout, "\n\t\treturn a->%s.soap_out(soap, \"%s\", -1, \"%s\");",
					ident(p->sym->name), ns_add(p->sym->name, nse), xsi_type_cond_u(p->info.typ, !has_ns_eq(NULL, p->sym->name)));
			}
			else if(is_qname(p->info.typ) || is_stdqname(p->info.typ)) {
				fprintf(fout, "\n\tcase SOAP_UNION_%s_%s:", c_ident(typ), ident(p->sym->name));
				fprintf(fout, "\n\t{\tconst char *soap_tmp_%s = soap_QName2s(soap, a->%s);",
					ident(p->sym->name), ident(p->sym->name));
				fprintf(fout, "\n\t\treturn soap_out_%s(soap, \"%s\", -1, (char*const*)&soap_tmp_%s, \"%s\");\n\t}",
					c_ident(p->info.typ), ns_add(p->sym->name, nse), ident(p->sym->name),
					xsi_type_cond_u(p->info.typ, !has_ns_eq(NULL, p->sym->name)));
			}
			else if(is_XML(p->info.typ) && is_string(p->info.typ)) {
				fprintf(fout, "\n\tcase SOAP_UNION_%s_%s:", c_ident(typ), ident(p->sym->name));
				fprintf(fout, "\n\t\treturn soap_outliteral(soap, \"%s\", &a->%s, NULL);",
					ns_add(p->sym->name, nse), ident(p->sym->name));
			}
			else if(is_XML(p->info.typ) && is_wstring(p->info.typ)) {
				fprintf(fout, "\n\tcase SOAP_UNION_%s_%s:", c_ident(typ), ident(p->sym->name));
				fprintf(fout, "\n\t\treturn soap_outwliteral(soap, \"%s\", &a->%s, NULL);",
					ns_add(p->sym->name, nse), ident(p->sym->name));
			}
			else if(p->info.typ->type != Tfun && !is_void(p->info.typ)) {
				fprintf(fout, "\n\tcase SOAP_UNION_%s_%s:", c_ident(typ), ident(p->sym->name));
				fprintf(fout, "\n\t\treturn soap_out_%s(soap, \"%s\", -1, &a->%s, \"%s\");",
					c_ident(p->info.typ), ns_add(p->sym->name, nse), ident(p->sym->name),
					xsi_type_cond_u(p->info.typ, !has_ns_eq(NULL, p->sym->name)));
			}
		}
		fprintf(fout, "\n\tdefault:\n\t\tbreak;\n\t}\n\treturn SOAP_OK;\n}");
		fflush(fout);
		break;

	    case Tpointer:
		if(is_external(typ)) {
			fprintf(fhead, "\nSOAP_FMAC1 int SOAP_FMAC2 soap_out_%s(struct soap*, const char *, int, %s, const char *);",
				c_ident(typ), c_type_id(typ, "const*"));
			return;
		}
		fprintf(fhead, "\nSOAP_FMAC3 int FASTCALL soap_out_%s(struct soap*, const char *, int, %s, const char *);",
			c_ident(typ), c_type_id(typ, "const*"));
		fprintf(fout, "\n\nSOAP_FMAC3 int FASTCALL soap_out_%s(struct soap *soap, const char *tag, int id, %s, const char *type)\n{",
			c_ident(typ), c_type_id(typ, "const*a"));
		if(is_template(typ)) {
			fprintf(fout, "\n\tif(!*a)");
			fprintf(fout, "\n\t\treturn soap_element_null(soap, tag, id, type);");
			fprintf(fout, "\n\treturn soap_out_%s(soap, tag, id, *a, type);", c_ident((Tnode *)typ->ref));
		}
		else {p = is_dynamic_array((Tnode *)typ->ref);
		      if(p) {
			      d = get_Darraydims((Tnode *)typ->ref);
			      if(d)
				      fprintf(fout, "\n\tid = soap_element_id(soap, tag, id, *a, (struct soap_array*)&(*a)->%s, %d, type, %s);",
					      ident(p->sym->name), d, soap_type((Tnode *)typ->ref));
			      else
				      fprintf(fout, "\n\tid = soap_element_id(soap, tag, id, *a, (struct soap_array*)&(*a)->%s, 1, type, %s);",
					      ident(p->sym->name), soap_type((Tnode *)typ->ref));
		      }
		      else
			      fprintf(fout, "\n\tid = soap_element_id(soap, tag, id, *a, NULL, 0, type, %s);", soap_type((Tnode *)typ->ref));
		      //fprintf(fout, "\n\tif(id < 0)\n\t\treturn soap->error;");
		      if(((Tnode *)typ->ref)->type == Tclass && !is_external((Tnode *)typ->ref) && !is_volatile((Tnode *)typ->ref) && !is_typedef((Tnode *)typ->ref))
				  fprintf(fout, "\n\treturn (id < 0) ? soap->error : (*a)->soap_out(soap, tag, id, type);");
		      else
			      fprintf(fout, "\n\treturn (id < 0) ? soap->error : soap_out_%s(soap, tag, id, *a, type);", c_ident((Tnode *)typ->ref)); }
		fprintf(fout, "\n}");
		break;

	    case Tarray:
		if(is_external(typ)) {
			fprintf(fhead, "\nSOAP_FMAC1 int SOAP_FMAC2 soap_out_%s(struct soap*, const char*, int, %s, const char*);",
				c_ident(typ), c_type_id(typ, "const"));
			return;
		}
		fprintf(fhead, "\nSOAP_FMAC3 int FASTCALL soap_out_%s(struct soap*, const char*, int, %s, const char*);",
			c_ident(typ), c_type_id(typ, "const"));
		fprintf(fout,
			"\n\nSOAP_FMAC3 int FASTCALL soap_out_%s(struct soap *soap, const char *tag, int id, %s, const char *type)\n{",
			c_ident(typ), c_type_id(typ, "const a"));
		fprintf(fout, "\n\tint i;");
		fprintf(fout, "\n\tsoap_array_begin_out(soap, tag, soap_embedded_id(soap, id, a, %s), \"%s[%d]\", 0);",
			soap_type(typ), xsi_type_Tarray(typ), get_dimension(typ));
		n = (Tnode *)typ->ref;
		cardinality = 1;
		while(n->type==Tarray) {
			n = (Tnode *)n->ref;
			cardinality++;
		}
		fprintf(fout, "\n\tfor(i = 0; i < %d; i++) {\n\t", get_dimension(typ));
		if(((Tnode *)typ->ref)->type == Tclass && !is_external((Tnode *)typ->ref) &&
		   !is_volatile((Tnode *)typ->ref) && !is_typedef((Tnode *)typ->ref)) {
			if(cardinality>1)
				fprintf(fout, "\n\t\ta[i].soap_out(soap, \"item\", -1, \"%s\")", xsi_type_u((Tnode *)typ->ref));
			else 
				fprintf(fout, "\n\t\t(a+i)->soap_out(soap, \"item\", -1, \"%s\")", xsi_type_u((Tnode *)typ->ref));
		}
		else {
			if(((Tnode *)typ->ref)->type != Tarray) {
			      if(((Tnode *)typ->ref)->type == Tpointer)
				      fprintf(fout,
					      "\n\t\tsoap->position = 1;\n\t\tsoap->positions[0] = i;\n\t\tsoap_out_%s(soap, \"item\", -1, a",
					      c_ident((Tnode *)typ->ref));
			      else
				      fprintf(fout, "\n\t\tsoap_out_%s(soap, \"item\", -1, a", c_ident((Tnode *)typ->ref));
		      }
		      else
			      fprintf(fout, "\n\t\tsoap_out_%s(soap, \"item\", -1, a", c_ident((Tnode *)typ->ref));
		      if(cardinality>1)
			      fprintf(fout, "[i], \"%s\")", xsi_type_u((Tnode *)typ->ref));
		      else
			      fprintf(fout, "+i, \"%s\")", xsi_type_u((Tnode *)typ->ref)); 
		}
		if(((Tnode *)typ->ref)->type == Tpointer)
			fprintf(fout, ";\n\t}\n\tsoap->position = 0;\n\treturn soap_element_end_out(soap, tag);\n}");
		else
			fprintf(fout, ";\n\t}\n\treturn soap_element_end_out(soap, tag);\n}");
		break;

	    case Tenum:
		if(is_external(typ)) {
			fprintf(fhead,
				"\nSOAP_FMAC1 int SOAP_FMAC2 soap_out_%s(struct soap*, const char*, int, const %s, const char*);",
				c_ident(typ), c_type_id(typ, "*"));
			return;
		}
		fprintf(fhead,
			"\nSOAP_FMAC3 int FASTCALL soap_out_%s(struct soap*, const char*, int, const %s, const char*);",
			c_ident(typ), c_type_id(typ, "*"));
		if(!is_typedef(typ)) {
			fprintf(fout, "\n\nstatic const struct soap_code_map soap_codes_%s[] =\n{", c_ident(typ));
			for(t = (Table *)typ->ref; t; t = t->prev) {
				for(p = t->list; p; p = p->next)
					fprintf(fout, "\t{ (long)%s, \"%s\" },\n", ident(p->sym->name),
						ns_remove2(p->sym->name));
			}
			fprintf(fout, "\t{ 0, NULL }\n");
			fprintf(fout, "};");
		}
		fprintf(fhead, "\n\nSOAP_FMAC3S const char* SOAP_FMAC4S soap_%s2s(struct soap*, %s);", c_ident(typ), c_type(typ));
		fprintf(fout, "\n\nSOAP_FMAC3S const char* SOAP_FMAC4S soap_%s2s(struct soap *soap, %s)", c_ident(typ), c_type_id(typ, "n"));
		if(is_typedef(typ))
			fprintf(fout, "\n{\n\treturn soap_%s2s(soap, n);\n}", t_ident(typ));
		else if(is_boolean(typ))
			fprintf(fout, "\n{\n\t(void)soap; /* appease -Wall -Werror */\nreturn soap_code_str(soap_codes_%s, n!=0);\n}", c_ident(typ));
		else if(!is_mask(typ)) {
			fprintf(fout, "\n{\n\tconst char *s = soap_code_str(soap_codes_%s, (long)n);", c_ident(typ));
			fprintf(fout, "\n\tif(s)\n\t\treturn s;");
			fprintf(fout, "\n\treturn soap_long2s(soap, (long)n);");
			fprintf(fout, "\n}");
		}
		else
			fprintf(fout, "\n{\n\treturn soap_code_list(soap, soap_codes_%s, (long)n);\n}", c_ident(typ));
		fprintf(fout,
			"\n\nSOAP_FMAC3 int FASTCALL soap_out_%s(struct soap *soap, const char *tag, int id, const %s, const char *type)",
			c_ident(typ), c_type_id(typ, "*a"));
		fprintf(fout, "\n{\n\tif(soap_element_begin_out(soap, tag, soap_embedded_id(soap, id, a, %s), type)", soap_type(typ));
		fprintf(fout, " || soap_send(soap, soap_%s2s(soap, *a)))\n\t\treturn soap->error;", c_ident(typ));
		fprintf(fout, "\n\treturn soap_element_end_out(soap, tag);\n}");
		break;
	    case Ttemplate:
		if(is_external(typ)) {
			fprintf(fhead,
				"\nSOAP_FMAC1 int SOAP_FMAC2 soap_out_%s(struct soap*, const char*, int, const %s, const char*);",
				c_ident(typ), c_type_id(typ, "*"));
			return;
		}
		fprintf(fhead,
			"\nSOAP_FMAC3 int FASTCALL soap_out_%s(struct soap*, const char*, int, const %s, const char*);",
			c_ident(typ), c_type_id(typ, "*"));
		n = (Tnode *)typ->ref;
		if(!n)
			return;
		fprintf(fout,
			"\n\nSOAP_FMAC3 int FASTCALL soap_out_%s(struct soap *soap, const char *tag, int id, const %s, const char *type)\n{",
			c_ident(typ), c_type_id(typ, "*a"));

		fprintf(fout, "\n\tfor(%s::const_iterator i = a->begin(); i != a->end(); ++i) {\n\t", c_type(typ));
		if(n->type==Tarray)
			fprintf(fout, "\n\t\tif(soap_out_%s(soap, tag, id, *i, \"%s\"))", c_ident(n), xsi_type_u(typ));
		else if(n->type==Tclass && !is_external(n) && !is_volatile(n) && !is_typedef(n))
			fprintf(fout, "\n\t\tif((*i).soap_out(soap, tag, id, \"%s\"))", xsi_type_u(typ));
		else if(is_qname(n))
			fprintf(fout,
				"\n\t\tconst char *soap_tmp = soap_QName2s(soap, *i);\n\t\tif(soap_out_%s(soap, tag, id, (char*const*)&soap_tmp, \"%s\"))",
				c_ident(n), xsi_type_u(typ));
		else if(is_stdqname(n))
			fprintf(fout,
				"\n\t\tstd::string soap_tmp(soap_QName2s(soap, (*i).c_str()));\n\t\tif(soap_out_%s(soap, tag, id, &soap_tmp, \"%s\"))",
				c_ident(n), xsi_type_u(typ));
		else if(is_XML(n) && is_string(n))
			fprintf(fout, "\n\t\tif(soap_outliteral(soap, tag, &(*i), NULL))");
		else if(is_XML(n) && is_wstring(n))
			fprintf(fout, "\n\t\tif(soap_outwliteral(soap, tag, &(*i), NULL))");
		else if(n->type == Tenum && (Table *)n->ref == booltable)
			fprintf(fout, "\n\t\tbool b = (*i);\n\t\tif(soap_out_%s(soap, tag, id, &b, \"%s\"))", c_ident(n), xsi_type_u(typ));
		else
			fprintf(fout, "\n\t\tif(soap_out_%s(soap, tag, id, &(*i), \"%s\"))", c_ident(n), xsi_type_u(typ));
		fprintf(fout, "\n\t\t\treturn soap->error;");
		fprintf(fout, "\n\t}\n\treturn SOAP_OK;\n}");
		break;
	    default: break;
	}
}

void soap_out_Darray(Tnode * typ)
{
	int i, j, d = 0;
	Table * t;
	Entry * p;
	char * nse = ns_qualifiedElement(typ);
	char * nsa = ns_qualifiedAttribute(typ);
	char * item;
	Table * table = (Table *)typ->ref;
	fprintf(fhead, "\nSOAP_FMAC3 int FASTCALL soap_out_%s(struct soap*, const char*, int, const %s, const char*);",
		c_ident(typ), c_type_id(typ, "*"));
	if(is_external(typ))
		return;
	if(typ->type == Tclass && !is_volatile(typ) && !is_typedef(typ)) {
		fprintf(fout, "\n\nint %s::soap_out(struct soap *soap, const char *tag, int id, const char *type) const", c_type(typ));
		fprintf(fout, "\n\t{ return soap_out_%s(soap, tag, id, this, type); }", c_ident(typ));
	}
	fflush(fout);
	fprintf(fout, "\n\nSOAP_FMAC3 int FASTCALL soap_out_%s(struct soap *soap, const char *tag, int id, const %s, const char *type)\n{",
		c_ident(typ), c_type_id(typ, "*a"));
	if(has_setter(typ))
		fprintf(fout, "\n\t((%s)a)->set(soap);", c_type_id(typ, "*"));
	if(!is_binary(typ)) {
		d = get_Darraydims(typ);
		if(d)
			fprintf(fout, "\n\tint i, n = soap_size(a->__size, %d);", d);
		else
			fprintf(fout, "\n\tint i, n = a->__size;");
	}
	if(typ->type == Tclass) {
		for(t = table; t; t = t->prev)                          {
			for(p = t->list; p; p = p->next)                                                          {
				if(p->info.sto&Sattribute)
					soap_set_attr(p,
						ptr_cast(t, "a"), ident(p->sym->name), ns_add(p->sym->name, nsa));
			}
		}
	}
	else {
		for(t = table; t; t = t->prev) {
			for(p = t->list; p; p = p->next) {
				if(p->info.sto&Sattribute)
					soap_set_attr(p, "a", ident(p->sym->name), ns_add(p->sym->name, nsa));
			}
		}
	}
	p = is_dynamic_array(typ);
	if(p->sym->name[5])
		item = ns_add(p->sym->name+5, nse);
	else
		item = ns_add("item", nse);
	if(!has_ns(typ) && !is_untyped(typ) && !is_binary(typ)) {
		if(is_untyped(p->info.typ)) {
			if(has_offset(typ))
				if(d)
					fprintf(fout,
						"\n\tchar *t = a->%s ? soap_putsizesoffsets(soap, \"%s\", a->__size, a->__offset, %d) : NULL;",
						ident(p->sym->name), wsdl_type(p->info.typ, "xsd"), d);
				else
					fprintf(fout,
						"\n\tchar *t = a->%s ? soap_putsize(soap, \"%s\", n + a->__offset) : NULL;",
						ident(p->sym->name), wsdl_type(p->info.typ, "xsd"));
			else if(d)
				fprintf(fout,
					"\n\tchar *t = a->%s ? soap_putsizes(soap, \"%s\", a->__size, %d) : NULL;",
					ident(p->sym->name), wsdl_type(p->info.typ, "xsd"), d);
			else
				fprintf(fout, "\n\tchar *t = a->%s ? soap_putsize(soap, \"%s\", n) : NULL;",
					ident(p->sym->name), wsdl_type(p->info.typ, "xsd"));
		}
		else {
			if(has_offset(typ))
			      if(d)
				      fprintf(fout,
					      "\n\tchar *t = a->%s ? soap_putsizesoffsets(soap, \"%s\", a->__size, a->__offset, %d) : NULL;",
					      ident(p->sym->name), xsi_type(typ), d);
			      else
				      fprintf(fout,
					      "\n\tchar *t = a->%s ? soap_putsize(soap, \"%s\", n + a->__offset) : NULL;",
					      ident(p->sym->name), xsi_type(typ));
		      else if(d)
			      fprintf(fout, "\n\tchar *t = a->%s ? soap_putsizes(soap, \"%s\", a->__size, %d) : NULL;",
				      ident(p->sym->name), xsi_type(typ), d);
		      else
			      fprintf(fout, "\n\tchar *t = a->%s ? soap_putsize(soap, \"%s\", a->__size) : NULL;",
				      ident(p->sym->name), xsi_type(typ)); 
		}
	}
	if(d)
		fprintf(fout, "\n\tid = soap_element_id(soap, tag, id, a, (struct soap_array*)&a->%s, %d, type, %s);",
			ident(p->sym->name), d, soap_type(typ));
	else if(is_attachment(typ)) {
		fprintf(fout,
			"\n#ifndef WITH_LEANER\n\tid = soap_attachment(soap, tag, id, a, (struct soap_array*)&a->%s, a->id, a->type, a->options, 1, type, %s);",
			ident(p->sym->name), soap_type(typ));
		fprintf(fout,
			"\n#else\n\tid = soap_element_id(soap, tag, id, a, (struct soap_array*)&a->%s, 1, type, %s);\n#endif",
			ident(p->sym->name), soap_type(typ));
	}
	else
		fprintf(fout, "\n\tid = soap_element_id(soap, tag, id, a, (struct soap_array*)&a->%s, 1, type, %s);",
			ident(p->sym->name), soap_type(typ));
	fprintf(fout, "\n\tif(id < 0)\n\t\treturn soap->error;");
	fprintf(fout, "\n\tif(");
	if(has_ns(typ) || is_untyped(typ) || is_binary(typ)) {
		if(table->prev)
			fprintf(fout, "soap_element_begin_out(soap, tag, id, \"%s\")", xsi_type(typ));
		else
			fprintf(fout, "soap_element_begin_out(soap, tag, id, type)");
	}
	else if(has_offset(typ)) {
		if(d)
			fprintf(fout, "soap_array_begin_out(soap, tag, id, t, soap_putoffsets(soap, a->__offset, %d))", d);
		else
			fprintf(fout, "soap_array_begin_out(soap, tag, id, t, soap_putoffset(soap, a->__offset))");
	}
	else
		fprintf(fout, "soap_array_begin_out(soap, tag, id, t, NULL)");
	fprintf(fout, ")\n\t\treturn soap->error;");
	if(is_binary(typ) && !is_hexBinary(typ))
		fprintf(fout, "\n\tif(soap_putbase64(soap, a->__ptr, a->__size))\n\t\treturn soap->error;");
	else if(is_hexBinary(typ))
		fprintf(fout, "\n\tif(soap_puthex(soap, a->__ptr, a->__size))\n\t\treturn soap->error;");
	else {
		fprintf(fout, "\n\tfor(i = 0; i < n; i++) {\n\t");
		if(!has_ns(typ) && !is_untyped(typ)) {
		      if(d) {
			      fprintf(fout, "\n\t\tsoap->position = %d;", d);
			      for(i = 0; i < d; i++) {
				      fprintf(fout, "\n\t\tsoap->positions[%d] = i", i);
				      for(j = i+1; j < d; j++)
					      fprintf(fout, "/a->__size[%d]", j);
				      fprintf(fout, "%%a->__size[%d];", i);
			      }
			      if(is_XML((Tnode *)p->info.typ->ref) && is_string((Tnode *)p->info.typ->ref))
				      fprintf(fout, "\n\t\tsoap_outliteral(soap, \"%s\", &a->%s[i], NULL);", item, ident(p->sym->name));
			      else if(is_XML((Tnode *)p->info.typ->ref) && is_wstring((Tnode *)p->info.typ->ref))
				      fprintf(fout, "\n\t\tsoap_outwliteral(soap, \"%s\", &a->%s[i], NULL);", item, ident(p->sym->name));
			      else if(((Tnode *)p->info.typ->ref)->type == Tclass && !is_external((Tnode *)p->info.typ->ref) &&
						!is_volatile((Tnode *)p->info.typ->ref) && !is_typedef((Tnode *)p->info.typ->ref))
				      fprintf(fout, "\n\t\ta->%s[i].soap_out(soap, \"item\", -1, \"%s\");",
					      ident(p->sym->name), xsi_type_u(((Tnode *)p->info.typ->ref)));
			      else
				      fprintf(fout, "\n\t\tsoap_out_%s(soap, \"%s\", -1, &a->%s[i], \"%s\");",
					      c_ident(((Tnode *)p->info.typ->ref)), item, ident(p->sym->name),
					      xsi_type_u(((Tnode *)p->info.typ->ref)));
		      }
		      else {
				fprintf(fout, "\n\t\tsoap->position = 1;\n\t\tsoap->positions[0] = i;");
				if(is_XML((Tnode *)p->info.typ->ref) && is_string((Tnode *)p->info.typ->ref))
				    fprintf(fout, "\n\t\tsoap_outliteral(soap, \"%s\", &a->%s[i], NULL);", item, ident(p->sym->name));
			    else if(is_XML((Tnode *)p->info.typ->ref) && is_wstring((Tnode *)p->info.typ->ref))
				    fprintf(fout, "\n\t\tsoap_outwliteral(soap, \"%s\", &a->%s[i], NULL);", item, ident(p->sym->name));
			    else if(((Tnode *)p->info.typ->ref)->type == Tclass && !is_external((Tnode *)p->info.typ->ref) &&
			            !is_volatile((Tnode *)p->info.typ->ref) && !is_typedef((Tnode *)p->info.typ->ref))
				    fprintf(fout, "\n\t\ta->%s[i].soap_out(soap, \"%s\", -1, \"%s\");",
					    ident(p->sym->name), item, xsi_type_u(((Tnode *)p->info.typ->ref)));
			    else
				    fprintf(fout, "\n\t\tsoap_out_%s(soap, \"%s\", -1, &a->%s[i], \"%s\");",
					    c_ident(((Tnode *)p->info.typ->ref)), item, ident(p->sym->name),
					    xsi_type_u(((Tnode *)p->info.typ->ref))); }
	      }
	      else {
			if(is_XML((Tnode *)p->info.typ->ref) && is_string((Tnode *)p->info.typ->ref))
			    fprintf(fout, "\n\t\tsoap_outliteral(soap, \"%s\", &a->%s[i], NULL);", item, ident(p->sym->name));
		    else if(is_XML((Tnode *)p->info.typ->ref) && is_wstring((Tnode *)p->info.typ->ref))
			    fprintf(fout, "\n\t\tsoap_outwliteral(soap, \"%s\", &a->%s[i], NULL);", item, ident(p->sym->name));
		    else if(((Tnode *)p->info.typ->ref)->type == Tclass && !is_external((Tnode *)p->info.typ->ref) &&
		            !is_volatile((Tnode *)p->info.typ->ref) && !is_typedef((Tnode *)p->info.typ->ref))
			    fprintf(fout, "\n\t\ta->%s[i].soap_out(soap, \"%s\", -1, \"%s\");", ident(
					    p->sym->name), item, xsi_type_u(((Tnode *)p->info.typ->ref)));
		    else
			    fprintf(fout, "\n\t\tsoap_out_%s(soap, \"%s\", -1, &a->%s[i], \"%s\");",
				    c_ident(((Tnode *)p->info.typ->ref)), item, ident(p->sym->name),
				    xsi_type_u(((Tnode *)p->info.typ->ref))); 
		  }
	}
	if(is_binary(typ))
		fprintf(fout, "\n\treturn soap_element_end_out(soap, tag);\n}");
	else if(!has_ns(typ) && !is_untyped(typ))
		fprintf(fout, "\n\t}\n\tsoap->position = 0;\n\treturn soap_element_end_out(soap, tag);\n}");
	else
		fprintf(fout, "\n\t}\n\treturn soap_element_end_out(soap, tag);\n}");
}

void soap_get(Tnode * typ)
{
	Tnode * temp;
	if(typ->type == Ttemplate || typ->type == Tunion)
		return;
	if(typ->type != Treference) {
		if(!is_external(typ) && namespaceid)
			fprintf(fhead,
				"\n#ifndef soap_read_%s\n\t#define soap_read_%s(soap, data) ( soap_begin_recv(soap) || !%s::soap_get_%s(soap, data, NULL, NULL) || soap_end_recv(soap) )\n#endif\n",
				c_ident(typ), c_ident(typ), namespaceid, c_ident(typ));
		else
			fprintf(fhead,
				"\n#ifndef soap_read_%s\n\t#define soap_read_%s(soap, data) ( soap_begin_recv(soap) || !soap_get_%s(soap, data, NULL, NULL) || soap_end_recv(soap) )\n#endif\n",
				c_ident(typ), c_ident(typ), c_ident(typ));
	}
	if(is_typedef(typ) && is_element(typ)) {
		fprintf(fhead, "\n\n#define soap_get_%s soap_get_%s\n", c_ident(typ), t_ident(typ));
		return;
	}
	if(typ->type == Tarray) {
		/* ARRAY */
		temp = typ;
		while(temp->type == Tarray) {
			temp = (Tnode *)temp->ref;
		}
		fprintf(fhead, "\nSOAP_FMAC3 %s * SOAP_FMAC4 soap_get_%s(struct soap*, %s, const char*, const char*);", c_type(temp), c_ident(typ), c_type(typ));
		fprintf(fout, "\n\nSOAP_FMAC3 %s * SOAP_FMAC4 soap_get_%s(struct soap *soap, %s, const char *tag, const char *type)", c_type(temp), c_ident(typ), c_type_id(typ, "a"));
		fprintf(fout, "\n{\n\t%s;", c_type_id(temp, "(*p)"));
		fprintf(fout, "\n\tif((p = soap_in_%s(soap, tag, a, type)))", c_ident(typ));
	}
	else if(typ->type==Tclass && !is_external(typ) && !is_volatile(typ) && !is_typedef(typ)) {
		/* CLASS  */
		fprintf(fout, "\n\nvoid * %s::soap_get(struct soap *soap, const char *tag, const char *type)", c_type(typ));
		fprintf(fout, "\n\t{ return soap_get_%s(soap, this, tag, type); }", c_ident(typ));
		fprintf(fhead, "\nSOAP_FMAC3 %s SOAP_FMAC4 soap_get_%s(struct soap*, %s, const char*, const char*);", c_type_id(typ, "*"), c_ident(typ), c_type_id(typ, "*"));
		fprintf(fout, "\n\nSOAP_FMAC3 %s SOAP_FMAC4 soap_get_%s(struct soap *soap, %s, const char *tag, const char *type)\n{",
			c_type_id(typ, "*"), c_ident(typ), c_type_id(typ, "*p"));
		fprintf(fout, "\n\tif((p = soap_in_%s(soap, tag, p, type)))", c_ident(typ));
	}
	else {
		fprintf(fhead, "\nSOAP_FMAC3 %s SOAP_FMAC4 soap_get_%s(struct soap*, %s, const char*, const char*);",
			c_type_id(typ, "*"), c_ident(typ), c_type_id(typ, "*"));
		fprintf(fout,
			"\n\nSOAP_FMAC3 %s SOAP_FMAC4 soap_get_%s(struct soap *soap, %s, const char *tag, const char *type)\n{",
			c_type_id(typ, "*"), c_ident(typ), c_type_id(typ, "*p"));
		fprintf(fout, "\n\tif((p = soap_in_%s(soap, tag, p, type)))", c_ident(typ));
	}
	fprintf(fout, "\n\t\tif(soap_getindependent(soap))\n\t\t\treturn NULL;");
	fprintf(fout, "\n\treturn p;\n}");
	fflush(fout);
}

void soap_in(Tnode * typ)
{
	Entry * p = NULL;
	Table * table, * t;
	int total, a, f, cardinality, i, j;
	long min, max;
	Tnode * n, * temp;
	char * nse = ns_qualifiedElement(typ);
	char * nsa = ns_qualifiedAttribute(typ);
	if(is_dynamic_array(typ)) {
		soap_in_Darray(typ);
		return;
	}
	if(is_external(typ))
		fprintf(fhead, "\n\nSOAP_FMAC3S int SOAP_FMAC4S soap_s2%s(struct soap*, const char*, %s);", c_ident(typ), c_type_id(typ, "*"));
	if(is_typedef(typ) && is_element(typ)) {
		fprintf(fhead, "\n\n#define soap_in_%s soap_in_%s\n", c_ident(typ), t_ident(typ));
		return;
	}
	if(is_primitive_or_string(typ) && typ->type != Tenum) {
		if(is_stdqname(typ)) {
			fprintf(fhead, "\nSOAP_FMAC3 std::string * FASTCALL soap_in_%s(struct soap*, const char*, std::string*, const char*);",
				c_ident(typ));
			fprintf(fout, "\n\nSOAP_FMAC1 std::string * SOAP_FMAC2 soap_in_%s(struct soap *soap, const char *tag, std::string *s, const char *type)\n{\n\tif(soap_element_begin_in(soap, tag, 1, type))\n\t\treturn NULL;\n\tif(!s)\n\t\ts = soap_new_std__string(soap, -1);\n\tif(soap->null)\n\t\tif(s)\n\t\t\ts->erase();",
				c_ident(typ));
			fprintf(fout, "\n\tif(soap->body && !*soap->href) {\n\t\tchar *t;\n\t\ts = (std::string*)soap_class_id_enter(soap, soap->id, s, %s, sizeof(std::string), soap->type, soap->arrayType);\n\t\tif(s)\n\t\t{\tif(!(t = soap_string_in(soap, 2, %ld, %ld)))\n\t\t\t\treturn NULL;\n\t\t\ts->assign(t);\n\t\t}\n\t}\n\telse\n\t\ts = (std::string*)soap_id_forward(soap, soap->href, soap_class_id_enter(soap, soap->id, s, %s, sizeof(std::string), soap->type, soap->arrayType), 0, %s, 0, sizeof(std::string), 0, soap_copy_%s);\n\tif(soap->body && soap_element_end_in(soap, tag))\n\t\treturn NULL;\n\treturn s;\n}",
				soap_type(typ), minlen(typ), maxlen(typ), soap_type(typ), soap_type(typ), c_ident(typ));
			return;
		}
		if(is_stdstring(typ)) {
			if(is_external(typ)) {
				fprintf(fhead, "\nSOAP_FMAC1 std::string * SOAP_FMAC2 soap_in_%s(struct soap*, const char*, std::string*, const char*);",
					c_ident(typ));
				return;
			}
			fprintf(fhead, "\nSOAP_FMAC3 std::string * FASTCALL soap_in_%s(struct soap*, const char*, std::string*, const char*);",
				c_ident(typ));
			if(is_stdXML(typ))
				fprintf(fout, "\n\nSOAP_FMAC3 std::string * FASTCALL soap_in_%s(struct soap *soap, const char *tag, std::string *s, const char *type)\n{\n\tchar *t;\n\t(void)type; /* appease -Wall -Werror */\n\tif(soap_inliteral(soap, tag, &t)) {\n\t\tif(!s)\n\t\t\ts = soap_new_std__string(soap, -1);\n\t\ts->assign(t);\n\t\treturn s;\n\t}\n\treturn NULL;\n}",
					c_ident(typ));
			else {
				fprintf(fout, "\n\nSOAP_FMAC3 std::string * FASTCALL soap_in_%s(struct soap *soap, const char *tag, std::string *s, const char *type)\n{\n\t(void)type; /* appease -Wall -Werror */\n\tif(soap_element_begin_in(soap, tag, 1, NULL))\n\t\treturn NULL;\n\tif(!s)\n\t\ts = soap_new_std__string(soap, -1);\n\tif(soap->null)\n\t\tif(s)\n\t\t\ts->erase();",
				      c_ident(typ));
				fprintf(fout, "\n\tif(soap->body && !*soap->href) {\n\t\tchar *t;\n\t\ts = (std::string*)soap_class_id_enter(soap, soap->id, s, %s, sizeof(std::string), soap->type, soap->arrayType);\n\t\tif(s)\n\t\t{\tif(!(t = soap_string_in(soap, 1, %ld, %ld)))\n\t\t\t\treturn NULL;\n\t\t\ts->assign(t);\n\t\t}\n\t}\n\telse\n\t\ts = (std::string*)soap_id_forward(soap, soap->href, soap_class_id_enter(soap, soap->id, s, %s, sizeof(std::string), soap->type, soap->arrayType), 0, %s, 0, sizeof(std::string), 0, soap_copy_%s);\n\tif(soap->body && soap_element_end_in(soap, tag))\n\t\treturn NULL;\n\treturn s;\n}",
				      soap_type(typ), minlen(typ), maxlen(typ), soap_type(typ), soap_type(typ), c_ident(typ)); }
			return;
		}
		if(is_stdwstring(typ)) {
			if(is_external(typ))                         {
				fprintf(fhead, "\nSOAP_FMAC3 std::wstring * FASTCALL soap_in_%s(struct soap*, const char*, std::wstring*, const char*);", c_ident(typ));
				return;
			}
			if(is_stdXML(typ))
				fprintf(fout, "\n\nSOAP_FMAC3 std::wstring * FASTCALL soap_in_%s(struct soap *soap, const char *tag, std::wstring *s, const char *type)\n{\n\twchar_t *t;\n\t(void)type; /* appease -Wall -Werror */\n\tif(soap_inwliteral(soap, tag, &t)) {\n\t\tif(!s)\n\t\t\ts = soap_new_std__wstring(soap, -1);\n\t\ts->assign(t);\n\t\treturn s;\n\t}\n\treturn NULL;\n}",
					c_ident(typ));
			else {
				fprintf(fhead, "\nSOAP_FMAC3 std::wstring * FASTCALL soap_in_%s(struct soap*, const char*, std::wstring*, const char*);",
				      c_ident(typ));
			      fprintf(fout, "\n\nSOAP_FMAC3 std::wstring * FASTCALL soap_in_%s(struct soap *soap, const char *tag, std::wstring *s, const char *type)\n{\n\t(void)type; /* appease -Wall -Werror */\n\tif(soap_element_begin_in(soap, tag, 1, NULL))\n\t\treturn NULL;\n\tif(!s)\n\t\ts = soap_new_std__wstring(soap, -1);\n\tif(soap->null)\n\t\tif(s)\n\t\t\ts->erase();",
				      c_ident(typ));
			      fprintf(fout, "\n\tif(soap->body && !*soap->href) {\n\t\twchar_t *t;\n\t\ts = (std::wstring*)soap_class_id_enter(soap, soap->id, s, %s, sizeof(std::wstring), soap->type, soap->arrayType);\n\t\tif(s)\n\t\t{\tif(!(t = soap_wstring_in(soap, 1, %ld, %ld)))\n\t\t\t\treturn NULL;\n\t\t\ts->assign(t);\n\t\t}\n\t}\n\telse\n\t\ts = (std::wstring*)soap_id_forward(soap, soap->href, soap_class_id_enter(soap, soap->id, s, %s, sizeof(std::wstring), soap->type, soap->arrayType), 0, %s, 0, sizeof(std::wstring), 0, soap_copy_%s);\n\tif(soap->body && soap_element_end_in(soap, tag))\n\t\treturn NULL;\n\treturn s;\n}",
				      soap_type(typ), minlen(typ), maxlen(typ), soap_type(typ), soap_type(typ), c_ident(typ)); 
			}
			return;
		}
		if(is_external(typ)) {
			fprintf(fhead, "\nSOAP_FMAC1 %s * SOAP_FMAC2 soap_in_%s(struct soap*, const char*, %s, const char*);",
				c_type(typ), c_ident(typ), c_type_id(typ, "*"));
			return;
		}
		fprintf(fhead, "\nSOAP_FMAC3 %s * FASTCALL soap_in_%s(struct soap*, const char*, %s, const char*);",
			c_type(typ), c_ident(typ), c_type_id(typ, "*"));
		//
		/*
		fprintf(fout, "\n\nSOAP_FMAC3 %s * FASTCALL soap_in_%s(struct soap *soap, const char *tag, %s, const char *type)\n{\n\t%s;",
			c_type(typ), c_ident(typ), c_type_id(typ, "*a"), c_type_id(typ, "*p"));
		if(is_wstring(typ)) {
			fprintf(fout, "\n\tp = soap_inwstring(soap, tag, a, type, %s, %ld, %ld);", 
				soap_type(typ), minlen(typ), maxlen(typ));
		}
		else if(is_string(typ)) {
			fprintf(fout, "\n\tp = soap_instring(soap, tag, a, type, %s, %d, %ld, %ld);", 
				soap_type(typ), is_qname(typ)+1, minlen(typ), maxlen(typ));
		}
		else {
			if(typ->type == Tllong || typ->type == Tullong)
				fprintf(fout, "\n\tp = soap_in%s(soap, tag, a, type, %s);", c_type(typ), soap_type(typ));
			else
				fprintf(fout, "\n\tp = soap_in%s(soap, tag, a, type, %s);", the_type(typ), soap_type(typ));
			if(typ->minLength != MINLONG64 && (typ->minLength > 0 || typ->type < Tuchar || typ->type > Tullong))
				fprintf(fout, "\n\tif(p && *p < " SOAP_LONG_FORMAT ") {\n\t\tsoap->error = SOAP_LENGTH;\n\t\treturn NULL;\n\t}", typ->minLength);
			if(typ->maxLength != MAXLONG64)
				fprintf(fout, "\n\tif(p && *p > " SOAP_LONG_FORMAT ") {\n\t\tsoap->error = SOAP_LENGTH;\n\t\treturn NULL;\n\t}", typ->maxLength); 
		}
		*/
		fprintf(fout, "\n\nSOAP_FMAC3 %s * FASTCALL soap_in_%s(struct soap *soap, const char *tag, %s, const char *type)\n{;",
			c_type(typ), c_ident(typ), c_type_id(typ, "*a"));
		if(is_wstring(typ)) {
			fprintf(fout, "\n\t%s = soap_inwstring(soap, tag, a, type, %s, %ld, %ld);", 
				c_type_id(typ, "* p"), soap_type(typ), minlen(typ), maxlen(typ));
		}
		else if(is_string(typ)) {
			fprintf(fout, "\n\t%s = soap_instring(soap, tag, a, type, %s, %d, %ld, %ld);", 
				c_type_id(typ, "* p"), soap_type(typ), is_qname(typ)+1, minlen(typ), maxlen(typ));
		}
		else {
			if(typ->type == Tllong || typ->type == Tullong)
				fprintf(fout, "\n\t%s = soap_in%s(soap, tag, a, type, %s);", c_type_id(typ, "* p"), c_type(typ), soap_type(typ));
			else
				fprintf(fout, "\n\t%s = soap_in%s(soap, tag, a, type, %s);", c_type_id(typ, "* p"), the_type(typ), soap_type(typ));
			if(typ->minLength != MINLONG64 && (typ->minLength > 0 || typ->type < Tuchar || typ->type > Tullong))
				fprintf(fout, "\n\tif(p && *p < " SOAP_LONG_FORMAT ") {\n\t\tsoap->error = SOAP_LENGTH;\n\t\treturn NULL;\n\t}", typ->minLength);
			if(typ->maxLength != MAXLONG64)
				fprintf(fout, "\n\tif(p && *p > " SOAP_LONG_FORMAT ") {\n\t\tsoap->error = SOAP_LENGTH;\n\t\treturn NULL;\n\t}", typ->maxLength); 
		}
		//
		fprintf(fout, "\n\treturn p;\n}");
		fflush(fout);
		return;
	}
	if(is_fixedstring(typ)) {
		fprintf(fhead, "\nSOAP_FMAC3 char* FASTCALL soap_in_%s(struct soap*, const char*, char[], const char*);", c_ident(typ));
		fprintf(fout, "\n\nSOAP_FMAC3 char* FASTCALL soap_in_%s(struct soap *soap, const char *tag, char a[], const char *type)\n{\n\tchar *p;\n\tif(soap_instring(soap, tag, &p, type, %s, 1, 0, %d))\n\t\treturn strcpy(a, p);\n\treturn NULL;\n}",
			c_ident(typ), soap_type(typ), typ->width/((Tnode *)typ->ref)->width-1);
		return;
	}
	switch(typ->type) {
	    case Tstruct:
		if(is_external(typ)) {
			fprintf(fhead, "\nSOAP_FMAC1 %s SOAP_FMAC2 soap_in_%s(struct soap*, const char*, %s, const char*);",
				c_type_id(typ, "*"), c_ident(typ), c_type_id(typ, "*"));
			return;
		}
		fprintf(fhead, "\nSOAP_FMAC3 %s FASTCALL soap_in_%s(struct soap*, const char*, %s, const char*);",
			c_type_id(typ, "*"), c_ident(typ), c_type_id(typ, "*"));
		fprintf(fout, "\n\nSOAP_FMAC3 %s FASTCALL soap_in_%s(struct soap *soap, const char *tag, %s, const char *type)\n{",
			c_type_id(typ, "*"), c_ident(typ), c_type_id(typ, "*a"));
		table = (Table *)typ->ref;
		if(is_primclass(typ)) {
			fprintf(fout, "\n\t(void)type; /* appease -Wall -Werror */\n\tif(soap_element_begin_in(soap, tag, 1, NULL))\n\t\treturn NULL;");
			if(has_class(typ))
				fprintf(fout, "\n\tif(!(a = (%s)soap_class_id_enter(soap, soap->id, a, %s, sizeof(%s), soap->type, soap->arrayType)))\n\t\treturn NULL;",
					c_type_id(typ, "*"), soap_type(typ), c_type(typ));
			else
				fprintf(fout, "\n\tif(!(a = (%s)soap_id_enter(soap, soap->id, a, %s, sizeof(%s), 0, NULL, NULL, NULL)))\n\t\treturn NULL;",
					c_type_id(typ, "*"), soap_type(typ), c_type(typ));
			fprintf(fout, "\n\tsoap_revert(soap);\n\t*soap->id = '\\0';");
			/* fprintf(fout,"\n\tif(soap->alloced)"); */
			fprintf(fout, "\n\tsoap_default_%s(soap, a);", c_ident(typ));
			for(t = (Table *)typ->ref; t; t = t->prev) {
				for(p = t->list; p; p = p->next)
					if(p->info.sto&Sattribute)
						soap_attr_value(p, "a", ident(p->sym->name), ns_add(p->sym->name, nsa));
			}
			fflush(fout);
			for(table = (Table *)typ->ref; table; table = table->prev) {
				p = table->list;
				if(p && is_item(p))
					break;
			}
			if(is_XML(p->info.typ) && is_string(p->info.typ)) {
				fprintf(fout, "\n\tif(!soap_inliteral(soap, tag, &a->%s))", ident(p->sym->name));
			}
			else if(is_XML(p->info.typ) && is_wstring(p->info.typ)) {
				fprintf(fout, "\n\tif(!soap_inwliteral(soap, tag, &a->%s))", ident(p->sym->name));
			}
			else if(p->info.typ->type==Tarray) {
				fprintf(fout, "\n\tif(!soap_in_%s(soap, tag, a->%s, \"%s\"))", c_ident(
						p->info.typ), ident(p->sym->name), xsi_type(typ));
			}
			else if(p->info.typ->type==Tclass && !is_external(p->info.typ) && !is_volatile(p->info.typ) &&
			        !is_typedef(p->info.typ)) {
				fprintf(fout, "\n\tif(!a->%s.soap_in(soap, tag, \"%s\"))", ident(
						p->sym->name), xsi_type(typ));
			}
			else if(p->info.typ->type != Tfun && !is_void(p->info.typ)) {
				fprintf(fout, "\n\tif(!soap_in_%s(soap, tag, &a->%s, \"%s\"))", c_ident(
						p->info.typ), ident(p->sym->name), xsi_type(typ));
			}
			fprintf(fout, "\n\t\treturn NULL;");
			fprintf(fout, "\n\treturn a;\n}");
		}
		else {table = (Table *)typ->ref;
		      if(!is_discriminant(typ)) {
			      for(t = table; t; t = t->prev) {
				      for(p = t->list; p; p = p->next) {
					      if(!(p->info.sto&(Sconst|Sprivate|Sprotected)) && !(p->info.sto&Sattribute) && p->info.typ->type != Tfun &&
					         !is_void(p->info.typ) && !is_transient(p->info.typ) && !is_template(p->info.typ))
					      {
						      if(is_anytype(p) || is_choice(p))
							      p = p->next;
						      if(is_repetition(p)) {
							      fprintf(fout, "\n\tstruct soap_blist *soap_blist_%s = NULL;", ident(p->next->sym->name));
							      p = p->next;
						      }
						      else
							      fprintf(fout, "\n\tsize_t soap_flag_%s = " SOAP_LONG_FORMAT ";", ident(p->sym->name), p->info.maxOccurs);
					      }
				      }
			      }
		      }
		      if(!is_invisible(typ->id->name)) {
			      fprintf(fout, "\n\tif(soap_element_begin_in(soap, tag, 0, type))\n\t\treturn NULL;");
		      }
		      else if(!is_discriminant(typ)) {
			      if(table->prev || table->list)
				      fprintf(fout, "\n\tshort soap_flag;");
		      }
		      if(has_class(typ)) {
			      if(is_invisible(typ->id->name))
				      fprintf(fout, "\n\ta = (%s)soap_class_id_enter(soap, \"\", a, %s, sizeof(%s), soap->type, soap->arrayType);",
					      c_type_id(typ, "*"), soap_type(typ), c_type(typ));
			      else
				      fprintf(fout, "\n\ta = (%s)soap_class_id_enter(soap, soap->id, a, %s, sizeof(%s), soap->type, soap->arrayType);",
					      c_type_id(typ, "*"), soap_type(typ), c_type(typ));
		      }
		      else if(is_invisible(typ->id->name))
			      fprintf(fout, "\n\ta = (%s)soap_id_enter(soap, \"\", a, %s, sizeof(%s), 0, NULL, NULL, NULL);",
				      c_type_id(typ, "*"), soap_type(typ), c_type(typ));
		      else
			      fprintf(fout, "\n\ta = (%s)soap_id_enter(soap, soap->id, a, %s, sizeof(%s), 0, NULL, NULL, NULL);",
				      c_type_id(typ, "*"), soap_type(typ), c_type(typ));
		      fprintf(fout, "\n\tif(!a)\n\t\treturn NULL;");
			/* fprintf(fout,"\n\tif(soap->alloced)"); */
		      fprintf(fout, "\n\tsoap_default_%s(soap, a);", c_ident(typ));
		      for(t = table; t; t = t->prev) {
			      for(p = t->list; p; p = p->next)
				      if(p->info.sto&Sattribute)
					      soap_attr_value(p, "a", ident(p->sym->name), ns_add(p->sym->name, nsa));
		      }
		      if(!is_invisible(typ->id->name)) {
			      if(!is_discriminant(typ)) {
				      fprintf(fout, "\n\tif(soap->body && !*soap->href) {\n");
				      fprintf(fout, "\t\tfor(;;) {\n\t\t\tsoap->error = SOAP_TAG_MISMATCH;");
			      }
			      else
				      fprintf(fout, "\n\tif(!tag || *tag == '-' || (soap->body && !*soap->href)) {\n\t");
		      }
		      else if(!is_discriminant(typ)) {
			      if(table->prev || table->list)
					  fprintf(fout, "\n\t\tfor(soap_flag = 0;; soap_flag = 1) {\n\t\t\tsoap->error = SOAP_TAG_MISMATCH;");
		      }
		      a = 0;
		      f = 0;
		      for(t = table; t; t = t->prev) {
			      for(p = t->list; p; p = p->next) {
				      if(p->info.sto&(Sconst|Sprivate|Sprotected))
					      fprintf(fout, "\n\t\t/* non-serializable %s skipped */", ident(p->sym->name));
				      else if(is_transient(p->info.typ))
					      fprintf(fout, "\n\t\t/* transient %s skipped */", ident(p->sym->name));
				      else if(p->info.sto&Sattribute)
					      ;
				      else if(is_repetition(p)) {
					      f = 1;
					      fprintf(fout, "\n\t\t\tif(soap->error == SOAP_TAG_MISMATCH && ");
					      if(is_unmatched(p->next->sym))
						      fprintf(fout, "!soap_element_begin_in(soap, NULL, 1, NULL))");
					      else if(is_invisible(p->next->sym->name))
						      fprintf(fout, "!soap_peek_element(soap))");
					      else
						      fprintf(fout, "!soap_element_begin_in(soap, \"%s\", 1, NULL))", ns_add(p->next->sym->name, nse));
					      fprintf(fout,
							  " {\n\t\t\t\tif(a->%s == NULL) {\n\t\t\t\t\tif(soap_blist_%s == NULL)\n\t\t\t\t\t\tsoap_blist_%s = soap_new_block(soap);\n\t\t\t\t\ta->%s = static_cast<%s>(soap_push_block(soap, soap_blist_%s, sizeof(%s)));\n\t\t\t\t\tif(a->%s == NULL)\n\t\t\t\t\t\treturn NULL;",
						      ident(p->next->sym->name), ident(p->next->sym->name),
						      ident(p->next->sym->name), ident(p->next->sym->name),
						      c_type(p->next->info.typ), ident(p->next->sym->name),
						      c_type((Tnode *)p->next->info.typ->ref), ident(p->next->sym->name));
					      if(((Tnode *)p->next->info.typ->ref)->type == Tclass || has_class((Tnode *)p->next->info.typ->ref))
						      fprintf(fout, "\n\t\t\t\t\tSOAP_PLACEMENT_NEW(a->%s, %s);",
							      ident(p->next->sym->name), c_type((Tnode *)p->next->info.typ->ref));
					      if(((Tnode *)p->next->info.typ->ref)->type == Tclass &&
					         !is_external((Tnode *)p->next->info.typ->ref) &&
					         !is_volatile((Tnode *)p->next->info.typ->ref) &&
					         !is_typedef((Tnode *)p->next->info.typ->ref))
						      fprintf(fout, "\n\t\t\t\t\ta->%s->soap_default(soap);", ident(p->next->sym->name));
					      else if(((Tnode *)p->next->info.typ->ref)->type != Tpointer  && !is_XML((Tnode *)p->next->info.typ->ref))
						      fprintf(fout, "\n\t\t\t\t\tsoap_default_%s(soap, a->%s);", c_ident((Tnode *)p->next->info.typ->ref), ident(p->next->sym->name));
					      else
						      fprintf(fout, "\n\t\t\t\t\t*a->%s = NULL;", ident(p->next->sym->name));
					      fprintf(fout, "\n\t\t\t\t}");
					      if(!is_invisible(p->next->sym->name))
						      fprintf(fout, "\n\t\t\t\tsoap_revert(soap);");
					      if(is_unmatched(p->next->sym)) {
						      if(is_XML((Tnode *)p->next->info.typ->ref) && is_string((Tnode *)p->next->info.typ->ref))
							      fprintf(fout, "\n\t\t\t\tif(soap_inliteral(soap, NULL, a->%s))", ident(p->next->sym->name));
						      else if(is_XML((Tnode *)p->next->info.typ->ref) && is_wstring((Tnode *)p->next->info.typ->ref))
							      fprintf(fout, "\n\t\t\t\tif(soap_inwliteral(soap, NULL, a->%s))",
								      ident(p->next->sym->name));
						      else
							      fprintf(fout, "\n\t\t\t\tif(soap_in_%s(soap, NULL, a->%s, \"%s\"))",
								      c_ident((Tnode *)p->next->info.typ->ref), ident(p->next->sym->name), xsi_type((Tnode *)p->next->info.typ->ref));
					      }
					      else {
							  if(is_XML((Tnode *)p->next->info.typ->ref) && is_string((Tnode *)p->next->info.typ->ref))
							    fprintf(fout, "\n\t\t\t\tif(soap_inliteral(soap, \"%s\", a->%s))",
								    ns_add(p->next->sym->name, nse), ident(p->next->sym->name));
						    else if(is_XML((Tnode *)p->next->info.typ->ref) &&
						            is_wstring((Tnode *)p->next->info.typ->ref))
							    fprintf(fout, "\n\t\t\t\tif(soap_inwliteral(soap, \"%s\", a->%s))",
								    ns_add(p->next->sym->name, nse), ident(p->next->sym->name));
						    else
							    fprintf(fout, "\n\t\t\t\tif(soap_in_%s(soap, \"%s\", a->%s, \"%s\"))",
								    c_ident((Tnode *)p->next->info.typ->ref), ns_add(p->next->sym->name, nse), ident(p->next->sym->name),
								    xsi_type((Tnode *)p->next->info.typ->ref)); 
						  }
						  fprintf(fout, " {\n\t\t\t\t\ta->%s++;\n\t\t\t\t\ta->%s = NULL;\n\t\t\t\t\tcontinue;\n\t\t\t\t}\n\t\t\t}",
						      ident(p->sym->name), ident(p->next->sym->name));
					      /* THIS CODE IS RETAINED HERE FOR REFERENCE PURPUSES
					         IT ILLUSTRATES A DIFFERENT APPROACH THAT IS NOW OBSOLETE */
#if 0
					      fprintf(fout, "\n\t\t\tif(soap_flag_%s && soap->error == SOAP_TAG_MISMATCH)", ident(p->next->sym->name));
					      if(((Tnode *)p->next->info.typ->ref)->type == Tclass && !is_volatile(p->next->info.typ->ref))
						      fprintf(fout,
							      "\n\t\t\t{\t%s;\n\t\t\t\t%s;\n\t\t\t\tq.soap_default(soap);\n\t\t\t\tif(soap_new_block(soap) == NULL)\n\t\t\t\t\treturn NULL;",
							      c_type_id(p->next->info.typ, "p"),
							      c_type_id((Tnode *)p->next->info.typ->ref, "q"));
					      else if(((Tnode *)p->next->info.typ->ref)->type == Tclass || has_class(p->next->info.typ->ref))
						      fprintf(fout, "\n\t\t\t{\t%s;\n\t\t\t\t%s;\n\t\t\t\tif(soap_new_block(soap) == NULL)\n\t\t\t\t\treturn NULL;",
							      c_type_id(p->next->info.typ, "p"), c_type_id((Tnode *)p->next->info.typ->ref, "q"));
					      else
						      fprintf(fout, "\n\t\t\t{\t%s;\n\t\t\t\tif(soap_new_block(soap) == NULL)\n\t\t\t\t\treturn NULL;",
							      c_type_id(p->next->info.typ, "p"));
					      if(is_unmatched(p->next->sym))
						      fprintf(fout, "\n\t\t\t\tfor(a->%s = 0; !soap_element_begin_in(soap, NULL, 1, NULL); a->%s++)",
							      ident(p->sym->name), ident(p->sym->name));
					      else if(is_invisible(p->next->sym->name))
						      fprintf(fout, "\n\t\t\t\tfor(a->%s = 0; !soap_peek_element(soap); a->%s++)",
							      ident(p->sym->name), ident(p->sym->name));
					      else
						      fprintf(fout, "\n\t\t\t\tfor(a->%s = 0; !soap_element_begin_in(soap, \"%s\", 1, NULL); a->%s++)",
							      ident(p->sym->name), ns_add(p->next->sym->name, nse), ident(p->sym->name));
					      fprintf(fout, "\n\t\t\t\t{\tp = static_cast<%s>(soap_push_block(soap, NULL, sizeof(%s)));\n\t\t\t\tif(!p)\n\t\t\t\t\treturn NULL;",
						      c_type(p->next->info.typ), c_type((Tnode *)p->next->info.typ->ref));
					      if(((Tnode *)p->next->info.typ->ref)->type == Tclass || has_class(p->next->info.typ->ref))
						      fprintf(fout, "\n\t\t\t\t\tmemcpy(p, &q, sizeof(%s));", c_type((Tnode *)p->next->info.typ->ref));
					      if(((Tnode *)p->next->info.typ->ref)->type == Tclass && !is_external(p->next->info.typ->ref) &&
					         !is_volatile(p->next->info.typ->ref) && !is_typedef(p->next->info.typ->ref))
						      fprintf(fout, "\n\t\t\t\t\tp->soap_default(soap);");
					      else if(((Tnode *)p->next->info.typ->ref)->type != Tpointer  && !is_XML(p->next->info.typ->ref))
						      fprintf(fout, "\n\t\t\t\t\tsoap_default_%s(soap, p);", c_ident(p->next->info.typ->ref));
					      else
						      fprintf(fout, "\n\t\t\t\t\t*p = NULL;");
					      if(!is_invisible(p->next->sym->name))
						      fprintf(fout, "\n\t\t\t\t\tsoap_revert(soap);");
					      if(is_unmatched(p->next->sym)) {
						      if(is_XML(p->next->info.typ->ref) &&
						         is_string(p->next->info.typ->ref))
							      fprintf(fout,
								      "\n\t\t\t\t\tif(!soap_inliteral(soap, NULL, p))");
						      else if(is_XML(p->next->info.typ->ref) &&
						              is_wstring(p->next->info.typ->ref))
							      fprintf(
								      fout,
								      "\n\t\t\t\t\tif(!soap_inwliteral(soap, NULL, p))");
						      else
							      fprintf(
								      fout,
								      "\n\t\t\t\t\tif(!soap_in_%s(soap, NULL, p, \"%s\"))",
								      c_ident(p->next->info.typ->ref),
								      xsi_type(p->next->info.typ->ref));
					      }
					      else {if(is_XML(p->next->info.typ->ref) &&
						       is_string(p->next->info.typ->ref))
							    fprintf(fout,
								    "\n\t\t\t\t\tif(!soap_inliteral(soap, \"%s\", p))",
								    ns_add(p->next->sym->name,
									    nse));
						    else if(is_XML(p->next->info.typ->ref) &&
						            is_wstring(p->next->info.typ->ref))
							    fprintf(
								    fout,
								    "\n\t\t\t\t\tif(!soap_inwliteral(soap, \"%s\", p))",
								    ns_add(p->next->sym->name, nse));
						    else
							    fprintf(
								    fout,
								    "\n\t\t\t\t\tif(!soap_in_%s(soap, \"%s\", p, \"%s\"))",
								    c_ident(p->next->info.typ->ref),
								    ns_add(p->next->sym->name,
									    nse), xsi_type(p->next->info.typ->ref)); }
					      fprintf(fout, "\n\t\t\t\t\t\tbreak;");
					      fprintf(fout, "\n\t\t\t\t\tsoap_flag_%s = 0;", ident(p->next->sym->name));
					      fprintf(fout, "\n\t\t\t\t}");
					      fprintf(fout, "\n\t\t\t\ta->%s = (%s)soap_save_block(soap, 0, 0, 1);", ident(p->next->sym->name), c_type(p->next->info.typ));
					      fprintf(fout, "\n\t\t\t\tif(!soap_flag_%s && soap->error == SOAP_TAG_MISMATCH)\n\t\t\t\t\tcontinue;\n\t\t\t}", ident(p->next->sym->name));
#endif
					      p = p->next;
				      }
				      else if(is_anytype(p)) {
					      f = 1;
					      fprintf(fout, "\n\t\t\tif(soap_flag_%s && soap->error == SOAP_TAG_MISMATCH)", ident(p->next->sym->name));
					      fprintf(fout, "\n\t\t\t\tif((a->%s = soap_getelement(soap, &a->%s)))", ident(p->next->sym->name), ident(p->sym->name));
					      fprintf(fout, "\n\t\t\t\t{\tsoap_flag_%s = 0;", ident(p->next->sym->name));
					      fprintf(fout, "\n\t\t\t\t\tcontinue;");
					      fprintf(fout, "\n\t\t\t\t}");
					      p = p->next;
				      }
				      else if(is_discriminant(typ) && p->next) {
					      f = 1;
					      fprintf(fout, "\n\t\tif(!soap_in_%s(soap, &a->%s, &a->%s))", c_ident(p->next->info.typ), ident(p->sym->name), ident(p->next->sym->name));
					      fprintf(fout, "\n\t\t\treturn NULL;");
					      break;
				      }
				      else if(is_choice(p)) {
					      f = 1;
					      fprintf(fout, "\n\t\t\tif(soap_flag_%s && soap->error == SOAP_TAG_MISMATCH)", ident(p->next->sym->name));
					      fprintf(fout, "\n\t\t\t\tif(soap_in_%s(soap, &a->%s, &a->%s))", c_ident(p->next->info.typ), ident(p->sym->name), ident(p->next->sym->name));
						  fprintf(fout, " {\n\t\t\t\t\tsoap_flag_%s = 0;", ident(p->next->sym->name));
					      fprintf(fout, "\n\t\t\t\t\tcontinue;");
					      fprintf(fout, "\n\t\t\t\t}");
					      p = p->next;
				      }
				      else {f = 1;
					    if(!is_invisible(p->sym->name) && !is_primclass(typ) && p->info.typ->type != Tfun && !is_void(p->info.typ)) {
						    if(is_string(p->info.typ) || is_wstring(p->info.typ) || is_stdstr(p->info.typ))
							    fprintf(fout, "\n\t\t\tif(soap_flag_%s && oneof2(soap->error, SOAP_TAG_MISMATCH, SOAP_NO_TAG))", ident(p->sym->name));
						    else if(is_template(p->info.typ))
							    fprintf(fout, "\n\t\t\tif(soap->error == SOAP_TAG_MISMATCH)");
						    else
							    fprintf(fout, "\n\t\t\tif(soap_flag_%s && soap->error == SOAP_TAG_MISMATCH)", ident(p->sym->name));
					    }
					    if(is_unmatched(p->sym)) {
						    if(is_XML(p->info.typ) && is_string(p->info.typ)) {
							    fprintf(fout, "\n\t\t\t\tif(soap_inliteral(soap, NULL, &a->%s))", ident(p->sym->name));
						    }
						    else if(is_XML(p->info.typ) && is_wstring(p->info.typ)) {
							    fprintf(fout, "\n\t\t\t\tif(soap_inwliteral(soap, NULL, &a->%s))", ident(p->sym->name));
						    }
						    else if(p->info.typ->type==Tarray) {
							    fprintf(fout, "\n\t\t\t\tif(soap_in_%s(soap, NULL, a->%s, \"%s\"))", c_ident(p->info.typ), ident(p->sym->name), xsi_type(p->info.typ));
						    }
						    else if(p->info.typ->type==Tclass && !is_external(p->info.typ) && !is_volatile(p->info.typ) && !is_typedef(p->info.typ)) {
							    fprintf(fout, "\n\t\t\t\tif(a->%s.soap_in(soap, NULL, \"%s\"))", ident(p->sym->name), xsi_type(p->info.typ));
						    }
						    else if(p->info.typ->type != Tfun && !is_void(p->info.typ)) {
							    fprintf(fout, "\n\t\t\t\tif(soap_in_%s(soap, NULL, &a->%s, \"%s\"))", c_ident(p->info.typ), ident(p->sym->name), xsi_type(p->info.typ));
						    }
					    }
					    else if(!is_invisible(p->sym->name)) {
						    if(is_XML(p->info.typ) && is_string(p->info.typ)) {
							    fprintf(fout, "\n\t\t\t\tif(soap_inliteral(soap, \"%s\", &a->%s))", ns_add(p->sym->name, nse), ident(p->sym->name));
						    }
						    else if(is_XML(p->info.typ) && is_wstring(p->info.typ)) {
							    fprintf(fout, "\n\t\t\t\tif(soap_inwliteral(soap, \"%s\", &a->%s))", ns_add(p->sym->name, nse), ident(p->sym->name));
						    }
						    else if(p->info.typ->type==Tarray) {
							    fprintf(fout, "\n\t\t\t\tif(soap_in_%s(soap, \"%s\", a->%s, \"%s\"))", c_ident(p->info.typ),
								    ns_add(p->sym->name, nse), ident(p->sym->name), xsi_type(p->info.typ));
						    }
						    else if(p->info.typ->type==Tclass && !is_external(p->info.typ) && !is_volatile(p->info.typ) && !is_typedef(p->info.typ)) {
							    fprintf(fout, "\n\t\t\t\tif(a->%s.soap_in(soap, \"%s\", \"%s\"))", ident(p->sym->name), ns_add(p->sym->name, nse), xsi_type(p->info.typ));
						    }
						    else if(p->info.typ->type != Tfun && !is_void(p->info.typ)) {
							    fprintf(fout, "\n\t\t\t\tif(soap_in_%s(soap, \"%s\", &a->%s, \"%s\"))",
								    c_ident(p->info.typ), ns_add(p->sym->name, nse), ident(p->sym->name), xsi_type(p->info.typ));
						    }
					    }
					    if(!is_invisible(p->sym->name) && !is_primclass(typ) && p->info.typ->type != Tfun && !is_void(p->info.typ)) {
						    if(is_template(p->info.typ))
							    fprintf(fout, "\n\t\t\t\t\tcontinue;");
						    else {
								fprintf(fout, " {\n\t\t\t\t\tsoap_flag_%s--;", ident(p->sym->name));
								fprintf(fout, "\n\t\t\t\t\tcontinue;");
								fprintf(fout, "\n\t\t\t\t}"); 
							}
					    }
				      }
				      fflush(fout);
			      }
		      }
		      if(!f && is_invisible(typ->id->name))
			      fprintf(fout, "\n\tsoap->error = SOAP_TAG_MISMATCH;\n\ta = NULL;");
		      if(!is_discriminant(typ)) {
			      for(t = table; t; t = t->prev) {
				      for(p = t->list; p; p = p->next) {
					      if(is_repetition(p) || is_anytype(p) || is_choice(p)) {
						      p = p->next;
						      continue;
					      }
					      if(is_invisible(p->sym->name) && !(p->info.sto&(Sconst|Sprivate|Sprotected)) && !is_transient(p->info.typ) && !(p->info.sto&Sattribute)) {
						      if(is_string(p->info.typ) || is_wstring(p->info.typ) || is_stdstr(p->info.typ))
							      fprintf(fout, "\n\t\t\tif(soap_flag_%s && oneof2(soap->error, SOAP_TAG_MISMATCH, SOAP_NO_TAG))", ident(p->sym->name));
						      else if(is_template(p->info.typ))
							      fprintf(fout, "\n\t\t\tif(soap->error == SOAP_TAG_MISMATCH)");
						      else
							      fprintf(fout, "\n\t\t\tif(soap_flag_%s && soap->error == SOAP_TAG_MISMATCH)", ident(p->sym->name));
						      if(is_XML(p->info.typ) && is_string(p->info.typ))
							      fprintf(fout, "\n\t\t\t\tif(soap_inliteral(soap, \"%s\", &a->%s))", ns_add(p->sym->name, nse), ident(p->sym->name));
						      else if(is_XML(p->info.typ) && is_wstring(p->info.typ))
							      fprintf(fout, "\n\t\t\t\tif(soap_inwliteral(soap, \"%s\", &a->%s))", ns_add(p->sym->name, nse), ident(p->sym->name));
						      else if(p->info.typ->type==Tarray)
							      fprintf(fout, "\n\t\t\t\tif(soap_in_%s(soap, \"%s\", a->%s, \"%s\"))",
								      c_ident(p->info.typ), ns_add(p->sym->name, nse), ident(p->sym->name), xsi_type(p->info.typ));
						      else if(p->info.typ->type==Tclass && !is_external(p->info.typ) && !is_volatile(p->info.typ) && !is_typedef(p->info.typ))
							      fprintf(fout, "\n\t\t\t\tif(a->%s.soap_in(soap, \"%s\", \"%s\"))",
								      ident(p->sym->name), ns_add(p->sym->name, nse), xsi_type(p->info.typ));
						      else if(p->info.typ->type != Tfun && !is_void(p->info.typ))
							      fprintf(fout, "\n\t\t\t\tif(soap_in_%s(soap, \"%s\", &a->%s, \"%s\"))",
								      c_ident(p->info.typ), ns_add(p->sym->name, nse), ident(p->sym->name), xsi_type(p->info.typ));
						      if(is_template(p->info.typ))
							      fprintf(fout, "\n\t\t\t\t\tcontinue;");
						      else {
								  fprintf(fout, "\n\t\t\t\t{\tsoap_flag_%s--;", ident(p->sym->name));
							    fprintf(fout, "\n\t\t\t\t\tcontinue;");
							    fprintf(fout, "\n\t\t\t\t}"); 
							  }
					      }
				      }
			      }
			      for(t = table; t; t = t->prev)
				      for(p = t->list; p; p = p->next)
					      if(p->info.sto&Sreturn)
						      if(nse || has_ns_eq(NULL, p->sym->name))
							      fprintf(fout, "\n\t\t\tsoap_check_result(soap, \"%s\");", ns_add(p->sym->name, nse));
			      if(!is_invisible(typ->id->name) || table->prev || table->list) {
				      fprintf(fout, "\n\t\t\tif(soap->error == SOAP_TAG_MISMATCH)");
				      if(!is_invisible(typ->id->name) || is_discriminant(typ))
					      fprintf(fout, "\n\t\t\t\tsoap->error = soap_ignore_element(soap);");
				      else
						  fprintf(fout, "\n\t\t\t\tif(soap_flag) {\n\t\t\t\t\tsoap->error = SOAP_OK;\n\t\t\t\t\tbreak;\n\t\t\t\t}");
				      if(!is_invisible(typ->id->name))
					      fprintf(fout, "\n\t\t\tif(soap->error == SOAP_NO_TAG)");
				      else
					      fprintf(fout, "\n\t\t\tif(soap_flag && soap->error == SOAP_NO_TAG)");
				      fprintf(fout, "\n\t\t\t\tbreak;");
				      fprintf(fout, "\n\t\t\tif(soap->error)\n\t\t\t\treturn NULL;");
				      fprintf(fout, "\n\t\t}");
			      }
		      }
		      if(table && !is_discriminant(typ)) {
			      for(p = table->list; p; p = p->next)
				      if(is_repetition(p)) {
					      fprintf(fout, "\n\t\tif(a->%s)\n\t\t\tsoap_pop_block(soap, soap_blist_%s);",
						      ident(p->next->sym->name), ident(p->next->sym->name));
					      fprintf(fout, "\n\t\tif(a->%s)\n\t\t\ta->%s = (%s)soap_save_block(soap, soap_blist_%s, NULL, 1);\n\t\telse\n\t\t{\ta->%s = NULL;\n\t\t\tif(soap_blist_%s)\n\t\t\t\tsoap_end_block(soap, soap_blist_%s);\n\t\t}",
						      ident(p->sym->name), ident(p->next->sym->name), c_type(p->next->info.typ), ident(p->next->sym->name), ident(p->next->sym->name), ident(p->next->sym->name),
						      ident(p->next->sym->name));
					      p = p->next;
				      }
		      }
		      if(!is_invisible(typ->id->name)) {
			      fprintf(fout, "\n\t\tif(soap_element_end_in(soap, tag))\n\t\t\treturn NULL;");
			      fprintf(fout, "\n\t}\n\telse {\n\t\t");
				  if(has_class(typ)) {
					  /*
				      fprintf(fout, "a = (%s)soap_id_forward(soap, soap->href, (void*)a, 0, %s, 0, sizeof(%s), 0, soap_copy_%s);",
					      c_type_id(typ, "*"), soap_type(typ), c_type(typ), c_ident(typ));
					  */
				      fprintf(fout, "a = static_cast<%s>(soap_id_forward(soap, soap->href, a, 0, %s, 0, sizeof(%s), 0, soap_copy_%s));",
					      c_type_id(typ, "*"), soap_type(typ), c_type(typ), c_ident(typ));
				  }
				  else {
					  /*
				      fprintf(fout, "a = (%s)soap_id_forward(soap, soap->href, (void*)a, 0, %s, 0, sizeof(%s), 0, NULL);",
					      c_type_id(typ, "*"), soap_type(typ), c_type(typ));
					  */
				      fprintf(fout, "a = static_cast<%s>(soap_id_forward(soap, soap->href, a, 0, %s, 0, sizeof(%s), 0, NULL));",
					      c_type_id(typ, "*"), soap_type(typ), c_type(typ));
				  }
			      fprintf(fout, "\n\t\tif(soap->body && soap_element_end_in(soap, tag))\n\t\t\treturn NULL;");
			      fprintf(fout, "\n\t}");
		      }
		      a = 0;
		      if(table && !is_discriminant(typ)) {
			      for(p = table->list; p; p = p->next) {
				      if(p->info.minOccurs > 0 && p->info.maxOccurs >= 0 && !(p->info.sto&(Sconst|Sprivate|Sprotected)) && !(p->info.sto&Sattribute) &&
				         p->info.typ->type != Tfun && !is_void(p->info.typ) && !is_transient(p->info.typ) && !is_template(p->info.typ) &&
				         !is_repetition(p) && !is_choice(p) && p->info.hasval == False) {
					      if(is_item(p))
						      continue;
					      if(is_anytype(p))
						      p = p->next;
					      if(a==0) {
						      fprintf(fout, "\n\tif(%s(soap_flag_%s > " SOAP_LONG_FORMAT "", strict_check(), ident(p->sym->name), p->info.maxOccurs-p->info.minOccurs);
						      a = 1;
					      }
					      else
						      fprintf(fout, " || soap_flag_%s > " SOAP_LONG_FORMAT "", ident(p->sym->name), p->info.maxOccurs-p->info.minOccurs);
				      }
				      else if(is_template(p->info.typ)) {
					      if(p->info.minOccurs > 0) {
						      if(p->info.typ->type == Tpointer) {
							      if(a == 0) {
								      fprintf(fout, "\n\tif(%s(!a->%s || a->%s->size() < " SOAP_LONG_FORMAT "", strict_check(), ident(p->sym->name),
									      ident(p->sym->name), p->info.minOccurs);
								      a = 1;
							      }
							      else
								      fprintf(fout, " || !a->%s || a->%s->size() < " SOAP_LONG_FORMAT "", ident(p->sym->name), ident(p->sym->name), p->info.minOccurs);
						      }
						      else {
								  if(a==0) {
								    fprintf(fout, "\n\tif(%s(a->%s.size() < " SOAP_LONG_FORMAT "",
									    strict_check(), ident(p->sym->name), p->info.minOccurs);
								    a = 1;
							    }
							    else
								    fprintf(fout, " || a->%s.size() < " SOAP_LONG_FORMAT "", ident(p->sym->name), p->info.minOccurs); }
					      }
					      if(p->info.maxOccurs > 1) {
						      if(p->info.typ->type == Tpointer) {
							      if(a== 0) {
								      fprintf(fout, "\n\tif(%s((a->%s && a->%s->size() > " SOAP_LONG_FORMAT ")", strict_check(), ident(p->sym->name), ident(p->sym->name), p->info.maxOccurs);
								      a = 1;
							      }
							      else
								      fprintf(fout, " || (a->%s && a->%s->size() > " SOAP_LONG_FORMAT ")", ident(p->sym->name), ident(p->sym->name), p->info.maxOccurs);
						      }
						      else {if(a==0)      {
								    fprintf(fout, "\n\tif(%s(a->%s.size() > " SOAP_LONG_FORMAT "", strict_check(), ident(p->sym->name), p->info.maxOccurs);
								    a = 1;
							    }
							    else
								    fprintf(fout, " || a->%s.size() > " SOAP_LONG_FORMAT "", ident(p->sym->name), p->info.maxOccurs); }
					      }
				      }
				      else if(is_repetition(p)) {
					      if(p->info.minOccurs > 0)                            {
						      if(a==0)                                                       {
							      fprintf(fout, "\n\tif(%s(a->%s < " SOAP_LONG_FORMAT "", strict_check(), ident(p->sym->name), p->info.minOccurs);
							      a = 1;
						      }
						      else
							      fprintf(fout, " || a->%s < " SOAP_LONG_FORMAT "", ident(p->sym->name), p->info.minOccurs);
					      }
					      if(p->info.maxOccurs > 1) {
						      if(a==0) {
							      fprintf(fout, "\n\tif(%s(a->%s > " SOAP_LONG_FORMAT "",
								      strict_check(), ident(p->sym->name), p->info.maxOccurs);
							      a = 1;
						      }
						      else
							      fprintf(fout, " || a->%s > " SOAP_LONG_FORMAT "",
								      ident(p->sym->name), p->info.maxOccurs);
					      }
					      p = p->next;
				      }
				      else if(is_choice(p)) {
					      if(p->info.minOccurs != 0) {
						      if(a==0) {
							      fprintf(fout, "\n\tif(%s(soap_flag_%s", strict_check(), ident(p->next->sym->name));
							      a = 1;
						      }
						      else
							      fprintf(fout, " || soap_flag_%s", ident(p->next->sym->name));
					      }
					      p = p->next;
				      }
			      }
			      if(a)
				      fprintf(fout, "))\n\t{\tsoap->error = SOAP_OCCURS;\n\t\treturn NULL;\n\t}");
		      }
		      fprintf(fout, "\n\treturn a;\n}"); 
		}
		break;

	    case Tclass:
		if(is_external(typ)) {
			fprintf(fhead, "\nSOAP_FMAC1 %s SOAP_FMAC2 soap_in_%s(struct soap*, const char*, %s, const char*);",
				c_type_id(typ, "*"), c_ident(typ), c_type_id(typ, "*"));
			return;
		}
		fprintf(fhead, "\nSOAP_FMAC3 %s FASTCALL soap_in_%s(struct soap*, const char*, %s, const char*);",
			c_type_id(typ, "*"), c_ident(typ), c_type_id(typ, "*"));
		if(!is_volatile(typ) && !is_typedef(typ)) {
			fprintf(fout, "\n\nvoid *%s::soap_in(struct soap *soap, const char *tag, const char *type)", c_type(typ));
			fprintf(fout, "\n\t{ return soap_in_%s(soap, tag, this, type); }", c_ident(typ));
			fflush(fout);
		}
		fprintf(fout, "\n\nSOAP_FMAC3 %s FASTCALL soap_in_%s(struct soap *soap, const char *tag, %s, const char *type)\n{",
			c_type_id(typ, "*"), c_ident(typ), c_type_id(typ, "*a"));
		if(is_primclass(typ)) {
			fprintf(fout, "\n\t(void)type; /* appease -Wall -Werror */\n\tif(soap_element_begin_in(soap, tag, 1, NULL))\n\t\treturn NULL;");
			fprintf(fout, "\n\tif(!(a = (%s)soap_class_id_enter(soap, soap->id, a, %s, sizeof(%s), soap->type, soap->arrayType))) {\n\t\tsoap->error = SOAP_TAG_MISMATCH;\n\t\treturn NULL;\n\t}",
				c_type_id(typ, "*"), soap_type(typ), c_type(typ));
			fprintf(fout, "\n\tsoap_revert(soap);\n\t*soap->id = '\\0';");
			fprintf(fout, "\n\tif(soap->alloced) {");
			fprintf(fout, "\n\t\ta->soap_default(soap);");
			fprintf(fout, "\n\t\tif(soap->clist->type != %s)", soap_type(typ));
			fprintf(fout, "\n\t\t\treturn static_cast<%s>(a->soap_in(soap, tag, type));", c_type_id(typ, "*"));
			fprintf(fout, "\n\t}");
			for(t = (Table *)typ->ref; t; t = t->prev) {
				Entry * e = entry(classtable, t->sym);
				char * nsa1 = e ? ns_qualifiedAttribute(e->info.typ) : nsa;
				for(p = t->list; p; p = p->next)
					if(p->info.sto&Sattribute)
						soap_attr_value(p, ptr_cast(t, "a"), ident(p->sym->name), ns_add(p->sym->name, nsa1));
			}
			fflush(fout);
			for(table = (Table *)typ->ref; table; table = table->prev) {
				p = table->list;
				if(p && is_item(p))
					break;
			}
			if(is_XML(p->info.typ) && is_string(p->info.typ)) {
				fprintf(fout, "\n\tif(!soap_inliteral(soap, tag, &(a->%s::%s)))", ident(table->sym->name), ident(p->sym->name));
			}
			else if(is_XML(p->info.typ) && is_wstring(p->info.typ)) {
				fprintf(fout, "\n\tif(!soap_inwliteral(soap, tag, &(a->%s::%s)))", ident(table->sym->name), ident(p->sym->name));
			}
			else if(p->info.typ->type==Tarray) {
				fprintf(fout, "\n\tif(!soap_in_%s(soap, tag, a->%s::%s, \"%s\"))", c_ident(p->info.typ), ident(table->sym->name), ident(p->sym->name),
					xsi_type(typ));
			}
			else if(p->info.typ->type==Tclass && !is_external(p->info.typ) && !is_volatile(p->info.typ) && !is_typedef(p->info.typ)) {
				fprintf(fout, "\n\tif(!(a->%s::%s).soap_in(soap, tag, \"%s\"))", ident(table->sym->name), ident(p->sym->name), xsi_type(typ));
			}
			else if(p->info.typ->type != Tfun && !is_void(p->info.typ)) {
				fprintf(fout, "\n\tif(!soap_in_%s(soap, tag, &(a->%s::%s), \"%s\"))",
					c_ident(p->info.typ), ident(table->sym->name), ident(p->sym->name),
					xsi_type(typ));
			}
			fprintf(fout, "\n\t\treturn NULL;");
			if(has_getter(typ))
				fprintf(fout, "\n\tif(a->get(soap))\n\t\treturn NULL;");
			fprintf(fout, "\n\treturn a;\n}");
		}
		else {
			if(!is_invisible(typ->id->name)) {
				fprintf(fout, "\n\t(void)type; /* appease -Wall -Werror */\n\tif(soap_element_begin_in(soap, tag, 0, NULL))\n\t\treturn NULL;");
				fprintf(fout, "\n\ta = (%s)soap_class_id_enter(soap, soap->id, a, %s, sizeof(%s), soap->type, soap->arrayType);",
					c_type_id(typ, "*"), soap_type(typ), c_type(typ));
			}
			else
				fprintf(fout, "\n\ta = (%s)soap_class_id_enter(soap, \"\", a, %s, sizeof(%s), soap->type, soap->arrayType);",
					c_type_id(typ, "*"), soap_type(typ), c_type(typ));
			fprintf(fout, "\n\tif(!a)\n\t\treturn NULL;");
			if(!is_discriminant(typ)) {
				fprintf(fout, "\n\tif(soap->alloced)");
				if(is_volatile(typ) || is_typedef(typ))
					fprintf(fout, " {\n\t\tsoap_default_%s(soap, a);", c_ident(typ));
				else
					fprintf(fout, " {\n\t\ta->soap_default(soap);");
				if(!is_invisible(typ->id->name)) {
					fprintf(fout, "\n\t\tif(soap->clist->type != %s) {", soap_type(typ));
					fprintf(fout, "\n\t\t\tsoap_revert(soap);");
					fprintf(fout, "\n\t\t\t*soap->id = '\\0';");
					if(is_volatile(typ) || is_typedef(typ))
						fprintf(fout, "\n\t\t\treturn soap_in_%s(soap, tag, a, type);", c_ident(typ));
					else
						fprintf(fout, "\n\t\t\treturn static_cast<%s>(a->soap_in(soap, tag, type));", c_type_id(typ, "*"));
					fprintf(fout, "\n\t\t}");
				}
				fprintf(fout, "\n\t}");
			}
			else
				fprintf(fout, "\n\ta->soap_default(soap);");
			table = (Table *)typ->ref;
			for(t = table; t; t = t->prev) {
				Entry * e = entry(classtable, t->sym);
				char * nsa1 = e ? ns_qualifiedAttribute(e->info.typ) : nsa;
				for(p = t->list; p; p = p->next)
					if(p->info.sto&Sattribute)
						soap_attr_value(p, ptr_cast(t, "a"), ident(p->sym->name), ns_add(p->sym->name, nsa1));
			}
			fflush(fout);

			i = 0;
			if(!is_discriminant(typ)) {
				for(t = table; t; t = t->prev)
					i++;
				a = 0;
				for(; i > 0; i--) {
					t = table;
					for(j = 0; j < i-1; j++)
						t = t->prev;
					for(p = t->list; p; p = p->next) {
						if(!(p->info.sto&(Sconst|Sprivate|Sprotected)) && !(p->info.sto&Sattribute) && p->info.typ->type != Tfun &&
						   !is_void(p->info.typ) && !is_transient(p->info.typ) && !is_template(p->info.typ)) {
							if(is_anytype(p) || is_choice(p))
								p = p->next;
							if(is_repetition(p)) {
								fprintf(fout, "\n\tstruct soap_blist *soap_blist_%s%d = NULL;", ident(p->next->sym->name), i);
								p = p->next;
							}
							else
								fprintf(fout, "\n\tsize_t soap_flag_%s%d = " SOAP_LONG_FORMAT ";", ident(p->sym->name), i, p->info.maxOccurs);
						}
					}
				}
				if(a)
					fprintf(fout, ";");
			}
			fflush(fout);
			if(!is_invisible(typ->id->name)) {
				if(!is_discriminant(typ)) {
					fprintf(fout, "\n\tif(soap->body && !*soap->href) {\n");
					fprintf(fout, "\t\tfor(;;) {\n\t\t\tsoap->error = SOAP_TAG_MISMATCH;");
				}
				else
					fprintf(fout, "\n\tif(!tag || *tag == '-' || (soap->body && !*soap->href)) {\n\t");
			}
			else if(!is_discriminant(typ)) {
				if(table->prev || table->list)
					fprintf(fout, "\n\t\tfor(short soap_flag = 0;; soap_flag = 1)\n\t\t{\tsoap->error = SOAP_TAG_MISMATCH;");
			}
			table = (Table *)typ->ref;
			a = 0;
			i = 0;
			f = 0;
			for(t = table; t; t = t->prev)
				i++;
			for(; i > 0; i--) {
				Entry * e;
				char * nse1;
				t = table;
				for(j = 0; j < i-1; j++)
					t = t->prev;
				e = entry(classtable, t->sym);
				nse1 = e ? ns_qualifiedElement(e->info.typ) : nse;
				for(p = t->list; p; p = p->next) {
					if(is_item(p))
						;
					else if(p->info.sto&(Sconst|Sprivate|Sprotected))
						fprintf(fout, "\n\t\t\t/* non-serializable %s skipped */", ident(p->sym->name));
					else if(is_transient(p->info.typ))
						fprintf(fout, "\n\t\t\t/* transient %s skipped */", ident(p->sym->name));
					else if(p->info.sto&Sattribute)
						;
					else if(is_repetition(p)) {
						f = 1;
						fprintf(fout, "\n\t\t\tif(soap->error == SOAP_TAG_MISMATCH && ");
						if(is_unmatched(p->next->sym))
							fprintf(fout, "!soap_element_begin_in(soap, NULL, 1, NULL))");
						else if(is_invisible(p->next->sym->name))
							fprintf(fout, "!soap_peek_element(soap))");
						else
							fprintf(fout, "!soap_element_begin_in(soap, \"%s\", 1, NULL))", ns_add(p->next->sym->name, nse1));
						fprintf(fout,
							" {\n\t\t\t\tif(a->%s::%s == NULL) {\n\t\t\t\t\tif(soap_blist_%s%d == NULL)\n\t\t\t\t\t\tsoap_blist_%s%d = soap_new_block(soap);\n\t\t\t\t\ta->%s::%s = static_cast<%s>(soap_push_block(soap, soap_blist_%s%d, sizeof(%s)));\n\t\t\t\t\tif(a->%s::%s == NULL)\n\t\t\t\t\t\treturn NULL;",
							ident(t->sym->name), ident(p->next->sym->name), ident(p->next->sym->name), i, ident(p->next->sym->name), i, 
							ident(t->sym->name), ident(p->next->sym->name), c_type(p->next->info.typ), ident(p->next->sym->name), i,
							c_type((Tnode *)p->next->info.typ->ref), ident(t->sym->name),ident(p->next->sym->name));
						if(((Tnode *)p->next->info.typ->ref)->type == Tclass || has_class((Tnode *)p->next->info.typ->ref))
							fprintf(fout, "\n\t\t\t\t\tSOAP_PLACEMENT_NEW(a->%s::%s, %s);",
								ident(t->sym->name), ident(p->next->sym->name), c_type((Tnode *)p->next->info.typ->ref));
						if(((Tnode *)p->next->info.typ->ref)->type == Tclass && !is_external((Tnode *)p->next->info.typ->ref) &&
						   !is_volatile((Tnode *)p->next->info.typ->ref) && !is_typedef((Tnode *)p->next->info.typ->ref))
							fprintf(fout, "\n\t\t\t\t\ta->%s::%s->soap_default(soap);", ident(t->sym->name), ident(p->next->sym->name));
						else if(((Tnode *)p->next->info.typ->ref)->type != Tpointer && !is_XML((Tnode *)p->next->info.typ->ref))
							fprintf(fout, "\n\t\t\t\t\tsoap_default_%s(soap, a->%s::%s);",
								c_ident((Tnode *)p->next->info.typ->ref), ident(t->sym->name), ident(p->next->sym->name));
						else
							fprintf(fout, "\n\t\t\t\t\t*a->%s::%s = NULL;", ident(t->sym->name), ident(p->next->sym->name));
						fprintf(fout, "\n\t\t\t\t}");
						if(!is_invisible(p->next->sym->name))
							fprintf(fout, "\n\t\t\t\tsoap_revert(soap);");
						if(is_unmatched(p->next->sym)) {
							if(is_XML((Tnode *)p->next->info.typ->ref) && is_string((Tnode *)p->next->info.typ->ref))
								fprintf(fout, "\n\t\t\t\tif(soap_inliteral(soap, NULL, a->%s::%s))", ident(t->sym->name), ident(p->next->sym->name));
							else if(is_XML((Tnode *)p->next->info.typ->ref) && is_wstring((Tnode *)p->next->info.typ->ref))
								fprintf(fout, "\n\t\t\t\tif(soap_inwliteral(soap, NULL, a->%s::%s))", ident(t->sym->name), ident(p->next->sym->name));
							else
								fprintf(fout, "\n\t\t\t\tif(soap_in_%s(soap, NULL, a->%s::%s, \"%s\"))",
									c_ident((Tnode *)p->next->info.typ->ref), ident(t->sym->name), ident(p->next->sym->name), xsi_type((Tnode *)p->next->info.typ->ref));
						}
						else {
							if(is_XML((Tnode *)p->next->info.typ->ref) && is_string((Tnode *)p->next->info.typ->ref))
							      fprintf(fout, "\n\t\t\t\tif(soap_inliteral(soap, \"%s\", a->%s::%s))",
								      ns_add(p->next->sym->name, nse1), ident(t->sym->name), ident(p->next->sym->name));
						      else if(is_XML((Tnode *)p->next->info.typ->ref) && is_wstring((Tnode *)p->next->info.typ->ref))
							      fprintf(fout, "\n\t\t\t\tif(soap_inwliteral(soap, \"%s\", a->%s::%s))",
								      ns_add(p->next->sym->name, nse1), ident(t->sym->name), ident(p->next->sym->name));
						      else
							      fprintf(fout, "\n\t\t\t\tif(soap_in_%s(soap, \"%s\", a->%s::%s, \"%s\"))",
								      c_ident((Tnode *)p->next->info.typ->ref), ns_add(p->next->sym->name, nse1), ident(
									      t->sym->name), ident(p->next->sym->name),
								      xsi_type((Tnode *)p->next->info.typ->ref)); 
						}
						fprintf(fout, "\n\t\t\t\t{\ta->%s::%s++;\n\t\t\t\t\ta->%s::%s = NULL;\n\t\t\t\t\tcontinue;\n\t\t\t\t}\n\t\t\t}",
							ident(t->sym->name), ident(p->sym->name), ident(t->sym->name), ident(p->next->sym->name));
						/* THIS CODE IS RETAINED HERE FOR REFERENCE PURPUSES
						   IT ILLUSTRATES A DIFFERENT APPROACH THAT IS NOW OBSOLETE */
#if 0
						fprintf(fout, "\n\t\t\tif(soap_flag_%s%d && soap->error == SOAP_TAG_MISMATCH)",
							ident(p->next->sym->name), i);
						if(((Tnode *)p->next->info.typ->ref)->type == Tclass && !is_volatile(p->next->info.typ->ref))
							fprintf(fout, "\n\t\t\t{\t%s;\n\t\t\t\t%s;\n\t\t\t\tq.soap_default(soap);\n\t\t\t\tif(soap_new_block(soap) == NULL)\n\t\t\t\t\treturn NULL;",
								c_type_id(p->next->info.typ, "p"), c_type_id((Tnode *)p->next->info.typ->ref, "q"));
						else if(((Tnode *)p->next->info.typ->ref)->type == Tclass || has_class(p->next->info.typ->ref))
							fprintf(fout, "\n\t\t\t{\t%s;\n\t\t\t\t%s;\n\t\t\t\tif(soap_new_block(soap) == NULL)\n\t\t\t\t\treturn NULL;",
								c_type_id(p->next->info.typ, "p"), c_type_id((Tnode *)p->next->info.typ->ref, "q"));
						else
							fprintf(fout, "\n\t\t\t{\t%s;\n\t\t\t\tif(soap_new_block(soap) == NULL)\n\t\t\t\t\treturn NULL;",
								c_type_id(p->next->info.typ, "p"));
						if(is_unmatched(p->next->sym))
							fprintf(fout, "\n\t\t\t\tfor(a->%s::%s = 0; !soap_element_begin_in(soap, NULL, 1, NULL); a->%s::%s++)",
								ident(t->sym->name), ident(p->sym->name), ns_add_overridden(t, p->next, nse1), ident(t->sym->name));
						else if(is_invisible(p->next->sym->name))
							fprintf(fout, "\n\t\t\t\tfor(a->%s::%s = 0; !soap_peek_element(soap); a->%s::%s++)",
								ident(t->sym->name), ident(p->sym->name), ident(t->sym->name), ident(p->sym->name));
						else
							fprintf(fout, "\n\t\t\t\tfor(a->%s::%s = 0; !soap_element_begin_in(soap, \"%s\", 1, NULL); a->%s::%s++)",
								ident(t->sym->name), ident(p->sym->name), ns_add_overridden(t, p->next, nse1), ident(t->sym->name), ident(p->sym->name));
						fprintf(fout, "\n\t\t\t\t{\tp = static_cast<%s>(soap_push_block(soap, NULL, sizeof(%s)));\n\t\t\t\t\tif(!p)\n\t\t\t\t\t\treturn NULL;",
							c_type(p->next->info.typ), c_type((Tnode *)p->next->info.typ->ref));
						if(((Tnode *)p->next->info.typ->ref)->type == Tclass || has_class(p->next->info.typ->ref))
							fprintf(fout, "\n\t\t\t\t\tmemcpy(p, &q, sizeof(%s));", c_type((Tnode *)p->next->info.typ->ref));
						if(((Tnode *)p->next->info.typ->ref)->type == Tclass && !is_external(p->next->info.typ->ref) &&
						   !is_volatile(p->next->info.typ->ref) && !is_typedef(p->next->info.typ->ref))
							fprintf(fout, "\n\t\t\t\t\tp->soap_default(soap);");
						else if(((Tnode *)p->next->info.typ->ref)->type != Tpointer && !is_XML(p->next->info.typ->ref))
							fprintf(fout, "\n\t\t\t\t\tsoap_default_%s(soap, p);", c_ident(p->next->info.typ->ref));
						else
							fprintf(fout, "\n\t\t\t\t\t*p = NULL;");
						if(!is_invisible(p->next->sym->name))
							fprintf(fout, "\n\t\t\t\t\tsoap_revert(soap);");
						if(is_unmatched(p->next->sym)) {
							if(is_XML(p->next->info.typ->ref) && is_string(p->next->info.typ->ref))
								fprintf(fout, "\n\t\t\t\t\tif(!soap_inliteral(soap, NULL, p))");
							else if(is_XML(p->next->info.typ->ref) && is_wstring(p->next->info.typ->ref))
								fprintf(fout, "\n\t\t\t\t\tif(!soap_inwliteral(soap, NULL, p))");
							else
								fprintf(fout, "\n\t\t\t\t\tif(!soap_in_%s(soap, NULL, p, \"%s\"))", c_ident(p->next->info.typ->ref), xsi_type(p->next->info.typ->ref));
						}
						else {
							if(is_XML(p->next->info.typ->ref) && is_string(p->next->info.typ->ref))
							      fprintf(fout, "\n\t\t\t\t\tif(!soap_inliteral(soap, \"%s\", p))", ns_add_overridden(t, p->next, nse1));
						      else if(is_XML(p->next->info.typ->ref) && is_wstring(p->next->info.typ->ref))
							      fprintf(fout, "\n\t\t\t\t\tif(!soap_inwliteral(soap, \"%s\", p))", ns_add_overridden(t, p->next, nse1));
						      else
							      fprintf(fout, "\n\t\t\t\t\tif(!soap_in_%s(soap, \"%s\", p, \"%s\"))",
								      c_ident(p->next->info.typ->ref), ns_add_overridden(t, p->next, nse1), xsi_type(p->next->info.typ->ref)); }
						fprintf(fout, "\n\t\t\t\t\t\tbreak;");
						fprintf(fout, "\n\t\t\t\t\tsoap_flag_%s%d = 0;", ident(p->next->sym->name), i);
						fprintf(fout, "\n\t\t\t\t}");
						fprintf(fout, "\n\t\t\t\ta->%s::%s = (%s)soap_save_block(soap, 0, 0, 1);",
							ident(t->sym->name), ident(p->next->sym->name), c_type(p->next->info.typ));
						fprintf(fout, "\n\t\t\t\tif(!soap_flag_%s%d && soap->error == SOAP_TAG_MISMATCH)\n\t\t\t\t\tcontinue;\n\t\t\t}",
							ident(p->next->sym->name), i);
#endif
						p = p->next;
					}
					else if(is_anytype(p)) {
						f = 1;
						fprintf(fout, "\n\t\t\tif(soap_flag_%s%d && soap->error == SOAP_TAG_MISMATCH)", ident(p->next->sym->name), i);
						fprintf(fout, "\n\t\t\t\tif((a->%s::%s = soap_getelement(soap, &a->%s::%s)))",
							ident(t->sym->name), ident(p->next->sym->name), ident(t->sym->name), ident(p->sym->name));
						fprintf(fout, "\n\t\t\t\t{\tsoap_flag_%s%d = 0;", ident(p->next->sym->name), i);
						fprintf(fout, "\n\t\t\t\t\tcontinue;");
						fprintf(fout, "\n\t\t\t\t}");
						p = p->next;
					}
					else if(is_discriminant(typ) && p->next) {
						f = 1;
						fprintf(fout, "\n\t\tif(!soap_in_%s(soap, &a->%s, &a->%s))", c_ident(p->next->info.typ), ident(p->sym->name), ident(p->next->sym->name));
						fprintf(fout, "\n\t\t\treturn NULL;");
						i = 0;
						break;
					}
					else if(is_choice(p)) {
						f = 1;
						fprintf(fout, "\n\t\t\tif(soap_flag_%s%d && soap->error == SOAP_TAG_MISMATCH)", ident(p->next->sym->name), i);
						fprintf(fout, "\n\t\t\t\tif(soap_in_%s(soap, &a->%s::%s, &a->%s::%s))", c_ident(p->next->info.typ), ident(t->sym->name),
							ident(p->sym->name), ident(t->sym->name), ident(p->next->sym->name));
						fprintf(fout, " {\n\t\t\t\t\tsoap_flag_%s%d = 0;", ident(p->next->sym->name), i);
						fprintf(fout, "\n\t\t\t\t\tcontinue;");
						fprintf(fout, "\n\t\t\t\t}");
						p = p->next;
					}
					else {
						f = 1;
					      if(!is_invisible(p->sym->name) && !is_primclass(typ) && p->info.typ->type != Tfun && !is_void(p->info.typ)) {
						      if(is_string(p->info.typ) || is_wstring(p->info.typ) || is_stdstr(p->info.typ))
							      fprintf(fout, "\n\t\t\tif(soap_flag_%s%d && oneof2(soap->error, SOAP_TAG_MISMATCH, SOAP_NO_TAG))",
								      ident(p->sym->name), i);
						      else if(is_template(p->info.typ))
							      fprintf(fout, "\n\t\t\tif(soap->error == SOAP_TAG_MISMATCH)");
						      else
							      fprintf(fout, "\n\t\t\tif(soap_flag_%s%d && soap->error == SOAP_TAG_MISMATCH)", ident(p->sym->name), i);
					      }
					      if(is_unmatched(p->sym)) {
						      if(is_XML(p->info.typ) && is_string(p->info.typ)) {
							      fprintf(fout, "\n\t\t\t\tif(soap_inliteral(soap, NULL, &(a->%s::%s)))", ident(t->sym->name), ident(p->sym->name));
						      }
						      else if(is_XML(p->info.typ) && is_wstring(p->info.typ)) {
							      fprintf(fout, "\n\t\t\t\tif(soap_inwliteral(soap, NULL, &(a->%s::%s)))", ident(t->sym->name), ident(p->sym->name));
						      }
						      else if(p->info.typ->type==Tarray) {
							      fprintf(fout, "\n\t\t\t\tif(soap_in_%s(soap, NULL, a->%s::%s, \"%s\"))",
								      c_ident(p->info.typ), ident(t->sym->name), ident(p->sym->name), xsi_type(p->info.typ));
						      }
						      else if(p->info.typ->type==Tclass && !is_external(p->info.typ) &&
						              !is_volatile(p->info.typ) && !is_typedef(p->info.typ)) {
							      fprintf(fout, "\n\t\t\t\tif((a->%s::%s).soap_in(soap, NULL, \"%s\"))",
								      ident(t->sym->name), ident(p->sym->name), xsi_type(p->info.typ));
						      }
						      else if(p->info.typ->type != Tfun && !is_void(p->info.typ)) {
							      fprintf(fout, "\n\t\t\t\tif(soap_in_%s(soap, NULL, &(a->%s::%s), \"%s\"))",
								      c_ident(p->info.typ), ident(t->sym->name), ident(p->sym->name), xsi_type(p->info.typ));
						      }
					      }
					      else if(!is_invisible(p->sym->name)) {
						      if(is_XML(p->info.typ) && is_string(p->info.typ)) {
							      fprintf(fout, "\n\t\t\t\tif(soap_inliteral(soap, \"%s\", &(a->%s::%s)))",
								      ns_add_overridden(t, p, nse1), ident(t->sym->name), ident(p->sym->name));
						      }
						      else if(is_XML(p->info.typ) && is_wstring(p->info.typ)) {
							      fprintf(fout, "\n\t\t\t\tif(soap_inwliteral(soap, \"%s\", &(a->%s::%s)))",
								      ns_add_overridden(t, p, nse1), ident(t->sym->name), ident(p->sym->name));
						      }
						      else if(p->info.typ->type==Tarray) {
							      fprintf(fout, "\n\t\t\t\tif(soap_in_%s(soap, \"%s\", a->%s::%s, \"%s\"))",
								      c_ident(p->info.typ), ns_add_overridden(t, p, nse1), ident(t->sym->name), ident(p->sym->name), xsi_type(p->info.typ));
						      }
						      else if(p->info.typ->type == Tclass && !is_external(p->info.typ) && !is_volatile(p->info.typ) && !is_typedef(p->info.typ)) {
							      fprintf(fout, "\n\t\t\t\tif((a->%s::%s).soap_in(soap, \"%s\", \"%s\"))",
								      ident(t->sym->name), ident(p->sym->name), ns_add_overridden(t, p, nse1), xsi_type(p->info.typ));
						      }
						      else if(p->info.typ->type != Tfun && !is_void(p->info.typ)) {
							      fprintf(fout, "\n\t\t\t\tif(soap_in_%s(soap, \"%s\", &(a->%s::%s), \"%s\"))",
								      c_ident(p->info.typ), ns_add_overridden(t, p, nse1), ident(t->sym->name), ident(p->sym->name), xsi_type(p->info.typ));
						      }
					      }
					      if(!is_invisible(p->sym->name) && !is_primclass(typ) && p->info.typ->type != Tfun && !is_void(p->info.typ)) {
						      if(is_template(p->info.typ))
							      fprintf(fout, "\n\t\t\t\t\tcontinue;");
						      else {
								  fprintf(fout, " {\n\t\t\t\t\tsoap_flag_%s%d--;", ident(p->sym->name), i);
									fprintf(fout, "\n\t\t\t\t\tcontinue;");
									fprintf(fout, "\n\t\t\t\t}"); 
							  }
					      }
					      fflush(fout); }
				}
			}
			if(!f && is_invisible(typ->id->name))
				fprintf(fout, "\n\tsoap->error = SOAP_TAG_MISMATCH;\n\ta = NULL;");
			if(!is_discriminant(typ)) {
				Entry * e;
				char * nse1;
				i = 0;
				for(t = table; t; t = t->prev)
					i++;
				for(; i > 0; i--) {
					t = table;
					for(j = 0; j < i-1; j++)
						t = t->prev;
					e = entry(classtable, t->sym);
					nse1 = e ? ns_qualifiedElement(e->info.typ) : nse;
					for(p = t->list; p; p = p->next) {
						if(is_repetition(p) || is_anytype(p) || is_choice(p)) {
							p = p->next;
							continue;
						}
						if(is_invisible(p->sym->name) && !(p->info.sto&(Sconst|Sprivate|Sprotected)) &&
						   !is_transient(p->info.typ) && !(p->info.sto&Sattribute)) {
							if(is_string(p->info.typ) || is_wstring(p->info.typ) || is_stdstr(p->info.typ))
								fprintf(fout, "\n\t\t\tif(soap_flag_%s%d && oneof2(soap->error, SOAP_TAG_MISMATCH, SOAP_NO_TAG))", ident(p->sym->name), i);
							else if(is_template(p->info.typ))
								fprintf(fout, "\n\t\t\tif(soap->error == SOAP_TAG_MISMATCH)");
							else
								fprintf(fout, "\n\t\t\tif(soap_flag_%s%d && soap->error == SOAP_TAG_MISMATCH)", ident(p->sym->name), i);
							if(is_XML(p->info.typ) && is_string(p->info.typ)) {
								fprintf(fout, "\n\t\t\t\tif(soap_inliteral(soap, \"%s\", &(a->%s::%s)))", ns_add_overridden(t, p, nse1), ident(t->sym->name), ident(p->sym->name));
							}
							else if(is_XML(p->info.typ) && is_wstring(p->info.typ)) {
								fprintf(fout, "\n\t\t\t\tif(soap_inwliteral(soap, \"%s\", &(a->%s::%s)))", ns_add_overridden(t, p, nse1), ident(t->sym->name), ident(p->sym->name));
							}
							else if(p->info.typ->type==Tarray) {
								fprintf(fout, "\n\t\t\t\tif(soap_in_%s(soap, \"%s\", a->%s::%s, \"%s\"))",
									c_ident(p->info.typ), ns_add_overridden(t, p, nse1), ident(t->sym->name), ident(p->sym->name), xsi_type(p->info.typ));
							}
							else if(p->info.typ->type==Tclass && !is_external(p->info.typ) && !is_volatile(p->info.typ) && !is_typedef(p->info.typ)) {
								fprintf(fout, "\n\t\t\t\tif((a->%s::%s).soap_in(soap, \"%s\", \"%s\"))",
									ident(t->sym->name), ident(p->sym->name), ns_add_overridden(t, p, nse1), xsi_type(p->info.typ));
							}
							else if(p->info.typ->type != Tfun && !is_void(p->info.typ)) {
								fprintf(fout, "\n\t\t\t\tif(soap_in_%s(soap, \"%s\", &(a->%s::%s), \"%s\"))",
									c_ident(p->info.typ), ns_add_overridden(t, p, nse1), ident(t->sym->name), ident(p->sym->name), xsi_type(p->info.typ));
							}
							if(is_template(p->info.typ))
								fprintf(fout, "\n\t\t\t\t\tcontinue;");
							else {
								fprintf(fout, " {\n\t\t\t\t\tsoap_flag_%s%d--;", ident(p->sym->name), i);
								fprintf(fout, "\n\t\t\t\t\tcontinue;");
								fprintf(fout, "\n\t\t\t\t}"); 
							}
						}
					}
				}
				for(t = table; t; t = t->prev)
					for(p = t->list; p; p = p->next)
						if(p->info.sto&Sreturn)
							if(nse || has_ns_eq(NULL, p->sym->name))
								fprintf(fout, "\n\t\t\tsoap_check_result(soap, \"%s\");", ns_add(p->sym->name, nse));
				if(!is_invisible(typ->id->name) || table->prev || table->list) {
					fprintf(fout, "\n\t\t\tif(soap->error == SOAP_TAG_MISMATCH)");
					if(!is_invisible(typ->id->name) || is_discriminant(typ))
						fprintf(fout, "\n\t\t\t\tsoap->error = soap_ignore_element(soap);");
					else
						fprintf(fout, "\n\t\t\t\tif(soap_flag) {\n\t\t\t\t\n\t\t\t\t\tsoap->error = SOAP_OK;\n\t\t\t\t\tbreak;\n\t\t\t\t}");
					if(!is_invisible(typ->id->name))
						fprintf(fout, "\n\t\t\tif(soap->error == SOAP_NO_TAG)");
					else
						fprintf(fout, "\n\t\t\tif(soap_flag && soap->error == SOAP_NO_TAG)");
					fprintf(fout, "\n\t\t\t\tbreak;");
					fprintf(fout, "\n\t\t\tif(soap->error)\n\t\t\t\treturn NULL;");
					fprintf(fout, "\n\t\t}");
				}
			}
			if(!is_discriminant(typ)) {
				i = 0;
				for(t = table; t; t = t->prev)
					i++;
				for(; i > 0; i--) {
					t = table;
					for(j = 0; j < i-1; j++)
						t = t->prev;
					for(p = t->list; p; p = p->next) {
						if(is_repetition(p)) {
							fprintf(fout, "\n\t\tif(a->%s::%s)\n\t\t\tsoap_pop_block(soap, soap_blist_%s%d);",
								ident(t->sym->name), ident(p->next->sym->name), ident(p->next->sym->name), i);
							fprintf(fout,
								"\n\t\tif(a->%s::%s)\n\t\t\ta->%s::%s = (%s)soap_save_block(soap, soap_blist_%s%d, NULL, 1);\n\t\telse\n\t\t{\ta->%s::%s = NULL;\n\t\t\tif(soap_blist_%s%d)\n\t\t\t\tsoap_end_block(soap, soap_blist_%s%d);\n\t\t}",
								ident(t->sym->name), ident(p->sym->name), ident(t->sym->name), ident(p->next->sym->name),
								c_type(p->next->info.typ), ident(p->next->sym->name), i,
								ident(t->sym->name), ident(p->next->sym->name),
								ident(p->next->sym->name), i, ident(p->next->sym->name), i);
							p = p->next;
						}
					}
				}
			}
			if(has_getter(typ))
				fprintf(fout, "\n\t\tif(a->get(soap))\n\t\t\treturn NULL;");
			if(!is_invisible(typ->id->name)) {
				fprintf(fout, "\n\t\tif(soap_element_end_in(soap, tag))\n\t\t\treturn NULL;");
				fprintf(fout, "\n\t}\n\telse {\n\t");
				/*
				fprintf(fout, "\ta = (%s)soap_id_forward(soap, soap->href, (void*)a, 0, %s, 0, sizeof(%s), 0, soap_copy_%s);",
					c_type_id(typ, "*"), soap_type(typ), c_type(typ), c_ident(typ));
				*/
				fprintf(fout, "\ta = static_cast<%s>(soap_id_forward(soap, soap->href, a, 0, %s, 0, sizeof(%s), 0, soap_copy_%s));",
					c_type_id(typ, "*"), soap_type(typ), c_type(typ), c_ident(typ));
				fprintf(fout, "\n\t\tif(soap->body && soap_element_end_in(soap, tag))\n\t\t\treturn NULL;");
				fprintf(fout, "\n\t}");
			}
			if(!is_discriminant(typ)) {
				a = 0;
				i = 0;
				for(t = table; t; t = t->prev)
					i++;
				for(; i > 0; i--) {
					t = table;
					for(j = 0; j < i-1; j++)
						t = t->prev;
					for(p = t->list; p; p = p->next) {
						if(p->info.minOccurs > 0 && p->info.maxOccurs >= 0 &&
						   !(p->info.sto&(Sconst|Sprivate|Sprotected)) &&
						   !(p->info.sto&Sattribute) && p->info.typ->type != Tfun &&
						   !is_void(p->info.typ) && !is_transient(p->info.typ) &&
						   !is_template(p->info.typ) && !is_repetition(p) && !is_choice(p) &&
						   p->info.hasval == False)                                 {
							if(is_item(p))
								continue;
							if(is_anytype(p))
								p = p->next;
							if(a==0) {
								fprintf(fout, "\n\tif(%s(soap_flag_%s%d > " SOAP_LONG_FORMAT "",
									strict_check(), ident(p->sym->name), i, p->info.maxOccurs-p->info.minOccurs);
								a = 1;
							}
							else
								fprintf(fout, " || soap_flag_%s%d > " SOAP_LONG_FORMAT "", ident(p->sym->name), i, p->info.maxOccurs-p->info.minOccurs);
						}
						else if(is_template(p->info.typ)) {
							if(p->info.minOccurs > 0) {
								if(p->info.typ->type == Tpointer) {
									if(a==0) {
										fprintf(fout, "\n\tif(%s(!a->%s::%s || a->%s::%s->size() < " SOAP_LONG_FORMAT "", strict_check(), ident(
												t->sym->name), ident(p->sym->name), ident(t->sym->name), ident(p->sym->name), p->info.minOccurs);
										a = 1;
									}
									else
										fprintf(fout, " || !a->%s::%s || a->%s::%s->size() < " SOAP_LONG_FORMAT "", ident(t->sym->name),
											ident(p->sym->name), ident(t->sym->name), ident(p->sym->name), p->info.minOccurs);
								}
								else {
									if(a==0) {
									      fprintf( fout, "\n\tif(%s(a->%s::%s.size() < " SOAP_LONG_FORMAT "", 
											  strict_check(), ident(t->sym->name), ident(p->sym->name), p->info.minOccurs);
									      a = 1;
								      }
								      else
									      fprintf(fout, " || a->%s::%s.size() < " SOAP_LONG_FORMAT "", 
											ident(t->sym->name), ident(p->sym->name), p->info.minOccurs); }
							}
							if(p->info.maxOccurs > 1) {
								if(p->info.typ->type == Tpointer) {
									if(a==0) {
										fprintf(fout, "\n\tif(%s((a->%s::%s && a->%s::%s->size() > " SOAP_LONG_FORMAT ")", strict_check(), ident(
												t->sym->name), ident(p->sym->name), ident(t->sym->name), ident(p->sym->name), p->info.maxOccurs);
										a = 1;
									}
									else
										fprintf(fout, " || (a->%s::%s && a->%s::%s->size() > " SOAP_LONG_FORMAT ")", ident(t->sym->name),
											ident(p->sym->name), ident(t->sym->name), ident(p->sym->name), p->info.maxOccurs);
								}
								else {
									if(a==0) {
									      fprintf(fout, "\n\tif(%s(a->%s::%s.size() > " SOAP_LONG_FORMAT "", 
											  strict_check(), ident(t->sym->name), ident(p->sym->name), p->info.maxOccurs);
									      a = 1;
								      }
								      else
									      fprintf(fout, " || a->%s::%s.size() > " SOAP_LONG_FORMAT "", ident(t->sym->name), ident(p->sym->name), p->info.maxOccurs); 
								}
							}
						}
						else if(is_repetition(p)) {
							if(p->info.minOccurs > 0) {
								if(a== 0) {
									fprintf(fout,
										"\n\tif(%s(a->%s::%s < " SOAP_LONG_FORMAT "", strict_check(), ident(t->sym->name), ident(p->sym->name), p->info.minOccurs);
									a = 1;
								}
								else
									fprintf(fout, " || a->%s::%s < " SOAP_LONG_FORMAT "", ident(t->sym->name), ident(p->sym->name), p->info.minOccurs);
							}
							if(p->info.maxOccurs > 1) {
								if(a==0) {
									fprintf(fout,
										"\n\tif(%s(a->%s::%s > " SOAP_LONG_FORMAT "",
										strict_check(), ident(t->sym->name), ident(p->sym->name), p->info.maxOccurs);
									a = 1;
								}
								else
									fprintf(fout, " || a->%s::%s > " SOAP_LONG_FORMAT "", ident(t->sym->name), ident(p->sym->name), p->info.maxOccurs);
							}
							p = p->next;
						}
						else if(is_choice(p)) {
							if(p->info.minOccurs != 0) {
								if(a== 0) {
									fprintf(fout, "\n\tif(%s(soap_flag_%s%d", strict_check(), ident(p->next->sym->name), i);
									a = 1;
								}
								else
									fprintf(fout, " || soap_flag_%s%d", ident(p->next->sym->name), i);
							}
							p = p->next;
						}
					}
				}
				if(a)
					fprintf(fout, ")) {\n\t\tsoap->error = SOAP_OCCURS;\n\t\treturn NULL;\n\t}");
			}
			fprintf(fout, "\n\treturn a;\n}");
		}
		break;

	    case Tunion:
		if(is_external(typ)) {
			fprintf(fhead, "\nSOAP_FMAC1 %s SOAP_FMAC2 soap_in_%s(struct soap*, int*, %s);", c_type_id(typ, "*"), c_ident(typ), c_type_id(typ, "*"));
			return;
		}
		fprintf(fhead, "\nSOAP_FMAC3 %s FASTCALL soap_in_%s(struct soap*, int*, %s);", c_type_id(typ, "*"), c_ident(typ), c_type_id(typ, "*"));
		fprintf(fout, "\n\nSOAP_FMAC3 %s FASTCALL soap_in_%s(struct soap *soap, int *choice, %s)\n{",
			c_type_id(typ, "*"), c_ident(typ), c_type_id(typ, "*a"));
		fprintf(fout, "\tsoap->error = SOAP_TAG_MISMATCH;");
		table = (Table *)typ->ref;
		for(p = table->list; p; p = p->next) {
			if(p->info.sto&(Sconst|Sprivate|Sprotected))
				fprintf(fout, "\n\t/* non-serializable %s skipped */", ident(p->sym->name));
			else if(is_transient(p->info.typ))
				fprintf(fout, "\n\t/* transient %s skipped */", ident(p->sym->name));
			else if(p->info.sto&Sattribute)
				;
			else if(is_repetition(p))
				;
			else if(is_anytype(p))
				;
			else if(!is_invisible(p->sym->name)) {
				if(is_unmatched(p->sym)) {
					if(is_XML(p->info.typ) && is_string(p->info.typ))
						fprintf(fout, "\n\tif(soap->error == SOAP_TAG_MISMATCH && soap_inliteral(soap, NULL, &a->%s))", ident(p->sym->name));
					else if(is_XML(p->info.typ) && is_wstring(p->info.typ))
						fprintf(fout, "\n\tif(soap->error == SOAP_TAG_MISMATCH && soap_inwliteral(soap, NULL, &a->%s))", ident(p->sym->name));
					else if(p->info.typ->type == Tarray)
						fprintf(fout, "\n\tif(soap->error == SOAP_TAG_MISMATCH && soap_in_%s(soap, NULL, a->%s, \"%s\"))",
							c_ident(p->info.typ), ident(p->sym->name), xsi_type(p->info.typ));
					else if(p->info.typ->type == Tclass && !is_external(p->info.typ) && !is_volatile(p->info.typ) && !is_typedef(p->info.typ))
						fprintf(fout, "\n\tif(soap->error == SOAP_TAG_MISMATCH && a->%s.soap_in(soap, NULL, \"%s\"))",
							ident(p->sym->name), xsi_type(p->info.typ));
					else if(p->info.typ->type != Tfun && !is_void(p->info.typ)) {
						if(p->info.typ->type == Tpointer)
							fprintf(fout, "\n\ta->%s = NULL;", ident(p->sym->name));
						fprintf(fout, "\n\tif(soap->error == SOAP_TAG_MISMATCH && soap_in_%s(soap, NULL, &a->%s, \"%s\"))", c_ident(p->info.typ), ident(p->sym->name), xsi_type(p->info.typ));
					}
				}
				else {
					if(is_XML(p->info.typ) && is_string(p->info.typ))
					      fprintf(fout, "\n\tif(soap->error == SOAP_TAG_MISMATCH && soap_inliteral(soap, \"%s\", &a->%s))", ns_add(p->sym->name, nse), ident(p->sym->name));
				      else if(is_XML(p->info.typ) && is_wstring(p->info.typ))
					      fprintf(fout, "\n\tif(soap->error == SOAP_TAG_MISMATCH && soap_inwliteral(soap, \"%s\", &a->%s))", ns_add(p->sym->name, nse), ident(p->sym->name));
				      else if(p->info.typ->type == Tarray)
					      fprintf(fout, "\n\tif(soap->error == SOAP_TAG_MISMATCH && soap_in_%s(soap, \"%s\", a->%s, \"%s\"))",
						      c_ident(p->info.typ), ns_add(p->sym->name, nse), ident(p->sym->name), xsi_type(p->info.typ));
				      else if(p->info.typ->type == Tclass && !is_external(p->info.typ) && !is_volatile(p->info.typ) && !is_typedef(p->info.typ))
					      fprintf(fout, "\n\tif(soap->error == SOAP_TAG_MISMATCH && a->%s.soap_in(soap, \"%s\", \"%s\"))",
						      ident(p->sym->name), ns_add(p->sym->name, nse), xsi_type(p->info.typ));
				      else if(p->info.typ->type != Tfun && !is_void(p->info.typ)) {
					      if(p->info.typ->type == Tpointer)
						      fprintf(fout, "\n\ta->%s = NULL;", ident(p->sym->name));
					      fprintf(fout, "\n\tif(soap->error == SOAP_TAG_MISMATCH && soap_in_%s(soap, \"%s\", &a->%s, \"%s\"))",
						      c_ident(p->info.typ), ns_add(p->sym->name, nse), ident(p->sym->name), xsi_type(p->info.typ));
				      }
				}
				fprintf(fout, "\n\t{\t*choice = SOAP_UNION_%s_%s;", c_ident(typ), ident(p->sym->name));
				fprintf(fout, "\n\t\treturn a;");
				fprintf(fout, "\n\t}");
				fflush(fout);
			}
		}
		table = (Table *)typ->ref;
		for(p = table->list; p; p = p->next) {
			if(p->info.sto&(Sconst|Sprivate|Sprotected))
				;
			else if(is_transient(p->info.typ))
				;
			else if(p->info.sto&Sattribute)
				;
			else if(is_repetition(p))
				;
			else if(is_anytype(p))
				;
			else if(is_invisible(p->sym->name)) {
				if(is_XML(p->info.typ) && is_string(p->info.typ))
					fprintf(fout, "\n\tif(soap->error == SOAP_TAG_MISMATCH && soap_inliteral(soap, \"%s\", &a->%s))",
						ns_add(p->sym->name, nse), ident(p->sym->name));
				else if(is_XML(p->info.typ) && is_wstring(p->info.typ))
					fprintf(fout, "\n\tif(soap->error == SOAP_TAG_MISMATCH && soap_inwliteral(soap, \"%s\", &a->%s))",
						ns_add(p->sym->name, nse), ident(p->sym->name));
				else if(p->info.typ->type == Tarray)
					fprintf(fout, "\n\tif(soap->error == SOAP_TAG_MISMATCH && soap_in_%s(soap, \"%s\", a->%s, NULL))",
						c_ident(p->info.typ), ns_add(p->sym->name, nse), ident(p->sym->name));
				else if(p->info.typ->type == Tclass && !is_external(p->info.typ) && !is_volatile(p->info.typ) && !is_typedef(p->info.typ))
					fprintf(fout, "\n\tif(soap->error == SOAP_TAG_MISMATCH && a->%s.soap_in(soap, \"%s\", NULL))",
						ident(p->sym->name), ns_add(p->sym->name, nse));
				else if(p->info.typ->type != Tfun && !is_void(p->info.typ)) {
					if(p->info.typ->type == Tpointer)
						fprintf(fout, "\n\ta->%s = NULL;", ident(p->sym->name));
					fprintf(fout, "\n\tif(soap->error == SOAP_TAG_MISMATCH && soap_in_%s(soap, \"%s\", &a->%s, NULL))",
						c_ident(p->info.typ), ns_add(p->sym->name, nse), ident(p->sym->name));
				}
				fprintf(fout, "\n\t{\t*choice = SOAP_UNION_%s_%s;", c_ident(typ), ident(p->sym->name));
				fprintf(fout, "\n\t\treturn a;");
				fprintf(fout, "\n\t}");
				fflush(fout);
			}
		}
		fprintf(fout, "\n\t*choice = 0;\n\tif(!soap->error)\n\t\tsoap->error = SOAP_TAG_MISMATCH;\n\treturn NULL;\n}");
		break;
	    case Tpointer:
		if(is_external(typ)) {
			fprintf(fhead, "\nSOAP_FMAC1 %s SOAP_FMAC2 soap_in_%s(struct soap*, const char*, %s, const char*);", c_type_id(typ, "*"), c_ident(typ), c_type_id(typ, "*"));
			return;
		}
		fprintf(fhead, "\nSOAP_FMAC3 %s FASTCALL soap_in_%s(struct soap*, const char*, %s, const char*);", c_type_id(typ, "*"), c_ident(typ), c_type_id(typ, "*"));
		fprintf(fout, "\n\nSOAP_FMAC3 %s FASTCALL soap_in_%s(struct soap *soap, const char *tag, %s, const char *type)\n{", c_type_id(typ, "*"), c_ident(typ), c_type_id(typ, "*a"));
		fprintf(fout, "\n\tif(soap_element_begin_in(soap, tag, 1, NULL))");
		fprintf(fout, "\n\t\treturn NULL;");
		if(is_template(typ)) {
			fprintf(fout, "\n\tsoap_revert(soap);");
			fprintf(fout, "\n\tif(!a)\n\t\tif(!(a = static_cast<%s>(soap_malloc(soap, sizeof(%s)))))\n\t\t\treturn NULL;", c_type_id(typ, "*"), c_type(typ));
			fprintf(fout, "\n\tif(!(*a = soap_in_%s(soap, tag, *a, type)))\n\t\treturn NULL;", c_ident((Tnode *)typ->ref));
			fprintf(fout, "\n\treturn a;\n}");
		}
		else if(((Tnode *)typ->ref)->type == Tclass && !is_external((Tnode *)typ->ref) &&
		        !is_volatile((Tnode *)typ->ref) && !is_typedef((Tnode *)typ->ref)) {
			fprintf(fout, "\n\tif(!a)\n\t\tif(!(a = static_cast<%s>(soap_malloc(soap, sizeof(%s)))))\n\t\t\treturn NULL;",
				c_type_id(typ, "*"), c_type(typ));
			fprintf(fout, "\n\t*a = NULL;\n\tif(!soap->null && *soap->href != '#') {");
			fprintf(fout, "\n\t\tsoap_revert(soap);");
			fprintf(fout, "\n\t\tif(!(*a = static_cast<%s>(soap_instantiate_%s(soap, -1, soap->type, soap->arrayType, 0))))", c_type(typ), c_ident((Tnode *)typ->ref));
			fprintf(fout, "\n\t\t\treturn NULL;");
			fprintf(fout, "\n\t\t(*a)->soap_default(soap);");
			fprintf(fout, "\n\t\tif(!(*a)->soap_in(soap, tag, NULL))");
			fprintf(fout, "\n\t\t\treturn NULL;");
			fprintf(fout, "\n\t}\n\telse {\n\t\t%s p = static_cast<%s>(soap_id_lookup(soap, soap->href, reinterpret_cast<void **>(a), %s, sizeof(%s), %d));",
				c_type_id(typ, "*"), c_type_id(typ, "*"), soap_type((Tnode *)typ->ref), c_type((Tnode *)typ->ref), reflevel((Tnode *)typ->ref));
			if(((Tnode *)typ->ref)->type == Tclass) {
				table = (Table *)((Tnode *)typ->ref)->ref;
				for(p = classtable->list; p; p = p->next) {
					if(p->info.typ->type == Tclass) {
						Table * q = (Table *)p->info.typ->ref;
						if(q)
							for(q = q->prev; q; q = q->prev)
								if(q == table)
									fprintf(fout,
										"\n\t\tif(!p && soap->error == SOAP_HREF)\n\t\t{\tsoap->error = SOAP_OK;\n\t\t\tp = static_cast<%s>(soap_id_lookup(soap, soap->href, reinterpret_cast<void **>(a), %s, sizeof(%s), 0));\n\t\t}",
										c_type_id(typ, "*"), soap_type(p->info.typ), c_type(p->info.typ));
					}
				}
			}
			fprintf(fout, "\n\t\ta = p;");
			fprintf(fout, "\n\t\tif(soap->body && soap_element_end_in(soap, tag))\n\t\t\treturn NULL;");
			fprintf(fout, "\n\t}\n\treturn a;\n}");
		}
		else {
			fprintf(fout, "\n\tif(!a)\n\t\tif(!(a = static_cast<%s>(soap_malloc(soap, sizeof(%s)))))\n\t\t\treturn NULL;", c_type_id(typ, "*"), c_type(typ));
			fprintf(fout, "\n\t*a = NULL;\n\tif(!soap->null && *soap->href != '#') {");
			fprintf(fout, "\n\t\tsoap_revert(soap);");
			fprintf(fout, "\n\t\tif(!(*a = soap_in_%s(soap, tag, *a, type)))", c_ident((Tnode *)typ->ref));
			fprintf(fout, "\n\t\t\treturn NULL;");
			fprintf(fout, "\n\t}\n\telse {\n\t\ta = static_cast<%s>(soap_id_lookup(soap, soap->href, reinterpret_cast<void **>(a), %s, sizeof(%s), %d));",
				c_type_id(typ, "*"), soap_type((Tnode *)typ->ref), c_type((Tnode *)typ->ref), reflevel((Tnode *)typ->ref) );
			fprintf(fout, "\n\t\tif(soap->body && soap_element_end_in(soap, tag))\n\t\t\treturn NULL;");
			fprintf(fout, "\n\t}\n\treturn a;\n}");
		}
		break;

	    case Tarray:
		temp = typ;
		while(temp->type == Tarray) {
			temp = (Tnode *)temp->ref;
		}
		if(is_external(typ)) {
			fprintf(fhead,"\nSOAP_FMAC1 %s SOAP_FMAC2 soap_in_%s(struct soap*, const char*, %s, const char*);",
				c_type_id(temp, "*"), c_ident(typ), c_type(typ));
			return;
		}
		fprintf(fhead, "\nSOAP_FMAC3 %s FASTCALL soap_in_%s(struct soap*, const char*, %s, const char*);",
			c_type_id(temp, "*"), c_ident(typ), c_type(typ));
		fprintf(fout, "\n\nSOAP_FMAC3 %s FASTCALL soap_in_%s(struct soap *soap, const char *tag, %s, const char *type)\n{",
			c_type_id(temp, "*"), c_ident(typ), c_type_id(typ, "a"));
		fprintf(fout, "\n\tif(soap_element_begin_in(soap, tag, 0, NULL))");
		fprintf(fout, "\n\t\treturn NULL;");
		fprintf(fout, "\n\tif(soap_match_array(soap, type))");
		fprintf(fout, " {\n\t\tsoap->error = SOAP_TYPE;\n\t\treturn NULL;\n\t}");
		fprintf(fout, "\n\ta = (%s)soap_id_enter(soap, soap->id, a, %s, sizeof(%s), 0, NULL, NULL, NULL);",
			c_type_id((Tnode *)typ->ref, "(*)"), soap_type(typ), c_type(typ));
		fprintf(fout, "\n\tif(!a)\n\t\treturn NULL;");
		fprintf(fout, "\n\tsoap_default_%s(soap, a);", c_ident(typ));
		fprintf(fout, "\n\tif(soap->body && !*soap->href)");
		total = get_dimension(typ);
		n = (Tnode *)typ->ref;
		cardinality = 1;
		while(n->type==Tarray) {
			total = total*get_dimension(n);
			n = (Tnode *)n->ref;
			cardinality++;
		}
		fprintf(fout, "{\n\t\tint i;\n\t\tfor(i = 0; i < %d; i++)", get_dimension(typ));
		fprintf(fout, "\n\t\t{\tsoap_peek_element(soap);\n\t\t\tif(soap->position)\n\t\t\t{\ti = soap->positions[0];\n\t\t\t\tif(i < 0 || i >= %d)\n\t\t\t\t{\tsoap->error = SOAP_IOB;\n\t\t\t\t\treturn NULL;\n\t\t\t\t}\n\t\t\t}",
			get_dimension(typ));
		fprintf(fout, "\n\t\t\tif(!soap_in_%s(soap, NULL, a", c_ident((Tnode *)typ->ref));
		if(cardinality > 1) {
			fprintf(fout, "[i]");
		}
		else {
			fprintf(fout, "+i");
		}
		fprintf(fout, ", \"%s\"))", xsi_type((Tnode *)typ->ref));
		fprintf(fout, "\n\t\t\t{\tif(soap->error != SOAP_NO_TAG)\n\t\t\t\t\treturn NULL;");
		fprintf(fout, "\n\t\t\t\tsoap->error = SOAP_OK;");
		fprintf(fout, "\n\t\t\t\tbreak;");
		fprintf(fout, "\n\t\t\t}");
		fprintf(fout, "\n\t\t}");
		fprintf(fout,
			"\n\t\tif(soap->mode & SOAP_C_NOIOB)\n\t\t\twhile(soap_element_end_in(soap, tag) == SOAP_SYNTAX_ERROR)\n\t\t\t{\tsoap->peeked = 1;\n\t\t\t\tsoap_ignore_element(soap);\n\t\t\t}");
		fprintf(fout,
			"\n\t\telse if(soap_element_end_in(soap, tag))\n\t\t{\tif(soap->error == SOAP_SYNTAX_ERROR)\n\t\t\t\tsoap->error = SOAP_IOB;\n\t\t\treturn NULL;\n\t\t}");
		fprintf(fout,
			"\n\t}\n\telse {\n\t\ta = (%s)soap_id_forward(soap, soap->href, (void*)soap_id_enter(soap, soap->id, a, %s, sizeof(%s), 0, NULL, NULL, NULL), 0, %s, 0, sizeof(%s), 0, NULL);",
			c_type_id((Tnode *)typ->ref, "(*)"), soap_type(typ), c_type(typ), soap_type(typ), c_type(typ));
		fprintf(fout, "\n\t\tif(soap->body && soap_element_end_in(soap, tag))\n\t\t\treturn NULL;");
		fprintf(fout, "\n\t}\n\treturn (%s)a;\n}", c_type_id(temp, "*"));
		break;

	    case Tenum:
		if(is_external(typ)) {
			fprintf(fhead, "\nSOAP_FMAC1 %s SOAP_FMAC2 soap_in_%s(struct soap*, const char*, %s, const char*);",
				c_type_id(typ, "*"), c_ident(typ), c_type_id(typ, "*"));
			return;
		}
		fprintf(fhead, "\nSOAP_FMAC3 %s FASTCALL soap_in_%s(struct soap*, const char*, %s, const char*);", c_type_id(typ, "*"), c_ident(typ), c_type_id(typ, "*"));
		fprintf(fhead, "\n\nSOAP_FMAC3S int SOAP_FMAC4S soap_s2%s(struct soap*, const char*, %s);", c_ident(typ), c_type_id(typ, "*"));
		fprintf(fout, "\n\nSOAP_FMAC3S int SOAP_FMAC4S soap_s2%s(struct soap *soap, const char *s, %s)\n{", c_ident(typ), c_type_id(typ, "*a"));
		if(is_typedef(typ))
			fprintf(fout, "\n\treturn soap_s2%s(soap, s, a);\n}", t_ident(typ));
		else if(!is_mask(typ)) {
			fprintf(fout, "\n\tconst struct soap_code_map *map;");
			t = (Table *)typ->ref;
			if(t && t->list && has_ns_eq(NULL, ns_remove1(t->list->sym->name))) {
				fprintf(fout, "\n\tchar *t;");
				fprintf(fout, "\n\tif(!s)\n\t\treturn soap->error;");
				fprintf(fout, "\n\tsoap_s2QName(soap, s, &t, %ld, %ld);", minlen(typ), maxlen(typ));
				fprintf(fout, "\n\tmap = soap_code(soap_codes_%s, t);", c_ident(typ));
			}
			else {
				fprintf(fout, "\n\tif(!s)\n\t\treturn soap->error;");
			      fprintf(fout, "\n\tmap = soap_code(soap_codes_%s, s);", c_ident(typ)); 
			}
			min = 0;
			max = 0;
			for(t = (Table *)typ->ref; t; t = t->prev) {
				for(p = t->list; p; p = p->next) {
					if(p->info.val.i < min)
						min = (unsigned long)p->info.val.i;
					if(p->info.val.i > max)
						max = (unsigned long)p->info.val.i;
				}
			}
			if(is_boolean(typ))
				fprintf(fout,
					"\n\tif(map)\n\t\t*a = (%s)(map->code != 0);\n\telse {\n\t\tlong n;\n\t\tif(soap_s2long(soap, s, &n) || n < 0 || n > 1)\n\t\t\treturn soap->error = SOAP_TYPE;\n\t\t*a = (%s)(n != 0);\n\t}\n\treturn SOAP_OK;\n}",
					c_type(typ), c_type(typ));
			else if(sflag)
				fprintf(fout,
					"\n\tif(map)\n\t\t*a = (%s)map->code;\n\telse\n\t\treturn soap->error = SOAP_TYPE;\n\treturn SOAP_OK;\n}",
					c_type(typ));
			else
				fprintf(fout,
					"\n\tif(map)\n\t\t*a = (%s)map->code;\n\telse {\n\t\tlong n;\n\t\tif(soap_s2long(soap, s, &n) || ((soap->mode & SOAP_XML_STRICT) && (n < %ld || n > %ld)))\n\t\t\treturn soap->error = SOAP_TYPE;\n\t\t*a = (%s)n;\n\t}\n\treturn SOAP_OK;\n}",
					c_type(typ), min, max, c_type(typ));
		}
		else {
			t = (Table *)typ->ref;
			if(t && t->list && has_ns_eq(NULL, ns_remove1(t->list->sym->name))) {
				fprintf(fout, "\n\tchar *t;");
				fprintf(fout, "\n\tsoap_s2QName(soap, s, &t, %ld, %ld);", minlen(typ), maxlen(typ));
				fprintf(fout, "\n\t*a = (%s)soap_code_bits(soap_codes_%s, t);", c_type(typ), c_ident(typ));
			}
			else
				fprintf(fout, "\n\t*a = (%s)soap_code_bits(soap_codes_%s, s);", c_type(typ), c_ident(typ));
			fprintf(fout, "\n\treturn SOAP_OK;\n}"); 
		}
		fprintf(fout,
			"\n\nSOAP_FMAC3 %s FASTCALL soap_in_%s(struct soap *soap, const char *tag, %s, const char *type)\n{",
			c_type_id(typ, "*"), c_ident(typ), c_type_id(typ, "*a"));
		if(is_boolean(typ)) {
			fprintf(fout, "\n\tif(soap_element_begin_in(soap, tag, 0, NULL))");
			fprintf(fout, "\n\t\treturn NULL;");
			fprintf(fout, "\n\tif(*soap->type && soap_match_tag(soap, soap->type, type) && soap_match_tag(soap, soap->type, \":boolean\"))");
			fprintf(fout, "\n\t{\tsoap->error = SOAP_TYPE;\n\t\treturn NULL;\n\t}");
		}
		else if(typ->sym) {
			fprintf(fout, "\n\tif(soap_element_begin_in(soap, tag, 0, NULL))");
			fprintf(fout, "\n\t\treturn NULL;");
			fprintf(fout, "\n\tif(*soap->type && soap_match_tag(soap, soap->type, type) && soap_match_tag(soap, soap->type, \"%s\"))", base_type(typ, ""));
			fprintf(fout, " {\n\t\tsoap->error = SOAP_TYPE;\n\t\treturn NULL;\n\t}");
		}
		else {
			fprintf(fout, "\n\tif(soap_element_begin_in(soap, tag, 0, type))");
			fprintf(fout, "\n\t\treturn NULL;"); 
		}
		fprintf(fout, "\n\ta = (%s)soap_id_enter(soap, soap->id, a, %s, sizeof(%s), 0, NULL, NULL, NULL);",
			c_type_id(typ, "*"), soap_type(typ), c_type(typ));
		fprintf(fout, "\n\tif(!a)\n\t\treturn NULL;");
		fprintf(fout, "\n\tif(soap->body && !*soap->href) {\n\t");
		fprintf(fout, "\tif(!a || soap_s2%s(soap, soap_value(soap), a) || soap_element_end_in(soap, tag))\n\t\t\treturn NULL;", c_ident(typ));
		fprintf(fout, "\n\t}\n\telse {\n\t\ta = (%s)soap_id_forward(soap, soap->href, (void*)a, 0, %s, 0, sizeof(%s), 0, NULL);", c_type_id(typ, "*"), soap_type(typ), c_type(typ));
		fprintf(fout, "\n\t\tif(soap->body && soap_element_end_in(soap, tag))\n\t\t\treturn NULL;");
		fprintf(fout, "\n\t}\n\treturn a;\n}");
		break;

	    case Ttemplate:
		if(is_external(typ)) {
			fprintf(fhead, "\nSOAP_FMAC1 %s SOAP_FMAC2 soap_in_%s(struct soap*, const char*, %s, const char*);",
				c_type_id(typ, "*"), c_ident(typ), c_type_id(typ, "*"));
			return;
		}
		fprintf(fhead, "\nSOAP_FMAC3 %s FASTCALL soap_in_%s(struct soap*, const char*, %s, const char*);",
			c_type_id(typ, "*"), c_ident(typ), c_type_id(typ, "*"));
		fprintf(fout, "\n\nSOAP_FMAC3 %s FASTCALL soap_in_%s(struct soap *soap, const char *tag, %s, const char *type)\n{",
			c_type_id(typ, "*"), c_ident(typ), c_type_id(typ, "*a"));
		n = (Tnode *)typ->ref;
		fprintf(fout, "\n\t(void)type; /* appease -Wall -Werror */\n\tif(soap_element_begin_in(soap, tag, 1, NULL))\n\t\treturn NULL;");
		fprintf(fout, "\n\tif(!a && !(a = soap_new_%s(soap, -1)))\n\t\treturn NULL;", c_ident(typ));
		/* fprintf(fout, "\n\t%s::iterator i;\n\t;", c_type(typ)); */
		fprintf(fout, "\n\t%s;\n\tshort soap_flag = 0;", c_type_id(n, "n"));
		fprintf(fout, "\n\tdo {\n");
		fprintf(fout, "\t\tif(tag && *tag != '-')\n\t\t\tsoap_revert(soap);\n\t\t");
		if(n->type == Tpointer)
			fprintf(fout, "n = NULL;");
		else if(n->type == Tarray)
			fprintf(fout, "soap_default_%s(soap, &n);", c_ident(n));
		else if(n->type==Tclass && !is_external(n) && !is_volatile(n) && !is_typedef(n))
			fprintf(fout, "n.soap_default(soap);");
		else if(n->type != Tfun && !is_void(n) && !is_XML(n))
			fprintf(fout, "soap_default_%s(soap, &n);", c_ident(n));
		fprintf(fout, "\n\t\tif(*soap->id || *soap->href)");
		fprintf(fout, "\n\t\t{\tif(!soap_container_id_forward(soap, *soap->id?soap->id:soap->href, a, (size_t)a->size(), %s, %s, sizeof(%s), %d))\n\t\t\t\tbreak;\n\t\t\t",
			soap_type(reftype(n)), soap_type(typ), c_type(reftype(n)), reflevel(n));
		if(is_XML(n) && is_string(n))
			fprintf(fout, "if(!soap_inliteral(soap, tag, NULL))");
		else if(is_XML(n) && is_wstring(n))
			fprintf(fout, "if(!soap_inwliteral(soap, tag, NULL))");
		else if(n->type==Tarray)
			fprintf(fout, "if(!soap_in_%s(soap, tag, NULL, \"%s\"))", c_ident(n), xsi_type(n));
		else if(n->type != Tfun && !is_void(n))
			fprintf(fout, "if(!soap_in_%s(soap, tag, NULL, \"%s\"))", c_ident(n), xsi_type(n));
		fprintf(fout, "\n\t\t\t\tbreak;");
		fprintf(fout, "\n\t\t}\n\t\telse\n\t\t{");
		if(is_XML(n) && is_string(n))
			fprintf(fout, "\n\t\t\tif(!soap_inliteral(soap, tag, &n))");
		else if(is_XML(n) && is_wstring(n))
			fprintf(fout, "\n\t\t\tif(!soap_inwliteral(soap, tag, &n))");
		else if(n->type==Tarray)
			fprintf(fout, "\n\t\t\tif(!soap_in_%s(soap, tag, &n, \"%s\"))", c_ident(n), xsi_type(n));
		else if(n->type != Tfun && !is_void(n))
			fprintf(fout, "\n\t\t\tif(!soap_in_%s(soap, tag, &n, \"%s\"))", c_ident(n), xsi_type(n));
		fprintf(fout, "\n\t\t\t\tbreak;");
		if((sstreq(typ->id->name, "std::vector") || sstreq(typ->id->name, "std::deque")) && (is_primitive(n) || n->type == Tpointer))
			fprintf(fout, "\n\t\t}\n\t\ta->push_back(n);\n\t\tsoap_flag = 1;");
		else {
			if(is_primitive(n) || n->type == Tpointer)
				fprintf(fout, "\n\t\t}\n\t\ta->insert(a->end(), n);\n\t\tsoap_flag = 1;");
			else
				fprintf(fout,
					"\n\t\t}\n\t\tsoap_update_pointers(soap, (char*)&n, (char*)&n + sizeof(n), (char*)&(*a->insert(a->end(), n)), (char*)&n);\n\t\tsoap_flag = 1;");
		}
		fprintf(fout, "\n\t}\n\twhile(tag && *tag != '-' && !soap_element_begin_in(soap, tag, 1, NULL));");
		fprintf(fout, "\n\tif(soap_flag && oneof2(soap->error, SOAP_TAG_MISMATCH, SOAP_NO_TAG)) {\n\t\tsoap->error = SOAP_OK;\n\t\treturn a;\n\t}\n\treturn NULL;\n}");
		break;
	    default: break;
	}
	fflush(fout);
}

void soap_in_Darray(Tnode * typ)
{
	int    i, j;
	Table * t;
	char * nsa = ns_qualifiedAttribute(typ);
	Table * table = (Table *)typ->ref;
	Entry * p = is_dynamic_array(typ);
	int    d = get_Darraydims(typ);
	if(is_external(typ)) {
		fprintf(fhead, "\nSOAP_FMAC1 %s SOAP_FMAC2 soap_in_%s(struct soap*, const char*, %s, const char*);",
			c_type_id(typ, "*"), c_ident(typ), c_type_id(typ, "*"));
		return;
	}
	fprintf(fhead, "\nSOAP_FMAC3 %s FASTCALL soap_in_%s(struct soap*, const char*, %s, const char*);",
		c_type_id(typ, "*"), c_ident(typ), c_type_id(typ, "*"));
	if(typ->type == Tclass && !is_volatile(typ) && !is_typedef(typ)) {
		fprintf(fout, "\n\nvoid *%s::soap_in(struct soap *soap, const char *tag, const char *type)", c_type(typ));
		fprintf(fout, "\n\t{ return soap_in_%s(soap, tag, this, type); }", c_ident(typ));
	}
	fflush(fout);
	fprintf(fout, "\n\nSOAP_FMAC3 %s FASTCALL soap_in_%s(struct soap *soap, const char *tag, %s, const char *type)",
		c_type_id(typ, "*"), c_ident(typ), c_type_id(typ, "*a"));
	if((has_ns(typ) || is_untyped(typ)) && is_binary(typ))
		fprintf(fout, "\n{\n\t(void)type; /* appease -Wall -Werror */");
	else if(d)
		fprintf(fout, "\n{\n\tint i, j, n;\n\t%s;", c_type_id(p->info.typ, "p"));
	else
		fprintf(fout, "\n{\n\tint i, j;\n\t%s;", c_type_id(p->info.typ, "p"));
	fprintf(fout, "\n\tif(soap_element_begin_in(soap, tag, 1, NULL))\n\t\treturn NULL;");
	if(has_ns(typ) || is_untyped(typ)) {
		if(is_hexBinary(typ))
			fprintf(fout, "\n\tif(*soap->type && soap_match_tag(soap, soap->type, type) && soap_match_tag(soap, soap->type, \":hexBinary\"))");
		else if(is_binary(typ))
			fprintf(fout, "\n\tif(*soap->type && soap_match_tag(soap, soap->type, type) && soap_match_tag(soap, soap->type, \":base64Binary\") && soap_match_tag(soap, soap->type, \":base64\"))");
		else
			fprintf(fout, "\n\tif(*soap->type && soap_match_array(soap, \"%s\") && soap_match_tag(soap, soap->type, type))",
				xsi_type((Tnode *)p->info.typ->ref));
	}
	else
		fprintf(fout, "\n\tif(soap_match_array(soap, type))");
	fprintf(fout, " {\n\t\tsoap->error = SOAP_TYPE;\n\t\treturn NULL;\n\t}");
	if(typ->type == Tclass) {
		fprintf(fout, "\n\ta = (%s)soap_class_id_enter(soap, soap->id, a, %s, sizeof(%s), soap->type, soap->arrayType);",
			c_type_id(typ, "*"), soap_type(typ), c_type(typ));
		fprintf(fout, "\n\tif(!a)\n\t\treturn NULL;");
		fprintf(fout, "\n\tif(soap->alloced)\n\t\ta->soap_default(soap);");
		for(t = (Table *)typ->ref; t; t = t->prev) {
			for(p = t->list; p; p = p->next)
				if(p->info.sto&Sattribute)
					soap_attr_value(p, ptr_cast(t, "a"), ident(p->sym->name), ns_add(p->sym->name, nsa));
		}
	}
	else {
		fprintf(fout, "\n\ta = (%s)soap_id_enter(soap, soap->id, a, %s, sizeof(%s), 0, NULL, NULL, NULL);",
		      c_type_id(typ, "*"), soap_type(typ), c_type(typ));
	      fprintf(fout, "\n\tif(!a)\n\t\treturn NULL;");
		/*fprintf(fout,"\n\tif(soap->alloced)");*/
	      fprintf(fout, "\n\tsoap_default_%s(soap, a);", c_ident(typ));
	      for(t = (Table *)typ->ref; t; t = t->prev) {
		      for(p = t->list; p; p = p->next)
			      if(p->info.sto&Sattribute)
				      soap_attr_value(p, "a", ident(p->sym->name), ns_add(p->sym->name, nsa));
	      }
	}
	fprintf(fout, "\n\tif(soap->body && !*soap->href) {\t");
	p = is_dynamic_array(typ);
	if((has_ns(typ) || is_untyped(typ)) && is_binary(typ)) {
		if(is_hexBinary(typ))
			fprintf(fout, "\n\t\ta->__ptr = soap_gethex(soap, &a->__size);");
		else {
			fprintf(fout, "\n\t\ta->__ptr = soap_getbase64(soap, &a->__size, 0);");
		      if(is_attachment(typ))
			      fprintf(fout, "\n#ifndef WITH_LEANER\n\t\tif(soap_xop_forward(soap, &a->__ptr, &a->__size, &a->id, &a->type, &a->options))\n\t\t\treturn NULL;\n#endif"); }
		fprintf(fout, "\n\t\tif((!a->__ptr && soap->error) || soap_element_end_in(soap, tag))\n\t\t\treturn NULL;");
	}
	else {
		if(d) {
		      fprintf(fout, "\n\t\tn = soap_getsizes(soap->arraySize, a->__size, %d);", d);
		      if(has_offset(typ))
			      fprintf(fout, "\n\t\tn -= j = soap_getoffsets(soap->arrayOffset, a->__size, a->__offset, %d);", d);
		      else
			      fprintf(fout, "\n\t\tn -= j = soap_getoffsets(soap->arrayOffset, a->__size, NULL, %d);", d);
		      if(p->info.minOccurs > 0)
			      fprintf(fout, "\n\t\tif(%sn >= 0 && n < " SOAP_LONG_FORMAT ")\n\t\t{\tsoap->error = SOAP_OCCURS;\n\t\t\treturn NULL;\n\t\t}",
				      strict_check(), p->info.minOccurs);
		      if(p->info.maxOccurs > 1)
			      fprintf(fout, "\n\t\tif(%sn > " SOAP_LONG_FORMAT ")\n\t\t{\tsoap->error = SOAP_OCCURS;\n\t\t\treturn NULL;\n\t\t}",
				      strict_check(), p->info.maxOccurs);
		      fprintf(fout, "\n\t\tif(n >= 0)");
		      if(((Tnode *)p->info.typ->ref)->type == Tclass) {
			      fprintf(fout, "\n\t\t{\ta->%s = soap_new_%s(soap, n);", ident(p->sym->name),
				      c_ident((Tnode *)p->info.typ->ref));
			      if(!is_external((Tnode *)p->info.typ->ref) && !is_volatile((Tnode *)p->info.typ->ref) && !is_typedef((Tnode *)p->info.typ->ref))
				      fprintf(fout, "\n\t\t\tfor(i = 0; i < n; i++)\n\t\t\t\t(a->%s+i)->%s::soap_default(soap);",
					      ident(p->sym->name), c_type((Tnode *)p->info.typ->ref));
			      else if(((Tnode *)p->info.typ->ref)->type == Tpointer)
				      fprintf(fout, "\n\t\t\tfor(i = 0; i < n; i++)\n\t\t\t\tsoap_default_%s(soap, a->%s+i);",
					      c_ident((Tnode *)p->info.typ->ref), ident(p->sym->name));
		      }
		      else if(has_class((Tnode *)p->info.typ->ref)) {
			      fprintf(fout, "\n\t\t{\ta->%s = soap_new_%s(soap, n);", ident(p->sym->name),
				      c_ident((Tnode *)p->info.typ->ref));
			      fprintf(fout, "\n\t\t\tfor(i = 0; i < n; i++)\n\t\t\t\tsoap_default_%s(soap, a->%s+i);",
				      c_ident((Tnode *)p->info.typ->ref), ident(p->sym->name));
		      }
		      else {fprintf(fout, "\n\t\t{\ta->%s = static_cast<%s>(soap_malloc(soap, n*sizeof(%s)));", ident(p->sym->name),
				    c_type_id((Tnode *)p->info.typ->ref, "*"),  c_type((Tnode *)p->info.typ->ref));
			    if(((Tnode *)p->info.typ->ref)->type == Tpointer)
				    fprintf(fout, "\n\t\t\tfor(i = 0; i < n; i++)\n\t\t\t\ta->%s[i] = NULL;", ident(p->sym->name));
			    else if(!is_XML((Tnode *)p->info.typ->ref))
				    fprintf(fout, "\n\t\t\tfor(i = 0; i < n; i++)\n\t\t\t\tsoap_default_%s(soap, a->%s+i);",
					    c_ident((Tnode *)p->info.typ->ref), ident(p->sym->name)); }
		      fprintf(fout, "\n\t\t\tfor(i = 0; i < n; i++)");
		      fprintf(fout, "\n\t\t\t{\tsoap_peek_element(soap);\n\t\t\t\tif(soap->position == %d)", d);
		      fprintf(fout, "\n\t\t\t\t{\ti = ");
		      for(i = 0; i < d; i++) {
			      fprintf(fout, "soap->positions[%d]", i);
			      for(j = 1; j < d-i; j++)
				      fprintf(fout, "*a->__size[%d]", j);
			      if(i < d-1)
				      fprintf(fout, "+");
		      }
		      fprintf(fout, "-j;");
		      fprintf(fout, "\n\t\t\t\t\tif(i < 0 || i >= n)\n\t\t\t\t\t{\tsoap->error = SOAP_IOB;\n\t\t\t\t\t\treturn NULL;\n\t\t\t\t\t}\n\t\t\t\t}");
		      fprintf(fout, "\n\t\t\t\tif(!soap_in_%s(soap, NULL, a->%s + i, \"%s\"))",
			      c_ident((Tnode *)p->info.typ->ref), ident(p->sym->name),
			      xsi_type((Tnode *)p->info.typ->ref));
		      fprintf(fout, "\n\t\t\t\t{\tif(soap->error != SOAP_NO_TAG)\n\t\t\t\t\t\treturn NULL;");
		      fprintf(fout, "\n\t\t\t\t\tsoap->error = SOAP_OK;");
		      fprintf(fout, "\n\t\t\t\t\tbreak;");
		      fprintf(fout, "\n\t\t\t\t}");
	      }
	      else {
			  fprintf(fout, "\n\t\ta->__size = soap_getsize(soap->arraySize, soap->arrayOffset, &j);");
		    if(has_offset(typ) && (p->next->next->info.sto&Sconst) == 0) {
			    fprintf(fout, "\n\t\ta->__offset = j;");
		    }
		    if(p->info.minOccurs > 0)
			    fprintf(fout, "\n\t\tif(%sa->__size >= 0 && a->__size < " SOAP_LONG_FORMAT ")\n\t\t{\tsoap->error = SOAP_OCCURS;\n\t\t\treturn NULL;\n\t\t}",
				    strict_check(), p->info.minOccurs);
		    if(p->info.maxOccurs > 1)
			    fprintf(fout, "\n\t\tif(%sa->__size > " SOAP_LONG_FORMAT ")\n\t\t{\tsoap->error = SOAP_OCCURS;\n\t\t\treturn NULL;\n\t\t}",
				    strict_check(), p->info.maxOccurs);
		    fprintf(fout, "\n\t\tif(a->__size >= 0)");
		    if(((Tnode *)p->info.typ->ref)->type == Tclass) {
			    fprintf(fout, "\n\t\t{\ta->%s = soap_new_%s(soap, a->__size);", ident(p->sym->name), c_ident((Tnode *)p->info.typ->ref));
			    if(!is_external((Tnode *)p->info.typ->ref) && !is_volatile((Tnode *)p->info.typ->ref) && !is_typedef((Tnode *)p->info.typ->ref))
				    fprintf(fout, "\n\t\t\tfor(i = 0; i < a->__size; i++)\n\t\t\t\t(a->%s+i)->%s::soap_default(soap);",
					    ident(p->sym->name), c_type((Tnode *)p->info.typ->ref));
			    else
				    fprintf(fout, "\n\t\t\tfor(i = 0; i < a->__size; i++)\n\t\t\t\tsoap_default_%s(soap, a->%s+i);",
					    c_ident((Tnode *)p->info.typ->ref), ident(p->sym->name));
		    }
		    else if(has_class((Tnode *)p->info.typ->ref)) {
			    fprintf(fout, "\n\t\t{\ta->%s = soap_new_%s(soap, a->__size);", ident(p->sym->name), c_ident((Tnode *)p->info.typ->ref));
			    fprintf(fout, "\n\t\t\tfor(i = 0; i < a->__size; i++)\n\t\t\t\tsoap_default_%s(soap, a->%s+i);", c_ident((Tnode *)p->info.typ->ref), ident(p->sym->name));
		    }
		    else {
				fprintf(fout, "\n\t\t{\ta->%s = static_cast<%s>(soap_malloc(soap, sizeof(%s) * a->__size));", ident(p->sym->name),
				  c_type_id((Tnode *)p->info.typ->ref, "*"),  c_type((Tnode *)p->info.typ->ref));
			  if(((Tnode *)p->info.typ->ref)->type == Tpointer)
				  fprintf(fout, "\n\t\t\tfor(i = 0; i < a->__size; i++)\n\t\t\t\ta->%s[i] = NULL;", ident(p->sym->name));
			  else if(!is_XML((Tnode *)p->info.typ->ref))
				  fprintf(fout, "\n\t\t\tfor(i = 0; i < a->__size; i++)\n\t\t\t\tsoap_default_%s(soap, a->%s+i);",
					  c_ident((Tnode *)p->info.typ->ref), ident(p->sym->name)); }
		    fprintf(fout, "\n\t\t\tfor(i = 0; i < a->__size; i++)");
		    fprintf(fout, "\n\t\t\t{\tsoap_peek_element(soap);\n\t\t\t\tif(soap->position)\n\t\t\t\t{\ti = soap->positions[0]-j;\n\t\t\t\t\tif(i < 0 || i >= a->__size)\n\t\t\t\t\t{\tsoap->error = SOAP_IOB;\n\t\t\t\t\t\treturn NULL;\n\t\t\t\t\t}\n\t\t\t\t}");
		    if(is_XML((Tnode *)p->info.typ->ref) && is_string((Tnode *)p->info.typ->ref))
			    fprintf(fout, "\n\t\t\t\tif(!soap_inliteral(soap, NULL, a->%s + i))", ident(p->sym->name));
		    else if(is_XML((Tnode *)p->info.typ->ref) && is_wstring((Tnode *)p->info.typ->ref))
			    fprintf(fout, "\n\t\t\t\tif(!soap_inwliteral(soap, NULL, a->%s + i))", ident(p->sym->name));
		    else
			    fprintf(fout, "\n\t\t\t\tif(!soap_in_%s(soap, NULL, a->%s + i, \"%s\"))",
				    c_ident((Tnode *)p->info.typ->ref), ident(p->sym->name), xsi_type((Tnode *)p->info.typ->ref));
		    fprintf(fout, "\n\t\t\t\t{\tif(soap->error != SOAP_NO_TAG)\n\t\t\t\t\t\treturn NULL;");
		    fprintf(fout, "\n\t\t\t\t\tsoap->error = SOAP_OK;");
		    fprintf(fout, "\n\t\t\t\t\tbreak;");
		    fprintf(fout, "\n\t\t\t\t}"); }
	      fprintf(fout, "\n\t\t\t}\n\t\t}\n\t\telse");
	      fprintf(fout, "\n\t\t{\tif(soap_new_block(soap) == NULL)\n\t\t\t\treturn NULL;");
	      if(p->info.maxOccurs > 1) {
		      if(d)
			      fprintf(fout, "\n\t\t\tfor(a->__size[0] = 0; a->__size[0] <= " SOAP_LONG_FORMAT "; a->__size[0]++)", p->info.maxOccurs);
		      else
			      fprintf(fout, "\n\t\t\tfor(a->__size = 0; a->__size <= " SOAP_LONG_FORMAT "; a->__size++)", p->info.maxOccurs);
	      }
	      else {
			  if(d)
			    fprintf(fout, "\n\t\t\tfor(a->__size[0] = 0; ; a->__size[0]++)");
		    else
			    fprintf(fout, "\n\t\t\tfor(a->__size = 0; ; a->__size++)"); 
		  }
	      fprintf(fout,
		      "\n\t\t\t{\tp = static_cast<%s>(soap_push_block(soap, NULL, sizeof(%s)));\n\t\t\t\tif(!p)\n\t\t\t\t\treturn NULL;",
		      c_type(p->info.typ), c_type((Tnode *)p->info.typ->ref));
	      if(((Tnode *)p->info.typ->ref)->type == Tclass || has_class((Tnode *)p->info.typ->ref))
		      fprintf(fout, "\n\t\t\t\tSOAP_PLACEMENT_NEW(p, %s);", c_type((Tnode *)p->info.typ->ref));
	      if(((Tnode *)p->info.typ->ref)->type == Tclass && !is_external((Tnode *)p->info.typ->ref) &&
		 !is_volatile((Tnode *)p->info.typ->ref) && !is_typedef((Tnode *)p->info.typ->ref))
		      fprintf(fout, "\n\t\t\t\tp->soap_default(soap);");
	      else if(((Tnode *)p->info.typ->ref)->type == Tpointer)
		      fprintf(fout, "\n\t\t\t\t*p = NULL;");
	      else if(!is_XML((Tnode *)p->info.typ->ref))
		      fprintf(fout, "\n\t\t\t\tsoap_default_%s(soap, p);", c_ident((Tnode *)p->info.typ->ref));
	      if(is_XML((Tnode *)p->info.typ->ref) && is_string((Tnode *)p->info.typ->ref))
		      fprintf(fout, "\n\t\t\t\tif(!soap_inliteral(soap, NULL, p))");
	      else if(is_XML((Tnode *)p->info.typ->ref) && is_wstring((Tnode *)p->info.typ->ref))
		      fprintf(fout, "\n\t\t\t\tif(!soap_inwliteral(soap, NULL, p))");
	      else
		      fprintf(fout, "\n\t\t\t\tif(!soap_in_%s(soap, NULL, p, \"%s\"))", c_ident((Tnode *)p->info.typ->ref), xsi_type((Tnode *)p->info.typ->ref));
	      fprintf(fout, "\n\t\t\t\t{\tif(soap->error != SOAP_NO_TAG)\n\t\t\t\t\t\treturn NULL;");
	      fprintf(fout, "\n\t\t\t\t\tsoap->error = SOAP_OK;");
	      fprintf(fout, "\n\t\t\t\t\tbreak;");
	      fprintf(fout, "\n\t\t\t\t}");
	      fprintf(fout, "\n\t\t\t}");
	      fprintf(fout, "\n\t\t\tsoap_pop_block(soap, NULL);");
	      if(p->info.minOccurs > 0)
		      fprintf(fout, "\n\t\t\tif(%sa->__size < " SOAP_LONG_FORMAT ")\n\t\t\t{\tsoap->error = SOAP_OCCURS;\n\t\t\t\treturn NULL;\n\t\t\t}",
			      strict_check(), p->info.minOccurs);
	      if(p->info.maxOccurs > 1)
		      fprintf(fout, "\n\t\t\tif(%sa->__size > " SOAP_LONG_FORMAT ")\n\t\t\t{\tsoap->error = SOAP_OCCURS;\n\t\t\t\treturn NULL;\n\t\t\t}",
			      strict_check(), p->info.maxOccurs);
	      if(((Tnode *)p->info.typ->ref)->type == Tclass || has_class((Tnode *)p->info.typ->ref))
		      fprintf(fout, "\n\t\t\tif(soap->blist->size)\n\t\t\t\ta->%s = soap_new_%s(soap, soap->blist->size/sizeof(%s));\n\t\t\telse\n\t\t\t\ta->%s = NULL;",
			      ident(p->sym->name), c_ident((Tnode *)p->info.typ->ref), c_type((Tnode *)p->info.typ->ref), ident(p->sym->name));
	      else
		      fprintf(fout, "\n\t\t\ta->%s = static_cast<%s>(soap_malloc(soap, soap->blist->size));", ident(p->sym->name), c_type(p->info.typ));
	      fprintf(fout, "\n\t\t\tsoap_save_block(soap, NULL, (char*)a->%s, 1);", ident(p->sym->name));
	      fprintf(fout, "\n\t\t}");
	      fprintf(fout, "\n\t\tif(soap_element_end_in(soap, tag))\n\t\t\treturn NULL;"); }
	if(has_getter(typ))
		fprintf(fout, "\n\t\tif(a->get(soap))\n\t\t\treturn NULL;");
	fprintf(fout, "\n\t}\n\telse {\n\t\t");
	if(is_attachment(typ))
		fprintf(fout,
			"\n#ifndef WITH_LEANER\n\t\tif(*soap->href != '#')\n\t\t{\tif(soap_dime_forward(soap, &a->__ptr, &a->__size, &a->id, &a->type, &a->options))\n\t\t\t\treturn NULL;\n\t\t}\n\t\telse\n#endif\n\t\t\t");
	if(typ->type == Tclass) {
		/*
		fprintf(fout, "a = (%s)soap_id_forward(soap, soap->href, (void*)a, 0, %s, 0, sizeof(%s), 0, soap_copy_%s);",
			c_type_id(typ, "*"), soap_type(typ), c_type(typ), c_ident(typ));
		*/
		fprintf(fout, "a = static_cast<%s>(soap_id_forward(soap, soap->href, a, 0, %s, 0, sizeof(%s), 0, soap_copy_%s));",
			c_type_id(typ, "*"), soap_type(typ), c_type(typ), c_ident(typ));
	}
	else {
		/*
		fprintf(fout, "a = (%s)soap_id_forward(soap, soap->href, (void*)a, 0, %s, 0, sizeof(%s), 0, NULL);",
			c_type_id(typ, "*"), soap_type(typ), c_type(typ));
		*/
		fprintf(fout, "a = static_cast<%s>(soap_id_forward(soap, soap->href, a, 0, %s, 0, sizeof(%s), 0, NULL));",
			c_type_id(typ, "*"), soap_type(typ), c_type(typ));
	}
	fprintf(fout, "\n\t\tif(soap->body && soap_element_end_in(soap, tag))\n\t\t\treturn NULL;");
	fprintf(fout, "\n\t}");
	fprintf(fout, "\n\treturn a;\n}");
}

const char * cstring(const char * s)
{
	size_t n;
	char * t;
	const char * r;
	for(n = 0, r = s; *r; n++, r++)
		if(*r == '"' || *r == '\\')
			n++;
		else if(*r < 32)
			n += 3;
	r = t = (char *)emalloc(n+1);
	for(; *s; s++) {
		if(*s == '"' || *s == '\\') {
			*t++ = '\\';
			*t++ = *s;
		}
		else if(*s < 32) {
			sprintf(t, "\\%03o", (uint)(uchar)*s);
			t += 4;
		}
		else
			*t++ = *s;
	}
	*t = '\0';
	return r;
}

const char * xstring(const char * s)
{
	size_t n;
	char * t;
	const char * r;
	for(n = 0, r = s; *r; n++, r++) {
		if(*r < 32 || *r >= 127)
			n += 4;
		else if(*r == '<' || *r == '>')
			n += 3;
		else if(*r == '&')
			n += 4;
		else if(*r == '"')
			n += 5;
		else if(*r == '\\')
			n += 1;
	}
	r = t = (char *)emalloc(n+1);
	for(; *s; s++) {
		if(*s < 32 || *s >= 127) {
			sprintf(t, "&#%.2x;", (uchar)*s);
			t += 5;
		}
		else if(*s == '<') {
			strcpy(t, "&lt;");
			t += 4;
		}
		else if(*s == '>') {
			strcpy(t, "&gt;");
			t += 4;
		}
		else if(*s == '&') {
			strcpy(t, "&amp;");
			t += 5;
		}
		else if(*s == '"') {
			strcpy(t, "&quot;");
			t += 6;
		}
		else if(*s == '\\') {
			strcpy(t, "\\\\");
			t += 2;
		}
		else
			*t++ = *s;
	}
	*t = '\0';
	return r;
}
