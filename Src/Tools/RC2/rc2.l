/*
	RC2.L
	Part of project Papyrus
	Лексический анализатор для языка описания броузеров
*/
%x incl

%{
#ifndef _PPRC2_
#define _PPRC2_
#endif
#define YY_NEVER_INTERACTIVE 1
    #include <stdio.h>
    #include <string.h>
    #include <stdlib.h>
	#define min(a,b)            (((a) < (b)) ? (a) : (b))
    #include "rc2.h"
    #include "rc2.tab.hpp"
    long yyline = 1;
    long yybrak = 0;
	typedef struct {
		YY_BUFFER_STATE yyin_buf;
		char fname[256];
		long yyin_line;
	} YYIN_STR;
	YYIN_STR * yyin_struct = 0;
	long yyin_cnt = 0;

	void yyerror(const char * str);
%}

EOL         "\n"
WHITE       [ \t]([ \t])*
STRING      \"[^"\n]*["\n]
REAL		[0-9]+([.][0-9]*)([Ee][+-]?[0-9]+)?
DIGIT       [0-9]
HEXDIGIT    [0-9AaBbCcDdEeFf]
INTEGER     ("-"|{DIGIT})*{DIGIT}+
HEXINT      0[xX][0-9AaBbCcDdEeFf]+
NDIGIT      [A-Z_a-z@]
IDENT       {NDIGIT}({NDIGIT}|{DIGIT})*
COMMENT     "//"(.)*"\n"
COLOR       "#"{HEXDIGIT}{HEXDIGIT}{HEXDIGIT}{HEXDIGIT}{HEXDIGIT}{HEXDIGIT}

R_BROWSER	"browser"
R_GROUP		"group"
R_TOOLBAR	"toolbar"
R_SEPARATOR	"separator"
R_CROSSTAB	"crosstab"
R_NOTYPE	"notype"
R_ZSTRING	"zstring"
R_CHR		"char"
R_EXPORT	"export"
R_FILE      "file"
R_DECLARE_CMD "declare command"

%%
{R_BROWSER}	{ return T_BROWSER; }
{R_GROUP}	{ return T_GROUP; }
{R_TOOLBAR}	{ return T_TOOLBAR; }
{R_CROSSTAB}	{ return T_CROSSTAB; }
{R_SEPARATOR}	{ return T_SEPARATOR; }
{R_NOTYPE}	{ return T_NOTYPE; }
{R_ZSTRING}	{ return T_ZSTRING; }
{R_CHR}		{ return T_CHR; }
{R_EXPORT}	{ return T_EXPORT; }
{R_DECLARE_CMD} { return T_DECLARE_CMD; }
"job"       { return T_JOB; }
"ppobject"  { return T_OBJ; }
"cmd"       { return T_CMD; }
"record"    { return T_RECORD; }
"format"    { return T_FORMAT; }
"view"      { return T_VIEW;   }
"filter"    { return T_FILTER; }
"ctrlmenu"  { return T_CTRLMENU; }
"reportstub" { return T_REPORTSTUB; }
"dl600data"  { return T_DL600DATA;  }
"hidden"     { return T_HIDDEN; }
"bitmap"     { return T_BITMAP; }
"drawvector" { return T_DRAWVECTOR; }
"file"       { return T_FILE; }
"noreplacecolor" { return T_NOREPLACECOLOR; } // @v12.4.10
{EOL}		{ ++yyline; }
{COMMENT}	{ ++yyline; }
{WHITE}		{ ; }

{STRING}	{
	size_t len = MIN(yyleng - 1, sizeof(yylval.sval)-1);
	memcpy(yylval.sval, yytext + 1, len - 1);
	yylval.sval[len - 1] = 0;
	return T_STR;
}
{INTEGER}   { yylval.lval = atol(yytext); return T_TINT; }
{HEXINT}    { yylval.lval = strtol(yytext, 0, 0); return T_TINT; }
{IDENT}     {
	size_t len = MIN(yyleng, sizeof(yylval.sval)-1);
	memcpy(yylval.sval, yytext, len);
	yylval.sval[len] = 0;
	return T_IDENT;
}
{COLOR}     { 
	size_t len = MIN(yyleng, sizeof(yylval.sval)-1);
	SString temp_buf;
	temp_buf.CatN(yytext, len);
	yylval.scolor_val.FromStr(temp_buf);
	return T_COLOR; 
}
"["         { return '['; }
"]"         { return ']'; }
"{"         { return '{'; }
"}"         { return '}'; }
"="         { return '='; }
[-,;+*/.:()?<>~|] { return yytext[0]; }
<incl>[ \t]+
<incl>[^ \t\n]+	{
		yyin_cnt++;
		yyin_struct = (YYIN_STR *)SAlloc::R(yyin_struct, yyin_cnt*sizeof(YYIN_STR));
		if(!yyin_struct)
			yyerror("Not enough memory");
		yyin_struct[yyin_cnt-1].yyin_buf = YY_CURRENT_BUFFER;
		yyin_struct[yyin_cnt-1].yyin_line = yyline;
		strcpy(yyin_struct[yyin_cnt-1].fname, yytext);
		yyin = fopen(yytext, "r");
		if(!yyin)
			yyerror("Could not open include file");
		yyline = 1;
		yy_switch_to_buffer(yy_create_buffer(yyin, YY_BUF_SIZE));
		BEGIN(INITIAL);
	}
<<EOF>> {
		if(--yyin_cnt < 0 ) {
			SAlloc::F(yyin_struct);
			yyterminate();
		}
		else {
			yy_delete_buffer(YY_CURRENT_BUFFER);
			yy_switch_to_buffer(yyin_struct[yyin_cnt].yyin_buf);
			yyline = yyin_struct[yyin_cnt].yyin_line;
		}
	}

%%

int yywrap(void)
{
    return 1;
}
