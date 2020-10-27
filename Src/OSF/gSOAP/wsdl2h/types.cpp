/*
        types.cpp

        Generate gSOAP types from XML schemas (e.g. embedded in WSDL).

   --------------------------------------------------------------------------------
   gSOAP XML Web services tools
   Copyright (C) 2001-2009, Robert van Engelen, Genivia Inc. All Rights Reserved.
   This software is released under one of the following two licenses:
   GPL or Genivia's license for commercial use.
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
   --------------------------------------------------------------------------------
   A commercial use license is available from Genivia, Inc., contact@genivia.com
   --------------------------------------------------------------------------------
 */
#include <slib.h>
#include "wsdlH.h"
#pragma hdrstop
#include "types.h"

static char * getline(char * s, size_t n, FILE * fd);
static const char * nonblank(const char * s);
static const char * fill(char * t, int n, const char * s, int e);
static const char * utf8(char * t, const char * s);
static const char * FASTCALL cstring(const char * s);
static const char * FASTCALL xstring(const char * s);
static bool is_integer(const char * s);
static LONG64 to_integer(const char * s);
static void documentation(const char * text);

static int comment_nest = 0; /* keep track of block comments to avoid nesting */

////////////////////////////////////////////////////////////////////////////////
//
//	Keywords and reserved words
//
////////////////////////////////////////////////////////////////////////////////

static const char * keywords[] = { 
	"and",
	"asm",
	"auto",
	"bool",
	"break",
	"case",
	"catch",
	"char",
	"class",
	"const",
	"const_cast",
	"continue",
	"default",
	"delete",
	"do",
	"double",
	"dynamic_cast",
	"else",
	"enum",
	"errno",
	"explicit",
	"export",
	"extern",
	"false",
	"FILE",
	"float",
	"for",
	"friend",
	"goto",
	"if",
	"inline",
	"int",
	"interface",
	"long",
	"LONG64",
	"max",
	"min",
	"mustUnderstand",
	"mutable",
	"namespace",
	"new",
	"not",
	"NULL",
	"operator",
	"or",
	"private",
	"protected",
	"public",
	"_QName",
	"register",
	"reinterpret_cast",
	"restrict",
	"return",
	"short",
	"signed",
	"size_t",
	"sizeof",
	"static",
	"static_cast",
	"struct",
	"switch",
	"template",
	"this",
	"throw",
	"time_t",
	"true",
	"typedef",
	"typeid",
	"typeof",
	"typename",
	"ULONG64",
	"union",
	"unsigned",
	"using",
	"virtual",
	"void",
	"volatile",
	"wchar_t",
	"while",
	"XML",
	"_XML",
	"xor", 
};
//
//	Types methods
//
Types::Types()
{
	init();
}

int Types::read(const char * file)
{
	FILE * fd;
	char buf[1024], xsd[1024], def[1024], use[1024], ptr[1024], uri[1024];
	const char * s;
	short copy = 0;
	strcpy(buf, file);
	fd = fopen(buf, "r");
	if(!fd && import_path) {
		strcpy(buf, import_path);
		strcat(buf, "/");
		strcat(buf, file);
		fd = fopen(buf, "r");
	}
	if(!fd) {
		fprintf(stderr, "Cannot open file '%s'\n", buf);
		return SOAP_EOF;
	}
	fprintf(stderr, "Reading type definitions from type map file '%s'\n", buf);
	while(getline(buf, sizeof(buf), fd)) {
		s = buf;
		if(copy) {
			if(*s == ']')
				copy = 0;
			else
				fprintf(stream, "%s\n", buf);
		}
		else if(*s == '[')
			copy = 1;
		else if(*s == '<') {
			s = fill(uri, sizeof(uri), s+1, -1);
			infile[infiles++] = estrdup(uri);
			if(infiles >= MAXINFILES) {
				fprintf(stderr, "wsdl2h: too many files\n");
				exit(1);
			}
		}
		else if(*s == '>') {
			s = fill(uri, sizeof(uri), s+1, -1);
			if(!outfile) {
				outfile = estrdup(uri);
				stream = fopen(outfile, "w");
				if(!stream) {
					fprintf(stderr, "Cannot write to %s\n", outfile);
					exit(1);
				}
				if(cppnamespace)
					fprintf(stream, "namespace %s {\n", cppnamespace);
				fprintf(stderr, "Saving %s\n\n", outfile);
			}
		}
		else if(*s && *s != '#') {
			s = fill(xsd, sizeof(xsd), s, '=');
			if(strstr(xsd, "__")) {
				s = fill(def, sizeof(def), s, '|');
				s = fill(use, sizeof(use), s, '|');
				s = fill(ptr, sizeof(ptr), s, '|');
				if(*xsd) {
					s = estrdup(xsd);
					if(*def == '$') {
						const char * t = modtypemap[s];
						if(t) {
							char * r = static_cast<char *>(emalloc(strlen(t)+strlen(def)+1));
							strcpy(r, t);
							strcat(r, def);
							free((void *)modtypemap[s]);
							modtypemap[s] = r;
						}
						else
							modtypemap[s] = estrdup(def);
					}
					else {
						if(*def)
						      deftypemap[s] = estrdup(def);
					      else
						      deftypemap[s] = "";
					      if(*use)
						      usetypemap[s] = estrdupf(use);
					      else
						      usetypemap[s] = estrdupf(xsd);
					      if(*ptr)
						      ptrtypemap[s] = estrdupf(ptr); }
				}
			}
			else if(*xsd) {
				s = fill(uri, sizeof(uri), s, 0);
				if(uri[0] == '"') {
					uri[strlen(uri)-1] = '\0';
					nsprefix(xsd, estrdup(uri+1));
				}
				else if(uri[0] == '<') {
					uri[strlen(uri)-1] = '\0';
					char * s = estrdup(uri+1);
					nsprefix(xsd, s);
					exturis.insert(s);
				}
				else
					nsprefix(xsd, estrdup(uri));
			}
		}
	}
	fclose(fd);
	return SOAP_OK;
}

void Types::init()
{
	snum = 1;
	unum = 1;
	gnum = 1;
	with_union = false;
	fake_union = false;
	knames.insert(keywords, keywords+sizeof(keywords)/sizeof(char *));
	if(cflag) {
		deftypemap["xsd__ur_type"] = "";
		if(dflag) {
			usetypemap["xsd__ur_type"] = "xsd__anyType";
			ptrtypemap["xsd__ur_type"] = "xsd__anyType*";
		}
		else {usetypemap["xsd__ur_type"] = "_XML";
		      ptrtypemap["xsd__ur_type"] = "_XML"; }
	}
	else {deftypemap["xsd__ur_type"] = "class xsd__ur_type { _XML __item; struct soap *soap; };";
	      usetypemap["xsd__ur_type"] = "xsd__ur_type"; }
	if(cflag) {
		deftypemap["xsd__anyType"] = "";
		if(dflag) {
			usetypemap["xsd__anyType"] = "xsd__anyType";
			ptrtypemap["xsd__anyType"] = "xsd__anyType*";
		}
		else {usetypemap["xsd__anyType"] = "_XML";
		      ptrtypemap["xsd__anyType"] = "_XML"; }
	}
	else {
		if(dflag) {
		      deftypemap["xsd__anyType"] = "";
		      usetypemap["xsd__anyType"] = "xsd__anyType";
		      ptrtypemap["xsd__anyType"] = "xsd__anyType*";
	      }
	      else {
			  deftypemap["xsd__anyType"] = "class xsd__anyType { _XML __item; struct soap *soap; };";
		    usetypemap["xsd__anyType"] = "xsd__anyType*"; 
		  }
	}
	deftypemap["xsd__any"] = "";
	if(dflag) {
		usetypemap["xsd__any"] = "xsd__anyType";
		ptrtypemap["xsd__any"] = "xsd__anyType*";
	}
	else {
		usetypemap["xsd__any"] = "_XML";
	      ptrtypemap["xsd__any"] = "_XML"; 
	}
	deftypemap["xsd__anyAttribute"] = "";
	if(dflag) {
		usetypemap["xsd__anyAttribute"] = "xsd__anyAttribute";
		ptrtypemap["xsd__anyAttribute"] = "xsd__anyAttribute*";
	}
	else {
		usetypemap["xsd__anyAttribute"] = "_XML";
	      ptrtypemap["xsd__anyAttribute"] = "_XML"; 
	}
	if(cflag) {
		deftypemap["xsd__base64Binary"] =
		        "struct xsd__base64Binary\n{\tunsigned char *__ptr;\n\tint __size;\n\tchar *id, *type, *options; // NOTE: for DIME and MTOM XOP attachments only\n};";
		usetypemap["xsd__base64Binary"] = "struct xsd__base64Binary";
	}
	else {
		deftypemap["xsd__base64Binary"] =
		      "class xsd__base64Binary\n{\tunsigned char *__ptr;\n\tint __size;\n\tchar *id, *type, *options; // NOTE: for DIME and MTOM XOP attachments only\n\tstruct soap *soap;\n};";
	      usetypemap["xsd__base64Binary"] = "xsd__base64Binary"; 
	}
	if(cflag) {
		if(eflag)
			deftypemap["xsd__boolean"] = "enum xsd__boolean { false_, true_ };";
		else
			deftypemap["xsd__boolean"] = "enum xsd__boolean { xsd__boolean__false_, xsd__boolean__true_ };";
		usetypemap["xsd__boolean"] = "enum xsd__boolean";
	}
	else {
		deftypemap["xsd__boolean"] = "";
	      usetypemap["xsd__boolean"] = "bool"; 
	}
	deftypemap["xsd__byte"] = "typedef char xsd__byte;";
	usetypemap["xsd__byte"] = "xsd__byte";
	deftypemap["xsd__dateTime"] = "";
	usetypemap["xsd__dateTime"] = "time_t";
	deftypemap["xsd__double"] = "";
	usetypemap["xsd__double"] = "double";
	deftypemap["xsd__float"] = "";
	usetypemap["xsd__float"] = "float";
	if(cflag) {
		deftypemap["xsd__hexBinary"] = "struct xsd__hexBinary { unsigned char *__ptr; int __size; };";
		usetypemap["xsd__hexBinary"] = "struct xsd__hexBinary";
	}
	else {deftypemap["xsd__hexBinary"] = "class xsd__hexBinary { unsigned char *__ptr; int __size; };";
	      usetypemap["xsd__hexBinary"] = "xsd__hexBinary"; }
	deftypemap["xsd__int"] = "";
	usetypemap["xsd__int"] = "int";
	deftypemap["xsd__long"] = "";
	usetypemap["xsd__long"] = "LONG64";
	deftypemap["xsd__short"] = "";
	usetypemap["xsd__short"] = "short";
	if(cflag || sflag) {
		deftypemap["xsd__string"] = "";
		usetypemap["xsd__string"] = "char*";
	}
	else {deftypemap["xsd__string"] = "";
	      usetypemap["xsd__string"] = "std::string"; }
	if(cflag || sflag) {
		deftypemap["xsd__QName"] = "";
		usetypemap["xsd__QName"] = "_QName";
		ptrtypemap["xsd__QName"] = "_QName";
	}
	else {deftypemap["xsd__QName"] = "typedef std::string xsd__QName;";
	      usetypemap["xsd__QName"] = "xsd__QName"; }
	deftypemap["xsd__unsignedByte"] = "typedef unsigned char xsd__unsignedByte;";
	usetypemap["xsd__unsignedByte"] = "xsd__unsignedByte";
	deftypemap["xsd__unsignedInt"] = "";
	usetypemap["xsd__unsignedInt"] = "unsigned int";
	deftypemap["xsd__unsignedLong"] = "";
	usetypemap["xsd__unsignedLong"] = "ULONG64";
	deftypemap["xsd__unsignedShort"] = "";
	usetypemap["xsd__unsignedShort"] = "unsigned short";
	if(cflag) {
		deftypemap["SOAP_ENC__base64Binary"] =
		        "struct SOAP_ENC__base64Binary { unsigned char *__ptr; int __size; };";
		usetypemap["SOAP_ENC__base64Binary"] = "struct SOAP_ENC__base64Binary";
		deftypemap["SOAP_ENC__base64"] = "struct SOAP_ENC__base64 { unsigned char *__ptr; int __size; };";
		usetypemap["SOAP_ENC__base64"] = "struct SOAP_ENC__base64";
	}
	else {deftypemap["SOAP_ENC__base64Binary"] =
		      "class SOAP_ENC__base64Binary { unsigned char *__ptr; int __size; };";
	      usetypemap["SOAP_ENC__base64Binary"] = "SOAP_ENC__base64Binary";
	      deftypemap["SOAP_ENC__base64"] = "class SOAP_ENC__base64 { unsigned char *__ptr; int __size; };";
	      usetypemap["SOAP_ENC__base64"] = "SOAP_ENC__base64"; }
	if(cflag) {
		deftypemap["SOAP_ENC__boolean"] = "enum SOAP_ENC__boolean { false_, true_ };";
		usetypemap["SOAP_ENC__boolean"] = "enum SOAP_ENC__boolean";
	}
	else {deftypemap["SOAP_ENC__boolean"] = "typedef bool SOAP_ENC__boolean;";
	      usetypemap["SOAP_ENC__boolean"] = "SOAP_ENC__boolean"; }
	deftypemap["SOAP_ENC__byte"] = "typedef char SOAP_ENC__byte;";
	usetypemap["SOAP_ENC__byte"] = "SOAP_ENC__byte";
	deftypemap["SOAP_ENC__dateTime"] = "typedef time_t SOAP_ENC__dateTime;";
	usetypemap["SOAP_ENC__dateTime"] = "SOAP_ENC__dateTime";
	deftypemap["SOAP_ENC__double"] = "typedef double SOAP_ENC__double;";
	usetypemap["SOAP_ENC__double"] = "SOAP_ENC__double";
	deftypemap["SOAP_ENC__float"] = "typedef float SOAP_ENC__float";
	usetypemap["SOAP_ENC__float"] = "SOAP_ENC__float";
	if(cflag) {
		deftypemap["SOAP_ENC__hexBinary"] = "struct SOAP_ENC__hexBinary { unsigned char *__ptr; int __size; };";
		usetypemap["SOAP_ENC__hexBinary"] = "struct SOAP_ENC__hexBinary";
	}
	else {deftypemap["SOAP_ENC__hexBinary"] = "class SOAP_ENC__hexBinary { unsigned char *__ptr; int __size; };";
	      usetypemap["SOAP_ENC__hexBinary"] = "SOAP_ENC__hexBinary"; }
	deftypemap["SOAP_ENC__int"] = "typedef int SOAP_ENC__int;";
	usetypemap["SOAP_ENC__int"] = "SOAP_ENC__int";
	deftypemap["SOAP_ENC__long"] = "typedef LONG64 SOAP_ENC__long;";
	usetypemap["SOAP_ENC__long"] = "SOAP_ENC__long";
	deftypemap["SOAP_ENC__short"] = "typedef short SOAP_ENC__short;";
	usetypemap["SOAP_ENC__short"] = "SOAP_ENC__short";
	if(cflag || sflag) {
		deftypemap["SOAP_ENC__string"] = "";
		usetypemap["SOAP_ENC__string"] = "char*";
	}
	else {
		deftypemap["SOAP_ENC__string"] = "";
	      usetypemap["SOAP_ENC__string"] = "std::string"; }
	deftypemap["SOAP_ENC__unsignedByte"] = "typedef unsigned char SOAP_ENC__unsignedByte;";
	usetypemap["SOAP_ENC__unsignedByte"] = "SOAP_ENC__unsignedByte";
	deftypemap["SOAP_ENC__unsignedInt"] = "typedef unsigned int SOAP_ENC__unsignedInt;";
	usetypemap["SOAP_ENC__unsignedInt"] = "SOAP_ENC__unsignedInt";
	deftypemap["SOAP_ENC__unsignedLong"] = "typedef ULONG64 SOAP_ENC__unsignedLong;";
	usetypemap["SOAP_ENC__unsignedLong"] = "SOAP_ENC__unsignedLong";
	deftypemap["SOAP_ENC__unsignedShort"] = "typedef unsigned short SOAP_ENC__unsignedShort;";
	usetypemap["SOAP_ENC__unsignedShort"] = "SOAP_ENC__unsignedShort";
	deftypemap["SOAP_ENC__Array"] = "";
	usetypemap["SOAP_ENC__Array"] = "struct { _XML *__ptr; int __size; }";
	deftypemap["_SOAP_ENC__arrayType"] = "";
	deftypemap["SOAP_ENV__Header"] = "";
	usetypemap["SOAP_ENV__Header"] = "struct SOAP_ENV__Header";
	deftypemap["_SOAP_ENV__mustUnderstand"] = "";
	if(cflag || sflag)
		usetypemap["_SOAP_ENV__mustUnderstand"] = "char*";
	else
		usetypemap["_SOAP_ENV__mustUnderstand"] = "std::string";
	deftypemap["SOAP_ENV__Fault"] = "";
	usetypemap["SOAP_ENV__Fault"] = "struct SOAP_ENV__Fault";
	deftypemap["SOAP_ENV__detail"] = "";
	usetypemap["SOAP_ENV__detail"] = "struct SOAP_ENV__Detail";
	deftypemap["SOAP_ENV__Detail"] = "";
	usetypemap["SOAP_ENV__Detail"] = "struct SOAP_ENV__Detail";
	deftypemap["SOAP_ENV__Code"] = "";
	usetypemap["SOAP_ENV__Code"] = "struct SOAP_ENV__Code";
	deftypemap["SOAP_ENV__Reason"] = "";
	usetypemap["SOAP_ENV__Reason"] = "struct SOAP_ENV__Reason";
	if(read(mapfile))
		fprintf(stderr,
			"Problem reading type map file '%s'.\nUsing internal type definitions for %s instead.\n\n",
			mapfile,
			cflag ? "C" : "C++");
}

const char * Types::nsprefix(const char * prefix, const char * URI)
{
	if(URI) {
		const char * s = uris[URI];
		if(!s) {
			size_t n;
			if(!prefix || !*prefix || *prefix == '_')
				s = schema_prefix;
			else
				s = estrdup(prefix);
			if(!syms[s])
				n = syms[s] = 1;
			else
				n = ++syms[s];
			if(n != 1 || !prefix || !*prefix || *prefix == '_') {
				char * t = static_cast<char *>(emalloc(strlen(s)+16));
				sprintf(t, "%s%lu", s, (unsigned long)n);
				s = t;
			}
			uris[URI] = s;
			if(vflag)
				fprintf(stderr, "namespace prefix %s = \"%s\"\n", s, URI);
		}
		// if *prefix == '_', then add prefix string to s
		if(prefix && *prefix == '_') {
			char * t = static_cast<char *>(emalloc(strlen(s)+2));
			*t = '_';
			strcpy(t+1, s);
			s = t;
		}
		return s;
	}
	return NULL;
}

// Find a C name for a QName. If the name has no qualifier, use URI. Suggest prefix for URI
const char * Types::fname(const char * prefix, const char * URI, const char * qname, SetOfString * reserved,
	enum Lookup lookup, bool isqname)
{
	char buf[1024], * t;
	const char * p, * s, * name;
	if(!qname) {
		fprintf(stream, "// Warning: internal error, no QName in fname()\n");
		if(vflag)
			fprintf(stderr, "Internal error, no QName in fname()\n");
		qname = "?";
	}
	name = qname;
	if(isqname)
		s = strrchr(name, ':');
	else
		s = NULL;
	if(s) {
		name = s+1;
		if(qname[0] == '"' && qname[1] == '"')
			s = NULL;
		else if(*qname == '"') {
			t = static_cast<char *>(emalloc(s-qname-1));
			strncpy(t, qname+1, s-qname-2);
			t[s-qname-2] = '\0';
			URI = t;
		}
		else if(!strncmp(qname, "xs:", 3)) { // this hack is necessary since the nsmap table defines "xs" for "xsd"
			s = "xsd";
			URI = NULL;
		}
		else {
			t = static_cast<char *>(emalloc(s-qname+1));
		      strncpy(t, qname, s-qname);
		      t[s-qname] = '\0';
		      s = t;
		      URI = NULL; 
		}
	}
	if(URI)
		p = nsprefix(prefix, URI);
	else if(s)
		p = s;
	else
		p = "";
	if(lookup == LOOKUP) {
		s = qnames[Pair(p, name)];
		if(s)
			return s;
	}
	t = buf;
	if(!prefix || *prefix) {
		s = p;
		// no longer add '_' when URI != NULL, since nsprefix() will do this
		if(prefix && *prefix == ':')
			*t++ = ':';
		else if(prefix && *prefix == '_') {
			if(!URI)
				*t++ = '_';
			if(prefix[1] == '_') { // ensures ns prefix starts with __
				strcpy(t, prefix+1);
				t += strlen(prefix+1);
			}
		}
		if(s && *s) {
			for(; *s; s++) {
				if(isalnum(*s))
					*t++ = *s;
				else if(*s == '-' && s != p)
					*t++ = '_';
				else if(*s == '_') {
					if(s == p)
						*t++ = '_';
					else if(!_flag) {
						strcpy(t, "_USCORE");
						t += 7;
					}
					else {s = utf8(t, s);
					      t += 6; }
				}
				else {s = utf8(t, s);
				      t += 6; }
			}
			if(!prefix || *prefix != '*') {
				*t++ = '_';
				*t++ = '_';
			}
		}
		else if(isdigit(*name))
			*t++ = '_';
	}
	for(s = name; *s; s++) {
		if(isalnum(*s))
			*t++ = *s;
		else if(*s == '-' && s[1] != '\0' && s != name)
			*t++ = '_';
		else if(!_flag && *s == '_') {
			strcpy(t, "_USCORE");
			t += 7;
		}
		else {s = utf8(t, s);
		      t += 6; }
		if(t >= buf+sizeof(buf))
			break;
	}
	*t = '\0';
	while(knames.find(buf) != knames.end() || (reserved && reserved->find(buf) != reserved->end())) {
		*t++ = '_';
		*t = '\0';
	}
	if(isalpha(*buf) || *buf == '_' || *buf == ':') {
		t = static_cast<char *>(emalloc(strlen(buf)+1));
		strcpy(t, buf);
	}
	else {
		t = static_cast<char *>(emalloc(strlen(buf)+2));
      *t = '_';
      strcpy(t+1, buf); 
	}
	if(lookup == LOOKUP) {
		qnames[Pair(p, name)] = t;
		if(vflag)
			cerr<<"Mapping '"<<p<<":"<<name<<"' to '"<<t<<"'"<<endl;
		/*
		   for (MapOfPairToString::const_iterator i = qnames.begin(); i != qnames.end(); ++i)
		   cerr << "(" << (*i).first.first << "," << (*i).first.second << ") = " << (*i).second << endl;
		 */
	}
	return t;
}

bool Types::is_defined(const char * prefix, const char * URI, const char * qname)
{
	const char * t = fname(prefix, URI, qname, NULL, LOOKUP, true);
	return usetypemap.find(t) != usetypemap.end();
}

const char * Types::aname(const char * prefix, const char * URI, const char * qname)
{
	return fname(prefix, URI, qname, NULL, NOLOOKUP, true);
}

const char * Types::cname(const char * prefix, const char * URI, const char * qname)
{
	return fname(prefix, URI, qname, NULL, LOOKUP, true);
}

const char * Types::tname(const char * prefix, const char * URI, const char * qname)
{
	const char * s;
	const char * t = cname(prefix, URI, qname);
	if(usetypemap.find(t) != usetypemap.end())
		s = usetypemap[t];
	else {
		s = t;
	      fprintf(stream,
		      "\n// Warning: undefined QName '%s' for type '%s' in namespace '%s' (FIXME: check WSDL and schema definitions)\n",
		      qname ? qname : "", t, URI ? URI : "?");
	      if(vflag)
		      fprintf(stderr, "Warning: undefined QName '%s' for type '%s' in namespace '%s'\n",
			      qname ? qname : "", t,
			      URI ? URI : "?"); 
	}
	return s;
}

const char * Types::tnameptr(bool flag, const char * prefix, const char * URI, const char * qname)
{
	const char * s = pname(flag, prefix, URI, qname);
	if(flag) {
		if(!strncmp(s, "char*", 5))
			return "char**";
		if(!strchr(s, '*')) {
			char * r = static_cast<char *>(emalloc(strlen(s)+2));
			strcpy(r, s);
			strcat(r, "*");
			return r;
		}
	}
	return s;
}

const char * Types::pname(bool flag, const char * prefix, const char * URI, const char * qname)
{
	const char * r, * s = NULL;
	const char * t = cname(prefix, URI, qname);
	if(flag) {
		if(ptrtypemap.find(t) != ptrtypemap.end())
			s = ptrtypemap[t];
		else {
			if(usetypemap.find(t) != usetypemap.end())
			      s = usetypemap[t];
		      if(!s) {
			      s = t;
			      fprintf(stream, "\n// Warning: undefined QName '%s' for pointer to type '%s' (FIXME: check WSDL and schema definitions)\n",
				      qname, t);
			      if(vflag)
				      fprintf(stderr, "Warning: undefined QName '%s' for pointer to type '%s' in namespace '%s'\n",
					      qname, t, URI ? URI : "?");
		      }
		      r = s;
		      while(r && *r) {
			      r = strchr(r+1, '*');
			      if(r && *(r-1) != '/' && *(r+1) != '/')
				      break;
		      }
		      if(!r) { // already pointer?
			      char * p = static_cast<char *>(emalloc(strlen(s)+2));
			      strcpy(p, s);
			      strcat(p, "*");
			      s = p;
		      }
		      if(vflag)
			      cerr<<"Mapping pointer to '"<<t<<"' to '"<<s<<"'"<<endl;
		      ptrtypemap[t] = s; }
	}
	else if(usetypemap.find(t) != usetypemap.end())
		s = usetypemap[t];
	else {
		s = t;
	      fprintf(stream,
		      "\n// Warning: undefined QName '%s' for type '%s' in namespace '%s' (FIXME: check WSDL and schema definitions)\n",
		      qname, t, URI ? URI : "?");
	      if(vflag)
		      fprintf(stderr, "Warning: undefined QName '%s' for type '%s' in namespace '%s'\n", qname, t,
			      URI ? URI : "?"); }
	return s;
}

const char * Types::deftname(enum Type type, const char * pointer, bool is_pointer, const char * prefix,
	const char * URI, const char * qname)
{
	char buf[1024];
	char * s;
	const char * q = NULL;
	const char * t = fname(prefix, URI, qname, NULL, LOOKUP, true);
	if(deftypemap[t]) {
		if(vflag)
			fprintf(stderr, "Name %s already defined (probably in %s file)\n", qname, mapfile);
		return NULL;
	}
	switch(type) {
	    case ENUM:
		q = "enum";
		if(yflag)
			knames.insert(t);
		break;
	    case STRUCT:
		q = "struct";
		if(yflag)
			knames.insert(t);
		break;
	    case CLASS:
	    case TYPEDEF:
		knames.insert(t);
	    default:
		break;
	}
	if(q) {
		strcpy(buf, q);
		strcat(buf, " ");
	}
	else
		buf[0] = '\0';
	strcat(buf, t);
	if(pointer)
		strcat(buf, pointer);
	s = static_cast<char *>(emalloc(strlen(buf)+1));
	strcpy(s, buf);
	usetypemap[t] = s;
	if(pointer || is_pointer)
		ptrtypemap[t] = s;
	if(vflag)
		cerr<<"Defined '"<<t<<"' ('"<<qname<<"' in namespace '"<<(URI ? URI : prefix ? prefix : "")<<
		"') as '"<<s<<endl;
	return t;
}

// get enumeration value. URI/type refers to the enum simpleType.
const char * Types::ename(const char * type, const char * value, bool isqname)
{
	const char * s = enames[Pair(type, value)];
	if(!s) {
		s = fname(NULL, NULL, value, &rnames, NOLOOKUP, isqname);
		if(!eflag && type && *type) { // Add prefix to enum
			if(!*s || (s[0] == '_' && s[1] == '\0'))
				s = "_x0000";
			char * buf = static_cast<char *>(emalloc(strlen(type)+strlen(s)+3));
			// _xXXXX is OK here
			if(s[0] == '_' && s[1] != 'x' && strncmp(s, "_USCORE", 7))
				sprintf(buf, "%s_%s", type, s);
			else
				sprintf(buf, "%s__%s", type, s);
			s = buf;
		}
		else
			rnames.insert(s);
		enames[Pair(type, value)] = s;
	}
	return s;
}

// get operation name
const char * Types::oname(const char * prefix, const char * URI, const char * qname)
{
	const char * s = fname(prefix, URI, qname, NULL, LOOKUP, true);
	if(s && usetypemap.find(s) != usetypemap.end()) { // Avoid name clash with structs/classes of the same name
		onames.insert(s);
	}
	s = fname(prefix, URI, qname, &onames, NOLOOKUP, true);
	onames.insert(s);
	return s;
}

// generate struct name
const char * Types::sname(const char * URI, const char * name)
{
	const char * s;
	char * t;
	if(!aflag && name) {
		size_t len = 0;
		for(VectorOfString::const_iterator i = scope.begin(); i != scope.end(); ++i)
			len += strlen(*i)+1;
		t = static_cast<char *>(emalloc(len+strlen(name)+1));
		*t = '\0';
		for(VectorOfString::const_iterator j = scope.begin(); j != scope.end(); ++j) {
			strcat(t, *j);
			strcat(t, "-");
		}
		strcat(t, name);
		s = fname("_", URI, t, &rnames, NOLOOKUP, true);
		rnames.insert(s);
	}
	else if(URI) {
		s = nsprefix(NULL, URI);
		t = static_cast<char *>(emalloc(strlen(s)+16));
		sprintf(t, "_%s__struct_%d", s, snum++);
		s = t;
	}
	else {
		t = static_cast<char *>(emalloc(16));
	      sprintf(t, "struct_%d", snum++);
	      s = t; 
	}
	return s;
}

// generate union name
const char * Types::uname(const char * URI)
{
	const char * s;
	char * t;
	if(!aflag) {
		size_t len = 0;
		for(VectorOfString::const_iterator i = scope.begin(); i != scope.end(); ++i)
			len += strlen(*i)+1;
		t = static_cast<char *>(emalloc(len+6));
		strcpy(t, "union");
		for(VectorOfString::const_iterator j = scope.begin(); j != scope.end(); ++j) {
			strcat(t, "-");
			strcat(t, *j);
		}
		s = fname("_", URI, t, &rnames, NOLOOKUP, true);
		rnames.insert(s);
	}
	else if(URI) {
		s = nsprefix(NULL, URI);
		t = static_cast<char *>(emalloc(strlen(s)+16));
		sprintf(t, "_%s__union_%d", s, unum++);
		s = t;
	}
	else {
		t = static_cast<char *>(emalloc(16));
	      sprintf(t, "_union_%d", unum++);
	      s = t; 
	}
	return s;
}

// generate enum name
const char * Types::gname(const char * URI, const char * name)
{
	const char * s;
	char * t;
	if(!aflag && name) {
		size_t len = 0;
		for(VectorOfString::const_iterator i = scope.begin(); i != scope.end(); ++i)
			len += strlen(*i)+1;
		t = static_cast<char *>(emalloc(len+strlen(name)+1));
		*t = '\0';
		for(VectorOfString::const_iterator j = scope.begin(); j != scope.end(); ++j) {
			strcat(t, *j);
			strcat(t, "-");
		}
		strcat(t, name);
		s = fname("_", URI, t, &rnames, LOOKUP, true);
		rnames.insert(s);
	}
	else if(URI) {
		s = nsprefix(NULL, URI);
		t = static_cast<char *>(emalloc(strlen(s)+16));
		sprintf(t, "_%s__enum_%d", s, gnum++);
		s = t;
	}
	else {
		t = static_cast<char *>(emalloc(16));
	      sprintf(t, "enum_%d", gnum++);
	      s = t; 
	}
	return s;
}

// checks if nillable or minOccurs=0 (and no default value is present)
bool Types::is_nillable(const xs__element & element)
{
	return !element.default_ && (element.nillable || (element.minOccurs && sstreq(element.minOccurs, "0")));
}

bool Types::is_basetypeforunion(const char * prefix, const char * URI, const char * type)
{
	const char * t = tname(prefix, URI, type);
	if(sstreq(t, "std::string") || sstreq(t, "std::wstring"))
		return false;
	return is_basetype(prefix, URI, type);
}

bool Types::is_basetype(const char * prefix, const char * URI, const char * type)
{
	const char * t = tname(prefix, URI, type);
	if(!strncmp(t, "enum ", 5))
		return true;
	if(strstr(t, "__") && strcmp(t, "xsd__byte"))
		return false;
	return !strncmp(type, "xs:", 3) || !strncmp(type, "SOAP-ENC:", 9);
}

void Types::dump(FILE * fd)
{
	fprintf(fd, "\nTypes:\n");
	for(MapOfStringToString::const_iterator i = usetypemap.begin(); i != usetypemap.end(); ++i)
		fprintf(fd, "%s=%s\n", (*i).first, (*i).second ? (*i).second : "(null)");
	fprintf(fd, "\nPointers:\n");
	for(MapOfStringToString::const_iterator j = ptrtypemap.begin(); j != ptrtypemap.end(); ++j)
		fprintf(fd, "%s=%s\n", (*j).first, (*j).second ? (*j).second : "(null)");
}

void Types::define(const char * URI, const char * name, const xs__complexType & complexType)
{ // generate prototype for structs/classes and store name
	const char * prefix = NULL;
	if(complexType.name)
		name = complexType.name;
	else
		prefix = "_";
	if(complexType.complexContent && complexType.complexContent->restriction && sstreq(complexType.complexContent->restriction->base, "SOAP-ENC:Array")) {
		if(strcmp(schema_prefix, "ns"))
			prefix = "*";
		else
			prefix = "";
	}
	if(cflag) {
		const char * t = deftname(STRUCT, "*", true, prefix, URI, name);
		if(t) {
			if(yflag)
				fprintf(stream, "\n/// Typedef synonym for struct %s.\ntypedef struct %s %s;\n", t, t,
					t);
		}
		else if(name) {
			t = deftypemap[cname(prefix, URI, name)];
			if(t) {
				fprintf(stream, "\n/// Imported complexType \"%s\":%s from typemap %s.\n", URI, name, mapfile ? mapfile : "");
				document(complexType.annotation);
				if(*t)
					format(t);
				else
					fprintf(stream, "// complexType definition intentionally left blank.\n");
			}
		}
	}
	else {
		const char * t = deftname(CLASS, "*", true, prefix, URI, name);
		if(t) {
			//fprintf(stream, "\n// Forward declaration of class %s.\nclass %s;\n", t, t);
			fprintf(stream, "class %s; // Forward declaration of class %s\n", t, t);
		}
		else if(name) {
			t = deftypemap[cname(prefix, URI, name)];
			if(t) {
				fprintf(stream, "\n/// Imported complexType \"%s\":%s from typemap %s.\n", URI, name, mapfile ? mapfile : "");
				document(complexType.annotation);
				if(*t)
					format(t);
				else
					fprintf(stream, "// complexType definition intentionally left blank.\n");
			}
		}
	}
}

void Types::gen(const char * URI, const char * name, const xs__simpleType & simpleType, bool anonymous)
{
	const char * t = NULL;
	const char * prefix = NULL;
	if(simpleType.name)
		name = simpleType.name;
	else
		prefix = "_";
	if(!anonymous) {
		t = deftypemap[cname(NULL, URI, name)];
		if(t) {
			fprintf(stream, "\n/// Imported simpleType \"%s\":%s from typemap %s.\n", URI, name,
				mapfile ? mapfile : "");
			document(simpleType.annotation);
			if(*t)
				format(t);
			else
				fprintf(stream, "// simpleType definition intentionally left blank.\n");
			return;
		}
	}
	if(simpleType.restriction && simpleType.restriction->base) {
		const char * base = simpleType.restriction->base;
		const char * baseURI = NULL;
		if(simpleType.restriction->simpleTypePtr() && simpleType.restriction->simpleTypePtr()->schemaPtr())
			baseURI = simpleType.restriction->simpleTypePtr()->schemaPtr()->targetNamespace;
		if(!anonymous)
			fprintf(stream, "\n/// \"%s\":%s is a simpleType restriction of %s.\n", URI ? URI : "", name,
				base);
		document(simpleType.annotation);
		document(simpleType.restriction->annotation);
		if(!simpleType.restriction->enumeration.empty()) {
			bool is_numeric = true; // check if all enumeration values are numeric
			bool is_qname = sstreq(base, "xs:QName") ? true : false;
			if(!anonymous) {
				t = deftname(ENUM, NULL, false, prefix, URI, name);
				if(t && !eflag)
					fprintf(stream,
						"/// Note: enum values are prefixed with '%s' to avoid name clashes, please use wsdl2h option -e to omit this prefix\n",
						t);
			}
			SETIFZ(t, gname(URI, name));
			if(!anonymous)
				fprintf(stream, "enum %s\n{\n", t);
			else
				fprintf(stream, "    enum %s\n    {\n", t);
			for(vector <xs__enumeration>::const_iterator enumeration1 = simpleType.restriction->enumeration.begin();
			    enumeration1 != simpleType.restriction->enumeration.end(); ++enumeration1) {
				const char * s;
				if((s = (*enumeration1).value))
					is_numeric &= is_integer(s);
			}
			SetOfString enumvals;
			for(vector <xs__enumeration>::const_iterator enumeration2 =
			            simpleType.restriction->enumeration.begin();
			    enumeration2 != simpleType.restriction->enumeration.end(); ++enumeration2) {
				const char * s;
				document((*enumeration2).annotation);
				if((s = (*enumeration2).value)) {
					if(!enumvals.count(s)) {
						enumvals.insert(s);
						if(is_numeric)
							fprintf(stream, "\t%s = %s,\t///< %s value=\"%s\"\n", ename(t, s, false), s, base, s);
						else if(is_qname && (*enumeration2).value_)
							fprintf(stream, "\t%s,\t///< %s value=\"%s\"\n", ename(t, (*enumeration2).value_, true), base, (*enumeration2).value_);
						else
							fprintf(stream, "\t%s,\t///< %s value=\"%s\"\n", ename(t, s, false), base, s);
					}
				}
				else
					fprintf(stream, "//\tunrecognized: enumeration '%s' has no value\n", name ? name : "");
			}
			if(!anonymous) {
				fprintf(stream, "};\n");
				if(yflag)
					fprintf(stream, "/// Typedef synonym for enum %s.\ntypedef enum %s %s;\n", t, t,
						t);
				if(pflag && simpleType.name) {
					const char * s = aname(prefix, URI, name);
					knames.insert(s);
					s = aname(prefix, URI, name);
					fprintf(stream, "\n/// Class wrapper\n");
					fprintf(stream, "class %s : public xsd__anyType\n{ public:\n", s);
					fprintf(stream, elementformat, tname(prefix, URI, name), "__item;");
					modify(s);
					fprintf(stream, "\n};\n");
				}
			}
			else
				fprintf(stream, "    }\n");
		}
		else {
			if(simpleType.restriction->length && simpleType.restriction->length->value) {
			      fprintf(stream, "/// Length of this string is exactly %s characters\n", simpleType.restriction->length->value);
			      document(simpleType.restriction->length->annotation);
		      }
		      else {
				  const char * a = NULL, * b = NULL;
			    if(simpleType.restriction->minLength) {
				    a = simpleType.restriction->minLength->value;
				    document(simpleType.restriction->minLength->annotation);
			    }
			    if(simpleType.restriction->maxLength) {
				    b = simpleType.restriction->maxLength->value;
				    document(simpleType.restriction->maxLength->annotation);
			    }
			    if(a || b)
				    fprintf(stream, "/// Length of this string is within %s..%s characters\n", a ? a : "0", b ? b : ""); }
		      if(simpleType.restriction->precision && simpleType.restriction->precision->value)
			      fprintf(stream, "/// %sprecision is %s (note: not automatically enforced)\n",
				      simpleType.restriction->precision->fixed ? "fixed " : "", simpleType.restriction->precision->value);
		      if(simpleType.restriction->scale && simpleType.restriction->scale->value)
			      fprintf(stream, "/// %sscale is %s (note: not automatically enforced)\n",
				      simpleType.restriction->scale->fixed ? "fixed " : "", simpleType.restriction->scale->value);
		      if(simpleType.restriction->totalDigits && simpleType.restriction->totalDigits->value)
			      fprintf(stream, "/// %snumber of total digits is %s (note: not automatically enforced)\n",
				      simpleType.restriction->totalDigits->fixed ? "fixed " : "", simpleType.restriction->totalDigits->value);
		      if(simpleType.restriction->fractionDigits && simpleType.restriction->fractionDigits->value)
			      fprintf(stream, "/// %snumber of fraction digits is %s (note: not automatically enforced)\n",
				      simpleType.restriction->fractionDigits->fixed ? "fixed " : "", simpleType.restriction->fractionDigits->value);
		      for(vector <xs__pattern>::const_iterator pattern1 = simpleType.restriction->pattern.begin();
			  pattern1 != simpleType.restriction->pattern.end(); ++pattern1)
			      fprintf(stream, "/// Content pattern is \"%s\" (note: not automatically enforced)\n", xstring((*pattern1).value));
		      const char * ai = NULL, * ae = NULL, * bi = NULL, * be = NULL;
		      if(simpleType.restriction->minInclusive) {
			      ai = simpleType.restriction->minInclusive->value;
			      document(simpleType.restriction->minInclusive->annotation);
		      }
		      else if(simpleType.restriction->minExclusive) {
			      ae = simpleType.restriction->minExclusive->value;
			      document(simpleType.restriction->minExclusive->annotation);
		      }
		      if(simpleType.restriction->maxInclusive) {
			      bi = simpleType.restriction->maxInclusive->value;
			      document(simpleType.restriction->maxInclusive->annotation);
		      }
		      else if(simpleType.restriction->maxExclusive) {
			      be = simpleType.restriction->maxExclusive->value;
			      document(simpleType.restriction->maxExclusive->annotation);
		      }
		      if(ai || ae || bi || be) {
			      fprintf(stream, "/// Value range is ");
			      if(ai)
				      fprintf(stream, "[%s..", ai);
			      else if(ae)
				      fprintf(stream, "(%s..", ae);
			      else
				      fprintf(stream, "[..");
			      if(bi)
				      fprintf(stream, "%s]\n", bi);
			      else if(be)
				      fprintf(stream, "%s)\n", be);
			      else
				      fprintf(stream, "]\n");
		      }
		      if(!simpleType.restriction->attribute.empty()) {
			      if(!Wflag)
				      fprintf(stderr, "\nWarning: simpleType '%s' should not have attributes\n", name ? name : "");
		      }
		      const char * s = tname(NULL, baseURI, base);
		      if(!anonymous) {
			      bool is_ptr = false;
			      is_ptr = (strchr(s, '*') != NULL) || (s == pname(true, NULL, baseURI, base));
			      t = deftname(TYPEDEF, NULL, is_ptr, prefix, URI, name);
			      if(t)
				      fprintf(stream, "typedef %s %s", s, t);
		      }
		      else {
				  t = "";
			    fprintf(stream, elementformat, s, "");
			    fprintf(stream, "\n"); 
			  }
		      if(t) {
			      if(!anonymous && !simpleType.restriction->pattern.empty()) {
				      fprintf(stream, " \"");
				      for(vector <xs__pattern>::const_iterator pattern2 = simpleType.restriction->pattern.begin(); pattern2 != simpleType.restriction->pattern.end(); ++pattern2) {
					      if(pattern2 != simpleType.restriction->pattern.begin())
						      fprintf(stream, "|");
					      fprintf(stream, "%s", xstring((*pattern2).value));
				      }
				      fprintf(stream, "\"");
			      }
			      // add range info only when type is numeric
			      bool is_numeric = false, is_float = false;
			      if(!strncmp(s, "unsigned ", 9))
				      s += 9;
			      else if(!strncmp(s, "xsd__unsigned", 13))
				      s += 13;
			      else if(!strncmp(s, "xsd__", 5))
				      s += 5;
			      if(sstreq(s, "double") || sstreq(s, "float"))
				      is_numeric = is_float = true;
			      else if(sstreq(s, "bool") || sstreq(s, "byte") || sstreq(s, "Byte") ||
					sstreq(s, "char") || sstreq(s, "double") || sstreq(s, "float") || sstreq(s, "int") ||
					sstreq(s, "Int") || sstreq(s, "long") || sstreq(s, "Long") || sstreq(s, "LONG64") ||
					sstreq(s, "short") || sstreq(s, "Short") || sstreq(s, "ULONG64"))
				      is_numeric = true;
			      if(!anonymous && simpleType.restriction->minLength && simpleType.restriction->minLength->value)
				      fprintf(stream, " %s", simpleType.restriction->minLength->value);
			      else if(is_numeric && !anonymous && simpleType.restriction->minInclusive &&
			              simpleType.restriction->minInclusive->value && is_integer(simpleType.restriction->minInclusive->value))
				      fprintf(stream, " %s", simpleType.restriction->minInclusive->value);
			      else if(is_float && !anonymous && simpleType.restriction->minExclusive &&
			              simpleType.restriction->minExclusive->value && is_integer(simpleType.restriction->minExclusive->value))
				      fprintf(stream, " %s", simpleType.restriction->minExclusive->value);
			      else if(is_numeric && !anonymous && simpleType.restriction->minExclusive &&
			              simpleType.restriction->minExclusive->value && is_integer(simpleType.restriction->minExclusive->value))
				      fprintf(stream, " " SOAP_LONG_FORMAT, to_integer(simpleType.restriction->minExclusive->value)+1);
			      if(!anonymous && simpleType.restriction->maxLength && simpleType.restriction->maxLength->value)
				      fprintf(stream, ":%s", simpleType.restriction->maxLength->value);
			      else if(is_numeric && !anonymous && simpleType.restriction->maxInclusive &&
			              simpleType.restriction->maxInclusive->value && is_integer(simpleType.restriction->maxInclusive->value))
				      fprintf(stream, ":%s", simpleType.restriction->maxInclusive->value);
			      else if(is_float && !anonymous && simpleType.restriction->maxExclusive &&
			              simpleType.restriction->maxExclusive->value && is_integer(simpleType.restriction->maxExclusive->value))
				      fprintf(stream, ":%s", simpleType.restriction->maxExclusive->value);
			      else if(is_numeric && !anonymous && simpleType.restriction->maxExclusive && simpleType.restriction->maxExclusive->value && 
					  is_integer(simpleType.restriction->maxExclusive->value))
				      fprintf(stream, ":" SOAP_LONG_FORMAT, to_integer(simpleType.restriction->maxExclusive->value)-1);
			      if(!anonymous) {
				      fprintf(stream, ";\n");
				      if(pflag && simpleType.name) {
					      const char * s = aname(prefix, URI, name);
					      knames.insert(s);
					      s = aname(prefix, URI, name);
					      fprintf(stream, "\n/// Class wrapper\n");
					      fprintf(stream, "class %s : public xsd__anyType\n{ public:\n", s);
					      fprintf(stream, elementformat, tname(prefix, URI, name), "__item;");
					      modify(s);
					      fprintf(stream, "\n};\n");
				      }
			      }
		      }
		}
	}
	else if(simpleType.list) {
		if(simpleType.list->restriction && simpleType.list->restriction->base) {
			if(!anonymous) {
				fprintf(stream, "\n/// \"%s\":%s is a simpleType list restriction of %s.\n",
					URI ? URI : "", name, simpleType.list->restriction->base);
				fprintf(stream, "/// Note: this enumeration is a bitmask, so a set of values is supported (using | and & bit-ops on the bit vector).\n");
			}
			document(simpleType.annotation);
			if(!anonymous) {
				t = deftname(ENUM, NULL, false, prefix, URI, name);
				if(t)
					fprintf(stream, "enum * %s\n{\n", t);
			}
			else {
				t = "";
			      fprintf(stream, "enum *\n{\n"); }
			if(t) {
				for(vector <xs__enumeration>::const_iterator enumeration = simpleType.list->restriction->enumeration.begin(); 
					enumeration != simpleType.list->restriction->enumeration.end(); ++enumeration) {
					if((*enumeration).value) {
						if(sstreq(simpleType.list->restriction->base, "xs:QName") && (*enumeration).value_)
							fprintf(stream, "\t%s,\t///< %s value=\"%s\"\n", ename(t, (*enumeration).value_, true), simpleType.list->restriction->base,
								(*enumeration).value_);
						else
							fprintf(stream, "\t%s,\t///< %s value=\"%s\"\n", ename(t, (*enumeration).value, false), simpleType.list->restriction->base,
								(*enumeration).value);
					}
					else
						fprintf(stream, "//\tunrecognized: bitmask enumeration '%s' has no value\n", t);
				}
				if(!anonymous) {
					fprintf(stream, "};\n");
					if(yflag)
						fprintf(stream, "/// Typedef synonym for enum %s.\ntypedef enum %s %s;\n", t, t, t);
					if(pflag && simpleType.name) {
						const char * s = aname(prefix, URI, name);
						knames.insert(s);
						s = aname(prefix, URI, name);
						fprintf(stream, "\n/// Class wrapper\n");
						fprintf(stream, "class %s : public xsd__anyType\n{ public:\n", s);
						fprintf(stream, elementformat, tname(prefix, URI, name), "__item;");
						modify(s);
						fprintf(stream, "\n};\n");
					}
				}
				else
					fprintf(stream, "}\n");
			}
		}
		else if(simpleType.list->itemType) {
			const xs__simpleType * p = simpleType.list->itemTypePtr();
			if(p && p->restriction && p->restriction->base && !p->restriction->enumeration.empty() && p->restriction->enumeration.size() <= 64) {
				if(!anonymous)                                               {
					fprintf(stream, "\n/// \"%s\":%s is a simpleType list of %s.\n", URI ? URI : "", name, simpleType.list->itemType);
					fprintf(stream, "/// Note: this enumeration is a bitmask, so a set of values is supported (using | and & bit-ops on the bit vector).\n");
				}
				document(simpleType.annotation);
				if(!anonymous) {
					t = deftname(ENUM, NULL, false, prefix, URI, name);
					if(t)
						fprintf(stream, "enum * %s\n{\n", t);
				}
				else {
					t = "";
				      fprintf(stream, "enum *\n{\n"); 
				}
				if(t) {
					for(vector <xs__enumeration>::const_iterator enumeration = p->restriction->enumeration.begin();
					    enumeration != p->restriction->enumeration.end(); ++enumeration) {
						if((*enumeration).value) {
							if(sstreq(p->restriction->base, "xs:QName") && (*enumeration).value_)
								fprintf(stream, "\t%s,\t///< %s value=\"%s\"\n", ename(t, (*enumeration).value_, true), p->restriction->base,
									(*enumeration).value_);
							else
								fprintf(stream, "\t%s,\t///< %s value=\"%s\"\n", ename(t, (*enumeration).value, false), p->restriction->base,
									(*enumeration).value);
						}
						else
							fprintf(
								stream,
								"//\tunrecognized: bitmask enumeration '%s' has no value\n",
								t);
					}
					if(!anonymous) {
						fprintf(stream, "};\n");
						if(yflag)
							fprintf(stream, "/// Typedef synonym for enum %s.\ntypedef enum %s %s;\n", t, t, t);
						if(pflag && simpleType.name) {
							const char * s = aname(prefix, URI, name);
							knames.insert(s);
							s = aname(prefix, URI, name);
							fprintf(stream, "\n/// Class wrapper.\n");
							fprintf(stream, "class %s : public xsd__anyType\n{ public:\n", s);
							fprintf(stream, elementformat, tname(prefix, URI, name), "__item;");
							modify(s);
							fprintf(stream, "\n};\n");
						}
					}
					else
						fprintf(stream, "}\n");
				}
			}
			else {
				const char * s;
			      if(sstreq(simpleType.list->itemType, "xs:QName"))
				      s = tname(NULL, NULL, "xsd:QName");
			      else
				      s = tname(NULL, NULL, "xsd:string");
			      if(!anonymous) {
				      fprintf(
					      stream,
					      "\n/// \"%s\":%s is a simpleType containing a whitespace separated list of %s.\n",
					      URI ? URI : "", name, simpleType.list->itemType);
				      t = deftname(TYPEDEF, NULL, strchr(s, '*') != NULL, prefix, URI, name);
			      }
			      document(simpleType.annotation);
			      if(t)
				      fprintf(stream, "typedef %s %s;\n", s, t);
			      else {
					  fprintf(stream, elementformat, s, "");
				    fprintf(stream, "\n"); 
				  }
			}
		}
		else {
			if(!anonymous) {
			      fprintf(stream, "\n/// \"%s\":%s is a simpleType list.\n", URI ? URI : "", name);
			      fprintf(stream,
				      "/// Note: this enumeration is a bitmask, so a set of values is supported (using | and & bit-ops on the bit vector).\n");
		      }
		      document(simpleType.annotation);
		      if(!anonymous) {
			      t = deftname(ENUM, NULL, false, prefix, URI, name);
			      if(t && !eflag)
				      fprintf(stream,
					      "/// Note: enum values are prefixed with '%s' to avoid name clashes, please use wsdl2h option -e to omit this prefix\n",
					      t);
		      }
		      else
			      t = "";
		      if(t) {
			      fprintf(stream, "enum * %s\n{\n", t);
			      for(vector <xs__simpleType>::const_iterator simple = simpleType.list->simpleType.begin();
			          simple != simpleType.list->simpleType.end(); ++simple) {
				      if((*simple).restriction &&
				         (*simple).restriction->base)
				      {
					      for(vector <xs__enumeration>::const_iterator enumeration =
					                  (*simple).restriction->enumeration.begin();
					          enumeration != (*simple).restriction->enumeration.end();
					          ++enumeration)
					      {
						      if((*enumeration).value) {
							      if(sstreq((*simple).restriction->base, "xs:QName") && (*enumeration).value_)
								      fprintf(stream, "\t%s,\t///< %s value=\"%s\"\n", ename(t, (*enumeration).value_, true),
									      (*simple).restriction->base, (*enumeration).value_);
							      else
								      fprintf(stream, "\t%s,\t///< %s value=\"%s\"\n", ename(t, (*enumeration).value, false),
									      (*simple).restriction->base, (*enumeration).value);
						      }
						      else
							      fprintf(stream, "//\tunrecognized: bitmask enumeration '%s' has no value\n", t);
					      }
				      }
			      }
			      if(!anonymous) {
				      fprintf(stream, "};\n");
				      if(yflag)
					      fprintf(stream, "/// Typedef synonym for enum %s.\ntypedef enum %s %s;\n",
						      t, t, t);
				      if(pflag && simpleType.name) {
					      const char * s = aname(prefix, URI, name);
					      knames.insert(s);
					      s = aname(prefix, URI, name);
					      fprintf(stream, "\n/// Class wrapper.\n");
					      fprintf(stream, "class %s : public xsd__anyType\n{ public:\n", s);
					      fprintf(stream, elementformat, tname(prefix, URI, name), "__item;");
					      modify(s);
					      fprintf(stream, "\n};\n");
				      }
			      }
			      else
				      fprintf(stream, "}\n");
		      }
		}
	}
	else if(simpleType.union_) {
		if(simpleType.union_->memberTypes)                             {
			const char * s = tname(NULL, NULL, "xsd:string");
			if(!anonymous)
				t = deftname(TYPEDEF, NULL, strchr(s, '*') != NULL, prefix, URI, name);
			fprintf(stream, "\n/// union of values \"%s\"\n", simpleType.union_->memberTypes);
			if(t)
				fprintf(stream, "typedef %s %s;\n", s, t);
			else {fprintf(stream, elementformat, s, "");
			      fprintf(stream, "\n"); }
		}
		else if(!simpleType.union_->simpleType.empty()) {
			const char * s = tname(NULL, NULL, "xsd:string");
			fprintf(stream, "\n");
			if(!anonymous)
				t = deftname(TYPEDEF, NULL, strchr(s, '*') != NULL, prefix, URI, name);
			for(vector <xs__simpleType>::const_iterator simpleType1 = simpleType.union_->simpleType.begin();
			    simpleType1 != simpleType.union_->simpleType.end(); ++simpleType1)
				if((*simpleType1).restriction) {
					fprintf(stream, "/// union of values from \"%s\"\n",
						(*simpleType1).restriction->base);
					// TODO: are there any other types we should report here?
				}
			if(t)
				fprintf(stream, "typedef %s %s;\n", s, t);
			else {fprintf(stream, elementformat, s, "");
			      fprintf(stream, "\n"); }
		}
		else
			fprintf(stream, "//\tunrecognized\n");
	}
	else
		fprintf(stream, "//\tunrecognized simpleType\n");
}

void Types::gen(const char * URI, const char * name, const xs__complexType & complexType, bool anonymous)
{
	const char * t = NULL;
	const char * prefix = NULL;
	bool soapflag = false;
	if(complexType.name)
		name = complexType.name;
	else
		prefix = "_";
	if(anonymous && name)
		t = sname(URI, name);
	else if(name) {
		t = cname(prefix, URI, name);
		if(deftypemap[t])
			return;
	}
	if(name)
		scope.push_back(name);
	if(complexType.simpleContent) {
		if(!anonymous)
			fprintf(stream, "\n/// \"%s\":%s is a%s complexType with simpleContent.\n", URI ? URI : "",
				name,
				complexType.abstract ? "n abstract" : "");
		document(complexType.annotation);
		if(complexType.simpleContent->restriction) {
			if(anonymous) {
				if(cflag)
					fprintf(stream, "    struct %s\n    {\n", t);
				else
					fprintf(stream, "    class %s\n    {\n", t);
			}
			else if(cflag)
				fprintf(stream, "struct %s\n{\n", t);
			else if(pflag && complexType.name)
				fprintf(stream, "class %s : public xsd__anyType\n{ public:\n", t);
			else
				fprintf(stream, "class %s\n{ public:\n", t);
			const char * base = "xs:string";
			const char * baseURI = NULL;
			const xs__complexType * p = &complexType;
			do {if(!p->simpleContent)
				    break;
			    if(p->simpleContent->restriction) {
				    if(p->simpleContent->restriction->complexTypePtr())
					    p = p->simpleContent->restriction->complexTypePtr();
				    else {base = p->simpleContent->restriction->base;
					  if(p->simpleContent->restriction->simpleTypePtr() &&
					     p->simpleContent->restriction->simpleTypePtr()->schemaPtr())
						  baseURI =
						          p->simpleContent->restriction->simpleTypePtr()->schemaPtr()->
						          targetNamespace;
					  break; }
			    }
			    else if(p->simpleContent->extension) {
				    if(p->simpleContent->extension->complexTypePtr())
					    p = p->simpleContent->extension->complexTypePtr();
				    else {base = p->simpleContent->extension->base;
					  if(p->simpleContent->extension->simpleTypePtr() &&
					     p->simpleContent->extension->simpleTypePtr()->schemaPtr())
						  baseURI =
						          p->simpleContent->extension->simpleTypePtr()->schemaPtr()->
						          targetNamespace;
					  break; }
			    }
			    else
				    break; } while(p);
			fprintf(stream, "/// __item wraps '%s' simpleContent.\n", base);
			fprintf(stream, elementformat, tname(NULL, baseURI, base), "__item");
			fprintf(stream, ";\n");
			p = &complexType;
			bool flag = true;
			do {if(!p->simpleContent)
				    break;
			    if(p->simpleContent->restriction) { // TODO: should only generate attribute when name is different?
				    gen(URI, p->simpleContent->restriction->attribute);
				    if(p->simpleContent->restriction->anyAttribute && flag) {
					    gen(URI, *p->simpleContent->restriction->anyAttribute);
					    flag = false;
				    }
				    if(p->simpleContent->restriction->complexTypePtr())
					    p = p->simpleContent->restriction->complexTypePtr();
				    else
					    break;
			    }
			    else if(p->simpleContent->extension) {
				    gen(URI, p->simpleContent->extension->attribute);
				    gen(URI, p->simpleContent->extension->attributeGroup);
				    if(p->simpleContent->extension->anyAttribute && flag) {
					    gen(URI, *p->simpleContent->extension->anyAttribute);
					    flag = false;
				    }
				    if(p->simpleContent->extension->complexTypePtr())
					    p = p->simpleContent->extension->complexTypePtr();
				    else
					    break;
			    }
			    else
				    break; } while(p);
		}
		else if(complexType.simpleContent->extension) {
			const char * base = "xs:string";
			const char * baseURI = NULL;
			if(cflag || fflag || anonymous) {
				if(anonymous)                                  {
					if(cflag)
						fprintf(stream, "    struct %s\n    {\n", t);
					else
						fprintf(stream, "    class %s\n    {\n", t);
				}
				else if(cflag)
					fprintf(stream, "struct %s\n{\n", t);
				else if(pflag && complexType.name)
					fprintf(stream, "class %s : public xsd__anyType\n{ public:\n", t);
				else
					fprintf(stream, "class %s\n{ public:\n", t);
				const xs__complexType * p = &complexType;
				do {if(!p->simpleContent)
					    break;
				    if(p->simpleContent->restriction) {
					    if(p->simpleContent->restriction->complexTypePtr())
						    p = p->simpleContent->restriction->complexTypePtr();
					    else {base = p->simpleContent->restriction->base;
						  if(p->simpleContent->restriction->simpleTypePtr() &&
						     p->simpleContent->restriction->simpleTypePtr()->schemaPtr())
							  baseURI =
							          p->simpleContent->restriction->simpleTypePtr()->
							          schemaPtr()
							          ->targetNamespace;
						  break; }
				    }
				    else if(p->simpleContent->extension) {
					    if(p->simpleContent->extension->complexTypePtr())
						    p = p->simpleContent->extension->complexTypePtr();
					    else {base = p->simpleContent->extension->base;
						  if(p->simpleContent->extension->simpleTypePtr() &&
						     p->simpleContent->extension->simpleTypePtr()->schemaPtr())
							  baseURI =
							          p->simpleContent->extension->simpleTypePtr()->
							          schemaPtr()
							          ->targetNamespace;
						  break; }
				    }
				    else
					    break; } while(p);
				fprintf(stream, "/// __item wraps '%s' simpleContent.\n", base);
				fprintf(stream, elementformat, tname(NULL, baseURI, base), "__item");
				fprintf(stream, ";\n");
				p = &complexType;
				bool flag = true;
				do {if(!p->simpleContent)
					    break;
				    if(p->simpleContent->restriction) {
					    gen(URI, p->simpleContent->restriction->attribute);
					    if(p->simpleContent->restriction->anyAttribute && flag)
						    gen(URI, *p->simpleContent->restriction->anyAttribute);
					    break;
				    }
				    else if(p->simpleContent->extension) {
					    gen(URI, p->simpleContent->extension->attribute);
					    gen(URI, p->simpleContent->extension->attributeGroup);
					    if(p->simpleContent->extension->anyAttribute && flag) {
						    gen(URI, *p->simpleContent->extension->anyAttribute);
						    flag = false;
					    }
					    if(p->simpleContent->extension->complexTypePtr())
						    p = p->simpleContent->extension->complexTypePtr();
					    else
						    break;
				    }
				    else
					    break; } while(p);
			}
			else {base = complexType.simpleContent->extension->base;
			      if(
				      /* TODO: in future, may want to add check here for base type == class
				         complexType.simpleContent->extension->simpleTypePtr()
				       ||
				       */
				      complexType.simpleContent->extension->complexTypePtr()) {
				      if(complexType.simpleContent->extension->complexTypePtr()->schemaPtr())
					      baseURI =
					              complexType.simpleContent->extension->complexTypePtr()->schemaPtr()
					              ->
					              targetNamespace;
				      fprintf(stream, "class %s : public %s\n{ public:\n", t, cname(NULL, baseURI, base));
				      soapflag = true;
			      }
			      else {if(complexType.simpleContent->extension->simpleTypePtr() &&
				       complexType.simpleContent->extension->simpleTypePtr()->schemaPtr())
					    baseURI =
					            complexType.simpleContent->extension->simpleTypePtr()->schemaPtr()
					            ->
					            targetNamespace;
				    else if(complexType.simpleContent->extension->complexTypePtr() &&
				            complexType.simpleContent->extension->complexTypePtr()->schemaPtr())
					    baseURI =
					            complexType.simpleContent->extension->complexTypePtr()->schemaPtr()
					            ->
					            targetNamespace;
				    if(pflag && complexType.name)
					    fprintf(stream, "class %s : public xsd__anyType\n{ public:\n", t);
				    else
					    fprintf(stream, "class %s\n{ public:\n", t);
				    fprintf(stream, "/// __item wraps '%s' simpleContent.\n", base);
				    fprintf(stream, elementformat, tname(NULL, baseURI, base), "__item");
				    fprintf(stream, ";\n"); }
			      gen(URI, complexType.simpleContent->extension->attribute);
			      gen(URI, complexType.simpleContent->extension->attributeGroup);
			      if(complexType.simpleContent->extension->anyAttribute)
				      gen(URI, *complexType.simpleContent->extension->anyAttribute); }
		}
		else
			fprintf(stream, "//\tunrecognized\n");
	}
	else if(complexType.complexContent) {
		if(complexType.complexContent->restriction)                                      {
			if(!anonymous)
				fprintf(stream,
					"\n/// \"%s\":%s is a%s complexType with complexContent restriction of %s.\n",
					URI ? URI : "", name, complexType.abstract ? "n abstract" : "",
					complexType.complexContent->restriction->base);
			document(complexType.annotation);
			if(sstreq(complexType.complexContent->restriction->base, "SOAP-ENC:Array")) {
				char * item = NULL, * type = NULL;
				if(!complexType.complexContent->restriction->attribute.empty()) {
					xs__attribute & attribute =
					        complexType.complexContent->restriction->attribute.front();
					if(attribute.wsdl__arrayType)
						type = attribute.wsdl__arrayType;
				}
				xs__seqchoice * s = complexType.complexContent->restriction->sequence;
				if(s &&
				   !s->__contents.empty() &&
				   s->__contents.front().__union == SOAP_UNION_xs__union_content_element &&
				   s->__contents.front().__content.element) {
					xs__element & element = *s->__contents.front().__content.element;
					if(!type) {
						if(element.type)
							type = element.type;
						else if(element.simpleTypePtr()) {
							if(element.simpleTypePtr()->name)
								type = element.simpleTypePtr()->name;
							else if(element.simpleTypePtr()->restriction)
								type = element.simpleTypePtr()->restriction->base;
						}
						else if(element.complexTypePtr()) {
							if(element.complexTypePtr()->name)
								type = element.complexTypePtr()->name;
							else if(element.complexTypePtr()->complexContent &&
							        element.complexTypePtr()->complexContent->restriction)
								type =
								        element.complexTypePtr()->complexContent->
								        restriction->
								        base;
						}
					}
					item = element.name;
				}
				gen_soap_array(name, t, item, type);
			}
			else {if(anonymous)      {
				      if(cflag)
					      fprintf(stream, "    struct %s\n    {\n", t);
				      else
					      fprintf(stream, "    class %s\n    {\n", t);
			      }
			      else if(cflag)
				      fprintf(stream, "struct %s\n{\n", t);
			      else if(pflag && complexType.name)
				      fprintf(stream, "class %s : public xsd__anyType\n{ public:\n", t);
			      else
				      fprintf(stream, "class %s\n{ public:\n", t);
			      if(complexType.complexContent->restriction->group)
				      gen(URI, *complexType.complexContent->restriction->group);
			      if(complexType.complexContent->restriction->all)
				      gen(URI, *complexType.complexContent->restriction->all);
			      if(complexType.complexContent->restriction->sequence)
				      gen(URI, *complexType.complexContent->restriction->sequence);
			      if(complexType.complexContent->restriction->choice)
				      gen(URI, name, *complexType.complexContent->restriction->choice);
			      const xs__complexType * p = &complexType;
			      bool flag = true;
			      do {if(p->complexContent && p->complexContent->restriction)    { // TODO: should only generate attribute when name is different?
					  gen(URI, p->complexContent->restriction->attribute);
					  if(p->complexContent->restriction->anyAttribute && flag) {
						  gen(URI, *p->complexContent->restriction->anyAttribute);
						  flag = false;
					  }
					  if(p->complexContent->restriction->complexTypePtr())
						  p = p->complexContent->restriction->complexTypePtr();
					  else
						  break;
				  }
				  else if(p->complexContent && p->complexContent->extension) {
					  gen(URI, p->complexContent->extension->attribute);
					  gen(URI, p->complexContent->extension->attributeGroup);
					  if(p->complexContent->extension->anyAttribute && flag) {
						  gen(URI, *p->complexContent->extension->anyAttribute);
						  flag = false;
					  }
					  if(p->complexContent->extension->complexTypePtr())
						  p = p->complexContent->extension->complexTypePtr();
					  else
						  break;
				  }
				  else {gen(URI, p->attribute);
					gen(URI, p->attributeGroup);
					if(p->anyAttribute && flag)
						gen(URI, *p->anyAttribute);
					break; }} while(p); }
		}
		else if(complexType.complexContent->extension) {
			const char * base = complexType.complexContent->extension->base;
			xs__complexType * p = complexType.complexContent->extension->complexTypePtr();
			if(!anonymous)
				fprintf(stream,
					"\n/// \"%s\":%s is a%s complexType with complexContent extension of %s.\n",
					URI ? URI : "", name, complexType.abstract ? "n abstract" : "",
					base);
			document(complexType.annotation);
			if(anonymous) {
				if(cflag)
					fprintf(stream, "    struct %s\n    {\n", t);
				else
					fprintf(stream, "    class %s\n    {\n", t);
			}
			else if(cflag)
				fprintf(stream, "struct %s\n{\n", t);
			else if(fflag)
				fprintf(stream, "class %s\n{ public:\n", t);
			else { // TODO: what to do if base class is in another namespace and elements must be qualified in XML payload?
				const char * baseURI = NULL;
				if(p && p->schemaPtr())
					baseURI = p->schemaPtr()->targetNamespace;
				fprintf(stream, "class %s : public %s\n{ public:\n", t, cname(NULL, baseURI, base));
				soapflag = true;
			}
			gen_inh(URI, p, anonymous);
			if(complexType.complexContent->extension->group)
				gen(URI, *complexType.complexContent->extension->group);
			if(complexType.complexContent->extension->all)
				gen(URI, *complexType.complexContent->extension->all);
			if(complexType.complexContent->extension->sequence)
				gen(URI, *complexType.complexContent->extension->sequence);
			if(complexType.complexContent->extension->choice)
				gen(URI, name, *complexType.complexContent->extension->choice);
			gen(URI, complexType.complexContent->extension->attribute);
			gen(URI, complexType.complexContent->extension->attributeGroup);
			if(complexType.complexContent->extension->anyAttribute)
				gen(URI, *complexType.complexContent->extension->anyAttribute);
		}
		else
			fprintf(stream, "//\tunrecognized\n");
	}
	else {if(!anonymous)
		      fprintf(stream, "\n/// \"%s\":%s is a%s complexType.\n", URI ? URI : "", name,
			      complexType.abstract ? "n abstract" : "");
	      document(complexType.annotation);
	      if(anonymous) {
		      if(cflag)
			      fprintf(stream, "    struct %s\n    {\n", t);
		      else
			      fprintf(stream, "    class %s\n    {\n", t);
	      }
	      else if(cflag)
		      fprintf(stream, "struct %s\n{\n", t);
	      else if(pflag && complexType.name)
		      fprintf(stream, "class %s : public xsd__anyType\n{ public:\n", t);
	      else
		      fprintf(stream, "class %s\n{ public:\n", t);
	      if(complexType.all)
		      gen(URI, *complexType.all);
	      else if(complexType.choice)
		      gen(URI, name, *complexType.choice);
	      else if(complexType.sequence)
		      gen(URI, *complexType.sequence);
	      else if(complexType.any)
		      gen(URI, *complexType.any); }
	gen(URI, complexType.attribute);
	gen(URI, complexType.attributeGroup);
	if(complexType.anyAttribute)
		gen(URI, *complexType.anyAttribute);
	if(complexType.mixed ||
	   (complexType.complexContent &&
	    complexType.complexContent->extension &&
	    complexType.complexContent->extension->complexTypePtr() &&
	    complexType.complexContent->extension->complexTypePtr()->mixed
	   )) {
		fprintf(
			stream,
			"/// TODO: this mixed complexType is user-definable.\n///       Consult the protocol documentation to change or insert declarations.\n///       Use wsdl2h option -d for xsd__anyType DOM (soap_dom_element).\n");
		if(dflag) {
			fprintf(stream, elementformat, "xsd__anyType", "__mixed");
			fprintf(stream, "0;\t///< Catch mixed content in DOM soap_dom_element linked node structure.\n");
		}
		else {fprintf(stream, elementformat, "_XML", "__mixed");
		      fprintf(stream, "0;\t///< Catch mixed content in XML string\n"); }
	}
	if(!anonymous) {
		if(!cflag                 &&
		   !(pflag && complexType.name) &&
		   !soapflag) {
			if(!complexType.complexContent || !complexType.complexContent->extension ||
			   !complexType.complexContent->extension->complexTypePtr())                {
				fprintf(
					stream,
					"/// A handle to the soap struct that manages this instance (automatically set)\n");
				fprintf(stream, pointerformat, "struct soap", "soap");
				fprintf(stream, ";\n");
			}
		}
		modify(t);
		fprintf(stream, "};\n");
	}
	scope.pop_back();
}

void Types::gen(const char * URI, const vector <xs__attribute> & attributes)
{
	for(vector <xs__attribute>::const_iterator attribute = attributes.begin(); attribute != attributes.end();
	    ++attribute)
		gen(URI, *attribute);
}

void Types::gen(const char * URI, const xs__attribute & attribute)
{
	const char * name, * type, * nameURI = NULL, * typeURI = NULL, * nameprefix = NULL, * typeprefix = NULL;
	name = attribute.name;
	type = attribute.type;
	bool is_optional = attribute.use != required && attribute.use != default_ && attribute.use != fixed_ &&
	                   !attribute.default_;
	document(attribute.annotation);
	if(!URI)
		URI = attribute.schemaPtr()->targetNamespace;
	if(attribute.form) {
		if(*attribute.form == qualified)
			nameURI = URI;
		else {nameURI = NULL;
		      nameprefix = ":"; }
	}
	if(attribute.attributePtr()) { // attribute ref
		name = attribute.attributePtr()->name;
		type = attribute.attributePtr()->type;
		if(!type) {
			type = name;
			typeprefix = "_";
		}
		if(attribute.attributePtr()->schemaPtr()) {
			typeURI = attribute.attributePtr()->schemaPtr()->targetNamespace;
			if(attribute.form && *attribute.form == unqualified)
				nameprefix = ":";
			else if(zflag == 3 &&
			        URI &&
			        typeURI &&
			        attribute.schemaPtr()->attributeFormDefault == unqualified &&
			        sstreq(URI, typeURI))
				nameprefix = NULL;
			else
				nameURI = typeURI;
		}
		fprintf(stream, "/// Attribute reference %s.\n", attribute.ref);
		document(attribute.attributePtr()->annotation);
		fprintf(stream, attributeformat,
			pname(is_optional, typeprefix, typeURI, type), aname(nameprefix, nameURI, name));              // make sure no name - type clash
	}
	else if(name && type) {
		fprintf(stream, "/// Attribute %s of type %s.\n", name, type);
		fprintf(stream, attributeformat, pname(is_optional, NULL, URI, type), aname(nameprefix, nameURI, name)); // make sure no name - type clash
	}
	else if(name && attribute.simpleTypePtr()) {
		fprintf(stream, "@");
		gen(URI, name, *attribute.simpleTypePtr(), true);
		// 8/1/09 Changed (is_optional && !cflag && !sflag) to is_optional
		fprintf(stream, is_optional ? pointerformat : elementformat, "", aname(nameprefix, nameURI, name));
	}
	else if(attribute.ref) {
		fprintf(stream, "/// Imported attribute reference %s.\n", attribute.ref);
		fprintf(stream, attributeformat,
			pname(is_optional, "_", NULL, attribute.ref), aname(NULL, NULL, attribute.ref));
	}
	else {fprintf(stream, "/// Attribute '%s' has no type or ref: assuming string content.\n", name ? name : "");
	      fprintf(stream, attributeformat, tname(NULL, NULL, "xs:string"), aname(NULL, nameURI, name)); }
	switch(attribute.use) {
	    case prohibited:
		fprintf(stream, " 0:0");
		break;
	    case required:
		fprintf(stream, " 1");
		break;
	    default:
		fprintf(stream, " 0");
		break;
	}
	if(attribute.default_ ||
	   (attribute.fixed && !is_optional)) {
		const char * value, * QName;
		if(attribute.default_) {
			value = attribute.default_;
			QName = attribute.default__;
		}
		else {value = attribute.fixed;
		      QName = attribute.fixed_; }
		const char * t = NULL;
		if(!type && attribute.simpleTypePtr()) {
			if(attribute.simpleTypePtr()->restriction &&
			   attribute.simpleTypePtr()->restriction->base)                                         {
				if(!attribute.simpleTypePtr()->restriction->enumeration.empty())
				{
					const char * s;
					if(is_integer(value))
						fprintf(stream, " = %s", value);
					else if(!*value)
						fprintf(stream, " = 0");
					else if((s = enames[Pair(gname(URI, name), value)]))
						fprintf(stream, " = %s", s);
				}
				else {const char * baseURI = NULL;
				      if(attribute.simpleTypePtr()->restriction->simpleTypePtr() &&
					 attribute.simpleTypePtr()->restriction->simpleTypePtr()->schemaPtr())
					      baseURI =
					              attribute.simpleTypePtr()->restriction->simpleTypePtr()->
					              schemaPtr()->
					              targetNamespace;
				      t = tname(NULL, baseURI, attribute.simpleTypePtr()->restriction->base); }
			}
		}
		if(type && !t)
			t = tname(NULL, typeURI ? typeURI : URI, type);
		if(t) {
			if(!strncmp(t, "unsigned ", 9))
				t += 9;
			else if(!strncmp(t, "xsd__unsigned", 13))
				t += 13;
			else if(!strncmp(t, "xsd__", 5))
				t += 5;
			if(sstreq(t, "bool") ||
			   sstreq(t, "byte") ||
			   sstreq(t, "Byte") ||
			   sstreq(t, "char") ||
			   sstreq(t, "double") ||
			   sstreq(t, "float") ||
			   sstreq(t, "int") ||
			   sstreq(t, "Int") ||
			   sstreq(t, "long") ||
			   sstreq(t, "Long") ||
			   sstreq(t, "LONG64") ||
			   sstreq(t, "short") ||
			   sstreq(t, "Short") ||
			   sstreq(t, "ULONG64"))
				fprintf(stream, " = %s", value);
			else if(!strncmp(t, "enum ", 5)) {
				const char * s;
				if(is_integer(value))
					fprintf(stream, " = %s", value);
				else if(!*value)
					fprintf(stream, " = 0");
				else if((s = enames[Pair(t+5, value)]))
					fprintf(stream, " = %s", s);
			}
			else if(sstreq(t, "char*") ||
			        sstreq(t, "char *") || // not elegant
			        sstreq(t, "std::string") ||
			        sstreq(t, "std::string*") ||
			        sstreq(t, "std::string *")) // not elegant
				fprintf(stream, " = \"%s\"", cstring(value));
			else if(sstreq(t, "xsd__QName") && QName) // QName
				fprintf(stream, " = \"%s\"", cstring(QName));
		}
		if(attribute.default_)
			fprintf(stream, ";\t///< Default value=\"%s\".\n", value);
		else
			fprintf(stream, ";\t///< Fixed required value=\"%s\".\n", value);
	}
	else if(attribute.fixed)
		fprintf(stream, ";\t///< Fixed optional value=\"%s\".\n", attribute.fixed);
	else if(attribute.use == required)
		fprintf(stream, ";\t///< Required attribute.\n");
	else if(attribute.use == prohibited)
		fprintf(stream, ";\t///< Prohibited attribute.\n");
	else
		fprintf(stream, ";\t///< Optional attribute.\n");
}

void Types::gen(const char * URI, const vector <xs__attributeGroup> & attributeGroups)
{
	for(vector <xs__attributeGroup>::const_iterator attributeGroup = attributeGroups.begin();
	    attributeGroup != attributeGroups.end(); ++attributeGroup) {
		if((*attributeGroup).attributeGroupPtr())
		{                              // attributeGroup ref
			gen(URI, (*attributeGroup).attributeGroupPtr()->attribute);
			gen(URI, (*attributeGroup).attributeGroupPtr()->attributeGroup);
			if((*attributeGroup).attributeGroupPtr()->anyAttribute)
				gen(URI, *(*attributeGroup).attributeGroupPtr()->anyAttribute);
		}
		else {fprintf(stream, "/// Begin attributeGroup %s.\n",
			      (*attributeGroup).name ? (*attributeGroup).name : "");
		      gen(URI, (*attributeGroup).attribute);
		      gen(URI, (*attributeGroup).attributeGroup);
		      if((*attributeGroup).anyAttribute)
			      gen(URI, *(*attributeGroup).anyAttribute);
		      fprintf(stream, "/// End of attributeGroup %s.\n",
			      (*attributeGroup).name ? (*attributeGroup).name : ""); }
	}
}

void Types::gen(const char * URI, const vector <xs__all> & alls)
{
	for(vector <xs__all>::const_iterator all = alls.begin(); all != alls.end(); ++all)
		gen(URI, *all);
}

void Types::gen(const char * URI, const xs__all & all)
{
	bool tmp_union1 = with_union;
	bool tmp_union2 = fake_union;
	with_union = false;
	fake_union = false;
	gen(URI, all.element);
	with_union = tmp_union1;
	fake_union = tmp_union2;
}

void Types::gen(const char * URI, const vector <xs__contents> & contents)
{
	for(vector <xs__contents>::const_iterator content = contents.begin(); content != contents.end(); ++content) {
		switch((*content).__union)
		{
		    case SOAP_UNION_xs__union_content_element:
			if((*content).__content.element)
				gen(URI, *(*content).__content.element, true);
			break;
		    case SOAP_UNION_xs__union_content_group:
			if((*content).__content.group)
				gen(URI, *(*content).__content.group);
			break;
		    case SOAP_UNION_xs__union_content_choice:
			if((*content).__content.choice)
				gen(URI, NULL, *(*content).__content.choice);
			break;
		    case SOAP_UNION_xs__union_content_sequence:
			if((*content).__content.sequence)
				gen(URI, *(*content).__content.sequence);
			break;
		    case SOAP_UNION_xs__union_content_any:
			if((*content).__content.any)
				gen(URI, *(*content).__content.any);
			break;
		}
	}
}

void Types::gen(const char * URI, const xs__seqchoice & sequence)
{
	const char * s = NULL;
	char * t = NULL;
	bool tmp_union = with_union;
	with_union = false;
	if((sequence.minOccurs && strcmp(sequence.minOccurs, "1")) ||
	   (sequence.maxOccurs && strcmp(sequence.maxOccurs, "1"))) {
		fprintf(stream, "/// SEQUENCE OF ELEMENTS <xs:sequence");
		if(sequence.minOccurs)
			fprintf(stream, " minOccurs=\"%s\"", sequence.minOccurs);
		if(sequence.maxOccurs)
			fprintf(stream, " maxOccurs=\"%s\"", sequence.maxOccurs);
		fprintf(stream, ">\n");
		document(sequence.annotation);
		s = sname(URI, "sequence");
		t = static_cast<char *>(emalloc(strlen(s)+2));
		strcpy(t, "_");
		strcat(t, s);
		s = strstr(s, "__");
		if(!s)
			s = t;
		if(cflag || sflag || zflag == 2) {
			fprintf(stream, sizeformat, "int", s+1);
			if(!fake_union && sequence.minOccurs)
				fprintf(stream, " %s", sequence.minOccurs);
			if(sequence.maxOccurs &&
			   strcmp(sequence.maxOccurs, "1") &&
			   is_integer(sequence.maxOccurs))
				fprintf(stream, ":%s", sequence.maxOccurs);
			fprintf(stream, ";\n");
		}
		else {fprintf(stream, elementformat, "std::vector<", "");
		      fprintf(stream, "\n"); }
		if(cflag)
			fprintf(stream, "    struct %s\n    {\n", t);
		else
			fprintf(stream, "    class %s\n    {\n", t);
	}
	else {
		if(fake_union)
		      fprintf(stream, "/// SEQUENCE OF ELEMENTS <xs:sequence>\n");
	      document(sequence.annotation); 
	}
	gen(URI, sequence.__contents);
	if(s) {
		if(cflag || sflag || zflag == 2)
			fprintf(stream, pointerformat, "}", s);
		else {
			fprintf(stream, elementformat, "}>", s);
		      if(!fake_union && sequence.minOccurs)
			      fprintf(stream, " %s", sequence.minOccurs);
		      if(sequence.maxOccurs && strcmp(sequence.maxOccurs, "1") && is_integer(sequence.maxOccurs))
			      fprintf(stream, ":%s", sequence.maxOccurs); 
		}
		fprintf(stream, ";\n");
	}
	if(s || fake_union)
		fprintf(stream, "//  END OF SEQUENCE\n");
	with_union = tmp_union;
}

void Types::gen(const char * URI, const vector <xs__element> & elements)
{
	for(vector <xs__element>::const_iterator element = elements.begin(); element != elements.end(); ++element)
		gen(URI, *element, true);
}

void Types::gen(const char * URI, const xs__element & element, bool substok)
{
	const char * name, * type, * nameURI = NULL, * typeURI = NULL, * nameprefix = NULL, * typeprefix = NULL;
	name = element.name;
	type = element.type;
	document(element.annotation);
	if(!URI)
		URI = element.schemaPtr()->targetNamespace;
	if(element.xmime__expectedContentTypes)
		fprintf(stream, "/// MTOM attachment with content types %s.\n", element.xmime__expectedContentTypes);
	if(element.form) {
		if(*element.form == qualified)
			nameURI = URI;
		else {
			nameURI = NULL;
		      nameprefix = ":"; 
		}
	}
	if(element.elementPtr()) { // element ref
		name = element.elementPtr()->name;
		type = element.elementPtr()->type;
		if(!type) {
			type = name;
			typeprefix = "_";
		}
		if(element.elementPtr()->schemaPtr()) {
			typeURI = element.elementPtr()->schemaPtr()->targetNamespace;
			if(element.form && *element.form == unqualified)
				nameprefix = ":";
			else if(zflag == 3 && URI && typeURI && element.schemaPtr()->elementFormDefault == unqualified && sstreq(URI, typeURI))
				nameprefix = NULL;
			else
				nameURI = typeURI;
		}
		document(element.elementPtr()->annotation);
		if(element.elementPtr()->xmime__expectedContentTypes)
			fprintf(stream, "/// MTOM attachment with content types %s.\n", element.elementPtr()->xmime__expectedContentTypes);
		if(substok && element.elementPtr()->abstract) {
			fprintf(stream, "/// Reference %s to abstract element.\n", element.ref);
			gen_substitutions(URI, element);
		}
		else if(substok && element.elementPtr()->substitutionsPtr() && !element.elementPtr()->substitutionsPtr()->empty()) {
			fprintf(stream, "/// Warning: element ref '%s' stands as the head of a substitutionGroup but is not declared abstract.\n", element.ref);
			if(vflag)
				fprintf(stderr, "Warning: element ref '%s' stands as the head of a substitutionGroup but is not declared abstract\n", element.ref);
			gen_substitutions(URI, element);
		}
		else if(element.maxOccurs && strcmp(element.maxOccurs, "1")) { // maxOccurs != "1"
			const char * s = tnameptr(cflag && zflag != 1, typeprefix, typeURI, type);
			if(cflag || sflag) {
				fprintf(stream, "/// Size of the dynamic array of %s is %s..%s\n", s,
					element.minOccurs ? element.minOccurs : "1", element.maxOccurs);
				fprintf(stream, sizeformat, "int", aname(NULL, NULL, name));
				fprintf(stream, " %s", fake_union ? "0" : element.minOccurs ? element.minOccurs : "1");
				if(is_integer(element.maxOccurs))
					fprintf(stream, ":%s", element.maxOccurs);
				fprintf(stream, ";\n");
				if(cflag && zflag != 1) {
					fprintf(stream, "/// Array %s of length %s..%s\n", s, element.minOccurs ? element.minOccurs : "1", element.maxOccurs);
					fprintf(stream, elementformat, s, aname(nameprefix, nameURI, name));
				}
				else {
					fprintf(stream, "/// Pointer to array %s of length %s..%s\n", s,
					      element.minOccurs ? element.minOccurs : "1", element.maxOccurs);
				      fprintf(stream, pointerformat, s, aname(nameprefix, nameURI, name)); 
				}
			}
			else {
				fprintf(stream, "/// Vector of %s element refs with length %s..%s\n", s, element.minOccurs ? element.minOccurs : "1", element.maxOccurs);
			      if(with_union)
				      fprintf(stream, pointervectorformat, s, aname(nameprefix, nameURI, name));
			      else
				      fprintf(stream, vectorformat, s, aname(nameprefix, nameURI, name)); 
			}
		}
		else {
			fprintf(stream, "/// Element reference %s.\n", element.ref);
		      fprintf(stream, elementformat, pname((with_union && !cflag && !is_basetypeforunion(typeprefix, typeURI,
				type)) || fake_union || is_nillable(element), typeprefix, typeURI, type), aname(nameprefix, nameURI, name)); 
		}
	}
	else if(name && type) {
		if(substok && element.abstract) {
			fprintf(stream, "/// Abstract element %s of type %s.\n", name, type);
			gen_substitutions(URI, element);
		}
		else if(substok && element.substitutionsPtr() && !element.substitutionsPtr()->empty()) {
			fprintf(stream, "/// Warning: element '%s' stands as the head of a substitutionGroup but is not declared abstract.\n", name);
			if(vflag)
				fprintf(stderr, "Warning: element '%s' stands as the head of a substitutionGroup but is not declared abstract\n", name);
			gen_substitutions(URI, element);
		}
		else if(element.maxOccurs && strcmp(element.maxOccurs, "1")) { // maxOccurs != "1"
			const char * s = tnameptr(cflag && zflag != 1, NULL, URI, type);
			if(cflag || sflag) {
				fprintf(stream, "/// Size of array of %s is %s..%s\n", s, element.minOccurs ? element.minOccurs : "1", element.maxOccurs);
				fprintf(stream, sizeformat, "int", aname(NULL, NULL, name));
				fprintf(stream, " %s", fake_union ? "0" : element.minOccurs ? element.minOccurs : "1");
				if(is_integer(element.maxOccurs))
					fprintf(stream, ":%s", element.maxOccurs);
				fprintf(stream, ";\n");
				if(cflag && zflag != 1) {
					fprintf(stream, "/// Array %s of length %s..%s\n", s, element.minOccurs ? element.minOccurs : "1", element.maxOccurs);
					fprintf(stream, elementformat, s, aname(nameprefix, nameURI, name));
				}
				else {
					fprintf(stream, "/// Pointer to array %s of length %s..%s\n", s, element.minOccurs ? element.minOccurs : "1", element.maxOccurs);
				      fprintf(stream, pointerformat, s, aname(nameprefix, nameURI, name)); 
				}
			}
			else {
				fprintf(stream, "/// Vector of %s with length %s..%s\n", s, element.minOccurs ? element.minOccurs : "1", element.maxOccurs);
			      if(with_union)
				      fprintf(stream, pointervectorformat, s, aname(nameprefix, nameURI, name));
			      else
				      fprintf(stream, vectorformat, s, aname(nameprefix, nameURI, name)); 
			}
		}
		else {
			fprintf(stream, "/// Element %s of type %s.\n", name, type);
		      fprintf(stream, elementformat, pname((with_union && !cflag && !is_basetypeforunion(NULL, URI,
					     type)) || (fake_union && !element.default_) || is_nillable(element), NULL, URI, type), aname(nameprefix, nameURI, name)); 
		}
	}
	else if(name && element.simpleTypePtr()) {
		const char * s = "";
		document(element.simpleTypePtr()->annotation);
		if(element.maxOccurs && strcmp(element.maxOccurs, "1")) { // maxOccurs != "1"
			if(cflag || sflag) {
				fprintf(stream, "/// Size of %s array is %s..%s\n", name,
					element.minOccurs ? element.minOccurs : "1", element.maxOccurs);
				fprintf(stream, sizeformat, "int", aname(NULL, NULL, name));
				fprintf(stream, " %s", fake_union ? "0" : element.minOccurs ? element.minOccurs : "1");
				if(is_integer(element.maxOccurs))
					fprintf(stream, ":%s", element.maxOccurs);
				fprintf(stream, ";\n");
			}
			else {
				s = ">";
			      fprintf(stream, "/// Vector %s with length %s..%s\n", name,
				      element.minOccurs ? element.minOccurs : "1", element.maxOccurs);
			      fprintf(stream, vectorformat_open, "\n"); 
			}
		}
		gen(URI, name, *element.simpleTypePtr(), true);
		if(is_nillable(element) || ((cflag || sflag) && element.maxOccurs && strcmp(element.maxOccurs, "1")) || // maxOccurs != "1"
		   (with_union && !cflag) ||
		   (fake_union && !element.default_))
			fprintf(stream, pointerformat, s, aname(nameprefix, nameURI, name));
		else
			fprintf(stream, elementformat, s, aname(nameprefix, nameURI, name));
	}
	else if(name && element.complexTypePtr()) {
		const char * s = "}";
		document(element.complexTypePtr()->annotation);
		if(element.maxOccurs && strcmp(element.maxOccurs, "1")) { // maxOccurs != "1"
			if(cflag || sflag) {
				fprintf(stream, "/// Size of %s array is %s..%s\n", name,
					element.minOccurs ? element.minOccurs : "1", element.maxOccurs);
				fprintf(stream, sizeformat, "int", aname(NULL, NULL, name));
				fprintf(stream, " %s", fake_union ? "0" : element.minOccurs ? element.minOccurs : "1");
				if(is_integer(element.maxOccurs))
					fprintf(stream, ":%s", element.maxOccurs);
				fprintf(stream, ";\n");
			}
			else {
				s = "}>"; 
				fprintf(stream, "/// Vector %s with length %s..%s\n", name, element.minOccurs ? element.minOccurs : "1", element.maxOccurs);
			      fprintf(stream, vectorformat_open, "\n"); 
			}
		}
		gen(URI, name, *element.complexTypePtr(), true);
		if(is_nillable(element) || ((cflag || sflag) && element.maxOccurs && strcmp(element.maxOccurs, "1")) || // maxOccurs != "1"
		   (with_union && !cflag) || (fake_union && !element.default_))
			fprintf(stream, pointerformat, s, aname(nameprefix, nameURI, name));
		else
			fprintf(stream, elementformat, s, aname(nameprefix, nameURI, name));
	}
	else if(element.ref) {
		fprintf(stream, "/// Imported element reference %s.\n", element.ref);
		fprintf(stream, elementformat,
			pname((with_union && !cflag) || fake_union || is_nillable(element), "_", NULL,
				element.ref), aname(nameprefix, nameURI, element.ref));
	}
	else if(name) {
		fprintf(stream, "/// Element '%s' has no type or ref (empty or with XML content).\n", name ? name : "");
		if(element.maxOccurs && strcmp(element.maxOccurs, "1")) { // maxOccurs != "1"
			if(cflag || sflag) {
				fprintf(stream, sizeformat, "int", aname(NULL, NULL, name));
				fprintf(stream, " %s", fake_union ? "0" : element.minOccurs ? element.minOccurs : "1");
				if(is_integer(element.maxOccurs))
					fprintf(stream, ":%s", element.maxOccurs);
				fprintf(stream, ";\n");
				fprintf(stream, "/// Pointer to array of XML.\n");
				fprintf(stream, pointerformat, "_XML", aname(NULL, nameURI, name));
			}
			else {
				fprintf(stream, "/// Vector of XML with length %s..%s\n", element.minOccurs ? element.minOccurs : "1", element.maxOccurs);
			      if(with_union)
				      fprintf(stream, pointervectorformat, "_XML", aname(NULL, nameURI, name));
			      else
				      fprintf(stream, vectorformat, "_XML", aname(NULL, nameURI, name)); }
		}
		else
			fprintf(stream, elementformat, "_XML", aname(NULL, nameURI, name));
	}
	else
		fprintf(stream, "/// Element has no name, type, or ref.");
	if(!substok || (!(element.elementPtr() && element.elementPtr()->abstract) &&
	    !(element.substitutionsPtr() && !element.substitutionsPtr()->empty()) &&
	    !(element.elementPtr() && element.elementPtr()->substitutionsPtr() && !element.elementPtr()->substitutionsPtr()->empty()))) {
		if(!fake_union && !element.minOccurs && !element.nillable && !element.default_ && !element.abstract)
			fprintf(stream, " 1");
		else if(!fake_union && element.minOccurs)
			fprintf(stream, " %s", element.minOccurs);
		if(element.maxOccurs && strcmp(element.maxOccurs, "1") && is_integer(element.maxOccurs))
			fprintf(stream, ":%s", element.maxOccurs);
		if(element.default_ || (element.fixed && !fake_union &&
		    (!element.minOccurs || sstreq(element.minOccurs, "1")) && (!element.maxOccurs || sstreq(element.maxOccurs, "1")))) { 
			// determine whether the element can be assigned a default value, this is dependent on the choice of mapping for primitive types
			const char * value, * QName;
			if(element.default_) {
				value = element.default_;
				QName = element.default__;
			}
			else {
				value = element.fixed;
			    QName = element.fixed_; 
			}
			if(type) {
				const char * t = tname(NULL, typeURI ? typeURI : URI, type);
				if(!strncmp(t, "unsigned ", 9))
					t += 9;
				else if(!strncmp(t, "xsd__unsigned", 13))
					t += 13;
				else if(!strncmp(t, "xsd__", 5))
					t += 5;
				if(sstreq(t, "bool") || sstreq(t, "byte") || sstreq(t, "Byte") || sstreq(t, "char") || sstreq(t, "double") || sstreq(t, "float") || sstreq(t, "int") || 
					sstreq(t, "Int") || sstreq(t, "long") || sstreq(t, "Long") || sstreq(t, "LONG64") || sstreq(t, "short") || sstreq(t, "Short") || sstreq(t, "ULONG64"))
					fprintf(stream, " = %s", value);
				else if(!strncmp(t, "enum ", 5)) {
					const char * s;
					if(is_integer(value))
						fprintf(stream, " = %s", value);
					else if(!*value)
						fprintf(stream, " = 0");
					else if((s = enames[Pair(t+5, value)]))
						fprintf(stream, " = %s", s);
				}
				else if(sstreq(t, "char*") || sstreq(t, "char *") || // not elegant
				        sstreq(t, "std::string") || sstreq(t, "std::string*") || sstreq(t, "std::string *")) // not elegant
					fprintf(stream, " = \"%s\"", cstring(value));
				else if(sstreq(t, "xsd__QName") && QName) // QName
					fprintf(stream, " = \"%s\"", cstring(QName));
			}
			if(element.default_)
				fprintf(stream, ";\t///< Default value=\"%s\".\n", value);
			else
				fprintf(stream, ";\t///< Fixed required value=\"%s\".\n", value);
		}
		else if(element.nillable)
			fprintf(stream, ";\t///< Nullable pointer.\n");
		else if(!fake_union && (!element.minOccurs || sstreq(element.minOccurs, "1")) && (!element.maxOccurs || sstreq(element.maxOccurs, "1")))
			fprintf(stream, ";\t///< Required element.\n");
		else if(element.fixed)
			fprintf(stream, ";\t///< Fixed optional value=\"%s\".\n", element.fixed);
		else if(!fake_union && element.minOccurs && sstreq(element.minOccurs, "0") && (!element.maxOccurs || sstreq(element.maxOccurs, "1")))
			fprintf(stream, ";\t///< Optional element.\n");
		else
			fprintf(stream, ";\n");
	}
}

void Types::gen(const char * URI, const vector <xs__group> & groups)
{
	for(vector <xs__group>::const_iterator group = groups.begin(); group != groups.end(); ++group)
		gen(URI, *group);
}

void Types::gen(const char * URI, const xs__group & group)
{
	fprintf(stream, "/// Begin group %s.\n", group.name ? group.name : "");
	if(group.groupPtr()) {
		if(group.schemaPtr() == group.groupPtr()->schemaPtr())
			gen(URI, *group.groupPtr());
		else
			gen(group.groupPtr()->schemaPtr()->targetNamespace, *group.groupPtr());
	}
	if(group.all)
		gen(URI, *group.all);
	else if(group.choice)
		gen(URI, NULL, *group.choice);
	else if(group.sequence)
		gen(URI, *group.sequence);
	fprintf(stream, "/// End of group %s.\n", group.name ? group.name : "");
}

void Types::gen(const char * URI, const char * name, const xs__seqchoice & choice)
{
	const char * r = NULL, * s = NULL, * t = NULL;
	bool use_union = !uflag;
	bool wrap_union = false;
	bool tmp_union;
	if(!URI && choice.schemaPtr())
		URI = choice.schemaPtr()->targetNamespace;
	fprintf(stream, "/// CHOICE OF ELEMENTS <xs:choice");
	if(choice.minOccurs)
		fprintf(stream, " minOccurs=\"%s\"", choice.minOccurs);
	if(choice.maxOccurs)
		fprintf(stream, " maxOccurs=\"%s\"", choice.maxOccurs);
	fprintf(stream, ">\n");
	document(choice.annotation);
	for(vector <xs__contents>::const_iterator c1 = choice.__contents.begin(); c1 != choice.__contents.end();
	    ++c1) {
		if((*c1).__union == SOAP_UNION_xs__union_content_group || (*c1).__union == SOAP_UNION_xs__union_content_sequence) {
			fprintf(stream, "/// Note: <xs:choice> with embedded <xs:sequence> or <xs:group> prevents the use of a union\n");
			use_union = false;
			break;
		}
	}
	if(use_union && (cflag || sflag)) {
		for(vector <xs__contents>::const_iterator c2 = choice.__contents.begin(); c2 != choice.__contents.end(); ++c2) {
			if((*c2).__union == SOAP_UNION_xs__union_content_element && (*c2).__content.element && (*c2).__content.element->maxOccurs && strcmp((*c2).__content.element->maxOccurs, "1")) {
				fprintf(stream, "/// Note: <xs:choice> of element with maxOccurs>1 prevents the use of a union\n");
				use_union = false;
				break;
			}
		}
	}
	t = uname(URI);
	s = strstr(t, "__union");
	if(s)
		r = s+7;
	if(!r || !*r) {
		r = t;
		s = "__union";
	}
	if(choice.maxOccurs && strcmp(choice.maxOccurs, "1")) {
		if(with_union) { // Generate a wrapper when we need a union within a union
			wrap_union = true;
			fprintf(stream, "    struct __%s\n    {\n", t);
		}
		fprintf(stream, sizeformat, "int", r);
		fprintf(stream, " %s", choice.minOccurs ? choice.minOccurs : "0");
		if(is_integer(choice.maxOccurs))
			fprintf(stream, ":%s", choice.maxOccurs);
		if(cflag)
			fprintf(stream, ";\n    struct _%s\n    {\n", t);
		else
			fprintf(stream, ";\n    class _%s\n    {\n", t);
	}
	if(use_union) {
		if(!with_union || wrap_union) {
			fprintf(stream, choiceformat, "int", r);
			if(choice.minOccurs)
				fprintf(stream, " %s", choice.minOccurs);
			fprintf(stream, ";\t///< Union %s selector: set to SOAP_UNION_%s_<fieldname>%s\n", t, t,
				choice.minOccurs && sstreq(choice.minOccurs, "0") ? " or 0" : "");
			if(name)
				fprintf(stream, "/// Union for choice in type %s\n", cname(NULL, URI, name));
			fprintf(stream, "    union %s\n    {\n", t);
		}
		tmp_union = with_union;
		with_union = true;
	}
	else {tmp_union = fake_union;
	      fake_union = true; }
	gen(URI, choice.__contents);
	if(use_union) {
		with_union = tmp_union;
		if(!with_union || wrap_union)
			fprintf(stream, elementformat, "}", s+2);
	}
	else
		fake_union = tmp_union;
	if(choice.maxOccurs && strcmp(choice.maxOccurs, "1")) {
		if(use_union)
			fprintf(stream, ";\n");
		fprintf(stream, pointerformat, "}", s);
	}
	fprintf(stream, ";\n");
	if(wrap_union) {
		fprintf(stream, elementformat, "}", s);
		fprintf(stream, ";\n");
	}
	fprintf(stream, "//  END OF CHOICE\n");
}

void Types::gen(const char * URI, const vector <xs__any> & anys)
{
	for(vector <xs__any>::const_iterator any = anys.begin(); any != anys.end(); ++any)
		gen(URI, *any);
}

void Types::gen(const char * URI, const xs__any & any)
{
	fprintf(stream, "/// TODO: <any");
	if(any.namespace_)
		fprintf(stream, " namespace=\"%s\"", any.namespace_);
	if(any.minOccurs)
		fprintf(stream, " minOccurs=\"%s\"", any.minOccurs);
	if(any.maxOccurs)
		fprintf(stream, " maxOccurs=\"%s\"", any.maxOccurs);
	fprintf(
		stream,
		">\n/// TODO: Schema extensibility is user-definable.\n///       Consult the protocol documentation to change or insert declarations.\n///       Use wsdl2h option -x to remove this element.\n///       Use wsdl2h option -d for xsd__anyType DOM (soap_dom_element).\n");
	if(!xflag) {
		const char * t = tname(NULL, NULL, "xsd:any");
		if(any.maxOccurs && strcmp(any.maxOccurs, "1")) {
			fprintf(stream, "/// Size of the array of XML or DOM nodes is %s..%s\n",
				any.minOccurs ? any.minOccurs : "1",
				any.maxOccurs);
			if(cflag || sflag) {
				if(!with_union)                     {
					fprintf(stream, sizeformat, "int", "");
					fprintf(stream, "0;\n");
					fprintf(stream, pointerformat, t, "__any");
				}
				else
					fprintf(stream, elementformat, t, "__any");
			}
			else if(with_union)
				fprintf(stream, pointervectorformat, t, "__any");
			else
				fprintf(stream, vectorformat, t, "__any");
		}
		else
			fprintf(stream, elementformat, t, "__any");
		if(dflag)
			fprintf(stream, "0;\t///< Catch any element content in DOM.\n");
		else
			fprintf(stream, "0;\t///< Catch any element content in XML string.\n");
	}
}

void Types::gen(const char * URI, const xs__anyAttribute & anyAttribute)
{
	if(anyAttribute.namespace_)
		fprintf(stream, "/// <anyAttribute namespace=\"%s\">\n", anyAttribute.namespace_);
	fprintf(stream,
		"/// TODO: Schema extensibility is user-definable.\n///       Consult the protocol documentation to change or insert declarations.\n///       Use wsdl2h option -x to remove this attribute.\n///       Use wsdl2h option -d for xsd__anyAttribute DOM (soap_dom_attribute).\n");
	if(!xflag) {
		const char * t = tname(NULL, NULL, "xsd:anyAttribute");
		fprintf(stream, attributeformat, t, "__anyAttribute");
		if(dflag)
			fprintf(stream,
				";\t///< Store anyAttribute content in DOM soap_dom_attribute linked node structure.\n");
		else
			fprintf(stream, ";\t///< A placeholder that has no effect: please see comment.\n");
	}
}

void Types::gen_inh(const char * URI, const xs__complexType * complexType, bool anonymous)
{
	const xs__complexType * p = complexType;
	if(!p)
		return;
	const char * pURI;
	if(p->schemaPtr())
		pURI = p->schemaPtr()->targetNamespace;
	else
		pURI = URI;
	const char * b = cname(NULL, pURI, p->name);
	if(p->complexContent && p->complexContent->extension)
		gen_inh(URI, p->complexContent->extension->complexTypePtr(), anonymous);
	if(cflag || fflag || anonymous)
		fprintf(stream, "/// INHERITED FROM %s:\n", b);
	else if(comment_nest == 0)
		fprintf(stream, "/*  INHERITED FROM %s:\n", b);
	else
		fprintf(stream, "    INHERITED FROM %s:\n", b);
	comment_nest++;
	if(cflag || fflag)
		pURI = URI;  // if base ns != derived ns then qualify elts
	if(p->complexContent && p->complexContent->extension) {
		if(p->complexContent->extension->group)
			gen(pURI, *p->complexContent->extension->group);
		if(p->complexContent->extension->all)
			gen(pURI, *p->complexContent->extension->all);
		if(p->complexContent->extension->sequence)
			gen(pURI, *p->complexContent->extension->sequence);
		if(p->complexContent->extension->choice)
			gen(pURI, p->name, *p->complexContent->extension->choice);
		gen(pURI, p->complexContent->extension->attribute);
		gen(pURI, p->complexContent->extension->attributeGroup);
		if(p->complexContent->extension->anyAttribute)
			gen(pURI, *p->complexContent->extension->anyAttribute);
	}
	else {if(p->all)
		      gen(pURI, p->all->element);
	      else if(p->all)
		      gen(pURI, *p->all);
	      else if(p->choice)
		      gen(pURI, p->name, *p->choice);
	      else if(p->sequence)
		      gen(pURI, *p->sequence);
	      else if(p->any)
		      gen(pURI, *p->any);
	      gen(pURI, p->attribute);
	      gen(pURI, p->attributeGroup);
	      if(p->anyAttribute)
		      gen(pURI, *p->anyAttribute); }
	modify(b);
	comment_nest--;
	if(cflag || fflag || anonymous)
		fprintf(stream, "//  END OF INHERITED FROM %s\n", b);
	else if(comment_nest == 0)
		fprintf(stream, "    END OF INHERITED FROM %s */\n", b);
	else
		fprintf(stream, "    END OF INHERITED FROM %s\n", b);
}

void Types::gen_soap_array(const char * name, const char * t, const char * item, const char * type)
{
	char * tmp = NULL, * dims = NULL, size[8];
	if(type) {
		tmp = static_cast<char *>(emalloc(strlen(type)+1));
		strcpy(tmp, type);
	}
	*size = '\0';
	if(tmp)
		dims = strrchr(tmp, '[');
	if(dims)
		*dims++ = '\0';
	fprintf(stream, "/// SOAP encoded array of %s\n", tmp ? tmp : "xs:anyType");
	if(cflag)
		fprintf(stream, "struct %s\n{\n", t);
	else if(pflag)
		fprintf(stream, "class %s : public xsd__anyType\n{ public:\n", t);
	else
		fprintf(stream, "class %s\n{ public:\n", t);
	if(dims) {
		char * s = strchr(dims, ']');
		if(s && s != dims)
			sprintf(size, "[%d]", (int)(s-dims+1));
	}
	if(tmp) {
		if(strchr(tmp, '[') != NULL)          {
			gen_soap_array(NULL, "", item, tmp);
			fprintf(stream, arrayformat, "}", item ? aname(NULL, NULL, item) : "");
			fprintf(stream, ";\n");
		}
		else {
			const char * s = pname(!is_basetype(NULL, NULL, tmp), NULL, NULL, tmp);
		      fprintf(stream, "/// Pointer to array of %s.\n", s);
		      fprintf(stream, arrayformat, s, item ? aname(NULL, NULL, item) : "");
		      fprintf(stream, ";\n"); 
		}
		if(*size)
			fprintf(stream, "/// Size of the multidimensional dynamic array with dimensions=%s\n", size);
		else
			fprintf(stream, "/// Size of the dynamic array.\n");
		fprintf(stream, arraysizeformat, "int", size);
		fprintf(stream, ";\n/// Offset for partially transmitted arrays (uncomment only when required).\n");
		fprintf(stream, arrayoffsetformat, "int", size);
		fprintf(stream, ";\n");
	}
	else { // TODO: how to handle generic SOAP array? E.g. as an array of anyType?
		fprintf(stream, "// TODO: add declarations to handle generic SOAP-ENC:Array (array of anyType)\n");
	}
	free(tmp);
}

void Types::gen_substitutions(const char * URI, const xs__element & element)
{
	const std::vector <xs__element *> * substitutions;
	const char * name;
	const char * r = NULL, * s = NULL;
	bool use_union = !uflag;
	bool wrap_union = false;
	bool tmp_union;
	bool abstract = false;
	if(!URI && element.schemaPtr())
		URI = element.schemaPtr()->targetNamespace;
	if(element.elementPtr()) {
		name = element.elementPtr()->name;
		substitutions = element.elementPtr()->substitutionsPtr();
		abstract = element.elementPtr()->abstract;
	}
	else {
		name = element.name;
	      substitutions = element.substitutionsPtr();
	      abstract = element.abstract; 
	}
	fprintf(stream, "/// CHOICE OF ELEMENTS FOR <xs:element substitutionGroup=\"%s\"", name);
	if(element.minOccurs)
		fprintf(stream, " minOccurs=\"%s\"", element.minOccurs);
	if(element.maxOccurs)
		fprintf(stream, " maxOccurs=\"%s\"", element.maxOccurs);
	fprintf(stream, "> with elements");
	for(std::vector <xs__element *>::const_iterator i1 = substitutions->begin(); i1 != substitutions->end(); ++i1)
		fprintf(stream, " %s", (*i1)->name);
	fprintf(stream, "\n");
	if(use_union) {
		const char * t = uname(URI);
		// TODO: could reuse the union instead of generating a new one each time!
		s = strstr(t, "__union");
		SETIFZ(s, "__union");
		r = aname(NULL, NULL, name);
		if(element.maxOccurs && strcmp(element.maxOccurs, "1")) {
			if(with_union) { // Generate a wrapper when we need a union within a union
				wrap_union = true;
				fprintf(stream, "    struct __%s\n    {\n", t);
			}
			fprintf(stream, sizeformat, "int", r);
			fprintf(stream, " %s", element.minOccurs ? element.minOccurs : "0");
			if(element.maxOccurs && strcmp(element.maxOccurs, "1") && is_integer(element.maxOccurs))
				fprintf(stream, ":%s", element.maxOccurs);
			if(cflag)
				fprintf(stream, ";\n    struct _%s\n    {\n", t);
			else
				fprintf(stream, ";\n    class _%s\n    {\n", t);
		}
		if(!with_union || wrap_union) {
			fprintf(stream, choiceformat, "int", r);
			fprintf(stream, " %s", element.minOccurs ? element.minOccurs : "0");
			fprintf(stream, ";\t///< Union %s selector: set to SOAP_UNION_%s_<fieldname>%s\n", t, t,
				element.minOccurs && sstreq(element.minOccurs, "0") ? " or 0" : "");
			fprintf(stream, "/// Union for substitutionGroup=\"%s\"\n", name);
			fprintf(stream, "    union %s\n    {\n", t);
		}
		tmp_union = with_union;
		with_union = true;
	}
	else {tmp_union = fake_union;
	      fake_union = true; }
	if(!abstract)
		gen(URI, element, false);
	for(vector <xs__element *>::const_iterator i2 = substitutions->begin(); i2 != substitutions->end(); ++i2)
		gen(URI, *(*i2), true);  // substitutions are recursive?
	if(use_union) {
		with_union = tmp_union;
		if(!with_union || wrap_union) {
			fprintf(stream, elementformat, "}", s);
			fprintf(stream, ";\n");
		}
		if(element.maxOccurs && strcmp(element.maxOccurs, "1")) {
			fprintf(stream, ";\n");
			fprintf(stream, pointerformat, "}", s);
			fprintf(stream, ";\n");
		}
		if(wrap_union) {
			fprintf(stream, elementformat, "}", s);
			fprintf(stream, ";\n");
		}
	}
	else
		fake_union = tmp_union;
	fprintf(stream, "//  END OF CHOICE\n");
}

void Types::document(const xs__annotation * annotation)
{
	if(annotation && annotation->documentation) {
		fprintf(stream, "/// @brief");
		documentation(annotation->documentation);
	}
}

void Types::modify(const char * name)
{ // TODO: consider support removal of elements/attributes with ns__X = $- Y
	const char * s = modtypemap[name];
	if(s) {
		while(*s)        {
			if(*s++ == '$')
				fprintf(stream, "/// Member declared in %s\n   ", mapfile);
			s = format(s);
		}
	}
}

const char * Types::format(const char * text)
{
	const char * s = text;
	if(!s)
		return NULL;
	while(*s && *s != '$') {
		if(*s == '\\') {
			switch(s[1]) {
			    case 'n': fputc('\n', stream); break;
			    case 't': fputc('\t', stream); break;
			    default: fputc(s[1], stream);
			}
			s++;
		}
		else
			fputc(*s, stream);
		s++;
	}
	fputc('\n', stream);
	return s;
}
//
//	Type map file parsing
//
static char * getline(char * s, size_t n, FILE * fd)
{
	int c;
	char * t = s;
	if(n)
		n--;
	for(;; ) {
		c = fgetc(fd);
		if(c == '\r')
			continue;
		if(c == '\\') {
			c = fgetc(fd);
			if(c == '\r')
				c = fgetc(fd);
			if(c < ' ')
				continue;
			if(n) {
				*t++ = '\\';
				n--;
			}
		}
		if(c == '\n' || c == EOF)
			break;
		if(n) {
			*t++ = c;
			n--;
		}
	}
	*t++ = '\0';
	if(!*s && c == EOF)
		return NULL;
	return s;
}

static const char * nonblank(const char * s)
{
	while(*s && isspace(*s))
		s++;
	return s;
}

static const char * fill(char * t, int n, const char * s, int e)
{
	int i = n;
	s = nonblank(s);
	while(*s && *s != e && --i)
		*t++ = *s++;
	while(*s && *s != e)
		s++;
	if(*s)
		s++;
	i = n-i;
	if(i == 0)
		*t = '\0';
	else {
		while(isspace(*--t) && i--)
		      ;
      t[1] = '\0'; 
	}
	return s;
}
//
//	Miscellaneous
//
static const char * utf8(char * t, const char * s)
{
	unsigned int c = 0;
	unsigned int c1, c2, c3, c4;
	c = (unsigned char)*s;
	if(c >= 0x80) {
		c1 = (unsigned char)*++s;
		if(c1 < 0x80)
			s--;
		else {
			c1 &= 0x3F;
			if(c < 0xE0)
				c = ((c&0x1F)<<6)|c1;
			else {
				c2 = (unsigned char)*++s&0x3F;
				if(c < 0xF0)
					c = ((c&0x0F)<<12)|(c1<<6)|c2;
				else {
					c3 = (unsigned char)*++s&0x3F;
					if(c < 0xF8)
						c = ((c&0x07)<<18)|(c1<<12)|(c2<<6)|c3;
					else {
						c4 = (unsigned char)*++s&0x3F;
						if(c < 0xFC)
							c = ((c&0x03)<<24)|(c1<<18)|(c2<<12)|(c3<<6)|c4;
						else
							c = ((c&0x01)<<30)|(c1<<24)|(c2<<18)|(c3<<12)|(c4<<6)|*++s&0x3F; 
					}
				}
			}
		}
	}
	sprintf(t, "_x%.4x", c);
	return s;
}

static const char * FASTCALL cstring(const char * s)
{
	size_t n;
	char * t;
	const char * r;
	for(n = 0, r = s; *r; n++, r++)
		if(*r == '"' || *r == '\\')
			n++;
		else if(*r < 32)
			n += 3;
	r = t = static_cast<char *>(emalloc(n+1));
	for(; *s; s++) {
		if(*s == '"' || *s == '\\') {
			*t++ = '\\';
			*t++ = *s;
		}
		else if(*s < 32) {
			sprintf(t, "\\%03o", (unsigned int)(unsigned char)*s);
			t += 4;
		}
		else
			*t++ = *s;
	}
	*t = '\0';
	return r;
}

static const char * FASTCALL xstring(const char * s)
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
	r = t = static_cast<char *>(emalloc(n+1));
	for(; *s; s++) {
		if(*s < 32 || *s >= 127)                 {
			sprintf(t, "&#%.2x;", (unsigned char)*s);
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

static LONG64 to_integer(const char * s)
{
	LONG64 n;
#ifdef HAVE_STRTOLL
	char * r;
	n = strtoll(s, &r, 10);
#else
 #ifdef HAVE_SSCANF
	sscanf(s, SOAP_LONG_FORMAT, &n);
 #endif
#endif
	return n;
}

static bool is_integer(const char * s)
{
	if((*s == '-' || *s == '+') && s[1])
		s++;
	if(!*s || strlen(s) > 20)
		return false;
	while(*s && isdigit(*s))
		s++;
	return *s == '\0';
}

static void documentation(const char * text)
{
	const char * s = text;
	bool flag = true;
	if(!s)
		return;
	while(*s) {
		switch(*s) {
		    case '\n':
		    case '\t':
		    case ' ':
			flag = true;
			break;
		    default:
			if(*s > 32) {
				if(flag) {
					fputc(' ', stream);
					flag = false;
				}
				fputc(*s, stream);
			}
		}
		s++;
	}
	fputc('\n', stream);
}
//
//	Allocation
//
void * emalloc(size_t size)
{
	void * p = malloc(size);
	if(!p) {
		fprintf(stderr, "Error: Malloc failed\n");
		exit(1);
	}
	return p;
}

char * estrdup(const char * s)
{
	char * t = static_cast<char *>(emalloc(strlen(s)+1));
	strcpy(t, s);
	return t;
}

char * estrdupf(const char * s)
{
	char * t = static_cast<char *>(emalloc(strlen(s)+1));
	char * p;
	for(p = t; *s; p++, s++) {
		if(s[0] == '/' && s[1] == '*') {
			for(s += 2; s[0] && s[1]; s++) {
				if(s[0] == '*' && s[1] == '/') {
					s++;
					break;
				}
			}
			continue;
		}
		*p = *s;
	}
	*p = '\0';
	return t;
}
