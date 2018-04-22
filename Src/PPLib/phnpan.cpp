// PHNPAN.CPP
// Copyright (c) A.Sobolev 2018
//
#include <pp.h>
#pragma hdrstop

class SelectObjByPhoneDialog : public TDialog {
public:
	struct Param {
		Param() : ExtSelector(0)
		{
			Oid.Set(0, 0);
		}
		SString Phone;
		PPObjID Oid;
		PPID   ExtSelector;
		SString SearchStr;
	};
	SelectObjByPhoneDialog() : TDialog(DLG_SELOBJBYPHN)
	{
	}
	int setDTS(const Param * pData)
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
	int getDTS(Param * pData)
	{
		int    ok = 1;
		GetClusterData(CTL_SELOBJBYPHN_WHAT, &Data.Oid.Obj);
		Data.ExtSelector = getCtrlLong(CTLSEL_SELOBJBYPHN_EXT);
		if(Data.Oid.Obj == PPOBJ_PERSON) {
			Data.Oid.Id = getCtrlLong(CTL_SELOBJBYPHN_SRCH);
		}
		else if(Data.Oid.Obj == PPOBJ_SCARD) {
			Data.Oid.Id = getCtrlLong(CTL_SELOBJBYPHN_SRCH);
		}
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
			const PPID preserve_ext = Data.ExtSelector;
			Data.ExtSelector = getCtrlLong(CTLSEL_SELOBJBYPHN_EXT);
			if(Data.ExtSelector != preserve_ext && oneof2(Data.Oid.Obj, PPOBJ_PERSON, PPOBJ_SCARD)) {
				SetupCtrls();
			}
		}
		else
			return;
		clearEvent(event);
	}
	void   SetupCtrls()
	{
		const PPID preserve_obj_type = Data.Oid.Obj;
		GetClusterData(CTL_SELOBJBYPHN_WHAT, &Data.Oid.Obj);
		if(Data.Oid.Obj != preserve_obj_type)
			Data.ExtSelector = 0;
		switch(Data.Oid.Obj) {
			case PPOBJ_PERSON:
				disableCtrl(CTLSEL_SELOBJBYPHN_EXT, 0);
				SetupPPObjCombo(this, CTLSEL_SELOBJBYPHN_EXT, PPOBJ_PRSNKIND, Data.ExtSelector, 0);
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
					if(p_se) {
						SetupWordSelector(CTL_SELOBJBYPHN_SRCH, p_se, 0, 5, 0);
					}
				}
				break;
			default:
				disableCtrl(CTLSEL_SELOBJBYPHN_EXT, 1);
				ResetWordSelector(CTL_SELOBJBYPHN_SRCH);
				break;
		}
	}
	Param  Data;
};

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
		SLAPI  State() : Mode(lmNone), Status(0), PhnSvcID(0), PersonID(0), SCardID(0), LocID(0), SinceUp(getcurdatetime_()),
			SinceDown(ZERODATETIME)
		{
		}
		long   Mode;
		int    Status; // PhnSvcChannelStatus::stXXX
		LDATETIME SinceUp;   // Момент времени отсчета поднятой трубки
		LDATETIME SinceDown; // Момент времени отсчета опущенной трубки
		PPID   PhnSvcID;
		SString Channel;
		SString CallerID;
		SString ConnectedLine;
		SString BridgeID;
		//
		PPObjIDArray RelEntries;
		PPID   PersonID; // Персоналия ассоциированная с выбранным номером звонящего
		PPID   SCardID;  // Персональная карта ассоциированная с выбранным номером звонящего
		PPID   LocID;    // Локация ассоциированная с выбранным номером звонящего
	};
	int    SetupInfo()
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
			showCtrl(CTL_PHNCPANE_AUTOCLOSE, 0);
		}
		else {
			temp_buf.CatDivIfNotEmpty(';', 2).Cat("DOWN");
		}
		setStaticText(CTL_PHNCPANE_ST_INFO, temp_buf);
		if(S.Status != PhnSvcChannelStatus::stUp) {
			showCtrl(CTL_PHNCPANE_AUTOCLOSE, 1);
			if(!S.SinceDown) {
				SetClusterData(CTL_PHNCPANE_AUTOCLOSE, 1);
			}
			else {
				const   long time_to_close = 15;
				long s = diffdatetimesec(getcurdatetime_(), S.SinceDown);
				long sec_left = (s < time_to_close) ? (time_to_close - s) : 0;
				PPFormatS(PPSTR_TEXT, PPTXT_CLOSEWINAFTERXSEC, &temp_buf, sec_left);
				SetClusterItemText(CTL_PHNCPANE_AUTOCLOSE, 0, temp_buf);
				if(sec_left <= 0 && GetClusterData(CTL_PHNCPANE_AUTOCLOSE) == 1) {
					// (выбивает сеанс) messageCommand(this, cmClose);
					Sf |= sfCloseMe;
					result = 100;
				}
			}
		}
		return result;
	}
	void   Setup(PhoneServiceEventResponder * pPSER, const PhonePaneDialog::State * pSt)
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
				StrAssocArray name_list;
				PPID   init_id = 0;
				SString list_item_buf;
				if(P_PSER && P_PSER->IsConsistent())
					P_PSER->IdentifyCaller(S.ConnectedLine, OidList);
				else
					OidList.clear();
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
							LocationCore::GetExField(&loc_rec, LOCEXSTR_CONTACT, temp_buf);
							GetObjectTitle(r_oid.Obj, list_item_buf);
							list_item_buf.CatDiv(':', 2).Cat(temp_buf);
						}
					}
					else if(r_oid.Obj == PPOBJ_SCARD) {
						SCardTbl::Rec sc_rec;
						if(ScObj.Search(r_oid.Id, &sc_rec) > 0) {
							GetObjectTitle(r_oid.Obj, list_item_buf);
							list_item_buf.CatDiv(':', 2).Cat(sc_rec.Code);
							PersonTbl::Rec psn_rec;
							if(sc_rec.PersonID && PsnObj.Search(sc_rec.PersonID, &psn_rec) > 0) {
								list_item_buf.Space().Cat(psn_rec.Name);
							}
						}
					}
					if(list_item_buf.NotEmpty()) {
						name_list.Add(i+1, list_item_buf);
						if(init_id == 0)
							init_id = i+1;
					}
				}
				SetupStrAssocCombo(this, CTLSEL_PHNCPANE_NAME, &name_list, init_id, 0, 0, 0);
				OnContactSelection(1);
			}
		}
	}
	PhonePaneDialog(PhoneServiceEventResponder * pPSER, const PhonePaneDialog::State * pSt) : 
		TDialog(DLG_PHNCPANE), P_PSER(pPSER), P_Box(0), P_PhnSvcCli(0), ChnlStatusReqTmr(1000)
	{
		P_Box = (SmartListBox*)getCtrlView(CTL_PHNCPANE_INFOLIST);
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
	}
	~PhonePaneDialog()
	{
		ZDELETE(P_PhnSvcCli);
	}
	static PhonePaneDialog * FindAnalogue(const char * pChannel)
	{
		const long res_id = DLG_PHNCPANE;
		for(TView * p = APPL->P_DeskTop->GetFirstView(); p != 0; p = p->nextView()) {
			if(p->IsConsistent() && p->GetSubSign() == TV_SUBSIGN_DIALOG && ((TDialog *)p)->resourceID == res_id)
				return (PhonePaneDialog *)p;
		}
		return 0;
	}
private:
	DECL_HANDLE_EVENT
	{
		TDialog::handleEvent(event);
		if(TVBROADCAST && TVCMD == cmIdle) {
			if(ChnlStatusReqTmr.Check(0)) {
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
				if(!still_up) {
					S.Status = 0;
					if(!S.SinceDown)
						S.SinceDown = getcurdatetime_();
				}
				if(state == 0)
					S.Channel.Z();
				if(SetupInfo() == 100) {
					clearEvent(event);
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
		else if(event.isCmd(cmLBDblClk)) {
			if(event.isCtlEvent(CTL_PHNCPANE_INFOLIST) && P_Box) {
				if(S.Mode == S.lmSwitchTo && S.PhnSvcID) {
					long   ci = 0;
					if(P_Box->getCurID(&ci) && ci > 0 && P_PSER && P_PSER->IsConsistent()) {
						const StrAssocArray * p_internal_phone_list = P_PSER->GetInternalPhoneList();
						if(p_internal_phone_list && ci <= (long)p_internal_phone_list->getCount()) {
							StrAssocArray::Item item = p_internal_phone_list->Get(ci-1);
							if(!isempty(item.Txt)) {
								if(P_PhnSvcCli) {
									PhnSvcChannelStatusPool sp;
									P_PhnSvcCli->GetChannelStatus(S.Channel, sp);
									P_PhnSvcCli->Redirect(S.Channel, item.Txt, 0, 1);
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
	void   OnContactSelection(int onInit);
	void   ShowList(int mode, int onInit);
	void   NewContact()
	{
		SelectObjByPhoneDialog * dlg = new SelectObjByPhoneDialog;
		if(CheckDialogPtrErr(&dlg)) {
			const char * p_phone = S.ConnectedLine.cptr();
			SelectObjByPhoneDialog::Param param;
			param.Phone = p_phone;
			param.Oid.Set(PPOBJ_PERSON, 0);
			dlg->setDTS(&param);
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
					}
				}
				else if(param.Oid.Obj == PPOBJ_SCARD) {
				}
				else if(param.Oid.Obj == PPOBJ_LOCATION) {
				}
			}
		}
		delete dlg;
	}
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
	//long   SecToClose; // Количество секунд до авто-закрытия окна (при повешенной трубке)
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
		SLAPI  InfoListData() : SStrGroup()
		{
		}
	};
};

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
				InfoListData new_list;
				PPIDArray person_id_list;
				PPIDArray ar_id_list;
				person_id_list.add(S.PersonID);
				ArObj.GetByPersonList(0, &person_id_list, &ar_id_list);
				if(ar_id_list.getCount()) {
					DateRange period;
					period.Set(plusdate(getcurdate_(), -365), ZERODATE);
					for(uint i = 0; i < ar_id_list.getCount(); i++) {
						const PPID ar_id = ar_id_list.get(i);
						BillTbl::Rec bill_rec;
						for(DateIter di(&period); BillObj->P_Tbl->EnumByObj(ar_id, &di, &bill_rec) > 0;) {
							InfoListEntry new_entry;
							new_entry.ID = bill_rec.ID;
							new_entry.Dtm.d = bill_rec.Dt;
							new_entry.LocID = bill_rec.LocID;
							new_entry.OpID = bill_rec.OpID;
							new_entry.Amount = bill_rec.Amount;
							new_list.AddS(bill_rec.Code, &new_entry.CodeP);
							new_list.AddS(bill_rec.Memo, &new_entry.MemoP);
							new_list.insert(&new_entry);
						}
					}
				}
				if(new_list.getCount()) {
					for(uint ilidx = 0; ilidx < new_list.getCount(); ilidx++) {
						const InfoListEntry & r_entry = new_list.at(ilidx);
						ss.clear();
						ss.add(temp_buf.Z().Cat(r_entry.Dtm.d, MKSFMT(0, DATF_DMY)));
						new_list.GetS(r_entry.CodeP, temp_buf);
						ss.add(temp_buf);
						GetOpName(r_entry.OpID, temp_buf);
						ss.add(temp_buf);
						GetLocationName(r_entry.LocID, temp_buf);
						ss.add(temp_buf);
						ss.add(temp_buf.Z().Cat(r_entry.Amount, MKSFMTD(0, 2, 0)));
						ss.add(temp_buf.Z().Cat(r_entry.Debt, MKSFMTD(0, 2, 0)));
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
				P_Box->AddColumn(-1, "@time",        15, 0, 3);
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
							new_list.AddS(todo_rec.Descr, &new_entry.MemoP);
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
							new_list.AddS(todo_rec.Descr, &new_entry.MemoP);
							new_list.insert(&new_entry);
						}
					}
				}
				if(new_list.getCount()) {
					for(uint ilidx = 0; ilidx < new_list.getCount(); ilidx++) {
						const InfoListEntry & r_entry = new_list.at(ilidx);
						ss.clear();
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
				P_Box->AddColumn(-1, "@time",         15, 0, 2);
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
					new_list.AddS(pe_rec.Memo, &new_entry.MemoP);
					if(pe_rec.EstDuration)
						(new_entry.DueDtm = new_entry.Dtm).addsec(pe_rec.EstDuration * 3600 * 24);
					new_list.insert(&new_entry);
				}
				if(new_list.getCount()) {
					PPObjPsnOpKind pok_obj;
					PPPsnOpKind pok_rec;
					for(uint ilidx = 0; ilidx < new_list.getCount(); ilidx++) {
						const InfoListEntry & r_entry = new_list.at(ilidx);
						ss.clear();
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
				P_Box->AddColumn(-1, "@time",     10, 0, 2);
				P_Box->AddColumn(-1, "@amount",   10, 0, 3);
				P_Box->AddColumn(-1, "@rest",     10, 0, 4);
			}
			S.Mode = mode;
			if(S.SCardID) {
			}
		}
		else if(mode == State::lmScCCheck) {
			// columns: 
			if(onInit || S.Mode != mode) {
				P_Box->RemoveColumns();
				P_Box->AddColumn(-1, "@time",         10, 0, 2);
				P_Box->AddColumn(-1, "@posnode_s",     8, 0, 3);
				P_Box->AddColumn(-1, "@checkno",      10, 0, 4);
				P_Box->AddColumn(-1, "@amount",       10, ALIGN_RIGHT, 5);
				P_Box->AddColumn(-1, "@discount",     10, ALIGN_RIGHT, 6);
			}
			S.Mode = mode;
			if(S.SCardID) {
			}
		}
		else if(mode == State::lmLocCCheck) {
			// columns: 
			if(onInit || S.Mode != mode) {
				P_Box->RemoveColumns();
				P_Box->AddColumn(-1, "@time",         10, 0, 2);
				P_Box->AddColumn(-1, "@posnode_s",     8, 0, 3);
				P_Box->AddColumn(-1, "@checkno",      10, 0, 4);
				P_Box->AddColumn(-1, "@amount",       10, ALIGN_RIGHT, 5);
				P_Box->AddColumn(-1, "@discount",     10, ALIGN_RIGHT, 6);
			}
			S.Mode = mode;
			if(S.LocID) {
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
						ss.clear();
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
	S.PersonID = 0;
	S.SCardID = 0;
	S.LocID = 0;
	if(item_id && item_id > 0 && item_id <= (long)OidList.getCount()) {
		const PPObjID & r_oid = OidList.at(item_id-1);
		if(r_oid.Obj == PPOBJ_PERSON) {
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
			S.PersonID = r_oid.Id;
			if(acs_id_suppl) {
				ArObj.P_Tbl->PersonToArticle(r_oid.Id, acs_id_suppl, &ar_id_suppl);
			}
			if(acs_id_sell) {
				ArObj.P_Tbl->PersonToArticle(r_oid.Id, acs_id_sell, &ar_id_sell);
			}
			//DisableClusterItem(CTL_PHNCPANE_LISTMODE, 1, !(ar_id_suppl || ar_id_sell));
		}
		else if(r_oid.Obj == PPOBJ_LOCATION) {
			S.LocID = r_oid.Id;
			LocationTbl::Rec loc_rec;
			if(PsnObj.LocObj.Fetch(S.LocID, &loc_rec) > 0) {
				if(loc_rec.OwnerID)
					S.PersonID = loc_rec.OwnerID;
			}
		}
		else if(r_oid.Obj == PPOBJ_SCARD) {
			S.SCardID = r_oid.Id;
			SCardTbl::Rec sc_rec;
			if(ScObj.Fetch(S.SCardID, &sc_rec) > 0) {
				if(sc_rec.PersonID)
					S.PersonID = sc_rec.PersonID;
			}
		}
	}
	else {
		//DisableClusterItem(CTL_PHNCPANE_LISTMODE, 1, 1);
		//DisableClusterItem(CTL_PHNCPANE_LISTMODE, 2, 1);
		//DisableClusterItem(CTL_PHNCPANE_LISTMODE, 3, 1);
		//DisableClusterItem(CTL_PHNCPANE_LISTMODE, 4, 1);
		//DisableClusterItem(CTL_PHNCPANE_LISTMODE, 5, 1);
		//DisableClusterItem(CTL_PHNCPANE_LISTMODE, 6, 1);
	}
	{
		long   mode = 0;
		GetClusterData(CTL_PHNCPANE_LISTMODE, &mode);
		ShowList(mode, onInit);
	}
}

int SLAPI ShowPhoneCallPane(PhoneServiceEventResponder * pPSER, const PhonePaneDialog::State * pSt)
{
	int    ok = 1;
	PhonePaneDialog * p_prev_dlg = PhonePaneDialog::FindAnalogue("");
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

static const uint32 PhoneServiceEventResponder_Signature = 0x5A6B7C8E;

int SLAPI PhoneServiceEventResponder::IsConsistent() const
	{ return (Signature == PhoneServiceEventResponder_Signature); }

SLAPI PhoneServiceEventResponder::PhoneServiceEventResponder() : Signature(PhoneServiceEventResponder_Signature), 
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

SLAPI PhoneServiceEventResponder::~PhoneServiceEventResponder()
{
	DS.Unadvise(AdvCookie_Ringing);
	DS.Unadvise(AdvCookie_Up);
	ZDELETE(P_PsnObj);
	ZDELETE(P_InternalPhoneList);
	Signature = 0;
}

const StrAssocArray * SLAPI PhoneServiceEventResponder::GetInternalPhoneList()
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

int SLAPI PhoneServiceEventResponder::IdentifyCaller(const char * pCaller, PPObjIDArray & rList)
{
	rList.clear();
	int    ok = -1;
	SString caller_buf;
	PPEAddr::Phone::NormalizeStr(pCaller, caller_buf);
	THROW_SL(SETIFZ(P_PsnObj, new PPObjPerson));
	ok = P_PsnObj->LocObj.P_Tbl->SearchPhoneObjList(caller_buf, 0, rList);
	CATCHZOK
	return ok;
}

//static 
int PhoneServiceEventResponder::AdviseCallback(int kind, const PPNotifyEvent * pEv, void * procExtPtr)
{
	int    ok = -1;
	SString msg_buf;
	SString temp_buf;
	SString caller;
	SString channel;
	SString connected_line;
	SString bridge;
	if(kind == PPAdviseBlock::evPhoneRinging) {
		PhoneServiceEventResponder * p_self = (PhoneServiceEventResponder *)procExtPtr;
		if(p_self) {
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
						for(uint i = 0; contact_buf.Empty() && i < identified_caller_list.getCount(); i++) {
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
					p_win->Open(msg_buf, 0, /*H()*/0, 0, 5000, GetColorRef(SClrCadetblue),
						SMessageWindow::fTopmost|SMessageWindow::fSizeByText|SMessageWindow::fPreserveFocus, 0);
				}
			}
			ok = 1;
		}
	}
	else if(kind == PPAdviseBlock::evPhoneUp) {
		PhoneServiceEventResponder * p_self = (PhoneServiceEventResponder *)procExtPtr;
		if(p_self) {
			(msg_buf = "PhoneUp").CatDiv(':', 2);
			pEv->GetExtStrData(pEv->extssChannel, channel);
			msg_buf.CatEq("channel", channel).CatDiv(';', 2);
			pEv->GetExtStrData(pEv->extssCallerId, caller);
			msg_buf.CatEq("callerid", caller).CatDiv(';', 2);
			pEv->GetExtStrData(pEv->extssConnectedLineNum, connected_line);
			msg_buf.CatEq("connectedline", connected_line);
			pEv->GetExtStrData(pEv->extssBridgeId, bridge); // @v10.0.02
			msg_buf.CatEq("bridge", bridge); // @v10.0.02
			PPLogMessage(PPFILNAM_DEBUG_LOG, msg_buf, LOGMSGF_TIME);
			{
				PhonePaneDialog::State state;
				state.Status = PhnSvcChannelStatus::stUp;
				state.PhnSvcID = (pEv->ObjType == PPOBJ_PHONESERVICE) ? pEv->ObjID : 0;
				state.CallerID = caller;
				state.Channel = channel;
				state.ConnectedLine = connected_line;
				state.BridgeID = bridge; // @v10.0.02
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

IMPLEMENT_PPFILT_FACTORY(PhnSvcMonitor); SLAPI PhnSvcMonitorFilt::PhnSvcMonitorFilt() : PPBaseFilt(PPFILT_PHNSVCMONITOR, 0, 0)
{
	SetFlatChunk(offsetof(PhnSvcMonitorFilt, ReserveStart),
		offsetof(PhnSvcMonitorFilt, ReserveEnd)-offsetof(PhnSvcMonitorFilt, ReserveStart)+sizeof(ReserveEnd));
	Init(1, 0);
}

SLAPI PPViewPhnSvcMonitor::PPViewPhnSvcMonitor() : PPView(0, &Filt, PPVIEW_PHNSVCMONITOR), P_Cli(0), P_PsnObj(0)
{
	ImplementFlags |= implBrowseArray;
}

SLAPI PPViewPhnSvcMonitor::~PPViewPhnSvcMonitor()
{
	delete P_PsnObj;
}

PPBaseFilt * SLAPI PPViewPhnSvcMonitor::CreateFilt(void * extraPtr) const
{
	PhnSvcMonitorFilt * p_filt = new PhnSvcMonitorFilt;
	{
		PPEquipConfig eq_cfg;
		ReadEquipConfig(&eq_cfg);
		p_filt->PhnSvcID = eq_cfg.PhnSvcID;
	}
	return p_filt;
}

int SLAPI PPViewPhnSvcMonitor::EditBaseFilt(PPBaseFilt * pBaseFilt)
{
	return 1;
}

int SLAPI PPViewPhnSvcMonitor::CreatePhnSvcClient()
{
	int    ok = 1;
	PPObjPhoneService ps_obj(0);
	ZDELETE(P_Cli);
	THROW(Filt.PhnSvcID);
	P_Cli = ps_obj.InitAsteriskAmiClient(Filt.PhnSvcID);
	CATCHZOK
	return ok;
}

int SLAPI PPViewPhnSvcMonitor::Init_(const PPBaseFilt * pBaseFilt)
{
	int    ok = 1;
	THROW(Helper_InitBaseFilt(pBaseFilt));
	ZDELETE(P_Cli);
	THROW(CreatePhnSvcClient());
	THROW(Update());
	CATCHZOK
	return ok;
}

int SLAPI PPViewPhnSvcMonitor::Update()
{
	int    ok = -1;
	if(P_Cli) {
		int r = P_Cli->GetChannelStatus(0, List);
		if(!r) {
			r = CreatePhnSvcClient();
			if(r) {
				assert(P_Cli);
				P_Cli->GetChannelStatus(0, List);
			}
		}
		if(List.GetCount() && SETIFZ(P_PsnObj, new PPObjPerson)) {
			SString contact_buf;
			PPObjIDArray identified_caller_list;
			SString caller_buf;
			PhnSvcChannelStatus status_entry;
			for(uint i = 0; i < List.GetCount(); i++) {
				if(List.Get(i, status_entry) && status_entry.IdentifiedCallerName.Empty()) {
					PPEAddr::Phone::NormalizeStr(status_entry.ConnectedLineNum, caller_buf);
					identified_caller_list.clear();
					if(P_PsnObj->LocObj.P_Tbl->SearchPhoneObjList(caller_buf, 0, identified_caller_list) > 0) {
						contact_buf.Z();
						for(uint j = 0; contact_buf.Empty() && j < identified_caller_list.getCount(); j++) {
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
	}
	else
		List.Clear();
	return ok;
}

void SLAPI PPViewPhnSvcMonitor::PreprocessBrowser(PPViewBrowser * pBrw)
{
	if(pBrw) {
		pBrw->SetDefUserProc(PPViewPhnSvcMonitor::GetDataForBrowser, this);
		pBrw->SetRefreshPeriod(1);
		//pBrw->SetCellStyleFunc(CellStyleFunc, pBrw);
	}
}

int SLAPI PPViewPhnSvcMonitor::_GetDataForBrowser(SBrowserDataProcBlock * pBlk)
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
					// @v10.0.01 pBlk->Set(TempStatusEntry.CallerIdName.Transf(CTRANSF_UTF8_TO_INNER));
					pBlk->Set(TempStatusEntry.IdentifiedCallerName); // @v10.0.01
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

//static 
int SLAPI PPViewPhnSvcMonitor::GetDataForBrowser(SBrowserDataProcBlock * pBlk)
{
	PPViewPhnSvcMonitor * p_v = (PPViewPhnSvcMonitor *)pBlk->ExtraPtr;
	return p_v ? p_v->_GetDataForBrowser(pBlk) : 0;
}

SArray * SLAPI PPViewPhnSvcMonitor::CreateBrowserArray(uint * pBrwId, SString * pSubTitle)
{
	uint   brw_id = BROWSER_PHNSVCMONITOR;
	SArray * p_array = new TSArray <uint>; // Array - not Vector
	if(p_array) {
		for(uint i = 0; i < List.GetCount(); i++) {
			uint   pos = i+1;
			p_array->insert(&pos);
		}
	}
	ASSIGN_PTR(pBrwId, brw_id);
	return p_array;
}

int SLAPI PPViewPhnSvcMonitor::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	if(ok == -2 && oneof2(ppvCmd, PPVCMD_REFRESHBYPERIOD, PPVCMD_REFRESH)) {
		Update();
		AryBrowserDef * p_def = (AryBrowserDef *)pBrw->getDef();
		if(p_def) {
			SArray * p_array = CreateBrowserArray(0, 0);
			p_def->setArray(p_array, 0, 0);
		}
		ok = 1;
	}
	return ok;
}