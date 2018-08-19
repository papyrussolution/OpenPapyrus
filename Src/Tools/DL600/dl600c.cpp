// DL600C.CPP
// Copyright (c) A.Sobolev 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2014, 2015, 2016, 2017
// Compile-time DL600 modules
//
#include <pp.h>
#include <dl600.h>
#include "dl600c.tab.cpp.h"

// Строка отладки для интерфейсов: ../rsrc/dl600/ppifc.dl6
// Строка отладки для структуры БД: /dict:..\..\BASE\INIT_DL6 /oracle ..\rsrc\dl600\ppdbs.dl6
//   /dict:..\..\base\init_dl6 /data:..\..\base\init_dl6 /oracle ..\rsrc\dl600\ppdbs.dl6
// Строка отладки для экспортных структур: ..\rsrc\dl600\ppexp.dl6
// Строка отладки для диалогов: ..\rsrc\dl600\ppdlg.dl6 /d

//
//
// На интерфейсы генерируется:
// .h
// .cpp
// .idl
//
DlContext DCtx(1); // @global

int DlContext::LastError; // @global
SString DlContext::AddedMsgString; // @global
//
// Функция замещает общую PPSetError, определенную в PPLIB\PPMSG.CPP
//
int FASTCALL PPSetError(int errCode)
{
	DCtx.SetError(errCode);
	return 1;
}

int PPSetErrorNoMem() { return PPSetError(PPERR_NOMEM); }
int PPSetErrorSLib() { return PPSetError(PPERR_SLIB); }
int PPSetErrorInvParam() { return PPSetError(PPERR_INVPARAM); }
//
//
//
int CtmToken::Init()
{
	Code = 0;
	MEMSZERO(U);
	return 1;
}

void CtmToken::Destroy()
{
	if(oneof4(Code, T_IDENT, T_AT_IDENT, T_CONST_STR, T_FMT))
		delete U.S;
	Code = 0;
	MEMSZERO(U);
}

int CtmToken::Create(uint code)
{
	Init();
	Code = code;
	return Code;
}

int CtmToken::Create(uint code, const char * pStr)
{
	Init();
	Code = code;
	if(Code == T_CONST_STR)
		U.S = newStr(pStr);
	else if(Code == T_FMT)
		U.S = newStr(pStr);
	else if(Code == T_CONST_INT)
		U.I = strtol(pStr, 0, 0);
	else if(Code == T_CONST_REAL)
		U.FD = atof(pStr);
	else if(Code == T_CONST_UUID) {
		if(!U.Uuid.FromStr(pStr))
			Code = 0;
	}
	else
		U.S = newStr(pStr);
	return Code;
}

int CtmToken::Create(uint code, DLSYMBID id)
{
	Init();
	Code = code;
	U.ID = id;
	return Code;
}
//
//
//
void CtmDclr::Init()
{
	Tok.Init();
	DimList.Init();
	PtrList.Init();
	TypeID = 0;
	IfaceArgDirMod = 0;
	Reserve = 0;
}

void CtmDclr::Destroy()
{
	Tok.Destroy();
	DimList.Destroy();
	PtrList.Destroy();
	Alias.Destroy();
	Format.Destroy();
}

int CtmDclr::Copy(const CtmDclr & rDclr)
{
	Init();
	Tok = rDclr.Tok;
	DimList.Copy(rDclr.DimList);
	PtrList.Copy(rDclr.PtrList);
	TypeID = rDclr.TypeID;
	IfaceArgDirMod = rDclr.IfaceArgDirMod;
	Reserve = rDclr.Reserve;
	return 1;
}

int CtmDclr::AddDim(uint dim)
{
	DimList.Add(dim);
	return 1;
}
//
//
//
union DL600_TempDecDim {
	struct I {
		uint8  dec;        //
		uint8  prec;       //
		uint16 signature;  // 0xf77f
	};
	I      Dd;
	uint32 V;
};

uint32 SetDecimalDim(uint dec, uint precision)
{
	DL600_TempDecDim d;
	d.V = 0;
	d.Dd.signature = 0xf77f;
	if(dec > 20 || dec < 1)
		dec = 8;
	if(precision > 12)
		precision = 2;
	d.Dd.dec = (uint8)dec;
	d.Dd.prec = (uint8)precision;
	return d.V;
}

int GetDecimalDim(uint32 dim, uint * pDec, uint * pPrec)
{
	DL600_TempDecDim d;
	d.V = dim;
	if(d.Dd.signature == 0xf77f) {
		ASSIGN_PTR(pDec, d.Dd.dec);
		ASSIGN_PTR(pPrec, d.Dd.prec);
		return 1;
	}
	else
		return 0;
}

int CtmDclr::AddDecimalDim(uint dec, uint precision)
{
	uint   dim = SetDecimalDim(dec, precision);
	DimList.Add(dim);
	return 1;
}

int CtmDclr::AddPtrMod(uint ptrMod, uint modifier)
{
	PtrList.Add((uint32)MakeLong(ptrMod, modifier));
	return 1;
}
//
//
//
void CtmDclrList::Init()
{
	P_List = 0;
}

void CtmDclrList::Destroy()
{
	uint c = P_List ? P_List->getCount() : 0;
	if(c) do {
		CtmDclr * p_dclr = P_List->at(--c);
		CALLPTRMEMB(p_dclr, Destroy());
	} while(c);
	ZDELETE(P_List);
}

int CtmDclrList::Add(const CtmDclr & rDclr)
{
	SETIFZ(P_List, new TSCollection <CtmDclr>);
	CtmDclr * p_dclr = new CtmDclr;
	p_dclr->Copy(rDclr);
	P_List->insert(p_dclr);
	return 1;
}
//
//
//
void CtmFuncDclr::Init()
{
	P_Fn = new DlFunc;
}

void CtmFuncDclr::Destroy()
{
	ZDELETE(P_Fn);
}
//
//
//
SLAPI DlMacro::DlMacro() : S(";")
{
}

int SLAPI DlMacro::Add(const char * pSymb, const char * pResult)
{
	SString buf;
	S.add(buf.Cat(pSymb).Comma().Cat(pResult), 0);
	return 1;
}

int SLAPI DlMacro::Subst(const char * pSymb, SString & rResult) const
{
	SString pat = pSymb;
	pat.Comma();
	for(uint pos = 0; S.get(&pos, rResult) > 0;)
		if(rResult.CmpPrefix(pat, 0) == 0) {
			rResult.ShiftLeft(pat.Len());
			return 1;
		}
	rResult = 0;
	return 0;
}
//
//
//
int DlContext::AddMacro(const char * pMacro, const char * pResult)
{
	SETIFZ(P_M, new DlMacro);
	return P_M->Add(pMacro, pResult);
}

int DlContext::GetMacro(const char * pMacro, SString & rResult) const
{
	return P_M ? P_M->Subst(pMacro, rResult) : 0;
}
//
//
//
int DlContext::AddStrucDeclare(const char * pDecl)
{
	CurDeclList.add(pDecl, 0);
	return 1;
}

int SLAPI DlContext::AddType(const char * pName, TYPEID stypId, char mangleC)
{
	int    ok = 1;
	DLSYMBID id = CreateSymb(pName, '@', crsymfErrorOnDup);
	if(id) {
		TypeEntry entry;
		MEMSZERO(entry);
		entry.SymbID = id;
		entry.T.Typ  = stypId;
		entry.MangleC = mangleC;
		TypeList.ordInsert(&entry, 0, PTR_CMPFUNC(uint));
	}
	else
		ok = 0;
	return ok;
}

int DlContext::UnrollType(DLSYMBID typeID, const STypEx & rTyp, TypeDetail * pTd)
{
	int    ok = -1;
	STypEx term_t = rTyp;
	DLSYMBID term_type_id = typeID;
	while(term_t.Flags & STypEx::fOf) {
		TypeEntry te;
		THROW(SearchTypeID(term_t.Link, 0, &te));
		if(oneof2(term_t.Mod, STypEx::modPtr, STypEx::modRef))
			pTd->PtrList.Add((uint32)MakeLong(term_t.Mod, 0));
		else if(term_t.Mod == STypEx::modArray)
			pTd->DimList.Add(term_t.Dim);
		term_t = te.T;
		term_type_id = term_t.Link;
	}
	pTd->T = term_t;
	pTd->TerminalTypeID = term_type_id;
	CATCHZOK
	return ok;
}

int SLAPI DlContext::AddTypedef(const CtmToken & rSymb, DLSYMBID typeID, uint tdFlags)
{
	EXCEPTVAR(LastError);
	int    ok = 1;
	uint   fld_id = 0;
	uint   pos = 0;
	SdbField fld;
	DLSYMBID new_type_id = 0;
	TypeEntry te, new_te;
	DlScope * p_cur_scope = 0, * p_scope = 0;
	THROW(p_cur_scope = GetScope(CurScopeID));
	p_scope = p_cur_scope->SearchByName(DlScope::kTypedefPool, "typedef", 0);
	if(p_scope == 0) {
		THROW_V(p_scope = new DlScope(GetNewSymbID(), DlScope::kTypedefPool, "typedef", 0), PPERR_NOMEM);
		THROW(p_cur_scope->Add(p_scope));
	}
	THROW(SearchTypeID(typeID, &(pos = 0), &te));
	fld.T.Init();
	fld.T = te.T;
	fld.Name = rSymb.U.S;
	if(p_scope->SearchName(fld.Name, &(pos = 0), 0)) {
		AddedMsgString = 0;
		Sc.GetQualif(CurScopeID, "::", 0, AddedMsgString);
		if(AddedMsgString.NotEmpty())
			AddedMsgString.Cat("::");
		AddedMsgString.Cat(fld.Name);
		CALLEXCEPTV(PPERR_DL6_DUPVARINSCOPE);
	}
	else {
		THROW(p_scope->AddField(&fld_id, &fld));
		THROW(new_type_id = CreateSymb(fld.Name, '@', crsymfErrorOnDup));
		MEMSZERO(new_te);
		new_te.SymbID = new_type_id;
		new_te.T.Link = typeID;
		new_te.T.Flags |= STypEx::fTypedef;
		TypeList.ordInsert(&new_te, 0, PTR_CMPFUNC(uint));
	}
	CATCH
		Error(LastError, 0, erfLog);
		ok = 0;
	ENDCATCH
	return ok;
}
//
//
//
int SLAPI DlContext::AddTempFldProp(const CtmToken & rSymb, long val)
{
	EXCEPTVAR(LastError);
	int    ok = 1;
	CtmExprConst c;
	DlScope * p_scope = 0;
	int    prop_id = 0;
	THROW(p_scope = GetScope(CurScopeID));
	THROW(prop_id = p_scope->ResolvePropName(rSymb.U.S));
	if(oneof4(prop_id, DlScope::cuifReadOnly, DlScope::cuifDisabled, DlScope::cuifAlignment, DlScope::cuifHidden)) {
		THROW(AddConst((int8)val, &c));
		THROW(p_scope->AddTempFldConst((DlScope::COption)prop_id, c));
	}
	else if(oneof2(prop_id, DlScope::cuifLabelRect, DlScope::cuifFont)) {
		AddedMsgString = rSymb.U.S;
		CALLEXCEPTV(PPERR_DL6_INVPROPTYPE);
	}
	else {
		THROW(AddConst((int32)val, &c));
	}
	THROW(p_scope->AddTempFldConst((DlScope::COption)prop_id, c));
	CATCHZOK
	return ok;
}

int SLAPI DlContext::AddTempFldProp(const CtmToken & rSymb, double val)
{
	EXCEPTVAR(LastError);
	int    ok = 1;
	CtmExprConst c;
	DlScope * p_scope = 0;
	int    prop_id = 0;
	THROW(p_scope = GetScope(CurScopeID));
	THROW(prop_id = p_scope->ResolvePropName(rSymb.U.S));
	if(oneof2(prop_id, DlScope::cuifLabelRect, DlScope::cuifFont)) {
		AddedMsgString = rSymb.U.S;
		CALLEXCEPTV(PPERR_DL6_INVPROPTYPE);
	}
	else {
		THROW(AddConst(val, &c));
		THROW(p_scope->AddTempFldConst((DlScope::COption)prop_id, c));
	}
	CATCHZOK
	return ok;
}

int SLAPI DlContext::AddTempFldProp(const CtmToken & rSymb, const char * pStr)
{
	int    ok = 1;
	CtmExprConst c;
	DlScope * p_scope = 0;
	int    prop_id = 0;
	SString temp_buf(1);
	THROW(p_scope = GetScope(CurScopeID));
	THROW(prop_id = p_scope->ResolvePropName(rSymb.U.S));
	temp_buf = pStr;
	THROW(AddConst(temp_buf, &c));
	THROW(p_scope->AddTempFldConst((DlScope::COption)prop_id, c));
	CATCHZOK
	return ok;
}

int SLAPI DlContext::AddTempFldProp(const CtmToken & rSymb, const void * pData, size_t sz)
{
	int    ok = 1;
	CtmExprConst c;
	DlScope * p_scope = 0;
	int    prop_id = 0;
	THROW(p_scope = GetScope(CurScopeID));
	THROW(prop_id = p_scope->ResolvePropName(rSymb.U.S));
	THROW(AddConst(pData, sz, &c));
	THROW(p_scope->AddTempFldConst((DlScope::COption)prop_id, c));
	CATCHZOK
	return ok;
}

uint SLAPI DlContext::AddUiCluster(int kind, const CtmToken & rSymb, const CtmToken & rText, DLSYMBID typeID, const UiRelRect & rRect)
{
	//
	// Для элементов кластера создается специализированная область с именем 'DLG_SCOPE_NAME:CTRL_NAME'
	//
	SString scope_name;
	DLSYMBID symb_id = 0;
	DLSYMBID type_id = typeID;
	DLSYMBID scope_id = 0;
	DlScope * p_cur_scope = DCtx.GetCurScope();
	THROW(p_cur_scope);
	(scope_name = p_cur_scope->GetName()).CatChar(':').Cat(rSymb.U.S);
	if(type_id == 0)
		SearchSymb("uint16", '@', &type_id);
	uint   fld_id = AddUiCtrl(kind, rSymb, rText, type_id, rRect);
	THROW(fld_id);
	THROW(symb_id = CreateSymb(scope_name, '#', 0));
	scope_id = EnterScope(DlScope::kUiCtrl, scope_name, symb_id, 0);  // checkcluster {
	if(fld_id) {
		CtmExprConst c;
		THROW(AddConst((uint32)scope_id, &c));
		THROW(p_cur_scope->AddFldConst(fld_id, DlScope::cuifCtrlScope, c, 1));
	}
	CATCH
		fld_id = 0;
	ENDCATCH
	return fld_id;
}

int SLAPI DlContext::AddUiClusterItem(const CtmToken & rText, const UiRelRect & rRect, const CtmToken & rDescr)
{
	int    ok = 1;
	uint   item_id = 0;
	SString temp_buf;
	SdbField fld, cluster_fld;
	DlScope * p_cur_scope = GetCurScope();
	const DlScope * p_parent = 0;
	THROW(p_cur_scope);
	THROW(p_cur_scope->IsKind(DlScope::kUiCtrl));
	THROW(p_parent = p_cur_scope->GetOwner());
	{
		//
		// Так как внутренняя область кластера имеет наименование вида 'DLG_SCOPE_NAME:CTRL_NAME',
		// разбиваем это наименование по символу ':' дабы получить собственно имя кластера.
		//
		SString cluster_fld_name;
		int    r = p_cur_scope->Name.Divide(':', temp_buf, cluster_fld_name);
		assert(r > 0);
		assert(temp_buf == p_parent->Name);
		THROW(p_parent->GetFieldByName(cluster_fld_name, &cluster_fld));
	}
	fld.T.Init();
	fld.T = cluster_fld.T;
	(fld.Name = cluster_fld.Name).CatChar('_').CatLongZ((long)(p_cur_scope->GetCount()+1), 2);
	fld.Descr = rDescr.U.S;
	p_cur_scope->AddField(&item_id, &fld);
	{
		CtmExprConst c;
		temp_buf = rText.U.S;
		if(temp_buf.NotEmptyS()) {
			THROW(AddConst(temp_buf, &c));
			THROW(p_cur_scope->AddFldConst(item_id, DlScope::cuifCtrlText, c, 1));
		}
		if(!rRect.IsEmpty()) {
			THROW(AddConst(&rRect, sizeof(rRect), &c));
			THROW(p_cur_scope->AddFldConst(item_id, DlScope::cuifCtrlRect, c, 1));
		}
	}
	CATCHZOK
	return ok;
}

DlScope * SLAPI DlContext::GetCurDialogScope()
{
	const DlScope * p_scope = GetCurScope();
	while(p_scope) {
		if(p_scope->IsKind(DlScope::kUiDialog))
			break;
		else
			p_scope = p_scope->GetOwner();
	}
	return (DlScope *)p_scope; // @badcast
}

int SLAPI DlContext::GetUiSymbSeries(const char * pSymb, SString & rSerBuf, DLSYMBID * pId)
{
	rSerBuf = 0;
	int    ok = 0;
	DLSYMBID symb_id = 0;
	SString pfx, ser, sfx;
	SString symb = pSymb;
	if(symb.NotEmptyS()) {
		if(symb.CmpPrefix("DLG_", 0) == 0) {
			ser = "UIDIALOG";
			ok = 3;
		}
		else if(symb.CmpPrefix("cma", 0) == 0 || symb.CmpPrefix("cm", 0) == 0) {
			ser = "UISSCMD";
			ok = 2;
		}
		else if(symb.CmpPrefix("STDCTL_", 0) == 0) {
			ser = "UISSSTDCTL";
			ok = 2;
		}
		else if(symb.CmpPrefix("CTL_CMD_", 0) == 0) {
			ser = "UISSCMDCTL";
			ok = 2;
		}
		else {
			if(symb.Divide('_', pfx, ser) > 0) {
				symb = ser;
				if(symb.Divide('_', ser, sfx) < 0) {
					ser = symb;
				}
			}
			ok = 1;
		}
		THROW(symb_id = CreateSymb(ser, '$', 0));
		rSerBuf = ser;
	}
	CATCHZOK
	ASSIGN_PTR(pId, symb_id);
	return ok;
}

uint SLAPI DlContext::AddUiListbox(const CtmToken & rSymb, const CtmToken & rText, const UiRelRect & rRect, const CtmToken & rColumns)
{
	DlScope * p_scope = 0;
	uint   fld_id = AddUiCtrl(DlScope::ckListbox, rSymb, rText, 0, rRect);
	THROW(fld_id);
	THROW(p_scope = GetCurScope());
	{
		SString columns_buf = rColumns.U.S;
		if(columns_buf.NotEmptyS()) {
			CtmExprConst c;
			THROW(AddConst(columns_buf, &c));
			THROW(p_scope->AddFldConst(fld_id, DlScope::cuifCtrlListboxColDescr, c, 1));
		}
	}
	CATCH
		fld_id = 0;
	ENDCATCH
	return fld_id;
}

uint SLAPI DlContext::AddUiButton(const CtmToken & rSymb, const CtmToken & rText, const UiRelRect & rRect, const CtmToken & rCmdSymb)
{
	DlScope * p_scope = 0;
	uint   fld_id = AddUiCtrl(DlScope::ckPushbutton, rSymb, rText, 0, rRect);
	THROW(fld_id);
	THROW(p_scope = GetCurScope());
	{
		SString symb_ser, cmd_buf = rCmdSymb.U.S;
		if(cmd_buf.NotEmptyS()) {
			CtmExprConst c;
			THROW(AddConst(cmd_buf, &c));
			THROW(p_scope->AddFldConst(fld_id, DlScope::cuifCtrlCmdSymb, c, 1));
			{
				DLSYMBID ss_id = 0; // Ид серии символов
				DLSYMBID cmd_id = 0;
				int    ss_r = GetUiSymbSeries(cmd_buf, symb_ser, &ss_id);
				THROW(ss_r);
				THROW(cmd_id = CreateSymb(cmd_buf, '$', 0));
				if(!UiSymbAssoc.SearchByVal(cmd_id, 0, 0))
					UiSymbAssoc.Add(ss_id, cmd_id, 0);
				THROW(AddConst((uint32)cmd_id, &c));
				THROW(p_scope->AddFldConst(fld_id, DlScope::cuifCtrlCmd, c, 1));
			}
		}
	}
	CATCH
		fld_id = 0;
	ENDCATCH
	return fld_id;
}

uint SLAPI DlContext::AddUiCtrl(int kind, const CtmToken & rSymb, const CtmToken & rText, DLSYMBID typeID, const UiRelRect & rRect)
{
	EXCEPTVAR(LastError);
	uint   fld_id = 0;
	uint   pos = 0;
	int    ss_r = 0;
	SString temp_buf, symb_ser, dlg_symb_ser;
	TypeEntry te;
	SdbField fld;
	DLSYMBID ss_id = 0; // Ид серии символов
	DLSYMBID dlg_ss_id = 0; // Ид серии символов
	DlScope * p_scope = 0;
	DlScope * p_dlg_scope = 0;
	THROW(p_scope = GetCurScope());
	THROW(p_dlg_scope = GetCurDialogScope());
	fld.T.Init();
	if(typeID) {
		THROW(SearchTypeID(typeID, &(pos = 0), &te));
		fld.T = te.T;
	}
	else {
		fld.T.Typ = S_VOID;
	}
	fld.Name = rSymb.U.S;
	//
	{
		CtmExprConst c_ss;
		ss_r = GetUiSymbSeries(rSymb.U.S, symb_ser, &ss_id);
		if(p_dlg_scope->GetConst(DlScope::cuifSymbSeries, &c_ss)) {
			char   s_buf[256];
			s_buf[0] = 0;
			GetConstData(c_ss, s_buf, sizeof(s_buf));
			dlg_symb_ser = s_buf;
			THROW(dlg_ss_id = CreateSymb(symb_ser, '$', 0));
		}
		else if(ss_r == 1) {
			THROW(AddConst(symb_ser, &c_ss));
			THROW(p_dlg_scope->AddConst(DlScope::cuifSymbSeries, c_ss, 1));
			dlg_symb_ser = symb_ser;
			dlg_ss_id = ss_id;
		}
	}
	if(fld.Name.Empty()) {
		if(dlg_symb_ser.NotEmpty() || symb_ser.NotEmpty()) {
			(fld.Name = "CTL").CatChar('_').Cat(dlg_symb_ser.NotEmpty() ? dlg_symb_ser : symb_ser).CatChar('_').CatLongZ(p_dlg_scope->GetLocalId(), 3);
		}
		else {
			(fld.Name = "CTL").CatChar('_').Cat("NS").CatChar('_').CatLongZ(p_dlg_scope->GetLocalId(), 3);
		}
	}
	THROW(fld_id = CreateSymb(fld.Name, '$', 0));
	if(!UiSymbAssoc.SearchByVal(fld_id, 0, 0)) {
		if(ss_r == 2) {
			//
			// Если серия символов не принадлежит диалогу, то и символ ассоциируем с этой серией.
			//
			UiSymbAssoc.Add(ss_id, fld_id, 0);
		}
		else {
			//
			// В противном случае символ ассоциируем с диалоговой серией.
			//
			UiSymbAssoc.Add(dlg_ss_id, fld_id, 0);
		}
	}
	fld.ID = fld_id;
	if(p_scope->SearchName(fld.Name, &(pos = 0), 0)) {
		AddedMsgString = 0;
		Sc.GetQualif(CurScopeID, "::", 0, AddedMsgString);
		if(AddedMsgString.NotEmpty())
			AddedMsgString.Cat("::");
		AddedMsgString.Cat(fld.Name);
		CALLEXCEPTV(PPERR_DL6_DUPVARINSCOPE);
	}
	else {
		THROW(p_scope->AddField(&fld_id, &fld));
		{
			if(!rRect.IsEmpty()) {
				CtmExprConst c;
				THROW(AddConst(&rRect, sizeof(rRect), &c));
				THROW(p_scope->AddFldConst(fld_id, DlScope::cuifCtrlRect, c, 1));
			}
			temp_buf = rText.U.S;
			if(temp_buf.NotEmptyS()) {
				CtmExprConst c;
				THROW(AddConst(temp_buf, &c));
				THROW(p_scope->AddFldConst(fld_id, DlScope::cuifCtrlText, c, 1));
			}
			{
				CtmExprConst c;
				THROW(AddConst((uint32)kind, &c));
				THROW(p_scope->AddFldConst(fld_id, DlScope::cuifCtrlKind, c, 1));
			}
		}
		THROW(p_scope->AcceptTempFldConstList(fld_id));
	}
	CATCH
		Error(LastError, 0, erfLog);
		fld_id = 0;
	ENDCATCH
	return fld_id;
}
//
//
//
SString & SLAPI GetIdlTypeString(TYPEID typ, SString & rBuf, const char * pFldName, size_t indent)
{
	rBuf.Z();
	size_t sz = stsize(typ);
	size_t dim = 0;
	SString type_text;
	switch(GETSTYPE(typ)) {
		case S_AUTOINC:
		case S_INT:
		case S_LOGICAL:
			if(sz == 1)
				type_text = "char";
			else if(sz == 2)
				type_text = "short";
			else if(sz == 4)
				type_text = "long";
			else
				(type_text = "int_invalid_size_").Cat(sz);
			break;
		case S_UINT:
			(type_text = "unsigned").Space();
			if(sz == 1)
				type_text.Cat("char");
			else if(sz == 2)
				type_text.Cat("short");
			else if(sz == 4)
				type_text.Cat("long");
			else
				(type_text = "uint_invalid_size_").Cat(sz);
			break;
		case S_FLOAT:
			if(sz == 4)
				type_text = "float";
			else if(sz == 8)
				type_text = "double";
			else if(sz == 10)
				type_text = "long double";
			else
				(type_text = "float_invalid_size_").Cat(sz);
			break;
		case S_DATE:      type_text = "DATE"; break;
		case S_TIME:      type_text = "DATE"; break;
		case S_DATETIME:  type_text = "DATE"; break;
		case S_MONEY:
		case S_NUMERIC:   type_text = "char"; dim = sz; break;
		case S_CHAR:
		case S_LSTRING:
		case S_ZSTRING:
		case S_NOTE:      type_text = "BSTR"; dim = sz; break;
		case S_WCHAR:     type_text = "wchar_t"; dim = sz / 2; break;
		case S_WZSTRING:  type_text = "wchar_t"; dim = sz / 2; break; // @v8.6.2
		case S_INTRANGE:  type_text = "IntRange";  break;
		case S_REALRANGE: type_text = "RealRange"; break;
		case S_DATERANGE: type_text = "DateRange"; break;
		case S_VARIANT:   type_text = "VARIANT";   break;
		case S_VOID:      type_text = "void";      break;
		default:          type_text = "unknown";   break;
	}
	rBuf = type_text;
	if(pFldName) {
		if(indent <= rBuf.Len())
			indent = rBuf.Len()+1;
		rBuf.Align(indent, ADJ_LEFT).Cat(pFldName);
		if(dim > 1)
			rBuf.CatChar('[').Cat(dim).CatChar(']');
	}
	return rBuf;
}

int SLAPI DlContext::Format_C_Type(DLSYMBID typeID, STypEx & rTyp, const char * pFldName, long flags, SString & rBuf)
{
	int    ok = 1;
	SString type_buf, name_buf, comment;
	TypeEntry te;
	uint   i;
	TypeDetail td;
	if(typeID && flags & fctfResolveTypeID) {
		THROW(SearchTypeID(typeID, 0, &te));
		//
		// Здесь нельзя применять присваивание из-за того, что тогда ссылка rTyp будет алиасом
		// te, которую потом будем использовать как временную переменную
		//
		memcpy(&rTyp, &te.T, sizeof(STypEx));
	}
	THROW(UnrollType(typeID, rTyp, &td));
	if(td.T.Mod == STypEx::modLink) {
		td.T.Typ = MKSTYPE(S_INT, 4);
		GetBinaryTypeString(td.T.Typ, 0, type_buf, 0, 0);
		if(td.T.Flags & STypEx::fStruct) {
			THROW(GetSymb(td.T.Link, name_buf, '@') || GetSymb(td.T.Link, name_buf, '!'));
			if(flags & fctfSourceOutput)
				comment.CatCharN('/', 2).Space().CatChar('-').CatChar('>').Cat(name_buf);
			name_buf = 0;
		}
	}
	else {
		if(td.T.Flags & STypEx::fStruct) {
			THROW(SearchTypeID(td.T.Link, 0, &te));
			THROW(GetSymb(td.T.Link, type_buf, '@') || GetSymb(td.T.Link, type_buf, '!'));
		}
		else if(td.T.Flags & STypEx::fTypedef && typeID) {
			THROW(GetSymb(td.TerminalTypeID, type_buf, '@'));
		}
		else if(flags & fctfIDL)
			GetIdlTypeString(td.T.Typ, type_buf, 0, 0);
		else if(flags & fctfIfaceImpl && GETSTYPE(td.T.Typ) == S_ZSTRING) {
			type_buf = "SString";
			if(!(flags & fctfInstance)) {
				//
				// Указатель на ссылку недопустим - будет просто указатель на SString
				//
				if((rTyp.Flags & STypEx::fOf) || rTyp.Mod != STypEx::modPtr)
					type_buf.Space().CatChar('&');
			}
		}
		else
			GetBinaryTypeString(td.T.Typ, 0, type_buf, 0, 0);
	}
	if(td.T.Mod == STypEx::modPtr) {
		if(flags & (fctfSourceOutput | fctfIDL))
			type_buf.Space();
		type_buf.CatChar('*');
	}
	else if(td.T.Mod == STypEx::modRef) {
		if(flags & (fctfSourceOutput | fctfIDL))
			type_buf.Space();
		type_buf.CatChar('&');
	}
	for(i = 0; i < td.PtrList.GetCount(); i++) {
		int    c = LoWord(td.PtrList.Get(i));
		if(c == STypEx::modPtr)
			type_buf.CatChar('*');
		else if(c == STypEx::modRef)
			type_buf.CatChar('&');
	}
	//
	//
	//
	name_buf = pFldName;
	if(!(flags & (fctfIDL | fctfIfaceImpl))) {
		int   _t = GETSTYPE(td.T.Typ);
		if(oneof4(_t, S_ZSTRING, S_NOTE, S_LSTRING, S_RAW))
			name_buf.CatChar('[').Cat(GETSSIZE(td.T.Typ), NMBF_NOZERO).CatChar(']');
		else if(oneof2(_t, S_WCHAR, S_WZSTRING)) // @v8.6.2 
			name_buf.CatChar('[').Cat(GETSSIZE(td.T.Typ)/sizeof(wchar_t), NMBF_NOZERO).CatChar(']');
		else if(oneof2(_t, S_DEC, S_MONEY))
			name_buf.CatChar('[').Cat(GETSSIZED(td.T.Typ), NMBF_NOZERO).CatChar(']');
	}
	if(td.T.Mod == STypEx::modArray)
		name_buf.CatChar('[').Cat(td.T.Dim).CatChar(']');
	for(i = 0; i < td.DimList.GetCount(); i++)
		name_buf.CatChar('[').Cat(td.DimList.Get(i)).CatChar(']');
	if(flags & fctfSourceOutput /*&& !(flags & fctfIDL)*/) {
		type_buf.Align(6, ADJ_LEFT).Space();
		name_buf.Semicol();
		if(comment.NotEmpty())
			name_buf.Align(16, ADJ_LEFT).Space();
	}
	rBuf = type_buf;
	if(rBuf.Last() != ' ' && rBuf.Last() != '\t' && pFldName && pFldName[0])
		rBuf.Space();
	rBuf.Cat(name_buf).Cat(comment);
	CATCHZOK
	return ok;
}

int SLAPI DlContext::GetFuncName(int, const CtmExpr * pExpr, SString & rBuf)
{
	int    ok = 1;
	SString arg_buf, temp_buf;
	rBuf = 0;
	if(pExpr->GetKind() == CtmExpr::kFuncName) {
		rBuf = pExpr->U.S;
	}
	else if(pExpr->GetKind() == CtmExpr::kOp) {
		DlFunc::GetOpName(pExpr->U.Op, rBuf);
	}
	else
		ok = 0;
	if(ok) {
		uint   c = pExpr->GetArgCount();
		for(uint i = 0; i < c; i++) {
			const CtmExpr * p_arg = pExpr->GetArg(i);
			if(p_arg) {
				TypeEntry te;
				DLSYMBID type_id = p_arg->GetTypeID();
				Format_C_Type(type_id, te.T, 0, fctfResolveTypeID, temp_buf);
				if(arg_buf.NotEmpty())
					arg_buf.Comma();
				arg_buf.Cat(temp_buf);
			}
		}
		rBuf.Cat(arg_buf.Quot('(', ')'));
	}
	return ok;
}

int SLAPI DlContext::Format_Func(const DlFunc & rFunc, long options, SString & rBuf)
{
	int    ok = 1;
	SString temp_buf, arg_name;
	TypeEntry te;
	THROW(Format_C_Type(rFunc.TypID, te.T, 0, fctfResolveTypeID, temp_buf));
	(rBuf = temp_buf).Space();
	rFunc.GetName(0, temp_buf);
	rBuf.Cat(temp_buf).CatChar('(');
	for(uint i = 0, c = rFunc.GetArgCount(); i < c; i++) {
		rFunc.GetArgName(i, arg_name);
		THROW(Format_C_Type(rFunc.GetArgType(i), te.T, arg_name, fctfResolveTypeID, temp_buf));
		rBuf.Cat(temp_buf);
		if(i < (c-1))
			rBuf.Comma();
	}
	rBuf.CatChar(')');
	CATCH
		ok = 0;
		(rBuf = "Error on output func ").Cat(rFunc.Name);
	ENDCATCH
	return ok;
}

int SLAPI DlContext::Write_Scope(int indent, SFile & rOutFile, const DlScope & rScope)
{
	uint   i;
	SString line;
	line.CatCharN('\t', indent).CatEq(rScope.Name, rScope.ID).CatDiv(';', 2).
		CatEq("size", (long)rScope.GetRecSize()).CatDiv(';', 2).CatEq("count", (long)rScope.GetCount()).CR();
	rOutFile.WriteLine(line);
	DlScope * p_child = 0;
	SString fld_buf;
	SdbField fld;
	for(i = 0; rScope.EnumFields(&i, &fld);) {
		(fld_buf = 0).CatCharN('\t', indent+1).Cat(fld.Name).CatDiv(';', 2).CatEq("size", (long)fld.T.GetBinSize()).CR();
		rOutFile.WriteLine(fld_buf);
	}
	for(i = 0; rScope.EnumChilds(&i, &p_child);)
		if(p_child)
			Write_Scope(indent+1, rOutFile, *p_child); // @recursion
		else
			rOutFile.WriteLine((line = 0).CatCharN('\t', indent+1).Cat("null").CR());
	{
		DlFunc func;
		SString func_name;
		for(i = 0; rScope.GetFuncByPos(i, &func); i++) {
			Format_Func(func, 0, func_name);
			(line = 0).CatCharN('\t', indent+1).Cat(func_name).CR();
			rOutFile.WriteLine(line);
		}
	}
	return 1;
}

int SLAPI DlContext::Write_DebugListing()
{
	SFile out_file(LogFileName, SFile::mWrite);
	SString line;
	out_file.WriteLine((line = "//").CR());
	out_file.WriteLine((line = "// Scope").CR());
	out_file.WriteLine((line = "//").CR());
	Write_Scope(0, out_file, Sc);

	out_file.WriteLine((line = "//").CR());
	out_file.WriteLine((line = "// Type").CR());
	out_file.WriteLine((line = "//").CR());
	for(uint i = 0; i < TypeList.getCount(); i++) {
		Format_TypeEntry(TypeList.at(i), line = 0);
		out_file.WriteLine(line.CR());
	}
	return 1;
}


static SString & FASTCALL _RectToLine(const UiRelRect & rRect, SString & rBuf)
{
	return rBuf.CatChar('(').Cat(rRect.L.X.Val).CatDiv(',', 2).Cat(rRect.L.Y.Val).CatDiv(',', 2).
		Cat(rRect.R.X.Val).CatDiv(',', 2).Cat(rRect.R.Y.Val).CatChar(')');
}

int SLAPI DlContext::Write_DialogReverse()
{
	int    ok = 1;
	SString out_file_name;
	{
		SPathStruc ps;
		ps.Split(InFileName);
		ps.Nam.CatChar('-').Cat("reverse");
		ps.Merge(out_file_name);
	}
	char   c_buf[1024]; // Буфер для извлечения констант
	SFile f_out(out_file_name, SFile::mWrite);
	SString line_buf, temp_buf, text_buf, cmd_buf, type_buf;
	LongArray scope_id_list;
	StrAssocArray prop_list;
	SdbField ctrl, ctrl_item;
	THROW_SL(f_out.IsValid());
	Sc.GetChildList(DlScope::kUiDialog, 1, &scope_id_list);
	for(uint i = 0; i < scope_id_list.getCount(); i++) {
		const DlScope * p_scope = GetScope(scope_id_list.at(i), 0);
		if(p_scope) {
			prop_list.Z();
			(line_buf = 0).CR().Cat("dialog").Space().Cat(p_scope->Name);
			{
				text_buf.Z();
				if(GetConstData(p_scope->GetConst(DlScope::cuifCtrlText), c_buf, sizeof(c_buf)))
					text_buf = (const char *)c_buf;
				line_buf.Space().CatQStr(text_buf);
			}
			if(GetConstData(p_scope->GetConst(DlScope::cuifCtrlRect), c_buf, sizeof(c_buf)))
				_RectToLine(*(UiRelRect *)c_buf, line_buf.Space());
			if(GetConstData(p_scope->GetConst(DlScope::cuifFont), c_buf, sizeof(c_buf))) {
				if(((const char *)c_buf)[0]) {
					(text_buf = 0).CatQStr((const char *)c_buf);
					prop_list.Add(DlScope::cuifFont, text_buf);
				}
			}
			DlScope::PropListToLine(prop_list, 1, line_buf).CR();
			line_buf.CatChar('{').CR();
			f_out.WriteLine(line_buf);
			for(uint j = 0; p_scope->EnumFields(&j, &ctrl);) {
				int    skip_line_finish = 0;
				uint32 kind = 0;
				UiRelRect rect;
				rect.Reset();
				prop_list.Z();
				(line_buf = 0).CatChar('\t');
				if(GetConstData(p_scope->GetFldConst(ctrl.ID, DlScope::cuifCtrlKind), c_buf, sizeof(c_buf)))
					kind = *(uint32 *)c_buf;
				if(GetConstData(p_scope->GetFldConst(ctrl.ID, DlScope::cuifCtrlRect), c_buf, sizeof(c_buf)))
					rect = *(UiRelRect *)c_buf;
				//
				// Находим строку текста элемента
				//
				text_buf.Z();
				if(GetConstData(p_scope->GetFldConst(ctrl.ID, DlScope::cuifCtrlText), c_buf, sizeof(c_buf)))
					text_buf = (const char *)c_buf;
				//
				// Формируем строку типа данных STypEx
				//
				type_buf = 0;
				GetBinaryTypeString(ctrl.T.Typ, 1, type_buf, "", 0);
				if(type_buf.Cmp("unknown", 0) == 0)
					(type_buf = "string").CatChar('[').Cat(48).CatChar(']');
				//
				cmd_buf = 0;
				UiItemKind uiik(kind);
				if(uiik.Symb.NotEmpty())
					line_buf.Cat(uiik.Symb);
				else
					line_buf.CatEq("???", (long)kind);
				if(GetConstData(p_scope->GetFldConst(ctrl.ID, DlScope::cuifReadOnly), c_buf, sizeof(c_buf)))
					prop_list.Add(DlScope::cuifReadOnly, "");
				if(GetConstData(p_scope->GetFldConst(ctrl.ID, DlScope::cuifStaticEdge), c_buf, sizeof(c_buf)))
					prop_list.Add(DlScope::cuifStaticEdge, "");
				if(GetConstData(p_scope->GetFldConst(ctrl.ID, DlScope::cuifLabelRect), c_buf, sizeof(c_buf))) {
					_RectToLine(*(UiRelRect *)c_buf, temp_buf = 0);
					prop_list.Add(DlScope::cuifLabelRect, temp_buf);
				}
				line_buf.Space().Cat(ctrl.Name).Space().CatQStr(text_buf);
				if(!rect.IsEmpty())
					_RectToLine(rect, line_buf.Space());
				if(kind == DlScope::ckInput) {
					line_buf.Space().Cat(type_buf);
				}
				else if(kind == DlScope::ckPushbutton) {
					if(GetConstData(p_scope->GetFldConst(ctrl.ID, DlScope::cuifCtrlCmdSymb), c_buf, sizeof(c_buf))) {
						temp_buf = (const char *)c_buf;
						line_buf.Space().Cat(temp_buf);
					}
				}
				else if(oneof2(kind, DlScope::ckCheckCluster, DlScope::ckRadioCluster)) {
					if(GetConstData(p_scope->GetFldConst(ctrl.ID, DlScope::cuifCtrlScope), c_buf, sizeof(c_buf))) {
						DLSYMBID inner_scope_id = *(uint32 *)c_buf;
						DlScope * p_inner_scope = GetScope(inner_scope_id);
						if(p_inner_scope && p_inner_scope->GetCount()) {
							skip_line_finish = 1;
							line_buf.Space().CatChar('{').CR();
							f_out.WriteLine(line_buf);
							for(uint k = 0; p_inner_scope->EnumFields(&k, &ctrl_item);) {
								(line_buf = 0).CatCharN('\t', 2);
								rect.Reset();
								if(GetConstData(p_inner_scope->GetFldConst(ctrl_item.ID, DlScope::cuifCtrlRect), c_buf, sizeof(c_buf)))
									rect = *(UiRelRect *)c_buf;
								text_buf = 0;
								if(GetConstData(p_inner_scope->GetFldConst(ctrl_item.ID, DlScope::cuifCtrlText), c_buf, sizeof(c_buf)))
									text_buf = (const char *)c_buf;
								line_buf.CatQStr(text_buf);
								if(!rect.IsEmpty())
									_RectToLine(rect, line_buf.Space());
								line_buf.Semicol().CR();
								f_out.WriteLine(line_buf);
							}
							(line_buf = 0).CatChar('\t').CatChar('}').CR();
							f_out.WriteLine(line_buf);
						}
					}
				}
				else if(kind == DlScope::ckListbox) {
					if(GetConstData(p_scope->GetFldConst(ctrl.ID, DlScope::cuifCtrlListboxColDescr), c_buf, sizeof(c_buf))) {
						temp_buf = (const char *)c_buf;
						line_buf.Space().Cat("columns").Space().CatQStr(temp_buf);
					}
				}
				if(!skip_line_finish) {
					DlScope::PropListToLine(prop_list, 2, line_buf);
					line_buf.Semicol().CR();
					f_out.WriteLine(line_buf);
				}
			}
			//
			(line_buf = 0).CatChar('}').CR();
			f_out.WriteLine(line_buf);
		}
	}
	CATCHZOK
	return ok;
}

DLSYMBID DlContext::SetDeclTypeMod(DLSYMBID ofTyp, int mod /* STypEx::modXXX */, uint arrayDim)
{
	EXCEPTVAR(LastError);
	DLSYMBID new_type_id = 0;
	uint   pos = 0;
	STypEx t, t_of;
	THROW(SearchTypeID(ofTyp, &pos, 0));
	t.Init();
	t_of = TypeList.at(pos).T;
	if(t_of.Mod == 0) {
		t = t_of;
		if(mod == STypEx::modArray) {
			uint   _len = 0, _prec = 0;
			int    is_dec_dim = GetDecimalDim(arrayDim, &_len, &_prec);
			int    _t = GETSTYPE(t.Typ);
			int    _is_pure = t.IsPure();
			if(is_dec_dim) {
				(AddedMsgString = 0).CatChar('[').Cat(_len).Dot().Cat(_prec).CatChar(']');
				THROW_V(t.IsPure() && oneof2(_t, S_DEC, S_MONEY), PPERR_DL6_DECMODONNONDECTYPE);
				t.Typ = MKSTYPED(_t, _len, _prec);
			}
			else if(t.IsZStr(&_len) && _len == 0) t.Typ = MKSTYPE(S_ZSTRING, arrayDim);
			else if(t.IsWZStr(&_len) && _len == 0) t.Typ = MKSTYPE(S_WZSTRING, arrayDim * sizeof(wchar_t)); // @v8.6.2
			else if(_is_pure && _t == S_NOTE)     t.Typ = MKSTYPE(S_NOTE,    arrayDim);
			else if(_is_pure && _t == S_LSTRING)  t.Typ = MKSTYPE(S_LSTRING, arrayDim);
			else if(_is_pure && _t == S_DEC)      t.Typ = MKSTYPED(S_DEC,    arrayDim, 2);
			else if(_is_pure && _t == S_MONEY)    t.Typ = MKSTYPED(S_MONEY,  arrayDim, 2);
			else if(_is_pure && _t == S_RAW)      t.Typ = MKSTYPE(S_RAW,     arrayDim);
			else if(_is_pure && _t == S_CLOB)     t.Typ = MKSTYPE(S_CLOB,    arrayDim);
			else if(_is_pure && _t == S_BLOB)     t.Typ = MKSTYPE(S_BLOB,    arrayDim);
			else {
				t.Mod = mod;
				t.Dim = arrayDim;
			}
		}
		else
			t.Mod = mod;
	}
	else {
		t.Flags |= STypEx::fOf;
		t.Link   = ofTyp;
		t.Mod    = mod;
		if(t.Mod == STypEx::modArray) {
			//
			// Проверка на то, чтобы не впихнули определение размерности [d.p] в середину массива {
			//
			uint   _len = 0, _prec = 0;
			int    is_dec_dim = GetDecimalDim(arrayDim, &_len, &_prec);
			(AddedMsgString = 0).CatChar('[').Cat(_len).Dot().Cat(_prec).CatChar(']');
			THROW_V(is_dec_dim, PPERR_DL6_DECMODONNONDECTYPE);
			// }
			t.Dim = arrayDim;
		}
	}
	new_type_id = SearchSTypEx(t, 0);
	if(new_type_id == 0) {
		SString name;
		THROW(MangleType(new_type_id, t, name));
		THROW(new_type_id = CreateSymb(name, '@', crsymfErrorOnDup));
		TypeEntry te;
		MEMSZERO(te);
		te.SymbID = new_type_id;
		te.T = t;
		TypeList.ordInsert(&te, 0, PTR_CMPFUNC(uint));
	}
	CATCH
		new_type_id = 0;
		Error(LastError, 0, erfLog);
	ENDCATCH
	return new_type_id;
}

int SLAPI DlContext::AddPropDeclare(CtmDclr & rSymb, int propDirParam)
{
	int    ok = 1;
	DLSYMBID symb_id = 0;
	CtmDclrList arg_list;
	arg_list.Init();
	if(propDirParam & DlFunc::fArgOut) {
		THROW(AddFuncDeclare(rSymb, arg_list, DlFunc::fArgOut));
	}
	if(propDirParam & DlFunc::fArgIn) {
		CtmDclr arg;
		arg.Copy(rSymb);
		//delete arg.Tok.U.S;
		arg.Tok.U.S = newStr("value");
		arg_list.Add(arg);

		rSymb.DimList.Destroy();
		rSymb.PtrList.Destroy();
		THROW(SearchSymb("void", '@', &symb_id) > 0);
		rSymb.TypeID = symb_id;
		THROW(AddFuncDeclare(rSymb, arg_list, DlFunc::fArgIn));
	}
	CATCHZOK
	arg_list.Destroy();
	return ok;
}

int DlContext::AddFuncDeclare(const CtmDclr & rSymb, const CtmDclrList & rArgList, int propDirParam)
{
	int    ok = 1;
	uint   j, type_pos;
	TypeEntry te;
	DLSYMBID full_type_id;
	DlScope * p_scope = GetScope(CurScopeID);
	DlFunc func;
	{
		const CtmDclr * p_dclr = &rSymb;
		full_type_id = p_dclr->TypeID;
		THROW(SearchTypeID(full_type_id, &(type_pos = 0), &te));
		//
		for(j = 0; j < p_dclr->PtrList.GetCount(); j++) {
			uint32 mod_ptr = p_dclr->PtrList.Get(j);
			THROW(full_type_id = SetDeclTypeMod(full_type_id, LoWord(mod_ptr), 0));
		}
		//
		for(j = 0; j < p_dclr->DimList.GetCount(); j++) {
			uint32 dim = p_dclr->DimList.Get(j);
			THROW(full_type_id = SetDeclTypeMod(full_type_id, STypEx::modArray, dim));
		}
		if(full_type_id != p_dclr->TypeID)
			THROW(SearchTypeID(full_type_id, &(type_pos = 0), &te));
	}
	func.TypID = full_type_id;
	if(propDirParam == DlFunc::fArgIn) { // propput
		func.Flags |= DlFunc::fPropPut;
	}
	else if(propDirParam == DlFunc::fArgOut) { // propget
		func.Flags |= DlFunc::fPropGet;
	}
	const uint   sc_kind = p_scope ? p_scope->GetKind() : 0;
	const int    is_exp_func = oneof2(sc_kind, DlScope::kExpDataHdr, DlScope::kExpDataIter);
	THROW_PP(is_exp_func || sc_kind == DlScope::kInterface, PPERR_DL6_FUNCINVSCOPE);
	if(is_exp_func)
		func.Name.CatChar('?').Cat(rSymb.Tok.U.S);
	else
		func.Name = rSymb.Tok.U.S;
	if(rArgList.P_List) {
		for(uint i = 0; i < rArgList.P_List->getCount(); i++) {
			const CtmDclr * p_dclr = rArgList.P_List->at(i);
			full_type_id = p_dclr->TypeID;
			THROW(SearchTypeID(full_type_id, &(type_pos = 0), &te));
			//
			for(j = 0; j < p_dclr->PtrList.GetCount(); j++) {
				uint32 mod_ptr = p_dclr->PtrList.Get(j);
				THROW(full_type_id = SetDeclTypeMod(full_type_id, LoWord(mod_ptr), 0));
			}
			//
			for(j = 0; j < p_dclr->DimList.GetCount(); j++) {
				uint32 dim = p_dclr->DimList.Get(j);
				THROW(full_type_id = SetDeclTypeMod(full_type_id, STypEx::modArray, dim));
			}
			if(full_type_id != p_dclr->TypeID)
				THROW(SearchTypeID(full_type_id, &(type_pos = 0), &te));
			func.AddArg(full_type_id, p_dclr->Tok.U.S, p_dclr->IfaceArgDirMod);
		}
	}
	THROW(p_scope->AddFunc(&func));
	CATCHZOK
	return ok;

#if 0 // @sample {
int SLAPI DlContext::AddBFunc(const char * pFuncName, uint implID, const char * pRetType, va_list pArgList)
{
	int    ok = 1;
	DLSYMBID symb_id = 0;
	DlFunc f;
	f.Name.CatChar('?').Cat(pFuncName);
	if(SearchSymb(pRetType, '@', &symb_id) > 0) {
		f.TypID = symb_id;
		const char * p_arg_typ = va_arg(pArgList, const char *);
		while(ok && p_arg_typ) {
			if(SearchSymb(p_arg_typ, '@', &symb_id))
				f.AddArg(symb_id, 0);
			else
				ok = 0;
			p_arg_typ = va_arg(pArgList, const char *);
		}
		f.Flags |= DlFunc::fImplByID;
		f.ImplID = implID;
		if(ok)
			ok = Sc.AddFunc(&f);
	}
	else
		ok = 0;
	return ok;
}
#endif // } 0
}

int DlContext::AddEnumItem(const CtmToken & rSymb, int useExplVal, uint val)
{
	EXCEPTVAR(LastError);
	int    ok = 1;
	DlScope * p_scope = GetScope(CurScopeID);
	THROW(p_scope);
	{
		const  uint sc_kind = p_scope->GetKind();
		uint   fld_id = 0;
		SdbField fld;
		uint   type_pos = 0, fld_pos = 0;
		int    is_formula = 0;
		DLSYMBID type_id = 0;
		TypeEntry te;
		THROW_V(sc_kind == DlScope::kEnum, PPERR_DL6_NOENUM);
		THROW(SearchSymb("uint32", '@', &type_id));
		THROW(SearchTypeID(type_id, &type_pos, &te));
		fld.T.Init();
		fld.T = te.T;
		fld.Name = rSymb.U.S;
		if(useExplVal)
			fld.EnumVal = val;
		else if(p_scope->GetCount())
			fld.EnumVal = (uint)p_scope->GetFieldOuterFormat(p_scope->GetCount()-1) + 1;
		else
			fld.EnumVal = 0;
		if(p_scope->SearchName(fld.Name, &(fld_pos = 0), 0)) {
			AddedMsgString = 0;
			Sc.GetQualif(CurScopeID, "::", 0, AddedMsgString);
			if(AddedMsgString.NotEmpty())
				AddedMsgString.Cat("::");
			AddedMsgString.Cat(fld.Name);
			CALLEXCEPTV(PPERR_DL6_DUPVARINSCOPE);
		}
		else {
			THROW(p_scope->AddField(&fld_id, &fld));
		}
	}
	CATCH
		Error(LastError, 0, erfLog);
		ok = 0;
	ENDCATCH
	return ok;
}

int DlContext::AddDeclaration(DLSYMBID typeId, const CtmDclr & rDclr, CtmExpr * pExpr)
{
	EXCEPTVAR(LastError);
	int    ok = 1;
	DlScope * p_scope = GetScope(CurScopeID);
	THROW(p_scope);
	{
		const  uint sc_kind = p_scope->GetKind();
		uint   i, fld_id = 0;
		SdbField fld;
		uint   type_pos = 0, fld_pos = 0;
		int    is_formula = 0;
		TypeEntry te;
		if(pExpr) {
			THROW(ResolveExpr(CurScopeID, CurScopeID, 0, pExpr));
			is_formula = 1;
			pExpr->Pack(fld.Formula);
			typeId = pExpr->GetTypeID();
			pExpr->Destroy();
		}
		DLSYMBID full_type_id = typeId;
		THROW(SearchTypeID(typeId, &type_pos, &te));
		//
		uint   c = rDclr.PtrList.GetCount();
		for(i = 0; i < c; i++) {
			uint32 mod_ptr = rDclr.PtrList.Get(i);
			THROW(full_type_id = SetDeclTypeMod(full_type_id, LoWord(mod_ptr), 0));
		}
		//
		c = rDclr.DimList.GetCount();
		if(c == 0 && oneof2(te.T.Typ, S_DEC, S_MONEY)) {
			//
			// Если для типа decimal или money не указана размерность, то принимаем
			// [8.2] (2 - по умолчанию)
			//
			THROW(full_type_id = SetDeclTypeMod(full_type_id, STypEx::modArray, 8));
		}
		else {
			for(i = 0; i < c; i++) {
				uint32 dim = rDclr.DimList.Get(i);
				THROW(full_type_id = SetDeclTypeMod(full_type_id, STypEx::modArray, dim));
			}
		}
		if(full_type_id != typeId) {
			THROW(SearchTypeID(full_type_id, &(type_pos = 0), &te));
		}
		fld.T.Init();
		fld.T = te.T;
		SETFLAG(fld.T.Flags, STypEx::fFormula, is_formula);
		fld.Name = rDclr.Tok.U.S;
		if(rDclr.Format.Code == T_FMT) {
			fld.OuterFormat = ParseFormat(rDclr.Format.U.S, te.T.Typ);
		}
		if(rDclr.Alias.Code == T_AT_IDENT) {
			fld.Descr = rDclr.Alias.U.S;
		}
		{
			int    is_dup = 0;
			if(p_scope->SearchName(fld.Name, &(fld_pos = 0), 0)) {
				is_dup = 1;
			}
			else {
				// @v9.2.8 {
				for(const DlScope * p_base = p_scope->GetBase(); !is_dup && p_base != 0; p_base = p_base->GetBase()) {
					if(p_base->SearchName(fld.Name, &(fld_pos = 0), 0))
						is_dup = 1;
				}
				// } @v9.2.8 
			}
			if(is_dup) {
				AddedMsgString = 0;
				Sc.GetQualif(CurScopeID, "::", 0, AddedMsgString);
				if(AddedMsgString.NotEmpty())
					AddedMsgString.Cat("::");
				AddedMsgString.Cat(fld.Name);
				CALLEXCEPTV(PPERR_DL6_DUPVARINSCOPE);
			}
			else {
				THROW(p_scope->AddField(&fld_id, &fld));
			}
		}	
	}
	CATCH
		Error(LastError, 0, erfLog);
		ok = 0;
	ENDCATCH
	return ok;
}

int SLAPI DlContext::InitDbIndexName(const char * pIdxName, SString & rBuf)
{
	EXCEPTVAR(LastError);
	int    ok = 1;
	DlScope * p_child = 0;
	uint   ci = 0, cc = 0;
	DlScope * p_scope = GetScope(CurScopeID, DlScope::kDbTable);
	THROW(p_scope);
	for(ci = 0; p_scope->EnumChilds(&ci, &p_child) > 0;) {
		if(p_child->IsKind(DlScope::kDbIndex)) {
			if(pIdxName && pIdxName[0]) {
				AddedMsgString = pIdxName;
				THROW_V(p_child->GetName().CmpNC(pIdxName) != 0, PPERR_DL6_DUPDBIDXNAME);
			}
			++cc;
		}
	}
	if(pIdxName && pIdxName[0])
		rBuf = pIdxName;
	else
		(rBuf = "Key").Cat(cc);
	CATCH
		Error(LastError, 0, erfLog);
		ok = 0;
	ENDCATCH
	return ok;
}

int SLAPI DlContext::AddDbIndexSegmentDeclaration(const char * pFieldName, long options)
{
	int    ok = 1;
	DlScope * p_scope = GetScope(CurScopeID, DlScope::kDbIndex);
	THROW(p_scope);
	THROW(p_scope->AddDbIndexSegment(pFieldName, options));
	CATCH
		Error(LastError, 0, erfLog);
		ok = 0;
	ENDCATCH
	return ok;
}

int SLAPI DlContext::ResolveDbIndexSegFlag(long flags, const char * pSymb, long * pResultFlags)
{
	int    ok = 1;
	long   f = 0;
	if(sstreqi_ascii(pSymb, "DESC"))
		flags |= XIF_DESC;
	else if(sstreqi_ascii(pSymb, "ACS"))
		flags |= XIF_ACS;
	else if(sstreqi_ascii(pSymb, "IGNORECASE") || sstreqi_ascii(pSymb, "NOCASE") || sstreqi_ascii(pSymb, "CASEINSENSITIVE") || sstreqi_ascii(pSymb, "I")) {
		flags |= XIF_NOCASE;
	}
	else {
		Error(PPERR_DL6_INVIDXSEGFLAG, pSymb, erfLog);
		ok = 0;
	}
	ASSIGN_PTR(pResultFlags, flags);
	return ok;
}

int SLAPI DlContext::ResolveDbIndexFlag(const char * pSymb)
{
	int    ok = 1;
	long   f = 0;
	DlScope::Attr attr;
	MEMSZERO(attr);
	DlScope * p_scope = GetScope(CurScopeID, DlScope::kDbIndex);
	THROW(p_scope);
	if(sstreqi_ascii(pSymb, "DUP")) {
		if(p_scope->GetAttrib(DlScope::sfDbiUnique, 0)) {
			Error(PPERR_DL6_MISSIDXUNIQ, 0, erfLog);
			ok = 0;
		}
		attr.A = DlScope::sfDbiDup;
		p_scope->SetAttrib(attr);
	}
	else if(sstreqi_ascii(pSymb, "UNIQUE")) {
		if(p_scope->GetAttrib(DlScope::sfDbiDup, 0)) {
			Error(PPERR_DL6_MISSIDXUNIQ, 0, erfLog);
			ok = 0;
		}
		attr.A = DlScope::sfDbiUnique;
		p_scope->SetAttrib(attr);
	}
	else if(sstreqi_ascii(pSymb, "MOD")) {
		attr.A = DlScope::sfDbiMod;
		p_scope->SetAttrib(attr);
	}
	else if(sstreqi_ascii(pSymb, "ALLSEGNULL")) {
		attr.A = DlScope::sfDbiAllSegNull;
		p_scope->SetAttrib(attr);
	}
	else if(sstreqi_ascii(pSymb, "ANYSEGNULL")) {
		attr.A = DlScope::sfDbiAnySegNull;
		p_scope->SetAttrib(attr);
	}
	else {
		Error(PPERR_DL6_INVIDXSEGFLAG, pSymb, erfLog);
		ok = 0;
	}
	CATCHZOK
	return ok;
}

int SLAPI DlContext::ResolveDbFileDefinition(const CtmToken & rSymb, const char * pConstStr, int constInt)
{
	int    ok = 1;
	CtmExprConst c;
	uint32 val;
	DlScope::Attr attr;
	MEMSZERO(attr);
	SString temp_buf(1); // (1) уверены, что (const char *)temp_buf != 0.
	DlScope * p_scope = GetScope(CurScopeID, DlScope::kDbTable);
	THROW(p_scope);
	if(rSymb.Code == 0) {
		if(pConstStr) {
			temp_buf = pConstStr;
			THROW(AddConst(temp_buf, &c));
			THROW(p_scope->AddConst(DlScope::cdbtFileName, c, 1));
		}
	}
	else if(sstreqi_ascii(rSymb.U.S, "page")) {
		ok = 0;
		val = 0;
		for(uint i = 0; !ok && i < NUMPGSIZES; i++)
			if(Btrieve::LimitPgInfo[i].pgSz == constInt) {
				val = (uint32)constInt;
				ok = 1;
			}
		THROW(AddConst((uint32)val, &c));
		THROW(p_scope->AddConst(DlScope::cdbtPageSize, c, 1));
		if(!ok)
			Error(PPERR_DL6_INVDBFILEPAGE, (temp_buf = 0).Cat(constInt), erfLog);
		// @todo Надо еще проверить на максимальное количество сегментов
	}
	else if(sstreqi_ascii(rSymb.U.S, "prealloc")) {
		if(constInt < 0 || constInt > 65535) {
			// @err
		}
		else {
			val = (uint32)constInt;
			THROW(AddConst(val, &c));
			THROW(p_scope->AddConst(DlScope::cdbtPrealloc, c, 1));
		}
	}
	else if(sstreqi_ascii(rSymb.U.S, "threshold")) {
		if(oneof5(constInt, 0, 5, 10, 20, 30)) {
			val = (uint32)constInt;
			THROW(AddConst(val, &c));
			THROW(p_scope->AddConst(DlScope::cdbtPctThreshold, c, 1));
		}
		else {
			// @err
		}
	}
	else if(sstreqi_ascii(rSymb.U.S, "acstable")) {
		if(pConstStr) {
			temp_buf = sstreqi_ascii(pConstStr, "default") ? "rusncase.alt" : pConstStr;
			THROW(AddConst(temp_buf, &c));
			THROW(p_scope->AddConst(DlScope::cdbtAcsName, c, 1));
		}
	}
	else if(sstreqi_ascii(rSymb.U.S, "vlr")) {
		attr.A = DlScope::sfDbtVLR;
		p_scope->SetAttrib(attr);
	}
	else if(sstreqi_ascii(rSymb.U.S, "vat")) {
		attr.A = DlScope::sfDbtVAT;
		p_scope->SetAttrib(attr);
	}
	else if(sstreqi_ascii(rSymb.U.S, "replace")) {
		attr.A = DlScope::sfDbtReplace;
		p_scope->SetAttrib(attr);
	}
	else if(sstreqi_ascii(rSymb.U.S, "truncate")) {
		attr.A = DlScope::sfDbtTruncate;
		p_scope->SetAttrib(attr);
	}
	else if(sstreqi_ascii(rSymb.U.S, "compress")) {
		attr.A = DlScope::sfDbtCompress;
		p_scope->SetAttrib(attr);
	}
	else if(sstreqi_ascii(rSymb.U.S, "balanced")) {
		attr.A = DlScope::sfDbtBalanced;
		p_scope->SetAttrib(attr);
	}
	else if(sstreqi_ascii(rSymb.U.S, "temporary")) {
		attr.A = DlScope::sfDbtTemporary;
		p_scope->SetAttrib(attr);
	}
	else if(sstreqi_ascii(rSymb.U.S, "system")) {
		attr.A = DlScope::sfDbtSystem;
		p_scope->SetAttrib(attr);
	}
	else if(sstreqi_ascii(rSymb.U.S, "access")) {
		if(constInt < 0 || constInt > 100) {
			// @err
		}
		else {
			val = (uint32)constInt;
			THROW(AddConst(val, &c));
			THROW(p_scope->AddConst(DlScope::cdbtAccess, c, 1));
		}
	}
	else {
		Error(PPERR_DL6_INVDBFILEOPTION, rSymb.U.S, erfLog);
		ok = 0;
	}
	CATCH
		Error(0, 0, erfLog);
		ok = 0;
	ENDCATCH
	return ok;
}

int SLAPI DlContext::AddCvtFuncToArgList(const DlFunc & rFunc, CtmExpr * pExpr, const LongArray & rCvtPosList) const
{
	int    ok = 1;
	for(uint i = 0; ok && i < rCvtPosList.getCount(); i++) {
		uint   arg_no = rCvtPosList.at(i);
		DLSYMBID param_typ_id = rFunc.GetArgType(arg_no);
		CtmExpr * p_arg = pExpr->GetArg(arg_no);
		if(!p_arg)
			ok = (SetError(PPERR_DL6_NOARGBYPOS, 0), 0);
		else if(!p_arg->SetImplicitCast(param_typ_id))
			ok = (SetError(PPERR_DL6_IMPLICITCVTOVRD, 0), 0);
	}
	return ok;
}

int SLAPI DlContext::ProcessQuestArgList(const DlFunc & rFunc, CtmExpr * pExpr, const LongArray & rCvtPosList)
{
	EXCEPTVAR(LastError);
	int    ok = 1;
	DLSYMBID res_typ = 0;
	CtmExpr * p_arg1 = pExpr->GetArg(1);
	CtmExpr * p_arg2 = pExpr->GetArg(2);
	DLSYMBID arg_typ1 = p_arg1->GetTypeID();
	if(p_arg2 == 0 || p_arg2->GetKind() == CtmExpr::kEmpty) {
		res_typ = arg_typ1;
	}
	else {
		DLSYMBID arg_typ2 = p_arg2->GetTypeID();
		int    loss12 = 0, loss21 = 0;
		int    c12 = TypeCast(arg_typ1, arg_typ2, 0, 0, 0, &loss12);
		int    c21 = TypeCast(arg_typ2, arg_typ1, 0, 0, 0, &loss21);
		THROW(c12 && c21);
		if(c12 == c21) {
			if(loss12 > loss21) {
				if(c21 > tcrEqual)
					THROW_V(p_arg2->SetImplicitCast(arg_typ1), PPERR_DL6_IMPLICITCVTOVRD);
				res_typ = arg_typ1;
			}
			else {
				if(c12 > tcrEqual)
					THROW_V(p_arg1->SetImplicitCast(arg_typ2), PPERR_DL6_IMPLICITCVTOVRD);
				res_typ = arg_typ2;
			}
		}
		else if(c12 > 0 && (c12 <= c21 || c21 < 0)) {
			if(c12 > tcrEqual)
				THROW_V(p_arg1->SetImplicitCast(arg_typ2), PPERR_DL6_IMPLICITCVTOVRD);
			res_typ = arg_typ2;
		}
		else if(c21 > 0 && (c21 <= c12 || c12 < 0)) {
			if(c21 > tcrEqual)
				THROW_V(p_arg2->SetImplicitCast(arg_typ1), PPERR_DL6_IMPLICITCVTOVRD);
			res_typ = arg_typ1;
		}
		else {
			SString temp_buf, msg_buf;
			TypeEntry te;
			Format_C_Type(arg_typ1, te.T, 0, fctfResolveTypeID, temp_buf);
			(msg_buf = temp_buf).CatDiv(':', 1);
			Format_C_Type(arg_typ2, te.T, 0, fctfResolveTypeID, temp_buf);
			SetError(PPERR_DL6_UNMATCHQUESTTYPES, msg_buf.Cat(temp_buf));
			CALLEXCEPT();
		}
	}
	pExpr->SetType(res_typ);
	{
		//
		// Преобразуем (если необходимо) тип первого аргумента.
		//
		LongArray temp_cvt_pos_list;
		uint i = rCvtPosList.getCount();
		do {
			int pos = rCvtPosList.at(--i);
			if(pos == 0) { // Только для первого аргумента
				temp_cvt_pos_list.insert(&pos);
				THROW(AddCvtFuncToArgList(rFunc, pExpr, temp_cvt_pos_list));
				i = 0; // exit
			}
		} while(i);
	}
	CATCHZOK
	return ok;
}

int SLAPI DlContext::IsFuncSuited(const DlFunc & rFunc, CtmExpr * pExpr, LongArray * pCvtArgList)
{
	EXCEPTVAR(LastError);
	int    s = 1;
	LongArray cvt_arg_list;
	const  uint arg_count = rFunc.GetArgCount();
	if(arg_count == pExpr->GetArgCount()) {
		for(uint j = 0; j < arg_count; j++) {
			DLSYMBID param_typ_id = rFunc.GetArgType(j);
			const CtmExpr * p_arg = pExpr->GetArg(j);
			THROW_V(p_arg, PPERR_DL6_NOARGBYPOS);
			int    r = TypeCast(p_arg->GetTypeID(), param_typ_id, 0, 0, 0);
			THROW(r);
			if(r == tcrUnable) {
				s = -1;
				break;
			}
			else if(r == tcrEqual)
				s = MAX(s, 1);
			else {
				cvt_arg_list.insert(&j);
				if(r == tcrCast)
					s = MAX(s, 1);
				else if(r == tcrCastSuper)
					s = MAX(s, 2);
				else if(r == tcrCastLoss)
					s = MAX(s, 3);
				else {
					CALLEXCEPT();
				}
			}
		}
	}
	else
		s = -1;
	CATCH
		s = 0;
	ENDCATCH
	if(s > 0)
		ASSIGN_PTR(pCvtArgList, cvt_arg_list);
	return s;
}

int DlContext::ResolveFunc(DLSYMBID scopeID, int exactScope, CtmExpr * pExpr)
{
	EXCEPTVAR(LastError);
	int    ok = -1;
	uint   i, j;
	CtmFunc rf;
	SString func_name, arg_list_buf;
	assert(oneof2(pExpr->GetKind(), CtmExpr::kFuncName, CtmExpr::kOp));
	if(pExpr->GetKind() == CtmExpr::kFuncName) {
		func_name.CatChar('?').Cat(pExpr->U.S);
		GetFuncName(0, pExpr, AddedMsgString);
	}
	else if(pExpr->GetKind() == CtmExpr::kOp) {
		func_name.CatCharN('?', 2).CatChar(pExpr->U.Op);
		GetFuncName(0, pExpr, AddedMsgString);
	}
	{
		LongArray fpl[3]; // func pos list
		const DlScope * p_org_scope = GetScope(scopeID);
		const DlScope * p_scope = p_org_scope;
		while(ok < 0 && p_scope) {
			uint   pos = 0;
			uint   prev_scope_kind = p_scope->GetKind();
			DlFunc func;
			int    ambig[3]; // Признаки неоднозначности разрешения функции
			int    s_[3];
			memzero(ambig, sizeof(ambig));
			s_[0] = s_[1] = s_[2] = -1;
			LongArray func_pos_list;
			LongArray cvt_arg_list, cvt_al[3];
			p_scope->GetFuncListByName(func_name, &func_pos_list);
			for(i = 0; i < func_pos_list.getCount(); i++) {
				THROW_V(p_scope->GetFuncByPos(func_pos_list.at(i), &func), PPERR_DL6_NOFUNCBYPOS);
				int    s = IsFuncSuited(func, pExpr, &cvt_arg_list);
				THROW(s);
				if(s > 0)
					for(j = 0; j < 3; j++)
						if(s == (j+1)) {
							//
							// Так как неоднозначность может встретиться на менее
							// предпочтительном уровне преобразования аргументов,
							// сохраним информацию о неоднозначности для того, чтобы
							// иметь возможность подобрать более предпочтительную
							// однозначную функцию с точки зрения преобразования аргументов.
							//
							if(s_[j] >= 0)
								ambig[j] = 1;
							else {
								s_[j] = i;
								cvt_al[j] = cvt_arg_list;
								break;
							}
						}
			}
			if(s_[0] >= 0 || s_[1] >= 0 || s_[2] >= 0) {
				rf.ScopeID = p_scope->ID;
				for(i = 0; ok < 0 && i < 3; i++)
					if(s_[i] >= 0) {
						THROW_V(ambig[i] == 0, PPERR_DL6_FUNCAMBIG);
						rf.Pos = func_pos_list.at(s_[i]);
						THROW_V(p_scope->GetFuncByPos(rf.Pos, &func), PPERR_DL6_NOFUNCBYPOS);
						if(pExpr->GetKind() == CtmExpr::kOp && pExpr->U.Op == dlopQuest) {
							THROW(ProcessQuestArgList(func, pExpr, cvt_al[i]));
						}
						else {
							THROW(AddCvtFuncToArgList(func, pExpr, cvt_al[i]));
							long proc_addr = 0;
							if(AdjRetTypeProcList.Search(func.ImplID, &proc_addr, 0)) {
								AdjRetTypeProc proc = (AdjRetTypeProc)proc_addr;
								DLSYMBID ret_type_id = proc(this, pExpr);
								THROW(ret_type_id);
								pExpr->SetType(ret_type_id);
							}
							else
								pExpr->SetType(func.TypID);
						}
						pExpr->SetResolvedFunc(rf);
						ok = 1;
					}
			}
			else {
				if(p_scope->GetKind() == DlScope::kExpData) {
					//
					// Обращение к функции может быть связано с заголовочной областью kExpData
					// в то время как физически функция определена в одной из внутренних областей
					// вида kExpDataHdr.
					//
					uint   pos = 0;
					DlScope * p_child = 0;
					for(uint i = 0; ok < 0 && p_scope->EnumChilds(&i, &p_child);)
						if(p_child->GetKind() == DlScope::kExpDataHdr) {
							ok = ResolveFunc(p_child->ID, 1, pExpr); // @recursion
							if(!ok && LastError == PPERR_DL6_NOFUNCBYNAME)
								ok = -1;
						}
				}
				if(ok < 0) {
					THROW_V(!exactScope, PPERR_DL6_NOFUNCBYNAME);
					p_scope = NZOR(p_scope->GetBase(), p_scope->GetOwner());
					//p_scope = p_scope->GetOwner();
				}
			}
		}
	}
	THROW_V(ok > 0, PPERR_DL6_NOFUNCBYNAME);
	CATCHZOK
	return ok;
}

int DlContext::ResolveVar(DLSYMBID scopeID, int exactScope, CtmExpr * pExpr)
{
	CtmVar var;
	var.ScopeID = 0;
	var.Pos = 0;
	assert(pExpr->GetKind() == CtmExpr::kVarName);
	int    ok = ResolveVar(scopeID, exactScope, pExpr->U.S, &var);
	if(ok > 0) {
		SdbField fld;
		TypeEntry te;
		THROW(GetField(var, &fld));
		DLSYMBID type_id = SearchSTypEx(fld.T, &te);
		THROW(type_id);
		THROW(pExpr->SetResolvedVar(var, type_id));
	}
	CATCHZOK
	return ok;
}

int DlContext::ResolveExpr(DLSYMBID scopeID, DLSYMBID callerScopeID, int exactScope, CtmExpr * pExpr, int dontResolveNext)
{
	EXCEPTVAR(LastError);
	int    ok = 1;
	DLSYMBID typ_id = 0;
	DLSYMBID scope_id = 0;
	if(pExpr && !pExpr->IsResolved(1)) {
		if(pExpr->P_Next && !dontResolveNext) {
			THROW(ResolveExpr(scopeID, callerScopeID, exactScope, pExpr->P_Next)); // @recursion
		}
		switch(pExpr->GetKind()) {
			case CtmExpr::kEmpty:
				break;
			case CtmExpr::kConst:
				break;
			case CtmExpr::kList:
				for(CtmExpr * p_next = pExpr->P_Next; p_next; p_next = p_next->P_Next)
					THROW(ResolveExpr(scopeID, callerScopeID, exactScope, p_next)); // @recursion
				break;
			case CtmExpr::kVar:
				break;
			case CtmExpr::kFunc:
				break;
			case CtmExpr::kOp:
				{
					uint   pos = 0;
					TypeEntry te;
					CtmFunc rf;
					if(pExpr->U.Op == dlopDot) {
						THROW(ResolveExpr(scopeID, callerScopeID, exactScope, pExpr->P_Arg, 1)); // @recursion
						THROW(SearchTypeID(pExpr->P_Arg->GetTypeID(), &pos, &te));
						THROW_V(te.T.Mod == STypEx::modLink || (te.T.Flags & STypEx::fStruct && te.T.Mod == 0), PPERR_DL6_INVLEFTOFDOT);
						THROW(SearchTypeID(te.T.Link, &pos, &te));
						THROW(ResolveExpr(te.T.Link, callerScopeID, 1, pExpr->P_Arg->P_Next)); // @recursion
						pExpr->SetType(pExpr->P_Arg->P_Next->GetTypeID());
						THROW(GetDotFunc(&rf));
						THROW(pExpr->SetResolvedFunc(rf));
					}
					else if(pExpr->U.Op == dlopObjRef) {
						DLSYMBID ref_type_id = 0;
						THROW(SearchTypeID(pExpr->U.Ref.Typ, &pos, &te));
						THROW_V(te.T.Mod == STypEx::modLink || (te.T.Flags & STypEx::fStruct && te.T.Mod == 0), PPERR_DL6_INVLEFTOFREF);
						THROW(ResolveExpr(callerScopeID, callerScopeID, /*exactScope*/0, pExpr->P_Arg)); // @recursion
						THROW(ref_type_id = SetDeclTypeMod(te.T.Link, STypEx::modLink, 0));
						pExpr->SetType(ref_type_id);
						THROW(GetRefFunc(&rf));
						THROW(pExpr->SetResolvedFunc(rf));
					}
					else {
						THROW(ResolveExpr(callerScopeID, callerScopeID, /*exactScope*/0, pExpr->P_Arg)); // @recursion
						THROW(ResolveFunc(scopeID, exactScope, pExpr));
					}
				}
				break;
			case CtmExpr::kFuncName:
				THROW(ResolveExpr(callerScopeID, callerScopeID, /*exactScope*/0, pExpr->P_Arg)); // @recursion
				THROW(ResolveFunc(scopeID, exactScope, pExpr)); // @recursion
				break;
			case CtmExpr::kVarName:
				THROW(ResolveVar(scopeID, exactScope, pExpr));
				break;
		}
		/*// @debug {
		{
			SString expr_buf, test_buf;
			pExpr->Pack(expr_buf);
			SLS.LogMessage(LogFileName, expr_buf);
			{
				CtmExpr test_expr;
				test_expr.Init();
				SStrScan scan(expr_buf);
				test_expr.Unpack(scan);
				test_expr.Pack(test_buf);
				if(test_buf.Cmp(expr_buf, 0) == 0)
					SLS.LogMessage(LogFileName, "CtmExpr::Pack/Unpack work NORMAL");
				else
					SLS.LogMessage(LogFileName, "CtmExpr::Pack/Unpack FAILURE");
				test_expr.Destroy();
			}
		}
		// } @debug */
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

int DlScope::SetDeclList(const StringSet * pSet)
{
	SString temp_buf;
	ResetPrototypeFlag();
	for(uint pos = 0; pSet->get(&pos, temp_buf);)
		if(temp_buf.CmpNC("destroy") == 0)
			DvFlags |= declfDestroy;
		else if(temp_buf.CmpNC("set") == 0)
			DvFlags |= declfSet;
		else if(temp_buf.CmpNC("DOSSTUB") == 0)
			DvFlags |= declfDosStub;
		else if(temp_buf.CmpNC("noaldd") == 0)
			DvFlags |= declfNoALDD;
		else if(temp_buf.CmpNC("WSDL") == 0)
			DvFlags |= declfWSDL;
		else {
			// warn "Invalid decalre statement '%s'"
		}
	return 1;
}

int DlContext::CompleteExportDataStruc()
{
	int    ok = 1;
	DlScope * p = GetScope(CurScopeID);
	if(p) {
		p->SetDeclList(&CurDeclList);
	}
	else {
		SString msg_buf;
		msg_buf.Cat(CurScopeID);
		ok = (Error(PPERR_DL6_DATASTRUCIDNFOUND, msg_buf), 0);
	}
	//
	// Cleaning instant members of Context
	//
	CurDeclList.clear();
	return ok;
}
//
//
//
int DlContext::StoreUuidList()
{
	int    ok = 1;
	if(UuidList.getCount()) {
		SString file_name = InFileName;
		SPathStruc::ReplaceExt(file_name, "uuid", 1);
		SFile  f_out(file_name, SFile::mWrite);
		if(f_out.IsValid()) {
			SString line_buf, uuid_buf;
			UUIDAssoc * p_item;
			for(uint i = 0; UuidList.enumItems(&i, (void **)&p_item);) {
				DlScope * p_scope = GetScope(p_item->Key, 0);
				if(p_scope) {
					p_item->Val.ToStr(S_GUID::fmtIDL, uuid_buf);
					(line_buf = 0).Cat(p_scope->Name).Semicol().Cat(uuid_buf).CR();
					f_out.WriteLine(line_buf);
				}
			}
		}
		else
			ok = 0;
	}
	else
		ok = -1;
	return ok;
}

int DlContext::RestoreUuidList()
{
	int    ok = 1;
	SString file_name = InFileName;
	SPathStruc::ReplaceExt(file_name, "uuid", 1);
	SyncUuidNameList.clear();
	SyncUuidList.freeAll();
	if(fileExists(file_name)) {
		SFile  f_out(file_name, SFile::mRead);
		SString line_buf, name_buf, uuid_buf;
		THROW(f_out.IsValid());
		while(f_out.ReadLine(line_buf))
			if(line_buf.Divide(';', name_buf, uuid_buf) > 0) {
				SyncUuidAssoc entry;
				MEMSZERO(entry);
				SyncUuidNameList.add(name_buf.Strip(), &entry.NamePos);
				THROW(entry.Uuid.FromStr(uuid_buf));
				THROW(SyncUuidList.insert(&entry));
			}
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

int DlContext::SetupScopeUUID(DLSYMBID scopeID, const char * pName, const S_GUID * pForceUUID)
{
	int    ok = 1;
	uint   pos = 0, pos2 = 0;
	S_GUID uuid;
	if(!UuidList.Search(scopeID, 0, &pos)) {
		if(pForceUUID)
			uuid = *pForceUUID;
		else if(SyncUuidNameList.search(pName, &(pos = 0), 0) && SyncUuidList.lsearch(&pos, &pos2, CMPF_LONG))
			uuid = SyncUuidList.at(pos2).Uuid;
		else
			THROW(uuid.Generate());
		THROW(UuidList.Add(scopeID, uuid, 0));
	}
	else if(pForceUUID) {
		UuidList.atFree(pos);
		THROW(UuidList.Add(scopeID, *pForceUUID, 0));
	}
	CATCHZOK
	return ok;
}
//
//
//
SLAPI DlContext::TypeDetail::TypeDetail()
{
	DimList.Init();
	PtrList.Init();
	T.Init();
}

SLAPI DlContext::TypeDetail::~TypeDetail()
{
	DimList.Destroy();
	PtrList.Destroy();
}

int SLAPI DlContext::TypeDetail::IsInterfaceTypeConversionNeeded() const
{
	const TYPEID st = GETSTYPE(T.Typ);
	if(oneof3(st, S_DATE, S_TIME, S_DATETIME))
		return 1;
	else if(oneof4(st, S_CHAR, S_ZSTRING, S_LSTRING, S_NOTE))
		return 1;
	else
		return -1;
}

int DlContext::IsInterfaceTypeConversionNeeded(const STypEx & rTyp)
{
	int    ok = -1;
	TypeDetail td;
	if(UnrollType(0, rTyp, &td))
		ok = td.IsInterfaceTypeConversionNeeded();
	else
		ok = 0;
	return ok;
}
//
//
//
DLSYMBID DlContext::EnterScope(uint scopeKind, const char * pName, DLSYMBID scopeId, const SV_Uint32 * pAttrList)
{
	SETIFZ(scopeId, GetNewSymbID());
	DLSYMBID id = Sc.EnterScope(CurScopeID, scopeId, scopeKind, pName);
	if(id == 0) {
		(AddedMsgString = 0).Cat(CurScopeID);
		Error(PPERR_DL6_SCOPEIDNFOUND, 0, erfExit);
	}
	else {
		CurScopeID = id;
		if(oneof3(scopeKind, DlScope::kInterface, DlScope::kIClass, DlScope::kLibrary)) {
			S_GUID uuid;
			int    is_uuid = 0;
			if(pAttrList) {
				DlScope * p_scope = DCtx.GetScope(id);
				if(p_scope) {
					uint   c = pAttrList->GetCount() / 2;
					for(uint i = 0; i < c; i++) {
						DlScope::Attr attr;
						attr.A   = pAttrList->Get(i*2);
						attr.Ver = pAttrList->Get(i*2+1);
						if(attr.A == DlScope::sfUUID) {
							if(!is_uuid) {
								is_uuid = 1;
								uuid = TempUuidList.at(attr.UuidIdx-1).Val;
							}
						}
						else
							p_scope->SetAttrib(attr);
					}
				}
			}
			SetupScopeUUID(id, pName, is_uuid ? &uuid : 0);
		}
	}
	return id;
}

int DlContext::LeaveScope()
{
	DLSYMBID scope_id = CurScopeID;
	if(!Sc.LeaveScope(scope_id, &CurScopeID)) {
		(AddedMsgString = 0).Cat(scope_id);
		return Error(PPERR_DL6_SCOPEIDNFOUND, 0, erfExit);
	}
	else
		return 1;
}

int DlContext::PushScope()
{
	ScopeStack.push(&CurScopeID);
	return LeaveScope();
}

int DlContext::PopScope()
{
	DLSYMBID scope_id = *(DLSYMBID *)ScopeStack.pop();
	if(scope_id) {
		scope_id = Sc.EnterScope(CurScopeID, scope_id, 0, 0);
		if(scope_id == 0) {
			(AddedMsgString = 0).Cat(CurScopeID);
			Error(PPERR_DL6_SCOPEIDNFOUND, 0, erfExit);
		}
		else
			CurScopeID = scope_id;
	}
	return scope_id;
}

int SLAPI DlContext::SetInheritance(DLSYMBID scopeID, DLSYMBID baseID)
{
	int    ok = 1;
	DlScope * p = GetScope(scopeID);
	if(p) {
		const DlScope * p_base = baseID ? GetScope(baseID) : 0;
		p->SetInheritance(p_base, this);
	}
	else {
		SString msg_buf;
		msg_buf.Cat(scopeID);
		ok = (Error(PPERR_DL6_DATASTRUCIDNFOUND, msg_buf), 0);
	}
	return ok;
}
//
//
//
int DlContext::Write_Func(Generator_CPP & gen, const DlFunc & rFunc, int format, const char * pForward)
{
	int    ok = 1;
	const  int idl_type = oneof4(format, ffIDL, ffH_Iface, ffCPP_Iface, ffCPP_VTbl) ? 1 : 0;
	uint   c;
	SString temp_buf, arg_name, arg_buf;
	TypeEntry ret_te, te;
	TypeDetail td;
	THROW(SearchTypeID(rFunc.TypID, 0, &ret_te));
	rFunc.GetName(0, temp_buf);
	if(format == ffIDL) {
		arg_buf = 0;
		if(rFunc.Flags & DlFunc::fPropGet)
			arg_buf = "propget";
		else if(rFunc.Flags & DlFunc::fPropPut)
			arg_buf = "propput";
		if(arg_buf.NotEmpty())
			THROW(gen.WriteLine(gen.CatIndent(arg_name = 0).CatChar('[').Cat(arg_buf).CatChar(']').CR()));
	}
	else if(!oneof4(format, ffCPP_VTbl, ffCPP_Iface, ffCPP_CallImp, ffCPP_Imp)) {
		arg_buf = 0;
		if(rFunc.Flags & DlFunc::fPropGet) {
			(arg_buf = "get").CatChar('_').Cat(temp_buf);
		}
		else if(rFunc.Flags & DlFunc::fPropPut) {
			(arg_buf = "put").CatChar('_').Cat(temp_buf);
		}
		if(arg_buf.NotEmpty())
			temp_buf = arg_buf;
	}
	if(format == ffCPP_CallImp) {
		gen.CatIndent(arg_name = 0);
		if(pForward)
			arg_name.Cat(pForward);
		gen.WriteLine(arg_name.Cat(temp_buf).CatChar('('));
	}
	else if(idl_type) {
		THROW(gen.Wr_StartDeclFunc(0, 0, "HRESULT", temp_buf,
			(format == ffCPP_VTbl) ? Generator_CPP::fcmDefault : Generator_CPP::fcmStdCall));
	}
	else {
		Format_C_Type(rFunc.TypID, ret_te.T, 0, fctfIfaceImpl, arg_buf);
		THROW(gen.Wr_StartDeclFunc(0, 0, arg_buf, temp_buf, Generator_CPP::fcmDefault));
	}
	c = rFunc.GetArgCount();
	temp_buf = 0;
	if(c) {
		for(uint i = 0; i < c; i++) {
			DlFunc::Arg arg;
			rFunc.GetArgName(i, arg_name);
			if(i)
				temp_buf.Comma().Space();
			if(format == ffCPP_CallImp) {
				THROW(rFunc.GetArg(i, &arg));
				THROW(SearchTypeID(arg.TypID, 0, &te));
				THROW(UnrollType(arg.TypID, te.T, &td));
				const TYPEID st = GETSTYPE(td.T.Typ);
				if(st == S_ZSTRING) {
					if(td.T.Mod == STypEx::modPtr)
						temp_buf.CatChar('&');
				}
				temp_buf.Cat(arg_name);
			}
			else {
				THROW(rFunc.GetArg(i, &arg));
				THROW(SearchTypeID(arg.TypID, 0, &te));
				if(format == ffIDL) {
					if(arg.Flags & (DlFunc::fArgIn | DlFunc::fArgOut)) {
						int    comma = 0;
						temp_buf.CatChar('[');
						if(arg.Flags & DlFunc::fArgIn) {
							temp_buf.Cat("in");
							comma = 1;
						}
						if(arg.Flags & DlFunc::fArgOut) {
							if(comma)
								temp_buf.Comma();
							temp_buf.Cat("out");
						}
						temp_buf.CatChar(']').Space();
					}
				}
				else if(format == ffCPP_Iface) {
					if(IsInterfaceTypeConversionNeeded(te.T) > 0)
						arg_name.CatChar('_');
				}
				Format_C_Type(arg.TypID, te.T, (format == ffCPP_VTbl) ? 0 : (const char *)arg_name,
					idl_type ? fctfIDL : fctfIfaceImpl, arg_buf);
				temp_buf.Cat(arg_buf);
			}
		}
	}
	if(idl_type) {
		if(ret_te.MangleC != 'X') { // other than "void"
			TypeEntry ret_te_mod;
			DLSYMBID ptr_typ = SetDeclTypeMod(rFunc.TypID, STypEx::modPtr);
			THROW(ptr_typ);
			THROW(SearchTypeID(ptr_typ, 0, &ret_te_mod));
			if(format == ffCPP_VTbl)
				arg_name = 0;
			else {
				arg_name = "pRet";
				if(format == ffCPP_Iface) {
					if(IsInterfaceTypeConversionNeeded(ret_te_mod.T) > 0)
						arg_name.CatChar('_');
				}
			}
			Format_C_Type(ptr_typ, ret_te_mod.T, arg_name, fctfIDL, arg_buf);
			if(c)
				temp_buf.CatDiv(',', 2);
			if(format == ffIDL)
				temp_buf.CatChar('[').Cat("out").Comma().Cat("retval").CatChar(']').Space();
			temp_buf.Cat(arg_buf);
		}
	}
	gen.WriteLine(temp_buf);
	gen.Wr_EndDeclFunc(oneof2(format, ffCPP_Iface, ffCPP_Imp) ? 0 : 1, 1);
	CATCHZOK
	return ok;
}

int DlContext::Write_IDL_Attr(Generator_CPP & gen, const DlScope & rScope)
{
	SString line_buf, temp_buf;
	uint   pos = 0;
	int    next = 0;
	S_GUID uuid;
	DlScope::Attr attr;
	gen.WriteLine(gen.CatIndent(line_buf = 0).CatChar('[').CR());
	gen.IndentInc();
	if(UuidList.Search(rScope.ID, &uuid, 0))
		uuid.ToStr(S_GUID::fmtIDL, temp_buf);
	else
		temp_buf = "UUID not defined";
	line_buf = 0;
	if(rScope.IsKind(DlScope::kInterface)) {
		gen.CatIndent(line_buf).Cat("object");
		next = 1;
	}
	if(next)
		line_buf.Comma().CR();
	gen.CatIndent(line_buf).Cat("uuid").CatParStr(temp_buf);
	next = 1;
	if(rScope.GetAttrib(DlScope::sfVersion, &attr)) {
		(temp_buf = 0).Cat(LoWord(attr.Ver)).Dot().Cat(HiWord(attr.Ver));
		if(next)
			line_buf.Comma().CR();
		gen.CatIndent(line_buf).Cat("version").CatParStr(temp_buf);
		next = 1;
	}
	if(rScope.GetAttrib(DlScope::sfHidden, &attr)) {
		if(next)
			line_buf.Comma().CR();
		gen.CatIndent(line_buf).Cat("hidden");
		next = 1;
	}
	if(rScope.GetAttrib(DlScope::sfRestricted, &attr)) {
		if(next)
			line_buf.Comma().CR();
		gen.CatIndent(line_buf).Cat("restricted");
		next = 1;
	}
	line_buf.CR();
	gen.WriteLine(line_buf);
	gen.IndentDec();
	gen.WriteLine(gen.CatIndent(line_buf = 0).CatChar(']').CR());
	return 1;
}

int DlContext::Write_IDL_File(Generator_CPP & gen, const DlScope & rScope)
{
	int    ok = 1;
	uint   j;
	DlFunc func;
	SdbField fld;
	SString line_buf, temp_buf;
	DlScope * p_ds = 0;
	for(uint i = 0; rScope.EnumChilds(&i, &p_ds);) {
		int    do_write_end = 0;
		if(p_ds->GetAttrib(DlScope::sfNoIDL, 0)) {
			THROW(Write_IDL_File(gen, *p_ds));
		}
		else if(p_ds->IsKind(DlScope::kTypedefPool)) {
			for(j = 0; j < p_ds->GetCount(); j++) {
				gen.CatIndent(line_buf = 0);
				if(p_ds->GetFieldByPos(j, &fld)) {
					Format_C_Type(0, fld.T, fld.Name, fctfIDL, temp_buf);
					line_buf.Cat("typedef").Space().Cat(temp_buf).Semicol().CR();
				}
				else
					line_buf.Cat("Error opening field");
				gen.WriteLine(line_buf.CR());
			}
			gen.WriteLine(0);
		}
		else if(p_ds->IsKind(DlScope::kStruct)) {
			gen.CatIndent(line_buf = 0).Cat("typedef").Space().Cat("struct").Space().
				/*Cat("tag").*/Cat(p_ds->Name).Space().CatChar('{').CR();
			gen.WriteLine(line_buf);
			gen.IndentInc();
			for(j = 0; j < p_ds->GetCount(); j++) {
				gen.CatIndent(line_buf = 0);
				if(p_ds->GetFieldByPos(j, &fld)) {
					Format_C_Type(0, fld.T, fld.Name, fctfIDL, temp_buf);
					line_buf.Cat(temp_buf).Semicol();
				}
				else
					line_buf.Cat("Error opening field");
				gen.WriteLine(line_buf.CR());
			}
			gen.IndentDec();
			gen.Wr_CloseBrace(1, p_ds->Name);
			gen.WriteLine(0);
		}
		else if(p_ds->IsKind(DlScope::kEnum)) {
			gen.CatIndent(line_buf = 0).Cat("typedef").Space().Cat("[v1_enum]").Space().
				Cat("enum").Space().CatChar('_').Cat(p_ds->Name).Space().CatChar('{').CR();
			gen.WriteLine(line_buf);
			gen.IndentInc();
			uint c = p_ds->GetCount();
			for(j = 0; j < c; j++) {
				gen.CatIndent(line_buf = 0);
				if(p_ds->GetFieldByPos(j, &fld)) {
					line_buf.Cat(fld.Name).CatDiv('=', 1).Cat(fld.EnumVal);
					if(j < (c-1))
						line_buf.Comma();
				}
				else
					line_buf.Cat("Error opening field");
				gen.WriteLine(line_buf.CR());
			}
			gen.IndentDec();
			gen.Wr_CloseBrace(1, p_ds->Name);
			gen.WriteLine(0);
		}
		else if(p_ds->IsKind(DlScope::kInterface)) {
			Write_IDL_Attr(gen, *p_ds);
			gen.Wr_Indent();
			gen.Wr_StartIdlInterfaceDecl(p_ds->Name, 0);
			gen.IndentInc();
			for(j = 0; p_ds->EnumFunctions(&j, &func) > 0;)
				Write_Func(gen, func, ffIDL);
			do_write_end = 1;
		}
		else if(p_ds->IsKind(DlScope::kIClass)) {
			DlScope::IfaceBase ifbase;
			Write_IDL_Attr(gen, *p_ds);
			gen.Wr_Indent();
			gen.Wr_StartIdlCoClassDecl(p_ds->Name);
			gen.IndentInc();
			for(j = 0; j < p_ds->GetIfaceBaseCount(); j++)
				if(p_ds->GetIfaceBase(j, &ifbase)) {
					DlScope * p_iface = GetScope(ifbase.ID);
					if(p_iface) {
						line_buf = 0;
						if(ifbase.Flags & DlScope::IfaceBase::fDefault)
							line_buf.CatChar('[').Cat("default").CatChar(']').Space();
						line_buf.Cat("interface").Space().Cat(p_iface->Name).Semicol().CR();
						gen.Wr_Indent();
						gen.WriteLine(line_buf);
					}
				}
			//
			// ISupportErrorInfo {
			//
			(line_buf = 0).Cat("interface").Space().Cat("ISupportErrorInfo").Semicol().CR();
			gen.Wr_Indent();
			gen.WriteLine(line_buf);
			//
			// }
			//
			do_write_end = 1;
		}
		else if(p_ds->IsKind(DlScope::kLibrary)) {
			Write_IDL_Attr(gen, *p_ds);
			gen.Wr_Indent();
			gen.WriteLine((line_buf = 0).Cat("library").Space().Cat(p_ds->Name).Space().CatChar('{').CR());
			gen.IndentInc();
			gen.CatIndent(line_buf = 0).Cat("importlib(\"STDOLE2.TLB\");").CR();
			gen.WriteLine(line_buf);
			gen.WriteLine(0);
			Write_IDL_File(gen, *p_ds); // @recursion
			do_write_end = 1;
		}
		else {
			THROW(Write_IDL_File(gen, *p_ds));
		}
		if(do_write_end) {
			gen.IndentDec();
			gen.Wr_CloseBrace(1, 0);
			gen.WriteLine(0);
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI DlContext::Write_C_FileHeader(Generator_CPP & gen, const char * pFileName)
{
	SString temp_buf;
	SPathStruc ps;
	ps.Split(pFileName);
	ps.Merge(0, SPathStruc::fDrv|SPathStruc::fDir, temp_buf);
	gen.Wr_Comment(temp_buf.ToUpper());
	temp_buf.Printf("This file was generated by DL600C.EXE from '%s'", InFileName.cptr());
	gen.Wr_Comment(temp_buf);
	gen.Wr_Comment(0);
	return 1;
}

int SLAPI DlContext::MakeDlRecName(const DlScope * pRec, int instanceName, SString & rBuf) const
{
	int    ok = 1;
	rBuf = 0;
	SString left, right;
	if(pRec->IsKind(DlScope::kExpDataHdr))
		rBuf = instanceName ? "H" : "Head";
	else if(pRec->IsKind(DlScope::kExpDataIter))
		rBuf = instanceName ? "I" : "Iter";
	else
		ok = -1;
	if(ok > 0) {
		pRec->Name.Divide('@', left, right);
		if(right.NotEmpty() && right.Cmp("def", 0) != 0)
			rBuf.CatChar('_').Cat(right);
	}
	return ok;
}

int SLAPI DlContext::Write_C_DeclFile(Generator_CPP & gen, const DlScope & rScope)
{
	int    ok = 1, r;
	uint   i, j, k;
	SString cls_name, inst_name, fld_buf, line_buf;
	SdbField fld;
	DlFunc func;
	DlScope * p_ds = 0;
	for(i = 0; rScope.EnumChilds(&i, &p_ds);) {
		if(p_ds->IsKind(DlScope::kTypedefPool)) {
			for(j = 0; p_ds->EnumFields(&j, &fld);) {
				gen.CatIndent(line_buf = 0);
				Format_C_Type(0, fld.T, fld.Name, fctfIDL|fctfSourceOutput, fld_buf);
				gen.WriteLine(line_buf.Cat("typedef").Space().Cat(fld_buf).CR());
			}
			gen.WriteLine(0);
		}
		else if(p_ds->IsKind(DlScope::kStruct)) {
			MakeClassName(p_ds, clsnfCPP, cls_name);
			gen.WriteLine((fld_buf = 0).Cat("#pragma pack(push)").CR());
			gen.WriteLine((fld_buf = 0).Cat("#pragma pack(8)").CR());
			gen.Wr_StartClassDecl(Generator_CPP::clsStruct, cls_name, 0, Generator_CPP::acsPublic);
			gen.IndentInc();
			for(j = 0; p_ds->EnumFields(&j, &fld);) {
				gen.CatIndent(line_buf = 0);
				Format_C_Type(0, fld.T, fld.Name, fctfIDL|fctfSourceOutput, fld_buf);
				gen.WriteLine(line_buf.Cat(fld_buf).CR());
			}
			gen.IndentDec();
			gen.Wr_CloseBrace(1);
			gen.WriteLine((fld_buf = 0).Cat("#pragma pack(pop)").CR());
			gen.WriteLine(0);
		}
		else if(p_ds->IsKind(DlScope::kEnum)) {
			MakeClassName(p_ds, clsnfCPP, cls_name);
			gen.Wr_StartClassDecl(Generator_CPP::clsEnum, cls_name, 0);
			gen.IndentInc();
			for(j = 0; p_ds->EnumFields(&j, &fld);) {
				gen.CatIndent(line_buf = 0);
				line_buf.Cat(fld.Name).CatDiv('=', 1).Cat(fld.EnumVal);
				if(j < p_ds->GetCount())
					line_buf.Comma();
				gen.WriteLine(line_buf.CR());
			}
			gen.IndentDec();
			gen.Wr_CloseBrace(1);
			gen.WriteLine(0);
		}
		else if(p_ds->IsKind(DlScope::kIClass)) {
			DlScope::IfaceBase ifb;
			const DlScope * p_ifs;
			MakeClassName(p_ds, clsnfCPP, cls_name);
			gen.Wr_StartClassDecl(Generator_CPP::clsClass, cls_name, "SCoClass");
			gen.Wr_ClassAcsZone(Generator_CPP::acsPublic);
			gen.IndentInc();
			gen.CatIndent(line_buf = 0).Cat("DL6_IC_CONSTRUCTION_DECL").CatParStr(p_ds->Name).Semicol().CR();
			gen.WriteLine(line_buf);
			for(j = 0; (r = EnumInterfacesByICls(p_ds, &j, &ifb, &p_ifs)) >= 0;)
				if(r == 0) {
					fld_buf.Printf("Error extracting interface %s::%u", cls_name.cptr(), j-1);
					gen.WriteLine(fld_buf.CR());
				}
				else {
					//
					// Interface method's declarations
					//
					gen.IndentDec();
					gen.Wr_ClassAcsZone(Generator_CPP::acsPublic);
					gen.IndentInc();
					gen.Wr_Comment(0);
					gen.Wr_Comment((fld_buf = "Interface").Space().Cat(p_ifs->Name));
					gen.Wr_Comment(0);
					for(k = 0; p_ifs->EnumFunctions(&k, &func) > 0;) {
						func.Name.CatChar('_');
						Write_Func(gen, func, ffH_Iface);
					}
					//
					// Method's implementation declarations
					//
					gen.IndentDec();
					gen.Wr_ClassAcsZone(Generator_CPP::acsPrivate);
					gen.IndentInc();
					gen.Wr_Comment(0);
					gen.Wr_Comment((fld_buf = "Interface").Space().Cat(p_ifs->Name).Space().Cat("implementation"));
					gen.Wr_Comment(0);
					for(k = 0; p_ifs->EnumFunctions(&k, &func) > 0;)
						Write_Func(gen, func, ffH_Imp);
				}
			//
			gen.IndentDec();
			gen.Wr_CloseBrace(1);
			gen.WriteLine(0);
		}
		else if(p_ds->IsKind(DlScope::kExpData) && !p_ds->GetBase()) {
			MakeClassName(p_ds, clsnfCPP, cls_name);
			if(p_ds->IsPrototype()) {
				gen.Wr_ClassPrototype(Generator_CPP::clsClass, cls_name);
			}
			else {
				DlScope * p_rec = 0;

				gen.Wr_StartClassDecl(Generator_CPP::clsClass, cls_name, "DlRtm");
				gen.Wr_ClassAcsZone(Generator_CPP::acsPublic);
				gen.IndentInc();
				const int is_iter = BIN(p_ds->GetFirstChildByKind(DlScope::kExpDataIter, 0));
				int is_func_decl = 0;
				for(j = 0; !is_func_decl && p_ds->EnumChilds(&j, &p_rec);)
					if((p_rec->IsKind(DlScope::kExpDataHdr) || p_rec->IsKind(DlScope::kExpDataIter)) && p_rec->GetFuncCount())
						is_func_decl = 1;
				//
				// Methods
				//
				gen.Wr_StartDeclFunc(Generator_CPP::fkConstr, 0, 0, cls_name);
				gen.Wr_VarDecl("DlContext *", "pCtx", 0, ',');
				gen.Wr_VarDecl("DlScope *", "pScope", 0, 0);
				gen.Wr_EndDeclFunc(1, 1);

				gen.Wr_StartDeclFunc(Generator_CPP::fkDestr, 0, 0, cls_name);
				gen.Wr_EndDeclFunc(1, 1);

				gen.Wr_StartDeclFunc(Generator_CPP::fkOrdinary, Generator_CPP::fmVirtual, "int", "InitData");
				gen.Wr_VarDecl("PPFilt &", 0, 0, ',');
				gen.Wr_VarDecl("long", "rsrv", "0", 0);
				gen.Wr_EndDeclFunc(1, 1);
				if(is_iter) {
					gen.Wr_StartDeclFunc(Generator_CPP::fkOrdinary, Generator_CPP::fmVirtual, "int", "InitIteration");
					gen.Wr_VarDecl("long", 0, 0, ',');
					gen.Wr_VarDecl("int",  "sortId", 0, ',');
					gen.Wr_VarDecl("long", "rsrv", "0", 0);
					gen.Wr_EndDeclFunc(1, 1);

					gen.Wr_StartDeclFunc(Generator_CPP::fkOrdinary, Generator_CPP::fmVirtual, "int", "NextIteration");
					gen.Wr_VarDecl("long", 0, 0, 0); // @v9.6.8 ','-->0
					// @v9.6.8 gen.Wr_VarDecl("long", "rsrv", "0", 0);
					gen.Wr_EndDeclFunc(1, 1);
				}
				if(p_ds->CheckDvFlag(DlScope::declfDestroy)) {
					gen.Wr_StartDeclFunc(Generator_CPP::fkOrdinary, Generator_CPP::fmVirtual, "void", "Destroy"); // @v9.6.4 "int"-->"void"
					gen.Wr_EndDeclFunc(1, 1);
				}
				if(p_ds->CheckDvFlag(DlScope::declfSet)) {
					gen.Wr_StartDeclFunc(Generator_CPP::fkOrdinary, Generator_CPP::fmVirtual, "int", "Set");
					gen.Wr_VarDecl("long", "iterId", 0, ',');
					gen.Wr_VarDecl("int", "commit", 0, 0);
					gen.Wr_EndDeclFunc(1, 1);
				}
				if(is_func_decl) {
					// virtual int EvaluateFunc(const DlFunc * pF, SV_Uint32 * pApl, RtmStack & rS);
					gen.Wr_StartDeclFunc(Generator_CPP::fkOrdinary, Generator_CPP::fmVirtual, "void", "EvaluateFunc"); // @v9.8.6 "int"-->"void"
					gen.Wr_VarDecl("const DlFunc *", "pF", 0, ',');
					gen.Wr_VarDecl("SV_Uint32 *",  "pApl", 0, ',');
					gen.Wr_VarDecl("RtmStack &",  "rS", 0, 0);
					gen.Wr_EndDeclFunc(1, 1);
				}
				gen.WriteLine(0);
				//
				for(j = 0; p_ds->EnumChilds(&j, &p_rec);) {
					if(p_rec->IsKind(DlScope::kExpDataHdr) || p_rec->IsKind(DlScope::kExpDataIter)) {
						MakeDlRecName(p_rec, 0, cls_name);
						MakeDlRecName(p_rec, 1, inst_name);
						gen.Wr_StartClassDecl(Generator_CPP::clsStruct, cls_name, 0);
						gen.IndentInc();
						for(k = 0; p_rec->EnumFields(&k, &fld);)
							if(!(fld.T.Flags & STypEx::fFormula)) {
								THROW(Format_C_Type(0, fld.T, fld.Name, fctfSourceOutput, fld_buf));
								fld_buf.CR();
								gen.Wr_Indent();
								gen.WriteLine(fld_buf);
							}
						gen.IndentDec();
						gen.Wr_CloseBrace(1, inst_name);
					}
				}
				gen.IndentDec();
				gen.Wr_CloseBrace(1);
			}
			gen.WriteLine(0);
		}
		else if(p_ds->IsKind(DlScope::kDbTable)) {
			MakeClassName(p_ds, clsnfCPP, cls_name);
			gen.Wr_StartClassDecl(Generator_CPP::clsClass, cls_name, "DBTable");
			gen.Wr_ClassAcsZone(Generator_CPP::acsPublic);
			gen.IndentInc();
			//
			// Конструктор
			//
			gen.Wr_StartDeclFunc(Generator_CPP::fkConstr, 0, 0, cls_name);
			gen.Wr_VarDecl("const char *", "pFileName", "0", 0); // @v9.6.4 ','-->0
			// @v9.6.4 gen.Wr_VarDecl("int", "openMode", "omNormal", 0);
			gen.Wr_EndDeclFunc(1, 1);
			gen.WriteLine(0);
			//
			// Структура записи
			//
			gen.Wr_StartClassDecl(Generator_CPP::clsStruct, "Rec", 0);
			gen.IndentInc();
			for(k = 0; p_ds->EnumFields(&k, &fld);) {
				THROW(Format_C_Type(0, fld.T, fld.Name, fctfSourceOutput, fld_buf));
				if(fld.T.IsPure()) {
					int _t = GETSTYPE(fld.T.Typ);
					SString comment_type;
					if(_t == S_DEC) {
						comment_type = "decimal";
						comment_type.CatChar('[').Cat(GETSSIZED(fld.T.Typ)).Dot().Cat(GETSPRECD(fld.T.Typ)).CatChar(']');
					}
					else if(_t == S_MONEY) {
						comment_type = "money";
						comment_type.CatChar('[').Cat(GETSSIZED(fld.T.Typ)).Dot().Cat(GETSPRECD(fld.T.Typ)).CatChar(']');
					}
					else if(_t == S_NOTE)
						comment_type = "note";
					else if(_t == S_LSTRING)
						comment_type = "lstring";
					else if(_t == S_RAW)
						comment_type = "raw";
					if(comment_type.NotEmpty())
						fld_buf.Align(18, ADJ_LEFT).Space().CatCharN('/', 2).Space().Cat(comment_type);
				}
				fld_buf.CR();
				gen.Wr_Indent();
				gen.WriteLine(fld_buf);
			}
			gen.IndentDec();
			gen.Wr_CloseBrace(1, "data");
			//
			// Структуры индексов
			//
			DlScope * p_index = 0;
			for(j = 0; p_ds->EnumChilds(&j, &p_index);) {
				if(p_index->IsKind(DlScope::kDbIndex)) {
					MakeClassName(p_index, clsnfCPP, cls_name);
					gen.Wr_StartClassDecl(Generator_CPP::clsStruct, cls_name, 0);
					gen.IndentInc();
					for(k = 0; p_index->EnumFields(&k, &fld);) {
						THROW(Format_C_Type(0, fld.T, fld.Name, fctfSourceOutput, fld_buf));
						fld_buf.CR();
						gen.Wr_Indent();
						gen.WriteLine(fld_buf);
					}
					gen.IndentDec();
					gen.Wr_CloseBrace(1, 0);
				}
			}
			//
			// Дескрипторы полей
			//
			for(k = 0; p_ds->EnumFields(&k, &fld);) {
				gen.Wr_Indent();
				gen.WriteLine((fld_buf = "DBField").Space().Cat(fld.Name).Semicol().CR());
			}
			//
			gen.IndentDec();
			gen.Wr_CloseBrace(1);
			gen.WriteLine(0);
		}
		else {
			THROW(Write_C_DeclFile(gen, *p_ds)); // @recursion
		}
	}
	CATCHZOK
	return ok;
}

static void SLAPI Wr_YourCodeHere(Generator_CPP & gen)
{
	gen.IndentInc();
	gen.Wr_Comment("Your code here...");
	gen.IndentDec();
}

int SLAPI DlContext::Write_C_ImplInterfaceFunc(Generator_CPP & gen, const SString & rClsName, DlFunc & rFunc)
{
	int    ok = 1, is_ret = 0;
	const  uint arg_count = rFunc.GetArgCount();
	uint   k;
	TypeEntry te, ret_te;
	TypeDetail td;
	DlFunc::Arg arg;
	SString func_name, line_buf, temp_buf, arg_name;
	//
	// Преобазуем наименование функции к виду ClassName::FuncName_
	// (предварительно сохраняем оригинальное наименование функции в save_func_name)
	//
	SString save_func_name = rFunc.Name;
	SString mod_func_name;
	{
		//
		//  Property name modification
		//
		temp_buf = 0;
		if(rFunc.Flags & DlFunc::fPropGet)
			temp_buf = "get";
		else if(rFunc.Flags & DlFunc::fPropPut)
			temp_buf = "put";
		if(temp_buf.NotEmpty())
			mod_func_name = temp_buf.CatChar('_').Cat(rFunc.Name);
		else
			mod_func_name = rFunc.Name;
	}
	rFunc.Name = gen.MakeClsfName(rClsName, mod_func_name, func_name).CatChar('_');
	Write_Func(gen, rFunc, ffCPP_Iface);
	gen.Wr_OpenBrace();
	gen.IndentInc();
	gen.WriteLine(gen.CatIndent(line_buf = 0).Cat("IFACE_METHOD_PROLOG").CatParStr(rClsName).Semicol().CR());
	//
	// Конвертация (если необходимо) типов параметров
	//
	for(k = 0; k < arg_count; k++) {
		rFunc.GetArgName(k, arg_name);
		THROW(rFunc.GetArg(k, &arg));
		THROW(SearchTypeID(arg.TypID, 0, &te));
		THROW(UnrollType(arg.TypID, te.T, &td));
		if(td.IsInterfaceTypeConversionNeeded() > 0) {
			int   do_assign = 0;
			const TYPEID st = GETSTYPE(td.T.Typ);
			STypEx t_stripped = td.T;
			if(oneof2(t_stripped.Mod, STypEx::modPtr, STypEx::modRef))
				t_stripped.Mod = 0;
			Format_C_Type(td.TerminalTypeID, t_stripped, arg_name, fctfInstance | fctfIfaceImpl, temp_buf);
			if(st == S_DATE) {
				temp_buf.CatDiv(';', 2);
				do_assign = 1;
			}
			else if(st == S_TIME) {
				temp_buf.CatDiv(';', 2);
				do_assign = 1;
			}
			else if(st == S_DATETIME) {
				temp_buf.CatDiv(';', 2);
				do_assign = 1;
			}
			else if(st == S_CHAR) {
			}
			else if(st == S_ZSTRING) {
				temp_buf.CatDiv(';', 2).Cat(arg_name).Dot().Cat("CopyFromOleStr");
				temp_buf.CatChar('(');
				//
				if(td.T.Mod == STypEx::modPtr)
					temp_buf.CatChar('*');
				temp_buf.CatCharN('*', td.PtrList.GetCount());
				//
				temp_buf.Cat(arg_name).CatChar('_');
				temp_buf.CatChar(')');
			}
			else if(st == S_LSTRING) {
			}
			else if(st == S_NOTE) {
			}
			if(do_assign) {
				temp_buf.Cat(arg_name).CatDiv('=', 1);
				if(td.T.Mod == STypEx::modPtr)
					temp_buf.CatChar('*');
				temp_buf.CatCharN('*', td.PtrList.GetCount());
				temp_buf.Cat(arg_name).CatChar('_');
			}
			temp_buf.Semicol().CR();
			gen.WriteLine(gen.CatIndent(line_buf = 0).Cat(temp_buf));
		}
	}
	THROW(SearchTypeID(rFunc.TypID, 0, &ret_te));
	if(ret_te.MangleC != 'X') { // other than "void"
		Format_C_Type(rFunc.TypID, ret_te.T, "ret", fctfInstance | fctfIfaceImpl, temp_buf);
		temp_buf.CatDiv('=', 1);
		is_ret = 1;
	}
	(rFunc.Name = 0).Cat("IMCI").CatParStr(mod_func_name); // ICMD(func)
	Write_Func(gen, rFunc, ffCPP_CallImp, is_ret ? (const char *)temp_buf : 0);
	//
	// Конвертация (если необходимо) типов исходящих параметров
	//
	for(k = 0; k < arg_count; k++) {
		THROW(rFunc.GetArg(k, &arg));
		//
		// Обратную конвертацию делаем только для OUT-аргументов
		//
		if(arg.Flags & DlFunc::fArgOut) {
			rFunc.GetArgName(k, arg_name);
			THROW(SearchTypeID(arg.TypID, 0, &te));
			THROW(UnrollType(arg.TypID, te.T, &td));
			if(td.IsInterfaceTypeConversionNeeded() > 0) {
				const TYPEID st = GETSTYPE(td.T.Typ);
				STypEx t_stripped = td.T;
				if(oneof2(t_stripped.Mod, STypEx::modPtr, STypEx::modRef))
					t_stripped.Mod = 0;
				gen.CatIndent(line_buf = 0);
				if(st == S_DATE) {
					temp_buf.Printf("ASSIGN_PTR(%s_, %s);", arg_name.cptr(), arg_name.cptr());
					gen.WriteLine(line_buf.Cat(temp_buf).CR());
				}
				else if(st == S_TIME) {
				}
				else if(st == S_DATETIME) {
					temp_buf.Printf("ASSIGN_PTR(%s_, %s);", arg_name.cptr(), arg_name.cptr());
					gen.WriteLine(line_buf.Cat(temp_buf).CR());
				}
				else if(st == S_CHAR) {
				}
				else if(st == S_ZSTRING) {
					temp_buf.Printf("%s.CopyToOleStr(", arg_name.cptr());
					temp_buf.CatCharN('*', td.PtrList.GetCount());
					temp_buf.Cat(arg_name).CatChar('_').CatChar(')').Semicol();
					gen.WriteLine(line_buf.Cat(temp_buf).CR());
				}
				else if(st == S_LSTRING) {
				}
				else if(st == S_NOTE) {
				}
			}
		}
	}
	//
	// Конвертация возвращаемого значения //
	//
	if(is_ret) {
		gen.CatIndent(line_buf = 0);
		THROW(UnrollType(rFunc.TypID, ret_te.T, &td));
		if(td.IsInterfaceTypeConversionNeeded() > 0) {
			const TYPEID st = GETSTYPE(td.T.Typ);
			STypEx t_stripped = td.T;
			if(oneof2(t_stripped.Mod, STypEx::modPtr, STypEx::modRef))
				t_stripped.Mod = 0;
			if(st == S_DATE) {
				gen.WriteLine(line_buf.Cat("ASSIGN_PTR(pRet_, (OleDate)ret);").CR());
			}
			else if(st == S_TIME) {
			}
			else if(st == S_DATETIME) {
				gen.WriteLine(line_buf.Cat("ASSIGN_PTR(pRet_, (OleDate)ret);").CR());
			}
			else if(st == S_CHAR) {
			}
			else if(st == S_ZSTRING) {
				gen.WriteLine(line_buf.Cat("ret.CopyToOleStr(pRet_);").CR());
			}
			else if(st == S_LSTRING) {
			}
			else if(st == S_NOTE) {
			}
		}
		else {
			gen.WriteLine(line_buf.Cat("ASSIGN_PTR(pRet, ret);").CR());
		}
	}
	gen.WriteLine(gen.CatIndent(line_buf = 0).Cat("IFACE_METHOD_EPILOG;").CR());
	gen.IndentDec();
	gen.Wr_CloseBrace(0, 0);
	gen.WriteLine(0);
	CATCHZOK
	return ok;
}

static SString & SLAPI Make_USEIMPL_DefSymb(const char * pClsName, SString & rBuf)
{
	return (rBuf = "USE_IMPL_").Cat(pClsName);
}

int SLAPI DlContext::Write_C_AutoImplFile(Generator_CPP & gen, const DlScope & rScope, StringSet & rSs)
{
	int    ok = 1, r;
	uint   i, j, k;
	SString cls_name, fld_buf, func_name, vtab_name;
	SdbField fld;
	DlFunc func;
	DlScope * p_ds = 0;
	for(i = 0; rScope.EnumChilds(&i, &p_ds);) {
		if(p_ds->IsKind(DlScope::kIClass)) {
			/*
				#define MFP(f) (__stdcall PpSession::*f)
				struct DL6ICLS_PpSession_VTab {
					//
					// Interface IPpSession
					//
					IUNKN_METHOD_PTRS(00);
					HRESULT MFP(f000_00)(double, void *);
					DL6ICLS_PpSession_VTab()
					{
						IUNKN_METHOD_PTRS_ASSIGN(00);
						f000_00=PpSession::xxx_;
					}
				}
				#undef MFP
			*/
			DlScope::IfaceBase ifb;
			const DlScope * p_ifs;
			//
			// Тест целостности списка интерфейсов
			//
			for(j = 0; (r = EnumInterfacesByICls(p_ds, &j, &ifb, &p_ifs)) >= 0;)
				if(r == 0) {
					fld_buf.Printf("Error extracting interface %s::%u", cls_name.cptr(), j-1);
					gen.WriteLine(fld_buf.CR());
				}
			MakeClassName(p_ds, clsnfCPP, cls_name);

			gen.Wr_Comment(0);
			gen.Wr_Comment(fld_buf.Printf("Implementation of interface %s", cls_name.cptr()));
			gen.Wr_Comment(0);
			gen.Wr_IfDef(Make_USEIMPL_DefSymb(cls_name, fld_buf));
			rSs.add(fld_buf, 0);
			//
			// Таблица виртуальных функция класса (собственно, та таблица, которая будет возвращаться //
			// клиенту при обращении к QueryInterface)
			//
			// ...
			(vtab_name = cls_name).CatChar('_').Cat("VTab");
			gen.Wr_StartClassDecl(Generator_CPP::clsStruct, vtab_name, 0, 0);
			gen.IndentInc();
			gen.Wr_Define("MFP(f)", fld_buf.Printf("(__stdcall %s::*f)", cls_name.cptr()));
			for(j = 0; EnumInterfacesByICls(p_ds, &j, &ifb, &p_ifs) > 0;) {
				//
				// Interface method's declarations
				//
				gen.Wr_Comment(0);
				gen.Wr_Comment((fld_buf = "Interface").Space().Cat(p_ifs->Name));
				gen.Wr_Comment(0);
				gen.CatIndent(func_name = 0).Cat("IUNKN_METHOD_PTRS").
					CatChar('(').CatLongZ(j-1, 2).CatChar(')').Semicol().CR();
				gen.WriteLine(func_name);
				for(k = 0; p_ifs->EnumFunctions(&k, &func) > 0;) {
					(func.Name = "MFP").
						CatChar('(').CatChar('f').CatLongZ(k-1, 3).CatLongZ(j-1, 2).CatChar(')');
					Write_Func(gen, func, ffCPP_VTbl);
				}
			}
			//
			// ISupportErrorInfo {
			//
			gen.Wr_Comment(0);
			gen.Wr_Comment((fld_buf = "Interface").Space().Cat("ISupportErrorInfo"));
			gen.Wr_Comment(0);
			gen.CatIndent(func_name = 0).Cat("IUNKN_METHOD_PTRS").CatParStr("SEI").Semicol().CR();
			gen.WriteLine(func_name);
			gen.CatIndent(func_name = 0).Cat("ISUPERRINFO_METHOD_PTRS").CatParStr("SEI").Semicol().CR();
			gen.WriteLine(func_name);
			//
			// }
			//
			gen.WriteLine((fld_buf = "#undef MFP").CR());
			//
			// Constructor of vtable struct
			//
			gen.Wr_StartDeclFunc(Generator_CPP::fkConstr, 0, 0, vtab_name, 0);
			gen.Wr_EndDeclFunc(0, 1);
			gen.Wr_OpenBrace();
			gen.IndentInc();
			for(j = 0; EnumInterfacesByICls(p_ds, &j, &ifb, &p_ifs) > 0;) {
				gen.CatIndent(func_name = 0).Cat("IUNKN_METHOD_PTRS_ASSIGN").
					CatChar('(').CatLongZ(j-1, 2).CatChar(')').Semicol().CR();
				gen.WriteLine(func_name);
				for(k = 0; p_ifs->EnumFunctions(&k, &func) > 0;) {
					gen.CatIndent(fld_buf = 0);
					fld_buf.CatChar('f').CatLongZ(k-1, 3).CatLongZ(j-1, 2).CatDiv('=', 1).CatChar('&');
					if(func.Flags & DlFunc::fPropGet) {
						(func_name = "get").CatChar('_').Cat(func.Name);
						func.Name = func_name;
					}
					else if(func.Flags & DlFunc::fPropPut) {
						(func_name = "put").CatChar('_').Cat(func.Name);
						func.Name = func_name;
					}
					gen.MakeClsfName(cls_name, func.Name, func_name).CatChar('_');
					fld_buf.Cat(func_name).Semicol().CR();
					gen.WriteLine(fld_buf);
				}
			}
			//
			// ISupportErrorInfo {
			//
			gen.CatIndent(func_name = 0).Cat("IUNKN_METHOD_PTRS_ASSIGN").CatParStr("SEI").Semicol().CR();
			gen.WriteLine(func_name);
			gen.CatIndent(func_name = 0).Cat("ISUPERRINFO_METHOD_PTRS_ASSIGN").CatParStr("SEI").Semicol().CR();
			gen.WriteLine(func_name);
			//
			// }
			//
			gen.IndentDec();
			gen.Wr_CloseBrace(0, 0);
			gen.IndentDec();
			gen.Wr_CloseBrace(1, 0);
			//
			// Оболочка реализации интерфейсов
			//
			for(j = 0; EnumInterfacesByICls(p_ds, &j, &ifb, &p_ifs) > 0;) {
				gen.Wr_Comment(0);
				gen.Wr_Comment((fld_buf = "Interface").Space().Cat(p_ifs->Name));
				gen.Wr_Comment(0);
				for(k = 0; p_ifs->EnumFunctions(&k, &func) > 0;)
					Write_C_ImplInterfaceFunc(gen, cls_name, func);
			}
			gen.Wr_EndIf(Make_USEIMPL_DefSymb(cls_name, fld_buf));
		}
		else
			THROW(Write_C_AutoImplFile(gen, *p_ds, rSs)); // @recursion
	}
	CATCHZOK
	return ok;
}

int SLAPI DlContext::Write_WSDL_File(const char * pFileName, const DlScope & rScope)
{
	int    ok = -1;
	/*
	xmlTextWriterPtr writer = 0;

	writer = xmlNewTextWriterFilename(pFileName, 0);
	xmlTextWriterStartDocument(writer, 0, "UTF-8", "yes");

	DlScope * p_ds = 0;
	for(uint i = 0; rScope.EnumChilds(&i, &p_ds);) {
		if(p_ds->IsKind(DlScope::kExpData) && !p_ds->GetBase() && p_ds->CheckDvFlag(DlScope::declfWSDL)) {
			const int is_iter = BIN(p_ds->GetFirstChildByKind(DlScope::kExpDataIter, 0));
			if(is_iter) {

			}
		}
	}

	xmlTextWriterEndDocument(writer);
	*/
	return ok;
}

int SLAPI DlContext::Write_C_ImplFile(Generator_CPP & gen, const DlScope & rScope)
{
	int    ok = 1;
	uint   j, k;
	SString cls_name, inst_name, fld_buf, func_name, vtab_name, mod_func_name;
	SString base_name = "DlRtm";
	SdbField fld;
	DlFunc func;
	DlScope * p_ds = 0;
	for(uint i = 0; rScope.EnumChilds(&i, &p_ds);) {
		if(p_ds->IsKind(DlScope::kIClass)) {
			DlScope::IfaceBase ifb;
			const DlScope * p_ifs;
			//
			// Методы реализации интерфейсов
			//
			/*
				DL6_IC_CONSTRUCTOR(PpSession, DL6ICLS_PpSession_VTab)
				{
				}
				DL6_IC_DESTRUCTOR(PpSession)
				{
				}
			*/
			MakeClassName(p_ds, clsnfCPP, cls_name);
			(vtab_name = cls_name).CatChar('_').Cat("VTab");
			gen.WriteLine(0);
			gen.CatIndent(fld_buf = 0).Cat("DL6_IC_CONSTRUCTOR").CatChar('(').
				Cat(p_ds->Name).Comma().Space().Cat(vtab_name).CatChar(')').CR();
			gen.WriteLine(fld_buf);
			gen.Wr_OpenBrace();
			Wr_YourCodeHere(gen);
			gen.Wr_CloseBrace(0, 0);

			gen.WriteLine(0);
			gen.CatIndent(fld_buf = 0).Cat("DL6_IC_DESTRUCTOR").CatParStr(p_ds->Name).CR();
			gen.WriteLine(fld_buf);
			gen.Wr_OpenBrace();
			Wr_YourCodeHere(gen);
			gen.Wr_CloseBrace(0, 0);
			//
			// Method's implementation declarations
			//
			for(j = 0; EnumInterfacesByICls(p_ds, &j, &ifb, &p_ifs) > 0;) {
				gen.Wr_Comment(0);
				gen.Wr_Comment((fld_buf = "Interface").Space().Cat(p_ifs->Name).Space().Cat("implementation"));
				gen.Wr_Comment(0);
				for(k = 0; p_ifs->EnumFunctions(&k, &func) > 0;) {
					{
						//
						//  Property name modification
						//
						fld_buf = 0;
						if(func.Flags & DlFunc::fPropGet)
							fld_buf = "get";
						else if(func.Flags & DlFunc::fPropPut)
							fld_buf = "put";
						if(fld_buf.NotEmpty())
							mod_func_name = fld_buf.CatChar('_').Cat(func.Name);
						else
							mod_func_name = func.Name;
					}
					func.Name = gen.MakeClsfName(cls_name, mod_func_name, func_name);
					Write_Func(gen, func, ffCPP_Imp);
					gen.Wr_OpenBrace();
					Wr_YourCodeHere(gen);
					gen.Wr_CloseBrace(0, 0);
					gen.WriteLine(0);
				}
			}
			ok = 2;
		}
		else if(p_ds->IsKind(DlScope::kExpData) && !p_ds->GetBase()) {
			MakeClassName(p_ds, clsnfCPP, cls_name);
			if(p_ds->IsPrototype()) {
				gen.Wr_ClassPrototype(Generator_CPP::clsClass, cls_name);
			}
			else {
				const int is_iter = BIN(p_ds->GetFirstChildByKind(DlScope::kExpDataIter, 0));
				gen.Wr_Comment(0);
				gen.Wr_Comment(fld_buf.Printf("Implementation of %s", cls_name.cptr()));
				gen.Wr_Comment(0);
				//
				// constructor
				//
				fld_buf.Printf("PPALDD_CONSTRUCTOR(%s)", p_ds->GetName().cptr()).CR();
				gen.WriteLine(fld_buf);
				gen.Wr_OpenBrace();
				// code... {
				gen.IndentInc();
				DlScope * p_rec = 0;
				for(j = 0; p_ds->EnumChilds(&j, &p_rec);)
					if(p_rec->IsKind(DlScope::kExpDataHdr)) {
						gen.Wr_Indent();
						if(p_rec->Name.Cmp("hdr", 0) == 0) {
							fld_buf = "InitFixData(rscDefHdr, &H, sizeof(H));";
							gen.WriteLine(fld_buf.CR());
						}
						else {
							MakeDlRecName(p_rec, 1, inst_name);
							fld_buf.Printf("InitFixData(\"%s\", &%s, sizeof(%s));", p_rec->Name.cptr(), inst_name.cptr(), inst_name.cptr());
							gen.WriteLine(fld_buf.CR());
						}
					}
					else if(p_rec->IsKind(DlScope::kExpDataIter)) {
						gen.Wr_Indent();
						if(p_rec->Name.Cmp("iter@def", 0) == 0) {
							fld_buf = "InitFixData(rscDefIter, &I, sizeof(I));";
							gen.WriteLine(fld_buf.CR());
						}
						else {
							MakeDlRecName(p_rec, 1, inst_name);
							fld_buf.Printf("InitFixData(\"%s\", &%s, sizeof(%s));", p_rec->Name.cptr(), inst_name.cptr(), inst_name.cptr());
							gen.WriteLine(fld_buf.CR());
						}
					}
				gen.IndentDec();
				// } ...code
				gen.Wr_CloseBrace(0, 0);
				gen.WriteLine(0);
				//
				// destructor
				//
				fld_buf.Printf("PPALDD_DESTRUCTOR(%s)", p_ds->GetName().cptr()).CR();
				gen.WriteLine(fld_buf);
				gen.Wr_OpenBrace();
				// code... {
				gen.IndentInc();
				{
					gen.Wr_Indent();
					fld_buf = "Destroy(";
					gen.WriteLine(fld_buf);
					gen.Wr_EndDeclFunc(1, 1);
				}
				gen.IndentDec();
				// } ...code
				gen.Wr_CloseBrace(0, 0);
				gen.WriteLine(0);
				/*
				int PPALDD_GoodsBillBase::InitData(PPFilt & rFilt, long rsrv)
				{
					return DlRtm::InitData(rFilt, rsrv);
				}
				*/
				gen.MakeClsfName(cls_name, "InitData", fld_buf);
				gen.Wr_StartDeclFunc(Generator_CPP::fkOrdinary, 0, "int", fld_buf);
				gen.Wr_VarDecl("PPFilt &", "rFilt", 0, ',');
				gen.Wr_VarDecl("long", "rsrv", 0, 0);
				gen.Wr_EndDeclFunc(0, 1);
				gen.Wr_OpenBrace();
				// code... {
				gen.IndentInc();
				{
					gen.Wr_Indent();
					fld_buf = "return DlRtm::InitData(rFilt, rsrv";
					gen.WriteLine(fld_buf);
					gen.Wr_EndDeclFunc(1, 1);
				}
				gen.IndentDec();
				// } ...code
				gen.Wr_CloseBrace(0, 0);
				if(is_iter) {
					gen.WriteLine(0);
					/*
					int PPALDD_GoodsBillBase::InitIteration(PPIterID iterId, int sortId, long rsrv)
					{
						IterProlog(iterId, 1);
						return -1;
					}
					*/
					gen.MakeClsfName(cls_name, "InitIteration", fld_buf);
					gen.Wr_StartDeclFunc(Generator_CPP::fkOrdinary, 0, "int", fld_buf);
					gen.Wr_VarDecl("long", "iterId", 0, ',');
					gen.Wr_VarDecl("int",  "sortId", 0, ',');
					gen.Wr_VarDecl("long", "rsrv",   0,   0);
					gen.Wr_EndDeclFunc(0, 1);
					gen.Wr_OpenBrace();
					// code... {
					gen.IndentInc();
					{
						gen.WriteLine(gen.CatIndent(fld_buf = 0).Cat("IterProlog(iterId, 1);").CR());
						gen.Wr_Return("-1");
					}
					gen.IndentDec();
					// } ...code
					gen.Wr_CloseBrace(0, 0);
					gen.WriteLine(0);
					/*
					int PPALDD_GoodsBillBase::NextIteration(PPIterID iterId, long rsrv)
					{
						IterProlog(iterId, 0);
						return DlRtm::NextIteration(iterId, rsrv);
					}
					*/
					gen.MakeClsfName(cls_name, "NextIteration", fld_buf);
					gen.Wr_StartDeclFunc(Generator_CPP::fkOrdinary, 0, "int", fld_buf);
					gen.Wr_VarDecl("long", "iterId", 0, ',');
					gen.Wr_VarDecl("long", "rsrv",   0,   0);
					gen.Wr_EndDeclFunc(0, 1);
					gen.Wr_OpenBrace();
					// code... {
					gen.IndentInc();
					{
						gen.WriteLine(gen.CatIndent(fld_buf = 0).Cat("IterProlog(iterId, 0);").CR());
						gen.Wr_Indent();
						fld_buf = "return DlRtm::NextIteration(iterId, rsrv";
						gen.WriteLine(fld_buf);
						gen.Wr_EndDeclFunc(1, 1);
					}
					gen.IndentDec();
					// } ...code
					gen.Wr_CloseBrace(0, 0);
				}
				if(p_ds->CheckDvFlag(DlScope::declfSet)) {
					gen.WriteLine(0);
					gen.MakeClsfName(cls_name, "Set", fld_buf);
					gen.Wr_StartDeclFunc(Generator_CPP::fkOrdinary, 0, "int", fld_buf);
					gen.Wr_VarDecl("long", "iterId", 0, ',');
					gen.Wr_VarDecl("int", "commit",  0,   0);
					gen.Wr_EndDeclFunc(0, 1);
					gen.Wr_OpenBrace();
					// code... {
					gen.IndentInc();
					{
						gen.Wr_Return("-1");
					}
					gen.IndentDec();
					// } ...code
					gen.Wr_CloseBrace(0, 0);
				}
				if(p_ds->CheckDvFlag(DlScope::declfDestroy)) {
					gen.WriteLine(0);
					gen.MakeClsfName(cls_name, "Destroy", fld_buf);
					gen.Wr_StartDeclFunc(Generator_CPP::fkOrdinary, 0, "int", fld_buf); // @v9.6.4 "int"-->"void"
					gen.Wr_EndDeclFunc(0, 1);
					gen.Wr_OpenBrace();
					// code... {
					gen.IndentInc();
					{
						// @v9.6.4 gen.Wr_Return("-1");
					}
					gen.IndentDec();
					// } ...code
					gen.Wr_CloseBrace(0, 0);
				}
			}
		}
		else if(p_ds->IsKind(DlScope::kDbTable)) {
			//
			// Получаем самое первое поле для ссылки в конструкторе DBTable
			//
			fld_buf = 0;
			for(k = 0; p_ds->EnumFields(&k, &fld);) {
				fld_buf = fld.Name;
				break;
			}
			//
			// Вызов макроса DBTABLE_CONSTRUCTOR
			//
			(func_name = "DBTABLE_CONSTRUCTOR").CatChar('(').Cat(p_ds->GetName()).
				CatDiv(',', 2).Cat(fld_buf).CatChar(')').CR();
			gen.WriteLine(func_name);
		}
		else
			THROW(ok = Write_C_ImplFile(gen, *p_ds)); // @recursion
	}
	CATCHZOK
	return ok;
}
//
//
//
int SLAPI DlContext::Write_Code()
{
	// CtmConstList AddConst
	int    ok = 1;
	if(fileExists(BinFileName))
		::remove(BinFileName);
	SFile  outf(BinFileName, SFile::mReadWrite|SFile::mBinary);
	SymbHashTable::Iter hti;
	DlCtxHdr hdr;
	SBuffer buf;
	uint   symb_id;
	SString symb;
	THROW(outf.Write(&hdr, sizeof(hdr)));
	//
	for(Tab.InitIteration(&hti); Tab.NextIteration(&hti, &symb_id, 0, &symb) > 0;) {
		THROW(buf.Write(&symb_id, sizeof(symb_id)));
		THROW(buf.Write(symb));
	}
	symb_id = 0xffffffffU;
	symb = "<<EOT>>";
	THROW(buf.Write(&symb_id, sizeof(symb_id)));
	THROW(buf.Write(symb));
	//
	THROW(ConstList.Write(&buf));
	THROW(buf.Write(&TypeList, 0));
	THROW(buf.Write(&UuidList, 0));
	THROW(Sc.Write(buf));
	THROW(outf.Write(buf));
	THROW(outf.CalcCRC(sizeof(hdr), &hdr.Crc32));
	outf.Seek(0);
	THROW(outf.Write(&hdr, sizeof(hdr)));
	CATCHZOK
	return ok;
}
//
//
//
int SLAPI DlContext::FindImportFile(const char * pFileName, SString & rPath)
{
	SPathStruc ps, ps_s;
	ps.Split(pFileName);
	ps_s.Split(InFileName);
	if(ps.Drv.NotEmpty())
		rPath = pFileName;
	else {
		ps_s.Split(InFileName);
		if(ps.Dir.NotEmpty()) {
			if(ps_s.Dir.NotEmpty())
				ps_s.Dir.SetLastSlash().Cat(ps.Dir.ShiftLeftChr('/').ShiftLeftChr('\\'));
			else
				ps_s.Dir = ps.Dir;
		}
		ps_s.Merge(&ps, SPathStruc::fNam|SPathStruc::fExt, rPath);
	}
	if(fileExists(rPath)) {
		return 1;
	}
	else {
		SetError(PPERR_DL6_IMPFILENFOUND, rPath);
		return 0;
	}
}

int SLAPI DlContext::Test()
{
	SString symb;
	LongArray scope_id_list;
	Sc.GetChildList(DlScope::kIClass, 1, &scope_id_list);
	Sc.GetChildList(DlScope::kInterface, 1, &scope_id_list);
	for(uint i = 0; i < scope_id_list.getCount(); i++) {
#ifndef NDEBUG
		S_GUID uuid;
#endif
		DLSYMBID symb_id = 0;
		const DlScope * p_scope = GetScope(scope_id_list.at(i), 0);
		assert(p_scope != 0);
		assert(SearchSymb(p_scope->Name, '@', &symb_id));
		assert(symb_id == p_scope->ID);
		assert(GetSymb(symb_id, symb, '@'));
		assert(GetUuidByScopeID(symb_id, &uuid));
		assert(SearchUuid(uuid) == symb_id);
	}
	return 1;
}

int SLAPI DlContext::CreateDbDictionary(const char * pDictPath, const char * pDataPath, SqlServerType sqlst)
{
	int    ok = 1;
	char   acs[512];
	SString msg_buf;
	LongArray scope_id_list;
	Sc.GetChildList(DlScope::kDbTable, 1, &scope_id_list);

	SString sql_file_name;
	Generator_SQL * p_sqlgen = 0;
	if(sqlst) {
		sql_file_name = InFileName;
		SPathStruc::ReplaceExt(sql_file_name, "sql", 1);
		p_sqlgen = new Generator_SQL(sqlst, Generator_SQL::fIndent);
	}
	{
		DbLoginBlock dlb;
		DbProvider * p_db = BDictionary::CreateBtrDictInstance(pDictPath);
		dlb.SetAttr(DbLoginBlock::attrDictPath, pDictPath);
		dlb.SetAttr(DbLoginBlock::attrDbPath, pDataPath);
		if(!p_db->Login(&dlb, 0)) {
			(msg_buf = pDictPath).Cat("::").Cat(pDataPath);
			SetError(PPERR_DL6_DBDICTOPENFAULT, msg_buf);
			CALLEXCEPT();
		}
		if(!DBS.OpenDictionary2(p_db)) {
			(msg_buf = pDictPath).Cat("::").Cat(pDataPath).Cat("(open)");
			SetError(PPERR_DL6_DBDICTOPENFAULT, msg_buf);
			CALLEXCEPT();
		}
	}
	for(uint i = 0; i < scope_id_list.getCount(); i++) {
		DBTable tbl;
		THROW(LoadDbTableSpec(scope_id_list.at(i), &tbl, 0));
		if(CurDict) {
			if(!CurDict->CreateTableSpec(&tbl)) {
				(msg_buf = tbl.GetTableName()).CatDiv(':', 1).CatEq("BtrError", (long)BtrError);
				SetError(PPERR_DL6_DDFENTRYCRFAULT, msg_buf);
				CALLEXCEPT();
			}
			if(!(tbl.GetFlags() & XTF_TEMP) && !(tbl.GetFlags() & XTF_DICT)) {
				if(!CurDict->CreateDataFile(&tbl, 0, crmTTSReplace, GetRusNCaseACS(acs))) {
					(msg_buf = tbl.GetName()).CatDiv(':', 1).CatEq("BtrError", (long)BtrError);
					SetError(PPERR_DL6_BTRFILECRFAULT, msg_buf);
					CALLEXCEPT();
				}
			}
		}
		if(p_sqlgen) {
			p_sqlgen->CreateTable(tbl, tbl.GetTableName());
			p_sqlgen->Eos().Cr();
			uint j;
			for(j = 0; j < tbl.GetIndices().getNumKeys(); j++) {
				p_sqlgen->CreateIndex(tbl, tbl.GetTableName(), j);
				p_sqlgen->Eos();
			}
			p_sqlgen->Cr();
			if(sqlst == sqlstORA) {
				for(j = 0; j < tbl.GetFields().getCount(); j++) {
					TYPEID _t = tbl.GetFields()[j].T;
					if(GETSTYPE(_t) == S_AUTOINC) {
						p_sqlgen->CreateSequenceOnField(tbl, tbl.GetTableName(), j, 0);
						p_sqlgen->Eos();
					}
				}
				p_sqlgen->Cr();
			}
			{
				SFile f_sql(sql_file_name, SFile::mWrite);
				if(f_sql.IsValid()) {
					f_sql.WriteLine((SString &)*p_sqlgen);
				}
			}
		}
	}
	DBS.CloseDictionary();
	CATCHZOK
	delete p_sqlgen;
	return ok;
}

#define DL600C_RELEASE_DEBUG

int SLAPI DlContext::Compile(const char * pInFileName, const char * pDictPath, const char * pDataPath, long cflags)
{
	extern FILE * yyin;
	extern int    yyparse();
	int    ok = 1;
	SString file_name;
	SString line_buf;
	SString h_once_macro;
	SString temp_buf;
	{
		SPathStruc::ReplaceExt(file_name = pInFileName, "ald", 0);
		Init(file_name);
	}
#ifdef DL600C_RELEASE_DEBUG
	SFile  f_debug;
	{
		SPathStruc::ReplaceExt(file_name = pInFileName, "debug", 1);
		f_debug.Open(file_name, SFile::mWrite);
	}
#endif
	yyin = fopen(InFileName, "r");
#ifdef DL600C_RELEASE_DEBUG
	f_debug.WriteLine((temp_buf = "fopen(InFileName)").CR()); // @debug
#endif
	if(yyin) {
		uint   i;
		LongArray scope_id_list;
		DLSYMBID scope_id = EnterScope(DlScope::kFile, InFileName, 0, 0);
#ifdef DL600C_RELEASE_DEBUG
		f_debug.WriteLine((temp_buf = "yyparse before").CR()); // @debug
#endif
		yyparse();
#ifdef DL600C_RELEASE_DEBUG
		f_debug.WriteLine((temp_buf = "yyparse after").CR()); // @debug
#endif
		if(scope_id != CurScopeID)
			Error(PPERR_DL6_UNMATCHSCOPE, 0, erfLog);
		LeaveScope();
		if(!StoreUuidList())
			Error(LastError);
		if(!(cflags & cfBinOnly)) {
			Generator_CPP gen(0);
			//
			// Writing H-file
			//
			{
				SPathStruc ps;
				ps.Split(CDeclFileName);
				(h_once_macro = 0).CatCharN('_', 2).Cat(ps.Nam).CatChar('_').Cat(ps.Ext).ToUpper();
			}
			gen.Open(CDeclFileName);
			Write_C_FileHeader(gen, CDeclFileName);

			gen.Wr_IfDef(h_once_macro, 1);
			gen.Wr_Define(h_once_macro, 0);
			gen.WriteLine(0);
			if(Sc.GetFirstChildByKind(DlScope::kUiDialog, 1)) {
				SString symb;
				DLSYMBID ss_id = 0;
				//
				scope_id_list.freeAll();
				Sc.GetChildList(DlScope::kUiDialog, 1, &scope_id_list);
				gen.Wr_Comment(symb = 0);
				gen.Wr_Comment("Dialogs");
				gen.Wr_Comment(symb = 0);
				for(i = 0; i < scope_id_list.getCount(); i++) {
					const DlScope * p_scope = GetScope(scope_id_list.at(i), 0);
					if(p_scope) {
						(temp_buf = 0).Cat(p_scope->GetId());
						//
						CtmExprConst c_ss;
						if(p_scope->GetConst(DlScope::cuifSymbSeries, &c_ss)) {
							char   s_buf[256];
							s_buf[0] = 0;
							GetConstData(c_ss, s_buf, sizeof(s_buf));
							temp_buf.Space().Cat("//").Space().Cat(s_buf);
						}
						gen.Wr_Define(p_scope->Name, temp_buf);
					}
				}
				//
				{
					uint   max_len = 0;
					UiSymbAssoc.Sort();
					const  uint sc = UiSymbAssoc.getCount();
					for(i = 0; i < sc; i++) {
						if(GetSymb(UiSymbAssoc.at(i).Val, symb, '$')) {
							SETMAX(max_len, symb.Len());
						}
					}
					max_len = ALIGNSIZE(max_len, 2);
					for(i = 0; i < sc; i++) {
						const LAssoc & r_assc = UiSymbAssoc.at(i);
						if(ss_id != r_assc.Key) {
							gen.Wr_Comment(symb = 0);
							GetSymb(r_assc.Key, symb, '$');
							gen.Wr_Comment(symb);
							gen.Wr_Comment(symb = 0);
						}
						if(GetSymb(r_assc.Val, symb, '$')) {
							(temp_buf = 0).Cat(r_assc.Val).Align(max_len - symb.Len(), ADJ_RIGHT);
							gen.Wr_Define(symb, temp_buf);
						}
						ss_id = r_assc.Key;
					}
				}
			}
			if(Sc.GetFirstChildByKind(DlScope::kDbTable, 1)) {
				gen.Wr_Include("db.h", 0);
				gen.Wr_Include("dl600.h", 0);
				gen.WriteLine(0);
			}
			if(Sc.GetFirstChildByKind(DlScope::kExpData, 1)) {
				gen.Wr_Define("Head_data", "H");
				gen.Wr_Define("IT_default_data", "I");
				gen.WriteLine(0);
			}
			//
			// Прототипы интерфейсов {
			//
			if(Sc.GetFirstChildByKind(DlScope::kInterface, 1) || Sc.GetFirstChildByKind(DlScope::kIClass, 1)) {
				gen.Wr_ClassPrototype(Generator_CPP::clsStruct, "IUnknown");
				scope_id_list.freeAll();
				Sc.GetChildList(DlScope::kInterface, 1, &scope_id_list);
				for(i = 0; i < scope_id_list.getCount(); i++) {
					const DlScope * p_scope = GetScope(scope_id_list.at(i), 0);
					if(p_scope)
						gen.Wr_ClassPrototype(Generator_CPP::clsStruct, p_scope->Name);
				}
				gen.WriteLine(0);
			}
			//
			// }
			//
			if(cflags & cfDebug)
				gen.WriteLine((line_buf = 0).Cat("#pragma pack(show)").CR().CR());
			if(!Write_C_DeclFile(gen, Sc))
				Error(LastError, 0, 0);
			if(cflags & cfDebug)
				gen.WriteLine((line_buf = 0).CR().Cat("#pragma pack(show)").CR());
			gen.WriteLine(0);
			gen.Wr_EndIf(h_once_macro);
			//
			// Writing CPP-files
			//
			{
				StringSet ss;
				SString impl_filename;
				SPathStruc ps;
				ps.Split(InFileName);
				//
				// _auto cpp-файл пишем только в том случае, если есть хоть одно определение iclass
				//
				scope_id_list.freeAll();
				Sc.GetChildList(DlScope::kIClass, 1, &scope_id_list);
				if(scope_id_list.getCount()) {
					ps.Ext = "cpp";
					ps.Nam.CatChar('_').Cat("auto");
					ps.Merge(impl_filename);
					//
					gen.Open(impl_filename);
					Write_C_FileHeader(gen, impl_filename);
					if(!Write_C_AutoImplFile(gen, Sc, ss))
						Error(LastError, 0, 0);
				}
				{
					gen.Open(CImplFileName);
					Write_C_FileHeader(gen, CImplFileName);
					//
					for(uint ssp = 0; ss.get(&ssp, temp_buf);)
						gen.Wr_Define(temp_buf, 0);
					gen.WriteLine(0);
					ps.Ext = "h";
					ps.Merge(0, SPathStruc::fDrv|SPathStruc::fDir, impl_filename);
					gen.Wr_Include(impl_filename, 1);
					gen.WriteLine(0);
					//
					if(!Write_C_ImplFile(gen, Sc))
						Error(LastError, 0, 0);
				}
			}
			//
			// Writing IDL file
			//
			SPathStruc::ReplaceExt(file_name = InFileName, "idl", 1);
			gen.Open(file_name);
			Write_C_FileHeader(gen, file_name);
			temp_buf = "unknwn.idl";
			(line_buf = 0).Cat("import").Space().Cat(temp_buf.Quot('\"', '\"')).Semicol().CR();
			gen.WriteLine(line_buf);
			temp_buf = "oaidl.idl";
			(line_buf = 0).Cat("import").Space().Cat(temp_buf.Quot('\"', '\"')).Semicol().CR();
			gen.WriteLine(line_buf);
			gen.WriteLine(0);
			//
			// Прототипы интерфейсов {
			//
			scope_id_list.freeAll();
			Sc.GetChildList(DlScope::kInterface, 1, &scope_id_list);
			for(i = 0; i < scope_id_list.getCount(); i++) {
				const DlScope * p_scope = GetScope(scope_id_list.at(i), 0);
				if(p_scope)
					gen.Wr_ClassPrototype(Generator_CPP::clsInterface, p_scope->Name);
			}
			gen.WriteLine(0);
			//
			// }
			//
			if(!Write_IDL_File(gen, Sc))
				Error(LastError, 0, 0);
			//
			// Writing WSDL file
			//
			{
				file_name = InFileName;
				SPathStruc::ReplaceExt(file_name, "wsdl", 1);
				if(!Write_WSDL_File(file_name, Sc))
					Error(LastError, 0, 0);
			}
		}
		if(Write_Code()) {
			//
			// Testing
			//
			if(cflags & cfDebug) {
				DlContext test(0);
				test.Init(BinFileName);
				test.Read_Code();
				assert(test.Test_ReWr_Code(*this));
				assert(Test());
				assert(test.Test());

				Write_DialogReverse();
			}
		}
		else
			Error(LastError, 0, 0);
		//
		// Создание словаря и файлов базы данных
		//
		if(!(cflags & cfBinOnly)) {
			SqlServerType sqlst = sqlstNone;
			if(cflags & cfOracle)
				sqlst = sqlstORA;
			else if(cflags & cfSQL)
				sqlst = sqlstGeneric;
			scope_id_list.freeAll();
			Sc.GetChildList(DlScope::kDbTable, 1, &scope_id_list);
			if(scope_id_list.getCount()) {
				if(pDictPath && pDictPath[0]) {
					if(!CreateDbDictionary(pDictPath, pDataPath, sqlst))
						Error(LastError, 0, 0);
				}
				else
					Error(PPERR_DL6_UNDEFDICTPATH, 0, 0);
			}
		}
		if(cflags & cfDebug)
			Write_DebugListing();
	}
	else
		ok = Error(PPERR_DL6_INPUTOPENFAULT, InFileName, 0);
	return ok;
}
