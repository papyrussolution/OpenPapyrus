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
#define	T_REPORT	258
#define	T_DATA	259
#define	T_VAR	260
#define	T_NAME	261
#define	T_TYPE	262
#define	T_FMTFLAG	263
#define	T_FLDFMTFLAG	264
#define	T_GRPFMTFLAG	265
#define	T_BEGIN	266
#define	T_END	267
#define	T_INTEGER	268
#define	T_FLOAT	269
#define	T_BAND	270
#define	T_DATASIZE	271
#define	T_AGGR	272
#define	T_PGLEN	273
#define	T_LEFTMGN	274
#define	T_PRNOPTION	275
#define	T_PRNFLAG	276
#define	T_PORTRAIT_PGLEN	277
#define	T_LANDSCAPE_PGLEN	278
#define	T_LAYOUT	279


extern YYSTYPE yylval;
