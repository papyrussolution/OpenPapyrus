// DLS.CPP
// Copyright (c) A.Sobolev 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2013, 2014, 2015, 2016
//
#include <pp.h>
#pragma hdrstop

struct _GoodsInfo {
	PPID   ID;
	long   PLU;
	double Price;
};

SLAPI DeviceLoadingStat::DeviceLoadingStat() : DvcLoadingStatTbl()
{
	StatID = 0;
	P_GoodsList = new SArray(sizeof(_GoodsInfo));
	P_StatCache = new SArray(sizeof(DvcLoadingStatTbl::Rec));
}

SLAPI DeviceLoadingStat::~DeviceLoadingStat()
{
	delete P_GoodsList;
	delete P_StatCache;
}

int SLAPI DeviceLoadingStat::Fetch(PPID statID, DvcLoadingStatTbl::Rec * pRec)
{
	int    ok = -1;
	uint   pos = 0;
	if(P_StatCache == 0)
		THROW_MEM(P_StatCache = new SArray(sizeof(DvcLoadingStatTbl::Rec)));
	if(P_StatCache->lsearch(&statID, &pos, CMPF_LONG)) {
		ASSIGN_PTR(pRec, *(DvcLoadingStatTbl::Rec *)P_StatCache->at(pos));
		ok = 1;
	}
	else {
		DvcLoadingStatTbl::Rec rec;
		if(Search(statID, &rec) > 0) {
			THROW_SL(P_StatCache->insert(&rec));
			ASSIGN_PTR(pRec, rec);
			ok = 1;
		}
	}
	CATCHZOK
	return ok;
}

PPID SLAPI DeviceLoadingStat::GetCurStatID() const
{
	return StatID;
}

int SLAPI DeviceLoadingStat::Search(PPID statID, DvcLoadingStatTbl::Rec * pRec)
{
	return SearchByID(this, 0, statID, pRec);
}

int SLAPI DeviceLoadingStat::GetPrev(PPID curStatID, PPID * pStatID, DvcLoadingStatTbl::Rec * pRec)
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

int SLAPI DeviceLoadingStat::GetLast(int dvcType, PPID dvcID,
	const LDATETIME & rDtm, PPID * pStatID, DvcLoadingStatTbl::Rec * pRec)
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

int SLAPI DeviceLoadingStat::GetLastObjInfo(PPID objType, PPID objID, LDATE dt, DlsObjTbl::Rec * pRec)
{
	DvcLoadingStatTbl::Rec rec;
	return (Fetch(StatID, &rec) > 0) ? GetLastObjInfo(rec.DvcType, rec.DvcID, objType, objID, dt, pRec) : 0;
}

int SLAPI DeviceLoadingStat::GetLastObjInfo(int dvcType, PPID dvcID, PPID objType, PPID objID, LDATE dt, DlsObjTbl::Rec * pRec)
{
	DlsObjTbl::Key0 k0;
	MEMSZERO(k0);
	k0.ObjType = (int16)objType;
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

int SLAPI DeviceLoadingStat::StartLoading(PPID * pStatID, int deviceType, PPID deviceID, int use_ta)
{
	int    ok = 1;
	PPID   id = 0;
	DvcLoadingStatTbl::Rec rec;
	MEMSZERO(rec);
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

int SLAPI DeviceLoadingStat::RegisterGoods(PPID statID, const GoodsInfo * pInfo)
{
	int    ok = 1;
	uint   pos = 0;
	THROW_INVARG(statID == StatID);
	if(P_GoodsList == 0)
		THROW_MEM(P_GoodsList = new SArray(sizeof(_GoodsInfo)));
	if(!P_GoodsList->lsearch(&pInfo->ID, &pos, CMPF_LONG)) {
		_GoodsInfo  gds_info;
		gds_info.ID    = pInfo->ID;
		gds_info.PLU   = pInfo->PLU;
		gds_info.Price = pInfo->Price;
		THROW_SL(P_GoodsList->insert(&gds_info));
	}
	CATCHZOK
	return ok;
}

int SLAPI DeviceLoadingStat::RegisterSCard(PPID statID, const SCardInfo * pInfo)
{
	int    ok = 1;
	THROW_INVARG(statID == StatID);
	if(!SCardList.Search(pInfo->ID, 0, 0))
		THROW_SL(SCardList.Add(pInfo->ID, pInfo->Discount, 0));
	CATCHZOK
	return ok;
}

int SLAPI DeviceLoadingStat::CutTables(short dvcType, int use_ta)
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

int SLAPI DeviceLoadingStat::FinishLoading(PPID statID, int status, int use_ta)
{
	int    ok = 1;
	long   timing = 0;
	DvcLoadingStatTbl::Rec rec;
	THROW_INVARG(statID == StatID);
	timing = clock() - StartClock;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(SearchByID(this, 0, statID, &rec) > 0);
		THROW(CutTables(rec.DvcType, 0));
		rec.Cont = timing;
		rec.Status = status;
		THROW(UpdateByID(this, 0, statID, &rec, 0));
		{
			DlsObjTbl::Rec dlso_rec;
			BExtInsert bei(&DlsoT);
			if(P_GoodsList->getCount()) {
				for(uint i = 0; i < P_GoodsList->getCount(); i++) {
					const _GoodsInfo & gds_info = *(_GoodsInfo *)P_GoodsList->at(i);
					MEMSZERO(dlso_rec);
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
					MEMSZERO(dlso_rec);
					dlso_rec.DlsID   = statID;
					dlso_rec.ObjType = PPOBJ_SCARD;
					dlso_rec.ObjID   = SCardList.at(i).Key;
					dlso_rec.Val     = SCardList.at(i).Val;
					THROW_DB(bei.insert(&dlso_rec));
				}
			}
			THROW_DB(bei.flash());
		}
		StatID = 0;
		StartClock = 0;
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI DeviceLoadingStat::GetExportedObjectsSince(PPID objType, PPID sinceDlsID, PPIDArray * pObjList)
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
		const PPID _id = ev_list.get(i);
		DlsObjTbl::Key1 k1;
		MEMSZERO(k1);
		k1.DlsID = rec.ID;
		k1.ObjType = (int16)objType;
		BExtQuery q(&DlsoT, 1);
		q.select(DlsoT.ObjID, 0).where(DlsoT.DlsID == _id && DlsoT.ObjType == objType);
		for(q.initIteration(0, &k1, spGe); q.nextIteration() > 0;) {
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

int SLAPI DeviceLoadingStat::GetUpdatedObjects(PPID objType, const LDATETIME & since, PPIDArray * pObjList)
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
		acn_list.addzlist(PPACN_GOODSQUOTUPD, PPACN_QUOTUPD2, PPACN_GOODSNODISRMVD, 0L); // @v7.3.3 PPACN_QUOTUPD2
	else if(objType == PPOBJ_SCARD)
		acn_list.add(PPACN_SCARDDISUPD);
	acn_list.sort();
	for(q.initIteration(0, &k, spGe); q.nextIteration() > 0;) { // @v7.7.8 spGt-->spGe
		if(cmp(since, p_sj->data.Dt, p_sj->data.Tm) < 0 && acn_list.bsearch(p_sj->data.Action))
			if(pObjList)
				THROW(pObjList->add(p_sj->data.ObjID)); // @v8.0.10 addUnique-->add
	}
	CALLPTRMEMB(pObjList, sortAndUndup()); // @v8.0.10 sort-->sortAndUndup
	CATCHZOK
	return ok;
}

int SLAPI DeviceLoadingStat::DoMaintain(LDATE toDt)
{
	int    ok = 1;
	long   total = 0;
	SString msg;
	IterCounter counter;
	DvcLoadingStatTbl::Key2 k;
	{
		SString buf, added_param;
		SPathStruc ps;
		ps.Split(fileName);
		ps.Merge(0, SPathStruc::fDrv|SPathStruc::fDir, added_param);
		msg.Printf(PPLoadTextS(PPTXT_DBMAINTAIN, buf), added_param);
	}
	k.Dt      = toDt;
	k.Tm.v    = MAXLONG;
	k.DvcType = MAXSHORT; // @v8.9.8 MAXINT-->MAXSHORT
	k.DvcID   = MAXLONG;
	if(search(2, &k, spLe) && k.Dt <= toDt) {
		total++;
		while(search(2, &k, spPrev) && k.Dt <= toDt)
			total++;
	}
	counter.Init(total);
	k.Dt      = toDt;
	k.Tm.v    = MAXLONG;
	k.DvcType = MAXSHORT; // @v8.9.8 MAXINT-->MAXSHORT
	k.DvcID   = MAXLONG;
	while(search(2, &k, spLe) && k.Dt <= toDt) {
		THROW(Remove(data.ID, 1));
		PPWaitPercent(counter.Increment(), msg);
	}
	CATCHZOK
	return ok;
}

int SLAPI DeviceLoadingStat::Remove(PPID id, int useTa)
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
IMPLEMENT_PPFILT_FACTORY(DvcLoadingStat); SLAPI DvcLoadingStatFilt::DvcLoadingStatFilt() : PPBaseFilt(PPFILT_DVCLOADINGSTAT, 0, 0)
{
	SetFlatChunk(offsetof(DvcLoadingStatFilt, ReserveStart),
		offsetof(DvcLoadingStatFilt, Reserve)-offsetof(DvcLoadingStatFilt, ReserveStart)+sizeof(Reserve));
	Init(1, 0);
}
//
//
//
#define GRP_GOODS 1

class DLSFiltDialog : public TDialog {
public:
	DLSFiltDialog() : TDialog(DLG_DLSFLT)
	{
		LastDvcTypeSel = 0;
		addGroup(GRP_GOODS, new GoodsCtrlGroup(CTLSEL_DLSFLT_GGRP, CTLSEL_DLSFLT_GOODS));
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
	SetupCalCtrl(CTLCAL_DLSFLT_PERIOD, this, CTL_DLSFLT_PERIOD, 1);
	SetPeriodInput(this, CTL_DLSFLT_PERIOD, &Data.Period);
	{
		GoodsCtrlGroup::Rec ggrp_rec(Data.GoodsGrpID, Data.GoodsID, 0, GoodsCtrlGroup::enableSelUpLevel);
		setGroupData(GRP_GOODS, &ggrp_rec);
	}
	return 1;
}

int DLSFiltDialog::getDTS(DvcLoadingStatFilt * pData)
{
	int    ok = 1;
	uint   sel = 0;
	PPID   dvc_type, dvc_id;
	GetDeviceData(&dvc_type, &dvc_id);
	Data.DvcType = (short)dvc_type;
	Data.DvcID   = dvc_id;
	sel = CTL_DLSFLT_DVCTYPE;
	THROW_PP(Data.DvcType, PPERR_DVCTYPENEEDED);
	sel = CTL_DLSFLT_PERIOD;
	THROW(GetPeriodInput(this, CTL_DLSFLT_PERIOD, &Data.Period));
	{
		GoodsCtrlGroup::Rec ggrp_rec;
		getGroupData(GRP_GOODS, &ggrp_rec);
		Data.GoodsGrpID = ggrp_rec.GrpID;
		Data.GoodsID    = ggrp_rec.GoodsID;
	}
	ASSIGN_PTR(pData, Data);
	CATCH
		ok = PPErrorByDialog(this, sel);
	ENDCATCH
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
int SLAPI PPViewDvcLoadingStat::EditBaseFilt(PPBaseFilt * pBaseFilt)
{
	if(!Filt.IsA(pBaseFilt))
		return 0;
	DvcLoadingStatFilt * p_filt = (DvcLoadingStatFilt *)pBaseFilt;
	DIALOG_PROC_BODY(DLSFiltDialog, p_filt);
}

SLAPI PPViewDvcLoadingStat::PPViewDvcLoadingStat() : PPView(0, &Filt, PPVIEW_DVCLOADINGSTAT)
{
}

SLAPI PPViewDvcLoadingStat::~PPViewDvcLoadingStat()
{
}

int SLAPI PPViewDvcLoadingStat::Init_(const PPBaseFilt * pBaseFilt)
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

int SLAPI PPViewDvcLoadingStat::InitIteration()
{
	int    ok = 1;
	DvcLoadingStatTbl * p_t = &DlsT;
	DvcLoadingStatTbl::Key1 k1, k1_;
	ZDELETE(P_IterQuery);
	Counter.Init();

	THROW_MEM(P_IterQuery = new BExtQuery(p_t, 1));
	P_IterQuery->select(p_t->ID, p_t->DvcType, p_t->DvcID, p_t->Dt, p_t->Tm, p_t->Cont, p_t->Status, 0L);
	MEMSZERO(k1);
	k1_ = k1;
	Counter.Init(P_IterQuery->countIterations(0, &k1_, spFirst));
	P_IterQuery->initIteration(0, &k1, spFirst);
	CATCHZOK
	return ok;
}

int SLAPI PPViewDvcLoadingStat::NextIteration(DvcLoadingStatViewItem * pItem)
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
		char * p = strchr(dvc_name, ',');
		if(p) {
			pSs->Cat(++p);
			ok = 1;
		}
	}
	return ok;
}

DBQuery * SLAPI PPViewDvcLoadingStat::CreateBrowserQuery(uint * pBrwId, SString * pSubTitle)
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
	THROW(CheckTblPtr(p_tbl = new DvcLoadingStatTbl(DlsT.fileName)));
	if(Filt.DvcID && pSubTitle) {
		pSubTitle->CatChar(0xa0);
		pSubTitle->CatChar(':');
		pSubTitle->Space();
		GetDvcName(Filt.DvcType, pSubTitle);
		pSubTitle->Space();
		GetObjectName(dvc_obj_type, Filt.DvcID, *pSubTitle, 1);
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
			0).from(p_tbl, p_dlso_tbl, (Filt.GoodsGrpID) ? p_g_tbl : 0L, 0L);
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
	dbq = & (p_tbl->DvcType == (long)Filt.DvcType);
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

int SLAPI PPViewDvcLoadingStat::PreprocessBrowser(PPViewBrowser * pBrw)
{
	int    ok = -1;
	if(pBrw && !Filt.DvcID) {
		SString  dvc_name;
		GetDvcName(Filt.DvcType, &dvc_name);
		pBrw->view->insertColumn(0, dvc_name, 7, 0L, MKSFMTD(20, 0, 0), 0);
		ok = 1;
	}
	if(Filt.GoodsGrpID || Filt.GoodsID) {
		pBrw->view->insertColumn(2, "@ware", 8, 0L, MKSFMTD(20, 0, 0), 0);
		pBrw->view->insertColumn(3, "@price", 9, 0L, SFMT_MONEY, 0);
		ok = 1;
	}
	return ok;
}

int SLAPI PPViewDvcLoadingStat::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	if(ok == -2) {
		BrwHdr hdr;
		if(pHdr)
			hdr = *(PPViewDvcLoadingStat::BrwHdr *)pHdr;
		else
			MEMSZERO(hdr);
		if(ppvCmd == PPVCMD_EDITGOODS || (ppvCmd == PPVCMD_SCARDS && hdr.DvcType == dvctCashs)) {
			DLSDetailFilt  dlsd_flt;
			dlsd_flt.DlsID   = hdr.DlsID;
			dlsd_flt.DvcType = hdr.DvcType;
			dlsd_flt.DvcID   = hdr.DvcID;
			dlsd_flt.ObjType = (ppvCmd == PPVCMD_EDITGOODS) ? PPOBJ_GOODS : PPOBJ_SCARD;
			ViewDLSDetail(&dlsd_flt);
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
SLAPI DLSDetailFilt::DLSDetailFilt()
{
	THISZERO();
}

SLAPI PPViewDLSDetail::PPViewDLSDetail() : PPView(0)
{
	P_DlsObjTbl = 0;
}

SLAPI PPViewDLSDetail::~PPViewDLSDetail()
{
	delete P_DlsObjTbl;
}

int SLAPI PPViewDLSDetail::Init(const DLSDetailFilt * pFilt)
{
	Filt = *pFilt;
	return CheckTblPtr(P_DlsObjTbl = new DlsObjTbl);
}

DBQuery * SLAPI PPViewDLSDetail::CreateBrowserQuery(uint * pBrwId, SString * pSubTitle)
{
	uint   brw_id = (Filt.ObjType == PPOBJ_GOODS) ? BROWSER_DLS_GOODS : BROWSER_DLS_SCARDS;
	DBQuery * q = 0;
	DBQ  * dbq = 0;
	DBE    dbe_goods;
	DlsObjTbl * tbl = 0;
	SCardTbl  * sc  = 0;
	DBFieldList fld_list;
	THROW(CheckTblPtr(tbl = new DlsObjTbl(P_DlsObjTbl->fileName)));
	if(pSubTitle) {
		GetDvcName(Filt.DvcType, pSubTitle);
		pSubTitle->Space();
		GetObjectName((Filt.DvcType == dvctCashs) ? PPOBJ_CASHNODE : PPOBJ_SCALE, Filt.DvcID, *pSubTitle, 1);
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
	else {
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

int SLAPI PPViewDLSDetail::PreprocessBrowser(PPViewBrowser * pBrw)
{
	int    ok = -1;
	if(pBrw && Filt.DvcType == dvctScales) {
		pBrw->InsColumnWord(1, PPWORD_PLU, 3, 0L, MKSFMTD(5, 0, NMBF_NOZERO), 0);
		ok = 1;
	}
	return ok;
}

int SLAPI PPViewDLSDetail::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	if(ok == -2) {
		BrwHdr hdr;
		if(pHdr)
			hdr = *(PPViewDLSDetail::BrwHdr *)pHdr;
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
int SLAPI ViewDLSDetail(DLSDetailFilt * pFilt)
{
	int    ok = 1;
	PPViewDLSDetail * p_v = new PPViewDLSDetail;
	PPWait(1);
	THROW(p_v->Init(pFilt));
	THROW(p_v->Browse(0));
	CATCHZOKPPERR
	delete p_v;
	return ok;
}

