// GDS2OBJA.CPP
// Copyright (c) A.Sobolev 2005, 2006, 2007, 2008, 2014, 2015, 2016, 2017
//
// Список соответствий Товар(Группа товаров) - Объект
// Используется для автоматического формирования документов
// производства и передачи со складов, на которых товар производится //
// на склады, на которых он потребляется.
//
#include <pp.h>
#pragma hdrstop

SLAPI GoodsToObjAssoc::GoodsToObjAssoc(PPID asscTyp, PPID objType, int dupAllowing)
{
	AsscType = asscTyp;
	ObjType = objType;
	Flags = 0;
	SETFLAG(Flags, fDup, dupAllowing);
	MEMSZERO(NoaRec);
	if(asscTyp > 1000) {
		PPObjNamedObjAssoc noa_obj;
		THROW(noa_obj.Search(asscTyp, &NoaRec) > 0);
		THROW_PP(NoaRec.PrmrObjType == PPOBJ_GOODS, PPERR_NAMEDOBJASSCNGOODS);
		ObjType = NoaRec.ScndObjType;
	}
	CATCH
		Flags |= fError;
	ENDCATCH
}

int SLAPI GoodsToObjAssoc::IsValid() const
{
	return !BIN(Flags & fError);
}

SLAPI GoodsToObjAssoc::operator const LAssocArray & () const
{
	return List;
}

uint SLAPI GoodsToObjAssoc::GetCount() const
{
	return List.getCount();
}

const LAssoc & SLAPI GoodsToObjAssoc::at(uint pos) const
{
	return List.at(pos);
}

SString & SLAPI GoodsToObjAssoc::GetKeyName(PPID id, SString & rBuf)
{
	Goods2Tbl::Rec goods_rec;
	rBuf = 0;
	if(GObj.Fetch(id, &goods_rec) > 0) { // @v5.4.6 Search-->Fetch
		if(goods_rec.Kind == PPGDSK_GROUP)
			rBuf.CatChar('@');
		rBuf.Cat(goods_rec.Name);
	}
	else
		rBuf.Cat(id);
	return rBuf;
}

int SLAPI GoodsToObjAssoc::Save()
{
	return PPRef->Assc.AddArray(AsscType, &List, 1, 1);
}

int SLAPI GoodsToObjAssoc::Load()
{
	List.freeAll();
	return PPRef->Assc.GetList(AsscType, &List);
}

int SLAPI GoodsToObjAssoc::Add(PPID goodsID, PPID objID, uint * pPos)
{
	int    ok = 1;
	if(List.SearchPair(goodsID, objID, 0))
		ok = PPSetError(PPERR_DUPGOODSTOOBJASSOC);
	else if((Flags & fDup) || List.CheckUnique(goodsID, 1))
		ok = List.Add(goodsID, objID, pPos, 1) ? 1 : PPSetErrorSLib();
	else
		ok = PPSetError(PPERR_DUPGOODSTOOBJASSOC);
	return ok;
}

#if 0 // {
int SLAPI GoodsToObjAssoc::Update(PPID goodsID, PPID objID, uint * pPos)
{
	long   obj_id = 0;
	uint   pos = 0;
	if(List.Search(goodsID, &obj_id, &pos, 1)) {
		List.at(pos).Val = objID;
		ASSIGN_PTR(pPos, pos);
		return 1;
	}
	else {
		PPSetAddedMsgObjName(PPOBJ_GOODS, goodsID);
		return PPSetError(PPERR_GOODS2OBJASSCNFOUND);
	}
}
#endif // } 0

int SLAPI GoodsToObjAssoc::SearchPair(PPID goodsID, PPID objID, uint * pPos) const
{
	return List.SearchPair(goodsID, objID, pPos);
}

int SLAPI GoodsToObjAssoc::UpdateByPos(uint pos, PPID goodsID, PPID objID)
{
	int    ok = 0;
	if(pos < List.getCount()) {
		LAssoc temp_item = List.at(pos);
		List.atFree(pos);
		{
			if(List.SearchPair(goodsID, objID, 0))
				ok = PPSetError(PPERR_DUPGOODSTOOBJASSOC);
			else if((Flags & fDup) || List.CheckUnique(goodsID, 1)) {
				LAssoc new_item;
				new_item.Key = goodsID;
				new_item.Val = objID;
				ok = List.atInsert(pos, &new_item) ? 1 : PPSetErrorSLib();
			}
			else
				ok = PPSetError(PPERR_DUPGOODSTOOBJASSOC);
		}
		if(!ok) {
			List.atInsert(pos, &temp_item);
			ok = 1;
		}
	}
	return ok;
}

int SLAPI GoodsToObjAssoc::Remove(PPID goodsID, PPID objID)
{
	int    ok = -1;
	uint   pos = 0;
	if(List.SearchPair(goodsID, objID, &pos)) {
		List.atFree(pos);
		ok = 1;
	}
	return ok;
}

int SLAPI GoodsToObjAssoc::GetListByObj(PPID objID, PPIDArray & rGoodsList) const
{
	return List.GetListByVal(objID, rGoodsList);
}

int SLAPI GoodsToObjAssoc::GetListByGoods(PPID goodsID, PPIDArray & rObjList)
{
	Goods2Tbl::Rec goods_rec;
	PPIDArray temp_list;
	List.GetListByKey(goodsID, temp_list);
	if(GObj.Fetch(goodsID, &goods_rec) > 0 && goods_rec.ParentID) {
		PPIDArray temp_list2;
		GetListByGoods(goods_rec.ParentID, temp_list2); // @recursion
		temp_list.addUnique(&temp_list2);
	}
	rObjList.addUnique(&temp_list);
	return temp_list.getCount() ? 1 : -1;
}

int SLAPI GoodsToObjAssoc::Get(PPID goodsID, PPID * pObjID)
{
	int    ok = -1;
	PPID   obj_id = 0;
	Goods2Tbl::Rec goods_rec;
	PPIDArray cycle_list;
	for(PPID temp_id = goodsID; ok < 0 && temp_id;) {
		if(cycle_list.lsearch(temp_id))
			ok = PPSetError(PPERR_CYCLELINKGOODSGRP, goods_rec.Name);
		else if(List.Search(temp_id, &obj_id, 0, 1))
			ok = 1;
		else if(GObj.Fetch(temp_id, &goods_rec) > 0) {
			cycle_list.add(temp_id);
			temp_id = goods_rec.ParentID;
		}
		else
			temp_id = 0;
	}
	ASSIGN_PTR(pObjID, obj_id);
	return ok;
}
//
// @ModuleDef(PPViewGoodsToObjAssoc)
//
IMPLEMENT_PPFILT_FACTORY(GoodsToObjAssoc); SLAPI GoodsToObjAssocFilt::GoodsToObjAssocFilt() :
	PPBaseFilt(PPFILT_GOODSTOOBJASSOC, 0, 1) // @v8.1.9 ver 0-->1
{
	P_LocF = 0;
	SetFlatChunk(offsetof(GoodsToObjAssocFilt, ReserveStart),
		offsetof(GoodsToObjAssocFilt, Reserve)-offsetof(GoodsToObjAssocFilt, ReserveStart)+sizeof(Reserve));
	SetBranchBaseFiltPtr(PPFILT_LOCATION, offsetof(GoodsToObjAssocFilt, P_LocF)); // @v8.1.9
	Init(1, 0);
}

SLAPI PPViewGoodsToObjAssoc::PPViewGoodsToObjAssoc() : PPView(0, &Filt, PPVIEW_GOODSTOOBJASSOC)
{
	P_Assoc = 0;
	P_TempTbl = 0;
}

SLAPI PPViewGoodsToObjAssoc::~PPViewGoodsToObjAssoc()
{
	delete P_Assoc;
	delete P_TempTbl;
	DBRemoveTempFiles(); //turistti
	if(Filt.Flags & GoodsToObjAssocFilt::fDestroyExtraParam && Filt.ExtraParam) {
		free((void *)Filt.ExtraParam);
		Filt.ExtraParam = 0;
	}
}

int SLAPI PPViewGoodsToObjAssoc::SearchItem(PPID goodsID, PPID objID, TempGoodsObjAsscTbl::Rec * pRec)
{
	int    ok = -1;
	if(P_TempTbl) {
		TempGoodsObjAsscTbl::Key0 k0;
		k0.GoodsID = goodsID;
		k0.ObjID = objID;
		ok = SearchByKey(P_TempTbl, 0, &k0, pRec);
	}
	return ok;
}

int SLAPI PPViewGoodsToObjAssoc::UpdateTempTblEntry(uint pos)
{
	int    ok = -1;
	if(P_Assoc && P_TempTbl && pos < P_Assoc->GetCount()) {
		TempGoodsObjAsscTbl::Rec rec;
		if(SearchItem(P_Assoc->at(pos).Key, P_Assoc->at(pos).Val, &rec) > 0) {
			MakeTempRec(pos, &rec);
			THROW_DB(P_TempTbl->updateRecBuf(&rec));
		}
		else {
			MakeTempRec(pos, &rec);
			THROW_DB(P_TempTbl->insertRecBuf(&rec));
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PPViewGoodsToObjAssoc::MakeTempRec(uint i, TempGoodsObjAsscTbl::Rec * pRec)
{
	int    ok = 0;
	if(pRec && P_Assoc) {
		memzero(pRec, sizeof(*pRec));
		SString buf;
		const LAssoc & r_item = P_Assoc->at(i);
		pRec->GoodsID = r_item.Key;
		pRec->ObjID   = r_item.Val;
		P_Assoc->GetKeyName(r_item.Key, buf).CopyTo(pRec->GoodsName, sizeof(pRec->GoodsName));
		GetObjectName(Filt.ObjType, r_item.Val, buf);
		buf.CopyTo(pRec->ObjName, sizeof(pRec->ObjName));
		ok = 1;
	}
	return ok;
}

int SLAPI PPViewGoodsToObjAssoc::EditBaseFilt(PPBaseFilt * pFilt)
{
	return 1;
}

PPBaseFilt * SLAPI PPViewGoodsToObjAssoc::CreateFilt(void * extraPtr) const
{
	PPBaseFilt * p_base_filt = 0;
	if(PPView::CreateFiltInstance(PPFILT_GOODSTOOBJASSOC, &p_base_filt)) {
		GoodsToObjAssocFilt * p_filt = (GoodsToObjAssocFilt *)p_base_filt;
		if(extraPtr) {
			p_filt->AsscType = ((long)extraPtr);
			if(p_filt->AsscType == PPASS_GOODS2WAREPLACE) {
				p_filt->ObjType = PPOBJ_LOCATION;
				/* @v8.1.9
				LocationFilt * p_locf = (LocationFilt *)malloc(sizeof(LocationFilt));
				memzero(p_locf, sizeof(*p_locf));
				p_filt->ExtraParam = (long)p_locf;
				p_filt->Flags |= GoodsToObjAssocFilt::fDestroyExtraParam;
				*/
				p_filt->P_LocF = new LocationFilt(LOCTYP_WAREPLACE); // @v8.1.9
			}
			else if(p_filt->AsscType == PPASS_GOODS2LOC) {
				p_filt->ObjType = PPOBJ_LOCATION;
			}
			else if(p_filt->AsscType == PPASS_GOODS2SUPPL) {
				p_filt->ObjType = PPOBJ_ARTICLE;
				p_filt->ExtraParam = GetSupplAccSheet();
			}
			else if(p_filt->AsscType == PPASS_GOODS2CASHNODE) {
				p_filt->ObjType = PPOBJ_CASHNODE;
			}
			else {
				PPObjNamedObjAssoc noa_obj;
				PPNamedObjAssoc noa_rec;
				if(noa_obj.Search(p_filt->AsscType, &noa_rec) > 0) {
					if(noa_rec.PrmrObjType == PPOBJ_GOODS) {
						p_filt->ObjType = noa_rec.ScndObjType;
						if(p_filt->ObjType == PPOBJ_LOCATION && noa_rec.ScndObjGrp) {
							/* @v8.1.9
							LocationFilt * p_locf = (LocationFilt *)malloc(sizeof(LocationFilt));
							memzero(p_locf, sizeof(*p_locf));
							p_locf->LocType = noa_rec.ScndObjGrp;
							p_filt->ExtraParam = (long)p_locf;
							p_filt->Flags |= GoodsToObjAssocFilt::fDestroyExtraParam;
							*/
							p_filt->P_LocF = new LocationFilt(noa_rec.ScndObjGrp); // @v8.1.9
						}
						else
							p_filt->ExtraParam = noa_rec.ScndObjGrp;
					}
					else {
						ZDELETE(p_base_filt);
						PPSetError(PPERR_NAMEDOBJASSCNGOODS);
					}
				}
				else {
					ZDELETE(p_base_filt);
					SString msg_buf;
					msg_buf.Cat(p_filt->AsscType);
					PPSetError(PPERR_UNKGOODSTOOBJASSOC, msg_buf);
				}
			}
		}
	}
	return p_base_filt;
}

PP_CREATE_TEMP_FILE_PROC(CreateTempFile, TempGoodsObjAssc);

int SLAPI PPViewGoodsToObjAssoc::Init_(const PPBaseFilt * pBaseFilt)
{
	int    ok = 1;
	PPObject * p_obj = 0;
	THROW(Helper_InitBaseFilt(pBaseFilt));
	THROW(p_obj = GetPPObject(Filt.ObjType, 0));
	ZDELETE(P_TempTbl);
	THROW_MEM(P_Assoc = new GoodsToObjAssoc(Filt.AsscType, Filt.ObjType));
	THROW(P_Assoc->Load());
	THROW(P_TempTbl = CreateTempFile());
	{
		BExtInsert bei(P_TempTbl);
		for(uint i = 0; i < P_Assoc->GetCount(); i++) {
			TempGoodsObjAsscTbl::Rec rec;
			THROW(MakeTempRec(i, &rec));
			THROW_DB(bei.insert(&rec));
		}
		THROW_DB(bei.flash());
	}
	CATCHZOK
	delete p_obj;
	return ok;
}

int SLAPI PPViewGoodsToObjAssoc::InitIteration()
{
	ZDELETE(P_IterQuery);
	Counter.Init();

	int    ok = 1;
	if(P_TempTbl) {
		TempGoodsObjAsscTbl::Key1 k1;
		MEMSZERO(k1);
		P_IterQuery = new BExtQuery(P_TempTbl, 1);
		P_IterQuery->selectAll();
		Counter.Init(P_TempTbl);
		P_IterQuery->initIteration(0, &k1, spFirst);
	}
	else
		ok = 0;
	return ok;
}

int SLAPI PPViewGoodsToObjAssoc::NextIteration(GoodsToObjAssocViewItem * pItem)
{
	if(P_IterQuery && P_IterQuery->nextIteration() > 0) {
		if(pItem) {
			pItem->GoodsID = P_TempTbl->data.GoodsID;
			pItem->ObjID = P_TempTbl->data.ObjID;
			STRNSCPY(pItem->GoodsName, P_TempTbl->data.GoodsName);
			STRNSCPY(pItem->ObjName, P_TempTbl->data.ObjName);
		}
		Counter.Increment();
		return 1;
	}
	return -1;
}

DBQuery * SLAPI PPViewGoodsToObjAssoc::CreateBrowserQuery(uint * pBrwId, SString * pSubTitle)
{
	uint   brw_id = BROWSER_GOODSTOOBJASSC;
	TempGoodsObjAsscTbl * p_t = 0;
	DBQuery * q = 0;
	if(P_TempTbl) {
		p_t = new TempGoodsObjAsscTbl(P_TempTbl->fileName);
		q = &select(
			p_t->GoodsID,   // #00
			p_t->ObjID,     // #01
			p_t->GoodsName, // #02
			p_t->ObjName,   // #03
			0L).from(p_t, 0L).orderBy(p_t->GoodsName, 0L);
	}
	if(pSubTitle) {
		SString temp_buf, obj_name;
		GetObjectTitle(Filt.ObjType, temp_buf);
		if(Filt.AsscType > 1000) {
			GetObjectName(PPOBJ_NAMEDOBJASSOC, Filt.AsscType, obj_name);
			temp_buf.Space().CatParStr(obj_name);
		}
		*pSubTitle = temp_buf;
	}
	ASSIGN_PTR(pBrwId, brw_id);
	return q;
}

int SLAPI PPViewGoodsToObjAssoc::EditGoodsToObjAssoc(LAssoc * pData, PPID objType, long extraParam, int newItem)
{
	int    ok = -1;
	PPObjGoods goods_obj;
	Goods2Tbl::Rec goods_rec;
	TDialog * dlg = new TDialog(DLG_G2OA);
	if(CheckDialogPtr(&dlg, 1)) {
		SString obj_title;
		dlg->setSubTitle(GetObjectTitle(objType, obj_title));
		dlg->setLabelText(CTL_G2OA_OBJ, obj_title);
		dlg->addGroup(1, new GoodsCtrlGroup(CTLSEL_G2OA_GOODSGRP, CTLSEL_G2OA_GOODS));
		GoodsCtrlGroup::Rec rec;
		if(pData->Key && goods_obj.Fetch(pData->Key, &goods_rec) > 0)
			if(goods_rec.Kind == PPGDSK_GROUP)
				rec.GrpID = goods_rec.ID;
			else
				rec.GoodsID = goods_rec.ID;
		rec.Flags |= GoodsCtrlGroup::enableSelUpLevel;
		dlg->setGroupData(1, &rec);
		if(objType == PPOBJ_LOCATION && Filt.P_LocF)
			SetupLocationCombo(dlg, CTLSEL_G2OA_OBJ, pData->Val, OLW_CANSELUPLEVEL, Filt.P_LocF);
		else
			SetupPPObjCombo(dlg, CTLSEL_G2OA_OBJ, objType, pData->Val, OLW_CANSELUPLEVEL, (void *)extraParam);
		if(!newItem)
			dlg->disableCtrls(1, CTLSEL_G2OA_GOODSGRP, CTLSEL_G2OA_GOODS, 0);
		for(int valid_data = 0; !valid_data && ExecView(dlg) == cmOK;) {
			if(dlg->getGroupData(1, &rec)) {
				if(!rec.GoodsID && !rec.GrpID)
					PPErrorByDialog(dlg, CTLSEL_G2OA_GOODSGRP, PPERR_GOODSNEEDED);
				else {
					pData->Key = NZOR(rec.GoodsID, rec.GrpID);
					pData->Val = dlg->getCtrlLong(CTLSEL_G2OA_OBJ);
					if(pData->Val)
						ok = valid_data = 1;
					else
						PPErrorByDialog(dlg, CTLSEL_G2OA_OBJ, PPERR_USERINPUT);
				}
			}
			else
				PPError();
		}
	}
	delete dlg;
	return ok;
}

int SLAPI PPViewGoodsToObjAssoc::AddItem(PPViewBrowser * pBrw)
{
	int    ok = -1;
	LAssoc assc;
	MEMSZERO(assc);
	while(EditGoodsToObjAssoc(&assc, Filt.ObjType, Filt.ExtraParam, 1) > 0) {
		uint pos = 0;
		if(!P_Assoc->Add(assc.Key, assc.Val, &pos) || !P_Assoc->Save())
			PPError();
		else if(!UpdateTempTblEntry(pos))
			PPError();
		else {
			if(pBrw)
				pBrw->Update();
			ok = 1;
		}
	}
	return ok;
}

int SLAPI PPViewGoodsToObjAssoc::EditItem(const BrwHdr * pHdr)
{
	int    ok = -1;
	if(pHdr && P_Assoc && P_TempTbl) {
		TempGoodsObjAsscTbl::Rec rec;
		if(P_Assoc->Get(pHdr->GoodsID, 0) > 0 && SearchItem(pHdr->GoodsID, pHdr->ObjID, &rec) > 0) {
			LAssoc assc;
			assc.Key = rec.GoodsID;
			assc.Val = rec.ObjID;
			while(ok < 0 && EditGoodsToObjAssoc(&assc, Filt.ObjType, Filt.ExtraParam, 0) > 0) {
				uint   pos = 0;
				if(P_Assoc->SearchPair(assc.Key, assc.Val, &pos)) {
					if(!P_Assoc->UpdateByPos(pos, assc.Key, assc.Val) || !P_Assoc->Save())
						PPError();
					else if(!UpdateTempTblEntry(pos))
						PPError();
					else
						ok = 1;
				}
			}
		}
	}
	return ok;
}

int SLAPI PPViewGoodsToObjAssoc::DeleteItem(const BrwHdr * pHdr)
{
	int    ok = -1;
	if(pHdr && P_Assoc && P_TempTbl) {
		TempGoodsObjAsscTbl::Rec rec;
		if(P_Assoc->Get(pHdr->GoodsID, 0) > 0 && SearchItem(pHdr->GoodsID, pHdr->ObjID, &rec) > 0)
			if(!P_Assoc->Remove(pHdr->GoodsID, pHdr->ObjID) || !P_Assoc->Save() || !P_TempTbl->deleteRec())
				ok = (PPError(PPERR_DBENGINE, 0), 0);
			else
				ok = 1;
	}
	return ok;
}

int SLAPI PPViewGoodsToObjAssoc::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	if(ok == -2) {
		BrwHdr hdr;
		MEMSZERO(hdr);
		if(pHdr)
			hdr = *(BrwHdr *)pHdr;
		switch(ppvCmd) {
			case PPVCMD_ADDITEM:
				ok = AddItem(pBrw);
				break;
			case PPVCMD_EDITITEM:
				ok = EditItem(&hdr);
				break;
			case PPVCMD_DELETEITEM:
				ok = DeleteItem(&hdr);
				break;
			case PPVCMD_EDITGOODS:
				ok = -1;
				if(hdr.GoodsID){
					PPObjGoods goods_obj;
					if(goods_obj.Edit(&hdr.GoodsID, 0) == cmOK)
						ok = 1;
				}
				break;
			case PPVCMD_EDITOBJ:
				ok = EditPPObj(Filt.ObjType, hdr.ObjID);
				break;
		}
	}
	return ok;
}

int SLAPI ViewGoodsToObjAssoc(PPID objType, PPID objID, PPID asscType, long extraParam, long options)
{
	GoodsToObjAssocFilt flt;
	flt.ObjType = objType;
	flt.ObjID   = objID;
	flt.AsscType = asscType;
	flt.ExtraParam = extraParam;
	SETFLAG(flt.Flags, GoodsToObjAssocFilt::fDestroyExtraParam, options & goafFreeExtraAsPtr);
	return PPView::Execute(PPVIEW_GOODSTOOBJASSOC, &flt, 1, 0);
}

int SLAPI ViewGoodsToLocAssoc(PPID locID, PPID asscType, const LocationFilt * pLocFilt, long options)
{
	GoodsToObjAssocFilt flt;
	flt.ObjType = PPOBJ_LOCATION;
	flt.ObjID   = locID;
	flt.AsscType = asscType;
	if(pLocFilt) {
		flt.P_LocF = new LocationFilt;
		*flt.P_LocF = *pLocFilt;
	}
	return PPView::Execute(PPVIEW_GOODSTOOBJASSOC, &flt, 1, 0);
}

int SLAPI ViewGoodsToObjAssoc(long extraParam)
{
	return PPView::Execute(PPVIEW_GOODSTOOBJASSOC, 0, 1, (void *)extraParam);
}
//
//
//
SLAPI PPObjNamedObjAssoc::PPObjNamedObjAssoc(void * extraPtr) : PPObjReference(PPOBJ_NAMEDOBJASSOC, extraPtr)
{
}

class NamedObjAssocDialog : public TDialog {
public:
	NamedObjAssocDialog() : TDialog(DLG_NOBJASSC)
	{
	}
	int    setDTS(const PPNamedObjAssoc * pData);
	int    getDTS(PPNamedObjAssoc * pData);
private:
	DECL_HANDLE_EVENT
	{
		TDialog::handleEvent(event);
		if(event.isCbSelected(CTLSEL_NOBJASSC_SCND)) {
			PPID prev_scnd = Data.ScndObjType;
			Data.ScndObjType = getCtrlLong(CTLSEL_NOBJASSC_SCND);
			if(Data.ScndObjType != prev_scnd) {
				Data.ScndObjGrp = 0;
				SetupScndObjGrp();
			}
			clearEvent(event);
		}
	}
	void   SetupScndObjGrp();
	PPNamedObjAssoc Data;
};

void NamedObjAssocDialog::SetupScndObjGrp()
{
	int    dsbl_grp = 0;
	if(Data.ScndObjType == PPOBJ_LOCATION) {
		SetupStringCombo(this, CTLSEL_NOBJASSC_SCNDGRP, PPTXT_LOCTYPE, Data.ScndObjGrp);
	}
	else if(Data.ScndObjType == PPOBJ_ARTICLE) {
		SetupPPObjCombo(this, CTLSEL_NOBJASSC_SCNDGRP, PPOBJ_ACCSHEET, Data.ScndObjGrp, 0, 0);
	}
	else if(Data.ScndObjType == PPOBJ_PERSON) {
		disableCtrl(CTLSEL_NOBJASSC_SCNDGRP, 0);
		SetupPPObjCombo(this, CTLSEL_NOBJASSC_SCNDGRP, PPOBJ_PRSNKIND, Data.ScndObjGrp, 0, 0);
	}
	else
		dsbl_grp = 1;
	disableCtrl(CTLSEL_NOBJASSC_SCNDGRP, dsbl_grp);
}

int NamedObjAssocDialog::setDTS(const PPNamedObjAssoc * pData)
{
	int    ok = 1;
	Data = *pData;

	PPIDArray obj_type_list;
	obj_type_list.addzlist(PPOBJ_GOODS, 0L);
	SetupObjListCombo(this, CTLSEL_NOBJASSC_PRMR, Data.PrmrObjType, &obj_type_list);
	obj_type_list.clear();
	obj_type_list.addzlist(PPOBJ_LOCATION, PPOBJ_ARTICLE, 0);
	SetupObjListCombo(this, CTLSEL_NOBJASSC_SCND, Data.ScndObjType, &obj_type_list);
	setCtrlData(CTL_NOBJASSC_NAME, Data.Name);
	setCtrlData(CTL_NOBJASSC_SYMB, Data.Symb);
	setCtrlLong(CTL_NOBJASSC_ID, Data.ID);
	disableCtrl(CTL_NOBJASSC_ID, 1);
	SetupScndObjGrp();
	return ok;
}

int NamedObjAssocDialog::getDTS(PPNamedObjAssoc * pData)
{
	int    ok = 1;
	uint   sel = 0;
	PPObjNamedObjAssoc noa_obj;
	getCtrlData(sel = CTL_NOBJASSC_NAME, Data.Name);
	THROW(noa_obj.CheckName(Data.ID, Data.Name, 1));
	getCtrlData(sel = CTL_NOBJASSC_SYMB, Data.Symb);
	THROW(PPRef->CheckUniqueSymb(PPOBJ_NAMEDOBJASSOC, Data.ID, Data.Symb, offsetof(PPNamedObjAssoc, Symb)));
	getCtrlData(sel = CTLSEL_NOBJASSC_PRMR, &Data.PrmrObjType);
	THROW_PP(Data.PrmrObjType, PPERR_OBJTYPENEEDED);
	getCtrlData(sel = CTLSEL_NOBJASSC_SCND, &Data.ScndObjType);
	THROW_PP(Data.ScndObjType, PPERR_OBJTYPENEEDED);
	getCtrlData(CTLSEL_NOBJASSC_SCNDGRP, &Data.ScndObjGrp);
	ASSIGN_PTR(pData, Data);
	CATCH
		ok = PPErrorByDialog(this, sel);
	ENDCATCH
	return ok;
}

int SLAPI PPObjNamedObjAssoc::Edit(PPID * pID, void * extraPtr)
{
	int    ok = cmCancel;
	PPNamedObjAssoc rec;
	NamedObjAssocDialog * dlg = new NamedObjAssocDialog();
	THROW(CheckDialogPtr(&dlg, 0));
	if(*pID) {
		THROW(Search(*pID, &rec) > 0);
	}
	else
		MEMSZERO(rec);
	dlg->setDTS(&rec);
	while(ok == cmCancel && ExecView(dlg) == cmOK) {
		if(dlg->getDTS(&rec)) {
			if(*pID)
				*pID = rec.ID;
			if(EditItem(Obj, *pID, &rec, 1)) {
				*pID = rec.ID;
				ok = cmOK;
			}
			else
				PPError();
		}
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}

int SLAPI PPObjNamedObjAssoc::Browse(void * extraPtr)
{
	class NamedObjAssocView : public ObjViewDialog {
	public:
		NamedObjAssocView(PPObjNamedObjAssoc * pObj) : ObjViewDialog(DLG_NOBJASSCVIEW, pObj, 0)
		{
		}
	private:
		virtual void extraProc(long id)
		{
			if(id)
				ViewGoodsToObjAssoc(id);
		}
	};
	int    ok = 1;
	if(CheckRights(PPR_READ)) {
		TDialog * dlg = new NamedObjAssocView(this);
		if(CheckDialogPtr(&dlg, 1))
			ExecViewAndDestroy(dlg);
		else
			ok = 0;
	}
	else
		ok = PPErrorZ();
	return ok;
}
