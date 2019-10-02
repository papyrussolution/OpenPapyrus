// V_QREQ.CPP
// Copyright (c) A.Starodub 2008, 2009, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018
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

int SLAPI PPViewQuoteReqAnalyze::Init_(const PPBaseFilt * pBaseFilt)
{
	int    ok = 1;
	PPObjBill * p_bobj = BillObj;
	THROW(Helper_InitBaseFilt(pBaseFilt));
	List.clear();
	CALLPTRMEMB(P_DsList, clear());
	{
		PPIDArray op_list;
		PPViewBill bill_view;
		BillFilt bill_filt;
		if(Filt.OpID) {
			op_list.add(Filt.OpID);
		}
		else {
			PPOprKind op_rec;
			for(PPID op_id = 0; EnumOperations(PPOPT_DRAFTQUOTREQ, &op_id, &op_rec) > 0;) {
				op_list.add(op_id);
			}
		}
		if(op_list.getCount()) {
			PPBillPacket bpack;
			for(uint opidx = 0; opidx < op_list.getCount(); opidx++) {
				const PPID op_id = op_list.get(opidx);
				BillTbl::Rec bill_rec;
				for(SEnum en = p_bobj->P_Tbl->EnumByOp(op_id, &Filt.Period, 0); en.Next(&bill_rec) > 0;) {
					const PPID bill_id = bill_rec.ID;
					if(p_bobj->ExtractPacket(bill_id, &bpack) > 0) {
						for(uint tiidx = 0; tiidx < bpack.GetTCount(); tiidx++) {
							const PPTransferItem & r_ti = bpack.ConstTI(tiidx);
							BrwItem new_item;
							MEMSZERO(new_item);
							/*
								struct BrwItem {
									PPID   LeadBillID;
									int    LeadRbb;
									PPID   LeadArID;
									PPID   GoodsID;
									double ReqQtty;
									double ReqPrice;
									PPID   ReqCurID;
									int    ReqShipmTerm; // Требуемый срок поставки в днях
									PPID   LinkBillID;
									int    LinkRbb;
									PPID   LinkArID;
									double RepQtty;
									double RepPrice;
									PPID   RepCurID;
									int    RepShipmTerm; // Возможный срок поставки в днях
								};
							*/
							new_item.LeadBillID = bill_id;
							new_item.LeadRbb = r_ti.RByBill;
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
		}
	}
	CATCHZOK
	return ok;
}

class QuoteReqFiltDialog : public TDialog {
public:
	enum {
		ctlgroupAr = 1
	};
	QuoteReqFiltDialog() : TDialog(DLG_QUOTEREQFLT)
	{
		addGroup(ctlgroupAr,  new ArticleCtrlGroup(0, CTLSEL_QUOTEREQFLT_OP, CTLSEL_QUOTEREQFLT_AR,  0, 0, 0));
		SetupCalPeriod(CTLCAL_QUOTEREQFLT_PERIOD, CTL_QUOTEREQFLT_PERIOD);
	}
	int    setDTS(const QuoteReqAnalyzeFilt * pData)
	{
		int    ok = 1;
		PPIDArray op_type_list;
		RVALUEPTR(Data, pData);
		SetPeriodInput(this, CTL_QUOTEREQFLT_PERIOD, &Data.Period);
		op_type_list.add(PPOPT_DRAFTQUOTREQ);
		SetupOprKindCombo(this, CTLSEL_QUOTEREQFLT_OP, Data.OpID, 0, &op_type_list, 0);
		{
			ArticleCtrlGroup::Rec ar_grp_rec(0, Data.OpID, Data.ArID);
			ArticleCtrlGroup * p_ar_grp = static_cast<ArticleCtrlGroup *>(getGroup(ctlgroupAr));
			setGroupData(ctlgroupAr, &ar_grp_rec);
		}
		return ok;
	}
	int    getDTS(QuoteReqAnalyzeFilt * pData)
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
private:
	QuoteReqAnalyzeFilt Data;
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
	THROW(MakeList());
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
				if(EditTransferItem(&seq_bpack, -1, &tidid, 0, 0) == cmOK) {
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
					if(P_BObj->Fetch(p_item->LeadBillID, &bill_rec) > 0)
						pBlk->Set(bill_rec.Dt); 
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
					pBlk->Set(ZERODATE);
					break; 
				case 11: // LinkBillCode
					break; 
				case 12: // LinkContractor
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

int SLAPI PPViewQuoteReqAnalyze::MakeList()
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

