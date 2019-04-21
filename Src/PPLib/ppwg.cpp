// PPWG.CPP
// Copyright (c) A.Sobolev 2006, 2007, 2010, 2013, 2015, 2016, 2017, 2018, 2019
//
// √рафики рабочего времени
//
#include <pp.h>
#pragma hdrstop
//
// @ModuleDef(PPObjDateTimeRep)
//
SLAPI PPObjDateTimeRep::PPObjDateTimeRep(void * extraPtr) : PPObjReference(PPOBJ_DATETIMEREP, extraPtr)
{
}

static int EditDtr(PPDateTimeRep * pData)
{
	class DtrDialog : public TDialog {
	public:
		DtrDialog() : TDialog(DLG_DTR)
		{
		}
		int    setDTS(const PPDateTimeRep * pData)
		{
			Data = *pData;
			setCtrlData(CTL_DTR_NAME, Data.Name);
			setCtrlData(CTL_DTR_ID, &Data.ID);
			return 1;
		}
		int     getDTS(PPDateTimeRep * pData)
		{
			getCtrlData(CTL_DTR_NAME, Data.Name);
			//getCtrlData(CTL_DTR_ID, &Data.ID);
			if(*strip(Data.Name) == 0) {
				selectCtrl(CTL_DTR_NAME);
				PPError(PPERR_NAMENEEDED, 0);
				return 0;
			}
			else {
				ASSIGN_PTR(pData, Data);
				return 1;
			}
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(TVCOMMAND && TVCMD == cmDtrRepeating) {
				RepeatingDialog * dlg = new RepeatingDialog(RepeatingDialog::fEditTime|RepeatingDialog::fEditDuration);
				if(CheckDialogPtrErr(&dlg)) {
					dlg->setDTS(&Data.Dtr);
					dlg->setDuration(Data.Duration);
					while(ExecView(dlg) == cmOK)
						if(dlg->getDTS(&Data.Dtr)) {
							dlg->getDuration(&Data.Duration);
							clearEvent(event);
							return;
						}
				}
			}
		}
		PPDateTimeRep Data;
	};
	DIALOG_PROC_BODY(DtrDialog, pData);
}

int SLAPI PPObjDateTimeRep::Edit(PPID * pID, void * extraPtr)
{
	int    ok = cmCancel;
	PPDateTimeRep rec;
	THROW(CheckRightsModByID(pID));
	if(*pID) {
		THROW(Search(*pID, &rec) > 0);
	}
	else
		MEMSZERO(rec);
	if(EditDtr(&rec) > 0) {
		if(*pID)
			*pID = rec.ID;
		THROW(EditItem(Obj, *pID, &rec, 1));
		*pID = rec.ID;
		ok = cmOK;
	}
	CATCHZOKPPERR
	return ok;
}
//
// @ModuleDef(PPObjDutySched)
//
IMPL_CMPFUNC(PPDutyCountPoint_Dt, i1, i2) { return CMPFUNC(LDATE, &((PPDutyCountPoint *)i1)->Dtm.d, &((PPDutyCountPoint *)i2)->Dtm.d); }

SLAPI PPDutySchedPacket::PPDutySchedPacket()
{
	MEMSZERO(Rec);
}

int SLAPI PPDutySchedPacket::Normalyze()
{
	if(CpList.getCount()) {
		CpList.sort(PTR_CMPFUNC(PPDutyCountPoint_Dt));
		Cp.Init(CpList.at(0).Dtm, CpList.at(CpList.getCount()-1).Dtm);
	}
	else {
		Cp.Start.Z();
		Cp.Finish.Z();
	}
	{
		PPObjDateTimeRep dtr_obj;
		PPDateTimeRep dtr_rec;
		for(uint i = 0; i < List.getCount(); i++) {
			PPDutySchedEntry & r_entry = List.at(i);
			if(r_entry.DtrID && dtr_obj.Search(r_entry.DtrID, &dtr_rec) > 0) {
				r_entry.Dtr = dtr_rec.Dtr;
				r_entry.Duration = dtr_rec.Duration;
			}
		}
	}
	return 1;
}

int SLAPI PPDutySchedPacket::InitIteration(EnumParam * pEnum, const STimeChunk & rBounds) const
{
	int    ok = 1;
	EnumParam ep;
	MEMSZERO(ep);
	ep.Dtm = rBounds.Start;
	ep.Bounds = rBounds;
	if(ep.Bounds.Finish.d == 0)
		ep.Bounds.Finish.d = encodedate(31, 12, 2099);
	THROW_PP_S(List.getCount(), PPERR_DUTYSCHEDEMPTY, Rec.Name);
	THROW_PP_S(Cp.Start.d, PPERR_DUTYSCHEDNOCP, Rec.Name);
	if(ep.Dtm.d == 0) {
		ep.Dtm = Cp.Start;
	}
	else {
		uint   i = CpList.getCount();
		THROW_PP_S(ep.Dtm.d >= Cp.Start.d, PPERR_DUTYSCHEDENUMDT, Rec.Name);
		do {
			LDATETIME dtm = CpList.at(--i).Dtm;
			if(ep.Dtm.d >= dtm.d) {
				ep.Dtm = dtm;
				break;
			}
		} while(i != 0);
	}
	CATCHZOK
	ASSIGN_PTR(pEnum, ep);
	return ok;
}

int FASTCALL PPDutySchedPacket::NextIteration(EnumParam * pEnum) const
{
	LDATETIME next;
	if(Cp.Start.d) {
		uint   qp = pEnum->QueuePos % List.getCount();
		const  PPDutySchedEntry * p_entry = & List.at(qp);
		DateTimeRepIterator dr_iter(p_entry->Dtr, pEnum->Dtm);
		while(!!(next = dr_iter.Next())) {
			if(next.d <= Cp.Finish.d && IsCountPoint(&next)) {
				p_entry = &List.at(0);
				pEnum->QueuePos = 0;
			}
			pEnum->Dtm      = next;
			pEnum->Duration = p_entry->Duration;
			pEnum->ObjID    = p_entry->ObjID;
			pEnum->QueuePos++;
			if(cmp(next, pEnum->Bounds.Start) >= 0)
				return (cmp(next, pEnum->Bounds.Finish) > 0) ? -1 : 1;
			else {
				qp = pEnum->QueuePos % List.getCount();
				p_entry = &List.at(qp);
			}
		}
		return 0; // unreacheble point
	}
	else
		return PPSetError(PPERR_DUTYSCHEDNOCP, Rec.Name);
}

int SLAPI PPDutySchedPacket::IsCountPoint(LDATETIME * pDtm) const
{
	PPDutyCountPoint * p_point;
	LTIME count_time = ZEROTIME;
	if(List.getCount())
		count_time = List.at(0).Dtr.Time;
	for(uint i = 0; CpList.enumItems(&i, (void **)&p_point);)
		if(p_point->Dtm.d == pDtm->d && pDtm->t == count_time) {
			*pDtm = p_point->Dtm;
			pDtm->t = count_time;
			return 1;
		}
	return 0;
}

int SLAPI PPDutySchedPacket::AddCountPointDate(LDATE dt, uint * pPos)
{
	if(checkdate(&dt)) {
		uint pos = 0;
		PPDutyCountPoint cp;
		MEMSZERO(cp);
		cp.Dtm.d = dt;
		if(CpList.lsearch(&cp, &pos, PTR_CMPFUNC(PPDutyCountPoint_Dt))) {
			ASSIGN_PTR(pPos, pos);
			return -1;
		}
		else {
			CpList.ordInsert(&cp, &pos, PTR_CMPFUNC(PPDutyCountPoint_Dt));
			ASSIGN_PTR(pPos, pos);
			return 1;
		}
	}
	else
		return PPSetErrorSLib();
}

int SLAPI PPDutySchedPacket::RemoveCountPoint(uint pos)
{
	return CpList.atFree(pos);
}

int SLAPI PPDutySchedPacket::Test(const char * pOutFile) const
{
	int    ok = 1;
	SString msg_buf, add_buf;
	EnumParam ep;
	STimeChunk bounds;
	bounds.Start.Set(encodedate(1, 2, 2007), ZEROTIME);
	bounds.Finish.Set(encodedate(1, 1, 2008), ZEROTIME);
	msg_buf.Z();
	add_buf.Z().Cat(bounds.Start).CatCharN('.', 2).Cat(bounds.Finish);
	msg_buf.CatEq("period", add_buf);
	PPLogMessage(pOutFile, msg_buf, 0);
	THROW(InitIteration(&ep, bounds));
	while(NextIteration(&ep) > 0) {
		msg_buf.Z();
		LTIME tm_dur;
		GetObjectName(Rec.ObjType, ep.ObjID, add_buf.Z());
		tm_dur.settotalsec(ep.Duration);
		msg_buf.Tab().Cat(ep.Dtm).CatDiv('-', 1).Cat(tm_dur).CatDiv('-', 1).Cat(add_buf);
		PPLogMessage(pOutFile, msg_buf, 0);
	}
	CATCH
		ok = 0;
		PPGetLastErrorMessage(1, msg_buf);
		PPLogMessage(pOutFile, msg_buf, 0);
	ENDCATCH
	return ok;
}
//
//
//
SLAPI PPObjDutySched::PPObjDutySched(void * extraPtr) : PPObjReference(PPOBJ_DUTYSCHED, extraPtr)
{
}

int SLAPI PPObjDutySched::SearchByObjType(PPID objType, long objGroup, PPID * pID)
{
	PPDutySched rec;
	for(PPID id = 0; EnumItems(&id, &rec) > 0;)
		if(rec.ObjType == objType && rec.ObjGroup == objGroup) {
			ASSIGN_PTR(pID, rec.ID);
			return 1;
		}
	return -1;
}

int SLAPI PPObjDutySched::PutPacket(PPID * pID, PPDutySchedPacket * pPack, int use_ta)
{
	int    ok = 1;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(pPack) {
			THROW(CheckDupName(*pID, pPack->Rec.Name));
			if(*pID) {
				THROW(ref->UpdateItem(Obj, *pID, &pPack->Rec, 1, 0));
			}
			else {
				*pID = pPack->Rec.ID;
				THROW(ref->AddItem(Obj, pID, &pPack->Rec, 0));
			}
			THROW(ref->PutPropArray(Obj, *pID, DSHPRP_LIST, &pPack->List, 0));
			THROW(ref->PutPropArray(Obj, *pID, DSHPRP_POINTLIST, &pPack->CpList, 0));
		}
		else if(*pID) {
			THROW(ref->RemoveItem(Obj, *pID, 0));
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjDutySched::GetPacket(PPID id, PPDutySchedPacket * pPack)
{
	int    ok = -1;
	if(Search(id, &pPack->Rec) > 0) {
		THROW(ref->GetPropArray(Obj, id, DSHPRP_LIST, &pPack->List));
		THROW(ref->GetPropArray(Obj, id, DSHPRP_POINTLIST, &pPack->CpList));
		pPack->Normalyze();
		ok = 1;
	}
	CATCHZOK
	return ok;
}

static int EditDutySchedItem(const PPDutySched * pHead, PPDutySchedEntry * pData)
{
	int    ok = -1;
	TDialog * dlg = new TDialog(DLG_DUTYSCHEDITEM);
	if(CheckDialogPtrErr(&dlg)) {
		PPObjDateTimeRep dtr_obj;
		//PPDateTimeRep dtr_rec;
		if(pHead->ObjType)
			SetupPPObjCombo(dlg, CTLSEL_DUTYSCHEDITEM_OBJ, pHead->ObjType, pData->ObjID, 0, reinterpret_cast<void *>(pHead->ObjGroup));
		SetupPPObjCombo(dlg, CTLSEL_DUTYSCHEDITEM_REP, PPOBJ_DATETIMEREP, pData->DtrID, 0);
		while(ok <= 0 && ExecView(dlg) == cmOK) {
			uint sel = 0;
			dlg->getCtrlData(sel = CTLSEL_DUTYSCHEDITEM_OBJ, &pData->ObjID);
			if(pData->ObjID == 0) {
				if(pHead->ObjType == PPOBJ_ARTICLE)
					PPError(PPERR_ARNEEDED, 0);
				else //if(pHead->ObjType == PPOBJ_PERSON)
					PPError(PPERR_PERSONNEEDED, 0);
				dlg->selectCtrl(sel);
			}
			else {
				dlg->getCtrlData(sel = CTLSEL_DUTYSCHEDITEM_REP, &pData->DtrID);
				if(pData->DtrID == 0)
					PPErrorByDialog(dlg, sel, PPERR_DTRNEEDED);
				else
					ok = 1;
			}
		}
	}
	else
		ok = 0;
	delete dlg;
	return ok;
}

class DutySchedDialog : public PPListDialog {
public:
	DutySchedDialog() : PPListDialog(DLG_DUTYSCHED, CTL_DUTYSCHED_LIST)
	{
		SmartListBox * p_list = static_cast<SmartListBox *>(getCtrlView(CTL_DUTYSCHED_CPLIST));
		if(p_list && !SetupStrListBox(p_list))
			PPError();
	}
	int    setDTS(const PPDutySchedPacket * pData);
	int    getDTS(PPDutySchedPacket * pData);
private:
	DECL_HANDLE_EVENT;
	virtual int setupList();
	virtual int addItem(long * pPos, long * pID);
	virtual int editItem(long pos, long id);
	virtual int delItem(long pos, long id);
	int    setupObjType();
	void   updateCountPointList(long pos);
	void   fillStaffCal();

	PPDutySchedPacket Data;
};

IMPL_HANDLE_EVENT(DutySchedDialog)
{
	PPListDialog::handleEvent(event);
	if(event.isClusterClk(CTL_DUTYSCHED_OBJTYPE))
		setupObjType();
	else if(event.isCmd(cmDutySchedAddCp)) {
		LDATE  dt = ZERODATE;
		if(InputDateDialog(0, 0, &dt) > 0) {
			uint pos = 0;
			int r = Data.AddCountPointDate(dt, &pos);
			if(r)
				updateCountPointList(static_cast<long>(pos));
			else
				PPError();
		}
	}
	else if(event.isCmd(cmDutySchedRmvCp)) {
		SmartListBox * p_list = static_cast<SmartListBox *>(getCtrlView(CTL_DUTYSCHED_CPLIST));
		if(p_list && p_list->def) {
			long i = 0;
			p_list->getCurID(&i);
			Data.RemoveCountPoint((uint)i-1);
			updateCountPointList((uint)i-1);
		}
	}
	else if(event.isCmd(cmFillStaffCal))
		fillStaffCal();
	else if(event.isCmd(cmDutySchedStaffCal)) {
		long pos = 0, id = 0;
		if(Data.Rec.ID && getCurItem(&pos, &id) > 0 &&
			checkupper((uint)pos, Data.List.getCount())) {
			const PPDutySchedEntry & r_entry = Data.List.at(static_cast<uint>(pos));
			if(Data.Rec.ObjType == PPOBJ_PERSON && r_entry.ObjID) {
				StaffCalFilt filt;
				filt.LinkObjType = PPOBJ_PERSON;
				filt.LinkObjList.Add(r_entry.ObjID);
				PPObjStaffCal sc_obj;
				sc_obj.Browse(&filt);
			}
		}
	}
	else if(event.isKeyDown(kbCtrlT))
		Data.Test(0);
	else
		return;
	clearEvent(event);
}

void DutySchedDialog::fillStaffCal()
{
	if(Data.Rec.ID && Data.Rec.ObjType == PPOBJ_PERSON) {
		int   r = -1;
		PPObjStaffCal sc_obj;
		PPObjPerson psn_obj;
		DateRange period;
		period.Z();
		PPID   base_cal_id = psn_obj.GetConfig().RegStaffCalID;
		{
			TDialog * dlg = new TDialog(DLG_FILLSCDT);
			if(CheckDialogPtrErr(&dlg)) {
				dlg->SetupCalPeriod(CTLCAL_FILLSCDT_PERIOD, CTL_FILLSCDT_PERIOD);
				SetupPPObjCombo(dlg, CTLSEL_FILLSCDT_CAL, PPOBJ_STAFFCAL, base_cal_id, 0, 0);
				SetPeriodInput(dlg, CTL_FILLSCDT_PERIOD, &period);
				while(r < 0 && ExecView(dlg) == cmOK) {
					base_cal_id = dlg->getCtrlLong(CTLSEL_FILLSCDT_CAL);
					GetPeriodInput(dlg, CTL_FILLSCDT_PERIOD, &period);
					if(base_cal_id == 0)
						PPErrorByDialog(dlg, CTLSEL_FILLSCDT_CAL, PPERR_STAFFCALNEEDED);
					else if(!period.low || !period.upp)
						PPErrorByDialog(dlg, CTL_FILLSCDT_PERIOD, PPERR_PERIODMUSTBECLOSED);
					else
						r = 1;
				}
			}
			delete dlg;
		}
		if(r > 0) {
			if(!sc_obj.SetEntriesByDutySched(base_cal_id, &Data, period, 1))
				PPError();
		}
	}
}

void DutySchedDialog::updateCountPointList(long pos)
{
	SmartListBox * p_list = static_cast<SmartListBox *>(getCtrlView(CTL_DUTYSCHED_CPLIST));
	if(p_list) {
		PPDutyCountPoint * p_point;
		SString sub;
		int    sav_pos = (int)p_list->def->_curItem();
		p_list->freeAll();
		for(uint i = 0; Data.CpList.enumItems(&i, (void **)&p_point);) {
			sub.Z().Cat(p_point->Dtm.d, DATF_DMY);
			p_list->addItem(i, sub);
		}
	   	p_list->focusItem((pos < 0) ? sav_pos : pos);
		p_list->Draw_();
	}
}

int DutySchedDialog::setupList()
{
	int    ok = 1;
	PPDutySchedEntry * p_item = 0;
	SString sub;
	for(uint i = 0; ok && Data.List.enumItems(&i, (void **)&p_item);) {
		StringSet ss(SLBColumnDelim);
		GetObjectName(Data.Rec.ObjType, p_item->ObjID, sub);
		ss.add(sub);
		GetObjectName(PPOBJ_DATETIMEREP, p_item->DtrID, sub);
		ss.add(sub);
		if(!addStringToList(i, ss.getBuf()))
			ok = 0;
	}
	return ok;
}

int DutySchedDialog::addItem(long * pPos, long * pID)
{
	PPDutySchedEntry entry;
	MEMSZERO(entry);
	Data.Rec.ObjGroup = getCtrlLong(CTLSEL_DUTYSCHED_OBJGRP);
	if(EditDutySchedItem(&Data.Rec, &entry) > 0) {
		Data.List.insert(&entry);
		ASSIGN_PTR(pPos, Data.List.getCount()-1);
		ASSIGN_PTR(pID, Data.List.getCount());
		return 1;
	}
	else
		return -1;
}

int DutySchedDialog::editItem(long pos, long id)
{
	if(pos < (long)Data.List.getCount()) {
		PPDutySchedEntry entry = Data.List.at(pos);
		Data.Rec.ObjGroup = getCtrlLong(CTLSEL_DUTYSCHED_OBJGRP);
		if(EditDutySchedItem(&Data.Rec, &entry) > 0) {
			Data.List.at(pos) = entry;
			return 1;
		}
	}
	return -1;
}

int DutySchedDialog::delItem(long pos, long id)
{
	return Data.List.atFree(static_cast<uint>(pos)) ? 1 : -1;
}

int DutySchedDialog::setupObjType()
{
	PPID   prev_obj_type = Data.Rec.ObjType;
	GetClusterData(CTL_DUTYSCHED_OBJTYPE, &Data.Rec.ObjType);
	if(Data.Rec.ObjType != prev_obj_type)
		Data.Rec.ObjGroup = 0;
	if(Data.Rec.ObjType == PPOBJ_PERSON) {
		SetupPPObjCombo(this, CTLSEL_DUTYSCHED_OBJGRP, PPOBJ_PRSNKIND, Data.Rec.ObjGroup, 0, 0);
		enableCommand(cmDutySchedStaffCal, Data.Rec.ID);
		enableCommand(cmFillStaffCal, Data.Rec.ID);
	}
	else {
		if(Data.Rec.ObjType == PPOBJ_ARTICLE) {
			SetupPPObjCombo(this, CTLSEL_DUTYSCHED_OBJGRP, PPOBJ_ACCSHEET, Data.Rec.ObjGroup, 0, 0);
		}
		else
			setCtrlLong(CTLSEL_DUTYSCHED_OBJGRP, 0);
		enableCommand(cmDutySchedStaffCal, 0);
		enableCommand(cmFillStaffCal, 0);
	}
	disableCtrls(Data.List.getCount(), CTL_DUTYSCHED_OBJTYPE, CTLSEL_DUTYSCHED_OBJGRP, 0);
	return 1;
}

int DutySchedDialog::setDTS(const PPDutySchedPacket * pData)
{
	Data = *pData;
	setCtrlLong(CTL_DUTYSCHED_ID, Data.Rec.ID);
	setCtrlData(CTL_DUTYSCHED_NAME, Data.Rec.Name);
	AddClusterAssoc(CTL_DUTYSCHED_OBJTYPE, 0, PPOBJ_ARTICLE);
	AddClusterAssoc(CTL_DUTYSCHED_OBJTYPE, 1, PPOBJ_PERSON);
	SetClusterData(CTL_DUTYSCHED_OBJTYPE, Data.Rec.ObjType);
	setupObjType();
	updateList(-1);
	updateCountPointList(-1);
	return 1;
}

int DutySchedDialog::getDTS(PPDutySchedPacket * pData)
{
	int    ok = 1;
	uint   sel = 0;
	getCtrlData(sel = CTL_DUTYSCHED_NAME, Data.Rec.Name);
	THROW_PP(*strip(Data.Rec.Name), PPERR_NAMENEEDED);
	GetClusterData(CTL_DUTYSCHED_OBJTYPE, &Data.Rec.ObjType);
	Data.Rec.ObjGroup = getCtrlLong(sel = CTLSEL_DUTYSCHED_OBJGRP);
	THROW_PP(Data.Rec.ObjGroup, PPERR_OBJGRPNEEDED);
	Data.Normalyze();
	ASSIGN_PTR(pData, Data);
	CATCH
		ok = PPErrorByDialog(this, sel);
	ENDCATCH
	return ok;
}

int SLAPI PPObjDutySched::Edit(PPID * pID, void * extraPtr)
{
	int    ok = cmCancel, valid_data = 0;
	DutySchedDialog * dlg = 0;
	PPDutySchedPacket data;
	if(*pID)
		THROW(GetPacket(*pID, &data));
	THROW(CheckDialogPtr(&(dlg = new DutySchedDialog)));
	dlg->setDTS(&data);
	while(!valid_data && ExecView(dlg) == cmOK)
		if(dlg->getDTS(&data)) {
			if(PutPacket(pID, &data, 1)) {
				valid_data = 1;
				ok = cmOK;
			}
			else
				PPError();
		}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}
//
//
//
struct PPWorkCalendarEntry {
	WorkDate  Wdt;
	//
	// “ак как рабочий день может переходить через границу суток,
	// временной период работы представлен не временем начала и
	// временем окончани€, а временем начала и продолжительностью //
	// ѕродолжительность определена в секундах.
	//
	LTIME  BegTm;    // ¬рем€ начала      //
	long   Duration; // ѕродолжительность (sec) //
};

/*
class PPWorkCalendar {
public:
	PPWorkCalendar();
	int    Add(const PPWorkCalendarEntry *);
	int    SearchDate(LDATE, PPWorkCalendarEntry *) const;
private:
	TSArray <PPWorkCalendarEntry> List;
};
*/

