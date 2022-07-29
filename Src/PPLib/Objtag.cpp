// OBJTAG.CPP
// Copyright (c) A.Sobolev 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022
// @codepage UTF-8
// Теги объектов
//
#include <pp.h>
#pragma hdrstop
#include <charry.h>
//
// TagFilt
//
// @v11.3.6 (moved to PPConstParam::P_TagValRestrict_Empty) const char * P_EmptyTagValRestrict = "#EMPTY";
// @v11.3.6 (moved to PPConstParam::P_TagValRestrict_Exist) const char * P_ExistTagValRestrict = "#EXIST";
// @v11.3.6 (moved to PPConstParam::P_TagValRestrict_List)  const char * P_ListTagValRestrict = "#LIST";

/*static*/int TagFilt::ParseString(const char * pItemString, SString & rRestrictionBuf, SString & rColorBuf)
{
	int    ok = 0;
	const  char * p = 0;
	if(pItemString && ((p = strstr(pItemString, "/#")) != 0)) {
		(rColorBuf = (p+1)).Strip(); // '/' пропускаем
		(rRestrictionBuf = pItemString).Trim(p - pItemString).Strip();
		ok = 2;
	}
	else {
		rColorBuf.Z();
		(rRestrictionBuf = pItemString).Strip();
		if(rRestrictionBuf.HasPrefixIAscii(_PPConst.P_TagValRestrict_List))
			ok = 4;
		else
			ok = 1;
	}
	return ok;
}

/*static*/int FASTCALL TagFilt::SetRestrictionIdList(SString & rRestrictionBuf, const PPIDArray & rList)
{
	int    ok = -1;
	rRestrictionBuf.Z();
	const uint _c = rList.getCount();
	if(_c == 1) {
        rRestrictionBuf.Cat(rList.get(0));
		ok = 1;
	}
	else if(_c > 1) {
		rRestrictionBuf.Cat(_PPConst.P_TagValRestrict_List).Colon();
		for(uint i = 0; i < _c; i++) {
			if(i)
				rRestrictionBuf.Semicol();
			rRestrictionBuf.Cat(rList.get(i));
		}
		ok = 2;
	}
	return ok;
}

/*static*/int FASTCALL TagFilt::GetRestrictionIdList(const SString & rRestrictionBuf, PPIDArray * pList)
{
	int    ok = -1;
	CALLPTRMEMB(pList, clear());
	if(rRestrictionBuf.HasPrefixIAscii(_PPConst.P_TagValRestrict_List)) {
		SString restrict = rRestrictionBuf;
		restrict.ShiftLeft(sstrlen(_PPConst.P_TagValRestrict_List)).Strip().ShiftLeftChr(':').Strip();
		StringSet ss(';', restrict);
		for(uint ssp = 0; ss.get(&ssp, restrict);) {
			const PPID restrict_val = restrict.ToLong();
			if(restrict_val > 0) {
				CALLPTRMEMB(pList, add(restrict_val));
				ok = 2;
			}
		}
	}
	else {
		const PPID restrict_val = rRestrictionBuf.ToLong();
        if(restrict_val > 0) {
			CALLPTRMEMB(pList, add(restrict_val));
			ok = 1;
        }
	}
	return ok;
}

void TagFilt::MergeString(const char * pRestrictionString, const char * pColorString, SString & rItemBuf)
{
	rItemBuf.Z();
	SString temp_buf(pRestrictionString);
	if(temp_buf.NotEmptyS())
		rItemBuf.Cat(temp_buf);
	temp_buf = pColorString;
	if(temp_buf.NotEmptyS() && temp_buf.C(0) == '#')
		rItemBuf.CatChar('/').Cat(temp_buf);
}

/*static*/void FASTCALL TagFilt::SetRestriction(const char * pRestrictionString, SString & rItemBuf)
{
	SString rbuf, cbuf;
	ParseString(rItemBuf, rbuf, cbuf);
	MergeString(pRestrictionString, cbuf, rItemBuf);
}

/*static*/void FASTCALL TagFilt::SetColor(const SColor * pClr, SString & rItemBuf)
{
	SString rbuf, cbuf;
	ParseString(rItemBuf, rbuf, cbuf);
	if(pClr)
		pClr->ToStr(cbuf, SColorBase::fmtHEX);
	else
		cbuf.Z();
	MergeString(rbuf, cbuf, rItemBuf);
}

/*static*/int FASTCALL TagFilt::GetRestriction(const char * pItemString, SString & rRestrictionBuf)
{
	SString cbuf;
	return ParseString(pItemString, rRestrictionBuf, cbuf);
}

/*static*/int FASTCALL TagFilt::GetColor(const char * pItemString, SColor & rClr)
{
	int    ok = 0;
	SString rbuf, cbuf;
	ParseString(pItemString, rbuf, cbuf);
	if(cbuf.NotEmptyS()) {
		rClr.FromStr(cbuf);
		ok = 1;
	}
	return ok;
}

IMPLEMENT_PPFILT_FACTORY(Tag); TagFilt::TagFilt() : PPBaseFilt(PPFILT_TAG, 0, 1)
{
	SetFlatChunk(offsetof(TagFilt, ReserveStart),
		offsetof(TagFilt, TagsRestrict) - offsetof(TagFilt, ReserveStart));
	SetBranchStrAssocArray(offsetof(TagFilt, TagsRestrict));
	Init(1, 0);
}

TagFilt & FASTCALL TagFilt::operator = (const TagFilt & rS)
{
	Copy(&rS, 1);
	return *this;
}

bool TagFilt::IsEmpty() const
{
	return !TagsRestrict.getCount();
}

static int CheckTagItemForRawRestriction(const ObjTagItem * pTagItem, const char * pRawRestrictionString, SColor & rClr)
{
	int    select_ok = 0;
	if(pTagItem) {
		SString restriction;
		TagFilt::GetRestriction(pRawRestrictionString, restriction);
		if(restriction.IsEqiAscii(_PPConst.P_TagValRestrict_Empty))
			select_ok = pTagItem->IsZeroVal();
		else if(restriction.IsEqiAscii(_PPConst.P_TagValRestrict_Exist))
			select_ok = 1;
		else if(oneof3(pTagItem->TagDataType, OTTYP_BOOL, OTTYP_ENUM, OTTYP_OBJLINK))
			select_ok = BIN(restriction.ToLong() == pTagItem->Val.IntVal);
		else if(pTagItem->TagDataType == OTTYP_NUMBER) {
			RealRange rr;
			strtorrng(restriction.cptr(), &rr.low, &rr.upp);
			select_ok = rr.CheckVal(pTagItem->Val.RealVal);
		}
		else if(oneof2(pTagItem->TagDataType, OTTYP_STRING, OTTYP_GUID)) {
			size_t len = sstrlen(pTagItem->Val.PStr);
			if(restriction.Len() && len) {
				if(restriction.C(0) == '*')
					select_ok = BIN(stristr866(pTagItem->Val.PStr, restriction.ShiftLeft()));
				else if(restriction.C(restriction.Len() - 1) == '*')
					select_ok = BIN(strnicmp866(restriction, pTagItem->Val.PStr, restriction.Len() - 1) == 0);
				else
					select_ok = BIN(stricmp866(restriction, pTagItem->Val.PStr) == 0);
			}
			else
				select_ok = BIN(!restriction.Len() && !len);
		}
		else if(pTagItem->TagDataType == OTTYP_DATE) {
			DateRange period;
			strtoperiod(restriction, &period, 0);
			period.Actualize(pTagItem->Val.DtVal);
			select_ok = BIN(period.CheckDate(pTagItem->Val.DtVal) > 0);
		}
		if(select_ok)
			TagFilt::GetColor(pRawRestrictionString, rClr);
	}
	return select_ok;
}

int TagFilt::SelectIndicator(const ObjTagList * pTagList, SColor & rClr) const
{
	int    select_ok = 0;
	if(pTagList) {
		for(uint i = 0; !select_ok && i < TagsRestrict.getCount(); i++) {
			StrAssocArray::Item tr_item = TagsRestrict.at_WithoutParent(i);
			select_ok = CheckTagItemForRawRestriction(pTagList->GetItem(tr_item.Id), tr_item.Txt, rClr);
		}
	}
	return select_ok;
}

int TagFilt::SelectIndicator(PPID objID, SColor & rClr) const
{
	int    select_ok = 0;
	PPObjTag tag_obj;
	for(uint i = 0; !select_ok && i < TagsRestrict.getCount(); i++) {
		StrAssocArray::Item tr_item = TagsRestrict.at_WithoutParent(i);
		ObjTagItem item;
		if(tag_obj.FetchTag(objID, tr_item.Id, &item) > 0)
			select_ok = CheckTagItemForRawRestriction(&item, tr_item.Txt, rClr);
	}
	return select_ok;
}

int TagFilt::Helper_CheckTagItemForRestrict_EnumID(const ObjTagItem * pItem, long restrictVal) const
{
	int    check_ok = 0;
	if(restrictVal == pItem->Val.IntVal)
		check_ok = 1;
	else {
		PPID   item_id = pItem->Val.IntVal;
		if(item_id && IS_DYN_OBJTYPE(pItem->TagEnumID)) {
			PPObjTag tag_obj;
			PPObjectTag tag_rec;
			if(tag_obj.Fetch(pItem->TagID, &tag_rec) > 0 && tag_rec.Flags & OTF_HIERENUM) {
				assert(tag_rec.TagEnumID == pItem->TagEnumID);
				Reference * p_ref = PPRef;
				ReferenceTbl::Rec rec;
				PPIDArray recur_list;
				if(p_ref->GetItem(tag_rec.TagEnumID, item_id, &rec) > 0 && rec.Val2) {
					recur_list.add(item_id);
					for(item_id = rec.Val2; !check_ok && item_id && p_ref->GetItem(tag_rec.TagEnumID, item_id, &rec) > 0; item_id = rec.Val2) {
						if(recur_list.addUnique(item_id) > 0) {
							if(restrictVal == item_id) {
								check_ok = 1;
							}
						}
						else
							break; // Зацикленная цепь ребенок->родитель
					}
				}
			}
		}
	}
	return check_ok;
}

int TagFilt::CheckTagItemForRestrict(const ObjTagItem * pItem, const SString & rRestrict) const
{
	int    check_ok = 1;
	if(pItem) {
		if(Flags & TagFilt::fNotTagsInList)
			check_ok = 0;
		else {
			if(rRestrict.IsEqiAscii(_PPConst.P_TagValRestrict_Empty))
				check_ok = pItem->IsZeroVal();
			else if(rRestrict.IsEqiAscii(_PPConst.P_TagValRestrict_Exist))
				check_ok = 1;
			else if(oneof2(pItem->TagDataType, OTTYP_BOOL, OTTYP_OBJLINK))
				check_ok = BIN(rRestrict.ToLong() == pItem->Val.IntVal);
			else if(pItem->TagDataType == OTTYP_ENUM) {
				if(rRestrict.HasPrefixIAscii(_PPConst.P_TagValRestrict_List)) {
					SString restrict = rRestrict;
					restrict.ShiftLeft(sstrlen(_PPConst.P_TagValRestrict_List)).Strip().ShiftLeftChr(':').Strip();
					StringSet ss(';', restrict);
					check_ok = 0;
					for(uint ssp = 0; !check_ok && ss.get(&ssp, restrict);) {
						const PPID restrict_val = restrict.ToLong();
						check_ok = Helper_CheckTagItemForRestrict_EnumID(pItem, restrict_val);
					}
				}
				else {
					const PPID restrict_val = rRestrict.ToLong();
					check_ok = Helper_CheckTagItemForRestrict_EnumID(pItem, restrict_val);
				}
			}
			else if(pItem->TagDataType == OTTYP_NUMBER) {
				RealRange rr;
				strtorrng(rRestrict.cptr(), &rr.low, &rr.upp);
				check_ok = rr.CheckVal(pItem->Val.RealVal);
			}
			else if(oneof2(pItem->TagDataType, OTTYP_STRING, OTTYP_GUID)) {
				size_t len = pItem->Val.PStr ? sstrlen(pItem->Val.PStr) : 0;
				if(rRestrict.Len() && len) {
					if(rRestrict.C(0) == '*') {
						SString temp_buf = rRestrict;
						check_ok = BIN(stristr866(pItem->Val.PStr, temp_buf.ShiftLeft()));
					}
					else if(rRestrict.C(rRestrict.Len() - 1) == '*')
						check_ok = BIN(strnicmp866(rRestrict, pItem->Val.PStr, rRestrict.Len() - 1) == 0);
					else
						check_ok = BIN(stricmp866(rRestrict, pItem->Val.PStr) == 0);
				}
				else
					check_ok = BIN(!rRestrict.Len() && !len);
			}
			else if(pItem->TagDataType == OTTYP_DATE) {
				DateRange period;
				strtoperiod(rRestrict, &period, strtoprdfEnableAnySign);
				period.ActualizeCmp(/*pItem->Val.DtVal*/ZERODATE, pItem->Val.DtVal);
				check_ok = BIN(period.CheckDate(pItem->Val.DtVal) > 0);
			}
		}
	}
	else if(!(Flags & TagFilt::fNotTagsInList) && !rRestrict.IsEqiAscii(_PPConst.P_TagValRestrict_Empty))
		check_ok = 0;
	return check_ok;
}

int FASTCALL TagFilt::Check(const ObjTagList * pList) const
{
	int    check_ok = 1;
	SString restrict;
	for(uint i = 0; check_ok && i < TagsRestrict.getCount(); i++) {
		StrAssocArray::Item tr_item = TagsRestrict.at_WithoutParent(i);
		TagFilt::GetRestriction(tr_item.Txt, restrict);
		const ObjTagItem * p_item = pList ? pList->GetItem(tr_item.Id) : 0;
		if(!CheckTagItemForRestrict(p_item, restrict))
			check_ok = 0;
	}
	return check_ok;
}
//
// PPObjTagPacket
//
PPObjTagPacket::PPObjTagPacket()
{
	// @v10.6.5 @ctr MEMSZERO(Rec);
}

PPObjTagPacket & PPObjTagPacket::Z()
{
	MEMSZERO(Rec);
	Rule.Z();
	return *this;
}

PPObjTagPacket & FASTCALL PPObjTagPacket::operator = (const PPObjTagPacket & rS)
{
	// @v10.6.5 memcpy(&Rec, &rS.Rec, sizeof(Rec));
	Rec = rS.Rec; // @v10.6.5 
	Rule = rS.Rule;
	return *this;
}

PPObjectTag2::PPObjectTag2()
{
	THISZERO();
}
//
// PPTagEnumList
//
PPTagEnumList::PPTagEnumList(PPID enumID) : StrAssocArray(), EnumID(enumID), Flags(0)
{
}

PPTagEnumList & FASTCALL PPTagEnumList::operator = (const PPTagEnumList & s)
{
	Copy(s);
	return *this;
}

int FASTCALL PPTagEnumList::Copy(const PPTagEnumList & src)
{
	EnumID = src.EnumID;
	Flags = src.Flags;
	return StrAssocArray::Copy(src);
}

void PPTagEnumList::SetEnumID(PPID enumID)
{
	EnumID = enumID;
}

void PPTagEnumList::SetFlags(long flags)
{
	SETFLAG(Flags, PPCommObjEntry::fHierarchical, flags & PPCommObjEntry::fHierarchical);
}

int PPTagEnumList::PutItem(PPID * pID, const char * pName, PPID parentID)
{
	int    ok = 1;
	if(!*pID) {
		if(pName && *pName) {
			long   _max = 1000;
			if(GetMaxID(&_max) < 0 || _max < 1000)
				_max = 1000;
			*pID = _max+1;
			return StrAssocArray::Add(*pID, pName, 1) ? 1 : PPSetErrorSLib();
		}
		else
			ok = -1;
	}
	else if(pName && *pName)
		ok = StrAssocArray::Add(*pID, pName, 1) ? 1 : PPSetErrorSLib();
	else
		StrAssocArray::Remove(*pID);
	return ok;
}

int PPTagEnumList::Read(PPID enumID)
{
	int    ok = 1;
	Reference * p_ref = PPRef;
	Z();
	if(enumID)
		SetEnumID(enumID);
	else
		enumID = EnumID;
	ReferenceTbl::Rec rec, hdr_rec;
	if(p_ref->GetItem(PPOBJ_DYNAMICOBJS, enumID, &hdr_rec) > 0) {
		SETFLAGBYSAMPLE(Flags, PPCommObjEntry::fHierarchical, hdr_rec.Val1);
		for(SEnum en = p_ref->Enum(enumID, 0); en.Next(&rec) > 0;) {
			THROW(PutItem(&rec.ObjID, rec.ObjName, rec.Val2));
		}
	}
	CATCHZOK
	return ok;
}

int PPTagEnumList::Write(int use_ta)
{
	int    ok = 1;
	Reference * p_ref = PPRef;
	uint   i;
	PPID   item_id;
	SString name;
	PPIDArray processed_items;
	ReferenceTbl::Rec rec, hdr_rec;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(EnumID == 0 || p_ref->GetItem(PPOBJ_DYNAMICOBJS, EnumID, &hdr_rec) < 0) {
			THROW(p_ref->AllocDynamicObj(&EnumID, 0, Flags, 0));
		}
		else if((hdr_rec.Val1 & PPCommObjEntry::fHierarchical) != (Flags & PPCommObjEntry::fHierarchical)) {
			SETFLAGBYSAMPLE(hdr_rec.Val1, PPCommObjEntry::fHierarchical, Flags);
			THROW(p_ref->UpdateItem(PPOBJ_DYNAMICOBJS, EnumID, &hdr_rec, 1, 0));
		}
		for(item_id = 0; p_ref->EnumItems(EnumID, &item_id, &rec) > 0;) {
			uint  _pos = 0;
			if(Search(item_id, &_pos)) {
				StrAssocArray::Item _item = Get(_pos);
				(name = _item.Txt).Strip();
				if(name.Cmp(strip(rec.ObjName), 0) != 0 || _item.ParentId != rec.Val2) {
					name.CopyTo(rec.ObjName, sizeof(rec.ObjName));
					rec.Val2 = _item.ParentId;
					THROW(p_ref->UpdateItem(EnumID, item_id, &rec, 1, 0));
				}
				THROW(processed_items.add(item_id));
			}
			else {
				THROW(p_ref->deleteRec());
			}
		}
		processed_items.sort();
		for(i = 0; i < getCount(); i++) {
			StrAssocArray::Item item = Get(i);
			if(!processed_items.bsearch(item.Id)) {
				MEMSZERO(rec);
				STRNSCPY(rec.ObjName, item.Txt);
				rec.Val2 = item.ParentId;
				THROW(p_ref->AddItem(EnumID, &item.Id, &rec, 0));
			}
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

/*virtual*/void * PPObjTag::CreateObjListWin(uint flags, void * extraPtr)
{
	class PPObjTagListWindow : public PPObjListWindow {
	public:
		PPObjTagListWindow(PPObject * pObj, uint flags, void * extraPtr) : PPObjListWindow(pObj, flags, extraPtr)
		{
			//
			// Так как окно не модальное, объект на который указывает extraPtr может оказаться разрушенным
			// во время работы экземпляра окна. Потому создаем собственную копию объекта и передаем
			// базовому классу ссылку на него в ExtraPtr.
			//
			if(pObj) {
				static_cast<const PPObjTag *>(pObj)->InitFilt(extraPtr, Filt);
				ExtraPtr = &Filt;
			}
			else
				ExtraPtr = 0;
			DefaultCmd = cmaEdit;
			SetToolbar(TOOLBAR_LIST_TAG);
			{
				SString title_buf;
				PPLoadString("objtags", title_buf);
				setTitle(title_buf);
			}
		}
	private:
		DECL_HANDLE_EVENT
		{
			int    update = 0;
			PPID   id = 0;
			// @v9.5.5 { Переопределяет код из PPObjListWindow::handleEvent для предустановки типа объекта в создаваемом теге
			PPID   preserve_focus_id = 0;
			if(event.isCmd(cmaInsert) && P_Obj && Flags & OLW_CANINSERT && !(Flags & OLW_OUTERLIST)) {
				PPID   obj_type_id = 0;
				if(getResult(&id) > 0 && !ObjTagFilt::ObjTypeRootIdentToObjType(id, &obj_type_id)) {
					PPObjectTag tag_rec;
					if(static_cast<PPObjTag *>(P_Obj)->Search(id, &tag_rec) > 0)
						obj_type_id = tag_rec.ObjTypeID;
				}
				ObjTagFilt filt(obj_type_id);
				filt.Flags |= ObjTagFilt::fAnyObjects;
				id = 0;
				if(P_Obj->Edit(&id, &filt) == cmOK) {
					preserve_focus_id = id;
					update = 2;
				}
				else
					::SetFocus(H());
				PostProcessHandleEvent(update, preserve_focus_id);
			}
			else { // } @v9.5.5
				PPObjListWindow::handleEvent(event);
				if(P_Obj) {
					getResult(&id);
					if(TVCOMMAND) {
						switch(TVCMD) {
							case cmaMore:
								if(id) {
									PPObjTagPacket pack;
									if(static_cast<PPObjTag *>(P_Obj)->GetPacket(id, &pack) > 0 && pack.Rec.TagDataType == OTTYP_ENUM) {
										if(pack.Rec.TagEnumID) {
											ShowObjects(pack.Rec.TagEnumID, 0);
										}
									}
								}
								break;
							case cmTransmitCharry:
								{
									PPIDArray id_list;
									ReferenceTbl::Rec rec;
									for(PPID item_id = 0; static_cast<PPObjReference *>(P_Obj)->EnumItems(&item_id, &rec) > 0;)
										id_list.add(rec.ObjID);
									if(!SendCharryObject(PPDS_CRROBJTAG, id_list))
										PPError();
									clearEvent(event);
								}
								break;
						}
					}
					PostProcessHandleEvent(update, id);
				}
			}
		}
		ObjTagFilt Filt;
	};
	return /*0; */ new PPObjTagListWindow(this, flags, extraPtr);
}
//
//
//
// DLG_TAGENUMVIEW, CTL_TAGENUMVIEW_LIST, 48
//
//
//
static int SelectObjTagType(PPObjectTag * pData, const ObjTagFilt * pObjTagF)
{
	class SelectObjTagTypeDialog : public TDialog {
		DECL_DIALOG_DATA(PPObjectTag);
	public:
		explicit SelectObjTagTypeDialog(const ObjTagFilt * pObjTagF) : TDialog(DLG_OBJTAGTYP), P_ObjTypeList(0)
		{
			RVALUEPTR(Filt, pObjTagF);
			TagObjTypeList.addzlist(PPOBJ_PERSON, PPOBJ_GOODS, PPOBJ_BILL, PPOBJ_LOT, PPOBJ_WORKBOOK,
				PPOBJ_LOCATION, PPOBJ_GLOBALUSERACC, PPOBJ_UHTTSTORE, PPOBJ_CASHNODE, PPOBJ_TRANSPORT, PPOBJ_BRAND, 0); // @v11.2.12 PPOBJ_TRANSPORT, PPOBJ_BRAND
		}
		DECL_DIALOG_SETDTS()
		{
			RVALUEPTR(Data, pData);
			if(Data.ObjTypeID)
				TagObjTypeList.addUnique(Data.ObjTypeID);
			SetupObjListCombo(this, CTLSEL_OBJTAG_TAGOBJTYP, Data.ObjTypeID, &TagObjTypeList);
			if(Data.ObjTypeID && !(Filt.Flags & ObjTagFilt::fAnyObjects))
				disableCtrl(CTLSEL_OBJTAG_TAGOBJTYP, 1);
			AddClusterAssoc(CTL_OBJTAG_TYPE, 0, OTTYP_GROUP);
			AddClusterAssoc(CTL_OBJTAG_TYPE, 1, OTTYP_BOOL);
			AddClusterAssoc(CTL_OBJTAG_TYPE, 2, OTTYP_STRING);
			AddClusterAssoc(CTL_OBJTAG_TYPE, 3, OTTYP_NUMBER);
			AddClusterAssoc(CTL_OBJTAG_TYPE, 4, OTTYP_ENUM);
			AddClusterAssoc(CTL_OBJTAG_TYPE, 5, OTTYP_DATE);
			AddClusterAssoc(CTL_OBJTAG_TYPE, 6, OTTYP_TIMESTAMP);
			AddClusterAssoc(CTL_OBJTAG_TYPE, 7, OTTYP_OBJLINK);
			AddClusterAssoc(CTL_OBJTAG_TYPE, 8, OTTYP_GUID);
			AddClusterAssoc(CTL_OBJTAG_TYPE, 9, OTTYP_IMAGE);
			SetClusterData(CTL_OBJTAG_TYPE, Data.TagDataType);
			SetupTagObjType();
			return 1;
		}
		DECL_DIALOG_GETDTS()
		{
			int    ok = 1;
			if(!Data.ObjTypeID || (Filt.Flags & ObjTagFilt::fAnyObjects)) {
				getCtrlData(CTLSEL_OBJTAG_TAGOBJTYP, &Data.ObjTypeID);
			}
			GetClusterData(CTL_OBJTAG_TYPE, &Data.TagDataType);
			if(Data.TagDataType == OTTYP_OBJLINK) {
				getCtrlData(CTLSEL_OBJTAG_OBJTYP, &Data.TagEnumID);
				getCtrlData(CTLSEL_OBJTAG_OBJGRP, &Data.LinkObjGrp);
			}
			if(Data.ObjTypeID == 0) {
                ok = PPErrorByDialog(this, CTLSEL_OBJTAG_TAGOBJTYP, PPERR_OBJTYPENEEDED);
			}
			else {
				ASSIGN_PTR(pData, Data);
			}
			return ok;
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isClusterClk(CTL_OBJTAG_TYPE)) {
				GetClusterData(CTL_OBJTAG_TYPE, &Data.TagDataType);
				SetupObjType();
			}
			else if(event.isCbSelected(CTLSEL_OBJTAG_TAGOBJTYP)) {
				PPID   obj_type = getCtrlLong(CTLSEL_OBJTAG_TAGOBJTYP);
				if(TagObjTypeList.lsearch(obj_type)) {
					if(Data.ObjTypeID != obj_type) {
						Data.ObjTypeID = obj_type;
						SetupTagObjType();
					}
				}
				else
					setCtrlLong(CTLSEL_OBJTAG_TAGOBJTYP, Data.ObjTypeID);
			}
			else if(event.isCbSelected(CTLSEL_OBJTAG_OBJTYP))
				SetupObjGroup();
			else
				return;
			clearEvent(event);
		}
		void   SetupTagObjType()
		{
			LinkObjTypeList.addzlist(PPOBJ_QCERT, PPOBJ_PERSON, PPOBJ_QUOTKIND, PPOBJ_GLOBALUSERACC, 
				PPOBJ_TAXSYSTEMKIND, PPOBJ_INTERNETACCOUNT, PPOBJ_TRANSPORT, PPOBJ_TAG, PPOBJ_WORLD, 0); 
			// @v10.0.05 PPOBJ_GLOBALUSERACC // @v10.6.12 PPOBJ_TAXSYSTEMKIND // @v10.7.11 PPOBJ_INTERNETACCOUNT // @v10.8.12 PPOBJ_TRANSPORT // @v10.9.4 PPOBJ_TAG
			// @v11.2.9 PPOBJ_WORLD
			P_ObjTypeList = &LinkObjTypeList;
			DisableClusterItem(CTL_OBJTAG_TYPE, 7, BIN(!P_ObjTypeList));
			SetupObjType();
		}
		void   SetupObjType()
		{
			if(Data.TagDataType == OTTYP_OBJLINK && P_ObjTypeList && P_ObjTypeList->getCount()) {
				disableCtrl(CTLSEL_OBJTAG_OBJTYP, 0);
				SetupObjListCombo(this, CTLSEL_OBJTAG_OBJTYP, Data.TagEnumID, P_ObjTypeList);
			}
			else
				disableCtrl(CTLSEL_OBJTAG_OBJTYP, 1);
			SetupObjGroup();
		}
		void   SetupObjGroup()
		{
			const  PPID obj_type = getCtrlLong(CTLSEL_OBJTAG_OBJTYP);
			int    dsbl = 0;
			switch(obj_type) {
				case PPOBJ_PERSON:
					SetupPPObjCombo(this, CTLSEL_OBJTAG_OBJGRP, PPOBJ_PERSONKIND, Data.LinkObjGrp, OLW_CANINSERT, 0);
					break;
				case PPOBJ_GLOBALUSERACC: // @v10.5.5
					SetupStringCombo(this, CTLSEL_OBJTAG_OBJGRP, PPTXT_GLOBALSERVICELIST, Data.LinkObjGrp);
					break;
				case PPOBJ_TAG: // @v10.9.4
					SetupObjListCombo(this, CTLSEL_OBJTAG_OBJGRP, Data.LinkObjGrp, 0);
					break;
				default:
					setCtrlLong(CTLSEL_OBJTAG_OBJGRP, 0);
					dsbl = 1;
					break;
			}
			disableCtrl(CTLSEL_OBJTAG_OBJGRP, dsbl);
		}
		ObjTagFilt Filt;
		PPIDArray TagObjTypeList;
		PPIDArray LinkObjTypeList;
		const PPIDArray * P_ObjTypeList;
	};
	DIALOG_PROC_BODY_P1(SelectObjTagTypeDialog, pObjTagF, pData);
}
//
//
//
int STDCALL SetupObjTagCombo(TDialog * dlg, uint ctl, PPID id, uint flags, ObjTagFilt * pFilt)
{
	return SetupPPObjCombo(dlg, ctl, PPOBJ_TAG, id, flags, pFilt);
}
//
//
//
/*static*/long FASTCALL ObjTagFilt::MakeObjTypeRootIdent(PPID objType)
{
	return objType ? (objType * TAG_OBJTYPEROOT_MULT) : TAG_OBJTYPEROOT_MULT;
}

/*static*/int FASTCALL ObjTagFilt::ObjTypeRootIdentToObjType(long rootIdent, PPID * pObjType)
{
	int    yes = 0;
	PPID   obj_type = 0;
	if(rootIdent >= TAG_OBJTYPEROOT_MULT) {
		if(rootIdent == TAG_OBJTYPEROOT_MULT)
			obj_type = 0;
		else
			obj_type = rootIdent / TAG_OBJTYPEROOT_MULT;
		yes = 1;
	}
	ASSIGN_PTR(pObjType, obj_type);
	return yes;
}

ObjTagFilt::ObjTagFilt(PPID objTypeID, long flags, PPID parentID)
{
	Flags = CHKXORFLAGS(flags, fOnlyGroups, fOnlyTags) | (flags & ~(fOnlyGroups|fOnlyTags));
	if(objTypeID)
		ObjTypeID = objTypeID;
	else if(Flags & fAnyObjects)
		ObjTypeID = 0;
	else
		ObjTypeID = PPOBJ_PERSON;
	ParentID = parentID;
}

ObjTagFilt & ObjTagFilt::Z()
{
	ObjTypeID = 0;
	ParentID = 0;
	Flags = 0;
	return *this;
}

/*static*/PPID PPObjTag::Helper_GetTag(PPID objType, PPID objID, const char * pTagSymb)
{
	long   sur_id = 0;
	Reference * p_ref = PPRef;
	if(p_ref) {
		PPObjTag tag_obj;
		PPID   tag_id = 0;
		ObjTagTbl::Rec tag_rec;
		if(tag_obj.SearchBySymb(pTagSymb, &tag_id, 0) > 0 && p_ref->Ot.GetTagRec(objType, objID, tag_id, &tag_rec) > 0)
			DS.GetTLA().SurIdList.Add(&sur_id, &tag_rec, sizeof(tag_rec));
	}
	return sur_id;
}

/*static*/PPID PPObjTag::Helper_GetTagByID(PPID objType, PPID objID, PPID tagID)
{
	long   sur_id = 0;
	if(tagID) {
		Reference * p_ref = PPRef;
		if(p_ref) {
			PPObjTag tag_obj;
			ObjTagTbl::Rec tag_rec;
			if(p_ref->Ot.GetTagRec(objType, objID, tagID, &tag_rec) > 0)
				DS.GetTLA().SurIdList.Add(&sur_id, &tag_rec, sizeof(tag_rec));
		}
	}
	return sur_id;
}

PPObjTag::PPObjTag(void * extraPtr) : PPObjReference(PPOBJ_TAG, extraPtr)
{
	ImplementFlags |= (implStrAssocMakeList | implTreeSelector);
}

PPObjTag::~PPObjTag()
{
}

int SelfbuildStaffForManual_ReservedObjTagList()
{
	int    ok = 1;
	uint   num_recs = 0;
	SString temp_buf;
	SString obj_type_symb;
	SString data_type_symb;
	SString tag_name;
	SString tag_symb;
	SString line_buf;
	TVRez * p_rez = P_SlRez;
	PPGetFilePath(PPPATH_OUT, "reservedobjects-tag", temp_buf);
	SFile  doc_file(temp_buf, SFile::mWrite);
	THROW_SL(doc_file.IsValid());
	THROW_PP(p_rez, PPERR_RESFAULT);
	THROW_PP(p_rez->findResource(ROD_TAG, PP_RCDATA), PPERR_RESFAULT);
	THROW_PP(num_recs = p_rez->getUINT(), PPERR_RESFAULT);
	{
			/*
		\begin{description}
			\item[Права агента на доступ к кассовым операциям]

				Ид=4
				\\Символ = \ppyrsrv{POSRIGHTS}
				\\Тип объекта=\ppyrsrv{PERSON}
				\\Тип тега = \ppyrsrv{STRING}
			\item[Дата рождения]

				Ид=5
				\\Символ = \ppyrsrv{DOB}
				\\Тип объекта=\ppyrsrv{PERSON}
				\\Тип тега = \ppyrsrv{DATE}
		\end{description}
			*/
		line_buf.Z().CatChar('\\').Cat("begin").CatChar('{').Cat("description").CatChar('}').CR();
		doc_file.WriteLine(line_buf);
		for(uint i = 0; i < num_recs; i++) {
			const PPID id = p_rez->getUINT();
			p_rez->getString(tag_name.Z(), 2); // Name
			PPExpandString(tag_name, CTRANSF_UTF8_TO_OUTER);
			p_rez->getString(tag_symb.Z(), 2); // Symb
			p_rez->getString(obj_type_symb.Z(), 2);  // ObjType
			p_rez->getString(data_type_symb.Z(), 2); // DataType
			{
				line_buf.Z();
				line_buf.Tab().CatChar('\\').Cat("item").CatBrackStr(tag_name).CR();
				line_buf.CR();
				PPLoadString("id", temp_buf);
				line_buf.Tab(2).CatEq(temp_buf.Transf(CTRANSF_INNER_TO_OUTER), id).CR();
				PPLoadString("symbol", temp_buf);
				line_buf.Tab(2).CatCharN('\\', 2).Cat(temp_buf.Transf(CTRANSF_INNER_TO_OUTER)).CatDiv('=', 1).
					CatChar('\\').Cat("ppyrsrv").CatChar('{').Cat(tag_symb).CatChar('}').CR();
				PPLoadString("objtype", temp_buf);
				line_buf.Tab(2).CatCharN('\\', 2).Cat(temp_buf.Transf(CTRANSF_INNER_TO_OUTER)).CatDiv('=', 1).
					CatChar('\\').Cat("ppyrsrv").CatChar('{').Cat(obj_type_symb).CatChar('}').CR();
				PPLoadString("tagtype", temp_buf);
				line_buf.Tab(2).CatCharN('\\', 2).Cat(temp_buf.Transf(CTRANSF_INNER_TO_OUTER)).CatDiv('=', 1).
					CatChar('\\').Cat("ppyrsrv").CatChar('{').Cat(data_type_symb).CatChar('}').CR();
				doc_file.WriteLine(line_buf);
			}
		}
		line_buf.Z().CatChar('\\').Cat("end").CatChar('{').Cat("description").CatChar('}').CR();
		doc_file.WriteLine(line_buf);
	}
	CATCHZOK
	return ok;
}

int PPObjTag::MakeReserved(long flags)
{
	// {ID, Name, Symb, ObjType, DataType}
	int    ok = 1;
	uint   num_recs = 0;
	SString temp_buf, obj_type_symb, data_type_symb;
	TVRez * p_rez = P_SlRez;
	THROW_PP(p_rez, PPERR_RESFAULT);
	THROW_PP(p_rez->findResource(ROD_TAG, PP_RCDATA), PPERR_RESFAULT);
	THROW_PP(num_recs = p_rez->getUINT(), PPERR_RESFAULT);
	for(uint i = 0; i < num_recs; i++) {
		PPObjectTag temp_rec;
		PPObjTagPacket pack;
		const PPID id = p_rez->getUINT();
		p_rez->getString(temp_buf.Z(), 2); // Name
		PPExpandString(temp_buf, CTRANSF_UTF8_TO_INNER);
		temp_buf.CopyTo(pack.Rec.Name, sizeof(pack.Rec.Name));
		p_rez->getString(temp_buf.Z(), 2); // Symb
		temp_buf.CopyTo(pack.Rec.Symb, sizeof(pack.Rec.Symb));
		pack.Rec.ID = id;
		p_rez->getString(obj_type_symb.Z(), 2);  // ObjType
		p_rez->getString(data_type_symb.Z(), 2); // DataType
		if(id && Search(id, &temp_rec) <= 0 && SearchBySymb(pack.Rec.Symb, 0, &temp_rec) <= 0) {
			long   extra = 0;
			pack.Rec.ObjTypeID = GetObjectTypeBySymb(obj_type_symb, &extra);
			if(pack.Rec.ObjTypeID) {
				if(data_type_symb.IsEqiAscii("STRING"))
					pack.Rec.TagDataType = OTTYP_STRING;
				else if(data_type_symb.IsEqiAscii("INT"))
					pack.Rec.TagDataType = OTTYP_INT;
				else if(data_type_symb.IsEqiAscii("NUMBER") || data_type_symb.IsEqiAscii("REAL"))
					pack.Rec.TagDataType = OTTYP_NUMBER;
				else if(data_type_symb.IsEqiAscii("DATE"))
					pack.Rec.TagDataType = OTTYP_DATE;
				else if(data_type_symb.IsEqiAscii("TIMESTAMP"))
					pack.Rec.TagDataType = OTTYP_TIMESTAMP;
				else if(data_type_symb.IsEqiAscii("GUID") || data_type_symb.IsEqiAscii("UUID"))
					pack.Rec.TagDataType = OTTYP_GUID;
				else if(data_type_symb.IsEqiAscii("ENUM"))
					pack.Rec.TagDataType = OTTYP_ENUM;
				// @v10.9.4 {
				else if(data_type_symb.HasPrefixIAscii("LINK:")) {
					// link:tag:goods
					PPID    link_obj_type = 0;
					PPID    link_obj_type_group = 0;
					StringSet ss(':', data_type_symb);
					for(uint ssp = 0, fldn = 0; ss.get(&ssp, temp_buf); fldn++) {
						assert(fldn > 0 || temp_buf.IsEqiAscii("LINK"));
						if(fldn == 1) {
							link_obj_type = GetObjectTypeBySymb(temp_buf, 0);
						}
						else if(fldn == 2) {
							if(link_obj_type == PPOBJ_TAG) {
								link_obj_type_group = GetObjectTypeBySymb(temp_buf, 0);
							}
						}
					}
					if(link_obj_type) {
						pack.Rec.TagDataType = OTTYP_OBJLINK;
						pack.Rec.TagEnumID = link_obj_type;
						pack.Rec.LinkObjGrp = link_obj_type_group;
					}
				}
				// } @v10.9.4 
				if(pack.Rec.TagDataType) {
					//
					// Здесь нельзя использовать PutPacket поскольку добавляется запись
					// с предопределенным идентификатором.
					//
					PPTransaction tra(1);
					THROW(tra);
					THROW(Helper_CreateEnumObject(pack));
					THROW(StoreItem(Obj, 0, &pack.Rec, 0));
					THROW(tra.Commit());
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

ObjTagFilt & PPObjTag::InitFilt(void * extraPtr, ObjTagFilt & rFilt) const
{
	if(extraPtr) {
		rFilt = *static_cast<const ObjTagFilt *>(extraPtr);
	}
	else {
		rFilt.ObjTypeID = PPOBJ_PERSON;
		rFilt.ParentID = 0;
		rFilt.Flags = 0;
	}
	return rFilt;
}

int FASTCALL PPObjTag::IsUnmirrored(PPID tagID)
{
	return BIN(oneof3(tagID, PPTAG_LOT_FSRARINFA, PPTAG_LOT_FSRARINFB, PPTAG_LOT_VETIS_UUID)); // @v10.2.5 PPTAG_LOT_VETIS_UUID
}

int PPObjTag::GetPacket(PPID id, PPObjTagPacket * pPack)
{
	int    ok = -1;
	assert(pPack);
	pPack->Z();
	if(PPCheckGetObjPacketID(Obj, id)) { // @v10.3.6
		ok = Search(id, &pPack->Rec);
	}
	return ok;
}

int PPObjTag::Helper_CreateEnumObject(PPObjTagPacket & rPack)
{
	int    ok = 1;
	if(rPack.Rec.TagDataType == OTTYP_ENUM) {
		/*
		pPack->EnumList.SetFlags((pPack->Rec.Flags & OTF_HIERENUM) ? PPCommObjEntry::fHierarchical : 0);
		THROW(pPack->EnumList.Write(0));
		pPack->Rec.TagEnumID = pPack->EnumList.GetEnumID();
		*/
		ReferenceTbl::Rec hdr_rec;
		const long hdr_flags = (rPack.Rec.Flags & OTF_HIERENUM) ? PPCommObjEntry::fHierarchical : 0;
		if(!rPack.Rec.TagEnumID || P_Ref->GetItem(PPOBJ_DYNAMICOBJS, rPack.Rec.TagEnumID, &hdr_rec) < 0) {
			THROW(P_Ref->AllocDynamicObj(&rPack.Rec.TagEnumID, 0, hdr_flags, 0));
		}
		else if((hdr_rec.Val1 & PPCommObjEntry::fHierarchical) != (hdr_flags & PPCommObjEntry::fHierarchical)) {
			SETFLAGBYSAMPLE(hdr_rec.Val1, PPCommObjEntry::fHierarchical, hdr_flags);
			THROW(P_Ref->UpdateItem(PPOBJ_DYNAMICOBJS, rPack.Rec.TagEnumID, &hdr_rec, 1, 0));
		}
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

int PPObjTag::PutPacket(PPID * pID, PPObjTagPacket * pPack, int use_ta)
{
	int    ok = 1;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(*pID && pPack == 0) {
			PPObjTagPacket rmvp;
			if(GetPacket(*pID, &rmvp) > 0) {
				if(rmvp.Rec.TagDataType == OTTYP_ENUM && rmvp.Rec.TagEnumID) {
					THROW(P_Ref->FreeDynamicObj(rmvp.Rec.TagEnumID, 0));
				}
				THROW(P_Ref->RemoveItem(PPOBJ_TAG, *pID, 0));
			}
		}
		else {
			THROW(Helper_CreateEnumObject(*pPack));
			THROW(StoreItem(PPOBJ_TAG, *pID, &pPack->Rec, 0));
			*pID = pPack->Rec.ID = P_Ref->data.ObjID;
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int PPObjTag::Edit(PPID * pID, void * extraPtr)
{
	class ObjTagDialog : public TDialog {
		DECL_DIALOG_DATA(PPObjTagPacket);
	public:
		explicit ObjTagDialog(uint rezID) : TDialog(rezID)
		{
		}
		DECL_DIALOG_SETDTS()
		{
			SString typ_name_buf;
			RVALUEPTR(Data, pData);
			enableCommand(cmaMore, Data.Rec.TagDataType == OTTYP_ENUM);
			disableCtrl(CTL_OBJTAG_ID, !PPMaster || Data.Rec.ID);
			ObjTagFilt ot_filt(Data.Rec.ObjTypeID, ObjTagFilt::fOnlyGroups);
			SetupPPObjCombo(this, CTLSEL_OBJTAG_GRP, PPOBJ_TAG, Data.Rec.TagGroupID, OLW_CANSELUPLEVEL, &ot_filt);
			setCtrlData(CTL_OBJTAG_NAME, Data.Rec.Name);
			setCtrlData(CTL_OBJTAG_ID,   &Data.Rec.ID);
			setCtrlData(CTL_OBJTAG_SYMB, Data.Rec.Symb);
			const PPID typ = Data.Rec.TagDataType;
			AddClusterAssoc(CTL_OBJTAG_FLAGS, 0, OTF_WARNZERO);
			AddClusterAssoc(CTL_OBJTAG_FLAGS, 1, OTF_INHERITABLE);
			AddClusterAssoc(CTL_OBJTAG_FLAGS, 2, OTF_NOTICEINCASHPANE);
			AddClusterAssoc(CTL_OBJTAG_FLAGS, 3, OTF_HIERENUM);
			SetClusterData(CTL_OBJTAG_FLAGS, Data.Rec.Flags);
			DisableClusterItem(CTL_OBJTAG_FLAGS, 3, (typ != OTTYP_ENUM));
			ObjTagItem::GetTypeString(typ, Data.Rec.TagEnumID, typ_name_buf);
			if(typ == OTTYP_OBJLINK) {
				int    dsbl = 1;
				if(Data.Rec.TagEnumID == PPOBJ_PERSON) {
					dsbl = 0;
					SetupPPObjCombo(this, CTLSEL_OBJTAG_OBJGRP, PPOBJ_PERSONKIND, Data.Rec.LinkObjGrp, OLW_CANINSERT, 0);
				}
				else if(Data.Rec.TagEnumID == PPOBJ_GLOBALUSERACC) { // @v10.5.5
					dsbl = 0;
					SetupStringCombo(this, CTLSEL_OBJTAG_OBJGRP, PPTXT_GLOBALSERVICELIST, Data.Rec.LinkObjGrp);
				}
				else if(Data.Rec.TagEnumID == PPOBJ_TAG) { // @v10.9.4
					dsbl = 0;
					SetupObjListCombo(this, CTLSEL_OBJTAG_OBJGRP, Data.Rec.LinkObjGrp, 0);
				}
				else
					setCtrlLong(CTLSEL_OBJTAG_OBJGRP, 0);
				disableCtrl(CTLSEL_OBJTAG_OBJGRP, dsbl);
			}
			setCtrlString(CTL_OBJTAG_TYPE, typ_name_buf);
			// @v11.2.8 {
			if(Data.Rec.HotKey) {
				SString buf;
				uint32 hk = 0;
				KeyDownCommand kc;
				kc.Code = LoWord(Data.Rec.HotKey);
				kc.State = HiWord(Data.Rec.HotKey);
				if(kc.GetKeyName(buf, 0/*onlySpecKeys*/) > 0) {
					kc.GetKeyName(buf);
				}
				setCtrlString(CTL_OBJTAG_HOTKEY, buf);
			}
			// } @v11.2.8 
			disableCtrl(CTL_OBJTAG_TYPE, 1);
			enableCommand(cmaMore, Data.Rec.TagDataType == OTTYP_ENUM);
			return 1;
		}
		DECL_DIALOG_GETDTS()
		{
			int    ok = 1;
			uint   selctl = 0;
			getCtrlData(selctl = CTL_OBJTAG_NAME, Data.Rec.Name);
			THROW_PP(*strip(Data.Rec.Name) != 0, PPERR_NAMENEEDED);
			getCtrlData(CTL_OBJTAG_ID, &Data.Rec.ID);
			getCtrlData(selctl = CTLSEL_OBJTAG_GRP, &Data.Rec.TagGroupID);
			getCtrlData(CTL_OBJTAG_SYMB, Data.Rec.Symb);
			GetClusterData(CTL_OBJTAG_FLAGS, &Data.Rec.Flags);
			if(Data.Rec.TagDataType != OTTYP_ENUM) {
				Data.Rec.Flags &= ~OTF_HIERENUM;
			}
			if(Data.Rec.TagDataType == OTTYP_OBJLINK) {
				if(Data.Rec.TagEnumID == PPOBJ_PERSON) {
					getCtrlData(CTLSEL_OBJTAG_OBJGRP, &Data.Rec.LinkObjGrp);
				}
			}
			THROW(CheckRecursion(Data.Rec.ID, Data.Rec.TagGroupID));
			ASSIGN_PTR(pData, Data);
			CATCH
				ok = PPErrorByDialog(this, selctl);
			ENDCATCH
			return ok;
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isCmd(cmaMore) && Data.Rec.TagDataType == OTTYP_ENUM) {
				if(Data.Rec.TagEnumID) {
					ShowObjects(Data.Rec.TagEnumID, 0);
				}
				clearEvent(event);
			}
			// @v11.2.8 {
			else if(event.isCmd(cmWinKeyDown)) {
				if(isCurrCtlID(CTL_OBJTAG_HOTKEY)) {
					SString buf;
					uint32 hk = 0;
					const KeyDownCommand * p_cmd = static_cast<const KeyDownCommand *>(event.message.infoPtr);
					if(p_cmd && p_cmd->GetKeyName(buf, 0/*onlySpecKeys*/) > 0) {
						p_cmd->GetKeyName(buf);
						hk = MakeLong(p_cmd->Code, p_cmd->State);
					}
					setCtrlString(CTL_OBJTAG_HOTKEY, buf);
					Data.Rec.HotKey = hk;
				}
			}
			// } @v11.2.8 
		}
		int CheckRecursion(PPID id, PPID grpID)
		{
			int    ok = 1;
			PPID   grp_id = grpID;
			PPObjTag tag_obj;
			do {
				if(grp_id != 0) {
					PPObjectTag tag_rec;
					THROW_PP(id != grp_id, PPERR_RECURSIONFOUND);
					THROW(tag_obj.Fetch(grp_id, &tag_rec));
					grp_id = tag_rec.TagGroupID;
				}
			} while(grp_id != 0);
			CATCHZOK
			return ok;
		}
	};
	int    ok = 1;
	int    r = cmCancel;
	int    valid_data = 0;
	int    is_new = 0;
	ObjTagDialog * dlg = 0;
	if(*pID && ObjTagFilt::ObjTypeRootIdentToObjType(*pID, 0)) {
		// Специальный идентификатор, поступивший из списка - тип объекта - его редактировать не надо.
		ok = -1;
	}
	else {
		// @v10.3.0 (never used) int    tagtype = 0;
		ObjTagFilt ot_filt;
		InitFilt(NZOR(extraPtr, ExtraPtr), ot_filt);
		PPObjTagPacket pack;
		THROW(EditPrereq(pID, 0, &is_new));
		if(!is_new) {
			THROW(GetPacket(*pID, &pack) > 0);
		}
		else {
			PPObjectTag parent_rec;
			if(ot_filt.ParentID && Fetch(ot_filt.ParentID, &parent_rec) > 0)
				pack.Rec.TagGroupID = ot_filt.ParentID;
			else
				pack.Rec.TagGroupID = 0;
			pack.Rec.ObjTypeID  = NZOR(ot_filt.ObjTypeID, PPOBJ_PERSON);
			r = (SelectObjTagType(&pack.Rec, &ot_filt) > 0) ? 1 : -1;
		}
		if(r > 0) {
			THROW(CheckDialogPtr(&(dlg = new ObjTagDialog(pack.Rec.TagDataType ? DLG_OBJTAG : DLG_OBJTAGGRP))));
			THROW(EditPrereq(pID, dlg, 0));
			dlg->setDTS(&pack);
			while(!valid_data && (r = ExecView(dlg)) == cmOK) {
				THROW(is_new || CheckRights(PPR_MOD));
				if(dlg->getDTS(&pack)) {
					valid_data = 1;
					if(*pID)
						*pID = pack.Rec.ID;
					THROW(PutPacket(pID, &pack, 1));
				}
			}
		}
	}
	CATCHZOKPPERR
	delete dlg;
	return ok ? r : 0;
}

#if 0 // @v10.7.8 (unused) {
/*static*/int PPObjTag::EditEnumListDialog(PPTagEnumList * pList)
{
	class TagEnumListDialog : public PPListDialog {
		DECL_DIALOG_DATA(PPTagEnumList);
	public:
		TagEnumListDialog(uint dlgID, uint listCtlID, size_t /*listBufLen*/) : PPListDialog(dlgID, listCtlID)
		{
		}
		DECL_DIALOG_SETDTS()
		{
			RVALUEPTR(Data, pData);
			updateList(-1);
			return 1;
		}
		DECL_DIALOG_GETDTS()
		{
			ASSIGN_PTR(pData, Data);
			return 1;
		}
	private:
		virtual int setupList()
		{
			for(uint i = 0; i < Data.getCount(); i++) {
				StrAssocArray::Item item = Data.Get(i);
				if(!addStringToList(item.Id, item.Txt))
					return 0;
			}
			return 1;
		}
		virtual int addItem(long * pPos, long * pID)
		{
			int    ok = -1;
			PPCommObjEntry param(Data.GetEnumID());
			if(PPObjReference::EditCommObjItem(&param) > 0) {
				if(!Data.PutItem(&param.ID, param.Name, param.ParentID))
					ok = PPSetErrorSLib();
				else {
					ASSIGN_PTR(pPos, Data.getCount());
					ASSIGN_PTR(pID, param.ID);
					ok = 1;
				}
			}
			return ok;
		}
		virtual int editItem(long, long id)
		{
			int    ok = -1;
			SString name;
			if(id && Data.GetText(id, name) > 0) {
				PPCommObjEntry param(Data.GetEnumID(), id, name, 0);
				if(PPObjReference::EditCommObjItem(&param) > 0) {
					ok = Data.PutItem(&param.ID, param.Name, param.ParentID);
				}
			}
			return ok;
		}
		virtual int delItem(long, long id)
		{
			return id ? Data.PutItem(&id, 0, 0) : 0;
		}
	};
	int    ok = -1;
	TagEnumListDialog * dlg = new TagEnumListDialog((pList->GetFlags() & PPCommObjEntry::fHierarchical) ? DLG_TAGENUMTREEVIEW : DLG_TAGENUMVIEW, CTL_TAGENUMVIEW_LIST, 48);
	if(CheckDialogPtrErr(&dlg)) {
		dlg->setDTS(pList);
		for(int valid_data = 0; !valid_data && ExecView(dlg) == cmOK;)
			if(dlg->getDTS(pList))
				valid_data = ok = 1;
	}
	else
		ok = 0;
	delete dlg;
	return ok;
}
#endif // } 0 @v10.7.8 (unused)

SArray * PPObjTag::CreateList(long current, long parent)
{
	int    grpOnly = BIN(parent < 0);
	ReferenceTbl::Key1 k;
	struct {
		PPID   id;
		char   text[sizeof(static_cast<const PPObjectTag *>(0)->Name) + 3];
	} item;
	PPObjectTag tag;
	long   lplus = 0x202b20L, lminus = 0x202d20L;
	BExtQuery q(P_Ref, 1);
	SArray * p_ary = new SArray(sizeof(item));
	THROW_MEM(p_ary);
	parent = (Search(current, &tag) > 0) ? tag.TagGroupID : labs(parent);
	if(parent == 1)
		parent = 0;
	MEMSZERO(k);
	k.ObjType = PPOBJ_TAG;
	q.selectAll().where(P_Ref->ObjType == PPOBJ_TAG && P_Ref->Val2 == parent);
	if(parent) {
		item.id = 0;
		STRNSCPY(item.text, reinterpret_cast<const char *>(&lminus));
		THROW_SL(p_ary->insert(&item));
	}
	for(q.initIteration(false, &k, spGe); q.nextIteration() > 0;) {
		const PPObjectTag * p_rec = reinterpret_cast<const PPObjectTag *>(&P_Ref->data);
		if(!grpOnly || p_rec->TagDataType == 0) {
			item.id = p_rec->ID;
			if(!p_rec->TagDataType)
				strcpy(stpcpy(item.text, reinterpret_cast<const char *>(&lplus)), p_rec->Name);
			else
				STRNSCPY(item.text, p_rec->Name);
			THROW_SL(p_ary->insert(&item));
		}
	}
	THROW_DB(BTROKORNFOUND);
	CATCH
		ZDELETE(p_ary);
	ENDCATCH
	return p_ary;
}

int PPObjTag::CheckForFilt(const ObjTagFilt * pFilt, const PPObjectTag & rRec) const
{
	int    ok = 1;
	if(pFilt) {
		if(pFilt->Flags & ObjTagFilt::fOnlyTags && rRec.TagDataType == OTTYP_GROUP)
			ok = 0;
		else if(pFilt->Flags & ObjTagFilt::fOnlyGroups && rRec.TagDataType != OTTYP_GROUP)
			ok = 0;
		else if(pFilt->ParentID && (rRec.TagGroupID != pFilt->ParentID && rRec.ID != pFilt->ParentID))
			ok = 0;
		else if(!(pFilt->Flags & ObjTagFilt::fAnyObjects)) {
			if(pFilt->ObjTypeID && rRec.ObjTypeID != pFilt->ObjTypeID)
				ok = 0;
			else if(pFilt->ObjTypeID == 0 && rRec.ObjTypeID != PPOBJ_PERSON)
				ok = 0;
		}
	}
	return ok;
}

int PPObjTag::GetObjListByFilt(PPID objType, const TagFilt * pFilt, UintHashTable & rList, UintHashTable & rExcludeList)
{
	int    ok = -1;
	int    intersect_list_inited = 0;
	UintHashTable intersect_list, exclude_list;
	rList.Clear();
	rExcludeList.Clear();
	if(pFilt && !pFilt->IsEmpty()) {
		SString restrict;
		uint   i;
		if(pFilt->Flags & TagFilt::fNotTagsInList) {
			for(i = 0; i < pFilt->TagsRestrict.getCount(); i++) {
				StrAssocArray::Item item = pFilt->TagsRestrict.Get(i);
				UintHashTable local_list;
				if(P_Ref->Ot.GetObjectList(objType, item.Id, local_list) > 0) {
					exclude_list.Add(local_list);
				}
			}
			ok = 100;
		}
		else {
			for(i = 0; i < pFilt->TagsRestrict.getCount(); i++) {
				StrAssocArray::Item item = pFilt->TagsRestrict.Get(i);
				TagFilt::GetRestriction(item.Txt, restrict);
				UintHashTable local_list;
				if(restrict.IsEqiAscii(_PPConst.P_TagValRestrict_Empty)) {
					if(P_Ref->Ot.GetObjectList(objType, item.Id, local_list) > 0) {
						for(ulong v = 0; local_list.Enum(&v);) {
							ObjTagItem tag_item;
							if(FetchTag((PPID)v, item.Id, &tag_item) > 0 && !tag_item.IsZeroVal())
								exclude_list.Add(local_list);
						}
					}
				}
				else {
					if(P_Ref->Ot.GetObjectList(objType, item.Id, local_list) > 0) {
						if(restrict.IsEqiAscii(_PPConst.P_TagValRestrict_Exist)) {
							if(intersect_list_inited)
								intersect_list.Intersect(local_list);
							else {
								intersect_list = local_list;
								intersect_list_inited = 1;
							}
						}
						else {
							for(ulong v = 0; local_list.Enum(&v);) {
								ObjTagItem tag_item;
								if(FetchTag((PPID)v, item.Id, &tag_item) > 0 && pFilt->CheckTagItemForRestrict(&tag_item, restrict) > 0)
									intersect_list.Add(v);
								else
									intersect_list.Remove(v);
								intersect_list_inited = 1;
							}
						}
					}
					else
						intersect_list.Clear();
				}
				ok = 1;
			}
		}
	}
	rList = intersect_list;
	rExcludeList = exclude_list;
	return ok;
}

StrAssocArray * PPObjTag::MakeStrAssocList(void * extraPtr)
{
	ObjTagFilt ot_filt;
	InitFilt(extraPtr, ot_filt);
	PPIDArray obj_type_list;
	SString temp_buf;
	PPObjectTag rec;
	StrAssocArray * p_list = new StrAssocArray;
	THROW_MEM(p_list);
	for(SEnum en = P_Ref->Enum(Obj, 0); en.Next(&rec) > 0;) {
		if(CheckForFilt(&ot_filt, rec)) {
			PPID   parent_id = rec.TagGroupID;
			if(ot_filt.Flags & ObjTagFilt::fObjTypeRoots && parent_id == 0) {
				parent_id = ObjTagFilt::MakeObjTypeRootIdent(rec.ObjTypeID);
				if(obj_type_list.addUnique(rec.ObjTypeID) > 0) {
					GetObjectTitle(rec.ObjTypeID, temp_buf.Z());
                    p_list->Add(parent_id, 0, temp_buf);
				}
			}
			THROW_SL(p_list->Add(rec.ID, parent_id, rec.Name));
		}
	}
	p_list->SortByText();
	CATCH
		ZDELETE(p_list);
	ENDCATCH
	return p_list;
}

int PPObjTag::GetListByFlag(long mask, PPIDArray & rList)
{
	int    ok  = -1;
	PPObjectTag rec;
	for(SEnum en = P_Ref->Enum(Obj, 0); en.Next(&rec) > 0;) {
		if((rec.Flags & mask) == mask) {
			rList.addUnique(rec.ID);
			ok = 1;
		}
	}
	return ok;
}

int PPObjTag::GetListByHotKey(uint32 keyCode, PPID objType, PPIDArray & rList) // @v11.2.8
{
	rList.Z();
	int    ok  = -1;
	if(keyCode) {
		PPObjectTag rec;
		for(SEnum en = P_Ref->Enum(Obj, 0); en.Next(&rec) > 0;) {
			if(rec.HotKey == keyCode && (!objType || objType == rec.ObjTypeID)) {
				rList.addUnique(rec.ID);
				ok = 1;
			}
		}
	}
	return ok;
}

int PPObjTag::NormalizeTextCriterion(PPID tagID, const char * pCrit, SString & rNormCrit)
{
	int    ok = 1;
	PPObjectTag tag_rec;
	rNormCrit = pCrit;
	if(rNormCrit.NotEmptyS()) {
		if(Fetch(tagID, &tag_rec) > 0) {
			switch(tag_rec.TagDataType) {
				case OTTYP_BOOL:
					if(rNormCrit.OneOf(';', "no;false;0", 1)) // @v11.1.11 "no;false;0;нет;ложь"-->"no;false;0"
						rNormCrit.Z().Cat("0");
					else
						rNormCrit.Z().Cat("1");
					break;
				case OTTYP_ENUM:
					{
						PPID   _id = 0;
						if(P_Ref->SearchName(tag_rec.TagEnumID, &_id, rNormCrit, 0) > 0) {
							rNormCrit.Z().Cat(_id);
						}
						else {
							rNormCrit.Z().Cat(-1234567890); // С высокой вероятностью невозможное значение
							ok = -1;
						}
					}
					break;
				case OTTYP_OBJLINK:
					ok = -1;
					break;
			}
		}
	}
	return ok;
}

int PPObjTag::GetWarnList(const ObjTagList * pTagList, StrAssocArray * pResultList, StrAssocArray * pInfoList)
{
	int    ok = -1;
	PPIDArray warn_tag_list;
	if(GetListByFlag(OTF_WARNZERO, warn_tag_list) > 0) {
		SString fmt_buf, msg_buf;
		for(uint i = 0; i < warn_tag_list.getCount(); i++) {
			int    invalid = 0;
			const  PPID tag_id = warn_tag_list.get(i);
			PPObjectTag tag_rec;
			// @v10.6.5 @ctr MEMSZERO(tag_rec);
			Fetch(tag_id, &tag_rec);
			if(pTagList == 0) {
				msg_buf.Printf(PPLoadTextS(PPTXT_TAGABSENT, fmt_buf), tag_rec.Name);
				invalid = 1;
			}
			else {
				const ObjTagItem * p_item = pTagList->GetItem(tag_id);
				if(!p_item) {
					msg_buf.Printf(PPLoadTextS(PPTXT_TAGABSENT, fmt_buf), tag_rec.Name);
					invalid = 1;
				}
				else if(p_item->IsWarnVal()) {
					msg_buf.Printf(PPLoadTextS(PPTXT_TAGWARNVAL, fmt_buf), tag_rec.Name);
					invalid = 1;
				}
				else if(pInfoList) {
					GetCurrTagVal(p_item, msg_buf);
					pInfoList->Add(tag_id, msg_buf);
				}
			}
			if(invalid) {
				CALLPTRMEMB(pResultList, Add(tag_id, msg_buf));
				ok = 1;
			}
		}
	}
	return ok;
}

int PPObjTag::GetCurrTagVal(const ObjTagItem * pItem, SString & rBuf)
{
	rBuf.Z();
	return pItem ? pItem->GetStr(rBuf) : -1;
}

int PPObjTag::Read(PPObjPack * p, PPID id, void * stream, ObjTransmContext * pCtx)
{
	int    ok = 1;
	p->Data = new PPObjectTag;
	THROW_MEM(p->Data);
	if(stream == 0) {
		p->Priority = 120;
		THROW(Search(id, p->Data) > 0);
	}
	else {
		THROW(Serialize_(-1, static_cast<ReferenceTbl::Rec *>(p->Data), stream, pCtx));
	}
	CATCHZOK
	return ok;
}

int PPObjTag::ProcessObjRefs(PPObjPack * p, PPObjIDArray * ary, int replace, ObjTransmContext * pCtx)
{
	int    ok = 1;
	if(p && p->Data) {
		PPObjectTag * p_rec = static_cast<PPObjectTag *>(p->Data);
		THROW(ProcessObjRefInArray(PPOBJ_TAG, &p_rec->TagGroupID, ary, replace));
		if(IS_DYN_OBJTYPE(p_rec->TagEnumID)) {
			THROW(ProcessObjRefInArray(PPOBJ_DYNAMICOBJS, &p_rec->TagEnumID, ary, replace));
		}
		else if(p_rec->TagEnumID == PPOBJ_PERSON) {
			THROW(ProcessObjRefInArray(PPOBJ_PERSONKIND, &p_rec->LinkObjGrp, ary, replace));
		}
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

/*static*/int PPObjTag::RecoverLostUnifiedLinks()
{
	int    ok = -1;
	Reference * p_ref = PPRef;
	PPLogger logger;
	StrAssocArray * p_tags_list = 0;
	SysJournal * p_sj = DS.GetTLA().P_SysJ;
	if(p_sj && p_ref) {
		SString msg_buf, fmt_buf, temp_buf;
		PPObjTag tag_obj;
		ObjTagFilt ot_filt(0, ObjTagFilt::fOnlyTags|ObjTagFilt::fAnyObjects);
		p_tags_list = tag_obj.MakeStrAssocList(&ot_filt);
		if(p_tags_list) {
			PPObjPerson psn_obj;
			uint count = p_tags_list->getCount();
			PPWaitStart();
			PPTransaction tra(1);
			THROW(tra);
			for(uint i = 0; i < count; i++) {
				PPObjectTag tag;
				const PPID tag_id = p_tags_list->Get(i).Id;
				if(tag_obj.Fetch(tag_id, &tag) > 0 && oneof2(tag.TagDataType, OTTYP_OBJLINK, OTTYP_ENUM) && tag.TagEnumID) {
					ObjTagTbl::Key1 k1;
					k1.TagID = tag_id;
					k1.ObjID = 0;
					if(p_ref->Ot.search(1, &k1, spGe)) {
						do {
							ObjTagTbl::Rec rec;
							p_ref->Ot.copyBufTo(&rec);
							if(rec.IntVal) {
								if(tag.TagDataType == OTTYP_ENUM) {
									ReferenceTbl::Rec ref_rec;
									int r = p_ref->GetItem(tag.TagEnumID, rec.IntVal, &ref_rec);
									THROW(r);
									if(r < 0) {
										// PPTXT_TAGHANGEDLINK         "Обнаружена висячая ссылка в теге '@tag' на объект {'@objtitle', @int}"
										PPFormatT(PPTXT_TAGHANGEDLINK, &msg_buf, tag_id, tag.TagEnumID, rec.IntVal);
										PPID   subst_id = 0;
										if(p_sj->GetLastObjUnifyEvent(tag.TagEnumID, rec.IntVal, &subst_id, 0) > 0) {
											THROW_DB(p_ref->Ot.rereadForUpdate(1, &k1));
											p_ref->Ot.data.IntVal = subst_id;
											THROW_DB(p_ref->Ot.updateRec());
											PPLoadString("corrected", temp_buf);
											msg_buf.CatDiv(':', 2).Cat(temp_buf);
										}
										else {
											PPLoadString("notcorrected", temp_buf);
											msg_buf.CatDiv(':', 2).Cat(temp_buf);
										}
										logger.Log(msg_buf);
									}
								}
								else if(tag.TagDataType == OTTYP_OBJLINK) {
									if(tag.TagEnumID == PPOBJ_PERSON) {
										PersonTbl::Rec psn_rec;
										int r = psn_obj.Search(rec.IntVal, &psn_rec);
										THROW(r);
										if(r < 0) {
											// PPTXT_TAGHANGEDLINK         "Обнаружена висячая ссылка в теге '@tag' на объект {'@objtitle', @int}"
											PPFormatT(PPTXT_TAGHANGEDLINK, &msg_buf, tag_id, tag.TagEnumID, rec.IntVal);
											PPID   subst_id = 0;
											if(p_sj->GetLastObjUnifyEvent(tag.TagEnumID, rec.IntVal, &subst_id, 0) > 0) {
												THROW_DB(p_ref->Ot.rereadForUpdate(1, &k1));
												p_ref->Ot.data.IntVal = subst_id;
												THROW_DB(p_ref->Ot.updateRec());
												PPLoadString("corrected", temp_buf);
												msg_buf.CatDiv(':', 2).Cat(temp_buf);
											}
											else {
												PPLoadString("notcorrected", temp_buf);
												msg_buf.CatDiv(':', 2).Cat(temp_buf);
											}
											logger.Log(msg_buf);
										}
									}
								}
							}
						} while(p_ref->Ot.search(1, &k1, spNext) && p_ref->Ot.data.TagID == tag_id);
					}
				}
			}
			THROW(tra.Commit());
			PPWaitStop();
		}
	}
	CATCH
		logger.LogLastError();
		ok = PPErrorZ();
	ENDCATCH
	ZDELETE(p_tags_list);
	return ok;
}

int PPObjTag::HandleMsg(int msg, PPID _obj, PPID _id, void * extraPtr)
{
	int    ok = DBRPL_OK;
	if(msg == DBMSG_OBJDELETE) {
		if(_obj == PPOBJ_DYNAMICOBJS) {
			ObjTagFilt ot_filt(0, ObjTagFilt::fOnlyTags|ObjTagFilt::fAnyObjects);
			StrAssocArray * p_tags_list = MakeStrAssocList(&ot_filt);
			if(p_tags_list) {
				uint count = p_tags_list->getCount();
				for(uint i = 0; ok == DBRPL_OK && i < count; i++) {
					PPObjectTag tag;
					if(Fetch(p_tags_list->Get(i).Id, &tag) > 0 && oneof2(_id, tag.ObjTypeID, tag.TagEnumID))
						ok = RetRefsExistsErr(Obj, _id);
				}
			}
			ZDELETE(p_tags_list);
		}
		else if(_obj == PPOBJ_PERSON) {
			ObjTagFilt ot_filt(0, ObjTagFilt::fOnlyTags|ObjTagFilt::fAnyObjects);
			StrAssocArray * p_tags_list = MakeStrAssocList(&ot_filt);
			if(p_tags_list) {
				uint count = p_tags_list->getCount();
				for(uint i = 0; ok == DBRPL_OK && i < count; i++) {
					const PPID tag_id = p_tags_list->Get(i).Id;
					PPObjectTag tag;
					if(Fetch(tag_id, &tag) > 0) {
						if(tag.TagEnumID == _obj) {
							ObjTagTbl::Key2 k2;
							k2.TagID = tag_id;
							k2.IntVal = _id;
							if(P_Ref->Ot.search(2, &k2, spEq)) {
								ok = RetRefsExistsErr(Obj, tag_id);
							}
						}
					}
				}
			}
			ZDELETE(p_tags_list);
		}
	}
	else if(msg == DBMSG_OBJREPLACE) {
		if(_obj == PPOBJ_DYNAMICOBJS) {
			ObjTagFilt ot_filt(0, ObjTagFilt::fOnlyTags|ObjTagFilt::fAnyObjects);
			StrAssocArray * p_tags_list = MakeStrAssocList(&ot_filt);
			if(p_tags_list) {
				uint count = p_tags_list->getCount();
				for(uint i = 0; ok == DBRPL_OK && i < count; i++) {
					int update = 0;
					PPObjectTag tag;
					if(Fetch(p_tags_list->Get(i).Id, &tag) > 0) {
						if(_id == tag.ObjTypeID) {
							tag.ObjTypeID = reinterpret_cast<long>(extraPtr);
							update = 1;
						}
						else if(_id == tag.TagEnumID) {
							tag.TagEnumID = reinterpret_cast<long>(extraPtr);
							update = 1;
						}
					}
					if(update) {
						PPObjTagPacket pack;
						if(GetPacket(tag.ID, &pack) > 0) {
							pack.Rec.TagEnumID = tag.TagEnumID;
							pack.Rec.ObjTypeID = tag.ObjTypeID;
							if(!PutPacket(&tag.ID, &pack, 0))
								ok = DBRPL_ERROR;
						}
					}
				}
			}
			ZDELETE(p_tags_list);
			if(ok && !BroadcastObjMessage(DBMSG_OBJREPLACE, Obj, _id, extraPtr))
				ok = DBRPL_ERROR;
		}
		else if(_obj == PPOBJ_PERSON) {
			ObjTagFilt ot_filt(0, ObjTagFilt::fOnlyTags|ObjTagFilt::fAnyObjects);
			StrAssocArray * p_tags_list = MakeStrAssocList(&ot_filt);
			if(p_tags_list) {
				uint count = p_tags_list->getCount();
				for(uint i = 0; ok == DBRPL_OK && i < count; i++) {
					const PPID tag_id = p_tags_list->Get(i).Id;
					PPObjectTag tag;
					if(Fetch(tag_id, &tag) > 0) {
						if(tag.TagEnumID == _obj) {
							ObjTagTbl::Key2 k2;
							k2.TagID = tag_id;
							k2.IntVal = _id;
							while(P_Ref->Ot.search(2, &k2, spEq)) {
								if(!P_Ref->Ot.rereadForUpdate(2, &k2)) {
									ok = PPSetErrorDB();
								}
								else {
									P_Ref->Ot.data.IntVal = reinterpret_cast<long>(extraPtr);
									if(!P_Ref->Ot.updateRec()) {
										ok = PPSetErrorDB();
									}
								}
								k2.TagID = tag_id;
								k2.IntVal = _id;
							}
						}
					}
				}
			}
			ZDELETE(p_tags_list);
		}
	}
	return ok;
}
//
//
//
class TagFiltDialog : public PPListDialog {
public:
	explicit TagFiltDialog(PPID objType) : PPListDialog(DLG_TAGFLT, CTL_TAGFLT_LIST), ObjType(objType)
	{
		SString temp_buf;
		GetObjectTitle(ObjType, temp_buf);
		setCtrlString(CTL_TAGFLT_INFO, temp_buf);
	}
	int    setDTS(const TagFilt * pData);
	int    getDTS(TagFilt * pData);
private:
	DECL_HANDLE_EVENT
	{
		PPListDialog::handleEvent(event);
		if(event.isCmd(cmDeleteAll)) {
			Data.TagsRestrict.Z();
			updateList(-1);
			clearEvent(event);
		}
	}
	virtual int addItem(long * pPos, long *)
	{
		int    ok = -1;
		long   pos = -1;
		if((ok = EditItem(&pos)) > 0)
			ASSIGN_PTR(pPos, pos);
		return ok;
	}
	virtual int delItem(long pos, long)
	{
		return Data.TagsRestrict.AtFree(static_cast<uint>(pos)) ? 1 : -1;
	}
	virtual int editItem(long pos, long)
	{
		return (pos >= 0 && (uint)pos < Data.TagsRestrict.getCount()) ? EditItem(&pos) : -1;
	}
	virtual int moveItem(long pos, long id, int up)
	{
		if(up && pos > 0) {
			Data.TagsRestrict.Swap(pos, pos-1);
			return 1;
		}
		else if(!up && pos < (long)(Data.TagsRestrict.getCount()-1)) {
			Data.TagsRestrict.Swap(pos, pos+1);
			return 1;
		}
		else
			return -1;
	}
	virtual int setupList();
	int    EditItem(long * pPos);

	PPID     ObjType;
	TagFilt  Data;
	PPObjTag ObjTag;
};

struct SelTagDialogData {
	explicit SelTagDialogData(StrAssocArray::Item * pItem = 0) : Id(0)
	{
		if(pItem) {
			Id = pItem->Id;
			Txt = pItem->Txt;
		}
	}
	long   Id;
	SString Txt;
};

class SelTagDialog : public TDialog {
	DECL_DIALOG_DATA(SelTagDialogData);
	enum {
		ctlgroupColor = 1
	};
public:
	SelTagDialog(int checkRestrict, PPID objType) : TDialog(DLG_SELTAG), EnumID(0), CheckRestrict(checkRestrict), ObjType(objType)
	{
		{
			SString temp_buf;
			GetObjectTitle(ObjType, temp_buf);
			setCtrlString(CTL_SELTAG_INFO, temp_buf);
		}
		addGroup(ctlgroupColor, new ColorCtrlGroup(CTL_SELTAG_COLOR, CTLSEL_SELTAG_COLOR, cmSelColor, CTL_SELTAG_SELCOLOR));
	}
	DECL_DIALOG_SETDTS()
	{
		SString restrict_buf;
		SColor color;
		ObjTagFilt tag_flt(ObjType);
		PPObjectTag tag;
		RVALUEPTR(Data, pData);
		TagFilt::GetRestriction(Data.Txt, restrict_buf);
		TagFilt::GetColor(Data.Txt, color);
		SetupPPObjCombo(this, CTLSEL_SELTAG_TAG, PPOBJ_TAG, Data.Id, 0, &tag_flt);
		GetTagRec(Data.Id, &tag);
		int    option = 0;
		if(Data.Txt.IsEqiAscii(_PPConst.P_TagValRestrict_Empty))
			option = 1;
		else if(Data.Txt.IsEqiAscii(_PPConst.P_TagValRestrict_Exist))
			option = 2;
		else
			option = 0;
		AddClusterAssocDef(CTL_SELTAG_OPTION, 0, 0);
		AddClusterAssoc(CTL_SELTAG_OPTION, 1, 1);
		AddClusterAssoc(CTL_SELTAG_OPTION, 2, 2);
		SetClusterData(CTL_SELTAG_OPTION, option);
		SetupRestrict(tag.TagDataType, restrict_buf, tag.TagEnumID);
		SetupTag(Data.Id);
		{
			ColorCtrlGroup::Rec color_rec;
			color_rec.SetupStdColorList();
			color_rec.C = NZOR((COLORREF)color, GetColorRef(SClrCoral));
			setGroupData(ctlgroupColor, &color_rec);
		}
		return 1;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		uint   sel = 0;
		SString restrict_buf;
		SColor color;
		PPObjectTag tag;
		const  int  option = GetClusterData(CTL_SELTAG_OPTION);
		getCtrlData(sel = CTLSEL_SELTAG_TAG, &Data.Id);
		THROW(GetTagRec(Data.Id, &tag));
		THROW_PP(tag.TagDataType != 0, PPERR_MUSTBETAG);
		if(option == 1)
			restrict_buf = _PPConst.P_TagValRestrict_Empty;
		else if(option == 2)
			restrict_buf = _PPConst.P_TagValRestrict_Exist;
		else {
			getCtrlString(CTL_SELTAG_RESTRICT, restrict_buf);
			if(oneof2(tag.TagDataType, OTTYP_ENUM, OTTYP_OBJLINK)) {
				if(EnumID) {
					restrict_buf.Z().Cat(EnumID);
				}
				PPIDArray id_list;
				TagFilt::GetRestrictionIdList(restrict_buf, &id_list);
				if(CheckRestrict) {
					THROW_PP(id_list.getCount(), PPERR_USERINPUT);
				}
				//restrict_buf.Z().Cat(EnumID);
			}
			else {
				if(CheckRestrict) {
					if(tag.TagDataType == OTTYP_BOOL) {
						THROW_PP(oneof2(restrict_buf.C(0), '0', '1'), PPERR_USERINPUT);
					}
					else if(tag.TagDataType == OTTYP_STRING) {
						THROW_PP(restrict_buf.Len(), PPERR_USERINPUT);
					}
					else if(tag.TagDataType == OTTYP_DATE) {
						DateRange period;
						THROW(GetPeriodInput(this, CTL_SELTAG_RESTRICT, &period, strtoprdfEnableAnySign));
						getCtrlString(CTL_SELTAG_RESTRICT, restrict_buf);
					}
				}
			}
		}
		{
			ColorCtrlGroup::Rec color_rec;
			getGroupData(ctlgroupColor, &color_rec);
			color = SColor((COLORREF)color_rec.C);
		}
		TagFilt::SetRestriction(restrict_buf, Data.Txt);
		TagFilt::SetColor(&color, Data.Txt);
		ASSIGN_PTR(pData, Data);
		CATCHZOKPPERRBYDLG
		return ok;
	}
private:
	DECL_HANDLE_EVENT
	{
		TDialog::handleEvent(event);
		if(event.isCbSelected(CTLSEL_SELTAG_TAG)) {
			SString restrict_buf;
			getCtrlData(CTLSEL_SELTAG_TAG, &Data.Id);
			setCtrlString(CTL_SELTAG_RESTRICT, restrict_buf);
			EnumID = 0;
			SetupTag(Data.Id);
		}
		else if(event.isCmd(cmSelEnum)) {
			PPObjectTag tag;
			SString restrict_buf;
			getCtrlData(CTLSEL_SELTAG_TAG, &Data.Id);
			getCtrlString(CTL_SELTAG_RESTRICT, restrict_buf);
			GetTagRec(Data.Id, &tag);
			void * extra_ptr = reinterpret_cast<void *>((tag.TagDataType == OTTYP_OBJLINK) ? tag.LinkObjGrp : 0);
			{
				PPIDArray id_list;
				TagFilt::GetRestrictionIdList(restrict_buf, &id_list);
				ListToListData ltld(tag.TagEnumID, extra_ptr, &id_list);
				if(tag.Flags & OTF_HIERENUM)
					ltld.Flags |= ListToListData::fIsTreeList;
				ltld.TitleStrID = 0; // PPTXT_XXX;
				if(ListToListDialog(&ltld) > 0) {
					TagFilt::SetRestrictionIdList(restrict_buf, id_list);
					SetupRestrict(tag.TagDataType, restrict_buf, tag.TagEnumID);
				}
			}
		}
		else if(event.isClusterClk(CTL_SELTAG_OPTION)) {
			const  int  option = GetClusterData(CTL_SELTAG_OPTION);
			PPObjectTag tag;
			SString restrict_buf;
			GetTagRec(Data.Id, &tag);
			getCtrlData(CTLSEL_SELTAG_TAG, &Data.Id);
			getCtrlString(CTL_SELTAG_RESTRICT, restrict_buf);
			if(option == 1)
				restrict_buf = _PPConst.P_TagValRestrict_Empty;
			else if(option == 2)
				restrict_buf = _PPConst.P_TagValRestrict_Exist;
			else if(restrict_buf.IsEqiAscii(_PPConst.P_TagValRestrict_Empty) || restrict_buf.IsEqiAscii(_PPConst.P_TagValRestrict_Exist))
				restrict_buf = 0;
			SetupRestrict(tag.TagDataType, restrict_buf, tag.TagEnumID);
		}
		else
			return;
		clearEvent(event);
	}
	int    GetTagRec(PPID tagID, PPObjectTag * pRec)
	{
		int    ok = -1;
		if(tagID) {
			THROW(ObjTag.Fetch(tagID, pRec));
			ok = 1;
		}
		CATCHZOK
		return ok;
	}
	int    SetupTag(PPID tagID)
	{
		if(tagID) {
			PPObjectTag tag;
			TStaticText * p_text = static_cast<TStaticText *>(getCtrlView(CTL_SELTAG_TAGTYPE));
			GetTagRec(tagID, &tag);
			if(p_text) {
				SString type_str, msg;
				ObjTagItem::GetTypeString(tag.TagDataType, 0, type_str);
				if(tag.TagDataType == OTTYP_BOOL)
					msg.Z().CatChar('[').Cat(type_str).CatDiv('-', 1).CatChar('0').Comma().CatChar('1').CatChar(']');
				else if(oneof2(tag.TagDataType, OTTYP_NUMBER, OTTYP_DATE))
					msg.Z().CatChar('[').Cat(type_str).Space().CatCharN('.', 2).Space().Cat(type_str).CatChar(']');
				else if(oneof3(tag.TagDataType, OTTYP_STRING, OTTYP_ENUM, OTTYP_OBJLINK))
					msg.Z().CatBrackStr(type_str);
				p_text->setText(msg);
			}
			enableCommand(cmSelEnum, oneof2(tag.TagDataType, OTTYP_ENUM, OTTYP_OBJLINK) && CheckRestrict);
			disableCtrl(CTL_SELTAG_RESTRICT, oneof2(tag.TagDataType, OTTYP_ENUM, OTTYP_OBJLINK) || GetClusterData(CTL_SELTAG_OPTION) != 0);
		}
		else
			enableCommand(cmSelEnum, 0);
		return 1;
	}
	int    SetupRestrict(int tagType, const char * pTxt, long extra)
	{
		SString restrict(pTxt);
		int    option = 0;
		if(restrict.IsEqiAscii(_PPConst.P_TagValRestrict_Empty))
			option = 1;
		else if(restrict.IsEqiAscii(_PPConst.P_TagValRestrict_Exist))
			option = 2;
		else
			option = 0;
		SetClusterData(CTL_SELTAG_OPTION, option);
		disableCtrl(CTL_SELTAG_RESTRICT, oneof2(tagType, OTTYP_ENUM, OTTYP_OBJLINK) || option != 0);
		if(option == 0 && oneof2(tagType, OTTYP_ENUM, OTTYP_OBJLINK)) {
			PPIDArray id_list;
			int r = TagFilt::GetRestrictionIdList(restrict, &id_list);
			if(r == 1) {
				assert(id_list.getCount() == 1);
				EnumID = id_list.get(0);
				GetObjectName(extra, EnumID, restrict);
			}
			else
				EnumID = 0;
		}
		setCtrlString(CTL_SELTAG_RESTRICT, restrict);
		return 1;
	}
	const  int  CheckRestrict;
	const  PPID ObjType;
	PPID   EnumID;
	PPObjTag ObjTag;
};

int TagFiltDialog::EditItem(long * pPos)
{
	int    ok = -1;
	const  int  is_new = (*pPos >= 0) ? 0 : 1;
	uint   pos = is_new ? -1 : static_cast<uint>(*pPos);
	SelTagDialogData item(is_new ? 0 : &Data.TagsRestrict.Get(pos));
	SelTagDialog * p_dlg = 0;
	GetClusterData(CTL_TAGFLT_FLAGS, &Data.Flags);
	THROW(CheckDialogPtr(&(p_dlg = new SelTagDialog((Data.Flags & TagFilt::fNotTagsInList) ? 0 : 1, ObjType))));
	p_dlg->setDTS(&item);
	while(ok <= 0 && ExecView(p_dlg) == cmOK) {
		if(p_dlg->getDTS(&item) > 0) {
			int    replace_dup_factor = 1;
			if(Data.Flags & Data.fColors) {
				replace_dup_factor = -1;
				// @v10.4.4 {
				if(!is_new)
					THROW_SL(Data.TagsRestrict.AtFree(*pPos));
				// } @v10.4.4 
			}
			else {
				uint   fp = 0;
				if(!is_new || Data.TagsRestrict.Search(item.Id, &fp) > 0)
					THROW_SL(Data.TagsRestrict.AtFree(fp));
			}
			THROW_SL(Data.TagsRestrict.Add(item.Id, item.Txt, replace_dup_factor));
			Data.TagsRestrict.Search(item.Id, &pos);
			Data.TagsRestrict.SortByID();
			ok = 1;
		}
	}
	CATCHZOKPPERR
	ASSIGN_PTR(pPos, pos);
	delete p_dlg;
	return ok;
}

int TagFiltDialog::setupList()
{
	int    ok = 1;
	SString restrict;
	PPIDArray id_list;
	StringSet ss(SLBColumnDelim);
	for(uint i = 0; i < Data.TagsRestrict.getCount(); i++) {
		StrAssocArray::Item item = Data.TagsRestrict.Get(i);
		TagFilt::GetRestriction(item.Txt, restrict);
		PPObjectTag tag;
		THROW(ObjTag.Fetch(item.Id, &tag));
		ss.clear();
		ss.add(tag.Name);
		if(oneof2(tag.TagDataType, OTTYP_ENUM, OTTYP_OBJLINK)) {
			int r = TagFilt::GetRestrictionIdList(restrict, &id_list);
			if(r == 1) {
				assert(id_list.getCount() == 1);
				GetObjectName(tag.TagEnumID, id_list.get(0), restrict.Z());
			}
		}
		ss.add(restrict);
		THROW(addStringToList(i - 1, ss.getBuf()));
	}
	CATCHZOK
	return ok;
}

int TagFiltDialog::setDTS(const TagFilt * pData)
{
	RVALUEPTR(Data, pData);
	updateList(-1);
	AddClusterAssoc(CTL_TAGFLT_FLAGS, 0, TagFilt::fNotTagsInList);
	SetClusterData(CTL_TAGFLT_FLAGS, Data.Flags);
	return 1;
}

int TagFiltDialog::getDTS(TagFilt * pData)
{
	GetClusterData(CTL_TAGFLT_FLAGS, &Data.Flags);
	ASSIGN_PTR(pData, Data);
	return 1;
}

int FASTCALL EditTagFilt(PPID objType, TagFilt * pData)
{
	int    ok = 1, valid_data = 0;
	TagFiltDialog * p_dlg = 0;
	THROW_MEM(p_dlg = new TagFiltDialog(objType));
	THROW(CheckDialogPtr(&p_dlg));
	p_dlg->setDTS(pData);
	while(!valid_data && ExecView(p_dlg) == cmOK) {
		p_dlg->getDTS(pData);
		ok = valid_data = 1;
	}
	CATCHZOK
	delete p_dlg;
	return ok;
}

/*static*/int PPObjTag::CheckForTagFilt(PPID objType, PPID objID, const TagFilt * pFilt)
{
	int    ok = 1;
	if(pFilt && !pFilt->IsEmpty()) {
		ObjTagList tags_list;
		THROW(PPRef->Ot.GetList(objType, objID, &tags_list));
		ok = pFilt->Check(&tags_list);
	}
	CATCHZOK
	return ok;
}
//
//
//
int SelectObjTag(PPID * pTagID, const PPIDArray * pAllowedTags, ObjTagFilt * pFilt)
{
	return PPSelectObject(PPOBJ_TAG, pTagID, PPTXT_SELECTOBJTAG, pFilt);
}

union TagDlgVal {
	ushort b;
	double n;
	char   s[sizeof(static_cast<const ObjTagTbl::Rec *>(0)->StrVal)];
	long   l;
	LDATETIME dtm;
	//LDATE  dt;
};

#define GRP_IMG 1

TagDlgParam::TagDlgParam() : ObjType(0), ObjID(0), BoolDlgID(0), NmbDlgID(0), LnkDlgID(0), StrDlgID(0), DateDlgID(0),
	TimestampDlgID(0), GuidDlgID(0), ImgDlgID(0), ObjNameCtl(0), TagNameCtl(0), ValBoolCtl(0), ValNmbCtl(0),
	ValLnkCtl(0), ValStrCtl(0), ValDateCtl(0), ValTimeCtl(0), ValGuidCtl(0), ValImgCtl(0)
{
}

int TagDlgParam::GetDlgID(long tagDataType, uint * pDlgID) const
{
	uint   rez_id = 0;
	switch(tagDataType) {
		case OTTYP_BOOL:      rez_id = BoolDlgID; break;
		case OTTYP_STRING:    rez_id = StrDlgID;  break;
		case OTTYP_NUMBER:    rez_id = NmbDlgID;  break;
		case OTTYP_ENUM:
		case OTTYP_OBJLINK:   rez_id = LnkDlgID;  break;
		case OTTYP_DATE:      rez_id = DateDlgID; break;
		case OTTYP_TIMESTAMP: rez_id = TimestampDlgID; break;
		case OTTYP_GUID:      rez_id = GuidDlgID; break;
		case OTTYP_IMAGE:     rez_id = ImgDlgID;  break;
	}
	ASSIGN_PTR(pDlgID, rez_id);
	return BIN(rez_id);
}

int TagDlgParam::SetDlgData(TDialog * dlg, const ObjTagItem * pItem)
{
	int    ok = 1;
	TagDlgVal val;
	PPObjTag  tag_obj;
	PPObjectTag tag;
	SString temp_buf;
	if(ObjType && ObjNameCtl) {
		if(ObjID)
			GetObjectName(ObjType, ObjID, temp_buf);
		else
			GetObjectTitle(ObjType, temp_buf);
		dlg->setCtrlString(ObjNameCtl, temp_buf);
	}
	THROW(tag_obj.Fetch(pItem->TagID, &tag) > 0);
	temp_buf = tag.Name;
	if(TagNameCtl)
		dlg->setCtrlString(TagNameCtl, temp_buf);
	switch(pItem->TagDataType) {
		case OTTYP_BOOL:
			dlg->setCtrlData(ValBoolCtl, &(val.b = BIN(pItem->Val.IntVal)));
			dlg->selectCtrl(ValBoolCtl);
			break;
		case OTTYP_STRING:
			PTR32(val.s)[0] = 0;
			dlg->setCtrlData(ValStrCtl, STRNSCPY(val.s, pItem->Val.PStr));
			dlg->selectCtrl(ValStrCtl);
			break;
		case OTTYP_NUMBER:
			dlg->setCtrlData(ValNmbCtl, &(val.n = pItem->Val.RealVal));
			dlg->selectCtrl(ValNmbCtl);
			break;
		case OTTYP_OBJLINK:
		case OTTYP_ENUM:
			// @v10.9.4 {
			if(tag.TagEnumID == PPOBJ_TAG) {
				LinkTagFilt.Z();
				LinkTagFilt.ObjTypeID = tag.LinkObjGrp;
				SetupPPObjCombo(dlg, ValLnkCtl, tag.TagEnumID, pItem->Val.IntVal, OLW_CANINSERT, &LinkTagFilt);
			} 
			else /* } @v10.9.4 */ {
				SetupPPObjCombo(dlg, ValLnkCtl, tag.TagEnumID, pItem->Val.IntVal, OLW_CANINSERT, reinterpret_cast<void *>(tag.LinkObjGrp));
			}
			break;
		case OTTYP_DATE:
			dlg->SetupCalDate(CTLCAL_TAGV_DATE, ValDateCtl);
			dlg->setCtrlData(ValDateCtl, &(val.dtm.d = pItem->Val.DtVal));
			dlg->selectCtrl(ValDateCtl);
			break;
		case OTTYP_TIMESTAMP:
			val.dtm = pItem->Val.DtmVal;
			dlg->SetupCalDate(CTLCAL_TAGV_DATE, ValDateCtl);
			dlg->setCtrlData(ValDateCtl, &val.dtm.d);
			SetupTimePicker(dlg, CTL_TAGV_TIME, CTLTM_TAGV_TIME);
			dlg->setCtrlData(ValTimeCtl, &val.dtm.t);
			dlg->selectCtrl(ValDateCtl);
			break;
		case OTTYP_GUID:
			PTR32(val.s)[0] = 0;
			dlg->setCtrlData(ValGuidCtl, STRNSCPY(val.s, pItem->Val.PStr));
			dlg->selectCtrl(ValGuidCtl);
			break;
		case OTTYP_IMAGE:
			PTR32(val.s)[0];
			const SString path(pItem->Val.PStr);
			ImageBrowseCtrlGroup::Rec grp_rec(&path);
			if(dlg->getGroup(GRP_IMG) == 0)
				dlg->addGroup(GRP_IMG, new ImageBrowseCtrlGroup(/*PPTXT_FILPAT_PICT,*/ValImgCtl, cmAddImage, cmDelImage));
			dlg->setGroupData(GRP_IMG, &grp_rec);
			break;
	}
	CATCHZOK
	return ok;
}

int TagDlgParam::GetDlgData(TDialog * dlg, ObjTagItem * pItem)
{
	int    ok = 1;
	long   typ = pItem->TagDataType;
	TagDlgVal val;
	if(typ == OTTYP_BOOL) {
		dlg->getCtrlData(ValBoolCtl, &val.b);
		pItem->Val.IntVal = BIN(val.b);
	}
	else if(typ == OTTYP_STRING) {
		SString temp_buf, mark_buf;
		dlg->getCtrlString(ValStrCtl, temp_buf);
		temp_buf.Strip();
		if(pItem->TagID == PPTAG_LOT_FSRARLOTGOODSCODE) {
			if(PrcssrAlcReport::IsEgaisMark(temp_buf, &mark_buf)) {
				PrcssrAlcReport::EgaisMarkBlock emb;
				if(PrcssrAlcReport::ParseEgaisMark(mark_buf, emb))
					temp_buf = emb.EgaisCode;
			}
			{
				STokenRecognizer tr;
				SNaturalTokenArray nta;
				tr.Run(temp_buf.ucptr(), -1, nta, 0);
				THROW_PP_S(nta.Has(SNTOK_EGAISWARECODE) > 0.0f, PPERR_INVEGAISWARECODE, temp_buf);
			}
		}
		STRNSCPY(val.s, temp_buf);
		ZDELETE(pItem->Val.PStr);
		if(val.s[0])
			pItem->Val.PStr = newStr(val.s);
	}
	else if(typ == OTTYP_NUMBER) {
		dlg->getCtrlData(ValNmbCtl, &val.n);
		pItem->Val.RealVal = val.n;
	}
	else if(oneof2(typ, OTTYP_OBJLINK, OTTYP_ENUM)) {
		dlg->getCtrlData(ValLnkCtl, &val.l);
		pItem->Val.IntVal = val.l;
	}
	else if(typ == OTTYP_DATE) {
		dlg->getCtrlData(ValDateCtl, &val.dtm.d);
		THROW_SL(checkdate(val.dtm.d));
		pItem->Val.DtVal = val.dtm.d;
	}
	else if(typ == OTTYP_TIMESTAMP) {
		dlg->getCtrlData(ValDateCtl, &val.dtm.d);
		dlg->getCtrlData(ValTimeCtl, &val.dtm.t);
		pItem->Val.DtmVal = val.dtm;
	}
	else if(typ == OTTYP_GUID) {
		dlg->getCtrlData(ValGuidCtl, val.s);
		strip(val.s);
		ZDELETE(pItem->Val.PStr);
		if(val.s[0])
			pItem->Val.PStr = newStr(val.s);
	}
	else if(typ == OTTYP_IMAGE) {
		ImageBrowseCtrlGroup::Rec grp_rec;
		dlg->getGroupData(GRP_IMG, &grp_rec);
		grp_rec.Path.CopyTo(val.s, sizeof(val.s));
		strip(val.s);
		ZDELETE(pItem->Val.PStr);
		if(val.s[0])
			pItem->Val.PStr = newStr(val.s);
	}
	CATCHZOK
	return ok;
}

static int EditPosRights(ObjTagItem * pItem)
{
	int    ok = -1;
	TDialog * dlg = 0;
	SString rt_buf;
	pItem->GetStr(rt_buf);
	if(CheckDialogPtrErr(&(dlg = new TDialog(DLG_RTPOSAGENT)))) {
		long   rt = 0;
		long   ort = 0;
		long   v = 0;
		PPObjCSession::StringToRights(rt_buf, &rt, &ort);
		dlg->AddClusterAssoc(CTL_RTPOSAGENT_FLAGS,  0, 0x0001); SETFLAG(v, 0x0001, ort & CSESSOPRT_RETCHECK);
		dlg->AddClusterAssoc(CTL_RTPOSAGENT_FLAGS,  1, 0x0002); SETFLAG(v, 0x0002, ort & CSESSOPRT_ESCCLINE);
		dlg->AddClusterAssoc(CTL_RTPOSAGENT_FLAGS,  2, 0x0004); SETFLAG(v, 0x0004, ort & CSESSOPRT_BANKING);
		dlg->AddClusterAssoc(CTL_RTPOSAGENT_FLAGS,  3, 0x0008); SETFLAG(v, 0x0008, rt & CSESSRT_CLOSE);
		dlg->AddClusterAssoc(CTL_RTPOSAGENT_FLAGS,  4, 0x0010); SETFLAG(v, 0x0010, rt & CSESSRT_ESCCHECK);
		dlg->AddClusterAssoc(CTL_RTPOSAGENT_FLAGS,  5, 0x0020); SETFLAG(v, 0x0020, ort & CSESSOPRT_COPYCHECK);
		dlg->AddClusterAssoc(CTL_RTPOSAGENT_FLAGS,  6, 0x0040); SETFLAG(v, 0x0040, ort & CSESSOPRT_COPYZREPT);
		dlg->AddClusterAssoc(CTL_RTPOSAGENT_FLAGS,  7, 0x0080); SETFLAG(v, 0x0080, ort & CSESSOPRT_ROWDISCOUNT);
		dlg->AddClusterAssoc(CTL_RTPOSAGENT_FLAGS,  8, 0x0100); SETFLAG(v, 0x0100, ort & CSESSOPRT_XREP);
		dlg->AddClusterAssoc(CTL_RTPOSAGENT_FLAGS,  9, 0x0200); SETFLAG(v, 0x0200, ort & CSESSOPRT_SPLITCHK);
		dlg->AddClusterAssoc(CTL_RTPOSAGENT_FLAGS, 10, 0x0400); SETFLAG(v, 0x0400, ort & CSESSOPRT_MERGECHK);
		dlg->AddClusterAssoc(CTL_RTPOSAGENT_FLAGS, 11, 0x0800); SETFLAG(v, 0x0800, ort & CSESSOPRT_CHGPRINTEDCHK);
		dlg->AddClusterAssoc(CTL_RTPOSAGENT_FLAGS, 12, 0x1000); SETFLAG(v, 0x1000, rt & CSESSRT_ADDCHECK);
		dlg->AddClusterAssoc(CTL_RTPOSAGENT_FLAGS, 13, 0x2000); SETFLAG(v, 0x2000, ort & CSESSOPRT_CHGCCAGENT);
		dlg->AddClusterAssoc(CTL_RTPOSAGENT_FLAGS, 14, 0x4000); SETFLAG(v, 0x4000, ort & CSESSOPRT_ESCCLINEBORD);
		dlg->SetClusterData(CTL_RTPOSAGENT_FLAGS, v);
		if(ExecView(dlg) == cmOK) {
			v = dlg->GetClusterData(CTL_RTPOSAGENT_FLAGS);
			SETFLAG(rt,  CSESSRT_CLOSE,           v & 0x0008);
			SETFLAG(rt,  CSESSRT_ESCCHECK,        v & 0x0010);
			SETFLAG(ort, CSESSOPRT_RETCHECK,      v & 0x0001);
			SETFLAG(ort, CSESSOPRT_ESCCLINE,      v & 0x0002);
			SETFLAG(ort, CSESSOPRT_BANKING,       v & 0x0004);
			SETFLAG(ort, CSESSOPRT_COPYCHECK,     v & 0x0020);
			SETFLAG(ort, CSESSOPRT_COPYZREPT,     v & 0x0040);
			SETFLAG(ort, CSESSOPRT_ROWDISCOUNT,   v & 0x0080);
			SETFLAG(ort, CSESSOPRT_XREP,          v & 0x0100);
			SETFLAG(ort, CSESSOPRT_SPLITCHK,      v & 0x0200);
			SETFLAG(ort, CSESSOPRT_MERGECHK,      v & 0x0400);
			SETFLAG(ort, CSESSOPRT_CHGPRINTEDCHK, v & 0x0800);
			SETFLAG(rt,  CSESSRT_ADDCHECK,        v & 0x1000);
			SETFLAG(ort, CSESSOPRT_CHGCCAGENT,    v & 0x2000);
			SETFLAG(ort, CSESSOPRT_ESCCLINEBORD,  v & 0x4000);
			PPObjCSession::RightsToString(rt, ort, rt_buf);
			pItem->SetStr(pItem->TagID, rt_buf);
			ok = 1;
		}
	}
	delete dlg;
	return ok;
}

int FASTCALL EditObjTagItem(PPID objType, PPID objID, ObjTagItem * pItem, const PPIDArray * pAllowedTags)
{
	class TagValDialog : public TDialog {
	public:
		TagValDialog(uint resID, const ObjTagItem * pItem, PPID objID) : TDialog(resID), ObjID(objID)
		{
			RVALUEPTR(Item, pItem);
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isCmd(cmGenerate)) {
				SString temp_buf;
				S_GUID(SCtrGenerate_).ToStr(S_GUID::fmtIDL, temp_buf);
				setCtrlString(CTL_TAGV_STR, temp_buf);
			}
			else if(event.isCmd(cmLinkObj)) {
				if(ObjID) {
					PPObjTag tag_obj;
					PPObjectTag tag;
					if(tag_obj.Fetch(Item.TagID, &tag) > 0 && tag.ObjTypeID) {
						if(EditPPObj(tag.ObjTypeID, ObjID) > 0) {
							SString obj_name;
							if(GetObjectName(tag.ObjTypeID, ObjID, obj_name) > 0)
								setCtrlString(CTL_TAGV_OBJNAME, obj_name);
						}
					}
				}
			}
			else if(event.isKeyDown(kbF2)) {
				const uint ctl_id = GetCurrId();
				if(oneof2(ctl_id, CTL_TAGV_LINK, CTLSEL_TAGV_LINK)) {
					ComboBox * p_combo = static_cast<ComboBox *>(getCtrlView(CTLSEL_TAGV_LINK));
					if(p_combo && Item.TagDataType == OTTYP_OBJLINK) {
						PPObjTag tag_obj;
						PPObjectTag tag;
						if(tag_obj.Fetch(Item.TagID, &tag) > 0) {
							if(tag.TagEnumID == PPOBJ_PERSON && tag.LinkObjGrp) {
								PPObjPersonKind pk_obj;
								PPPersonKind pk_rec;
								if(pk_obj.Fetch(tag.LinkObjGrp, &pk_rec) > 0 && pk_rec.CodeRegTypeID) {
									SString code, title;
									PPIDArray psn_list;
									PPObjPerson psn_obj;
									PPRegisterType reg_type_rec;
									SearchObject(PPOBJ_REGISTERTYPE, pk_rec.CodeRegTypeID, &reg_type_rec);
									PPLoadText(PPTXT_SEARCHPERSON, title);
									PPInputStringDialogParam isd_param(title, reg_type_rec.Name);
									if(InputStringDialog(&isd_param, code) > 0) {
										psn_obj.GetListByRegNumber(pk_rec.CodeRegTypeID, pk_rec.ID, code, psn_list);
										if(psn_list.getCount())
											setCtrlLong(CTLSEL_TAGV_LINK, psn_list.get(0));
									}
								}
							}
						}
					}
				}
				else if(ctl_id == CTL_TAGV_STR) {
					if(oneof7(Item.TagID, PPTAG_GUA_GOODSRIGHTS, PPTAG_GUA_PERSONRIGHTS,
						PPTAG_GUA_SCARDRIGHTS, PPTAG_GUA_FILESRIGHTS, PPTAG_GUA_SALOCRIGHTS, PPTAG_GUA_TSESSRIGHTS, PPTAG_GUA_PRCRIGHTS)) {
						SString line_buf;
						getCtrlString(CTL_TAGV_STR, line_buf);
						if(PPGlobalAccRights::EditDialog(Item.TagID, line_buf) > 0) {
							setCtrlString(CTL_TAGV_STR, line_buf);
						}
					}
				}
			}
			else
				return;
			clearEvent(event);
		}
		const PPID ObjID;
		ObjTagItem Item;
	};
	int    ok = -1, r, valid_data = 0;
	TagValDialog * dlg = 0;
	uint   dlg_id = 0;
	TagDlgParam param;
	// @v10.9.4 @ctr MEMSZERO(param);
	param.ObjType = objType;
	param.ObjID   = objID;
	param.BoolDlgID  = DLG_TAGVLOG;
	param.StrDlgID   = DLG_TAGVSTR;
	param.NmbDlgID   = DLG_TAGVNMB;
	param.LnkDlgID   = DLG_TAGVLNK;
	param.DateDlgID  = DLG_TAGVDATE;
	param.TimestampDlgID = DLG_TAGVTIMESTAMP;
	param.GuidDlgID  = DLG_TAGVGUID;
	param.ImgDlgID   = DLG_TAGVIMG;
	param.ObjNameCtl = CTL_TAGV_OBJNAME;
	param.TagNameCtl = CTL_TAGV_TAG;
	param.ValBoolCtl = CTL_TAGV_BOOL;
	param.ValStrCtl  = CTL_TAGV_STR;
	param.ValNmbCtl  = CTL_TAGV_NUMBER;
	param.ValLnkCtl  = CTLSEL_TAGV_LINK;
	param.ValDateCtl = CTL_TAGV_DATE;
	param.ValTimeCtl = CTL_TAGV_TIME;
	param.ValGuidCtl = CTL_TAGV_STR;
	param.ValImgCtl  = CTL_TAGV_IMAGE;
	if(pItem->TagID == 0) {
		PPID   tag_id = 0;
		pItem->Destroy();
		ObjTagFilt ot_filt(objType);
		if((r = SelectObjTag(&tag_id, pAllowedTags, &ot_filt)) > 0) {
			THROW(pItem->Init(tag_id));
		}
		else
			return r;
	}
	if(oneof2(objType, 0, PPOBJ_PERSON) && pItem->TagID == PPTAG_PERSON_POSRIGHTS) {
		ok = EditPosRights(pItem);
	}
	else if(oneof2(objType, 0, PPOBJ_LOT) && pItem->TagID == PPTAG_LOT_EGIASINFAREG) {
		PPEgaisProcessor::InformAReg infareg;
		SString temp_buf;
		pItem->GetStr(temp_buf);
		if(temp_buf.NotEmptyS())
			infareg.FromStr(temp_buf);
		if(PPEgaisProcessor::EditInformAReg(infareg) > 0) {
			if(infareg.ToStr(temp_buf)) {
				pItem->SetStr(pItem->TagID, temp_buf);
				ok = 1;
			}
			else
				PPError();
		}
	}
	else if(oneof2(objType, 0, PPOBJ_LOT) && pItem->TagID == PPTAG_LOT_DIMENTIONS) {
		ok = ReceiptCore::LotDimensions::EditTag(0, pItem);
	}
	else if(oneof2(objType, 0, PPOBJ_LOT) && pItem->TagID == PPTAG_LOT_FREIGHTPACKAGE) { // @v10.4.1
		PPTransferItem::FreightPackage fp;
		SString temp_buf;
		pItem->GetStr(temp_buf);
		if(temp_buf.NotEmptyS())
			fp.FromStr(temp_buf);
		if(PPTransferItem::FreightPackage::Edit(&fp) > 0) {
			if(fp.ToStr(temp_buf)) {
				pItem->SetStr(pItem->TagID, temp_buf);
				ok = 1;
			}
			else
				PPError();
		}
	}
	else {
		param.GetDlgID(pItem->TagDataType, &dlg_id);
		THROW(CheckDialogPtr(&(dlg = new TagValDialog(dlg_id, pItem, objID))));
		THROW(param.SetDlgData(dlg, pItem));
		while(!valid_data && (r = ExecView(dlg)) == cmOK)
			if(param.GetDlgData(dlg, pItem))
				ok = valid_data = 1;
			else
				PPError();
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}

class TagValListDialog : public PPListDialog {
public:
	struct DataBlock {
		ObjTagList List;
		const PPIDArray * P_AllowedTags;
		int    UpdateMode;
	};
	explicit TagValListDialog(uint dlgId) : PPListDialog(dlgId/*DLG_TAGVALVIEW*/, CTL_TAGVALVIEW_LIST), P_AllowedTags(0), UpdateMode(0)
	{
		if(SmartListBox::IsValidS(P_Box))
			P_Box->P_Def->SetOption(lbtFocNotify, 1);
	}
	int    setDTS(const DataBlock * pData)
	{
		Data = pData->List;
		if(Data.ObjType) {
			SString obj_buf;
			if(Data.ObjID) {
				GetObjectName(Data.ObjType, Data.ObjID, obj_buf);
			}
			if(!obj_buf.NotEmptyS()) {
				GetObjectTitle(Data.ObjType, obj_buf);
			}
			setStaticText(CTL_TAGVALVIEW_OBJ, obj_buf);
		}
		P_AllowedTags = pData->P_AllowedTags;
		UpdateMode = pData->UpdateMode;
		if(getCtrlView(CTL_TAGVALVIEW_UPD)) {
			long   _mode = 0;
			if(UpdateMode & ObjTagList::mumRemove)
				_mode = 3;
			else if(UpdateMode & ObjTagList::mumUpdate)
				_mode = 2;
			else
				_mode = 1;
			AddClusterAssocDef(CTL_TAGVALVIEW_UPD, 0, 1);
			AddClusterAssoc(CTL_TAGVALVIEW_UPD, 1, 2);
			AddClusterAssoc(CTL_TAGVALVIEW_UPD, 2, 3);
			SetClusterData(CTL_TAGVALVIEW_UPD, _mode);
		}
		updateList(-1);
		return 1;
	}
	int    getDTS(DataBlock * pData)
	{
		int    ok = 1;
		if(pData) {
			if(P_AllowedTags) {
				const ObjTagItem * p_item = 0;
				for(uint i = 0; (p_item = Data.EnumItems(&i)) != 0;)
					THROW_PP(P_AllowedTags->lsearch(p_item->TagID) <= 0, PPERR_INVTAGID);
			}
			if(getCtrlView(CTL_TAGVALVIEW_UPD)) {
				switch(GetClusterData(CTL_TAGVALVIEW_UPD)) {
					case 1: UpdateMode = ObjTagList::mumAdd; break;
					case 2: UpdateMode = (ObjTagList::mumAdd|ObjTagList::mumUpdate); break;
					case 3: UpdateMode = ObjTagList::mumRemove; break;
					default: UpdateMode = ObjTagList::mumAdd; break;
				}
			}
			pData->List = Data;
			pData->UpdateMode = UpdateMode;
		}
		CATCHZOK
		return ok;
	}
private:
	virtual int  setupList();
	virtual int  addItem(long * pos, long * id);
	virtual int  editItem(long pos, long id);
	virtual int  delItem(long pos, long id);

	const PPIDArray * P_AllowedTags;
	PPObjTag    TagObj;
	ObjTagList  Data;
	int    UpdateMode;
};

int TagValListDialog::setupList()
{
	PPObjTag    objtag;
	PPObjectTag tag;
	SString buf;
	StringSet ss(SLBColumnDelim);
	const  ObjTagItem * p_item;
	for(uint i = 0; (p_item = Data.EnumItems(&i)) != 0;) {
		ss.clear();
		buf.Z();
		if(objtag.Fetch(p_item->TagID, &tag) > 0)
			buf = tag.Name;
		else
			buf.Cat(p_item->TagID);
		ss.add(buf);
		TagObj.GetCurrTagVal(p_item, buf.Z());
		ss.add(buf);
		if(!addStringToList(p_item->TagID, ss.getBuf()))
			return 0;
	}
	return 1;
}

int TagValListDialog::addItem(long * pPos, long * pID)
{
	int    ok = -1;
	ObjTagItem item;
	if(EditObjTagItem(Data.ObjType, Data.ObjID, &item, P_AllowedTags) > 0) {
		Data.PutItem(item.TagID, &item);
		ASSIGN_PTR(pPos, Data.GetCount());
		ASSIGN_PTR(pID, item.TagID);
		ok = 1;
	}
	return ok;
}

int TagValListDialog::editItem(long pos, long id)
{
	int    ok = -1;
	const  ObjTagItem * p_item = Data.GetItemByPos(static_cast<uint>(pos));
   	if(p_item && p_item->TagID == id)
		if(EditObjTagItem(Data.ObjType, Data.ObjID, const_cast<ObjTagItem *>(p_item), P_AllowedTags) > 0) // @badcast
			ok = 1;
	return ok;
}

int TagValListDialog::delItem(long, long id)
{
	return id ? Data.PutItem(id, 0) : 0;
}

int EditObjTagValList(ObjTagList * pList, const PPIDArray * pAllowedTags)
{
	int    ok = -1;
	TagValListDialog::DataBlock blk;
	blk.List = *pList;
	blk.P_AllowedTags = pAllowedTags;
	blk.UpdateMode = 0;
	TagValListDialog * dlg = new TagValListDialog(DLG_TAGVALVIEW);
	if(CheckDialogPtrErr(&dlg)) {
		dlg->setDTS(&blk);
		while(ok < 0 && ExecView(dlg) == cmOK) {
			if(dlg->getDTS(&blk) > 0) {
				*pList = blk.List;
				ok = 1;
			}
			else
				PPError();
		}
		delete dlg;
	}
	else
		ok = 0;
	return ok;
}

int EditObjTagValUpdateList(ObjTagList * pList, const PPIDArray * pAllowedTags, int * pUpdateMode)
{
	int    ok = -1;
	TagValListDialog::DataBlock blk;
	blk.List = *pList;
	blk.P_AllowedTags = pAllowedTags;
	blk.UpdateMode = DEREFPTRORZ(pUpdateMode);
	TagValListDialog * dlg = new TagValListDialog(DLG_TAGVALUPD);
	if(CheckDialogPtrErr(&dlg)) {
		dlg->setDTS(&blk);
		while(ok < 0 && ExecView(dlg) == cmOK) {
			if(dlg->getDTS(&blk) > 0) {
				*pList = blk.List;
				ASSIGN_PTR(pUpdateMode, blk.UpdateMode);
				ok = 1;
			}
			else
				PPError();
		}
		delete dlg;
	}
	else
		ok = 0;
	return ok;
}

int EditObjTagValList(PPID objType, PPID objID, const PPIDArray * pAllowedTags)
{
	int    ok = 1;
	Reference * p_ref = PPRef;
	ObjTagList list;
	THROW(p_ref->Ot.GetList(objType, objID, &list));
	list.ObjType = objType;
	list.ObjID = objID;
	if(EditObjTagValList(&list, pAllowedTags) > 0)
		THROW(p_ref->Ot.PutList(objType, objID, &list, 1));
	CATCHZOKPPERR
	return ok;
}
//
//
//
class ObjTagCache {
public:
	ObjTagCache();
	~ObjTagCache();
	int    Fetch(PPID objID, PPID tagID, ObjTagItem * pItem);
	int    FASTCALL Dirty(PPID objType, PPID objID, PPID tagID);
private:
	struct TagTypeEntry {
		PPID   TagID;
		PPID   ObjType;
		PPID   TagEnumID;
		int    TagDataType;
		UintHashTable UndefList; // Список идентификаторов объектов, для которых нет тега TagID
	};
	struct Entry {
		enum {
			fBusy  = 0x0001,
			fDirty = 0x0002
		};
		PPID   ObjID;
		uint16 TagIdx;
		uint16 F;
		ACount Counter; // Счетчик обращений к этому элементу
		union {
			int32  I;
			double R;
			uint   StrP;
		};
	};
	struct Stat {
		ACount Hits;
		ACount Misses;
		ACount Count;
		ACount Collisions;    // Количество коллизий при нахождении слота (для ObjCacheHash)
		long   UnusedEntries; // Количество неиспользуемых слотов (для ObjCacheHash)
		long   MaxCounter;    // Максимальный счетчик
		long   MinCounter;    // Минимальный счетчик
	};
	static int OnSysJ(int kind, const PPNotifyEvent * pEv, void * procExtPtr);
	uint   Hash(PPID objID, PPID tagID, uint n) const;
	Entry * FASTCALL SearchByPos(uint pos, int incr);
	int    Helper_Get(PPID objID, PPID tagID, ObjTagItem * pDataRec);
	int    Helper_GetByPos(uint pos, ObjTagItem * pDataRec);
	void   EntryToData(const ObjTagCache::Entry * pEntry, ObjTagItem * pDataRec) const;
	int    AddItem(const ObjTagCache::Entry * pEntry, uint * pPos);

	uint   MaxItems;
	uint   MaxTries;

	class TagTypeArray : public TSArray <TagTypeEntry> {
	public:
		TagTypeArray() : TSArray <TagTypeEntry> (aryDataOwner | aryEachItem)
		{
		}
		virtual void FASTCALL freeItem(void * pItem)
		{
			static_cast<TagTypeEntry *>(pItem)->UndefList.Destroy();
		}
	};
	TagTypeArray TagTypeList;
	Entry * P_Items;
	long   AdvCookie;
	StringSet Ss;
	Stat   StatData;
	ReadWriteLock RwL;
};

/*static*/int ObjTagCache::OnSysJ(int kind, const PPNotifyEvent * pEv, void * procExtPtr)
{
	int    ok = -1;
	if(kind == PPAdviseBlock::evDirtyCacheBySysJ) {
		if(oneof3(pEv->Action, PPACN_OBJTAGUPD, PPACN_OBJTAGRMV, PPACN_OBJTAGADD)) {
			ObjTagCache * p_cache = static_cast<ObjTagCache *>(procExtPtr);
			if(p_cache) {
				p_cache->Dirty(pEv->ObjType, pEv->ObjID, pEv->ExtInt_);
				ok = 1;
			}
		}
	}
	return ok;
}

ObjTagCache::ObjTagCache() : AdvCookie(0), P_Items(0), MaxItems(0), MaxTries(8)
{
	Ss.add("$"); // zero index - is empty string
	const uint init_items_count = SMEGABYTE(1) / sizeof(Entry);
	MEMSZERO(StatData);
	size_t i = init_items_count;
	if(i) {
		do {
			if(IsPrime(i)) {
				MaxItems = i;
				break;
			}
		} while(--i);
	}
	assert(MaxItems);
	P_Items = static_cast<Entry *>(SAlloc::C(MaxItems, sizeof(Entry)));
	if(P_Items) {
		long   cookie = 0;
		{
			PPAdviseBlock adv_blk;
			adv_blk.Kind = PPAdviseBlock::evDirtyCacheBySysJ;
			adv_blk.Action = PPACN_OBJTAGUPD;
			adv_blk.DbPathID = DBS.GetDbPathID();
			adv_blk.ObjType = 0;
			adv_blk.Proc = ObjTagCache::OnSysJ;
			adv_blk.ProcExtPtr = this;
			DS.Advise(&cookie, &adv_blk);
		}
		{
			PPAdviseBlock adv_blk;
			adv_blk.Kind = PPAdviseBlock::evDirtyCacheBySysJ;
			adv_blk.Action = PPACN_OBJTAGADD;
			adv_blk.DbPathID = DBS.GetDbPathID();
			adv_blk.ObjType = 0;
			adv_blk.Proc = ObjTagCache::OnSysJ;
			adv_blk.ProcExtPtr = this;
			DS.Advise(&cookie, &adv_blk);
		}
		{
			PPAdviseBlock adv_blk;
			adv_blk.Kind = PPAdviseBlock::evDirtyCacheBySysJ;
			adv_blk.Action = PPACN_OBJTAGRMV;
			adv_blk.DbPathID = DBS.GetDbPathID();
			adv_blk.ObjType = 0;
			adv_blk.Proc = ObjTagCache::OnSysJ;
			adv_blk.ProcExtPtr = this;
			DS.Advise(&cookie, &adv_blk);
		}
	}
	else {
		MaxItems = 0;
	}
}

ObjTagCache::~ObjTagCache()
{
	delete P_Items;
}

uint ObjTagCache::Hash(PPID objID, PPID tagID, uint n) const
{
	uint32 key[3];
	key[0] = (uint32)objID;
	key[1] = (uint32)tagID;
	key[2] = n;
	uint32 h = SlHash::BobJenc(key, sizeof(key));
	return (uint)(h % MaxItems);
}

ObjTagCache::Entry * FASTCALL ObjTagCache::SearchByPos(uint pos, int incr)
{
	ObjTagCache::Entry * p_entry = 0;
	if(pos < MaxItems) {
		p_entry = (ObjTagCache::Entry *)(PTR8(P_Items) + pos * sizeof(Entry));
		if(incr)
			p_entry->Counter.Incr();
	}
	return p_entry;
}

void ObjTagCache::EntryToData(const ObjTagCache::Entry * pEntry, ObjTagItem * pTag) const
{
	assert(pEntry->TagIdx > 0 && pEntry->TagIdx <= TagTypeList.getCount());
	const TagTypeEntry & r_te = TagTypeList.at(pEntry->TagIdx-1);
	if(pTag->Init(r_te.TagID)) {
		if(r_te.TagDataType == pTag->TagDataType) {
			switch(r_te.TagDataType) {
				case OTTYP_BOOL: pTag->Val.IntVal = pEntry->I; break;
				case OTTYP_NUMBER: pTag->Val.RealVal = pEntry->R; break;
				case OTTYP_ENUM: pTag->Val.IntVal = pEntry->I; break;
				case OTTYP_INT:  pTag->Val.IntVal = pEntry->I; break;
				case OTTYP_OBJLINK: pTag->Val.IntVal = pEntry->I; break;
				case OTTYP_DATE: pTag->Val.DtVal.v = pEntry->I; break;
				case OTTYP_TIMESTAMP:
					{
						OleDate od;
						od.v = pEntry->R;
						pTag->Val.DtmVal = od;
					}
					break;
				case OTTYP_GUID:
				case OTTYP_IMAGE:
				case OTTYP_STRING:
					if(pEntry->StrP) {
						SString temp_buf;
						uint   sp = pEntry->StrP;
						Ss.get(&sp, temp_buf);
						if(temp_buf.NotEmptyS())
							pTag->Val.PStr = newStr(temp_buf);
					}
					break;
			}
		}
	}
}

int ObjTagCache::Helper_GetByPos(uint pos, ObjTagItem * pDataRec)
{
	int    ok = 1;
	Entry * p_entry = SearchByPos(pos, 1);
	if(p_entry) {
		if(p_entry->F & ObjCacheEntry::fDirty)
			ok = -1;
		else
			EntryToData(p_entry, pDataRec);
	}
	else
		ok = 0;
	return ok;
}

int ObjTagCache::Helper_Get(PPID objID, PPID tagID, ObjTagItem * pDataRec)
{
	int    ok = -1;
	if(objID && tagID) {
		int    found = 0;
		for(uint t = 0; !found && t < MaxTries; t++) {
			const  uint f = Hash(objID, tagID, t);
			Entry * p_entry = SearchByPos(f, 0);
			if(p_entry && p_entry->ObjID == objID) {
				PPID   tag_id_ = TagTypeList.at(p_entry->TagIdx-1).TagID;
				if(tag_id_ == tagID) {
					found = 1;
					if(!(p_entry->F & Entry::fDirty)) {
						ok = Helper_GetByPos(f, pDataRec);
						if(ok > 0)
							StatData.Hits.Incr();
					}
				}
			}
		}
	}
	return ok;
}

int ObjTagCache::AddItem(const ObjTagCache::Entry * pEntry, uint * pPos)
{
	int    ok = 0;
	int    pos_undefined = 1; // @debug
	uint   idx = *pPos;
	long   min_count = MAXLONG;
	assert(pEntry->TagIdx > 0 && pEntry->TagIdx <= TagTypeList.getCount());
	const TagTypeEntry & r_te = TagTypeList.at(pEntry->TagIdx-1);
	for(uint t = 0; min_count && !ok && t < MaxTries; t++) {
		const  uint f = Hash(pEntry->ObjID, r_te.TagID, t);
		Entry * p_entry = SearchByPos(f, 0);
		if(p_entry) {
			if(p_entry->ObjID == pEntry->ObjID && p_entry->TagIdx == pEntry->TagIdx) {
				idx = f;
				pos_undefined = 0;
				/*
				if(Ss.getDataLen())
					UnusedNameSpace += Ss.getLen(p_entry->NameIdx)+1; // (+1) - разделитель строк
				*/
				//
				// Чтобы не затереть счетчик обращений копируем только прикладную часть буфера
				// и индекс строки наименования //
				//
				*p_entry = *pEntry;
				p_entry->F &= ~ObjCacheEntry::fDirty;
				//PackNames();
				ok = 2;
			}
			else if(p_entry->F & Entry::fDirty) {
				//
				// Если встретили "грязный" элемент, то можем с чистой совестью его затереть
				//
				idx = f;
				pos_undefined = 0;
				break;
			}
			else if(p_entry->Counter < min_count) {
				idx = f;
				min_count = p_entry->Counter;
				pos_undefined = 0;
			}
		}
	}
	assert(pos_undefined == 0); // @debug
	if(ok <= 0 && idx < MaxItems) {
		Entry * p_entry = SearchByPos(idx, 0);
		if(p_entry) {
			/*
			if(Ss.getDataLen())
				UnusedNameSpace += Ss.getLen(p_entry->NameIdx)+1; // (+1) - разделитель строк
			*/
			*p_entry = *pEntry;
			p_entry->F &= ~ObjCacheEntry::fDirty;
			//PackNames();
			ok = 1;
		}
	}
	ASSIGN_PTR(pPos, idx);
	return ok;
}

int ObjTagCache::Fetch(PPID objID, PPID tagID, ObjTagItem * pItem)
{
	int    ok = -1;
	uint   p = 0;
	if(objID && tagID) {
		uint16 tp = 0;
		uint   pos = 0;
		SRWLOCKER(RwL, SReadWriteLocker::Read);
		StatData.Count.Incr();
		if(TagTypeList.lsearch(&tagID, &pos, CMPF_LONG)) {
			assert(pos < MAXSHORT);
			tp = static_cast<uint16>(pos+1);
		}
		if(tp && TagTypeList.at(tp-1).UndefList.Has((uint32)objID)) {
			ok = -1; // Объект находится в списке тех, для кого тег не определен.
		}
		else {
			ok = Helper_Get(objID, tagID, pItem);
			if(ok < 0) {
				SRWLOCKER_TOGGLE(SReadWriteLocker::Write);
				//
				// Пока мы ждали своей очереди на запись
				// другой поток мог занести нужный нам элемент в кэш.
				// По-этому снова проверяем наличие этого элемента.
				//
				ok = Helper_Get(objID, tagID, pItem);
				if(ok < 0) {
					Entry  entry;
					MEMSZERO(entry);
					if(!tp) {
						PPObjTag tag_obj;
						PPObjectTag tag_rec;
						if(tag_obj.Fetch(tagID, &tag_rec) > 0) {
							TagTypeEntry tt_entry;
							tt_entry.TagID = tag_rec.ID;
							tt_entry.ObjType = tag_rec.ObjTypeID;
							tt_entry.TagDataType = tag_rec.TagDataType;
							TagTypeList.insert(&tt_entry);
							pos = TagTypeList.getCount();
							assert(pos < MAXSHORT);
							tp = (uint16)pos;
						}
					}
					if(tp) {
						ObjTagItem tag_item;
						const TagTypeEntry & r_te = TagTypeList.at(tp-1);
						if(PPRef->Ot.GetTag(r_te.ObjType, objID, tagID, &tag_item) > 0) {
							entry.ObjID = objID;
							entry.TagIdx = tp;
							entry.F |= Entry::fBusy;
							switch(tag_item.TagDataType) {
								case OTTYP_BOOL:
								case OTTYP_ENUM:
								case OTTYP_INT:
								case OTTYP_OBJLINK: entry.I = tag_item.Val.IntVal; break;
								case OTTYP_NUMBER:  entry.R = tag_item.Val.RealVal; break;
								case OTTYP_DATE:    entry.I = tag_item.Val.DtVal.v; break;
								case OTTYP_TIMESTAMP:
									{
										OleDate od;
										od = tag_item.Val.DtmVal;
										entry.R = od.v;
									}
									break;
								case OTTYP_GUID:
								case OTTYP_IMAGE:
								case OTTYP_STRING:
									if(tag_item.Val.PStr)
										Ss.add(tag_item.Val.PStr, &entry.StrP);
									else
										entry.StrP = 0;
									break;
							}
							ok = 1;
						}
						else {
							TagTypeList.at(tp-1).UndefList.Add((uint32)objID);
							ok = -1;
						}
					}
					if(ok > 0) {
						entry.F &= ~Entry::fDirty;
						if(AddItem(&entry, &p)) {
							StatData.Misses.Incr();
							ok = Helper_GetByPos(p, pItem);
						}
						else
							ok = 0;
					}
				}
			}
		}
	}
	return ok;
}

int FASTCALL ObjTagCache::Dirty(PPID objType, PPID objID, PPID tagID)
{
	int    ok = 1;
	{
		SRWLOCKER(RwL, SReadWriteLocker::Write);
		for(uint i = 0; i < TagTypeList.getCount(); i++) {
			TagTypeEntry & r_te = TagTypeList.at(i);
			if((!tagID || r_te.TagID == tagID) && r_te.ObjType == objType) {
				if(r_te.UndefList.Has(objID)) {
					r_te.UndefList.Remove(objID);
				}
				for(uint t = 0; t < MaxTries; t++) {
					const  uint f = Hash(objID, r_te.TagID, t);
					Entry * p_entry = SearchByPos(f, 0);
					if(p_entry && p_entry->ObjID == objID && p_entry->TagIdx == (i+1)) {
						p_entry->F |= Entry::fDirty;
						break;
					}
				}
			}
		}
	}
	return ok;
}
//
// TagCache
//
class TagCache : public ObjCache {
public:
	TagCache() : ObjCache(PPOBJ_TAG, sizeof(TagCacheEntry)), SymbList(PPOBJ_TAG), P_Ic(0)
	{
	}
	~TagCache()
	{
		delete P_Ic;
	}
	int    FetchTag(PPID objID, PPID tagID, ObjTagItem * pItem);
	int    DirtyTag(PPID objType, PPID objID, PPID tagID);
	int    FetchBySymb(const char * pSymb, PPID * pID)
	{
		return SymbList.FetchBySymb(pSymb, pID);
	}
private:
	virtual int  FetchEntry(PPID, ObjCacheEntry * pEntry, long);
	virtual void EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const;
	virtual int  FASTCALL Dirty(PPID id); // @sync_w

	struct TagCacheEntry : public ObjCacheEntry {
		int16  Flags;
		int16  Reserve;    // @alignment
		PPID   LinkObjGrp;
		PPID   TagEnumID;
		long   TagDataType;
		PPID   ObjTypeID;
		PPID   TagGroupID;
	};
	ObjTagCache * P_Ic;
	RefSymbArray SymbList;
};

int FASTCALL TagCache::Dirty(PPID id)
{
	int    ok = 1;
	ObjCache::Dirty(id);
	SymbList.Dirty(id);
	return ok;
}

int TagCache::FetchTag(PPID objID, PPID tagID, ObjTagItem * pItem)
{
	SETIFZ(P_Ic, new ObjTagCache);
	return P_Ic ? P_Ic->Fetch(objID, tagID, pItem) : 0;
}

int TagCache::DirtyTag(PPID objType, PPID objID, PPID tagID)
{
	CALLPTRMEMB(P_Ic, Dirty(objType, objID, tagID));
	return 1;
}

int TagCache::FetchEntry(PPID id, ObjCacheEntry * pEntry, long)
{
	int    ok = 1;
	TagCacheEntry * p_rec = static_cast<TagCacheEntry *>(pEntry);
	PPObjectTag tag;
	if(oneof2(id, PPTAG_LOT_CLB, PPTAG_LOT_SN)) {
		p_rec->Flags       = OTF_NOZERO;
		p_rec->LinkObjGrp  = 0;
		p_rec->TagEnumID   = 0;
		p_rec->TagDataType = OTTYP_STRING;
		p_rec->ObjTypeID   = PPOBJ_LOT;
		p_rec->TagGroupID  = 0;
		const char * p_tag_name = 0;
		switch(id) {
			case PPTAG_LOT_CLB: p_tag_name = "LOT_CLB"; break;
			case PPTAG_LOT_SN: p_tag_name = "LOT_SERIAL"; break;
			default: p_tag_name = "UNKNOWN"; break;
		}
		StringSet & r_ss = DS.AcquireRvlSsSCD();
		r_ss.add(p_tag_name);
		r_ss.add(p_tag_name);
		ok = PutName(r_ss.getBuf(), p_rec);
	}
	else if(id == PPTAG_FLOAT_SERIAL) {
		p_rec->Flags       = OTF_NOZERO;
		p_rec->LinkObjGrp  = 0;
		p_rec->TagEnumID   = 0;
		p_rec->TagDataType = OTTYP_STRING;
		p_rec->ObjTypeID   = PPOBJ_SERIAL;
		p_rec->TagGroupID  = 0;
		const char * p_tag_name = "FREE_SERIAL";
		StringSet & r_ss = DS.AcquireRvlSsSCD();
		r_ss.add(p_tag_name);
		r_ss.add(p_tag_name);
		ok = PutName(r_ss.getBuf(), p_rec);
	}
	else if(oneof4(id, PPTAG_BILL_CREATEDTM, PPTAG_BILL_CREATEDTMEND, PPTAG_BILL_GPSCOORD, PPTAG_BILL_GPSCOORDEND)) {
		p_rec->Flags       = OTF_NOZERO;
		p_rec->LinkObjGrp  = 0;
		p_rec->TagEnumID   = 0;
		p_rec->TagDataType = OTTYP_STRING;
		p_rec->ObjTypeID   = PPOBJ_LOT;
		p_rec->TagGroupID  = 0;
		const char * p_tag_name = 0;
		switch(id) {
			case PPTAG_BILL_CREATEDTM: p_tag_name = "BILL_CREATEDTM"; break;
			case PPTAG_BILL_CREATEDTMEND: p_tag_name = "BILL_CREATEDTMEND"; break;
			case PPTAG_BILL_GPSCOORD: p_tag_name = "BILL_GPSCOORD"; break;
			case PPTAG_BILL_GPSCOORDEND: p_tag_name = "BILL_GPSCOORDEND"; break;
			default: p_tag_name = "UNKNOWN"; break;
		}
		StringSet & r_ss = DS.AcquireRvlSsSCD();
		r_ss.add(p_tag_name);
		r_ss.add(p_tag_name);
		ok = PutName(r_ss.getBuf(), p_rec);
	}
	else if((ok = PPRef->GetItem(PPOBJ_TAG, id, &tag)) > 0) {
	   	p_rec->Flags       = static_cast<int16>(tag.Flags);
	   	p_rec->LinkObjGrp  = tag.LinkObjGrp;
		p_rec->TagEnumID   = tag.TagEnumID;
		p_rec->TagDataType = tag.TagDataType;
		p_rec->ObjTypeID   = tag.ObjTypeID;
		p_rec->TagGroupID  = tag.TagGroupID;
		StringSet & r_ss = DS.AcquireRvlSsSCD();
		r_ss.add(tag.Name);
		r_ss.add(tag.Symb);
		ok = PutName(r_ss.getBuf(), p_rec);
	}
	return ok;
}

void TagCache::EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const
{
	PPObjectTag   * p_tag = static_cast<PPObjectTag *>(pDataRec);
	const TagCacheEntry * p_cr  = static_cast<const TagCacheEntry *>(pEntry);
	memzero(p_tag, sizeof(PPObjectTag));
	p_tag->Tag = PPOBJ_TAG;
	p_tag->ID  = p_cr->ID;
   	p_tag->Flags       = p_cr->Flags;
   	p_tag->LinkObjGrp  = p_cr->LinkObjGrp;
	p_tag->TagEnumID   = p_cr->TagEnumID;
	p_tag->TagDataType = p_cr->TagDataType;
	p_tag->ObjTypeID   = p_cr->ObjTypeID;
	p_tag->TagGroupID  = p_cr->TagGroupID;
	//
	char   temp_buf[2048];
	GetName(pEntry, temp_buf, sizeof(temp_buf));
	// @v9.9.5 PPStringSetSCD ss;
	StringSet & r_ss = DS.AcquireRvlSsSCD(); // @v9.9.5
	r_ss.setBuf(temp_buf, sstrlen(temp_buf)+1);
	uint   p = 0;
	r_ss.get(&p, p_tag->Name, sizeof(p_tag->Name));
	r_ss.get(&p, p_tag->Symb, sizeof(p_tag->Symb));
}

int PPObjTag::Fetch(PPID id, PPObjectTag * pRec)
{
	TagCache * p_cache = GetDbLocalCachePtr <TagCache> (Obj);
	return p_cache ? p_cache->Get(id, pRec) : Search(id, pRec);
}

int PPObjTag::FetchBySymb(const char * pSymb, PPID * pID)
{
	TagCache * p_cache = GetDbLocalCachePtr <TagCache> (Obj);
	return p_cache ? p_cache->FetchBySymb(pSymb, pID) : SearchBySymb(pSymb, pID, 0);
}

int PPObjTag::FetchTag(PPID objID, PPID tagID, ObjTagItem * pItem)
{
	int    ok = -1;
	TagCache * p_cache = GetDbLocalCachePtr <TagCache> (PPOBJ_TAG);
	if(p_cache)
		ok = p_cache->FetchTag(objID, tagID, pItem);
	else {
		Reference * p_ref = PPRef;
		if(p_ref) {
			PPObjectTag tag_rec;
			if(Fetch(tagID, &tag_rec) > 0)
				ok = p_ref->Ot.GetTag(tag_rec.ObjTypeID, objID, tagID, pItem);
		}
	}
	return ok;
}

int PPObjTag::DirtyTag(PPID objType, PPID objID, PPID tagID)
{
	TagCache * p_cache = GetDbLocalCachePtr <TagCache> (PPOBJ_TAG);
	return p_cache ? p_cache->DirtyTag(objType, objID, tagID) : 1;
}
//
// Implementation of PPALDD_TagType
//
PPALDD_CONSTRUCTOR(TagType)
{
	if(Valid)
		AssignHeadData(&H, sizeof(H));
}

PPALDD_DESTRUCTOR(TagType) { Destroy(); }

int PPALDD_TagType::InitData(PPFilt & rFilt, long rsrv)
{
	if(rFilt.ID != H.ID) {
		MEMSZERO(H);
		H.ID = rFilt.ID;
		PPObjTag tag_obj;
		PPObjectTag tag_rec;
		if(tag_obj.Fetch(H.ID, &tag_rec) > 0) {
			H.TagDataType = tag_rec.TagDataType;
			H.Flags       = tag_rec.Flags;
			H.TagEnumID   = tag_rec.TagEnumID;
			H.LinkObjGrp  = tag_rec.LinkObjGrp;
			H.ObjTypeID   = tag_rec.ObjTypeID;
			H.TagGroupID  = tag_rec.TagGroupID;
			STRNSCPY(H.Name, tag_rec.Name);
			STRNSCPY(H.Symb, tag_rec.Symb);
		}
	}
	return DlRtm::InitData(rFilt, rsrv);
}
//
// Implementation of PPALDD_ObjectTag
//
PPALDD_CONSTRUCTOR(ObjectTag) { InitFixData(rscDefHdr, &H, sizeof(H)); }
PPALDD_DESTRUCTOR(ObjectTag) { Destroy(); }

int PPALDD_ObjectTag::InitData(PPFilt & rFilt, long rsrv)
{
	int    ok = -1;
	if(rFilt.ID == H.SurID)
		ok = DlRtm::InitData(rFilt, rsrv);
	else {
		size_t data_len = 0;
		MEMSZERO(H);
		const ObjTagTbl::Rec * p_tag_rec = static_cast<const ObjTagTbl::Rec *>(DS.GetTLA().SurIdList.Get(rFilt.ID, &data_len));
		if(p_tag_rec && data_len == sizeof(*p_tag_rec)) {
			H.SurID = rFilt.ID;
			H.ObjTypeID  = p_tag_rec->ObjType;
			H.ObjID      = p_tag_rec->ObjID;
			H.TagID      = p_tag_rec->TagID;
			H.TagByObj   = p_tag_rec->TagByObj;
			H.IntVal     = p_tag_rec->IntVal;
			H.DateVal.v  = p_tag_rec->IntVal;
			H.RealVal    = p_tag_rec->RealVal;
			STRNSCPY(H.StrVal, p_tag_rec->StrVal);
			ok = DlRtm::InitData(rFilt, rsrv);
		}
	}
	return ok;
}

void PPALDD_ObjectTag::EvaluateFunc(const DlFunc * pF, SV_Uint32 * pApl, RtmStack & rS)
{
	#define _RET_STR     (**static_cast<SString **>(rS.GetPtr(pApl->Get(0))))
	if(pF->Name == "?GetStr") {
		_RET_STR.Z();
		size_t data_len = 0;
		const ObjTagTbl::Rec * p_tag_rec = static_cast<const ObjTagTbl::Rec *>(DS.GetTLA().SurIdList.Get(H.SurID, &data_len));
		if(p_tag_rec && data_len == sizeof(*p_tag_rec)) {
			ObjTagItem item;
			if(item.Get_(*p_tag_rec) > 0)
				item.GetStr(_RET_STR);
		}
	}
}
