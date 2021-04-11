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
	SetBranchObjIdListFilt(offsetof(AlcoDeclRuFilt, DivList));
	SetBranchSString(offsetof(AlcoDeclRuFilt, AlcoCodeList));
	Init(1, 0);
}

int AlcoDeclRuFilt::IsEqualExcept(const AlcoDeclRuFilt & rS, long flags) const
{
	#define NEQ_FLD(f) (f) != (rS.f)
	if(NEQ_FLD(Period))
		return 0;
	if(NEQ_FLD(AlcoCodeList))
		return 0;
	if(!DivList.IsEqual(rS.DivList))
		return 0;
	#undef NEQ_FLD
	if(flags & eqxShowMode) {
		if((Flags & ~fShowAsRcpt) != (rS.Flags & ~fShowAsRcpt))
			return 0;
	}
	else {
		if(Flags != rS.Flags)
			return 0;
	}
	return 1;
}

PPViewAlcoDeclRu::InnerRcptEntry::InnerRcptEntry()
{
	THISZERO();
}

PPViewAlcoDeclRu::InnerMovEntry::InnerMovEntry()
{
	THISZERO();
}

PPViewAlcoDeclRu::PPViewAlcoDeclRu() : PPView(0, &Filt, PPVIEW_ALCODECLRU, implBrowseArray, 0), P_BObj(BillObj), State(0)
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

uint PPViewAlcoDeclRu::GetMovListItemIdx(PPID divID, long alcoCodeIdent, PPID manufID)
{
	uint   item_idx = 0;
	for(uint i = 0; !item_idx && i < MovList.getCount(); i++) {
		const InnerMovEntry & r_item = MovList.at(i);
		if(r_item.DivID == divID && r_item.ManufID == manufID && alcoCodeIdent == r_item.AlcoCodeId)
			item_idx = i+1;
	}
	if(!item_idx) {
		InnerMovEntry new_item;
		new_item.DivID = divID;
		new_item.ManufID = manufID;
		new_item.AlcoCodeId = alcoCodeIdent;
		MovList.insert(&new_item);
		item_idx = MovList.getCount();
	}
	return item_idx;
}

void PPViewAlcoDeclRu::ProcessStock(int startOrEnd, PPID divID, const ObjIdListFilt & rWhList, const PPIDArray & rGoodsList)
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
		rWhList.Get(gp.LocList);
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
					const int is_beer = PrcssrAlcReport::IsBeerCategoryCode(alc_goods_ext.CategoryCode);
					if((Filt.Flags & Filt.fOnlyBeer && is_beer) || (Filt.Flags & Filt.fOnlyNonBeerAlco && !is_beer)) {
						const long alco_code_ident = GetAlcoCodeIdent(alc_goods_ext.CategoryCode);
						if(alco_code_ident) {
							if(!alc_goods_ext.MnfOrImpPsnID) {
								PPID   manuf_id = 0;
								if(Arp.GetLotManufID(org_lot_id, &manuf_id, 0) > 0)
									alc_goods_ext.MnfOrImpPsnID = manuf_id;
							}
							uint   item_idx = GetMovListItemIdx(divID, alco_code_ident, alc_goods_ext.MnfOrImpPsnID);
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
		}
		PPWaitPercent(i+1, _gc, wait_msg_buf);
	}
}

void PPViewAlcoDeclRu::GetDivisionList(PPIDArray & rList) const
{
	rList.Z();
	{
		for(uint i = 0; i < RcptList.getCount(); i++) {
			rList.addnz(RcptList.at(i).DivID);
		}
	}
	{
		for(uint i = 0; i < MovList.getCount(); i++) {
			rList.addnz(MovList.at(i).DivID);
		}
	}
	rList.sortAndUndup();
}

void PPViewAlcoDeclRu::GetAlcoCodeList(PPID divID, PPIDArray & rList) const
{
	rList.Z();
	{
		for(uint i = 0; i < RcptList.getCount(); i++) {
			if(RcptList.at(i).DivID == divID)
				rList.addnz(RcptList.at(i).AlcoCodeId);
		}
	}
	{
		for(uint i = 0; i < MovList.getCount(); i++) {
			if(MovList.at(i).DivID == divID)
				rList.addnz(MovList.at(i).AlcoCodeId);
		}
	}
	rList.sortAndUndup();
}

void PPViewAlcoDeclRu::GetSupplList(PPID divID, long alcoCodeId, PPID manufID, PPIDArray & rList) const
{
	rList.Z();
	for(uint i = 0; i < RcptList.getCount(); i++) {
		const InnerRcptEntry & r_entry = RcptList.at(i);
		if((!alcoCodeId || (r_entry.AlcoCodeId == alcoCodeId && r_entry.DivID == divID)) && (!manufID || r_entry.ManufID == manufID)) {
			rList.addnz(r_entry.SupplID);
		}
	}
	rList.sortAndUndup();
}

void PPViewAlcoDeclRu::GetManufList(PPID divID, long alcoCodeId, PPIDArray & rList) const
{
	rList.Z();
	{
		for(uint i = 0; i < RcptList.getCount(); i++) {
			const InnerRcptEntry & r_entry = RcptList.at(i);
			if(!alcoCodeId || (r_entry.AlcoCodeId == alcoCodeId && r_entry.DivID == divID))
				rList.addnz(r_entry.ManufID);
		}
	}
	{
		for(uint i = 0; i < MovList.getCount(); i++) {
			const InnerMovEntry & r_entry = MovList.at(i);
			if(!alcoCodeId || (r_entry.AlcoCodeId == alcoCodeId && r_entry.DivID == divID))
				rList.addnz(r_entry.ManufID);
		}
	}
	rList.sortAndUndup();
}

/*virtual*/int PPViewAlcoDeclRu::Init_(const PPBaseFilt * pBaseFilt)
{
	int    ok = 1;
	SString wait_msg_buf;
	const AlcoDeclRuFilt prev_filt = Filt;
	THROW(Helper_InitBaseFilt(pBaseFilt));
	if(State & stOnceInited && Filt.IsEqualExcept(prev_filt, AlcoDeclRuFilt::eqxShowMode)) {
		;
	}
	else {
		RcptList.clear();
		RcptList_Detailed.clear();
		MovList.clear();
		MovList_Detailed.clear();
		BExtQuery::ZDelete(&P_IterQuery);
		Filt.Period.Actualize(ZERODATE);
		PPIDArray goods_list;
		PPIDArray div_list;
		if(Filt.DivList.GetCount()) {
			Filt.DivList.Get(div_list);
		}
		else
			div_list.add(0L);
		for(uint dividx = 0; dividx < div_list.getCount(); dividx++) {
			const PPID div_id = div_list.get(dividx);
			PPViewTrfrAnlz ta_view;
			TrfrAnlzFilt ta_filt;
			TrfrAnlzViewItem_AlcRep ta_item;
			PrcssrAlcReport::GoodsItem alc_goods_ext;
			PersonTbl::Rec manuf_psn_rec;
			ta_filt.Period = Filt.Period;
			bool do_skip_iteration = false;
			if(div_id == 0) {
				ta_filt.LocList.Z();
			}
			else {
				PPLocationPacket div_pack;
				if(Arp.PsnObj.LocObj.GetPacket(div_id, &div_pack) > 0 && div_pack.WarehouseList.GetCount()) {
					ta_filt.LocList = div_pack.WarehouseList;
				}
				else
					do_skip_iteration = true;
			}
			if(!do_skip_iteration) {
				THROW(ta_view.Init_(&ta_filt));
				goods_list.Z();
				PPWaitStart();
				for(ta_view.InitIteration(PPViewTrfrAnlz::OrdByDate); ta_view.NextIteration_AlcRep(&ta_item) > 0;) {
					const PPID goods_id = ta_item.Item.GoodsID;
					if(Arp.IsAlcGoods(goods_id) > 0 && Arp.PreprocessGoodsItem(goods_id, ta_item.OrgLotRec.ID, 0, 0, alc_goods_ext) > 0) {
						//const bool is_rcpt = (ta_item.OrgLotRec.ID == ta_item.Item.LotID && GetOpType(ta_item.Item.OpID, 0) == PPOPT_GOODSRECEIPT);
						if(!alc_goods_ext.MnfOrImpPsnID) {
							PPID   manuf_id = 0;
							if(Arp.GetLotManufID(ta_item.OrgLotRec.ID, &manuf_id, 0) > 0)
								alc_goods_ext.MnfOrImpPsnID = manuf_id;
						}
						const int is_beer = PrcssrAlcReport::IsBeerCategoryCode(alc_goods_ext.CategoryCode);
						if((Filt.Flags & Filt.fOnlyBeer && is_beer) || (Filt.Flags & Filt.fOnlyNonBeerAlco && !is_beer)) {
							const  long alco_code_ident = GetAlcoCodeIdent(alc_goods_ext.CategoryCode);
							if(alco_code_ident && alc_goods_ext.Volume > 0.0) {
								const double qtty_dal = fabs((ta_item.Item.Qtty * alc_goods_ext.Volume) / 10.0);
								const PPID op_id = ta_item.Item.OpID;
								bool  is_rcpt = false;
								bool  is_rcpt_ret = false;
								{
									uint   item_idx = GetMovListItemIdx(div_id, alco_code_ident, alc_goods_ext.MnfOrImpPsnID);
									assert(item_idx > 0 && item_idx <= MovList.getCount());
									InnerMovEntry & r_item = MovList.at(item_idx-1);
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
												if(op_rec.AccSheetID == GetSupplAccSheet()) {
													is_rcpt = true;
													r_item.RcptWhs += qtty_dal;
												}
												else if(op_rec.AccSheetID == GetSellAccSheet())
													r_item.SaleRet += qtty_dal;
												else 
													r_item.RcptEtc += qtty_dal;
											}
											else if(op_type_id == PPOPT_GOODSRETURN && op_rec.LinkOpID && op_rec.LinkOpID == CConfig.RetailOp) {
												r_item.SaleRet += qtty_dal;
											}
											else if(op_type_id == PPOPT_GOODSRETURN && GetOpType(op_rec.LinkOpID) == PPOPT_GOODSRECEIPT) {
												is_rcpt_ret = true;
												r_item.SupplRet += qtty_dal;
											}
											else if(op_type_id == PPOPT_GOODSRETURN && GetOpType(op_rec.LinkOpID) == PPOPT_GOODSEXPEND) {
												r_item.SaleRet += qtty_dal;
											}
											else if(op_type_id == PPOPT_GOODSEXPEND && op_rec.AccSheetID == GetSellAccSheet()) {
												r_item.ExpEtc += qtty_dal;
											}
											else if(ta_item.Item.Qtty < 0.0)
												r_item.ExpEtc += qtty_dal;
											else if(ta_item.Item.Qtty > 0.0)
												r_item.RcptEtc += qtty_dal;
										}
									}
								}
								if(is_rcpt || is_rcpt_ret) {
									uint   item_idx = 0;
									assert((is_rcpt && !is_rcpt_ret) || (!is_rcpt && is_rcpt_ret));
									PPID   suppl_id = ta_item.Item.PersonID;
									for(uint i = 0; !item_idx && i < RcptList.getCount(); i++) {
										const InnerRcptEntry & r_item = RcptList.at(i);
										if(r_item.DivID == div_id && r_item.ManufID == alc_goods_ext.MnfOrImpPsnID && r_item.BillID == ta_item.Item.BillID && alco_code_ident == r_item.AlcoCodeId) {
											if((is_rcpt_ret && r_item.ItemKind == 1) || (is_rcpt && r_item.ItemKind == 0))
												item_idx = i+1;
										}
									}
									if(!item_idx) {
										InnerRcptEntry new_item;
										new_item.DivID = div_id;
										new_item.ManufID = alc_goods_ext.MnfOrImpPsnID;
										new_item.BillID = ta_item.Item.BillID;
										new_item.AlcoCodeId = alco_code_ident;
										new_item.ItemKind = is_rcpt_ret ? 1 : 0;
										new_item.SupplID = suppl_id;
										RcptList.insert(&new_item);
										item_idx = RcptList.getCount();
									}
									assert(item_idx > 0 && item_idx <= RcptList.getCount());
									{
										InnerRcptEntry & r_item = RcptList.at(item_idx-1);
										r_item.BillDt = ta_item.Item.Dt;
										assert(r_item.SupplID == suppl_id);
										if(is_rcpt_ret)
											r_item.Qtty -= qtty_dal;
										else
											r_item.Qtty += qtty_dal;
									}
								}
							}
							goods_list.add(ta_item.Item.GoodsID);
						}
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
					ProcessStock(0, div_id, ta_filt.LocList, goods_list);
					ProcessStock(1, div_id, ta_filt.LocList, goods_list);
				}
			}
		}
		State |= stOnceInited;
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
		addGroup(ctlgroupLoc, new LocationCtrlGroup(CTLSEL_ALCODECL_LOC, 0, 0, cmLocList, 0, LocationCtrlGroup::fDivision, 0));
		SetupCalPeriod(CTLCAL_ALCODECL_PERIOD, CTL_ALCODECL_PERIOD);
	}
	DECL_DIALOG_SETDTS()
	{
		int    ok = 1;
		RVALUEPTR(Data, pData);
		//
		SetPeriodInput(this, CTL_ALCODECL_PERIOD, &Data.Period);
		SetupPersonCombo(this, CTLSEL_ALCODECL_MAINORG, Data.MainOrgID, 0, PPPRK_MAIN, 1);
		{
			PPID   main_org_id = Data.MainOrgID;
			if(!main_org_id)
				GetMainOrgID(&main_org_id);
			LocationCtrlGroup::Rec l_rec(&Data.DivList, 0, main_org_id);
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
		setCtrlData(CTL_ALCODECL_CORRNO, &Data.CorrectionNo);
		return ok;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		GetPeriodInput(this, CTL_ALCODECL_PERIOD, &Data.Period);
		getCtrlData(CTLSEL_ALCODECL_MAINORG, &Data.MainOrgID);
		{
			LocationCtrlGroup::Rec l_rec;
			getGroupData(ctlgroupLoc, &l_rec);
			Data.DivList = l_rec.LocList;
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
		getCtrlData(CTL_ALCODECL_CORRNO, &Data.CorrectionNo);
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
				case 7:
					if(p_item->DivID) {
						LocationTbl::Rec loc_rec;
						if(Arp.PsnObj.LocObj.Fetch(p_item->DivID, &loc_rec) > 0)
							temp_buf = loc_rec.Name;
						else
							ideqvalstr(p_item->DivID, temp_buf);
					}
					pBlk->Set(temp_buf);
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
				case 3: pBlk->Set(p_item->StockBeg); break;
				case 4: pBlk->Set(p_item->StockEnd); break;
				case 5: pBlk->Set(p_item->RcptManuf); break;
				case 6: pBlk->Set(p_item->RcptWhs); break;
				case 7: pBlk->Set(p_item->RcptImp); break;
				case 8: pBlk->Set(p_item->RcptIntr); break;
				case 9: pBlk->Set(p_item->RcptEtc); break;
				case 10: pBlk->Set(p_item->ExpRetail); break;
				case 11: pBlk->Set(p_item->ExpEtc); break;
				case 12: pBlk->Set(p_item->SupplRet); break;
				case 13: pBlk->Set(p_item->SaleRet); break;
				case 14: pBlk->Set(p_item->ExpIntr); break;
				case 15:
					{
						double balance = p_item->StockBeg - p_item->StockEnd + p_item->RcptManuf + p_item->RcptWhs + p_item->RcptImp +
							p_item->RcptIntr + p_item->RcptEtc + p_item->SaleRet - p_item->ExpRetail - p_item->ExpIntr -
							p_item->ExpEtc - p_item->SupplRet;
						pBlk->Set(balance);
					}
					break;
				case 16:
					if(p_item->DivID) {
						LocationTbl::Rec loc_rec;
						if(Arp.PsnObj.LocObj.Fetch(p_item->DivID, &loc_rec) > 0)
							temp_buf = loc_rec.Name;
						else
							ideqvalstr(p_item->DivID, temp_buf);
					}
					pBlk->Set(temp_buf);
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

/*virtual*/int PPViewAlcoDeclRu::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	if(ok == -2) {
		switch(ppvCmd) {
			case PPVCMD_EXPORT:
				ok = -1;
				Export();
				break;
			case PPVCMD_TOGGLE:
				ok = -1;
				INVERSEFLAG(Filt.Flags, Filt.fShowAsRcpt);
				ChangeFilt(1, pBrw);
				break;
		}
	}
	return ok;
}

int PPViewAlcoDeclRu::Export()
{
	int    ok = 1;
	const  LDATETIME _now = getcurdatetime_();
	SString temp_buf;
	SString file_name;
	const char * p_form_no = 0;
	PersonTbl::Rec psn_rec;
	SString period_tag;
	S_GUID rep_uuid;
	PPIDArray manuf_list;
	PPIDArray suppl_list;
	LongArray alco_code_id_list;
	DocNalogRu_Generator g;
	RegisterTbl::Rec reg_rec;
	PPID   main_org_id = Filt.MainOrgID;
	int    main_org_region_code = 0;
	SString main_org_name;
	SString main_org_inn;
	SString main_org_kpp;
	if(!main_org_id)
		GetMainOrgID(&main_org_id);
	THROW_PP(Arp.PsnObj.Search(main_org_id, &psn_rec) > 0, PPERR_UNDEFMAINORG);
	main_org_name = psn_rec.Name;
	if(Arp.PsnObj.GetRegNumber(main_org_id, PPREGT_TPID, Filt.Period.low, temp_buf) > 0) {
		main_org_inn = temp_buf;
	}
	if(Arp.GetWkrRegister(Arp.wkrKPP, main_org_id, 0, Filt.Period.low, &reg_rec) > 0) {
		main_org_kpp = reg_rec.Num;
	}
	if(main_org_kpp.NotEmpty()) {
		main_org_kpp.Sub(0, 2, temp_buf.Z());
		main_org_region_code = temp_buf.ToLong();
	}
	else if(main_org_inn.NotEmpty()) {
		main_org_inn.Sub(0, 2, temp_buf.Z());
		main_org_region_code = temp_buf.ToLong();
	}
	// R_INN_Z_ddmmyyyy_N.xml
	// Z - номер квартала по номеру последнего месяца и последняя цифра года. Например: 031 - 1-й кв 2021; 060 - 2-й кв 2020; 091 - 3-й кв 2021; 001 - 4-й кв 2021
	// dd - день выгрузки
	// mm - месяц выгрузки
	// yyyy - год выгрузки
	// N - GUID
	rep_uuid.Generate();
	temp_buf.Z();
	if(Filt.Flags & Filt.fOnlyBeer) {
		temp_buf.Cat("R08");
		p_form_no = "38";
	}
	else if(Filt.Flags & Filt.fOnlyNonBeerAlco) {
		temp_buf.Cat("R07");
		p_form_no = "37";
	}
	else {
		temp_buf.Cat("RX");
		p_form_no = "00";
	}
	temp_buf.CatChar('_').Cat(main_org_inn).CatChar('_');
	{
		int    q = 0;
		int    y = 0;
		if(Filt.Period.GetQuart(&q, &y)) {
			temp_buf.CatLongZ((q * 3) % 12, 2).Cat(y % 10);
			period_tag.Z().Cat((q * 3) % 12);
		}
		else {
			temp_buf.Cat("000");
			period_tag = "3";
		}
	}
	temp_buf.CatChar('_').Cat(_now.d, DATF_DMY|DATF_CENTURY|DATF_NODIV);
	temp_buf.CatChar('_').Cat(rep_uuid, S_GUID::fmtIDL);
	PPGetPath(PPPATH_OUT, file_name);
	file_name.SetLastSlash().Cat(temp_buf).Dot().Cat("xml");
	{
		THROW(g.StartDocument(file_name));
		//
		{
			SXml::WNode nf(g.P_X, g.GetToken_Ansi(PPHSC_RU_FILE));
			nf.PutAttrib(g.GetToken_Ansi(PPHSC_RU_DOCDATE), temp_buf.Z().Cat(_now.d, DATF_GERMAN|DATF_CENTURY));
			nf.PutAttrib(g.GetToken_Ansi(PPHSC_RU_VERFORM), "4.4");
			{
				PPVersionInfo vi = DS.GetVersionInfo();
				vi.GetTextAttrib(PPVersionInfo::taiProductName, temp_buf);
				nf.PutAttrib(g.GetToken_Ansi(PPHSC_RU_PROGNAME), temp_buf);
			}
			{
				SXml::WNode n_(g.P_X, g.GetToken_Ansi(PPHSC_RU_REPFORM));
				n_.PutAttrib(g.GetToken_Ansi(PPHSC_RU_FORMNO), p_form_no);
				n_.PutAttrib(g.GetToken_Ansi(PPHSC_RU_REPPERIODTAG), period_tag);
				n_.PutAttrib(g.GetToken_Ansi(PPHSC_RU_REPPERIODYEAR), temp_buf.Z().Cat(Filt.Period.low.year()));
				if(Filt.CorrectionNo == 0) {
					SXml::WNode n2(g.P_X, g.GetToken_Ansi(PPHSC_RU_PRIMFORM));
				}
				else {
					SXml::WNode n2(g.P_X, g.GetToken_Ansi(PPHSC_RU_CORRFORM));
					n2.PutAttrib(g.GetToken_Ansi(PPHSC_RU_CORRECTIONNO), temp_buf.Z().Cat(Filt.CorrectionNo));
				}
			}
			{
				SString ca_inn;
				SString ca_kpp;
				GetManufList(0, 0, manuf_list);
				GetSupplList(0, 0, 0, suppl_list);
				SXml::WNode n_(g.P_X, g.GetToken_Ansi(PPHSC_RU_REFERENCES));
				for(uint midx = 0; midx < manuf_list.getCount(); midx++) {
					const PPID manuf_id = manuf_list.get(midx);
					ca_inn.Z();
					ca_kpp.Z();
					if(Arp.PsnObj.Search(manuf_id, &psn_rec) > 0) {
						SXml::WNode n2(g.P_X, g.GetToken_Ansi(PPHSC_RU_MANUFANDIMPORTERS));
						n2.PutAttrib(g.GetToken_Ansi(PPHSC_RU_MANUFORIMPORTERID), temp_buf.Z().Cat(manuf_id));
						(temp_buf = psn_rec.Name).Transf(CTRANSF_INNER_TO_OUTER);
						n2.PutAttrib(g.GetToken_Ansi_Pe0(4), temp_buf);
						Arp.PsnObj.GetRegNumber(manuf_id, PPREGT_TPID, Filt.Period.low, ca_inn);
						if(Arp.GetWkrRegister(Arp.wkrKPP, manuf_id, 0, Filt.Period.low, &reg_rec) > 0)
							ca_kpp = reg_rec.Num;
						if(Filt.Flags & Filt.fOnlyBeer) {
							if(ca_inn.Len() == 12) {
								SXml::WNode n3(g.P_X, g.GetToken_Ansi(PPHSC_RU_IND_S));
								n3.PutAttrib(g.GetToken_Ansi_Pe0(5), ca_inn);
							}
							else {
								SXml::WNode n3(g.P_X, g.GetToken_Ansi(PPHSC_RU_JUR_S));
								n3.PutAttrib(g.GetToken_Ansi_Pe0(5), ca_inn);
								if(ca_kpp.NotEmpty())
									n3.PutAttrib(g.GetToken_Ansi_Pe0(6), ca_kpp);
							}
						}
						else {
							if(ca_inn.NotEmpty())
								n2.PutAttrib(g.GetToken_Ansi_Pe0(5), ca_inn); // ИНН
							if(ca_kpp.NotEmpty())
								n2.PutAttrib(g.GetToken_Ansi_Pe0(6), ca_kpp); // КПП
						}
					}
				}
				for(uint sidx = 0; sidx < suppl_list.getCount(); sidx++) {
					const PPID suppl_id = suppl_list.get(sidx);
					ca_inn.Z();
					ca_kpp.Z();
					if(Arp.PsnObj.Search(suppl_id, &psn_rec) > 0) {
						SXml::WNode n2(g.P_X, g.GetToken_Ansi(PPHSC_RU_SUPPLIERS));
						n2.PutAttrib(g.GetToken_Ansi(PPHSC_RU_SUPPLIERID), temp_buf.Z().Cat(suppl_id));
						(temp_buf = psn_rec.Name).Transf(CTRANSF_INNER_TO_OUTER);
						n2.PutAttrib(g.GetToken_Ansi_Pe0(7), temp_buf);
						Arp.PsnObj.GetRegNumber(suppl_id, PPREGT_TPID, Filt.Period.low, ca_inn);
						if(Arp.GetWkrRegister(Arp.wkrKPP, suppl_id, 0, Filt.Period.low, &reg_rec) > 0)
							ca_kpp.Cat(reg_rec.Num);
						if(ca_inn.NotEmpty()) {
							if(ca_inn.Len() == 12) {
								SXml::WNode n3(g.P_X, g.GetToken_Ansi(PPHSC_RU_IND_S));
								n3.PutAttrib(g.GetToken_Ansi_Pe0(9), ca_inn);
							}
							else {
								SXml::WNode n3(g.P_X, g.GetToken_Ansi(PPHSC_RU_JUR_S));
								n3.PutAttrib(g.GetToken_Ansi_Pe0(9), ca_inn);
								if(ca_kpp.NotEmpty())
									n3.PutAttrib(g.GetToken_Ansi_Pe0(10), ca_kpp);
							}
						}
					}
				}
			}
			{
				PPID   main_org_loc_id = 0;
				PPLocationPacket main_org_addr_loc_pack;
				PPLocationPacket addr_loc_pack_;
				SXml::WNode n_(g.P_X, g.GetToken_Ansi(PPHSC_RU_DOCUMENT));
				{
					SXml::WNode n2(g.P_X, g.GetToken_Ansi(PPHSC_RU_ORG));
					{
						SXml::WNode n3(g.P_X, g.GetToken_Ansi(PPHSC_RU_REQUISITES));
						(temp_buf = main_org_name).Transf(CTRANSF_INNER_TO_OUTER);
						n3.PutAttrib(g.GetToken_Ansi(PPHSC_RU_APPEL), temp_buf);
						{
							if(psn_rec.RLoc && Arp.PsnObj.LocObj.GetPacket(psn_rec.RLoc, &main_org_addr_loc_pack) > 0)
								main_org_loc_id = psn_rec.RLoc;
							else if(psn_rec.MainLoc && Arp.PsnObj.LocObj.GetPacket(psn_rec.MainLoc, &main_org_addr_loc_pack) > 0)
								main_org_loc_id = psn_rec.MainLoc;
							if(main_org_loc_id) {
								g.WriteAddress(main_org_addr_loc_pack, main_org_region_code, PPHSC_RU_ORGADDR);
							}
							if(main_org_inn.Len() == 12) {
								SXml::WNode n4(g.P_X, g.GetToken_Ansi(PPHSC_RU_IND_S));
								n4.PutAttrib(g.GetToken_Ansi(PPHSC_RU_INNPHS), main_org_inn);
							}
							else {
								SXml::WNode n4(g.P_X, g.GetToken_Ansi(PPHSC_RU_JUR_S));
								n4.PutAttrib(g.GetToken_Ansi(PPHSC_RU_INNJUR), main_org_inn);
								if(main_org_kpp.NotEmpty())
									n4.PutAttrib(g.GetToken_Ansi(PPHSC_RU_KPPJUR), main_org_kpp);
							}
						}
					}
					{
						PPObjStaffList stlobj;
						PersonPostTbl::Rec post_rec;
						PPID   chief_id = 0;
						PPID   chief_accountant_id = 0;
						stlobj.GetFixedPostOnDate(main_org_id, PPFIXSTF_DIRECTOR, ZERODATE, &post_rec);
						chief_id = post_rec.PersonID;
						stlobj.GetFixedPostOnDate(main_org_id, PPFIXSTF_ACCOUNTANT, ZERODATE, &post_rec);
						chief_accountant_id = post_rec.PersonID;
						if(!chief_id || !chief_accountant_id) {
							SETIFZ(chief_id, DS.CCfg().MainOrgDirector_);
							SETIFZ(chief_accountant_id, DS.CCfg().MainOrgAccountant_);
						}
						SXml::WNode n3(g.P_X, g.GetToken_Ansi(PPHSC_RU_RESPONSIBLEPERSON));
						if(Arp.PsnObj.Search(chief_id, &psn_rec) > 0) {
							SXml::WNode n4(g.P_X, g.GetToken_Ansi(PPHSC_RU_CHIEF));
							g.WriteFIO(psn_rec.Name);
						}
						if(Arp.PsnObj.Search(chief_accountant_id, &psn_rec) > 0) {
							SXml::WNode n4(g.P_X, g.GetToken_Ansi(PPHSC_RU_CHIEFACCOUNTANT));
							g.WriteFIO(psn_rec.Name);
						}
					}
					if(Filt.Flags & Filt.fOnlyNonBeerAlco) {
						const PrcssrAlcReport::Config & r_cfg = Arp.GetConfig();
						SXml::WNode n3(g.P_X, g.GetToken_Ansi(PPHSC_RU_ORGACTIVITY));
						if(r_cfg.AlcLicRegTypeID && Arp.PsnObj.GetRegister(main_org_id, r_cfg.AlcLicRegTypeID, Filt.Period.low, &reg_rec) > 0) {
							SXml::WNode n4(g.P_X, g.GetToken_Ansi(PPHSC_RU_ORGACTIVITYLICD));
							n4.PutInner(g.GetToken_Ansi(PPHSC_RU_ORGACTIVITYKIND), "06");
							temp_buf.Z().Cat(reg_rec.Serial).Space().Cat(reg_rec.Num).Strip().Transf(CTRANSF_INNER_TO_OUTER);
							n4.PutInner(g.GetToken_Ansi(PPHSC_RU_LICSERIAL), temp_buf);
							n4.PutInner(g.GetToken_Ansi(PPHSC_RU_LICINITDATE), temp_buf.Z().Cat(reg_rec.Dt, DATF_GERMAN|DATF_CENTURY));
							n4.PutInner(g.GetToken_Ansi(PPHSC_RU_LICEXPIRY), temp_buf.Z().Cat(reg_rec.Expiry, DATF_GERMAN|DATF_CENTURY));
						}
						else {
							SXml::WNode n4(g.P_X, g.GetToken_Ansi(PPHSC_RU_ORGACTIVITYNONLICD));
							n4.PutInner(g.GetToken_Ansi(PPHSC_RU_ORGACTIVITYKIND), "12");
						}
					}					
				}
				{
					PPIDArray div_list;
					GetDivisionList(div_list);
					for(uint dividx = 0; dividx < div_list.getCount(); dividx++) {
						const  PPID div_id = div_list.get(dividx);
						SXml::WNode n2(g.P_X, g.GetToken_Ansi(PPHSC_RU_TURNOVERVOLUME));
						{
							SString loc_kpp;
							uint   loc_region_code = 0;
							if(div_id && Arp.PsnObj.LocObj.GetPacket(div_id, &addr_loc_pack_) > 0) {
								if(Arp.GetWkrRegister(Arp.wkrKPP, main_org_id, div_id, Filt.Period.low, &reg_rec) > 0) {
									loc_kpp = reg_rec.Num;
								}
								if(loc_kpp.NotEmpty()) {
									loc_kpp.Sub(0, 2, temp_buf.Z());
									loc_region_code = temp_buf.ToLong();
								}
								n2.PutAttrib(g.GetToken_Ansi(PPHSC_RU_APPEL), (temp_buf = main_org_name).Transf(CTRANSF_INNER_TO_OUTER));
								n2.PutAttrib(g.GetToken_Ansi(PPHSC_RU_KPPJUR), (temp_buf = loc_kpp));
								g.WriteAddress(addr_loc_pack_, loc_region_code, PPHSC_RU_ORGADDR);
							}
							else {
								if(Arp.GetWkrRegister(Arp.wkrKPP, main_org_id, 0, Filt.Period.low, &reg_rec) > 0) {
									loc_kpp = reg_rec.Num;
								}
								if(loc_kpp.NotEmpty()) {
									loc_kpp.Sub(0, 2, temp_buf.Z());
									loc_region_code = temp_buf.ToLong();
								}
								n2.PutAttrib(g.GetToken_Ansi(PPHSC_RU_APPEL), (temp_buf = main_org_name).Transf(CTRANSF_INNER_TO_OUTER));
								n2.PutAttrib(g.GetToken_Ansi(PPHSC_RU_KPPJUR), (temp_buf = loc_kpp));
								if(main_org_loc_id) {
									g.WriteAddress(main_org_addr_loc_pack, loc_region_code, PPHSC_RU_ORGADDR);
								}								
							}
						}
						GetAlcoCodeList(div_id, alco_code_id_list);
						for(uint acidx = 0; acidx < alco_code_id_list.getCount(); acidx++) {
							const long alco_code_id = alco_code_id_list.get(acidx);
							GetManufList(div_id, alco_code_id, manuf_list);
							SXml::WNode n3(g.P_X, g.GetToken_Ansi(PPHSC_RU_TURNOVER));
							n3.PutAttrib(g.GetToken_Ansi(PPHSC_RU_SEQUENCE), temp_buf.Z().Cat(acidx+1));
							GetAlcoCode(alco_code_id, temp_buf);
							n3.PutAttrib(g.GetToken_Ansi_Pe0(3), temp_buf);
							for(uint manufidx = 0; manufidx < manuf_list.getCount(); manufidx++) {
								const PPID manuf_id = manuf_list.get(manufidx);
								SXml::WNode n4(g.P_X, g.GetToken_Ansi(PPHSC_RU_MANUFORIMPORTERINFO));
								n4.PutAttrib(g.GetToken_Ansi(PPHSC_RU_SEQUENCE), temp_buf.Z().Cat(manufidx+1));
								n4.PutAttrib(g.GetToken_Ansi(PPHSC_RU_MANUFORIMPORTERID_), temp_buf.Z().Cat(manuf_id)); // ИдПроизвИмп (не PPHSC_RU_MANUFORIMPORTERID "ИДПроизвИмп")
								{
									GetSupplList(div_id, alco_code_id, manuf_id, suppl_list);
									for(uint supplidx = 0; supplidx < suppl_list.getCount(); supplidx++) {
										const PPID suppl_id = suppl_list.get(supplidx);
										SXml::WNode n5(g.P_X, g.GetToken_Ansi(PPHSC_RU_SUPPLIER));
										n5.PutAttrib(g.GetToken_Ansi(PPHSC_RU_SEQUENCE), temp_buf.Z().Cat(supplidx+1));
										n4.PutAttrib(g.GetToken_Ansi(PPHSC_RU_SUPPLIERID_), temp_buf.Z().Cat(suppl_id));
										uint   seq = 0;
										for(uint ridx = 0; ridx < RcptList.getCount(); ridx++) {
											const InnerRcptEntry & r_entry = RcptList.at(ridx);
											if(r_entry.AlcoCodeId == alco_code_id && r_entry.ManufID == manuf_id && r_entry.SupplID == suppl_id) {
												BillTbl::Rec suppl_bill_rec;
												if(P_BObj->Search(r_entry.BillID, &suppl_bill_rec) > 0) {
													assert(suppl_bill_rec.Dt == r_entry.BillDt);
													if(r_entry.ItemKind == 0) {
														SXml::WNode n6(g.P_X, g.GetToken_Ansi(PPHSC_RU_SUPPLY));
														n6.PutAttrib(g.GetToken_Ansi_Pe0(13), temp_buf.Z().Cat(suppl_bill_rec.Dt, DATF_GERMAN|DATF_CENTURY));
														BillCore::GetCode(temp_buf = suppl_bill_rec.Code);
														n6.PutAttrib(g.GetToken_Ansi_Pe0(14), temp_buf.Transf(CTRANSF_INNER_TO_OUTER));
														n6.PutAttrib(g.GetToken_Ansi_Pe0(15), "");
														n6.PutAttrib(g.GetToken_Ansi_Pe0(16), temp_buf.Z().Cat(fabs(r_entry.Qtty), MKSFMTD(0, 5, 0)));
													}
													else if(r_entry.ItemKind == 1) {
														SXml::WNode n6(g.P_X, g.GetToken_Ansi(PPHSC_RU_RETURN));
														n6.PutAttrib(g.GetToken_Ansi_Pe0(13), temp_buf.Z().Cat(suppl_bill_rec.Dt, DATF_GERMAN|DATF_CENTURY));
														BillCore::GetCode(temp_buf = suppl_bill_rec.Code);
														n6.PutAttrib(g.GetToken_Ansi_Pe0(14), temp_buf.Transf(CTRANSF_INNER_TO_OUTER));
														n6.PutAttrib(g.GetToken_Ansi_Pe0(15), "");
														n6.PutAttrib(g.GetToken_Ansi_Pe0(16), temp_buf.Z().Cat(fabs(r_entry.Qtty), MKSFMTD(0, 5, 0)));
													}
												}
											}
										}
									}
								}
								{
									uint   seq = 0;
									for(uint midx = 0; midx < MovList.getCount(); midx++) {
										const InnerMovEntry & r_entry = MovList.at(midx);
										if(r_entry.AlcoCodeId == alco_code_id && r_entry.ManufID == manuf_id) {
											seq++;
											double sub_total = 0.0;
											SXml::WNode n5(g.P_X, g.GetToken_Ansi(PPHSC_RU_MOVING));
											// Первые 4 атрибута общие и для пива и для алкашки
											n5.PutAttrib(g.GetToken_Ansi(PPHSC_RU_SEQUENCE), temp_buf.Z().Cat(seq));
											n5.PutAttrib(g.GetToken_Ansi_Pe1(6), temp_buf.Z().Cat(r_entry.StockBeg, MKSFMTD(0, 5, 0)));
											n5.PutAttrib(g.GetToken_Ansi_Pe1(7), temp_buf.Z().Cat(r_entry.RcptManuf, MKSFMTD(0, 5, 0)));
											sub_total += r_entry.RcptManuf;
											n5.PutAttrib(g.GetToken_Ansi_Pe1(8), temp_buf.Z().Cat(r_entry.RcptWhs, MKSFMTD(0, 5, 0)));
											sub_total += r_entry.RcptWhs;
											if(Filt.Flags & Filt.fOnlyBeer) {
												/* 08
													# ПN Номер по порядку
													# П100000000006 Остаток на начало отчетного периода (дал)
													# П100000000007 Поступление - закупки от организаций- производителей (дал)
													# П100000000008 Поступление - закупки от организаций оптовой торговли (дал)

													П100000000009 Поступление - закупки по импорту (дал)
													П100000000010 Поступление - закупки итого (дал)
													П100000000011 Поступление - возврат от покупателей (дал)
													П100000000012 Прочие поступления (дал)
													П100000000013 Поступление - перемещение внутри одной организации (дал)
													П100000000014 Поступление - всего (дал)
													П100000000015 Расход - объем розничной продажи (дал)
													П100000000016 Прочий расход (дал)
													П100000000017 Возврат поставщику (дал)
													П100000000018 Расход - перемещение внутри одной организации (дал)
													П100000000019 Расход всего (дал)
													П100000000020 Остаток на конец отчетного периода (дал)
												*/
												n5.PutAttrib(g.GetToken_Ansi_Pe1(9), temp_buf.Z().Cat(r_entry.RcptImp, MKSFMTD(0, 5, 0)));
												sub_total += r_entry.RcptImp;
												n5.PutAttrib(g.GetToken_Ansi_Pe1(10), temp_buf.Z().Cat(sub_total, MKSFMTD(0, 5, 0)));
												n5.PutAttrib(g.GetToken_Ansi_Pe1(11), temp_buf.Z().Cat(r_entry.SaleRet, MKSFMTD(0, 5, 0)));
												sub_total += r_entry.SaleRet;
												n5.PutAttrib(g.GetToken_Ansi_Pe1(12), temp_buf.Z().Cat(r_entry.RcptEtc, MKSFMTD(0, 5, 0)));
												sub_total += r_entry.RcptEtc;
												n5.PutAttrib(g.GetToken_Ansi_Pe1(13), temp_buf.Z().Cat(r_entry.RcptIntr, MKSFMTD(0, 5, 0)));
												sub_total += r_entry.RcptIntr;
												n5.PutAttrib(g.GetToken_Ansi_Pe1(14), temp_buf.Z().Cat(sub_total, MKSFMTD(0, 5, 0)));
												sub_total = 0.0;
												n5.PutAttrib(g.GetToken_Ansi_Pe1(15), temp_buf.Z().Cat(r_entry.ExpRetail, MKSFMTD(0, 5, 0)));
												sub_total += r_entry.ExpRetail;
												n5.PutAttrib(g.GetToken_Ansi_Pe1(16), temp_buf.Z().Cat(r_entry.ExpEtc, MKSFMTD(0, 5, 0)));
												sub_total += r_entry.ExpEtc;
												n5.PutAttrib(g.GetToken_Ansi_Pe1(17), temp_buf.Z().Cat(r_entry.SupplRet, MKSFMTD(0, 5, 0)));
												sub_total += r_entry.SupplRet;
												n5.PutAttrib(g.GetToken_Ansi_Pe1(18), temp_buf.Z().Cat(r_entry.ExpIntr, MKSFMTD(0, 5, 0)));
												sub_total += r_entry.ExpIntr;
												n5.PutAttrib(g.GetToken_Ansi_Pe1(19), temp_buf.Z().Cat(sub_total, MKSFMTD(0, 5, 0)));
												sub_total = 0.0;
												n5.PutAttrib(g.GetToken_Ansi_Pe1(20), temp_buf.Z().Cat(r_entry.StockEnd, MKSFMTD(0, 5, 0)));
											}
											else if(Filt.Flags & Filt.fOnlyNonBeerAlco) {
												/* 07
													# ПN Номер по порядку
													# П100000000006 Остаток на начало отчетного периода (дал)
													# П100000000007 Поступление (закупки) от организаций- производителей (дал)
													# П100000000008 Поступление (закупки) от организаций оптовой торговли (дал)

													П100000000009 Поступление (закупки) итого (дал)
													П100000000010 Поступление (возврат от покупателей) (дал)
													П100000000011 Прочие поступления (дал)
													П100000000012 Поступление (перемещение внутри одной организации) (дал)
													П100000000013 Поступление всего (дал)
													П100000000014 Расход (объем розничной продажи) (дал)
													П100000000015 Прочий расход (дал)
													П100000000016 Возврат поставщику (дал)
													П100000000017 Расход (перемещение внутри одной организации) (дал)
													П100000000018 Расход всего (дал)
													П100000000019 Остаток продукции на конец отчетного периода - всего (дал)
													П100000000020 В том числе остаток продукции, маркированной федеральными специальными и (или) акцизными марками, требования к которым утрачивают силу (дал)
												*/
												n5.PutAttrib(g.GetToken_Ansi_Pe1(9), temp_buf.Z().Cat(sub_total, MKSFMTD(0, 5, 0)));
												n5.PutAttrib(g.GetToken_Ansi_Pe1(10), temp_buf.Z().Cat(r_entry.SaleRet, MKSFMTD(0, 5, 0)));
												sub_total += r_entry.SaleRet;
												n5.PutAttrib(g.GetToken_Ansi_Pe1(11), temp_buf.Z().Cat(r_entry.RcptEtc, MKSFMTD(0, 5, 0)));
												sub_total += r_entry.RcptEtc;
												n5.PutAttrib(g.GetToken_Ansi_Pe1(12), temp_buf.Z().Cat(r_entry.RcptIntr, MKSFMTD(0, 5, 0)));
												sub_total += r_entry.RcptIntr;
												n5.PutAttrib(g.GetToken_Ansi_Pe1(13), temp_buf.Z().Cat(sub_total, MKSFMTD(0, 5, 0)));
												sub_total = 0.0;
												n5.PutAttrib(g.GetToken_Ansi_Pe1(14), temp_buf.Z().Cat(r_entry.ExpRetail, MKSFMTD(0, 5, 0)));
												sub_total += r_entry.ExpRetail;
												n5.PutAttrib(g.GetToken_Ansi_Pe1(15), temp_buf.Z().Cat(r_entry.ExpEtc, MKSFMTD(0, 5, 0)));
												sub_total += r_entry.ExpEtc;
												n5.PutAttrib(g.GetToken_Ansi_Pe1(16), temp_buf.Z().Cat(r_entry.SupplRet, MKSFMTD(0, 5, 0)));
												sub_total += r_entry.SupplRet;
												n5.PutAttrib(g.GetToken_Ansi_Pe1(17), temp_buf.Z().Cat(r_entry.ExpIntr, MKSFMTD(0, 5, 0)));
												sub_total += r_entry.ExpIntr;
												n5.PutAttrib(g.GetToken_Ansi_Pe1(18), temp_buf.Z().Cat(sub_total, MKSFMTD(0, 5, 0)));
												sub_total = 0.0;
												n5.PutAttrib(g.GetToken_Ansi_Pe1(19), temp_buf.Z().Cat(r_entry.StockEnd, MKSFMTD(0, 5, 0)));
												n5.PutAttrib(g.GetToken_Ansi_Pe1(20), temp_buf.Z().Cat(0.0, MKSFMTD(0, 5, 0))); // В том числе остаток продукции, маркированной федеральными специальными и (или) акцизными марками, требования к которым утрачивают силу (дал)
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
		g.EndDocument();
	}
	CATCHZOK
	return ok;
}
