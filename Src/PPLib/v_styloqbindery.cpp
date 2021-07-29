// V_STYLOQBINDERY.CPP
// Copyright (c) A.Sobolev 2021
//
#include <pp.h>
#pragma hdrstop

IMPLEMENT_PPFILT_FACTORY(StyloQBindery); StyloQBinderyFilt::StyloQBinderyFilt() : PPBaseFilt(PPFILT_STYLOQBINDERY, 0, 0)
{
	SetFlatChunk(offsetof(StyloQBinderyFilt, ReserveStart),
		offsetof(StyloQBinderyFilt, Reserve)-offsetof(StyloQBinderyFilt, ReserveStart)+sizeof(Reserve));
	Init(1, 0);
}

PPViewStyloQBindery::PPViewStyloQBindery() : PPView(0, &Filt, PPVIEW_STYLOQBINDERY, (implBrowseArray|implDontEditNullFilter), 0), P_DsList(0)
{
}

PPViewStyloQBindery::~PPViewStyloQBindery()
{
	ZDELETE(P_DsList);
}

/*virtual*/int PPViewStyloQBindery::Init_(const PPBaseFilt * pBaseFilt)
{
	int    ok = 1;
	THROW(Helper_InitBaseFilt(pBaseFilt));
	BExtQuery::ZDelete(&P_IterQuery);
	Counter.Init();
	CATCHZOK
	return ok;
}

/*virtual*/int PPViewStyloQBindery::EditBaseFilt(PPBaseFilt * pBaseFilt)
{
	return -1;
}

int PPViewStyloQBindery::MakeList(PPViewBrowser * pBrw)
{
	int    ok = 1;
	if(P_DsList)
		P_DsList->clear();
	else {
		THROW_SL(P_DsList = new SArray(sizeof(uint)));
	}
	{
		StyloQSecTbl::Key0 k0;
		MEMSZERO(k0);
		if(T.search(0, &k0, spFirst)) do {
			
		} while(T.search(&k0, spNext));
	}
	/*for(uint i = 0; i < Ts.GetCount(); i++) {
		uint idx = i+1;
		THROW_SL(P_DsList->insert(&idx));
	}*/
	CATCHZOK
	return ok;
}

/*virtual*/SArray * PPViewStyloQBindery::CreateBrowserArray(uint * pBrwId, SString * pSubTitle)
{
	uint   brw_id = BROWSER_STYLOQBINDERY;
	SArray * p_array = 0;
	PPTimeSeries ds_item;
	THROW(MakeList(0));
	p_array = new SArray(*P_DsList);
	CATCH
		ZDELETE(P_DsList);
	ENDCATCH
	ASSIGN_PTR(pBrwId, brw_id);
	return p_array;
}

int PPViewStyloQBindery::_GetDataForBrowser(SBrowserDataProcBlock * pBlk)
{
	int    ok = 1;
	if(pBlk->P_SrcData && pBlk->P_DestData) {
		ok = 1;
		const  uint idx = *static_cast<const uint *>(pBlk->P_SrcData) - 1;
		int    r = 0;
		if(pBlk->ColumnN == 0)
			pBlk->Set(static_cast<int32>(idx));
		else {
			/*
			if(pBlk->ColumnN == 1) {
				SUniTime ut;
				LDATETIME dtm;
				Ts.GetTime(idx, &ut);
				ut.Get(dtm);
				SString & r_temp_buf = SLS.AcquireRvlStr();
				r_temp_buf.Cat(dtm, DATF_ISO8601|DATF_CENTURY, 0);
				pBlk->Set(r_temp_buf);
			}
			else if(pBlk->ColumnN == 2) {
				int32 diffsec = 0;
				if(idx > 0) {
					SUniTime ut;
					LDATETIME dtm;
					LDATETIME dtm2;
					Ts.GetTime(idx, &ut);
					ut.Get(dtm);
					Ts.GetTime(idx-1, &ut);
					ut.Get(dtm2);
					diffsec = diffdatetimesec(dtm, dtm2);
				}
				pBlk->Set(diffsec);
			}
			else if(pBlk->ColumnN > 2 && pBlk->ColumnN < static_cast<int>(Ts.GetValueVecCount()+3)) {
				double value = 0.0;
				Ts.GetValue(idx, pBlk->ColumnN-3, &value);
				pBlk->Set(value);
			}
			*/
		}
	}
	return ok;
}

//static
int FASTCALL PPViewStyloQBindery::GetDataForBrowser(SBrowserDataProcBlock * pBlk)
{
	PPViewStyloQBindery * p_v = static_cast<PPViewStyloQBindery *>(pBlk->ExtraPtr);
	return p_v ? p_v->_GetDataForBrowser(pBlk) : 0;
}

static int CellStyleFunc(const void * pData, long col, int paintAction, BrowserWindow::CellStyle * pStyle, void * extraPtr)
{
	int    ok = -1;
	PPViewBrowser * p_brw = static_cast<PPViewBrowser *>(extraPtr);
	if(p_brw) {
		PPViewStyloQBindery * p_view = static_cast<PPViewStyloQBindery *>(p_brw->P_View);
		ok = p_view ? p_view->CellStyleFunc_(pData, col, paintAction, pStyle, p_brw) : -1;
	}
	return ok;
}

int PPViewStyloQBindery::CellStyleFunc_(const void * pData, long col, int paintAction, BrowserWindow::CellStyle * pCellStyle, PPViewBrowser * pBrw)
{
	int    ok = -1;
	if(pBrw && pData && pCellStyle && col >= 0) {
		const BrowserDef * p_def = pBrw->getDef();
		if(col < static_cast<long>(p_def->getCount())) {
			/*
			const BroColumn & r_col = p_def->at(col);
			if(col == 0) { // id
				const long cfg_flags = static_cast<const BrwItem *>(pData)->CfgFlags;
				if(cfg_flags & PPObjTimeSeries::Config::efDisableStake)
					ok = pCellStyle->SetLeftTopCornerColor(GetColorRef(SClrGrey));
				if(cfg_flags & PPObjTimeSeries::Config::efLong && cfg_flags & PPObjTimeSeries::Config::efShort)
					ok = pCellStyle->SetRightFigCircleColor(GetColorRef(SClrOrange));
				else if(cfg_flags & PPObjTimeSeries::Config::efLong)
					ok = pCellStyle->SetRightFigCircleColor(GetColorRef(SClrGreen));
				else if(cfg_flags & PPObjTimeSeries::Config::efShort)
					ok = pCellStyle->SetRightFigCircleColor(GetColorRef(SClrRed));
			}
			else if(col == 2) { // name
				const int16 type = static_cast<const BrwItem *>(pData)->Type;
				if(type == PPTimeSeries::tForex)
					ok = pCellStyle->SetRightFigCircleColor(GetColorRef(SClrPink));
				else if(type == PPTimeSeries::tStocks)
					ok = pCellStyle->SetRightFigCircleColor(GetColorRef(SClrLightblue));
				else if(type == PPTimeSeries::tCrypto) // @v10.8.7
					ok = pCellStyle->SetRightFigCircleColor(GetColorRef(SClrLightgreen));
			}
			*/
		}
	}
	return ok;
}
	
/*virtual*/void PPViewStyloQBindery::PreprocessBrowser(PPViewBrowser * pBrw)
{
	if(pBrw) {
		pBrw->SetDefUserProc(PPViewStyloQBindery::GetDataForBrowser, this);
		pBrw->SetCellStyleFunc(CellStyleFunc, pBrw);
		//pBrw->Helper_SetAllColumnsSortable();
	}
}
	
/*virtual*/int PPViewStyloQBindery::OnExecBrowser(PPViewBrowser *)
{
	return -1;
}
	
/*virtual*/int PPViewStyloQBindery::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = -2;
	PPID   id = pHdr ? *static_cast<const PPID *>(pHdr) : 0;
	const  PPID preserve_id = id;
	ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	if(ok == -2) {
		switch(ppvCmd) {
			case PPVCMD_USERSORT: ok = 1; break; // The rest will be done below
			case PPVCMD_DELETEALL:
				// @todo ok = DeleteAll();
				break;
			case PPVCMD_REFRESH:
				ok = 1;
				break;
		}
	}
	if(ok > 0) {
		MakeList(pBrw);
		if(pBrw) {
			AryBrowserDef * p_def = static_cast<AryBrowserDef *>(pBrw->getDef());
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

/*virtual*/int PPViewStyloQBindery::Detail(const void *, PPViewBrowser * pBrw)
{
	return -1;
}