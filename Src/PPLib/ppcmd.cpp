// PPCMD.CPP
// Copyright (c) A.Sobolev 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019
// @Kernel
//
#include <pp.h>
#pragma hdrstop
//
// @ModuleDef(PPCommand)
//
//static
const char * PPCommandDescr::P_FactoryPrfx = "CFF_";

SLAPI PPCommandDescr::PPCommandDescr()
{
	Init();
}

void SLAPI PPCommandDescr::Init()
{
	CmdID = 0;
	Flags = 0;
	MenuCm = 0;
	IconId = 0;
	ToolbarId = 0;
	ViewId = 0; // @v10.3.8 @fix
	FiltId = 0; // @v10.3.8 @fix
	FiltExtId = 0; // @v10.3.8 @fix
	memzero(&Reserve, sizeof(Reserve));
	Symb.Z();
	Text.Z();
}

SString & FASTCALL PPCommandDescr::GetFactoryFuncName(SString & rBuf) const
{
	(rBuf = P_FactoryPrfx).Cat(Symb).ToUpper();
	return rBuf;
}

int SLAPI PPCommandDescr::Write(SBuffer & rBuf, long) const
{
	int    ok = 1;
	THROW_SL(rBuf.Write(CmdID));
	THROW_SL(rBuf.Write(Flags));
	THROW_SL(rBuf.Write(MenuCm));
	THROW_SL(rBuf.Write(IconId));
	THROW_SL(rBuf.Write(ToolbarId));
	THROW_SL(rBuf.Write(ViewId));
	THROW_SL(rBuf.Write(FiltId));
	THROW_SL(rBuf.Write(FiltExtId));
	THROW_SL(rBuf.Write(Reserve));
	THROW_SL(rBuf.Write(Symb));
	THROW_SL(rBuf.Write(Text));
	CATCHZOK
	return ok;
}

int SLAPI PPCommandDescr::Read(SBuffer & rBuf, long)
{
	int    ok = 1;
	THROW_SL(rBuf.Read(CmdID));
	THROW_SL(rBuf.Read(Flags));
	THROW_SL(rBuf.Read(MenuCm));
	THROW_SL(rBuf.Read(IconId));
	THROW_SL(rBuf.Read(ToolbarId));
	THROW_SL(rBuf.Read(ViewId));
	THROW_SL(rBuf.Read(FiltId));
	THROW_SL(rBuf.Read(FiltExtId));
	THROW_SL(rBuf.Read(Reserve));
	THROW_SL(rBuf.Read(Symb));
	THROW_SL(rBuf.Read(Text));
	CATCHZOK
	return ok;
}

int SLAPI PPCommandDescr::LoadResource(long cmdDescrID)
{
	int    ok = 1;
	TVRez * p_rez = P_SlRez;
	Init();
	if(p_rez) {
		THROW_PP(p_rez->findResource((uint)cmdDescrID, PP_RCDECLCMD), PPERR_RESFAULT);
		CmdID = cmdDescrID;
		p_rez->getString(Symb, 2);
		p_rez->getString(Text, 2);
		SLS.ExpandString(Text, CTRANSF_UTF8_TO_INNER); // @v9.0.11
		IconId    = static_cast<long>(p_rez->getUINT());
		ToolbarId = static_cast<long>(p_rez->getUINT());
		MenuCm    = static_cast<long>(p_rez->getUINT());
		Flags     = static_cast<long>(p_rez->getUINT());
		ViewId    = static_cast<long>(p_rez->getUINT());
		FiltId    = static_cast<long>(p_rez->getUINT());
		FiltExtId = static_cast<long>(p_rez->getUINT());
	}
	CATCHZOK
	return ok;
}

// static
int SLAPI PPCommandDescr::GetResourceList(LAssocArray * pList)
{
	int    ok = 1;
	TVRez * p_rez = P_SlRez;
	pList->freeAll();
	if(p_rez) {
		ulong pos = 0;
		for(uint   rsc_id = 0; p_rez->enumResources(PP_RCDECLCMD, &rsc_id, &pos) > 0;) {
			PPCommandDescr descr;
			THROW(descr.LoadResource(rsc_id));
			THROW_SL(pList->Add(descr.CmdID, descr.MenuCm, 0, 0));
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PPCommandDescr::GetResourceList(int loadText, StrAssocArray * pList)
{
	int    ok = 1;
	TVRez * p_rez = P_SlRez;
	pList->Z();
	if(p_rez) {
		ulong pos = 0;
		for(uint   rsc_id = 0; p_rez->enumResources(PP_RCDECLCMD, &rsc_id, &pos) > 0;) {
			PPCommandDescr descr;
			THROW(descr.LoadResource(rsc_id));
			THROW_SL(pList->Add(descr.CmdID, loadText ? descr.Text : descr.Symb));
		}
	}
	CATCHZOK
	return ok;
}

PPCommandHandler * SLAPI PPCommandDescr::CreateInstance(long cmdDescrID)
{
	PPCommandHandler * p_h = 0;
	SString ffn;
	THROW(LoadResource(cmdDescrID));
	PPSetAddedMsgString(Text);
	FN_CMD_FACTORY f = reinterpret_cast<FN_CMD_FACTORY>(GetProcAddress(SLS.GetHInst(), GetFactoryFuncName(ffn)));
	if(!f && MenuCm) {
		SString def_factory_name;
		def_factory_name = P_FactoryPrfx;
		def_factory_name.Cat("DEFAULT").ToUpper();
		f = reinterpret_cast<FN_CMD_FACTORY>(GetProcAddress(SLS.GetHInst(), def_factory_name));
	}
	THROW(f);
	THROW(p_h = f(this));
	CATCH
		PPSetError(PPERR_JOBUNIMPL);
		p_h = 0;
	ENDCATCH
	return p_h;
}

int SLAPI PPCommandDescr::DoCommand(PPCommand * pCmd, void * extraPtr)
{
	int    ok = -1;
	PPCommandHandler * p_h = 0;
	if(pCmd) {
		p_h = CreateInstance(pCmd->CmdID);
		size_t sav_offs = pCmd->Param.GetRdOffs();
		if(p_h) {
			SBuffer * p_param = &pCmd->Param;
			SBuffer temp_param;
			int    r = 1;
			if(pCmd->Flags & PPCommandItem::fAllowEditFilt) {
				temp_param = pCmd->Param;
				r = p_h->EditParam(&temp_param, pCmd->ID, extraPtr);
				p_param = &temp_param;
			}
			if(r > 0)
				ok = p_h->Run(p_param, pCmd->ID, extraPtr);
		}
		else
			ok = 0;
		pCmd->Param.SetRdOffs(sav_offs);
	}
	delete p_h;
	return ok;
}

int SLAPI PPCommandDescr::DoCommandSimple(PPID cmdDescrID, int allowEditParam, const char * pTextFilt, void * extraPtr)
{
	int    ok = -1;
	SBuffer * p_filt_buf = 0;
	SBuffer filt_buf;
	PPCommandHandler * p_h = CreateInstance(cmdDescrID);
	THROW(p_h);
	if(pTextFilt) {
		SString temp_buf;
		temp_buf.Cat(PPBaseFilt::P_TextSignature).Space().Cat(pTextFilt);
		THROW_SL(filt_buf.Write(temp_buf.cptr(), temp_buf.Len()+1));
		p_filt_buf = &filt_buf;
	}
	{
		int    r = 1;
		if(allowEditParam) {
			SETIFZ(p_filt_buf, &filt_buf);
			r = p_h->EditParam(p_filt_buf, cmdDescrID, extraPtr);
		}
		if(r > 0)
			ok = p_h->Run(p_filt_buf, 0, extraPtr);
	}
	CATCHZOK
	delete p_h;
	return ok;
}

int SLAPI PPCommandDescr::EditCommandParam(PPID cmdDescrID, long cmdID, SBuffer * pParam, void * extraPtr)
{
	PPCommandHandler * p_h = CreateInstance(cmdDescrID);
	return p_h ? p_h->EditParam(pParam, cmdID, extraPtr) : 0;
}
//
//
//
SLAPI PPCommandItem::PPCommandItem(int kind) : Kind(kind), Flags(0), ID(0)
{
}

SLAPI PPCommandItem::~PPCommandItem()
{
}

int FASTCALL PPCommandItem::Copy(const PPCommandItem & s)
{
	Kind = s.Kind;
	Flags = s.Flags;
	ID = s.ID;
	Name = s.Name;
	Icon = s.Icon;
	return 1;
}

const PPCommandItem * SLAPI PPCommandItem::Next(uint * pPos) const
{
	return 0;
}

int SLAPI PPCommandItem::Enumerate(CmdItemIterFunc func, long parentID, void * extraPtr) const
{
	int    ok = 1;
	const PPCommandItem * p_item = 0;
	THROW(func(this, parentID, extraPtr));
	for(uint i = 0; (p_item = Next(&i)) != 0;) {
		THROW(p_item->Enumerate(func, ID, extraPtr));
	}
	CATCHZOK
	return ok;
}

PPCommandItem * SLAPI PPCommandItem::Dup() const
{
	PPCommandItem * p_item = new PPCommandItem;
	p_item->Copy(*this);
	return p_item;
}

struct _kf_block {
	_kf_block(const PPCommandItem * pItem) : Kind(pItem ? pItem->Kind : 0), Flags(pItem ? (pItem->Flags & ~PPCommandItem::fBkgndImageLoaded) : 0)
	{
	}
	int16  Kind;
	int16  Flags;
};

int SLAPI PPCommandItem::Write(SBuffer & rBuf, long) const
{
	int    ok = 1;
	_kf_block _kf(this);
	THROW_SL(rBuf.Write(&_kf, sizeof(_kf)));
	THROW_SL(rBuf.Write(ID));
	THROW_SL(rBuf.Write(Name));
	THROW_SL(rBuf.Write(Icon));
	CATCHZOK
	return ok;
}

int SLAPI PPCommandItem::Read(SBuffer & rBuf, long)
{
	int    ok = 1;
	_kf_block _kf(0);
	THROW_SL(rBuf.Read(&_kf, sizeof(_kf)));
	Kind = _kf.Kind;
	Flags = _kf.Flags;
	THROW_SL(rBuf.Read(ID));
	THROW_SL(rBuf.Read(Name));
	THROW_SL(rBuf.Read(Icon));
	CATCHZOK
	return ok;
}

// virtual
void SLAPI PPCommandItem::SetUniqueID(long * pID)
{
	ID = *pID;
	(*pID)++;
}
//
//
//
SLAPI PPCommand::PPCommand() : PPCommandItem(kCommand), CmdID(0)
{
	P = 0;
	memzero(Reserve, sizeof(Reserve));
}

int FASTCALL PPCommand::Copy(const PPCommand & s)
{
	PPCommandItem::Copy(s);
	CmdID = s.CmdID;
	P = s.P;
	memcpy(Reserve, s.Reserve, sizeof(Reserve));
	Param = s.Param;
	return 1;
}

PPCommandItem * SLAPI PPCommand::Dup() const
{
	PPCommand * p_item = new PPCommand;
	p_item->Copy(*this);
	return p_item;
}

int SLAPI PPCommand::Write(SBuffer & rBuf, long extraParam) const
{
	int    ok = 1;
	THROW(PPCommandItem::Write(rBuf, extraParam));
	THROW_SL(rBuf.Write(CmdID));
	//
	// @v9.0.11 ѕол€ X и Y заменены на TPoint
	// TPoint содержит x и y в том же пор€дке но используютс€ знаковые int16
	// (ранее X и Y были беззнаковыми uint16).
	// ¬еро€тнее всего замена
	// {
	// THROW(rBuf.Write(&X, sizeof(X)));
	// THROW(rBuf.Write(&Y, sizeof(Y)));
	// }
	// на THROW(rBuf.Write(&P, sizeof(P)));
	// не послечет каких-либо проблем
	//
	THROW_SL(rBuf.Write(&P, sizeof(P)));
	//
	THROW_SL(rBuf.Write(Reserve, sizeof(Reserve)));
	THROW_SL(rBuf.Write(Param));
	CATCHZOK
	return ok;
}

int SLAPI PPCommand::Read(SBuffer & rBuf, long extraParam)
{
	int    ok = 1;
	THROW(PPCommandItem::Read(rBuf, extraParam));
	THROW(rBuf.Read(CmdID));
	THROW(rBuf.Read(&P, sizeof(P))); // @v9.0.11 see comment in PPCommand::Write
	THROW(rBuf.Read(Reserve, sizeof(Reserve)));
	THROW(rBuf.Read(Param));
	CATCHZOK
	return ok;
}
//
//
//
SLAPI PPCommandFolder::PPCommandFolder() : PPCommandItem(kFolder)
{
}

static int _GetIdList(const PPCommandItem * pItem, long parentID, void * extraPtr)
{
	if(pItem)
		static_cast<PPIDArray *>(extraPtr)->addUnique(pItem->ID);
	return 1;
}

int SLAPI PPCommandFolder::GetUniqueID(long * pID) const
{
	long   id = 0;
	PPIDArray id_list;
	id_list.addUnique(ID);
	Enumerate(_GetIdList, 0, &id_list);
	id_list.sort();
	id = id_list.getLast() + 1;
	ASSIGN_PTR(pID, id);
	return 1;
}

// virtual
void SLAPI PPCommandFolder::SetUniqueID(long * pID)
{
	PPCommandItem::SetUniqueID(pID);
	for(uint i = 0; i < List.getCount(); i++)
		List.at(i)->SetUniqueID(pID);
}

int FASTCALL PPCommandFolder::Copy(const PPCommandFolder & s)
{
	int    ok = 1;
	PPCommandItem::Copy(s);
	List.freeAll();
	for(uint i = 0; i < s.GetCount(); i++) {
		PPCommandItem * p_item = s.List.at(i)->Dup();
		THROW_SL(List.insert(p_item));
	}
	CATCHZOK
	return ok;
}

const PPCommandItem * SLAPI PPCommandFolder::Next(uint * pPos) const
{
	if(*pPos < GetCount()) {
		(*pPos)++;
		return List.at((*pPos)-1);
	}
	else
		return 0;
}

int SLAPI PPCommandFolder::Write(SBuffer & rBuf, long extraParam) const
{
	int    ok = 1;
	uint16 c = 0, i;
	THROW(PPCommandItem::Write(rBuf, extraParam));
	c = List.getCount();
	THROW_SL(rBuf.Write(&c, sizeof(c)));
	for(i = 0; i < c; i++)
		THROW(List.at(i)->Write(rBuf, extraParam));
	CATCHZOK
	return ok;
}

int SLAPI PPCommandFolder::Read(SBuffer & rBuf, long extraParam)
{
	int    ok = 1;
	uint16 c = 0, i;
	THROW(PPCommandItem::Read(rBuf, extraParam));
	THROW_SL(rBuf.Read(&c, sizeof(c)));
	for(i = 0; i < c; i++) {
		//char * ptr = 0;
		PPCommandItem * ptr = 0;
		size_t offs = rBuf.GetRdOffs();
		PPCommandItem item;
		THROW(item.Read(rBuf, extraParam));
		rBuf.SetRdOffs(offs);
		if(item.Kind == PPCommandItem::kCommand) {
			ptr = /*(char *)*/new PPCommand;
			static_cast<PPCommand *>(ptr)->Read(rBuf, extraParam);
		}
		else if(item.Kind == PPCommandItem::kFolder) {
			ptr = /*(char *)*/new PPCommandFolder;
			static_cast<PPCommandFolder *>(ptr)->Read(rBuf, extraParam);
		}
		else if(item.Kind == PPCommandItem::kGroup) {
			ptr = /*(char *)*/new PPCommandGroup;
			static_cast<PPCommandGroup *>(ptr)->Read(rBuf, extraParam);
		}
		else if(item.Kind == PPCommandItem::kSeparator) {
			ptr = /*(char *)*/new PPCommandItem;
			static_cast<PPCommandItem *>(ptr)->Read(rBuf, extraParam);
		}
		THROW_SL(List.insert(/*(PPCommandItem*)*/ptr));
	}
	CATCHZOK
	return ok;
}

PPCommandFolder & FASTCALL PPCommandFolder::operator = (const PPCommandFolder & s)
{
	Copy(s);
	return *this;
}

PPCommandItem * SLAPI PPCommandFolder::Dup() const
{
	PPCommandFolder * p_item = new PPCommandFolder;
	p_item->Copy(*this);
	return p_item;
}

uint SLAPI PPCommandFolder::GetCount() const
{
	return List.getCount();
}

const PPCommandItem * SLAPI PPCommandFolder::Get(uint pos) const
{
	return (pos < GetCount()) ? List.at(pos) : 0;
}

int SLAPI PPCommandFolder::Update(uint pos, const PPCommandItem * pItem)
{
	int    ok = 1;
	const  uint c = GetCount();
	const  PPCommandGroup * p_new_grp = (pItem->Kind == PPCommandItem::kGroup) ? static_cast<const PPCommandGroup *>(pItem) : 0;
	THROW(pos < c);
	for(uint i = 0; i < c; i++) {
		if(i != pos) {
			const PPCommandItem * p = List.at(i);
			if(p && p->Kind == pItem->Kind) {
				const PPCommandGroup * p_grp = (p->Kind == PPCommandItem::kGroup) ? static_cast<const PPCommandGroup *>(p) : 0;
				if(p_new_grp && p_grp) {
					THROW_PP_S(pItem->Name.CmpNC(p->Name) != 0 || !p_new_grp->IsDbSymbEq(*p_grp), PPERR_DUPCMDNAME, pItem->Name);
				}
				else {
					THROW_PP_S(pItem->Name.CmpNC(p->Name) != 0, PPERR_DUPCMDNAME, pItem->Name);
				}
			}
		}
	}
	{
		PPCommandItem * p_item = pItem->Dup();
		THROW_MEM(p_item);
		delete List.at(pos);
		List.atPut(pos, p_item);
	}
	CATCHZOK
	return ok;
}

int SLAPI PPCommandFolder::Add(int pos, const PPCommandItem * pItem)
{
	int    ok = 1;
	THROW_INVARG(pItem);
	const  uint c = GetCount();
	const  PPCommandGroup * p_new_grp = (pItem->Kind == PPCommandItem::kGroup) ? static_cast<const PPCommandGroup *>(pItem) : 0;
	for(uint i = 0; i < c; i++) {
		const PPCommandItem * p = List.at(i);
		if(p && p->Kind == pItem->Kind) {
			const PPCommandGroup * p_grp = (p->Kind == PPCommandItem::kGroup) ? static_cast<const PPCommandGroup *>(p) : 0;
			if(p_new_grp && p_grp) {
				THROW_PP_S(pItem->Name.CmpNC(p->Name) != 0 || !p_new_grp->IsDbSymbEq(*p_grp), PPERR_DUPCMDNAME, pItem->Name);
			}
			else {
				THROW_PP_S(pItem->Name.CmpNC(p->Name) != 0, PPERR_DUPCMDNAME, pItem->Name);
			}
		}
	}
	{
		PPCommandItem * p_item = pItem->Dup();
		THROW_MEM(p_item);
		if(pos < 0 || pos >= (int)c)
			List.insert(p_item);
		else
			List.atInsert(pos, p_item);
	}
	CATCHZOK
	return ok;
}

int SLAPI PPCommandFolder::AddSeparator(int pos)
{
	int    ok = 1;
	PPCommandItem * p_item = new PPCommandItem(kSeparator);
	p_item->Name.Z().CatCharN('-', 40).Transf(CTRANSF_OUTER_TO_INNER);
	if(pos < 0 || pos >= (int)GetCount())
		List.insert(p_item);
	else
		List.atInsert(pos, p_item);
	return ok;
}

int SLAPI PPCommandFolder::Remove(uint pos)
{
	if(pos < GetCount()) {
		List.atFree(pos);
		return 1;
	}
	else
		return 0;
}

// static
int SLAPI PPCommandFolder::GetMenuList(const PPCommandGroup * pGrp, StrAssocArray * pAry, int isDesktop)
{
	int    ok = 1;
	SString db_symb;
	const PPCommandItem * p_item = 0;
	PPCommandGroup * p_desk = 0, grp;
	const PPCommandGroup * p_grp = 0;
	PPCommandMngr * p_mgr = 0;
	if(pGrp)
		p_grp = pGrp;
	else {
		THROW(p_mgr = GetCommandMngr(1, isDesktop));
		THROW(p_mgr->Load__(&grp));
		p_grp = &grp;
		ZDELETE(p_mgr);
	}
	CurDict->GetDbSymb(db_symb);
	for(uint i = 0; p_item = p_grp->Next(&i);) {
		if(isDesktop)
			p_desk = (p_item->Kind == PPCommandItem::kGroup) ? static_cast<PPCommandGroup *>(p_item->Dup()) : 0;
		if((!isDesktop && p_item->Kind == PPCommandItem::kFolder) || (p_desk && p_desk->IsDbSymbEq(db_symb)))
			pAry->Add(p_item->ID, p_item->Name);
		ZDELETE(p_desk);
	}
	CATCHZOK
	ZDELETE(p_mgr);
	ZDELETE(p_desk);
	return ok;
}

struct _ParentList {
	int    OnlyFolders;
	StrAssocArray * P_List;
};

int _GetIdParentList(const PPCommandItem * pItem, long parentID, void * extraPtr)
{
	int    ok = 0;
	if(pItem) {
		int    only_folders = static_cast<_ParentList *>(extraPtr)->OnlyFolders;
		StrAssocArray * p_list = static_cast<_ParentList *>(extraPtr)->P_List;
		if(p_list) {
			if(!only_folders || pItem->Kind == PPCommandItem::kFolder) {
				// @v9.2.8 {
				SString cmd_buf = pItem->Name;
				if(cmd_buf.C(0) == '@') {
					SString temp_buf;
					if(PPLoadString(cmd_buf.ShiftLeft(), temp_buf) > 0)
						cmd_buf = temp_buf;
				}
				// } @v9.2.8
				p_list->Add(pItem->ID, parentID, cmd_buf.Strip(), 0);
			}
			ok = 1;
		}
	}
	return ok;
}

int SLAPI PPCommandFolder::GetCommandList(StrAssocArray * pList, int onlyFolders)
{
	int    ok = -1;
	if(pList) {
		long   id = ID;
		_ParentList _list;
		_list.P_List = pList;
		_list.P_List->Z();
		_list.OnlyFolders = onlyFolders;
		ID = 0;
		ok = Enumerate(_GetIdParentList, 0, &_list);
		if(_list.P_List->getCount())
			_list.P_List->AtFree(0);
		_list.P_List->SortByID();
		ID = id;
	}
	return ok;
}

const PPCommandItem * SLAPI PPCommandFolder::SearchByName(const char * pName, const char * pDbSymb, uint * pPos) const
{
	uint   pos = 0;
	const  PPCommandItem * p_ret_item = 0;
	while(pos < List.getCount()) {
		const PPCommandItem * p_item = List.at(pos++);
		if(p_item->Name.CmpNC(pName) == 0) {
			if(!pDbSymb || (p_item->Kind == PPCommandItem::kGroup && static_cast<const PPCommandGroup *>(p_item)->IsDbSymbEq(pDbSymb))) {
				ASSIGN_PTR(pPos, pos);
				p_ret_item = p_item;
				break;
			}
		}
	}
	return p_ret_item;
}

const PPCommandItem * SLAPI PPCommandFolder::SearchByID(long id, uint * pPos) const
{
	uint   pos = 0;
	PPCommandItem * p_item = 0;
	if(List.lsearch(&id, &pos, CMPF_LONG, offsetof(PPCommandItem, ID)) > 0) {
		p_item = List.at(pos);
		ASSIGN_PTR(pPos, pos);
	}
	return p_item;
}

struct _Srch {
	long   ParentID;               // out param
	long   ID;                     // in param
	const  PPCommandItem * P_Item; // out param
};

static int _SearchByID(const PPCommandItem * pItem, long parentID, void * extraPtr)
{
	int    ok = -1;
	if(pItem) {
		_Srch * p_s = static_cast<_Srch *>(extraPtr);
		if(p_s->ID == pItem->ID) {
			p_s->ParentID = parentID;
			p_s->P_Item   = pItem;
			ok = 1;
		}
	}
	return ok;
}

const PPCommandItem * SLAPI PPCommandFolder::SearchByIDRecursive(long id, long * pParentID)
{
	_Srch _s;
	_s.ParentID = 0;
	_s.ID       = id;
	_s.P_Item   = 0;
	if(Enumerate(_SearchByID, ID, &_s) > 0)
		ASSIGN_PTR(pParentID, _s.ParentID);
	return _s.P_Item;
}

const PPCommandItem * SLAPI PPCommandFolder::SearchByCoord(TPoint coord, const PPDesktop & rD, uint * pPos) const
{
	const  int _igap = rD.GetIconGap();
	const  int _isz  = rD.GetIconSize();
	const PPCommandItem * p_item = 0;
	for(uint i = 0; p_item = Next(&i);) {
		if(p_item->Kind == PPCommandItem::kCommand) {
			TPoint lu = static_cast<const PPCommand *>(p_item)->P;
			TRect sqr_coord(lu, lu + (_isz * 2));
			if(sqr_coord.contains(coord)) {
				ASSIGN_PTR(pPos, i - 1);
				break;
			}
		}
	}
	return p_item;
}

int SLAPI PPCommandFolder::SearchFreeCoord(RECT r, const PPDesktop & rD, TPoint * pCoord) const
{
	const  int _igap = rD.GetIconGap();
	const  int _isz  = rD.GetIconSize();
	int    ok = -1;
	long   x = r.left + _igap + _isz * 2;
	long   y = r.top  + _igap + _isz * 2;
	long   mx = r.right - _igap;
	long   my = r.bottom - _igap;
	for(long dx = x; ok < 0 && dx < mx; dx += _isz * 2 + _igap) {
		for(long dy = y; ok < 0 && dy < my; dy += _isz * 2 + _igap) {
			TPoint c;
			c.Set(dx - _isz * 2, dy - _isz * 2);
			if(SearchByCoord(c, rD, 0) == 0) {
				ASSIGN_PTR(pCoord, c);
				ok = 1;
			}
		}
	}
	return ok;
}

const PPCommandItem * SLAPI PPCommandFolder::SearchFirst(uint * pPos)
{
	POINT  c;
	long   pos = -1;
	const  PPCommandItem * p_item = 0;
	MEMSZERO(c);
	for(uint i = 0; p_item = Next(&i);) {
		if(p_item->Kind == PPCommandItem::kCommand) {
			const long x = static_cast<const PPCommand *>(p_item)->P.x;
			const long y = static_cast<const PPCommand *>(p_item)->P.y;
			if((c.y == 0 || y < c.y) || (c.x == 0 || y == c.y && x <= c.x)) {
				c.x = x;
				c.y = y;
				pos = static_cast<long>(i - 1);
			}
		}
	}
	ASSIGN_PTR(pPos, static_cast<uint>(pos));
	return (pos >= 0) ? Get(static_cast<uint>(pos)) : 0;
}

const PPCommandItem * SLAPI PPCommandFolder::SearchNextByCoord(POINT coord, const PPDesktop & rD, Direction next, uint * pPos)
{
	const  int _igap = rD.GetIconGap();
	const  int _isz  = rD.GetIconSize();
	POINT  c;
	long   pos = -1;
	double distance = MAXLONG;
	const  PPCommandItem * p_item = 0;
	c.x = coord.x + _isz;
	c.y = coord.y + _isz;
	for(uint i = 0; p_item = Next(&i);) {
		if(p_item->Kind == PPCommandItem::kCommand) {
			long   x = static_cast<const PPCommand *>(p_item)->P.x;
			long   y = static_cast<const PPCommand *>(p_item)->P.y;
			if(next == nextUp && y < coord.y || next == nextDown && y > coord.y ||
				next == nextLeft && x < coord.x || next == nextRight && x > coord.x) {
				x += _isz;
				y += _isz;
				double d = sqrt((double)(x - c.x)*(x - c.x) + (y - c.y)*(y - c.y));
				if(d != 0 && d < distance) {
					distance = d;
					pos = static_cast<uint>(i - 1);
				}
			}
		}
	}
	ASSIGN_PTR(pPos, static_cast<uint>(pos));
	return (pos >= 0) ? Get(static_cast<uint>(pos)) : 0;
}

int SLAPI PPCommandFolder::GetIntersectIDs(const TRect & rR, const PPDesktop & rD, PPIDArray * pAry)
{
	const  int _isz  = rD.GetIconSize();
	int    found = 0;
	const PPCommandItem * p_item = 0;
	for(uint i = 0; p_item = Next(&i);) {
		if(p_item->Kind == PPCommandItem::kCommand) {
			TRect ir;
			/*
			RECT ri;
			ri.top  = ((PPCommand*)p_item)->Y;
			ri.left = ((PPCommand*)p_item)->X;
			ri.bottom = ri.top  + _isz * 2;
			ri.right  = ri.left + _isz * 2;
			*/
			rD.CalcIconRect(static_cast<const PPCommand *>(p_item)->P, ir);
			//if(SIntersectRect(ir, rect)) {
			if(rR.Intersect(ir, 0)) {
				CALLPTRMEMB(pAry, add(p_item->ID));
				found = 1;
			}
		}
	}
	return found;
}

int PPCommandFolder::GetIntersectIDs(TPoint coord, const PPDesktop & rD, PPIDArray * pAry)
{
	/*
	const  int _isz  = rD.GetIconSize();
	RECT   r;
	r.top    = coord.y;
	r.left   = coord.x;
	r.bottom = r.top  + _isz * 2;
	r.right  = r.left + _isz * 2;
	*/
	TRect ir;
	rD.CalcIconRect(coord, ir);
	return GetIntersectIDs(ir, rD, pAry);
}

int SLAPI PPCommandFolder::GetIconRect(long id, const PPDesktop & rD, TRect * pRect) const
{
	//const  int _isz = rD.GetIconSize();
	int    ok = -1;
	TRect  ir;
	const  PPCommandItem * p_item = SearchByID(id, 0);
	if(p_item && p_item->Kind == PPCommandItem::kCommand) {
        rD.CalcIconRect(static_cast<const PPCommand *>(p_item)->P, ir);
		ok = 1;
	}
	ASSIGN_PTR(pRect, ir);
	return ok;
}
//
//
//
SLAPI PPCommandGroup::PPCommandGroup() : PPCommandFolder()
{
	Kind = kGroup;
}

const SString & PPCommandGroup::GetLogo() const
{
	return Logo_;
}

int SLAPI PPCommandGroup::SetLogo(const char * pPath)
{
	int   ok = 1;
	Flags &= ~fBkgndImage;
	(Logo_ = pPath).Strip();
	if(Logo_.NotEmpty()) {
		if(fileExists(Logo_)) {
			Flags |= fBkgndImage;
			ok = 1;
		}
		else {
			Logo_ = 0;
			ok = PPSetErrorSLib();
		}
	}
	return ok;
}

int FASTCALL PPCommandGroup::SetDbSymb(const char * pDbSymb)
{
	(DbSymb = pDbSymb).Strip();
	return 1;
}

int FASTCALL PPCommandGroup::IsDbSymbEq(const char * pDbSymb) const
{
	return BIN(DbSymb.CmpNC(pDbSymb) == 0);
}

int FASTCALL PPCommandGroup::IsDbSymbEq(const PPCommandGroup & rGrp) const
{
	return BIN(DbSymb.CmpNC(rGrp.DbSymb) == 0);
}

int FASTCALL PPCommandGroup::Copy(const PPCommandGroup & s)
{
	DbSymb = s.DbSymb;
	Logo_  = s.Logo_;
	return PPCommandFolder::Copy(s);
}

PPCommandItem * SLAPI PPCommandGroup::Dup() const
{
	PPCommandGroup * p_item = new PPCommandGroup;
	p_item->Copy(*this);
	return p_item;
}

int SLAPI PPCommandGroup::Write(SBuffer & rBuf, long extraParam) const
{
	int    ok = 1;
	THROW(PPCommandFolder::Write(rBuf, extraParam));
	THROW_SL(rBuf.Write(DbSymb));
	if(Flags & (fBkgndImage|fBkgndImageLoaded)) {
		SString buf, dir;
		PPGetPath(PPPATH_BIN, dir);
		PPLoadText(PPTXT_DESKIMGDIR, buf);
		dir.SetLastSlash().Cat(buf).SetLastSlash();
		ObjLinkFiles _lf;
		_lf.Init(PPOBJ_DESKTOP, dir);
		_lf.Load(ID, 0L);
		if(Logo_.NotEmpty()) {
			_lf.Replace(0, Logo_);
		}
		else {
			_lf.Remove(0);
		}
		_lf.Save(ID, 0L);
	}
	CATCHZOK
	return ok;
}

int SLAPI PPCommandGroup::Read(SBuffer & rBuf, long extraParam)
{
	int    ok = 1;
	THROW(PPCommandFolder::Read(rBuf, extraParam));
	THROW_SL(rBuf.Read(DbSymb));
	if(Flags & fBkgndImage) {
		PROFILE_START
		SString buf, dir;
		PPGetPath(PPPATH_BIN, dir);
		PPLoadText(PPTXT_DESKIMGDIR, buf);
		dir.SetLastSlash().Cat(buf).SetLastSlash();
		ObjLinkFiles logo;
		logo.Init(PPOBJ_DESKTOP, dir);
		logo.Load(ID, 0L);
		logo.At(0, buf);
		SetLogo(buf);
		if(Flags & fBkgndImage)
			Flags |= fBkgndImageLoaded;
		PROFILE_END
	}
	CATCHZOK
	return ok;
}

int SLAPI PPCommandGroup::LoadLogo()
{
	int    ok = -1;
	SString buf, dir;
	PPGetPath(PPPATH_BIN, dir);
	PPLoadText(PPTXT_DESKIMGDIR, buf);
	dir.SetLastSlash().Cat(buf).SetLastSlash();
	ObjLinkFiles logo;
	logo.Init(PPOBJ_DESKTOP, dir);
	logo.Load(ID, 0L);
	if(logo.At(0, Logo_.Z()))
		ok = 1;
	return ok;
}

int SLAPI PPCommandGroup::StoreLogo()
{
	int    ok = 1;
	SString buf, dir;
	PPGetPath(PPPATH_BIN, dir);
	PPLoadText(PPTXT_DESKIMGDIR, buf);
	dir.SetLastSlash().Cat(buf).SetLastSlash();

	ObjLinkFiles _lf;
	_lf.Init(PPOBJ_DESKTOP, dir);
	_lf.Load(ID, 0L);
	if(Logo_.Len() == 0)
		_lf.Remove(0);
	else
		_lf.Replace(0, Logo_);
	_lf.Save(ID, 0L);
	return ok;
}

PPCommandGroup * FASTCALL PPCommandGroup::GetDesktop(long id)
{
	uint   pos = 0;
	const PPCommandItem * p_item = SearchByID(id, &pos);
	return (p_item && p_item->Kind == PPCommandItem::kGroup) ? static_cast<PPCommandGroup *>(List.at(pos)) : 0;
}

PPCommandGroup & FASTCALL PPCommandGroup::operator = (const PPCommandGroup & rSrc)
{
	Copy(rSrc);
	return *this;
}

int SLAPI PPCommandGroup::InitDefaultDesktop(const char * pName)
{
	int    ok = 1;
	Name = pName;
	CurDict->GetDbSymb(DbSymb);
	Flags = PPCommandItem::fBkgndGradient;
	return ok;
}
//
//
//
#define PPCS_SIGNATURE 0x54435050L

PPCommandMngr * SLAPI GetCommandMngr(int readOnly, int isDesktop, const char * pPath /*=0*/)
{
	PPCommandMngr * p_mgr = 0;
	SString path;
	if(pPath)
		path = pPath;
	else if(isDesktop)
		PPGetFilePath(PPPATH_BIN, PPFILNAM_PPDESK_BIN, path);
	else
		PPGetFilePath(PPPATH_BIN, PPFILNAM_PPCMD_BIN, path);
	if(path.Empty())
		PPSetError(PPERR_UNDEFCMDFILENAME);
	else
		p_mgr = new PPCommandMngr(path, readOnly);
	if(p_mgr) {
		if(!p_mgr->IsValid_()) {
			ZDELETE(p_mgr);
		}
	}
	else
		PPSetErrorNoMem();
	return p_mgr;
}

/*
int SLAPI PPCommandMngr::Backup()
{

}
*/

SLAPI PPCommandMngr::PPCommandMngr(const char * pFileName, int readOnly) : ReadOnly(readOnly)
{
	long   mode = 0;
	if(readOnly)
		mode = (SFile::mRead | SFile::mDenyWrite); // @v9.0.11 SFile::mDenyWrite
	else
		mode = (SFile::mReadWrite | SFile::mDenyWrite | mDenyRead); // @v9.0.11 mDenyRead
	mode |= (SFile::mBinary | SFile::mNoStd);
	//
	// “ак как файл может быть заблокирован другим пользователем,
	// предпримем несколько попыток его открыти€.
	// »сходим из предположени€, что файл дл€ записи открываетс€ на малое врем€.
	//
	for(uint i = 0; i < 10; i++) {
		if(F.Open(pFileName, mode))
			break;
		else
			SDelay(100);
	}
}

SLAPI PPCommandMngr::~PPCommandMngr()
{
}

int SLAPI PPCommandMngr::IsValid_() const
{
	return F.IsValid() ? 1 : PPSetErrorSLib();
}

int SLAPI PPCommandMngr::Save__(const PPCommandGroup * pCmdGrp)
{
	int    ok = 1;
	Hdr    hdr;
	uint32 crc = 0;
	SBuffer buf;
	THROW_SL(F.IsValid());
	THROW_PP(!ReadOnly, PPERR_CMDMNGREADONLY);
	F.Seek(0, SEEK_SET);
	MEMSZERO(hdr);
	hdr.Signature = PPCS_SIGNATURE;
	THROW_SL(F.Write(&hdr, sizeof(Hdr)));
	THROW(pCmdGrp->Write(buf, 0));
	THROW_SL(F.Write(buf));
	THROW_SL(F.CalcCRC(sizeof(hdr), &crc));
	F.Seek(0, SEEK_SET);
	hdr.Crc = crc;
	THROW_SL(F.Write(&hdr, sizeof(Hdr)));
	CATCHZOK
	return ok;
}

int SLAPI PPCommandMngr::Load__(PPCommandGroup * pCmdGrp)
{
	int    ok = 1, r = 0;
	Hdr    hdr;
	int64  fsz = 0;
	uint32 crc = 0;
	SBuffer buf;
	THROW_SL(F.IsValid());
	F.CalcSize(&fsz);
	if(fsz > 0) {
		THROW_SL(F.CalcCRC(sizeof(hdr), &crc));
		F.Seek(0, SEEK_SET);
		THROW_SL(F.Read(&hdr, sizeof(hdr)));
		THROW_PP_S(hdr.Signature == PPCS_SIGNATURE, PPERR_CMDFILSIGN, F.GetName());
		THROW_PP_S(hdr.Crc == crc, PPERR_CMDFILCRC, F.GetName());
		THROW_SL(F.Read(buf));
		THROW(pCmdGrp->Read(buf, 0));
	}
	CATCHZOK
	return ok;
}
//
//
//
SLAPI PPCommandHandler::PPCommandHandler(const PPCommandDescr * pDescr)
{
	if(!RVALUEPTR(D, pDescr))
		MEMSZERO(D);
}

SLAPI PPCommandHandler::~PPCommandHandler()
{
}

int SLAPI PPCommandHandler::EditParam(SBuffer * pParam, long, void * extraPtr)
{
	return -1;
}

int SLAPI PPCommandHandler::Run(SBuffer * pParam, long, void * extraPtr)
{
	return -1;
}
//
//
//
#define ICON_COMMAND_BIAS 10000L

int SLAPI EditPPViewFilt(int viewID, SBuffer * pParam, void * extraPtr)
{
	int    ok = 1;
	size_t sav_offs = 0;
	PPBaseFilt * p_filt = 0;
	PPView * p_view = 0;
	THROW_INVARG(pParam);
	sav_offs = pParam->GetRdOffs();
	THROW(PPView::CreateInstance(viewID, &p_view) > 0);
	{
		THROW(PPView::ReadFiltPtr(*pParam, &p_filt));
		SETIFZ(p_filt, p_view->CreateFilt(extraPtr));
		if((ok = p_view->EditBaseFilt(p_filt)) > 0) {
			THROW(p_view->WriteFiltPtr(pParam->Z(), p_filt));
		}
		else
			pParam->SetRdOffs(sav_offs);
	}
	CATCH
		CALLPTRMEMB(pParam, SetRdOffs(sav_offs));
		ok = 0;
	ENDCATCH
	ZDELETE(p_filt);
	ZDELETE(p_view);
	return ok;
}

int SLAPI RunPPViewCmd(int viewID, SBuffer * pParam, long menuCm, long cmdID, void * extraPtr)
{
	int    ok = 1;
	PPBaseFilt * p_filt = 0;
	PPView * p_view = 0;
	if((menuCm || cmdID) && APPL) {
		static_cast<PPApp *>(APPL)->LastCmd = (cmdID) ? (cmdID + ICON_COMMAND_BIAS) : menuCm;
		if(pParam)
			PPView::ReadFiltPtr(*pParam, &p_filt);
		THROW(p_view->Execute(viewID, p_filt, 1, extraPtr));
		static_cast<PPApp *>(APPL)->LastCmd = 0;
		ok = 1;
	}
	CATCHZOK
	ZDELETE(p_filt);
	ZDELETE(p_view);
	return ok;
}

class CMD_HDL_CLS(DEFAULT) : public PPCommandHandler {
public:
	SLAPI  CMD_HDL_CLS(DEFAULT)(const PPCommandDescr * pDescr) : PPCommandHandler(pDescr)
	{
	}
	virtual int SLAPI EditParam(SBuffer * pParam, long cmdID, void * extraPtr)
	{
		int    ok = -1;
		if(D.ViewId) {
			if(D.ViewId == PPVIEW_BILL) {
				BillFilt::FiltExtraParam p(1, static_cast<BrowseBillsType>(D.FiltExtId));
				ok = EditPPViewFilt(D.ViewId, pParam, &p);
			}
			else
				ok = EditPPViewFilt(D.ViewId, pParam, reinterpret_cast<void *>(D.FiltExtId));
		}
		return ok;
	}
	virtual int SLAPI Run(SBuffer * pParam, long cmdID, void * extraPtr)
	{
		int    ok = -1;
		if(D.ViewId) {
			if(D.ViewId == PPVIEW_BILL) {
				BillFilt::FiltExtraParam p(1, static_cast<BrowseBillsType>(D.FiltExtId));
				THROW(PPCheckDatabaseChain());
				ok = RunPPViewCmd(D.ViewId, pParam, D.MenuCm, cmdID, &p);
			}
			else
				ok = RunPPViewCmd(D.ViewId, pParam, D.MenuCm, cmdID, reinterpret_cast<void *>(D.FiltExtId));
		}
		else if(D.MenuCm && APPL) {
			static_cast<PPApp *>(APPL)->processCommand(static_cast<uint>(D.MenuCm));
			ok = 1;
		}
		CATCHZOK
		return ok;
	}
};

IMPLEMENT_CMD_HDL_FACTORY(DEFAULT);
//
//
//
class CMD_HDL_CLS(ADDPERSON) : public PPCommandHandler {
public:
	SLAPI  CMD_HDL_CLS(ADDPERSON)(const PPCommandDescr * pDescr) : PPCommandHandler(pDescr)
	{
	}
	virtual int SLAPI EditParam(SBuffer * pParam, long, void * extraPtr)
	{
		return -1;
	}
	virtual int SLAPI Run(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = -1;
		size_t sav_offs = 0;
		if(D.MenuCm && APPL) {
			int    ok = -1, r = cmCancel;
			PPID   id = 0;
			SString name;
			PPObjPerson psn_obj;
			PPInputStringDialogParam isd_param;
			PsnSelAnalogDialog * dlg = 0;
			THROW_INVARG(pParam);
			sav_offs = pParam->GetRdOffs();
			pParam->Read(name);
			static_cast<PPApp *>(APPL)->LastCmd = D.MenuCm;
			GetObjectTitle(PPOBJ_PERSON, isd_param.Title);
			isd_param.InputTitle = isd_param.Title;
			if(InputStringDialog(&isd_param, name) > 0) {
				THROW(pParam->Z().Write(name));
				if(CheckDialogPtrErr(&(dlg = new PsnSelAnalogDialog(&psn_obj)))) {
					dlg->setSrchString(name);
					if(ExecView(dlg) == cmOK)
						dlg->getResult(&id);
					if(!id) {
						PPObjPerson::EditBlock eb;
						eb.InitStatusID = PPPRS_LEGAL;
						eb.Name = name;
						r = psn_obj.Edit_(&id, eb);
					}
					else
						r = cmOK;
				}
			}
			else
				pParam->SetRdOffs(sav_offs);
			delete dlg;
			if(r == cmOK) {
				PersonFilt filt;
				filt.PersonID = id;
				ViewPerson(&filt);
				ok = 1;
			}
		}
		CATCH
			if(pParam)
				pParam->SetRdOffs(sav_offs);
			ok = 0;
		ENDCATCH
		static_cast<PPApp *>(APPL)->LastCmd = 0;
		return ok;
	}
};

IMPLEMENT_CMD_HDL_FACTORY(ADDPERSON);
//
//
//
static int ReadPrjTaskRec(PrjTaskTbl::Rec * pRec, SBuffer & rBuf, long)
{
	int    ok = 1;
	SString code, descr, memo;
	THROW_INVARG(pRec);
	THROW_SL(rBuf.Read(pRec->ID));
	THROW_SL(rBuf.Read(pRec->ProjectID));
	THROW_SL(rBuf.Read(pRec->Kind));
	THROW_SL(rBuf.Read(code));
	THROW_SL(rBuf.Read(pRec->CreatorID));
	THROW_SL(rBuf.Read(pRec->GroupID));
	THROW_SL(rBuf.Read(pRec->EmployerID));
	THROW_SL(rBuf.Read(pRec->ClientID));
	THROW_SL(rBuf.Read(pRec->TemplateID));
	THROW_SL(rBuf.Read(pRec->Dt));
	THROW_SL(rBuf.Read(pRec->Tm));
	THROW_SL(rBuf.Read(pRec->StartDt));
	THROW_SL(rBuf.Read(pRec->StartTm));
	THROW_SL(rBuf.Read(pRec->EstFinishDt));
	THROW_SL(rBuf.Read(pRec->EstFinishTm));
	THROW_SL(rBuf.Read(pRec->FinishDt));
	THROW_SL(rBuf.Read(pRec->FinishTm));
	THROW_SL(rBuf.Read(pRec->Priority));
	THROW_SL(rBuf.Read(pRec->Status));
	THROW_SL(rBuf.Read(pRec->DrPrd));
	THROW_SL(rBuf.Read(pRec->DrKind));
	THROW_SL(rBuf.Read(pRec->DrDetail));
	THROW_SL(rBuf.Read(pRec->Flags));
	THROW_SL(rBuf.Read(pRec->DlvrAddrID));
	THROW_SL(rBuf.Read(pRec->LinkTaskID));
	THROW_SL(rBuf.Read(pRec->Amount));
	THROW_SL(rBuf.Read(pRec->OpenCount));
	THROW_SL(rBuf.Read(pRec->BillArID));
	THROW_SL(rBuf.Read(descr));
	THROW_SL(rBuf.Read(memo));
	code.CopyTo(pRec->Code, sizeof(pRec->Code));
	descr.CopyTo(pRec->Descr, sizeof(pRec->Descr));
	memo.CopyTo(pRec->Memo, sizeof(pRec->Memo));
	CATCHZOK
	return ok;
}

static int WritePrjTaskRec(const PrjTaskTbl::Rec * pRec, SBuffer & rBuf, long)
{
	int    ok = 1;
	SString code, descr, memo;
	THROW_INVARG(pRec);
	code.CopyFrom(pRec->Code);
	descr.CopyFrom(pRec->Descr);
	memo.CopyFrom(pRec->Memo);
	THROW_SL(rBuf.Write(pRec->ID));
	THROW_SL(rBuf.Write(pRec->ProjectID));
	THROW_SL(rBuf.Write(pRec->Kind));
	THROW_SL(rBuf.Write(code));
	THROW_SL(rBuf.Write(pRec->CreatorID));
	THROW_SL(rBuf.Write(pRec->GroupID));
	THROW_SL(rBuf.Write(pRec->EmployerID));
	THROW_SL(rBuf.Write(pRec->ClientID));
	THROW_SL(rBuf.Write(pRec->TemplateID));
	THROW_SL(rBuf.Write(pRec->Dt));
	THROW_SL(rBuf.Write(pRec->Tm));
	THROW_SL(rBuf.Write(pRec->StartDt));
	THROW_SL(rBuf.Write(pRec->StartTm));
	THROW_SL(rBuf.Write(pRec->EstFinishDt));
	THROW_SL(rBuf.Write(pRec->EstFinishTm));
	THROW_SL(rBuf.Write(pRec->FinishDt));
	THROW_SL(rBuf.Write(pRec->FinishTm));
	THROW_SL(rBuf.Write(pRec->Priority));
	THROW_SL(rBuf.Write(pRec->Status));
	THROW_SL(rBuf.Write(pRec->DrPrd));
	THROW_SL(rBuf.Write(pRec->DrKind));
	THROW_SL(rBuf.Write(pRec->DrDetail));
	THROW_SL(rBuf.Write(pRec->Flags));
	THROW_SL(rBuf.Write(pRec->DlvrAddrID));
	THROW_SL(rBuf.Write(pRec->LinkTaskID));
	THROW_SL(rBuf.Write(pRec->Amount));
	THROW_SL(rBuf.Write(pRec->OpenCount));
	THROW_SL(rBuf.Write(pRec->BillArID));
	THROW_SL(rBuf.Write(descr));
	THROW_SL(rBuf.Write(memo));
	CATCHZOK
	return ok;
}

class CMD_HDL_CLS(ADDTASK) : public PPCommandHandler {
public:
	SLAPI  CMD_HDL_CLS(ADDTASK)(const PPCommandDescr * pDescr) : PPCommandHandler(pDescr)
	{
	}
	virtual int SLAPI EditParam(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = -1;
		size_t sav_offs = 0;
		PrjTaskTbl::Rec rec;
		THROW_INVARG(pParam);
        sav_offs = pParam->GetRdOffs();
		MEMSZERO(rec);
		if(pParam->GetAvailableSize() == 0)
			TodoObj.InitPacket(&rec, TODOKIND_TASK, 0, 0, 0, 0);
		else
			THROW(ReadPrjTaskRec(&rec, *pParam, 0));
		getcurdatetime(&rec.Dt, &rec.Tm);
		if(TodoObj.EditDialog(&rec) > 0) {
			THROW(WritePrjTaskRec(&rec, pParam->Z(), 0));
			ok = 1;
		}
		else
			pParam->SetRdOffs(sav_offs);
		CATCH
			CALLPTRMEMB(pParam, SetRdOffs(sav_offs));
			ok = 0;
		ENDCATCH
		return ok;
	}
	virtual int SLAPI Run(SBuffer * pParam, long cmdID, void * extraPtr)
	{
		int    ok = -1;
		if((D.MenuCm || cmdID)&& APPL) {
			PPID   id = 0;
			SBuffer param;
			THROW_INVARG(pParam);
			THROW(TodoObj.CheckRightsModByID(&id));
			static_cast<PPApp *>(APPL)->LastCmd = (cmdID) ? (cmdID + ICON_COMMAND_BIAS) : D.MenuCm;
			param = *pParam;
			if(EditParam(&param, cmdID, extraPtr) > 0) {
				PrjTaskTbl::Rec rec;
				MEMSZERO(rec);
				THROW(ReadPrjTaskRec(&rec, param, 0));
				THROW(ok = TodoObj.PutPacket(&id, &rec, 1));
			}
		}
		CATCHZOK
		static_cast<PPApp *>(APPL)->LastCmd = 0;
		return ok;
	}
private:
	PPObjPrjTask TodoObj;
};

IMPLEMENT_CMD_HDL_FACTORY(ADDTASK);
//
//
//
class CMD_HDL_CLS(ADDPERSONEVENT) : public PPCommandHandler {
public:
	SLAPI  CMD_HDL_CLS(ADDPERSONEVENT)(const PPCommandDescr * pDescr) : PPCommandHandler(pDescr)
	{
	}
	virtual int SLAPI EditParam(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = -1, valid_data = 0;
		size_t sav_offs = 0;
		AddPersonEventFilt filt;
		THROW_INVARG(pParam);
        sav_offs = pParam->GetRdOffs();
		filt.Read(*pParam, 0);
		if(filt.Edit() > 0) {
			THROW(filt.Write(pParam->Z(), 0));
		}
		else
			pParam->SetRdOffs(sav_offs);
		CATCH
			CALLPTRMEMB(pParam, SetRdOffs(sav_offs));
			ok = 0;
		ENDCATCH
		return ok;
	}
	virtual int SLAPI Run(SBuffer * pParam, long, void * extraPtr);
private:
	int    SLAPI RunBySymb(SBuffer * pSymb);
	PPObjPersonEvent PsnEvObj;
};

class SelectPersonByCodeDialog : public TDialog {
public:
	struct Rec {
		Rec()
		{
			THISZERO();
		}
		PPID   PrmrPsnID;
		PPID   ScndPsnID;
		PPID   SCardID; // @v9.1.3
		SCardTbl::Rec Sc;
	};
	SelectPersonByCodeDialog(const char * pSubTitle, PPPersonKind * pPsnKindRec, PPPersonKind * pPsnScndKindRec, const char * pInputPrompt) :
		TDialog(DLG_SELPERSONC)
	{
		setStaticText(CTL_SELPERSONC_SUBTITLE, pSubTitle);
		if(!RVALUEPTR(PsnKindRec, pPsnKindRec))
			MEMSZERO(PsnKindRec);
		if(!RVALUEPTR(PsnScndKindRec, pPsnScndKindRec))
			MEMSZERO(PsnScndKindRec);
		if(pInputPrompt)
			setLabelText(CTL_SELPERSONC_CODEINP, pInputPrompt);
		showCtrl(CTLSEL_SELPERSONC_SCARD, 0); // @v9.1.3
	}
	int    setDTS(const Rec *);
	int    getDTS(Rec *);
private:
	DECL_HANDLE_EVENT;
	void   SetupCtrls();
	Rec    Data;
	PPPersonKind PsnKindRec;
	PPPersonKind PsnScndKindRec;
	PPObjSCard ScObj;
	PPObjPerson PsnObj;
};

IMPL_HANDLE_EVENT(SelectPersonByCodeDialog)
{
	TDialog::handleEvent(event);
	if(event.isCmd(cmCBSelected) && event.isCtlEvent(CTLSEL_SELPERSONC_PRSN)) {
		SetupCtrls();
		clearEvent(event);
	}
}

void SelectPersonByCodeDialog::SetupCtrls()
{
	SString code;
	PPID   psn_id = getCtrlLong(CTLSEL_SELPERSONC_PRSN);
	if(psn_id) {
		RegisterTbl::Rec reg_rec;
		MEMSZERO(reg_rec);
		PsnObj.GetRegister(psn_id, PsnKindRec.CodeRegTypeID, &reg_rec);
		code = reg_rec.Num;

		showCtrl(CTLSEL_SELPERSONC_SCARD, 1); // @v9.1.3

		PPObjSCard::Filt sc_filt;
		sc_filt.OwnerID = psn_id;
		SetupPPObjCombo(this, CTLSEL_SELPERSONC_SCARD, PPOBJ_SCARD, Data.SCardID, 0, &sc_filt);
	}
	else
		showCtrl(CTLSEL_SELPERSONC_SCARD, 0); // @v9.1.3
	disableCtrl(CTL_SELPERSONC_CODEINP, psn_id);
	setCtrlString(CTL_SELPERSONC_CODEINP, code);
}

int SelectPersonByCodeDialog::setDTS(const Rec * pData)
{
	int    ok = 1;
	if(!RVALUEPTR(Data, pData))
		MEMSZERO(Data);
	SetupPersonCombo(this, CTLSEL_SELPERSONC_PRSN, Data.PrmrPsnID, 0, PsnKindRec.ID, 0);
	if(PsnScndKindRec.ID)
		SetupPersonCombo(this, CTLSEL_SELPERSONC_PRSNSC, Data.ScndPsnID, 0, PsnScndKindRec.ID, 0);
	else
		disableCtrl(CTLSEL_SELPERSONC_PRSNSC, 1);
	SetupCtrls();
	return ok;
}

int SelectPersonByCodeDialog::getDTS(Rec * pData)
{
	int    ok = 1;
	PPIDArray psn_list;
	SCardTbl::Rec  sc_rec;
	MEMSZERO(pData->Sc);
	getCtrlData(CTLSEL_SELPERSONC_PRSN, &Data.PrmrPsnID);
	if(PsnScndKindRec.ID)
		getCtrlData(CTLSEL_SELPERSONC_PRSNSC, &Data.ScndPsnID);
	if(!Data.PrmrPsnID) {
		SString code;
		getCtrlString(CTL_SELPERSONC_CODEINP, code);
		if(code.Len()) {
			if(PsnKindRec.CodeRegTypeID && PsnObj.GetListByRegNumber(PsnKindRec.CodeRegTypeID, PsnKindRec.ID, code, psn_list) > 0)
				Data.PrmrPsnID = psn_list.getCount() ? psn_list.at(0) : 0;
			else {
				PersonTbl::Rec psn_rec;
				MEMSZERO(psn_rec);
				MEMSZERO(sc_rec);
				if(ScObj.P_Tbl->SearchCode(0, code, &sc_rec) > 0 && PsnObj.P_Tbl->IsBelongToKind(sc_rec.PersonID, PsnKindRec.ID) > 0) {
					Data.PrmrPsnID = sc_rec.PersonID;
					Data.Sc = sc_rec;
				}
			}
		}
		THROW_PP_S(Data.PrmrPsnID, PPERR_PERSONNOTFOUND, code);
	}
	else {
        getCtrlData(CTLSEL_SELPERSONC_SCARD, &Data.SCardID);
        if(Data.SCardID && ScObj.Search(Data.SCardID, &sc_rec) > 0 && sc_rec.PersonID == Data.PrmrPsnID) {
			Data.Sc = sc_rec;
        }
        else {
			Data.SCardID = 0;
        }
	}
	ASSIGN_PTR(pData, Data);
	CATCHZOK
	return ok;
}

int SLAPI CMD_HDL_CLS(ADDPERSONEVENT)::RunBySymb(SBuffer * pParam)
{
	int    ok = -1;
	SelectPersonByCodeDialog * p_dlg = 0;
	if(D.MenuCm && pParam) {
		SString title, code, symb, prompt;
		PPPsnOpKind  pop_rec;
		PPPersonKind pk_rec, scnd_pk_rec;
		PPPsnOpKindPacket pop_pack;
		PPRegisterTypePacket regtyp_pack;
		PPObjPersonKind pk_obj;
		PPObjPsnOpKind  pop_obj;
		PPObjPersonEvent pev_obj;
		PPObjPerson psn_obj;
		PPObjRegisterType obj_regt;
		PPObjSCard sc_obj;
		SelectPersonByCodeDialog::Rec psn_data;
		static_cast<PPApp *>(APPL)->LastCmd = D.MenuCm;
		MEMSZERO(pop_rec);
		MEMSZERO(pk_rec);
		MEMSZERO(scnd_pk_rec);
		CALLPTRMEMB(pParam, Read(symb));
		THROW(pop_obj.SearchBySymb(symb, 0, &pop_rec) > 0);
		THROW(pop_obj.GetPacket(pop_rec.ID, &pop_pack) > 0);
		THROW(pk_obj.Search(pop_pack.PCPrmr.PersonKindID, &pk_rec) > 0);
		if(pop_pack.PCScnd.PersonKindID)
			THROW(pk_obj.Search(pop_pack.PCScnd.PersonKindID, &scnd_pk_rec) > 0);
		if(pop_pack.PCScnd.DefaultID)
			psn_data.ScndPsnID = pop_pack.PCScnd.DefaultID;
		if(pk_rec.CodeRegTypeID && obj_regt.GetPacket(pk_rec.CodeRegTypeID, &regtyp_pack) > 0)
			prompt = regtyp_pack.Rec.Name;
		else
			PPLoadText(PPTXT_SELPERSONBYSCARD, prompt);
		THROW(CheckDialogPtr(&(p_dlg = new SelectPersonByCodeDialog(pop_rec.Name, &pk_rec, &scnd_pk_rec, prompt))));
		THROW(p_dlg->setDTS(&psn_data));
		while(ok < 0 && ExecView(p_dlg) == cmOK) {
			int    r = 1;
			int    disable_op = 0;
			PPID   prmr_psn_id = 0, scnd_psn_id = 0;
			if(p_dlg->getDTS(&psn_data) > 0) {
				PPPsnEventPacket pack;
				prmr_psn_id = psn_data.PrmrPsnID;
				scnd_psn_id = psn_data.ScndPsnID ? psn_data.ScndPsnID : pop_pack.PCScnd.DefaultID;
				THROW(pev_obj.InitPacket(&pack, pop_rec.ID, prmr_psn_id));
				pack.Rec.SecondID   = scnd_psn_id;
				pack.Rec.LocationID = LConfig.Location;
				pack.Rec.PrmrSCardID = psn_data.Sc.ID;
				if(psn_obj.GetConfig().Flags & PPPersonConfig::fShowPsnImageAfterCmdAssoc) {
					uint   pos = 0;
					SString info, warn, buf, reg_buf;
					RegisterTbl::Rec reg_rec;
					PPPersonPacket psn_pack;
					MEMSZERO(reg_rec);
					THROW(psn_obj.GetPacket(prmr_psn_id, &psn_pack, 0));
					info.Cat(psn_pack.Rec.Name);
					// @v9.1.5 PPGetWord(PPWORD_ISVALIDBEFORE, 0, buf);
					PPLoadString("validuntil", buf); // @v9.1.5
					if(psn_data.Sc.ID) {
						// @v9.0.2 PPGetWord(PPWORD_CARD, 0, symb);
						PPLoadString("card", symb); // @v9.0.2
						prompt.Z().Cat(symb).CatDiv(':', 2).Cat(psn_data.Sc.Code);
						// @v9.1.3 {
						{
							const int scst = PPObjSCard::GetSeriesType(psn_data.Sc.SeriesID);
							if(oneof2(scst, scstCredit, scstBonus)) {
								double sc_rest = 0.0;
								sc_obj.P_Tbl->GetRest(psn_data.Sc.ID, MAXDATE, &sc_rest);
								PPLoadString("rest", symb);
								prompt.Space().Cat(symb).CatDiv(':', 2).Cat(sc_rest, MKSFMTD(0, 2, 0));
							}
						}
						// } @v9.1.3
						if(psn_data.Sc.Expiry) {
							//
							// “екст сообщени€ о сроке годности карты
							//
							prompt.CatDiv(';', 2).Cat(PPLoadTextS(PPTXT_SCARD_EXPIRY, symb)).Space().Cat(psn_data.Sc.Expiry);
						}
						if(psn_data.Sc.UsageTmStart || psn_data.Sc.UsageTmEnd) {
							//
							// “екст сообщени€ о времени действи€ карты
							//
							prompt.CatDiv(';', 2).Cat(PPLoadTextS(PPTXT_SCARD_TIMEPERIOD, symb)).Space();
							if(psn_data.Sc.UsageTmStart)
								prompt.Cat(psn_data.Sc.UsageTmStart);
							prompt.CatCharN('.', 2);
							if(psn_data.Sc.UsageTmEnd)
								prompt.Cat(psn_data.Sc.UsageTmEnd);
						}
						if(!sc_obj.CheckRestrictions(&psn_data.Sc, 0, getcurdatetime_())) {
							PPGetLastErrorMessage(1, warn);
							disable_op = 1;
						}
						info.CatDiv(';', 2).Cat(prompt);
					}
					else if(psn_pack.GetRegister(pk_rec.CodeRegTypeID, &pos) > 0) {
						reg_rec = psn_pack.Regs.at(pos-1);
						psn_obj.RegObj.Format(reg_rec.ID, 0, reg_buf);
						info.CatDivIfNotEmpty(';', 2).Cat(reg_buf);
						if(reg_rec.Expiry && reg_rec.Expiry < getcurdate_()) {
							//
							// “екст сообщени€ о том, что срок действи€ регистра истек
							//
							GetRegisterTypeName(reg_rec.RegTypeID, reg_buf);
							warn.Printf(PPLoadTextS(PPTXT_PSNREGEXPIRED, buf), reg_buf.cptr());
							disable_op = 1;
						}
					}
					if(!pev_obj.CheckRestrictions(&pack, psn_pack.Rec.ID, pack.Rec.PrmrSCardID, &pop_pack.PCPrmr)) {
						PPGetLastErrorMessage(1, warn);
						disable_op = 1;
					}
					if(warn.Empty()) {
						//
						// ≈сли все предыдущие проверки прошли успешно, то провер€ем
						// все регистры персоналии на предмет истечени€ срока действи€.
						//
						for(uint i = 0; i < psn_pack.Regs.getCount(); i++) {
							const RegisterTbl::Rec & r_rec = psn_pack.Regs.at(i);
							PPRegisterType rt_rec;
							if(obj_regt.Fetch(r_rec.RegTypeID, &rt_rec) > 0 && rt_rec.Flags & REGTF_WARNEXPIRY) {
								if(r_rec.Expiry && r_rec.Expiry < getcurdate_()) {
									warn.Printf(PPLoadTextS(PPTXT_PSNREGEXPIRED, buf), rt_rec.Name);
									break;
								}
							}
						}
					}
					/*
					if(warn.Empty()) {
						LTIME cur_tm;
						getcurtime(&cur_tm);
						if(!psn_data.UsageTm.Check(cur_tm)) {
							SString low_tm, upp_tm;
							PPLoadText(PPTXT_USAGETM, buf);
							low_tm.Cat(psn_data.UsageTm.low, TIMF_HM);
							upp_tm.Cat(psn_data.UsageTm.upp, TIMF_HM);
							warn.Printf(buf, (const char *)low_tm, (const char *)upp_tm);
						}
					}
					*/
					{
						PPObjTag tag_obj;
						PPObjectTag tag_rec;
						if(pop_pack.PCPrmr.RestrictTagID && tag_obj.Fetch(pop_pack.PCPrmr.RestrictTagID, &tag_rec) > 0) {
							const ObjTagItem * p_tag = psn_pack.TagL.GetItem_ForceEmpty(pop_pack.PCPrmr.RestrictTagID);
							if(p_tag) {
								p_tag->GetStr(prompt);
								info.CR().CatEq(tag_rec.Name, prompt);
							}
						}
						else {
							StrAssocArray warn_list, info_list;
							if(tag_obj.GetWarnList(&psn_pack.TagL, &warn_list, &info_list) > 0)
								warn = warn_list.Get(0).Txt;
							else if(info_list.getCount() && tag_obj.Fetch(info_list.Get(0).Id, &tag_rec) > 0)
								info.CR().CatEq(tag_rec.Name, info_list.Get(0).Txt);
						}
					}
					psn_pack.LinkFiles.Init(PPOBJ_PERSON);
					if(psn_pack.Rec.Flags & PSNF_HASIMAGES)
						psn_pack.LinkFiles.Load(psn_pack.Rec.ID, 0L);
					psn_pack.LinkFiles.At(0, buf);
					r = ViewImageInfo(buf, info, warn);
				}
				if(r > 0) {
					if(!disable_op) {
						PPID   ev_id = 0;
						if(pev_obj.PutPacket(&ev_id, &pack, 1))
							ok = 1;
					}
					else
						PPSetError(PPERR_OPERDISABLED);
					if(ok < 0)
						PPError();
				}
			}
			else
				PPError();
		}
	}
	CATCHZOK
	delete p_dlg;
	static_cast<PPApp *>(APPL)->LastCmd = 0;
	return ok;
}

int SLAPI CMD_HDL_CLS(ADDPERSONEVENT)::Run(SBuffer * pParam, long cmdID, void * extraPtr)
{
	int    ok = -1;
	int    interactive_level = 2;
	PsnEventDialog * p_dlg = 0;
	if((D.MenuCm || cmdID) && APPL) {
		if(extraPtr)
			ok = RunBySymb(static_cast<SBuffer *>(extraPtr));
		else {
			PPID   op_id = 0;
			PPObjPsnOpKind pop_obj;
			PPPsnEventPacket pack;

			SBuffer sbuf;
			AddPersonEventFilt filt;
			static_cast<PPApp *>(APPL)->LastCmd = cmdID ? (cmdID + ICON_COMMAND_BIAS) : D.MenuCm;
			if(pParam)
				filt.Read(*pParam, 0);
			interactive_level = filt.InteractiveLevel;
			THROW(PsnEvObj.InitPacket(&pack, filt, 1));
			{
				PPID   id = 0;
				if(interactive_level < 2) {
					THROW(ok = PsnEvObj.PutPacket(&id, &pack, 1));
					if(interactive_level == 1 && filt.PrmrSCardCode) {
						ViewPersonInfoBySCard(filt.PrmrSCardCode);
					}
					ok = 1;
				}
				else {
					int    r = 0, valid_data = 0;
					LDATE  dt = ZERODATE;
					PsnEventDialog::Param param;
					PsnEventDialog::GetParam(pack.Rec.OpID, &param);
					THROW(CheckDialogPtr(&(p_dlg = new PsnEventDialog(&param, &PsnEvObj))));
					getcurdatetime(&dt, &pack.Rec.Tm);
					pack.Rec.Dt = (pack.Rec.Dt == ZERODATE) ? dt : pack.Rec.Dt;
					if(p_dlg->setDTS(&pack) > 0) {
						while(!valid_data && ExecView(p_dlg) == cmOK) {
							if(p_dlg->getDTS(&pack))
								valid_data = r = 1;
							else
								r = PPErrorZ();
						}
						if(r > 0) {
							THROW(ok = PsnEvObj.PutPacket(&id, &pack, 1));
							ok = 1;
						}
					}
					else
						PPError();
				}
			}
			static_cast<PPApp *>(APPL)->LastCmd = 0;
		}
	}
	CATCH
		if(interactive_level > 1) {
			PPError();
		}
		else if(interactive_level == 1) {
			PPErrorTooltip(-1, 0);
		}
		ok = 0;
	ENDCATCH
	delete p_dlg;
	return ok;
}

IMPLEMENT_CMD_HDL_FACTORY(ADDPERSONEVENT);
//
//
//
class CashNodeFiltDialog : public TDialog {
public:
	CashNodeFiltDialog() : TDialog(DLG_CASHPANEFLT), PrevCashNodeID(0)
	{
	}
	int    setDTS(const CashNodePaneFilt * pData);
	int    getDTS(CashNodePaneFilt * pData);
private:
	DECL_HANDLE_EVENT;
	int    SetupCommands(PPID cashNodeID, long commandID);
	long   ConvertCommand(PPID cashNodeID, long cmd, int toDlgCmd);

	PPID   PrevCashNodeID;
	CashNodePaneFilt Data;
	PPObjCashNode CashNObj;
};

IMPL_HANDLE_EVENT(CashNodeFiltDialog)
{
	TDialog::handleEvent(event);
	if(TVCOMMAND && TVCMD == cmCBSelected && event.isCtlEvent(CTLSEL_CASHPANEFLT_CNODE)) {
		PPID   cash_node_id = getCtrlLong(CTLSEL_CASHPANEFLT_CNODE);
		long   cmd_id = getCtrlLong(CTLSEL_CASHPANEFLT_CMD);
		SetupCommands(cash_node_id, cmd_id);
		clearEvent(event);
	}
}

int CashNodeFiltDialog::SetupCommands(PPID cashNodeID, long commandID)
{
	if(!PrevCashNodeID || PrevCashNodeID != cashNodeID) {
		PPCashNode    cnrec, prev_cnrec;
		MEMSZERO(cnrec);
		MEMSZERO(prev_cnrec);
		if(cashNodeID && CashNObj.Search(cashNodeID, &cnrec) > 0) {
			SString commands;
			if(PrevCashNodeID)
				CashNObj.Search(PrevCashNodeID, &prev_cnrec);
			if((prev_cnrec.Flags & (CASHF_SYNC|CASHF_ASYNC)) != (cnrec.Flags & (CASHF_SYNC|CASHF_ASYNC))) {
				commandID = (PrevCashNodeID) ? 0 : commandID;
				SetupStringCombo(this, CTLSEL_CASHPANEFLT_CMD, (cnrec.Flags & CASHF_SYNC) ? PPTXT_SYNCCASHNODECOMMANDS : PPTXT_ASYNCCASHNODECOMMANDS, 0);
			}
		}
		else
			commandID = 0;
		setCtrlData(CTLSEL_CASHPANEFLT_CMD, &commandID);
		disableCtrl(CTLSEL_CASHPANEFLT_CMD, cashNodeID == 0);
	}
	PrevCashNodeID = cashNodeID;
	return 1;
}

long CashNodeFiltDialog::ConvertCommand(PPID cashNodeID, long cmd, int toDlgCmd)
{
	uint   out_cmd = 0;
	if(cmd && cashNodeID) {
		PPCashNode cn_rec;
		MEMSZERO(cn_rec);
		if(CashNObj.Search(cashNodeID, &cn_rec) > 0) {
			static const uint async_cmds[] = { cmCSOpen, cmACSUpdate, cmCSClose, cmCSViewCheckList,  cmACSViewExcess };
			static const uint sync_cmds[] =  { cmCSOpen, cmCSClose, cmCSViewCheckList, cmSCSLock, cmSCSUnlock, cmSCSXReport, cmSCSZReportCopy, cmSCSIncasso };
			const  int  is_async = BIN(cn_rec.Flags & CASHF_ASYNC);
			const  uint count = is_async ? SIZEOFARRAY(async_cmds) : SIZEOFARRAY(sync_cmds);
			if(toDlgCmd) {
				cmd--;
				out_cmd = (cmd >= 0 && cmd < static_cast<long>(count)) ? ((is_async) ? async_cmds[cmd] : sync_cmds[cmd]) : 0;
			}
			else {
				for(uint i = 0; !out_cmd && i < count; i++)
					if(is_async && async_cmds[i] == cmd || (is_async == 0 && sync_cmds[i] == cmd))
						out_cmd = i + 1;
			}
		}
	}
	return out_cmd;
}

int CashNodeFiltDialog::setDTS(const CashNodePaneFilt * pData)
{
	if(!RVALUEPTR(Data, pData))
		Data.Init(1, 0);
	long   cmd = ConvertCommand(Data.CashNodeID, Data.CommandID, 0);
	SetupPPObjCombo(this, CTLSEL_CASHPANEFLT_CNODE, PPOBJ_CASHNODE, Data.CashNodeID, 0);
	SetupCommands(Data.CashNodeID, cmd);
	return 1;
}

int CashNodeFiltDialog::getDTS(CashNodePaneFilt * pData)
{
	Data.CashNodeID = getCtrlLong(CTLSEL_CASHPANEFLT_CNODE);
	long   cmd = getCtrlLong(CTLSEL_CASHPANEFLT_CMD);
	Data.CommandID = ConvertCommand(Data.CashNodeID, cmd, 1);
	ASSIGN_PTR(pData, Data);
	return 1;
}

class CMD_HDL_CLS(CASHNODEPANEL) : public PPCommandHandler {
public:
	SLAPI  CMD_HDL_CLS(CASHNODEPANEL)(const PPCommandDescr * pDescr) : PPCommandHandler(pDescr)
	{
	}
	virtual int SLAPI EditParam(SBuffer * pParam, long cmdID, void * extraPtr)
	{
		int    ok = -1, valid_data = 0;
		size_t sav_offs = 0;
		CashNodePaneFilt * p_filt = 0;
		CashNodeFiltDialog * p_dlg = 0;
		THROW_INVARG(pParam);
		THROW(CheckDialogPtr(&(p_dlg = new CashNodeFiltDialog)));
		sav_offs = pParam->GetRdOffs();
		THROW(PPView::ReadFiltPtr(*pParam, reinterpret_cast<PPBaseFilt **>(&p_filt)));
		if(!p_filt)
			THROW(PPView::CreateFiltInstance(PPFILT_CASHNODEPANE, reinterpret_cast<PPBaseFilt **>(&p_filt)) > 0);
		p_dlg->setDTS(p_filt);
		while(!valid_data && ExecView(p_dlg) == cmOK) {
			if(p_dlg->getDTS(p_filt) > 0)
				ok = valid_data = 1;
			else
				PPError();
		}
		if(ok > 0) {
			THROW(PPView::WriteFiltPtr(pParam->Z(), p_filt));
		}
		else
			pParam->SetRdOffs(sav_offs);
		CATCH
			CALLPTRMEMB(pParam, SetRdOffs(sav_offs));
			ok = 0;
		ENDCATCH
		delete p_dlg;
		ZDELETE(p_filt);
		return ok;
	}
	virtual int SLAPI Run(SBuffer * pParam, long cmdID, void * extraPtr)
	{
		int    ok = -1;
		if((D.MenuCm || cmdID) && APPL) {
			CashNodePaneFilt * p_filt = 0;
			static_cast<PPApp *>(APPL)->LastCmd = cmdID ? (cmdID + ICON_COMMAND_BIAS) : D.MenuCm;
			PPView::ReadFiltPtr(*pParam, reinterpret_cast<PPBaseFilt **>(&p_filt));
			ok = ExecCSPanel(p_filt);
			static_cast<PPApp *>(APPL)->LastCmd = 0;
		}
		return ok;
	}
};

IMPLEMENT_CMD_HDL_FACTORY(CASHNODEPANEL);
//
//
//
class AddBillFiltDlg : public TDialog {
public:
	struct Param {
		Param() : Bbt(0), OpID(0), LocID(0)
		{
		}
		int    SLAPI Read(SBuffer & rBuf, long)
		{
			int    ok = 1;
			THROW_SL(rBuf.Read(Bbt));
			THROW_SL(rBuf.Read(OpID));
			THROW_SL(rBuf.Read(LocID));
			CATCHZOK
			return ok;
		}
		int    SLAPI Write(SBuffer & rBuf, long)
		{
			int    ok = 1;
			THROW_SL(rBuf.Write(Bbt));
			THROW_SL(rBuf.Write(OpID));
			THROW_SL(rBuf.Write(LocID));
			CATCHZOK
			return ok;
		}
		PPID   Bbt;
		PPID   OpID;
		PPID   LocID;
	};

	static int OpTypeListByBbt(PPID bbt, PPIDArray * pOpTypeList);
	AddBillFiltDlg() : TDialog(DLG_ADDBILLFLT), PrevBbt(0)
	{
	}
	int    setDTS(const Param *);
	int    getDTS(Param *);
private:
	DECL_HANDLE_EVENT;
	int    SetupOprKindList(PPID opTypeID, PPID opID);
	PPID   PrevBbt;
	Param  Data;
};

IMPL_HANDLE_EVENT(AddBillFiltDlg)
{
	TDialog::handleEvent(event);
	if(event.isCbSelected(CTLSEL_ADDBILLFLT_BBT)) {
		PPID   bbt = getCtrlLong(CTLSEL_ADDBILLFLT_BBT);
		PPID   op_id = getCtrlLong(CTLSEL_ADDBILLFLT_OP);
		SetupOprKindList(bbt, op_id);
		clearEvent(event);
	}
}

int AddBillFiltDlg::OpTypeListByBbt(PPID bbt, PPIDArray * pOpTypeList)
{
	int    ok = 1;
	THROW_INVARG(pOpTypeList);
	if(bbt == bbtGoodsBills) {
		pOpTypeList->addzlist(PPOPT_GOODSRECEIPT, PPOPT_GOODSEXPEND, PPOPT_GOODSRETURN,
			PPOPT_GOODSREVAL, PPOPT_GOODSMODIF, PPOPT_PAYMENT, PPOPT_CORRECTION, 0L);
	}
	else if(bbt == bbtOrderBills)
		pOpTypeList->add(PPOPT_GOODSORDER);
	else if(bbt == bbtAccturnBills)
		pOpTypeList->add(PPOPT_ACCTURN);
	else if(bbt == bbtInventoryBills)
		pOpTypeList->add(PPOPT_INVENTORY);
	else if(bbt == bbtDraftBills)
		pOpTypeList->addzlist(PPOPT_DRAFTRECEIPT, PPOPT_DRAFTEXPEND, PPOPT_DRAFTQUOTREQ, 0L); // @v10.5.7 PPOPT_DRAFTQUOTREQ
	pOpTypeList->sort();
	CATCHZOK
	return ok;
}

int AddBillFiltDlg::SetupOprKindList(PPID bbt, PPID opID)
{
	int    r = 0;
	PPID   op_id = 0;
	PPIDArray op_type_list;
	PPIDArray op_list;
	PPOprKind opk_rec;
	bbt--;
	AddBillFiltDlg::OpTypeListByBbt(bbt, &op_type_list);
	while((r = EnumOperations(0, &op_id, &opk_rec)) > 0)
		if(op_type_list.bsearch(opk_rec.OpTypeID, 0) > 0)
			op_list.add(op_id);
	opID = (PrevBbt == 0 || PrevBbt == bbt) ? opID : 0;
	PrevBbt = bbt;
	SetupOprKindCombo(this, CTLSEL_ADDBILLFLT_OP, opID, 0, &op_list, OPKLF_OPLIST);
	return 1;
}

int AddBillFiltDlg::setDTS(const Param * pData)
{
	PPID   bbt = 0;
	if(!RVALUEPTR(Data, pData))
		MEMSZERO(Data);
	bbt = (Data.Bbt >= 0) ? Data.Bbt + 1 : 0;
	SetupPPObjCombo(this, CTLSEL_ADDBILLFLT_LOC, PPOBJ_LOCATION, Data.LocID, 0);
	SetupStringCombo(this, CTLSEL_ADDBILLFLT_BBT, PPTXT_BILLTYPES, bbt);
	SetupOprKindList(bbt, Data.OpID);
	return 1;
}

int AddBillFiltDlg::getDTS(Param * pData)
{
	getCtrlData(CTLSEL_ADDBILLFLT_OP,  &Data.OpID);
	getCtrlData(CTLSEL_ADDBILLFLT_LOC, &Data.LocID);
	getCtrlData(CTLSEL_ADDBILLFLT_BBT, &Data.Bbt);
	Data.Bbt--;
	ASSIGN_PTR(pData, Data);
	return 1;
}

class CMD_HDL_CLS(ADDBILL) : public PPCommandHandler {
public:
	SLAPI  CMD_HDL_CLS(ADDBILL)(const PPCommandDescr * pDescr) : PPCommandHandler(pDescr)
	{
	}
	virtual int SLAPI EditParam(SBuffer * pParam, long cmdID, void * extraPtr)
	{
		int    ok = -1, valid_data = 0;
		size_t sav_offs = 0;
		AddBillFiltDlg::Param filt;
		AddBillFiltDlg * p_dlg = 0;

		THROW_INVARG(pParam);
		THROW(CheckDialogPtr(&(p_dlg = new AddBillFiltDlg)));
		sav_offs = pParam->GetRdOffs();
		filt.Read(*pParam, 0);
		p_dlg->setDTS(&filt);
		while(!valid_data && ExecView(p_dlg) == cmOK) {
			if(p_dlg->getDTS(&filt) > 0)
				ok = valid_data = 1;
			else
				PPError();
		}
		if(ok > 0) {
			THROW(filt.Write(pParam->Z(), 0));
		}
		else
			pParam->SetRdOffs(sav_offs);
		CATCH
			CALLPTRMEMB(pParam, SetRdOffs(sav_offs));
			ok = 0;
		ENDCATCH
		delete p_dlg;
		return ok;
	}
	virtual int SLAPI Run(SBuffer * pParam, long cmdID, void * extraPtr)
	{
		int    ok = -1;
		if(APPL) {
			if(D.MenuCm || cmdID) {
				int    r = 1;
				SBuffer param;
				AddBillFiltDlg::Param filt;
				static_cast<PPApp *>(APPL)->LastCmd = (cmdID) ? (cmdID + ICON_COMMAND_BIAS) : D.MenuCm;
				RVALUEPTR(param, pParam);
				if(!param.GetAvailableSize()) {
					if(EditParam(&param, cmdID, extraPtr) > 0) {
						filt.Read(param, 0);
						r = 1;
					}
					else
						r = -1;
				}
				else
					filt.Read(param, 0);
				if(r > 0 && filt.Bbt >= 0) {
					if(!filt.LocID || !filt.OpID) {
						PPIDArray op_type_list;
						SETIFZ(filt.LocID, LConfig.Location);
						AddBillFiltDlg::OpTypeListByBbt(filt.Bbt, &op_type_list);
						r = BillPrelude(&op_type_list, 0, 0, &filt.OpID, &filt.LocID);
					}
					if(r > 0) {
						PPID   id = 0;
						PPObjBill * p_bobj = BillObj;
						const  PPID save_loc_id = LConfig.Location;
						DS.SetLocation(filt.LocID);
						if(GetOpType(filt.OpID) == PPOPT_ACCTURN && !CheckOpFlags(filt.OpID, OPKF_EXTACCTURN))
							r = p_bobj->AddGenAccturn(&id, filt.OpID, 0);
						else {
							BillFilt bill_filt;
							bill_filt.SetupBrowseBillsType(static_cast<BrowseBillsType>(filt.Bbt));
							bill_filt.OpID = filt.OpID;
							bill_filt.LocList.Add(filt.LocID);
							r = p_bobj->AddGoodsBillByFilt(&id, &bill_filt, filt.OpID);
						}
						DS.SetLocation(save_loc_id);
						ok = (r == cmOK) ? 1 : -1;
					}
				}
			}
			static_cast<PPApp *>(APPL)->LastCmd = 0;
		}
		return ok;
	}
};

IMPLEMENT_CMD_HDL_FACTORY(ADDBILL);

int SLAPI SearchDlvrAddr()
{
	int    ok = -1;
	SString title, inp_title, srch_str;
	PPInputStringDialogParam isd_param;
	PPLoadText(PPTXT_SRCHDLVRADDRTITLE, isd_param.Title);
	PPLoadText(PPTXT_SRCHDLVRADDRINPUTTITLE, isd_param.InputTitle);
	for(int valid_data = 0; !valid_data && InputStringDialog(&isd_param, srch_str) > 0;) {
		if(srch_str.Len()) {
			int r = 0;
			PPID id = 0;
			PPObjLocation loc_obj;
			if(srch_str.C(0) == '#') {
				id = srch_str.ShiftLeft().ToLong();
				if(id != 0)
					r = loc_obj.Search(id, 0);
			}
			else
				r = loc_obj.P_Tbl->SearchCode(LOCTYP_ADDRESS, srch_str, &id, 0);
			if(r > 0) {
				ok = BIN(loc_obj.Edit(&id, 0) != 0);
				valid_data = 1;
			}
		}
	}
	return ok;
}

class CMD_HDL_CLS(SEARCHDLVRADDR) : public PPCommandHandler {
public:
	SLAPI  CMD_HDL_CLS(SEARCHDLVRADDR)(const PPCommandDescr * pDescr) : PPCommandHandler(pDescr)
	{
	}
	virtual int SLAPI EditParam(SBuffer * pParam, long cmdID, void * extraPtr)
	{
		return -1;
	}
	virtual int SLAPI Run(SBuffer * pParam, long cmdID, void * extraPtr)
	{
		int    ok = -1;
		PPApp * p_app = static_cast<PPApp *>(APPL);
		if(p_app) {
			if(D.MenuCm || cmdID) {
				p_app->LastCmd = (cmdID) ? (cmdID + ICON_COMMAND_BIAS) : D.MenuCm;
				ok = SearchDlvrAddr();
			}
			p_app->LastCmd = 0;
		}
		return ok;
	}
};

IMPLEMENT_CMD_HDL_FACTORY(SEARCHDLVRADDR);

class TSessCreateFilt { // @persistent
public:
	static const char * P_PrivSign;

	TSessCreateFilt() : PrivateVer(2), GrpID(0), PrcID(0), Status(0), Reserve(0)
	{
		memcpy(PrivateSignature, P_PrivSign, sizeof(PrivateSignature));
		memzero(Reserve2, sizeof(Reserve2));
	}
	int    SLAPI Write(SBuffer & rBuf, long) const
	{
		//return (rBuf.Write(GrpID) && rBuf.Write(PrcID) && rBuf.Write(Status) && rBuf.Write(Reserve)) ? 1 : PPSetErrorSLib();
		int    ok = 1;
		THROW_SL(rBuf.Write(PrivateSignature, sizeof(PrivateSignature)));
		THROW_SL(rBuf.Write(PrivateVer));
		THROW_SL(rBuf.Write(GrpID));
		THROW_SL(rBuf.Write(PrcID));
		THROW_SL(rBuf.Write(Status));
		THROW_SL(rBuf.Write(Reserve));
		THROW_SL(rBuf.Write(Reserve2, sizeof(Reserve2)));
		THROW_SL(rBuf.Write(WtmFileName));
		CATCHZOK
		return ok;
	}
	int    SLAPI Read(SBuffer & rBuf, long)
	{
		int    ok = 1;
		if(rBuf.GetAvailableSize()) {
			THROW_SL(rBuf.Read(PrivateSignature, sizeof(PrivateSignature)));
			if(memcmp(PrivateSignature, P_PrivSign, sizeof(PrivateSignature)) == 0) {
				THROW_SL(rBuf.Read(PrivateVer));
				THROW_SL(rBuf.Read(GrpID));
				THROW_SL(rBuf.Read(PrcID));
				THROW_SL(rBuf.Read(Status));
				THROW_SL(rBuf.Read(Reserve));
				THROW_SL(rBuf.Read(Reserve2, sizeof(Reserve2)));
				THROW_SL(rBuf.Read(WtmFileName));
			}
			else {
				THROW_SL(rBuf.Unread(sizeof(PrivateSignature)));
				memcpy(PrivateSignature, P_PrivSign, sizeof(PrivateSignature));
				PrivateVer = 2;
				THROW_SL(rBuf.Read(GrpID));
				THROW_SL(rBuf.Read(PrcID));
				THROW_SL(rBuf.Read(Status));
				THROW_SL(rBuf.Read(Reserve));
			}
		}
		CATCHZOK
		return ok;
	}
	char  PrivateSignature[8]; // "TSESCR\x20\x20"
	int32 PrivateVer;
	PPID  GrpID;
	PPID  PrcID;
	int16 Status;
	int16 Reserve;
	uint8 Reserve2[32];
	SString WtmFileName;
};

//static
const char * TSessCreateFilt::P_PrivSign = "TSESCR\x20\x20";

#define GRP_CRTTSESSFLT_WTM 1

class CMD_HDL_CLS(CREATETECHSESS) : public PPCommandHandler {
public:
	SLAPI  CMD_HDL_CLS(CREATETECHSESS)(const PPCommandDescr * pDescr) : PPCommandHandler(pDescr)
	{
	}
	virtual int SLAPI EditParam(SBuffer * pParam, long cmdID, void * extraPtr)
	{
		int    ok = -1;
		size_t sav_offs = 0;
		TDialog * p_dlg = new TDialog(DLG_CRTTSESSFLT);
		TSessCreateFilt filt;
		THROW_INVARG(pParam);
		sav_offs = pParam->GetRdOffs();
		filt.Read(*pParam, 0);
		THROW(CheckDialogPtr(&p_dlg));
		FileBrowseCtrlGroup::Setup(p_dlg, CTLBRW_CRTTSESSFLT_WTM, CTL_CRTTSESSFLT_WTM, GRP_CRTTSESSFLT_WTM, 0, PPTXT_FILPAT_WTM,
			FileBrowseCtrlGroup::fbcgfFile);
		SetupPPObjCombo(p_dlg, CTLSEL_CRTTSESSFLT_GRP, PPOBJ_PROCESSOR, filt.GrpID, OLW_CANINSERT|OLW_CANSELUPLEVEL, reinterpret_cast<void *>(PRCEXDF_GROUP));
		SetupPPObjCombo(p_dlg, CTLSEL_CRTTSESSFLT_PRC, PPOBJ_PROCESSOR, filt.PrcID, OLW_CANINSERT);
		p_dlg->AddClusterAssoc(CTL_CRTTSESSFLT_STATUS,    0, TSESST_PLANNED);
		p_dlg->AddClusterAssocDef(CTL_CRTTSESSFLT_STATUS, 1, TSESST_PENDING);
		p_dlg->AddClusterAssoc(CTL_CRTTSESSFLT_STATUS,    2, TSESST_INPROCESS);
		p_dlg->SetClusterData(CTL_CRTTSESSFLT_STATUS, filt.Status);
		if(!filt.WtmFileName.NotEmptyS()) {
			FileBrowseCtrlGroup * p_fbg = static_cast<FileBrowseCtrlGroup *>(p_dlg->getGroup(GRP_CRTTSESSFLT_WTM));
			if(p_fbg) {
				SString path;
				PPGetPath(PPPATH_WTM, path);
				p_fbg->setInitPath(path);
			}
		}
		p_dlg->setCtrlString(CTL_CRTTSESSFLT_WTM, filt.WtmFileName);
		for(int valid_data = 0; !valid_data && ExecView(p_dlg) == cmOK;) {
			p_dlg->getCtrlData(CTLSEL_CRTTSESSFLT_GRP,    &filt.GrpID);
			p_dlg->getCtrlData(CTLSEL_CRTTSESSFLT_PRC,    &filt.PrcID);
			p_dlg->GetClusterData(CTL_CRTTSESSFLT_STATUS, &filt.Status);
			p_dlg->getCtrlString(CTL_CRTTSESSFLT_WTM, filt.WtmFileName);
			if(filt.GrpID || filt.PrcID || fileExists(filt.WtmFileName))
				valid_data = ok = 1;
			else
				PPError(PPERR_PRCORGRPNOTDEF);
		}
		if(ok > 0) {
			THROW(filt.Write(pParam->Z(), 0));
		}
		else
			pParam->SetRdOffs(sav_offs);
		CATCH
			CALLPTRMEMB(pParam, SetRdOffs(sav_offs));
			ok = 0;
		ENDCATCH
		delete p_dlg;
		return ok;
	}
	virtual int SLAPI Run(SBuffer * pParam, long cmdID, void * extraPtr)
	{
		int    ok = -1;
		PPApp * p_app = static_cast<PPApp *>(APPL);
		if(p_app) {
			if(D.MenuCm || cmdID) {
				int    r = 1;
				SBuffer param;
				TSessCreateFilt filt;
				p_app->LastCmd = (cmdID) ? (cmdID + ICON_COMMAND_BIAS) : D.MenuCm;
				THROW_INVARG(pParam);
				param = *pParam;
				if(!param.GetAvailableSize()) {
					if(EditParam(&param, cmdID, extraPtr) > 0) {
						filt.Read(param, 0);
						r = 1;
					}
					else
						r = -1;
				}
				else
					filt.Read(param, 0);
				if(r > 0) {
					if(filt.WtmFileName.NotEmpty() && fileExists(filt.WtmFileName)) {
						TWhatmanObject::SelectObjRetBlock sel_blk;
						if(PPWhatmanWindow::Launch(filt.WtmFileName, 0, &sel_blk) > 0 && sel_blk.Val1 == PPOBJ_PROCESSOR && sel_blk.Val2 > 0)
							filt.PrcID = sel_blk.Val2;
						else
							r = -1;
					}
					if(r > 0) {
						PPID   id = 0;
						PPObjTSession obj_tsess;
						if(!filt.PrcID) {
							SString title;
							SString name;
							PPIDArray child_list;
							PPObjProcessor 	obj_prcssr;
							StrAssocArray prcssr_list;
							r = 0;
							// @v9.0.2 PPGetWord(PPWORD_PRC, 0, title);
							PPLoadString("processor", title); // @v9.0.2
							obj_prcssr.GetChildIDList(filt.GrpID, 0, &child_list);
							for(uint i = 0; i < child_list.getCount(); i++) {
								GetObjectName(PPOBJ_PROCESSOR, child_list.at(i), name = 0);
								prcssr_list.Add(child_list.at(i), 0, name);
							}
							for(int valid_data = 0; !valid_data && ListBoxSelDialog(&prcssr_list, title, &filt.PrcID, 0) > 0;) {
								if(filt.PrcID > 0)
									valid_data = r = 1;
								else
									PPError(PPERR_PRCNEEDED);
							}
						}
						if(r > 0)
							ok = obj_tsess.Add(&id, 0, filt.PrcID, TSESK_SESSION, filt.Status);
					}
				}
			}
			p_app->LastCmd = 0;
		}
		CATCHZOK
		return ok;
	}
};

IMPLEMENT_CMD_HDL_FACTORY(CREATETECHSESS);

class CMD_HDL_CLS(CTBLORDCREATE) : public PPCommandHandler {
public:
	SLAPI  CMD_HDL_CLS(CTBLORDCREATE)(const PPCommandDescr * pDescr) : PPCommandHandler(pDescr)
	{
	}
	virtual int SLAPI EditParam(SBuffer * pParam, long cmdID, void * extraPtr)
	{
		int    ok = -1;
		size_t sav_offs = 0;
		CTableOrder cto;
		CTableOrder::Param param;
		SSerializeContext ctx;
		THROW_INVARG(pParam);
		sav_offs = pParam->GetRdOffs();
		if(pParam->GetAvailableSize())
			THROW(param.Serialize(-1, *pParam, &ctx));
		THROW(ok = cto.EditParam(&param));
		if(ok > 0) {
			THROW(param.Serialize(+1, pParam->Z(), &ctx));
		}
		else
			pParam->SetRdOffs(sav_offs);
		CATCH
			CALLPTRMEMB(pParam, SetRdOffs(sav_offs));
			ok = 0;
		ENDCATCH
		return ok;
	}
	virtual int SLAPI Run(SBuffer * pParam, long cmdID, void * extraPtr)
	{
		int    ok = -1;
		PPApp * p_app = static_cast<PPApp *>(APPL);
		if(p_app) {
			if(D.MenuCm || cmdID) {
				int    r = 1;
				SBuffer param_buf;
				CTableOrder::Param param;
				p_app->LastCmd = (cmdID) ? (cmdID + ICON_COMMAND_BIAS) : D.MenuCm;
				THROW_INVARG(pParam);
				param_buf = *pParam;
				if(param_buf.GetAvailableSize()) {
					SSerializeContext ctx;
					THROW(param.Serialize(-1, param_buf, &ctx));
				}
				if(param.Flags & CTableOrder::Param::fShowTimeGraph) {
					CTableOrder::ShowTimeGraph(param.PosNodeID, 1);
				}
				else {
					CTableOrder cto;
					THROW(cto.Create(&param));
				}
			}
			p_app->LastCmd = 0;
		}
		CATCHZOK
		return ok;
	}
};

IMPLEMENT_CMD_HDL_FACTORY(CTBLORDCREATE);

class CMD_HDL_CLS(UPDATEQUOTS) : public PPCommandHandler {
public:
	SLAPI  CMD_HDL_CLS(UPDATEQUOTS)(const PPCommandDescr * pDescr) : PPCommandHandler(pDescr)
	{
	}
	virtual int SLAPI EditParam(SBuffer * pParam, long cmdID, void * extraPtr)
	{
		int    ok = -1;
		size_t sav_offs = 0;
		QuotUpdFilt filt;
		THROW_INVARG(pParam);
		sav_offs = pParam->GetRdOffs();
		if(!pParam->GetAvailableSize()) {
			filt.LocList.Add(LConfig.Location);
			filt.Flags |= QuotUpdFilt::fExistOnly;
		}
		else
			filt.Read(*pParam, 0);
		if(EditQuotUpdDialog(&filt) > 0) {
			THROW(filt.Write(pParam->Z(), 0));
			ok = 1;
		}
		else
			pParam->SetRdOffs(sav_offs);
		CATCH
			CALLPTRMEMB(pParam, SetRdOffs(sav_offs));
			ok = 0;
		ENDCATCH
		return ok;
	}
	virtual int SLAPI Run(SBuffer * pParam, long cmdID, void * extraPtr)
	{
		int    ok = -1;
		PPApp * p_app = static_cast<PPApp *>(APPL);
		if(p_app) {
			if(D.MenuCm || cmdID) {
				int  r = 1;
				QuotUpdFilt filt;
				SBuffer param;
				p_app->LastCmd = (cmdID) ? (cmdID + ICON_COMMAND_BIAS) : D.MenuCm;
				THROW_INVARG(pParam);
				param = *pParam;
				if(!param.GetAvailableSize()) {
					if(EditParam(&param, cmdID, extraPtr) > 0) {
						filt.Read(param, 0);
						r = 1;
					}
					else
						r = -1;
				}
				else
					filt.Read(param, 0);
				if(r > 0)
					UpdateQuots(&filt);
			}
			p_app->LastCmd = 0;
		}
		CATCHZOK
		return ok;
	}
private:
};

IMPLEMENT_CMD_HDL_FACTORY(UPDATEQUOTS);

class CMD_HDL_CLS(TRANSMITMODIFICATIONS) : public PPCommandHandler {
public:
	SLAPI  CMD_HDL_CLS(TRANSMITMODIFICATIONS)(const PPCommandDescr * pDescr) : PPCommandHandler(pDescr)
	{
	}
	virtual int SLAPI EditParam(SBuffer * pParam, long cmdID, void * extraPtr)
	{
		int    ok = -1;
		size_t sav_offs = 0;
		ObjTransmitParam trnsm_param;
		QuotUpdFilt filt;
		THROW_INVARG(pParam);

		sav_offs = pParam->GetRdOffs();
		if(pParam->GetAvailableSize())
			trnsm_param.Read(*pParam, 0);
		if(ObjTransmDialog(DLG_MODTRANSM, &trnsm_param, OBJTRNSMDLGF_SEARCHDTTM) > 0) {
			THROW(trnsm_param.Write(pParam->Z(), 0));
			ok = 1;
		}
		else
			pParam->SetRdOffs(sav_offs);
		CATCH
			CALLPTRMEMB(pParam, SetRdOffs(sav_offs));
			ok = 0;
		ENDCATCH
		return ok;
	}
	virtual int SLAPI Run(SBuffer * pParam, long cmdID, void * extraPtr)
	{
		int    ok = -1;
		if(APPL) {
			if(D.MenuCm || cmdID) {
				int  r = 1;
				ObjTransmitParam trnsm_param;
				SBuffer param;
				static_cast<PPApp *>(APPL)->LastCmd = (cmdID) ? (cmdID + ICON_COMMAND_BIAS) : D.MenuCm;
				THROW_INVARG(pParam);
				param = *pParam;
				if(!param.GetAvailableSize()) {
					if(EditParam(&param, cmdID, extraPtr) > 0) {
						trnsm_param.Read(param, 0);
						r = 1;
					}
					else
						r = -1;
				}
				else
					trnsm_param.Read(param, 0);
				if(r > 0)
					PPObjectTransmit::TransmitModificationsByDBDivList(&trnsm_param);
			}
			static_cast<PPApp *>(APPL)->LastCmd = 0;
		}
		CATCHZOK
		return ok;
	}
private:
};

IMPLEMENT_CMD_HDL_FACTORY(TRANSMITMODIFICATIONS);
//
//
//
// prototype @defined(filtrnsm.cpp)
int SLAPI EditObjReceiveParam(ObjReceiveParam * pParam, int editOptions);

class CMD_HDL_CLS(RECEIVEPACKETS) : public PPCommandHandler {
public:
	SLAPI  CMD_HDL_CLS(RECEIVEPACKETS)(const PPCommandDescr * pDescr) : PPCommandHandler(pDescr)
	{
	}
	virtual int SLAPI EditParam(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = -1, r;
		ObjReceiveParam param;
		param.Init();
		size_t sav_offs = pParam->GetRdOffs();
		if((r = param.Read(*pParam, 0)) != 0) {
			ok = EditObjReceiveParam(&param, 1);
			if(ok > 0) {
				param.Write(pParam->Z(), 0);
			}
			else if(r > 0)
				pParam->SetRdOffs(sav_offs);
		}
		return ok;
	}
	virtual int SLAPI Run(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = -1;
		ObjReceiveParam param;
		if(param.Read(*pParam, 0) > 0)
			param.Flags |= ObjReceiveParam::fNonInteractive;
		else
			param.Init();
		ok = PPObjectTransmit::ReceivePackets(&param);
		return ok;
	}
};

IMPLEMENT_CMD_HDL_FACTORY(RECEIVEPACKETS);
//
//
//
class CMD_HDL_CLS(SEARCHBILLBYCTX) : public PPCommandHandler {
public:
	SLAPI  CMD_HDL_CLS(SEARCHBILLBYCTX)(const PPCommandDescr * pDescr) : PPCommandHandler(pDescr)
	{
	}
	virtual int SLAPI EditParam(SBuffer * pParam, long, void * extraPtr)
	{
		return -1;
	}
	virtual int SLAPI Run(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = -1;
		size_t sav_offs = 0;
		if(D.MenuCm && APPL) {
			int    ok = -1, r = cmCancel;
			SString srch_str;
			LongArray dt_list;
			StrAssocArray codes_list;
			PPInputStringDialogParam isd_param;
			THROW_INVARG(pParam);
			sav_offs = pParam->GetRdOffs();
			pParam->Read(srch_str);
			static_cast<PPApp *>(APPL)->LastCmd = D.MenuCm;
			isd_param.InputTitle = PPLoadTextS(PPTXT_BILLSRCHCTX, isd_param.Title);
			if(InputStringDialog(&isd_param, srch_str) > 0 && srch_str.Len()) {
				SString srch_str2;
				SString temp_buf;
				(srch_str2 = srch_str).Transf(CTRANSF_INNER_TO_OUTER);
				//
				// ѕоиск дат
				//
				const char * p_dt_pattern = "[0-3]?[0-9][/.-][0-1]?[0-9][/.-][0-9]?[0-9]?[0-9]?[0-9]";
				CRegExp expr(p_dt_pattern);
				SStrScan scan(srch_str2);
				while(expr.Find(&scan) > 0) {
					DateRange period;
					scan.Get(temp_buf.Z());
					if(strtoperiod(temp_buf, &period, 0)) {
						if(checkdate(period.low, 1)) {
							LDATE dt_ = period.low.getactual(ZERODATE);
							dt_list.add(dt_.v);
						}
					}
					srch_str2.Excise(scan.Offs, scan.Len);
					scan.Offs = 0;
					scan.Len  = 0;
				}
				//
				// ѕоиск номеров документов
				//
				const char * p_code_pattern = "[^ ,][^ ,]+";
				expr.Compile(p_code_pattern);
				scan.Offs = 0;
				scan.Len  = 0;
				while(expr.Find(&scan) > 0) {
					int has_digit = 0;
					scan.Get(temp_buf.Z());
					for(uint i = 0; !has_digit && i < temp_buf.Len(); i++)
						if(temp_buf.C(i) > 47 && temp_buf.C(i) < 58)
							has_digit = 1;
					if(has_digit)
						codes_list.Add(codes_list.getCount(), 0, temp_buf.ToOem());
					srch_str2.Excise(scan.Offs, scan.Len);
					scan.Offs = 0;
					scan.Len  = 0;
				}
				THROW(pParam->Z().Write(srch_str));
			}
			else
				pParam->SetRdOffs(sav_offs);
			if(dt_list.getCount()) {
				BillFilt   filt;
				uint dts_count = dt_list.getCount();
				for(uint i = 0; i < dts_count; i++)  {
					LDATE dt = ZERODATE;
					DateIter di;
					BillTbl::Rec bill_rec;
					dt.v = dt_list.at(i);
					di.dt  = dt;
					di.end = dt;
					while(BillObj->P_Tbl->EnumByDate(&di, &bill_rec) > 0 && bill_rec.Dt == dt)
						if(codes_list.SearchByText(bill_rec.Code, 0, 0) > 0)
							filt.List.Add(bill_rec.ID);
				}
				if(filt.List.GetCount())
					ok = PPView::Execute(PPVIEW_BILL, &filt, 1, 0);
			}
		}
		CATCH
			CALLPTRMEMB(pParam, SetRdOffs(sav_offs));
			ok = 0;
		ENDCATCH
		static_cast<PPApp *>(APPL)->LastCmd = 0;
		return ok;
	}
};

IMPLEMENT_CMD_HDL_FACTORY(SEARCHBILLBYCTX);
//
//
//
class CMD_HDL_CLS(WRITEOFFDRAFTS) : public PPCommandHandler {
public:
	SLAPI  CMD_HDL_CLS(WRITEOFFDRAFTS)(const PPCommandDescr * pDescr) : PPCommandHandler(pDescr)
	{
	}
	virtual int SLAPI EditParam(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = -1;
		PrcssrWrOffDraftFilt filt; // PPBaseFilt
		if(pParam) {
			if(!filt.Read(*pParam, 0))
				filt.Init(1, 0);
			if(Prcssr.EditParam(&filt) > 0) {
				if(filt.Write(*pParam, 0)) {
					ok = 1;
				}
			}
		}
		return ok;
	}
	virtual int SLAPI Run(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = -1;
		PrcssrWrOffDraftFilt filt, * p_filt = 0;
		if(pParam) {
			if(filt.Read(*pParam, 0))
				p_filt = &filt;
			else
				PPError();
		}
		if(!p_filt && Prcssr.EditParam(&filt) > 0) {
			p_filt = &filt;
		}
		if(p_filt)
			ok = Prcssr.Init(p_filt) ? Prcssr.Run() : PPErrorZ();
		return ok;
	}
private:
	PrcssrWrOffDraft Prcssr;
};

IMPLEMENT_CMD_HDL_FACTORY(WRITEOFFDRAFTS);
//
//
//
class CMD_HDL_CLS(INFOKIOSK) : public PPCommandHandler {
public:
	SLAPI  CMD_HDL_CLS(INFOKIOSK)(const PPCommandDescr * pDescr) : PPCommandHandler(pDescr)
	{
	}
	virtual int SLAPI EditParam(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = -1;
		InfoKioskPaneFilt filt;
		if(pParam) {
			if(!filt.Read(*pParam, 0))
				filt.Init(1, 0);
			if(Obj.EditInfoKioskPaneFilt(&filt) > 0) {
				if(filt.Write(pParam->Z(), 0)) {
					ok = 1;
				}
			}
		}
		return ok;
	}
	virtual int SLAPI Run(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = -1;
		InfoKioskPaneFilt filt, * p_filt = 0;
		if(pParam) {
			if(filt.Read(*pParam, 0))
				p_filt = &filt;
			else
				PPError();
		}
		return ViewGoodsInfo(p_filt);
	}
private:
	PPObjGoodsInfo Obj;
};

IMPLEMENT_CMD_HDL_FACTORY(INFOKIOSK);
//
//
//
class CMD_HDL_CLS(UNIFYGOODSPRICE) : public PPCommandHandler {
public:
	SLAPI  CMD_HDL_CLS(UNIFYGOODSPRICE)(const PPCommandDescr * pDescr) : PPCommandHandler(pDescr)
	{
	}
	virtual int SLAPI EditParam(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = -1;
		if(pParam) {
			PrcssrUnifyPrice prc_uniprice;
			PrcssrUnifyPriceFilt filt;
			if(!filt.Read(*pParam, 0))
				filt.Init(1, 0);
			if(prc_uniprice.EditParam(&filt) > 0) {
				if(filt.Write(pParam->Z(), 0)) {
					ok = 1;
				}
			}
		}
		return ok;
	}
	virtual int SLAPI Run(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = -1;
		if(pParam) {
			PrcssrUnifyPriceFilt filt;
			if(filt.Read(*pParam, 0)) {
				PrcssrUnifyPrice prc_uniprice;
				if(!prc_uniprice.Process(&filt))
					ok = PPErrorZ();
			}
			else
				ok = UnifyGoodsPrice();
		}
		else
			ok = UnifyGoodsPrice();
		return ok;
	}
};

IMPLEMENT_CMD_HDL_FACTORY(UNIFYGOODSPRICE);
//
//
//
int SLAPI FASTCALL WriteParam(SBuffer & rBuf, const void * pParam, size_t paramSize); // @prototype(ppjob.cpp)
int SLAPI FASTCALL ReadParam(SBuffer & rBuf, void * pParam, size_t paramSize);  // @prototype(ppjob.cpp)

class CMD_HDL_CLS(TESTPREDICTSALESTBL) : public PPCommandHandler {
public:
	SLAPI  CMD_HDL_CLS(TESTPREDICTSALESTBL)(const PPCommandDescr * pDescr) : PPCommandHandler(pDescr)
	{
	}
	virtual int SLAPI EditParam(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = -1, r = 0;
		PrcssrPrediction::Param param;
		PrcssrPrediction prcssr;
		MEMSZERO(param);
		size_t sav_offs = pParam->GetRdOffs();
		if((r = ReadParam(*pParam, &param, sizeof(param))) != 0) {
			param.Process |= param.prcsTest;
			if((ok = prcssr.EditParam(&param)) > 0) {
				WriteParam(pParam->Z(), &param, sizeof(param));
			}
			else if(r > 0)
				pParam->SetRdOffs(sav_offs);
		}
		return ok;
	}
	virtual int SLAPI Run(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = -1;
		PrcssrPrediction::Param param;
		PrcssrPrediction prcssr;
		if(ReadParam(*pParam, &param, sizeof(param))) {
			param.Process |= param.prcsTest;
			prcssr.Init(&param);
			ok = prcssr.Run();
		}
		else
			ok = 0;
		return ok;
	}
};

IMPLEMENT_CMD_HDL_FACTORY(TESTPREDICTSALESTBL);
//
//
//
class CMD_HDL_CLS(EXPORTGOODSRESTUHTT) : public PPCommandHandler {
public:
	SLAPI  CMD_HDL_CLS(EXPORTGOODSRESTUHTT)(const PPCommandDescr * pDescr) : PPCommandHandler(pDescr)
	{
	}
	virtual int SLAPI EditParam(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = -1;
		uint   val = 0;
		PPViewGoodsRest gr_view;
		GoodsRestFilt filt;
		size_t sav_offs = pParam->GetRdOffs();
		if(pParam->GetAvailableSize() == 0) {
			;
		}
		else {
			filt.Read(*pParam, 0);
		}
		if(gr_view.EditBaseFilt(&filt) > 0) {
			if(filt.Write(pParam->Z(), 0))
				ok = 1;
		}
		else
			pParam->SetRdOffs(sav_offs);
		return ok;
	}
	virtual int SLAPI Run(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = -1;
		if(pParam->GetAvailableSize() != 0) {
			PPViewGoodsRest gr_view;
			GoodsRestFilt filt;
			THROW(filt.Read(*pParam, 0));
			THROW(gr_view.Init_(&filt));
			THROW(gr_view.ExportUhtt(1));
			ok = 1;
		}
		CATCHZOK
		return ok;
	}
};

IMPLEMENT_CMD_HDL_FACTORY(EXPORTGOODSRESTUHTT);
//
//
//
class CMD_HDL_CLS(IMPORTBILLS) : public PPCommandHandler {
public:
	SLAPI  CMD_HDL_CLS(IMPORTBILLS)(const PPCommandDescr * pDescr) : PPCommandHandler(pDescr)
	{
	}
	virtual int SLAPI EditParam(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = -1, r = 1;
		uint   val = 0;
		SSerializeContext sctx;
		PPBillImpExpBaseProcessBlock blk;
		size_t sav_offs = pParam->GetRdOffs();
		if(pParam->GetAvailableSize() == 0) {
			;
		}
		else {
			THROW(blk.SerializeParam(-1, *pParam, &sctx));
		}
		if(blk.Select(1) > 0) {
			THROW(blk.SerializeParam(+1, pParam->Z(), &sctx));
			ok = 1;
		}
		else
			pParam->SetRdOffs(sav_offs);
		CATCHZOKPPERR
		return ok;
	}
	virtual int SLAPI Run(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = -1;
		if(pParam->GetAvailableSize()) {
			SSerializeContext sctx;
			PPBillImporter prcssr;
			prcssr.Init();
			THROW(prcssr.SerializeParam(-1, *pParam, &sctx));
			THROW(prcssr.LoadConfig(1));
			THROW(prcssr.Run());
			ok = 1;
		}
		else {
			ok = ImportBills(0, 0, 0, 0);
		}
		CATCHZOK
		return ok;
	}
};

IMPLEMENT_CMD_HDL_FACTORY(IMPORTBILLS);
//
//
//
#if 0 // @construction {
class CMD_HDL_CLS(IMPORTGOODS) : public PPCommandHandler {
public:
	SLAPI  CMD_HDL_CLS(IMPORTGOODS)(const PPCommandDescr * pDescr) : PPCommandHandler(pDescr)
	{
	}
	virtual int SLAPI EditParam(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = -1, r = 1;
		uint   val = 0;
		SSerializeContext sctx;
		PPGoodsImporter prcssr;
		size_t sav_offs = pParam->GetRdOffs();
		if(pParam->GetAvailableSize() == 0) {
			;
		}
		else {
			THROW(prcssr.SerializeParam(-1, *pParam, &sctx));
		}
		if(prcssr.Select(1) > 0) {
			THROW(blk.SerializeParam(+1, pParam->Clear(), &sctx));
			ok = 1;
		}
		else
			pParam->SetRdOffs(sav_offs);
		CATCHZOKPPERR
		return ok;
	}
	virtual int SLAPI Run(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = -1;
		if(pParam->GetAvailableSize()) {
			SSerializeContext sctx;
			PPGoodsImporter prcssr;
			THROW(prcssr.Init())
			THROW(prcssr.SerializeParam(-1, *pParam, &sctx));
			THROW(prcssr.LoadConfig(1));
			THROW(prcssr.Run());
			ok = 1;
		}
		else {
			ok = ImportGoods(0, 0, 0, 0);
		}
		CATCHZOK
		return ok;
	}
};

IMPLEMENT_CMD_HDL_FACTORY(IMPORTGOODS);
#endif // } 0 @construction
//
//
//
class CMD_HDL_CLS(PROCESSOBJTEXT) : public PPCommandHandler {
public:
	SLAPI  CMD_HDL_CLS(PROCESSOBJTEXT)(const PPCommandDescr * pDescr) : PPCommandHandler(pDescr)
	{
	}
	virtual int SLAPI EditParam(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = -1;
		if(pParam) {
			PrcssrObjText prc;
			PrcssrObjTextFilt filt;
			if(!filt.Read(*pParam, 0))
				prc.InitParam(&filt);
			if(prc.EditParam(&filt) > 0) {
				if(filt.Write(pParam->Z(), 0)) {
					ok = 1;
				}
			}
		}
		return ok;
	}
	virtual int SLAPI Run(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = -1;
		if(pParam) {
			PrcssrObjTextFilt filt;
			if(filt.Read(*pParam, 0)) {
				PrcssrObjText prc;
				if(!prc.Init(&filt) || !prc.Run())
					ok = PPErrorZ();
			}
			else
				ok = DoProcessObjText(0);
		}
		else
			ok = DoProcessObjText(0);
		return ok;
	}
};

IMPLEMENT_CMD_HDL_FACTORY(PROCESSOBJTEXT);
//
//
//
class CMD_HDL_CLS(PERSONEVENTBYREADER) : public PPCommandHandler {
public:
	SLAPI  CMD_HDL_CLS(PERSONEVENTBYREADER)(const PPCommandDescr * pDescr) : PPCommandHandler(pDescr)
	{
	}
	virtual int SLAPI EditParam(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = -1;
		size_t sav_offs = 0;
		AddPersonEventFilt filt;
		THROW_INVARG(pParam);
        sav_offs = pParam->GetRdOffs();
		filt.Read(*pParam, 0);
		if(filt.Edit() > 0) {
			THROW(filt.Write(pParam->Z(), 0));
		}
		else
			pParam->SetRdOffs(sav_offs);
		CATCH
			CALLPTRMEMB(pParam, SetRdOffs(sav_offs));
			ok = 0;
		ENDCATCH
		return ok;
	}
	virtual int SLAPI Run(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = -1;
		if(pParam) {
			AddPersonEventFilt filt;
			if(filt.Read(*pParam, 0)) {
				PPObjPersonEvent pe_obj;
				pe_obj.ProcessDeviceInput(filt);
				ok = 1;
			}
		}
		return ok;
	}
};

IMPLEMENT_CMD_HDL_FACTORY(PERSONEVENTBYREADER);
//
//
//
// @vmiller
class CMD_HDL_CLS(TSESSAUTOSMS) : public PPCommandHandler {
public:
	SLAPI  CMD_HDL_CLS(TSESSAUTOSMS)(const PPCommandDescr * pDescr) : PPCommandHandler(pDescr)
	{
	}
	virtual int SLAPI EditParam(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = -1, r = 1;
		uint   val = 0;
		PPViewTSession tsess_view;
		SSerializeContext sctx;
		TSessionFilt tsess_filt;
		size_t sav_offs = pParam->GetRdOffs();
		if(pParam->GetAvailableSize() == 0) {
			;
		}
		else {
			THROW(tsess_filt.Serialize(-1, *pParam, &sctx));
		}
		if(tsess_view.EditBaseFilt(&tsess_filt) > 0) {
			THROW(tsess_filt.Serialize(+1, pParam->Z(), &sctx));
			ok = 1;
		}
		else
			pParam->SetRdOffs(sav_offs);
		CATCHZOKPPERR
		return ok;
	}
	virtual int SLAPI Run(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = -1;
		if(pParam->GetAvailableSize()) {
			SSerializeContext sctx;
			PPViewTSession tsess_view;
			TSessionFilt tsess_filt;
			THROW(tsess_filt.Serialize(-1, *pParam, &sctx));
			THROW(tsess_view.Init_(&tsess_filt));
			THROW(tsess_view.SendAutoSms());
			ok = 1;
		}
		CATCHZOK
		return ok;
	}
};

IMPLEMENT_CMD_HDL_FACTORY(TSESSAUTOSMS);
//
//
//
class CMD_HDL_CLS(CREATEDRAFTBYSUPPLORDER) : public PPCommandHandler {
public:
	SLAPI  CMD_HDL_CLS(CREATEDRAFTBYSUPPLORDER)(const PPCommandDescr * pDescr) : PPCommandHandler(pDescr)
	{
	}
	virtual int SLAPI EditParam(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = -1;
		size_t preserve_offs = 0;
		SStatFilt * p_filt = 0;
		if(pParam) {
			PPViewSStat view;
			preserve_offs = pParam->GetRdOffs();
			THROW_MEM(p_filt = static_cast<SStatFilt *>(view.CreateFilt(reinterpret_cast<void *>(1))));
			if(pParam->GetAvailableSize() != 0)
				p_filt->Read(*pParam, 0);
			if(view.EditBaseFilt(p_filt) > 0) {
				THROW(p_filt->Write(pParam->Z(), 0));
				ok = 1;
			}
			else
				pParam->SetRdOffs(preserve_offs);
		}
		CATCH
			CALLPTRMEMB(pParam, SetRdOffs(preserve_offs));
			ok = 0;
		ENDCATCH
		ZDELETE(p_filt);
		return ok;
	}
	virtual int SLAPI Run(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = -1;
		if(pParam) {
			SStatFilt filt;
			if(filt.Read(*pParam, 0) > 0)
				ok = PrcssrBillAutoCreate::CreateDraftBySupplOrders(&filt);
			else
				ok = 0;
		}
		return ok;
	}
};

IMPLEMENT_CMD_HDL_FACTORY(CREATEDRAFTBYSUPPLORDER);
//
//
//
class CMD_HDL_CLS(SENDBILLS) : public PPCommandHandler {
public:
	SLAPI  CMD_HDL_CLS(SENDBILLS)(const PPCommandDescr * pDescr) : PPCommandHandler(pDescr)
	{
	}
	virtual int SLAPI EditParam(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = -1;
		size_t preserve_offs = 0;
		BillTransmitParam * p_filt = 0;
		if(pParam) {
			preserve_offs = pParam->GetRdOffs();
			THROW_MEM(p_filt = new BillTransmitParam);
			if(pParam->GetAvailableSize() != 0)
				p_filt->Read(*pParam, 0);
			if(p_filt->Edit() > 0) {
				THROW(p_filt->Write(pParam->Z(), 0));
				ok = 1;
			}
			else
				pParam->SetRdOffs(preserve_offs);
		}
		CATCH
			CALLPTRMEMB(pParam, SetRdOffs(preserve_offs));
			ok = 0;
		ENDCATCH
		ZDELETE(p_filt);
		return ok;
	}
	virtual int SLAPI Run(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = -1;
		if(pParam && pParam->GetAvailableSize()) {
			BillTransmitParam filt;
			THROW(filt.Read(*pParam, 0) > 0);
			THROW(PPObjectTransmit::TransmitBillsByDBDivList(&filt));
			ok = 1;
		}
		else {
			PPObjectTransmit::TransmitBillsByDBDivList(0);
		}
		CATCHZOKPPERR
		return ok;
	}
};

IMPLEMENT_CMD_HDL_FACTORY(SENDBILLS);
//
//
// @v10.4.1 {
class CMD_HDL_CLS(SENDBILLSWITHFILT) : public PPCommandHandler {
public:
	SLAPI  CMD_HDL_CLS(SENDBILLSWITHFILT)(const PPCommandDescr * pDescr) : PPCommandHandler(pDescr)
	{
	}
	virtual int SLAPI EditParam(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = -1;
		const size_t preserve_offs = pParam ? pParam->GetRdOffs() : 0;
		if(pParam) {
			int    r = 1;
			BillFilt filt;
			ObjTransmitParam tr_param;
			filt.SetupBrowseBillsType(filt.Bbt = bbtUndef);
			if(pParam->GetAvailableSize() == 0) {
				uint   val = 0;
				if((r = SelectorDialog(DLG_BBTSEL, CTL_BBTSEL_TYPE, &val)) > 0)
					filt.SetupBrowseBillsType(filt.Bbt = static_cast<BrowseBillsType>(val));
			}
			else {
				filt.Read(*pParam, 0);
				tr_param.Read(*pParam, 0);
			}
			if(r > 0 && ObjTransmDialogExt(DLG_OBJTRANSM, PPVIEW_BILL, &tr_param, &filt) > 0) {
				THROW(filt.Write(pParam->Z(), 0));
				THROW(tr_param.Write(*pParam, 0));
				ok = 1;
			}
			else
				pParam->SetRdOffs(preserve_offs);
		}
		CATCH
			CALLPTRMEMB(pParam, SetRdOffs(preserve_offs));
			ok = 0;
		ENDCATCH
		return ok;
	}
	virtual int SLAPI Run(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = 1;
		if(pParam && pParam->GetAvailableSize()) {
			BillFilt filt;
			ObjTransmitParam tr_param;
			THROW(filt.Read(*pParam, 0));
			THROW(tr_param.Read(*pParam, 0));
			THROW(PPViewBill::TransmitByFilt(&filt, &tr_param));
		}
		else {
			THROW(PPViewBill::TransmitByFilt(0, 0));
		}
		CATCHZOKPPERR
		return ok;
	}
};

IMPLEMENT_CMD_HDL_FACTORY(SENDBILLSWITHFILT);
// } @v10.4.1
//
//
class CMD_HDL_CLS(EXPORTDBTBLTRANSFER) : public PPCommandHandler {
public:
	SLAPI  CMD_HDL_CLS(EXPORTDBTBLTRANSFER)(const PPCommandDescr * pDescr) : PPCommandHandler(pDescr)
	{
	}
	virtual int SLAPI EditParam(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = -1;
		PPDbTableXmlExportParam_TrfrBill filt;
		SSerializeContext sctx;
		size_t preserve_offs = 0;
		if(pParam) {
			preserve_offs = pParam->GetRdOffs();
			if(pParam->GetAvailableSize() != 0)
				THROW(filt.Serialize(-1, *pParam, &sctx));
			if(PPDbTableXmlExportParam_TrfrBill::Edit(&filt) > 0) {
				THROW(filt.Serialize(+1, pParam->Z(), &sctx));
				ok = 1;
			}
			else
				pParam->SetRdOffs(preserve_offs);
		}
		CATCH
			CALLPTRMEMB(pParam, SetRdOffs(preserve_offs));
			ok = 0;
		ENDCATCH
		return ok;
	}
	virtual int SLAPI Run(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = -1;
		PPDbTableXmlExportParam_TrfrBill filt;
		SSerializeContext sctx;
		THROW(filt.Serialize(-1, *pParam, &sctx));
		{
			PPDbTableXmlExporter_Transfer prc(filt);
            THROW(prc.Run(filt.FileName));
            ok = 1;
		}
		CATCHZOKPPERR
		return ok;
	}
};

IMPLEMENT_CMD_HDL_FACTORY(EXPORTDBTBLTRANSFER);
//
//
//
class CMD_HDL_CLS(EXPORTDBTBLBILL) : public CMD_HDL_CLS(EXPORTDBTBLTRANSFER) {
public:
	SLAPI  CMD_HDL_CLS(EXPORTDBTBLBILL)(const PPCommandDescr * pDescr) : CMD_HDL_CLS(EXPORTDBTBLTRANSFER)(pDescr)
	{
	}
	virtual int SLAPI Run(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = -1;
		PPDbTableXmlExportParam_TrfrBill filt;
		SSerializeContext sctx;
		THROW(filt.Serialize(-1, *pParam, &sctx));
		{
			PPDbTableXmlExporter_Bill prc(filt);
            THROW(prc.Run(filt.FileName));
            ok = 1;
		}
		CATCHZOKPPERR
		return ok;
	}
};

IMPLEMENT_CMD_HDL_FACTORY(EXPORTDBTBLBILL);
//
//
//
class CMD_HDL_CLS(IMPORTFIAS) : public PPCommandHandler {
public:
	SLAPI  CMD_HDL_CLS(IMPORTFIAS)(const PPCommandDescr * pDescr) : PPCommandHandler(pDescr)
	{
	}
	virtual int SLAPI EditParam(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = -1;
		FiasImporter::Param filt;
		SSerializeContext sctx;
		size_t preserve_offs = 0;
		if(pParam) {
			preserve_offs = pParam->GetRdOffs();
			if(pParam->GetAvailableSize() != 0)
				THROW(filt.Serialize(-1, *pParam, &sctx));
			if(FiasImporter::EditParam(filt) > 0) {
				THROW(filt.Serialize(+1, pParam->Z(), &sctx));
				ok = 1;
			}
			else
				pParam->SetRdOffs(preserve_offs);
		}
		CATCH
			CALLPTRMEMB(pParam, SetRdOffs(preserve_offs));
			ok = 0;
		ENDCATCH
		return ok;
	}
	virtual int SLAPI Run(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = -1;
		FiasImporter::Param filt;
		SSerializeContext sctx;
		THROW(filt.Serialize(-1, *pParam, &sctx));
		{
			FiasImporter prc;
			THROW(prc.Run(filt));
            ok = 1;
		}
		CATCHZOKPPERR
		return ok;
	}
};

IMPLEMENT_CMD_HDL_FACTORY(IMPORTFIAS);
//
//
//
class CMD_HDL_CLS(SUPPLINTERCHANGE) : public PPCommandHandler {
public:
	SLAPI  CMD_HDL_CLS(SUPPLINTERCHANGE)(const PPCommandDescr * pDescr) : PPCommandHandler(pDescr)
	{
	}
	virtual int SLAPI EditParam(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = -1;
		if(pParam) {
			PrcssrSupplInterchange prc;
			SupplInterchangeFilt filt;
			if(!filt.Read(*pParam, 0))
				prc.InitParam(&filt);
			if(prc.EditParam(&filt) > 0) {
				if(filt.Write(pParam->Z(), 0)) {
					ok = 1;
				}
			}
		}
		return ok;
	}
	virtual int SLAPI Run(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = -1;
		if(pParam) {
			SupplInterchangeFilt filt;
			if(filt.Read(*pParam, 0)) {
				PrcssrSupplInterchange prc;
				if(!prc.Init(&filt) || !prc.Run())
					ok = PPErrorZ();
			}
			else
				ok = DoSupplInterchange(0);
		}
		else
			ok = DoSupplInterchange(0);
		return ok;
	}
};

IMPLEMENT_CMD_HDL_FACTORY(SUPPLINTERCHANGE);
//
//
//
class CMD_HDL_CLS(PROCESSOSM) : public PPCommandHandler {
public:
	SLAPI  CMD_HDL_CLS(PROCESSOSM)(const PPCommandDescr * pDescr) : PPCommandHandler(pDescr)
	{
	}
	virtual int SLAPI EditParam(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = -1;
		if(pParam) {
			PrcssrOsm prc(0);
			PrcssrOsmFilt filt;
			if(!filt.Read(*pParam, 0))
				prc.InitParam(&filt);
			if(prc.EditParam(&filt) > 0) {
				if(filt.Write(pParam->Z(), 0)) {
					ok = 1;
				}
			}
		}
		return ok;
	}
	virtual int SLAPI Run(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = -1;
		PrcssrOsmFilt filt;
		if(pParam && filt.Read(*pParam, 0)) {
			PrcssrOsm prc(0);
			if(!prc.Init(&filt) || !prc.Run())
				ok = PPErrorZ();
		}
		else
			ok = DoProcessOsm(0);
		return ok;
	}
};

IMPLEMENT_CMD_HDL_FACTORY(PROCESSOSM);
//
//
//
class CMD_HDL_CLS(PROCESSSARTRE) : public PPCommandHandler {
public:
	SLAPI  CMD_HDL_CLS(PROCESSSARTRE)(const PPCommandDescr * pDescr) : PPCommandHandler(pDescr)
	{
	}
	virtual int SLAPI EditParam(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = -1;
		if(pParam) {
			PrcssrSartre prc(0);
			PrcssrSartreFilt filt;
			if(!filt.Read(*pParam, 0))
				prc.InitParam(&filt);
			if(prc.EditParam(&filt) > 0) {
				if(filt.Write(pParam->Z(), 0)) {
					ok = 1;
				}
			}
		}
		return ok;
	}
	virtual int SLAPI Run(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = -1;
		PrcssrSartreFilt filt;
		if(pParam && filt.Read(*pParam, 0)) {
			PrcssrSartre prc(0);
			if(!prc.Init(&filt) || !prc.Run())
				ok = PPErrorZ();
		}
		else
			ok = DoProcessSartre(0);
		return ok;
	}
};

IMPLEMENT_CMD_HDL_FACTORY(PROCESSSARTRE);
//
//
//
class CMD_HDL_CLS(BILLAUTOCREATE) : public PPCommandHandler {
public:
	SLAPI  CMD_HDL_CLS(BILLAUTOCREATE)(const PPCommandDescr * pDescr) : PPCommandHandler(pDescr)
	{
	}
	virtual int SLAPI EditParam(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = -1;
		if(pParam) {
			PrcssrBillAutoCreate prc;
			PPBillAutoCreateParam filt;
			if(!filt.Read(*pParam, 0))
				prc.InitParam(&filt);
			if(prc.EditParam(&filt) > 0) {
				if(filt.Write(pParam->Z(), 0)) {
					ok = 1;
				}
			}
		}
		return ok;
	}
	virtual int SLAPI Run(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = -1;
		PPBillAutoCreateParam filt;
		if(pParam && filt.Read(*pParam, 0)) {
			PrcssrBillAutoCreate prc;
			if(!prc.Init(&filt) || !prc.Run())
				ok = PPErrorZ();
		}
		else {
			PrcssrBillAutoCreate prc;
			if(prc.EditParam(&filt) > 0) {
				if(!prc.Init(&filt) || !prc.Run())
					ok = PPErrorZ();
				else
					ok = 1;
			}
		}
		return ok;
	}
};

IMPLEMENT_CMD_HDL_FACTORY(BILLAUTOCREATE);
//
//
//
class CMD_HDL_CLS(TIMESERIESSA) : public PPCommandHandler {
public:
	SLAPI  CMD_HDL_CLS(TIMESERIESSA)(const PPCommandDescr * pDescr) : PPCommandHandler(pDescr)
	{
	}
	virtual int SLAPI EditParam(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = -1;
		if(pParam) {
			PrcssrTsStrategyAnalyze prc;
			PrcssrTsStrategyAnalyzeFilt filt;
			if(!filt.Read(*pParam, 0))
				prc.InitParam(&filt);
			if(prc.EditParam(&filt) > 0) {
				if(filt.Write(pParam->Z(), 0)) {
					ok = 1;
				}
			}
		}
		return ok;
	}
	virtual int SLAPI Run(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = -1;
		PrcssrTsStrategyAnalyzeFilt filt;
		if(pParam && filt.Read(*pParam, 0)) {
			PrcssrTsStrategyAnalyze prc;
			if(!prc.Init(&filt) || !prc.Run())
				ok = PPErrorZ();
		}
		else
			ok = DoProcessOsm(0);
		return ok;
	}
};

IMPLEMENT_CMD_HDL_FACTORY(TIMESERIESSA);
//
//
//
class CMD_HDL_CLS(EXPORTVIEW) : public PPCommandHandler {
public:
	SLAPI  CMD_HDL_CLS(EXPORTVIEW)(const PPCommandDescr * pDescr) : PPCommandHandler(pDescr)
	{
	}
	virtual int SLAPI EditParam(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = -1;
		if(pParam) {
			PPView::ExecNfViewParam filt;
			filt.Read(*pParam, 0);
			if(PPView::EditExecNfViewParam(filt) > 0) {
				if(filt.Write(pParam->Z(), 0)) {
					ok = 1;
				}
			}
		}
		return ok;
	}
	virtual int SLAPI Run(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = 1;
		PPView::ExecNfViewParam param;
		SString result_fname, dest_fname;
		// @debug {
			const PPThreadLocalArea & r_tla = DS.GetConstTLA();
			assert((&r_tla) != 0);
		// } @debug
		THROW_INVARG(pParam);
		THROW(param.Read(*pParam, 0));
		THROW(PPView::ExecuteNF(param.NfSymb, param.Dl600_Name, result_fname));
		if(param.FileName.NotEmpty()) {
			SPathStruc dest_ps(param.FileName);
			if(dest_ps.Nam.Empty()) {
				SPathStruc src_ps(result_fname);
				dest_ps.Nam = src_ps.Nam;
				dest_ps.Ext = src_ps.Ext;
			}
			dest_ps.Merge(dest_fname);
			THROW(SCopyFile(result_fname, dest_fname, 0, FILE_SHARE_READ, 0));
			SFile::Remove(result_fname);
		}
		CATCHZOK
		return ok;
	}
};

IMPLEMENT_CMD_HDL_FACTORY(EXPORTVIEW);
