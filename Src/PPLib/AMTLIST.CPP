// AMTLIST.CPP
// Copyright (c) A.Sobolev 2000, 2001, 2002, 2004, 2005, 2007, 2011, 2012, 2014, 2015, 2016, 2017
// @Kernel
//
#include <pp.h>
#pragma hdrstop

SLAPI AmtEntry::AmtEntry()
{
	AmtTypeID = 0;
	CurID = 0;
	Amt = 0.0;
}

SLAPI AmtEntry::AmtEntry(PPID amtTypeID)
{
	AmtTypeID = amtTypeID;
	CurID = 0;
	Amt = 0.0;
}

SLAPI AmtEntry::AmtEntry(PPID amtTypeID, PPID curID)
{
	AmtTypeID = amtTypeID;
	CurID = curID;
	Amt = 0.0;
}

SLAPI AmtEntry::AmtEntry(PPID amtTypeID, PPID curID, double amt)
{
	AmtTypeID = amtTypeID;
	CurID = curID;
	Amt = amt;
}

int FASTCALL AmtEntry::IsEqual(const AmtEntry & rS) const
{
	return BIN(AmtTypeID == rS.AmtTypeID && CurID == rS.CurID && Amt == rS.Amt);
}

SLAPI AmtList::AmtList() : TSVector <AmtEntry> () // @v9.8.4 TSArray-->TSVector
{
}

AmtList & FASTCALL AmtList::operator = (const AmtList & s)
{
	SVector::copy(s);
	return *this;
}

int FASTCALL AmtList::HasAmtTypeID(PPID amtTypeID) const
{
	return lsearch(&amtTypeID, 0, CMPF_LONG) ? 1 : 0;
}

int FASTCALL AmtList::HasVatSum(const TaxAmountIDs * pTai) const
{
	int    yes = 0;
	if(pTai) {
		AmtEntry * p_ae;
		for(uint i = 0; !yes && enumItems(&i, (void**)&p_ae);)
			if(p_ae->AmtTypeID && p_ae->Amt != 0.0 &&
				oneof3(p_ae->AmtTypeID, pTai->VatAmtID[0], pTai->VatAmtID[1], pTai->VatAmtID[2]))
				yes = 1;
	}
	return yes;
}

int FASTCALL AmtList::IsEqual(const AmtList * pS) const
{
	int    ok = 1;
	if(getCount() == pS->getCount()) {
		AmtEntry * p_e1, * p_e2;
		LongArray saw_list;
		for(uint i = 0; ok && enumItems(&i, (void **)&p_e1);) {
			int found = 0;
			for(uint j = 0; !found && pS->enumItems(&j, (void **)&p_e2);) {
				long   p = (long)j;
				if(!saw_list.lsearch(p) && p_e1->IsEqual(*p_e2)) {
					saw_list.add(p);
					found = 1;
				}
			}
			if(!found)
				ok = 0;
		}
	}
	else
		ok = 0;
	return ok;
}

int SLAPI AmtList::Search(PPID amtTypeID, PPID curID, uint * pPos) const
{
	// @v9.3.5 {
	for(uint i = 0; i < count; i++) {
		const AmtEntry & r_entry = at(i);
		if(r_entry.AmtTypeID == amtTypeID && r_entry.CurID == curID) {
			ASSIGN_PTR(pPos, i);
			return 1;
		}
	}
	ASSIGN_PTR(pPos, 0);
	return 0;
	// } @v9.3.5
	/* @v9.3.5
	int    ok = 0;
	uint   pos = 0;
	AmtEntry key(amtTypeID, curID);
	if(lsearch(&key, &pos, PTR_CMPFUNC(_2long)))
		ok = 1;
	ASSIGN_PTR(pPos, pos);
	return ok;
	*/
}

int SLAPI AmtList::GetCurList(PPID amtTypeID, PPIDArray * pCurList) const
{
	int    ok = -1;
	if(pCurList)
		for(uint i = 0; ok && i < getCount(); i++)
			if(amtTypeID < 0 || at(i).AmtTypeID == amtTypeID)
				ok = pCurList->addUnique(at(i).CurID) ? 1 : 0;
	return ok;
}

int SLAPI AmtList::GetAmtTypeList(PPIDArray * pList) const
{
	int    ok = -1;
	if(pList)
		for(uint i = 0; ok && i < getCount(); i++)
			ok = pList->addUnique(at(i).AmtTypeID) ? 1 : 0;
	return ok;
}

int SLAPI AmtList::Get(PPID amtTypeID, PPID curID, double * pAmt) const
{
	int    ok = -1;
	uint   pos = 0;
	if(Search(amtTypeID, curID, &pos)) {
		ASSIGN_PTR(pAmt, at(pos).Amt);
		ok = 1;
	}
	else
		ASSIGN_PTR(pAmt, 0);
	return ok;
}

double SLAPI AmtList::Get(PPID amtTypeID, PPID curID) const
{
	double v;
	Get(amtTypeID, curID, &v);
	return v;
}

int SLAPI AmtList::Put(PPID amtTypeID, PPID curID, double amt, int ignoreZero, int replace)
{
	int    ok = 1;
	uint   pos = 0;
	amt = oneof2(amtTypeID, PPAMT_CRATE, PPAMT_TRANSITCRATE) ? R6(amt) : R2(amt);
	int    zero = BIN(ignoreZero && amt == 0.0);
	if(Search(amtTypeID, curID, &pos) > 0) {
		if(replace)
			if(zero)
				atFree(pos);
			else
				at(pos).Amt = amt;
	}
	else if(!zero) {
		AmtEntry e(amtTypeID, curID, amt);
		ok = insert(&e) ? 1 : PPSetErrorSLib();
	}
	return ok;
}

int SLAPI AmtList::Put(const AmtEntry * pEntry, int ignoreZero, int replace)
{
	return Put(pEntry->AmtTypeID, pEntry->CurID, pEntry->Amt, ignoreZero, replace);
}

int SLAPI AmtList::Put(const AmtList * pList, int ignoreZero, int replace)
{
	for(uint i = 0; i < pList->getCount(); i++) {
		AmtEntry & e = pList->at(i);
		if(!Put(e.AmtTypeID, e.CurID, e.Amt, ignoreZero, replace))
			return 0;
	}
	return 1;
}

int SLAPI AmtList::Add(PPID amtTypeID, PPID curID, double amt, int ignoreZero)
{
	AmtEntry ae(amtTypeID, curID, amt);
	return Add(&ae, ignoreZero);
}

int SLAPI AmtList::Add(const AmtEntry * pEntry, int ignoreZero)
{
	uint pos = 0;
	if(Search(pEntry->AmtTypeID, pEntry->CurID, &pos) > 0)
		at(pos).Amt += pEntry->Amt;
	else
		return Put(pEntry, ignoreZero, 1);
	return 1;
}

int SLAPI AmtList::Sub(const AmtEntry * pEntry, int ignoreZero)
{
	AmtEntry temp_entry = *pEntry;
	temp_entry.Amt = -temp_entry.Amt;
	return Add(&temp_entry, ignoreZero);
}

int SLAPI AmtList::Add(const AmtList * pList, int ignoreZero)
{
	if(pList)
		for(uint i = 0; i < pList->getCount(); i++)
			if(!Add(&pList->at(i), ignoreZero))
				return 0;
	return 1;
}

int SLAPI AmtList::Sub(const AmtList * pList, int ignoreZero)
{
	if(pList)
		for(uint i = 0; i < pList->getCount(); i++)
			if(!Sub(&pList->at(i), ignoreZero))
				return 0;
	return 1;
}

int SLAPI AmtList::Remove(PPID amtTypeID, PPID curID)
{
	int    ok = -1;
	uint   i = count;
	if(i) do {
		const AmtEntry & r_entry = at(--i);
		if(r_entry.AmtTypeID == amtTypeID && (curID < 0 || r_entry.CurID == curID)) {
			atFree(i);
			ok = 1;
		}
	} while(i);
	return ok;
}
