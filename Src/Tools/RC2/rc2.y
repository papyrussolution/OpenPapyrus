// rc2.y
// Copyright (c) V. Antonov, A.Sobolev 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2013, 2015, 2016
// Part of project Papyrus
// Процессор для языка описания броузеров
//
%{
#include <pp.h>
#include "rc2.h"

#define YYERROR_VERBOSE

typedef struct {
	void * yyin_buf;
	char   fname[_MAX_PATH];
	long   yyin_line;
} YYIN_STR;

extern long yyin_cnt;
extern YYIN_STR * yyin_struct;
extern int yylex(void);
extern long yyline;
extern FILE * yyin;

void yyerror(const char * str)
{
	printf("%s(%d): error: %s\n", (const char *)Rc2.GetInputFileName(), yyline, str);
	exit(-1);
}

//---------from dl400/types.cpp
struct ExtTypeInfo {
	TYPEID Id;
	char   Name[64];
	char   TypeName[64];
};

static ExtTypeInfo SimpleTypes[] = {
	{T_CHAR,	"char",		"S_CHAR"},
	{T_INT,		"int16",	"S_INT"},
	{T_UINT,	"uint16",	"S_UINT"},
	{T_INT,		"short",	"S_INT"},
	{T_UINT,	"ushort",	"S_UINT"},
	{T_LONG,	"int32",	"S_INT"},
	{T_LONG,	"long",		"S_INT"},
	{T_ULONG,	"uint32",	"S_UINT"},
	{T_ULONG,	"ulong",	"S_UINT"},
	{T_FLOAT,	"float",	"S_FLOAT"},
	{T_DOUBLE,	"double",	"S_FLOAT"},
	{S_ZSTRING,	"char*",	"S_ZSTRING"},
	{T_DATE,	"date",		"S_DATE"},
	{T_ACCT,	"acct",		"S_ACCT"},
	{T_TIME,    "time",     "S_TIME"},    // @v6.4.3
	{T_DATETIME, "datetime", "S_DATETIME"}, // @v6.4.3
	{T_GUID,     "guid",     "S_UUID_"}     // @v8.6.2
};

TYPEID GetSType(const char * pName)
{
	for(int i = 0; i < SIZEOFARRAY(SimpleTypes); i++)
		if(!strcmp(SimpleTypes[i].Name, pName))
			return SimpleTypes[i].Id;
	{
		SString msg_buf;
		msg_buf.Printf("Invalid type '%s'", pName);
		yyerror(msg_buf.Printf("Invalid type '%s'", pName));
	}
	return 0;
}

const char * GetSTypeName(TYPEID t)
{
	for(int i = 0; i < SIZEOFARRAY(SimpleTypes); i++)
		if(GETSTYPE(SimpleTypes[i].Id) == GETSTYPE(t) && (GETSTYPE(t) == S_ZSTRING || GETSSIZE(SimpleTypes[i].Id) == GETSSIZE(t)))
			return SimpleTypes[i].TypeName;
	yyerror("Invalid type");
	return 0;
}

struct FormatFlag {
	const  char * P_Symb;
	long   F;
	int    BaseType;
};

#define FMT_F_ENTRY(s,t) { #s, s, t }

static FormatFlag FormatFlagList[] = {
	FMT_F_ENTRY(ALIGN_RIGHT,  0),
	FMT_F_ENTRY(ALIGN_LEFT,   0),
	FMT_F_ENTRY(ALIGN_CENTER, 0),
	FMT_F_ENTRY(COMF_FILLOVF, 0),

	FMT_F_ENTRY(NMBF_NONEG,     0),
	FMT_F_ENTRY(NMBF_NEGPAR,    0),
	FMT_F_ENTRY(NMBF_FORCEPOS,  0),
	FMT_F_ENTRY(NMBF_NOZERO,    0),
	FMT_F_ENTRY(NMBF_TRICOMMA,  0),
	FMT_F_ENTRY(NMBF_TRIAPOSTR, 0),
	FMT_F_ENTRY(NMBF_TRISPACE,  0),
	{ "NMBF_DELCOMMA",  NMBF_TRICOMMA,  0 },
	{ "NMBF_DELAPOSTR", NMBF_TRIAPOSTR, 0 },
	{ "NMBF_DELSPACE",  NMBF_TRISPACE,  0 },	
	FMT_F_ENTRY(NMBF_NOTRAILZ,  0),

	FMT_F_ENTRY(DATF_AMERICAN, BTS_DATE),
	FMT_F_ENTRY(DATF_ANSI,     BTS_DATE),
	FMT_F_ENTRY(DATF_BRITISH,  BTS_DATE),
	FMT_F_ENTRY(DATF_FRENCH,   BTS_DATE),
	FMT_F_ENTRY(DATF_GERMAN,   BTS_DATE),
	FMT_F_ENTRY(DATF_ITALIAN,  BTS_DATE),
	FMT_F_ENTRY(DATF_JAPAN,    BTS_DATE),
	FMT_F_ENTRY(DATF_USA,      BTS_DATE),
	FMT_F_ENTRY(DATF_MDY,      BTS_DATE),
	FMT_F_ENTRY(DATF_DMY,      BTS_DATE),
	FMT_F_ENTRY(DATF_YMD,      BTS_DATE),
	FMT_F_ENTRY(DATF_CENTURY,  BTS_DATE),

	FMT_F_ENTRY(TIMF_HMS,   BTS_TIME),
	FMT_F_ENTRY(TIMF_HM,    BTS_TIME),
	FMT_F_ENTRY(TIMF_MS,    BTS_TIME),
	FMT_F_ENTRY(TIMF_S,     BTS_TIME),
	FMT_F_ENTRY(TIMF_MSEC,  BTS_TIME),
	FMT_F_ENTRY(TIMF_BLANK, BTS_TIME),

	FMT_F_ENTRY(STRF_UPPER,    BTS_STRING),
	FMT_F_ENTRY(STRF_LOWER,    BTS_STRING),
	FMT_F_ENTRY(STRF_PASSWORD, BTS_STRING),
	FMT_F_ENTRY(STRF_OEM,      BTS_STRING),
	FMT_F_ENTRY(STRF_ANSI,     BTS_STRING)
};

static long GetFmtFlag(const char * pSymb)
{
	for(uint i = 0; i < sizeof(FormatFlagList) / sizeof(FormatFlag); i++) {
		if(strcmp(pSymb, FormatFlagList[i].P_Symb) == 0) {
			return FormatFlagList[i].F;
		}
	}
	{
		char msg_buf[256];
		sprintf(msg_buf, "Invalid format flag '%s'", pSymb);
		yyerror(msg_buf);
	}
	return 0;
}
//
// Stub (instead PPLIB\PPDBUTIL.CPP
//
int CallbackCompress(long, long, const char *, int)
{
	return 1;
}

//---------

BrowserDefinition browser;
BrowserColumn     column;
GroupDefinition   group;
Rc2ToolbarItem    toolbar_item;
int  columnCount = 0;

%}

%union {
	long   lval;
	double dval;
	LAssocBase las_val; // @v10.4.0 LAssoc-->LAssocBase (@noctr)
	SColorBase scolor_val;
	char   sval[256];
	Rc2ToolbarItem tbi_val;
	CmdDefinition::CmdFilt cmd_filt;
}

%token T_BROWSER
%token T_TOOLBAR
%token T_GROUP
%token T_SEPARATOR
%token T_CROSSTAB
%token T_NOTYPE
%token T_ZSTRING
%token T_CHR
%token T_EXPORT
%token T_DECLARE_CMD

%token T_OBJ        // "ppobject"
%token T_JOB        // "job"
%token T_CMD        // "cmd"
%token T_RECORD     // "record"
%token T_FORMAT     // "format"
%token T_VIEW       // "view"
%token T_FILTER     // "filter"
%token T_CTRLMENU   // "ctrlmenu"
%token T_REPORTSTUB // "reportstub"
%token T_DL600DATA  // "dl600data"
%token T_HIDDEN     // "hidden"

%token T_BITMAP     // "bitmap"
%token T_FILE       // "file" 
%token T_DRAWVECTOR // "drawvector"

%token <sval> T_IDENT
%token <sval> T_STR
%token <lval> T_TINT
%token <scolor_val> T_COLOR

%type <tbi_val> tb_entry
%type <lval>    tb_entry_options_list
%type <lval>    tb_entry_option
%type <lval>    job_option_list
%type <lval>    job_option
%type <lval>    cmd_option_list
%type <lval>    cmd_option
%type <lval>    type_def
%type <lval>    width_def
%type <lval>    format_flag_list
%type <lval>    field_fmt_def
%type <sval>    field_descr
%type <cmd_filt> cmd_filt
%type <lval>    file_option
%type <lval>    file_options_list
%%

rc_file           : entry_list;
entry_list        : | entry_list entry | entry_list entry ';';
entry             : /*declare_stmt_list*/ declare_stmt | browser_def | global_toolbar_def | job_def | obj_def | cmd_def |
	record_def | view_def | filter_def | reportstub_def | ctrlmenu_def | bitmap_def | drawvector_def | file_def;
//declare_stmt_list : declare_stmt | declare_stmt_list declare_stmt;

declare_stmt :
	{
	} | T_DECLARE_CMD T_IDENT
	{
		long   symb_id = 0;
		SString msg_buf;
		if(!Rc2.AddSymb(DeclareSymb::kCommand, $2, &symb_id, msg_buf))
			yyerror(msg_buf);
	} | T_DECLARE_CMD T_IDENT '=' T_TINT
	{
		long   symb_id = $4;
		SString msg_buf;
		if(!Rc2.AddSymb(DeclareSymb::kCommand, $2, &symb_id, msg_buf))
			yyerror(msg_buf);
	}
	
drawvector_def : T_DRAWVECTOR T_STR
	{
		SColor dummy_replaced_color(0, 0, 0, 0);
		Rc2.SetupDrawVectorGroup($2, dummy_replaced_color);
	} '{' drawvector_entries_def '}' 
	{
	} | T_DRAWVECTOR T_STR T_COLOR 
	{
		Rc2.SetupDrawVectorGroup($2, $3);	
	} '{' drawvector_entries_def '}' 
	
drawvector_entries_def : | drawvector_entries_def drawvector_entry_def

drawvector_entry_def : T_IDENT
	{
		SString msg_buf;
		SColor dummy_replaced_color(0, 0, 0, 0);
		if(!Rc2.AddDrawVector($1, dummy_replaced_color, msg_buf))
			yyerror(msg_buf);	
	} | T_IDENT T_COLOR
	{
		SString msg_buf;
		if(!Rc2.AddDrawVector($1, $2, msg_buf))
			yyerror(msg_buf);		
	}
	
bitmap_def : T_BITMAP 
	{
		Rc2.SetupBitmapGroup(0);
	} '{' bitmap_entries_def '}'
	
bitmap_def : T_BITMAP T_STR
	{
		Rc2.SetupBitmapGroup($2);
	} '{' bitmap_entries_def '}'	

bitmap_entries_def : | bitmap_entries_def bitmap_entry_def

bitmap_entry_def : T_IDENT
	{
		SString msg_buf;
		if(!Rc2.AddBitmap($1, msg_buf))
			yyerror(msg_buf);
	}

browser_def : T_BROWSER T_IDENT layout_def ',' T_TINT ',' T_TINT ',' T_STR ',' browser_flags_def ',' helpid_def '{' browser_entries_def '}'
	{
		STRNSCPY(browser.Name, $2);
		browser.Height = $5;
		browser.Freeze = $7;
		STRNSCPY(browser.Header, $9);
		columnCount = 0;
		Rc2.GenerateBrowserDefinition(&browser);
		Rc2.GenerateBrowserDefine(browser.Name, browser.Header);
		browser.Groups.freeAll();
		browser.Columns.freeAll();
		browser.Toolbar.freeAll();
	}

helpid_def : T_IDENT
	{
		STRNSCPY(browser.HelpID, $1);
	} | T_TINT
	{
		browser.HelpID[0] = '0';
		browser.HelpID[1] = 0;
	}

browser_entries_def : | browser_entries_def browser_entry_def

browser_entry_def   : column_def | group_def | local_toolbar_def

group_def : T_GROUP T_STR '{' group_entries_def '}'
	{
		STRNSCPY(group.Name, $2);
		group.stopColumn = columnCount - 1;
		GroupDefinition * r = new GroupDefinition;
		*r = group;
		browser.Groups.insert(r);
		group.stopColumn = group.startColumn = 0;
		*group.Name = 0;
	}

group_entries_def :
	| group_entries_def column_def
	{
		if(group.startColumn == 0)
			group.startColumn = columnCount - 1;
	}
/* -------------------
	Toolbar definition
---------------------- */
local_toolbar_def : T_TOOLBAR T_IDENT '{' toolbar_entries_def '}'
	{
		*browser.Toolbar.Name = 0;
		STRNSCPY(browser.Toolbar.BitmapIndex, $2);
		browser.Toolbar.IsLocal = 1;
	} | T_TOOLBAR T_IDENT
	{
		browser.Toolbar.IsLocal = 1;
		STRNSCPY(browser.Toolbar.Name, $2);
		*browser.Toolbar.BitmapIndex = 0;
	}

global_toolbar_def : T_TOOLBAR T_IDENT T_IDENT '{' toolbar_entries_def '}'
	{
		browser.Toolbar.IsLocal = 0;
		browser.Toolbar.IsExport = 0;
		STRNSCPY(browser.Toolbar.Name, $2);
		STRNSCPY(browser.Toolbar.BitmapIndex, $3);
		Rc2.GenerateToolbarDefinition(&browser.Toolbar);
		Rc2.GenerateToolbarDefine(browser.Toolbar.Name);
		browser.Toolbar.freeAll();
	} | T_TOOLBAR T_EXPORT T_IDENT T_IDENT '{' toolbar_entries_def '}'
	{
		browser.Toolbar.IsLocal = 0;
		browser.Toolbar.IsExport = 1;
		STRNSCPY(browser.Toolbar.Name, $3);
		STRNSCPY(browser.Toolbar.BitmapIndex, $4);
		Rc2.GenerateToolbarDefinition(&browser.Toolbar);
		Rc2.GenerateToolbarDefine(browser.Toolbar.Name);
		browser.Toolbar.freeAll();
	}

toolbar_entries_def :
{
} | toolbar_entries_def tb_entry
{
	browser.Toolbar.insert(& $2);
}

tb_entry : T_SEPARATOR
	{
		$$.InitSeparator();
	} | T_IDENT ',' T_STR ',' T_IDENT
	{
		$$.Init($1, $3, $5);
	} | T_IDENT T_STR ',' T_IDENT
	{
		toolbar_item.Init($1, $2, $4);
	} | T_IDENT ',' T_STR T_IDENT
	{
		$$.Init($1, $3, $4);
	} | T_IDENT ':' T_IDENT ',' T_IDENT ',' T_STR
	{
		long   symb_id = 0;
		SString msg_buf;
		if(!Rc2.AddSymb(DeclareSymb::kCommand, $1, &symb_id, msg_buf))
			yyerror(msg_buf);
		$$.Init(symb_id, 0, $3, $5, $7);
	} | T_IDENT ':' T_IDENT ',' T_IDENT ',' tb_entry_options_list ',' T_STR
	{
		long   symb_id = 0;
		SString msg_buf;
		if(!Rc2.AddSymb(DeclareSymb::kCommand, $1, &symb_id, msg_buf))
			yyerror(msg_buf);
		$$.Init(symb_id, $7, $3, $5, $9);
	}

tb_entry_options_list : tb_entry_option
	{
		$$ = $1;
	} | tb_entry_options_list tb_entry_option
	{
		$$ = $1 | $2;
	}

tb_entry_option : T_HIDDEN
{
	$$ = Rc2ToolbarItem::fHidden;
}

/* -------------------
	Column definition
---------------------- */
column_def : T_CROSSTAB T_STR ',' type_def ',' column_flags_def ',' width_def
	{
		column.IsCrosstab = 1;
		STRNSCPY(column.Name, $2);
		column.Type = static_cast<TYPEID>($4);
		column.Width = SFMTLEN($8);
		column.Prec  = SFMTPRC($8);
		*column.ReqNumber = 0;
		columnCount++;
		browser.Columns.insert(new BrowserColumn(column));
	} | T_STR ',' reqnum_def ',' type_def ',' column_flags_def ',' width_def column_options
	{
		column.IsCrosstab = 0;
		STRNSCPY(column.Name, $1);
		column.Type = (TYPEID)$5;
		column.Width = SFMTLEN($9);
		column.Prec  = SFMTPRC($9);
		columnCount++;
		browser.Columns.insert(new BrowserColumn(column));
	}

column_options :
	{
		column.Options[0] = '0';
		column.Options[1] = 0;
	} | ',' column_options_list
	{
	}

column_options_list : T_IDENT
	{
		STRNSCPY(column.Options, $1);
	} | column_options_list '|' T_IDENT
	{
		strcat(strcat(column.Options, "|"), $3);
	}

reqnum_def : T_TINT
	{
		_itoa($1, column.ReqNumber, 10);
	} | T_IDENT
	{
		STRNSCPY(column.ReqNumber, $1);
	}

column_flags_def : T_TINT
	{
		if($1 == 0) {
			column.Flags[0] = '0';
			column.Flags[1] = 0;
		}
		else
			yyerror("invalid flags");
	} | T_IDENT
	{
		STRNSCPY(column.Flags, $1);
	} | column_flags_def '|' T_IDENT
	{
		strcat(strcat(column.Flags, "|"), $3);
	}

width_def : T_TINT
	{
		$$ = MKSFMT($1, 0);
	} | T_TINT '.' T_TINT
	{
		$$ = MKSFMTD($1, $3, 0);
	}

type_def : T_NOTYPE
	{
		$$ = 0;
	} | T_IDENT
	{
		$$ = GetSType($1);
	} | T_ZSTRING '(' T_TINT ')'
	{
		$$ = (TYPEID)MKSTYPE(S_ZSTRING, $3);
	} | T_CHR '[' T_TINT ']'
	{
		$$ = (TYPEID)MKSTYPE(S_ZSTRING, $3);
	}

browser_flags_def : T_TINT
	{
		if($1 == 0)
			browser.Flags = 0;
		else
			yyerror("invalid flags");
	} | T_IDENT
	{
		GetFlagValue($1, browser_flags, &browser.Flags);
	} | browser_flags_def '|' T_IDENT
	{
		int r = 0;
		GetFlagValue($3, browser_flags, &r);
		browser.Flags += r;
	}

layout_def : T_IDENT '(' T_TINT ')'
	{
		int t;
		if(SearchSubStr(base, &t, $1, 1) <= 0)
			yyerror("invalid browser layout");
		browser.Layout.LayoutBase = (BrowserLayout::Base)t;
		browser.Layout.SizeInPercent = $3;
	}

/*-----------------------------
RECORD definition
-------------------------------*/

record_def : T_RECORD T_IDENT '{' rec_field_list '}'
{
	SString msg_buf;
	if(!Rc2.AddRecord($2, msg_buf))
		yyerror(msg_buf);
}

rec_field_list : rec_field
{
} | rec_field_list rec_field
{
}

rec_field : T_IDENT type_def field_fmt_def field_descr ';'
{
	SString msg_buf;
	if(!Rc2.SetFieldDefinition($1, (TYPEID)$2, $3, $4, msg_buf))
		yyerror(msg_buf);
}

field_fmt_def :
	{
		$$ = 0;
	} | T_FORMAT '(' width_def ',' format_flag_list ')'
	{
		$$ = MKSFMTD(SFMTLEN($3), SFMTPRC($3), $5);
	} | T_FORMAT '(' width_def ')'
	{
		$$ = MKSFMTD(SFMTLEN($3), SFMTPRC($3), 0);
	} | T_FORMAT '(' format_flag_list ')'
	{
		$$ = MKSFMT(0, $3);
	}

format_flag_list : T_IDENT
	{
		$$ = GetFmtFlag($1);
	} | format_flag_list '|' T_IDENT
	{
		$$ = $1 | GetFmtFlag($3);
	}

field_descr :
	{
		$$[0] = 0;
	} | T_STR
	{
		STRNSCPY($$, $1);
	}

/*-----------------------------
COMMAND definition
-------------------------------*/
/*
cmd name { description icon_ident toolbar_ident cmIdent options }
*/
cmd_def : T_CMD T_IDENT '{' T_STR T_IDENT T_IDENT T_IDENT cmd_option_list cmd_filt '}'
{
	SString msg_buf;
	CmdDefinition def;
	STRNSCPY(def.Name, $2);
	STRNSCPY(def.Descr, $4);
	STRNSCPY(def.IconIdent, $5);
	STRNSCPY(def.ToolbarIdent, $6);
	STRNSCPY(def.MenuCmdIdent, $7);
	def.Flags = $8;
	def.Filt = $9;
	if(!Rc2.AddCmd(&def, msg_buf))
		yyerror(msg_buf);
}

cmd_option_list :
{
	$$ = 0;
} | cmd_option
{
	$$ |= $1;
} | cmd_option '|' cmd_option_list
{
	$$ |= ($1 | $3);
}

cmd_option : "noparam"
{
	$$ = PPCommandDescr::fNoParam;
}

cmd_filt :
{
	$$.DeclView = 0;
	$$.FiltSymb[0] = 0;
	$$.FiltExtraSymb[0] = 0;
} | T_FILTER '=' T_IDENT
{
	$$.DeclView = 0;
	STRNSCPY($$.FiltSymb, $3);
	$$.FiltExtraSymb[0] = 0;
} | T_FILTER '=' T_IDENT '(' T_IDENT ')'
{
	$$.DeclView = 0;
	STRNSCPY($$.FiltSymb, $3);
	STRNSCPY($$.FiltExtraSymb, $5);
} | T_FILTER '=' T_IDENT '(' T_TINT ')'
{
	$$.DeclView = 0;
	STRNSCPY($$.FiltSymb, $3);
	_ltoa($5, $$.FiltExtraSymb, 10);
} | T_VIEW '=' T_IDENT
{
	$$.DeclView = 1;
	STRNSCPY($$.FiltSymb, $3);
	$$.FiltExtraSymb[0] = 0;
} | T_VIEW '=' T_IDENT '(' T_IDENT ')'
{
	$$.DeclView = 1;
	STRNSCPY($$.FiltSymb, $3);
	STRNSCPY($$.FiltExtraSymb, $5);
} | T_VIEW '=' T_IDENT '(' T_TINT ')'
{
	$$.DeclView = 1;
	STRNSCPY($$.FiltSymb, $3);
	_ltoa($5, $$.FiltExtraSymb, 10);
}

/*-----------------------------
JOB definition
-------------------------------*/

job_def : T_JOB T_IDENT '{' T_STR job_option_list '}'
{
	SString msg_buf;
	JobDefinition def;
	STRNSCPY(def.Name, $2);
	STRNSCPY(def.Descr, $4);
	def.Flags = $5;
	if(!Rc2.AddJob(&def, msg_buf))
		yyerror(msg_buf);
}

job_option_list :
{
	$$ = 0;
} | job_option
{
	$$ |= $1;
} | job_option ',' job_option_list
{
	$$ |= ($1 | $3);
}

job_option : T_IDENT
{
	if(_stricmp($1, "noparam") == 0)
		$$ = PPJobDescr::fNoParam;
	else if(_stricmp($1, "nologin") == 0)
		$$ = PPJobDescr::fNoLogin;
	else {
		SString msg_buf;
		msg_buf.Printf("invalid job option '%s'", $1);
		yyerror(msg_buf);
	}
}

obj_def : T_OBJ T_IDENT '{' T_STR '}'
{
	SString msg_buf;
	ObjDefinition def;
	STRNSCPY(def.Name, $2);
	STRNSCPY(def.Descr, $4);
	if(!Rc2.AddObj(&def, msg_buf))
		yyerror(msg_buf);
}

obj_replyon_list : obj_replyon_item | obj_replyon_item obj_replyon_list
{
}

obj_replyon_item : '{' T_IDENT T_IDENT '}'
{
}

/*-----------------------------
VIEW definition
-------------------------------*/

view_def : T_VIEW T_IDENT '{' T_STR '}'
{
	SString msg_buf;
	ViewDefinition def;
	def.Name = $2;
	def.Descr = $4;
	if(!Rc2.AddView(&def, msg_buf))
		yyerror(msg_buf);
}

filter_def : T_FILTER T_IDENT '{' T_STR '}'
{
	SString msg_buf;
	ViewDefinition def;
	def.Flags |= ViewDefinition::fFilterOnly;
	def.Name = $2;
	def.Descr = $4;
	if(!Rc2.AddView(&def, msg_buf))
		yyerror(msg_buf);
}

/*-----------------------------
REPORTSTUB definition
-------------------------------*/

reportstub_def : T_REPORTSTUB T_IDENT '{' T_DL600DATA '=' T_IDENT ',' T_STR '}'
{
	SString msg_buf;
	ReportStubDefinition def;
	def.Name = $2;
	def.Data = $6;
	def.Descr = $8;
	if(!Rc2.AddReportStub(&def, msg_buf))
		yyerror(msg_buf);
}

/*-----------------------------
CTRLMENU definition
-------------------------------*/

ctrlmenu_def : ctrlmenu_header '{' ctrlmenu_item_list '}'
{
	Rc2.AcceptCtrlMenu();
}

ctrlmenu_header : T_CTRLMENU T_IDENT
{
	printf("ctrlmenu_header\n");
	Rc2.InitCurCtrlMenu($2);
	long   symb_id = 0;
	SString msg_buf;
	if(!Rc2.AddSymb(DeclareSymb::kCtrlMenu, $2, &symb_id, msg_buf))
		yyerror(msg_buf);
}

ctrlmenu_item_list : ctrlmenu_item | ctrlmenu_item ctrlmenu_item_list

ctrlmenu_item : '(' T_STR T_IDENT ')'
{
	Rc2.AddCtrlMenuItem($2, $3);
}
//
// FILE definition
//
file_def : T_FILE T_IDENT '{' T_STR T_IDENT file_options_list T_STR '}'
{
	SString path_mnem, msg_buf;
	long path_id = Rc2.ResolveRPathSymb($5, path_mnem, msg_buf);
	if(path_id) {
		if(!Rc2.AddRFileDefinition($2, $4, path_mnem, $6, $7, msg_buf))
			yyerror(msg_buf);
	}
	else
		yyerror(msg_buf);
}

file_options_list : 
{
	$$ = 0;
} | file_option 
{
	$$ = $1;
} | file_option file_options_list 
{
	$$ = $1 | $2;	
}

file_option : T_IDENT
{
	SString msg_buf;
	$$ = Rc2.ResolveRFileOption($1, msg_buf);
	if($$ == 0) 
		yyerror(msg_buf);
}
%%
