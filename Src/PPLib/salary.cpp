// SALARY.CPP
// Copyright (c) A.Sobolev 2007, 2008, 2009, 2010, 2011, 2012, 2014, 2015, 2016, 2017
//
#include <pp.h>
#pragma hdrstop
//
// @ModuleDef(SalaryCore)
//
SLAPI SalaryCore::SalaryCore() : SalaryTbl()
{
}

int SLAPI SalaryCore::Validate(const SalaryTbl::Rec * pRec)
{
	int    ok = 1;
	if(pRec) {
		THROW(pRec->ID >= 0);
		THROW_SL(checkdate(pRec->Beg, 0));
		THROW_SL(checkdate(pRec->End, 0));
		THROW(pRec->Beg <= pRec->End); // @todo @errorcode
		THROW(pRec->PostID > 0 && pRec->PostID < 0x00ffffffL);
		THROW(pRec->SalChargeID > 0 && pRec->SalChargeID < 0x00ffffffL);
		THROW(fabs(pRec->Amount) < fpow10i(8));
		THROW(pRec->Flags == 0);
		THROW(pRec->LinkBillID >= 0 && pRec->LinkBillID < 0x00ffffffL);
		THROW(pRec->GenBillID >= 0 && pRec->GenBillID < 0x00ffffffL);
		THROW(pRec->RByGenBill == 0);
	}
	CATCHZOK
	return ok;
}

int SLAPI SalaryCore::Search(PPID id, SalaryTbl::Rec * pRec)
{
	return SearchByID(this, 0, id, pRec);
}

int SLAPI SalaryCore::Put(PPID * pID, SalaryTbl::Rec * pRec, int use_ta)
{
	int    ok = 1;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(pRec) {
			int    r = -1;
			SalaryTbl::Rec intr_rec;
			DateRange period;
			period.Set(pRec->Beg, pRec->End);
			{
				int    r2 = 0, sp = spGe;
				SalaryTbl::Key1 k1;
				MEMSZERO(k1);
				k1.PostID = pRec->PostID;
				k1.SalChargeID = pRec->SalChargeID;
				k1.ExtObjID = pRec->ExtObjID;
				while(r < 0 && (r2 = search(1, &k1, sp)) && k1.PostID == pRec->PostID && k1.SalChargeID == pRec->SalChargeID && k1.ExtObjID == pRec->ExtObjID) {
					DateRange temp;
					temp.Set(data.Beg, data.End);
					if(temp.IsEqual(period)) {
						copyBufTo(&intr_rec);
						r = 2;
					}
					else if(temp.IsIntersect(period)) {
						copyBufTo(&intr_rec);
						r = 1;
					}
					sp = spGt;
				}
				THROW(r2 || PPDbSearchError());
			}
			if(r == 2) {
				if(*pID == 0) {
					pRec->LinkBillID = intr_rec.LinkBillID;
					pRec->GenBillID  = intr_rec.GenBillID;
					pRec->RByGenBill = intr_rec.RByGenBill;
					*pID = pRec->ID = intr_rec.ID;
				}
				else
					THROW_PP(*pID == intr_rec.ID, PPERR_SALPERIODINTERSECT);
			}
			else
				THROW_PP(r < 0, PPERR_SALPERIODINTERSECT);
			THROW(Validate(pRec));
		}
		if(*pID) {
			SalaryTbl::Rec org_rec;
			THROW(SearchByID_ForUpdate(this, 0, *pID, &org_rec) > 0);
			if(pRec) {
				THROW_DB(updateRecBuf(pRec)); // @sfu
			}
			else {
				//
				// Не будем считать ошибкой отсутствие документа при попытке его удалить
				//
				if(org_rec.GenBillID && BillObj->Search(org_rec.GenBillID, 0) > 0)
					THROW(BillObj->RemovePacket(org_rec.GenBillID, 0));
				THROW_DB(deleteRec()); // @sfu
			}
		}
		else if(pRec) {
			pRec->ID = 0;
			copyBufFrom(pRec);
			THROW_DB(insertRec(0, pID));
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI SalaryCore::Get__(PPID postID, PPID salChargeID, const DateRange & rPeriod, SalaryTbl::Rec * pRec)
{
	SalaryTbl::Key1 k1;
	MEMSZERO(k1);
	k1.PostID = postID;
	k1.SalChargeID = salChargeID;
	k1.Beg = rPeriod.low;
	return SearchByKey(this, 1, &k1, pRec);
}

int SLAPI SalaryCore::Calc(PPID postID, PPID salChargeID, int avg, const DateRange & rPeriod, double * pAmount)
{
	int    ok = -1;
	int    sp = spGe;
	long   count = 0;
	double amount = 0.0;
	SalaryTbl::Key1 k1;
	MEMSZERO(k1);
	k1.PostID = postID;
	k1.SalChargeID = salChargeID;
	k1.Beg = rPeriod.low;
	BExtQuery q(this, 1);
	q.select(this->ID, this->Beg, this->End, this->Amount, 0L).
		where(this->PostID == postID && this->SalChargeID == salChargeID && daterange(this->Beg, &rPeriod));
	for(q.initIteration(0, &k1, spGe); q.nextIteration() > 0;) {
		if(!rPeriod.upp || data.End <= rPeriod.upp) {
			amount += data.Amount;
			count++;
			ok = 1;
		}
	}
	if(avg)
		amount = fdivnz(amount, count);
	ASSIGN_PTR(pAmount, amount);
	return ok;
}

int SLAPI SalaryCore::GetIntersection(PPID postID, PPID salChargeID, const DateRange & rPeriod, SalaryTbl::Rec * pRec)
{
	int    ok = -1, r, sp = spGe;
	SalaryTbl::Key1 k1;
	MEMSZERO(k1);
	k1.PostID = postID;
	k1.SalChargeID = salChargeID;
	while(ok < 0 && (r = search(1, &k1, sp)) && k1.PostID == postID && k1.SalChargeID == salChargeID) {
		DateRange temp;
		temp.Set(data.Beg, data.End);
		if(temp.IsEqual(rPeriod)) {
			copyBufTo(pRec);
			ok = 2;
		}
		else if(temp.IsIntersect(rPeriod)) {
			copyBufTo(pRec);
			ok = 1;
		}
		sp = spGt;
	}
	if(ok < 0 && !BTROKORNFOUND)
		ok = PPSetErrorDB();
	return ok;
}

int SLAPI SalaryCore::GetObjectList(PPID objType, const DateRange & rPeriod, const UintHashTable * pIdList, PPIDArray * pList)
{
	int    ok = -1;
	SalaryTbl::Key2 k2;
	DBQ * dbq = 0;
	BExtQuery q(this, 2);
	dbq = &(this->Beg >= rPeriod.low && this->Beg <= rPeriod.upp && this->End <= rPeriod.upp && this->End >= rPeriod.low);
	q.select(this->ID, this->PostID, this->SalChargeID, this->GenBillID, 0L).where(*dbq);
	MEMSZERO(k2);
	k2.Beg = rPeriod.low;
	for(q.initIteration(0, &k2, spGe); q.nextIteration() > 0;) {
		if(!pIdList || pIdList->Has(data.ID)) {
			if(objType == PPOBJ_PERSONPOST)
				pList->addUnique(data.PostID);
			else if(objType == PPOBJ_SALCHARGE)
				pList->addUnique(data.SalChargeID);
			else if(objType == PPOBJ_BILL)
				pList->addUnique(data.GenBillID);
			ok = 1;
		}
	}
	return ok;
}

int SLAPI SalaryCore::GetListByObject(PPID objType, PPID objID, const DateRange & rPeriod,
	const UintHashTable * pIdList, PPIDArray * pList, double * pAmount)
{
	int    ok = -1, idx = -1;
	union {
		SalaryTbl::Key2 k2;
		SalaryTbl::Key3 k3;
		SalaryTbl::Key4 k4;
	} k;
	DBQ * dbq = 0;
	MEMSZERO(k);
	if(objType == PPOBJ_SALCHARGE) {
		idx = 3;
		k.k3.Beg = rPeriod.low;
		k.k3.SalChargeID = objID;
		// dbq = &(*dbq && this->SalChargeID == objID && this->Beg == rPeriod.low && this->End == rPeriod.upp);
		dbq = &(this->SalChargeID == objID && this->Beg >= rPeriod.low && this->Beg <= rPeriod.upp &&
			this->End <= rPeriod.upp && this->End >= rPeriod.low);
	}
	else if(objType == PPOBJ_PERSONPOST) {
		idx = 2;
		k.k2.Beg = rPeriod.low;
		//dbq = &(*dbq && this->Beg == rPeriod.low && this->End == rPeriod.upp && this->PostID == objID);
		dbq = &(this->Beg >= rPeriod.low && this->Beg <= rPeriod.upp &&
			this->End <= rPeriod.upp && this->End >= rPeriod.low && this->PostID == objID);
	}
	else if(objType == PPOBJ_BILL) {
		idx = 4;
		k.k4.GenBillID = objID;
		//dbq = &(*dbq && this->GenBillID == objID && this->Beg == rPeriod.low && this->End == rPeriod.upp);
		dbq = &(this->GenBillID == objID && this->Beg >= rPeriod.low && this->Beg <= rPeriod.upp &&
			this->End <= rPeriod.upp && this->End >= rPeriod.low);
	}
	if(idx != -1) {
		BExtQuery q(this, idx);
		q.select(this->ID, this->PostID, this->SalChargeID, this->GenBillID, 0L).where(*dbq);
		for(q.initIteration(0, &k, spGe); q.nextIteration() > 0;) {
			if(!pIdList || pIdList->Has(data.ID)) {
				pList->add(data.ID);
				ok = 1;
			}
		}
	}
	return ok;
}
//
// @ModuleDef(PPViewSalary)
//
IMPLEMENT_PPFILT_FACTORY(Salary); SLAPI SalaryFilt::SalaryFilt() : PPBaseFilt(PPFILT_SALARY, 0, 0)
{
	SetFlatChunk(offsetof(SalaryFilt, ReserveStart),
		offsetof(SalaryFilt, Reserve)-offsetof(SalaryFilt, ReserveStart)+sizeof(Reserve));
	Init(1, 0);
}
//
//
//
SLAPI PPViewSalary::PPViewSalary() : PPView(0, &Filt, PPVIEW_SALARY)
{
	P_TempTbl = 0;
}

SLAPI PPViewSalary::~PPViewSalary()
{
	if(!(ImplementFlags & implDontDelTempTables))
		ZDELETE(P_TempTbl);
}

int SLAPI PPViewSalary::EditBaseFilt(PPBaseFilt * pBaseFilt)
{
#define GRP_DIV 1
	int    ok = -1, valid_data = 0;
	TDialog * dlg = 0;
	THROW(Filt.IsA(pBaseFilt));
	THROW(CheckDialogPtr(&(dlg = new TDialog(DLG_SALARYFLT))));
	{
		SalaryFilt * p_filt = (SalaryFilt *)pBaseFilt;
		dlg->SetupCalPeriod(CTLCAL_SALARYFLT_PERIOD, CTL_SALARYFLT_PERIOD);
		dlg->addGroup(GRP_DIV, new DivisionCtrlGroup(CTLSEL_SALARYFLT_ORG, CTLSEL_SALARYFLT_DIV,
			CTLSEL_SALARYFLT_STAFF, CTLSEL_SALARYFLT_POST));
		DivisionCtrlGroup::Rec grp_rec(p_filt->OrgID, p_filt->DivID, p_filt->StaffID, p_filt->PostID);
		dlg->setGroupData(GRP_DIV, &grp_rec);
		SetPeriodInput(dlg, CTL_SALARYFLT_PERIOD, &p_filt->Period);
		SetupPPObjCombo(dlg, CTLSEL_SALARYFLT_CHARGE, PPOBJ_SALCHARGE, p_filt->SalChargeID, 0, 0);
		while(!valid_data && ExecView(dlg) == cmOK)
			if(dlg->getGroupData(GRP_DIV, &grp_rec)) {
				if(GetPeriodInput(dlg, CTL_SALARYFLT_PERIOD, &p_filt->Period)) {
					p_filt->OrgID = grp_rec.OrgID;
					p_filt->DivID = grp_rec.DivID;
					p_filt->StaffID = grp_rec.StaffID;
					p_filt->PostID = grp_rec.PostID;
					p_filt->SalChargeID = dlg->getCtrlLong(CTLSEL_SALARYFLT_CHARGE);
					ok = valid_data = 1;
				}
				else
					PPError();
			}
			else
				PPError();
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
#undef GRP_DIV
}

class SalaryDialog : public TDialog {
#define GRP_DIV 1
public:
	SalaryDialog() : TDialog(DLG_SALARY)
	{
		OrgID = DivID = StaffID = 0;
	}
	int    setDTS(const SalaryTbl::Rec * pData)
	{
		Data = *pData;
		PersonPostTbl::Rec post_rec;
		PPStaffEntry sl_rec;
		OrgID = DivID = StaffID = 0;
		DateRange period;
		period.Set(Data.Beg, Data.End);
		if(SlObj.FetchPost(Data.PostID, &post_rec) > 0) {
			StaffID = post_rec.StaffID;
			if(SlObj.Fetch(StaffID, &sl_rec) > 0) {
				OrgID = sl_rec.OrgID;
				DivID = sl_rec.DivisionID;
			}
		}
		SetupCalPeriod(CTLCAL_SALARY_PERIOD, CTL_SALARY_PERIOD);
		addGroup(GRP_DIV, new DivisionCtrlGroup(CTLSEL_SALARY_ORG, CTLSEL_SALARY_DIV, CTLSEL_SALARY_STAFF, 0));
		DivisionCtrlGroup::Rec grp_rec(OrgID, DivID, StaffID);
		setGroupData(GRP_DIV, &grp_rec);
		SetPeriodInput(this, CTL_SALARY_PERIOD, &period);
		SetupPPObjCombo(this, CTLSEL_SALARY_CHARGE, PPOBJ_SALCHARGE, Data.SalChargeID, 0, (void *)-10000);
		PPObjStaffList::SetupPostCombo(this, CTLSEL_SALARY_POST, Data.PostID, 0, OrgID, DivID, StaffID);
		setCtrlReal(CTL_SALARY_AMOUNT, Data.Amount);
		return 1;
	}
	int    getDTS(SalaryTbl::Rec * pData)
	{
		int    ok = 1;
		uint   sel = 0;
		DateRange period;
		DivisionCtrlGroup::Rec grp_rec;
		THROW(getGroupData(GRP_DIV, &grp_rec));
		THROW(GetPeriodInput(this, sel = CTL_SALARY_PERIOD, &period));
		Data.Beg = period.low;
		Data.End = period.upp;
		OrgID = grp_rec.OrgID;
		DivID = grp_rec.DivID;
		StaffID = grp_rec.StaffID;
		THROW_PP(Data.SalChargeID = getCtrlLong(sel = CTLSEL_SALARY_CHARGE), PPERR_SALCHARGENEEDED);
		THROW_PP(Data.PostID = getCtrlLong(sel = CTLSEL_SALARY_POST), PPERR_STAFFPOSTNEEDED);
		getCtrlData(sel = CTL_SALARY_AMOUNT, &Data.Amount);
		THROW_PP(fabs(Data.Amount) < 100000000., PPERR_USERINPUT);
		ASSIGN_PTR(pData, Data);
		CATCH
			ok = PPErrorByDialog(this, sel);
		ENDCATCH
		return ok;
	}
private:
	DECL_HANDLE_EVENT
	{
		TDialog::handleEvent(event);
		if(event.isCbSelected(CTLSEL_SALARY_STAFF)) {
			DivisionCtrlGroup::Rec grp_rec;
			getGroupData(GRP_DIV, &grp_rec);
			OrgID   = grp_rec.OrgID;
			DivID   = grp_rec.DivID;
			StaffID = grp_rec.StaffID;
			Data.PostID = 0;
			PPObjStaffList::SetupPostCombo(this, CTLSEL_SALARY_POST, Data.PostID, 0, OrgID, DivID, StaffID);
			clearEvent(event);
		}
	}
	PPObjStaffList SlObj;
	SalaryTbl::Rec Data;
	PPID   OrgID;
	PPID   DivID;
	PPID   StaffID;
#undef GRP_DIV
};

int SLAPI PPViewSalary::EditItemDialog(SalaryTbl::Rec * pRec)
{
	DIALOG_PROC_BODY(SalaryDialog, pRec);
}

int SLAPI PPViewSalary::IsTempTblNeeded() const
{
	return BIN(Filt.StaffID || Filt.OrgID || Filt.DivID || Filt.PersonID || (Filt.Flags & SalaryFilt::fCrosstab));
}

int SLAPI PPViewSalary::MakeTempRec(int order, const SalaryTbl::Rec * pRec, TempSalaryTbl::Rec * pTempRec)
{
	memzero(pTempRec, sizeof(*pTempRec));
	#define FLD(f) pTempRec->f = pRec->f
	FLD(ID);
	FLD(Beg);
	FLD(End);
	FLD(PostID);
	FLD(SalChargeID);
	FLD(ExtObjID);
	FLD(Amount);
	FLD(Flags);
	FLD(LinkBillID);
	FLD(GenBillID);
	FLD(RByGenBill);
	#undef FLD
	{
		PersonPostTbl::Rec post_rec;
		PPStaffEntry sl_rec;
		if(SlObj.FetchPost(pRec->PostID, &post_rec) > 0) {
			pTempRec->StaffID  = post_rec.StaffID;
			pTempRec->PersonID = post_rec.PersonID;
			if(SlObj.Fetch(post_rec.StaffID, &sl_rec) > 0) {
				pTempRec->OrgID = sl_rec.OrgID;
				pTempRec->DivID = sl_rec.DivisionID;
			}
			if(Filt.Flags & SalaryFilt::fCrosstab) {
				PersonTbl::Rec psn_rec;
				if(SlObj.PsnObj.Fetch(post_rec.PersonID, &psn_rec) > 0)
					STRNSCPY(pTempRec->Text, psn_rec.Name);
			}
		}
	}
	return 1;
}

int SLAPI PPViewSalary::TempRecToViewItem(TempSalaryTbl::Rec * pTempRec, SalaryViewItem * pItem)
{
	if(pItem) {
		memzero(pItem, sizeof(*pItem));
		#define FLD(f) pItem->f = pTempRec->f
		FLD(ID);
		FLD(Beg);
		FLD(End);
		FLD(PostID);
		FLD(SalChargeID);
		FLD(Amount);
		FLD(Flags);
		FLD(LinkBillID);
		FLD(GenBillID);
		FLD(RByGenBill);
		FLD(StaffID);
		FLD(PersonID);
		FLD(OrgID);
		FLD(DivID);
		#undef FLD
	}
	return 1;
}

// @v5.6.14 AHTOXA {
int SLAPI PPViewSalary::GetSalChargeGroupItems(PPID salChargeGrpID, PPIDArray * pItems) const
{
	int ok = -1;
	PPSalChargePacket pack;
	PPObjSalCharge sc_obj;
	if(sc_obj.GetPacket(salChargeGrpID, &pack) > 0 && (pack.Rec.Flags & PPSalCharge::fGroup)) {
		if(pItems) {
			*pItems = pack.GrpList;
			pItems->sort();
		}
		ok = 1;
	}
	return ok;
}

int SLAPI PPViewSalary::GetSalChargeName(PPID salChargeID, SString & rName)
{
	SString buf;
	rName = 0;
	GetObjectName(PPOBJ_SALCHARGE, salChargeID, buf);
	if(Filt.Flags & SalaryFilt::fCrosstab) {
		uint pos = 0;
		SalChargeList.bsearch(salChargeID, &pos);
		rName.Printf("%02ld %s", pos + 1, buf.cptr());
	}
	else
		rName = buf;
	return 1;
}

class SalaryCrosstab : public Crosstab {
public:
	SLAPI  SalaryCrosstab(PPViewSalary * pV) : Crosstab()
	{
		P_V = pV;
	}
	virtual BrowserWindow * SLAPI CreateBrowser(uint brwId, int dataOwner)
	{
		PPViewBrowser * p_brw = new PPViewBrowser(brwId, CreateBrowserQuery(), P_V, dataOwner);
		SetupBrowserCtColumns(p_brw);
		return p_brw;
	}
protected:
	virtual int SLAPI GetTabTitle(const void * pVal, TYPEID typ, SString & rBuf) const
	{
		return (pVal && P_V) ? P_V->GetTabTitle(*(const long *)pVal, rBuf) : 0;
	}
	PPViewSalary * P_V;
};

int SLAPI PPViewSalary::GetTabTitle(long tabID, SString & rBuf) const
{
	return GetObjectName(PPOBJ_SALCHARGE, tabID, (rBuf = 0));
}
// } @v5.4.1 AHTOXA

PP_CREATE_TEMP_FILE_PROC(CreateTempFile, TempSalary);

int SLAPI PPViewSalary::UpdateTempRec(PPID id)
{
	int    ok = 1;
	if(P_TempTbl) {
		TempSalaryTbl::Rec temp_rec;
		SalaryTbl::Rec rec;
		PPTransaction tra(ppDbDependTransaction, 1);
		THROW(tra);
		if(Tbl.Search(id, &rec) > 0) {
			if(SearchByID_ForUpdate(P_TempTbl, 0, id, &temp_rec) > 0) {
				MakeTempRec(0, &rec, &temp_rec);
				THROW_DB(P_TempTbl->updateRecBuf(&temp_rec)); // @sfu
			}
		}
		else
			THROW_DB(deleteFrom(P_TempTbl, 0, (P_TempTbl->ID == id)));
		THROW(tra.Commit());
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

int SLAPI PPViewSalary::Init_(const PPBaseFilt * pFilt)
{
	int    ok = 1, use_ta = 1;
	TempSalaryTbl * p_tbl = 0;
	ZDELETE(P_TempTbl);
	ZDELETE(P_Ct);
	SalChargeList.freeAll();
	THROW(Helper_InitBaseFilt(pFilt));
	Filt.Period.Actualize(ZERODATE); // @v6.1.10
	SETFLAG(Filt.Flags, SalaryFilt::fCrosstab, GetSalChargeGroupItems(Filt.SalChargeID, &SalChargeList) > 0);
	if(IsTempTblNeeded()) {
		{
			ZDELETE(P_TempTbl);
			int    order = 0;
			SalaryViewItem item;
			THROW(p_tbl = CreateTempFile());
			{
				BExtInsert bei(p_tbl);
				PPTransaction tra(ppDbDependTransaction, use_ta);
				THROW(tra);
				for(InitIteration(0); NextIteration(&item) > 0;) {
					TempSalaryTbl::Rec temp_rec;
					MakeTempRec(order, (SalaryTbl::Rec *)&item, &temp_rec);
					THROW_DB(bei.insert(&temp_rec));
				}
				THROW_DB(bei.flash());
				THROW(tra.Commit());
			}
			P_TempTbl = p_tbl;
		}
		{
			ZDELETE(P_Ct);
			if((Filt.Flags & SalaryFilt::fCrosstab) && P_TempTbl) {
				SString temp_buf;
				DBFieldList total_list;
				THROW_MEM(P_Ct = new SalaryCrosstab(this));
				P_Ct->SetTable(P_TempTbl, P_TempTbl->SalChargeID);
				P_Ct->AddIdxField(P_TempTbl->Beg);
				P_Ct->AddIdxField(P_TempTbl->PostID);
				P_Ct->AddIdxField(P_TempTbl->PersonID);
				P_Ct->AddInheritedFixField(P_TempTbl->End);
				P_Ct->AddInheritedFixField(P_TempTbl->Text);
				P_Ct->AddAggrField(P_TempTbl->Amount);
				total_list.Add(P_TempTbl->Amount);
				P_Ct->AddTotalRow(total_list, 0, PPGetWord(PPWORD_TOTAL, 0, temp_buf));
				THROW(P_Ct->Create(use_ta));
				ok = 1;
			}
		}
	}
	CATCH
		ZDELETE(P_Ct);
		ZDELETE(p_tbl);
		ok = 0;
	ENDCATCH
	return ok;
}

int SLAPI PPViewSalary::InitIteration(int order)
{
	ZDELETE(P_IterQuery);
	int    ok = 1, idx = 0;
	DBQ  * dbq = 0;
	BExtQuery * q = 0;
	if(P_TempTbl) {
		void * k;
		TempSalaryTbl::Key2 k2;
		TempSalaryTbl::Key3 k3;
		MEMSZERO(k2);
		MEMSZERO(k3);
		if(order == ordByPostID) {
			idx = 3;
			k = &k3;
		}
		else {
			idx = 2;
			k = &k2;
		}
		THROW_MEM(q = new BExtQuery(P_TempTbl, idx));
		q->selectAll(); // q->select(P_TempTbl->ID, 0L);
		Counter.Init(q->countIterations(0, k, spFirst));
		MEMSZERO(k2);
		MEMSZERO(k3);
		q->initIteration(0, &k, spFirst);
	}
	else {
		union  {
			SalaryTbl::Key1 k1;
			SalaryTbl::Key2 k2;
			SalaryTbl::Key3 k3;
		} k, k_;
		MEMSZERO(k);
		if(Filt.PostID || order == ordByPostID) {
			idx = 1;
			k.k1.PostID = Filt.PostID;
		}
		else if((!Filt.Flags & SalaryFilt::fCrosstab) && Filt.SalChargeID) { // @v5.6.14 AHTOXA
			idx = 3;
			k.k3.SalChargeID = Filt.SalChargeID;
		}
		else {
			idx = 2;
			if(!Filt.Period.IsZero())
				k.k2.Beg = Filt.Period.low;
		}
		THROW_MEM(q = new BExtQuery(&Tbl, idx));
		dbq = ppcheckfiltid(dbq, Tbl.PostID, Filt.PostID);
		if((!Filt.Flags & SalaryFilt::fCrosstab) && Filt.SalChargeID)    // @v5.6.14 AHTOXA
			dbq = ppcheckfiltid(dbq, Tbl.SalChargeID, Filt.SalChargeID); // @v5.6.14 AHTOXA
		dbq = & (*dbq && daterange(Tbl.Beg, &Filt.Period));
		q->where(*dbq);
		k_ = k;
		Counter.Init(q->countIterations(0, &k_, spGe));
		q->initIteration(0, &k, spGe);
	}
	P_IterQuery = q;
	CATCH
		ZDELETE(q);
		ok = 0;
	ENDCATCH
	return ok;
}

int SLAPI PPViewSalary::NextIteration(SalaryViewItem * pItem)
{
	int    ok = -1;
	memzero(pItem, sizeof(*pItem));
	for(; ok < 0 && P_IterQuery && P_IterQuery->nextIteration() > 0; Counter.Increment()) {
		if(P_TempTbl) {
			TempRecToViewItem(&P_TempTbl->data, pItem);
			ok = 1;
		}
		else {
			const SalaryTbl::Rec & r_rec = Tbl.data;
			PersonPostTbl::Rec post_rec;
			PPStaffEntry sl_rec;
			if(Filt.StaffID || Filt.OrgID || Filt.DivID || Filt.PersonID || (Filt.Flags & SalaryFilt::fCrosstab)) {
				if(SlObj.SearchPost(Tbl.data.PostID, &post_rec) > 0) {
					if(Filt.StaffID && post_rec.StaffID != Filt.StaffID)
						continue;
					else if(Filt.PersonID && post_rec.PersonID != Filt.PersonID)
						continue;
					else if(Filt.OrgID || Filt.DivID || (Filt.Flags & SalaryFilt::fCrosstab)) {
						if(SlObj.Fetch(post_rec.StaffID, &sl_rec) > 0) {
							if(Filt.OrgID && sl_rec.OrgID != Filt.OrgID)
								continue;
							else if(Filt.DivID && sl_rec.DivisionID != Filt.DivID)
								continue;
							else if((Filt.Flags & SalaryFilt::fCrosstab) && SalChargeList.bsearch(r_rec.SalChargeID) <= 0)
								continue;
							else {
								if(pItem) {
									*(SalaryTbl::Rec *)pItem = r_rec;
									pItem->StaffID  = post_rec.StaffID;
									pItem->PersonID = post_rec.PersonID;
									pItem->OrgID    = sl_rec.OrgID;
									pItem->DivID    = sl_rec.DivisionID;
								}
								ok = 1;
							}
						}
					}
					else {
						if(pItem) {
							//
							// Двойная перегонка данных - зато избегаем лишнего кода
							//
							TempSalaryTbl::Rec temp_rec;
							MakeTempRec(0, &r_rec, &temp_rec);
							TempRecToViewItem(&temp_rec, pItem);
						}
						ok = 1;
					}
				}
			}
			else {
				if(pItem) {
					//
					// Двойная перегонка данных - зато избегаем лишнего кода
					//
					TempSalaryTbl::Rec temp_rec;
					MakeTempRec(0, &r_rec, &temp_rec);
					TempRecToViewItem(&temp_rec, pItem);
				}
				ok = 1;
			}
		}
	}
	return ok;
}

DBQuery * SLAPI PPViewSalary::CreateBrowserQuery(uint * pBrwId, SString * pSubTitle)
{
	uint   brw_id = 0;
	DBQuery * q = 0;
	if(P_Ct)
		brw_id = BROWSER_SALARYCT; // @v5.6.14 AHTOXA
	else {
		brw_id = BROWSER_SALARY;
		SalaryTbl     * p_slr = 0;
		PersonPostTbl * p_pp = 0;
		TempSalaryTbl * p_tmp = 0;
		DBQ  * dbq = 0;
		DBE    dbe_staff, dbe_psn, dbe_charge, dbe_bill;
		if(P_TempTbl) {
			THROW_MEM(p_tmp = new TempSalaryTbl(P_TempTbl->fileName));
			PPDbqFuncPool::InitObjNameFunc(dbe_staff,  PPDbqFuncPool::IdObjNameStaff, p_tmp->StaffID);
			PPDbqFuncPool::InitObjNameFunc(dbe_psn,    PPDbqFuncPool::IdObjNamePerson, p_tmp->PersonID);
			PPDbqFuncPool::InitObjNameFunc(dbe_charge, PPDbqFuncPool::IdObjNameSalCharge, p_tmp->SalChargeID);
			PPDbqFuncPool::InitObjNameFunc(dbe_bill,   PPDbqFuncPool::IdObjCodeBillCmplx, p_tmp->GenBillID);
			q = & select(
				p_tmp->ID,     // #00
				p_tmp->Beg,    // #01
				p_tmp->End,    // #02
				dbe_psn,       // #03
				dbe_staff,     // #04
				dbe_charge,    // #05
				p_tmp->Amount, // #06
				dbe_bill,      // #07
				0L).from(p_tmp, 0).orderBy(p_tmp->Beg, 0L);
		}
		else {
			THROW_MEM(p_slr = new SalaryTbl);
			THROW_MEM(p_pp  = new PersonPostTbl);
			PPDbqFuncPool::InitObjNameFunc(dbe_staff,  PPDbqFuncPool::IdObjNameStaff,     p_pp->StaffID);
			PPDbqFuncPool::InitObjNameFunc(dbe_psn,    PPDbqFuncPool::IdObjNamePerson,    p_pp->PersonID);
			PPDbqFuncPool::InitObjNameFunc(dbe_charge, PPDbqFuncPool::IdObjNameSalCharge, p_slr->SalChargeID);
			PPDbqFuncPool::InitObjNameFunc(dbe_bill,   PPDbqFuncPool::IdObjCodeBillCmplx, p_slr->GenBillID);
			dbq = & (*dbq && p_pp->ID == p_slr->PostID);
			dbq = ppcheckfiltid(dbq, p_slr->PostID, Filt.PostID);
			dbq = ppcheckfiltid(dbq, p_slr->SalChargeID, Filt.SalChargeID);
			dbq = & (*dbq && daterange(p_slr->Beg, &Filt.Period));
			q = & select(
				p_slr->ID,     // #00
				p_slr->Beg,    // #01
				p_slr->End,    // #02
				dbe_psn,       // #03
				dbe_staff,     // #04
				dbe_charge,    // #05
				p_slr->Amount, // #06
				dbe_bill,      // #07
				0L).from(p_slr, p_pp, 0).where(*dbq).orderBy(p_slr->Beg, 0L);
		}
	}
	CATCH
		ZDELETE(q);
	ENDCATCH
	ASSIGN_PTR(pBrwId, brw_id);
	return (P_Ct) ? PPView::CrosstabDbQueryStub : q;
}

int SLAPI PPViewSalary::Print(const void *)
{
	int    ok = -1;
	uint   what = 0;
	if(SelectorDialog(DLG_PRNSAL, CTL_PRNSAL_WHAT, &what) > 0) {
		uint rpt_id = (what == 0) ? ((P_Ct) ? REPORT_SALARYCT : REPORT_SALARY) : REPORT_SALARYBYPOST;
		ok = Helper_Print(rpt_id, what);
	}
	return ok;
}

PPID SLAPI PPViewSalary::GetEditId(PPID id, PPViewBrowser * pBrw)
{
	PPID   ret_id = 0;
	if(P_Ct) {
		uint   tab_idx = pBrw ? pBrw->GetCurColumn() : 0;
		PPID   tab_id = 0;
		DBFieldList fld_list; // realy const, do not modify
		int    r = (tab_idx > 2) ? P_Ct->GetTab(tab_idx - 3, &tab_id) : -1;

		if(r > 0 && P_Ct->GetIdxFields(id, &fld_list) > 0) {
			SalaryTbl::Key2 k2;
			fld_list.GetValue(0, &k2.Beg,    0);
			fld_list.GetValue(1, &k2.PostID, 0);
			k2.SalChargeID = tab_id;
			if(Tbl.search(2, &k2, spEq) > 0)
				ret_id = Tbl.data.ID;
		}
	}
	else
		ret_id = id;
	return ret_id;
}

int SLAPI PPViewSalary::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	if(ok == -2) {
		PPID   id = pHdr ? *(PPID *)pHdr : 0;
		SalaryTbl::Rec rec;
		PersonPostTbl::Rec post_rec;
		if(ppvCmd == PPVCMD_ADDITEM) {
			MEMSZERO(rec);
			if(EditItemDialog(&rec) > 0)
				ok = Tbl.Put(&(id = 0), &rec, 1) ? 1 : PPErrorZ();
			else
				ok = -1;
			if(ok > 0 && P_Ct)
				ChangeFilt(1, pBrw);
		}
		else if(ppvCmd == PPVCMD_EDITITEM) {
			id = GetEditId(id, pBrw);
			if(Tbl.Search(id, &rec) > 0 && EditItemDialog(&rec) > 0) {
				ok = Tbl.Put(&id, &rec, 1) ? 1 : PPErrorZ();
				if(ok > 0) {
					UpdateTempRec(id);
					if(P_Ct)
						ChangeFilt(1, pBrw);
				}
			}
			else
				ok = -1;
		}
		else if(ppvCmd == PPVCMD_DELETEITEM) {
			id = GetEditId(id, pBrw);
			if(id > 0 && CONFIRM(PPCFM_DELETE)) {
				ok = Tbl.Put(&id, 0, 1) ? 1 : PPErrorZ();
				if(ok > 0) {
					UpdateTempRec(id);
					if(P_Ct)
						ChangeFilt(1, pBrw);
				}
			}
			else
				ok = -1;
		}
		else if(ppvCmd == PPVCMD_DELETEALL) {
			ok = -1;
			if(CONFIRMCRIT(PPCFM_DELETE)) {
				SalaryViewItem item;
				PPIDArray id_list;
				for(InitIteration(0); ok && NextIteration(&item) > 0;)
					id_list.addUnique(item.ID);
				for(uint i = 0; i < id_list.getCount(); i++) {
					PPID id = id_list.get(i);
					if(Tbl.Put(&id, 0, 1)) {
						UpdateTempRec(id);
						ok = 1;
					}
					else
						PPErrorZ();
				}
				if(ok > 0 && P_Ct)
					ChangeFilt(1, pBrw);
			}
		}
		else if(ppvCmd == PPVCMD_EDITPERSON) {
			ok = -1;
			if(Tbl.Search(id, &rec) > 0 && SlObj.FetchPost(rec.PostID, &post_rec) > 0)
				if(SlObj.PsnObj.Edit(&post_rec.PersonID, 0) == cmOK)
					ok = 1;
			if(ok > 0 && P_Ct)
				ChangeFilt(1, pBrw);
		}
		else if(ppvCmd == PPVCMD_EDITSTAFF) {
			ok = -1;
			if(Tbl.Search(id, &rec) > 0 && SlObj.FetchPost(rec.PostID, &post_rec) > 0)
				if(SlObj.Edit(&post_rec.StaffID, 0) == cmOK)
					ok = 1;
		}
		else if(ppvCmd == PPVCMD_EDITSALCHARGE) {
			ok = -1;
			if(Tbl.Search(id, &rec) > 0) {
				PPObjSalCharge sc_obj;
				if(sc_obj.Edit(&rec.SalChargeID, 0) == cmOK)
					ok = 1;
			}
			if(ok > 0 && P_Ct)
				ChangeFilt(1, pBrw);
		}
		else if(ppvCmd == PPVCMD_EDITBILL) {
			ok = -1;
			if(Tbl.Search(id, &rec) > 0 && rec.GenBillID)
				BillObj->Edit(&rec.GenBillID, 0);
		}
		else if(ppvCmd == PPVCMD_CHARGE) {
			ok = -1;
			PrcssrSalary prcssr;
			PrcssrSalary::Param param;
			prcssr.InitParam(&param);
			param.ActualPeriod = Filt.Period;
			param.NominalPeriod = Filt.Period;
			param.OrgID = Filt.OrgID;
			param.DivID = Filt.DivID;
			param.StaffID = Filt.StaffID;
			if(prcssr.EditParam(&param) > 0)
				if(prcssr.Init(&param) && prcssr.Run())
					ok = 1;
				else
					ok = PPErrorZ();
			if(ok > 0 && P_Ct)
				ChangeFilt(1, pBrw);
		}
	}
	return ok;
}
//
// @ModuleDef(PrcssrSalary)
//
class SalaryContext : public ExprEvalContext {
public:
	SLAPI  SalaryContext(PrcssrSalary * pPrcssr)
		{ P_Prcssr = pPrcssr; }
	virtual int SLAPI Resolve(const char * pSymb, double * pVal)
		{ return P_Prcssr ? P_Prcssr->Expr_ResolveSymb(pSymb, pVal) : 0; }
	virtual int SLAPI IsFunc(const char * pSymb, int * pFuncId)
		{ return P_Prcssr ? P_Prcssr->IsFunc(pSymb, pFuncId) : 0; }
	virtual int SLAPI ResolveFunc(int funcId, FC & rFc)
	{
		return P_Prcssr ? P_Prcssr->Expr_ResolveFunc(funcId,
			rFc.ArgList.getCount(), (double *)rFc.ArgList.dataPtr(), &rFc.RetReal) : 0;
	}
private:
	PrcssrSalary * P_Prcssr; // Not owned by this
};

/*
sckName     - вид начисления с символом Name //
samtName    - тип штатной суммы с символом Name
amtName     - тип суммы документов с символом Name
evName      - вид персональной операции с символом Name
prLastMonth - последний месяц до периода начисления                    = -30.   //
prLastQuart - последний квартал до периода начисления                  = -90.   //
prLastYear  - последний год до периода начисления                      = -360.  //
prThisMonth - с начала этого месяца                                    = -30.5  //
prThisQuart - с начала этого квартала                                  = -90.5  //
prThisYear  - с начала этого года                                      = -360.5 //
prNominal   - период начисления                                        = 1.    //
prActual    - период начисления с поправкой на дату приема (уволнения) = 1.5   //
tagName     - значение тега персоналии (преобразуется к double)

charge      - сумма начисления //
	Arg1 - вид начисления //
	Arg2 - период (если не задан, то за период начисления)
avgcharge   - средняя величина начисления //
	Arg1 - вид начисления //
	Arg2 - период (если период не задан, то равно charge(Arg1))
bill        - сумма по документам за заданный период.
	Arg1 - ид вида операции
	Arg2 - если 0 - то документы поднимаются по персоналии, как контрагенту документа
		   1 - документы поднимаются по персоналии, как дополнительному объекту документа
		   2 - документы поднимаются по персоналии, как агенту по документу
	Arg3 - период (если не задан, то за период начисления)
paym        - сумма оплат за заданный период по документам
	Arg1 - ид вида операции документов, к которым привязаны оплаты
	Arg2 - если 0 - то документы поднимаются по персоналии, как контрагенту документа
		   1 - документы поднимаются по персоналии, как дополнительному объекту документа
		   2 - документы поднимаются по персоналии, как агенту по документу
	Arg3 - период (если не задан, то за период начисления)

sieve[0; 20000:0.15; 40000:0.18](x)
*/

SLAPI PrcssrSalary::FuncDescr::FuncDescr()
{
	THISZERO();
}

int FASTCALL PrcssrSalary::FuncDescr::IsEqHdr(const FuncDescr & rS) const
{
	return BIN(FuncId == rS.FuncId && AmtID == rS.AmtID && Flags == rS.Flags);
}

SLAPI PrcssrSalary::PrcssrSalary() : PeriodStack(64)
{
	MEMSZERO(P);
	PPObjAmountType amt_obj;
	PPID   id;
	PPAmountType amt_rec;
	PPStaffCal sc_rec;
	P_BillView = 0;
	P_EvView = 0;
	P_CurEv = 0;
	LastFuncId = EXRP_EVAL_FIRST_FUNC;
	for(id = 0; amt_obj.EnumItems(&id, &amt_rec) > 0;)
		AmtTypeList.Add(amt_rec.ID, amt_rec.Symb);
	for(id = 0; CalObj.EnumItems(&id, &sc_rec) > 0;)
		CalList.Add(sc_rec.ID, sc_rec.Symb);
	{
		PPObjectTag tag_rec;
		for(SEnum en = PPRef->Enum(PPOBJ_TAG, 0); en.Next(&tag_rec) > 0;) {
			if(tag_rec.ObjTypeID == PPOBJ_PERSON) // @v6.3.x AHTOXA
				TagSymbList.Add(tag_rec.ID, tag_rec.Symb);
		}
	}
	{
		PPPsnOpKind pok_rec;
		for(SEnum en = PPRef->Enum(PPOBJ_PERSONOPKIND, 0); en.Next(&pok_rec) > 0;) {
			EvSymbList.Add(pok_rec.ID, pok_rec.Symb);
		}
	}
	{
		PPSalCharge sc_rec;
		for(SEnum en = PPRef->Enum(PPOBJ_SALCHARGE, 0); en.Next(&sc_rec) > 0;) {
			ScSymbList.Add(sc_rec.ID, sc_rec.Symb);
		}
	}
}

SLAPI PrcssrSalary::~PrcssrSalary()
{
	delete P_BillView;
	delete P_EvView;
}

int SLAPI PrcssrSalary::InitParam(Param * pParam)
{
	memzero(pParam, sizeof(*pParam));
	return 1;
}

int SLAPI PrcssrSalary::EditParam(Param * pParam)
{
#define GRP_DIV 1
	int    ok = -1;
	TDialog * dlg = new TDialog(DLG_PP_SALARY);
	if(CheckDialogPtrErr(&dlg)) {
		Param param = *pParam;
		dlg->SetupCalPeriod(CTLCAL_PP_SALARY_PERIOD, CTL_PP_SALARY_PERIOD);
		dlg->addGroup(GRP_DIV, new DivisionCtrlGroup(CTLSEL_PP_SALARY_ORG, CTLSEL_PP_SALARY_DIV, CTLSEL_PP_SALARY_STAFF, 0));
		DivisionCtrlGroup::Rec grp_rec(param.OrgID, param.DivID, param.StaffID);
		dlg->setGroupData(GRP_DIV, &grp_rec);
		SetPeriodInput(dlg, CTL_PP_SALARY_PERIOD, &param.NominalPeriod);
		dlg->AddClusterAssoc(CTL_PP_SALARY_FLAGS, 0, Param::fVerbose);
		dlg->SetClusterData(CTL_PP_SALARY_FLAGS, param.Flags);
		while(ok < 0 && ExecView(dlg) == cmOK)
			if(dlg->getGroupData(GRP_DIV, &grp_rec)) {
				if(GetPeriodInput(dlg, CTL_PP_SALARY_PERIOD, &param.NominalPeriod)) {
					param.OrgID = grp_rec.OrgID;
					param.DivID = grp_rec.DivID;
					param.StaffID = grp_rec.StaffID;
					dlg->GetClusterData(CTL_PP_SALARY_FLAGS, &param.Flags);
					ASSIGN_PTR(pParam, param);
					ok = 1;
				}
				else
					PPError();
			}
			else
				PPError();
	}
	else
		ok = 0;
	delete dlg;
	return ok;
#undef GRP_DIV
}

int SLAPI PrcssrSalary::Init(const Param * pParam)
{
	if(!RVALUEPTR(P, pParam))
		InitParam(&P);
	return 1;
}

int SLAPI PrcssrSalary::WriteOff(const UintHashTable * pIdList, int undoOnly, int use_ta)
{
	int    ok = 1;
	PPObjArticle ar_obj;
	SString memo_buf;
	SalaryTbl::Rec s_rec;
	PPIDArray charge_list, removed_bill_list;
	PPObjBill * p_bobj = BillObj;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		Tbl.GetObjectList(PPOBJ_SALCHARGE, P.NominalPeriod, pIdList, &charge_list);
		for(uint i = 0; i < charge_list.getCount(); i++) {
			const PPID sc_id = charge_list.get(i);
			PPSalChargePacket sc_pack;
			THROW(ScObj.Fetch(sc_id, &sc_pack) > 0);
			PPGetWord(PPWORD_CHARGE, 0, memo_buf).CatDiv('-', 1).Cat(sc_pack.Rec.Name);
			if(sc_pack.Rec.WrOffOpID) {
				PPOprKind op_rec;
				double amount = 0.0;
				PPIDArray list, bill_id_list;
				Logger.LogString(PPTXT_LOG_PRCSALWROFFCHARGE, sc_pack.Rec.Name);
				THROW(GetOpData(sc_pack.Rec.WrOffOpID, &op_rec) > 0);
				Tbl.GetListByObject(PPOBJ_SALCHARGE, sc_id, P.NominalPeriod, pIdList, &list, &amount);
				if(sc_pack.Rec.Flags & PPSalCharge::fWrOffSingle) {
					PPBillPacket pack;
					THROW(pack.CreateBlank2(op_rec.ID, P.NominalPeriod.upp, LConfig.Location, 0));
					pack.Rec.Amount = s_rec.Amount;
					pack.Amounts.Put(PPAMT_MAIN, 0, s_rec.Amount, 0, 1);
					STRNSCPY(pack.Rec.Memo, memo_buf);
					THROW(p_bobj->FillTurnList(&pack));
					THROW(p_bobj->TurnPacket(&pack, 0));
					bill_id_list.addUnique(pack.Rec.ID);
					Logger.LogAcceptMsg(PPOBJ_BILL, pack.Rec.ID, 0);
					for(uint j = 0; j < list.getCount(); j++) {
						PPID s_id = list.get(j);
						THROW(Tbl.Search(s_id, &s_rec) > 0);
						//
						// Удаляем документ, который был привязан к строке начисления до этого момента
						// (позаботимся о том, чтобы не пытаться удалить один документ дважды)
						//
						if(s_rec.GenBillID && !removed_bill_list.lsearch(s_rec.GenBillID)) {
							//
							// Не будем считать ошибкой отсутствие документа при попытке его удалить
							//
							if(p_bobj->Search(s_rec.GenBillID, 0) > 0) {
								THROW(p_bobj->RemovePacket(s_rec.GenBillID, 0));
								removed_bill_list.addUnique(s_rec.GenBillID);
							}
						}
						//
						// Фиксируем в строке начисления ид сгенерированного документа
						//
						s_rec.GenBillID = pack.Rec.ID;
						s_rec.RByGenBill = 0;
						THROW(Tbl.Put(&s_id, &s_rec, 0));
					}
				}
				else {
					for(uint j = 0; j < list.getCount(); j++) {
						PPID s_id = list.get(j);
						PersonPostTbl::Rec post_rec;
						PPBillPacket pack;
						THROW(Tbl.Search(s_id, &s_rec) > 0);
						//
						// Удаляем документ, который был привязан к строке начисления до этого момента
						// (позаботимся о том, чтобы не пытаться удалить один документ дважды)
						//
						if(s_rec.GenBillID && !removed_bill_list.lsearch(s_rec.GenBillID)) {
							//
							// Не будем считать ошибкой отсутствие документа при попытке его удалить
							//
							if(p_bobj->Search(s_rec.GenBillID, 0) > 0) {
								THROW(p_bobj->RemovePacket(s_rec.GenBillID, 0));
								removed_bill_list.addUnique(s_rec.GenBillID);
							}
							s_rec.GenBillID = 0;
						}
						THROW(pack.CreateBlank2(op_rec.ID, P.NominalPeriod.upp, LConfig.Location, 0));
						if(SlObj.FetchPost(s_rec.PostID, &post_rec) > 0) {
							//
							// Пытаемся установить персоналию, по которой было начисление,
							// в качестве контрагента по документу
							//
							PPID ar_id = 0;
							if(ar_obj.GetByPerson(op_rec.AccSheetID, post_rec.PersonID, &ar_id) > 0)
								pack.Rec.Object = ar_id;
							else if(ar_obj.GetByPerson(op_rec.AccSheet2ID, post_rec.PersonID, &ar_id) > 0)
								pack.Rec.Object2 = ar_id;
						}
						pack.Rec.Amount = s_rec.Amount;
						pack.Amounts.Put(PPAMT_MAIN, 0, s_rec.Amount, 0, 1);
						STRNSCPY(pack.Rec.Memo, memo_buf);
						THROW(p_bobj->FillTurnList(&pack));
						THROW(p_bobj->TurnPacket(&pack, 0));
						bill_id_list.addUnique(pack.Rec.ID);
						Logger.LogAcceptMsg(PPOBJ_BILL, pack.Rec.ID, 0);
						//
						// Фиксируем в строке начисления ид сгенерированного документа
						//
						s_rec.GenBillID = pack.Rec.ID;
						s_rec.RByGenBill = 0;
						THROW(Tbl.Put(&s_id, &s_rec, 0));
					}
				}
			}
			else {
				Logger.LogString(PPTXT_LOG_PRCSALNDEFWROFFOP, sc_pack.Rec.Name);
			}
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI PrcssrSalary::Run()
{
	int    ok = -1;
	PPObjArticle ar_obj;
	UintHashTable id_list;
	if(P.PostID) {
		THROW(ProcessPost(P.PostID, &id_list, 1));
	}
	else {
		PPViewStaffPost post_view, post_view2;
		StaffPostViewItem post_item;
		//
		// post_view && post_filt - для открытых назначений
		// post_view2 && post_filt2 - для закрытых в течении периода начисления назначений
		//
		StaffPostFilt post_filt;
		StaffPostFilt post_filt2;

		post_filt.OrgID = P.OrgID;
		post_filt.DivID = P.DivID;
		post_filt.StaffID = P.StaffID;
		post_filt.Flags |= StaffPostFilt::fOpenedOnly;

		post_filt2.OrgID = P.OrgID;
		post_filt2.DivID = P.DivID;
		post_filt2.StaffID = P.StaffID;
		post_filt2.Flags |= StaffPostFilt::fClosedOnly;
		post_filt2.FnPeriod = P.NominalPeriod;

		THROW(post_view.Init_(&post_filt));
		THROW(post_view2.Init_(&post_filt2));
		{
			PPTransaction tra(1);
			THROW(tra);
			for(post_view.InitIteration(0); post_view.NextIteration(&post_item) > 0;) {
				if(!ProcessPost(post_item.ID, &id_list, 0))
					Logger.LogLastError();
				else
					ok = 1;
			}
			//
			// Теперь проведем расчет по закрытым должностям, дата
			// закрытия которых находится в периоде начисления.
			//
			for(post_view2.InitIteration(0); post_view2.NextIteration(&post_item) > 0;) {
				if(!ProcessPost(post_item.ID, &id_list, 0))
					Logger.LogLastError();
				else
					ok = 1;
			}
			THROW(WriteOff(&id_list, 0, 0));
			THROW(tra.Commit());
		}
	}
	CATCH
		Logger.LogLastError();
		ok = PPErrorZ();
	ENDCATCH
	return ok;
}

int SLAPI PrcssrSalary::Expr_ResolveSymb(const char * pSymb, double * pVal)
{
	int    ok = 0;
	PPID   cur_id = 0; // ИД валюты (зарезервировано на будущее)
	uint   pos = 0;
	double result = 0.0;
	SString fmt_buf, msg_buf;
	if(sstreqi_ascii(pSymb, "stdcal")) { // стандартный календарь
		result = PPSTCAL_STANDARD;
		ok = 1;
	}
	else if(strnicmp(pSymb, "sck", 3) == 0) { // charge
		const char * p_symb = pSymb+3;
		uint   pos = 0;
		if(ScSymbList.SearchByText(p_symb, 1, &pos)) {
			result = (double)ScSymbList.at(pos).Id;
			if(!(P.Flags & Param::fSilent) && P.Flags & Param::fVerbose) {
				Logger.Log(PPFormatT(PPTXT_LOG_PRCSALUSEDPOSTAMT, &msg_buf, pSymb, result));
			}
			ok = 1;
		}
		else {
			PPSetError(PPERR_SALCHARGESYMBNFOUND, p_symb);
			if(!(P.Flags & Param::fSilent))
				Logger.LogLastError();
		}
	}
	else if(strnicmp(pSymb, "tag", 3) == 0) {
		const char * p_symb = pSymb+3;
		if(CurPostPack.Rec.PersonID) {
			uint   pos = 0;
			if(TagSymbList.SearchByText(p_symb, 1, &pos) > 0) {
				PPID   tag_id = TagSymbList.at(pos).Id;
				ObjTagItem tag_item;
				if(PPRef->Ot.GetTag(PPOBJ_PERSON, CurPostPack.Rec.PersonID, tag_id, &tag_item) > 0) {
					if(tag_item.TagDataType == OTTYP_NUMBER)
						result = tag_item.Val.RealVal;
					else if(tag_item.TagDataType == OTTYP_INT)
						result = (double)tag_item.Val.IntVal;
					else if(tag_item.TagDataType == OTTYP_BOOL)
						result = tag_item.Val.IntVal ? 1.0 : 0.0;
					else if(tag_item.TagDataType == OTTYP_STRING)
						result = tag_item.Val.PStr ? atof(tag_item.Val.PStr) : 0.0;
					else if(tag_item.TagDataType == OTTYP_DATE)
						result = (OleDate)tag_item.Val.DtVal;
				}
			}
			else {
				PPSetError(PPERR_TAGSYMBNFOUND, p_symb);
				if(!(P.Flags & Param::fSilent))
					Logger.LogLastError();
			}
		}
		ok = 1;
	}
	else if(sstreqi_ascii(pSymb, "evdate")) {
		if(P_CurEv) {
			LDATETIME dtm;
			OleDate od;
			dtm.Set(P_CurEv->Dt, P_CurEv->Tm);
			od = dtm;
			result = od.v;
			ok = 1;
		}
		else {
			PPSetError(PPERR_PRCSALEVONNONEVCHARGE, pSymb);
			if(!(P.Flags & Param::fSilent))
				Logger.LogLastError();
		}
	}
	else if(sstreqi_ascii(pSymb, "evpairdate")) {
		// PPObjPersonEvent PersonEventCore
		if(P_CurEv) {
			LDATETIME dtm;
			dtm.Set(P_CurEv->Dt, P_CurEv->Tm);

			PPObjPsnOpKind pok_obj;
			PPPsnOpKind pok_rec;
			if(pok_obj.Fetch(P_CurEv->OpID, &pok_rec) > 0) {
				if(pok_rec.PairOp) {
					int    r2 = 1;
					PersonEventTbl::Rec pair_rec;
					PersonEventCore::PairIdent pi;
					pi.PersonID = P_CurEv->PersonID;
					pi.ThisOpID = pok_rec.ID;
					pi.PairOpID = pok_rec.PairOp;
					pi.Dt    = P_CurEv->Dt;
					pi.OprNo = P_CurEv->OprNo;
					if(oneof2(pok_rec.PairType, POKPT_CLOSE, POKPT_NULLCLOSE)) {
						r2 = EvObj.P_Tbl->SearchPair(&pi, 0, &pair_rec);
						if(r2 > 0)
							dtm.Set(pair_rec.Dt, pair_rec.Tm);
					}
					else if(pok_rec.PairType == POKPT_OPEN) {
						r2 = EvObj.P_Tbl->SearchPair(&pi, 1, &pair_rec);
						if(r2 > 0)
							dtm.Set(pair_rec.Dt, pair_rec.Tm);
					}
				}
			}
			OleDate od;
			od = dtm;
			result = od.v;
			ok = 1;
		}
		else {
			PPSetError(PPERR_PRCSALEVONNONEVCHARGE, pSymb);
			if(!(P.Flags & Param::fSilent))
				Logger.LogLastError();
		}
	}
	else if(strnicmp(pSymb, "ev", 2) == 0) {
		const char * p_symb = pSymb+2;
		uint   pos = 0;
		if(EvSymbList.SearchByText(p_symb, 1, &pos)) {
			result = (double)EvSymbList.at(pos).Id;
			if(!(P.Flags & Param::fSilent) && P.Flags & Param::fVerbose) {
				Logger.Log(PPFormatT(PPTXT_LOG_PRCSALUUSEDPSNEV, &msg_buf, pSymb, result));
			}
			ok = 1;
		}
		else {
			PPSetError(PPERR_PEVKINDSYMBNFOUND, p_symb);
			if(!(P.Flags & Param::fSilent))
				Logger.LogLastError();
		}
	}
	else if(strnicmp(pSymb, "pr", 2) == 0) {
		const char * p_symb = pSymb+2;
		//
		// prLastMonth - последний месяц до периода начисления    =  -30.0 //
		// prLastQuart - последний квартал до периода начисления  =  -90.0 //
		// prLastYear  - последний год до периода начисления      = -360.0 //
		// prNextMonth - следующий за периодом начисления месяц   = -30.25
		// prNextQuart - следующий за периодом начисления квартал = -90.25
		// prNextYear  - следующий за периодом начисления год     = -360.25
		//
		// prThisMonth - с начала этого месяца                    =  -30.5 //
		// prThisQuart - с начала этого квартала                  =  -90.5 //
		// prThisYear  - с начала этого года                      = -360.5 //
		// prNominal   - период начисления                                        = 1.    //
		// prActual    - период начисления с поправкой на дату приема (уволнения) = 1.5   //
		//
		ok = 1;
		if(sstreqi_ascii(p_symb, "LastMonth"))
			result = -30.;
		else if(sstreqi_ascii(p_symb, "LastQuart"))
			result = -90.;
		else if(sstreqi_ascii(p_symb, "LastYear"))
			result = -360.;
		if(sstreqi_ascii(p_symb, "NextMonth"))
			result = -30.25;
		else if(sstreqi_ascii(p_symb, "NextQuart"))
			result = -90.25;
		else if(sstreqi_ascii(p_symb, "NextYear"))
			result = -360.25;
		else if(sstreqi_ascii(p_symb, "ThisMonth"))
			result = -30.5;
		else if(sstreqi_ascii(p_symb, "ThisQuart"))
			result = -90.5;
		else if(sstreqi_ascii(p_symb, "ThisYear"))
			result = -360.5;
		else if(sstreqi_ascii(p_symb, "Nominal"))
			result = 1.0;
		else if(sstreqi_ascii(p_symb, "Actual"))
			result = 1.5;
		else {
			ok = 0;
			PPSetError(PPERR_UNDEFSALPERIOD, pSymb);
			if(!(P.Flags & Param::fSilent))
				Logger.LogLastError();
		}
	}
	else if(AmtTypeList.SearchByText(pSymb, 1, &(pos = 0))) {
		PPID   amt_id = AmtTypeList.at(pos).Id;
		if(CurPostPack.Amounts.Get(amt_id, cur_id, P.NominalPeriod.low, &result) > 0) {
			if(!(P.Flags & Param::fSilent) && P.Flags & Param::fVerbose) {
				Logger.Log(PPFormatT(PPTXT_LOG_PRCSALUSEDPOSTAMT, &msg_buf, pSymb, result));
			}
		}
		else if(CurStaffPack.Amounts.Get(amt_id, cur_id, P.NominalPeriod.low, &result) > 0) {
			if(!(P.Flags & Param::fSilent) && P.Flags & Param::fVerbose) {
				Logger.Log(PPFormatT(PPTXT_LOG_PRCSALUSEDSTAFFAMT, &msg_buf, pSymb, result));
			}
		}
		else if(CurEmployerAmtList.Get(amt_id, cur_id, P.NominalPeriod.low, &result) > 0) {
			if(!(P.Flags & Param::fSilent) && P.Flags & Param::fVerbose) {
				Logger.Log(PPFormatT(PPTXT_LOG_PRCSALUSEDEMPLOYERAMT, &msg_buf, pSymb, result));
			}
		}
		else {
			if(CalList.SearchByText(pSymb, 1, &(pos = 0))) {
				result = (double)CalList.at(pos).Id;
			}
			else {
				if(!(P.Flags & Param::fSilent)) {
					Logger.Log(PPFormatT(PPTXT_LOG_PRCSALNOSTAFFAMTSYMB, &msg_buf, pSymb, CurStaffPack.Rec.Name));
				}
				result = 0.0;
			}
		}
		ok = 1;
	}
	else if(CalList.SearchByText(pSymb, 1, &(pos = 0))) {
		result = (double)CalList.at(pos).Id;
		ok = 1;
	}
	ASSIGN_PTR(pVal, result);
	return ok;
}

double SLAPI PrcssrSalary::StorePeriod(const DateRange & rPeriod)
{
	PeriodStack.push(rPeriod);
	return ((double)(PeriodStack.getPointer()-1) + 0.1);
}

int SLAPI PrcssrSalary::Helper_GetPeriod(double arg, const DateRange & rInitPeriod, DateRange & rPeriod)
{
	int    ok = 1;
	double ip;
	double fr = round(fabs(modf(arg, &ip)), 1);
	long   p = (long)ip;
	DateRange period = rInitPeriod;
	if(arg == 1.0)
		period = P.NominalPeriod;
	else if(arg == 1.5)
		period = P.ActualPeriod;
	else if(fr == 0.0) {
		LDATE td = encodedate(1, period.low.month(), period.low.year());
		if((p % 360) == 0) { // LastYear
			period.upp = plusdate(td, -1);
			plusperiod(&td, PRD_ANNUAL, p / 360, 0);
			period.low = td;
		}
		else if((p % 90) == 0) { // LastQuart
			period.upp = plusdate(td, -1);
			plusperiod(&td, PRD_QUART, p / 90, 0);
			period.low = td;
		}
		else if((p % 30) == 0) { // LastMonth
			period.upp = plusdate(td, -1);
			plusperiod(&td, PRD_MONTH, p / 30, 0);
			period.low = td;
		}
		else { // @error
			period.SetDate(encodedate(1, 1, 2200));
			ok = 0;
		}
	}
	else if(fr == 0.5) {
		LDATE td = encodedate(period.low.dayspermonth(), period.low.month(), period.low.year());
		if((p % 360) == 0) { // ThisYear
			period.low = encodedate(1, 1, period.low.year());
			period.upp = td;
		}
		else if((p % 90) == 0) { // ThisQuart
			period.low = encodedate(1, (period.low.month()-1) / 3 * 3 + 1, period.low.year());
			period.upp = td;
		}
		else if((p % 30) == 0) { // ThisMonth
			period.low = encodedate(1, period.low.month(), period.low.year());
			period.upp = td;
		}
		else { // @error
			period.SetDate(encodedate(1, 1, 2200));
			ok = 0;
		}
	}
	else if(fr == 0.25) {
		LDATE td = plusdate(encodedate(period.upp.dayspermonth(), period.upp.month(), period.upp.year()), 1);
		if((p % 360) == 0) { // NextYear
			period.low = td;
			plusperiod(&td, PRD_ANNUAL, labs(p / 360), 0);
			period.upp = td;
		}
		else if((p % 90) == 0) { // NextQuart
			period.low = td;
			plusperiod(&td, PRD_QUART, labs(p / 90), 0);
			period.upp = td;
		}
		else if((p % 30) == 0) { // NextMonth
			period.low = td;
			period.upp = encodedate(td.dayspermonth(), td.month(), td.year());
		}
		else { // @error
			period.SetDate(encodedate(1, 1, 2200));
			ok = 0;
		}
	}
	else if(fr == 0.1) {
		uint pos = (uint)p;
		if(pos < PeriodStack.getCount())
			period = *(DateRange *)PeriodStack.at(pos);
		else {
			// @error
			period.SetDate(encodedate(1, 1, 2200));
			ok = 0;
		}
	}
	rPeriod = period;
	return ok;
}

int SLAPI PrcssrSalary::GetArgPeriod(double arg, DateRange & rPeriod)
{
	return Helper_GetPeriod(arg, P.NominalPeriod, rPeriod);
}

int SLAPI PrcssrSalary::Helper_CalcShipment(PPID opID, const DateRange & rPeriod,
	int kind, const PPIDArray & rArList, const FuncDescr & rFc, double * pResult)
{
	int    ok = 1;
	double result = 0.0;
	SString fmt_buf, msg_buf;
	BillFilt filt;
	filt.OpID = opID;
	filt.Period = rPeriod;
	for(uint i = 0; i < rArList.getCount(); i++) {
		const PPID ar_id = rArList.get(i);
		if(kind == 0)
			filt.ObjectID = ar_id;
		else if(kind == 1)
			filt.Object2ID = ar_id;
		else if(kind == 2)
			filt.AgentID = ar_id;
		SETIFZ(P_BillView, new PPViewBill);
		BillViewItem item;
		THROW(P_BillView->Init_(&filt));
		for(P_BillView->InitIteration(PPViewBill::OrdByDefault); P_BillView->NextIteration(&item) > 0;) {
			double amt = 0.0;
			if(rFc.AmtID)
				BillObj->P_Tbl->GetAmount(item.ID, rFc.AmtID, 0, &amt);
			else
				amt = item.Amount;
			result += amt;
			if(!(P.Flags & Param::fSilent) && P.Flags & Param::fVerbose) {
				Logger.Log(PPFormatT(PPTXT_LOG_PRCSALTAKENBILLAMT, &msg_buf, amt, item.ID));
			}
		}
	}
	CATCHZOK
	ASSIGN_PTR(pResult, result);
	return ok;
}

int SLAPI PrcssrSalary::Helper_CalcPayment(PPID opID, const DateRange & rPeriod, int kind,
	const PPIDArray & rArList, const PPIDArray * pExtBillList, const FuncDescr & rFc, double * pResult)
{
	int    ok = 1;
	double result = 0.0;
	SString fmt_buf, msg_buf;
	for(uint i = 0; i < rArList.getCount(); i++) {
		const PPID ar_id = rArList.get(i);
		BillTbl::Rec bill_rec, paym_rec;
		for(DateIter di(&rPeriod); BillObj->P_Tbl->EnumByOpr(opID, &di, &paym_rec) > 0;) {
			int    r = -1;
			THROW(r = BillObj->P_Tbl->Search(paym_rec.LinkBillID, &bill_rec));
			if(r > 0) {
				if(kind == 0) {
					if(bill_rec.Object != ar_id)
						continue;
				}
				else if(kind == 1) {
					if(bill_rec.Object2 != ar_id)
						continue;
				}
				else if(kind == 2) {
					if(!pExtBillList || !pExtBillList->bsearch(bill_rec.ID))
						continue;
				}
				{
					double amt = 0.0;
					PPID   bill_id = 0;
					if(rFc.Flags & FuncDescr::fLink) {
						bill_id = bill_rec.ID;
						if(rFc.AmtID)
							BillObj->P_Tbl->GetAmount(bill_id, rFc.AmtID, 0, &amt);
						else
							amt = bill_rec.Amount;
					}
					else {
						bill_id = paym_rec.ID;
						if(rFc.AmtID)
							BillObj->P_Tbl->GetAmount(bill_id, rFc.AmtID, 0, &amt);
						else
							amt = paym_rec.Amount;
					}
					if(!(P.Flags & Param::fSilent) && P.Flags & Param::fVerbose) {
						Logger.Log(PPFormatT(PPTXT_LOG_PRCSALTAKENBILLAMT, &msg_buf, amt, bill_id));
					}
					result += amt;
				}
			}
			else {
				SString msg_buf;
				PPObjBill::MakeCodeString(&paym_rec, PPObjBill::mcsAddOpName, msg_buf);
				PPSetError(PPERR_ZEROLINKPAYM, msg_buf);
				Logger.LogLastError();
			}
		}
	}
	CATCHZOK
	ASSIGN_PTR(pResult, result);
	return ok;
}

int SLAPI PrcssrSalary::ParseAmtType(const char * pText, FuncDescr & rDescr)
{
	int    ok = 1;
	BillSymbCache * p_cache = GetDbLocalCachePtr <BillSymbCache> (PPOBJ_BILLSYMB);
	SString form;
	PPID   id = 0;
	int    r = p_cache->Fetch(pText, &id, form);
	if(r == 1) {
		rDescr.AmtID = id;
	}
	else {
		ok = PPSetError(PPERR_SALCHARGEFORM_WANTMATSYMB, pText);
	}
	return ok;
}

int SLAPI PrcssrSalary::ParseFunc(const char * pText)
{
	int    func_id = 0;
	FuncDescr descr;
	SString temp_buf;
	SStrScan scan(pText);
	if(scan.Skip().GetIdent(temp_buf)) {
		if(temp_buf.CmpNC("cday") == 0)
			descr.FuncId = PrcssrSalary::funcCalDay;
		else if(temp_buf.CmpNC("chour") == 0)
			descr.FuncId = PrcssrSalary::funcCalHour;
		//
		else if(temp_buf.CmpNC("diffd") == 0)
			descr.FuncId = PrcssrSalary::funcDiffDays;
		else if(temp_buf.CmpNC("diffm") == 0)
			descr.FuncId = PrcssrSalary::funcDiffMonths;
		else if(temp_buf.CmpNC("diffy") == 0)
			descr.FuncId = PrcssrSalary::funcDiffYears;
		//
		else if(temp_buf.CmpNC("charge") == 0)
			descr.FuncId = PrcssrSalary::funcCharge;
		else if(temp_buf.CmpNC("avgcharge") == 0)
			descr.FuncId = PrcssrSalary::funcAvgCharge;
		else if(temp_buf.CmpNC("bill") == 0)
			descr.FuncId = PrcssrSalary::funcBill;
		else if(temp_buf.CmpNC("paym") == 0)
			descr.FuncId = PrcssrSalary::funcPaym;
		else if(temp_buf.CmpNC("evcday") == 0)
			descr.FuncId = PrcssrSalary::funcEvCalDay;
		else if(temp_buf.CmpNC("evchour") == 0)
			descr.FuncId = PrcssrSalary::funcEvCalHour;
		else if(temp_buf.CmpNC("evcount") == 0)
			descr.FuncId = PrcssrSalary::funcEvCount;
		else if(temp_buf.CmpNC("thisevcday") == 0) {
			THROW_PP_S(P_CurEv, PPERR_PRCSALEVONNONEVCHARGE, temp_buf);
			descr.FuncId = PrcssrSalary::funcThisEvCalDay;
		}
		else if(temp_buf.CmpNC("thisevchour") == 0) {
			THROW_PP_S(P_CurEv, PPERR_PRCSALEVONNONEVCHARGE, temp_buf);
			descr.FuncId = PrcssrSalary::funcThisEvCalHour;
		}
		else if(temp_buf.CmpNC("LastMonth") == 0)
			descr.FuncId = PrcssrSalary::funcLastMonth;
		else if(temp_buf.CmpNC("LastQuart") == 0)
			descr.FuncId = PrcssrSalary::funcLastQuart;
		else if(temp_buf.CmpNC("LastYear") == 0)
			descr.FuncId = PrcssrSalary::funcLastYear;
		else if(temp_buf.CmpNC("NextMonth") == 0)
			descr.FuncId = PrcssrSalary::funcNextMonth;
		else if(temp_buf.CmpNC("NextQuart") == 0)
			descr.FuncId = PrcssrSalary::funcNextQuart;
		else if(temp_buf.CmpNC("NextYear") == 0)
			descr.FuncId = PrcssrSalary::funcNextYear;
		else if(temp_buf.CmpNC("ThisMonth") == 0)
			descr.FuncId = PrcssrSalary::funcThisMonth;
		else if(temp_buf.CmpNC("ThisQuart") == 0)
			descr.FuncId = PrcssrSalary::funcThisQuart;
		else if(temp_buf.CmpNC("ThisYear") == 0)
			descr.FuncId = PrcssrSalary::funcThisYear;
		else {
			CALLEXCEPT_PP_S(PPERR_UNDEFSALARYFUNC, pText);
		}
		if(scan.Skip()[0] == '.') {
			scan.Incr();
			THROW_PP_S(scan.GetIdent(temp_buf), PPERR_SALCHARGEFORM_WANTIDENT, pText);
			if(temp_buf.CmpNC("link") == 0) {
				descr.Flags |= FuncDescr::fLink;
				if(scan.Skip()[0] == '.') {
					scan.Incr();
					THROW_PP_S(scan.GetIdent(temp_buf), PPERR_SALCHARGEFORM_WANTIDENT, pText);
					THROW(ParseAmtType(temp_buf, descr));
				}
			}
			else {
				THROW(ParseAmtType(temp_buf, descr));
			}
		}
		for(uint i = 0; !func_id && i < FuncList.getCount(); i++) {
			if(FuncList.at(i).IsEqHdr(descr)) {
				func_id = FuncList.at(i).Id;
			}
		}
		if(!func_id) {
			descr.Id = ++LastFuncId;
			THROW_SL(FuncList.insert(&descr));
			func_id = descr.Id;
		}
	}
	CATCH
		func_id = 0;
	ENDCATCH
	return func_id;
}

int SLAPI PrcssrSalary::GetFunc(int id, FuncDescr * pDescr) const
{
	uint   pos = 0;
	if(FuncList.lsearch(&id, &pos, PTR_CMPFUNC(long))) {
		const FuncDescr & r_descr = FuncList.at(pos);
		ASSIGN_PTR(pDescr, r_descr);
		return id;
	}
	else
		return PPSetError(PPERR_SALARYCHARGEFORM_INVFUNCID);
}

int SLAPI PrcssrSalary::IsFunc(const char * pSymb, int * pFuncId)
{
	int    func_id = ParseFunc(pSymb);
	ASSIGN_PTR(pFuncId, func_id);
	return BIN(func_id);
}

int SLAPI PrcssrSalary::Expr_ResolveFunc(int funcId, uint argCount, double * pArgList, double * pResult)
{
	int    ok = 1;
	double result = 0.0;
	SString fmt_buf, msg_buf;
	ScObjAssoc sc_obj_assc;
	FuncDescr descr;
	THROW(GetFunc(funcId, &descr));
	if(oneof9(descr.FuncId, funcLastMonth, funcLastQuart, funcLastYear, funcThisMonth,
		funcThisQuart, funcThisYear, funcNextMonth, funcNextQuart, funcNextYear)) {
		LDATE  dt = ZERODATE;
		DateRange period;
		if(argCount > 0) {
			OleDate od;
			od.v = pArgList[0];
			dt = od;
		}
		else
			dt = P.NominalPeriod.low;
		period.SetDate(dt);
		if(checkdate(dt, 0)) {
			double _v = 0.0;
			switch(descr.FuncId) {
				case funcLastMonth: _v = -30.; break;
				case funcLastQuart: _v = -90.; break;
				case funcLastYear:  _v = -360.; break;
				case funcThisMonth: _v = -30.5; break;
				case funcThisQuart: _v = -90.5; break;
				case funcThisYear:  _v = -360.5; break;
				case funcNextMonth: _v = -30.25; break;
				case funcNextQuart: _v = -90.25; break;
				case funcNextYear:  _v = -360.25; break;
			}
			Helper_GetPeriod(_v, period, period);
			result = StorePeriod(period);
		}
		else {
			// @error
			ok = 0;
		}
	}
	if(oneof2(descr.FuncId, funcCalDay, funcCalHour)) {
		PPID   cal_id      = (argCount > 0) ? (PPID)pArgList[0] : 0;
		PPID   proj_cal_id = (argCount > 1) ? (PPID)pArgList[1] : 0;
		long   numdays = 0;
		double numhours = 0.0;
		const char * p_time_msr = "";
		int   inverse = 0;
		DateRange period = P.ActualPeriod;
		// @v6.2.4 {
		if(argCount > 2)
			GetArgPeriod(pArgList[2], period);
		else if(cal_id != PPSTCAL_STANDARD) {
			PPStaffCal sc_rec;
			THROW(CalObj.Fetch(cal_id, &sc_rec) > 0);
			if(sc_rec.Flags & PPStaffCal::fUseNominalPeriod)
				period = P.NominalPeriod;
		}
		// } @v6.2.4
		if(proj_cal_id < 0) {
			inverse = 1;
			proj_cal_id = labs(proj_cal_id);
		}
		THROW(CalObj.InitScObjAssoc(cal_id, proj_cal_id, &CurPostPack.Rec, &sc_obj_assc));
		THROW(CalObj.CalcPeriod(sc_obj_assc, period, inverse, &numdays, &numhours, 0));
		if(descr.FuncId == funcCalDay) {
			result = numdays;
			p_time_msr = "days";
		}
		else if(descr.FuncId == funcCalHour) {
			result = numhours;
			p_time_msr = "hours";
		}
		if(!(P.Flags & Param::fSilent) && P.Flags & Param::fVerbose) {
			Logger.Log(PPFormatT(PPTXT_LOG_PRCSALTAKENCAL, &msg_buf, cal_id, period, p_time_msr, result));
		}
	}
	if(oneof4(descr.FuncId, funcEvCalDay, funcEvCalHour, funcThisEvCalDay, funcThisEvCalHour)) {
		PPID   pok_id = 0;
		PPID   proj_cal_id = 0;
		int    inverse = 0;
		PPIDArray ev_list;
		DateRange period = P.ActualPeriod;
		if(oneof2(descr.FuncId, funcEvCalDay, funcEvCalHour)) {
			pok_id = (argCount > 0) ? (PPID)pArgList[0] : 0;
			proj_cal_id = (argCount > 1) ? (PPID)pArgList[1] : 0;
			if(argCount > 2)
				GetArgPeriod(pArgList[2], period);
		}
		else {
			if(P_CurEv) {
				pok_id = P_CurEv->OpID;
				period.SetDate(P_CurEv->Dt);
				ev_list.add(P_CurEv->ID);
			}
			proj_cal_id = (argCount > 0) ? (PPID)pArgList[0] : 0;
		}
		if(proj_cal_id < 0) {
			inverse = 1;
			proj_cal_id = labs(proj_cal_id);
		}
		THROW(CalObj.InitScObjAssoc(0, proj_cal_id, &CurPostPack.Rec, &sc_obj_assc));
		{
			long   numdays = 0;
			double numhours = 0.0;
			const char * p_time_msr = "";
			if(oneof2(descr.FuncId, funcEvCalDay, funcEvCalHour)) {
				PersonEventFilt pev_filt;
				PersonEventViewItem pev_item;
				SETIFZ(P_EvView, new PPViewPersonEvent);
				THROW_MEM(P_EvView);
				pev_filt.Period = period;
				pev_filt.PrmrID = CurPostPack.Rec.PersonID;
				pev_filt.PsnOpList.Add(pok_id);
				THROW(P_EvView->Init_(&pev_filt));
				for(P_EvView->InitIteration(); P_EvView->NextIteration(&pev_item) > 0;) {
					ev_list.addUnique(pev_item.ID);
				}
			}
			THROW(CalObj.CalcPeriodByPersonEvent(sc_obj_assc, ev_list, inverse, &numdays, &numhours, 0));
			if(oneof2(descr.FuncId, funcEvCalDay, funcThisEvCalDay)) {
				result = numdays;
				p_time_msr = "days";
			}
			else if(oneof2(descr.FuncId, funcEvCalHour, funcThisEvCalHour)) {
				result = numhours;
				p_time_msr = "hours";
			}
			if(!(P.Flags & Param::fSilent) && P.Flags & Param::fVerbose) {
				Logger.Log(PPFormatT(PPTXT_LOG_PRCSALTAKENEVCAL, &msg_buf, pok_id, period, p_time_msr, result));
			}
		}
	}
	else if(oneof3(descr.FuncId, funcDiffDays, funcDiffMonths, funcDiffYears)) {
		LDATE  upp = ZERODATE;
		LDATE  low = ZERODATE;
		DateRange temp_period;
		if(argCount > 0) {
			if(GetArgPeriod(pArgList[0], temp_period))
				low = temp_period.low;
			else
				low = pArgList[0]; // as OleDate
			if(argCount > 1) {
				if(GetArgPeriod(pArgList[1], temp_period))
					upp = temp_period.low;
				else
					upp = pArgList[1]; // as OleDate
			}
			else
				upp = P.NominalPeriod.low;
			if(!checkdate(low, 0) || !checkdate(upp, 0)) {
				PPLoadText(PPTXT_LOG_PRCSALINVDATE, fmt_buf);
				Logger.Log(msg_buf.Printf(fmt_buf, "diffx(date[, date])"));
			}
			else {
				long d = 0;
				if(oneof2(descr.FuncId, funcDiffMonths, funcDiffYears)) {
					int    y1 = low.year();
					int    m1 = low.month();
					int    y2 = upp.year();

					int    dy = upp.year() - low.year();
					int    dm = upp.month() - low.month();

					d = dy * 12;
					if(descr.FuncId == funcDiffMonths)
						d += dm;
				}
				else
					d = diffdate(upp, low);
				result = d;
			}
		}
		else {
			PPLoadText(PPTXT_LOG_PRCSALINVARGC, fmt_buf);
			Logger.Log(msg_buf.Printf(fmt_buf, "diffx(date[, date])"));
		}
	}
	else if(descr.FuncId == funcEvCount) {
		PPID   pok_id = (argCount > 0) ? (PPID)pArgList[0] : 0;
		DateRange period = P.ActualPeriod;
		if(argCount > 1)
			GetArgPeriod(pArgList[1], period);
		{
			long   c = 0;
			PersonEventFilt pev_filt;
			PersonEventViewItem pev_item;
			SETIFZ(P_EvView, new PPViewPersonEvent);
			THROW_MEM(P_EvView);
			pev_filt.Period = period;
			pev_filt.PrmrID = CurPostPack.Rec.PersonID;
			pev_filt.PsnOpList.Add(pok_id);
			THROW(P_EvView->Init_(&pev_filt));
			for(P_EvView->InitIteration(); P_EvView->NextIteration(&pev_item) > 0;) {
				c++;
			}
			result = (double)c;
			if(!(P.Flags & Param::fSilent) && P.Flags & Param::fVerbose) {
				Logger.Log(PPFormatT(PPTXT_LOG_PRCSALTAKENEVCNT, &msg_buf, pok_id, period, result));
			}
		}
	}
	else if(oneof2(descr.FuncId, funcCharge, funcAvgCharge)) {
		PPID   ck_id = (argCount > 0) ? (PPID)pArgList[0] : 0;
		DateRange period = P.NominalPeriod;
		if(argCount > 1)
			GetArgPeriod(pArgList[1], period);
		if(ck_id) {
			THROW(Tbl.Calc(CurPostPack.Rec.ID, ck_id, BIN(funcId == funcAvgCharge), period, &result));
			if(!(P.Flags & Param::fSilent) && P.Flags & Param::fVerbose) {
				Logger.Log(PPFormatT(PPTXT_LOG_PRCSALTAKENCHARGE, &msg_buf, result, ck_id, period));
			}
		}
	}
	else if(oneof2(descr.FuncId, funcBill, funcPaym)) {
		PPID   op_id = (argCount > 0) ? (PPID)pArgList[0] : 0;
		int    kind  = (argCount > 1) ? (int)pArgList[1] : 0;
		DateRange period = P.NominalPeriod;
		if(argCount > 2)
			GetArgPeriod(pArgList[2], period);
		if(op_id) {
			uint   i;
			PPID   psn_id = CurPostPack.Rec.PersonID;
			PPOprKind op_rec;
			//
			// 0 - то документы поднимаются по персоналии, как контрагенту документа
			// 1 - документы поднимаются по персоналии, как дополнительному объекту документа
			// 2 - документы поднимаются по персоналии, как агенту по документу
			//
			PPID   acs_id = 0;
			PPObjArticle ar_obj;
			PPIDArray ar_list, psn_list;
			PPID   c_acs_id = 0, c_acs2_id = 0;
			THROW(oneof3(kind, 0, 1, 2));
			THROW(GetOpData(op_id, &op_rec) > 0);
			GetOpCommonAccSheet(op_id, &c_acs_id, &c_acs2_id);
			if(kind == 0)
				acs_id = c_acs_id;
			else if(kind == 1)
				acs_id = c_acs2_id;
			else if(kind == 2)
				acs_id = GetAgentAccSheet();
			psn_list.add(psn_id);
			ar_obj.GetByPersonList(acs_id, &psn_list, &ar_list);
			if(ar_list.getCount()) {
				if(descr.FuncId == funcBill) {
					THROW(Helper_CalcShipment(op_id, period, kind, ar_list, descr, &result));
				}
				else if(descr.FuncId == funcPaym) {
					PPObjOprKind op_obj;
					PPIDArray op_list, paym_op_list, shipm_op_list, temp_list;
					PPIDArray ext_bill_list;
					int    is_ext_bill_list_inited = 0;
					if(op_rec.OpTypeID == PPOPT_GENERIC) {
						GetGenericOpList(op_id, &op_list);
					}
					else
						op_list.add(op_id);
					for(i = 0; i < op_list.getCount(); i++) {
						const PPID op2_id = op_list.get(i);
						PPOprKind op2_rec;
						if(GetOpData(op2_id, &op2_rec) > 0) {
							if(op2_rec.Flags & OPKF_NEEDPAYMENT) {
								temp_list.clear();
								op_obj.GetPaymentOpList(op2_id, &temp_list);
								paym_op_list.addUnique(&temp_list);
							}
							else
								shipm_op_list.addUnique(op2_id);
						}
					}
					for(i = 0; i < shipm_op_list.getCount(); i++) {
						double temp_val = 0.0;
						THROW(Helper_CalcShipment(shipm_op_list.get(i), period, kind, ar_list, descr, &temp_val));
						result += temp_val;
					}
					for(i = 0; i < paym_op_list.getCount(); i++) {
						double temp_val = 0.0;
						if(!is_ext_bill_list_inited && kind == 2) {
							THROW(BillObj->P_Tbl->GetBillListByExt(ar_list.get(0), 0L, ext_bill_list));
							// @v8.1.0 (сортировку теперь выполняет GetBillListByExt) ext_bill_list.sort();
							is_ext_bill_list_inited = 1;
						}
						THROW(Helper_CalcPayment(paym_op_list.get(i), period, kind, ar_list, &ext_bill_list, descr, &temp_val));
						result += temp_val;
					}
				}
			}
		}
	}
	CATCHZOK
	ASSIGN_PTR(pResult, result);
	return ok;
}

int SLAPI PrcssrSalary::ProcessPost(PPID postID, UintHashTable * pIdList, int use_ta)
{
	int    ok = -1;
	SString fmt_buf, msg_buf;
	PPID   charge_grp_id = 0;
	PPSalChargePacket sc_grp_pack, sc_pack;
	SalaryContext expr_ctx(this);
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(SlObj.GetPostPacket(postID, &CurPostPack) > 0);
		P.ActualPeriod = P.NominalPeriod;
		SETMAX(P.ActualPeriod.low, CurPostPack.Rec.Dt);
		if(CurPostPack.Rec.Closed > 0 && CurPostPack.Rec.Finish)
			SETMIN(P.ActualPeriod.upp, CurPostPack.Rec.Finish);
		if(P.NominalPeriod.low <= P.NominalPeriod.upp && P.ActualPeriod.low <= P.ActualPeriod.upp) {
			THROW(SlObj.GetPacket(CurPostPack.Rec.StaffID, &CurStaffPack) > 0);
			CurEmployerAmtList.freeAll();
			if(CurStaffPack.Rec.OrgID)
				THROW(SlObj.PsnObj.GetStaffAmtList(CurStaffPack.Rec.OrgID, &CurEmployerAmtList));
			if(!(P.Flags & Param::fSilent)) {
				THROW(PPLoadText(PPTXT_LOG_PRCSALCHARGEBYPOST, fmt_buf));
				Logger.Log(PPFormat(fmt_buf, &msg_buf, CurPostPack.Rec.PersonID, CurStaffPack.Rec.Name, P.NominalPeriod));
			}
			charge_grp_id = NZOR(CurPostPack.Rec.ChargeGrpID, CurStaffPack.Rec.ChargeGrpID);
			if(!charge_grp_id) {
				PersonTbl::Rec psn_rec;
				msg_buf = 0;
				if(SlObj.PsnObj.Fetch(CurPostPack.Rec.PersonID, &psn_rec) > 0)
					msg_buf.Cat(psn_rec.Name);
				else
					ideqvalstr(CurPostPack.Rec.PersonID, msg_buf);
				msg_buf.CatDiv('-', 1).Cat(CurStaffPack.Rec.Name);
				CALLEXCEPT_PP_S(PPERR_UNDEFPOSTSALCHARGEGRP, msg_buf);
			}
			THROW(ScObj.GetPacket(charge_grp_id, &sc_grp_pack) > 0);
			for(uint i = 0; i < sc_grp_pack.GrpList.getCount(); i++) {
				const PPID charge_id = sc_grp_pack.GrpList.at(i);
				THROW(ScObj.Fetch(charge_id, &sc_pack) > 0);
				if(sc_pack.Formula.NotEmptyS()) {
					if(sc_pack.Rec.EnumObjType == PPOBJ_PERSONEVENT) {
						if(sc_pack.Rec.EnumExtVal) {
							PersonEventFilt pev_filt;
							PersonEventViewItem pev_item; //PersonEventTbl
							PPViewPersonEvent pev_view;
							//
							// Предварительно удаляем записи по паре {postID, charge_id}
							// с датой начала, равной расчетной дате начала.
							//
							THROW_DB(deleteFrom(&Tbl, 0, (Tbl.PostID == postID &&
								Tbl.SalChargeID == charge_id && Tbl.Beg == P.NominalPeriod.low)));
							//
							pev_filt.Period = P.NominalPeriod;
							pev_filt.PrmrID = CurPostPack.Rec.PersonID;
							pev_filt.PsnOpList.Add(sc_pack.Rec.EnumExtVal);
							THROW(pev_view.Init_(&pev_filt));
							for(pev_view.InitIteration(); pev_view.NextIteration(&pev_item) > 0;) {
								P_CurEv = &pev_item;
								double value = 0.0;
								PPID   sal_id = 0;
								SalaryTbl::Rec sal_entry;
								PPExprParser::CalcExpression(sc_pack.Formula, &value, 0, &expr_ctx);
								MEMSZERO(sal_entry);
								sal_entry.Beg = P.NominalPeriod.low;
								sal_entry.End = P.NominalPeriod.upp;
								sal_entry.PostID = postID;
								sal_entry.SalChargeID = charge_id;
								sal_entry.ExtObjID = pev_item.ID;
								sal_entry.Amount = value;
								THROW(Tbl.Put(&sal_id, &sal_entry, 0));
								if(pIdList)
									pIdList->Add(sal_id);
							}
							P_CurEv = 0;
						}
					}
					else {
						double value = 0.0;
						PPID   sal_id = 0;
						SalaryTbl::Rec sal_entry;
						P_CurEv = 0;
						PPExprParser::CalcExpression(sc_pack.Formula, &value, 0, &expr_ctx);
						MEMSZERO(sal_entry);
						sal_entry.Beg = P.NominalPeriod.low;
						sal_entry.End = P.NominalPeriod.upp;
						sal_entry.PostID = postID;
						sal_entry.SalChargeID = charge_id;
						sal_entry.Amount = value;
						THROW(Tbl.Put(&sal_id, &sal_entry, 0));
						if(pIdList)
							pIdList->Add(sal_id);
					}
				}
				else if(!(P.Flags & Param::fSilent)) {
					THROW(PPLoadText(PPTXT_LOG_SALCHARGEFORMULAEMPTY, fmt_buf));
					Logger.Log(PPFormat(fmt_buf, &msg_buf, sc_pack.Formula.cptr()));
				}
			}
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	P_CurEv = 0;
	return ok;
}

int SLAPI PrcssrSalary::CalcPeriod(const CalcPeriodParam & rCpP, long * pDays, double * pHours)
{
	int    ok = 1;
	long   numdays  = 0;
	double numhours = 0.0;
	ScObjAssoc sc_obj_assc;
	SString temp_buf;
	SStrCollection dbg_log_list;
	if(!rCpP.Period.low || !rCpP.Period.upp || rCpP.Period.low > rCpP.Period.upp)
		ok = PPSetErrorInvParam();
	else {
		PersonPostTbl::Rec post_rec;
		PPStaffCal parent_cal, proj_parent_cal;
		PPID   parent_id = 0, proj_parent_id = 0;
		THROW(CalObj.SearchBySymb(rCpP.CalSymb, &parent_id, &parent_cal) > 0);
		if(rCpP.ProjCalSymb.NotEmpty()) {
			THROW(CalObj.SearchBySymb(rCpP.ProjCalSymb, &proj_parent_id, &proj_parent_cal) > 0);
		}
		THROW(SlObj.SearchPost(rCpP.PostID, &post_rec) > 0);
		THROW(CalObj.InitScObjAssoc(parent_id, proj_parent_id, &post_rec, &sc_obj_assc));
		THROW(CalObj.CalcPeriod(sc_obj_assc, rCpP.Period, 0, &numdays, &numhours, 0));
	}
	CATCHZOK
	ASSIGN_PTR(pDays, numdays);
	ASSIGN_PTR(pHours, numhours);
	return ok;
}

int SLAPI PrcssrSalary::TestCalcPeriod(PPID postID)
{
	class TestStaffCalDialog : public TDialog {
	public:
		TestStaffCalDialog(PrcssrSalary * pPrc) : TDialog(DLG_TESTSTAFFCAL)
		{
			P_Prc = pPrc;
			SetupCalPeriod(CTLCAL_TESTSTAFFCAL_PERIOD, CTL_TESTSTAFFCAL_PERIOD);
		}
		int    setDTS(const PrcssrSalary::CalcPeriodParam * pData)
		{
			Data = *pData;
			SString temp_buf;
			PersonPostTbl::Rec post_rec;
			if(P_Prc->SlObj.SearchPost(Data.PostID, &post_rec) > 0)
				P_Prc->SlObj.MakeCodeString(&post_rec, temp_buf);
			setCtrlString(CTL_TESTSTAFFCAL_POST, temp_buf);
			setCtrlString(CTL_TESTSTAFFCAL_CSYM, Data.CalSymb);
			setCtrlString(CTL_TESTSTAFFCAL_PCSYM, Data.ProjCalSymb);
			SetPeriodInput(this, CTL_TESTSTAFFCAL_PERIOD, &Data.Period);
			return 1;
		}
		int    getDTS(PrcssrSalary::CalcPeriodParam * pData)
		{
			getCtrlString(CTL_TESTSTAFFCAL_CSYM, Data.CalSymb);
			getCtrlString(CTL_TESTSTAFFCAL_PCSYM, Data.ProjCalSymb);
			GetPeriodInput(this, CTL_TESTSTAFFCAL_PERIOD, &Data.Period);
			ASSIGN_PTR(pData, Data);
			return 1;
		}
	private:
		PrcssrSalary * P_Prc;
		PrcssrSalary::CalcPeriodParam Data;
	};
	int    ok = 1;
	TestStaffCalDialog * dlg = new TestStaffCalDialog(this);
	if(CheckDialogPtrErr(&dlg)) {
		SString temp_buf;
		CalcPeriodParam param;
		MEMSZERO(param);
		param.PostID = postID;
		dlg->setDTS(&param);
		while(ExecView(dlg) == cmOK) {
			dlg->getDTS(&param);
			long   num_days = 0;
			double dur = 0.0;
			if(CalcPeriod(param, &num_days, &dur)) {
				dlg->setCtrlLong(CTL_TESTSTAFFCAL_DAYS,  num_days);
				dlg->setCtrlReal(CTL_TESTSTAFFCAL_HOURS, dur);
				temp_buf = "OK";
			}
			else {
				dlg->setCtrlLong(CTL_TESTSTAFFCAL_DAYS, 0);
				dlg->setCtrlReal(CTL_TESTSTAFFCAL_HOURS, 0.0);
				PPGetLastErrorMessage(1, temp_buf);
			}
			dlg->setStaticText(CTL_TESTSTAFFCAL_ST_MSG, temp_buf);
		}
	}
	else
		ok = 0;
	delete dlg;
	return ok;
}

int SLAPI DoChargeSalary()
{
	int    ok = -1;
	PrcssrSalary prcssr;
	PrcssrSalary::Param param;
	prcssr.InitParam(&param);
	if(prcssr.EditParam(&param) > 0)
		if(prcssr.Init(&param) && prcssr.Run())
			ok = 1;
		else
			ok = PPErrorZ();
	return ok;
}
//
// Implementation of PPALDD_Salary
//
PPALDD_CONSTRUCTOR(Salary)
{
	InitFixData(rscDefHdr, &H, sizeof(H));
	InitFixData(rscDefIter, &I, sizeof(I));
}

PPALDD_DESTRUCTOR(Salary)
{
	Destroy();
}

int PPALDD_Salary::InitData(PPFilt & rFilt, long rsrv)
{
	INIT_PPVIEW_ALDD_DATA_U(Salary, rsrv);
	H.FltBeg         = p_filt->Period.low;
	H.FltEnd         = p_filt->Period.upp;
	H.FltPostID      = p_filt->PostID;
	H.FltSalChargeID = p_filt->SalChargeID;
	H.FltOrgID       = p_filt->OrgID;
	H.FltDivID       = p_filt->DivID;
	H.FltStaffID     = p_filt->StaffID;
	H.FltPersonID    = p_filt->PersonID;
	H.FltFlags       = p_filt->Flags;
	return DlRtm::InitData(rFilt, rsrv);
}

int PPALDD_Salary::InitIteration(long iterId, int sortId, long rsrv)
{
	INIT_PPVIEW_ALDD_ITER_ORD(Salary, sortId);
}

int PPALDD_Salary::NextIteration(long iterId, long rsrv)
{
	START_PPVIEW_ALDD_ITER(Salary);
	I.ID          = item.ID;
	I.Beg         = item.Beg;
	I.End         = item.End;
	I.PostID      = item.PostID;
	I.SalChargeID = item.SalChargeID;
	I.Amount      = item.Amount;
	I.Flags       = item.Flags;
	I.LinkBillID  = item.LinkBillID;
	I.GenBillID   = item.GenBillID;
	I.RByGenBill  = item.RByGenBill;
	{
		SString buf;
		p_v->GetSalChargeName(item.SalChargeID, buf);
		buf.CopyTo(I.ChargeName, sizeof(I.ChargeName));
	}
	FINISH_PPVIEW_ALDD_ITER();
}

void PPALDD_Salary::Destroy()
{
	DESTROY_PPVIEW_ALDD(Salary);
}
//
// Implementation of PPALDD_SalaryByPost
//
int SLAPI PPViewSalary::InitPostDateList()
{
	Post_Date_List.freeAll();
	return 1;
}

int SLAPI PPViewSalary::SetPostDateListItem(PPID postID, LDATE dt)
{
	if(Post_Date_List.SearchPair(postID, dt.v, 0))
		return -1;
	else {
		Post_Date_List.Add(postID, dt.v, 0);
		return 1;
	}
}

PPALDD_CONSTRUCTOR(SalaryByPost)
{
	InitFixData(rscDefHdr, &H, sizeof(H));
	InitFixData(rscDefIter, &I, sizeof(I));
}

PPALDD_DESTRUCTOR(SalaryByPost)
{
	Destroy();
}

int PPALDD_SalaryByPost::InitData(PPFilt & rFilt, long rsrv)
{
	INIT_PPVIEW_ALDD_DATA_U(Salary, rsrv);
	H.FltBeg         = p_filt->Period.low;
	H.FltEnd         = p_filt->Period.upp;
	H.FltPostID      = p_filt->PostID;
	H.FltSalChargeID = p_filt->SalChargeID;
	H.FltOrgID       = p_filt->OrgID;
	H.FltDivID       = p_filt->DivID;
	H.FltStaffID     = p_filt->StaffID;
	H.FltPersonID    = p_filt->PersonID;
	H.FltFlags       = p_filt->Flags;
	return DlRtm::InitData(rFilt, rsrv);
}

int PPALDD_SalaryByPost::InitIteration(long iterId, int sortId, long rsrv)
{
	PPViewSalary * p_v = (PPViewSalary *)(Extra[1].Ptr ? Extra[1].Ptr : Extra[0].Ptr);
	IterProlog(iterId, 1);
	if(sortId >= 0)
		SortIdx = sortId;
	p_v->InitIteration(sortId);
	p_v->InitPostDateList();
	return 1;

	//INIT_PPVIEW_ALDD_ITER_ORD(Salary, sortId);
}

int PPALDD_SalaryByPost::NextIteration(long iterId, long rsrv)
{
	int    r = 0;
	IterProlog(iterId, 0);
	PPViewSalary * p_v = (PPViewSalary *)(Extra[1].Ptr ? Extra[1].Ptr : Extra[0].Ptr);
	SalaryViewItem item;
	while((r = p_v->NextIteration(&item)) > 0) {
		if(p_v->SetPostDateListItem(item.PostID, item.Beg) > 0) {
		//if(I.PostID != item.PostID) {
			I.ID          = item.ID;
			I.Beg         = item.Beg;
			I.End         = item.End;
			I.PostID      = item.PostID;
			break;
		}
	}
	if(r <= 0)
		return -1;
	FINISH_PPVIEW_ALDD_ITER();
}

int PPALDD_SalaryByPost::EvaluateFunc(const DlFunc * pF, SV_Uint32 * pApl, RtmStack & rS)
{
	#define _ARG_STR(n)  (**(SString **)rS.GetPtr(pApl->Get(n)))
	#define _RET_DBL     (*(double *)rS.GetPtr(pApl->Get(0)))

	if(pF->Name.Cmp("?GetAmount", 0) == 0) {
		double amt = 0.0;
		PPViewSalary * p_v = (PPViewSalary *)(Extra[1].Ptr ? Extra[1].Ptr : Extra[0].Ptr);
		if(p_v) {
			PPID   charge_id = 0;
			PPObjSalCharge charge_obj;
			if(charge_obj.SearchBySymb(_ARG_STR(1), &charge_id) > 0) {
				const SalaryFilt * p_filt = (SalaryFilt*)p_v->GetBaseFilt();
				DateRange period;
				period.Set(I.Beg, I.End);
				p_v->Calc(I.PostID, charge_id, 0, period, &amt);
			}
		}
		_RET_DBL = amt;
	}
	return 1;
}

void PPALDD_SalaryByPost::Destroy()
{
	DESTROY_PPVIEW_ALDD(Salary);
}
