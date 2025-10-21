// V_TRANSP.CPP
// Copyright (c) A.Starodub 2009, 2010, 2012, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2022, 2023, 2024
// @codepage UTF-8
//
#include <pp.h>
#pragma hdrstop
//
// Фильтр по транспорту
//
int TransportFilt::InitInstance()
{
	SetFlatChunk(offsetof(TransportFilt, ReserveStart), offsetof(TransportFilt, Code) - offsetof(TransportFilt, ReserveStart));
	SetBranchSString(offsetof(TransportFilt, Code));
	SetBranchSString(offsetof(TransportFilt, TrailCode));
	return Init(1, 0);
}

IMPLEMENT_PPFILT_FACTORY(Transport); TransportFilt::TransportFilt() : PPBaseFilt(PPFILT_TRANSPORT, 0, 0)
{
	InitInstance();
}

/*virtual*/int TransportFilt::Describe(long flags, SString & rBuf) const
{
	PutObjMembToBuf(PPOBJ_TRANSPMODEL, ModelID,   STRINGIZE(ModelID),      rBuf);
	PutObjMembToBuf(PPOBJ_PERSON,      OwnerID,   STRINGIZE(BrandOwnerID), rBuf);
	PutObjMembToBuf(PPOBJ_PERSON,      CaptainID, STRINGIZE(CaptainID),    rBuf);
	PutObjMembToBuf(PPOBJ_COUNTRY,     CountryID, STRINGIZE(CountryID),    rBuf);
	PutMembToBuf(Code,      STRINGIZE(Code),      rBuf);
	PutMembToBuf(TrailCode, STRINGIZE(TrailCode), rBuf);
	// TrType
	{
		long id = 1;
		StrAssocArray flag_list;
		if(TrType == PPTRTYP_CAR)       flag_list.Add(id++, STRINGIZE(PPTRTYPE_CAR));
		else if(TrType == PPTRTYP_SHIP) flag_list.Add(id++, STRINGIZE(PPTRTYPE_SHIP));
		PutFlagsMembToBuf(&flag_list, STRINGIZE(TrType), rBuf);
	}
	return 1;
}

bool TransportFilt::IsEmpty() const
{
	return !(TrType || OwnerID || CaptainID || ModelID || CountryID || Code.NotEmpty() || TrailCode.NotEmpty());
}
//
// Storing format for GoodsFilt
//
//
PPViewTransport::PPViewTransport() : PPView(0, &Filt, PPVIEW_TRANSPORT, 0, REPORT_TRANSPORTVIEW), P_TempTbl(0)
{
}

PPViewTransport::~PPViewTransport()
{
	delete P_TempTbl;
	DBRemoveTempFiles();
}

PPBaseFilt * PPViewTransport::CreateFilt(const void * extraPtr) const
{
	TransportFilt * p_filt = new TransportFilt;
	p_filt->TrType = PPTRTYP_CAR;
	return p_filt;
}

class TransportFilterDlg : public TDialog {
	DECL_DIALOG_DATA(TransportFilt);
public:
	TransportFilterDlg() : TDialog(DLG_FLTTRANSP)
	{
		PPObjTransport::ReadConfig(&Cfg);
	}
	DECL_DIALOG_SETDTS()
	{
		RVALUEPTR(Data, pData);
		PPID   owner_kind_id = NZOR(Cfg.OwnerKindID, PPPRK_SHIPOWNER);
		PPID   captain_kind_id = NZOR(Cfg.CaptainKindID, PPPRK_CAPTAIN);
		setCtrlString(CTL_FLTTRANSP_CODE,      Data.Code);
		setCtrlString(CTL_FLTTRANSP_TRAILCODE, Data.TrailCode);
		SetupPPObjCombo(this, CTLSEL_FLTTRANSP_MODEL,   PPOBJ_TRANSPMODEL, Data.ModelID,   OLW_CANINSERT);
		SetupPPObjCombo(this, CTLSEL_FLTTRANSP_OWNER,   PPOBJ_PERSON,      Data.OwnerID,   OLW_CANINSERT, reinterpret_cast<void *>(owner_kind_id));
		SetupPPObjCombo(this, CTLSEL_FLTTRANSP_CAPTAIN, PPOBJ_PERSON,      Data.CaptainID, OLW_CANINSERT, reinterpret_cast<void *>(captain_kind_id));
		SetupPPObjCombo(this, CTLSEL_FLTTRANSP_CNTRY,   PPOBJ_COUNTRY,     Data.CountryID, OLW_CANINSERT);
		AddClusterAssoc(CTL_FLTTRANSP_TRTYPE, 0, 0);
		AddClusterAssoc(CTL_FLTTRANSP_TRTYPE, 1, PPTRTYP_CAR);
		AddClusterAssoc(CTL_FLTTRANSP_TRTYPE, 2, PPTRTYP_SHIP);
		SetClusterData(CTL_FLTTRANSP_TRTYPE, Data.TrType);
		SetupCtrls();
		return 1;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		getCtrlString(CTL_FLTTRANSP_CODE,        Data.Code);
		getCtrlString(CTL_FLTTRANSP_TRAILCODE,   Data.TrailCode);
		getCtrlData(CTLSEL_FLTTRANSP_MODEL,   &Data.ModelID);
		getCtrlData(CTLSEL_FLTTRANSP_OWNER,   &Data.OwnerID);
		getCtrlData(CTLSEL_FLTTRANSP_CAPTAIN, &Data.CaptainID);
		getCtrlData(CTLSEL_FLTTRANSP_CNTRY,   &Data.CountryID);
		GetClusterData(CTL_FLTTRANSP_TRTYPE,  &Data.TrType);
		ASSIGN_PTR(pData, Data);
		return ok;
	}
private:
	DECL_HANDLE_EVENT;
	void   SetupCtrls();
	PPTransportConfig Cfg;
};

IMPL_HANDLE_EVENT(TransportFilterDlg)
{
	TDialog::handleEvent(event);
	if(event.isCmd(cmClusterClk) && event.isCtlEvent(CTL_FLTTRANSP_TRTYPE)) {
		SetupCtrls();
		clearEvent(event);
	}
}

void TransportFilterDlg::SetupCtrls()
{
	long   tr_type = 0;
	PPID   model_id = 0;
	PPID   country_id = 0;
	SString code;
	SString trail_code;
	getCtrlString(CTL_FLTTRANSP_CODE,      code);
	getCtrlString(CTL_FLTTRANSP_TRAILCODE, trail_code);
	getCtrlData(CTLSEL_FLTTRANSP_CNTRY,    &country_id);
	getCtrlData(CTLSEL_FLTTRANSP_MODEL,    &model_id);
	GetClusterData(CTL_FLTTRANSP_TRTYPE, &tr_type);
	disableCtrls(tr_type == PPTRTYP_SHIP, CTL_FLTTRANSP_CODE, CTL_FLTTRANSP_TRAILCODE, CTLSEL_FLTTRANSP_MODEL, 0L);
	disableCtrl(CTLSEL_FLTTRANSP_CNTRY, tr_type == PPTRTYP_CAR);
	if(tr_type == PPTRTYP_SHIP) {
		model_id = 0;
		code = 0;
		trail_code = 0;
	}
	else if(tr_type == PPTRTYP_CAR)
		country_id = 0;
	SetupPPObjCombo(this, CTLSEL_FLTTRANSP_MODEL, PPOBJ_TRANSPMODEL, model_id,   OLW_CANINSERT);
	SetupPPObjCombo(this, CTLSEL_FLTTRANSP_CNTRY, PPOBJ_COUNTRY,     country_id, OLW_CANINSERT);
	setCtrlString(CTL_FLTTRANSP_CODE, code);
	setCtrlString(CTL_FLTTRANSP_TRAILCODE, trail_code);
}

int PPViewTransport::EditBaseFilt(PPBaseFilt * pBaseFilt)
{
	if(Filt.IsA(pBaseFilt)) {
		DIALOG_PROC_BODYERR(TransportFilterDlg, static_cast<TransportFilt *>(pBaseFilt));
	}
	else
		return 0;
}

DBQuery * PPViewTransport::CreateBrowserQuery(uint * pBrwId, SString * pSubTitle)
{
	DBQuery * q = 0;
	DBE    dbe_owner, dbe_captain, dbe_country, dbe_trtype;
	DBQ  * dbq = 0;
	TempTransportTbl * p_tmp_t = 0;
	uint   brw_id = BROWSER_TRANSPORT;
	THROW(P_TempTbl);
	THROW(CheckTblPtr(p_tmp_t = new TempTransportTbl(P_TempTbl->GetName())));
	PPDbqFuncPool::InitObjNameFunc(dbe_owner,   PPDbqFuncPool::IdObjNamePerson,  p_tmp_t->OwnerID);
	PPDbqFuncPool::InitObjNameFunc(dbe_captain, PPDbqFuncPool::IdObjNamePerson,  p_tmp_t->CaptainID);
	PPDbqFuncPool::InitObjNameFunc(dbe_country, PPDbqFuncPool::IdObjNameWorld,   p_tmp_t->CountryID);
	PPDbqFuncPool::InitLongFunc(dbe_trtype, PPDbqFuncPool::IdTransportTypeName,  p_tmp_t->TrType);

	q = & select(p_tmp_t->ID, 0L);                          // #00
	q->addField(p_tmp_t->Name);                             // #01
	q->addField(dbe_trtype);                                // #02
	q->addField(p_tmp_t->ModelName);                        // #03
	q->addField(dbe_owner);                                 // #04
	q->addField(dbe_captain);                               // #05
	q->addField(dbe_country);                               // #06
	q->addField(p_tmp_t->Code);                             // #07
	q->addField(p_tmp_t->TrailCode);                        // #08
	q->addField(p_tmp_t->Capacity);                         // #09
	q->addTable(p_tmp_t);
	q->where(*dbq).orderBy(p_tmp_t->Name, 0L);
	THROW(CheckQueryPtr(q));
	CATCH
		if(q)
			ZDELETE(q);
		else
			delete p_tmp_t;
	ENDCATCH
	ASSIGN_PTR(pBrwId, brw_id);
	return q;
}

// PP_CREATE_TEMP_FILE_PROC(CreateTempTransportFile, TempTransport);

void PPViewTransport::MakeTempRec(const PPTransportPacket * pTranspPack, TempTransportTbl::Rec * pTempRec)
{
	if(pTempRec && pTranspPack) {
		SString temp_buf;
		memzero(pTempRec, sizeof(*pTempRec));
		pTempRec->ID = pTranspPack->Rec.ID;
		STRNSCPY(pTempRec->Name, pTranspPack->Rec.Name);
		GetObjectName(PPOBJ_TRANSPMODEL, pTranspPack->Rec.TrModelID, temp_buf.Z());
		STRNSCPY(pTempRec->ModelName, temp_buf);
		pTempRec->ModelID   = pTranspPack->Rec.TrModelID;
		pTempRec->OwnerID   = pTranspPack->Rec.OwnerID;
		pTempRec->CaptainID = pTranspPack->Rec.CaptainID;
		pTempRec->TrType    = pTranspPack->Rec.TrType;
		pTempRec->CountryID = pTranspPack->Rec.CountryID;
		pTempRec->Capacity  = pTranspPack->Rec.Capacity;
		STRNSCPY(pTempRec->Code,      pTranspPack->Rec.Code);
		STRNSCPY(pTempRec->TrailCode, pTranspPack->Rec.TrailerCode);
	}
}

int PPViewTransport::UpdateTempTable(PPID transpID, PPViewBrowser * pBrw)
{
	int    ok = 1;
	if(P_TempTbl) {
		if(transpID) {
			PPTransportPacket t_pack;
			PPTransaction tra(ppDbDependTransaction, 1);
			THROW(tra);
			if(TObj.Get(transpID, &t_pack) > 0 && CheckForFilt(&Filt, transpID, 0)) {
				TempTransportTbl::Rec temp_rec;
				MakeTempRec(&t_pack, &temp_rec);
				TempTransportTbl::Key0 k0;
				MEMSZERO(k0);
				k0.ID = transpID;
				if(P_TempTbl->searchForUpdate(0, &k0, spEq)) {
					THROW_DB(P_TempTbl->updateRecBuf(&temp_rec)); // @sfu
				}
				else {
					THROW_DB(P_TempTbl->insertRecBuf(&temp_rec));
				}
			}
			else {
				THROW_DB(deleteFrom(P_TempTbl, 0, P_TempTbl->ID == transpID));
			}
			THROW(tra.Commit());
		}
		else if(pBrw)
			ok = ChangeFilt(1, pBrw);
	}
	CATCHZOKPPERR
	return ok;
}

int PPViewTransport::CheckForFilt(TransportFilt * pFilt, PPID transpID, const PPTransportPacket * pPack)
{
	PPTransportPacket temp_pack;
	if(pPack)
		temp_pack = *pPack;
	else if(transpID)
		TObj.Get(transpID, &temp_pack);
	if(!CheckFiltID(pFilt->ModelID, temp_pack.Rec.TrModelID))
		return 0;
	else if(!CheckFiltID(pFilt->TrType, temp_pack.Rec.TrType))
		return 0;
	else if(!CheckFiltID(pFilt->OwnerID, temp_pack.Rec.OwnerID))
		return 0;
	else if(!CheckFiltID(pFilt->CaptainID, temp_pack.Rec.CaptainID))
		return 0;
	else if(pFilt->TrType == PPTRTYP_CAR) {
		if(pFilt->Code.NotEmpty() && pFilt->Code.Cmp(temp_pack.Rec.Code, 0) != 0)
			return 0;
		else if(pFilt->TrailCode.NotEmpty() && pFilt->TrailCode.Cmp(temp_pack.Rec.TrailerCode, 0) != 0)
			return 0;
	}
	else if(pFilt->TrType == PPTRTYP_SHIP)
		if(pFilt->CountryID && pFilt->CountryID != temp_pack.Rec.CountryID)
			return 0;
	return 1;
}

bool PPViewTransport::IsTempTblNeeded() { return true; }

int PPViewTransport::Init_(const PPBaseFilt * pFilt)
{
	int    ok = 1;
	LongArray * p_list = 0;
	BExtQuery::ZDelete(&P_IterQuery);
	ZDELETE(P_TempTbl);
	THROW(Helper_InitBaseFilt(pFilt));
	if(IsTempTblNeeded()) {
		THROW(p_list = TObj.MakeList(Filt.TrType));
		THROW(P_TempTbl = CreateTempFile <TempTransportTbl> ());
		uint   count = p_list->getCount();
		if(count) {
			BExtInsert bei(P_TempTbl);
			PPTransaction tra(ppDbDependTransaction, 1);
			THROW(tra);
			for(uint i = 0; i < count; i++) {
				PPID id = p_list->at(i);
				PPTransportPacket t_pack;
				if(TObj.Get(id, &t_pack) > 0 && CheckForFilt(&Filt, 0, &t_pack)) {
					TempTransportTbl::Rec rec;
					MakeTempRec(&t_pack, &rec);
					THROW_DB(bei.insert(&rec));
				}
				PPWaitPercent(i, count);
			}
			THROW_DB(bei.flash());
			THROW(tra.Commit());
		}
	}
	CATCH
		ZDELETE(P_TempTbl);
		ok = 0;
	ENDCATCH
	return ok;
}

int PPViewTransport::InitIteration(int aOrder)
{
	int    ok  = 1;
	Counter.Init();
	BExtQuery::ZDelete(&P_IterQuery);
	if(P_TempTbl) {
		TempTransportTbl::Key1 k1;
		PPInitIterCounter(Counter, P_TempTbl);
		THROW_MEM(P_IterQuery = new BExtQuery(P_TempTbl, 1));
		P_IterQuery->select(P_TempTbl->ID, 0);
		MEMSZERO(k1);
		P_IterQuery->initIteration(false, &k1, spFirst);
	}
	CATCHZOK
	return ok;
}

int FASTCALL PPViewTransport::NextIteration(TransportViewItem * pItem)
{
	int    ok = -1;
	if(P_IterQuery) {
		if(P_IterQuery->nextIteration() > 0) {
		   	const  PPID transp_id = P_TempTbl->data.ID;
			PPTransportPacket temp_pack;
		   	ok = TObj.Get(transp_id, &temp_pack);
			if(pItem) {
				*static_cast<PPTransport *>(pItem) = temp_pack.Rec;
			}
		   	Counter.Increment();
		}
	}
	return ok;
}

void PPViewTransport::ViewTotal()
{
	TDialog * p_dlg = new TDialog(DLG_GOODSTOTAL);
	if(CheckDialogPtrErr(&p_dlg)) {
		long   count = 0;
		SString title;
		TransportViewItem item;
		PPWaitStart();
		for(InitIteration(0); NextIteration(&item) > 0; PPWaitPercent(GetCounter()))
			count++;
		PPWaitStop();
		p_dlg->setCtrlLong(CTL_GOODSTOTAL_COUNT, count);
		PPLoadText(PPTXT_TRANSPTOTAL, title);
		p_dlg->setOrgTitle(title);
		ExecViewAndDestroy(p_dlg);
	}
}

int PPViewTransport::DeleteItem(PPID id)
{
	return (id && CONFIRM(PPCFM_DELETE)) ? TObj.Put(&id, 0, 1) : -1;
}

int PPViewTransport::Transmit(PPID /*id*/)
{
	int    ok = -1;
	ObjTransmitParam param;
	if(ObjTransmDialog(DLG_OBJTRANSM, &param) > 0) {
		TransportViewItem item;
		const PPIDArray & rary = param.DestDBDivList.Get();
		PPObjIDArray objid_ary;
		PPWaitStart();
		for(InitIteration(0); NextIteration(&item) > 0; PPWaitPercent(GetCounter()))
			objid_ary.Add(PPOBJ_TRANSPORT, item.ID);
		THROW(PPObjectTransmit::Transmit(&rary, &objid_ary, &param));
		ok = 1;
	}
	CATCHZOKPPERR
	PPWaitStop();
	return ok;
}
//
//
//
int PPViewTransport::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	if(ok == -2) {
		PPID   id = pHdr ? *static_cast<const PPID *>(pHdr) : 0;
		switch(ppvCmd) {
				break;
			case PPVCMD_EDITITEM:
				ok = (TObj.Edit(&id, reinterpret_cast<void *>(Filt.TrType)) == cmOK) ? 1 : -1;
				if(ok > 0)
					UpdateTempTable(id, pBrw);
				break;
			case PPVCMD_ADDITEM:
				id = 0;
				ok = (TObj.Edit(&id, reinterpret_cast<void *>(Filt.TrType)) == cmOK) ? 1 : -1;
				if(ok > 0)
					UpdateTempTable(id, pBrw);
				break;
			case PPVCMD_DELETEITEM:
				ok = DeleteItem(id);
				if(ok > 0)
					UpdateTempTable(id, pBrw);
				break;
			case PPVCMD_TRANSMIT:
				ok = -1;
				Transmit(id);
				break;
			case PPVCMD_SYSJ:
				if(id) {
					ViewSysJournal(PPOBJ_TRANSPORT, id, 0);
					ok = -1;
				}
				break;
		}
	}
	return ok;
}
//
// Implementation of PPALDD_Transport
//
PPALDD_CONSTRUCTOR(TransportView)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		AssignDefIterData(&I, sizeof(I));
	}
}

PPALDD_DESTRUCTOR(TransportView) { Destroy(); }

int PPALDD_TransportView::InitData(PPFilt & rFilt, long rsrv)
{
	INIT_PPVIEW_ALDD_DATA_U(Transport, rsrv);
	H.FltType        = p_filt->TrType;
	H.FltOwnerID     = p_filt->OwnerID;
	H.FltCaptainID   = p_filt->CaptainID;
	H.FltCountryID   = p_filt->CountryID;
	STRNSCPY(H.FltCode, p_filt->Code);
	STRNSCPY(H.FltTrailCode, p_filt->TrailCode);
	{
		SString temp_buf;
		GetObjectName(PPOBJ_TRANSPMODEL, p_filt->ModelID, temp_buf);
		STRNSCPY(H.FltModelName, temp_buf);
	}
	return DlRtm::InitData(rFilt, rsrv);
}

int PPALDD_TransportView::InitIteration(PPIterID iterId, int sortId, long /*rsrv*/)
{
	INIT_PPVIEW_ALDD_ITER(Transport);
}

int PPALDD_TransportView::NextIteration(PPIterID iterId)
{
	START_PPVIEW_ALDD_ITER(Transport);
	I.TransportID  = item.ID;
	PPWaitPercent(p_v->GetCounter());
	FINISH_PPVIEW_ALDD_ITER();
}

void PPALDD_TransportView::Destroy() { DESTROY_PPVIEW_ALDD(Transport); }
