/*
	Created by IBM PC LEX from file "lex.lxi"
*/

#include <stdio.h>
#include <lex.h>

#define LL16BIT int

extern struct lextab *_tabp;
extern int  yyline;
extern char *llend;
int lexval;

LL16BIT _Flextab[] = {
	   -1,   -1,
};

#define	LLTYPE1	char

LLTYPE1 _Nlextab[] = {
		1
};

LLTYPE1 _Clextab[] = {
	   -1
};

LLTYPE1 _Dlextab[] = {
		1
};

LL16BIT _Blextab[] = {
		0,    0
};

struct lextab lextab = {
	1,		/* Highest state */
	_Dlextab,	/* --> "Default state" table */
	_Nlextab,	/* --> "Next state" table */
	_Clextab,	/* --> "Check value" table */
	_Blextab,	/* --> "Base" table */
	0,		/* Index of last entry in "next" */
	_lmovb,		/* --> Byte-int move routine */
	_Flextab,	/* --> "Final state" table */
	_Alextab,	/* --> Action routine */

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
		lexswitch(&lextab);
}

