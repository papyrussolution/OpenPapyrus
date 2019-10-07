// V_QREQ.CPP
// Copyright (c) A.Sobolev 2019
// @codepage UTF-8
//
// @moduledef(PPViewQuoteReqAnalyze) Анализ котировочных запросов
//
#include <pp.h>
#pragma hdrstop

IMPLEMENT_PPFILT_FACTORY(QuoteReqAnalyze); SLAPI QuoteReqAnalyzeFilt::QuoteReqAnalyzeFilt() : PPBaseFilt(PPFILT_QUOTEREQANALYZE, 0, 1)
{
	SetFlatChunk(offsetof(QuoteReqAnalyzeFilt, ReserveStart),
		offsetof(QuoteReqAnalyzeFilt, ReserveEnd)-offsetof(QuoteReqAnalyzeFilt, ReserveStart)+sizeof(ReserveEnd));
	Init(1, 0);
}

SLAPI PPViewQuoteReqAnalyze::PPViewQuoteReqAnalyze() : PPView(0, &Filt, PPVIEW_QUOTEREQANALYZE), P_BObj(BillObj), P_DsList(0)
{
	ImplementFlags |= (implBrowseArray);
}

SLAPI PPViewQuoteReqAnalyze::~PPViewQuoteReqAnalyze()
{
	ZDELETE(P_DsList);
	P_BObj = 0;
}

int SLAPI PPViewQuoteReqAnalyze::FinishListBySeq(PPID leadBillID, int leadRbb)
{
	int    ok = 1;
	TSArray <PPObjBill::QuoteReqLink> qrl_list;
	const uint org_lcount = List.getCount();
	BillTbl::Rec seq_bill_rec;
	DateRange scope_period;
	scope_period.Z();
	{
		for(uint lidx = 0; lidx < org_lcount; lidx++) {
			const BrwItem & r_item = List.at(lidx);
			if(scope_period.IsZero())
				scope_period.SetDate(r_item.LeadDt);
			else
				scope_period.AdjustToDate(r_item.LeadDt);
		}
	}
	DateRange link_period = scope_period;
	link_period.upp = ZERODATE;
	P_BObj->SearchQuoteReqSeq(&link_period, qrl_list);
	for(uint lidx = 0; lidx < org_lcount; lidx++) {
		BrwItem & r_item = List.at(lidx);
		int    is_there_anything = 0;
		for(uint j = 0; j < qrl_list.getCount(); j++) {
			const PPObjBill::QuoteReqLink & r_qrl_item = qrl_list.at(j);
			if(r_qrl_item.LeadBillID == r_item.LeadBillID && r_qrl_item.LeadRbb == r_item.LeadRbb) {
				if(!is_there_anything) {
					r_item.LinkBillID = r_qrl_item.SeqBillID;
					r_item.LinkRbb = r_qrl_item.SeqRbb;
					if(P_BObj->Fetch(r_qrl_item.SeqBillID, &seq_bill_rec) > 0) {
						r_item.LinkArID = seq_bill_rec.Object;
						r_item.LinkDt = seq_bill_rec.Dt;
					}
					else {
						r_item.LinkArID = 0;
						r_item.LinkDt = ZERODATE;
					}
				}
				else {
					BrwItem new_item(r_item);
					new_item.LinkBillID = r_qrl_item.SeqBillID;
					new_item.LinkRbb = r_qrl_item.SeqRbb;
					if(P_BObj->Fetch(r_qrl_item.SeqBillID, &seq_bill_rec) > 0) {
						new_item.LinkArID = seq_bill_rec.Object;
						new_item.LinkDt = seq_bill_rec.Dt;
					}
					else {
						new_item.LinkArID = 0;
						new_item.LinkDt = ZERODATE;
					}
					List.insert(&new_item);
				}
				is_there_anything = 1;
			}
		}
	}
	return ok;
}

int SLAPI PPViewQuoteReqAnalyze::CreateList(PPID leadBillID, int leadRbb)
{
	int    ok = 1;
	if(!leadBillID) {
		PPIDArray op_list;
		PPOprKind op_rec;
		List.clear();
		if(Filt.OpID) {
			op_list.add(Filt.OpID);
		}
		else {
			for(PPID op_id = 0; EnumOperations(PPOPT_DRAFTQUOTREQ, &op_id, &op_rec) > 0;) {
				if(!op_rec.LinkOpID)
					op_list.add(op_id);
			}
		}
		if(op_list.getCount()) {
			PPBillPacket bpack;
			for(uint opidx = 0; opidx < op_list.getCount(); opidx++) {
				const PPID op_id = op_list.get(opidx);
				BillTbl::Rec bill_rec;
				for(SEnum en = P_BObj->P_Tbl->EnumByOp(op_id, &Filt.Period, 0); en.Next(&bill_rec) > 0;) {
					const PPID bill_id = bill_rec.ID;
					if(P_BObj->ExtractPacket(bill_id, &bpack) > 0) {
						for(uint tiidx = 0; tiidx < bpack.GetTCount(); tiidx++) {
							const PPTransferItem & r_ti = bpack.ConstTI(tiidx);
							BrwItem new_item;
							MEMSZERO(new_item);
							new_item.LeadBillID = bill_id;
							new_item.LeadRbb = r_ti.RByBill;
							new_item.LeadDt = bpack.Rec.Dt;
							new_item.LeadArID = bpack.Rec.Object;
							new_item.GoodsID = r_ti.GoodsID;
							new_item.ReqQtty = fabs(r_ti.Quantity_);
							new_item.ReqPrice = r_ti.Price;
							new_item.ReqCurID = bpack.Rec.CurID;
							THROW_SL(List.insert(&new_item));
						}
					}
				}
			}
			THROW(FinishListBySeq(0, 0));
		}
	}
	else {
	}
	CATCHZOK
	return ok;
}

int SLAPI PPViewQuoteReqAnalyze::Init_(const PPBaseFilt * pBaseFilt)
{
	int    ok = 1;
	THROW(Helper_InitBaseFilt(pBaseFilt));
	List.clear();
	CALLPTRMEMB(P_DsList, clear());
	THROW(CreateList(0, 0));
	CATCHZOK
	return ok;
}

class QuoteReqFiltDialog : public TDialog {
	typedef QuoteReqAnalyzeFilt DlgDataType;
	DlgDataType Data;
public:
	enum {
		ctlgroupAr = 1
	};
	QuoteReqFiltDialog() : TDialog(DLG_QUOTEREQFLT)
	{
		addGroup(ctlgroupAr,  new ArticleCtrlGroup(0, CTLSEL_QUOTEREQFLT_OP, CTLSEL_QUOTEREQFLT_AR,  0, 0, 0));
		SetupCalPeriod(CTLCAL_QUOTEREQFLT_PERIOD, CTL_QUOTEREQFLT_PERIOD);
	}
	int    setDTS(const DlgDataType * pData)
	{
		int    ok = 1;
		PPIDArray op_type_list;
		RVALUEPTR(Data, pData);
		SetPeriodInput(this, CTL_QUOTEREQFLT_PERIOD, &Data.Period);
		op_type_list.add(PPOPT_DRAFTQUOTREQ);
		SetupOprKindCombo(this, CTLSEL_QUOTEREQFLT_OP, Data.OpID, 0, &op_type_list, 0);
		{
			ArticleCtrlGroup::Rec ar_grp_rec(0, Data.OpID, Data.ArID);
			setGroupData(ctlgroupAr, &ar_grp_rec);
		}
		return ok;
	}
	int    getDTS(DlgDataType * pData)
	{
		int    ok = 1;
		uint   sel = 0;
		THROW(GetPeriodInput(this, sel = CTL_QUOTEREQFLT_PERIOD, &Data.Period));
		getCtrlData(CTLSEL_QUOTEREQFLT_OP, &Data.OpID);
		{
			ArticleCtrlGroup::Rec ar_grp_rec;
			getGroupData(ctlgroupAr, &ar_grp_rec);
			Data.ArID = ar_grp_rec.ArList.GetSingle();
		}
		ASSIGN_PTR(pData, Data);
		CATCHZOKPPERRBYDLG
		return ok;
	}
};

int SLAPI PPViewQuoteReqAnalyze::EditBaseFilt(PPBaseFilt * pBaseFilt)
{
	if(!Filt.IsA(pBaseFilt))
		return 0;
	QuoteReqAnalyzeFilt * p_filt = static_cast<QuoteReqAnalyzeFilt *>(pBaseFilt);
	DIALOG_PROC_BODY(QuoteReqFiltDialog, p_filt);
}

int SLAPI PPViewQuoteReqAnalyze::InitIteration()
{
	int    ok = -1;
	return ok;
}

int FASTCALL PPViewQuoteReqAnalyze::NextIteration(QuoteReqAnalyzeViewItem * pItem)
{
	int    ok = -1;
	return ok;
}

//static 
int SLAPI PPViewQuoteReqAnalyze::CellStyleFunc_(const void * pData, long col, int paintAction, BrowserWindow::CellStyle * pCellStyle, PPViewBrowser * pBrw)
{
	int    ok = 1;
	return ok;
}

//static 
int FASTCALL PPViewQuoteReqAnalyze::GetDataForBrowser(SBrowserDataProcBlock * pBlk)
{
	PPViewQuoteReqAnalyze * p_v = static_cast<PPViewQuoteReqAnalyze *>(pBlk->ExtraPtr);
	return p_v ? p_v->_GetDataForBrowser(pBlk) : 0;
}

SArray * SLAPI PPViewQuoteReqAnalyze::CreateBrowserArray(uint * pBrwId, SString * pSubTitle)
{
	uint   brw_id = BROWSER_QUOTEREQANALYZE;
	SArray * p_array = 0;
	THROW(MakeViewList());
	p_array = new SArray(*P_DsList);
	CATCH
		ZDELETE(P_DsList);
	ENDCATCH
	ASSIGN_PTR(pBrwId, brw_id);
	return p_array;
}

void SLAPI PPViewQuoteReqAnalyze::PreprocessBrowser(PPViewBrowser * pBrw)
{
	if(pBrw) {
		pBrw->SetDefUserProc(PPViewQuoteReqAnalyze::GetDataForBrowser, this);
		//pBrw->SetCellStyleFunc(CellStyleFunc, pBrw);
	}
}

int SLAPI PPViewQuoteReqAnalyze::OnExecBrowser(PPViewBrowser * pBrw)
{
	return -1;
}

int SLAPI PPViewQuoteReqAnalyze::CreateLinkedRequest(PPID leadBillID, int leadRbb)
{
	int    ok = -1;
	if(leadBillID && leadRbb) {
		PPBillPacket lead_bpack;
		PPObjOprKind op_obj;
		uint lead_tipos = 0;
		if(P_BObj->ExtractPacket(leadBillID, &lead_bpack) > 0 && lead_bpack.SearchTI(leadRbb, &lead_tipos)) {
			PPOprKind lead_op_rec;
			PPIDArray seq_op_list;
			const PPTransferItem & r_lead_ti = lead_bpack.ConstTI(lead_tipos);
			op_obj.GetQuoteReqSeqOpList(lead_bpack.Rec.OpID, &seq_op_list);
			if(seq_op_list.getCount()) {
				PPID   seq_op_id = seq_op_list.get(0);
				PPBillPacket seq_bpack;
				TIDlgInitData tidid;
				tidid.GoodsID = r_lead_ti.GoodsID;
				tidid.Quantity = fabs(r_lead_ti.Quantity_);
				tidid.Flags |= TIDIF_SEQQREQ;
				THROW(seq_bpack.CreateBlank(seq_op_id, lead_bpack.Rec.ID, lead_bpack.Rec.LocID, 1));
				seq_bpack.Rec.Dt = lead_bpack.Rec.Dt;
				seq_bpack.Rec.DueDate = lead_bpack.Rec.DueDate;
				if(EditTransferItem(&seq_bpack, -1, &tidid, 0, 0) == cmOK) {
					const  uint tipos = seq_bpack.GetTCount() - 1;
					PPTransferItem * p_ti = &seq_bpack.TI(tipos);
					p_ti->Cost = 0.0;
					p_ti->Discount = 0.0;
					if(p_ti->Suppl) {
						PPBillPacket::SetupObjectBlock sob;
						seq_bpack.SetupObject(p_ti->Suppl, sob);
					}
					p_ti->Lbr.ID = leadBillID;
					p_ti->Lbr.RByBill = leadRbb;
					THROW(seq_bpack.InitAmounts(0));
					THROW(P_BObj->TurnPacket(&seq_bpack, 1));
					CreateList(0, 0); // @todo Список надо перестроить только по одному документу
					ok = 1;
				}
			}
		}
	}
	CATCHZOKPPERR
	return ok;
}

int SLAPI PPViewQuoteReqAnalyze::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	if(ok == -2) {
		const  uint idx = pHdr ? *static_cast<const uint *>(pHdr) : 0;
		const  BrwItem * p_item = (idx > 0 && idx <= List.getCount()) ? &List.at(idx-1) : 0;
		//if(idx)
		switch(ppvCmd) {
			case PPVCMD_CREATELINKQR:
				ok = -1;
				if(p_item) {
					ok = CreateLinkedRequest(p_item->LeadBillID, p_item->LeadRbb);
				}
				break;
		}
	}
	return ok;
}

int SLAPI PPViewQuoteReqAnalyze::Detail(const void *, PPViewBrowser * pBrw)
{
	int    ok = -1;
	return ok;
}

int SLAPI PPViewQuoteReqAnalyze::_GetDataForBrowser(SBrowserDataProcBlock * pBlk)
{
	int    ok = 0;
	if(pBlk->P_SrcData && pBlk->P_DestData) {
		const  uint idx = *static_cast<const uint *>(pBlk->P_SrcData);
		const  BrwItem * p_item = (idx > 0 && idx <= List.getCount()) ? &List.at(idx-1) : 0;
		if(p_item) {
			ok = 1;
			int    r = 0;
			BillTbl::Rec bill_rec;
			SString temp_buf;
			switch(pBlk->ColumnN) {
				case 0: pBlk->Set(static_cast<int32>(idx)); break; // Index
				case 1: pBlk->Set(p_item->LeadBillID); break; // LeadBillID
				case 2: // LeadBillDate
					pBlk->Set(p_item->LeadDt); 
					break; 
				case 3: // LeadBillCode
					if(P_BObj->Fetch(p_item->LeadBillID, &bill_rec) > 0)
						pBlk->Set(bill_rec.Code); 
					break; 
				case 4: // LeadContractor
					GetArticleName(p_item->LeadArID, temp_buf);
					pBlk->Set(temp_buf); 
					break; 
				case 5: // ware
					pBlk->Set(GetGoodsName(p_item->GoodsID, temp_buf)); 
					break; 
				case 6: // ReqQtty
					pBlk->Set(p_item->ReqQtty); 
					break; 
				case 7: // ReqCurrency
					if(P_BObj->Fetch(p_item->LeadBillID, &bill_rec) > 0) {
						if(bill_rec.CurID) {
							PPCurrency cur_rec;
							if(CurObj.Fetch(bill_rec.CurID, &cur_rec) > 0) {
								if(cur_rec.Symb[0])
									temp_buf = cur_rec.Symb;
								else
									temp_buf = cur_rec.Name;
							}
						}
						pBlk->Set(temp_buf); 
					}
					break; 
				case 8: // ReqPrice
					pBlk->Set(p_item->ReqPrice); 
					break; 
				case 9: // ReqShipmTerm
					if(p_item->ReqShipmTerm > 0) {
						temp_buf.Cat(p_item->ReqShipmTerm).Cat("days");
					}
					pBlk->Set(temp_buf);
					break; 
				case 10: // LinkBillDate
					pBlk->Set(p_item->LinkDt);
					break; 
				case 11: // LinkBillCode
					if(P_BObj->Fetch(p_item->LinkBillID, &bill_rec) > 0)
						pBlk->Set(bill_rec.Code); 
					break; 
				case 12: // LinkContractor
					GetArticleName(p_item->LinkArID, temp_buf);
					pBlk->Set(temp_buf); 
					break; 
				case 13: // RepQtty
					pBlk->Set(0.0);
					break; 
				case 14: // RepCurrency
					if(p_item->RepCurID) {
						PPCurrency cur_rec;
						if(CurObj.Fetch(p_item->RepCurID, &cur_rec) > 0) {
							if(cur_rec.Symb[0])
								temp_buf = cur_rec.Symb;
							else
								temp_buf = cur_rec.Name;
						}
					}
					pBlk->Set(temp_buf); 
					break; 
				case 15: // RepPrice
					pBlk->Set(0.0);
					break; 
				case 16: // RepShipmTerm
					if(p_item->RepShipmTerm > 0) {
						temp_buf.Cat(p_item->RepShipmTerm).Cat("days");
					}
					pBlk->Set(temp_buf);
					break; 
			}
		}
	}
	return ok;
}

int SLAPI PPViewQuoteReqAnalyze::MakeViewList()
{
	int    ok = 1;
	if(P_DsList)
		P_DsList->clear();
	else {
		THROW_SL(P_DsList = new SArray(sizeof(uint)));
	}
	for(uint i = 0; i < List.getCount(); i++) {
		uint idx = i+1;
		THROW_SL(P_DsList->insert(&idx));
	}
	CATCHZOK
	return ok;
}

