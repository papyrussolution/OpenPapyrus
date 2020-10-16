/*
	DL200.L
	Copytight (c) A.Sobolev 2003, 2009, 2020
*/
%x incl

%{
#include <pp.h>
//#include "dl200.tab.h"
#include "dl200.tab.cpp.h"
#include "dl200_yy.h"


long yyline = 1;
long yybrace_count = 0;
YYIN_STR * yyin_struct = 0;
long yyin_count = 0;
%}

EOL         "\n"
WHITE       [ \t]*
STRING      \"[^"\n]*["\n]
PERIODOFFS  ((-)?[0-9]+)[DMQYdmqy]
REALVAL     [0-9]+([.][0-9]*)([Ee][+-]?[0-9]+)?
INTVAL      [0-9]+
HEXINT      0[xX][0-9A-Fa-f]+
NDIGIT      [A-Z_a-z]
IDENT       [A-Z_a-z]([0-9A-Z_a-z])*
COMMENT     "//"(.)*"\n"
DATE        [0-9]{1,2}[./-][0-9]{1,2}[./-][0-9]{2,4}
DATERANGE   ({DATE}".."{DATE}) | ({DATE}"..") | (".."{DATE}) | {DATE}
PSEUDOVAR   "@"{IDENT}
ACCNUMBER   ([0-9]+(.[0-9]+){0,2})
ACC_CODE    "~"{IDENT}
ACCDEF      {ACCNUMBER}|{ACC_CODE}([Oo][123])?
ACCVAL      (\x5b)({ACCDEF})?(([DdCc])?([IiRrTtSs])?)?(:{ACCDEF})?(,{PERIODOFFS})?(\x5d)

%%
"IMPORT"    BEGIN(incl);
"GROUP"     { return T_GROUP; }
"DATA"      { return T_DATA;  }
"HEADER"    { return T_HEADER; }
"INT"       { yylval.ival = DL2CIT_INT; return T_COLUMNTYPE; }
"REAL"      { yylval.ival = DL2CIT_REAL; return T_COLUMNTYPE; }
"STRING"    { yylval.ival = DL2CIT_STRING; return T_COLUMNTYPE; }
"DATE"      { yylval.ival = DL2CIT_DATE; return T_COLUMNTYPE; }
"TIME"      { yylval.ival = DL2CIT_TIME; return T_COLUMNTYPE; }
"PERIOD"    { yylval.ival = DL2CIT_PERIOD; return T_COLUMNTYPE; }
{INTVAL}    { yylval.ival = atoi(yytext); return T_INTVAL;  }
{REALVAL}   { yylval.rval = atof(yytext); return T_REALVAL; }
"{"         { yybrace_count++; return T_LBRACE; }
"}"         { yybrace_count--; return T_RBRACE; }
";"         {
	return T_SEMICOLON;
}
"="         {
	return T_ASSIGN;
}
{ACCVAL} {
	yytext[yyleng] = 0;
	yylval.ciacc.GetFromStr(yytext);
	return T_ACCVAL;
}
{IDENT}     {
	size_t len = MIN(yyleng, sizeof(yylval.name)-1);
	memcpy(yylval.name, yytext, len);
	yylval.name[len] = 0;
	return T_IDENT;
}
{PSEUDOVAR} {
	yytext[yyleng] = 0;
	yylval.sval = yytext+1;
	return T_PSEUDOVAR;
}
[-,+*/.:()?<>~] {
	return yytext[0];
}
{EOL}       { yyline++; }
{COMMENT}   { yyline++; }
{WHITE}     { ; }
{STRING}    {
	yytext[yyleng-1] = 0;
	yylval.sval = yytext+1;
	return T_STRING;
}

<incl>[ \t]+
<incl>[^ \t\n]+ {
	yyin_count++;
	yyin_struct = (YYIN_STR *) realloc(yyin_struct, yyin_count*sizeof(YYIN_STR));
	if(!yyin_struct)
		yyerror("Not enough memory");
	yyin_struct[yyin_count-1].yyin_buf = YY_CURRENT_BUFFER;
	yyin_struct[yyin_count-1].yyin_line = yyline;
	strnzcpy(yyin_struct[yyin_count-1].fname, yytext, _MAX_PATH);
	yyin = fopen(yytext, "r");
	if(!yyin)
		yyerror("Could not open include file");
	yyline = 1;
	yy_switch_to_buffer(yy_create_buffer(yyin, YY_BUF_SIZE));
	BEGIN(INITIAL);
}
<<EOF>> {
	if(--yyin_count < 0) {
		free(yyin_struct);
		yyterminate();
	}
	else {
		yy_delete_buffer(YY_CURRENT_BUFFER);
		yy_switch_to_buffer(yyin_struct[yyin_count].yyin_buf);
		yyline = yyin_struct[yyin_count].yyin_line;
	}
}
%%

int yywrap(void)
{
    return 1;
}
