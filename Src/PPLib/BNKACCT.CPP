// BNKACCT.CPP
// Copyright (c) A.Sobolev 1998, 1999, 2000, 2001, 2004, 2005, 2007, 2008, 2015, 2016
// @codepage windows-1251
// @Kernel
//
#include <pp.h>
#pragma hdrstop

#if 0 // @v9.0.4 {

SLAPI BankAccountCore::BankAccountCore() : BankAccountTbl()
{
}

int SLAPI BankAccountCore::Search_(PPID id, BankAccountTbl::Rec * pRec)
{
	return SearchByID(this, PPOBJ_BACCT, id, pRec);
}

int SLAPI BankAccountCore::Enum_(PPID personID, PPID * pBankID, char * pAcct)
{
	BankAccountTbl::Key1 k;
	k.PersonID = personID;
	k.BankID   = *pBankID;
	STRNSCPY(k.Acct, pAcct);
   	if(search(1, &k, spGt) && k.PersonID == personID) {
		*pBankID = k.BankID;
		strnzcpy(pAcct, k.Acct, sizeof(k.Acct));
		return 1;
	}
	return PPDbSearchError();
}

int SLAPI BankAccountCore::FetchList_(PPID personID, BnkAcctArray * pList)
{
	BankAccountTbl::Key1 k1;
	BExtQuery q(this, 1);
	q.selectAll().where(this->PersonID == personID);
	MEMSZERO(k1);
	for(q.initIteration(0, &k1, spGe); q.nextIteration() > 0;)
		if(data.Flags & BACCTF_PREFERRED) {
			if(!pList->atInsert(0, &data))
				return PPSetErrorSLib();
		}
		else if(!pList->insert(&data))
			return PPSetErrorSLib();
	return 1;
}

int SLAPI BnkAcctArray_RemoveDup(BnkAcctArray * pList);

int SLAPI BankAccountCore::UpdateList_(PPID personID, BnkAcctArray * pList, int use_ta)
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
		while(Enum_(personID, &bank, acc) > 0) {
			int    found = 0;
			for(i = 0; !found && pList->enumItems(&i, (void **)&rec);)
				if(rec->ID == data.ID)
					found = 1;
				/*
				else if(strcmp(rec->Acct, data.Acct) == 0) {
					rec->ID = data.ID;
					found = 1;
				}
				*/
			if(!found)
				THROW_DB(deleteRec());
		}
		//
		// Изменяем и вставляем исправленные и добавленные записи
		//
		for(i = 0; pList->enumItems(&i, (void **)&rec);) {
			rec->PersonID = personID;
			if(Search_(rec->ID) > 0) {
				if(memcmp(&data, rec, sizeof(data)) != 0)
					THROW_DB(updateRecBuf(rec));
			}
			else
				THROW_DB(insertRecBuf(rec));
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI BankAccountCore::RemoveList_(PPID personID, int use_ta)
{
	return deleteFrom(this, use_ta, (this->PersonID == personID)) ? 1 : PPSetErrorDB();
}

#endif // } 0 @v9.0.4
