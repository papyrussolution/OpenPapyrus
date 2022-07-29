// OBJQCERT.CPP
// Copyright (c) A.Sobolev 1996, 1997, 1998-2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2013, 2015, 2016, 2017, 2019, 2020, 2022
// @codepage windows-1251
//
#include <pp.h>
#pragma hdrstop

TLP_IMPL(PPObjQCert, QualityCertTbl, P_Tbl);

PPObjQCert::PPObjQCert(void * extraPtr) : PPObject(PPOBJ_QCERT), ExtraPtr(extraPtr)
{
	TLP_OPEN(P_Tbl);
}

PPObjQCert::~PPObjQCert()
{
	TLP_CLOSE(P_Tbl);
}

int PPObjQCert::Search(PPID id, void * b)
{
	return SearchByID(P_Tbl, Obj, id, b);
}

int PPObjQCert::SearchByCode(const char * pCode, PPID * pID, QualityCertTbl::Rec * pRec)
{
	QualityCertTbl::Key1 k;
	MEMSZERO(k);
	STRNSCPY(k.Code, pCode);
	int    ok = SearchByKey(P_Tbl, 1, &k, pRec);
	if(ok > 0)
		ASSIGN_PTR(pID, P_Tbl->data.ID);
	return ok;
}

int PPObjQCert::PutPacket(PPID * pID, QualityCertTbl::Rec * pPack, int use_ta)
{
	int    ok = 1;
	int    action = 0;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(*pID) {
			if(pPack) {
				int    r;
				THROW(CheckRights(PPR_MOD));
				pPack->ID = *pID;
				THROW(r = UpdateByID_Cmp(P_Tbl, Obj, *pID, pPack, 0));
				if(r > 0)
					action = PPACN_OBJUPD;
				else
					ok = -1;
			}
			else {
				THROW(CheckRights(PPR_DEL));
				THROW_DB(deleteFrom(P_Tbl, 0, (P_Tbl->ID == *pID)));
				action = PPACN_OBJRMV;
			}
		}
		else if(pPack) {
			THROW(CheckRights(PPR_INS));
			THROW(AddObjRecByID(P_Tbl, Obj, pID, pPack, 0));
			action = PPACN_OBJADD;
		}
		if(action)
			DS.LogAction(action, Obj, *pID, 0, 0);
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int PPObjQCert::SetPassiveTag(PPID id, int set, int use_ta)
{
	int    ok = -1;
	QualityCertTbl::Rec rec;
	if(Search(id, &rec) > 0) {
		int    nothing_todo = 0;
		if(set && rec.Passive == 0)
			rec.Passive = 1;
		else if(!set && rec.Passive)
			rec.Passive = 0;
		else
			nothing_todo = 1;
		if(!nothing_todo) {
			THROW(PutPacket(&id, &rec, use_ta));
			ok = 1;
		}
	}
	CATCHZOK
	return ok;
}

int PPObjQCert::DeleteObj(PPID id)
{
	return PutPacket(&id, 0, 0);
}

ListBoxDef * PPObjQCert::Selector(ListBoxDef * pOrgDef, long flags, void * extraPtr)
{
	ListBoxDef * def = 0;
	DBQuery   * q = 0;
	QualityCertTbl * t = 0;
	if(extraPtr) {
		const PPID goods_id = reinterpret_cast<PPID>(extraPtr);
		struct {
			PPID   QCertID;
			char   Code[26];
		} item;
		LDATE  dt = MAXDATE;
		long   oprno = 0;
		ReceiptTbl::Rec lot_rec;
		PPIDArray list;
		SArray * p_items = new SArray(sizeof(item));
		while(BillObj->trfr->Rcpt.EnumLastLots(goods_id, 0L, &dt, &oprno, &lot_rec) > 0)
			if(lot_rec.QCertID)
				list.addUnique(lot_rec.QCertID);
		for(uint i = 0; i < list.getCount(); i++) {
			MEMSZERO(item);
			item.QCertID = list.at(i);
			if(Search(item.QCertID) > 0 && !P_Tbl->data.Passive) {
				STRNSCPY(item.Code, P_Tbl->data.Code);
				p_items->insert(&item);
			}
		}
		def = new StdListBoxDef(p_items, lbtDblClkNotify|lbtFocNotify|lbtDisposeData, MKSTYPE(S_ZSTRING, sizeof(item.Code)));
	}
	else {
		THROW(CheckTblPtr(t = new QualityCertTbl));
		q = & select(t->ID, t->Code, 0L).from(t, 0L).where(t->Passive == 0L).orderBy(t->Passive, t->Code, 0L);
		THROW(CheckQueryPtr(q));
		def = new DBQListBoxDef(*q, lbtDblClkNotify|lbtFocNotify|lbtDisposeData);
		THROW_MEM(def);
	}
	CATCH
		if(def)
			ZDELETE(def);
		else if(q == 0)
			ZDELETE(t);
		else
			ZDELETE(q);
	ENDCATCH
	return def;
}

int EditQCertDialog(QualityCertTbl::Rec * aRec, int viewOnly)
{
	int    ok = 1;
	int    r = 0, valid_data = 0;
	ushort v;
	QualityCertTbl::Rec rec = *aRec;
	TDialog * dlg = 0;
	THROW(CheckDialogPtr(&(dlg = new TDialog(DLG_QCERT))));
	dlg->SetupCalDate(CTLCAL_QCERT_INITDATE, CTL_QCERT_INITDATE);
	dlg->SetupCalDate(CTLCAL_QCERT_EXPIRY,   CTL_QCERT_EXPIRY);
	dlg->setCtrlData(CTL_QCERT_CODE,     rec.Code);
	dlg->setCtrlData(CTL_QCERT_BCODE,    rec.BlankCode);
	dlg->setCtrlData(CTL_QCERT_INNERCODE, rec.InnerCode);
	dlg->setCtrlData(CTL_QCERT_GOODS,    rec.GoodsName);
	dlg->setCtrlData(CTL_QCERT_MANUF,    rec.Manuf);
	dlg->setCtrlData(CTL_QCERT_SPRDATE,  rec.SPrDate);
	dlg->setCtrlData(CTL_QCERT_INITDATE, &rec.InitDate);
	dlg->setCtrlData(CTL_QCERT_EXPIRY,   &rec.Expiry);
	dlg->setCtrlData(CTL_QCERT_ETC,      rec.Etc);
	SetupPersonCombo(dlg, CTLSEL_QCERT_REGORGAN, rec.RegOrgan, OLW_CANINSERT, PPPRK_BUSADMIN, 0);
	v = rec.Passive ? 1 : 0;
	dlg->setCtrlData(CTL_QCERT_PASSIVE, &v);
	if(viewOnly)
		dlg->enableCommand(cmOK, 0);
	while(!valid_data && (r = ExecView(dlg)) == cmOK && !viewOnly) {
		dlg->getCtrlData(CTL_QCERT_CODE,        rec.Code);
		dlg->getCtrlData(CTL_QCERT_BCODE,       rec.BlankCode);
		dlg->getCtrlData(CTL_QCERT_INNERCODE,   rec.InnerCode);
		dlg->getCtrlData(CTL_QCERT_GOODS,       rec.GoodsName);
		dlg->getCtrlData(CTL_QCERT_MANUF,       rec.Manuf);
		dlg->getCtrlData(CTL_QCERT_SPRDATE,     rec.SPrDate);
		dlg->getCtrlData(CTL_QCERT_INITDATE,    &rec.InitDate);
		dlg->getCtrlData(CTL_QCERT_EXPIRY,      &rec.Expiry);
		dlg->getCtrlData(CTLSEL_QCERT_REGORGAN, &rec.RegOrgan);
		dlg->getCtrlData(CTL_QCERT_ETC,         rec.Etc);
		dlg->getCtrlData(CTL_QCERT_PASSIVE, &(v = 0));
		rec.Passive = BIN(v & 0x01);
		if(*strip(rec.Code) == 0)
			PPErrorByDialog(dlg, CTL_QCERT_CODE, PPERR_QCCODENEEDED);
		/*
		else if(*strip(rec.GoodsName) == 0) {
			PPError(PPERR_GOODSNEEDED, 0);
			dlg->selectCtrl(CTL_QCERT_GOODS);
		}
		*/
		else if(!rec.Expiry && PPMessage(mfConf|mfYesNo, PPCFM_QCEXPIRYNEEDED) != cmYes) {
			dlg->selectCtrl(CTL_QCERT_EXPIRY);
		}
		else if(rec.Expiry && !checkdate(&rec.Expiry))
			PPErrorByDialog(dlg, CTL_QCERT_EXPIRY, PPERR_QCEXPIRYNEEDED);
		else {
			valid_data = 1;
			*aRec = rec;
		}
	}
	CATCHZOKPPERR
	delete dlg;
	return ok ? r : 0;
}

int ViewQCertDialog(PPID id)
{
	if(id) {
		PPObjQCert qcobj(0);
		return (qcobj.Search(id) > 0) ? qcobj.Edit(&id, 0) : PPErrorZ();
	}
	else
		return -1;
}

int PPObjQCert::Edit(PPID * pID, void * extraPtr /*sampleID*/)
{
	const  PPID sample_id = reinterpret_cast<PPID>(extraPtr);
	int    ok = 1, r = 0;
	QualityCertTbl::Rec rec;
	THROW(CheckRightsModByID(pID));
	if(*pID || sample_id) {
		THROW(Search(sample_id ? sample_id : *pID, &rec) > 0);
		if(sample_id)
			rec.ID = 0;
	}
	else {
		SString temp_buf;
		// @v10.7.3 @ctr MEMSZERO(rec);
		PPLoadText(PPTXT_ONPACK, temp_buf);
		STRNSCPY(rec.SPrDate, temp_buf);
	}
	if((r = EditQCertDialog(&rec, 0)) == cmOK) {
		if(!PutPacket(pID, &rec, 1))
			PPError();
	}
	CATCHZOKPPERR
	return ok ? r : 0;
}

const char * PPObjQCert::GetNamePtr()
{
	return P_Tbl->data.Code;
}

int PPObjQCert::HandleMsg(int msg, PPID objType, PPID objID, void * extraPtr)
{
	QualityCertTbl::Key3 k;
	if(msg == DBMSG_OBJDELETE && objType == PPOBJ_PERSON) {
		k.RegOrgan = objID;
		k.InitDate = ZERODATE;
		if(P_Tbl->search(3, &k, spGe) && k.RegOrgan == objID)
			return RetRefsExistsErr(Obj, P_Tbl->data.ID);
		return (BTROKORNFOUND) ? DBRPL_OK : (PPSetErrorDB(), DBRPL_ERROR);
	}
	else if(msg == DBMSG_OBJREPLACE && objType == PPOBJ_PERSON) {
		if(!updateFor(P_Tbl, 0, (P_Tbl->RegOrgan == objID), set(P_Tbl->RegOrgan, dbconst(reinterpret_cast<long>(extraPtr))))) {
			return (PPSetErrorDB(), DBRPL_ERROR);
		}
	}
	return DBRPL_OK;
}

int PPObjQCert::Browse(void * extraPtr) { return PPView::Execute(PPVIEW_QCERT, 0, PPView::exefModeless, 0); }

int PPObjQCert::Read(PPObjPack * p, PPID id, void * stream, ObjTransmContext * pCtx)
{
	int    ok = 1;
	THROW_MEM(p->Data = new QualityCertTbl::Rec);
	if(stream == 0) {
		THROW(Search(id, p->Data) > 0);
	}
	else {
		SBuffer buffer;
		THROW_SL(buffer.ReadFromFile(static_cast<FILE *>(stream), 0));
		THROW_SL(P_Tbl->SerializeRecord(-1, p->Data, buffer, &pCtx->SCtx));
	}
	CATCHZOK
	return ok;
}

int PPObjQCert::Write(PPObjPack * p, PPID * pID, void * stream, ObjTransmContext * pCtx) // @srlz
{
	int    ok = 1;
	THROW(p && p->Data);
	if(stream == 0) {
		int    done = 0;
		QualityCertTbl::Rec * p_rec = static_cast<QualityCertTbl::Rec *>(p->Data);
		if(*pID == 0) {
			p_rec->ID = 0;
			strip(p_rec->Etc);
			char cod[64]; // #1
			STRNSCPY(cod, p_rec->Code);
			strip(cod);
			if(P_Tbl->search(1, cod, spEq))
				do {
					if(stricmp866(P_Tbl->data.Etc, p_rec->Etc) == 0) {
						*pID = P_Tbl->data.ID;
						done = 1;
					}
				} while(!done && P_Tbl->search(1, cod, spNext) && stricmp866(cod, P_Tbl->data.Code) == 0);
		}
		else
			p_rec->ID = *pID;
		if(!done) {
			if(!PutPacket(pID, p_rec, 1)) {
				pCtx->OutputAcceptErrMsg(PPTXT_ERRACCEPTQCERT, p_rec->ID, p_rec->Code);
				ok = -1;
			}
		}
	}
	else {
		SBuffer buffer;
		THROW_SL(P_Tbl->SerializeRecord(+1, p->Data, buffer, &pCtx->SCtx));
		THROW_SL(buffer.WriteToFile(static_cast<FILE *>(stream), 0, 0));
	}
	CATCHZOK
	return ok;
}

int PPObjQCert::ProcessObjRefs(PPObjPack * p, PPObjIDArray * ary, int replace, ObjTransmContext * pCtx)
{
	if(p && p->Data) {
		QualityCertTbl::Rec * r = static_cast<QualityCertTbl::Rec *>(p->Data);
		return (ProcessObjRefInArray(PPOBJ_PERSON, &r->RegOrgan, ary, replace)) ? 1 : 0;
	}
	return -1;
}
//
//
//
class SelQCertDialog : public TDialog {
public:
	SelQCertDialog() : TDialog(DLG_SELQCERT), IsSelByGoods(0), GoodsID(0)
	{
		P_List = static_cast<SmartListBox *>(getCtrlView(CTL_SELQCERT_LIST));
		// @v11.3.2 @obsolete setCtrlOption(CTL_SELQCERT_LIST, ofFramed, 1);
		updateList(0);
	}
	int    setSelectedItem(PPID);
	PPID   getSelectedItem();
private:
	DECL_HANDLE_EVENT;
	int    updateList(int);
	int    updateView(PPID);
	void   selByGoods();
	void   addItem();
	void   addBySample();
	void   editItem();
	void   delItem();

	SmartListBox * P_List;
	PPObjQCert QcObj;
	int    IsSelByGoods;
	PPID   GoodsID;
};

void SelQCertDialog::selByGoods()
{
	if(SelectGoods(GoodsID) > 0) {
		IsSelByGoods = 1;
		updateList(-1);
	}
}

int SelQCertDialog::setSelectedItem(PPID id)
{
	if(P_List) {
		P_List->TransmitData(+1, &id);
		if(P_List->HasState(SmartListBox::stDataFounded)) {
			//selectCtrl(CTL_SELQCERT_LIST);
			P_List->Draw_();
			return 1;
		}
	}
	return -1;
}

PPID SelQCertDialog::getSelectedItem()
{
	PPID   id = 0;
	return (P_List && P_List->getCurID(&id)) ? id : 0;
}

int SelQCertDialog::updateView(PPID id)
{
	if(P_List) {
		long   extra_param = IsSelByGoods ? GoodsID : 0;
		// @v11.1.10 QcObj.UpdateSelector(P_List->def, 0, reinterpret_cast<void *>(extra_param));
		QcObj.Selector(P_List->P_Def, 0, reinterpret_cast<void *>(extra_param)); // @v11.1.10
		if(id >= 0)
			P_List->Search_(&id, 0, srchFirst|lbSrchByID);
		P_List->Draw_();
	}
	return 1;
}

int SelQCertDialog::updateList(int)
{
	if(P_List) {
		long   extra_param = IsSelByGoods ? GoodsID : 0;
		ListBoxDef * p_def = QcObj.Selector(0, 0, reinterpret_cast<void *>(extra_param));
		if(p_def) {
			P_List->setDef(p_def);
			P_List->Draw_();
		}
	}
	return 1;
}

void SelQCertDialog::addItem()
{
	if(P_List) {
		PPID id = 0;
		if(QcObj.Edit(&id, 0) == cmOK)
			updateView(id);
	}
}

void SelQCertDialog::addBySample()
{
	if(P_List) {
		PPID id = 0, sample_id = 0;
		if(P_List->getCurID(&sample_id) && sample_id && QcObj.Edit(&id, reinterpret_cast<void *>(sample_id)) == cmOK)
			updateView(id);
	}
}

void SelQCertDialog::editItem()
{
	if(P_List) {
		PPID id = 0;
		if(P_List->getCurID(&id) && id && QcObj.Edit(&id, 0) == cmOK)
			updateView(id);
	}
}

void SelQCertDialog::delItem()
{
	PPID   id = 0;
	if(P_List && P_List->getCurID(&id) && id && QcObj.RemoveObjV(id, 0, PPObject::rmv_default, 0))
		updateView(-1);
}

IMPL_HANDLE_EVENT(SelQCertDialog)
{
	if(TVCOMMAND) {
		if(oneof2(TVCMD, cmOK, cmLBDblClk)) {
			PPID id = 0;
			if(!P_List || !P_List->getCurID(&id) || !id)
				clearEvent(event);
			else if(TVCMD == cmLBDblClk && event.isCtlEvent(CTL_SELQCERT_LIST))
				event.message.command = cmOK;
		}
	}
	TDialog::handleEvent(event);
	if(TVCOMMAND) {
		switch(TVCMD) {
			case cmaInsert: addItem(); break;
			case cmAddBySample: addBySample(); break;
			case cmaDelete: delItem(); break;
			case cmaEdit: editItem(); break;
			case cmSelQCertByGoods: selByGoods(); break;
			default: return;
		}
	}
	else if(TVKEYDOWN) {
		switch(TVKEY) {
			case kbIns: addItem(); break;
			case kbAltF2: addBySample(); break;
			case kbDel: delItem(); break;
			case kbF3: editItem(); break;
			case kbF2: selByGoods(); break;
			default: return;
		}
	}
	else
		return;
	clearEvent(event);
}
//
//
//
QCertCtrlGroup::Rec::Rec(PPID qCertID) : QCertID(qCertID)
{
}

QCertCtrlGroup::QCertCtrlGroup(uint ctlInput, uint selCmd) : CtrlGroup(), CtlInput(ctlInput), SelCmd(selCmd)
{
	// @v10.7.3 @ctr MEMSZERO(Data);
}

QCertCtrlGroup::~QCertCtrlGroup()
{
}

void QCertCtrlGroup::handleEvent(TDialog * pDlg, TEvent & event)
{
	if(TVCOMMAND) {
		if(TVCMD == SelCmd) {
			selectQCert(pDlg);
		}
	}
	else if(TVKEYDOWN) {
		if(pDlg->isCurrCtlID(CtlInput)) {
			if(TVKEY == kbDown)
				selectQCert(pDlg);
			else if(TVKEY == kbDel) {
				Data.QCertID = 0;
				setupQcNumber(pDlg);
			}
			else if(TVKEY == kbShiftF2) {
				if(Data.QCertID && QcObj.Search(Data.QCertID) > 0) {
					PPSetAddedMsgString(QcObj.P_Tbl->data.Code);
					if(CONFIRM(PPCFM_SAVEQCERT))
						DS.GetTLA().Lid.QCertID = Data.QCertID;
				}
			}
			else if(TVKEY == kbF2) {
				if(DS.GetTLA().Lid.QCertID) {
					Data.QCertID = DS.GetTLA().Lid.QCertID;
					setupQcNumber(pDlg);
				}
			}
		}
	}
}

void QCertCtrlGroup::setupQcNumber(TDialog * pDlg)
{
	char   code[32];
	if(QcObj.Search(Data.QCertID) > 0)
		STRNSCPY(code, QcObj.P_Tbl->data.Code);
	else
		code[0] = 0;
	pDlg->setCtrlData(CtlInput, code);
}

int QCertCtrlGroup::setData(TDialog * pDlg, void * pData)
{
	if(!RVALUEPTR(Data, static_cast<Rec *>(pData)))
		MEMSZERO(Data);
	setupQcNumber(pDlg);
	pDlg->setCtrlReadOnly(CtlInput, 1);
	return 1;
}

int QCertCtrlGroup::getData(TDialog * /*pDlg*/, void * pData)
{
	if(pData)
		*static_cast<Rec *>(pData) = Data;
	return 1;
}

void QCertCtrlGroup::selectQCert(TDialog * pDlg)
{
	SelQCertDialog * dlg = new SelQCertDialog();
	if(CheckDialogPtrErr(&dlg)) {
		dlg->setSelectedItem(Data.QCertID);
		if(ExecView(dlg) == cmOK) {
			Data.QCertID = dlg->getSelectedItem();
			setupQcNumber(pDlg);
		}
	}
	delete dlg;
}
//
// Implementation of PPALDD_QualityCert
//
PPALDD_CONSTRUCTOR(QualityCert)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		Extra[0].Ptr = new PPObjQCert;
	}
}

PPALDD_DESTRUCTOR(QualityCert)
{
	Destroy();
	delete static_cast<PPObjQCert *>(Extra[0].Ptr);
}

int PPALDD_QualityCert::InitData(PPFilt & rFilt, long rsrv)
{
	int    ok = -1;
	if(rFilt.ID == H.ID)
		ok = DlRtm::InitData(rFilt, rsrv);
	else {
		MEMSZERO(H);
		H.ID = rFilt.ID;
		QualityCertTbl::Rec rec;
		if(static_cast<PPObjQCert *>(Extra[0].Ptr)->Search(rFilt.ID, &rec) > 0) {
			H.ID = rec.ID;
			H.PersonID = rec.RegOrgan;
			STRNSCPY(H.Code, rec.Code);
			STRNSCPY(H.Blank, rec.BlankCode);
			STRNSCPY(H.InnerCode, rec.InnerCode);
			STRNSCPY(H.GoodsName, rec.GoodsName);
			STRNSCPY(H.ManufName, rec.Manuf);
			STRNSCPY(H.ProduceDate, rec.SPrDate);
			STRNSCPY(H.Etc, rec.Etc);
			H.Passive = rec.Passive;
			H.InitDate = rec.InitDate;
			H.Expiry = rec.Expiry;
			ok = DlRtm::InitData(rFilt, rsrv);
		}
	}
	return ok;
}
