/*
	RPTLEX.L
	Copyright (c) A.Sobolev 1996, 1997, 1998, 2003
*/

%{
#include <stdio.h>
#include <stdlib.h>
#include <pp.h>
#include "_report.h"
#include "_rpty.h"
#include "rpt_yy.h"

#pragma warn -rch

long yyline = 1;
char temp_buf1[64];
char temp_buf2[64];
char * temp_p;

struct _Option {
	char * option;
	int flag;
	int tok;
} _options[] = {
{ "ALIGN_RIGHT",    ALIGN_RIGHT,  T_FMTFLAG },
{ "ALIGN_LEFT",     ALIGN_LEFT,   T_FMTFLAG },
{ "ALIGN_CENTER",   ALIGN_CENTER, T_FMTFLAG },
{ "COMF_FILLOVF",   COMF_FILLOVF, T_FMTFLAG },

{ "NMBF_NONEG",     NMBF_NONEG,     T_FMTFLAG },
{ "NMBF_NEGPAR",    NMBF_NEGPAR,    T_FMTFLAG },
{ "NMBF_FORCEPOS",  NMBF_FORCEPOS,  T_FMTFLAG },
{ "NMBF_NOZERO",    NMBF_NOZERO,    T_FMTFLAG },
{ "NMBF_DELCOMMA",  NMBF_DELCOMMA,  T_FMTFLAG },
{ "NMBF_DELAPOSTR", NMBF_DELAPOSTR, T_FMTFLAG },
{ "NMBF_DELSPACE",  NMBF_DELSPACE,  T_FMTFLAG },
{ "NMBF_NOTRAILZ",  NMBF_NOTRAILZ,  T_FMTFLAG },

{ "DATF_AMERICAN",  DATF_AMERICAN, T_FMTFLAG },
{ "DATF_ANSI",      DATF_ANSI,     T_FMTFLAG },
{ "DATF_BRITISH",   DATF_BRITISH,  T_FMTFLAG },
{ "DATF_FRENCH",    DATF_FRENCH,   T_FMTFLAG },
{ "DATF_GERMAN",    DATF_GERMAN,   T_FMTFLAG },
{ "DATF_ITALIAN",   DATF_ITALIAN,  T_FMTFLAG },
{ "DATF_JAPAN",     DATF_JAPAN,    T_FMTFLAG },
{ "DATF_USA",       DATF_USA,      T_FMTFLAG },
{ "DATF_MDY",       DATF_MDY,      T_FMTFLAG },
{ "DATF_DMY",       DATF_DMY,      T_FMTFLAG },
{ "DATF_YMD",       DATF_YMD,      T_FMTFLAG },
{ "DATF_NUMBER",    DATF_NUMBER,   T_FMTFLAG },
{ "DATF_CENTURY",   DATF_CENTURY,  T_FMTFLAG },

{ "TIMF_HMS",       TIMF_HMS,   T_FMTFLAG },
{ "TIMF_HM",        TIMF_HM,    T_FMTFLAG },
{ "TIMF_MS",        TIMF_MS,    T_FMTFLAG },
{ "TIMF_S",         TIMF_S,     T_FMTFLAG },
{ "TIMF_MSEC",      TIMF_MSEC,  T_FMTFLAG },
{ "TIMF_BLANK",     TIMF_BLANK, T_FMTFLAG },

{ "STRF_UPPER",     STRF_UPPER,    T_FMTFLAG },
{ "STRF_LOWER",     STRF_LOWER,    T_FMTFLAG },
{ "STRF_PASSWORD",  STRF_PASSWORD, T_FMTFLAG },
{ "STRF_OEM",       STRF_OEM,      T_FMTFLAG },
{ "STRF_ANSI",      STRF_ANSI,     T_FMTFLAG },

{ "BOLD",           FLDFMT_BOLD,          T_FLDFMTFLAG },
{ "ITALIC",         FLDFMT_ITALIC,        T_FLDFMTFLAG },
{ "UNDERLINE",      FLDFMT_UNDERLINE,     T_FLDFMTFLAG },
{ "SUPERSCRIPT",    FLDFMT_SUPERSCRIPT,   T_FLDFMTFLAG },
{ "SUBSCRIPT",      FLDFMT_SUBSCRIPT,     T_FLDFMTFLAG },
{ "NOREPEATONPG",   FLDFMT_NOREPEATONPG,  T_FLDFMTFLAG },
{ "NOREPEATONRPT",  FLDFMT_NOREPEATONRPT, T_FLDFMTFLAG },
{ "STRETCHVERT",    FLDFMT_STRETCHVERT,   T_FLDFMTFLAG },
{ "ESC",            FLDFMT_ESC,           T_FLDFMTFLAG },

{ "NEWPAGE",        GRPFMT_NEWPAGE,     T_GRPFMTFLAG },
{ "SWAPHEAD",       GRPFMT_SWAPHEAD,    T_GRPFMTFLAG },
{ "SWAPFOOT",       GRPFMT_SWAPFOOT,    T_GRPFMTFLAG },
{ "REPRINTHEAD",    GRPFMT_REPRINTHEAD, T_GRPFMTFLAG },
{ "RESETPGNMB",     GRPFMT_RESETPGNMB,  T_GRPFMTFLAG },

{ "EJECTBEFORE",    SPRN_EJECTBEFORE, T_PRNFLAG },
{ "EJECTAFTER",     SPRN_EJECTAFTER,  T_PRNFLAG },
{ "EJECT",          SPRN_EJECTAFTER,  T_PRNFLAG },
{ "NLQ",            SPRN_NLQ,         T_PRNFLAG },
{ "CPI10",          SPRN_CPI10,       T_PRNFLAG },
{ "CPI12",          SPRN_CPI12,       T_PRNFLAG },
{ "CONDENCED",      SPRN_CONDS,       T_PRNFLAG },
{ "COND",           SPRN_CONDS,       T_PRNFLAG },
{ "PORTRAIT",       0,                T_PRNFLAG },
{ "LANDSCAPE",      SPRN_LANDSCAPE,   T_PRNFLAG }
};

static int getword()
{
	int i;
	for(i = 0; i < sizeof(_options) / sizeof(_Option); i++)
		if(stricmp(yytext, _options[i].option) == 0) {
			yylval.ival = _options[i].flag;
			printf("Word = %s, option = %ld\n", yytext, yylval.ival);
			return _options[i].tok;
		}
	strcpy(temp_buf1, yytext);
	printf("Word = %s, lenght = %ld\n", yytext, yyleng);
	yylval.sptr = temp_buf1;
	return T_NAME;
}

static void getdatasize()
{
	char buf[64];
	char * p, * p1, * p2;
	int sz, dec;
	strip(strcpy(buf, yytext));
	if((p = buf)[0] == '[')
		p++;
	if((p1 = strchr(p, '.')) != 0)
		*p1++ = 0;
	if((p2 = strchr(p1 ? p1 : p, ']')) != 0)
		*p2 = 0;
	sz  = atoi(p);
	dec = p1 ? atoi(p1) : 0;
	yylval.ival = ((sz << 8) | dec);
}
%}

eol           "\n"
white         [ \t]([ \t])*
c_comment     "/*"(.)*"*/"
cpp_comment   "//"(.)*"\n"
alpha         [a-zA-Z_]
alphanum      [a-zA-Z0-9_]
word          {alpha}({alphanum})*
var           "@"{alpha}({alphanum})*
dec_nmb       [0-9]+
datasz        "["{dec_nmb}("]"|("."{dec_nmb}"]"))
layout        "!"{white}"."
title         {eol}"."({white})*"title"
summary       {eol}"."({white})*"summary"
pghead        {eol}"."({white})*"pghead"
pgfoot        {eol}"."({white})*"pgfoot"
group         {eol}"."({white})*"group"
endg          {eol}"."({white})*"endg"
detail        {eol}"."({white})*"detail"
rptend        {eol}"."({white})*{eol}

%%
{eol}         { yyline++; printf("%ld", yyline);}
{white}       { ; }
{c_comment}   { yyline++; }
{cpp_comment} { yyline++; }
{var}    {
	yylval.sptr = strcpy(temp_buf1, yytext);
	return T_VAR;
}

"report" { return T_REPORT; }
"data"   { return T_DATA; }

"portrait pglen" |
"portrait length" |
"portrait pagelength"  { return T_PORTRAIT_PGLEN; }

"landscape pglen" |
"landscape length" |
"landscape pagelength" { return T_LANDSCAPE_PGLEN; }

"pglen" |
"length" |
"pagelength" { return T_PGLEN; }
"leftmgn" |
"leftmargin" { return T_LEFTMGN; }
"option" { return T_PRNOPTION; }

"char"   { yylval.ival = MKSTYPE(S_ZSTRING, 0); return T_TYPE; }
"int"    { yylval.ival = MKSTYPE(S_INT, 2); return T_TYPE; }
"long"   { yylval.ival = MKSTYPE(S_INT, 4); return T_TYPE; }
"float"  { yylval.ival = MKSTYPE(S_FLOAT, 4); return T_TYPE; }
"double" { yylval.ival = MKSTYPE(S_FLOAT, 8); return T_TYPE; }
"ldbl"   { yylval.ival = MKSTYPE(S_FLOAT, 10); return T_TYPE; }
"dec" |
"decimal" { yylval.ival = MKSTYPED(S_DEC, 8, 2); return T_TYPE; }
"money"  { yylval.ival = MKSTYPED(S_MONEY, 8, 2); return T_TYPE; }
"date"   { yylval.ival = MKSTYPE(S_DATE, 4); return T_TYPE; }
"time"   { yylval.ival = MKSTYPE(S_TIME, 4); return T_TYPE; }

"count"  { yylval.ival = AGGR_COUNT; return T_AGGR; }
"sum"    { yylval.ival = AGGR_SUM; return T_AGGR; }
"avg"    { yylval.ival = AGGR_AVG; return T_AGGR; }
"min"    { yylval.ival = AGGR_MIN; return T_AGGR; }
"max"    { yylval.ival = AGGR_MAX; return T_AGGR; }

{datasz} {
	getdatasize();
	return T_DATASIZE;
}

"curdate" { yylval.ival = BIVAR_CURDATE; return T_VAR; }
"curtime" { yylval.ival = BIVAR_CURTIME; return T_VAR; }
"page"    { yylval.ival = BIVAR_PAGE;    return T_VAR; }

{layout}  {
	yylval.lval = yyleng;
	return T_LAYOUT;
}
{title}   {
	yylval.ival = RPT_HEAD;
	return T_BAND;
}
{summary} {
	yylval.ival = RPT_FOOT;
	return T_BAND;
}
{pghead}  {
	yylval.ival = PAGE_HEAD;
	return T_BAND;
}
{pgfoot}  {
	yylval.ival = PAGE_FOOT;
	return T_BAND;
}
{group}   {
	yylval.ival = GROUP_HEAD;
	return T_BAND;
}
{endg}    {
	yylval.ival = GROUP_FOOT;
	return T_BAND;
}
{detail}  {
	yylval.ival = DETAIL_BODY;
	return T_BAND;
}
{rptend}  {
	yyline+=2;
	yylval.ival = 0;
	return T_END;
}
{word}    { return getword(); }
{dec_nmb} {
	yylval.ival = atoi(yytext);
	return T_INTEGER;
}
[=|,;{}\(\)] {
	return yytext[0];
}

%%

int yywrap(void)
{
    return 1;
}
