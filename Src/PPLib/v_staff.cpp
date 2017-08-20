// V_STAFF.CPP
// Copyright (c) A.Sobolev 2007, 2009, 2015, 2016, 2017
//
#include <pp.h>
#pragma hdrstop
//
// @ModuleDef(PPViewStaffList)
//
IMPLEMENT_PPFILT_FACTORY(StaffList); SLAPI StaffListFilt::StaffListFilt() : PPBaseFilt(PPFILT_STAFFLIST, 0, 0)
{
	SetFlatChunk(offsetof(StaffListFilt, ReserveStart),
		offsetof(StaffListFilt, Reserve)-offsetof(StaffListFilt, ReserveStart)+sizeof(Reserve));
	Init(1, 0);
}

int SLAPI StaffListFilt::Init(int fullyDestroy, long extraData)
{
	PPBaseFilt::Init(fullyDestroy, extraData);
	if(extraData)
		if(GetMainEmployerID(&OrgID) && extraData > 0)
			DivID = extraData;
	return 1;
}
//
//
//
SLAPI PPViewStaffList::PPViewStaffList() : PPView(&SlObj, &Filt, PPVIEW_STAFFLIST)
{
	ImplementFlags |= (implBrowseArray /* @v9.0.4 |implOnAddSetupPos*/ );
	DefReportId = REPORT_STAFFLIST;
	int     type_no;
	SString buf;
	PPAmountType    at_rec;
	PPObjAmountType at_obj;
	PPLoadText(PPTXT_SALARY_AMOUNT_TYPES, buf);
	memzero(SalaryAmountTypes, sizeof(PPID) * 4);
	for(PPID id = 0; at_obj.EnumItems(&id, &at_rec) > 0;)
		if((at_rec.Flags & PPAmountType::fStaffAmount) && PPSearchSubStr(buf, &type_no, at_rec.Symb, 1) && type_no < 4)
			SalaryAmountTypes[type_no] = at_rec.ID;
}

void * SLAPI PPViewStaffList::GetEditExtraParam()
{
	MEMSZERO(TempExtra);
	TempExtra.OrgID = Filt.OrgID;
	TempExtra.DivID = Filt.DivID;
	return &TempExtra;
}

int SLAPI PPViewStaffList::EditBaseFilt(PPBaseFilt * pFilt)
{
#define GRP_DIV 1
	int    ok = -1, valid_data = 0;
	TDialog * dlg = 0;
	THROW(Filt.IsA(pFilt));
	THROW(CheckDialogPtr(&(dlg = new TDialog(DLG_STAFFLFLT))));
	{
		StaffListFilt * p_filt = (StaffListFilt *)pFilt;
		dlg->addGroup(GRP_DIV, new DivisionCtrlGroup(CTLSEL_STAFFLFLT_ORG, CTLSEL_STAFFLFLT_DIV, 0, 0));
		DivisionCtrlGroup::Rec grp_rec(p_filt->OrgID, p_filt->DivID);
		dlg->setGroupData(GRP_DIV, &grp_rec);
		while(!valid_data && ExecView(dlg) == cmOK)
			if(dlg->getGroupData(GRP_DIV, &grp_rec)) {
				p_filt->OrgID = grp_rec.OrgID;
				p_filt->DivID = grp_rec.DivID;
				ok = valid_data = 1;
			}
			else
				PPError();
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
#undef GRP_DIV
}

//static
void FASTCALL PPViewStaffList::MakeListEntry(const PPStaffEntry & rSrc, PPViewStaffList::BrwEntry & rEntry)
{
	MEMSZERO(rEntry);
	rEntry.ID = rSrc.ID;
	rEntry.OrgID = rSrc.OrgID;
	rEntry.DivisionID = rSrc.DivisionID;
	rEntry.Flags = rSrc.Flags;
	rEntry.FixedStaff = rSrc.FixedStaff;
	rEntry.VacancyCount = rSrc.VacancyCount;
	rEntry.VacancyBusy = rSrc.VacancyBusy;
}

int FASTCALL PPViewStaffList::CheckForFilt(const PPStaffEntry & rItem) const
{
	return BIN((!Filt.OrgID || rItem.OrgID == Filt.OrgID) && (!Filt.DivID || rItem.DivisionID == Filt.DivID));
}

IMPL_CMPCFUNC(PPViewStaffList_BrwEntry_Name, p1, p2)
{
	int   si = 0;
	PPObjStaffList * p_obj = (PPObjStaffList *)pExtraData;
	if(p_obj) {
		const PPViewStaffList::BrwEntry * i1 = (const PPViewStaffList::BrwEntry *)p1;
		const PPViewStaffList::BrwEntry * i2 = (const PPViewStaffList::BrwEntry *)p2;
		PPStaffEntry e1, e2;
        p_obj->Fetch(i1->ID, &e1);
        p_obj->Fetch(i2->ID, &e2);
        si = stricmp866(e1.Name, e2.Name);
	}
	return si;
}

int SLAPI PPViewStaffList::FetchData(PPID id)
{
	int    ok = 1;
	BrwEntry entry;
	if(id == 0) {
		Data.clear();
		SEnum en;
		Reference * p_ref = PPRef;
		if(Filt.DivID)
			en = p_ref->EnumByIdxVal(PPOBJ_STAFFLIST2, 2, Filt.DivID);
		else if(Filt.OrgID)
			en = p_ref->EnumByIdxVal(PPOBJ_STAFFLIST2, 1, Filt.OrgID);
		else
			en = p_ref->Enum(PPOBJ_STAFFLIST2, 0);
		PPStaffEntry rec;
		while(en.Next(&rec) > 0) {
			if(CheckForFilt(rec)) {
                BrwEntry entry;
				MakeListEntry(rec, entry);
                THROW_SL(Data.insert(&entry));
			}
		}
	}
	else {
		uint   pos = 0;
		int    found = BIN(Data.lsearch(&id, &pos, CMPF_LONG) > 0);
		PPStaffEntry rec;
		if(SlObj.Search(id, &rec) > 0 && CheckForFilt(rec) > 0) {
			MakeListEntry(rec, entry);
			if(found)
				Data.at(pos) = entry;
			else
				THROW_SL(Data.insert(&entry));
		}
		else if(found)
			THROW_SL(Data.atFree(pos));
	}
	Data.sort(PTR_CMPCFUNC(PPViewStaffList_BrwEntry_Name), &SlObj);
	CATCHZOK
	return ok;
}

int SLAPI PPViewStaffList::Init_(const PPBaseFilt * pFilt)
{
	int    ok = 1;
	THROW(Helper_InitBaseFilt(pFilt));
	Data.freeAll();
	{
		SEnum en;
		Reference * p_ref = PPRef;
		if(Filt.DivID)
			en = p_ref->EnumByIdxVal(PPOBJ_STAFFLIST2, 2, Filt.DivID);
		else if(Filt.OrgID)
			en = p_ref->EnumByIdxVal(PPOBJ_STAFFLIST2, 1, Filt.OrgID);
		else
			en = p_ref->Enum(PPOBJ_STAFFLIST2, 0);
		PPStaffEntry rec;
		while(en.Next(&rec) > 0) {
			if((!Filt.OrgID || rec.OrgID == Filt.OrgID) && (!Filt.DivID || rec.DivisionID == Filt.DivID)) {
                BrwEntry entry;
                MEMSZERO(entry);
                entry.ID = rec.ID;
                entry.OrgID = rec.OrgID;
                entry.DivisionID = rec.DivisionID;
                entry.Flags = rec.Flags;
                entry.FixedStaff = rec.FixedStaff;
                entry.VacancyCount = rec.VacancyCount;
                entry.VacancyBusy = rec.VacancyBusy;
                THROW_SL(Data.insert(&entry));
			}
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PPViewStaffList::InitIteration()
{
	int    ok = 1;
	Counter.Init(Data.getCount());
	return ok;
}

int FASTCALL PPViewStaffList::NextIteration(StaffListViewItem * pItem)
{
	int    ok = -1;
	for(; ok < 0 && Counter < Data.getCount(); Counter.Increment()) {
		const BrwEntry & r_entry = Data.at(Counter);
		if(pItem) {
			PPStaffEntry se;
			if(SlObj.Search(r_entry.ID, &se) > 0) {
				*(PPStaffEntry *)pItem = se;
				ok = 1;
			}
		}
		else
			ok = 1;
	}
	return ok;
}

// static
int PPViewStaffList::GetDataForBrowser(SBrowserDataProcBlock * pBlk)
{
	PPViewStaffList * p_v = (PPViewStaffList *)pBlk->ExtraPtr;
	return p_v ? p_v->_GetDataForBrowser(pBlk) : 0;
}

int SLAPI PPViewStaffList::_GetDataForBrowser(SBrowserDataProcBlock * pBlk)
{
	int    ok = 0;
	if(pBlk->P_SrcData && pBlk->P_DestData) {
		ok = 1;
		SString temp_buf;
		BrwEntry * p_item = (BrwEntry *)pBlk->P_SrcData;
		switch(pBlk->ColumnN) {
			case 0: // ИД
				pBlk->Set(p_item->ID);
				break;
			case 1: // Наименование
				{
					PPStaffEntry se;
					if(SlObj.Fetch(p_item->ID, &se) > 0)
						temp_buf = se.Name;
					else
						ideqvalstr(p_item->ID, temp_buf.Z());
					pBlk->Set(temp_buf);
				}
				break;
			case 2: // Подразделение
				if(p_item->DivisionID)
					GetLocationName(p_item->DivisionID, temp_buf);
				else
					temp_buf = 0;
				pBlk->Set(temp_buf);
				break;
			case 3: // Rank
				pBlk->Set(p_item->Rank);
				break;
			case 4: // Количество ставок
				pBlk->Set((int32)p_item->VacancyCount);
				break;
			case 5: // Занято вакансий
				pBlk->Set((int32)p_item->VacancyBusy);
				break;
			case 6: // Группа начислений
				temp_buf = 0;
				if(p_item->ChargeGrpID) {
					PPObjSalCharge sc_obj;
					PPSalChargePacket sc_pack;
					if(sc_obj.Fetch(p_item->ChargeGrpID, &sc_pack) > 0)
						temp_buf = sc_pack.Rec.Name;
					else
						ideqvalstr(p_item->ChargeGrpID, temp_buf.Z());
				}
				pBlk->Set(temp_buf);
				break;
		}
	}
	return ok;
}

int SLAPI PPViewStaffList::PreprocessBrowser(PPViewBrowser * pBrw)
{
	int    ok = -1;
	if(pBrw) {
		pBrw->SetDefUserProc(PPViewStaffList::GetDataForBrowser, this);
		ok = 1;
	}
	return ok;
}

SArray * SLAPI PPViewStaffList::CreateBrowserArray(uint * pBrwId, SString * pSubTitle)
{
	SArray * p_array = new SArray(Data);
	uint   brw_id = BROWSER_STAFFLIST;
	if(pSubTitle) {
		PersonTbl::Rec psn_rec;
		*pSubTitle = 0;
		if(SlObj.PsnObj.Fetch(Filt.OrgID, &psn_rec) > 0)
			pSubTitle->CatDivIfNotEmpty('-', 1).Cat(psn_rec.Name);
		if(Filt.DivID) {
			SString div_name;
			if(GetLocationName(Filt.DivID, div_name) > 0)
				pSubTitle->CatDivIfNotEmpty('-', 1).Cat(div_name);
		}
	}
	ASSIGN_PTR(pBrwId, brw_id);
	return p_array;
}

int SLAPI PPViewStaffList::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	const  PPID   id = pHdr ? *(PPID *)pHdr : 0;
	int    ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	if(ok == -2) {
		if(ppvCmd == PPVCMD_STAFFPOST) {
			ok = -1;
			if(id) {
				StaffPostFilt sp_filt;
				sp_filt.StaffID = id;
				sp_filt.Flags &= ~StaffPostFilt::fClosedOnly;
				sp_filt.Flags |= StaffPostFilt::fOpenedOnly;
				PPView::Execute(PPVIEW_STAFFPOST, &sp_filt, PPView::exefModeless, 0);
				ok = 1;
			}
		}
		else if(ppvCmd == PPVCMD_STAFFCAL) {
			ok = -1;
			if(id) {
				StaffCalFilt sc_flt;
				sc_flt.LinkObjType = PPOBJ_STAFFLIST2;
				sc_flt.LinkObjList.Add(id);
				ShowObjects(PPOBJ_STAFFCAL, &sc_flt);
			}
		}
		else if(ppvCmd == PPVCMD_AMOUNTS) {
			ok = -1;
			if(id && SlObj.EditAmounts(id) > 0)
				ok = 1;
		}
		else if(ppvCmd == PPVCMD_SALARY) {
			ok = -1;
			if(id) {
				SalaryFilt s_flt;
				s_flt.StaffID = id;
				PPView::Execute(PPVIEW_SALARY, &s_flt, PPView::exefModeless, 0);
			}
		}
		else if(ppvCmd == PPVCMD_DORECOVER) {
			ok = -1;
			int    ta = 0;
			StaffListViewItem item;
			if(PPStartTransaction(&ta, 1)) {
				for(InitIteration(); ok && NextIteration(&item) > 0;) {
					int r = SlObj.Recover(item.ID, 0);
					if(!r)
						ok = 0;
					else if(r == 2)
						ok = 1;
				}
				if(ok) {
					if(!PPCommitWork(&ta))
						ok = 0;
				}
				else
					PPRollbackWork(&ta);
			}
			else
				ok = 0;
			if(!ok)
				PPError();
		}
	}
	if(ok > 0) {
		AryBrowserDef * p_def = (AryBrowserDef *)pBrw->getDef();
		if(p_def) {
			LongArray id_list;
			PPID   last_id = 0;
			if(GetLastUpdatedObjects(id, id_list) > 0) {
				for(uint i = 0; i < id_list.getCount(); i++) {
					last_id = id_list.get(i);
					FetchData(last_id);
				}
			}
			if(last_id) {
				p_def->setArray(new SArray(Data), 0, 1);
				if(ppvCmd != PPVCMD_DELETEITEM)
					pBrw->search2(&last_id, CMPF_LONG, srchFirst, 0);
			}
		}
	}
	/*
	if(ok > 0) {
		FetchData(id);
		AryBrowserDef * p_def = (AryBrowserDef *)pBrw->getDef();
		if(p_def) {
			long   c = p_def->_curItem();
			p_def->setArray(new SArray(Data), 0, 1);
			pBrw->go(c);
		}
		ok = 1;
	}
	*/
	return ok;
}

PPID SLAPI PPViewStaffList::GetSalaryAmountType(int typeNo)
{
	return SalaryAmountTypes[typeNo];
}
//
// @ModuleDef(PPViewStaffPost)
//
IMPLEMENT_PPFILT_FACTORY(StaffPost); SLAPI StaffPostFilt::StaffPostFilt() :
	PPBaseFilt(PPFILT_STAFFPOST, 0, 1) // @v6.2.2 ver 0-->1
{
	SetFlatChunk(offsetof(StaffPostFilt, ReserveStart),
		offsetof(StaffPostFilt, Reserve)-offsetof(StaffPostFilt, ReserveStart)+sizeof(Reserve));
	Init(1, 0);
}

SLAPI PPViewStaffPost::PPViewStaffPost() : PPView(0, &Filt, PPVIEW_STAFFPOST)
{
	P_TempTbl = 0;
}

SLAPI PPViewStaffPost::~PPViewStaffPost()
{
	delete P_TempTbl;
}

void * SLAPI PPViewStaffPost::GetEditExtraParam()
{
	return 0;
}

PPBaseFilt * PPViewStaffPost::CreateFilt(void * extraPtr) const
{
	StaffPostFilt * p_filt = new StaffPostFilt;
	p_filt->Flags |= StaffPostFilt::fOpenedOnly;
	PPID   org_id = 0;
	if(GetMainEmployerID(&org_id))
		p_filt->DivID = org_id;
	return p_filt;
}

int SLAPI PPViewStaffPost::EditBaseFilt(PPBaseFilt * pBaseFilt)
{
#define GRP_DIV 1
	int    ok = -1, valid_data = 0;
	TDialog * dlg = 0;
	THROW(Filt.IsA(pBaseFilt));
	THROW(CheckDialogPtr(&(dlg = new TDialog(DLG_PPOSTFLT))));
	{
		StaffPostFilt * p_filt = (StaffPostFilt *)pBaseFilt;
		dlg->SetupCalPeriod(CTLCAL_PPOSTFLT_PERIOD, CTL_PPOSTFLT_PERIOD);
		dlg->SetupCalPeriod(CTLCAL_PPOSTFLT_FNPERIOD, CTL_PPOSTFLT_FNPERIOD);
		dlg->addGroup(GRP_DIV, new DivisionCtrlGroup(CTLSEL_PPOSTFLT_ORG, CTLSEL_PPOSTFLT_DIV, CTLSEL_PPOSTFLT_STAFF, 0));
		DivisionCtrlGroup::Rec grp_rec(p_filt->OrgID, p_filt->DivID, p_filt->StaffID);
		dlg->setGroupData(GRP_DIV, &grp_rec);
		SetPeriodInput(dlg, CTL_PPOSTFLT_PERIOD, &p_filt->Period);
		SetPeriodInput(dlg, CTL_PPOSTFLT_FNPERIOD, &p_filt->FnPeriod);
		ushort v = 0;
		long   f = CheckXORFlags(p_filt->Flags, StaffPostFilt::fOpenedOnly, StaffPostFilt::fClosedOnly);
		if(f & StaffPostFilt::fOpenedOnly)
			v = 1;
		else if(f & StaffPostFilt::fClosedOnly)
			v = 2;
		dlg->setCtrlData(CTL_PPOSTFLT_OPENED, &v);
		while(!valid_data && ExecView(dlg) == cmOK) {
			int    local_ok = 1;
			uint   sel = 0;
			if(!dlg->getGroupData(GRP_DIV, &grp_rec))
				local_ok = 0;
			else if(!GetPeriodInput(dlg, sel = CTL_PPOSTFLT_PERIOD, &p_filt->Period))
				local_ok = 0;
			else if(!GetPeriodInput(dlg, sel = CTL_PPOSTFLT_FNPERIOD, &p_filt->FnPeriod))
				local_ok = 0;
			else {
				p_filt->OrgID = grp_rec.OrgID;
				p_filt->DivID = grp_rec.DivID;
				p_filt->StaffID = grp_rec.StaffID;
				dlg->getCtrlData(CTL_PPOSTFLT_OPENED, &v);
				p_filt->Flags &= ~(StaffPostFilt::fOpenedOnly | StaffPostFilt::fClosedOnly);
				if(v == 1)
					p_filt->Flags |= StaffPostFilt::fOpenedOnly;
				else if(v == 2)
					p_filt->Flags |= StaffPostFilt::fClosedOnly;
				ok = valid_data = 1;
			}
			if(!local_ok)
				PPErrorByDialog(dlg, sel);
		}
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
#undef GRP_DIV
}

int SLAPI PPViewStaffPost::CreateTempTable(int order)
{
	ZDELETE(P_TempTbl);

	int    ok = 1;
	TempOrderTbl * p_tbl = CreateTempOrderFile();
	StaffPostViewItem item;
	SString temp_buf;
	THROW(p_tbl);
	{
		BExtInsert bei(p_tbl);
		for(InitIteration(0); NextIteration(&item) > 0;) {
			temp_buf = 0;
			TempOrderTbl::Rec ord_rec;
			PPStaffEntry sl_rec;
			PersonTbl::Rec psn_rec;
			if(order == ordByPerson) {
				if(SlObj.PsnObj.Fetch(item.PersonID, &psn_rec) > 0)
					temp_buf.Cat(psn_rec.Name).Trim(30);
				else
					temp_buf.CatLongZ(item.PersonID, 30);
			}
			else if(order == ordByDate) {
				temp_buf.Cat(item.Dt, MKSFMT(8, DATF_YMD|DATF_CENTURY));
			}
			else if(order == ordByStaffPerson) {
				if(SlObj.Fetch(item.StaffID, &sl_rec) > 0)
					temp_buf.Cat(sl_rec.Name).Trim(20);
				else
					temp_buf.CatLongZ(item.StaffID, 20);
				if(SlObj.PsnObj.Fetch(item.PersonID, &psn_rec) > 0)
					temp_buf.Cat(psn_rec.Name).Trim(30);
				else
					temp_buf.CatLongZ(item.PersonID, 30);
			}
			else if(order == ordByStaffDate) {
				if(SlObj.Fetch(item.StaffID, &sl_rec) > 0)
					temp_buf.Cat(sl_rec.Name).Trim(20);
				else
					temp_buf.CatLongZ(item.StaffID, 20);
				temp_buf.Cat(item.Dt, MKSFMT(8, DATF_YMD|DATF_CENTURY));
			}
			else {
				if(SlObj.PsnObj.Fetch(item.PersonID, &psn_rec) > 0)
					temp_buf.Cat(psn_rec.Name).Trim(30);
				else
					temp_buf.CatLongZ(item.PersonID, 30);
				temp_buf.Cat(item.Dt, MKSFMT(8, DATF_YMD|DATF_CENTURY));
			}
			ord_rec.ID = item.ID;
			temp_buf.CopyTo(ord_rec.Name, sizeof(ord_rec.Name));
			THROW_DB(bei.insert(&ord_rec));
		}
		THROW_DB(bei.flash());
	}
	P_TempTbl = p_tbl;
	CATCH
		ZDELETE(p_tbl);
		ok = 0;
	ENDCATCH
	return ok;
}

int SLAPI PPViewStaffPost::Init_(const PPBaseFilt * pFilt)
{
	int    ok = 1;
	StaffList.Set(0);
	ZDELETE(P_TempTbl);
	THROW(Helper_InitBaseFilt(pFilt));
	Filt.Period.Actualize(ZERODATE);
	Filt.FnPeriod.Actualize(ZERODATE);
	if(pFilt) {
		if(!CheckXORFlags(Filt.Flags, StaffPostFilt::fOpenedOnly, StaffPostFilt::fClosedOnly))
			Filt.Flags &= ~(StaffPostFilt::fOpenedOnly, StaffPostFilt::fClosedOnly);
	}
	if(Filt.OrgID || Filt.DivID) {
		PPIDArray temp_list;
		PPObjStaffList::Filt obj_filt;
		obj_filt.OrgID = Filt.OrgID;
		obj_filt.DivID = Filt.DivID;
		StaffList.InitEmpty();
		THROW(SlObj.GetList(obj_filt, &temp_list, 0));
		StaffList.Set(&temp_list);
		THROW(CreateTempTable(Filt.InitOrder));
	}
	else if(Filt.InitOrder)
		THROW(CreateTempTable(Filt.InitOrder));
	CATCHZOK
	return ok;
}

int SLAPI PPViewStaffPost::InitIteration(int order)
{
	BExtQuery::ZDelete(&P_IterQuery);
	int    ok = 1, idx = 0;
	DBQ * dbq = 0;
	BExtQuery * q = 0;
	if(P_TempTbl) {
		TempOrderTbl::Key1 ord_k1;
		THROW_MEM(q = new BExtQuery(P_TempTbl, 1));
		q->select(P_TempTbl->ID, 0L);
		MEMSZERO(ord_k1);
		Counter.Init(q->countIterations(0, &ord_k1, spFirst));
		MEMSZERO(ord_k1);
		q->initIteration(0, &ord_k1, spFirst);
	}
	else {
		union  {
			PersonPostTbl::Key1 k1;
			PersonPostTbl::Key2 k2;
		} k, k_;
		PersonPostTbl * p_tbl = SlObj.P_PostTbl;
		MEMSZERO(k);
		if(Filt.StaffID) {
			idx = 1;
			k.k1.StaffID = Filt.StaffID;
		}
		else if(Filt.PersonID) {
			idx = 2;
			k.k2.PersonID = Filt.PersonID;
		}
		else {
			idx = 1;
		}
		THROW_MEM(q = new BExtQuery(p_tbl, idx));
		dbq = ppcheckfiltid(dbq, p_tbl->StaffID, Filt.StaffID);
		dbq = ppcheckfiltid(dbq, p_tbl->PersonID, Filt.PersonID);
		dbq = & (*dbq && daterange(p_tbl->Dt, &Filt.Period));
		dbq = & (*dbq && daterange(p_tbl->Finish, &Filt.FnPeriod));
		if(Filt.Flags & StaffPostFilt::fOpenedOnly)
			dbq = & (*dbq && p_tbl->Closed == 0L);
		else if(Filt.Flags & StaffPostFilt::fClosedOnly)
			dbq = & (*dbq && p_tbl->Closed > 0L);
		q->where(*dbq);
		k_ = k;
		Counter.Init(q->countIterations(0, &k_, spGe));
		q->initIteration(0, &k, spGe);
	}
	P_IterQuery = q;
	CATCH
		BExtQuery::ZDelete(&q);
		ok = 0;
	ENDCATCH
	return ok;
}

int FASTCALL PPViewStaffPost::NextIteration(StaffPostViewItem * pItem)
{
	int    ok = -1;
	while(ok < 0 && P_IterQuery && P_IterQuery->nextIteration() > 0) {
		if(P_TempTbl) {
			PersonPostTbl::Rec rec;
			if(SlObj.SearchPost(P_TempTbl->data.ID, &rec) > 0) {
				ASSIGN_PTR(pItem, rec);
				ok = 1;
			}
		}
		else {
			PersonPostTbl::Rec & r_rec = SlObj.P_PostTbl->data;
			if(StaffList.CheckID(r_rec.StaffID)) {
				ASSIGN_PTR(pItem, r_rec);
				ok = 1;
			}
		}
	}
	return ok;
}

DBQuery * SLAPI PPViewStaffPost::CreateBrowserQuery(uint * pBrwId, SString * pSubTitle)
{
	DBQuery * q = 0;
	uint   brw_id = BROWSER_PERSONPOST;
	TempOrderTbl * p_temp_tbl = 0;
	PersonPostTbl * p_tbl = new PersonPostTbl;
	DBQ  * dbq = 0;
	DBE    dbe_psn, dbe_org, dbe_div, dbe_chargegrp;
	DBE    dbe_staff;
	PPDbqFuncPool::InitObjNameFunc(dbe_psn,   PPDbqFuncPool::IdObjNamePerson, p_tbl->PersonID);
	PPDbqFuncPool::InitObjNameFunc(dbe_staff, PPDbqFuncPool::IdObjNameStaff,  p_tbl->StaffID);
	PPDbqFuncPool::InitObjNameFunc(dbe_org,   PPDbqFuncPool::IdObjStaffOrg, p_tbl->StaffID);
	PPDbqFuncPool::InitObjNameFunc(dbe_div,   PPDbqFuncPool::IdObjStaffDiv, p_tbl->StaffID);
	PPDbqFuncPool::InitObjNameFunc(dbe_chargegrp, PPDbqFuncPool::IdObjNameSalCharge, p_tbl->ChargeGrpID);
	q = & select(
		p_tbl->ID,      // #00
		p_tbl->Code,    // #01
		p_tbl->Dt,      // #02
		p_tbl->Finish,  // #03
		dbe_staff,      // #04
		dbe_psn,        // #05
		dbe_org,        // #06
		dbe_div,        // #07
		dbe_chargegrp,  // #08
		0);
	if(P_TempTbl) {
		p_temp_tbl = new TempOrderTbl(P_TempTbl->fileName);
		dbq = &(p_tbl->ID == p_temp_tbl->ID);
		q->from(p_temp_tbl, p_tbl, 0L).where(*dbq);
	}
	else {
		dbq = ppcheckfiltid(dbq, p_tbl->StaffID, Filt.StaffID);
		dbq = ppcheckfiltid(dbq, p_tbl->PersonID, Filt.PersonID);
		dbq = & (*dbq && daterange(p_tbl->Dt, &Filt.Period));
		dbq = & (*dbq && daterange(p_tbl->Finish, &Filt.FnPeriod));
		if(Filt.Flags & StaffPostFilt::fOpenedOnly)
			dbq = & (*dbq && p_tbl->Closed == 0L);
		else if(Filt.Flags & StaffPostFilt::fClosedOnly)
			dbq = & (*dbq && p_tbl->Closed > 0L);
		q->from(p_tbl, 0L).where(*dbq);
 	}
	if(pSubTitle) {
		*pSubTitle = 0;
		PersonTbl::Rec psn_rec;
		if(Filt.OrgID && SlObj.PsnObj.Fetch(Filt.OrgID, &psn_rec) > 0)
			*pSubTitle = psn_rec.Name;
		if(Filt.DivID) {
			pSubTitle->CatDivIfNotEmpty('-', 1);
			GetObjectName(PPOBJ_LOCATION, Filt.DivID, *pSubTitle, 1);
		}
	}
	ASSIGN_PTR(pBrwId, brw_id);
	return q;
}

int SLAPI PPViewStaffPost::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	if(ok == -2) {
		PPID   id = pHdr ? *(PPID *)pHdr : 0;
		if(ppvCmd == PPVCMD_EDITITEM) {
			ok = -1;
			PPPsnPostPacket pack;
			if(SlObj.GetPostPacket(id, &pack) > 0) {
				if(SlObj.EditPostDialog(&pack, 0) > 0) {
					if(SlObj.PutPostPacket(&id, &pack, 1))
						ok = 1;
					else
						PPError();
				}
			}
		}
		else if(ppvCmd == PPVCMD_ADDITEM) {
			ok = -1;
			PPPsnPostPacket pack;
			if(Filt.StaffID)
				pack.Rec.StaffID = Filt.StaffID;
			if(SlObj.EditPostDialog(&pack, PPObjStaffList::epdfFixedPost) > 0) {
				PPID   post_id = 0;
				if(SlObj.PutPostPacket(&post_id, &pack, 1))
					ok = 1;
				else
					PPError();
			}
		}
		else if(ppvCmd == PPVCMD_DELETEITEM) {
			ok = -1;
			if(id && PPMessage(mfConf|mfYes|mfCancel, PPCFM_DELETE) == cmYes)
				if(SlObj.PutPostPacket(&id, 0, 1))
					ok = 1;
				else
					PPError();
		}
		else if(ppvCmd == PPVCMD_EDITPERSON) {
			ok = -1;
			PersonPostTbl::Rec rec;
			if(SlObj.SearchPost(id, &rec) > 0 && rec.PersonID)
				if(SlObj.PsnObj.Edit(&rec.PersonID, 0) == cmOK)
					ok = 1;
		}
		else if(ppvCmd == PPVCMD_STAFFPOST) {
			ok = -1;
			if(id) {
				StaffPostFilt sp_filt;
				sp_filt.StaffID = id;
				PPView::Execute(PPVIEW_STAFFPOST, &sp_filt, PPView::exefModeless, 0);
				ok = 1;
			}
		}
		else if(ppvCmd == PPVCMD_STAFFCAL) {
			ok = -1;
			if(id) {
				StaffCalFilt sc_flt;
				sc_flt.LinkObjType = PPOBJ_PERSONPOST;
				sc_flt.LinkObjList.Add(id);
				ShowObjects(PPOBJ_STAFFCAL, &sc_flt);
			}
		}
		else if(ppvCmd == PPVCMD_AMOUNTS) {
			ok = -1;
			if(id && SlObj.EditPostAmounts(id) > 0)
				ok = 1;
		}
		else if(ppvCmd == PPVCMD_SALARY) {
			ok = -1;
			if(id) {
				SalaryFilt s_flt;
				s_flt.PostID = id;
				PPView::Execute(PPVIEW_SALARY, &s_flt, PPView::exefModeless, 0);
			}
		}
	}
	return ok;
}

// @v6.1.x AHTOXA {
#define TOP_ID         1000000000L
#define STAFF_OFFS     (TOP_ID + 1L)
#define STAFFPOST_OFFS (STAFF_OFFS + 100000000L)

#define AMOUNTTYPE_OFFS 100000L

enum DivType {
	divtTop       = 0,
	divtEmployer  = 1,
	divtStaff     = 2,
	divtStaffPost = 3,
	divtPerson    = 4
};

PPID GetRealDivID(PPObjPerson * pObjPsn, PPID divID, DivType * pDivT)
{
	PPID real_div_id = divID;
	DivType divt;
	if(divID < TOP_ID) {
		if(pObjPsn && pObjPsn->P_Tbl->IsBelongToKind(real_div_id, PPPRK_EMPLOYER) > 0)
			divt = divtEmployer;
		else
			divt = divtPerson;
	}
	else if(divID == TOP_ID)
		divt = divtTop;
	else if(divID > STAFF_OFFS && divID < STAFFPOST_OFFS) {
		real_div_id -= STAFF_OFFS;
		divt = divtStaff;
	}
	else {
		real_div_id -= STAFFPOST_OFFS;
		divt = divtStaffPost;
	}
	ASSIGN_PTR(pDivT, divt);
	return real_div_id;
}

int GetAmountList(PPID realDivID, DivType divt, StaffAmtList * pAmtList)
{
	int ok = 1;
	StaffAmtList amt_list;
	PPObjPerson    obj_psn;
	PPObjStaffList obj_staff;
	if(divt == divtEmployer)
		obj_psn.GetStaffAmtList(realDivID, &amt_list);
	else if(divt == divtStaff) {
		PPStaffPacket staff_pack;
		THROW(obj_staff.GetPacket(realDivID, &staff_pack) > 0);
		amt_list.copy(staff_pack.Amounts);
	}
	else if(divt == divtStaffPost) {
		PPPsnPostPacket post_pack;
		THROW(obj_staff.GetPostPacket(realDivID, &post_pack) > 0);
		amt_list.copy(post_pack.Amounts);
	}
	if(pAmtList)
		pAmtList->copy(amt_list);
	CATCHZOK
	return ok;
}

int PutAmountList(PPID realDivID, DivType divt, StaffAmtList * pAmtList)
{
	int ok = 1;
	StaffAmtList amt_list;
	PPObjPerson    obj_psn;
	PPObjStaffList obj_staff;
	if(pAmtList)
		amt_list.copy(*pAmtList);
	if(divt == divtEmployer)
		obj_psn.PutStaffAmtList(realDivID, &amt_list);
	else if(divt == divtStaff) {
		PPStaffPacket staff_pack;
		THROW(obj_staff.GetPacket(realDivID, &staff_pack) > 0);
		staff_pack.Amounts.copy(amt_list);
		THROW(obj_staff.PutPacket(&realDivID, &staff_pack, 1) > 0);
	}
	else if(divt == divtStaffPost) {
		PPPsnPostPacket post_pack;
		THROW(obj_staff.GetPostPacket(realDivID, &post_pack) > 0);
		post_pack.Amounts.copy(amt_list);
		THROW(obj_staff.PutPostPacket(&realDivID, &post_pack, 1) > 0);
	}
	if(pAmtList)
		pAmtList->copy(amt_list);
	CATCHZOK
	return ok;
}

class FastEditSumByDivDlg : public PPListDialog {
public:
	FastEditSumByDivDlg() : PPListDialog(DLG_EDDIVSUM, CTL_EDDIVSUM_SUM)
	{
		SetupDivList();
		CurDivID = TOP_ID;
		updateList(-1);
	}
private:
	DECL_HANDLE_EVENT;
	virtual int addItem(long * pPos, long * pID);
	virtual int editItem(long pos, long id);
	virtual int delItem(long pos, long id);
	virtual int setupList();

	int   EditAmount(long * pPos);

	StrAssocArray * MakeDivList();
	StrAssocArray * MakeSumList(long divID);
	int SetupDivList();

	PPID CurDivID;
	PPObjStaffList  ObjStaffL;
	PPObjStaffCal   ObjStaffCal;
	PPObjPerson     ObjPsn;
	PPObjAmountType ObjAmtT;
};

IMPL_HANDLE_EVENT(FastEditSumByDivDlg)
{
	PPListDialog::handleEvent(event);
	if(TVCOMMAND) {
		if(TVCMD == cmLBItemSelected && event.isCtlEvent(CTL_EDDIVSUM_DIV)) {
			SmartListBox * p_lb = (SmartListBox*)getCtrlView(CTL_EDDIVSUM_DIV);
			if(p_lb) {
				long id = 0;
				p_lb->getCurID(&id);
				if(id != CurDivID) {
					CurDivID = id;
					updateList(-1);
				}
			}
		}
		else
			return;
	}
	else
		return;
	clearEvent(event);
}

StrAssocArray * FastEditSumByDivDlg::MakeDivList()
{
	SString buf;
	StrAssocArray empl_list, * p_ret_list = 0;
	PPIDArray empl_idlist;

	THROW_MEM(p_ret_list = new StrAssocArray);
	// @v9.4.12 PPGetWord(PPWORD_ALL, 0, buf);
	PPLoadString("all", buf); // @v9.4.12
	p_ret_list->Add(TOP_ID, 0, buf);
	//
	// Заносим работодателей
	//
	if(ObjPsn.GetListByKind(PPPRK_EMPLOYER, &empl_idlist, &empl_list) > 0) {
		PPIDArray staff_idlist;
		for(uint i = 0; i < empl_list.getCount(); i++) {
			StrAssocArray::Item empl = empl_list.at(i);
			p_ret_list->Add(empl.Id, TOP_ID, empl.Txt);
		}
		//
		// Заносим штатные должности
		//
		{
			StrAssocArray staff_entry_list;
			PPObjStaffList::Filt filt;
			filt.OrgID = (empl_list.getCount() == 1) ? empl_list.at_WithoutParent(0).Id : 0;
			ObjStaffL.GetList(filt, 0, &staff_entry_list);
			for(uint i = 0; i < staff_entry_list.getCount(); i++) {
				StrAssocArray::Item item = staff_entry_list.at(i);
				PPStaffEntry se;
				if(ObjStaffL.Fetch(item.Id, &se) > 0) {
					if(empl_list.getCount() <= 1 || empl_list.Search(se.OrgID, 0)) {
						p_ret_list->Add(se.ID + STAFF_OFFS, se.OrgID, se.Name);
						staff_idlist.add(se.ID);
					}
				}
			}
		 }
		 //
		 // Заносим штатные назначения и персоналии им соотвествующие
		 //
		 {
			DBQ * post_dbq = 0;
			PersonPostTbl::Key0 post_k0;
			BExtQuery post_iter(ObjStaffL.P_PostTbl, 0, 64);
			post_iter.select(ObjStaffL.P_PostTbl->ID, ObjStaffL.P_PostTbl->StaffID, ObjStaffL.P_PostTbl->PersonID, 0L);
			post_iter.where(*post_dbq);
			for(post_iter.initIteration(0, &post_k0); post_iter.nextIteration() > 0;) {
				PPID post_id  = ObjStaffL.P_PostTbl->data.ID + STAFFPOST_OFFS;
				PPID staff_id = ObjStaffL.P_PostTbl->data.StaffID + STAFF_OFFS;
				PPID psn_id = ObjStaffL.P_PostTbl->data.PersonID;
				SString post_name;
				PersonTbl::Rec psn_rec;
				THROW(ObjPsn.Fetch(psn_id, &psn_rec) > 0);
		 		(post_name = "X").Space().Cat(psn_rec.Name);
		 		p_ret_list->Add(post_id, staff_id, post_name);
		 		p_ret_list->Add(psn_rec.ID, post_id, psn_rec.Name);
			}
		 }
		 p_ret_list->SortByText();
	}
	CATCH
		ZDELETE(p_ret_list);
		PPErrorZ();
	ENDCATCH
	return p_ret_list;
}

StrAssocArray * FastEditSumByDivDlg::MakeSumList(long divID)
{
	StrAssocArray * p_ret_list      = 0;
	StrAssocArray * p_staffcal_list = 0;
	if(divID) {
		DivType divt;
		PPID real_div_id = GetRealDivID(&ObjPsn, divID, &divt);
		StaffAmtList amt_list;
		StaffCalFilt sc_flt;
		THROW_MEM(p_ret_list = new StrAssocArray);

		GetAmountList(real_div_id, divt, &amt_list);
		if(oneof2(divt, divtEmployer, divtPerson))
			sc_flt.LinkObjType = PPOBJ_PERSON;
		else if(divt == divtStaff)
			sc_flt.LinkObjType = PPOBJ_STAFFLIST2;
		else
			sc_flt.LinkObjType = PPOBJ_PERSONPOST;
		if(divt != divtTop)
			sc_flt.LinkObjList.Add(real_div_id);
		if(ObjAmtT.CheckRights(PPR_READ)) {
			for(uint i = 0; i < amt_list.getCount(); i++) {
				SString sub;
				StringSet ss(SLBColumnDelim);
				StaffAmtEntry & r_amt_e = amt_list.at(i);
				PPAmountType amtt_rec;

				THROW(ObjAmtT.Fetch(r_amt_e.AmtTypeID, &amtt_rec) > 0);
				ss.add((sub = amtt_rec.Name));
				ss.add(sub.Z().Cat(r_amt_e.Period));
				ss.add(sub.Z().Cat(r_amt_e.Amt));
				p_ret_list->Add((long)i + AMOUNTTYPE_OFFS, ss.getBuf());
			}
			p_ret_list->SortByText();
		}
		if(ObjStaffCal.CheckRights(PPR_READ)) {
			if(p_staffcal_list = ObjStaffCal.MakeStrAssocList((divt != divtTop) ? &sc_flt : 0)) {
				p_staffcal_list->SortByText();
				for(uint i = 0; i < p_staffcal_list->getCount(); i++)
					p_ret_list->Add(p_staffcal_list->at(i).Id, p_staffcal_list->at(i).Txt);
			}
		}
	}
	CATCH
		ZDELETE(p_ret_list);
		PPErrorZ();
	ENDCATCH
	ZDELETE(p_staffcal_list);
	return p_ret_list;
}

int FastEditSumByDivDlg::SetupDivList()
{
	int ok = 0;
	SmartListBox * p_lb = (SmartListBox *)getCtrlView(CTL_EDDIVSUM_DIV);
	if(p_lb) {
		StrAssocArray * p_div_list = MakeDivList();
		if(p_div_list) {
			ListBoxDef * p_def = new StdTreeListBoxDef(p_div_list, lbtDblClkNotify|lbtFocNotify|lbtSelNotify|lbtDisposeData, MKSTYPE(S_ZSTRING, 64));
			for(uint i = 0; i < p_div_list->getCount(); i++) {
				long img_id = 0;
				DivType divt;
				StrAssocArray::Item item = p_div_list->at(i);
				GetRealDivID(&ObjPsn, item.Id, &divt);
				if(divt == divtEmployer)
					img_id = ICON_SMALLEMPLOYER;
				else if(divt == divtStaff)
					img_id = ICON_SMALLSTAFF;
				else if(divt == divtStaffPost)
					img_id = ICON_SMALLSTAFFPOST;
				else if(divt == divtPerson)
					img_id = ICON_SMALLPERSON;
				else
					img_id = ICON_SMALLALL;
				p_def->AddImageAssoc(item.Id, img_id);
			}
			p_lb->setDef(p_def);
			p_lb->Draw_();
			ok = 1;
		}
	}
	return ok;
}

int FastEditSumByDivDlg::EditAmount(long * pPos)
{
	int ok = -1;
	long pos = (pPos) ? *pPos : -1;
	DivType divt;
	PPID real_div_id = GetRealDivID(&ObjPsn, CurDivID, &divt);
	if(oneof3(divt, divtEmployer, divtStaff, divtStaffPost)) {
		StaffAmtList amt_list;
		THROW(GetAmountList(real_div_id, divt, &amt_list));
		if(EditStaffAmtEntry(pos, &amt_list) > 0)
			THROW(ok = PutAmountList(real_div_id, divt, &amt_list));
	}
	CATCHZOK
	return ok;
}

// virtual
int FastEditSumByDivDlg::addItem(long * pPos, long * pID)
{
	int ok = -1, create_cal = 0;
	long pos = -1, id = 0;
	uint what = 0;
	DivType divt;
	PPID real_div_id = GetRealDivID(&ObjPsn, CurDivID, &divt);

	if(oneof2(divt, divtTop, divtPerson))
		create_cal = 1;
	if(!create_cal && SelectorDialog(DLG_SELNEWSTAFFAMT, CTL_SELNEWSTAFFAMT_WHAT, &what) > 0)
		create_cal = (what == 0) ? 1 : -1;
	if(create_cal == 1) {
		StaffCalFilt sc_flt;
		THROW(ObjStaffCal.CheckRights(PPR_INS));
		if(oneof2(divt, divtEmployer, divtPerson))
			sc_flt.LinkObjType = PPOBJ_PERSON;
		else if(divt == divtStaff)
			sc_flt.LinkObjType = PPOBJ_STAFFLIST2;
		else
			sc_flt.LinkObjType = PPOBJ_PERSONPOST;
		if(divt != divtTop)
			sc_flt.LinkObjList.Add(real_div_id);
		THROW(ok = ObjStaffCal.Edit(&id, (divt != divtTop) ? &sc_flt : 0));
	}
	else if(create_cal == -1) {
		THROW(ObjAmtT.CheckRights(PPR_INS));
		THROW(ok = EditAmount(&(id = -1)));
		if(ok > 0)
			id += AMOUNTTYPE_OFFS;
	}
	if(ok > 0)
		updateList(-1);
	ASSIGN_PTR(pPos, pos);
	ASSIGN_PTR(pID, id);
	CATCHZOKPPERR
	return ok;
}

//virtual
int FastEditSumByDivDlg::editItem(long pos, long id)
{
	int ok = -1;
	if(id < AMOUNTTYPE_OFFS) {
		THROW(ObjStaffCal.CheckRights(PPR_MOD));
		THROW(ok = ObjStaffCal.Edit(&id, 0));
	}
	else {
		id -= AMOUNTTYPE_OFFS;
		THROW(ObjAmtT.CheckRights(PPR_MOD));
		THROW(ok = EditAmount(&id));
	}
	CATCHZOKPPERR
	return ok;
}

//virtual
int FastEditSumByDivDlg::delItem(long pos, long id)
{
	int ok = -1;
	if(id < AMOUNTTYPE_OFFS) {
		THROW(ObjStaffCal.CheckRights(PPR_MOD));
		if(CONFIRM(PPCFM_DELSTAFFCAL))
			THROW(ok = ObjStaffCal.PutPacket(&id, 0, 1));
	}
	else {
		DivType divt;
		PPID real_div_id = GetRealDivID(&ObjPsn, CurDivID, &divt);
		PPID amt_pos = id - AMOUNTTYPE_OFFS;
		THROW(ObjAmtT.CheckRights(PPR_MOD));
		if(amt_pos >= 0 && oneof3(divt, divtEmployer, divtStaff, divtStaffPost) && CONFIRM(PPCFM_DELSTAFFAMT)) {
			StaffAmtList amt_list;
			THROW(GetAmountList(real_div_id, divt, &amt_list));
			amt_list.atFree(amt_pos);
			THROW(ok = PutAmountList(real_div_id, divt, &amt_list));
		}
	}
	CATCHZOKPPERR
	return ok;
}

// virtual
int FastEditSumByDivDlg::setupList()
{
	int ok = 1;
	StrAssocArray * p_list = MakeSumList(CurDivID);
	SmartListBox * p_lb = (SmartListBox*)getCtrlView(CTL_EDDIVSUM_SUM);
	if(p_list && p_lb) {
		ListBoxDef * p_def = new StrAssocListBoxDef(p_list, lbtDblClkNotify|lbtFocNotify|lbtDisposeData);
		THROW_MEM(p_def);
		p_def->ClearImageAssocList();
		for(uint i = 0; i < p_list->getCount(); i++) {
			long img_id = 0;
			StrAssocArray::Item item = p_list->at(i);
			if(item.Id < AMOUNTTYPE_OFFS)
				img_id = ICON_SMALLCALENDAR;
			else
				img_id = ICON_SMALLAMOUNT;
			p_def->AddImageAssoc(item.Id, img_id);
		}
		p_lb->setDef(p_def);
	}
	CATCHZOKPPERR
	return ok;
}
//
// FastEditDivBySumDlg
//
class FastEditDivBySumDlg : public PPListDialog {
public:
	FastEditDivBySumDlg() : PPListDialog(DLG_VIEWSUMDIV, CTL_VIEWSUMDIV_DIV)
	{
		SetupSumList();
		updateList(-1);
	}
private:
	DECL_HANDLE_EVENT;
	virtual int editItem(long pos, long id);
	virtual int setupList();

	int  EditAmount(PPID divID, PPID amtID);
	int  EditCalendar(PPID divID, PPID parentCalID);
	int  SetupSumList();
	StrAssocArray * MakeDivList(PPID amtID);
	StrAssocArray * MakeSumList();
	int PutDivEntryToList(PPID objType, PPID objID, StrAssocArray * pDivList);
	int PutDivListByAmt(PPID objType, PPID propID, PPID amtID, StrAssocArray * pList);

	PPID CurAmtID;
	PPObjStaffList  ObjStaffL;
	PPObjStaffCal   ObjStaffCal;
	PPObjPerson     ObjPsn;
	PPObjAmountType ObjAmtT;
};

IMPL_HANDLE_EVENT(FastEditDivBySumDlg)
{
	PPListDialog::handleEvent(event);
	if(TVCOMMAND) {
		if(TVCMD == cmLBItemSelected && event.isCtlEvent(CTL_VIEWSUMDIV_SUM)) {
			SmartListBox * p_lb = (SmartListBox*)getCtrlView(CTL_VIEWSUMDIV_SUM);
			if(p_lb) {
				long id = 0;
				p_lb->getCurID(&id);
				if(id != CurAmtID) {
					CurAmtID = id;
					updateList(-1);
				}
			}
		}
		else
			return;
	}
	else
		return;
	clearEvent(event);
}

int FastEditDivBySumDlg::PutDivEntryToList(PPID objType, PPID objID, StrAssocArray * pDivList)
{
	int ok = -1;
	PPID div_id = 0;
	SString name;
	THROW_INVARG(pDivList);
	if(objType == PPOBJ_PERSON)
		div_id = objID;
	else if(objType == PPOBJ_STAFFLIST2)
		div_id = objID + STAFF_OFFS;
	else if(objType == PPOBJ_PERSONPOST) {
		SString buf;
		PersonPostTbl::Key0 k0;
		k0.ID = objID;
		THROW(ObjStaffL.P_PostTbl->search(0, &k0, spEq) > 0);
		GetObjectName(PPOBJ_PERSON, ObjStaffL.P_PostTbl->data.PersonID, buf);
		(name = "X").Space().Cat(buf);
		div_id = objID + STAFFPOST_OFFS;
	}
	if(div_id != 0) {
		SString type;
		StringSet ss(SLBColumnDelim);

		if(!name.Len())
			GetObjectName(objType, objID, name);
		GetObjectTitle(objType, type);
		ss.add(type);
		ss.add(name);
		pDivList->Add(div_id, 0, ss.getBuf());
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int FastEditDivBySumDlg::PutDivListByAmt(PPID objType, PPID propID, PPID amtID, StrAssocArray * pList)
{
	int    ok = 1;
	StaffAmtList amt_list;
	DBQ * dbq = 0;
	PropertyTbl::Key0 k0;
	Reference * p_ref = PPRef;
	PropertyTbl & r_prop = p_ref->Prop;
	BExtQuery bext(&r_prop, 0, 64);

	MEMSZERO(k0);
	k0.ObjType = objType;
	k0.Prop    = propID;
	dbq = &(*dbq && r_prop.ObjType == objType && r_prop.Prop == propID);
	bext.select(r_prop.ObjType, r_prop.ObjID, r_prop.Prop, 0L).where(*dbq);
	for(bext.initIteration(0, &k0); bext.nextIteration() > 0;)
		if(p_ref->GetPropArray(r_prop.data.ObjType, r_prop.data.ObjID, r_prop.data.Prop, &amt_list) > 0 && amt_list.getCount() && amt_list.Search(amtID, 0) > 0)
			THROW(PutDivEntryToList(r_prop.data.ObjType, r_prop.data.ObjID, pList));
	CATCHZOK
	return ok;
}

StrAssocArray * FastEditDivBySumDlg::MakeDivList(PPID amtID)
{
	SString buf;
	StrAssocArray * p_ret_list = 0;
	StrAssocArray * p_cal_list = 0;

	p_ret_list = new StrAssocArray;
	if(amtID < AMOUNTTYPE_OFFS) {
		StaffCalFilt sc_flt;
		sc_flt.CalList.Add(amtID);
		sc_flt.LinkObjType = -1;
		p_cal_list = ObjStaffCal.MakeStrAssocList(&sc_flt);
		if(p_cal_list && p_cal_list->getCount()) {
			for(uint i = 0; i < p_cal_list->getCount(); i++) {
				PPID cal_id = p_cal_list->at(i).Id;
				PPStaffCal staff_cal;
				THROW(ObjStaffCal.Search(cal_id, &staff_cal) > 0);
				THROW(PutDivEntryToList(staff_cal.LinkObjType, staff_cal.LinkObjID, p_ret_list));
			}
			p_ret_list->SortByID();
		}
	}
	else {
		PPID amt_id = (amtID - AMOUNTTYPE_OFFS);
		StaffAmtList amt_list;
		// Извлечем персоналии у которых есть штатная сумма amtID
		THROW(PutDivListByAmt(PPOBJ_PERSON,     SLPPRP_AMTLIST,   amt_id, p_ret_list));
		// Извлечем штатные должности у которых есть штатная сумма amtID
		THROW(PutDivListByAmt(PPOBJ_STAFFLIST2, SLPPRP_AMTLIST,   amt_id, p_ret_list));
		// Извлечем штатные назначения у которых есть штатная сумма amtID
		THROW(PutDivListByAmt(PPOBJ_PERSONPOST, PSNPPPRP_AMTLIST, amt_id, p_ret_list));
	}
	CATCH
		ZDELETE(p_ret_list);
		PPErrorZ();
	ENDCATCH
	ZDELETE(p_cal_list);
	return p_ret_list;
}

StrAssocArray * FastEditDivBySumDlg::MakeSumList()
{
	StrAssocArray * p_ret_list      = 0;
	StrAssocArray * p_staffcal_list = 0;
	PPAmountType amtt_rec;

	THROW_MEM(p_ret_list = new StrAssocArray);
	if(ObjAmtT.CheckRights(PPR_READ)) {
		for(PPID amt_id = 0; ObjAmtT.EnumItems(&amt_id, &amtt_rec) > 0;)
			if(amtt_rec.Flags & PPAmountType::fStaffAmount)
				p_ret_list->Add(amt_id + AMOUNTTYPE_OFFS, amtt_rec.Name);
	}
	if(ObjStaffCal.CheckRights(PPR_READ)) {
		p_staffcal_list = ObjStaffCal.MakeStrAssocList(0);
		if(p_staffcal_list) {
			for(uint i = 0; i < p_staffcal_list->getCount(); i++)
				p_ret_list->Add(p_staffcal_list->at(i).Id, p_staffcal_list->at(i).Txt);
		}
	}
	CATCH
		ZDELETE(p_ret_list);
		PPErrorZ();
	ENDCATCH
	ZDELETE(p_staffcal_list);
	return p_ret_list;
}

int FastEditDivBySumDlg::SetupSumList()
{
	int ok = 0;
	SmartListBox * p_lb = (SmartListBox *)getCtrlView(CTL_VIEWSUMDIV_SUM);
	if(p_lb) {
		StrAssocArray * p_list = MakeSumList();
		if(p_list) {
			ListBoxDef * p_def = new StrAssocListBoxDef(p_list, lbtDblClkNotify|lbtFocNotify|lbtSelNotify|lbtDisposeData);
			for(uint i = 0; i < p_list->getCount(); i++) {
				long img_id = 0;
				StrAssocArray::Item item = p_list->at(i);
				if(item.Id < AMOUNTTYPE_OFFS)
					img_id = ICON_SMALLCALENDAR;
				else
					img_id = ICON_SMALLAMOUNT;
				p_def->AddImageAssoc(item.Id, img_id);
			}
			p_lb->setDef(p_def);
			p_lb->Draw_();
			ok = 1;
		}
		p_lb->getCurID(&CurAmtID);
	}
	return ok;
}

int FastEditDivBySumDlg::EditAmount(PPID divID, PPID amtID)
{
	int ok = -1;
	long pos = -1;
	DivType divt;
	PPID real_div_id = GetRealDivID(&ObjPsn, divID, &divt);
	SString amt_name;
	StaffAmtList amt_list;
	StrAssocArray sel_amt_list;
	StaffAmtEntry * p_entry = 0;

	THROW(ObjAmtT.CheckRights(PPR_MOD));
	THROW(GetAmountList(real_div_id, divt, &amt_list));
	GetObjectName(PPOBJ_AMOUNTTYPE, amtID, amt_name);
	for(uint i = 0; amt_list.enumItems(&i, (void **)&p_entry);)
		if(p_entry->AmtTypeID == amtID)
			sel_amt_list.Add(i - 1, amt_name);
	if(sel_amt_list.getCount() > 0) {
		if(sel_amt_list.getCount() > 1) {
			SString title;
			PPLoadText(PPTXT_SELSTAFFAMTTITLE, title);
			if(ListBoxSelDialog(&sel_amt_list, title, &pos, 0) <= 0)
				pos = -1;
		}
		else
			pos = sel_amt_list.at(0).Id;
		if(pos >= 0 && EditStaffAmtEntry(pos, &amt_list) > 0)
			THROW(ok = PutAmountList(real_div_id, divt, &amt_list));
	}
	CATCHZOK
	return ok;
}

int FastEditDivBySumDlg::EditCalendar(PPID divID, PPID parentCalID)
{
	int    ok = -1;
	DivType divt;
	PPID real_div_id = GetRealDivID(&ObjPsn, divID, &divt);
	StrAssocArray * p_div_cal_list = 0;

	THROW(ObjStaffCal.CheckRights(PPR_MOD));
	{
		StaffCalFilt sc_flt;
		if(oneof2(divt, divtEmployer, divtPerson))
			sc_flt.LinkObjType = PPOBJ_PERSON;
		else if(divt == divtStaff)
			sc_flt.LinkObjType = PPOBJ_STAFFLIST2;
		else
			sc_flt.LinkObjType = PPOBJ_PERSONPOST;
		sc_flt.LinkObjList.Add(real_div_id);
		sc_flt.CalList.Add(parentCalID);
		if((p_div_cal_list = ObjStaffCal.MakeStrAssocList(&sc_flt)) != 0 && p_div_cal_list->getCount()) {
			//
			// Выбор из списка календаря для редактирования //
			//
			PPID   cal_id = 0;
			if(p_div_cal_list->getCount() > 1) {
				SString title;
				PPLoadText(PPTXT_SELSTAFFCALLTITLE, title);
				ListBoxSelDialog(p_div_cal_list, title, &cal_id, 0);
			}
			else
				cal_id = p_div_cal_list->at(0).Id;
			if(cal_id)
				THROW(ok = ObjStaffCal.Edit(&cal_id, 0));
		}
	}
	CATCHZOK
	ZDELETE(p_div_cal_list);
	return ok;
}

//virtual
int FastEditDivBySumDlg::editItem(long pos, long id)
{
	int ok = -1;
	if(CurAmtID < AMOUNTTYPE_OFFS) {
		THROW(ok = EditCalendar(id, CurAmtID));
	}
	else {
		THROW(ok = EditAmount(id, CurAmtID - AMOUNTTYPE_OFFS));
	}
	CATCHZOKPPERR
	return ok;
}

// virtual
int FastEditDivBySumDlg::setupList()
{
	int ok = 1;
	StrAssocArray * p_list = MakeDivList(CurAmtID);
	SmartListBox * p_lb = (SmartListBox*)getCtrlView(CTL_VIEWSUMDIV_DIV);
	if(p_list && p_lb) {
		ListBoxDef * p_def = new StrAssocListBoxDef(p_list, lbtDblClkNotify|lbtFocNotify|lbtDisposeData);
		THROW_MEM(p_def);
		p_def->ClearImageAssocList();
		for(uint i = 0; i < p_list->getCount(); i++) {
			long img_id = 0;
			StrAssocArray::Item item = p_list->at(i);
			DivType divt;
			PPID real_div_id = GetRealDivID(&ObjPsn, item.Id, &divt);
			if(divt == divtEmployer)
				img_id = ICON_SMALLEMPLOYER;
			else if(divt == divtStaff)
				img_id = ICON_SMALLSTAFF;
			else if(divt == divtStaffPost)
				img_id = ICON_SMALLSTAFFPOST;
			else if(divt == divtPerson)
				img_id = ICON_SMALLPERSON;
			p_def->AddImageAssoc(item.Id, img_id);
		}
		p_lb->setDef(p_def);
	}
	CATCHZOKPPERR
	return ok;
}

int SLAPI FastEditSumByDivDialog()
{
	FastEditSumByDivDlg * p_dlg = new FastEditSumByDivDlg();
	return ExecViewAndDestroy(p_dlg);
}

int SLAPI FastViewDivBySumDialog()
{
	FastEditDivBySumDlg * p_dlg = new FastEditDivBySumDlg;
	return ExecViewAndDestroy(p_dlg);
}
//
// Implementation of PPALDD_StaffListView
//
PPALDD_CONSTRUCTOR(StaffListView)
{
	InitFixData(rscDefHdr,  &H, sizeof(H));
	InitFixData(rscDefIter, &I, sizeof(I));
}

PPALDD_DESTRUCTOR(StaffListView)
{
	Destroy();
}

int PPALDD_StaffListView::InitData(PPFilt & rFilt, long rsrv)
{
	PPViewStaffList * p_v = 0;
	if(rsrv)
		Extra[1].Ptr = p_v = (PPViewStaffList *)rFilt.Ptr;
	else {
		Extra[0].Ptr = p_v = new PPViewStaffList;
		p_v->Init_((StaffListFilt *)rFilt.Ptr);
	}
	H.ExtAmt1ID = p_v->GetSalaryAmountType(1);
	H.ExtAmt2ID = p_v->GetSalaryAmountType(2);
	H.ExtAmt3ID = p_v->GetSalaryAmountType(3);
	return DlRtm::InitData(rFilt, rsrv);
}

int PPALDD_StaffListView::InitIteration(long iterId, int sortId, long rsrv)
{
	INIT_PPVIEW_ALDD_ITER(StaffList);
}

int PPALDD_StaffListView::NextIteration(long iterId)
{
	START_PPVIEW_ALDD_ITER(StaffList);
	I.StaffID = item.ID;
	I.BaseAmt = item.Salary;
	I.ExtAmt1 = item.RiseInWages1;
	I.ExtAmt2 = item.RiseInWages2;
	I.ExtAmt3 = item.RiseInWages3;
	FINISH_PPVIEW_ALDD_ITER();
}

void PPALDD_StaffListView::Destroy()
{
	DESTROY_PPVIEW_ALDD(StaffList);
}
