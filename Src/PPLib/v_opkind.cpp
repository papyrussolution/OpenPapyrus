// V_OPKIND.CPP
// Copyright (c) A.Starodub 2004, 2006, 2007, 2008, 2009, 2013, 2015, 2016
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

SLAPI PPViewOprKind::PPViewOprKind() : PPView(&OpkObj, &Filt, PPVIEW_OPRKIND)
{
	DefReportId = REPORT_OPRKINDLIST;
	ImplementFlags |= implBrowseArray;
	OpListIdx = TmplsIdx = 0;
	//Counter.Init();
	//Filt.Init();
	P_OpList  = 0;
}

SLAPI PPViewOprKind::~PPViewOprKind()
{
	ZDELETE(P_OpList);
}

int SLAPI PPViewOprKind::Init_(const PPBaseFilt * pBaseFilt)
{
	int    ok = 1;
	THROW(Helper_InitBaseFilt(pBaseFilt));
	ZDELETE(P_IterQuery);
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
			temp_buf.Cat(dbt_buf).Space().Cat(crd_buf).Space().Cat(r_templ.Expr).
				CopyTo(pItem->Template, sizeof(pItem->Template));
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

int SLAPI PPViewOprKind::NextIteration(OprKindViewItem * pItem)
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
		OprKindFilt * p_filt = (OprKindFilt *)pBaseFilt;
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

#if 0 // {

class OprKindBrowser : public BrowserWindow {
public:
	OprKindBrowser(uint resID, SArray * pAry, PPViewOprKind * pV, int dataOwner) :
		BrowserWindow(resID, pAry)
	{
		P_View = pV;
		IsDataOwner = dataOwner;
	}
	~OprKindBrowser()
	{
		if(IsDataOwner)
			delete P_View;
	}
	PPViewOprKind *P_View;
private:
	DECL_HANDLE_EVENT;
	PPID    currID()
	{
		void * p_row;
		if(view && (p_row = view->getCurItem()) != 0) {
			return *(PPID *)p_row;
		}
		else
			return 0;
	}
	void    updateView();
	int    IsDataOwner;
};

IMPL_HANDLE_EVENT(OprKindBrowser)
{
	BrowserWindow::handleEvent(event);
	if(!P_View)
		return;
	PPID   op_id = currID();
	if(TVCOMMAND) {
		switch(TVCMD) {
			case cmaEdit:
				if(P_View->EditItem(op_id) > 0)
					updateView();
				break;
			case cmaInsert:
				if(P_View->AddItem() > 0)
					updateView();
				break;
			case cmaDelete:
				if(P_View->DeleteItem(op_id) > 0)
					updateView();
				break;
			default:
				return;
		}
	}
	else if(TVKEYDOWN) {
		switch(TVKEY) {
			case kbAltF2:
				if(op_id && P_View->AddBySample(op_id) > 0)
					updateView();
				break;
			case kbF3:
				if(op_id)
					P_View->ViewBills(op_id);
				break;
			case kbAltF3:
				if(op_id && P_View->ViewLinkOps(op_id) > 0)
					updateView();
				break;
			case kbCtrlV:
				{
					OprKindFilt filt;
					filt = *P_View->GetFilt();
					if(P_View->EditFilt(&filt) > 0) {
						P_View->Init_(&filt);
						updateView();
					}
				}
				break;
			case kbCtrlJ: // @v9.2.1
				if(op_id)
					ViewSysJournal(PPOBJ_OPRKIND, op_id, 0);
				break;
			case kbCtrlB:
				P_View->Transmit(0);
				break;
			case kbF7:
				P_View->Print();
				break;
			default:
				return;
		}
	}
	else
		return;
	clearEvent(event);
}

void OprKindBrowser::updateView()
{
	if(view) {
		AryBrowserDef * p_def = (AryBrowserDef *)view->getDef();
		if(p_def) {
			long   c = p_def->_curItem();
			p_def->setArray(0, 0, 1);
			SArray * p_list = P_View->CreateBrowserArray();
			if(p_list) {
				lock();
				p_def->setArray(p_list, 0, 1);
				view->setRange(p_list->getCount());
				view->go(c);
				unlock();
			}
			else
				PPError();
		}
	}
}

#endif // } 0

IMPL_CMPFUNC(OprKindBrwItemName, _i1, _i2)
{
	int    r = 0;
	const OprKindBrwItem * p_item1 = (OprKindBrwItem*)_i1;
	const OprKindBrwItem * p_item2 = (OprKindBrwItem*)_i2;
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
	const OprKindBrwItem * p_item1 = (OprKindBrwItem*)_i1;
	const OprKindBrwItem * p_item2 = (OprKindBrwItem*)_i2;
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
	const OprKindBrwItem * p_item1 = (OprKindBrwItem*)_i1;
	const OprKindBrwItem * p_item2 = (OprKindBrwItem*)_i2;
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
	const  OprKindBrwItem * p_item1 = (OprKindBrwItem*)_i1;
	const  OprKindBrwItem * p_item2 = (OprKindBrwItem*)_i2;
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
		if(pOpData->Flags & OPKF_NEEDPAYMENT)
			buf.CatChar('P');
		if(pOpData->Flags & OPKF_NOUPDLOTREST)
			buf.CatChar('x');
		if(pOpData->Flags & OPKF_ADVACC)
			buf.CatChar('>');
		if(pOpData->Flags & OPKF_PROFITABLE)
			buf.CatChar('G');
		if(pOpData->Flags & OPKF_ONORDER)
			buf.CatChar('D');
		if(pOpData->Flags & OPKF_ORDRESERVE)
			buf.CatChar('R');
		if(pOpData->Flags & OPKF_CALCSTAXES)
			buf.CatChar('V');
		if(pOpData->Flags & OPKF_OUTBALACCTURN)
			buf.CatChar('O');
		if(pOpData->Flags & OPKF_EXTACCTURN)
			buf.CatChar('E');
		if(pOpData->Flags & OPKF_EXTAMTLIST)
			buf.CatChar('L');
		if(pOpData->Flags & OPKF_RENT)
			buf.CatChar('N');
		if(pOpData->Flags & OPKF_NEEDACK)
			buf.CatChar('A');
		if(pOpData->Flags & OPKF_RECKON)
			buf.CatChar('K');
		if(pOpData->Flags & OPKF_BANKING)
			buf.CatChar('B');
		if(pOpData->Flags & OPKF_PASSIVE)
			buf.CatChar('S');
		if(pOpData->Flags & OPKF_CURTRANSIT)
			buf.CatChar('T');
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
		PPID   id = pHdr ? *(PPID *)pHdr : 0;
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
	return ViewOprKind(&filt);
}

int SLAPI PPViewOprKind::ViewBills(PPID opID)
{
	if(opID) {
		BillFilt flt;
		//flt.LocID = LConfig.Location;
		flt.OpID = opID;
		::ViewGoodsBills(&flt, 1);
	}
	return -1;
}

/*int SLAPI PPViewOprKind::Browse(int modeless)
{
	int    ok = 1;
	SArray * p_ary = 0;
	OprKindBrowser * p_brw = 0;
	PPWait(1);
	THROW(p_ary = CreateBrowserArray());
	THROW_MEM(p_brw = new OprKindBrowser(BROWSER_OPRKIND, p_ary, this, modeless));
	PPWait(0);
	PPOpenBrowser(p_brw, modeless);
	CATCH
		ok = (PPWait(0), 0);
		if(p_brw == 0)
			delete p_ary;
	ENDCATCH
	if(!modeless || !ok)
		delete p_brw;
	return ok;
}*/

/*int SLAPI PPViewOprKind::Print()
{
	int    ok = 1;
	PView  pv(this);
	PPAlddPrint(REPORT_OPRKINDLIST, &pv);
	return ok;
}*/

int SLAPI ViewOprKind(OprKindFilt * pFilt)
{
#if 0 // {
	int    ok = 1, view_in_use = 0;
	int    modeless = GetModelessStatus();
	BrowserWindow * p_prev_win = 0;
	OprKindFilt flt;
	PPViewOprKind * p_v = new PPViewOprKind;
	if(modeless)
		p_prev_win = PPFindLastBrowser();
	if(pFilt)
		flt = *pFilt;
	else if(p_prev_win)
		flt = *(((OprKindBrowser *)p_prev_win)->P_View->GetFilt());
	else
		flt.Init();
	while(pFilt || p_v->EditFilt(&flt) > 0) {
		PPWait(1);
		THROW(p_v->Init_(&flt));
		PPCloseBrowser(p_prev_win);
		THROW(p_v->Browse(modeless));
		if(modeless || pFilt) {
			view_in_use = 1;
			break;
		}
	}
	CATCHZOKPPERR
	if(!modeless || !ok || !view_in_use)
		delete p_v;
	return ok;
#endif // } 0
	return PPView::Execute(PPVIEW_OPRKIND, pFilt, 1, 0);
}
//
// Implementation of PPALDD_OprKindList
//
PPALDD_CONSTRUCTOR(OprKindList)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		AssignIterData(1, &I, sizeof(I));
	}
}

PPALDD_DESTRUCTOR(OprKindList)
{
	Destroy();
}

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

int PPALDD_OprKindList::NextIteration(PPIterID iterId, long rsrv)
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

