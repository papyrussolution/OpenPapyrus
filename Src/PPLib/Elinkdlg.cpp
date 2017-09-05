// ELINKDLG.CPP
// Copyright (c) A.Sobolev 1998, 1999, 2000, 2001, 2002, 2003, 2006, 2007, 2009, 2015, 2016, 2017
// @codepage windows-1251
//
#include <pp.h>
#pragma hdrstop

int SLAPI EditELink(PPELink * pLink)
{
	int    r = cmCancel;
	TDialog * dlg = new TDialog(DLG_ELINK);
	if(CheckDialogPtrErr(&dlg)) {
		SetupPPObjCombo(dlg, CTLSEL_ELINK_KIND, PPOBJ_ELINKKIND, pLink->KindID, OLW_CANINSERT, 0);
		dlg->setCtrlData(CTL_ELINK_ADDR, pLink->Addr);
		for(int valid_data = 0; !valid_data && (r = ExecView(dlg)) == cmOK;) {
			dlg->getCtrlData(CTLSEL_ELINK_KIND, &pLink->KindID);
			dlg->getCtrlData(CTL_ELINK_ADDR, pLink->Addr);
			if(pLink->KindID == 0)
				PPErrorByDialog(dlg, CTLSEL_ELINK_KIND, PPERR_ELINKKINDNEEDED);
			else if(*strip(pLink->Addr) == 0)
				PPErrorByDialog(dlg, CTL_ELINK_ADDR, PPERR_ELINKADDRNEEDED);
			else {
				valid_data = 1;
				if(pLink->KindID == PPELK_EMAIL) {
					PPTokenRecognizer tr;
					PPNaturalTokenArray nta;
					tr.Run((const uchar *)pLink->Addr, -1, nta, 0);
					if(nta.Has(PPNTOK_EMAIL) == 0.0f) {
						valid_data = PPSetError(PPERR_INVEMAILADDR, pLink->Addr);
						PPErrorByDialog(dlg, CTL_ELINK_ADDR);
					}
				}
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
			ideqvalstr(elr->KindID, sub = 0);
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
	if(EditELink(&link) == cmOK)
		if(!Data.insert(&link))
			ok = PPSetErrorSLib();
		else {
			ASSIGN_PTR(pPos, Data.getCount()-1);
			ASSIGN_PTR(pID, Data.getCount());
			ok = 1;
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

static int SLAPI CompareELinkKinds2(PPELinkKind * k1, PPELinkKind * k2)
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
	PPELinkKind * k1 = (PPELinkKind*)i1;
	PPELinkKind * k2 = (PPELinkKind*)i2;
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
	int  getDTS(PPELinkArray *);
private:
	DECL_HANDLE_EVENT
	{
		TDialog::handleEvent(event);
		if(event.isCmd(cmaMore)) {
			editList();
			clearEvent(event);
		}
	}
	void   setup();
	void   editList();

	SArray kinds;
	PPELinkArray data;
};

int ELinkDialog::getDTS(PPELinkArray * pData)
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
			PPTokenRecognizer tr;
			PPNaturalTokenArray nta;
			tr.Run((const uchar *)p_item->Addr, -1, nta, 0);
			THROW_PP_S(nta.Has(PPNTOK_EMAIL) > 0.0f, PPERR_INVEMAILADDR, p_item->Addr);
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

void ELinkDialog::setup()
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

void ELinkDialog::editList()
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

int SLAPI EditELinks(PPELinkArray * pList)
{
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
