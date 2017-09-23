// DLSCOPE.CPP
// Copyright (c) A.Sobolev 2007, 2008, 2009, 2010, 2011, 2015, 2016
//
#include <pp.h>
#pragma hdrstop
//
//
//
static long MakeSdRecFlags(uint kind)
{
	if(kind == DlScope::kEnum)
		return SdRecord::fEnum;
	else if(kind == DlScope::kTypedefPool)
		return SdRecord::fNoData;
	else
		return 0;
}

SLAPI DlScope::DlScope(DLSYMBID id, uint kind, const char * pName, int prototype) : SdRecord(MakeSdRecFlags(kind))
{
	ID = id;
	Name = pName;
	ScFlags  = 0;
	if(prototype)
		ScFlags |= sfPrototype;
	Kind     = kind;
	DvFlags  = 0;
	BaseId   = 0;
	ParentId = 0;
	Version  = 0;
	P_Parent = 0;
	P_Base   = 0;
	P_IfaceBaseList = 0;
	P_DbIdxSegFlags = 0;
	FixDataBuf.Init();
#ifdef DL600C // {
	LastLocalId = 0;
#endif
}

SLAPI DlScope::DlScope(const DlScope & s) :
	SdRecord() // Это - не copy-constructor так как функция копирования сделает
		// работу, которую должен был выполнить copy-constructor базового класса
{
	P_Parent = 0;
	P_Base   = 0;
	P_IfaceBaseList = 0;
	P_DbIdxSegFlags = 0;
	FixDataBuf.Init();
	Copy(s, 0);
}

SLAPI DlScope::~DlScope()
{
	delete P_IfaceBaseList;
	delete P_DbIdxSegFlags;
}

DlScope & FASTCALL DlScope::operator = (const DlScope & s)
{
	Copy(s, 0);
	return *this;
}

int SLAPI DlScope::Copy(const DlScope & s, int withoutChilds)
{
	SdRecord::Copy(s);
	Kind     = s.Kind;
	ScFlags  = s.ScFlags;
	DvFlags  = s.DvFlags;
	BaseId   = s.BaseId;
	ParentId = s.ParentId;
	Version  = s.Version;
	//
	ZDELETE(P_IfaceBaseList);
	if(s.P_IfaceBaseList)
		P_IfaceBaseList = new SArray(*s.P_IfaceBaseList);
	//
	CList = s.CList;
	//
	ZDELETE(P_DbIdxSegFlags);
	if(s.P_DbIdxSegFlags)
		P_DbIdxSegFlags = new LongArray(*s.P_DbIdxSegFlags);
	//
	ChildList.freeAll();
	if(!withoutChilds) {
		uint c = s.ChildList.getCount();
		if(c) do {
			Add(new DlScope(*s.ChildList.at(--c)));
		} while(c);
	}
	return 1;
}

int FASTCALL DlScope::IsEqual(const DlScope & rPat) const
{
	int    ok = 1;
	uint   c = 0;
	THROW(SdRecord::IsEqual(rPat));
	THROW(Kind == rPat.Kind);
	THROW(ScFlags == rPat.ScFlags);
	THROW(DvFlags == rPat.DvFlags);
	THROW(BaseId  == rPat.BaseId);
	THROW(ParentId == rPat.ParentId);
	THROW(Version  == rPat.Version);
	//
	if(P_IfaceBaseList && rPat.P_IfaceBaseList) {
		THROW(P_IfaceBaseList->getCount() == rPat.P_IfaceBaseList->getCount());
		c = P_IfaceBaseList->getCount();
		if(c) do {
			--c;
			THROW(memcmp(P_IfaceBaseList->at(c), rPat.P_IfaceBaseList->at(c), sizeof(IfaceBase)) == 0);
		} while(c);
	}
	else {
		THROW(P_IfaceBaseList == 0 && rPat.P_IfaceBaseList == 0);
	}
	//
	THROW(CList.IsEqual(rPat.CList));
	//
	if(P_DbIdxSegFlags && rPat.P_DbIdxSegFlags) {
		THROW(P_DbIdxSegFlags->IsEqual(rPat.P_DbIdxSegFlags));
	}
	else {
		THROW(P_DbIdxSegFlags == 0 && rPat.P_DbIdxSegFlags == 0);
	}
	//
	THROW(FuncPool.IsEqual(rPat.FuncPool));
	c = ChildList.getCount();
	THROW(c == rPat.ChildList.getCount());
	if(c) do {
		--c;
		THROW(ChildList.at(c)->IsEqual(*rPat.ChildList.at(c))); // @recursion
	} while(c);
	CATCHZOK
	return ok;
}

int SLAPI DlScope::Write(SBuffer & rBuf) const
{
	int    ok = 1;
	uint32 c = 0;
	THROW(SdRecord::Write_(rBuf));
	THROW(rBuf.Write(&Kind, sizeof(Kind)));
	THROW(rBuf.Write(&ScFlags, sizeof(ScFlags)));
	THROW(rBuf.Write(&DvFlags, sizeof(DvFlags)));
	THROW(rBuf.Write(&BaseId,  sizeof(BaseId)));
	THROW(rBuf.Write(&ParentId, sizeof(ParentId)));
	THROW(rBuf.Write(&Version, sizeof(Version)));
	THROW(rBuf.Write(P_IfaceBaseList, 0));
	THROW(rBuf.Write(&CList, 0));
	THROW(rBuf.Write(&CfList, 0));
	THROW(rBuf.Write(P_DbIdxSegFlags, 0));
	THROW(FuncPool.Write(rBuf));
	c = ChildList.getCount();
	rBuf.Write(&c, sizeof(c));
	for(uint i = 0; i < c; i++) {
		THROW(ChildList.at(i)->Write(rBuf));
	}
	CATCHZOK
	return ok;
}

int SLAPI DlScope::Read(SBuffer & rBuf)
{
	int    ok = 1;
	uint32 c;
	THROW(SdRecord::Read_(rBuf));
	THROW(rBuf.Read(&Kind, sizeof(Kind)));
	THROW(rBuf.Read(&ScFlags, sizeof(ScFlags)));
	THROW(rBuf.Read(&DvFlags, sizeof(DvFlags)));
	THROW(rBuf.Read(&BaseId, sizeof(BaseId)));
	THROW(rBuf.Read(&ParentId, sizeof(ParentId)));
	THROW(rBuf.Read(&Version, sizeof(Version)));
	{
		SArray temp_list(sizeof(IfaceBase));
		THROW(rBuf.Read(&temp_list, 0));
		ZDELETE(P_IfaceBaseList);
		if(temp_list.getCount())
			P_IfaceBaseList = new SArray(temp_list);
	}
	THROW(rBuf.Read(&CList, 0));
	THROW(rBuf.Read(&CfList, 0));
	{
		LongArray temp_list;
		THROW(rBuf.Read(&temp_list, 0));
		ZDELETE(P_DbIdxSegFlags);
		if(temp_list.getCount())
			P_DbIdxSegFlags = new LongArray(temp_list);
	}
	THROW(FuncPool.Read(rBuf));
	THROW(rBuf.Read(&c, sizeof(c)));
	ChildList.freeAll();
	for(uint i = 0; i < c; i++) {
		DlScope * p_scope = new DlScope(0, 0, 0, 0);
		THROW(p_scope->Read(rBuf));
		THROW(Add(p_scope));
	}
	CATCHZOK
	return ok;
}

void SLAPI DlScope::SetFixDataBuf(void * pBuf, size_t size, int clear)
{
	FixDataBuf.P_Buf = (char *)pBuf;
	FixDataBuf.Size = size;
	if(clear)
		FixDataBuf.Zero();
}

void * FASTCALL DlScope::GetFixDataPtr(size_t offs) const
{
	return (offs < FixDataBuf.Size) ? (FixDataBuf.P_Buf+offs) : 0;
}

int FASTCALL DlScope::Add(DlScope * pChild)
{
	if(pChild) {
		pChild->ParentId = ID;
		pChild->P_Parent = this;
		return BIN(ChildList.insert(pChild));
	}
	else
		return 0;
}

int SLAPI DlScope::Remove(DLSYMBID scopeID)
{
	int    ok = 0;
	uint   c = ChildList.getCount();
	if(c)
		do {
			if(ChildList.at(--c)->ID == scopeID) {
				ChildList.atFree(c);
				ok++;
			}
		} while(c);
	return ok;
}

DLSYMBID SLAPI DlScope::GetId() const
{
	return (DLSYMBID)ID;
}

DLSYMBID SLAPI DlScope::GetBaseId() const
{
	return BaseId;
}

const SString & SLAPI DlScope::GetName() const
{
	return Name;
}

uint SLAPI SLAPI DlScope::GetKind() const
{
	return Kind;
}

uint32 SLAPI DlScope::GetVersion() const
{
	return Version;
}

int SLAPI DlScope::CheckDvFlag(long f) const
{
	return BIN(DvFlags & f);
}

int FASTCALL DlScope::IsKind(const uint kind) const
{
	return BIN(Kind == kind);
}

const DlScope * SLAPI DlScope::GetOwner() const
{
	return P_Parent;
}

const DlScopeList & SLAPI DlScope::GetChildList() const
{
	return ChildList;
}

const DlScope * SLAPI DlScope::GetFirstChildByKind(int kind, int recursive) const
{
	for(uint i = 0; i < ChildList.getCount(); i++) {
		const DlScope * p_child = ChildList.at(i);
		if(p_child->Kind == kind)
			return p_child;
		else if(recursive) {
			p_child = p_child->GetFirstChildByKind(kind, recursive); // @recursion
			if(p_child)
				return p_child;
		}
	}
	return 0;
}

int SLAPI DlScope::GetChildList(int kind, int recursive, IntArray * pList) const
{
	const DlScopeList & r_list = GetChildList();
	for(uint i = 0; i < r_list.getCount(); i++) {
		const DlScope * p_scope = r_list.at(i);
		if(p_scope->IsKind(kind))
			pList->insert(&p_scope->ID);
		if(recursive)
			p_scope->GetChildList(kind, recursive, pList); // @recursion
	}
	return 1;
}

int SLAPI DlScope::IsChildOf(const DlScope * pOwner) const
{
	if(pOwner) {
		uint   i = pOwner->ChildList.getCount();
		if(i) do {
			if(this == pOwner->ChildList.at(--i))
				return 1;
		} while(i);
	}
	return 0;
}

int SLAPI DlScope::EnumChilds(uint * pIdx, DlScope ** ppScope) const
{
	int    ok = 1;
	DlScope * p_scope = 0;
	if(*pIdx < ChildList.getCount())
		p_scope = ChildList.at((*pIdx)++);
	else
		ok = 0;
	ASSIGN_PTR(ppScope, p_scope);
	return ok;
}

int SLAPI DlScope::EnumInheritance(uint * pIdx, const DlScope ** ppScope) const
{
	uint   c = 0;
	const DlScope * p_base = this;
	while(p_base) {
		c++;
		p_base = p_base->P_Base;
	}
	if(*pIdx < c)
		for(p_base = this; p_base; p_base = p_base->P_Base)
			if(--c == *pIdx) {
				*ppScope = p_base;
				(*pIdx)++;
				return 1;
			}
	*ppScope = 0;
	return 0;
}

int SLAPI DlScope::SetupTitle(uint kind, const char * pName)
{
	Kind = kind;
	Name = pName;
	return 1;
}

int SLAPI DlScope::GetQualif(DLSYMBID id, const char * pDiv, int inverse, SString & rBuf) const
{
	int    ok = 0;
	if(ID == id) {
		if(Kind != DlScope::kFile && Kind != DlScope::kGlobal)
			rBuf.Cat(Name);
		ok = 1;
	}
	else {
		rBuf.Z();
		uint c = ChildList.getCount();
		SString temp_buf;
		if(c) do {
			const DlScope * p_parent = ChildList.at(--c);
			temp_buf.Z();
			if(p_parent->GetQualif(id, pDiv, inverse, temp_buf)) {
				if(inverse)
					rBuf.Cat(temp_buf);
				if(Kind != DlScope::kFile && Kind != DlScope::kGlobal) {
					if(inverse && rBuf.NotEmpty())
						rBuf.Cat(pDiv);
					rBuf.Cat(Name);
				}
				if(!inverse) {
					if(rBuf.NotEmpty())
						rBuf.Cat(pDiv);
			 		rBuf.Cat(temp_buf);
				}
				ok = 1;
			}
		} while(c && !ok);
	}
	return ok;
}

int SLAPI DlScope::IsPrototype() const
{
	return BIN(ScFlags & sfPrototype);
}

int SLAPI DlScope::ResetPrototypeFlag()
{
	ScFlags &= ~sfPrototype;
	return 1;
}

int SLAPI DlScope::SetRecord(const DlScope * pRec)
{
	int    ok = 1, found = 0;
	if(pRec) {
		ResetPrototypeFlag();
		for(uint i = 0; !found && i < ChildList.getCount(); i++) {
			DlScope * p_this_rec = ChildList.at(i);
			if(p_this_rec->GetName().Cmp(pRec->GetName(), 0) == 0) {
				for(uint j = 0; j < pRec->GetCount(); j++) {
					uint fld_id = 0;
					SdbField fld;
					pRec->GetFieldByPos(j, &fld);
					p_this_rec->AddField(&fld_id, &fld);
				}
				found = 1;
			}
		}
		if(!found) {
			Add(new DlScope(*pRec));
		}
	}
	else
		ok = -1;
	return ok;
}

int SLAPI DlScope::SetRecList(const DlScopeList * pList)
{
	int    ok = 1;
	if(pList) {
		ResetPrototypeFlag();
		for(uint i = 0; ok && i < pList->getCount(); i++)
			if(!SetRecord(pList->at(i)))
				ok = 0;
	}
	else
		ok = -1;
	return ok;
}

const DlScope * SLAPI DlScope::SearchByName_Const(uint kind, const char * pName, DLSYMBID * pParentID) const
{
	DLSYMBID parent_id = 0;
	const DlScope * p_scope = 0;
	if(pName && pName[0]) {
		if((!kind || Kind == kind) && Name.Cmp(pName, 0) == 0) {
			p_scope = this;
		}
		else {
			uint   c = ChildList.getCount();
			if(c) do {
				DlScope * p_parent = ChildList.at(--c);
				p_scope = p_parent->SearchByName(kind, pName, &parent_id); // @resursion
				if(p_scope) {
					SETIFZ(parent_id, p_parent->ID);
					break;
				}
			} while(c);
		}
	}
	ASSIGN_PTR(pParentID, parent_id);
	return p_scope;
}

DlScope * SLAPI DlScope::SearchByName(uint kind, const char * pName, DLSYMBID * pParentID)
{
	return (DlScope *)SearchByName_Const(kind, pName, pParentID);
	/*
	DLSYMBID parent_id = 0;
	DlScope * p_scope = 0;
	if(pName && pName[0]) {
		if((!kind || Kind == kind) && Name.Cmp(pName, 0) == 0) {
			p_scope = this;
		}
		else {
			uint   c = ChildList.getCount();
			if(c) do {
				DlScope * p_parent = ChildList.at(--c);
				p_scope = p_parent->SearchByName(kind, pName, &parent_id); // @resursion
				if(p_scope) {
					SETIFZ(parent_id, p_parent->ID);
					break;
				}
			} while(c);
		}
	}
	ASSIGN_PTR(pParentID, parent_id);
	return p_scope;
	*/
}

DlScope * SLAPI DlScope::SearchByID(DLSYMBID id, DLSYMBID * pParentID)
{
	PROFILE_START
	DLSYMBID parent_id = 0;
	DlScope * p_scope = 0;
	if(ID == id)
		p_scope = this;
	else {
		uint c = ChildList.getCount();
		if(c) do {
			DlScope * p_parent = ChildList.at(--c);
			p_scope = p_parent->SearchByID(id, &parent_id); // @resursion
			if(p_scope) {
				SETIFZ(parent_id, this->ID);
				break;
			}
		} while(c);
	}
	ASSIGN_PTR(pParentID, parent_id);
	return p_scope;
	PROFILE_END
}

const DlScope * SLAPI DlScope::SearchByID_Const(DLSYMBID id, DLSYMBID * pParentID) const
{
	DLSYMBID parent_id = 0;
	const DlScope * p_scope = 0;
	if(ID == id)
		p_scope = this;
	else {
		uint   c = ChildList.getCount();
		if(c) do {
			const  DlScope * p_parent = ChildList.at(--c);
			p_scope = p_parent->SearchByID_Const(id, &parent_id); // @resursion
			if(p_scope) {
				SETIFZ(parent_id, this->ID);
				break;
			}
		} while(c);
	}
	ASSIGN_PTR(pParentID, parent_id);
	return p_scope;
}

const DlScope * SLAPI DlScope::GetBase() const
{
	return P_Base;
}

DLSYMBID SLAPI DlScope::EnterScope(DLSYMBID parentId, DLSYMBID newScopeID, uint kind, const char * pName)
{
	DLSYMBID scope_id = 0;
	if(parentId == 0) {
		DLSYMBID pid;
		if(SearchByID(newScopeID, &pid))
			scope_id = newScopeID;
		else {
			DLSYMBID parent_id = 0;
			DlScope * p_scope = SearchByName(kind, pName, &parent_id);
			if(p_scope && p_scope->ParentId == ID)
				scope_id = p_scope->ID;
			else {
				p_scope = new DlScope(newScopeID, kind, pName, 0);
				Add(p_scope);
				scope_id = newScopeID;
			}
		}
	}
	else {
		DlScope * p_parent = SearchByID(parentId, 0);
		if(p_parent)
			scope_id = p_parent->EnterScope(0, newScopeID, kind, pName); // @recursion
	}
	return scope_id;
}

int SLAPI DlScope::LeaveScope(DLSYMBID scopeID, DLSYMBID * pParentID)
{
	DlScope * p_scope = SearchByID(scopeID, pParentID);
	return p_scope ? 1 : 0;
}

int FASTCALL DlScope::InitInheritance(const DlScope * pTopScope)
{
	int    ok = 1;
	const  uint c = ChildList.getCount();
	for(uint i = 0; i < c; i++) {
		DlScope * p_child = ChildList.at(i);
		if(p_child)
			THROW(p_child->InitInheritance(pTopScope)); // @recursion
	}
	if(BaseId) {
		const DlScope * p_base = pTopScope->SearchByID_Const(BaseId, 0);
		THROW(p_base);
		P_Base = p_base;
	}
	CATCHZOK
	return ok;
}

#ifdef DL600C // {

int SLAPI DlScope::SetInheritance(const DlScope * pBase, DlContext * pCtx)
{
	int    ok = 1;
	ResetPrototypeFlag();
	if(pBase) {
		BaseId = pBase->GetId();
		P_Base = pBase;
		const DlScopeList & r_list = pBase->GetChildList();
		for(uint i = 0; i < r_list.getCount(); i++) {
			const DlScope * p_base_child = r_list.at(i);
			if(oneof2(p_base_child->Kind, kExpDataHdr, kExpDataIter)) {
				DlScope * p_child = 0;
				DLSYMBID new_scope_id = EnterScope(ID, pCtx->GetNewSymbID(), p_base_child->Kind, p_base_child->Name);
				THROW(new_scope_id);
				p_child = SearchByID(new_scope_id, 0);
				THROW(p_child->SetInheritance(p_base_child, pCtx)); // @recursion
				THROW(LeaveScope(new_scope_id, 0));
			}
		}
	}
	else {
		BaseId = 0;
		pBase  = 0;
	}
	CATCHZOK
	return ok;
}

#endif // } DL600C

int SLAPI DlScope::AddFunc(const DlFunc * pF)
{
	return FuncPool.Add(pF);
}

int SLAPI DlScope::GetFuncListByName(const char * pSymb, IntArray * pList) const
{
	int    ok = -1;
	for(uint pos = 0; FuncPool.EnumByName(pSymb, &pos, 0) > 0; pos++) {
		pList->insert(&pos);
		ok = 1;
	}
	return ok;
}

uint SLAPI DlScope::GetFuncCount() const
{
	return FuncPool.GetCount();
}

int SLAPI DlScope::GetFuncByPos(uint pos, DlFunc * pFunc) const
{
	return FuncPool.GetByPos(pos, pFunc);
}

int SLAPI DlScope::EnumFunctions(uint * pI, DlFunc * pFunc) const
{
	int    ok = -1;
	uint   i = *pI;
	if(i < FuncPool.GetCount()) {
		ok = FuncPool.GetByPos(i, pFunc);
		*pI = ++i;
	}
	return ok;
}

int SLAPI DlScope::AddIfaceBase(const IfaceBase * pEntry)
{
	if(!P_IfaceBaseList)
		P_IfaceBaseList = new SArray(sizeof(IfaceBase));
#ifdef DL600C
	return BIN(P_IfaceBaseList && P_IfaceBaseList->insert(pEntry));
#else
	if(!P_IfaceBaseList)
		return PPSetErrorNoMem();
	else
		return P_IfaceBaseList->insert(pEntry) ? 1 : PPSetErrorSLib();
#endif
}

uint SLAPI DlScope::GetIfaceBaseCount() const
{
	return P_IfaceBaseList ? P_IfaceBaseList->getCount() : 0;
}

int SLAPI DlScope::GetIfaceBase(uint pos, IfaceBase * pEntry) const
{
	int    ok = 1;
	if(pos < GetIfaceBaseCount()) {
		ASSIGN_PTR(pEntry, *(IfaceBase *)P_IfaceBaseList->at(pos));
	}
	else
		ok = 0;
	return ok;
}

int FASTCALL DlScope::SetAttrib(const Attr & rAttr)
{
	int    ok = 1;
	// @v5.7.1 if(oneof4(rAttr.A, sfHidden, sfRestricted, sfVersion, sfNoIDL)) {
		ScFlags |= rAttr.A;
		if(rAttr.A == sfVersion)
			Version = rAttr.Ver;
	/* @v5.7.1
		ok = 1;
	}
	else {
		ok = 0;
	}
	*/
	return ok;
}

int SLAPI DlScope::GetAttrib(uint attrFlag /* DlScope::sfXXX */, Attr * pAttr) const
{
	int    ok = 0;
	if(ScFlags & attrFlag) {
		Attr attr;
		attr.A = attrFlag;
		if(attrFlag == sfVersion)
			attr.Ver = Version;
		else
			attr.Ver = 0;
		ASSIGN_PTR(pAttr, attr);
		ok = 1;
	}
	return ok;
}

int SLAPI DlScope::AddConst(COption id, const CtmExprConst & rConst, int replace)
{
	int    ok = 1;
	uint   pos = 0;
	if(CList.lsearch(&id, &pos, CMPF_LONG)) {
		if(replace)
			CList.at(pos).C = rConst;
		else
			ok = -1;
	}
	else {
		CItem item;
		item.I = id;
		item.C = rConst;
		if(!CList.insert(&item))
			ok = 0;
	}
	return ok;
}

CtmExprConst FASTCALL DlScope::GetConst(COption id) const
{
	uint   pos = 0;
	CtmExprConst c;
	return CList.lsearch(&id, &pos, CMPF_LONG) ?  CList.at(pos).C : c.Init();
}

int SLAPI DlScope::GetConst(COption id, CtmExprConst * pConst) const
{
	int    ok  = 0;
	uint   pos = 0;
	if(CList.lsearch(&id, &pos, CMPF_LONG)) {
		ASSIGN_PTR(pConst, CList.at(pos).C);
		ok = 1;
	}
	return ok;
}

int SLAPI DlScope::AddFldConst(uint fldID, COption id, const CtmExprConst & rConst, int replace)
{
	int    ok = 1;
	uint   pos = 0;
	CfItem key;
	key.FldID = fldID;
	key.I = id;
	if(CfList.lsearch(&key, &pos, PTR_CMPFUNC(_2long))) {
		if(replace)
			CfList.at(pos).C = rConst;
		else
			ok = -1;
	}
	else {
		CfItem item;
		item.FldID = fldID;
		item.I = id;
		item.C = rConst;
		if(!CfList.insert(&item))
			ok = 0;
	}
	return ok;
}

CtmExprConst SLAPI DlScope::GetFldConst(uint fldID, COption id) const
{
	uint   pos = 0;
	CtmExprConst c;
	CfItem key;
	key.FldID = fldID;
	key.I = id;
	return CfList.lsearch(&key, &pos, PTR_CMPFUNC(_2long)) ? CfList.at(pos).C : c.Init();
}

int SLAPI DlScope::GetFldConst(uint fldID, COption id, CtmExprConst * pConst) const
{
	int    ok  = 0;
	uint   pos = 0;
	CfItem key;
	key.FldID = fldID;
	key.I = id;
	if(CfList.lsearch(&key, &pos, PTR_CMPFUNC(_2long))) {
		ASSIGN_PTR(pConst, CfList.at(pos).C);
		ok = 1;
	}
	return ok;
}

struct DlScopePropIdAssoc {
	int    Id;
	const char * P_Text;
};

static DlScopePropIdAssoc DlScopePropIdAssocList[] = {
	{ DlScope::cuifLabelRect,  "labelrect" },
	{ DlScope::cuifReadOnly,   "readonly" },
	{ DlScope::cuifDisabled,   "disabled" },
	{ DlScope::cuifAlignment,  "alignment" },
	{ DlScope::cuifHidden,     "hidden" },
	{ DlScope::cuifFont,       "font" },
	{ DlScope::cuifStaticEdge, "staticedge" }

};

//static
int SLAPI DlScope::GetPropSymb(int propId, SString & rSymb)
{
	int    ok = 0;
	rSymb = 0;
	for(uint i = 0; !ok && i < SIZEOFARRAY(DlScopePropIdAssocList); i++)
		if(DlScopePropIdAssocList[i].Id == propId) {
			rSymb = DlScopePropIdAssocList[i].P_Text;
			ok = 1;
		}
	return ok;
}

//static
int SLAPI DlScope::ResolvePropName(const char * pName)
{
	int    id = 0;
	for(uint i = 0; !id && i < SIZEOFARRAY(DlScopePropIdAssocList); i++)
		if(strcmp(DlScopePropIdAssocList[i].P_Text, pName) == 0)
			id = DlScopePropIdAssocList[i].Id;
	// ...
	//PPSetAddedMsgString(pName);
	THROW_PP(id, PPERR_DL6_INVPROPSYMB);
	CATCH
		id = 0;
	ENDCATCH
	return id;
}

//static
SString & DlScope::PropListToLine(const StrAssocArray & rPropList, uint tabCount, SString & rBuf)
{
	if(rPropList.getCount()) {
		SString prop_symb, prop_val;
		rBuf.CR().Tab(tabCount).Cat("property").Space().CatChar('{').CR();
		for(uint i = 0; i < rPropList.getCount(); i++) {
			StrAssocArray::Item prop = rPropList.at(i);
			if(DlScope::GetPropSymb(prop.Id, prop_symb)) {
				prop_val = prop.Txt;
				rBuf.Tab(tabCount+1).Cat(prop_symb);
				if(prop_val.NotEmptyS())
					rBuf.Eq().Cat(prop_val);
				rBuf.CR();
			}
		}
		rBuf.Tab(tabCount).CatChar('}');
	}
	return rBuf;
}

#ifdef DL600C // {

int SLAPI DlScope::AddTempFldConst(COption id, const CtmExprConst & rConst)
{
	int    ok = 1;
	uint   pos = 0;
	CfItem key;
	key.FldID = 0;
	key.I = id;
	if(TempCfList.lsearch(&key, &pos, PTR_CMPFUNC(_2long))) {
		TempCfList.at(pos).C = rConst;
	}
	else {
		CfItem item;
		item.FldID = 0;
		item.I = id;
		item.C = rConst;
		if(!TempCfList.insert(&item))
			ok = 0;
	}
	return ok;
}

int SLAPI DlScope::AcceptTempFldConstList(uint fldID)
{
	if(fldID == 0) {
		//
		// Специальный случай: свойства диалога. Временные константы "свалены" в родительскую
		// область. Нам надо их от туда забрать и обработать как свои (см. описание синтаксиса в DL600C.Y).
		//
		if(P_Parent) {
			TempCfList = P_Parent->TempCfList;
			((DlScope *)P_Parent)->TempCfList.clear(); // @badcast
		}
	}
	for(uint i = 0; i < TempCfList.getCount(); i++) {
		CfItem item;
		item = TempCfList.at(i);
		if(fldID) {
			item.FldID = fldID;
			CfList.insert(&item);
		}
		else {
			AddConst((COption)item.I, item.C, 1);
		}
	}
	TempCfList.clear();
	return 1;
}

#endif

int SLAPI DlScope::AddDbIndexSegment(const char * pFieldName, long options)
{
	int    ok = 1;
	uint   fld_id = 0;
	SdbField fld;
	const DlScope * p_sc_table = 0;
	THROW(p_sc_table = GetOwner()); // @err
	THROW_PP(p_sc_table->GetFieldByName(pFieldName, &fld), PPERR_DL6_IDXSEGINVFLDNAME);
	THROW_MEM(SETIFZ(P_DbIdxSegFlags, new LongArray));
	fld_id = fld.ID;
	THROW(AddField(&fld_id, &fld));
	P_DbIdxSegFlags->add(options);
	CATCHZOK
	return ok;
}

long SLAPI DlScope::GetDbIndexSegOptions(uint pos) const
{
	return (P_DbIdxSegFlags && pos < P_DbIdxSegFlags->getCount()) ? P_DbIdxSegFlags->get(pos) : 0;
}

