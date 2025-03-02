// DL600C.CPP
// Copyright (c) A.Sobolev 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2014, 2015, 2016, 2017, 2020, 2021, 2022, 2023, 2024, 2025
// @codepage UTF-8
// Compile-time DL600 modules
//
#include <pp.h>
#include <dl600.h>
#include "dl600c.tab.hpp"

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
void CtmToken::Init()
{
	Code = 0;
	MEMSZERO(U);
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
	else if(Code == T_CONST_COLORRGB) { // @v11.0.4
		if(!U.Color.FromStr(pStr))
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

CtmToken & CtmToken::Copy(const CtmToken & rS)
{
	Init();
	Code = rS.Code;
	if(oneof4(Code, T_IDENT, T_AT_IDENT, T_CONST_STR, T_FMT)) {
		U.S = newStr(rS.U.S);
	}
	else {
		U = rS.U;
	}
	return *this;
}

bool CtmToken::IsEmpty() const { return (Code == 0); }
bool CtmToken::IsIdent() const { return (Code == T_IDENT); }
bool CtmToken::IsString() const { return (Code == T_CONST_STR); }

double CtmToken::GetDouble(uint * pCastFlags) const
{
	uint   cast_flags = STCASTF_NORMAL;
	double result = 0.0;
	if(Code == T_CONST_REAL)
		result = U.FD;
	else if(Code == T_CONST_INT)
		result = U.I;
	else if(Code == CtmToken::acLayoutItemSizeEntry) {
		result = U.UIC.Val;
		if(!(U.UIC.Flags & UiCoord::dfAbs)) {
			cast_flags |= STCASTF_POT_LOSS;
		}
	}
	else
		cast_flags = STCASTF_UNDEF;
	ASSIGN_PTR(pCastFlags, cast_flags);
	return result;
}
	
float  CtmToken::GetFloat(uint * pCastFlags) const
{
	uint   cast_flags = STCASTF_NORMAL;
	float  result = 0.0f;
	if(Code == T_CONST_REAL)
		result = static_cast<float>(U.FD);
	else if(Code == T_CONST_INT) {
		cast_flags |= STCASTF_POT_LOSS;
		result = static_cast<float>(U.I);
	}
	else if(Code == CtmToken::acLayoutItemSizeEntry) {
		result = U.UIC.Val;
		if(!(U.UIC.Flags & UiCoord::dfAbs)) {
			cast_flags |= STCASTF_POT_LOSS;
		}
	}
	else
		cast_flags = STCASTF_UNDEF;
	ASSIGN_PTR(pCastFlags, cast_flags);
	return result;
}

int CtmToken::GetInt(uint * pCastFlags) const
{
	uint   cast_flags = STCASTF_NORMAL;
	int    result = 0;
	if(Code == T_CONST_REAL) {
		cast_flags |= STCASTF_POT_LOSS;
		result = static_cast<int>(U.FD);
	}
	else if(Code == T_CONST_INT) {
		result = U.I;
	}
	else if(Code == CtmToken::acLayoutItemSizeEntry) {
		result = static_cast<int>(U.UIC.Val);
		cast_flags |= STCASTF_POT_LOSS;
	}
	else
		cast_flags = STCASTF_UNDEF;
	ASSIGN_PTR(pCastFlags, cast_flags);
	return result;
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

void CtmDclr::AddDim(uint dim)
{
	DimList.Add(dim);
}
//
//
//
void CtmProperty::Init()
{
	Key.Init();
	Value.Destroy();
}

void CtmProperty::Destroy()
{
	Key.Destroy();
	Value.Destroy();
}

CtmProperty & CtmProperty::Copy(const CtmProperty & rS)
{
	Init();
	Key.Copy(rS.Key);
	Value.Copy(rS.Value);
	return *this;
}

void CtmPropertySheet::Init()
{
	P_List = 0;
}

void CtmPropertySheet::Destroy()
{
	if(P_List) {
		for(uint i = 0; i < P_List->getCount(); i++) {
			CtmProperty * p_item = P_List->at(i);
			if(p_item)
				p_item->Destroy();
		}
		ZDELETE(P_List);
	}
}

int CtmPropertySheet::Add(const CtmProperty & rItem)
{
	int    ok = 0;
	SETIFZ(P_List, new TSCollection <CtmProperty>());
	if(P_List) {
		CtmProperty * p_new_item = new CtmProperty;
		if(p_new_item) {
			p_new_item->Copy(rItem);
			P_List->insert(p_new_item);
			ok = 1;
		}
	}
	return ok;
}

int CtmPropertySheet::Add(const CtmPropertySheet & rList)
{
	int    ok = 0;
	const uint src_count = SVectorBase::GetCount(rList.P_List);
	if(src_count) {
		SETIFZ(P_List, new TSCollection <CtmProperty>());
		if(P_List) {
			for(uint i = 0; i < src_count; i++) {
				const CtmProperty * p_src_item = rList.P_List->at(i);
				if(p_src_item) {
					CtmProperty * p_new_item = new CtmProperty;
					if(p_new_item) {
						p_new_item->Copy(*p_src_item);
						P_List->insert(p_new_item);
						ok = 1;
					}				
				}
			}
		}
	}
	else
		ok = -1;
	return ok;
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

void CtmDclr::AddDecimalDim(uint dec, uint precision)
{
	uint   dim = SetDecimalDim(dec, precision);
	DimList.Add(dim);
}

void CtmDclr::AddPtrMod(uint ptrMod, uint modifier)
{
	PtrList.Add((uint32)MakeLong(ptrMod, modifier));
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
DlMacro::DlMacro() : S(";")
{
}

void DlMacro::Add(const char * pSymb, const char * pResult)
{
	SString buf;
	S.add(buf.Cat(pSymb).Comma().Cat(pResult));
}

int DlMacro::Subst(const char * pSymb, SString & rResult) const
{
	SString pat(pSymb);
	pat.Comma();
	for(uint pos = 0; S.get(&pos, rResult);)
		if(rResult.CmpPrefix(pat, 0) == 0) {
			rResult.ShiftLeft(pat.Len());
			return 1;
		}
	rResult.Z();
	return 0;
}
//
//
//
void DlContext::AddMacro(const char * pMacro, const char * pResult)
{
	SETIFZ(P_M, new DlMacro);
	P_M->Add(pMacro, pResult);
}

int DlContext::GetMacro(const char * pMacro, SString & rResult) const
{
	return P_M ? P_M->Subst(pMacro, rResult) : 0;
}
//
//
//
void DlContext::AddStrucDeclare(const char * pDecl)
{
	CurDeclList.add(pDecl);
}

int DlContext::AddType(const char * pName, TYPEID stypId, char mangleC)
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

int DlContext::AddTypedef(const CtmToken & rSymb, DLSYMBID typeID, uint tdFlags)
{
	EXCEPTVAR(LastError);
	int    ok = 1;
	uint   fld_id = 0;
	uint   pos = 0;
	SdbField fld;
	DLSYMBID new_type_id = 0;
	TypeEntry te, new_te;
	DlScope * p_cur_scope = 0;
	DlScope * p_scope = 0;
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
		AddedMsgString.Z();
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
int DlContext::ApplyBrakPropList(DLSYMBID scopeID, const CtmToken * pViewKind, DLSYMBID typeID, const CtmPropertySheet & rS) // @v11.0.4
{
	EXCEPTVAR(LastError);
	int    ok = 1;
	const  uint _c = SVectorBase::GetCount(rS.P_List);
	SString prop_key;
	SString prop_val;
	//
	SUiLayoutParam alb;
	SString class_ident;
	SString font_ident;
	SString var_ident;
	SString data_ident;
	SString command_ident;
	SString title_text;
	SString figure_symb;
	FRect  label_bbox;
	double font_size = 0.0;
	// Следующие флаги устанавливаются в переменной occurence_flags для индикации факта, что
	// свойство уже присутствует в списке.
	enum {
		occfNoWrap       = 0x0001,
		occfBBox         = 0x0002,
		occfGrowFactor   = 0x0004,
		occfShrinkFactor = 0x0008,
		occfReadOnly     = 0x0010,
		occfStaticEdge   = 0x0020,
		occfTabStop      = 0x0040,
		occfDisabled     = 0x0080,
		occfHidden       = 0x0100,
		occfLabelBBox    = 0x0200,
		occfMultiLine    = 0x0400,
		occfWantReturn   = 0x0800,
		occfPassword     = 0x1000,
		occfDefault      = 0x2000,
		occfFigureSymb   = 0x4000,
		occfBBoxOrigin   = 0x8000, // @v12.2.9
	};
	enum {
		occsLeft         = 0x0001,
		occsTop          = 0x0002,
		occsRight        = 0x0004,
		occsBottom       = 0x0008,
	};
	uint   occurence_flags = 0;
	uint   occurence_margin = 0; // occsXXX
	uint   occurence_padding = 0; // occsXXX
	uint   control_flags = 0; // UiItemKind::fXXX
	uint   label_relation = SOW_UNKN; 
	//
	struct ControlFlagEntry {
		uint   Flag;
		uint   OccurenceFlag;
		const  char * P_Prop;
	};
	static const ControlFlagEntry control_flag_list[] = {
		{ UiItemKind::fReadOnly, occfReadOnly, "readonly" },
		{ UiItemKind::fTabStop, occfTabStop, "tabstop" },
		{ UiItemKind::fStaticEdge, occfStaticEdge, "staticedge" },
		{ UiItemKind::fDisabled, occfDisabled, "disabled" },
		{ UiItemKind::fHidden, occfHidden, "hidden" },
		{ UiItemKind::fMultiLine,  occfMultiLine, "multiline" },
		{ UiItemKind::fWantReturn, occfWantReturn, "wantreturn" },
		{ UiItemKind::fPassword, occfPassword, "password" },
		{ UiItemKind::fDefault, occfDefault, "defaultitem" },
	};
	struct AlignmentEntry {
		uint16 * P_Var;
		uint    Occured;
		const char * P_Prop;
	};
	AlignmentEntry alignment_list[] = {
		{ &alb.JustifyContent, 0, "justifycontent" },
		{ &alb.AlignContent, 0, "aligncontent" },
		{ &alb.AlignItems, 0, "alignitems" },
		{ &alb.AlignSelf, 0, "alignself" },
	};
	//
	DlScope * p_scope = GetScope(scopeID);
	THROW(p_scope);
	for(uint i = 0; i < _c; i++) {
		const CtmProperty * p_prop = rS.P_List->at(i);
		if(p_prop) {
			bool    processed = false;
			THROW(p_prop->Key.IsIdent()); // @todo @err
			prop_key = p_prop->Key.U.S;
			{
				for(uint tidx = 0; !processed && tidx < SIZEOFARRAY(control_flag_list); tidx++) {
					const ControlFlagEntry & r_entry = control_flag_list[tidx];
					if(prop_key == r_entry.P_Prop) {
						THROW(!(occurence_flags & r_entry.OccurenceFlag)); // @err dup feature
						if(p_prop->Value.IsEmpty())
							control_flags |= r_entry.Flag;
						else {
							if(p_prop->Value.IsIdent() || p_prop->Value.IsString())
								prop_val = p_prop->Value.U.S;
							if(prop_val.IsEqiAscii("true") || prop_val.IsEqiAscii("yes") || prop_val.IsEqiAscii(".T."))
								control_flags |= r_entry.Flag;
							else if(prop_val.IsEqiAscii("false") || prop_val.IsEqiAscii("no") || prop_val.IsEqiAscii(".F."))
								control_flags &= ~r_entry.Flag;
							else {
								// @err invalid readonly value
							}
						}
						occurence_flags |= r_entry.OccurenceFlag;										
						processed = true;
					}
				}
			}
			if(!processed) {
				for(uint tidx = 0; !processed && tidx < SIZEOFARRAY(alignment_list); tidx++) {
					AlignmentEntry & r_entry = alignment_list[tidx];
					if(prop_key == r_entry.P_Prop) {
						if(!r_entry.Occured) {
							if(p_prop->Value.IsIdent() || p_prop->Value.IsString()) {
								prop_val = p_prop->Value.U.S;
								if(prop_val.IsEqiAscii("auto"))
									*r_entry.P_Var = SUiLayoutParam::alignAuto;
								else if(prop_val.IsEqiAscii("stretch"))
									*r_entry.P_Var = SUiLayoutParam::alignStretch;
								else if(prop_val.IsEqiAscii("center"))
									*r_entry.P_Var = SUiLayoutParam::alignCenter;
								else if(prop_val.IsEqiAscii("start"))
									*r_entry.P_Var = SUiLayoutParam::alignStart;
								else if(prop_val.IsEqiAscii("end"))
									*r_entry.P_Var = SUiLayoutParam::alignEnd;
								else if(prop_val.IsEqiAscii("spacebetween"))
									*r_entry.P_Var = SUiLayoutParam::alignSpaceBetween;
								else if(prop_val.IsEqiAscii("spacearound"))
									*r_entry.P_Var = SUiLayoutParam::alignSpaceAround;
								else if(prop_val.IsEqiAscii("spaceevenly"))
									*r_entry.P_Var = SUiLayoutParam::alignSpaceEvenly;
								else {
									// @erro invalid [r_entry.P_Prop] value
								}
							}
						}
						processed = true;
					}
				}
			}
			if(!processed) {
				if(prop_key == "orientation") {
					prop_val.Z();
					if(p_prop->Value.IsIdent() || p_prop->Value.IsString()) {
						prop_val = p_prop->Value.U.S;
					}
					if(prop_val.IsEqiAscii("horizontal") || prop_val.IsEqiAscii("horz") || prop_val.IsEqiAscii("horiz")) {
						THROW(!(alb.Flags & (SUiLayoutParam::fContainerRow|SUiLayoutParam::fContainerCol))); // @err dup feature
						alb.Flags |= alb.fContainerRow;					
					}
					else if(prop_val.IsEqiAscii("vertical") || prop_val.IsEqiAscii("vert")) {
						THROW(!(alb.Flags & (SUiLayoutParam::fContainerRow|SUiLayoutParam::fContainerCol))); // @err dup feature
						alb.Flags |= alb.fContainerCol;										
					}
					else {
						; // @err
					}
				}
				else if(prop_key == "horizontal") {
					//CtmExprConst c;
					//THROW(AddConst((uint32)scopeID, &c));
					//THROW(p_scope->AddConst(DlScope::cuifCtrlScope, c, 1));
					THROW(!(alb.Flags & (SUiLayoutParam::fContainerRow|SUiLayoutParam::fContainerCol))); // @err dup feature
					alb.Flags |= alb.fContainerRow;
				}
				else if(prop_key == "vertical") {
					THROW(!(alb.Flags & (SUiLayoutParam::fContainerRow|SUiLayoutParam::fContainerCol))); // @err dup feature
					alb.Flags |= alb.fContainerCol;
				}
				else if(prop_key == "wrap") {
					if(p_prop->Value.IsEmpty()) {
						THROW(!(occurence_flags & occfNoWrap) && !(alb.Flags & (alb.fContainerWrap|alb.fContainerWrapReverse))); // @err dup feature
						alb.Flags |= alb.fContainerWrap;
					}
					else {
						if(p_prop->Value.IsIdent() || p_prop->Value.IsString()) {
							THROW(!(occurence_flags & occfNoWrap) && !(alb.Flags & (alb.fContainerWrap|alb.fContainerWrapReverse))); // @err dup feature
							prop_val = p_prop->Value.U.S;
						}
						if(prop_val.IsEqiAscii("true") || prop_val.IsEqiAscii("yes") || prop_val.IsEqiAscii(".T.")) {
							THROW(!(occurence_flags & occfNoWrap) && !(alb.Flags & (alb.fContainerWrap|alb.fContainerWrapReverse))); // @err dup feature
							alb.Flags |= alb.fContainerWrap;
						}
						else if(prop_val.IsEqiAscii("reverse")) {
							THROW(!(occurence_flags & occfNoWrap) && !(alb.Flags & (alb.fContainerWrap|alb.fContainerWrapReverse))); // @err dup feature
							alb.Flags |= (alb.fContainerWrap|alb.fContainerWrapReverse);
						}
						else if(prop_val.IsEqiAscii("false") || prop_val.IsEqiAscii("no") || prop_val.IsEqiAscii(".F.")) {
							THROW(!(occurence_flags & occfNoWrap) && !(alb.Flags & (alb.fContainerWrap|alb.fContainerWrapReverse))); // @err dup feature
							occurence_flags |= occfNoWrap;
							alb.Flags &= ~(alb.fContainerWrap|alb.fContainerWrapReverse);
						}
						else {
							// @err invalid wrap value
						}
					}
				}
				else if(prop_key == "nowrap") {
					THROW(p_prop->Value.IsEmpty()); // @err invalid nowrap value
					THROW(!(occurence_flags & occfNoWrap) && !(alb.Flags & (alb.fContainerWrap|alb.fContainerWrapReverse))); // @err dup feature
					occurence_flags |= occfNoWrap;
					alb.Flags &= ~(alb.fContainerWrap|alb.fContainerWrapReverse);
				}
				else if(prop_key == "class") {
					THROW(class_ident.IsEmpty()); // @err dup feature
					if(p_prop->Value.IsIdent() || p_prop->Value.IsString()) {
						prop_val = p_prop->Value.U.S;
						(class_ident = prop_val).Strip();
					}
					else {
						// @err invalid class value
					}
				}
				else if(prop_key == "font") {
					if(font_ident.IsEmpty()) { // @err dup feature
						if(p_prop->Value.IsIdent() || p_prop->Value.IsString()) {
							prop_val = p_prop->Value.U.S;
							SString left_buf, right_buf;
							if(prop_val.Divide(',', left_buf, right_buf) > 0 && left_buf.NotEmptyS() && right_buf.ToReal() > 0.0) {
								(font_ident = left_buf).Strip();
								font_size = right_buf.ToReal();
							}
							else {
								(font_ident = prop_val).Strip();
							}
						}
						else {
							// @err invalid class value
						}
					}
				}
				else if(prop_key == "fontsize") {
					uint cast_flags;
					if(font_size == 0.0) { // @err dup feature
						font_size = p_prop->Value.GetDouble(&cast_flags);
						THROW(font_size > 0.0 && font_size < 200.0); // @err invalid font size
					}
				}
				else if(prop_key == "title" || prop_key == "label") {
					if(title_text.IsEmpty()) { // @err dup feature
						if(p_prop->Value.IsIdent() || p_prop->Value.IsString()) {
							prop_val = p_prop->Value.U.S;
							(title_text = prop_val).Strip();
						}
						else {
							// @err invalid title or label value
						}
					}
				}
				else if(prop_key == "figuresymb" || prop_key == "figuresymbol") {
					if(figure_symb.IsEmpty()) { // @err dup feature
						if(p_prop->Value.IsIdent() || p_prop->Value.IsString()) {
							prop_val = p_prop->Value.U.S;
							(figure_symb = prop_val).Strip();
						}
						else {
							// @err invalid figuresymb value
						}
					}
				}
				else if(prop_key == "height") {
					THROW(p_prop->Value.Code == CtmToken::acLayoutItemSizeEntry); // @err invalid height value
					if(alb.GetSizeY(0) == SUiLayoutParam::szUndef) { // @err dup feature
						const float fv = p_prop->Value.U.UIC.Val;
						if(p_prop->Value.U.UIC.Flags & UiCoord::dfRel) {
							THROW(fv > 0.0f && fv <= 100.0f); // @err invalid height value
							alb.SetVariableSizeY(SUiLayoutParam::szByContainer, fv / 100.0f);
						}
						else if(p_prop->Value.U.UIC.Flags & UiCoord::dfContent) {
							alb.SetVariableSizeY(SUiLayoutParam::szByContent, 0.0f);
						}
						else {
							THROW(fv > 0.0f && fv <= 32000.0f); // @err invalid height value
							alb.SetFixedSizeY(p_prop->Value.U.UIC.Val);
						}
					}
				}
				else if(prop_key == "width") {
					THROW(p_prop->Value.Code == CtmToken::acLayoutItemSizeEntry); // @err invalid width value
					if(alb.GetSizeX(0) == SUiLayoutParam::szUndef) { // @err dup feature
						const float fv = p_prop->Value.U.UIC.Val;
						if(p_prop->Value.U.UIC.Flags & UiCoord::dfRel) {
							THROW(fv > 0.0f && fv <= 100.0f); // @err invalid width value
							alb.SetVariableSizeX(SUiLayoutParam::szByContainer, fv / 100.0f);
						}
						else if(p_prop->Value.U.UIC.Flags & UiCoord::dfContent) {
							alb.SetVariableSizeX(SUiLayoutParam::szByContent, 0.0f);
						}
						else {
							THROW(fv > 0.0f && fv <= 32000.0f); // @err invalid width value
							alb.SetFixedSizeX(p_prop->Value.U.UIC.Val);
						}
					}
				}
				else if(prop_key == "margin") {
					THROW(p_prop->Value.Code == CtmToken::acBoundingBox); // @err invalid margin value
					if(!(occurence_margin & (occsLeft|occsTop|occsRight|occsBottom))) { // @err dup feature
						alb.Margin.a.Set(p_prop->Value.U.Rect.L.X.Val, p_prop->Value.U.Rect.L.Y.Val);
						alb.Margin.b.Set(p_prop->Value.U.Rect.R.X.Val, p_prop->Value.U.Rect.R.Y.Val);
						occurence_margin |= (occsLeft|occsTop|occsRight|occsBottom);
					}
				}
				else if(prop_key == "margin_left") {
					const float fv = p_prop->Value.GetFloat(0);
					THROW(fv >= 32000.0f && fv <= 32000.0f); // @err invalid margin_left value
					if(!(occurence_margin & (occsLeft))) { // @err dup feature
						alb.Margin.a.x = fv;
						occurence_margin |= occsLeft;
					}
				}
				else if(prop_key == "margin_top") {
					const float fv = p_prop->Value.GetFloat(0);
					THROW(fv >= 32000.0f && fv <= 32000.0f); // @err invalid margin_top value
					if(!(occurence_margin & (occsTop))) { // @err dup feature
						alb.Margin.a.y = fv;
						occurence_margin |= occsTop;
					}
				}
				else if(prop_key == "margin_right") {
					const float fv = p_prop->Value.GetFloat(0);
					THROW(fv >= 32000.0f && fv <= 32000.0f); // @err invalid margin_right value
					if(!(occurence_margin & (occsRight))) { // @err dup feature
						alb.Margin.b.x = fv;
						occurence_margin |= occsRight;
					}
				}
				else if(prop_key == "margin_botton") {
					const float fv = p_prop->Value.GetFloat(0);
					THROW(fv >= 32000.0f && fv <= 32000.0f); // @err invalid margin_bottom value
					if(!(occurence_margin & (occsBottom))) { // @err dup feature
						alb.Margin.b.y = fv;
						occurence_margin |= occsBottom;
					}
				}
				else if(prop_key == "padding") {
					THROW(p_prop->Value.Code == CtmToken::acBoundingBox); // @err invalid padding value
					if(!(occurence_padding & (occsLeft|occsTop|occsRight|occsBottom))) { // @err dup feature
						alb.Padding.a.Set(p_prop->Value.U.Rect.L.X.Val, p_prop->Value.U.Rect.L.Y.Val);
						alb.Padding.b.Set(p_prop->Value.U.Rect.R.X.Val, p_prop->Value.U.Rect.R.Y.Val);
						occurence_padding |= (occsLeft|occsTop|occsRight|occsBottom);
					}
				}
				else if(prop_key == "padding_left") {
					const float fv = p_prop->Value.GetFloat(0);
					THROW(fv >= 32000.0f && fv <= 32000.0f); // @err invalid padding_left value
					if(!(occurence_padding & (occsLeft))) { // @err dup feature
						alb.Padding.a.x = fv;
						occurence_padding |= occsLeft;
					}
				}
				else if(prop_key == "padding_top") {
					const float fv = p_prop->Value.GetFloat(0);
					THROW(fv >= 32000.0f && fv <= 32000.0f); // @err invalid padding_top value
					if(!(occurence_padding & (occsTop))) { // @err dup feature
						alb.Padding.a.y = fv;
						occurence_padding |= occsTop;
					}
				}
				else if(prop_key == "padding_right") {
					const float fv = p_prop->Value.GetFloat(0);
					THROW(fv >= 32000.0f && fv <= 32000.0f); // @err invalid padding_right value
					if(!(occurence_padding & (occsRight))) { // @err dup feature
						alb.Padding.b.x = fv;
						occurence_padding |= occsRight;
					}
				}
				else if(prop_key == "padding_bottom") {
					const float fv = p_prop->Value.GetFloat(0);
					THROW(fv >= 32000.0f && fv <= 32000.0f); // @err invalid padding_bottom value
					if(!(occurence_padding & (occsBottom))) { // @err dup feature
						alb.Padding.b.y = fv;
						occurence_padding |= occsBottom;
					}
				}
				else if(prop_key == "aspectratio") {
					const float fv = p_prop->Value.GetFloat(0);
					THROW(fv > 0.0f && fv <= 32000.0f); // @err invalid aspectratio value
					if(alb.AspectRatio == 0.0f) { // @err dup feature
						alb.AspectRatio = fv;
					}
				}
				else if(prop_key == "bbox") {
					THROW(p_prop->Value.Code == CtmToken::acBoundingBox); // @err invalid bbox value
					if(!(occurence_flags & (occfBBox|occfBBoxOrigin))) { // @err dup feature
						alb.Nominal.a.Set(p_prop->Value.U.Rect.L.X.Val, p_prop->Value.U.Rect.L.Y.Val);
						alb.Nominal.b.Set(p_prop->Value.U.Rect.R.X.Val, p_prop->Value.U.Rect.R.Y.Val);
						occurence_flags |= occfBBox;
					}
				}
				else if(prop_key == "origin") { // @v12.2.9
					THROW(p_prop->Value.Code == CtmToken::acBoundingBoxOrigin); // @err invalid bbox value
					if(!(occurence_flags & (occfBBox|occfBBoxOrigin))) { // @err dup feature
						alb.Nominal.a.Set(p_prop->Value.U.Rect.L.X.Val, p_prop->Value.U.Rect.L.Y.Val);
						//alb.Nominal.b.Set(p_prop->Value.U.Rect.R.X.Val, p_prop->Value.U.Rect.R.Y.Val);
						occurence_flags |= occfBBoxOrigin;
					}
				}
				else if(prop_key == "size") { // @v12.2.9
					// @todo
				}
				else if(prop_key == "variable") {
					if(var_ident.IsEmpty()) { // @err dup feature
						if(p_prop->Value.IsIdent() || p_prop->Value.IsString()) {
							prop_val = p_prop->Value.U.S;
							(var_ident = prop_val).Strip();
						}
						else {
							// @err invalid variable value
						}
					}
				}
				else if(prop_key == "data") {
					THROW(data_ident.IsEmpty()); // @err dup feature
					if(p_prop->Value.IsIdent() || p_prop->Value.IsString()) {
						prop_val = p_prop->Value.U.S;
						(data_ident = prop_val).Strip();
					}
					else {
						// @err invalid variable value
					}
				}
				else if(prop_key == "labelrelation") {
					THROW(label_relation == 0); // @err dup feature
					if(p_prop->Value.IsIdent() || p_prop->Value.IsString()) {
						prop_val = p_prop->Value.U.S;
						if(prop_val.IsEqiAscii("left"))
							label_relation = SIDE_LEFT;
						else if(prop_val.IsEqiAscii("top"))
							label_relation = SIDE_TOP;
						else if(prop_val.IsEqiAscii("right"))
							label_relation = SIDE_RIGHT;
						else if(prop_val.IsEqiAscii("bottom"))
							label_relation = SIDE_BOTTOM;
						else {
							// @err invalid labelrelation value
						}
					}
					else {
						// @err invalid labelrelation value
					}
				}
				else if(prop_key == "labelbbox") {
					THROW(p_prop->Value.Code == CtmToken::acBoundingBox); // @err invalid bbox value
					if(!(occurence_flags & occfLabelBBox)) { // @err dup feature
						label_bbox.a.Set(p_prop->Value.U.Rect.L.X.Val, p_prop->Value.U.Rect.L.Y.Val);
						label_bbox.b.Set(p_prop->Value.U.Rect.R.X.Val, p_prop->Value.U.Rect.R.Y.Val);
						occurence_flags |= occfLabelBBox;
					}
				}
				else if(prop_key == "command") {
					if(command_ident.IsEmpty()) { // @err dup feature
						if(p_prop->Value.IsIdent() || p_prop->Value.IsString()) {
							prop_val = p_prop->Value.U.S;
							(command_ident = prop_val).Strip();
						}
						else {
							// @err invalid variable value
						}
					}
				}
				else if(prop_key == "gravity") {
					if(!alb.GravityX && !alb.GravityY) { // @err dup feature
						if(p_prop->Value.IsIdent() || p_prop->Value.IsString()) {
							prop_val = p_prop->Value.U.S;
							if(prop_val.IsEqiAscii("left")) {
								alb.GravityX = SIDE_LEFT;
								alb.GravityY = 0;
							}
							else if(prop_val.IsEqiAscii("top")) {
								alb.GravityX = 0;
								alb.GravityY = SIDE_TOP;
							}
							else if(prop_val.IsEqiAscii("right")) {
								alb.GravityX = SIDE_RIGHT;
								alb.GravityY = 0;
							}
							else if(prop_val.IsEqiAscii("bottom")) {
								alb.GravityX = 0;
								alb.GravityY = SIDE_BOTTOM;
							}
							else if(prop_val.IsEqiAscii("lefttop")) {
								alb.GravityX = SIDE_LEFT;
								alb.GravityY = SIDE_TOP;
							}
							else if(prop_val.IsEqiAscii("righttop")) {
								alb.GravityX = SIDE_RIGHT;
								alb.GravityY = SIDE_TOP;
							}
							else if(prop_val.IsEqiAscii("leftbottom")) {
								alb.GravityX = SIDE_LEFT;
								alb.GravityY = SIDE_BOTTOM;
							}
							else if(prop_val.IsEqiAscii("rightbottom")) {
								alb.GravityX = SIDE_RIGHT;
								alb.GravityY = SIDE_BOTTOM;
							}
							else if(prop_val.IsEqiAscii("center")) {
								alb.GravityX = SIDE_CENTER;
								alb.GravityY = SIDE_CENTER;
							}
							else {
								// @err invalid gravity value
							}
						}
					}
				}
				else if(prop_key == "growfactor") {
					const float fv = p_prop->Value.GetFloat(0);
					THROW(fv >= 0.0f && fv <= 32000.0f); // @err invalid growfactor value
					THROW(!(occurence_flags & occfGrowFactor)); // @err dup feature
					alb.GrowFactor = fv;
					occurence_flags |= occfGrowFactor;
				}
				else if(prop_key == "shrinkfactor") {
					const float fv = p_prop->Value.GetFloat(0);
					THROW(fv >= 0.0f && fv <= 32000.0f); // @err invalid shrinkfactor value
					THROW(!(occurence_flags & occfShrinkFactor)); // @err dup feature
					alb.ShrinkFactor = fv;
					occurence_flags |= occfShrinkFactor;
				}
				else if(prop_key == "format") {
				}
				else {
					// @todo @err invalid property
				}
			}
		}
	}
	if(pViewKind) {
		if(pViewKind->IsIdent() || pViewKind->IsString()) {
			uint32 view_kind = 0;
			prop_val = pViewKind->U.S;
			if(prop_val.IsEqiAscii("view")) {
				view_kind = UiItemKind::kGenericView;
			}
			else if(prop_val.IsEqiAscii("dialog")) {
				view_kind = UiItemKind::kDialog;
			}
			else if(prop_val.IsEqiAscii("input")) {
				view_kind = UiItemKind::kInput;
			}
			else if(prop_val.IsEqiAscii("statictext")) {
				view_kind = UiItemKind::kStatic;
			}
			else if(prop_val.IsEqiAscii("imageview")) { // @v11.0.6
				view_kind = UiItemKind::kImageView;
			}
			else if(prop_val.IsEqiAscii("frame")) {
				view_kind = UiItemKind::kFrame;
			}
			else if(prop_val.IsEqiAscii("combobox")) {
				view_kind = UiItemKind::kCombobox;
			}
			else if(prop_val.IsEqiAscii("button")) {
				view_kind = UiItemKind::kPushbutton;
			}
			else if(prop_val.IsEqiAscii("checkbox")) {
				view_kind = UiItemKind::kCheckbox;
			}
			else if(prop_val.IsEqiAscii("checkboxcluster")) {
				view_kind = UiItemKind::kCheckCluster;
			}
			else if(prop_val.IsEqiAscii("radiocluster")) {
				view_kind = UiItemKind::kRadioCluster;
			}
			else if(prop_val.IsEqiAscii("radiobutton")) {
				view_kind = UiItemKind::kRadiobutton;
			}
			else if(prop_val.IsEqiAscii("listbox")) {
				view_kind = UiItemKind::kListbox;
			}
			else if(prop_val.IsEqiAscii("treelistbox")) {
				view_kind = UiItemKind::kTreeListbox;
			}
			else {
				// @error invalid view kind
			}
			{
				SETIFZ(view_kind, UiItemKind::kGenericView);
				CtmExprConst c;
				THROW(AddConst(view_kind, &c));
				p_scope->AddConst(DlScope::cuifViewKind, c, 1);
			}
		}
	}
	if(typeID) {
		CtmExprConst c;
		AddConst(static_cast<uint32>(typeID), &c);
		p_scope->AddConst(DlScope::cuifViewDataType, c, 1);
	}
	{
		CtmExprConst c;
		AddConst(&alb, sizeof(alb), &c);
		p_scope->AddConst(DlScope::cuifLayoutBlock, c, 1);
	}
	{
		CtmExprConst c;
		AddConst(static_cast<uint32>(control_flags), &c);
		p_scope->AddConst(DlScope::cuifFlags, c, 1);
	}
	if(command_ident.NotEmpty()) {
		CtmExprConst c;
		AddConst(command_ident, &c);
		p_scope->AddConst(DlScope::cuifCtrlCmdSymb, c, 1);		
	}
	if(data_ident.NotEmpty()) {
		CtmExprConst c;
		AddConst(data_ident, &c);
		p_scope->AddConst(DlScope::cuifViewDataIdent, c, 1);		
	}
	if(var_ident.NotEmpty()) {
		CtmExprConst c;
		AddConst(var_ident, &c);
		p_scope->AddConst(DlScope::cuifViewVariableIdent, c, 1);		
	}
	if(title_text.NotEmpty()) {
		CtmExprConst c;
		AddConst(title_text, &c);
		p_scope->AddConst(DlScope::cuifCtrlText, c, 1);		
	}
	if(font_ident.NotEmpty()) {
		CtmExprConst c;
		AddConst(font_ident, &c);
		p_scope->AddConst(DlScope::cuifFont, c, 1);		
	}
	if(font_size > 0.0) {
		CtmExprConst c;
		AddConst(font_size, &c);
		p_scope->AddConst(DlScope::cuifFontSize, c, 1);				
	}
	if(label_relation) {
		CtmExprConst c;
		AddConst(static_cast<int8>(label_relation), &c);
		p_scope->AddConst(DlScope::cuifCtrlLblRelation, c, 1);		
	}
	if(occurence_flags & occfLabelBBox) {
		CtmExprConst c;
		AddConst(&label_bbox, sizeof(label_bbox), &c);
		p_scope->AddConst(DlScope::cuifLabelRect, c, 1);
	}
	CATCHZOK
	return ok;
}

int DlContext::AddTempFldProp(const CtmToken & rSymb, long val)
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

int DlContext::AddTempFldProp(const CtmToken & rSymb, double val)
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

int DlContext::AddTempFldProp(const CtmToken & rSymb, const char * pStr)
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

int DlContext::AddTempFldProp(const CtmToken & rSymb, const void * pData, size_t sz)
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

uint DlContext::AddUiCluster(int kind, const CtmToken & rSymb, const CtmToken & rText, DLSYMBID typeID, const UiRelRect & rRect)
{
	uint   fld_id = 0;
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
	fld_id = AddUiCtrl(kind, rSymb, rText, type_id, rRect);
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

int DlContext::AddUiClusterItem(const CtmToken & rText, const UiRelRect & rRect, const CtmToken & rDescr)
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

DlScope * DlContext::GetCurDialogScope()
{
	const DlScope * p_scope = GetCurScope();
	while(p_scope) {
		if(p_scope->IsKind(DlScope::kUiDialog))
			break;
		else
			p_scope = p_scope->GetOwner();
	}
	return const_cast<DlScope *>(p_scope); // @badcast
}

int DlContext::GetUiSymbSeries(const char * pSymb, SString & rSerBuf, DLSYMBID * pId)
{
	rSerBuf.Z();
	int    ok = 0;
	DLSYMBID symb_id = 0;
	SString pfx, ser, sfx;
	SString symb(pSymb);
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

uint DlContext::AddUiListbox(const CtmToken & rSymb, const CtmToken & rText, const UiRelRect & rRect, const CtmToken & rColumns)
{
	DlScope * p_scope = 0;
	uint   fld_id = AddUiCtrl(DlScope::ckListbox, rSymb, rText, 0, rRect);
	THROW(fld_id);
	THROW(p_scope = GetCurScope());
	{
		SString columns_buf(rColumns.U.S);
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

uint DlContext::AddUiButton(const CtmToken & rSymb, const CtmToken & rText, const UiRelRect & rRect, const CtmToken & rCmdSymb)
{
	DlScope * p_scope = 0;
	uint   fld_id = AddUiCtrl(DlScope::ckPushbutton, rSymb, rText, 0, rRect);
	THROW(fld_id);
	THROW(p_scope = GetCurScope());
	{
		SString symb_ser;
		SString cmd_buf(rCmdSymb.U.S);
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
				if(!UiSymbAssoc.SearchByVal(cmd_id, 0))
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

uint DlContext::AddUiCtrl(int kind, const CtmToken & rSymb, const CtmToken & rText, DLSYMBID typeID, const UiRelRect & rRect)
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
			PTR32(s_buf)[0] = 0;
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
	if(fld.Name.IsEmpty()) {
		if(dlg_symb_ser.NotEmpty() || symb_ser.NotEmpty()) {
			(fld.Name = "CTL").CatChar('_').Cat(dlg_symb_ser.NotEmpty() ? dlg_symb_ser : symb_ser).CatChar('_').CatLongZ(p_dlg_scope->GetLocalId(), 3);
		}
		else {
			(fld.Name = "CTL").CatChar('_').Cat("NS").CatChar('_').CatLongZ(p_dlg_scope->GetLocalId(), 3);
		}
	}
	THROW(fld_id = CreateSymb(fld.Name, '$', 0));
	if(!UiSymbAssoc.SearchByVal(fld_id, 0)) {
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
		AddedMsgString.Z();
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
				THROW(p_scope->AddFldConst(fld_id, DlScope::cuifViewKind, c, 1));
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
SString & GetIdlTypeString(TYPEID typ, SString & rBuf, const char * pFldName, size_t indent)
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
		case S_INT64: type_text = "__int64"; break; // @v10.6.3
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
		// @v11.8.5 case S_WZSTRING:  type_text = "wchar_t"; dim = sz / 2; break;
		case S_WZSTRING:  type_text = "BSTR"; dim = sz; break; // @v11.8.5
		case S_WCHAR:     type_text = "wchar_t"; dim = sz / 2; break;
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

int DlContext::Format_C_Type(DLSYMBID typeID, STypEx & rTyp, const char * pFldName, long flags, SString & rBuf)
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
			name_buf.Z();
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
				// Указатель на ссылку недопустим - будет просто указатель на SString
				if((rTyp.Flags & STypEx::fOf) || rTyp.Mod != STypEx::modPtr)
					type_buf.Space().CatChar('&');
			}
		}
		// @v11.8.5 {
		else if(flags & fctfIfaceImpl && GETSTYPE(td.T.Typ) == S_WZSTRING) {
			type_buf = "SStringU";
			if(!(flags & fctfInstance)) {
				// Указатель на ссылку недопустим - будет просто указатель на SStringU
				if((rTyp.Flags & STypEx::fOf) || rTyp.Mod != STypEx::modPtr)
					type_buf.Space().CatChar('&');
			}
		}
		// } @v11.8.5 
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

int DlContext::GetFuncName(int, const CtmExpr * pExpr, SString & rBuf)
{
	int    ok = 1;
	SString arg_buf, temp_buf;
	rBuf.Z();
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

int DlContext::Format_Func(const DlFunc & rFunc, long options, SString & rBuf)
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

int DlContext::Write_Scope(int indent, SFile & rOutFile, const DlScope & rScope)
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
		fld_buf.Z().CatCharN('\t', indent+1).Cat(fld.Name).CatDiv(';', 2).CatEq("size", (long)fld.T.GetBinSize()).CR();
		rOutFile.WriteLine(fld_buf);
	}
	for(i = 0; rScope.EnumChilds(&i, &p_child);)
		if(p_child)
			Write_Scope(indent+1, rOutFile, *p_child); // @recursion
		else
			rOutFile.WriteLine(line.Z().CatCharN('\t', indent+1).Cat("null").CR());
	{
		DlFunc func;
		SString func_name;
		for(i = 0; rScope.GetFuncByPos(i, &func); i++) {
			Format_Func(func, 0, func_name);
			line.Z().CatCharN('\t', indent+1).Cat(func_name).CR();
			rOutFile.WriteLine(line);
		}
	}
	return 1;
}

void DlContext::Write_DebugListing()
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
		Format_TypeEntry(TypeList.at(i), line.Z());
		out_file.WriteLine(line.CR());
	}
}

static SString & FASTCALL _RectToLine(const UiRelRect & rRect, SString & rBuf)
{
	return rBuf.CatChar('(').Cat(rRect.L.X.Val).CatDiv(',', 2).Cat(rRect.L.Y.Val).CatDiv(',', 2).
		Cat(rRect.R.X.Val).CatDiv(',', 2).Cat(rRect.R.Y.Val).CatChar(')');
}

int DlContext::Write_DialogReverse()
{
	int    ok = 1;
	SString out_file_name;
	{
		SFsPath ps;
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
			line_buf.Z().CR().Cat("dialog").Space().Cat(p_scope->Name);
			{
				text_buf.Z();
				if(GetConstData(p_scope->GetConst(DlScope::cuifCtrlText), c_buf, sizeof(c_buf)))
					text_buf = c_buf;
				line_buf.Space().CatQStr(text_buf);
			}
			if(GetConstData(p_scope->GetConst(DlScope::cuifCtrlRect), c_buf, sizeof(c_buf)))
				_RectToLine(*reinterpret_cast<const UiRelRect *>(c_buf), line_buf.Space());
			if(GetConstData(p_scope->GetConst(DlScope::cuifFont), c_buf, sizeof(c_buf))) {
				if(PTR8C(c_buf)[0]) {
					text_buf.Z().CatQStr(c_buf);
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
				rect.Z();
				prop_list.Z();
				line_buf.Z().CatChar('\t');
				if(GetConstData(p_scope->GetFldConst(ctrl.ID, DlScope::cuifViewKind), c_buf, sizeof(c_buf)))
					kind = *reinterpret_cast<const uint32 *>(c_buf);
				if(GetConstData(p_scope->GetFldConst(ctrl.ID, DlScope::cuifCtrlRect), c_buf, sizeof(c_buf)))
					rect = *reinterpret_cast<const UiRelRect *>(c_buf);
				//
				// Находим строку текста элемента
				//
				text_buf.Z();
				if(GetConstData(p_scope->GetFldConst(ctrl.ID, DlScope::cuifCtrlText), c_buf, sizeof(c_buf)))
					text_buf = c_buf;
				//
				// Формируем строку типа данных STypEx
				//
				type_buf.Z();
				GetBinaryTypeString(ctrl.T.Typ, 1, type_buf, "", 0);
				if(type_buf.Cmp("unknown", 0) == 0)
					(type_buf = "string").CatChar('[').Cat(48).CatChar(']');
				//
				cmd_buf.Z();
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
					_RectToLine(*reinterpret_cast<const UiRelRect *>(c_buf), temp_buf.Z());
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
						temp_buf = c_buf;
						line_buf.Space().Cat(temp_buf);
					}
				}
				else if(oneof2(kind, DlScope::ckCheckCluster, DlScope::ckRadioCluster)) {
					if(GetConstData(p_scope->GetFldConst(ctrl.ID, DlScope::cuifCtrlScope), c_buf, sizeof(c_buf))) {
						DLSYMBID inner_scope_id = *reinterpret_cast<const uint32 *>(c_buf);
						DlScope * p_inner_scope = GetScope(inner_scope_id);
						if(p_inner_scope && p_inner_scope->GetCount()) {
							skip_line_finish = 1;
							line_buf.Space().CatChar('{').CR();
							f_out.WriteLine(line_buf);
							for(uint k = 0; p_inner_scope->EnumFields(&k, &ctrl_item);) {
								line_buf.Z().CatCharN('\t', 2);
								rect.Z();
								if(GetConstData(p_inner_scope->GetFldConst(ctrl_item.ID, DlScope::cuifCtrlRect), c_buf, sizeof(c_buf)))
									rect = *reinterpret_cast<const UiRelRect *>(c_buf);
								text_buf.Z();
								if(GetConstData(p_inner_scope->GetFldConst(ctrl_item.ID, DlScope::cuifCtrlText), c_buf, sizeof(c_buf)))
									text_buf = reinterpret_cast<const char *>(c_buf);
								line_buf.CatQStr(text_buf);
								if(!rect.IsEmpty())
									_RectToLine(rect, line_buf.Space());
								line_buf.Semicol().CR();
								f_out.WriteLine(line_buf);
							}
							line_buf.Z().CatChar('\t').CatChar('}').CR();
							f_out.WriteLine(line_buf);
						}
					}
				}
				else if(kind == DlScope::ckListbox) {
					if(GetConstData(p_scope->GetFldConst(ctrl.ID, DlScope::cuifCtrlListboxColDescr), c_buf, sizeof(c_buf))) {
						temp_buf = c_buf;
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
			line_buf.Z().CatChar('}').CR();
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

int DlContext::AddPropDeclare(CtmDclr & rSymb, int propDirParam)
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
	{
		const uint   sc_kind = p_scope ? p_scope->GetKind() : 0;
		const int    is_exp_func = oneof2(sc_kind, DlScope::kExpDataHdr, DlScope::kExpDataIter);
		THROW_PP(is_exp_func || sc_kind == DlScope::kInterface, PPERR_DL6_FUNCINVSCOPE);
		if(is_exp_func)
			func.Name.CatChar('?').Cat(rSymb.Tok.U.S);
		else
			func.Name = rSymb.Tok.U.S;
	}
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
	p_scope->AddFunc(&func);
	CATCHZOK
	return ok;

#if 0 // @sample {
int DlContext::AddBFunc(const char * pFuncName, uint implID, const char * pRetType, va_list pArgList)
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
			Sc.AddFunc(&f);
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
			AddedMsgString.Z();
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
			pExpr->Pack(fld.InnerFormula); // @v10.9.1 Formula-->InnerFormula
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
				for(const DlScope * p_base = p_scope->GetBase(); !is_dup && p_base != 0; p_base = p_base->GetBase()) {
					if(p_base->SearchName(fld.Name, &(fld_pos = 0), 0))
						is_dup = 1;
				}
			}
			if(is_dup) {
				AddedMsgString.Z();
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

int DlContext::InitDbIndexName(const char * pIdxName, SString & rBuf)
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

int DlContext::AddDbIndexSegmentDeclaration(const char * pFieldName, long options)
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

int DlContext::ResolveDbIndexSegFlag(long flags, const char * pSymb, long * pResultFlags)
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

int DlContext::ResolveDbIndexFlag(const char * pSymb)
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

int DlContext::ResolveDbFileDefinition(const CtmToken & rSymb, const char * pConstStr, int constInt)
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
			Error(PPERR_DL6_INVDBFILEPAGE, temp_buf.Z().Cat(constInt), erfLog);
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

int DlContext::AddCvtFuncToArgList(const DlFunc & rFunc, CtmExpr * pExpr, const LongArray & rCvtPosList) const
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

int DlContext::ProcessQuestArgList(const DlFunc & rFunc, CtmExpr * pExpr, const LongArray & rCvtPosList)
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

int DlContext::IsFuncSuited(const DlFunc & rFunc, CtmExpr * pExpr, LongArray * pCvtArgList)
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
								AdjRetTypeProc proc = reinterpret_cast<AdjRetTypeProc>(proc_addr);
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
	// Cleaning instant members of Context
	CurDeclList.Z();
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
		SFsPath::ReplaceExt(file_name, "uuid", 1);
		SFile  f_out(file_name, SFile::mWrite);
		if(f_out.IsValid()) {
			SString line_buf, uuid_buf;
			UUIDAssoc * p_item;
			for(uint i = 0; UuidList.enumItems(&i, (void **)&p_item);) {
				const DlScope * p_scope = GetScope(p_item->Key, 0);
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
	SFsPath::ReplaceExt(file_name, "uuid", 1);
	SyncUuidNameList.Z();
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
DlContext::TypeDetail::TypeDetail()
{
	DimList.Init();
	PtrList.Init();
	T.Init();
}

DlContext::TypeDetail::~TypeDetail()
{
	DimList.Destroy();
	PtrList.Destroy();
}

int DlContext::TypeDetail::IsInterfaceTypeConversionNeeded() const
{
	const TYPEID st = GETSTYPE(T.Typ);
	if(oneof3(st, S_DATE, S_TIME, S_DATETIME))
		return 1;
	else if(oneof5(st, S_CHAR, S_ZSTRING, S_LSTRING, S_NOTE, S_WZSTRING)) // @v11.8.5 S_WZSTRING
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

DLSYMBID DlContext::EnterViewScope(const char * pSymb)
{
	const uint scope_kind = DlScope::kUiView;
	// @fixme AddStructType(symb_id) в случае view не нужно (или нужно?)
	DLSYMBID id = 0;
	DlScope * p_cur_scope = GetCurScope();
	assert(p_cur_scope); // Не может такого быть, что нет текущей области: значит мы вызывали функцию откуда-то не от туда.
	if(p_cur_scope) {
		DLSYMBID symb_id = 0;
		SString name_;
		if(isempty(pSymb)) {
			S_GUID uuid;
			uuid.Generate();
			uuid.ToStr(S_GUID::fmtPlain, name_); // automatic generated ident
			const int ssr = SearchSymb(name_, '^', &symb_id);
			assert(!ssr); // Если ssr != то нам удалось найти дубликат GUID'а: можно ползти на кладбище - в этом чертовом мире делать больше нечего
			if(!ssr) {
				symb_id = CreateSymb(name_, '^', 0);
				THROW(symb_id);
			}
			else {
				SetError(PPERR_DL6_CLASSEXISTS, name_);
				CALLEXCEPT();
			}
		}
		else {
			const DlScope * p_top_view_scope = 0;
			const DlScope * p_par = p_cur_scope;
			if(p_par->GetKind() == scope_kind) {
				do {
					p_top_view_scope = p_par;
					p_par = p_par->GetOwner();
				} while(p_par && p_par->GetKind() == scope_kind);
			}
			if(p_top_view_scope) {
				//
				// Если родительская область вида DlScope::kUiView существует, то проверяем уникальность
				// символа только внутри нее.
				//
				DLSYMBID local_par_id = 0;
				if(!p_top_view_scope->SearchByName_Const(scope_kind, pSymb, &local_par_id)) {
					if(!SearchSymb(pSymb, '^', &symb_id)) {
						symb_id = CreateSymb(pSymb, '^', 0);
						THROW(symb_id);
					}					
				}
				else {
					CALLEXCEPT(); // @todo err
				}
			}
			else {
				//
				// Если родительская область вида DlScope::kUiView не существует, значит мы создаем топовую
				// область такого вида. Стало быть символ должет быть уникальным "насквозь".
				//
				if(!SearchSymb(pSymb, '^', &symb_id)) {
					symb_id = CreateSymb(pSymb, '^', 0);
					THROW(symb_id);
				}
				else {
					SetError(PPERR_DL6_CLASSEXISTS, pSymb);
					CALLEXCEPT();
				}
			}
		}
		id = EnterScope(scope_kind, name_, symb_id, 0);  // view {
	}
	CATCH
		id = 0;
	ENDCATCH
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
	DLSYMBID scope_id = *static_cast<const DLSYMBID *>(ScopeStack.pop());
	if(scope_id) {
		scope_id = Sc.EnterScope(CurScopeID, scope_id, 0, 0);
		if(scope_id == 0) {
			AddedMsgString.Z().Cat(CurScopeID);
			Error(PPERR_DL6_SCOPEIDNFOUND, 0, erfExit);
		}
		else
			CurScopeID = scope_id;
	}
	return scope_id;
}

int DlContext::SetInheritance(DLSYMBID scopeID, DLSYMBID baseID)
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
	const  int idl_type = BIN(oneof4(format, ffIDL, ffH_Iface, ffCPP_Iface, ffCPP_VTbl));
	uint   c;
	SString temp_buf;
	SString arg_name;
	SString arg_buf;
	TypeEntry ret_te, te;
	TypeDetail td;
	THROW(SearchTypeID(rFunc.TypID, 0, &ret_te));
	rFunc.GetName(0, temp_buf);
	/*
		ffH_GravityIface,   // @v10.8.6 Прототип интерфейса Gravity в H-файле
		ffH_GravityImp,     // @v10.8.6 Прототип реализации интерфейса Gravity в H-файле
		ffCPP_GravityIface, // @v10.8.6 Реализация интефейса Gravity в CPP-файле
		ffCPP_GravityImp,   // @v10.8.6 Реализация Gravity в CPP-файле
	*/
	if(format == ffH_GravityIface) {
		arg_buf.Z().Cat("_Callee_").Cat(temp_buf);
		gen.Wr_StartDeclFunc(gen.fkOrdinary, gen.fmStatic, "bool", arg_buf, gen.fcmCDecl);
		gen.Wr_VarDecl("gravity_vm *", "vm", 0, ',');
		gen.Wr_VarDecl("GravityValue *", "args", 0, ',');
		gen.Wr_VarDecl("uint16", "nargs", 0, ',');
		gen.Wr_VarDecl("uint32", "rindex", 0, 0);
		gen.Wr_EndDeclFunc(1, 1);
	}
	else if(format == ffCPP_GravityIface) {
		gen.Wr_StartDeclFunc(gen.fkOrdinary, 0, "bool", temp_buf, gen.fcmCDecl);
		gen.Wr_VarDecl("gravity_vm *", "vm", 0, ',');
		gen.Wr_VarDecl("GravityValue *", "args", 0, ',');
		gen.Wr_VarDecl("uint16", "nargs", 0, ',');
		gen.Wr_VarDecl("uint32", "rindex", 0, 0);
		gen.Wr_EndDeclFunc(0, 1);
		gen.Wr_OpenBrace();
		gen.Wr_CloseBrace(0, 0);
	}
	else if(format == ffH_GravityImp) {
		Format_C_Type(rFunc.TypID, ret_te.T, 0, fctfIfaceImpl, arg_buf);
		gen.Wr_StartDeclFunc(gen.fkOrdinary, 0, arg_buf, temp_buf, gen.fcmDefault);
		c = rFunc.GetArgCount();
		temp_buf.Z();
		if(c) {
			for(uint i = 0; i < c; i++) {
				DlFunc::Arg arg;
				rFunc.GetArgName(i, arg_name);
				if(i)
					temp_buf.Comma().Space();
				THROW(rFunc.GetArg(i, &arg));
				THROW(SearchTypeID(arg.TypID, 0, &te));
				if(IsInterfaceTypeConversionNeeded(te.T) > 0)
					arg_name.CatChar('_');
				Format_C_Type(arg.TypID, te.T, arg_name.cptr(), idl_type ? fctfIDL : fctfIfaceImpl, arg_buf);
				temp_buf.Cat(arg_buf);
			}
			gen.WriteLine(temp_buf);
		}
		gen.Wr_EndDeclFunc(1, 1);
	}
	else {
		if(format == ffIDL) {
			arg_buf.Z();
			if(rFunc.Flags & DlFunc::fPropGet)
				arg_buf = "propget";
			else if(rFunc.Flags & DlFunc::fPropPut)
				arg_buf = "propput";
			if(arg_buf.NotEmpty())
				THROW(gen.WriteLine(gen.CatIndent(arg_name.Z()).CatChar('[').Cat(arg_buf).CatChar(']').CR()));
		}
		else if(!oneof4(format, ffCPP_VTbl, ffCPP_Iface, ffCPP_CallImp, ffCPP_Imp)) {
			arg_buf.Z();
			if(rFunc.Flags & DlFunc::fPropGet)
				(arg_buf = "get").CatChar('_').Cat(temp_buf);
			else if(rFunc.Flags & DlFunc::fPropPut)
				(arg_buf = "put").CatChar('_').Cat(temp_buf);
			if(arg_buf.NotEmpty())
				temp_buf = arg_buf;
		}
		if(format == ffCPP_CallImp) {
			gen.CatIndent(arg_name.Z());
			if(pForward)
				arg_name.Cat(pForward);
			gen.WriteLine(arg_name.Cat(temp_buf).CatChar('('));
		}
		else if(idl_type) {
			THROW(gen.Wr_StartDeclFunc(0, 0, "HRESULT", temp_buf, (format == ffCPP_VTbl) ? Generator_CPP::fcmDefault : Generator_CPP::fcmStdCall));
		}
		else {
			Format_C_Type(rFunc.TypID, ret_te.T, 0, fctfIfaceImpl, arg_buf);
			THROW(gen.Wr_StartDeclFunc(0, 0, arg_buf, temp_buf, Generator_CPP::fcmDefault));
		}
		c = rFunc.GetArgCount();
		temp_buf.Z();
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
					Format_C_Type(arg.TypID, te.T, (format == ffCPP_VTbl) ? 0 : arg_name.cptr(), idl_type ? fctfIDL : fctfIfaceImpl, arg_buf);
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
					arg_name.Z();
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
	}
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
	gen.WriteLine(gen.CatIndent(line_buf.Z()).CatChar('[').CR());
	gen.IndentInc();
	if(UuidList.Search(rScope.ID, &uuid, 0))
		uuid.ToStr(S_GUID::fmtIDL, temp_buf);
	else
		temp_buf = "UUID not defined";
	line_buf.Z();
	if(rScope.IsKind(DlScope::kInterface)) {
		gen.CatIndent(line_buf).Cat("object");
		next = 1;
	}
	if(next)
		line_buf.Comma().CR();
	gen.CatIndent(line_buf).Cat("uuid").CatParStr(temp_buf);
	next = 1;
	if(rScope.GetAttrib(DlScope::sfVersion, &attr)) {
		temp_buf.Z().Cat(LoWord(attr.Ver)).Dot().Cat(HiWord(attr.Ver));
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
	gen.WriteLine(gen.CatIndent(line_buf.Z()).CatChar(']').CR());
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
				gen.CatIndent(line_buf.Z());
				if(p_ds->GetFieldByPos(j, &fld)) {
					Format_C_Type(0, fld.T, fld.Name, fctfIDL, temp_buf);
					line_buf.Cat("typedef").Space().Cat(temp_buf).Semicol().CR();
				}
				else
					line_buf.Cat("Error opening field");
				gen.WriteLine(line_buf.CR());
			}
			gen.WriteBlancLine();
		}
		else if(p_ds->IsKind(DlScope::kStruct)) {
			gen.CatIndent(line_buf.Z()).Cat("typedef").Space().Cat("struct").Space().
				/*Cat("tag").*/Cat(p_ds->Name).Space().CatChar('{').CR();
			gen.WriteLine(line_buf);
			gen.IndentInc();
			for(j = 0; j < p_ds->GetCount(); j++) {
				gen.CatIndent(line_buf.Z());
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
			gen.WriteBlancLine();
		}
		else if(p_ds->IsKind(DlScope::kEnum)) {
			gen.CatIndent(line_buf.Z()).Cat("typedef").Space().Cat("[v1_enum]").Space().
				Cat("enum").Space().CatChar('_').Cat(p_ds->Name).Space().CatChar('{').CR();
			gen.WriteLine(line_buf);
			gen.IndentInc();
			uint c = p_ds->GetCount();
			for(j = 0; j < c; j++) {
				gen.CatIndent(line_buf.Z());
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
			gen.WriteBlancLine();
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
						line_buf.Z();
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
			line_buf.Z().Cat("interface").Space().Cat("ISupportErrorInfo").Semicol().CR();
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
			gen.WriteLine(line_buf.Z().Cat("library").Space().Cat(p_ds->Name).Space().CatChar('{').CR());
			gen.IndentInc();
			gen.CatIndent(line_buf.Z()).Cat("importlib(\"STDOLE2.TLB\")").Semicol().CR();
			gen.WriteLine(line_buf);
			gen.WriteBlancLine();
			Write_IDL_File(gen, *p_ds); // @recursion
			do_write_end = 1;
		}
		else {
			THROW(Write_IDL_File(gen, *p_ds)); // @recursion
		}
		if(do_write_end) {
			gen.IndentDec();
			gen.Wr_CloseBrace(1, 0);
			gen.WriteBlancLine();
		}
	}
	CATCHZOK
	return ok;
}

void DlContext::Write_C_FileHeader(Generator_CPP & gen, const char * pFileName)
{
	SString temp_buf;
	SFsPath ps;
	ps.Split(pFileName);
	ps.Merge(0, SFsPath::fDrv|SFsPath::fDir, temp_buf);
	gen.Wr_Comment(temp_buf.ToUpper());
	temp_buf.Printf("This file was generated by DL600C.EXE from '%s'", InFileName.cptr());
	gen.Wr_Comment(temp_buf);
	gen.Wr_Comment(0);
}

int DlContext::MakeDlRecName(const DlScope * pRec, int instanceName, SString & rBuf) const
{
	int    ok = 1;
	rBuf.Z();
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

int DlContext::Write_C_DeclFile(Generator_CPP & gen, const DlScope & rScope, long cflags)
{
	int    ok = 1;
	int    r;
	SString cls_name;
	SString inst_name;
	SString fld_buf;
	SString line_buf;
	SdbField fld;
	DlFunc func;
	DlScope * p_ds = 0;
	for(uint scopeidx = 0; rScope.EnumChilds(&scopeidx, &p_ds);) {
		if(p_ds->IsKind(DlScope::kTypedefPool)) {
			for(uint fldidx = 0; p_ds->EnumFields(&fldidx, &fld);) {
				gen.CatIndent(line_buf.Z());
				Format_C_Type(0, fld.T, fld.Name, fctfIDL|fctfSourceOutput, fld_buf);
				gen.WriteLine(line_buf.Cat("typedef").Space().Cat(fld_buf).CR());
			}
			gen.WriteBlancLine();
		}
		else if(p_ds->IsKind(DlScope::kStruct)) {
			MakeClassName(p_ds, clsnfCPP, cflags, cls_name);
			gen.WriteLine(fld_buf.Z().Cat("#pragma pack(push)").CR());
			gen.WriteLine(fld_buf.Z().Cat("#pragma pack(8)").CR());
			gen.Wr_StartClassDecl(Generator_CPP::clsStruct, cls_name, 0, Generator_CPP::acsPublic, 0);
			gen.IndentInc();
			for(uint fldidx = 0; p_ds->EnumFields(&fldidx, &fld);) {
				gen.CatIndent(line_buf.Z());
				Format_C_Type(0, fld.T, fld.Name, fctfIDL|fctfSourceOutput, fld_buf);
				gen.WriteLine(line_buf.Cat(fld_buf).CR());
			}
			gen.IndentDec();
			gen.Wr_CloseBrace(1);
			gen.WriteLine(fld_buf.Z().Cat("#pragma pack(pop)").CR());
			gen.WriteBlancLine();
		}
		else if(p_ds->IsKind(DlScope::kEnum)) {
			MakeClassName(p_ds, clsnfCPP, cflags, cls_name);
			gen.Wr_StartClassDecl(Generator_CPP::clsEnum, cls_name, 0, Generator_CPP::acsPublic, 0);
			gen.IndentInc();
			for(uint fldidx = 0; p_ds->EnumFields(&fldidx, &fld);) {
				gen.CatIndent(line_buf.Z());
				line_buf.Cat(fld.Name).CatDiv('=', 1).Cat(fld.EnumVal);
				if(fldidx < p_ds->GetCount())
					line_buf.Comma();
				gen.WriteLine(line_buf.CR());
			}
			gen.IndentDec();
			gen.Wr_CloseBrace(1);
			gen.WriteBlancLine();
		}
		else if(p_ds->IsKind(DlScope::kIClass)) {
			DlScope::IfaceBase ifb;
			const DlScope * p_ifs;
			gen.WriteLine(fld_buf.Z().Cat("#pragma pack(push)").CR()); // @v11.7.0
			gen.WriteLine(fld_buf.Z().Cat("#pragma pack(8)").CR()); // @v11.7.0
			MakeClassName(p_ds, clsnfCPP, cflags, cls_name);
			gen.Wr_StartClassDecl(Generator_CPP::clsClass, cls_name, "SCoClass", Generator_CPP::acsPublic, 0);
			gen.Wr_ClassAcsZone(Generator_CPP::acsPublic);
			gen.IndentInc();
			gen.CatIndent(line_buf.Z()).Cat("DL6_IC_CONSTRUCTION_DECL").CatParStr(p_ds->Name).Semicol().CR();
			gen.WriteLine(line_buf);
			for(uint ifcidx = 0; (r = EnumInterfacesByICls(p_ds, &ifcidx, &ifb, &p_ifs)) >= 0;)
				if(r == 0) {
					fld_buf.Printf("Error extracting interface %s::%u", cls_name.cptr(), ifcidx-1);
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
					for(uint funcidx = 0; p_ifs->EnumFunctions(&funcidx, &func) > 0;) {
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
					for(uint funcidx = 0; p_ifs->EnumFunctions(&funcidx, &func) > 0;)
						Write_Func(gen, func, ffH_Imp);
				}
			//
			gen.IndentDec();
			gen.Wr_CloseBrace(1);
			gen.WriteLine(fld_buf.Z().Cat("#pragma pack(pop)").CR()); // @v11.7.0
			gen.WriteBlancLine();
		}
		else if(p_ds->IsKind(DlScope::kExpData) && !p_ds->GetBase()) {
			MakeClassName(p_ds, clsnfCPP, cflags, cls_name);
			if(p_ds->IsPrototype()) {
				gen.Wr_ClassPrototype(Generator_CPP::clsClass, cls_name);
			}
			else {
				DlScope * p_rec = 0;
				gen.WriteLine(fld_buf.Z().Cat("#pragma pack(push)").CR()); // @v11.7.0
				gen.WriteLine(fld_buf.Z().Cat("#pragma pack(1)").CR()); // @v11.7.0
				gen.Wr_StartClassDecl(Generator_CPP::clsClass, cls_name, "DlRtm", Generator_CPP::acsPublic, 0);
				gen.Wr_ClassAcsZone(Generator_CPP::acsPublic);
				gen.IndentInc();
				const int is_iter = BIN(p_ds->GetFirstChildByKind(DlScope::kExpDataIter, 0));
				int is_func_decl = 0;
				for(uint fldidx = 0; !is_func_decl && p_ds->EnumChilds(&fldidx, &p_rec);)
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
					gen.Wr_VarDecl("long", 0, 0, 0);
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
					gen.Wr_StartDeclFunc(Generator_CPP::fkOrdinary, Generator_CPP::fmVirtual, "void", "EvaluateFunc");
					gen.Wr_VarDecl("const DlFunc *", "pF", 0, ',');
					gen.Wr_VarDecl("SV_Uint32 *",  "pApl", 0, ',');
					gen.Wr_VarDecl("RtmStack &",  "rS", 0, 0);
					gen.Wr_EndDeclFunc(1, 1);
				}
				gen.WriteBlancLine();
				//
				for(uint strucidx = 0; p_ds->EnumChilds(&strucidx, &p_rec);) {
					if(p_rec->IsKind(DlScope::kExpDataHdr) || p_rec->IsKind(DlScope::kExpDataIter)) {
						MakeDlRecName(p_rec, 0, cls_name);
						MakeDlRecName(p_rec, 1, inst_name);
						gen.Wr_StartClassDecl(Generator_CPP::clsStruct, cls_name, 0, Generator_CPP::acsPublic, 0);
						gen.IndentInc();
						for(uint fldidx = 0; p_rec->EnumFields(&fldidx, &fld);)
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
				gen.WriteLine(fld_buf.Z().Cat("#pragma pack(pop)").CR()); // @v11.7.0
			}
			gen.WriteBlancLine();
		}
		else if(p_ds->IsKind(DlScope::kDbTable)) {
			MakeClassName(p_ds, clsnfCPP, cflags, cls_name);
			gen.WriteLine(fld_buf.Z().Cat("#pragma pack(push)").CR()); // @v11.7.0
			gen.WriteLine(fld_buf.Z().Cat("#pragma pack(1)").CR()); // @v11.7.0
			gen.Wr_StartClassDecl(Generator_CPP::clsClass, cls_name, "DBTable", Generator_CPP::acsPublic, 0);
			gen.Wr_ClassAcsZone(Generator_CPP::acsPublic);
			gen.IndentInc();
			//
			// Конструктор
			//
			gen.Wr_StartDeclFunc(Generator_CPP::fkConstr, 0, 0, cls_name);
			gen.Wr_VarDecl("const char *", "pFileName", "0", 0);
			gen.Wr_EndDeclFunc(1, 1);
			gen.WriteBlancLine();
			//
			// Структура записи
			//
			gen.Wr_StartClassDecl(Generator_CPP::clsStruct, "Rec", 0, Generator_CPP::acsPublic, 0);
			gen.IndentInc();
			gen.Wr_StartDeclFunc(Generator_CPP::fkConstr, 0, 0, "Rec");
			gen.Wr_EndDeclFunc(1, 1);
			// @v11.9.8 {
			gen.Wr_StartDeclFunc(Generator_CPP::fkOrdinary, 0, "Rec &", "Clear"); // Функция обнуления записи. Функция должны была называться Z() (ради унифицированной семантики), но 
				// из-за того, что в существующих таблицах есть поля с именем Z пришлось использовать олдскульный Clear().
			gen.Wr_EndDeclFunc(1, 1);
			// } @v11.9.8 
			for(uint fldidx = 0; p_ds->EnumFields(&fldidx, &fld);) {
				THROW(Format_C_Type(0, fld.T, fld.Name, fctfSourceOutput, fld_buf));
				if(fld.T.IsPure()) {
					int _t = GETSTYPE(fld.T.Typ);
					SString comment_type;
					if(_t == S_DEC) {
						(comment_type = "decimal").CatChar('[').Cat(GETSSIZED(fld.T.Typ)).Dot().Cat(GETSPRECD(fld.T.Typ)).CatChar(']');
					}
					else if(_t == S_MONEY) {
						(comment_type = "money").CatChar('[').Cat(GETSSIZED(fld.T.Typ)).Dot().Cat(GETSPRECD(fld.T.Typ)).CatChar(']');
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
			{
				//
				// Структуры индексов
				//
				DlScope * p_index = 0;
				for(uint idxidx = 0; p_ds->EnumChilds(&idxidx, &p_index);) {
					if(p_index->IsKind(DlScope::kDbIndex)) {
						MakeClassName(p_index, clsnfCPP, cflags, cls_name);
						gen.Wr_StartClassDecl(Generator_CPP::clsStruct, cls_name, 0, Generator_CPP::acsPublic, 0);
						gen.IndentInc();
						for(uint fldidx = 0; p_index->EnumFields(&fldidx, &fld);) {
							THROW(Format_C_Type(0, fld.T, fld.Name, fctfSourceOutput, fld_buf));
							fld_buf.CR();
							gen.Wr_Indent();
							gen.WriteLine(fld_buf);
						}
						gen.IndentDec();
						gen.Wr_CloseBrace(1, 0);
					}
				}
			}
			{
				//
				// Дескрипторы полей
				//
				for(uint fldidx = 0; p_ds->EnumFields(&fldidx, &fld);) {
					gen.Wr_Indent();
					gen.WriteLine((fld_buf = "DBField").Space().Cat(fld.Name).Semicol().CR());
				}
			}
			gen.IndentDec();
			gen.Wr_CloseBrace(1);
			gen.WriteLine(fld_buf.Z().Cat("#pragma pack(pop)").CR()); // @v11.7.0
			gen.WriteBlancLine();
		}
		else {
			THROW(Write_C_DeclFile(gen, *p_ds, cflags)); // @recursion
		}
	}
	CATCHZOK
	return ok;
}

static void Wr_YourCodeHere(Generator_CPP & gen)
{
	gen.IndentInc();
	gen.Wr_Comment("Your code here...");
	gen.IndentDec();
}

int DlContext::Write_C_ImplInterfaceFunc(Generator_CPP & gen, const SString & rClsName, DlFunc & rFunc, long cflags)
{
	int    ok = 1;
	int    is_ret = 0;
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
	if(cflags & cfGravity) {
		(mod_func_name = "_Callee_").Cat(rFunc.Name);
		rFunc.Name = gen.MakeClsfName(rClsName, mod_func_name, func_name);
		Write_Func(gen, rFunc, ffCPP_GravityIface);
	}
	else {
		{
			//
			//  Property name modification
			//
			temp_buf.Z();
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
				Format_C_Type(td.TerminalTypeID, t_stripped, arg_name, fctfInstance|fctfIfaceImpl, temp_buf);
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
				else if(st == S_WZSTRING) { // @v11.8.5
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
		Write_Func(gen, rFunc, ffCPP_CallImp, is_ret ? temp_buf.cptr() : 0);
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
					gen.CatIndent(line_buf.Z());
					if(st == S_DATE) {
						// @v10.8.7 temp_buf.Printf("ASSIGN_PTR(%s_, %s);", arg_name.cptr(), arg_name.cptr());
						temp_buf.Z().Cat("ASSIGN_PTR").CatChar('(').Cat(arg_name).CatChar('_').CatDiv(',', 2).Cat(arg_name).CatChar(')').Semicol(); // @v10.8.7 
						gen.WriteLine(line_buf.Cat(temp_buf).CR());
					}
					else if(st == S_TIME) {
					}
					else if(st == S_DATETIME) {
						// @v10.8.7 temp_buf.Printf("ASSIGN_PTR(%s_, %s);", arg_name.cptr(), arg_name.cptr());
						temp_buf.Z().Cat("ASSIGN_PTR").CatChar('(').Cat(arg_name).CatChar('_').CatDiv(',', 2).Cat(arg_name).CatChar(')').Semicol(); // @v10.8.7 
						gen.WriteLine(line_buf.Cat(temp_buf).CR());
					}
					else if(st == S_CHAR) {
					}
					else if(st == S_ZSTRING) {
						// @v10.8.7 temp_buf.Printf("%s.CopyToOleStr(", arg_name.cptr());
						temp_buf.Z().Cat(arg_name).Dot().Cat("CopyToOleStr").CatChar('('); // @v10.8.7 
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
			gen.CatIndent(line_buf.Z());
			THROW(UnrollType(rFunc.TypID, ret_te.T, &td));
			if(td.IsInterfaceTypeConversionNeeded() > 0) {
				const TYPEID st = GETSTYPE(td.T.Typ);
				STypEx t_stripped = td.T;
				if(oneof2(t_stripped.Mod, STypEx::modPtr, STypEx::modRef))
					t_stripped.Mod = 0;
				if(st == S_DATE) {
					gen.WriteLine(line_buf.Cat("ASSIGN_PTR(pRet_, ret.GetOleDate())").Semicol().CR());
				}
				else if(st == S_TIME) {
				}
				else if(st == S_DATETIME) {
					gen.WriteLine(line_buf.Cat("ASSIGN_PTR(pRet_, (OleDate)ret)").Semicol().CR());
				}
				else if(st == S_CHAR) {
				}
				else if(st == S_ZSTRING) {
					gen.WriteLine(line_buf.Cat("ret.CopyToOleStr(pRet_)").Semicol().CR());
				}
				else if(st == S_LSTRING) {
				}
				else if(st == S_NOTE) {
				}
			}
			else {
				gen.WriteLine(line_buf.Cat("ASSIGN_PTR(pRet, ret)").Semicol().CR());
			}
		}
		gen.WriteLine(gen.CatIndent(line_buf.Z()).Cat("IFACE_METHOD_EPILOG").Semicol().CR());
		gen.IndentDec();
		gen.Wr_CloseBrace(0, 0);
		gen.WriteBlancLine();
	}
	CATCHZOK
	return ok;
}

static SString & Make_USEIMPL_DefSymb(const char * pClsName, SString & rBuf)
{
	return (rBuf = "USE_IMPL_").Cat(pClsName);
}

int DlContext::Write_C_AutoImplFile(Generator_CPP & gen, const DlScope & rScope, StringSet & rSs, long cflags)
{
	int    ok = 1, r;
	uint   i, j, k;
	SString cls_name;
	SString fld_buf;
	SString func_name;
	SString vtab_name;
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
			for(j = 0; (r = EnumInterfacesByICls(p_ds, &j, &ifb, &p_ifs)) >= 0;) {
				if(r == 0) {
					fld_buf.Printf("Error extracting interface %s::%u", cls_name.cptr(), j-1);
					gen.WriteLine(fld_buf.CR());
				}
			}
			MakeClassName(p_ds, clsnfCPP, cflags, cls_name);

			gen.Wr_Comment(0);
			gen.Wr_Comment(fld_buf.Printf("Implementation of interface %s", cls_name.cptr()));
			gen.Wr_Comment(0);
			gen.Wr_IfDef(Make_USEIMPL_DefSymb(cls_name, fld_buf));
			rSs.add(fld_buf);
			//
			// Таблица виртуальных функция класса (собственно, та таблица, которая будет возвращаться //
			// клиенту при обращении к QueryInterface)
			//
			// ...
			(vtab_name = cls_name).CatChar('_').Cat("VTab");
			gen.Wr_StartClassDecl(Generator_CPP::clsStruct, vtab_name, 0, Generator_CPP::acsPublic, 0);
			gen.IndentInc();
			gen.Wr_Define("MFP(f)", fld_buf.Printf("(__stdcall %s::*f)", cls_name.cptr()));
			for(j = 0; EnumInterfacesByICls(p_ds, &j, &ifb, &p_ifs) > 0;) {
				//
				// Interface method's declarations
				//
				gen.Wr_Comment(0);
				gen.Wr_Comment((fld_buf = "Interface").Space().Cat(p_ifs->Name));
				gen.Wr_Comment(0);
				gen.CatIndent(func_name.Z()).Cat("IUNKN_METHOD_PTRS").CatChar('(').CatLongZ(j-1, 2).CatChar(')').Semicol().CR();
				gen.WriteLine(func_name);
				for(k = 0; p_ifs->EnumFunctions(&k, &func) > 0;) {
					(func.Name = "MFP").CatChar('(').CatChar('f').CatLongZ(k-1, 3).CatLongZ(j-1, 2).CatChar(')');
					Write_Func(gen, func, ffCPP_VTbl);
				}
			}
			//
			// ISupportErrorInfo {
			//
			gen.Wr_Comment(0);
			gen.Wr_Comment((fld_buf = "Interface").Space().Cat("ISupportErrorInfo"));
			gen.Wr_Comment(0);
			gen.CatIndent(func_name.Z()).Cat("IUNKN_METHOD_PTRS").CatParStr("SEI").Semicol().CR();
			gen.WriteLine(func_name);
			gen.CatIndent(func_name.Z()).Cat("ISUPERRINFO_METHOD_PTRS").CatParStr("SEI").Semicol().CR();
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
				gen.CatIndent(func_name.Z()).Cat("IUNKN_METHOD_PTRS_ASSIGN").CatChar('(').CatLongZ(j-1, 2).CatChar(')').Semicol().CR();
				gen.WriteLine(func_name);
				for(k = 0; p_ifs->EnumFunctions(&k, &func) > 0;) {
					gen.CatIndent(fld_buf.Z());
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
			gen.CatIndent(func_name.Z()).Cat("IUNKN_METHOD_PTRS_ASSIGN").CatParStr("SEI").Semicol().CR();
			gen.WriteLine(func_name);
			gen.CatIndent(func_name.Z()).Cat("ISUPERRINFO_METHOD_PTRS_ASSIGN").CatParStr("SEI").Semicol().CR();
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
					Write_C_ImplInterfaceFunc(gen, cls_name, func, cflags);
			}
			gen.Wr_EndIf(Make_USEIMPL_DefSymb(cls_name, fld_buf));
		}
		else
			THROW(Write_C_AutoImplFile(gen, *p_ds, rSs, cflags)); // @recursion
	}
	CATCHZOK
	return ok;
}

int DlContext::Write_WSDL_File(const char * pFileName, const DlScope & rScope)
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

int DlContext::Write_C_ImplFile(Generator_CPP & gen, const DlScope & rScope, long cflags)
{
	int    ok = 1;
	uint   j, k;
	SString cls_name, inst_name, fld_buf, func_name, vtab_name, mod_func_name;
	SString base_name("DlRtm");
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
			MakeClassName(p_ds, clsnfCPP, cflags, cls_name);
			(vtab_name = cls_name).CatChar('_').Cat("VTab");
			gen.WriteBlancLine();
			gen.CatIndent(fld_buf.Z()).Cat("DL6_IC_CONSTRUCTOR").CatChar('(').
				Cat(p_ds->Name).Comma().Space().Cat(vtab_name).CatChar(')').CR();
			gen.WriteLine(fld_buf);
			gen.Wr_OpenBrace();
			Wr_YourCodeHere(gen);
			gen.Wr_CloseBrace(0, 0);

			gen.WriteBlancLine();
			gen.CatIndent(fld_buf.Z()).Cat("DL6_IC_DESTRUCTOR").CatParStr(p_ds->Name).CR();
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
						fld_buf.Z();
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
					gen.WriteBlancLine();
				}
			}
			ok = 2;
		}
		else if(p_ds->IsKind(DlScope::kExpData) && !p_ds->GetBase()) {
			MakeClassName(p_ds, clsnfCPP, cflags, cls_name);
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
				gen.WriteBlancLine();
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
				gen.WriteBlancLine();
				/*int PPALDD_GoodsBillBase::InitData(PPFilt & rFilt, long rsrv)
				{
					return DlRtm::InitData(rFilt, rsrv);
				}*/
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
					gen.WriteBlancLine();
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
						gen.WriteLine(gen.CatIndent(fld_buf.Z()).Cat("IterProlog(iterId, 1)").Semicol().CR());
						gen.Wr_Return("-1");
					}
					gen.IndentDec();
					// } ...code
					gen.Wr_CloseBrace(0, 0);
					gen.WriteBlancLine();
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
						gen.WriteLine(gen.CatIndent(fld_buf.Z()).Cat("IterProlog(iterId, 0)").Semicol().CR());
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
					gen.WriteBlancLine();
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
					gen.WriteBlancLine();
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
			fld_buf.Z();
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
		else {
			THROW(ok = Write_C_ImplFile(gen, *p_ds, cflags)); // @recursion
		}
	}
	CATCHZOK
	return ok;
}
//
//
//
int DlContext::Write_Code()
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
int DlContext::FindImportFile(const char * pFileName, SString & rPath)
{
	SFsPath ps, ps_s;
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
		ps_s.Merge(&ps, SFsPath::fNam|SFsPath::fExt, rPath);
	}
	if(fileExists(rPath)) {
		return 1;
	}
	else {
		SetError(PPERR_DL6_IMPFILENFOUND, rPath);
		return 0;
	}
}

int DlContext::Test()
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

int DlContext::CreateDbDictionary(const char * pDictPath, const char * pDataPath, bool skipBtrDictCreation, const LongArray * pSqlServerTypeList/*SqlServerType sqlst*/)
{
	int    ok = 1;
	char   acs[512];
	SString msg_buf;
	LongArray scope_id_list;
	Sc.GetChildList(DlScope::kDbTable, 1, &scope_id_list);
	TSCollection <Generator_SQL> sqlgen_list;
	if(SVector::GetCount(pSqlServerTypeList)) {
		PPIDArray sqlserver_type_list(*pSqlServerTypeList);
		sqlserver_type_list.sortAndUndup();
		for(uint ssti = 0; ssti < sqlserver_type_list.getCount(); ssti++) {
			const long __sqlst = sqlserver_type_list.get(ssti);
			if(oneof4(__sqlst, sqlstGeneric, sqlstORA, sqlstMySQL, sqlstSQLite)) {
				Generator_SQL * p___sqlgen = new Generator_SQL(static_cast<SqlServerType>(__sqlst), Generator_SQL::fIndent);
				if(p___sqlgen) {
					sqlgen_list.insert(p___sqlgen);
				}
			}
		}
	}
	/*if(sqlst) {
		sql_file_name = InFileName;
		SFsPath::ReplaceExt(sql_file_name, "sql", 1);
		p_sqlgen = new Generator_SQL(sqlst, Generator_SQL::fIndent);
	}*/
	if(!skipBtrDictCreation) {
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
		if(!skipBtrDictCreation) {
			if(CurDict) {
				if(!CurDict->CreateTableSpec(&tbl)) {
					(msg_buf = tbl.GetTableName()).CatDiv(':', 1).CatEq("BtrError", static_cast<long>(BtrError));
					SetError(PPERR_DL6_DDFENTRYCRFAULT, msg_buf);
					CALLEXCEPT();
				}
				if(!(tbl.GetFlags() & XTF_TEMP) && !(tbl.GetFlags() & XTF_DICT)) {
					if(!CurDict->CreateDataFile(&tbl, 0, crmTTSReplace, GetRusNCaseACS(acs))) {
						(msg_buf = tbl.GetName()).CatDiv(':', 1).CatEq("BtrError", static_cast<long>(BtrError));
						SetError(PPERR_DL6_BTRFILECRFAULT, msg_buf);
						CALLEXCEPT();
					}
				}
			}
		}
		if(sqlgen_list.getCount()) {
			for(uint sgli = 0; sgli < sqlgen_list.getCount(); sgli++) {
				Generator_SQL * p_sqlgen = sqlgen_list.at(sgli);
				if(p_sqlgen) {
					p_sqlgen->CreateTable(tbl, tbl.GetTableName(), false, 1);
					p_sqlgen->Eos().Cr();
					uint j;
					for(j = 0; j < tbl.GetIndices().getNumKeys(); j++) {
						int   do_skip_index = 0;
						if(p_sqlgen->GetServerType() == sqlstMySQL && j == 0) {
							const BNFieldList & r_fl = tbl.GetFields();
							for(uint fi = 0; !do_skip_index && fi < r_fl.getCount(); fi++) {
								if(GETSTYPE(r_fl[fi].T) == S_AUTOINC)
									do_skip_index = 1;
							}
						}
						if(!do_skip_index) {
							p_sqlgen->CreateIndex(tbl, tbl.GetTableName(), j);
							p_sqlgen->Eos();
						}
					}
					p_sqlgen->Cr();
					if(p_sqlgen->GetServerType() == sqlstORA) {
						for(j = 0; j < tbl.GetFields().getCount(); j++) {
							TYPEID _t = tbl.GetFields()[j].T;
							if(GETSTYPE(_t) == S_AUTOINC) {
								p_sqlgen->CreateSequenceOnField(tbl, tbl.GetTableName(), j, 0);
								p_sqlgen->Eos();
							}
						}
						p_sqlgen->Cr();
					}
				}
			}
		}
		/*if(p_sqlgen) {
			p_sqlgen->CreateTable(tbl, tbl.GetTableName());
			p_sqlgen->Eos().Cr();
			uint j;
			for(j = 0; j < tbl.GetIndices().getNumKeys(); j++) {
				p_sqlgen->CreateIndex(tbl, tbl.GetTableName(), j);
				p_sqlgen->Eos();
			}
			p_sqlgen->Cr();
			if(p_sqlgen->GetServerType() == sqlstORA) {
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
					f_sql.WriteLine(static_cast<SString &>(*p_sqlgen));
				}
			}
		}*/
	}
	DBS.CloseDictionary();
	if(sqlgen_list.getCount()) {
		for(uint sgli = 0; sgli < sqlgen_list.getCount(); sgli++) {
			Generator_SQL * p_sqlgen = sqlgen_list.at(sgli);
			if(p_sqlgen) {
				SString __sql_file_name(InFileName);
				SFsPath ps(InFileName);
				ps.Ext = "sql";
				switch(p_sqlgen->GetServerType()) {
					case sqlstORA: ps.Nam.CatChar('-').Cat("oracle"); break;
					case sqlstMySQL: ps.Nam.CatChar('-').Cat("mysql"); break;
					case sqlstSQLite: ps.Nam.CatChar('-').Cat("sqlite"); break;
					case sqlstMSS: ps.Nam.CatChar('-').Cat("mssql"); break;
				}
				ps.Merge(__sql_file_name);
				SFile f_sql(__sql_file_name, SFile::mWrite);
				if(f_sql.IsValid()) {
					f_sql.WriteLine(static_cast<SString &>(*p_sqlgen));
				}
			}
		}
	}
	CATCHZOK
	//delete p_sqlgen;
	return ok;
}

#define DL600C_RELEASE_DEBUG

int DlContext::Compile(const char * pInFileName, const char * pDictPath, const char * pDataPath, long cflags)
{
	extern FILE * yyin;
	extern int    yyparse();
	int    ok = 1;
	SString file_name;
	SString line_buf;
	SString h_once_macro;
	SString temp_buf;
	{
		SFsPath::ReplaceExt(file_name = pInFileName, "ald", 0);
		Init(file_name);
	}
#ifdef DL600C_RELEASE_DEBUG
	//yydebug = 1; // @v11.0.4
	SFile  f_debug;
	{
		SFsPath::ReplaceExt(file_name = pInFileName, "debug", 1);
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
				SFsPath ps;
				ps.Split(CDeclFileName);
				h_once_macro.Z().CatCharN('_', 2).Cat(ps.Nam).CatChar('_').Cat(ps.Ext).ToUpper();
			}
			gen.Open(CDeclFileName);
			Write_C_FileHeader(gen, CDeclFileName);

			gen.Wr_IfDef(h_once_macro, 1);
			gen.Wr_Define(h_once_macro, 0);
			gen.WriteBlancLine();
			// @v12.2.9 {
			if(Sc.GetFirstChildByKind(DlScope::kUiView, 1)) {
				scope_id_list.freeAll();
				Sc.GetChildList(DlScope::kUiView, 1, &scope_id_list);				
				for(i = 0; i < scope_id_list.getCount(); i++) {
					const DlScope * p_scope = GetScope(scope_id_list.at(i), 0);
					if(p_scope) {
						temp_buf.Z().Cat(p_scope->GetId());
					}
				}
			}
			// } @v12.2.9 
			if(Sc.GetFirstChildByKind(DlScope::kUiDialog, 1)) {
				SString symb;
				DLSYMBID ss_id = 0;
				scope_id_list.freeAll();
				Sc.GetChildList(DlScope::kUiDialog, 1, &scope_id_list);
				gen.Wr_Comment(symb.Z());
				gen.Wr_Comment("Dialogs");
				gen.Wr_Comment(symb.Z());
				for(i = 0; i < scope_id_list.getCount(); i++) {
					const DlScope * p_scope = GetScope(scope_id_list.at(i), 0);
					if(p_scope) {
						temp_buf.Z().Cat(p_scope->GetId());
						//
						CtmExprConst c_ss;
						if(p_scope->GetConst(DlScope::cuifSymbSeries, &c_ss)) {
							char   s_buf[256];
							PTR32(s_buf)[0] = 0;
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
							gen.Wr_Comment(symb.Z());
							GetSymb(r_assc.Key, symb, '$');
							gen.Wr_Comment(symb);
							gen.Wr_Comment(symb.Z());
						}
						if(GetSymb(r_assc.Val, symb, '$')) {
							temp_buf.Z().Cat(r_assc.Val).Align(max_len - symb.Len(), ADJ_RIGHT);
							gen.Wr_Define(symb, temp_buf);
						}
						ss_id = r_assc.Key;
					}
				}
			}
			if(Sc.GetFirstChildByKind(DlScope::kDbTable, 1)) {
				gen.Wr_Include("db.h", 0);
				gen.Wr_Include("dl600.h", 0);
				gen.WriteBlancLine();
			}
			if(Sc.GetFirstChildByKind(DlScope::kExpData, 1)) {
				gen.Wr_Define("Head_data", "H");
				gen.Wr_Define("IT_default_data", "I");
				gen.WriteBlancLine();
			}
			//
			// Прототипы интерфейсов {
			//
			if(Sc.GetFirstChildByKind(DlScope::kInterface, 1) || Sc.GetFirstChildByKind(DlScope::kIClass, 1)) {
				if(cflags & cfGravity) {
					// @construction {
					/*
					class GravityClassImplementation_Math : public GravityClassImplementation {
					private:
						static bool _Callee_Func(gravity_vm * vm, GravityValue * args, uint16 nargs, uint32 rindex);
					};
					*/
					scope_id_list.clear();
					Sc.GetChildList(DlScope::kIClass, 1, &scope_id_list);
					for(i = 0; i < scope_id_list.getCount(); i++) {
						const DlScope * p_scope = GetScope(scope_id_list.at(i), 0);
						if(p_scope) {
							temp_buf.Z().Cat("GCI").CatChar('_').Cat(p_scope->Name);
							//int    Wr_StartDeclFunc(int funcKind, int funcMod, const char * pRetType, const char * pFuncName, int funcCallMod = 0);
							//int    Wr_EndDeclFunc(int semicol, int newLine);
							gen.Wr_StartClassDecl(Generator_CPP::clsClass, temp_buf, "GravityClassImplementation", Generator_CPP::acsPublic, 0/*declAlignment*/);
							gen.Wr_ClassAcsZone(Generator_CPP::acsPrivate);
							{
								DlScope::IfaceBase ifb;
								const DlScope * p_ifs;
								{
									for(uint j = 0; EnumInterfacesByICls(p_scope, &j, &ifb, &p_ifs) > 0;) {
										DlFunc func;
										gen.IndentInc();
										temp_buf.Z().Cat("Interface").Space().Cat(p_ifs->Name).Space().Cat("implementation");
										gen.Wr_Comment(0);
										gen.Wr_Comment(temp_buf);
										gen.Wr_Comment(0);
										for(uint k = 0; p_ifs->EnumFunctions(&k, &func) > 0;) {
											Write_Func(gen, func, ffH_GravityIface);
										}
										gen.IndentDec();
									}
								}
								{
									for(uint j = 0; EnumInterfacesByICls(p_scope, &j, &ifb, &p_ifs) > 0;) {
										DlFunc func;
										gen.IndentInc();
										temp_buf.Z().Cat("Interface").Space().Cat(p_ifs->Name).Space().Cat("implementation");
										gen.Wr_Comment(0);
										gen.Wr_Comment(temp_buf);
										gen.Wr_Comment(0);
										for(uint k = 0; p_ifs->EnumFunctions(&k, &func) > 0;) {
											Write_Func(gen, func, ffH_GravityImp);
										}
										gen.IndentDec();
									}
								}
							}
							gen.Wr_CloseBrace(1, 0);
							gen.WriteBlancLine();
						}
					}
					// } @construction
				}
				else {
					gen.Wr_ClassPrototype(Generator_CPP::clsStruct, "IUnknown");
					scope_id_list.clear();
					Sc.GetChildList(DlScope::kInterface, 1, &scope_id_list);
					for(i = 0; i < scope_id_list.getCount(); i++) {
						const DlScope * p_scope = GetScope(scope_id_list.at(i), 0);
						if(p_scope)
							gen.Wr_ClassPrototype(Generator_CPP::clsStruct, p_scope->Name);
					}
					gen.WriteBlancLine();
				}
			}
			//
			// }
			//
			if(cflags & cfDebug)
				gen.WriteLine(line_buf.Z().Cat("#pragma pack(show)").CR().CR());
			if(!Write_C_DeclFile(gen, Sc, cflags)) // Body of the header file
				Error(LastError, 0, 0);
			if(cflags & cfDebug)
				gen.WriteLine(line_buf.Z().CR().Cat("#pragma pack(show)").CR());
			gen.WriteBlancLine();
			gen.Wr_EndIf(h_once_macro);
			//
			// Writing CPP-files
			//
			{
				StringSet ss;
				SString impl_filename;
				SFsPath ps;
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
					if(!Write_C_AutoImplFile(gen, Sc, ss, cflags))
						Error(LastError, 0, 0);
				}
				{
					gen.Open(CImplFileName);
					Write_C_FileHeader(gen, CImplFileName);
					//
					for(uint ssp = 0; ss.get(&ssp, temp_buf);)
						gen.Wr_Define(temp_buf, 0);
					gen.WriteBlancLine();
					ps.Ext = "h";
					ps.Merge(0, SFsPath::fDrv|SFsPath::fDir, impl_filename);
					gen.Wr_Include(impl_filename, 1);
					gen.WriteBlancLine();
					//
					if(!Write_C_ImplFile(gen, Sc, cflags))
						Error(LastError, 0, 0);
				}
			}
			//
			// Writing IDL file
			//
			SFsPath::ReplaceExt(file_name = InFileName, "idl", 1);
			gen.Open(file_name);
			Write_C_FileHeader(gen, file_name);
			temp_buf = "unknwn.idl";
			line_buf.Z().Cat("import").Space().Cat(temp_buf.Quot('\"', '\"')).Semicol().CR();
			gen.WriteLine(line_buf);
			temp_buf = "oaidl.idl";
			line_buf.Z().Cat("import").Space().Cat(temp_buf.Quot('\"', '\"')).Semicol().CR();
			gen.WriteLine(line_buf);
			gen.WriteBlancLine();
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
			gen.WriteBlancLine();
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
				SFsPath::ReplaceExt(file_name, "wsdl", 1);
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
			LongArray sqlst_list;
			if(cflags & cfOracle)
				sqlst_list.add(sqlstORA);
			if(cflags & cfSQL)
				sqlst_list.add(sqlstGeneric);
			if(cflags & cfMySQL)
				sqlst_list.add(sqlstMySQL);
			if(cflags & cfSqLite)
				sqlst_list.add(sqlstSQLite);
			scope_id_list.freeAll();
			Sc.GetChildList(DlScope::kDbTable, 1, &scope_id_list);
			if(scope_id_list.getCount()) {
				if(!isempty(pDictPath)) {
					if(!CreateDbDictionary(pDictPath, pDataPath, LOGIC(cflags & cfSkipBtrDict), &sqlst_list))
						Error(LastError, 0, 0);
				}
				else
					Error(PPERR_DL6_UNDEFDICTPATH, 0, 0);
				// @v11.1.2 {
				if(cflags & cfStyloQAndroid) {
					file_name = InFileName;
					SFsPath::ReplaceExt(file_name, "java", 1);
					SFile f_sq_out(file_name, SFile::mWrite);
					temp_buf.Z().Cat("class StyloQDatabase").Space().CatChar('{').CR();
					f_sq_out.WriteLine(temp_buf);
					//
					for(uint i = 0; i < scope_id_list.getCount(); i++) {
						const long scope_id = scope_id_list.get(i);
						const DlScope * p_scope = GetScope(scope_id, 0);
						if(p_scope && p_scope->IsKind(DlScope::kDbTable)) {
							//public static class GoodsGroupTable extends DbTable {
							temp_buf.Z().Tab().Cat("public").Space().Cat("static").Space().Cat("class").Space().Cat(p_scope->Name).Space().Cat("extends").Space().Cat("DbTable").Space().CatChar('{');
							f_sq_out.WriteLine(temp_buf);
							//

							//
							temp_buf.Z().Tab().CatChar('}').CR();
							f_sq_out.WriteLine(temp_buf);
						}
					}
					//
					temp_buf.Z().CatChar('}').CR();
					f_sq_out.WriteLine(temp_buf);
				}
				// } @v11.1.2 
			}
		}
		if(cflags & cfDebug)
			Write_DebugListing();
	}
	else
		ok = Error(PPERR_DL6_INPUTOPENFAULT, InFileName, 0);
	return ok;
}
