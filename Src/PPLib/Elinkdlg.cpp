// ELINKDLG.CPP
// Copyright (c) A.Sobolev 1998, 1999, 2000, 2001, 2002, 2003, 2006, 2007, 2009, 2015, 2016, 2017, 2018
// @codepage windows-1251
//
#include <pp.h>
#pragma hdrstop

int SLAPI EditELink(PPELink * pLink)
{
	class ELinkDialog : public TDialog {
	public:
		ELinkDialog() : TDialog(DLG_ELINK)
		{
		}
		int    setDTS(const PPELink * pData)
		{
			RVALUEPTR(Data, pData);
			SetupPPObjCombo(this, CTLSEL_ELINK_KIND, PPOBJ_ELINKKIND, Data.KindID, OLW_CANINSERT, 0);
			setCtrlData(CTL_ELINK_ADDR, Data.Addr);
			return 1;
		}
		int    getDTS(PPELink * pData)
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
						tr.Run((const uchar *)Data.Addr, -1, nta, 0);
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
	private:
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
							PPEAddr::Phone::NormalizeStr(addr, info_buf);
						}
					}
				}
				setStaticText(CTL_ELINK_INFO, info_buf);
			}	
			else if(event.isKeyDown(kbF2)) { // test
				SString addr;
				getCtrlString(CTL_ELINK_ADDR, addr);
				if(addr.Len()) {
					SString phone_to;
					addr.Transf(CTRANSF_INNER_TO_UTF8).Utf8ToLower();
					PPEAddr::Phone::NormalizeStr(addr, phone_to);
					if(phone_to.NotEmpty()) {
						SString channel_from;
						PPObjPhoneService ps_obj(0);
						PPPhoneServicePacket ps_pack;
						PPEquipConfig eq_cfg;
						ReadEquipConfig(&eq_cfg);
						if(eq_cfg.PhnSvcID && ps_obj.GetPacket(eq_cfg.PhnSvcID, &ps_pack) > 0) {
							if(ps_pack.GetPrimaryOriginateSymb(channel_from)) {
								AsteriskAmiClient * p_phnsvccli = ps_obj.InitAsteriskAmiClient(eq_cfg.PhnSvcID);
								if(p_phnsvccli) {
									p_phnsvccli->Originate(channel_from, phone_to, 0, 1);
								}
								delete p_phnsvccli;
							}
						}
					}
				}
			}
			else
				return;
			clearEvent(event);
		}
		PPELink Data;
		PPObjELinkKind ElkObj;
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
public:
	ELinkListDialog::ELinkListDialog(PPELinkArray * pArray) : PPListDialog(DLG_ELNKLST, CTL_ELNKLST_LIST)
	{
		Data.copy(*pArray);
		updateList(-1);
	}
	int    getDTS(PPELinkArray *);
private:
	virtual int setupList();
	virtual int addItem(long * pos, long * id);
	virtual int editItem(long pos, long id);
	virtual int delItem(long pos, long id);

	PPELinkArray   Data;
	PPObjELinkKind elkobj;
};

int ELinkListDialog::getDTS(PPELinkArray * pData)
{
	CALLPTRMEMB(pData, copy(Data));
	return 1;
}

int ELinkListDialog::setupList()
{
	PPELinkKind elkr;
	PPELink * elr;
	SString sub;
	for(uint i = 0; Data.enumItems(&i, (void**)&elr);) {
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
	MEMSZERO(link);
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
	if(id > 0 && id <= (long)Data.getCount()) {
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
	if(id > 0 && id <= (long)Data.getCount()) {
		Data.atFree((uint)id-1);
		return 1;
	}
	return -1;
}

static int SLAPI CompareELinkKinds2(const PPELinkKind * k1, const PPELinkKind * k2)
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
	const PPELinkKind * k1 = (const PPELinkKind *)i1;
	const PPELinkKind * k2 = (const PPELinkKind *)i2;
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

static int SLAPI OrderELinkArray(PPELinkArray * ary, SArray * kinds)
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
			tk = ((PPELinkKind*)kinds->at((uint)tk))->ID;
		else
			tk -= 1000L;
	}
	return ok;
}

int SLAPI EditELinks(PPELinkArray * pList)
{
	class ELinkDialog : public TDialog {
	public:
		ELinkDialog(PPELinkArray * ary) : TDialog(DLG_ELNKSET), kinds(sizeof(PPELinkKind))
		{
			PPELinkKind k;
			PPObjELinkKind elk_obj;
			for(PPID id = 0; elk_obj.EnumItems(&id, &k) > 0;)
				kinds.insert(&k);
			kinds.sort(PTR_CMPFUNC(PPELinkKind));
			data.copy(*ary);
			setup();
		}
		int  getDTS(PPELinkArray * pData)
		{
			int    ok = 1;
			uint   sel = 0;
			PPELink * p_item;
			for(uint i = 0; i < 5 && i < data.getCount(); i++) {
				p_item = & data.at(i);
				getCtrlData(sel = (i * 3 + 2 + WINDOWS_ID_BIAS), &p_item->KindID);
				getCtrlData(sel = (i * 3 + 3 + WINDOWS_ID_BIAS), p_item->Addr);
				strip(p_item->Addr);
				if(p_item->KindID == PPELK_EMAIL && p_item->Addr[0]) {
					STokenRecognizer tr;
					SNaturalTokenArray nta;
					tr.Run((const uchar *)p_item->Addr, -1, nta, 0);
					THROW_PP_S(nta.Has(SNTOK_EMAIL) > 0.0f, PPERR_INVEMAILADDR, p_item->Addr);
				}
			}
			for(int j = data.getCount() - 1; j >= 0; j--) {
				p_item = & data.at(j);
				if(p_item->KindID == 0 || p_item->Addr[0] == 0)
					data.atFree(j);
			}
			CALLPTRMEMB(pData, copy(data));
			CATCH
				ok = PPErrorByDialog(this, sel);
			ENDCATCH
			return ok;
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isCmd(cmaMore)) {
				editList();
				clearEvent(event);
			}
		}
		void   setup()
		{
			uint   i;
			PPELink lnk;
			OrderELinkArray(&data, &kinds);
			if(data.getCount() < 5) {
				for(i = 0; i < kinds.getCount() && data.getCount() <= 5; i++) {
					PPID k = ((PPELinkKind*)kinds.at(i))->ID;
					if(!data.lsearch(&k, 0, CMPF_LONG)) {
						MEMSZERO(lnk);
						lnk.KindID = k;
						data.insert(&lnk);
					}
				}
			}
			for(i = 0; i < 5 && i < data.getCount(); i++) {
				lnk = data.at(i);
				SetupPPObjCombo(this, i * 3 + 2 + WINDOWS_ID_BIAS, PPOBJ_ELINKKIND, lnk.KindID, OLW_CANINSERT, 0);
				setCtrlData(i * 3 + 3 + WINDOWS_ID_BIAS, lnk.Addr);
			}
		}
		void   editList()
		{
			ELinkListDialog * dlg = 0;
			getDTS(0);
			if(CheckDialogPtrErr(&(dlg = new ELinkListDialog(&data)))) {
				if(ExecView(dlg) == cmOK) {
					dlg->getDTS(&data);
					setup();
				}
				delete dlg;
			}
		}
		SArray kinds;
		PPELinkArray data;
	};
	int    r = -1;
	ELinkDialog * dlg = new ELinkDialog(pList);
	if(CheckDialogPtrErr(&dlg)) {
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
