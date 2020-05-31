// RC2.H
// Copyright V.Antonov, A.Sobolev 2006, 2007, 2008, 2013, 2016, 2020
// Part of project Papyrus
//
#include <slib.h>
#include <string.h>
#include <db.h>

class TRect;

struct BrowserLayout {
	BrowserLayout() : LayoutBase(LayoutCenter), SizeInPercent(0)
	{ 
	}
	enum Base {
		LayoutCenter = 0,
		LayoutNorth,
		LayoutSouth,
		LayoutWest,
		LayoutEast
	};
	Base   LayoutBase;
	int    SizeInPercent;
};
//
//
//
struct DeclareSymb {
	enum {
		kCommand = 1,
		kJob,
		kObj,
		kCmd,
		kRecord,
		kView,
		kReportStub,
		kCtrlMenu,
		kBitmap,
		kRFile,
		kDrawVector
	};
	int    Kind;
	char   Symb[40];
	long   ID;
};

class DeclareSymbList : public TSArray <DeclareSymb> {
public:
	DeclareSymbList();
	int    AddSymb(int kind, const char * pSymb, long * pID);
	int    SearchSymb(int kind, const char * pSymb, uint * pPos);
	int    SearchSymbByID(int kind, long id, char * pBuf, size_t bufLen);
	SString & GetSymbKindName(int kind, SString & rBuf) const;
private:
	LAssocArray LastIdList;
};
//
//
//
struct Rc2BitmapItem {
	long   SymbID;
	char   FileName[128];
};

struct Rc2DrawVectorItem {
	Rc2DrawVectorItem() : SymbID(0), ReplacedColor(ZEROCOLOR)
	{
		PTR32(FileName)[0] = 0;
	}
	long   SymbID;
	SColor ReplacedColor; // Цвет, который может быть изменен при отрисовке. Если ReplacedColor.Alpha == 0, то не определен
	char   FileName[128];
};
//
//
//
struct Rc2ToolbarItem {
	Rc2ToolbarItem & InitSeparator();
	Rc2ToolbarItem & Init(const char * pKeyCode, const char * pToolTip, const char * pBitmap);
	Rc2ToolbarItem & Init(long cmd, long flags, const char * pKeyCode, const char * pBitmap, const char * pToolTip);

	enum {
		fHidden    = 0x0001,
		fSeparator = 0x0002
	};
	long   Cmd;
	long   Flags;
	char   KeyCode[32];
	char   BitmapIndex[32];
	char   ToolTipText[128];
};

class ToolbarDefinition : public TSArray <Rc2ToolbarItem> {
public:
	ToolbarDefinition() : TSArray <Rc2ToolbarItem> (), IsLocal(0), IsExport(0)
	{
		PTR32(Name)[0] = 0;
		PTR32(BitmapIndex)[0] = 0;
	}
	int    IsLocal;
	int    IsExport;
	char   Name[40];
	char   BitmapIndex[40];
};

struct GroupDefinition {
	GroupDefinition() : startColumn(0), stopColumn(0)
	{
		PTR32(Name)[0] = 0;
	}
	char   Name[64];
	int    startColumn;
	int    stopColumn;
};

struct BrowserColumn {
	BrowserColumn() : IsCrosstab(0), Width(0), Prec(0), Type(0)
	{
		PTR32(Name)[0] = 0;
		PTR32(ReqNumber)[0] = 0;
		PTR32(Flags)[0] = 0;
		PTR32(Options)[0] = 0;
	}
	int    IsCrosstab;
	char   Name[64];
	char   ReqNumber[64];
	char   Flags[128];
	char   Options[128];
	int    Width;
	int    Prec;
	TYPEID Type;
};

struct BrowserDefinition {
	BrowserDefinition() : Height(0), Freeze(0), Flags(0)
	{
		PTR32(Name)[0] = 0;
		PTR32(Header)[0] = 0;
		PTR32(HelpID)[0] = 0;
	}
	char   Name[64];
	int    Height;
	int    Freeze;
	int    Flags;
	char   Header[64];
	char   HelpID[64];
	TSCollection <GroupDefinition> Groups;
	TSCollection <BrowserColumn>   Columns;
	ToolbarDefinition Toolbar;
	BrowserLayout Layout;
};

struct CmdDefinition {
	struct CmdFilt {
		CmdFilt & Z()
		{
			DeclView = 0;
			PTR32(FiltSymb)[0] = 0;
			PTR32(FiltExtraSymb)[0] = 0;
			return *this;
		}
		int    DeclView;           // Если !0 то FiltSymb рассматривается как символ view, а не filter
		char   FiltSymb[32];
		char   FiltExtraSymb[32];
	};
	CmdDefinition() : ID(0), Flags(0)
	{
		PTR32(Name)[0] = 0;
		PTR32(Descr)[0] = 0;
		PTR32(IconIdent)[0] = 0;
		PTR32(ToolbarIdent)[0] = 0;
		PTR32(MenuCmdIdent)[0] = 0;
		Filt.Z();
	}
	long   ID;
	long   Flags;
	char   Name[64];
	char   Descr[256];
	char   IconIdent[32];
	char   ToolbarIdent[32];
	char   MenuCmdIdent[32];
	CmdFilt Filt;
};

struct CtrlMenuItem {
	char   Descr[128];
	char   KeyCode[32];
};

class CtrlMenuDefinition : public TSArray <CtrlMenuItem> {
public:
	CtrlMenuDefinition() : TSArray <CtrlMenuItem> ()
	{
		PTR32(Name)[0] = 0;
	}
	char   Name[64];
};

struct JobDefinition {
	JobDefinition() : ID(0), Flags(0)
	{
		PTR32(Name)[0] = 0;
		PTR32(Descr)[0] = 0;
	}
	long   ID;
	long   Flags;
	char   Name[64];
	char   Descr[256];
};

struct ObjDefinition {
	ObjDefinition() : ID(0)
	{
		PTR32(Name)[0] = 0;
		PTR32(Descr)[0] = 0;
	}
	long   ID;
	char   Name[64];
	char   Descr[256];
};

struct ViewDefinition {
	enum {
		fFilterOnly = 0x0001
	};
	ViewDefinition() : ID(0), Flags(0)
	{
	}
	long   ID;
	long   Flags;
	SString Name;
	SString Descr;
};

struct ReportStubDefinition {
	ReportStubDefinition() : ID(0)
	{
	}
	long   ID;
	SString Name;
	SString Data;
	SString Descr;
};

struct RFileDefinition {
	RFileDefinition() : ID(0), Flags(0)
	{
	}
	long   ID;
	long   Flags;
	SString Symb;
	SString Name;
	SString PathMnem;
	SString SrcPathMnem;
	SString Descr;
};

TYPEID GetSType(const char * pName);
const  char * GetSTypeName(TYPEID t);
int    yyparse();

//extern const char tv_browser_flags[];
extern const char browser_flags[];
extern const char base[];
extern FILE * yyin; // Входной файл

int SLAPI SearchSubStr(const char * pStr, int * pIdx, const char * pTestStr, int ignoreCase);
int SLAPI GetFlagValue(const char * pFlagText, const char * pVariants, int * pResult);

class Rc2Data {
public:
	SLAPI  Rc2Data() : pHdr(0), pRc(0), BrwCounter(0), TbCounter(0), P_Record(0), P_CurCtrlMenuDef(0)
	{
	}
	SLAPI ~Rc2Data()
	{
		SFile::ZClose(&pHdr);
		SFile::ZClose(&pRc);
		delete P_CurCtrlMenuDef;
	}
	int    SLAPI Init(const char * pInputFileName, const char * pRcName, const char * pHdrName, const char * pEmbedRcName);
	const  SString & GetInputFileName() const { return InputFileName; }
	int    SLAPI GenerateRCHeader();
	void   SLAPI GenerateIncHeader();
	int    SLAPI GenerateSymbDefinitions();
	int    SLAPI GenerateBrowserDefine(char * pName, char * comment);
	int    SLAPI GenerateBrowserDefinition(BrowserDefinition * pB);
	int    SLAPI GenerateToolbarDefine(char * pName);
	int    SLAPI GenerateToolbarDefinition(ToolbarDefinition * pT);
	int    SLAPI GenerateJobDefinitions();
	int    SLAPI GenerateObjDefinitions();
	int    SLAPI GenerateCmdDefinitions();
	int    SLAPI GenerateRecDefinitions();
	int    SLAPI GenerateReportStubDefinitions();
	int    SLAPI GenerateViewDefinitions();
	int    SLAPI GenerateCtrlMenuDefinitions();
	int    SLAPI GenerateBitmapDefinitions();
	int    SLAPI GenerateDrawVectorFile(const char * pStorageFileName);
	int    SLAPI GenerateRFileDefinitions();
	int    SLAPI AddSymb(int kind, const char * pSymb, long * pID, SString & rErrMsg); // @>>SymbolList.AddSymb
	int    SLAPI AddJob(const JobDefinition *, SString & rErrMsg);   // @>>AddSymb
	int    SLAPI AddObj(const ObjDefinition *, SString & rErrMsg);   // @>>AddSymb
	int    SLAPI AddCmd(const CmdDefinition *, SString & rErrMsg);   // @>>AddSymb
	int    SLAPI AddView(const ViewDefinition *, SString & rErrMsg);
	int    SLAPI AddReportStub(const ReportStubDefinition *, SString & rErrMsg);
	int    SLAPI InitCurCtrlMenu(const char * pName);
	int    SLAPI AddCtrlMenuItem(const char * pDescr, const char * pKeyCode);
	int    SLAPI AcceptCtrlMenu();
	int    SetFieldDefinition(const char * pName, TYPEID typ, long fmt, const char * pDescr, SString & rErrMsg);
	int    AddRecord(const char * pName, SString & rErrMsg); // @>>AddSymb
	int    SetupBitmapGroup(const char * pPath);
	int    AddBitmap(const char * pSymbol, SString & rErrMsg); // @>>AddSymb
	int    SetupDrawVectorGroup(const char * pPath, SColor replacedColor);
	int    AddDrawVector(const char * pSymbol, SColor replacedColor, SString & rErrMsg);
	long   ResolveRFileOption(const char * pSymbol, SString & rErrMsg);
	long   ResolveRPathSymb(const char * pSymbol, SString & rCppMnem, SString & rErrMsg);
	int    AddRFileDefinition(const char * pSymbol, const char * pName, const char * pPathMnem, long flags, const char * pDescr, SString & rErrMsg);
private:
	int    SLAPI GenerateIncludeDirec(FILE * pF, const char * pFileName, int angleBraces);
	int    SLAPI GenerateToolbarEntries(FILE * pF, ToolbarDefinition * pT);
	int    SLAPI LayoutToRect(BrowserLayout * pL, TRect * pR);
	int    SLAPI GenerateRecordStruct(const SdRecord * pRec);

	int    BrwCounter;
	int    TbCounter;
	SdRecord * P_Record;
	SString EmbedFileName; // Имя rc-файла, который будет включен в ресурс ppw.rc
	TSCollection <JobDefinition> JobList;
	TSCollection <ObjDefinition> ObjList;
	TSCollection <CmdDefinition> CmdList;
	TSCollection <SdRecord> RecList;
	TSCollection <ViewDefinition> ViewList;
	TSCollection <ReportStubDefinition> RptStubList;
	TSCollection <CtrlMenuDefinition> CtrlMenuList;
	TSCollection <RFileDefinition> RFileList;
	struct BitmapGroup {
		SString Path;
		TSArray <Rc2BitmapItem> List;
	};
	TSCollection <BitmapGroup> BitmapList;

	struct DrawVectorGroup {
		DrawVectorGroup() : ReplacedColor(0, 0, 0, 0)
		{
		}
		SString Path;
		SColor ReplacedColor;
		TSArray <Rc2DrawVectorItem> List;
	};
	TSCollection <DrawVectorGroup> DrawVectorList;
	FILE * pHdr; // Генерируемый заголовочный файл
	FILE * pRc;  // Генерируемый RC файл
	SString InputFileName;
	DeclareSymbList SymbolList;
	CtrlMenuDefinition * P_CurCtrlMenuDef; // Текущее описание локального меню
};

extern Rc2Data Rc2;
//
//
//
//extern const char * P_DefinePrefix;   // "#define ";   @defined(rc2.cpp)
//extern const char * P_IncludePrefix;  // "#include ";  @defined(rc2.cpp)
//extern const char * P_VCmdPrefix;     // "PPVCMD_";    @defined(rc2.cpp)
//extern const char * P_BrwPrefix;      // "BROWSER_"    @defined(rc2.cpp)
//extern const char * P_TbPrefix;       // "TOOLBAR_"    @defined(rc2.cpp)
//extern const char * P_JobPrefix;      // "PPJOB_";     @defined(rc2.cpp)
//extern const char * P_ObjPrefix;      // "PPOBJ_";     @defined(rc2.cpp)
//extern const char * P_CmPrefix;       // "PPCMD_";     @defined(rc2.cpp)
//extern const char * P_RecPrefix;      // "PPREC_";     @defined(rc2.cpp)
//extern const char * P_FldPrefix;      // "PPFLD_"      @defined(rc2.cpp)
//extern const char * P_ViewPrefix;     // "PPVIEW_"     @defined(rc2.cpp)
//extern const char * P_FiltPrefix;     // "PPFILT_"     @defined(rc2.cpp)
//extern const char * P_ViewItemPrefix; // "PPVIEWITEM_" @defined(rc2.cpp)
//extern const char * P_CtrlMenuPrefix; // "CTRLMENU_"   @defined(rc2.cpp)
//extern const char * P_RFilePrefix;    // "PPRFILE_"    @defined(rc2.cpp)

/* @Project
class Rc2App {
public:
	Rc2App();
	~Rc2App();

	FILE * P_Hdr;
	FILE * P_Rc;
};
*/
