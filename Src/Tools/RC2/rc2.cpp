// RC2.CPP
// Copyright (c) V.Antonov, A.Sobolev 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2013, 2016, 2017, 2019
//
#include <pp.h>
#include "rc2.h"

static const char * P_DefinePrefix  = "#define ";   // @declared(rc2.h)
static const char * P_IncludePrefix = "#include ";  // @declared(rc2.h)
static const char * P_VCmdPrefix = "PPVCMD_";       // @declared(rc2.h)
static const char * P_BrwPrefix  = "BROWSER_";      // @declared(rc2.h)
static const char * P_TbPrefix   = "TOOLBAR_";      // @declared(rc2.h)
static const char * P_JobPrefix  = "PPJOB_";        // @declared(rc2.h)
static const char * P_ObjPrefix  = "PPOBJ_";        // @declared(rc2.h)
static const char * P_CmPrefix   = "PPCMD_";        // @declared(rc2.h)
static const char * P_RecPrefix  = "PPREC_";        // @declared(rc2.h)
static const char * P_FldPrefix  = "PPFLD_";        // @declared(rc2.h)
static const char * P_ViewPrefix = "PPVIEW_";       // @declared(rc2.h)
static const char * P_FiltPrefix = "PPFILT_";       // @declared(rc2.h)
static const char * P_ViewItemPrefix   = "PPVIEWITEM_"; // @declared(rc2.h)
static const char * P_ReportStubPrefix = "REPORT_";     // @declared(rc2.h)
static const char * P_CtrlMenuPrefix = "CTRLMENU_";     // @declared(rc2.h)
static const char * P_RFilePrefix = "PPRFILE_";         // @declared(rc2.h)

void yyerror(const char * str);

static const char tv_browser_flags[] = "BRO_OWNER;BRO_GRID";
const char browser_flags[] = "OWNER;GRID";
const char base[] = "center;north;south;west;east";

Rc2Data Rc2; // @global
//
//
//
int SLAPI GetSubStr(const char * pStr, int idx, char * buf, size_t buflen)
{
	uint pos = 0;
	StringSet ss(';', pStr);
	for(int i = 0; ss.get(&pos, buf, buflen); i++)
		if(i == idx)
			return 1;
	buf[0] = 0;
	return 0;
}

int SLAPI SearchSubStr(const char * pStr, int * pIdx, const char * pTestStr, int ignoreCase)
{
	int  idx = -1;
	uint pos = 0;
	char temp_buf[128];
	StringSet ss(';', pStr);
	for(int i = 0; idx < 0 && ss.get(&pos, temp_buf, sizeof(temp_buf)); i++)
		if(!_stricmp(temp_buf, pTestStr))
			idx = i;
	ASSIGN_PTR(pIdx, idx);
	return (idx < 0) ? 0 : 1;
}

int SLAPI GetFlagValue(const char * pFlagText, const char * pVariants, int * pResult)
{
	int r = 0;
	if(SearchSubStr(pVariants, &r, pFlagText, 0) <= 0) {
		char msg_buf[128];
		sprintf(msg_buf, "invalid flags '%s'", pFlagText);
		yyerror(msg_buf);
		return 0;
	}
	else {
		*pResult = 2<<r;
		return 1;
	}
}

int GetFlagsAsString(int flag, const char * pVariants, char * pBuf, int maxBuf)
{
	char buf[256];
	*pBuf = 0;
	if(flag) {
		for(int i = 1, j = 0; i <= flag; i*=2, j++)
			if(i & flag)
				if(GetSubStr(pVariants, j - 1, buf, sizeof(buf)) <= 0)
					yyerror("invalid flags");
				else {
					strncat(pBuf, buf, maxBuf - strlen(pBuf) - 1);
					if((i * 2) <= flag)
						strncat(pBuf, "|", maxBuf - strlen(pBuf) - 1);
				}
	}
	else {
		pBuf[0] = '0';
		pBuf[1] = 0;
	}
	return 1;
}
//
//
//
int SLAPI Rc2Data::Init(const char * pInputFileName, const char * pRcName, const char * pHdrName, const char * pEmbedRcName)
{
	InputFileName = pInputFileName;
	SFile::ZClose(&pHdr);
	SFile::ZClose(&pRc);
	pRc  = fopen(pRcName, "w");
	pHdr = fopen(pHdrName, "w");
	if(!pRc || !pHdr) {
		printf("cant create file %s\n" , pRc ? pHdrName : pRcName);
		return 0;
	}
	else {
		EmbedFileName = pEmbedRcName;
		return 1;
	}
}

int SLAPI Rc2Data::AddSymb(int kind, const char * pSymb, long * pID, SString & rErrMsg)
{
	long   id = pID ? *pID : 0;
	int    ok = SymbolList.AddSymb(kind, pSymb, pID);
	if(ok < 0) {
		SString kind_name;
		SymbolList.GetSymbKindName(kind, kind_name);
		rErrMsg.Printf("Duplicated name '%s' of %s", pSymb, kind_name.cptr());
	}
	else if(ok == 0) {
		(rErrMsg = "Can't create symbol").Space().Cat(pSymb);
		if(id)
			rErrMsg.CatChar('=').Cat(id);
	}
	return ok;
}
//
//
//
int Rc2Data::SetupDrawVectorGroup(const char * pPath, SColor replacedColor)
{
	DrawVectorGroup * p_group = new DrawVectorGroup;
	p_group->ReplacedColor = replacedColor;
	p_group->Path = pPath;
	if(p_group->Path.NotEmptyS())
		p_group->Path.ToLower().SetLastSlash();
	DrawVectorList.insert(p_group);
	return 1;
}

int Rc2Data::AddDrawVector(const char * pSymbol, SColor replacedColor, SString & rErrMsg)
{
	int    ok = 1;
	const  SColor dummy_replaced_color(0, 0, 0, 0);
	SString symbol(pSymbol);
	if(symbol.NotEmptyS()) {
		DrawVectorGroup * p_group = 0;
		Rc2DrawVectorItem item;
		{
			uint   c = DrawVectorList.getCount();
			if(c)
				p_group = DrawVectorList.at(c-1);
			if(!p_group) {
				if(SetupDrawVectorGroup(0, dummy_replaced_color))
					p_group = DrawVectorList.at(DrawVectorList.getCount()-1);
			}
		}
		if(p_group) {
			symbol.ToUpper();
			int    r = AddSymb(DeclareSymb::kDrawVector, symbol, &item.SymbID, rErrMsg);
			if(r > 0) {
				SString prefix_buf, body_buf, file_name;
				file_name = (symbol.Divide('_', prefix_buf, body_buf) > 0) ? body_buf : symbol;
				file_name.ToLower();
				SPathStruc::ReplaceExt(file_name, "svg", 0);
				if(p_group->Path.NotEmpty()) {
					(body_buf = p_group->Path).Cat(file_name);
					file_name = body_buf;
				}
				file_name.CopyTo(item.FileName, sizeof(item.FileName));
				item.ReplacedColor = (replacedColor == dummy_replaced_color) ? p_group->ReplacedColor : replacedColor;
				p_group->List.insert(&item);
			}
			else
				ok = 0;
		}
	}
	return ok;
}
//
//
//
int Rc2Data::SetupBitmapGroup(const char * pPath)
{
	BitmapGroup * p_group = new BitmapGroup;
	p_group->Path = pPath;
	if(p_group->Path.NotEmptyS())
		p_group->Path.ToLower().SetLastSlash().CatChar('\\');
	BitmapList.insert(p_group);
	return 1;
}

int Rc2Data::AddBitmap(const char * pSymbol, SString & rErrMsg)
{
	int    ok = 1;
	SString symbol(pSymbol);
	if(symbol.NotEmptyS()) {
		BitmapGroup * p_group = 0;
		Rc2BitmapItem item;
		MEMSZERO(item);
		{
			uint   c = BitmapList.getCount();
			if(c)
				p_group = BitmapList.at(c-1);
			if(!p_group) {
				if(SetupBitmapGroup(0))
					p_group = BitmapList.at(BitmapList.getCount()-1);
			}
		}
		if(p_group) {
			symbol.ToUpper();
			int    r = AddSymb(DeclareSymb::kBitmap, symbol, &item.SymbID, rErrMsg);
			if(r > 0) {
				SString prefix_buf, body_buf, file_name;
				file_name = (symbol.Divide('_', prefix_buf, body_buf) > 0) ? body_buf : symbol;
				file_name.ToLower();
				SPathStruc::ReplaceExt(file_name, "bmp", 0);
				if(p_group->Path.NotEmpty()) {
					(body_buf = p_group->Path).Cat(file_name);
					file_name = body_buf;
				}
				file_name.CopyTo(item.FileName, sizeof(item.FileName));
				p_group->List.insert(&item);
			}
			else
				ok = 0;
		}
	}
	return ok;
}
//
//
//
int SLAPI Rc2Data::AddCmd(const CmdDefinition * pCmd, SString & rErrMsg)
{
	CmdDefinition * p_cmd = new CmdDefinition(*pCmd);
	if(p_cmd) {
		return (AddSymb(DeclareSymb::kCmd, p_cmd->Name, &p_cmd->ID, rErrMsg) > 0) ? CmdList.insert(p_cmd) : 0;
	}
	else
		return 0;
}

int SLAPI Rc2Data::AddJob(const JobDefinition * pJob, SString & rErrMsg)
{
	JobDefinition * p_job = new JobDefinition(*pJob);
	if(p_job) {
		return (AddSymb(DeclareSymb::kJob, p_job->Name, &p_job->ID, rErrMsg) > 0) ? JobList.insert(p_job) : 0;
	}
	else
		return 0;
}

int SLAPI Rc2Data::AddObj(const ObjDefinition * pObj, SString & rErrMsg)
{
	ObjDefinition * p_obj = new ObjDefinition(*pObj);
	if(p_obj) {
		return (AddSymb(DeclareSymb::kObj, p_obj->Name, &p_obj->ID, rErrMsg) > 0) ? ObjList.insert(p_obj) : 0;
	}
	else
		return 0;
}

int SLAPI Rc2Data::AddView(const ViewDefinition * pView, SString & rErrMsg)
{
	ViewDefinition * p_view = new ViewDefinition(*pView);
	if(p_view) {
		int    r = AddSymb(DeclareSymb::kView, p_view->Name, &p_view->ID, rErrMsg);
		p_view->ID *= 10;
		return (r > 0) ? ViewList.insert(p_view) : 0;
	}
	else
		return 0;
}

int SLAPI Rc2Data::AddReportStub(const ReportStubDefinition * pRep, SString & rErrMsg)
{
	ReportStubDefinition * p_rep = new ReportStubDefinition(*pRep);
	if(p_rep) {
		int    r = AddSymb(DeclareSymb::kReportStub, p_rep->Name, &p_rep->ID, rErrMsg);
		p_rep->ID += 1000;
		return (r > 0) ? RptStubList.insert(p_rep) : 0;
	}
	else
		return 0;
}

int SLAPI Rc2Data::InitCurCtrlMenu(const char * pName)
{
	P_CurCtrlMenuDef = new CtrlMenuDefinition;
	STRNSCPY(P_CurCtrlMenuDef->Name, pName);
	return 1;
}

int SLAPI Rc2Data::AddCtrlMenuItem(const char * pDescr, const char * pKeyCode, const char * pCmdCode) // @v10.8.11 pCmdCode
{
	if(P_CurCtrlMenuDef) {
		CtrlMenuItem item;
		STRNSCPY(item.Descr, pDescr);
		STRNSCPY(item.KeyCode, pKeyCode);
		STRNSCPY(item.CmdCode, pCmdCode); // @v10.8.11
		P_CurCtrlMenuDef->insert(&item);
		return 1;
	}
	else
		return 0;
}

int SLAPI Rc2Data::AcceptCtrlMenu()
{
	if(P_CurCtrlMenuDef) {
		CtrlMenuList.insert(P_CurCtrlMenuDef);
		P_CurCtrlMenuDef = 0;
		return 1;
	}
	else
		return 0;
}

int Rc2Data::SetFieldDefinition(const char * pName, TYPEID typ, long fmt, const char * pDescr, SString & rErrMsg)
{
	if(P_Record == 0) {
		P_Record = new SdRecord;
		if(P_Record == 0) {
			rErrMsg = "Unsiffisient memory for allocation SdRecord";
			return 0;
		}
	}
	SdbField fld;
	fld.Name = pName;
	fld.T.Typ = typ;
	fld.OuterFormat = fmt;
	fld.Descr = pDescr;
	if(P_Record->AddField(0, &fld))
		return 1;
	else {
		rErrMsg.Printf("Can't append field '%s'", pName);
		return 0;
	}
}

int Rc2Data::AddRecord(const char * pName, SString & rErrMsg)
{
	if(P_Record) {
		int    r = AddSymb(DeclareSymb::kRecord, pName, &P_Record->ID, rErrMsg);
		if(r < 0)
			return 0;
		else {
			P_Record->Name = pName;
			RecList.insert(P_Record);
			P_Record = 0; // Функция SetFieldDefinition при добавлении первого
				// поля следующей записи создаст новый объект
			return 1;
		}
	}
	else {
		rErrMsg.Printf("Can't add record '%s': record is not defined", pName);
		return 0;
	}
}

long Rc2Data::ResolveRFileOption(const char * pSymbol, SString & rErrMsg)
{
	if(_stricmp(pSymbol, "TEXT") == 0)
		return PPRFILEF_TEXT;
	else if(_stricmp(pSymbol, "DISTRIB") == 0)
		return PPRFILEF_DISTRIB;
	else if(_stricmp(pSymbol, "UPD") == 0)
		return PPRFILEF_UPD;
	else {
		rErrMsg.Printf("Undefined file option '%s'", pSymbol);
		return 0;
	}
}

long Rc2Data::ResolveRPathSymb(const char * pSymbol, SString & rCppMnem, SString & rErrMsg)
{
	rCppMnem = "PPPATH_";
	if(_stricmp(pSymbol, "BIN") == 0) {
		rCppMnem.Cat("BIN");
		return PPPATH_BIN;
	}
	else if(_stricmp(pSymbol, "DD") == 0) {
		rCppMnem.Cat("DD");
		return PPPATH_DD;
	}
	else if(_stricmp(pSymbol, "LOG") == 0) {
		rCppMnem.Cat("LOG");
		return PPPATH_LOG;
	}
	else if(_stricmp(pSymbol, "LOCAL") == 0) {
		rCppMnem.Cat("LOCAL");
		return PPPATH_LOCAL;
	}
	else if(_stricmp(pSymbol, "WTM") == 0) {
		rCppMnem.Cat("WTM");
		return PPPATH_WTM;
	}
	else if(_stricmp(pSymbol, "PACK") == 0) {
		rCppMnem.Cat("PACK");
		return PPPATH_PACK;
	}
	else if(_stricmp(pSymbol, "IN") == 0) {
		rCppMnem.Cat("IN");
		return PPPATH_IN;
	}
	else if(_stricmp(pSymbol, "OUT") == 0) {
		rCppMnem.Cat("OUT");
		return PPPATH_OUT;
	}
	else if(_stricmp(pSymbol, "TEMP") == 0) {
		rCppMnem.Cat("TEMP");
		return PPPATH_TEMP;
	}
	else if(_stricmp(pSymbol, "SYSROOT") == 0) {
		rCppMnem.Cat("SYSROOT");
		return PPPATH_SYSROOT;
	}
	else if(_stricmp(pSymbol, "DOC") == 0) {
		rCppMnem.Cat("DOC");
		return PPPATH_DOC;
	}
	else {
		rCppMnem = 0;
		rErrMsg.Printf("Undefined path symbol '%s'", pSymbol);
		return 0;
	}
}

int Rc2Data::AddRFileDefinition(const char * pSymbol, const char * pName, const char * pPathMnem, long flags, const char * pDescr, SString & rErrMsg)
{
	int    ok = 1;
	RFileDefinition * p_def = new RFileDefinition;
	if(p_def) {
		p_def->Flags = flags;
		p_def->Symb = pSymbol;
		p_def->Name = pName;
		p_def->PathMnem = pPathMnem;
		p_def->SrcPathMnem = 0;
		p_def->Descr = pDescr;

		int    r = AddSymb(DeclareSymb::kRFile, p_def->Symb, &p_def->ID, rErrMsg);
		ok = (r > 0) ? RFileList.insert(p_def) : 0;
	}
	else
		ok = 0;
	return ok;
}
//
//
//
int SLAPI Rc2Data::GenerateIncludeDirec(FILE * pF, const char * pFileName, int angleBraces)
{
	char   left_brace  = angleBraces ? '<' : '\"';
	char   right_brace = angleBraces ? '>' : '\"';
	fprintf(pF, "%s%c%s%c\n", P_IncludePrefix, left_brace, pFileName, right_brace);
	return 1;
}

int SLAPI Rc2Data::GenerateRCHeader()
{
	fprintf(pRc, "//\n// This file have been generated by RC2 compiler, do not modify it!\n//\n");
	GenerateIncludeDirec(pRc, "slib.h",   1);
	GenerateIncludeDirec(pRc, "tvdefs.h", 1);
	GenerateIncludeDirec(pRc, "ppdefs.h", 1);
	fprintf(pRc, "\n");
	return 1;
}

void SLAPI Rc2Data::GenerateIncHeader()
{
	fprintf(pHdr, "//\n// This file have been generated by RC2 compiler, do not modify it!\n//\n");
	fprintf(pHdr, "\n");
}

int SLAPI Rc2Data::GenerateRecordStruct(const SdRecord * pRec)
{
	SString type_text;
	SdbField fld;
	fprintf(pHdr, "struct Sdr_%s {\n", pRec->Name.cptr());
	fprintf(pHdr, "\tSdr_%s() { THISZERO(); }\n", pRec->Name.cptr()); // @v10.7.9
	for(uint k = 0; k < pRec->GetCount(); k++) {
		if(pRec->GetFieldByPos(k, &fld) > 0) {
			GetBinaryTypeString(fld.T.Typ, 0, type_text, fld.Name, 7);
			fprintf(pHdr, "\t%s;\n", type_text.cptr());
		}
	}
	fprintf(pHdr, "};\n");
	return 1;
}

int SLAPI Rc2Data::GenerateSymbDefinitions()
{
	fprintf(pHdr, "\n");
	if(SymbolList.getCount()) {
		SString symb_kind_name;
		int   prev_kind = -1;
		const char * p_prefix = 0;
		for(uint i = 0; i < SymbolList.getCount(); i++) {
			DeclareSymb & r_symb = SymbolList.at(i);
			if(r_symb.Kind != prev_kind) {
				SymbolList.GetSymbKindName(r_symb.Kind, symb_kind_name);
				if(r_symb.Kind == DeclareSymb::kCommand)
					p_prefix = P_VCmdPrefix;
				else if(r_symb.Kind == DeclareSymb::kJob)
					p_prefix = P_JobPrefix;
				else if(r_symb.Kind == DeclareSymb::kObj)
					p_prefix = P_ObjPrefix;
				else if(r_symb.Kind == DeclareSymb::kCmd)
					p_prefix = P_CmPrefix;
				else if(r_symb.Kind == DeclareSymb::kRecord)
					p_prefix = P_RecPrefix;
				else if(r_symb.Kind == DeclareSymb::kView)
					p_prefix = P_ViewPrefix;
				else if(r_symb.Kind == DeclareSymb::kReportStub)
					p_prefix = P_ReportStubPrefix;
				else if(r_symb.Kind == DeclareSymb::kCtrlMenu)
					p_prefix = P_CtrlMenuPrefix;
				else if(r_symb.Kind == DeclareSymb::kRFile)
					p_prefix = P_RFilePrefix;
				else if(r_symb.Kind == DeclareSymb::kBitmap)
					p_prefix = "";
				else
					p_prefix = "";
				fprintf(pHdr, "//\n// %s definitions\n//\n", symb_kind_name.cptr());
			}
			if(strcmp(r_symb.Symb, "0") != 0) {
				if(r_symb.Kind == DeclareSymb::kView) {
					uint   vp = 0;
					long   symbid = r_symb.ID * 10;
					if(ViewList.lsearch(&symbid, &vp, CMPF_LONG)) {
						const ViewDefinition * p_view = ViewList.at(vp);
						if(p_view) {
							if(p_view->Flags & ViewDefinition::fFilterOnly) {
								fprintf(pHdr, "%s%s%-32s%10d\n", P_DefinePrefix, P_FiltPrefix,     _strupr(r_symb.Symb), symbid+1);
							}
							else {
								fprintf(pHdr, "%s%s%-32s%10d\n",   P_DefinePrefix, p_prefix,         _strupr(r_symb.Symb), symbid);
								fprintf(pHdr, "\t%s%s%-32s%10d\n", P_DefinePrefix, P_FiltPrefix,     _strupr(r_symb.Symb), symbid+1);
								fprintf(pHdr, "\t%s%s%-32s%10d\n", P_DefinePrefix, P_ViewItemPrefix, _strupr(r_symb.Symb), symbid+2);
							}
						}
					}
				}
				else {
					long id = r_symb.ID;
					if(r_symb.Kind == DeclareSymb::kReportStub)
						id += 1000;
					fprintf(pHdr, "%s%s%-32s%10d\n", P_DefinePrefix, p_prefix, _strupr(r_symb.Symb), id);
					if(r_symb.Kind == DeclareSymb::kRecord) {
						const SdRecord * p_rec = 0;
						for(uint j = 0; RecList.enumItems(&j, (void **)&p_rec);) {
							if(p_rec->ID == r_symb.ID) {
								for(uint k = 0; k < p_rec->GetCount(); k++) {
									SdbField fld;
									if(p_rec->GetFieldByPos(k, &fld) > 0)
										fprintf(pHdr, "\t%s%s%s_%-32s%10u\n", P_DefinePrefix, P_FldPrefix, _strupr(r_symb.Symb), fld.Name.ToUpper().cptr(), fld.ID);
								}
								fprintf(pHdr, "\n");
								GenerateRecordStruct(p_rec);
								break;
							}
						}
						fprintf(pHdr, "\n");
					}
				}
			}
			prev_kind = r_symb.Kind;
		}
		fprintf(pHdr, "\n");
	}
	return 1;
}

int SLAPI Rc2Data::GenerateBrowserDefine(char * pName, char * comment)
{
	BrwCounter++;
	fprintf(pHdr, "%s%s%-32s%10d // %s\n", P_DefinePrefix, P_BrwPrefix, _strupr(pName), BrwCounter, comment);
	fprintf(pHdr, "\t%sHELP_%s%-32s%10d\n", P_DefinePrefix, P_BrwPrefix, _strupr(pName), BrwCounter);
	return 1;
}

int SLAPI Rc2Data::LayoutToRect(BrowserLayout * pL, TRect * pR)
{
	const int maxX = 80;
	const int maxY = 23;
	switch(pL->LayoutBase) {
		case BrowserLayout::LayoutNorth:
			pR->a.x = pR->a.y = 0;
			pR->b.x = maxX;
			pR->b.y = maxY * pL->SizeInPercent / 100;
			break;
		case BrowserLayout::LayoutSouth:
			pR->a.x = 0;
			pR->b.x = maxX;
			pR->a.y = maxY - maxY * pL->SizeInPercent / 100;
			pR->b.y = maxY;
			break;
		case BrowserLayout::LayoutWest:
			pR->a.x = pR->a.y = 0;
			pR->b.x = maxX * pL->SizeInPercent / 100;
			pR->b.y = maxY;
			break;
		case BrowserLayout::LayoutEast:
			pR->a.x = maxX - maxX * pL->SizeInPercent / 100;
			pR->b.x = maxX;
			pR->a.y = 0;
			pR->b.y = maxY;
			break;
		case BrowserLayout::LayoutCenter:
			pR->a.x = 0;
			pR->b.x = maxX;
			pR->a.y = maxY * pL->SizeInPercent / 100 / 2;
			pR->b.y = maxY - maxY * pL->SizeInPercent / 100 / 2;
	}
	return 1;
}

int SLAPI Rc2Data::GenerateBrowserDefinition(BrowserDefinition * pB)
{
	TRect r;
	char  flags[256];
	LayoutToRect(&pB->Layout, &r);
	GetFlagsAsString(pB->Flags, tv_browser_flags, flags, sizeof(flags));
	fprintf(pRc, "\n%s%s TV_BROWSER {\n", P_BrwPrefix, _strupr(pB->Name));
	fprintf(pRc, "\t%d, %d, %d, %d, %d, %d, \"%s\\0\", %s, %s\n",
		r.a.x, r.a.y, r.b.x, r.b.y, pB->Height, pB->Freeze, pB->Header, flags, pB->HelpID);

	uint current_group = 0;
	BrowserColumn * p_b;
	for(uint i = 0; pB->Columns.enumItems(&i, (void **)&p_b);) {
		if(pB->Groups.getCount() > current_group) {
			GroupDefinition * p_g = pB->Groups.at(current_group);
			if(p_g->startColumn == i - 1)
				fprintf(pRc, "\tTV_BROGROUP \"%s\\0\", 1\n", p_g->Name);
			if(p_g->startColumn <= (int)(i - 1))
				fprintf(pRc, "\t");
		}
		int size = GETSSIZE(p_b->Type);
		int prec = (GETSTYPE(p_b->Type) == S_FLOAT) ? GETSPRECD(p_b->Type) : 0;
		if(p_b->IsCrosstab)
			fprintf(pRc, "\tTV_CROSSTAB \"%s\\0\", 0, %s, %d, %d, %s, %d, %d\n",
				p_b->Name,
				p_b->Type ? GetSTypeName(p_b->Type) : "0",
				size,
				prec,
				p_b->Flags,
				p_b->Width,
				p_b->Prec);
		else
			fprintf(pRc, "\tTV_BROCOLUMN \"%s\\0\", %d, %s, %s, %d, %d, %s, %d, %d, \"%s\\0\"\n",
				p_b->Name,
				atoi(p_b->ReqNumber),
				p_b->Options,
				p_b->Type ? GetSTypeName(p_b->Type) : "0",
				size,
				prec,
				p_b->Flags,
				p_b->Width,
				p_b->Prec,
				p_b->ReqNumber);

		if(pB->Groups.getCount() > current_group) {
			GroupDefinition * p_g = pB->Groups.at(current_group);
			if(p_g->stopColumn == i - 1) {
				fprintf(pRc, "\tTV_END\n");
				current_group++;
			}
		}
	}
	GenerateToolbarDefinition(&pB->Toolbar);
	fprintf(pRc, "};\n");
	return 1;
}

int SLAPI Rc2Data::GenerateToolbarDefine(char * pName)
{
	TbCounter++;
	fprintf(pHdr, "%s%s%-26s%6d\n", P_DefinePrefix, P_TbPrefix, _strupr(pName), TbCounter);
	return 1;
}

int SLAPI Rc2Data::GenerateToolbarEntries(FILE * pF, ToolbarDefinition * pT)
{
	Rc2ToolbarItem * p_t;
	for(uint i = 0; pT->enumItems(&i, (void **)&p_t);)
		if(p_t->Flags & Rc2ToolbarItem::fSeparator)
			fprintf(pF, "\t\tTV_MENUSEPARATOR,\n");
		else {
			char   cmd_symb[64];
			if(SymbolList.SearchSymbByID(DeclareSymb::kCommand, p_t->Cmd, cmd_symb, sizeof(cmd_symb))) {
				char   temp_buf[128];
				STRNSCPY(temp_buf, P_VCmdPrefix);
				strcat(temp_buf, cmd_symb);
				STRNSCPY(cmd_symb, temp_buf);
			}
			else {
				cmd_symb[0] = '0';
				cmd_symb[1] = 0;
			}
			fprintf(pF, "\t\t%s, %s, 0x%04lX, %s, \"%s\\0\"\n", cmd_symb,
				p_t->KeyCode, p_t->Flags, p_t->BitmapIndex, p_t->ToolTipText);
		}
	return 1;
}

int SLAPI Rc2Data::GenerateToolbarDefinition(ToolbarDefinition * pT)
{
	if(pT->IsLocal) {
		if(*pT->Name)
			fprintf(pRc, "\tTV_IMPTOOLBAR %s%s\n", P_TbPrefix, _strupr(pT->Name));
		else if(pT->getCount()) {
			fprintf(pRc, "\tTV_TOOLBAR %s\n", pT->BitmapIndex);
			GenerateToolbarEntries(pRc, pT);
			fprintf(pRc, "\tTV_END\n");
		}
	}
	else {
		if(pT->IsExport)
			fprintf(pRc, "%s%s TV_EXPTOOLBAR {\n\t%s,\n", P_TbPrefix, _strupr(pT->Name), pT->BitmapIndex);
		else
			fprintf(pRc, "%s%s TV_GLBTOOLBAR {\n\t%s,\n", P_TbPrefix, _strupr(pT->Name), pT->BitmapIndex);
		GenerateToolbarEntries(pRc, pT);
		fprintf(pRc, "\tTV_END\n};\n");
	}
	return 1;
}

int SLAPI Rc2Data::GenerateCmdDefinitions()
{
	SString view_symb(1), filt_symb(1), filt_ext_symb(1);
	for(uint i = 0; i < CmdList.getCount(); i++) {
		CmdDefinition * p_cmd = CmdList.at(i);
		char   name[128];
		STRNSCPY(name, p_cmd->Name);
		fprintf(pRc, "\n%s%s PP_RCDECLCMD {\n", P_CmPrefix, _strupr(name));
		strip(p_cmd->IconIdent);
		if(_stricmp(p_cmd->IconIdent, "none") == 0) {
			p_cmd->IconIdent[0] = '0';
			p_cmd->IconIdent[1] = 0;
		}
		strip(p_cmd->ToolbarIdent);
		if(_stricmp(p_cmd->ToolbarIdent, "none") == 0) {
			p_cmd->ToolbarIdent[0] = '0';
			p_cmd->ToolbarIdent[1] = 0;
		}
		strip(p_cmd->MenuCmdIdent);
		if(_stricmp(p_cmd->MenuCmdIdent, "none") == 0) {
			p_cmd->MenuCmdIdent[0] = '0';
			p_cmd->MenuCmdIdent[1] = 0;
		}
		view_symb.Z();
		filt_symb.Z();
		filt_ext_symb.Z();
		strip(p_cmd->Filt.FiltSymb);
		if(strip(p_cmd->Filt.FiltSymb)[0]) {
			if(p_cmd->Filt.DeclView)
				(view_symb = "PPVIEW_").Cat(p_cmd->Filt.FiltSymb).ToUpper();
			else
				(filt_symb = "PPFILT_").Cat(p_cmd->Filt.FiltSymb).ToUpper();
			filt_ext_symb = strip(p_cmd->Filt.FiltExtraSymb);
		}
		if(view_symb.Empty())
			view_symb.CatChar('0');
		if(filt_symb.Empty())
			filt_symb.CatChar('0');
		if(filt_ext_symb.Empty())
			filt_ext_symb.CatChar('0');
		fprintf(pRc, "\t\"%s\\0\", \"%s\\0\", %s,\n\t%s, %s, 0x%08lX, %s, %s, %s\n",
			p_cmd->Name, p_cmd->Descr, p_cmd->IconIdent,
			p_cmd->ToolbarIdent, p_cmd->MenuCmdIdent, p_cmd->Flags, view_symb.cptr(), filt_symb.cptr(), filt_ext_symb.cptr());
		fprintf(pRc, "};\n");
	}
	return 1;
}

int SLAPI Rc2Data::GenerateJobDefinitions()
{
	for(uint i = 0; i < JobList.getCount(); i++) {
		JobDefinition * p_job = JobList.at(i);
		char name[128];
		STRNSCPY(name, p_job->Name);
		fprintf(pRc, "\n%s%s PP_RCDECLJOB {\n", P_JobPrefix, _strupr(name));
		fprintf(pRc, "\t\"%s\\0\", \"%s\\0\", 0x%08lX\n", p_job->Name, p_job->Descr, p_job->Flags);
		fprintf(pRc, "};\n");
	}
	return 1;
}

int SLAPI Rc2Data::GenerateObjDefinitions()
{
	for(uint i = 0; i < ObjList.getCount(); i++) {
		ObjDefinition * p_obj = ObjList.at(i);
		char name[128];
		STRNSCPY(name, p_obj->Name);
		fprintf(pRc, "\n%s%s PP_RCDECLOBJ {\n", P_ObjPrefix, _strupr(name));
		fprintf(pRc, "\t\"%s\\0\", \"%s\\0\"\n", p_obj->Name, p_obj->Descr);
		fprintf(pRc, "};\n");
	}
	return 1;
}

int SLAPI Rc2Data::GenerateRecDefinitions()
{
	SString name;
	for(uint i = 0; i < RecList.getCount(); i++) {
		const SdRecord * p_rec = RecList.at(i);
		(name = p_rec->Name).ToUpper();
		fprintf(pRc, "\n%s%s PP_RCDECLRECORD {\n", P_RecPrefix, name.cptr());
		fprintf(pRc, "\t\"%s\\0\", %u,\n", p_rec->Name.cptr(), p_rec->GetCount());
		for(uint j = 0; j < p_rec->GetCount(); j++) {
			SdbField fld;
			p_rec->GetFieldByPos(j, &fld);
			const int typ = GETSTYPE(fld.T.Typ);
			const int size = GETSSIZE(fld.T.Typ);
			const int prec = (GETSTYPE(fld.T.Typ) == S_FLOAT) ? GETSPRECD(fld.T.Typ) : 0;
			const uint fmtflg = SFMTFLAG(fld.OuterFormat);
			const uint fmtlen = SFMTLEN(fld.OuterFormat);
			const uint fmtprc = SFMTPRC(fld.OuterFormat);
			//   FldID, "FldName\0", FldType, FldSize, FldPrec, FldFormatLen, FldFormatPrec, FldFormatFlags, "FldDescr\0"
			fld.Name.CatChar('\\').CatChar('0').Quot('\"', '\"');
			fld.Descr.CatChar('\\').CatChar('0').Quot('\"', '\"');
			fprintf(pRc, "\t%u, %s, %s, %d, %d, %d, %d, %d, %s\n", fld.ID, fld.Name.cptr(),
				typ ? GetSTypeName(fld.T.Typ) : "0", size, prec, fmtlen, fmtprc, fmtflg, fld.Descr.cptr());
			// fprintf(pRc, "\t%u, %s, 0x%lX, 0x%lX, %s\n", fld.ID, (const char *)fld.Name, fld.T.Typ, fld.OuterFormat, (const char *)fld.Descr);
		}
		fprintf(pRc, "};\n");
	}
	return 1;
}

int SLAPI Rc2Data::GenerateReportStubDefinitions()
{
	SString name;
	for(uint i = 0; i < RptStubList.getCount(); i++) {
		const ReportStubDefinition * p_def = RptStubList.at(i);
		(name = p_def->Name).ToUpper();
		fprintf(pRc, "\n%s%s PP_RCDECLREPORTSTUB { \"%s\\0\", \"%s\\0\", \"%s\\0\" }\n",
			P_ReportStubPrefix, name.cptr(), p_def->Name.cptr(), p_def->Data.cptr(), p_def->Descr.cptr());
	}
	return 1;
}

int SLAPI Rc2Data::GenerateViewDefinitions()
{
	//#define PP_RCDECLVIEW         508   // Тип ресурса для декларации КАД (PPView)
	//
	// PPVIEW_XXX PP_RCDECLVIEW { "Symb\0", "Descript\0" }
	//
	//#define PP_RCDECLFILT         509   // Тип ресурса для декларации фильтра (PPBaseFilt)
	//
	// PPFILT_XXX PP_RCDECLFILT { "Symb\0" }
	//
	SString temp_buf, symb;
	for(uint i = 0; i < ViewList.getCount(); i++) {
		const ViewDefinition * p_def = ViewList.at(i);
		(symb = p_def->Name).ToUpper();
		if(!(p_def->Flags & ViewDefinition::fFilterOnly)) {
			temp_buf = 0;
			temp_buf.CR().Cat(P_ViewPrefix).Cat(symb).Space().Cat("PP_RCDECLVIEW").
				CatDiv('{', 1).CatChar('\"').Cat(p_def->Name).Cat("\\0\"").CatDiv(',', 2).
				CatChar('\"').Cat(p_def->Descr).Cat("\\0\"").Space().CatChar('}').CR();
			fprintf(pRc, temp_buf.cptr());
		}
		temp_buf = 0;
		temp_buf.Cat(P_FiltPrefix).Cat(symb).Space().Cat("PP_RCDECLFILT").
			CatDiv('{', 1).CatChar('\"').Cat(p_def->Name).Cat("\\0\"").Space().CatChar('}').CR();
		fprintf(pRc, temp_buf.cptr());
	}
	return 1;
}

int SLAPI Rc2Data::GenerateCtrlMenuDefinitions()
{
	//
	// PPCTRLMENU_XXX PP_RCDECLCTRLMENU { items_count,
	//	"Text\0", keyCode, cmdCode
	//	"Text\0", keyCode, cmdCode
	// }
	//
	SString temp_buf, symb;
	for(uint i = 0; i < CtrlMenuList.getCount(); i++) {
		const CtrlMenuDefinition * p_def = CtrlMenuList.at(i);
		(symb = p_def->Name).ToUpper();
		temp_buf.CR();
		temp_buf.Cat(P_CtrlMenuPrefix).Cat(symb).Space().Cat("PP_RCDECLCTRLMENU").Space().CatChar('{').
			Space().Cat(p_def->getCount()).Comma().CR();
		for(uint j = 0; j < p_def->getCount(); j++) {
			const CtrlMenuItem & r_item = p_def->at(j);
			temp_buf.CatChar('\t').CatChar('\"').Cat(r_item.Descr).Cat("\\0\"").CatDiv(',', 2).Cat(r_item.KeyCode);
			// @v10.8.11 {
			if(!isempty(r_item.CmdCode)) {
				temp_buf.CatDiv(',', 2).Cat(r_item.CmdCode);
			}
			else
				temp_buf.CatDiv(',', 2).Cat("0");
			// } @v10.8.11 
			if(j < (p_def->getCount()-1))
				temp_buf.Comma();
			temp_buf.CR();
		}
		temp_buf.CatChar('}').CR();
	}
	fprintf(pRc, temp_buf.cptr());
	return 1;
}

int SLAPI Rc2Data::GenerateDrawVectorFile(const char * pStorageFileName)
{
#define DO_CREATE_VECT_STORAGE 1

	const char * p_whtm_obj_symb = "DrawFigure";

	int    ok = -1;
	if(DrawVectorList.getCount()) {
#if DO_CREATE_VECT_STORAGE
		TWhatmanToolArray tool_array;
#endif
		for(uint grp_idx = 0; grp_idx < DrawVectorList.getCount(); grp_idx++) {
			const DrawVectorGroup * p_grp = DrawVectorList.at(grp_idx);
			if(p_grp) {
#if DO_CREATE_VECT_STORAGE
				for(uint item_idx = 0; item_idx < p_grp->List.getCount(); item_idx++) {
					const Rc2DrawVectorItem & r_item = p_grp->List.at(item_idx);
					char   symb[128];
					if(SymbolList.SearchSymbByID(DeclareSymb::kDrawVector, r_item.SymbID, symb, sizeof(symb))) {
						TWhatmanToolArray::Item tool_item;
						tool_item.Id = r_item.SymbID;
						tool_item.WtmObjSymb = p_whtm_obj_symb;
						tool_item.Text = symb;
						tool_item.Symb = symb;
						tool_item.FigPath = r_item.FileName;
						tool_item.PicSize = 32;
						tool_item.FigSize = 64;
						tool_item.ReplacedColor = r_item.ReplacedColor; // @v9.2.7
						THROW(tool_array.Set(tool_item, 0));
						ok = 1;
					}
				}
#endif
			}
		}
		if(ok > 0) {
			if(!isempty(pStorageFileName)) {
#if DO_CREATE_VECT_STORAGE // @construction
				THROW(tool_array.Store(pStorageFileName));
#endif
			}
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI Rc2Data::GenerateBitmapDefinitions()
{
	// IDB_BG_PAPYRUS          BITMAP                  "..\\rsrc\\bitmap\\bg-papyrus.bmp"
	if(BitmapList.getCount()) {
		SFile emb_file(EmbedFileName, SFile::mWrite);
		if(emb_file.IsValid()) {
			SString line_buf;
			char   symb[128];
			//
			fprintf(emb_file, "//\n// This file have been generated by RC2 compiler, do not modify it!\n//\n");
			// @v9.1.12 GenerateIncludeDirec(emb_file, "slib.h",   1);
			GenerateIncludeDirec(emb_file, "tvdefs.h", 1);
			GenerateIncludeDirec(emb_file, "ppdefs.h", 1);
			fprintf(emb_file, "\n");
			//
			for(uint i = 0; i < BitmapList.getCount(); i++) {
				const BitmapGroup * p_group = BitmapList.at(i);
				if(p_group) {
					for(uint j = 0; j < p_group->List.getCount(); j++) {
						const Rc2BitmapItem & r_item = p_group->List.at(j);
						if(SymbolList.SearchSymbByID(DeclareSymb::kBitmap, r_item.SymbID, symb, sizeof(symb))) {
							(line_buf = 0).Cat(symb).Align(28, ADJ_LEFT).Cat("BITMAP").Align(54, ADJ_LEFT).CatQStr(r_item.FileName).CR();
							emb_file.WriteLine(line_buf);
						}
					}
					emb_file.WriteLine((line_buf = 0).CR());
				}
			}
		}
	}
	return 1;
}

int SLAPI Rc2Data::GenerateRFileDefinitions()
{
// #define PP_RCDECLRFILE        511   // Тип ресурса для декларации зарезервированных файлов
	//
	// PPRFILE_XXX PP_RCDECLRFILE { "Symb\0", "file_name", PPPATH_XXX(1), PPPATH_XXX(2), flags, "Descript\0" }
	// Мнемоника PPPATH_XXX(1) определяет местонахождение файла в установленном дистрибутиве
	// Мнемоника PPPATH_XXX(2) определяет исходное местонахождение файла при сборке дистрибутива (reserved)
	//

	SString temp_buf, symb, src_mnem;
	for(uint i = 0; i < RFileList.getCount(); i++) {
		const RFileDefinition * p_def = RFileList.at(i);
		(symb = p_def->Symb).ToUpper();
		src_mnem = p_def->SrcPathMnem;
		if(!src_mnem.NotEmptyS())
			src_mnem = "0";
		(temp_buf = 0).Cat(P_RFilePrefix).Cat(symb).Space().Cat("PP_RCDECLRFILE").
			CatDiv('{', 1).
				CatChar('\"').Cat(p_def->Symb).Cat("\\0\"").CatDiv(',', 2).
				CatChar('\"').Cat(p_def->Name).Cat("\\0\"").CatDiv(',', 2).
				Cat(p_def->PathMnem).CatDiv(',', 2).
				Cat(src_mnem).CatDiv(',', 2).
				CatHex(p_def->Flags).CatDiv(',', 2).
				CatChar('\"').Cat(p_def->Descr).Cat("\\0\"").
			CatChar('}').CR();
		fprintf(pRc, temp_buf.cptr());
	}
	return 1;
}
//
//
//
IMPL_CMPFUNC(DeclareSymb_ID, i1, i2)
{
	int    si;
	CMPCASCADE2(si, static_cast<const DeclareSymb *>(i1), static_cast<const DeclareSymb *>(i2), Kind, ID);
	return si;
}

IMPL_CMPFUNC(DeclareSymb_Symb, i1, i2)
{
	const DeclareSymb * s1 = static_cast<const DeclareSymb *>(i1);
	const DeclareSymb * s2 = static_cast<const DeclareSymb *>(i2);
	if(s1->Kind < s2->Kind)
		return -1;
	else if(s1->Kind > s2->Kind)
		return 1;
	else
		return CMPFUNC(PcharNoCase, s1->Symb, s2->Symb);
}

DeclareSymbList::DeclareSymbList() : TSArray<DeclareSymb> (1)
{
}

SString & DeclareSymbList::GetSymbKindName(int kind, SString & rBuf) const
{
	switch(kind) {
		case DeclareSymb::kCommand: rBuf = "PPVIEW COMMAND"; break;
		case DeclareSymb::kJob: rBuf = "PPJOB"; break;
		case DeclareSymb::kObj: rBuf = "PPOBJECT"; break;
		case DeclareSymb::kCmd: rBuf = "PPCMD"; break;
		case DeclareSymb::kRecord: rBuf = "RECORD"; break;
		case DeclareSymb::kView: rBuf = "PPVIEW"; break;
		case DeclareSymb::kReportStub: rBuf = "REPORT"; break;
		case DeclareSymb::kCtrlMenu: rBuf = "CTRLMENU"; break;
		case DeclareSymb::kBitmap: rBuf = "BITMAP"; break;
		case DeclareSymb::kDrawVector: rBuf = "DRAWVECTOR"; break;
		case DeclareSymb::kRFile: rBuf = "FILE"; break;
		default: rBuf = "UNKNOWN"; break;
	}
	return rBuf;
}

int DeclareSymbList::AddSymb(int kind, const char * pSymb, long * pID)
{
	int    ok = 0;
	DeclareSymb s;
	MEMSZERO(s);
	s.Kind = kind;
	STRNSCPY(s.Symb, pSymb);
	s.ID = *pID;
	uint   pos = 0;
	if(lsearch(&s, &pos, PTR_CMPFUNC(DeclareSymb_Symb))) {
		ASSIGN_PTR(pID, at(pos).ID);
		ok = -1;
	}
	else {
		long last_id = 0;
		if(!LastIdList.Search(kind, &last_id, 0)) {
			if(kind == DeclareSymb::kCmd)
				last_id = 1000;
			// @v10.5.0 {
			else if(kind == DeclareSymb::kDrawVector) 
				last_id = 0x8000;
			// } @v10.5.0 
			else
				last_id = 0;
		}
		if(*pID)
			if(lsearch(&s, &(pos = 0), PTR_CMPFUNC(DeclareSymb_ID)))
				ok = 0;
			else {
				if(s.ID > last_id)
					LastIdList.Update(kind, s.ID);
				ok = insert(&s) ? 1 : 0;
			}
		else {
			for(long id = last_id+1; ; id++) {
				s.ID = id;
				if(!lsearch(&s, &(pos = 0), PTR_CMPFUNC(DeclareSymb_ID))) {
					LastIdList.Update(kind, id);
					ASSIGN_PTR(pID, id);
					ok = insert(&s) ? 1 : 0;
					break;
				}
			}
		}
	}
	return ok;
}

int DeclareSymbList::SearchSymb(int kind, const char * pSymb, uint * pPos)
{
	DeclareSymb s;
	MEMSZERO(s);
	s.Kind = kind;
	STRNSCPY(s.Symb, pSymb);
	uint pos = 0;
	if(lsearch(&s, &pos, PTR_CMPFUNC(DeclareSymb_Symb))) {
		ASSIGN_PTR(pPos, pos);
		return 1;
	}
	return 0;
}

int DeclareSymbList::SearchSymbByID(int kind, long id, char * pBuf, size_t bufLen)
{
	DeclareSymb s;
	MEMSZERO(s);
	s.Kind = kind;
	s.ID = id;
	uint   pos = 0;
	if(lsearch(&s, &pos, PTR_CMPFUNC(DeclareSymb_ID))) {
		strnzcpy(pBuf, at(pos).Symb, bufLen);
		return 1;
	}
	if(pBuf)
		pBuf[0] = 0;
	return 0;
}
//
//
//
Rc2ToolbarItem & Rc2ToolbarItem::InitSeparator()
{
	THISZERO();
	Flags |= fSeparator;
	STRNSCPY(KeyCode, "TV_MENUSEPARATOR");
	return *this;
}

Rc2ToolbarItem & Rc2ToolbarItem::Init(const char * pKeyCode, const char * pToolTip, const char * pBitmap)
{
	THISZERO();
	STRNSCPY(KeyCode, pKeyCode);
	STRNSCPY(ToolTipText, pToolTip);
	STRNSCPY(BitmapIndex, pBitmap);
	return *this;
}

Rc2ToolbarItem & Rc2ToolbarItem::Init(long cmd, long flags, const char * pKeyCode, const char * pBitmap, const char * pToolTip)
{
	THISZERO();
	Flags |= flags;
	Cmd = cmd;
	STRNSCPY(KeyCode, pKeyCode);
	STRNSCPY(BitmapIndex, pBitmap);
	STRNSCPY(ToolTipText, pToolTip);
	return *this;
}
//
//
//
void main(int argc, char ** argv)
{
	SString temp_buf;
	if(argc <= 1)
		printf("Usage: %s src[.rc2] [result.rc] [result.h]\n", argv[0]);
	else {
		SLS.Init("RC2");
		// @v10.5.6 char   rc2_name[MAXPATH];
		// @v10.5.6 char   rc_name[MAXPATH];
		// @v10.5.6 char   h_name[MAXPATH];
		SString rc2_name; // @v10.5.6
		SString rc_name;  // @v10.5.6
		SString h_name;  // @v10.5.6
		char   embed_rc_name[MAXPATH];
		SString drawvector_storage_name;
		// @v10.5.6 STRNSCPY(rc2_name, argv[1]);
		// @v10.5.6 replaceExt(rc2_name, "rc2", 0);
		rc2_name = argv[1]; // @v10.5.6 
		SPathStruc::ReplaceExt(rc2_name, "rc2", 0); // @v10.5.6 
		/* @v10.5.6 
		if(argc >= 3)
			STRNSCPY(rc_name,  argv[2]);
		else {
			STRNSCPY(rc_name,  argv[1]);
			replaceExt(rc_name, "rc", 1);
		} 
		if(argc >= 4)
			STRNSCPY(h_name,  argv[3]);
		else {
			STRNSCPY(h_name,  argv[1]);
			replaceExt(h_name, "h", 1);
		}
		*/
		// @v10.5.6 {
		if(argc >= 3)
			rc_name = argv[2];
		else {
			rc_name = argv[1];
			SPathStruc::ReplaceExt(rc_name, "rc", 1);
		}
		if(argc >= 4)
			h_name = argv[3];
		else {
			h_name = argv[1];
			SPathStruc::ReplaceExt(h_name, "h", 1);
		}
		// } @v10.5.6 
		if(argc >= 5) {
			STRNSCPY(embed_rc_name, argv[4]);
		}
		else {
			SPathStruc ps;
			ps.Split(rc_name);
			ps.Nam.Cat("-embed");
			ps.Merge(temp_buf);
			temp_buf.CopyTo(embed_rc_name, sizeof(embed_rc_name));
		}
		//
		{
			SPathStruc ps;
			ps.Split(rc_name);
			ps.Nam.Cat("-dv");
			ps.Ext = "wta";
			ps.Merge(drawvector_storage_name);
		}
		yyin = fopen(rc2_name, "r");
		if(!yyin)
			printf("cant open file %s\n", rc2_name.cptr());
		else {
			RegisterBIST();
			if(Rc2.Init(rc2_name, rc_name, h_name, embed_rc_name)) {
				Rc2.GenerateRCHeader();
				Rc2.GenerateIncHeader();
				yyparse();
				Rc2.GenerateRecDefinitions();
				Rc2.GenerateJobDefinitions();
				Rc2.GenerateObjDefinitions();
				Rc2.GenerateCmdDefinitions();
				Rc2.GenerateSymbDefinitions();
				Rc2.GenerateReportStubDefinitions();
				Rc2.GenerateViewDefinitions();
				Rc2.GenerateCtrlMenuDefinitions();
				Rc2.GenerateBitmapDefinitions();
				Rc2.GenerateDrawVectorFile(drawvector_storage_name); // @v9.1.1
				Rc2.GenerateRFileDefinitions();
			}
		}
	}
}
