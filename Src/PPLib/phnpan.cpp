// PHNPAN.CPP
// Copyright (c) A.Sobolev 2018, 2019, 2020, 2021, 2023, 2024, 2025
// @codepage UTF-8
//
#include <pp.h>
#pragma hdrstop

class PhonePaneDialog : public TDialog {
public:
	struct State {
		enum {
			lmNone = 0,
			lmBill,
			lmTask,
			lmPersonEvent,
			lmScOp,
			lmScCCheck,  // Чеки по карте
			lmLocCCheck, // Чеки по автономному адресу
			lmSwitchTo
		};
		enum {
			fLockAutoExit       = 0x0001,
			fCloseButtonInMinimizeState = 0x0002  // Кнопка "Закрыть" находится в состоянии "Свернуть"
		};
		State() : Mode(lmNone), Status(0), Flags(0), PhnSvcID(0), PersonID(0), SCardID(0), LocID(0), SinceUp(getcurdatetime_()),
			SinceDown(ZERODATETIME)
		{
		}
		long   Mode;
		int    Status; // PhnSvcChannelStatus::stXXX
		long   Flags;
		LDATETIME SinceUp;   // Момент времени отсчета поднятой трубки
		LDATETIME SinceDown; // Момент времени отсчета опущенной трубки
		PPID   PhnSvcID;
		SString Channel;
		SString CallerID;
		SString ConnectedLine;
		SString BridgeID;
		PPObjIDArray RelEntries;
		PPID   PersonID; // Персоналия ассоциированная с выбранным номером звонящего
		PPID   SCardID;  // Персональная карта ассоциированная с выбранным номером звонящего
		PPID   LocID;    // Локация ассоциированная с выбранным номером звонящего
	};
	PhonePaneDialog(PhoneServiceEventResponder * pPSER, const PhonePaneDialog::State * pSt);
	~PhonePaneDialog();
	int    SetupInfo();
	void   Setup(PhoneServiceEventResponder * pPSER, const PhonePaneDialog::State * pSt);
	void   SetupOidList(const PPObjID * pOidToSelect);
private:
	class ActionByPhoneDialog : public TDialog {
	public:
		struct Param {
			enum {
				acnUndef = 0,
				acnGoodsOrder,
				acnPurchase,
				acnPersonalEvent,
				acnPrjTask,
				acnCcOrder,
				acnViewPrcBusy
			};
			Param() : ExtSelector(0), Action(acnUndef), PersonID(0), SCardID(0), LocID(0)
			{
			}
			SString Phone;
			PPID   PersonID; // Персоналия ассоциированная с выбранным номером звонящего
			PPID   SCardID;  // Персональная карта ассоциированная с выбранным номером звонящего
			PPID   LocID;    // Локация ассоциированная с выбранным номером звонящего
			long   Action;
			PPID   ExtSelector;
		};
		DECL_DIALOG_DATA(Param);		

		ActionByPhoneDialog() : TDialog(DLG_SELACNBYPHN)
		{
		}
		DECL_DIALOG_SETDTS()
		{
			RVALUEPTR(Data, pData);
			PPID   buyer_ar_id = 0;
			PPID   suppl_ar_id = 0;
			SString temp_buf;
			setCtrlString(CTL_SELACNBYPHN_INFO, Data.Phone);
			AddClusterAssocDef(CTL_SELACNBYPHN_WHAT, 0, Param::acnGoodsOrder);
			AddClusterAssoc(CTL_SELACNBYPHN_WHAT, 1, Param::acnPurchase);
			AddClusterAssoc(CTL_SELACNBYPHN_WHAT, 2, Param::acnPersonalEvent);
			AddClusterAssoc(CTL_SELACNBYPHN_WHAT, 3, Param::acnPrjTask);
			AddClusterAssoc(CTL_SELACNBYPHN_WHAT, 4, Param::acnCcOrder);
			AddClusterAssoc(CTL_SELACNBYPHN_WHAT, 5, Param::acnViewPrcBusy);
			SetClusterData(CTL_SELACNBYPHN_WHAT, Data.Action);
			if(Data.PersonID) {
				GetPersonName(Data.PersonID, temp_buf);
				setCtrlString(CTL_SELACNBYPHN_PERSON, temp_buf);
				PPObjArticle ar_obj;
				ar_obj.P_Tbl->PersonToArticle(Data.PersonID, GetSellAccSheet(), &buyer_ar_id);
				ar_obj.P_Tbl->PersonToArticle(Data.PersonID, GetSupplAccSheet(), &suppl_ar_id);
			}
			else {
				showCtrl(CTL_SELACNBYPHN_PERSON, false);
			}
			if(Data.SCardID) {
				SCardTbl::Rec sc_rec;
				if(ScObj.Fetch(Data.SCardID, &sc_rec) > 0)
					temp_buf = sc_rec.Code;
				else
					temp_buf.Z();
				setCtrlString(CTL_SELACNBYPHN_SCARD, temp_buf);
			}
			else
				showCtrl(CTL_SELACNBYPHN_SCARD, false);
			if(Data.LocID) {
				LocationTbl::Rec loc_rec;
				if(PsnObj.LocObj.Fetch(Data.LocID, &loc_rec) > 0) {
					PsnObj.LocObj.MakeCodeString(&loc_rec, PPObjLocation::mcsDefault, temp_buf);
					setCtrlString(CTL_SELACNBYPHN_LOC, temp_buf);
				}
			}
			else
				showCtrl(CTL_SELACNBYPHN_LOC, false);
			DisableClusterItem(CTL_SELACNBYPHN_WHAT, 0, !buyer_ar_id);
			DisableClusterItem(CTL_SELACNBYPHN_WHAT, 1, !suppl_ar_id);
			DisableClusterItem(CTL_SELACNBYPHN_WHAT, 2, !Data.PersonID);
			DisableClusterItem(CTL_SELACNBYPHN_WHAT, 3, !Data.PersonID);
			SetupCtrls();
			return 1;
		}
		DECL_DIALOG_GETDTS()
		{
			int    ok = 1;
			GetClusterData(CTL_SELACNBYPHN_WHAT, &Data.Action);
			getCtrlData(CTLSEL_SELACNBYPHN_EXT, &Data.ExtSelector);
			ASSIGN_PTR(pData, Data);
			return ok;
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isClusterClk(CTL_SELACNBYPHN_WHAT)) {
				GetClusterData(CTL_SELACNBYPHN_WHAT, &Data.Action);
				SetupCtrls();
			}
			else
				return;
			clearEvent(event);
		}
		void   SetupCtrls()
		{
			if(Data.Action == Param::acnPersonalEvent) {
				disableCtrl(CTLSEL_SELACNBYPHN_EXT, 0);
				SetupPPObjCombo(this, CTLSEL_SELACNBYPHN_EXT, PPOBJ_PERSONOPKIND,  Data.ExtSelector = 0, 0);
			}
			else if(Data.Action == Param::acnCcOrder) {
				disableCtrl(CTLSEL_SELACNBYPHN_EXT, 0);
				PPObjCashNode::SelFilt f;
				f.LocID = 0;
				f.SyncGroup = 2; // only async nodes
				SetupPPObjCombo(this, CTLSEL_SELACNBYPHN_EXT, PPOBJ_CASHNODE,  Data.ExtSelector = 0, 0, &f);
			}
			else
				disableCtrl(CTLSEL_SELACNBYPHN_EXT, 1);
		}
		PPObjSCard ScObj;
		PPObjPerson PsnObj;
	};

	class SelectObjByPhoneDialog : public TDialog {
	public:
		struct Param {
			Param() : ExtSelector(0)
			{
			}
			SString Phone;
			PPObjID Oid;
			PPID   ExtSelector;
			SString SearchStr;
		};
		DECL_DIALOG_DATA(Param);

		SelectObjByPhoneDialog() : TDialog(DLG_SELOBJBYPHN)
		{
		}
		DECL_DIALOG_SETDTS()
		{
			RVALUEPTR(Data, pData);
			setCtrlString(CTL_SELOBJBYPHN_INFO, Data.Phone);
			AddClusterAssocDef(CTL_SELOBJBYPHN_WHAT, 0, PPOBJ_PERSON);
			AddClusterAssoc(CTL_SELOBJBYPHN_WHAT, 1, PPOBJ_SCARD);
			AddClusterAssoc(CTL_SELOBJBYPHN_WHAT, 2, PPOBJ_LOCATION); // автономный адрес
			SetClusterData(CTL_SELOBJBYPHN_WHAT, Data.Oid.Obj);
			SetupCtrls();
			return 1;
		}
		DECL_DIALOG_GETDTS()
		{
			int    ok = 1;
			GetClusterData(CTL_SELOBJBYPHN_WHAT, &Data.Oid.Obj);
			Data.ExtSelector = getCtrlLong(CTLSEL_SELOBJBYPHN_EXT);
			if(Data.Oid.Obj == PPOBJ_PERSON)
				Data.Oid.Id = getCtrlLong(CTL_SELOBJBYPHN_SRCH);
			else if(Data.Oid.Obj == PPOBJ_SCARD)
				Data.Oid.Id = getCtrlLong(CTL_SELOBJBYPHN_SRCH);
			ASSIGN_PTR(pData, Data);
			return ok;
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isClusterClk(CTL_SELOBJBYPHN_WHAT)) {
				SetupCtrls();
			}
			else if(event.isCbSelected(CTLSEL_SELOBJBYPHN_EXT)) {
				const  PPID preserve_ext = Data.ExtSelector;
				Data.ExtSelector = getCtrlLong(CTLSEL_SELOBJBYPHN_EXT);
				if(Data.ExtSelector != preserve_ext && oneof2(Data.Oid.Obj, PPOBJ_PERSON, PPOBJ_SCARD))
					SetupCtrls();
			}
			else
				return;
			clearEvent(event);
		}
		void   SetupCtrls()
		{
			const  PPID preserve_obj_type = Data.Oid.Obj;
			GetClusterData(CTL_SELOBJBYPHN_WHAT, &Data.Oid.Obj);
			if(Data.Oid.Obj != preserve_obj_type)
				Data.ExtSelector = 0;
			switch(Data.Oid.Obj) {
				case PPOBJ_PERSON:
					disableCtrl(CTLSEL_SELOBJBYPHN_EXT, 0);
					SetupPPObjCombo(this, CTLSEL_SELOBJBYPHN_EXT, PPOBJ_PERSONKIND, Data.ExtSelector, 0);
					{
						PersonSelExtra * p_se = new PersonSelExtra(0, Data.ExtSelector);
						if(p_se) {
							p_se->SetTextMode(false);
							SetupWordSelector(CTL_SELOBJBYPHN_SRCH, p_se, 0, 3, 0);
						}
					}
					break;
				case PPOBJ_SCARD:
					disableCtrl(CTLSEL_SELOBJBYPHN_EXT, 0);
					SetupPPObjCombo(this, CTLSEL_SELOBJBYPHN_EXT, PPOBJ_SCARDSERIES, Data.ExtSelector, 0);
					{
						SCardSelExtra * p_se = new SCardSelExtra(Data.ExtSelector);
						if(p_se)
							SetupWordSelector(CTL_SELOBJBYPHN_SRCH, p_se, 0, 5, 0);
					}
					break;
				default:
					disableCtrl(CTLSEL_SELOBJBYPHN_EXT, 1);
					ResetWordSelector(CTL_SELOBJBYPHN_SRCH);
					break;
			}
		}
	};

	DECL_HANDLE_EVENT;
	void   SelectContact(PPObjID oid, int onInit);
	void   OnContactSelection(int onInit);
	void   ShowList(int mode, int onInit);
	void   DoAction()
	{
		ActionByPhoneDialog * dlg = new ActionByPhoneDialog;
		if(CheckDialogPtrErr(&dlg)) {
			const char * p_phone = S.ConnectedLine.cptr();
			ActionByPhoneDialog::Param param;
			param.Phone = p_phone;
			param.PersonID = S.PersonID;
			param.SCardID = S.SCardID;
			param.LocID = S.LocID;
			dlg->setDTS(&param);
			if(ExecView(dlg) == cmOK) {
				if(dlg->getDTS(&param)) {
					switch(param.Action) {
						case ActionByPhoneDialog::Param::acnGoodsOrder:
							{
								PPID   new_bill_id = 0;
								PPIDArray op_list;
								GetOpList(PPOPT_GOODSORDER, &op_list);
								if(op_list.getCount()) {
									PPObjBill::AddBlock ab;
									ArObj.P_Tbl->PersonToArticle(param.PersonID, GetSellAccSheet(), &ab.ObjectID);
									if(ab.ObjectID) {
										PPAlbatrossConfig acfg;
										PPAlbatrosCfgMngr::Get(&acfg);
										ab.OpID = acfg.Hdr.OpID;
										SETIFZ(ab.OpID, op_list.getSingle());
										{
											S.Flags |= S.fLockAutoExit;
											if(ab.OpID || BillPrelude(&op_list, OPKLF_OPLIST, 0, &ab.OpID, &ab.LocID) > 0) {
												BillObj->AddGoodsBill(&new_bill_id, &ab);
											}
											S.Flags &= ~S.fLockAutoExit;
										}
									}
								}
							}
							break;
						case ActionByPhoneDialog::Param::acnPurchase:
							{
								PPID   new_bill_id = 0;
								PPIDArray op_list;
								GetOpList(PPOPT_DRAFTRECEIPT, &op_list);
								if(op_list.getCount()) {
									PPObjBill::AddBlock ab;
									ArObj.P_Tbl->PersonToArticle(param.PersonID, GetSupplAccSheet(), &ab.ObjectID);
									if(ab.ObjectID) {
										PPPredictConfig pr_cfg;
										PrcssrPrediction::GetPredictCfg(&pr_cfg);
										ab.OpID = pr_cfg.PurchaseOpID;
										SETIFZ(ab.OpID, op_list.getSingle());
										{
											S.Flags |= S.fLockAutoExit;
											if(ab.OpID || BillPrelude(&op_list, OPKLF_OPLIST, 0, &ab.OpID, &ab.LocID) > 0) {
												BillObj->AddGoodsBill(&new_bill_id, &ab);
											}
											S.Flags &= ~S.fLockAutoExit;
										}
									}
								}
							}
							break;
						case ActionByPhoneDialog::Param::acnPersonalEvent:
							if(param.ExtSelector && param.PersonID) {
								PPID   pe_id = 0;
								PPID   op_id = param.ExtSelector;
								PPPsnEventPacket pack;
								if(PeObj.InitPacket(&pack, op_id, param.PersonID)) {
									getcurdatetime(&pack.Rec.Dt, &pack.Rec.Tm);
									PsnEventDialog::Param pe_param;
									PsnEventDialog::GetParam(op_id, &pe_param);
									PsnEventDialog * p_dlg = new PsnEventDialog(&pe_param, &PeObj);
									if(CheckDialogPtrErr(&p_dlg)) {
										if(p_dlg->setDTS(&pack) > 0) {
											int    r = -1;
											S.Flags |= S.fLockAutoExit;
											while(r <= 0 && ExecView(p_dlg) == cmOK) {
												r = p_dlg->getDTS(&pack) ? 1 : PPErrorZ();
											}
											if(r > 0) {
												if(!PeObj.PutPacket(&pe_id, &pack, 1))
													PPError();
											}
											S.Flags &= ~S.fLockAutoExit;
										}
										else
											PPError();
									}
									ZDELETE(p_dlg);
								}
								else
									PPError();
							}
							break;
						case ActionByPhoneDialog::Param::acnPrjTask:
							if(param.PersonID) {
								PPPrjTaskPacket pack;
								PPID   emplr_id = 0;
								PPID   new_task_id = 0;
								PPObjPerson::GetCurUserPerson(&emplr_id, 0);
								if(TodoObj.InitPacket(&pack, 0, 0, param.PersonID, emplr_id, 1)) {
									S.Flags |= S.fLockAutoExit;
									if(TodoObj.EditDialog(&pack) > 0) {
										if(TodoObj.PutPacket(&new_task_id, &pack, 1)) {
										}
										else
											PPError();
									}
									S.Flags &= ~S.fLockAutoExit;
								}
								else
									PPError();
							}
							break;
						case ActionByPhoneDialog::Param::acnCcOrder:
							break;
						case ActionByPhoneDialog::Param::acnViewPrcBusy:
							{
								PPView * p_view = 0;
								if(PPView::Execute(PPVIEW_PRCBUSY, 0, PPView::exefModeless, &p_view, 0) && p_view) {
									PPViewPrcBusy::OuterContext ctx;
									ctx.PersonID = param.PersonID;
									ctx.SCardID = param.SCardID;
									ctx.Phone = param.Phone;
									static_cast<PPViewPrcBusy *>(p_view)->SetOuterContext(&ctx);
								}
							}
							break;
					}
				}
			}
		}
		delete dlg;
	}
	void   NewContact();
	int    QueryPhnState(int * pStillUp);
	State  S;
	SmartListBox * P_Box;
	PhoneServiceEventResponder * P_PSER;
	AsteriskAmiClient * P_PhnSvcCli;
	PPObjPerson PsnObj;
	PPObjPersonEvent PeObj;
	PPObjArticle ArObj;
	PPObjSCard ScObj;
	PPObjPrjTask TodoObj;
	PPObjIDArray OidList;
	SCycleTimer ChnlStatusReqTmr;
	//
	// Descr: Унифицированная структура для отображения различных данных в общем списке
	//   Применяется для отражения следующих типов данных: документы, задачи, персональные события,
	//   операции по персональным картам, кассовые чеки, списки телефонов для переключения вызова
	//
	struct InfoListEntry {
		InfoListEntry()
		{
			THISZERO();
		}
		PPID   ID;
		LDATETIME Dtm;
		LDATETIME DueDtm;
		PPID   OpID;
		PPID   LocID;
		PPID   ClientID;
		PPID   ExecutorID;
		uint   CodeP;
		uint   TextP;
		uint   MemoP;
		double Amount;
		double Debt;
	};
	class InfoListData : public SStrGroup, public TSVector <InfoListEntry> {
	public:
		InfoListData() : SStrGroup()
		{
		}
	};
};

PhonePaneDialog::PhonePaneDialog(PhoneServiceEventResponder * pPSER, const PhonePaneDialog::State * pSt) :
	TDialog(DLG_PHNCPANE), P_PSER(pPSER), P_Box(0), P_PhnSvcCli(0), ChnlStatusReqTmr(1000)
{
	P_Box = static_cast<SmartListBox *>(getCtrlView(CTL_PHNCPANE_INFOLIST));
	SetupStrListBox(P_Box);
	AddClusterAssoc(CTL_PHNCPANE_LISTMODE, 0, State::lmBill);
	AddClusterAssoc(CTL_PHNCPANE_LISTMODE, 1, State::lmTask);
	AddClusterAssoc(CTL_PHNCPANE_LISTMODE, 2, State::lmPersonEvent);
	AddClusterAssoc(CTL_PHNCPANE_LISTMODE, 3, State::lmScOp);
	AddClusterAssoc(CTL_PHNCPANE_LISTMODE, 4, State::lmScCCheck);
	AddClusterAssoc(CTL_PHNCPANE_LISTMODE, 5, State::lmLocCCheck);
	AddClusterAssoc(CTL_PHNCPANE_LISTMODE, 6, State::lmSwitchTo);
	AddClusterAssocDef(CTL_PHNCPANE_LISTMODE, 7, State::lmNone);
	AddClusterAssoc(CTL_PHNCPANE_AUTOCLOSE, 0, 1);
	Setup(pPSER, pSt);
	static_cast<PPApp *>(APPL)->LastCmd = cmPhonePane; // @V10.5.9
}

PhonePaneDialog::~PhonePaneDialog()
{
	ZDELETE(P_PhnSvcCli);
}

int PhonePaneDialog::SetupInfo()
{
	int   result = 1;
	SString temp_buf;
	setCtrlString(CTL_PHNCPANE_PHN, S.ConnectedLine);
	temp_buf = S.Channel;
	if(S.ConnectedLine.NotEmpty())
		temp_buf.CatDivIfNotEmpty(';', 2).Cat(S.ConnectedLine);
	if(S.Status == PhnSvcChannelStatus::stUp) {
		if(!!S.SinceUp) {
			long   up_time = diffdatetimesec(getcurdatetime_(), S.SinceUp);
			temp_buf.CatDivIfNotEmpty(';', 2).CatEq("UpTime", up_time);
		}
		showCtrl(CTL_PHNCPANE_AUTOCLOSE, false);
	}
	else {
		temp_buf.CatDivIfNotEmpty(';', 2).Cat("DOWN");
	}
	setCtrlString(CTL_PHNCPANE_ST_INFO, temp_buf);
	if(S.Status != PhnSvcChannelStatus::stUp) {
		showCtrl(CTL_PHNCPANE_AUTOCLOSE, true);
		if(!S.SinceDown) {
			SetClusterData(CTL_PHNCPANE_AUTOCLOSE, 1);
		}
		else {
			const   long time_to_close = 15;
			long s = diffdatetimesec(getcurdatetime_(), S.SinceDown);
			long sec_left = (s < time_to_close) ? (time_to_close - s) : 0;
			PPFormatS(PPSTR_TEXT, PPTXT_CLOSEWINAFTERXSEC, &temp_buf, sec_left);
			SetClusterItemText(CTL_PHNCPANE_AUTOCLOSE, 0, temp_buf);
			if(!(S.Flags & S.fLockAutoExit)) {
				if(sec_left <= 0 && GetClusterData(CTL_PHNCPANE_AUTOCLOSE) == 1) {
					// (выбивает сеанс) messageCommand(this, cmClose);
					Sf |= sfCloseMe;
					result = 100;
				}
			}
		}
	}
	return result;
}

void PhonePaneDialog::SetupOidList(const PPObjID * pOidToSelect)
{
	SString temp_buf;
	SString name_buf;
	StrAssocArray name_list;
	PPID   init_id = 0;
	SString list_item_buf;
	for(uint i = 0; i < OidList.getCount(); i++) {
		const PPObjID & r_oid = OidList.at(i);
		list_item_buf.Z();
		if(r_oid.Obj == PPOBJ_PERSON) {
			GetPersonName(r_oid.Id, temp_buf);
			GetObjectTitle(r_oid.Obj, list_item_buf);
			list_item_buf.CatDiv(':', 2).Cat(temp_buf);
		}
		else if(r_oid.Obj == PPOBJ_LOCATION) {
			LocationTbl::Rec loc_rec;
			if(PsnObj.LocObj.Search(r_oid.Id, &loc_rec) > 0) {
				name_buf.Z();
				temp_buf = loc_rec.Name;
				if(temp_buf.NotEmpty())
					name_buf.CatDivIfNotEmpty('-', 1).Cat(temp_buf);
				LocationCore::GetExField(&loc_rec, LOCEXSTR_CONTACT, temp_buf);
				if(temp_buf.NotEmpty())
					name_buf.CatDivIfNotEmpty('-', 1).Cat(temp_buf);
				GetObjectTitle(r_oid.Obj, list_item_buf);
				list_item_buf.CatDiv(':', 2).Cat(name_buf);
			}
		}
		else if(r_oid.Obj == PPOBJ_SCARD) {
			SCardTbl::Rec sc_rec;
			if(ScObj.Search(r_oid.Id, &sc_rec) > 0) {
				GetObjectTitle(r_oid.Obj, list_item_buf);
				list_item_buf.CatDiv(':', 2).Cat(sc_rec.Code);
				PersonTbl::Rec psn_rec;
				if(sc_rec.PersonID && PsnObj.Search(sc_rec.PersonID, &psn_rec) > 0)
					list_item_buf.Space().Cat(psn_rec.Name);
			}
		}
		if(list_item_buf.NotEmpty()) {
			name_list.Add(i+1, list_item_buf);
			if(pOidToSelect && !pOidToSelect->IsZero()) {
				if(*pOidToSelect == r_oid)
					init_id = i+1;
			}
			else
				SETIFZ(init_id, i+1);
		}
	}
	SetupStrAssocCombo(this, CTLSEL_PHNCPANE_NAME, name_list, init_id, 0, 0, 0);
}

void PhonePaneDialog::Setup(PhoneServiceEventResponder * pPSER, const PhonePaneDialog::State * pSt)
{
	if(pSt->Channel != S.Channel) {
		SString temp_buf;
		RVALUEPTR(S, pSt);
		ZDELETE(P_PhnSvcCli);
		if(S.Channel.NotEmpty()) {
			if(S.PhnSvcID) {
				PPObjPhoneService ps_obj(0);
				P_PhnSvcCli = ps_obj.InitAsteriskAmiClient(S.PhnSvcID);
			}
		}
		SetClusterData(CTL_PHNCPANE_LISTMODE, S.Mode);
		SetClusterData(CTL_PHNCPANE_AUTOCLOSE, 1);
		if(SetupInfo() == 100)
			return;
		else {
			if(P_PSER && P_PSER->IsConsistent())
				P_PSER->IdentifyCaller(S.ConnectedLine, OidList);
			else
				OidList.clear();
			SetupOidList(0);
			OnContactSelection(1);
		}
	}
}

int PhonePaneDialog::QueryPhnState(int * pStillUp)
{
	int    still_up = 0;
	int    state = 0;
	if(P_PhnSvcCli && S.Channel) {
		PhnSvcChannelStatusPool status_list;
		PhnSvcChannelStatus cnl_status;
		P_PhnSvcCli->GetChannelStatus(S.Channel, status_list);
		for(uint i = 0; !still_up && i < status_list.GetCount(); i++) {
			status_list.Get(i, cnl_status);
			if(cnl_status.Channel == S.Channel) {
				state = cnl_status.State;
				if(cnl_status.State == PhnSvcChannelStatus::stUp)
					still_up = 1;
			}
		}
	}
	ASSIGN_PTR(pStillUp, still_up);
	return state;
}

IMPL_HANDLE_EVENT(PhonePaneDialog)
{
	if(event.isCmd(cmClose)) {
		int    still_up = 0;
		int    state = QueryPhnState(&still_up);
		if(still_up) {
			::ShowWindow(H(), SW_MINIMIZE);
			clearEvent(event);
		}
		// else default processing is used by TDialog::handleEvent(event)
	}
	TDialog::handleEvent(event);
	if(TVBROADCAST && TVCMD == cmIdle) {
		if(ChnlStatusReqTmr.Check(0)) {
			int    still_up = 0;
			int    state = QueryPhnState(&still_up);
			if(!still_up) {
				S.Status = 0;
				if(!S.SinceDown)
					S.SinceDown = getcurdatetime_();
				if(S.Flags & S.fCloseButtonInMinimizeState) {
					TButton * p_button = static_cast<TButton *>(getCtrlView(STDCTL_CANCELBUTTON));
					if(p_button) {
						SString temp_buf;
						p_button->SetBitmap(PPDV_CANCEL01);
						PPLoadString("but_close", temp_buf);
						setButtonText(cmClose, temp_buf.Transf(CTRANSF_INNER_TO_OUTER));
						drawCtrl(STDCTL_CANCELBUTTON);
					}
					S.Flags &= ~S.fCloseButtonInMinimizeState;
				}
			}
			else {
				if(!(S.Flags & S.fCloseButtonInMinimizeState)) {
					TButton * p_button = static_cast<TButton *>(getCtrlView(STDCTL_CANCELBUTTON));
					if(p_button) {
						SString temp_buf;
						p_button->SetBitmap(PPDV_MINIMIZEWINLEFT01);
						PPLoadString("but_minimize", temp_buf);
						setButtonText(cmClose, temp_buf.Transf(CTRANSF_INNER_TO_OUTER));
						drawCtrl(STDCTL_CANCELBUTTON);
					}
					S.Flags |= S.fCloseButtonInMinimizeState;
				}
			}
			if(state == 0)
				S.Channel.Z();
			if(SetupInfo() == 100) {
				// @v10.5.9 clearEvent(event);
				return;
			}
		}
	}
	else if(event.isCbSelected(CTLSEL_PHNCPANE_NAME)) {
		OnContactSelection(0);
	}
	else if(event.isClusterClk(CTL_PHNCPANE_LISTMODE)) {
		long   mode = 0;
		GetClusterData(CTL_PHNCPANE_LISTMODE, &mode);
		ShowList(mode, 0);
	}
	else if(event.isCmd(cmNewContact)) {
		NewContact();
	}
	else if(event.isCmd(cmEditContact)) {
		if(S.PersonID) {
			PsnObj.Edit(&S.PersonID, 0);
		}
		else if(S.SCardID) {
			ScObj.Edit(&S.SCardID, 0);
		}
		else if(S.LocID) {
			PsnObj.LocObj.Edit(&S.LocID, 0);
		}
	}
	else if(event.isCmd(cmPhnCPaneAction)) {
		DoAction();
	}
	else if(event.isCmd(cmMinimizeWindow)) {
		::ShowWindow(H(), SW_MINIMIZE);
	}
	else if(event.isCmd(cmLBDblClk)) {
		if(event.isCtlEvent(CTL_PHNCPANE_INFOLIST) && P_Box) {
			if(S.Mode == S.lmSwitchTo && S.PhnSvcID) {
				long   ci = 0;
				if(P_Box->getCurID(&ci) && ci > 0 && P_PSER && P_PSER->IsConsistent()) {
					const StrAssocArray * p_internal_phone_list = P_PSER->GetInternalPhoneList();
					if(p_internal_phone_list && ci <= (long)p_internal_phone_list->getCount()) {
						StrAssocArray::Item item = p_internal_phone_list->Get(ci-1);
						PhnSvcChannelStatusPool sp;
						PhnSvcChannelStatus status;
						if(!isempty(item.Txt) && P_PhnSvcCli && P_PhnSvcCli->GetChannelListLinkedByBridge(S.Channel, sp) > 0) {
							for(uint i = 0; i < sp.GetCount(); i++) {
								if(sp.Get(i, status) && !status.Channel.IsEqiAscii(S.Channel)) {
									P_PhnSvcCli->Redirect(status.Channel, item.Txt, /*S.Channel*/0, 0, 1);
									//P_PhnSvcCli->Redirect(S.Channel, item.Txt, status.Channel, 0, 1);
									break;
								}
							}
						}
					}
				}
			}
		}
	}
	else
		return;
	clearEvent(event);
}

void PhonePaneDialog::NewContact()
{
	SelectObjByPhoneDialog * dlg = new SelectObjByPhoneDialog;
	if(CheckDialogPtrErr(&dlg)) {
		const char * p_phone = S.ConnectedLine.cptr();
		SelectObjByPhoneDialog::Param param;
		param.Phone = p_phone;
		param.Oid.Set(PPOBJ_PERSON, 0);
		dlg->setDTS(&param);
		S.Flags |= S.fLockAutoExit;
		if(ExecView(dlg) == cmOK) {
			dlg->getDTS(&param);
			if(param.Oid.Obj == PPOBJ_PERSON) {
				PersonTbl::Rec psn_rec;
				PPID   psn_id = 0;
				if(param.Oid.Id && PsnObj.Search(param.Oid.Id, &psn_rec) > 0) {
					psn_id = param.Oid.Id;
				}
				PPObjPerson::EditBlock eb;
				PsnObj.InitEditBlock(param.ExtSelector, eb);
				eb.InitPhone = p_phone;
				if(PsnObj.Edit_(&psn_id, eb) > 0) {
					OidList.Add(param.Oid.Obj, psn_id);
					PPObjID sel_oid(param.Oid.Obj, psn_id);
					SetupOidList(&sel_oid);
				}
			}
			else if(param.Oid.Obj == PPOBJ_SCARD) {
				PPID   sc_id = 0;
				PPObjSCard::AddParam ap;
				PPObjSCardSeries scs_obj;
				PPSCardSeries scs_rec;
				if(scs_obj.Fetch(param.ExtSelector, &scs_rec) > 0)
					ap.SerID = param.ExtSelector;
				ap.Phone = p_phone;
				if(ScObj.Edit(&sc_id, ap) > 0) {
					OidList.Add(param.Oid.Obj, sc_id);
					PPObjID sel_oid(param.Oid.Obj, sc_id);
					SetupOidList(&sel_oid);
				}
			}
			else if(param.Oid.Obj == PPOBJ_LOCATION) {
				PPID   loc_id = 0;
				PPLocationPacket loc_pack;
				loc_pack.Type = LOCTYP_ADDRESS;
				loc_pack.Flags |= LOCF_STANDALONE;
				LocationCore::SetExField(&loc_pack, LOCEXSTR_PHONE, p_phone);
				if(PsnObj.LocObj.EditDialog(LOCTYP_ADDRESS, &loc_pack, 0) > 0) {
					if(PsnObj.LocObj.PutPacket(&loc_id, &loc_pack, 1)) {
						OidList.Add(param.Oid.Obj, loc_id);
						PPObjID sel_oid(param.Oid.Obj, loc_id);
						SetupOidList(&sel_oid);
					}
					else
						PPError();
				}
			}
		}
	}
	delete dlg;
	S.Flags &= ~S.fLockAutoExit;
}

void PhonePaneDialog::ShowList(int mode, int onInit)
{
	if(P_Box) {
		SString temp_buf;
		StringSet ss(SLBColumnDelim);
		P_Box->freeAll();
		if(mode == State::lmNone) {
			if(onInit || S.Mode != mode) {
				P_Box->RemoveColumns();
			}
			S.Mode = mode;
		}
		else if(mode == State::lmBill) {
			// columns: id; date; code; warehouse; amount; debt
			if(onInit || S.Mode != mode) {
				P_Box->RemoveColumns();
				P_Box->AddColumn(-1, "@date",      10, 0, 2);
				P_Box->AddColumn(-1, "@code",      14, 0, 3);
				P_Box->AddColumn(-1, "@oprkind",   20, 0, 4);
				P_Box->AddColumn(-1, "@warehouse", 20, 0, 5);
				P_Box->AddColumn(-1, "@amount",     8, ALIGN_RIGHT, 6);
				P_Box->AddColumn(-1, "@debt",       8, ALIGN_RIGHT, 7);
			}
			S.Mode = mode;
			if(S.PersonID) {
				PPObjBill * p_bobj = BillObj;
				InfoListData new_list;
				PPIDArray person_id_list;
				PPIDArray ar_id_list;
				person_id_list.add(S.PersonID);
				ArObj.GetByPersonList(0, &person_id_list, &ar_id_list);
				if(ar_id_list.getCount()) {
					DateRange period;
					period.Set(plusdate(getcurdate_(), -365), ZERODATE);
					for(uint i = 0; i < ar_id_list.getCount(); i++) {
						const  PPID ar_id = ar_id_list.get(i);
						BillTbl::Rec bill_rec;
						for(DateIter di(&period); p_bobj->P_Tbl->EnumByObj(ar_id, &di, &bill_rec) > 0;) {
							InfoListEntry new_entry;
							new_entry.ID = bill_rec.ID;
							new_entry.Dtm.d = bill_rec.Dt;
							new_entry.LocID = bill_rec.LocID;
							new_entry.OpID = bill_rec.OpID;
							new_entry.Amount = bill_rec.Amount;
							new_list.AddS(bill_rec.Code, &new_entry.CodeP);
							// @v11.1.12 new_list.AddS(bill_rec.Memo, &new_entry.MemoP);
							p_bobj->P_Tbl->GetItemMemo(bill_rec.ID, temp_buf); // @v11.1.12
							new_list.AddS(temp_buf, &new_entry.MemoP); // @v11.1.12
							new_list.insert(&new_entry);
						}
					}
				}
				if(new_list.getCount()) {
					for(uint ilidx = 0; ilidx < new_list.getCount(); ilidx++) {
						const InfoListEntry & r_entry = new_list.at(ilidx);
						ss.Z();
						ss.add(temp_buf.Z().Cat(r_entry.Dtm.d, MKSFMT(0, DATF_DMY)));
						new_list.GetS(r_entry.CodeP, temp_buf);
						ss.add(temp_buf);
						GetOpName(r_entry.OpID, temp_buf);
						ss.add(temp_buf);
						GetLocationName(r_entry.LocID, temp_buf);
						ss.add(temp_buf);
						ss.add(temp_buf.Z().Cat(r_entry.Amount, MKSFMTD_020));
						ss.add(temp_buf.Z().Cat(r_entry.Debt, MKSFMTD_020));
						P_Box->addItem(r_entry.ID, ss.getBuf());
					}
				}
			}
		}
		else if(mode == State::lmTask) {
			// columns: id; datetime; code
			if(onInit || S.Mode != mode) {
				P_Box->RemoveColumns();
				P_Box->AddColumn(-1, "@code",        15, 0, 2);
				P_Box->AddColumn(-1, "@time",        20, 0, 3);
				P_Box->AddColumn(-1, "@duetime",     15, 0, 4);
				P_Box->AddColumn(-1, "@executor",    30, 0, 5);
				P_Box->AddColumn(-1, "@client",      30, 0, 6);
				P_Box->AddColumn(-1, "@description", 60, 0, 7);
			}
			S.Mode = mode;
			if(S.PersonID) {
				InfoListData new_list;
				DateRange period;
				period.Set(plusdate(getcurdate_(), -365), ZERODATE);
				PrjTaskTbl::Rec todo_rec;
				{
					for(SEnum en = TodoObj.P_Tbl->EnumByClient(S.PersonID, &period, 0); en.Next(&todo_rec) > 0;) {
						// Так как Enum заполняет не все поля в записи нам придется извляечь полную запись
						if(!oneof2(todo_rec.Status, TODOSTTS_REJECTED, TODOSTTS_COMPLETED) && TodoObj.Search(todo_rec.ID, &todo_rec) > 0) {
							InfoListEntry new_entry;
							new_entry.ID = todo_rec.ID;
							new_entry.Dtm.Set(todo_rec.Dt, todo_rec.Tm);
							new_entry.DueDtm.Set(todo_rec.EstFinishDt, todo_rec.EstFinishTm);
							new_entry.LocID = todo_rec.DlvrAddrID;
							new_entry.OpID = 0;
							new_entry.ClientID = todo_rec.ClientID;
							new_entry.ExecutorID = todo_rec.EmployerID;
							new_entry.Amount = todo_rec.Amount;
							new_list.AddS(todo_rec.Code, &new_entry.CodeP);
							TodoObj.GetItemDescr(todo_rec.ID, temp_buf);
							new_list.AddS(temp_buf, &new_entry.MemoP);
							new_list.insert(&new_entry);
						}
					}
				}
				{
					for(SEnum en = TodoObj.P_Tbl->EnumByEmployer(S.PersonID, &period, 0); en.Next(&todo_rec) > 0;) {
						// Так как Enum заполняет не все поля в записи нам придется извлечь полную запись
						if(!oneof2(todo_rec.Status, TODOSTTS_REJECTED, TODOSTTS_COMPLETED) && TodoObj.Search(todo_rec.ID, &todo_rec) > 0) {
							InfoListEntry new_entry;
							new_entry.ID = todo_rec.ID;
							new_entry.Dtm.Set(todo_rec.Dt, todo_rec.Tm);
							new_entry.DueDtm.Set(todo_rec.EstFinishDt, todo_rec.EstFinishTm);
							new_entry.LocID = todo_rec.DlvrAddrID;
							new_entry.OpID = 0;
							new_entry.ClientID = todo_rec.ClientID;
							new_entry.ExecutorID = todo_rec.EmployerID;
							new_entry.Amount = todo_rec.Amount;
							new_list.AddS(todo_rec.Code, &new_entry.CodeP);
							TodoObj.GetItemDescr(todo_rec.ID, temp_buf);
							new_list.AddS(temp_buf, &new_entry.MemoP);
							new_list.insert(&new_entry);
						}
					}
				}
				if(new_list.getCount()) {
					for(uint ilidx = 0; ilidx < new_list.getCount(); ilidx++) {
						const InfoListEntry & r_entry = new_list.at(ilidx);
						ss.Z();
						new_list.GetS(r_entry.CodeP, temp_buf);
						ss.add(temp_buf);
						ss.add(temp_buf.Z().Cat(r_entry.Dtm, MKSFMT(0, DATF_DMY), MKSFMT(0, TIMF_HM)));
						temp_buf.Z();
						if(!!r_entry.DueDtm)
							temp_buf.Cat(r_entry.DueDtm, MKSFMT(0, DATF_DMY), MKSFMT(0, TIMF_HM));
						ss.add(temp_buf);
						if(r_entry.ExecutorID)
							GetPersonName(r_entry.ExecutorID, temp_buf);
						else
							temp_buf.Z();
						ss.add(temp_buf);
						if(r_entry.ClientID)
							GetPersonName(r_entry.ClientID, temp_buf);
						else
							temp_buf.Z();
						ss.add(temp_buf);
						new_list.GetS(r_entry.MemoP, temp_buf);
						ss.add(temp_buf);
						P_Box->addItem(r_entry.ID, ss.getBuf());
					}
				}
			}
		}
		else if(mode == State::lmPersonEvent) {
			// columns:
			if(onInit || S.Mode != mode) {
				P_Box->RemoveColumns();
				P_Box->AddColumn(-1, "@time",         20, 0, 2);
				P_Box->AddColumn(-1, "@oprkind",      20, 0, 3);
				P_Box->AddColumn(-1, "@contractor",   40, 0, 5);
				P_Box->AddColumn(-1, "@memo",         60, 0, 6);
			}
			S.Mode = mode;
			if(S.PersonID) {
				InfoListData new_list;
				DateRange period;
				period.Set(plusdate(getcurdate_(), -365), ZERODATE);
				PersonEventTbl::Rec pe_rec;
				for(SEnum en = PeObj.P_Tbl->EnumByPerson(S.PersonID, &period); en.Next(&pe_rec) > 0;) {
					InfoListEntry new_entry;
					new_entry.ID = pe_rec.ID;
					new_entry.Dtm.Set(pe_rec.Dt, pe_rec.Tm);
					new_entry.OpID = pe_rec.OpID;
					new_entry.LocID = pe_rec.LocationID;
					new_entry.ClientID = pe_rec.SecondID; // ! person (not article)
					{
						// @v11.1.12 new_list.AddS(pe_rec.Memo, &new_entry.MemoP);
						PeObj.P_Tbl->GetItemMemo(pe_rec.ID, temp_buf); // @v11.1.12
						new_list.AddS(temp_buf, &new_entry.MemoP); // @v11.1.12
					}
					if(pe_rec.EstDuration)
						(new_entry.DueDtm = new_entry.Dtm).addsec(pe_rec.EstDuration * 3600 * 24);
					new_list.insert(&new_entry);
				}
				if(new_list.getCount()) {
					PPObjPsnOpKind pok_obj;
					PPPsnOpKind pok_rec;
					for(uint ilidx = 0; ilidx < new_list.getCount(); ilidx++) {
						const InfoListEntry & r_entry = new_list.at(ilidx);
						ss.Z();
						ss.add(temp_buf.Z().Cat(r_entry.Dtm, DATF_DMY, TIMF_HMS));
						if(pok_obj.Fetch(r_entry.OpID, &pok_rec) > 0)
							temp_buf = pok_rec.Name;
						else
							temp_buf.Z();
						ss.add(temp_buf);
						temp_buf.Z();
						if(r_entry.ClientID)
							GetPersonName(r_entry.ClientID, temp_buf);
						ss.add(temp_buf);
						new_list.GetS(r_entry.MemoP, temp_buf);
						ss.add(temp_buf);
						P_Box->addItem(r_entry.ID, ss.getBuf());
					}
				}
			}
		}
		else if(mode == State::lmScOp) {
			// columns:
			if(onInit || S.Mode != mode) {
				P_Box->RemoveColumns();
				P_Box->AddColumn(-1, "@time",     20, 0, 2);
				P_Box->AddColumn(-1, "@amount",   10, 0, 3);
				P_Box->AddColumn(-1, "@rest",     10, 0, 4);
			}
			S.Mode = mode;
			if(S.SCardID) {
				SCardOpFilt flt;
				SCardOpViewItem item;
				PPViewSCardOp   view;
				flt.SCardID = S.SCardID;
				if(view.Init_(&flt)) {
					view.InitIteration();
					for(uint i = 1; view.NextIteration(&item) > 0; i++) {
						ss.Z();
						LDATETIME dtm;
						dtm.Set(item.Dt, item.Tm);
						ss.add(temp_buf.Z().Cat(dtm, DATF_DMY, TIMF_HMS)); // time
						if(item.Flags & SCARDOPF_FREEZING) {
							DateRange frz_prd;
							frz_prd.Set(item.FreezingStart, item.FreezingEnd);
							ss.add(temp_buf.Z().Cat(frz_prd));
							ss.add(temp_buf.Z());
						}
						else {
							ss.add(temp_buf.Z().Cat(item.Amount, SFMT_MONEY|NMBF_NOZERO)); // Сумма
							ss.add(temp_buf.Z().Cat(item.Rest, SFMT_MONEY|NMBF_NOZERO));   // Остаток
						}
						P_Box->addItem(i+1, ss.getBuf());
						//addStringToList(i, ss.getBuf());
					}
				}
			}
		}
		else if(mode == State::lmScCCheck) {
			// columns:
			if(onInit || S.Mode != mode) {
				P_Box->RemoveColumns();
				if(!S.SCardID && S.PersonID)
					P_Box->AddColumn(-1, "@scard",    12, 0, 2);
				P_Box->AddColumn(-1, "@time",         20, 0, 2);
				P_Box->AddColumn(-1, "@posnode_s",     8, 0, 3);
				P_Box->AddColumn(-1, "@checkno",      10, 0, 4);
				P_Box->AddColumn(-1, "@amount",       10, ALIGN_RIGHT, 5);
				P_Box->AddColumn(-1, "@discount",     10, ALIGN_RIGHT, 6);
			}
			S.Mode = mode;
			if(S.SCardID) {
				CCheckFilt flt;
				CCheckViewItem item;
				PPViewCCheck view;
				flt.SCardID = S.SCardID;
				if(view.Init_(&flt)) {
					for(view.InitIteration(0); view.NextIteration(&item) > 0;) {
						if(!(item.Flags & CCHKF_SKIP)) {
							ss.Z();
							LDATETIME dtm;
							dtm.Set(item.Dt, item.Tm);
							ss.add(temp_buf.Z().Cat(dtm, DATF_DMY, TIMF_HMS)); // time
							ss.add(temp_buf.Z().Cat(item.PosNodeID));                         // Касса
							ss.add(temp_buf.Z().Cat(item.Code));                              // Номер чека
							ss.add(temp_buf.Z().Cat(MONEYTOLDBL(item.Amount), SFMT_MONEY));   // Сумма
							ss.add(temp_buf.Z().Cat(MONEYTOLDBL(item.Discount), SFMT_MONEY)); // Скидка
							P_Box->addItem(item.ID, ss.getBuf());
						}
					}
				}
			}
			else if(S.PersonID) {
				PPIDArray sc_list;
				ScObj.P_Tbl->GetListByPerson(S.PersonID, 0, &sc_list);
				if(sc_list.getCount()) {
					CCheckFilt flt;
					CCheckViewItem item;
					PPViewCCheck view;
					for(uint i = 0; i < sc_list.getCount(); i++) {
						const  PPID sc_id = sc_list.get(i);
						SCardTbl::Rec sc_rec;
						if(ScObj.Fetch(sc_id, &sc_rec) > 0) {
							flt.SCardID = sc_id;
							if(view.Init_(&flt)) {
								for(view.InitIteration(0); view.NextIteration(&item) > 0;) {
									if(!(item.Flags & CCHKF_SKIP)) {
										ss.Z();
										LDATETIME dtm;
										ss.add(temp_buf = sc_rec.Code); // scard code
										dtm.Set(item.Dt, item.Tm);
										ss.add(temp_buf.Z().Cat(dtm, DATF_DMY, TIMF_HMS)); // time
										ss.add(temp_buf.Z().Cat(item.PosNodeID));                         // Касса
										ss.add(temp_buf.Z().Cat(item.Code));                              // Номер чека
										ss.add(temp_buf.Z().Cat(MONEYTOLDBL(item.Amount), SFMT_MONEY));   // Сумма
										ss.add(temp_buf.Z().Cat(MONEYTOLDBL(item.Discount), SFMT_MONEY)); // Скидка
										P_Box->addItem(item.ID, ss.getBuf());
									}
								}
							}
						}
					}
				}
			}
		}
		else if(mode == State::lmLocCCheck) {
			// columns:
			if(onInit || S.Mode != mode) {
				P_Box->RemoveColumns();
				P_Box->AddColumn(-1, "@time",         20, 0, 2);
				P_Box->AddColumn(-1, "@posnode_s",     8, 0, 3);
				P_Box->AddColumn(-1, "@checkno",      10, 0, 4);
				P_Box->AddColumn(-1, "@amount",       10, ALIGN_RIGHT, 5);
				P_Box->AddColumn(-1, "@discount",     10, ALIGN_RIGHT, 6);
			}
			S.Mode = mode;
			if(S.LocID) {
				CCheckFilt flt;
				CCheckViewItem item;
				PPViewCCheck view;
				flt.DlvrAddrID = S.LocID;
				if(view.Init_(&flt)) {
					for(view.InitIteration(0); view.NextIteration(&item) > 0;) {
						if(!(item.Flags & CCHKF_SKIP)) {
							ss.Z();
							LDATETIME dtm;
							dtm.Set(item.Dt, item.Tm);
							ss.add(temp_buf.Z().Cat(dtm, DATF_DMY, TIMF_HMS)); // time
							ss.add(temp_buf.Z().Cat(item.PosNodeID));                         // Касса
							ss.add(temp_buf.Z().Cat(item.Code));                              // Номер чека
							ss.add(temp_buf.Z().Cat(MONEYTOLDBL(item.Amount), SFMT_MONEY));   // Сумма
							ss.add(temp_buf.Z().Cat(MONEYTOLDBL(item.Discount), SFMT_MONEY)); // Скидка
							P_Box->addItem(item.ID, ss.getBuf());
						}
					}
				}
			}
		}
		else if(mode == State::lmSwitchTo) {
			// columns:
			if(onInit || S.Mode != mode) {
				P_Box->RemoveColumns();
				P_Box->AddColumn(-1, "@name",        50, 0, 2);
				P_Box->AddColumn(-1, "@phone",       10, 0, 3);
			}
			S.Mode = mode;
			const StrAssocArray * p_internal_phone_list = (P_PSER && P_PSER->IsConsistent()) ? P_PSER->GetInternalPhoneList() : 0;
			if(p_internal_phone_list && p_internal_phone_list->getCount()) {
				PersonTbl::Rec psn_rec;
				for(uint ilidx = 0; ilidx < p_internal_phone_list->getCount(); ilidx++) {
					StrAssocArray::Item entry = p_internal_phone_list->at_WithoutParent(ilidx);
					if(entry.Id && !isempty(entry.Txt) && PsnObj.Fetch(entry.Id, &psn_rec) > 0) {
						ss.Z();
						ss.add(temp_buf.Z().Cat(psn_rec.Name).Strip());
						ss.add(temp_buf.Z().Cat(entry.Txt).Strip());
						P_Box->addItem(/*entry.Id*/ilidx+1, ss.getBuf());
					}
				}
			}
		}
		P_Box->Draw_();
	}
}

void PhonePaneDialog::SelectContact(PPObjID oid, int onInit)
{
	S.PersonID = 0;
	S.SCardID = 0;
	S.LocID = 0;
	if(oid.IsFullyDefined()) {
		if(oid.Obj == PPOBJ_PERSON) {
			//DisableClusterItem(CTL_PHNCPANE_LISTMODE, 2, 0);
			//DisableClusterItem(CTL_PHNCPANE_LISTMODE, 3, 0);
			//DisableClusterItem(CTL_PHNCPANE_LISTMODE, 4, 1);
			//DisableClusterItem(CTL_PHNCPANE_LISTMODE, 5, 1);
			//DisableClusterItem(CTL_PHNCPANE_LISTMODE, 6, 1);
			//
			PPID   acs_id_suppl = GetSupplAccSheet();
			PPID   acs_id_sell = GetSellAccSheet();
			PPID   ar_id_suppl = 0;
			PPID   ar_id_sell = 0;
			S.PersonID = oid.Id;
			if(acs_id_suppl) {
				ArObj.P_Tbl->PersonToArticle(oid.Id, acs_id_suppl, &ar_id_suppl);
			}
			if(acs_id_sell) {
				ArObj.P_Tbl->PersonToArticle(oid.Id, acs_id_sell, &ar_id_sell);
			}
			//DisableClusterItem(CTL_PHNCPANE_LISTMODE, 1, !(ar_id_suppl || ar_id_sell));
		}
		else if(oid.Obj == PPOBJ_LOCATION) {
			S.LocID = oid.Id;
			LocationTbl::Rec loc_rec;
			if(PsnObj.LocObj.Fetch(S.LocID, &loc_rec) > 0) {
				if(loc_rec.OwnerID)
					S.PersonID = loc_rec.OwnerID;
			}
		}
		else if(oid.Obj == PPOBJ_SCARD) {
			S.SCardID = oid.Id;
			SCardTbl::Rec sc_rec;
			if(ScObj.Fetch(S.SCardID, &sc_rec) > 0) {
				if(sc_rec.PersonID)
					S.PersonID = sc_rec.PersonID;
			}
		}
	}
	{
		long   mode = 0;
		GetClusterData(CTL_PHNCPANE_LISTMODE, &mode);
		ShowList(mode, onInit);
	}
}

void PhonePaneDialog::OnContactSelection(int onInit)
{
	long   item_id = getCtrlLong(CTLSEL_PHNCPANE_NAME);
	/*AddClusterAssocDef(CTL_PHNCPANE_LISTMODE, 0, State::lmSwitchTo);
	AddClusterAssoc(CTL_PHNCPANE_LISTMODE, 1, State::lmBill);
	AddClusterAssoc(CTL_PHNCPANE_LISTMODE, 2, State::lmTask);
	AddClusterAssoc(CTL_PHNCPANE_LISTMODE, 3, State::lmPersonEvent);
	AddClusterAssoc(CTL_PHNCPANE_LISTMODE, 4, State::lmScOp);
	AddClusterAssoc(CTL_PHNCPANE_LISTMODE, 5, State::lmScCCheck);
	AddClusterAssoc(CTL_PHNCPANE_LISTMODE, 6, State::lmLocCCheck);*/
	PPObjID oid;
	if(item_id && item_id > 0 && item_id <= OidList.getCountI())
		oid = OidList.at(item_id-1);
	else
		oid.Z();
	SelectContact(oid, onInit);
}

int OpenPhonePane()
{
	int    ok = -1;
	TWindow * p_phn_pane = static_cast<PPApp *>(APPL)->FindPhonePaneDialog(); 
	if(p_phn_pane) {
		::ShowWindow(p_phn_pane->H(), SW_NORMAL);
	}
	return ok;
}

int ShowPhoneCallPane(PhoneServiceEventResponder * pPSER, const PhonePaneDialog::State * pSt)
{
	int    ok = 1;
	PhonePaneDialog * p_prev_dlg = static_cast<PhonePaneDialog *>(static_cast<PPApp *>(APPL)->FindPhonePaneDialog());
	if(p_prev_dlg) {
		p_prev_dlg->Setup(pPSER, pSt);
		ok = 2;
	}
	else {
		PhonePaneDialog * p_dlg = new PhonePaneDialog(pPSER, pSt);
		if(CheckDialogPtr(&p_dlg)) {
			APPL->P_DeskTop->Insert_(p_dlg);
			p_dlg->Insert();
		}
	}
	return ok;
}

bool PhoneServiceEventResponder::IsConsistent() const { return (Signature == PPConst::Signature_PhoneServiceEventResponder); }

PhoneServiceEventResponder::PhoneServiceEventResponder() : Signature(PPConst::Signature_PhoneServiceEventResponder),
	AdvCookie_Ringing(0), AdvCookie_Up(0), P_PsnObj(0), P_InternalPhoneList(0)
{
	{
		PPAdviseBlock adv_blk;
		adv_blk.Kind = PPAdviseBlock::evPhoneRinging;
		adv_blk.ProcExtPtr = this;
		adv_blk.Proc = PhoneServiceEventResponder::AdviseCallback;
		DS.Advise(&AdvCookie_Ringing, &adv_blk);
	}
	{
		PPAdviseBlock adv_blk;
		adv_blk.Kind = PPAdviseBlock::evPhoneUp;
		adv_blk.ProcExtPtr = this;
		adv_blk.Proc = PhoneServiceEventResponder::AdviseCallback;
		DS.Advise(&AdvCookie_Up, &adv_blk);
	}
}

PhoneServiceEventResponder::~PhoneServiceEventResponder()
{
	DS.Unadvise(AdvCookie_Ringing);
	DS.Unadvise(AdvCookie_Up);
	ZDELETE(P_PsnObj);
	ZDELETE(P_InternalPhoneList);
	Signature = 0;
}

const StrAssocArray * PhoneServiceEventResponder::GetInternalPhoneList()
{
	if(!P_InternalPhoneList) {
		THROW_SL(P_InternalPhoneList = new StrAssocArray);
		{
			PPID   pk_id = 0;
			PPObjPersonKind pk_obj;
			PPPersonKind pk_rec;
			if(pk_obj.Fetch(PPPRK_EMPL, &pk_rec) > 0)
				pk_id = PPPRK_EMPL;
			THROW_SL(SETIFZ(P_PsnObj, new PPObjPerson));
			THROW(P_PsnObj->P_Tbl->GetELinkList(ELNKRT_INTERNALEXTEN, pk_id, *P_InternalPhoneList));
		}
	}
	CATCH
		ZDELETE(P_InternalPhoneList);
	ENDCATCH
	return P_InternalPhoneList;
}

int PhoneServiceEventResponder::IdentifyCaller(const char * pCaller, PPObjIDArray & rList)
{
	rList.clear();
	int    ok = -1;
	SString caller_buf;
	PPEAddr::Phone::NormalizeStr(pCaller, 0, caller_buf);
	THROW_SL(SETIFZ(P_PsnObj, new PPObjPerson));
	ok = P_PsnObj->LocObj.P_Tbl->SearchPhoneObjList(caller_buf, 0, rList);
	CATCHZOK
	return ok;
}

/*static*/int PhoneServiceEventResponder::AdviseCallback(int kind, const PPNotifyEvent * pEv, void * procExtPtr)
{
	int    ok = -1;
	SString msg_buf;
	SString temp_buf;
	SString caller;
	SString channel;
	SString connected_line;
	SString bridge;
	if(kind == PPAdviseBlock::evPhoneRinging) {
		PhoneServiceEventResponder * p_self = static_cast<PhoneServiceEventResponder *>(procExtPtr);
		if(p_self && p_self->IsConsistent()) {
			(msg_buf = "PhoneRinging").CatDiv(':', 2);
			pEv->GetExtStrData(pEv->extssChannel, channel);
			msg_buf.CatEq("channel", channel).CatDiv(';', 2);
			pEv->GetExtStrData(pEv->extssCallerId, caller);
			msg_buf.CatEq("callerid", caller).CatDiv(';', 2);
			pEv->GetExtStrData(pEv->extssConnectedLineNum, connected_line);
			msg_buf.CatEq("connectedline", connected_line);
			PPLogMessage(PPFILNAM_DEBUG_LOG, msg_buf, LOGMSGF_TIME);
			if(connected_line.NotEmpty()) {
				SMessageWindow * p_win = new SMessageWindow;
				if(p_win) {
					SString contact_buf;
					PPObjIDArray identified_caller_list;
					if(p_self->IdentifyCaller(connected_line, identified_caller_list) > 0) {
						for(uint i = 0; contact_buf.IsEmpty() && i < identified_caller_list.getCount(); i++) {
							const PPObjID & r_oid = identified_caller_list.at(i);
							if(r_oid.Obj == PPOBJ_PERSON) {
								GetPersonName(r_oid.Id, contact_buf);
							}
							else if(r_oid.Obj == PPOBJ_LOCATION) {
								LocationTbl::Rec loc_rec;
								if(p_self->P_PsnObj->LocObj.Search(r_oid.Id, &loc_rec) > 0)
									LocationCore::GetExField(&loc_rec, LOCEXSTR_CONTACT, contact_buf);
							}
						}
					}
					SString fmt_buf;
					PPLoadText(PPTXT_YOUARETELEPHONED, fmt_buf);
					temp_buf = connected_line;
					if(contact_buf.NotEmpty()) {
						temp_buf.Space().CatParStr(contact_buf);
					}
					msg_buf.Printf(fmt_buf, temp_buf.cptr());
					p_win->Open(msg_buf, 0, /*H()*/0, 0, 5000, /*GetColorRef(SClrCadetblue)*/RGB(0x8E, 0xE4, 0xAF),
						SMessageWindow::fTopmost|SMessageWindow::fSizeByText|SMessageWindow::fPreserveFocus|SMessageWindow::fLargeText, 0);
				}
			}
			ok = 1;
		}
	}
	else if(kind == PPAdviseBlock::evPhoneUp) {
		PhoneServiceEventResponder * p_self = static_cast<PhoneServiceEventResponder *>(procExtPtr);
		if(p_self && p_self->IsConsistent()) {
			(msg_buf = "PhoneUp").CatDiv(':', 2);
			pEv->GetExtStrData(pEv->extssChannel, channel);
			msg_buf.CatEq("channel", channel).CatDiv(';', 2);
			pEv->GetExtStrData(pEv->extssCallerId, caller);
			msg_buf.CatEq("callerid", caller).CatDiv(';', 2);
			pEv->GetExtStrData(pEv->extssConnectedLineNum, connected_line);
			msg_buf.CatEq("connectedline", connected_line);
			pEv->GetExtStrData(pEv->extssBridgeId, bridge);
			msg_buf.CatEq("bridge", bridge);
			PPLogMessage(PPFILNAM_DEBUG_LOG, msg_buf, LOGMSGF_TIME);
			{
				PhonePaneDialog::State state;
				state.Status = PhnSvcChannelStatus::stUp;
				state.PhnSvcID = (pEv->ObjType == PPOBJ_PHONESERVICE) ? pEv->ObjID : 0;
				state.CallerID = caller;
				state.Channel = channel;
				state.ConnectedLine = connected_line;
				state.BridgeID = bridge;
				p_self->IdentifyCaller(caller, state.RelEntries);
				ShowPhoneCallPane(p_self, &state);
			}
			ok = 1;
		}
	}
	return ok;
}
//
// @ModuleDef(PPViewJobPool)
//
//Event: Status;Privilege: Call;Channel: SIP/198-0000027c;ChannelState: 6;ChannelStateDesc: Up;CallerIDNum: 198;CallerIDName: Sobolev (soft sip);
	// ConnectedLineNum: 110;ConnectedLineName: Соболев Антон;Accountcode: ;Context: from-internal;Exten: ;Priority: 1;Uniqueid: 1520526041.660;Type: SIP;DNID: ;
	// EffectiveConnectedLineNum: 110;EffectiveConnectedLineName: Соболев Антон;TimeToHangup: 0;BridgeID: 89e33c3e-f276-45a3-b66e-b0a5fda6fa4e;
	// Linkedid: 1520526041.659;Application: AppDial;Data: (Outgoing Line);Nativeformats: (ulaw);Readformat: ulaw;Readtrans: ;Writeformat: ulaw;
	// Writetrans: ;Callgroup: 0;Pickupgroup: 0;Seconds: 12;ActionID: 2899;;
//Event: Status;Privilege: Call;Channel: SIP/110-0000027b;ChannelState: 6;ChannelStateDesc: Up;CallerIDNum: 110;CallerIDName: Соболев Антон;ConnectedLineNum: 198;ConnectedLineName: Sobolev (soft sip);Accountcode: ;Context: macro-dial-one;Exten: s;Priority: 52;Uniqueid: 1520526041.659;Type: SIP;DNID: 198;EffectiveConnectedLineNum: 198;EffectiveConnectedLineName: Sobolev (soft sip);TimeToHangup: 0;BridgeID: 89e33c3e-f276-45a3-b66e-b0a5fda6fa4e;Linkedid: 1520526041.659;Application: Dial;Data: SIP/198,,TtrIb(func-apply-sipheaders^s^1);Nativeformats: (ulaw);Readformat: ulaw;Readtrans: ;Writeformat: ulaw;Writetrans: ;Callgroup: 0;Pickupgroup: 0;Seconds: 12;ActionID: 2899;;

IMPLEMENT_PPFILT_FACTORY(PhnSvcMonitor); PhnSvcMonitorFilt::PhnSvcMonitorFilt() : PPBaseFilt(PPFILT_PHNSVCMONITOR, 0, 0)
{
	SetFlatChunk(offsetof(PhnSvcMonitorFilt, ReserveStart),
		offsetof(PhnSvcMonitorFilt, ReserveEnd)-offsetof(PhnSvcMonitorFilt, ReserveStart)+sizeof(ReserveEnd));
	Init(1, 0);
}

PPViewPhnSvcMonitor::PPViewPhnSvcMonitor() : PPView(0, &Filt, PPVIEW_PHNSVCMONITOR, implBrowseArray, 0), P_Cli(0), P_PsnObj(0)
{
}

PPViewPhnSvcMonitor::~PPViewPhnSvcMonitor()
{
	delete P_PsnObj;
}

PPBaseFilt * PPViewPhnSvcMonitor::CreateFilt(const void * extraPtr) const
{
	PhnSvcMonitorFilt * p_filt = new PhnSvcMonitorFilt;
	{
		PPEquipConfig eq_cfg;
		ReadEquipConfig(&eq_cfg);
		p_filt->PhnSvcID = eq_cfg.PhnSvcID;
	}
	return p_filt;
}

int PPViewPhnSvcMonitor::EditBaseFilt(PPBaseFilt * pBaseFilt) { return 1; }

int PPViewPhnSvcMonitor::CreatePhnSvcClient()
{
	int    ok = 1;
	PPObjPhoneService ps_obj(0);
	ZDELETE(P_Cli);
	THROW(Filt.PhnSvcID);
	P_Cli = ps_obj.InitAsteriskAmiClient(Filt.PhnSvcID);
	CATCHZOK
	return ok;
}

int PPViewPhnSvcMonitor::Init_(const PPBaseFilt * pBaseFilt)
{
	int    ok = 1;
	THROW(Helper_InitBaseFilt(pBaseFilt));
	ZDELETE(P_Cli);
	THROW(CreatePhnSvcClient());
	THROW(Update());
	CATCHZOK
	return ok;
}

int PPViewPhnSvcMonitor::Update()
{
	int    ok = -1;
	if(P_Cli) {
		ENTER_CRITICAL_SECTION
		if(!P_Cli->GetChannelStatus(0, List)) {
			for(uint tn = 0; tn < 3; tn++) {
				if(CreatePhnSvcClient() && P_Cli) {
					P_Cli->GetChannelStatus(0, List);
					break;
				}
			}
		}
		if(List.GetCount() && SETIFZ(P_PsnObj, new PPObjPerson)) {
			SString contact_buf;
			PPObjIDArray identified_caller_list;
			SString caller_buf;
			PhnSvcChannelStatus status_entry;
			for(uint i = 0; i < List.GetCount(); i++) {
				if(List.Get(i, status_entry) && status_entry.IdentifiedCallerName.IsEmpty()) {
					PPEAddr::Phone::NormalizeStr(status_entry.ConnectedLineNum, 0, caller_buf);
					identified_caller_list.clear();
					if(P_PsnObj->LocObj.P_Tbl->SearchPhoneObjList(caller_buf, 0, identified_caller_list) > 0) {
						contact_buf.Z();
						for(uint j = 0; contact_buf.IsEmpty() && j < identified_caller_list.getCount(); j++) {
							const PPObjID & r_oid = identified_caller_list.at(j);
							if(r_oid.Obj == PPOBJ_PERSON) {
								GetPersonName(r_oid.Id, contact_buf);
							}
							else if(r_oid.Obj == PPOBJ_LOCATION) {
								LocationTbl::Rec loc_rec;
								if(P_PsnObj->LocObj.Search(r_oid.Id, &loc_rec) > 0)
									LocationCore::GetExField(&loc_rec, LOCEXSTR_CONTACT, contact_buf);
							}
						}
						List.SetIdentifiedCallerName(i, contact_buf);
					}
				}
			}
		}
		LEAVE_CRITICAL_SECTION
	}
	else
		List.Z();
	return ok;
}

void PPViewPhnSvcMonitor::PreprocessBrowser(PPViewBrowser * pBrw)
{
	if(pBrw) {
		pBrw->SetDefUserProc(PPViewPhnSvcMonitor::GetDataForBrowser, this);
		pBrw->SetRefreshPeriod(1);
		//pBrw->SetCellStyleFunc(CellStyleFunc, pBrw);
	}
}

int PPViewPhnSvcMonitor::_GetDataForBrowser(SBrowserDataProcBlock * pBlk)
{
	int    ok = 0;
	if(pBlk->P_SrcData && pBlk->P_DestData) {
		ok = 1;
		uint   _pos = *(uint *)pBlk->P_SrcData;
		if(_pos > 0 && _pos <= List.GetCount()) {
			List.Get(_pos-1, TempStatusEntry);
			switch(pBlk->ColumnN) {
				case 0: pBlk->Set((long)_pos); break; // @id
				case 1: pBlk->Set(TempStatusEntry.Channel); break; // Channel
				case 2: // State
					{
						SString & r_temp_buf = SLS.AcquireRvlStr();
						AsteriskAmiClient::GetStateText(TempStatusEntry.State, r_temp_buf);
						pBlk->Set(r_temp_buf);
					}
					break;
				case 3: pBlk->Set(TempStatusEntry.Priority); break; // Priority
				case 4: pBlk->Set(TempStatusEntry.Seconds); break; // Seconds
				case 5: pBlk->Set(TempStatusEntry.TimeToHungUp); break; // TimeToHungUp
				case 6: pBlk->Set(TempStatusEntry.CallerId); break; // CallerId
				case 7: // CallerName
					pBlk->Set(TempStatusEntry.IdentifiedCallerName);
					break;
				case 8: pBlk->Set(TempStatusEntry.ConnectedLineNum); break; // ConnectedLineNum
				case 9: pBlk->Set(TempStatusEntry.ConnectedLineName.Transf(CTRANSF_UTF8_TO_INNER)); break; // ConnectedLineName
				case 10: pBlk->Set(TempStatusEntry.EffConnectedLineNum); break; // EffConnectedLineNum
				case 11: pBlk->Set(TempStatusEntry.EffConnectedLineName.Transf(CTRANSF_UTF8_TO_INNER)); break; // EffConnectedLineName
				case 12: pBlk->Set(TempStatusEntry.Context.Transf(CTRANSF_UTF8_TO_INNER)); break; // Context
				case 13: pBlk->Set(TempStatusEntry.Exten); break; // Exten
				case 14: pBlk->Set(TempStatusEntry.DnId); break; // DnId
				case 15: pBlk->Set(TempStatusEntry.Application.Transf(CTRANSF_UTF8_TO_INNER)); break; // Application
				case 16: pBlk->Set(TempStatusEntry.Data.Transf(CTRANSF_UTF8_TO_INNER)); break; // Data
				case 17: pBlk->Set(TempStatusEntry.BridgeId); break; // BridgeId
			}
		}
	}
	return ok;
}

/*static*/int FASTCALL PPViewPhnSvcMonitor::GetDataForBrowser(SBrowserDataProcBlock * pBlk)
{
	PPViewPhnSvcMonitor * p_v = static_cast<PPViewPhnSvcMonitor *>(pBlk->ExtraPtr);
	return p_v ? p_v->_GetDataForBrowser(pBlk) : 0;
}

SArray * PPViewPhnSvcMonitor::CreateBrowserArray(uint * pBrwId, SString * pSubTitle)
{
	SArray * p_array = new TSArray <uint>; // Array - not Vector
	if(p_array) {
		for(uint i = 0; i < List.GetCount(); i++) {
			uint   pos = i+1;
			p_array->insert(&pos);
		}
	}
	ASSIGN_PTR(pBrwId, BROWSER_PHNSVCMONITOR);
	return p_array;
}

int PPViewPhnSvcMonitor::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	if(ok == -2 && oneof2(ppvCmd, PPVCMD_REFRESHBYPERIOD, PPVCMD_REFRESH)) {
		Update();
		AryBrowserDef * p_def = static_cast<AryBrowserDef *>(pBrw->getDef());
		if(p_def) {
			SArray * p_array = CreateBrowserArray(0, 0);
			p_def->setArray(p_array, 0, 0);
		}
		ok = 1;
	}
	return ok;
}
