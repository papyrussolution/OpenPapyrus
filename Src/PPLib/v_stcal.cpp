// V_STCAL.CPP
// Copyright (c) A.Sobolev 2007, 2008, 2009, 2010, 2011, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2024
//
#include <pp.h>
#pragma hdrstop
//
// @ModuleDef(PPViewStaffCal)
//
IMPLEMENT_PPFILT_FACTORY(StaffCal); StaffCalFilt::StaffCalFilt() : PPBaseFilt(PPFILT_STAFFCAL, 0, 2)
{
	SetFlatChunk(offsetof(StaffCalFilt, ReserveStart),
		offsetof(StaffCalFilt, LinkObjList)-offsetof(StaffCalFilt, ReserveStart));
	SetBranchObjIdListFilt(offsetof(StaffCalFilt, LinkObjList));
	SetBranchObjIdListFilt(offsetof(StaffCalFilt, CalList));
	Init(1, 0);
}

PPViewStaffCal::PPViewStaffCal() : PPView(0, &Filt, PPVIEW_STAFFCAL, implChangeFilt, 0), P_TempTbl(0), Grid(this)
{
}

PPViewStaffCal::~PPViewStaffCal()
{
	UpdateTimeBrowser(1);
	delete P_TempTbl;
}
//
//
//
class StaffCalFiltDialog : public TDialog {
	DECL_DIALOG_DATA(StaffCalFilt);
	enum {
		ctlgroupStaffCalList = 1
	};
public:
	StaffCalFiltDialog() : TDialog(DLG_STAFFCALFLT)
	{
		SetupCalPeriod(CTLCAL_STAFFCALFLT_PERIOD, CTL_STAFFCALFLT_PERIOD);
		addGroup(ctlgroupStaffCalList, new StaffCalCtrlGroup(CTLSEL_STAFFCALFLT_CAL, cmStaffCalFiltCalList));
	}
	DECL_DIALOG_SETDTS()
	{
		RVALUEPTR(Data, pData);
		SetPeriodInput(this, CTL_STAFFCALFLT_PERIOD, &Data.Period);
		PPIDArray obj_type_list;
		obj_type_list.addzlist(PPOBJ_PERSON, PPOBJ_STAFFLIST2, PPOBJ_PERSONPOST, 0L);
		SetupObjListCombo(this, CTLSEL_STAFFCALFLT_OT, Data.LinkObjType, &obj_type_list);
		SetupPPObjCombo(this, CTLSEL_STAFFCALFLT_PK, PPOBJ_PERSONKIND, Data.LinkPersonKind, 0, 0);
		SetupSubstDateCombo(this, CTLSEL_STAFFCALFLT_GRP, Data.Sgd);
		StaffCalCtrlGroup::Rec sc_rec(&Data.CalList);
		setGroupData(ctlgroupStaffCalList, &sc_rec);
		//
		SetupPPObjCombo(this, CTLSEL_STAFFCALFLT_PROJC, PPOBJ_STAFFCAL, Data.ProjCalID, 0, 0);
		AddClusterAssoc(CTL_STAFFCALFLT_FLAGS, 0, StaffCalFilt::fInverseProj);
		SetClusterData(CTL_STAFFCALFLT_FLAGS, Data.Flags);
		//
		AddClusterAssocDef(CTL_STAFFCALFLT_ORDER,  0, PPViewStaffCal::ordByDate);
		AddClusterAssoc(CTL_STAFFCALFLT_ORDER,  1, PPViewStaffCal::ordByObject);
		SetClusterData(CTL_STAFFCALFLT_ORDER, Data.Order);
		OnObjectSelection();
		return 1;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		StaffCalCtrlGroup::Rec sc_rec;
		THROW(GetPeriodInput(this, CTL_STAFFCALFLT_PERIOD, &Data.Period));
		getCtrlData(CTLSEL_STAFFCALFLT_OT, &Data.LinkObjType);
		if(Data.LinkObjType == PPOBJ_PERSON) {
			Data.LinkPersonKind = NZOR(getCtrlLong(CTLSEL_STAFFCALFLT_PK), PPPRK_EMPL);
		}
		else
			Data.LinkPersonKind = 0;
		getGroupData(ctlgroupStaffCalList, &sc_rec);
		Data.CalList = sc_rec.List;
		Data.ProjCalID = getCtrlLong(CTLSEL_STAFFCALFLT_PROJC);
		GetClusterData(CTL_STAFFCALFLT_FLAGS, &Data.Flags);
		GetClusterData(CTL_STAFFCALFLT_ORDER, &Data.Order);
		getCtrlData(CTLSEL_STAFFCALFLT_GRP, &Data.Sgd);
		ASSIGN_PTR(pData, Data);
		CATCHZOKPPERR
		return ok;
	}
private:
	DECL_HANDLE_EVENT;
	void   OnObjectSelection();
};

IMPL_HANDLE_EVENT(StaffCalFiltDialog)
{
	TDialog::handleEvent(event);
	if(event.isCbSelected(CTLSEL_STAFFCALFLT_OT)) {
		OnObjectSelection();
	}
	else if(event.isCbSelected(CTLSEL_STAFFCALFLT_PK)) {
		OnObjectSelection();
	}
	else if(event.isCbSelected(CTLSEL_STAFFCALFLT_OBJ)) {
		Data.LinkObjList.Set(0);
		Data.LinkObjList.Add(getCtrlLong(CTLSEL_STAFFCALFLT_OBJ));
	}
	else if(event.isCmd(cmStaffCalFiltObjList)) {
		PPID   obj_type = getCtrlLong(CTLSEL_STAFFCALFLT_OT);
		if(oneof2(obj_type, PPOBJ_PERSON, PPOBJ_STAFFLIST2)) {
			PPIDArray temp_list;
			Data.LinkObjList.CopyTo(&temp_list);
			ListToListData lst_param(obj_type, ((obj_type == PPOBJ_PERSON) ? reinterpret_cast<void *>(NZOR(Data.LinkPersonKind, PPPRK_EMPL)) : 0), &temp_list);
			if(ListToListDialog(&lst_param) > 0) {
				Data.LinkObjList.Set(&temp_list);
				if(Data.LinkObjList.GetCount() == 1) {
					setCtrlLong(CTLSEL_STAFFCALFLT_OBJ, Data.LinkObjList.GetSingle());
					disableCtrl(CTLSEL_STAFFCALFLT_OBJ, false);
				}
				else if(Data.LinkObjList.GetCount() == 0) {
					setCtrlLong(CTLSEL_STAFFCALFLT_OBJ, 0);
					disableCtrl(CTLSEL_STAFFCALFLT_OBJ, false);
				}
				else {
					SetComboBoxListText(this, CTLSEL_STAFFCALFLT_OBJ);
					disableCtrl(CTLSEL_STAFFCALFLT_OBJ, true);
				}
			}
		}
	}
	else
		return;
	clearEvent(event);
}

void StaffCalFiltDialog::OnObjectSelection()
{
	PPID   obj_type = getCtrlLong(CTLSEL_STAFFCALFLT_OT);
	PPID   single_obj_id = Data.LinkObjList.GetSingle();
	int    disbl = 0, dsbl_kind = 1;
	if(obj_type == PPOBJ_PERSON) {
		PPID   pk = NZOR(getCtrlLong(CTLSEL_STAFFCALFLT_PK), PPPRK_EMPL);
		SetupPPObjCombo(this, CTLSEL_STAFFCALFLT_OBJ, PPOBJ_PERSON, single_obj_id, 0, reinterpret_cast<void *>(pk));
		dsbl_kind = 0;
	}
	else if(obj_type == PPOBJ_STAFFLIST2)
		SetupPPObjCombo(this, CTLSEL_STAFFCALFLT_OBJ, PPOBJ_STAFFLIST2, single_obj_id, 0);
	else
		disbl = 1;
	disableCtrl(CTLSEL_STAFFCALFLT_OBJ, disbl);
	disableCtrl(CTLSEL_STAFFCALFLT_PK,  dsbl_kind);
	enableCommand(cmStaffCalFiltObjList, !disbl);
}

int PPViewStaffCal::EditBaseFilt(PPBaseFilt * pFilt)
{
	if(!Filt.IsA(pFilt))
		return 0;
	DIALOG_PROC_BODY(StaffCalFiltDialog, static_cast<StaffCalFilt *>(pFilt));
}

IMPL_CMPFUNC(StaffCalendarKey0, i1, i2)
{
	int    si = 0;
	CMPCASCADE3(si, static_cast<const StaffCalendarTbl::Key0 *>(i1), static_cast<const StaffCalendarTbl::Key0 *>(i2), CalID, DtVal, TmStart);
	return si;
}

int PPViewStaffCal::CreateEntryByObj(PPID objType, PPID objID, StrAssocArray * pNameList, int refresh)
{
	int    ok = 1;
	uint   name_pos = 0;
	PPTransaction * p_tra = 0;
	PersonPostTbl::Rec post_rec;
	if(objID && pNameList && !pNameList->Search(objID, &name_pos)) {
		PersonTbl::Rec psn_rec;
		SString obj_name;
		if(objType == PPOBJ_PERSON) {
			if(SlObj.PsnObj.Fetch(objID, &psn_rec) > 0)
				obj_name = psn_rec.Name;
			else
				ideqvalstr(objID, obj_name);
			pNameList->Add(objID, obj_name);
		}
		else if(objType == PPOBJ_PERSONPOST) {
			if(SlObj.SearchPost(objID, &post_rec) > 0)
				if(SlObj.PsnObj.Fetch(post_rec.PersonID, &psn_rec) > 0)
					obj_name = psn_rec.Name;
				else
					ideqvalstr(objID, obj_name);
			else
				ideqvalstr(objID, obj_name);
			pNameList->Add(objID, obj_name);
		}
		else if(objType == PPOBJ_STAFFLIST2) {
			PPStaffEntry sl_rec;
			if(SlObj.Fetch(objID, &sl_rec) > 0)
				obj_name = sl_rec.Name;
			else
				ideqvalstr(objID, obj_name);
			pNameList->Add(objID, obj_name);
		}
	}
	if(refresh) {
		THROW_MEM(p_tra = new PPTransaction(ppDbDependTransaction, 1));
		THROW(*p_tra);
		THROW_DB(deleteFrom(P_TempTbl, 0, P_TempTbl->LinkObjID == objID));
		Grid.RemoveRow(objID);
	}
	for(uint i = 0; i < CalList.getCount(); i++) {
		int    r = 1;
		const  PPID cal_id = CalList.get(i);
		const  PPID proj_cal_id = Filt.ProjCalID;
		STimeChunkArray list;
		MEMSZERO(post_rec);
		if(objType == PPOBJ_PERSON)
			post_rec.PersonID = objID;
		else if(objType == PPOBJ_STAFFLIST2)
			post_rec.StaffID = objID;
		else if(objType == PPOBJ_PERSONPOST)
			THROW(r = SlObj.SearchPost(objID, &post_rec));
		if(r > 0) {
			STimeChunk * p_chunk;
			ScObjAssoc sc_obj_assc;
			long   numdays = 0;
			double numhours = 0.0;
			THROW(Obj.InitScObjAssoc(cal_id, proj_cal_id, &post_rec, &sc_obj_assc));
			THROW(Obj.CalcPeriod(sc_obj_assc, Filt.Period, BIN(Filt.Flags & StaffCalFilt::fInverseProj), &numdays, &numhours, &list));
			for(uint j = 0; list.enumItems(&j, (void **)&p_chunk);) {
				int    update = 0;
				TempStaffCalTbl::Rec temp_rec;
				temp_rec.CalID = cal_id;
				temp_rec.LinkObjID = objID;
				temp_rec.Count = 1;
				if(p_chunk->GetDuration()) {
					temp_rec.TmStart = p_chunk->Start.t;
					temp_rec.TmEnd = p_chunk->Finish.t;
					temp_rec.TmVal = p_chunk->GetDuration();
				}
				else {
					temp_rec.TmStart = ZEROTIME;
					temp_rec.TmEnd = MAXDAYTIME;
					temp_rec.TmVal = p_chunk->GetDuration();
				}
				ShrinkSubstDate(Filt.Sgd, p_chunk->Start.d, &temp_rec.Dt);
				if(Filt.Sgd != sgdNone) {
					TempStaffCalTbl::Key3 k3;
					MEMSZERO(k3);
					k3.Dt        = temp_rec.Dt;
					k3.CalID     = temp_rec.CalID;
					k3.LinkObjID = objID;
					if(P_TempTbl->searchForUpdate(3, &k3, spEq) > 0) {
						update = 1;
						temp_rec.ID__ = P_TempTbl->data.ID__;
					}
				}
				if(update) {
					temp_rec.Count = (P_TempTbl->data.Count + 1);
					temp_rec.TmVal  += P_TempTbl->data.TmVal;
					STRNSCPY(temp_rec.DtText, P_TempTbl->data.DtText);
					THROW_DB(P_TempTbl->updateRecBuf(&temp_rec)); // @sfu
				}
				else {
					if(Filt.Sgd != sgdNone)
						FormatSubstDate(Filt.Sgd, temp_rec.Dt, temp_rec.DtText, sizeof(temp_rec.DtText));
					THROW_DB(P_TempTbl->insertRecBuf(&temp_rec));
				}
				temp_rec.ID__ = P_TempTbl->data.ID__;
				THROW(AddItemToTimeGrid(&temp_rec, 0));
			}
		}
	}
	if(p_tra) {
		THROW(p_tra->Commit());
	}
	CATCHZOK
	ZDELETE(p_tra);
	return ok;
}

int PPViewStaffCal::AddItemToTimeGrid(const TempStaffCalTbl::Rec * pRec, int rmv)
{
	long   row_id = pRec->LinkObjID;
	if(rmv)
		Grid.RemoveChunk(row_id, pRec->ID__);
	else {
		STimeChunk chunk;
		chunk.Start.Set(pRec->Dt, pRec->TmStart);
		chunk.Finish.Set(pRec->Dt, pRec->TmEnd);
		if(!chunk.Finish.d)
			chunk.Finish.SetFar();
		if(Grid.SetChunk(row_id, pRec->ID__, 0, &chunk) == 2) {
			SString name_buf;
			if(row_id <= 0)
				name_buf = "ALL";
			else
				ObjNameList.GetText(row_id, name_buf);
			Grid.SetRowText(row_id, name_buf, 1);
		}
	}
	return 1;
}

PP_CREATE_TEMP_FILE_PROC(CreateTempFile, TempStaffCal);

int PPViewStaffCal::Init_(const PPBaseFilt * pFilt)
{
	int    ok = 1;
	PPWaitStart();
	Grid.freeAll();
	THROW(Helper_InitBaseFilt(pFilt));
	Filt.Period.Actualize(ZERODATE);
	ObjNameList.Z();
	CalList.clear();
	if(Filt.CalList.IsEmpty()) {
		PPStaffCal cal_rec;
		for(PPID cal_id = 0; Obj.EnumItems(&cal_id, &cal_rec) > 0;) {
			if(cal_rec.LinkObjType == 0)
				CalList.add(cal_id);
		}
	}
	else
		CalList = Filt.CalList.Get();
	{
		PPIDArray temp_list;
		ZDELETE(P_TempTbl);
		THROW(P_TempTbl = CreateTempFile());
		if(CalList.getCount()) {
			uint   i;
			PPTransaction tra(ppDbDependTransaction, 1);
			THROW(tra);
			if(Filt.LinkObjType == PPOBJ_PERSON) {
				if(Filt.LinkObjList.IsEmpty()) {
					THROW(SlObj.PsnObj.GetListByKind(NZOR(Filt.LinkPersonKind, PPPRK_EMPL), &temp_list, 0));
					Filt.LinkObjList.Set(&temp_list);
				}
				const uint c = Filt.LinkObjList.GetCount();
				for(i = 0; i < c; i++) {
					THROW(CreateEntryByObj(PPOBJ_PERSON, Filt.LinkObjList.Get().get(i), &ObjNameList));
					PPWaitPercent(i+1, c);
				}
			}
			else if(Filt.LinkObjType == PPOBJ_PERSONPOST) {
				if(Filt.LinkObjList.IsEmpty()) {
					PPIDArray sl_list;
					PersonPostArray post_list;
					PPObjStaffList::Filt sl_filt;
					MEMSZERO(sl_filt);
					THROW(SlObj.GetList(sl_filt, &sl_list, 0));
					for(i = 0; i < sl_list.getCount(); i++) {
						THROW(SlObj.GetPostList(sl_list.get(i), &post_list));
					}
					for(i = 0; i < post_list.getCount(); i++) {
						Filt.LinkObjList.Add(post_list.at(i).ID);
					}
				}
				const uint c = Filt.LinkObjList.GetCount();
				for(i = 0; i < c; i++) {
					THROW(CreateEntryByObj(PPOBJ_PERSONPOST, Filt.LinkObjList.Get().get(i), &ObjNameList));
					PPWaitPercent(i+1, c);
				}
			}
			else if(Filt.LinkObjType == PPOBJ_STAFFLIST2) {
				if(Filt.LinkObjList.IsEmpty()) {
					PPObjStaffList::Filt sl_filt;
					MEMSZERO(sl_filt);
					THROW(SlObj.GetList(sl_filt, &temp_list, 0));
					Filt.LinkObjList.Set(&temp_list);
				}
				const uint c = Filt.LinkObjList.GetCount();
				for(i = 0; i < c; i++) {
					THROW(CreateEntryByObj(PPOBJ_STAFFLIST2, Filt.LinkObjList.Get().get(i), &ObjNameList));
					PPWaitPercent(i+1, c);
				}
			}
			else { // base calendars
				THROW(CreateEntryByObj(0, 0, &ObjNameList));
			}
			{
				//
				// Инициализируем индекс наименования объекта в записях временной таблицы
				//
				ObjNameList.SortByText();
				TempStaffCalTbl::Key0 k0;
				MEMSZERO(k0);
				if(P_TempTbl->searchForUpdate(0, &k0, spFirst))
					do {
						PPID   obj_id = P_TempTbl->data.LinkObjID;
						uint   name_pos = 0;
						if(ObjNameList.Search(obj_id, &name_pos)) {
							P_TempTbl->data.NameIdx = (long)name_pos;
							THROW_DB(P_TempTbl->updateRec()); // @sfu
						}
					} while(P_TempTbl->searchForUpdate(0, &k0, spNext));
			}
			THROW(tra.Commit());
		}
	}
	CATCH
		ZDELETE(P_TempTbl);
		ok = 0;
	ENDCATCH
	PPWaitStop();
	return ok;
}

int PPViewStaffCal::InitIteration(int ord)
{
	int    ok = 0;
	int    idx = 0;
	union {
		TempStaffCalTbl::Key0 k0;
		TempStaffCalTbl::Key1 k1;
		TempStaffCalTbl::Key2 k2;
	} k, k_;
	MEMSZERO(k);
	BExtQuery::ZDelete(&P_IterQuery);
	if(P_TempTbl) {
		if(ord == ordByDate)
			idx = 1;
		else if(ord == ordByObject)
			idx = 2;
		else
			idx = 0;
		P_IterQuery = new BExtQuery(P_TempTbl, idx);
		P_IterQuery->selectAll();
		k_ = k;
		Counter.Init(P_IterQuery->countIterations(0, &k_, spFirst));
		P_IterQuery->initIteration(false, &k, spFirst);
		ok = 1;
	}
	return ok;
}

int FASTCALL PPViewStaffCal::NextIteration(StaffCalViewItem * pItem)
{
	if(P_IterQuery && P_IterQuery->nextIteration() > 0) {
		if(pItem) {
			TempStaffCalTbl::Rec & r_rec = P_TempTbl->data;
			memzero(pItem, sizeof(*pItem));
			pItem->CalID = r_rec.CalID;
			pItem->Dt = r_rec.Dt;
			pItem->TmStart = r_rec.TmStart;
			pItem->TmEnd   = r_rec.TmEnd;
			pItem->Duration = r_rec.TmVal;
			pItem->LinkObj.Obj = Filt.LinkObjType;
			pItem->LinkObj.Id  = r_rec.LinkObjID;
			pItem->Count       = r_rec.Count;
			STRNSCPY(pItem->DtText, r_rec.DtText);
			uint pos = 0;
			if(ObjNameList.Search(r_rec.LinkObjID, &pos))
				STRNSCPY(pItem->LinkObjName, ObjNameList.Get(pos).Txt);
		}
		Counter.Increment();
		return 1;
	}
	else
		return -1;
}

DBQuery * PPViewStaffCal::CreateBrowserQuery(uint * pBrwId, SString * pSubTitle)
{
	DBQuery * p_q = 0;
	uint   brw_id = 0; //BROWSER_STAFFCAL;
	DBE    dbe_objname, dbe_cal, dbe_duration;
	TempStaffCalTbl * p_t = 0;
	if(Filt.Sgd != sgdNone)
		brw_id = BROWSER_STAFFCAL_DTGRP;
	else if(Filt.Order == ordByObject)
		brw_id = BROWSER_STAFFCAL_OBJ;
	else
		brw_id = BROWSER_STAFFCAL_DT;
	THROW_PP(P_TempTbl, PPERR_PPVIEWNOTINITED);
	p_t = new TempStaffCalTbl(P_TempTbl->GetFileName());
	PPDbqFuncPool::InitObjNameFunc(dbe_cal, PPDbqFuncPool::IdObjNameStaffCal, p_t->CalID);
	if(Filt.LinkObjType == PPOBJ_PERSON)
		PPDbqFuncPool::InitObjNameFunc(dbe_objname, PPDbqFuncPool::IdObjNamePerson, p_t->LinkObjID);
	else if(Filt.LinkObjType == PPOBJ_STAFFLIST2)
		PPDbqFuncPool::InitObjNameFunc(dbe_objname, PPDbqFuncPool::IdObjNameStaff, p_t->LinkObjID);
	else if(Filt.LinkObjType == PPOBJ_PERSONPOST)
		PPDbqFuncPool::InitObjNameFunc(dbe_objname, PPDbqFuncPool::IdObjNamePersonPost, p_t->LinkObjID);
	else {
		dbe_objname.init();
		dbe_objname.push(static_cast<DBFunc>(PPDbqFuncPool::IdEmpty));
	}
	{
		dbe_duration.init();
		dbe_duration.push(p_t->TmVal);
		dbe_duration.push(static_cast<DBFunc>(PPDbqFuncPool::IdDurationToTime));
	}
	p_q = &select(
		p_t->ID__,      // #0
		p_t->CalID,     // #1
		p_t->LinkObjID, // #2
		p_t->Dt,        // #3
		p_t->TmStart,   // #4
		p_t->TmEnd,     // #5
		dbe_duration,   // #6
		dbe_cal,        // #7
		dbe_objname,    // #8
		p_t->DtText,    // #9
		p_t->Count,     // #10
		0).from(p_t, 0);
	if(Filt.Order == ordByDate)
		p_q->orderBy(p_t->Dt, 0L);
	else if(Filt.Order == ordByObject)
		p_q->orderBy(p_t->NameIdx, 0L);
	else
		p_q->orderBy(p_t->Dt, 0L);
	if(pSubTitle) {
		SString temp_buf;
		pSubTitle->Z().Cat(Filt.Period, 1);
		//StaffCalFilt ObjIdListFilt
		PPID obj_id = Filt.LinkObjList.GetSingle();
		if(Filt.LinkObjType && obj_id) {
			GetObjectName(Filt.LinkObjType, obj_id, temp_buf);
			pSubTitle->CatDivIfNotEmpty('-', 1).Cat(temp_buf);
		}
		PPID cal_id = Filt.CalList.GetSingle();
		if(cal_id) {
			GetObjectName(PPOBJ_STAFFCAL, cal_id, temp_buf);
			pSubTitle->CatDivIfNotEmpty('-', 1).Cat(temp_buf);
		}
	}
	CATCH
		if(p_q) {
			ZDELETE(p_q);
		}
		else
			ZDELETE(p_t);
	ENDCATCH
	ASSIGN_PTR(pBrwId, brw_id);
	return p_q;
}

int PPViewStaffCal::EditEntry(PPID calID, PPID objID, LDATETIME dtm, LTIME tmFinish)
{
	int    ok = -1;
	if(Filt.Sgd == sgdNone) {
		if(calID && objID && dtm.d) {
			PPID   child_cal_id = 0;
			PPStaffCal cal_rec, par_rec;
			PPStaffCalPacket pack;
			if(Filt.LinkObjType && Obj.Fetch(calID, &par_rec) > 0) {
				StaffCalendarTbl::Rec entry;
				PPObjID oid(Filt.LinkObjType, objID);
				if(Obj.SearchByObj(calID, oid, &cal_rec) > 0) {
					child_cal_id = cal_rec.ID;
					THROW(Obj.GetPacket(child_cal_id, &pack) > 0);
				}
				else {
					pack.Rec.LinkCalID   = calID;
					pack.Rec.LinkObjType = oid.Obj;
					pack.Rec.LinkObjID   = oid.Id;
				}
				{
					int    is_entry = 0;
					uint   pos = 0;
					if(pack.Get(dtm.d, 0, &pos) > 0) {
						is_entry = 1;
						entry = pack.Items.at(pos);
						pack.Items.atFree(pos);
					}
					else {
						MEMSZERO(entry);
						entry.ObjID = objID;
						entry.Kind = CALDATE::kDate;
						entry.DtVal = (long)dtm.d;
						entry.TmStart = dtm.t;
						entry.TmEnd = tmFinish;
					}
				}
				if(Obj.EditEntry(&pack, &entry) > 0) {
					uint   new_pos = 0;
					THROW(pack.AddItem(&entry, &new_pos));
					THROW(Obj.PutPacket(&child_cal_id, &pack, 1));
					THROW(CreateEntryByObj(Filt.LinkObjType, objID, &ObjNameList, 1));
            		ok = 1;
				}
			}
		}
	}
	CATCHZOKPPERR
	return ok;
}

int PPViewStaffCal::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	if(ok == -2) {
		switch(ppvCmd) {
			case PPVCMD_EDIT:
				{
					const BrwHdr * p_hdr = pHdr ? static_cast<const BrwHdr *>(pHdr) : 0;
					if(p_hdr)
						ok = EditEntry(p_hdr->CalID, p_hdr->ObjID, p_hdr->StartDtm, p_hdr->TmEnd);
				}
				break;
			case PPVCMD_CHANGEFILT:
				{
					STimeChunkBrowser * p_brw = PPFindLastTimeChunkBrowser();
					ok = ChangeFilt(0, pBrw);
					if(p_brw)
						if(ok == 2) {
							PPCloseBrowser(p_brw);
							TimeChunkBrowser();
							::SetFocus(pBrw->H());
						}
						else if(ok > 0)
							p_brw->UpdateData();
				}
				break;
			case PPVCMD_TIMEGRAPH:
				ok = -1;
				TimeChunkBrowser();
				break;
		}
	}
	return ok;
}

int PPViewStaffCal::Detail(const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = -1;
	PPID   rec_id = pHdr ? *static_cast<const  PPID *>(pHdr) : 0;
	if(rec_id) {
		TempStaffCalTbl::Rec rec;
		if(SearchByID(P_TempTbl, 0, rec_id, &rec) > 0) {
			if(Filt.Sgd != sgdNone) {
				StaffCalFilt filt;
				filt = Filt;
				filt.Sgd = sgdNone;
				filt.CalList.Z().Add(rec.CalID);
				filt.LinkObjList.Z().Add(rec.LinkObjID);
				ExpandSubstDate(Filt.Sgd, rec.Dt, &filt.Period);
				ok = PPView::Execute(PPVIEW_STAFFCAL, &filt, 0, 0);
			}
			else if(Filt.LinkObjType && rec.LinkObjID) {
				StaffCalFilt sc_flt;
				if(rec.CalID) {
					PPStaffCal sc_rec;
					if(Obj.SearchByObj(rec.CalID, PPObjID(Filt.LinkObjType, rec.LinkObjID), &sc_rec) > 0) {
						sc_flt.Period.SetDate(rec.Dt);
						if(Obj.Edit(&sc_rec.ID, &sc_flt) == cmOK) {
							CreateEntryByObj(Filt.LinkObjType, rec.LinkObjID, &ObjNameList, 1);
							ok = 1;
						}
					}
				}
				else {
					sc_flt.LinkObjType = Filt.LinkObjType;
					sc_flt.LinkObjList.Add(rec.LinkObjID);
					Obj.Browse(&sc_flt);
				}
			}
		}
	}
	return ok;
}

int PPViewStaffCal::Print(const void *)
{
	return Helper_Print((Filt.Sgd != sgdNone) ? REPORT_STAFFCALDTGRPNG : REPORT_STAFFCAL, Filt.Order);
}
//
//
//
PPViewStaffCal::StaffCalTimeChunkGrid::StaffCalTimeChunkGrid(PPViewStaffCal * pV) : STimeChunkGrid(), P_View(pV)
{
}

PPViewStaffCal::StaffCalTimeChunkGrid::~StaffCalTimeChunkGrid()
{
}

int PPViewStaffCal::StaffCalTimeChunkGrid::GetText(int item, long id, SString & rBuf)
{
	int    ok = -1;
	if(item == iTitle) {
		rBuf = "TEST";
		ok = 1;
	}
	else if(item == iRow)
		ok = (id < 0) ? 1 : STimeChunkGrid::GetText(item, id, rBuf);
	else if(item == iChunk)
		ok = P_View->GetTimeGridItemText(id, rBuf);
	return ok;
}

int PPViewStaffCal::GetTimeGridItemText(PPID id, SString & rBuf)
{
	int    ok = -1;
	if(P_TempTbl && P_TempTbl->search(0, &id, spEq) > 0) {
		LDATETIME duration;
		duration.settotalsec(P_TempTbl->data.TmVal);
		rBuf.Z().Cat(duration.t);
		ok = 1;
	}
	return ok;
}

int PPViewStaffCal::StaffCalTimeChunkGrid::Edit(int item, long rowID, const LDATETIME & rTm, long * pID)
{
	return -1;
}

int PPViewStaffCal::StaffCalTimeChunkGrid::MoveChunk(int mode, long id, long rowId, const STimeChunk & rNewChunk)
{
	return -1;
}

int PPViewStaffCal::StaffCalTimeChunkGrid::GetColor(long id, STimeChunkGrid::Color * pClr)
{
	int    ok = 1;
	PPStaffCal staff_cal;
	TempStaffCalTbl * p_temp_tbl = (P_View) ? P_View->P_TempTbl : 0;
	if(p_temp_tbl && p_temp_tbl->search(0, &id, spEq) > 0 && P_View->Obj.Fetch(p_temp_tbl->data.CalID, &staff_cal) > 0) {
		pClr->C = staff_cal.Color;
		ok = 1;
		if(pClr->C != 0)
			pClr->C--;
	}
	else
		ok = -1;
	SETIFZ(pClr->C, SClrCoral);
	return ok;
}

int PPViewStaffCal::UpdateTimeBrowser(int destroy)
{
	return PPView::UpdateTimeBrowser(&Grid, 0, destroy);
}

int PPViewStaffCal::TimeChunkBrowser()
{
	PPPersonConfig psn_cfg;
	UpdateTimeBrowser(1);
	PPTimeChunkBrowser * p_brw = new PPTimeChunkBrowser;
	STimeChunkBrowser::Param p, saved_params;
	InitSTimeChunkBrowserParam("PPViewStaffCal", &p);
	saved_params.RegSaveParam = p.RegSaveParam;
	PPObjPerson::ReadConfig(&psn_cfg);
	p.Quant = NZOR(psn_cfg.StaffCalQuant, (15 * 60));
	p.Quant = 15 * 60;
	p.PixQuant = 10;
	p.PixRow  = 20;
	p.PixRowMargin = 4;
	p.HdrLevelHeight = 20;
	p.DefBounds.Set(Filt.Period.low, plusdate(Filt.Period.upp, 30));
	if(p_brw->RestoreParameters(saved_params) > 0) {
		p.PixQuant     = saved_params.PixQuant;
		p.PixRow       = saved_params.PixRow;
		p.PixRowMargin = saved_params.PixRowMargin;
		p.TextZonePart = saved_params.TextZonePart;
	}
	p_brw->SetBmpId(STimeChunkBrowser::bmpModeGantt, BM_TIMEGRAPH_GANTT);
	p_brw->SetBmpId(STimeChunkBrowser::bmpModeHourDay, BM_TIMEGRAPH_HOURDAY);
	p_brw->SetBmpId(STimeChunkBrowser::bmpBack, BM_BACK);
	p_brw->SetParam(&p);
	p_brw->SetData(&Grid, 0);
	p_brw->SetResID(static_cast<PPApp *>(APPL)->LastCmd);
	InsertView(p_brw);
	return 1;
}
//
// Implementation of PPALDD_StaffCalView
//
PPALDD_CONSTRUCTOR(StaffCalView)
{
	InitFixData(rscDefHdr, &H, sizeof(H));
	InitFixData(rscDefIter, &I, sizeof(I));
}

PPALDD_DESTRUCTOR(StaffCalView) { Destroy(); }

int PPALDD_StaffCalView::InitData(PPFilt & rFilt, long rsrv)
{
	INIT_PPVIEW_ALDD_DATA_U(StaffCal, rsrv);
	H.FltBeg = p_filt->Period.low;
	H.FltEnd = p_filt->Period.upp;
	H.FltFlags       = p_filt->Flags;
	H.FltLinkObjType = p_filt->LinkObjType;
	H.FltSingleLinkObjID = p_filt->LinkObjList.GetSingle();
	H.FltSingleCalID     = p_filt->CalList.GetSingle();
	H.FltOrder       = p_filt->Order;
	{
		SString temp_buf;
		if(p_filt->LinkObjType && H.FltSingleLinkObjID)
			GetObjectName(p_filt->LinkObjType, H.FltSingleLinkObjID, temp_buf);
		temp_buf.CopyTo(H.FltSingleObjName, sizeof(H.FltSingleObjName));
	}
	return DlRtm::InitData(rFilt, rsrv);
}

int PPALDD_StaffCalView::InitIteration(long iterId, int sortId, long rsrv)
{
	INIT_PPVIEW_ALDD_ITER_ORD(StaffCal, sortId);
}

int PPALDD_StaffCalView::NextIteration(long iterId)
{
	START_PPVIEW_ALDD_ITER(StaffCal);
	SString temp_buf;
	I.CalID       = item.CalID;
	I.Dt  = item.Dt;
	I.TmStart     = item.TmStart;
	I.TmEnd       = item.TmEnd;
	I.Duration    = item.Duration;
	I.Flags       = item.Flags;
	I.LinkObjType = item.LinkObj.Obj;
	I.LinkObjID   = item.LinkObj.Id;
	I.Count       = item.Count;
	STRNSCPY(I.DtText, item.DtText);
	if(I.LinkObjType && I.LinkObjID)
		GetObjectName(I.LinkObjType, I.LinkObjID, temp_buf);
	temp_buf.CopyTo(I.LinkObjName, sizeof(I.LinkObjName));
	temp_buf.Z();
	LDATETIME dtm = ZERODATETIME;
	long days = dtm.settotalsec(item.Duration);
	if(days)
		temp_buf.Cat(days).CatChar('d').Space();
	temp_buf.Cat(dtm.t, TIMF_HMS);
	temp_buf.CopyTo(I.TxtDur, sizeof(I.TxtDur));
	FINISH_PPVIEW_ALDD_ITER();
}

void PPALDD_StaffCalView::Destroy() { DESTROY_PPVIEW_ALDD(StaffCal); }
