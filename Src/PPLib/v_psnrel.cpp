// V_PSNREL.CPP
// Copyright (c) A.Starodub 2006, 2007, 2009, 2010, 2016, 2017, 2019
//
#include <pp.h>
#pragma hdrstop

IMPLEMENT_PPFILT_FACTORY(PersonRel); SLAPI PersonRelFilt::PersonRelFilt() : PPBaseFilt(PPFILT_PERSONREL, 0, 0)
{
	P_PsnFilt = 0;
	SetFlatChunk(offsetof(PersonRelFilt, ReserveStart),
		offsetof(PersonRelFilt, P_PsnFilt)-offsetof(PersonRelFilt, ReserveStart));
	SetBranchBaseFiltPtr(PPFILT_PERSON, offsetof(PersonRelFilt, P_PsnFilt));
	Init(1, 0);
	SortOrd = ordByPrmrPerson;
	Flags = 0;
}

PersonRelFilt & FASTCALL PersonRelFilt::operator = (const PersonRelFilt & s)
{
	Copy(&s, 0);
	return *this;
}

SLAPI PPViewPersonRel::PPViewPersonRel() : PPView(0, &Filt, PPVIEW_PERSONREL), P_TempTbl(0), P_TempOrd(0)
{
	DefReportId = REPORT_PSNRELLIST;
}

SLAPI PPViewPersonRel::~PPViewPersonRel()
{
	ZDELETE(P_TempTbl);
	ZDELETE(P_TempOrd);
}

class PersonRelFiltDialog : public TDialog {
public:
	PersonRelFiltDialog() : TDialog(DLG_PSNRELFLT)
	{
	}
	int    setDTS(const PersonRelFilt *);
	int    getDTS(PersonRelFilt *);
private:
	DECL_HANDLE_EVENT;
	PersonRelFilt Data;
};

int PersonRelFiltDialog::setDTS(const PersonRelFilt * pData)
{
	if(pData)
		Data.Copy(pData, 1);
	SetupPPObjCombo(this, CTLSEL_PSNRELFLT_RELTYPE, PPOBJ_PERSONRELTYPE, Data.RelTypeID, 0, 0);
	long   apply_psn_filt_to_scnd = (Data.Flags & PersonRelFilt::fApplyPsnFiltToScnd) ? 1 : 0;
	AddClusterAssocDef(CTL_PSNRELFLT_PSNFLTTO,  0, 0);
	AddClusterAssoc(CTL_PSNRELFLT_PSNFLTTO,  1, 1);
	SetClusterData(CTL_PSNRELFLT_PSNFLTTO, apply_psn_filt_to_scnd);

	AddClusterAssocDef(CTL_PSNRELFLT_SORTORD,  0, PersonRelFilt::ordByPrmrPerson);
	AddClusterAssoc(CTL_PSNRELFLT_SORTORD,  1, PersonRelFilt::ordByScndPerson);
	AddClusterAssoc(CTL_PSNRELFLT_SORTORD,  2, PersonRelFilt::ordByRelationType);
	SetClusterData(CTL_PSNRELFLT_SORTORD, Data.SortOrd);

	long   added_sel = 0;
	if(Data.Flags & PersonRelFilt::fAddedSelectorByPrmr)
		added_sel = 1;
	else if(Data.Flags & PersonRelFilt::fAddedSelectorByScnd)
		added_sel = 2;
	AddClusterAssocDef(CTL_PSNRELFLT_ADDEDSEL, 0, 0);
	AddClusterAssoc(CTL_PSNRELFLT_ADDEDSEL, 1, 1);
	AddClusterAssoc(CTL_PSNRELFLT_ADDEDSEL, 2, 2);
	SetClusterData(CTL_PSNRELFLT_ADDEDSEL, added_sel);

	return 1;
}

int PersonRelFiltDialog::getDTS(PersonRelFilt * pData)
{
	int    ok = -1;
	getCtrlData(CTLSEL_PSNRELFLT_RELTYPE, &Data.RelTypeID);
	long   apply_psn_filt_to_scnd = GetClusterData(CTL_PSNRELFLT_PSNFLTTO);
	SETFLAG(Data.Flags, PersonRelFilt::fApplyPsnFiltToScnd, apply_psn_filt_to_scnd == 1);
	Data.SortOrd = GetClusterData(CTL_PSNRELFLT_SORTORD);
	long   added_sel = GetClusterData(CTL_PSNRELFLT_ADDEDSEL);
	Data.Flags &= ~(PersonRelFilt::fAddedSelectorByPrmr | PersonRelFilt::fAddedSelectorByScnd);
	if(added_sel == 1)
		Data.Flags |= PersonRelFilt::fAddedSelectorByPrmr;
	else if(added_sel == 2)
		Data.Flags |= PersonRelFilt::fAddedSelectorByScnd;
	ok = 1;
	if(pData)
		pData->Copy(&Data, 1);
	return ok;
}

IMPL_HANDLE_EVENT(PersonRelFiltDialog)
{
	TDialog::handleEvent(event);
	if(event.isCmd(cmPerson)) {
		PPViewPerson v_psn;
		if(Data.P_PsnFilt) {
			v_psn.EditBaseFilt(Data.P_PsnFilt);
		}
		else {
			PersonFilt * p_filt = (PersonFilt *)v_psn.CreateFilt(0);
			if(v_psn.EditBaseFilt(p_filt) > 0)
				Data.P_PsnFilt = p_filt;
			else
				delete p_filt;
		}
		if(Data.P_PsnFilt && Data.P_PsnFilt->IsEmpty()) {
			ZDELETE(Data.P_PsnFilt);
		}
		clearEvent(event);
	}
}

int SLAPI PPViewPersonRel::EditBaseFilt(PPBaseFilt * pBaseFilt)
{
	if(!Filt.IsA(pBaseFilt))
		return 0;
	PersonRelFilt * p_filt = (PersonRelFilt *)pBaseFilt;
	DIALOG_PROC_BODYERR(PersonRelFiltDialog, p_filt);
}

int SLAPI PPViewPersonRel::CheckForFilt(const PersonCore::RelationRecord * pRec)
{
	int    r = 0;
	if(pRec) {
		int    apply_flt_toscnd = BIN(Filt.Flags & PersonRelFilt::fApplyPsnFiltToScnd);
		if((!Filt.PrmrPersonID || Filt.PrmrPersonID == pRec->PrmrObjID) &&
			(!Filt.ScndPersonID || Filt.ScndPersonID == (pRec->ScndObjID & ~0xff000000)) &&
			(!Filt.RelTypeID || Filt.RelTypeID == pRec->RelTypeID) &&
			(!PersonList.IsExists() ||
			PersonList.Search(apply_flt_toscnd ? (pRec->ScndObjID & ~0xff000000): pRec->PrmrObjID, 0, 1) > 0))
			r = 1;
	}
	return r;
}

PP_CREATE_TEMP_FILE_PROC(CreateTempFile, TempPersonRel);

int SLAPI PPViewPersonRel::Init_(const PPBaseFilt * pBaseFilt)
{
	int    ok = 1;
	BExtQuery  * p_q = 0;
	PPViewPerson * p_psnv = 0;
	THROW(Helper_InitBaseFilt(pBaseFilt));
	{
		ObjAssocTbl::Key1 k;
		ObjAssocTbl & r_assc = PPRef->Assc;
		ZDELETE(P_TempTbl);
		PrmrList.Clear();
		ScndList.Clear();
		PersonList.Set(0);
		THROW(P_TempTbl = CreateTempFile());
		{
			BExtInsert bei(P_TempTbl);
			PPTransaction tra(ppDbDependTransaction, 1);
			THROW(tra);
			if(Filt.P_PsnFilt && !Filt.P_PsnFilt->IsEmpty()) {
				PersonViewItem item;
				PPIDArray psn_list;
				THROW(PersonList.InitEmpty());
				THROW_MEM(p_psnv = new PPViewPerson());
				THROW(p_psnv->Init_(Filt.P_PsnFilt));
				for(p_psnv->InitIteration(); p_psnv->NextIteration(&item) > 0;)
					psn_list.add(item.ID);
				psn_list.sort();
				PersonList.Set(&psn_list);
			}
			MEMSZERO(k);
			k.AsscType = PPASS_PERSONREL;
			THROW(p_q = new BExtQuery(&r_assc, 1));
			p_q->selectAll().where(r_assc.AsscType == PPASS_PERSONREL);
			for(p_q->initIteration(0, &k, spGe); p_q->nextIteration() > 0;) {
				const PersonCore::RelationRecord * p_rec = (const PersonCore::RelationRecord *)&r_assc.data;
				if(!Filt.RelTypeID || p_rec->RelTypeID == Filt.RelTypeID) {
					PrmrList.Add(p_rec->PrmrObjID);
					ScndList.Add(p_rec->ScndObjID);
				}
				if(CheckForFilt(p_rec)) {
					TempPersonRelTbl::Rec temp_rec;
					MakeTempEntry(p_rec, &temp_rec);
					THROW_DB(bei.insert(&temp_rec));
				}
			}
			THROW_DB(bei.flash());
			THROW(tra.Commit());
		}
		THROW(CreateOrderTable(Filt.SortOrd, &P_TempOrd));
	}
	PPWait(0);
	CATCH
		ZDELETE(P_TempTbl);
		ZDELETE(P_TempOrd);
		ok = 0;
	ENDCATCH
	ZDELETE(p_psnv);
	BExtQuery::ZDelete(&p_q);
	return ok;
}

int SLAPI PPViewPersonRel::MakeTempEntry(const PersonCore::RelationRecord * pRec, TempPersonRelTbl::Rec * pTempRec)
{
	int    ok = -1;
	if(pRec && pTempRec) {
		PPPersonRelType reltyp_rec;
		MEMSZERO(reltyp_rec);
		memzero(pTempRec, sizeof(TempPersonRelTbl::Rec));
		pTempRec->PrmrPersonID   = pRec->PrmrObjID;
		pTempRec->ScndPersonID   = pRec->ScndObjID & ~0xff000000;
		pTempRec->RelTypeID = pRec->RelTypeID;
		if(SearchObject(PPOBJ_PERSONRELTYPE, pTempRec->RelTypeID, &reltyp_rec) > 0)
			STRNSCPY(pTempRec->RelName, reltyp_rec.Name);
		else
			ltoa(pTempRec->RelTypeID, pTempRec->RelName, 10);
	}
	return ok;
}

int SLAPI PPViewPersonRel::MakeTempOrdEntry(long ord, const TempPersonRelTbl::Rec * pTempRec, TempOrderTbl::Rec * pOrdRec)
{
	int    ok = -1;
	if(pTempRec && pOrdRec) {
		SString buf;
		if(ord == PersonRelFilt::ordByScndPerson)
			GetPersonName(pTempRec->ScndPersonID, buf);
		else if(ord == PersonRelFilt::ordByRelationType)
			buf.CopyFrom(pTempRec->RelName);
		else
			GetPersonName(pTempRec->PrmrPersonID, buf);
		pOrdRec->ID = pTempRec->ID;
		buf.CopyTo(pOrdRec->Name, sizeof(pOrdRec->Name));
		ok = 1;
	}
	return ok;
}

int SLAPI PPViewPersonRel::UpdateTempTable(PPID prmrID, const PPIDArray & rScndList, PPID relation, int reverse)
{
	int    ok = 1;
	if(P_TempTbl) {
		PPTransaction tra(ppDbDependTransaction, 1);
		THROW(tra);
		for(uint i = 0; i < rScndList.getCount(); i++) {
			int found = 0;
			PPID id = 0;
			PPID scnd_id = (reverse) ? prmrID : rScndList.at(i), prmr_id = (reverse) ? rScndList.at(i) : prmrID;
			//ObjAssocTbl::Rec oa_rec;
			PersonCore::RelationRecord rel_rec;
			TempPersonRelTbl::Key1 k1;
			MEMSZERO(rel_rec);
			MEMSZERO(k1);
			k1.PrmrPersonID   = prmr_id;
			k1.ScndPersonID   = scnd_id;
			k1.RelTypeID = relation;
			if(P_TempTbl->search(1, &k1, spEq) > 0)
				id = P_TempTbl->data.ID;

			for(PPID next_id = 0; !found && PPRef->Assc.EnumByPrmr(PPASS_PERSONREL, prmr_id, &next_id, (ObjAssocTbl::Rec *)&rel_rec) > 0;)
				if(rel_rec.RelTypeID == relation && scnd_id == (rel_rec.ScndObjID & ~0xff000000)) {
					found = 1;
					if(!Filt.RelTypeID || relation == Filt.RelTypeID) {
						PrmrList.Add(rel_rec.PrmrObjID);
						ScndList.Add(rel_rec.ScndObjID);
					}
				}
			if(found && CheckForFilt(&rel_rec)) {
				TempOrderTbl::Rec ord_rec;
				TempPersonRelTbl::Rec temp_rec;
				MEMSZERO(ord_rec);
				MEMSZERO(temp_rec);
				MakeTempEntry(&rel_rec, &temp_rec);
				if(id && SearchByID(P_TempTbl, 0, id, 0) > 0) {
					temp_rec.ID = id;
					UpdateByID(P_TempTbl, 0, id, &temp_rec, 0);
					MakeTempOrdEntry(Filt.SortOrd, &temp_rec, &ord_rec);
					UpdateByID(P_TempOrd, 0, id, &ord_rec, 0);
				}
				else {
					AddByID(P_TempTbl, &id, &temp_rec, 0);
					temp_rec.ID = id;
					MakeTempOrdEntry(Filt.SortOrd, &temp_rec, &ord_rec);
					AddByID(P_TempOrd, &id, &ord_rec, 0);
				}
			}
			else {
				THROW_DB(deleteFrom(P_TempTbl, 0, P_TempTbl->ID == id));
				THROW_DB(deleteFrom(P_TempOrd, 0, P_TempOrd->ID == id));
			}
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI PPViewPersonRel::CreateOrderTable(long ord, TempOrderTbl ** ppTbl)
{
	int    ok = 1;
	ZDELETE(P_TempOrd);

	*ppTbl = 0;
	if(!P_TempTbl)
		return -1;
	IterCounter cntr;
	TempOrderTbl * p_o = 0;
	TempOrderTbl::Rec ord_rec;
	TempPersonRelTbl::Key0 k;
	TempPersonRelTbl * p_t = P_TempTbl;
	BExtQuery q(P_TempTbl, 0);
	BExtInsert * p_bei = 0;

	PPInitIterCounter(cntr, p_t);
	THROW(p_o = CreateTempOrderFile());
	THROW_MEM(p_bei = new BExtInsert(p_o));
	q.select(p_t->ID, p_t->PrmrPersonID, p_t->ScndPersonID, p_t->RelName, 0L);
	MEMSZERO(k);
	for(q.initIteration(0, &k, spFirst); q.nextIteration() > 0;) {
		MakeTempOrdEntry(ord, &p_t->data, &ord_rec);
		THROW_DB(p_bei->insert(&ord_rec));
		PPWaitPercent(cntr.Increment());
	}
	THROW_DB(p_bei->flash());
	*ppTbl = p_o;
	p_o = 0;
	CATCHZOK
	delete p_o;
	delete p_bei;
	return ok;
}

int SLAPI PPViewPersonRel::InitIteration()
{
	int    ok = 1;
	int    idx = 0;
	TempOrderTbl::Key1 k, k_;
	BExtQuery::ZDelete(&P_IterQuery);
	MEMSZERO(k);
	P_IterQuery = new BExtQuery(P_TempOrd, 1);
	P_IterQuery->selectAll();
	k_ = k;
	Counter.Init(P_IterQuery->countIterations(0, &k_, spGe));
	P_IterQuery->initIteration(0, &k, spGe);
	return ok;
}

int FASTCALL PPViewPersonRel::NextIteration(PersonRelViewItem * pItem)
{
	if(P_IterQuery && P_IterQuery->nextIteration() > 0) {
		TempPersonRelTbl::Key0 k;
		MEMSZERO(k);
		k.ID = P_TempOrd->data.ID;
		if(P_TempTbl->search(0, &k, spEq) > 0) {
			PersonRelViewItem item;
			Counter.Increment();
			MEMSZERO(item);
			item.ID             = P_TempTbl->data.ID;
			item.PrmrPersonID   = P_TempTbl->data.PrmrPersonID;
			item.ScndPersonID   = P_TempTbl->data.ScndPersonID;
			item.RelTypeID = P_TempTbl->data.RelTypeID;
			STRNSCPY(item.RelName, P_TempTbl->data.RelName);
			ASSIGN_PTR(pItem, item);
			return 1;
		}
	}
	return -1;
}

int SLAPI PPViewPersonRel::OnExecBrowser(PPViewBrowser * pBrw)
{
	if(Filt.Flags & PersonRelFilt::fAddedSelectorByPrmr) {
		PPIDArray id_list;
		for(ulong id = 0; PrmrList.Enum(&id);)
			id_list.add((long)id);
		pBrw->SetupToolbarCombo(PPOBJ_PERSON, Filt.PrmrPersonID, 0, id_list);
	}
	else if(Filt.Flags & PersonRelFilt::fAddedSelectorByScnd) {
		PPIDArray id_list;
		for(ulong id = 0; ScndList.Enum(&id);)
			id_list.add((long)id);
		pBrw->SetupToolbarCombo(PPOBJ_PERSON, Filt.ScndPersonID, 0, id_list);
	}
	return -1;
}

DBQuery * SLAPI PPViewPersonRel::CreateBrowserQuery(uint * pBrwId, SString * pSubTitle)
{
	DBQuery * q  = 0;
	TempPersonRelTbl * t = 0;
	TempOrderTbl * p_ord = 0;
	DBQ  * dbq = 0;
	DBE    dbe_psn1, dbe_psn2;
	uint   brw_id = BROWSER_PERSONREL;
	THROW(CheckTblPtr(p_ord = new TempOrderTbl(P_TempOrd->GetName())));
	THROW(CheckTblPtr(t = new TempPersonRelTbl(P_TempTbl->GetName())));
	PPDbqFuncPool::InitObjNameFunc(dbe_psn1, PPDbqFuncPool::IdObjNamePerson, t->PrmrPersonID);
	PPDbqFuncPool::InitObjNameFunc(dbe_psn2, PPDbqFuncPool::IdObjNamePerson, t->ScndPersonID);
	dbq = &(*dbq && t->ID == p_ord->ID);
	q = & select(
		t->ID,           // #01
		t->PrmrPersonID, // #02
		t->ScndPersonID, // #03
		t->RelTypeID,    // #04
		dbe_psn1,        // #05
		dbe_psn2,        // #06
		t->RelName,      // #07
		0L).from(p_ord, t, 0L).where(*dbq).orderBy(p_ord->Name, 0L);
	THROW(CheckQueryPtr(q));
	CATCH
		if(q)
			ZDELETE(q);
		else {
			delete p_ord;
			delete t;
		}
	ENDCATCH
	ASSIGN_PTR(pBrwId, brw_id);
	return q;
}

int SLAPI PPViewPersonRel::ViewReverseRelations(PPID personID)
{
	int    ok = -1;
	PersonRelFilt filt;
	PPViewPersonRel v_psnrel;
	filt.ScndPersonID = personID;
	THROW(v_psnrel.Init_(&filt));
	THROW(v_psnrel.Browse(0));
	CATCHZOK
	return ok;
}

int SLAPI PPViewPersonRel::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	int    reverse = 0;
	PPID   prmr_id = 0, rel = 0;
	PPIDArray scnd_list;
	if(ok == -2) {
		BrwHdr brw_hdr;
		if(pHdr)
			brw_hdr = *(BrwHdr*)pHdr;
		else
			MEMSZERO(brw_hdr);
		prmr_id = brw_hdr.PrmrPersonID;
		scnd_list.add(brw_hdr.ScndPersonID);
		rel = brw_hdr.RelTypeID;
		switch(ppvCmd) {
			case PPVCMD_ADDITEM:
			case PPVCMD_ADDREVERSEITEM:
				{
					reverse = (ppvCmd == PPVCMD_ADDITEM) ? 0 : 1;
					prmr_id = 0;
					scnd_list.freeAll();
					if(Filt.ScndPersonID) {
						prmr_id = Filt.ScndPersonID;
						reverse = (reverse) ? 0 :1;
					}
					else if(Filt.PrmrPersonID)
						prmr_id = Filt.PrmrPersonID;
					ok = PsnObj.AddRelationList(&prmr_id, &scnd_list, &rel, reverse);
				}
				break;
			case PPVCMD_DELETEITEM:
				if(!(ok = PsnObj.RemoveRelation(prmr_id, scnd_list.getSingle(), rel)))
					PPError();
				break;
			/*
			case PPVCMD_EDITITEM:
				{
					PPID scnd_id = scnd_list.getSingle();
					if((ok = PsnObj.EditRelation(&prmr_id, &scnd_id, &rel)) > 0) {
						scnd_list.addUnique(scnd_id);
					}
				}
				break;
			*/
			case PPVCMD_EDITPERSON:
				ok = -1;
				if(prmr_id && PsnObj.Edit(&prmr_id, 0) == cmOK)
					ok = 1;
				break;
			case PPVCMD_REVERSE:
				ok = ViewReverseRelations(NZOR(Filt.PrmrPersonID, prmr_id));
				break;
			case PPVCMD_TB_CBX_SELECTED:
				{
					ok = -1;
					PPID   person_id = 0;
					if(pBrw && pBrw->GetToolbarComboData(&person_id)) {
						if(Filt.Flags & PersonRelFilt::fAddedSelectorByPrmr) {
							if(person_id != Filt.PrmrPersonID) {
								Filt.PrmrPersonID = person_id;
								ok = ChangeFilt(1, pBrw);
							}
						}
						else if(Filt.Flags & PersonRelFilt::fAddedSelectorByScnd) {
							if(person_id != Filt.ScndPersonID) {
								Filt.ScndPersonID = person_id;
								ok = ChangeFilt(1, pBrw);
							}
						}
					}
				}
				break;
		}
	}
	if(ok > 0 && oneof4(ppvCmd, PPVCMD_ADDITEM, PPVCMD_ADDREVERSEITEM, PPVCMD_DELETEITEM, PPVCMD_EDITITEM)) {
		scnd_list.sort();
		UpdateTempTable(prmr_id, scnd_list, rel, reverse);
		CALLPTRMEMB(pBrw, Update());
	}
	return ok;
}
//
// Implementation of PPALDD_PsnRelList
//
PPALDD_CONSTRUCTOR(PsnRelList)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		AssignDefIterData(&I, sizeof(I));
	}
}

PPALDD_DESTRUCTOR(PsnRelList) { Destroy(); }

int PPALDD_PsnRelList::InitData(PPFilt & rFilt, long rsrv)
{
	INIT_PPVIEW_ALDD_DATA_U(PersonRel, rsrv);
	return DlRtm::InitData(rFilt, rsrv);
}

int PPALDD_PsnRelList::InitIteration(PPIterID iterId, int sortId, long rsrv)
{
	INIT_PPVIEW_ALDD_ITER(PersonRel);
}

int PPALDD_PsnRelList::NextIteration(PPIterID iterId)
{
	START_PPVIEW_ALDD_ITER(PersonRel);
	const PersonRelFilt * p_filt = (const PersonRelFilt *)p_v->GetBaseFilt();
	PPWaitPercent(p_v->GetCounter());
	I.PersonID  = item.PrmrPersonID;
	I.SecondID  = item.ScndPersonID;
	STRNSCPY(I.RelationName, item.RelName);
	FINISH_PPVIEW_ALDD_ITER();
}

void PPALDD_PsnRelList::Destroy()
{
	DESTROY_PPVIEW_ALDD(PersonRel);
}
