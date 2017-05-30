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

const LAssoc & FASTCALL GoodsToObjAssoc::at(uint pos) const
{
	return List.at(pos);
}

SString & SLAPI GoodsToObjAssoc::GetKeyName(PPID id, SString & rBuf)
{
	Goods2Tbl::Rec goods_rec;
	rBuf = 0;
	if(GObj.Fetch(id, &goods_rec) > 0) {
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

int SLAPI GoodsToObjAssoc::Search(PPID goodsID, PPID * pObjID, uint * pPos) const
{
	return List.Search(goodsID, pObjID, pPos);
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
		else if(List.BSearch(temp_id, &obj_id, 0))
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
	P_AsscObj = 0;
	ImplementFlags |= implBrowseArray;
}

SLAPI PPViewGoodsToObjAssoc::~PPViewGoodsToObjAssoc()
{
	delete P_Assoc;
	delete P_AsscObj;
	if(Filt.Flags & GoodsToObjAssocFilt::fDestroyExtraParam && Filt.ExtraParam) {
		SAlloc::F((void *)Filt.ExtraParam);
		Filt.ExtraParam = 0;
	}
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
				p_filt->P_LocF = new LocationFilt(LOCTYP_WAREPLACE);
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
							p_filt->P_LocF = new LocationFilt(noa_rec.ScndObjGrp);
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

int SLAPI PPViewGoodsToObjAssoc::Init_(const PPBaseFilt * pBaseFilt)
{
	int    ok = 1;
	ZDELETE(P_AsscObj);
	THROW(Helper_InitBaseFilt(pBaseFilt));
	THROW(P_AsscObj = GetPPObject(Filt.ObjType, 0));
	THROW_MEM(P_Assoc = new GoodsToObjAssoc(Filt.AsscType, Filt.ObjType));
	THROW(P_Assoc->Load());
	CATCHZOK
	return ok;
}

int SLAPI PPViewGoodsToObjAssoc::InitIteration()
{
	int    ok = 1;
	Counter.Init(P_Assoc ? P_Assoc->GetCount() : 0);
	IterIdx = 0;
	return ok;
}

int FASTCALL PPViewGoodsToObjAssoc::NextIteration(GoodsToObjAssocViewItem * pItem)
{
	int    ok = -1;
	if(P_Assoc && IterIdx < P_Assoc->GetCount()) {
		const LAssoc & r_item = P_Assoc->at(IterIdx);
		if(pItem) {
			SString temp_buf;
			memzero(pItem, sizeof(*pItem));
			pItem->GoodsID = r_item.Key;
			pItem->ObjID = r_item.Val;
			P_Assoc->GetKeyName(r_item.Key, temp_buf);
			STRNSCPY(pItem->GoodsName, temp_buf);
			GetObjectName(Filt.ObjType, r_item.Val, temp_buf = 0);
			STRNSCPY(pItem->ObjName, temp_buf);
		}
		IterIdx++;
		Counter.Increment();
		ok = 1;
	}
	return ok;
}

//static 
int SLAPI PPViewGoodsToObjAssoc::GetDataForBrowser(SBrowserDataProcBlock * pBlk)
{
	PPViewGoodsToObjAssoc * p_v = (PPViewGoodsToObjAssoc *)pBlk->ExtraPtr;
	return p_v ? p_v->_GetDataForBrowser(pBlk) : 0;
}

int SLAPI PPViewGoodsToObjAssoc::_GetDataForBrowser(SBrowserDataProcBlock * pBlk)
{
	int    ok = 0;
	if(pBlk->P_SrcData && pBlk->P_DestData) {
		ok = 1;
		SString temp_buf;
		const LAssoc * p_item = (LAssoc *)pBlk->P_SrcData;
		switch(pBlk->ColumnN) {
			case 0: // GoodsID
				pBlk->Set(p_item->Key);
				break;
			case 1: // ObjID
				pBlk->Set(p_item->Val);
				break;
			case 2: // Наименование товара
				if(P_Assoc) {
					P_Assoc->GetKeyName(p_item->Key, temp_buf);
				}
				if(temp_buf.Len() == 0)
					ideqvalstr(p_item->Key, temp_buf);
				pBlk->Set(temp_buf);
				break;
			case 3: // Наименование ассоциированного объекта
				GetObjectName(Filt.ObjType, p_item->Val, temp_buf);
				pBlk->Set(temp_buf);
				break;
		}
	}
	return ok;
}

int SLAPI PPViewGoodsToObjAssoc::PreprocessBrowser(PPViewBrowser * pBrw)
{
	int    ok = -1;
	if(pBrw) {
		pBrw->SetDefUserProc(PPViewGoodsToObjAssoc::GetDataForBrowser, this);
		ok = 1;
	}
	return ok;
}

SArray * SLAPI PPViewGoodsToObjAssoc::CreateBrowserArray(uint * pBrwId, SString * pSubTitle)
{
	uint   brw_id = BROWSER_GOODSTOOBJASSC;
	SArray * p_array = P_Assoc ? new SArray(*P_Assoc) : 0;
	ASSIGN_PTR(pBrwId, brw_id);
	if(pSubTitle) {
		SString obj_name, temp_buf;
		GetObjectTitle(Filt.ObjType, temp_buf);
		if(Filt.AsscType > 1000) {
			GetObjectName(PPOBJ_NAMEDOBJASSOC, Filt.AsscType, obj_name);
			temp_buf.Space().CatParStr(obj_name);
		}
		*pSubTitle = temp_buf;
	}
	return p_array;
}

int SLAPI PPViewGoodsToObjAssoc::EditGoodsToObjAssoc(LAssoc * pData, PPID objType, long extraParam, int newItem)
{
	int    ok = -1;
	PPObjGoods goods_obj;
	Goods2Tbl::Rec goods_rec;
	TDialog * dlg = new TDialog(DLG_G2OA);
	if(CheckDialogPtrErr(&dlg)) {
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
		else {
			if(pBrw) {
				UpdateOnEdit(pBrw);
				pBrw->Update();
			}
			ok = 1;
		}
	}
	return ok;
}

int SLAPI PPViewGoodsToObjAssoc::EditItem(PPViewBrowser * pBrw, const BrwHdr * pHdr)
{
	int    ok = -1;
	if(pHdr && P_Assoc) {
		PPID   obj_id = 0;
		if(P_Assoc->Get(pHdr->GoodsID, &obj_id) > 0) {
			LAssoc assc;
			assc.Key = pHdr->GoodsID;
			assc.Val = pHdr->ObjID;
			while(ok < 0 && EditGoodsToObjAssoc(&assc, Filt.ObjType, Filt.ExtraParam, 0) > 0) {
				uint   pos = 0;
				if(P_Assoc->SearchPair(assc.Key, assc.Val, &pos)) {
					if(!P_Assoc->UpdateByPos(pos, assc.Key, assc.Val) || !P_Assoc->Save())
						PPError();
					else {
						UpdateOnEdit(pBrw);
						ok = 1;
					}
				}
				else {
					long   ex_val = 0;
					pos = 0;
					if(P_Assoc->Search(assc.Key, &ex_val, &pos) && ex_val != assc.Val) {
						if(!P_Assoc->UpdateByPos(pos, assc.Key, assc.Val) || !P_Assoc->Save())
							PPError();
						else {
							UpdateOnEdit(pBrw);
							ok = 1;
						}
					}
				}
			}
		}
	}
	return ok;
}

int SLAPI PPViewGoodsToObjAssoc::DeleteItem(const BrwHdr * pHdr)
{
	int    ok = -1;
	if(pHdr && P_Assoc) {
		if(P_Assoc->Get(pHdr->GoodsID, 0) > 0)
			if(!P_Assoc->Remove(pHdr->GoodsID, pHdr->ObjID) || !P_Assoc->Save())
				ok = PPError(PPERR_DBENGINE);
			else
				ok = 1;
	}
	return ok;
}

void SLAPI PPViewGoodsToObjAssoc::UpdateOnEdit(PPViewBrowser * pBrw)
{
	if(pBrw) {
		AryBrowserDef * p_def = (AryBrowserDef *)pBrw->getDef();
		if(p_def) {
			SArray * p_array = P_Assoc ? new SArray(*P_Assoc) : 0;
			p_def->setArray(p_array, 0, 0/*setupPosition*/);
		}
	}
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
				ok = EditItem(pBrw, &hdr);
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
	if(ok > 0) {
		if(!oneof2(ppvCmd, PPVCMD_ADDITEM, PPVCMD_EDITITEM))
			UpdateOnEdit(pBrw);
		ok = 1;
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
	THROW(CheckDialogPtr(&dlg));
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
		if(CheckDialogPtrErr(&dlg))
			ExecViewAndDestroy(dlg);
		else
			ok = 0;
	}
	else
		ok = PPErrorZ();
	return ok;
}
