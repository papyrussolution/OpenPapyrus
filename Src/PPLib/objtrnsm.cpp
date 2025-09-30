// OBJTRNSM.CPP
// Copyright (c) A.Sobolev 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022, 2023, 2024, 2025
// @codepage UTF-8
// Передача объектов между разделами БД
//
#include <pp.h>
#pragma hdrstop

#define OT_MAGIC 0x534F5050L // "PPOS"

SVerT PPSession::GetMinCompatVersion() const
{
	//
	// @attention При изменении формата передачи данных необходимо установить здесь минимальную
	//   версию системы, с которой пакеты паредачи данных могут быть приняты.
	//
	static const SVerT __MinCompatVer(12, 0, 0);
		// @v11.0.4 10.9.2-->11.0.4
		// @v11.1.12 11.0.4-->11.1.12
		// @v11.2.0 11.1.12-->11.2.0
		// @v12.0.0 11.2.0-->12.0.0
	return __MinCompatVer;
}
//
// DB Exchange Config (PPDBXchgConfig)
//

/*static*/const long PPObjectTransmit::DefaultPriority = 1000;
/*static*/const long PPObjectTransmit::DependedPriority = 2000;

struct RestoreStackItem {
	PPObjID Oi;
	PPID   DbID;
};

/*static*/int PPObjectTransmit::EditConfig()
{
	class DbExchangeCfgDialog : public TDialog {
		DECL_DIALOG_DATA(PPDBXchgConfig);
	public:
		DbExchangeCfgDialog() : TDialog(DLG_DBXCHGCFG)
		{
		}
		DECL_DIALOG_SETDTS()
		{
			if(!RVALUEPTR(Data, pData))
				MEMSZERO(Data);
			AddClusterAssoc(CTL_DBXCHGCFG_FLAGS,  0, DBDXF_SKIPINCOMPLBILL);
			AddClusterAssoc(CTL_DBXCHGCFG_FLAGS,  1, DBDXF_CALCTOTALDEFICITE);
			AddClusterAssoc(CTL_DBXCHGCFG_FLAGS,  2, DBDXF_TURNTOTALDEFICITE);
			AddClusterAssoc(CTL_DBXCHGCFG_FLAGS,  3, DBDXF_LINKRETBILLS);
			AddClusterAssoc(CTL_DBXCHGCFG_FLAGS,  4, DBDXF_IGNOREACK);
			AddClusterAssoc(CTL_DBXCHGCFG_FLAGS,  5, DBDXF_TWOPASSRCV);
			AddClusterAssoc(CTL_DBXCHGCFG_FLAGS,  6, DBDXF_SENDINVWROFFBILLS);
			AddClusterAssoc(CTL_DBXCHGCFG_FLAGS,  7, DBDXF_NOUPDGOODS);
			AddClusterAssoc(CTL_DBXCHGCFG_FLAGS,  8, DBDXF_IMPOPSYNC);
			AddClusterAssoc(CTL_DBXCHGCFG_FLAGS,  9, DBDXF_SYNCSCARDWOCHECKS);
			AddClusterAssoc(CTL_DBXCHGCFG_FLAGS, 10, DBDXF_DONTCVTTOTALDIS);
			AddClusterAssoc(CTL_DBXCHGCFG_FLAGS, 11, DBDXF_SENDCSESSION);
			AddClusterAssoc(CTL_DBXCHGCFG_FLAGS, 12, DBDXF_PACKFILES);
			AddClusterAssoc(CTL_DBXCHGCFG_FLAGS, 13, DBDXF_IGNOBJUNIFY);
			AddClusterAssoc(CTL_DBXCHGCFG_FLAGS, 14, DBDXF_UNITEINVDUPREC);
			SetClusterData(CTL_DBXCHGCFG_FLAGS, Data.Flags);

			AddClusterAssoc(CTL_DBXCHGCFG_CHARRYF, 0, DBDXF_CHARRY_PRICEQCOST);
			AddClusterAssoc(CTL_DBXCHGCFG_CHARRYF, 1, DBDXF_CHARRY_GIDASARCODE);
			AddClusterAssoc(CTL_DBXCHGCFG_CHARRYF, 2, DBDXF_SUBSTDEFICITGOODS);
			AddClusterAssoc(CTL_DBXCHGCFG_CHARRYF, 3, DBDXF_NOCOMMITBYDEF);
			AddClusterAssoc(CTL_DBXCHGCFG_CHARRYF, 4, DBDXF_DESTROYQUEUEBYDEF);
			AddClusterAssoc(CTL_DBXCHGCFG_CHARRYF, 5, DBDXF_DONTLOGOBJUPD);
			AddClusterAssoc(CTL_DBXCHGCFG_CHARRYF, 6, DBDXF_SENDTAGATTCHM);
			AddClusterAssoc(CTL_DBXCHGCFG_CHARRYF, 7, DBDXF_SYNCUSRANDGRPS); // @v10.1.5
			SetClusterData(CTL_DBXCHGCFG_CHARRYF, Data.Flags);

			AddClusterAssocDef(CTL_DBXCHGCFG_RLZORD,  0, RLZORD_UNDEF);
			AddClusterAssoc(CTL_DBXCHGCFG_RLZORD,  1, RLZORD_FIFO);
			AddClusterAssoc(CTL_DBXCHGCFG_RLZORD,  2, RLZORD_LIFO);
			SetClusterData(CTL_DBXCHGCFG_RLZORD, Data.RealizeOrder);
			SetupPPObjCombo(this, CTLSEL_DBXCHGCFG_ONELOC, PPOBJ_LOCATION, Data.OneRcvLocID, 0);
			{
				PPIDArray op_type_list;
				op_type_list.addzlist(PPOPT_GOODSRECEIPT, PPOPT_DRAFTRECEIPT, 0);
				SetupOprKindCombo(this, CTLSEL_DBXCHGCFG_DROP, Data.DfctRcptOpID, OLW_CANINSERT, &op_type_list, 0);
			}
			SetupPPObjCombo(this, CTLSEL_DBXCHGCFG_SPCSGG, PPOBJ_GOODSGROUP, Data.SpcSubstGoodsGrpID, OLW_CANINSERT|OLW_CANSELUPLEVEL);
			setCtrlReal(CTL_DBXCHGCFG_PCTADD, R2(fdiv100i(Data.PctAdd)));
			SetupCtrls(Data.Flags);
			return 1;
		}
		DECL_DIALOG_GETDTS()
		{
			int    ok = -1;
			uint   sel = 0;
			GetClusterData(CTL_DBXCHGCFG_FLAGS,   &Data.Flags);
			GetClusterData(CTL_DBXCHGCFG_CHARRYF, &Data.Flags);
			GetClusterData(CTL_DBXCHGCFG_RLZORD,  &Data.RealizeOrder);
			getCtrlData(CTLSEL_DBXCHGCFG_ONELOC,  &Data.OneRcvLocID);
			getCtrlData(CTLSEL_DBXCHGCFG_DROP,    &Data.DfctRcptOpID);
			sel = CTL_DBXCHGCFG_DROP;
			if(Data.DfctRcptOpID) {
				PPOprKind op_rec;
				THROW(GetOpData(Data.DfctRcptOpID, &op_rec) > 0);
				THROW_PP(op_rec.AccSheetID == GetSupplAccSheet(), PPERR_DFCTRCPTOPACSNEQSUPPLACS);
			}
			getCtrlData(CTLSEL_DBXCHGCFG_SPCSGG, &Data.SpcSubstGoodsGrpID);
			Data.PctAdd = (long)(getCtrlReal(CTL_DBXCHGCFG_PCTADD) * 100.0);
			ok = 1;
			ASSIGN_PTR(pData, Data);
			CATCHZOKPPERRBYDLG
			return ok;
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isCmd(cmClusterClk)) {
				long flags = GetClusterData(CTL_DBXCHGCFG_FLAGS);
				SetupCtrls(flags);
			}
			else if(event.isCbSelected(CTLSEL_DBXCHGCFG_DROP)) {
				long flags = GetClusterData(CTL_DBXCHGCFG_FLAGS);
				getCtrlData(CTLSEL_DBXCHGCFG_DROP, &Data.DfctRcptOpID);
				SetupCtrls(flags);
			}
			else
				return;
			clearEvent(event);
		}
		void   SetupCtrls(long flags)
		{
			const int disable = (!(flags & DBDXF_TURNTOTALDEFICITE) && (flags & DBDXF_CALCTOTALDEFICITE)) ? 0 : 1;
			DisableClusterItem(CTL_DBXCHGCFG_CHARRYF, 2, disable);
			if(disable) {
				flags = GetClusterData(CTL_DBXCHGCFG_CHARRYF);
				flags &= ~DBDXF_SUBSTDEFICITGOODS;
				SetClusterData(CTL_DBXCHGCFG_CHARRYF, flags);
			}
			DisableClusterItem(CTL_DBXCHGCFG_FLAGS, 5, BIN(Data.DfctRcptOpID && GetOpType(Data.DfctRcptOpID) != PPOPT_GOODSRECEIPT));
		}
	};
	int    ok = -1;
	int    is_new = 0;
	PPDBXchgConfig cfg;
	DbExchangeCfgDialog * p_dlg = new DbExchangeCfgDialog();
	THROW(CheckDialogPtr(&p_dlg));
	MEMSZERO(cfg);
	THROW(CheckCfgRights(PPCFGOBJ_DBXCHNG, PPR_READ, 0));
	THROW(is_new = ReadConfig(&cfg));
	p_dlg->setDTS(&cfg);
	while(ok < 0 && ExecView(p_dlg) == cmOK) {
		if(p_dlg->getDTS(&cfg)) {
			THROW(CheckCfgRights(PPCFGOBJ_DBXCHNG, PPR_MOD, 0));
			THROW(WriteConfig(&cfg, 1));
			ok = 1;
		}
	}
	CATCHZOKPPERR
	delete p_dlg;
	return ok;
}

struct __PPDBXchgConfig {  // @persistent @store(PropertyTbl)
	PPID   Tag;            // Const=PPOBJ_CONFIG
	PPID   ID;             // Const=PPCFG_MAIN
	PPID   Prop;           // Const=PPPRP_DBXCHGCFG
	char   Reserve1[40];   // @reserve // @v10.0.10 [44]-->[40]
	PPID   SpcSubstGoodsGrpID; // @v10.0.10 Товарная группа для специальной подстановки при дефиците
	PPID   DfctRcptOpID;       // Вид операции приходования дефицита.
	long   CharryOutCounter;   // Счетчик исходящих файлов Charry
	int16  Reserve2;       //
	int16  RealizeOrder;   //
	long   PctAdd;         //
	long   OneRcvLocID;    //
	long   Flags;          //
	long   Reserve3;       //
};

/*static*/int FASTCALL PPObjectTransmit::WriteConfig(const PPDBXchgConfig * pCfg, int use_ta)
{
	int    ok = 1;
	Reference * p_ref = PPRef;
	int    is_new = 1;
	__PPDBXchgConfig p, prev_cfg;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(p_ref->GetPropMainConfig(PPPRP_DBXCHGCFG, &prev_cfg, sizeof(prev_cfg)) > 0)
			is_new = 0;
		MEMSZERO(p);
		p.RealizeOrder = pCfg->RealizeOrder;
		p.OneRcvLocID  = pCfg->OneRcvLocID;
		p.PctAdd = pCfg->PctAdd;
		p.Flags  = pCfg->Flags;
		p.CharryOutCounter = pCfg->CharryOutCounter;
		p.SpcSubstGoodsGrpID = pCfg->SpcSubstGoodsGrpID; // @v10.0.10
		p.DfctRcptOpID = pCfg->DfctRcptOpID;
		THROW(p_ref->PutProp(PPOBJ_CONFIG, PPCFG_MAIN, PPPRP_DBXCHGCFG, &p, 0, 0));
		DS.LogAction(is_new ? PPACN_CONFIGCREATED : PPACN_CONFIGUPDATED, PPCFGOBJ_DBXCHNG, 0, 0, 0);
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

/*static*/int FASTCALL PPObjectTransmit::ReadConfig(PPDBXchgConfig * pCfg)
{
	__PPDBXchgConfig p;
	int    r = PPRef->GetPropMainConfig(PPPRP_DBXCHGCFG, &p, sizeof(p));
	memzero(pCfg, sizeof(*pCfg));
	if(r > 0) {
		pCfg->RealizeOrder = p.RealizeOrder;
		pCfg->OneRcvLocID = p.OneRcvLocID;
		pCfg->PctAdd      = p.PctAdd;
		pCfg->Flags       = p.Flags;
		pCfg->CharryOutCounter = p.CharryOutCounter;
		pCfg->SpcSubstGoodsGrpID = p.SpcSubstGoodsGrpID; // @v10.0.10
		pCfg->DfctRcptOpID = p.DfctRcptOpID;
	}
	return r;
}

/*static*/int PPObjectTransmit::IncrementCharryOutCounter()
{
	int    ok = 1;
	PPDBXchgConfig cfg;
	{
		PPTransaction tra(1);
		THROW(tra);
		ReadConfig(&cfg);
		cfg.CharryOutCounter++;
		THROW(WriteConfig(&cfg, 0));
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}
//
//
//
ObjTransmitParam::ObjTransmitParam() : Since_(ZERODATETIME), UpdProtocol(0), Flags(0), TrnsmFlags(0)
{
}

void ObjTransmitParam::Init()
{
	DestDBDivList.Z();
	DestDBDivList.InitEmpty();
	ObjList.Z();
	ObjList.InitEmpty();
	Since_.Z();
	UpdProtocol = 0;
	Flags       = 0;
	TrnsmFlags  = 0;
}

int ObjTransmitParam::Read(SBuffer & rBuf, long)
{
	int    ok = -1;
	if(rBuf.GetAvailableSize()) {
		PPIDArray dbdiv_list, obj_list;
		if(rBuf.Read(&dbdiv_list, 0) && rBuf.Read(&obj_list, 0) && rBuf.Read(Since_) &&
			rBuf.Read(&UpdProtocol, sizeof(UpdProtocol)) && rBuf.Read(Flags) && rBuf.Read(TrnsmFlags)) {
			DestDBDivList.Set(&dbdiv_list);
			ObjList.Set(&obj_list);
			ok = 1;
		}
		else
			ok = PPSetErrorSLib();
	}
	return ok;
}

int ObjTransmitParam::Write(SBuffer & rBuf, long) const
{
	const PPIDArray * p_dbdiv_list = &DestDBDivList.Get();
	const PPIDArray * p_obj_list = &ObjList.Get();
	if(rBuf.Write(p_dbdiv_list, 0) && rBuf.Write(p_obj_list, 0) && rBuf.Write(Since_) &&
		rBuf.Write(&UpdProtocol, sizeof(UpdProtocol)) && rBuf.Write(Flags) && rBuf.Write(TrnsmFlags))
		return 1;
	else
		return PPSetErrorSLib();
}
//
//
//
ObjReceiveParam::ObjReceiveParam() : Flags(0), P_SyncCmpTbl(0)
{
}

void ObjReceiveParam::Init()
{
	SenderDbDivList.Z();
	Flags |= (fGetFromOutSrcr | fClearInpBefore);
	SsOnlyFileNames.Z(); // @v10.6.8
}

int ObjReceiveParam::CheckDbDivID(PPID id) const
	{ return BIN(!SenderDbDivList.getCount() || SenderDbDivList.lsearch(id)); }
int ObjReceiveParam::Write(SBuffer & rBuf, long) const
	{ return (rBuf.Write(Flags) && rBuf.Write(&SenderDbDivList, 0)) ? 1 : PPSetErrorSLib(); }

int ObjReceiveParam::Read(SBuffer & rBuf, long)
{
	int    ok = -1;
	if(rBuf.GetAvailableSize())
		if(rBuf.Read(Flags) && rBuf.Read(&SenderDbDivList, 0))
			ok = 1;
		else
			ok = PPSetErrorSLib();
	return ok;
}
//
//
//
ObjTransmContext::ObjTransmContext(uint ctrFlags, PPLogger * pLogger) : State(0), P_Ot(0), P_Btd(0), P_ThisDbDivPack(0), P_SrcDbDivPack(0), P_DestDbDivPack(0),
	P_Rb(0), P_ForceRestoreObj(0), Flags(0), Extra(0), LastStreamId(-1), TransmitSince(ZERODATETIME), P_Gra(0)
{
	MEMSZERO(Cfg);
	if(pLogger) {
		P_Logger = pLogger;
		State |= stOuterLogger;
	}
	else
		P_Logger = new PPLogger((ctrFlags & ctrfDisableLogWindow) ? PPLogger::fDisableWindow : 0);
}

ObjTransmContext::~ObjTransmContext()
{
	delete P_ForceRestoreObj;
	delete P_Gra;
	P_Ot = 0;
	if(!(State & stOuterLogger)) {
		ZDELETE(P_Logger);
	}
}

int ObjTransmContext::ResetOuterLogger()
{
	int    ok = -1;
	if(State & stOuterLogger) {
		P_Logger = new PPLogger;
		State &= ~stOuterLogger;
		ok = 1;
	}
	return ok;
}

int ObjTransmContext::OutReceivingMsg(const char * pMsg) { return P_Logger ? P_Logger->Log(pMsg) : 0; }
int ObjTransmContext::Output(const char * pText) { return P_Logger ? P_Logger->Log(pText) : 0; }
int ObjTransmContext::OutputLastError() { return P_Logger ? P_Logger->LogLastError() : 0; }
int ObjTransmContext::OutputString(uint strId, const char * pAddedInfo) { return P_Logger ? P_Logger->LogString(strId, pAddedInfo) : 0; }
int ObjTransmContext::OutputAcceptMsg(PPID objType, PPID objID, int upd) { return P_Logger ? P_Logger->LogAcceptMsg(objType, objID, upd) : 0; }

int ObjTransmContext::OutputAcceptErrMsg(uint msgID, PPID objID, const char * pObjName)
{
	SString msg_buf, err_msg, fmt_buf;
	PPGetLastErrorMessage(1, err_msg);
	PPLoadText(msgID, fmt_buf);
	return OutReceivingMsg(msg_buf.Printf(fmt_buf, objID, pObjName, err_msg.cptr()));
}

int ObjTransmContext::OutputAcceptObjErrMsg(PPID objType, PPID objID, const char * pObjName)
{
	SString msg_buf, err_msg, fmt_buf, obj_title;
	PPGetLastErrorMessage(1, err_msg);
	PPLoadText(PPTXT_ERRACCEPTOBJECT, fmt_buf);
	GetObjectTitle(objType, obj_title);
	return OutReceivingMsg(msg_buf.Printf(fmt_buf, obj_title.cptr(), objID, pObjName, err_msg.cptr()));
}

int ObjTransmContext::GetPrevRestoredObj(PPObjID * pOi) const
{
	int    ok = 0;
	const PPObjectTransmit::RestoreObjBlock * p_rb = static_cast<const PPObjectTransmit::RestoreObjBlock *>(P_Rb);
	if(p_rb) {
		const uint pntr = p_rb->S.getPointer();
		if(pntr > 1) {
			const RestoreStackItem * p_rsi = static_cast<const RestoreStackItem *>(p_rb->S.at(pntr-2));
			if(p_rsi) {
				ASSIGN_PTR(pOi, p_rsi->Oi);
				ok = 1;
			}
		}
	}
	return ok;
}

int ObjTransmContext::ForceRestore(PPObjID oi)
{
	int    ok = -1;
	if(oi.IsFullyDefined()) {
		SETIFZ(P_ForceRestoreObj, new PPObjIDArray);
		if(P_ForceRestoreObj) {
			P_ForceRestoreObj->Add(oi.Obj, oi.Id);
			ok = 1;
		}
	}
	return ok;
}

int ObjTransmContext::GetPrimaryObjID(PPID objType, PPID foreignID, PPID * pPrimID)
{
	int    ok = -1, r;
	PPID   prim_id = 0;
	if(P_Rb && P_SrcDbDivPack) {
		const PPObjectTransmit::RestoreObjBlock * p_rb = static_cast<const PPObjectTransmit::RestoreObjBlock *>(P_Rb);
		PPObjectTransmit * p_ot = p_rb->P_Ot;
		if(p_ot) {
			const  PPID src_db_div_id = P_SrcDbDivPack->Rec.ID;
			ObjSyncTbl::Rec rec;
			ObjSyncQueueTbl::Rec idx_rec;
			THROW(r = p_ot->SyncTbl.SearchSync(objType, foreignID, src_db_div_id, 1, &rec));
			if(r > 0) {
				prim_id = rec.ObjID;
				ok = 1;
			}
			else if(p_ot->SearchQueueItem(objType, foreignID, src_db_div_id, &idx_rec) > 0) {
				PPCommSyncID comm_id(idx_rec);
				if(p_ot->SyncTbl.SearchCommon(objType, comm_id, 0 /* own dbdivid */, &rec) > 0) {
					prim_id = rec.ObjID;
					ok = 1;
				}
			}
		}
	}
	CATCHZOK
	ASSIGN_PTR(pPrimID, prim_id);
	return ok;
}

int ObjTransmContext::IsForced(PPObjID oi) const
	{ return BIN(P_ForceRestoreObj && P_ForceRestoreObj->lsearch(&oi, 0, PTR_CMPFUNC(_2long))); }
int ObjTransmContext::RegisterDependedNonObject(PPObjID objid, PPCommSyncID & rCommID, int use_ta)
	{ return P_Ot ? P_Ot->RegisterDependedNonObject(objid, rCommID, use_ta) : 0; }
int ObjTransmContext::ResolveDependedNonObject(PPID objType, PPID foreignID, PPID * pPrimID)
	{ return GetPrimaryObjID(objType, foreignID, pPrimID); }
int ObjTransmContext::AcceptDependedNonObject(PPObjID foreignObjId, PPID primaryID, const LDATETIME * pModDtm, int use_ta)
	{ return P_Ot ? P_Ot->AcceptDependedNonObject(foreignObjId, primaryID, pModDtm, use_ta) : 0; }
//
//
//
PP_CREATE_TEMP_FILE_PROC(CreateTempIndex, ObjSyncQueue);
PP_CREATE_TEMP_FILE_PROC(CreateTempSyncCmp, TempSyncCmp);

/*
PPObjectTransmit::PPObjectTransmit(TransmitMode mode, int syncCmp, int recoverTransmission) : CtrError(0), IamDispatcher(0),
	P_TmpIdxTbl(0), P_Queue(0), P_ObjColl(0), P_InStream(0), P_OutStream(0), Mode(mode), DestDbDivID(0),
	SyncCmpTransmit(BIN(syncCmp && mode == PPObjectTransmit::tmWriting)), RecoverTransmission(BIN(recoverTransmission))
*/
PPObjectTransmit::PPObjectTransmit(TransmitMode mode, uint ctrFlags) : CtrError(0), IamDispatcher(0),
	P_TmpIdxTbl(0), P_Queue(0), P_ObjColl(0), P_InStream(0), P_OutStream(0), Mode(mode), DestDbDivID(0),
	SyncCmpTransmit(BIN(ctrFlags & ctrfSyncCmp && mode == PPObjectTransmit::tmWriting)), 
	RecoverTransmission(BIN(ctrFlags & ctrfRecoverTransmission)),
	Ctx(((ctrFlags & ctrfDisableLogWindow) ? Ctx.ctrfDisableLogWindow : 0), 0)
{
	const PPConfig & r_cfg = LConfig;
	Ctx.Flags = 0;
	Ctx.P_Ot = this;
	PPObjectTransmit::ReadConfig(&Ctx.Cfg);
	if(Ctx.Cfg.SpcSubstGoodsGrpID)
		Ctx.P_Gra = new GoodsReplacementArray(Ctx.Cfg.SpcSubstGoodsGrpID);
	if(Mode == PPObjectTransmit::tmReading) {
		SetDestDbDivID(r_cfg.DBDiv);
		Ctx.P_DestDbDivPack = 0;
	}
	else
		Ctx.P_DestDbDivPack = &DestDbDivPack;
	if(DObj.Get(r_cfg.DBDiv, &ThisDbDivPack) <= 0)
		CtrError = 1;
	Ctx.P_ThisDbDivPack = &ThisDbDivPack;
	IamDispatcher = BIN(ThisDbDivPack.Rec.Flags & DBDIVF_DISPATCH);
}

PPObjectTransmit::~PPObjectTransmit()
{
	CloseInPacket();
	CloseOutPacket();
	delete P_TmpIdxTbl;
	delete P_Queue;
	delete P_ObjColl;
	CALLPTRMEMB(Ctx.P_Logger, Save(PPFILNAM_PROBLEMS_LOG, 0));
	//DBRemoveTempFiles(); // @turistti
}

PPObject * FASTCALL PPObjectTransmit::_GetObjectPtr(PPID objType)
{
	SETIFZ(P_ObjColl, new ObjCollection);
	return P_ObjColl ? P_ObjColl->GetObjectPtr(objType) : 0;
}

void PPObjectTransmit::CloseOutPacket()
{
	SFile::ZClose(&P_OutStream);
}

int PPObjectTransmit::SetDestDbDivID(PPID dbDivID)
{
	int    ok = 1;
	DestDbDivID = dbDivID;
	if(DObj.Get(dbDivID, &DestDbDivPack) <= 0) {
		DestDbDivID = 0;
		ok = 0;
	}
	return ok;
}

void PPObjectTransmit::SetupHeader(uint type, PPID destDBID, PPObjectTransmit::Header * pHdr)
{
	const PPConfig & r_cfg = LConfig;
	memzero(pHdr, sizeof(*pHdr));
	pHdr->Magic      = OT_MAGIC;
	pHdr->PacketType = type;
	pHdr->DBID       = r_cfg.DBDiv;
	pHdr->DestDBID   = destDBID;
	pHdr->ExtraData  = 0;
	pHdr->UserID     = r_cfg.UserID;
	if(type == PPOT_OBJ) {
		if(Ctx.Cfg.Flags & DBDXF_IGNOREACK)
			pHdr->Flags |= PPOTF_IGNACK;
		if(DestDbDivPack.Rec.Flags & DBDIVF_CONSOLID)
			pHdr->Flags |= PPOTF_CONSOLID;
		if(RecoverTransmission)
			pHdr->Flags |= PPOTF_RECOVER;
	}
	pHdr->SwVer      = DS.GetVersion();
	pHdr->MinDestVer = DS.GetMinCompatVersion(); // @v9.8.11 __MinCompatVer-->DS.GetMinCompatVersion()
	pHdr->SrcDivUuid  = ThisDbDivPack.Rec.Uuid;
	pHdr->DestDivUuid = DestDbDivPack.Rec.Uuid;
}

int PPObjectTransmit::UpdateInHeader(FILE * stream, const PPObjectTransmit::Header * pHdr)
{
	if(stream) {
		long   pos = ftell(stream);
		if(pos >= 0) {
			rewind(stream);
			fwrite(pHdr, sizeof(Header), 1, stream);
			fseek(stream, pos, SEEK_SET);
		}
	}
	return 1;
}

/*static*/int FASTCALL PPObjectTransmit::CheckInHeader(const PPObjectTransmit::Header & rHdr, int checkVer, const char * pFileName)
{
	int    ok = 1;
	THROW_PP_S(rHdr.Magic == OT_MAGIC, PPERR_PPOSNOTPACKET, pFileName);
	THROW_PP_S(rHdr.DestDBID == LConfig.DBDiv, PPERR_PPOSMISS, pFileName);
	THROW_PP_S(oneof3(rHdr.PacketType, PPOT_OBJ, PPOT_ACK, PPOT_SYNCCMP), PPERR_PPOSINVTYPE, pFileName);
	if(checkVer && rHdr.MinDestVer.V) {
		SString temp_buf;
		SVerT cur_ver = DS.GetVersion();
		int    mj, mn, r;
		rHdr.MinDestVer.Get(&mj, &mn, &r);
		THROW_PP_S(!cur_ver.IsLt(mj, mn, r), PPERR_RCVPACKETVER, temp_buf.Z().CatDotTriplet(mj, mn, r).Space().Cat(pFileName));
		DS.GetMinCompatVersion().Get(&mj, &mn, &r);
		THROW_PP_S(!rHdr.SwVer.IsLt(mj, mn, r), PPERR_RCVPACKETSRCVER, temp_buf.Z().CatDotTriplet(mj, mn, r).Space().Cat(pFileName));
	}
	CATCHZOK
	return ok;
}

int CallbackCompress(long, long, const char *, int); // @prototype
int PackTransmitFile(const char * pFileName, int pack, PercentFunc callbackProc);  // @prototype

int PPObjectTransmit::OpenInPacket(const char * fName, PPObjectTransmit::Header * pHdr)
{
	int    ok = -1;
	Header h;
	CloseInPacket();
	PPSetAddedMsgString(fName);
	THROW_PP((P_InStream = fopen(fName, "r+b")), PPERR_PPOSOPENFAULT);
	THROW(Read(P_InStream, &h, sizeof(h)));
	if(PPObjectTransmit::CheckInHeader(h, 0, fName)) {
		InHead = h;
		ASSIGN_PTR(pHdr, h);
		InFileName = fName;
		ok = 1;
	}
	CATCHZOK
	if(ok <= 0)
		CloseInPacket();
	return ok;
}

int PPObjectTransmit::CloseInPacket()
{
	int    ok = 1;
	if(P_InStream) {
		SFile::ZClose(&P_InStream);
		InFileName.Z();
	}
	else
		ok = -1;
	return ok;
}

/*static*/int PPObjectTransmit::GetHeader(const char * pFileName, PPObjectTransmit::Header * pHdr)
{
	int    ok = 1;
	Header h;
	SFile stream(pFileName, SFile::mRead | SFile::mBinary);
	THROW_PP_S(stream.IsValid(), PPERR_PPOSOPENFAULT, pFileName);
	THROW_PP_S(stream.Read(&h, sizeof(h)), PPERR_PPOSREADFAULT, pFileName);
	ASSIGN_PTR(pHdr, h);
	CATCHZOK
	return ok;
}

int PPObjectTransmit::Write(FILE * stream, const void * p, size_t s)
	{ return (stream && fwrite(p, s, 1, stream) == 1) ? 1 : PPSetError(PPERR_PPOSWRITEFAULT); }
int PPObjectTransmit::Read(FILE * stream, void * p, size_t s)
	{ return (stream && fread(p, s, 1, stream) == 1) ? 1 : PPSetError(PPERR_PPOSREADFAULT); }
//
//
//
PPObjectTransmit::IndexItem * PPObjectTransmit::TmpTblRecToIdxItem(const ObjSyncQueueTbl::Rec * pRec, IndexItem * pItem)
{
	assert(pItem);
	if(pItem) {
		memzero(pItem, sizeof(*pItem));
		pItem->ObjType  = pRec->ObjType;
		pItem->ObjID    = pRec->ObjID;
		pItem->CommID   = *pRec;
		pItem->ObjOffs  = pRec->FilePos;
		pItem->Mod.Set(pRec->ModDt, pRec->ModTm);
		pItem->Flags    = pRec->Flags;
		pItem->Priority = pRec->Priority;
	}
	return pItem;
}

int PPObjectTransmit::EnumObjectsByIndex(PPObjID * pObjId, ObjSyncQueueTbl::Rec * pRec)
{
	ObjSyncQueueTbl::Key1 k;
	k.ObjType = static_cast<short>(pObjId->Obj);
	k.ObjID   = pObjId->Id;
	k.DBID    = MAXSHORT;
	if(P_TmpIdxTbl->search(1, &k, spGt)) {
		pObjId->Set(k.ObjType, k.ObjID);
		P_TmpIdxTbl->CopyBufTo(pRec);
		return 1;
	}
	else
		return PPDbSearchError();
}

int PPObjectTransmit::PutSyncCmpToIndex(PPID objType, PPID id)
{
	int    ok = -1;
	int    r = -1;
	PPCommSyncID comm_id;
	ObjSyncTbl::Rec sync_rec;
	ObjSyncQueueTbl::Key1 k1;
	THROW_PP(DestDbDivID, PPERR_INVDESTDBDIV);
	THROW_PP(SyncCmpTransmit, PPERR_PPOS_NSYNCCMPMODE);
	THROW(SETIFZ(P_TmpIdxTbl, CreateTempIndex()));
	MEMSZERO(k1);
	k1.ObjType = (short)objType;
	k1.ObjID = id;
	k1.DBID  = (short)LConfig.DBDiv;
	THROW(r = SearchByKey(P_TmpIdxTbl, 1, &k1, 0));
	if(r < 0) {
		if(SyncTbl.SearchPrivate(objType, id, 0, &sync_rec) > 0)
			comm_id = sync_rec;
		if(/*!comm_id.IsZero() &&*/ id) {
			PPObject * p_obj = _GetObjectPtr(objType);
			SString obj_name;
			if(p_obj && p_obj->GetName(id, &obj_name) > 0) {
				int    cr_event;
				LDATETIME modif;
				ObjSyncQueueTbl::Rec rec;
				PPID   dest_id = (!comm_id.IsZero() && SyncTbl.SearchCommon(objType, comm_id, DestDbDivID, &sync_rec) > 0) ? sync_rec.ObjID : 0;
				rec.DBID    = static_cast<short>(LConfig.DBDiv);
				rec.ObjType = static_cast<short>(objType);
				comm_id.Get(&rec);
				rec.ObjID   = id;
				rec.FileId  = dest_id;
				if(p_obj->GetLastModifEvent(id, &modif, &cr_event) > 0) {
					rec.ModDt = modif.d;
					rec.ModTm = modif.t;
					SETFLAG(rec.Flags, PPObjPack::fCreationDtTm, cr_event);
					rec.Flags |= PPObjPack::fDefinedHeader;
				}
				obj_name.CopyTo(rec.ObjName, sizeof(rec.ObjName));
				THROW_DB(P_TmpIdxTbl->insertRecBuf(&rec));
				ok = 1;
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPObjectTransmit::PutObjectToIndex(PPID objType, PPID objID, int updProtocol, int innerUpdProtocol, long extra)
{
	#define NEED_SEND_NOOBJ 1000

	int    ok = 1;
	const PPConfig & r_cfg = LConfig;
	PPObject  * p_obj = 0;
	int    r, need_send = -1;
	int    this_obj_upd_protocol = updProtocol;
	const  int  do_log_send_info = BIN(CConfig.Flags & CCFLG_DEBUG);
	SString send_log_msg;
	SString obj_name;
	SString temp_buf;
	PPObjID oi(objType, objID);
	THROW_PP(DestDbDivID, PPERR_INVDESTDBDIV);
	THROW_PP(!SyncCmpTransmit, PPERR_PPOS_NOBJTRANMODE);
	THROW(SETIFZ(P_TmpIdxTbl, CreateTempIndex()));
	{
		PPIDArray exclude_obj_type_list;
		// @v11.1.7 PPOBJ_STYLOQBINDERY
		exclude_obj_type_list.addzlist(PPOBJ_CONFIG, PPOBJ_SCALE, PPOBJ_BHT, PPOBJ_BCODEPRINTER, PPOBJ_STYLOPALM, PPOBJ_STYLOQBINDERY, /*PPOBJ_USRGRP, PPOBJ_USR,*/ 0L); 
		if(!(Ctx.Cfg.Flags & DBDXF_SYNCUSRANDGRPS)) {
			exclude_obj_type_list.add(PPOBJ_USR);
			exclude_obj_type_list.add(PPOBJ_USRGRP);
		}
		if(oi.IsFullyDefined() && !exclude_obj_type_list.lsearch(oi.Obj)) {
			ObjSyncQueueTbl::Key1 k1;
			MEMSZERO(k1);
			k1.ObjType = static_cast<short>(oi.Obj);
			k1.ObjID   = oi.Id;
			k1.DBID    = static_cast<short>(r_cfg.DBDiv);
			int    r = SearchByKey(P_TmpIdxTbl, 1, &k1, 0);
			THROW(r);
			if(r > 0)
				need_send = -1;
			else {
				switch(oi.Obj) { // Обработка специальных случаев для отдельных типов объектов
					case PPOBJ_SCARD:
						if(IamDispatcher > 0)
							this_obj_upd_protocol = PPOTUP_FORCE;
						need_send = 1;
						break;
					case PPOBJ_BILL:
						THROW(need_send = BillObj->NeedTransmit(oi.Id, DestDbDivPack, &Ctx));
						assert(need_send != NEED_SEND_NOOBJ);
						break;
					case PPOBJ_CSESSION:
						{
							PPObjCSession * p_csess_obj = static_cast<PPObjCSession *>(_GetObjectPtr(oi.Obj));
							THROW(need_send = p_csess_obj->NeedTransmit(oi.Id, DestDbDivPack, &Ctx));
							assert(need_send != NEED_SEND_NOOBJ);
						}
						break;
					case PPOBJ_LOT:
						(obj_name = "LOT").Space().CatChar('#').Cat(objID);
						need_send = NEED_SEND_NOOBJ;
						break;
					default:
						need_send = 1;
						break;
				}
				if(do_log_send_info) {
					if(need_send != NEED_SEND_NOOBJ) {
						THROW(SETIFZ(p_obj, _GetObjectPtr(objType)));
					}
					send_log_msg.Z().Cat(DestDbDivID).Space().CatChar('{').Cat(SObjID_ToStr(oi, temp_buf)).CatChar('}').Space().Cat(need_send).Space().
						Cat(updProtocol).Space().Cat(innerUpdProtocol);
					if(obj_name.NotEmpty())
						temp_buf = obj_name;
					else if(!p_obj || p_obj->GetName(objID, &temp_buf) <= 0)
						temp_buf.Z();
					if(temp_buf.NotEmpty())
						send_log_msg.Space().Cat(temp_buf);
					PPLogMessage(PPFILNAM_DBXSEND_LOG, send_log_msg, LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_DBINFO);
				}
			}
		}
	}
	if(need_send == NEED_SEND_NOOBJ) {
		Ctx.Extra = extra;
		ushort transmit_flags = PPObjPack::fNoObj;
		ObjSyncQueueTbl::Rec rec;
		ObjSyncTbl::Rec s_rec;
		THROW(r = SyncTbl.SearchSync(objType, objID, DestDbDivID, 0, &s_rec));
		if(this_obj_upd_protocol == PPOTUP_FORCE)
			transmit_flags |= PPObjPack::fForceUpdate;
		else if(this_obj_upd_protocol == PPOTUP_BYTIME) {
			transmit_flags &= ~PPObjPack::fForceUpdate;
			transmit_flags |= PPObjPack::fUpdate;
		}
		else
			transmit_flags &= ~(PPObjPack::fForceUpdate | PPObjPack::fUpdate);
		if(RecoverTransmission)
			transmit_flags |= PPObjPack::fRecover;
		rec.DBID     = static_cast<short>(r_cfg.DBDiv);
		rec.ObjType  = static_cast<ushort>(objType);
		rec.ObjID    = objID;
		rec.Flags    = transmit_flags;
		rec.Priority = PPObjectTransmit::DefaultPriority;
		obj_name.CopyTo(rec.ObjName, sizeof(rec.ObjName));
		THROW_DB(P_TmpIdxTbl->insertRecBuf(&rec));
	}
	else if(need_send > 0) {
		Ctx.Extra = extra;
		need_send = 1;
		uint   i;
		int    cr_event;
		LDATETIME modif;
		PPObjPack   pack;
		PPObjIDArray temp;
		ObjSyncTbl::Rec s_rec;
		THROW(SETIFZ(p_obj, _GetObjectPtr(objType)));
		if(p_obj->GetLastModifEvent(objID, &modif, &cr_event) > 0) {
			pack.Mod = modif;
			SETFLAG(pack.Flags, PPObjPack::fCreationDtTm, cr_event);
			pack.Flags |= PPObjPack::fDefinedHeader;
		}
		THROW(r = SyncTbl.SearchSync(objType, objID, DestDbDivID, 0, &s_rec));
		//
		// @v11.2.12 Уточняем протокол: PPOTUP_FORCE имеет приоритет и не должен отменяться.
		//
		if(this_obj_upd_protocol == PPOTUP_FORCE) {
			pack.Flags |= PPObjPack::fForceUpdate;
			pack.Flags &= ~(PPObjPack::fUpdate|PPObjPack::fLateUpdatingDecision); // @v11.2.12
		}
		// @v11.3.0 {
		else if(this_obj_upd_protocol == PPOTUP_LATEUPDATINGDECISION) {
			pack.Flags |= PPObjPack::fLateUpdatingDecision;
			pack.Flags &= ~(PPObjPack::fUpdate);
		}
		// } @v11.3.0 
		else if(!(pack.Flags & (PPObjPack::fForceUpdate|PPObjPack::fLateUpdatingDecision))) { // @v11.2.12
			if(this_obj_upd_protocol == PPOTUP_BYTIME) {
				// @v11.2.12 pack.Flags &= ~PPObjPack::fForceUpdate;
				pack.Flags |= PPObjPack::fUpdate;
			}
			else {
				// @v11.2.12 pack.Flags &= ~(PPObjPack::fForceUpdate);
				pack.Flags &= ~(PPObjPack::fUpdate);
			}
		}
		if(RecoverTransmission)
			pack.Flags |= PPObjPack::fRecover;
		//
		// Вызов GetName гарантированно вызывает p_obj->Search и возвращает
		// результат этого вызова. Таким образом, избегаем двойного обращения к записи
		//
		if(need_send && p_obj->GetName(objID, &obj_name) > 0) {
			ObjSyncQueueTbl::Rec rec;
			rec.DBID     = static_cast<short>(r_cfg.DBDiv);
			rec.ObjType  = static_cast<ushort>(objType);
			rec.ObjID    = objID;
			rec.Flags    = static_cast<ushort>(pack.Flags);
			rec.ModDt    = pack.Mod.d;
			rec.ModTm    = pack.Mod.t;
			SETFLAG(Ctx.Flags, ObjTransmContext::fNotTrnsmLots, DestDbDivPack.Rec.Flags & DBDIVF_CONSOLID);
			//
			// Устанавливаем приоритет приема по умолчанию
			//
			pack.Priority = PPObjectTransmit::DefaultPriority;
			THROW(p_obj->Read(&pack, objID, 0, &Ctx));
			//
			// Функция PPObject::Read могла изменить приоритет приема => принимаем это значение
			//
			rec.Priority = pack.Priority;
			obj_name.CopyTo(rec.ObjName, sizeof(rec.ObjName));
			THROW_DB(P_TmpIdxTbl->insertRecBuf(&rec));
			//
			// Обрабатываем ссылки внутри пакета объекта
			//
			Ctx.Flags &= ~ObjTransmContext::fNotTrnsmLots;
		   	THROW(p_obj->ProcessObjRefs(&pack, &temp, 0, &Ctx));
			p_obj->Destroy(&pack);
			if(innerUpdProtocol == PPOTUP_DEFAULT) {
				if(updProtocol == PPOTUP_LATEUPDATINGDECISION) // @v11.3.0
					innerUpdProtocol = PPOTUP_BYTIME;
				else
					innerUpdProtocol = updProtocol;
			}
			{
				const uint oc = temp.getCount();
				for(i = 0; i < oc; i++) {
					const PPObjID & r_oi = temp.at(i);
					THROW(PutObjectToIndex(r_oi.Obj, r_oi.Id, innerUpdProtocol, PPOTUP_DEFAULT)); // @recursion
				}
			}
		}
		else
			ok = -1;
	}
	CATCHZOK
	return ok;
}
//
//
//
static int ConvertInBill(ILBillPacket * pPack, const ObjTransmContext * pCtx)
{
	int    ok = 1;
	const  int intr_op_tag = IsIntrOp(pPack->Rec.OpID);
	if(pCtx->Cfg.OneRcvLocID) {
		if(oneof2(intr_op_tag, INTREXPND, INTRRCPT))
			ok = -1;
		else
			pPack->Rec.LocID = pCtx->Cfg.OneRcvLocID;
	}
	else if(intr_op_tag == INTREXPND) {
		ILTI * p_ilti;
		const DBDivPack * p_dbd = pCtx->P_ThisDbDivPack;
		if(p_dbd->LocList.getCount() && p_dbd->Rec.IntrRcptOpr) {
			const  PPID loc_id = pPack->Rec.LocID;
			const  PPID dest_loc_id = PPObjLocation::ObjToWarehouse(pPack->Rec.Object);
			if(!p_dbd->LocList.lsearch(loc_id) && p_dbd->LocList.lsearch(dest_loc_id)) {
				pPack->Rec.OpID   = p_dbd->Rec.IntrRcptOpr;
				pPack->Rec.LocID  = dest_loc_id;
				pPack->Rec.Object = PPObjLocation::WarehouseToObj(loc_id);
				//
				// По не понятным причинам иногда JobServer принимает межскладские приходы
				// без контрагента (оригинальный документ нормальный, кроме того, если принимать
				// данные в ручную, то проблема не возникает).
				// Для точной идентификации проблемы сделан следующий участок.
				//
				if(pPack->Rec.Object == 0) {
					SString msg_buf, fmt_buf, id_buf;
					ArticleTbl::Rec ar_rec;
					PPObjAccTurn * p_atobj = BillObj->atobj;
					if(p_atobj->P_Tbl->Art.SearchObjRef(LConfig.LocAccSheetID, loc_id, &ar_rec) > 0) {
						pPack->Rec.Object = ar_rec.ID;
						PPLoadText(PPTXT_DIAG_WHTOOBJ657_SUCC, fmt_buf);
					}
					else {
						PPLoadText(PPTXT_DIAG_WHTOOBJ657_FAIL, fmt_buf);
					}
					PPLogMessage(PPFILNAM_DEBUG_LOG, msg_buf.Printf(fmt_buf, id_buf.Cat(loc_id).cptr()), LOGMSGF_TIME|LOGMSGF_USER);
				}
				//
				for(uint p = 0; pPack->Lots.enumItems(&p, (void **)&p_ilti);) {
					p_ilti->Quantity = fabs(p_ilti->Quantity);
					p_ilti->Rest     = fabs(p_ilti->Rest);
					p_ilti->Flags   |= (PPTFR_RECEIPT|PPTFR_FORCESUPPL);
				}
				pPack->IlbFlags |= ILBillPacket::ilbfConvertedIntrExp;
			}
		}
	}
	return ok;
}

int PPObjectTransmit::UpdateSyncCmpItem(TempSyncCmpTbl * pTbl, PPID objType, PPCommSyncID commID)
{
	int    ok = -1, r;
	TempSyncCmpTbl::Rec sct_rec;
	TempSyncCmpTbl::Key0 k0;
	MEMSZERO(k0);
	k0.ObjType   = objType;
	k0.CommIdPfx = commID.P;
	k0.CommID    = commID.I;
	THROW(r = SearchByKey(pTbl, 0, &k0, &sct_rec));
	if(r > 0) {
		THROW(SetupSyncCmpRec(0, &sct_rec));
		THROW_DB(pTbl->updateRecBuf(&sct_rec));
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int PPObjectTransmit::SetupSyncCmpRec(const ObjSyncQueueTbl::Rec * pQueueRec, TempSyncCmpTbl::Rec * pRec)
{
	int    ok = 1, r;
	double score = 0.0;
	LDATETIME mod;
	PPID   prim_id = 0;
	const  PPID src_id   = pQueueRec ? pQueueRec->ObjID   : pRec->SrcID;
	const  PPID obj_type = pQueueRec ? pQueueRec->ObjType : pRec->ObjType;
	const  PPID dest_id  = pQueueRec ? pQueueRec->FileId  : pRec->DestID;
	PPCommSyncID comm_id;
	if(pQueueRec) {
		comm_id = *pQueueRec;
		comm_id.Get(pRec);
		STRNSCPY(pRec->SrcName, pQueueRec->ObjName);
	}
	else
		comm_id = *pRec;
	pRec->ObjType = obj_type;
	pRec->SrcID   = src_id;
	THROW(r = GetPrivateObjSyncData(obj_type, comm_id, &prim_id, &mod, pRec->DestName, sizeof(pRec->DestName)));
	if(pQueueRec) {
		pRec->SrcModDt = pQueueRec->ModDt;
		pRec->SrcModTm = pQueueRec->ModTm;
	}
	pRec->DestModDt = mod.d;
	pRec->DestModTm = mod.t;
	if(pRec->DestName[0] == 0 || pRec->SrcName[0] == 0)
		pRec->CmpScore = 0.0;
	else if(ApproxStrCmp(pRec->DestName, pRec->SrcName, 1, &score) > 0)
		pRec->CmpScore = 1.0;
	else
		pRec->CmpScore = score;
	if(r > 0 && dest_id && prim_id && prim_id != dest_id)
		pRec->ErrCode = 3;
	else if(r == -1)
		pRec->ErrCode = 1;
	else if(r == -3)
		pRec->ErrCode = 4;
	else if(pRec->CmpScore != 1.0)
		pRec->ErrCode = 2;
	pRec->DestID = prim_id;
	CATCHZOK
	return ok;
}

PPObjectTransmit::Header::Header()
{
	THISZERO();
}

PPObjectTransmit::PacketStat::PacketStat() : Items(sizeof(PPObjectTransmit::IndexItem))
{
}

/*static*/int PPObjectTransmit::ReadFileStat(const char * pFileName, PacketStat & rStat)
{
	int    ok = 1;
	char * p_temp_buf = 0;
	PPObjectTransmit::IndexItem idx_item;
	size_t act_size;
	SFile stream(pFileName, SFile::mRead|SFile::mBinary);
	rStat.Items.clear();
	rStat.NameList.Z();
	THROW_PP_S(stream.IsValid(), PPERR_PPOSOPENFAULT, pFileName);
	THROW_SL(stream.Read(&rStat.Hdr, sizeof(rStat.Hdr), &act_size));
	if(!(rStat.Hdr.Flags & PPOTF_ARC)) {
		//
		// Считываем наименования объектов
		//
		SString obj_name;
		stream.Seek(rStat.Hdr.NameListOffs);
		uint32 name_list_size = 0;
		THROW_SL(stream.Read(&name_list_size, sizeof(name_list_size)));
		THROW_MEM(p_temp_buf = static_cast<char *>(SAlloc::M(name_list_size)));
		THROW_SL(stream.Read(p_temp_buf, name_list_size));
		THROW_SL(rStat.NameList.setBuf(p_temp_buf, name_list_size));
		ZFREE(p_temp_buf);
		//
		// Считываем элементы индекса
		//
		stream.Seek(rStat.Hdr.IndexOffs);
		for(uint idx = 0; idx < rStat.Hdr.IndexCount; idx++) {
			THROW_SL(stream.Read(&idx_item, sizeof(idx_item)));
			THROW_SL(rStat.Items.insert(&idx_item));
		}
	}
	CATCHZOK
	SAlloc::F(p_temp_buf);
	return ok;
}

/*static*/SString & PPObjectTransmit::GetQueueFilePath(SString & rBuf)
{
	rBuf.Z();
	PPGetPath(PPPATH_DAT, rBuf);
	return rBuf.SetLastSlash().Cat("SYNCQUE").SetLastSlash();
}

int PPObjectTransmit::Helper_PushObjectsToQueue(const PPObjectTransmit::Header & rHdr, long sysFileId, const TSVector <ObjSyncQueueTbl::Rec> & rList, int use_ta)
{
	int    ok = 1;
	SString msg_buf;
	SString fmt_buf;
	SString err_msg;
	SString obj_buf;
	SString wait_fmt_buf;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		PPLoadText(PPTXT_DBDE_SYNCQSETOBJECT, wait_fmt_buf);
		for(uint i = 0; i < rList.getCount(); i++) {
			ObjSyncQueueTbl::Rec idx_rec = rList.at(i);
			ObjSyncQueueTbl::Rec ex_rec;
			PPWaitMsg((msg_buf = wait_fmt_buf).CatDiv(':', 2).Cat(idx_rec.ObjName));
			int    r = P_Queue->SearchObject_(idx_rec.ObjType, idx_rec.ObjID, rHdr.DBID, &ex_rec);
			THROW(r);
			idx_rec.FileId = sysFileId;
			if(r > 0) {
				LDATETIME dtm_idx, dtm_ex;
				dtm_idx.Set(idx_rec.ModDt, idx_rec.ModTm);
				dtm_ex.Set(ex_rec.ModDt, ex_rec.ModTm);
				if(ex_rec.Priority > idx_rec.Priority || (ex_rec.Priority == idx_rec.Priority && cmp(dtm_ex, dtm_idx) < 0)) {
					ex_rec.CommIdPfx = idx_rec.CommIdPfx;
					ex_rec.CommID    = idx_rec.CommID;
					ex_rec.Flags     = idx_rec.Flags;
					ex_rec.ModDt     = idx_rec.ModDt;
					ex_rec.ModTm     = idx_rec.ModTm;
					ex_rec.Priority  = idx_rec.Priority;
					ex_rec.FileId    = idx_rec.FileId;
					ex_rec.FilePos   = idx_rec.FilePos;
					STRNSCPY(ex_rec.ObjName, idx_rec.ObjName);
					PPWaitMsg((msg_buf = wait_fmt_buf).CatDiv('-', 1).Cat("UPD").CatDiv(':', 2).Cat(idx_rec.ObjName));
					{
						int   ur = P_Queue->rereadForUpdate(1, 0); // @v9.8.3 ! idx=1 - because call P_Queue->SearchObject_ uses index = 1
						if(ur)
							ur = P_Queue->updateRecBuf(&ex_rec);
						if(!ur) {
							PPLoadText(PPTXT_LOG_ERRMODSYNCQUEUE, fmt_buf);
							GetObjectTitle(ex_rec.ObjType, err_msg);
							obj_buf.Z().Cat(err_msg).CatDiv('-', 1).Cat(ex_rec.ObjID).CatDiv('-', 1).Cat(ex_rec.DBID);
							PPGetLastErrorMessage(1, err_msg);
							Ctx.OutReceivingMsg(msg_buf.Printf(fmt_buf, obj_buf.cptr(), err_msg.cptr()));
							PPLogMessage(PPFILNAM_ERR_LOG, err_msg, LOGMSGF_DBINFO|LOGMSGF_TIME|LOGMSGF_USER);
						}
					}
				}
				else {
					// Оставляет существующую запись
					PPWaitMsg((msg_buf = wait_fmt_buf).CatDiv('-', 1).Cat("SKIP").CatDiv(':', 2).Cat(idx_rec.ObjName));
				}
			}
			else {
				idx_rec.ID = 0;
				PPWaitMsg((msg_buf = wait_fmt_buf).CatDiv('-', 1).Cat("ADD").CatDiv(':', 2).Cat(idx_rec.ObjName));
				if(!P_Queue->insertRecBuf(&idx_rec)) {
					PPLoadText(PPTXT_LOG_ERRINSSYNCQUEUE, fmt_buf);
					GetObjectTitle(idx_rec.ObjType, err_msg);
					obj_buf.Z().Cat(err_msg).CatDiv('-', 1).Cat(idx_rec.ObjID).CatDiv('-', 1).Cat(idx_rec.DBID);
					PPGetLastErrorMessage(1, err_msg);
					Ctx.OutReceivingMsg(msg_buf.Printf(fmt_buf, obj_buf.cptr(), err_msg.cptr()));
					PPLogMessage(PPFILNAM_ERR_LOG, err_msg, LOGMSGF_DBINFO|LOGMSGF_TIME|LOGMSGF_USER);
				}
			}
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int PPObjectTransmit::PushObjectsToQueue(PPObjectTransmit::Header & rHdr, const char * pInFileName, FILE * pInStream, int use_ta)
{
	int    ok = 1;
	SString sys_file_name;
	SString fmt_buf;
	SString msg_buf;
	SString err_msg;
	SString wait_fmt_buf;
	long   sys_file_id = 0;
	ObjSyncQueueTbl::Rec idx_rec;
	//
	// Заносим все объекты, которые надлежит акцептировать в общую очередь приема P_Queue
	//
	SETIFZ(P_Queue, new ObjSyncQueueCore);
	THROW_MEM(P_Queue);
	//
	// Если файл уже обработан и в очереди хранится ссылка на него то не следует
	// заталкивать его в очередь
	//
	if(rHdr.Flags & PPOTF_ACK && P_Queue->SearchRefToOrgFile(pInFileName, 0) > 0) {
		PPLoadText(PPTXT_LOG_FILEALLREADYINQUEUE, fmt_buf);
		Ctx.OutReceivingMsg(msg_buf.Printf(fmt_buf, pInFileName));
		ok = -1;
	}
	else {
		const uint max_idx_recs_per_ta = 512;
		//
		// Функция P_Queue->AddFileRecord вызывается в рамках собственной транзакции,
		// а вся очередь объектов на прием формируется отрезками по max_idx_recs_per_ta записей.
		//
		{
			ObjSyncQueueCore::FileInfo fi;
			{
				//
				// Создаем копию файла в подкаталоге SYNCQUE каталога базы данных
				// и заносим ссылку на этот файл в таблицу P_Queue
				//
				SFile sys_file;
				size_t bytes_read = 0;
				STempBuffer temp_buf(4096);
				SString sys_file_path;
				THROW_SL(SFile::CreateDir(GetQueueFilePath(sys_file_path)));
				THROW_SL(MakeTempFileName(sys_file_path, 0, 0, 0, sys_file_name));
				THROW_SL(sys_file.Open(sys_file_name, SFile::mWrite | SFile::mBinary));
				fseek(pInStream, 0L, SEEK_SET);
				while((bytes_read = fread(temp_buf, 1, temp_buf.GetSize(), pInStream)) > 0) {
					THROW_SL(sys_file.Write(temp_buf, bytes_read));
				}
				// @todo проверить CRC на копию файла
				//
				// Формируем запись о файле для таблицы P_Queue
				//
				fi.InnerFileName = sys_file_name;
				fi.OrgFileName = pInFileName;
				fi.Ver   = rHdr.SwVer;
				fi.Flags = rHdr.Flags;
				sys_file.GetDateTime(0, 0, &fi.Mod);
				sys_file.Close();
			}
			PPLoadText(PPTXT_DBDE_SYNCQSETFILE, wait_fmt_buf);
			PPWaitMsg((msg_buf = wait_fmt_buf).CatDiv(':', 2).Cat(pInFileName));
			if(!P_Queue->AddFileRecord(&sys_file_id, fi, use_ta)) {
				PPLoadText(PPTXT_LOG_ERRINSFILESYNCQUEUE, fmt_buf);
				PPGetLastErrorMessage(1, err_msg);
				Ctx.OutReceivingMsg(msg_buf.Printf(fmt_buf, pInFileName, err_msg.cptr()));
				CALLEXCEPT();
			}
		}
		{
			TSVector <ObjSyncQueueTbl::Rec> idx_rec_list;
			for(PPObjID objid; EnumObjectsByIndex(&objid, &idx_rec) > 0;) {
				THROW_SL(idx_rec_list.insert(&idx_rec));
				if(idx_rec_list.getCount() >= max_idx_recs_per_ta) {
					THROW(Helper_PushObjectsToQueue(rHdr, sys_file_id, idx_rec_list, use_ta));
					idx_rec_list.clear();
				}
			}
			if(idx_rec_list.getCount()) {
				THROW(Helper_PushObjectsToQueue(rHdr, sys_file_id, idx_rec_list, use_ta));
			}
		}
		rHdr.Flags |= PPOTF_ACK;
		THROW(UpdateInHeader(pInStream, &rHdr));
	}
	CATCHZOK
	return ok;
}

int PPObjectTransmit::RestoreFromStream(const char * pInFileName, FILE * stream, TempSyncCmpTbl * pTbl)
{
	int     ok = 1;
	ulong   idx;
	ObjSyncQueueTbl::Rec idx_rec;
	PPObjectTransmit::Header hdr;
	PPObjectTransmit::IndexItem idx_item;
	StringSet name_list;
	char * p_temp_buf = 0;
	fseek(stream, 0L, SEEK_SET);
	THROW(Read(stream, &hdr, sizeof(hdr)));
	{
		SBuffer state_buf;
		fseek(stream, hdr.SCtxStOffs, SEEK_SET);
		THROW_SL(state_buf.ReadFromFile(stream, 0));
		THROW_SL(Ctx.SCtx.SerializeStateOfContext(-1, state_buf));
	}
	if(hdr.IndexCount > 0) {
		//
		// Считываем наименования объектов
		//
		SString obj_name;
		fseek(stream, hdr.NameListOffs, SEEK_SET);
		uint32 name_list_size = 0;
		THROW(Read(stream, &name_list_size, sizeof(name_list_size)));
		THROW_MEM(p_temp_buf = static_cast<char *>(SAlloc::M(name_list_size)));
		THROW(Read(stream, p_temp_buf, name_list_size));
		THROW_SL(name_list.setBuf(p_temp_buf, name_list_size));
		ZFREE(p_temp_buf);
		//
		// Строим предварительную таблицу индекса принимаемых объектов P_TmpIdxTbl
		//
		fseek(stream, hdr.IndexOffs, SEEK_SET);
		BExtInsert bei(P_TmpIdxTbl);
		for(idx = 0; idx < hdr.IndexCount; idx++) {
			THROW(Read(stream, &idx_item, sizeof(idx_item)));
			MEMSZERO(idx_rec);
			idx_rec.DBID     = static_cast<short>(hdr.DBID);
			idx_rec.ObjType  = static_cast<short>(idx_item.ObjType);
			idx_rec.ObjID    = idx_item.ObjID;
			idx_item.CommID.Get(&idx_rec);
			idx_rec.FilePos  = idx_item.ObjOffs;
			idx_rec.ModDt    = idx_item.Mod.d;
			idx_rec.ModTm    = idx_item.Mod.t;
			idx_rec.Priority = idx_item.Priority;
			idx_rec.Flags    = static_cast<short>(idx_item.Flags);
			name_list.getnz(idx_item.ObjNamePos, obj_name);
			obj_name.CopyTo(idx_rec.ObjName, sizeof(idx_rec.ObjName));
			THROW_DB(bei.insert(&idx_rec));
		}
		THROW_DB(bei.flash());
	}
	else {
		Ctx.P_Logger->LogString(PPTXT_LOG_OBJSYNC_PACKETISEMPTY, pInFileName);
	}
	if(hdr.PacketType == PPOT_SYNCCMP) {
		if(pTbl) {
			uint32 count = 0;
			for(PPObjID objid; EnumObjectsByIndex(&objid, &idx_rec) > 0;) {
				int    skip = 0;
				TempSyncCmpTbl::Rec sct_rec, ex_rec;
				TempSyncCmpTbl::Key0 k0;
				TempSyncCmpTbl::Key1 k1;
				LDATETIME dtm;
				THROW(SetupSyncCmpRec(&idx_rec, &sct_rec));
				dtm.Set(sct_rec.SrcModDt, sct_rec.SrcModTm);
				MEMSZERO(k0);
				k0.ObjType   = sct_rec.ObjType;
				k0.CommIdPfx = sct_rec.CommIdPfx;
				k0.CommID    = sct_rec.CommID;
				if(SearchByKey(pTbl, 0, &k0, &ex_rec) > 0) {
					if(cmp(dtm, ex_rec.SrcModDt, ex_rec.SrcModTm) <= 0)
						skip = 1;
					else {
						THROW_DB(pTbl->rereadForUpdate(0, 0));
						THROW_DB(pTbl->deleteRec()); // @sfu
					}
				}
				else {
					MEMSZERO(k1);
					k1.ObjType = sct_rec.ObjType;
					k1.SrcID   = sct_rec.SrcID;
					if(SearchByKey(pTbl, 1, &k1, &ex_rec) > 0) {
						if(cmp(dtm, ex_rec.SrcModDt, ex_rec.SrcModTm) <= 0)
							skip = 1;
						else {
							THROW_DB(pTbl->rereadForUpdate(1, 0));
							THROW_DB(pTbl->deleteRec()); // @sfu
						}
					}
				}
				if(!skip)
					THROW_DB(pTbl->insertRecBuf(&sct_rec));
				PPWaitPercent(++count, hdr.IndexCount, /*fb.FileName*/0);
			}
		}
	}
	else {
		int    r;
    	THROW(r = PushObjectsToQueue(hdr, pInFileName, stream, 1));
		if(r < 0)
			ok = -1;
	}
	CATCH
		Ctx.P_Logger->LogLastError();
		ok = 0;
	ENDCATCH
	SAlloc::F(p_temp_buf);
	return ok;
}

PPObjectTransmit::RestoreObjBlock::RestoreObjBlock(ObjSyncQueueCore * pQueue, PPObjectTransmit * pOt) :
	S(sizeof(RestoreStackItem)), P_Queue(pQueue), P_Ot(pOt)
{
}

int PPObjectTransmit::RestoreObjBlock::PushRestoredObj(PPID dbID, PPObjID oi)
{
	RestoreStackItem i;
	i.Oi = oi;
	i.DbID = dbID;
	return S.push(&i) ? 1 : PPSetErrorSLib();
}

int PPObjectTransmit::RestoreObjBlock::DetectRecur(PPID dbID, PPObjID oi) const
{
	for(uint n = 0; n < S.getPointer(); n++) {
		const RestoreStackItem & r_si = *static_cast<const RestoreStackItem *>(S.at(n));
		if(r_si.Oi == oi && r_si.DbID == dbID)
			return 1;
	}
	return 0;
}

int PPObjectTransmit::RestoreObjBlock::PopRestoredObj(PPID dbID, PPObjID oi)
{
	int    ok = 1;
	RestoreStackItem i;
	return (!S.pop(&i) || oi != i.Oi || dbID != i.DbID) ? PPSetError(PPERR_OBJTSTACKFAULT) : 0;
}

PPObjectTransmit::OtFilePoolItem * PPObjectTransmit::RestoreObjBlock::SearchFile(long fileId)
{
	OtFilePoolItem * p_fpi = 0;
	uint   pos = 0;
	if(Fp.lsearch(&fileId, &pos, CMPF_LONG)) {
		p_fpi = Fp.at(pos);
	}
	else {
		ObjSyncQueueCore::FileInfo fi;
		THROW(P_Queue->GetFileRecord(fileId, fi));
		THROW_MEM(p_fpi = new OtFilePoolItem);
		p_fpi->FileId = fileId;
		p_fpi->OrgFileName = fi.OrgFileName;
		p_fpi->InVer = fi.Ver;
		p_fpi->AckCount = 0;
		THROW_SL(p_fpi->F.Open(fi.InnerFileName, SFile::mRead | SFile::mBinary));
		{
			PPObjectTransmit::Header hdr;
			p_fpi->F.Seek(0L);
			THROW(p_fpi->F.Read(&hdr, sizeof(hdr)));
			THROW(CheckInHeader(hdr, 1, fi.InnerFileName));
			p_fpi->F.Seek(hdr.SCtxStOffs);
			THROW_SL(p_fpi->SCtxState.ReadFromFile(p_fpi->F, 0));
		}
		THROW_SL(Fp.insert(p_fpi));
	}
	CATCH
		ZDELETE(p_fpi);
	ENDCATCH
	return p_fpi;
}

int PPObjectTransmit::RestoreObjBlock::SetQueueItem(const ObjSyncQueueTbl::Rec & rItem, RestoreObjItem * pRoi)
{
	int    ok = 1;
	memzero(pRoi, sizeof(*pRoi));
	pRoi->QueueID = rItem.ID;
	pRoi->Oi.Set(rItem.ObjType, rItem.ObjID);
	pRoi->CommID = rItem;
	pRoi->Flags  = rItem.Flags;
	pRoi->DBID   = rItem.DBID;
	pRoi->Mod.Set(rItem.ModDt, rItem.ModTm);
	pRoi->FileId  = rItem.FileId;
	pRoi->ObjOffs = rItem.FilePos;
	OtFilePoolItem * p_fpi = SearchFile(pRoi->FileId);
	THROW(p_fpi);
	pRoi->InVer = p_fpi->InVer;
	STRNSCPY(pRoi->ObjName, rItem.ObjName);
	CATCHZOK
	return ok;
}

int PPObjectTransmit::LogRcvObj(int msgId, const PPObjectTransmit::RestoreObjItem & rItem)
{
	SString added_msg;
	added_msg.Cat(rItem.Oi.Obj).CatDiv('-', 1).Cat(rItem.ObjName);
	return Ctx.P_Logger->LogString(msgId, added_msg);
}

int PPObjectTransmit::NeedRestoreObj(PPID objType, const PPObjectTransmit::RestoreObjItem & rItem, PPID * pPrimID)
{
	int    ok = -1;
	bool   is_unified_obj = false;
	const  bool _debug = LOGIC(CConfig.Flags & CCFLG_DEBUG);
	PPID   primary_id = 0;
	LDATETIME mod;
	int    is_cr = 0;
	ObjSyncTbl::Rec sync_rec;
	PPObject * ppobj = _GetObjectPtr(objType);
	if(ppobj) {
		SysJournal * p_sj = DS.GetTLA().P_SysJ;
		int    r = SyncTbl.SearchCommonObj(objType, rItem.CommID, &primary_id, &sync_rec);
		if(r) {
			const  bool was_deleted = (r > 0 && sync_rec.Flags & OBJSYNCF_DELETED);
			int    nfound = 0;
			if(primary_id == 0 || was_deleted || (nfound = BIN(ppobj->Search(primary_id) < 0)) != 0) {
				ok = 1;
				if(_debug) {
					if(primary_id == 0)
						LogRcvObj(PPTXT_LOG_OBJSYNC_RCVNSYNC, rItem);
					if(was_deleted)
						LogRcvObj(PPTXT_LOG_OBJSYNC_RCVRMVD, rItem);
					if(nfound)
						LogRcvObj(PPTXT_LOG_OBJSYNC_RCVNFND, rItem);
				}
				if(p_sj && !(Ctx.Cfg.Flags & DBDXF_IGNOBJUNIFY)) {
					SysJournalTbl::Rec sj_rec;
					if(was_deleted || primary_id) {
						PPID   subst_id = 0;
						PPIDArray recur_list;
						if(p_sj->GetLastObjUnifyEvent(objType, primary_id, &(subst_id = 0), &sj_rec) > 0) {
							do {
								if(recur_list.addUnique(subst_id) > 0) {
									if(_debug)
										LogRcvObj(PPTXT_LOG_OBJSYNC_RCVUNI, rItem);
									primary_id = subst_id;
									if(subst_id && ppobj->Search(subst_id) > 0) {
										is_unified_obj = true;
										ok = -100;
										break;
									}
								}
								else {
									LogRcvObj(PPTXT_LOG_OBJSYNC_RCVUNIRECUR, rItem);
									primary_id = 0;
									break;
								}
							} while(p_sj->GetLastObjUnifyEvent(objType, primary_id, &(subst_id = 0), &sj_rec) > 0);
						}
						else
							primary_id = 0;
					}
					else {
						const  long oldfashion_comm_id = (long)((rItem.CommID.P << 24) | rItem.CommID.I);
						PPID   subst_id = 0;
						if(p_sj->GetLastObjUnifyEvent(objType, oldfashion_comm_id, &subst_id, &sj_rec) > 0 && ppobj->Search(subst_id) > 0) {
							primary_id = subst_id;
							if(_debug)
								LogRcvObj(PPTXT_LOG_OBJSYNC_RCVUNIO, rItem);
							is_unified_obj = true;
							ok = -100;
						}
						else
							primary_id = 0;
					}
				}
				else
					primary_id = 0;
			}
			if(primary_id > 0) {
				if(!(rItem.Flags & PPObjPack::fProcessed)) {
					if(rItem.Flags & (PPObjPack::fForceUpdate|PPObjPack::fLateUpdatingDecision) || Ctx.IsForced(rItem.Oi)) // @v11.3.0 PPObjPack::fLateUpdatingDecision
						ok = 1;
					else if(rItem.Flags & PPObjPack::fUpdate && p_sj) {
						if(p_sj->GetLastObjModifEvent(objType, primary_id, &mod, &is_cr) > 0) {
							if(cmp(rItem.Mod, mod) > 0)
								ok = 1;
							else {
								if(_debug)
									LogRcvObj(PPTXT_LOG_OBJSYNC_RCVOLD, rItem);
							}
						}
						else
							ok = 1;
					}
				}
				if(ok > 0 && is_unified_obj)
					ok = -100;
			}
		}
		else
			ok = 0;
	}
	ASSIGN_PTR(pPrimID, primary_id);
	return ok;
}
//
// Функция RestoreObj является рекурсивной.
//
int PPObjectTransmit::RestoreObj(RestoreObjBlock & rBlk, RestoreObjItem & rItem, PPID * pPrimID)
{
	int    ok = 1;
	int    r;
	bool   pushed = false;
	bool   mark_item_as_processed = false;
	PPObject * ppobj = 0;
	PPID   primary_id = 0;
	PPObjID oi_f = rItem.Oi;
	PPCommSyncID comm_id(rItem.CommID);
	PPObjID dont_process_pair;
	SString added_buf;
	SString msg_buf;
	SString temp_buf;
	THROW(PPCheckUserBreak());
	if(IS_DYN_OBJTYPE(oi_f.Obj)) {
		//
		// Проверяем возможность создания динамического объекта. Если это - невозможно, то
		// искусственно разрешаем ссылку на тип объекта и пытаемся снова.
		//
		ObjSyncTbl::Rec rec;
		THROW(r = SyncTbl.SearchSync(PPOBJ_DYNAMICOBJS, oi_f.Obj, rItem.DBID, 1, &rec));
		if(r > 0)
			oi_f.Obj = rec.ObjID;
		if(_GetObjectPtr(oi_f.Obj))
			dont_process_pair.Set(PPOBJ_DYNAMICOBJS, oi_f.Obj);
		else {
			Ctx.P_Logger->LogString(PPTXT_LOG_UNRESOLVEDDYNOBJ, added_buf.Cat(rItem.Oi.Obj).CatDiv(';', 2).Cat(rItem.Oi.Id));
			ok = -1;
		}
	}
	if(ok > 0) {
		const int nro = NeedRestoreObj(oi_f.Obj, rItem, &primary_id);
		if(nro > 0 && !(rItem.Flags & PPObjPack::fNoObj)) {
			if(rBlk.DetectRecur(rItem.DBID, oi_f)) {
				//
				// Рекурсивная ссылка обнуляется //
				//
				comm_id.Z();
				primary_id = 0;
				LogRcvObj(PPTXT_LOG_OBJSYNC_RCVRECUR, rItem);
			}
			else if((ppobj = _GetObjectPtr(oi_f.Obj)) != 0) {
				ObjSyncQueueTbl::Rec idx_rec;
				PPObjID * p_entry;
				PPObjIDArray temp;
				PPObjPack  pack;
				pack.SrcVer = rItem.InVer;
				//
				// Запоминаем в стеке восстанавливаемый объект (для проверки на рекурсию)
				//
				THROW(rBlk.PushRestoredObj(rItem.DBID, oi_f));
				pushed = true;
				//
				OtFilePoolItem * p_fpi = rBlk.SearchFile(rItem.FileId);
				THROW(p_fpi);
				if(Ctx.LastStreamId != p_fpi->FileId) {
					p_fpi->SCtxState.SetRdOffs(0);
					THROW(Ctx.SCtx.SerializeStateOfContext(-1, p_fpi->SCtxState));
					Ctx.LastStreamId = p_fpi->FileId;
				}
				p_fpi->F.Seek(rItem.ObjOffs);
				THROW(ppobj->Read(&pack, oi_f.Id, static_cast<FILE *>(p_fpi->F), &Ctx));
				SETFLAGBYSAMPLE(pack.Flags, PPObjPack::fLateUpdatingDecision, rItem.Flags); // @v11.3.0
				SETFLAGBYSAMPLE(pack.Flags, PPObjPack::fForceUpdate, rItem.Flags); // @v11.3.0
				SETFLAG(pack.Flags, PPObjPack::fDispatcher, DestDbDivPack.Rec.Flags & DBDIVF_DISPATCH);
				THROW(ppobj->ProcessObjRefs(&pack, &temp, 0, &Ctx));
				for(uint i = 0; temp.enumItems(&i, (void **)&p_entry);) {
					if(p_entry->IsFullyDefined() && *p_entry != dont_process_pair /*&& !temp.Is_NotPreprocess_Pos(i-1)*/) { // @v8.0.9 && !temp.Is_NotPreprocess_Pos(i-1)
						if(!temp.Is_NotPreprocess_Pos(i-1) && P_Queue->SearchObject_(p_entry->Obj, p_entry->Id, rItem.DBID, &idx_rec) > 0) {
							RestoreObjItem inner_item;
							THROW(rBlk.SetQueueItem(idx_rec, &inner_item));
							THROW(InitContextSrcDiv(idx_rec.DBID));
							THROW(RestoreObj(rBlk, inner_item, &p_entry->Id)); // @recursion
						}
						else {
							ObjSyncTbl::Rec rec;
							THROW(r = SyncTbl.SearchSync(p_entry->Obj, p_entry->Id, rItem.DBID, 1, &rec));
							if(r > 0)
								p_entry->Id = rec.ObjID;
							else {
								PPLoadText(PPTXT_RCVREFUNRESOLVED, added_buf);
								GetObjectTitle(p_entry->Obj, temp_buf.Z()).CatDiv(';', 2).Cat(p_entry->Id);
								Ctx.OutReceivingMsg(msg_buf.Printf(added_buf, temp_buf.cptr()));
								p_entry->Id = 0;
							}
						}
					}
				}
				//
				// @v7.0.10
				// Предыдущий цикл мог переключить контекст извлечения данных.
				// По-этому, мы должны после этого цикла снова восстановить правильный контекст.
				//
				if(Ctx.LastStreamId != p_fpi->FileId) {
					p_fpi->SCtxState.SetRdOffs(0);
					THROW(Ctx.SCtx.SerializeStateOfContext(-1, p_fpi->SCtxState));
					Ctx.LastStreamId = p_fpi->FileId;
				}
				//
				// }
				//
				THROW(r = ppobj->ProcessObjRefs(&pack, &temp, 1, &Ctx));
				if(r == -2)
					r = -1;
				else if(oi_f.Obj == PPOBJ_BILL) {
					THROW(r = ConvertInBill(static_cast<ILBillPacket *>(pack.Data), &Ctx));
				}
				else
					r = 1;
				if(r > 0) {
					r = ppobj->Write(&pack, &primary_id, 0, &Ctx);
					THROW(r);
					if(r > 0) {
						mark_item_as_processed = true;
						if(r == 101 || (r == 102 && !(Ctx.Cfg.Flags & DBDXF_DONTLOGOBJUPD)))
							Ctx.OutputAcceptMsg(oi_f.Obj, primary_id, BIN(r == 102));
					}
				}
				ppobj->Destroy(&pack);
			}
			else
				Ctx.P_Logger->LogString(PPTXT_LOG_UNIDENTOBJ, added_buf.Cat(oi_f.Obj).CatDiv(';', 2).Cat(oi_f.Id));
		}
		else
			mark_item_as_processed = true;
		if(mark_item_as_processed) {
			PPTransaction tra(1);
			THROW(tra);
			THROW(P_Queue->MarkAsProcessed(rItem.QueueID, primary_id, 0));
			THROW_SL(rBlk.ProcessedList.addUnique(rItem.QueueID));
			if(primary_id) {
				if(nro != -100) {
					//
					// Для объекта, принятого по объединению не фиксируем записи в таблице синхронизации
					//
					THROW(SyncTbl._RcvObj(oi_f.Obj, primary_id, comm_id, LConfig.DBDiv, &rItem.Mod, 0));
					THROW(SyncTbl._RcvObj(oi_f.Obj, oi_f.Id, comm_id, rItem.DBID, &rItem.Mod, 0));
				}
			}
			rItem.PrimID = primary_id;
			THROW(tra.Commit());
		}
	}
	*pPrimID = primary_id;
	CATCHZOK
	if(pushed)
		rBlk.PopRestoredObj(rItem.DBID, oi_f);
	return ok;
}

int PPObjectTransmit::InitContextSrcDiv(PPID dbDivID)
{
	DBDivPack * p_pack = 0;
	uint   pos = 0;
	if(SrcDivPool.lsearch(&dbDivID, &pos, CMPF_LONG, offsetof(PPDBDiv, ID)))
		p_pack = SrcDivPool.at(pos);
	else {
		p_pack = new DBDivPack;
		if(p_pack)
			if(DObj.Get(dbDivID, p_pack) > 0)
				SrcDivPool.insert(p_pack);
			else
				ZDELETE(p_pack);
		else
			PPSetError(PPERR_NOMEM);
	}
	Ctx.P_SrcDbDivPack = p_pack;
	return BIN(p_pack);
}

int PPObjectTransmit::CommitQueue(const PPIDArray & rSrcDivList, int forceDestroyQueue)
{
	int    ok = 1, ta = 0;
	int    next_pass = 0, first_pass = 1;
	ObjSyncQueueCore::FileInfo fi;
	SString file_path;
	SETIFZ(P_Queue, new ObjSyncQueueCore);
	THROW_MEM(P_Queue);
	if(DS.CheckExtFlag(ECF_TRACESYNCLOT)) {
		P_Queue->PrintDebugObjList(PPOBJ_LOT);
	}
	{
		PPObjSecur::Exclusion ose(PPEXCLRT_OBJDBDIVACCEPT); // @v8.6.1
		do {
			next_pass = 0;
			uint   i;
			int    is_there_unprocessed_objects = 0;
			RestoreObjBlock blk(P_Queue, this);
			ObjSyncQueueTbl::Rec queue_rec;
			Ctx.P_Rb = &blk;
			//
			// Перебираем записи в очереди по индексу {Priority, Dt desc, Tm desc}
			//
			{
				PPIDArray queue_id_list;
				P_Queue->GetUnprocessedList(&queue_id_list);
				for(i = 0; i < queue_id_list.getCount(); i++) {
					THROW(P_Queue->Search(queue_id_list.get(i), &queue_rec) > 0);
					if(!rSrcDivList.getCount() || rSrcDivList.lsearch(queue_rec.DBID)) {
						if(queue_rec.ObjType && queue_rec.ObjID && !(queue_rec.Flags & PPObjPack::fProcessed)) {
							PPID   prmr_id = 0;
							RestoreObjItem item;
							THROW(blk.SetQueueItem(queue_rec, &item));
							THROW(InitContextSrcDiv(queue_rec.DBID));
							THROW(RestoreObj(blk, item, &prmr_id));
						}
					}
					PPWaitPercent(i+1, queue_id_list.getCount(), queue_rec.ObjName);
				}
				//
				// Определяем, остались ли еще необработанные объекты?
				//
				if(P_Queue->GetUnprocessedList(0) > 0)
					is_there_unprocessed_objects = 1;
				if(first_pass && DS.CheckExtFlag(ECF_TRACESYNCLOT)) {
					P_Queue->PrintDebugObjList(PPOBJ_LOT);
				}
			}
			//
			// Формируем пакеты подтверждений
			//
			// @todo if(!(hdr.Flags & PPOTF_IGNACK))
			for(i = 0; i < blk.ProcessedList.getCount(); i++) {
				Ack ack;
				THROW(P_Queue->Search(blk.ProcessedList.get(i), &queue_rec) > 0);
				OtFilePoolItem * p_fpi = blk.SearchFile(queue_rec.FileId);
				THROW(p_fpi);
				if(!p_fpi->A.IsValid()) {
					Header hdr;
					//
					// Уникальный идентификатор UUID раздела введен в версии 8.0.12. При передаче данных в другой раздел
					// раздел-отправитель теперь должен отправить свой UUID.
					// Функция MakeTransmitFileName() теперь одновременно генерирует (если необходимо) собственный UUID.
					//
					S_GUID src_div_uuid;
					THROW(MakeTransmitFileName(file_path, &src_div_uuid));
					ThisDbDivPack.Rec.Uuid = src_div_uuid;
					THROW_PP_S(p_fpi->A.Open(file_path, SFile::mReadWrite|SFile::mBinary), PPERR_PPOSOPENFAULT, file_path);
					SetupHeader(PPOT_ACK, queue_rec.DBID, &hdr);
					THROW_SL(p_fpi->A.Write(&hdr, sizeof(hdr)));
					//
					// Заносим в файл подтверждения имя оригинального файла, который был обработан
					//
					THROW(P_Queue->GetFileRecord(queue_rec.FileId, fi));
					if(fi.OrgFileName.NotEmptyS()) {
						MEMSZERO(ack);
						ack.Obj = -1;
						fi.OrgFileName.CopyTo(ack.FileName, sizeof(ack.FileName));
						THROW_SL(p_fpi->A.Write(&ack, sizeof(ack)));
					}
				}
				{
					PPCommSyncID comm_id(queue_rec);
					if(queue_rec.ObjType && queue_rec.PrimObjID && !comm_id.IsZero()) {
						MEMSZERO(ack);
						ack.Obj = queue_rec.ObjType;
						ack.Id  = queue_rec.PrimObjID;
						ack.CommId = comm_id;
						// @todo Здесь должен быть момент изменения объекта в собственном разделе
						ack.DT.Set(queue_rec.ModDt, queue_rec.ModTm);
						THROW_SL(p_fpi->A.Write(&ack, sizeof(ack)));
						p_fpi->AckCount++;
					}
				}
			}
			PPObjBill::TotalTransmitProblems(&Ctx, &next_pass);
			if(!next_pass) {
				for(i = 0; i < blk.Fp.getCount(); i++) {
					OtFilePoolItem * p_fpi = blk.Fp.at(i);
					if(p_fpi->A.IsValid()) {
						file_path = p_fpi->A.GetName();
						p_fpi->A.Close();
						//
						// Удаляем файлы подтверждений, не имеющие ни одной записи
						//
						if(p_fpi->AckCount == 0)
							SFile::Remove(file_path);
					}
					if(!is_there_unprocessed_objects)
						if(p_fpi->F.IsValid())
							p_fpi->F.Close();
				}
				//
				// Если нет ни одного необработанного объекта, то вычищаем всю очередь вместе с файлами.
				// Если же есть хотя бы один необработанный объект, то придется все оставить на месте
				// из-за того, что необработанный объект может ссылаться на обработанные.
				//
				if(!is_there_unprocessed_objects || forceDestroyQueue)
					THROW(P_Queue->Clear());
			}
			Ctx.P_Rb = 0;
			first_pass = 0;
		} while(next_pass);
	}
	CATCHZOK
	Ctx.P_Rb = 0;
	return ok;
}

int PPObjectTransmit::CommitAck()
{
	int    ok = 1;
	Ack    ack;
	fseek(P_InStream, sizeof(Header), SEEK_SET);
	while(Read(P_InStream, &ack, sizeof(ack))) {
		if(ack.Obj != -1) {
			THROW(SyncTbl.AckObj(ack.Obj, ack.Id, ack.CommId, InHead.DBID, &ack.DT, 1));
		}
		else if(ack.FileName[0]) {
			int    r = -1;
			Header hdr;
			SString path;
			PPGetPath(PPPATH_OUT, path);
			path.Cat(ack.FileName);
			if(fileExists(path)) {
				SFile f(path, SFile::mReadWrite | SFile::mBinary);
				THROW_SL(f.IsValid());
				THROW_SL(f.Read(&hdr, sizeof(hdr)));
				if(hdr.Magic == OT_MAGIC && hdr.DBID == LConfig.DBDiv && !(hdr.Flags & PPOTF_ACK)) {
					hdr.Flags |= PPOTF_ACK;
					f.Seek(0);
					THROW_SL(f.Write(&hdr, sizeof(hdr)));
					r = 1;
				}
			}
		}
	}
	InHead.Flags |= PPOTF_ACK;
	THROW(UpdateInHeader(P_InStream, &InHead));
	CATCHZOK
	return ok;
}

int PPObjectTransmit::SearchQueueItem(PPID objType, PPID objID, PPID dbID, ObjSyncQueueTbl::Rec * pRec)
{
	return P_Queue ? P_Queue->SearchObject_(objType, objID, dbID, pRec) : 0;
}

int PPObjectTransmit::RegisterDependedNonObject(PPObjID objid, PPCommSyncID & rCommID, int use_ta)
{
	rCommID.Z();
	int    ok = 1;
	ObjSyncQueueTbl::Key1 k;
	k.ObjType = static_cast<short>(objid.Obj);
	k.ObjID   = objid.Id;
	k.DBID    = static_cast<short>(LConfig.DBDiv);
	PPTransaction tra(use_ta);
	THROW(tra);
	if(P_TmpIdxTbl->searchForUpdate(1, &k, spEq)) {
		ObjSyncQueueTbl::Rec rec;
		P_TmpIdxTbl->CopyBufTo(&rec);
		THROW(SyncTbl.TransmitObj(objid, &rCommID, 0));
		rCommID.Get(&rec);
		rec.FilePos = 0;
		THROW_DB(P_TmpIdxTbl->updateRecBuf(&rec)); // @sfu
	}
	else {
		;
	}
	THROW(tra.Commit());
	CATCHZOK
	return ok;
}

int PPObjectTransmit::AcceptDependedNonObject(PPObjID foreignObjId, PPID primaryID, const LDATETIME * pModDtm, int use_ta)
{
	int    ok = -1;
	THROW_PP(Ctx.P_SrcDbDivPack && Ctx.P_SrcDbDivPack->Rec.ID, PPERR_PPOS_UNDEFCTXSRCDIV);
	if(primaryID && P_Queue) {
		const  PPID obj_type = foreignObjId.Obj;
		// @v8.5.8 @fix const  PPID src_div_id = SrcDbDivPack.Rec.ID;
		const  PPID src_div_id = Ctx.P_SrcDbDivPack->Rec.ID; // @v8.5.8 @fix
		ObjSyncQueueTbl::Rec idx_rec;
		PPTransaction tra(use_ta);
		THROW(tra);
		const int r = P_Queue->SearchObject_(obj_type, foreignObjId.Id, src_div_id, &idx_rec);
		if(r > 0) {
			PPCommSyncID comm_id(idx_rec);
			THROW(SyncTbl._RcvObj(obj_type, primaryID, comm_id, LConfig.DBDiv, pModDtm, 0));
			THROW(SyncTbl._RcvObj(obj_type, foreignObjId.Id, comm_id, src_div_id, pModDtm, 0));
			ok = 1;
		}
		else {
			SString msg_buf, err_msg;
			(msg_buf = "AcceptDependedNonObject QueueSearch result").Eq().Cat(r).CatDiv(':', 1).CatEq("obj_type", obj_type).CatDiv(',', 2).
				CatEq("foreignObjId", foreignObjId.Id).CatDiv(',', 2).CatEq("src_div_id", src_div_id);
			if(r < 0) {
				// При безуспешном поиске без ошибки код ошибки PPERR_DBENGINE не устанавливается - здесь важен код ошибки
				PPSetErrorDB();
			}
			PPGetMessage(mfError, -1, 0, 1, err_msg);
			msg_buf.Space().CatBrackStr(err_msg);
			//
			Ctx.OutReceivingMsg(msg_buf);
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int PPObjectTransmit::CreateTransmitPacket(long extra /*=0*/)
{
	int    ok = -1;
	SString file_name, temp_file_name;
	RECORDNUMBER obj_count = 0;
	int    todo = BIN(P_TmpIdxTbl && P_TmpIdxTbl->getNumRecs(&obj_count));
	const  uint packet_type = SyncCmpTransmit ? PPOT_SYNCCMP : PPOT_OBJ;
	if(todo && obj_count) {
		SString wait_msg;
		ObjSyncQueueTbl::Rec rec;
		PPObjectTransmit::Header hdr;
		IterCounter cntr;
		long   lpos;
		S_GUID src_div_uuid;
		THROW(MakeTransmitFileName(file_name, &src_div_uuid));
		CALLPTRMEMB(Ctx.P_Logger, LogString(PPTXT_LOG_OBJSYNC_TRANSMFILE, file_name)); // @v10.4.0 diag
		ThisDbDivPack.Rec.Uuid = src_div_uuid;
		{
			SString ext("tmp");
			temp_file_name = file_name;
			SFsPath::ReplaceExt(temp_file_name, ext, 1);
			for(long cnt = 0; fileExists(temp_file_name); cnt++) {
				ext.Z().Cat(++cnt);
				SFsPath::ReplaceExt(temp_file_name, ext, 1);
			}
			CloseOutPacket();
			{
				Header h;
				THROW_PP_S((P_OutStream = fopen(temp_file_name, "w+b")), PPERR_PPOSOPENFAULT, temp_file_name);
				SetupHeader(packet_type, DestDbDivID, &h);
				THROW(Write(P_OutStream, &h, sizeof(h)));
			}
		}
		//
		// Инициализируем контекст сериализации.
		// Дескрипторы структур данных будут записываться в поток раздельно
		// Опорная дата - текущая системная.
		//
		Ctx.SCtx.Init(SSerializeContext::fSeparateDataStruct, getcurdate_());
		if(!SyncCmpTransmit) {
			//
			// До начала транзакции создадим экземляры всех необходимых объектов данных (что бы не открывать таблицы внутри транзакции).
			//
			for(PPObjID objid; EnumObjectsByIndex(&objid, &rec) > 0;) {
				if(!(rec.Flags & PPObjPack::fNoObj)) {
					THROW(_GetObjectPtr(objid.Obj));
				}
			}
		}
		{
			const int mini_scope_transaction = 1;
			PPTransaction tra(mini_scope_transaction ? 0 : 1);
			THROW(tra);
			fseek(P_OutStream, 0L, SEEK_SET);
			THROW_PP(fread(&hdr, sizeof(hdr), 1, P_OutStream) == 1, PPERR_PPOSWRITEFAULT);
			fseek(P_OutStream, sizeof(hdr), SEEK_SET);
			PPLoadText(PPTXT_PUTTINGOBJTOSTRM, wait_msg);
			PPWaitMsg(wait_msg);
			{
				PPInitIterCounter(cntr, P_TmpIdxTbl);
				if(!SyncCmpTransmit) {
					PPObjID iter_objid;
					for(iter_objid.Z(); EnumObjectsByIndex(&iter_objid, &rec) > 0; PPWaitPercent(cntr.Increment(), wait_msg)) {
						PPObjID objid = iter_objid; // objid внутри блока может измениться //
						if(!(rec.Flags & PPObjPack::fNoObj)) {
							DBRowId rowid;
							PPObjPack  pack;
							PPCommSyncID comm_id;
							THROW_DB(P_TmpIdxTbl->getPosition(&rowid));
							pack.Data = 0;
							pack.SrcVer = hdr.SwVer;
							pack.Priority = PPObjectTransmit::DefaultPriority;
							PPObject * p_obj = _GetObjectPtr(objid.Obj);
							THROW(p_obj);
							SETFLAG(Ctx.Flags, ObjTransmContext::fNotTrnsmLots, DestDbDivPack.Rec.Flags & DBDIVF_CONSOLID);
							Ctx.Extra = extra;
							THROW(p_obj->Read(&pack, objid.Id, 0, &Ctx));
							rec.Priority = pack.Priority;
							lpos = ftell(P_OutStream);
							THROW(p_obj->Write(&pack, &objid.Id, P_OutStream, &Ctx));
							p_obj->Destroy(&pack);
							{
								PPTransaction ms_tra(mini_scope_transaction ? 1 : 0);
								THROW(ms_tra);
								THROW(SyncTbl.TransmitObj(objid, &comm_id, 0));
								comm_id.Get(&rec);
								rec.FilePos = lpos;
								THROW_DB(P_TmpIdxTbl->getDirectForUpdate(1, 0, rowid));
								THROW_DB(P_TmpIdxTbl->updateRecBuf(&rec)); // @sfu
								THROW(ms_tra.Commit());
							}
						}
					}
				}
				hdr.IndexOffs  = ftell(P_OutStream);
				hdr.IndexCount = cntr.GetTotal();
				{
					StringSet name_list;
					name_list.add("$");
					for(PPObjID objid; EnumObjectsByIndex(&objid, &rec) > 0;) {
						PPObjectTransmit::IndexItem idx_item;
						TmpTblRecToIdxItem(&rec, &idx_item);
						if(*strip(rec.ObjName)) {
							uint name_pos = 0;
							name_list.add(rec.ObjName, &name_pos);
							idx_item.ObjNamePos = name_pos;
						}
						THROW(Write(P_OutStream, &idx_item, sizeof(idx_item)));
					}
					hdr.NameListOffs = ftell(P_OutStream);
					const uint32 name_list_size = static_cast<uint32>(name_list.getDataLen());
					THROW(Write(P_OutStream, &name_list_size, sizeof(name_list_size)));
					THROW(Write(P_OutStream, name_list.getBuf(), name_list_size));
				}
				//
				// Сохраняем состояние контекста сериализации
				//
				{
					SBuffer state_buf;
					THROW_SL(Ctx.SCtx.SerializeStateOfContext(+1, state_buf));
					hdr.SCtxStOffs = ftell(P_OutStream);
					THROW_SL(state_buf.WriteToFile(P_OutStream, 0, 0));
				}
			}
			THROW_PP(fseek(P_OutStream, 0, SEEK_SET) == 0, PPERR_PPOSWRITEFAULT);
			THROW(Write(P_OutStream, &hdr, sizeof(hdr)));
			THROW(tra.Commit());
		}
		CloseOutPacket();
		THROW_SL(SFile::Rename(temp_file_name, file_name));
		ok = 1;
	}
	CATCH
		ok = 0;
		CloseOutPacket();
		SFile::Remove(file_name);
		SFile::Remove(temp_file_name);
	ENDCATCH
	return ok;
}

int PPObjectTransmit::PostObject(PPID objType, PPID id, int otup /* PPOTUP_XXX */, int syncCmp)
{
	return syncCmp ? PutSyncCmpToIndex(objType, id) : PutObjectToIndex(objType, id, otup);
}

int PPObjectTransmit::MakeTransmitFileName(SString & rFileName, S_GUID * pDbDivUuid)
{
	int    ok = 1;
	SString file_name, fmt_buf, msg_buf;
	long   counter = 0;
	PPLoadText(PPTXT_MAKETRNSMFILENAME, fmt_buf);
	{
		const  PPID db_div_id = LConfig.DBDiv;
		PPTransaction tra(1);
		THROW(tra);
		if(pDbDivUuid) {
			THROW(DObj.GetUuid(db_div_id, pDbDivUuid, 0));
		}
		do {
			THROW(DObj.GetCounter(db_div_id, &counter, 0));
			file_name.Z().CatLongZ(db_div_id, 4).CatLongZ(counter, 6).Cat(PPConst::FnExt_PPS);
			THROW(PPGetFilePath(PPPATH_OUT, file_name, rFileName));
			PPWaitMsg(msg_buf.Z().Printf(fmt_buf, rFileName.cptr()));
		} while(fileExists(rFileName));
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

/*static*/int PPObjectTransmit::TransmitModificationsByDBDivList(const ObjTransmitParam * pParam)
{
	MemLeakTracer mlt;
	int    ok = 1;
	PPLogger logger;
	ObjTransmitParam param;
	if(!RVALUEPTR(param, pParam))
		param.Init();
	if(pParam || ObjTransmDialog(DLG_MODTRANSM, &param, OBJTRNSMDLGF_SEARCHDTTM) > 0) {
		const PPIDArray & rary = param.DestDBDivList.Get();
		for(uint i = 0; i < rary.getCount(); i++)
			if(!TransmitModifications(rary.get(i), &param))
				if(rary.getCount() > 1) {
					logger.LogLastError();
					ok = 0;
				}
				else {
					THROW(0);
				}
	}
	else
		ok = -1;
	CATCHZOKPPERR
	return ok;
}

/*static*/int PPObjectTransmit::TransmitModifications(PPID destDBDiv, const ObjTransmitParam * pParam)
{
	int    ok = -1;
	ObjTransmitParam param;
	PPObjectTransmit * p_ot = 0;
	if(pParam || ObjTransmDialog(DLG_MODTRANSM, &param, OBJTRNSMDLGF_SEARCHDTTM) > 0) {
		RVALUEPTR(param, pParam);
		SysJournalFilt   sj_flt;
		PPViewSysJournal sj_view;
		SysJournalViewItem sj_item;
		SString wait_msg;
		//int    sync_cmp = BIN(param.Flags & ObjTransmitParam::fSyncCmp);
		uint   ot_ctrf = 0;
		if(param.Flags & ObjTransmitParam::fSyncCmp)
			ot_ctrf |= PPObjectTransmit::ctrfSyncCmp;
		if(param.Flags & ObjTransmitParam::fRecoverTransmission)
			ot_ctrf |= PPObjectTransmit::ctrfRecoverTransmission;
		PPWaitStart();
		THROW_MEM(p_ot = new PPObjectTransmit(PPObjectTransmit::tmWriting, ot_ctrf));
		THROW(p_ot->SetDestDbDivID(destDBDiv));
		param.Since_.d = param.Since_.d.getactual(ZERODATE);
		p_ot->Ctx.TransmitSince = param.Since_;
		PPLoadText(PPTXT_PREPTRANSMOBJ, wait_msg);
		sj_flt.Period.low = param.Since_.d;
		sj_flt.BegTm = param.Since_.t;
		sj_flt.ActionIDList.addzlist(PPACN_OBJADD, PPACN_OBJUPD, 0);
		sj_flt.ActionIDList.addzlist(PPACN_OBJTAGADD, PPACN_OBJTAGUPD, PPACN_OBJTAGRMV, 0);
		if(p_ot->DestDbDivPack.Rec.Flags & DBDIVF_SCARDSONLY) {
			sj_flt.ObjType = PPOBJ_SCARD;
		}
		else {
			if(param.ObjList.Search(PPOBJ_BILL, 0) > 0)
				sj_flt.ActionIDList.addzlist(PPACN_TURNBILL, PPACN_UPDBILL, PPACN_UPDBILLFREIGHT, PPACN_UPDBILLWLABEL, PPACN_BILLWROFF, PPACN_BILLWROFFUNDO, 0);
			if(param.ObjList.Search(PPOBJ_QUOT, 0) > 0 || param.ObjList.Search(PPOBJ_QUOT2, 0) > 0 || param.ObjList.Search(PPOBJ_GOODS, 0) > 0)
				sj_flt.ActionIDList.addzlist(PPACN_GOODSQUOTUPD, PPACN_QUOTUPD2, PPACN_QUOTRMV2, 0);
			if(p_ot->Ctx.Cfg.Flags & DBDXF_SENDCSESSION && param.ObjList.Search(PPOBJ_CSESSION, 0) > 0)
				sj_flt.ActionIDList.add(PPACN_CSESSCLOSED);
		}
		sj_view.Init_(&sj_flt);
		for(sj_view.InitIteration(); sj_view.NextIteration(&sj_item) > 0;) {
			if(!sj_item.ObjType || param.ObjList.Search(sj_item.ObjType, 0) > 0) {
				if(sj_item.ObjType != PPOBJ_GOODSBASKET) {
					// Корзины в пакете модифицированных объектов передавать не следует
					int update_mode = PPOTUP_BYTIME;
					// @v11.2.12 {
					// Специальный случай: товар не менялся, а надо передать измененя котировок - устанавливаем режим
					// акцепта в "безусловный" ибо в противном случае принимающая сторона не станет пытаться принимать пакет товара,
					// хотя в нем содержатся измененные котировки.
					if(sj_item.ObjType == PPOBJ_GOODS && oneof3(sj_item.Action, PPACN_GOODSQUOTUPD, PPACN_QUOTUPD2, PPACN_QUOTRMV2)) {
						update_mode = PPOTUP_LATEUPDATINGDECISION;
						//PPObjPack::fLateUpdatingDecision
					}
					// } @v11.2.12 
					THROW(p_ot->PostObject(sj_item.ObjType, sj_item.ObjID, update_mode, BIN(param.Flags & ObjTransmitParam::fSyncCmp)));
				}
			}
			PPWaitPercent(sj_view.GetCounter(), wait_msg);
		}
		THROW(p_ot->CreateTransmitPacket());
		ZDELETE(p_ot);
		PPWaitStop();
		THROW(PutTransmitFiles(destDBDiv, param.TrnsmFlags));
		DS.LogAction(PPACN_TRANSMOD, PPOBJ_DBDIV, destDBDiv, 0, 1);
		ok = 1;
	}
	CATCH
		ZDELETE(p_ot);
		ok = PPErrorZ();
	ENDCATCH
	return ok;
}

BillTransmitParam::BillTransmitParam() : PPBaseFilt(PPFILT_BILLTRANSMITPARAM, 0, 0), P_BillF(0)
{
	SetFlatChunk(offsetof(BillTransmitParam, ReserveStart), offsetof(BillTransmitParam, DestDBDivList)-offsetof(BillTransmitParam, ReserveStart));
	SetBranchObjIdListFilt(offsetof(BillTransmitParam, DestDBDivList));
	SetBranchBaseFiltPtr(PPFILT_BILL, offsetof(BillTransmitParam, P_BillF));
	Init(1, 0);
	DestDBDivList.InitEmpty();
}

class BillTransDialog : public WLDialog {
public:
	enum {
		ctlgroupBtranAr  = 1,
		ctlgroupBtranAr2 = 2
	};
	BillTransDialog() : WLDialog(DLG_BTRAN, CTL_BTRAN_LABEL)
	{
		SetupCalPeriod(CTLCAL_BTRAN_PERIOD, CTL_BTRAN_PERIOD);
		addGroup(ctlgroupBtranAr,  new ArticleCtrlGroup(0, CTLSEL_BTRAN_OP, CTLSEL_BTRAN_AR,  0, 0, 0));
		addGroup(ctlgroupBtranAr2, new ArticleCtrlGroup(0, CTLSEL_BTRAN_OP, CTLSEL_BTRAN_AR2, 0, 0, ArticleCtrlGroup::fByOpAccSheet2));
		//ArticleCtrlGroup(uint ctlselAcs, uint ctlselOp, uint ctlselAr, uint cmEditList, long accSheetID);
		if(!SetupStrListBox(this, CTL_BTRAN_LIST))
			PPError();
	}
	int setDTS(const BillTransmitParam * pData)
	{
		PPIDArray op_list;
		if(!RVALUEPTR(Data, pData))
			MEMSZERO(Data);
		if(Data.Period.IsZero())
			Data.Period.SetDate(LConfig.OperDate);
		SetPeriodInput(this, CTL_BTRAN_PERIOD, Data.Period);
		op_list.addzlist(PPOPT_ACCTURN, PPOPT_DRAFTEXPEND, PPOPT_DRAFTRECEIPT, PPOPT_GOODSRECEIPT,
			PPOPT_GOODSEXPEND, PPOPT_GOODSRETURN, PPOPT_GENERIC, PPOPT_DRAFTQUOTREQ, 0); // @v10.5.7 PPOPT_DRAFTQUOTREQ
		SetupOprKindCombo(this, CTLSEL_BTRAN_OP, Data.OpID, 0, &op_list, 0);
		{
			op_list.clear();
			PPOprKind op_rec;
			for(PPID op_id = 0; EnumOperations(0, &op_id, &op_rec) > 0;)
				if(oneof2(op_rec.OpTypeID, PPOPT_GOODSRECEIPT, PPOPT_GOODSEXPEND))
					op_list.add(op_id);
			SetupOprKindCombo(this, CTLSEL_BTRAN_TOOP, Data.ToOpID, 0, &op_list, OPKLF_OPLIST);
		}
		{
			ArticleCtrlGroup::Rec ar_grp_rec(0, Data.OpID, Data.ArID);
			ArticleCtrlGroup * p_ar_grp = static_cast<ArticleCtrlGroup *>(getGroup(ctlgroupBtranAr));
			setGroupData(ctlgroupBtranAr, &ar_grp_rec);
		}
		{
			ArticleCtrlGroup::Rec ar2_grp_rec(0, Data.OpID, Data.Ar2ID);
			ArticleCtrlGroup * p_ar2_grp = static_cast<ArticleCtrlGroup *>(getGroup(ctlgroupBtranAr2));
			setGroupData(ctlgroupBtranAr2, &ar2_grp_rec);
		}
		SetupCtrls();
		setWL(Data.Flags & BillTransmitParam::fLabelOnly);
		AddClusterAssoc(CTL_BTRAN_TRNSMFLAGS, 0, TRNSMF_DELINFILES);
		AddClusterAssoc(CTL_BTRAN_TRNSMFLAGS, 1, TRNSMF_DELOUTFILES);
		SetClusterData(CTL_BTRAN_TRNSMFLAGS, Data.TrnsmFlags);
		Data.DestDBDivList.InitEmpty();
		updateList();
		return 1;
	}
	int getDTS(BillTransmitParam * pData)
	{
		int    ok = 1;
		uint   i, sel = 0;
		PPDBDiv dbdiv_rec;
		PPObjDBDiv dbdiv_obj;
		SETFLAG(Data.Flags, BillTransmitParam::fLabelOnly, getWL());
		if(GetPeriodInput(this, sel = CTL_BTRAN_PERIOD, &Data.Period)) {
			const PPIDArray & rary = Data.DestDBDivList.Get();
			getCtrlData(CTLSEL_BTRAN_OP, &Data.OpID);
			getCtrlData(CTL_BTRAN_TOOP, &Data.ToOpID);
			Data.ToOpID = Data.OpID ? Data.ToOpID : 0L;
			{
				ArticleCtrlGroup::Rec ar_grp_rec;
				getGroupData(ctlgroupBtranAr, &ar_grp_rec);
				Data.ArID = ar_grp_rec.ArList.GetSingle();
			}
			{
				ArticleCtrlGroup::Rec ar2_grp_rec;
				getGroupData(ctlgroupBtranAr2, &ar2_grp_rec);
				Data.Ar2ID = ar2_grp_rec.ArList.GetSingle();
			}
			sel = CTL_BTRAN_LIST;
			THROW_PP(Data.DestDBDivList.GetCount(), PPERR_INVDESTDBDIV);
			for(i = 0; i < rary.getCount(); i++) {
				THROW_PP(dbdiv_obj.Search(rary.at(i), &dbdiv_rec) > 0, PPERR_INVDESTDBDIV)
				PPSetAddedMsgString(dbdiv_rec.Name);
				THROW_PP(!(dbdiv_rec.Flags & DBDIVF_SCARDSONLY), PPERR_CANTSENDBILLSTODIV);
			}
			GetClusterData(CTL_BTRAN_TRNSMFLAGS, &Data.TrnsmFlags);
			ASSIGN_PTR(pData, Data);
			ok = 1;
		}
		CATCHZOKPPERRBYDLG
		return ok;
	}
private:
	DECL_HANDLE_EVENT
	{
		WLDialog::handleEvent(event);
		if(event.isCmd(cmaInsert)) {
			if(!addItem())
				PPError();
		}
		else if(event.isCmd(cmaDelete)) {
			if(!delItem())
				PPError();
		}
		else if(event.isCbSelected(CTLSEL_BTRAN_OP))
			SetupCtrls();
		else
			return;
		clearEvent(event);
	}
	void   SetupCtrls();
	int    updateList();
	int    addItem();
	int    delItem();

	BillTransmitParam Data;
};

void BillTransDialog::SetupCtrls()
{
	const  PPID op_id = getCtrlLong(CTLSEL_BTRAN_OP);
	const  PPID op_type_id = GetOpType(op_id);
	const  bool disable = !oneof2(op_type_id, PPOPT_GOODSEXPEND, PPOPT_GOODSRECEIPT);
	disableCtrl(CTLSEL_BTRAN_TOOP, disable);
	if(disable)
		setCtrlLong(CTLSEL_BTRAN_TOOP, 0);
}

int BillTransDialog::updateList()
{
	int    ok = 1;
	SmartListBox * p_list = static_cast<SmartListBox *>(getCtrlView(CTL_BTRAN_LIST));
	if(p_list) {
		SString text;
		p_list->freeAll();
		if(Data.DestDBDivList.GetCount()) {
			const PPIDArray & rary = Data.DestDBDivList.Get();
			for(uint i = 0; i < rary.getCount(); i++) {
				const  PPID id = rary.at(i);
				GetObjectName(PPOBJ_DBDIV, id, text);
				THROW_SL(p_list->addItem(id, text));
			}
		}
		p_list->Draw_();
	}
	CATCHZOK
	return ok;
}

int BillTransDialog::addItem()
{
	int    ok = 1;
	PPID   id = 0;
	PPIDArray dd_list;
	ListToListData  ll_data(PPOBJ_DBDIV, 0, 0);
	dd_list.copy(Data.DestDBDivList.Get());
	ll_data.TitleStrID = PPTXT_SELECTDBDIV;
	ll_data.P_List = &dd_list;
	THROW(ListToListDialog(&ll_data));
	dd_list.removeByID(LConfig.DBDiv);
	Data.DestDBDivList.Set(&dd_list);
	THROW(updateList());
	CATCHZOK
	return ok;
}

int BillTransDialog::delItem()
{
	int    ok = -1;
	PPID   id = 0;
	SmartListBox * p_list = static_cast<SmartListBox *>(getCtrlView(CTL_BTRAN_LIST));
	if(p_list && p_list->getCurID(&id) && Data.DestDBDivList.Remove(id)) {
		ok = 1;
		updateList();
	}
	return ok;
}

int BillTransmitParam::Edit() { DIALOG_PROC_BODY(BillTransDialog, this); }

/*static*/int PPObjectTransmit::TransmitBillsByDBDivList(BillTransmitParam * pParam)
{
	int    ok = -1;
	PPLogger logger;
	BillTransmitParam flt;
	if(!pParam && flt.Edit() > 0) {
		pParam = &flt;
	}
	if(pParam) {
		const PPIDArray & rary = pParam->DestDBDivList.Get();
		for(uint i = 0; i < rary.getCount(); i++) {
			if(!TransmitBills(rary.at(i), pParam)) {
				if(rary.getCount() > 1) {
					logger.LogLastError();
					ok = 0;
				}
				else
					CALLEXCEPT();
			}
		}
	}
	else
		ok = -1;
	CATCHZOKPPERR
	return ok;
}

/*static*/int PPObjectTransmit::TransmitBills(PPID destDBDiv, const BillTransmitParam * pFilt)
{
	int    ok = 1;
	PPObjBill * p_bobj = BillObj;
	int    is_generic_op = 0;
	uint   gen_op_iterator = 0;
	PPID   op_id = 0;
	BillTbl::Rec br;
	PPIDArray op_list;
	PPObjectTransmit * p_ot = 0;
	BillTransmitParam flt;
	RVALUEPTR(flt, pFilt);
	DateIter diter(&flt.Period);
	THROW_MEM(p_ot = new PPObjectTransmit(PPObjectTransmit::tmWriting, 0/*ctrFlags*/));
	THROW(p_ot->SetDestDbDivID(destDBDiv));
	PPWaitStart();
	if(flt.OpID) {
		if(IsGenericOp(flt.OpID) > 0) {
			PPObjOprKind op_obj;
			op_obj.GetGenericList(flt.OpID, &op_list);
			is_generic_op = 1;
			gen_op_iterator = 0;
			op_id = (gen_op_iterator < op_list.getCount()) ? op_list.at(gen_op_iterator++) : 0;
		}
		else
			op_id = flt.OpID;
	}
	while(1) {
		int    r = -1;
		if(flt.OpID) {
			while(r < 0 && op_id) {
				THROW(r = p_bobj->P_Tbl->EnumByOpr(op_id, &diter, &br));
				if(r < 0)
	   		        if(is_generic_op && gen_op_iterator < op_list.getCount()) {
						op_id = op_list.at(gen_op_iterator++);
						diter.Init(&flt.Period);
					}
					else
				   		op_id = 0;
			}
		}
		else {
			THROW(r = p_bobj->P_Tbl->EnumByDate(&diter, &br));
		}
		if(r > 0) {
			int    is_suit = 1;
			if((flt.OpID && flt.Ar2ID) && br.Object2 != flt.Ar2ID)
				is_suit = 0;
			else if((flt.OpID && flt.ArID) && br.Object != flt.ArID)
				is_suit = 0;
			else if((flt.Flags & BillTransmitParam::fLabelOnly) && !(br.Flags & BILLF_WHITELABEL))
				is_suit = 0;
			else {
				THROW(p_ot->PutObjectToIndex(PPOBJ_BILL, br.ID, PPOTUP_FORCE, PPOTUP_BYTIME, flt.ToOpID));
			}
		}
		else
			break; // End of loop
	}
	THROW(p_ot->CreateTransmitPacket(flt.ToOpID));
	ZDELETE(p_ot);
	PPWaitStop();
	THROW(PutTransmitFiles(destDBDiv, flt.TrnsmFlags));
	CATCH
		ZDELETE(p_ot);
		ok = 0;
	ENDCATCH
	return ok;
}

int PPObjectTransmit::Transmit(const PPIDArray * pDBDivAry, const PPObjIDArray * pObjAry, const ObjTransmitParam * pParam)
{
	int    ok = 1;
	// @v10.4.1 PPLogger logger;
	if(pDBDivAry && pObjAry) {
		for(uint i = 0; i < pDBDivAry->getCount(); i++) {
			PPWaitStart();
			int    r = Transmit(pDBDivAry->at(i), pObjAry, pParam);
			if(!r) {
				if(pDBDivAry->getCount() > 1) {
					PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR_TIME_USER|LOGMSGF_DBINFO); // @v10.4.1
					// @v10.4.1 logger.LogLastError();
					ok = 0;
				}
				else
					CALLEXCEPT();
			}
			PPWaitStop();
		}
	}
	else
		ok = -1;
	CATCHZOK
	// @v10.4.1 logger.Save(PPFILNAM_ERR_LOG, 0); // @v10.4.0
	return ok;
}

/*static*/int PPObjectTransmit::Transmit(PPID dbDivID, const PPObjIDArray * pObjAry, const ObjTransmitParam * pParam)
{
	int    ok = 0;
	PPObjectTransmit * p_ot = 0;
	if(pObjAry && dbDivID && pParam) {
		//const int sync_cmp = BIN(pParam->Flags & ObjTransmitParam::fSyncCmp);
		uint   ot_ctrf = 0;
		if(pParam->Flags & ObjTransmitParam::fSyncCmp)
			ot_ctrf |= PPObjectTransmit::ctrfSyncCmp;
		if(pParam->Flags & pParam->fRecoverTransmission)
			ot_ctrf |= PPObjectTransmit::ctrfRecoverTransmission;
		THROW_MEM(p_ot = new PPObjectTransmit(PPObjectTransmit::tmWriting, ot_ctrf));
		THROW(p_ot->SetDestDbDivID(dbDivID));
		{
			const uint c = pObjAry->getCount();
			SString msg_buf;
			PPLoadText(PPTXT_DBDE_POSTOBJECTS, msg_buf);
			for(uint i = 0; i < c; i++) {
				const PPObjID & r_oid = pObjAry->at(i);
				THROW(p_ot->PostObject(r_oid.Obj, r_oid.Id, pParam->UpdProtocol, BIN(pParam->Flags & ObjTransmitParam::fSyncCmp)));
				PPWaitPercent((i+1), c, msg_buf);
			}
		}
		THROW(p_ot->CreateTransmitPacket());
		ZDELETE(p_ot);
		THROW(PutTransmitFiles(dbDivID, pParam->TrnsmFlags));
		DS.LogAction(PPACN_TRANSMIT, 0, 0, dbDivID, 1);
		ok = 1;
	}
	CATCHZOK
	ZDELETE(p_ot);
	return ok;
}
//
//
//
class ObjTranDialog : public TDialog {
public:
	ObjTranDialog(uint dlgID, long dlgFlags) : TDialog(dlgID), DlgFlags(dlgFlags), Since(ZERODATETIME)
	{
		if(!SetupStrListBox(this, CTL_OBJTRANSM_DBDIVLIST))
			PPError();
		if(!SetupStrListBox(this, CTL_OBJTRANSM_OBJLIST))
			PPError();
		showCtrl(CTL_OBJTRANSM_FILT, false);
	}
	int    setDTS(const ObjTransmitParam *);
	int    getDTS(ObjTransmitParam *);
protected:
	DECL_HANDLE_EVENT;
private:
	int    setupTransmissionEvent(int mode);
	int    addItem(int dbDivList);
	int    delItem(int dbDivList);
	int    updateList(int dbDivList);
	void   setSinceDate(LDATE);
	void   getSince();

	long   DlgFlags;
	LDATETIME Since;
	ObjTransmitParam Data;
};

int ObjTranDialog::updateList(int dbDivList)
{
	int    ok = 1;
	uint   ctl_id = dbDivList ? CTL_OBJTRANSM_DBDIVLIST : CTL_OBJTRANSM_OBJLIST;
	ObjIdListFilt * p_ary = dbDivList ? &Data.DestDBDivList : &Data.ObjList;
	SmartListBox * p_list = static_cast<SmartListBox *>(getCtrlView(ctl_id));
	if(p_list) {
		SString text;
		p_list->freeAll();
		if(p_ary->GetCount()) {
			const PPIDArray & rary = p_ary->Get();
			for(uint i = 0; i < rary.getCount(); i++) {
				const PPID id = rary.at(i);
				if(dbDivList)
					GetObjectName(PPOBJ_DBDIV, id, text);
				else
					GetObjectTitle(id, text);
				THROW_SL(p_list->addItem(id, text));
			}
		}
		p_list->Draw_();
	}
	CATCHZOK
	return ok;
}

int ObjTranDialog::addItem(int dbDivList)
{
	int    ok = 1;
	ObjIdListFilt * p_ary = dbDivList ? &Data.DestDBDivList : &Data.ObjList;
	if(dbDivList) {
		PPIDArray dd_list;
		ListToListData  ll_data(PPOBJ_DBDIV, 0, 0);
		dd_list.copy(p_ary->Get());
		ll_data.TitleStrID = PPTXT_SELECTDBDIV;
		ll_data.P_List = &dd_list;
		THROW(ListToListDialog(&ll_data));
		dd_list.removeByID(LConfig.DBDiv);
		p_ary->Set(&dd_list);
		getSince();
		setupTransmissionEvent(0);
	}
	else {
		SString obj_title;
		PPIDArray obj_type_list;
		const PPIDArray & rary = Data.ObjList.Get();
		StrAssocArray src_list;
		PPIDArray dest_list = rary;
		PPGetObjTypeList(&obj_type_list, 0);
		for(uint i = 0; i < obj_type_list.getCount(); i++) {
			const  PPID obj_type = obj_type_list.get(i);
			src_list.Add(obj_type, GetObjectTitle(obj_type, obj_title));
		}
		src_list.SortByText();
		ListToListData ll_data(&src_list, 0, &dest_list);
		ll_data.TitleStrID = PPTXT_SELECTOBJECT;
		if(ListToListDialog(&ll_data) > 0) {
			Data.ObjList.Set(&dest_list);
		}
	}
	THROW(updateList(dbDivList));
	CATCHZOK
	return ok;
}

int ObjTranDialog::delItem(int dbDivList)
{
	int    ok = -1;
	PPID   id = 0;
	uint   ctl_id = dbDivList ? CTL_OBJTRANSM_DBDIVLIST : CTL_OBJTRANSM_OBJLIST;
	ObjIdListFilt * p_ary = dbDivList ? &Data.DestDBDivList : &Data.ObjList;
	SmartListBox * p_list = static_cast<SmartListBox *>(getCtrlView(ctl_id));
	if(p_list && p_list->getCurID(&id) && p_ary->Remove(id)) {
		ok = 1;
		updateList(dbDivList);
	}
	return ok;
}

int ObjTranDialog::setupTransmissionEvent(int mode /* 0 - last, -1 - prev, 1 - next */)
{
	int    ok = -1;
	PPWaitStart();
	if(DlgFlags & OBJTRNSMDLGF_SEARCHDTTM && (getCtrlView(CTL_OBJTRANSM_DT) || getCtrlView(CTL_OBJTRANSM_TM))) {
		int    by_this_dbdiv = 0;
		PPID   db_div_id = 0;
		SysJournal * p_sj = DS.GetTLA().P_SysJ;
		SmartListBox * p_list = static_cast<SmartListBox *>(getCtrlView(CTL_OBJTRANSM_DBDIVLIST));
		if((mode == 1 || mode == -1) && p_list) {
			p_list->getCurID(&db_div_id);
			by_this_dbdiv = 1;
		}
		if(p_sj && Data.DestDBDivList.GetCount()) {
			LDATETIME since = Since;
			LDATETIME min_since;
			min_since = since;
			SETIFZ(min_since.d, getcurdate_());
			min_since.d = plusdate(min_since.d, -30);
			const PPIDArray & rary = Data.DestDBDivList.Get();
			for(uint i = 0; i < rary.getCount(); i++) {
				if(by_this_dbdiv == 0)
					db_div_id = rary.at(i);
				if(mode > 0) {
					while(ok < 0 && p_sj->GetEvent(PPACN_TRANSMOD, 0/*extraVal*/, 1, &min_since, 0, 0) > 0)
						if(!db_div_id || p_sj->data.ObjID == db_div_id)
							ok = 1;
				}
				else {
					if(!mode)
						since = min_since;
					LDATETIME prev = ZERODATETIME;
					while(ok < 0 && p_sj->GetEvent(PPACN_TRANSMOD, 0/*extraVal*/, 1, &since, 0, 0) > 0) {
						if(!db_div_id || p_sj->data.ObjID == db_div_id) {
							if(cmp(min_since, Since) > 0) {
								if(prev.d)
									since = prev;
								ok = 1;
							}
							else
								prev = since;
						}
					}
					/*
					while(ok < 0 && p_sj->GetLastEvent(PPACN_TRANSMOD, &since, 7, 0) > 0) {
						if(!db_div_id || p_sj->data.ObjID == db_div_id)
							ok = 1;
					}
					*/
					if(mode || (since.d && (!min_since.d || cmp(since, min_since) < 0)))
						min_since = since;
				}
				if(by_this_dbdiv)
					break;
			}
			if(ok > 0) {
				Since = min_since;
				setSinceDate(Since.d);
				setCtrlData(CTL_OBJTRANSM_TM, &Since.t);
			}
		}
	}
	PPWaitStop();
	return ok;
}

IMPL_HANDLE_EVENT(ObjTranDialog)
{
	TDialog::handleEvent(event);
	if(TVCOMMAND)
		if(TVCMD == cmaLevelUp)
			setupTransmissionEvent(1);
		else if(TVCMD == cmaLevelDown)
			setupTransmissionEvent(-1);
		else if(oneof2(TVCMD, cmaInsert, cmAddTrnsmObj)) {
			if(!addItem(TVCMD == cmaInsert ? 1 : 0))
				PPError();
		}
		else if(oneof2(TVCMD, cmaDelete, cmDelTrnsmObj)) {
			if(!delItem(TVCMD == cmaDelete ? 1 : 0))
				PPError();
		}
		else
			return;
	else
		return;
	clearEvent(event);
}

void ObjTranDialog::setSinceDate(LDATE dt)
{
	SString buf;
	if(dt.year() == (int)0x8000) {
		int16  lw = (int16)dt.v;
		if(lw != (int16)0x8000) {
			buf.CatChar('@');
			if(lw)
				buf.CatChar((lw > 0) ? '+' : '-').Cat(abs(lw));
		}
	}
	else
		buf.Cat(dt);
	setCtrlString(CTL_OBJTRANSM_DT, buf);
}

void ObjTranDialog::getSince()
{
	char   buf[64];
	long   dt_val = 0;
	buf[0] = 0;
	getCtrlData(CTL_OBJTRANSM_DT, buf);
	ParseBound(buf, &dt_val);
	Data.Since_.d.v = (ulong)dt_val;
	getCtrlData(CTL_OBJTRANSM_TM, &Data.Since_.t);
	Since = Data.Since_;
}

int ObjTranDialog::setDTS(const ObjTransmitParam * pData)
{
	Data = *pData;
	ushort v = 0;
	SString buf;
	Data.DestDBDivList.InitEmpty();
	Data.ObjList.InitEmpty();
	if(Data.DestDBDivList.GetCount() == 0) {
		PPID   single_dbdiv_id = 0;
		PPObjDBDiv dbdiv_obj;
		PPDBDiv dbdiv_rec;
		for(PPID div_id = 0; dbdiv_obj.EnumItems(&div_id, &dbdiv_rec) > 0;) {
			if(div_id != LConfig.DBDiv)
				if(single_dbdiv_id == 0)
					single_dbdiv_id = div_id;
				else {
					single_dbdiv_id = 0;
					break;
				}
		}
		Data.DestDBDivList.Add(single_dbdiv_id);
	}
	switch(Data.UpdProtocol) {
		case PPOTUP_NONE:   v = 0; break;
		case PPOTUP_BYTIME: v = 1; break;
		case PPOTUP_FORCE:  v = 2; break;
	}
	setCtrlData(CTL_OBJTRANSM_PROTOCOL, &v);
	v = 0;
	SETFLAG(v, 0x01, pData->Flags & ObjTransmitParam::fSyncCmp);
	setCtrlData(CTL_OBJTRANSM_FLAGS, &v);
	setSinceDate(Data.Since_.d);
	setCtrlData(CTL_OBJTRANSM_TM, &Data.Since_.t);
	AddClusterAssoc(CTL_OBJTRANSM_TRNSMFLAGS, 0, TRNSMF_DELINFILES);
	AddClusterAssoc(CTL_OBJTRANSM_TRNSMFLAGS, 1, TRNSMF_DELOUTFILES);
	SetClusterData(CTL_OBJTRANSM_TRNSMFLAGS, Data.TrnsmFlags);
	if(Data.Flags & Data.fQueryInmassTransmission) {
		showCtrl(CTL_OBJTRANSM_INMASS, true);
		AddClusterAssoc(CTL_OBJTRANSM_INMASS, 0, Data.fInmassTransmission);
		SetClusterData(CTL_OBJTRANSM_INMASS, Data.Flags);
	}
	else {
		showCtrl(CTL_OBJTRANSM_INMASS, false);
	}
	if(Id == DLG_MODTRANSM)
		updateList(0);
	updateList(1);
	if(Data.Since_.d == 0)
		setupTransmissionEvent(0);
	return 1;
}

int ObjTranDialog::getDTS(ObjTransmitParam * pData)
{
	int    ok = 1;
	if(!Data.DestDBDivList.GetCount())
		ok = PPErrorByDialog(this, CTL_OBJTRANSM_DBDIVLIST, PPERR_INVDESTDBDIV);
	else if(Id == DLG_MODTRANSM && !Data.ObjList.GetCount())
		ok = PPErrorByDialog(this, CTL_OBJTRANSM_OBJLIST, PPERR_INVOBJ);
	else {
		ushort v = 0;
		if(getCtrlData(CTL_OBJTRANSM_PROTOCOL, &v))
			switch(v) {
				case 0: Data.UpdProtocol = PPOTUP_NONE; break;
				case 1: Data.UpdProtocol = PPOTUP_BYTIME; break;
				case 2: Data.UpdProtocol = PPOTUP_FORCE; break;
			}
		v = getCtrlUInt16(CTL_OBJTRANSM_FLAGS);
		SETFLAG(Data.Flags, ObjTransmitParam::fSyncCmp, v & 0x01);
		getSince();
		GetClusterData(CTL_OBJTRANSM_TRNSMFLAGS, &Data.TrnsmFlags);
		if(Data.Flags & Data.fQueryInmassTransmission) {
			GetClusterData(CTL_OBJTRANSM_INMASS, &Data.Flags);
		}
		else {
			Data.Flags &= ~Data.fInmassTransmission;
		}
		ASSIGN_PTR(pData, Data);
	}
	return ok;
}

int STDCALL ObjTransmDialog(uint dlgID, ObjTransmitParam * pParam, long dlgFlags /*=0*/)
{
	DIALOG_PROC_BODY_P2(ObjTranDialog, dlgID, dlgFlags, pParam);
}

class ObjTranDialogExt : public ObjTranDialog {
public:
	ObjTranDialogExt(uint dlgID, int viewId, long dlgFlags) : ObjTranDialog(dlgID, dlgFlags), P_Filt(0), P_View(0)
	{
		PPView::CreateInstance(viewId, &P_View);
	}
	~ObjTranDialogExt()
	{
		ZDELETE(P_View);
		ZDELETE(P_Filt);
	}
	int setDTS(const ObjTransmitParam * pParam, const PPBaseFilt * pFilt);
	int getDTS(ObjTransmitParam * pParam, PPBaseFilt * pFilt);
private:
	PPBaseFilt * P_Filt;
	PPView * P_View;
	DECL_HANDLE_EVENT;
};

IMPL_HANDLE_EVENT(ObjTranDialogExt)
{
	ObjTranDialog::handleEvent(event);
	if(event.isCmd(cmEditFilt)) {
		if(P_Filt)
			P_View->EditBaseFilt(P_Filt);
		clearEvent(event);
	}
}

int ObjTranDialogExt::setDTS(const ObjTransmitParam * pParam, const PPBaseFilt * pFilt)
{
	ZDELETE(P_Filt);
	ObjTranDialog::setDTS(pParam);
	if(P_View && pFilt) {
		P_Filt = P_View->CreateFilt(PPView::GetDescriptionExtra(P_View->GetViewId()));
		if(P_Filt)
			(*P_Filt) = (*pFilt);
	}
	showCtrl(CTL_OBJTRANSM_FILT, LOGIC(P_View && pFilt));
	return 1;
}

int ObjTranDialogExt::getDTS(ObjTransmitParam * pParam, PPBaseFilt * pFilt)
{
	int    ok = ObjTranDialog::getDTS(pParam);
	if(ok > 0 && pFilt && P_Filt)
		(*pFilt) = (*P_Filt);
	return ok;
}

int STDCALL ObjTransmDialogExt(uint dlgID, int viewId, ObjTransmitParam * pParam, PPBaseFilt * pFilt, long dlgFlags /*=0*/)
{
	int    ok = -1;
	ObjTranDialogExt * dlg = new ObjTranDialogExt(dlgID, viewId, dlgFlags);
	if(CheckDialogPtrErr(&dlg) && dlg->setDTS(pParam, pFilt)) {
		while(ok <= 0 && ExecView(dlg) == cmOK)
			if(dlg->getDTS(pParam, pFilt))
				ok = 1;
			else
				PPError();
	}
	else
		ok = 0;
	delete dlg;
	return ok;
}

int PPObjectTransmit::StartReceivingPacket(const char * pFileName, const void * pHdr)
{
	int    ok = 1;
	const  PPObjectTransmit::Header * p_hdr = static_cast<const PPObjectTransmit::Header *>(pHdr);
	SString buf, msg_buf, temp_buf;
	PPLoadText(PPTXT_STARTRCVPACKET, buf);
	RecoverTransmission = 0;
	if(!p_hdr->DBID || DObj.Get(p_hdr->DBID, &SrcDbDivPack) <= 0) {
		temp_buf.Cat(p_hdr->DBID);
		SrcDbDivPack.Init();
		ok = -1;
	}
	else
		temp_buf = SrcDbDivPack.Rec.Name;
	msg_buf.Printf(buf, pFileName, temp_buf.cptr());
	Ctx.OutReceivingMsg(msg_buf);
	Ctx.P_SrcDbDivPack = &SrcDbDivPack;
	if(ok > 0 && p_hdr->MinDestVer.V) {
		SVerT cur_ver = DS.GetVersion();
		int    mj, mn, r;
		p_hdr->MinDestVer.Get(&mj, &mn, &r);
		if(cur_ver.IsLt(mj, mn, r)) {
			PPLoadText(PPTXT_RCVPACKETREJVER, buf);
			Ctx.OutReceivingMsg(msg_buf.Printf(buf, temp_buf.Z().CatDotTriplet(mj, mn, r).cptr()));
			ok = -1;
		}
		DS.GetMinCompatVersion().Get(&mj, &mn, &r);
		if(p_hdr->SwVer.IsLt(mj, mn, r)) {
			PPLoadText(PPTXT_RCVPACKETREJSRCVER, buf);
			Ctx.OutReceivingMsg(msg_buf.Printf(buf, temp_buf.Z().CatDotTriplet(mj, mn, r).cptr()));
			ok = -1;
		}
	}
	if(!SyncCmpTransmit) {
		ZDELETE(P_TmpIdxTbl);
		THROW(P_TmpIdxTbl = CreateTempIndex());
		SETFLAG(Ctx.Flags, ObjTransmContext::fConsolid, p_hdr->Flags & PPOTF_CONSOLID);
		if(p_hdr->Flags & PPOTF_RECOVER) {
			RecoverTransmission = 1;
			Ctx.Flags |= ObjTransmContext::fRecover;
		}
		else
			Ctx.Flags &= ~ObjTransmContext::fRecover;
	}
	CATCHZOK
	return ok;
}

int PPObjectTransmit::GetPrivateObjSyncData(PPID objType, PPCommSyncID commID, PPID * pPrimID, LDATETIME * pModDtm, char * pObjName, size_t bufLen)
{
	int    ok = -1;
	PPID   primary_id = 0;
	CALLPTRMEMB(pModDtm, Z());
	ASSIGN_PTR(pObjName, 0);
	int    r = SyncTbl.SearchCommonObj(objType, commID, &primary_id, 0);
	if(r > 0) {
		if(primary_id) {
			PPObject * ppobj = _GetObjectPtr(objType);
			if(pModDtm) {
				SysJournal * p_sj = DS.GetTLA().P_SysJ;
				int    is_cr = 0;
				LDATETIME mod;
				if(p_sj && p_sj->GetLastObjModifEvent(objType, primary_id, &mod, &is_cr) > 0)
					*pModDtm = mod;
			}
			if(ppobj && ppobj->GetName(primary_id, pObjName, bufLen) > 0)
				ok = 1;
			else
				ok = -3;
		}
		else
			ok = -2;
	}
	else if(r < 0)
		ok = -1;
	else
		ok = 0;
	ASSIGN_PTR(pPrimID, primary_id);
	return ok;
}

/*static*/int PPObjectTransmit::ReceivePackets(const ObjReceiveParam * pParam)
{
	int    ok = 1;
	int    next_pass = 0;
	int    r;
	int    is_locked = 0;
	const  long db_path_id = DBS.GetDbPathID();
	ObjReceiveParam param;
	if(!RVALUEPTR(param, pParam)) {
		PPDBXchgConfig cfg;
		PPObjectTransmit::ReadConfig(&cfg);
		if(!(cfg.Flags & DBDXF_NOCOMMITBYDEF))
			param.Flags |= ObjReceiveParam::fCommitQueue;
		if(cfg.Flags & DBDXF_DESTROYQUEUEBYDEF)
			param.Flags |= ObjReceiveParam::fForceDestroyQueue;
	}
	THROW(LockReceiving(0));
	DS.SetDbCacheDeferredState(db_path_id, 1);
	is_locked = 1;
	THROW(r = GetTransmitFiles(&param));
	if(r > 0) {
		char * p_fname = 0;
		SString temp_buf;
		SString file_path;
		SString ack_file_path;
		SStrCollection  flist;
		SFileEntryPool fep;
		SFileEntryPool::Entry fe;
		PPObjDBDiv dbdiv_obj;
		uint   ot_ctrf = 0;
		if(param.Flags & ObjReceiveParam::fSyncCmp)
			ot_ctrf |= PPObjectTransmit::ctrfSyncCmp;
		if(param.Flags & ObjReceiveParam::fDisableLogWindow)
			ot_ctrf |= PPObjectTransmit::ctrfDisableLogWindow;
		if(!(param.Flags & ObjReceiveParam::fNonInteractive)) {
			PPWaitStart();
		}
		PPObjectTransmit ot(PPObjectTransmit::tmReading, ot_ctrf);
		if(param.SsOnlyFileNames.getCount()) {
			for(uint ssp = 0; param.SsOnlyFileNames.get(&ssp, temp_buf);) {
				if(fileExists(temp_buf)) {
					fep.Add(temp_buf, 0);
				}
			}
		}
		else {
			SString pattern_pps;
			pattern_pps.CatChar('*').Cat(PPConst::FnExt_PPS);
			THROW(PPGetPath(PPPATH_IN, file_path));
			THROW(fep.Scan(file_path.SetLastSlash(), pattern_pps, 0));
		}
		do {
			next_pass = 0;
			for(uint p = 0; p < fep.GetCount(); p++) {
				if(fep.Get(p, &fe, &file_path) && SFile::WaitForWriteSharingRelease(file_path, 20000)) {
					PPObjectTransmit::Header hdr;
					if(ot.OpenInPacket(file_path, &hdr) > 0) {
						if(hdr.DestDBID == LConfig.DBDiv && param.CheckDbDivID(hdr.DBID)) {
							int    is_received = 0;
							int    do_accept_src_uuid = 0;
							PPWaitMsg(fe.Name);
							if(param.Flags & ObjReceiveParam::fSyncCmp) {
								if(hdr.PacketType == PPOT_SYNCCMP) {
									THROW(ot.StartReceivingPacket(fe.Name, &hdr));
									if(ot.RestoreFromStream(file_path, ot.P_InStream, pParam->P_SyncCmpTbl)) {
										THROW_MEM(p_fname = newStr(file_path));
										THROW_SL(flist.insert(p_fname));
										do_accept_src_uuid = 1;
									}
								}
							}
							else {
								if(hdr.PacketType == PPOT_OBJ) {
									THROW(r = ot.StartReceivingPacket(fe.Name, &hdr));
									if(r > 0) {
										if(ot.RestoreFromStream(file_path, ot.P_InStream, 0)) {
											is_received = 1;
											do_accept_src_uuid = 1;
										}
										ZDELETE(ot.P_TmpIdxTbl);
									}
								}
								else if(hdr.PacketType == PPOT_ACK) {
									if(!(ot.Ctx.Cfg.Flags & DBDXF_IGNOREACK)) {
										THROW(ot.CommitAck());
										do_accept_src_uuid = 1;
									}
									is_received = 1;
								}
								if(!next_pass && is_received) {
									THROW_MEM(p_fname = newStr(file_path));
									THROW_SL(flist.insert(p_fname));
								}
							}
							if(do_accept_src_uuid) {
								if(hdr.SwVer.IsGt(8, 0, 11) && !hdr.SrcDivUuid.IsZero()) {
									THROW(dbdiv_obj.AcceptUuid(hdr.DBID, hdr.SrcDivUuid, 1));
								}
							}
						}
						ot.CloseInPacket();
					}
				}
			}
		} while(next_pass);
		if(param.Flags & ObjReceiveParam::fClearInpAfter) {
			for(uint i = 0; flist.enumItems(&i, (void **)&p_fname);) {
				PPBackupOperationFile(p_fname, "ppos-backup", 0); // @v10.7.5
				SFile::Remove(p_fname);
			}
		}
		if(param.Flags & ObjReceiveParam::fCommitQueue) {
			//SString flagsval = "flags: ";
			//Profile *p_profile = StartUserProfile(PPFILNAM_USERPROFILE_LOG, "PPObjectTransmit", flagsval.Cat(param.Flags));
			THROW(ot.CommitQueue(param.SenderDbDivList, BIN(param.Flags & ObjReceiveParam::fForceDestroyQueue)));
			//FinishUserProfile(p_profile, PPFILNAM_USERPROFILE_LOG, "PPObjectTransmit", flagsval.Cat(param.Flags));
		}
		if(!(param.Flags & ObjReceiveParam::fNonInteractive)) {
			PPWaitStop();
		}
	}
	else
		ok = -1;
	CATCH
		if(param.Flags & param.fNonInteractive) {
			PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR_TIME_USER|LOGMSGF_DBINFO);
		}
		else
			PPError();
	ENDCATCH
	if(is_locked) {
		DS.SetDbCacheDeferredState(db_path_id, 0);
		LockReceiving(1);
	}
	return ok;
}

/*static*/int FASTCALL PPObjectTransmit::LockReceiving(int unlock)
{
	int    ok = 1;
	if(!unlock) {
		long   mutex_id = 0;
		PPSyncItem sync_item;
		int    r = DS.GetSync().CreateMutex_(LConfig.SessionID, PPOBJ_DBXCHG, 1L, &mutex_id, &sync_item);
		if(r < 0)
			ok = PPSetError(PPERR_DBXCHGRCVISLOCKED, sync_item.Name);
		else if(!r)
			ok = 0;
	}
	else
		ok = DS.GetSync().ReleaseMutex(PPOBJ_DBXCHG, 1L);
	return ok;
}
//
//
//
struct SelfSyncParam {
	enum {
		fDontSyncBills = 0x0001
	};
	PPID   DestDBID;
	uint   Flags;
};

static int SelfSyncDialog(SelfSyncParam * pParam)
{
	int    ok = -1;
	ushort v;
	PPID   dest_db_id = pParam->DestDBID;
	TDialog * dlg = new TDialog(DLG_SELFSYNC);
	if(CheckDialogPtrErr(&dlg)) {
		SetupPPObjCombo(dlg, CTLSEL_SELFSYNC_DBDIV, PPOBJ_DBDIV, dest_db_id, 0, 0);
		v = 0;
		if(pParam->Flags & SelfSyncParam::fDontSyncBills)
			v = 1;
		dlg->setCtrlData(CTL_SELFSYNC_FLAGS, &v);
		if(ExecView(dlg) == cmOK) {
			dlg->getCtrlData(CTL_SELFSYNC_FLAGS, &(v = 0));
			SETFLAG(pParam->Flags, SelfSyncParam::fDontSyncBills, v & 1);
			dlg->getCtrlData(CTLSEL_SELFSYNC_DBDIV, &dest_db_id);
			if(dest_db_id == 0)
				ok = -1;
			else {
				pParam->DestDBID = dest_db_id;
				ok = 1;
			}
		}
		delete dlg;
	}
	else
		ok = 0;
	return ok;
}

static int FASTCALL SyncRefObj(ObjSyncCore * pSyncTbl, PPID obj, PPID dest)
{
	int    ok = 1;
	int    r;
	Reference * p_ref = PPRef;
	//SString msg_buf;
	PPWaitMsg(GetObjectTitle(obj, SLS.AcquireRvlStr())); // @v11.2.12 msg_buf-->SLS.AcquireRvlStr()
	for(PPID id = 0; (r = p_ref->EnumItems(obj, &id)) > 0;)
		THROW(pSyncTbl->SelfSync(obj, id, dest, 0));
	THROW(r);
	CATCHZOK
	return ok;
}

static int FASTCALL SyncTblObj(ObjSyncCore * sync, DBTable * tbl, PPID obj, PPID dest)
{
	int    ok = 1;
	SString msg_buf;
	IterCounter cntr;
	PPInitIterCounter(cntr, tbl);
	GetObjectTitle(obj, msg_buf);
	for(PPID id = 0; tbl->search(0, &id, spGt); PPWaitPercent(cntr.Increment(), msg_buf))
		THROW(sync->SelfSync(obj, id, dest, 0));
	CATCHZOK
	return ok;
}

static int SyncGoodsObjs(ObjSyncCore * sync, Goods2Tbl * tbl, PPID dest)
{
	int    ok = 1;
	SString msg_buf;
	IterCounter cntr;
	PPInitIterCounter(cntr, tbl);
	GetObjectTitle(PPOBJ_GOODS, msg_buf);
	for(PPID id = 0; tbl->search(0, &id, spGt); PPWaitPercent(cntr.Increment(), msg_buf)) {
		PPID   obj_type = 0;
		switch(tbl->data.Kind) {
			case PPGDSK_GOODS:     obj_type = PPOBJ_GOODS;      break;
			case PPGDSK_GROUP:     obj_type = PPOBJ_GOODSGROUP; break;
			case PPGDSK_TRANSPORT: obj_type = PPOBJ_TRANSPORT;  break;
			case PPGDSK_BRAND:     obj_type = PPOBJ_BRAND;      break;
		}
		if(obj_type)
			THROW(sync->SelfSync(obj_type, id, dest, 0));
	}
	CATCHZOK
	return ok;
}

int SynchronizeObjects(PPID dest)
{
	int    ok = 1;
	SelfSyncParam param;
	MEMSZERO(param);
	param.DestDBID = dest;
	if(dest == 0 && SelfSyncDialog(&param) <= 0)
		ok = -1;
	else {
		PPDBXchgConfig dbxcfg;
		PPObjectTransmit::ReadConfig(&dbxcfg);
		dest = param.DestDBID;
		PPWaitStart();
		ObjSyncCore & r_sync = *DS.GetTLA().P_ObjSync;
		PPObjLocation   loc_obj;
		PPObjGoods      gobj;
		PPObjPerson     pobj;
		PPObjArticle    part;
		PPObjAccount    pact;
		PPObjQCert      qcobj;
		PPObjSCard      scobj;
		PPObjProject    prj_obj;
		PPObjPrjTask    todo_obj;
		PPObjWorld      world_obj;
		PPObjCSession   cses_obj;
		PPObjTSession   tses_obj;
		PPObjProcessor  prc_obj;
		PPObjTech       tec_obj;
		PPObjBill * p_bobj = BillObj;
		PPIDArray ref_obj_list;
		{
			PPTransaction tra(1);
			THROW(tra);
			{
				ref_obj_list.addzlist(
					PPOBJ_ACCSHEET,
					PPOBJ_ADVBILLKIND,
					PPOBJ_AMOUNTTYPE,
					PPOBJ_ASSTWROFFGRP,
					PPOBJ_BCODESTRUC,
					PPOBJ_BILLSTATUS,
					PPOBJ_BNKACCTYPE,
					PPOBJ_CASHNODE,
					PPOBJ_CITYSTATUS,
					PPOBJ_CURRENCY,
					PPOBJ_DEBTDIM,
					PPOBJ_DUTYSCHED,
					PPOBJ_DYNAMICOBJS,
					PPOBJ_ELINKKIND,
					PPOBJ_EVENTTOKEN,
					PPOBJ_GLOBALUSERACC,
					PPOBJ_GOODSCLASS,
					PPOBJ_GOODSTAX,
					PPOBJ_GOODSTYPE,
					PPOBJ_GOODSVALRESTR,
					PPOBJ_INTERNETACCOUNT,
					PPOBJ_NAMEDOBJASSOC,
					PPOBJ_OPCOUNTER,
					PPOBJ_OPRKIND,
					PPOBJ_OPRTYPE,
					PPOBJ_PALLET,
					PPOBJ_PERSONRELTYPE,
					PPOBJ_PHONESERVICE,
					PPOBJ_PRICETYPE,
					PPOBJ_PRSNCATEGORY,
					PPOBJ_PERSONKIND,
					PPOBJ_PRSNSTATUS,
					PPOBJ_QUOTKIND,
					PPOBJ_REGISTERTYPE,
					PPOBJ_SALCHARGE,
					PPOBJ_SCARDSERIES,
					PPOBJ_SMSPRVACCOUNT,
					PPOBJ_STAFFCAL,
					PPOBJ_STAFFRANK,
					PPOBJ_TAG,
					PPOBJ_UNIT,
					PPOBJ_ACCOUNT2,
					PPOBJ_FREIGHTPACKAGETYPE, // @v10.7.5
					PPOBJ_TAXSYSTEMKIND,      // @v10.7.5
					0);
				// @v10.1.5 {
				if(dbxcfg.Flags & DBDXF_SYNCUSRANDGRPS) {
					ref_obj_list.add(PPOBJ_USR);
					ref_obj_list.add(PPOBJ_USRGRP);
				}
				// } @v10.1.5
				for(PPID dyn_obj_type = 0; PPRef->EnumItems(PPOBJ_DYNAMICOBJS, &dyn_obj_type) > 0;)
					ref_obj_list.addUnique(dyn_obj_type);
				for(uint i = 0; i < ref_obj_list.getCount(); i++)
					THROW(SyncRefObj(&r_sync, ref_obj_list.get(i), dest));
			}
			THROW(SyncTblObj(&r_sync, part.P_Tbl,      PPOBJ_ARTICLE,    dest));
			THROW(SyncTblObj(&r_sync, world_obj.P_Tbl, PPOBJ_WORLD,      dest));
			THROW(SyncTblObj(&r_sync, loc_obj.P_Tbl,   PPOBJ_LOCATION,   dest));
			THROW(SyncTblObj(&r_sync, pobj.P_Tbl,      PPOBJ_PERSON,     dest));
			THROW(SyncTblObj(&r_sync, scobj.P_Tbl,     PPOBJ_SCARD,      dest));
			if(!(param.Flags & SelfSyncParam::fDontSyncBills)) {
				THROW(SyncTblObj(&r_sync, p_bobj->P_Tbl, PPOBJ_BILL, dest));
				if(CConfig.Flags2 & CCFLG2_SYNCLOT) {
					THROW(SyncTblObj(&r_sync, &p_bobj->trfr->Rcpt, PPOBJ_LOT, dest));
				}
			}
			THROW(SyncTblObj(&r_sync, qcobj.P_Tbl,    PPOBJ_QCERT,   dest));
			THROW(SyncTblObj(&r_sync, prj_obj.P_Tbl,  PPOBJ_PROJECT, dest));
			THROW(SyncTblObj(&r_sync, todo_obj.P_Tbl, PPOBJ_PRJTASK, dest));
			THROW(SyncGoodsObjs(&r_sync, gobj.P_Tbl, dest));
			THROW(SyncTblObj(&r_sync, cses_obj.P_Tbl, PPOBJ_CSESSION, dest));
			THROW(SyncTblObj(&r_sync, prc_obj.P_Tbl,  PPOBJ_PROCESSOR, dest));
			THROW(SyncTblObj(&r_sync, tec_obj.P_Tbl,  PPOBJ_TECH,      dest));
			THROW(SyncTblObj(&r_sync, tses_obj.P_Tbl, PPOBJ_TSESSION,  dest));
			THROW(tra.Commit());
		}
	}
	CATCHZOKPPERR
	PPWaitStop();
	return ok;
}
