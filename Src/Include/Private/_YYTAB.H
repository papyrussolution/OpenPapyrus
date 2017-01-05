#ifndef __Y_TAB_H
#	define __Y_TAB_H

#define T_VAR 257
#define T_NUMBER 258

typedef union {
	int     ival;
	double  dval;
	char   *sptr;
	void   *vptr;
} YYSTYPE;

extern YYSTYPE yylval;

#define YYERRCODE 256

#endif /* __Y_TAB_H */
