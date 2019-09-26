// V_QREQ.CPP
// Copyright (c) A.Starodub 2008, 2009, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018
// @codepage UTF-8
//
// @moduledef(PPViewQuoteReq) Анализ котировочных запросов
//
#include <pp.h>
#pragma hdrstop

class QuoteReqFilt : public PPBaseFilt {
public:
	SLAPI  QuoteReqFilt();

	uint8  ReserveStart[128]; // @anchor
	DateRange Period;
	PPID   OpID;
	PPID   ArID;
	PPID   LocID;
	long   Flags;
	long   SortOrd;
	long   ReserveEnd;        // @anchor
};

struct QuoteReqViewItem {
	PPID   LeadBillID;
	int    LeadRbb;
	PPID   LinkBillID;
	int    LinkRbb;
	PPID   GoodsID;
};

class PPViewQuoteReq : public PPView {
public:
	struct BrwItem {
		PPID   LeadBillID;
		PPID   LeadRbb;
		PPID   GoodsID;
		double ReqQtty;
		double ReqPrice;
		PPID   ReqCurID;
	};
	SLAPI  PPViewQuoteReq();
	SLAPI ~PPViewQuoteReq();
	virtual int SLAPI Init_(const PPBaseFilt * pBaseFilt);
	virtual int SLAPI EditBaseFilt(PPBaseFilt * pBaseFilt);
	int    SLAPI InitIteration();
	int    FASTCALL NextIteration(QuoteReqViewItem *);
	static int SLAPI CellStyleFunc_(const void * pData, long col, int paintAction, BrowserWindow::CellStyle * pCellStyle, PPViewBrowser * pBrw);
private:
	static int FASTCALL GetDataForBrowser(SBrowserDataProcBlock * pBlk);
	virtual SArray * SLAPI CreateBrowserArray(uint * pBrwId, SString * pSubTitle);
	virtual void SLAPI PreprocessBrowser(PPViewBrowser * pBrw);
	virtual int  SLAPI OnExecBrowser(PPViewBrowser *);
	virtual int  SLAPI ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw);
	virtual int  SLAPI Detail(const void *, PPViewBrowser * pBrw);
	int    SLAPI _GetDataForBrowser(SBrowserDataProcBlock * pBlk);
	int    SLAPI MakeList();

	QuoteReqFilt Filt;
	TSArray <BrwItem> List;
};

IMPLEMENT_PPFILT_FACTORY(QuoteReq); SLAPI QuoteReqFilt::QuoteReqFilt() : PPBaseFilt(PPFILT_QUOTEREQANALYZE, 0, 1)
{
	SetFlatChunk(offsetof(QuoteReqFilt, ReserveStart),
		offsetof(QuoteReqFilt, ReserveEnd)-offsetof(QuoteReqFilt, ReserveStart)+sizeof(ReserveEnd));
	Init(1, 0);
}

SLAPI PPViewQuoteReq::PPViewQuoteReq() : PPView(0, &Filt, PPVIEW_QUOTEREQANALYZE)
{
}

SLAPI PPViewQuoteReq::~PPViewQuoteReq()
{
}

int SLAPI PPViewQuoteReq::Init_(const PPBaseFilt * pBaseFilt)
{
	int    ok = 1;
	PPObjBill * p_bobj = BillObj;
	THROW(Helper_InitBaseFilt(pBaseFilt));
	List.clear();
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
							new_item.LeadBillID = bill_id;
							new_item.LeadRbb = r_ti.RByBill;
							new_item.GoodsID = r_ti.GoodsID;

						}
					}
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PPViewQuoteReq::EditBaseFilt(PPBaseFilt * pBaseFilt)
{
	int    ok = -1;
	return ok;
}

int SLAPI PPViewQuoteReq::InitIteration()
{
	int    ok = -1;
	return ok;
}

int FASTCALL PPViewQuoteReq::NextIteration(QuoteReqViewItem * pItem)
{
	int    ok = -1;
	return ok;
}

//static 
int SLAPI PPViewQuoteReq::CellStyleFunc_(const void * pData, long col, int paintAction, BrowserWindow::CellStyle * pCellStyle, PPViewBrowser * pBrw)
{
	int    ok = 1;
	return ok;
}

//static 
int FASTCALL PPViewQuoteReq::GetDataForBrowser(SBrowserDataProcBlock * pBlk)
{
	PPViewQuoteReq * p_v = static_cast<PPViewQuoteReq *>(pBlk->ExtraPtr);
	return p_v ? p_v->_GetDataForBrowser(pBlk) : 0;
}

SArray * SLAPI PPViewQuoteReq::CreateBrowserArray(uint * pBrwId, SString * pSubTitle)
{
	SArray * p_list = 0;
	return p_list;
}

void SLAPI PPViewQuoteReq::PreprocessBrowser(PPViewBrowser * pBrw)
{
}

int SLAPI PPViewQuoteReq::OnExecBrowser(PPViewBrowser * pBrw)
{
	return -1;
}

int SLAPI PPViewQuoteReq::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	if(ok == -2) {
	}
	return ok;
}

int SLAPI PPViewQuoteReq::Detail(const void *, PPViewBrowser * pBrw)
{
	int    ok = -1;
	return ok;
}

int SLAPI PPViewQuoteReq::_GetDataForBrowser(SBrowserDataProcBlock * pBlk)
{
	int    ok = 1;
	return ok;
}

int SLAPI PPViewQuoteReq::MakeList()
{
	int    ok = 1;
	return ok;
}

