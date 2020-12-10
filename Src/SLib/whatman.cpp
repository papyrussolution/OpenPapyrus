// WHATMAN.CPP
// Copyright (c) A.Sobolev 2010, 2011, 2015, 2016, 2017, 2018, 2019, 2020
// @codepage UTF-8
//
#include <slib-internal.h>
#pragma hdrstop
//
//
//
TWhatmanObject::TextParam::TextParam()
{
	SetDefault();
}

void TWhatmanObject::TextParam::SetDefault()
{
	Side = SIDE_BOTTOM;
	Flags = 0;
	AlongSize = -1.0f;
	AcrossSize = -0.5f;
	CStyleIdent = 0;
	ParaIdent = 0;
}

class WhatmanObjectRegTable : private SVector { // @v9.8.5 SArray-->SVector
public:
	struct Item {
		long   Id;
		FN_WTMOBJ_FACTORY Factory;
		const char * P_Symb;
		const char * P_Name;
	};
	WhatmanObjectRegTable() : SVector(sizeof(Entry)), LastIndex(0)
	{
		Pool.add("$"); // zero index - is empty string
	}
	int    Add(const char * pSymb, const char * pName, FN_WTMOBJ_FACTORY factory);
	int    Get(uint pos, Item & rItem) const;
	int    Search(const char * pSymb, uint * pPos, Item * pItem) const;
	int    Search(long id, uint * pPos, Item * pItem) const;
	StrAssocArray * MakeStrAssocList() const;
private:
	struct Entry {
		long   Id;
		uint   SymbPos;
		uint   NamePos;
		FN_WTMOBJ_FACTORY Factory;
	};
	long   LastIndex;
	StringSet Pool;
};

int WhatmanObjectRegTable::Search(const char * pSymb, uint * pPos, Item * pItem) const
{
	int    ok = 0;
	uint   pos = 0, assc_pos;
	while(!ok && Pool.search(pSymb, &pos, 0)) {
		if(lsearch(&pos, &(assc_pos = 0), CMPF_LONG, offsetof(Entry, SymbPos))) {
			ASSIGN_PTR(pPos, assc_pos);
			ok = pItem ? Get(assc_pos, *pItem) : 1;
		}
	}
	return ok;
}

int WhatmanObjectRegTable::Search(long id, uint * pPos, Item * pItem) const
{
	int    ok = 0;
	uint   pos = 0;
	if(lsearch(&id, &pos, CMPF_LONG)) {
		ASSIGN_PTR(pPos, pos);
		ok = pItem ? Get(pos, *pItem) : 1;
	}
	return ok;
}

int WhatmanObjectRegTable::Get(uint pos, Item & rItem) const
{
	int    ok = 1;
	if(pos < getCount()) {
		const Entry * p_entry = static_cast<const Entry *>(at(pos));
		memzero(&rItem, sizeof(rItem));
		rItem.Id = p_entry->Id;
		rItem.Factory = p_entry->Factory;
		if(p_entry->SymbPos && p_entry->SymbPos < Pool.getDataLen())
			rItem.P_Symb = Pool.getBuf() + p_entry->SymbPos;
		if(p_entry->NamePos && p_entry->NamePos < Pool.getDataLen())
			rItem.P_Name = Pool.getBuf() + p_entry->NamePos;
	}
	else
		ok = 0;
	return ok;
}

int WhatmanObjectRegTable::Add(const char * pSymb, const char * pName, FN_WTMOBJ_FACTORY factory)
{
	int    ok = 1;
	if(isempty(pSymb) || !factory) {
		ok = 0;
	}
	else if(Search(pSymb, 0, 0)) {
		ok = -1;
	}
	else {
		Entry entry;
		entry.Id = ++LastIndex;
		Pool.add(pSymb, &entry.SymbPos);
		Pool.add(pName, &entry.NamePos);
		entry.Factory = factory;
		THROW(insert(&entry));
	}
	CATCHZOK
	return ok;
}

StrAssocArray * WhatmanObjectRegTable::MakeStrAssocList() const
{
	StrAssocArray * p_list = new StrAssocArray;
	SString name_buf;
	for(uint i = 0; i < getCount(); i++) {
		Item item;
		if(Get(i, item)) {
			const char * p_name = item.P_Name;
			if(p_name && p_name[0] == '@' && SLS.LoadString_(p_name+1, name_buf))
				p_name = name_buf;
			p_list->Add(item.Id, NZOR(p_name, item.P_Symb));
		}
	}
	return p_list;
}

static WhatmanObjectRegTable * GetRegTable()
{
	static WhatmanObjectRegTable * p_tab = 0;
	SETIFZ(p_tab, new WhatmanObjectRegTable);
	return p_tab;
}

/*static*/int TWhatmanObject::Register(const char * pSymb, const char * pName, FN_WTMOBJ_FACTORY factory)
{
	int    ok = 1;
	ENTER_CRITICAL_SECTION
	WhatmanObjectRegTable * p_tab = GetRegTable();
	ok = p_tab ? p_tab->Add(pSymb, pName, factory) : 0;
	LEAVE_CRITICAL_SECTION
	return ok;
}

/*static*/TWhatmanObject * TWhatmanObject::CreateInstance(long id)
{
	WhatmanObjectRegTable::Item item;
	WhatmanObjectRegTable * p_tab = GetRegTable();
	return (p_tab && p_tab->Search(id, 0, &item) && item.Factory) ? item.Factory() : 0;
}

/*static*/TWhatmanObject * TWhatmanObject::CreateInstance(const char * pSymb)
{
	WhatmanObjectRegTable::Item item;
	WhatmanObjectRegTable * p_tab = GetRegTable();
	return (p_tab && p_tab->Search(pSymb, 0, &item) && item.Factory) ? item.Factory() : 0;
}

/*static*/int TWhatmanObject::GetRegSymbById(long id, SString & rSymb)
{
	int    ok = 0;
	WhatmanObjectRegTable * p_tab = GetRegTable();
	WhatmanObjectRegTable::Item item;
	if(p_tab && p_tab->Search(id, 0, &item)) {
		rSymb = item.P_Symb;
		ok = 1;
	}
	else
		rSymb.Z();
	return ok;
}

/*static*/long TWhatmanObject::GetRegIdBySymb(const char * pSymb)
{
	WhatmanObjectRegTable * p_tab = GetRegTable();
	WhatmanObjectRegTable::Item item;
	return (p_tab && p_tab->Search(pSymb, 0, &item)) ? item.Id : 0;
}

/*static*/StrAssocArray * TWhatmanObject::MakeStrAssocList()
{
	WhatmanObjectRegTable * p_tab = GetRegTable();
	return p_tab ? p_tab->MakeStrAssocList() : 0;
}

TWhatmanObject::TWhatmanObject(const char * pSymb) : Symb(pSymb), State(0), Options(0), P_Owner(0)
{
}

TWhatmanObject::~TWhatmanObject()
{
}

int FASTCALL TWhatmanObject::Copy(const TWhatmanObject & rS)
{
	Symb = rS.Symb;
	State = rS.State;
	Options = rS.Options;
	TextOptions = rS.TextOptions;
	Bounds = rS.Bounds;
	return 1;
}

TWhatmanObject * TWhatmanObject::Dup() const
{
	TWhatmanObject * p_obj = new TWhatmanObject(0);
	CALLPTRMEMB(p_obj, Copy(*this));
	return p_obj;
}

int TWhatmanObject::HandleCommand(int cmd, void * pExt)
{
	int    ok = -1;
	if(cmd == cmdGetSelRetBlock) {
		SelectObjRetBlock * p_blk = static_cast<SelectObjRetBlock *>(pExt);
		if(p_blk) {
			p_blk->WtmObjTypeSymb = Symb;
			p_blk->Val1 = p_blk->Val2 = 0;
			p_blk->ExtString.Z();
		}
	}
	return ok;
}

int    TWhatmanObject::GetTextLayout(STextLayout & rTl, int options) const { return -1; }
int    TWhatmanObject::EditTool(TWhatmanToolArray::Item * pWtaItem) { return HandleCommand(cmdEditTool, pWtaItem); }
int    TWhatmanObject::Edit() { return HandleCommand(cmdEdit, 0); }
int    FASTCALL TWhatmanObject::HasOption(int f) const { return BIN(Options & f); }
int    FASTCALL TWhatmanObject::HasState(int f) const { return BIN(State & f); }
TRect  TWhatmanObject::GetBounds() const { return Bounds; }
TRect  TWhatmanObject::GetInvalidationRect() const { return TRect(Bounds).grow(10, 10); }
int    TWhatmanObject::Draw(TCanvas2 & rCanv) { return -1; }
const  TWhatmanObject::TextParam & TWhatmanObject::GetTextOptions() const { return TextOptions; }
TWhatman * TWhatmanObject::GetOwner() const { return P_Owner; }
TWindow  * TWhatmanObject::GetOwnerWindow() const { return P_Owner ? P_Owner->GetOwnerWindow() : 0; }
const SString & TWhatmanObject::GetSymb() const { return Symb; }

const TLayout::EntryBlock & TWhatmanObject::GetLayoutBlock() const { return Le; }

void TWhatmanObject::SetLayoutBlock(const TLayout::EntryBlock * pBlk)
{
	if(pBlk) {
		Le = *pBlk;
	}
	else {
	}
}

const SString & TWhatmanObject::GetLayoutContainerIdent() const { return LayoutContainerIdent; }
void  TWhatmanObject::SetLayoutContainerIdent(const char * pIdent) { (LayoutContainerIdent = pIdent).Strip(); }

int TWhatmanObject::Setup(const TWhatmanToolArray::Item * pWtaItem)
{
	int    ok = 1;
	if(pWtaItem) {
		THROW(Symb == pWtaItem->WtmObjSymb);
		if(Options & oBackground && P_Owner && P_Owner->GetOwnerWindow())
			Bounds = P_Owner->GetOwnerWindow()->getRect();
		else
			Bounds = pWtaItem->FigSize;
		HandleCommand(cmdSetupByTool, (void *)pWtaItem); // @badcast
	}
	CATCHZOK
	return ok;
}

int TWhatmanObject::SetBounds(const TRect & rRect)
{
	if(rRect.b.x >= rRect.a.x && rRect.b.y >= rRect.a.y) {
		Bounds = rRect;
		HandleCommand(cmdSetBounds, (void *)&rRect); // @badcast
		return 1;
	}
	else
		return 0;
}

int TWhatmanObject::SetTextOptions(const TextParam * pParam)
{
	if(!RVALUEPTR(TextOptions, pParam))
		TextOptions.SetDefault();
	return 1;
}

TRect TWhatmanObject::GetTextBounds() const
{
	TRect  rc;
	const  float wd = static_cast<float>(Bounds.width());
	const  float ht = static_cast<float>(Bounds.height());
	FPoint size(wd, ht);
	if(oneof2(TextOptions.Side, SIDE_BOTTOM, SIDE_TOP)) {
		if(TextOptions.AlongSize < 0.0f)
			size.X *= -TextOptions.AlongSize;
		else if(TextOptions.AlongSize > 0.0f)
			size.X = TextOptions.AlongSize;
		rc.a.x = static_cast<int16>(Bounds.a.x + (wd / 2.0f) - (size.X / 2.0f));
		rc.b.x = static_cast<int16>(Bounds.b.x - (wd / 2.0f) + (size.X / 2.0f));
		//
		if(TextOptions.AcrossSize < 0.0f)
			size.Y *= -TextOptions.AcrossSize;
		else if(TextOptions.AcrossSize > 0.0f)
			size.Y = TextOptions.AcrossSize;
		if(TextOptions.Side == SIDE_BOTTOM)
			rc.setheightrel(static_cast<int>(Bounds.b.y+1), static_cast<int>(size.Y));
		else
			rc.setheightrel(static_cast<int>(Bounds.a.y - size.Y - 1), static_cast<int>(size.Y));
	}
	else if(oneof2(TextOptions.Side, SIDE_LEFT, SIDE_RIGHT)) {
		if(TextOptions.AlongSize < 0.0f)
			size.Y *= -TextOptions.AlongSize;
		else if(TextOptions.AlongSize > 0.0f)
			size.Y = TextOptions.AlongSize;
		rc.a.y = static_cast<int16>(Bounds.a.y + (ht / 2.0f) - (size.Y / 2.0f));
		rc.b.y = static_cast<int16>(Bounds.b.y - (ht / 2.0f) + (size.Y / 2.0f));
		//
		if(TextOptions.AcrossSize < 0.0f)
			size.X *= -TextOptions.AcrossSize;
		else if(TextOptions.AcrossSize > 0.0f)
			size.X = TextOptions.AcrossSize;
		rc.a.x = static_cast<int16>(Bounds.a.x + (wd / 2.0f) - (size.X / 2.0f));
		rc.b.x = static_cast<int16>(Bounds.b.x - (wd / 2.0f) + (size.X / 2.0f));
	}
	else { // SIDE_CENTER
		if(TextOptions.AlongSize < 0.0f)
			size.X *= -TextOptions.AlongSize;
		else if(TextOptions.AlongSize > 0.0f)
			size.X = TextOptions.AlongSize;
		rc.a.x = static_cast<int16>(Bounds.a.x + (wd / 2.0f) - (size.X / 2.0f));
		rc.b.x = static_cast<int16>(Bounds.b.x - (wd / 2.0f) + (size.X / 2.0f));
		//
		if(TextOptions.AcrossSize < 0.0f)
			size.Y *= -TextOptions.AcrossSize;
		else if(TextOptions.AcrossSize > 0.0f)
			size.Y = TextOptions.AcrossSize;
		rc.a.y = static_cast<int16>(Bounds.a.y + (ht / 2.0f) - (size.Y / 2.0f));
		rc.b.y = static_cast<int16>(Bounds.b.y - (ht / 2.0f) + (size.Y / 2.0f));
	}
	return rc;
}

int TWhatmanObject::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	THROW(pCtx->Serialize(dir, Symb, rBuf));
	THROW(pCtx->Serialize(dir, Bounds.a, rBuf));
	THROW(pCtx->Serialize(dir, Bounds.b, rBuf));
	THROW(pCtx->Serialize(dir, Options, rBuf));
	CATCHZOK
	return ok;
}

int TWhatmanObject::DrawToImage(SPaintToolBox & rTb, SImageBuffer & rImg)
{
	int    ok = 1;
	const TRect save_bounds = Bounds;
	TCanvas2 canv(rTb, rImg);
	Bounds.move(-Bounds.a.x, -Bounds.a.y);
	Draw(canv);
	Bounds = save_bounds;
	return ok;
}

int TWhatmanObject::Redraw()
{
	int    ok = -1;
	if(P_Owner) {
		TWindow * p_win = P_Owner->GetOwnerWindow();
		if(p_win) {
			P_Owner->InvalidateObjScope(this);
			::UpdateWindow(p_win->H());
			ok = 1;
		}
	}
	return ok;
}

WhatmanObjectLayoutBase::WhatmanObjectLayoutBase() : TWhatmanObject("Layout")
{
}
	
WhatmanObjectLayoutBase::~WhatmanObjectLayoutBase()
{
}
	
TWhatmanObject * WhatmanObjectLayoutBase::Dup() const
{
	WhatmanObjectLayoutBase * p_obj = new WhatmanObjectLayoutBase();
	CALLPTRMEMB(p_obj, Copy(*this));
	return p_obj;
}
	
int WhatmanObjectLayoutBase::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	//uint8  ind = 0;
	THROW(TWhatmanObject::Serialize(dir, rBuf, pCtx));
	THROW(pCtx->Serialize(dir, ContainerIdent, rBuf));
	if(dir > 0) {
	}
	else if(dir < 0) {
	}
	CATCHZOK
	return ok;
}

int FASTCALL WhatmanObjectLayoutBase::Copy(const WhatmanObjectLayoutBase & rS)
{
	int    ok = 1;
	TWhatmanObject::Copy(rS);
	ContainerIdent = rS.ContainerIdent;
	return ok;
}

TWhatman::Param::Param()
{
	THISZERO();
	//Unit = UNIT_METER;
	//UnitFactor = 0.001;
	Unit = UNIT_INCH;
	UnitFactor = 1.0;
	Scale = 1.0;
}

TArrangeParam::TArrangeParam()
{
	Init(DIREC_HORZ);
}

TArrangeParam & TArrangeParam::Init(int dir)
{
	THISZERO();
	Dir = static_cast<int16>(dir);
	return *this;
}

int TArrangeParam::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	THROW(pCtx->Serialize(dir, Dir, rBuf));
	THROW(pCtx->Serialize(dir, RowSize, rBuf));
	THROW(pCtx->Serialize(dir, UlGap, rBuf));
	THROW(pCtx->Serialize(dir, LrGap, rBuf));
	THROW(pCtx->Serialize(dir, InnerGap, rBuf));
	THROW(pCtx->Serialize(dir, ForceSize, rBuf));
	CATCHZOK
	return ok;
}

TWhatman::TWhatman(TWindow * pOwnerWin) : CurObjPos(-1), ContainerCandidatePos(-1), SrcFileVer(0), P_Wnd(pOwnerWin), P_MultObjPosList(0)
{
	ScrollPos = 0;
}

TWhatman::~TWhatman()
{
	ZDELETE(P_MultObjPosList);
}

int TWhatman::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	if(dir < 0)
		Clear();
	THROW(pCtx->Serialize(dir, P.Name, rBuf));
	THROW(pCtx->Serialize(dir, P.Symb, rBuf));
	THROW(pCtx->Serialize(dir, P.Unit, rBuf));
	THROW(pCtx->Serialize(dir, P.UnitFactor, rBuf));
	THROW(pCtx->Serialize(dir, P.Scale, rBuf));
	THROW(pCtx->Serialize(dir, P.Flags, rBuf));
	THROW(pCtx->SerializeBlock(dir, sizeof(P.Reserve), P.Reserve, rBuf, 0));
	{
		SBuffer obj_buf;
		uint32 c = ObjList.getCount();
		THROW(pCtx->Serialize(dir, c, rBuf));
		if(dir > 0) {
			for(uint i = 0; i < c; i++) {
				TWhatmanObject * p_obj = ObjList.at(i);
				obj_buf.Z();
				THROW(p_obj->Serialize(dir, obj_buf, pCtx));
				THROW(rBuf.Write(obj_buf));
			}
		}
		else if(dir < 0) {
			SString symb;
			for(uint i = 0; i < c; i++) {
				obj_buf.Z();
				THROW(rBuf.Read(obj_buf));
				const size_t preserve_offs = obj_buf.GetRdOffs();
				THROW(pCtx->Serialize(dir, symb, obj_buf));
				{
					TWhatmanObject * p_obj = TWhatmanObject::CreateInstance(symb);
					if(p_obj) {
						obj_buf.SetRdOffs(preserve_offs);
						THROW(p_obj->Serialize(dir, obj_buf, pCtx));
						THROW(InsertObject(p_obj, -1));
					}
					else {
						; // @todo Надо как-то информировать caller о том, что объект не считан
					}
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

void TWhatman::Clear()
{
	ObjList.freeAll();
	CurObjPos = -1;
	SelArea.set(0, 0, 0, 0);
}

TWindow * TWhatman::GetOwnerWindow() const { return P_Wnd; }
const  TWhatman::Param & TWhatman::GetParam() const { return P; }
uint   TWhatman::GetObjectsCount() const { return ObjList.getCount(); }
TWhatmanObject * FASTCALL TWhatman::GetObjectByIndex(int idx) { return (idx >= 0 && idx < ObjList.getCountI()) ? ObjList.at(idx) : 0; }
const  TWhatmanObject * FASTCALL TWhatman::GetObjectByIndexC(int idx) const { return (idx >= 0 && idx < ObjList.getCountI()) ? ObjList.at(idx) : 0; }
const  LongArray * TWhatman::GetMultSelIdxList() const { return P_MultObjPosList; }
int    FASTCALL TWhatman::IsMultSelObject(int idx) const { return BIN(P_MultObjPosList && P_MultObjPosList->lsearch(idx)); }
const  TRect & TWhatman::GetArea() const { return Area; }
const  TRect & TWhatman::GetSelArea() const { return SelArea; }
void   TWhatman::SetScrollPos(TPoint p) { ScrollPos = p; }

int TWhatman::SetParam(const TWhatman::Param & rP)
{
	P = rP;
	return 1;
}

int TWhatman::InsertObject(TWhatmanObject * pObj, int beforeIdx)
{
	int    ok = 1;
	// @v10.9.7 {
	if(pObj->Options & TWhatmanObject::oContainer) {
		WhatmanObjectLayoutBase * p_lo = static_cast<WhatmanObjectLayoutBase *>(pObj);
		if(p_lo->GetContainerIdent().Empty()) {
			SString temp_buf;
			S_GUID uuid;
			uuid.Generate();
			uuid.ToStr(S_GUID::fmtPlain|S_GUID::fmtLower, temp_buf);
			p_lo->SetContainerIdent(temp_buf);
		}
	}
	// } @v10.9.7 
	if(pObj->Options & TWhatmanObject::oBackground) {
		//
		// При вставке фонового объекта предварительно удаляем существующие фоновые объекты
		//
		uint c = ObjList.getCount();
		if(c)
			do {
				TWhatmanObject * p_obj = ObjList.at(--c);
				if(p_obj && p_obj->Options & TWhatmanObject::oBackground) {
					RemoveObject(c);
				}
			} while(c);
		//
		if(CurObjPos >= 0)
			CurObjPos++;
		ObjList.atInsert(0, pObj);
	}
	else if(beforeIdx >= 0 && beforeIdx <= static_cast<int>(ObjList.getCount())) {
		if(CurObjPos >= beforeIdx)
			CurObjPos++;
		ok = ObjList.atInsert(static_cast<uint>(beforeIdx), pObj);
	}
	else
		ok = ObjList.insert(pObj);
	if(ok) {
		pObj->P_Owner = this;
		pObj->HandleCommand(TWhatmanObject::cmdObjInserted, 0);
	}
	return ok;
}

int TWhatman::RemoveObject(int idx)
{
	int    ok = -1;
	if(idx >= 0 && idx < static_cast<int>(ObjList.getCount())) {
		if(CurObjPos == idx)
			SetCurrentObject(-1, 0);
		else if(CurObjPos > idx)
			CurObjPos--;
		ObjList.atFree(idx);
		ok = 1;
	}
	return ok;
}

int TWhatman::CheckUniqLayoutSymb(const TWhatmanObject * pObj) const
{
	int    ok = 1;
	if(pObj) {
		const WhatmanObjectLayoutBase * p_layout_obj = static_cast<const WhatmanObjectLayoutBase *>(pObj);
		if(p_layout_obj->GetContainerIdent().NotEmpty()) {
			for(uint i = 0; ok && i < ObjList.getCount(); i++) {
				const TWhatmanObject * p_iter_obj = ObjList.at(i);
				if(p_iter_obj && p_iter_obj != pObj && p_iter_obj->Symb.IsEqiAscii("Layout")) {
					const WhatmanObjectLayoutBase * p_iter_layout_obj = static_cast<const WhatmanObjectLayoutBase *>(p_iter_obj);
					if(p_iter_layout_obj->GetContainerIdent() == p_layout_obj->GetContainerIdent()) {
						SLS.SetError(SLERR_WTM_DUPLAYOUTSYMB, p_layout_obj->GetContainerIdent());
						ok = 0;
					}
				}
			}
		}
	}
	else
		ok = -1;
	return ok;
}

int TWhatman::GetLayoutSymbList(StrAssocArray & rList) const
{
	rList.Z();
	int    ok = -1;
	for(uint i = 0; i < ObjList.getCount(); i++) {
		const TWhatmanObject * p_iter_obj = ObjList.at(i);
		if(p_iter_obj && p_iter_obj->Symb.IsEqiAscii("Layout")) {
			const WhatmanObjectLayoutBase * p_iter_layout_obj = static_cast<const WhatmanObjectLayoutBase *>(p_iter_obj);
			if(p_iter_layout_obj->GetContainerIdent().NotEmpty()) {
				rList.Add(i+1, p_iter_layout_obj->GetContainerIdent());
			}
		}
	}
	if(rList.getCount())
		ok = 1;
	return ok;
}

int TWhatman::BringObjToFront(int idx)
{
	int    ok = -1;
	const uint c = ObjList.getCount();
	if(c > 1 && idx >= 0 && !(ObjList.at(idx)->Options & TWhatmanObject::oBackground)) {
		for(uint i = static_cast<uint>(idx); (i+1) < c; i++) {
			ObjList.swap(i, i+1);
			ok = 1;
		}
	}
	return ok;
}

int TWhatman::SendObjToBack(int idx)
{
	int    ok = -1;
	const uint c = ObjList.getCount();
	if(c > 1 && idx < static_cast<int>(c)) {
		for(uint i = static_cast<uint>(idx); i > 0; i--) {
			if(i != 1 || !(ObjList.at(0)->Options & TWhatmanObject::oBackground)) {
				ObjList.swap(i, i-1);
				ok = 1;
			}
		}
	}
	return ok;
}

int TWhatman::SnapX(float p, float * pDest) const
{
	int    ok = RuleY.GetNearest(p, static_cast<float>(Area.width()), pDest);
	return ok;
}

int TWhatman::SnapY(float p, float * pDest) const
{
	int    ok = RuleX.GetNearest(p, static_cast<float>(Area.height()), pDest);
	return ok;
}

int TWhatman::MoveObject(TWhatmanObject * pObj, const TRect & rRect)
{
	int    ok = -1;
	if(pObj) {
		if(P.Flags & Param::fSnapToGrid && P.Flags & Param::fGrid) {
			TRect result = rRect;
			//result.move(-ScrollPos.x, -ScrollPos.y);
			FPoint p;
			if(SnapX(rRect.a.x, &p.X) > 0)
				result.setwidthrel(static_cast<int>(p.X), rRect.width());
			else if(SnapX(rRect.b.x, &p.X) > 0) {
				result.b.x = static_cast<int>(p.X);
				result.a.x = result.b.x - rRect.width();
			}
			if(SnapY(rRect.a.y, &p.Y) > 0)
				result.setheightrel(static_cast<int>(p.Y), rRect.height());
			else if(SnapY(rRect.b.y, &p.Y) > 0) {
				result.b.y = static_cast<int>(p.Y);
				result.a.y = result.b.y - rRect.height();
			}
			ok = pObj->SetBounds(result);
		}
		else {
			ok = pObj->SetBounds(rRect);
		}
	}
	return ok;
}

static void __stdcall FlexSetupProc_WhatmanFig(LayoutFlexItem * pItem, float size[4])
{
	TWhatmanObject * p_obj = static_cast<TWhatmanObject *>(pItem->managed_ptr);
	if(p_obj) {
		TRect r;
		FRect fr = pItem->GetFrame();
		if(pItem->P_Parent) {
			fr.a.X += pItem->P_Parent->frame[0];
			fr.b.X += pItem->P_Parent->frame[0];
			fr.a.Y += pItem->P_Parent->frame[1];
			fr.b.Y += pItem->P_Parent->frame[1];
		}
		p_obj->SetBounds(r.set(fr));
	}
}

int TWhatman::ArrangeObjects2(const LongArray * pObjPosList, const TArrangeParam & rParam)
{
	int    ok = -1;
	TRect area = Area;
	TPoint pt_next;
	pt_next = area.a + rParam.UlGap;
	const  uint row_size = rParam.RowSize;
	//const  int dir = ((rParam.Dir == DIREC_HORZ && row_size) || (rParam.Dir != DIREC_HORZ && !row_size)) ? DIREC_VERT : DIREC_HORZ;
	uint   row_no = 0;
	uint   item_in_row = 0;
	int    row_bound = 0; // Минимальный отступ от предыдущего ряда.
	LayoutFlexItem lo_root;
	lo_root.Direction = ((rParam.Dir == DIREC_HORZ && row_size) || (rParam.Dir != DIREC_HORZ && !row_size)) ? FLEX_DIRECTION_COLUMN : FLEX_DIRECTION_ROW;
	lo_root.AlignContent = FLEX_ALIGN_START;
	//lo_root.WrapMode = FLEX_WRAP_WRAP;
	lo_root.Flags |= LayoutFlexItem::fWrap;
	lo_root.Padding.a.X = 32.0f;
	lo_root.Padding.b.X = 32.0f;
	for(uint i = 0; i < ObjList.getCount(); i++) {
		if(!pObjPosList || pObjPosList->lsearch(static_cast<long>(i))) {
			TWhatmanObject * p_obj = ObjList.at(i);
			if(p_obj) {
				const TRect obj_bounds = p_obj->Bounds;
				const float _item_width = 32.0f/*static_cast<float>(obj_bounds.width())*/;
				const float _item_height = 32.0f/*static_cast<float>(obj_bounds.height())*/;
				LayoutFlexItem * p_lo_item = lo_root.InsertItem();
				if(p_lo_item) {
					LayoutFlexItem * p_lo_text = 0;
					p_lo_item->Direction = FLEX_DIRECTION_COLUMN;

					TRect bounds;
					STextLayout tlo;
					p_lo_item->Margin.a.X = (static_cast<float>(rParam.InnerGap.x) / 2.0f);
					p_lo_item->Margin.a.Y = (static_cast<float>(rParam.InnerGap.y) / 2.0f);
					p_lo_item->Margin.b.X = (static_cast<float>(rParam.InnerGap.x) / 2.0f);
					p_lo_item->Margin.b.Y = (static_cast<float>(rParam.InnerGap.y) / 2.0f);

					LayoutFlexItem * p_lo_fig = p_lo_item->InsertItem();
					p_lo_fig->Size.X = _item_width;
					p_lo_fig->Size.Y = _item_height;
					p_lo_fig->managed_ptr = p_obj;
					p_lo_fig->CbSetup = FlexSetupProc_WhatmanFig;

					if(p_obj->GetTextLayout(tlo, TWhatmanObject::gtloQueryForArrangeObject) > 0) {
						TRect text_bounds;
						text_bounds.set(tlo.GetBounds());
						p_lo_text = p_lo_item->InsertItem();
						if(p_lo_text) {
							p_lo_text->Size.X = static_cast<float>(text_bounds.width());
							p_lo_text->Size.Y = static_cast<float>(text_bounds.height());
							p_lo_text->Margin.a.Y = 2.0f;
						}
						/*p_lo_item->padding_bottom -= static_cast<float>(text_bounds.height());
						if(text_bounds.width() > _item_width) {
							p_lo_item->padding_left -= static_cast<float>(text_bounds.width() - _item_width) / 2.0f;
							p_lo_item->padding_right -= static_cast<float>(text_bounds.width() - _item_width) / 2.0f;
							//p_lo_item->width = static_cast<float>(text_bounds.width());
						}*/
					}
					{
						{
							float fs = 0.0f;
							float ts = 0.0f;
							if((!p_lo_fig || p_lo_fig->GetFullHeight(&fs)) && (!p_lo_text || p_lo_text->GetFullHeight(&ts))) {
								p_lo_item->Size.Y = (fs + ts);
							}
						}
						{
							float text_width = 0.0f;
							float fig_width = 0.0f;
							if((!p_lo_text || p_lo_text->GetFullWidth(&text_width)) && (!p_lo_fig || p_lo_fig->GetFullWidth(&fig_width))) {
								p_lo_item->Size.X = MAX(fig_width, text_width);
							}
						}
					}
					/*if(bounds != p_obj->Bounds) {
						p_obj->SetBounds(bounds);
						ok = 1;
					}*/
				}
			}
		}
	}
	{
		lo_root.Size.X = static_cast<float>(Area.width() - rParam.UlGap.x - rParam.LrGap.x);
		lo_root.Size.Y = static_cast<float>(Area.height() - rParam.UlGap.y - rParam.LrGap.y);
		lo_root.Evaluate();
		/*for(uint i = 0; i < p_lo_root->getCount(); i++) {
			const LayoutFlexItem * p_lo_item = p_lo_root->at(i);
			if(p_lo_item && p_lo_item->managed_ptr) {
				TWhatmanObject * p_obj = static_cast<TWhatmanObject *>(p_lo_item->managed_ptr);
				TRect r;
				p_obj->SetBounds(r.set(p_lo_item->GetFrame()));
				ok = 1;
			}
		}*/
	}
	return ok;
}

int TWhatman::ArrangeObjects(const LongArray * pObjPosList, const TArrangeParam & rParam)
{
	int    ok = -1;
	TRect area = Area;
	TPoint pt_next;
	pt_next = area.a + rParam.UlGap;
	const  uint row_size = rParam.RowSize;
	const  int dir = ((rParam.Dir == DIREC_HORZ && row_size) || (rParam.Dir != DIREC_HORZ && !row_size)) ? DIREC_VERT : DIREC_HORZ;
	uint   row_no = 0;
	uint   item_in_row = 0;
	int    row_bound = 0; // Минимальный отступ от предыдущего ряда.
	for(uint i = 0; i < ObjList.getCount(); i++) {
		if(!pObjPosList || pObjPosList->lsearch(static_cast<long>(i))) {
			TWhatmanObject * p_obj = ObjList.at(i);
			if(p_obj) {
				const TRect obj_bounds = p_obj->Bounds;
				TRect bounds;
				STextLayout tlo;
				TPoint ul_txt_gap; // Зазор верхнего левого угла для текста
				TPoint lr_txt_gap; // Зазор нижнего правого угла для текста
				ul_txt_gap = 0;
				lr_txt_gap = 0;
				if(p_obj->GetTextLayout(tlo, TWhatmanObject::gtloQueryForArrangeObject) > 0) {
					TRect text_bounds;
					text_bounds.set(tlo.GetBounds());
					if(text_bounds.a.x < obj_bounds.a.x)
						ul_txt_gap.x = -(text_bounds.a.x - obj_bounds.a.x);
					if(text_bounds.a.y < obj_bounds.a.y)
						ul_txt_gap.y = -(text_bounds.a.y - obj_bounds.a.y);
					if(text_bounds.b.x > obj_bounds.b.x)
						lr_txt_gap.x = (text_bounds.b.x - obj_bounds.b.x);
					if(text_bounds.b.y > obj_bounds.b.y)
						lr_txt_gap.y = (text_bounds.b.y - obj_bounds.b.y);
					pt_next += ul_txt_gap;
				}
				for(int ok_bounds = 0; !ok_bounds;) {
					bounds.setwidthrel(pt_next.x,  p_obj->Bounds.width());
					bounds.setheightrel(pt_next.y, p_obj->Bounds.height());
					if(dir == DIREC_HORZ) {
						if((row_size && item_in_row >= row_size) || (item_in_row && !row_size && (bounds.b.x + rParam.LrGap.x + lr_txt_gap.x) > area.b.x)) {
							pt_next.Set(area.a.x + rParam.UlGap.x + ul_txt_gap.x, row_bound + rParam.InnerGap.y);
							row_no++;
							item_in_row = 0;
						}
						else {
							pt_next.Set(bounds.b.x + rParam.InnerGap.x + lr_txt_gap.x, bounds.a.y);
							SETMAX(row_bound, bounds.b.y + lr_txt_gap.y);
							ok_bounds = 1;
						}
					}
					else { // dir == DIREC_VERT
						if((row_size && item_in_row >= row_size) || (item_in_row && !row_size && (bounds.b.y + rParam.LrGap.y + lr_txt_gap.y) > area.b.y)) {
							pt_next.Set(row_bound + rParam.InnerGap.x, area.a.y + rParam.UlGap.y + ul_txt_gap.y);
							row_no++;
							item_in_row = 0;
						}
						else {
							pt_next.Set(bounds.a.x, bounds.b.y + rParam.InnerGap.y);
							SETMAX(row_bound, bounds.b.x + lr_txt_gap.x);
							ok_bounds = 1;
						}
					}
				}
				item_in_row++;
				if(bounds != p_obj->Bounds) {
					p_obj->SetBounds(bounds);
					ok = 1;
				}
			}
		}
	}
	return ok;
}

int FASTCALL TWhatman::GetCurrentObject(int * pIdx) const
{
	int    ok = 0;
	int    idx = -1;
	if(CurObjPos >= 0 && CurObjPos < ObjList.getCountI()) {
		idx = CurObjPos;
		ok = 1;
	}
	ASSIGN_PTR(pIdx, idx);
	return ok;
}

int TWhatman::SetCurrentObject(int idx, int * pPrevCurObjIdx)
{
	int    ok = -1;
	int    prev_idx = -1;
	if(CurObjPos >= 0 && CurObjPos < ObjList.getCountI()) {
		ObjList.at(CurObjPos)->State &= ~TWhatmanObject::stCurrent;
		prev_idx = CurObjPos;
		CurObjPos = -1;
		ok = 1;
	}
	if(idx >= 0 && idx < ObjList.getCountI()) {
		CurObjPos = idx;
		ObjList.at(idx)->State |= TWhatmanObject::stCurrent;
		ok = 1;
	}
	ASSIGN_PTR(pPrevCurObjIdx, prev_idx);
	return ok;
}

void TWhatman::SetupContainerCandidate(int idx, bool set)
{
	ContainerCandidatePos = -1;
	if(idx >= 0 && idx < ObjList.getCountI()) {
		TWhatmanObject * p_container = ObjList.at(idx);
		assert(p_container && p_container->HasOption(TWhatmanObject::oContainer));
		if(p_container && p_container->HasOption(TWhatmanObject::oContainer)) {
			SETFLAG(p_container->State, TWhatmanObject::stContainerCandidate, set);
			if(set)
				ContainerCandidatePos = idx;
		}
	}
	//
	// В любом случае, при изменении у единственного объекта статуса TWhatmanObject::stContainerCandidate
	// мы снимаем аналогичный статус у всех, кто его имеет.
	// По спецификации, такой статус может быть установлен только у одного объекта.
	//
	for(uint i = 0; i < ObjList.getCount(); i++) {
		if(idx < 0 || i != static_cast<uint>(idx)) {
			TWhatmanObject * p_obj = ObjList.at(i);
			assert(p_obj);
			if(p_obj) {
				if(p_obj->State & TWhatmanObject::stContainerCandidate) {
					assert(p_obj->HasOption(TWhatmanObject::oContainer));
					p_obj->State &= ~TWhatmanObject::stContainerCandidate;
				}
			}
		}
	}
}

int TWhatman::AddMultSelObject(int idx)
{
	int    ok = 0;
	TWhatmanObject * p_obj = GetObjectByIndex(idx);
	if(p_obj) {
		if(p_obj->HasOption(TWhatmanObject::oMultSelectable)) {
			if(SETIFZ(P_MultObjPosList, new LongArray)) {
				ok = (P_MultObjPosList->addUnique(idx) > 0) ? 1 : -2;
				if(ok > 0)
					p_obj->State |= TWhatmanObject::stSelected;
			}
		}
		else
			ok = -1;
	}
	return ok;
}

int TWhatman::RmvMultSelObject(int idx)
{
	int    ok = -1;
	if(P_MultObjPosList) {
		uint   pos = 0;
		if(idx == -1) {
			for(uint i = 0; i < P_MultObjPosList->getCount(); i++) {
				TWhatmanObject * p_obj = GetObjectByIndex(P_MultObjPosList->at(i));
				if(p_obj)
					p_obj->State &= ~TWhatmanObject::stSelected;
			}
			ZDELETE(P_MultObjPosList);
			ok = 1;
		}
		else if(P_MultObjPosList->lsearch(idx, &pos)) {
			TWhatmanObject * p_obj = GetObjectByIndex(idx);
			if(p_obj)
				p_obj->State &= ~TWhatmanObject::stSelected;
			P_MultObjPosList->atFree(pos);
			ok = 1;
		}
	}
	return ok;
}

int TWhatman::SetupMultSelBySelArea()
{
	int    ok = -1;
	TRect sr = SelArea;
	sr.Normalize();
	for(uint i = 0; i < ObjList.getCount(); i++) {
		const TWhatmanObject * p_obj = ObjList.at(i);
		if(p_obj) {
			TRect b, isr;
			if(sr.Intersect(b.set(p_obj->Bounds), &isr) > 0) {
				if(AddMultSelObject(i) > 0)
					ok = 1;
			}
		}
	}
	return ok;
}

int FASTCALL TWhatman::HaveMultSelObjectsOption(int f) const
{
	int    ok = 1;
	if(P_MultObjPosList) {
		for(uint i = 0; i < P_MultObjPosList->getCount(); i++) {
			const TWhatmanObject * p_obj = GetObjectByIndexC(P_MultObjPosList->at(i));
			if(!p_obj || !p_obj->HasOption(f))
				ok = 0;
		}
	}
	else
		ok = 0;
	return ok;
}

int TWhatman::FindObjectByPoint(TPoint p, int * pIdx) const
{
	int    ok = 0;
	uint   c = ObjList.getCount();
	p += ScrollPos;
	if(c) do {
		const TWhatmanObject * p_obj = ObjList.at(--c);
		if(p_obj) {
			ObjZone zone_list[16];
			if(p_obj->State & TWhatmanObject::stCurrent) {
				GetResizeRectList(p_obj, zone_list);
				for(uint i = 0; !ok && i < 8; i++)
					if(zone_list[i].R.Contains(p))
						ok = 100 + zone_list[i].I;
			}
			if(!ok) {
				if(p_obj->HasOption(TWhatmanObject::oFrame)) {
					GetFrameRectList(p_obj, zone_list);
					for(uint i = 0; !ok && i < 4; i++)
						if(zone_list[i].R.Contains(p))
							ok = 10 + zone_list[i].I;
				}
				else if(p_obj->Bounds.contains(p))
					ok = 1;
			}
		}
	} while(!ok && c);
	ASSIGN_PTR(pIdx, static_cast<int>(c));
	return ok;
}

int TWhatman::FindContainerCandidateForObjectByPoint(TPoint p, const TWhatmanObject * pObj, int * pIdx) const
{
	int    ok = 0;
	if(pObj) {
		const SString & r_current_container_symb = pObj->GetLayoutContainerIdent();
		uint   c = ObjList.getCount();
		p += ScrollPos;
		if(c) do {
			const TWhatmanObject * p_obj = ObjList.at(--c);
			if(p_obj && p_obj != pObj && p_obj->HasOption(TWhatmanObject::oContainer) && p_obj->Bounds.contains(p)) {
				if(r_current_container_symb.Empty() || p_obj->Symb != r_current_container_symb) {
					ASSIGN_PTR(pIdx, static_cast<int>(c));				
					ok = 1;		
				}
			}
		} while(!ok && c);
	}
	return ok;
}

int TWhatman::SetTool(int toolId, int paintObjIdent)
{
	int    ok = 1;
	switch(toolId) {
		case toolPenObjBorder:    TidPenObjBorder = paintObjIdent; break;
		case toolPenObjBorderSel: TidPenObjBorderSel = paintObjIdent; break;
		case toolPenObjBorderCur: TidPenObjBorderCur = paintObjIdent; break;
		case toolPenObjRszSq:     TidPenObjRszSq = paintObjIdent; break;
		case toolBrushObjRszSq:   TidBrushObjRszSq = paintObjIdent; break;
		case toolPenObjNonmovBorder: TidPenObjNonmovBorder = paintObjIdent; break;
		case toolPenRule:    TidPenRule = paintObjIdent; break;
		case toolBrushRule:  TidBrushRule = paintObjIdent; break;
		case toolPenGrid:    TidPenGrid = paintObjIdent; break;
		case toolPenSubGrid: TidPenSubGrid = paintObjIdent; break;
		case toolPenLayoutBorder: TidPenLayoutBorder = paintObjIdent; break; // @v10.4.8
		case toolPenContainerCandidateBorder: TidPenContainerCandidateBorder = paintObjIdent; break; // @v10.9.6
		default: ok = 0; break;
	}
	return ok;
}

int TWhatman::GetTool(int toolId) const
{
	switch(toolId) {
		case toolPenObjBorder:    return TidPenObjBorder;
		case toolPenObjBorderSel: return TidPenObjBorderSel;
		case toolPenObjBorderCur: return TidPenObjBorderCur;
		case toolPenObjRszSq:     return TidPenObjRszSq;
		case toolBrushObjRszSq:   return TidBrushObjRszSq;
		case toolPenObjNonmovBorder: return TidPenObjNonmovBorder;
		case toolPenRule:    return TidPenRule;
		case toolBrushRule:  return TidBrushRule;
		case toolPenGrid:    return TidPenGrid;
		case toolPenSubGrid: return TidPenSubGrid;
		case toolPenLayoutBorder: return TidPenLayoutBorder; // @v10.4.8
		case toolPenContainerCandidateBorder: return TidPenContainerCandidateBorder; // @v10.9.6
	}
	return 0;
}

int TWhatman::SetArea(const TRect & rArea)
{
	Area = rArea;
	CalcScrollRange();
	{
		//
		// Для фонового объекта необходимо изменить размер
		//
		uint c = ObjList.getCount();
		if(c)
			do {
				TWhatmanObject * p_obj = ObjList.at(--c);
				if(p_obj && p_obj->Options & TWhatmanObject::oBackground) {
					TRect br;
					br.setwidthrel(0, Area.width());
					br.setheightrel(0, Area.height());
					p_obj->SetBounds(br);
				}
			} while(c);
	}
	return 1;
}

int TWhatman::SetSelArea(TPoint p, int mode)
{
	int    ok = 1;
	if(mode == 0)
		SelArea.set(0, 0, 0, 0);
	else if(mode == 1)
		SelArea.a = SelArea.b = p;
	else if(mode == 2) {
		SelArea.b = p;
	}
	else
		ok = 0;
	return ok;
}

void TWhatman::GetScrollRange(IntRange * pX, IntRange * pY) const
{
	CALLPTRMEMB(pX, Set(ScrollRange.a.x, ScrollRange.b.x));
	CALLPTRMEMB(pY, Set(ScrollRange.a.y, ScrollRange.b.y));
}

TPoint TWhatman::GetScrollDelta() const
{
	TPoint delta;
	return delta.Set(RuleX.ScrollDelta, RuleY.ScrollDelta);
}

static const float frame_sq = 7.0f;
static const float frame_semisq = frame_sq / 2.0f;

void TWhatman::GetFrameRectList(const TWhatmanObject * pObj, ObjZone * pList) const
{
	pList[0].R.a.X = (pObj->Bounds.a.x - frame_semisq);
	pList[0].R.b.X = (pObj->Bounds.a.x + frame_semisq);
	pList[0].R.a.Y = (pObj->Bounds.a.y - frame_semisq);
	pList[0].R.b.Y = (pObj->Bounds.b.y + frame_semisq);
	pList[0].I = SOW_WEST;

	pList[1].R.a.X = (pObj->Bounds.a.x - frame_semisq);
	pList[1].R.b.X = (pObj->Bounds.b.x + frame_semisq);
	pList[1].R.a.Y = (pObj->Bounds.a.y - frame_semisq);
	pList[1].R.b.Y = (pObj->Bounds.a.y + frame_semisq);
	pList[1].I = SOW_NORD;

	pList[2].R.a.X = (pObj->Bounds.b.x - frame_semisq);
	pList[2].R.b.X = (pObj->Bounds.b.x + frame_semisq);
	pList[2].R.a.Y = (pObj->Bounds.a.y - frame_semisq);
	pList[2].R.b.Y = (pObj->Bounds.b.y + frame_semisq);
	pList[2].I = SOW_EAST;

	pList[3].R.a.X = (pObj->Bounds.a.x - frame_semisq);
	pList[3].R.b.X = (pObj->Bounds.b.x + frame_semisq);
	pList[3].R.a.Y = (pObj->Bounds.b.y - frame_semisq);
	pList[3].R.b.Y = (pObj->Bounds.b.y + frame_semisq);
	pList[3].I = SOW_SOUTH;
}

void TWhatman::GetResizeRectList(const TWhatmanObject * pObj, ObjZone * pList) const
{
	FPoint sq_size(frame_sq);
	FPoint dot;

	dot = pObj->Bounds.a;
	pList[0].R.Around(dot, sq_size);
	pList[0].I = SOW_NORDWEST;

	dot.X = pObj->Bounds.CenterX();
	pList[1].R.Around(dot, sq_size);
	pList[1].I = SOW_NORD;

	dot.X = pObj->Bounds.b.x;
	pList[2].R.Around(dot, sq_size);
	pList[2].I = SOW_NORDEAST;

	dot.Y = pObj->Bounds.CenterY();
	pList[3].R.Around(dot, sq_size);
	pList[3].I = SOW_EAST;

	dot.Y = pObj->Bounds.b.y;
	pList[4].R.Around(dot, sq_size);
	pList[4].I = SOW_SOUTHEAST;

	dot.X = pObj->Bounds.CenterX();
	pList[5].R.Around(dot, sq_size);
	pList[5].I = SOW_SOUTH;

	dot.X = pObj->Bounds.a.x;
	pList[6].R.Around(dot, sq_size);
	pList[6].I = SOW_SOUTHWEST;

	dot.Y = pObj->Bounds.CenterY();
	pList[7].R.Around(dot, sq_size);
	pList[7].I = SOW_WEST;
}

int TWhatman::EditObject(int objIdx)
{
	int    ok = -1;
	if(objIdx >= 0 && objIdx < static_cast<int>(ObjList.getCount())) {
		TWhatmanObject * p_obj = ObjList.at(objIdx);
		if(p_obj)
			ok = p_obj->Edit();
	}
	return ok;
}

int TWhatman::ResizeObject(TWhatmanObject * pObj, int dir, TPoint toPoint, TRect * pResult)
{
	int    ok = -1;
	TRect  b;
	if(pObj) {
		b = pObj->Bounds;
		if(P.Flags & Param::fSnapToGrid && P.Flags & Param::fGrid) {
			FPoint fp;
			SnapX(toPoint.x, &fp.X);
			SnapY(toPoint.y, &fp.Y);
			switch(dir) {
				case SOW_NORD:
					if(fp.Y <= b.b.y)
						b.a.y = static_cast<int16>(fp.Y);
					break;
				case SOW_NORDEAST:
					if(fp.X >= b.a.x)
						b.b.x = static_cast<int16>(fp.X);
					if(fp.Y <= b.b.y)
						b.a.y = static_cast<int16>(fp.Y);
					break;
				case SOW_EAST:
					if(fp.X >= b.a.x)
						b.b.x = static_cast<int16>(fp.X);
					break;
				case SOW_SOUTHEAST:
					if(fp.X >= b.a.x)
						b.b.x = static_cast<int16>(fp.X);
					if(fp.Y >= b.a.y)
						b.b.y = static_cast<int16>(fp.Y);
					break;
				case SOW_SOUTH:
					if(fp.Y >= b.a.y)
						b.b.y = static_cast<int16>(fp.Y);
					break;
				case SOW_SOUTHWEST:
					if(fp.X <= b.b.x)
						b.a.x = static_cast<int16>(fp.X);
					if(fp.Y >= b.a.y)
						b.b.y = static_cast<int16>(fp.Y);
					break;
				case SOW_WEST:
					if(fp.X <= b.b.x)
						b.a.x = static_cast<int16>(fp.X);
					break;
				case SOW_NORDWEST:
					if(fp.X <= b.b.x)
						b.a.x = static_cast<int16>(fp.X);
					if(fp.Y <= b.b.y)
						b.a.y = static_cast<int16>(fp.Y);
					break;
			}
		}
		else {
			switch(dir) {
				case SOW_NORD:
					if(toPoint.y <= b.b.y)
						b.a.y = toPoint.y;
					break;
				case SOW_NORDEAST:
					if(toPoint.x >= b.a.x)
						b.b.x = toPoint.x;
					if(toPoint.y <= b.b.y)
						b.a.y = toPoint.y;
					break;
				case SOW_EAST:
					if(toPoint.x >= b.a.x)
						b.b.x = toPoint.x;
					break;
				case SOW_SOUTHEAST:
					if(toPoint.x >= b.a.x)
						b.b.x = toPoint.x;
					if(toPoint.y >= b.a.y)
						b.b.y = toPoint.y;
					break;
				case SOW_SOUTH:
					if(toPoint.y >= b.a.y)
						b.b.y = toPoint.y;
					break;
				case SOW_SOUTHWEST:
					if(toPoint.x <= b.b.x)
						b.a.x = toPoint.x;
					if(toPoint.y >= b.a.y)
						b.b.y = toPoint.y;
					break;
				case SOW_WEST:
					if(toPoint.x <= b.b.x)
						b.a.x = toPoint.x;
					break;
				case SOW_NORDWEST:
					if(toPoint.x <= b.b.x)
						b.a.x = toPoint.x;
					if(toPoint.y <= b.b.y)
						b.a.y = toPoint.y;
					break;
			}
		}
		if(b != pObj->Bounds) {
			pObj->SetBounds(b);
			ok = 1;
		}
	}
	ASSIGN_PTR(pResult, b);
	return ok;
}

TPoint FASTCALL TWhatman::TransformScreenToPoint(TPoint p) const
{
	p.x += ScrollPos.x;
	p.y += ScrollPos.y;
	return p;
}

TPoint FASTCALL TWhatman::TransformPointToScreen(TPoint p) const
{
	p.x -= ScrollPos.x;
	p.y -= ScrollPos.y;
	return p;
}

FPoint FASTCALL TWhatman::TransformPointToScreen(FPoint p) const
{
	p.X -= static_cast<float>(ScrollPos.x);
	p.Y -= static_cast<float>(ScrollPos.y);
	return p;
}

int TWhatman::GetObjTextLayout(const TWhatmanObject * pObj, STextLayout & rTl, int options)
{
	int    ok = -1;
	if(pObj) {
		ok = pObj->GetTextLayout(rTl, options);
		if(ok > 0) {
			FRect b = rTl.GetBounds();
			b.a = TransformPointToScreen(b.a);
			b.b = TransformPointToScreen(b.b);
			rTl.SetBounds(b);
		}
	}
	return ok;
}

int TWhatman::DrawSingleObject(TCanvas2 & rCanv, TWhatmanObject * pObj)
{
	int    ok = 1;
	if(pObj) {
		LMatrix2D mtx;
		rCanv.PushTransform();
		if(ScrollPos.x || ScrollPos.y) {
			mtx.InitTranslate(-ScrollPos.x, -ScrollPos.y);
			rCanv.SetTransform(mtx);
		}
		ok = DrawObject(rCanv, pObj);
		rCanv.PopTransform();
	}
	else
		ok = -1;
	return ok;
}

int TWhatman::DrawObjectContour(TCanvas2 & rCanv, const TWhatmanObject * pObj, const TPoint * pOffs)
{
	int    ok = 1;
	if(pObj) {
		TRect r = pObj->GetBounds();
		FRect fr(pOffs ? r.move(pOffs->x, pOffs->y) : r);
		rCanv.Rect(fr.Grow(0.5f, 0.5f), TidPenRule, 0);
	}
	else
		ok = -1;
	return ok;
}

int FASTCALL TWhatman::InvalidateObjScope(const TWhatmanObject * pObj)
{
	int    ok = 1;
	if(P_Wnd && pObj) {
		STextLayout tlo;
		TRect ob = pObj->GetInvalidationRect();
		ob.a = TransformPointToScreen(ob.a);
		ob.b = TransformPointToScreen(ob.b);
		if(GetObjTextLayout(pObj, tlo, TWhatmanObject::gtloBoundsOnly) > 0) {
			SRegion rgn(ob);
			rgn.Add(ob.set(tlo.GetBounds()), SCOMBINE_OR);
			P_Wnd->invalidateRegion(rgn, 1);
		}
		else
			P_Wnd->invalidateRect(ob, 1);
	}
	return ok;
}


int TWhatman::InvalidateMultSelContour(const TPoint * pOffs)
{
	int    ok = -1;
	if(P_MultObjPosList && P_Wnd) {
		const uint c = P_MultObjPosList->getCount();
		if(c) {
			SRegion rgn;
			for(uint i = 0; i < c; i++) {
				const TWhatmanObject * p_obj = GetObjectByIndexC(P_MultObjPosList->at(i));
				if(p_obj) {
					TRect r = p_obj->GetBounds();
					if(pOffs)
						r.move(pOffs->x, pOffs->y);
					rgn.AddFrame(r, 3, SCOMBINE_OR);
					ok = 1;
				}
			}
			if(ok > 0) {
				P_Wnd->invalidateRegion(rgn, 1);
			}
		}
	}
	return ok;
}

int TWhatman::DrawMultSelContour(TCanvas2 & rCanv, const TPoint * pOffs)
{
	int    ok = 1;
	if(P_MultObjPosList) {
		for(uint i = 0; i < P_MultObjPosList->getCount(); i++) {
			TWhatmanObject * p_obj = GetObjectByIndex(P_MultObjPosList->at(i));
			DrawObjectContour(rCanv, p_obj, pOffs);
		}
	}
	else
		ok = -1;
	return ok;
}

int TWhatman::DrawObject(TCanvas2 & rCanv, TWhatmanObject * pObj)
{
	int    ok = 1;
	if(pObj) {
		pObj->Draw(rCanv);
		int    pen_ident;
		if(pObj->State & TWhatmanObject::stCurrent)
			pen_ident = (pObj->HasOption(TWhatmanObject::oMovable) && !(P.Flags & Param::fDisableMoveObj)) ?
				TidPenObjBorderCur : TidPenObjNonmovBorder;
		else if(pObj->State & TWhatmanObject::stSelected)
			pen_ident = TidPenObjBorderSel;
		else
			pen_ident = TidPenObjBorder;
		FRect b = pObj->Bounds;
		rCanv.Rect(b.Grow(0.5f, 0.5f), pen_ident, 0);
		if(pObj->State & TWhatmanObject::stCurrent) {
			if(pObj->HasOption(TWhatmanObject::oResizable) && !(P.Flags & Param::fDisableReszObj)) {
				ObjZone zone_list[8];
				GetResizeRectList(pObj, zone_list);
				for(uint j = 0; j < SIZEOFARRAY(zone_list); j++)
					rCanv.Rect(zone_list[j].R, TidPenObjRszSq, TidBrushObjRszSq);
			}
		}
	}
	else
		ok = -1;
	return ok;
}

/*static*/float TWhatman::GetRuleWidth()
{
	static const float __rule_width = 15.5f;
	return __rule_width;
}

int TWhatman::GetNotchList(const Rule & rRule, float size, float offs, int kind, TSVector <RuleNotch> & rList) const
{
	int    ok = 1;
	rList.clear();
	const float limit = static_cast<float>(((size + offs) / rRule.OneUnitDots) + 1.0f);
	const uint nc = rRule.NotchList.getCount();
	for(float t = -offs; t < limit; t += 1.0f) {
		const float start = static_cast<float>(t * rRule.OneUnitDots) + GetRuleWidth() - offs;
		for(uint i = 0; i < nc; i++) {
			RuleNotch notch = rRule.NotchList.at(i);
			if(kind == 0 || (kind == 1 && notch.H == 1.0f) || (kind == 2 && notch.H >= 0.5f && notch.H < 1.0f)) {
				notch.P += start;
				rList.insert(&notch);
			}
		}
	}
	return ok;
}

int TWhatman::CalcScrollRange()
{
	TRect range;
	{
		const TRect & r_b = Area;
		SETMIN(range.a.x, MIN(r_b.a.x, r_b.b.x));
		SETMIN(range.a.y, MIN(r_b.a.y, r_b.b.y));
		SETMAX(range.b.x, MAX(r_b.a.x, r_b.b.x));
		SETMAX(range.b.y, MAX(r_b.a.y, r_b.b.y));
	}
	const uint c = ObjList.getCount();
	for(uint i = 0; i < c; i++) {
		TWhatmanObject * p_obj = ObjList.at(i);
		if(p_obj) {
			const TRect & r_b = p_obj->Bounds;
			SETMIN(range.a.x, MIN(r_b.a.x, r_b.b.x));
			SETMIN(range.a.y, MIN(r_b.a.y, r_b.b.y));
			SETMAX(range.b.x, MAX(r_b.a.x, r_b.b.x));
			SETMAX(range.b.y, MAX(r_b.a.y, r_b.b.y));
		}
	}
	range.setmarginx(P.ScrollMargin);
	range.setmarginy(P.ScrollMargin);
	ScrollRange = range;
	return 1;
}

int TWhatman::Draw(TCanvas2 & rCanv)
{
	int    ok = 1;
	uint   i;
	uint   nc;  // Временная переменная, используемая для хранения количества элементов в notch_list (для ускорения циклов)
	FRect  r;
	FRect  hrr; // Область горизонтальной линейки
	FRect  vrr; // Область вертикальной линейки
	LMatrix2D mtx;
	FPoint notch_area;
	FPoint notch_offs;
	TSVector <RuleNotch> notch_list; // @v9.8.4 TSArray-->TSVector
	TCanvas2::Capability caps;
	rCanv.GetCapability(&caps);
	//
	// Расчет горизонтальной линейки
	//
	CalcRule(caps.PtPerInch.X, RuleX);
	hrr.a = Area.a;
	hrr.a.X += TWhatman::GetRuleWidth();
	hrr.b = Area.b;
	hrr.b.Y = hrr.a.Y + TWhatman::GetRuleWidth();
	if(P.Flags & Param::fRule)
		rCanv.Rect(hrr, TidPenRule, TidBrushRule);
	//
	// Расчет вертикальной линейки
	//
	CalcRule(caps.PtPerInch.Y, RuleY);
	vrr.a = Area.a;
	vrr.a.Y += TWhatman::GetRuleWidth();
	vrr.b = Area.b;
	vrr.b.X = vrr.a.X + TWhatman::GetRuleWidth();
	if(P.Flags & Param::fRule)
		rCanv.Rect(vrr, TidPenRule, TidBrushRule);
	//
	notch_area.Set(static_cast<float>(Area.width()), static_cast<float>(Area.height()));
	notch_offs = ScrollPos;
	//
	if(P.Flags & Param::fGrid) {
		{
			//
			// Горизонтальная решетка (основные линии)
			//
			GetNotchList(RuleX, notch_area.X, notch_offs.X, 1, notch_list);
			nc = notch_list.getCount();
			for(i = 0; i < nc; i++) {
				FPoint p(notch_list.at(i).P, hrr.b.Y);
				rCanv.MoveTo(p);
				p.Y = static_cast<float>(Area.height());
				rCanv.Line(p);
			}
			//
			// Вертикальная решетка (основные линии)
			//
			GetNotchList(RuleY, notch_area.Y, notch_offs.Y, 1, notch_list);
			nc = notch_list.getCount();
			for(i = 0; i < nc; i++) {
				FPoint p(vrr.b.X, notch_list.at(i).P);
				rCanv.MoveTo(p);
				p.X = static_cast<float>(Area.width());
				rCanv.Line(p);
			}
			//
			// Отрисовываем решетку (основные линии) горизонтальной и вертикальной линеек сразу
			//
			rCanv.Stroke(TidPenGrid, 0);
		}
		{
			//
			// Горизонтальная решетка (вспомогательные линии)
			//
			GetNotchList(RuleX, notch_area.X, notch_offs.X, 2, notch_list);
			nc = notch_list.getCount();
			for(i = 0; i < nc; i++) {
				FPoint p(notch_list.at(i).P, hrr.b.Y);
				rCanv.MoveTo(p);
				p.Y = static_cast<float>(Area.height());
				rCanv.Line(p);
			}
			//
			// Вертикальная решетка (вспомогательные линии)
			//
			GetNotchList(RuleY, notch_area.Y, notch_offs.Y, 2, notch_list);
			nc = notch_list.getCount();
			for(i = 0; i < nc; i++) {
				FPoint p(vrr.b.X, notch_list.at(i).P);
				rCanv.MoveTo(p);
				p.X = static_cast<float>(Area.width());
				rCanv.Line(p);
			}
			//
			// Отрисовываем решетку (вспомогательные линии) горизонтальной и вертикальной линеек сразу
			//
			rCanv.Stroke(TidPenSubGrid, 0);
		}
	}
	//
	// Отрисовка объектов
	//
	rCanv.PushTransform(); // {
	if(ScrollPos.x || ScrollPos.y) {
		mtx.InitTranslate(-ScrollPos.x, -ScrollPos.y);
		rCanv.SetTransform(mtx);
	}
	//
	{
		STextLayout tlo;
		SDrawContext dctx = rCanv;
		const uint oc = ObjList.getCount();
		for(i = 0; i < oc; i++) {
			TWhatmanObject * p_obj = ObjList.at(i);
			if(p_obj) {
				DrawObject(rCanv, p_obj);
				if(p_obj->GetTextLayout(tlo, 0) > 0) {
					tlo.Arrange(dctx, rCanv.GetToolBox());
					rCanv.DrawTextLayout(&tlo);
				}
			}
		}
	}
	rCanv.PopTransform(); // }
	if(P.Flags & Param::fRule) {
		//
		// Засечки горизонтальной линейки
		//
		GetNotchList(RuleX, notch_area.X, notch_offs.X, 0, notch_list);
		nc = notch_list.getCount();
		for(i = 0; i < nc; i++) {
			const RuleNotch & r_n = notch_list.at(i);
			FPoint p(r_n.P, hrr.b.Y);
			rCanv.MoveTo(p);
			p.Y = hrr.b.Y - r_n.H * TWhatman::GetRuleWidth();
			rCanv.Line(p);
		}
		//
		// Засечки вертикальной линейки
		//
		GetNotchList(RuleY, notch_area.Y, notch_offs.Y, 0, notch_list);
		nc = notch_list.getCount();
		for(i = 0; i < nc; i++) {
			const RuleNotch & r_n = notch_list.at(i);
			FPoint p(vrr.b.X, r_n.P);
			rCanv.MoveTo(p);
			p.X = vrr.b.X - r_n.H * TWhatman::GetRuleWidth();
			rCanv.Line(p);
		}
		//
		// Отрисовываем засечки горизонтальной и вертикальной линеек сразу
		//
		rCanv.Stroke(TidPenRule, 0);
		//
		// Область между линейками в левом-верхнем углу
		//
	}
	if(!SelArea.IsEmpty()) {
		FRect fr(SelArea);
		rCanv.Rect(fr.Grow(0.5f, 0.5f), TidPenRule, 0);
	}
	return ok;
}

void TWhatman::Rule::Init()
{
	OneUnitLog10 = 10.0;
	OneUnitDots = 10.0;
	ScrollDelta = 0;
	NotchList.clear();
}

int TWhatman::Rule::AddNotch(float p, float h, long f)
{
	RuleNotch n;
	n.P = p;
	n.H = h;
	n.F = f;
	if(h >= 0.5f)
		n.F |= RuleNotch::fSnap;
	return NotchList.insert(&n) ? 1 : 0;
}


int TWhatman::Rule::AddNotchList(uint c, float subDelta, const float * pHiList)
{
	for(uint i = 1; i < c; i++) {
		float h = pHiList[i-1];
		AddNotch(static_cast<float>(round(i * subDelta, 1.0, 0)), h, 0);
	}
	return 1;
}

int TWhatman::Rule::GetNearest(float p, float size, float * pNearestP) const
{
	int    ok = -1;
	const  uint nc = NotchList.getCount();
	uint   lower_idx = 0;
	float  lower_p = 0.0f;
	const float limit = static_cast<float>((size / OneUnitDots) + 1.0f);
	for(float t = 0.0f; ok < 0 && t < limit; t += 1.0f) {
		const float start = static_cast<float>(t * OneUnitDots) + TWhatman::GetRuleWidth() - 0.5f;
		for(uint i = 0; ok < 0 && i < nc; i++) {
			const RuleNotch & r_n = NotchList.at(i);
			if(r_n.F & RuleNotch::fSnap) {
				float np = r_n.P + start;
				if(p == np) {
					ok = 2;
				}
				else if(lower_idx && p < np) {
					p = ((p - lower_p) < (np - p)) ? lower_p : np;
					ok = 1;
				}
				else if(p > np) {
					lower_idx = i+1;
					lower_p = np;
				}
			}
		}
	}
	ASSIGN_PTR(pNearestP, p);
	return ok;
}

int TWhatman::CalcRule(double ptPerInch, Rule & rRule) const
{
	const double about_dots_per_unit = 100.0;
	const double min_notch_dots = 3.0;

	rRule.Init();

	double dots_per_unit = 1.0;
	if(P.Unit == UNIT_METER)
		dots_per_unit = ptPerInch / 25.4 * 1000.0 * P.UnitFactor;
	else if(P.Unit == UNIT_INCH)
		dots_per_unit = ptPerInch * P.UnitFactor;
	else if(P.Unit == UNIT_GR_PIXEL)
		dots_per_unit = P.UnitFactor;
	if(dots_per_unit != 1.0) {
		double min_delta = SMathConst::Max;
		double min_s = 0;
		for(double s = -12.0; s <= 12.0; s += 1.0) {
			double p = pow(10.0, s) * dots_per_unit;
			double delta = fabs(p - about_dots_per_unit);
			if(delta < min_delta) {
				min_delta = delta;
				min_s = s;
			}
		}
		rRule.OneUnitLog10 = min_s;
		rRule.OneUnitDots  = (dots_per_unit * pow(10.0, min_s));

		uint   nn = 1; // Количество засечек
		rRule.AddNotch(0.0f, 1.0f, 0);
		double nd = rRule.OneUnitDots / 20; // Расстояние в точках между засечками
		if(nd < min_notch_dots) {
			nd = rRule.OneUnitDots / 10;
			if(nd < min_notch_dots) {
				nd = rRule.OneUnitDots / 5;
				if(nd < min_notch_dots) {
					rRule.ScrollDelta = static_cast<int>(nd);
				}
				else {
					nn = 5;
					const float hl[4] = { 0.3f, 0.4f, 0.3f, 0.4f };
					rRule.AddNotchList(nn, static_cast<float>(rRule.OneUnitDots / nn), hl);
					rRule.ScrollDelta = static_cast<int>(2.0 * rRule.OneUnitDots / nn);
				}
			}
			else {
				nn = 10;
				const float hl[9] = { 0.3f, 0.4f, 0.3f, 0.4f, 0.75f, 0.4f, 0.3f, 0.4f, 0.3f };
				rRule.AddNotchList(nn, static_cast<float>(rRule.OneUnitDots / nn), hl);
				rRule.ScrollDelta = static_cast<int>(4.0 * rRule.OneUnitDots / nn);
			}
		}
		else {
			nn = 20;
			//                     1     2     3     4     5     6     7     8     9     10     11    12    13    14    15    16    17    18    19
			const float hl[19] = { 0.3f, 0.2f, 0.3f, 0.2f, 0.5f, 0.2f, 0.3f, 0.2f, 0.3f, 0.75f, 0.3f, 0.2f, 0.3f, 0.2f, 0.5f, 0.2f, 0.3f, 0.2f, 0.3f };
			rRule.AddNotchList(nn, static_cast<float>(rRule.OneUnitDots / nn), hl);
			rRule.ScrollDelta = static_cast<int>(8.0 * rRule.OneUnitDots / nn);
		}
	}
	return 1;
}

struct LocalLoEntry {
	LocalLoEntry(const WhatmanObjectLayoutBase * pLo) : Flags(fIsLayout), P_Lo(pLo), P_ParentLo(0), LayId(static_cast<lay_id>(-1))
	{
		if(pLo) {
			STRNSCPY(LoSymb, pLo->GetLayoutContainerIdent());
		}
		else {
			PTR32(LoSymb)[0] = 0;
		}
		PTR32(ParentLoSymb)[0] = 0;
	}
	LocalLoEntry(const TWhatmanObject * pObj) : Flags(0), P_Obj(pObj), P_ParentLo(0), LayId(static_cast<lay_id>(-1))
	{
		PTR32(LoSymb)[0] = 0;
		PTR32(ParentLoSymb)[0] = 0;
	}
	enum {
		fIsLayout    = 0x0001,
		fHasChildren = 0x0002
	};
	int    Flags;
	lay_id LayId;
	union {
		const TWhatmanObject * P_Obj;
		const WhatmanObjectLayoutBase * P_Lo;
	};
	const  WhatmanObjectLayoutBase * P_ParentLo;
	char   LoSymb[64];
	char   ParentLoSymb[64];
};

static IMPL_CMPFUNC(LocalLoEntry_LoSymb, p1, p2)
{
	const LocalLoEntry * p_entry1 = static_cast<const LocalLoEntry *>(p1);
	const LocalLoEntry * p_entry2 = static_cast<const LocalLoEntry *>(p2);
	return strcmp(p_entry1->LoSymb, p_entry2->LoSymb);
}

int TWhatman::ProcessLayouts()
{
	int    ok = -1;
	{
		struct ObjTreeAssocEntry {
			const TWhatmanObject * P_Obj;
			uint32 TreeP;
		};
		SVector obj_tree_assoc(sizeof(ObjTreeAssocEntry));
		STree layout_tree(sizeof(LocalLoEntry));
		for(uint i = 0; i < ObjList.getCount(); i++) {
			const TWhatmanObject * p_iter_obj = ObjList.at(i);
			if(p_iter_obj) {
				if(p_iter_obj->Symb.IsEqiAscii("Layout")) {
					const WhatmanObjectLayoutBase * p_iter_layout_obj = static_cast<const WhatmanObjectLayoutBase *>(p_iter_obj);
					LocalLoEntry entry(p_iter_layout_obj);
					STRNSCPY(entry.ParentLoSymb, p_iter_obj->GetLayoutContainerIdent());
					uint32 np = 0;
					THROW(layout_tree.Insert(&entry, 0/*parent*/, &np));
					{
						ObjTreeAssocEntry ae;
						ae.P_Obj = p_iter_obj;
						ae.TreeP = np;
						obj_tree_assoc.insert(&ae);
					}
					//layout_list.insert(&entry);
					//p_iter_layout_obj->GetContainerIdent()
				}
				else if(p_iter_obj->GetLayoutContainerIdent().NotEmpty()) {
					LocalLoEntry entry(p_iter_obj);
					STRNSCPY(entry.ParentLoSymb, p_iter_obj->GetLayoutContainerIdent());
					uint32 np = 0;
					THROW(layout_tree.Insert(&entry, 0/*parent*/, &np));
					{
						ObjTreeAssocEntry ae;
						ae.P_Obj = p_iter_obj;
						ae.TreeP = np;
						obj_tree_assoc.insert(&ae);
					}
					//layout_list.insert(&entry);
				}
			}
		}
		for(int stop_iter = 0; !stop_iter;) {
			stop_iter = 1;
			for(STree::Iter iter; layout_tree.Enum(iter) > 0;) {
				LocalLoEntry * p_entry = static_cast<LocalLoEntry *>(iter.GetData());
				if(p_entry->ParentLoSymb[0]) {
					LocalLoEntry key(static_cast<const WhatmanObjectLayoutBase *>(0));
					STRNSCPY(key.LoSymb, p_entry->ParentLoSymb);
					uint32 parent_p = 0;
					if(layout_tree.Search(&key, &parent_p, PTR_CMPFUNC(LocalLoEntry_LoSymb))) {
						if(layout_tree.UpdateNodeParent(iter.GetCurrentPos(), parent_p) > 0) {
							//
							// Мы нашли правильного родителя для элемента iter и успешно переместили его в дереве.
							// Теперь нам надо выпрыгнуть из цикла перебора и начать все снова, поскольку
							// итератор может запутаться из-за внесенных изменений.
							//
							stop_iter = 0;
							break;
						}
					}
					else {
						//
						// Родительский элемент по символу не идентифицирован - выбрасываем объект из layout-дерева 
						// После этого необходимо выскочить из цикла итератора и возобновить перебор из-за риска 
						// нарушения работы итератора после модификации дерева.
						//
						layout_tree.Delete(iter.GetCurrentPos());
						stop_iter = 0;
						break;
					}
				}
			}
		}
		{
			//
			// Дерево готово. Теперь по этому дереву строим TLayout
			//
			TLayout layout;
			LocalLoEntry * p_root_entry = 0;
			for(STree::Iter iter; layout_tree.Enum(iter) > 0;) {
				LocalLoEntry * p_entry = static_cast<LocalLoEntry *>(iter.GetData());
				//assert((p_entry->Flags & LocalLoEntry::fIsLayout) || iter.)
				assert(p_entry->LayId == static_cast<lay_id>(-1));
				p_entry->LayId = layout.CreateItem();
				const uint32 parent_p = iter.GetParentPos();
				LocalLoEntry * p_parent_entry = parent_p ? static_cast<LocalLoEntry *>(layout_tree.GetData(parent_p)) : 0;
				assert(!p_parent_entry || (p_parent_entry->LayId >= 0 && p_parent_entry->Flags & LocalLoEntry::fIsLayout));
				if(p_parent_entry) {
					layout.Insert(p_parent_entry->LayId, p_entry->LayId);
				}
				else {
					assert(p_root_entry == 0);
					p_root_entry = p_entry;
				}
				{
					const TLayout::EntryBlock & r_lo_blk = p_entry->P_Obj->GetLayoutBlock();
					layout.SetItemSizeXy(p_entry->LayId, r_lo_blk.Sz.x, r_lo_blk.Sz.y);
				}
				if(p_entry->Flags & LocalLoEntry::fIsLayout) {

				}
				else {
				}
			}
		}
	}
	{
		SArray layout_list(sizeof(LocalLoEntry));
		{
			for(uint i = 0; i < ObjList.getCount(); i++) {
				const TWhatmanObject * p_iter_obj = ObjList.at(i);
				if(p_iter_obj) {
					if(p_iter_obj->Symb.IsEqiAscii("Layout")) {
						const WhatmanObjectLayoutBase * p_iter_layout_obj = static_cast<const WhatmanObjectLayoutBase *>(p_iter_obj);
						LocalLoEntry entry(p_iter_layout_obj);
						STRNSCPY(entry.ParentLoSymb, p_iter_obj->GetLayoutContainerIdent());
						layout_list.insert(&entry);
						//p_iter_layout_obj->GetContainerIdent()
					}
					else if(p_iter_obj->GetLayoutContainerIdent().NotEmpty()) {
						LocalLoEntry entry(p_iter_obj);
						STRNSCPY(entry.ParentLoSymb, p_iter_obj->GetLayoutContainerIdent());
						layout_list.insert(&entry);
					}
				}
			}
		}
		if(layout_list.getCount()) {
			TLayout layout;
			for(uint i = 0; i < layout_list.getCount(); i++) {
				LocalLoEntry * p_entry = static_cast<LocalLoEntry *>(layout_list.at(i));
				if(!p_entry->P_ParentLo && p_entry->ParentLoSymb[0]) {
					for(uint j = 0; !p_entry->P_ParentLo && j < layout_list.getCount(); j++) {
						LocalLoEntry * p_lo_entry = static_cast<LocalLoEntry *>(layout_list.at(j));
						if((p_lo_entry->Flags & LocalLoEntry::fIsLayout) && p_lo_entry->P_Lo->GetContainerIdent().IsEqual(p_entry->ParentLoSymb)) {
							p_entry->P_ParentLo = p_lo_entry->P_Lo;
							p_lo_entry->Flags |= LocalLoEntry::fHasChildren;
						}
					}
				}
			}
			uint lidx = layout_list.getCount();
			if(lidx) do {
				LocalLoEntry * p_entry = static_cast<LocalLoEntry *>(layout_list.at(--lidx));
				if(!p_entry->P_ParentLo && !(p_entry->Flags & LocalLoEntry::fIsLayout)) {
					layout_list.atFree(lidx);
				}
			} while(lidx);
		}
	}
	CATCHZOK
	return ok;
}
//
//
//
TWhatmanToolArray::Param::Param() : Flags(0)
{
	PicSize = 32;
}

TWhatmanToolArray::Item::Item(const TWhatmanToolArray * pOwner) : Id(0), Flags(0), P_Owner(pOwner), ExtSize(0)
{
	FigSize.Set(16, 16);
	PicSize.Set(16, 16);
	memzero(ExtData, sizeof(ExtData));
}

TWhatmanToolArray::TWhatmanToolArray() : SVector(sizeof(TWhatmanToolArray::Entry)), SrcFileVer(0) // @v9.8.5 SArray-->SVector
	// @v9.1.9 В метод Init инициализацию SrcFileVer вставлять нельзя - он вызывается после чтения файла
{
	Init();
}

TWhatmanToolArray::~TWhatmanToolArray()
{
}

TWhatmanToolArray & TWhatmanToolArray::Init()
{
	Pool.clear();
	Pool.add("$");
	SymbP = 0;
	TextP = 0;
	FileP = 0;
	Flags = 0;
	PicSize = 32;
	Ap.Init(DIREC_HORZ);
	Dg.Clear();
	return *this;
}

int TWhatmanToolArray::SetParam(const Param & rP)
{
	int    ok = 1;
	SString symb, text, file;
	Pool.getnz(SymbP, symb);
	Pool.getnz(TextP, text);
	Pool.getnz(FileP, file);
	if(symb != rP.Symb) {
		symb = rP.Symb;
		uint pos = 0;
		if(symb.NotEmptyS())
			Pool.add(symb, &pos);
		SymbP = pos;
	}
	if(text != rP.Text) {
		text = rP.Text;
		uint pos = 0;
		if(text.NotEmptyS())
			Pool.add(text, &pos);
		TextP = pos;
	}
	if(file.CmpNC(rP.FileName) != 0) {
		file = rP.FileName;
		uint pos = 0;
		if(file.NotEmptyS())
			Pool.add(file, &pos);
		FileP = pos;
	}
	Flags = rP.Flags;
	PicSize = rP.PicSize;
	Ap = rP.Ap;
	return ok;
}

void TWhatmanToolArray::GetParam(Param & rP) const
{
	Pool.getnz(SymbP, rP.Symb);
	Pool.getnz(TextP, rP.Text);
	Pool.getnz(FileP, rP.FileName);
	rP.Flags = Flags;
	rP.PicSize = PicSize;
	rP.Ap = Ap;
}

uint TWhatmanToolArray::GetCount() const
{
	return SVector::getCount();
}

SString & TWhatmanToolArray::MakeUniqueSymb(SString & rBuf) const
{
	do {
		rBuf.Z().CatChar('S').Cat(SLS.GetTLA().Rg.GetUniformInt(9999999));
	} while(SearchBySymb(rBuf, 0));
	return rBuf;
}

int TWhatmanToolArray::SearchBySymb(const char * pSymb, uint * pPos) const
{
	int    ok = 0;
	uint   idx = 0;
	uint   pos = 0;
	while(!ok && Pool.search(pSymb, &pos, 1)) {
		if(lsearch(&pos, &idx, CMPF_LONG, offsetof(Entry, SymbP)))
			ok = 1;
		else {
			idx = 0;
			pos++;
		}
	}
	ASSIGN_PTR(pPos, idx);
	return ok;
}

int TWhatmanToolArray::SearchById(uint id, uint * pPos) const
{
	int    ok = 0;
	uint   pos = 0;
    if(id && lsearch(&id, &pos, CMPF_LONG, offsetof(Entry, Id))) {
		ok = 1;
    }
	ASSIGN_PTR(pPos, pos);
	return ok;
}

int TWhatmanToolArray::Remove(uint pos)
{
	return atFree(pos);
}

int TWhatmanToolArray::CreateFigure(const Item & rItem, const char * pPath, int pic)
{
	int    ok = 1;
	SDrawFigure * p_fig = 0;
	SFileFormat fmt;
	SString sid;
	(sid = rItem.Symb).CatChar('-').Cat(pic ? "PIC" : "FIG");
	THROW(p_fig = SDrawFigure::CreateFromFile(pPath, sid));
	if(pic) {
		SDrawImage * p_img_fig = p_fig->DupToImage(rItem.PicSize, 0, sid);
		THROW(p_img_fig);
		DELETEANDASSIGN(p_fig, p_img_fig);
	}
	THROW(Dg.Remove(sid, 0));
	THROW(Dg.Add(p_fig));
	CATCH
		ZDELETE(p_fig);
		ok = 0;
	ENDCATCH
	return ok;
}

int TWhatmanToolArray::UpdateFigures(Item & rItem)
{
	int    ok = 1;
	int    pic_def = 0;
	if(rItem.FigPath.NotEmptyS()) {
		THROW(CreateFigure(rItem, rItem.FigPath, 0));
		pic_def |= 0x01;
	}
	if(rItem.PicPath.NotEmptyS()) {
		THROW(CreateFigure(rItem, rItem.PicPath, 1));
		pic_def |= 0x02;
	}
	THROW_S_S(pic_def, SLERR_WTMTA_UNDEFFIG, rItem.Symb);
	if(!(pic_def & 0x01)) {
		//
		// Если не определен путь до фигуры, то вместо фигуры используем иконку
		//
		THROW(CreateFigure(rItem, rItem.PicPath, 0));
	}
	if(!(pic_def & 0x02)) {
		//
		// Если не определен путь до иконки, то вместо иконки используем фигуру
		//
		THROW(CreateFigure(rItem, rItem.FigPath, 1));
	}
	CATCHZOK
	return ok;
}

int TWhatmanToolArray::Set(Item & rItem, uint * pPos)
{
	int    ok = 1;
	int    is_exists = 0;
	uint   pos = 0;
	uint   pos_by_id = 0;
	Entry  entry;
	SString temp_buf;
	if(!rItem.Symb.NotEmptyS()) {
		if(rItem.Id && SearchById(rItem.Id, &pos_by_id)) {
			Entry & r_entry = *static_cast<Entry *>(at(pos_by_id));
			Pool.getnz(r_entry.SymbP, rItem.Symb);
			pos = pos_by_id;
			is_exists = 1;
		}
		else {
			MakeUniqueSymb(rItem.Symb);
		}
	}
	else if(SearchBySymb(rItem.Symb, &pos)) {
		if(rItem.Id && SearchById(rItem.Id, &pos_by_id)) {
			THROW_S_S(pos_by_id == pos, SLERR_DUPSYMBWITHUNEQID, rItem.Symb);
		}
		is_exists = 1;
	}
	else {
		THROW_S_S(!rItem.Id || !SearchById(rItem.Id, &pos_by_id), SLERR_DUPSYMBWITHUNEQID, rItem.Symb);
	}
	if(is_exists) {
		int    upd = 0;
		Entry & r_entry = *static_cast<Entry *>(at(pos));
		Item ex_item(this);
		Get(pos, &ex_item);
		if(ex_item.Id != rItem.Id) {
			r_entry.Id = rItem.Id;
			upd = 1;
		}
		if(ex_item.Text != rItem.Text) {
			if(rItem.Text.NotEmpty())
				Pool.add(rItem.Text, reinterpret_cast<uint *>(&r_entry.TextP));
			else
				r_entry.TextP = 0;
			upd = 1;
		}
		if(ex_item.WtmObjSymb != rItem.WtmObjSymb) {
			if(rItem.WtmObjSymb.NotEmpty())
				Pool.add(rItem.WtmObjSymb, reinterpret_cast<uint *>(&r_entry.WtmObjSymbP));
			else
				r_entry.WtmObjSymbP = 0;
			upd = 1;
		}
		if(ex_item.Flags != rItem.Flags) {
			r_entry.Flags = rItem.Flags;
			upd = 1;
		}
		if(ex_item.FigPath != rItem.FigPath) {
			if(rItem.FigPath.NotEmpty())
				Pool.add(rItem.FigPath, reinterpret_cast<uint *>(&r_entry.FigPathP));
			else
				r_entry.FigPathP = 0;
			upd = 1;
		}
		if(ex_item.PicPath != rItem.PicPath) {
			if(rItem.PicPath.NotEmpty())
				Pool.add(rItem.PicPath, reinterpret_cast<uint *>(&r_entry.PicPathP));
			else
				r_entry.PicPathP = 0;
			upd = 1;
		}
		{
			const uint32 extsz = rItem.ExtSize;
			if(ex_item.ExtSize != extsz || (extsz && memcmp(ex_item.ExtData, rItem.ExtData, extsz) != 0)) {
				if(extsz) {
					temp_buf.Z().EncodeMime64(rItem.ExtData, extsz);
					Pool.add(temp_buf, reinterpret_cast<uint *>(&r_entry.ExtDataP));
				}
				else
					r_entry.ExtDataP = 0;
				upd = 1;
			}
		}
		if(r_entry.FigSize != rItem.FigSize) {
			r_entry.FigSize = rItem.FigSize;
			upd = 1;
		}
		if(r_entry.PicSize != rItem.PicSize) {
			r_entry.PicSize = rItem.PicSize;
			upd = 1;
		}
		// @v9.2.7 {
		if(r_entry.ReplacedColor != rItem.ReplacedColor) {
			r_entry.ReplacedColor = rItem.ReplacedColor;
			upd = 1;
		}
		// } @v9.2.7
		if(!upd)
			ok = -1;
		else {
			THROW(UpdateFigures(rItem));
		}
	}
	else {
		MEMSZERO(entry);
		if(rItem.Symb.NotEmpty())
			Pool.add(rItem.Symb, &entry.SymbP);
		if(rItem.Text.NotEmpty())
			Pool.add(rItem.Text, &entry.TextP);
		if(rItem.WtmObjSymb.NotEmpty())
			Pool.add(rItem.WtmObjSymb, &entry.WtmObjSymbP);
		if(rItem.FigPath.NotEmpty())
			Pool.add(rItem.FigPath, &entry.FigPathP);
		if(rItem.PicPath.NotEmpty())
			Pool.add(rItem.PicPath, &entry.PicPathP);
		if(rItem.ExtSize) {
			temp_buf.Z().EncodeMime64(rItem.ExtData, rItem.ExtSize);
			Pool.add(temp_buf, &entry.ExtDataP);
		}
		THROW(UpdateFigures(rItem));
		entry.FigSize = rItem.FigSize;
		entry.PicSize = rItem.PicSize;
		entry.Flags = rItem.Flags;
		entry.Id = rItem.Id;
		entry.ReplacedColor = rItem.ReplacedColor; // @v9.2.7
		pos = getCount();
		THROW(insert(&entry));
	}
	CATCHZOK
	ASSIGN_PTR(pPos, pos);
	return ok;
}

int TWhatmanToolArray::Get(uint pos, Item * pItem) const
{
	int    ok = 1;
	Item item(this);
	SString temp_buf;
	THROW_S(pos < getCount(), SLERR_BOUNDS);
	const Entry & r_entry = *static_cast<const Entry *>(at(pos));
	Pool.getnz(r_entry.TextP, item.Text);
	Pool.getnz(r_entry.SymbP, item.Symb);
	Pool.getnz(r_entry.WtmObjSymbP, item.WtmObjSymb);
	Pool.getnz(r_entry.FigPathP, item.FigPath);
	Pool.getnz(r_entry.PicPathP, item.PicPath);
	Pool.getnz(r_entry.ExtDataP, temp_buf);
	if(temp_buf.NotEmpty()) {
		size_t sz = 0;
		temp_buf.DecodeMime64(item.ExtData, sizeof(item.ExtData), &sz);
		THROW_S(sz <= sizeof(item.ExtData), SLERR_BUFTOOSMALL);
		item.ExtSize = (uint32)sz;
	}
	item.FigSize = r_entry.FigSize;
	item.PicSize = r_entry.PicSize;
	item.Flags = r_entry.Flags;
	item.Id = r_entry.Id;
	item.ReplacedColor = r_entry.ReplacedColor; // @v9.2.7
	CATCHZOK
	ASSIGN_PTR(pItem, item);
	return ok;
}

int TWhatmanToolArray::GetBySymb(const char * pSymb, Item * pItem) const
{
	uint   pos = 0;
	return SearchBySymb(pSymb, &pos) ? Get(pos, pItem) : 0;
}

const SDrawFigure * TWhatmanToolArray::GetFig(int figOrPic, uint pos, TWhatmanToolArray::Item * pItem) const
{
	const SDrawFigure * p_result = 0;
	SString symb;
	Item   item(this);
	if(Get(pos, &item)) {
		p_result = Dg.Find((symb = item.Symb).CatChar('-').Cat(figOrPic ? "FIG" : "PIC"), 0);
		if(p_result)
			ASSIGN_PTR(pItem, item);
	}
	return p_result;
}

const SDrawFigure * TWhatmanToolArray::GetFig(int figOrPic, const char * pSymb, TWhatmanToolArray::Item * pItem) const
{
	uint   pos = 0;
	return SearchBySymb(pSymb, &pos) ? GetFig(figOrPic, pos, pItem) : 0;
}

const SDrawFigure * TWhatmanToolArray::GetFigById(int figOrPic, uint id, TWhatmanToolArray::Item * pItem) const
{
	uint   pos = 0;
	return SearchById(id, &pos) ? GetFig(figOrPic, pos, pItem) : 0;
}

int TWhatmanToolArray::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	THROW(pCtx->Serialize(dir, (SVector *)this, rBuf)); // @v9.8.5 SArray-->SVector
	THROW(pCtx->Serialize(dir, SymbP, rBuf));
	THROW(pCtx->Serialize(dir, TextP, rBuf));
	THROW(pCtx->Serialize(dir, Flags, rBuf));
	THROW(pCtx->Serialize(dir, PicSize, rBuf));
	THROW(Ap.Serialize(dir, rBuf, pCtx));
	THROW(Pool.Serialize(dir, rBuf, pCtx));
	THROW(Dg.Serialize(dir, rBuf, pCtx));
	CATCHZOK
	return ok;
}
//
//
//
struct WtHeader {
	uint32 Signature;
	uint32 Crc;
	uint32 Lock;
	uint32 Ver;
	uint8  Reserve[16];
};

#define WTA_SIGN 0x00415457
#define WTM_SIGN 0x004D5457
#define WTA_VER  0
#define WTM_VER  1 // @v10.4.7 0-->1

static int StoreWtBuffer(const char * pFileName, const SBuffer & rBuf, uint32 sign, uint32 ver)
{
	int    ok = 1;
	WtHeader hdr;
	SFile f(pFileName, SFile::mReadWriteTrunc|SFile::mBinary);
	THROW(f.IsValid());
	MEMSZERO(hdr);
	hdr.Signature = sign;
	hdr.Ver = ver;
	THROW(f.Write(&hdr, sizeof(hdr)));
	THROW(f.Write(rBuf));
	THROW(f.CalcCRC(offsetof(WtHeader, Ver), &hdr.Crc));
	f.Seek(0);
	THROW(f.Write(&hdr, sizeof(hdr)));
	CATCHZOK
	return ok;
}

static int LoadWtBuffer(const char * pFileName, SBuffer & rBuf, uint32 sign, uint32 * pVer)
{
	int    ok = 1;
	uint32 crc = 0;
	WtHeader hdr;
	SFile f(pFileName, SFile::mRead|SFile::mBinary);
	SLS.SetAddedMsgString(pFileName); // @v7.4.3
	THROW(f.IsValid());
	THROW(f.Read(&hdr, sizeof(hdr)));
	THROW_S_S(hdr.Signature == sign, SLERR_WTMTA_INVSIGNATURE, pFileName);
	THROW(f.CalcCRC(offsetof(WtHeader, Ver), &crc));
	THROW_S_S(crc == hdr.Crc, SLERR_WTMTA_BADCRC, pFileName);
	THROW(f.Read(rBuf));
	CATCHZOK
	return ok;
}

int TWhatmanToolArray::Store(const char * pFileName)
{
	int    ok = 1;
	SSerializeContext ctx;
	SBuffer buf;
	THROW(Serialize(+1, buf, &ctx));
	THROW(StoreWtBuffer(pFileName, buf, WTA_SIGN, WTA_VER));
	CATCHZOK
	return ok;
}

int TWhatmanToolArray::Load(const char * pFileName)
{
	int    ok = 1;
	SSerializeContext ctx;
	SBuffer buf;
	THROW(LoadWtBuffer(pFileName, buf, WTA_SIGN, &SrcFileVer));
	THROW(Init().Serialize(-1, buf, &ctx));
	{
		SString file;
		Pool.getnz(FileP, file);
		if(file.CmpNC(pFileName) != 0) {
			file = pFileName;
			uint pos = 0;
			if(file.NotEmptyS())
				Pool.add(file, &pos);
			FileP = pos;
		}
	}
	CATCHZOK
	return ok;
}

int TWhatman::Store(const char * pFileName)
{
	int    ok = 1;
	SSerializeContext ctx;
	SBuffer buf;
	THROW(Serialize(+1, buf, &ctx));
	THROW(StoreWtBuffer(pFileName, buf, WTM_SIGN, WTM_VER));
	CATCHZOK
	return ok;
}

int TWhatman::Load(const char * pFileName)
{
	int    ok = 1;
	SSerializeContext ctx;
	SBuffer buf;
	THROW(LoadWtBuffer(pFileName, buf, WTM_SIGN, &SrcFileVer));
	THROW(Serialize(-1, buf, &ctx));
	CATCHZOK
	return ok;
}


/*
int TWhatmanToolArray::LockStorage(const char * pFileName)
{
}
*/
