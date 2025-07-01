// V_OPKIND.CPP
// Copyright (c) A.Starodub 2004, 2006, 2007, 2008, 2009, 2013, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2024, 2025
//
#include <pp.h>
#pragma hdrstop
#include <charry.h>
//
//
//
IMPLEMENT_PPFILT_FACTORY(OprKind); OprKindFilt::OprKindFilt() : PPBaseFilt(PPFILT_OPRKIND, 0, 0)
{
	SetFlatChunk(offsetof(OprKindFilt, ReserveStart),
		offsetof(OprKindFilt, Reserve)-offsetof(OprKindFilt, ReserveStart)+sizeof(Reserve));
	Init(1, 0);
	// Flags |= fUseRank;
}

PPViewOprKind::PPViewOprKind() : PPView(&OpObj, &Filt, PPVIEW_OPRKIND, implBrowseArray, REPORT_OPRKINDLIST), OpListIdx(0), TmplsIdx(0), P_DsList(0)
{
}

PPViewOprKind::~PPViewOprKind()
{
	ZDELETE(P_DsList);
}

int PPViewOprKind::Init_(const PPBaseFilt * pBaseFilt)
{
	int    ok = 1;
	THROW(Helper_InitBaseFilt(pBaseFilt));
	BExtQuery::ZDelete(&P_IterQuery);
	Counter.Init();
	CATCHZOK
	return ok;
}

int PPViewOprKind::InitIteration()
{
	Counter.Init();
	OpListIdx = 0;
	TmplsIdx  = 0;
	ATTmpls.freeAll();
	return 1;
}

int PPViewOprKind::InnerIteration(OprKindViewItem * pItem)
{
	int    ok = -1;
	if(pItem) {
		if(TmplsIdx == 0) {
			PPOprKindPacket op_pack;
			if(OpObj.GetPacket(pItem->ID, &op_pack) > 0)
				ATTmpls.copy(op_pack.ATTmpls);
		}
		if(TmplsIdx < ATTmpls.getCount()) {
			SString dbt_buf, crd_buf, temp_buf;
			PPAccTurnTempl & r_templ = ATTmpls.at(TmplsIdx);
			r_templ.AccTemplToStr(PPDEBIT, dbt_buf);
			r_templ.AccTemplToStr(PPCREDIT, crd_buf);
			temp_buf.Cat(dbt_buf).Space().Cat(crd_buf).Space().Cat(r_templ.Expr).CopyTo(pItem->Template, sizeof(pItem->Template));
			TmplsIdx++;
		}
		if(TmplsIdx >= ATTmpls.getCount()) {
			TmplsIdx = 0;
			ATTmpls.clear();
		}
		else
			ok = 1;
	}
	return ok;
}

int FASTCALL PPViewOprKind::NextIteration(OprKindViewItem * pItem)
{
	int    ok = -1;
	if(pItem && OpListIdx < SVectorBase::GetCount(P_DsList)) {
		const OprKindBrwItem * p_item = static_cast<OprKindBrwItem *>(P_DsList->at(OpListIdx));
		pItem->ID = p_item->ID;
		STRNSCPY(pItem->Name, p_item->Name);
		STRNSCPY(pItem->AccSheet, p_item->AccSheet);
		STRNSCPY(pItem->OpType, p_item->OpType);
		STRNSCPY(pItem->FlagsMnems, p_item->FlagsMnems);
		memzero(pItem->Template, sizeof(pItem->Template));
		if(InnerIteration(pItem) < 0)
			OpListIdx++;
		Counter.Increment();
		ok = 1;
	}
	return ok;
}

int PPViewOprKind::EditBaseFilt(PPBaseFilt * pBaseFilt)
{
	int    ok = -1;
	ushort v = 0;
	TDialog * p_dlg = 0;
	THROW(Filt.IsA(pBaseFilt));
	{
		OprKindFilt * p_filt = static_cast<OprKindFilt *>(pBaseFilt);
		THROW(CheckDialogPtr(&(p_dlg = new TDialog(DLG_OPKINDFLT))));
		SetupPPObjCombo(p_dlg, CTLSEL_OPKINDFLT_TYPE, PPOBJ_OPRTYPE, p_filt->OpTypeID, 0, 0);
		SetupPPObjCombo(p_dlg, CTLSEL_OPKINDFLT_ACCSHT, PPOBJ_ACCSHEET,	p_filt->AccSheetID, OLW_CANINSERT, 0);
		p_dlg->AddClusterAssoc(CTL_OPKINDFLT_FLAGS, 0, OprKindFilt::fOnlyPassive);
		p_dlg->AddClusterAssoc(CTL_OPKINDFLT_FLAGS, 1, OprKindFilt::fOnlyReckon);
		p_dlg->AddClusterAssoc(CTL_OPKINDFLT_FLAGS, 2, OprKindFilt::fOnlyProfitable);
		p_dlg->AddClusterAssoc(CTL_OPKINDFLT_FLAGS, 3, OprKindFilt::fOnlyNeedPayment);
		p_dlg->AddClusterAssoc(CTL_OPKINDFLT_FLAGS, 4, OprKindFilt::fUseRank);
		p_dlg->SetClusterData(CTL_OPKINDFLT_FLAGS, p_filt->Flags);
		if(p_filt->SortOrd == OprKindFilt::sortByTypeName)
			v = 1;
		else
			v = 0;
		p_dlg->setCtrlData(CTL_OPKINDFLT_SORTORD, &v);
		if(ExecView(p_dlg) == cmOK) {
			p_dlg->getCtrlData(CTLSEL_OPKINDFLT_TYPE,   &p_filt->OpTypeID);
			p_dlg->getCtrlData(CTLSEL_OPKINDFLT_ACCSHT, &p_filt->AccSheetID);
			p_dlg->GetClusterData(CTL_OPKINDFLT_FLAGS, &p_filt->Flags);
			p_dlg->getCtrlData(CTL_OPKINDFLT_SORTORD, &v);
			if(v == 1)
				p_filt->SortOrd = OprKindFilt::sortByTypeName;
			else
				p_filt->SortOrd = OprKindFilt::sortByName;
			ok = 1;
		}
	}
	CATCHZOK
	delete p_dlg;
	return ok;
}

int PPViewOprKind::Transmit(PPID id)
{
	int    ok = -1;
	PPIDArray id_list;
	PPWaitStart();
	if(id)
		id_list.add(id);
	else {
		OprKindViewItem item;
		for(InitIteration(); NextIteration(&item) > 0;)
			id_list.add(item.ID);
	}
	THROW(SendCharryObject(PPDS_CRROPRKIND, id_list));
	CATCHZOKPPERR
	PPWaitStop();
	return ok;
}

IMPL_CMPFUNC(OprKindBrwItemName, _i1, _i2)
{
	int    r = 0;
	const OprKindBrwItem * p_item1 = static_cast<const OprKindBrwItem *>(_i1);
	const OprKindBrwItem * p_item2 = static_cast<const OprKindBrwItem *>(_i2);
	if((r = stricmp866(p_item1->Name, p_item2->Name)) < 0)
		return -1;
	else if(r > 0)
		return 1;
	else
		return 0;
}

IMPL_CMPFUNC(OprKindBrwItemNameRank, _i1, _i2)
{
	int    r = 0;
	const OprKindBrwItem * p_item1 = static_cast<const OprKindBrwItem *>(_i1);
	const OprKindBrwItem * p_item2 = static_cast<const OprKindBrwItem *>(_i2);
	if(p_item1->Rank > p_item2->Rank)
		return -1;
	else if(p_item1->Rank < p_item2->Rank)
		return 1;
	else if((r = stricmp866(p_item1->Name, p_item2->Name)) < 0)
		return -1;
	else if(r > 0)
		return 1;
	else
		return 0;
}

IMPL_CMPFUNC(OprKindBrwItemTypeName, _i1, _i2)
{
	int    r = 0;
	const OprKindBrwItem * p_item1 = static_cast<const OprKindBrwItem *>(_i1);
	const OprKindBrwItem * p_item2 = static_cast<const OprKindBrwItem *>(_i2);
	if((r = stricmp866(p_item1->OpType, p_item2->OpType)) < 0)
		return -1;
	else if(r > 0)
		return 1;
	else if((r = stricmp866(p_item1->Name, p_item2->Name)) < 0)
		return -1;
	else if(r > 0)
		return 1;
	else
		return 0;
}

IMPL_CMPFUNC(OprKindBrwItemTypeNameRank, _i1, _i2)
{
	int    r = 0;
	const  OprKindBrwItem * p_item1 = static_cast<const OprKindBrwItem *>(_i1);
	const  OprKindBrwItem * p_item2 = static_cast<const OprKindBrwItem *>(_i2);
	if((r = stricmp866(p_item1->OpType, p_item2->OpType)) < 0)
		return -1;
	else if(r > 0)
		return 1;
	else if(p_item1->Rank > p_item2->Rank)
		return -1;
	else if(p_item1->Rank < p_item2->Rank)
		return 1;
	else if((r = stricmp866(p_item1->Name, p_item2->Name)) < 0)
		return -1;
	else if(r > 0)
		return 1;
	else
		return 0;
}

int PPViewOprKind::CreateFlagsMnemonics(const PPOprKind * pOpData, char * pBuf, size_t bufSize)
{
	SString buf;
	if(pOpData) {
		// set op flags
		static const struct FL { long F; char L; } fl_list[] = {
			{ OPKF_NEEDPAYMENT,   'P' },
			{ OPKF_NOUPDLOTREST,  'x' },
			{ OPKF_ADVACC,        '>' },
			{ OPKF_PROFITABLE,    'G' },
			{ OPKF_ONORDER,       'D' },
			{ OPKF_ORDRESERVE,    'R' },
			{ OPKF_CALCSTAXES,    'V' },
			{ OPKF_OUTBALACCTURN, 'O' },
			{ OPKF_EXTACCTURN,    'E' },
			{ OPKF_EXTAMTLIST,    'L' },
			{ OPKF_RENT,          'N' },
			{ OPKF_NEEDACK,       'A' },
			{ OPKF_RECKON,        'K' },
			{ OPKF_BANKING,       'B' },
			{ OPKF_PASSIVE,       'S' },
			{ OPKF_CURTRANSIT,    'T' }
		};
		for(uint i = 0; i < SIZEOFARRAY(fl_list); i++)
			if(pOpData->Flags & fl_list[i].F)
				buf.CatChar(fl_list[i].L);
		buf.CatChar('-');
		// Set Amount type
		int    use_main_amt = 0;
		long   s, b;
		char symb = 'N';
		if(pOpData->OpTypeID == PPOPT_ACCTURN)
			use_main_amt = 2;
		else if(oneof4(pOpData->OpTypeID, PPOPT_GOODSEXPEND, PPOPT_GOODSRETURN, PPOPT_GOODSREVAL, PPOPT_GOODSORDER))
			use_main_amt = 1;
		else
			use_main_amt = 0;
		s = (pOpData->Flags & OPKF_SELLING);
		b = (pOpData->Flags & OPKF_BUYING);
		if(use_main_amt) {
			if(use_main_amt == 1)
				symb = (s ^ b) ? (b ? 'C' : 'P') : 'N';
			else if(s)
				symb = 'C';
			else
				symb = 'P';
		}
		buf.CatChar(symb);
		// Set Sub type
		{
			char c = 0;
			switch(pOpData->SubType) {
				case OPSUBT_COMMON: c = 'S'; break;
				case OPSUBT_ADVANCEREP: c = 'A'; break;
				case OPSUBT_REGISTER: c = 'R'; break;
				case OPSUBT_ASSETRCV: c = 'G'; break;
				case OPSUBT_ASSETEXPL: c = 'E'; break;
				case OPSUBT_WARRANT: c = 'W'; break;
			}
			if(c)
				buf.CatChar('-').CatChar(c);
		}
	}
	buf.CopyTo(pBuf, bufSize);
	return 1;
}

int PPViewOprKind::OnExecBrowser(PPViewBrowser * pBrw)
{
	pBrw->SetupToolbarCombo(PPOBJ_OPRTYPE, Filt.OpTypeID, 0, 0);
	return -1;
}

/*virtual*/void * PPViewOprKind::GetEditExtraParam() { return Filt.OpTypeID ? reinterpret_cast<void *>(Filt.OpTypeID) : 0; }

int PPViewOprKind::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	PPID   id = 0;
	if(ok == -2) {
		id = pHdr ? *static_cast<const  PPID *>(pHdr) : 0;
		switch(ppvCmd) {
			case PPVCMD_USERSORT: ok = 1; break; // The rest will be done below
			case PPVCMD_ADDBYSAMPLE:
				if(id && AddBySample(id) > 0)
					ok = 1;
				else
					ok = -1;
				break;
			case PPVCMD_VIEWLINKOPS:
				if(id)
					ViewLinkOps(id);
				ok = -1;
				break;
			case PPVCMD_VIEWBILLS:
				if(id)
					ViewBills(id);
				ok = -1;
				break;
			case PPVCMD_TRANSMITCHARRY:
				ok = -1;
				Transmit(0);
				break;
			case PPVCMD_TB_CBX_SELECTED:
				{
					ok = -1;
					PPID   op_type_id = 0;
					if(pBrw && pBrw->GetToolbarComboData(&op_type_id) && Filt.OpTypeID != op_type_id) {
						Filt.OpTypeID = op_type_id;
						ok = ChangeFilt(1, pBrw);
					}
				}
				break;
		}
	}
	if(ok > 0 && oneof6(ppvCmd, PPVCMD_USERSORT, PPVCMD_ADDITEM, PPVCMD_ADDBYSAMPLE, PPVCMD_EDITITEM, PPVCMD_DELETEITEM, PPVCMD_REFRESH)) {
		AryBrowserDef * p_def = static_cast<AryBrowserDef *>(pBrw->getDef());
		if(p_def) {
			if(MakeList(pBrw)) {
				LongArray last_upd_obj_list;
				SArray * p_array = new SArray(*P_DsList);
				long   c = p_def->_curItem();
				p_def->setArray(p_array, 0, 1);
				if(ppvCmd == PPVCMD_USERSORT && id) {
					pBrw->search2(&id, CMPF_LONG, srchFirst, 0);
				}
				else if(!oneof2(ppvCmd, PPVCMD_DELETEITEM, PPVCMD_REFRESH) && GetLastUpdatedObjects(0, last_upd_obj_list) > 0) {
					assert(last_upd_obj_list.getCount());
					last_upd_obj_list.sortAndUndup();
					pBrw->search2(&last_upd_obj_list.at(0), CMPF_LONG, srchFirst, 0);
				}
				else
					pBrw->go(c);
			}
		}
		ok = 1;
	}
	return ok;
}

int PPViewOprKind::MakeEntry(const PPOprKind & rOpRec, OprKindBrwItem & rEntry)
{
	int    ok = -1;
	bool   accept_op = false;
	MEMSZERO(rEntry);
	accept_op = (CheckFiltID(Filt.OpTypeID, rOpRec.OpTypeID) && CheckFiltID(Filt.LinkOpID, rOpRec.LinkOpID));
	accept_op = (accept_op && (!Filt.AccSheetID || Filt.AccSheetID == rOpRec.AccSheetID));
	if(Filt.Flags & OprKindFilt::fOnlyPassive)
		accept_op = (accept_op && (rOpRec.Flags & OPKF_PASSIVE));
	if(Filt.Flags & OprKindFilt::fOnlyProfitable)
		accept_op = (accept_op &&  (rOpRec.Flags & OPKF_PROFITABLE));
	if(Filt.Flags & OprKindFilt::fOnlyNeedPayment)
		accept_op = (accept_op && (rOpRec.Flags & OPKF_NEEDPAYMENT));
	if(Filt.Flags & OprKindFilt::fOnlyReckon)
		accept_op = (accept_op && (rOpRec.Flags & OPKF_RECKON));
	if(accept_op) {
		SString temp_buf;
		PPAccSheet acs_rec;
		rEntry.ID = rOpRec.ID;
		STRNSCPY(rEntry.Name, rOpRec.Name);
		STRNSCPY(rEntry.Symb, rOpRec.Symb);
		if(AcsObj.Fetch(rOpRec.AccSheetID, &acs_rec) > 0)
			STRNSCPY(rEntry.AccSheet, acs_rec.Name);
		GetObjectName(PPOBJ_OPRTYPE, rOpRec.OpTypeID, temp_buf.Z());
		STRNSCPY(rEntry.OpType, temp_buf);
		CreateFlagsMnemonics(&rOpRec, rEntry.FlagsMnems, sizeof(rEntry.FlagsMnems));
		rEntry.Rank = rOpRec.Rank;
		ok = 1;
	}
	return ok;
}

int PPViewOprKind::CmpSortIndexItems(PPViewBrowser * pBrw, const OprKindBrwItem * pItem1, const OprKindBrwItem * pItem2)
{
	return Implement_CmpSortIndexItems_OnArray(pBrw, pItem1, pItem2);
}

static IMPL_CMPFUNC(PPViewOprKindBrwItem, i1, i2)
{
	int    si = 0;
	PPViewBrowser * p_brw = static_cast<PPViewBrowser *>(pExtraData);
	if(p_brw) {
		PPViewOprKind * p_view = static_cast<PPViewOprKind *>(p_brw->P_View);
		if(p_view) {
			const OprKindBrwItem * p_item1 = static_cast<const OprKindBrwItem *>(i1);
			const OprKindBrwItem * p_item2 = static_cast<const OprKindBrwItem *>(i2);
			si = p_view->CmpSortIndexItems(p_brw, p_item1, p_item2);
		}
	}
	return si;
}

int PPViewOprKind::MakeList(PPViewBrowser * pBrw)
{
	int    ok = 1;
	const  bool is_sorting_needed = (pBrw && pBrw->GetSettledOrderList().getCount());
	PPID   op_id = 0;
	PPOprKind op_data;
	OprKindBrwItem _e;
	if(P_DsList)
		P_DsList->clear();
	else {
		THROW_MEM(P_DsList = new SArray(sizeof(OprKindBrwItem)));
	}
	for(SEnum en = OpObj.Enum(0); en.Next(&op_data) > 0;) {
		if(MakeEntry(op_data, _e) > 0) {
			THROW_SL(P_DsList->insert(&_e));
		}
	}
	{
		int    sorting_done = 0;
		if(pBrw) {
			pBrw->Helper_SetAllColumnsSortable();
			if(is_sorting_needed) {
				P_DsList->sort(PTR_CMPFUNC(PPViewOprKindBrwItem), pBrw);
				sorting_done = 1;
			}
		}
		if(!sorting_done) {
			if(Filt.Flags & OprKindFilt::fUseRank) {
				if(Filt.SortOrd == OprKindFilt::sortByTypeName)
					P_DsList->sort(PTR_CMPFUNC(OprKindBrwItemTypeNameRank));
				else
					P_DsList->sort(PTR_CMPFUNC(OprKindBrwItemNameRank));
			}
			else if(Filt.SortOrd == OprKindFilt::sortByTypeName)
				P_DsList->sort(PTR_CMPFUNC(OprKindBrwItemTypeName));
			else
				P_DsList->sort(PTR_CMPFUNC(OprKindBrwItemName));
		}
	}
	CATCH
		ZDELETE(P_DsList);
		ok = 0;
	ENDCATCH
	return ok;
}

void PPViewOprKind::PreprocessBrowser(PPViewBrowser * pBrw)
{
	CALLPTRMEMB(pBrw, Helper_SetAllColumnsSortable());
}

SArray * PPViewOprKind::CreateBrowserArray(uint * pBrwId, SString * pSubTitle)
{
	SArray * p_array = 0;
	THROW(MakeList(0));
	THROW_MEM(p_array = new SArray(sizeof(OprKindBrwItem)));
	*p_array = *P_DsList;
	CATCH
		ZDELETE(P_DsList);
		ZDELETE(p_array);
	ENDCATCH
	ASSIGN_PTR(pBrwId, BROWSER_OPRKIND);
	return p_array;
}

int PPViewOprKind::AddBySample(PPID sampleID)
{
	PPID   id = 0;
	return (OpObj.AddBySample(&id, sampleID) == cmOK) ? 1 : -1;
}

int PPViewOprKind::ViewLinkOps(PPID opID)
{
	OprKindFilt filt;
	filt.LinkOpID = opID;
	return PPView::Execute(PPVIEW_OPRKIND, &filt, 1, 0);
}

int PPViewOprKind::ViewBills(PPID opID)
{
	if(opID) {
		BillFilt flt;
		switch(GetOpType(opID)) {
			case PPOPT_DRAFTEXPEND:
			case PPOPT_DRAFTRECEIPT:
			case PPOPT_DRAFTQUOTREQ: // @v10.5.7
			case PPOPT_DRAFTTRANSIT: flt.Bbt = bbtDraftBills; break;
			case PPOPT_ACCTURN: flt.Bbt = bbtAccturnBills; break;
			case PPOPT_INVENTORY: flt.Bbt = bbtInventoryBills; break;
			case PPOPT_GOODSORDER: flt.Bbt = bbtOrderBills; break;
			case PPOPT_POOL: flt.Bbt = bbtPoolBills;
		}
		flt.SetupBrowseBillsType(flt.Bbt);
		flt.OpID = opID;
		{
			BillFilt::FiltExtraParam p(0, flt.Bbt);
			PPView::Execute(PPVIEW_BILL, &flt, (GetModelessStatus() ? PPView::exefModeless : 0), &p);
		}
	}
	return -1;
}
//
// Implementation of PPALDD_OprKindList
//
PPALDD_CONSTRUCTOR(OprKindList)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		AssignDefIterData(&I, sizeof(I));
	}
}

PPALDD_DESTRUCTOR(OprKindList) { Destroy(); }

int PPALDD_OprKindList::InitData(PPFilt & rFilt, long rsrv)
{
	INIT_PPVIEW_ALDD_DATA_U(OprKind, rsrv);
	H.FltReckon      = BIN(p_filt->Flags & OprKindFilt::fOnlyReckon);
	H.FltProfitable  = BIN(p_filt->Flags & OprKindFilt::fOnlyProfitable);
	H.FltNeedPayment = BIN(p_filt->Flags & OprKindFilt::fOnlyNeedPayment);
	H.FltPassive     = BIN(p_filt->Flags & OprKindFilt::fOnlyPassive);
	H.FltAccSheet    = p_filt->AccSheetID;
	H.FltOpType      = p_filt->OpTypeID;
	{
		SString temp_buf;
		GetObjectName(PPOBJ_OPRTYPE, p_filt->OpTypeID, temp_buf.Z());
		STRNSCPY(H.FltOpTypeName, temp_buf);
	}
	return DlRtm::InitData(rFilt, rsrv);
}

void PPALDD_OprKindList::Destroy() { DESTROY_PPVIEW_ALDD(OprKind); }
int  PPALDD_OprKindList::InitIteration(PPIterID iterId, int sortId, long /*rsrv*/) { INIT_PPVIEW_ALDD_ITER(OprKind); }

int PPALDD_OprKindList::NextIteration(PPIterID iterId)
{
	START_PPVIEW_ALDD_ITER(OprKind);
	I.OpID = item.ID;
	STRNSCPY(I.FlagsMnemonics, item.FlagsMnems);
	STRNSCPY(I.OpType, item.OpType);
	STRNSCPY(I.Template, item.Template);
	FINISH_PPVIEW_ALDD_ITER();
}

