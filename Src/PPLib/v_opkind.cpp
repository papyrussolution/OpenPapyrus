// V_OPKIND.CPP
// Copyright (c) A.Starodub 2004, 2006, 2007, 2008, 2009, 2013, 2015, 2016, 2017, 2018, 2019
//
#include <pp.h>
#pragma hdrstop
#include <charry.h>
//
//
//
IMPLEMENT_PPFILT_FACTORY(OprKind); SLAPI OprKindFilt::OprKindFilt() : PPBaseFilt(PPFILT_OPRKIND, 0, 0)
{
	SetFlatChunk(offsetof(OprKindFilt, ReserveStart),
		offsetof(OprKindFilt, Reserve)-offsetof(OprKindFilt, ReserveStart)+sizeof(Reserve));
	Init(1, 0);
	// Flags |= OPKF_USERANK;
}

SLAPI PPViewOprKind::PPViewOprKind() : PPView(&OpkObj, &Filt, PPVIEW_OPRKIND), OpListIdx(0), TmplsIdx(0), P_OpList(0)
{
	DefReportId = REPORT_OPRKINDLIST;
	ImplementFlags |= (implBrowseArray|implOnAddSetupPos); // @v10.0.06 @fix (|implOnAddSetupPos)
}

SLAPI PPViewOprKind::~PPViewOprKind()
{
	ZDELETE(P_OpList);
}

int SLAPI PPViewOprKind::Init_(const PPBaseFilt * pBaseFilt)
{
	int    ok = 1;
	THROW(Helper_InitBaseFilt(pBaseFilt));
	BExtQuery::ZDelete(&P_IterQuery);
	Counter.Init();
	CATCHZOK
	return ok;
}

int SLAPI PPViewOprKind::InitIteration()
{
	Counter.Init();
	OpListIdx = 0;
	TmplsIdx  = 0;
	ATTmpls.freeAll();
	return 1;
}

int SLAPI PPViewOprKind::InnerIteration(OprKindViewItem * pItem)
{
	int    ok = -1;
	if(pItem) {
		if(TmplsIdx == 0) {
			PPOprKindPacket opk_pack;
			if(OpkObj.GetPacket(pItem->ID, &opk_pack) > 0)
				ATTmpls.copy(opk_pack.ATTmpls);
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
	if(pItem && OpListIdx < P_OpList->getCount()) {
		OprKindBrwItem * p_item = (OprKindBrwItem*)P_OpList->at(OpListIdx);
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

int SLAPI PPViewOprKind::EditBaseFilt(PPBaseFilt * pBaseFilt)
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
		p_dlg->AddClusterAssoc(CTL_OPKINDFLT_FLAGS, 0, OPKF_PASSIVE);
		p_dlg->AddClusterAssoc(CTL_OPKINDFLT_FLAGS, 1, OPKF_RECKON);
		p_dlg->AddClusterAssoc(CTL_OPKINDFLT_FLAGS, 2, OPKF_PROFITABLE);
		p_dlg->AddClusterAssoc(CTL_OPKINDFLT_FLAGS, 3, OPKF_NEEDPAYMENT);
		p_dlg->AddClusterAssoc(CTL_OPKINDFLT_FLAGS, 4, OPKF_USERANK);
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

int SLAPI PPViewOprKind::Transmit(PPID id)
{
	int    ok = -1;
	PPIDArray id_list;
	PPWait(1);
	if(id)
		id_list.add(id);
	else {
		OprKindViewItem item;
		for(InitIteration(); NextIteration(&item) > 0;)
			id_list.add(item.ID);
	}
	THROW(SendCharryObject(PPDS_CRROPRKIND, id_list));
	CATCHZOKPPERR
	PPWait(0);
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

int SLAPI PPViewOprKind::CreateFlagsMnemonics(const PPOprKind * pOpData, char * pBuf, size_t bufSize)
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
		buf.CatChar('-');
		if(pOpData->SubType == OPSUBT_COMMON)
			buf.CatChar('S');
		else if(pOpData->SubType == OPSUBT_ADVANCEREP)
			buf.CatChar('A');
		else if(pOpData->SubType == OPSUBT_REGISTER)
			buf.CatChar('R');
		else if(pOpData->SubType == OPSUBT_ASSETRCV)
			buf.CatChar('G');
		else if(pOpData->SubType == OPSUBT_ASSETEXPL)
			buf.CatChar('E');
		else if(pOpData->SubType == OPSUBT_WARRANT)
			buf.CatChar('W');
	}
	buf.CopyTo(pBuf, bufSize);
	return 1;
}

int SLAPI PPViewOprKind::OnExecBrowser(PPViewBrowser * pBrw)
{
	pBrw->SetupToolbarCombo(PPOBJ_OPRTYPE, Filt.OpTypeID, 0, 0);
	return -1;
}

int SLAPI PPViewOprKind::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	if(ok == -2) {
		PPID   id = pHdr ? *static_cast<const PPID *>(pHdr) : 0;
		switch(ppvCmd) {
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
	return ok;
}

SArray * SLAPI PPViewOprKind::CreateBrowserArray(uint * pBrwId, SString * pSubTitle)
{
	uint   brw_id = BROWSER_OPRKIND;
	PPID   op_id = 0;
	PPOprKind op_data;
	SArray * p_array = 0;
	OprKindBrwItem _e;
	if(!P_OpList) {
		THROW_MEM(P_OpList = new SArray(sizeof(OprKindBrwItem)));
	}
	else
		P_OpList->freeAll();
	THROW_MEM(p_array = new SArray(sizeof(OprKindBrwItem)));
	while(EnumOperations(Filt.OpTypeID, &op_id, &op_data) > 0) {
		int    accept_op = 0;
		MEMSZERO(_e);
		accept_op = (!Filt.LinkOpID || Filt.LinkOpID == op_data.LinkOpID);
		accept_op = (accept_op && (!Filt.AccSheetID || Filt.AccSheetID == op_data.AccSheetID));
		if(Filt.Flags & OPKF_PASSIVE)
			accept_op = (accept_op && (op_data.Flags & OPKF_PASSIVE));
		if(Filt.Flags & OPKF_PROFITABLE)
			accept_op = (accept_op &&  (op_data.Flags & OPKF_PROFITABLE));
		if(Filt.Flags & OPKF_NEEDPAYMENT)
			accept_op = (accept_op && (op_data.Flags & OPKF_NEEDPAYMENT));
		if(Filt.Flags & OPKF_RECKON)
			accept_op = (accept_op && (op_data.Flags & OPKF_RECKON));
		if(accept_op) {
			_e.ID = op_id;
			STRNSCPY(_e.Name, op_data.Name);
			STRNSCPY(_e.Symb, op_data.Symb);
			GetObjectName(PPOBJ_OPRTYPE, GetOpType(_e.ID), _e.OpType, sizeof(_e.OpType));
			GetObjectName(PPOBJ_ACCSHEET, op_data.AccSheetID, _e.AccSheet, sizeof(_e.AccSheet) - 1);
			CreateFlagsMnemonics(&op_data, _e.FlagsMnems, sizeof(_e.FlagsMnems));
			_e.Rank = op_data.Rank;
			THROW_SL(p_array->insert(&_e));
		}
	}
	if(Filt.Flags & OPKF_USERANK) {
		if(Filt.SortOrd == OprKindFilt::sortByTypeName)
			p_array->sort(PTR_CMPFUNC(OprKindBrwItemTypeNameRank));
		else
			p_array->sort(PTR_CMPFUNC(OprKindBrwItemNameRank));
	}
	else if(Filt.SortOrd == OprKindFilt::sortByTypeName)
		p_array->sort(PTR_CMPFUNC(OprKindBrwItemTypeName));
	else
		p_array->sort(PTR_CMPFUNC(OprKindBrwItemName));
	P_OpList->copy(*p_array);
	CATCH
		ZDELETE(P_OpList);
		ZDELETE(p_array);
	ENDCATCH
	ASSIGN_PTR(pBrwId, brw_id);
	return p_array;
}

/*int SLAPI PPViewOprKind::AddItem()
{
	int    ok = -1;
	PPID   id = 0;
	if(OpkObj.Edit(&id, Filt.OpTypeID, Filt.LinkOpID) == cmOK)
		ok = 1;
	return ok;
}*/

/*int SLAPI PPViewOprKind::EditItem(PPID id)
{
	int    ok = -1;
	if(id) {
		if(OpkObj.Edit(&id, 0, 0) == cmOK)
			ok = 1;
	}
	return ok;
}*/

int SLAPI PPViewOprKind::AddBySample(PPID sampleID)
{
	PPID   id = 0;
	return (OpkObj.AddBySample(&id, sampleID) == cmOK) ? 1 : -1;
}

/*int SLAPI PPViewOprKind::DeleteItem(PPID opID)
{
	return opID ? OpkObj.RemoveObjV(opID, 0, PPObject::rmv_default, 0) : -1;
}*/

int SLAPI PPViewOprKind::ViewLinkOps(PPID opID)
{
	OprKindFilt filt;
	//filt.Init();
	filt.LinkOpID = opID;
	return PPView::Execute(PPVIEW_OPRKIND, &filt, 1, 0);
}

int SLAPI PPViewOprKind::ViewBills(PPID opID)
{
	if(opID) {
		BillFilt flt;
		//flt.LocID = LConfig.Location;
		// @v9.7.10 flt.OpID = opID;
		// @v9.7.10 {
		const PPID op_type_id = GetOpType(opID);
		if(oneof3(op_type_id, PPOPT_DRAFTEXPEND, PPOPT_DRAFTRECEIPT, PPOPT_DRAFTTRANSIT))
			flt.Bbt = bbtDraftBills;
		else if(op_type_id == PPOPT_ACCTURN)
			flt.Bbt = bbtAccturnBills;
		else if(op_type_id == PPOPT_INVENTORY)
			flt.Bbt = bbtInventoryBills;
		else if(op_type_id == PPOPT_GOODSORDER)
			flt.Bbt = bbtOrderBills;
		else if(op_type_id == PPOPT_POOL)
			flt.Bbt = bbtPoolBills;
		flt.SetupBrowseBillsType(flt.Bbt);
		flt.OpID = opID;
		{
			BillFilt::FiltExtraParam p(0, flt.Bbt);
			PPView::Execute(PPVIEW_BILL, &flt, GetModelessStatus(), &p);
		}
		// } @v9.7.10
		// @v9.7.10 ::ViewGoodsBills(&flt, 1);
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
	H.FltReckon      = BIN(p_filt->Flags & OPKF_RECKON);
	H.FltProfitable  = BIN(p_filt->Flags & OPKF_PROFITABLE);
	H.FltNeedPayment = BIN(p_filt->Flags & OPKF_NEEDPAYMENT);
	H.FltPassive     = BIN(p_filt->Flags & OPKF_PASSIVE);
	H.FltAccSheet    = p_filt->AccSheetID;
	H.FltOpType      = p_filt->OpTypeID;
	GetObjectName(PPOBJ_OPRTYPE, p_filt->OpTypeID, H.FltOpTypeName, sizeof(H.FltOpTypeName));
	return DlRtm::InitData(rFilt, rsrv);
}

int PPALDD_OprKindList::InitIteration(PPIterID iterId, int sortId, long /*rsrv*/)
{
	INIT_PPVIEW_ALDD_ITER(OprKind);
}

int PPALDD_OprKindList::NextIteration(PPIterID iterId)
{
	START_PPVIEW_ALDD_ITER(OprKind);
	I.OpID = item.ID;
	STRNSCPY(I.FlagsMnemonics, item.FlagsMnems);
	STRNSCPY(I.OpType, item.OpType);
	STRNSCPY(I.Template, item.Template);
	FINISH_PPVIEW_ALDD_ITER();
}

void PPALDD_OprKindList::Destroy()
{
	DESTROY_PPVIEW_ALDD(OprKind);
}

