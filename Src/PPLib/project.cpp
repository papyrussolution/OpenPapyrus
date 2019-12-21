// PROJECT.CPP
// Copyright (c) A.Sobolev 2003, 2004, 2005, 2006, 2007, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019
//
#include <pp.h>
#pragma hdrstop
//
//
//
PPProjectConfig::PPProjectConfig()
{
	Z();
}

PPProjectConfig & PPProjectConfig::Z()
{
	THISZERO();
	return *this;
}

//static
int FASTCALL PPObjProject::ReadConfig(PPProjectConfig * pCfg)
{
	int    r = PPRef->GetPropMainConfig(PPPRP_PROJECTCFG, pCfg, sizeof(*pCfg));
	if(r <= 0)
		memzero(pCfg, sizeof(*pCfg));
	return r;
}

static int SLAPI PutCounter(PPID * pID, PPObjOpCounter * pOpcObj, PPOpCounterPacket * pCntr)
{
	pCntr->Head.ObjType = PPOBJ_PROJECT;
	pCntr->Head.OwnerObjID = -1;
	return BIN(pOpcObj->PutPacket(pID, pCntr, 0));
}

static int SLAPI PPObjProject_WriteConfig(PPProjectConfig * pCfg, PPOpCounterPacket * pPrjCntr, PPOpCounterPacket * pPhsCntr,
	PPOpCounterPacket * pTodoCntr, PPOpCounterPacket * pTemplCntr)
{
	int    ok = 1;
	int    ta = 0;
	int    is_new = 1;
	Reference * p_ref = PPRef;
	PPObjOpCounter opc_obj;
	PPProjectConfig prev_cfg;
	{
		PPTransaction tra(1);
		THROW(tra);
		if(p_ref->GetPropMainConfig(PPPRP_PROJECTCFG, &prev_cfg, sizeof(prev_cfg)) > 0)
			is_new = 0;
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

//static
int SLAPI PPObjProject::EditConfig()
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
			Data.Cfg.RemindPrd.Set(0); // @v9.6.8
			GetIntRangeInput(this, sel = CTL_PRJCFG_REMINDPRD, &Data.Cfg.RemindPrd);
			if(Data.Cfg.Flags & PRJCFGF_INCOMPLETETASKREMIND)  {
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
	int    ok = -1, valid_data = 0, ta = 0, is_new = 0;
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

//static
SString & FASTCALL PPObjProject::MakeCodeString(const ProjectTbl::Rec * pRec, SString & rBuf)
{
	return rBuf.Z().Cat(pRec->Code).CatDiv('-', 1).Cat(pRec->Name);
}

TLP_IMPL(PPObjProject, ProjectTbl, P_Tbl);

SLAPI PPObjProject::PPObjProject(void * extraPtr) : PPObject(PPOBJ_PROJECT), ExtraPtr(extraPtr)
{
	TLP_OPEN(P_Tbl);
	ImplementFlags |= implStrAssocMakeList;
}

SLAPI PPObjProject::~PPObjProject()
{
	TLP_CLOSE(P_Tbl);
}

int SLAPI PPObjProject::DeleteObj(PPID id)
	{ return PutPacket(&id, 0, 0); }
int SLAPI PPObjProject::Search(PPID id, void * b)
	{ return SearchByID(P_Tbl, Obj, id, b); }
const char * SLAPI PPObjProject::GetNamePtr()
	{ return MakeCodeString(&P_Tbl->data, NameBuf).cptr(); }

StrAssocArray * SLAPI PPObjProject::MakeStrAssocList(void * extraPtr /*parentPrjID*/)
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
		for(q.initIteration(0, &k2, spGe); q.nextIteration() > 0;)
			THROW_SL(p_list->Add(P_Tbl->data.ID, MakeCodeString(&P_Tbl->data, name_buf)));
		THROW_DB(BTROKORNFOUND);
	}
	CATCH
		ZDELETE(p_list);
	ENDCATCH
	return p_list;
}

int SLAPI PPObjProject::GetFullName(PPID id, SString & rBuf)
{
	int    ok = -1;
	ProjectTbl::Rec rec;
	rBuf.Z();
	if(Search(id, &rec) > 0) {
		PPID   parent_id = 0;
		char   name_buf[64];
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

int SLAPI PPObjProject::Browse(void * extraPtr) { return PPView::Execute(PPVIEW_PROJECT, 0, PPView::exefModeless, 0); }

int SLAPI PPObjProject::InitPacket(ProjectTbl::Rec * pRec, int kind, PPID parentID, int use_ta)
{
	PPProjectConfig cfg;
	PPObjProject::ReadConfig(&cfg);
	PPObjOpCounter opc_obj;
	long   counter = 0;
	PPID   cntr_id = 0;
	memzero(pRec, sizeof(*pRec));
	pRec->Kind = NZOR(kind, PPPRJK_PROJECT);
	if(pRec->Kind == PPPRJK_PROJECT)
		cntr_id = cfg.PrjCntrID;
	else if(pRec->Kind == PPPRJK_PHASE)
		cntr_id = cfg.PhaseCntrID;
	if(cntr_id)
		opc_obj.GetCode(cntr_id, &counter, pRec->Code, sizeof(pRec->Code), 0, use_ta);
	pRec->Dt = LConfig.OperDate;
	pRec->ParentID   = parentID;
	pRec->Status = PPPRJSTS_ACTIVE;
	return 1;
}

int SLAPI PPObjProject::PutPacket(PPID * pID, ProjectTbl::Rec * pRec, int use_ta)
{
	int    ok = 1;
    PPID   acn_id = 0;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(pRec) {
			if(*pID) {
				THROW(UpdateByID(P_Tbl, Obj, *pID, pRec, 0));
				acn_id = PPACN_OBJUPD;
			}
			else {
				THROW(AddObjRecByID(P_Tbl, Obj, pID, pRec, 0));
				acn_id = PPACN_OBJADD;
			}
		}
		else if(*pID) {
			THROW(RemoveByID(P_Tbl, *pID, 0));
			acn_id = PPACN_OBJRMV;
		}
		DS.LogAction(acn_id, Obj, *pID, 0, 0);
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjProject::Read(PPObjPack * p, PPID id, void * stream, ObjTransmContext * pCtx)
{
	int    ok = 1;
	ProjectTbl::Rec * p_pack = new ProjectTbl::Rec;
	THROW_MEM(p_pack);
	p->Data = p_pack;
	if(stream == 0) {
		THROW(Search(id, p_pack) > 0);
	}
	else {
		SBuffer buffer;
		THROW_SL(buffer.ReadFromFile(static_cast<FILE *>(stream), 0));
		THROW_SL(P_Tbl->SerializeRecord(-1, p->Data, buffer, &pCtx->SCtx));
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjProject::Write(PPObjPack * p, PPID * pID, void * stream, ObjTransmContext * pCtx) // @srlz
{
	int    ok = 1;
	ProjectTbl::Rec * p_pack = 0;
	THROW(p && p->Data);
	p_pack = static_cast<ProjectTbl::Rec *>(p->Data);
	if(stream == 0) {
		p_pack->ID = *pID;
		if(!PutPacket(pID, p_pack, 1)) {
			pCtx->OutputAcceptErrMsg(PPTXT_ERRACCEPTPROJECT, p_pack->ID, p_pack->Name);
			ok = -1;
		}
	}
	else {
		SBuffer buffer;
		THROW_SL(P_Tbl->SerializeRecord(+1, p->Data, buffer, &pCtx->SCtx));
		THROW_SL(buffer.WriteToFile(static_cast<FILE *>(stream), 0, 0));
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjProject::ProcessObjRefs(PPObjPack * p, PPObjIDArray * ary, int replace, ObjTransmContext * pCtx)
{
	int    ok = 1;
	THROW(p && p->Data);
	{
		ProjectTbl::Rec * p_pack = static_cast<ProjectTbl::Rec *>(p->Data);
		THROW(ProcessObjRefInArray(PPOBJ_PROJECT, &p_pack->ParentID,   ary, replace));
		THROW(ProcessObjRefInArray(PPOBJ_PERSON,  &p_pack->MngrID,     ary, replace));
		THROW(ProcessObjRefInArray(PPOBJ_PERSON,  &p_pack->ClientID,   ary, replace));
		THROW(ProcessObjRefInArray(PPOBJ_PROJECT, &p_pack->TemplateID, ary, replace));
		THROW(ProcessObjRefInArray(PPOBJ_OPRKIND, &p_pack->BillOpID,   ary, replace));
	}
	CATCHZOK
	return ok;
}
//
//
//
class ProjectDialog : public TDialog {
	DECL_DIALOG_DATA(ProjectTbl::Rec);
public:
	explicit ProjectDialog(uint dlgID) : TDialog(dlgID)
	{
		SetupCalCtrl(CTLCAL_PRJ_DT, this, CTL_PRJ_DT, 4);
		SetupCalCtrl(CTLCAL_PRJ_START, this, CTL_PRJ_START, 4);
		SetupCalCtrl(CTLCAL_PRJ_ESTFINISH, this, CTL_PRJ_ESTFINISH, 4);
		SetupCalCtrl(CTLCAL_PRJ_FINISH, this, CTL_PRJ_FINISH, 4);
		SetupInputLine(CTL_PRJ_DESCR, MKSTYPE(S_ZSTRING, 256), MKSFMT(256, 0));
	}
	DECL_DIALOG_SETDTS()
	{
		int    ok = 1;
		ushort v = 0;
		RVALUEPTR(Data, pData);
		setCtrlData(CTL_PRJ_NAME,      Data.Name);
		setCtrlData(CTL_PRJ_CODE,      Data.Code);
		setCtrlData(CTL_PRJ_DT,        &Data.Dt);
		setCtrlData(CTL_PRJ_START,     &Data.BeginDt);
		setCtrlData(CTL_PRJ_ESTFINISH, &Data.EstFinishDt);
		setCtrlData(CTL_PRJ_FINISH,    &Data.FinishDt);
		SetupPersonCombo(this, CTLSEL_PRJ_CLIENT, Data.ClientID, OLW_CANINSERT, (PPID)PPPRK_CLIENT, 0);
		SetupPersonCombo(this, CTLSEL_PRJ_MNGR,   Data.MngrID,   OLW_CANINSERT, (PPID)PPPRK_EMPL,   0);
		if(oneof2(Data.Kind, PPPRJK_PROJECT, PPPRJK_PRJTEMPLATE)) {
			switch(Data.Status) {
				case PPPRJSTS_ACTIVE:    v = 0; break;
				case PPPRJSTS_NONACTIVE: v = 1; break;
				case PPPRJSTS_ARCHIVED:  v = 2; break;
				default: v = 0; break;
			}
		}
		else if(oneof2(Data.Kind, PPPRJK_PHASE, PPPRJK_PHSTEMPLATE)) {
			switch(Data.Status) {
				case PPPRJSTS_ACTIVE:    v = 0; break;
				case PPPRJSTS_NONACTIVE: v = 1; break;
				case PPPRJSTS_ARCHIVED:  v = 1; break; // Invalid for PHASE
				default: v = 0; break;
			}
			if(Data.ParentID) {
				PPObjProject prj_obj;
				SString buf;
				prj_obj.GetFullName(Data.ParentID, buf);
				setStaticText(CTL_PRJ_PARENTNAME, buf);
			}
		}
		setCtrlData(CTL_PRJ_STATUS, &v);
		setCtrlData(CTL_PRJ_DESCR, Data.Descr);
		{
			PPIDArray types;
			types.addzlist(PPOPT_ACCTURN, PPOPT_GOODSRECEIPT, PPOPT_GOODSEXPEND, PPOPT_GOODSRETURN,
				PPOPT_GOODSREVAL, PPOPT_GOODSMODIF, PPOPT_PAYMENT, PPOPT_CHARGE, PPOPT_GOODSACK,
				PPOPT_GENERIC, PPOPT_DRAFTRECEIPT, PPOPT_DRAFTEXPEND, 0L);
			SetupOprKindCombo(this, CTLSEL_PRJ_BILLOP, Data.BillOpID, 0, &types, 0);
		}
		return ok;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		ushort v;
		getCtrlData(CTL_PRJ_NAME,      Data.Name);
		getCtrlData(CTL_PRJ_CODE,      Data.Code);
		getCtrlData(CTL_PRJ_DT,        &Data.Dt);
		getCtrlData(CTL_PRJ_START,     &Data.BeginDt);
		getCtrlData(CTL_PRJ_ESTFINISH, &Data.EstFinishDt);
		getCtrlData(CTL_PRJ_FINISH,    &Data.FinishDt);
		getCtrlData(CTLSEL_PRJ_CLIENT, &Data.ClientID);
		getCtrlData(CTLSEL_PRJ_MNGR,   &Data.MngrID);
		getCtrlData(CTL_PRJ_STATUS, &(v = 0));
		if(oneof2(Data.Kind, PPPRJK_PROJECT, PPPRJK_PRJTEMPLATE)) {
			switch(v) {
				case 0: Data.Status = PPPRJSTS_ACTIVE; break;
				case 1: Data.Status = PPPRJSTS_NONACTIVE; break;
				case 2: Data.Status = PPPRJSTS_ARCHIVED; break;
				default: Data.Status = PPPRJSTS_ACTIVE; break;
			}
		}
		else if(oneof2(Data.Kind, PPPRJK_PHASE, PPPRJK_PHSTEMPLATE)) {
			switch(v) {
				case 0: Data.Status = PPPRJSTS_ACTIVE; break;
				case 1: Data.Status = PPPRJSTS_NONACTIVE; break;
				default: Data.Status = PPPRJSTS_ACTIVE; break;
			}
		}
		getCtrlData(CTL_PRJ_DESCR, Data.Descr);
		getCtrlData(CTLSEL_PRJ_BILLOP, &Data.BillOpID);
		ASSIGN_PTR(pData, Data);
		return ok;
	}
};
//
// If(parentPrjID != 0 && *pID == 0) then - необходимо создать элемент типа Phase
//
int SLAPI PPObjProject::Edit(PPID * pID, void * extraPtr /*parentPrjID*/)
{
	const  PPID extra_parent_prj_id = reinterpret_cast<PPID>(extraPtr);
	int    ok = cmCancel, valid_data = 0;
	uint   dlg_id = 0;
	ProjectTbl::Rec rec, parent_rec;
	ProjectDialog * dlg = 0;
	THROW(CheckRightsModByID(pID));
	if(*pID) {
		THROW(Search(*pID, &rec) > 0);
		if(rec.Kind == PPPRJK_PROJECT)
			dlg_id = DLG_PROJECT;
		else if(rec.Kind == PPPRJK_PHASE)
			dlg_id = DLG_PRJPHASE;
	}
	else {
		if(extra_parent_prj_id == 0) {
			InitPacket(&rec, PPPRJK_PROJECT, 0, 1);
			dlg_id = DLG_PROJECT;
		}
		else if(Search(extra_parent_prj_id, &parent_rec) > 0) {
			InitPacket(&rec, PPPRJK_PHASE, extra_parent_prj_id, 1);
			dlg_id = DLG_PRJPHASE;
		}
	}
	THROW(CheckDialogPtr(&(dlg = new ProjectDialog(dlg_id))));
	dlg->setDTS(&rec);
	for(valid_data = 0; !valid_data && ExecView(dlg) == cmOK;) {
		if(dlg->getDTS(&rec)) {
			THROW(PutPacket(pID, &rec, 1));
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
IMPLEMENT_PPFILT_FACTORY(Project); SLAPI ProjectFilt::ProjectFilt() : PPBaseFilt(PPFILT_PROJECT, 0, 1)
{
	SetFlatChunk(offsetof(ProjectFilt, ReserveStart),
		offsetof(ProjectFilt, Reserve)-offsetof(ProjectFilt, ReserveStart)+sizeof(Reserve));
	Init(1, 0);
}
//
//
//
SLAPI PPViewProject::PPViewProject() : PPView(&PrjObj, &Filt, PPVIEW_PROJECT), P_PrjTaskView(0)
{
	DefReportId = REPORT_PROJECTVIEW;
}

SLAPI PPViewProject::~PPViewProject()
{
	delete P_PrjTaskView;
}

int SLAPI PPViewProject::EditBaseFilt(PPBaseFilt * pBaseFilt)
{
	int    ok = -1;
	TDialog * dlg = 0;
	ProjectFilt filt;
	THROW(Filt.IsA(pBaseFilt));
	filt = *static_cast<const ProjectFilt *>(pBaseFilt);
	THROW(CheckDialogPtr(&(dlg = new TDialog(DLG_PRJFLT))));
	SetupCalCtrl(CTLCAL_PRJFLT_PRDSTART, dlg, CTL_PRJFLT_PRDSTART, 1);
	SetupCalCtrl(CTLCAL_PRJFLT_PRDESTFINISH, dlg, CTL_PRJFLT_PRDESTFINISH, 1);
	SetPeriodInput(dlg, CTL_PRJFLT_PRDSTART, &filt.StartPeriod);
	SetPeriodInput(dlg, CTL_PRJFLT_PRDESTFINISH, &filt.EstFinishPeriod);
	SetupPersonCombo(dlg, CTLSEL_PRJFLT_CLIENT, filt.ClientID, 0, (PPID)PPPRK_CLIENT, 0);
	SetupPersonCombo(dlg, CTLSEL_PRJFLT_MNGR,   filt.MngrID,   0, (PPID)PPPRK_EMPL,   0);
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
		if(pBaseFilt)
			*static_cast<ProjectFilt *>(pBaseFilt) = filt;
		ok = valid_data = 1;
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}

int SLAPI PPViewProject::Init_(const PPBaseFilt * pBaseFilt)
{
	if(Helper_InitBaseFilt(pBaseFilt)) {
		Filt.StartPeriod.Actualize(ZERODATE);
		Filt.EstFinishPeriod.Actualize(ZERODATE);
		return 1;
	}
	else
		return 0;
}

int SLAPI PPViewProject::InitIteration()
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
	P_IterQuery->initIteration(0, &k3, spGe);
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
				*((ProjectTbl::Rec *)pItem) = rec;
				ok = (Filt.Flags & ProjectFilt::fPrintPrjTasks) ? InitPrjTaskIterations(id) : 1;
			}
		}
	}
	if(ok < 0)
		ZDELETE(P_PrjTaskView);
	return ok;
}

int SLAPI PPViewProject::Transmit(PPID /*id*/)
{
	int    ok = -1;
	ObjTransmitParam param;
	if(ObjTransmDialog(DLG_OBJTRANSM, &param) > 0) {
		ProjectViewItem item;
		const  PPIDArray & rary = param.DestDBDivList.Get();
		PPObjIDArray objid_ary;
		PPWait(1);
		for(InitIteration(); NextIteration(&item) > 0; PPWaitPercent(GetCounter()))
			objid_ary.Add(PPOBJ_PROJECT, item.ID);
		THROW(PPObjectTransmit::Transmit(&rary, &objid_ary, &param));
		ok = 1;
	}
	CATCHZOKPPERR
	PPWait(0);
	return ok;
}

DBQuery * SLAPI PPViewProject::CreateBrowserQuery(uint * pBrwId, SString * pSubTitle)
{
	uint   brw_id = 0;
	DBQuery * q  = 0;
	ProjectTbl * t = 0;
	DBE    dbe_mgr, dbe_cli;
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
		t->Descr,       // #10
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

int SLAPI PPViewProject::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	if(ok == -2) {
		PPID   id = pHdr ? *static_cast<const PPID *>(pHdr) : 0;
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

void * SLAPI PPViewProject::GetEditExtraParam()
{
	return reinterpret_cast<void *>(Filt.ParentID);
}

int SLAPI PPViewProject::Detail(const void * pHdr, PPViewBrowser * pBrw)
{
	PPID   id = pHdr ? *static_cast<const PPID *>(pHdr) : 0;
	if(id) {
		PrjTaskFilt filt;
		filt.Init(1, 0);
		filt.ProjectID = id;
		ViewPrjTask(&filt);
	}
	return -1;
}

int SLAPI PPViewProject::ViewTotal()
{
	return -1;
}

int SLAPI PPViewProject::InitPrjTaskIterations(PPID prjID)
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

int SLAPI PPViewProject::PrintProjectTasks(PPID prjID)
{
	int    ok = -1;
	PPViewProject prj_view;
	ProjectFilt filt;
	filt.ParentID = prjID;
	filt.Flags    = ProjectFilt::fPrintPrjTasks;
	prj_view.Init_(&filt);
	if(prj_view.InitPrjTaskIterations(prjID) > 0) {
		PView  pv(&prj_view);
		ok = PPAlddPrint(REPORT_PROJECTTASKS, &pv);
	}
	return ok;
}

int SLAPI PPViewProject::Export()
{
	return -1;
}
//
// PrjTaskCore
//
//static
int FASTCALL PrjTaskCore::IsValidStatus(int s) { return (s >= 1 && s <= 5) ? 1 : PPSetError(PPERR_INVTODOSTATUS); }
//static
int FASTCALL PrjTaskCore::IsValidPrior(int p) { return (p >= 1 && p <= 5); }

SLAPI PrjTaskCore::PrjTaskCore() : PrjTaskTbl()
{
}

int SLAPI PrjTaskCore::Search(PPID id, PrjTaskTbl::Rec * pRec)
{
	return SearchByID(this, PPOBJ_PRJTASK, id, pRec);
}

int SLAPI PrjTaskCore::NextEnum(long enumHandle, PrjTaskTbl::Rec * pRec)
	{ return (EnumList.NextIter(enumHandle) > 0) ? (copyBufTo(pRec), 1) : -1; }
int SLAPI PrjTaskCore::DestroyIter(long enumHandle)
	{ return EnumList.DestroyIterHandler(enumHandle); }

BExtQuery * SLAPI PrjTaskCore::StartupEnumQuery(int idx, int options)
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

SEnumImp * SLAPI PrjTaskCore::EnumByClient(PPID cliPersonID, const DateRange * pPeriod, int options)
{
	long   h = -1;
	int    idx = 5;
	BExtQuery * q = StartupEnumQuery(idx, options);
	q->where(this->ClientID == cliPersonID && daterange(this->Dt, pPeriod));
	PrjTaskTbl::Key5 k5;
	MEMSZERO(k5);
	k5.ClientID = cliPersonID;
	k5.Dt = pPeriod ? pPeriod->low : ZERODATE;
	q->initIteration(0, &k5, spGe);
	return EnumList.RegisterIterHandler(q, &h) ? new PPTblEnum <PrjTaskCore>(this, h) : 0;
}

SEnumImp * SLAPI PrjTaskCore::EnumByEmployer(PPID emplPersonID, const DateRange * pPeriod, int options)
{
	long   h = -1;
	int    idx = 4;
	BExtQuery * q = StartupEnumQuery(idx, options);
	q->where(this->EmployerID == emplPersonID && daterange(this->Dt, pPeriod));
	PrjTaskTbl::Key4 k4;
	MEMSZERO(k4);
	k4.EmployerID = emplPersonID;
	k4.Dt = pPeriod ? pPeriod->low : ZERODATE;
	q->initIteration(0, &k4, spGe);
	return EnumList.RegisterIterHandler(q, &h) ? new PPTblEnum <PrjTaskCore>(this, h) : 0;
}

int SLAPI PrjTaskCore::SearchByTime(const LDATETIME & dtm, PPID * pID, PrjTaskTbl::Rec * pRec)
{
	PrjTaskTbl::Key1 k1;
	k1.Dt = dtm.d;
	k1.Tm = dtm.t;
	int    ok = SearchByKey(this, 1, &k1, pRec);
	if(ok > 0)
		ASSIGN_PTR(pID, data.ID);
	return ok;
}

int SLAPI PrjTaskCore::SearchByTemplate(PPID templID, LDATE startDt, PrjTaskTbl::Rec * pRec)
{
	PrjTaskTbl::Key3 k3;
	MEMSZERO(k3);
	k3.TemplateID = templID;
	BExtQuery q(this, 3);
	q.select(this->ID, 0L).where(this->TemplateID == templID && this->StartDt == startDt);
	for(q.initIteration(0, &k3, spGe); q.nextIteration() > 0;)
		if(Search(data.ID, pRec) > 0)
			return 1;
	return -1;
}

int SLAPI PrjTaskCore::Add(PPID * pID, PrjTaskTbl::Rec * pRec, int use_ta)
{
	LDATETIME dtm;
	dtm.Set(pRec->Dt, pRec->Tm);
	while(SearchByTime(dtm, 0, 0) > 0)
		dtm.t.v++;
	pRec->Tm = dtm.t;
	return AddObjRecByID(this, PPOBJ_PRJTASK, pID, pRec, use_ta);
}

int SLAPI PrjTaskCore::Update(PPID id, PrjTaskTbl::Rec * pRec, int use_ta)
{
	LDATETIME dtm;
	dtm.Set(pRec->Dt, pRec->Tm);
	while(SearchByTime(dtm, 0, 0) > 0)
		dtm.t.v++;
	pRec->Tm = dtm.t;
	return UpdateByID(this, PPOBJ_PRJTASK, id, pRec, use_ta);
}

int SLAPI PrjTaskCore::UpdateStatus(PPID id, int newStatus, int use_ta)
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

int SLAPI PrjTaskCore::Remove(PPID id, int use_ta)
	{ return deleteFrom(this, use_ta, this->ID == id) ? 1 : PPSetErrorDB(); }
int SLAPI PrjTaskCore::RemoveByProject(PPID prjID, int use_ta)
	{ return deleteFrom(this, use_ta, this->ProjectID == prjID) ? 1 : PPSetErrorDB(); }

int SLAPI PrjTaskCore::SearchAnyRef(PPID objType, PPID objID, PPID * pID)
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
		q.initIteration(0, &k_, spFirst);
		if(q.nextIteration() > 0) {
			ASSIGN_PTR(pID, data.ID);
			ok = 1;
		}
	}
	else if(objType == PPOBJ_ARTICLE) {
		BExtQuery q(this, 0, 1);
		q.select(this->ID, 0).where(this->BillArID == objID);
		q.initIteration(0, &k_, spFirst);
		if(q.nextIteration() > 0) {
			ASSIGN_PTR(pID, data.ID);
			ok = 1;
		}
	}
	else if(objType == PPOBJ_LOCATION) {
		BExtQuery q(this, 0, 1);
		q.select(this->ID, 0).where(this->DlvrAddrID == objID);
		q.initIteration(0, &k_, spFirst);
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

int SLAPI PrjTaskCore::ReplaceRefs(PPID objType, PPID replacedID, PPID newID, int use_ta)
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
		for(q.initIteration(0, &k_, spFirst); q.nextIteration() > 0;) {
			id_list.add(data.ID);
		}
		if(id_list.getCount()) {
			PPTransaction tra(use_ta);
			THROW(tra);
			for(uint i = 0; i < id_list.getCount(); i++) {
				const PPID id = id_list.get(i);
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

int SLAPI PrjTaskCore::GetSingleByCode(long kind, const char * pCode, PPID * pID)
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
		for(q.initIteration(0, &k0, spGe); q.nextIteration() > 0;)
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
static SString & SLAPI _GetEnumText(uint strId, int i, SString & rBuf)
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

// static
SString & SLAPI PPObjPrjTask::GetStatusText(int statusId, SString & rBuf)
	{ return _GetEnumText(PPTXT_TODO_STATUS, statusId, rBuf); }
// static
SString & SLAPI PPObjPrjTask::GetPriorText(int priorId, SString & rBuf)
	{ return _GetEnumText(PPTXT_TODO_PRIOR, priorId, rBuf); }

class VCalImportParamDlg : public TDialog {
public:
	struct Param {
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
	};
private:
	DECL_DIALOG_DATA(Param);
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
			Data.Init();
		SetupPersonCombo(this, CTLSEL_VCALPAR_CREATOR, Data.DefCreatorID,  0, PPPRK_EMPL, 0);
		SetupPersonCombo(this, CTLSEL_VCALPAR_EMPL,    Data.DefEmployerID, 0, PPPRK_EMPL, 0);
		SetupPersonCombo(this, CTLSEL_VCALPAR_CLIENT,  Data.DefClientID,   0, PPPRK_CLIENT, 0);
		CALLPTRMEMB(p_grp, addPattern(PPTXT_FILPAT_VCALENDAR));
		return 1;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = -1;
		uint   sel = 0;
		getCtrlData(sel = CTLSEL_VCALPAR_CREATOR, &Data.DefCreatorID);
		THROW_PP(Data.DefCreatorID, PPERR_INVDEFCREATOR);
		getCtrlData(CTLSEL_VCALPAR_EMPL,    &Data.DefClientID);
		getCtrlData(CTLSEL_VCALPAR_CLIENT,  &Data.DefEmployerID);
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

// Static
int SLAPI PPObjPrjTask::ImportFromVCal()
{
	int    ok = -1;
	int    valid_data = 0;
	VCalImportParamDlg::Param param;
	VCalImportParamDlg * p_dlg = new VCalImportParamDlg;
	THROW(CheckDialogPtr(&p_dlg));
	p_dlg->setDTS(&param);
	while(!valid_data && ExecView(p_dlg) == cmOK) {
		if(p_dlg->getDTS(&param) > 0)
			valid_data = ok = 1;
		else
			PPError();
	}
	if(ok > 0) {
		VCalendar vcal;
		PPWait(1);
		if(vcal.Open(param.FilePath, 0) > 0) {
			VCalendar::Todo vcal_rec;
			PPObjPrjTask todo_obj;
			PPObjPerson  psn_obj;
			PersonTbl::Rec psn_rec;
			const PPID cli_pk_id = PPPRK_CLIENT;
			PPIDArray cli_kind_list;
			cli_kind_list.add(cli_pk_id);
			{
				PPTransaction tra(1);
				THROW(tra);
				while(vcal.GetTodo(&vcal_rec) > 0) {
					if(vcal_rec.Descr.Len() > 0) {
						PPID   id = 0;
						PrjTaskTbl::Rec todo_rec;
						THROW(todo_obj.InitPacket(&todo_rec, TODOKIND_TASK, 0, 0, 0, 0));
						psn_obj.P_Tbl->SearchByName(vcal_rec.Owner, &todo_rec.EmployerID);
						todo_rec.EmployerID = (todo_rec.EmployerID) ? todo_rec.EmployerID : param.DefEmployerID;
						vcal_rec.Descr.CopyTo(todo_rec.Descr, sizeof(todo_rec.Descr));

						todo_rec.Dt          = vcal_rec.CreatedDtm.d;
						todo_rec.Tm          = vcal_rec.CreatedDtm.t;
						todo_rec.StartDt     = vcal_rec.StartDtm.d;
						todo_rec.StartTm     = vcal_rec.StartDtm.t;
						todo_rec.FinishDt    = vcal_rec.CompletedDtm.d;
						todo_rec.FinishTm    = vcal_rec.CompletedDtm.t;
						todo_rec.EstFinishDt = vcal_rec.DueDtm.d;
						todo_rec.EstFinishTm = vcal_rec.DueDtm.t;
						todo_rec.OpenCount   = (int32)vcal_rec.Sequence;
						todo_rec.Priority    = vcal_rec.Priority;
						if(vcal_rec.Status == VCalendar::stAccepted)
							todo_rec.Status = TODOSTTS_NEW;
						else if(vcal_rec.Status == VCalendar::stDeclined)
							todo_rec.Status = TODOSTTS_REJECTED;
						else if(vcal_rec.Status == VCalendar::stConfirmed)
							todo_rec.Status = TODOSTTS_INPROGRESS;
						else if(vcal_rec.Status == VCalendar::stNeedsAction)
							todo_rec.Status = TODOSTTS_ONHOLD;
						else if(vcal_rec.Status == VCalendar::stCompleted)
							todo_rec.Status = TODOSTTS_COMPLETED;
						// @v9.5.9 {
						if(vcal_rec.Contact.NotEmptyS()) {
							if(psn_obj.SearchFirstByName(vcal_rec.Contact, &cli_kind_list, 0, &psn_rec) > 0) {
								todo_rec.ClientID = psn_rec.ID;
							}
						}
						SETIFZ(todo_rec.ClientID, param.DefClientID);
						SETIFZ(todo_rec.CreatorID, param.DefCreatorID);
						// } @v9.5.9
						THROW(todo_obj.PutPacket(&id, &todo_rec, 0));
					}
				}
				THROW(tra.Commit());
			}
		}
		PPWait(0);
	}
	CATCHZOKPPERR
	delete p_dlg;
	return ok;
}

TLP_IMPL(PPObjPrjTask, PrjTaskCore, P_Tbl);

SLAPI PPObjPrjTask::PPObjPrjTask(void * extraPtr) : PPObject(PPOBJ_PRJTASK), ExtraPtr(extraPtr)
{
	TLP_OPEN(P_Tbl);
	ImplementFlags |= implStrAssocMakeList;
}

SLAPI PPObjPrjTask::~PPObjPrjTask()
{
	TLP_CLOSE(P_Tbl);
}

//virtual
int SLAPI PPObjPrjTask::Search(PPID id, void * pRec)
	{ return P_Tbl->Search(id, (PrjTaskTbl::Rec *)pRec); }
//virtual
const char * SLAPI PPObjPrjTask::GetNamePtr()
	{ return P_Tbl->data.Code; }

StrAssocArray * SLAPI PPObjPrjTask::MakeStrAssocList(void * extraPtr)
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
	for(q.initIteration(0, &k0, spGe); q.nextIteration() > 0;) {
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

int SLAPI PPObjPrjTask::HandleMsg(int msg, PPID _obj, PPID _id, void * extraPtr)
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

int SLAPI PPObjPrjTask::Read(PPObjPack * p, PPID id, void * stream, ObjTransmContext * pCtx)
{
	int    ok = 1;
	PrjTaskTbl::Rec * p_pack = new PrjTaskTbl::Rec;
	THROW_MEM(p_pack);
	p->Data = p_pack;
	if(stream == 0) {
		THROW(P_Tbl->Search(id, p_pack) > 0);
	}
	else {
		SBuffer buffer;
		THROW_SL(buffer.ReadFromFile(static_cast<FILE *>(stream), 0));
		THROW_SL(P_Tbl->SerializeRecord(-1, p->Data, buffer, &pCtx->SCtx));
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjPrjTask::Write(PPObjPack * p, PPID * pID, void * stream, ObjTransmContext * pCtx) // @srlz
{
	int    ok = 1;
	PrjTaskTbl::Rec * p_pack = 0;
	THROW(p && p->Data);
	p_pack = static_cast<PrjTaskTbl::Rec *>(p->Data);
	if(stream == 0) {
		p_pack->ID = *pID;
		if(!PutPacket(pID, p_pack, 1)) {
			pCtx->OutputAcceptErrMsg(PPTXT_ERRACCEPTPRJTASK, p_pack->ID, p_pack->Code);
			ok = -1;
		}
	}
	else {
		SBuffer buffer;
		THROW_SL(P_Tbl->SerializeRecord(+1, p->Data, buffer, &pCtx->SCtx));
		THROW_SL(buffer.WriteToFile(static_cast<FILE *>(stream), 0, 0));
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjPrjTask::EditRights(uint bufSize, ObjRights * rt, EmbedDialog * pDlg)
{
	return EditSpcRightFlags(DLG_RTPRJTASK, CTL_RTPRJTASK_FLAGS, CTL_RTPRJTASK_SFLAGS, bufSize, rt, pDlg);
}

int SLAPI PPObjPrjTask::ProcessObjRefs(PPObjPack * p, PPObjIDArray * ary, int replace, ObjTransmContext * pCtx)
{
	int    ok = 1;
	THROW(p && p->Data);
	{
		PrjTaskTbl::Rec * p_pack = static_cast<PrjTaskTbl::Rec *>(p->Data);
		THROW(ProcessObjRefInArray(PPOBJ_PROJECT,  &p_pack->ProjectID,   ary, replace));
		THROW(ProcessObjRefInArray(PPOBJ_PERSON,   &p_pack->CreatorID,   ary, replace));
		THROW(ProcessObjRefInArray(PPOBJ_PERSON,   &p_pack->EmployerID,  ary, replace));
		THROW(ProcessObjRefInArray(PPOBJ_PERSON,   &p_pack->ClientID,    ary, replace));
		THROW(ProcessObjRefInArray(PPOBJ_PRJTASK,  &p_pack->TemplateID,  ary, replace));
		THROW(ProcessObjRefInArray(PPOBJ_PRJTASK,  &p_pack->LinkTaskID,  ary, replace));
		THROW(ProcessObjRefInArray(PPOBJ_LOCATION, &p_pack->DlvrAddrID,  ary, replace));
		THROW(ProcessObjRefInArray(PPOBJ_ARTICLE,  &p_pack->BillArID,    ary, replace));
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjPrjTask::DetermineNewStatus(const PPProjectConfig * pCfg, const PrjTaskTbl::Rec * pRec, int * pNewStatus)
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

int SLAPI PPObjPrjTask::Maintain()
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
			q.where(P_Tbl->Kind == (long)TODOKIND_TASK);
			PrjTaskTbl::Key0 k0, k0_;
			MEMSZERO(k0);
			k0_ = k0;
			cntr.Init(q.countIterations(0, &k0_, spFirst));
			for(q.initIteration(0, &k0, spFirst); q.nextIteration() > 0;) {
				PrjTaskTbl::Rec rec;
				P_Tbl->copyBufTo(&rec);
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
			q.where(P_Tbl->Kind == (long)TODOKIND_TEMPLATE);
			PrjTaskTbl::Key0 k0, k0_;
			MEMSZERO(k0);
			k0_ = k0;
			cntr.Init(q.countIterations(0, &k0_, spFirst));
			for(q.initIteration(0, &k0, spFirst); q.nextIteration() > 0;) {
				PrjTaskTbl::Rec rec;
				P_Tbl->copyBufTo(&rec);
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

int SLAPI MaintainPrjTask()
{
	int    ok = -1;
	if(CONFIRM(PPCFM_MAINTAINPRJTASK)) {
		PPWait(1);
		PPObjPrjTask todo_obj;
		if(todo_obj.Maintain())
			ok = 1;
		else
			ok = PPErrorZ();
		PPWait(0);
	}
	return ok;
}

int SLAPI PPObjPrjTask::InitPacket(PrjTaskTbl::Rec * pRec, int kind, PPID prjID, PPID clientID, PPID employerID, int use_ta)
{
	PPProjectConfig cfg;
	PPObjProject::ReadConfig(&cfg);
	PPObjOpCounter opc_obj;
	long   counter = 0;
	PPID   cntr_id = 0;
	memzero(pRec, sizeof(*pRec));
	pRec->Kind = NZOR(kind, TODOKIND_TASK);
	if(pRec->Kind == TODOKIND_TASK)
		cntr_id = cfg.TaskCntrID;
	else if(pRec->Kind == TODOKIND_TEMPLATE)
		cntr_id = cfg.TemplCntrID;
	if(cntr_id)
		opc_obj.GetCode(cntr_id, &counter, pRec->Code, sizeof(pRec->Code), 0, use_ta);
	pRec->ProjectID  = prjID;
	pRec->ClientID   = clientID;
	pRec->EmployerID = employerID;
	pRec->Priority   = TODOPRIOR_NORMAL;
	pRec->Status     = TODOSTTS_NEW;
	pRec->LinkTaskID = LinkTaskID;
	PPObjPerson::GetCurUserPerson(&pRec->CreatorID, 0);
	getcurdatetime(&pRec->Dt, &pRec->Tm);
	return 1;
}

int SLAPI PPObjPrjTask::PutPacket(PPID * pID, PrjTaskTbl::Rec * pRec, int use_ta)
{
	int    ok = 1;
	PPID   acn_id = 0;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(*pID) {
			if(pRec) {
				const int acn_viewed = BIN(pRec->Flags & TODOF_ACTIONVIEWED);
				pRec->Flags &= ~TODOF_ACTIONVIEWED;
				THROW(P_Tbl->Update(*pID, pRec, 0));
				acn_id = acn_viewed ? PPACN_OBJVIEWED : PPACN_OBJUPD;
			}
			else {
				THROW(P_Tbl->Remove(*pID, 0));
				acn_id = PPACN_OBJRMV;
			}
		}
		else if(pRec) {
			THROW(P_Tbl->Add(pID, pRec, 0));
			acn_id = PPACN_OBJADD;
		}
		DS.LogAction(acn_id, Obj, *pID, 0, 0);
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjPrjTask::GetPacket(PPID id, PrjTaskTbl::Rec * pRec)
{
	return Search(id, pRec);
}

int SLAPI PPObjPrjTask::SubstDescr(PrjTaskTbl::Rec * pPack)
{
	int    ok = 1;
	char   buf[512], * b = buf;
	SString temp_buf;
	size_t len = 0;
	PPObjPerson * p_psn_obj = 0;
	PersonTbl::Rec psn_rec;
	PPSymbTranslator st;
	for(char * p = pPack->Descr; *p;) {
		if(*p == '@') {
			size_t next = 1;
			long   sym  = st.Translate(p, &next);
			switch(sym) {
				case PPSYM_CLIENT:
					THROW_MEM(SETIFZ(p_psn_obj, new PPObjPerson));
					if(p_psn_obj->Search(pPack->ClientID, &psn_rec) > 0)
						strnzcpy(b, psn_rec.Name, sizeof(buf)-len);
					else
						*b = 0;
					break;
				case PPSYM_CLIENTADDR:
					if(pPack->ClientID) {
						THROW_MEM(SETIFZ(p_psn_obj, new PPObjPerson));
						if(pPack->DlvrAddrID) {
							// @v9.5.5 p_psn_obj->LocObj.P_Tbl->GetAddress(pPack->DlvrAddrID, 0, temp_buf);
							p_psn_obj->LocObj.GetAddress(pPack->DlvrAddrID, 0, temp_buf); // @v9.5.5
						}
						else
							p_psn_obj->GetAddress(pPack->ClientID, temp_buf);
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
	STRNSCPY(pPack->Descr, buf);
	CATCHZOK
	delete p_psn_obj;
	return ok;
}

int SLAPI PPObjPrjTask::InitPacketByTemplate(const PrjTaskTbl::Rec * pTemplRec, LDATE startDt, PrjTaskTbl::Rec * pRec, int use_ta)
{
	int    ok = 1;
	THROW_INVARG(pTemplRec && pRec && startDt != ZERODATE);
	THROW(InitPacket(pRec, TODOKIND_TASK, pTemplRec->ProjectID, pTemplRec->ClientID, pTemplRec->EmployerID, use_ta));
	getcurdatetime(&pRec->Dt, &pRec->Tm);
	pRec->TemplateID = pTemplRec->ID;
	pRec->DlvrAddrID = pTemplRec->DlvrAddrID;
	pRec->ProjectID  = pTemplRec->ProjectID;
	pRec->BillArID   = pTemplRec->BillArID;
	pRec->StartDt    = startDt;
	pRec->Priority   = pTemplRec->Priority;
	STRNSCPY(pRec->Descr, pTemplRec->Descr);
	THROW(SubstDescr(pRec));
	CATCHZOK
	return ok;
}

int SLAPI PPObjPrjTask::CreateByTemplate(PPID templID, const DateRange * pPeriod, PPIDArray * pIdList, int use_ta)
{
	int    ok = -1;
	PrjTaskTbl::Rec templ_rec;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(Search(templID, &templ_rec) > 0 && templ_rec.Kind == TODOKIND_TEMPLATE &&
			templ_rec.Status != TODOSTTS_REJECTED && templ_rec.DrPrd && templ_rec.DrPrd != PRD_REPEATAFTERPRD) {
			DateRange period;
			if(pPeriod) {
				period = *pPeriod;
				SETIFZ(period.upp, getcurdate_());
			}
			else {
				period.low = ZERODATE;
				period.upp = getcurdate_();
			}
			const DateRepeating rept = *(DateRepeating *)&templ_rec.DrPrd;
			DateRepIterator dr_iter(rept, templ_rec.Dt, period.upp);
			for(LDATE dt = dr_iter.Next(); dt; dt = dr_iter.Next()) {
				if(period.CheckDate(dt)) {
					const int r = P_Tbl->SearchByTemplate(templID, dt, 0);
					PPID   id = 0;
					THROW(r);
					if(r < 0) {
						PrjTaskTbl::Rec rec;
						THROW(InitPacketByTemplate(&templ_rec, dt, &rec, 0));
						THROW(PutPacket(&id, &rec, 0));
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
public:
	explicit PrjTaskDialog(uint dlgID) : TDialog(/*DLG_TODO*/dlgID)
	{
		SetupCalDate(CTLCAL_TODO_DT, CTL_TODO_DT);
		SetupCalDate(CTLCAL_TODO_START, CTL_TODO_START);
		SetupCalDate(CTLCAL_TODO_ESTFINISH, CTL_TODO_ESTFINISH);
		SetupCalDate(CTLCAL_TODO_FINISH, CTL_TODO_FINISH);
		SetupInputLine(CTL_TODO_DESCR, MKSTYPE(S_ZSTRING, 256), MKSFMT(256, 0));
		SetupInputLine(CTL_TODO_MEMO, MKSTYPE(S_ZSTRING, 1024), MKSFMT(1024, 0));
		SetupTimePicker(this, CTL_TODO_STARTTM, CTLTM_TODO_STARTTM);
		SetupTimePicker(this, CTL_TODO_ESTFINISHTM, CTLTM_TODO_ESTFINISHTM);
		SetupTimePicker(this, CTL_TODO_FINISHTM, CTLTM_TODO_FINISHTM);
	}
	int    setDTS(const PrjTaskTbl::Rec * pData);
	int    getDTS(PrjTaskTbl::Rec * pData);
private:
	DECL_HANDLE_EVENT;
	int    editRepeating();
	PrjTaskTbl::Rec Data;
	PrjTaskCore PTCore;
	PPObjProject PrjObj;
	PPObjPerson PsnObj;
};

int PrjTaskDialog::editRepeating()
{
	int    ok = -1;
	if(Data.Kind == TODOKIND_TEMPLATE) {
		DateRepeating dr = *(DateRepeating *)&Data.DrPrd;
		RepeatingDialog * dlg = new RepeatingDialog(RepeatingDialog::fEditRepeatAfterItem);
		if(CheckDialogPtrErr(&dlg)) {
			dlg->setDTS(&dr);
			for(int valid_data = 0; !valid_data && ExecView(dlg) == cmOK;)
				if(dlg->getDTS(&dr)) {
					*(DateRepeating *)&Data.DrPrd = dr;
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
		if(new_cli_id != Data.ClientID) {
			Data.ClientID = new_cli_id;
			PsnObj.SetupDlvrLocCombo(this, CTLSEL_TODO_DLVRADDR, Data.ClientID, 0);
		}
	}
	else if(event.isCbSelected(CTLSEL_TODO_TEMPLATE) && !Data.ID) {
		getCtrlData(CTLSEL_TODO_TEMPLATE, &Data.TemplateID);
		if(Data.TemplateID) {
			PPObjPrjTask     todo_obj;
			PrjTaskTbl::Rec  templ_rec;
			if(todo_obj.Search(Data.TemplateID, &templ_rec) > 0) {
				getCtrlData(CTLSEL_TODO_EMPLOYER, &Data.EmployerID);
				if(!Data.EmployerID)
					SetupPPObjCombo(this, CTLSEL_TODO_EMPLOYER, PPOBJ_PERSON, templ_rec.EmployerID, OLW_CANINSERT, (void *)PPPRK_EMPL);
				getCtrlData(CTLSEL_TODO_CLIENT, &Data.ClientID);
				if(!Data.ClientID)
					SetupPPObjCombo(this, CTLSEL_TODO_CLIENT, PPOBJ_PERSON, templ_rec.ClientID, OLW_CANINSERT, (void *)PPPRK_CLIENT);
				getCtrlData(CTLSEL_TODO_DLVRADDR, &Data.DlvrAddrID);
				if(!Data.DlvrAddrID)
					PsnObj.SetupDlvrLocCombo(this, CTLSEL_TODO_DLVRADDR, templ_rec.ClientID, templ_rec.DlvrAddrID);
				getCtrlData(CTL_TODO_DESCR, Data.Descr);
				if(Data.Descr[0] == 0)
					setCtrlData(CTL_TODO_DESCR, templ_rec.Descr);
				getCtrlData(CTL_TODO_AMOUNT, &Data.Amount);
				if(Data.Amount == 0)
					setCtrlData(CTL_TODO_AMOUNT, &templ_rec.Amount);
			}
		}
	}
	else if(event.isCbSelected(CTLSEL_TODO_PRJ)) {
		PPID   prj_id = getCtrlLong(CTLSEL_TODO_PRJ);
		SetupPPObjCombo(this, CTLSEL_TODO_PHASE, PPOBJ_PROJECT, 0, OLW_CANINSERT, (void *)NZOR(prj_id, -1));
		Data.ProjectID = prj_id;
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
							Data.Status = (int16)temp_id;
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

int PrjTaskDialog::setDTS(const PrjTaskTbl::Rec * pData)
{
	Data = *pData;

	SString buf;
	PPID   prj_id = 0;
	PPID   phase_id = 0;
	if(Data.ProjectID) {
		PPObjProject prj_obj;
		ProjectTbl::Rec prj_rec;
		if(prj_obj.Search(Data.ProjectID, &prj_rec) > 0) {
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
	setCtrlLong(CTL_TODO_ID, Data.ID); // @v7.6.10
	SetupPPObjCombo(this, CTLSEL_TODO_PRJ,   PPOBJ_PROJECT, prj_id,   OLW_CANINSERT, 0);
	SetupPPObjCombo(this, CTLSEL_TODO_PHASE, PPOBJ_PROJECT, phase_id, OLW_CANINSERT, (void *)NZOR(prj_id, -1));
	setCtrlData(CTL_TODO_CODE,        Data.Code);
	if(Data.Kind == TODOKIND_TASK && Data.LinkTaskID) {
		PrjTaskTbl::Rec  link_rec;
		if(PTCore.Search(Data.LinkTaskID, &link_rec) > 0)
			setCtrlData(CTL_TODO_LINKTASK, link_rec.Code);
	}
	setCtrlData(CTL_TODO_DT,          &Data.Dt);
	setCtrlData(CTL_TODO_TM,          &Data.Tm);
	setCtrlData(CTL_TODO_START,       &Data.StartDt);
	setCtrlData(CTL_TODO_STARTTM,     &Data.StartTm);
	setCtrlData(CTL_TODO_ESTFINISH,   &Data.EstFinishDt);
	setCtrlData(CTL_TODO_ESTFINISHTM, &Data.EstFinishTm);
	setCtrlData(CTL_TODO_FINISH,      &Data.FinishDt);
	setCtrlData(CTL_TODO_FINISHTM,    &Data.FinishTm);
	SetupPPObjCombo(this, CTLSEL_TODO_TEMPLATE, PPOBJ_PRJTASK, Data.TemplateID, 0, (void *)TODOKIND_TEMPLATE);
	SetupPersonCombo(this, CTLSEL_TODO_CREATOR,  Data.CreatorID,  OLW_CANINSERT, (PPID)PPPRK_EMPL,   0);
	SetupPersonCombo(this, CTLSEL_TODO_EMPLOYER, Data.EmployerID, OLW_CANINSERT, (PPID)PPPRK_EMPL,   0);
	SetupPersonCombo(this, CTLSEL_TODO_CLIENT,   Data.ClientID,   OLW_CANINSERT, (PPID)PPPRK_CLIENT, 0);
	PsnObj.SetupDlvrLocCombo(this, CTLSEL_TODO_DLVRADDR, Data.ClientID, Data.DlvrAddrID);
	SetupStringCombo(this, CTLSEL_TODO_PRIOR,  PPTXT_TODO_PRIOR,  Data.Priority);
	int    str_id = 0;
	if(Data.Kind == TODOKIND_TEMPLATE)
		str_id = PPTXT_TODOTEMPL_STATUS;
	else {
		str_id = PPTXT_TODO_STATUS;
		selectCtrl(CTL_TODO_START);
	}
	SetupStringCombo(this, CTLSEL_TODO_STATUS, str_id, Data.Status);
	setCtrlData(CTL_TODO_DESCR,   Data.Descr);
	setCtrlData(CTL_TODO_MEMO,    Data.Memo);
	setCtrlData(CTL_TODO_AMOUNT, &Data.Amount);
	{
		PPID accsheet = 0, billop = 0;
		ProjectTbl::Rec prj_rec;
		// @v10.6.4 MEMSZERO(prj_rec);
		for(billop = 0, prj_id = Data.ProjectID; !billop && PrjObj.Search(prj_id, &prj_rec) > 0; prj_id = prj_rec.ParentID)
			billop = prj_rec.BillOpID;
		if(!billop) {
			PPProjectConfig prj_cfg;
			MEMSZERO(prj_cfg);
			PPObjProject::ReadConfig(&prj_cfg);
			billop = prj_cfg.BillOpID;
		}
		if(billop) {
			PPOprKind opr_kind;
			GetOpData(billop, &opr_kind);
			accsheet = opr_kind.AccSheetID;
		}
		Data.BillArID = (accsheet) ? Data.BillArID : 0;
		SetupArCombo(this, CTLSEL_TODO_BILLAR, Data.BillArID, OLW_LOADDEFONOPEN|OLW_CANINSERT, accsheet, sacfDisableIfZeroSheet);
	}
	return 1;
}

int PrjTaskDialog::getDTS(PrjTaskTbl::Rec * pData)
{
	int    ok = 1;
	uint   sel = 0;
	char   linktask_code[32];
	PPID   temp_id = 0;
	PPID   prj_id = getCtrlLong(CTLSEL_TODO_PRJ);
	PPID   phase_id = getCtrlLong(CTLSEL_TODO_PHASE);
	Data.ProjectID = (phase_id && prj_id) ? phase_id : prj_id;
	getCtrlData(CTL_TODO_CODE, Data.Code);
	if(Data.Kind == TODOKIND_TASK) {
		getCtrlData(sel = CTL_TODO_LINKTASK, linktask_code);
		if(linktask_code[0]) {
			int    r;
			THROW_PP_S(strcmp(Data.Code, linktask_code), PPERR_DUPTASKCODE, linktask_code);
			THROW_PP_S(r = PTCore.GetSingleByCode(TODOKIND_TASK, linktask_code, &Data.LinkTaskID), PPERR_DUPTASKCODE, linktask_code);
			THROW_PP_S(r > 0, PPERR_UNDEFTASKWCODE, linktask_code);
		}
		else
			Data.LinkTaskID = 0;
	}
	getCtrlData(sel = CTL_TODO_DT, &Data.Dt);
	THROW_SL(checkdate(Data.Dt, 1));
	getCtrlData(sel = CTL_TODO_TM, &Data.Tm);
	getCtrlData(sel = CTL_TODO_START, &Data.StartDt);
	THROW_SL(checkdate(Data.StartDt, 1));
	getCtrlData(sel = CTL_TODO_STARTTM, &Data.StartTm);
	getCtrlData(sel = CTL_TODO_ESTFINISH, &Data.EstFinishDt);
	THROW_SL(checkdate(Data.EstFinishDt, 1));
	getCtrlData(sel = CTL_TODO_ESTFINISHTM, &Data.EstFinishTm);
	getCtrlData(sel = CTL_TODO_FINISH, &Data.FinishDt);
	THROW_SL(checkdate(Data.FinishDt, 1));
	getCtrlData(CTL_TODO_FINISHTM, &Data.FinishTm);
	getCtrlData(CTLSEL_TODO_TEMPLATE, &Data.TemplateID);
	getCtrlData(CTLSEL_TODO_CREATOR,  &Data.CreatorID);
	getCtrlData(CTLSEL_TODO_EMPLOYER, &Data.EmployerID);
	getCtrlData(CTLSEL_TODO_CLIENT,   &Data.ClientID);
	getCtrlData(CTLSEL_TODO_DLVRADDR, &Data.DlvrAddrID);
	if(getCtrlData(CTLSEL_TODO_PRIOR, &(temp_id = 0)))
		Data.Priority = (int16)temp_id;
	if(getCtrlData(CTLSEL_TODO_STATUS, &(temp_id = 0)))
		Data.Status = (int16)temp_id;
	getCtrlData(CTL_TODO_DESCR,   Data.Descr);
	getCtrlData(CTL_TODO_MEMO,    Data.Memo);
	getCtrlData(CTL_TODO_AMOUNT, &Data.Amount);
	getCtrlData(CTLSEL_TODO_BILLAR, &Data.BillArID);
	ASSIGN_PTR(pData, Data);
	CATCHZOKPPERRBYDLG
	return ok;
}

int SLAPI PPObjPrjTask::EditDialog(PrjTaskTbl::Rec * pRec)
{
	uint   dlg_id = 0;
	if(pRec->Kind == TODOKIND_TEMPLATE)
		dlg_id = DLG_TODOTEMPL;
	else
		dlg_id = DLG_TODO;
	DIALOG_PROC_BODY_P1(PrjTaskDialog, dlg_id, pRec);
}

int SLAPI PPObjPrjTask::Edit(PPID * pID, void * extraPtr)
{
	const  long extra_param = reinterpret_cast<long>(extraPtr);
	int    ok = cmCancel, r = 0, is_new = 0, task_finished = 0;
	PPID   prev_employer = 0, parent_prj = 0, client = 0;
	PPID   employer = 0;
	PrjTaskTbl::Rec rec;
	// @v10.6.4 MEMSZERO(rec);
	THROW(CheckRightsModByID(pID));
	if(*pID) {
		THROW(Search(*pID, &rec) > 0);
		prev_employer = rec.EmployerID;
	}
	else {
		if(extra_param & PRJTASKBIAS_CLIENT)
			client = extra_param & ~(PRJTASKBIAS_CLIENT|PRJTASKBIAS_EMPLOYER|PRJTASKBIAS_TEMPLATE);
		else if(extra_param & PRJTASKBIAS_EMPLOYER)
			employer = extra_param & ~(PRJTASKBIAS_CLIENT|PRJTASKBIAS_EMPLOYER|PRJTASKBIAS_TEMPLATE);
		else
			parent_prj = extra_param & ~(PRJTASKBIAS_CLIENT|PRJTASKBIAS_EMPLOYER|PRJTASKBIAS_TEMPLATE);
		int    kind = (extra_param & PRJTASKBIAS_TEMPLATE) ? TODOKIND_TEMPLATE : TODOKIND_TASK;
		InitPacket(&rec, kind, parent_prj, client, employer, 1);
		is_new = 1;
	}
	task_finished = BIN(rec.Status == TODOSTTS_COMPLETED && rec.FinishDt != ZERODATE);
	while(ok != cmOK && (r = EditDialog(&rec)) > 0) {
		if(PutPacket(pID, &rec, 1))
			ok = cmOK;
		else
			PPError();
	}
	if(r == 0)
		ok = 0;
	CATCHZOKPPERR
	if(rec.Kind == TODOKIND_TASK) {
		//
		// @todo ѕлохой блок. Ќеобходимо переработать.
		//
		int    output_to_status_win = 0;
		int    employer_changed = BIN(prev_employer && prev_employer != rec.EmployerID);
		PPObjPerson::GetCurUserPerson(&employer, 0);
		int    employer_equal = BIN(employer && employer == rec.EmployerID);
		if(is_new && ok == cmOK && employer_equal) {
			rec.Flags |= TODOF_OPENEDBYEMPL;
			PutPacket(pID, &rec, 1);
   			output_to_status_win = 1;
		}
		else if(!is_new && ok != 0 && Search(*pID, &rec) > 0) {
			employer = 0; // @v10.3.2 PPID employer-->employer
			SysJournal * p_sj = DS.GetTLA().P_SysJ;
			rec.OpenCount++;
			rec.Flags |= TODOF_ACTIONVIEWED;
			if(p_sj && employer_changed) {
				PPID   user_id = 0;
				PPIDArray act_list;
				act_list.add(PPACN_OBJVIEWED);
				GetUserByPerson(rec.EmployerID, &user_id);
				SETFLAG(rec.Flags, TODOF_OPENEDBYEMPL,
					employer_equal || p_sj->IsEventExists(PPOBJ_PRJTASK, rec.ID, user_id, &act_list) > 0);
			}
			else if(employer_equal)
				rec.Flags |= TODOF_OPENEDBYEMPL;
			PutPacket(pID, &rec, 1);
			output_to_status_win = 1;
		}
		//
		// создаем такую же задачу по шаблону, если текуща€ имеет статус
		// выполнена и в шаблоне указано - создавать новую задачу через опред. период
		//
		if(!task_finished && rec.TemplateID && rec.Status == TODOSTTS_COMPLETED && rec.FinishDt != ZERODATE) {
			PrjTaskTbl::Rec templ_rec;
			// @v10.6.4 MEMSZERO(templ_rec);
			if(Search(rec.TemplateID, &templ_rec) > 0 && templ_rec.DrPrd == PRD_REPEATAFTERPRD) {
				PPTransaction tra(1);
				if(!!tra) {
					PPID   new_task_id = 0;
					PrjTaskTbl::Rec new_task;
					DateRepeating dr = *reinterpret_cast<const DateRepeating *>(&templ_rec.DrPrd);
					LDATE  dt = (dr.Dtl.RA.AfterStart == 0) ? rec.FinishDt : ((rec.StartDt == ZERODATE) ? rec.Dt : rec.StartDt);
					// @v10.6.4 MEMSZERO(new_task);
					plusperiod(&dt, dr.RepeatKind, dr.Dtl.RA.NumPrd, 0);
					memzero(rec.Code, sizeof(rec.Code));
					InitPacketByTemplate(&rec, dt, &new_task, 0);
					new_task.StartTm = rec.StartTm;
					new_task.TemplateID = rec.TemplateID;
					new_task.Amount = rec.Amount;
					STRNSCPY(new_task.Memo, rec.Memo);
					PutPacket(&new_task_id, &new_task, 0);
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

int SLAPI PPObjPrjTask::AddBySample(PPID * pID, PPID sampleID)
{
	int    ok = -1;
	if(sampleID > 0) {
		PrjTaskTbl::Rec sample_rec, rec;
		THROW(CheckRights(PPR_INS));
		THROW(Search(sampleID, &sample_rec) > 0);
		rec = sample_rec;
		rec.ID = 0;
		rec.Status = TODOSTTS_NEW;
		rec.LinkTaskID = LinkTaskID;
		PPObjPerson::GetCurUserPerson(&rec.CreatorID, 0);
		getcurdatetime(&rec.Dt, &rec.Tm);
		{
			PPProjectConfig cfg;
			PPObjProject::ReadConfig(&cfg);
			PPObjOpCounter opc_obj;
			long   counter = 0;
			PPID   cntr_id = 0;
			if(rec.Kind == TODOKIND_TASK)
				cntr_id = cfg.TaskCntrID;
			else if(rec.Kind == TODOKIND_TEMPLATE)
				cntr_id = cfg.TemplCntrID;
			if(cntr_id)
				opc_obj.GetCode(cntr_id, &counter, rec.Code, sizeof(rec.Code), 0, 1);
		}
		while(ok <= 0 && (ok = EditDialog(&rec)) > 0)
			if(PutPacket(pID, &rec, 1))
				ok = 1;
			else
				ok = PPErrorZ();
	}
	CATCHZOKPPERR
	return ok;
}

int SLAPI PPObjPrjTask::Browse(void * extraPtr)
{
	return ViewPrjTask(0);
}

int SLAPI PPObjPrjTask::DeleteObj(PPID id)
{
	return PutPacket(&id, 0, 0);
}

int SLAPI PPObjPrjTask::GetLinkTasks(PPID taskID, PPIDArray * pAry)
{
	int    ok  = -1;
	if(pAry) {
		PrjTaskTbl::Key6  k6;
		BExtQuery q(P_Tbl, 6);
		q.select(P_Tbl->ID, P_Tbl->LinkTaskID, 0L).where(P_Tbl->LinkTaskID == taskID);
		MEMSZERO(k6);
		k6.LinkTaskID = taskID;
		pAry->freeAll();
		for(q.initIteration(0, &k6, spGe); q.nextIteration() > 0;) {
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

// virtual
int ViewTasksDialog::setupList()
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
		PrjTaskTbl::Rec rec;
		// @v10.6.4 MEMSZERO(rec);
		if(ObjPrjT.GetPacket(id, &rec) > 0) {
			ObjPrjT.EditDialog(&rec);
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

typedef TSVector <LostPrjTPersonItem> LostPrjTPersonArray; // @v9.8.4 TSArray-->TSVector

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
				if(ListBoxSelDialog(PPOBJ_PERSON, &psn_id, reinterpret_cast<void *>((creator || employer) ? PPPRK_EMPL : PPPRK_CLIENT)) > 0) {
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
				enableCommand(cmResolveCreator,      0);
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
		word_notf.CopyFrom("[ID=%ld] не найден; ").ToOem();
		(buf = rWord).CatChar(' ').Cat(word_notf);
		msg.Printf(buf.cptr(), id);
		rBuf.Cat(msg);
		ok = 1;
	}
	return ok;
}

// virtual
int RestoreLostPrjTPersonDlg::setupList()
{
	int    ok = 1;
	SString temp_buf;
	SString creator_word, employer_word, client_word;
	// @v10.6.3 PPGetWord(PPWORD_CREATOR,  0, creator_word);
	// @v10.6.3 PPGetWord(PPWORD_EMPLOYER, 0, employer_word);
	PPLoadString("creator", creator_word); // @v10.6.3
	PPLoadString("executor", employer_word); // @v10.6.3
	PPLoadString("client", client_word);
	for(uint i = 0; i < Data.getCount(); i++) {
		LostPrjTPersonItem * p_item = &Data.at(i);
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
		PrjTaskTbl prjt_tbl;
		if(cm == cmViewTasksByCreator) {
			idx = 0;
			MEMSZERO(k.k0);
			k_ = (void *)&k.k0;
			dbq = ppcheckfiltid(dbq, prjt_tbl.CreatorID, pItem->CreatorID);
		}
		else if(cm == cmViewTasksByEmployer) {
			idx = 4;
			MEMSZERO(k.k4);
			k.k4.EmployerID = pItem->EmployerID;
			k_ = (void *)&k.k4;
			dbq = ppcheckfiltid(dbq, prjt_tbl.EmployerID, pItem->EmployerID);
		}
		else {
			idx = 5;
			MEMSZERO(k.k5);
			k.k5.ClientID = pItem->ClientID;
			k_ = (void *)&k.k5;
			dbq = ppcheckfiltid(dbq, prjt_tbl.ClientID, pItem->ClientID);
		}
		p_q = new BExtQuery(&prjt_tbl, idx);
		p_q->select(prjt_tbl.ID, prjt_tbl.Descr, 0).where(*dbq);
		for(p_q->initIteration(0, &k_); p_q->nextIteration() > 0;)
			list.Add(prjt_tbl.data.ID, prjt_tbl.data.Descr);
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

int SLAPI PPObjPrjTask::ResolveAbsencePersonHelper_(PPID newID, PPID prevID, int todoPerson)
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
		for(p_q->initIteration(0, &k0); p_q->nextIteration() > 0;)
			todo_list.add(P_Tbl->data.ID);
		for(uint i = 0; i < todo_list.getCount(); i++) {
			PrjTaskTbl::Rec prjt_rec;
			// @v10.6.4 MEMSZERO(prjt_rec);
			THROW(GetPacket(todo_list.at(i), &prjt_rec) > 0);
			if(todoPerson == TODOPSN_CREATOR)
				prjt_rec.CreatorID  = newID;
			if(todoPerson == TODOPSN_EMPLOYER)
				prjt_rec.EmployerID  = newID;
			if(todoPerson == TODOPSN_CLIENT)
				prjt_rec.ClientID  = newID;
			THROW(PutPacket(&prjt_rec.ID, &prjt_rec, 0));
		}
		ok = 1;
	}
	CATCHZOK
	BExtQuery::ZDelete(&p_q);
	return ok;
}

// static
int SLAPI PPObjPrjTask::RecoverAbsencePerson()
{
	int    ok = 1;
	LostPrjTPersonArray list;
	PrjTaskTbl::Key0 k0;
	PPObjPrjTask obj_prjt;
	PPObjPerson obj_psn;
	RestoreLostPrjTPersonDlg * p_dlg = 0;
	BExtQuery * p_q = 0;
#define PSN_NOTF_SRCH(notfFlag, personID, offs) if(notfFlag) notfFlag = (list.lsearch(&personID, 0, PTR_CMPFUNC(long), offs) > 0) ? 0 : 1
	MEMSZERO(k0);
	THROW_MEM(p_q = new BExtQuery(obj_prjt.P_Tbl, 0));
	p_q->select(obj_prjt.P_Tbl->ID, obj_prjt.P_Tbl->CreatorID, obj_prjt.P_Tbl->EmployerID, obj_prjt.P_Tbl->ClientID, 0L);
	PPWait(1);
	for(p_q->initIteration(0, &k0); p_q->nextIteration() > 0;) {
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
	PPWait(0);
	if(list.getCount()) {
		THROW(CheckDialogPtr(&(p_dlg = new RestoreLostPrjTPersonDlg())));
		p_dlg->setDTS(&list);
		if(ExecView(p_dlg) == cmOK) {
			p_dlg->getDTS(&list);
			PPWait(1);
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
	PPWait(0);
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
	delete (PPObjProject *) Extra[0].Ptr;
}

int PPALDD_Project::InitData(PPFilt & rFilt, long rsrv)
{
	int    ok = -1;
	if(rFilt.ID == H.ID)
		ok = DlRtm::InitData(rFilt, rsrv);
	else {
		MEMSZERO(H);
		H.ID = rFilt.ID;
		ProjectTbl::Rec rec;
		PPObjProject * p_prj_obj = static_cast<PPObjProject *>(Extra[0].Ptr);
		if(p_prj_obj->Search(rFilt.ID, &rec) > 0) {
			SString full_name;
			H.ID          = rec.ID;
			H.Kind        = rec.Kind;
			H.ParentID    = rec.ParentID;
			H.Dt          = rec.Dt;
			H.BeginDt     = rec.BeginDt;
			H.EstFinishDt = rec.EstFinishDt;
			H.FinishDt    = rec.FinishDt;
			H.MngrID      = rec.MngrID;
			H.ClientID    = rec.ClientID;
			H.TemplateID  = rec.TemplateID;
			H.Status      = rec.Status;
			H.Flags       = rec.Flags;
			STRNSCPY(H.Name, rec.Name);
			STRNSCPY(H.Code, rec.Code);
			STRNSCPY(H.Descr, rec.Descr);
			STRNSCPY(H.Memo, rec.Memo);
			p_prj_obj->GetFullName(rec.ID, full_name);
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

void PPALDD_ProjectView::Destroy()
{
	DESTROY_PPVIEW_ALDD(Project);
}
