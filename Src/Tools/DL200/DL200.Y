/*
	DL200.Y
	Copyright (c) A.Sobolev 2002, 2003, 2009, 2015, 2020
*/
%{
#include <pp.h>
#include "dl200_yy.h"

class DL2_ExprStack : public SStack {
public:
	DL2_ExprStack() : SStack(sizeof(void *), /*3,*/ aryDataOwner|aryPtrContainer)
	{
	}
};

DL2_Storage    G_Storage;
DL2_ExprStack  G_ExprStack;
DL2_GroupStack G_GroupStack;

DL2_Data  * GP_CurData = 0;
DL2_Group * GP_CurGroup = 0;

int PPErrCode = 0; // Dup of Papyrus global var

//
// Dup of Papyrus function
//
void FASTCALL PPSetAddedMsgString(const char * /*pStr*/)
{
	//ZDELETE(P_AddedMsgString);
	//P_AddedMsgString = newStr(pStr);
}

void yyerror(char * str)
{
	if(yyin_count)
		printf("Error %s(%d): %s\n", yyin_struct[yyin_count-1].fname, yyline, str);
	else
		printf("Error (%d): %s\n", yyline, str);
	exit(-1);
}

static void Fatal(const char * pMsg, const char * pAddStr)
{
	char msg_buf[128];
	if(pAddStr)
		sprintf(msg_buf, pMsg, pAddStr);
	else
		STRNSCPY(msg_buf, pMsg);
	yyerror(msg_buf);
}

static DL2_Data * SetupDataHeader(const char * pName, const char * pDescript)
{
	if(!G_Storage.CheckDupSymb(DL2ENT_DATA, pName))
		Fatal("Data name '%s' redifinition", pName);
	GP_CurData = new DL2_Data;
	GP_CurData->Setup(pName, pDescript, 0);
	G_Storage.AddSymb(GP_CurData);
	return GP_CurData;
}

static DL2_Formula * ProcessFunc(uint16 op, int numArgs)
{
	int i;
	DL2_Formula * p_expr = new DL2_Formula;
	DL2_Formula * p_operand[32];
	DL2_CI ciop;
	ciop.InitOp(op, numArgs);
	//
	//for(i = numArgs-1; i >= 0; i--) {
	for(i = 0; i < numArgs; i++) {
		p_operand[i] = (DL2_Formula *)G_ExprStack.pop();
		if(p_operand[i] == 0)
			Fatal("Invalid expression stack", 0);
	}
	for(i = 0; i < numArgs; i++)
		p_expr->PushExpression(p_operand[i]);
	//
	p_expr->PushItem(&ciop);
	for(i = 0; i < numArgs; i++) {
		(p_operand[i])->destroy();
		delete (p_operand[i]);
	}
	G_ExprStack.push(p_expr);
	return (DL2_Formula *)G_ExprStack.peek();
}

#define PUSH_CONST_TO_EXPR(c) \
	DL2_Formula * p_expr = new DL2_Formula;\
	DL2_CI v;                              \
	v.Init(c);                             \
	p_expr->PushItem(&v);                  \
	G_ExprStack.push(p_expr);

%}

%union {
	long   ival;
	double rval;
	char * sval;
	char   name[48];
	DL2_Acc    ciacc;
	DL2_Column dl2col;
	DL2_Row   * dl2row;       // -> G_CurRow
	DL2_Group * dl2group;     // G_GroupStack.peek()
	DL2_Data  * dl2data;      // -> G_CurData
	DL2_Formula * dl2formula; //
}
/*
	Tokens (%token)
*/
%token <name>   T_IDENT
%token <rval>   T_REALVAL
%token <ival>   T_INTVAL
%token <sval>   T_STRING
%token <sval>   T_PSEUDOVAR
%token <ciacc>  T_ACCVAL
%token <ival>   T_COLUMNTYPE
%token T_DATA
%token T_GROUP
%token T_HEADER
%token T_LBRACE
%token T_RBRACE
%token T_SEMICOLON
%token T_ASSIGN

%left '+' '-'
%left '*' '/'

%nonassoc UPLUS UMINUS

%type <dl2row> row_def;
%type <dl2formula> expression;
%type <dl2formula> expression_list;
%type <sval> description;
%type <dl2group> group_def;
%type <dl2group> group_def_header;
%type <dl2data>  data_def;
%type <dl2data>  data_def_header;
%type <dl2col>   column_def;
%type <ival>     columnmaxsize;
/*
	Syntax
*/
%%

dl200_file : entry_list
{
	G_Storage.Close();
	printf("OK: DL200 file compiled successfully\n");
}

entry_list : | entry_list entry;

entry : row_def
{
	if($1) {
		if(!G_Storage.WriteEntry($1))
			Fatal("Unable writing row '%s' to stream", $1->Name);
		$1->destroy();
		ZDELETE($1);
	}
} | group_def
{
	if(GP_CurGroup) {
		if(!G_Storage.WriteEntry(GP_CurGroup))
			Fatal("Unable writing group '%s' to stream", GP_CurGroup->Name);
		GP_CurGroup->destroy();
		ZDELETE(GP_CurGroup);
	}
} | data_def
{
	if(GP_CurData) {
		if(!G_Storage.WriteEntry(GP_CurData))
			Fatal("Unable writing data '%s' to stream", GP_CurData->Name);
		printf("OK: Data '%s' accepted and stored\n", GP_CurData->Name);
		GP_CurData->destroy();
		ZDELETE(GP_CurData);
	}
}

data_def : data_def_header columns_def data_def_body
{
	$$ = $1;
}

data_def_header : T_DATA T_IDENT description
{
	$$ = SetupDataHeader($2, $3);
}

columns_def :
{
	if(GP_CurData) {
		DL2_Column col;
		memset(&col, 0, sizeof(col));
		col.P_Title = "Default";
		col.CiType = DL2CIT_REAL;
		GP_CurData->AddColumn(&col);
	}
} | '<' column_def_list '>'
{
}

column_def_list : column_def
{
	if(GP_CurData) {
		if($1.P_Title == 0) {
			char temp_buf[48];
			sprintf(temp_buf, "Column %u", GP_CurData->GetColumnsCount()+1);
			$1.P_Title = newStr(temp_buf);
		}
		if(GP_CurData->SearchColumnByName($1.P_Title, 0, 0) > 0)
			Fatal("Column with name '%s' too exists in data", $1.P_Title);
		GP_CurData->AddColumn(&$1);
	}
} | column_def_list ',' column_def
{
	if(GP_CurData) {
		if($3.P_Title == 0) {
			char temp_buf[48];
			sprintf(temp_buf, "Column %u", GP_CurData->GetColumnsCount()+1);
			$3.P_Title = newStr(temp_buf);
		}
		if(GP_CurData->SearchColumnByName($3.P_Title, 0, 0) > 0)
			Fatal("Column with name '%s' too exists in data", $3.P_Title);
		GP_CurData->AddColumn(&$3);
	}
}

column_def : T_STRING T_COLUMNTYPE columnmaxsize
{
	DL2_Column col;
	memset(&col, 0, sizeof(col));
	col.P_Title = newStr($1);
	col.CiType = (uint16)$2;
	col.MaxOutSize = (uint16)$3;
	if(col.MaxOutSize != 0 && col.CiType != DL2CIT_STRING)
		Fatal("Maximum size declaration allowed only for STRING type", 0);
	$$ = col;
} | T_COLUMNTYPE columnmaxsize
{
	DL2_Column col;
	memset(&col, 0, sizeof(col));
	col.CiType = (uint16)$1;
	col.MaxOutSize = (uint16)$2;
	if(col.MaxOutSize != 0 && col.CiType != DL2CIT_STRING)
		Fatal("Maximum size declaration allowed only for STRING type", 0);
	$$ = col;
} | T_STRING T_ACCVAL
{
	DL2_Column col;
	memset(&col, 0, sizeof(col));
	col.P_Title = newStr($1);
	col.CiType = DL2CIT_ACC;
	col.CiAc = $2;
	$$ = col;
} | T_ACCVAL
{
	DL2_Column col;
	memset(&col, 0, sizeof(col));
	col.CiType = DL2CIT_ACC;
	col.CiAc = $1;
	$$ = col;
}

columnmaxsize : 
{
	$$ = 0;
} | '(' T_INTVAL ')'
{
	$$ = $2;
}

data_def_body : T_LBRACE group_items_list T_RBRACE
{
}

group_items_list :
{
} | group_items_list expression_list T_SEMICOLON
{
	DL2_Row * p_row = new DL2_Row;
	p_row->Setup(0, 0, 0);
	p_row->P_F = new DL2_Formula(*$2);
	G_ExprStack.freeAll();
	if(GP_CurGroup)
		GP_CurGroup->AddItem(p_row);
	else if(GP_CurData)
		GP_CurData->AddItem(p_row);
	else
		Fatal("Undefined GP_CurGroup or GP_CurData", 0);
} | group_items_list group_def
{
} | group_items_list T_GROUP T_IDENT T_SEMICOLON
{
	if(G_Storage.LookupSymb(DL2ENT_GROUP, $3, 0, 0) <= 0)
		Fatal("Undefined group name '%s'", $3);
	DL2_Group * p_group = new DL2_Group;
	p_group->Setup($3, 0, 1);
	if(GP_CurGroup)
		GP_CurGroup->AddItem(p_group);
	else if(GP_CurData)
		GP_CurData->AddItem(p_group);
	else
		Fatal("Undefined GP_CurGroup or GP_CurData", 0);
}

row_def : T_IDENT description T_ASSIGN expression T_SEMICOLON
{
	$$ = 0;
	if(!G_Storage.CheckDupSymb(DL2ENT_ROW, $1))
		Fatal("Row name '%s' redifinition", $1);
	DL2_Row * p_row = new DL2_Row;
	p_row->Setup($1, $2, 0);
	p_row->P_F = new DL2_Formula(*$4);
	G_ExprStack.freeAll();
	G_Storage.AddSymb(p_row);
	if(GP_CurGroup)
		GP_CurGroup->AddItem(p_row);
	else if(GP_CurData)
		GP_CurData->AddItem(p_row);
	else
		$$ = p_row;
}

group_def : group_def_header data_def_body
{
	$$ = 0;
	DL2_Group * p_group = 0;
	DL2_Group * p_tos = (DL2_Group *)G_GroupStack.pop();
	if(p_tos) {
		p_group = GP_CurGroup;
		GP_CurGroup = p_tos;
	}
	else {
		p_group = GP_CurGroup;
		GP_CurGroup = 0;
	}
	if(GP_CurGroup)
		GP_CurGroup->AddItem(p_group);
	else if(GP_CurData)
		GP_CurData->AddItem(p_group);
	else
		$$ = p_group;
}

group_def_header : T_GROUP T_IDENT description
{
	$$ = 0;
	if(!G_Storage.CheckDupSymb(DL2ENT_GROUP, $2))
		Fatal("Group name '%s' redifinition", $2);
	DL2_Group * p_group = new DL2_Group;
	p_group->Setup($2, $3, 0);
	if(GP_CurGroup) {
		G_GroupStack.push(GP_CurGroup);
		GP_CurGroup = p_group;
	}
	else
		GP_CurGroup = p_group;
	G_Storage.AddSymb(p_group);
	$$ = p_group;
}

description :
{
	$$ = 0;
} | T_STRING
{
	$$ = $1;
}

expression_list : expression
{
	$$ = $1;
} | expression_list ',' expression
{
	if($1) {
		if(GP_CurData) {
			if((uint)$1->GetCount() >= GP_CurData->GetColumnsCount()) {
				Fatal("Too many subexpressions", 0);
			}
		}
		$1->AddExpression($3);
	}
	$$ = $1;
}

expression : T_IDENT
{
	if(G_Storage.LookupSymb(DL2ENT_ROW, $1, 0, 0) <= 0)
		Fatal("Undefined row name '%s'", $1);
	DL2_Row row;
	if(!G_Storage.ReadEntry(DL2ENT_ROW, $1, &row))
		Fatal("Unable reading row '%s'", $1);
	DL2_Formula * p_expr = new DL2_Formula;
	p_expr->PushExpression(row.P_F);
	row.destroy();
	G_ExprStack.push(p_expr);
	$$ = (DL2_Formula *)G_ExprStack.peek();
} | T_REALVAL
{
	PUSH_CONST_TO_EXPR($1);
	$$ = (DL2_Formula *)G_ExprStack.peek();
} | T_INTVAL
{
	PUSH_CONST_TO_EXPR($1);
	$$ = (DL2_Formula *)G_ExprStack.peek();
} | T_STRING
{
	DL2_Formula * p_expr = new DL2_Formula;
	DL2_CI * p_v = DL2_CI::MakeStr($1);
	p_expr->PushItem(p_v);
	DL2_CI::Destroy(p_v);
	G_ExprStack.push(p_expr);
	$$ = (DL2_Formula *)G_ExprStack.peek();
} | T_ACCVAL
{
	PUSH_CONST_TO_EXPR(&$1);
	$$ = (DL2_Formula *)G_ExprStack.peek();
} | '(' expression ')'
{
	$$ = (DL2_Formula *)G_ExprStack.peek();
} | '-' expression %prec UMINUS
{
	$$ = ProcessFunc(DL2CIT_OP_UMINUS, 1);
} | '+' expression %prec UPLUS
{
	$$ = ProcessFunc(DL2CIT_OP_UPLUS, 1);
} | expression '*' expression
{
	$$ = ProcessFunc(DL2CIT_OP_MULT, 2);
} | expression '/' expression
{
	$$ = ProcessFunc(DL2CIT_OP_DIV, 2);
} | expression '+' expression
{
	$$ = ProcessFunc(DL2CIT_OP_PLUS, 2);
} | expression '-' expression
{
	$$ = ProcessFunc(DL2CIT_OP_MINUS, 2);
}

%%
/*
	Program code
*/
//
// Copy of functions from  ACCT.CPP (PPKERNEL)
//
static int delim(long format)
{
	if(format & ACCF_DELDOT)   return '.';
	if(format & ACCF_DELSPACE) return ' ';
	if(format & ACCF_DELHYP)   return '-';
	return 0;
}

char * AccToStr(const Acct * acc, long format, char * buf)
{
	char * b = buf;
	int dlm = delim(format);
	if(acc->ac != 0) {
		if(acc->ac < 10 && acc->ac > 0 && (format & ACCF_PADACC))
			*b++ = '0';
		b += strlen(_itoa(acc->ac, b, 10));
		if(acc->sb) {
			if(dlm)
				*b++ = dlm;
			if(acc->sb < 10 && (format & ACCF_PADSUB))
				*b++ = '0';
			b += strlen(_itoa(acc->sb, b, 10));
		}
		if(acc->ar && !(format & ACCF_BAL)) {
			if(dlm)
				*b++ = dlm;
			b += strlen(_itoa((int)acc->ar, b, 10));
		}
	}
	else
		*b = 0;
	return _commfmt(format, buf);
}
//
//
//
static int TestReadData(const char * pFileName)
{
	FILE * output = fopen("output.", "w");
	if(!G_Storage.Open(pFileName, 1)) {
		printf("Error: Could not create input file %s\n", pFileName);
		return 0;
	}
	DL2_Data data;
	if(!G_Storage.ReadEntry(DL2ENT_DATA, "IncomeStat", &data)) {
		printf("Error: Could not read data\n");
		return 0;
	}
	if(!data.Print(output)) {
		printf("Error printing data\n");
		return 0;
	}
	G_Storage.Close();
	if(output)
		fclose(output);
	return 1;
}

int main(int argc, char * argv[])
{
	int    use_debug = 0;
	printf("System Papyrus\nCopyrighs (c) A.Sobolev, A.Fokin 1996-2004\n");
	printf("DL200 language compiler\n");
	if(argc < 2) {
		printf("Usage: DL200C inputFileName [/d]\n");
		printf("/d - debug mode\n");
		return -1;
	}
	if(argc > 2) {
		for(char * p = argv[2]; *p != 0; p++) {
			if(*p == '/' || *p == '-') {
				p++;
				if(toupper(*p) == 'D')
					use_debug = 1;
			}
		}
	}
	char   infilename[MAXPATH];
	// @v10.5.6 char   outfilename[MAXPATH];
	SString outfilename; // @v10.5.6 
	STRNSCPY(infilename, argv[1]);
	if(!fileExists(infilename)) {
		printf("Error: file %s not found\n", infilename);
		return -1;
	}
	// @v10.5.6 STRNSCPY(outfilename, infilename);
	// @v10.5.6 replaceExt(outfilename, "bin", 1);
	outfilename = infilename; // @v10.5.6 
	SPathStruc::ReplaceExt(outfilename, "bin", 1); // @v10.5.6 	
	yyin = fopen(infilename, "r");
	if(yyin == 0) {
		printf("Error: Could not open file %s\n", infilename);
		return -1;
	}
	/*
	if(fileExists(outfilename)) {
		char r = 0;
		printf("Warning: File %s too exists. Rewrite it ? (Y/N)\n", outfilename);
		do {
			scanf("%c", &r);
		} while(toupper(r) != 'Y' && toupper(r) != 'N');
		if(toupper(r) != 'Y')
			return 0;
	}
	*/
	if(!G_Storage.Open(outfilename, 0)) {
		printf("Error: Could not create output file %s\n", outfilename);
		return -1;
	}
	yyparse();
	G_Storage.Close();
	if(use_debug)
		TestReadData(outfilename);
	return 0;
}
