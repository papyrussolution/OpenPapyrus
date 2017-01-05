/*
	Created by IBM PC LEX from file "rptlex.lxi"
*/

#include <stdio.h>
#include <lex.h>

#define LL16BIT int

extern struct lextab *_tabp;
extern int  yyline;
extern char *llend;
int lexval;


#include "_report.h"
#include "_rpty.h"

#pragma warn -rch

char yytext[256];
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
			return _options[i].tok;
		}
	strcpy(temp_buf1, yytext);
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

int _Arptlex(int __na__)		/* Action routine */
	{
	switch (__na__) {

		case 0:
 yyline++; 
			break;
		case 1:
;
			break;
		case 2:
 comment("*/"); 
			break;
		case 3:
 comment("\n"); 
			break;
		case 4:

	gettoken(yytext, sizeof(yytext));
	yylval.sptr = strcpy(temp_buf1, yytext);
	return T_VAR;

			break;
		case 5:
 return T_REPORT; 
			break;
		case 6:
 return T_DATA; 
			break;
		case 7:
 return T_PORTRAIT_PGLEN; 
			break;
		case 8:
 return T_LANDSCAPE_PGLEN; 
			break;
		case 9:
 return T_PGLEN; 
			break;
		case 10:
 return T_LEFTMGN; 
			break;
		case 11:
 return T_PRNOPTION; 
			break;
		case 12:
 yylval.ival = MKSTYPE(S_ZSTRING, 0); return T_TYPE; 
			break;
		case 13:
 yylval.ival = MKSTYPE(S_INT, 2); return T_TYPE; 
			break;
		case 14:
 yylval.ival = MKSTYPE(S_INT, 4); return T_TYPE; 
			break;
		case 15:
 yylval.ival = MKSTYPE(S_FLOAT, 4); return T_TYPE; 
			break;
		case 16:
 yylval.ival = MKSTYPE(S_FLOAT, 8); return T_TYPE; 
			break;
		case 17:
 yylval.ival = MKSTYPE(S_FLOAT, 10); return T_TYPE; 
			break;
		case 18:
 yylval.ival = MKSTYPED(S_DEC, 8, 2); return T_TYPE; 
			break;
		case 19:
 yylval.ival = MKSTYPED(S_MONEY, 8, 2); return T_TYPE; 
			break;
		case 20:
 yylval.ival = MKSTYPE(S_DATE, 4); return T_TYPE; 
			break;
		case 21:
 yylval.ival = MKSTYPE(S_TIME, 4); return T_TYPE; 
			break;
		case 22:
 yylval.ival = AGGR_COUNT; return T_AGGR; 
			break;
		case 23:
 yylval.ival = AGGR_SUM; return T_AGGR; 
			break;
		case 24:
 yylval.ival = AGGR_AVG; return T_AGGR; 
			break;
		case 25:
 yylval.ival = AGGR_MIN; return T_AGGR; 
			break;
		case 26:
 yylval.ival = AGGR_MAX; return T_AGGR; 
			break;
		case 27:

	gettoken(yytext, sizeof(yytext));
	getdatasize();
	return T_DATASIZE;

			break;
		case 28:
 yylval.ival = BIVAR_CURDATE; return T_VAR; 
			break;
		case 29:
 yylval.ival = BIVAR_CURTIME; return T_VAR; 
			break;
		case 30:
 yylval.ival = BIVAR_PAGE;    return T_VAR; 
			break;
		case 31:

	yyline++;
	yylval.ival = RPT_HEAD;
	return T_BAND;

			break;
		case 32:

	yylval.ival = RPT_FOOT;
	return T_BAND;

			break;
		case 33:

	yylval.ival = PAGE_HEAD;
	return T_BAND;

			break;
		case 34:

	yylval.ival = PAGE_FOOT;
	return T_BAND;

			break;
		case 35:

	yylval.ival = GROUP_HEAD;
	return T_BAND;

			break;
		case 36:

	yylval.ival = GROUP_FOOT;
	return T_BAND;

			break;
		case 37:

	yylval.ival = DETAIL_BODY;
	return T_BAND;

			break;
		case 38:

	yyline+=2;
	yylval.ival = 0;
	return T_END;

			break;
		case 39:

	gettoken(yytext, sizeof(yytext));
	return getword();

			break;
		case 40:

	gettoken(yytext, sizeof(yytext));
	yylval.ival = atoi(yytext);
	return T_INTEGER;

			break;
		case 41:

	gettoken(yytext, sizeof(yytext));
	return yytext[0];

			break;	}
	return(LEXSKIP);
}


LL16BIT _Frptlex[] = {
	   -1,   41,   40,   40,   39,   39,   -1,   -1,   -1,   -1,
	   27,   -1,   27,   -1,   39,   39,   24,   39,   39,   23,
	   39,   39,   39,   21,   39,   39,   26,   39,   25,   39,
	   39,   39,   19,   39,   39,   39,   39,   15,   39,   39,
	   13,   39,   39,   39,   39,   39,   39,   29,   39,   39,
	   39,   28,   39,   39,   39,   22,   39,   39,   12,   39,
	   39,   39,   39,   39,   11,   39,   39,   39,   17,   39,
	   39,   14,   39,   39,   39,   39,   39,   39,   39,   39,
	   10,   39,   10,   39,   39,   39,    9,   39,   39,   39,
	   39,   39,   39,   39,   39,   -1,   -1,   -1,   -1,   -1,
	   -1,    8,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
	   -1,    8,   -1,   -1,   -1,    8,   39,   39,   39,   30,
	   39,   39,   39,   39,   39,    9,   39,   39,   39,    9,
	   39,   39,   39,   39,   39,   39,   39,   -1,   -1,   -1,
	   -1,   -1,   -1,    7,   -1,   -1,   -1,   -1,   -1,   -1,
	   -1,   -1,   -1,    7,   -1,   -1,   -1,    7,   39,   39,
	   18,   39,   39,   39,   18,   39,   39,   39,   39,   16,
	   39,   39,   20,    6,   39,   39,   39,   39,   39,    5,
	   -1,    4,    4,   -1,    3,    2,    1,    1,    0,   -1,
	   38,   -1,   -1,   -1,   -1,   -1,   37,   -1,   -1,   -1,
	   36,   -1,   -1,   -1,   -1,   35,   -1,   -1,   -1,   -1,
	   -1,   34,   -1,   -1,   -1,   33,   -1,   -1,   -1,   -1,
	   -1,   -1,   32,   -1,   -1,   -1,   -1,   31,   -1,   -1,
	
};

#define	LLTYPE1	char

LLTYPE1 _Nrptlex[] = {
	  229,  229,  229,  229,  229,  229,  229,  229,  229,  186,
	  188,  229,  229,  187,  229,  229,  229,  229,  229,  229,
	  229,  229,  229,  229,  229,  229,  229,  229,  229,  229,
	  229,  229,  186,    1,   95,  137,  187,  229,  229,  229,
	    1,    1,  229,  229,    1,  229,  189,  183,    2,    2,
	    2,    2,    2,    2,    2,    2,    2,    2,  185,    1,
	  229,    1,  229,  184,  180,    4,    4,    4,    4,    4,
	    4,    4,    4,    4,    4,    4,    4,    4,    4,    4,
	    4,    4,    4,    4,    4,    4,    4,    4,    4,    4,
	    4,    6,  229,  229,  229,    4,  229,   14,    4,   41,
	  158,    4,   33,    4,    4,   38,    4,    4,   65,   24,
	    4,   59,  116,    4,  174,   17,   20,    4,    4,    4,
	    4,    4,    4,    1,    1,    1,    3,    3,    3,    3,
	    3,    3,    3,    3,    3,    3,    5,    5,    5,    5,
	    5,    5,    5,    5,    5,    5,   15,   16,   18,   19,
	   21,   22,   23,    5,    5,    5,    5,    5,    5,    5,
	    5,    5,    5,    5,    5,    5,    5,    5,    5,    5,
	    5,    5,    5,    5,    5,    5,    5,    5,    5,   26,
	   28,   30,   31,    5,   32,    5,    5,    5,    5,    5,
	    5,    5,    5,    5,    5,    5,    5,    5,    5,    5,
	    5,    5,    5,    5,    5,    5,    5,    5,    5,    5,
	    5,    7,    7,    7,    7,    7,    7,    7,    7,    7,
	    7,    8,   34,   13,   13,   13,   13,   13,   13,   13,
	   13,   13,   13,    9,    9,    9,    9,    9,    9,    9,
	    9,    9,    9,   11,   11,   11,   11,   11,   11,   11,
	   11,   11,   11,   25,   35,   36,   37,   39,   40,   56,
	   43,   27,   48,   45,   46,   47,   52,   29,   12,   49,
	   50,   51,   42,   53,   54,   55,   57,   58,   44,   60,
	   61,   62,   63,   64,   67,   68,   87,   70,   10,   66,
	   72,   71,   74,   73,   75,   77,   76,   78,   79,   80,
	   69,   83,   81,   82,   84,   85,   86,   88,   89,   90,
	   91,   92,   93,   94,   96,   97,   98,   99,  102,  100,
	  101,  103,  104,  105,  106,  107,  108,  112,  109,  110,
	  111,  113,  114,  115,  117,  118,  119,  120,  121,  122,
	  126,  123,  124,  125,  127,  128,  129,  131,  130,  132,
	  133,  134,  135,  136,  138,  139,  140,  141,  144,  142,
	  143,  145,  146,  147,  148,  149,  150,  154,  151,  152,
	  153,  155,  156,  157,  170,  160,  161,  162,  159,  163,
	  164,  166,  167,  168,  169,  171,  173,  175,  165,  176,
	  172,  177,  178,  179,  181,  181,  181,  181,  181,  181,
	  181,  181,  181,  181,  181,  181,  181,  181,  181,  181,
	  181,  181,  181,  181,  181,  181,  181,  181,  181,  181,
	  192,  193,  194,  195,  181,  196,  181,  181,  181,  181,
	  181,  181,  181,  181,  181,  181,  181,  181,  181,  181,
	  181,  181,  181,  181,  181,  181,  181,  181,  181,  181,
	  181,  181,  182,  182,  182,  182,  182,  182,  182,  182,
	  182,  182,  198,  199,  200,  202,  203,  204,  205,  182,
	  182,  182,  182,  182,  182,  182,  182,  182,  182,  182,
	  182,  182,  182,  182,  182,  182,  182,  182,  182,  182,
	  182,  182,  182,  182,  182,  207,  208,  209,  212,  182,
	  210,  182,  182,  182,  182,  182,  182,  182,  182,  182,
	  182,  182,  182,  182,  182,  182,  182,  182,  182,  182,
	  182,  182,  182,  182,  182,  182,  182,  228,  190,  211,
	  213,  214,  215,  217,  218,  219,  220,  221,  222,  224,
	  225,  226,  227,  229,  229,  229,  229,  229,  229,  229,
	  228,  229,  229,  229,  229,  229,  229,  229,  229,  229,
	  229,  229,  229,  229,  229,  229,  229,  229,  229,  229,
	  229,  229,  229,  229,  229,  229,  229,  229,  229,  229,
	  229,  229,  229,  229,  229,  229,  229,  229,  229,  229,
	  229,  229,  229,  229,  229,  229,  229,  229,  229,  229,
	  229,  229,  229,  229,  229,  229,  229,  229,  229,  229,
	  229,  229,  229,  229,  229,  229,  229,  229,  191,  197,
	  229,  201,  229,  229,  229,  229,  229,  229,  229,  229,
	  206,  229,  229,  216,  223
};

LLTYPE1 _Crptlex[] = {
	   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,    0,
	    0,   -1,   -1,  186,   -1,   -1,   -1,   -1,   -1,   -1,
	   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
	   -1,   -1,    0,    0,   94,  136,  186,   -1,   -1,   -1,
	    0,    0,   -1,   -1,    0,   -1,  188,    0,    0,    0,
	    0,    0,    0,    0,    0,    0,    0,    0,  183,    0,
	   -1,    0,   -1,  183,    0,    0,    0,    0,    0,    0,
	    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
	    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
	    0,    0,   -1,   -1,   -1,    0,   -1,    0,    0,    0,
	    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
	    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
	    0,    0,    0,    0,    0,    0,    2,    2,    2,    2,
	    2,    2,    2,    2,    2,    2,    4,    4,    4,    4,
	    4,    4,    4,    4,    4,    4,   14,   15,   17,   18,
	   20,   21,   22,    4,    4,    4,    4,    4,    4,    4,
	    4,    4,    4,    4,    4,    4,    4,    4,    4,    4,
	    4,    4,    4,    4,    4,    4,    4,    4,    4,   25,
	   27,   29,   30,    4,   31,    4,    4,    4,    4,    4,
	    4,    4,    4,    4,    4,    4,    4,    4,    4,    4,
	    4,    4,    4,    4,    4,    4,    4,    4,    4,    4,
	    4,    6,    6,    6,    6,    6,    6,    6,    6,    6,
	    6,    7,   33,    7,    7,    7,    7,    7,    7,    7,
	    7,    7,    7,    8,    8,    8,    8,    8,    8,    8,
	    8,    8,    8,    9,    9,    9,    9,    9,    9,    9,
	    9,    9,    9,   24,   34,   35,   36,   38,   39,   41,
	   42,   24,   43,   44,   45,   46,   41,   24,    7,   48,
	   49,   50,   41,   52,   53,   54,   56,   57,   43,   59,
	   60,   61,   62,   63,   66,   67,   65,   69,    9,   65,
	   65,   70,   73,   72,   74,   76,   75,   77,   78,   79,
	   65,   72,   75,   81,   83,   84,   85,   87,   88,   89,
	   90,   91,   92,   93,   95,   96,   97,   98,   95,   99,
	  100,  102,  103,  104,  105,  106,  107,  102,  108,  109,
	  110,  112,  113,  114,  116,  117,  118,  119,  120,  121,
	  116,  122,  123,  124,  126,  127,  128,  130,  116,  131,
	  132,  133,  134,  135,  137,  138,  139,  140,  137,  141,
	  142,  144,  145,  146,  147,  148,  149,  144,  150,  151,
	  152,  154,  155,  156,  158,  159,  160,  161,  158,  162,
	  163,  165,  166,  167,  168,  170,  171,  174,  158,  175,
	  171,  176,  177,  178,  180,  180,  180,  180,  180,  180,
	  180,  180,  180,  180,  180,  180,  180,  180,  180,  180,
	  180,  180,  180,  180,  180,  180,  180,  180,  180,  180,
	  191,  192,  193,  194,  180,  195,  180,  180,  180,  180,
	  180,  180,  180,  180,  180,  180,  180,  180,  180,  180,
	  180,  180,  180,  180,  180,  180,  180,  180,  180,  180,
	  180,  180,  181,  181,  181,  181,  181,  181,  181,  181,
	  181,  181,  197,  198,  199,  201,  202,  203,  204,  181,
	  181,  181,  181,  181,  181,  181,  181,  181,  181,  181,
	  181,  181,  181,  181,  181,  181,  181,  181,  181,  181,
	  181,  181,  181,  181,  181,  206,  207,  208,  207,  181,
	  209,  181,  181,  181,  181,  181,  181,  181,  181,  181,
	  181,  181,  181,  181,  181,  181,  181,  181,  181,  181,
	  181,  181,  181,  181,  181,  181,  181,  189,  189,  210,
	  212,  213,  214,  216,  217,  218,  219,  220,  221,  223,
	  224,  225,  226,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
	  189,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
	   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
	   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
	   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
	   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
	   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
	   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,  189,  189,
	   -1,  189,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
	  189,   -1,   -1,  189,  189
};

LLTYPE1 _Drptlex[] = {
	  229,  229,  229,    2,  229,    4,  229,  229,  229,  229,
	  229,    9,  229,    7,    4,    4,    4,    4,    4,    4,
	    4,    4,    4,    4,    4,    4,    4,    4,    4,    4,
	    4,    4,    4,    4,    4,    4,    4,    4,    4,    4,
	    4,    4,    4,    4,    4,    4,    4,    4,    4,    4,
	    4,    4,    4,    4,    4,    4,    4,    4,    4,    4,
	    4,    4,    4,    4,    4,    4,    4,    4,    4,    4,
	    4,    4,    4,    4,    4,    4,    4,    4,    4,    4,
	    4,    4,    4,    4,    4,    4,    4,    4,    4,    4,
	    4,    4,    4,    4,    4,  229,  229,  229,  229,  229,
	  229,  229,  229,  229,  229,  229,  229,  229,  229,  229,
	  229,  229,  229,  229,  229,  229,    4,    4,    4,    4,
	    4,    4,    4,    4,    4,    4,    4,    4,    4,    4,
	    4,    4,    4,    4,    4,    4,    4,  229,  229,  229,
	  229,  229,  229,  229,  229,  229,  229,  229,  229,  229,
	  229,  229,  229,  229,  229,  229,  229,  229,    4,    4,
	    4,    4,    4,    4,    4,    4,    4,    4,    4,    4,
	    4,    4,    4,    4,    4,    4,    4,    4,    4,    4,
	  229,  229,  181,  229,  229,  229,  229,  186,  229,  229,
	  229,  229,  229,  229,  229,  229,  229,  229,  229,  229,
	  229,  229,  229,  229,  229,  229,  229,  229,  229,  229,
	  229,  229,  229,  229,  229,  229,  229,  229,  229,  229,
	  229,  229,  229,  229,  229,  229,  229,  229,  189
};

LL16BIT _Brptlex[] = {
	    0,    0,   78,    0,   88,    0,  163,  175,  185,  195,
	    0,    0,    0,    0,   28,   44,    0,   31,   40,    0,
	   45,   42,   51,    0,  156,   59,    0,   70,    0,   71,
	   81,   63,    0,  114,  143,  158,  140,    0,  147,  142,
	    0,  155,  146,  162,  158,  155,  164,    0,  172,  154,
	  170,    0,  156,  164,  159,    0,  179,  163,    0,  167,
	  164,  176,  171,  173,    0,  189,  186,  177,    0,  177,
	  188,    0,  191,  176,  185,  199,  181,  194,  193,  189,
	    0,  193,    0,  201,  189,  202,    0,  197,  208,  194,
	  211,  214,  200,  212,    2,  206,  214,  206,  214,  203,
	  216,    0,  224,  219,  222,  216,  224,  216,  225,  213,
	  226,    0,  223,  231,  223,    0,  237,  232,  235,  229,
	  237,  229,  238,  226,  239,    0,  236,  244,  236,    0,
	  233,  233,  236,  254,  247,  237,    3,  246,  254,  246,
	  254,  243,  256,    0,  264,  259,  262,  256,  264,  256,
	  265,  253,  266,    0,  263,  271,  263,    0,  277,  276,
	  271,  268,  282,  272,    0,  264,  284,  275,  283,    0,
	  269,  289,    0,    0,  286,  277,  280,  278,  277,    0,
	  329,  404,    0,   16,    0,    0,    4,    0,    0,  518,
	    0,  319,  305,  325,  318,  317,    0,  352,  363,  361,
	    0,  351,  355,  350,  356,    0,  392,  394,  386,  389,
	  413,    0,  429,  434,  432,    0,  416,  425,  426,  439,
	  423,  417,    0,  434,  424,  433,  441,    0,    0,    0
	
};

struct lextab rptlex = {
	229,		/* Highest state */
	_Drptlex,	/* --> "Default state" table */
	_Nrptlex,	/* --> "Next state" table */
	_Crptlex,	/* --> "Check value" table */
	_Brptlex,	/* --> "Base" table */
	634,		/* Index of last entry in "next" */
	_lmovb,		/* --> Byte-int move routine */
	_Frptlex,	/* --> "Final state" table */
	_Arptlex,	/* --> Action routine */

	NULL,   	/* Look-ahead vector */
	0,		/* No Ignore class */
	0,		/* No Break class */
	0,		/* No Illegal class */
};

/* Standard I/O selected */
FILE *lexin;

void llstin(void) {
	if(lexin == NULL)
		lexin = stdin;
	if(_tabp == NULL)
		lexswitch(&rptlex);
}

