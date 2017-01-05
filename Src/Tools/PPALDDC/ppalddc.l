/*
	PPALDDC.L
	Copyright (c) A.Sobolev 2000-2005
	Part of project Papyrus
	Lexer of PPALDD language
*/
%x incl

%{
#ifndef _PPALDDC_
#define _PPALDDC_
#endif
    #include <stdio.h>
    #include <stdlib.h>
    #include <ppaldd.h>
    #include "ppalddc.tab.cpp.h"
    long yyline = 1;
    long yybrak = 0;
	typedef struct {
		YY_BUFFER_STATE yyin_buf;
		char fname[_MAX_PATH];
		long yyin_line;
	} YYIN_STR;
	YYIN_STR * yyin_struct = 0;
	long yyin_cnt = 0;

	void yyerror(char * str);
%}

EOL         "\n"
WHITE       [ \t]([ \t])*
STRING      \"[^"\n]*["\n]
R_VALUE     [0-9]+([.][0-9]*)([Ee][+-]?[0-9]+)?
DIGIT       [0-9]
INTEGER     {DIGIT}+
HEXINT      0[xX][0-9AaBbCcDdEeFf]+
NDIGIT      [A-Z_a-z@]
IDENT       {NDIGIT}({NDIGIT}|{DIGIT})*
DATA        "SData"|"data"
FILTER      "SFilter"
COMMENT     "//"(.)*"\n"
L_BR        "{"
R_BR        "}"
R_INT16     "short"|"word"|"int16"
R_INT32     "int"|"int32"
R_COUNTER   "autoinc"
R_LONG      "long"
R_FLOAT     "float"
R_DOUBLE    "double"
R_STRING    "string"
R_DATE      "date"
R_TIME      "time"
R_MONEY     "money"
R_DEC       "decimal"
R_LINK      "link"
R_KEY       "key"
R_ARRAY_LONG "array long"
R_ARRAY_DBL  "array double"
R_FMT       [~][<>|]?[*]?{DIGIT}*([.][0-9]{DIGIT}*)?([@#$&]{NDIGIT}({NDIGIT}{NDIGIT}?)?)?

%%
{R_DOUBLE}  { return T_TDBL;   }
{R_LONG}    { return T_TLONG;  }
{R_STRING}  { return T_TSTR;   }
{R_DEC}     { return T_TDEC;   }
{R_MONEY}   { return T_TMON;   }
{R_INT32}   { return T_TINT;   }
{R_INT16}   { return T_TINT16; }
{R_DATE}    { return T_TDATE;  }
{R_TIME}    { return T_TTIME;  }
{R_FLOAT}   { return T_TFLOAT; }
{R_LINK}    { return T_TLINK;  }
{R_KEY}     { return T_TKEY;   }
{R_ARRAY_LONG} {return T_ARRAY_LONG;}
{R_ARRAY_DBL}  {return T_ARRAY_DBL;}
{R_COUNTER} {return T_COUNTER;}

"declare"   { return T_DECLARE; }
"IMPORT"    BEGIN(incl);
"(double)"  { return TO_DBL;    }
"(int)"     { return TO_INT;    }
"format"    { return TO_STR;    }
"formatperiod" { return T_FORMATPERIOD; }
"PrintableBarcode" { return T_PRINTABLEBARCODE; }
"abs"       { return T_ABS;     }
"Abs"       { return T_ABS;     }
"left"      { return T_LEFT;    }
"middle"    { return T_MIDL;    }
"right"     { return T_RIGHT;   }
"wrap"      { return T_WRAP;    }
"sqrt"      { return T_SQRT;    }
"Money2Str" { return TS_MONEY;  }
"Num2Str"   { return TS_NUMBER; }
"Date2Str"  { return TS_DATE;   }
"Date2WStr" { return TWS_DATE;  }
"MkString"  { return T_MKSTR;   }
"Round"     { return T_ROUND;   }
"Trunc"     { return T_TRUNC;   }
"if"        { return T_IF;      }
"else"      { return T_ELSE;    }
"length"    { return T_LEN;     }
"iteration" { return T_ITER;    }
"order"     { return T_ORDER;   }
"codegen"   { return T_CODEGEN; }
"NO_DESTRUCTOR"     { return T_NODESTRUCTORMETHOD; }
"NO_DESTROY_METHOD" { return T_NODESTROYMETHOD; }
{DATA}      { return T_DATA;    }
{FILTER}    {return T_FILTER;   }
{EOL}       { ++yyline; }
{COMMENT}   { ++yyline; }
{WHITE}     { ; }
{STRING}    {
		yytext[yyleng-1] = 0;
		yylval.sval = yytext+1;
		return T_STR;
	}
{INTEGER}   { yylval.lval = atol(yytext); return T_INTEGER; }
{HEXINT}    { yylval.lval = strtol(yytext, 0, 0); return T_INTEGER; }
{R_VALUE}   { yylval.sval = yytext; return T_VALUE; }
"["         { return '['; }
"]"         { return ']'; }
{L_BR}      { yybrak++; return T_LBR; }
{R_BR}      { yybrak--; return T_RBR; }
{IDENT}     { yylval.sval = yytext; return T_IDENT;}
"<"         { return T_LESS;}
">"         { return T_GREAT;}
"<="        { return T_LESS_EQ;}
">="        { return T_GREAT_EQ;}
"|"         { return T_BIT_OR;}
"&"         { return T_BIT_AND;}
"^"         { return T_BIT_XOR;}
"||"        { return T_LOG_OR;}
"&&"        { return T_LOG_AND;}
"="         { return T_EQ;}
"=="        { return T_EQUL;}
"!="        { return T_NOTEQUL;}
{R_FMT}     { yylval.sval = yytext+1; return T_FMT;}
[-,;+*/.:()?<>~] { return yytext[0]; }
<incl>[ \t]+
<incl>[^ \t\n]+	{
		yyin_cnt++;
		yyin_struct = (YYIN_STR *) realloc(yyin_struct, yyin_cnt*sizeof(YYIN_STR));
		if(!yyin_struct)
			yyerror("Not enough memory");
		yyin_struct[yyin_cnt-1].yyin_buf = YY_CURRENT_BUFFER;
		yyin_struct[yyin_cnt-1].yyin_line = yyline;
		strnzcpy(yyin_struct[yyin_cnt-1].fname, yytext, _MAX_PATH);
		yyin = fopen(yytext, "r");
		if(!yyin)
			yyerror("Could not open include file");
		yyline = 1;
		yy_switch_to_buffer(yy_create_buffer(yyin, YY_BUF_SIZE));
		BEGIN(INITIAL);
	}
<<EOF>> {
		if(--yyin_cnt < 0 ) {
			free(yyin_struct);
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

