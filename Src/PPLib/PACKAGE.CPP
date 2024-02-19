// PACKAGE.CPP
// Copyright (c) A.Sobolev 2001, 2002, 2006, 2007, 2008, 2009, 2010, 2015, 2016, 2017, 2019, 2020, 2024
// @Kernel
//
#include <pp.h>
#pragma hdrstop
//
// Implementation of LPackage
// @todo 29/06/2006 Перевести на LAssocArray
//
struct LPII { // @flat
	int16  Idx;
	int32  ID;
};

LPackage::LPackage() : SVector(sizeof(LPII))
{
	Init();
}

int FASTCALL LPackage::Copy(const LPackage & s)
{
	SVector::copy(s);
	PckgIdx = s.PckgIdx;
	ID = s.ID;
	PrevID = s.PrevID;
	PckgTypeID = s.PckgTypeID;
	LocID = s.LocID;
	memcpy(Code, s.Code, sizeof(Code));
	Closed = s.Closed;
	UniqCntr = s.UniqCntr;
	Flags = s.Flags;
	Qtty = s.Qtty;
	PhQtty = s.PhQtty;
	Cost = s.Cost;
	Price = s.Price;
	return 1;
}

LPackage & FASTCALL LPackage::operator = (const LPackage & s)
{
	Copy(s);
	return *this;
}

void LPackage::Init()
{
	ID = 0;
	PrevID = 0;
	PckgTypeID = 0;
	LocID = 0;
	memzero(Code, sizeof(Code));
	Closed = 0;
	UniqCntr = 0;
	Flags = 0;
	PckgIdx = -1;
	Qtty = PhQtty = 0;
	Cost = Price = 0;
	freeAll();
}

int LPackage::EnumItems(uint * pI, int * pIdx, PPID * pID) const
{
	LPII * p_item;
	if(enumItems(pI, (void **)&p_item)) {
		ASSIGN_PTR(pIdx, p_item->Idx);
		ASSIGN_PTR(pID, p_item->ID);
		return 1;
	}
	ASSIGN_PTR(pIdx, 0);
	ASSIGN_PTR(pID, 0);
	return 0;
}

int LPackage::UpdateItem(uint i, int idx, PPID id)
{
	if(i < getCount()) {
		static_cast<LPII *>(at(i))->Idx = idx;
		static_cast<LPII *>(at(i))->ID  = id;
		return 1;
	}
	return 0;
}

int LPackage::GetLotIDList(PPIDArray * pList) const
{
	if(pList) {
		for(uint i = 0; i < getCount(); i++)
			if(static_cast<LPII *>(at(i))->ID != 0)
				if(!pList->addUnique(static_cast<LPII *>(at(i))->ID))
					return 0;
		return 1;
	}
	else
		return -1;
}

int LPackage::SearchByID(PPID lotID, int * pIdx) const
{
	for(uint i = 0; i < getCount(); i++)
		if(static_cast<LPII *>(at(i))->ID == lotID) {
			ASSIGN_PTR(pIdx, static_cast<LPII *>(at(i))->Idx);
			return 1;
		}
	return 0;
}

int LPackage::SearchByIdx(int idx, PPID * pLotID) const
{
	for(uint i = 0; i < getCount(); i++)
		if(static_cast<LPII *>(at(i))->Idx == idx) {
			ASSIGN_PTR(pLotID, static_cast<LPII *>(at(i))->ID);
			return 1;
		}
	return 0;
}

int LPackage::AddLotIDList(const PPIDArray * pList)
{
	for(uint i = 0; i < pList->getCount(); i++)
		if(!AddItem(pList->at(i), -1))
			return 0;
	return 1;
}

int LPackage::AddItem(PPID lotID, int idx)
{
	if(lotID) {
		if(SearchByID(lotID, 0) > 0)
			return -1;
	}
	else if(SearchByIdx(idx, 0) > 0)
		return -1;
	LPII item;
	item.Idx = idx;
	item.ID = lotID;
	return insert(&item) ? 1 : PPSetErrorSLib();
}

int LPackage::RemoveByID(PPID lotID)
{
	for(int i = getCount()-1; i >= 0; i--)
		if(static_cast<LPII *>(at(i))->ID == lotID)
			atFree(i);
	return 1;
}

int LPackage::RemoveByIdx(int idx)
{
	for(int i = getCount()-1; i >= 0; i--)
		if(static_cast<LPII *>(at(i))->Idx == idx)
			atFree(i);
	return 1;
}

int FASTCALL LPackage::ShiftIdx(int idx)
{
	LPII * p_item;
	for(uint i = 0; enumItems(&i, (void **)&p_item);)
		if(p_item->Idx > idx)
			p_item->Idx--;
	return 1;
}
//
// Implementation of LPackageList
//
LPackageList::LPackageList() : SCollection()
{
}

LPackageList::~LPackageList()
{
	freeAll();
}

/*virtual*/void FASTCALL LPackageList::freeItem(void * pItem) { delete static_cast<LPackage *>(pItem); }

int LPackageList::Add(const LPackage * pItem)
{
	LPackage * p_temp = new LPackage;
	if(p_temp) {
		*p_temp = *pItem;
		return insert(p_temp) ? 1 : PPSetErrorSLib();
	}
	else
		return PPSetErrorNoMem();
}

int LPackageList::ShiftIdx(int16 idx)
{
	LPackage * p_pckg;
	for(uint i = 0; enumItems(&i, (void **)&p_pckg);) {
		p_pckg->ShiftIdx(idx);
		if(p_pckg->PckgIdx > idx)
			p_pckg->PckgIdx--;
	}
	return 1;
}

int LPackageList::RemoveByIdx(int16 idx)
{
	LPackage * p_pckg;
	if(idx >= 0)
		for(uint i = 0; enumItems(&i, (void **)&p_pckg);)
			if(p_pckg->PckgIdx == idx) {
				atFree(i-1);
				break;
			}
	return 1;
}

LPackage * LPackageList::GetByID(PPID id)
{
	for(uint i = 0; i < getCount(); i++)
		if(static_cast<const LPackage *>(at(i))->ID == id)
			return static_cast<LPackage *>(at(i));
	return 0;
}

LPackage * LPackageList::GetByIdx(int16 idx)
{
	for(uint i = 0; i < getCount(); i++)
		if(static_cast<const LPackage *>(at(i))->PckgIdx == idx)
			return static_cast<LPackage *>(at(i));
	return 0;
}

int LPackageList::EnumItems(uint * pI, LPackage ** ppPckg)
{
	return enumItems(pI, (void **)ppPckg);
}

int LPackageList::CheckCodeUnique(PPID pckgTypeID, const char * pCode, int pos)
{
	LPackage * p_pckg;
	for(uint i = 0; EnumItems(&i, &p_pckg);)
		if(p_pckg->PckgTypeID == pckgTypeID &&
			stricmp866(pCode, p_pckg->Code) == 0 && pos != p_pckg->PckgIdx) {
			return 0;
		}
	return 1;
}
//
// Implementation of PackageCore
//
PackageCore::PackageCore() : PackageTbl()
{
}

int PackageCore::Search(PPID id, PackageTbl::Rec * pPckg)
{
	return SearchByID(this, PPOBJ_PACKAGE, id, pPckg);
}

int PackageCore::SearchPrev(PPID prevID, PackageTbl::Rec * pPckg)
{
	PackageTbl::Key2 k2;
	k2.PrevID = prevID;
	k2.ID = 0;
	if(search(2, &k2, spGe) && k2.PrevID == prevID) {
		copyBufTo(pPckg);
		return 1;
	}
	return PPDbSearchError();
}

int PackageCore::GetLotLink(PPID lotID, PPID * pPckgID, PackageTbl::Rec * pRec)
{
	int    ok = -1;
	PackageLinkTbl::Key1 k;

	ASSIGN_PTR(pPckgID, 0);
	k.LotID = lotID;
	if(PL.search(1, &k, spEq)) {
		ASSIGN_PTR(pPckgID, PL.data.PckgID);
		if(pRec) {
			int r = Search(PL.data.PckgID, pRec);
			if(r > 0)
				ok = 1;
			else if(r < 0)
				ok = PPSetError(PPERR_BL_LOT2PCKG);
			else
				ok = 0;
		}
		else
			ok = 1;
	}
	else
		ok = PPDbSearchError();
	return ok;
}

int PackageCore::GetPckgLinkList(PPID id, PPIDArray * pList)
{
	PackageLinkTbl::Key0 k;
	BExtQuery q(&PL, 0);
	q.selectAll().where(PL.PckgID == id);
	k.PckgID = id;
	k.LotID = 0;
	for(q.initIteration(false, &k, spGe); q.nextIteration() > 0;)
		if(!pList->add(PL.data.LotID))
			return 0;
	return 1;
}

int PackageCore::GetPckg(PPID id, LPackage * pPckg)
{
	int    ok = 1;
	PackageTbl::Rec rec;
	PPIDArray lot_id_list;

	pPckg->Init();
	THROW(Search(id, &rec) > 0);
	THROW(GetPckgLinkList(id, &lot_id_list));
	pPckg->ID = rec.ID;
	pPckg->PrevID = rec.PrevID;
	pPckg->PckgTypeID = rec.PckgTypeID;
	pPckg->LocID = rec.LocID;
	pPckg->Flags = rec.Flags;
	pPckg->Closed = rec.Closed;
	pPckg->UniqCntr = rec.UniqCntr;
	STRNSCPY(pPckg->Code, rec.Code);
	THROW(pPckg->AddLotIDList(&lot_id_list));
	CATCHZOK
	return ok;
}

int PackageCore::AddPckgLink(PPID pckgID, PPID lotID)
{
	if(pckgID == 0 || lotID == 0)
		return -1;
	PackageLinkTbl::Rec rec;
	rec.PckgID = pckgID;
	rec.LotID = lotID;
	return PL.insertRecBuf(&rec) ? 1 : PPSetErrorDB();
}

int PackageCore::RemovePckgLink(PPID pckgID, PPID lotID)
{
	return deleteFrom(&PL, 0, lotID ? (PL.PckgID == pckgID && PL.LotID == lotID) : (PL.PckgID == pckgID));
}

int PackageCore::SetClosedTag(PPID id, int tag, int use_ta)
{
	int    ok = 1;
	PackageTbl::Rec rec;
	if(Search(id, &rec) > 0) {
		if(rec.Closed != (tag ? 1 : 0)) {
			rec.Closed = (tag ? 1 : 0);
			if(tag || !(rec.Flags & PCKGF_UNIQUECODE)) {
				int    closed_cntr = rec.Closed;
				int    uniq_cntr   = rec.UniqCntr;
				PackageTbl::Key1 k;
				MEMSZERO(k);
				k.Closed = closed_cntr;
				STRNSCPY(k.Code, rec.Code);
				k.PckgTypeID = rec.PckgTypeID;
				k.UniqCntr = rec.UniqCntr;
				while(search(1, &k, spEq))
					if(tag)
						k.Closed = ++closed_cntr;
					else
						k.UniqCntr = ++uniq_cntr;
				if(tag)
					rec.Closed = closed_cntr;
				else
					rec.UniqCntr = uniq_cntr;
			}
			if(UpdateByID(this, PPOBJ_PACKAGE, id, &rec, use_ta))
				ok = 1;
			else {
				PPSetAddedMsgString(rec.Code);
				ok = 0;
			}
		}
	}
	return ok;
}

int PackageCore::AdjustUniqCntr(PackageTbl::Rec * pRec)
{
	int    ok = 1;
	PPIDArray dup_list;
	if(pRec->Closed) {
		int    closed_cntr = pRec->Closed;
		PackageTbl::Key1 k;
		MEMSZERO(k);
		k.Closed = closed_cntr;
		STRNSCPY(k.Code, pRec->Code);
		k.PckgTypeID = pRec->PckgTypeID;
		k.UniqCntr = pRec->UniqCntr;
		while(search(1, &k, spEq))
			k.Closed = ++closed_cntr;
		pRec->Closed = closed_cntr;
	}
	else {
		THROW(SearchByCode(pRec->PckgTypeID, pRec->Code, &dup_list));
		dup_list.removeByID(pRec->ID);
		if(dup_list.getCount()) {
			if(!(pRec->Flags & PCKGF_UNIQUECODE)) {
				PackageTbl::Rec tr;
				long cntr = 1;
				for(uint i = 0; i < dup_list.getCount();)
					if(Search(dup_list.at(i), &tr) > 0 && tr.UniqCntr == cntr) {
						cntr++;
						i = 0;
					}
					else
						i++;
				pRec->UniqCntr = (int16)cntr;
			}
			else
				CALLEXCEPT_PP_S(PPERR_DUPPCKGCODE, pRec->Code);
		}
	}
	CATCHZOK
	return ok;
}

int PackageCore::PutPckg(PPID id, const LPackage * pPckg, int use_ta)
{
	int    ok = 1, insert_links = 0;
	PackageTbl::Rec rec;
	if(id) {
		PPTransaction tra(use_ta);
		THROW(tra);
		if(Search(id, 0) > 0) {
			if(pPckg) {
				PPSetAddedMsgString(pPckg->Code);
				MEMSZERO(rec);
				rec.ID = id;
				rec.PrevID = pPckg->PrevID;
				rec.PckgTypeID = pPckg->PckgTypeID;
				rec.LocID    = pPckg->LocID;
				STRNSCPY(rec.Code, pPckg->Code);
				rec.Closed   = pPckg->Closed;
				rec.UniqCntr = pPckg->UniqCntr;
				rec.Flags    = pPckg->Flags;

				THROW(AdjustUniqCntr(&rec));
				THROW(UpdateByID(this, PPOBJ_PACKAGE, id, &rec, 0));
				insert_links = 1;
			}
			else {
				PPID   prev_id = id;
				THROW(RemoveByID(this, id, 0));
				if(SearchPrev(prev_id, 0) > 0)
					THROW(PutPckg(data.ID, 0, 0));
			}
			THROW(RemovePckgLink(id, 0));
		}
		else if(pPckg) {
			MEMSZERO(rec);
			rec.ID = id;
			rec.PrevID = pPckg->PrevID;
			rec.PckgTypeID = pPckg->PckgTypeID;
			rec.LocID = pPckg->LocID;
			STRNSCPY(rec.Code, pPckg->Code);
			rec.Closed = pPckg->Closed;
			rec.UniqCntr = pPckg->UniqCntr;
			rec.Flags  = pPckg->Flags;

			THROW(AdjustUniqCntr(&rec));
			THROW(AddByID(this, 0, &rec, 0));
			insert_links = 1;
		}
		if(insert_links) {
			uint   i;
			PPIDArray lot_id_list;
			THROW(pPckg->GetLotIDList(&lot_id_list));
			for(i = 0; i < lot_id_list.getCount(); i++)
				THROW(AddPckgLink(id, lot_id_list.at(i)));
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int PackageCore::SearchByCode(PPID pckgTypeID, const char * pCode, PPIDArray * pList)
{
	int    sp = spGe, found = 0;
	PackageTbl::Key1 k;
	MEMSZERO(k);
	k.Closed = 0;
	STRNSCPY(k.Code, pCode);
	k.PckgTypeID = (pckgTypeID >= 0) ? pckgTypeID : 0;
	k.UniqCntr = 0;
	while(search(1, &k, sp) && k.Closed == 0 && stricmp866(k.Code, pCode) == 0 && (pckgTypeID < 0 || k.PckgTypeID == pckgTypeID)) {
		if(pList)
			pList->add(data.ID);
		sp = spGt;
		found = 1;
	}
	return found ? 1 : -1;
}

int PackageCore::CheckCodeUnique(PPID pckgTypeID, PPID pckgID, const char * pCode, long * pCntr)
{
	int    ok = 1;
	long   cntr = 0;
	PackageTbl::Key1 k;
	MEMSZERO(k);
	k.Closed = 0;
	STRNSCPY(k.Code, pCode);
	k.PckgTypeID = pckgTypeID;
	k.UniqCntr = MAXSHORT;
	if(search(1, &k, spLe) && k.Closed == 0 && k.PckgTypeID == pckgTypeID &&
		!stricmp866(k.Code, pCode) && data.ID != pckgID) {
		cntr = k.UniqCntr;
		ok = -1;
	}
	ASSIGN_PTR(pCntr, cntr+1);
	return ok;
}

