// ALCODECL-RU.CPP
// Copyright (c) A.Sobolev 2021
// @codepage UTF-8
// Алкогольная декларация (Россия)
//
#include <pp.h>
#pragma hdrstop

IMPLEMENT_PPFILT_FACTORY(AlcoDeclRu); AlcoDeclRuFilt::AlcoDeclRuFilt() : PPBaseFilt(PPFILT_ALCODECLRU, 0, 0)
{
	SetFlatChunk(offsetof(AlcoDeclRuFilt, ReserveStart),
		offsetof(AlcoDeclRuFilt, Reserve)-offsetof(AlcoDeclRuFilt, ReserveStart)+sizeof(Reserve));
	SetBranchObjIdListFilt(offsetof(AlcoDeclRuFilt, LocList));
	SetBranchSString(offsetof(AlcoDeclRuFilt, AlcoCodeList));
	Init(1, 0);
}

PPViewAlcoDeclRu::InnerRcptEntry::InnerRcptEntry()
{
	THISZERO();
}

PPViewAlcoDeclRu::InnerMovEntry::InnerMovEntry()
{
	THISZERO();
}

PPViewAlcoDeclRu::PPViewAlcoDeclRu() : PPView(0, &Filt, PPVIEW_ALCODECLRU, implBrowseArray, 0), P_BObj(BillObj)
{
	Arp.SetConfig(0);
	Arp.Init();
}

PPViewAlcoDeclRu::~PPViewAlcoDeclRu()
{
}

long PPViewAlcoDeclRu::GetAlcoCodeIdent(const char * pCode)
{
	long   ident = 0;
	if(!isempty(pCode)) {
		uint    pos = 0;
		if(AlcoCodeList.SearchByText(pCode, 1, &pos)) {
			ident = AlcoCodeList.at_WithoutParent(pos).Id;
		}
		else {
			long   max_id = 0;
			AlcoCodeList.GetMaxID(&max_id);
			ident = max_id+1;
			AlcoCodeList.AddFast(ident, pCode);
			{
				pos = 0;
				assert(AlcoCodeList.SearchByText(pCode, 1, &pos) && AlcoCodeList.at_WithoutParent(pos).Id == ident);
			}
		}
	}
	return ident;
}

int PPViewAlcoDeclRu::GetAlcoCode(long id, SString & rBuf) const
{
	int    ok = 0;
	rBuf.Z();
	uint   pos = 0;
	if(AlcoCodeList.Search(id, &pos)) {
		rBuf = AlcoCodeList.at_WithoutParent(pos).Txt;
		ok = 1;
	}
	return ok;
}

int PPViewAlcoDeclRu::AddRcptItem(const PrcssrAlcReport::GoodsItem & rGi, const TrfrAnlzViewItem_AlcRep & rTi, TSArray <InnerRcptEntry> & rList)
{
	int    ok = 1;
	const  long alco_code_ident = GetAlcoCodeIdent(rGi.CategoryCode);
	if(alco_code_ident) {
		uint   item_idx = 0;
		for(uint i = 0; !item_idx && i < rList.getCount(); i++) {
			const InnerRcptEntry & r_item = rList.at(i);
			if(r_item.ManufID == rGi.MnfOrImpPsnID && r_item.BillID == rTi.BillRec.ID && alco_code_ident == r_item.AlcoCodeId)
				item_idx = i+1;
		}
		{
			if(!item_idx) {
				InnerRcptEntry new_item;
				new_item.ManufID = rGi.MnfOrImpPsnID;
				new_item.BillID = rTi.BillRec.ID;
				new_item.AlcoCodeId = alco_code_ident;
				rList.insert(&new_item);
				item_idx = rList.getCount();
			}
			assert(item_idx > 0 && item_idx <= rList.getCount());
			{
				InnerRcptEntry & r_item = rList.at(item_idx-1);
				r_item.BillDt = rTi.BillRec.Dt;
				r_item.Qtty += (rTi.Item.Qtty * rGi.Volume) / 10.0;
			}
		}
	}
	return ok;
}

uint PPViewAlcoDeclRu::GetMovListItemIdx(long alcoCodeIdent, PPID manufID)
{
	uint   item_idx = 0;
	for(uint i = 0; !item_idx && i < MovList.getCount(); i++) {
		const InnerMovEntry & r_item = MovList.at(i);
		if(r_item.ManufID == manufID && alcoCodeIdent == r_item.AlcoCodeId)
			item_idx = i+1;
	}
	if(!item_idx) {
		InnerMovEntry new_item;
		new_item.ManufID = manufID;
		new_item.AlcoCodeId = alcoCodeIdent;
		MovList.insert(&new_item);
		item_idx = MovList.getCount();
	}
	return item_idx;
}

int PPViewAlcoDeclRu::AddMovItem(const PrcssrAlcReport::GoodsItem & rGi, const TrfrAnlzViewItem_AlcRep & rTi, TSArray <InnerMovEntry> & rList)
{
	int    ok = 1;
	const  long alco_code_ident = GetAlcoCodeIdent(rGi.CategoryCode);
	if(alco_code_ident) {
		uint   item_idx = GetMovListItemIdx(alco_code_ident, rGi.MnfOrImpPsnID);
		assert(item_idx > 0 && item_idx <= rList.getCount());
		{
			InnerMovEntry & r_item = rList.at(item_idx-1);
			if(rGi.Volume > 0.0) {
				const double qtty_dal = fabs((rTi.Item.Qtty * rGi.Volume) / 10.0);
				const PPID op_id = rTi.Item.OpID;
				if(op_id) {
					int introp = IsIntrOp(op_id);
					if(introp == INTREXPND) {
						r_item.ExpIntr += qtty_dal;
					}
					else if(introp == INTRRCPT) {
						r_item.RcptIntr += qtty_dal;
					}
					else if(op_id == CConfig.RetailOp) {
						r_item.ExpRetail += qtty_dal;
					}
					else {
						PPOprKind op_rec;
						const PPID op_type_id = GetOpType(op_id, &op_rec);
						if(op_type_id == PPOPT_GOODSRECEIPT) {
							if(op_rec.AccSheetID == GetSupplAccSheet())
								r_item.RcptWhs += qtty_dal;
							else if(op_rec.AccSheetID == GetSellAccSheet())
								r_item.SaleRet += qtty_dal;
							else 
								r_item.RcptEtc += qtty_dal;
						}
						else if(op_type_id == PPOPT_GOODSRETURN && op_rec.LinkOpID && op_rec.LinkOpID == CConfig.RetailOp) {
							r_item.SaleRet += qtty_dal;
						}
						else if(op_type_id == PPOPT_GOODSRETURN && GetOpType(op_rec.LinkOpID) == PPOPT_GOODSRECEIPT) {
							r_item.SupplRet += qtty_dal;
						}
						else if(op_type_id == PPOPT_GOODSRETURN && GetOpType(op_rec.LinkOpID) == PPOPT_GOODSEXPEND) {
							r_item.SaleRet += qtty_dal;
						}
						else if(op_type_id == PPOPT_GOODSEXPEND && op_rec.AccSheetID == GetSellAccSheet()) {
							r_item.ExpEtc += qtty_dal;
						}
						else if(rTi.Item.Qtty < 0.0)
							r_item.ExpEtc += qtty_dal;
						else if(rTi.Item.Qtty > 0.0)
							r_item.RcptEtc += qtty_dal;
					}
				}
			}
		}
	}
	return ok;
}

void PPViewAlcoDeclRu::ProcessStock(int startOrEnd, const PPIDArray & rGoodsList)
{
	const uint _gc = rGoodsList.getCount();
	Transfer * trfr = P_BObj->trfr;
	ReceiptTbl::Rec lot_rec;
	ReceiptTbl::Rec org_lot_rec;
	PrcssrAlcReport::GoodsItem alc_goods_ext;
	SString wait_msg_buf;
	PPLoadText(PPTXT_WAIT_GOODSREST, wait_msg_buf);
	for(uint i = 0; i < _gc; i++) {
		PPID   goods_id = rGoodsList.get(i);
		GoodsRestParam gp;
		gp.GoodsID = goods_id;
		Filt.LocList.Get(gp.LocList);
		if(startOrEnd == 0)
			gp.Date = plusdate(Filt.Period.low, -1);
		else 
			gp.Date = Filt.Period.upp;
		gp.DiffParam |= GoodsRestParam::_diffLotID;
		trfr->GetRest(gp);
		for(uint lidx = 0; lidx < gp.getCount(); lidx++) {
			const GoodsRestVal & r_val = gp.at(lidx);
			assert(r_val.Rest > 0.0);
			PPID   org_lot_id = 0;
			if(trfr->Rcpt.SearchOrigin(r_val.LotID, &org_lot_id, &lot_rec, &org_lot_rec)) { 
				if(Arp.IsAlcGoods(goods_id) > 0 && Arp.PreprocessGoodsItem(goods_id, org_lot_id, 0, 0, alc_goods_ext) > 0) {
					const long alco_code_ident = GetAlcoCodeIdent(alc_goods_ext.CategoryCode);
					if(alco_code_ident) {
						if(!alc_goods_ext.MnfOrImpPsnID) {
							PPID   manuf_id = 0;
							if(Arp.GetLotManufID(org_lot_id, &manuf_id, 0) > 0)
								alc_goods_ext.MnfOrImpPsnID = manuf_id;
						}
						uint   item_idx = GetMovListItemIdx(alco_code_ident, alc_goods_ext.MnfOrImpPsnID);
						assert(item_idx > 0 && item_idx <= MovList.getCount());
						{
							InnerMovEntry & r_list_item = MovList.at(item_idx-1);
							if(alc_goods_ext.Volume > 0.0) {
								const double qtty_dal = fabs((r_val.Rest * alc_goods_ext.Volume) / 10.0);
								if(startOrEnd == 0)
									r_list_item.StockBeg += qtty_dal;
								else
									r_list_item.StockEnd += qtty_dal;
							}
						}
					}
				}
			}
		}
		PPWaitPercent(i+1, _gc, wait_msg_buf);
	}
}

/*virtual*/int PPViewAlcoDeclRu::Init_(const PPBaseFilt * pBaseFilt)
{
	int    ok = 1;
	SString wait_msg_buf;
	THROW(Helper_InitBaseFilt(pBaseFilt));
	{
		RcptList.clear();
		MovList.clear();
		BExtQuery::ZDelete(&P_IterQuery);
		Filt.Period.Actualize(ZERODATE);
		{
			PPIDArray goods_list;
			PPViewTrfrAnlz ta_view;
			TrfrAnlzFilt ta_filt;
			TrfrAnlzViewItem_AlcRep ta_item;
			PrcssrAlcReport::GoodsItem alc_goods_ext;
			PersonTbl::Rec manuf_psn_rec;
			ta_filt.Period = Filt.Period;
			ta_filt.LocList = Filt.LocList;
			THROW(ta_view.Init_(&ta_filt));
			PPWaitStart();
			for(ta_view.InitIteration(PPViewTrfrAnlz::OrdByDate); ta_view.NextIteration_AlcRep(&ta_item) > 0;) {
				const PPID goods_id = ta_item.Item.GoodsID;
				if(Arp.IsAlcGoods(goods_id) > 0 && Arp.PreprocessGoodsItem(goods_id, ta_item.OrgLotRec.ID, 0, 0, alc_goods_ext) > 0) {
					const bool is_rcpt = (ta_item.OrgLotRec.ID == ta_item.Item.LotID && GetOpType(ta_item.Item.OpID, 0) == PPOPT_GOODSRECEIPT);
					if(!alc_goods_ext.MnfOrImpPsnID) {
						PPID   manuf_id = 0;
						if(Arp.GetLotManufID(ta_item.OrgLotRec.ID, &manuf_id, 0) > 0)
							alc_goods_ext.MnfOrImpPsnID = manuf_id;
					}
					if(is_rcpt) {
						AddRcptItem(alc_goods_ext, ta_item, RcptList);
					}
					{
						AddMovItem(alc_goods_ext, ta_item, MovList);
					}
					goods_list.add(ta_item.Item.GoodsID);
				}
				PPWaitPercent(ta_view.GetCounter());
			}
			{
				goods_list.sortAndUndup();
				{
					PPIDArray _gl;
					Arp.GetAlcGoodsList(_gl);
					_gl.add(&goods_list);
					_gl.sortAndUndup();
					goods_list = _gl;
				}
				ProcessStock(0, goods_list);
				ProcessStock(1, goods_list);
			}
		}
	}
	CATCHZOK
	PPWaitStop();
	return ok;
}

class AlcoDeclFiltDialog : public TDialog {
	DECL_DIALOG_DATA(AlcoDeclRuFilt);
	enum {
		ctlgroupLoc = 1
	};
public:
	AlcoDeclFiltDialog() : TDialog(DLG_ALCODECL)
	{
		addGroup(ctlgroupLoc, new LocationCtrlGroup(CTLSEL_ALCODECL_LOC, 0, 0, cmLocList, 0, 0, 0));
	}
	DECL_DIALOG_SETDTS()
	{
		int    ok = 1;
		RVALUEPTR(Data, pData);
		//
		SetPeriodInput(this, CTL_ALCODECL_PERIOD, &Data.Period);
		{
			LocationCtrlGroup::Rec l_rec(&Data.LocList);
			setGroupData(ctlgroupLoc, &l_rec);
		}
		{
			SString code_list;
			SString temp_buf;
			StringSet ss;
			Data.AlcoCodeList.Tokenize(",; ", ss);
			for(uint ssp = 0; ss.get(&ssp, temp_buf);) {
				code_list.CatDivIfNotEmpty(',', 2).Cat(temp_buf);
			}
			setCtrlString(CTL_ALCODECL_ACODLIST, code_list);
		}
		{
			// 1 - beer
			// 2 - alco
			long   val = 0;
			if(Data.Flags & Data.fOnlyBeer)
				val = 1;
			else if(Data.Flags & Data.fOnlyNonBeerAlco)
				val = 2;
			else
				val = 1;
			AddClusterAssocDef(CTL_ALCODECL_PRODGRP, 0, 1);
			AddClusterAssoc(CTL_ALCODECL_PRODGRP, 1, 2);
			SetClusterData(CTL_ALCODECL_PRODGRP, val);
		}
		{
			// 1 - mov
			// 2 - rcpt
			long   val = 0;
			if(Data.Flags & Data.fShowAsRcpt)
				val = 2;
			else
				val = 1;
			AddClusterAssocDef(CTL_ALCODECL_VIEW, 0, 1);
			AddClusterAssoc(CTL_ALCODECL_VIEW, 1, 2);
			SetClusterData(CTL_ALCODECL_VIEW, val);
		}
		return ok;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		GetPeriodInput(this, CTL_ALCODECL_PERIOD, &Data.Period);
		{
			LocationCtrlGroup::Rec l_rec;
			getGroupData(ctlgroupLoc, &l_rec);
			Data.LocList = l_rec.LocList;
		}
		{
			Data.AlcoCodeList.Z();
			SString code_list;
			getCtrlString(CTL_ALCODECL_ACODLIST, code_list);
			SString temp_buf;
			StringSet ss;
			code_list.Tokenize(",; ", ss);
			for(uint ssp = 0; ss.get(&ssp, temp_buf);) {
				Data.AlcoCodeList.CatDivIfNotEmpty(',', 2).Cat(temp_buf);
			}
		}
		{
			long val = GetClusterData(CTL_ALCODECL_PRODGRP);
			Data.Flags &= ~(Data.fOnlyBeer|Data.fOnlyNonBeerAlco);
			if(val == 1)
				Data.Flags |= Data.fOnlyBeer;
			else if(val == 2)
				Data.Flags |= Data.fOnlyNonBeerAlco;
			else
				Data.Flags |= Data.fOnlyBeer;
		}
		{
			long val = GetClusterData(CTL_ALCODECL_VIEW);
			SETFLAG(Data.Flags, Data.fShowAsRcpt, val == 2);
		}
		//
		ASSIGN_PTR(pData, Data);
		return ok;
	}
};

/*virtual*/int PPViewAlcoDeclRu::EditBaseFilt(PPBaseFilt * pBaseFilt)
{
	if(!Filt.IsA(pBaseFilt))
		return PPErrorZ();
	AlcoDeclRuFilt * p_filt = static_cast<AlcoDeclRuFilt *>(pBaseFilt);
	DIALOG_PROC_BODY(AlcoDeclFiltDialog, p_filt);
}

int PPViewAlcoDeclRu::InitIteration()
{
	int    ok = -1;
	return ok;
}

int FASTCALL PPViewAlcoDeclRu::NextIteration(AlcoDeclRuViewItem * pItem)
{
	int    ok = -1;
	return ok;
}

int FASTCALL PPViewAlcoDeclRu::_GetDataForBrowser(SBrowserDataProcBlock * pBlk)
{
	int    ok = 0;
	if(pBlk->P_SrcData && pBlk->P_DestData) {
		ok = 1;
		SString temp_buf;
		int    r = 0;
		if(Filt.Flags & AlcoDeclRuFilt::fShowAsRcpt) {
			/*
				browser ALCODECLRU_RCPT north(100), 1, 0, "", OWNER|GRID, 0
				{
					"Код продукции",              1,  zstring(16),   0, 0, BCO_USERPROC
					"Производитель/импортер",     2,  zstring(128),  0, 0, BCO_USERPROC
					"@billno",                    3,  zstring(32),   0, 0, BCO_USERPROC
					"@billdate",                  4,  date,          DATF_DMY|DATF_CENTURY, 0, BCO_USERPROC
					"@cargocustomsdeclaration_s", 5,  zstring(32),   0, 0, BCO_USERPROC
					"@qtty",                      6,  double,        NMBF_NOZERO, 10.2, BCO_USERPROC
				}
			*/
			const InnerRcptEntry * p_item = static_cast<const InnerRcptEntry *>(pBlk->P_SrcData);
			switch(pBlk->ColumnN) {
				case 1:
					GetAlcoCode(p_item->AlcoCodeId, temp_buf);
					pBlk->Set(temp_buf);
					break;
				case 2:
					GetPersonName(p_item->ManufID, temp_buf);
					pBlk->Set(temp_buf);
					break;
				case 3:
					{
						BillTbl::Rec bill_rec;
						temp_buf.Z();
						if(P_BObj->Fetch(p_item->BillID, &bill_rec) > 0)
							temp_buf = bill_rec.Code;
						pBlk->Set(temp_buf);
					}
					break;
				case 4:
					pBlk->Set(p_item->BillDt);
					break;
				case 5:
					StrPool.GetS(p_item->ClbP, temp_buf);
					pBlk->Set(temp_buf);
					break;
				case 6:
					pBlk->Set(p_item->Qtty);
					break;
			}
		}
		else {
			/*
				browser ALCODECLRU_MOV north(100), 1, 0, "", OWNER|GRID, 0
				{
					"Код продукции",             1,  zstring(16),   0, 0, BCO_USERPROC
					"Производитель/импортер",    2,  zstring(128),  0, 0, BCO_USERPROC
					"Остаток на начало",         3,  double,   NMBF_NOZERO, 10.2, BCO_USERPROC
					"Остаток на конец",          4,  double,   NMBF_NOZERO, 10.2, BCO_USERPROC
					"Приход от производителя",   5,  double,   NMBF_NOZERO, 10.2, BCO_USERPROC
					"Приход от дистрибьютора",   6,  double,   NMBF_NOZERO, 10.2, BCO_USERPROC
					"Приход от импортера",       7,  double,   NMBF_NOZERO, 10.2, BCO_USERPROC
					"Межскладской приход",       8,  double,   NMBF_NOZERO, 10.2, BCO_USERPROC
					"Прочий приход",             9,  double,   NMBF_NOZERO, 10.2, BCO_USERPROC
					"@selling_retail",          10,  double,   NMBF_NOZERO, 10.2, BCO_USERPROC
					"Прочий расход",            11,  double,   NMBF_NOZERO, 10.2, BCO_USERPROC
					"Возврат поставщику",       12,  double,   NMBF_NOZERO, 10.2, BCO_USERPROC
					"Возврат от покупателей",   13,  double,   NMBF_NOZERO, 10.2, BCO_USERPROC
					"Межскладской расход",      14,  double,   NMBF_NOZERO, 10.2, BCO_USERPROC
					"Баланс",                   15,  double,   NMBF_NOZERO, 10.2, BCO_USERPROC
				}
			*/
			const InnerMovEntry * p_item = static_cast<const InnerMovEntry *>(pBlk->P_SrcData);
			switch(pBlk->ColumnN) {
				case 1:
					GetAlcoCode(p_item->AlcoCodeId, temp_buf);
					pBlk->Set(temp_buf);
					break;
				case 2:
					GetPersonName(p_item->ManufID, temp_buf);
					pBlk->Set(temp_buf);
					break;
				case 3:
					pBlk->Set(p_item->StockBeg);
					break;
				case 4:
					pBlk->Set(p_item->StockEnd);
					break;
				case 5:
					pBlk->Set(p_item->RcptManuf);
					break;
				case 6:
					pBlk->Set(p_item->RcptWhs);
					break;
				case 7:
					pBlk->Set(p_item->RcptImp);
					break;
				case 8:
					pBlk->Set(p_item->RcptIntr);
					break;
				case 9:
					pBlk->Set(p_item->RcptEtc);
					break;
				case 10:
					pBlk->Set(p_item->ExpRetail);
					break;
				case 11:
					pBlk->Set(p_item->ExpEtc);
					break;
				case 12:
					pBlk->Set(p_item->SupplRet);
					break;
				case 13:
					pBlk->Set(p_item->SaleRet);
					break;
				case 14:
					pBlk->Set(p_item->ExpIntr);
					break;
				case 15:
					{
						double balance = p_item->StockBeg - p_item->StockEnd + p_item->RcptManuf + p_item->RcptWhs + p_item->RcptImp +
							p_item->RcptIntr + p_item->RcptEtc + p_item->SaleRet - p_item->ExpRetail - p_item->ExpIntr -
							p_item->ExpEtc - p_item->SupplRet;
						pBlk->Set(balance);
					}
					break;
			}
		}
	}
	return ok;
}

/*static*/int FASTCALL PPViewAlcoDeclRu::GetDataForBrowser(SBrowserDataProcBlock * pBlk)
{
	PPViewAlcoDeclRu * p_v = static_cast<PPViewAlcoDeclRu *>(pBlk->ExtraPtr);
	return p_v ? p_v->_GetDataForBrowser(pBlk) : 0;
}

void PPViewAlcoDeclRu::PreprocessBrowser(PPViewBrowser * pBrw)
{
	if(pBrw) {
		pBrw->SetDefUserProc(PPViewAlcoDeclRu::GetDataForBrowser, this);
		//pBrw->SetCellStyleFunc(CellStyleFunc, pBrw);
		//pBrw->Helper_SetAllColumnsSortable();
	}
}

/*virtual*/SArray * PPViewAlcoDeclRu::CreateBrowserArray(uint * pBrwId, SString * pSubTitle)
{
	SArray * p_array = 0;
	uint   brw_id = 0;
	if(Filt.Flags & AlcoDeclRuFilt::fShowAsRcpt) {
		p_array = new SArray(RcptList);
		brw_id = BROWSER_ALCODECLRU_RCPT;
	}
	else {
		p_array = new SArray(MovList);
		brw_id = BROWSER_ALCODECLRU_MOV;
	}
	ASSIGN_PTR(pBrwId, brw_id);
	return p_array;
}
