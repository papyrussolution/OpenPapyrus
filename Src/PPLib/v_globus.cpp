// V_GLOBUS.CPP
// Copyright (c) A.Starodub 2012, 2016, 2017, 2019, 2020, 2021
//
// PPViewGlobalUserAcc
//
#include <pp.h>
#pragma hdrstop
#include <charry.h>

IMPLEMENT_PPFILT_FACTORY(GlobalUserAcc); GlobalUserAccFilt::GlobalUserAccFilt() : PPBaseFilt(PPFILT_GLOBALUSERACC, 0, 0)
{
	SetFlatChunk(offsetof(GlobalUserAccFilt, ReserveStart),	offsetof(GlobalUserAccFilt, ReserveEnd) - offsetof(GlobalUserAccFilt, ReserveStart));
	Init(1, 0);
}

GlobalUserAccFilt & FASTCALL GlobalUserAccFilt::operator = (const GlobalUserAccFilt & s)
{
	Copy(&s, 1);
	return *this;
}

PPViewGlobalUserAcc::PPViewGlobalUserAcc() : PPView(&ObjGlobAcc, &Filt, PPVIEW_GLOBALUSERACC, implUseQuickTagEditFunc, 0), P_TempTbl(0) // @v11.2.8 implUseQuickTagEditFunc
{
}

PPViewGlobalUserAcc::~PPViewGlobalUserAcc()
{
	ZDELETE(P_TempTbl);
}

TempGlobUserAccTbl::Rec & PPViewGlobalUserAcc::MakeTempEntry(const PPGlobalUserAcc & rRec, TempGlobUserAccTbl::Rec & rTempRec)
{
	rTempRec.ID = rRec.ID;
	rTempRec.PersonID = rRec.PersonID;
	rTempRec.Flags    = rRec.Flags;
	rTempRec.GlobalService = rRec.ServiceIdent; // @v12.2.0
	STRNSCPY(rTempRec.Name, rRec.Name);
	rRec.LocalDbUuid.ToStr(0, SLS.AcquireRvlStr()).CopyTo(rTempRec.Guid, sizeof(rTempRec.Guid));
	return rTempRec;
}

/*virtual*/int PPViewGlobalUserAcc::EditBaseFilt(PPBaseFilt *) { return 1; } // @stub

PP_CREATE_TEMP_FILE_PROC(CreateTempFile, TempGlobUserAcc);

/*virtual*/int PPViewGlobalUserAcc::Init_(const PPBaseFilt * pFilt)
{
	int    ok = 1;
	THROW(Helper_InitBaseFilt(pFilt));
	BExtQuery::ZDelete(&P_IterQuery);
	ZDELETE(P_TempTbl);
	THROW(P_TempTbl = CreateTempFile());
	{
		PPGlobalUserAcc rec;
		BExtInsert bei(P_TempTbl);
		PPTransaction tra(ppDbDependTransaction, 1);
		THROW(tra);
		for(PPID id = 0; ObjGlobAcc.EnumItems(&id, &rec) > 0;) {
			if(CheckForFilt(&rec) > 0) {
				TempGlobUserAccTbl::Rec temp_rec;
				THROW_DB(bei.insert(&MakeTempEntry(rec, temp_rec)));
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

int PPViewGlobalUserAcc::UpdateTempTable(const PPIDArray * pIdList)
{
	int    ok = -1;
	if(pIdList && P_TempTbl) {
		PPTransaction tra(ppDbDependTransaction, 1);
		THROW(tra);
		for(uint i = 0; i < pIdList->getCount(); i++) {
			PPID   id = pIdList->at(i);
			PPGlobalUserAcc rec;
			TempGlobUserAccTbl::Rec temp_rec;
			if(ObjGlobAcc.Search(id, &rec) > 0 && CheckForFilt(&rec)) {
				ok = 1;
				MakeTempEntry(rec, temp_rec);
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

int PPViewGlobalUserAcc::InitIteration()
{
	int    ok = 1;
	TempGlobUserAccTbl::Key0 k, k_;
	BExtQuery::ZDelete(&P_IterQuery);
	P_IterQuery = new BExtQuery(P_TempTbl, 0, 128);
	P_IterQuery->selectAll();
	MEMSZERO(k);
	k_ = k;
	Counter.Init(P_IterQuery->countIterations(0, &k_, spGe));
	P_IterQuery->initIteration(false, &k, spGe);
	return ok;
}

int FASTCALL PPViewGlobalUserAcc::NextIteration(GlobalUserAccViewItem * pItem)
{
	while(pItem && P_IterQuery && P_IterQuery->nextIteration() > 0) {
		PPGlobalUserAcc rec;
		TempGlobUserAccTbl * p_t = P_TempTbl;
		if(ObjGlobAcc.Search(p_t->data.ID, &rec) > 0) {
			ASSIGN_PTR(pItem, rec);
			return 1;
		}
	}
	return -1;
}

/*virtual*/DBQuery * PPViewGlobalUserAcc::CreateBrowserQuery(uint * pBrwId, SString * pSubTitle)
{
	DBQuery * q  = 0;
	TempGlobUserAccTbl * t = 0;
	uint   brw_id = BROWSER_GLOBALUSERACC;
	DBQ  * dbq = 0;
	DBE    dbe_psn;
	DBE    dbe_svc; // @v12.2.0
	THROW(CheckTblPtr(t = new TempGlobUserAccTbl(P_TempTbl->GetName())));
	PPDbqFuncPool::InitObjNameFunc(dbe_psn,  PPDbqFuncPool::IdObjNamePerson, t->PersonID);
	// @v12.2.0 {
	{
		dbe_svc.init();
		dbe_svc.push(dbconst(PPTXT_GLOBALSERVICELIST));
		dbe_svc.push(t->GlobalService);
		dbe_svc.push(static_cast<DBFunc>(PPDbqFuncPool::IdSubStrById));
	}
	// } @v12.2.0
	q = & select(
			t->ID,            // #0
			t->Name,          // #1
			dbe_psn,          // #2
			t->Guid,          // #3
			dbe_svc,          // #4
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

/*virtual*/int PPViewGlobalUserAcc::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = (ppvCmd != PPVCMD_ADDITEM) ? PPView::ProcessCommand(ppvCmd, pHdr, pBrw) : -2;
	PPID   id = pHdr ? *static_cast<const PPID *>(pHdr) : 0;
	if(ok == -2) {
		switch(ppvCmd) {
			case PPVCMD_ADDITEM:
				ok = (ObjGlobAcc.Edit(&(id = 0), 0) == cmOK) ? 1 : -1;
				break;
			case PPVCMD_QUICKTAGEDIT: // @v11.2.8
				// � ���� ������� ��������� pHdr ����� ��� ������ ��������������� �����, ��������������� ������� �������
				// � ����� � ���� ������� ������� ������� �������� �������� ����� ������� pBrw->getCurItem()
				//
				{
					const BrwHdr * p_row = static_cast<const BrwHdr *>(pBrw->getCurItem());
					ok = PPView::Helper_ProcessQuickTagEdit(PPObjID(PPOBJ_GLOBALUSERACC, p_row ? p_row->ID : 0), pHdr/*(LongArray *)*/);
				}
				break;
			case PPVCMD_TAGS:
				if(id)
					ok = EditObjTagValList(PPOBJ_GLOBALUSERACC, id, 0);
				else
					ok = -1;
				break;
			case PPVCMD_TRANSACTJ:
				{
					GtaJournalFilt filt;
					filt.GlobalUserID = id;
					PPView::Execute(PPVIEW_GTAJOURNAL, &filt, 1, 0);
				}
				break;
		}
	}
	if(ok > 0 && oneof3(ppvCmd, PPVCMD_ADDITEM, PPVCMD_EDITITEM, PPVCMD_DELETEITEM)) {
		PPIDArray id_list;
		id_list.add(id);
		ok = UpdateTempTable(&id_list);
	}
	return ok;
}

int PPViewGlobalUserAcc::CheckForFilt(const PPGlobalUserAcc * pRec) const
{
	int    ok = 1;
	return ok;
}
