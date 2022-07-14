// V_ACCT.CPP
// Copyright (c) A.Sobolev, A.Starodub 2000-2002, 2003, 2004, 2005, 2006, 2007, 2008, 2011, 2016, 2018, 2019, 2020, 2021
// @codepage UTF-8
// PPViewAccount
//
#include <pp.h>
#pragma hdrstop
#include <charry.h>

IMPLEMENT_PPFILT_FACTORY(Account); AccountFilt::AccountFilt() : PPBaseFilt(PPFILT_ACCOUNT, 0, 0)
{
	SetFlatChunk(offsetof(AccountFilt, ReserveStart),
		offsetof(AccountFilt, Reserve)-offsetof(AccountFilt, ReserveStart)+sizeof(Reserve));
	Init(1, 0);
}

AccountFilt & FASTCALL AccountFilt::operator = (const AccountFilt & s)
{
	Copy(&s, 0);
	return *this;
}

PPViewAccount::PPViewAccount() : PPView(&AccObj, &Filt, PPVIEW_ACCOUNT, implBrowseArray, REPORT_ACCOUNTVIEW), P_AnlzFilt(0)
{
}

PPViewAccount::~PPViewAccount()
{
	delete P_AnlzFilt;
}

/*virtual*/void * PPViewAccount::GetEditExtraParam()
{
	return reinterpret_cast<void *>(Filt.Type);
}

/*virtual*/PPBaseFilt * PPViewAccount::CreateFilt(const void * extraPtr) const
{
	AccountFilt * p_filt = 0;
	if(PPView::CreateFiltInstance(PPFILT_ACCOUNT, reinterpret_cast<PPBaseFilt **>(&p_filt)))
		p_filt->Type = (reinterpret_cast<long>(extraPtr));
	return static_cast<PPBaseFilt *>(p_filt);
}

/*virtual*/int PPViewAccount::EditBaseFilt(PPBaseFilt * pBaseFilt)
{
	int    ok = -1;
	uint   v = 0;
	if(Filt.IsA(pBaseFilt)) {
		AccountFilt * p_filt = static_cast<AccountFilt *>(pBaseFilt);
		switch(p_filt->Type) {
			case ACY_BAL:  v = 0; break;
			case ACY_OBAL: v = 1; break;
			case ACY_REGISTER: v = 2; break;
			case ACY_AGGR: v = 3; break;
		}
		if(SelectorDialog(DLG_SELACTYP, CTL_SELACTYP_TYP, &v) > 0) {
			switch(v) {
				case 0: p_filt->Type = ACY_BAL; break;
				case 1: p_filt->Type = ACY_OBAL; break;
				case 2: p_filt->Type = ACY_REGISTER; break;
				case 3: p_filt->Type = ACY_AGGR; break;
			}
			ok = 1;
		}
	}
	else
		ok = 0;
	return ok;
}

int PPViewAccount::ViewArticles(PPID accID)
{
	PPAccount acc_rec;
	if(accID && AccObj.Search(accID, &acc_rec) > 0 && acc_rec.AccSheetID) {
		ArticleFilt ar_filt;
		ar_filt.AccSheetID = acc_rec.AccSheetID;
		ShowObjects(PPOBJ_ARTICLE, &ar_filt);
	}
	return -1;
}

int PPViewAccount::ViewAccAnalyze(PPID accID)
{
	PPAccount acc_rec;
	if(accID && AccObj.Search(accID, &acc_rec) > 0) {
		PPViewAccAnlz * p_temp_view = new PPViewAccAnlz;
		if(P_AnlzFilt == 0) {
			P_AnlzFilt = static_cast<AccAnlzFilt *>(p_temp_view->CreateFilt(reinterpret_cast<void *>(aakndGeneric)));
			P_AnlzFilt->Period.Z();
		}
		P_AnlzFilt->AccID = accID;
		P_AnlzFilt->AcctId.ac = accID;
		P_AnlzFilt->AccSheetID = acc_rec.AccSheetID;
		if(acc_rec.A.Sb == 0)
			P_AnlzFilt->Aco = ACO_1;
		else
			P_AnlzFilt->Aco = ACO_2;
		if(p_temp_view->EditBaseFilt(P_AnlzFilt) > 0) {
			ZDELETE(p_temp_view);
			ViewAccAnlz(P_AnlzFilt, aakndGeneric);
		}
		else
			delete p_temp_view;
	}
	return -1;
}

/*static*/void FASTCALL PPViewAccount::MakeListEntry(const PPAccount & rSrc, PPViewAccount::BrwEntry & rEntry)
{
	MEMSZERO(rEntry);
	rEntry.ID = rSrc.ID;
	rEntry.Type = rSrc.Type;
	rEntry.Kind = rSrc.Kind;
	rEntry.A = rSrc.A;
	rEntry.CurID = rSrc.CurID;
	rEntry.ParentID = rSrc.ParentID;
	rEntry.AccSheetID = rSrc.AccSheetID;
	rEntry.MainOrgID = rSrc.MainOrgID;
	rEntry.Flags = rSrc.Flags;
	rEntry.OpenDate = rSrc.OpenDate;
	rEntry.Frrl_Date = rSrc.Frrl_Date;
	rEntry.Limit = rSrc.Limit;
	rEntry.Overdraft = rSrc.Overdraft;
	STRNSCPY(rEntry.Code, rSrc.Code);
}

int FASTCALL PPViewAccount::CheckForFilt(const PPAccount & rItem) const
{
	return BIN(rItem.CurID == 0 && CheckFiltID(Filt.Type, rItem.Type) && ObjRts.CheckAccID(rItem.ID, PPR_READ));
}

IMPL_CMPCFUNC(PPViewAccount_BrwEntry_A, p1, p2) { RET_CMPCASCADE2(static_cast<const PPViewAccount::BrwEntry *>(p1), static_cast<const PPViewAccount::BrwEntry *>(p2), A.Ac, A.Sb); }

int PPViewAccount::FetchData(long id)
{
	int    ok = 1;
	BrwEntry entry;
	if(id == 0) {
		Data.clear();
		PPAccount rec;
		for(SEnum en = AccObj.Enum(0); en.Next(&rec) > 0;) {
			if(CheckForFilt(rec)) {
				MakeListEntry(rec, entry);
                THROW_SL(Data.insert(&entry));
			}
		}
	}
	else {
		uint   pos = 0;
		const  bool found = Data.lsearch(&id, &pos, CMPF_LONG);
		PPAccount rec;
		if(AccObj.Search(id, &rec) > 0 && CheckForFilt(rec) > 0) {
			MakeListEntry(rec, entry);
			if(found)
				Data.at(pos) = entry;
			else
				THROW_SL(Data.insert(&entry));
		}
		else if(found)
			THROW_SL(Data.atFree(pos));
	}
	Data.sort(PTR_CMPCFUNC(PPViewAccount_BrwEntry_A));
	CATCHZOK
	return ok;
}

/*virtual*/int PPViewAccount::Init_(const PPBaseFilt * pFilt)
{
	int    ok = 1;
	THROW(Helper_InitBaseFilt(pFilt));
	THROW(FetchData(0));
	CATCHZOK
	return ok;
}

int PPViewAccount::InitIteration()
{
	int    ok = 1;
	Counter.Init(Data.getCount());
	return ok;
}

int FASTCALL PPViewAccount::NextIteration(AccountViewItem * pItem)
{
	int    ok = -1;
	PPObjCurrency cur_obj;
	for(; ok < 0 && Counter < Data.getCount(); Counter.Increment()) {
		const BrwEntry & r_entry = Data.at(Counter);
		if(pItem) {
			PPAccount acc_rec;
			if(AccObj.Search(r_entry.ID, &acc_rec) > 0) {
				SString temp_buf;
				PPIDArray cur_id_list;
				AccObj.GetCurList(acc_rec.ID, 0, &cur_id_list);
				for(uint i = 0; i < cur_id_list.getCount(); i++) {
					PPCurrency cur_rec;
					if(cur_obj.Fetch(cur_id_list.get(i), &cur_rec) > 0)
						temp_buf.CatDiv(',', 1).Cat(cur_rec.Symb);
				}
                *static_cast<PPAccount *>(pItem) = acc_rec;
				STRNSCPY(pItem->CurList, temp_buf);
				ok = 1;
			}
		}
		else
			ok = 1;
	}
	return ok;
}

/*static*/int FASTCALL PPViewAccount::GetDataForBrowser(SBrowserDataProcBlock * pBlk)
{
	PPViewAccount * p_v = static_cast<PPViewAccount *>(pBlk->ExtraPtr);
	return p_v ? p_v->_GetDataForBrowser(pBlk) : 0;
}

int PPViewAccount::_GetDataForBrowser(SBrowserDataProcBlock * pBlk)
{
	int    ok = 0;
	if(pBlk->P_SrcData && pBlk->P_DestData) {
		ok = 1;
		SString temp_buf;
		const BrwEntry * p_item = static_cast<const BrwEntry *>(pBlk->P_SrcData);
		switch(pBlk->ColumnN) {
			case 0: // ИД
				pBlk->Set(p_item->ID);
				break;
			case 1: // Номер счета
				pBlk->Set((long)p_item->A.Ac);
				break;
			case 2: // Номер субсчета
				pBlk->Set((long)p_item->A.Sb);
				break;
			case 3: // Вид счета
				PPGetSubStrById(PPTXT_ACCKIND, p_item->Kind, temp_buf);
				pBlk->Set(temp_buf);
				break;
			case 4:
				{
					PPAccount acc_rec;
					if(AccObj.Fetch(p_item->ID, &acc_rec) > 0)
						temp_buf = acc_rec.Name;
					else
						ideqvalstr(p_item->ID, temp_buf.Z());
				}
				pBlk->Set(temp_buf);
				break;
			case 5:
				temp_buf.Z();
				if(p_item->AccSheetID) {
					PPObjAccSheet acs_obj;
					PPAccSheet acs_rec;
					if(acs_obj.Fetch(p_item->AccSheetID, &acs_rec) > 0)
						temp_buf = acs_rec.Name;
					else
						ideqvalstr(p_item->AccSheetID, temp_buf);
				}
				pBlk->Set(temp_buf);
				break;
			case 6:
				pBlk->Set(temp_buf.Z().CatHex(p_item->Flags));
				break;
			case 7:
				pBlk->Set(p_item->Frrl_Date);
				break;
			case 8:
				pBlk->Set(p_item->Code);
				break;
		}
	}
	return ok;
}

void PPViewAccount::PreprocessBrowser(PPViewBrowser * pBrw)
{
	CALLPTRMEMB(pBrw, SetDefUserProc(PPViewAccount::GetDataForBrowser, this));
}

SArray * PPViewAccount::CreateBrowserArray(uint * pBrwId, SString * pSubTitle)
{
	SArray * p_array = new SArray(Data);
	uint   brw_id = 0;
	if(oneof2(Filt.Type, ACY_REGISTER, ACY_BUDGET))
		brw_id = BROWSER_ACCREGISTER;
	else
		brw_id = PPMaster ? BROWSER_ACCOUNT_SYS : BROWSER_ACCOUNT;
	if(pSubTitle)
		PPGetSubStr(PPTXT_ACCTITLES, Filt.Type, *pSubTitle);
	ASSIGN_PTR(pBrwId, brw_id);
	return p_array;
}

#if 0 // {

static void dbqf_account_checkrights_i(int option, DBConst * result, DBConst * params)
{
	long   ok = 1;
	PPID   acc_id = params[0].lval;
	if(!ObjRts.CheckAccID(acc_id, PPR_READ))
		ok = 0;
	result->init(ok);
}

/*static*/int PPViewAccount::DynFuncCheckRights = 0;

/*virtual*/DBQuery * PPViewAccount::CreateBrowserQuery(uint * pBrwId, SString * pSubTitle)
{
	DbqFuncTab::RegisterDyn(&DynFuncCheckRights, BTS_INT, dbqf_account_checkrights_i, 1, BTS_INT);

	static DbqStringSubst acckind_subst(3);  // @global @threadsafe
	uint    brw_id = 0;
	if(oneof2(Filt.Type, ACY_REGISTER, ACY_BUDGET))
		brw_id = BROWSER_ACCREGISTER;
	else
		brw_id = PPMaster ? BROWSER_ACCOUNT_SYS : BROWSER_ACCOUNT;
	DBQuery * q = 0;
	DBE * dbe;
	DBQ * dbq = 0;
	DBE   dbe_acs;
	DBE   dbe_restr;
	AccountTbl * a = new AccountTbl;
	THROW(CheckTblPtr(a));
	PPDbqFuncPool::InitObjNameFunc(dbe_acs, PPDbqFuncPool::IdObjNameAccSheet, a->AccSheetID);
	{
		dbe_restr.init();
		dbe_restr.push(a->ID);
		dbe_restr.push(static_cast<DBFunc>(PPViewAccount::DynFuncCheckRights));
	}
	dbe = & enumtoa(a->Kind, 3, acckind_subst.Get(PPTXT_ACCKIND));
	dbq = &(a->CurID == 0L);
	dbq = ppcheckfiltid(dbq, a->Type, Filt.Type);
	dbq = &(*dbq && dbe_restr > 0L);
	q = &select(
		a->ID,
		a->Ac,
		a->Sb,
		*dbe,
		a->Name,
		dbe_acs,
		a->Flags,
		a->FRRL_Date,
		a->Code,
		0L).from(a, 0L).where(*dbq);
	delete dbe;
	if(Filt.Type == ACY_BAL || Filt.Type == ACY_OBAL)
		q->orderBy(a->Ac, a->Sb, 0L);
	else
		q->orderBy(a->Code, a->Ac, a->Sb, 0L);
	THROW(CheckQueryPtr(q));
	if(pSubTitle)
		PPGetSubStr(PPTXT_ACCTITLES, Filt.Type, *pSubTitle);
	CATCH
		if(q)
			ZDELETE(q);
		else
			delete a;
	ENDCATCH
	ASSIGN_PTR(pBrwId, brw_id);
	return q;
}

#endif // } 0

int PPViewAccount::Transmit(PPID id, int transmitKind)
{
	int    ok = -1;
	if(transmitKind == 0) {
		ObjTransmitParam param;
		if(ObjTransmDialog(DLG_OBJTRANSM, &param) > 0) {
			AccountViewItem item;
			const PPIDArray & rary = param.DestDBDivList.Get();
			PPObjIDArray objid_ary;
			PPWaitStart();
			for(InitIteration(); NextIteration(&item) > 0; PPWaitPercent(GetCounter()))
				objid_ary.Add(PPOBJ_ACCOUNT2, item.ID);
			THROW(PPObjectTransmit::Transmit(&rary, &objid_ary, &param));
			ok = 1;
		}
	}
	else if(transmitKind == 1) {
		PPIDArray id_list;
		PPWaitStart();
		if(id)
			id_list.add(id);
		else {
			AccountViewItem item;
			for(InitIteration(); NextIteration(&item) > 0;)
				id_list.add(item.ID);
		}
		THROW(SendCharryObject(PPDS_CRRACCOUNT, id_list));
		PPWaitStop();
	}
	CATCHZOKPPERR
	return ok;
}

/*virtual*/int PPViewAccount::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	Hdr    hdr;
	if(!RVALUEPTR(hdr, static_cast<const Hdr *>(pHdr)))
		MEMSZERO(hdr);
	int    ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	if(ok == -2) {
		switch(ppvCmd) {
			case PPVCMD_ACCANLZ:
				ok = ViewAccAnalyze(hdr.Id);
				break;
			case PPVCMD_ARTICLES:
				ok = ViewArticles(hdr.Id);
				break;
			case PPVCMD_TRANSMITCHARRY:
				ok = Transmit(0, 1);
				break;
			case PPVCMD_TRANSMIT:
				ok = Transmit(0, 0);
				break;
		}
	}
	if(ok > 0) {
		AryBrowserDef * p_def = static_cast<AryBrowserDef *>(pBrw->getDef());
		if(p_def) {
			LongArray id_list;
			PPID   last_id = 0;
			if(GetLastUpdatedObjects(hdr.Id, id_list) > 0) {
				for(uint i = 0; i < id_list.getCount(); i++) {
					last_id = id_list.get(i);
					FetchData(last_id);
				}
			}
			if(last_id) {
				p_def->setArray(new SArray(Data), 0, 1);
				if(ppvCmd != PPVCMD_DELETEITEM)
					pBrw->search2(&last_id, CMPF_LONG, srchFirst, 0);
			}
		}
	}
	return ok;
}
