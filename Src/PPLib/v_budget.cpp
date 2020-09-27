// V_BUDGET.CPP
// Copyright (c) A.Starodub 2010, 2011, 2014, 2015, 2016, 2017, 2018, 2019, 2020
// @codepage UTF-8
// PPViewBudget
//
#include <pp.h>
#pragma hdrstop
#include <charry.h>

SLAPI PPBudgetPacket::PPBudgetPacket()
{
	// @v10.7.9 @ctr Init();
}

void SLAPI PPBudgetPacket::Init()
{
	MEMSZERO(Rec);
	Items.freeAll();
	ScenList.freeAll();
}

int SLAPI PPBudgetPacket::PutItems(const BudgetItemsList * pList)
{
	Items.freeAll();
	if(pList)
		Items.copy(*pList);
	return 1;
}

int SLAPI PPBudgetPacket::EnumItems(uint * pIdx, BudgetItemTbl::Rec * pRec)
{
	int    ok = -1;
	if(pIdx && *pIdx < Items.getCount()) {
		ASSIGN_PTR(pRec, Items.at(*pIdx));
		(*pIdx)++;
		ok = 1;
	}
	return ok;
}

int SLAPI PPBudgetPacket::AddItem(BudgetItemTbl::Rec * pRec)
{
	int    ok = -1;
	if(pRec && Items.lsearch(&pRec->ID, 0, PTR_CMPFUNC(long)) <= 0)
		ok = Items.insert(pRec);
	return ok;
}

int SLAPI PPBudgetPacket::UpdateItem(uint pos, BudgetItemTbl::Rec * pRec)
{
	int    ok = -1;
	if(pRec && pos < Items.getCount()) {
		Items.at(pos) = *pRec;
		ok = 1;
	}
	return ok;
}

int SLAPI PPBudgetPacket::DelItem(uint pos, PPID id)
{
	uint p = 0;
	if(id && Items.lsearch(&id, &p, PTR_CMPFUNC(long)) > 0)
		Items.atFree(p);
	else if(pos < Items.getCount())
		Items.atFree(pos);
	return 1;
}

PPBudgetPacket & FASTCALL PPBudgetPacket::operator = (const PPBudgetPacket & rSrc)
{
	//Items.freeAll();
	Rec = rSrc.Rec;
	Items.copy(rSrc.Items);
	ScenList.copy(rSrc.ScenList);
	return *this;
}

SLAPI PPBudget::PPBudget()
{
	THISZERO();
}

SLAPI BudgetItemCore::BudgetItemCore()
{
}

int SLAPI BudgetItemCore::Search(PPID id, void * pRec /*=0*/)
{
	return SearchByID(this, 0, id, pRec);
}

int SLAPI BudgetItemCore::Search(PPID budgetID, PPID acc, PPID kind, LDATE dt, void * pRec)
{
	int    ok = -1;
	BudgetItemTbl::Key1 k1;
	MEMSZERO(k1);
	k1.BudgetID = budgetID;
	k1.Acc      = acc;
	k1.Kind     = kind;
	k1.Dt       = dt;
	if(search(1, &k1, spEq) > 0) {
		copyBufTo(pRec);
		ok = 1;
	}
	return ok;
}

int SLAPI BudgetItemCore::PutItem_(PPID * pID, BudgetItemTbl::Rec * pRec, int useTa)
{
	int    ok = 1;
	if(pID) {
		PPID   tmp_id = 0;
		PPTransaction tra(useTa);
		THROW(tra);
		if(*pID && pRec == 0)
			deleteFrom(this, 0, ID == *pID);
		else if(*pID) {
			if(SearchByID_ForUpdate(this, 0, *pID, 0) > 0) {
				THROW_DB(this->updateRecBuf(pRec));
			}
			else {
				this->copyBufFrom(pRec);
				THROW_DB(this->insertRec(0, &tmp_id));
			}
		}
		else {
			this->copyBufFrom(pRec);
			THROW_DB(this->insertRec(0, &tmp_id));
		}
		if(tmp_id)
			ASSIGN_PTR(pID, tmp_id);
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI BudgetItemCore::PutItem(PPID * pID, BudgetItemTbl::Rec * pRec, int useTa)
{
	int    ok = 1; //, ta = 0;
	double prev_amt = 0.0;
	PPObjAccount obj_acct;
	if(pID) {
		int    del = BIN(*pID && pRec == 0);
		BudgetItemTbl::Rec prev_rec;
		// @v10.6.4 MEMSZERO(prev_rec);
		if(*pID)
			Search(*pID, &prev_rec);
		{
			PPTransaction tra(useTa);
			THROW(tra);
			THROW(PutItem_(pID, pRec, 0));
			if(del && prev_rec.ID) {
				StrAssocArray child_list;
				if(obj_acct.GetChildList(prev_rec.Acc, &child_list) > 0)
					for(uint i = 0; i < child_list.getCount(); i++)
						deleteFrom(this, 0, BudgetID == prev_rec.BudgetID && Kind == prev_rec.Kind && Dt == prev_rec.Dt && Acc == child_list.Get(i).Id);
			}
			{
				StrAssocArray acc_list;
				BudgetItemTbl::Rec * p_cur_rec = (pRec) ? pRec : &prev_rec;
				if(obj_acct.GetParentList(p_cur_rec->Acc, &acc_list) > 0) {
					for(uint i = 0; i < acc_list.getCount(); i++) {
						PPID   id = 0;
						PPID   parent_acc = acc_list.Get(i).Id;
						BudgetItemTbl::Rec parent_bi_rec;
						// @v10.6.4 MEMSZERO(parent_bi_rec);
						if(Search(p_cur_rec->BudgetID, parent_acc, p_cur_rec->Kind, p_cur_rec->Dt, &parent_bi_rec) > 0) {
							double new_amt = parent_bi_rec.Amount - prev_rec.Amount;
							parent_bi_rec.Amount = (pRec) ? new_amt + pRec->Amount : new_amt;
						}
						else {
							parent_bi_rec     = *p_cur_rec;
							parent_bi_rec.ID  = 0;
							parent_bi_rec.Acc = parent_acc;
						}
						THROW(PutItem_(&(id = parent_bi_rec.ID), &parent_bi_rec, 0));
					}
				}
			}
			THROW(tra.Commit());
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI BudgetItemCore::GetItemsByBudget(PPID budgetID, PPID accID, long kind, BudgetItemsList * pItems)
{
	int    ok = 1;
	DBQ * dbq = 0;
	BExtQuery q(this, 1);
	BudgetItemTbl::Key1 k1;

	THROW_INVARG(pItems);
	pItems->freeAll();
	MEMSZERO(k1);
	k1.BudgetID = budgetID;
	k1.Acc      = accID;
	q.select(ID, BudgetID, Acc, Flags, Kind, Dt, Amount, Memo, 0L);
	dbq = ppcheckfiltid(dbq, BudgetID, budgetID);
	dbq = ppcheckfiltid(dbq, Acc, accID);
	q.where(*dbq);
	for(q.initIteration(0, &k1, spGe); q.nextIteration() > 0;)
		if(kind == -1 || data.Kind == kind)
			pItems->insert(&data);
	CATCHZOK
	return ok;
}

int SLAPI BudgetItemCore::PutItems(BudgetItemsList * pItems, int useTa)
{
	int ta = 0, ok = -1;
	if(pItems) {
		THROW(PPStartTransaction(&ta, useTa));
		for(uint i = 0; i < pItems->getCount();i++) {
			PPID id = 0;
			BudgetItemTbl::Rec rec = pItems->at(i);
			if(rec.Amount || rec.ID) {
				id = rec.ID;
				THROW_PP(rec.ID || Search(rec.BudgetID, rec.Acc, rec.Kind, rec.Dt, 0) <= 0, PPERR_BUDGITEMEXISTS);
				THROW(PutItem(&id, (rec.Amount) ? &rec : 0, 0));
				pItems->at(i).ID = id;
				ok = 1;
			}
		}
		THROW(PPCommitWork(&ta));
		ok = 1;
	}
	CATCH
		PPRollbackWork(&ta);
		ok = 0;
	ENDCATCH
	return ok;
}

SLAPI PPObjBudget::PPObjBudget(void * extraPtr) : PPObjReference(PPOBJ_BUDGET, extraPtr)
{
	ImplementFlags |= (implStrAssocMakeList | implTreeSelector);
	PPLoadText(PPTXT_WEEKS,    StrWeeks);
	PPLoadText(PPTXT_MONTHES,  StrMonthes);
	PPLoadText(PPTXT_QUARTS,   StrQuarts);
	PPLoadText(PPTXT_SEMIYEAR, StrSemiYear);
}

SLAPI PPObjBudget::~PPObjBudget()
{
}

int SLAPI PPObjBudget::GetPacket(PPID id, PPBudgetPacket * pPack)
{
	int    ok = -1;
	THROW_INVARG(pPack);
	pPack->Init();
	if(Search(id, &pPack->Rec) > 0) {
		PPIDArray scen_list;
		//
		// Извлекаем сценарии
		//
		{
			SString buf;
			PPBudget rec = pPack->Rec;
			PPGetWord(PPWORD_BASE, 0, rec.Name, sizeof(rec.Name));
			pPack->ScenList.insert(&rec);
		}
		if(GetChildBudgets(pPack->Rec.ID, &scen_list) > 0)
			for(uint i = 0; i < scen_list.getCount(); i++) {
				PPBudget rec;
				// @v10.7.9 @ctr MEMSZERO(rec);
				if(Search(scen_list.at(i), &rec) > 0)
					pPack->ScenList.insert(&rec);
			}
		//
		// Извлекаем список доходных/расходных статей
		//
		{
			BudgetItemsList items_list;
			THROW(ItemsTbl.GetItemsByBudget(pPack->Rec.ID, 0, -1, &items_list));
			pPack->PutItems(&items_list);
			ok = 1;
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjBudget::PutRec(PPID * pID, PPBudget * pRec, int use_ta)
{
	int    ok = 1;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(*pID && pRec == 0) {
			THROW(ref->RemoveItem(PPOBJ_BUDGET, *pID, 0));
		}
		else if(pRec) {
			pRec->ObjType = PPOBJ_BUDGET;
			THROW(EditItem(PPOBJ_BUDGET, *pID, pRec, 0));
			*pID = pRec->ID = ref->data.ObjID;
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjBudget::PutPacket(PPID * pID, PPBudgetPacket * pPack, int use_ta)
{
	int    ok = 1;
	PPBudgetPacket prev_pack;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(pID && *pID) {
			if(!RVALUEPTR(prev_pack, pPack))
				GetPacket(*pID, &prev_pack);
		}
		//
		// Модифицируем запись бюджета
		//
		THROW(PutRec(pID, (pPack) ? &pPack->Rec : 0, 0));
		//
		// Модифицируем записи сценариев
		//
		for(uint i = 0; i < prev_pack.ScenList.getCount(); i++) {
			PPBudget & r_scen_rec = prev_pack.ScenList.at(i);
			if(i != 0) {
				r_scen_rec.ParentID = prev_pack.Rec.ID;
				THROW(PutRec(&r_scen_rec.ID, (pPack) ? &r_scen_rec : 0, 0));
			}
		}
		//
		// Модифицируем записи доходных/расходных статей
		//
		deleteFrom(&ItemsTbl, 0, ItemsTbl.BudgetID == *pID);
		if(*pID && pPack != 0) {
			PPID item_id = 0;
			BudgetItemTbl::Rec item;
			// @v10.7.9 @ctr MEMSZERO(item);
			for(uint i = 0; pPack->EnumItems(&i, &item) > 0;) {
				item.BudgetID = *pID;
				THROW(ItemsTbl.PutItem_(&item.ID, &item, 0));
				pPack->UpdateItem(i-1, &item);
			}
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjBudget::GetChildBudgets(PPID parentID, PPIDArray * pChildList)
{
	int    ok = -1;
	long h = 0;
	PPBudget budget;
	for(ref->InitEnum(PPOBJ_BUDGET, 0, &h); ref->NextEnum(h, &budget) > 0;) {
		if(budget.ParentID == parentID) {
			ok = 1;
			CALLPTRMEMB(pChildList, add(budget.ID));
		}
	}
	return ok;
}

class BudgetScenDialog : public PPListDialog {
public:
	BudgetScenDialog() : PPListDialog(DLG_LBXSEL, CTL_LBXSEL_LIST)
	{
		PPLoadString(PPSTR_TEXT, PPTXT_BUDGSCEN, BudgTitle);
		setTitle(BudgTitle);
	}
	int setDTS(const PPBudgetPacket *);
	int getDTS(PPBudgetPacket *);
private:
	virtual int addItem(long *, long *);
	virtual int delItem(long, long);
	virtual int editItem(long, long);
	virtual int setupList();

	SString BudgTitle;
	PPBudgetPacket Data;
};

int BudgetScenDialog::setDTS(const PPBudgetPacket * pData)
{
	if(!RVALUEPTR(Data, pData))
		Data.Init();
	updateList(-1);
	return 1;
}

int BudgetScenDialog::getDTS(PPBudgetPacket * pData)
{
	ASSIGN_PTR(pData, Data);
	return 1;
}

int BudgetScenDialog::addItem(long * pPos, long * pID)
{
	int    ok = -1;
	SString name;
	PPInputStringDialogParam isd_param(BudgTitle, BudgTitle);
	if(InputStringDialog(&isd_param, name) > 0) {
		PPBudget rec;
		// @v10.7.9 @ctr MEMSZERO(rec);
		rec    = Data.Rec;
		rec.ID = 0;
		name.CopyTo(rec.Name, sizeof(rec.Name));
		ok = Data.ScenList.insert(&rec);
	}
	return ok;
}

int BudgetScenDialog::editItem(long pos, long id)
{
	int    ok = -1;
	if(pos > 0 && pos < (long)Data.ScenList.getCount()) {
		SString name;
		name = Data.ScenList.at(pos).Name;
		PPInputStringDialogParam isd_param(BudgTitle, BudgTitle);
		if(InputStringDialog(&isd_param, name) > 0) {
			STRNSCPY(Data.ScenList.at(pos).Name, name);
			ok = 1;
		}
	}
	return ok;
}

int BudgetScenDialog::delItem(long pos, long id)
{
	int    ok = -1;
	if(pos > 0 && pos < (long)Data.ScenList.getCount() && CONFIRM(PPCFM_DELSCEN))
		ok = Data.ScenList.atFree(pos);
	return ok;
}

int BudgetScenDialog::setupList()
{
	int    ok = -1;
	for(uint i = 0; i < Data.ScenList.getCount();i++)
		if(!addStringToList(i + 1, Data.ScenList.at(i).Name))
			ok = PPErrorZ();
	return ok;
}

#define GRP_CYCLE 1L

class BudgetDialog : public TDialog {
public:
	BudgetDialog() : TDialog(DLG_BUDGET)
	{
		CycleCtrlGroup * p_cycle_grp = new CycleCtrlGroup(CTLSEL_BUDGET_CYCLE, 0, 0);
		addGroup(GRP_CYCLE, p_cycle_grp);
		SetupCalPeriod(CTLCAL_BUDGET_PERIOD, CTL_BUDGET_PERIOD);
		disableCtrl(CTL_BUDGET_ID, 1);
	}
	int setDTS(const PPBudgetPacket *);
	int getDTS(PPBudgetPacket *);
private:
	DECL_HANDLE_EVENT;

	PPBudgetPacket Data;
	PPObjBudget    BudgObj;
};

IMPL_HANDLE_EVENT(BudgetDialog)
{
	TDialog::handleEvent(event);
	if(event.isCmd(cmScenaries)) {
		BudgetScenDialog * p_dlg = 0;
		if(CheckDialogPtrErr(&(p_dlg = new BudgetScenDialog)) > 0) {
			p_dlg->setDTS(&Data);
			for(int valid_data = 0; !valid_data && ExecView(p_dlg) > 0;) {
				if(p_dlg->getDTS(&Data) > 0)
					valid_data = 1;
				else
					PPError();
			}
		}
		delete p_dlg;
	}
}

int BudgetDialog::setDTS(const PPBudgetPacket * pData)
{
	uint   idx = 0;
	DateRange period;
	CycleCtrlGroup::Rec cycle_rec;
	if(!RVALUEPTR(Data, pData))
		Data.Init();
	if(Data.ScenList.getCount() == 0) {
		PPBudget rec = Data.Rec;
		PPGetWord(PPWORD_BASE, 0, rec.Name, sizeof(rec.Name));
		Data.ScenList.insert(&rec);
	}
	cycle_rec.C.Cycle = Data.Rec.Cycle;
	setGroupData(GRP_CYCLE, &cycle_rec);
	setCtrlString(CTL_BUDGET_CODE, SString(Data.Rec.Code));
	setCtrlString(CTL_BUDGET_NAME, SString(Data.Rec.Name));
	setCtrlData(CTL_BUDGET_ID, &Data.Rec.ID);
	period.Set(Data.Rec.LowDt, Data.Rec.UppDt);
	SetPeriodInput(this, CTL_BUDGET_PERIOD, &period);
	disableCtrl(CTLCAL_BUDGET_PERIOD, BIN(Data.Rec.ID));
	disableCtrls(BIN(Data.EnumItems(&idx, 0) > 0), CTL_BUDGET_PERIOD, CTLSEL_BUDGET_CYCLE, 0L);
	return 1;
}

int BudgetDialog::getDTS(PPBudgetPacket * pData)
{
	int    ok = 1;
	uint   sel = 0;
	CycleCtrlGroup::Rec cycle_rec;
	DateRange period;
	getCtrlData(CTL_BUDGET_CODE, Data.Rec.Code);
	getCtrlData(sel = CTL_BUDGET_NAME, Data.Rec.Name);
	getCtrlData(CTL_BUDGET_ID,  &Data.Rec.ID);
	THROW_PP(sstrlen(Data.Rec.Name) > 0, PPERR_NAMENEEDED);
	THROW(GetPeriodInput(this, sel = CTL_BUDGET_PERIOD, &period));
	THROW_PP(!period.IsZero(), PPERR_INVPERIODINPUT);
	Data.Rec.LowDt = period.low;
	Data.Rec.UppDt = period.upp;

	getGroupData(GRP_CYCLE, &cycle_rec);
	Data.Rec.Cycle     = cycle_rec.C.Cycle;
	ASSIGN_PTR(pData, Data);
	CATCH
		selectCtrl(sel);
		ok = 0;
	ENDCATCH
	return ok;
}

int SLAPI PPObjBudget::AddBySample(PPID * pID, PPID sampleID)
{
	int  ok = -1, ta = 0;
	PPBudgetPacket pack;
	if(GetPacket(sampleID, &pack) > 0) {
		LDATE sample_low_dt = pack.Rec.LowDt;
		pack.Rec.ID = 0;
		if(Helper_Edit(&pack) > 0) {
			BudgetItemTbl::Rec item;
			for(uint i = 0; pack.EnumItems(&i, &item) > 0;) {
				item.ID       = 0;
				item.BudgetID = 0;
				THROW(pack.UpdateItem(i - 1, &item));
			}
			THROW(ok = PutPacket(&pack.Rec.ID, &pack, 1));
		}
	}
	ASSIGN_PTR(pID, pack.Rec.ID);
	CATCHZOKPPERR
	return ok;
}

int SLAPI PPObjBudget::AddLineBySample(PPID * pID, PPID sampleID)
{
	int  ok = -1, ta = 0;
	BudgetItemTbl::Rec rec;
	// @v10.6.4 MEMSZERO(rec);
	if(ItemsTbl.Search(sampleID, &rec) > 0) {
		rec.ID = 0;
		rec.Dt = ZERODATE;
		if(Helper_EditLine(&rec) > 0) {
			THROW(ok = ItemsTbl.PutItem(pID, &rec, 1));
		}
	}
	ASSIGN_PTR(pID, rec.ID);
	CATCHZOKPPERR
	return ok;
}

int SLAPI PPObjBudget::Helper_Edit(PPBudgetPacket * pPack)
{
	int    ok = -1, is_locked = 0;
	BudgetDialog * p_dlg = 0;

	THROW(CheckRights(PPR_READ));
	THROW(CheckDialogPtr(&(p_dlg = new BudgetDialog)));
	p_dlg->setDTS(pPack);
	for(int valid_data = 0; !valid_data && ExecView(p_dlg) == cmOK;) {
		if(p_dlg->getDTS(pPack) > 0) {
			valid_data = 1;
			ok = 1;
		}
		else
			PPError();
	}
	CATCHZOKPPERR
	delete p_dlg;
	return ok;
}

/*virtual*/int SLAPI PPObjBudget::Edit(PPID * pID, void * extraPtr)
{
	int    r = cmCancel, is_locked = 0, is_new = 0;
	PPBudgetPacket pack;
	THROW(EditPrereq(pID, 0, &is_new));
	if(!is_new) {
		THROW(GetPacket(*pID, &pack) > 0);
		// THROW(Lock(*pID));
		is_locked = 1;
	}
	if(Helper_Edit(&pack) > 0) {
		THROW(PutPacket(pID, &pack, 1));
		r = cmOK;
	}
	CATCH
		r = PPErrorZ();
	ENDCATCH
	/*
	if(is_locked)
		Unlock(*pID);
	*/
	return r;
}

#define CYCLE_BEG_YEAR 1980
#define CYCLE_END_YEAR 2051

class BudgetItemDialog : public TDialog {
public:
	explicit BudgetItemDialog(PPObjBudget * pObj) : TDialog(DLG_BUDGITEM), P_Obj(pObj)
	{
		SetupCalDate(CTLCAL_BUDGITEM_DT, CTL_BUDGITEM_DT);
	}
	int setDTS(const BudgetItemTbl::Rec * pData);
	int getDTS(BudgetItemTbl::Rec * pData);
private:
	DECL_HANDLE_EVENT;

	BudgetItemTbl::Rec Data;
	PPObjBudget * P_Obj;
};

IMPL_HANDLE_EVENT(BudgetItemDialog)
{
	TDialog::handleEvent(event);
	if(event.isCbSelected(CTLSEL_BUDGITEM_BUDGET)) {
		PPBudget budg_rec;
		PPID id = getCtrlLong(CTLSEL_BUDGITEM_BUDGET);
		if(P_Obj && P_Obj->Search(id, &budg_rec) > 0 && budg_rec.Cycle > 1)
			endModal(cmaMore);
		clearEvent(event);
	}
}

int BudgetItemDialog::setDTS(const BudgetItemTbl::Rec * pData)
{
	uint   sel = CTL_BUDGITEM_DT;
	if(!RVALUEPTR(Data, pData))
		MEMSZERO(Data);
	SetupPPObjCombo(this, CTLSEL_BUDGITEM_BUDGET, PPOBJ_BUDGET,   Data.BudgetID, OLW_CANSELUPLEVEL, reinterpret_cast<void *>(SEL_ALL_BUDGETS));
	SetupPPObjCombo(this, CTLSEL_BUDGITEM_ACCT,   PPOBJ_ACCOUNT2, Data.Acc, OLW_CANSELUPLEVEL|OLW_CANINSERT, reinterpret_cast<void *>(ACY_SEL_BUDGET));
	setCtrlData(CTL_BUDGITEM_DT, &Data.Dt);
	AddClusterAssocDef(CTL_BUDGITEM_KIND, 0, 0);
	AddClusterAssoc(CTL_BUDGITEM_KIND, 1, 1);
	SetClusterData(CTL_BUDGITEM_KIND, Data.Kind);
	setCtrlData(CTL_BUDGITEM_AMOUNT,  &Data.Amount);
	setCtrlData(CTL_BUDGITEM_MEMO,    Data.Memo);
	disableCtrls(Data.ID, CTL_BUDGITEM_KIND, CTLSEL_BUDGITEM_ACCT, 0L);
	disableCtrl(CTLSEL_BUDGITEM_BUDGET, 1);
	{
		int disable_dt = 0;
		PPBudget budg_rec;
		disable_dt = BIN(Data.ID || P_Obj && P_Obj->Search(Data.BudgetID, &budg_rec) > 0 && budg_rec.Cycle != 1);
		disableCtrl(CTL_BUDGITEM_DT, disable_dt);
		::ShowWindow(::GetDlgItem(H(), CTLCAL_BUDGITEM_DT), BIN(disable_dt == 0) ? SW_SHOW : SW_HIDE);
	}
	if(Data.Acc == 0)
		sel = CTL_BUDGITEM_ACCT;
	else
		sel = CTL_BUDGITEM_AMOUNT;
	selectCtrl(sel);
	return 1;
}

int BudgetItemDialog::getDTS(BudgetItemTbl::Rec * pData)
{
	int    ok = 1;
	uint   sel = 0;
	getCtrlData(sel = CTL_BUDGITEM_DT, &Data.Dt);
	getCtrlData(sel = CTLSEL_BUDGITEM_BUDGET, &Data.BudgetID);
	THROW_PP(Data.BudgetID, PPERR_INVBUDGET);
	getCtrlData(sel = CTLSEL_BUDGITEM_ACCT,   &Data.Acc);
	THROW_PP(Data.Acc, PPERR_INVBUDGACC);
	GetClusterData(CTL_BUDGITEM_KIND, &Data.Kind);
	getCtrlData(sel = CTL_BUDGITEM_AMOUNT,  &Data.Amount);
	THROW_PP(Data.Amount, PPERR_INVAMOUNT);
	getCtrlData(CTL_BUDGITEM_MEMO,    Data.Memo);
	ASSIGN_PTR(pData, Data);
	CATCH
		selectCtrl(sel);
		ok = 0;
	ENDCATCH
	return ok;
}

class BudgetItemsDialog : public PPListDialog {
public:
	BudgetItemsDialog(PPID initID, PPObjBudget * pObj) : PPListDialog(DLG_BUDGITEMS, CTL_BUDGITEM_LIST),
		InitID(initID), PrevAcctID(0), PrevKind(0), P_Obj(pObj)
	{
	}
	int setDTS(const BudgetItemsList * pData);
	int getDTS(BudgetItemsList * pData);
private:
	DECL_HANDLE_EVENT;
	virtual int setupList();
	virtual int delItem(long pos, long id);
	virtual int editItem(long pos, long id);
	int SetupItem(long id, int sel);

	long   PrevKind;
	PPID   InitID;
	PPID   PrevAcctID;
	BudgetItemsList Data;
	PPObjBudget * P_Obj;
};

IMPL_HANDLE_EVENT(BudgetItemsDialog)
{
	PPListDialog::handleEvent(event);
	if(event.isClusterClk(CTL_BUDGITEM_KIND)) {
		long kind = 0;
		PPID acct_id = 0;
		GetClusterData(CTL_BUDGITEM_KIND, &kind);
		getCtrlData(CTLSEL_BUDGITEM_ACCT,   &acct_id);
		if(acct_id && kind != PrevKind)
			endModal(cmaMore);
		clearEvent(event);
	}
	else if(event.isCbSelected(CTLSEL_BUDGITEM_ACCT)) {
		PPID acct_id = getCtrlLong(CTLSEL_BUDGITEM_ACCT);
		if(acct_id != PrevAcctID)
			endModal(cmaMore);
		clearEvent(event);
	}
}

/*virtual*/int BudgetItemsDialog::setupList()
{
	SString buf;
	const uint count = Data.getCount();
	for(uint i = 0; i < count; i++) {
		const BudgetItemTbl::Rec & r_rec = Data.at(i);
		StringSet ss(SLBColumnDelim);
		buf.Z();
		CALLPTRMEMB(P_Obj, FormatDate(r_rec.BudgetID, 0, r_rec.Dt, buf));
		ss.add(buf);
		buf.Z().Cat(r_rec.Amount, SFMT_MONEY);
		ss.add(buf);
		buf = r_rec.Memo;
		ss.add(buf);
		addStringToList(i+1, ss.getBuf());
	}
	return 1;
}

/*virtual*/int BudgetItemsDialog::delItem(long pos, long id)
{
	int    ok = -1;
	if(pos >= 0 && pos < Data.getCountI()) {
		Data.at(pos).Amount = 0;
		memzero(Data.at(pos).Memo, sizeof(Data.at(pos).Memo));
		ok = 1;
	}
	return ok;
}

/*virtual*/int BudgetItemsDialog::editItem(long pos, long id)
{
	int    ok = -1;
	if(pos >= 0 && pos < Data.getCountI()) {
		TDialog * p_dlg = new TDialog(DLG_BUDGICYCLEPAR);
		if(CheckDialogPtrErr(&p_dlg) > 0) {
			double amount = Data.at(pos).Amount;
			SString memo(Data.at(pos).Memo);
			p_dlg->setCtrlData(CTL_BUDGICYCLEPAR_AMT,    &amount);
			p_dlg->setCtrlString(CTL_BUDGICYCLEPAR_MEMO, memo);
			if(ExecView(p_dlg) == cmOK) {
				p_dlg->getCtrlData(CTL_BUDGICYCLEPAR_AMT,    &amount);
				p_dlg->getCtrlString(CTL_BUDGICYCLEPAR_MEMO, memo);
				ok = 1;
			}
			if(ok > 0) {
				Data.at(pos).Amount = amount;
				memo.CopyTo(Data.at(pos).Memo, sizeof(Data.at(pos).Memo));
			}
		}
	}
	return ok;
}

int BudgetItemsDialog::setDTS(const BudgetItemsList * pData)
{
	uint pos = 0;
	uint sel = CTL_BUDGITEM_LIST;
	BudgetItemTbl::Rec rec;
	if(pData)
		Data.copy(*pData);
	else
		Data.freeAll();
	// @v10.6.4 MEMSZERO(rec);
	if(Data.lsearch(&InitID, &pos, PTR_CMPFUNC(long)) > 0)
		rec = Data.at(pos);
	for(uint i = 0; rec.ID == 0 && i < Data.getCount(); i++) {
		rec = Data.at(i);
		if(rec.ID)
			pos = i;
	}
	SetupPPObjCombo(this, CTLSEL_BUDGITEM_BUDGET, PPOBJ_BUDGET,   rec.BudgetID, OLW_CANSELUPLEVEL, reinterpret_cast<void *>(SEL_ALL_BUDGETS));
	SetupPPObjCombo(this, CTLSEL_BUDGITEM_ACCT,   PPOBJ_ACCOUNT2, rec.Acc, OLW_CANSELUPLEVEL|OLW_CANINSERT, reinterpret_cast<void *>(ACY_SEL_BUDGET));
	AddClusterAssocDef(CTL_BUDGITEM_KIND, 0, 0);
	AddClusterAssoc(CTL_BUDGITEM_KIND, 1, 1);
	SetClusterData(CTL_BUDGITEM_KIND, rec.Kind);
	disableCtrl(CTLSEL_BUDGITEM_BUDGET, 1);
	if(rec.Acc == 0)
		sel = CTL_BUDGITEM_ACCT;
	else
		sel = CTL_BUDGITEM_LIST;
	selectCtrl(sel);
	PrevAcctID = rec.Acc;
	PrevKind   = rec.Kind;
	updateList(-1);
	if(P_Box && P_Box->def) {
		P_Box->def->go(pos);
		P_Box->Draw_();
	}
	return 1;
}

int BudgetItemsDialog::getDTS(BudgetItemsList * pData)
{
	int  ok = 1;
	uint sel = 0;
	BudgetItemTbl::Rec rec;
	// @v10.6.4 MEMSZERO(rec);
	getCtrlData(sel = CTLSEL_BUDGITEM_BUDGET, &rec.BudgetID);
	THROW_PP(rec.BudgetID, PPERR_INVBUDGET);
	getCtrlData(sel = CTLSEL_BUDGITEM_ACCT,   &rec.Acc);
	THROW_PP(rec.Acc, PPERR_INVBUDGACC);
	GetClusterData(CTL_BUDGITEM_KIND, &rec.Kind);
	for(uint i = 0; i < Data.getCount(); i++) {
		Data.at(i).BudgetID = rec.BudgetID;
		Data.at(i).Acc      = rec.Acc;
		Data.at(i).Kind     = rec.Kind;
	}
	ASSIGN_PTR(pData, Data);
	CATCH
		selectCtrl(sel);
		ok = 0;
	ENDCATCH
	return ok;
}

int SLAPI PPObjBudget::Helper_EditLine(BudgetItemTbl::Rec * pRec)
{
	int    ok = -1;
	uint cm = 0;
	BudgetItemDialog * p_dlg = 0;

	THROW_INVARG(pRec);
	THROW(CheckDialogPtr(&(p_dlg = new BudgetItemDialog(this))));
	p_dlg->setDTS(pRec);
	for(int valid_data = 0; !valid_data && ((cm = ExecView(p_dlg)) == cmOK || cm == cmaMore);) {
		if(p_dlg->getDTS(pRec) > 0)
			ok = valid_data = 1;
		else if(cm != cmaMore)
			PPError();
		if(cm == cmaMore)
			ok = valid_data = 2;
	}
	CATCHZOKPPERR
	delete p_dlg;
	return ok;
}

int SLAPI PPObjBudget::Helper_EditLines(PPID initID, BudgetItemsList * pList)
{
	int    ok = -1;
	uint cm = 0;
	BudgetItemsDialog * p_dlg = 0;

	THROW_INVARG(pList);
	THROW(CheckDialogPtr(&(p_dlg = new BudgetItemsDialog(initID, this))));
	p_dlg->setDTS(pList);
	for(int valid_data = 0; !valid_data && ((cm = ExecView(p_dlg)) == cmOK || cm == cmaMore);) {
		if(p_dlg->getDTS(pList) > 0)
			ok = valid_data = 1;
		else if(cm != cmaMore)
			PPError();
		if(cm == cmaMore)
			ok = valid_data = 2;
	}
	CATCHZOKPPERR
	delete p_dlg;
	return ok;
}

int SLAPI PPObjBudget::InitItemsCycleList(const BudgetItemTbl::Rec * pRec, BudgetItemsList * pList)
{
	int    ok = -1;
	BudgetItemTbl::Rec rec;
	PPBudget budg_rec;
	THROW_INVARG(pRec && pList);
	pList->freeAll();
	rec = *pRec;
	if(rec.BudgetID && Search(rec.BudgetID, &budg_rec) > 0 && budg_rec.Cycle > 1) {
		long beg = 1, end = 0;
		if(budg_rec.Cycle == 7)
			end = 7;
		else
			end = 360 / budg_rec.Cycle;
		if(budg_rec.Cycle == 360) {
			beg = CYCLE_BEG_YEAR;
			end = CYCLE_END_YEAR;
		}
		MEMSZERO(rec);
		rec.Acc      = pRec->Acc;
		rec.BudgetID = pRec->BudgetID;
		rec.Kind     = pRec->Kind;
		for(long c = beg; c <= end; c++) {
			rec.Dt.v = c;
			THROW_SL(pList->insert(&rec));
		}
		if(rec.Acc) {
			BudgetItemsList list;
			THROW(ItemsTbl.GetItemsByBudget(pRec->BudgetID, pRec->Acc, pRec->Kind, &list));
			for(uint i = 0; i < list.getCount(); i++) {
				uint p = 0;
				if(pList->lsearch(&list.at(i).Dt.v, &p, PTR_CMPFUNC(long), offsetof(BudgetItemTbl::Rec, Dt)) > 0)
					pList->at(p) = list.at(i);
			}
		}
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjBudget::EditLine(PPID * pID, PPIDArray * pIdList, PPID budgetID, LDATE dt)
{
	int    ok = -1, by_list = 0;
	PPID   sel_id = DEREFPTRORZ(pID);
	BudgetItemsList list;
	BudgetItemTbl::Rec rec;
	BudgetItemDialog * p_dlg = 0;
	THROW_INVARG(pIdList);
	// @v10.6.4 MEMSZERO(rec);
	if(pID && *pID) {
		THROW(ItemsTbl.Search(*pID, &rec) > 0);
	}
	else {
		rec.BudgetID = budgetID;
		rec.Dt = dt;
	}
	if(InitItemsCycleList(&rec, &list) > 0)
		by_list = 1;
	for(int valid_data = 0, r = 1; !valid_data && r > 0;) {
		if(by_list) {
			THROW(r = Helper_EditLines(sel_id, &list));
			rec = list.at(0);
		}
		else {
			THROW(r = Helper_EditLine(&rec));
			list.freeAll();
			list.insert(&rec);
		}
		if(r == 1) {
			if(ItemsTbl.PutItems(&list, 1) > 0) {
				pIdList->freeAll();
				for(uint i = 0; i < list.getCount(); i++)
					pIdList->add(list.at(i).ID);
				ASSIGN_PTR(pID, list.at(0).ID);
				ok = valid_data = 1;
			}
			else
				PPError();
		}
		else if(r == 2)
			by_list = BIN(InitItemsCycleList(&rec, &list) > 0);
	}
	CATCHZOKPPERR
	delete p_dlg;
	return ok;
}

int SLAPI PPObjBudget::FormatDate(PPID budgetID, int16 cycle, LDATE dt, SString & rText)
{
	int    ok = -1;
	rText.Z();
	if(cycle == 0 && budgetID) {
		PPBudget budg_rec;
		// @v10.7.9 @ctr MEMSZERO(budg_rec);
		if(Search(budgetID, &budg_rec) > 0)
			cycle = budg_rec.Cycle;
	}
	if(cycle == 1) {
		char buf[16];
		memzero(buf, sizeof(buf));
		datefmt(&dt, DATF_DMY, buf);
		rText.CopyFrom(buf);
		ok = 1;
	}
	else {
		SString str;
		if(cycle == 7)
			str = StrWeeks;
		else if(cycle == 30)
			str = StrMonthes;
		else if(cycle == 90)
			str = StrQuarts;
		else if(cycle == 180)
			str = StrSemiYear;
		else if(cycle == 360) {
			rText.Cat(dt.v);
			ok = 1;
		}
		if(str.Len() > 0) {
			StringSet ss(";");
			SString idx_str, name_str;
			ss.setBuf(str, str.Len() + 1);
			for(uint i = 0; ok < 0 && ss.get(&i, str.Z()) > 0;) {
				str.Divide(',', idx_str, name_str);
				if(idx_str.ToLong() == dt.v)
					rText = name_str;
			}
		}
	}
	return ok;
}

/*virtual*/StrAssocArray * SLAPI PPObjBudget::MakeStrAssocList(void * extraPtr)
{
	long   extra_param = reinterpret_cast<long>(extraPtr);
	long   h = 0;
	StrAssocArray * p_list = new StrAssocArray;
	if(p_list) {
		const  long _extra = reinterpret_cast<long>(ExtraPtr);
		int    sel_with_parent = BIN(_extra < 0);
		PPBudget budget;
		extra_param = (extra_param > SEL_ALL_BUDGETS) ? labs(extra_param) : extra_param;
		for(ref->InitEnum(PPOBJ_BUDGET, 0, &h); ref->NextEnum(h, &budget) > 0;)
			if(extra_param == SEL_ALL_BUDGETS || budget.ParentID == extra_param || sel_with_parent && budget.ID == extra_param)
				p_list->Add(budget.ID, budget.ParentID, budget.Name);
	}
	return p_list;
}

IMPLEMENT_PPFILT_FACTORY(Budget); SLAPI BudgetFilt::BudgetFilt() : PPBaseFilt(PPFILT_BUDGET, 0, 0)
{
	SetFlatChunk(offsetof(BudgetFilt, ReserveStart),
		offsetof(BudgetFilt, Reserve)-offsetof(BudgetFilt, ReserveStart)+sizeof(Reserve));
	Init(1, 0);
}

/*virtual*/int SLAPI BudgetFilt::Init(int fullyDestroy, long extraData)
{
	int    ok = PPBaseFilt::Init(fullyDestroy, extraData);
	ParentKind = -1;
	return ok;
}

BudgetFilt & FASTCALL BudgetFilt::operator = (const BudgetFilt & s)
{
	Copy(&s, 0);
	return *this;
}

SLAPI PPViewBudget::PPViewBudget() : PPView(0, &Filt, PPVIEW_BUDGET), P_TempBudgTbl(0), P_TempBudgItemTbl(0), UpdateID(0)
{
	ImplementFlags |= implDontEditNullFilter;
	DefReportId = REPORT_ACCOUNTVIEW;
}

SLAPI PPViewBudget::~PPViewBudget()
{
	delete P_TempBudgTbl;
	delete P_TempBudgItemTbl;
}

/*virtual*/void * SLAPI PPViewBudget::GetEditExtraParam()
{
	return reinterpret_cast<void *>(Filt.Kind);
}

/*virtual*/PPBaseFilt * SLAPI PPViewBudget::CreateFilt(void * extraPtr) const
{
	BudgetFilt * p_filt = 0;
	if(PPView::CreateFiltInstance(PPFILT_BUDGET, reinterpret_cast<PPBaseFilt **>(&p_filt)))
		p_filt->Kind = reinterpret_cast<uint16>(extraPtr); // @valid
	return static_cast<PPBaseFilt *>(p_filt);
}

class BudgetFiltDialog : public TDialog {
public:
	BudgetFiltDialog() : TDialog(DLG_BUDGFLT)
	{
		SetupCalPeriod(CTLCAL_BUDGFLT_PERIOD, CTL_BUDGFLT_PERIOD);
	}
	int setDTS(const BudgetFilt *);
	int getDTS(BudgetFilt *);
private:
	BudgetFilt  Data;
	PPObjBudget BudgObj;
};

int BudgetFiltDialog::setDTS(const BudgetFilt * pData)
{
	if(!RVALUEPTR(Data, pData))
		Data.Init(1, 0);
	SetupPPObjCombo(this, CTLSEL_BUDGFLT_BUDGET, PPOBJ_BUDGET,  Data.BudgetID, 0, reinterpret_cast<void *>(SEL_ALL_BUDGETS));
	SetPeriodInput(this, CTL_BUDGFLT_PERIOD, &Data.Period);
	return 1;
}

int BudgetFiltDialog::getDTS(BudgetFilt * pData)
{
	int    ok = 1;
	getCtrlData(CTL_BUDGFLT_BUDGET, &Data.BudgetID);
	THROW(GetPeriodInput(this, CTL_BUDGFLT_PERIOD, &Data.Period));
	if(Data.Kind == 1)
		THROW_PP(Data.BudgetID, PPERR_INVBUDGET);
	ASSIGN_PTR(pData, Data);
	CATCHZOK
	return ok;
}

/*virtual*/int SLAPI PPViewBudget::EditBaseFilt(PPBaseFilt * pBaseFilt)
{
	int    ok = -1;
	uint   v = 0;
	BudgetFiltDialog * p_dlg = 0;
	if(Filt.IsA(pBaseFilt)) {
		BudgetFilt * p_filt = static_cast<BudgetFilt *>(pBaseFilt);
		p_dlg = new BudgetFiltDialog;
		if(CheckDialogPtrErr(&p_dlg) && p_dlg->setDTS(p_filt)) {
			while(ok <= 0 && ExecView(p_dlg) == cmOK)
				if(p_dlg->getDTS(p_filt))
					ok = 1;
				else
					PPError();
		}
		else
			ok = 0;
	}
	else
		ok = 0;
	delete p_dlg;
	return ok;
}

void SLAPI PPViewBudget::MakeTempRec(const void * pRec, void * pTempRec)
{
	if(Filt.Kind == BudgetFilt::kBudget) {
		const PPBudget * p_rec = static_cast<const PPBudget *>(pRec);
		TempBudgetTbl::Rec temp_rec;
		// @v10.6.4 MEMSZERO(temp_rec);
		if(p_rec) {
			// @v10.6.4 MEMSZERO(temp_rec);
			temp_rec.ID       = p_rec->ID;
			temp_rec.ParentID = p_rec->ParentID;
			STRNSCPY(temp_rec.Code, p_rec->Code);
			STRNSCPY(temp_rec.Name, p_rec->Name);
			temp_rec.LowDt = p_rec->LowDt;
			temp_rec.UppDt = p_rec->UppDt;
		}
		ASSIGN_PTR(static_cast<TempBudgetTbl::Rec *>(pTempRec), temp_rec);
	}
	else {
		const BudgetItemTbl::Rec * p_rec = static_cast<const BudgetItemTbl::Rec *>(pRec);
		TempBudgItemTbl::Rec temp_rec;
		if(p_rec) {
			SString buf;
			PPAccount acc_rec;
			// @v10.6.4 MEMSZERO(temp_rec);
			temp_rec.ID       = p_rec->ID;
			temp_rec.BudgetID = p_rec->BudgetID;
			temp_rec.Acc      = p_rec->Acc;
			temp_rec.Flags    = p_rec->Flags;
			temp_rec.Kind     = p_rec->Kind;
			temp_rec.Dt       = p_rec->Dt;
			temp_rec.PlanAmt   = (p_rec->Kind == 0) ? p_rec->Amount : 0;
			temp_rec.FactAmt   = (p_rec->Kind == 1) ? p_rec->Amount : 0;
			ObjBudg.FormatDate(temp_rec.BudgetID, 0, temp_rec.Dt, buf);
			buf.CopyTo(temp_rec.DtText, sizeof(temp_rec.DtText));
			if(ObjAcct.Fetch(p_rec->Acc, &acc_rec) > 0)
				STRNSCPY(temp_rec.AccText, acc_rec.Name);
			ASSIGN_PTR(static_cast<TempBudgItemTbl::Rec *>(pTempRec), temp_rec);
		}
	}
}

int SLAPI PPViewBudget::CheckForFilt(void * pRec)
{
	int    ok = 1;
	if(Filt.Kind == BudgetFilt::kBudget) {
		PPBudget * p_rec = static_cast<PPBudget *>(pRec);
		if(p_rec->ParentID != 0)
			ok = 0;
		else if(!Filt.Period.CheckDate(p_rec->LowDt) || !Filt.Period.CheckDate(p_rec->UppDt))
			ok = 0;
	}
	else {
		PPAccount acc_rec;
		BudgetItemTbl::Rec * p_rec = static_cast<BudgetItemTbl::Rec *>(pRec);
		if(Filt.BudgetID != p_rec->BudgetID)
			ok = 0;
		else if(Filt.ParentKind != -1 && Filt.ParentKind != p_rec->Kind)
			ok = 0;
		else if(Filt.ParentDt != ZERODATE && Filt.ParentDt != p_rec->Dt)
			ok = 0;
		else if(ObjAcct.Search(p_rec->Acc, &acc_rec) > 0 && acc_rec.ParentID != Filt.ParentAcctID)
			ok = 0;
	}
	return ok;
}

PP_CREATE_TEMP_FILE_PROC(CreateTempFile, TempBudget);
PP_CREATE_TEMP_FILE_PROC(CreateTempItemFile, TempBudgItem);

int SLAPI PPViewBudget::UpdateTempTable(const PPIDArray & rIdList)
{
	int    ok = 1;
	PPID  id = 0;
	if(P_TempBudgTbl) {
		id = rIdList.getCount() ? rIdList.at(0) : 0;
		PPBudget budg_rec;
		TempBudgetTbl::Key0 k0;
		TempBudgetTbl::Rec  temp_rec;
		k0.ID = id;
		// @v10.6.4 MEMSZERO(temp_rec);
		// @v10.7.9 @ctr MEMSZERO(budg_rec);
		if(id) {
			if(ObjBudg.Search(id, &budg_rec) > 0 && CheckForFilt(&budg_rec) > 0) {
				MakeTempRec(&budg_rec, &temp_rec);
				if(SearchByID_ForUpdate(P_TempBudgTbl, 0,  id, 0) > 0) {
					THROW_DB(P_TempBudgTbl->updateRecBuf(&temp_rec));
				}
				else {
					THROW_DB(P_TempBudgTbl->insertRecBuf(&temp_rec));
				}
			}
			else
				deleteFrom(P_TempBudgTbl, 0, P_TempBudgTbl->ID == id);
		}
		ok = 1;
	}
	else {
		uint count = rIdList.getCount();
		for(uint i = 0; i < count; i++) {
			const PPID id = rIdList.at(i);
			BudgetItemTbl::Rec rec;
			TempBudgItemTbl::Rec  temp_rec;
			TempBudgItemTbl::Key0 k0;
			// @v10.6.4 MEMSZERO(temp_rec);
			// @v10.6.4 MEMSZERO(rec);
			k0.ID = id;
			if(ObjBudg.ItemsTbl.Search(id, &rec) > 0 && CheckForFilt(&rec) > 0) {
				MakeTempRec(&rec, &temp_rec);
				if(SearchByID_ForUpdate(P_TempBudgItemTbl, 0,  id, 0) > 0) {
					THROW_DB(P_TempBudgItemTbl->updateRecBuf(&temp_rec));
				}
				else {
					THROW_DB(P_TempBudgItemTbl->insertRecBuf(&temp_rec));
				}
			}
			else
				deleteFrom(P_TempBudgItemTbl, 0, P_TempBudgItemTbl->ID == id);
			//
			// Если счет введенного элемента не удовлетворяет фильтру, попробуем отыскать элемент с родительским счетом, который бы подходил под
			// условия фильтра.
			//
			{
				PPAccount acc_rec;
				MEMSZERO(acc_rec);
				if(rec.Acc && ObjAcct.Search(rec.Acc, &acc_rec) > 0 && acc_rec.ParentID != Filt.ParentAcctID) {
					StrAssocArray list;
					if(ObjAcct.GetParentList(rec.Acc, &list) > 0) {
						PPIDArray idlist;
						for(uint i = 0, stop = 0; !stop && i < list.getCount(); i++) {
							StrAssocArray::Item _item = list.Get(i);
							if(_item.ParentId == Filt.ParentAcctID) {
								BudgetItemTbl::Rec par_rec;
								// @v10.6.4 MEMSZERO(par_rec);
								stop = 1;
								if(ObjBudg.ItemsTbl.Search(rec.BudgetID, _item.Id, rec.Kind, rec.Dt, &par_rec) > 0) {
									idlist.clear();
									idlist.add(par_rec.ID);
									UpdateTempTable(idlist); // @recursion
								}
							}
						}
					}
				}
			}
			ok = 1;
		}
	}
	CATCHZOK
	return ok;
}

void SLAPI PPViewBudget::GetTabTitle(long tabID, SString & rBuf)
{
	rBuf.Z();
	if(tabID) {
		LDATE dt;
		dt.v = tabID;
		ObjBudg.FormatDate(Filt.BudgetID, 0, dt, rBuf);
	}
}

/*virtual*/int SLAPI PPViewBudget::Init_(const PPBaseFilt * pFilt)
{
	int    ok = 1;
	THROW(Helper_InitBaseFilt(pFilt));
	BExtQuery::ZDelete(&P_IterQuery);
	ZDELETE(P_TempBudgTbl);
	ZDELETE(P_TempBudgItemTbl);
	if(Filt.Kind == BudgetFilt::kBudget) {
		THROW(P_TempBudgTbl = CreateTempFile());
		{
			long h = 0;
			PPBudget budget;
			BExtInsert bei(P_TempBudgTbl);
			PPTransaction tra(ppDbDependTransaction, 1);
			for(ObjBudg.ref->InitEnum(PPOBJ_BUDGET, 0, &h); ObjBudg.ref->NextEnum(h, &budget) > 0;) {
				if(CheckForFilt(&budget) > 0) {
					TempBudgetTbl::Rec temp_rec;
					// @v10.6.4 MEMSZERO(temp_rec);
					MakeTempRec(&budget, &temp_rec);
					THROW_DB(bei.insert(&temp_rec));
				}
			}
			THROW_DB(bei.flash());
			THROW(tra.Commit());
		}
	}
	else {
		THROW(P_TempBudgItemTbl = CreateTempItemFile());
		{
			BExtInsert bei(P_TempBudgItemTbl);
			BudgetItemTbl * t = &ObjBudg.ItemsTbl;
			BExtQuery q(t, 0);
			BudgetItemTbl::Rec k0;
			PPTransaction tra(ppDbDependTransaction, 1);

			MEMSZERO(k0);
			q.selectAll();
			for(q.initIteration(0, &k0, spGe); q.nextIteration() > 0;) {
				if(CheckForFilt(&ObjBudg.ItemsTbl.data) > 0) {
					TempBudgItemTbl::Rec temp_rec;
					// @v10.6.4 MEMSZERO(temp_rec);
					MakeTempRec(&ObjBudg.ItemsTbl.data, &temp_rec);
					THROW_DB(bei.insert(&temp_rec));
				}
			}
			THROW_DB(bei.flash());
			THROW(tra.Commit());
		}
		{
			class BudgetItemsCrosstab : public Crosstab {
			public:
				explicit SLAPI  BudgetItemsCrosstab(PPViewBudget * pV) : Crosstab(), P_V(pV)
				{
				}
				virtual BrowserWindow * SLAPI CreateBrowser(uint brwId, int dataOwner)
				{
					PPViewBrowser * p_brw = new PPViewBrowser(brwId, CreateBrowserQuery(), P_V, dataOwner);
					SetupBrowserCtColumns(p_brw);
					return p_brw;
				}
			private:
				virtual void SLAPI GetTabTitle(const void * pVal, TYPEID typ, SString & rBuf) const
				{
					if(pVal && P_V) 
						P_V->GetTabTitle(*static_cast<const long *>(pVal), rBuf);
				}
				PPViewBudget * P_V;
			};
			PPBudget   budg_rec;
			Crosstab * p_prev_ct = P_Ct;
			P_Ct = 0;
			if(ObjBudg.Search(Filt.BudgetID, &budg_rec) > 0 && budg_rec.Cycle > 1) {
				SString plan_word, fact_word, total_word;
				DBFieldList total_list;

				// @v9.0.2 PPGetWord(PPWORD_PLAN,  0, plan_word);
				PPLoadString("plan", plan_word); // @v9.0.2
				// @v9.0.2 PPGetWord(PPWORD_FACT,  0, fact_word);
				PPLoadString("fact", fact_word); // @v9.0.2
				PPGetWord(PPWORD_TOTAL, 0, total_word);

				THROW_MEM(P_Ct = new BudgetItemsCrosstab(this));
				P_Ct->SetTable(P_TempBudgItemTbl, P_TempBudgItemTbl->Dt);
				P_Ct->AddIdxField(P_TempBudgItemTbl->Acc);
				P_Ct->SetSortIdx("AccText", 0);
				P_Ct->AddInheritedFixField(P_TempBudgItemTbl->AccText);

				P_Ct->AddAggrField(P_TempBudgItemTbl->PlanAmt, Crosstab::afSum, plan_word);
				P_Ct->AddAggrField(P_TempBudgItemTbl->FactAmt, Crosstab::afSum, fact_word);

				total_list.Add(P_TempBudgItemTbl->PlanAmt);
				total_list.Add(P_TempBudgItemTbl->FactAmt);
				P_Ct->AddTotalRow(total_list, 0, total_word);
				P_Ct->AddTotalColumn(P_TempBudgItemTbl->PlanAmt, 0, plan_word);
				P_Ct->AddTotalColumn(P_TempBudgItemTbl->FactAmt, 0, fact_word);
				THROW(P_Ct->Create(1));
			}
			ZDELETE(p_prev_ct);
		}
	}
	CATCH
		ok = 0;
		ZDELETE(P_TempBudgTbl);
		ZDELETE(P_TempBudgItemTbl);
	ENDCATCH
	return ok;
}

int SLAPI PPViewBudget::InitIteration()
{
	DBQ * dbq = 0;
	BExtQuery::ZDelete(&P_IterQuery);
	if(Filt.Kind == BudgetFilt::kBudget)
		P_IterQuery = new BExtQuery(P_TempBudgTbl, 0);
	else if(Filt.Kind == BudgetFilt::kBudgetItems)
		P_IterQuery = new BExtQuery(P_TempBudgItemTbl, 0);
	if(P_IterQuery) {
		P_IterQuery->selectAll().where(*dbq);
		P_IterQuery->initIteration(0, 0, 0);
		return 1;
	}
	else
		return 0;
}

int FASTCALL PPViewBudget::NextIteration(BudgetViewItem * pItem)
{
	int    ok = -1;
	if(P_IterQuery && P_IterQuery->nextIteration() > 0) {
		if(Filt.Kind == BudgetFilt::kBudget) {
			PPBudget rec;
			if(ObjBudg.Search(P_TempBudgTbl->data.ID, &rec) > 0) {
				if(pItem)
					pItem->Budget = rec;
				ok = 1;
			}
		}
		else {
			BudgetItemTbl::Rec rec;
			if(ObjBudg.ItemsTbl.Search(P_TempBudgItemTbl->data.ID, &rec) > 0) {
				if(pItem)
					pItem->Item = rec;
				ok = 1;
			}
		}
	}
	return ok;
}

/*virtual*/DBQuery * SLAPI PPViewBudget::CreateBrowserQuery(uint * pBrwId, SString * pSubTitle)
{
	PPID brw_id = 0;
	DBQuery * q = 0;
	DBQ * dbq = 0;
	TempBudgetTbl * tt  = 0;
	TempBudgItemTbl * tit = 0;

	if(Filt.Kind == BudgetFilt::kBudget) {
		THROW(P_TempBudgTbl);
		THROW(CheckTblPtr(tt = new TempBudgetTbl(P_TempBudgTbl->GetName())));
		q = &select(
			tt->ID,
			tt->Name,
			tt->Code,
			tt->LowDt,
			tt->UppDt,
			0L).from(tt, 0L).where(*dbq);
		brw_id = BROWSER_BUDGET;
		THROW(CheckQueryPtr(q));
	}
	else {
		if(P_Ct) {
			brw_id = BROWSER_BUDGETITEMS_CROSSTAB;
			q = PPView::CrosstabDbQueryStub;
		}
		else {
			THROW(CheckTblPtr(tit = new TempBudgItemTbl(P_TempBudgItemTbl->GetName())));
			q = &select(
				tit->ID,
				tit->Acc,
				tit->Dt,
				tit->Kind,
				tit->AccText,
				tit->DtText,
				tit->PlanAmt,
				tit->FactAmt,
				tit->Memo,
				0L).from(tit, 0L).where(*dbq).orderBy(tit->Dt, 0L);
			brw_id = BROWSER_BUDGET_ITEMS;
			THROW(CheckQueryPtr(q));
		}
	}
	CATCH
		if(q)
			ZDELETE(q);
		else {
			delete tt;
			delete tit;
		}
	ENDCATCH
	ASSIGN_PTR(pBrwId, brw_id);
	return q;
}

class BudgetTotalDialog : public TDialog {
public:
	explicit BudgetTotalDialog(RPair * pData) : TDialog(DLG_BUDGTOTAL)
	{
		SString word;
		if(!RVALUEPTR(Data, pData))
			MEMSZERO(Data);
		setCtrlData(CTL_BUDGTOTAL_PLAN, &Data.X);
		setCtrlData(CTL_BUDGTOTAL_FACT, &Data.Y);
		setCtrlData(CTL_BUDGTOTAL_DIFF, &(Diff = Data.X - Data.Y));
		if(Diff < 0) {
			PPGetWord(PPWORD_DEFICITBUDG, 0, word);
		}
		else if(Diff > 0)
			PPGetWord(PPWORD_PROFICITBUDG, 0, word);
		setStaticText(CTL_BUDGTOTAL_DEFICITTXT, word);
		Ptb.SetBrush(deficitBrush, SPaintObj::bsSolid, GetColorRef(SClrRed), 0);
		Ptb.SetBrush(proficitBrush, SPaintObj::bsSolid, GetColorRef(SClrGreen), 0);
	}
private:
	DECL_HANDLE_EVENT;

	enum {
		deficitBrush  = 1,
		proficitBrush = 2
	};

	double Diff;
	RPair Data;
	SPaintToolBox Ptb;
};

IMPL_HANDLE_EVENT(BudgetTotalDialog)
{
	TDialog::handleEvent(event);
	if(TVCOMMAND && event.isCmd(cmCtlColor)) {
		TDrawCtrlData * p_dc = static_cast<TDrawCtrlData *>(TVINFOPTR);
		if(p_dc && getCtrlHandle(CTL_BUDGTOTAL_DEFICITTXT) == p_dc->H_Ctl && Diff != 0.0) {
			::SetBkMode(p_dc->H_DC, TRANSPARENT);
			::SetTextColor(p_dc->H_DC, GetColorRef(SClrWhite));
			p_dc->H_Br = static_cast<HBRUSH>(Ptb.Get((Diff > 0) ? proficitBrush : deficitBrush));
			clearEvent(event);
		}
	}
}

int SLAPI PPViewBudget::ViewTotal()
{
	int    ok = 1;
	BudgetTotalDialog * p_dlg = 0;
	if(Filt.Kind == BudgetFilt::kBudgetItems) {
		RPair data;
		BudgetViewItem item;
		MEMSZERO(data);
		for(InitIteration(); NextIteration(&item) > 0;)
			if(item.Item.Kind == 0)
				data.X += item.Item.Amount;
			else
				data.Y += item.Item.Amount;
		THROW(CheckDialogPtr(&(p_dlg = new BudgetTotalDialog(&data))));
		ExecView(p_dlg);
	}
	CATCHZOKPPERR
	delete p_dlg;
	return ok;
}

/*virtual*/int SLAPI PPViewBudget::Detail(const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = -1;
	long   ct_id = pHdr ? *static_cast<const long *>(pHdr) : 0;
	Hdr hdr;
	GetEditIds(pHdr, &hdr, (pBrw) ? pBrw->GetCurColumn() : 0);
	if(Filt.Kind == BudgetFilt::kBudget) {
		BudgetFilt filt;
		if(hdr.ID) {
			filt.Kind = BudgetFilt::kBudgetItems;
			filt.BudgetID = hdr.ID;
			ok = PPView::Execute(PPVIEW_BUDGET, &filt, PPView::exefModeless, 0);
		}
	}
	else if(hdr.AccID || hdr.Dt || hdr.Kind != -1) {
		BudgetFilt filt;
		filt.Kind = BudgetFilt::kBudgetItems;
		filt.BudgetID = Filt.BudgetID;
		filt.ParentAcctID = hdr.AccID;
		filt.ParentDt     = hdr.Dt;
		filt.ParentKind   = hdr.Kind;
		if(Filt.Kind == BudgetFilt::kBudgetItems)
			UpdateID = ct_id;
		ok = PPView::Execute(PPVIEW_BUDGET, &filt, PPView::exefModeless, 0);
	}
	return ok;
}

/*virtual*/int SLAPI PPViewBudget::Print(const void * pHdr)
{
	int    ok = 1;
	uint rpt_id = rpt_id = REPORT_BUDGET;
	PPID budg_id = (Filt.Kind == BudgetFilt::kBudget) ? (pHdr ? *static_cast<const long *>(pHdr) : 0) : Filt.BudgetID;
	PPBudgetPacket pack;
	if(budg_id && ObjBudg.GetPacket(budg_id, &pack) > 0) {
		PView  pv(&pack);
		PPReportEnv env;
		env.Sort = 0;
		ok = PPAlddPrint(rpt_id, &pv, &env);
	}
	else
		ok = -1;
	return ok;
}

void SLAPI PPViewBudget::GetEditIds(const void * pRow, Hdr * pHdr, long col)
{
	Hdr hdr;
	MEMSZERO(hdr);
	hdr.Kind = -1;
	if(pRow) {
		if(P_Ct) {
			if(col > 0) {
				uint   tab_idx = (col - 1) / 2;
				long   kind = BIN(!(col % 2));
				LDATE dt;
				PPID  acct = 0;
				int r = P_Ct->GetTab(tab_idx, &dt.v);
				if(r > 0) {
					BudgetItemTbl::Rec rec;
					// @v10.6.4 MEMSZERO(rec);
					P_Ct->GetIdxFieldVal(0, pRow, &acct, sizeof(acct));
					if(ObjBudg.ItemsTbl.Search(Filt.BudgetID, acct, kind, dt, &rec) > 0) {
						hdr.ID    = rec.ID;
						hdr.AccID = acct;
						hdr.Dt    = dt;
						hdr.Kind  = kind;

					}
				}
			}
			else
				P_Ct->GetIdxFieldVal(0, pRow, &hdr.AccID, sizeof(hdr.AccID));
			PPAccount rec;
			hdr.AccID = (ObjAcct.Fetch(hdr.AccID, &rec) > 0) ? hdr.AccID : 0;
		}
		else
			hdr = *static_cast<const Hdr *>(pRow);
	}
	ASSIGN_PTR(pHdr, hdr);
}

int SLAPI PPViewBudget::OnExecBrowser(PPViewBrowser * pBrw)
{
	PPIDArray list;
	if(Filt.Kind == BudgetFilt::kBudgetItems && Filt.BudgetID) {
		PPID   parent_id = Filt.BudgetID;
		PPBudget rec;
		if(ObjBudg.Search(Filt.BudgetID, &rec) > 0)
			parent_id = (rec.ParentID) ? rec.ParentID : parent_id;
		pBrw->SetupToolbarCombo(PPOBJ_BUDGET, Filt.BudgetID, OLW_CANSELUPLEVEL, reinterpret_cast<void *>(-parent_id));
	}
	return 1;
}

/*virtual*/int SLAPI PPViewBudget::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	long cur_col = (pBrw) ? pBrw->GetCurColumn() : 0;
	PPID id = 0;
	PPIDArray idlist;
	PPID ct_id = (pHdr) ? *static_cast<const long *>(pHdr) : 0;
	if(ok == -2) {
		Hdr hdr;
		if(ppvCmd != PPVCMD_MOUSEHOVER) {
			GetEditIds(pHdr, &hdr, cur_col);
			id = hdr.ID;
		}
		switch(ppvCmd) {
			case PPVCMD_ADDITEM:
				{
					id = 0;
					ok = (Filt.Kind == BudgetFilt::kBudget) ? ((ObjBudg.Edit(&id, 0) == cmOK) ? 1 : -1) : ObjBudg.EditLine(&id, &idlist, Filt.BudgetID, Filt.ParentDt);
				}
				break;
			case PPVCMD_EDITITEM:
				if(id)
					ok = (Filt.Kind == BudgetFilt::kBudget) ? ((ObjBudg.Edit(&id, 0) == cmOK) ? 1 : -1) : ObjBudg.EditLine(&id, &idlist, Filt.BudgetID, ZERODATE);
				else
					ok = -1;
				break;
			case PPVCMD_DELETEITEM:
				if(id && CONFIRM(PPCFM_DELETE))
					ok = (Filt.Kind == BudgetFilt::kBudget) ? ObjBudg.PutPacket(&id, 0, 1) : ObjBudg.ItemsTbl.PutItem(&id, 0, 1);
				break;
			case PPVCMD_VIEWACC:
				if(Filt.Kind == BudgetFilt::kBudgetItems)
					ok = ObjAcct.Edit(&hdr.AccID, (void *)ACY_BUDGET);
				break;
			case PPVCMD_SYSJ:
				if(id) {
					PPID budg_id = (Filt.Kind == BudgetFilt::kBudget) ? id : Filt.BudgetID;
					ViewSysJournal(PPOBJ_BUDGET, budg_id, 0);
					ok = -1;
				}
				break;
			case PPVCMD_RECEIVEDFOCUS:
				ct_id = UpdateID;
				if(UpdateID)
					idlist.add(UpdateID);
				UpdateID = 0;
				ok = 1;
				break;
			case PPVCMD_ADDBYSAMPLE:
				if(Filt.Kind == BudgetFilt::kBudget)
					ok = ObjBudg.AddBySample(&id, id);
				else
					ok = ObjBudg.AddLineBySample(&id, id);
				break;
			case PPVCMD_TB_CBX_SELECTED:
				{
					ok = -1;
					PPID budg_id = 0;
					if(pBrw && pBrw->GetToolbarComboData(&budg_id) && Filt.BudgetID != budg_id) {
						Filt.BudgetID = budg_id;
						ok = ChangeFilt(1, pBrw);
					}
				}
				break;
			case PPVCMD_MOUSEHOVER:
				{
					long h = 0;
					if(pBrw->ItemByMousePos(&h, 0)) {
						GetEditIds(pHdr, &hdr, h);
						id = hdr.ID;
						if(id && Filt.Kind == BudgetFilt::kBudgetItems) {
							SString buf;
							BudgetItemTbl::Rec rec;
							if(ObjBudg.ItemsTbl.Search(id, &rec) > 0 && sstrlen(rec.Memo))
								PPTooltipMessage(rec.Memo, 0, pBrw->H(), 10000, 0, SMessageWindow::fShowOnCursor|SMessageWindow::fCloseOnMouseLeave|
									SMessageWindow::fTextAlignLeft|SMessageWindow::fOpaque|SMessageWindow::fSizeByText|SMessageWindow::fChildWindow);
						}
					}
					ok = 1;
				}
				break;
		}
	}
	if(P_Ct) {
		if(ct_id) {
			idlist.freeAll();
			idlist.add(ct_id);
		}
	}
	else if(idlist.getCount() == 0 && id)
		idlist.add(id);
	if(ok > 0 && oneof6(ppvCmd, PPVCMD_ADDITEM, PPVCMD_EDITITEM, PPVCMD_DELETEITEM, PPVCMD_ADDBYSAMPLE, PPVCMD_VIEWACC, PPVCMD_RECEIVEDFOCUS) && idlist.getCount()) {
		//
		// Полностью перестраиваем таблицу, так как это Crosstab
		//
		if(P_Ct && Filt.Kind == BudgetFilt::kBudgetItems) {
			if((ok = ChangeFilt(1, pBrw)) > 0 && idlist.getCount()) {
				pBrw->SetCurColumn(cur_col);
				pBrw->search2(&ct_id, CMPF_LONG, srchFirst, 0);
			}
		}
		else if(idlist.getCount())
			ok = UpdateTempTable(idlist);
	}
	return ok;
}
//
// Implementation of PPALDD_BudgetItem
//
//
// Implementation of PPALDD_World
//
PPALDD_CONSTRUCTOR(BudgetItem)
{
	InitFixData(rscDefHdr, &H, sizeof(H));
	Extra[0].Ptr = new BudgetItemCore;
}

PPALDD_DESTRUCTOR(BudgetItem)
{
	Destroy();
	delete static_cast<BudgetItemCore *>(Extra[0].Ptr);
}

int PPALDD_BudgetItem::InitData(PPFilt & rFilt, long rsrv)
{
	int    ok = -1;
	if(rFilt.ID == H.ID)
		ok = DlRtm::InitData(rFilt, rsrv);
	else {
		MEMSZERO(H);
		H.ID = rFilt.ID;
		BudgetItemTbl::Rec rec;
		if(static_cast<BudgetItemCore *>(Extra[0].Ptr)->Search(rFilt.ID, &rec) > 0) {
			H.BudgetID = rec.BudgetID;
			H.Acc    = rec.Acc;
			H.Flags  = rec.Flags;
			H.Kind   = rec.Kind;
			H.Dt     = rec.Dt;
			H.Amount = rec.Amount;
			STRNSCPY(H.Memo, rec.Memo);
			ok = DlRtm::InitData(rFilt, rsrv);
		}
	}
	return ok;
}
//
// Implementation of PPALDD_Budget
//

PPALDD_CONSTRUCTOR(Budget)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		AssignDefIterData(&I, sizeof(I));
	}
}

PPALDD_DESTRUCTOR(Budget) { Destroy(); }

int PPALDD_Budget::InitData(PPFilt & rFilt, long rsrv)
{
	PPBudgetPacket * p_pack = 0;
	if(rsrv)
		Extra[1].Ptr = p_pack = static_cast<PPBudgetPacket *>(rFilt.Ptr);
	if(p_pack) {
		STRNSCPY(H.Name, p_pack->Rec.Name);
		STRNSCPY(H.Code, p_pack->Rec.Code);
		H.PeriodBeg = p_pack->Rec.LowDt;
		H.PeriodEnd = p_pack->Rec.UppDt;
	}
	return DlRtm::InitData(rFilt, rsrv);
}

int PPALDD_Budget::InitIteration(long iterId, int sortId, long rsrv)
{
	uint idx = 0;
	BudgetItemTbl::Rec item;
	PPBudgetPacket * p_pack = static_cast<PPBudgetPacket *>(Extra[1].Ptr);
	IterProlog(iterId, 1);
	if(sortId >= 0)
		SortIdx = sortId;
	return (p_pack && p_pack->EnumItems(&idx, &item) > 0) ? 1 : 0;
}

int PPALDD_Budget::NextIteration(long iterId)
{
	uint no = (uint)I.No;
	BudgetItemTbl::Rec item;
	IterProlog(iterId, 0);
	PPBudgetPacket * p_pack = static_cast<PPBudgetPacket *>(Extra[1].Ptr);
	if(p_pack && p_pack->EnumItems(&no, &item) > 0) {
		I.No = (long)no;
		I.BudgItemID = item.ID;
	}
	else
		return -1;
	return DlRtm::NextIteration(iterId);
}

//
void PPALDD_Budget::Destroy()
{
}
