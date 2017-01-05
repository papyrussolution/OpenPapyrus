/*
	PPALDDC.Y
	Copyright (c) A.Osolotkin, A.Sobolev 1999-2003, 2005
	Part of project Papyrus
*/
%{
#include <slib.h>
#include "ppalddc.h"

// Prototypes
static void SetupVarDef(const char * pName, long typ);
static NodeType GetNodeType1(long opType, NodeType & arg1);
static NodeType GetNodeType2(long opType, NodeType & arg1, NodeType & arg2);
static NodeType GetNodeType3(long opType, NodeType & arg1, NodeType & arg2, NodeType & arg3);

VARDEFS  * VarDefs = 0;
ITERDEFS * IterDefs = 0;
DATADEFS * DataDefs = 0;
ORDERDEFS * OrderDefs = 0;
long ary_type = 0;

extern FILE * yyin;

char in_file_name[MAXPATH];
char h_file_name[MAXPATH];
char cpp_file_name[MAXPATH];
char hflt_file_name[MAXPATH];
char cppflt_file_name[MAXPATH];
char bin_file_name[MAXPATH];
long have_key  = 0;
long have_iter = 0;
long cur_iter  = 0;
long link_var  = 0;
long node_offset = 0;
long ext_cnt   = 0;
long IsOrder   = 0;
FormatSpec fspec;

typedef struct {
	void * yyin_buf;
	char   fname[_MAX_PATH];
	long   yyin_line;
} YYIN_STR;

extern long yyin_cnt;
extern YYIN_STR * yyin_struct;

NodeType   args[10];
FILE     * nodefile;

void yyerror(char * str)
{
	if(yyin_cnt)
		printf("Error %s(%d): %s\n", yyin_struct[yyin_cnt-1].fname, yyline, str);
	else
		printf("Error (%d): %s\n", yyline, str);
	exit(-1);
}

void yyerr_invparam(char * pFuncName)
{
	char   msg_buf[256];
	sprintf(msg_buf, "Invalid parameters in function %s", pFuncName);
	yyerror(msg_buf);
}

static int DotFound(char * pB)
{
	if(*(pB-1) == '.')
		pB--;
	switch(*--pB) {
		case ':'  :
			if(*(pB-2) != '\0')
				break;
		case '/'  :
		case '\\' :
		case '\0' :
			return 1;
	}
	return 0;
}

void fnmerge(register char *pathP, const char *driveP, const char *dirP, const char *nameP, const char *extP)
{
	_makepath(pathP,driveP,dirP,nameP,extP);
}

%}

%union {
	long   lval;
	double dval;
	char * sval;
	NodeType nType;
}

%token T_TDBL
%token T_TLONG
%token T_TSTR
%token T_TDEC
%token T_TMON
%token T_TINT
%token T_TINT16
%token T_TDATE
%token T_TTIME
%token T_TFLOAT
%token T_TLINK
%token T_TKEY
%token T_ORDER
%token T_ARRAY_LONG
%token T_ARRAY_DBL
%token T_COUNTER

%token T_DATA
%token T_FILTER
%token T_ITER
%token T_LBR
%token T_RBR
%token T_DECLARE
%token T_EQ
%token T_IF

%token T_LEFT
%token T_MIDL
%token T_RIGHT
%token T_WRAP
%token T_FORMATPERIOD
%token T_PRINTABLEBARCODE
%token T_SQRT
%token TS_MONEY
%token TS_DATE
%token TWS_DATE
%token TS_NUMBER
%token T_MKSTR
%token T_ROUND
%token T_TRUNC
%token T_ABS

%token T_CODEGEN
%token T_NODESTRUCTORMETHOD
%token T_NODESTROYMETHOD

%token <sval> T_IDENT
%token <sval> T_STR
%token <sval> T_FMT
%token <lval> T_INTEGER
%token <sval> T_VALUE

%left '?'
%left T_LOG_OR
%left T_LOG_AND
%left T_EQUL T_NOTEQUL
%left T_GREAT T_LESS_EQ T_GREAT_EQ T_LESS
%left T_BIT_OR
%left T_BIT_XOR
%left T_BIT_AND
%left '+' '-'
%left '*' '/'
%left '.'

%nonassoc IFXS
%nonassoc IFX
%nonassoc T_ELSE
%nonassoc TO_DBL TO_INT TO_STR
%nonassoc T_LEN
%nonassoc UPLUS UMINUS T_BIT_NOT

%type <nType> chain_ident expr chain_tail

%%

/*descr_file : descr_list;*/

descr_list :
| descr_list descr_struct;

descr_struct : list_head parent_str T_LBR declare_list list_body list_tail
| filt_head T_LBR list_body list_tail;

filt_head : T_FILTER T_IDENT
{
	if(strlen($2) > 14)
		yyerror("Identifier too long. It mast be less or equal than 14 character");
	iter = 0; DataRedef($2, 2);
	have_key = 0;
	have_iter = 0;
	STRNSCPY(DataDefs[datacnt].Name, $2);
	DataDefs[datacnt].TypeId = 2;
	DataDefs[datacnt].HasParent = 0;
	DataDefs[datacnt].ParentId = datacnt+1;
};

list_head : T_DATA T_IDENT
{
	if(strlen($2) > 14)
		yyerror("Identifier too long. It mast be less or equal than 14 character");
	iter = 0;
	DataRedef($2, 1);
	have_key = 0;
	have_iter = 0;
	STRNSCPY(DataDefs[datacnt].Name, $2);
	DataDefs[datacnt].TypeId = 1;
	DataDefs[datacnt].DeclareFlags = 0;
};

parent_str :
{
	DataDefs[datacnt].HasParent = 0;
	DataDefs[datacnt].ParentId = datacnt+1;
	DataDefs[datacnt].ParentName[0] = 0;
} | ':' T_IDENT
{
	DataDefs[datacnt].HasParent = 1;
	DataDefs[datacnt].ParentId = DataDefs[GetDataID($2, 0)-1].ParentId;
	STRNSCPY(DataDefs[datacnt].ParentName, $2);
};

declare_list : declare_statement | declare_list declare_statement
{
}

declare_statement :
{
} | T_DECLARE T_STR
{
	if(strcmp($2, "Destroy") == 0)
		DataDefs[datacnt].DeclareFlags |= DATADECLF_DESTROY;
	else if(strcmp($2, "DOSSTUB") == 0)
		DataDefs[datacnt].DeclareFlags |= DATADECLF_DOSSTUB;
	else {
		char msgbuf[256];
		sprintf(msgbuf, "Invalid decalre statement \"%s\"", $2);
	}
}

list_tail : T_RBR
{
	datacnt++;
}

list_body : s_field | list_body s_field;

s_field : s_type
{
	if(DataDefs[datacnt].HasParent)
		yyerror("Could have only <id>=<expr>;");
} dim alias format sf_tail
| s_array alias format sf_tail;

s_array : T_ARRAY_LONG T_IDENT
{
	SetupVarDef($2, MKSTYPE(S_INT | 0x80, 4));
} | T_ARRAY_DBL T_IDENT
{
	SetupVarDef($2, MKSTYPE(S_FLOAT | 0x80, 8));
}

s_type : T_TINT T_IDENT
{
	SetupVarDef($2, MKSTYPE(S_INT, 4));
} | T_TKEY T_IDENT
{
	if(DataDefs[datacnt].TypeId == 2)
		yyerror("Could not place 'key' in SFilter");
	if(iter)
		yyerror("Could not place 'key' in iterator");
	if(DataDefs[datacnt].HasParent)
		yyerror("Could not place 'key' in derived struct");
	if(have_iter)
		yyerror("Could not place 'key' in struct with iteration");
	have_key = 1;
	VarDefs[varcnt].rel = -1;
	SetupVarDef($2, MKSTYPE(S_INT, 4));
} | T_TLONG T_IDENT
{
	SetupVarDef($2, MKSTYPE(S_INT, 4));
} | T_TDBL T_IDENT
{
	SetupVarDef($2, MKSTYPE(S_FLOAT, 8));
} | T_TSTR T_IDENT
{
	SetupVarDef($2, T_TSTR);
} | T_TDEC T_IDENT
{
	SetupVarDef($2, T_TDEC);
} | T_TMON T_IDENT
{
	SetupVarDef($2, T_TMON);
} | T_TINT16 T_IDENT
{
	SetupVarDef($2, MKSTYPE(S_INT, 2));
} | T_TDATE T_IDENT
{
	SetupVarDef($2, MKSTYPE(S_DATE, sizeof(LDATE)));
} | T_TTIME T_IDENT
{
	SetupVarDef($2, MKSTYPE(S_TIME, sizeof(LTIME)));
} | T_TFLOAT T_IDENT
{
	SetupVarDef($2, MKSTYPE(S_FLOAT, 4));
} | T_COUNTER T_IDENT
{
	if(DataDefs[datacnt].TypeId == 2)
		yyerror("Could not place 'autoinc' in SFilter");
	if(!iter)
		yyerror("Could not place 'autoinc' out of iterator");
	SetupVarDef($2, MKSTYPE(S_AUTOINC, 4));
}

dim :
{
	if(VarDefs[varcnt].type == T_TDEC || VarDefs[varcnt].type == T_TSTR)
		yyerror("Must be indexed.");
	if(VarDefs[varcnt].type == T_TMON)
		VarDefs[varcnt].type = MKSTYPED(S_DEC, 8, 2);
} | '[' T_INTEGER ']'
{
	if(VarDefs[varcnt].type == T_TDEC || VarDefs[varcnt].type == T_TMON)
		VarDefs[varcnt].type = MKSTYPED(S_DEC, $2, 2);
	else if(VarDefs[varcnt].type == T_TSTR)
		VarDefs[varcnt].type = MKSTYPE(S_ZSTRING, $2);
	else {
		printf("VarDefs[varcnt].type = %08lx\n", VarDefs[varcnt].type);
		yyerror("Could not be indexed.");
	}
} | '[' T_INTEGER '.' T_INTEGER ']'
{
	if(VarDefs[varcnt].type != T_TDEC && VarDefs[varcnt].type != T_TMON)
		yyerror("Could not be indexed.");
	VarDefs[varcnt].type = MKSTYPED(S_DEC, $2, $4);
}

alias :
{
	VarDefs[varcnt].alias[0] = 0;
} | T_IDENT
{
	strncpy(VarDefs[varcnt].alias, $1, 15);
	VarDefs[varcnt].alias[15] = 0;
}

format :
{
	VarDefs[varcnt].format = SetFormat(0, VarDefs[varcnt].type);
} | T_FMT
{
	VarDefs[varcnt].format = SetFormat($1, VarDefs[varcnt].type);
}

sf_tail : ';'
{
	VarDefs[varcnt].dataID = datacnt;
	VarDefs[varcnt].iterID = iter ? cur_iter : 0;
	VarsRedef(VarDefs[varcnt].name, datacnt, VarDefs[varcnt].iterID);
	varcnt++;
}

s_field : T_TLINK T_IDENT
{
	VarDefs[varcnt].rel = GetDataID($2, 1);
} lf_tail alias format lf_post_tail
{
	if(DataDefs[datacnt].TypeId == 2)
		yyerror("Could not place 'link' in SFilter");
	if(DataDefs[datacnt].HasParent)
		yyerror("Could have only <id>=<expr>;");
	varcnt++;
};

lf_tail : T_IDENT
{
	VarDefs[varcnt].dataID = datacnt;
	VarDefs[varcnt].iterID = iter ? cur_iter : 0;
	VarDefs[varcnt].type = MKSTYPE(S_INT, 4);
	STRNSCPY(VarDefs[varcnt].name, $1);
	VarsRedef($1, datacnt, VarDefs[varcnt].iterID);
};

lf_post_tail : ';' ;

s_field : exf_head T_EQ expr
{
	if(DataDefs[datacnt].TypeId == 2)
		yyerror("Could not place 'equality' in SFilter");
	VarDefs[varcnt].type = $3.ret_val;
} alias format ';'
{
	VarDefs[varcnt].node_offset = node_offset;
	node_offset += VarDefs[varcnt].node_cnt * sizeof(NodeType) + ext_cnt;
	varcnt++;
};

exf_head : T_IDENT
{
	VarDefs[varcnt].dataID = datacnt;
	VarDefs[varcnt].iterID = iter ? cur_iter : 0;
	STRNSCPY(VarDefs[varcnt].name, $1);
	ext_cnt = 0;
	VarsRedef($1, datacnt, VarDefs[varcnt].iterID);
};

expr : T_VALUE
{
	$$.type = OP_TERM;
	$$.ret_val = MKSTYPE(S_FLOAT, 8);
	$$.dbl_val = atof($1);
	$$.ext_val = 0;
	SaveNode($$);
} | T_INTEGER
{
	$$.type = OP_TERM;
	$$.ret_val = MKSTYPE(S_INT, 4);
	$$.lng_val = $1;
	$$.ext_val = 0;
	SaveNode($$);
} | T_STR
{
	$$.type = OP_TERM;
	$$.ext_val = (strlen($1) < MAX_STR_LEN) ? strlen($1) : (MAX_STR_LEN-1);
	$$.ret_val = MKSTYPE(S_ZSTRING, ++$$.ext_val);
	SaveNode($$);
	fwrite($1, $$.ext_val, 1, nodefile);
	ext_cnt += $$.ext_val;
}
| '(' expr ')'                       { $$ = $2; }
| expr '?' expr ':' expr %prec IFXS  { SaveNode($$ = GetNodeType3(OP_IF_ELSE, $1, $3, $5)); }
| T_IF '(' expr ')' expr %prec IFX   { SaveNode($$ = GetNodeType2(OP_IF, $3, $5)); }
| T_IF '(' expr ')' expr T_ELSE expr { SaveNode($$ = GetNodeType3(OP_IF_ELSE, $3, $5, $7)); }
| '~' expr %prec T_BIT_NOT           { SaveNode($$ = GetNodeType1(OP_BIT_NOT, $2)); }
| '-' expr %prec UMINUS              { SaveNode($$ = GetNodeType1(OP_UMINUS, $2)); }
| '+' expr %prec UPLUS               { SaveNode($$ = GetNodeType1(OP_UPLUS, $2)); }
| T_LEN expr                         { SaveNode($$ = GetNodeType1(OP_LEN, $2)); }
| T_SQRT '(' expr ')'
{
	if(GETSTYPE($3.ret_val) == S_INT) {
		$$.type = OP_SQRT_INT;
		$$.ret_val = MKSTYPE(S_FLOAT, 8);
		$$.ext_val = 0;
		$$.fnk_val.VarCount = 1;
		SaveNode($$);
	}
	else if(GETSTYPE($3.ret_val) == S_FLOAT) {
		$$.type = OP_SQRT_DBL;
		$$.ret_val = MKSTYPE(S_FLOAT, 8);
		$$.ext_val = 0;
		$$.fnk_val.VarCount = 1;
		SaveNode($$);
	}
	else
		yyerror("Not correct type of parameter");
} | expr '[' expr ']'
{
	ary_type = GETSTYPE($1.ret_val);
	if(ary_type & 0x80 && (GETSTYPE($3.ret_val) == S_INT || GETSTYPE($3.ret_val) == S_AUTOINC)) {
		switch(ary_type & 0x7f) {
			case S_INT:
				$$.ret_val = MKSTYPE(S_INT, 4);
				$$.type = OP_ARRAY_MEMBER_INT;
				break;
			case S_FLOAT:
				$$.ret_val = MKSTYPE(S_FLOAT, 8);
				$$.type = OP_ARRAY_MEMBER_DBL;
				break;
			default:
				yyerror("Not correct type of parameter");
		}
		$$.ext_val = 0;
		$$.fnk_val.VarCount = 2;
		SaveNode($$);
	}
	else
		yyerror("Not correct type of parameter");
} | T_ROUND '(' expr ',' expr ')'
{
	$$.type = OP_ROUND;
	$$.ret_val = MKSTYPE(S_FLOAT, 8);
	$$.ext_val = 0;
	$$.fnk_val.VarCount = 2;
	$$.fnk_val.VarTypes[0] = $3.ret_val;
	$$.fnk_val.VarTypes[1] = $5.ret_val;
	if((GETSTYPE($3.ret_val) == S_DEC || GETSTYPE($3.ret_val) == S_FLOAT) && GETSTYPE($5.ret_val) == S_INT)
		SaveNode($$);
	else
		yyerror("Not correct type of parameter");
} | T_TRUNC '(' expr ',' expr ')'
{
	$$.type = OP_TRUNC;
	$$.ret_val = MKSTYPE(S_FLOAT, 8);
	$$.ext_val = 0;
	$$.fnk_val.VarCount = 2;
	$$.fnk_val.VarTypes[0] = $3.ret_val;
	$$.fnk_val.VarTypes[1] = $5.ret_val;
	if((GETSTYPE($3.ret_val) == S_DEC || GETSTYPE($3.ret_val) == S_FLOAT) && GETSTYPE($5.ret_val) == S_INT)
		SaveNode($$);
	else
		yyerror("Not correct type of parameter");
} | TO_DBL expr
{
	if(GETSTYPE($2.ret_val) == S_FLOAT)
		$$ = $2;
	else
		args[0] = $2;
	$$ = GetNodeType(OP_TO_DBL, args);
	SaveNode($$);
} | TO_INT expr
{
	if(GETSTYPE($2.ret_val) == S_INT)
		$$ = $2;
	else
		args[0] = $2;
	$$ = GetNodeType(OP_TO_INT, args);
	SaveNode($$);
} | TS_MONEY '(' expr ',' T_INTEGER ')'
{
	if(GETSTYPE($3.ret_val) == S_FLOAT) {
		$$.type = OP_MONEY;
		$$.ret_val = MKSTYPE(S_ZSTRING, $5+1);
		$$.ext_val = 0;
		$$.fnk_val.VarCount = 1;
		SaveNode($$);
	}
	else
		yyerror("Not correct type of parameter");
} | TS_MONEY '(' expr ',' T_INTEGER ',' T_INTEGER ')'
{
	if(GETSTYPE($3.ret_val) == S_FLOAT) {
		$$.type = OP_MONEY_FULL;
		$$.ret_val = MKSTYPE(S_ZSTRING, $5+1);
		$$.ext_val = 0;
		$$.fnk_val.VarCount = 1;
		SaveNode($$);
	}
	else
		yyerror("Not correct type of parameter");
} | TS_DATE '(' expr ',' T_INTEGER ')'
{
	if(GETSTYPE($3.ret_val) == S_DATE) {
		$$.type = OP_DATE;
		$$.ret_val = MKSTYPE(S_ZSTRING, $5+1);
		$$.ext_val = 0;
		$$.fnk_val.VarCount = 1;
		SaveNode($$);
	}
	else
		yyerror("Not correct type of parameter");
} | TWS_DATE '(' expr ',' T_INTEGER ')'
{
	if(GETSTYPE($3.ret_val) == S_DATE) {
		$$.type = OP_WORD_DATE;
		$$.ret_val = MKSTYPE(S_ZSTRING, $5+1);
		$$.ext_val = 0;
		$$.fnk_val.VarCount = 1;
		SaveNode($$);
	}
	else
		yyerr_invparam("Date2WStr(date, int)");
} | TS_NUMBER '(' expr ',' T_INTEGER ')'
{
	if(GETSTYPE($3.ret_val) == S_FLOAT) {
		$$.type = OP_NUMBER;
		$$.ret_val = MKSTYPE(S_ZSTRING, $5+1);
		$$.ext_val = 0;
		$$.fnk_val.VarCount = 1;
		SaveNode($$);
	}
	else
		yyerr_invparam("Num2Str(number, int)");
} | T_MKSTR '(' T_INTEGER ',' T_INTEGER ')'
{
} | T_LEFT '(' expr ',' expr ')'
{
	if(GETSTYPE($3.ret_val) == S_ZSTRING && GETSTYPE($5.ret_val) == S_INT)
		SaveNode($$ = GetNodeType2(OP_LEFT, $3, $5));
	else
		yyerr_invparam("left(string, int)");
} | T_RIGHT '(' expr ',' expr ')'
{
	if(GETSTYPE($3.ret_val) == S_ZSTRING && GETSTYPE($5.ret_val) == S_INT)
		SaveNode($$ = GetNodeType2(OP_RIGHT, $3, $5));
	else
		yyerr_invparam("right(string, int)");
} | T_MIDL '(' expr ',' expr ',' expr ')' {
	if(GETSTYPE($3.ret_val) == S_ZSTRING && GETSTYPE($5.ret_val) == S_INT && GETSTYPE($7.ret_val) == S_INT)
		SaveNode($$ = GetNodeType3(OP_MIDL, $3, $5, $7));
	else
		yyerr_invparam("midl(string, int, int)");
} | T_WRAP '(' expr ',' expr ')'
{
	if((GETSTYPE($3.ret_val) == S_ZSTRING) && (GETSTYPE($5.ret_val) == S_INT))
		SaveNode($$ = GetNodeType2(OP_WRAP, $3, $5));
	else
		yyerr_invparam("wrap(string, int)");
} | TO_STR '(' expr ',' T_FMT ')'
{
	if(GETSTYPE($3.ret_val) == S_ZSTRING)
		$$ = $3;
	else {
		$$.type = OP_TERM;
		$$.ret_val = MKSTYPE(S_INT, 4);
		fspec = SetFormat($5, $3.ret_val);
		if((GETSTYPE($3.ret_val) == S_FLOAT) || (GETSTYPE($3.ret_val) == S_DEC))
			$$.lng_val = MKSFMTD(fspec.len, fspec.prec, fspec.flags);
		else
			$$.lng_val = MKSFMT(fspec.len, fspec.flags);
		SaveNode($$);
		args[0] = $3;
		$$ = GetNodeType(OP_TO_STR, args);
		$$.ret_val = MKSTYPE(S_ZSTRING, fspec.len ? fspec.len+1 : 32);
		SaveNode($$);
	}
} | T_FORMATPERIOD '(' expr ',' expr ')'
{
	if(GETSTYPE($3.ret_val) == S_DATE && GETSTYPE($5.ret_val) == S_DATE)
		SaveNode($$ = GetNodeType2(OP_FORMATPERIOD, $3, $5));
	else
		yyerr_invparam("formatperiod(date, date)");
} | T_PRINTABLEBARCODE '(' expr ',' expr ')'
{
	if(GETSTYPE($3.ret_val) == S_ZSTRING && GETSTYPE($5.ret_val) == S_INT)
		SaveNode($$ = GetNodeType2(OP_PRINTABLEBARCODE, $3, $5));
	else
		yyerr_invparam("PrintableBarcode(string, int)");
} | T_ABS '(' expr ')'
{
	if(GETSTYPE($3.ret_val) == S_FLOAT || GETSTYPE($3.ret_val) == S_DEC)
		SaveNode($$ = GetNodeType1(OP_ABS_DBL, $3));
	else if(GETSTYPE($3.ret_val) == S_INT)
		SaveNode($$ = GetNodeType1(OP_ABS_INT, $3));
	else
		yyerr_invparam("abs(number)");
}
| expr T_EQUL expr     { SaveNode($$ = GetNodeType2(OP_EQUL,     $1, $3)); }
| expr T_NOTEQUL expr  { SaveNode($$ = GetNodeType2(OP_NOTEQUL,  $1, $3)); }
| expr T_LESS expr     { SaveNode($$ = GetNodeType2(OP_LESS,     $1, $3)); }
| expr T_LESS_EQ expr  { SaveNode($$ = GetNodeType2(OP_LESS_EQ,  $1, $3)); }
| expr T_GREAT expr    { SaveNode($$ = GetNodeType2(OP_GREAT,    $1, $3)); }
| expr T_GREAT_EQ expr { SaveNode($$ = GetNodeType2(OP_GREAT_EQ, $1, $3)); }
| expr T_LOG_OR expr   { SaveNode($$ = GetNodeType2(OP_LOG_OR,   $1, $3)); }
| expr T_LOG_AND expr  { SaveNode($$ = GetNodeType2(OP_LOG_AND,  $1, $3)); }
| expr T_BIT_OR expr   { SaveNode($$ = GetNodeType2(OP_BIT_OR,   $1, $3)); }
| expr T_BIT_XOR expr  { SaveNode($$ = GetNodeType2(OP_BIT_XOR,  $1, $3)); }
| expr T_BIT_AND expr  { SaveNode($$ = GetNodeType2(OP_BIT_AND,  $1, $3)); }
| expr '+' expr        { SaveNode($$ = GetNodeType2(OP_PLUS,     $1, $3)); }
| expr '-' expr        { SaveNode($$ = GetNodeType2(OP_MINUS,    $1, $3)); }
| expr '*' expr        { SaveNode($$ = GetNodeType2(OP_MULT,     $1, $3)); }
| expr '/' expr        { SaveNode($$ = GetNodeType2(OP_DIV,      $1, $3)); }
| chain_ident          { $$ = $1; };

chain_ident : T_IDENT
{
	if((link_var = GetVarID($1, datacnt, iter ? cur_iter : 0)) >= 0) {
		$$.type = OP_IDENT;
		$$.lnk_val.DataId = datacnt;
		$$.lnk_val.IterId = VarDefs[link_var].iterID ? cur_iter : 0;
		$$.lnk_val.VarId = link_var;
		$$.ret_val = VarDefs[link_var].type;
		SaveNode($$);
	}
	else
		yyerror("Bad identifier");
} | chain_ident '.' chain_tail
{
	$$.type = OP_LINK;
	$$.ret_val = $3.ret_val;
	$$.fnk_val.VarCount = 2;
	$$.fnk_val.VarTypes[0] = $1.ret_val;
	$$.fnk_val.VarTypes[1] = $3.ret_val;
	SaveNode($$);
}

chain_tail : T_IDENT
{
	if(VarDefs[link_var].rel <= 0)
		yyerror("Not 'link' variable");
	if((link_var = GetVarID($1, VarDefs[link_var].rel-1, 0)) >= 0) {
		$$.type = OP_IDENT;
		$$.lnk_val.DataId = VarDefs[link_var].dataID;
		$$.lnk_val.IterId = VarDefs[link_var].iterID;
		$$.lnk_val.VarId = link_var;
		$$.ret_val = VarDefs[link_var].type;
		SaveNode($$);
	}
	else
		yyerror("Bad identifier");
}

s_field : if_head T_LBR order_body list_body if_tail;

order_body :
{
} | T_ORDER
{
	if(!iter)
		yyerror("Order must be inside an iteration");
	if(IsOrder)
		yyerror("Could be just one Order inside an iteration");
	if(DataDefs[datacnt].HasParent)
		yyerror("Could not be inside child struct");
	IsOrder = 1;
} T_LBR order_list T_RBR
{
	IsOrder = 0;
}

order_list : order_field | order_list ',' order_field;

order_field : T_IDENT
{
	OrderDefs[ordercnt].iterID = itercnt;
	OrderDefs[ordercnt].dataID = datacnt;
	STRNSCPY(OrderDefs[ordercnt].name, $1);
	ordercnt++;
}

if_head : T_ITER T_IDENT
{
	if(DataDefs[datacnt].TypeId == 2)
		yyerror("Could not place 'iteration' in SFilter");
	if(iter)
		yyerror("Embedded iteration");
	if(!strcmp("Head", $2))
		yyerror("Reserved symbol");
	if(!strcmp("default", $2))
		yyerror("Reserved symbol");
	if(have_key)
		yyerror("Could not place iteration in struct with 'key'");
	have_iter = 1;
	cur_iter = itercnt;
	if(DataDefs[datacnt].HasParent)
		cur_iter = GetIterID(datacnt, $2);
	else
		STRNSCPY(IterDefs[itercnt].name, $2);
	iter = 1;
} | T_ITER
{
	if(iter)
		yyerror("Embedded iterations");
	if(have_key)
		yyerror("Could not place iteration in struct with 'key'");
	have_iter = 1;
	cur_iter = itercnt;
	if(DataDefs[datacnt].HasParent)
		cur_iter = GetIterID(datacnt, "default");
	else
		STRNSCPY(IterDefs[itercnt].name, "default");
	iter = 1;
}

if_tail : T_RBR
{
	IterDefs[cur_iter].dataID = datacnt;
	IterDefs[cur_iter].type = T_ITER;
	if(!DataDefs[datacnt].HasParent)
		itercnt++;
	iter = 0;
};

%%

void SaveNode(NodeType nt)
{
	VarDefs[varcnt].node_cnt++;
	fwrite(&nt, sizeof(nt), 1, nodefile);
}

static void SetupVarDef(const char * pName, long typ)
{
	VarDefs[varcnt].type = typ;
	STRNSCPY(VarDefs[varcnt].name, pName);
}

int main(int argc, char *argv[])
{
	FILE *fs;
	long i, j, k, l, m, len, fld_offset, ext_fld_offset, n;
	PpalddHead Head = { {'P','P','A','L','D','D',' ',0}, 0, 0, 0, 0, 0, 0};
	PpalddDataHead DataHead;
	PpalddListHead IterHead;
	PpalddField    Field;
	PpalddOrder    Order;
	PpalddIdxData  S_id;
	NodeType       Node;

	STRNSCPY(PREFIX, "PPALDD");
	/*
	yydebug = 1;
	*/
	if(argc < 2)
		yyerror("Too few params...\nMust be: ppalddc source[.ald] [h-file] [cpp-file] [bin-file].\n");
	replaceExt(STRNSCPY(in_file_name, argv[1]), ".ALD", 0);
	if(argc > 2) {
		replaceExt(STRNSCPY(h_file_name, argv[2]), ".H", 0);
		replaceExt(STRNSCPY(hflt_file_name, argv[2]), ".H_", 0);
	}
	else {
		replaceExt(STRNSCPY(h_file_name, in_file_name), ".H", 1);
		replaceExt(STRNSCPY(hflt_file_name, in_file_name), ".H_", 1);
	}
	if(argc > 3) {
		replaceExt(STRNSCPY(cpp_file_name, argv[3]), ".CPP", 0);
		replaceExt(STRNSCPY(cppflt_file_name, argv[3]), ".CP_", 0);
	}
	else {
		replaceExt(STRNSCPY(cpp_file_name, in_file_name), ".CPP", 1);
		replaceExt(STRNSCPY(cppflt_file_name, in_file_name), ".CP_", 1);
	}
	if(argc > 4)
		replaceExt(STRNSCPY(bin_file_name, argv[4]), ".BIN", 0);
	else
		replaceExt(STRNSCPY(bin_file_name, in_file_name), ".BIN", 1);
	nodefile = fopen("node.tmp", "w+b");
	if(!(fs = fopen(h_file_name, "w+"))) {
		sprintf(tmpbuff, "Could not open file \"%s\".\n", h_file_name);
		yyerror(tmpbuff);
	}
	fclose(fs);
	if(!(fs = fopen(hflt_file_name, "w+"))) {
		sprintf(tmpbuff, "Could not open file \"%s\".\n", hflt_file_name);
		yyerror(tmpbuff);
	}
	fclose(fs);
	if(!(fs = fopen(bin_file_name, "w+b"))) {
		sprintf(tmpbuff, "Could not open file \"%s\".\n", bin_file_name);
		yyerror(tmpbuff);
	}
	fclose(fs);
	if(!(fs = fopen(cpp_file_name, "w+"))) {
		sprintf(tmpbuff, "Could not open file \"%s\".\n", cpp_file_name);
		yyerror(tmpbuff);
	}
	fclose(fs);
	if(!(fs = fopen(cppflt_file_name, "w+"))) {
		sprintf(tmpbuff, "Could not open file \"%s\".\n", cppflt_file_name);
		yyerror(tmpbuff);
	}
	fclose(fs);
	yyin = fopen(in_file_name, "r");
	if(!(yyin = fopen(in_file_name, "r"))) {
		sprintf(tmpbuff, "Could not open file \"%s\".\n", in_file_name);
		yyerror(tmpbuff);
	}
	Head.Len = sizeof(Head);
	VarDefs = 0;
	DataDefs = 0;
	IterDefs = 0;
	OrderDefs = 0;
	VarDefs   = (VARDEFS *)calloc(BUFF_SIZE, sizeof(VARDEFS));
	DataDefs  = (DATADEFS *)calloc(BUFF_SIZE, sizeof(DATADEFS));
	IterDefs  = (ITERDEFS *)calloc(BUFF_SIZE, sizeof(ITERDEFS));
	OrderDefs = (ORDERDEFS *)calloc(BUFF_SIZE, sizeof(ORDERDEFS));
	if(VarDefs && DataDefs && IterDefs && OrderDefs) {
		IterDefs[0].dataID = -1;
		IterDefs[0].type = 0;
		STRNSCPY(IterDefs[0].name, "Head");

		yyparse();

		/*
			Запись *.h файла
		*/
		fs = fopen(h_file_name, "w+");
		fprintf(fs,"\n//\n// This file was compiled from %s by %sC.EXE\n//\n\n", in_file_name, PREFIX);
		WriteHFile(fs, 1);
		fclose(fs);
		/*
			Запись *.h_ файла
		*/
		fs = fopen(hflt_file_name, "w+");
		fprintf(fs,"\n//\n// This file was compiled from %s by %sC.EXE\n//\n\n", in_file_name, PREFIX);
		WriteHFile(fs, 2);
		fclose(fs);
		/*
			Запись *.cpp файла
		*/
		fs = fopen(cpp_file_name, "w+");
		fprintf(fs,"\n//\n// This file was compiled from %s by %sC.EXE\n//\n\n", in_file_name, PREFIX);

		// @v5.4.9 fprintf(fs, "#ifdef __BORLANDC__\n");
		// @v5.4.9 fprintf(fs, "\t#pragma hdrfile \"PPDEF.SYM\"\n");
		// @v5.4.9 fprintf(fs, "#endif\n");
		fprintf(fs, "#include <pp.h>\n");
		fprintf(fs, "#include <ppdlgs.h>\n");
		fprintf(fs, "#pragma hdrstop\n\n");
		// @v5.4.9 fprintf(fs, "#include <%s>\n", h_file_name);
		WriteCppFile(fs, 1);
		fclose(fs);
		/*
			Запись *.cp_ файла
		*/
		fs = fopen(cppflt_file_name, "w+");
		fprintf(fs,"\n//\n// This file was compiled from %s by %sC.EXE\n//\n\n", in_file_name, PREFIX);

		// @v5.4.9 fprintf(fs, "#ifdef __BORLANDC__\n");
		// @v5.4.9 fprintf(fs, "\t#pragma hdrfile \"PPDEF.SYM\"\n");
		// @v5.4.9 fprintf(fs, "#endif\n");
		fprintf(fs, "#include <pp.h>\n");
		// @v5.4.9 fprintf(fs, "#include <ppobj.h>\n");
		fprintf(fs, "#include <ppdlgs.h>\n");
		fprintf(fs, "#pragma hdrstop\n\n");
		// @v5.4.9 fprintf(fs, "#include <%s>\n", hflt_file_name);
		WriteCppFile(fs, 2);
		fclose(fs);
		/*
			Запись BIN'а
		*/
		fs = fopen(bin_file_name, "w+b");
		Head.DataCount = datacnt;
		Head.Len = sizeof(Head);
		Head.DataOffset = datacnt * sizeof(S_id) + sizeof(Head);
		/*
			Write Header
		*/
		fwrite(&Head, sizeof(Head), 1, fs);
		for(i = 0; i < datacnt; i++)
			fwrite(&S_id, sizeof(S_id), 1, fs);
		/*
			Circle about List of Lists
		*/
		for(i = 0, len = 0; i < datacnt; i++) {
			/*
				set offset of res
			*/
			fseek(fs, i * sizeof(S_id) + sizeof(Head), SEEK_SET);
			S_id.ID = i+1;
			S_id.Offset = len+Head.DataOffset;
			fwrite(&S_id, sizeof(S_id), 1, fs);
			fseek(fs, 0, SEEK_END);

			memset(&DataHead, 0, sizeof(DataHead));
			DataHead.ID       = i+1;
			DataHead.TypeID   = DataDefs[i].TypeId;
			DataHead.ParentID = DataDefs[i].ParentId;
			STRNSCPY(DataHead.Name, DataDefs[i].Name);
			STRNSCPY(DataHead.ParentName, DataDefs[i].ParentName);
			/*
				Counting Lists in List of Lists
			*/
			for(j = 0, k = 0; j < itercnt; j++)
				if(IterDefs[j].dataID == i)
					k++;
			DataHead.ListCnt = k+1;
			fwrite(&DataHead, sizeof(DataHead), 1, fs);
			len += sizeof(DataHead);
			/*
				Circle about List of Fields
			*/
			for(j = 0, k = 1; j < itercnt; j++)
				if(IterDefs[j].dataID == i || !j) {
					memset(&IterHead, 0, sizeof(IterHead));
					IterDefs[j].ID = IterHead.ID = k;
					STRNSCPY(IterHead.Name, IterDefs[j].name);
					IterHead.Type = IterDefs[j].type;
					/*
						Counting Fields and Order in List of Fields
					*/
					for(l = 0, m = 0; l < ordercnt; l++)
						if(OrderDefs[l].dataID == i && OrderDefs[l].iterID == j)
							m++;
					IterHead.OrderCnt = m;
					for(l = 0, m = 0; l < varcnt; l++)
						if(VarDefs[l].dataID == i && VarDefs[l].iterID == j)
							m++;
					IterHead.FieldCnt = m;
					fwrite((void*) &IterHead, sizeof(IterHead), 1, fs);
					len += sizeof(IterHead);
					/*
						Circle about Order
					*/
					for(l = 0, m = 1; l < ordercnt; l++)
						if(OrderDefs[l].dataID == i && OrderDefs[l].iterID == j) {
							Order.ID = m++;
							STRNSCPY(Order.Name, OrderDefs[l].name);
							fwrite(&Order, sizeof(Order), 1, fs);
							len += sizeof(Order);
						}
					/*
						Circle about Fields
					*/
					for(ext_fld_offset = fld_offset = l = 0, m = 1; l < varcnt; l++)
						if(VarDefs[l].dataID == i && VarDefs[l].iterID == j) {
							memset(&Field, 0, sizeof(Field));
							VarDefs[l].ID = Field.ID = m;
							STRNSCPY(Field.Name, VarDefs[l].name);
							STRNSCPY(Field.Alias, VarDefs[l].alias);
							Field.Type = VarDefs[l].type;
							Field.Rel  = VarDefs[l].rel;
							Field.NodeCount = VarDefs[l].node_cnt;
							if(Field.NodeCount) {
								Field.Offset = ext_fld_offset;
								ext_fld_offset += (GETSTYPE(Field.Type) & 0x80) ?
								 sizeof(void*) : ((GETSTYPE(Field.Type) == S_DEC) ?
								 GETSSIZED(Field.Type) : GETSSIZE(Field.Type));
							}
							else {
								Field.Offset = fld_offset;
								fld_offset += (GETSTYPE(Field.Type) == S_DEC) ?
									GETSSIZED(Field.Type) : GETSSIZE(Field.Type);
							}
							Field.Format = VarDefs[l].format;
							m++;
							fwrite(&Field, sizeof(Field), 1, fs);
							len += sizeof(Field);
							fseek(nodefile, VarDefs[l].node_offset, SEEK_SET);
							for(n = 0; n < Field.NodeCount; n++) {
								memset(&Node, 0, sizeof(NodeType));
								fread(&Node, sizeof(Node), 1, nodefile);
								if(Node.type == OP_IDENT) {
									Node.lnk_val.DataId++;
									Node.lnk_val.IterId = IterDefs[Node.lnk_val.IterId].ID;
									Node.lnk_val.VarId = VarDefs[Node.lnk_val.VarId].ID;
								}
								fwrite(&Node, sizeof(Node), 1, fs);
								len += sizeof(NodeType);
								if(Node.ext_val) {
									fread(tmpbuff, Node.ext_val, 1, nodefile);
									fwrite(tmpbuff, Node.ext_val, 1, fs);
									len += Node.ext_val;
								}
							}
						}
						k++;
				}
		}
		fclose(fs);
		fclose(nodefile);
	}
	else
		printf("Fail alloc for read buffs");
	free(VarDefs);
	free(DataDefs);
	free(IterDefs);
	free(OrderDefs);
	return 0;
}
/*
	Найти ID структуры для Link'а
*/
long GetDataID(char * name, long fl)
{
	int    i, j;
	char * cc = strchr(name, ' ');
	if(cc)
		*cc = 0;
	for(i = 0; i < datacnt; i++)
		if(strcmp(DataDefs[i].Name, name) == 0)
			if(fl) {
				for(j = 0; j < varcnt; j++)
					if(VarDefs[j].dataID == i && VarDefs[j].iterID == 0 && VarDefs[j].rel == -1)
						return i+1;
				yyerror("This SData have no 'key'");
				return 0;
			}
			else {
				FillData(i);
				return i+1;
			}
	yyerror("No linked SData");
	return 0;
}

void FillData(long dd)
{
	long i, j, k, l, m, oldcnt, ext_old = 0;
	NodeType Node;

	oldcnt = varcnt;

	for(k = 0, l = varcnt; k < varcnt; k++) {
		if((VarDefs[k].dataID == dd) && (VarDefs[k].iterID == 0)) {
			memcpy(VarDefs+l, VarDefs+k, sizeof(VARDEFS));
			VarDefs[l].dataID = datacnt;
			VarDefs[l].iterID = 0;
			l++;
		}
	}
	varcnt = l;
	for(i = 0, j = itercnt; i < itercnt; i++) {
		if(IterDefs[i].dataID == dd) {
			memcpy(IterDefs+j, IterDefs+i, sizeof(ITERDEFS));
			IterDefs[j].dataID = datacnt;
			for(k = 0, l = varcnt; k < varcnt; k++) {
				if((VarDefs[k].dataID == dd) && ((VarDefs[k].iterID == i) ||
					(VarDefs[k].iterID == -1))) {
					memcpy(VarDefs+l, VarDefs+k, sizeof(VARDEFS));
					VarDefs[l].dataID = datacnt;
					VarDefs[l].iterID = j;
					l++;
				}
			}
			varcnt = l;
			for(k = 0, l = ordercnt; k < ordercnt; k++) {
				if((OrderDefs[k].dataID == dd) && (OrderDefs[k].iterID == i)) {
					memcpy(OrderDefs+l, OrderDefs+k, sizeof(ORDERDEFS));
					OrderDefs[l].dataID = datacnt;
					OrderDefs[l].iterID = j;
					l++;
				}
			}
			ordercnt = l;
			j++;
		}
	}
	itercnt = j;
	for(l = oldcnt; l < varcnt; l++) {
		if(VarDefs[l].node_cnt) {
			for(m = 0, ext_old = 0; m < VarDefs[l].node_cnt; m++, ext_old += sizeof(Node)) {
				memset(&Node, 0, sizeof(NodeType));
				fseek(nodefile, ext_old + VarDefs[l].node_offset, SEEK_SET);
				fread(&Node, sizeof(Node), 1, nodefile);
				if(Node.type == OP_IDENT) {
					if(Node.lnk_val.DataId == dd) {
						Node.lnk_val.IterId = Node.lnk_val.IterId ? VarDefs[l].iterID : 0;
						Node.lnk_val.DataId = datacnt;
						Node.lnk_val.VarId = GetVarID(VarDefs[Node.lnk_val.VarId].name, datacnt, Node.lnk_val.IterId);
					}
				}
				if(Node.ext_val) {
					fread(tmpbuff, Node.ext_val, 1, nodefile);
					ext_old += Node.ext_val;
				}
				fseek(nodefile, 0, SEEK_END);
				fwrite(&Node, sizeof(Node), 1, nodefile);
				if(Node.ext_val)
					fwrite(tmpbuff, Node.ext_val, 1, nodefile);
			}
			VarDefs[l].node_offset = node_offset;
			node_offset += ext_old;
		}
	}
}

long GetIterID(long dt, char * name)
{
	int i;
	char * cc;
	if(cc = strchr(name, 32))
		*cc = 0;
	for(i = 1; i < itercnt; i++)
		if(IterDefs[i].dataID == dt)
			if(!strcmp(IterDefs[i].name, name))
				return i;
	yyerror("Could not add iteration in this SData");
	return 0;
}
/*
	Проверки имен переменных
*/
void DataRedef(char* str, long id)
{
	for(int i = 0; i < datacnt; i++)
		if(DataDefs[i].TypeId == id && !strcmp(DataDefs[i].Name, str))
			yyerror("Data redefinition");
}

void IterRedef(char* str, long dd)
{
	for(int i = 0; i < itercnt; i++)
		if(IterDefs[i].dataID == dd && !strcmp(IterDefs[i].name, str))
			yyerror("Iterator redefinition");
}

void VarsRedef(char* str, long dd, long ii)
{
	for(int i = 0; i < varcnt; i++)
		if(!strcmp(VarDefs[i].name, str) && VarDefs[i].dataID == dd && VarDefs[i].iterID == ii)
			yyerror("Variable redefinition");
}

int GetVarID(char* str, long dd, long ii)
{
	for(int i = 0; i < varcnt; i++)
		if(!strcmp(VarDefs[i].name, str) && VarDefs[i].dataID == dd &&
			(VarDefs[i].iterID == ii || VarDefs[i].iterID == 0))
			return i;
	return -1;
}

void WriteClassImplementation(FILE * fs, long classIdx, char * pClassName, long type)
{
	long j, k;
	if(DataDefs[classIdx].TypeId == 1 && type == 1) {
		fprintf(fs, "//\n// Implementation of %s_%s\n//\n", PREFIX, pClassName);
		/*
			constructor
		*/
		fprintf(fs, "%s_%s::%s_%s(long resId, PPALDD ** ppA) :\n", PREFIX, pClassName, PREFIX, pClassName);
		fprintf(fs, "\t%s(0, resId, ppA)\n{\n", PREFIX);
		fprintf(fs, "\tif(Valid) {\n");
		for(j = 0, k = 0; j < itercnt; j++)
			if((IterDefs[j].dataID == classIdx) || j == 0) {
				if(IterDefs[j].type == T_ITER)
					fprintf(fs, "\t\tAssignIterData(%d, &IT_%s_data, sizeof(IT_%s_data));\n", k, IterDefs[j].name, IterDefs[j].name);
				else if(IterDefs[j].type == 0)
					fprintf(fs, "\t\tAssignHeadData(&Head_data, sizeof(Head_data));\n");
				k++;
			}
		fprintf(fs, "\t}\n");
		fprintf(fs, "}\n\n");
		/*
			destructor
		*/
		fprintf(fs, "%s_%s::~%s_%s()\n{\n", PREFIX, pClassName, PREFIX, pClassName);
		fprintf(fs, "\tDestroy();\n");
		fprintf(fs, "}\n\n");
		/*
			virtual funcs
		*/
		fprintf(fs, "int %s_%s::InitData(PPFilt * pFilt, long rsrv)\n{\n\treturn PPALDD::InitData(pFilt, rsrv);\n}\n\n", PREFIX, pClassName);
		if(k > 1) {
			fprintf(fs, "int %s_%s::InitIteration(PPIterID iterId, int sortId, long rsrv)\n{\n", PREFIX, pClassName);
			fprintf(fs, "\tif(iterId == DEFAULT_ITER)\n");
			fprintf(fs, "\t\titerId = GetIterID();\n");
			fprintf(fs, "\treturn -1;\n}\n\n");
			fprintf(fs, "int %s_%s::NextIteration(PPIterID iterId, long rsrv)\n{\n", PREFIX, pClassName);
			fprintf(fs, "\tif(iterId == DEFAULT_ITER)\n");
			fprintf(fs, "\t\titerId = GetIterID();\n");
			fprintf(fs, "\treturn PPALDD::NextIteration(iterId, rsrv);\n}\n\n");
		}
		if(DataDefs[classIdx].DeclareFlags & DATADECLF_DESTROY)
			fprintf(fs, "int %s_%s::Destroy()\n{\n\treturn -1;\n}\n", PREFIX, pClassName);
	}
	else if(DataDefs[classIdx].TypeId == 2 && type == 2) {
		fprintf(fs, "//\n// Implementation of %s\n//\n", pClassName);
		/*
			constructor
		*/
		fprintf(fs, "%s::%s() {\n", pClassName, pClassName);
		fprintf(fs, "\tInitFilt();\n");
		fprintf(fs, "\tLoad();\n");
		fprintf(fs, "}\n\n");
		/*
			destructor
		*/
		fprintf(fs, "%s::~%s()\n{\n}\n\n", pClassName, pClassName);
		/*
			ather funcs
		*/
		fprintf(fs, "int %s::InitFilt(char * name)\n{\n\tpHead = &Head_data;\n", pClassName);
		fprintf(fs, "\tmemset(pHead, 0, sizeof(Head_data));\n");
		fprintf(fs, "\t_InitFilt(\"%s\", Name));\n}\n\n", pClassName);

		fprintf(fs, "int %s::Load(char * name)\n{\n", pClassName);
		fprintf(fs, "\tint ok = _Load(name);\n");
		fprintf(fs, "\t\\\\Here's the right place for your code.\n");
		fprintf(fs, "\treturn ok;\n}\n\n");

		fprintf(fs, "int %s::Save(char * name)\n{\n", pClassName);
		fprintf(fs, "\tint ok = 1;\n");
		fprintf(fs, "\t\\\\Here's' the right place for your code.\n");
		fprintf(fs, "\t\\\\Never forget that _Save return value.\n");
		fprintf(fs, "\t_Save();\n");
		fprintf(fs, "\treturn ok;\n}\n\n");
	}
}

void WriteCppFile(FILE * fs, long type)
{
	long   i;
	FILE * gf = 0;
	for(i = 0; i < datacnt; i++)
		if(!DataDefs[i].HasParent)
			WriteClassImplementation(fs, i, DataDefs[i].Name, type);
	/*
		static CreateInstance(resFile, resID)
	*/
	gf = fopen("ldstat.cpp", "w+");
	fprintf(gf,"//\n// This file was compiled from %s by %sC.EXE\n", in_file_name, PREFIX);
	fprintf(gf,"// !!! DON'T MODIFY IT !!!\n//\n");
	// @v5.4.9 fprintf(gf, "#ifdef __BORLANDC__\n");
	// @v5.4.9 fprintf(gf, "#\tpragma hdrfile \"PPDEF.SYM\"\n");
	// @v5.4.9 fprintf(gf, "#endif\n");
	fprintf(gf, "#include <pp.h>\n");
	fprintf(gf, "#include <ppdlgs.h>\n");
	fprintf(gf, "#pragma hdrstop\n");
	// @v5.4.9 fprintf(gf, "#include <%s>\n\n", h_file_name);
	fprintf(gf, "%s * %s::CreateInstance(const char * /*pResFile*/, long parentID, long resID, PPALDD ** ppa)\n{\n", PREFIX, PREFIX);
	fprintf(gf, "\tPPALDD * p_rval = 0;\n");
	fprintf(gf, "\tif(!resID)\n\t\tresID = parentID;\n\n");
	fprintf(gf, "\tswitch(parentID) {\n");
	for(i = 0; i < datacnt; i++)
		if(!DataDefs[i].HasParent && DataDefs[i].TypeId == 1) {
			if(DataDefs[i].DeclareFlags & DATADECLF_DOSSTUB)
				fprintf(gf, "#ifdef __WIN32__\n");
			fprintf(gf, "\t\tcase %ld: ", i+1);
			fprintf(gf, "p_rval = new %s_%s(resID, ppa); break;\n", PREFIX, DataDefs[DataDefs[i].ParentId-1].Name);
			if(DataDefs[i].DeclareFlags & DATADECLF_DOSSTUB)
				fprintf(gf, "#endif\n");
		}
	fprintf(gf, "\t}\n");
	fprintf(gf, "\tif(p_rval && !p_rval->IsValid()) {\n");
	fprintf(gf, "\t\tdelete p_rval;\n");
	fprintf(gf, "\t\tp_rval = 0;\n");
	fprintf(gf, "\t}\n");
	fprintf(gf, "\treturn p_rval;\n}\n");
	fclose(gf);
}

void WriteHFile(FILE * fs, long type)
{
	long i, j;
	for(i = 0; i < datacnt; i++)
		if(DataDefs[i].TypeId == 1 && type == 1) {
			if(!DataDefs[i].HasParent) {
				if(DataDefs[i].DeclareFlags & DATADECLF_DOSSTUB) {
					fprintf(fs, "#ifdef __WIN32__\n");
				}
				for(j = 1; j < itercnt; j++)
					if(IterDefs[j].dataID == i)
						break;
				fprintf(fs, "class %s_%s : public %s {\npublic:\n", PREFIX, DataDefs[i].Name, PREFIX);
				fprintf(fs, "\t%s_%s(long resId = %ld, PPALDD ** ppA = 0);\n", PREFIX, DataDefs[i].Name, i+1);
				fprintf(fs, "\t~%s_%s();\n", PREFIX, DataDefs[i].Name);
				fprintf(fs, "\tvirtual int InitData(PPFilt *, long rsrv = 0);\n");
				if(j != itercnt) {
					fprintf(fs, "\tvirtual int InitIteration(PPIterID, int sortId, long rsrv = 0);\n");
					fprintf(fs, "\tvirtual int NextIteration(PPIterID, long rsrv = 0);\n");
				}
				if(DataDefs[i].DeclareFlags & DATADECLF_DESTROY)
					fprintf(fs, "\tvirtual int Destroy();\n");
				WriteFields(fs, i);
				fprintf(fs, "};\n\n");
				if(DataDefs[i].DeclareFlags & DATADECLF_DOSSTUB)
					fprintf(fs, "#endif // __WIN32__\n");
			}
		}
		else if(DataDefs[i].TypeId == 2 && type == 2) {
			fprintf(fs, "class %s : public %s {\npublic:\n", DataDefs[i].Name, PPFILTER);
			fprintf(fs, "\t%s();\n", DataDefs[i].Name);
			fprintf(fs, "\t~%s();\n", DataDefs[i].Name);
			fprintf(fs, "\tint Load(char * Name = 0);\n");
			fprintf(fs, "\tint Save(char * Name = 0);\n");
			fprintf(fs, "\tint InitFilt(char * Name = 0);\n");
			WriteFields(fs, i);
			fprintf(fs, "};\n\n");
		}
}

void WriteFields(FILE * fs, long dID)
{
	long i, j, k;
	fprintf(fs, "\n\tstruct Head {\n");
	WriteIterFields(fs, dID, 0);
	fprintf(fs, "\t} Head_data;\n");

	for(i = 1; i < itercnt; i++) {
		if(IterDefs[i].dataID == dID) {
			fprintf(fs, "\tstruct IT_%s {\n", IterDefs[i].name);
			for(j = 0; j < ordercnt; j++)
				if(OrderDefs[j].iterID == i)
					break;
			if(j < ordercnt) {
				fprintf(fs, "\t\tenum {\n");
				for(j = k = 0; j < ordercnt; j++)
					if(OrderDefs[j].iterID == i) {
						if(!k)
							fprintf(fs, "\t\t\tOrd%s = 1,\n", OrderDefs[j].name);
						else
							fprintf(fs, "\t\t\tOrd%s,\n", OrderDefs[j].name);
						OrderDefs[j].ID = ++k;
					}
				fprintf(fs, "\t\t};\n");
			}
			WriteIterFields(fs, dID, i);
			fprintf(fs, "\t} IT_%s_data;\n", IterDefs[i].name);
		}
	}
}

void WriteIterFields(FILE * fs, long dID, long iID)
{
	long i;
	for(i = 0; i < varcnt; i++) {
		VARDEFS j = VarDefs[i];
		if((!j.node_cnt) && (j.dataID == dID) && (j.iterID == iID)) {
			switch(GETARYSTYPE(j.type)) {
				case S_DEC:
					fprintf(fs, "\t\tchar %s[%d];\n", j.name, GETSSIZED(j.type));
					break;
				case S_ZSTRING:
					fprintf(fs, "\t\tchar %s[%d];\n", j.name, GETSSIZE(j.type));
					break;
				case S_AUTOINC:
				case S_INT:
					fprintf(fs, "\t\t");
					if(GETISARY(j.type))
						fprintf(fs, "TSArray <");
					if(GETSSIZE(j.type) == 2)
						fprintf(fs, "int16");
					else
						fprintf(fs, "int32");
					if(GETISARY(j.type))
						fprintf(fs, "> *");
					fprintf(fs, " %s;\n", j.name);
					break;
				case S_FLOAT:
					fprintf(fs, "\t\t");
					if(GETISARY(j.type))
						fprintf(fs, "TSArray<");
					if(GETSSIZE(j.type) == 4)
						fprintf(fs, "float");
					else
						fprintf(fs, "double");
					if(GETISARY(j.type))
						fprintf(fs, "> *");
					fprintf(fs, " %s;\n", j.name);
					break;
				case S_TIME:
					fprintf(fs, "\t\tLTIME %s;\n", j.name);
					break;
				case S_DATE:
					fprintf(fs, "\t\tLDATE %s;\n", j.name);
					break;
				case S_DATETIME:
					fprintf(fs, "\t\tLDATETIME %s;\n", j.name);
					break;
				case S_CHAR:
					fprintf(fs, "\t\tbyte %s;\n", j.name);
					break;
			}
		}
	}
}

static NodeType GetNodeType1(long opType, NodeType & arg1)
{
	NodeType arg_list[1];
	arg_list[0] = arg1;
	return GetNodeType(opType, arg_list);
}

static NodeType GetNodeType2(long opType, NodeType & arg1, NodeType & arg2)
{
	NodeType arg_list[2];
	arg_list[0] = arg1;
	arg_list[1] = arg2;
	return GetNodeType(opType, arg_list);
}

static NodeType GetNodeType3(long opType, NodeType & arg1, NodeType & arg2, NodeType & arg3)
{
	NodeType arg_list[3];
	arg_list[0] = arg1;
	arg_list[1] = arg2;
	arg_list[2] = arg3;
	return GetNodeType(opType, arg_list);
}

NodeType GetNodeType(long opType, NodeType * args)
{
#define ISARG(t1)        (arg_type[0]==(t1))
#define ISARG2(t1,t2)    (arg_type[0]==(t1)||arg_type[0]==(t2))
#define ISARG3(t1,t2,t3) (arg_type[0]==(t1)||arg_type[0]==(t2)||arg_type[0]==(t3))
#define ISARGPAIR(t1,t2) (arg_type[0]==(t1)&&arg_type[1]==(t2))
#define ISONEOFARGS2(t)  (arg_type[0]==(t)||arg_type[1]==(t))

	NodeType rval;
	int  i;
	long l1, l2;
	long arg_type[MAX_ARGS];
	rval.type = opType;
	rval.ret_val = 0;
	rval.ext_val = 0;
	for(i = 0; i < MAX_ARGS; i++) {
		arg_type[i] = GETSTYPE(args[i].ret_val);
		rval.fnk_val.VarTypes[i] = args[i].ret_val;
	}
	switch(opType) {
		case OP_PLUS:
		case OP_MINUS:
		case OP_MULT:
		case OP_DIV:
		case OP_TO_STR:
		case OP_IF:
		case OP_LEFT:
		case OP_RIGHT:
		case OP_WRAP:
		case OP_FORMATPERIOD:
		case OP_PRINTABLEBARCODE:
		case OP_EQUL:
		case OP_NOTEQUL:
		case OP_LESS:
		case OP_GREAT:
		case OP_LESS_EQ:
		case OP_GREAT_EQ:
		case OP_LOG_OR:
		case OP_LOG_AND:
		case OP_BIT_AND:
		case OP_BIT_OR:
		case OP_BIT_XOR: rval.fnk_val.VarCount = 2; break;
		case OP_ABS_DBL:
		case OP_ABS_INT:
		case OP_UPLUS:
		case OP_UMINUS:
		case OP_TO_DBL:
		case OP_TO_INT:
		case OP_LEN:
		case OP_BIT_NOT: rval.fnk_val.VarCount = 1; break;
		case OP_IF_ELSE:
		case OP_MIDL:    rval.fnk_val.VarCount = 3; break;
		default:
			yyerror("Could not translate operation");
			break;
	}
	switch(opType) {
		case OP_BIT_NOT:
			if(ISARG(S_INT))
				rval.SetTR(opType, args[0].ret_val);
			break;
		case OP_UPLUS:
			if(ISARG3(S_INT, S_FLOAT, S_DEC))
				rval.ret_val = args[0].ret_val;
			break;
		case OP_UMINUS:
			if(ISARG3(S_INT, S_FLOAT, S_DEC)) {
				rval.ret_val = args[0].ret_val;
				if(ISARG(S_INT))
					rval.type = OP_UMINUS_INT;
				else if(ISARG(S_FLOAT))
					rval.type = OP_UMINUS_DBL;
			}
			break;
		case OP_PLUS:
			if(ISARGPAIR(S_ZSTRING, S_ZSTRING)) {
				l1 = GETSSIZE(args[0].ret_val)+GETSSIZE(args[1].ret_val)-1;
				rval.ret_val = MKSTYPE(S_ZSTRING, (l1 < MAX_STR_LEN) ? l1 : MAX_STR_LEN);
				rval.type = OP_PLUS_STR_STR;
				break;
			}
			if(ISONEOFARGS2(S_ZSTRING))
				break;
			if(arg_type[0] == S_DATE && arg_type[1] != S_DATE) {
				rval.type = (arg_type[1] == S_INT) ? OP_PLUS_DAT_INT : OP_PLUS_DAT_DBL;
				rval.ret_val = MKSTYPE(S_DATE, sizeof(LDATE));
				break;
			}
			if(ISONEOFARGS2(S_DATE))
				break;
			if(arg_type[1] == S_DATE && arg_type[0] != S_DATE) {
				rval.type = (arg_type[0] == S_INT) ? OP_PLUS_INT_DAT : OP_PLUS_DBL_DAT;
				rval.ret_val = MKSTYPE(S_DATE, sizeof(LDATE));
				break;
			}
			if(arg_type[0] == S_TIME && arg_type[1] != S_TIME) {
				rval.type = (arg_type[1] == S_INT) ? OP_PLUS_TIM_INT : OP_PLUS_TIM_DBL;
				rval.ret_val = MKSTYPE(S_TIME, sizeof(LTIME));
				break;
			}
			if(arg_type[1] == S_TIME && arg_type[0] != S_TIME) {
				rval.type = (arg_type[0] == S_INT) ? OP_PLUS_INT_TIM : OP_PLUS_DBL_TIM;
				rval.ret_val = MKSTYPE(S_TIME, sizeof(LTIME));
				break;
			}
			if(ISONEOFARGS2(S_TIME))
				break;
			switch(arg_type[0]) {
				case S_DEC:
					if(arg_type[1] == S_DEC) {
						l1 = GETSSIZED(arg_type[0]);
						l2 = GETSSIZED(arg_type[1]);
						l1 = (l1 > l2) ? l1 : l2;
						l2 = GETSPRECD(arg_type[0]);
						l2 = (l2 > GETSPRECD(arg_type[1])) ? l2 : GETSPRECD(arg_type[1]);
						rval.ret_val = MKSTYPED(S_DEC, l1, l2);
					}
					else
						rval.ret_val = args[0].ret_val;
					rval.type = (arg_type[1] == S_INT) ? OP_PLUS_DBL_INT : OP_PLUS_DBL_DBL;
					break;
				case S_FLOAT:
					rval.type = (arg_type[1] == S_INT) ? OP_PLUS_DBL_INT : OP_PLUS_DBL_DBL;
					rval.ret_val = (arg_type[1] == S_DEC) ? args[1].ret_val : MKSTYPE(S_FLOAT, 8);
					break;
				case S_INT:
					if(arg_type[1] != S_INT)
						rval.SetTR(OP_PLUS_INT_DBL, args[1].ret_val);
					else
						rval.SetTR(OP_PLUS_INT_INT, MKSTYPE(S_INT, 4));
					break;
			}
			break;
		case OP_MINUS:
			if(ISONEOFARGS2(S_ZSTRING))
				break;
			if(arg_type[0] == S_DATE) {
				if(arg_type[1] != S_TIME) {
					if(arg_type[1] == S_DATE)
						rval.SetTR(OP_MINUS_DAT_DAT, MKSTYPE(S_INT, 4));
					else if(arg_type[1] == S_INT)
						rval.SetTR(OP_MINUS_DAT_INT, args[0].ret_val);
					else
						rval.SetTR(OP_MINUS_DAT_DBL, args[0].ret_val);
				}
				break;
			}
			if(ISONEOFARGS2(S_DATE))
				break;
			if(arg_type[0] == S_TIME) {
				if(arg_type[1] != S_DATE) {
					if(arg_type[1] == S_TIME)
						rval.SetTR(OP_MINUS_TIM_TIM, MKSTYPE(S_INT, 4));
					else if(arg_type[1] == S_INT)
						rval.SetTR(OP_MINUS_TIM_INT, args[0].ret_val);
					else
						rval.SetTR(OP_MINUS_TIM_DBL, args[0].ret_val);
				}
				break;
			}
			if(ISONEOFARGS2(S_TIME))
				break;
			switch(arg_type[0]) {
				case S_DEC:
					if(arg_type[1] == S_DEC) {
						l1 = GETSSIZED(arg_type[0]);
						l2 = GETSSIZED(arg_type[1]);
						l1 = (l1 > l2) ? l1 : l2;
						l2 = GETSPRECD(arg_type[0]);
						l2 = (l2 > GETSPRECD(arg_type[1])) ? l2 : GETSPRECD(arg_type[1]);
						rval.ret_val = MKSTYPED(S_DEC, l1, l2);
					}
					else
						rval.ret_val = args[0].ret_val;
					rval.type = (arg_type[1] == S_INT) ? OP_MINUS_DBL_INT : OP_MINUS_DBL_DBL;
					break;
				case S_FLOAT:
					rval.type = (arg_type[1] == S_INT) ? OP_MINUS_DBL_INT : OP_MINUS_DBL_DBL;
					rval.ret_val = (arg_type[1] == S_DEC) ? args[1].ret_val : MKSTYPE(S_FLOAT, 8);
					break;
				case S_INT:
					if(arg_type[1] != S_INT)
						rval.SetTR(OP_MINUS_INT_DBL, args[1].ret_val);
					else
						rval.SetTR(OP_MINUS_INT_INT, MKSTYPE(S_INT, 4));
					break;
			}
			break;
		case OP_MULT:
			if(ISARGPAIR(S_ZSTRING, S_ZSTRING)) {
				l1 = GETSSIZE(args[0].ret_val) + GETSSIZE(args[1].ret_val);
				rval.ret_val = MKSTYPE(S_ZSTRING, (l1 < MAX_STR_LEN) ? l1 : MAX_STR_LEN);
				rval.type = OP_MULT_STR_STR;
				break;
			}
			if(ISONEOFARGS2(S_ZSTRING))
				break;
			if(ISONEOFARGS2(S_TIME))
				break;
			if(ISONEOFARGS2(S_DATE))
				break;
			switch(arg_type[0]) {
				case S_DEC:
					if(arg_type[1] == S_DEC) {
						l1 = GETSSIZED(arg_type[0]);
						l2 = GETSSIZED(arg_type[1]);
						l1 = (l1 > l2) ? l1 : l2;
						l2 = GETSPRECD(arg_type[0]);
						l2 = (l2 > GETSPRECD(arg_type[1])) ? l2 : GETSPRECD(arg_type[1]);
						rval.ret_val = MKSTYPED(S_DEC, l1, l2);
					}
					else
						rval.ret_val = args[0].ret_val;
					rval.type = (arg_type[1] == S_INT) ? OP_MULT_DBL_INT : OP_MULT_DBL_DBL;
					break;
				case S_FLOAT:
					rval.type = (arg_type[1] == S_INT) ? OP_MULT_DBL_INT : OP_MULT_DBL_DBL;
					rval.ret_val = (arg_type[1] == S_DEC) ? args[1].ret_val : MKSTYPE(S_FLOAT, 8);
					break;
				case S_INT:
					if(arg_type[1] == S_INT)
						rval.SetTR(OP_MULT_INT_INT, MKSTYPE(S_INT, 4));
					else
						rval.SetTR(OP_MULT_INT_DBL, args[1].ret_val);
					break;
			}
			break;
		case OP_DIV:
			if(ISONEOFARGS2(S_ZSTRING))
				break;
			if(ISONEOFARGS2(S_TIME))
				break;
			if(ISONEOFARGS2(S_DATE))
				break;
			switch(arg_type[0]) {
				case S_DEC:
					if(arg_type[1] == S_DEC) {
						l1 = GETSSIZED(arg_type[0]);
						l2 = GETSSIZED(arg_type[1]);
						l1 = (l1 > l2) ? l1 : l2;
						l2 = GETSPRECD(arg_type[0]);
						l2 = (l2 > GETSPRECD(arg_type[1])) ? l2 : GETSPRECD(arg_type[1]);
						rval.ret_val = MKSTYPED(S_DEC, l1, l2);
					}
					else
						rval.ret_val = args[0].ret_val;
					rval.type = (arg_type[1] == S_INT) ? OP_DIV_DBL_INT : OP_DIV_DBL_DBL;
					break;
				case S_FLOAT:
					rval.type = (arg_type[1] == S_INT) ? OP_DIV_DBL_INT : OP_DIV_DBL_DBL;
					rval.ret_val = (arg_type[1] == S_DEC) ? args[1].ret_val : MKSTYPE(S_FLOAT, 8);
					break;
				case S_INT:
					if(arg_type[1] != S_INT)
						rval.SetTR(OP_DIV_INT_DBL, args[1].ret_val);
					else
						rval.SetTR(OP_DIV_INT_INT, MKSTYPE(S_INT, 4));
					break;
			}
			break;
		case OP_TO_DBL:
			if(ISARG(S_INT))
				rval.SetTR(OP_TO_DBL_INT, MKSTYPE(S_FLOAT, 8));
			else if(ISARG(S_ZSTRING))
				rval.SetTR(OP_TO_DBL_STR, MKSTYPE(S_FLOAT, 8));
			break;
		case OP_TO_INT:
			if(ISARG(S_FLOAT))
				rval.SetTR(OP_TO_INT_DBL, MKSTYPE(S_INT, 4));
			else if(ISARG(S_ZSTRING))
				rval.SetTR(OP_TO_INT_STR, MKSTYPE(S_INT, 4));
			break;
		case OP_TO_STR:
			switch(arg_type[0]) {
				case S_FLOAT:
				case S_DEC:  rval.SetTR(OP_TO_STR_DBL, MKSTYPE(S_ZSTRING, 16)); break;
				case S_INT:  rval.SetTR(OP_TO_STR_INT, MKSTYPE(S_ZSTRING, 16)); break;
				case S_DATE: rval.SetTR(OP_TO_STR_DAT, MKSTYPE(S_ZSTRING, 16)); break;
				case S_TIME: rval.SetTR(OP_TO_STR_TIM, MKSTYPE(S_ZSTRING, 16)); break;
			}
			break;
		case OP_IF:
			if(arg_type[0] == S_INT) {
				switch(arg_type[1]) {
					case S_FLOAT:
					case S_DEC:     rval.type = OP_IF_DBL; break;
					case S_INT:     rval.type = OP_IF_INT; break;
					case S_DATE:    rval.type = OP_IF_DAT; break;
					case S_TIME:    rval.type = OP_IF_TIM; break;
					case S_ZSTRING: rval.type = OP_IF_STR; break;
				}
				rval.ret_val = args[1].ret_val;
			}
			break;
		case OP_IF_ELSE:
			if(arg_type[0] == S_INT && arg_type[1] == arg_type[2]) {
				switch(arg_type[1]) {
					case S_FLOAT:
					case S_DEC:  rval.type = OP_IF_ELSE_DBL; break;
					case S_INT:  rval.type = OP_IF_ELSE_INT; break;
					case S_DATE: rval.type = OP_IF_ELSE_DAT; break;
					case S_TIME: rval.type = OP_IF_ELSE_TIM; break;
					case S_ZSTRING:
						rval.type = OP_IF_ELSE_STR;
						l1 = GETSSIZE(args[1].ret_val);
						l2 = GETSSIZE(args[2].ret_val);
						args[1].ret_val = MKSTYPE(S_ZSTRING, (l1 > l2) ? l1 : l2);
						break;
				}
				rval.ret_val = args[1].ret_val;
			}
			break;
		case OP_LEN:
			if(ISARG(S_ZSTRING))
				rval.SetTR(OP_LEN, MKSTYPE(S_INT, 4));
			break;
		case OP_LEFT:
		case OP_MIDL:
		case OP_RIGHT:
		case OP_WRAP:             rval.SetTR(opType, args[0].ret_val); break;
		case OP_FORMATPERIOD:     rval.SetTR(opType, MKSTYPE(S_ZSTRING, 30)); break;
		case OP_PRINTABLEBARCODE: rval.SetTR(opType, MKSTYPE(S_ZSTRING, 20)); break;
		case OP_ABS_DBL:          rval.SetTR(opType, MKSTYPE(S_FLOAT, 8)); break;
		case OP_ABS_INT:          rval.SetTR(opType, MKSTYPE(S_INT, 4)); break;
		case OP_EQUL:
			if(ISARGPAIR(S_ZSTRING, S_ZSTRING))
				rval.type = OP_EQUL_STR_STR;
			else if(ISONEOFARGS2(S_ZSTRING))
				break;
			if(ISARGPAIR(S_DATE, S_DATE))
				rval.type = OP_EQUL_DAT_DAT;
			else if(ISARGPAIR(S_DATE, S_INT))
				rval.type = OP_EQUL_DAT_INT;
			else if(ISARGPAIR(S_INT, S_DATE))
				rval.type = OP_EQUL_INT_DAT;
			else if(ISONEOFARGS2(S_DATE))
				break;
			if(ISARGPAIR(S_TIME, S_TIME))
				rval.type = OP_EQUL_TIM_TIM;
			else if(ISONEOFARGS2(S_TIME))
				break;
			if(ISONEOFARGS2(S_DEC))
				break;
			if(ISARG(S_FLOAT))
				rval.type = (arg_type[1] == S_INT) ? OP_EQUL_DBL_INT : OP_EQUL_DBL_DBL;
			else if(ISARG(S_INT))
				rval.type = (arg_type[1] != S_INT) ? OP_EQUL_INT_DBL : OP_EQUL_INT_INT;
			rval.ret_val = MKSTYPE(S_INT, 4);
			break;
		case OP_NOTEQUL:
			if(ISARGPAIR(S_ZSTRING, S_ZSTRING))
				rval.type = OP_NOTEQUL_STR_STR;
			else if(ISONEOFARGS2(S_ZSTRING))
				break;
			if(ISARGPAIR(S_DATE, S_DATE))
				rval.type = OP_NOTEQUL_DAT_DAT;
			else if(ISARGPAIR(S_DATE, S_INT))
				rval.type = OP_NOTEQUL_DAT_INT;
			else if(ISARGPAIR(S_INT, S_DATE))
				rval.type = OP_NOTEQUL_INT_DAT;
			else if(ISONEOFARGS2(S_DATE))
				break;
			if(ISARGPAIR(S_TIME, S_TIME))
				rval.type = OP_NOTEQUL_TIM_TIM;
			else if(ISONEOFARGS2(S_TIME))
				break;
			if(ISONEOFARGS2(S_DEC))
				break;
			switch(arg_type[0]) {
				case S_FLOAT:
					rval.type = (arg_type[1] == S_INT) ? OP_NOTEQUL_DBL_INT : OP_NOTEQUL_DBL_DBL;
					break;
				case S_INT:
					rval.type = (arg_type[1] != S_INT) ? OP_NOTEQUL_INT_DBL : OP_NOTEQUL_INT_INT;
					break;
			}
			rval.ret_val = MKSTYPE(S_INT, 4);
			break;
		case OP_LESS:
			if(ISARGPAIR(S_ZSTRING, S_ZSTRING))
				rval.type = OP_LESS_STR_STR;
			else if(ISONEOFARGS2(S_ZSTRING))
				break;
			if(ISARGPAIR(S_DATE, S_DATE))
				rval.type = OP_LESS_DAT_DAT;
			else if(ISONEOFARGS2(S_DATE))
				break;
			if(ISARGPAIR(S_TIME, S_TIME))
				rval.type = OP_LESS_TIM_TIM;
			else if(ISONEOFARGS2(S_TIME))
				break;
			if(ISONEOFARGS2(S_DEC))
				break;
			switch(arg_type[0]) {
				case S_FLOAT:
					rval.type = (arg_type[1] == S_INT) ? OP_LESS_DBL_INT : OP_LESS_DBL_DBL;
					break;
				case S_INT:
					rval.type = (arg_type[1] != S_INT) ? OP_LESS_INT_DBL : OP_LESS_INT_INT;
					break;
			}
			rval.ret_val = MKSTYPE(S_INT, 4);
			break;
		case OP_LESS_EQ:
			if(ISARGPAIR(S_ZSTRING, S_ZSTRING))
				rval.type = OP_LESSEQ_STR_STR;
			else if(ISONEOFARGS2(S_ZSTRING))
				break;
			if(ISARGPAIR(S_DATE, S_DATE))
				rval.type = OP_LESSEQ_DAT_DAT;
			else if(ISONEOFARGS2(S_DATE))
				break;
			if(ISARGPAIR(S_TIME, S_TIME))
				rval.type = OP_LESSEQ_TIM_TIM;
			else if(ISONEOFARGS2(S_TIME))
				break;
			if(ISONEOFARGS2(S_DEC))
				break;
			switch(arg_type[0]) {
				case S_FLOAT:
					rval.type = (arg_type[1] == S_INT) ? OP_LESSEQ_DBL_INT : OP_LESSEQ_DBL_DBL;
					break;
				case S_INT:
					rval.type = (arg_type[1] != S_INT) ? OP_LESSEQ_INT_DBL : OP_LESSEQ_INT_INT;
					break;
			}
			rval.ret_val = MKSTYPE(S_INT, 4);
			break;
		case OP_GREAT:
			if(ISARGPAIR(S_ZSTRING, S_ZSTRING))
				rval.type = OP_GREAT_STR_STR;
			else if(ISONEOFARGS2(S_ZSTRING))
				break;
			if(ISARGPAIR(S_DATE, S_DATE))
				rval.type = OP_GREAT_DAT_DAT;
			else if(ISONEOFARGS2(S_DATE))
				break;
			if(ISARGPAIR(S_TIME, S_TIME))
				rval.type = OP_GREAT_TIM_TIM;
			else if(ISONEOFARGS2(S_TIME))
				break;
			if(ISONEOFARGS2(S_DEC))
				break;
			switch(arg_type[0]) {
				case S_FLOAT:
					rval.type = (arg_type[1] == S_INT) ? OP_GREAT_DBL_INT : OP_GREAT_DBL_DBL;
					break;
				case S_INT:
					rval.type = (arg_type[1] != S_INT) ? OP_GREAT_INT_DBL : OP_GREAT_INT_INT;
					break;
			}
			rval.ret_val = MKSTYPE(S_INT, 4);
			break;
		case OP_GREAT_EQ:
			if(ISARGPAIR(S_ZSTRING, S_ZSTRING))
				rval.type = OP_GREATEQ_STR_STR;
			else if(arg_type[0] == S_ZSTRING || arg_type[1] == S_ZSTRING)
				break;
			if(ISARGPAIR(S_DATE, S_DATE))
				rval.type = OP_GREATEQ_DAT_DAT;
			else if(arg_type[0] == S_DATE || arg_type[1] == S_DATE)
				break;
			if(ISARGPAIR(S_TIME, S_TIME))
				rval.type = OP_GREATEQ_TIM_TIM;
			else if(arg_type[1] == S_TIME || arg_type[0] == S_TIME)
				break;
			if(arg_type[0] == S_DEC || arg_type[1] == S_DEC)
				break;
			switch(arg_type[0]) {
				case S_FLOAT:
					rval.type = (arg_type[1] == S_INT) ? OP_GREATEQ_DBL_INT : OP_GREATEQ_DBL_DBL;
					break;
				case S_INT:
					rval.type = (arg_type[1] != S_INT) ? OP_GREATEQ_INT_DBL : OP_GREATEQ_INT_INT;
					break;
			}
			rval.ret_val = MKSTYPE(S_INT, 4);
			break;
		case OP_LOG_OR:
		case OP_LOG_AND:
		case OP_BIT_AND:
		case OP_BIT_OR:
		case OP_BIT_XOR:
			if(arg_type[0] == S_INT && arg_type[1] == S_INT)
				rval.SetTR(opType, MKSTYPE(S_INT, 4));
			break;
	}
	if(!rval.ret_val)
		yyerror("Could not translate operation1");
	return rval;
}

FormatSpec SetFormat(char * fmt, long tp)
{
	int    oset = 0;
	char * curpos = fmt;
	FormatSpec fs;

	memset(&fs, 0, sizeof(FormatSpec));
	switch(GETARYSTYPE(tp)) {
		case S_INT:
			if(fmt && (fmt[0] != '>') && (fmt[0] != '|') && (fmt[0] != '<'))
				fs.flags = ALIGN_RIGHT;
			break;
		case S_FLOAT:
			if(fmt && (fmt[0] != '>') && (fmt[0] != '|') && (fmt[0] != '<'))
				fs.flags = ALIGN_RIGHT;
			fs.prec = 2;
			break;
		case S_DEC:
			if(fmt && (fmt[0] != '>') && (fmt[0] != '|') && (fmt[0] != '<'))
				fs.flags = ALIGN_RIGHT;
			fs.prec = (short)GETSPRECD(tp);
			break;
		case S_ZSTRING:
			fs.len = (short)GETSSIZE(tp);
		case S_DATE:
		case S_TIME:
			if(fmt && (fmt[0] != '>') && (fmt[0] != '|') && (fmt[0] != '<'))
				fs.flags = ALIGN_LEFT;
			break;
	}
	if(fmt && fmt[0]) {
		if(fmt[0] == '>') {
			fs.flags = ALIGN_RIGHT;
			curpos++;
		}
		else if(fmt[0] == '<') {
			fs.flags = ALIGN_LEFT;
			curpos++;
		}
		else if(fmt[0] == '|') {
			fs.flags = ALIGN_CENTER;
			curpos++;
		}
		if(*curpos == '*') {
			fs.flags |= COMF_FILLOVF;
			curpos++;
		}
		if(atoi(curpos))
			fs.len = atoi(curpos);
		if(GETSTYPE(tp) == S_FLOAT || GETSTYPE(tp) == S_DEC)
			if(curpos = strchr(curpos, '.'))
				fs.prec = atoi(curpos+1);
		if(curpos = strchr(fmt, '@')) {
			switch(toupper(curpos[1])) {
				case 'U': fs.flags |= STRF_UPPER;    break;
				case 'L': fs.flags |= STRF_LOWER;    break;
				case 'P': fs.flags |= STRF_PASSWORD; break;
			}
		}
		if(curpos = strchr(fmt, '#')) {
			switch(toupper(curpos[1])) {
				case 'A': fs.flags |= DATF_AMERICAN; break;
				case 'G': fs.flags |= DATF_GERMAN;   break;
				case 'B': fs.flags |= DATF_BRITISH;  break;
				case 'I': fs.flags |= DATF_ITALIAN;  break;
				case 'J': fs.flags |= DATF_JAPAN;    break;
				case 'F': fs.flags |= DATF_FRENCH;   break;
				case 'U': fs.flags |= DATF_USA;      break;
				case 'D': fs.flags |= DATF_DMY;      break;
				case 'W': fs.flags |= DATF_ANSI;     break;
				case 'M': fs.flags |= DATF_MDY;      break;
				case 'Y': fs.flags |= DATF_YMD;      break;
			}
			if(toupper(curpos[2]) == 'C') {
				fs.flags |= DATF_CENTURY;
				if(!fs.len)
					fs.len = 10;
			}
			else if(!fs.len)
				fs.len = 8;
		}
		if(curpos = strchr(fmt, '$')) {
			for(curpos++; *curpos; curpos++)
				switch(toupper(*curpos)) {
					case 'C': fs.flags |= NMBF_DELCOMMA;  break;
					case 'A': fs.flags |= NMBF_DELAPOSTR; break;
					case 'S': fs.flags |= NMBF_DELSPACE;  break;
					case 'Z': fs.flags |= NMBF_NOZERO;    break;
					case 'F': fs.flags |= NMBF_FORCEPOS;  break;
					case 'N': fs.flags |= NMBF_NOTRAILZ;  break;
				}
		}
		if((curpos = strchr(fmt, '&')) != 0) {
			switch(curpos[1]) {
				case 'F':
				case 'f':
					if(!fs.len)
						fs.len = 11;
					fs.flags |= TIMF_HMS | TIMF_MSEC;
					break;
				case 'N':
				case 'n':
					if(!fs.len)
						fs.len = 8;
					fs.flags |= TIMF_HMS;
					break;
				case 'H':
				case 'h':
					if(!fs.len)
						fs.len = 5;
					fs.flags |= TIMF_HM;
					break;
				case 'L':
					if(!fs.len)
						fs.len = 5;
					fs.flags |= TIMF_MS | TIMF_MSEC;
					break;
				case 'l':
					if(!fs.len)
						fs.len = 5;
					fs.flags |= TIMF_MS;
					break;
				case 'S':
					if(!fs.len)
						fs.len = 5;
					fs.flags |= TIMF_S | TIMF_MSEC;
					break;
				case 's':
					if(!fs.len)
						fs.len = 2;
					fs.flags |= TIMF_S;
					break;
			}
			if(toupper(curpos[2]) == 'B')
				fs.flags |= TIMF_BLANK;
		}
	}
	return fs;
}

