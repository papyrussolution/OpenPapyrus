/*
	RPTYY.Y
	Copyright (c) A.Sobolev 1996, 1997, 1998, 1999, 2001
*/

%{

//#include <stdio.h>
//#include <stdlib.h>
#include <pp.h>
#include "_report.h"
#include "rpt_yy.h"

extern long yyline;
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

static char lexpeekc()
{
	char c = getc(yyin);
	ungetc(c, yyin);
	return c;
}

void addvar_to_list();
void addvar_to_tab(int id = 0);

void getband(int kind, int options)
{
	int  i, j, l, fl;
	long fmt;
	uint pos = 0;
	char buf[256];
	char * p = buf;
	char c;
	//SRptVar * var;
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
		c = getc(yyin);
	while(c != '\n' && c != EOF);
	do {
		if(c == EOF)
			yyerror("Unexpected EOF");
		yyline++;
		c = getc(yyin);
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
					c = getc(yyin);
				} while(isalpha(c) || isdigit(c) || c == '_');
				*p = 0;
				i = var_to_id(buf);
				i = var_to_idx(buf);
				for(fl = strlen(buf); c == '.'; fl++)
					c = getc(yyin);
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
				c = getc(yyin);
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
	fseek(yyin, -1, SEEK_CUR);
	c = lexpeekc();
	yyline++;
	if(!_report->addBand(&band, grp_flds, &pos))
		yyerror("Unable add band");
}

void check_bands()
{
	int c = 0;
	int d = 0; // Признак того, что встретилась зона DETAIL_BODY
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
		if(paf->Alias[0]) {
			memset(&varitem, 0, sizeof(varitem));
			strcpy(varitem.name, paf->Alias);
			if (GETSTYPE(paf->Type) == S_AUTOINC)
				varitem.type = MKSTYPE(S_INT, 4);
			else
				varitem.type = (TYPEID)paf->Type;
			varitem.format = MKSFMTD(0, paf->Format.prec, paf->Format.flags);
			addvar_to_list();
		}
	}
}

#pragma warn -pia

%}

%union {
	int      ival;
	unsigned uival;
	long     lval;
	double   rval;
	char   * sptr;
	struct {
		long format;
		int  fldfmt;
	} ffval;
}

%token        T_REPORT
%token        T_DATA
%token <sptr> T_VAR
%token <sptr> T_NAME
%token <ival> T_TYPE
%token <ival> T_FMTFLAG
%token <ival> T_FLDFMTFLAG
%token <ival> T_GRPFMTFLAG
%token        T_BEGIN
%token        T_END
%token <ival> T_INTEGER
%token <rval> T_FLOAT
%token <ival> T_BAND
%token <ival> T_DATASIZE
%token <ival> T_AGGR
%token        T_PGLEN
%token        T_LEFTMGN
%token        T_PRNOPTION
%token <ival> T_PRNFLAG
%token        T_PORTRAIT_PGLEN
%token        T_LANDSCAPE_PGLEN
%token <lval> T_LAYOUT

%type  <ival>  fldfmt_options_list fmt_options_list
%type  <lval>  outwd_spec
%type  <ffval> format_definition
%type  <ival>  assignment band_options
%type  <ival>  prn_options prn_option_list

%%

report_file : directive_list report_list;

directive_list : directive | directive_list directive;

directive : T_PORTRAIT_PGLEN '=' T_INTEGER ';'
{
	default_portrait_pglen = $3;
} | T_LANDSCAPE_PGLEN '=' T_INTEGER ';'
{
	default_landscape_pglen = $3;
};

report_list : report | report_list report;

report : rpt_decl data_decl_def prn_param_list var_decl_list var_init_list line_layout bands_list T_END
{
	if(_report->pglen == 0)
		if(_report->prnoptions & SPRN_LANDSCAPE)
			_report->pglen = default_landscape_pglen;
		else
			_report->pglen = default_portrait_pglen;
	check_bands();
	accept_report(_report);
};

prn_param_list : prn_param_item | prn_param_list prn_param_item;

prn_param_item :
{
} | T_PGLEN '=' T_INTEGER ';'
{
	_report->pglen = $3;
} | T_LEFTMGN '=' T_INTEGER ';'
{
	_report->leftmarg = $3;
} | prn_options ';'
{
	_report->prnoptions |= $1;
};

prn_options : T_PRNOPTION '=' prn_option_list
{
	$$ = $3;
};

prn_option_list : prn_option_list '|' T_PRNFLAG
{
	$$ = $1 | $3;
} | T_PRNFLAG
{
	$$ = $1;
};

data_decl_def :
 | data_decl_head ';'
;

data_decl_head : T_DATA T_NAME
{
	_report->data_name = newStr($2);
	ald_data = new PPALDD(ResFileName, PPALDD::GetResID(ResFileName, $2).ID, 0);
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
		_report->main_id = PPALDD::GetResID(ResFileName, $2).ParentID;
		for (ii = 1; ii <= ald_data->GetListCount(); ii++)
			scan_aldd(ii);
		addvar_to_tab();
	}
};

var_init_list :
{
} | var_init_list aggr_init
{
} | aggr_init
{
};

aggr_init : assignment T_AGGR '(' T_VAR ')' ';'
{
	if(_report->setAggrToField($1, $2, var_to_id($4)) == 0)
		yyerror("Невозможно установить агрегатную функцию");
};

assignment : T_VAR '='
{
	$$ = var_to_id($1);
};

rpt_decl : T_REPORT T_NAME
{
	delete ald_data;
	ald_data = 0;
	delete _report;
	_report = new SReport($2);
	vartab.freeAll();
	tmp_id = 0;
};

var_decl_list :
{
} | var_decl_list var_decl
{
	addvar_to_tab();
} | var_decl
{
	addvar_to_tab();
};

var_decl : var_type var_list format_definition ';'
{
	for(i = 0; i < varlistcount; i++) {
		varlist[i].format = $3.format;
		varlist[i].fldfmt = $3.fldfmt;
	}
};

var_type : T_TYPE
{
	_type = $1;
	varlistcount = 0;
	free(varlist);
	varlist = 0;
};

var_list : var_list ',' var_item
{
	addvar_to_list();
} | var_item
{
	addvar_to_list();
};

var_item : T_VAR
{
	memset(&varitem, 0, sizeof(varitem));
	strcpy(varitem.name, $1);
	varitem.type = _type;
} | T_VAR T_DATASIZE
{
	memset(&varitem, 0, sizeof(varitem));
	strcpy(varitem.name, $1);
	if(LoByte($2))
		varitem.type = MKSTYPED(GETSTYPE(_type), HiByte($2), LoByte($2));
	else
		varitem.type = MKSTYPE(GETSTYPE(_type), HiByte($2));
};

format_definition : '{' fmt_options_list outwd_spec fldfmt_options_list '}'
{
	SETSFMTFLAG($3, $2);
	$$.format = $3;
	$$.fldfmt = $4;
};

fmt_options_list : T_INTEGER
{
	$$ = 0;
} | fmt_options_list '|' T_FMTFLAG
{
	$$ = $1 | $3;
} | T_FMTFLAG {
	$$ = $1;
};

outwd_spec :
{
	$$ = 0;
} | ',' T_INTEGER
{
	$$ = 0;
} | ',' T_INTEGER ',' T_INTEGER
{
	$$ = MKSFMTD(0, $4, 0);
};

fldfmt_options_list :
{
	$$ = 0;
} | fldfmt_options_list '|' T_FLDFMTFLAG
{
	$$ = $1 | $3;
} | ',' T_FLDFMTFLAG
{
	$$ = $2;
};

line_layout : T_LAYOUT
{
	linewidth = $1;
};

bands_list : bands_list band_def
{
} | band_def
{
};

band_def : T_BAND grp_fld_list ';'
{
	getband($1, 0);
} | T_BAND grp_fld_list '{' band_options '}' ';'
{
	getband($1, $4);
};

grp_fld_list :
{
	grp_flds[0] = 0;
} | grp_fld_list ',' T_VAR
{
	grp_flds[(grp_flds[0])++] = var_to_id($3);
} | T_VAR
{
	grp_flds[0] = 1;
	grp_flds[1] = var_to_id($1);
};

band_options : band_options '|' T_GRPFMTFLAG
{
	$$ = $1 | $3;
} | T_GRPFMTFLAG
{
	$$ = $1;
};

%%
