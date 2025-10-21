// V_DBDIV.CPP
// Copyright (c) A.Starodub 2013, 2014, 2016, 2019, 2022
// @codepage windows-1251
//
#include <pp.h>
#pragma hdrstop

IMPLEMENT_PPFILT_FACTORY(DBDiv); DBDivFilt::DBDivFilt() : PPBaseFilt(PPFILT_DBDIV, 0, 0)
{
	SetFlatChunk(offsetof(DBDivFilt, ReserveStart), offsetof(DBDivFilt, ReserveEnd) - offsetof(DBDivFilt, ReserveStart));
	SetBranchObjIdListFilt(offsetof(DBDivFilt, LocList));
	Init(1, 0);
}

DBDivFilt & FASTCALL DBDivFilt::operator = (const DBDivFilt & s)
{
	Copy(&s, 0);
	return *this;
}

PPViewDBDiv::PPViewDBDiv() : PPView(&ObjDBDiv, &Filt, PPVIEW_DBDIV, implDontEditNullFilter, 0), P_TempTbl(0)
{
}

PPViewDBDiv::~PPViewDBDiv()
{
	ZDELETE(P_TempTbl);
}

int PPViewDBDiv::CheckForFilt(const DBDivPack * pPack) const
{
	if(pPack) {
		if(!Filt.LocList.IsEmpty()) {
			ObjIdListFilt loc_list;
			loc_list.Set(&pPack->LocList);
			if(loc_list.Intersect(&Filt.LocList, 0) <= 0 || loc_list.GetCount() <= 0)
				return 0;
		}
	}
	return 1;
}

TempDBDivTbl::Rec & PPViewDBDiv::MakeTempEntry(const DBDivPack & rPack, TempDBDivTbl::Rec & rTempRec)
{
	rTempRec.ID = rPack.Rec.ID;
	STRNSCPY(rTempRec.Name, rPack.Rec.Name);
	STRNSCPY(rTempRec.Address, rPack.Rec.Addr);
	rTempRec.Flags = rPack.Rec.Flags;
	return rTempRec;
}

// @v11.4.4. #define GRP_LOC 1

int PPViewDBDiv::EditBaseFilt(PPBaseFilt * pFilt)
{
	enum {
		ctlgroupLoc = 1,
	};
	int    ok = -1;
	DBDivFilt filt;
	TDialog * p_dlg = new TDialog(DLG_DBDIVFLT);
	LocationCtrlGroup::Rec loc_rec;

	filt.Copy(pFilt, 0);
	THROW(CheckDialogPtr(&p_dlg));
	p_dlg->addGroup(ctlgroupLoc, new LocationCtrlGroup(CTLSEL_DBDIVFLT_LOCLIST, 0, 0, cmLocList, 0, 0, 0));

	loc_rec.LocList = filt.LocList;
	p_dlg->setGroupData(ctlgroupLoc, &loc_rec);
	if(ExecView(p_dlg) == cmOK) {
		p_dlg->getGroupData(ctlgroupLoc, &loc_rec);
		filt.LocList = loc_rec.LocList;
		CALLPTRMEMB(pFilt, Copy(&filt, 0));
		ok = 1;
	}
	CATCHZOKPPERR
	delete p_dlg;
	return ok;
}

PP_CREATE_TEMP_FILE_PROC(CreateTempFile, TempDBDiv);

int PPViewDBDiv::Init_(const PPBaseFilt * pFilt)
{
	int    ok = 1;
	THROW(Helper_InitBaseFilt(pFilt));
	BExtQuery::ZDelete(&P_IterQuery);
	ZDELETE(P_TempTbl);
	THROW(P_TempTbl = CreateTempFile());
	{
		PPDBDiv rec;
		BExtInsert bei(P_TempTbl);
		PPTransaction tra(ppDbDependTransaction, 1);
		THROW(tra);
		for(PPID id = 0; ObjDBDiv.EnumItems(&id, &rec) > 0;) {
			DBDivPack pack;
			if(ObjDBDiv.Get(rec.ID, &pack) > 0) {
				if(CheckForFilt(&pack) > 0) {
					TempDBDivTbl::Rec temp_rec;
					THROW_DB(bei.insert(&MakeTempEntry(pack, temp_rec)));
				}
			}
		}
		THROW_DB(bei.flash());
		THROW(tra.Commit());
	}
	CATCH
		ZDELETE(P_TempTbl);
		ok = 0;
	ENDCATCH
	return ok;
}

int PPViewDBDiv::UpdateTempTable(const PPIDArray * pIdList)
{
	int    ok = -1;
	if(pIdList && P_TempTbl) {
		PPTransaction tra(ppDbDependTransaction, 1);
		THROW(tra);
		for(uint i = 0; i < pIdList->getCount(); i++) {
			PPID   id = pIdList->at(i);
			DBDivPack pack;
			TempDBDivTbl::Rec temp_rec;
			if(ObjDBDiv.Get(id, &pack) > 0 && CheckForFilt(&pack)) {
				ok = 1;
				MakeTempEntry(pack, temp_rec);
				if(SearchByID_ForUpdate(P_TempTbl, 0,  id, 0) > 0) {
					THROW_DB(P_TempTbl->updateRecBuf(&temp_rec)); // @sfu
				}
				else {
					THROW_DB(P_TempTbl->insertRecBuf(&temp_rec));
				}
			}
			else {
				THROW_DB(deleteFrom(P_TempTbl, 0, P_TempTbl->ID == id));
				ok = 1;
			}
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int PPViewDBDiv::InitIteration()
{
	int    ok = 1;
	TempDBDivTbl::Key0 k, k_;
	BExtQuery::ZDelete(&P_IterQuery);
	P_IterQuery = new BExtQuery(P_TempTbl, 0, 128);
	P_IterQuery->selectAll();
	MEMSZERO(k);
	k_ = k;
	Counter.Init(P_IterQuery->countIterations(0, &k_, spGe));
	P_IterQuery->initIteration(false, &k, spGe);
	return ok;
}

int FASTCALL PPViewDBDiv::NextIteration(DBDivViewItem * pItem)
{
	while(pItem && P_IterQuery && P_IterQuery->nextIteration() > 0) {
		PPDBDiv rec;
		TempDBDivTbl * p_t = P_TempTbl;
		if(ObjDBDiv.Search(p_t->data.ID, &rec) > 0) {
			ASSIGN_PTR(pItem, rec);
			return 1;
		}
	}
	return -1;
}

DBQuery * PPViewDBDiv::CreateBrowserQuery(uint * pBrwId, SString * pSubTitle)
{
	DBQuery * q  = 0;
	TempDBDivTbl * t = 0;
	uint   brw_id = BROWSER_DBDIV;
	DBQ  * dbq = 0;
	THROW(CheckTblPtr(t = new TempDBDivTbl(P_TempTbl->GetName())));
	q = & select(
			t->ID,            // #0
			t->Name,          // #1
			t->Address,       // #2
			0L).from(t, 0L).where(*dbq).orderBy(t->Name, 0L);
	THROW(CheckQueryPtr(q));
	CATCH
		if(q)
			ZDELETE(q);
		else
			delete t;
	ENDCATCH
	ASSIGN_PTR(pBrwId, brw_id);
	return q;
}

int PPViewDBDiv::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = (ppvCmd != PPVCMD_ADDITEM) ? PPView::ProcessCommand(ppvCmd, pHdr, pBrw) : -2;
	PPIDArray id_list;
	PPID   id = (pHdr) ? *static_cast<const PPID *>(pHdr) : 0;
	if(ok == -2) {
		switch(ppvCmd) {
			case PPVCMD_ADDITEM:
				ok = (ObjDBDiv.Edit(&(id = 0), 0) == cmOK) ? 1 : -1;
				break;
		}
	}
	if(ok > 0 && oneof3(ppvCmd, PPVCMD_ADDITEM, PPVCMD_EDITITEM, PPVCMD_DELETEITEM)) {
		id_list.add(id);
		ok = UpdateTempTable(&id_list);
	}
	return ok;
}
