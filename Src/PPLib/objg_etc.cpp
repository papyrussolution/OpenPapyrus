// OBJG_ETC.CPP
// Copyright (c) A.Sobolev 2002, 2003, 2005, 2007, 2008, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022
// @codepage UTF-8
// Дополнительные классы инфраструктуры управления товарами
//
#include <pp.h>
#pragma hdrstop
#include <charry.h>
//
// @ModuleDef(PPObjGoodsType)
//
PPGoodsType2::PPGoodsType2()
{
	THISZERO();
}

PPObjGoodsType::PPObjGoodsType(void * extraPtr) : PPObjReference(PPOBJ_GOODSTYPE, extraPtr)
{
}

int PPObjGoodsType::HandleMsg(int msg, PPID _obj, PPID _id, void * extraPtr)
{
	if(msg == DBMSG_OBJDELETE) {
		if(_obj == PPOBJ_AMOUNTTYPE && _id) {
			PPGoodsType rec;
			for(PPID id = 0; EnumItems(&id, &rec) > 0;)
				if(oneof4(_id, rec.AmtCost, rec.AmtPrice, rec.AmtDscnt, rec.AmtCVat))
					return RetRefsExistsErr(Obj, id);
		}
	}
	return DBRPL_OK;
}

int PPObjGoodsType::Edit(PPID * pID, void * extraPtr)
{
	int    ok = 1;
	int    r = cmCancel;
	int    valid_data = 0;
	int    is_new = 0;
	PPGoodsType rec;
	TDialog * dlg = 0;
	THROW(CheckDialogPtr(&(dlg = new TDialog(DLG_GDSTYP))));
	THROW(EditPrereq(pID, dlg, &is_new));
	if(!is_new) {
		THROW(Search(*pID, &rec) > 0);
	}
	// @v10.7.5 @ctr else MEMSZERO(rec);
	dlg->setCtrlData(CTL_GDSTYP_NAME, rec.Name);
	dlg->setCtrlData(CTL_GDSTYP_SYMB, rec.Symb);
	dlg->setCtrlData(CTL_GDSTYP_ID,   &rec.ID);
	dlg->disableCtrl(CTL_GDSTYP_ID,   (!PPMaster || rec.ID));
	SetupPPObjCombo(dlg, CTLSEL_GDSTYP_COST,     PPOBJ_AMOUNTTYPE, rec.AmtCost,  OLW_CANINSERT, 0);
	SetupPPObjCombo(dlg, CTLSEL_GDSTYP_PRICE,    PPOBJ_AMOUNTTYPE, rec.AmtPrice, OLW_CANINSERT, 0);
	SetupPPObjCombo(dlg, CTLSEL_GDSTYP_DISCOUNT, PPOBJ_AMOUNTTYPE, rec.AmtDscnt, OLW_CANINSERT, 0);
	SetupPPObjCombo(dlg, CTLSEL_GDSTYP_CVAT,     PPOBJ_AMOUNTTYPE, rec.AmtCVat,  OLW_CANINSERT, 0);
	//SetupPPObjCombo(dlg, CTLSEL_GDSTYP_AWOG,     PPOBJ_ASSTWROFFGRP, rec.WrOffGrpID, OLW_CANINSERT, 0);
	SetupPPObjCombo(dlg, CTLSEL_GDSTYP_PRICERESTR, PPOBJ_GOODSVALRESTR, rec.PriceRestrID, OLW_CANINSERT, 0);
	SetupStringCombo(dlg, CTLSEL_GDSTYP_CHZNPT, PPTXT_CHZNPRODUCTTYPES, rec.ChZnProdType); // @v10.7.2
	dlg->AddClusterAssoc(CTL_GDSTYP_UNLIM, 0, GTF_UNLIMITED);
	dlg->AddClusterAssoc(CTL_GDSTYP_UNLIM, 1, GTF_AUTOCOMPL);
	dlg->AddClusterAssoc(CTL_GDSTYP_UNLIM, 2, GTF_ASSETS);
	dlg->AddClusterAssoc(CTL_GDSTYP_UNLIM, 3, GTF_ADVANCECERT); // @v10.4.1
	dlg->SetClusterData(CTL_GDSTYP_UNLIM, rec.Flags);

	dlg->AddClusterAssoc(CTL_GDSTYP_FLAGS,  0, GTF_RPLC_COST);
	dlg->AddClusterAssoc(CTL_GDSTYP_FLAGS,  1, GTF_RPLC_PRICE);
	dlg->AddClusterAssoc(CTL_GDSTYP_FLAGS,  2, GTF_RPLC_DSCNT);
	dlg->AddClusterAssoc(CTL_GDSTYP_FLAGS,  3, GTF_PRICEINCLDIS);
	dlg->AddClusterAssoc(CTL_GDSTYP_FLAGS,  4, GTF_EXCLAMOUNT);
	dlg->AddClusterAssoc(CTL_GDSTYP_FLAGS,  5, GTF_ALLOWZEROPRICE);
	dlg->AddClusterAssoc(CTL_GDSTYP_FLAGS,  6, GTF_EXCLVAT);
	dlg->AddClusterAssoc(CTL_GDSTYP_FLAGS,  7, GTF_REQBARCODE);
	dlg->AddClusterAssoc(CTL_GDSTYP_FLAGS,  8, GTF_QUASIUNLIM);
	dlg->AddClusterAssoc(CTL_GDSTYP_FLAGS,  9, GTF_LOOKBACKPRICES);
	dlg->AddClusterAssoc(CTL_GDSTYP_FLAGS, 10, GTF_GMARKED); // @v10.4.11
	dlg->SetClusterData(CTL_GDSTYP_FLAGS, rec.Flags);
	dlg->setCtrlReal(CTL_GDSTYP_STKTLR, rec.StockTolerance);
	while(!valid_data && (r = ExecView(dlg)) == cmOK) {
		THROW(is_new || CheckRights(PPR_MOD));
		dlg->getCtrlData(CTL_GDSTYP_ID,   &rec.ID);
		dlg->getCtrlData(CTL_GDSTYP_NAME, rec.Name);
		dlg->getCtrlData(CTL_GDSTYP_SYMB, rec.Symb);
		if(!CheckName(rec.ID, strip(rec.Name), 1))
			PPErrorByDialog(dlg, CTL_GDSTYP_NAME);
		else if(!P_Ref->CheckUniqueSymb(Obj, rec.ID, strip(rec.Symb), offsetof(ReferenceTbl::Rec, Symb))) {
			PPErrorByDialog(dlg, CTL_GDSTYP_SYMB);
		}
		else {
			valid_data = 1;
			dlg->getCtrlData(CTLSEL_GDSTYP_COST,     &rec.AmtCost);
			dlg->getCtrlData(CTLSEL_GDSTYP_PRICE,    &rec.AmtPrice);
			dlg->getCtrlData(CTLSEL_GDSTYP_DISCOUNT, &rec.AmtDscnt);
			dlg->getCtrlData(CTLSEL_GDSTYP_CVAT,     &rec.AmtCVat);
			//dlg->getCtrlData(CTLSEL_GDSTYP_AWOG,   &rec.WrOffGrpID);
			dlg->getCtrlData(CTLSEL_GDSTYP_PRICERESTR, &rec.PriceRestrID);
			dlg->getCtrlData(CTLSEL_GDSTYP_CHZNPT, &rec.ChZnProdType); // @v10.7.2
			dlg->GetClusterData(CTL_GDSTYP_UNLIM, &rec.Flags);
			dlg->GetClusterData(CTL_GDSTYP_FLAGS, &rec.Flags);
			rec.StockTolerance = dlg->getCtrlReal(CTL_GDSTYP_STKTLR);
			if(*pID)
				*pID = rec.ID;
			THROW(StoreItem(PPOBJ_GOODSTYPE, *pID, &rec, 1));
			Dirty(*pID);
		}
	}
	CATCHZOKPPERR
	delete dlg;
	return ok ? r : 0;
}

int PPObjGoodsType::Browse(void * extraPtr) { return RefObjView(this, PPDS_CRRGOODSTYPE, 0); }
//
//
//
class GoodsTypeCache : public ObjCache {
public:
	GoodsTypeCache() : ObjCache(PPOBJ_GOODSTYPE, sizeof(GoodsTypeData)) {}
private:
	virtual int FetchEntry(PPID id, ObjCacheEntry * pEntry, long)
	{
		int    ok = 1;
		GoodsTypeData * p_cache_rec = static_cast<GoodsTypeData *>(pEntry);
		PPObjGoodsType gt_obj;
		PPGoodsType rec;
		if(gt_obj.Search(id, &rec) > 0) {
			#define FLD(f) p_cache_rec->f = rec.f
			FLD(PriceRestrID);
			FLD(WrOffGrpID);
			FLD(AmtCost);
			FLD(AmtPrice);
			FLD(AmtDscnt);
			FLD(AmtCVat);
			FLD(ChZnProdType); // @v10.7.2
			FLD(Flags);
			#undef FLD
			/* @v10.4.2 ok =*/PutName(rec.Name, p_cache_rec);
		}
		else
			ok = -1;
		return ok;
	}
	virtual void EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const
	{
		PPGoodsType * p_data_rec = static_cast<PPGoodsType *>(pDataRec);
		const GoodsTypeData * p_cache_rec = static_cast<const GoodsTypeData *>(pEntry);
		memzero(p_data_rec, sizeof(*p_data_rec));
		p_data_rec->Tag   = PPOBJ_GOODSTYPE;
		#define FLD(f) p_data_rec->f = p_cache_rec->f
		FLD(ID);
		FLD(PriceRestrID);
		FLD(WrOffGrpID);
		FLD(AmtCost);
		FLD(AmtPrice);
		FLD(AmtDscnt);
		FLD(AmtCVat);
		FLD(ChZnProdType); // @v10.7.2
		FLD(Flags);
		#undef FLD
		GetName(pEntry, p_data_rec->Name, sizeof(p_data_rec->Name));
	}
public:
	struct GoodsTypeData : public ObjCacheEntry {
		PPID   PriceRestrID;
		PPID   WrOffGrpID;
		PPID   AmtCost;
		PPID   AmtPrice;
		PPID   AmtDscnt;
		PPID   AmtCVat;
		long   ChZnProdType; // @v10.7.2
		long   Flags;
	};
};

IMPL_OBJ_FETCH(PPObjGoodsType, PPGoodsType, GoodsTypeCache);

int FASTCALL PPObjGoodsType::IsUnlim(PPID id)
{
	PPGoodsType gt_rec;
	return BIN(id && id != PPGT_DEFAULT && Fetch(id, &gt_rec) > 0 && gt_rec.Flags & (GTF_UNLIMITED|GTF_AUTOCOMPL));
}

int PPObjGoodsType::ProcessObjRefs(PPObjPack * p, PPObjIDArray * ary, int replace, ObjTransmContext * pCtx)
{
	int    ok = 1;
	if(p && p->Data) {
		PPGoodsType * p_rec = static_cast<PPGoodsType *>(p->Data);
		THROW(ProcessObjRefInArray(PPOBJ_AMOUNTTYPE, &p_rec->AmtCost,  ary, replace));
		THROW(ProcessObjRefInArray(PPOBJ_AMOUNTTYPE, &p_rec->AmtPrice, ary, replace));
		THROW(ProcessObjRefInArray(PPOBJ_AMOUNTTYPE, &p_rec->AmtDscnt, ary, replace));
		THROW(ProcessObjRefInArray(PPOBJ_AMOUNTTYPE, &p_rec->AmtCVat,  ary, replace));
		THROW(ProcessObjRefInArray(PPOBJ_GOODSVALRESTR, &p_rec->PriceRestrID,  ary, replace)); // @v10.1.6
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}
//
// @ModuleDef(PPObjBarCodeStruc)
//
PPBarcodeStruc2::PPBarcodeStruc2()
{
	THISZERO();
}

PPObjBarCodeStruc::PPObjBarCodeStruc(void * extraPtr) : PPObjReference(PPOBJ_BCODESTRUC, extraPtr)
{
}

int PPObjBarCodeStruc::Edit(PPID * pID, void * extraPtr)
{
	int    ok = 1;
	int    r = cmCancel;
	int    valid_data = 0;
	int    is_new = 0;
	PPBarcodeStruc rec;
	TDialog * dlg = 0;
	THROW(CheckDialogPtr(&(dlg = new TDialog(DLG_BCODESTR))));
	THROW(EditPrereq(pID, dlg, &is_new));
	if(!is_new) {
		THROW(Search(*pID, &rec) > 0);
	}
	// @v10.7.6 @ctr else MEMSZERO(rec);
	dlg->setCtrlData(CTL_BCODESTR_NAME, rec.Name);
	dlg->setCtrlData(CTL_BCODESTR_TEMPL, rec.Templ);
	// @v10.7.6 {
	dlg->AddClusterAssocDef(CTL_BCODESTR_SPC, 0, PPBarcodeStruc::spcNone);
	dlg->AddClusterAssoc(CTL_BCODESTR_SPC, 1, PPBarcodeStruc::spcUhttSync);
	dlg->SetClusterData(CTL_BCODESTR_SPC, rec.Speciality);
	// } @v10.7.6 
	while(!valid_data && (r = ExecView(dlg)) == cmOK) {
		dlg->getCtrlData(CTL_BCODESTR_NAME, rec.Name);
		if(*strip(rec.Name) == 0)
			PPErrorByDialog(dlg, CTL_BCODESTR_NAME, PPERR_NAMENEEDED);
		else {
			valid_data = 1;
			dlg->getCtrlData(CTL_BCODESTR_TEMPL, rec.Templ);
			dlg->GetClusterData(CTL_BCODESTR_SPC, &rec.Speciality); // @v10.7.6
			if(*pID)
				*pID = rec.ID;
			THROW(StoreItem(PPOBJ_BCODESTRUC, *pID, &rec, 1));
		}
	}
	CATCHZOKPPERR
	delete dlg;
	return ok ? r : 0;
}

int PPObjBarCodeStruc::Browse(void * extraPtr) { return RefObjView(this, PPDS_CRRBARCODESTRUC, 0); }
//
// @ModuleDef(PPObjGoodsValRestr)
//
PPGoodsValRestrPacket::PPGoodsValRestrPacket()
{
	MEMSZERO(Rec);
}

const ObjRestrictArray & PPGoodsValRestrPacket::GetBillArRestrictList() const { return BillArRestr; }
int PPGoodsValRestrPacket::SetBillArRestr(PPID arID, long option) { return BillArRestr.UpdateItemByID(arID, option); }
int PPGoodsValRestrPacket::RemoveBillArRestr(PPID arID) { return BillArRestr.RemoveItemByID(arID); }
//
//
//
PPObjGoodsValRestr::GvrArray::GvrArray() : TSVector <PPObjGoodsValRestr::GvrItem> ()
{
}

int PPObjGoodsValRestr::GvrArray::TestGvrBillArPair(PPID gvrID, PPID arID) const
	{ return Helper_TestGvrBillArPair(gvrID, arID, 0); }
int PPObjGoodsValRestr::GvrArray::TestGvrBillExtArPair(PPID gvrID, PPID arID) const
	{ return Helper_TestGvrBillArPair(gvrID, arID, 1); }

int PPObjGoodsValRestr::GvrArray::Helper_TestGvrBillArPair(PPID gvrID, PPID arID, int what) const
{
	assert(oneof2(what, 0, 1));
	int    ok = -1;
	int    has_this_gvr = 0;
	int    has_only_restrict = 0;
	for(uint i = 0; ok < 0 && i < getCount(); i++) {
		const GvrItem & r_item = at(i);
		if(r_item.GvrID == gvrID) {
			has_this_gvr = 1;
			const int only_rule = (what == 0) ? BIN(r_item.GvrBarOption == PPGoodsValRestrPacket::barMainArOnly) : BIN(r_item.GvrBarOption == PPGoodsValRestrPacket::barExtArOnly);
			const int dsbl_rule = (what == 0) ? BIN(r_item.GvrBarOption == PPGoodsValRestrPacket::barMainArDisable) : BIN(r_item.GvrBarOption == PPGoodsValRestrPacket::barExtArDisable);
			if(only_rule) {
				has_only_restrict = 1;
				if(arID == r_item.ArID)
					ok = 1;
			}
			else if(dsbl_rule && arID == r_item.ArID) {
				ok = 0;
			}
		}
	}
	if(ok < 0) {
		if(!has_this_gvr)
			ok = 1;
		else if(has_only_restrict)
			ok = 0;
		else
			ok = 1;
	}
	return ok;
}
//
//
//
PPObjGoodsValRestr::PPObjGoodsValRestr(void * extraPtr) : PPObjReference(PPOBJ_GOODSVALRESTR, extraPtr)
{
}

class GoodsValRestrDialog : public PPListDialog {
	DECL_DIALOG_DATA(PPGoodsValRestrPacket);
public:
	GoodsValRestrDialog() : PPListDialog(DLG_GVR, CTL_GVR_BARLIST)
	{
		SetupInputLine(CTL_GVR_LOWBFORM, MKSTYPE(S_ZSTRING, 1024), MKSFMT(1024, 0)); // @v10.2.8
		SetupInputLine(CTL_GVR_UPPBFORM, MKSTYPE(S_ZSTRING, 1024), MKSFMT(1024, 0)); // @v10.2.8
		updateList(-1);
	}
	DECL_DIALOG_SETDTS()
	{
		RVALUEPTR(Data, pData);
		setCtrlData(CTL_GVR_NAME, Data.Rec.Name);
		setCtrlData(CTL_GVR_SYMB, Data.Rec.Symb);
		setCtrlLong(CTL_GVR_ID,   Data.Rec.ID);
		setCtrlString(CTL_GVR_LOWBFORM, Data.LowBoundFormula);
		setCtrlString(CTL_GVR_UPPBFORM, Data.UppBoundFormula);
		updateList(-1);
		return 1;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		getCtrlData(CTL_GVR_NAME, Data.Rec.Name);
		getCtrlData(CTL_GVR_SYMB, Data.Rec.Symb);
		getCtrlData(CTL_GVR_ID, &Data.Rec.ID);
		getCtrlString(CTL_GVR_LOWBFORM, Data.LowBoundFormula);
		getCtrlString(CTL_GVR_UPPBFORM, Data.UppBoundFormula);
		ASSIGN_PTR(pData, Data);
		return ok;
	}
private:
	DECL_HANDLE_EVENT
	{
		PPListDialog::handleEvent(event);
		if(event.isCmd(cmTest)) {
			SString temp_buf;
			getCtrlString(CTL_GVR_LOWBFORM, temp_buf.Z());
			if(temp_buf.NotEmpty() && !PPObjGoodsValRestr::TestFormula(temp_buf)) {
				PPErrorByDialog(this, CTL_GVR_LOWBFORM);
			}
			else {
				getCtrlString(CTL_GVR_UPPBFORM, temp_buf.Z());
				if(temp_buf.NotEmpty() && !PPObjGoodsValRestr::TestFormula(temp_buf)) {
					PPErrorByDialog(this, CTL_GVR_UPPBFORM);
				}
			}
		}
		else if(event.isCmd(cmShipmControl)) {
			EditShipmControlParams();
		}
		else
			return;
		clearEvent(event);
	}
	virtual int setupList();
	virtual int addItem(long * pPos, long * pID);
	virtual int editItem(long pos, long id);
	virtual int delItem(long pos, long id);
	int    EditItem(ObjRestrictItem & rItem);
	int    EditShipmControlParams()
	{
		int    ok = -1;
		double up_dev = 0.0, dn_dev = 0.0;
		PPIDArray op_type_list;
		TDialog * dlg = new TDialog(DLG_SHIPMCTRL);
		THROW(CheckDialogPtr(&dlg));
		op_type_list.addzlist(PPOPT_GOODSEXPEND, PPOPT_GENERIC, 0);
		SetupOprKindCombo(dlg, CTLSEL_SHIPMCTRL_SHIPMOP, Data.Rec.ScpShipmOpID, 0, &op_type_list, 0);
		op_type_list.clear();
		op_type_list.addzlist(PPOPT_GOODSRETURN, PPOPT_GENERIC, PPOPT_GOODSRECEIPT, 0);
		SetupOprKindCombo(dlg, CTLSEL_SHIPMCTRL_RETOP, Data.Rec.ScpRetOpID, 0, &op_type_list, 0);
		{
			op_type_list.clear();
			PPOprKind op_rec;
			for(PPID op_id = 0; EnumOperations(0, &op_id, &op_rec) > 0;) {
				if(op_rec.SubType == OPSUBT_TRADEPLAN && oneof3(op_rec.OpTypeID, PPOPT_DRAFTEXPEND, PPOPT_DRAFTRECEIPT, PPOPT_DRAFTTRANSIT))
					op_type_list.add(op_id);
			}
			SetupOprKindCombo(dlg, CTLSEL_SHIPMCTRL_SHLIMOP, Data.Rec.ScpShipmLimitOpID, 0, &op_type_list, OPKLF_OPLIST);
		}
		dlg->setCtrlLong(CTL_SHIPMCTRL_DURATION, Data.Rec.ScpDurationDays);
		dlg->setCtrlReal(CTL_SHIPMCTRL_UPDEV, Data.Rec.ScpUpDev / 10.0);
		dlg->setCtrlReal(CTL_SHIPMCTRL_DNDEV, Data.Rec.ScpDnDev / 10.0);
		while(ok < 0 && ExecView(dlg) == cmOK) {
			dlg->getCtrlData(CTLSEL_SHIPMCTRL_SHIPMOP, &Data.Rec.ScpShipmOpID);
			dlg->getCtrlData(CTLSEL_SHIPMCTRL_RETOP, &Data.Rec.ScpRetOpID);
			dlg->getCtrlData(CTLSEL_SHIPMCTRL_SHLIMOP, &Data.Rec.ScpShipmLimitOpID);
			Data.Rec.ScpDurationDays = dlg->getCtrlLong(CTL_SHIPMCTRL_DURATION);
			Data.Rec.ScpUpDev = (long)(dlg->getCtrlReal(CTL_SHIPMCTRL_UPDEV) * 10.0);
			Data.Rec.ScpDnDev = (long)(dlg->getCtrlReal(CTL_SHIPMCTRL_DNDEV) * 10.0);
			ok = 1;
		}
		CATCHZOKPPERR
		delete dlg; // @v10.7.5 @fix
		return ok;
	}
};

int GoodsValRestrDialog::setupList()
{
	int    ok = 1;
	SString temp_buf;
	StringSet ss(SLBColumnDelim);
	const ObjRestrictArray & r_list = Data.GetBillArRestrictList();
	for(uint i = 0; i < r_list.getCount(); i++) {
		const ObjRestrictItem & r_item = r_list.at(i);
		ss.clear();
		GetArticleName(r_item.ObjID, temp_buf.Z());
		ss.add(temp_buf);
		{
			switch(r_item.Flags) {
				case PPGoodsValRestrPacket::barMainArOnly:    PPLoadString("gvrbar_mainaronly", temp_buf); break;
				case PPGoodsValRestrPacket::barMainArDisable: PPLoadString("gvrbar_mainardisable", temp_buf); break;
				case PPGoodsValRestrPacket::barExtArOnly:     PPLoadString("gvrbar_extaronly", temp_buf); break;
				case PPGoodsValRestrPacket::barExtArDisable:  PPLoadString("gvrbar_extardisable", temp_buf); break;
				default: temp_buf.Z().Cat(r_item.Flags); break;
			}
			ss.add(temp_buf);
		}
		THROW(addStringToList(r_item.ObjID, ss.getBuf()));
	}
	CATCHZOK
	return ok;
}

int GoodsValRestrDialog::EditItem(ObjRestrictItem & rItem)
{
#define GRP_ARTICLE 1

	int    ok = -1;
	TDialog * dlg = 0;
	THROW(CheckDialogPtrErr(&(dlg = new TDialog(DLG_GVRBAR))));
	dlg->addGroup(GRP_ARTICLE, new ArticleCtrlGroup(CTLSEL_GVRBAR_ACS, 0, CTLSEL_GVRBAR_AR, 0, 0));
	{
		PPID   acs_id = 0;
		if(rItem.ObjID)
			GetArticleSheetID(rItem.ObjID, &acs_id, 0);
		ArticleCtrlGroup::Rec acg_rec(acs_id, 0, rItem.ObjID);
		dlg->setGroupData(GRP_ARTICLE, &acg_rec);
	}
	dlg->AddClusterAssoc(CTL_GVRBAR_OPTIONS, 0, PPGoodsValRestrPacket::barMainArOnly);
	dlg->AddClusterAssoc(CTL_GVRBAR_OPTIONS, 1, PPGoodsValRestrPacket::barMainArDisable);
	dlg->AddClusterAssocDef(CTL_GVRBAR_OPTIONS, 2, PPGoodsValRestrPacket::barExtArOnly);
	dlg->AddClusterAssoc(CTL_GVRBAR_OPTIONS, 3, PPGoodsValRestrPacket::barExtArDisable);
	dlg->SetClusterData(CTL_GVRBAR_OPTIONS, rItem.Flags);
	while(ok < 0 && ExecView(dlg) == cmOK) {
		ArticleCtrlGroup::Rec acg_rec;
		ArticleCtrlGroup * p_acg = static_cast<ArticleCtrlGroup *>(getGroup(GRP_ARTICLE));
		dlg->getGroupData(GRP_ARTICLE, &acg_rec);
		PPID   ar_id = acg_rec.ArList.GetSingle();
		if(!ar_id) {
			PPError(PPERR_ARTICLENEEDED);
		}
		else {
			rItem.ObjID = ar_id;
			rItem.Flags = dlg->GetClusterData(CTL_GVRBAR_OPTIONS);
			ok = 1;
		}
	}
	CATCHZOK
	delete dlg;
	return ok;
}

int GoodsValRestrDialog::addItem(long * pPos, long * pID)
{
	int    ok = -1;
	ObjRestrictItem item;
	// @v10.7.8 @ctr MEMSZERO(item);
	while(ok < 0 && EditItem(item) > 0) {
		if(!Data.SetBillArRestr(item.ObjID, item.Flags)) {
			PPError();
		}
		else
			ok = 1;
	}
	return ok;
}

int GoodsValRestrDialog::editItem(long pos, long id)
{
	int    ok = -1;
	uint   _p = 0;
	const ObjRestrictArray & r_list = Data.GetBillArRestrictList();
	if(r_list.SearchItemByID(id, &_p)) {
		ObjRestrictItem item = r_list.at(_p);
		while(ok < 0 && EditItem(item) > 0) {
			if(!Data.SetBillArRestr(item.ObjID, item.Flags)) {
				PPError();
			}
			else
				ok = 1;
		}
	}
	return ok;
}

int GoodsValRestrDialog::delItem(long pos, long id)
{
	return Data.RemoveBillArRestr(id) ? 1 : -1;
}

/*static*/int PPObjGoodsValRestr::TestFormula(const char * pFormula)
{
	double bound = 0.0;
	PPBillPacket pack;
	pack.Rec.Dt = getcurdate_();
	PPTransferItem ti;
	ti.GoodsID = 1000000;
	ti.Cost = 1.0;
	ti.Price = 2.0;
	GdsClsCalcExprContext ctx(&ti, &pack);
	return BIN(PPCalcExpression(pFormula, &bound, &ctx));
}

int PPObjGoodsValRestr::Edit(PPID * pID, void * extraPtr)
{
	int    r = cmCancel;
	int    ok = 1;
	int    valid_data = 0;
	int    is_new = 0;
	PPGoodsValRestrPacket pack;
	GoodsValRestrDialog * dlg = new GoodsValRestrDialog;
	THROW(CheckDialogPtr(&dlg));
	THROW(EditPrereq(pID, dlg, &is_new));
	if(!is_new)
		THROW(GetPacket(*pID, &pack) > 0);
	dlg->setDTS(&pack);
	while(!valid_data && (r = ExecView(dlg)) == cmOK) {
		THROW(is_new || CheckRights(PPR_MOD));
		dlg->getDTS(&pack);
		if(!CheckName(*pID, strip(pack.Rec.Name), 0))
			dlg->selectCtrl(CTL_GVR_NAME);
		else if(*strip(pack.Rec.Symb) && !P_Ref->CheckUniqueSymb(Obj, pack.Rec.ID, pack.Rec.Symb, offsetof(PPGoodsValRestr, Symb)))
			PPErrorByDialog(dlg, CTL_GVR_SYMB);
		else {
			valid_data = 1;
			if(*pID)
				*pID = pack.Rec.ID;
			if(PutPacket(pID, &pack, 1)) {
				*pID = pack.Rec.ID;
				Dirty(*pID);
			}
			else
				PPError();
		}
	}
	CATCHZOKPPERR
	delete dlg;
	return ok ? r : 0;
}

int PPObjGoodsValRestr::PutPacket(PPID * pID, const PPGoodsValRestrPacket * pPack, int use_ta)
{
	int    ok = 1;
	int    acn = 0;
	THROW_PP(!pPack || pPack->Rec.Name[0], PPERR_NAMENEEDED);
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(*pID) {
			if(pPack) {
				PPGoodsValRestrPacket org_pack;
				THROW(GetPacket(*pID, &org_pack) > 0);
				if(!IsPacketEq(*pPack, org_pack, 0)) {
					THROW(CheckDupName(*pID, pPack->Rec.Name));
					THROW(CheckDupSymb(*pID, pPack->Rec.Symb));
					THROW(CheckRights(PPR_MOD));
					THROW(P_Ref->UpdateItem(Obj, *pID, &pPack->Rec, 1, 0));
				}
				else
					ok = -1;
			}
			else {
				THROW(CheckRights(PPR_DEL));
				THROW(P_Ref->RemoveItem(Obj, *pID, 0));
				acn = PPACN_OBJRMV;
			}
		}
		else if(pPack) {
			THROW(CheckRights(PPR_INS));
			THROW(CheckDupName(*pID, pPack->Rec.Name));
			THROW(CheckDupSymb(*pID, pPack->Rec.Symb));
			*pID = pPack->Rec.ID;
			THROW(P_Ref->AddItem(Obj, pID, &pPack->Rec, 0));
		}
		if(ok > 0) {
			{
				const char * p = 0;
				SString text;
				if(pPack) {
					PPPutExtStrData(1, text, pPack->LowBoundFormula);
					PPPutExtStrData(2, text, pPack->UppBoundFormula);
					p = text;
				}
				THROW(P_Ref->PutPropVlrString(Obj, *pID, GVRPROP_TEXT, p));
			}
			{
				const SVector * p_array = (pPack && pPack->GetBillArRestrictList().getCount()) ? &pPack->GetBillArRestrictList() : 0;
				THROW(P_Ref->PutPropArray(Obj, *pID, GVRPROP_BAR, p_array, 0));
			}
		}
		if(acn)
			DS.LogAction(acn, Obj, *pID, 0, 0);
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int PPObjGoodsValRestr::IsPacketEq(const PPGoodsValRestrPacket & rS1, const PPGoodsValRestrPacket & rS2, long flags)
{
#define CMP_MEMB(m)  if(rS1.Rec.m != rS2.Rec.m) return 0;
#define CMP_MEMBS(m) if(!sstreq(rS1.Rec.m, rS2.Rec.m)) return 0;
	CMP_MEMB(ID);
	CMP_MEMBS(Name);
	CMP_MEMBS(Symb);
	CMP_MEMB(ScpShipmOpID);
	CMP_MEMB(ScpRetOpID);
	CMP_MEMB(ScpDurationDays);
	CMP_MEMB(ScpUpDev);
	CMP_MEMB(ScpDnDev);
	CMP_MEMB(ScpShipmLimitOpID);
	CMP_MEMB(Flags);
#undef CMP_MEMBS
#undef CMP_MEMB
	if(rS1.LowBoundFormula != rS2.LowBoundFormula)
		return 0;
	if(rS1.UppBoundFormula != rS2.UppBoundFormula)
		return 0;
	if(!rS1.BillArRestr.IsEq(rS2.BillArRestr))
		return 0;
	return 1;
}

int PPObjGoodsValRestr::SerializePacket(int dir, PPGoodsValRestrPacket * pPack, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	THROW_SL(P_Ref->SerializeRecord(dir, &pPack->Rec, rBuf, pSCtx));
	THROW_SL(pSCtx->Serialize(dir, pPack->LowBoundFormula, rBuf));
	THROW_SL(pSCtx->Serialize(dir, pPack->UppBoundFormula, rBuf));
	THROW_SL(pSCtx->Serialize(dir, &pPack->BillArRestr, rBuf));
	CATCHZOK
	return ok;
}

IMPL_DESTROY_OBJ_PACK(PPObjGoodsValRestr, PPGoodsValRestrPacket);

int  PPObjGoodsValRestr::Read(PPObjPack * p, PPID id, void * stream, ObjTransmContext * pCtx)
	{ return Implement_ObjReadPacket<PPObjGoodsValRestr, PPGoodsValRestrPacket>(this, p, id, stream, pCtx); }

/*virtual*/int PPObjGoodsValRestr::Write(PPObjPack * p, PPID * pID, void * stream, ObjTransmContext * pCtx)
{
	int    ok = 1, r;
	if(p && p->Data) {
		PPGoodsValRestrPacket * p_pack = static_cast<PPGoodsValRestrPacket *>(p->Data);
		if(stream == 0) {
			if(*pID == 0) {
				PPID   same_id = 0;
				PPGoodsValRestr same_rec;
				if(p_pack->Rec.ID < PP_FIRSTUSRREF) {
					if(Search(p_pack->Rec.ID, &same_rec) > 0) {
						*pID = same_id = p_pack->Rec.ID;
						ok = 1;
					}
				}
				else if(p_pack->Rec.Symb[0] && SearchBySymb(p_pack->Rec.Symb, &same_id, &same_rec) > 0) {
					*pID = same_id;
					ok = 1;
				}
				else if(p_pack->Rec.Name[0] && SearchByName(p_pack->Rec.Name, &same_id, &same_rec) > 0) {
					*pID = same_id;
					ok = 1;
				}
				else {
					same_id = p_pack->Rec.ID = 0;
				}
				if(same_id == 0) {
					p_pack->Rec.ID = 0;
					r = PutPacket(pID, p_pack, 1);
					if(!r) {
						pCtx->OutputAcceptErrMsg(PPTXT_ERRACCEPTGOODSVALRESTR, p_pack->Rec.ID, p_pack->Rec.Name);
						ok = -1;
					}
					else if(r > 0)
						ok = 1; // 101; // @ObjectCreated
					else
						ok = 1;
				}
			}
			else {
				p_pack->Rec.ID = *pID;
				r = PutPacket(pID, p_pack, 1);
				if(!r) {
					pCtx->OutputAcceptErrMsg(PPTXT_ERRACCEPTGOODSVALRESTR, p_pack->Rec.ID, p_pack->Rec.Name);
					ok = -1;
				}
				else if(r > 0)
					ok = 1; // 102; // @ObjectUpdated
				else
					ok = 1;
			}
		}
		else {
			SBuffer buffer;
			THROW(SerializePacket(+1, p_pack, buffer, &pCtx->SCtx));
			THROW_SL(buffer.WriteToFile(static_cast<FILE *>(stream), 0, 0))
		}
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

/*virtual*/int  PPObjGoodsValRestr::ProcessObjRefs(PPObjPack * p, PPObjIDArray * ary, int replace, ObjTransmContext * pCtx)
{
	int    ok = 1;
	if(p && p->Data) {
		PPGoodsValRestrPacket * p_pack = static_cast<PPGoodsValRestrPacket *>(p->Data);
		THROW(ProcessObjRefInArray(PPOBJ_OPRKIND, &p_pack->Rec.ScpShipmOpID, ary, replace));
		THROW(ProcessObjRefInArray(PPOBJ_OPRKIND, &p_pack->Rec.ScpRetOpID, ary, replace));
		THROW(ProcessObjRefInArray(PPOBJ_OPRKIND, &p_pack->Rec.ScpShipmLimitOpID, ary, replace));
		for(uint i = 0; i < p_pack->BillArRestr.getCount(); i++) {
			THROW(ProcessObjRefInArray(PPOBJ_ARTICLE, &p_pack->BillArRestr.at(i).ObjID, ary, replace));
		}
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

int PPObjGoodsValRestr::Helper_ReadBarList(PPID id, PPGoodsValRestrPacket & rPack)
{
	int    ok = -1;
	ObjRestrictArray array;
	if(P_Ref->GetPropArray(Obj, id, GVRPROP_BAR, &array) > 0) {
		if(array.getCount()) {
			for(uint i = 0; i < array.getCount(); i++) {
				const ObjRestrictItem & r_item = array.at(i);
				if(rPack.SetBillArRestr(r_item.ObjID, r_item.Flags) > 0)
					ok = 1;
			}
		}
	}
	return ok;
}

int PPObjGoodsValRestr::ReadBarList(PPID id, ObjRestrictArray & rList)
{
	int    ok = -1;
	rList.clear();
	PPGoodsValRestrPacket pack;
	if(Helper_ReadBarList(id, pack) > 0) {
		rList = pack.GetBillArRestrictList();
		ok = 1;
	}
	return ok;
}

int PPObjGoodsValRestr::GetPacket(PPID id, PPGoodsValRestrPacket * pPack)
{
	int    ok = -1;
	PPGoodsValRestrPacket pack;
	if(PPCheckGetObjPacketID(Obj, id)) { // @v10.3.6
		ok = Search(id, &pack.Rec);
		if(ok > 0) {
			SString text;
			if(P_Ref->GetPropVlrString(Obj, id, GVRPROP_TEXT, text) > 0) {
				PPGetExtStrData(1, text, pack.LowBoundFormula);
				PPGetExtStrData(2, text, pack.UppBoundFormula);
			}
			Helper_ReadBarList(id, pack);
		}
	}
	ASSIGN_PTR(pPack, pack);
	return ok;
}
//
//
//
class GoodsValRestrCache : public ObjCache {
public:
	GoodsValRestrCache() : ObjCache(PPOBJ_GOODSVALRESTR, sizeof(GoodsValRestrData)), P_BarList(0)
	{
	}
	~GoodsValRestrCache()
	{
		delete P_BarList;
	}
	int    FetchBarList(PPObjGoodsValRestr::GvrArray & rList);
	int    DirtyBarList();
private:
	virtual int  FetchEntry(PPID, ObjCacheEntry * pEntry, long);
	virtual void EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const;
	virtual int  FASTCALL Dirty(PPID id);
public:
	struct GoodsValRestrData : public ObjCacheEntry {
		PPID   ScpShipmOpID;
		PPID   ScpRetOpID;
		PPID   ScpShipmLimitOpID;
		long   ScpDurationDays;
		long   ScpUpDev;
		long   ScpDnDev;
		long   Flags;
	};
	PPObjGoodsValRestr::GvrArray * P_BarList;
	ReadWriteLock BarLock; // Блокировка для P_BarList
};

int GoodsValRestrCache::FetchBarList(PPObjGoodsValRestr::GvrArray & rList)
{
	int    ok = -1;
	rList.clear();
	{
		SRWLOCKER(BarLock, SReadWriteLocker::Read);
		if(!P_BarList) {
			SRWLOCKER_TOGGLE(SReadWriteLocker::Write);
			if(!P_BarList) {
				PPObjGoodsValRestr gvr_obj;
				PPGoodsValRestrPacket pack;
				PPGoodsValRestr gvr_rec;
				ObjRestrictArray gvr_list;
				for(SEnum en = PPRef->Enum(PPOBJ_GOODSVALRESTR, 0); ok && en.Next(&gvr_rec) > 0;) {
					if(gvr_obj.ReadBarList(gvr_rec.ID, gvr_list) > 0) {
						for(uint i = 0; ok && i < gvr_list.getCount(); i++) {
							SETIFZ(P_BarList, new PPObjGoodsValRestr::GvrArray);
							if(P_BarList) {
								const ObjRestrictItem & r_oritem = gvr_list.at(i);
								PPObjGoodsValRestr::GvrItem new_item;
								MEMSZERO(new_item);
								new_item.GvrID = gvr_rec.ID;
								new_item.ArID = r_oritem.ObjID;
								new_item.GvrBarOption = r_oritem.Flags;
								if(P_BarList->insert(&new_item))
									ok = 1;
								else
									ok = PPSetErrorSLib();
							}
							else
								ok = PPSetErrorNoMem();
						}
					}
				}
			}
		}
		if(ok && P_BarList) {
			rList = *P_BarList;
			ok = 1;
		}
	}
	return ok;
}

int FASTCALL GoodsValRestrCache::Dirty(PPID id)
{
	int    ok = 1;
	ObjCache::Dirty(id);
	{
		SRWLOCKER(BarLock, SReadWriteLocker::Write);
		ZDELETE(P_BarList);
	}
	return ok;
}

int GoodsValRestrCache::FetchEntry(PPID id, ObjCacheEntry * pEntry, long)
{
	int    ok = 1;
	GoodsValRestrData * p_cache_rec = static_cast<GoodsValRestrData *>(pEntry);
	PPObjGoodsValRestr gvr_obj;
	PPGoodsValRestrPacket pack;
	if(gvr_obj.GetPacket(id, &pack) > 0) {
#define CPY_FLD(Fld) p_cache_rec->Fld=pack.Rec.Fld
		CPY_FLD(ScpShipmOpID);
		CPY_FLD(ScpRetOpID);
		CPY_FLD(ScpShipmLimitOpID); // @v9.2.3
		CPY_FLD(ScpDurationDays);
		CPY_FLD(ScpUpDev);
		CPY_FLD(ScpDnDev);
		CPY_FLD(Flags);
#undef CPY_FLD
		MultTextBlock b;
		b.Add(pack.Rec.Name);
		b.Add(pack.Rec.Symb);
		b.Add(pack.LowBoundFormula);
		b.Add(pack.UppBoundFormula);
		ok = PutTextBlock(b, pEntry);
	}
	else
		ok = -1;
	return ok;
}

void GoodsValRestrCache::EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const
{
	PPGoodsValRestrPacket * p_data_pack = static_cast<PPGoodsValRestrPacket *>(pDataRec);
	const GoodsValRestrData * p_cache_rec = static_cast<const GoodsValRestrData *>(pEntry);
	MEMSZERO(p_data_pack->Rec);
	p_data_pack->LowBoundFormula.Z();
	p_data_pack->UppBoundFormula.Z();
	p_data_pack->Rec.Tag   = PPOBJ_GOODSVALRESTR;
	p_data_pack->Rec.ID    = p_cache_rec->ID;
#define CPY_FLD(Fld) p_data_pack->Rec.Fld=p_cache_rec->Fld
	CPY_FLD(ScpShipmOpID);
	CPY_FLD(ScpRetOpID);
	CPY_FLD(ScpShipmLimitOpID);
	CPY_FLD(ScpDurationDays);
	CPY_FLD(ScpUpDev);
	CPY_FLD(ScpDnDev);
	CPY_FLD(Flags);
#undef CPY_FLD
	MultTextBlock b(this, pEntry);
	b.Get(p_data_pack->Rec.Name, sizeof(p_data_pack->Rec.Name));
	b.Get(p_data_pack->Rec.Symb, sizeof(p_data_pack->Rec.Symb));
	b.Get(p_data_pack->LowBoundFormula);
	b.Get(p_data_pack->UppBoundFormula);
}

int PPObjGoodsValRestr::Fetch(PPID id, PPGoodsValRestrPacket * pPack)
{
	GoodsValRestrCache * p_cache = GetDbLocalCachePtr <GoodsValRestrCache> (Obj);
	return p_cache ? p_cache->Get(id, pPack, 0) : GetPacket(id, pPack);
}

int PPObjGoodsValRestr::FetchBarList(PPObjGoodsValRestr::GvrArray & rList)
{
	GoodsValRestrCache * p_cache = GetDbLocalCachePtr <GoodsValRestrCache> (Obj);
	return p_cache ? p_cache->FetchBarList(rList) : 0;
}
//
//
//
PPObjPallet::PPObjPallet(void * extraPtr) : PPObjReference(PPOBJ_PALLET, extraPtr)
{
}

int PPObjPallet::Edit(PPID * pID, void * extraPtr)
{
	int    ok = cmCancel;
	int    is_new = 0;
	TDialog * dlg = 0;
	PPPallet pack;
	THROW(CheckDialogPtr(&(dlg = new TDialog(DLG_PALLET))));
	THROW(EditPrereq(pID, dlg, &is_new));
	if(!is_new) {
		THROW(Search(*pID, &pack) > 0);
	}
	else {
		MEMSZERO(pack);
	}
	dlg->setCtrlData(CTL_PALLET_NAME, pack.Name);
	dlg->setCtrlData(CTL_PALLET_SYMB, pack.Symb);
	dlg->setCtrlLong(CTL_PALLET_ID,   pack.ID);
	dlg->setCtrlLong(CTL_PALLET_LENGTH,  pack.Dim.Length);
	dlg->setCtrlLong(CTL_PALLET_WIDTH,   pack.Dim.Width);
	dlg->setCtrlLong(CTL_PALLET_MAXLOAD, pack.MaxLoad);
	while(ok == cmCancel && ExecView(dlg) == cmOK) {
		THROW(is_new || CheckRights(PPR_MOD));
		dlg->getCtrlData(CTL_PALLET_NAME, pack.Name);
		dlg->getCtrlData(CTL_PALLET_SYMB, pack.Symb);
		if(!CheckName(*pID, strip(pack.Name), 0))
			dlg->selectCtrl(CTL_PALLET_NAME);
		else if(*strip(pack.Symb) && !P_Ref->CheckUniqueSymb(Obj, pack.ID, pack.Symb, offsetof(PPPallet, Symb)))
			PPErrorByDialog(dlg, CTL_PALLET_SYMB);
		else {
			dlg->getCtrlData(CTL_PALLET_ID, &pack.ID);
			pack.Dim.Length = dlg->getCtrlLong(CTL_PALLET_LENGTH);
			pack.Dim.Width = dlg->getCtrlLong(CTL_PALLET_WIDTH);
			pack.MaxLoad = dlg->getCtrlLong(CTL_PALLET_MAXLOAD);
			if(*pID)
				*pID = pack.ID;
			if(StoreItem(Obj, *pID, &pack, 1)) {
				*pID = pack.ID;
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
