/*
	CSSTOKENIZER.L
*/
%{
#include <slib.h>
#include <tv.h>
//#pragma hdrstop
#include "cssparser.h"
#include "cssgrammar.tab.hpp"
%}
%option case-insensitive
%option noyywrap
%option 8bit
%option stack
%s mediaquery
%s forkeyword

h               [0-9a-fA-F]
nonascii        [\200-\377]
unicode         \\{h}{1,6}[ \t\r\n\f]?
escape          {unicode}|\\[ -~\200-\377]
nmstart         [_a-zA-Z]|{nonascii}|{escape}
nmchar          [_a-zA-Z0-9-]|{nonascii}|{escape}
string1         \"([\t !#$%&(-~]|\\{nl}|\'|{nonascii}|{escape})*\"
string2         \'([\t !#$%&(-~]|\\{nl}|\"|{nonascii}|{escape})*\'
hexcolor        {h}{3}|{h}{6}

ident           -?{nmstart}{nmchar}*
name            {nmchar}+
num             [0-9]+|[0-9]*"."[0-9]+
intnum          [0-9]+
string          {string1}|{string2}
url             ([!#$%&*-~]|{nonascii}|{escape})*
w               [ \t\r\n\f]*
nl              \n|\r\n|\r|\f
range           \?{1,6}|{h}(\?{0,5}|{h}(\?{0,4}|{h}(\?{0,3}|{h}(\?{0,2}|{h}(\??|{h})))))
nth             (-?[0-9]*n[\+-][0-9]+)|(-?[0-9]*n)

%%

\/\*[^*]*\*+([^/*][^*]*\*+)*\/  /* ignore comments */

[ \t\r\n\f]+            { return WHITESPACE; }
"<!--"                  { return SGML_CD; }
"-->"                   { return SGML_CD; }
"~="                    { return INCLUDES; }
"|="                    { return DASHMATCH; }
"^="                    { return BEGINSWITH; }
"$="                    { return ENDSWITH; }
"*="                    { return CONTAINS; }
<mediaquery>"not"       { return MEDIA_NOT; }
<mediaquery>"only"      { return MEDIA_ONLY; }
<mediaquery>"and"       { return MEDIA_AND; }
<forkeyword>"for"       { BEGIN(mediaquery); return VARIABLES_FOR; }
{string}                { return STRING; }
{ident}                 { return IDENT; }
{nth}                   { return NTH; }
"#"{hexcolor}           { return HEX; }
"#"{ident}              { return IDSEL; }
"@import"               { BEGIN(mediaquery); return IMPORT_SYM; }
"@page"                 { return PAGE_SYM; }
"@media"                { BEGIN(mediaquery); return MEDIA_SYM; }
"@font-face"            { return FONT_FACE_SYM; }
"@charset"              { return CHARSET_SYM; }
"@namespace"            { return NAMESPACE_SYM; }
"@-webkit-rule"         { return WEBKIT_RULE_SYM; }
"@-webkit-decls"        { return WEBKIT_DECLS_SYM; }
"@-webkit-value"        { return WEBKIT_VALUE_SYM; }
"@-webkit-mediaquery"   { BEGIN(mediaquery); return WEBKIT_MEDIAQUERY_SYM; }
"@-webkit-selector"     { return WEBKIT_SELECTOR_SYM; }
"@-webkit-variables"    { BEGIN(mediaquery); return WEBKIT_VARIABLES_SYM; }
"@-webkit-define"       { BEGIN(forkeyword); return WEBKIT_DEFINE_SYM; }
"@-webkit-variables-decls" { return WEBKIT_VARIABLES_DECLS_SYM; }
"@-webkit-keyframes"    { return WEBKIT_KEYFRAMES_SYM; }
"@-webkit-keyframe-rule" { return WEBKIT_KEYFRAME_RULE_SYM; }
"@"{ident}              { return ATKEYWORD; }
"!"{w}"important"       { return IMPORTANT_SYM; }
{num}em                 { return EMS; }
{num}__qem              { return QEMS; } /* quirky ems */
{num}ex                 { return EXS; }
{num}px                 { return PXS; }
{num}cm                 { return CMS; }
{num}mm                 { return MMS; }
{num}in                 { return INS; }
{num}pt                 { return PTS; }
{num}pc                 { return PCS; }
{num}deg                { return DEGS; }
{num}rad                { return RADS; }
{num}grad               { return GRADS; }
{num}turn               { return TURNS; }
{num}ms                 { return MSECS; }
{num}s                  { return SECS; }
{num}Hz                 { return HERZ; }
{num}kHz                { return KHERZ; }
{num}{ident}            { return DIMEN; }
{num}%+                 { return PERCENTAGE; }
{intnum}                { return INTEGER; }
{num}                   { return FLOATTOKEN; }
"not("                  { return NOTFUNCTION; }
"url("{w}{string}{w}")" { return URI; }
"url("{w}{url}{w}")"    { return URI; }
"-webkit-var("{w}{ident}{w}")" { return VARCALL; }
{ident}"("              { return FUNCTION; }
U\+{range}              { return UNICODERANGE; }
U\+{h}{1,6}-{h}{1,6}    { return UNICODERANGE; }
<mediaquery>"{"         |
<mediaquery>";"         {BEGIN(INITIAL); return *yytext; }
.                       { return *yytext; }

%%
