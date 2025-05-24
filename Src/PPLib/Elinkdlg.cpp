// ELINKDLG.CPP
// Copyright (c) A.Sobolev 1998, 1999, 2000, 2001, 2002, 2003, 2006, 2007, 2009, 2015, 2016, 2017, 2018, 2019, 2020, 2022, 2024, 2025
// @codepage UTF-8
// Диалоги редактирования электронных адресов
//
#include <pp.h>
#pragma hdrstop

int EditELink(PPELink * pLink)
{
	class ELinkDialog : public TDialog {
		DECL_DIALOG_DATA(PPELink);
		PPObjELinkKind ElkObj;

		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isCbSelected(CTLSEL_ELINK_KIND)) {
				getCtrlData(CTLSEL_ELINK_KIND, &Data.KindID);
			}
			else if(event.isCmd(cmInputUpdated) && event.isCtlEvent(CTL_ELINK_ADDR)) {
				PPELinkKind elk_rec;
				SString info_buf;
				if(ElkObj.Fetch(Data.KindID, &elk_rec) > 0) {
					if(elk_rec.Type == ELNKRT_PHONE) {
						SString addr;
						getCtrlString(CTL_ELINK_ADDR, addr);
						if(addr.Len()) {
							addr.Transf(CTRANSF_INNER_TO_UTF8).Utf8ToLower();
							PPEAddr::Phone::NormalizeStr(addr, 0, info_buf);
						}
					}
				}
				setCtrlString(CTL_ELINK_INFO, info_buf);
			}	
			else if(event.isKeyDown(kbF2)) { // test
				SString addr;
				getCtrlString(CTL_ELINK_ADDR, addr);
				if(addr.Len()) {
					SString phone_to;
					addr.Transf(CTRANSF_INNER_TO_UTF8).Utf8ToLower();
					PPEAddr::Phone::NormalizeStr(addr, 0, phone_to);
					PPObjPhoneService::PhoneTo(phone_to);
				}
			}
			else
				return;
			clearEvent(event);
		}
	public:
		ELinkDialog() : TDialog(DLG_ELINK)
		{
		}
		DECL_DIALOG_SETDTS()
		{
			RVALUEPTR(Data, pData);
			SetupPPObjCombo(this, CTLSEL_ELINK_KIND, PPOBJ_ELINKKIND, Data.KindID, OLW_CANINSERT, 0);
			setCtrlData(CTL_ELINK_ADDR, Data.Addr);
			return 1;
		}
		DECL_DIALOG_GETDTS()
		{
			int    ok = 1;
			getCtrlData(CTLSEL_ELINK_KIND, &Data.KindID);
			getCtrlData(CTL_ELINK_ADDR, Data.Addr);
			if(Data.KindID == 0)
				ok = PPErrorByDialog(this, CTLSEL_ELINK_KIND, PPERR_ELINKKINDNEEDED);
			else if(*strip(Data.Addr) == 0)
				ok = PPErrorByDialog(this, CTL_ELINK_ADDR, PPERR_ELINKADDRNEEDED);
			else {
				PPELinkKind elk_rec;
				if(ElkObj.Fetch(Data.KindID, &elk_rec) > 0) {
					if(elk_rec.Type == ELNKRT_EMAIL) {
						STokenRecognizer tr;
						SNaturalTokenArray nta;
						tr.Run(reinterpret_cast<const uchar *>(Data.Addr), -1, nta, 0);
						if(nta.Has(SNTOK_EMAIL) == 0.0f) {
							PPSetError(PPERR_INVEMAILADDR, Data.Addr);
							ok = PPErrorByDialog(this, CTL_ELINK_ADDR);
						}
					}
				}
				else 
					ok = PPErrorByDialog(this, CTLSEL_ELINK_KIND);
			}
			if(ok)
				ASSIGN_PTR(pData, Data);
			return ok;
		}
	};
	int    r = cmCancel;
	ELinkDialog * dlg = new ELinkDialog;
	if(CheckDialogPtrErr(&dlg)) {
		dlg->setDTS(pLink);
		for(int valid_data = 0; !valid_data && (r = ExecView(dlg)) == cmOK;) {
			if(dlg->getDTS(pLink)) {
				valid_data = 1;
			}
		}
		delete dlg;
		return r;
	}
	else
		return 0;
}

class ELinkListDialog : public PPListDialog {
	DECL_DIALOG_DATA(PPELinkArray);
	PPObjELinkKind elkobj;
public:
	explicit ELinkListDialog(DlgDataType * pArray) : PPListDialog(DLG_ELNKLST, CTL_ELNKLST_LIST)
	{
		Data.copy(*pArray);
		updateList(-1);
	}
	DECL_DIALOG_GETDTS()
	{
		CALLPTRMEMB(pData, copy(Data));
		return 1;
	}
private:
	virtual int setupList();
	virtual int addItem(long * pos, long * id);
	virtual int editItem(long pos, long id);
	virtual int delItem(long pos, long id);
};

int ELinkListDialog::setupList()
{
	PPELinkKind elkr;
	PPELink * elr;
	SString sub;
	for(uint i = 0; Data.enumItems(&i, (void **)&elr);) {
		StringSet ss(SLBColumnDelim);
		if(elkobj.Fetch(elr->KindID, &elkr) > 0)
			ss.add(elkr.Name);
		else {
			ideqvalstr(elr->KindID, sub.Z());
			ss.add(sub);
		}
		ss.add(elr->Addr);
		if(!addStringToList(i, ss.getBuf()))
			return 0;
	}
	return 1;
}

int ELinkListDialog::addItem(long * pPos, long * pID)
{
	int    ok = -1;
	PPELink link;
	if(EditELink(&link) == cmOK) {
		if(!Data.insert(&link))
			ok = PPSetErrorSLib();
		else {
			ASSIGN_PTR(pPos, Data.getCount()-1);
			ASSIGN_PTR(pID, Data.getCount());
			ok = 1;
		}
	}
	return ok;
}

int ELinkListDialog::editItem(long, long id)
{
	if(id > 0 && id <= Data.getCountI()) {
		PPELink link = Data.at((uint)id-1);
		if(EditELink(&link) == cmOK) {
			Data.at((uint)id-1) = link;
			return 1;
		}
	}
	return -1;
}

int ELinkListDialog::delItem(long, long id)
{
	if(id > 0 && id <= Data.getCountI()) {
		Data.atFree((uint)id-1);
		return 1;
	}
	return -1;
}

static int CompareELinkKinds2(const PPELinkKind * k1, const PPELinkKind * k2)
{
	if(k1->Type == k2->Type)
		return stricmp866(k1->Name, k2->Name);
	else if(k1->Type == ELNKRT_UNKNOWN)
	   	return 1;
	else if(k2->Type == ELNKRT_UNKNOWN)
		return -1;
	else
		return (int)(k1->Type - k2->Type);
}

static IMPL_CMPFUNC(PPELinkKind, i1, i2)
{
	const PPELinkKind * k1 = static_cast<const PPELinkKind *>(i1);
	const PPELinkKind * k2 = static_cast<const PPELinkKind *>(i2);
	if(k1->Flags & ELNKF_PREF)
		if(k2->Flags & ELNKF_PREF)
			return CompareELinkKinds2(k1, k2);
		else
			return -1;
	else if(k2->Flags & ELNKF_PREF)
		return 1;
	else
	   	return CompareELinkKinds2(k1, k2);
}

static int OrderELinkArray(PPELinkArray * ary, SArray * kinds)
{
	int    ok = 1;
	//
	// Алгоритм сортировки массива ELinkArray следующий:
	// 1. Виды электронной связи (kinds) отсортированы по предпочтительности
	// 2. Заменяем идентификаторы вида связи на порядковый номер этого
	//    вида в массиве kinds (критерий сортировки) в массиве ary.
	//    Если вид связи не является валидным значением, то вместо
	//    замены просто прибавляем к нему смещение 1000
	// 3. Сортируем массив ary согласно сортировке массива kinds
	// 4. Делаем обратную замену идентификаторов
	//
	uint   i, kp;
	for(i = 0; i < ary->getCount(); i++)
		if(kinds->lsearch(&ary->at(i).KindID, &(kp = 0), CMPF_LONG, sizeof(PPID)))
			ary->at(i).KindID = kp;
		else
			ary->at(i).KindID += 1000L;
	ary->sort(CMPF_LONG);
	for(i = 0; i < ary->getCount(); i++) {
		long & tk = ary->at(i).KindID;
		if(tk < 1000L)
			tk = static_cast<const PPELinkKind *>(kinds->at((uint)tk))->ID;
		else
			tk -= 1000L;
	}
	return ok;
}

int EditELinks(const char * pInfo, PPELinkArray * pList)
{
	static const uint SlotCount = 5;
	class ELinkDialog : public TDialog {
	public:
		ELinkDialog(PPELinkArray * pArray) : TDialog(DLG_ELNKSET), KindList(sizeof(PPELinkKind))
		{
			PPELinkKind k;
			PPObjELinkKind elk_obj;
			for(PPID id = 0; elk_obj.EnumItems(&id, &k) > 0;)
				KindList.insert(&k);
			KindList.sort(PTR_CMPFUNC(PPELinkKind));
			Data.copy(*pArray);
			Setup();
		}
		int  getDTS(PPELinkArray * pData)
		{
			int    ok = 1;
			uint   sel = 0;
			PPELink * p_item;
			for(uint i = 0; i < SlotCount && i < Data.getCount(); i++) {
				p_item = & Data.at(i);
				getCtrlData(sel = (i * 3 + 2 + WINDOWS_ID_BIAS), &p_item->KindID);
				getCtrlData(sel = (i * 3 + 3 + WINDOWS_ID_BIAS), p_item->Addr);
				strip(p_item->Addr);
				if(p_item->KindID == PPELK_EMAIL && p_item->Addr[0]) {
					STokenRecognizer tr;
					SNaturalTokenArray nta;
					tr.Run(reinterpret_cast<const uchar *>(p_item->Addr), -1, nta, 0);
					THROW_PP_S(nta.Has(SNTOK_EMAIL) > 0.0f, PPERR_INVEMAILADDR, p_item->Addr);
				}
			}
			for(int j = Data.getCount() - 1; j >= 0; j--) {
				p_item = &Data.at(j);
				if(p_item->KindID == 0 || p_item->Addr[0] == 0)
					Data.atFree(j);
			}
			CALLPTRMEMB(pData, copy(Data));
			CATCHZOKPPERRBYDLG
			return ok;
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(TVCOMMAND) {
				if(event.isCmd(cmaMore)) {
					editList();
					clearEvent(event);
				}
				else {
					for(uint i = 0; i < SlotCount; i++) {
						if(event.message.command == (cmELnkSetAction1+i)) {
							SString phone_buf;
							if(IsPhoneNumber(i, phone_buf)) {
								PPObjPhoneService::PhoneTo(phone_buf);
							}
							break;
						}
					}
				}
			}
		}
		int    IsPhoneNumber(uint position, SString & rPhone)
		{
			int    ok = 0;
			rPhone.Z();
			if(position >= 0 && position < SlotCount) {
				int   mode = 0; // 0 - hide button, 1 - phone button, 2 - mail button (reserved)
				const uint ctl_id = CTL_ELNKSET_ACN1 + position;
				if(position < Data.getCount()) {
					const PPELinkKind * p_elk = (PPELinkKind *)KindList.at(position);
					if(oneof2(p_elk->Type, ELNKRT_PHONE, ELNKRT_INTERNALEXTEN)) {
						SString addr_buf;
						getCtrlString(position * 3 + 3 + WINDOWS_ID_BIAS, addr_buf);
						if(addr_buf.Len()) {
							addr_buf.Transf(CTRANSF_INNER_TO_UTF8).Utf8ToLower();
							PPEAddr::Phone::NormalizeStr(addr_buf, 0, rPhone);
							if(rPhone.Len())
								mode = 1;
						}
					}

				}
				if(mode == 1)
					ok = 1;
			}
			return ok;
		}
		void   SetupActionButton(uint position)
		{
			SString phone_buf;
			if(position >= 0 && position < SlotCount) {
				const uint ctl_id = CTL_ELNKSET_ACN1 + position;
				const  PPID def_phn_svc_id = DS.GetConstTLA().DefPhnSvcID;
				if(def_phn_svc_id && IsPhoneNumber(position, phone_buf)) {
					showCtrl(ctl_id, true);
					setButtonBitmap(cmELnkSetAction1+position, IDB_PHONEFORWARDED);
				}
				else
					showCtrl(ctl_id, false);
			}
		}
		void   Setup()
		{
			uint   i;
			PPELink lnk;
			OrderELinkArray(&Data, &KindList);
			if(Data.getCount() < SlotCount) {
				for(i = 0; i < KindList.getCount() && Data.getCount() <= SlotCount; i++) {
					PPID k = static_cast<const PPELinkKind *>(KindList.at(i))->ID;
					if(!Data.lsearch(&k, 0, CMPF_LONG)) {
						MEMSZERO(lnk);
						lnk.KindID = k;
						Data.insert(&lnk);
					}
				}
			}
			for(i = 0; i < SlotCount && i < Data.getCount(); i++) {
				lnk = Data.at(i);
				SetupPPObjCombo(this, i * 3 + 2 + WINDOWS_ID_BIAS, PPOBJ_ELINKKIND, lnk.KindID, OLW_CANINSERT, 0);
				setCtrlData(i * 3 + 3 + WINDOWS_ID_BIAS, lnk.Addr);
			}
			for(i = 0; i < SlotCount; i++)
				SetupActionButton(i);
		}
		void   editList()
		{
			ELinkListDialog * dlg = 0;
			getDTS(0);
			if(CheckDialogPtrErr(&(dlg = new ELinkListDialog(&Data)))) {
				if(ExecView(dlg) == cmOK) {
					dlg->getDTS(&Data);
					Setup();
				}
				delete dlg;
			}
		}
		SArray KindList;
		PPELinkArray Data;
		PPObjELinkKind ElkObj;
	};
	int    r = -1;
	ELinkDialog * dlg = new ELinkDialog(pList);
	if(CheckDialogPtrErr(&dlg)) {
		SString temp_buf(pInfo);
		dlg->setCtrlString(CTL_ELNKSET_INFO, temp_buf);
		while(r < 0 && ExecView(dlg) == cmOK) {
			if(dlg->getDTS(pList))
				r = 1;
		}
		delete dlg;
	}
	else
		r = 0;
	return r;
}
