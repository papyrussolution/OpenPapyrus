// OBJBACCT.CPP
// Copyright (c) A.Sobolev 1996, 1997-2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2015, 2016
// @codepage windows-1251
// Объект, управляющий банковскими счетами
//
#include <pp.h>
#pragma hdrstop

#if 0 // @v9.0.4 {

//
// @ModuleDef(PPObjBnkAcct)
//
TLP_IMPL(PPObjBnkAcct, BankAccountTbl, P_Tbl); // BankAccountCore-->BankAccountTbl

SLAPI PPObjBnkAcct::PPObjBnkAcct(void * extraPtr) : PPObject(PPOBJ_BACCT)
{
	TLP_OPEN(P_Tbl);
	ExtraPtr = extraPtr;
	ImplementFlags |= implStrAssocMakeList;
}

SLAPI PPObjBnkAcct::~PPObjBnkAcct()
{
	TLP_CLOSE(P_Tbl);
}

int SLAPI PPObjBnkAcct::Search(PPID id, void * b)
{
	//return P_Tbl->Search(id, (BankAccountTbl::Rec *)b);
	return SearchByID(P_Tbl, PPOBJ_BACCT, id, b);
}

int SLAPI PPObjBnkAcct::FetchList(PPID personID, BnkAcctArray * pList)
{
	BankAccountTbl::Key1 k1;
	BExtQuery q(P_Tbl, 1);
	q.selectAll().where(P_Tbl->PersonID == personID);
	MEMSZERO(k1);
	for(q.initIteration(0, &k1, spGe); q.nextIteration() > 0;)
		if(P_Tbl->data.Flags & BACCTF_PREFERRED) {
			if(!pList->atInsert(0, &P_Tbl->data))
				return PPSetErrorSLib();
		}
		else if(!pList->insert(&P_Tbl->data))
			return PPSetErrorSLib();
	return 1;
}

int SLAPI PPObjBnkAcct::Enum(PPID personID, PPID * pBankID, char * pAcct)
{
	int    ok = -1;
	BankAccountTbl::Key1 k;
	k.PersonID = personID;
	k.BankID   = *pBankID;
	STRNSCPY(k.Acct, pAcct);
   	if(P_Tbl->search(1, &k, spGt) && k.PersonID == personID) {
		*pBankID = k.BankID;
		strnzcpy(pAcct, k.Acct, sizeof(k.Acct));
		ok = 1;
	}
	else
		ok = PPDbSearchError();
	return ok;
}

int SLAPI BnkAcctArray_RemoveDup(BnkAcctArray * pList)
{
	BankAccountTbl::Rec * p_item, * p_item2;
	for(uint i = 0; pList->enumItems(&i, (void **)&p_item);) {
		for(uint j = i; pList->enumItems(&j, (void **)&p_item2);) {
			if(j != i && p_item->PersonID == p_item2->PersonID &&
				p_item->BankID == p_item2->BankID && stricmp866(p_item->Acct, p_item2->Acct) == 0) {
				pList->atFree(--j);
			}
		}
	}
	return 1;
}

int SLAPI PPObjBnkAcct::UpdateList(PPID personID, BnkAcctArray * pList, int use_ta)
{
	int    ok = 1;
	PPID   bank = 0;
	char   acc[64];
	uint   i;
	BankAccountTbl::Rec * rec;
	BnkAcctArray_RemoveDup(pList);
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		//
		// Вычищаем из БД все удаленные записи
		//
		acc[0] = 0;
		while(Enum(personID, &bank, acc) > 0) {
			int    found = 0;
			for(i = 0; !found && pList->enumItems(&i, (void**)&rec);)
				if(rec->ID == P_Tbl->data.ID)
					found = 1;
				/*
				else if(strcmp(rec->Acct, data.Acct) == 0) {
					rec->ID = data.ID;
					found = 1;
				}
				*/
			if(!found)
				THROW_DB(P_Tbl->deleteRec());
		}
		//
		// Изменяем и вставляем исправленные и добавленные записи
		//
		for(i = 0; pList->enumItems(&i, (void**)&rec);) {
			rec->PersonID = personID;
			BankAccountTbl::Rec item;
			if(Search(rec->ID, &item) > 0) {
				if(memcmp(&item, rec, sizeof(item)) != 0)
					THROW_DB(P_Tbl->updateRecBuf(rec));
			}
			else
				THROW_DB(P_Tbl->insertRecBuf(rec));
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjBnkAcct::DeleteObj(PPID)
{
	return P_Tbl->deleteRec() ? 1 : PPSetErrorDB();
}

//static
int SLAPI PPObjBnkAcct::EditRecord(BankAccountTbl::Rec * pRec, PPID psnKindID)
{
	class BnkAcctDialog : public TDialog {
	public:
		BnkAcctDialog() : TDialog(DLG_BACCT)
		{
			ValidAcc = -1;
			SetupCalDate(CTLCAL_BACCT_OPENDATE, CTL_BACCT_OPENDATE);
			Ptb.SetBrush(brushValidNumber,   SPaintObj::bsSolid, GetColorRef(SClrAqua),  0);
			Ptb.SetBrush(brushInvalidNumber, SPaintObj::bsSolid, GetColorRef(SClrCoral), 0);
		}
		void   SetupBIC()
		{
			BIC = 0;
			PPID   bank_id = getCtrlLong(CTLSEL_BACCT_BANK);
			if(bank_id) {
				PPObjPerson psn_obj;
				RegisterTbl::Rec reg_rec;
				if(psn_obj.GetRegister(bank_id, PPREGT_BIC, &reg_rec) > 0)
					BIC = reg_rec.Num;
			}
			setCtrlString(CTL_BACCT_BIC, BIC);
			SString data_buf;
			getCtrlString(CTL_BACCT_ACCT, data_buf);
			ValidAcc = BIN(CheckBnkAcc(data_buf, BIC));
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isCbSelected(CTLSEL_BACCT_BANK)) {
				SetupBIC();
			}
			else if(event.isCmd(cmInputUpdated) && event.isCtlEvent(CTL_BACCT_ACCT)) {
				SString data_buf, temp_buf;
				getCtrlString(CTL_BACCT_ACCT, data_buf);
				setStaticText(CTL_BACCT_ACCTLEN, temp_buf.Cat(data_buf.Strip().Len()));
				int    prev_valid = ValidAcc;
				ValidAcc = BIN(CheckBnkAcc(data_buf, BIC));
				if(ValidAcc != prev_valid)
					drawCtrl(CTL_BACCT_ACCT);
			}
			else if(TVCMD == cmCtlColor) {
				TDrawCtrlData * p_dc = (TDrawCtrlData *)TVINFOPTR;
				if(p_dc && ValidAcc >= 0 && getCtrlHandle(CTL_BACCT_ACCT) == p_dc->H_Ctl) {
					if(ValidAcc > 0) {
						::SetBkMode(p_dc->H_DC, TRANSPARENT);
						p_dc->H_Br = (HBRUSH)Ptb.Get(brushValidNumber);
					}
					else {
						::SetBkMode(p_dc->H_DC, TRANSPARENT);
						p_dc->H_Br = (HBRUSH)Ptb.Get(brushInvalidNumber);
					}
				}
				else
					return;
			}
			else
				return;
			clearEvent(event);
		}
		enum {
			dummyFirst = 1,
			brushValidNumber,
			brushInvalidNumber
		};
		int    ValidAcc;
		SString BIC;
		SPaintToolBox Ptb;
	};
	int    ok = -1, valid_data = 0;
	BankAccountTbl::Rec rec = *pRec;
	BnkAcctDialog * dlg = new BnkAcctDialog();
	if(CheckDialogPtr(&dlg, 1)) {
		SetupPersonCombo(dlg, CTLSEL_BACCT_BANK, rec.BankID, OLW_CANINSERT, (PPID)PPPRK_BANK, 0);
		SetupPPObjCombo(dlg, CTLSEL_BACCT_ACCTYPE, PPOBJ_BNKACCTYPE, rec.AccType, OLW_CANINSERT, 0);
		dlg->setCtrlData(CTL_BACCT_ACCT,     rec.Acct);
		dlg->setCtrlData(CTL_BACCT_OPENDATE, &rec.OpenDate);
		dlg->AddClusterAssoc(CTL_BACCT_FLAGS, 0, BACCTF_PREFERRED);
		dlg->SetClusterData(CTL_BACCT_FLAGS, rec.Flags);
		dlg->SetupBIC();
		{
			PPObjBnkAcct ba_obj;
			if(!ba_obj.CheckRightsModByID(&rec.ID))
				DisableOKButton(dlg);
		}
		while(!valid_data && ExecView(dlg) == cmOK) {
			dlg->getCtrlData(CTLSEL_BACCT_BANK,    &rec.BankID);
			dlg->getCtrlData(CTLSEL_BACCT_ACCTYPE, &rec.AccType);
			dlg->getCtrlData(CTL_BACCT_ACCT,       rec.Acct);
			dlg->getCtrlData(CTL_BACCT_OPENDATE,   &rec.OpenDate);
			dlg->GetClusterData(CTL_BACCT_FLAGS,   &rec.Flags);
			if(rec.BankID == 0)
				PPErrorByDialog(dlg, CTL_BACCT_BANK, PPERR_BANKNEEDED);
			else if(rec.AccType == 0)
				PPErrorByDialog(dlg, CTL_BACCT_ACCTYPE, PPERR_BACCTYPENEEDED);
			else if((rec.AccType == PPBAC_NOSTRO || rec.AccType == PPBAC_LORO) && psnKindID && psnKindID != PPPRK_BANK)
				PPErrorByDialog(dlg, CTL_BACCT_ACCTYPE, PPERR_INCOMPBACCTYPE);
			else {
				*pRec = rec;
				ok = valid_data = 1;
			}
		}
	}
	else
		ok = 0;
	delete dlg;
	return ok;
}

int SLAPI PPObjBnkAcct::Edit(PPID * pID, void * extraPtr /*personID*/)
{
	const  PPID extra_person_id = (PPID)extraPtr;
	int    ok = -1;
	if(extra_person_id > 0) {
		BankAccountTbl::Rec rec;
		if(*pID) {
			THROW(Search(*pID, &rec) > 0);
			THROW(ok = EditRecord(&rec, 0));
			if(ok > 0)
				THROW(UpdateByID(P_Tbl, Obj, *pID, &rec, 1));
		}
		else {
			MEMSZERO(rec);
			rec.PersonID = extra_person_id;
			rec.AccType  = PPBAC_CURRENT;
			ok = EditRecord(&rec, 0);
			if(ok > 0)
				THROW(AddByID(P_Tbl, pID, &rec, 1));
		}
	}
	CATCHZOKPPERR
	if(ok > 0)
		ok = cmOK;
	else if(ok < 0)
		ok = cmCancel;
	return ok;
}

StrAssocArray * SLAPI PPObjBnkAcct::MakeStrAssocList(void * extraPtr /*personID*/)
{
	const  PPID person_id = (PPID)extraPtr;
	StrAssocArray * p_list = new StrAssocArray;
	uint   i;
	BnkAcctArray bacc_list;
	THROW_MEM(p_list);
	THROW(FetchList(person_id, &bacc_list));
	for(i = 0; i < bacc_list.getCount(); i++)
		THROW_SL(p_list->Add(bacc_list.at(i).ID, bacc_list.at(i).Acct));
	CATCH
		ZDELETE(p_list);
	ENDCATCH
	return p_list;
}

int SLAPI PPObjBnkAcct::Browse(void * extraPtr /*personID*/)
{
	const  PPID person_id = (PPID)extraPtr;
	int    ok = 1;
	BnkAcctArray ary;
	THROW(FetchList(person_id, &ary));
	if(EditList(person_id, &ary) == cmOK) {
		THROW(UpdateList(person_id, &ary, 1));
	}
	CATCHZOKPPERR
	return ok;
}

int SLAPI PPObjBnkAcct::HandleMsg(int msg, PPID _obj, PPID _id, void * extraPtr)
{
	int    ok = DBRPL_OK;
	PPID   id;
	if(msg == DBMSG_OBJDELETE) {
		switch(_obj) {
			case PPOBJ_PERSON:
				for(id = 0; ok == DBRPL_OK && P_Tbl->search(0, &id, spGt);) {
					if(P_Tbl->data.BankID == _id) {
						ok = RetRefsExistsErr(Obj, P_Tbl->data.ID);
					}
					else if(P_Tbl->data.PersonID == _id)
						THROW_DB(P_Tbl->deleteRec());
				}
				break;
			case PPOBJ_BNKACCTYPE:
				id = 0;
				BExtQuery q(P_Tbl, 0);
				q.select(P_Tbl->ID, 0L).where(P_Tbl->AccType == _id);
				if(q.fetchFirst(&id, spFirst) > 0) {
					ok = RetRefsExistsErr(Obj, P_Tbl->data.ID);
				}
				break;
		}
	}
	else if(msg == DBMSG_OBJREPLACE)
		if(_obj == PPOBJ_PERSON) {
			for(PPID id = 0; P_Tbl->search(0, &id, spGt);) {
				if(P_Tbl->data.PersonID == _id) {
					THROW_DB(P_Tbl->deleteRec());
				}
				if(P_Tbl->data.BankID == _id) {
					P_Tbl->data.BankID = extraParam;
					THROW_DB(P_Tbl->updateRec());
				}
			}
		}
	CATCHZOK
	return ok;
}
//
// BnkAccListDialog
//
class BnkAccListDialog : public PPListDialog {
public:
	BnkAccListDialog(char * pPsnName) : PPListDialog(DLG_BACCLST, CTL_BACCLST_LIST)
	{
		setStaticText(CTL_BACCLST_NAME, pPsnName);
	}
	int  setDTS(const BnkAcctArray * pData)
	{
		int    ok = 1;
		if(Data.copy(*pData))
			updateList(0);
		else
			ok = PPSetErrorSLib();
		return ok;
	}
	int  getDTS(BnkAcctArray * pData)
	{
		return pData->copy(Data) ? 1 : PPSetErrorSLib();
	}
private:
	virtual int  setupList()
	{
		SString sub;
		BankAccountTbl::Rec * p_rec;
		for(uint i = 0; Data.enumItems(&i, (void**)&p_rec);) {
			StringSet ss(SLBColumnDelim);
			sub = 0;
			GetPersonName(p_rec->BankID, sub);
			ss.add(sub);
			ss.add(p_rec->Acct);
			if(!addStringToList(i, ss.getBuf()))
				return 0;
		}
		return 1;
	}
	virtual int  addItem(long *, long * pID)
	{
		int    ok = -1, valid_data = 0;
		PPObjBnkAcct ba_obj;
		if(ba_obj.CheckRights(PPR_INS)) {
			BankAccountTbl::Rec rec;
			PersonTbl::Rec psn_rec;
			MEMSZERO(rec);
			MEMSZERO(psn_rec);
			rec.AccType = PPBAC_CURRENT;
			while(!valid_data && (ok = PPObjBnkAcct::EditRecord(&rec, 0)) > 0) {
				if(PPObjBnkAcct::CheckDuplicateBnkAcct(&rec, &Data, -1)) {
					valid_data = 1;
					if(!Data.insert(&rec))
						ok = PPSetErrorSLib();
					else {
						ASSIGN_PTR(pID, rec.ID);
						if(rec.Flags & BACCTF_PREFERRED)
							for(uint i = 0; i < Data.getCount()-1; i++)
								Data.at(i).Flags &= ~BACCTF_PREFERRED;
					}
				}
				else
					ok = 0;
				if(!ok)
					PPError();
			}
		}
		else
			ok = PPErrorZ();
		return ok;
	}
	virtual int  editItem(long pos, long)
	{
		int    ok = -1, valid_data = 0;
		if(pos >= 0 && pos < (long)Data.getCount()) {
			BankAccountTbl::Rec item = Data.at((uint)pos);
			while(!valid_data && (ok = PPObjBnkAcct::EditRecord(&item, 0)) > 0) {
				if(PPObjBnkAcct::CheckDuplicateBnkAcct(&item, &Data, pos)) {
					valid_data = 1;
					if(item.Flags & BACCTF_PREFERRED)
						for(uint i = 0; i < Data.getCount(); i++)
							if(i != (uint)pos)
								Data.at(i).Flags &= ~BACCTF_PREFERRED;
					Data.at((uint)pos) = item;
				}
				else
					ok = PPErrorZ();
			}
		}
		return ok;
	}
	virtual int  delItem(long pos, long)
	{
		if(pos >= 0 && pos < (long)Data.getCount()) {
			if(Data.at((uint)pos).ID) {
				PPObjBnkAcct ba_obj;
				if(!ba_obj.CheckRights(PPR_DEL))
					return PPErrorZ();
			}
			if(CONFIRM(PPCFM_DELETE)) {
				Data.atFree((uint)pos);
				return 1;
			}
		}
		return -1;
	}

	PPObjPerson  psnobj;
	BnkAcctArray Data;
};

int SLAPI PPObjBnkAcct::EditList(PPID personID, BnkAcctArray * ary)
{
	int    r = cmCancel, valid_data = 0;
	BnkAccListDialog * dlg = 0;
	PPObjPerson psnobj;
	if(personID) {
		THROW(psnobj.Search(personID) > 0);
	}
	dlg = new BnkAccListDialog(personID ? psnobj.P_Tbl->data.Name : 0);
	THROW(CheckDialogPtr(&dlg));
	THROW(dlg->setDTS(ary));
	while(!valid_data && (r = ExecView(dlg)) == cmOK)
		if(dlg->getDTS(ary))
			valid_data = 1;
	CATCH
		r = PPErrorZ();
	ENDCATCH
	delete dlg;
	return r;
}

// static
int SLAPI PPObjBnkAcct::CheckDuplicateBnkAcct(const BankAccountTbl::Rec * pRec, const BnkAcctArray * pAry, long pos)
{
	int    ok = 1;
	for(uint i = 0; i < pAry->getCount(); i++) {
		BankAccountTbl::Rec & r_rec = pAry->at(i);
		if(!stricmp(pRec->Acct, r_rec.Acct) && pRec->BankID == r_rec.BankID && (uint)pos != i) {
			ok = PPSetError(PPERR_DUPLBNKACCT);
			break;
		}
	}
	return ok;
}

#endif // } 0 @v9.0.4
