// V_PALM.CPP
// Copyright (c) A.Starodub 2009, 2010, 2015, 2016, 2018, 2022
// @codepage windows-1251
//
#include <pp.h>
#pragma hdrstop
#include <charry.h>
#include <ppsoapclient.h>
//
// @ModuleDef(PPViewPalm)
//
IMPLEMENT_PPFILT_FACTORY(Palm); PalmFilt::PalmFilt() : PPBaseFilt(PPFILT_PALM, 0, 0)
{
	SetFlatChunk(offsetof(PalmFilt, ReserveStart), offsetof(PalmFilt, ReserveEnd) - offsetof(PalmFilt, ReserveStart) + sizeof(ReserveEnd));
	SetBranchObjIdListFilt(offsetof(PalmFilt, LocList));
	Init(1, 0);
}

PalmFilt & FASTCALL PalmFilt::operator = (const PalmFilt & s)
{
	Copy(&s, 1);
	return *this;
}

PPViewPalm::PPViewPalm() : PPView(&ObjPalm, &Filt, PPVIEW_PALM, 0, 0), P_TempTbl(0)
{
}

PPViewPalm::~PPViewPalm()
{
	ZDELETE(P_TempTbl);
}

int PPViewPalm::CheckForFilt(const PPStyloPalm * pRec)
{
	if(pRec) {
		PPID   ord_op_id = pRec->OrderOpID;
		if(Filt.Type != PalmFilt::devtAll && (Filt.Type == PalmFilt::devtOnlyGroups && !(pRec->Flags & PLMF_GENERIC) ||
			Filt.Type == PalmFilt::devtOnlyDevs && (pRec->Flags & PLMF_GENERIC)))
			return 0;
		if(!CheckFiltID(Filt.GroupID, pRec->GroupID))
			return 0;
		if(pRec->GroupID) {
			PPStyloPalm rec;
			MEMSZERO(rec);
			if(ObjPalm.Search(pRec->GroupID, &rec) > 0) {
				ord_op_id = rec.OrderOpID;
			}
		}
		if(!CheckFiltID(Filt.GoodsGrpID, pRec->GoodsGrpID))
			return 0;
		if(!CheckFiltID(Filt.OrderOpID, ord_op_id))
			return 0;
		if(!CheckFiltID(Filt.AgentID, pRec->AgentID))
			return 0;
		if(!CheckFiltID(Filt.FTPAcctID, pRec->FTPAcctID))
			return 0;
		if(Filt.LocList.GetCount()) {
			int    result = 0;
			ObjIdListFilt loc_list;
			int    r = ObjPalm.GetLocList(pRec->ID, loc_list);
			if(r < 0)
				r = ObjPalm.GetLocList(pRec->GroupID, loc_list);
			if(r > 0) {
				for(uint i = 0; !result && i < loc_list.GetCount(); i++) {
					if(Filt.LocList.Search(loc_list.Get(i), 0))
						result = 1;
				}
			}
			if(!result)
				return 0;
		}
	}
	return 1;
}

TempPalmTbl::Rec & PPViewPalm::MakeTempEntry(const PPStyloPalmPacket & rPack, TempPalmTbl::Rec & rTempRec)
{
	int    ok = -1;
	memzero(&rTempRec, sizeof(TempPalmTbl::Rec));
	rTempRec.ID = rPack.Rec.ID;
	STRNSCPY(rTempRec.Name, rPack.Rec.Name);
	STRNSCPY(rTempRec.Symb, rPack.Rec.Symb);
	rTempRec.GoodsGrpID = rPack.Rec.GoodsGrpID;
	rTempRec.OrderOpID  = rPack.Rec.OrderOpID;
	rTempRec.FTPAcctID  = rPack.Rec.FTPAcctID;
	rTempRec.Flags      = rPack.Rec.Flags;
	rTempRec.GroupID    = rPack.Rec.GroupID;
	rTempRec.AgentID    = rPack.Rec.AgentID;
	if(rPack.P_Path)
		STRNSCPY(rTempRec.Path, rPack.P_Path);
	if(rPack.P_FTPPath)
		STRNSCPY(rTempRec.FtpPath, rPack.P_FTPPath);
	if(rPack.LocList.GetCount() == 1) {
		rTempRec.LocID = rPack.LocList.Get(0);
	}
	else if(rPack.LocList.GetCount() > 1)
		rTempRec.LocID = -1;
	{
		PPStyloPalm parent_rec;
		if(rTempRec.GroupID && ObjPalm.Search(rTempRec.GroupID, &parent_rec) > 0)
			STRNSCPY(rTempRec.GroupName, parent_rec.Name);
	}
	return rTempRec;
}

// @v11.4.4 #define GRP_LOC 1

int PPViewPalm::EditBaseFilt(PPBaseFilt * pFilt)
{
	enum {
		ctlgroupLoc = 1,
	};
	int    ok = -1;
	PalmFilt filt;
	TDialog * p_dlg = new TDialog(DLG_PALMFLT);
	filt.Copy(pFilt, 0);
	THROW(CheckDialogPtr(&p_dlg));
	SetupPPObjCombo(p_dlg, CTLSEL_PALMFLT_GROUP, PPOBJ_STYLOPALM, filt.GroupID, OLW_CANSELUPLEVEL, reinterpret_cast<void *>(PLMF_GENERIC));
	SetupArCombo(p_dlg, CTLSEL_PALMFLT_AGENT, filt.AgentID, OLW_CANINSERT, GetAgentAccSheet(), sacfDisableIfZeroSheet);
	SetupPPObjCombo(p_dlg, CTLSEL_PALMFLT_GOODSGRP, PPOBJ_GOODSGROUP, filt.GoodsGrpID, OLW_CANSELUPLEVEL);
	SetupPPObjCombo(p_dlg, CTLSEL_PALMFLT_FTPACC, PPOBJ_INTERNETACCOUNT, filt.FTPAcctID, 0, 
		reinterpret_cast<void *>(PPObjInternetAccount::filtfFtp)/*INETACCT_ONLYFTP*/);
	{
		PPIDArray op_type_list;
		LocationCtrlGroup::Rec loc_rec(&filt.LocList);
		p_dlg->addGroup(ctlgroupLoc, new LocationCtrlGroup(CTLSEL_PALMFLT_LOC, 0, 0, cmLocList, 0, LocationCtrlGroup::fEnableSelUpLevel, 0));
		p_dlg->setGroupData(ctlgroupLoc, &loc_rec);
		op_type_list.addzlist(PPOPT_GOODSORDER, PPOPT_GOODSEXPEND, 0L);
		SetupOprKindCombo(p_dlg, CTLSEL_PALMFLT_ORDOP, filt.OrderOpID, 0, &op_type_list, 0);
	}
	p_dlg->AddClusterAssocDef(CTL_PALMFLT_TYPE,  0, PalmFilt::devtAll);
	p_dlg->AddClusterAssoc(CTL_PALMFLT_TYPE,  1, PalmFilt::devtOnlyGroups);
	p_dlg->AddClusterAssoc(CTL_PALMFLT_TYPE,  2, PalmFilt::devtOnlyDevs);
	p_dlg->SetClusterData(CTL_PALMFLT_TYPE, (long)filt.Type);
	if(ExecView(p_dlg) == cmOK) {
		long type = 0;
		LocationCtrlGroup::Rec loc_rec;
		p_dlg->getGroupData(ctlgroupLoc, &loc_rec);
		filt.LocList = loc_rec.LocList;
		p_dlg->getCtrlData(CTLSEL_PALMFLT_GROUP,    &filt.GroupID);
		p_dlg->getCtrlData(CTLSEL_PALMFLT_AGENT,    &filt.AgentID);
		p_dlg->getCtrlData(CTLSEL_PALMFLT_GOODSGRP, &filt.GoodsGrpID);
		p_dlg->getCtrlData(CTLSEL_PALMFLT_FTPACC,   &filt.FTPAcctID);
		p_dlg->getCtrlData(CTLSEL_PALMFLT_ORDOP,    &filt.OrderOpID);
		p_dlg->GetClusterData(CTL_PALMFLT_TYPE,  &type);
		filt.Type = (int16)type;
		if(pFilt)
			pFilt->Copy(&filt, 0);
		ok = 1;
	}
	CATCHZOKPPERR
	delete p_dlg;
	return ok;
}

int PPViewPalm::Init_(const PPBaseFilt * pFilt)
{
	int    ok = 1;
	PPStyloPalm rec;
	PPStyloPalmPacket palm_pack;
	THROW(Helper_InitBaseFilt(pFilt));
	BExtQuery::ZDelete(&P_IterQuery);
	ZDELETE(P_TempTbl);
	THROW(P_TempTbl = CreateTempFile <TempPalmTbl> ());
	{
		BExtInsert bei(P_TempTbl);
		PPTransaction tra(ppDbDependTransaction, 1);
		THROW(tra);
		for(PPID id = 0; ObjPalm.EnumItems(&id, &rec) > 0;) {
			if(CheckForFilt(&rec) > 0 && ObjPalm.GetPacket(rec.ID, &palm_pack) > 0) {
				TempPalmTbl::Rec temp_rec;
				THROW_DB(bei.insert(&MakeTempEntry(palm_pack, temp_rec)));
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

int PPViewPalm::UpdateTempTable(const PPIDArray * pIdList)
{
	int    ok = -1;
	if(pIdList && P_TempTbl) {
		PPStyloPalmPacket palm_pack;
		PPTransaction tra(ppDbDependTransaction, 1);
		THROW(tra);
		for(uint i = 0; i < pIdList->getCount(); i++) {
			PPID   id = pIdList->at(i);
			PPStyloPalm rec;
			TempPalmTbl::Rec temp_rec;
			if(ObjPalm.Search(id, &rec) > 0 && CheckForFilt(&rec) > 0 && ObjPalm.GetPacket(rec.ID, &palm_pack) > 0) {
				MakeTempEntry(palm_pack, temp_rec);
				if(SearchByID_ForUpdate(P_TempTbl, 0, id, 0) > 0) {
					THROW(P_TempTbl->updateRecBuf(&temp_rec));
				}
				else {
					THROW(AddByID(P_TempTbl, &id, &temp_rec, 0));
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

int PPViewPalm::InitIteration()
{
	int    ok = 1;
	TempPalmTbl::Key0 k, k_;
	BExtQuery::ZDelete(&P_IterQuery);
	P_IterQuery = new BExtQuery(P_TempTbl, 0, 128);
	P_IterQuery->select(P_TempTbl->ID, 0);
	MEMSZERO(k);
	k_ = k;
	Counter.Init(P_IterQuery->countIterations(0, &k_, spGe));
	P_IterQuery->initIteration(false, &k, spGe);
	return ok;
}

int FASTCALL PPViewPalm::NextIteration(PalmViewItem * pItem)
{
	int    ok = -1;
	if(P_IterQuery) {
		while(ok < 0 && P_IterQuery->nextIteration() > 0) {
			if(pItem) {
				PPStyloPalmPacket pack;
				if(ObjPalm.GetPacket(P_TempTbl->data.ID, &pack) > 0) {
					*(PPStyloPalm *)pItem = pack.Rec;
					STRNSCPY(pItem->Path, pack.P_Path);
					STRNSCPY(pItem->FtpPath, pack.P_FTPPath);
					ok = 1;
				}
			}
			else
				ok = 1;
		}
	}
	return ok;
}

DBQuery * PPViewPalm::CreateBrowserQuery(uint * pBrwId, SString * pSubTitle)
{
	DBQuery * q  = 0;
	TempPalmTbl * t = 0;
	uint   brw_id = BROWSER_PALM;
	DBQ  * dbq = 0;
	DBE    dbe_loc, dbe_ggrp, dbe_ar;
	THROW(CheckTblPtr(t = new TempPalmTbl(P_TempTbl->GetName())));
	PPDbqFuncPool::InitObjNameFunc(dbe_loc,   PPDbqFuncPool::IdObjNameLoc,    t->LocID);
	PPDbqFuncPool::InitObjNameFunc(dbe_ggrp,  PPDbqFuncPool::IdObjNameGoods,  t->GoodsGrpID);
	PPDbqFuncPool::InitObjNameFunc(dbe_ar,    PPDbqFuncPool::IdObjNameAr,     t->AgentID);
	q = & select(
		t->ID,            // #0
		t->Name,          // #1
		dbe_loc,          // #2
		dbe_ggrp,         // #3
		dbe_ar,           // #4
		t->GroupName,     // #5
		t->Path,          // #6
		t->FtpPath,       // #7
		t->Symb,          // #8 @v7.4.7
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

int PPViewPalm::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = (ppvCmd != PPVCMD_ADDITEM) ? PPView::ProcessCommand(ppvCmd, pHdr, pBrw) : -2;
	PPID   id = 0;
	if(ok == -2) {
		switch(ppvCmd) {
			case PPVCMD_ADDITEM:
				ok = (ObjPalm.Edit(&(id = 0), 0) == cmOK) ? 1 : -1;
				break;
			case PPVCMD_OPPANEL:
				ok = -1;
				{
					id = pHdr ? *static_cast<const PPID *>(pHdr) : 0;
					if(id) {
						PalmPaneData param;
						param.PalmID = id;
						param.LocID = LConfig.Location;
						param.Flags |= PalmPaneData::fForceEdit;
						if(PPObjStyloPalm::ImpExp(&param) > 0)
							ok = 1;
					}
				}
				break;
			case PPVCMD_EXPORTUHTT:
				ok = -1;
				ExportUhtt();
				break;
			case PPVCMD_TEST: // @v11.2.6
				ok = -1;
				break;
		}
	}
	if(ok > 0 && oneof3(ppvCmd, PPVCMD_ADDITEM, PPVCMD_EDITITEM, PPVCMD_DELETEITEM)) {
		PPIDArray id_list;
		PPStyloPalm rec;
		MEMSZERO(rec);
		if(ppvCmd != PPVCMD_ADDITEM)
			id = pHdr ? *static_cast<const PPID *>(pHdr) : 0;
		if(ObjPalm.Search(id, &rec) > 0 && (rec.Flags & PLMF_GENERIC))
			for(PPID child_id = 0; ObjPalm.EnumItems(&child_id, &rec) > 0;)
				if(rec.GroupID == id)
					id_list.add(child_id);
		id_list.add(id);
		ok = UpdateTempTable(&id_list);
	}
	return ok;
}

int PPViewPalm::ExportUhtt()
{
	int    ok = -1;
	SString msg_buf, fmt_buf, temp_buf, loc_text_buf;
	SString phone_buf, contact_buf, addr_buf;
	PPLogger logger;
	PPUhttClient uhtt_cli;
	PPWaitStart();
	THROW(uhtt_cli.Auth());
	{
		PalmViewItem item;
		for(InitIteration(); NextIteration(&item) > 0; PPWaitPercent(GetCounter())) {
			const PPID _id = item.ID;
			PPStyloPalm rec, parent_rec;
			if(ObjPalm.Search(_id, &rec) > 0) {
				long    uhtt_stylo_id = 0;
				UhttStyloDevicePacket uhtt_pack, ret_pack;

				//PPTXT_UHTTEXPLOC_FOUND      "На сервере Universe-HTT адрес '@zstr' найден по коду"
				//PPTXT_UHTTEXPLOC_EXPORTED   "Адрес '@zstr' экспортирован на сервер Universe-HTT"
				//PPTXT_UHTTEXPLOC_CODEASSGN  "Адресу '@zstr' присвоен код с сервера Universe-HTT"
				//PPTXT_UHTTEXPLOC_EGETBYCOD  "Ошибка обратного получения адреса '@zstr' по идентификатору Universe-HTT: @zstr"
				//PPTXT_UHTTEXPLOC_EEXPORT    "Ошибка экспорта адреса '@zstr' на сервер Universe-HTT: @zstr"
				int   dont_update = 0;
				if(rec.Symb[0] && uhtt_cli.GetStyloDeviceByCode(rec.Symb, ret_pack) > 0) {
					uhtt_stylo_id = ret_pack.ID;
					dont_update = 1;
				}
				if(dont_update) {
					uhtt_stylo_id = ret_pack.ID;
					//
					PPLoadText(PPTXT_UHTTEXPSTYLO_FOUND, fmt_buf);
					PPFormat(fmt_buf, &msg_buf, (const char *)rec.Name);
					logger.Log(msg_buf);
				}
				else {
					long   uhtt_parent_id = 0;
					if(rec.GroupID && ObjPalm.Search(rec.GroupID, &parent_rec) > 0) {
						if(parent_rec.Symb[0]) {
							UhttStyloDevicePacket temp_uhtt_pack;
							if(uhtt_cli.GetStyloDeviceByCode(parent_rec.Symb, temp_uhtt_pack) > 0)
								uhtt_parent_id = temp_uhtt_pack.ID;
						}
					}
					uhtt_pack.ID = uhtt_stylo_id;
					uhtt_pack.Flags = rec.Flags;
					uhtt_pack.ParentID = uhtt_parent_id;
					uhtt_pack.DeviceVer = rec.DeviceVer;
					uhtt_pack.RegisterTime = rec.RegisterTime;
					uhtt_pack.Name = rec.Name;
					uhtt_pack.Symb = rec.Symb;
					int    cr = uhtt_cli.CreateStyloDevice(&uhtt_stylo_id, uhtt_pack);
					if(cr) {
						PPLoadText(PPTXT_UHTTEXPSTYLO_EXPORTED, fmt_buf);
						PPFormat(fmt_buf, &msg_buf, (const char *)rec.Name);
						logger.Log(msg_buf);
					}
					else {
						// Ошибка экспорта адреса на сервер Universe-HTT
						(temp_buf = uhtt_cli.GetLastMessage()).ToOem();
						PPLoadText(PPTXT_UHTTEXPSTYLO_EEXPORT, fmt_buf);
						PPFormat(fmt_buf, &msg_buf, (const char *)rec.Name, temp_buf.cptr());
						logger.Log(msg_buf);
					}
				}
			}
		}
	}
	PPWaitStop();
	CATCH
		logger.LogLastError();
		ok = PPErrorZ();
	ENDCATCH
	return ok;
}
