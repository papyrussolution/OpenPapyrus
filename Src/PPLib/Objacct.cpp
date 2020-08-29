// OBJACCT.CPP
// Copyright (c) A.Sobolev 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2010, 2011, 2015, 2016, 2017, 2018, 2019, 2020
// @codepage UTF-8
//
#include <pp.h>
#pragma hdrstop

SLAPI PPObjAccount::PPObjAccount(void * extraPtr) : PPObjReference(PPOBJ_ACCOUNT2, extraPtr)
{
	ImplementFlags |= (implStrAssocMakeList|implTreeSelector);
}

SLAPI PPObjAccount::~PPObjAccount()
{
}

//static
Reference2Tbl::Key2 & PPObjAccount::MakeAcctKey(int ac, int sb, Reference2Tbl::Key2 & rKey)
{
    rKey.ObjType = PPOBJ_ACCOUNT2;
	rKey.Val1 = MakeLong((int16)sb, (int16)ac);
    return rKey;
}

// static
void SLAPI PPObjAccount::GenerateCode(PPAccount & rRec)
{
	char   buf[48];
	Acct   acct;
	acct.ac = rRec.A.Ac;
	acct.sb = rRec.A.Sb;
	acct.ar = 0;
	acct.ToStr(ACCF_DEFAULT, buf);
	STRNSCPY(rRec.Code, buf);
}

int SLAPI PPObjAccount::SearchCode(const char * pCode, PPID curID, PPAccount * pRec)
{
	// @todo Это - дорогая функция. Для увеличения производительности необходимо оптимизировать
	int    ok = -1;
	PPAccount rec;
	for(SEnum en = Enum(0); ok < 0 && en.Next(&rec) > 0;) {
		if(stricmp866(pCode, rec.Code) == 0 && rec.CurID == curID) {
			ASSIGN_PTR(pRec, rec);
			ok = 1;
		}
	}
	if(ok < 0)
		PPSetError(PPERR_ACCNFOUND, pCode);
	return ok;
}

int SLAPI PPObjAccount::ParseString(const char * pStr, int tok[])
{
	int    i = 0;
	PPAccount rec;
	char   temp_buf[256];
	char * p = strtok(STRNSCPY(temp_buf, pStr), ".,");
	if(p) {
		do {
			if(i == 0 && (!isdec(p[0]) || p[0] == '0'))
				tok[i++] = (SearchCode(p, 0L, &rec) > 0) ? rec.A.Ac : 0;
			else
				tok[i++] = atoi(p);
		} while(i < 3 && (p = strtok(0, ".,")) != 0);
	}
	while(i < 3)
		tok[i++] = 0;
	return 1;
}

int SLAPI PPObjAccount::AddCurRecord(const PPAccount * pBaseRec, PPID curID)
{
	PPAccount cur_acc_rec = *pBaseRec;
	cur_acc_rec.ID = 0;
	cur_acc_rec.CurID = curID;
	cur_acc_rec.Limit = cur_acc_rec.Overdraft = 0;
	return EditItem(Obj, 0, &cur_acc_rec, 0);
}

int SLAPI PPObjAccount::PutPacket(PPID * pID, PPAccountPacket * pPack, int use_ta)
{
	int    ok = 1;
	uint   i;
	int16  acc_type = 0;
	PPIDArray cur_acc_list;
	PPIDArray cur_list;
	PPAccount cur_acc_rec, acc_rec;
	if(pPack) {
		THROW_PP(pPack->Rec.CurID == 0, PPERR_WACCSCURACC);
		if(*strip(pPack->Rec.Code) == 0 || pPack->Rec.Type == ACY_BAL)
			PPObjAccount::GenerateCode(pPack->Rec);
	}
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(*pID == 0) {
			PPID   new_id = 0;
			acc_type = pPack->Rec.Type;
			if(acc_type == ACY_AGGR) {
				if(pPack->Rec.A.Ac == 0) {
					//GetAggrNumber(&pPack->Rec);
					//int SLAPI AccountCore::GetAggrNumber(PPAccount * pRec)
					for(int _n = MINGENACCNUMBER; pPack->Rec.A.Ac == 0 && _n <= MAXGENACCNUMBER; _n++)
						if(SearchNum(_n, 0, 0L) < 0) {
							pPack->Rec.A.Ac = _n;
						}
				}
			}
			THROW(EditItem(Obj, new_id, &pPack->Rec, 0));
			new_id = pPack->Rec.ID;
			// @v10.2.12 @fix pPack->Rec.ID = new_id;
			ASSIGN_PTR(pID, new_id);
			if(pPack->Rec.CurID == 0 && pPack->CurList.getCount()) {
				for(i = 0; i < pPack->CurList.getCount(); i++) {
					THROW(AddCurRecord(&pPack->Rec, pPack->CurList.at(i)));
				}
			}
			DS.LogAction(PPACN_OBJADD, PPOBJ_ACCOUNT2, new_id, 0, 0);
		}
		else if(pPack) {
			PPAccount prev_rec;
			THROW(Search(*pID, &prev_rec) > 0);
			pPack->Rec.ID = *pID;
			acc_type = pPack->Rec.Type;
			THROW(ref->UpdateItem(Obj, *pID, &pPack->Rec, 0 /*logAction*/, 0));
			if(pPack->Rec.CurID == 0) {
				THROW(GetCurList(pPack->Rec.A.Ac, pPack->Rec.A.Sb, &cur_acc_list, &cur_list));
				for(i = 0; i < pPack->CurList.getCount(); i++) {
					PPID   cur_id = pPack->CurList.at(i);
					uint   pos = 0;
					if(cur_list.lsearch(cur_id, &pos)) {
						PPID   acc_id = cur_acc_list.at(pos);
						THROW(Search(acc_id, &cur_acc_rec) > 0);
						cur_acc_rec.A = pPack->Rec.A;
						STRNSCPY(cur_acc_rec.Name, pPack->Rec.Name);
						cur_acc_rec.AccSheetID = pPack->Rec.AccSheetID;
						cur_acc_rec.Kind  = pPack->Rec.Kind;
						cur_acc_rec.Limit = cur_acc_rec.Overdraft = 0;
						//THROW_DB(updateRecBuf(&cur_acc_rec));
						THROW(ref->UpdateItem(Obj, acc_id, &cur_acc_rec, 0 /*logAction*/, 0));
						cur_list.atFree(pos);
						cur_acc_list.atFree(pos);
					}
					else {
						THROW(AddCurRecord(&pPack->Rec, cur_id));
					}
				}
				for(i = 0; i < cur_acc_list.getCount(); i++) {
					const PPID acc_id = cur_acc_list.get(i);
					THROW_DB(deleteFrom(ref, 0, (ref->ObjType == Obj && ref->ObjID == acc_id)));
				}
			}
			DS.LogAction(PPACN_OBJUPD, PPOBJ_ACCOUNT2, *pID, 0, 0);
		}
		else {
			THROW(Search(*pID, &acc_rec) > 0);
			acc_type = acc_rec.Type;
			if(acc_rec.A.Sb == 0) {
				THROW_PP(HasAnySubacct(acc_rec.A.Ac) <= 0, PPERR_ACCHASBRANCH);
				THROW(Search(*pID, &acc_rec) > 0);
			}
			THROW_DB(deleteFrom(ref, 0, (ref->ObjType == Obj && ref->ObjID == *pID)));
			if(acc_rec.CurID == 0) {
				THROW(GetCurList(acc_rec.A.Ac, acc_rec.A.Sb, &cur_acc_list, 0));
				for(i = 0; i < cur_acc_list.getCount(); i++) {
					const PPID acc_id = cur_acc_list.get(i);
					THROW_DB(deleteFrom(ref, 0, (ref->ObjType == Obj && ref->ObjID == acc_id)));
				}
			}
		}
		if(acc_type == ACY_AGGR)
			THROW(ref->PutPropArray(PPOBJ_ACCOUNT2, *pID, ACCPRP_GENACCLIST, pPack ? &pPack->GenList : 0, 0));
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjAccount::GetPacket(PPID id, PPAccountPacket * pPack)
{
	int    ok = 1;
	MEMSZERO(pPack->Rec);
	pPack->CurList.clear();
	pPack->GenList.clear();
	if(Search(id, &pPack->Rec) > 0) {
		THROW_PP(pPack->Rec.CurID == 0, PPERR_RACCSCURACC);
		if(pPack->Rec.CurID == 0 && pPack->Rec.Flags & ACF_CURRENCY)
			THROW(GetCurList(pPack->Rec.A.Ac, pPack->Rec.A.Sb, 0, &pPack->CurList));
		if(pPack->Rec.Type == ACY_AGGR)
			THROW(ref->GetPropArray(PPOBJ_ACCOUNT2, id, ACCPRP_GENACCLIST, &pPack->GenList));
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

int SLAPI PPObjAccount::InitAccSheetForAcctID(AcctID * pAcctId, PPID * pAccSheetID)
{
	int    r = 1;
	*pAccSheetID = 0;
	if(pAcctId->ac) {
		PPAccount acc_rec;
		if(Search(pAcctId->ac, &acc_rec) > 0) {
			*pAccSheetID = acc_rec.AccSheetID;
			if(*pAccSheetID == 0)
				pAcctId->ar = 0;
		}
		else
			r = 0;
	}
	return r;
}

int SLAPI PPObjAccount::GetCurList(int ac, int sb, PPIDArray * pAccList, PPIDArray * pCurList)
{
	int    ok = 1;
	Reference2Tbl::Key2 k2;
	MakeAcctKey(ac, sb, k2);
	if(ref->search(2, &k2, spEq)) do {
		PPAccount rec;
		ref->copyBufTo(&rec);
		if(rec.A.Ac == (int16)ac && rec.A.Sb == (int16)sb) {
			if(rec.CurID) {
				THROW_SL(!pCurList || pCurList->addUnique(rec.CurID));
				THROW_SL(!pAccList || pAccList->addUnique(rec.ID));
			}
		}
		else
			break;
	} while(ref->search(2, &k2, spNext));
	THROW_DB(BTROKORNFOUND);
	CATCHZOK
	return ok;
}

int SLAPI PPObjAccount::GetIntersectCurList(PPID accID_1, PPID accID_2, PPIDArray * pCurList)
{
	PPIDArray cur_list, cur_list1;
	GetCurList(accID_1, 0, &cur_list);
	GetCurList(accID_2, 0, &cur_list1);
	if(accID_2)
		if(accID_1)
			cur_list.intersect(&cur_list1);
		else
			cur_list.copy(cur_list1);
	CALLPTRMEMB(pCurList, copy(cur_list));
	return 1;
}

int SLAPI PPObjAccount::GetCurList(PPID accID, PPIDArray * pAccList, PPIDArray * pCurList)
{
	int    ok = -1;
	PPAccount rec;
	if(Search(accID, &rec) > 0) {
		if(rec.Flags & ACF_CURRENCY)
			ok = GetCurList(rec.A.Ac, rec.A.Sb, pAccList, pCurList);
		else {
			CALLPTRMEMB(pCurList, addUnique(0L));
			CALLPTRMEMB(pAccList, addUnique(accID));
			ok = 1;
		}
	}
	return ok;
}

int SLAPI PPObjAccount::GetSubacctList(int ac, int sb, PPID curID, PPIDArray * pList)
{
	int    ok = -1;
	Reference2Tbl::Key2 k2;
	MakeAcctKey(ac, (sb >= 0) ? sb : 0, k2);
	if(ref->search(2, &k2, spGe)) do {
		PPAccount rec;
		ref->copyBufTo(&rec);
		if(rec.A.Ac == (int16)ac && (sb < 0 || rec.A.Sb == (int16)sb)) {
			if((curID < 0 || rec.CurID == curID)) {
				THROW_SL(pList->addUnique(rec.ID));
				ok = 1;
			}
		}
		else
			break;
	} while(ref->search(2, &k2, spNext));
	THROW_DB(BTROKORNFOUND);
	CATCHZOK
	return ok;
}

int SLAPI PPObjAccount::HasAnySubacct(int ac)
{
	int    ok = -1;
	Reference2Tbl::Key2 k2;
	MakeAcctKey(ac, 0, k2);
	if(ref->search(2, &k2, spGe)) do {
		PPAccount rec;
		ref->copyBufTo(&rec);
		if(rec.A.Ac == (int16)ac) {
			if(rec.A.Sb != 0) {
				ok = 1;
			}
		}
		else
			break;
	} while(ok < 0 && ref->search(2, &k2, spNext));
	THROW_DB(BTROKORNFOUND);
	CATCHZOK
	return ok;
}

int SLAPI PPObjAccount::SearchNum(int ac, int sb, PPID curID, PPAccount * pRec)
{
	int    ok = -1;
	Reference2Tbl::Key2 k2;
	MakeAcctKey(ac, sb, k2);
	if(ref->search(2, &k2, spEq)) do {
		PPAccount rec;
		ref->copyBufTo(&rec);
		if(rec.A.Ac == (int16)ac && rec.A.Sb == (int16)sb) {
			if(rec.CurID == curID) {
				ASSIGN_PTR(pRec, rec);
				ok = 1;
			}
		}
		else
			break;
	} while(ok < 0 && ref->search(2, &k2, spNext));
	THROW_DB(BTROKORNFOUND);
	if(ok < 0) {
		SString msg;
		msg.Cat(ac).Dot().Cat(sb);
		if(curID)
			msg.CatChar(':').Cat(curID);
		PPSetError(PPERR_ACCNFOUND, msg);
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjAccount::SearchBase(PPID curAccID, PPID * pBaseAccID, PPAccount * pRec)
{
	int    ok = 1;
	PPAccount rec;
	ASSIGN_PTR(pBaseAccID, 0);
	THROW(Search(curAccID, &rec) > 0);
	if(rec.CurID)
		THROW_PP(SearchNum(rec.A.Ac, rec.A.Sb, 0L, &rec) > 0, PPERR_BASEACCNFOUND);
	ASSIGN_PTR(pRec, rec);
	ASSIGN_PTR(pBaseAccID, rec.ID);
	CATCHZOK
	return ok;
}

int SLAPI PPObjAccount::SearchCur(PPID accID, PPID curID, PPID * pCurAccID, PPAccount * pRec)
{
	int    ok = 1;
	PPAccount rec;
	ASSIGN_PTR(pCurAccID, 0);
	THROW(Search(accID, &rec) > 0);
	if(rec.CurID != curID)
		THROW_PP(SearchNum(rec.A.Ac, rec.A.Sb, curID, &rec) > 0, PPERR_CURACCNFOUND);
	ASSIGN_PTR(pRec, rec);
	ASSIGN_PTR(pCurAccID, rec.ID);
	CATCHZOK
	return ok;
}

int SLAPI PPObjAccount::LockFRR(PPID accID, LDATE dt, int doUnlock)
{
	int    ok = -1;
	int    r = 1;
	int    try_count = 5;
	do {
		ReferenceTbl::Key0 k;
		k.ObjType = Obj;
		k.ObjID   = accID;
		if(ref->searchForUpdate(0, &k, spEq)) {
			PPAccount rec;
			ref->copyBufTo(&rec);
			if(doUnlock) {
				rec.Flags &= ACF_FRRL;
				rec.Frrl_Date = ZERODATE;
			}
			else {
				rec.Flags |= ACF_FRRL;
				if(rec.Frrl_Date == 0 || dt < rec.Frrl_Date)
					rec.Frrl_Date = dt;
			}
			r = ref->updateRecBuf(&rec); // @sfu
			if(!r) {
				THROW_DB(BtrError == BE_CONFLICT && try_count > 0);
				//
				// Если встречаем ошибку "Конфликт блокировок на уровне записи", то повторяем попытку чтения-изменения try_count раз.
				//
				ref->unlock(0);
				SDelay(10);
				--try_count;
			}
			else
				ok = 1;
		}
		else {
			THROW_DB(BTROKORNFOUND);
		}
	} while(r == 0);
	CATCHZOK
	return ok;
}

int SLAPI PPObjAccount::GenerateNumber(PPAccount * pRec)
{
	int    ok = -1, r;
	int    start = 0, finish = 0;
	THROW_INVARG(pRec);
	pRec->A.Ac = 0;
	pRec->A.Sb = 0;
	if(pRec->Type == ACY_OBAL) {
		start = 1000;
		finish = 1999;
	}
	else if(pRec->Type == ACY_REGISTER) {
		start = 2000;
		finish = 9999;
	}
	else if(pRec->Type == ACY_AGGR) {
		start = 10000;
		finish = 11999;
	}
	else if(pRec->Type == ACY_ALIAS) {
		start = 12000;
		finish = 13999;
	}
	else if(pRec->Type == ACY_BUDGET) {
		start  = 14000;
		finish = 21999;
	}
	if(start > 0) {
		for(int ac = start; ok < 0 && ac <= finish; ac++) {
			THROW(r = SearchNum(ac, 0, 0L));
			if(r < 0) {
				pRec->A.Ac = ac;
				pRec->A.Sb = 0;
				ok = 1;
			}
		}
	}
	THROW_PP(ok >= 0, PPERR_CANTGENACCNUMBER);
	CATCHZOK
	return ok;
}

int SLAPI PPObjAccount::GetListByAccSheet(PPID accSheetID, PPIDArray & rList)
{
	rList.clear();
	int    ok = 1;
	PPAccount rec;
    for(SEnum en = EnumByIdxVal(2, accSheetID); en.Next(&rec) > 0;) {
		if(rec.Type == ACY_BAL && rec.CurID == 0 && rec.AccSheetID == accSheetID) {
			THROW_SL(rList.add(rec.ID));
		}
    }
    CATCHZOK
    return ok;
}

StrAssocArray * SLAPI PPObjAccount::MakeStrAssocList(void * extraPtr /*acySelType*/)
{
	const  long acy_sel_type = reinterpret_cast<long>(extraPtr);
	SString temp_buf;
	StrAssocArray * p_list = new StrAssocArray;
	PPAccount rec;
	THROW_MEM(p_list);
	{
		ReferenceTbl::Key2 k2;
		MEMSZERO(k2);
		BExtQuery q(ref, 2);
		q.selectAll().where(ref->ObjType == Obj);
		k2.ObjType = Obj;
		k2.Val1 = -MAXLONG;
		for(q.initIteration(0, &k2, spGe); q.nextIteration() > 0;) {
			ref->copyBufTo(&rec);
			int    _suite = 0;
			if(!rec.CurID && (PPMaster || ObjRts.CheckAccID(rec.ID, PPR_READ))) {
				switch(acy_sel_type) {
					case ACY_SEL_AGGR:
						if(rec.Type == ACY_AGGR)
							_suite = 1;
						break;
					case ACY_SEL_REGISTER:
						if(rec.Type == ACY_REGISTER)
							_suite = 1;
						break;
					case ACY_SEL_OBAL:
						if(rec.Type == ACY_OBAL)
							_suite = 1;
						break;
					case ACY_SEL_BAL:
						if(rec.Type == ACY_BAL)
							_suite = 1;
						break;
					case ACY_SEL_ALIAS:
						if(rec.Type == ACY_ALIAS)
							_suite = 1;
						break;
					case ACY_SEL_BUDGET:
						if(rec.Type == ACY_BUDGET)
							_suite = 1;
						break;
					case ACY_SEL_BALOBAL:
						if(oneof3(rec.Type, ACY_BAL, ACY_OBAL, ACY_REGISTER))
							_suite = 1;
						break;
					case ACY_SEL_BALCUR:
						if(rec.Type == ACY_BAL && rec.Flags & ACF_CURRENCY)
							_suite = 1;
						//p_temp_dbe = & (a->Flags & ACF_CURRENCY);
						//dbq = & (*dbq && (*p_temp_dbe == ACF_CURRENCY));
						break;
					case ACY_SEL_BALOBALALIAS:
						if(oneof4(rec.Type, ACY_BAL, ACY_OBAL, ACY_REGISTER, ACY_ALIAS))
							_suite = 1;
						break;
					default:
						if(acy_sel_type >= 1000L) {
							if(rec.Type == ACY_BAL && rec.AccSheetID == (acy_sel_type - 1000L))
								_suite = 1;
							//dbq = & (*dbq && P_Tbl->Type == (long)ACY_BAL && P_Tbl->AccSheetID == (acy_sel_type - 1000L));
						}
						else
							_suite = 1;
						break;
				}
				if(_suite) {
					temp_buf = rec.Code;
					temp_buf.Strip().CatDiv('-', 1).Cat(rec.Name);
					THROW_SL(p_list->Add(rec.ID, rec.ParentID, temp_buf));
				}
			}
		}
	}
	CATCH
		ZDELETE(p_list);
	ENDCATCH
	return p_list;
}

int SLAPI PPObjAccount::Browse(void * extraPtr /*accType*/)
{
	AccountFilt flt;
	flt.Type = reinterpret_cast<long>(extraPtr);
	flt.Flags = 0;
	return PPView::Execute(PPVIEW_ACCOUNT, &flt, 1, 0);
}

class GenAccountDialog : public ObjRestrictListDialog {
	DECL_DIALOG_DATA(PPAccountPacket);
public:
	GenAccountDialog() : ObjRestrictListDialog(DLG_ACCAGGR, CTL_ACCAGGR_LIST)
	{
	}
	DECL_DIALOG_SETDTS()
	{
		RVALUEPTR(Data, pData);
		setParams(PPOBJ_ACCOUNT2, &Data.GenList);
		setCtrlData(CTL_ACCAGGR_CODE, Data.Rec.Code);
		setCtrlData(CTL_ACCAGGR_NAME, Data.Rec.Name);
		setCtrlData(CTL_ACCAGGR_NUMBER, &Data.Rec.A.Ac);
		SetupPPObjCombo(this, CTLSEL_ACCAGGR_ACCSHEET, PPOBJ_ACCSHEET, Data.Rec.AccSheetID, 0, 0);
		disableCtrl(CTLSEL_ACCAGGR_ACCSHEET, Data.GenList.getCount());
		AddClusterAssoc(CTL_ACCAGGR_FLAGS, 0, ACF_SYSNUMBER);
		AddClusterAssoc(CTL_ACCAGGR_FLAGS, 1, ACF_EXCLINNERTRNOVR);
		SetClusterData(CTL_ACCAGGR_FLAGS, Data.Rec.Flags);
		disableCtrl(CTL_ACCAGGR_NUMBER, BIN(Data.Rec.Flags & ACF_EXCLINNERTRNOVR));
		updateList(-1);
		return 1;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok  = 1;
		uint   sel = 0;
		getCtrlData(sel = CTL_ACCAGGR_NAME, Data.Rec.Name);
		THROW_PP(*strip(Data.Rec.Name), PPERR_NAMENEEDED);
		getCtrlData(sel = CTL_ACCAGGR_CODE, Data.Rec.Code);
		strip(Data.Rec.Code);
		getCtrlData(CTL_ACCAGGR_NUMBER, &Data.Rec.A.Ac);
		THROW_PP(*Data.Rec.Code || Data.Rec.A.Ac, PPERR_ACCNUMORCODENEEDED);
		if(Data.Rec.A.Ac < MINGENACCNUMBER || Data.Rec.A.Ac > MAXGENACCNUMBER) {
			sel = CTL_ACCAGGR_NUMBER;
			THROW_PP(Data.Rec.ID == 0 && Data.Rec.A.Ac == 0, PPERR_INVGENACCNUMBER);
		}
		getCtrlData(CTLSEL_ACCAGGR_ACCSHEET, &Data.Rec.AccSheetID);
		GetClusterData(CTL_ACCAGGR_FLAGS, &Data.Rec.Flags);
		AtObj.P_Tbl->SortGenAccList(&Data.GenList);
		ASSIGN_PTR(pData, Data);
		CATCHZOKPPERRBYDLG
		return ok;
	}
private:
	DECL_HANDLE_EVENT;
	virtual void getObjName(PPID objID, long objFlags, SString &);
	virtual void getExtText(PPID objID, long objFlags, SString &);
	virtual int  editItemDialog(ObjRestrictItem *);
	virtual int  addItem(long * pPos, long * pID);
	virtual int  delItem(long pos, long id);
	
	PPObjAccount    AccObj;
	PPObjAccTurn    AtObj;
};

IMPL_HANDLE_EVENT(GenAccountDialog)
{
	ObjRestrictListDialog::handleEvent(event);
	if(event.isCbSelected(CTLSEL_ACCAGGR_ACCSHEET)) {
		PPID   acc_sheet_id = getCtrlLong(CTLSEL_ACCAGGR_ACCSHEET);
		if(PPMessage(mfConf|mfYesNo, PPCFM_FILLACCAGGRLIST) == cmYes) {
			PPIDArray acc_list;
			Data.GenList.freeAll();
			AccObj.GetListByAccSheet(acc_sheet_id, acc_list);
			for(uint i = 0; i < acc_list.getCount(); i++) {
				ObjRestrictItem item(acc_list.at(i), ACO_2);
				Data.GenList.insert(&item);
			}
			selectCtrl(CTL_ACCAGGR_LIST);
			disableCtrl(CTLSEL_ACCAGGR_ACCSHEET, Data.GenList.getCount());
			updateList(-1);
		}
		clearEvent(event);
	}
}

int GenAccountDialog::addItem(long * pPos, long * pID)
{
	int    r = ObjRestrictListDialog::addItem(pPos, pID);
	if(r > 0 && Data.GenList.getCount())
		disableCtrl(CTLSEL_ACCAGGR_ACCSHEET, 1);
	return r;
}

int GenAccountDialog::delItem(long pos, long id)
{
	int    r = ObjRestrictListDialog::delItem(pos, id);
	if(r > 0 && !Data.GenList.getCount())
		disableCtrl(CTLSEL_ACCAGGR_ACCSHEET, 0);
	return r;
}

void GenAccountDialog::getObjName(PPID objID, long objFlags, SString & rBuf)
{
	rBuf.Z();
	SString temp_buf;
	PPAccount acc_rec;
	if(abs(GetAcoByGenFlags(objFlags)) == ACO_3) {
		AcctRelTbl::Rec arel_rec;
		if(AtObj.P_Tbl->AccRel.Search(objID, &arel_rec) > 0) {
			Acct acct;
			acct.ac = arel_rec.Ac;
			acct.sb = arel_rec.Sb;
			acct.ar = arel_rec.Ar;
			acct.ToStr(ACCF_DEFAULT, rBuf);
			GetAcctName(&acct, 0L, 0, temp_buf);
			rBuf.Align(10, ADJ_LEFT).Space().Cat(temp_buf);
		}
		else
			ideqvalstr(objID, rBuf);
	}
	else if(AccObj.Search(objID, &acc_rec) > 0) {
		Acct acct;
		acct.ac = acc_rec.A.Ac;
		acct.sb = acc_rec.A.Sb;
		acct.ar = 0;
		acct.ToStr(ACCF_DEFAULT, rBuf);
		rBuf.Align(8, ADJ_LEFT).Space().Cat(acc_rec.Name);
	}
	else
		ideqvalstr(objID, rBuf);
}

void GenAccountDialog::getExtText(PPID, long objFlags, SString & rBuf)
{
	rBuf.Z();
	int    aco = abs(GetAcoByGenFlags(objFlags));
	if(aco == ACO_1)
		rBuf.CatChar('g').CatChar('1');
	else if(aco == ACO_2)
		rBuf.CatChar('g').CatChar('2');
	if(objFlags & ACGF_NEGATIVE)
		rBuf.CatChar('-');
}

int GenAccountDialog::editItemDialog(ObjRestrictItem * pItem)
{
#define ACCT_GROUP 1
	int  ok = -1, valid_data = 0;
	int  aco = abs(GetAcoByGenFlags(pItem->Flags));
	PPID acc_sheet_id = 0;
	TDialog * dlg = new TDialog(DLG_ACCAGGRI);
	if(CheckDialogPtrErr(&dlg)) {
		ushort v = 0;
		getCtrlData(CTLSEL_ACCAGGR_ACCSHEET, &acc_sheet_id);
		AcctCtrlGroup::Rec ag_rec;
		AcctCtrlGroup * p_grp = new AcctCtrlGroup(CTL_ACCAGGRI_ACC, CTL_ACCAGGRI_ART, CTLSEL_ACCAGGRI_ACCNAME, CTLSEL_ACCAGGRI_ARTNAME);
		dlg->addGroup(ACCT_GROUP, p_grp);
		// @v10.7.3 @ctr MEMSZERO(ag_rec);
		if(oneof2(aco, ACO_1, ACO_2))
			ag_rec.AcctId.ac = pItem->ObjID;
		else
			AtObj.P_Tbl->AcctRelToID(pItem->ObjID, &ag_rec.AcctId, &ag_rec.AccSheetID);
		ag_rec.AccSelParam = acc_sheet_id ? (acc_sheet_id + 1000L) : 0;
		dlg->setGroupData(ACCT_GROUP, &ag_rec);
		if(pItem->ObjID) {
			dlg->disableCtrls(1, CTL_ACCAGGRI_ACC, CTLSEL_ACCAGGRI_ACCNAME, CTL_ACCAGGRI_ART, CTLSEL_ACCAGGRI_ARTNAME, 0);
			if(aco == ACO_3)
				dlg->disableCtrl(CTL_ACCAGGRI_ACCGRP, 1);
		}
		v = 0;
		if(aco == ACO_1)
			v = 1;
		else if(aco == ACO_2)
			v = 2;
		else //if(aco == ACO_3)
			v = 0;
		dlg->setCtrlData(CTL_ACCAGGRI_ACCGRP, &v);
		v = 0;
		SETFLAG(v, 0x01, pItem->Flags & ACGF_NEGATIVE);
		dlg->setCtrlData(CTL_ACCAGGRI_FLAGS, &v);
		while(!valid_data && ExecView(dlg) == cmOK) {
			dlg->getGroupData(ACCT_GROUP, &ag_rec);
			pItem->ObjID = ag_rec.AcctId.ac;
			pItem->Flags = 0;
			dlg->getCtrlData(CTL_ACCAGGRI_FLAGS, &v);
			SETFLAG(pItem->Flags, ACGF_NEGATIVE, v & 0x01);
			dlg->getCtrlData(CTL_ACCAGGRI_ACCGRP, &(v = 0));
			if(v == 0) {
				aco = ACO_3;
				AtObj.P_Tbl->AcctIDToRel(&ag_rec.AcctId, &pItem->ObjID);
			}
			else if(v == 1) {
				aco = ACO_1;
				pItem->ObjID = ag_rec.AcctId.ac;
				pItem->Flags |= ACGF_ACO1GRP;
			}
			else if(v == 2) {
				aco = ACO_2;
				pItem->ObjID  = ag_rec.AcctId.ac;
				pItem->Flags |= ACGF_ACO2GRP;
			}
			if(pItem->ObjID != 0)
				ok = valid_data = 1;
		}
		delete dlg;
	}
	else
		ok = 0;
	return ok;

#undef ACCT_GROUP
}
//
//
//
class AccountDialog : public PPListDialog {
public:
	AccountDialog(uint dlgID) : PPListDialog(dlgID, CTL_ACCOUNT_CURLIST)
	{
		updateList(-1);
	}
	int    setDTS(const PPAccountPacket *);
	int    getDTS(PPAccountPacket *);
private:
	DECL_HANDLE_EVENT;
	virtual int  setupList();
	virtual int  addItem(long * pos, long * id);
	virtual int  delItem(long pos, long id);
	int    validate();

	PPObjAccTurn AtObj;
	PPAccountPacket AccPack;
};

int AccountDialog::validate()
{
	int    r = 1, err = 1;
	uint   sel = 0;
	PPObjAccount accobj;
	if(*strip(AccPack.Rec.Name) == 0) {
		PPSetError(PPERR_NAMENEEDED);
		sel = CTL_ACCOUNT_NAME;
	}
	else if(AccPack.Rec.A.Ac == 0/*&& !(AccPack.Rec.Flags & ACF_SYSNUMBER)*/) {
		PPSetError(PPERR_ACC1NEEDED);
		sel = CTL_ACCOUNT_NUMBER;
	}
	else if(AccPack.Rec.Limit < 0) {
		PPSetError(PPERR_NEGACCLIMIT);
		sel = CTL_ACCOUNT_LIMIT;
	}
	else
		err = 0;
	if(!err && AccPack.Rec.A.Sb) {
		if((r = accobj.SearchNum(AccPack.Rec.A.Ac, 0, 0)) < 0) {
			char   msg[48];
			PPSetError(PPERR_BALNOTEXISTS, itoa(AccPack.Rec.A.Ac, msg, 10));
			sel = CTL_ACCOUNT_NUMBER;
			err = 1;
		}
		else
			err = r ? 0 : 1;
	}
	if(!err && AccPack.Rec.ID == 0) {
		if((r = accobj.SearchNum(AccPack.Rec.A.Ac, AccPack.Rec.A.Sb, 0)) > 0) {
			PPSetError(PPERR_DUPACCOUNTNUM);
			sel = CTL_ACCOUNT_NUMBER;
			err = 1;
		}
		else
			err = r ? 0 : 1;
	}
	if(!err) {
		err = BIN(PPObjAccount::CheckRecursion(AccPack.Rec.ID, AccPack.Rec.ParentID) == 0);
	}
	if(err)
		PPErrorByDialog(this, sel);
	return !err;
}

int AccountDialog::setDTS(const PPAccountPacket * pAccPack)
{
	ushort v = 0;
	int    is_cur_acc = 0;
	AccPack = *pAccPack;

	setCtrlData(CTL_ACCOUNT_CODE,      AccPack.Rec.Code);
	setCtrlData(CTL_ACCOUNT_NUMBER,    &AccPack.Rec.A.Ac);
	setCtrlData(CTL_ACCOUNT_SUBNUMBER, &AccPack.Rec.A.Sb);
	setCtrlData(CTL_ACCOUNT_NAME,      &AccPack.Rec.Name);
	setCtrlData(CTL_ACCOUNT_OVERDRAFT, &AccPack.Rec.Overdraft);
	setCtrlData(CTL_ACCOUNT_LIMIT,     &AccPack.Rec.Limit);
	{
		const  int kind = AccPack.Rec.Kind;
		v = (kind == ACT_ACTIVE) ? 0 : ((kind == ACT_PASSIVE) ? 1 : ((kind == ACT_AP) ? 2 : 0));
		setCtrlUInt16(CTL_ACCOUNT_TYPE, v);
	}
	SetupPPObjCombo(this, CTLSEL_ACCOUNT_ACCSHEET, PPOBJ_ACCSHEET, AccPack.Rec.AccSheetID, OLW_CANINSERT, 0);
	if(AccPack.Rec.ID && !PPMaster) {
		disableCtrls(1, CTL_ACCOUNT_NUMBER, CTL_ACCOUNT_SUBNUMBER, 0);
		if(AccPack.Rec.AccSheetID)
			disableCtrl(CTLSEL_ACCOUNT_ACCSHEET, 1);
	}
	setCtrlUInt16(CTL_ACCOUNT_FLAGS, BIN(AccPack.Rec.Flags & ACF_CURRENCY));
	setCtrlUInt16(CTL_ACCOUNT_OUTBAL, BIN(AccPack.Rec.Type == ACY_OBAL));
	disableCtrl(CTL_ACCOUNT_OUTBAL, 1);
	updateList(-1);
	is_cur_acc = (AccPack.Rec.Flags & ACF_CURRENCY) ? 1 : 0;
	disableCtrls(!is_cur_acc, CTL_ACCOUNT_CURLIST, 0);
	enableCommand(cmaInsert, is_cur_acc);
	enableCommand(cmaDelete, is_cur_acc);
	if(getCtrlView(CTL_ACCOUNT_AUTONUMBER)) {
		v = BIN(AccPack.Rec.Flags & ACF_SYSNUMBER);
		setCtrlUInt16(CTL_ACCOUNT_AUTONUMBER, v);
		disableCtrls(v, CTL_ACCOUNT_NUMBER, CTL_ACCOUNT_SUBNUMBER, 0);
	}
	if(AtObj.VerifyChangingAccsheetOfAccount(AccPack.Rec.ID)) {
		disableCtrl(CTLSEL_ACCOUNT_ACCSHEET, 0);
		setStaticText(CTL_ACCOUNT_ST_ACSMSG, 0);
	}
	else if(DS.CheckExtFlag(ECF_AVERAGE) && PPMaster) {
		SString msg_buf;
		PPLoadText(PPTXT_ACC_ACSUSED, msg_buf);
		setStaticText(CTL_ACCOUNT_ST_ACSMSG, msg_buf);
	}
	else
		disableCtrl(CTLSEL_ACCOUNT_ACCSHEET, 1);
	if(AccPack.Rec.Type == ACY_BUDGET)
		SetupPPObjCombo(this, CTLSEL_ACCOUNT_PARENT,  PPOBJ_ACCOUNT2, AccPack.Rec.ParentID, OLW_CANSELUPLEVEL, reinterpret_cast<void *>(ACY_SEL_BUDGET));
	return 1;
}

int AccountDialog::getDTS(PPAccountPacket * pAccPack)
{
	ushort v;
	getCtrlData(CTL_ACCOUNT_CODE,      AccPack.Rec.Code);
	getCtrlData(CTL_ACCOUNT_NUMBER,    &AccPack.Rec.A.Ac);
	getCtrlData(CTL_ACCOUNT_SUBNUMBER, &AccPack.Rec.A.Sb);
	getCtrlData(CTL_ACCOUNT_NAME,      &AccPack.Rec.Name);
	getCtrlData(CTL_ACCOUNT_OVERDRAFT, &AccPack.Rec.Overdraft);
	getCtrlData(CTL_ACCOUNT_LIMIT,     &AccPack.Rec.Limit);
	v = getCtrlUInt16(CTL_ACCOUNT_TYPE);
	AccPack.Rec.Kind = (v == 0) ? ACT_ACTIVE : ((v == 1) ? ACT_PASSIVE : ((v == 2) ? ACT_AP : ACT_ACTIVE));
	getCtrlData(CTLSEL_ACCOUNT_ACCSHEET, &AccPack.Rec.AccSheetID);
	v = getCtrlUInt16(CTL_ACCOUNT_FLAGS);
	SETFLAG(AccPack.Rec.Flags, ACF_CURRENCY, v & 1);
	v = getCtrlUInt16(CTL_ACCOUNT_AUTONUMBER);
	SETFLAG(AccPack.Rec.Flags, ACF_SYSNUMBER, v & 1);
	if(AccPack.Rec.Type == ACY_BUDGET)
		getCtrlData(CTLSEL_ACCOUNT_PARENT,  &AccPack.Rec.ParentID);

	if(validate()) {
		*pAccPack = AccPack;
		return 1;
	}
	else
		return 0;
}

IMPL_HANDLE_EVENT(AccountDialog)
{
	PPListDialog::handleEvent(event);
	if(event.isClusterClk(CTL_ACCOUNT_FLAGS)) {
		ushort v = getCtrlUInt16(CTL_ACCOUNT_FLAGS);
		int    is_cur_acc = BIN(v & 1);
		if(!is_cur_acc)
			for(uint i = 0; i < AccPack.CurList.getCount(); i++)
				if(!AtObj.VerifyRevokingCurFromAccount(AccPack.Rec.ID, AccPack.CurList.at(i))) {
					PPError();
					is_cur_acc = 1;
					setCtrlUInt16(CTL_ACCOUNT_FLAGS, 1);
					break;
				}
		disableCtrls(!is_cur_acc, CTL_ACCOUNT_CURLIST, 0);
		enableCommand(cmaInsert, is_cur_acc);
		enableCommand(cmaDelete, is_cur_acc);
		updateList(-1);
		clearEvent(event);
	}
	else if(event.isClusterClk(CTL_ACCOUNT_AUTONUMBER)) {
		ushort v = getCtrlUInt16(CTL_ACCOUNT_AUTONUMBER);
		SETFLAG(AccPack.Rec.Flags, ACF_SYSNUMBER, v & 1);
		if(v) {
			getCtrlData(CTL_ACCOUNT_NUMBER, &AccPack.Rec.A.Ac);
			getCtrlData(CTL_ACCOUNT_SUBNUMBER, &AccPack.Rec.A.Sb);
			if(AccPack.Rec.A.Ac == 0) {
				PPObjAccount acc_obj;
				if(acc_obj.GenerateNumber(&AccPack.Rec)) {
					setCtrlData(CTL_ACCOUNT_NUMBER, &AccPack.Rec.A.Ac);
					AccPack.Rec.A.Sb = 0;
					setCtrlData(CTL_ACCOUNT_SUBNUMBER, &AccPack.Rec.A.Sb);
				}
				else
					PPError();
			}
		}
		disableCtrls(v, CTL_ACCOUNT_NUMBER, CTL_ACCOUNT_SUBNUMBER, 0);
		clearEvent(event);
	}
}

int AccountDialog::setupList()
{
	for(uint i = 0; i < AccPack.CurList.getCount(); i++) {
		char   str[48];
		PPCurrency cur_rec;
		PPID   cur_id = AccPack.CurList.at(i);
		if(SearchObject(PPOBJ_CURRENCY, cur_id, &cur_rec) > 0)
			if(*strip(cur_rec.Symb))
				STRNSCPY(str, cur_rec.Symb);
			else
				STRNSCPY(str, cur_rec.Name);
		else
			ltoa(cur_id, str, 10);
		if(!addStringToList(cur_id, str))
			return 0;
	}
	return 1;
}

int AccountDialog::addItem(long *, long * pID)
{
	int    r;
	PPID   cur_id = 0;
	PPIDArray exclude_list;
	PPObjCurrency cur_obj;
	exclude_list.copy(AccPack.CurList);
	if((r = cur_obj.Select(1, 0, &exclude_list, &cur_id)) > 0) {
		*pID = cur_id;
		AccPack.CurList.insert(&cur_id);
		return 1;
	}
	return r ? -1 : 0;
}

int AccountDialog::delItem(long, long id)
{
	int    ok = -1;
	uint   p;
	if(AccPack.CurList.lsearch(id, &p))
		if(AtObj.VerifyRevokingCurFromAccount(AccPack.Rec.ID, id)) {
			AccPack.CurList.atFree(p);
			ok = 1;
		}
		else
			ok = PPErrorZ();
	return ok;
}

// static
int SLAPI PPObjAccount::CheckRecursion(PPID id, PPID parentID)
{
	int    ok = 1;
	if(id) {
		PPObjAccount obj;
		do {
			 if(parentID) {
				PPAccount rec;
				THROW_PP(id != parentID, PPERR_RECURSIONFOUND);
				parentID = (obj.Search(parentID, &rec) > 0) ? rec.ParentID : 0;
			 }
		 } while(parentID);
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjAccount::Edit(PPID * pID, void * extraPtr /*accType*/)
{
	const  int extra_acc_type = reinterpret_cast<int>(extraPtr);
	int    ok = -1, valid_data = 0, is_new = 0;
	int    acc_type = ACY_BAL;
	AccountDialog    * p_bal_dlg = 0;
	GenAccountDialog * p_gen_dlg = 0;
	PPAccountPacket acc_pack;
	THROW(CheckRightsModByID(pID));
	THROW(EditPrereq(pID, 0, &is_new));
	if(!is_new) {
		THROW(GetPacket(*pID, &acc_pack) > 0);
		acc_type = acc_pack.Rec.Type;
	}
	else {
		if(extra_acc_type <= 0) {
			int    r;
			uint   v = BIN(acc_type == ACY_AGGR);
			THROW(r = SelectorDialog(DLG_SELACTYP, CTL_SELACTYP_TYP, &v));
			acc_type = -1;
			if(r > 0)
				switch(v) {
					case 0: acc_pack.Rec.Type = acc_type = ACY_BAL;      break;
					case 1: acc_pack.Rec.Type = acc_type = ACY_OBAL;     break;
					case 2: acc_pack.Rec.Type = acc_type = ACY_REGISTER; break;
					case 3: acc_pack.Rec.Type = acc_type = ACY_AGGR;     break;
					case 4: acc_pack.Rec.Type = acc_type = ACY_ALIAS;    break;
					case 5: acc_pack.Rec.Type = acc_type = ACY_BUDGET;   break;
				}
		}
		else
			acc_pack.Rec.Type = acc_type = extra_acc_type;
		if(oneof2(acc_pack.Rec.Type, ACY_REGISTER, ACY_BUDGET)) {
			acc_pack.Rec.Flags |= ACF_SYSNUMBER;
			THROW(GenerateNumber(&acc_pack.Rec));
		}
	}
	if(oneof5(acc_type, ACY_BAL, ACY_OBAL, ACY_REGISTER, ACY_ALIAS, ACY_BUDGET)) {
		SString dlg_title;
		uint   dlg_id = 0;
		if(acc_type == ACY_BAL)
			dlg_id = DLG_ACCOUNT;
		else if(acc_type == ACY_ALIAS)
			dlg_id = DLG_ACCALIAS;
		else if(acc_type == ACY_BUDGET)
			dlg_id = DLG_ACCBUDGET;
		else
			dlg_id = DLG_ACCREGISTER;
		THROW(CheckDialogPtr(&(p_bal_dlg = new AccountDialog(dlg_id))));
		THROW(EditPrereq(pID, p_bal_dlg, 0));
		PPGetSubStr(PPTXT_ACCTITLES, acc_type, dlg_title);
		p_bal_dlg->setTitle(dlg_title);
		p_bal_dlg->setDTS(&acc_pack);
		while(!valid_data && ExecView(p_bal_dlg) == cmOK) {
			int16  ac = acc_pack.Rec.A.Ac;
			int16  sb = acc_pack.Rec.A.Sb;
			valid_data = p_bal_dlg->getDTS(&acc_pack);
			if(valid_data) {
				int    num_changed = 0;
				if(*pID)
					num_changed = (ac != acc_pack.Rec.A.Ac || sb != acc_pack.Rec.A.Sb);
				if(num_changed) {
					Exchange(&acc_pack.Rec.A.Ac, &ac);
					Exchange(&acc_pack.Rec.A.Sb, &sb);
				}
				THROW(PutPacket(pID, &acc_pack, 1));
				if(num_changed) {
					PPObjAccTurn atobj;
					THROW(atobj.P_Tbl->UpdateAccNum(*pID, ac, sb, 1));
				}
				Dirty(*pID); // @v9.0.4
				ok = 1;
			}
		}
	}
	else if(acc_type == ACY_AGGR) {
		THROW(CheckDialogPtr(&(p_gen_dlg = new GenAccountDialog())));
		THROW(EditPrereq(pID, p_gen_dlg, 0));
		p_gen_dlg->setDTS(&acc_pack);
		while(!valid_data && ExecView(p_gen_dlg) == cmOK)
			if((valid_data = p_gen_dlg->getDTS(&acc_pack)) != 0) {
				THROW(PutPacket(pID, &acc_pack, 1));
				Dirty(*pID); // @v9.0.4
				ok = 1;
			}
	}
	CATCHZOKPPERR
	delete p_bal_dlg;
	delete p_gen_dlg;
	return (ok > 0) ? cmOK : ((ok < 0) ? cmCancel : 0);
}

int SLAPI PPObjAccount::HandleMsg(int msg, PPID _obj, PPID _id, void * extraPtr)
{
	PPAccount rec;
	if(msg == DBMSG_OBJDELETE) {
		if(_obj == PPOBJ_ACCSHEET) {
			SEnum en = EnumByIdxVal(2, _id); 
			if(en.Next(&rec) > 0)
				return RetRefsExistsErr(Obj, rec.ID);
		}
		else if(_obj == PPOBJ_CURRENCY) {
			for(SEnum en = Enum(0); en.Next(&rec) > 0;) {
				if(rec.CurID == _id) {
					return RetRefsExistsErr(Obj, rec.ID);
				}
			}
		}
	}
	return DBRPL_OK;
}

IMPL_DESTROY_OBJ_PACK(PPObjAccount, PPAccountPacket);

int SLAPI PPObjAccount::SerializePacket(int dir, PPAccountPacket * pPack, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	THROW_SL(ref->SerializeRecord(dir, &pPack->Rec, rBuf, pSCtx));
	THROW_SL(pSCtx->Serialize(dir, &pPack->CurList, rBuf));
	THROW_SL(pSCtx->Serialize(dir, &pPack->GenList, rBuf));
	CATCHZOK
	return ok;
}

int SLAPI PPObjAccount::Write(PPObjPack * p, PPID * pID, void * stream, ObjTransmContext * pCtx) // @srlz
{
	int    ok = 1;
	PPAccountPacket * p_pack = 0;
	THROW(p && p->Data);
	p_pack = static_cast<PPAccountPacket *>(p->Data);
	if(stream == 0) {
		if(*pID == 0) {
			PPAccount same_rec;
			const  int16 org_ac = p_pack->Rec.A.Ac;
			const  int16 org_sb = p_pack->Rec.A.Sb;
			int    r = SearchNum(p_pack->Rec.A.Ac, p_pack->Rec.A.Sb, p_pack->Rec.CurID, &same_rec);
			if(r > 0 /* @v9.2.10 && (same_rec.AccSheetID == p_pack->Rec.AccSheetID || (p->Flags & PPObjPack::fDispatcher)) */) {
				*pID = same_rec.ID;
			}
			else {
				if(r > 0) { // @v9.2.7 @fix(r-->(r>0)) Счет в разделе найден, но с ним ассоциирована иная таблица статей: придется присвоить счету другой номер.
					for(int i = 1; i < 100; i++) {
						if(SearchNum(p_pack->Rec.A.Ac, i, 0L/*@curID*/) < 0) {
							p_pack->Rec.A.Sb = i;
							break;
						}
					}
					{
						// PPTXT_LOG_ACCEPTACCSAMENO      "Акцептируемый счет @zstr найден по номеру, но имеет иную таблицу статей - номер изменен на @zstr"
						SString msg_buf;
						PPFormatT(PPTXT_LOG_ACCEPTACCSAMENO, &msg_buf, (int)org_ac, (int)org_sb, (int)p_pack->Rec.A.Ac, (int)p_pack->Rec.A.Sb);
						pCtx->Output(msg_buf);
					}

				}
				p_pack->Rec.ID   = 0;
				p_pack->Rec.Type = ACY_BAL;
				if(!PutPacket(pID, p_pack, 1)) {
					pCtx->OutputAcceptErrMsg(PPTXT_ERRACCEPTACCOUNT, p_pack->Rec.ID, p_pack->Rec.Code);
					ok = -1;
				}
			}
		}
		else if(!PutPacket(pID, p_pack, 1)) {
			pCtx->OutputAcceptErrMsg(PPTXT_ERRACCEPTACCOUNT, p_pack->Rec.ID, p_pack->Rec.Code);
			ok = -1;
		}
	}
	else {
		SBuffer buffer;
		THROW(SerializePacket(+1, p_pack, buffer, &pCtx->SCtx));
		THROW_SL(buffer.WriteToFile(static_cast<FILE *>(stream), 0, 0))
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjAccount::Read(PPObjPack * p, PPID id, void * stream, ObjTransmContext * pCtx)
	{ return Implement_ObjReadPacket<PPObjAccount, PPAccountPacket>(this, p, id, stream, pCtx); }

int SLAPI PPObjAccount::ProcessObjRefs(PPObjPack * p, PPObjIDArray * ary, int replace, ObjTransmContext * pCtx)
{
	int    ok = 1;
	if(p && p->Data) {
		uint i;
		PPAccountPacket * p_pack = static_cast<PPAccountPacket *>(p->Data);
		for(i = 0; i < p_pack->CurList.getCount(); i++) {
			PPID & r_cur_id = p_pack->CurList.at(i);
			THROW(ProcessObjRefInArray(PPOBJ_CURRENCY, &r_cur_id, ary, replace));
		}
		for(i = 0; i < p_pack->GenList.getCount(); i++) {
			ObjRestrictItem & r_item = p_pack->GenList.at(i);
			THROW(ProcessObjRefInArray(PPOBJ_ACCOUNT2, &r_item.ObjID, ary, replace));
		}
		THROW(ProcessObjRefInArray(PPOBJ_ACCSHEET, &p_pack->Rec.AccSheetID, ary, replace));
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}
//
//
//
class AccountCache : public ObjCache {
public:
	SLAPI  AccountCache() : ObjCache(PPOBJ_ACCOUNT2, sizeof(Data))
	{
	}
	int    SLAPI FetchNum(int ac, int sb, PPID curID, PPAccount * pRec);
private:
	virtual int  SLAPI FetchEntry(PPID, ObjCacheEntry * pEntry, long);
	virtual void SLAPI EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const;
public:
	struct Data : public ObjCacheEntry {
		int16  Ac;
		int16  Sb;
		long   CurID;
		long   AccSheetID;
		LDATE  OpenDate;
		int16  Type;
		int16  Kind;
		long   Flags;
		double Limit;
		double Overdraft;
	};
};

IMPL_CMPFUNC(AccCacheNum, i1, i2) { RET_CMPCASCADE3(static_cast<const AccountCache::Data *>(i1), static_cast<const AccountCache::Data *>(i2), Ac, Sb, CurID); }

int SLAPI AccountCache::FetchNum(int ac, int sb, PPID curID, PPAccount * pRec)
{
	int    ok = -1;
	AccountCache::Data key;
	key.Ac = ac;
	key.Sb = sb;
	key.CurID = curID;
	if(GetBySrchFunc(&key, PTR_CMPFUNC(AccCacheNum), pRec) > 0) {
		ok = 1;
	}
	else {
		PPObjAccount acc_obj;
		PPAccount acc_rec;
		ok = acc_obj.SearchNum(ac, sb, curID, &acc_rec);
		if(ok > 0) {
			//
			// Если запись в БД найдена, то добавляем ее в кэш и уже из кэша
			// получаем результат для вызывающей функции.
			// Это несколько замедляет первое обращение, но зато в дальнейшем
			// эта запись будет в кэше и в следующий раз мы ее получим очень быстро.
			//
			ok = Get(acc_rec.ID, pRec);
		}
	}
	return ok;
}

int SLAPI AccountCache::FetchEntry(PPID id, ObjCacheEntry * pEntry, long)
{
	int    ok = 1;
	Data * p_cache_rec = static_cast<Data *>(pEntry);
	PPObjAccount acc_obj;
	PPAccount rec;
	if(acc_obj.Search(id, &rec) > 0) {
#define CPY_FLD(Fld) p_cache_rec->Fld=rec.Fld
		CPY_FLD(CurID);
		CPY_FLD(AccSheetID);
		CPY_FLD(OpenDate);
		CPY_FLD(Type);
		CPY_FLD(Kind);
		CPY_FLD(Flags);
		CPY_FLD(Limit);
		CPY_FLD(Overdraft);
#undef CPY_FLD
		p_cache_rec->Ac = rec.A.Ac;
		p_cache_rec->Sb = rec.A.Sb;
		PPStringSetSCD ss;
		ss.add(rec.Code);
		ss.add(rec.Name);
		PutName(ss.getBuf(), p_cache_rec);
	}
	else
		ok = -1;
	return ok;
}

void SLAPI AccountCache::EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const
{
	PPAccount * p_data_rec = static_cast<PPAccount *>(pDataRec);
	const Data * p_cache_rec = static_cast<const Data *>(pEntry);
#define CPY_FLD(Fld) p_data_rec->Fld=p_cache_rec->Fld
	CPY_FLD(ID);
	CPY_FLD(CurID);
	CPY_FLD(AccSheetID);
	CPY_FLD(OpenDate);
	CPY_FLD(Type);
	CPY_FLD(Kind);
	CPY_FLD(Flags);
	CPY_FLD(Limit);
	CPY_FLD(Overdraft);
#undef CPY_FLD
	p_data_rec->A.Ac = p_cache_rec->Ac;
	p_data_rec->A.Sb = p_cache_rec->Sb;
	char   temp_buf[2048];
	GetName(pEntry, temp_buf, sizeof(temp_buf));
	PPStringSetSCD ss;
	ss.setBuf(temp_buf, sstrlen(temp_buf)+1);
	uint   p = 0;
	ss.get(&p, p_data_rec->Code, sizeof(p_data_rec->Code));
	ss.get(&p, p_data_rec->Name, sizeof(p_data_rec->Name));
}

IMPL_OBJ_FETCH(PPObjAccount, PPAccount, AccountCache);
IMPL_OBJ_DIRTY(PPObjAccount, AccountCache);

int SLAPI PPObjAccount::FetchNum(int ac, int sb, PPID curID, PPAccount * pRec)
{
	AccountCache * p_cache = GetDbLocalCachePtr <AccountCache> (Obj);
	return p_cache ? p_cache->FetchNum(ac, sb, curID, pRec) : SearchNum(ac, sb, curID, pRec);
}

int SLAPI PPObjAccount::GetChildList(PPID parentID, StrAssocArray * pChildList)
{
	int    ok = -1;
	StrAssocArray * p_list = MakeStrAssocList(reinterpret_cast<void *>(ACY_SEL_BUDGET));
	if(p_list) {
		for(uint i = 0; i < p_list->getCount(); i++) {
			StrAssocArray::Item item = p_list->Get(i);
			if(BelongTo(item.Id, parentID) > 0) {
				CALLPTRMEMB(pChildList, Add(item.Id, item.ParentId, item.Txt));
				ok = 1;
			}
		}
	}
	ZDELETE(p_list);
	return ok;
}

int SLAPI PPObjAccount::GetParentList(PPID acctID, StrAssocArray * pParentList)
{
	int    ok = -1;
	StrAssocArray * p_list = MakeStrAssocList(reinterpret_cast<void *>(ACY_SEL_BUDGET));
	if(p_list) {
		for(uint i = 0; i < p_list->getCount(); i++) {
			StrAssocArray::Item item = p_list->Get(i);
			if(BelongTo(acctID, item.Id) > 0) {
				CALLPTRMEMB(pParentList, Add(item.Id, item.ParentId, item.Txt));
				ok = 1;
			}
		}
	}
	ZDELETE(p_list);
	return ok;
}

int SLAPI PPObjAccount::BelongTo(PPID acctID, PPID parentID)
{
	int    ok = -1;
	PPAccount rec;
	rec.ID = acctID;
	while(ok < 0 && rec.ID && Search(rec.ID, &rec) > 0 && rec.ParentID) {
		if(rec.ParentID == parentID)
			ok = 1;
		else
			rec.ID = rec.ParentID;
	}
	return ok;
}
