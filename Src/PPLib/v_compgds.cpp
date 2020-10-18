// V_COMPGDS.CPP
// Copyright (c) V.Miller 2013, 2016, 2017, 2019, 2020
//
#include <pp.h>
#pragma hdrstop
//
// Фильтр по виду составной продукции и строке, содержащейся в названии
//
int SLAPI SuprWareFilt::InitInstance()
{
	SetFlatChunk(offsetof(SuprWareFilt, ReserveStart), offsetof(SuprWareFilt, SrchStr) - offsetof(SuprWareFilt, ReserveStart));
	SetBranchSVector(offsetof(SuprWareFilt, TypeIDList)); // @v9.8.4 SetBranchSArray-->SetBranchSVector
	SetBranchSString(offsetof(SuprWareFilt, SrchStr));
	return Init(1, 0);
}
//
// @vmiller
//
IMPLEMENT_PPFILT_FACTORY(SuprWare); SLAPI SuprWareFilt::SuprWareFilt() : PPBaseFilt(PPFILT_SUPRWARE, 0, 0)
{
	InitInstance();
}

/*virtual*/int SLAPI SuprWareFilt::Describe(long flags, SString & rBuf) const
{
	PutMembToBuf(SuprWareType, STRINGIZE(SuprWareType), rBuf);
	PutMembToBuf(SuprWareCat, STRINGIZE(SuprWareCat), rBuf);
	PutMembToBuf(SrchStr, STRINGIZE(SrchStr), rBuf);
	return 1;
}

int SLAPI SuprWareFilt::IsEmpty() const
{
	if(SuprWareType)
		return 0;
	if(SuprWareCat)
		return 0;
	if(SrchStr.NotEmpty())
		return 0;
	return 1;
}
//
//
//
SLAPI PPViewSuprWare::PPViewSuprWare() : PPView(&SwObj, &Filt, PPVIEW_SUPRWARE)
{
}

SLAPI PPViewSuprWare::~PPViewSuprWare()
{
}

int SLAPI PPViewSuprWare::EditBaseFilt(PPBaseFilt * pBaseFilt)
{
	class SuprWareFilterDlg : public TDialog {
		DECL_DIALOG_DATA(SuprWareFilt);
	public:
		SuprWareFilterDlg() : TDialog(DLG_FLTCOMPGDS)
		{
		}
		DECL_DIALOG_SETDTS()
		{
			RVALUEPTR(Data, pData);
			AddClusterAssocDef(CTL_FLTCOMPGDS_SWTYPE, 0, SUPRWARETYPE_GOODS);
			AddClusterAssoc(CTL_FLTCOMPGDS_SWTYPE, 1, SUPRWARETYPE_COMPONENT);
			SetClusterData(CTL_FLTCOMPGDS_SWTYPE, Data.SuprWareType);
			SetupStringCombo(this, CTLSEL_FLTCOMPGDS_SWCAT, PPTXT_COMPGDS_TYPES, Data.SuprWareCat);
			setCtrlString(CTL_FLTCOMPGDS_NAMESTR, Data.SrchStr.Strip());
			SetupCtrls();
			return 1;
		}
		DECL_DIALOG_GETDTS()
		{
			GetClusterData(CTL_FLTCOMPGDS_SWTYPE, &Data.SuprWareType);
			getCtrlData(CTLSEL_FLTCOMPGDS_SWCAT, &Data.SuprWareCat);
			getCtrlString(CTL_FLTCOMPGDS_NAMESTR, Data.SrchStr);
			ASSIGN_PTR(pData, Data);
			return 1;
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isCmd(cmClusterClk)) {
				SetupCtrls();
				clearEvent(event);
			}
		}
		void   SetupCtrls()
		{
			PPID   cat_id = 0;
			SString str_srch;
			getCtrlData(CTLSEL_FLTCOMPGDS_SWCAT, &cat_id);
			getCtrlString(CTL_FLTCOMPGDS_NAMESTR, str_srch);
			SetupStringCombo(this, CTLSEL_FLTCOMPGDS_SWCAT, PPTXT_COMPGDS_TYPES, cat_id);
			getCtrlData(CTLSEL_FLTCOMPGDS_SWCAT, &cat_id);
			setCtrlString(CTL_FLTCOMPGDS_NAMESTR, str_srch.Strip());
		}
	};
	if(Filt.IsA(pBaseFilt)) {
		DIALOG_PROC_BODYERR(SuprWareFilterDlg, static_cast<SuprWareFilt *>(pBaseFilt));
	}
	else
		return 0;
}

DBQuery * SLAPI PPViewSuprWare::CreateBrowserQuery(uint * pBrwId, SString * pSubTitle)
{
	DBQuery * q = 0;
	DBE    dbe_brand, dbe_group, dbe_strexst;
	DBConst dbc_substr;
	DBQ  * dbq = 0;
	uint   brw_id = BROWSER_SUPRWARE;
	Goods2Tbl * p_tbl = new Goods2Tbl;
	static DbqStringSubst type_subst(2);  // @global @threadsafe
    THROW(CheckTblPtr(p_tbl));
	PPDbqFuncPool::InitObjNameFunc(dbe_group, PPDbqFuncPool::IdObjNameGoods, p_tbl->ID);
	dbq = &(p_tbl->Kind == PPGDSK_SUPRWARE);
	//dbq = &(p_tbl->GoodsTypeID == COMPGDSTYPE_GOODS); // Нам нужны только товары, а не их составляющие
	dbq = ppcheckfiltid(dbq, p_tbl->GoodsTypeID, Filt.SuprWareType);
	dbq = ppcheckfiltid(dbq, p_tbl->WrOffGrpID, Filt.SuprWareCat);
	if(Filt.SrchStr.NotEmpty()) {
		dbc_substr.init(Filt.SrchStr);
		dbe_strexst.init();
		dbe_strexst.push(p_tbl->Name);
		dbe_strexst.push(dbc_substr);
		dbe_strexst.push(static_cast<DBFunc>(PPDbqFuncPool::IdStrExistSubStr));
		dbq = &(*dbq && dbe_strexst == 1L);
	}
	dbe_brand = enumtoa(p_tbl->WrOffGrpID, 2, type_subst.Get(PPTXT_COMPGDS_TYPES));
	q = & select(p_tbl->ID, 0L);
	q->addField(p_tbl->Name);
	q->addField(dbe_brand);
	q->from(p_tbl, 0L);
	q->where(*dbq).orderBy(p_tbl->Kind, p_tbl->Name, 0L);
	THROW(CheckQueryPtr(q));
	if(pSubTitle)
		PPGetSubStr(PPTXT_COMPGDS_TYPES, Filt.SuprWareType, *pSubTitle);
	CATCH
		if(q)
			ZDELETE(q);
		else
			delete p_tbl;
	ENDCATCH
	ASSIGN_PTR(pBrwId, brw_id);
	return q;
}

int SLAPI PPViewSuprWare::Init_(const PPBaseFilt * pFilt)
{
	BExtQuery::ZDelete(&P_IterQuery);
	return Helper_InitBaseFilt(pFilt);
}
//
//
//
int SLAPI PPViewSuprWare::InitIteration()
{
	Goods2Tbl * _t = SwObj.P_Tbl;
	DBQ * dbq = 0;
	BExtQuery::ZDelete(&P_IterQuery);
	P_IterQuery = new BExtQuery(_t, 1);
	dbq = &(_t->Kind == PPGDSK_SUPRWARE);
	dbq = ppcheckfiltid(dbq, _t->GoodsTypeID, Filt.SuprWareType);
	dbq = ppcheckfiltid(dbq, _t->WrOffGrpID, Filt.SuprWareCat);
	P_IterQuery->select(_t->ID, _t->Name, 0L).where(*dbq);
	P_IterQuery->initIteration(0, 0, spFirst);
	return 1;
}

int FASTCALL PPViewSuprWare::NextIteration(SuprWareViewItem * pItem)
{
	int    ok = -1;
	if(P_IterQuery) {
		while(ok < 0 && P_IterQuery->nextIteration() > 0) {
			Goods2Tbl::Rec rec;
			SwObj.P_Tbl->copyBufTo(&rec);
			if(Filt.SrchStr.Empty() || ExtStrSrch(rec.Name, Filt.SrchStr, 0)) {
				PPSuprWarePacket sw_pack;
				if(SwObj.Get(rec.ID, &sw_pack) > 0) {
					ASSIGN_PTR(pItem, sw_pack.Rec);
					ok = 1;
				}
			}
			Counter.Increment();
		}
	}
	return ok;
}

int SLAPI PPViewSuprWare::ViewTotal()
{
	TDialog * dlg = new TDialog(DLG_GOODSTOTAL);
	if(CheckDialogPtrErr(&dlg)) {
		long   count = 0;
		SuprWareViewItem item;
		PPWait(1);
		for(InitIteration(); NextIteration(&item) > 0; PPWaitPercent(GetCounter())) {
			count++;
		}
		PPWait(0);
		dlg->setCtrlLong(CTL_GOODSTOTAL_COUNT, count);
		ExecViewAndDestroy(dlg);
		return -1;
	}
	else
		return 0;
}
//
//
//
int SLAPI PPViewSuprWare::DeleteAll()
{
	int    ok = -1;
	THROW(SwObj.CheckRights(PPR_DEL));
	if(CONFIRMCRIT(PPCFM_DELALLSUPRWARE)) {
		PPIDArray id_list;
		SuprWareViewItem item;
		PPWait(1);
		for(InitIteration(); NextIteration(&item) > 0;) {
			id_list.addUnique(item.ID);
		}
		const uint cnt = id_list.getCount();
		if(cnt) {
			PPLogger logger;
			PPTransaction tra(1);
			THROW(tra);
			for(uint i = 0; i < cnt; i++) {
				PPID _id = id_list.get(i);
				if(_id) {
					if(!SwObj.Put(&_id, 0, 0)) {
						logger.LogLastError();
					}
					else
						ok = 1;
				}
				PPWaitPercent(i+1, cnt);
			}
			THROW(tra.Commit());
		}
		PPWait(0);
	}
	CATCHZOKPPERR
	return ok;
}

/*virtual*/int SLAPI PPViewSuprWare::Detail(const void * pHdr, PPViewBrowser * pBrw)
{
	PPID   id = pHdr ? *static_cast<const PPID *>(pHdr) : 0;
	return (id && SwObj.EditList(&id) == cmOK) ? 1 : -1;
}

int SLAPI PPViewSuprWare::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	if(ok == -2) {
		PPID   id = pHdr ? *static_cast<const PPID *>(pHdr) : 0;
		switch(ppvCmd) {
			// На самом деле никакого редактирования не происходит. Только просмотр информации о составе товара
#if 0 // {
			case PPVCMD_EDITITEM:
				ok = (SwObj.EditList(&id) == cmOK) ? 1 : -1;
				break;
			case PPVCMD_DELETEITEM:
				ok = -1;
				if(id > 0 && CONFIRM(PPCFM_DELETE)) {
					ok = SwObj.Put(&id, 0, 1) ? 1 : PPErrorZ();
				}
				break;
#endif // } 0
			case PPVCMD_DELETEALL:
				ok = DeleteAll();
				break;
		}
	}
	return ok;
}
