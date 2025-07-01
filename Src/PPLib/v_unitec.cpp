// V_UNITEC.CPP
// Copyright (c) A.Sobolev 2025
// @codepage UTF-8
// Аналитическая таблица в стиле unit-economics 
//
#include <pp.h>
#pragma hdrstop

IMPLEMENT_PPFILT_FACTORY(UnitEc); UnitEcFilt::UnitEcFilt() : PPBaseFilt(PPFILT_UNITEC, 0, 0)
{
	SetFlatChunk(offsetof(UnitEcFilt, ReserveStart),
		offsetof(UnitEcFilt, Reserve)-offsetof(UnitEcFilt, ReserveStart)+sizeof(Reserve));
	Init(1, 0);
}

UnitEcFilt & FASTCALL UnitEcFilt::operator = (const UnitEcFilt & rS)
{
	Copy(&rS, 1);
	return *this;
}

PPViewUnitEc::PPViewUnitEc() : PPView(0, &Filt, PPVIEW_UNITEC, (implBrowseArray), 0), P_DsList(0)
{
}

PPViewUnitEc::~PPViewUnitEc()
{
	delete P_DsList;
}

int PPViewUnitEc::InitIteration()
{
	return -1;
}

int FASTCALL PPViewUnitEc::NextIteration(UnitEcViewItem * pItem)
{
	return 0;
}

int PPViewUnitEc::GetGoodsListForProcessing(PPIDArray & rIdList)
{
	rIdList.Z();
	int    ok = -1;
	GoodsFilt goods_filt;
	Goods2Tbl::Rec goods_rec;
	PPIDArray child_idlist;
	if(Filt.GoodsGroupID)
		goods_filt.GrpIDList.Add(Filt.GoodsGroupID);
	goods_filt.Flags |= GoodsFilt::fWithStrucOnly;
	for(GoodsIterator gi(&goods_filt, 0); gi.Next(&goods_rec) > 0;) {
		if(!(goods_rec.Flags & GF_PASSIV)) {
			PPGoodsStrucHeader2 gsh;
			if(GsObj.Fetch(goods_rec.StrucID, &gsh) > 0) {
				if(gsh.Flags & GSF_PRICEPLANNING) {
					rIdList.add(goods_rec.ID);
				}
				else if(gsh.Flags & GSF_FOLDER) {
					child_idlist.Z();
					if(GsObj.GetChildIDList(gsh.ID, &child_idlist) > 0) {
						for(uint i = 0; i < child_idlist.getCount(); i++) {
							const PPID child_gs_id = child_idlist.get(i);
							if(GsObj.Fetch(child_gs_id, &gsh) > 0) {
								if(gsh.Flags & GSF_PRICEPLANNING) {
									rIdList.add(goods_rec.ID);
								}
							}
						}
					}
				}
			}
		}
		//PPWaitPercent(gi.GetIterCounter());
	}
	if(rIdList.getCount())
		ok = 1;
	return ok;
}

/*virtual*/int PPViewUnitEc::Init_(const PPBaseFilt * pBaseFilt)
{
	int    ok = -1;
	if(Helper_InitBaseFilt(pBaseFilt)) {
		IndicatorList.freeAll();
		Filt.Period.Actualize(ZERODATE);
		GetGoodsListForProcessing(GoodsIdList);
		ok = 1;
	}
	else
		ok = 0;
	return ok;
}

/*virtual*/int PPViewUnitEc::EditBaseFilt(PPBaseFilt * pBaseFilt)
{
	class UnitEcFiltDialog : public TDialog {
		DECL_DIALOG_DATA(UnitEcFilt);
	public:
		UnitEcFiltDialog() : TDialog(DLG_UNITECFILT)
		{
			SetupCalPeriod(CTLCAL_UNITECFILT_PERIOD, CTL_UNITECFILT_PERIOD);
		}
		DECL_DIALOG_SETDTS()
		{
			int    ok = 1;
			RVALUEPTR(Data, pData);
			SetPeriodInput(this, CTL_UNITECFILT_PERIOD, Data.Period);
			return ok;
		}
		DECL_DIALOG_GETDTS()
		{
			int    ok = 1;
			GetPeriodInput(this, CTL_UNITECFILT_PERIOD, &Data.Period);
			ASSIGN_PTR(pData, Data);
			return ok;
		}
	};
	if(!Filt.IsA(pBaseFilt))
		return 0;
	UnitEcFilt * p_filt = static_cast<UnitEcFilt *>(pBaseFilt);
	DIALOG_PROC_BODY(UnitEcFiltDialog, p_filt);
}

int PPViewUnitEc::MakeList(PPViewBrowser * pBrw)
{
	int    ok = -1;
	if(P_DsList)
		P_DsList->clear();
	else
		P_DsList = new SArray(sizeof(BrwItem));
	{
		
	}
	return ok;
}

void PPViewUnitEc::PreprocessBrowser(PPViewBrowser * pBrw)
{
	/*
		Здесь колонки добавлять будем
	*/ 
}

/*static*/int FASTCALL PPViewUnitEc::GetDataForBrowser(SBrowserDataProcBlock * pBlk)
{
	PPViewUnitEc * p_v = static_cast<PPViewUnitEc *>(pBlk->ExtraPtr);
	return p_v ? p_v->_GetDataForBrowser(pBlk) : 0;
}

int PPViewUnitEc::_GetDataForBrowser(SBrowserDataProcBlock * pBlk)
{
	int    ok = 0;
	if(pBlk->P_SrcData && pBlk->P_DestData) {
		switch(pBlk->ColumnN) {
			case 1: 
				break; // @stub
		}
	}
	return ok;
}

/*virtual*/SArray * PPViewUnitEc::CreateBrowserArray(uint * pBrwId, SString * pSubTitle)
{

	SArray * p_array = 0;
	THROW(MakeList(0));
	p_array = new SArray(*P_DsList);
	CATCH
		ZDELETE(p_array);
		ZDELETE(P_DsList);
	ENDCATCH
	ASSIGN_PTR(pBrwId, BROWSER_UNITEQ);
	return p_array;
}

/*virtual*/int PPViewUnitEc::OnExecBrowser(PPViewBrowser *)
{
	return -1; // @stub
}

/*virtual*/int PPViewUnitEc::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	if(ok == -2) {
		switch(ppvCmd) {
			case 0: break; // @stub
		}
	}
	return ok;
}
