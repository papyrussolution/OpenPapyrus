// V_SCALE.CPP
// Copyright (c) A.Starodub 2008, 2009, 2010, 2015, 2016, 2017, 2018, 2019, 2022
// @codepage windows-1251
//
#include <pp.h>
#pragma hdrstop
#include <charry.h>
//
// @ModuleDef(PPViewScale)
//
IMPLEMENT_PPFILT_FACTORY(Scale); ScaleFilt::ScaleFilt() : PPBaseFilt(PPFILT_SCALE, 0, 1)
{
	SetFlatChunk(offsetof(ScaleFilt, ReserveStart), offsetof(ScaleFilt, ReserveEnd) - offsetof(ScaleFilt, ReserveStart));
	Init(1, 0);
}

ScaleFilt & FASTCALL ScaleFilt::operator = (const ScaleFilt & s)
{
	Copy(&s, 1);
	return *this;
}

PPViewScale::PPViewScale() : PPView(&ObjScale, &Filt, PPVIEW_SCALE, implDontEditNullFilter, 0), P_TempTbl(0)
{
	PPLoadText(PPTXT_SCLT, ScaleTypeNames);
}

PPViewScale::~PPViewScale()
{
	ZDELETE(P_TempTbl);
}

int PPViewScale::CheckForFilt(const PPScalePacket * pPack) const
{
	if(pPack) {
		const int is_tcp = BIN(pPack->Rec.Flags & SCALF_TCPIP);
		if(Filt.LocID && Filt.LocID != pPack->Rec.Location)
			return 0;
		if(Filt.ScaleTypeID && Filt.ScaleTypeID != pPack->Rec.ScaleTypeID)
			return 0;
		if(Filt.AltGoodsGrpID && Filt.AltGoodsGrpID != pPack->Rec.AltGoodsGrp)
			return 0;
		if((Filt.Protocol == ScaleFilt::protTcpIp && !is_tcp) || (Filt.Protocol == ScaleFilt::protCom && is_tcp))
			return 0;
		if(Filt.GroupID && pPack->Rec.ParentID != Filt.GroupID)
			return 0;
		if(Filt.Flags && (pPack->Rec.Flags & Filt.Flags) != Filt.Flags)
			return 0;
	}
	return 1;
}

TempScaleTbl::Rec & PPViewScale::MakeTempEntry(const PPScalePacket & rPack, TempScaleTbl::Rec & rTempRec)
{
	SString temp_buf;
	rTempRec.ID = rPack.Rec.ID;
	STRNSCPY(rTempRec.Name, rPack.Rec.Name);
	rTempRec.QuotKindID = rPack.Rec.QuotKindID;
	rTempRec.ScaleTypeID = rPack.Rec.ScaleTypeID;
	rTempRec.ProtocolVer = rPack.Rec.ProtocolVer;
	rTempRec.LogNum = rPack.Rec.LogNum;
	rTempRec.LocID = rPack.Rec.Location;
	rTempRec.AltGoodsGrp = rPack.Rec.AltGoodsGrp;
	rPack.GetExtStrData(rPack.extssPort, temp_buf);
	STRNSCPY(rTempRec.Port, temp_buf);
	{
		SString buf1, buf2;
		PPGetSubStr(ScaleTypeNames, rTempRec.ScaleTypeID - 1, temp_buf);
		temp_buf.Divide(',', buf1, buf2);
		buf2.CopyTo(rTempRec.ScaleTypeName, sizeof(rTempRec.ScaleTypeName));
	}
	return rTempRec;
}

int PPViewScale::EditBaseFilt(PPBaseFilt * pFilt)
{
	int    ok = -1;
	ScaleFilt filt;
	TDialog * p_dlg = new TDialog(DLG_SCALEFLT);
	filt.Copy(pFilt, 0);
	THROW(CheckDialogPtr(&p_dlg));
	SetupStringCombo(p_dlg, CTLSEL_SCALEFLT_TYPE, PPTXT_SCLT, filt.ScaleTypeID);
	SetupPPObjCombo(p_dlg,  CTLSEL_SCALEFLT_LOC, PPOBJ_LOCATION, filt.LocID, 0);
	SetupPPObjCombo(p_dlg,  CTLSEL_SCALEFLT_ALTGRP, PPOBJ_GOODSGROUP, filt.AltGoodsGrpID, 0, reinterpret_cast<void *>(GGRTYP_SEL_ALT));
	SetupPPObjCombo(p_dlg,  CTLSEL_SCALEFLT_GROUP,  PPOBJ_SCALE, filt.GroupID, 0, PPObjScale::MakeExtraParam(PPSCLT_SCALEGROUP, 0));
	p_dlg->AddClusterAssocDef(CTL_SCALEFLT_PROT,  0, ScaleFilt::protAll);
	p_dlg->AddClusterAssoc(CTL_SCALEFLT_PROT,  1, ScaleFilt::protTcpIp);
	p_dlg->AddClusterAssoc(CTL_SCALEFLT_PROT,  2, ScaleFilt::protCom);
	p_dlg->SetClusterData(CTL_SCALEFLT_PROT, filt.Protocol);
	p_dlg->AddClusterAssoc(CTL_SCALEFLT_FLAGS, 0, SCALF_PASSIVE);
	p_dlg->SetClusterData(CTL_SCALEFLT_FLAGS, filt.Flags);
	if(ExecView(p_dlg) == cmOK) {
		filt.LocID = p_dlg->getCtrlLong(CTLSEL_SCALEFLT_LOC);
		filt.ScaleTypeID   = p_dlg->getCtrlLong(CTLSEL_SCALEFLT_TYPE);
		filt.AltGoodsGrpID = p_dlg->getCtrlLong(CTLSEL_SCALEFLT_ALTGRP);
		filt.GroupID       = p_dlg->getCtrlLong(CTLSEL_SCALEFLT_GROUP);
		p_dlg->GetClusterData(CTL_SCALEFLT_PROT, &filt.Protocol);
		p_dlg->GetClusterData(CTL_SCALEFLT_FLAGS, &filt.Flags);
		if(pFilt)
			pFilt->Copy(&filt, 0);
		ok = 1;
	}
	CATCHZOKPPERR
	delete p_dlg;
	return ok;
}

PP_CREATE_TEMP_FILE_PROC(CreateTempFile, TempScale);

int PPViewScale::Init_(const PPBaseFilt * pFilt)
{
	int    ok = 1;
	THROW(Helper_InitBaseFilt(pFilt));
	BExtQuery::ZDelete(&P_IterQuery);
	ZDELETE(P_TempTbl);
	THROW(P_TempTbl = CreateTempFile());
	{
		PPScale rec;
		BExtInsert bei(P_TempTbl);
		PPTransaction tra(ppDbDependTransaction, 1);
		THROW(tra);
		for(PPID id = 0; ObjScale.EnumItems(&id, &rec) > 0;) {
			PPScalePacket pack;
			if(ObjScale.GetPacket(rec.ID, &pack) > 0) {
				if(CheckForFilt(&pack) > 0) {
					TempScaleTbl::Rec temp_rec;
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

int PPViewScale::UpdateTempTable(const PPIDArray * pIdList)
{
	int    ok = -1;
	if(pIdList && P_TempTbl) {
		PPTransaction tra(ppDbDependTransaction, 1);
		THROW(tra);
		for(uint i = 0; i < pIdList->getCount(); i++) {
			const PPID id = pIdList->at(i);
			PPScalePacket pack;
			TempScaleTbl::Rec temp_rec;
			if(ObjScale.GetPacket(id, &pack) > 0 && CheckForFilt(&pack)) {
				MakeTempEntry(pack, temp_rec);
				if(SearchByID_ForUpdate(P_TempTbl, 0,  id, 0) > 0) {
					THROW_DB(P_TempTbl->updateRecBuf(&temp_rec));
				}
				else {
					THROW_DB(P_TempTbl->insertRecBuf(&temp_rec));
				}
			}
			else {
				THROW_DB(deleteFrom(P_TempTbl, 0, P_TempTbl->ID == id));
			}
			ok = 1;
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int PPViewScale::InitIteration()
{
	int    ok = 1;
	TempScaleTbl::Key0 k, k_;
	BExtQuery::ZDelete(&P_IterQuery);
	P_IterQuery = new BExtQuery(P_TempTbl, 0, 128);
	P_IterQuery->selectAll();
	MEMSZERO(k);
	k_ = k;
	Counter.Init(P_IterQuery->countIterations(0, &k_, spGe));
	P_IterQuery->initIteration(0, &k, spGe);
	return ok;
}

int FASTCALL PPViewScale::NextIteration(ScaleViewItem * pItem)
{
	while(pItem && P_IterQuery && P_IterQuery->nextIteration() > 0) {
		PPScalePacket pack;
		TempScaleTbl * p_t = P_TempTbl;
		Counter.Increment();
		if(ObjScale.GetPacket(p_t->data.ID, &pack) > 0) {
			*static_cast<PPScale *>(pItem) = pack.Rec;
			pack.GetExtStrData(pack.extssPort, pItem->Port);
			return 1;
		}
	}
	return -1;
}

int PPViewScale::CheckScaleStatus(PPID scaleID, int statusFromList)
{
	long   ok = 0;
	if(statusFromList) {
		if(!ScaleStatusList.Search(scaleID, &ok, 0))
			ok = -2;
	}
	else  {
		PPScalePacket pack;
		if(ObjScale.GetPacket(scaleID, &pack) > 0) {
			if(pack.Rec.Flags & SCALF_TCPIP) {
				SString port_buf;
				pack.GetExtStrData(pack.extssPort, port_buf);
				//char ip[16];
				//THROW(PPObjScale::DecodeIP(scale.Port, ip));
				ok = static_cast<long>(PPObjScale::CheckForConnection(port_buf, 1000, 5));
			}
			else
				ok = -1;
		}
		ScaleStatusList.AddUnique(scaleID, ok, 0);
	}
	//CATCHZOK
	return (int)ok;
}

static int ScaleReadyColorFunc(const void * pData, long col, int paintAction, BrowserWindow::CellStyle * pStyle, void * extraPtr)
{
	int    ok = -1;
	PPViewScale * p_brw = static_cast<PPViewScale *>(extraPtr);
	if(p_brw && pData && pStyle && col == 4) {
		const long scale_id = *static_cast<const long *>(pData);
		const int  is_ready = p_brw->CheckScaleStatus(scale_id, 1);
		if(is_ready == 0) {
			pStyle->Color = GetColorRef(SClrRed);
			pStyle->Flags = BrowserWindow::CellStyle::fCorner;
			ok = 1;
		}
		else if(is_ready == 1) {
			pStyle->Color = GetColorRef(SClrGreen);
			pStyle->Flags = BrowserWindow::CellStyle::fCorner;
			ok = 1;
		}
		else if(is_ready == -1) {
			pStyle->Color = GetColorRef(SClrGrey);
			pStyle->Flags = BrowserWindow::CellStyle::fCorner;
			ok = 1;
		}
	}
	return ok;
}

void PPViewScale::PreprocessBrowser(PPViewBrowser * pBrw)
{
	CALLPTRMEMB(pBrw, SetCellStyleFunc(ScaleReadyColorFunc, this));
}

DBQuery * PPViewScale::CreateBrowserQuery(uint * pBrwId, SString * pSubTitle)
{
	DBQuery * q  = 0;
	TempScaleTbl * t = 0;
	uint   brw_id = BROWSER_SCALE;
	DBQ  * dbq = 0;
	DBE    dbe_loc, dbe_ggrp;
	THROW(CheckTblPtr(t = new TempScaleTbl(P_TempTbl->GetName())));
	PPDbqFuncPool::InitObjNameFunc(dbe_loc,  PPDbqFuncPool::IdObjNameLoc,   t->LocID);
	PPDbqFuncPool::InitObjNameFunc(dbe_ggrp, PPDbqFuncPool::IdObjNameGoods, t->AltGoodsGrp);
	q = & select(
		t->ID,            // #0
		t->Name,          // #1
		t->ScaleTypeName, // #2
		dbe_loc,          // #3
		dbe_ggrp,         // #4
		t->Port,          // #5
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

int PPViewScale::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int        ok = (ppvCmd != PPVCMD_ADDITEM) ? PPView::ProcessCommand(ppvCmd, pHdr, pBrw) : -2;
	PPIDArray  id_list;
	PPID       id = (pHdr) ? *static_cast<const PPID *>(pHdr) : 0;
	if(ok == -2) {
		switch(ppvCmd) {
			case PPVCMD_ADDITEM:
				ok = (ObjScale.Edit(&(id = 0), 0) == cmOK) ? 1 : -1;
				break;
			case PPVCMD_VIEWGOODS:
				ok = -1;
				{
					PPScale sc_rec;
					if(ObjScale.Search(id, &sc_rec) > 0 && sc_rec.AltGoodsGrp) {
						GoodsFilt filt;
						filt.GrpIDList.Add(sc_rec.AltGoodsGrp);
						PPView::Execute(PPVIEW_GOODS, &filt, 1, 0);
					}
				}
				break;
			case PPVCMD_PREPAREDATA:
				ok = PPObjScale::PrepareData(id);
				break;
			case PPVCMD_LOADDATA:
				ok = PPObjScale::TransmitData(id);
				break;
			case PPVCMD_LOADSTAT:
				ok = -1;
				{
					DvcLoadingStatFilt filt;
					filt.DvcID   = id;
					filt.DvcType = 2;
					PPView::Execute(PPVIEW_DVCLOADINGSTAT, &filt, 1, 0);
				}
				break;
			case PPVCMD_CHECKSTATUS:
				{
					SString msg;
					ScaleViewItem item;

					PPGetSubStr(PPTXT_SCALEREADY, 2, msg);
					ScaleStatusList.freeAll();
					PPWaitStart();
					for(InitIteration(); NextIteration(&item) > 0;) {
						PPWaitPercent(GetCounter(), msg);
						CheckScaleStatus(item.ID, 0);
					}
					PPWaitStop();
					ok = 1;
				}
				break;
			case PPVCMD_TRANSMIT:
				{
					ScaleViewItem item;
					ObjTransmitParam param;
					PPIDArray id_list;
					for(InitIteration(); NextIteration(&item) > 0;)
						id_list.add(item.ID);
					if(ObjTransmDialog(DLG_OBJTRANSM, &param) > 0) {
						const PPIDArray & rary = param.DestDBDivList.Get();
						PPObjIDArray objid_ary;
						PPWaitStart();
						objid_ary.Add(PPOBJ_SCALE, id_list);
						ok = PPObjectTransmit::Transmit(&rary, &objid_ary, &param);
						PPWaitStop();
					}
					if(!ok)
						PPError();
				}
				break;
		}
	}
	if(ok > 0 && oneof3(ppvCmd, PPVCMD_ADDITEM, PPVCMD_EDITITEM, PPVCMD_DELETEITEM)) {
		id_list.add(id);
		ok = UpdateTempTable(&id_list);
	}
	return ok;
}
