// PROJECT.CPP
// Copyright (c) A.Sobolev 2003, 2004, 2005, 2006, 2007, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022, 2023, 2024, 2025
// @codepage UTF-8
//
#include <pp.h>
#pragma hdrstop

PPProjectConfig::PPProjectConfig()
{
	Z();
}

PPProjectConfig & PPProjectConfig::Z()
{
	THISZERO();
	return *this;
}

/*static*/int FASTCALL PPObjProject::ReadConfig(PPProjectConfig * pCfg)
{
	int    r = PPRef->GetPropMainConfig(PPPRP_PROJECTCFG, pCfg, sizeof(*pCfg));
	if(r <= 0)
		memzero(pCfg, sizeof(*pCfg));
	return r;
}

static bool PutCounter(PPID * pID, PPObjOpCounter * pOpcObj, PPOpCounterPacket * pCntr)
{
	pCntr->Head.ObjType = PPOBJ_PROJECT;
	pCntr->Head.OwnerObjID = -1;
	return LOGIC(pOpcObj->PutPacket(pID, pCntr, 0));
}

static int PPObjProject_WriteConfig(PPProjectConfig * pCfg, PPOpCounterPacket * pPrjCntr, PPOpCounterPacket * pPhsCntr,
	PPOpCounterPacket * pTodoCntr, PPOpCounterPacket * pTemplCntr)
{
	int    ok = 1;
	int    ta = 0;
	bool   is_new = true;
	Reference * p_ref = PPRef;
	PPObjOpCounter opc_obj;
	PPProjectConfig prev_cfg;
	{
		PPTransaction tra(1);
		THROW(tra);
		if(p_ref->GetPropMainConfig(PPPRP_PROJECTCFG, &prev_cfg, sizeof(prev_cfg)) > 0)
			is_new = false;
		THROW(PutCounter(&pCfg->PrjCntrID, &opc_obj, pPrjCntr));
		THROW(PutCounter(&pCfg->PhaseCntrID, &opc_obj, pPhsCntr));
		THROW(PutCounter(&pCfg->TaskCntrID, &opc_obj, pTodoCntr));
		THROW(PutCounter(&pCfg->TemplCntrID, &opc_obj, pTemplCntr));
		pCfg->Flags &= ~PRJCFGF_VALID;
		THROW(p_ref->PutProp(PPOBJ_CONFIG, PPCFG_MAIN, PPPRP_PROJECTCFG, pCfg, sizeof(*pCfg), 0));
		DS.LogAction(is_new ? PPACN_CONFIGCREATED : PPACN_CONFIGUPDATED, PPCFGOBJ_PROJECT, 0, 0, 0);
		THROW(tra.Commit());
	}
	PPObjProject::DirtyConfig();
	CATCHZOK
	return ok;
}

/*static*/int PPObjProject::EditConfig()
{
	class ProjectCfgDialog : public TDialog {
	public:
		struct Rec {
			PPOpCounterPacket PrjCntr;
			PPOpCounterPacket PhsCntr;
			PPOpCounterPacket TodoCntr;
			PPOpCounterPacket ToDtCntr;
			PPProjectConfig   Cfg;
		};
		DECL_DIALOG_DATA(Rec);

		ProjectCfgDialog() : TDialog(DLG_PRJCFG)
		{
		}
		DECL_DIALOG_SETDTS()
		{
			RVALUEPTR(Data, pData);
			setCtrlData(CTL_PRJCFG_PRJTEMPL,      Data.PrjCntr.Head.CodeTemplate);
			setCtrlLong(CTL_PRJCFG_PRJCNTR,       Data.PrjCntr.Head.Counter);
			setCtrlData(CTL_PRJCFG_PHSTEMPL,      Data.PhsCntr.Head.CodeTemplate);
			setCtrlLong(CTL_PRJCFG_PHSCNTR,       Data.PhsCntr.Head.Counter);
			setCtrlData(CTL_PRJCFG_TODOTEMPL,     Data.TodoCntr.Head.CodeTemplate);
			setCtrlLong(CTL_PRJCFG_TODOCNTR,      Data.TodoCntr.Head.Counter);
			setCtrlData(CTL_PRJCFG_TODTTEMPL,     Data.ToDtCntr.Head.CodeTemplate);
			setCtrlLong(CTL_PRJCFG_TODTCNTR,      Data.ToDtCntr.Head.Counter);
			setCtrlData(CTL_PRJCFG_NEWTASKTERM,  &Data.Cfg.NewTaskTerm);
			setCtrlData(CTL_PRJCFG_REJTASKTERM,  &Data.Cfg.RejTaskTerm);
			setCtrlData(CTL_PRJCFG_TEMPLGENTERM, &Data.Cfg.TemplGenTerm);
			setCtrlData(CTL_PRJCFG_REFRESHTIME,  &Data.Cfg.RefreshTime);
			AddClusterAssoc(CTL_PRJCFG_FLAGS, 0, PRJCFGF_NEWTASKNOTICEONLOGIN);
			AddClusterAssoc(CTL_PRJCFG_FLAGS, 1, PRJCFGF_NEWTASKNOTICE);
			AddClusterAssoc(CTL_PRJCFG_FLAGS, 2, PRJCFGF_INCOMPLETETASKREMIND);
			SetClusterData(CTL_PRJCFG_FLAGS,  Data.Cfg.Flags);
			SetIntRangeInput(this, CTL_PRJCFG_REMINDPRD, &Data.Cfg.RemindPrd);
			{
				PPIDArray types;
				types.addzlist(PPOPT_ACCTURN, PPOPT_GOODSRECEIPT, PPOPT_GOODSEXPEND, PPOPT_GOODSRETURN,
					PPOPT_GOODSREVAL, PPOPT_GOODSMODIF, PPOPT_PAYMENT, PPOPT_CHARGE, PPOPT_GOODSACK,
					PPOPT_GENERIC, PPOPT_DRAFTRECEIPT, PPOPT_DRAFTEXPEND, 0L);
				SetupOprKindCombo(this, CTLSEL_PRJCFG_BILLOP, Data.Cfg.BillOpID, 0, &types, 0);
			}
			SetRealRangeInput(this, CTL_PRJCFG_WORKHOURS, Data.Cfg.WorkHoursBeg, Data.Cfg.WorkHoursEnd);
			SetupCtrls();
			return 1;
		}
		DECL_DIALOG_GETDTS()
		{
			int    ok = 1;
			uint   sel = 0;
			getCtrlData(CTL_PRJCFG_PRJTEMPL,        Data.PrjCntr.Head.CodeTemplate);
			getCtrlData(CTL_PRJCFG_PRJCNTR,        &Data.PrjCntr.Head.Counter);
			getCtrlData(CTL_PRJCFG_PHSTEMPL,        Data.PhsCntr.Head.CodeTemplate);
			getCtrlData(CTL_PRJCFG_PHSCNTR,        &Data.PhsCntr.Head.Counter);
			getCtrlData(CTL_PRJCFG_TODOTEMPL,       Data.TodoCntr.Head.CodeTemplate);
			getCtrlData(CTL_PRJCFG_TODOCNTR,       &Data.TodoCntr.Head.Counter);
			getCtrlData(CTL_PRJCFG_TODTTEMPL,       Data.ToDtCntr.Head.CodeTemplate);
			getCtrlData(CTL_PRJCFG_TODTCNTR,       &Data.ToDtCntr.Head.Counter);
			getCtrlData(CTL_PRJCFG_NEWTASKTERM,    &Data.Cfg.NewTaskTerm);
			getCtrlData(CTL_PRJCFG_REJTASKTERM,    &Data.Cfg.RejTaskTerm);
			getCtrlData(CTL_PRJCFG_TEMPLGENTERM,   &Data.Cfg.TemplGenTerm);
			getCtrlData(CTL_PRJCFG_REFRESHTIME,    &Data.Cfg.RefreshTime);
			GetClusterData(CTL_PRJCFG_FLAGS,       &Data.Cfg.Flags);
			getCtrlData(CTLSEL_PRJCFG_BILLOP,      &Data.Cfg.BillOpID);
			Data.Cfg.RemindPrd.Set(0);
			GetIntRangeInput(this, sel = CTL_PRJCFG_REMINDPRD, &Data.Cfg.RemindPrd);
			if(Data.Cfg.Flags & PRJCFGF_INCOMPLETETASKREMIND) {
				THROW_PP(Data.Cfg.RemindPrd.low <= Data.Cfg.RemindPrd.upp && (Data.Cfg.RemindPrd.low != 0 || Data.Cfg.RemindPrd.upp != 0), PPERR_INVPERIODINPUT);
			}
			double beg_h = 0.0, end_h = 0.0;
			GetRealRangeInput(this, sel = CTL_PRJCFG_WORKHOURS, &beg_h, &end_h);
			Data.Cfg.WorkHoursBeg = (long)beg_h;
			Data.Cfg.WorkHoursEnd = (long)end_h;
			THROW_PP(Data.Cfg.WorkHoursBeg <= Data.Cfg.WorkHoursEnd, PPERR_INVPERIODINPUT);
			ASSIGN_PTR(pData, Data);
			CATCH
				selectCtrl(sel);
				ok = 0;
			ENDCATCH
			return ok;
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isClusterClk(CTL_PRJCFG_FLAGS)) {
				SetupCtrls();
				clearEvent(event);
			}
		}
		void   SetupCtrls()
		{
			long   flags = 0;
			GetClusterData(CTL_PRJCFG_FLAGS, &flags);
			disableCtrl(CTL_PRJCFG_REMINDPRD, !(flags & PRJCFGF_INCOMPLETETASKREMIND));
			if(!(flags & PRJCFGF_INCOMPLETETASKREMIND)) {
				Data.Cfg.RemindPrd.low = Data.Cfg.RemindPrd.upp = 0;
				SetIntRangeInput(this, CTL_PRJCFG_REMINDPRD, &Data.Cfg.RemindPrd);
			}
		}
	};
	int    ok = -1;
	int    valid_data = 0;
	int    ta = 0;
	int    is_new = 0;
	ProjectCfgDialog * dlg = new ProjectCfgDialog;
	PPObjOpCounter opc_obj;
	ProjectCfgDialog::Rec data;
	THROW(CheckDialogPtr(&dlg));
	THROW(CheckCfgRights(PPCFGOBJ_PROJECT, PPR_READ, 0));
	is_new = ReadConfig(&data.Cfg);
	opc_obj.GetPacket(data.Cfg.PrjCntrID,   &data.PrjCntr);
	opc_obj.GetPacket(data.Cfg.PhaseCntrID, &data.PhsCntr);
	opc_obj.GetPacket(data.Cfg.TaskCntrID,  &data.TodoCntr);
	opc_obj.GetPacket(data.Cfg.TemplCntrID, &data.ToDtCntr);
	dlg->setDTS(&data);
	while(!valid_data && ExecView(dlg) == cmOK) {
		THROW(CheckCfgRights(PPCFGOBJ_PROJECT, PPR_MOD, 0));
		if(dlg->getDTS(&data) <= 0)
			PPError();
		else {
			THROW(PPObjProject_WriteConfig(&data.Cfg, &data.PrjCntr, &data.PhsCntr, &data.TodoCntr, &data.ToDtCntr));
			ok = valid_data = 1;
		}
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}

/*static*/SString & FASTCALL PPObjProject::MakeCodeString(const ProjectTbl::Rec * pRec, SString & rBuf)
{
	return rBuf.Z().Cat(pRec->Code).CatDiv('-', 1).Cat(pRec->Name);
}

PPProjectPacket::PPProjectPacket()
{
}

PPProjectPacket & PPProjectPacket::Z()
{
	MEMSZERO(Rec);
	SDescr.Z();
	SMemo.Z();
	TagL.Z(); // @v12.2.6
	return *this;
}

TLP_IMPL(PPObjProject, ProjectTbl, P_Tbl);

PPObjProject::PPObjProject(void * extraPtr) : PPObject(PPOBJ_PROJECT), ExtraPtr(extraPtr)
{
	TLP_OPEN(P_Tbl);
	ImplementFlags |= implStrAssocMakeList;
}

PPObjProject::~PPObjProject()
{
	TLP_CLOSE(P_Tbl);
}

int PPObjProject::DeleteObj(PPID id) { return PutPacket(&id, static_cast<PPProjectPacket *>(0), 0); }
int PPObjProject::Search(PPID id, void * b) { return SearchByID(P_Tbl, Obj, id, b); }
const char * PPObjProject::GetNamePtr() { return MakeCodeString(&P_Tbl->data, NameBuf).cptr(); }

StrAssocArray * PPObjProject::MakeStrAssocList(void * extraPtr /*parentPrjID*/)
{
	const   PPID parent_prj_id = reinterpret_cast<PPID>(extraPtr);
	StrAssocArray * p_list = new StrAssocArray;
	SString name_buf;
	ProjectTbl::Key2 k2;
	MEMSZERO(k2);
	k2.ParentID = parent_prj_id;
	THROW_MEM(p_list);
	{
		BExtQuery q(P_Tbl, 2);
		q.select(P_Tbl->ID, P_Tbl->Code, P_Tbl->Name, 0L).where(P_Tbl->ParentID == parent_prj_id);
		for(q.initIteration(false, &k2, spGe); q.nextIteration() > 0;)
			THROW_SL(p_list->Add(P_Tbl->data.ID, MakeCodeString(&P_Tbl->data, name_buf)));
		THROW_DB(BTROKORNFOUND);
	}
	CATCH
		ZDELETE(p_list);
	ENDCATCH
	return p_list;
}

int PPObjProject::GetFullName(PPID id, SString & rBuf)
{
	int    ok = -1;
	ProjectTbl::Rec rec;
	rBuf.Z();
	if(Search(id, &rec) > 0) {
		PPID   parent_id = 0;
		char   name_buf[128];
		SStack name_stack(sizeof(name_buf));
		do {
			parent_id = rec.ParentID;
			STRNSCPY(name_buf, strip(rec.Name));
			if(!name_stack.push(name_buf))
				return PPSetErrorSLib();
		} while(parent_id && Search(parent_id, &rec) > 0);
		while(name_stack.pop(name_buf))
			rBuf.CatDivIfNotEmpty('/', 1).Cat(name_buf);
		ok = 1;
	}
	return ok;
}

int PPObjProject::Browse(void * extraPtr) { return PPView::Execute(PPVIEW_PROJECT, 0, PPView::exefModeless, 0); }

int PPObjProject::SerializePacket(int dir, PPProjectPacket * pPack, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	THROW_SL(P_Tbl->SerializeRecord(dir, &pPack->Rec, rBuf, pSCtx));
	THROW_SL(pSCtx->Serialize(dir, pPack->SDescr, rBuf));
	THROW_SL(pSCtx->Serialize(dir, pPack->SMemo, rBuf));
	CATCHZOK
	return ok;
}

int PPObjProject::InitPacket(PPProjectPacket * pPack, int kind, PPID parentID, int use_ta)
{
	PPProjectConfig cfg;
	PPObjProject::ReadConfig(&cfg);
	PPObjOpCounter opc_obj;
	long   counter = 0;
	PPID   cntr_id = 0;
	pPack->Z();
	pPack->Rec.Kind = NZOR(kind, PPPRJK_PROJECT);
	if(pPack->Rec.Kind == PPPRJK_PROJECT)
		cntr_id = cfg.PrjCntrID;
	else if(pPack->Rec.Kind == PPPRJK_PHASE)
		cntr_id = cfg.PhaseCntrID;
	if(cntr_id)
		opc_obj.GetCode(cntr_id, &counter, pPack->Rec.Code, sizeof(pPack->Rec.Code), 0, use_ta);
	pPack->Rec.Dt = LConfig.OperDate;
	pPack->Rec.ParentID   = parentID;
	pPack->Rec.Status = PPPRJSTS_ACTIVE;
	return 1;
}

SString & PPObjProject::GetItemDescr(PPID id, SString & rBuf)
{
	rBuf.Z();
	PPRef->UtrC.GetText(TextRefIdent(Obj, id, PPTRPROP_DESCR), rBuf);
	rBuf.Transf(CTRANSF_UTF8_TO_INNER);
	return rBuf;
}

SString & PPObjProject::GetItemMemo(PPID id, SString & rBuf)
{
	rBuf.Z();
	PPRef->UtrC.GetText(TextRefIdent(Obj, id, PPTRPROP_MEMO), rBuf);
	rBuf.Transf(CTRANSF_UTF8_TO_INNER);
	return rBuf;
}

int PPObjProject::GetPacket(PPID id, PPProjectPacket * pPack)
{
	int    ok = -1;
	if(pPack) {
		Reference * p_ref = PPRef;
		pPack->Z();
		if(Search(id, &pPack->Rec) > 0) {
			GetItemDescr(id, pPack->SDescr);
			GetItemMemo(id, pPack->SMemo);
			p_ref->Ot.GetList(Obj, id, &pPack->TagL); // @v12.2.6
			ok = 1;
		}
	}
	else {
		ok = Search(id, 0);
	}
	return ok;
}

int PPObjProject::PutPacket(PPID * pID, PPProjectPacket * pPack, int use_ta)
{
	int    ok = 1;
	Reference * p_ref = PPRef;
    PPID   acn_id = 0;
	SString ext_buffer;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(pPack) {
			if(*pID) {
				THROW(UpdateByID(P_Tbl, Obj, *pID, &pPack->Rec, 0));
				THROW(p_ref->Ot.PutList(Obj, *pID, &pPack->TagL, 0)); // @v12.2.6
				acn_id = PPACN_OBJUPD;
			}
			else {
				THROW(AddObjRecByID(P_Tbl, Obj, pID, &pPack->Rec, 0));
				THROW(p_ref->Ot.PutList(Obj, *pID, &pPack->TagL, 0)); // @v12.2.6
				acn_id = PPACN_OBJADD;
			}
		}
		else if(*pID) {
			THROW(RemoveByID(P_Tbl, *pID, 0));
			THROW(p_ref->Ot.PutList(Obj, *pID, 0, 0)); // @v12.2.6
			acn_id = PPACN_OBJRMV;
		}
		{
			if(pPack)
				(ext_buffer = pPack->SDescr).Strip();
			else
				ext_buffer.Z();
			THROW(p_ref->UtrC.SetText(TextRefIdent(Obj, *pID, PPTRPROP_DESCR), ext_buffer.Transf(CTRANSF_INNER_TO_UTF8), 0));
			if(pPack)
				(ext_buffer = pPack->SMemo).Strip();
			else
				ext_buffer.Z();
			THROW(p_ref->UtrC.SetText(TextRefIdent(Obj, *pID, PPTRPROP_MEMO), ext_buffer.Transf(CTRANSF_INNER_TO_UTF8), 0));
		}
		DS.LogAction(acn_id, Obj, *pID, 0, 0);
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int PPObjProject::Read(PPObjPack * p, PPID id, void * stream, ObjTransmContext * pCtx)
{
	// @v12.2.6 @dbd_exchange @todo TagL
	int    ok = 1;
	PPProjectPacket * p_pack = new PPProjectPacket;
	THROW_MEM(p_pack);
	p->Data = p_pack;
	if(stream == 0) {
		THROW(GetPacket(id, p_pack) > 0);
	}
	else {
		SBuffer buffer;
		THROW_SL(buffer.ReadFromFile(static_cast<FILE *>(stream), 0));
		THROW(SerializePacket(-1, p_pack, buffer, &pCtx->SCtx));
	}
	CATCHZOK
	return ok;
}

int PPObjProject::Write(PPObjPack * p, PPID * pID, void * stream, ObjTransmContext * pCtx) // @srlz
{
	// @v12.2.6 @dbd_exchange @todo TagL
	int    ok = 1;
	PPProjectPacket * p_pack = 0;
	THROW(p && p->Data);
	p_pack = static_cast<PPProjectPacket *>(p->Data);
	if(stream == 0) {
		p_pack->Rec.ID = *pID;
		if(!PutPacket(pID, p_pack, 1)) {
			pCtx->OutputAcceptErrMsg(PPTXT_ERRACCEPTPROJECT, p_pack->Rec.ID, p_pack->Rec.Name);
			ok = -1;
		}
	}
	else {
		SBuffer buffer;
		THROW(SerializePacket(+1, p_pack, buffer, &pCtx->SCtx));
		THROW_SL(buffer.WriteToFile(static_cast<FILE *>(stream), 0, 0));
	}
	CATCHZOK
	return ok;
}

int PPObjProject::ProcessObjRefs(PPObjPack * p, PPObjIDArray * ary, int replace, ObjTransmContext * pCtx)
{
	// @v12.2.6 @dbd_exchange @todo TagL
	int    ok = 1;
	THROW(p && p->Data);
	{
		PPProjectPacket * p_pack = static_cast<PPProjectPacket *>(p->Data);
		THROW(ProcessObjRefInArray(PPOBJ_PROJECT, &p_pack->Rec.ParentID,   ary, replace));
		THROW(ProcessObjRefInArray(PPOBJ_PERSON,  &p_pack->Rec.MngrID,     ary, replace));
		THROW(ProcessObjRefInArray(PPOBJ_PERSON,  &p_pack->Rec.ClientID,   ary, replace));
		THROW(ProcessObjRefInArray(PPOBJ_PROJECT, &p_pack->Rec.TemplateID, ary, replace));
		THROW(ProcessObjRefInArray(PPOBJ_OPRKIND, &p_pack->Rec.BillOpID,   ary, replace));
	}
	CATCHZOK
	return ok;
}
//
//
//
class ProjectDialog : public TDialog {
	DECL_DIALOG_DATA(PPProjectPacket);
public:
	explicit ProjectDialog(uint dlgID) : TDialog(dlgID)
	{
		SetupCalDate(CTLCAL_PRJ_DT, CTL_PRJ_DT);
		SetupCalDate(CTLCAL_PRJ_START, CTL_PRJ_START);
		SetupCalDate(CTLCAL_PRJ_ESTFINISH, CTL_PRJ_ESTFINISH);
		SetupCalDate(CTLCAL_PRJ_FINISH, CTL_PRJ_FINISH);
		SetupInputLine(CTL_PRJ_DESCR, MKSTYPE(S_ZSTRING, 2048), MKSFMT(2048, 0));
	}
	DECL_DIALOG_SETDTS()
	{
		int    ok = 1;
		ushort v = 0;
		RVALUEPTR(Data, pData);
		setCtrlData(CTL_PRJ_NAME,      Data.Rec.Name);
		setCtrlData(CTL_PRJ_CODE,      Data.Rec.Code);
		setCtrlData(CTL_PRJ_DT,        &Data.Rec.Dt);
		setCtrlData(CTL_PRJ_START,     &Data.Rec.BeginDt);
		setCtrlData(CTL_PRJ_ESTFINISH, &Data.Rec.EstFinishDt);
		setCtrlData(CTL_PRJ_FINISH,    &Data.Rec.FinishDt);
		SetupPersonCombo(this, CTLSEL_PRJ_CLIENT, Data.Rec.ClientID, OLW_CANINSERT, PPPRK_CLIENT, 0);
		SetupPersonCombo(this, CTLSEL_PRJ_MNGR,   Data.Rec.MngrID,   OLW_CANINSERT, PPPRK_EMPL,   0);
		if(oneof2(Data.Rec.Kind, PPPRJK_PROJECT, PPPRJK_PRJTEMPLATE)) {
			switch(Data.Rec.Status) {
				case PPPRJSTS_ACTIVE:    v = 0; break;
				case PPPRJSTS_NONACTIVE: v = 1; break;
				case PPPRJSTS_ARCHIVED:  v = 2; break;
				default: v = 0; break;
			}
		}
		else if(oneof2(Data.Rec.Kind, PPPRJK_PHASE, PPPRJK_PHSTEMPLATE)) {
			switch(Data.Rec.Status) {
				case PPPRJSTS_ACTIVE:    v = 0; break;
				case PPPRJSTS_NONACTIVE: v = 1; break;
				case PPPRJSTS_ARCHIVED:  v = 1; break; // Invalid for PHASE
				default: v = 0; break;
			}
			if(Data.Rec.ParentID) {
				PPObjProject prj_obj;
				SString buf;
				prj_obj.GetFullName(Data.Rec.ParentID, buf);
				setStaticText(CTL_PRJ_PARENTNAME, buf);
			}
		}
		setCtrlData(CTL_PRJ_STATUS, &v);
		setCtrlString(CTL_PRJ_DESCR, Data.SDescr);
		{
			PPIDArray types;
			types.addzlist(PPOPT_ACCTURN, PPOPT_GOODSRECEIPT, PPOPT_GOODSEXPEND, PPOPT_GOODSRETURN,
				PPOPT_GOODSREVAL, PPOPT_GOODSMODIF, PPOPT_PAYMENT, PPOPT_CHARGE, PPOPT_GOODSACK,
				PPOPT_GENERIC, PPOPT_DRAFTRECEIPT, PPOPT_DRAFTEXPEND, 0L);
			SetupOprKindCombo(this, CTLSEL_PRJ_BILLOP, Data.Rec.BillOpID, 0, &types, 0);
		}
		return ok;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		ushort v;
		getCtrlData(CTL_PRJ_NAME,      Data.Rec.Name);
		getCtrlData(CTL_PRJ_CODE,      Data.Rec.Code);
		getCtrlData(CTL_PRJ_DT,        &Data.Rec.Dt);
		getCtrlData(CTL_PRJ_START,     &Data.Rec.BeginDt);
		getCtrlData(CTL_PRJ_ESTFINISH, &Data.Rec.EstFinishDt);
		getCtrlData(CTL_PRJ_FINISH,    &Data.Rec.FinishDt);
		getCtrlData(CTLSEL_PRJ_CLIENT, &Data.Rec.ClientID);
		getCtrlData(CTLSEL_PRJ_MNGR,   &Data.Rec.MngrID);
		getCtrlData(CTL_PRJ_STATUS, &(v = 0));
		if(oneof2(Data.Rec.Kind, PPPRJK_PROJECT, PPPRJK_PRJTEMPLATE)) {
			switch(v) {
				case 0: Data.Rec.Status = PPPRJSTS_ACTIVE; break;
				case 1: Data.Rec.Status = PPPRJSTS_NONACTIVE; break;
				case 2: Data.Rec.Status = PPPRJSTS_ARCHIVED; break;
				default: Data.Rec.Status = PPPRJSTS_ACTIVE; break;
			}
		}
		else if(oneof2(Data.Rec.Kind, PPPRJK_PHASE, PPPRJK_PHSTEMPLATE)) {
			switch(v) {
				case 0: Data.Rec.Status = PPPRJSTS_ACTIVE; break;
				case 1: Data.Rec.Status = PPPRJSTS_NONACTIVE; break;
				default: Data.Rec.Status = PPPRJSTS_ACTIVE; break;
			}
		}
		getCtrlString(CTL_PRJ_DESCR, Data.SDescr);
		getCtrlData(CTLSEL_PRJ_BILLOP, &Data.Rec.BillOpID);
		ASSIGN_PTR(pData, Data);
		return ok;
	}
private:
	DECL_HANDLE_EVENT
	{
		TDialog::handleEvent(event);
		if(event.isCmd(cmTags)) { // @v12.2.6
			Data.TagL.Oid.Obj = PPOBJ_PROJECT;
			EditObjTagValList(&Data.TagL, 0);
			clearEvent(event);
		}
	}
};
//
// If(parentPrjID != 0 && *pID == 0) then - необходимо создать элемент типа Phase
//
int PPObjProject::Edit(PPID * pID, void * extraPtr /*parentPrjID*/)
{
	const  PPID extra_parent_prj_id = reinterpret_cast<PPID>(extraPtr);
	int    ok = cmCancel;
	int    valid_data = 0;
	uint   dlg_id = 0;
	PPProjectPacket pack;
	PPProjectPacket parent_pack;
	ProjectDialog * dlg = 0;
	THROW(CheckRightsModByID(pID));
	if(*pID) {
		THROW(GetPacket(*pID, &pack) > 0);
		if(pack.Rec.Kind == PPPRJK_PROJECT)
			dlg_id = DLG_PROJECT;
		else if(pack.Rec.Kind == PPPRJK_PHASE)
			dlg_id = DLG_PRJPHASE;
	}
	else {
		if(extra_parent_prj_id == 0) {
			InitPacket(&pack, PPPRJK_PROJECT, 0, 1);
			dlg_id = DLG_PROJECT;
		}
		else if(GetPacket(extra_parent_prj_id, &parent_pack) > 0) {
			InitPacket(&pack, PPPRJK_PHASE, extra_parent_prj_id, 1);
			dlg_id = DLG_PRJPHASE;
		}
	}
	THROW(CheckDialogPtr(&(dlg = new ProjectDialog(dlg_id))));
	dlg->setDTS(&pack);
	for(valid_data = 0; !valid_data && ExecView(dlg) == cmOK;) {
		if(dlg->getDTS(&pack)) {
			THROW(PutPacket(pID, &pack, 1));
			ok = cmOK;
			valid_data = 1;
		}
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}
//
//
//
IMPLEMENT_PPFILT_FACTORY(Project); ProjectFilt::ProjectFilt() : PPBaseFilt(PPFILT_PROJECT, 0, 1)
{
	SetFlatChunk(offsetof(ProjectFilt, ReserveStart),
		offsetof(ProjectFilt, Reserve)-offsetof(ProjectFilt, ReserveStart)+sizeof(Reserve));
	Init(1, 0);
}
//
//
//
PPViewProject::PPViewProject() : PPView(&PrjObj, &Filt, PPVIEW_PROJECT, 0, REPORT_PROJECTVIEW), P_PrjTaskView(0)
{
}

PPViewProject::~PPViewProject()
{
	delete P_PrjTaskView;
}

int PPViewProject::EditBaseFilt(PPBaseFilt * pBaseFilt)
{
	int    ok = -1;
	TDialog * dlg = 0;
	ProjectFilt filt;
	THROW(Filt.IsA(pBaseFilt));
	filt = *static_cast<const ProjectFilt *>(pBaseFilt);
	THROW(CheckDialogPtr(&(dlg = new TDialog(DLG_PRJFLT))));
	dlg->SetupCalPeriod(CTLCAL_PRJFLT_PRDSTART, CTL_PRJFLT_PRDSTART);
	dlg->SetupCalPeriod(CTLCAL_PRJFLT_PRDESTFINISH, CTL_PRJFLT_PRDESTFINISH);
	SetPeriodInput(dlg, CTL_PRJFLT_PRDSTART, filt.StartPeriod);
	SetPeriodInput(dlg, CTL_PRJFLT_PRDESTFINISH, filt.EstFinishPeriod);
	SetupPersonCombo(dlg, CTLSEL_PRJFLT_CLIENT, filt.ClientID, 0, PPPRK_CLIENT, 0);
	SetupPersonCombo(dlg, CTLSEL_PRJFLT_MNGR,   filt.MngrID,   0, PPPRK_EMPL,   0);
	dlg->AddClusterAssoc(CTL_PRJFLT_FLAGS, 0, ProjectFilt::fShowNonActive);
	dlg->AddClusterAssoc(CTL_PRJFLT_FLAGS, 1, ProjectFilt::fShowArchived);
	dlg->SetClusterData(CTL_PRJFLT_FLAGS, filt.Flags);
	dlg->AddClusterAssocDef(CTL_PRJFLT_SORTORD, 0,  ProjectFilt::ordByName);
	dlg->AddClusterAssoc(CTL_PRJFLT_SORTORD, 1,  ProjectFilt::ordByBegDt);
	dlg->SetClusterData(CTL_PRJFLT_SORTORD, filt.SortOrd);
	for(int valid_data = 0; !valid_data && ExecView(dlg) == cmOK;) {
		GetPeriodInput(dlg, CTL_PRJFLT_PRDSTART,     &filt.StartPeriod);
		GetPeriodInput(dlg, CTL_PRJFLT_PRDESTFINISH, &filt.EstFinishPeriod);
		dlg->getCtrlData(CTLSEL_PRJFLT_CLIENT,  &filt.ClientID);
		dlg->getCtrlData(CTLSEL_PRJFLT_MNGR,    &filt.MngrID);
		dlg->GetClusterData(CTL_PRJFLT_FLAGS,   &filt.Flags);
		dlg->GetClusterData(CTL_PRJFLT_SORTORD, &filt.SortOrd);
		ASSIGN_PTR(static_cast<ProjectFilt *>(pBaseFilt), filt);
		ok = valid_data = 1;
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}

int PPViewProject::Init_(const PPBaseFilt * pBaseFilt)
{
	if(Helper_InitBaseFilt(pBaseFilt)) {
		Filt.StartPeriod.Actualize(ZERODATE);
		Filt.EstFinishPeriod.Actualize(ZERODATE);
		return 1;
	}
	else
		return 0;
}

int PPViewProject::InitIteration()
{
	int    ok = 1;
	int    idx = 3;
	DBQ  * dbq = 0;
	ProjectTbl::Key3 k3, k3_;
	ProjectTbl * t = PrjObj.P_Tbl;
	BExtQuery::ZDelete(&P_IterQuery);
	Counter.Init();
	THROW_MEM(P_IterQuery = new BExtQuery(t, idx, 256));
	P_IterQuery->select(t->ID, 0L);
	dbq = & (*dbq && daterange(t->BeginDt, &Filt.StartPeriod) &&
		daterange(t->EstFinishDt, &Filt.EstFinishPeriod));
	if(Filt.ParentID)
		dbq = & (*dbq && t->ParentID == Filt.ParentID);
	else
		dbq = & (*dbq && t->Kind == PPPRJK_PROJECT);
	dbq = ppcheckfiltid(dbq, t->ClientID, Filt.ClientID);
	dbq = ppcheckfiltid(dbq, t->MngrID, Filt.MngrID);
	P_IterQuery->where(*dbq);
	MEMSZERO(k3);
	k3.BeginDt = Filt.StartPeriod.low;
	k3_ = k3;
	Counter.Init(P_IterQuery->countIterations(0, &k3_, spGe));
	P_IterQuery->initIteration(false, &k3, spGe);
	CATCHZOK
	return ok;
}

int FASTCALL PPViewProject::NextIteration(ProjectViewItem * pItem)
{
	int    ok = -1;
	if(pItem) {
		memzero(pItem, sizeof(*pItem));
		if(Filt.Flags & ProjectFilt::fPrintPrjTasks) {
			PrjTaskViewItem  prj_task_item;
			if(P_PrjTaskView && P_PrjTaskView->NextIteration(&prj_task_item) > 0) {
				pItem->PrjTaskID = prj_task_item.ID;
				ok = 1;
			}
		}
		while(ok < 0 && P_IterQuery && P_IterQuery->nextIteration() > 0) {
			Counter.Increment();
			PPID   id = PrjObj.P_Tbl->data.ID;
			ProjectTbl::Rec rec;
			if(PrjObj.Search(id, &rec) > 0) {
				if(rec.Status == PPPRJSTS_NONACTIVE && !(Filt.Flags & ProjectFilt::fShowNonActive))
					continue;
				if(rec.Status == PPPRJSTS_ARCHIVED && !(Filt.Flags & ProjectFilt::fShowArchived))
					continue;
				*static_cast<ProjectTbl::Rec *>(pItem) = rec;
				ok = (Filt.Flags & ProjectFilt::fPrintPrjTasks) ? InitPrjTaskIterations(id) : 1;
			}
		}
	}
	if(ok < 0)
		ZDELETE(P_PrjTaskView);
	return ok;
}

int PPViewProject::Transmit(PPID /*id*/)
{
	int    ok = -1;
	ObjTransmitParam param;
	if(ObjTransmDialog(DLG_OBJTRANSM, &param) > 0) {
		ProjectViewItem item;
		const  PPIDArray & rary = param.DestDBDivList.Get();
		PPObjIDArray objid_ary;
		PPWaitStart();
		for(InitIteration(); NextIteration(&item) > 0; PPWaitPercent(GetCounter()))
			objid_ary.Add(PPOBJ_PROJECT, item.ID);
		THROW(PPObjectTransmit::Transmit(&rary, &objid_ary, &param));
		ok = 1;
	}
	CATCHZOKPPERR
	PPWaitStop();
	return ok;
}

DBQuery * PPViewProject::CreateBrowserQuery(uint * pBrwId, SString * pSubTitle)
{
	uint   brw_id = 0;
	DBQuery * q  = 0;
	ProjectTbl * t = 0;
	DBE    dbe_mgr;
	DBE    dbe_cli;
	DBE    dbe_descr;
	DBQ  * dbq = 0;
	if(Filt.ParentID == 0)
		brw_id = BROWSER_PROJECT;
	else
		brw_id = BROWSER_PRJPHASE;
	THROW(CheckTblPtr(t = new ProjectTbl));
	dbq = & (*dbq && daterange(t->BeginDt, &Filt.StartPeriod) && daterange(t->EstFinishDt, &Filt.EstFinishPeriod));
	if(Filt.ParentID)
		dbq = & (*dbq && t->ParentID == Filt.ParentID);
	else
		dbq = & (*dbq && t->Kind == PPPRJK_PROJECT);
	dbq = ppcheckfiltid(dbq, t->ClientID, Filt.ClientID);
	dbq = ppcheckfiltid(dbq, t->MngrID, Filt.MngrID);
	{
		PPIDArray status_list;
		status_list.add(PPPRJSTS_ACTIVE);
		if(Filt.Flags & ProjectFilt::fShowNonActive)
			status_list.add(PPPRJSTS_NONACTIVE);
		if(Filt.Flags & ProjectFilt::fShowArchived)
			status_list.add(PPPRJSTS_ARCHIVED);
		dbq = ppcheckfiltidlist(dbq, t->Status, &status_list);
	}
	PPDbqFuncPool::InitObjNameFunc(dbe_cli, PPDbqFuncPool::IdObjNamePerson, t->ClientID);
	PPDbqFuncPool::InitObjNameFunc(dbe_mgr, PPDbqFuncPool::IdObjNamePerson, t->MngrID);
	{
		dbe_descr.init();
		dbe_descr.push(dbconst(PPOBJ_PROJECT));
		dbe_descr.push(t->ID);
		dbe_descr.push(dbconst(static_cast<long>(PPTRPROP_DESCR)));
		dbe_descr.push(static_cast<DBFunc>(PPDbqFuncPool::IdUnxText));
	}
	q = & select(
		t->ID,          // #00
		t->Name,        // #01
		t->Code,        // #02
		t->Dt,          // #03
		t->BeginDt,     // #04
		t->EstFinishDt, // #05
		t->FinishDt,    // #06
		t->Status,      // #07
		dbe_cli,        // #08
		dbe_mgr,        // #09
		dbe_descr,      // #10
		0L).from(t, 0L).where(*dbq);
	if(Filt.SortOrd == ProjectFilt::ordByName)
		q->orderBy(t->ParentID, t->Name, 0L);
	else
		q->orderBy(t->BeginDt, 0L);
	THROW(CheckQueryPtr(q));
	if(pSubTitle)
		PrjObj.GetFullName(Filt.ParentID, *pSubTitle);
	CATCH
		if(q)
			ZDELETE(q);
		else
			delete t;
	ENDCATCH
	ASSIGN_PTR(pBrwId, brw_id);
	return q;
}

int PPViewProject::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	if(ok == -2) {
		PPID   id = pHdr ? *static_cast<const  PPID *>(pHdr) : 0;
		switch(ppvCmd) {
			case PPVCMD_VIEWPRJPHASE:
				ok = -1;
				if(id) {
					ProjectTbl::Rec prj_rec;
					if(PrjObj.Search(id, &prj_rec) > 0) {
						ProjectFilt filt;
						filt.ParentID = id;
						PPView::Execute(PPVIEW_PROJECT, &filt, PPView::exefModeless, 0);
					}
				}
				break;
			case PPVCMD_TRANSMIT:
				ok = -1;
				Transmit(0);
				break;
			case PPVCMD_PRNPRJTASKS:
				ok = -1;
				if(id)
					PrintProjectTasks(id);
				break;
		}
	}
	return ok;
}

void * PPViewProject::GetEditExtraParam() { return reinterpret_cast<void *>(Filt.ParentID); }

int PPViewProject::Detail(const void * pHdr, PPViewBrowser * pBrw)
{
	PPID   id = pHdr ? *static_cast<const  PPID *>(pHdr) : 0;
	if(id) {
		PrjTaskFilt filt;
		filt.Init(1, 0);
		filt.ProjectID = id;
		ViewPrjTask(&filt);
	}
	return -1;
}

void PPViewProject::ViewTotal()
{
}

int PPViewProject::InitPrjTaskIterations(PPID prjID)
{
	int  ok = 1;
	PrjTaskFilt  filt;
	filt.Init(1, 0);
	filt.ProjectID = prjID;
	filt.Flags = PrjTaskFilt::fNotShowPPWaitOnInit;
	if(P_PrjTaskView)
		ZDELETE(P_PrjTaskView);
	THROW_MEM(P_PrjTaskView = new PPViewPrjTask);
	THROW(P_PrjTaskView->Init_(&filt));
	THROW(P_PrjTaskView->InitIteration());
	CATCH
		ZDELETE(P_PrjTaskView);
		ok = 0;
	ENDCATCH
	return ok;
}

SString & PPViewProject::GetItemDescr(PPID id, SString & rBuf) { return PrjObj.GetItemDescr(id, rBuf); }
SString & PPViewProject::GetItemMemo(PPID id, SString & rBuf) { return PrjObj.GetItemMemo(id, rBuf); }

int PPViewProject::PrintProjectTasks(PPID prjID)
{
	int    ok = -1;
	PPViewProject prj_view;
	ProjectFilt filt;
	filt.ParentID = prjID;
	filt.Flags    = ProjectFilt::fPrintPrjTasks;
	prj_view.Init_(&filt);
	if(prj_view.InitPrjTaskIterations(prjID) > 0)
		ok = PPAlddPrint(REPORT_PROJECTTASKS, PView(&prj_view), 0);
	return ok;
}

int PPViewProject::Export()
{
	return -1;
}
//
// PrjTaskCore
//
/*static*/int FASTCALL PrjTaskCore::IsValidStatus(int s) { return (s >= 1 && s <= 5) ? 1 : PPSetError(PPERR_INVTODOSTATUS); }
/*static*/int FASTCALL PrjTaskCore::IsValidPrior(int p) { return (p >= 1 && p <= 5); }

PrjTaskCore::PrjTaskCore() : PrjTaskTbl()
{
}

int PrjTaskCore::Search(PPID id, PrjTaskTbl::Rec * pRec) { return SearchByID(this, PPOBJ_PRJTASK, id, pRec); }
int PrjTaskCore::NextEnum(long enumHandle, PrjTaskTbl::Rec * pRec) { return (EnumList.NextIter(enumHandle) > 0) ? (CopyBufTo(pRec), 1) : -1; }
int PrjTaskCore::DestroyIter(long enumHandle) { return EnumList.DestroyIterHandler(enumHandle); }

BExtQuery * PrjTaskCore::StartupEnumQuery(int idx, int options)
{
	BExtQuery * q = new BExtQuery(this, idx);
	if(q) {
		q->select(this->ID, this->ProjectID, this->Kind, this->Code, this->CreatorID, this->EmployerID,
			this->ClientID, this->TemplateID, this->Dt, this->Tm, this->StartDt, this->StartTm,
			this->EstFinishDt, this->EstFinishTm, this->FinishDt, this->FinishTm, this->Priority,
			this->Status, this->Flags, this->DlvrAddrID, this->LinkTaskID, this->Amount, 0L);
	}
	return q;
}

SEnum::Imp * PrjTaskCore::EnumByClient(PPID cliPersonID, const DateRange * pPeriod, int options)
{
	long   h = -1;
	int    idx = 5;
	BExtQuery * q = StartupEnumQuery(idx, options);
	q->where(this->ClientID == cliPersonID && daterange(this->Dt, pPeriod));
	PrjTaskTbl::Key5 k5;
	MEMSZERO(k5);
	k5.ClientID = cliPersonID;
	k5.Dt = pPeriod ? pPeriod->low : ZERODATE;
	q->initIteration(false, &k5, spGe);
	return EnumList.RegisterIterHandler(q, &h) ? new PPTblEnum <PrjTaskCore>(this, h) : 0;
}

SEnum::Imp * PrjTaskCore::EnumByEmployer(PPID emplPersonID, const DateRange * pPeriod, int options)
{
	long   h = -1;
	int    idx = 4;
	BExtQuery * q = StartupEnumQuery(idx, options);
	q->where(this->EmployerID == emplPersonID && daterange(this->Dt, pPeriod));
	PrjTaskTbl::Key4 k4;
	MEMSZERO(k4);
	k4.EmployerID = emplPersonID;
	k4.Dt = pPeriod ? pPeriod->low : ZERODATE;
	q->initIteration(false, &k4, spGe);
	return EnumList.RegisterIterHandler(q, &h) ? new PPTblEnum <PrjTaskCore>(this, h) : 0;
}

int PrjTaskCore::SearchByTime(const LDATETIME & dtm, PPID * pID, PrjTaskTbl::Rec * pRec)
{
	PrjTaskTbl::Key1 k1;
	k1.Dt = dtm.d;
	k1.Tm = dtm.t;
	int    ok = SearchByKey(this, 1, &k1, pRec);
	if(ok > 0)
		ASSIGN_PTR(pID, data.ID);
	return ok;
}

int PrjTaskCore::SearchByTemplate(PPID templID, LDATE startDt, PrjTaskTbl::Rec * pRec)
{
	PrjTaskTbl::Key3 k3;
	MEMSZERO(k3);
	k3.TemplateID = templID;
	BExtQuery q(this, 3);
	q.select(this->ID, 0L).where(this->TemplateID == templID && this->StartDt == startDt);
	for(q.initIteration(false, &k3, spGe); q.nextIteration() > 0;)
		if(Search(data.ID, pRec) > 0)
			return 1;
	return -1;
}

int PrjTaskCore::Add(PPID * pID, PrjTaskTbl::Rec * pRec, int use_ta)
{
	LDATETIME dtm;
	dtm.Set(pRec->Dt, pRec->Tm);
	while(SearchByTime(dtm, 0, 0) > 0)
		dtm.t.v++;
	pRec->Tm = dtm.t;
	return AddObjRecByID(this, PPOBJ_PRJTASK, pID, pRec, use_ta);
}

int PrjTaskCore::Update(PPID id, PrjTaskTbl::Rec * pRec, int use_ta)
{
	LDATETIME dtm;
	dtm.Set(pRec->Dt, pRec->Tm);
	while(SearchByTime(dtm, 0, 0) > 0)
		dtm.t.v++;
	pRec->Tm = dtm.t;
	return UpdateByID(this, PPOBJ_PRJTASK, id, pRec, use_ta);
}

int PrjTaskCore::UpdateStatus(PPID id, int newStatus, int use_ta)
{
	int    ok = 1;
	PrjTaskTbl::Rec rec;
	THROW(PrjTaskCore::IsValidStatus(newStatus));
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(Search(id, &rec) > 0);
		if(rec.Status != newStatus) {
			rec.Status = newStatus;
			THROW(Update(id, &rec, 0));
		}
		else
			ok = -1;
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int PrjTaskCore::Remove(PPID id, int use_ta)
	{ return deleteFrom(this, use_ta, this->ID == id) ? 1 : PPSetErrorDB(); }
int PrjTaskCore::RemoveByProject(PPID prjID, int use_ta)
	{ return deleteFrom(this, use_ta, this->ProjectID == prjID) ? 1 : PPSetErrorDB(); }

int PrjTaskCore::SearchAnyRef(PPID objType, PPID objID, PPID * pID)
{
	int    ok = -1;
	union {
		PrjTaskTbl::Key0 k0;
		PrjTaskTbl::Key2 k2;
		PrjTaskTbl::Key3 k3;
		PrjTaskTbl::Key6 k6;
	} k_;
	MEMSZERO(k_);
	if(objType == PPOBJ_PERSON) {
		BExtQuery q(this, 0, 1);
		q.select(this->ID, 0).where(this->CreatorID == objID || this->EmployerID == objID || this->ClientID == objID);
		q.initIteration(false, &k_, spFirst);
		if(q.nextIteration() > 0) {
			ASSIGN_PTR(pID, data.ID);
			ok = 1;
		}
	}
	else if(objType == PPOBJ_ARTICLE) {
		BExtQuery q(this, 0, 1);
		q.select(this->ID, 0).where(this->BillArID == objID);
		q.initIteration(false, &k_, spFirst);
		if(q.nextIteration() > 0) {
			ASSIGN_PTR(pID, data.ID);
			ok = 1;
		}
	}
	else if(objType == PPOBJ_LOCATION) {
		BExtQuery q(this, 0, 1);
		q.select(this->ID, 0).where(this->DlvrAddrID == objID);
		q.initIteration(false, &k_, spFirst);
		if(q.nextIteration() > 0) {
			ASSIGN_PTR(pID, data.ID);
			ok = 1;
		}
	}
	else if(objType == PPOBJ_PROJECT) {
		k_.k2.ProjectID = objID;
		if(search(2, &k_.k2, spGe) && k_.k2.ProjectID == objID) {
			ASSIGN_PTR(pID, data.ID);
			ok = 1;
		}
	}
	else if(objType == PPOBJ_PRJTASK) {
		k_.k3.TemplateID = objID;
		if(search(3, &k_.k3, spGe) && k_.k3.TemplateID == objID) {
			ASSIGN_PTR(pID, data.ID);
			ok = 1;
		}
		else {
			k_.k6.LinkTaskID = objID;
			if(search(6, &k_.k6, spGe) && k_.k6.LinkTaskID == objID) {
				ASSIGN_PTR(pID, data.ID);
				ok = 1;
			}
		}
	}
	return ok;
}

int PrjTaskCore::ReplaceRefs(PPID objType, PPID replacedID, PPID newID, int use_ta)
{
	int    ok = 1;
	union {
		PrjTaskTbl::Key0 k0;
	} k_;
	if(oneof3(objType, PPOBJ_PERSON, PPOBJ_ARTICLE, PPOBJ_LOCATION)) {
		PPIDArray id_list;
		MEMSZERO(k_);
		BExtQuery q(this, 0);
		DBQ * dbq = 0;
		if(objType == PPOBJ_PERSON)
			dbq = &(this->CreatorID == replacedID || this->EmployerID == replacedID || this->ClientID == replacedID);
		else if(objType == PPOBJ_ARTICLE)
			dbq = &(this->BillArID == replacedID);
		else if(objType == PPOBJ_LOCATION)
			dbq = &(this->DlvrAddrID == replacedID);
		q.select(this->ID, 0).where(*dbq);
		for(q.initIteration(false, &k_, spFirst); q.nextIteration() > 0;) {
			id_list.add(data.ID);
		}
		if(id_list.getCount()) {
			PPTransaction tra(use_ta);
			THROW(tra);
			for(uint i = 0; i < id_list.getCount(); i++) {
				const  PPID id = id_list.get(i);
				PrjTaskTbl::Rec rec;
				if(Search(id, &rec) > 0) {
					int do_update = 0;
					switch(objType) {
						case PPOBJ_PERSON:
							if(rec.CreatorID == replacedID) {
								rec.CreatorID = newID;
								do_update = 1;
							}
							if(rec.EmployerID == replacedID) {
								rec.EmployerID = newID;
								do_update = 1;
							}
							if(rec.ClientID == replacedID) {
								rec.ClientID = newID;
								do_update = 1;
							}
							break;
						case PPOBJ_ARTICLE:
							if(rec.BillArID == replacedID) {
								rec.BillArID = newID;
								do_update = 1;
							}
							break;
						case PPOBJ_LOCATION:
							if(rec.DlvrAddrID == replacedID) {
								rec.DlvrAddrID = newID;
								do_update = 1;
							}
							break;
					}
					if(do_update)
						THROW(Update(id, &rec, 0));
				}
			}
			THROW(tra.Commit());
		}
	}
	CATCHZOK
	return ok;
}

int PrjTaskCore::GetSingleByCode(long kind, const char * pCode, PPID * pID)
{
	int    ok = -1;
	PPID   id = 0;
	if(pCode && pCode[0]) {
		int    count = 0;
		PrjTaskTbl::Key0  k0;
		DBQ  * dbq = 0;
		BExtQuery q(this, 0);
		q.select(ID, Kind, Code, 0L);
		if(kind)
			dbq = & (*dbq && (Kind == kind));
		q.where(*dbq);
		MEMSZERO(k0);
		for(q.initIteration(false, &k0, spGe); q.nextIteration() > 0;)
			if(strcmp(data.Code, pCode) == 0) {
				if(++count > 1) {
					id = 0;
					ok = 0;
					break;
				}
				else {
					id = data.ID;
					ok = 1;
				}
			}
	}
	ASSIGN_PTR(pID, id);
	return ok;
}
//
// @ModuleDef(PPObjPrjTask)
//
static SString & _GetEnumText(uint strId, int i, SString & rBuf)
{
	int    ok = 0;
	SString item_buf;
	for(int idx = 0; !ok && PPGetSubStr(strId, idx, item_buf) > 0; idx++) {
		long   id = 0;
		const  char * p = item_buf.SearchChar(',', 0);
		if(p) {
			id = atol(item_buf);
			p++;
		}
		else {
			p = item_buf;
			id = idx + 1;
		}
		if(id == i) {
			(rBuf = p).Strip();
			ok = 1;
		}
	}
	if(!ok)
		rBuf.Z();
	return rBuf;
}

/*static*/SString & PPObjPrjTask::GetStatusText(int statusId, SString & rBuf)
	{ return _GetEnumText(PPTXT_TODO_STATUS, statusId, rBuf); }
/*static*/SString & PPObjPrjTask::GetPriorText(int priorId, SString & rBuf)
	{ return _GetEnumText(PPTXT_TODO_PRIOR, priorId, rBuf); }


iCalendarImportParam::iCalendarImportParam() : TodoDefCreatorID(0), TodoDefClientID(0), TodoDefEmployerID(0)
{
}

iCalendarImportParam & iCalendarImportParam::Z()
{
	TodoDefCreatorID = 0;
	TodoDefClientID = 0;
	TodoDefEmployerID = 0;
	FilePath.Z();
	return *this;
}

class VCalImportParamDlg : public TDialog {
public:
	/*struct Param {
		Param() : DefCreatorID(0), DefClientID(0), DefEmployerID(0)
		{
		}
		void Init()
		{
			DefCreatorID = 0;
			DefClientID = 0;
			DefEmployerID = 0;
			FilePath.Z();
		}
		PPID   DefCreatorID;
		PPID   DefClientID;
		PPID   DefEmployerID;
		SString FilePath;
	};*/
private:
	DECL_DIALOG_DATA(iCalendarImportParam);
	enum {
		ctlgroupFileName = 1
	};
public:
	VCalImportParamDlg() : TDialog(DLG_VCALPAR)
	{
		addGroup(ctlgroupFileName, new FileBrowseCtrlGroup(CTLBRW_VCALPAR_FILE, CTL_VCALPAR_FILE, 0, 0));
	}
	DECL_DIALOG_SETDTS()
	{
		FileBrowseCtrlGroup * p_grp = static_cast<FileBrowseCtrlGroup *>(getGroup(ctlgroupFileName));
		if(!RVALUEPTR(Data, pData))
			Data.Z();
		SetupPersonCombo(this, CTLSEL_VCALPAR_CREATOR, Data.TodoDefCreatorID,  0, PPPRK_EMPL, 0);
		SetupPersonCombo(this, CTLSEL_VCALPAR_EMPL,    Data.TodoDefEmployerID, 0, PPPRK_EMPL, 0);
		SetupPersonCombo(this, CTLSEL_VCALPAR_CLIENT,  Data.TodoDefClientID,   0, PPPRK_CLIENT, 0);
		CALLPTRMEMB(p_grp, addPattern(PPTXT_FILPAT_VCALENDAR));
		return 1;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = -1;
		uint   sel = 0;
		getCtrlData(sel = CTLSEL_VCALPAR_CREATOR, &Data.TodoDefCreatorID);
		// @v11.0.3 THROW_PP(Data.TodoDefCreatorID, PPERR_INVDEFCREATOR);
		getCtrlData(CTLSEL_VCALPAR_EMPL,    &Data.TodoDefClientID);
		getCtrlData(CTLSEL_VCALPAR_CLIENT,  &Data.TodoDefEmployerID);
		{
			getCtrlString(sel = CTL_VCALPAR_FILE, Data.FilePath);
			SLibError = SLERR_OPENFAULT;
			PPSetAddedMsgString(Data.FilePath);
			THROW_SL(access(Data.FilePath, 0) == 0);
			ok = 1;
		}
		ASSIGN_PTR(pData, Data);
		CATCH
			ok = (selectCtrl(sel), 0);
		ENDCATCH
		return ok;
	}
};

struct iCalendarParsingBlock {
	enum {
		statusUnkn  = -1, // Не удалось идентифицировать статус по тексту
		statusUndef = 0,
		statusNeedsAction = 1,
		statusCompleted,
		statusInProcess,
		statusCancelled,
	};
	enum {
		roleUnkn = -1,
		roleUndef = 0,
		roleChair,
		roleReqParticipant,
		roleOptParticipant,
		roleNoneParticipant
	};
	struct Person {
		enum {
			refasUndef = 0,
			refasOrganizer,
			refasContact,
			refasAttendee
		};
		Person() : RefAs(refasUndef), Role(roleUndef)
		{
		}
		int    RefAs; // Признак, по которому объект ссылается на персоналию (refasXXX)
		int    Role;
		long   XPapyrusID;
		SString TxtCN;
		SString TxtValue;
		SString TxtUID;
		SString TxtINN;
		SString TxtKPP;
		SString TxtPhone;
	};
	iCalendarParsingBlock() : XPapyrusID(0), Priority(0), Status(statusUndef)
	{
		DtmDtStamp.Z();
		DtmCreated.Z();
		DtmLastModified.Z();
		DtmDtStart.Z();
		DtmDue.Z();
		DtmDtEnd.Z();
		DtmCompleted.Z();
	}
	iCalendarParsingBlock & Z()
	{
		XPapyrusID = 0;
		Priority = 0;
		Status = statusUndef;
		DtmDtStamp.Z();
		DtmCreated.Z();
		DtmLastModified.Z();
		DtmDtStart.Z();
		DtmDue.Z();
		DtmDtEnd.Z();
		DtmCompleted.Z();
		TxtUID.Z();
		TxtSummary.Z();
		TxtDescription.Z();
		TxtComment.Z();
		XPapyrusCode.Z();
		PersonList.freeAll();
		return *this;
	}
	int    CreatePersonEntry(int refas, const VCalendar::Entry & rIcEntry)
	{
		int    ok = 1;
		iCalendarParsingBlock::Person * p_new_psn = 0;
		assert(oneof3(refas, Person::refasOrganizer, Person::refasContact, Person::refasAttendee));
		THROW(oneof3(refas, Person::refasOrganizer, Person::refasContact, Person::refasAttendee));
		THROW_SL(p_new_psn = PersonList.CreateNewItem());
		p_new_psn->RefAs = refas;
		p_new_psn->TxtValue = rIcEntry.Value;
		{
			for(uint i = 0; i < rIcEntry.ParamList.getCount(); i++) {
				StrAssocArray::Item item = rIcEntry.ParamList.Get(i);
				switch(item.Id) {
					case VCalendar::tokROLE:
						if(p_new_psn->Role == iCalendarParsingBlock::roleUndef) {
							int st = VCalendar::GetToken(item.Txt);
							switch(st) {
								case VCalendar::tokCHAIR: p_new_psn->Role = iCalendarParsingBlock::roleChair; break;
								case VCalendar::tokREQPARTICIPANT: p_new_psn->Role = iCalendarParsingBlock::roleReqParticipant; break;
								case VCalendar::tokOPTPARTICIPANT: p_new_psn->Role = iCalendarParsingBlock::roleOptParticipant; break;
								case VCalendar::tokNONPARTICIPANT: p_new_psn->Role = iCalendarParsingBlock::roleNoneParticipant; break;
								default: p_new_psn->Role = iCalendarParsingBlock::roleUnkn; break;
							}
						}
						break;
					case VCalendar::tokCN: p_new_psn->TxtCN = item.Txt; break;
					case VCalendar::tokXPAPYRUSID: p_new_psn->XPapyrusID = satoi(item.Txt); break;
					case VCalendar::tokXRUINN: p_new_psn->TxtINN = item.Txt; break;
					case VCalendar::tokXRUKPP: p_new_psn->TxtKPP = item.Txt; break;
					case VCalendar::tokXPHONE: p_new_psn->TxtPhone = item.Txt; break;
				}
			}
		}
		CATCHZOK
		return ok;
	}
	long   XPapyrusID;
	int    Priority;
	int    Status; // statusXXX
	LDATETIME DtmDtStamp;
	LDATETIME DtmCreated;
	LDATETIME DtmLastModified;
	LDATETIME DtmDtStart;
	LDATETIME DtmDue;
	LDATETIME DtmDtEnd;
	LDATETIME DtmCompleted;
	SString TxtUID;
	SString TxtSummary;
	SString TxtDescription;
	SString TxtComment;
	SString XPapyrusCode;
	TSCollection <Person> PersonList;
};

int PPObjPrjTask::ImportFromOuterFormat(const char * pInput, const iCalendarImportParam * pParam, TSCollection <PPPrjTaskPacket> & rList) // @v11.0.3 @construction
{
	enum {
		icstateUNDEF = 0,
		icstateVCALENDAR = 1, // begin:vcalendar
		icstateVEVENT,
		icstateVTODO,
		icstateVJOURNAL,
		icstateVALARM,
		icstateUNKN // BEGIN:something-unknown (neither vevent, no vtodo, no vjournal no valarm)
	};

	int    ok = -1;
	Reference * p_ref = PPRef;
	int    format = 0;
	PPObjPerson psn_obj;
	SString temp_buf;
	SStrScan scan(pInput);
	SString line_buf;
	uint   line_no = 0;
	int    current_ic_vcalendar_state = icstateUNDEF; // icstateUNDEF || icstateVCALENDAR
	int    current_ic_state = icstateUNDEF; // icstateUNDEF || icstateVEVENT || icstateVTODO || icstateVJOURNAL || icstateVALARM || icstateUNKN
	PPPrjTaskPacket * p_curr_todo_pack = 0; // assert(!p_curr_todo_pack || current_ic_state == icstateVTODO)
	iCalendarParsingBlock pb;
	VCalendar::Entry ic_entry;
	int    tok = 0;
	bool   first_nonempty_line = true;
	int    getlr = 0;
	while((getlr = scan.GetLine(eolSpcICalendar, line_buf)) != 0) {
		line_no++;
		line_buf.Strip();
		if(line_buf.IsEmpty() && getlr < 0) // Последняя строка пустая - уходим
			break;
		else {
			if(line_buf.Len() && first_nonempty_line) {
				first_nonempty_line = false;
				if(line_buf.IsEqiAscii("BEGIN:VCALENDAR"))
					format = piefICalendar;
			}
			if(format == piefICalendar) {
				if(VCalendar::ParseLine(line_buf, ic_entry)) {
					const int val_tok = VCalendar::GetToken(ic_entry.Value);
					switch(ic_entry.Token) {
						case VCalendar::tokBEGIN:
							pb.Z();
							switch(val_tok) {
								case VCalendar::tokVCALENDAR:
									THROW(current_ic_vcalendar_state == icstateUNDEF);
									current_ic_vcalendar_state = icstateVCALENDAR;
									break;
								case VCalendar::tokVALARM:
									THROW(current_ic_state == icstateUNDEF);
									current_ic_state = icstateVALARM;
									break;
								case VCalendar::tokVJOURNAL:
									THROW(current_ic_state == icstateUNDEF);
									current_ic_state = icstateVJOURNAL;
									break;
								case VCalendar::tokVTODO:
									THROW(current_ic_state == icstateUNDEF);
									current_ic_state = icstateVTODO;
									assert(p_curr_todo_pack == 0);
									ZDELETE(p_curr_todo_pack);
									THROW_SL(p_curr_todo_pack = new PPPrjTaskPacket);
									break;
								case VCalendar::tokVEVENT:
									THROW(current_ic_state == icstateUNDEF);
									current_ic_state = icstateVEVENT;
									break;
								default:
									THROW(current_ic_state == icstateUNDEF);
									current_ic_state = icstateUNKN;
									break;
							}
							break;
						case VCalendar::tokEND:
							switch(val_tok) {
								case VCalendar::tokVCALENDAR:
									THROW(current_ic_vcalendar_state == icstateVCALENDAR);
									current_ic_vcalendar_state = icstateUNDEF;
									break;
								case VCalendar::tokVALARM:
									THROW(current_ic_state == icstateVALARM);
									current_ic_state = icstateUNDEF;
									break;
								case VCalendar::tokVJOURNAL:
									THROW(current_ic_state == icstateVJOURNAL);
									current_ic_state = icstateUNDEF;
									break;
								case VCalendar::tokVTODO:
									THROW(current_ic_state == icstateVTODO);
									current_ic_state = icstateUNDEF;
									assert(p_curr_todo_pack);
									if(p_curr_todo_pack) {
										THROW(InitPacket(p_curr_todo_pack, TODOKIND_TASK, 0, 0, 0, 0));
										p_curr_todo_pack->Rec.CreatorID = 0;
										if(checkdate(pb.DtmCreated.d)) {
											p_curr_todo_pack->Rec.Dt = pb.DtmCreated.d;
											p_curr_todo_pack->Rec.Tm = pb.DtmCreated.t;
										}
										else if(checkdate(pb.DtmDtStamp.d)) {
											p_curr_todo_pack->Rec.Dt = pb.DtmDtStamp.d;
											p_curr_todo_pack->Rec.Tm = pb.DtmDtStamp.t;
										}
										else if(checkdate(pb.DtmLastModified.d)) {
											p_curr_todo_pack->Rec.Dt = pb.DtmLastModified.d;
											p_curr_todo_pack->Rec.Tm = pb.DtmLastModified.t;
										}
										if(checkdate(pb.DtmDtStart.d)) {
											p_curr_todo_pack->Rec.StartDt = pb.DtmDtStart.d;
											p_curr_todo_pack->Rec.StartTm = pb.DtmDtStart.t;
										}
										if(checkdate(pb.DtmDue.d)) {
											p_curr_todo_pack->Rec.EstFinishDt = pb.DtmDue.d;
											p_curr_todo_pack->Rec.EstFinishTm = pb.DtmDue.t;
										}
										if(checkdate(pb.DtmCompleted.d)) {
											p_curr_todo_pack->Rec.FinishDt = pb.DtmCompleted.d;
											p_curr_todo_pack->Rec.FinishDt = pb.DtmCompleted.t;
										}
										else if(checkdate(pb.DtmDtEnd.d)) {
											p_curr_todo_pack->Rec.FinishDt = pb.DtmDtEnd.d;
											p_curr_todo_pack->Rec.FinishDt = pb.DtmDtEnd.t;
										}
										p_curr_todo_pack->Rec.Priority = pb.Priority;
										switch(pb.Status) {
											case iCalendarParsingBlock::statusNeedsAction: p_curr_todo_pack->Rec.Status = TODOSTTS_NEW; break;
											case iCalendarParsingBlock::statusInProcess: p_curr_todo_pack->Rec.Status = TODOSTTS_INPROGRESS; break;
											case iCalendarParsingBlock::statusCompleted: p_curr_todo_pack->Rec.Status = TODOSTTS_COMPLETED; break;
											case iCalendarParsingBlock::statusCancelled: p_curr_todo_pack->Rec.Status = TODOSTTS_REJECTED; break;
											default: p_curr_todo_pack->Rec.Status = TODOSTTS_NEW; break;
										}
										if(pb.TxtDescription.NotEmptyS()) {
											(p_curr_todo_pack->SDescr = pb.TxtDescription).Transf(CTRANSF_UTF8_TO_INNER);
										}
										else if(pb.TxtSummary.NotEmptyS()) {
											(p_curr_todo_pack->SDescr = pb.TxtSummary).Transf(CTRANSF_UTF8_TO_INNER);
										}
										if(pb.TxtComment.NotEmptyS()) {
											(p_curr_todo_pack->SMemo = pb.TxtComment).Transf(CTRANSF_UTF8_TO_INNER);
										}
										if(pb.XPapyrusCode.NotEmptyS()) {
											(temp_buf = pb.XPapyrusCode).Transf(CTRANSF_UTF8_TO_INNER);
											STRNSCPY(p_curr_todo_pack->Rec.Code, temp_buf);
										}
										else if(pb.TxtUID.NotEmptyS()) {
											(temp_buf = pb.TxtUID).Transf(CTRANSF_UTF8_TO_INNER);
											STRNSCPY(p_curr_todo_pack->Rec.Code, temp_buf);
										}
										{
											PPIDArray psn_candidate_list;
											for(uint i = 0; i < pb.PersonList.getCount(); i++) {
												const iCalendarParsingBlock::Person * p_ic_person = pb.PersonList.at(i);
												if(p_ic_person) {
													//PPPRK_EMPL
													//PPPRK_CLIENT
													PPID   person_id = 0;
													PPID   psn_kind_id = 0;
													if(p_ic_person->RefAs == iCalendarParsingBlock::Person::refasOrganizer)
														psn_kind_id = PPPRK_EMPL;
													else if(p_ic_person->RefAs == iCalendarParsingBlock::Person::refasContact)
														psn_kind_id = PPPRK_CLIENT;
													else if(p_ic_person->RefAs == iCalendarParsingBlock::Person::refasAttendee)
														psn_kind_id = PPPRK_EMPL;
													{
														S_GUID uuid;
														PPObjPerson::ResolverParam rslvp;
														rslvp.Flags = rslvp.fCreateIfNFound;
														rslvp.KindID = psn_kind_id;
														(temp_buf = p_ic_person->TxtUID).Transf(CTRANSF_UTF8_TO_INNER);
														if(temp_buf.NotEmpty() && uuid.FromStr(temp_buf)) {
															rslvp.Uuid = uuid;
														}
														(temp_buf = p_ic_person->TxtCN).Transf(CTRANSF_UTF8_TO_INNER);
														if(temp_buf.NotEmptyS())
															rslvp.CommonName = temp_buf;
														(temp_buf = p_ic_person->TxtINN).Transf(CTRANSF_UTF8_TO_INNER);
														if(temp_buf.NotEmptyS())
															rslvp.INN = temp_buf;
														(temp_buf = p_ic_person->TxtKPP).Transf(CTRANSF_UTF8_TO_INNER);
														if(temp_buf.NotEmptyS())
															rslvp.KPP = temp_buf;
														(temp_buf = p_ic_person->TxtPhone).Transf(CTRANSF_UTF8_TO_INNER);
														if(temp_buf.NotEmptyS())
															rslvp.Phone = temp_buf;
														(temp_buf = p_ic_person->TxtValue).Transf(CTRANSF_UTF8_TO_INNER);
														if(temp_buf.NotEmptyS()) {
															const char * p_mailto_prefix = "mailto:";
															if(temp_buf.HasPrefixIAscii(p_mailto_prefix)) {
																temp_buf.ShiftLeft(strlen(p_mailto_prefix));
																rslvp.EMail = temp_buf;
															}
														}
														{
															psn_candidate_list.Z();
															int rr = psn_obj.Resolve(rslvp, psn_candidate_list, 1);
															if(rr > 0) {
																assert(psn_candidate_list.getCount());
																if(psn_candidate_list.getCount()) {
																	PPID   resolved_id = psn_candidate_list.get(0);
																	if(p_ic_person->RefAs == iCalendarParsingBlock::Person::refasOrganizer) {
																		if(!p_curr_todo_pack->Rec.CreatorID) {
																			p_curr_todo_pack->Rec.CreatorID = resolved_id;
																		}
																	}
																	else if(p_ic_person->RefAs == iCalendarParsingBlock::Person::refasContact) {
																		if(!p_curr_todo_pack->Rec.ClientID) {
																			p_curr_todo_pack->Rec.ClientID = resolved_id;
																		}
																	}
																	else if(p_ic_person->RefAs == iCalendarParsingBlock::Person::refasAttendee) {
																		if(!p_curr_todo_pack->Rec.EmployerID) {
																			p_curr_todo_pack->Rec.EmployerID = resolved_id;
																		}
																	}
																}
															}
														}
													}
												}
											}
											if(pParam) {
												SETIFZ(p_curr_todo_pack->Rec.CreatorID, pParam->TodoDefCreatorID);
												SETIFZ(p_curr_todo_pack->Rec.EmployerID, pParam->TodoDefEmployerID);
												SETIFZ(p_curr_todo_pack->Rec.ClientID, pParam->TodoDefClientID);
											}
										}
										rList.insert(p_curr_todo_pack);
										p_curr_todo_pack = 0;
									}
									break;
								case VCalendar::tokVEVENT:
									THROW(current_ic_state == icstateVEVENT);
									current_ic_state = icstateUNDEF;
									break;
								default:
									THROW(current_ic_state == icstateUNKN);
									current_ic_state = icstateUNDEF;
									break;
							}
							break;
						case VCalendar::tokDTSTAMP: VCalendar::ParseDatetime(ic_entry.Value, pb.DtmDtStamp, 0); break;
						case VCalendar::tokCREATED: VCalendar::ParseDatetime(ic_entry.Value, pb.DtmCreated, 0); break;
						case VCalendar::tokLASTMODIFIED: VCalendar::ParseDatetime(ic_entry.Value, pb.DtmLastModified, 0); break;
						case VCalendar::tokDTSTART: VCalendar::ParseDatetime(ic_entry.Value, pb.DtmDtStart, 0); break;
						case VCalendar::tokDUE: VCalendar::ParseDatetime(ic_entry.Value, pb.DtmDue, 0); break;
						case VCalendar::tokDTEND: VCalendar::ParseDatetime(ic_entry.Value, pb.DtmDtEnd, 0); break;
						case VCalendar::tokCOMPLETED: VCalendar::ParseDatetime(ic_entry.Value, pb.DtmCompleted, 0); break;
						case VCalendar::tokPRIORITY: pb.Priority = ic_entry.Value.ToLong(); break;
						case VCalendar::tokORGANIZER:
							pb.CreatePersonEntry(iCalendarParsingBlock::Person::refasOrganizer, ic_entry);
							break;
						case VCalendar::tokCONTACT:
							pb.CreatePersonEntry(iCalendarParsingBlock::Person::refasContact, ic_entry);
							break;
						case VCalendar::tokATTENDEE:
							pb.CreatePersonEntry(iCalendarParsingBlock::Person::refasAttendee, ic_entry);
							break;
						case VCalendar::tokSUMMARY:
							pb.TxtSummary = ic_entry.Value;
							break;
						case VCalendar::tokDESCRIPTION:
							pb.TxtDescription = ic_entry.Value;
							break;
						case VCalendar::tokCOMMENT:
							pb.TxtComment = ic_entry.Value;
							break;
						case VCalendar::tokSTATUS:
							{
								int st = VCalendar::GetToken(ic_entry.Value);
								switch(st) {
									case VCalendar::tokNEEDSACTION: pb.Status = pb.statusNeedsAction; break;
									case VCalendar::tokCOMPLETED: pb.Status = pb.statusCompleted; break;
									case VCalendar::tokINPROCESS: pb.Status = pb.statusInProcess; break;
									case VCalendar::tokCANCELLED: pb.Status = pb.statusCancelled; break;
									default: pb.Status = pb.statusUnkn; break;
								}
							}
							break;
						case VCalendar::tokUID:
							pb.TxtUID = ic_entry.Value;
							break;
						case VCalendar::tokXPAPYRUSID:
							pb.XPapyrusID = ic_entry.Value.ToLong();
							break;
						case VCalendar::tokXPAPYRUSCODE:
							pb.XPapyrusCode = ic_entry.Value;
							break;
					}
				}
			}
			else if(!first_nonempty_line) // Неизвестный формат и была встречена одна непустая строка - уходим
				break;
		}
	}
	CATCHZOK
	return ok;
}

int PPObjPrjTask::SearchAnalog(const PPPrjTaskPacket * pPack, PPID * pAnalogID)
{
	int    ok = -1;
	PPID   id = 0;
	if(pPack && checkdate(pPack->Rec.Dt) && !isempty(pPack->Rec.Code)) {
		LDATETIME dtm_low;
		LDATETIME dtm_upp;
		dtm_low.Set(pPack->Rec.Dt, pPack->Rec.Tm);
		dtm_upp.Set(pPack->Rec.Dt, pPack->Rec.Tm);
		dtm_low.addsec(-30);
		dtm_upp.addsec(+30);
		PrjTaskTbl::Key1 k1;
		MEMSZERO(k1);
		k1.Dt = dtm_low.d;
		k1.Tm = dtm_low.t;
		if(P_Tbl->search(1, &k1, spGe)) do {
			LDATETIME dtm;
			dtm.Set(P_Tbl->data.Dt, P_Tbl->data.Tm);
			if(cmp(dtm, dtm_low) >= 0 && cmp(dtm, dtm_upp) <= 0) {
				if(sstreq(P_Tbl->data.Code, pPack->Rec.Code)) {
					id = P_Tbl->data.ID;
					ok = 1;
					break;
				}
			}
			else
				break;
		} while(P_Tbl->search(1, &k1, spNext));
	}
	ASSIGN_PTR(pAnalogID, id);
	return ok;
}

// Static
int PPObjPrjTask::ImportFromVCal()
{
	int    ok = -1;
	int    valid_data = 0;
	//VCalImportParamDlg::Param param;
	iCalendarImportParam param;
	VCalImportParamDlg * p_dlg = new VCalImportParamDlg;
	PPLogger logger;
	THROW(CheckDialogPtr(&p_dlg));
	p_dlg->setDTS(&param);
	while(!valid_data && ExecView(p_dlg) == cmOK) {
		if(p_dlg->getDTS(&param) > 0)
			valid_data = ok = 1;
		else
			PPError();
	}
	if(ok > 0) {
		PPObjPrjTask todo_obj;
		SFile f_in(param.FilePath, SFile::mRead|SFile::mBinary);
		if(f_in.IsValid()) {
			int64 fs = 0;
			if(f_in.CalcSize(&fs)) {
				STempBuffer in_data(static_cast<size_t>(fs+1024));
				size_t actual_size = 0;
				SString msg_buf;
				SString fmt_buf;
				SString temp_buf;
				SString todo_buf;
				PPWaitStart();
				PPWaitMsg(PPSTR_TEXT, PPTXT_IMPTODO);
				if(f_in.Read(in_data, static_cast<size_t>(fs), &actual_size)) {
					in_data[actual_size] = 0;
					TSCollection <PPPrjTaskPacket> pack_list;
					todo_obj.ImportFromOuterFormat(in_data, &param, pack_list);
					if(pack_list.getCount()) {
						PPTransaction tra(1);
						THROW(tra);
						for(uint i = 0; i < pack_list.getCount(); i++) {
							PPPrjTaskPacket * p_pack = pack_list.at(i);
							if(p_pack) {
								PPID   analog_id = 0;
								int sar = todo_obj.SearchAnalog(p_pack, &analog_id);
								todo_buf.Z().Cat(p_pack->Rec.Dt, DATF_DMY).Space().Cat(p_pack->Rec.Tm, TIMF_HMS).Space().Cat(p_pack->Rec.Code);
								if(p_pack->Rec.EmployerID) {
									GetPersonName(p_pack->Rec.EmployerID, temp_buf);
									todo_buf.Space().Cat(temp_buf);
								}
								if(sar > 0) {
									PPLoadText(PPTXT_IMPTODO_ANALOGFOUND, fmt_buf);
									logger.Log(msg_buf.Printf(fmt_buf.cptr(), todo_buf.cptr()));
								}
								else {
									PPID   new_id = 0;
									if(todo_obj.PutPacket(&new_id, p_pack, 0)) {
										PPLoadText(PPTXT_IMPTODO_CREATED, fmt_buf);
										logger.Log(msg_buf.Printf(fmt_buf.cptr(), todo_buf.cptr()));
									}
									else
										logger.LogLastError();
								}
							}
						}
						THROW(tra.Commit());
					}
				}
				PPWaitStop();
			}
		}
#if 0 // @v11.0.3 @deprecated {
		{
			VCalendar vcal;
			PPWaitStart();
			if(vcal.Open(param.FilePath, 0) > 0) {
				VCalendar::Todo vcal_rec;
				PPObjPerson  psn_obj;
				PersonTbl::Rec psn_rec;
				const  PPID cli_pk_id = PPPRK_CLIENT;
				PPIDArray cli_kind_list;
				cli_kind_list.add(cli_pk_id);
				{
					PPTransaction tra(1);
					THROW(tra);
					while(vcal.GetTodo(&vcal_rec) > 0) {
						if(vcal_rec.Descr.Len() > 0) {
							PPID   id = 0;
							PPPrjTaskPacket todo_pack;
							THROW(todo_obj.InitPacket(&todo_pack, TODOKIND_TASK, 0, 0, 0, 0));
							psn_obj.P_Tbl->SearchByName(vcal_rec.Owner, &todo_pack.Rec.EmployerID);
							todo_pack.Rec.EmployerID = NZOR(todo_pack.Rec.EmployerID, param.TodoDefEmployerID);
							todo_pack.SDescr = vcal_rec.Descr;

							todo_pack.Rec.Dt  = vcal_rec.CreatedDtm.d;
							todo_pack.Rec.Tm  = vcal_rec.CreatedDtm.t;
							todo_pack.Rec.StartDt     = vcal_rec.StartDtm.d;
							todo_pack.Rec.StartTm     = vcal_rec.StartDtm.t;
							todo_pack.Rec.FinishDt    = vcal_rec.CompletedDtm.d;
							todo_pack.Rec.FinishTm    = vcal_rec.CompletedDtm.t;
							todo_pack.Rec.EstFinishDt = vcal_rec.DueDtm.d;
							todo_pack.Rec.EstFinishTm = vcal_rec.DueDtm.t;
							todo_pack.Rec.OpenCount   = (int32)vcal_rec.Sequence;
							todo_pack.Rec.Priority    = vcal_rec.Priority;
							if(vcal_rec.Status == VCalendar::stAccepted)
								todo_pack.Rec.Status = TODOSTTS_NEW;
							else if(vcal_rec.Status == VCalendar::stDeclined)
								todo_pack.Rec.Status = TODOSTTS_REJECTED;
							else if(vcal_rec.Status == VCalendar::stConfirmed)
								todo_pack.Rec.Status = TODOSTTS_INPROGRESS;
							else if(vcal_rec.Status == VCalendar::stNeedsAction)
								todo_pack.Rec.Status = TODOSTTS_ONHOLD;
							else if(vcal_rec.Status == VCalendar::stCompleted)
								todo_pack.Rec.Status = TODOSTTS_COMPLETED;
							if(vcal_rec.Contact.NotEmptyS()) {
								if(psn_obj.SearchFirstByName(vcal_rec.Contact, &cli_kind_list, 0, &psn_rec) > 0) {
									todo_pack.Rec.ClientID = psn_rec.ID;
								}
							}
							SETIFZ(todo_pack.Rec.ClientID, param.TodoDefClientID);
							SETIFZ(todo_pack.Rec.CreatorID, param.TodoDefCreatorID);
							THROW(todo_obj.PutPacket(&id, &todo_pack, 0));
						}
					}
					THROW(tra.Commit());
				}
			}
			PPWaitStop();
		}
#endif // } 0 @v11.0.3 @deprecated
	}
	CATCHZOKPPERR
	delete p_dlg;
	return ok;
}

PPPrjTaskPacket::PPPrjTaskPacket()
{
}

PPPrjTaskPacket & PPPrjTaskPacket::Z()
{
	MEMSZERO(Rec);
	SDescr.Z();
	SMemo.Z();
	return *this;
}

TLP_IMPL(PPObjPrjTask, PrjTaskCore, P_Tbl);

PPObjPrjTask::PPObjPrjTask(void * extraPtr) : PPObject(PPOBJ_PRJTASK), ExtraPtr(extraPtr)
{
	TLP_OPEN(P_Tbl);
	ImplementFlags |= implStrAssocMakeList;
}

PPObjPrjTask::~PPObjPrjTask()
{
	TLP_CLOSE(P_Tbl);
}

/*virtual*/int PPObjPrjTask::Search(PPID id, void * pRec) { return P_Tbl->Search(id, static_cast<PrjTaskTbl::Rec *>(pRec)); }
/*virtual*/const char * PPObjPrjTask::GetNamePtr() { return P_Tbl->data.Code; }

StrAssocArray * PPObjPrjTask::MakeStrAssocList(void * extraPtr)
{
	PrjTaskTbl::Key0  k0;
	StrAssocArray * p_list = new StrAssocArray;
	SString temp_buf;
	DBQ  * dbq = 0;
	BExtQuery q(P_Tbl, 0);
	THROW_MEM(p_list);
	q.select(P_Tbl->ID, P_Tbl->Kind, P_Tbl->Code, 0L);
	if(extraPtr)
		dbq = & (*dbq && (P_Tbl->Kind == reinterpret_cast<long>(extraPtr)));
	q.where(*dbq);
	MEMSZERO(k0);
	for(q.initIteration(false, &k0, spGe); q.nextIteration() > 0;) {
		if(P_Tbl->data.Code[0])
			temp_buf = P_Tbl->data.Code;
		else
			ideqvalstr(P_Tbl->data.ID, temp_buf.Z());
		THROW_SL(p_list->Add(P_Tbl->data.ID, temp_buf));
	}
	CATCH
		ZDELETE(p_list);
	ENDCATCH
	return p_list;
}

int PPObjPrjTask::HandleMsg(int msg, PPID _obj, PPID _id, void * extraPtr)
{
	int    ok = DBRPL_OK;
	if(msg == DBMSG_OBJDELETE) {
		PPID   todo_id = 0;
		int    r = P_Tbl->SearchAnyRef(_obj, _id, &todo_id);
		ok = (r > 0) ? RetRefsExistsErr(Obj, todo_id) : (r ? DBRPL_OK : DBRPL_ERROR);
	}
	else if(msg == DBMSG_OBJREPLACE)
		if(!P_Tbl->ReplaceRefs(_obj, _id, reinterpret_cast<long>(extraPtr), 0))
			ok = DBRPL_ERROR;
	return ok;
}

int PPObjPrjTask::Read(PPObjPack * p, PPID id, void * stream, ObjTransmContext * pCtx)
{
	int    ok = 1;
	PPPrjTaskPacket * p_pack = new PPPrjTaskPacket;
	THROW_MEM(p_pack);
	p->Data = p_pack;
	if(stream == 0) {
		THROW(GetPacket(id, p_pack) > 0);
	}
	else {
		SBuffer buffer;
		THROW_SL(buffer.ReadFromFile(static_cast<FILE *>(stream), 0));
		THROW(SerializePacket(-1, p_pack, buffer, &pCtx->SCtx));
	}
	CATCHZOK
	return ok;
}

int PPObjPrjTask::Write(PPObjPack * p, PPID * pID, void * stream, ObjTransmContext * pCtx) // @srlz
{
	int    ok = 1;
	PPPrjTaskPacket * p_pack = 0;
	THROW(p && p->Data);
	p_pack = static_cast<PPPrjTaskPacket *>(p->Data);
	if(stream == 0) {
		p_pack->Rec.ID = *pID;
		if(!PutPacket(pID, p_pack, 1)) {
			pCtx->OutputAcceptErrMsg(PPTXT_ERRACCEPTPRJTASK, p_pack->Rec.ID, p_pack->Rec.Code);
			ok = -1;
		}
	}
	else {
		SBuffer buffer;
		THROW(SerializePacket(+1, p_pack, buffer, &pCtx->SCtx));
		THROW_SL(buffer.WriteToFile(static_cast<FILE *>(stream), 0, 0));
	}
	CATCHZOK
	return ok;
}

int PPObjPrjTask::EditRights(uint bufSize, ObjRights * rt, EmbedDialog * pDlg)
{
	return EditSpcRightFlags(DLG_RTPRJTASK, CTL_RTPRJTASK_FLAGS, CTL_RTPRJTASK_SFLAGS, bufSize, rt, pDlg);
}

int PPObjPrjTask::ProcessObjRefs(PPObjPack * p, PPObjIDArray * ary, int replace, ObjTransmContext * pCtx)
{
	int    ok = 1;
	THROW(p && p->Data);
	{
		PPPrjTaskPacket * p_pack = static_cast<PPPrjTaskPacket *>(p->Data);
		THROW(ProcessObjRefInArray(PPOBJ_PROJECT,  &p_pack->Rec.ProjectID,   ary, replace));
		THROW(ProcessObjRefInArray(PPOBJ_PERSON,   &p_pack->Rec.CreatorID,   ary, replace));
		THROW(ProcessObjRefInArray(PPOBJ_PERSON,   &p_pack->Rec.EmployerID,  ary, replace));
		THROW(ProcessObjRefInArray(PPOBJ_PERSON,   &p_pack->Rec.ClientID,    ary, replace));
		THROW(ProcessObjRefInArray(PPOBJ_PRJTASK,  &p_pack->Rec.TemplateID,  ary, replace));
		THROW(ProcessObjRefInArray(PPOBJ_PRJTASK,  &p_pack->Rec.LinkTaskID,  ary, replace));
		THROW(ProcessObjRefInArray(PPOBJ_LOCATION, &p_pack->Rec.DlvrAddrID,  ary, replace));
		THROW(ProcessObjRefInArray(PPOBJ_ARTICLE,  &p_pack->Rec.BillArID,    ary, replace));
	}
	CATCHZOK
	return ok;
}

int PPObjPrjTask::DetermineNewStatus(const PPProjectConfig * pCfg, const PrjTaskTbl::Rec * pRec, int * pNewStatus)
{
	int    new_status = 0;
	LDATE  cur = getcurdate_();
	const  int st = pRec->Status;
	if(pCfg->NewTaskTerm > 0 && st == TODOSTTS_NEW)
		if(diffdate(cur, pRec->Dt) >= pCfg->NewTaskTerm)
			new_status = TODOSTTS_ONHOLD;
	if(pCfg->RejTaskTerm > 0 && st != TODOSTTS_REJECTED &&
		oneof3(st, TODOSTTS_NEW, TODOSTTS_ONHOLD, TODOSTTS_INPROGRESS)) {
		LDATE  d = NZOR(pRec->StartDt, pRec->Dt);
		if(d && diffdate(cur, d) >= pCfg->RejTaskTerm)
			new_status = TODOSTTS_REJECTED;
	}
	ASSIGN_PTR(pNewStatus, new_status);
	return new_status ? 1 : -1;
}

int PPObjPrjTask::Maintain()
{
	int    ok = 1;
	long   look_count = 0, upd_count = 0;
	PPProjectConfig prj_cfg;
	if(PPObjProject::ReadConfig(&prj_cfg) > 0) {
		const LDATE  cur = getcurdate_();
		IterCounter cntr;
		SString wait_msg;
		PPTransaction tra(1);
		THROW(tra);
		if((prj_cfg.NewTaskTerm || prj_cfg.RejTaskTerm) && CheckRights(PPR_MOD)) {
			PPLoadText(PPTXT_WAIT_CHNGTODOSTATUS, wait_msg);
			BExtQuery q(P_Tbl, 0);
			q.select(P_Tbl->ID, P_Tbl->Status, P_Tbl->Dt, P_Tbl->StartDt, P_Tbl->EstFinishDt, P_Tbl->FinishDt, 0L);
			q.where(P_Tbl->Kind == static_cast<long>(TODOKIND_TASK));
			PrjTaskTbl::Key0 k0, k0_;
			MEMSZERO(k0);
			k0_ = k0;
			cntr.Init(q.countIterations(0, &k0_, spFirst));
			for(q.initIteration(false, &k0, spFirst); q.nextIteration() > 0;) {
				PrjTaskTbl::Rec rec;
				P_Tbl->CopyBufTo(&rec);
				int    new_status = 0;
				look_count++;
				if(DetermineNewStatus(&prj_cfg, &rec, &new_status) > 0) {
					int r = 0;
					THROW(r = P_Tbl->UpdateStatus(rec.ID, new_status, 0));
					if(r > 0) {
						DS.LogAction(PPACN_OBJUPD, Obj, rec.ID, 0, 0);
						upd_count++;
					}
				}
				PPWaitPercent(cntr.Increment(), wait_msg);
			}
		}
		if(prj_cfg.TemplGenTerm > 0 && CheckRights(PPR_INS)) {
			PPLoadText(PPTXT_WAIT_GENTEMPLTASK, wait_msg);
			PPIDArray new_task_list;
			BExtQuery q(P_Tbl, 0);
			q.select(P_Tbl->ID, P_Tbl->Status, 0L);
			q.where(P_Tbl->Kind == static_cast<long>(TODOKIND_TEMPLATE));
			PrjTaskTbl::Key0 k0, k0_;
			MEMSZERO(k0);
			k0_ = k0;
			cntr.Init(q.countIterations(0, &k0_, spFirst));
			for(q.initIteration(false, &k0, spFirst); q.nextIteration() > 0;) {
				PrjTaskTbl::Rec rec;
				P_Tbl->CopyBufTo(&rec);
				if(rec.Status != TODOSTTS_REJECTED) {
					DateRange period;
					period.Set(cur, plusdate(cur, prj_cfg.TemplGenTerm-1));
					THROW(CreateByTemplate(rec.ID, &period, &new_task_list, 0));
				}
				PPWaitPercent(cntr.Increment(), wait_msg);
			}
		}
		DS.LogAction(PPACN_MAINTAINPRJTASK, Obj, 0, upd_count, 0);
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int MaintainPrjTask()
{
	int    ok = -1;
	if(CONFIRM(PPCFM_MAINTAINPRJTASK)) {
		PPWaitStart();
		PPObjPrjTask todo_obj;
		if(todo_obj.Maintain())
			ok = 1;
		else
			ok = PPErrorZ();
		PPWaitStop();
	}
	return ok;
}

int PPObjPrjTask::SerializePacket(int dir, PPPrjTaskPacket * pPack, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	THROW_SL(P_Tbl->SerializeRecord(dir, &pPack->Rec, rBuf, pSCtx));
	THROW_SL(pSCtx->Serialize(dir, pPack->SDescr, rBuf));
	THROW_SL(pSCtx->Serialize(dir, pPack->SMemo, rBuf));
	CATCHZOK
	return ok;
}

int PPObjPerson::Helper_WritePersonInfoInICalendarFormat(PPID personID, int icalToken, const char * pRole, SString & rBuf)
{
	int    ok = -1;
	PPPersonPacket pack;
	if(personID && GetPacket(personID, &pack, 0) > 0) {
		SString temp_buf;
		SNaturalTokenArray nta;
		STokenRecognizer tr;
		VCalendar::WriteToken(icalToken, rBuf);
		if(!isempty(pRole)) {
			rBuf.Semicol().CatEq("ROLE", pRole);
		}
		(temp_buf = pack.Rec.Name).Transf(CTRANSF_INNER_TO_UTF8);
		VCalendar::PreprocessText(temp_buf);
		rBuf.Semicol().CatEq("CN", temp_buf.Quot('\"', '\"'));
		if(GetLinguaCode(slangRU, temp_buf)) {
			rBuf.Semicol().CatEq("LANGUAGE", temp_buf);
		}
		if(pack.Regs.GetRegNumber(PPREGT_TPID, getcurdate_(), temp_buf)) {
			temp_buf.Strip();
			tr.Run(temp_buf.ucptr(), temp_buf.Len(), nta.Z(), 0);
			if(nta.Has(SNTOK_RU_INN))
				rBuf.Semicol().CatEq("X-RU-INN", temp_buf);
		}
		if(pack.Regs.GetRegNumber(PPREGT_KPP, getcurdate_(), temp_buf)) {
			temp_buf.Strip();
			tr.Run(temp_buf.ucptr(), temp_buf.Len(), nta.Z(), 0);
			if(nta.Has(SNTOK_RU_KPP))
				rBuf.Semicol().CatEq("X-RU-KPP", temp_buf);
		}

		rBuf.Semicol().CatEq("X-PAPYRUS-ID", temp_buf.Z().Cat(pack.Rec.ID));
		{
			S_GUID uuid;
			const ObjTagItem * p_tag_item = pack.TagL.GetItem(PPTAG_PERSON_UUID);
			if(p_tag_item && p_tag_item->GetGuid(&uuid)) {
				temp_buf.Z().Cat(uuid, S_GUID::fmtIDL);
				rBuf.Semicol().CatEq("UID", temp_buf);
			}
		}
		{
			bool is_there_email = false;
			bool is_there_phone = false;
			StringSet ss_ela;
			if(pack.ELA.GetListByType(ELNKRT_PHONE, ss_ela) > 0) {
				for(uint ssp = 0; !is_there_phone && ss_ela.get(&ssp, temp_buf);) {
					tr.Run(temp_buf.Strip().ucptr(), -1, nta.Z(), 0);
					if(nta.Has(SNTOK_PHONE)) {
						VCalendar::PreprocessText(temp_buf);
						rBuf.Semicol().CatEq("X-PHONE", temp_buf);
						is_there_phone = true;
					}
				}
			}
			ss_ela.Z();
			if(pack.ELA.GetListByType(ELNKRT_EMAIL, ss_ela) > 0) {
				for(uint ssp = 0; !is_there_email && ss_ela.get(&ssp, temp_buf);) {
					tr.Run(temp_buf.Strip().ucptr(), -1, nta.Z(), 0);
					if(nta.Has(SNTOK_EMAIL)) {
						VCalendar::PreprocessText(temp_buf);
						rBuf.Colon().Cat("mailto").Colon().Cat(temp_buf);
						is_there_email = true;
					}
				}
			}
			if(!is_there_email)
				rBuf.Colon().Cat("none");
		}
		rBuf.CRB();
		ok = 1;
	}
	return ok;
}

int PPObjPrjTask::WritePacketWithPredefinedFormat(const PPPrjTaskPacket * pPack, int format, SString & rBuf, void * pCtx)
{
	int    ok = -1;
	Reference * p_ref = PPRef;
	if(pPack) {
		if(format == piefICalendar) {
			SysJournal * p_sj = DS.GetTLA().P_SysJ;
			SString temp_buf;
			LDATETIME dtm;
			LDATETIME sj_cr_dtm;
			LDATETIME sj_mod_dtm;	
			PPObjPerson psn_obj;
			PPObjLocation loc_obj;
			S_GUID uuid;
			StringSet ss_ela; // Список электронных адресов
			//
			{
				VCalendar::WriteToken(VCalendar::tokUID, rBuf);
				rBuf.Colon();
				rBuf.Cat(pPack->Rec.ID);
				rBuf.CRB();
			}
			sj_cr_dtm.Z();
			sj_mod_dtm.Z();
			if(p_sj) {
				int is_creation_event = 0;
				if(p_sj->GetLastObjModifEvent(Obj, pPack->Rec.ID, &sj_mod_dtm, &is_creation_event, 0) > 0) {
					if(is_creation_event)
						sj_cr_dtm = sj_mod_dtm;	
					else {
						SysJournalTbl::Rec sj_rec;
						if(p_sj->GetObjCreationEvent(Obj, pPack->Rec.ID, &sj_rec) > 0)
							sj_cr_dtm.Set(sj_rec.Dt, sj_rec.Tm);
					}
				}
			}
			//
			if(!checkdate(sj_cr_dtm.d)) {
				if(checkdate(pPack->Rec.Dt))
					sj_cr_dtm.Set(pPack->Rec.Dt, pPack->Rec.Tm);
				else
					sj_cr_dtm = getcurdatetime_();
			}
			assert(checkdate(sj_cr_dtm.d));
			{
				VCalendar::WriteToken(VCalendar::tokDTSTAMP, rBuf);
				rBuf.Colon();
				VCalendar::WriteDatetime(sj_cr_dtm, rBuf);
				rBuf.CRB();
			}
			if(checkdate(sj_mod_dtm.d) && cmp(sj_mod_dtm, sj_cr_dtm) > 0) {
				VCalendar::WriteToken(VCalendar::tokLASTMODIFIED, rBuf);
				rBuf.Colon();
				VCalendar::WriteDatetime(sj_mod_dtm, rBuf);
				rBuf.CRB();
			}
			VCalendar::WriteToken(VCalendar::tokCREATED, rBuf);
			rBuf.Colon();
			VCalendar::WriteDatetime(dtm.Set(pPack->Rec.Dt, pPack->Rec.Tm), rBuf);
			rBuf.CRB();
			if(checkdate(pPack->Rec.FinishDt)) {
				VCalendar::WriteToken(VCalendar::tokCOMPLETED, rBuf);
				rBuf.Colon();
				VCalendar::WriteDatetime(dtm.Set(pPack->Rec.FinishDt, pPack->Rec.FinishTm), rBuf);
				rBuf.CRB();
			}
			if(checkdate(pPack->Rec.StartDt)) {
				VCalendar::WriteToken(VCalendar::tokDTSTART, rBuf);
				rBuf.Colon();
				VCalendar::WriteDatetime(dtm.Set(pPack->Rec.StartDt, pPack->Rec.StartTm), rBuf);
				rBuf.CRB();
			}
			if(checkdate(pPack->Rec.EstFinishDt)) {
				VCalendar::WriteToken(VCalendar::tokDUE, rBuf);
				rBuf.Colon();
				VCalendar::WriteDatetime(dtm.Set(pPack->Rec.EstFinishDt, pPack->Rec.EstFinishTm), rBuf);
				rBuf.CRB();
			}
			{
				int tok_status = 0;
				switch(pPack->Rec.Status) {
					case TODOSTTS_NEW: tok_status = VCalendar::tokNEEDSACTION; break;
					case TODOSTTS_REJECTED: tok_status = VCalendar::tokCANCELLED; break;
					case TODOSTTS_INPROGRESS: tok_status = VCalendar::tokINPROCESS; break;
					case TODOSTTS_ONHOLD: tok_status = VCalendar::tokNEEDSACTION; break;
					case TODOSTTS_COMPLETED: tok_status = VCalendar::tokCOMPLETED; break;
				}
				if(tok_status) {
					VCalendar::WriteToken(VCalendar::tokSTATUS, rBuf);
					rBuf.Colon();
					VCalendar::WriteToken(tok_status, rBuf);
					rBuf.CRB();
				}
				{
					VCalendar::WriteToken(VCalendar::tokPRIORITY, rBuf); // [0..9]
					rBuf.Colon();
					// В Papyrus'е штатный диапазон приоритетов задач [1..5] - транслируем значение в диапазон [0..9]
					int ical_priority = pPack->Rec.Priority;
					if(ical_priority >= 1 && ical_priority <= 5)
						ical_priority = (ical_priority * 2) - 1;
					else
						ical_priority = 9;
					rBuf.Cat(ical_priority);
					rBuf.CRB();
				}
				psn_obj.Helper_WritePersonInfoInICalendarFormat(pPack->Rec.CreatorID, VCalendar::tokORGANIZER, 0, rBuf);
				VCalendar::WriteToken(VCalendar::tokREQPARTICIPANT, temp_buf);
				psn_obj.Helper_WritePersonInfoInICalendarFormat(pPack->Rec.EmployerID,  VCalendar::tokATTENDEE, temp_buf, rBuf);
				psn_obj.Helper_WritePersonInfoInICalendarFormat(pPack->Rec.ClientID,  VCalendar::tokCONTACT, 0, rBuf);
				if(pPack->Rec.DlvrAddrID) {
					PPLocationPacket loc_pack;
					if(loc_obj.GetPacket(pPack->Rec.DlvrAddrID, &loc_pack) > 0) {
						VCalendar::WriteToken(VCalendar::tokLOCATION, rBuf);
						if(GetLinguaCode(slangRU, temp_buf)) {
							rBuf.Semicol().CatEq("LANGUAGE", temp_buf);
						}
						rBuf.Semicol().CatEq("X-PAPYRUS-ID", temp_buf.Z().Cat(loc_pack.ID));
						{
							S_GUID uuid;
							const ObjTagItem * p_tag_item = loc_pack.TagL.GetItem(PPTAG_LOC_UUID);
							if(p_tag_item && p_tag_item->GetGuid(&uuid)) {
								temp_buf.Z().Cat(uuid, S_GUID::fmtIDL);
								rBuf.Semicol().CatEq("UID", temp_buf);
							}
						}
						LocationCore::GetAddress(loc_pack, 0, temp_buf);
						temp_buf.Transf(CTRANSF_INNER_TO_UTF8);
						VCalendar::PreprocessText(temp_buf);
						rBuf.Colon().Cat(temp_buf);
						rBuf.CRB();
						if(loc_pack.Latitude != 0.0 && loc_pack.Longitude != 0.0) {
							VCalendar::WriteToken(VCalendar::tokGEO, rBuf);
							rBuf.Colon().Cat(loc_pack.Latitude, MKSFMTD(0, 8, NMBF_NOTRAILZ)).Semicol().Cat(loc_pack.Longitude, MKSFMTD(0, 8, NMBF_NOTRAILZ));
							rBuf.CRB();
						}
					}
				}
				if(pPack->SDescr.NotEmpty()) {
					VCalendar::WriteToken(VCalendar::tokDESCRIPTION, rBuf);
					(temp_buf = pPack->SDescr).Transf(CTRANSF_INNER_TO_UTF8);
					VCalendar::PreprocessText(temp_buf);
					rBuf.Colon().Cat(temp_buf);
					rBuf.CRB();
				}
				if(pPack->SMemo.NotEmpty()) {
					VCalendar::WriteToken(VCalendar::tokCOMMENT, rBuf);
					(temp_buf = pPack->SMemo).Transf(CTRANSF_INNER_TO_UTF8);
					VCalendar::PreprocessText(temp_buf);
					rBuf.Colon().Cat(temp_buf);
					rBuf.CRB();
				}
				VCalendar::WriteToken(VCalendar::tokXPAPYRUSID, rBuf);
				rBuf.Colon().Cat(pPack->Rec.ID);
				rBuf.CRB();
				if(!isempty(pPack->Rec.Code)) {
					VCalendar::WriteToken(VCalendar::tokXPAPYRUSCODE, rBuf);
					(temp_buf = pPack->Rec.Code).Transf(CTRANSF_INNER_TO_UTF8);
					VCalendar::PreprocessText(temp_buf);
					rBuf.Colon().Cat(temp_buf);
					rBuf.CRB();
				}
			}
		}
	}
	return ok;
}

int PPObjPrjTask::InitPacket(PPPrjTaskPacket * pPack, int kind, PPID prjID, PPID clientID, PPID employerID, int use_ta)
{
	PPProjectConfig cfg;
	PPObjProject::ReadConfig(&cfg);
	PPObjOpCounter opc_obj;
	long   counter = 0;
	PPID   cntr_id = 0;
	pPack->Z();
	pPack->Rec.Kind = NZOR(kind, TODOKIND_TASK);
	if(pPack->Rec.Kind == TODOKIND_TASK)
		cntr_id = cfg.TaskCntrID;
	else if(pPack->Rec.Kind == TODOKIND_TEMPLATE)
		cntr_id = cfg.TemplCntrID;
	if(cntr_id)
		opc_obj.GetCode(cntr_id, &counter, pPack->Rec.Code, sizeof(pPack->Rec.Code), 0, use_ta);
	pPack->Rec.ProjectID  = prjID;
	pPack->Rec.ClientID   = clientID;
	pPack->Rec.EmployerID = employerID;
	pPack->Rec.Priority   = TODOPRIOR_NORMAL;
	pPack->Rec.Status     = TODOSTTS_NEW;
	pPack->Rec.LinkTaskID = LinkTaskID;
	PPObjPerson::GetCurUserPerson(&pPack->Rec.CreatorID, 0);
	getcurdatetime(&pPack->Rec.Dt, &pPack->Rec.Tm);
	return 1;
}

int PPObjPrjTask::PutPacket(PPID * pID, PPPrjTaskPacket * pPack, int use_ta)
{
	int    ok = 1;
	Reference * p_ref = PPRef;
	PPID   acn_id = 0;
	SString ext_buffer;
	/*// Участок кода на переходный период пока не будет сконвертирована таблица PrjTaskTbl {
	if(pPack) {
		if(pPack->SDescr.Empty() && !isempty(pPack->Rec.Descr_))
			pPack->SDescr = pPack->Rec.Descr_;
		if(pPack->SMemo.Empty() && !isempty(pPack->Rec.Memo_))
			pPack->SMemo = pPack->Rec.Memo_;
	}
	// }*/
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(*pID) {
			if(pPack) {
				const int acn_viewed = BIN(pPack->Rec.Flags & TODOF_ACTIONVIEWED);
				pPack->Rec.Flags &= ~TODOF_ACTIONVIEWED;
				THROW(P_Tbl->Update(*pID, &pPack->Rec, 0));
				acn_id = acn_viewed ? PPACN_OBJVIEWED : PPACN_OBJUPD;
			}
			else {
				THROW(P_Tbl->Remove(*pID, 0));
				acn_id = PPACN_OBJRMV;
			}
		}
		else if(pPack) {
			THROW(P_Tbl->Add(pID, &pPack->Rec, 0));
			acn_id = PPACN_OBJADD;
		}
		{
			if(pPack)
				(ext_buffer = pPack->SDescr).Strip();
			else
				ext_buffer.Z();
			THROW(p_ref->UtrC.SetText(TextRefIdent(Obj, *pID, PPTRPROP_DESCR), ext_buffer.Transf(CTRANSF_INNER_TO_UTF8), 0));
			if(pPack)
				(ext_buffer = pPack->SMemo).Strip();
			else
				ext_buffer.Z();
			THROW(p_ref->UtrC.SetText(TextRefIdent(Obj, *pID, PPTRPROP_MEMO), ext_buffer.Transf(CTRANSF_INNER_TO_UTF8), 0));
		}
		DS.LogAction(acn_id, Obj, *pID, 0, 0);
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

SString & PPObjPrjTask::GetItemDescr(PPID id, SString & rBuf)
{
	rBuf.Z();
	PPRef->UtrC.GetText(TextRefIdent(Obj, id, PPTRPROP_DESCR), rBuf);
	rBuf.Transf(CTRANSF_UTF8_TO_INNER);
	return rBuf;
}

SString & PPObjPrjTask::GetItemMemo(PPID id, SString & rBuf)
{
	rBuf.Z();
	PPRef->UtrC.GetText(TextRefIdent(Obj, id, PPTRPROP_MEMO), rBuf);
	rBuf.Transf(CTRANSF_UTF8_TO_INNER);
	return rBuf;
}

int PPObjPrjTask::GetPacket(PPID id, PPPrjTaskPacket * pPack)
{
	int    ok = -1;
	if(pPack) {
		Reference * p_ref = PPRef;
		pPack->Z();
		if(Search(id, &pPack->Rec) > 0) {
			GetItemDescr(id, pPack->SDescr);
			GetItemMemo(id, pPack->SMemo);
			ok = 1;
		}
	}
	else
		ok = Search(id, 0);
	return ok;
}

int PPObjPrjTask::SubstDescr(PPPrjTaskPacket * pPack)
{
	int    ok = 1;
	char   buf[2048];
	char * b = buf;
	SString temp_buf;
	size_t len = 0;
	PPObjPerson * p_psn_obj = 0;
	PersonTbl::Rec psn_rec;
	PPSymbTranslator st;
	for(const char * p = pPack->SDescr; *p;) {
		if(*p == '@') {
			size_t next = 1;
			long   sym  = st.Translate(p, &next);
			switch(sym) {
				case PPSYM_CLIENT:
					THROW_MEM(SETIFZ(p_psn_obj, new PPObjPerson));
					if(p_psn_obj->Search(pPack->Rec.ClientID, &psn_rec) > 0)
						strnzcpy(b, psn_rec.Name, sizeof(buf)-len);
					else
						*b = 0;
					break;
				case PPSYM_CLIENTADDR:
					if(pPack->Rec.ClientID) {
						THROW_MEM(SETIFZ(p_psn_obj, new PPObjPerson));
						if(pPack->Rec.DlvrAddrID)
							p_psn_obj->LocObj.GetAddress(pPack->Rec.DlvrAddrID, 0, temp_buf);
						else
							p_psn_obj->GetAddress(pPack->Rec.ClientID, temp_buf);
						temp_buf.CopyTo(b, sizeof(buf)-len);
					}
					else
						*b = 0;
					break;
				default:
					*b = 0;
					break;
			}
			len += sstrlen(b);
			b += sstrlen(b);
			p += next;
		}
		else {
			*b++ = *p++;
			len++;
		}
	}
	*b = 0;
	pPack->SDescr = buf;
	CATCHZOK
	delete p_psn_obj;
	return ok;
}

int PPObjPrjTask::InitPacketByTemplate(const PPPrjTaskPacket * pTemplPack, LDATE startDt, PPPrjTaskPacket * pPack, int use_ta)
{
	int    ok = 1;
	THROW_INVARG(pTemplPack && pPack && startDt != ZERODATE);
	THROW(InitPacket(pPack, TODOKIND_TASK, pTemplPack->Rec.ProjectID, pTemplPack->Rec.ClientID, pTemplPack->Rec.EmployerID, use_ta));
	getcurdatetime(&pPack->Rec.Dt, &pPack->Rec.Tm);
	pPack->Rec.TemplateID = pTemplPack->Rec.ID;
	pPack->Rec.DlvrAddrID = pTemplPack->Rec.DlvrAddrID;
	pPack->Rec.ProjectID  = pTemplPack->Rec.ProjectID;
	pPack->Rec.BillArID   = pTemplPack->Rec.BillArID;
	pPack->Rec.StartDt    = startDt;
	pPack->Rec.Priority   = pTemplPack->Rec.Priority;
	pPack->SDescr = pTemplPack->SDescr;
	THROW(SubstDescr(pPack));
	CATCHZOK
	return ok;
}

int PPObjPrjTask::CreateByTemplate(PPID templID, const DateRange * pPeriod, PPIDArray * pIdList, int use_ta)
{
	int    ok = -1;
	PPPrjTaskPacket templ_pack;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(GetPacket(templID, &templ_pack) > 0 && templ_pack.Rec.Kind == TODOKIND_TEMPLATE &&
			templ_pack.Rec.Status != TODOSTTS_REJECTED && templ_pack.Rec.DrPrd && templ_pack.Rec.DrPrd != PRD_REPEATAFTERPRD) {
			DateRange period;
			if(pPeriod) {
				period = *pPeriod;
				SETIFZ(period.upp, getcurdate_());
			}
			else
				period.Set(ZERODATE, getcurdate_());
			const DateRepeating rept = *reinterpret_cast<const DateRepeating *>(&templ_pack.Rec.DrPrd);
			DateRepIterator dr_iter(rept, templ_pack.Rec.Dt, period.upp);
			for(LDATE dt = dr_iter.Next(); dt; dt = dr_iter.Next()) {
				if(period.CheckDate(dt)) {
					const int r = P_Tbl->SearchByTemplate(templID, dt, 0);
					PPID   id = 0;
					THROW(r);
					if(r < 0) {
						PPPrjTaskPacket pack;
						THROW(InitPacketByTemplate(&templ_pack, dt, &pack, 0));
						THROW(PutPacket(&id, &pack, 0));
						CALLPTRMEMB(pIdList, add(id));
						ok = 1;
					}
				}
			}
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}
//
//
//
class PrjTaskDialog : public TDialog {
	DECL_DIALOG_DATA(PPPrjTaskPacket);
public:
	explicit PrjTaskDialog(uint dlgID) : TDialog(/*DLG_TODO*/dlgID)
	{
		SetupCalDate(CTLCAL_TODO_DT, CTL_TODO_DT);
		SetupCalDate(CTLCAL_TODO_START, CTL_TODO_START);
		SetupCalDate(CTLCAL_TODO_ESTFINISH, CTL_TODO_ESTFINISH);
		SetupCalDate(CTLCAL_TODO_FINISH, CTL_TODO_FINISH);
		SetupInputLine(CTL_TODO_DESCR, MKSTYPE(S_ZSTRING, 2048), MKSFMT(2048, 0));
		SetupInputLine(CTL_TODO_MEMO, MKSTYPE(S_ZSTRING, 2048), MKSFMT(2048, 0));
		SetupTimePicker(this, CTL_TODO_STARTTM, CTLTM_TODO_STARTTM);
		SetupTimePicker(this, CTL_TODO_ESTFINISHTM, CTLTM_TODO_ESTFINISHTM);
		SetupTimePicker(this, CTL_TODO_FINISHTM, CTLTM_TODO_FINISHTM);
	}
	DECL_DIALOG_SETDTS()
	{
		RVALUEPTR(Data, pData);
		SString buf;
		PPID   prj_id = 0;
		PPID   phase_id = 0;
		if(Data.Rec.ProjectID) {
			PPObjProject prj_obj;
			ProjectTbl::Rec prj_rec;
			if(prj_obj.Search(Data.Rec.ProjectID, &prj_rec) > 0) {
				if(prj_rec.ParentID) {
					phase_id = prj_rec.ID;
					prj_id = prj_rec.ParentID;
				}
				else
					prj_id = prj_rec.ID;
				//prj_obj.GetFullName(Data.ProjectID, buf);
				//setStaticText(CTL_TODO_PARENTNAME, buf);
			}
		}
		setCtrlLong(CTL_TODO_ID, Data.Rec.ID);
		SetupPPObjCombo(this, CTLSEL_TODO_PRJ,   PPOBJ_PROJECT, prj_id,   OLW_CANINSERT, 0);
		SetupPPObjCombo(this, CTLSEL_TODO_PHASE, PPOBJ_PROJECT, phase_id, OLW_CANINSERT, reinterpret_cast<void *>(NZOR(prj_id, -1)));
		setCtrlData(CTL_TODO_CODE,        Data.Rec.Code);
		if(Data.Rec.Kind == TODOKIND_TASK && Data.Rec.LinkTaskID) {
			PrjTaskTbl::Rec  link_rec;
			if(PTCore.Search(Data.Rec.LinkTaskID, &link_rec) > 0)
				setCtrlData(CTL_TODO_LINKTASK, link_rec.Code);
		}
		setCtrlData(CTL_TODO_DT,          &Data.Rec.Dt);
		setCtrlData(CTL_TODO_TM,          &Data.Rec.Tm);
		setCtrlData(CTL_TODO_START,       &Data.Rec.StartDt);
		setCtrlData(CTL_TODO_STARTTM,     &Data.Rec.StartTm);
		setCtrlData(CTL_TODO_ESTFINISH,   &Data.Rec.EstFinishDt);
		setCtrlData(CTL_TODO_ESTFINISHTM, &Data.Rec.EstFinishTm);
		setCtrlData(CTL_TODO_FINISH,      &Data.Rec.FinishDt);
		setCtrlData(CTL_TODO_FINISHTM,    &Data.Rec.FinishTm);
		SetupPPObjCombo(this, CTLSEL_TODO_TEMPLATE, PPOBJ_PRJTASK, Data.Rec.TemplateID, 0, reinterpret_cast<void *>(TODOKIND_TEMPLATE));
		SetupPersonCombo(this, CTLSEL_TODO_CREATOR,  Data.Rec.CreatorID,  OLW_CANINSERT, PPPRK_EMPL,   0);
		SetupPersonCombo(this, CTLSEL_TODO_EMPLOYER, Data.Rec.EmployerID, OLW_CANINSERT, PPPRK_EMPL,   0);
		SetupPersonCombo(this, CTLSEL_TODO_CLIENT,   Data.Rec.ClientID,   OLW_CANINSERT, PPPRK_CLIENT, 0);
		PsnObj.SetupDlvrLocCombo(this, CTLSEL_TODO_DLVRADDR, Data.Rec.ClientID, Data.Rec.DlvrAddrID);
		SetupStringCombo(this, CTLSEL_TODO_PRIOR,  PPTXT_TODO_PRIOR,  Data.Rec.Priority);
		int    str_id = 0;
		if(Data.Rec.Kind == TODOKIND_TEMPLATE)
			str_id = PPTXT_TODOTEMPL_STATUS;
		else {
			str_id = PPTXT_TODO_STATUS;
			selectCtrl(CTL_TODO_START);
		}
		SetupStringCombo(this, CTLSEL_TODO_STATUS, str_id, Data.Rec.Status);
		SetupWordSelector(CTL_TODO_DESCR, new TextHistorySelExtra("todo-descr-common"), 0, 2, WordSel_ExtraBlock::fFreeText);
		SetupWordSelector(CTL_TODO_MEMO, new TextHistorySelExtra("todo-memo-common"), 0, 2, WordSel_ExtraBlock::fFreeText);
		setCtrlString(CTL_TODO_DESCR,   Data.SDescr);
		setCtrlString(CTL_TODO_MEMO,    Data.SMemo);
		setCtrlData(CTL_TODO_AMOUNT, &Data.Rec.Amount);
		{
			PPID   accsheet = 0;
			PPID   billop = 0;
			ProjectTbl::Rec prj_rec;
			for(billop = 0, prj_id = Data.Rec.ProjectID; !billop && PrjObj.Search(prj_id, &prj_rec) > 0; prj_id = prj_rec.ParentID)
				billop = prj_rec.BillOpID;
			if(!billop) {
				PPProjectConfig prj_cfg;
				PPObjProject::ReadConfig(&prj_cfg);
				billop = prj_cfg.BillOpID;
			}
			if(billop) {
				PPOprKind opr_kind;
				GetOpData(billop, &opr_kind);
				accsheet = opr_kind.AccSheetID;
			}
			Data.Rec.BillArID = accsheet ? Data.Rec.BillArID : 0;
			SetupArCombo(this, CTLSEL_TODO_BILLAR, Data.Rec.BillArID, OLW_LOADDEFONOPEN|OLW_CANINSERT, accsheet, sacfDisableIfZeroSheet);
		}
		return 1;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		uint   sel = 0;
		char   linktask_code[32];
		PPID   temp_id = 0;
		PPID   prj_id = getCtrlLong(CTLSEL_TODO_PRJ);
		PPID   phase_id = getCtrlLong(CTLSEL_TODO_PHASE);
		Data.Rec.ProjectID = (phase_id && prj_id) ? phase_id : prj_id;
		getCtrlData(CTL_TODO_CODE, Data.Rec.Code);
		if(Data.Rec.Kind == TODOKIND_TASK) {
			getCtrlData(sel = CTL_TODO_LINKTASK, linktask_code);
			if(linktask_code[0]) {
				int    r;
				THROW_PP_S(strcmp(Data.Rec.Code, linktask_code), PPERR_DUPTASKCODE, linktask_code);
				THROW_PP_S(r = PTCore.GetSingleByCode(TODOKIND_TASK, linktask_code, &Data.Rec.LinkTaskID), PPERR_DUPTASKCODE, linktask_code);
				THROW_PP_S(r > 0, PPERR_UNDEFTASKWCODE, linktask_code);
			}
			else
				Data.Rec.LinkTaskID = 0;
		}
		getCtrlData(sel = CTL_TODO_DT, &Data.Rec.Dt);
		THROW_SL(checkdate(Data.Rec.Dt, 1));
		getCtrlData(sel = CTL_TODO_TM, &Data.Rec.Tm);
		getCtrlData(sel = CTL_TODO_START, &Data.Rec.StartDt);
		THROW_SL(checkdate(Data.Rec.StartDt, 1));
		getCtrlData(sel = CTL_TODO_STARTTM, &Data.Rec.StartTm);
		getCtrlData(sel = CTL_TODO_ESTFINISH, &Data.Rec.EstFinishDt);
		THROW_SL(checkdate(Data.Rec.EstFinishDt, 1));
		getCtrlData(sel = CTL_TODO_ESTFINISHTM, &Data.Rec.EstFinishTm);
		getCtrlData(sel = CTL_TODO_FINISH, &Data.Rec.FinishDt);
		THROW_SL(checkdate(Data.Rec.FinishDt, 1));
		getCtrlData(CTL_TODO_FINISHTM, &Data.Rec.FinishTm);
		getCtrlData(CTLSEL_TODO_TEMPLATE, &Data.Rec.TemplateID);
		getCtrlData(CTLSEL_TODO_CREATOR,  &Data.Rec.CreatorID);
		getCtrlData(CTLSEL_TODO_EMPLOYER, &Data.Rec.EmployerID);
		getCtrlData(CTLSEL_TODO_CLIENT,   &Data.Rec.ClientID);
		getCtrlData(CTLSEL_TODO_DLVRADDR, &Data.Rec.DlvrAddrID);
		if(getCtrlData(CTLSEL_TODO_PRIOR, &(temp_id = 0)))
			Data.Rec.Priority = (int16)temp_id;
		if(getCtrlData(CTLSEL_TODO_STATUS, &(temp_id = 0)))
			Data.Rec.Status = (int16)temp_id;
		getCtrlString(CTL_TODO_DESCR,   Data.SDescr);
		getCtrlString(CTL_TODO_MEMO,    Data.SMemo);
		getCtrlData(CTL_TODO_AMOUNT, &Data.Rec.Amount);
		getCtrlData(CTLSEL_TODO_BILLAR, &Data.Rec.BillArID);
		ASSIGN_PTR(pData, Data);
		CATCHZOKPPERRBYDLG
		return ok;
	}
private:
	DECL_HANDLE_EVENT;
	int    editRepeating();
	PrjTaskCore PTCore;
	PPObjProject PrjObj;
	PPObjPerson PsnObj;
};

int PrjTaskDialog::editRepeating()
{
	int    ok = -1;
	if(Data.Rec.Kind == TODOKIND_TEMPLATE) {
		DateRepeating dr = *reinterpret_cast<const DateRepeating *>(&Data.Rec.DrPrd);
		RepeatingDialog * dlg = new RepeatingDialog(RepeatingDialog::fEditRepeatAfterItem);
		if(CheckDialogPtrErr(&dlg)) {
			dlg->setDTS(&dr);
			for(int valid_data = 0; !valid_data && ExecView(dlg) == cmOK;)
				if(dlg->getDTS(&dr)) {
					*reinterpret_cast<DateRepeating *>(&Data.Rec.DrPrd) = dr;
					ok = valid_data = 1;
				}
		}
		delete dlg;
	}
	return ok;
}

IMPL_HANDLE_EVENT(PrjTaskDialog)
{
	TDialog::handleEvent(event);
	if(event.isCmd(cmRepeating))
		editRepeating();
	else if(event.isCbSelected(CTLSEL_TODO_CLIENT)) {
		PPID   new_cli_id = getCtrlLong(CTLSEL_TODO_CLIENT);
		if(new_cli_id != Data.Rec.ClientID) {
			Data.Rec.ClientID = new_cli_id;
			PsnObj.SetupDlvrLocCombo(this, CTLSEL_TODO_DLVRADDR, Data.Rec.ClientID, 0);
		}
	}
	else if(event.isCbSelected(CTLSEL_TODO_TEMPLATE) && !Data.Rec.ID) {
		getCtrlData(CTLSEL_TODO_TEMPLATE, &Data.Rec.TemplateID);
		if(Data.Rec.TemplateID) {
			PPObjPrjTask     todo_obj;
			PPPrjTaskPacket templ_pack;
			if(todo_obj.GetPacket(Data.Rec.TemplateID, &templ_pack) > 0) {
				getCtrlData(CTLSEL_TODO_EMPLOYER, &Data.Rec.EmployerID);
				if(!Data.Rec.EmployerID)
					SetupPPObjCombo(this, CTLSEL_TODO_EMPLOYER, PPOBJ_PERSON, templ_pack.Rec.EmployerID, OLW_CANINSERT, reinterpret_cast<void *>(PPPRK_EMPL));
				getCtrlData(CTLSEL_TODO_CLIENT, &Data.Rec.ClientID);
				if(!Data.Rec.ClientID)
					SetupPPObjCombo(this, CTLSEL_TODO_CLIENT, PPOBJ_PERSON, templ_pack.Rec.ClientID, OLW_CANINSERT, reinterpret_cast<void *>(PPPRK_CLIENT));
				getCtrlData(CTLSEL_TODO_DLVRADDR, &Data.Rec.DlvrAddrID);
				if(!Data.Rec.DlvrAddrID)
					PsnObj.SetupDlvrLocCombo(this, CTLSEL_TODO_DLVRADDR, templ_pack.Rec.ClientID, templ_pack.Rec.DlvrAddrID);
				getCtrlString(CTL_TODO_DESCR, Data.SDescr);
				if(Data.SDescr.IsEmpty())
					setCtrlString(CTL_TODO_DESCR, templ_pack.SDescr);
				getCtrlData(CTL_TODO_AMOUNT, &Data.Rec.Amount);
				if(Data.Rec.Amount == 0)
					setCtrlData(CTL_TODO_AMOUNT, &templ_pack.Rec.Amount);
			}
		}
	}
	else if(event.isCbSelected(CTLSEL_TODO_PRJ)) {
		PPID   prj_id = getCtrlLong(CTLSEL_TODO_PRJ);
		SetupPPObjCombo(this, CTLSEL_TODO_PHASE, PPOBJ_PROJECT, 0, OLW_CANINSERT, reinterpret_cast<void *>(NZOR(prj_id, -1)));
		Data.Rec.ProjectID = prj_id;
	}
	else if(TVBROADCAST && oneof2(TVCMD, cmReleasedFocus, cmCommitInput)) {
		if(event.isCtlEvent(CTL_TODO_FINISH)) {
			LDATE dt = ZERODATE;
			getCtrlData(CTL_TODO_FINISH, &dt);
			if(dt)
				if(checkdate(&dt)) {
					long temp_id = 0;
					if(getCtrlData(CTLSEL_TODO_STATUS, &temp_id))
						if(temp_id != TODOSTTS_COMPLETED) {
							temp_id = TODOSTTS_COMPLETED;
							Data.Rec.Status = (int16)temp_id;
							setCtrlData(CTLSEL_TODO_STATUS, &temp_id);
						}
				}
				else
					selectCtrl(CTL_TODO_FINISH);
		}
	}
	else
		return;
	clearEvent(event);
}

int PPObjPrjTask::EditDialog(PPPrjTaskPacket * pPack)
{
	uint   dlg_id = 0;
	if(pPack->Rec.Kind == TODOKIND_TEMPLATE)
		dlg_id = DLG_TODOTEMPL;
	else
		dlg_id = DLG_TODO;
	DIALOG_PROC_BODY_P1(PrjTaskDialog, dlg_id, pPack);
}

int PPObjPrjTask::Edit(PPID * pID, void * extraPtr)
{
	const  long extra_param = reinterpret_cast<long>(extraPtr);
	int    ok = cmCancel;
	int    r = 0;
	bool   is_new = false;
	int    task_finished = 0;
	PPID   prev_employer = 0;
	PPID   parent_prj = 0;
	PPID   client = 0;
	PPID   employer = 0;
	PPPrjTaskPacket pack;
	THROW(CheckRightsModByID(pID));
	if(*pID) {
		THROW(GetPacket(*pID, &pack) > 0);
		prev_employer = pack.Rec.EmployerID;
	}
	else {
		if(extra_param & PRJTASKBIAS_CLIENT)
			client = extra_param & ~(PRJTASKBIAS_CLIENT|PRJTASKBIAS_EMPLOYER|PRJTASKBIAS_TEMPLATE);
		else if(extra_param & PRJTASKBIAS_EMPLOYER)
			employer = extra_param & ~(PRJTASKBIAS_CLIENT|PRJTASKBIAS_EMPLOYER|PRJTASKBIAS_TEMPLATE);
		else
			parent_prj = extra_param & ~(PRJTASKBIAS_CLIENT|PRJTASKBIAS_EMPLOYER|PRJTASKBIAS_TEMPLATE);
		int    kind = (extra_param & PRJTASKBIAS_TEMPLATE) ? TODOKIND_TEMPLATE : TODOKIND_TASK;
		InitPacket(&pack, kind, parent_prj, client, employer, 1);
		is_new = true;
	}
	task_finished = BIN(pack.Rec.Status == TODOSTTS_COMPLETED && pack.Rec.FinishDt != ZERODATE);
	while(ok != cmOK && (r = EditDialog(&pack)) > 0) {
		if(PutPacket(pID, &pack, 1))
			ok = cmOK;
		else
			PPError();
	}
	if(!r)
		ok = 0;
	CATCHZOKPPERR
	if(pack.Rec.Kind == TODOKIND_TASK) {
		//
		// @todo Плохой блок. Необходимо переработать.
		//
		int    output_to_status_win = 0;
		int    employer_changed = BIN(prev_employer && prev_employer != pack.Rec.EmployerID);
		PPObjPerson::GetCurUserPerson(&employer, 0);
		int    employer_equal = BIN(employer && employer == pack.Rec.EmployerID);
		if(is_new && ok == cmOK && employer_equal) {
			pack.Rec.Flags |= TODOF_OPENEDBYEMPL;
			PutPacket(pID, &pack, 1);
   			output_to_status_win = 1;
		}
		else if(!is_new && ok != 0 && GetPacket(*pID, &pack) > 0) {
			employer = 0;
			SysJournal * p_sj = DS.GetTLA().P_SysJ;
			pack.Rec.OpenCount++;
			pack.Rec.Flags |= TODOF_ACTIONVIEWED;
			if(p_sj && employer_changed) {
				PPID   user_id = 0;
				PPIDArray act_list;
				act_list.add(PPACN_OBJVIEWED);
				GetUserByPerson(pack.Rec.EmployerID, &user_id);
				SETFLAG(pack.Rec.Flags, TODOF_OPENEDBYEMPL,
					employer_equal || p_sj->IsEventExists(PPOBJ_PRJTASK, pack.Rec.ID, user_id, &act_list) > 0);
			}
			else if(employer_equal)
				pack.Rec.Flags |= TODOF_OPENEDBYEMPL;
			PutPacket(pID, &pack, 1);
			output_to_status_win = 1;
		}
		//
		// создаем такую же задачу по шаблону, если текущая имеет статус
		// выполнена и в шаблоне указано - создавать новую задачу через опред. период
		//
		if(!task_finished && pack.Rec.TemplateID && pack.Rec.Status == TODOSTTS_COMPLETED && pack.Rec.FinishDt != ZERODATE) {
			PrjTaskTbl::Rec templ_rec;
			if(Search(pack.Rec.TemplateID, &templ_rec) > 0 && templ_rec.DrPrd == PRD_REPEATAFTERPRD) {
				PPTransaction tra(1);
				if(!!tra) {
					PPID   new_task_id = 0;
					PPPrjTaskPacket new_task_packet;
					DateRepeating dr = *reinterpret_cast<const DateRepeating *>(&templ_rec.DrPrd);
					LDATE  dt = (dr.Dtl.RA.AfterStart == 0) ? pack.Rec.FinishDt : ((pack.Rec.StartDt == ZERODATE) ? pack.Rec.Dt : pack.Rec.StartDt);
					plusperiod(&dt, dr.RepeatKind, dr.Dtl.RA.NumPrd, 0);
					memzero(pack.Rec.Code, sizeof(pack.Rec.Code));
					InitPacketByTemplate(&pack, dt, &new_task_packet, 0);
					new_task_packet.Rec.StartTm = pack.Rec.StartTm;
					new_task_packet.Rec.TemplateID = pack.Rec.TemplateID;
					new_task_packet.Rec.Amount = pack.Rec.Amount;
					new_task_packet.SMemo = pack.SMemo;
					PutPacket(&new_task_id, &new_task_packet, 0);
					tra.Commit();
					output_to_status_win = 1;
				}
			}
		}
		if(output_to_status_win)
			StatusWinChange();
	}
	return ok;
}

int PPObjPrjTask::AddBySample(PPID * pID, PPID sampleID)
{
	int    ok = -1;
	if(sampleID > 0) {
		PPPrjTaskPacket sample_pack;
		PPPrjTaskPacket pack;
		THROW(CheckRights(PPR_INS));
		THROW(GetPacket(sampleID, &sample_pack) > 0);
		pack = sample_pack;
		pack.Rec.ID = 0;
		pack.Rec.Status = TODOSTTS_NEW;
		pack.Rec.LinkTaskID = LinkTaskID;
		PPObjPerson::GetCurUserPerson(&pack.Rec.CreatorID, 0);
		getcurdatetime(&pack.Rec.Dt, &pack.Rec.Tm);
		{
			PPProjectConfig cfg;
			PPObjProject::ReadConfig(&cfg);
			PPObjOpCounter opc_obj;
			long   counter = 0;
			PPID   cntr_id = 0;
			if(pack.Rec.Kind == TODOKIND_TASK)
				cntr_id = cfg.TaskCntrID;
			else if(pack.Rec.Kind == TODOKIND_TEMPLATE)
				cntr_id = cfg.TemplCntrID;
			if(cntr_id)
				opc_obj.GetCode(cntr_id, &counter, pack.Rec.Code, sizeof(pack.Rec.Code), 0, 1);
		}
		while(ok <= 0 && (ok = EditDialog(&pack)) > 0)
			if(PutPacket(pID, &pack, 1))
				ok = 1;
			else
				ok = PPErrorZ();
	}
	CATCHZOKPPERR
	return ok;
}

int PPObjPrjTask::Browse(void * extraPtr)
{
	return ViewPrjTask(0);
}

int PPObjPrjTask::DeleteObj(PPID id)
{
	return PutPacket(&id, static_cast<PPPrjTaskPacket *>(0), 0);
}

int PPObjPrjTask::GetLinkTasks(PPID taskID, PPIDArray * pAry)
{
	int    ok  = -1;
	if(pAry) {
		PrjTaskTbl::Key6  k6;
		BExtQuery q(P_Tbl, 6);
		q.select(P_Tbl->ID, P_Tbl->LinkTaskID, 0L).where(P_Tbl->LinkTaskID == taskID);
		MEMSZERO(k6);
		k6.LinkTaskID = taskID;
		pAry->freeAll();
		for(q.initIteration(false, &k6, spGe); q.nextIteration() > 0;) {
			THROW(pAry->add(P_Tbl->data.ID));
		}
		ok = 1;
	}
	CATCH
		ok = (pAry->freeAll(), 0);
	ENDCATCH
	return ok;
}

class ViewTasksDialog : public PPListDialog {
public:
	explicit ViewTasksDialog(StrAssocArray * pData) : PPListDialog(DLG_VIEWTASKS, CTL_VIEWTASKS_LIST), P_Data(pData)
	{
		updateList(-1);
	}
private:
	virtual int setupList();
	virtual int editItem(long pos, long id);

	StrAssocArray * P_Data;
	PPObjPrjTask ObjPrjT;
};

/*virtual*/int ViewTasksDialog::setupList()
{
	int    ok = -1;
	if(P_Data) {
		SString buf;
		for(uint i = 0; i < P_Data->getCount(); i++) {
			buf = P_Data->Get(i).Txt;
			if(!buf.Len())
				buf.Cat(P_Data->Get(i).Id);
			THROW(addStringToList(P_Data->Get(i).Id, buf.cptr()));
		}
		ok = 1;
	}
	CATCHZOKPPERR
	return ok;
}

int ViewTasksDialog::editItem(long pos, long id)
{
	int    ok = -1;
	if(id > 0) {
		PPPrjTaskPacket pack;
		if(ObjPrjT.GetPacket(id, &pack) > 0) {
			ObjPrjT.EditDialog(&pack);
			ok = 1;
		}
	}
	return ok;
}

struct LostPrjTPersonItem { // @flat
	PPID   CreatorID;
	PPID   EmployerID;
	PPID   ClientID;
	PPID   ResolveCreatorID;
	PPID   ResolveEmployerID;
	PPID   ResolveClientID;
	long   Flags;
};

typedef TSVector <LostPrjTPersonItem> LostPrjTPersonArray;

class RestoreLostPrjTPersonDlg : public PPListDialog {
	DECL_DIALOG_DATA(LostPrjTPersonArray);
public:
	RestoreLostPrjTPersonDlg() : PPListDialog(DLG_RLOSTPSN, CTL_RLOSTPSN_LIST)
	{
		setSmartListBoxOption(CTL_VIEWTASKS_LIST, lbtSelNotify);
		setSmartListBoxOption(CTL_VIEWTASKS_LIST, lbtFocNotify);
	}
	DECL_DIALOG_SETDTS()
	{
		if(pData)
			Data.copy(*pData);
		else
			Data.freeAll();
		updateList(-1);
		return 1;
	}
	DECL_DIALOG_GETDTS()
	{
		CALLPTRMEMB(pData, copy(Data));
		return 1;
	}
private:
	DECL_HANDLE_EVENT;
	virtual int setupList();
	int    GetText(PPID id, const SString & rWord, SString & rBuf, int cat);
	LostPrjTPersonItem * GetCurItem();
	int ViewTasks(uint cm, const LostPrjTPersonItem * pItem);
};

IMPL_HANDLE_EVENT(RestoreLostPrjTPersonDlg)
{
	PPListDialog::handleEvent(event);
	if(TVCOMMAND) {
		if(oneof3(TVCMD, cmViewTasksByCreator, cmViewTasksByEmployer, cmViewTasksByClient))
			ViewTasks(TVCMD, GetCurItem());
		else if(oneof3(TVCMD, cmResolveCreator, cmResolveEmployer, cmResolveClient)) {
			LostPrjTPersonItem * p_item = GetCurItem();
			if(p_item) {
				PPID psn_id = 0;
				const int creator  = BIN(TVCMD == cmResolveCreator);
				const int employer = BIN(TVCMD == cmResolveEmployer);
				const int client   = BIN(TVCMD == cmResolveClient);
				if(ListBoxSelDialog::Run(PPOBJ_PERSON, &psn_id, reinterpret_cast<void *>((creator || employer) ? PPPRK_EMPL : PPPRK_CLIENT)) > 0) {
					if(creator)
						p_item->ResolveCreatorID = psn_id;
					else if(employer)
						p_item->ResolveEmployerID = psn_id;
					else
						p_item->ResolveClientID = psn_id;
					updateList(-1);
				}
			}
		}
		else if(oneof2(TVCMD, cmLBItemSelected, cmLBItemFocused)) {
			LostPrjTPersonItem * p_item = GetCurItem();
			if(p_item) {
				enableCommand(cmResolveCreator,      p_item->CreatorID  && !p_item->ResolveCreatorID);
				enableCommand(cmResolveEmployer,     p_item->EmployerID && !p_item->ResolveEmployerID);
				enableCommand(cmResolveClient,       p_item->ClientID   && !p_item->ResolveClientID);
				enableCommand(cmViewTasksByCreator,  p_item->CreatorID  && !p_item->ResolveCreatorID);
				enableCommand(cmViewTasksByEmployer, p_item->EmployerID && !p_item->ResolveEmployerID);
				enableCommand(cmViewTasksByClient,   p_item->ClientID   && !p_item->ResolveClientID);
			}
			else {
				enableCommand(cmResolveCreator,      0);
				enableCommand(cmViewTasksByCreator,  0);
				enableCommand(cmViewTasksByEmployer, 0);
				enableCommand(cmResolveClient,       0);
			}
		}
		else
			return;
	}
	else
		return;
	clearEvent(event);
}

LostPrjTPersonItem * RestoreLostPrjTPersonDlg::GetCurItem()
{
	long   pos = 0;
	LostPrjTPersonItem * p_item = 0;
	getSelection(&pos);
	if(pos >= 0 && pos < Data.getCountI())
		p_item = &Data.at(static_cast<uint>(pos));
	return p_item;
}

int RestoreLostPrjTPersonDlg::GetText(PPID id, const SString & rWord, SString & rBuf, int cat)
{
	int    ok = -1;
	if(cat) {
		SString buf, msg, word_notf;
		(word_notf = "[ID=%ld] is not found").CatDiv(';', 2);
		(buf = rWord).Space().Cat(word_notf);
		msg.Printf(buf.cptr(), id);
		rBuf.Cat(msg);
		ok = 1;
	}
	return ok;
}

/*virtual*/int RestoreLostPrjTPersonDlg::setupList()
{
	int    ok = 1;
	SString temp_buf;
	SString creator_word, employer_word, client_word;
	PPLoadString("creator", creator_word);
	PPLoadString("executor", employer_word);
	PPLoadString("client", client_word);
	for(uint i = 0; i < Data.getCount(); i++) {
		const LostPrjTPersonItem * p_item = &Data.at(i);
		const int resolve_creator  = BIN(p_item->CreatorID  && !p_item->ResolveCreatorID);
		const int resolve_employer = BIN(p_item->EmployerID && !p_item->ResolveEmployerID);
		const int resolve_client   = BIN(p_item->ClientID   && !p_item->ResolveClientID);
		if(resolve_creator || resolve_employer || resolve_client) {
			temp_buf.Z();
			GetText(p_item->CreatorID,  creator_word,  temp_buf, resolve_creator);
			GetText(p_item->EmployerID, employer_word, temp_buf, resolve_employer);
			GetText(p_item->ClientID,   client_word,   temp_buf, resolve_client);
			THROW(addStringToList(i, temp_buf));
		}
	}
	CATCHZOKPPERR
	return ok;
}

int RestoreLostPrjTPersonDlg::ViewTasks(uint cm, const LostPrjTPersonItem * pItem)
{
	int    ok = -1;
	BExtQuery * p_q = 0;
	ViewTasksDialog * p_dlg = 0;
	if(pItem) {
		union {
			PrjTaskTbl::Key0 k0;
			PrjTaskTbl::Key4 k4;
			PrjTaskTbl::Key5 k5;
		} k;
		void * k_ = 0;
		uint idx = 0;
		DBQ * dbq = 0;
		StrAssocArray list;
		//PrjTaskTbl prjt_tbl;
		PPObjPrjTask todo_obj;
		PrjTaskTbl * p_t = todo_obj.P_Tbl;
		SString temp_buf;
		MEMSZERO(k);
		if(cm == cmViewTasksByCreator) {
			idx = 0;
			k_ = static_cast<void *>(&k.k0);
			dbq = ppcheckfiltid(dbq, p_t->CreatorID, pItem->CreatorID);
		}
		else if(cm == cmViewTasksByEmployer) {
			idx = 4;
			k.k4.EmployerID = pItem->EmployerID;
			k_ = &k.k4;
			dbq = ppcheckfiltid(dbq, p_t->EmployerID, pItem->EmployerID);
		}
		else {
			idx = 5;
			k.k5.ClientID = pItem->ClientID;
			k_ = &k.k5;
			dbq = ppcheckfiltid(dbq, p_t->ClientID, pItem->ClientID);
		}
		p_q = new BExtQuery(p_t, idx);
		p_q->select(p_t->ID, /*p_t->Descr,*/ 0).where(*dbq);
		for(p_q->initIteration(false, &k_); p_q->nextIteration() > 0;) {
			const  PPID id = p_t->data.ID;
			todo_obj.GetItemDescr(id, temp_buf);
			list.Add(id, temp_buf);
		}
		if(list.getCount()) {
			THROW(CheckDialogPtr(&(p_dlg = new ViewTasksDialog(&list))));
			ExecView(p_dlg);
		}
		ok = 1;
	}
	CATCHZOK
	BExtQuery::ZDelete(&p_q);
	delete p_dlg;
	return ok;
}

int PPObjPrjTask::ResolveAbsencePersonHelper_(PPID newID, PPID prevID, int todoPerson)
{
	int    ok = -1;
	BExtQuery * p_q = 0;
	if(newID) {
		PrjTaskTbl::Key0 k0;
		PPIDArray todo_list;
		DBQ * dbq = 0;
		THROW_MEM(p_q = new BExtQuery(P_Tbl, 0));
		MEMSZERO(k0);
		if(todoPerson == TODOPSN_CREATOR)
			dbq = & (*dbq && P_Tbl->CreatorID == prevID);
		else if(todoPerson == TODOPSN_EMPLOYER)
			dbq = & (*dbq && P_Tbl->EmployerID == prevID);
		else if(todoPerson == TODOPSN_CLIENT)
			dbq = & (*dbq && P_Tbl->ClientID == prevID);
		p_q->select(P_Tbl->ID, 0L).where(*dbq);
		for(p_q->initIteration(false, &k0); p_q->nextIteration() > 0;)
			todo_list.add(P_Tbl->data.ID);
		for(uint i = 0; i < todo_list.getCount(); i++) {
			PPPrjTaskPacket prjt_pack;
			THROW(GetPacket(todo_list.at(i), &prjt_pack) > 0);
			if(todoPerson == TODOPSN_CREATOR)
				prjt_pack.Rec.CreatorID  = newID;
			if(todoPerson == TODOPSN_EMPLOYER)
				prjt_pack.Rec.EmployerID  = newID;
			if(todoPerson == TODOPSN_CLIENT)
				prjt_pack.Rec.ClientID  = newID;
			THROW(PutPacket(&prjt_pack.Rec.ID, &prjt_pack, 0));
		}
		ok = 1;
	}
	CATCHZOK
	BExtQuery::ZDelete(&p_q);
	return ok;
}

/*static*/int PPObjPrjTask::RecoverAbsencePerson()
{
	int    ok = 1;
	LostPrjTPersonArray list;
	PrjTaskTbl::Key0 k0;
	PPObjPrjTask obj_prjt;
	PPObjPerson obj_psn;
	RestoreLostPrjTPersonDlg * p_dlg = 0;
	BExtQuery * p_q = 0;
#define PSN_NOTF_SRCH(notfFlag, personID, offs) if(notfFlag) notfFlag = list.lsearch(&personID, 0, CMPF_LONG, offs) ? 0 : 1
	MEMSZERO(k0);
	THROW_MEM(p_q = new BExtQuery(obj_prjt.P_Tbl, 0));
	p_q->select(obj_prjt.P_Tbl->ID, obj_prjt.P_Tbl->CreatorID, obj_prjt.P_Tbl->EmployerID, obj_prjt.P_Tbl->ClientID, 0L);
	PPWaitStart();
	for(p_q->initIteration(false, &k0); p_q->nextIteration() > 0;) {
		int creator_notf  = BIN(obj_prjt.P_Tbl->data.CreatorID && obj_psn.Search(obj_prjt.P_Tbl->data.CreatorID)   <= 0);
		int employer_notf = BIN(obj_prjt.P_Tbl->data.EmployerID && obj_psn.Search(obj_prjt.P_Tbl->data.EmployerID) <= 0);
		int client_notf   = BIN(obj_prjt.P_Tbl->data.ClientID && obj_psn.Search(obj_prjt.P_Tbl->data.ClientID)     <= 0);
		PSN_NOTF_SRCH(creator_notf,  obj_prjt.P_Tbl->data.CreatorID,  offsetof(LostPrjTPersonItem, CreatorID));
		PSN_NOTF_SRCH(employer_notf, obj_prjt.P_Tbl->data.EmployerID, offsetof(LostPrjTPersonItem, EmployerID));
		PSN_NOTF_SRCH(client_notf,   obj_prjt.P_Tbl->data.ClientID,   offsetof(LostPrjTPersonItem, ClientID));
		if(creator_notf || employer_notf || client_notf) {
			LostPrjTPersonItem item;
			MEMSZERO(item);
			item.CreatorID  = (creator_notf)  ? obj_prjt.P_Tbl->data.CreatorID  : 0;
			item.EmployerID = (employer_notf) ? obj_prjt.P_Tbl->data.EmployerID : 0;
			item.ClientID   = (client_notf)   ? obj_prjt.P_Tbl->data.ClientID   : 0;
			THROW_SL(list.insert(&item));
		}
	}
	BExtQuery::ZDelete(&p_q);
	PPWaitStop();
	if(list.getCount()) {
		THROW(CheckDialogPtr(&(p_dlg = new RestoreLostPrjTPersonDlg())));
		p_dlg->setDTS(&list);
		if(ExecView(p_dlg) == cmOK) {
			p_dlg->getDTS(&list);
			PPWaitStart();
			{
				PPTransaction tra(1);
				THROW(tra);
				for(uint i = 0; i < list.getCount(); i++) {
		  			THROW(obj_prjt.ResolveAbsencePersonHelper_(list.at(i).ResolveCreatorID,  list.at(i).CreatorID,  TODOPSN_CREATOR));
					THROW(obj_prjt.ResolveAbsencePersonHelper_(list.at(i).ResolveEmployerID, list.at(i).EmployerID, TODOPSN_EMPLOYER));
					THROW(obj_prjt.ResolveAbsencePersonHelper_(list.at(i).ResolveClientID,   list.at(i).ClientID,   TODOPSN_CLIENT));
					PPWaitPercent(i + 1, list.getCount());
				}
				THROW(tra.Commit());
			}
		}
	}
	CATCHZOKPPERR
	delete p_dlg;
	BExtQuery::ZDelete(&p_q);
	PPWaitStop();
	return ok;
}
//
// Implementation of PPALDD_Project
//
PPALDD_CONSTRUCTOR(Project)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		Extra[0].Ptr = new PPObjProject;
	}
}

PPALDD_DESTRUCTOR(Project)
{
	Destroy();
	delete static_cast<PPObjProject *>(Extra[0].Ptr);
}

int PPALDD_Project::InitData(PPFilt & rFilt, long rsrv)
{
	int    ok = -1;
	if(rFilt.ID == H.ID)
		ok = DlRtm::InitData(rFilt, rsrv);
	else {
		MEMSZERO(H);
		H.ID = rFilt.ID;
		PPProjectPacket pack;
		PPObjProject * p_prj_obj = static_cast<PPObjProject *>(Extra[0].Ptr);
		if(p_prj_obj->GetPacket(rFilt.ID, &pack) > 0) {
			SString full_name;
			H.ID  = pack.Rec.ID;
			H.Kind        = pack.Rec.Kind;
			H.ParentID    = pack.Rec.ParentID;
			H.Dt  = pack.Rec.Dt;
			H.BeginDt     = pack.Rec.BeginDt;
			H.EstFinishDt = pack.Rec.EstFinishDt;
			H.FinishDt    = pack.Rec.FinishDt;
			H.MngrID      = pack.Rec.MngrID;
			H.ClientID    = pack.Rec.ClientID;
			H.TemplateID  = pack.Rec.TemplateID;
			H.Status      = pack.Rec.Status;
			H.Flags       = pack.Rec.Flags;
			STRNSCPY(H.Name, pack.Rec.Name);
			STRNSCPY(H.Code, pack.Rec.Code);
			STRNSCPY(H.Descr, pack.SDescr);
			STRNSCPY(H.Memo, pack.SMemo);
			p_prj_obj->GetFullName(pack.Rec.ID, full_name);
			STRNSCPY(H.FullName, full_name);
			ok = DlRtm::InitData(rFilt, rsrv);
		}
	}
	return ok;
}
//
// Implementation of PPALDD_ProjectView
//
PPALDD_CONSTRUCTOR(ProjectView)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		AssignDefIterData(&I, sizeof(I));
	}
}

PPALDD_DESTRUCTOR(ProjectView) { Destroy(); }

int PPALDD_ProjectView::InitData(PPFilt & rFilt, long rsrv)
{
	INIT_PPVIEW_ALDD_DATA_U(Project, rsrv);
	H.FltParentID  = p_filt->ParentID;
	H.FltClientID  = p_filt->ClientID;
	H.FltManagerID = p_filt->MngrID;
	H.FltFlags     = p_filt->Flags;
	H.FltStart_beg = p_filt->StartPeriod.low;
	H.FltStart_end = p_filt->StartPeriod.upp;
	H.FltEstFn_beg = p_filt->EstFinishPeriod.low;
	H.FltEstFn_end = p_filt->EstFinishPeriod.upp;
	return DlRtm::InitData(rFilt, rsrv);
}

int PPALDD_ProjectView::InitIteration(PPIterID iterId, int sortId, long rsrv)
{
	INIT_PPVIEW_ALDD_ITER(Project);
}

int PPALDD_ProjectView::NextIteration(PPIterID iterId)
{
	START_PPVIEW_ALDD_ITER(Project);
	I.ProjectID = item.ID;
	I.PrjTaskID = item.PrjTaskID;
	FINISH_PPVIEW_ALDD_ITER();
}

void PPALDD_ProjectView::Destroy() { DESTROY_PPVIEW_ALDD(Project); }
