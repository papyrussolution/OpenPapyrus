/*
        init2.c

        Symbol table initialization.

   gSOAP XML Web services tools
   Copyright (C) 2000-2008, Robert van Engelen, Genivia Inc. All Rights Reserved.
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

#include "soapcpp2.h"
#include "soapcpp2_yacc.tab.hpp"

typedef struct Keyword { 
	const char * s;      /* name */
	Token t;      /* token */
} Keyword;

static Keyword keywords[] = {       
	{ "asm",                NONE },
	{ "auto",               AUTO },
	{ "bool",               TOK_BOOL },
	{ "break",              BREAK },
	{ "case",               CASE },
	{ "catch",              NONE },
	{ "char",               TOK_CHAR },
	{ "class",              CLASS },
	{ "const",              TOK_CONST },
	{ "const_cast", NONE },
	{ "continue",   CONTINUE },
	{ "default",    DEFAULT },
	{ "delete",             NONE },
	{ "do",                 DO },
	{ "double",             TOK_DOUBLE },
	{ "dynamic_cast",       NONE },
	{ "else",               ELSE },
	{ "enum",               ENUM },
	{ "errno",              NONE },
	{ "explicit",           EXPLICIT },
	{ "export",             NONE },
	{ "extern",             EXTERN },
	{ "false",              CFALSE },
	{ "float",              TOK_FLOAT },
	{ "for",                FOR },
	{ "friend",             FRIEND },
	{ "goto",               GOTO },
	{ "if",                 IF },
	{ "inline",             INLINE },
	{ "int",                TOK_INT },
	{ "int8_t",             TOK_CHAR },
	{ "int16_t",            TOK_SHORT },
	{ "int32_t",            TOK_INT },
	{ "int64_t",            LLONG },
	{ "long",               TOK_LONG },
	{ "LONG64",             LLONG },
	{ "mutable",            NONE },
	{ "namespace",          NAMESPACE },
	{ "new",                NONE },
	{ "NULL",               null },
	{ "operator",           OPERATOR },
	{ "private",            PRIVATE },
	{ "protected",          PROTECTED },
	{ "public",             PUBLIC },
	{ "register",           REGISTER },
	{ "reinterpret_cast",   NONE },
	{ "restrict",           NONE },
	{ "return",             RETURN },
	{ "short",              TOK_SHORT },
	{ "signed",             SIGNED },
	{ "size_t",             TOK_SIZE },
	{ "sizeof",             SIZEOF },
	{ "static",             STATIC },
	{ "static_cast",        NONE },
	{ "struct",             STRUCT },
	{ "switch",             SWITCH },
	{ "template",           TEMPLATE },
	{ "this",               NONE },
	{ "throw",              NONE },
	{ "time_t",             TIME },
	{ "true",               CTRUE },
	{ "typedef",            TYPEDEF },
	{ "typeid",             NONE },
	{ "typename",           TYPENAME },
	{ "uint8_t",            TOK_UCHAR },
	{ "uint16_t",           TOK_USHORT },
	{ "uint32_t",           TOK_UINT },
	{ "uint64_t",           ULLONG },
	{ "ULONG64",            ULLONG },
	{ "union",              UNION },
	{ "unsigned",           UNSIGNED },
	{ "using",              USING },
	{ "virtual",            VIRTUAL },
	{ "void",               TOK_VOID },
	{ "volatile",           VOLATILE },
	{ "wchar_t",            TOK_WCHAR },
	{ "while",              WHILE },

	{ "operator!",          NONE },
	{ "operator~",          NONE },
	{ "operator=",          NONE },
	{ "operator+=",         NONE },
	{ "operator-=",         NONE },
	{ "operator*=",         NONE },
	{ "operator/=",         NONE },
	{ "operator%=",         NONE },
	{ "operator&=",         NONE },
	{ "operator^=",         NONE },
	{ "operator|=",         NONE },
	{ "operator<<=",        NONE },
	{ "operator>>=",        NONE },
	{ "operator||",         NONE },
	{ "operator&&",         NONE },
	{ "operator|",          NONE },
	{ "operator^",          NONE },
	{ "operator&",          NONE },
	{ "operator==",         NONE },
	{ "operator!=",         NONE },
	{ "operator<",          NONE },
	{ "operator<=",         NONE },
	{ "operator>",          NONE },
	{ "operator>=",         NONE },
	{ "operator<<",         NONE },
	{ "operator>>",         NONE },
	{ "operator+",          NONE },
	{ "operator-",          NONE },
	{ "operator*",          NONE },
	{ "operator/",          NONE },
	{ "operator%",          NONE },
	{ "operator++",         NONE },
	{ "operator--",         NONE },
	{ "operator->",         NONE },
	{ "operator[]",         NONE },
	{ "operator()",         NONE },

	{ "mustUnderstand",     MUSTUNDERSTAND },

	{ "soap",               ID },
	{ "SOAP_ENV__Header",   ID },
	{ "dummy",              ID },
	{ "soap_header",        ID },

	{ "SOAP_ENV__Fault",    ID },
	{ "SOAP_ENV__Code",     ID },
	{ "SOAP_ENV__Subcode",  ID },
	{ "SOAP_ENV__Reason",   ID },
	{ "SOAP_ENV__Text",     ID },
	{ "SOAP_ENV__Detail",   ID },
	{ "SOAP_ENV__Value",    ID },
	{ "SOAP_ENV__Node",     ID },
	{ "SOAP_ENV__Role",     ID },
	{ "faultcode",          ID },
	{ "faultstring",        ID },
	{ "faultactor",         ID },
	{ "detail",             ID },
	{ "__type",             ID },
	{ "fault",              ID },
	{ "__any",              ID },

	{ "_QName",             ID },
	{ "_XML",               ID },
	{ "std::string",        TOK_TYPE },
	{ "std::wstring",       TOK_TYPE },

	{ "/*?*/",              NONE },

	{ 0,                    0 }};
/*
   init - initialize symbol table with predefined keywords
 */
void init(void)
{
	for(struct Keyword * k = keywords; k->s; k++)
		install(k->s, k->t);
}
