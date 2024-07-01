// OBJG_ETC.CPP
// Copyright (c) A.Sobolev 2002, 2003, 2005, 2007, 2008, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022, 2023, 2024
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
	bool   is_new = false;
	PPGoodsType rec;
	TDialog * dlg = 0;
	THROW(CheckDialogPtr(&(dlg = new TDialog(DLG_GDSTYP))));
	THROW(EditPrereq(pID, dlg, &is_new));
	if(!is_new) {
		THROW(Search(*pID, &rec) > 0);
	}
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
	SetupStringCombo(dlg, CTLSEL_GDSTYP_CHZNPT, PPTXT_CHZNPRODUCTTYPES, rec.ChZnProdType);
	dlg->AddClusterAssoc(CTL_GDSTYP_UNLIM, 0, GTF_UNLIMITED);
	dlg->AddClusterAssoc(CTL_GDSTYP_UNLIM, 1, GTF_AUTOCOMPL);
	dlg->AddClusterAssoc(CTL_GDSTYP_UNLIM, 2, GTF_ASSETS);
	dlg->AddClusterAssoc(CTL_GDSTYP_UNLIM, 3, GTF_ADVANCECERT);
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
	dlg->AddClusterAssoc(CTL_GDSTYP_FLAGS, 11, GTF_EXCISEPROFORMA); // @v11.7.10
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

bool FASTCALL PPObjGoodsType::IsUnlim(PPID id)
{
	PPGoodsType gt_rec;
	return (id && id != PPGT_DEFAULT && Fetch(id, &gt_rec) > 0 && gt_rec.Flags & (GTF_UNLIMITED|GTF_AUTOCOMPL));
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
	bool   is_new = false;
	PPBarcodeStruc rec;
	TDialog * dlg = 0;
	THROW(CheckDialogPtr(&(dlg = new TDialog(DLG_BCODESTR))));
	THROW(EditPrereq(pID, dlg, &is_new));
	if(!is_new) {
		THROW(Search(*pID, &rec) > 0);
	}
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
PPGoodsValRestr::PPGoodsValRestr()
{
	THISZERO();
}

PPGoodsValRestrPacket::PPGoodsValRestrPacket()
{
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

int PPObjGoodsValRestr::GvrArray::TestGvrBillArPair(PPID gvrID, PPID arID) const { return Helper_TestGvrBillArPair(gvrID, arID, 0); }
int PPObjGoodsValRestr::GvrArray::TestGvrBillExtArPair(PPID gvrID, PPID arID) const { return Helper_TestGvrBillArPair(gvrID, arID, 1); }

int PPObjGoodsValRestr::GvrArray::Helper_TestGvrBillArPair(PPID gvrID, PPID arID, int what) const
{
	assert(oneof2(what, 0, 1));
	int    ok = -1;
	bool   has_this_gvr = false;
	bool   has_only_restrict = false;
	for(uint i = 0; ok < 0 && i < getCount(); i++) {
		const GvrItem & r_item = at(i);
		if(r_item.GvrID == gvrID) {
			has_this_gvr = true;
			const bool only_rule = (what == 0) ? (r_item.GvrBarOption == PPGoodsValRestrPacket::barMainArOnly) : (r_item.GvrBarOption == PPGoodsValRestrPacket::barExtArOnly);
			const bool dsbl_rule = (what == 0) ? (r_item.GvrBarOption == PPGoodsValRestrPacket::barMainArDisable) : (r_item.GvrBarOption == PPGoodsValRestrPacket::barExtArDisable);
			if(only_rule) {
				has_only_restrict = true;
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
		ss.Z();
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
	bool   is_new = false;
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
	virtual void FASTCALL Dirty(PPID id);
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

void FASTCALL GoodsValRestrCache::Dirty(PPID id)
{
	ObjCache::Dirty(id);
	{
		SRWLOCKER(BarLock, SReadWriteLocker::Write);
		ZDELETE(P_BarList);
	}
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
	bool   is_new = false;
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
//
//
//
PPComputer::PPComputer() : ID(0), Flags(0), CategoryID(0)
{
	Name[0] = 0;
	Code[0] = 0;
}

PPComputer & PPComputer::Z()
{
	ID = 0;
	Flags = 0;
	CategoryID = 0;
	Uuid.Z();
	MacAdr.Z();
	IpAdr.Z();
	Name[0] = 0;
	Code[0] = 0;
	return *this;
}

bool FASTCALL PPComputer::IsEq(const PPComputer & rS) const
{
	return (ID == rS.ID && sstreq(Name, rS.Name) && sstreq(Code, rS.Code) && Flags == rS.Flags && 
		CategoryID == rS.CategoryID && Uuid == rS.Uuid && MacAdr == rS.MacAdr && IpAdr == rS.IpAdr);
}

bool FASTCALL PPComputer::CheckForFilt(const ComputerFilt * pFilt) const
{
	return true; // @stub
}

PPComputerPacket::PPComputerPacket() : LinkFiles(PPOBJ_COMPUTER)
{
}

PPComputerPacket & PPComputerPacket::Z()
{
	Rec.Z();
	LinkFiles.Clear();
	TagL.Destroy();
	return *this;
}

bool FASTCALL PPComputerPacket::IsEq(const PPComputerPacket & rS) const
{
	return (Rec.IsEq(rS.Rec) && TagL.IsEq(rS.TagL));
}

bool FASTCALL PPComputerPacket::Copy(const PPComputerPacket & rS)
{
	Rec = rS.Rec;
	TagL = rS.TagL;
	LinkFiles = rS.LinkFiles;
	return true;
}

PPObjComputer::PPObjComputer(void * extraPtr) : PPObjGoods(PPOBJ_COMPUTER, PPGDSK_COMPUTER, extraPtr), P_PrcObj(new PPObjProcessor)
{
}

PPObjComputer::~PPObjComputer()
{
	delete P_PrcObj;
}

/*virtual*/ListBoxDef * PPObjComputer::Selector(ListBoxDef * pOrgDef, long flags, void * extraPtr)
{
	return _Selector2(pOrgDef, 0, PPObjGoods::selfByName, extraPtr, 0, 0);
}

#pragma pack(push,1)
struct PPComputerSysBlock { // @persistent @store(PropertyTbl)
	PPID   Tag;            // Const=PPOBJ_COMPUTER
	PPID   ID;             // ->Computer.ID
	PPID   PropID;         // Const=COMPUTERPRP_SYS
	char   Reserve[42];    //
	MACAddr MacAdr;        // 6bytes
	S_IPAddr IpAdr;        // 16bytes 
	long   Val1;           // @reserve 
	long   Val2;           // @reserve
};
#pragma pop

int PPObjComputer::GenerateName(PPID categoryID, SString & rBuf)
{
	int    ok = -1;
	SString prefix;
	SString name_buf;
	if(categoryID) {
		PPObjComputerCategory compcat_obj;
		PPComputerCategory compcat_rec;
		if(compcat_obj.Search(categoryID, &compcat_rec) > 0) {
			prefix = compcat_rec.CNameTemplate;
		}
	}
	if(prefix.Strip().IsEmpty()) {
		prefix = "Computer";
	}
	long   seq = 0;
	do {
		seq++;
		name_buf.Z().Cat(prefix).CatChar('-').CatLongZ(seq, 3);
	} while(SearchByName(name_buf, 0, 0) > 0);
	rBuf = name_buf;
	ok = 1;
	return ok;
}

int PPObjComputer::Implement_GetByMacAddr(const MACAddr & rKey, PPID * pID, PPComputerPacket * pPack, PPIDArray * pIdList)
{
	int    ok = -1;
	constexpr PPID prop_id = COMPUTERPRP_SYS;
	CALLPTRMEMB(pIdList, Z());
	if(pIdList || (pID || pPack)) {
		const  bool   first_only = (pIdList == 0 && (pID || pPack));
		Reference * p_ref = PPRef;
		DBQ * dbq = 0;
		PropertyTbl::Key0 k0;
		PropertyTbl & r_prop = p_ref->Prop;
		BExtQuery bext(&r_prop, 0, 64);
		MEMSZERO(k0);
		k0.ObjType = Obj;
		k0.Prop    = prop_id;
		dbq = &(*dbq && r_prop.ObjType == Obj && r_prop.Prop == prop_id);
		bext.selectAll().where(*dbq);
		for(bext.initIteration(false, &k0); bext.nextIteration() > 0;) {
			const PPComputerSysBlock * p_csblk = reinterpret_cast<const PPComputerSysBlock *>(&r_prop.data);
			if(p_csblk->MacAdr == rKey) {
				const PPID _id = r_prop.data.ObjID;
				ASSIGN_PTR(pID, _id);
				CALLPTRMEMB(pIdList, add(_id));
				if(first_only) {
					if(pPack) {
						THROW(Get(_id, pPack) > 0);
					}
					break;
				}
				ok = 1;
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPObjComputer::GetListByMacAddr(const MACAddr & rKey, PPIDArray & rIdList) { return Implement_GetByMacAddr(rKey, 0, 0, &rIdList); }
int PPObjComputer::SearchByMacAddr(const MACAddr & rKey, PPID * pID, PPComputerPacket * pPack) { return Implement_GetByMacAddr(rKey, pID, pPack, 0); }

/*static*/int PPObjComputer::Helper_GetRec(const Goods2Tbl::Rec & rGoodsRec, PPComputer * pRec)
{
	int    ok = 1;
	if(pRec) {
		memzero(pRec, sizeof(*pRec));
		if(rGoodsRec.Kind == PPGDSK_COMPUTER) {
			pRec->ID = rGoodsRec.ID;
			STRNSCPY(pRec->Name, rGoodsRec.Name);
			pRec->Flags = rGoodsRec.Flags;
			pRec->CategoryID = rGoodsRec.WrOffGrpID;
		}
		else
			ok = PPSetError(/*PPERR_INVBRANDRECKIND*/1, rGoodsRec.ID);
	}
	return ok;
}

int PPObjComputer::Get(PPID id, PPComputerPacket * pPack)
{
	CALLPTRMEMB(pPack, Z());
	int    ok = PPObjGoods::Search(id);
	if(ok > 0) {
		if(pPack) {
			THROW(Helper_GetRec(P_Tbl->data, &pPack->Rec));
			THROW(GetTagList(id, &pPack->TagL));
			{
				const  ObjTagItem * p_tag_item = pPack->TagL.GetItem(PPTAG_COMPUTER_GUID);
				if(p_tag_item)
					p_tag_item->GetGuid(&pPack->Rec.Uuid);
			}
			{
				Reference * p_ref = PPRef;
				PPComputerSysBlock csblk;
				if(p_ref->GetProperty(PPOBJ_COMPUTER, id, COMPUTERPRP_SYS, &csblk, sizeof(csblk)) > 0) {
					pPack->Rec.IpAdr = csblk.IpAdr;
					pPack->Rec.MacAdr = csblk.MacAdr;
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

/*static*/int PPObjComputer::Helper_SetRec(const PPComputer * pRec, Goods2Tbl::Rec & rGoodsRec)
{
	int    ok = 1;
	memzero(&rGoodsRec, sizeof(rGoodsRec));
	if(pRec) {
		rGoodsRec.ID = pRec->ID;
		rGoodsRec.Kind = PPGDSK_COMPUTER;
		STRNSCPY(rGoodsRec.Name, pRec->Name);
		rGoodsRec.Flags = pRec->Flags;
		rGoodsRec.WrOffGrpID = pRec->CategoryID;
	}
	return ok;
}

int PPObjComputer::Put(PPID * pID, PPComputerPacket * pPack, int use_ta)
{
	//CategoryID
	int    ok = 1;
	Reference * p_ref = PPRef;
	Goods2Tbl::Rec raw_rec;
	PPComputerSysBlock csblk;
	PPComputerSysBlock * p_csblk = 0;
	MEMSZERO(csblk);
	{
		if(pPack) {
			//
			// Поле UUID хранится одновременно и в записи PPComputer и в тегах (собственно, теги - способ хранения поля в базе данных).
			// Считаем, что значение в поле PPComputer::Uuid приоритетное.
			//
			if(!!pPack->Rec.Uuid) {
				ObjTagItem tag_item;
				tag_item.SetGuid(PPTAG_COMPUTER_GUID, &pPack->Rec.Uuid);
				pPack->TagL.PutItem(PPTAG_COMPUTER_GUID, &tag_item);
			}
			else {
				pPack->TagL.PutItem(PPTAG_COMPUTER_GUID, 0);
			}
		}
		PPTransaction tra(use_ta);
		THROW(tra);
		if(*pID) {
			PPComputerPacket org_pack;
			THROW(Get(*pID, &org_pack) > 0);
			if(pPack) {
				if(!pPack->IsEq(org_pack) || pPack->LinkFiles.IsChanged(*pID, 0L)) {
					THROW(CheckRights(PPR_MOD));
					Helper_SetRec(&pPack->Rec, raw_rec);
					THROW(P_Tbl->Update(pID, &raw_rec, 0));
					{
						if(!(pPack->Rec.MacAdr.IsZero() && pPack->Rec.IpAdr.IsZero())) {
							csblk.ID = *pID;
							csblk.IpAdr = pPack->Rec.IpAdr;
							csblk.MacAdr = pPack->Rec.MacAdr;
							p_csblk = &csblk;
						}
						THROW(p_ref->PutProp(PPOBJ_COMPUTER, *pID, COMPUTERPRP_SYS, p_csblk, sizeof(*p_csblk)));
					}
					THROW(SetTagList(*pID, &pPack->TagL, 0));
					if(pPack->Rec.CategoryID != org_pack.Rec.CategoryID) {
						THROW(SendObjMessage(DBMSG_COMPUTERLOSECAT, PPOBJ_PROCESSOR, pPack->Rec.ID, org_pack.Rec.CategoryID));
						THROW(SendObjMessage(DBMSG_COMPUTERACQUIRECAT, PPOBJ_PROCESSOR, Obj, *pID, reinterpret_cast<void *>(pPack->Rec.CategoryID), 0));
					}
					if(pPack->LinkFiles.IsChanged(*pID, 0L)) {
						pPack->LinkFiles.Save(*pID, 0L);
					}
					DS.LogAction(PPACN_OBJUPD, Obj, *pID, 0, 0);
				}
				else
					ok = -1;
			}
			else {
				THROW(CheckRights(PPR_DEL));
				THROW(P_Tbl->Update(pID, 0, 0));
				THROW(p_ref->PutProp(PPOBJ_COMPUTER, *pID, COMPUTERPRP_SYS, 0, 0));
				THROW(SetTagList(*pID, 0, 0));
				{
					ObjLinkFiles _lf(Obj);
					_lf.Save(*pID, 0L);
				}
				DS.LogAction(PPACN_OBJRMV, Obj, *pID, 0, 0);
			}
		}
		else if(pPack) {
			THROW(CheckRights(PPR_INS));
			Helper_SetRec(&pPack->Rec, raw_rec);
			THROW(P_Tbl->Update(pID, &raw_rec, 0));
			{
				if(!(pPack->Rec.MacAdr.IsZero() && pPack->Rec.IpAdr.IsZero())) {
					csblk.ID = *pID;
					csblk.IpAdr = pPack->Rec.IpAdr;
					csblk.MacAdr = pPack->Rec.MacAdr;
					p_csblk = &csblk;
					
				}
				THROW(p_ref->PutProp(PPOBJ_COMPUTER, *pID, COMPUTERPRP_SYS, p_csblk, sizeof(*p_csblk)));
			}
			THROW(SetTagList(*pID, &pPack->TagL, 0));
			if(pPack->Rec.CategoryID) {
				THROW(SendObjMessage(DBMSG_COMPUTERACQUIRECAT, PPOBJ_PROCESSOR, Obj, *pID, reinterpret_cast<void *>(pPack->Rec.CategoryID), 0));
			}
			if(pPack->LinkFiles.IsChanged(*pID, 0L)) {
				pPack->LinkFiles.Save(*pID, 0L);
			}
			DS.LogAction(PPACN_OBJADD, Obj, *pID, 0, 0);
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

class ComputerDialog : public TDialog {
	DECL_DIALOG_DATA(PPComputerPacket);
	enum {
		ctlgroupIbg = 1
	};
public:
	ComputerDialog() : TDialog(DLG_COMPUTER)
	{
		addGroup(ctlgroupIbg, new ImageBrowseCtrlGroup(/*PPTXT_PICFILESEXTS,*/CTL_COMPUTER_IMAGE,
			cmAddImage, cmDelImage, 1, ImageBrowseCtrlGroup::fUseExtOpenDlg));
	}
	DECL_DIALOG_SETDTS()
	{
		int    ok = 1;
		SString temp_buf;
		RVALUEPTR(Data, pData);
		//
		setCtrlLong(CTL_COMPUTER_ID, Data.Rec.ID);
		setCtrlData(CTL_COMPUTER_NAME, Data.Rec.Name);
		SetupPPObjCombo(this, CTLSEL_COMPUTER_CAT, PPOBJ_COMPUTERCATEGORY, Data.Rec.CategoryID, OLW_CANINSERT);
		if(!!Data.Rec.Uuid)
			Data.Rec.Uuid.ToStr(S_GUID::fmtIDL, temp_buf);
		else
			temp_buf.Z();
		setCtrlString(CTL_COMPUTER_UUID, temp_buf);
		if(!Data.Rec.MacAdr.IsZero())
			Data.Rec.MacAdr.ToStr(0, temp_buf);
		else
			temp_buf.Z();
		setCtrlString(CTL_COMPUTER_MACADR, temp_buf);
		if(!Data.Rec.IpAdr.IsZero())
			Data.Rec.IpAdr.ToStr(0, temp_buf);
		else
			temp_buf.Z();
		setCtrlString(CTL_COMPUTER_IPADR, temp_buf);
		{
			ImageBrowseCtrlGroup::Rec rec;
			Data.LinkFiles.Init(PPOBJ_COMPUTER);
			if(Data.Rec.Flags & GF_DERIVED_HASIMAGES)
				Data.LinkFiles.Load(Data.Rec.ID, 0L);
			Data.LinkFiles.At(0, rec.Path);
			setGroupData(ctlgroupIbg, &rec);
		}
		return ok;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		uint   sel = 0;
		SString temp_buf;
		getCtrlData(sel = CTL_COMPUTER_NAME,  Data.Rec.Name);
		THROW_PP(*strip(Data.Rec.Name), PPERR_NAMENEEDED);
		getCtrlData(CTLSEL_COMPUTER_CAT, &Data.Rec.CategoryID);
		getCtrlString(CTL_COMPUTER_MACADR, temp_buf);
		// @todo MACAddr::FromStr
		getCtrlString(CTL_COMPUTER_IPADR, temp_buf);
		Data.Rec.IpAdr.FromStr(temp_buf); // @todo check error
		{
			ImageBrowseCtrlGroup::Rec rec;
			if(getGroupData(ctlgroupIbg, &rec))
				if(rec.Path.Len()) {
					THROW(Data.LinkFiles.Replace(0, rec.Path));
				}
				else
					Data.LinkFiles.Remove(0);
			SETFLAG(Data.Rec.Flags, GF_DERIVED_HASIMAGES, Data.LinkFiles.GetCount());
		}
		ASSIGN_PTR(pData, Data);
		CATCHZOKPPERRBYDLG
		return ok;
	}
private:
	DECL_HANDLE_EVENT
	{
		TDialog::handleEvent(event);
		if(event.isCmd(cmTags)) {
			Data.TagL.ObjType = PPOBJ_COMPUTER;
			if(EditObjTagValList(&Data.TagL, 0) > 0) {
				const ObjTagItem * p_tag_item = Data.TagL.GetItem(PPTAG_COMPUTER_GUID);
				S_GUID uuid;
				if(p_tag_item && p_tag_item->GetGuid(&uuid)) {
					Data.Rec.Uuid = uuid;
					{
						SString temp_buf;
						if(!!Data.Rec.Uuid)
							Data.Rec.Uuid.ToStr(S_GUID::fmtIDL, temp_buf);
						else
							temp_buf.Z();
						setCtrlString(CTL_COMPUTER_UUID, temp_buf);
					}
				}
			}
		}
		else if(event.isCmd(cmInputUpdated) && event.isCtlEvent(CTL_COMPUTER_UUID)) {
			SString temp_buf;
			getCtrlString(CTL_COMPUTER_UUID, temp_buf);
			S_GUID uuid;
			if(temp_buf.IsEmpty() || uuid.FromStr(temp_buf)) {
				ObjTagItem tag_item;
				if(tag_item.SetGuid(PPTAG_COMPUTER_GUID, &uuid)) {
					Data.TagL.PutItem(PPTAG_COMPUTER_GUID, &tag_item);
				}
			}
		}
		else
			return;
		clearEvent(event);
	}
};

/*virtual*/int PPObjComputer::Edit(PPID * pID, void * extraPtr)
{
	int    ok = -1;
	bool   valid_data = false;
	bool   is_new = false;
	ComputerDialog * dlg = 0;
	PPComputerPacket pack;
	THROW(EditPrereq(pID, dlg, &is_new));
	THROW(CheckDialogPtr(&(dlg = new ComputerDialog())));
	if(!is_new) {
		THROW(Get(*pID, &pack) > 0);
	}
	else
		pack.Z();
	THROW(dlg->setDTS(&pack));
	while(!valid_data && ExecView(dlg) == cmOK) {
		if(dlg->getDTS(&pack)) {
			if(Put(pID, &pack, 1)) {
				ok = cmOK;
				valid_data = true;
			}
			else
				PPError();
		}
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}

IMPLEMENT_PPFILT_FACTORY(Computer); ComputerFilt::ComputerFilt() : PPBaseFilt(PPFILT_COMPUTER, 0, 1)
{
	SetFlatChunk(offsetof(ComputerFilt, ReserveStart), offsetof(ComputerFilt, SrchStr) - offsetof(ComputerFilt, ReserveStart));
	SetBranchSString(offsetof(ComputerFilt, SrchStr));
	Init(1, 0);
}

ComputerFilt & FASTCALL ComputerFilt::operator = (const ComputerFilt & rS)
{
	Copy(&rS, 1);
	return *this;
}

/*virtual*/bool ComputerFilt::IsEmpty() const
{
	return (Flags == 0 && SrchStr.IsEmpty());
}

PPViewComputer::BrwItem::BrwItem(const PPComputer * pS) : ID(0), Flags(0), ViewFlags(0)
{
	Name[0] = 0;
	if(pS) {
		ID = pS->ID;
		Flags = pS->Flags;
		STRNSCPY(Name, pS->Name);
	}
}

PPViewComputer::PPViewComputer() : PPView(&Obj, &Filt, PPVIEW_COMPUTER, PPView::implBrowseArray|PPView::implDontEditNullFilter, 0), P_DsList(0)
{
}

PPViewComputer::~PPViewComputer()
{
	delete P_DsList;
}

/*virtual*/int PPViewComputer::Init_(const PPBaseFilt * pBaseFilt)
{
	int    ok = 1;
	THROW(Helper_InitBaseFilt(pBaseFilt));
	BExtQuery::ZDelete(&P_IterQuery);
	Counter.Init();
	CATCHZOK
	return ok;
}
	
/*virtual*/int PPViewComputer::EditBaseFilt(PPBaseFilt * pBaseFilt)
{
	return -1; // @stub
}
	
int PPViewComputer::InitIteration()
{
	return -1; // @stub
}
	
int FASTCALL PPViewComputer::NextIteration(ComputerViewItem * pItem)
{
	return -1; // @stub
}
	
/*static*/int PPViewComputer::CellStyleFunc_(const void * pData, long col, int paintAction, BrowserWindow::CellStyle * pCellStyle, PPViewBrowser * pBrw)
{
	return -1; // @stub
}
	
/*static*/int FASTCALL PPViewComputer::GetDataForBrowser(SBrowserDataProcBlock * pBlk)
{
	PPViewComputer * p_v = static_cast<PPViewComputer *>(pBlk->ExtraPtr);
	return p_v ? p_v->_GetDataForBrowser(pBlk) : 0;
}
	
/*virtual*/SArray * PPViewComputer::CreateBrowserArray(uint * pBrwId, SString * pSubTitle)
{
	SArray * p_array = 0;
	PPBrand ds_item;
	THROW(MakeList());
	p_array = new SArray(*P_DsList);
	CATCH
		ZDELETE(P_DsList);
	ENDCATCH
	ASSIGN_PTR(pBrwId, BROWSER_COMPUTER);
	return p_array;
}

static int PPViewComputer_CellStyleFunc(const void * pData, long col, int paintAction, BrowserWindow::CellStyle * pStyle, void * extraPtr)
{
	int    ok = -1;
	PPViewBrowser * p_brw = static_cast<PPViewBrowser *>(extraPtr);
	if(p_brw) {
		PPViewComputer * p_view = static_cast<PPViewComputer *>(p_brw->P_View);
		ok = p_view ? p_view->CellStyleFunc_(pData, col, paintAction, pStyle, p_brw) : -1;
	}
	return ok;
}
	
/*virtual*/void PPViewComputer::PreprocessBrowser(PPViewBrowser * pBrw)
{
	if(pBrw) {
		pBrw->SetDefUserProc(PPViewComputer::GetDataForBrowser, this);
		pBrw->SetCellStyleFunc(PPViewComputer_CellStyleFunc, pBrw);
	}
}
	
/*virtual*/int PPViewComputer::OnExecBrowser(PPViewBrowser *)
{
	return -1; // @stub
}
	
/*virtual*/int PPViewComputer::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	PPID   preserve_id = 0;
	if(ok == -2) {
		PPID   id = pHdr ? *static_cast<const  PPID *>(pHdr) : 0;
		preserve_id = id;
		switch(ppvCmd) {
			case PPVCMD_MOUSEHOVER:
				if(id && static_cast<const BrwItem *>(pHdr)->Flags & GF_DERIVED_HASIMAGES) {
					SString img_path;
					ObjLinkFiles link_files(PPOBJ_COMPUTER);
					link_files.Load(id, 0L);
					link_files.At(0, img_path);
					PPTooltipMessage(0, img_path, pBrw->H(), 10000, 0, SMessageWindow::fShowOnCursor|SMessageWindow::fCloseOnMouseLeave|
						SMessageWindow::fOpaque|SMessageWindow::fSizeByText|SMessageWindow::fChildWindow);
				}
				break;
			case PPVCMD_REFRESH:
				ok = 1;
				break;
		}
	}
	if(ok > 0) {
		MakeList();
		if(pBrw) {
			AryBrowserDef * p_def = pBrw ? static_cast<AryBrowserDef *>(pBrw->getDef()) : 0;
			if(p_def) {
				SArray * p_array = new SArray(*P_DsList);
				p_def->setArray(p_array, 0, 1);
				pBrw->setRange(p_array->getCount());
				uint   temp_pos = 0;
				long   update_pos = -1;
				if(preserve_id > 0 && P_DsList->lsearch(&preserve_id, &temp_pos, CMPF_LONG))
					update_pos = temp_pos;
				if(update_pos >= 0)
					pBrw->go(update_pos);
				else if(update_pos == MAXLONG)
					pBrw->go(p_array->getCount()-1);
			}
			pBrw->Update();
		}
	}
	return ok;
}
	
/*virtual*/void PPViewComputer::ViewTotal()
{
}

int PPViewComputer::_GetDataForBrowser(SBrowserDataProcBlock * pBlk)
{
	int    ok = 0;
	if(pBlk->P_SrcData && pBlk->P_DestData) {
		ok = 1;
		const  BrwItem * p_item = static_cast<const BrwItem *>(pBlk->P_SrcData);
		int    r = 0;
		switch(pBlk->ColumnN) {
			case 0: pBlk->Set(p_item->ID); break; // @id
			case 1: pBlk->Set(p_item->Name); break; // @name
		}
	}
	return ok;
}
	
int PPViewComputer::MakeList()
{
	int    ok = 1;
	PPSwProgram item;
	GoodsCore * p_tbl = Obj.P_Tbl;
	PPIDArray result_list;
	Goods2Tbl::Key2 k2;
	//const  PPID single_owner_id = Filt.OwnerList.GetSingle();
	PPObjGoods goods_obj;
	//PPIDArray single_brand_list;
	PPIDArray goods_list;
	BExtQuery q(p_tbl, 2);
	DBQ * dbq = &(p_tbl->Kind == PPGDSK_COMPUTER);
	if(P_DsList)
		P_DsList->clear();
	else
		P_DsList = new SArray(sizeof(BrwItem));
	//dbq = ppcheckfiltid(dbq, p_tbl->ManufID, single_owner_id);
	q.select(p_tbl->ID, p_tbl->Name, p_tbl->Abbr, p_tbl->WrOffGrpID, p_tbl->GoodsTypeID, p_tbl->ManufID, p_tbl->Flags, 0L).where(*dbq);
	MEMSZERO(k2);
	k2.Kind = PPGDSK_COMPUTER;
	for(q.initIteration(false, &k2, spGe); q.nextIteration() > 0;) {
		PPComputer rec;
		if(Obj.Helper_GetRec(p_tbl->data, &rec) && rec.CheckForFilt(&Filt)) {
			BrwItem new_item(&rec);
			//if(Filt.Flags & Filt.fShowGoodsCount) {
				//single_brand_list.clear();
				//single_brand_list.add(rec.ID);
				//goods_obj.P_Tbl->GetListByBrandList(single_brand_list, goods_list);
				//new_item.LinkGoodsCount = goods_list.getCount();
			//}
			THROW_SL(P_DsList->insert(&new_item));
		}
	}
	CATCHZOK
	CALLPTRMEMB(P_DsList, setPointer(0));
	return ok;
}
//
//
//
PPSwProgram::PPSwProgram()
{
	THISZERO();
}

PPSwProgram & PPSwProgram::Z()
{
	THISZERO();
	return *this;
}

bool FASTCALL PPSwProgram::IsEq(const PPSwProgram & rS) const
{
	return (ID == rS.ID && Flags == rS.Flags && CategoryID == rS.CategoryID && sstreq(Name, rS.Name) && sstreq(ExeFn, rS.ExeFn) && sstreq(Code, rS.Code));
}

bool FASTCALL PPSwProgram::CheckForFilt(const SwProgramFilt * pFilt) const
{
	return true; // @stub
}

PPSwProgramPacket::PPSwProgramPacket() : LinkFiles(PPOBJ_SWPROGRAM)
{
}

PPSwProgramPacket::PPSwProgramPacket(const PPSwProgramPacket & rS) : LinkFiles(PPOBJ_SWPROGRAM)
{
	Copy(rS);
}

PPSwProgramPacket & FASTCALL PPSwProgramPacket::operator = (const PPSwProgramPacket & rS) 
{
	Copy(rS);
	return *this;
}

PPSwProgramPacket & PPSwProgramPacket::Z()
{
	Rec.Z();
	LinkFiles.Clear();
	TagL.Destroy();
	CategoryName_.Z(); // @v12.0.4
	return *this;
}

bool FASTCALL PPSwProgramPacket::Copy(const PPSwProgramPacket & rS)
{
	Rec = rS.Rec;
	TagL = rS.TagL;
	LinkFiles = rS.LinkFiles;
	CategoryName_ = rS.CategoryName_; // @v12.0.4
	return true;
}

bool FASTCALL PPSwProgramPacket::IsEq(const PPSwProgramPacket & rS) const
{
	// Поле CategoryName_ не учитывается при сравнении поскольку временное.
	return (Rec.IsEq(rS.Rec) && TagL.IsEq(rS.TagL));	
}

PPObjSwProgram::PPObjSwProgram(void * extraPtr) : PPObjGoods(PPOBJ_SWPROGRAM, PPGDSK_SWPROGRAM, extraPtr)
{
}
	
PPObjSwProgram::~PPObjSwProgram()
{
}

class SwProgramDialog : public TDialog {
	DECL_DIALOG_DATA(PPSwProgramPacket);
	enum {
		ctlgroupIbg = 1
	};
public:
	SwProgramDialog() : TDialog(DLG_SWPROGRAM)
	{
		addGroup(ctlgroupIbg, new ImageBrowseCtrlGroup(/*PPTXT_PICFILESEXTS,*/CTL_SWPROGRAM_IMAGE,
			cmAddImage, cmDelImage, 1, ImageBrowseCtrlGroup::fUseExtOpenDlg));
	}
	DECL_DIALOG_SETDTS()
	{
		int    ok = 1;
		RVALUEPTR(Data, pData);
		//
		setCtrlLong(CTL_SWPROGRAM_ID, Data.Rec.ID);
		setCtrlData(CTL_SWPROGRAM_NAME, Data.Rec.Name);
		setCtrlData(CTL_SWPROGRAM_EXEFN, Data.Rec.ExeFn);
		{
			ImageBrowseCtrlGroup::Rec rec;
			Data.LinkFiles.Init(PPOBJ_SWPROGRAM);
			if(Data.Rec.Flags & GF_DERIVED_HASIMAGES)
				Data.LinkFiles.Load(Data.Rec.ID, 0L);
			Data.LinkFiles.At(0, rec.Path);
			setGroupData(ctlgroupIbg, &rec);
		}
		SetupPPObjCombo(this, CTLSEL_SWPROGRAM_CAT, PPOBJ_SWPROGRAMCATEGORY, Data.Rec.CategoryID, OLW_CANINSERT, 0);
		return ok;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		uint   sel = 0;
		getCtrlData(sel = CTL_SWPROGRAM_NAME,  Data.Rec.Name);
		THROW_PP(*strip(Data.Rec.Name), PPERR_NAMENEEDED);
		getCtrlData(CTL_SWPROGRAM_EXEFN, Data.Rec.ExeFn);
		getCtrlData(CTLSEL_SWPROGRAM_CAT, &Data.Rec.CategoryID);
		{
			ImageBrowseCtrlGroup::Rec rec;
			if(getGroupData(ctlgroupIbg, &rec))
				if(rec.Path.Len()) {
					THROW(Data.LinkFiles.Replace(0, rec.Path));
				}
				else
					Data.LinkFiles.Remove(0);
			SETFLAG(Data.Rec.Flags, GF_DERIVED_HASIMAGES, Data.LinkFiles.GetCount());
		}
		ASSIGN_PTR(pData, Data);
		CATCHZOKPPERRBYDLG
		return ok;
	}
private:
	DECL_HANDLE_EVENT
	{
		TDialog::handleEvent(event);
		if(event.isCmd(cmTags)) {
			Data.TagL.ObjType = PPOBJ_SWPROGRAM;
			EditObjTagValList(&Data.TagL, 0);
			clearEvent(event);
		}
	}
};

/*virtual*/int PPObjSwProgram::Edit(PPID * pID, void * extraPtr)
{
	int    ok = -1;
	bool   valid_data = false;
	bool   is_new = false;
	SwProgramDialog * dlg = 0;
	PPSwProgramPacket pack;
	THROW(EditPrereq(pID, dlg, &is_new));
	THROW(CheckDialogPtr(&(dlg = new SwProgramDialog())));
	if(!is_new) {
		THROW(Get(*pID, &pack) > 0);
	}
	else
		pack.Z();
	THROW(dlg->setDTS(&pack));
	while(!valid_data && ExecView(dlg) == cmOK) {
		if(dlg->getDTS(&pack)) {
			if(Put(pID, &pack, 1)) {
				ok = cmOK;
				valid_data = true;
			}
			else
				PPError();
		}
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}

/*virtual*/ListBoxDef * PPObjSwProgram::Selector(ListBoxDef * pOrgDef, long flags, void * extraPtr)
{
	return _Selector2(pOrgDef, 0, PPObjGoods::selfByName, extraPtr, 0, 0);
}

/*virtual*/void * PPObjSwProgram::CreateObjListWin(uint flags, void * extraPtr)
{
	return 0;
}

int PPObjSwProgram::Put(PPID * pID, PPSwProgramPacket * pPack, int use_ta)
{
	int    ok = 1;
	Goods2Tbl::Rec raw_rec;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(*pID) {
			PPSwProgramPacket org_pack;
			THROW(Get(*pID, &org_pack) > 0);
			if(pPack) {
				if(!pPack->IsEq(org_pack) || pPack->LinkFiles.IsChanged(*pID, 0L)) {
					THROW(CheckRights(PPR_MOD));
					Helper_SetRec(&pPack->Rec, raw_rec);
					THROW(P_Tbl->Update(pID, &raw_rec, 0));
					THROW(SetTagList(*pID, &pPack->TagL, 0));
					if(pPack->LinkFiles.IsChanged(*pID, 0L)) {
						pPack->LinkFiles.Save(*pID, 0L);
					}
					DS.LogAction(PPACN_OBJUPD, Obj, *pID, 0, 0);
				}
				else
					ok = -1;
			}
			else {
				THROW(CheckRights(PPR_DEL));
				THROW(P_Tbl->Update(pID, 0, 0));
				THROW(SetTagList(*pID, 0, 0));
				{
					ObjLinkFiles _lf(Obj);
					_lf.Save(*pID, 0L);
				}
				DS.LogAction(PPACN_OBJRMV, Obj, *pID, 0, 0);
			}
		}
		else if(pPack) {
			THROW(CheckRights(PPR_INS));
			Helper_SetRec(&pPack->Rec, raw_rec);
			THROW(P_Tbl->Update(pID, &raw_rec, 0));
			THROW(SetTagList(*pID, &pPack->TagL, 0));
			if(pPack->LinkFiles.IsChanged(*pID, 0L)) {
				pPack->LinkFiles.Save(*pID, 0L);
			}
			DS.LogAction(PPACN_OBJADD, Obj, *pID, 0, 0);
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

/*static*/int PPObjSwProgram::Helper_SetRec(const PPSwProgram * pRec, Goods2Tbl::Rec & rGoodsRec)
{
	int    ok = 1;
	memzero(&rGoodsRec, sizeof(rGoodsRec));
	if(pRec) {
		rGoodsRec.ID = pRec->ID;
		rGoodsRec.Kind = PPGDSK_SWPROGRAM;
		STRNSCPY(rGoodsRec.Name, pRec->Name);
		STRNSCPY(rGoodsRec.Abbr, pRec->ExeFn);
		rGoodsRec.WrOffGrpID = pRec->CategoryID;
		rGoodsRec.Flags = pRec->Flags;
	}
	return ok;
}

/*static*/int PPObjSwProgram::Helper_GetRec(const Goods2Tbl::Rec & rGoodsRec, PPSwProgram * pRec)
{
	int    ok = 1;
	if(pRec) {
		memzero(pRec, sizeof(*pRec));
		if(rGoodsRec.Kind == PPGDSK_SWPROGRAM) {
			pRec->ID = rGoodsRec.ID;
			STRNSCPY(pRec->Name, rGoodsRec.Name);
			STRNSCPY(pRec->ExeFn, rGoodsRec.Abbr);
			pRec->Flags = rGoodsRec.Flags;
			pRec->CategoryID = rGoodsRec.WrOffGrpID;
		}
		else
			ok = PPSetError(/*PPERR_INVBRANDRECKIND*/1, rGoodsRec.ID);
	}
	return ok;
}

int PPObjSwProgram::Fetch(PPID id, PPSwProgram * pRec)
{
	Goods2Tbl::Rec goods_rec;
	int    ok = PPObjGoods::Fetch(id, &goods_rec);
	return (ok > 0) ? Helper_GetRec(goods_rec, pRec) : ok;
}

int PPObjSwProgram::Get(PPID id, PPSwProgramPacket * pPack)
{
	int    ok = PPObjGoods::Search(id);
	if(ok > 0) {
		if(pPack) {
			THROW(Helper_GetRec(P_Tbl->data, &pPack->Rec));
			THROW(GetTagList(id, &pPack->TagL));
		}
	}
	CATCHZOK
	return ok;
}

/*static*/SJson * PPObjSwProgram::PackToJson(const PPSwProgramPacket & rPack)
{
	SJson * p_result = SJson::CreateObj();
	if(p_result) {
		Reference * p_ref = PPRef;
		SString temp_buf;
		p_result->InsertInt("id", rPack.Rec.ID);
		if(rPack.Rec.CategoryID) {
			Reference2Tbl::Rec cat_rec;
			if(p_ref->GetItem(PPOBJ_SWPROGRAMCATEGORY, rPack.Rec.CategoryID, &cat_rec) > 0) {
				p_result->InsertString("category", (temp_buf = cat_rec.ObjName).Transf(CTRANSF_INNER_TO_UTF8));
			}
		}
		else {
			p_result->InsertString("category", (temp_buf = "uncategorized").Transf(CTRANSF_INNER_TO_UTF8));
		}
		p_result->InsertString("title", (temp_buf = rPack.Rec.Name).Transf(CTRANSF_INNER_TO_UTF8));
		if(!isempty(rPack.Rec.ExeFn)) {
			p_result->InsertString("exefile", (temp_buf = rPack.Rec.ExeFn).Transf(CTRANSF_INNER_TO_UTF8));
		}
	}
	return p_result;
}

/*static*/int PPObjSwProgram::PackFromJson(const SJson * pJsObj, const char * pImgPathUtf8, PPSwProgramPacket & rPack)
{
	int    ok = 0;
	rPack.Z();
	if(SJson::IsObject(pJsObj)) {
		SString temp_buf;
		for(const SJson * p_cur = pJsObj->P_Child; p_cur; p_cur = p_cur->P_Next) {
			if(p_cur->P_Child) {
				if(p_cur->Text.IsEqiAscii("id")) {
					rPack.Rec.ID = p_cur->P_Child->Text.ToLong();
				}
				else if(p_cur->Text.IsEqiAscii("category")) {
					(rPack.CategoryName_ = p_cur->P_Child->Text).Transf(CTRANSF_UTF8_TO_INNER);
				}
				else if(p_cur->Text.IsEqiAscii("title")) {
					(temp_buf = p_cur->P_Child->Text).Transf(CTRANSF_UTF8_TO_INNER);
					STRNSCPY(rPack.Rec.Name, temp_buf);
				}
				else if(p_cur->Text.IsEqiAscii("exefile")) {
					(temp_buf = p_cur->P_Child->Text).Transf(CTRANSF_UTF8_TO_INNER);
					STRNSCPY(rPack.Rec.ExeFn, temp_buf);
				}
				else if(p_cur->Text.IsEqiAscii("picsymb")) {
					temp_buf = p_cur->P_Child->Text;
					if(pImgPathUtf8) {
						SString img_file_name;
						(img_file_name = pImgPathUtf8).Strip().SetLastSlash().Cat(temp_buf.Strip());
						if(fileExists(img_file_name)) {
							rPack.LinkFiles.Replace(0, img_file_name);
						}
					}
				}
			}
		}
		if(!isempty(rPack.Rec.Name)) {
			ok = 1;
		}
		else
			rPack.Z();
	}
	return ok;
}

/*static*/bool PPObjSwProgram::MakeImgSymb(SFileFormat fmt, PPID id, SString & rBuf)
{
	bool ok = false;
	rBuf.Z();
	if(id) {
		rBuf.Cat("swprogram_img").CatChar('-').CatLongZ(id, 6);
		SString & r_ext = SLS.AcquireRvlStr();
		if(SFileFormat::GetExt(fmt, r_ext)) {
			rBuf.Dot().Cat(r_ext);
			ok = true;
		}
	}
	return ok;
}

SJson * PPObjSwProgram::ExportToJson(const char * pImgPath)
{
	SJson * p_result = SJson::CreateObj();
	DbProvider * p_dict = CurDict;
	Reference * p_ref = PPRef;
	SString temp_buf;
	SString img_file_name;
	SString img_file_symb; // Имя (с расширением) файла изображения, скопированного в каталог pImgPath
	S_GUID db_uuid;
	const int hash_alg = SHASHF_SHA256;
	p_dict->GetDbUUID(&db_uuid);
	const SBinaryChunk bc_db_uuid(&db_uuid, sizeof(db_uuid));
	SlHash::GetAlgorithmSymb(hash_alg, temp_buf);
	const SString hash_alg_symb(temp_buf);
	{
		SJson * p_js_list = SJson::CreateArr();
		if(p_js_list) {
			const bool is_img_path_valid = SFile::IsDir(pImgPath);
			SwProgramFilt filt;
			GoodsCore * p_tbl = P_Tbl;
			PPIDArray result_list;
			Goods2Tbl::Key2 k2;
			PPObjGoods goods_obj;
			PPIDArray goods_list;
			BExtQuery q(p_tbl, 2);
			DBQ * dbq = &(p_tbl->Kind == PPGDSK_SWPROGRAM);
			q.select(p_tbl->ID, p_tbl->Name, p_tbl->Abbr, p_tbl->WrOffGrpID, p_tbl->GoodsTypeID, p_tbl->ManufID, p_tbl->Flags, 0L).where(*dbq);
			MEMSZERO(k2);
			k2.Kind = PPGDSK_SWPROGRAM;
			for(q.initIteration(false, &k2, spGe); q.nextIteration() > 0;) {
				PPSwProgram rec;
				if(Helper_GetRec(p_tbl->data, &rec) && rec.CheckForFilt(&filt)) {
					const PPObjID oid(PPOBJ_SWPROGRAM, rec.ID);
					PPSwProgramPacket pack;
					if(Get(rec.ID, &pack) > 0) {
						SJson * p_js_item = PPObjSwProgram::PackToJson(pack);
						if(p_js_item) {
							pack.LinkFiles.Load(pack.Rec.ID, 0L);
							if(pack.LinkFiles.GetCount()) { // @v12.0.5
								const uint blob_idx = 0;
								pack.LinkFiles.At(blob_idx, img_file_name);
								if(fileExists(img_file_name)) {
									SFileFormat ff;
									const int ffr = ff.Identify(img_file_name);
									if(oneof2(ffr, 2, 3) && SImageBuffer::IsSupportedFormat(ff)) {
										SFile f_in(img_file_name, SFile::mRead|SFile::mBinary);
										SBinaryChunk hash;
										if(f_in.CalcHash(0, hash_alg, hash)) {
											if(f_in.IsValid()) {
												if(is_img_path_valid) {
													if(PPObjSwProgram::MakeImgSymb(ff, rec.ID, img_file_symb)) {
														(temp_buf = pImgPath).SetLastSlash().Cat(img_file_symb);
														if(SCopyFile(img_file_name, temp_buf, 0, FILE_SHARE_READ, 0)) {
															p_js_item->InsertString("picsymb", img_file_symb);
														}
													}
												}
												else {
													PPObject::MakeBlobSignature(bc_db_uuid, oid, blob_idx+1, img_file_symb);
													p_js_item->InsertString("picsymb", img_file_symb);
													p_js_item->InsertString("hashalg", hash_alg_symb);
													hash.Mime64(temp_buf);
													p_js_item->InsertString("pichash", temp_buf.Escape());
												}
											}
										}
									}
								}
							}
							p_js_list->InsertChild(p_js_item);
							p_js_item = 0;
						}
					}
				}
			}
			p_result->Insert("list", p_js_list);
		}
	}
	//CATCH
		//ZDELETE(p_result);
	//ENDCATCH
	return p_result;
}

int PPObjSwProgram::ImportFromJson(const SJson * pJs, const char * pImgPathUtf8)
{
	int    ok = -1;
	SString img_file_name;
	PPObjReference pgmcat_obj(PPOBJ_SWPROGRAMCATEGORY, 0);
	if(SJson::IsObject(pJs)) {
		const SJson * p_c = pJs->FindChildByKey("list");
		if(SJson::IsArray(p_c)) {
			PPTransaction tra(1);
			THROW(tra);
			for(const SJson * p_js_inner = p_c->P_Child; p_js_inner; p_js_inner = p_js_inner->P_Next) {
				PPSwProgramPacket pack;
				if(PPObjSwProgram::PackFromJson(p_js_inner, pImgPathUtf8, pack)) {
					// @todo accept packet
					if(!isempty(pack.Rec.Name)) {
						PPID   ex_id = 0;
						if(SearchByName(pack.Rec.Name, &ex_id, 0) > 0) {
							PPSwProgramPacket ex_pack;
							if(Get(ex_id, &ex_pack) > 0) {
								if(pack.LinkFiles.GetCount() && (ex_pack.LinkFiles.GetCount() == 0 || !(ex_pack.Rec.Flags & GF_DERIVED_HASIMAGES))) {
									pack.LinkFiles.At(0, img_file_name);
									if(fileExists(img_file_name)) {
										ex_pack.LinkFiles.Replace(0, img_file_name);
										ex_pack.Rec.Flags |= GF_DERIVED_HASIMAGES;
										THROW(Put(&ex_id, &ex_pack, 0));
									}
								}
							}
						}
						else {
							pack.Rec.ID = 0;
							if(pack.CategoryName_.NotEmpty()) {
								PPID cat_id = 0;
								if(pgmcat_obj.SearchByName(pack.CategoryName_, &cat_id, 0) > 0) {
									pack.Rec.CategoryID = cat_id;
								}
								else {
									ReferenceTbl::Rec cat_rec;
									STRNSCPY(cat_rec.ObjName, pack.CategoryName_);
									if(pgmcat_obj.AddItem(&cat_id, &cat_rec, 0)) {
										pack.Rec.CategoryID = cat_id;
									}
								}
							}
							{
								PPID   new_id = 0;
								if(pack.LinkFiles.GetCount()) {
									pack.LinkFiles.At(0, img_file_name);
									if(fileExists(img_file_name)) {
										pack.Rec.Flags |= GF_DERIVED_HASIMAGES;
									}
								}
								THROW(Put(&new_id, &pack, 0));
							}
						}
						ok = 1;
					}
				}
			}
			THROW(tra.Commit());
		}
	}
	CATCHZOK
	return ok;
}

struct SwProgramImpExpParam {
	SString FilePath;
	SString ImgPath;
};

class SwProgramImpExpDialog : public TDialog {
	enum {
		ctlgroupFile = 1,
		ctlgroupImgDir = 2
	};
	DECL_DIALOG_DATA(SwProgramImpExpParam);
	const uint64 UedImpExp;
public:
	SwProgramImpExpDialog(uint64 uedImpExp) : TDialog(DLG_IMPSWPGM), UedImpExp(uedImpExp)
	{
		long   file_flags = FileBrowseCtrlGroup::fbcgfFile;
		if(UedImpExp == UED_IMPEXP_EXPORT) {
			file_flags |= FileBrowseCtrlGroup::fbcgfAllowNExists;
		}
		FileBrowseCtrlGroup::Setup(this, CTLBRW_IMPSWPGM_FILE, CTL_IMPSWPGM_FILE, ctlgroupFile, 0, PPTXT_FILPAT_JSON, file_flags);
		FileBrowseCtrlGroup::Setup(this, CTLBRW_IMPSWPGM_IMGDIR, CTL_IMPSWPGM_IMGDIR, ctlgroupImgDir,
			0, PPTXT_FILPAT_JSON, FileBrowseCtrlGroup::fbcgfPath);
		setTitle((UedImpExp == UED_IMPEXP_EXPORT) ? "@exportswprogram" : "@importswprogram");
		setLabelText(CTL_IMPSWPGM_FILE, (UedImpExp == UED_IMPEXP_EXPORT) ? "@exportswprogram_file" : "@importswprogram_file");
	}
	DECL_DIALOG_SETDTS()
	{
		int    ok = 1;
		RVALUEPTR(Data, pData);
		setCtrlString(CTL_IMPSWPGM_FILE, Data.FilePath);
		setCtrlString(CTL_IMPSWPGM_IMGDIR, Data.ImgPath);
		return ok;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		getCtrlString(CTL_IMPSWPGM_FILE, Data.FilePath);
		getCtrlString(CTL_IMPSWPGM_IMGDIR, Data.ImgPath);
		ASSIGN_PTR(pData, Data);
		return ok;
	}
};

int PPViewSwProgram::Export()
{
	int    ok = -1;
	SString temp_buf;
	SwProgramImpExpParam param;
	SwProgramImpExpDialog * dlg = new SwProgramImpExpDialog(UED_IMPEXP_EXPORT);
	if(CheckDialogPtrErr(&dlg)) {
		dlg->setDTS(&param);
		if(ExecView(dlg) == cmOK) {
			dlg->getDTS(&param);
			SJson * p_js = Obj.ExportToJson(param.ImgPath);
			if(p_js) {
				p_js->ToStr(temp_buf);
				SFile f_out(param.FilePath, SFile::mWrite);
				if(f_out.IsValid()) {
					f_out.Write(temp_buf.cptr(), temp_buf.Len());
				}
			}
		}
	}
	delete dlg;
	return ok;
}

int ImportSwProgram()
{
	int    ok = -1;
	SwProgramImpExpParam param;
	SwProgramImpExpDialog * dlg = new SwProgramImpExpDialog(UED_IMPEXP_IMPORT);
	if(CheckDialogPtrErr(&dlg)) {
		dlg->setDTS(&param);
		if(ExecView(dlg) == cmOK) {
			dlg->getDTS(&param);
			if(fileExists(param.FilePath)) {
				param.ImgPath.Transf(CTRANSF_INNER_TO_UTF8);
				if(!SFile::IsDir(param.ImgPath)) {
					SFsPath ps(param.FilePath);
					ps.Merge(SFsPath::fDrv|SFsPath::fDir, param.ImgPath);
				}
				PPObjSwProgram swpgm_obj;
				SJson * p_js = SJson::ParseFile(param.FilePath);
				if(p_js) {
					swpgm_obj.ImportFromJson(p_js, param.ImgPath);
					ZDELETE(p_js);
				}
			}
		}
	}
	delete dlg;
	return ok;
}
//
//
//
IMPLEMENT_PPFILT_FACTORY(SwProgram); SwProgramFilt::SwProgramFilt() : PPBaseFilt(PPFILT_SWPROGRAM, 0, 1)
{
	SetFlatChunk(offsetof(SwProgramFilt, ReserveStart), offsetof(SwProgramFilt, SrchStr) - offsetof(SwProgramFilt, ReserveStart));
	SetBranchSString(offsetof(SwProgramFilt, SrchStr));
	SetBranchObjIdListFilt(offsetof(SwProgramFilt, ParentList));
	Init(1, 0);
}
	
SwProgramFilt & FASTCALL SwProgramFilt::operator = (const SwProgramFilt & rS)
{
	Copy(&rS, 1);
	return *this;
}

/*virtual*/bool SwProgramFilt::IsEmpty() const
{
	return (Flags == 0 && SrchStr.IsEmpty() && ParentList.GetCount() == 0);
}

PPViewSwProgram::BrwItem::BrwItem(const PPSwProgram * pS) : ID(0), CategoryID(0), Flags(0), ViewFlags(0)
{
	Name[0] = 0;
	ExeFn[0] = 0;
	if(pS) {
		ID = pS->ID;
		Flags = pS->Flags;
		CategoryID = pS->CategoryID;
		STRNSCPY(Name, pS->Name);
		STRNSCPY(ExeFn, pS->ExeFn);
	}
}

PPViewSwProgram::PPViewSwProgram() : PPView(&Obj, &Filt, PPVIEW_SWPROGRAM, PPView::implBrowseArray|PPView::implDontEditNullFilter, /*defReportId*/0), P_DsList(0)
{
}
	
PPViewSwProgram::~PPViewSwProgram()
{
	ZDELETE(P_DsList);
}
	
/*virtual*/int PPViewSwProgram::Init_(const PPBaseFilt * pBaseFilt)
{
	int    ok = 1;
	THROW(Helper_InitBaseFilt(pBaseFilt));
	BExtQuery::ZDelete(&P_IterQuery);
	Counter.Init();
	CATCHZOK
	return ok;
}
	
/*virtual*/int PPViewSwProgram::EditBaseFilt(PPBaseFilt * pBaseFilt)
{
	int    ok = -1;
	return ok;
}
	
int PPViewSwProgram::InitIteration()
{
	return MakeList();
}
	
int FASTCALL PPViewSwProgram::NextIteration(SwProgramViewItem * pItem)
{
	int    ok = -1;
	while(ok < 0 && P_DsList && P_DsList->getPointer() < P_DsList->getCount()) {
		const  PPID id = static_cast<const BrwItem *>(P_DsList->at(P_DsList->getPointer()))->ID;
		PPSwProgramPacket pack;
		if(Obj.Get(id, &pack) > 0) {
			ASSIGN_PTR(pItem, pack.Rec);
			P_DsList->incPointer();
			ok = 1;
		}
	}
	return ok;
}

static int PPViewSwProgram_CellStyleFunc(const void * pData, long col, int paintAction, BrowserWindow::CellStyle * pStyle, void * extraPtr)
{
	int    ok = -1;
	PPViewBrowser * p_brw = static_cast<PPViewBrowser *>(extraPtr);
	if(p_brw) {
		PPViewSwProgram * p_view = static_cast<PPViewSwProgram *>(p_brw->P_View);
		ok = p_view ? p_view->CellStyleFunc_(pData, col, paintAction, pStyle, p_brw) : -1;
	}
	return ok;
}
	
/*static*/int PPViewSwProgram::CellStyleFunc_(const void * pData, long col, int paintAction, BrowserWindow::CellStyle * pCellStyle, PPViewBrowser * pBrw)
{
	int    ok = -1;
	if(pBrw && pData && pCellStyle && col >= 0) {
		BrowserDef * p_def = pBrw->getDef();
		if(col >= 0 && col < static_cast<long>(p_def->getCount())) {
			const BroColumn & r_col = p_def->at(col);
			if(col == 0) { // id
				const BrwItem * p_item = static_cast<const BrwItem *>(pData);
				if(p_item->Flags & GF_DERIVED_HASIMAGES) { // @todo replace GF_DERIVED_HASIMAGES with ...
					pCellStyle->Flags |= BrowserWindow::CellStyle::fLeftBottomCorner;
					pCellStyle->Color2 = GetColorRef(SClrGreen);
					ok = 1;
				}
			}
		}
	}
	return ok;
}

/*static*/int FASTCALL PPViewSwProgram::GetDataForBrowser(SBrowserDataProcBlock * pBlk)
{
	PPViewSwProgram * p_v = static_cast<PPViewSwProgram *>(pBlk->ExtraPtr);
	return p_v ? p_v->_GetDataForBrowser(pBlk) : 0;
}
	
/*virtual*/SArray * PPViewSwProgram::CreateBrowserArray(uint * pBrwId, SString * pSubTitle)
{
	SArray * p_array = 0;
	PPBrand ds_item;
	THROW(MakeList());
	p_array = new SArray(*P_DsList);
	CATCH
		ZDELETE(P_DsList);
	ENDCATCH
	ASSIGN_PTR(pBrwId, BROWSER_SWPROGRAM);
	return p_array;
}
	
/*virtual*/void PPViewSwProgram::PreprocessBrowser(PPViewBrowser * pBrw)
{
	if(pBrw) {
		pBrw->SetDefUserProc(PPViewSwProgram::GetDataForBrowser, this);
		pBrw->SetCellStyleFunc(PPViewSwProgram_CellStyleFunc, pBrw);
	}
}
	
/*virtual*/int PPViewSwProgram::OnExecBrowser(PPViewBrowser *)
{
	int    ok = -1;
	return ok;
}

/*virtual*/int PPViewSwProgram::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	PPID   preserve_id = 0;
	if(ok == -2) {
		PPID   id = pHdr ? *static_cast<const  PPID *>(pHdr) : 0;
		preserve_id = id;
		switch(ppvCmd) {
			case PPVCMD_EXPORT:
				ok = Export();
				break;
			case PPVCMD_MOUSEHOVER:
				if(id && static_cast<const BrwItem *>(pHdr)->Flags & GF_DERIVED_HASIMAGES) {
					SString img_path;
					ObjLinkFiles link_files(PPOBJ_SWPROGRAM);
					link_files.Load(id, 0L);
					link_files.At(0, img_path);
					PPTooltipMessage(0, img_path, pBrw->H(), 10000, 0, SMessageWindow::fShowOnCursor|SMessageWindow::fCloseOnMouseLeave|
						SMessageWindow::fOpaque|SMessageWindow::fSizeByText|SMessageWindow::fChildWindow);
				}
				break;
			case PPVCMD_REFRESH:
				ok = 1;
				break;
		}
	}
	if(ok > 0) {
		MakeList();
		if(pBrw) {
			AryBrowserDef * p_def = pBrw ? static_cast<AryBrowserDef *>(pBrw->getDef()) : 0;
			if(p_def) {
				SArray * p_array = new SArray(*P_DsList);
				p_def->setArray(p_array, 0, 1);
				pBrw->setRange(p_array->getCount());
				uint   temp_pos = 0;
				long   update_pos = -1;
				if(preserve_id > 0 && P_DsList->lsearch(&preserve_id, &temp_pos, CMPF_LONG))
					update_pos = temp_pos;
				if(update_pos >= 0)
					pBrw->go(update_pos);
				else if(update_pos == MAXLONG)
					pBrw->go(p_array->getCount()-1);
			}
			pBrw->Update();
		}
	}
	return ok;
}
	
/*virtual*/void PPViewSwProgram::ViewTotal()
{
}
	
int PPViewSwProgram::_GetDataForBrowser(SBrowserDataProcBlock * pBlk)
{
	int    ok = 0;
	if(pBlk->P_SrcData && pBlk->P_DestData) {
		ok = 1;
		const  BrwItem * p_item = static_cast<const BrwItem *>(pBlk->P_SrcData);
		int    r = 0;
		switch(pBlk->ColumnN) {
			case 0: pBlk->Set(p_item->ID); break; // @id
			case 1: pBlk->Set(p_item->Name); break; // @name
			case 2: 
				{
					bool settled = false;
					if(p_item->CategoryID) {
						Reference2Tbl::Rec cat_rec;
						if(PPRef->GetItem(PPOBJ_SWPROGRAMCATEGORY, p_item->CategoryID, &cat_rec) > 0) {
							pBlk->Set(cat_rec.ObjName);
							settled = true;
						}
					}
					if(!settled)
						pBlk->Set("");
				}
				break; // category
			case 3: pBlk->Set(p_item->ExeFn); break; // exe file name
		}
	}
	return ok;
}
	
int PPViewSwProgram::MakeList()
{
	int    ok = 1;
	PPSwProgram item;
	GoodsCore * p_tbl = Obj.P_Tbl;
	PPIDArray result_list;
	Goods2Tbl::Key2 k2;
	PPObjGoods goods_obj;
	PPIDArray goods_list;
	BExtQuery q(p_tbl, 2);
	DBQ * dbq = &(p_tbl->Kind == PPGDSK_SWPROGRAM);
	if(P_DsList)
		P_DsList->clear();
	else
		P_DsList = new SArray(sizeof(BrwItem));
	q.select(p_tbl->ID, p_tbl->Name, p_tbl->Abbr, p_tbl->WrOffGrpID, p_tbl->GoodsTypeID, p_tbl->ManufID, p_tbl->Flags, 0L).where(*dbq);
	MEMSZERO(k2);
	k2.Kind = PPGDSK_SWPROGRAM;
	for(q.initIteration(false, &k2, spGe); q.nextIteration() > 0;) {
		PPSwProgram rec;
		if(Obj.Helper_GetRec(p_tbl->data, &rec) && rec.CheckForFilt(&Filt)) {
			BrwItem new_item(&rec);
			//if(Filt.Flags & Filt.fShowGoodsCount) {
				//single_brand_list.clear();
				//single_brand_list.add(rec.ID);
				//goods_obj.P_Tbl->GetListByBrandList(single_brand_list, goods_list);
				//new_item.LinkGoodsCount = goods_list.getCount();
			//}
			THROW_SL(P_DsList->insert(&new_item));
		}
	}
	CATCHZOK
	CALLPTRMEMB(P_DsList, setPointer(0));
	return ok;
}

