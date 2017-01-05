#define YYBYACC      1
#define YYMAJOR      1
#define YYMINOR      9
#define yyclearin    (yychar=(-1))
#define yyerrok      (yyerrflag=0)
#define YYRECOVERING (yyerrflag!=0)
#define YYPREFIX "yy"

#include "rpty_tab.h"


#include "_report.h"
#include <lex.h>
#include "_rpty.h"

extern int yyline;
extern char ResFileName[MAXPATH]; 

int       i, ii;
int       ok;
int       default_portrait_pglen  = 0;
int       default_landscape_pglen = 0;
int       linewidth;
int       varlistcount  = 0;
SRptVar * varlist = 0;
SRptVar   varitem;
TYPEID    _type;
int     tmp_id = 0;
PPALDD * ald_data = 0;

int grp_flds[21];

uint pos;
VarTable vartab;

SReport * _report = 0;

FILE * header_file = 0;

int SLAPI PPError(int code, char *)
{
	printf("PPError code %d", code);
	return 1;
}

int SLAPI PPError()
{
	return PPError(PPErrCode, 0);
}

void yyerror(Pchar str)
{
	printf("Error (%d): %s\n", yyline, str);
	exit(-1);
}

void yywarning(Pchar str)
{
	printf("Warning (%d): %s\n", yyline, str);
}

int var_to_id(char * v)
{
	char buf[64];
	if(vartab.searchVar(v, &(pos = 0)))
		return vartab.at(pos).id;
	sprintf(buf, "Неизвестная переменная '%s'", v);
	yyerror(buf);
	return 0;
}

int var_to_idx(char * v)
{
	char buf[64];
	int i;
	for(i = 0; i < _report->fldCount; i++)
		if(!strcmp(v, _report->text + _report->fields[i].name))
			return i+1;
	sprintf(buf, "Неизвестная переменная '%s'", v);
	yyerror(buf);
	return 0;
}

void getlayout()
{
	linewidth = 1;
	char c = lexpeekc();
	while(c != '\n' && c != '.' && c != EOF) {
		lexchar();
		linewidth++;
		c = lexpeekc();
	}
	if(c != '\n') {
		if(c == EOF)
			yyerror("Unexpected EOF");
		lexchar();
	}
}

void addvar_to_list();
void addvar_to_tab(int id = 0);

void getband(int kind, int options)
{
	int  i, j, l, fl, bl;
	long fmt;
	uint pos = 0;
	char buf[256];
	char * p = buf;
	char c;
	SRptVar * var;
	for(i = 0; i < _report->bandCount; i++)
		if((j = _report->bands[i].kind) == kind) {
			if(j == PAGE_HEAD || j == PAGE_FOOT ||
				j == RPT_HEAD || j == RPT_FOOT || j == DETAIL_BODY)
				yyerror("Duplicate band's kind");
		}
	SReport::Band band;
	memset(&band, 0, sizeof(band));
	band.kind = kind;
	band.options = options;
	do
		c = lexchar();
	while(c != '\n' && c != EOF);
	do {
		if(c == EOF)
			yyerror("Unexpected EOF");
		yyline++;
		c = lexchar();
		l = 0;
		while(c != '\n' && c != EOF) {
			if(c == '@') {
				if(p != buf) {
					*p = 0;
					if((pos = _report->addText(buf)) != 0)
						band.addField(pos);
					else
						yyerror("Error adding text");
					p = buf;
				}
				do {
					*p++ = c;
					c = lexchar();
				} while(isalpha(c) || isdigit(c) || c == '_');
				*p = 0;
				i = var_to_id(buf);
				i = var_to_idx(buf);
				for(fl = strlen(buf); c == '.'; fl++)
					c = lexchar();
				fmt = vartab.at(::pos).format;
				if (((SFMTLEN(fmt) & 0xfff) != fl) && (SFMTLEN(fmt) & 0xfff)) {
					varlistcount = 0;
					free(varlist);
					varlist = 0;
					memset(&varitem, 0, sizeof(varitem));
					sprintf(varitem.name, "@%d", tmp_id);
					varitem.type = vartab.at(::pos).type;
					varitem.format = MKSFMTD(fl, SFMTPRC(fmt), SFMTFLAG(fmt));
					addvar_to_list();
					addvar_to_tab(i);
					tmp_id++;
					i = var_to_idx(buf);
					i = var_to_idx(varitem.name);
				}
				else {
					fmt |= MKSFMTD(fl, 0, 0);
					vartab.at(::pos).format  = fmt;
					_report->fields[i-1].format = fmt;
				}
				band.addField(i);
				l += fl;
				p = buf;
			}
			else {
				if(l < linewidth) {
					*p++ = c;
					l++;
				}
				c = lexchar();
			}
		}
		*p++ = '\n';
		*p = 0;
		if((pos = _report->addText(buf)) != 0)
			band.addField(pos);
		else
			yyerror("Error adding text");
		p = buf;
		band.ht++;
	} while(lexpeekc() != '.');
	fseek(lexin, -1, SEEK_CUR);
	c = lexpeekc();
	yyline++;
	if(!_report->addBand(&band, grp_flds, &pos))
		yyerror("Unable add band");
}

void check_bands()
{
	int c = 0;
	int d = 0; /* Признак того, что встретилась зона DETAIL_BODY*/
	for(int i = _report->bandCount-1; i >= 0; i--) {
		if(_report->bands[i].kind == GROUP_FOOT) {
			if(d)
				yyerror("Дно группы должно находиться ниже зоны детализации");
			c++;
		}
		else if(_report->bands[i].kind == GROUP_HEAD) {
			if(d == 0)
				yyerror("Заголовок группы должен находиться выше зоны детализации");
			c--;
		}
		else if(_report->bands[i].kind == DETAIL_BODY)
			d++;
	}
	if(c != 0)
		yyerror("Несоответствие между заголовками и поддонами групп");
	if(d > 1)
		yyerror("В отчете должно быть не более одной зоны детализации");
}

void addvar_to_list()
{
	varlist = (SRptVar*)realloc(varlist, sizeof(*varlist) * (varlistcount+1));
	varlist[varlistcount++] = varitem;
}

void addvar_to_tab(int id)
{
	for(int i = 0; i < varlistcount; i++) {
		int ok = vartab.addVar(varlist+i, &(pos = 0));
		if(ok < 0) {
			char msg_buf[128];
			sprintf(msg_buf, "Переменная %s уже определена", varlist[i].name);
			yyerror(msg_buf);
		}
		else if(ok == 0)
			yyerror("Невозможно добавить переменную в таблицу");
		SRptVar * var = &vartab.at(pos);
		if (id == 0)
			pos = _report->addField(vartab.getCount(), var->type, var->format,
				var->fldfmt, var->name);
		else
			pos = _report->addField(id, var->type, var->format,
				var->fldfmt, var->name);
		if(pos)
			var->id = pos;
		else
			yyerror("Невозможно добавить переменную в отчет");
	}
}

void scan_aldd(PPIterID id)
{
	long ci, i;
		ci = ald_data->GetFieldCount(id);
		for (i = 1; i <= ci; i++) {
			PpalddField * paf = ald_data->GetFieldInfo(id, i);
			if (paf->Alias[0]) {
				memset(&varitem, 0, sizeof(varitem));
				strcpy(varitem.name, paf->Alias);
				if (GETSTYPE(paf->Type) == S_AUTOINC)
					varitem.type = MKSTYPE(S_INT, 4);
				else
					varitem.type = paf->Type;
				varitem.format = MKSFMTD(0, paf->Format.prec, paf->Format.flags);
				addvar_to_list();
			}
		}
}

#pragma warn -pia


#define YYTABLESIZE 241
#define YYFINAL 3
#ifndef YYDEBUG
#define YYDEBUG 0
#endif
#define YYMAXTOKEN 277
extern short yylhs[];
extern short yylen[];
extern short yydefred[];
extern short yydgoto[];
extern short yysindex[];
extern short yyrindex[];
extern short yygindex[];
extern short yytable[];
extern short yycheck[];
#if YYDEBUG
extern char *yyname[];
extern char *yyrule[];
#endif
#ifdef YYSTACKSIZE
#	undef YYMAXDEPTH
#	define YYMAXDEPTH YYSTACKSIZE
#else
#	ifdef YYMAXDEPTH
#		define YYSTACKSIZE YYMAXDEPTH
#	else
#		define YYSTACKSIZE 500
#		define YYMAXDEPTH 500
#	endif
#endif
int       yydebug;
int       yynerrs;
int       yyerrflag;
int       yychar;
short   * yyssp;
int       yyvsp;
YYSTYPE   yyval;
YYSTYPE   yylval;
short     yyss[YYSTACKSIZE];
YYSTYPE   yyvs[YYSTACKSIZE];

#define yystacksize YYSTACKSIZE

#define YYABORT goto yyabort
#define YYREJECT goto yyabort
#define YYACCEPT goto yyaccept
#define YYERROR goto yyerrlab

int yyparse(void)
{
	int yym, yyn, yystate;
#if YYDEBUG
	char *yys = getenv("YYDEBUG");
	if(yys && *yys >= '0' && *yys <= '9')
		yydebug = *yys - '0';
#endif
	yynerrs   = 0;
	yyerrflag = 0;
	yychar    = -1;
	yyssp     = yyss;
	yyvsp     = 0;
	*yyssp    = yystate = 0;
yyloop:
	if(yyn = yydefred[yystate])
		goto yyreduce;
	if(yychar < 0) {
		if((yychar = yylex()) < 0)
			yychar = 0;
#if YYDEBUG
		if(yydebug) {
			yys = 0;
			if(yychar <= YYMAXTOKEN)
				yys = yyname[yychar];
			if(!yys)
				yys = "illegal-symbol";
			printf("%sdebug: state %d, reading %d (%s)\n",
					YYPREFIX, yystate, yychar, yys);
		}
#endif
	}
	if((yyn = yysindex[yystate]) && (yyn += yychar) >= 0 &&
			yyn <= YYTABLESIZE && yycheck[yyn] == yychar) {
#if YYDEBUG
		if(yydebug)
			printf("%sdebug: state %d, shifting to state %d\n",
					YYPREFIX, yystate, yytable[yyn]);
#endif
		if(yyssp >= yyss + yystacksize - 1)
			goto yyoverflow;
		*++yyssp = yystate = yytable[yyn];
		yyvs[++yyvsp] = yylval;
		yychar = (-1);
		if(yyerrflag > 0)
			--yyerrflag;
		goto yyloop;
	}
	if((yyn = yyrindex[yystate]) && (yyn += yychar) >= 0 &&
			yyn <= YYTABLESIZE && yycheck[yyn] == yychar) {
		yyn = yytable[yyn];
		goto yyreduce;
	}
	if(yyerrflag)
		goto yyinrecovery;
	yyerror("syntax error");
yyerrlab:
	++yynerrs;
yyinrecovery:
	if(yyerrflag < 3) {
		yyerrflag = 3;
		for(;;) {
			if((yyn = yysindex[*yyssp]) && (yyn += YYERRCODE) >= 0 &&
					yyn <= YYTABLESIZE && yycheck[yyn] == YYERRCODE) {
#if YYDEBUG
				if(yydebug)
					printf("%sdebug: state %d, error recovery shifting\
 to state %d\n", YYPREFIX, *yyssp, yytable[yyn]);
#endif
				if(yyssp >= yyss + yystacksize - 1)
					goto yyoverflow;
				*++yyssp = yystate = yytable[yyn];
				yyvs[++yyvsp] = yylval;
				goto yyloop;
			}
			else {
#if YYDEBUG
				if(yydebug)
					printf("%sdebug: error recovery discarding state %d\n",
							YYPREFIX, *yyssp);
#endif
				if(yyssp <= yyss)
					goto yyabort;
				--yyssp;
				--yyvsp;
			}
		}
	}
	else {
		if(yychar == 0)
			goto yyabort;
#if YYDEBUG
		if(yydebug) {
			yys = 0;
			if(yychar <= YYMAXTOKEN)
				yys = yyname[yychar];
			if(!yys)
				yys = "illegal-symbol";
			printf("%sdebug: state %d, error recovery discards token %d (%s)\n",
					YYPREFIX, yystate, yychar, yys);
		}
#endif
		yychar = (-1);
		goto yyloop;
	}
yyreduce:
#if YYDEBUG
	if(yydebug)
		printf("%sdebug: state %d, reducing by rule %d (%s)\n",
				YYPREFIX, yystate, yyn, yyrule[yyn]);
#endif
	yym = yylen[yyn];
	yyval = yyvs[yyvsp+1-yym];
	switch(yyn) {
case 4:
{
}
break;
case 5:
{
	default_portrait_pglen = yyvs[-1+yyvsp].ival;
}
break;
case 6:
{
	default_landscape_pglen = yyvs[-1+yyvsp].ival;
}
break;
case 9:
{
	if(_report->pglen == 0)
		if(_report->prnoptions & SPRN_LANDSCAPE)
			_report->pglen = default_landscape_pglen;
		else
			_report->pglen = default_portrait_pglen;
	check_bands();
	accept_report(_report);
}
break;
case 12:
{
}
break;
case 13:
{
	_report->pglen = yyvs[-1+yyvsp].ival;
}
break;
case 14:
{
	_report->leftmarg = yyvs[-1+yyvsp].ival;
}
break;
case 15:
{
	_report->prnoptions |= yyvs[-1+yyvsp].ival;
}
break;
case 16:
{
	yyval.ival = yyvs[0+yyvsp].ival;
}
break;
case 17:
{
	yyval.ival = yyvs[-2+yyvsp].ival | yyvs[0+yyvsp].ival;
}
break;
case 18:
{
	yyval.ival = yyvs[0+yyvsp].ival;
}
break;
case 21:
{
	_report->data_name = newStr(yyvs[0+yyvsp].sptr);
	ald_data = new PPALDD(ResFileName, PPALDD::GetResID(ResFileName, yyvs[0+yyvsp].sptr).ID, 0);
	if (!ald_data)
		yyerror("Couldn't open PPALDD data\n");
	else {
		if (!ald_data->IsValid()) {
			delete ald_data;
			yyerror("Couldn't open PPALDD data\n");
		}
		varlistcount = 0;
		free(varlist);
		varlist = 0;
		_report->main_id = PPALDD::GetResID(ResFileName, yyvs[0+yyvsp].sptr).ParentID;
		for (ii = 1; ii <= ald_data->GetListCount(); ii++)
			scan_aldd(ii);
		addvar_to_tab();
	}
}
break;
case 22:
{
}
break;
case 23:
{
}
break;
case 24:
{
}
break;
case 25:
{
}
break;
case 26:
{
	if(_report->setAggrToField(yyvs[-5+yyvsp].ival, yyvs[-4+yyvsp].ival, var_to_id(yyvs[-2+yyvsp].sptr)) == 0)
		yyerror("Невозможно установить агрегатную функцию");
}
break;
case 27:
{
	yyval.ival = var_to_id(yyvs[-1+yyvsp].sptr);
}
break;
case 28:
{
	delete ald_data;
	ald_data = 0;
	delete _report;
	_report = new SReport(yyvs[0+yyvsp].sptr);
	vartab.freeAll();
	tmp_id = 0;
}
break;
case 29:
{
}
break;
case 30:
{
	addvar_to_tab();
}
break;
case 31:
{
	addvar_to_tab();
}
break;
case 32:
{
	for(i = 0; i < varlistcount; i++) {
		varlist[i].format = yyvs[-1+yyvsp].ffval.format;
		varlist[i].fldfmt = yyvs[-1+yyvsp].ffval.fldfmt;
	}
}
break;
case 33:
{
	_type = yyvs[0+yyvsp].ival;
	varlistcount = 0;
	free(varlist);
	varlist = 0;
}
break;
case 34:
{
	addvar_to_list();
}
break;
case 35:
{
	addvar_to_list();
}
break;
case 36:
{
	memset(&varitem, 0, sizeof(varitem));
	strcpy(varitem.name, yyvs[0+yyvsp].sptr);
	varitem.type = _type;
}
break;
case 37:
{
	memset(&varitem, 0, sizeof(varitem));
	strcpy(varitem.name, yyvs[-1+yyvsp].sptr);
	if(LoByte(yyvs[0+yyvsp].ival))
		varitem.type = MKSTYPED(GETSTYPE(_type), HiByte(yyvs[0+yyvsp].ival), LoByte(yyvs[0+yyvsp].ival));
	else
		varitem.type = MKSTYPE(GETSTYPE(_type), HiByte(yyvs[0+yyvsp].ival));
}
break;
case 38:
{
	SETSFMTFLAG(yyvs[-2+yyvsp].lval, yyvs[-3+yyvsp].ival);
	yyval.ffval.format = yyvs[-2+yyvsp].lval;
	yyval.ffval.fldfmt = yyvs[-1+yyvsp].ival;
}
break;
case 39:
{
	yyval.ival = 0;
}
break;
case 40:
{
	yyval.ival = yyvs[-2+yyvsp].ival | yyvs[0+yyvsp].ival;
}
break;
case 41:
{
	yyval.ival = yyvs[0+yyvsp].ival;
}
break;
case 42:
{
	yyval.lval = 0;
}
break;
case 43:
{
	yyval.lval = 0;
}
break;
case 44:
{
	yyval.lval = MKSFMTD(0, yyvs[0+yyvsp].ival, 0);
}
break;
case 45:
{
	yyval.ival = 0;
}
break;
case 46:
{
	yyval.ival = yyvs[-2+yyvsp].ival | yyvs[0+yyvsp].ival;
}
break;
case 47:
{
	yyval.ival = yyvs[0+yyvsp].ival;
}
break;
case 48:
{
	getlayout();
}
break;
case 49:
{
}
break;
case 50:
{
}
break;
case 51:
{
	getband(yyvs[-2+yyvsp].ival, 0);
}
break;
case 52:
{
	getband(yyvs[-5+yyvsp].ival, yyvs[-2+yyvsp].ival);
}
break;
case 53:
{
	grp_flds[0] = 0;
}
break;
case 54:
{
	grp_flds[(grp_flds[0])++] = var_to_id(yyvs[0+yyvsp].sptr);
}
break;
case 55:
{
	grp_flds[0] = 1;
	grp_flds[1] = var_to_id(yyvs[0+yyvsp].sptr);
}
break;
case 56:
{
	yyval.ival = yyvs[-2+yyvsp].ival | yyvs[0+yyvsp].ival;
}
break;
case 57:
{
	yyval.ival = yyvs[0+yyvsp].ival;
}
break;
	}
	yyssp -= yym;
	yystate = *yyssp;
	yyvsp -= yym;
	yym = yylhs[yyn];
	if(yystate == 0 && yym == 0) {
#if YYDEBUG
		if(yydebug)
			printf("%sdebug: after reduction, shifting from state 0 to\
 state %d\n", YYPREFIX, YYFINAL);
#endif
		yystate = YYFINAL;
		*++yyssp = YYFINAL;
		yyvs[++yyvsp] = yyval;
		if(yychar < 0) {
			if((yychar = yylex()) < 0)
				yychar = 0;
#if YYDEBUG
			if(yydebug) {
				yys = 0;
				if(yychar <= YYMAXTOKEN)
					yys = yyname[yychar];
				if(!yys)
					yys = "illegal-symbol";
				printf("%sdebug: state %d, reading %d (%s)\n",
						YYPREFIX, YYFINAL, yychar, yys);
			}
#endif
		}
		if(yychar == 0)
			goto yyaccept;
		goto yyloop;
	}
	if((yyn = yygindex[yym]) && (yyn += yystate) >= 0 &&
			yyn <= YYTABLESIZE && yycheck[yyn] == yystate)
		yystate = yytable[yyn];
	else
		yystate = yydgoto[yym];
#if YYDEBUG
	if(yydebug)
		printf("%sdebug: after reduction, shifting from state %d \
to state %d\n", YYPREFIX, *yyssp, yystate);
#endif
	if(yyssp >= yyss + yystacksize - 1) {
		goto yyoverflow;
	}
	*++yyssp = yystate;
	yyvs[++yyvsp] = yyval;
	goto yyloop;
yyoverflow:
	yyerror("yacc stack overflow");
yyabort:
	return (1);
yyaccept:
	return (0);
}
