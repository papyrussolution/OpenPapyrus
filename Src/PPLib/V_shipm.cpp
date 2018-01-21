// V_SHIPM.CPP
// Copyright (c) A.Sobolev 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2009, 2010, 2012, 2013, 2015, 2016, 2017, 2018
// @codepage windows-1251
// Анализ отгрузки товаров
//
#include <pp.h>
#pragma hdrstop

IMPLEMENT_PPFILT_FACTORY(ShipmAnalyze); SLAPI ShipmAnalyzeFilt::ShipmAnalyzeFilt() : PPBaseFilt(PPFILT_SHIPMANALYZE, 0, 1)
{
	SetFlatChunk(offsetof(ShipmAnalyzeFilt, ReserveStart),
		offsetof(ShipmAnalyzeFilt, ReserveEnd) - offsetof(ShipmAnalyzeFilt, ReserveStart));
	Init(1, 0);
}

// virtual
int SLAPI ShipmAnalyzeFilt::Init(int fullyDestroy, long extraData)
{
	int   ok = PPBaseFilt::Init(1, 0);
	Period.SetDate(LConfig.OperDate);
	return ok;
}

int SLAPI ShipmAnalyzeFilt::TranslateToBillFilt(BillFilt * pBillFilt)
{
	if(pBillFilt) {
		pBillFilt->Period = Period;
		pBillFilt->LocList.Add(LocID);
		pBillFilt->OpID   = OpID;
		pBillFilt->AccSheetID = AccSheetID;
		pBillFilt->ObjectID = ObjectID;
		SETFLAG(pBillFilt->Flags, BillFilt::fDebtOnly,  Flags & ShipmAnalyzeFilt::fDebtOnly);
		SETFLAG(pBillFilt->Flags, BillFilt::fLabelOnly, Flags & ShipmAnalyzeFilt::fLabelOnly);
		return 1;
	}
	return -1;
}
//
//
// virtual
PPBaseFilt * SLAPI PPViewShipmAnalyze::CreateFilt(void * extraPtr) const
{
	ShipmAnalyzeFilt * p_filt = 0;
	if(PPView::CreateFiltInstance(PPFILT_SHIPMANALYZE, (PPBaseFilt**)&p_filt))
		p_filt->Init(1, 0);
	else
		PPErrCode = PPERR_BASEFILTUNSUPPORTED;
	return (PPBaseFilt*)p_filt;
}

SLAPI PPViewShipmAnalyze::PPViewShipmAnalyze() : PPView(0, &Filt, PPVIEW_SHIPMANALYZE), BObj(BillObj), Tbl(0)
{
}

SLAPI PPViewShipmAnalyze::~PPViewShipmAnalyze()
{
	delete Tbl;
	DBRemoveTempFiles();
}

int SLAPI PPViewShipmAnalyze::EditBaseFilt(PPBaseFilt * pFilt)
{
	class ShipmAnalyzeFiltDialog : public WLDialog {
	public:
		ShipmAnalyzeFiltDialog() : WLDialog(DLG_SHANLZFLT, CTL_SHANLZFLT_LABEL)
		{
			SetupCalPeriod(CTLCAL_SHANLZFLT_PERIOD, CTL_SHANLZFLT_PERIOD);
		}
		int    setDTS(const ShipmAnalyzeFilt * pFilt)
		{
			PPIDArray types;
			PPOprKind opk;
			Data = *pFilt;
			const PPConfig & r_cfg = LConfig;
			SETIFZ(Data.Period.low, r_cfg.InitDate);
			SETIFZ(Data.Period.upp, r_cfg.OperDate);
			SetPeriodInput(this, CTL_SHANLZFLT_PERIOD, &Data.Period);
			types.addzlist(PPOPT_GOODSORDER, PPOPT_GOODSRECEIPT, PPOPT_GOODSEXPEND, PPOPT_GOODSACK, 0);
			SetupOprKindCombo(this, CTLSEL_SHANLZFLT_OPRKIND, Data.OpID, 0, &types, 0);
			GetOpData(Data.OpID, &opk);
			setupAccSheet(Data.OpID ? opk.AccSheetID : Data.AccSheetID);
			SetupPPObjCombo(this, CTLSEL_SHANLZFLT_LOC, PPOBJ_LOCATION, Data.LocID, 0, 0);
			AddClusterAssoc(CTL_SHANLZFLT_FLAGS, 0, ShipmAnalyzeFilt::fDiffByBill);
			AddClusterAssoc(CTL_SHANLZFLT_FLAGS, 1, ShipmAnalyzeFilt::fDebtOnly);
			SetClusterData(CTL_SHANLZFLT_FLAGS, Data.Flags);
			setWL(BIN(Data.Flags & ShipmAnalyzeFilt::fLabelOnly));
			return 1;
		}
		int    getDTS(ShipmAnalyzeFilt * pFilt)
		{
			int    ok = 1;
			if(!GetPeriodInput(this, CTL_SHANLZFLT_PERIOD, &Data.Period) || !AdjustPeriodToRights(Data.Period, 1))
				ok = PPErrorByDialog(this, CTL_SHANLZFLT_PERIOD);
			else {
				getCtrlData(CTLSEL_SHANLZFLT_OPRKIND, &Data.OpID);
				if(!Data.OpID)
					ok = PPErrorByDialog(this, CTL_SHANLZFLT_OPRKIND, PPERR_OPRKINDNEEDED);
				else {
					if(Data.OpID || Data.AccSheetID)
						getCtrlData(CTLSEL_SHANLZFLT_OBJECT, &Data.ObjectID);
					else
						Data.ObjectID = 0;
					getCtrlData(CTLSEL_SHANLZFLT_LOC, &Data.LocID);
					GetClusterData(CTL_SHANLZFLT_FLAGS, &Data.Flags);
					SETFLAG(Data.Flags, ShipmAnalyzeFilt::fLabelOnly, getWL());
					*pFilt = Data;
				}
			}
			return ok;
		}
	private:
		DECL_HANDLE_EVENT
		{
			WLDialog::handleEvent(event);
			if(event.isCbSelected(CTLSEL_SHANLZFLT_OPRKIND)) {
				if(getCtrlView(CTLSEL_SHANLZFLT_OBJECT)) {
					PPID   acc_sheet_id = 0;
					PPOprKind opk;
					getCtrlData(CTLSEL_SHANLZFLT_OPRKIND, &Data.OpID);
					if(GetOpData(Data.OpID, &opk))
						if(opk.AccSheetID || GetOpData(opk.LinkOpID, &opk))
							acc_sheet_id = opk.AccSheetID;
					setupAccSheet(acc_sheet_id);
				}
				clearEvent(event);
			}
		}
		void   setupAccSheet(PPID accSheetID)
		{
			if(getCtrlView(CTLSEL_SHANLZFLT_OBJECT)) {
				SetupArCombo(this, CTLSEL_SHANLZFLT_OBJECT, Data.ObjectID, 0, accSheetID, sacfDisableIfZeroSheet);
				if(!accSheetID && isCurrCtlID(CTL_SHANLZFLT_OBJECT))
					selectNext();
			}
		}
		ShipmAnalyzeFilt Data;
	};
	DIALOG_PROC_BODY(ShipmAnalyzeFiltDialog, (ShipmAnalyzeFilt*)pFilt);
}


class ShipmAnalyzeCache : public SArray {
public:
	struct Entry {
		PPID   GoodsID;
		PPID   BillID;
		LDATE  Dt;
		double OrderQtty;
		double OrderAmount;
		double ShipmentQtty;
		double ShipmentAmount;
		double AckQtty;
		double AckAmount;
	};
	ShipmAnalyzeCache() : SArray(sizeof(ShipmAnalyzeCache::Entry))
	{
	}
	int    Add(int kind, long flags, const BillTbl::Rec * pBillRec, PPID goodsID, double qtty, double price);
	int    Search(PPID goodsID, uint * pPos) const;
	int    Search(PPID goodsID, PPID billID, uint * pPos) const;
};

int ShipmAnalyzeCache::Search(PPID goodsID, PPID billID, uint * pPos) const
{
	Entry key;
	key.GoodsID = goodsID;
	key.BillID = billID;
	return SArray::lsearch(&key, pPos, PTR_CMPFUNC(_2long));
}

int ShipmAnalyzeCache::Search(PPID goodsID, uint * pPos) const
{
	Entry key;
	key.GoodsID = goodsID;
	return SArray::lsearch(&key, pPos, CMPF_LONG);
}

int ShipmAnalyzeCache::Add(int kind, long flags, const BillTbl::Rec * pBillRec, PPID goodsID, double qtty, double price)
{
	int    ok = 1;
	if(oneof3(kind, 1, 2, 3)) {
		int    s = 0;
		uint   spos = 0;
		if(flags & ShipmAnalyzeFilt::fDiffByBill) {
			s = Search(goodsID, pBillRec->ID, &spos);
		}
		else {
			s = Search(goodsID, &spos);
		}
		if(s) {
			Entry * p_entry = (Entry *)at(spos);
			if(kind == 1) {
				p_entry->OrderQtty += qtty;
				p_entry->OrderAmount += (qtty * price);
			}
			else if(kind == 2) {
				p_entry->ShipmentQtty += qtty;
				p_entry->ShipmentAmount += (qtty * price);
			}
			else if(kind == 3) {
				p_entry->AckQtty += qtty;
				p_entry->AckAmount += (qtty * price);
			}
		}
		else {
			Entry entry;
			MEMSZERO(entry);
			if(flags & ShipmAnalyzeFilt::fDiffByBill) {
				entry.BillID = pBillRec->ID;
				entry.Dt = pBillRec->Dt;
			}
			entry.GoodsID = goodsID;
			if(kind == 1) {
				entry.OrderQtty = qtty;
				entry.OrderAmount = (qtty * price);
			}
			else if(kind == 2) {
				entry.ShipmentQtty = qtty;
				entry.ShipmentAmount = (qtty * price);
			}
			else if(kind == 3) {
				entry.AckQtty = qtty;
				entry.AckAmount = (qtty * price);
			}
			THROW_SL(insert(&entry));
		}
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

PP_CREATE_TEMP_FILE_PROC(CreateTempFile, TempShipmentAnlz);

int SLAPI PPViewShipmAnalyze::Init_(const PPBaseFilt * pFilt)
{
	int    ok = 1;
	Counter.Init();
	BExtQuery::ZDelete(&P_IterQuery);
	ZDELETE(Tbl);
	THROW(Helper_InitBaseFilt(pFilt) > 0);
	GbList.clear();
	PPWait(1);
	if(Filt.OpID) {
		PPID   op_type = GetOpType(Filt.OpID);
		if(oneof3(op_type, PPOPT_GOODSORDER, PPOPT_GOODSACK, PPOPT_GOODSEXPEND)) {
			ShipmAnalyzeCache cache;
			PPIDArray bill_list;
			BillFilt flt;
			PPViewBill bv;
			BillTbl::Rec bill_rec, shipm_bill_rec, ack_bill_rec;
			PPTransferItem ti, shipm_ti, ack_ti;
			Filt.Period.Actualize(ZERODATE); // @v8.7.7
			THROW(AdjustPeriodToRights(Filt.Period, 0));
			Filt.TranslateToBillFilt(&flt);
			THROW(bv.Init_(&flt));
			THROW(bv.GetBillIDList(&bill_list));
			THROW(Tbl = CreateTempFile());
			{
				uint i;
				PPTransaction tra(ppDbDependTransaction, 1);
				THROW(tra);
				for(i = 0; i < bill_list.getCount(); i++) {
					PPID   bill_id = bill_list.at(i);
					PPID   op_type = 0;
					int    r_by_bill, kind = 0;
					THROW(BObj->Search(bill_id, &bill_rec) > 0);
					op_type = GetOpType(bill_rec.OpID);
					if(op_type == PPOPT_GOODSORDER)
						kind = 1;
					else if(op_type == PPOPT_GOODSEXPEND)
						kind = 2;
					else if(op_type == PPOPT_GOODSACK)
						kind = 3;
					else
						continue;
					for(r_by_bill = 0; BObj->trfr->EnumItems(bill_id, &r_by_bill, &ti) > 0;) {
						THROW(cache.Add(kind, Filt.Flags, &bill_rec, labs(ti.GoodsID), fabs(ti.Quantity_), ti.NetPrice()));
						GbList.Add(labs(ti.GoodsID), bill_id, 0);
					}
					if(kind == 1) {
						PPIDArray shipments;
						DateRange shipm_period;
						shipm_period.SetZero();
						if(BObj->GetShipmByOrder(bill_id, &shipm_period, &shipments) > 0) {
							for(uint j = 0; j < shipments.getCount(); j++)
								if(BObj->Search(shipments.at(j), &shipm_bill_rec) > 0) {
									for(int r_by_bill = 0; BObj->trfr->EnumItems(shipm_bill_rec.ID, &r_by_bill, &shipm_ti) > 0;) {
										THROW(cache.Add(2, Filt.Flags, &bill_rec, labs(shipm_ti.GoodsID), fabs(shipm_ti.Quantity_), shipm_ti.NetPrice()));
									}
									if(GetOpType(shipm_bill_rec.OpID) != PPOPT_GOODSACK) {
										for(DateIter di; BObj->P_Tbl->EnumLinks(shipm_bill_rec.ID, &di, BLNK_ACK, &ack_bill_rec) > 0;) {
											for(int r_by_bill = 0; BObj->trfr->EnumItems(ack_bill_rec.ID, &r_by_bill, &ack_ti) > 0;) {
												THROW(cache.Add(3, Filt.Flags, &bill_rec, labs(ack_ti.GoodsID), fabs(ack_ti.Quantity_), ack_ti.NetPrice()))
											}
										}
									}
								}
						}
					}
					else if(kind == 2) {
						for(DateIter di; BObj->P_Tbl->EnumLinks(bill_id, &di, BLNK_ACK, &ack_bill_rec) > 0;) {
							for(int r_by_bill = 0; BObj->trfr->EnumItems(ack_bill_rec.ID, &r_by_bill, &ack_ti) > 0;) {
								THROW(cache.Add(3, Filt.Flags, &bill_rec, labs(ack_ti.GoodsID), fabs(ack_ti.Quantity_), ack_ti.NetPrice()))
							}
						}
					}
					else if(kind == 3) {
						if(bill_rec.LinkBillID && BObj->Search(bill_rec.LinkBillID, &bill_rec) > 0) {
							bill_id = bill_rec.ID;
							for(r_by_bill = 0; BObj->trfr->EnumItems(bill_id, &r_by_bill, &ti) > 0;) {
								THROW(cache.Add(2, Filt.Flags, &bill_rec, labs(ti.GoodsID), fabs(ti.Quantity_), ti.NetPrice()));
							}
						}
					}
					PPWaitPercent(i+1, bill_list.getCount());
				}
				{
					SString goods_name;
					BExtInsert bei(Tbl);
					for(i = 0; i < cache.getCount(); i++) {
						TempShipmentAnlzTbl::Rec rec;
						const ShipmAnalyzeCache::Entry * p_entry = (const ShipmAnalyzeCache::Entry *)cache.at(i);
						MEMSZERO(rec);
						STRNSCPY(rec.GoodsName, GetGoodsName(p_entry->GoodsID, goods_name));
						rec.GoodsID = p_entry->GoodsID;
						rec.BillID  = p_entry->BillID;
						rec.Dt      = p_entry->Dt;
						rec.OrderQtty      = p_entry->OrderQtty;
						rec.OrderAmount    = p_entry->OrderAmount;
						rec.ShipmentQtty   = p_entry->ShipmentQtty;
						rec.ShipmentAmount = p_entry->ShipmentAmount;
						rec.AckQtty        = p_entry->AckQtty;
						rec.AckAmount      = p_entry->AckAmount;
						THROW_DB(bei.insert(&rec));
					}
					THROW_DB(bei.flash());
				}
				THROW(tra.Commit());
			}
		}
	}
	CATCH
		ZDELETE(Tbl);
		ok = 0;
	ENDCATCH
	PPWait(0);
	return ok;
}

int SLAPI PPViewShipmAnalyze::InitIteration()
{
	BExtQuery::ZDelete(&P_IterQuery);
	PPInitIterCounter(Counter, Tbl);
	if(Tbl) {
		P_IterQuery = new BExtQuery(Tbl, 1);
		if(P_IterQuery) {
			char   k[MAXKEYLEN];
			memzero(k, sizeof(k));
			P_IterQuery->selectAll();
			P_IterQuery->initIteration(0, k, spFirst);
			return 1;
		}
		else
			return PPSetErrorNoMem();
	}
	else
		return 0;
}

int FASTCALL PPViewShipmAnalyze::NextIteration(ShipmAnalyzeViewItem * pItem)
{
	while(P_IterQuery && P_IterQuery->nextIteration() > 0) {
		Tbl->copyBufTo(pItem);
		Counter.Increment();
		return 1;
	}
	return -1;
}

// virtual
DBQuery * SLAPI PPViewShipmAnalyze::CreateBrowserQuery(uint * pBrwId, SString * pSubTitle)
{
	uint   brw_id = 0;
	DBQuery * q = 0;
	TempShipmentAnlzTbl * tbl = new TempShipmentAnlzTbl(Tbl->GetName());
	BillTbl * bt = 0;
	if(Filt.Flags & ShipmAnalyzeFilt::fDiffByBill) {
		brw_id = BROWSER_SHIPMANLZBYBILL;
		bt = new BillTbl;
		q = & select(
			tbl->BillID,          // #0
			tbl->GoodsID,         // #1
			tbl->Dt,              // #2
			tbl->GoodsName,       // #3
			tbl->OrderQtty,       // #4
			tbl->OrderAmount,     // #5
			tbl->ShipmentQtty,    // #6
			tbl->ShipmentAmount,  // #7
			tbl->AckQtty,         // #8
			tbl->AckAmount,       // #9
			bt->Code,             // #10
			0L).from(tbl, bt, 0L).where(bt->ID == tbl->BillID).
			orderBy(tbl->Dt, tbl->BillID, tbl->GoodsName, 0L);
	}
	else {
		brw_id = BROWSER_SHIPMANLZ;
		q = & select(
			tbl->BillID,          // #0
			tbl->GoodsID,         // #1
			tbl->Dt,              // #2
			tbl->GoodsName,       // #3
			tbl->OrderQtty,       // #4
			tbl->OrderAmount,     // #5
			tbl->ShipmentQtty,    // #6
			tbl->ShipmentAmount,  // #7
			tbl->AckQtty,         // #8
			tbl->AckAmount,       // #9
			0L).from(tbl, 0L).orderBy(tbl->GoodsName, 0L);
	}
	ASSIGN_PTR(pBrwId, brw_id);
	return q;
}
//
//
//
int SLAPI PPViewShipmAnalyze::Print(const void * pHdr)
{
	uint   rpt_id = (Filt.Flags & ShipmAnalyzeFilt::fDiffByBill) ? REPORT_SHIPMANLZBYBILL : REPORT_SHIPMANLZ;
	return Helper_Print(rpt_id);
}

// virtual
int SLAPI PPViewShipmAnalyze::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	if(ok == -2) {
		const BrwHdr * p_hdr = (const BrwHdr *)pHdr;
		switch(ppvCmd) {
			case PPVCMD_EDITGOODS:
				ok = -1;
				if(p_hdr && p_hdr->GoodsID) {
					PPID   goods_id = p_hdr->GoodsID;
					if(GObj.Edit(&goods_id, 0) == cmOK)
						ok = 1;
				}
				break;
			case PPVCMD_EDITBILL:
				ok = -1;
				if(p_hdr) {
					if(p_hdr->BillID) {
						PPID   bill_id = p_hdr->BillID;
						BObj->Edit(&bill_id, 0);
					}
					else if(p_hdr->GoodsID) {
						PPIDArray bill_list;
						GbList.GetListByKey(p_hdr->GoodsID, bill_list);
						if(bill_list.getCount()) {
							BillFilt bf;
							bf.List.Set(&bill_list);
							ViewGoodsBills(&bf, 1);
						}
					}
				}
				break;
		}
	}
	return ok;
}

//virtual
void SLAPI PPViewShipmAnalyze::PreprocessBrowser(PPViewBrowser * pBrw)
{
}
//
// Implementation of PPALDD_ShipmAnlz
//
PPALDD_CONSTRUCTOR(ShipmAnlz)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		AssignDefIterData(&I, sizeof(I));
	}
}

PPALDD_DESTRUCTOR(ShipmAnlz)
{
	Destroy();
}

int PPALDD_ShipmAnlz::InitData(PPFilt & rFilt, long rsrv)
{
	INIT_PPVIEW_ALDD_DATA_U(ShipmAnalyze, rsrv);
	H.FltLocID      = p_filt->LocID;
	H.FltOpID       = p_filt->OpID;
	H.FltAccSheetID = p_filt->AccSheetID;
	H.FltArID       = p_filt->ObjectID;
	H.FltBeg        = p_filt->Period.low;
	H.FltEnd        = p_filt->Period.upp;
	H.Flags       = p_filt->Flags;
	H.fDebtOnly   = BIN(p_filt->Flags & ShipmAnalyzeFilt::fDebtOnly);
	H.fLabelOnly  = BIN(p_filt->Flags & ShipmAnalyzeFilt::fLabelOnly);
	H.fDiffByBill = BIN(p_filt->Flags & ShipmAnalyzeFilt::fDiffByBill);
	return DlRtm::InitData(rFilt, rsrv);
}

int PPALDD_ShipmAnlz::InitIteration(PPIterID iterId, int sortId, long /*rsrv*/)
{
	INIT_PPVIEW_ALDD_ITER(ShipmAnalyze);
}

int PPALDD_ShipmAnlz::NextIteration(PPIterID iterId)
{
	START_PPVIEW_ALDD_ITER(ShipmAnalyze);
	I.BillID         = item.BillID;
	I.GoodsID        = item.GoodsID;
	I.OrderQtty      = item.OrderQtty;
	I.OrderAmount    = item.OrderAmount;
	I.ShipmentQtty   = item.ShipmentQtty;
	I.ShipmentAmount = item.ShipmentAmount;
	I.AckQtty        = item.AckQtty;
	I.AckAmount      = item.AckAmount;
	FINISH_PPVIEW_ALDD_ITER();
}

void PPALDD_ShipmAnlz::Destroy()
{
	DESTROY_PPVIEW_ALDD(ShipmAnalyze);
}

