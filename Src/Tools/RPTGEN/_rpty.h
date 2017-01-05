#ifndef __RPTY_TAB_H
#	define __RPTY_TAB_H

#define T_REPORT 257
#define T_DATA 258
#define T_VAR 259
#define T_NAME 260
#define T_TYPE 261
#define T_FMTFLAG 262
#define T_FLDFMTFLAG 263
#define T_GRPFMTFLAG 264
#define T_BEGIN 265
#define T_END 266
#define T_INTEGER 267
#define T_FLOAT 268
#define T_BAND 269
#define T_DATASIZE 270
#define T_AGGR 271
#define T_PGLEN 272
#define T_LEFTMGN 273
#define T_PRNOPTION 274
#define T_PRNFLAG 275
#define T_PORTRAIT_PGLEN 276
#define T_LANDSCAPE_PGLEN 277

typedef union {
	int      ival;
	unsigned uival;
	long     lval;
	double   rval;
	char   * sptr;
	struct {
		long format;
		int  fldfmt;
	} ffval;
} YYSTYPE;

extern YYSTYPE yylval;

#define YYERRCODE 256

#endif /* __RPTY_TAB_H */
