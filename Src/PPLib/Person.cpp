// PERSON.CPP
// Copyright (c) A.Sobolev 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022, 2024, 2025
// @codepage UTF-8
// @Kernel
//
#include <pp.h>
#pragma hdrstop
#include <charry.h>
//
//
//
PPPerson::PPPerson()
{
}

PPPerson::PPPerson(const PPPerson & rS) : Rec(rS.Rec), SMemo(rS.SMemo), Kinds(rS.Kinds), RelList(rS.RelList)
{
}

PPPerson & FASTCALL PPPerson::operator = (const PPPerson & rS)
{
	Rec = rS.Rec;
	SMemo = rS.SMemo; // @v11.1.12
	Kinds.copy(rS.Kinds);
	RelList.copy(rS.RelList);
	return *this;
}

PPPerson & PPPerson::Z()
{
	Rec.Clear();
	SMemo.Z(); // @v11.1.12
	Kinds.Z();
	RelList.clear();
	return *this;
}

int PPPerson::AddRelation(PPID personID, PPID relTypeID, uint * pPos)
{
	if(!RelList.SearchPair(personID, relTypeID, 0))
		return RelList.Add(personID, relTypeID, pPos, 0) ? 1 : PPSetErrorSLib();
	else
		return PPSetError(PPERR_DUPPERSONREL);
}

int PPPerson::AddRelations(const PPIDArray * pPersonList, PPID relTypeID, uint * pPos)
{
	int    ok = 1;
	if(pPersonList) {
		LAssocArray rel_list = RelList;
		for(uint i = 0; ok > 0 && i < pPersonList->getCount(); i++) {
			PPID psn_id = 0;
			if(ok > 0)
				ok = AddRelation(pPersonList->at(i), relTypeID, pPos);
		}
		if(ok <= 0)
			RelList = rel_list;
	}
	else
		ok = -1;
	return ok;
}

int PPPerson::RemoveRelationByPos(uint pos)
{
	int    ok = 1;
	if(pos < RelList.getCount())
		RelList.atFree(pos);
	else
		ok = 0;
	return ok;
}

int PPPerson::RemoveRelation(PPID personID, PPID relTypeID)
{
	int    ok = 1;
	uint   pos = 0;
	if(RelList.SearchPair(personID, relTypeID, &pos))
		RelList.atFree(pos);
	else
		ok = -1;
	return ok;
}

int PPPerson::RemoveRelations(PPIDArray * pPersonList, PPID relTypeID)
{
	int    ok = 1;
	PPIDArray * p_list = pPersonList;
	if(!p_list) {
		p_list = new PPIDArray;
		GetRelList(relTypeID, p_list);
	}
	if(p_list) {
		for(uint i = 0; i < p_list->getCount(); i++)
			RemoveRelation(p_list->at(i), relTypeID);
	}
	else
		ok = -1;
	if(!pPersonList)
		delete p_list;
	return ok;
}

int PPPerson::GetRelList(PPID relTypeID, PPIDArray * pList) const
{
	int    ok = 1;
	for(uint i = 0; i < RelList.getCount(); i++)
		if(RelList.at(i).Val == relTypeID && pList)
			pList->add(RelList.at(i).Key);
	return ok;
}

const LAssocArray & PPPerson::GetRelList() const
{
	return RelList;
}
//
// @ModuleDef(PPObjPersonRelType)
//
PPPersonRelType2::PPPersonRelType2()
{
	THISZERO();
}

PPPersonRelTypePacket::PPPersonRelTypePacket()
{
}

void PPPersonRelTypePacket::Init()
{
	MEMSZERO(Rec);
	InhRegTypeList.Z();
}

PPPersonRelTypePacket & FASTCALL PPPersonRelTypePacket::operator = (const PPPersonRelTypePacket & s)
{
	Rec = s.Rec;
	InhRegTypeList.copy(s.InhRegTypeList);
	return *this;
}
//
//
//
PPObjPersonRelType::PPObjPersonRelType(void * extraPtr) : PPObjReference(PPOBJ_PERSONRELTYPE, extraPtr)
{
}

IMPL_DESTROY_OBJ_PACK(PPObjPersonRelType, PPPersonRelTypePacket);

int PPObjPersonRelType::SerializePacket(int dir, PPPersonRelTypePacket * pPack, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	THROW_SL(P_Ref->SerializeRecord(dir, &pPack->Rec, rBuf, pSCtx));
	THROW_SL(pSCtx->Serialize(dir, &pPack->InhRegTypeList, rBuf));
	CATCHZOK
	return ok;
}

int PPObjPersonRelType::Read(PPObjPack * p, PPID id, void * stream, ObjTransmContext * pCtx)
	{ return Implement_ObjReadPacket<PPObjPersonRelType, PPPersonRelTypePacket>(this, p, id, stream, pCtx); }

int PPObjPersonRelType::Write(PPObjPack * p, PPID * pID, void * stream, ObjTransmContext * pCtx) // @srlz
{
	int    ok = -1;
	if(p->Data) {
		PPPersonRelTypePacket * p_pack = static_cast<PPPersonRelTypePacket *>(p->Data);
		if(stream == 0) {
			PPID   same_id = 0;
			if(*pID == 0)
				if(p_pack->Rec.ID < PP_FIRSTUSRREF) {
					if(Search(p_pack->Rec.ID) > 0) {
						*pID = p_pack->Rec.ID;
						ok = 1;
					}
				}
				else if(P_Ref->SearchSymb(Obj, &same_id, p_pack->Rec.Symb, offsetof(PPPersonRelType, Symb)) > 0) {
					*pID = same_id;
					ok = 1;
				}
				else
					p_pack->Rec.ID = 0;
			else
				p_pack->Rec.ID = *pID;
			if(ok < 0)
				if(PutPacket(pID, p_pack, 1))
   	        		ok = 1;
				else {
					pCtx->OutputAcceptErrMsg(PPTXT_ERRACCEPTPSNRELTYPE, p_pack->Rec.ID, p_pack->Rec.Name);
   	            	ok = -1;
				}
		}
		else {
			SBuffer buffer;
			THROW(SerializePacket(+1, p_pack, buffer, &pCtx->SCtx));
			THROW_SL(buffer.WriteToFile(static_cast<FILE *>(stream), 0, 0))
			ok = 1;
		}
	}
	CATCHZOK
	return ok;
}

int  PPObjPersonRelType::ProcessObjRefs(PPObjPack * p, PPObjIDArray * ary, int replace, ObjTransmContext * pCtx)
{
	if(p && p->Data) {
		PPPersonRelTypePacket * p_pack = static_cast<PPPersonRelTypePacket *>(p->Data);
		return ProcessObjListRefInArray(PPOBJ_REGISTERTYPE, p_pack->InhRegTypeList, ary, replace);
	}
	return -1;
}

int PPObjPersonRelType::GetGroupingList(PPIDArray * pList)
{
	int    ok = -1;
	SVector list(sizeof(PPPersonRelType));
	THROW(P_Ref->LoadItems(Obj, list));
	for(uint i = 0; i < list.getCount(); i++) {
		const PPPersonRelType * p_item = static_cast<const PPPersonRelType *>(list.at(i));
		if(p_item && p_item->Flags & PPPersonRelType::fGrouping) {
			if(pList)
				THROW_SL(pList->add(p_item->ID));
			ok = 1;
		}
	}
	CATCHZOK
	return ok;
}

int PPObjPersonRelType::ProcessReservedItem(TVRez & rez)
{
	int    ok = 1;
	int    r;
	SString name;
	SString symb;
	const  PPID id = rez.getLONG();
	rez.getString(name, 2);
	PPExpandString(name, CTRANSF_UTF8_TO_INNER);
	rez.getString(symb, 2);
	THROW(r = Search(id));
	if(r < 0) {
		PPPersonRelType2 rec;
		rec.Tag = Obj;
		rec.ID  = id;
		STRNSCPY(rec.Name, name);
		STRNSCPY(rec.Symb, symb);
		THROW(StoreItem(Obj, 0, &rec, 1));
	}
	CATCHZOK
	return ok;
}

int PPObjPersonRelType::SearchSymb(PPID * pID, const char * pSymb)
{
	return P_Ref->SearchSymb(Obj, pID, pSymb, offsetof(PPPersonRelType, Symb));
}

class PersonRelTypeCache : public ObjCache {
public:
	PersonRelTypeCache() : ObjCache(PPOBJ_PERSONRELTYPE, sizeof(PersonRelTypeCache::Entry)) {}
private:
	virtual int  FetchEntry(PPID id, ObjCacheEntry * pEntry, void * /*extraData*/);
	virtual void EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const;
public:
	struct Entry : public ObjCacheEntry {
		int16  StatusRestriction; // Ограничение по статусу отношений (PPPersonRelType::ssXXX)
		int16  Cardinality;       // Ограничение по множественности отношений (PPPersonRelType::cXXX)
		long   Flags;             // Флаги (PPPersonRelType::fXXX)
		PPID   InhRegTypeList[8];
	};
};

int PersonRelTypeCache::FetchEntry(PPID id, ObjCacheEntry * pEntry, void * /*extraData*/)
{
	int    ok = 1;
	Entry * p_cache_rec = static_cast<Entry *>(pEntry);
	PPPersonRelTypePacket pack;
	PPObjPersonRelType prt_obj;
	if(prt_obj.GetPacket(id, &pack) > 0) {
		memzero(p_cache_rec, sizeof(*p_cache_rec));
	   	p_cache_rec->StatusRestriction = pack.Rec.StatusRestriction;
		p_cache_rec->Cardinality = pack.Rec.Cardinality;
		p_cache_rec->Flags = pack.Rec.Flags;
		const uint _c = MIN(pack.InhRegTypeList.getCount(), SIZEOFARRAY(p_cache_rec->InhRegTypeList));
		for(uint i = 0; i < _c; i++)
			p_cache_rec->InhRegTypeList[i] = pack.InhRegTypeList.at(i);
#ifdef PERSONRELTYPE_CACHE_SYMB
		ok = PutName(pack.Rec.Symb, p_cache_rec);
#endif
	}
	else
		ok = -1;
	return ok;
}

void PersonRelTypeCache::EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const
{
	PPPersonRelTypePacket * p_pack = static_cast<PPPersonRelTypePacket *>(pDataRec);
	const Entry * p_cache_rec = static_cast<const Entry *>(pEntry);
	p_pack->Rec.Tag      = PPOBJ_PERSONRELTYPE;
	p_pack->Rec.ID       = p_cache_rec->ID;
	p_pack->Rec.StatusRestriction = p_cache_rec->StatusRestriction;
	p_pack->Rec.Cardinality = p_cache_rec->Cardinality;
	p_pack->Rec.Flags    = p_cache_rec->Flags;
	for(uint i = 0; i < SIZEOFARRAY(p_cache_rec->InhRegTypeList); i++) {
		if(p_cache_rec->InhRegTypeList[i])
			p_pack->InhRegTypeList.add(p_cache_rec->InhRegTypeList[i]);
	}
#ifdef PERSONRELTYPE_CACHE_SYMB
	GetName(p_cache_rec, p_pack->Rec.Symb, sizeof(p_pack->Rec.Symb));
#endif
}

int PPObjPersonRelType::Fetch(PPID id, PPPersonRelTypePacket * pPack)
{
	PersonRelTypeCache * p_cache = GetDbLocalCachePtr <PersonRelTypeCache> (Obj);
	return p_cache ? p_cache->Get(id, pPack) : GetPacket(id, pPack);
}
//
//
//
class PersonRelTypeDialog : public PPListDialog {
	DECL_DIALOG_DATA(PPPersonRelTypePacket);
public:
	PersonRelTypeDialog() : PPListDialog(DLG_PSNRELTYPE, CTL_PSNRELTYPE_INHRGLIST)
	{
	}
	DECL_DIALOG_SETDTS()
	{
		RVALUEPTR(Data, pData);
		setCtrlData(CTL_PSNRELTYPE_NAME, Data.Rec.Name);
		setCtrlData(CTL_PSNRELTYPE_SYMB, Data.Rec.Symb);
		setCtrlData(CTL_PSNRELTYPE_ID, &Data.Rec.ID);
		disableCtrl(CTL_PSNRELTYPE_ID, true);

		AddClusterAssoc(CTL_PSNRELTYPE_FLAGS, 0, PPPersonRelType::fInhAddr);
		AddClusterAssoc(CTL_PSNRELTYPE_FLAGS, 1, PPPersonRelType::fInhRAddr);
		AddClusterAssoc(CTL_PSNRELTYPE_FLAGS, 2, PPPersonRelType::fGrouping);
		AddClusterAssoc(CTL_PSNRELTYPE_FLAGS, 3, PPPersonRelType::fInhMainOrgAgreement);
		AddClusterAssoc(CTL_PSNRELTYPE_FLAGS, 4, PPPersonRelType::fInhAgreements);
		SetClusterData(CTL_PSNRELTYPE_FLAGS, Data.Rec.Flags);

		AddClusterAssocDef(CTL_PSNRELTYPE_CARDINAL, 0, PPPersonRelType::cOneToOne);
		AddClusterAssoc(CTL_PSNRELTYPE_CARDINAL, 1, PPPersonRelType::cOneToMany);
		AddClusterAssoc(CTL_PSNRELTYPE_CARDINAL, 2, PPPersonRelType::cManyToOne);
		AddClusterAssoc(CTL_PSNRELTYPE_CARDINAL, 3, PPPersonRelType::cManyToMany);
		SetClusterData(CTL_PSNRELTYPE_CARDINAL, Data.Rec.Cardinality);

		AddClusterAssocDef(CTL_PSNRELTYPE_STATUSR, 0, PPPersonRelType::ssUndef);
		AddClusterAssoc(CTL_PSNRELTYPE_STATUSR, 1, PPPersonRelType::ssPrivateToPrivate);
		AddClusterAssoc(CTL_PSNRELTYPE_STATUSR, 2, PPPersonRelType::ssPrivateToLegal);
		AddClusterAssoc(CTL_PSNRELTYPE_STATUSR, 3, PPPersonRelType::ssLegalToPrivate);
		AddClusterAssoc(CTL_PSNRELTYPE_STATUSR, 4, PPPersonRelType::ssLegalToLegal);
		SetClusterData(CTL_PSNRELTYPE_STATUSR, Data.Rec.StatusRestriction);
		updateList(-1);
		return 1;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		uint   sel = 0;
		long   lv = 0;
		getCtrlData(sel = CTL_PSNRELTYPE_NAME, Data.Rec.Name);
		THROW_PP(*strip(Data.Rec.Name), PPERR_NAMENEEDED);
		getCtrlData(CTL_PSNRELTYPE_SYMB, Data.Rec.Symb);
		//getCtrlData(CTL_PSNRELTYPE_ID, &Data.Rec.ID);
		GetClusterData(CTL_PSNRELTYPE_FLAGS,    &Data.Rec.Flags);
		if(GetClusterData(CTL_PSNRELTYPE_CARDINAL, &(lv = Data.Rec.Cardinality)))
			Data.Rec.Cardinality = (int16)lv;
		if(GetClusterData(CTL_PSNRELTYPE_STATUSR, &(lv = Data.Rec.StatusRestriction)))
			Data.Rec.StatusRestriction = (int16)lv;
		ASSIGN_PTR(pData, Data);
		CATCHZOKPPERRBYDLG
		return ok;
	}
private:
	virtual int setupList();
	virtual int addItem(long * pPos, long * pID);
	virtual int delItem(long pos, long id);
};

int PersonRelTypeDialog::setupList()
{
	int    ok = 1;
	PPID * p_reg_type_id = 0;
	SString name_buf;
	for(uint i = 0; ok && Data.InhRegTypeList.enumItems(&i, (void **)&p_reg_type_id);) {
		GetRegisterTypeName(*p_reg_type_id, name_buf);
		if(!addStringToList(*p_reg_type_id, name_buf))
			ok = 0;
	}
	return ok;
}

int PersonRelTypeDialog::addItem(long * pPos, long * pID)
{
	int    ok = -1;
	PPID   id = 0;
	if(PPSelectObject(PPOBJ_REGISTERTYPE, &id, 0, 0) > 0)
		if(Data.InhRegTypeList.addUnique(id) > 0) {
			ASSIGN_PTR(pPos, Data.InhRegTypeList.getCount()-1);
			ASSIGN_PTR(pID, id);
			ok = 1;
		}
	return ok;
}

int PersonRelTypeDialog::delItem(long pos, long /*id*/)
{
	if(pos >= 0 && pos < Data.InhRegTypeList.getCountI())
		return (Data.InhRegTypeList.atFree(static_cast<uint>(pos)) > 0) ? 1 : -1;
	return -1;
}

int PPObjPersonRelType::Edit(PPID * pID, void * extraPtr)
{
	int    ok = -1, valid_data = 0, r = cmCancel;
	const  bool is_new = (*pID == 0);
	PersonRelTypeDialog * dlg = 0;
	PPPersonRelTypePacket pack;
	THROW(CheckRightsModByID(pID));
	if(!is_new)
		THROW(GetPacket(*pID, &pack) > 0);
	THROW(CheckDialogPtr(&(dlg = new PersonRelTypeDialog)));
	dlg->setDTS(&pack);
	while(!valid_data && (r = ExecView(dlg)) == cmOK) {
		if(dlg->getDTS(&pack))
			if(PutPacket(pID, &pack, 1))
				ok = valid_data = 1;
			else
				PPError();
	}
	CATCH
		r = PPErrorZ();
	ENDCATCH
	delete dlg;
	return ok ? r : 0;
}

int PPObjPersonRelType::Browse(void * extraPtr) { return RefObjView(this, PPDS_CRRPERSONRELTYPE, 0); }
//
//
//
int PPObjPersonRelType::PutPacket(PPID * pID, PPPersonRelTypePacket * pPack, int use_ta)
{
	int    ok = 1;
	if(pPack) {
		strip(pPack->Rec.Name);
		strip(pPack->Rec.Symb);
		THROW(P_Ref->CheckUniqueSymb(Obj, *pID, pPack->Rec.Name, offsetof(PPPersonRelType, Name)));
		THROW(P_Ref->CheckUniqueSymb(Obj, *pID, pPack->Rec.Symb, offsetof(PPPersonRelType, Symb)));
	}
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(*pID) {
			if(pPack) {
				THROW(P_Ref->UpdateItem(Obj, *pID, &pPack->Rec, 1, 0));
			}
			else {
				THROW(P_Ref->RemoveItem(Obj, *pID, 0));
			}
		}
		else if(pPack) {
			*pID = pPack->Rec.ID;
			THROW(P_Ref->AddItem(Obj, pID, &pPack->Rec, 0));
		}
		THROW(P_Ref->PutPropArray(Obj, *pID, PRTPRP_INHREGLIST, (pPack ? &pPack->InhRegTypeList : 0), 0));
		THROW(tra.Commit());
	}
	if(*pID)
		Dirty(*pID);
	CATCHZOK
	return ok;
}

int PPObjPersonRelType::GetPacket(PPID id, PPPersonRelTypePacket * pPack)
{
	int    ok = -1;
	pPack->InhRegTypeList.freeAll();
	if(PPCheckGetObjPacketID(Obj, id)) {
		ok = Search(id, &pPack->Rec);
		if(ok > 0) {
			THROW(P_Ref->GetPropArray(Obj, id, PRTPRP_INHREGLIST, &pPack->InhRegTypeList));
		}
	}
	CATCHZOK
	return ok;
}
//
// PPELinkArray
//
PPELink::PPELink() : KindID(0)
{
	Addr[0] = 0;
}

/*static*/int PPELinkArray::Helper_SetupNewEntry(PPID elinkType, const char * pValue, PPELink & rEntry)
{
	int    ok = 1;
	rEntry.KindID = 0;
	rEntry.Addr[0] = 0;
	if(!isempty(pValue)) {
		PPObjELinkKind elk_obj;
		PPELinkKind elk_rec;
		for(SEnum en = elk_obj.Enum(0); en.Next(&elk_rec) > 0;) {
			if(elk_rec.Type == elinkType) {
				if(elk_rec.Flags & ELNKF_PREF) {
					rEntry.KindID = elk_rec.ID;
					break;
				}
				else if(!rEntry.KindID)
					rEntry.KindID = elk_rec.ID;
			}
		}
		STRNSCPY(rEntry.Addr, pValue);
		if(!rEntry.KindID)
			ok = -1;
	}
	else 
		ok = 0;
	return ok;
}

/*static*/int PPELinkArray::SetupNewPhoneEntry(const char * pPhone, PPELink & rEntry) { return Helper_SetupNewEntry(ELNKRT_PHONE, pPhone, rEntry); }
/*static*/int PPELinkArray::SetupNewEmailEntry(const char * pEmail, PPELink & rEntry) { return Helper_SetupNewEntry(ELNKRT_EMAIL, pEmail, rEntry); }

PPELinkArray::PPELinkArray() : TSArray <PPELink>()
{
}

int FASTCALL PPELinkArray::IsEq(const PPELinkArray & rS) const
{
	int    eq = 1;
	const uint c = getCount();
	const uint c2 = rS.getCount();
	if(c != c2)
		eq = 0;
	else {
		for(uint i = 0; eq && i < c; i++) {
			const PPELink & r_rec = at(i);
			const PPELink & r_rec2 = rS.at(i);
			if(r_rec.KindID != r_rec2.KindID)
				eq = 0;
			else if(strncmp(r_rec.Addr, r_rec2.Addr, sizeof(r_rec.Addr)) != 0)
				eq = 0;
		}
	}
	return eq;
}

int PPELinkArray::SearchByText(const char * pText, uint * pPos) const
{
	int    ok = 0;
	SString key(pText);
	if(key.NotEmptyS()) {
		for(uint i = 0; !ok && i < getCount(); i++) {
			const PPELink & r_item = at(i);
			if(key.CmpNC(r_item.Addr) == 0) {
				ASSIGN_PTR(pPos, i);
				ok = 1;
			}
		}
	}
	return ok;
}

int PPELinkArray::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	int32  c = (int32)getCount(); // @persistent
	THROW_SL(pSCtx->Serialize(dir, c, rBuf));
	for(int i = 0; i < c; i++) {
		PPELink item;
		if(dir > 0)
			item = at(i);
		THROW_SL(pSCtx->Serialize(dir, item.KindID, rBuf));
		THROW_SL(pSCtx->Serialize(dir, item.Addr, sizeof(item.Addr), rBuf));
		if(dir < 0) {
			THROW_SL(insert(&item));
		}
	}
	CATCHZOK
	return ok;
}

int PPELinkArray::AddItem(PPID kindID, const char * pAddr)
{
	if(kindID && pAddr && pAddr[0]) {
		PPELink item;
		item.KindID = kindID;
		STRNSCPY(item.Addr, pAddr);
		return insert(&item) ? 1 : PPSetErrorSLib();
	}
	else
		return -1;
}

int PPELinkArray::GetItem(PPID kindID, SString & rBuf) const
{
	uint   p = 0;
	if(lsearch(&kindID, &p, CMPF_LONG)) {
		rBuf = at(p).Addr;
		return 1;
	}
	else {
		rBuf.Z();
		return -1;
	}
}

int PPELinkArray::GetListByType(PPID eLinkType, StringSet & rSs) const
{
	int    ok = -1;
	PPObjELinkKind elk_obj;
	PPELinkKind elk_rec;
	SString addr;
	for(uint i = 0; i < getCount(); i++) {
		const PPELink & r_item = at(i);
		addr = r_item.Addr;
		if(addr.NotEmptyS() && elk_obj.Fetch(r_item.KindID, &elk_rec) > 0 && elk_rec.Type == eLinkType) {
			rSs.add(addr);
			ok = 1;
		}
	}
	return ok;
}

int PPELinkArray::GetPhones(uint maxCount, SString & rBuf, long elinkType /* = ELNKRT_PHONE */) const
{
	rBuf.Z();
	StringSet ss;
	int    ok = GetListByType(elinkType, ss);
	if(ok > 0) {
		SString addr;
		for(uint i = 0, c = 0; c < maxCount && ss.get(&i, addr); c++) {
			if(c > 0)
				rBuf.CatDiv(';', 2);
			rBuf.Cat(addr);
		}
	}
	return ok;
}

int PPELinkArray::GetSinglePhone(SString & rBuf, uint * pPos) const
{
	rBuf.Z();

	int    ok = -1;
	uint   pos = 0;
	PPID   kind_id = 0;
	PPObjELinkKind elk_obj;
	PPELinkKind elk_rec;
	uint   c = getCount();
	if(c) do {
		const PPELink & r_item = at(--c);
		if(elk_obj.Fetch(r_item.KindID, &elk_rec) > 0 && elk_rec.Type == ELNKRT_PHONE && r_item.Addr[0]) {
			if(elk_rec.Flags & ELNKF_PREF) {
				kind_id = r_item.KindID;
				pos = c;
				ok = 1;
				break; // Предпочтительный номер однозначно нас устраивает. Выходим из цикла.
			}
			else if(oneof4(r_item.KindID, PPELK_WORKPHONE, PPELK_HOMEPHONE, PPELK_ALTPHONE, PPELK_MOBILE)) { // @v11.3.5 PPELK_MOBILE
				kind_id = r_item.KindID;
				pos = c;
				ok = 1;
			}
			else if(!kind_id) {
				pos = c;
				ok = 1;
			}
		}
	} while(c);
	if(ok > 0)
		rBuf = at(pos).Addr;
	else
		rBuf.Z();
	ASSIGN_PTR(pPos, pos);
	return ok;
}
//
//
//
CashierInfo::CashierInfo()
{
	THISZERO();
}

int FASTCALL CashierInfo::IsEq(const CashierInfo & rS) const
{
	int    eq = 1;
	if(Rights != rS.Rights)
		eq = 0;
	else if(Flags != rS.Flags)
		eq = 0;
	else if(strncmp(Password, rS.Password, sizeof(CashierInfo::Password)) != 0)
		eq = 0;
	return eq;
}
//
// PPPersonPacket
//
PPPersonPacket::PPPersonPacket() : PPPerson(), ObjTagContainerHelper(TagL, PPOBJ_PERSON, PPTAG_PERSON_UUID), P_SCardPack(0), LinkFiles(PPOBJ_PERSON)
{
	// @v11.5.5 destroy();
}

PPPersonPacket::PPPersonPacket(const PPPersonPacket & rS) : PPPerson(rS), ObjTagContainerHelper(rS), P_SCardPack(0), Regs(rS.Regs), ELA(rS.ELA),
	Loc(rS.Loc), RLoc(rS.RLoc), CshrInfo(rS.CshrInfo), ExtString(rS.ExtString), LinkFiles(rS.LinkFiles), TagL(rS.TagL), UpdFlags(rS.UpdFlags), SelectedLocPos(rS.SelectedLocPos)
{
	TSCollection_Copy(DlvrLocList, rS.DlvrLocList);
	if(rS.P_SCardPack)
		P_SCardPack = new PPSCardPacket(*rS.P_SCardPack);
}

PPPersonPacket::~PPPersonPacket()
{
	ZDELETE(P_SCardPack);
}

PPPersonPacket & PPPersonPacket::Z()
{
	PPPerson::Z();
	Regs.clear();
	ELA.clear();
	Loc.destroy();
	RLoc.destroy();
	ExtString.Z();
	DlvrLocList.freeAll();
	ZDELETE(P_SCardPack);
	LinkFiles.Clear();
	TagL.Z();
	UpdFlags = 0;
	SelectedLocPos = 0;
	return *this;
}

PPPersonPacket & FASTCALL PPPersonPacket::operator = (const PPPersonPacket & s)
{
	Z();
	PPPerson::operator = (s);
	Regs.copy(s.Regs);
	ELA.copy(s.ELA);
	Loc = s.Loc;
	RLoc = s.RLoc;
	CshrInfo = s.CshrInfo;
	ExtString = s.ExtString;
	TSCollection_Copy(DlvrLocList, s.DlvrLocList);
	if(s.P_SCardPack)
		P_SCardPack = new PPSCardPacket(*s.P_SCardPack);
	LinkFiles = s.LinkFiles;
	TagL   = s.TagL;
	UpdFlags  = s.UpdFlags;
	SelectedLocPos = s.SelectedLocPos;
	return *this;
}

int PPPersonPacket::GetRAddress(uint f, SString & rBuf)
{
	LocationTbl::Rec rloc = RLoc;
	if(LocationCore::IsEmptyAddressRec(rloc))
		rloc = Loc;
	return LocationCore::GetAddress(rloc, f, rBuf);
}

int    PPPersonPacket::GetAddress(uint f, SString & rBuf) { return LocationCore::GetAddress(Loc, f, rBuf); }
int    PPPersonPacket::GetPhones(uint maxCount, SString & rBuf) { return ELA.GetPhones(maxCount, rBuf); }
int    PPPersonPacket::GetRegister(PPID regTyp, uint * pos) const { return Regs.GetRegister(regTyp, pos, 0); }
int    PPPersonPacket::GetRegNumber(PPID regTyp, SString & rBuf) const { return Regs.GetRegNumber(regTyp, rBuf); }
uint   PPPersonPacket::GetDlvrLocCount() const { return DlvrLocList.getCount(); }
void   FASTCALL PPPersonPacket::SetExtName(const char * pName) { ExtString = pName; }
void   PPPersonPacket::ClearDlvrLocList() { DlvrLocList.freeAll(); }
const  PPSCardPacket * PPPersonPacket::GetSCard() const { return P_SCardPack; }

bool PPPersonPacket::GetSrchRegNumber(PPID * pRegTypeID, SString & rBuf) const
{
	rBuf.Z();
	bool   ok = false;
	PPID   reg_type_id = 0;
	PPObjPersonKind pk_obj;
	PPPersonKind2 pk_rec;
	for(uint i = 0; !ok && i < Kinds.getCount(); i++) {
		if(pk_obj.Fetch(Kinds.at(i), &pk_rec) > 0 && pk_rec.CodeRegTypeID) {
			if(GetRegNumber(pk_rec.CodeRegTypeID, rBuf) > 0) {
				reg_type_id = pk_rec.CodeRegTypeID;
				ok = true;
			}
		}
	}
	ASSIGN_PTR(pRegTypeID, reg_type_id);
	return ok;
}

int PPPersonPacket::GetCurrBnkAcct(PPBankAccount * pRec) const
{
	int    ok = -1;
	uint   first_acc_idx = 0; // [1..]
	uint   first_pref_acc_idx = 0; // [1..]
	for(uint i = 0; i < Regs.getCount(); i++) {
		const RegisterTbl::Rec & r_reg = Regs.at(i);
		if(r_reg.RegTypeID == PPREGT_BANKACCOUNT) {
            PPBankAccount item(r_reg);
            if(item.AccType == PPBAC_CURRENT) {
				if(!first_acc_idx)
					first_acc_idx = i+1;
				if(item.Flags & PREGF_BACC_PREFERRED && !first_pref_acc_idx)
					first_pref_acc_idx = i+1;
            }
		}
	}
	if(first_pref_acc_idx) {
		const RegisterTbl::Rec & r_reg = Regs.at(first_pref_acc_idx-1);
		PPBankAccount item(r_reg);
		assert(r_reg.RegTypeID == PPREGT_BANKACCOUNT);
		assert(item.AccType == PPBAC_CURRENT);
		assert(item.Flags & PREGF_BACC_PREFERRED);
		ASSIGN_PTR(pRec, item);
		ok = 1;
	}
	else if(first_acc_idx) {
		const RegisterTbl::Rec & r_reg = Regs.at(first_acc_idx-1);
		PPBankAccount item(r_reg);
		assert(r_reg.RegTypeID == PPREGT_BANKACCOUNT);
		assert(item.AccType == PPBAC_CURRENT);
		ASSIGN_PTR(pRec, item);
		ok = 1;
	}
	return ok;
}

bool FASTCALL PPPersonPacket::GetExtName(SString & rBuf) const
{
	rBuf = ExtString;
	return rBuf.NotEmptyS();
}

int PPPersonPacket::AddRegister(PPID regTypeID, const char * pNumber, int checkUnique /* = 1 */)
{
	int    ok = -1;
	char   temp_buf[128];
	RegisterTbl::Rec reg_rec;
	PPObjRegister reg_obj;
	STRNSCPY(temp_buf, pNumber);
	if(*strip(temp_buf)) {
		bool   reg_exists = false;
		reg_rec.RegTypeID = regTypeID;
		STRNSCPY(reg_rec.Num, temp_buf);
		PPObjRegisterType obj_regt;
		PPRegisterType2 regt_rec;
		THROW(obj_regt.Fetch(reg_rec.RegTypeID, &regt_rec) > 0);
		if(regt_rec.Flags & REGTF_UNIQUE) {
			RegisterTbl::Rec test_rec;
			uint   pos = 0;
			while(Regs.GetRegister(reg_rec.RegTypeID, &pos, &test_rec) > 0) {
				if(sstreq(test_rec.Num, reg_rec.Num))
					reg_exists = true;
				else
					Regs.atFree(--pos);
			}
		}
		if(!reg_exists) {
			THROW(!checkUnique || reg_obj.CheckUniqueNumber(&reg_rec, &Regs, 0, 0));
			THROW_SL(Regs.insert(&reg_rec));
			ok = 1;
		}
	}
	CATCHZOK
	return ok;
}

int PPPersonPacket::ReplaceDlvrLoc(PPID locID, PPID replacementID)
{
	int    ok = -1;
	if(DlvrLocList.getCount()) {
		PPObjLocation loc_obj;
		PPLocationPacket replacement_pack;
		THROW(loc_obj.GetPacket(replacementID, &replacement_pack) > 0);
		THROW_PP(replacement_pack.Type == LOCTYP_ADDRESS, PPERR_REPLACEMENTID_NOTADDR);
		for(uint i = 0; ok < 0 && i < DlvrLocList.getCount(); i++) {
			if(DlvrLocList.at(i)->ID == locID) {
				int has_replacement = 0;
				for(uint j = 0; !has_replacement && j < DlvrLocList.getCount(); j++)
					if(DlvrLocList.at(j)->ID == replacementID)
						has_replacement = 1;
				DlvrLocList.atFree(i);
				if(!has_replacement) {
					THROW(AddDlvrLoc(replacement_pack));
					ok = 1;
				}
				else
					ok = 2;
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPPersonPacket::EnumDlvrLoc(uint * pPos, PPLocationPacket * pItem) const
{
	if(*pPos < DlvrLocList.getCount()) {
		ASSIGN_PTR(pItem, *DlvrLocList.at(*pPos));
		(*pPos)++;
		return 1;
	}
	else
		return 0;
}

int PPPersonPacket::GetDlvrLocByPos(uint pos, PPLocationPacket * pItem) const
{
	if(pos < DlvrLocList.getCount()) {
		ASSIGN_PTR(pItem, *DlvrLocList.at(pos));
		return 1;
	}
	else
		return 0;
}

int PPPersonPacket::AddDlvrLoc(const PPLocationPacket & rItem)
{
	PPLocationPacket * p_new_pack = new PPLocationPacket(rItem);
	return p_new_pack ? (DlvrLocList.insert(p_new_pack) ? 1 : PPSetErrorSLib()) : PPSetErrorNoMem();
}

int PPPersonPacket::PutDlvrLoc(uint pos, const PPLocationPacket * pItem)
{
	int    ok = 1;
	THROW_INVARG(pos < DlvrLocList.getCount());
	if(pItem == 0) {
		DlvrLocList.atFree(pos);
	}
	else {
		PPLocationPacket * p_new_pack = new PPLocationPacket(*pItem);
		THROW_MEM(p_new_pack);
		DlvrLocList.atFree(pos);
		THROW_SL(DlvrLocList.atInsert(pos, p_new_pack));
	}
	CATCHZOK
	return ok;
}

int PPPersonPacket::SetSCard(const PPSCardPacket * pScPack, int autoCreate)
{
	int    ok = 1;
	ZDELETE(P_SCardPack);
	if(pScPack) {
		P_SCardPack = new PPSCardPacket;
		if(P_SCardPack) {
			*P_SCardPack = *pScPack;
			SETFLAG(P_SCardPack->Rec.Flags, SCRDF_AUTOCREATE, autoCreate);
		}
		else
			ok = PPSetErrorNoMem();
	}
	return ok;
}
//
// PersonCore
//
PersonCore::PersonCore() : PersonTbl()
{
}

int PersonCore::Search(PPID id, PersonTbl::Rec * pRec) { return SearchByID(this, PPOBJ_PERSON, id, pRec); }

SString & PersonCore::GetItemMemo(PPID id, SString & rBuf) // @v11.1.12
{
	rBuf.Z();
	if(id) {
		PPRef->UtrC.GetText(TextRefIdent(PPOBJ_PERSON, id, PPTRPROP_MEMO), rBuf);
		rBuf.Transf(CTRANSF_UTF8_TO_INNER);
	}
	return rBuf;
}

int PersonCore::SearchByName(const char * pName, PPID * pID, PersonTbl::Rec * pRec)
{
	PersonTbl::Key1 k1;
	MEMSZERO(k1);
	strip(STRNSCPY(k1.Name, pName));
	int    ok = SearchByKey(this, 1, &k1, pRec);
	ASSIGN_PTR(pID, ((ok > 0) ? data.ID : 0));
	return ok;
}

int PersonCore::SearchByName(const char * pName, PPIDArray & rList)
{
	int    ok = -1;
	rList.Z();
	if(!isempty(pName)) {
		PersonTbl::Key1 k;
		SString pattern_buf;
		(pattern_buf = pName).Strip();
		MEMSZERO(k);
		STRNSCPY(k.Name, pattern_buf);
		if(search(1, &k, spEq)) {
			do {
				rList.add(data.ID);
				ok = 1;
			} while(search(1, &k, spNext) && stricmp866(data.Name, pattern_buf) == 0);
		}	
	}
	return ok;
}

int PersonCore::UpdateFlags(PPID id, long setF, long resetF, int use_ta)
{
	int    ok = -1;
	PersonTbl::Rec rec;
	if(setF || resetF) {
		PPTransaction tra(use_ta);
		THROW(tra);
		if(SearchByID_ForUpdate(this, PPOBJ_PERSON, id, &rec) > 0) {
			long   old_f = rec.Flags;
			if(setF & PSNF_NOVATAX) rec.Flags |= PSNF_NOVATAX;
			if(resetF & PSNF_NOVATAX) rec.Flags &= ~PSNF_NOVATAX;
			if(setF & PSNF_HASIMAGES) rec.Flags |= PSNF_HASIMAGES;
			if(resetF & PSNF_HASIMAGES) rec.Flags &= ~PSNF_HASIMAGES;
			if(old_f != rec.Flags) {
				THROW_DB(updateRecBuf(&rec)); // @sfu
				DS.LogAction(PPACN_OBJUPD, PPOBJ_PERSON, id, 0, 0);
				ok = 1;
			}
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int PersonCore::PutKinds(PPID id, PPPerson * pPack)
{
	int    ok = 1;
	uint   i, pos;
	if(id) {
		PersonKindTbl::Key1 k1;
		k1.PersonID = id;
		k1.KindID   = 0;
		for(int sp = spGe; Kind.search(1, &k1, sp) && Kind.data.PersonID == id; sp = spNext) {
			if(pPack->Kinds.lsearch(Kind.data.KindID, &(pos = 0))) {
				if(strncmp(strip(Kind.data.Name), pPack->Rec.Name, sizeof(Kind.data.Name)) != 0) {
					THROW_DB(Kind.rereadForUpdate(1, &k1));
					MEMSZERO(Kind.data.Name);
					STRNSCPY(Kind.data.Name, pPack->Rec.Name);
					THROW_DB(Kind.updateRec()); // @sfu
				}
				pPack->Kinds.atFree(pos);
			}
			else {
				THROW_DB(Kind.rereadForUpdate(1, &k1));
				THROW_DB(Kind.deleteRec()); // @sfu
			}
		}
		for(i = 0; i < pPack->Kinds.getCount(); i++) {
			Kind.clearDataBuf();
			Kind.data.PersonID = id;
			Kind.data.KindID   = pPack->Kinds.at(i);
			STRNSCPY(Kind.data.Name, pPack->Rec.Name);
			THROW_DB(Kind.insertRec());
		}
	}
	CATCHZOK
	return ok;
}

int PersonCore::Put(PPID * pID, PPPerson * pPack, int use_ta)
{
	int    ok = 1;
	Reference * p_ref = PPRef;
	SString memo_buf_utf8;
	if(pPack) {
		(memo_buf_utf8 = pPack->SMemo).Transf(CTRANSF_INNER_TO_UTF8);
	}
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(*pID) {
			THROW(SearchByID_ForUpdate(this, PPOBJ_PERSON, *pID, 0) > 0);
			if(pPack) {
				strip(pPack->Rec.Name);
				pPack->Rec.ID = *pID;
				THROW_DB(updateRecBuf(&pPack->Rec)); // @sfu
				THROW(p_ref->UtrC.SetText(TextRefIdent(PPOBJ_PERSON, *pID, PPTRPROP_MEMO), memo_buf_utf8, 0)); // @v11.1.12
				THROW(PutKinds(*pID, pPack));
				THROW(PutRelList(*pID, &pPack->GetRelList(), 0));
			}
			else {
				THROW_DB(deleteRec()); // @sfu
				THROW(p_ref->UtrC.SetText(TextRefIdent(PPOBJ_PERSON, *pID, PPTRPROP_MEMO), static_cast<const wchar_t *>(0), 0)); // @v11.1.12
				THROW(RemoveKind(*pID, 0, 0));
				THROW(PutRelList(*pID, 0, 0));
			}
		}
		else if(pPack) {
			strip(pPack->Rec.Name);
			THROW(ok = AdjustNewObjID(this, PPOBJ_PERSON, &pPack->Rec));
			CopyBufFrom(&pPack->Rec, sizeof(pPack->Rec));
			THROW_DB(insertRec(0, pID));
			pPack->Rec.ID = *pID;
			THROW(p_ref->UtrC.SetText(TextRefIdent(PPOBJ_PERSON, *pID, PPTRPROP_MEMO), memo_buf_utf8, 0)); // @v11.1.12
			THROW(PutKinds(*pID, pPack));
			THROW(PutRelList(*pID, &pPack->GetRelList(), 0));
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	if(ok == 2 && CConfig.Flags & CCFLG_DEBUG) {
		SString msg_buf, fmt_buf, obj_title;
		PPLoadText(PPTXT_LOG_ADDOBJREC_JUMPED_ID, fmt_buf);
		GetObjectTitle(PPOBJ_PERSON, obj_title);
		msg_buf.Printf(fmt_buf, obj_title.cptr());
		PPLogMessage(PPFILNAM_INFO_LOG, msg_buf, LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_DBINFO);
	}
	return ok;
}

int PersonCore::Get(PPID id, PPPerson * pPack)
{
	int    ok = Search(id, pPack ? &pPack->Rec : 0);
	if(ok > 0 && pPack) {
		GetItemMemo(id, pPack->SMemo); // @v11.1.12
		pPack->Kinds.clear();
		THROW(GetKindList(id, &pPack->Kinds));
		THROW(GetRelList(id, &pPack->RelList, 0));
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int PersonCore::SearchMainOrg(PersonTbl::Rec * pRec)
{
	PersonKindTbl::Key0 k;
	MEMSZERO(k);
	k.KindID = PPPRK_MAIN;
	if(Kind.search(0, &k, spGe) && k.KindID == PPPRK_MAIN && Search(k.PersonID, pRec) > 0)
		return 1;
	return (BTROKORNFOUND) ? (PPErrCode = PPERR_MAINORGNFOUND, -1) : PPSetErrorDB();
}

int PersonCore::_SearchKind(PPID id, PPID kind)
{
	PersonKindTbl::Key1 k;
	k.PersonID = id;
	k.KindID   = kind;
	return Kind.search(1, &k, spEq) ? 1 : PPDbSearchError();
}

int PersonCore::IsBelongsToKind(PPID id, PPID kind)
{
	return (_SearchKind(id, kind) > 0) ? 1 : PPSetError(PPERR_PSNDONTBELONGTOKIND);
}

int PersonCore::GetListByKind(PPID kindID, PPIDArray * pList)
{
	int    ok = -1;
	PersonKindTbl::Key0 k0;
	PersonKindTbl * t = &this->Kind;
	MEMSZERO(k0);
	BExtQuery q(t, 0, pList ? 512 : 4);
	q.select(t->PersonID, 0L);
	q.where(t->KindID == kindID);
	k0.KindID = kindID;
	for(q.initIteration(false, &k0, spGe); q.nextIteration() > 0;) {
		ok = 1;
		if(pList) {
			THROW_SL(pList->add(t->data.PersonID));
		}
		else
			break;
	}
	CATCHZOK
	return ok;
}

int PersonCore::AddKind(PPID id, PPID kind, int use_ta)
{
	int    ok = 1, r;
	THROW(r = _SearchKind(id, kind));
	if(r < 0) {
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(Search(id) > 0);
		Kind.clearDataBuf();
		Kind.data.KindID   = kind;
		Kind.data.PersonID = id;
		STRNSCPY(Kind.data.Name, data.Name);
		THROW_DB(Kind.insertRec());
		//
		// Если присвоенный вид не PPPRK_UNKNOWN, то отзываем у персоналии вид PPPRK_UNKNOWN
		//
		if(kind != PPPRK_UNKNOWN) {
			THROW_DB(deleteFrom(&Kind, 0, Kind.PersonID == id && Kind.KindID == PPPRK_UNKNOWN));
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int PersonCore::RemoveKind(PPID id, PPID kind, int use_ta)
{
	int    ok = 1, r;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(kind == 0) {
			THROW_DB(deleteFrom(&Kind, 0, Kind.PersonID == id));
		}
		else {
			THROW_DB(deleteFrom(&Kind, 0, Kind.PersonID == id && Kind.KindID == kind));
			//
			// Если у персоналии не осталось ни одного вида, то автоматически присваиваем
			// ей вид UNKNOWN.
			//
			THROW(r = GetKindList(id, 0));
			if(r < 0)
				THROW(AddKind(id, PPPRK_UNKNOWN, 0));
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int PersonCore::GetKindList(PPID personID, PPIDArray * pList)
{
	int    ok = -1;
	PersonKindTbl::Key1 k1;
	k1.PersonID = personID;
	k1.KindID   = 0;
	for(int sp = spGe; Kind.search(1, &k1, sp) && k1.PersonID == personID; sp = spNext) {
		CALLPTRMEMB(pList, addUnique(Kind.data.KindID));
		ok = 1;
	}
	return PPDbSearchError() ? ok : 0;
}

int PersonCore::GetVATFreePersonList(PPIDArray * list)
{
	PPUserFuncProfiler ufp(PPUPRF_VATFREEPERSONLIST);
	int    ok = 1;
	PPID   id = 0;
	long   _c = 0;
	BExtQuery q(this, 0, 128);
	q.select(this->ID, this->Flags, 0L).where(this->Flags > 0L); // @v8.1.2 where(this->Flags > 0)
	list->freeAll();
	for(q.initIteration(false, &id, spGe); ok && q.nextIteration() > 0;) {
		_c++;
		if(data.Flags & PSNF_NOVATAX)
			if(!list->add(data.ID))
				ok = PPSetErrorSLib();
	}
	ufp.SetFactor(0, (double)_c);
	ufp.Commit();
	return ok;
}

int PersonCore::PutRelList(PPID id, const LAssocArray * pList, int use_ta)
{
	int    ok = 1;
	Reference * p_ref = PPRef;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(p_ref->Assc.Remove(PPASS_PERSONREL, id, 0, 0));
		if(pList) {
			LAssoc * p_item = 0;
			LAssocArray list = *pList;
			list.Sort();
			PPID  prev_scnd = 0;
			uint  c = 0;
			for(uint i = 0; list.enumItems(&i, (void **)&p_item);) {
				PPID   assc_id = 0;
				RelationRecord rel_rec;
				MEMSZERO(rel_rec);
				rel_rec.AsscType = PPASS_PERSONREL;
				rel_rec.PrmrObjID = id;
				if(p_item->Key == prev_scnd) {
					rel_rec.ScndObjID = p_item->Key | ((++c) << 24);
					THROW_PP(c < MAXSAMEPSNREL, PPERR_MAXDUPPSNREL);
				}
				else {
					rel_rec.ScndObjID = p_item->Key;
					c = 0;
				}
				rel_rec.RelTypeID = p_item->Val;
				p_ref->Assc.SearchFreeNum(rel_rec.AsscType, rel_rec.PrmrObjID, &rel_rec.InnerNum, 0);
				THROW(p_ref->Assc.Add(&assc_id, (ObjAssocTbl::Rec *)&rel_rec, 0));
				prev_scnd = p_item->Key;
			}
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int PersonCore::GetRelList(PPID id, LAssocArray * pList, int reverse)
{
	int    ok = 1;
	Reference * p_ref = PPRef;
	RelationRecord rel_rec;
	STATIC_ASSERT(sizeof(rel_rec) == sizeof(ObjAssocTbl::Rec));
	pList->clear();
	if(!reverse) {
		for(PPID next_id = 0; p_ref->Assc.EnumByPrmr(PPASS_PERSONREL, id, &next_id, (ObjAssocTbl::Rec *)&rel_rec) > 0;) {
			PPID   scnd_id = (rel_rec.ScndObjID & ~0xff000000);
			if(!pList->SearchPair(scnd_id, rel_rec.RelTypeID, 0))
				pList->Add(scnd_id, rel_rec.RelTypeID, 0, 0);
		}
	}
	else {
		for(uint i = 0; i < MAXSAMEPSNREL; i++) {
			const  PPID scnd_id = id | (i << 24);
			for(SEnum en = p_ref->Assc.Enum(PPASS_PERSONREL, scnd_id, 1); en.Next(&rel_rec) > 0;) {
				if(!pList->SearchPair(rel_rec.PrmrObjID, rel_rec.RelTypeID, 0))
					pList->Add(rel_rec.PrmrObjID, rel_rec.RelTypeID, 0, 0);
			}
		}
	}
	return ok;
}

/*static*/int PersonCore::Helper_GetELinksFromPropRec(const PropertyTbl::Rec * pRec, const size_t recLen, PPELinkArray * pList)
{
	int    ok = -1;
	if(pRec && pList) {
		for(size_t i = PROPRECFIXSIZE; i < recLen;) {
			PPELink entry;
			entry.KindID = *reinterpret_cast<const PPID *>((PTR8C(pRec) + PROPRECFIXSIZE) + i - PROPRECFIXSIZE);
			i += sizeof(entry.KindID);
			STRNSCPY(entry.Addr, reinterpret_cast<const char *>((PTR8C(pRec) + PROPRECFIXSIZE) + i - PROPRECFIXSIZE));
			i += (sstrlen(entry.Addr) + 1);
			THROW_SL(pList->insert(&entry));
			ok = 1;
		}
	}
	CATCHZOK
	return ok;
}

int PersonCore::GetELinkList(int elnkrt, PPID personKindID, StrAssocArray & rList)
{
	rList.Z();
	int    ok = -1;
	Reference * p_ref = PPRef;
	PPELinkArray temp_list;
	PPIDArray additional_psn_list; 
	size_t buf_sz = SKILOBYTE(8);
	size_t max_req_buf_size = 0;
	PropertyTbl::Rec * p_buf = 0;
	PropertyTbl::Key1 k1;
	PPObjELinkKind elk_obj;
	PPELinkKind elk_rec;
	PPIDArray list_by_kind;
	if(personKindID) {
		GetListByKind(personKindID, &list_by_kind);
		list_by_kind.sortAndUndup();
	}
	MEMSZERO(k1);
	k1.ObjType = PPOBJ_PERSON;
	k1.Prop = PSNPRP_ELINK;
	THROW_MEM(p_buf = static_cast<PropertyTbl::Rec *>(SAlloc::M(buf_sz)));
	if(p_ref->Prop.search(1, &k1, spGe) && p_ref->Prop.data.ObjType == PPOBJ_PERSON && p_ref->Prop.data.Prop == PSNPRP_ELINK) do {
		const  PPID psn_id = p_ref->Prop.data.ObjID;
		if(!personKindID || list_by_kind.bsearch(psn_id)) {
			const size_t rec_sz = (size_t)p_ref->Prop.data.Val2 + PROPRECFIXSIZE;
			if(buf_sz < rec_sz) {
				max_req_buf_size = rec_sz;
				additional_psn_list.add(psn_id);
			}
			else {
				memcpy(p_buf, &p_ref->Prop.data, rec_sz);
				//
				temp_list.clear();
				THROW(Helper_GetELinksFromPropRec(p_buf, rec_sz, &temp_list));
				for(uint j = 0; j < temp_list.getCount(); j++) {
					const PPELink & r_item = temp_list.at(j);
					if(r_item.Addr[0] && (!elnkrt || (elk_obj.Fetch(r_item.KindID, &elk_rec) > 0 && elk_rec.Type == elnkrt))) {
						rList.AddFast(psn_id, r_item.Addr);
						ok = 1;
					}
				}
			}
		}
	} while(p_ref->Prop.search(1, &k1, spNext) && p_ref->Prop.data.ObjType == PPOBJ_PERSON && p_ref->Prop.data.Prop == PSNPRP_ELINK);
	if(additional_psn_list.getCount()) {
		buf_sz = max_req_buf_size;
		THROW_MEM(p_buf = static_cast<PropertyTbl::Rec *>(SAlloc::R(p_buf, buf_sz)));
		for(uint i = 0; i < additional_psn_list.getCount(); i++) {
			const  PPID psn_id = additional_psn_list.get(i);
			THROW(p_ref->GetProperty(PPOBJ_PERSON, psn_id, PSNPRP_ELINK, p_buf, buf_sz) > 0);
			const size_t rec_sz = (size_t)p_ref->Prop.data.Val2 + PROPRECFIXSIZE;
			//
			temp_list.clear();
			THROW(Helper_GetELinksFromPropRec(p_buf, rec_sz, &temp_list));
			for(uint j = 0; j < temp_list.getCount(); j++) {
				const PPELink & r_item = temp_list.at(j);
				if(r_item.Addr[0] && (!elnkrt || (elk_obj.Fetch(r_item.KindID, &elk_rec) > 0 && elk_rec.Type == elnkrt))) {
					rList.AddFast(psn_id, r_item.Addr);
					ok = 1;
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

/*static*/void PersonCore::SetGender(PersonTbl::Rec & rRec, int gender)
{
	rRec.Flags &= ~(PSNF_GENDER_MALE|PSNF_GENDER_FEMALE);
	if(gender == GENDER_MALE)
		rRec.Flags |= PSNF_GENDER_MALE;	
	else if(gender == GENDER_FEMALE)
		rRec.Flags |= PSNF_GENDER_FEMALE;	
	else if(gender != GENDER_UNDEF)
		rRec.Flags |= (PSNF_GENDER_MALE|PSNF_GENDER_FEMALE);	
}

/*static*/int  PersonCore::GetGender(const PersonTbl::Rec & rRec)
{
	if(rRec.Flags & PSNF_GENDER_MALE)
		return (rRec.Flags & PSNF_GENDER_FEMALE) ? GENDER_QUESTIONING : GENDER_MALE;
	else if(rRec.Flags & PSNF_GENDER_FEMALE)
		return GENDER_FEMALE;
	else
		return GENDER_UNDEF;
}

/*static*/int PersonCore::GetELinks(PPID id, PPELinkArray & rEla)
{
	int    ok = 1;
	int    r;
	Reference * p_ref = PPRef;
	size_t sz = SKILOBYTE(4);
	PropertyTbl::Rec * p_buf = 0;
	rEla.clear();
	THROW_MEM(p_buf = static_cast<PropertyTbl::Rec *>(SAlloc::M(sz)));
	THROW(r = p_ref->GetProperty(PPOBJ_PERSON, id, PSNPRP_ELINK, p_buf, sz));
	if(r > 0) {
		size_t i = sz;
		sz = static_cast<size_t>(p_buf->Val2 + PROPRECFIXSIZE);
		if(i < sz) {
			THROW_MEM(p_buf = static_cast<PropertyTbl::Rec *>(SAlloc::R(p_buf, sz)));
			THROW(p_ref->GetProperty(PPOBJ_PERSON, id, PSNPRP_ELINK, p_buf, sz) > 0);
		}
		THROW(Helper_GetELinksFromPropRec(p_buf, sz, &rEla));
	}
	THROW(r);
	CATCHZOK
	SAlloc::F(p_buf);
	return ok;
}

/*static*/int PersonCore::PutELinks(PPID id, PPELinkArray * ary, int use_ta)
{
	int    ok = 1;
	uint   i;
	size_t sz = 0;
	PropertyTbl::Rec * b = 0;
	char    * p = 0;
	PPELink * entry;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(ary && ary->getCount()) {
			sz = PROPRECFIXSIZE;
			for(i = 0; ary->enumItems(&i, (void **)&entry);)
				if(entry->KindID && *strip(entry->Addr))
					sz += (sizeof(entry->KindID) + sstrlen(entry->Addr) + 1);
			THROW_MEM(b = static_cast<PropertyTbl::Rec *>(SAlloc::C(1, sz)));
			b->Val2 = static_cast<int32>(sz - PROPRECFIXSIZE);
			for(p = reinterpret_cast<char *>(PTR8(b)+PROPRECFIXSIZE), i = 0; ary->enumItems(&i, (void **)&entry);)
				if(entry->KindID && entry->Addr[0]) {
					size_t s = (sizeof(entry->KindID) + sstrlen(entry->Addr) + 1);
					memmove(p, entry, s);
					p += s;
				}
		}
		THROW(PPRef->PutProp(PPOBJ_PERSON, id, PSNPRP_ELINK, b, sz));
		THROW(tra.Commit());
	}
	CATCHZOK
	SAlloc::F(b);
	return ok;
}
