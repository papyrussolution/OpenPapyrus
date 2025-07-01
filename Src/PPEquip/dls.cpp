// DLS.CPP
// Copyright (c) A.Sobolev 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2024, 2025
//
#include <pp.h>
#pragma hdrstop

struct _GoodsInfo { // @flat
	PPID   ID;
	long   PLU;
	double Price;
};

DeviceLoadingStat::DeviceLoadingStat() : DvcLoadingStatTbl(), StatID(0), GoodsList(sizeof(_GoodsInfo)), StatCache(sizeof(DvcLoadingStatTbl::Rec))
{
}

int DeviceLoadingStat::Fetch(PPID statID, DvcLoadingStatTbl::Rec * pRec)
{
	int    ok = -1;
	uint   pos = 0;
	if(StatCache.lsearch(&statID, &pos, CMPF_LONG)) {
		ASSIGN_PTR(pRec, *static_cast<const DvcLoadingStatTbl::Rec *>(StatCache.at(pos)));
		ok = 1;
	}
	else {
		DvcLoadingStatTbl::Rec rec;
		if(Search(statID, &rec) > 0) {
			THROW_SL(StatCache.insert(&rec));
			ASSIGN_PTR(pRec, rec);
			ok = 1;
		}
	}
	CATCHZOK
	return ok;
}

int DeviceLoadingStat::Search(PPID statID, DvcLoadingStatTbl::Rec * pRec) { return SearchByID(this, 0, statID, pRec); }

int DeviceLoadingStat::GetPrev(PPID curStatID, PPID * pStatID, DvcLoadingStatTbl::Rec * pRec)
{
	DvcLoadingStatTbl::Rec cur_rec;
	if(SETIFZ(curStatID, StatID) && Search(curStatID, &cur_rec) > 0) {
		LDATETIME dtm;
		dtm.Set(cur_rec.Dt, cur_rec.Tm);
		return GetLast(cur_rec.DvcType, cur_rec.DvcID, dtm, pStatID, pRec);
	}
	else
		return 0;
}

int DeviceLoadingStat::GetLast(int dvcType, PPID dvcID, const LDATETIME & rDtm, PPID * pStatID, DvcLoadingStatTbl::Rec * pRec)
{
	DvcLoadingStatTbl::Key1 k1;
	MEMSZERO(k1);
	k1.DvcType = dvcType;
	k1.DvcID = dvcID;
	k1.Dt = NZOR(rDtm.d, MAXDATE);
	k1.Tm = rDtm.d ? rDtm.t : MAXTIME;
	while(search(1, &k1, spLt) && k1.DvcType == dvcType && k1.DvcID == dvcID) {
		if(data.Status) {
			ASSIGN_PTR(pStatID, data.ID);
			copyBufTo(pRec);
			return 1;
		}
	}
	return PPDbSearchError();
}

int DeviceLoadingStat::GetLastObjInfo(PPID objType, PPID objID, LDATE dt, DlsObjTbl::Rec * pRec)
{
	DvcLoadingStatTbl::Rec rec;
	return (Fetch(StatID, &rec) > 0) ? GetLastObjInfo(rec.DvcType, rec.DvcID, objType, objID, dt, pRec) : 0;
}

int DeviceLoadingStat::GetLastObjInfo(int dvcType, PPID dvcID, PPID objType, PPID objID, LDATE dt, DlsObjTbl::Rec * pRec)
{
	DlsObjTbl::Key0 k0;
	MEMSZERO(k0);
	k0.ObjType = static_cast<int16>(objType);
	k0.ObjID = objID;
	k0.DlsID = MAXLONG;
	while(DlsoT.search(0, &k0, spLt) && k0.ObjType == objType && k0.ObjID == objID) {
		DvcLoadingStatTbl::Rec stat_rec;
		if(Fetch(DlsoT.data.DlsID, &stat_rec) > 0 && stat_rec.DvcType == dvcType &&
			stat_rec.DvcID == dvcID && stat_rec.Status && (!dt || stat_rec.Dt <= dt)) {
			DlsoT.copyBufTo(pRec);
			return 1;
		}
	}
	return -1;
}

int DeviceLoadingStat::StartLoading(PPID * pStatID, int deviceType, PPID deviceID, int use_ta)
{
	int    ok = 1;
	PPID   id = 0;
	DvcLoadingStatTbl::Rec rec;
	rec.DvcType = deviceType;
	rec.DvcID   = deviceID;
	getcurdatetime(&rec.Dt, &rec.Tm);
	THROW(AddByID(this, &id, &rec, use_ta));
	StatID = id;
	StartClock = clock();
	CATCH
		ok = 0;
		id = 0;
	ENDCATCH
	ASSIGN_PTR(pStatID, id);
	return ok;
}

int DeviceLoadingStat::RegisterBillList(PPID statID, const PPIDArray & rBillList)
{
	int    ok = 1;
	THROW_INVARG(statID == StatID);
	THROW_SL(UpdatedBillList.add(&rBillList));
	UpdatedBillList.sortAndUndup();
	CATCHZOK
	return ok;
}

int DeviceLoadingStat::RegisterGoods(PPID statID, const GoodsInfo * pInfo)
{
	int    ok = 1;
	uint   pos = 0;
	THROW_INVARG(statID == StatID);
	if(!GoodsList.lsearch(&pInfo->ID, &pos, CMPF_LONG)) {
		_GoodsInfo  gds_info;
		gds_info.ID    = pInfo->ID;
		gds_info.PLU   = pInfo->PLU;
		gds_info.Price = pInfo->Price;
		THROW_SL(GoodsList.insert(&gds_info));
	}
	CATCHZOK
	return ok;
}

int DeviceLoadingStat::RegisterSCard(PPID statID, const SCardInfo * pInfo)
{
	int    ok = 1;
	THROW_INVARG(statID == StatID);
	if(!SCardList.Search(pInfo->ID, 0, 0))
		THROW_SL(SCardList.Add(pInfo->ID, pInfo->Discount, 0));
	CATCHZOK
	return ok;
}

int DeviceLoadingStat::CutTables(short dvcType, int use_ta)
{
	int    ok = -1, removed_count = 0;
	DBMaintainParam  db_param;
	if(ReadDBMaintainCfg(&db_param) > 0 && db_param.DLSDays) {
		LDATE  to_dt = plusdate(getcurdate_(), -db_param.DLSDays);
		DvcLoadingStatTbl::Key2 k2;
		MEMSZERO(k2);
		while(removed_count < 2 && search(2, &k2, spGt) && k2.Dt <= to_dt)
			if(k2.DvcType == dvcType) {
				THROW(Remove(data.ID, use_ta));
				removed_count++;
				ok = 1;
			}
	}
	CATCHZOK
	return ok;
}

int DeviceLoadingStat::FinishLoading(PPID statID, int status, int use_ta)
{
	int    ok = 1;
	DvcLoadingStatTbl::Rec rec;
	const long timing = clock() - StartClock;
	THROW_INVARG(statID == StatID);
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(SearchByID(this, 0, statID, &rec) > 0);
		THROW(CutTables(rec.DvcType, 0));
		rec.Cont = timing;
		rec.Status = status;
		THROW(UpdateByID(this, 0, statID, &rec, 0));
		{
			BExtInsert bei(&DlsoT);
			if(GoodsList.getCount()) {
				for(uint i = 0; i < GoodsList.getCount(); i++) {
					const _GoodsInfo & gds_info = *static_cast<const _GoodsInfo *>(GoodsList.at(i));
					DlsObjTbl::Rec dlso_rec;
					dlso_rec.DlsID   = statID;
					dlso_rec.ObjType = PPOBJ_GOODS;
					dlso_rec.ObjID   = gds_info.ID;
					dlso_rec.LVal    = gds_info.PLU;
					dlso_rec.Val     = gds_info.Price;
					THROW_DB(bei.insert(&dlso_rec));
				}
			}
			if(SCardList.getCount()) {
				for(uint i = 0; i < SCardList.getCount(); i++) {
					DlsObjTbl::Rec dlso_rec;
					dlso_rec.DlsID   = statID;
					dlso_rec.ObjType = PPOBJ_SCARD;
					dlso_rec.ObjID   = SCardList.at(i).Key;
					dlso_rec.Val     = SCardList.at(i).Val;
					THROW_DB(bei.insert(&dlso_rec));
				}
			}
			if(UpdatedBillList.getCount()) {
				UpdatedBillList.sortAndUndup();
				for(uint i = 0; i < UpdatedBillList.getCount(); i++) {
					DlsObjTbl::Rec dlso_rec;
					dlso_rec.DlsID   = statID;
					dlso_rec.ObjType = PPOBJ_BILL;
					dlso_rec.ObjID   = UpdatedBillList.get(i);
					THROW_DB(bei.insert(&dlso_rec));
				}
			}
			THROW_DB(bei.flash());
		}
		StatID = 0;
		StartClock = 0;
		THROW(tra.Commit());
	}
	CATCH
		ok = 0;
		PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR_TIME_USER|LOGMSGF_DBINFO);
	ENDCATCH
	return ok;
}

int DeviceLoadingStat::GetExportedItems(PPID statID, PPID objType, TSVector <DlsObjTbl::Rec> & rList)
{
	int    ok = -1;
	rList.clear();
    DvcLoadingStatTbl::Rec rec;
    if(Search(statID, &rec) > 0) {
		DlsObjTbl::Key1 k1;
		MEMSZERO(k1);
		k1.DlsID = rec.ID;
		k1.ObjType = static_cast<int16>(objType);
		BExtQuery q(&DlsoT, 1);
		q.selectAll().where(DlsoT.DlsID == statID && DlsoT.ObjType == objType);
		for(q.initIteration(false, &k1, spGe); q.nextIteration() > 0;) {
			//temp_obj_list.add(DlsoT.data.ObjID);
			THROW_SL(rList.insert(&DlsoT.data));
		}
		if(rList.getCount())
			ok = 1;
	}
	CATCHZOK
	return ok;
}

int DeviceLoadingStat::GetExportedObjectsSince(PPID objType, PPID sinceDlsID, PPIDArray * pObjList)
{
    int    ok = 1;
    PPIDArray temp_obj_list;
    PPIDArray ev_list;
    DvcLoadingStatTbl::Rec rec;
    if(Search(sinceDlsID, &rec) > 0) {
		ev_list.add(rec.ID);
		DvcLoadingStatTbl::Key1 k1;
		MEMSZERO(k1);
		k1.DvcType = rec.DvcType;
		k1.DvcID = rec.DvcID;
		k1.Dt = rec.Dt;
		k1.Tm = rec.Tm;
		while(search(1, &k1, spGt) && data.DvcType == rec.DvcType && data.DvcID == rec.DvcID) {
			ev_list.add(data.ID);
		}
		ev_list.sortAndUndup();
    }
    for(uint i = 0; i < ev_list.getCount(); i++) {
		const  PPID _id = ev_list.get(i);
		DlsObjTbl::Key1 k1;
		MEMSZERO(k1);
		k1.DlsID = rec.ID;
		k1.ObjType = static_cast<int16>(objType);
		BExtQuery q(&DlsoT, 1);
		q.select(DlsoT.ObjID, 0).where(DlsoT.DlsID == _id && DlsoT.ObjType == objType);
		for(q.initIteration(false, &k1, spGe); q.nextIteration() > 0;) {
			temp_obj_list.add(DlsoT.data.ObjID);
		}
    }
    if(!temp_obj_list.getCount())
		ok = -1;
	else
		temp_obj_list.sortAndUndup();
    ASSIGN_PTR(pObjList, temp_obj_list);
    return ok;
}

int DeviceLoadingStat::GetBillList(PPID statID, PPIDArray & rList)
{
	rList.clear();
	int    ok = -1;
	DlsObjTbl::Key1 k1;
	MEMSZERO(k1);
	k1.DlsID = statID;
	k1.ObjType = PPOBJ_BILL;
	if(DlsoT.search(1, &k1, spGe) && DlsoT.data.DlsID == statID && DlsoT.data.ObjType == PPOBJ_BILL) do {
		rList.add(DlsoT.data.ObjID);
		ok = 1;
	} while(DlsoT.search(1, &k1, spNext) && DlsoT.data.DlsID == statID && DlsoT.data.ObjType == PPOBJ_BILL);
	if(ok > 0) {
		assert(rList.getCount() != 0);
		rList.sortAndUndup();
	}
	return ok;
}

int DeviceLoadingStat::GetUpdatedObjects(PPID objType, const LDATETIME & since, PPIDArray * pObjList)
{
	int    ok = 1;
	PPIDArray acn_list;
	SysJournal * p_sj = DS.GetTLA().P_SysJ;
	SysJournalTbl::Key0 k;
	k.Dt = since.d;
	k.Tm = since.t;
	BExtQuery q(p_sj, 0);
	q.selectAll().where(p_sj->ObjType == objType);
	acn_list.addzlist(PPACN_OBJADD, PPACN_OBJUPD, PPACN_OBJUNIFY, 0L);
	if(objType == PPOBJ_GOODS)
		acn_list.addzlist(PPACN_GOODSQUOTUPD, PPACN_QUOTUPD2, PPACN_GOODSNODISRMVD, 0L);
	else if(objType == PPOBJ_SCARD)
		acn_list.addzlist(PPACN_SCARDDISUPD, PPACN_SCARDOWNERUPDATED, 0); // @v11.4.0 PPACN_SCARDOWNERUPDATED
	acn_list.sort();
	for(q.initIteration(false, &k, spGe); q.nextIteration() > 0;) {
		if(cmp(since, p_sj->data.Dt, p_sj->data.Tm) < 0 && acn_list.bsearch(p_sj->data.Action))
			if(pObjList)
				THROW(pObjList->add(p_sj->data.ObjID));
	}
	CALLPTRMEMB(pObjList, sortAndUndup());
	CATCHZOK
	return ok;
}

int DeviceLoadingStat::DoMaintain(LDATE toDt)
{
	int    ok = 1;
	long   total = 0;
	SString msg;
	IterCounter counter;
	DvcLoadingStatTbl::Key2 k;
	{
		SString buf, added_param;
		SFsPath ps(DBTable::GetName());
		ps.Merge(0, SFsPath::fDrv|SFsPath::fDir, added_param);
		msg.Printf(PPLoadTextS(PPTXT_DBMAINTAIN, buf), added_param);
	}
	k.Dt      = toDt;
	k.Tm.v    = MAXLONG;
	k.DvcType = MAXSHORT;
	k.DvcID   = MAXLONG;
	if(search(2, &k, spLe) && k.Dt <= toDt) {
		total++;
		while(search(2, &k, spPrev) && k.Dt <= toDt)
			total++;
	}
	counter.Init(total);
	k.Dt      = toDt;
	k.Tm.v    = MAXLONG;
	k.DvcType = MAXSHORT;
	k.DvcID   = MAXLONG;
	while(search(2, &k, spLe) && k.Dt <= toDt) {
		THROW(Remove(data.ID, 1));
		PPWaitPercent(counter.Increment(), msg);
	}
	CATCHZOK
	return ok;
}

int DeviceLoadingStat::Remove(PPID id, int useTa)
{
	int    ok = 1;
	{
		PPTransaction tra(useTa);
		THROW(tra);
		THROW_DB(deleteFrom(&DlsoT, 0, (DlsoT.DlsID == id)));
		THROW_DB(deleteFrom(this, 0, (this->ID == id)));
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}
//
// @ModuleDef(PPViewDvcLoadingStat)
//
IMPLEMENT_PPFILT_FACTORY(DvcLoadingStat); DvcLoadingStatFilt::DvcLoadingStatFilt() : PPBaseFilt(PPFILT_DVCLOADINGSTAT, 0, 0)
{
	SetFlatChunk(offsetof(DvcLoadingStatFilt, ReserveStart),
		offsetof(DvcLoadingStatFilt, Reserve)-offsetof(DvcLoadingStatFilt, ReserveStart)+sizeof(Reserve));
	Init(1, 0);
}
//
//
//
class DLSFiltDialog : public TDialog {
public:
	enum {
		ctlgroupGoods = 1
	};
	DLSFiltDialog() : TDialog(DLG_DLSFLT), LastDvcTypeSel(0)
	{
		addGroup(ctlgroupGoods, new GoodsCtrlGroup(CTLSEL_DLSFLT_GGRP, CTLSEL_DLSFLT_GOODS));
	}
	int    setDTS(const DvcLoadingStatFilt *);
	int    getDTS(DvcLoadingStatFilt *);
private:
	void    GetDeviceData(PPID * pDvcType, PPID * pDvcID);
	void    ReplyOnDvcTypeSel(PPID dvcType, PPID dvcID);
	DECL_HANDLE_EVENT;
	DvcLoadingStatFilt Data;
	int     LastDvcTypeSel;
};

void DLSFiltDialog::GetDeviceData(PPID * pDvcType, PPID * pDvcID)
{
	PPID   dvc_type = 0, dvc_id = 0;
	getCtrlData(CTLSEL_DLSFLT_DVCTYPE, &dvc_type);
	if(dvc_type)
		getCtrlData(CTLSEL_DLSFLT_DVCNAME, &dvc_id);
	ASSIGN_PTR(pDvcType, dvc_type);
	ASSIGN_PTR(pDvcID, dvc_id);
}

void DLSFiltDialog::ReplyOnDvcTypeSel(PPID dvcType, PPID dvcID)
{
	PPID   dvc_id = (!LastDvcTypeSel || dvcType == LastDvcTypeSel) ? dvcID : 0;
	if(dvcType)
		SetupPPObjCombo(this, CTLSEL_DLSFLT_DVCNAME, (dvcType == dvctCashs) ? PPOBJ_CASHNODE : PPOBJ_SCALE, dvc_id, 0);
	else {
		SetComboBoxLinkText(this, CTLSEL_DLSFLT_DVCNAME, 0);
	}
	LastDvcTypeSel = dvcType;
	disableCtrl(CTLSEL_DLSFLT_DVCNAME, !dvcType);
}

int DLSFiltDialog::setDTS(const DvcLoadingStatFilt * pData)
{
	if(!RVALUEPTR(Data, pData))
		Data.Init(1, 0);
	SetupStringCombo(this, CTLSEL_DLSFLT_DVCTYPE, PPTXT_DEVICE_TYPES, Data.DvcType);
	ReplyOnDvcTypeSel(Data.DvcType, Data.DvcID);
	SetupCalPeriod(CTLCAL_DLSFLT_PERIOD, CTL_DLSFLT_PERIOD);
	SetPeriodInput(this, CTL_DLSFLT_PERIOD, Data.Period);
	{
		GoodsCtrlGroup::Rec ggrp_rec(Data.GoodsGrpID, Data.GoodsID, 0, GoodsCtrlGroup::enableSelUpLevel);
		setGroupData(ctlgroupGoods, &ggrp_rec);
	}
	return 1;
}

int DLSFiltDialog::getDTS(DvcLoadingStatFilt * pData)
{
	int    ok = 1;
	uint   sel = 0;
	PPID   dvc_type, dvc_id;
	GetDeviceData(&dvc_type, &dvc_id);
	Data.DvcType = static_cast<short>(dvc_type);
	Data.DvcID   = dvc_id;
	sel = CTL_DLSFLT_DVCTYPE;
	THROW_PP(Data.DvcType, PPERR_DVCTYPENEEDED);
	sel = CTL_DLSFLT_PERIOD;
	THROW(GetPeriodInput(this, CTL_DLSFLT_PERIOD, &Data.Period));
	{
		GoodsCtrlGroup::Rec ggrp_rec;
		getGroupData(ctlgroupGoods, &ggrp_rec);
		Data.GoodsGrpID = ggrp_rec.GoodsGrpID;
		Data.GoodsID    = ggrp_rec.GoodsID;
	}
	ASSIGN_PTR(pData, Data);
	CATCHZOKPPERRBYDLG
	return ok;
}

IMPL_HANDLE_EVENT(DLSFiltDialog)
{
	TDialog::handleEvent(event);
	if(event.isCbSelected(CTLSEL_DLSFLT_DVCTYPE)) {
		PPID   dvc_type, dvc_id;
		GetDeviceData(&dvc_type, &dvc_id);
		ReplyOnDvcTypeSel(dvc_type, dvc_id);
		clearEvent(event);
	}
}
//
//
//
int PPViewDvcLoadingStat::EditBaseFilt(PPBaseFilt * pBaseFilt)
{
	if(!Filt.IsA(pBaseFilt))
		return 0;
	DvcLoadingStatFilt * p_filt = static_cast<DvcLoadingStatFilt *>(pBaseFilt);
	DIALOG_PROC_BODY(DLSFiltDialog, p_filt);
}

PPViewDvcLoadingStat::PPViewDvcLoadingStat() : PPView(0, &Filt, PPVIEW_DVCLOADINGSTAT, 0, 0)
{
}

PPViewDvcLoadingStat::~PPViewDvcLoadingStat()
{
}

int PPViewDvcLoadingStat::Init_(const PPBaseFilt * pBaseFilt)
{
	int    ok = 1;
	StrAssocArray * p_ggrp_list = 0;
	THROW(Helper_InitBaseFilt(pBaseFilt));
	GGrpList.freeAll();
	if(Filt.GoodsGrpID) {
		PPObjGoodsGroup ggrp_obj;
		THROW(p_ggrp_list = ggrp_obj.MakeStrAssocList(0));
		p_ggrp_list->GetListByParent(Filt.GoodsGrpID, 0, GGrpList);
		GGrpList.addUnique(Filt.GoodsGrpID);
	}
	CATCHZOK
	delete p_ggrp_list;
	return ok;
}

int PPViewDvcLoadingStat::InitIteration()
{
	int    ok = 1;
	DvcLoadingStatTbl * p_t = &DlsT;
	DvcLoadingStatTbl::Key1 k1, k1_;
	BExtQuery::ZDelete(&P_IterQuery);
	Counter.Init();
	THROW_MEM(P_IterQuery = new BExtQuery(p_t, 1));
	P_IterQuery->select(p_t->ID, p_t->DvcType, p_t->DvcID, p_t->Dt, p_t->Tm, p_t->Cont, p_t->Status, 0L);
	MEMSZERO(k1);
	k1_ = k1;
	Counter.Init(P_IterQuery->countIterations(0, &k1_, spFirst));
	P_IterQuery->initIteration(false, &k1, spFirst);
	CATCHZOK
	return ok;
}

int FASTCALL PPViewDvcLoadingStat::NextIteration(DvcLoadingStatViewItem * pItem)
{
	int    ok = -1;
	memzero(pItem, sizeof(*pItem));
	if(P_IterQuery && P_IterQuery->nextIteration() > 0) {
		ASSIGN_PTR(pItem, DlsT.data);
		Counter.Increment();
		ok = 1;
	}
	return ok;
}

static int GetDvcName(int dvcType, SString *pSs)
{
	int    ok = -1;
	char   dvc_name[64];
	if(dvcType > 0 && PPGetSubStr(PPTXT_DEVICE_TYPES, dvcType - 1 , dvc_name, sizeof(dvc_name)) > 0) {
		char * p = sstrchr(dvc_name, ',');
		if(p) {
			pSs->Cat(++p);
			ok = 1;
		}
	}
	return ok;
}

DBQuery * PPViewDvcLoadingStat::CreateBrowserQuery(uint * pBrwId, SString * pSubTitle)
{
	uint   brw_id = BROWSER_DLSTAT;
	long   dvc_obj_type = (Filt.DvcType == dvctCashs) ? PPOBJ_CASHNODE : PPOBJ_SCALE;
	DBQuery * q = 0;
	DBQ  * dbq = 0;
	DBE    dbe_dvc;
	DBE    dbe_goods;
	DvcLoadingStatTbl * p_tbl = 0;
	Goods2Tbl * p_g_tbl = 0;
	DlsObjTbl * p_dlso_tbl = 0;
	DBFieldList fld_list;
	THROW(CheckTblPtr(p_tbl = new DvcLoadingStatTbl(DlsT.GetName())));
	if(Filt.DvcID && pSubTitle) {
		pSubTitle->CatChar(0xa0);
		pSubTitle->Colon();
		pSubTitle->Space();
		GetDvcName(Filt.DvcType, pSubTitle);
		pSubTitle->Space();
		CatObjectName(dvc_obj_type, Filt.DvcID, *pSubTitle);
	}
	if(Filt.DvcType == dvctCashs) {
		PPDbqFuncPool::InitObjNameFunc(dbe_dvc, PPDbqFuncPool::IdObjNameCashNode, p_tbl->DvcID);
	}
	else {
		PPDbqFuncPool::InitObjNameFunc(dbe_dvc, PPDbqFuncPool::IdObjNameScale, p_tbl->DvcID);
	}
	if(Filt.GoodsID || Filt.GoodsGrpID) {
		THROW(p_dlso_tbl = new DlsObjTbl);
		if(Filt.GoodsGrpID)
			THROW(p_g_tbl = new Goods2Tbl);
		PPDbqFuncPool::InitObjNameFunc(dbe_goods, PPDbqFuncPool::IdObjNameGoods, p_dlso_tbl->ObjID);
		q = &select(
			p_tbl->ID,           // #0
			p_tbl->DvcType,      // #1
			p_tbl->DvcID,        // #2
			p_tbl->Dt,           // #3
			p_tbl->Tm,           // #4
			p_tbl->Cont,         // #5
			p_tbl->Status,       // #6
			dbe_dvc,           // #7
			dbe_goods,         // #8
			p_dlso_tbl->Val,   // #9
			0).from(p_tbl, p_dlso_tbl, (Filt.GoodsGrpID) ? p_g_tbl : 0, 0);
	}
	else
		q = &select(
			p_tbl->ID,         // #0
			p_tbl->DvcType,    // #1
			p_tbl->DvcID,      // #2
			p_tbl->Dt,         // #3
			p_tbl->Tm,         // #4
			p_tbl->Cont,       // #5
			p_tbl->Status,     // #6
			dbe_dvc,         // #7
			0).from(p_tbl, 0L);
	dbq = & (p_tbl->DvcType == static_cast<long>(Filt.DvcType));
	if(Filt.DvcID) {
		dbq = & (*dbq && (p_tbl->DvcID == Filt.DvcID));
	}
	dbq = & (*dbq && daterange(p_tbl->Dt, &Filt.Period));
	if(Filt.GoodsID || Filt.GoodsGrpID) {
		dbq = & (* dbq && p_dlso_tbl->DlsID == p_tbl->ID && p_dlso_tbl->ObjType == PPOBJ_GOODS);
		if(Filt.GoodsID)
			dbq = & (*dbq && p_dlso_tbl->ObjID == Filt.GoodsID);
		else {
			dbq = & (*dbq && p_dlso_tbl->ObjID == p_g_tbl->ID);
			dbq = ppcheckfiltidlist(dbq, p_g_tbl->ParentID, &GGrpList);
		}
	}
	q->where(*dbq).orderBy(p_tbl->Dt, p_tbl->Tm, 0L);
	THROW(CheckQueryPtr(q));
	CATCH
		ZDELETE(q);
	ENDCATCH
	ASSIGN_PTR(pBrwId, brw_id);
	return q;
}

void PPViewDvcLoadingStat::PreprocessBrowser(PPViewBrowser * pBrw)
{
	if(pBrw && !Filt.DvcID) {
		SString  dvc_name;
		GetDvcName(Filt.DvcType, &dvc_name);
		pBrw->insertColumn(0, dvc_name, 7, 0L, MKSFMTD(20, 0, 0), 0);
	}
	if(Filt.GoodsGrpID || Filt.GoodsID) {
		pBrw->insertColumn(2, "@ware", 8, 0L, MKSFMTD(20, 0, 0), 0);
		pBrw->insertColumn(3, "@price", 9, 0L, SFMT_MONEY, 0);
	}
}

int PPViewDvcLoadingStat::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	if(ok == -2) {
		BrwHdr hdr;
		if(pHdr)
			hdr = *static_cast<const PPViewDvcLoadingStat::BrwHdr *>(pHdr);
		else
			MEMSZERO(hdr);
		if(oneof2(ppvCmd, PPVCMD_EDITGOODS, PPVCMD_VIEWBILLS) || (hdr.DvcType == dvctCashs && ppvCmd == PPVCMD_SCARDS)) {
			DLSDetailFilt  dlsd_flt;
			dlsd_flt.DlsID   = hdr.DlsID;
			dlsd_flt.DvcType = hdr.DvcType;
			dlsd_flt.DvcID   = hdr.DvcID;
			if(ppvCmd == PPVCMD_VIEWBILLS) {
				dlsd_flt.ObjType = PPOBJ_BILL;
				if(DlsT.GetBillList(hdr.DlsID, dlsd_flt.BillList) > 0)
					ViewDLSDetail(dlsd_flt);
			}
			else {
				dlsd_flt.ObjType = (ppvCmd == PPVCMD_EDITGOODS) ? PPOBJ_GOODS : PPOBJ_SCARD;
				ViewDLSDetail(dlsd_flt);
			}
			ok = -1;
		}
		else if(ppvCmd == PPVCMD_EDITDEVICE) {
			ok = -1;
			if(hdr.DvcID) {
				if(hdr.DvcType == dvctCashs) {
					PPObjCashNode cn_obj;
					if(cn_obj.Edit(&hdr.DvcID, 0) > 0)
						ok = 1;
				}
				else if(hdr.DvcType == dvctScales) {
					PPObjScale sc_obj;
					if(sc_obj.Edit(&hdr.DvcID, 0) > 0)
						ok = 1;
				}
			}
		}
		else if(ppvCmd == PPVCMD_PUTTOBASKET) { // @v10.4.10
			ok = -1;
			if(hdr.DlsID) {
				TSVector <DlsObjTbl::Rec> list;
				if(DlsT.GetExportedItems(hdr.DlsID, PPOBJ_GOODS, list) > 0) {
					SelBasketParam param;
					param.SelPrice = 3;
					int    r = GetBasketByDialog(&param, GetSymb());
					if(r > 0) {
						for(uint i = 0; i < list.getCount(); i++) {
							const DlsObjTbl::Rec & r_item = list.at(i);
							ILTI   i_i;
							i_i.GoodsID  = labs(r_item.ObjID);
							i_i.Price    = r_item.Val;
							i_i.CurPrice = 0.0;
							i_i.Flags    = 0;
							i_i.Quantity = 1.0;
							param.Pack.AddItem(&i_i, 0, param.SelReplace);
						}
						if(param.Pack.Lots.getCount()) {
							GoodsBasketDialog(param, 1);
						}
					}
				}
			}
		}
		else if(oneof2(ppvCmd, PPVCMD_EXPORT, PPVCMD_EXPORTREDOSINCE)) {
			ok = -1;
			if(hdr.DlsID) {
				DvcLoadingStatTbl::Rec rec;
				if(DlsT.Search(hdr.DlsID, &rec) > 0) {
					if(rec.DvcType == dvctCashs && rec.DvcID) {
						PPCashMachine * p_cm = PPCashMachine::CreateInstance(rec.DvcID);
						if(!p_cm || !p_cm->AsyncOpenSession((ppvCmd == PPVCMD_EXPORTREDOSINCE) ? 2 : 1, rec.ID))
							PPError();
						delete p_cm;
					}
				}
			}
		}
	}
	return ok;
}
//
// @ModuleDef(PPViewDLSDetail)
//
DLSDetailFilt::DLSDetailFilt() : DlsID(0), DvcType(0), Reserve(0), DvcID(0), ObjType(0)
{
}

PPViewDLSDetail::PPViewDLSDetail() : PPView(0, 0, 0, 0, 0), P_DlsObjTbl(0)
{
}

PPViewDLSDetail::~PPViewDLSDetail()
{
	delete P_DlsObjTbl;
}

int PPViewDLSDetail::Init(const DLSDetailFilt * pFilt)
{
	Filt = *pFilt;
	return CheckTblPtr(P_DlsObjTbl = new DlsObjTbl);
}

int PPViewDLSDetail::InitIteration()
{
	int    ok = 1;
	DlsObjTbl * p_t = P_DlsObjTbl;
	if(p_t) {
		int    idx = 0;
		union {
			DlsObjTbl::Key0 k0;
			DlsObjTbl::Key1 k1;
		} k, k_;
		BExtQuery::ZDelete(&P_IterQuery);
		Counter.Init();
		MEMSZERO(k);
		if(Filt.DlsID) {
			idx = 1;
			k.k1.DlsID = Filt.DlsID;
			k.k1.ObjType = static_cast<int16>(Filt.ObjType);
		}
		else {
			idx = 0;
			k.k0.ObjType = static_cast<int16>(Filt.ObjType);
		}
		THROW_MEM(P_IterQuery = new BExtQuery(p_t, idx));
		P_IterQuery->selectAll().where(p_t->DlsID == Filt.DlsID && p_t->ObjType == Filt.ObjType);
		k_ = k;
		Counter.Init(P_IterQuery->countIterations(0, &k_, spGe));
		P_IterQuery->initIteration(false, &k_, spGe);
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

int PPViewDLSDetail::NextIteration(DLSDetailViewItem * pItem)
{
	int    ok = -1;
	memzero(pItem, sizeof(*pItem));
	if(P_DlsObjTbl && P_IterQuery && P_IterQuery->nextIteration() > 0) {
		ASSIGN_PTR(pItem, P_DlsObjTbl->data);
		Counter.Increment();
		ok = 1;
	}
	return ok;
}

DBQuery * PPViewDLSDetail::CreateBrowserQuery(uint * pBrwId, SString * pSubTitle)
{
	uint   brw_id = (Filt.ObjType == PPOBJ_GOODS) ? BROWSER_DLS_GOODS : BROWSER_DLS_SCARDS;
	DBQuery * q = 0;
	DBQ  * dbq = 0;
	DBE    dbe_goods;
	DlsObjTbl * tbl = 0;
	SCardTbl  * sc  = 0;
	DBFieldList fld_list;
	THROW(CheckTblPtr(tbl = new DlsObjTbl(P_DlsObjTbl->GetName())));
	if(pSubTitle) {
		GetDvcName(Filt.DvcType, pSubTitle);
		pSubTitle->Space();
		CatObjectName((Filt.DvcType == dvctCashs) ? PPOBJ_CASHNODE : PPOBJ_SCALE, Filt.DvcID, *pSubTitle);
	}
	q = &select(
		tbl->DlsID,      // #0
		tbl->ObjType,    // #1
		tbl->ObjID,      // #2
		tbl->LVal,       // #3
		tbl->Val, 0);    // #4
	dbq = & (tbl->DlsID == Filt.DlsID && tbl->ObjType == Filt.ObjType);
	if(Filt.ObjType == PPOBJ_GOODS) {
		PPDbqFuncPool::InitObjNameFunc(dbe_goods, PPDbqFuncPool::IdObjNameGoods, tbl->ObjID);
		q->addField(dbe_goods);   // #5
		q->from(tbl, 0L);
	}
	else if(Filt.ObjType == PPOBJ_SCARD) {
		THROW(CheckTblPtr(sc = new SCardTbl));
		dbq = & (*dbq && (sc->ID == tbl->ObjID));
		q->addField(sc->Code);    // #5
		q->from(tbl, sc, 0L);
	}
	q->where(*dbq).orderBy(tbl->ObjID, 0L);
	THROW(CheckQueryPtr(q));
	CATCH
		if(q)
			ZDELETE(q);
		else
			delete sc;
	ENDCATCH
	ASSIGN_PTR(pBrwId, brw_id);
	return q;
}

void PPViewDLSDetail::PreprocessBrowser(PPViewBrowser * pBrw)
{
	if(pBrw && Filt.DvcType == dvctScales) {
		// @v10.6.4 pBrw->InsColumnWord(1, PPWORD_PLU, 3, 0L, MKSFMTD(5, 0, NMBF_NOZERO), 0);
		pBrw->InsColumn(1, "@plu", 3, 0L, MKSFMTD(5, 0, NMBF_NOZERO), 0); // @v10.6.4
	}
}

int PPViewDLSDetail::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	if(ok == -2) {
		BrwHdr hdr;
		if(pHdr)
			hdr = *static_cast<const PPViewDLSDetail::BrwHdr *>(pHdr);
		else
			MEMSZERO(hdr);
		ok = -1;
		switch(ppvCmd) {
			case PPVCMD_EDITITEM:
				{
					int    r = 0;
					if(hdr.ObjType == PPOBJ_GOODS)
						r = GObj.Edit(&hdr.ObjID, 0);
					else if(hdr.ObjType == PPOBJ_SCARD)
						r = SCObj.Edit(&hdr.ObjID, 0);
					if(r == cmOK)
						ok = 1;
				}
				break;
			case PPVCMD_PUTTOBASKET:
				ok = -1;
				if(hdr.ObjType == PPOBJ_GOODS && hdr.ObjID) {
					AddGoodsToBasket(hdr.ObjID, 0, 1.0, hdr.Val);
				}
				break;
			case PPVCMD_PUTTOBASKETALL:
				ok = -1;
				if(Filt.ObjType == PPOBJ_GOODS) {
					SelBasketParam param;
					param.SelPrice = 3;
					int    r = GetBasketByDialog(&param, GetSymb());
					if(r > 0) {
						DLSDetailViewItem item;
						PPWaitStart();
						for(InitIteration(); NextIteration(&item) > 0;) {
							ILTI   i_i;
							i_i.GoodsID  = labs(item.ObjID);
							i_i.Price    = item.Val;
							i_i.CurPrice = 0.0;
							i_i.Flags    = 0;
							i_i.Quantity = 1.0;
							param.Pack.AddItem(&i_i, 0, param.SelReplace);
						}
						PPWaitStop();
						if(param.Pack.Lots.getCount()) {
							GoodsBasketDialog(param, 1);
						}
					}
				}
				break;
			case PPVCMD_VIEWLOTS:
				if(hdr.ObjType == PPOBJ_GOODS)
					::ViewLots(hdr.ObjID, 0, 0, 0, 0);
				break;
			case PPVCMD_VIEWQUOT:
				if(hdr.ObjType == PPOBJ_GOODS)
					GObj.EditQuotations(hdr.ObjID, 0, -1, 0, PPQuot::clsGeneral);
				break;
			case PPVCMD_VIEWCCHECKS:
				if(hdr.ObjType == PPOBJ_SCARD) {
					CCheckFilt flt;
					flt.SCardID = hdr.ObjID;
					ViewCCheck(&flt, 0);
				}
				break;
			case PPVCMD_SYSJ:
				ViewSysJournal(hdr.ObjType, hdr.ObjID, 0);
				break;
			default:
				ok = -2;
		}
	}
	return ok;
}
//
//
//
int ViewDLSDetail(const DLSDetailFilt & rFilt)
{
	int    ok = 1;
	PPViewDLSDetail * p_v = 0;
	if(rFilt.ObjType == PPOBJ_BILL) {
		if(rFilt.BillList.getCount()) {
			BillFilt filt;
			filt.List.Set(&rFilt.BillList);
			filt.Flags |= BillFilt::fBillListOnly;
			PPView::Execute(PPVIEW_BILL, &filt, 1, 0);
		}
	}
	else if(oneof2(rFilt.ObjType, PPOBJ_GOODS, PPOBJ_SCARD)) {
		p_v = new PPViewDLSDetail;
		PPWaitStart();
		THROW(p_v->Init(&rFilt));
		THROW(p_v->Browse(0));
	}
	CATCHZOKPPERR
	delete p_v;
	return ok;
}
