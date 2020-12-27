// V_USPROF.CPP
// Copyright (c) A.Starodub 2014, 2015, 2016, 2017, 2018, 2019, 2020
// @codepage windows-1251
//
#include <pp.h>
#pragma hdrstop

IMPLEMENT_PPFILT_FACTORY(UserProfile); UserProfileFilt::UserProfileFilt() : PPBaseFilt(PPFILT_USERPROFILE, 0, 0)
{
	SetFlatChunk(offsetof(UserProfileFilt, ReserveStart),
		offsetof(UserProfileFilt, ReserveEnd)-offsetof(UserProfileFilt, ReserveStart)+sizeof(ReserveEnd));
	Init(1, 0);
}

PPViewUserProfile::PPViewUserProfile() : PPView(0, &Filt, 0, implDontEditNullFilter, 0), ParserBusy(0)
{
}

PPViewUserProfile::~PPViewUserProfile()
{
}

class UserProfileFiltDialog : public TDialog {
	DECL_DIALOG_DATA(UserProfileFilt);
public:
	explicit UserProfileFiltDialog(TSArray <PPUserProfileCore::UfpDbEntry> & rUfpDbList) : TDialog(DLG_FLTUSRPROF), R_UfpDbList(rUfpDbList)
	{
		SetupCalCtrl(CTLCAL_FLTUSRPROF_PRD, this, CTL_FLTUSRPROF_PRD, 1);
	}
	DECL_DIALOG_SETDTS()
	{
		long   db_pos = 0;
		uint   i;
		SString temp_buf;
		StrAssocArray db_list, func_list;
		if(!RVALUEPTR(Data, pData))
			Data.Init(1, 0);
		for(i = 0; i < R_UfpDbList.getCount(); i++) {
			const PPUserProfileCore::UfpDbEntry & r_entry = R_UfpDbList.at(i);
			if(!!r_entry.DbID) {
				temp_buf.Z().Cat(r_entry.DbSymb).CatDiv('-', 1).Cat(r_entry.DbID, S_GUID::fmtIDL|S_GUID::fmtLower);
				db_list.Add(i+1, 0, temp_buf);
				if(r_entry.DbID == Data.DbID)
					db_pos = i+1;
			}
		}
		db_list.SortByText(); // @v10.9.10
		for(i = PPUPRF_LOGIN; i < PPUPRF_LAST; i++) {
			PPLoadString(PPSTR_USRPROFILEFUNCNAM, i, temp_buf);
			if(temp_buf.NotEmptyS())
				func_list.Add(i, 0, temp_buf);
		}
		SetupStrAssocCombo(this, CTLSEL_FLTUSRPROF_DB, &db_list, db_pos, 0);
		SetupStrAssocCombo(this, CTLSEL_FLTUSRPROF_FUNC, &func_list, Data.FuncID, 0);
		SetPeriodInput(this, CTLCAL_FLTUSRPROF_PRD, &Data.Period);
		SetTimeRangeInput(this, CTL_FLTUSRPROF_TIMEPRD, TIMF_HMS, &Data.TmPeriod);
		return 1;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		long   db_pos = getCtrlLong(CTL_FLTUSRPROF_DB) - 1;
		Data.FuncID = getCtrlLong(CTL_FLTUSRPROF_FUNC);
		if(db_pos >= 0)
			Data.DbID = R_UfpDbList.at(db_pos).DbID;
		else
			Data.DbID.Z();
		GetPeriodInput(this, CTL_FLTUSRPROF_PRD, &Data.Period);
		GetTimeRangeInput(this, CTL_FLTUSRPROF_TIMEPRD, TIMF_HMS, &Data.TmPeriod);
		ASSIGN_PTR(pData, Data);
		return ok;
	}
private:
	TSArray <PPUserProfileCore::UfpDbEntry> & R_UfpDbList;
	TSArray <PPUserProfileFileItem> * P_OffsList;
};

/*virtual*/int PPViewUserProfile::EditBaseFilt(PPBaseFilt * pBaseFilt)
{
	if(!Filt.IsA(pBaseFilt))
		return 0;
	TSArray <PPUserProfileCore::UfpDbEntry> ufp_db_list;
	Tbl.GetDbEntyList(ufp_db_list);
	DIALOG_PROC_BODY_P1ERR(UserProfileFiltDialog, ufp_db_list, static_cast<UserProfileFilt *>(pBaseFilt))
}

/*virtual*/int PPViewUserProfile::Init_(const PPBaseFilt * pFilt)
{
	int    ok = 1;
	Counter.Init();
	THROW(Helper_InitBaseFilt(pFilt));
	CATCHZOK
	BExtQuery::ZDelete(&P_IterQuery);
	return ok;
}

int PPViewUserProfile::InitIteration()
{
	int    ok = 0;
	BExtQuery::ZDelete(&P_IterQuery);
	return ok;
}

int FASTCALL PPViewUserProfile::NextIteration(UserProfileViewItem * pItem)
{
	int    ok = -1;
	UserProfileViewItem item;
	// @v10.7.8 @ctr MEMSZERO(item);
	ASSIGN_PTR(pItem, item);
	return ok;
}

PP_CREATE_TEMP_FILE_PROC(CreateTempFile, TempUserProfile);

int PPViewUserProfile::RemoveAll()
{
	return Tbl.ClearState(0, 1) ? 1 : PPErrorZ();
}

int PPViewUserProfile::LoadFromFile(PPIDArray * pAddedIdList)
{
	int    ok = 1;
	PPWait(1);
	THROW(Tbl.Load(0));
	PPWait(0);
	CATCHZOKPPERR
	PPWait(0);
	return ok;
}

static int CellStyleFunc(const void * pData, long col, int paintAction, BrowserWindow::CellStyle * pCellStyle, void * extraPtr)
{
	int    ok = -1;
	PPViewUserProfile * p_view = static_cast<PPViewUserProfile *>(extraPtr);
	if(pData && pCellStyle && p_view) {
		if(col == 0) {
			const PPViewUserProfile::BrwHdr * p_hdr = static_cast<const PPViewUserProfile::BrwHdr *>(pData);
			if(!(p_hdr->Flags & USRPROFF_FINISHED)) {
				pCellStyle->Flags  = BrowserWindow::CellStyle::fCorner;
				pCellStyle->Color  = GetColorRef(SClrRed);
				ok = 1;
			}
		}
	}
	return ok;
}

/*virtual*/void PPViewUserProfile::PreprocessBrowser(PPViewBrowser * pBrw)
{
	if(pBrw) {
		pBrw->SetCellStyleFunc(CellStyleFunc, this);
		// pBrw->Advise(PPAdviseBlock::evQuartz, 0, 0, 0);
	}
}

/*virtual*/DBQuery * PPViewUserProfile::CreateBrowserQuery(uint * pBrwId, SString * pSubTitle)
{
	uint   brw_id = 0;
	SString temp_buf;
	TempUserProfileTbl * t = 0;
	UserFuncPrfSessTbl * t1 = 0;
	UserFuncPrfTbl * t2 = 0;
	DBQuery * q = 0;
	DBE    dbe_func, dbe_ver;
	DBE    dbe_dbuuidcmp;
	DBQ  * dbq = 0;
	{
		brw_id = BROWSER_USERPROFILE;
		THROW(CheckTblPtr(t1 = new UserFuncPrfSessTbl));
		THROW(CheckTblPtr(t2 = new UserFuncPrfTbl));
		PPDbqFuncPool::InitObjNameFunc(dbe_func, PPDbqFuncPool::IdUfpFuncName, t2->FuncID);
		PPDbqFuncPool::InitObjNameFunc(dbe_ver,  PPDbqFuncPool::IdVersionText, t1->Ver);
		dbq = &(t1->ID == t2->SessID);
		if(Filt.FuncID) {
			long   _f_b = Filt.FuncID * 1000;
			long   _f_e = Filt.FuncID * 1000 + 999;
			dbq = &(*dbq && t2->FuncID >= _f_b && t2->FuncID <= _f_e);
		}
		// @v10.9.10 {
		if(!!Filt.DbID) {
			temp_buf.Z().Cat(Filt.DbID, S_GUID::fmtIDL);
			dbe_dbuuidcmp.init();
			dbe_dbuuidcmp.push(t1->DbUUID_s);
			dbe_dbuuidcmp.push(dbconst(temp_buf));
			dbe_dbuuidcmp.push(static_cast<DBFunc>(PPDbqFuncPool::IdIsTxtUuidEq));
			dbq = &(*dbq && dbe_dbuuidcmp > 0L);
		}
		// } @v10.9.10 
		dbq = &(*dbq && daterange(t2->Dt, &Filt.Period));
		dbq = &(*dbq && timerange(t2->Tm, &Filt.TmPeriod));
		q = &select(
			t2->SessID,     // #0
			t2->SeqID,      // #1
			t2->Flags,      // #2
			t1->ThreadId,   // #3
			t2->Dt,         // #4
			t2->Tm,         // #5
			t2->Clock,      // #6
			t1->SessUUID_s, // #7
			t1->DbName,     // #8
			t1->UserName,   // #9
			dbe_func,       // #10
			dbe_ver,        // #11
			t2->Factor1,    // #12
			t2->Factor2,    // #13
			t2->Factor3,    // #14
			0).from(t2, t1, 0).where(*dbq).orderBy(t2->Dt, t2->Tm, 0);
	}
	THROW(CheckQueryPtr(q));
	CATCH
		if(q)
			ZDELETE(q);
		else {
			delete t;
			delete t1;
			delete t2;
		}
	ENDCATCH
	ASSIGN_PTR(pBrwId, brw_id);
	return q;
}

int PPViewUserProfile::HandleNotifyEvent(int kind, const PPNotifyEvent * pEv, PPViewBrowser * pBrw, void * extraProcPtr)
{
	int    ok = -1, update = 0, lock = 0;
	PPIDArray id_list;
	if(pEv) {
		if(kind == PPAdviseBlock::evQuartz && ParserBusy == 0) {
			ParserBusy = 1;
			lock = 1;
			THROW(LoadFromFile(&id_list));
			ok = 1;
		}
	}
	update = BIN(id_list.getCount() > 0);
	if(ok > 0 && update && pBrw) {
		// THROW(UpdateTempTable(&id_list));
		pBrw->Refresh();
	}
	CATCHZOK
	if(lock)
		ParserBusy = 0;
	return ok;
}

/*virtual*/int PPViewUserProfile::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	int    update = 0;
	PPIDArray id_list;
	if(ok == -2) {
		switch(ppvCmd) {
			case PPVCMD_UPDATEITEMS:
				if((ok = LoadFromFile(&id_list)) > 0)
					update = 1;
				break;
			case PPVCMD_DELETEALL:
				RemoveAll();
				break;
		}
	}
	/*
	if(ok > 0 && update > 0)
		ok = UpdateTempTable(&id_list);
	*/
	return (update > 0) ? ok : ((ok <= 0) ? ok : -1);
}
