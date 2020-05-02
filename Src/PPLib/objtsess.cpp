// OBJTSESS.CPP
// Copyright (c) A.Sobolev 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020
// @codepage UTF-8
//
#include <pp.h>
#pragma hdrstop
//
// @ModuleDef(PPObjTSession)
//
#define PRCFLAG     0x08000000L
#define PLANFLAG    0x04000000L
#define SUBSESSFLAG 0x02000000L
#define SPCFLAGMASK 0x0f000000L
#define NOPRCVAL    999999L
//
//
//
class BhtTSess : SVector { // @v9.8.11 SArray-->SVector
public:
	SLAPI  BhtTSess(PPLogger * pLogger) : SVector(sizeof(PSE)), LastUsedEntryPos(UINT_MAX), P_Logger(pLogger), Ta(0), LastLine_SessID(0), LastLine_OprNo(0)
	{
	}
	void   SLAPI Reset()
	{
		freeAll();
		LastUsedEntryPos = UINT_MAX;
	}
	int    SLAPI SearchByPrc(PPID prcID, uint * pPos) const { return lsearch(&prcID, pPos, CMPF_LONG, offsetof(PSE, PrcID)); }
	int    SLAPI SearchByBill(PPID billID, uint * pPos) const { return lsearch(&billID, pPos, CMPF_LONG, offsetof(PSE, BillID)); }
	int    SLAPI SearchBySess(PPID sessID, uint * pPos) const { return lsearch(&sessID, pPos, CMPF_LONG, offsetof(PSE, SessID)); }
	PPID   SLAPI GetLastUsedSessID() const { return (LastUsedEntryPos < getCount()) ? static_cast<const PSE *>(at(LastUsedEntryPos))->SessID : 0; }
	int    SLAPI Add(const TSessionTbl::Rec * pSessRec, int isProper);
	void   SLAPI SetLastLine(PPID sessID, long oprNo)
	{
		LastLine_SessID = sessID;
		LastLine_OprNo = oprNo;
	}
	int    SLAPI RemoveLastLine(int use_ta)
	{
		int    ok = 1;
		if(LastLine_SessID && LastLine_OprNo) {
			if(!TSesObj.PutLine(LastLine_SessID, &LastLine_OprNo, 0, use_ta))
				ok = 0;
		}
		else
			ok = -1;
		return ok;
	}
	int    SLAPI SelectSession(const BhtTSessRec * pRec);
	int    SLAPI Finish();
	int    SLAPI IsDupSerialAllowed()
	{
		if(LastUsedEntryPos < getCount()) {
			const PSE & entry = Get(LastUsedEntryPos);
			ProcessorTbl::Rec prc_rec;
			if(TSesObj.GetPrc(entry.PrcID, &prc_rec, 1, 1) && prc_rec.Flags & PRCF_ACCDUPSERIALINSESS)
				return 1;
		}
		return 0;
	}

	int    Ta;
private:
	enum {
		fProperSess = 0x0001, // Сессия создана этим сеансом обмена с терминалом
		fSwitchable = 0x0002  // Процессор, с которым связан элемент, допускает переключение
	};
	struct PSE { // @flat
		PPID   SessID;
		PPID   PrcID;
		PPID   ArID;
		PPID   BillID;
		long   Flags;
	};
	PSE &  SLAPI Get(uint pos) const
	{
		return *static_cast<BhtTSess::PSE *>(at(pos));
	}
	int    SLAPI CreateSess(PPID * pSessID, const BhtTSessRec * pRec, const ProcessorTbl::Rec * pPrcRec);
	int    SLAPI CloseSess(uint entryPos);
	int    SLAPI SwitchPrc(ProcessorTbl::Rec * pPrcRec, PPID destPrcID);
	uint   LastUsedEntryPos;
	PPID   LastLine_SessID;
	PPID   LastLine_OprNo;
	PPObjTSession TSesObj;
	PPLogger * P_Logger;
};
//
//
//
void SLAPI PPObjTSession::PlaceStatus::Init()
{
	TSessID = 0;
	Status = 0;
	GoodsID = 0;
	RegPersonID = 0;
	CipID = 0;
	Price = 0.0;
	PlaceCode.Z();
	Descr.Z();
	PinCode.Z();
}
//
//
//
SLAPI PPTSessConfig::PPTSessConfig()
{
	THISZERO();
}

IMPL_INVARIANT_C(PPTSessConfig)
{
	S_INVARIANT_PROLOG(pInvP);
	S_ASSERT_P(MinIdleCont >= 0 && MinIdleCont < 86400L, pInvP);
	S_ASSERT_P(ViewRefreshPeriod >= 0 && ViewRefreshPeriod < 86400L, pInvP);
	S_INVARIANT_EPILOG(pInvP);
}

// @vmiller
struct Storage_PPTSessionConfig { // @persistent @store(PropertyTbl)
	size_t GetSize() const { return (sizeof(*this) + ExtStrSize); }
	PPID   Tag;             // Const=PPOBJ_CONFIG
	PPID   ID;              // Const=PPCFG_MAIN
	PPID   Prop;            // Const=PPPRP_TSESSCFG
	long   Flags;           // PPTSessConfig::fXXX
	PPID   IdleAccSheetID;  // ->Ref(PPOBJ_ACCSHEET) @v4.9.12 Таблица статей видов простоев процессоров
	long   MinIdleCont;     // Минимальная продолжительность простоя (sec). Если простой меньше,
		// этой величины, то он не регистрируется (сессия простоя удаляется).
	LTIME  InitTime;        // @v5.0.6 Время, задаваемое в новых техн сессиях по умолчанию. Используется //
		// также как расчетное время для сессий, учитывающих товар по времени (гостиницы)
	long   RoundPeriod;     // @v5.0.6 Период округления (в большую сторону) для сессий, учитывающих
		// товар по времени
	long   ViewRefreshPeriod; // @v5.0.8 Период обновления таблиц техн сессий и строк техн сессий (sec)
	long   TimeChunkBrowserQuant; // @5.6.2 Квант времени (сек) во временной диаграмме
	long   ColorPlannedStatus;
	long   ColorPendingStatus;
	long   ColorInProgressStatus;
	long   ColorClosedStatus;
	long   ColorCanceledStatus;
	SVerT Ver;
	uint16 ExtStrSize;        // Размер "хвоста" под строки расширения. Общий размер записи, хранимой в БД
		// равен sizeof(PPTSessionConfig) + ExtStrSiz
	uint16 SmsConfigPos;
	PPID   DefTimeTechID;
	char   Reserve[12];
};

// @vmiller
//static
int FASTCALL PPObjTSession::WriteConfig(PPTSessConfig * pCfg, int use_ta)
{
	const  long prop_cfg_id = PPPRP_TSESSCFG;
	const  long cfg_obj_type = PPCFGOBJ_TECHSESS;
	int    ok = 1, is_new = 0, r;
	size_t sz = sizeof(Storage_PPTSessionConfig);
	Storage_PPTSessionConfig * p_cfg = 0;
	SSerializeContext sctx;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(r = PPRef->GetPropMainConfig(prop_cfg_id, 0, 0));
		is_new = (r > 0) ? 0 : 1;
		if(pCfg) {
			SBuffer buf;
			// @vmiller
			if(!pCfg->SmsConfig.IsEmpty()) {
				pCfg->SmsConfig.Serialize(+1, buf, &sctx);
			}
			const size_t ext_size = buf.GetAvailableSize();
			sz += ext_size;
			THROW_MEM(p_cfg = static_cast<Storage_PPTSessionConfig *>(SAlloc::M(sz)));
			memzero(p_cfg, sz);
			p_cfg->Tag = PPOBJ_CONFIG;
			p_cfg->ID = PPCFG_MAIN;
			p_cfg->Prop = PPPRP_TSESSCFG;
			p_cfg->Flags = pCfg->Flags;
			p_cfg->IdleAccSheetID = pCfg->IdleAccSheetID;
			p_cfg->MinIdleCont = pCfg->MinIdleCont;
			p_cfg->InitTime = pCfg->InitTime;
			p_cfg->RoundPeriod = pCfg->RoundPeriod;
			p_cfg->ViewRefreshPeriod = pCfg->ViewRefreshPeriod;
			p_cfg->TimeChunkBrowserQuant = pCfg->TimeChunkBrowserQuant;
			p_cfg->ColorPlannedStatus = pCfg->ColorPlannedStatus;
			p_cfg->ColorPendingStatus = pCfg->ColorPendingStatus;
			p_cfg->ColorInProgressStatus = pCfg->ColorInProgressStatus;
			p_cfg->ColorClosedStatus = pCfg->ColorClosedStatus;
			p_cfg->ColorCanceledStatus = pCfg->ColorCanceledStatus;
			p_cfg->Ver = DS.GetVersion();
			p_cfg->DefTimeTechID = pCfg->DefTimeTechID;
			p_cfg->SmsConfigPos = (uint16)sizeof(Storage_PPTSessionConfig);
			p_cfg->ExtStrSize = (uint16)ext_size;
			if(ext_size)
				THROW_SL(buf.Read(PTR8(p_cfg) + p_cfg->SmsConfigPos, ext_size));
			assert(p_cfg->GetSize() == sz);
		}
		THROW(PPObject::Helper_PutConfig(prop_cfg_id, cfg_obj_type, is_new, p_cfg, sz, 0));
		THROW(tra.Commit());
	}
	CATCHZOK
	SAlloc::F(p_cfg);
	return ok;
}

// @vmiller
//static
int FASTCALL PPObjTSession::ReadConfig(PPTSessConfig * pCfg)
{
	const  long prop_cfg_id = PPPRP_TSESSCFG;
	int    ok = -1, r;
	Reference * p_ref = PPRef;
	size_t sz = 0;
	Storage_PPTSessionConfig * p_cfg = 0;
	if(p_ref->GetPropActualSize(PPOBJ_CONFIG, PPCFG_MAIN, prop_cfg_id, &sz) > 0) {
		p_cfg = static_cast<Storage_PPTSessionConfig *>(SAlloc::M(sz));
		THROW_MEM(p_cfg);
		THROW(r = p_ref->GetPropMainConfig(prop_cfg_id, p_cfg, sz));
		if(r > 0 && p_cfg->GetSize() > sz) {
			sz = p_cfg->GetSize();
			p_cfg = static_cast<Storage_PPTSessionConfig *>(SAlloc::R(p_cfg, sz));
			THROW_MEM(p_cfg);
			THROW(r = p_ref->GetPropMainConfig(prop_cfg_id, p_cfg, sz));
		}
		if(r > 0) {
			pCfg->Flags = p_cfg->Flags;
			pCfg->IdleAccSheetID = p_cfg->IdleAccSheetID;
			pCfg->MinIdleCont = p_cfg->MinIdleCont;
			pCfg->InitTime = p_cfg->InitTime;
			pCfg->RoundPeriod = p_cfg->RoundPeriod;
			pCfg->ViewRefreshPeriod = p_cfg->ViewRefreshPeriod;
			pCfg->TimeChunkBrowserQuant = p_cfg->TimeChunkBrowserQuant;
			pCfg->ColorPlannedStatus = p_cfg->ColorPlannedStatus;
			pCfg->ColorPendingStatus = p_cfg->ColorPendingStatus;
			pCfg->ColorInProgressStatus = p_cfg->ColorInProgressStatus;
			pCfg->ColorClosedStatus = p_cfg->ColorClosedStatus;
			pCfg->ColorCanceledStatus = p_cfg->ColorCanceledStatus;
			pCfg->Ver = p_cfg->Ver;
			pCfg->DefTimeTechID = p_cfg->DefTimeTechID;
			if(p_cfg->SmsConfigPos && p_cfg->ExtStrSize) {
				SBuffer buf;
				SSerializeContext sctx;
				buf.Write(PTR8C(p_cfg) + p_cfg->SmsConfigPos, p_cfg->ExtStrSize);
				THROW(pCfg->SmsConfig.Serialize(-1, buf, &sctx));
			}
			ok = 1;
		}
		else {
			pCfg->Flags = 0;
			pCfg->IdleAccSheetID = 0;
			pCfg->MinIdleCont = 0;
			pCfg->InitTime = 0;
			pCfg->RoundPeriod = 0;
			pCfg->ViewRefreshPeriod = 0;
			pCfg->TimeChunkBrowserQuant = 0;
			pCfg->ColorPlannedStatus = 0;
			pCfg->ColorPendingStatus = 0;
			pCfg->ColorInProgressStatus = 0;
			pCfg->ColorClosedStatus = 0;
			pCfg->ColorCanceledStatus = 0;
			pCfg->DefTimeTechID = 0;
			ok = -1;
		}
	}
	CATCHZOK
	SAlloc::F(p_cfg);
	return ok;
}


////static
//int SLAPI PPObjTSession::ReadConfig(PPTSessConfig * pCfg)
//{
//	int    r = PPRef->GetPropMainConfig(PPPRP_TSESSCFG, pCfg, sizeof(*pCfg));
//	if(r <= 0)
//		memzero(pCfg, sizeof(*pCfg));
//	return r;
//}

#define GRP_PLANNED 1
#define GRP_PENDING 2
#define GRP_INPROGR 3
#define GRP_CLOSED  4
#define GRP_CANCEL  5

#define GRPS_COUNT  5

class TSessStatusColorsDialog : public TDialog {
private:
	struct Group {
		void Init(uint grp, uint ctl, uint ctlSel, uint cmd, uint cmdCtl, long * pColor)
		{
			Grp = grp; Ctl = ctl; CtlSel = ctlSel; Cmd = cmd; CmdCtl = cmdCtl; P_Color = pColor;
		}
		uint   Grp;
		uint   Ctl;
		uint   CtlSel;
		uint   Cmd;
		uint   CmdCtl;
		long * P_Color;
	};
	Group Grps[GRPS_COUNT];
public:
	TSessStatusColorsDialog() : TDialog(DLG_TSESSCLRS)
	{
		Grps[0].Init(GRP_PLANNED, CTL_TSESSCLRS_PLANNED, CTLSEL_TSESSCLRS_PLANNED, cmChooseColor, CTL_TSESSCLRS_PLANNEDNEW, &Data.ColorPlannedStatus);
		Grps[1].Init(GRP_PENDING, CTL_TSESSCLRS_PENDING, CTLSEL_TSESSCLRS_PENDING, cmChooseColor, CTL_TSESSCLRS_PENDINGNEW, &Data.ColorPendingStatus);
		Grps[2].Init(GRP_INPROGR, CTL_TSESSCLRS_INPROGR, CTLSEL_TSESSCLRS_INPROGR, cmChooseColor, CTL_TSESSCLRS_INPROGRNEW, &Data.ColorInProgressStatus);
		Grps[3].Init(GRP_CLOSED,  CTL_TSESSCLRS_CLOSED,  CTLSEL_TSESSCLRS_CLOSED,  cmChooseColor, CTL_TSESSCLRS_CLOSEDNEW,  &Data.ColorClosedStatus);
		Grps[4].Init(GRP_CANCEL,  CTL_TSESSCLRS_CANCEL,  CTLSEL_TSESSCLRS_CANCEL,  cmChooseColor, CTL_TSESSCLRS_CANCELNEW,  &Data.ColorCanceledStatus);
		for(uint i = 0; i < SIZEOFARRAY(Grps); i++)
			addGroup(Grps[i].Grp, new ColorCtrlGroup(Grps[i].Ctl, Grps[i].CtlSel, Grps[i].Cmd, Grps[i].CmdCtl));
	}
	int    setDTS(const PPTSessConfig * pData)
	{
		ColorCtrlGroup::Rec grp_rec;
		RVALUEPTR(Data, pData);
		grp_rec.SetupStdColorList();
		for(uint i = 0; i < SIZEOFARRAY(Grps); i++) {
			grp_rec.C = *(Grps[i].P_Color);
			setGroupData(Grps[i].Grp, &grp_rec);
		}
		return 1;
	}
	int    getDTS(PPTSessConfig * pData)
	{
		for(uint i = 0; i < SIZEOFARRAY(Grps); i++) {
			ColorCtrlGroup::Rec grp_rec;
			getGroupData(Grps[i].Grp, &grp_rec);
			ASSIGN_PTR(Grps[i].P_Color, grp_rec.C);
		}
		ASSIGN_PTR(pData, Data);
		return 1;
	}
private:
	PPTSessConfig Data;
};

// @vmiller {
class TSessAutoSmsParamsDialog : public TDialog {
public:
	TSessAutoSmsParamsDialog() : TDialog(DLG_TSASMS)
	{
		FileBrowseCtrlGroup::Setup(this, CTLBRW_TSASMS_FILENAME, CTL_TSASMS_TDDOPATH, 1, 0, PPTXT_FILPAT_TDDO, FileBrowseCtrlGroup::fbcgfFile);
	}
	int    setDTS(const PPTSessConfig * pData);
	int    getDTS(PPTSessConfig * pData);
private:
	PPTSessConfig Data;
};

int TSessAutoSmsParamsDialog::setDTS(const PPTSessConfig * pData)
{
	RVALUEPTR(Data, pData);
	setCtrlString(CTL_TSASMS_TDDOPATH,  Data.SmsConfig.TddoPath);
	AddClusterAssoc(CTL_TSASMS_DAYS, 0, PPAutoSmsConfig::asmsDaysMon);
	AddClusterAssoc(CTL_TSASMS_DAYS, 1, PPAutoSmsConfig::asmsDaysTue);
	AddClusterAssoc(CTL_TSASMS_DAYS, 2, PPAutoSmsConfig::asmsDaysWed);
	AddClusterAssoc(CTL_TSASMS_DAYS, 3, PPAutoSmsConfig::asmsDaysThu);
	AddClusterAssoc(CTL_TSASMS_DAYS, 4, PPAutoSmsConfig::asmsDaysFri);
	AddClusterAssoc(CTL_TSASMS_DAYS, 5, PPAutoSmsConfig::asmsDaysSat);
	AddClusterAssoc(CTL_TSASMS_DAYS, 6, PPAutoSmsConfig::asmsDaysSun);
	SetClusterData(CTL_TSASMS_DAYS, Data.SmsConfig.AllowedWeekDays);
	setCtrlData(CTL_TSASMS_STTM,    &Data.SmsConfig.AllowedStartTm);
	setCtrlData(CTL_TSASMS_FNTM,    &Data.SmsConfig.AllowedEndTm);
	return 1;
}

int TSessAutoSmsParamsDialog::getDTS(PPTSessConfig * pData)
{
	long allowed_week_days = 0;
 	getCtrlString(CTL_TSASMS_TDDOPATH, Data.SmsConfig.TddoPath);
	GetClusterData(CTL_TSASMS_DAYS, &allowed_week_days);
	Data.SmsConfig.AllowedWeekDays = (uint16)allowed_week_days;
	getCtrlData(CTL_TSASMS_STTM,    &Data.SmsConfig.AllowedStartTm);
	getCtrlData(CTL_TSASMS_FNTM,    &Data.SmsConfig.AllowedEndTm);
	ASSIGN_PTR(pData, Data);
	return 1;
}

// } @vmiller

class TSessCfgDialog : public TDialog {
	DECL_DIALOG_DATA(PPTSessConfig);
public:
	TSessCfgDialog() : TDialog(DLG_TSESSCFG)
	{
	}
	DECL_DIALOG_SETDTS()
	{
		RVALUEPTR(Data, pData);
		AddClusterAssoc(CTL_TSESSCFG_FLAGS, 0, PPTSessConfig::fUpdateTimeOnStatus);
		AddClusterAssoc(CTL_TSESSCFG_FLAGS, 1, PPTSessConfig::fUsePricing);
		AddClusterAssoc(CTL_TSESSCFG_FLAGS, 2, PPTSessConfig::fAllowLinesInPendingSessions);
		AddClusterAssoc(CTL_TSESSCFG_FLAGS, 3, PPTSessConfig::fAllowLinesInWrOffSessions);
		AddClusterAssoc(CTL_TSESSCFG_FLAGS, 4, PPTSessConfig::fSnapInTimeChunkBrowser);
		AddClusterAssoc(CTL_TSESSCFG_FLAGS, 5, PPTSessConfig::fUpdLinesByAutocompl);
		AddClusterAssoc(CTL_TSESSCFG_FLAGS, 6, PPTSessConfig::fFreeGoodsSelection); // @v9.3.4
		AddClusterAssoc(CTL_TSESSCFG_FLAGS, 7, PPTSessConfig::fSetupCcPricesInCPane); // @v9.9.7
		SetClusterData(CTL_TSESSCFG_FLAGS, Data.Flags);
		setCtrlData(CTL_TSESSCFG_MINIDLE, &Data.MinIdleCont);
		SetupPPObjCombo(this, CTLSEL_TSESSCFG_IDLEAS, PPOBJ_ACCSHEET, Data.IdleAccSheetID, OLW_CANINSERT, 0);
		SetupPPObjCombo(this, CTLSEL_TSESSCFG_DEFTMTEC, PPOBJ_TECH, Data.DefTimeTechID, OLW_CANINSERT, 0);
		setCtrlData(CTL_TSESSCFG_INITTIME,    &Data.InitTime);
		setCtrlData(CTL_TSESSCFG_ROUNDPERIOD, &Data.RoundPeriod);
		setCtrlData(CTL_TSESSCFG_VIEWREFRESH, &Data.ViewRefreshPeriod);
		setCtrlData(CTL_TSESSCFG_TCBQUANT,    &Data.TimeChunkBrowserQuant);
		return 1;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		SInvariantParam invp;
		getCtrlData(CTLSEL_TSESSCFG_IDLEAS,   &Data.IdleAccSheetID);
		getCtrlData(CTLSEL_TSESSCFG_DEFTMTEC, &Data.DefTimeTechID);
		GetClusterData(CTL_TSESSCFG_FLAGS,    &Data.Flags);
		getCtrlData(CTL_TSESSCFG_MINIDLE,     &Data.MinIdleCont);
		getCtrlData(CTL_TSESSCFG_INITTIME,    &Data.InitTime);
		getCtrlData(CTL_TSESSCFG_ROUNDPERIOD, &Data.RoundPeriod);
		getCtrlData(CTL_TSESSCFG_VIEWREFRESH, &Data.ViewRefreshPeriod);
		getCtrlData(CTL_TSESSCFG_TCBQUANT,    &Data.TimeChunkBrowserQuant);
		THROW_PP(Data.InvariantC(&invp), PPERR_USERINPUT);
		ASSIGN_PTR(pData, Data);
		CATCHZOK
		return ok;
	}
private:
	DECL_HANDLE_EVENT
	{
		TDialog::handleEvent(event);
		if(event.isCmd(cmTSessStatusColors))
			PPDialogProcBody <TSessStatusColorsDialog, PPTSessConfig> (&Data);
		else if(event.isCmd(cmTSSetAutoSmsParam)) // @vmiller
			PPDialogProcBody <TSessAutoSmsParamsDialog, PPTSessConfig> (&Data);
		else
			return;
		clearEvent(event);
	}
};
//
//
//
SLAPI TSessionPacket::TSessionPacket()
{
	destroy();
}

void SLAPI TSessionPacket::destroy()
{
	Flags = 0;
	MEMSZERO(Rec);
	CiList.Init(PPCheckInPersonItem::kTSession, 0);
	Lines.clear();
	TagL.Destroy();
	Ext.destroy();
}
//
//
//
//static
int SLAPI PPObjTSession::EditConfig()
{
	int    ok = -1, valid_data = 0, is_new = 0;
	PPTSessConfig cfg;
	TSessCfgDialog * p_dlg = new TSessCfgDialog;
	THROW(CheckCfgRights(PPCFGOBJ_TECHSESS, PPR_READ, 0));
	THROW(is_new = ReadConfig(&cfg));
	THROW(CheckDialogPtr(&p_dlg));
	p_dlg->setDTS(&cfg);
	for(valid_data = 0; !valid_data && ExecView(p_dlg) == cmOK;) {
		THROW(CheckCfgRights(PPCFGOBJ_TECHSESS, PPR_MOD, 0));
		if(p_dlg->getDTS(&cfg) > 0) {
			THROW(WriteConfig(&cfg, 1)); // @vmiller
			//THROW(PPObject::Helper_PutConfig(PPPRP_TSESSCFG, PPCFGOBJ_TECHSESS, (is_new < 0), &cfg, sizeof(cfg), 1));
			ok = valid_data = 1;
		}
		else
			PPError();
	}
	CATCHZOKPPERR
	delete p_dlg;
	return ok;
}

//static
int FASTCALL PPObjTSession::ValidateStatus(int status)
{
	return BIN(oneof5(status, TSESST_PLANNED, TSESST_PENDING, TSESST_INPROCESS, TSESST_CLOSED, TSESST_CANCELED));
}

struct TSessStatusSymb {
	const char * P_Symb;
	int8   Status;
};

static TSessStatusSymb TSessStatusSymbList[] = {
	{ "planned",  TSESST_PLANNED },
	{ "pending",  TSESST_PENDING },
	{ "runned",   TSESST_INPROCESS },
	{ "closed",   TSESST_CLOSED },
	{ "canceled", TSESST_CANCELED }
};

//static
int  FASTCALL PPObjTSession::ResolveStatusSymbol(const char * pSymbol)
{
	int    status = 0;
	SString temp_buf = pSymbol;
	if(temp_buf.NotEmptyS()) {
		temp_buf.ToLower();
		for(uint i = 0; !status && i < SIZEOFARRAY(TSessStatusSymbList); i++) {
			if(temp_buf == TSessStatusSymbList[i].P_Symb)
				status = TSessStatusSymbList[i].Status;
		}
	}
	return status;
}

//static
int  FASTCALL PPObjTSession::GetStatusSymbol(int status, SString & rBuf)
{
	rBuf.Z();
	int    ok = 0;
	for(uint i = 0; !ok && i < SIZEOFARRAY(TSessStatusSymbList); i++) {
		if(TSessStatusSymbList[i].Status == status) {
			rBuf = TSessStatusSymbList[i].P_Symb;
			ok = 1;
		}
	}
	return ok;
}

//static
int FASTCALL PPObjTSession::GetSessionKind(const TSessionTbl::Rec & rRec, int superSessAsSimple)
{
	int    kind = TSESK_SESSION;
	if(rRec.Flags & TSESF_IDLE)
		kind = TSESK_IDLE;
	else if(rRec.Flags & TSESF_PLAN)
		kind = TSESK_PLAN;
	else if(rRec.Flags & TSESF_SUPERSESS && !superSessAsSimple)
		kind = TSESK_SUPERSESS;
	else if(rRec.Flags & TSESF_SUBSESS)
		kind = TSESK_SUBSESS;
	else
		kind = TSESK_SESSION;
	return kind;
}

// static
long SLAPI PPObjTSession::GetContinuation(const TSessionTbl::Rec * pRec)
{
	return (pRec->StDt && pRec->FinDt) ? diffdatetimesec(pRec->FinDt, pRec->FinTm, pRec->StDt, pRec->StTm) : 0;
}

// static
int SLAPI PPObjTSession::IsIdleInsignificant(const TSessionTbl::Rec * pRec, int prevStatus)
{
	int    ok = 0;
	if(prevStatus != TSESST_CLOSED && pRec->Status == TSESST_CLOSED && pRec->Flags & TSESF_IDLE) {
		PPTSessConfig cfg;
		PPObjTSession::ReadConfig(&cfg);
		if(cfg.MinIdleCont > 0) {
			long   cont = GetContinuation(pRec);
			if(cont && cont < cfg.MinIdleCont)
				ok = 1;
		}
	}
	return ok;
}

// static
int SLAPI PPObjTSession::ConvertExtraParam(void * extraPtr, SelFilt * pFilt)
{
	int    ok = -1;
	if(pFilt) {
		long   extra_param = reinterpret_cast<long>(extraPtr);
		memzero(pFilt, sizeof(*pFilt));
		if(extra_param < 0) {
			pFilt->Kind = TSESK_SUPERSESS;
			extra_param = -extra_param;
			if(extra_param == NOPRCVAL)
				extra_param = 0;
		}
		else if(extra_param & PLANFLAG) {
			pFilt->Kind = TSESK_PLAN;
			extra_param &= ~PLANFLAG;
			if(extra_param == NOPRCVAL)
				extra_param = 0;
		}
		if(extra_param & PRCFLAG)
			pFilt->PrcID = (extra_param & ~SPCFLAGMASK);
		else
			pFilt->SuperSessID = (extra_param & ~SPCFLAGMASK);
		if(extra_param & SUBSESSFLAG)
			pFilt->Kind = TSESK_SUBSESS;
		if(pFilt->SuperSessID || pFilt->PrcID || pFilt->Kind)
			ok = 1;
	}
	return ok;
}

// static
void * FASTCALL PPObjTSession::MakeExtraParam(PPID superSessID, PPID prcID, int kind)
{
	long   param = 0;
	if(kind == TSESK_SUPERSESS) {
		if(superSessID)
			param = -superSessID;
		else if(prcID)
			param = -(prcID | PRCFLAG);
		else
			param = -NOPRCVAL;
	}
	else if(kind == TSESK_PLAN) {
		if(superSessID)
			param = (superSessID | PLANFLAG);
		else if(prcID)
			param = (prcID | (PRCFLAG | PLANFLAG));
		else
			param = PLANFLAG;
	}
	else if(kind == TSESK_SUBSESS) {
		if(superSessID)
			param = superSessID;
		else if(prcID)
			param = (prcID | PRCFLAG);
		param |= SUBSESSFLAG;
	}
	else if(kind == TSESK_SESSION) {
		if(superSessID)
			param = superSessID;
		else if(prcID)
			param = (prcID | PRCFLAG);
	}
	return reinterpret_cast<void *>(param);
}

#undef PRCFLAG
#undef NOPRCVAL
#undef PLANFLAG
#undef SUBSESSFLAG
//
//
//
TLP_IMPL(PPObjTSession, TSessionCore, P_Tbl);

SLAPI PPObjTSession::PPObjTSession(void * extraPtr) : PPObject(PPOBJ_TSESSION), P_BhtCurSess(0), ExtraPtr(extraPtr)
{
	TLP_OPEN(P_Tbl);
	MEMSZERO(Cfg);
	ImplementFlags |= implStrAssocMakeList;
}

SLAPI PPObjTSession::~PPObjTSession()
{
	TLP_CLOSE(P_Tbl);
	delete P_BhtCurSess;
}

const PPTSessConfig & SLAPI PPObjTSession::GetConfig()
{
	if(Cfg.ID == 0)
		ReadConfig(&Cfg);
	return Cfg;
}

int SLAPI PPObjTSession::Search(PPID id, void * b)
{
	return P_Tbl->Search(id, (TSessionTbl::Rec *)b);
}

int SLAPI PPObjTSession::SearchByGuid(const S_GUID_Base & rUuid, TSessionTbl::Rec * pRec)
{
	int    ok = -1;
	TSessionTbl::Rec _rec;
	ObjTagItem tag;
	PPIDArray id_list;
	THROW(tag.SetGuid(PPTAG_TSESS_UUID, &rUuid));
	if(PPRef->Ot.SearchObjectsByStr(Obj, PPTAG_TSESS_UUID, tag.Val.PStr, &id_list) > 0) {
		LDATETIME max_dtm = ZERODATETIME;
		PPID  _id = 0;
		//
		// Следующий цикл решает параноидальную проблему существования нескольких документов с одинаковым UUID'ом
		//
		for(uint i = 0; i < id_list.getCount(); i++) {
			TSessionTbl::Rec temp_rec;
			const PPID temp_id = id_list.get(i);
			if(Search(temp_id, &temp_rec) > 0) {
				LDATETIME _dtm;
				_dtm.Set(temp_rec.StDt, temp_rec.StTm);
				if(cmp(_dtm, max_dtm) > 0) {
					ok = !!max_dtm ? 2 : 1; // Найдено более одной сессии - код возврата 2 сигнализирует о том.
					_id = temp_id;
					_rec = temp_rec;
					max_dtm = _dtm;
				}
			}
			else if(!_id) {
				ok = -2; // Сигнализирует о существовании висячей записи тега.
			}
		}
	}
	CATCHZOK
	if(ok > 0) {
		ASSIGN_PTR(pRec, _rec);
	}
	else {
		memzero(pRec, sizeof(*pRec));
	}
	return ok;
}

int SLAPI PPObjTSession::SearchAnalog(const TSessionTbl::Rec & rKey, PPID * pID, TSessionTbl::Rec * pRec)
{
	int    ok = -1;
	PPID   id = 0;
	TSessionTbl::Rec rec;
    {
    	// PrcID, StDt, StTm (unique mod); // #4
    	TSessionTbl::Key4 k4;
    	MEMSZERO(k4);
    	k4.PrcID = rKey.PrcID;
    	k4.StDt = rKey.StDt;
    	k4.StTm = rKey.StTm;
    	if(SearchByKey(P_Tbl, 4, &k4, &rec) > 0) {
            id = rec.ID;
    	}
    }
    if(id) {
    	ASSIGN_PTR(pID, id);
		ASSIGN_PTR(pRec, rec);
		ok = 1;
    }
	return ok;
}

int SLAPI PPObjTSession::EditRights(uint bufSize, ObjRights * pRt, EmbedDialog * pDlg)
{
	return EditSpcRightFlags(DLG_RTTSES, 0, 0, bufSize, pRt, pDlg);
}

int SLAPI PPObjTSession::CheckForFilt(const TSessionFilt * pFilt, PPID id, const TSessionTbl::Rec * pRec)
{
	TSessionTbl::Rec rec;
	if(pRec == 0)
		if(Search(id, &rec) > 0)
			pRec = &rec;
		else
			return 0;
	if(!pFilt)
		return 1;
	if(pFilt->SuperSessID) {
		return (pRec->ParentID != pFilt->SuperSessID) ? 0 : 1;
	}
	else {
		if(pFilt->TechID && pRec->TechID != pFilt->TechID)
			return 0;
		if(pFilt->ArID && pRec->ArID != pFilt->ArID)
			return 0;
		if(pFilt->Ar2ID && pRec->Ar2ID != pFilt->Ar2ID)
			return 0;
		if(!pFilt->StPeriod.CheckDate(pRec->StDt))
			return 0;
		if(pFilt->StPeriod.low && pFilt->StTime)
			if(pRec->StDt == pFilt->StPeriod.low && pRec->StTm < pFilt->StTime)
				return 0;
		if(!pFilt->FnPeriod.CheckDate(pRec->FinDt))
			return 0;
		if(pFilt->FnPeriod.upp && pFilt->FnTime)
			if(pRec->FinDt == pFilt->FnPeriod.upp && pRec->FinTm > pFilt->FnTime)
				return 0;
		if(pFilt->Flags & TSessionFilt::fSuperSessOnly && !(pRec->Flags & TSESF_SUPERSESS))
			return 0;
		if(!pFilt->CheckStatus(pRec->Status))
			return 0;
		if(!pFilt->CheckIdle(pRec->Flags))
			return 0;
		if(pFilt->Flags & TSessionFilt::fManufPlan) {
			if(!(pRec->Flags & TSESF_PLAN))
				return 0;
		}
		else {
			if(pRec->Flags & TSESF_PLAN)
				return 0;
		}
		if(pFilt->PrcID) {
			if(pRec->PrcID != pFilt->PrcID) {
				PPIDArray parent_list;
				PrcObj.GetParentsList(pRec->PrcID, &parent_list);
				if(!parent_list.lsearch(pFilt->PrcID))
					return 0;
			}
		}
	}
	return 1;
}

int SLAPI PPObjTSession::GetTech(PPID tecID, TechTbl::Rec * pRec, int useCache)
{
	return useCache ? TecObj.Fetch(tecID, pRec) : TecObj.Search(tecID, pRec);
}

int SLAPI PPObjTSession::GetStatusText(int statusId, SString & rBuf) const
{
	SString item_buf, id_buf, val_buf;
	rBuf.Z();
	for(int idx = 0; PPGetSubStr(PPTXT_TSESS_STATUS, idx, item_buf) > 0; idx++)
		if(item_buf.Divide(',', id_buf, val_buf) > 0 && id_buf.ToLong() == statusId) {
			rBuf = val_buf;
			return 1;
		}
	return 0;
}

int SLAPI PPObjTSession::GetPrc(PPID prcID, ProcessorTbl::Rec * pRec, int withInheritance, int useCache)
{
	if(withInheritance)
		return PrcObj.GetRecWithInheritance(prcID, pRec, useCache);
	else
		return useCache ? PrcObj.Fetch(prcID, pRec) : PrcObj.Search(prcID, pRec);
}

int SLAPI PPObjTSession::GetPrcByCode(const char * pPrcCode, ProcessorTbl::Rec * pRec)
{
	return PrcObj.SearchByCode(pPrcCode, 0, pRec);
}

int SLAPI PPObjTSession::IsPrcSwitchable(PPID prcID, PPIDArray * pSwitchPrcList)
{
	return PrcObj.IsSwitchable(prcID, pSwitchPrcList);
}

int SLAPI PPObjTSession::SearchByLinkBillID(PPID linkBillID, TSessionTbl::Rec * pRec)
{
	int    ok = -1;
	if(linkBillID) {
		PPIDArray id_list;
		TSessionTbl::Key0 k0;
		BExtQuery q(P_Tbl, 0);
		q.select(P_Tbl->ID, 0).where(P_Tbl->LinkBillID == linkBillID);
		k0.ID = 0;
		for(q.initIteration(0, &k0, spFirst); q.nextIteration() > 0;)
			id_list.add(P_Tbl->data.ID);
		if(id_list.getCount())
			ok = Search(id_list.getLast(), pRec);
	}
	return ok;
}

int SLAPI PPObjTSession::IsProcessorBusy(PPID prcID, PPID tsesID, int kind, const LDATETIME & dtm, long cont, PPID * pTSessID)
{
	int    ok = -1;
	PPID   loaded_sess_id = 0;
	PrcBusy entry;
	PrcBusyArray busy_list;
	STimeChunk dtm_range;
	dtm_range.Start.Set(dtm.d, ZEROTIME);
	dtm_range.Finish.SetFar();
	THROW_INVARG(oneof5(kind, TSESK_SESSION, TSESK_PLAN, TSESK_IDLE, TSESK_SUPERSESS, TSESK_SUBSESS));
	P_Tbl->LoadBusyArray(prcID, tsesID, kind, &dtm_range, &busy_list);
	entry.Init(dtm, cont, 0);
	if(!busy_list.IsFreeEntry(dtm, cont, &loaded_sess_id)) {
		TSessionTbl::Rec loaded_rec;
		SString msg_buf;
		ProcessorTbl::Rec prc_rec;
		if(PrcObj.Fetch(prcID, &prc_rec) > 0)
			msg_buf.Cat(prc_rec.Name);
		else
			ideqvalstr(prcID, msg_buf);
		msg_buf.Semicol().Cat(dtm);
		if(loaded_sess_id && Search(loaded_sess_id, &loaded_rec) > 0) {
			msg_buf.CR().Cat(loaded_rec.Num).CatDiv('-', 1).Cat(*reinterpret_cast<const LDATETIME *>(&loaded_rec.StDt)).
				CatCharN('.', 2).Cat(*reinterpret_cast<const LDATETIME *>(&loaded_rec.FinDt));
		}
		PPSetError(PPERR_PRCISBUSY, msg_buf);
		ASSIGN_PTR(pTSessID, loaded_sess_id);
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjTSession::CheckNewPrc(const TSessionTbl::Rec * pRec, PPID newPrcID)
{
	int    ok = -1;
	ProcessorTbl::Rec prc_rec;
	if(newPrcID)
		THROW(PrcObj.Fetch(newPrcID, &prc_rec) > 0);
	if(pRec->TechID) {
		TechTbl::Rec tec_rec;
		THROW(TecObj.Fetch(pRec->TechID, &tec_rec) > 0);
		if(tec_rec.PrcID == newPrcID)
			ok = 1;
		else if(newPrcID) {
			PPIDArray prc_list;
			THROW(PrcObj.GetParentsList(newPrcID, &prc_list));
			if(prc_list.lsearch(tec_rec.PrcID))
				ok = 1;
		}
	}
	else
		ok = 1;
	if(ok < 0) {
		SString msg_buf, temp_buf;
		MakeName(pRec, temp_buf);
		msg_buf.Cat(temp_buf).Space().CatCharN('>', 2).Space();
		if(newPrcID)
			msg_buf.Cat(prc_rec.Name);
		else
			msg_buf.CatChar('0');
		PPSetError(PPERR_TSESSCANTMOVETOPRC, msg_buf);
	}
	CATCHZOK
	return ok;
}

int FASTCALL PPObjTSession::CheckSessionTime(const TSessionTbl::Rec & rRec)
{
	int    ok = 1;
	if(rRec.Status != TSESST_CANCELED) {
		const  int kind = PPObjTSession::GetSessionKind(rRec, 0);
		LDATETIME dtm, finish;
		dtm.Set(rRec.StDt, rRec.StTm);
		long   cont = rRec.FinDt ? PPObjTSession::GetContinuation(&rRec) : rRec.PlannedTiming;
		finish.Set(rRec.FinDt, rRec.FinTm);
		if(cmp(dtm, finish) > 0)
			ok = PPSetError(PPERR_STARTGTFINISH);
		else if(IsProcessorBusy(rRec.PrcID, rRec.ID, kind, dtm, cont, 0) >= 0)
			ok = 0;
	}
	return ok;
}

int SLAPI PPObjTSession::IsProcessorInProcess(PPID prcID, int kind, TSessionTbl::Rec * pSessRec)
{
	int    ok = -1;
	TSessionTbl::Rec tses_rec;
	if(P_Tbl->GetProcessed(prcID, kind, &tses_rec) > 0) {
		SString msg_buf;
		MakeName(&tses_rec, msg_buf);
		PPSetError(PPERR_PRCISBUSY, msg_buf);
		ASSIGN_PTR(pSessRec, tses_rec);
		ok = 1;
	}
	return ok;
}
//
//
//
struct DscntEntry { // @flat
	long   OprNo;
	PPID   GoodsID;
	double Qtty;
	double Price;
	double Sum;
	double Dscnt;
};

void SLAPI PPObjTSession::Helper_SetupDiscount(SVector & rList, int pct, double discount) // @v10.0.07 SArray-->SVector
{
	uint   i;
	uint   last_index = 0;
	double min_qtty = SMathConst::Max;
	double max_price = 0.0;
	double amount = 0.0, qtty, p, d;
	DscntEntry * p_item;
	PPIDArray wodis_goods_list;
	for(i = 0; rList.enumItems(&i, (void **)&p_item);)
		if(GObj.CheckFlag(p_item->GoodsID, GF_NODISCOUNT) > 0)
			wodis_goods_list.addUnique(p_item->GoodsID);
		else {
			qtty   = fabs(p_item->Qtty);
			p      = R2(p_item->Price);
			amount = R2(amount + p * qtty);
			if(qtty > 0 && (qtty < min_qtty || (qtty == min_qtty && p > max_price))) {
				last_index = i;
				min_qtty = qtty;
				max_price = p;
			}
		}
	double dscnt = pct ? (discount * fdiv100r(amount)) : discount;
	double part_dis = 0.0, part_amount = 0.0;
	int    prec = 2;
	for(i = 0; rList.enumItems(&i, (void **)&p_item);)
		if(i != last_index && !wodis_goods_list.lsearch(p_item->GoodsID)) {
			qtty = fabs(p_item->Qtty);
			p = R2(p_item->Price);
			d = round(fdivnz(p * (dscnt - part_dis), (amount - part_amount)), prec);
			p_item->Dscnt = d;
			part_dis    += (d * qtty);
			part_amount += (p * qtty);
		}
	if(last_index) {
		p_item = (DscntEntry *)rList.at(last_index-1);
		p_item->Dscnt = round((dscnt - part_dis) / fabs(p_item->Qtty), prec);
	}
}

int SLAPI PPObjTSession::SetupDiscount(PPID sessID, int pct, double discount, int use_ta)
{
	int    ok = -1;
	if(sessID && (GetConfig().Flags & PPTSessConfig::fUsePricing)) {
		TSessionTbl::Rec sess_rec;
		TSessLineTbl::Rec line_rec;
		double amount = 0.0;
		SVector ln_list(sizeof(DscntEntry)); // @v10.0.07 SArray-->SVector
		DscntEntry * p_entry;
		{
			PPTransaction tra(use_ta);
			THROW(tra);
			THROW(Search(sessID, &sess_rec) > 0);
			THROW_PP(sess_rec.Incomplete || (GetConfig().Flags & PPTSessConfig::fAllowLinesInWrOffSessions), PPERR_UPDLNCOMPLTSESS);
			{
				for(SEnum en = P_Tbl->EnumLines(sessID); en.Next(&line_rec) > 0;) {
					DscntEntry entry;
					MEMSZERO(entry);
					entry.OprNo   = line_rec.OprNo;
					entry.GoodsID = line_rec.GoodsID;
					entry.Qtty    = fabs(line_rec.Qtty);
					entry.Price   = fabs(line_rec.Price);
					entry.Sum     = entry.Qtty * entry.Price;
					ln_list.insert(&entry);
				}
			}
			Helper_SetupDiscount(ln_list, pct, discount);
			for(uint i = 0; ln_list.enumItems(&i, (void **)&p_entry);) {
				THROW(P_Tbl->SearchLine(sessID, p_entry->OprNo, &line_rec) > 0);
				amount = faddwsign(amount, R2(p_entry->Qtty * (p_entry->Price - p_entry->Dscnt)), line_rec.Sign);
				if(line_rec.Discount != p_entry->Dscnt) {
					line_rec.Discount = p_entry->Dscnt;
					THROW_DB(P_Tbl->Lines.updateRecBuf(&line_rec));
					ok = 1;
				}
			}
			if(ok > 0) {
				sess_rec.Amount = fabs(amount);
				THROW(UpdateByID(P_Tbl, PPOBJ_TSESSION, sessID, &sess_rec, 0));
			}
			THROW(tra.Commit());
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjTSession::PutLine(PPID sessID, long * pOprNo, TSessLineTbl::Rec * pRec, int use_ta)
{
	return Helper_PutLine(sessID, pOprNo, pRec, 0, use_ta);
}

int SLAPI PPObjTSession::Helper_PutLine(PPID sessID, long * pOprNo, TSessLineTbl::Rec * pRec, long options, int use_ta)
{
	int    ok = 1;
	const  int use_price = BIN(GetConfig().Flags & PPTSessConfig::fUsePricing);
	PPID   main_goods_id = 0;
	double qtty = 0.0;
	double amount = 0.0;
	TSessionTbl::Rec sess_rec;
	TechTbl::Rec tec_rec;
	SCardTbl::Rec sc_rec;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(Search(sessID, &sess_rec) > 0);
		if(sess_rec.TechID && TecObj.Fetch(sess_rec.TechID, &tec_rec) > 0)
			main_goods_id = labs(tec_rec.GoodsID);
		{
			if(pRec) {
				SString add_msg;
				PPSetAddedMsgString(add_msg.Cat(pRec->Sign));
				THROW_PP(oneof3(pRec->Sign, -1, +1, 0), PPERR_INVTSESLINESIGN);
				if(pRec->Flags & TSESLF_REST) {
					pRec->Sign = 0;
					if(!pOprNo || *pOprNo == 0) {
						if(pRec->Serial[0]) {
							//
							// Проследим, чтобы строка остатка в течении одной сессии по одному серийному номеру
							// была единственной
							//
							TSessLineTbl::Rec rest_rec;
							if(P_Tbl->SearchSerial(pRec->Serial, pRec->TSessID, 0, TSessionCore::sserRest, &rest_rec) > 0)
								ASSIGN_PTR(pOprNo, rest_rec.OprNo);
						}
						else {
							//
							// @todo Проследить за тем, чтобы без серийного номера для одного товара
							// в течении одной сессии была только одна строка остатка
							//
						}
					}
				}
				if(pRec->GoodsID == main_goods_id)
					qtty = faddwsign(qtty, pRec->Qtty, pRec->Sign);
				if(use_price)
					amount = faddwsign(amount, -R2(pRec->Qtty * (pRec->Price - pRec->Discount)), pRec->Sign);
			}
			if(pOprNo && *pOprNo) {
				TSessLineTbl::Rec rec;
				THROW(P_Tbl->SearchLine(sessID, *pOprNo, &rec) > 0);
				if(main_goods_id && rec.GoodsID == main_goods_id)
					qtty = faddwsign(qtty, -rec.Qtty, rec.Sign);
				if(use_price)
					amount = faddwsign(amount, R2(rec.Qtty * (rec.Price - rec.Discount)), rec.Sign);
				if(pRec == 0) {
					THROW_PP(sess_rec.Incomplete || (GetConfig().Flags & PPTSessConfig::fAllowLinesInWrOffSessions), PPERR_RMVLNCOMPLTSESS);
					THROW(deleteFrom(&P_Tbl->Lines, 0, P_Tbl->Lines.TSessID == sessID && P_Tbl->Lines.OprNo == *pOprNo));
				}
				else {
					THROW_PP(sess_rec.Incomplete || (GetConfig().Flags & PPTSessConfig::fAllowLinesInWrOffSessions), PPERR_UPDLNCOMPLTSESS);
					THROW_PP((rec.Flags & TSESLF_REST) == (pRec->Flags & TSESLF_REST), PPERR_UPDTESLNRESTSTATUS);
					pRec->TSessID = sessID;
					pRec->OprNo   = *pOprNo;
					P_Tbl->Lines.copyBufFrom(pRec);
					P_Tbl->Lines.data.Flags &= ~TSESLF_EXPANDSESS;
					THROW_DB(P_Tbl->Lines.updateRec());
				}
			}
			else if(pRec) {
				long   oprno = 0;
				THROW_PP(sess_rec.Incomplete || (GetConfig().Flags & PPTSessConfig::fAllowLinesInWrOffSessions), PPERR_ADDLNCOMPLTSESS);
				THROW(P_Tbl->SearchOprNo(sessID, &oprno, 0) > 0);
				pRec->TSessID = sessID;
				pRec->OprNo = oprno;
				P_Tbl->Lines.copyBufFrom(pRec);
				P_Tbl->Lines.data.Flags &= ~TSESLF_EXPANDSESS;
				THROW_DB(P_Tbl->Lines.insertRec());
				pRec->OprNo = oprno;
				ASSIGN_PTR(pOprNo, oprno);
			}
		}
		if(qtty != 0.0 || amount != 0.0 || (pRec && pRec->Flags & TSESLF_EXPANDSESS)) {
			sess_rec.ActQtty += qtty;
			sess_rec.Amount  += amount;
			if(pRec && pRec->Flags & TSESLF_EXPANDSESS) {
				LDATETIME fin, ln_dtm;
				fin.Set(sess_rec.FinDt, sess_rec.FinTm);
				ln_dtm.Set(pRec->Dt, pRec->Tm);
				if(fin.d == 0 || cmp(fin, ln_dtm) < 0) {
					sess_rec.FinDt = ln_dtm.d;
					sess_rec.FinTm = ln_dtm.t;
					THROW(CheckSessionTime(sess_rec));
				}
			}
			THROW_DB(P_Tbl->updateRecBuf(&sess_rec));
		}
		if(!(options & hploInner)) {
			// @v10.0.07 {
			if(tec_rec.Flags & TECF_RVRSCMAINGOODS && !(options & hploInner)) {
				TSessionPacket ts_pack;
				if(GetPacket(sessID, &ts_pack, gpoLoadLines) > 0) {
					LongArray upd_row_idx_list;
					if(RecalcSessionPacket(ts_pack, &upd_row_idx_list) > 0) {
						PPID   temp_id = sessID;
						for(uint i = 0; i < upd_row_idx_list.getCount(); i++) {
							TSessLineTbl::Rec inner_line = ts_pack.Lines.at(upd_row_idx_list.get(i)-1);
							THROW(Helper_PutLine(sessID, &inner_line.OprNo, &inner_line, hploInner, 0));
						}
					}
				}
			}
			// } @v10.0.07 
			if(use_price && sess_rec.SCardID && ScObj.Search(sess_rec.SCardID, &sc_rec) > 0) {
				THROW(SetupDiscount(sessID, 1, fdiv100i(sc_rec.PDis), 0));
			}
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjTSession::ReplaceGoodsInLines(PPID sessID, PPID replacedGoodsID, PPID substGoodsID, long flags, int use_ta)
{
	int    ok = 1;
	TSessLineTbl::Key0 k0;
	Goods2Tbl::Rec goods_rec;
	/*
	GoodsStockExt gse;
	double upp = 0;
	if(goods_obj.GetStockExt(substGoodsID, &gse) > 0 && gse.Package > 0)
		upp = gse.Package;
	*/
	if(flags & 0x0001)
		use_ta = 0;
	THROW(GObj.Search(substGoodsID, &goods_rec) > 0);
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		MEMSZERO(k0);
		k0.TSessID = sessID;
		while(P_Tbl->Lines.search(0, &k0, spGt) && k0.TSessID == sessID) {
			if(P_Tbl->Lines.data.GoodsID == replacedGoodsID && P_Tbl->Lines.data.Sign > 0) {
				P_Tbl->Lines.data.GoodsID = substGoodsID;
				if(!(flags & 0x0001))
					THROW_DB(P_Tbl->Lines.updateRec());
			}
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjTSession::Correct(long sessID, int use_ta)
{
	int    ok = -1;
	int    do_update = 0;
	PPID   main_goods_id = 0;
	TSessionTbl::Rec sess_rec;
	TechTbl::Rec tec_rec;
	TSessGoodsTotal total;
	MEMSZERO(total);
	THROW(CheckRights(PPR_MOD));
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(Search(sessID, &sess_rec) > 0);
		if(sess_rec.TechID && TecObj.Fetch(sess_rec.TechID, &tec_rec) > 0)
			main_goods_id = labs(tec_rec.GoodsID);
		if(main_goods_id) {
			THROW(P_Tbl->CalcGoodsTotal(sessID, main_goods_id, &total));
			if(total.Qtty != sess_rec.ActQtty) {
				sess_rec.ActQtty = total.Qtty;
				do_update = 1;
			}
		}
		MEMSZERO(total);
		THROW(P_Tbl->CalcGoodsTotal(sessID, 0, &total));
		if(total.Amount != sess_rec.Amount) {
			sess_rec.Amount = total.Amount;
			do_update = 1;
		}
		if(do_update) {
			THROW(P_Tbl->Put(&sessID, &sess_rec, 0));
			ok = 1;
		}
		if(sess_rec.Flags & TSESF_SUPERSESS) {
			THROW(P_Tbl->UpdateSuperSessCompleteness(sessID, 0));
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjTSession::GetPrevSession(const TSessionTbl::Rec & rSessRec, TSessionTbl::Rec * pPrevRec)
{
	int    ok = -1, r;
	TSessionTbl::Key4 k4;
	MEMSZERO(k4);
	k4.PrcID = rSessRec.PrcID;
	k4.StDt = rSessRec.StDt;
	k4.StTm = rSessRec.StTm;
	LDATETIME end;
	LDATETIME recent_end = ZERODATETIME;
	while((r = P_Tbl->search(4, &k4, spLt)) != 0 && k4.PrcID == rSessRec.PrcID) {
		if(GetSessionKind(P_Tbl->data) == TSESK_SESSION) {
			end.Set(P_Tbl->data.FinDt, P_Tbl->data.FinTm);
			if(cmp(end, rSessRec.StDt, rSessRec.StTm) < 0 && cmp(end, recent_end) > 0) {
				recent_end = end;
				ASSIGN_PTR(pPrevRec, P_Tbl->data);
				break;
			}
		}
	}
	if(r == 0 && BtrError && !BTRNFOUND)
		ok = 0;
	else if(recent_end.d)
		ok = 1;
	return ok;
}

// private
int SLAPI PPObjTSession::CompleteStruc(PPID sessID, PPID tecGoodsID, PPID tecStrucID,
	double tecQtty, const PPIDArray * pGoodsIdList, int tooling)
{
	int    ok = -1, r;
	if(sessID && tecGoodsID && tecStrucID) {
		PPObjGoodsStruc gs_obj;
		PPGoodsStruc gs;
		PPGoodsStrucItem gs_item;
		double qtty = 0;
		THROW(gs_obj.Get(tecStrucID, &gs));
		gs.GoodsID = tecGoodsID;
		for(uint gs_pos = 0; (r = gs.EnumItemsExt(&gs_pos, &gs_item, tecGoodsID, tecQtty, &qtty)) > 0;)
			if(tooling || (gs_item.Flags & GSIF_AUTOTSWROFF))
				if(!pGoodsIdList || !pGoodsIdList->lsearch(gs_item.GoodsID)) {
					long   oprno = 0;
					TSessLineTbl::Rec line_rec;
					if(gs_item.Formula__[0]) {
						double v = 0.0;
						GdsClsCalcExprContext ctx(&gs, sessID);
						THROW(PPCalcExpression(gs_item.Formula__, &v, &ctx));
						qtty = v;
					}
					qtty = -qtty;
					THROW(InitLinePacket(&line_rec, sessID));
					line_rec.Flags |= TSESLF_AUTOCOMPL;
					if(tooling)
						line_rec.Flags |= TSESLF_TOOLING;
					line_rec.GoodsID = gs_item.GoodsID;
					line_rec.Qtty = fabs(qtty);
					line_rec.Sign = (qtty < 0.0) ? -1 : 1;
					THROW(PutLine(sessID, &oprno, &line_rec, 0));
					ok = 1;
				}
		THROW(r);
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjTSession::RecalcSessionPacket(TSessionPacket & rPack, LongArray * pUpdRowIdxList)
{
	int    ok = -1;
	TechTbl::Rec tec_rec;
	CALLPTRMEMB(pUpdRowIdxList, clear());
	if(rPack.Rec.TechID && TecObj.Fetch(rPack.Rec.TechID, &tec_rec) > 0) {
		PPGoodsStruc gs;
		Goods2Tbl::Rec goods_rec;
		if(TecObj.GetGoodsStruc(rPack.Rec.TechID, &gs) > 0) {
			if(tec_rec.Flags & TECF_RVRSCMAINGOODS && GObj.Fetch(tec_rec.GoodsID, &goods_rec) > 0) {
				uint   single_main_line_pos = 0;
				uint   i;
				for(i = 0; i < rPack.Lines.getCount(); i++) {
					const TSessLineTbl::Rec & r_item = rPack.Lines.at(i);
					if(r_item.GoodsID == tec_rec.GoodsID) {
						if(!single_main_line_pos)
							single_main_line_pos = i+1;
						else {
							single_main_line_pos = 0;
							break;
						}
					}
				}
				if(single_main_line_pos) {
					TSessLineTbl::Rec & r_main_item = rPack.Lines.at(single_main_line_pos-1);
					assert(r_main_item.GoodsID == tec_rec.GoodsID);
					for(uint giidx = 0; giidx < gs.Items.getCount(); giidx++) {
						const PPGoodsStrucItem & r_gsi = gs.Items.at(giidx);
						if(r_gsi.Flags & GSIF_MAINITEM) {
							double gsi_qtty = 0.0;
							for(i = 0; i < rPack.Lines.getCount(); i++) {
								const TSessLineTbl::Rec & r_item = rPack.Lines.at(i);
								if(r_item.GoodsID == r_gsi.GoodsID) {
									if(r_item.Sign > 0)
										gsi_qtty -= fabs(r_item.Qtty);
									else if(r_item.Sign < 0)
										gsi_qtty += fabs(r_item.Qtty);
								}
							}
							if(gsi_qtty != 0.0) {
								double cq = 0.0;
								if(r_gsi.Formula__[0]) {
									double v = 0.0;
									GdsClsCalcExprContext ctx(&gs, rPack.Rec.ID);
									if(PPCalcExpression(r_gsi.Formula__, &v, &ctx))
										cq = v;
								}
								else if(gs.GetItemExt(giidx, 0, tec_rec.GoodsID, 1.0, &cq) > 0) {
								}
								if(cq > 0.0) {
									double result_qtty = gsi_qtty / cq;
									if(tec_rec.Flags & TECF_RECOMPLMAINGOODS) {
										if(goods_rec.Flags & GF_USEINDEPWT && !feqeps(r_main_item.WtQtty, result_qtty, 1E-8)) {
											r_main_item.WtQtty = result_qtty;
											CALLPTRMEMB(pUpdRowIdxList, add(single_main_line_pos));
											ok = 1;
										}
									}
									else if(!feqeps(r_main_item.Qtty, result_qtty, 1E-8)) {
										r_main_item.Qtty = result_qtty;
										CALLPTRMEMB(pUpdRowIdxList, add(single_main_line_pos));
										ok = 1;
									}
								}
							}
							break;
						}
					}
				}
			}
		}
	}
	return ok;
}

int SLAPI PPObjTSession::CompleteSession(PPID sessID, int use_ta)
{
	int    ok = -1;
	uint   i, j;
	long   hdl_ln_enum = -1;
	TSessionTbl::Rec tses_rec;
	ProcessorTbl::Rec prc_rec;
	PPObjGoodsStruc gs_obj;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(Search(sessID, &tses_rec) > 0);
		if(tses_rec.Flags & TSESF_SUPERSESS) {
			PPIDArray child_list;
			THROW(P_Tbl->GetChildIDList(sessID, 0, &child_list));
			for(i = 0; i < child_list.getCount(); i++)
				THROW(CompleteSession(child_list.at(i), 0)); // @recursion
		}
		else {
			PPID   tec_goods_id = 0;
			PPID   tec_struc_id = 0;
			int    r;
			PPIDArray tec_child_list;
			RAssocArray goods_qtty_list;
			PPIDArray goods_id_list;
			PPIDArray lines_to_remove;
			TSessLineTbl::Rec line_rec;
			TechTbl::Rec tec_rec, tec_rec2;
			TSessionTbl::Rec prev_sess_rec;
			THROW_PP(!(tses_rec.Flags & TSESF_WRITEDOFF), PPERR_TSESSWRITEDOFF);
			THROW_PP(!(tses_rec.Flags & TSESF_SUPERSESS), PPERR_INVOPONTSUPERSESS);
			THROW(PrcObj.GetRecWithInheritance(tses_rec.PrcID, &prc_rec) > 0);
			if(TecObj.Fetch(tses_rec.TechID, &tec_rec) > 0) {
				tec_goods_id = tec_rec.GoodsID;
				tec_struc_id = tec_rec.GStrucID;
			}
			else
				MEMSZERO(tec_rec);
			if(TecObj.GetChildList(tses_rec.TechID, tec_child_list) > 0) {
				for(uint i = 0; i < tec_child_list.getCount(); i++) {
					const PPID tec_child_id = tec_child_list.get(i);
					if(TecObj.Fetch(tec_child_id, &tec_rec2) > 0) {
						TSessionTbl::Rec sub_ses_rec;
						THROW(InitRec(&sub_ses_rec, TSESK_SUBSESS, tec_rec2.PrcID, sessID, TSESST_PLANNED));
						if(sub_ses_rec.TechID) {
							PPID   sub_ses_id = 0;
							//
							// Рассчитываем и устанавливаем планируемое время работы, а так же планируемое время окончания сессии
							//
							THROW(SetPlannedTiming(&sub_ses_rec));
							if(sub_ses_rec.PlannedTiming > 0) {
								const  long timing = sub_ses_rec.PlannedTiming;
								LDATETIME start;
								start.Set(sub_ses_rec.StDt, sub_ses_rec.StTm);
								LDATETIME finish = plusdatetime(start, timing, 3);
								if(!start.d)
									finish.d = ZERODATE;
								sub_ses_rec.FinDt = finish.d;
								sub_ses_rec.FinTm = finish.t;
							}
							//
							THROW(PutRec(&sub_ses_id, &sub_ses_rec, 0));
							THROW(CompleteSession(sub_ses_id, 0)); // @recursion
							ok = 1;
						}
					}
				}
			}
			else {
				THROW(P_Tbl->InitLineEnum(sessID, &hdl_ln_enum));
				while(P_Tbl->NextLineEnum(hdl_ln_enum, &line_rec) > 0) {
					if(line_rec.Flags & TSESLF_AUTOCOMPL)
						lines_to_remove.add(line_rec.OprNo);
					else {
						if(tec_rec.Flags & TECF_CALCTIMEBYROWS || line_rec.GoodsID == tec_goods_id)
							goods_qtty_list.Add(line_rec.GoodsID, fgetwsign(line_rec.Qtty, line_rec.Sign));
						goods_id_list.addUnique(line_rec.GoodsID);
					}
				}
				if(tec_struc_id && tec_goods_id) {
					PPGoodsStrucHeader gs_rec;
					if(gs_obj.Fetch(tec_struc_id, &gs_rec) > 0 && gs_rec.Flags & GSF_PARTITIAL) {
						const double qtty = (tses_rec.ActQtty != 0.0) ? tses_rec.ActQtty : tses_rec.PlannedQtty;
						//
						// Функция CompleteStruc изменяет знак в количестве. Поэтому здесь умножает количество на -1
						//
						goods_qtty_list.Add(tec_goods_id, -1.0 * fgetwsign(qtty, tec_rec.Sign));
						goods_id_list.addUnique(tec_goods_id);
					}
				}
				//
				// Removing lines, having flag TSESLF_AUTOCOMPLETE
				//
				for(i = 0; i < lines_to_remove.getCount(); i++) {
					long   oprno = lines_to_remove.at(i);
					THROW(PutLine(sessID, &oprno, 0, 0));
				}
				for(j = 0; j < goods_qtty_list.getCount(); j++) {
					const  PPID   goods_id = goods_qtty_list.at(j).Key;
					const  double qtty     = goods_qtty_list.at(j).Val;
					PPID   struc_id = 0;
					if(goods_id == tec_goods_id)
						struc_id = tec_struc_id;
					else if(GetTechByGoods(goods_id, tses_rec.PrcID, &tec_rec2) > 0)
						struc_id = tec_rec2.GStrucID;
					if(struc_id && qtty) {
						THROW(r = CompleteStruc(sessID, goods_id, struc_id, qtty, &goods_id_list, 0));
						if(r > 0)
							ok = 1;
					}
					//
					// Списание расходов на перенастройку
					// (До некоторой поры расходы на перенастройку списываем только по основному товару сессии)
					//
					if(goods_id == tec_goods_id) {
						THROW(r = GetPrevSession(tses_rec, &prev_sess_rec));
						if(r > 0 && TecObj.Fetch(prev_sess_rec.TechID, &tec_rec2) > 0 && goods_id != tec_rec2.GoodsID) {
							TSVector <TechTbl::Rec> t_list; // @v9.8.4 TSArray-->TSVect
							THROW(r = TecObj.SelectTooling(tses_rec.PrcID, goods_id, tec_rec2.GoodsID, &t_list));
							if(r > 0)
								for(i = 0; i < t_list.getCount(); i++) {
									THROW(r = CompleteStruc(sessID, goods_id, t_list.at(i).GStrucID, 1, 0, 1));
									if(r > 0)
										ok = 1;
								}
						}
					}
				}
			}
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	P_Tbl->DestroyIter(hdl_ln_enum);
	return ok;
}

int SLAPI PPObjTSession::SetSessionState(TSessionTbl::Rec * pRec, int newState, int checkOnly)
{
	return Helper_SetSessionState(pRec, newState, checkOnly, 0);
}

static void SetSessCurTime(TSessionTbl::Rec * pRec, int finish, int update)
{
	if(update)
		if(finish)
			getcurdatetime(&pRec->FinDt, &pRec->FinTm);
		else {
			LDATETIME dtm = getcurdatetime_();
			LDATETIME finish_dtm = dtm;
			finish_dtm.settotalsec(dtm.t.totalsec() + pRec->PlannedTiming + pRec->ToolingTime);
			pRec->StDt = dtm.d;
			pRec->StTm = dtm.t;
			pRec->FinDt = finish_dtm.d;
			pRec->FinTm = finish_dtm.t;
		}
}

int SLAPI PPObjTSession::AdjustTiming(const TSessionTbl::Rec & rSessRec, const STimeChunk & rChunk, STimeChunk & rResult, long * pTiming)
{
	rResult = rChunk;
	int    ok = -1;
	long   timing = 0;
	if(checkdate(rChunk.Start.d) && checkdate(rChunk.Finish.d)) {
		int    plus_one_day = 0;
		TechTbl::Rec tec_rec;
		if(GetTech(rSessRec.TechID, &tec_rec, 1) > 0 && tec_rec.GoodsID) {
			double unit_ratio = 1.0;
			if(IsTimingTech(&tec_rec, &unit_ratio) > 0) {
				PPProcessorPacket::ExtBlock prc_ext;
				if(PrcObj.GetExtWithInheritance(rSessRec.PrcID, &prc_ext) > 0) {
					plus_one_day = (prc_ext.TimeFlags & prc_ext.tfPlusOneDay) ? 1 : 0;
					LTIME ci_tm = prc_ext.CheckInTime;
					LTIME co_tm = prc_ext.CheckOutTime;
					if(!ci_tm && co_tm)
						ci_tm = co_tm;
					else if(!co_tm && ci_tm)
						co_tm = ci_tm;
					if(ci_tm && co_tm) {
						const LTIME _24h = encodetime(24, 0, 0, 0);
						if(prc_ext.TimeFlags & prc_ext.tfCheckInRoundForward) {
							if(rResult.Start.t < ci_tm) {
								rResult.Start.t = (ci_tm > _24h) ? _24h : ci_tm;
							}
							else if(rResult.Start.t > ci_tm) {
								rResult.Start.d = plusdate(rResult.Start.d, 1);
								rResult.Start.t = ci_tm;
							}
						}
						else {
							if(rResult.Start.t > ci_tm) {
								rResult.Start.t = (ci_tm > _24h) ? _24h : ci_tm;
							}
							else if(rResult.Start.t < ci_tm) {
								rResult.Start.d = plusdate(rResult.Start.d, -1);
								rResult.Start.t = ci_tm;
							}
						}
						if(prc_ext.TimeFlags & prc_ext.tfCheckOutRoundBackward) {
							if(rResult.Finish.t > co_tm) {
								rResult.Finish.t = (co_tm > _24h) ? _24h : co_tm;
							}
							else if(rResult.Finish.t < co_tm) {
								rResult.Finish.d = plusdate(rResult.Finish.d, -1);
								rResult.Finish.t = co_tm;
							}
						}
						else {
							if(rResult.Finish.t < co_tm) {
								rResult.Finish.t = (co_tm > _24h) ? _24h : co_tm;
							}
							else if(rResult.Finish.t > co_tm) {
								rResult.Finish.d = plusdate(rResult.Finish.d, 1);
								rResult.Finish.t = co_tm;
							}
						}
					}
				}
			}
		}
		if(rResult != rChunk)
			ok = 1;
		timing = rResult.GetDuration();
		if(plus_one_day)
			timing += 24 * 60 * 60;
	}
	else
		ok = 0;
	ASSIGN_PTR(pTiming, timing);
	return ok;
}

int SLAPI PPObjTSession::RoundTiming(PPID techID, long * pTiming)
{
	int    ok = -1;
	long   timing = *pTiming;
	TechTbl::Rec tec_rec;
	if(GetTech(techID, &tec_rec, 1) > 0 && timing && tec_rec.GoodsID && tec_rec.Rounding) {
		double unit_ratio = 1.0;
		if(IsTimingTech(&tec_rec, &unit_ratio) > 0) {
			double c = Round((double)timing / unit_ratio, tec_rec.Rounding, +1);
			timing = (long)(c * unit_ratio);
		}
	}
	ASSIGN_PTR(pTiming, timing);
	return ok;
}

int SLAPI PPObjTSession::CalcPlannedTiming(PPID techID, double qtty, int useRounding, long * pTiming)
{
	int    ok = -1;
	long   timing = 0;
	TechTbl::Rec tec_rec;
	if(GetTech(techID, &tec_rec, 1) > 0) {
		if(tec_rec.Capacity > 0.0) {
			if(tec_rec.Flags & TECF_ABSCAPACITYTIME)
				timing = R0i(tec_rec.Capacity);
			else
				timing = R0i(qtty / tec_rec.Capacity);
			if(useRounding)
				RoundTiming(techID, &timing);
		}
		ok = 1;
	}
	ASSIGN_PTR(pTiming, timing);
	return ok;
}

int SLAPI PPObjTSession::CalcPlannedQtty(const TSessionTbl::Rec * pPack, long forceTiming, double * pQtty)
{
	int    ok = -1;
	double qtty = 0.0;
	TechTbl::Rec tec_rec;
	if(GetTech(pPack->TechID, &tec_rec, 1) > 0) {
		long   timing = 0;
		double unit_ratio = 1.0;
		if(IsTimingTech(&tec_rec, &unit_ratio) > 0) {
			STimeChunk chunk, result_chunk;
			chunk.Start.Set(pPack->StDt, pPack->StTm);
			chunk.Finish.Set(pPack->FinDt, pPack->FinTm);
			if(AdjustTiming(*pPack, chunk, result_chunk, &timing) > 0)
				qtty = Round((double)timing / unit_ratio, tec_rec.Rounding, +1);
			else
				qtty = Round((double)GetContinuation(pPack) / unit_ratio, tec_rec.Rounding, +1);
			ok = 1;
		}
		else if(tec_rec.Capacity > 0.0) {
			timing = (forceTiming >= 0) ? forceTiming : GetContinuation(pPack);
			if(tec_rec.Flags & TECF_ABSCAPACITYTIME)
				qtty = timing;
			else
				qtty = timing * tec_rec.Capacity;
			ok = 1;
		}
	}
	ASSIGN_PTR(pQtty, qtty);
	return ok;
}

#if 0 // @v8.1.6 {
int SLAPI PPObjTSession::CalcPlannedQtty(PPID techID, long timing, double * pQtty)
{
	int    ok = -1;
	double qtty = 0.0;
	TechTbl::Rec tec_rec;
	if(GetTech(techID, &tec_rec, 1) > 0) {
		if(tec_rec.Capacity > 0.0) {
			if(tec_rec.Flags & TECF_ABSCAPACITYTIME)
				qtty = timing;
			else
				qtty = timing * tec_rec.Capacity;
			ok = 1;
		}
	}
	ASSIGN_PTR(pQtty, qtty);
	return ok;
}
#endif // } 0 @v8.1.6

int SLAPI PPObjTSession::CalcToolingTiming(const TSessionTbl::Rec * pRec, long * pTiming)
{
	int    ok = -1;
	long   timing = 0;
	TSessionTbl::Rec prev_sess_rec;
	TechTbl::Rec tec_rec, prev_tec_rec;
	if(GetPrevSession(*pRec, &prev_sess_rec) > 0 && TecObj.Fetch(pRec->TechID, &tec_rec) > 0 &&
		TecObj.Fetch(prev_sess_rec.TechID, &prev_tec_rec) > 0) {
		//if(tec_rec.GoodsID != prev_tec_rec.GoodsID) {
			TSVector <TechTbl::Rec> t_list; // @v9.8.4 TSArray-->TSVect
			if(TecObj.SelectTooling(pRec->PrcID, tec_rec.GoodsID, prev_tec_rec.GoodsID, &t_list) > 0)
				for(uint i = 0; i < t_list.getCount(); i++) {
					timing += t_list.at(i).Duration;
					ok = 1;
				}
		//}
	}
	ASSIGN_PTR(pTiming, timing);
	return ok;
}

int SLAPI PPObjTSession::GetTechByGoods(PPID goodsID, PPID prcID, TechTbl::Rec * pTechRec)
{
	PPIDArray tec_list;
	TecObj.GetListByPrcGoods(prcID, goodsID, &tec_list);
	for(uint i = 0; i < tec_list.getCount(); i++)
		if(TecObj.Fetch(tec_list.at(i), pTechRec) > 0)
			return 1;
	return -1;
}

int SLAPI PPObjTSession::SetPlannedTiming(TSessionTbl::Rec * pRec)
{
	int    ok = -1;
	long   hdl_ln_enum = -1;
	long   timing = 0, inner_timing = 0;
	TechTbl::Rec tec_rec, inner_tec_rec;
	if(CalcPlannedTiming(pRec->TechID, pRec->PlannedQtty, 1, &timing) > 0) {
		CalcToolingTiming(pRec, &inner_timing);
		timing += inner_timing;
		if(pRec->ID && pRec->PrcID && GetTech(pRec->TechID, &tec_rec, 1) > 0 && tec_rec.Flags & TECF_CALCTIMEBYROWS) {
			int    is_goods_list_inited = 0;
			TSessLineTbl::Rec line_rec;
			PPIDArray goods_id_list;
			for(P_Tbl->InitLineEnum(pRec->ID, &hdl_ln_enum); P_Tbl->NextLineEnum(hdl_ln_enum, &line_rec) > 0;) {
				if(!is_goods_list_inited) {
					TecObj.GetGoodsListByPrc(pRec->PrcID, &goods_id_list);
					is_goods_list_inited = 1;
				}
				if(!(line_rec.Flags & TSESLF_AUTOMAIN) && goods_id_list.lsearch(line_rec.GoodsID))
					if(GetTechByGoods(line_rec.GoodsID, pRec->PrcID, &inner_tec_rec) > 0)
						if(CalcPlannedTiming(inner_tec_rec.ID, line_rec.Qtty, 0, &inner_timing) > 0)
							timing += inner_timing;
			}
		}
		ok = 1;
	}
	pRec->PlannedTiming = timing;
	P_Tbl->DestroyIter(hdl_ln_enum);
	return ok;
}

int SLAPI PPObjTSession::Helper_SetSessionState(TSessionTbl::Rec * pRec, int newState, int checkOnly, int updateChilds)
{
	// 0 -> TSESST_PLANNED
	// 0 -> TSESST_PENDING
	// 0 -> TSESST_INPROCESS
	//
	// TSESST_PLANNED -> TSESST_PENDING
	// TSESST_PLANNED -> TSESST_INPROCESS   Проверяет наличие сессий со статусом TSESST_INPROCESS для //
	//        процессора. Устанавливается время начала сессии
	// @v5.0.12 TSESST_PLANNED -> TSESST_CLOSED      Устанавливает уровень завершенности в 10
	// TSESST_PLANNED -> TSESST_CANCELED
	//
	// TSESST_PENDING -> TSESST_INPROCESS   Устанавливается время начала сессии
	// @v5.0.12 TSESST_PENDING -> TSESST_CLOSED      Устанавливает уровень завершенности в 10
	// TSESST_PENDING -> TSESST_CANCELED
	//
	// TSESST_INPROCESS -> TSESST_CLOSED    Устанавливается время окончания сессии и уровень завершенности в 10
	//
	// TSESST_CLOSED   переходы недопустимы если в процессоре не установлен флаг PRCF_ALLOWCANCELAFTERCLOSE
	// TSESST_CANCELED переходы недопустимы

	int    ok = 1;
	const  int idle = BIN(pRec->Flags & TSESF_IDLE);
	ProcessorTbl::Rec prc_rec;
	if(newState != pRec->Status) {
		int    update_time = 0;
		if(!checkOnly && oneof2(newState, TSESST_INPROCESS, TSESST_CLOSED)) {
			if(GetConfig().Flags & PPTSessConfig::fUpdateTimeOnStatus)
				update_time = 1;
		}
		if(pRec->Status == 0) {
			if(newState == TSESST_INPROCESS) {
				THROW(IsProcessorInProcess(pRec->PrcID, idle, 0) < 0);
				SetSessCurTime(pRec, 0, update_time);
			}
			else
				THROW_PP(oneof2(newState, TSESST_PLANNED, TSESST_PENDING), PPERR_INVTSESSTTRANSITION);
		}
		else if(pRec->Status == TSESST_PLANNED) {
			if(newState == TSESST_INPROCESS) {
				THROW(IsProcessorInProcess(pRec->PrcID, idle, 0) < 0);
				SetSessCurTime(pRec, 0, update_time);
			}
			else if(newState == TSESST_CLOSED) {
				if(!checkOnly)
					pRec->Incomplete = 10;
			}
			else
				THROW_PP(oneof2(newState, TSESST_PENDING, TSESST_CANCELED), PPERR_INVTSESSTTRANSITION);
		}
		else if(pRec->Status == TSESST_PENDING) {
			if(newState == TSESST_INPROCESS) {
				THROW(IsProcessorInProcess(pRec->PrcID, idle, 0) < 0);
				SetSessCurTime(pRec, 0, update_time);
			}
			else if(newState == TSESST_CLOSED) {
				if(!checkOnly)
					pRec->Incomplete = 10;
			}
			else
				THROW_PP(newState == TSESST_CANCELED, PPERR_INVTSESSTTRANSITION);
		}
		else if(pRec->Status == TSESST_INPROCESS) {
			if(newState == TSESST_CLOSED) {
				if(!checkOnly) {
					SetSessCurTime(pRec, 1, pRec->FinDt ? update_time : 1);
					pRec->Incomplete = 10;
				}
			}
			else
				CALLEXCEPT_PP(PPERR_INVTSESSTTRANSITION);
		}
		else if(pRec->Status == TSESST_CLOSED) {
			if(newState == TSESST_CANCELED && GetPrc(pRec->PrcID, &prc_rec, 1, 1) > 0 && prc_rec.Flags & PRCF_ALLOWCANCELAFTERCLOSE) {
			}
			else {
				CALLEXCEPT_PP(PPERR_INVTSESSTTRANSITION);
			}
		}
		else if(pRec->Status == TSESST_CANCELED) {
			CALLEXCEPT_PP(PPERR_INVTSESSTTRANSITION);
		}
	}
	if(pRec->Flags & TSESF_SUPERSESS) {
		if(pRec->ID && GetPrc(pRec->PrcID, &prc_rec, 0) > 0 && prc_rec.Flags & PRCF_INDUCTSUPERSESSTATUS) {
			PPIDArray child_list;
			P_Tbl->GetChildIDList(pRec->ID, 0, &child_list);
			int    new_child_status = 0;
			if(newState == TSESST_PENDING)
				new_child_status = TSESST_PENDING;
			else if(newState == TSESST_INPROCESS)
				new_child_status = TSESST_PENDING;
			else if(newState == TSESST_CLOSED)
				new_child_status = TSESST_CLOSED;
			for(uint i = 0; i < child_list.getCount(); i++) {
				PPID   child_id = child_list.at(i);
				TSessionTbl::Rec child_rec;
				if(Search(child_id, &child_rec) > 0 && child_rec.Status != TSESST_CANCELED)
					if(updateChilds) {
						THROW(Helper_SetSessionState(&child_rec, new_child_status, 0, 1)); // @recursion
						THROW(PutRec(&child_id, &child_rec, 0));
					}
					else
						THROW(Helper_SetSessionState(&child_rec, new_child_status, 1, 0)); // @recursion
			}
		}
	}
	if(!checkOnly)
		pRec->Status = newState;
	CATCHZOK
	return ok;
}

int SLAPI PPObjTSession::CheckSuperSessLink(const TSessionTbl::Rec * pRec, PPID superSessID, TSessionTbl::Rec * pSuperSessRec)
{
	int    ok = 1;
	if(superSessID) {
		TSessionTbl::Rec super_rec;
		//
		// Суперсессию нельзя делать дочерней сессей (присваивать ей ссылку на суперсессию)
		//
		THROW_PP(!(pRec->Flags & TSESF_SUPERSESS), PPERR_CHILDCANTBESUPERSESS);
		if(pRec->ParentID != superSessID)
			//
			// Нельзя переприсваивать закрытую или отмененную сессию другой суперсессии
			//
			THROW_PP(pRec->Status != TSESST_CLOSED && pRec->Status != TSESST_CANCELED, PPERR_INVCHILDTSESSSTATUS);
		//
		// Проверка идентификатора суперсессии:
		// 1. Запись по этому идентификатору должна существовать
		// 2. Запись суперсессии должна иметь признак TSESF_SUPERSESS
		//
		THROW(Search(superSessID, &super_rec) > 0);
		ASSIGN_PTR(pSuperSessRec, super_rec);
		THROW_PP(pRec->Flags & TSESF_SUBSESS || super_rec.Flags & TSESF_SUPERSESS, PPERR_PARENTISNTSUPERSESS);
		if(pRec->ParentID != superSessID)
			//
			// Отмененной суперсессии нельзя присваивать дочернюю сессию
			//
 			THROW_PP(super_rec.Status != TSESST_CANCELED, PPERR_INVSUPERSESSSTATUS);
		if(pRec->Flags & TSESF_SUBSESS) {
			//
			// Субсессия должна иметь технологию, являющуюся прямым потомком технологии
			// родительской сессии.
			//
			if(pRec->TechID && super_rec.TechID && super_rec.TechID != pRec->TechID) {
				PPIDArray tech_list;
				TecObj.GetChildList(super_rec.TechID, tech_list);
				THROW_PP(tech_list.lsearch(pRec->TechID), PPERR_INCOMPSUBSESSTECH);
			}
		}
		else {
			//
			// Суперсессия должна быть привязана либо к тому же процессору, что и
			// проверяемая сессия, либо процессор суперсессии должен быть группой, к которой
			// принадлежит процессор проверяемой сессии.
			//
			if(pRec->PrcID && super_rec.PrcID && super_rec.PrcID != pRec->PrcID) {
				PPIDArray prc_list;
				THROW(PrcObj.GetParentsList(pRec->PrcID, &prc_list));
				THROW_PP(prc_list.lsearch(super_rec.PrcID), PPERR_INCOMPSUPERSESSPRC);
			}
		}
		if(pRec->StDt && super_rec.StDt) {
			//
			// Время начала дочерней сессии должно быть больше или равно
			// времени начала суперсессии
			//
			const long tm_diff = ((long)pRec->StTm.v) - ((long)super_rec.StTm.v);
			THROW_PP(pRec->StDt > super_rec.StDt || (pRec->StDt == super_rec.StDt && tm_diff > -99), PPERR_CHILDOLDERSUPERSESS);
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjTSession::InductSuperSess(TSessionTbl::Rec * pRec)
{
	int    ok = -1;
	TSessionTbl::Rec sup_rec;
	if(Search(pRec->ParentID, &sup_rec) > 0) {
		if(pRec->Flags & TSESF_SUBSESS) {
			TechTbl::Rec sup_tec_rec, tec_rec;
			if(sup_rec.TechID && TecObj.Fetch(sup_rec.TechID, &sup_tec_rec) > 0) {
				TSessionTbl::Rec last_sess_rec;
				PPID   last_sess_id = 0;
				LDATETIME last_sess_finish = ZERODATETIME;
				TSessionTbl::Key6 k6;
				MEMSZERO(k6);
				BExtQuery q(P_Tbl, 6);
				q.select(P_Tbl->ID, P_Tbl->FinDt, P_Tbl->FinTm, P_Tbl->TechID, 0L).where(P_Tbl->ParentID == pRec->ParentID);
				k6.ParentID = pRec->ParentID;
				for(q.initIteration(0, &k6, spGe); q.nextIteration() > 0;) {
					if(cmp(last_sess_finish, P_Tbl->data.FinDt, P_Tbl->data.FinTm) < 0) {
						last_sess_finish.Set(P_Tbl->data.FinDt, P_Tbl->data.FinTm);
						last_sess_id = P_Tbl->data.ID;
					}
				}
				if(last_sess_id && Search(last_sess_id, &last_sess_rec) > 0) {
					if(!!last_sess_finish && !pRec->StDt) {
						last_sess_finish.addsec(1);
						pRec->StDt = last_sess_finish.d;
						pRec->StTm = last_sess_finish.t;
						ok = 1;
					}
					if(!pRec->TechID) {
						if(TecObj.GetNextSibling(sup_rec.TechID, last_sess_rec.TechID, &tec_rec) > 0) {
							pRec->TechID = tec_rec.ID;
							pRec->PrcID = tec_rec.PrcID;
							ok = 1;
						}
					}
				}
				else {
					if(!pRec->StDt) {
						pRec->StDt = sup_rec.StDt;
						pRec->StTm = sup_rec.StTm;
						ok = 1;
					}
					if(sup_rec.TechID && TecObj.GetNextSibling(sup_rec.TechID, 0, &tec_rec) > 0) {
						pRec->TechID = tec_rec.ID;
						pRec->PrcID = tec_rec.PrcID;
						ok = 1;
					}
				}
				if(pRec->TechID && TecObj.Fetch(pRec->TechID, &tec_rec) > 0) {
					int    same_goods = 0;
					if(tec_rec.GoodsID == sup_tec_rec.GoodsID)
						same_goods = 1;
					else {
						PPID   tec_gen_goods_id = 0;
						PPID   sup_gen_goods_id = 0;
						if(GObj.BelongToGen(sup_tec_rec.GoodsID, &sup_gen_goods_id, 0) > 0) {
							if(GObj.BelongToGen(tec_rec.GoodsID, &tec_gen_goods_id, 0) > 0) {
								if(sup_gen_goods_id == tec_gen_goods_id)
									same_goods = 1;
							}
						}
						else if(GObj.IsGeneric(sup_tec_rec.GoodsID) && GObj.BelongToGen(tec_rec.GoodsID, &sup_tec_rec.GoodsID, 0) > 0) {
							same_goods = 1;
						}
					}
					if(same_goods) {
						pRec->PlannedQtty = sup_rec.PlannedQtty;
					}
				}
			}
		}
		else {
			ProcessorTbl::Rec prc_rec, sup_prc_rec;
			if(!pRec->StDt) {
				pRec->StDt = sup_rec.StDt;
				ok = 1;
			}
			if(!pRec->StTm) {
				pRec->StTm = sup_rec.StTm;
				ok = 1;
			}
			if(pRec->PrcID == 0 && GetPrc(sup_rec.PrcID, &sup_prc_rec, 1) > 0 && sup_prc_rec.Kind == PPPRCK_PROCESSOR) {
				pRec->PrcID = sup_rec.PrcID;
				SETIFZ(pRec->ArID, sup_rec.ArID);
				SETIFZ(pRec->Ar2ID, sup_rec.Ar2ID);
				if(pRec->StDt == sup_rec.StDt && pRec->StTm == sup_rec.StTm)
					pRec->StTm.addhs(100);
				ok = 1;
			}
			if(pRec->ArID == 0 && sup_rec.ArID && pRec->PrcID) {
				if(GetPrc(pRec->PrcID, &prc_rec, 1) > 0 && GetPrc(sup_rec.PrcID, &sup_prc_rec, 1) > 0) {
					if(sup_prc_rec.WrOffOpID == prc_rec.WrOffOpID) {
						pRec->ArID = sup_rec.ArID;
						SETIFZ(pRec->Ar2ID, sup_rec.Ar2ID);
						ok = 1;
					}
				}
			}
		}
	}
	else
		ok = -2;
	return ok;
}

int SLAPI PPObjTSession::SetSCardID(TSessionTbl::Rec * pRec, const SCardTbl::Rec * pSCardRec)
{
	int    ok = 1;
	if(pSCardRec == 0 || pSCardRec->ID == 0) {
		pRec->SCardID = 0;
	}
	else {
		pRec->SCardID = pSCardRec->ID;
		if(pSCardRec->PersonID) {
			ProcessorTbl::Rec prc_rec;
			PPOprKind op_rec;
			if(GetPrc(pRec->PrcID, &prc_rec, 1, 1) > 0 && GetOpData(prc_rec.WrOffOpID, &op_rec) > 0) {
				PPObjArticle ar_obj;
				PPID   ar_id = 0;
				if(ar_obj.GetByPerson(op_rec.AccSheetID, pSCardRec->PersonID, &ar_id) > 0) {
					if(!pRec->ArID) {
						pRec->ArID = ar_id;
						ok = 2;
					}
				}
				else if(ar_obj.GetByPerson(op_rec.AccSheet2ID, pSCardRec->PersonID, &ar_id) > 0) {
					if(!pRec->Ar2ID) {
						pRec->Ar2ID = ar_id;
						ok = 2;
					}
				}
			}
		}
	}
	return ok;
}

int SLAPI PPObjTSession::InitPacket(TSessionPacket * pPack, int kind /* TSESK_XXX */, PPID prcID, PPID superSessID, int status)
{
	int    ok = InitRec(pPack ? &pPack->Rec : static_cast<TSessionTbl::Rec *>(0), kind, prcID, superSessID, status);
	if(ok) {
		if(pPack) {
			pPack->CiList.Init(PPCheckInPersonItem::kTSession, 0);
		}
	}
	return ok;
}

int SLAPI PPObjTSession::InitRec(TSessionTbl::Rec * pRec, int kind /* TSESK_XXX */, PPID prcID, PPID superSessID, int status)
{
	int    ok = 1;
	TSessionTbl::Rec rec;
	MEMSZERO(rec);
	ProcessorTbl::Rec prc_rec;
	rec.Incomplete = 10;
	if(PrcObj.GetRecWithInheritance(labs(prcID), &prc_rec) > 0) {
		rec.PrcID = prc_rec.ID;
		rec.ArID  = prc_rec.WrOffArID;
	}
	else
		MEMSZERO(prc_rec);
	if(ValidateStatus(status))
		rec.Status = status;
	else {
		PPProcessorPacket::ExtBlock prc_ext;
		if(PrcObj.GetExtWithInheritance(labs(prcID), &prc_ext) > 0 && ValidateStatus(prc_ext.InitSessStatus)) {
			rec.Status = (int16)prc_ext.InitSessStatus;
		}
		else
			rec.Status = TSESST_PLANNED;
	}
	if(kind == TSESK_SUPERSESS) {
		rec.Flags |= TSESF_SUPERSESS;
		rec.PlannedTiming = prc_rec.SuperSessTiming;
	}
	else if(kind == TSESK_PLAN) {
		rec.Flags |= TSESF_PLAN;
	}
	else if(kind == TSESK_SESSION) {
		if(GetConfig().InitTime)
			rec.StTm = GetConfig().InitTime;
	}
	else if(kind == TSESK_SUBSESS) {
		rec.Flags |= TSESF_SUBSESS;
		if(GetConfig().InitTime)
			rec.StTm = GetConfig().InitTime;
	}
	if(superSessID) {
		TSessionTbl::Rec sup_rec;
		THROW(CheckSuperSessLink(&rec, superSessID, &sup_rec));
		rec.ParentID = superSessID;
		THROW(InductSuperSess(&rec));
	}
	if(!rec.StTm)
		getcurtime(&rec.StTm);
	ASSIGN_PTR(pRec, rec);
	CATCHZOK
	return ok;
}

int SLAPI PPObjTSession::CreateOnlineByLinkBill(PPID * pSessID,
	const ProcessorTbl::Rec * pPrcRec, const BillTbl::Rec * pBillRec)
{
	int    ok = 1;
	PPID   sess_id = 0;
	TSessionTbl::Rec rec;
	THROW_INVARG(pPrcRec && pBillRec);
	THROW(InitRec(&rec, 0, pPrcRec->ID, 0, 0));
	getcurdatetime(&rec.StDt, &rec.StTm);
	rec.Incomplete = 10;
	rec.LinkBillID = pBillRec->ID;
	if(pPrcRec && pBillRec && pPrcRec->WrOffOpID) {
		PPOprKind op_rec;
		if(GetOpData(pPrcRec->WrOffOpID, &op_rec) > 0) {
			PPObjArticle ar_obj;
			ArticleTbl::Rec ar_rec;
			if(ar_obj.Fetch(pBillRec->Object, &ar_rec) > 0 && ar_rec.AccSheetID == op_rec.AccSheetID)
				rec.ArID = pBillRec->Object;
		}
	}
	THROW(SetSessionState(&rec, TSESST_INPROCESS, 0));
	THROW(PutRec(&sess_id, &rec, 1));
	CATCHZOK
	ASSIGN_PTR(pSessID, sess_id);
	return ok;
}

int SLAPI PPObjTSession::DeleteObj(PPID id)
{
	return CheckRights(PPR_DEL) ? P_Tbl->Put(&id, 0, 0) : 0;
}

int SLAPI PPObjTSession::IsTimingTech(const TechTbl::Rec * pTechRec, double * pBaseRatio)
{
	int    ok = -1;
	double ratio = 0.0;
	if(pTechRec && pTechRec->GoodsID) {
		Goods2Tbl::Rec goods_rec;
		THROW(GObj.Fetch(pTechRec->GoodsID, &goods_rec) > 0);
		// @v9.9.12 {
		if(GObj.TranslateGoodsUnitToBase(goods_rec, PPUNT_SECOND, &ratio) > 0) {
			ok = 1;
		}
		// } @v9.9.12
		/* @v9.9.12 
		PPUnit unit_rec;
		THROW(GObj.FetchUnit(goods_rec.UnitID, &unit_rec) > 0);
		if(unit_rec.BaseUnitID == PPUNT_SECOND && unit_rec.BaseRatio) {
			ratio = unit_rec.BaseRatio;
			ok = 1;
		}*/
	}
	CATCHZOK
	ASSIGN_PTR(pBaseRatio, ratio);
	return ok;
}

int SLAPI PPObjTSession::PutTimingLine(const TSessionTbl::Rec * pPack)
{
	int    ok = -1;
	long   hdl_ln_enum = -1;
	TechTbl::Rec tec_rec;
	if(GetTech(pPack->TechID, &tec_rec, 1) > 0 && tec_rec.GoodsID) {
		Goods2Tbl::Rec goods_rec;
		double unit_ratio = 1.0;
		if(GObj.Fetch(tec_rec.GoodsID, &goods_rec) > 0) {
			double qtty = 0.0;
			//
			// @v8.1.6 Поменялись местами условия:
			// Было:
			// if(tec_rec.Flags & TECF_AUTOMAIN) {
			// }
			// else if(IsTimingTech(&tec_rec, &unit_ratio) > 0) {
			// }
			//
			// Стало:
			// if(IsTimingTech(&tec_rec, &unit_ratio) > 0) {
			// }
			// else if(tec_rec.Flags & TECF_AUTOMAIN) {
			// }
			//
			if(IsTimingTech(&tec_rec, &unit_ratio) > 0) {
				long   timing = 0;
				STimeChunk chunk, result_chunk;
				chunk.Start.Set(pPack->StDt, pPack->StTm);
				chunk.Finish.Set(pPack->FinDt, pPack->FinTm);
				if(AdjustTiming(*pPack, chunk, result_chunk, &timing) > 0)
					qtty = Round((double)timing / unit_ratio, tec_rec.Rounding, +1);
				else
					qtty = Round((double)GetContinuation(pPack) / unit_ratio, tec_rec.Rounding, +1);
			}
			else if(tec_rec.Flags & TECF_AUTOMAIN) {
				qtty = Round(pPack->PlannedQtty, tec_rec.Rounding, 0);
			}
			if(qtty != 0.0) {
				TSessLineTbl::Rec line_rec, ex_line_rec;
				long   oprno = 0;
				THROW(InitLinePacket(&line_rec, pPack->ID) > 0);
				THROW(SetupLineGoods(&line_rec, tec_rec.GoodsID, 0, 0));
				line_rec.Sign = -1;
				line_rec.Flags |= TSESLF_AUTOMAIN;
				line_rec.Qtty = qtty;
				{
					THROW(P_Tbl->InitLineEnum(pPack->ID, &hdl_ln_enum));
					while(P_Tbl->NextLineEnum(hdl_ln_enum, &ex_line_rec) > 0)
						if(ex_line_rec.Flags & TSESLF_AUTOMAIN)
							THROW(PutLine(pPack->ID, &ex_line_rec.OprNo, 0, 0));
				}
				THROW(PutLine(pPack->ID, &oprno, &line_rec, 0));
				ok = 1;
			}
		}
	}
	CATCHZOK
	P_Tbl->DestroyIter(hdl_ln_enum);
	return ok;
}

int SLAPI PPObjTSession::GetTagList(PPID id, ObjTagList * pTagList)
	{ return PPRef->Ot.GetList(Obj, id, pTagList); }
int SLAPI PPObjTSession::SetTagList(PPID id, const ObjTagList * pTagList, int use_ta)
	{ return PPRef->Ot.PutList(Obj, id, pTagList, use_ta); }

int SLAPI PPObjTSession::PutExtention(PPID id, PPProcessorPacket::ExtBlock * pExt, int use_ta)
{
	int    ok = 1;
	SBuffer buffer;
	SSerializeContext sctx;
	if(pExt && !pExt->IsEmpty()) {
		THROW(pExt->Serialize(+1, buffer, &sctx));
	}
	THROW(PPRef->PutPropSBuffer(Obj, id, TSESPRP_EXT, buffer, use_ta));
	CATCHZOK
	return ok;
}

int SLAPI PPObjTSession::GetExtention(PPID id, PPProcessorPacket::ExtBlock * pExt)
{
	int    ok = -1;
	size_t sz = 0;
	SBuffer buffer;
	if(PPRef->GetPropSBuffer(Obj, id, TSESPRP_EXT, buffer) > 0) {
		if(pExt) {
			SSerializeContext sctx;
			THROW(pExt->Serialize(-1, buffer, &sctx));
		}
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjTSession::GetPacket(PPID id, TSessionPacket * pPack, long options)
{
	int    ok = -1;
	pPack->destroy();
	if(PPCheckGetObjPacketID(Obj, id)) { // @v10.3.6
		int    r = Search(id, &pPack->Rec);
		if(r > 0) {
			PPCheckInPersonMngr ci_mgr;
			THROW(ci_mgr.GetList(PPCheckInPersonItem::kTSession, id, pPack->CiList));
			if(options & gpoLoadLines) {
				TSessLineTbl::Rec line_rec;
				for(SEnum en = P_Tbl->EnumLines(id); en.Next(&line_rec) > 0;) {
					THROW_SL(pPack->Lines.insert(&line_rec));
				}
				pPack->Flags |= TSessionPacket::fLinesInited;
			}
			THROW(GetTagList(pPack->Rec.ID, &pPack->TagL));
			THROW(GetExtention(pPack->Rec.ID, &pPack->Ext));
			ok = 1;
		}
		else {
			THROW(r);
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjTSession::PutPacket(PPID * pID, TSessionPacket * pPack, int use_ta)
{
	int    ok = 1;
	int    acn = 0;
	long   preserve_cfg_flags = Cfg.Flags;
	TSessionPacket old_pack;
	PPCheckInPersonMngr ci_mgr;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(*pID) {
			THROW(GetPacket(*pID, &old_pack, 0) > 0);
		}
		if(pPack == 0) {
			if(*pID) {
				THROW(RemoveObjV(*pID, 0, 0, 0));
				THROW(PutExtention(*pID, 0, 0));
				old_pack.LinkFiles.Init(Obj);
				old_pack.LinkFiles.Save(*pID, 0L);
			}
		}
		else {
			THROW(CheckRightsModByID(pID));
			if(pID)
				acn = PPACN_OBJUPD;
			else
				acn = PPACN_OBJADD;
			if(pPack->Rec.Flags & TSESF_SUPERSESS) {
				ProcessorTbl::Rec prc_rec;
				if(GetPrc(pPack->Rec.PrcID, &prc_rec, 0) > 0 && prc_rec.Flags & PRCF_INDUCTSUPERSESSTATUS)
					THROW(Helper_SetSessionState(&pPack->Rec, pPack->Rec.Status, 1, 1));
			}
			THROW(P_Tbl->Put(pID, &pPack->Rec, 0));
			THROW(NormalizePacket(pPack, 0));
			//
			// В функции PutLine, которая вызывается из PutTimingLine не должна срабатывать
			// защита от изменения строк в списанной сессии
			//
			GetConfig(); // Гарантируем извлечение конфигурации из базы данных
			preserve_cfg_flags = Cfg.Flags;
			Cfg.Flags |= PPTSessConfig::fAllowLinesInWrOffSessions;
			THROW(PutTimingLine(&pPack->Rec));
			THROW(ci_mgr.Put(pPack->CiList, 0));
			if(pPack->LinkFiles.IsChanged(*pID, 0L)) {
				pPack->LinkFiles.Save(*pID, 0L);
			}
			THROW(SetTagList(*pID, &pPack->TagL, 0));
			THROW(PutExtention(*pID, &pPack->Ext, 0));
		}
		if(acn)
			DS.LogAction(acn, Obj, *pID, 0, 0);
		THROW(tra.Commit());
		if(*pID)
			Dirty(*pID);
	}
	CATCHZOK
	Cfg.Flags = preserve_cfg_flags;
	return ok;
}

int SLAPI PPObjTSession::PutRec(PPID * pID, TSessionTbl::Rec * pRec, int use_ta)
{
	int    ok = 1;
	long   preserve_cfg_flags = Cfg.Flags;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(*pID && pRec == 0) {
			THROW(RemoveObjV(*pID, 0, 0, 0));
		}
		else {
			THROW(CheckRightsModByID(pID));
			if(pRec->Flags & TSESF_SUPERSESS) {
				ProcessorTbl::Rec prc_rec;
				if(GetPrc(pRec->PrcID, &prc_rec, 0) > 0 && prc_rec.Flags & PRCF_INDUCTSUPERSESSTATUS)
					THROW(Helper_SetSessionState(pRec, pRec->Status, 1, 1));
			}
			THROW(P_Tbl->Put(pID, pRec, 0));
			//
			// В функции PutLine, которая вызывается из PutTimingLine не должна срабатывать
			// защита от изменения строк в списанной сессии
			//
			GetConfig(); // Гарантируем извлечение конфигурации из базы данных
			preserve_cfg_flags = Cfg.Flags;
			Cfg.Flags |= PPTSessConfig::fAllowLinesInWrOffSessions;
			THROW(PutTimingLine(pRec));
		}
		THROW(tra.Commit());
	}
	if(*pID)
		Dirty(*pID);
	CATCHZOK
	Cfg.Flags = preserve_cfg_flags;
	return ok;
}

int SLAPI PPObjTSession::InitLinePacket(TSessLineTbl::Rec * pRec, PPID sessID)
{
	int    ok = -1;
	TSessLineTbl::Rec rec;
	MEMSZERO(rec);
	TSessionTbl::Rec tses_rec;
	if(Search(sessID, &tses_rec) > 0) {
		rec.TSessID = sessID;
		rec.UserID  = LConfig.User;
		getcurdatetime(&rec.Dt, &rec.Tm);
		P_Tbl->AdjustLineTime(&rec);
		if(tses_rec.Flags & TSESF_PLAN && tses_rec.Flags & TSESF_PLAN_PHUNIT)
			rec.Flags |= TSESLF_PLAN_PHUNIT;
		ok = 1;
	}
	ASSIGN_PTR(pRec, rec);
	return ok;
}

int SLAPI PPObjTSession::Browse(void * extraPtr)
{
	SelFilt sel_par;
	if(ConvertExtraParam(extraPtr, &sel_par) > 0) {
		TSessionFilt filt;
		filt.SuperSessID = sel_par.SuperSessID;
		filt.PrcID = sel_par.PrcID;
		SETFLAG(filt.Flags, TSessionFilt::fSuperSessOnly, sel_par.Kind == 1);
		ViewTSession(&filt);
	}
	else
		ViewTSession(0);
	return 1;
}

int SLAPI PPObjTSession::MakeName(const TSessionTbl::Rec * pRec, SString & rName)
{
	int    ok = 1;
	if(pRec) {
		ProcessorTbl::Rec prc_rec;
		if(PrcObj.Fetch(pRec->PrcID, &prc_rec) > 0)
			rName = prc_rec.Name;
		else
			ideqvalstr(pRec->PrcID, rName.Z());
		rName.CatDiv(':', 1).CatLongZ(pRec->Num, 4).CatDiv(':', 1);
		if(pRec->Flags & TSESF_PLAN) {
			DateRange period;
			rName.Cat(period.Set(pRec->StDt, pRec->FinDt), 1);
		}
		else
			rName.Cat(pRec->StDt).Space().Cat(pRec->StTm, TIMF_HM);
	}
	else {
		rName.Z();
		ok = -1;
	}
	return ok;
}

const char * SLAPI PPObjTSession::GetNamePtr()
{
	MakeName(&P_Tbl->data, NameBuf);
	return NameBuf.cptr();
}

StrAssocArray * SLAPI PPObjTSession::MakeStrAssocList(void * extraPtr)
{
	SelFilt sel_par;
	ConvertExtraParam(extraPtr, &sel_par);
	StrAssocArray * p_list = 0;
	PPIDArray prc_list;
	SString name_buf;
	THROW_MEM(p_list = new StrAssocArray);
	if(sel_par.PrcID) {
		if(sel_par.Kind == 1)
			THROW(PrcObj.GetParentsList(sel_par.PrcID, &prc_list));
		prc_list.addUnique(sel_par.SuperSessID);
		prc_list.sort();
		for(uint i = 0; i < prc_list.getCount(); i++) {
			TSessionTbl::Key4 k4;
			MEMSZERO(k4);
			k4.PrcID = prc_list.at(i);
			BExtQuery q(P_Tbl, 4);
			q.select(P_Tbl->ID, P_Tbl->PrcID, P_Tbl->Num, P_Tbl->StDt, P_Tbl->StTm, P_Tbl->Flags, 0L).
				where(P_Tbl->PrcID == prc_list.at(i));
			for(q.initIteration(0, &k4, spGe); q.nextIteration() > 0;) {
				TSessionTbl::Rec rec;
				P_Tbl->copyBufTo(&rec);
				if(sel_par.Kind != 1 || rec.Flags & TSESF_SUPERSESS) {
					MakeName(&rec, name_buf);
					THROW_SL(p_list->Add(P_Tbl->data.ID, name_buf));
				}
			}
		}
	}
	else {
		TSessionTbl::Key2 k2;
		MEMSZERO(k2);
		BExtQuery q(P_Tbl, 2);
		q.select(P_Tbl->ID, P_Tbl->PrcID, P_Tbl->Num, P_Tbl->StDt, P_Tbl->StTm, P_Tbl->Flags, 0L);
		for(q.initIteration(0, &k2, spGe); q.nextIteration() > 0;) {
			TSessionTbl::Rec rec;
			P_Tbl->copyBufTo(&rec);
			if(sel_par.Kind != 1 || rec.Flags & TSESF_SUPERSESS) {
				MakeName(&rec, name_buf);
				THROW_SL(p_list->Add(P_Tbl->data.ID, name_buf));
			}
		}
	}
	p_list->SortByText();
	CATCH
		ZDELETE(p_list);
	ENDCATCH
	return p_list;
}

int SLAPI PPObjTSession::GetCode(const TSessionTbl::Rec * pRec, long flags, char * pBuf, size_t bufLen)
{
	if(pRec)
		longfmtz(pRec->Num, 5, pBuf, bufLen);
	else
		ASSIGN_PTR(pBuf, 0);
	return 1;
}

int SLAPI PPObjTSession::GetCode(PPID sessID, long flags, char * pBuf, size_t bufLen)
{
	int    ok = 1;
	TSessionTbl::Rec tses_rec;
	if(Search(sessID, &tses_rec) > 0)
		ok = GetCode(&tses_rec, flags, pBuf, bufLen);
	else {
		GetCode(static_cast<TSessionTbl::Rec *>(0), flags, pBuf, bufLen);
		ok = -1;
	}
	return ok;
}

int SLAPI PPObjTSession::GenerateSerial(TSessLineTbl::Rec * pRec)
{
	int    ok = -1;
	if(pRec->Sign >= 0) {
		PPObjBill * p_bobj = BillObj;
		char   templt[128]; // [48]-->[128]
		char   tses_code[32];
		SString serial;
		STRNSCPY(templt, p_bobj->Cfg.SnTemplt);
		GetCode(pRec->TSessID, 0, tses_code, sizeof(tses_code));
		ok = p_bobj->GetSnByTemplate(tses_code, pRec->GoodsID, 0, templt, serial);
		if(ok > 0)
			STRNSCPY(pRec->Serial, serial);
		else
			pRec->Serial[0] = 0;
	}
	return ok;
}

int SLAPI PPObjTSession::GetLabelInfo(PPID tsesID, long oprNo, PPID * pPrnID, RetailGoodsInfo * pData)
{
	int    ok = 1;
	PPID   prn_id = 0;
	RetailGoodsInfo rgi;
	TSessLineTbl::Rec line_rec;
	if(P_Tbl->SearchLine(tsesID, oprNo, &line_rec) > 0 && line_rec.Sign > 0) {
		TSessionTbl::Rec tses_rec;
		if(Search(line_rec.TSessID, &tses_rec) > 0) {
			PPID   loc_id = 0;
			int16  label_count = 0;
			SString temp_buf;
			ProcessorTbl::Rec prc_rec;
			if(PrcObj.GetRecWithInheritance(tses_rec.PrcID, &prc_rec) > 0) {
				loc_id = prc_rec.LocID;
				prn_id = prc_rec.PrinterID;
				label_count = prc_rec.LabelCount;
			}
			GoodsStockExt gse;
			GObj.GetRetailGoodsInfo(line_rec.GoodsID, loc_id, &rgi);
			rgi.LocID    = loc_id;
			rgi.Qtty     = fabs(line_rec.Qtty);
			rgi.PhQtty   = R6(line_rec.Qtty * rgi.PhUPerU);
			rgi.BillDate = tses_rec.StDt;
			STRNSCPY(rgi.Serial, line_rec.Serial);
			if(GObj.GetStockExt(line_rec.GoodsID, &gse) > 0) {
				rgi.Brutto = gse.CalcBrutto(rgi.Qtty);
				/*
				if(gse.Package > 0)
					rgi.Qtty = R0(rgi.Qtty / gse.Package);
				*/
			}
			GetCode(&tses_rec, 0, rgi.BillCode, sizeof(rgi.BillCode));
			GetArticleName(tses_rec.ArID, temp_buf); temp_buf.CopyTo(rgi.ArName, sizeof(rgi.ArName));
			GetArticleName(tses_rec.Ar2ID, temp_buf); temp_buf.CopyTo(rgi.Ar2Name, sizeof(rgi.Ar2Name));
			STRNSCPY(rgi.PrcName, prc_rec.Name);
			rgi.LabelCount = label_count;
		}
		rgi.RevalPrice = rgi.Price;
	}
	else
		ok = -1;
	ASSIGN_PTR(pPrnID, prn_id);
	ASSIGN_PTR(pData, rgi);
	return ok;
}

int SLAPI PPObjTSession::PrintBarLabel(PPID tsesID, long oprNo, int numCopies, int silent)
{
	int    ok = 1;
	RetailGoodsInfo rgi;
	PPID   prn_id = 0;
	if(GetLabelInfo(tsesID, oprNo, &prn_id, &rgi) > 0) {
		if(numCopies > 0)
			rgi.LabelCount = numCopies;
		ok = BarcodeLabelPrinter::PrintGoodsLabel2(&rgi, prn_id, silent);
	}
	else
		ok = -1;
	return ok;
}

int SLAPI PPObjTSession::GetGoodsStruc(PPID id, PPGoodsStruc * pGs)
{
	TSessionTbl::Rec rec;
	return (Search(id, &rec) > 0) ? TecObj.GetGoodsStruc(rec.TechID, pGs) : 0;
}

int SLAPI PPObjTSession::GetGoodsStrucList(PPID id, int useSubst, TGSArray * pList)
{
	TSessionTbl::Rec rec;
	return (Search(id, &rec) > 0) ? TecObj.GetGoodsStrucList(rec.TechID, useSubst, pList) : 0;
}

static const int TSess_UseQuot_As_Price = 1; // @v9.9.7 Временная константа дабы не вводить конфигурационный параметр

int SLAPI PPObjTSession::GetRgi(PPID goodsID, double qtty, const TSessionTbl::Rec & rTSesRec, long extRgiFlags, RetailGoodsInfo & rRgi)
{
	int    ok = -1;
	RetailPriceExtractor::ExtQuotBlock * p_eqb = 0;
	ProcessorTbl::Rec prc_rec;
	if(GetPrc(rTSesRec.PrcID, &prc_rec, 1) > 0) {
		SCardTbl::Rec sc_rec;
		PPObjTSession::WrOffAttrib attrib;
		const  PPID agent_ar_id = (GetWrOffAttrib(&rTSesRec, &attrib) > 0) ? attrib.AgentID : 0;
		const  long rgi_flags = TSess_UseQuot_As_Price ? (PPObjGoods::rgifUseQuotWTimePeriod|PPObjGoods::rgifUseBaseQuotAsPrice) : PPObjGoods::rgifUseQuotWTimePeriod;
		int    nodis = 0;
		LDATETIME actual_dtm; // = P.Eccd.InitDtm;
		actual_dtm.Set(rTSesRec.StDt, rTSesRec.StTm);
		if(rTSesRec.SCardID && ScObj.Fetch(rTSesRec.SCardID, &sc_rec) > 0) {
  			const  int cfg_dsbl_no_dis = 0;//BIN(CsObj.GetEqCfg().Flags & PPEquipConfig::fIgnoreNoDisGoodsTag);
			nodis = BIN(!cfg_dsbl_no_dis && GObj.CheckFlag(goodsID, GF_NODISCOUNT) > 0);
			if(!nodis) {
				PPObjSCardSeries scs_obj;
				PPSCardSeries scs_rec;
				PPSCardSerPacket scs_pack;
				if(scs_obj.GetPacket(sc_rec.SeriesID, &scs_pack) > 0) {
					RetailPriceExtractor::ExtQuotBlock temp_eqb(scs_pack);
					if(temp_eqb.QkList.getCount())
						p_eqb = new RetailPriceExtractor::ExtQuotBlock(scs_pack);
				}
			}
		}
		ok = GObj.GetRetailGoodsInfo(goodsID, prc_rec.LocID, p_eqb, agent_ar_id, actual_dtm, fabs(qtty), &rRgi, rgi_flags|extRgiFlags);
		SETFLAG(rRgi.Flags, RetailGoodsInfo::fNoDiscount, nodis);
	}
	delete p_eqb;
	return ok;
}

int SLAPI PPObjTSession::SetupLineGoods(TSessLineTbl::Rec * pRec, PPID goodsID, const char * pSerial, long)
{
	int    ok = -1;
	pRec->Flags &= ~TSESLF_RECOMPL;
	if(goodsID) {
		TSessionTbl::Rec tses_rec;
		int    sign = 0, is_recompl_tec = 0;
		TGSArray tgs_list;
		TechTbl::Rec tec_rec;
		ProcessorTbl::Rec prc_rec;
		THROW(Search(pRec->TSessID, &tses_rec) > 0);
		if(TecObj.Fetch(tses_rec.TechID, &tec_rec) > 0 && tec_rec.Flags & TECF_RECOMPLMAINGOODS)
			is_recompl_tec = 1;
		THROW(TecObj.GetGoodsStrucList(tses_rec.TechID, 1, &tgs_list));
		if(tgs_list.SearchGoods(goodsID, &sign) > 0) {
			pRec->GoodsID = goodsID;
			pRec->Sign    = sign;
			if(sign > 0) {
				if(is_recompl_tec && tec_rec.GoodsID == goodsID) {
					STRNSCPY(pRec->Serial, pSerial);
					SString last_serial;
					TSessLineTbl::Rec line_rec;
					for(SEnum en = P_Tbl->EnumLines(tses_rec.ID); en.Next(&line_rec) > 0;) {
						if(line_rec.GoodsID == goodsID && line_rec.Sign > 0) {
							last_serial = line_rec.Serial;
							if(pRec->Serial[0])
								THROW_PP(sstreq(pRec->Serial, line_rec.Serial), PPERR_TSESRECOMPLDIFSER);
						}
					}
					if(isempty(pRec->Serial))
						last_serial.CopyTo(pRec->Serial, sizeof(pRec->Serial));
					THROW_PP(pRec->Serial[0], PPERR_TSESRECOMPLNOSER);
					pRec->Flags |= TSESLF_RECOMPL;
				}
				else
					GenerateSerial(pRec);
			}
		}
		else {
			pRec->GoodsID = goodsID;
			pRec->Sign = (tses_rec.Flags & TSESF_PLAN) ? 1 : -1;
		}
		SETFLAG(pRec->Flags, TSESLF_INDEPPHQTTY, GObj.CheckFlag(goodsID, GF_USEINDEPWT));
		if(GetConfig().Flags & PPTSessConfig::fUsePricing && GetPrc(tses_rec.PrcID, &prc_rec, 1) > 0) {
			PPObjTSession::WrOffAttrib attrib;
			const PPID agent_ar_id = (GetWrOffAttrib(&tses_rec, &attrib) > 0) ? attrib.AgentID : 0;
			RetailExtrItem rpi;
			RetailPriceExtractor rpe(prc_rec.LocID, 0, agent_ar_id, ZERODATETIME, 0);
			rpe.GetPrice(goodsID, 0, 0.0, &rpi);
			pRec->Price = rpi.Price;
			SCardTbl::Rec sc_rec;
			if(SearchObject(PPOBJ_SCARD, tses_rec.SCardID, &sc_rec) > 0)
				pRec->Discount = (pRec->Price * fdiv100r(fdiv100i(sc_rec.PDis)));
		}
		if(pSerial)
			STRNSCPY(pRec->Serial, pSerial);
		ok = 1;
	}
	else
		pRec->GoodsID = 0;
	CATCHZOK
	return ok;
}

int SLAPI PPObjTSession::EditLine(PPID tsesID, long * pOprNo, PPID goodsID, const char * pSerial, double initQtty)
{
	int    ok = -1, valid_data = 0, r = -1;
	SString loc_serial;
	TSessionTbl::Rec tses_rec;
	TSessLineTbl::Rec line_rec;
	THROW(CheckRights(TSESRT_MODLINE));
	THROW(Search(tsesID, &tses_rec) > 0);
	if(*pOprNo) {
		THROW(P_Tbl->SearchLine(tsesID, *pOprNo, &line_rec) > 0);
	}
	else {
		THROW_PP(tses_rec.Incomplete || (GetConfig().Flags & PPTSessConfig::fAllowLinesInWrOffSessions), PPERR_ADDLNCOMPLTSESS);
		THROW(InitLinePacket(&line_rec, tsesID));
		if(goodsID) {
			do {
				r = SetupLineGoods(&line_rec, goodsID, pSerial, 0);
				if(r == 0) {
					if(PPErrCode == PPERR_TSESRECOMPLNOSER) {
						ProcessorTbl::Rec prc_rec;
						if(GetPrc(tses_rec.PrcID, &prc_rec, 1, 1) > 0) {
							SerialByGoodsListItem si;
							if(SelectSerialByGoods(goodsID, prc_rec.LocID, &si) > 0) {
								loc_serial = si.Serial;
								pSerial  = loc_serial;
								initQtty = si.Qtty;
								r = -1;
							}
						}
						THROW_PP(r == -1, PPERR_TSESRECOMPLNOSER);
					}
					else
						CALLEXCEPT();
				}
				else
					r = 1;
			} while(r < 0);
		}
		line_rec.Qtty = initQtty;
	}
	while(!valid_data && EditLineDialog(&line_rec, BIN(tses_rec.Flags & TSESF_PLAN)) > 0) {
		long   oprno = *pOprNo;
		if(PutLine(tsesID, &oprno, &line_rec, 1)) {
			*pOprNo = oprno;
			ok = valid_data = 1;
		}
		else
			PPError();
	}
	CATCHZOKPPERR
	return ok;
}

int SLAPI PPObjTSession::Add(PPID * pID, PPID superSessID, PPID prcID, int kind, int status)
{
	int    ok = -1;
	TSessionPacket pack;
	THROW(InitPacket(&pack, kind, prcID, superSessID, status));
	SETIFZ(pack.Rec.StDt, LConfig.OperDate);
	while(ok < 0 && EditDialog(&pack) > 0) {
		*pID = pack.Rec.ID; // Функция EditDialog могла сохранить сессию, инициализировав идентификатор
		if(PutPacket(pID, &pack, 1))
			ok = 1;
		else
			PPError();
	}
	CATCHZOKPPERR
	return ok;
}

int SLAPI PPObjTSession::Edit(PPID * pID, void * extraPtr)
{
	int    ok = cmCancel, valid_data = 0;
	TSessionPacket pack;
	if(*pID) {
		THROW(GetPacket(*pID, &pack, 0) > 0);
	}
	else {
		SelFilt sel_par;
		ConvertExtraParam(extraPtr, &sel_par);
		THROW(InitPacket(&pack, sel_par.Kind, sel_par.PrcID, sel_par.SuperSessID, 0));
		SETIFZ(pack.Rec.StDt, LConfig.OperDate);
	}
	while(!valid_data && EditDialog(&pack) > 0) {
		if(*pID == 0 && pack.Rec.ID != 0)
			*pID = pack.Rec.ID;
		if(PutPacket(pID, &pack, 1))
			ok = valid_data = cmOK;
		else
			PPError();
	}
	CATCHZOKPPERR
	return ok;
}

int SLAPI PPObjTSession::EditNewIdleSession(PPID prcID, PPID curSessID, PPID * pSessID)
{
	int    ok = -1;
	PPID   super_id = 0;
	TDialog * dlg = 0;
	TSessionTbl::Rec rec, cur_rec;
	if(Search(curSessID, &cur_rec) > 0) {
		THROW(InitRec(&rec, 0, prcID, cur_rec.ParentID, 0));
	}
	else {
		THROW(InitRec(&rec, 0, prcID, 0, 0));
	}
	getcurdatetime(&rec.StDt, &rec.StTm);
	rec.Incomplete = 10;
	rec.Flags |= TSESF_IDLE;
	THROW(SetSessionState(&rec, TSESST_INPROCESS, 0));
	THROW(CheckDialogPtr(&(dlg = new TDialog(DLG_TSESSIDLE))));
	{
		if(GetConfig().IdleAccSheetID)
			SetupArCombo(dlg, CTLSEL_TSESS_OBJ2, rec.Ar2ID, OLW_CANINSERT, GetConfig().IdleAccSheetID, sacfDisableIfZeroSheet|sacfNonGeneric);
		else {
			ProcessorTbl::Rec prc_rec;
			if(PrcObj.GetRecWithInheritance(labs(prcID), &prc_rec) > 0) {
				PPOprKind op_rec;
				GetOpData(prc_rec.WrOffOpID, &op_rec);
				SetupArCombo(dlg, CTLSEL_TSESS_OBJ2, rec.Ar2ID, OLW_CANINSERT, op_rec.AccSheet2ID, sacfDisableIfZeroSheet|sacfNonGeneric);
			}
		}
		dlg->setCtrlData(CTL_TSESS_MEMO, rec.Memo);
		while(ok < 0 && ExecView(dlg) == cmOK) {
			dlg->getCtrlData(CTLSEL_TSESS_OBJ2, &rec.Ar2ID);
			dlg->getCtrlData(CTL_TSESS_MEMO, rec.Memo);
			if(PutRec(pSessID, &rec, 1))
				ok = 1;
			else
				PPError();
		}
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}

//int SLAPI PPObjTSession::Remove(PPID sessID, long, uint options)
//virtual
int SLAPI PPObjTSession::RemoveObjV(PPID sessID, ObjCollection * pObjColl, uint options/* = rmv_default*/, void * pExtraParam)
{
	int    ok = 1;
	int    level = 0; // 0 - remove, 1 - undo writing off, -1 - cancel
	THROW(CheckRights(PPR_DEL));
	if(options & user_request) {
		TSessionTbl::Rec rec;
		THROW(Search(sessID, &rec) > 0);
		if(rec.Flags & TSESF_WRITEDOFF) {
			uint   r = 0;
			if(SelectorDialog(DLG_RMVTSESS, CTL_RMVTSESS_WHAT, &r) > 0)
				if(r == 0)
					level = 1;
				else if(r == 1)
					level = 0;
				else
					level = -1;
			else
				level = -1;
		}
		else if(CONFIRM(PPCFM_DELETE))
			level = 0;
		else
			level = -1;
	}
	if(level >= 0) {
		PPTransaction tra(BIN(options & use_transaction));
		THROW(tra);
		if(level == 1) {
			THROW(UndoWritingOff(sessID, 0));
		}
		else if(level == 0) {
			THROW(UndoWritingOff(sessID, 0));
			THROW(P_Tbl->Put(&sessID, 0, 0));
			THROW(SetTagList(sessID, 0, 0));
			THROW(RemoveSync(sessID));
			DS.LogAction(PPACN_OBJRMV, Obj, sessID, 0, 0);
		}
		THROW(tra.Commit());
	}
	CATCHZOKPPERR
	return ok;
}

SLAPI PPObjTSession::SelectBySerialParam::SelectBySerialParam(PPID sessID, const char * pSerial)
{
	THISZERO();
	InTSesID = sessID;
	STRNSCPY(Serial, pSerial);
}


int SLAPI TSessionCore::SearchSerial(const char * pSerial, PPID sessID, int sign, long flags, TSessLineTbl::Rec * pRec)
{
	int    ok = -1;
	TSessLineTbl::Rec line_rec, temp_line_rec;
	MEMSZERO(line_rec);
	LDATETIME dtm = ZERODATETIME;
	long   hdl_ln_enum = -1;
	InitLineEnumBySerial(pSerial, sign, &hdl_ln_enum);
	while(NextLineEnum(hdl_ln_enum, &temp_line_rec) > 0)
		if((!(flags & sserRest) || temp_line_rec.Flags & TSESLF_REST) && (!sessID || temp_line_rec.TSessID == sessID)) {
			if(flags & sserLast) {
				if(cmp(dtm, temp_line_rec.Dt, temp_line_rec.Tm) < 0) {
					line_rec = temp_line_rec;
					dtm.Set(line_rec.Dt, line_rec.Tm);
					ok = 1;
				}
			}
			else {
				line_rec = temp_line_rec;
				//
				// Коль скоро не требуется искать самую последнюю строку, то нас устроит
				// первая встречная - уходим!
				//
				ok = 1;
				break;
			}
		}
	DestroyIter(hdl_ln_enum);
	ASSIGN_PTR(pRec, line_rec);
	return ok;
}

int SLAPI PPObjTSession::SelectBySerial(SelectBySerialParam * pParam)
{
	int    ok = -1, r = 0;
	PPObjBill * p_bobj = BillObj;
	TSessLineTbl::Rec line_rec;
	PPIDArray lot_list;
	PPID   loc_id = 0;

	if(pParam->InTSesID) {
		TSessionTbl::Rec tses_rec;
		ProcessorTbl::Rec prc_rec;
		THROW(Search(pParam->InTSesID, &tses_rec) > 0);
		THROW(PrcObj.GetRecWithInheritance(tses_rec.PrcID, &prc_rec) > 0);
		loc_id = prc_rec.LocID;
	}
	else
		loc_id = pParam->LocID;
	if(p_bobj->SearchLotsBySerial(pParam->Serial, &lot_list) > 0) {
		PPID   lot_id = 0;
		ReceiptTbl::Rec lot_rec;
		if((r = p_bobj->SelectLotFromSerialList(&lot_list, loc_id, &lot_id, &lot_rec)) > 0) {
			pParam->CodeType = 2;
			pParam->GoodsID  = labs(lot_rec.GoodsID);
			pParam->LotID    = lot_id;
			pParam->Qtty     = lot_rec.Rest;
			ok = 1;
		}
		else if(oneof2(r, -2, -3)) {
			//
			// В случае, если лот находится на другом складе или его остаток равен нулю,
			// инициализируем возвращаемую струкутуру, однако код возврата меньше нуля.
			// Вызывающая функция должна сама решить, что с этим делать.
			//
			pParam->CodeType = 2;
			pParam->GoodsID  = labs(lot_rec.GoodsID);
			pParam->LotID    = 0;
			pParam->Qtty     = lot_rec.Quantity;
			PPSetError(PPERR_LOTBYSERIALCLOSED, pParam->Serial);
			ok = -2;
		}
	}
	else {
		//
		// Сначала пытаемся найти последний остаток серийного номера
		//
		if(P_Tbl->SearchSerial(pParam->Serial, 0, 0, TSessionCore::sserLast | TSessionCore::sserRest, &line_rec) > 0) {
			pParam->CodeType = 3;
			pParam->OutTSesID = line_rec.TSessID;
			pParam->GoodsID = line_rec.GoodsID;
			pParam->LotID = 0;
			pParam->Qtty = fabs(line_rec.Qtty);
			ok = 1;
		}
		else if(P_Tbl->SearchSerial(pParam->Serial, 0, +1, TSessionCore::sserLast, &line_rec) > 0) {
			//
			// Если остаток не найден, то ищем полную партию по указанному серийному номеру.
			// Строка должна иметь знак ПЛЮС.
			//
			pParam->CodeType = 3;
			pParam->OutTSesID = line_rec.TSessID;
			pParam->GoodsID = line_rec.GoodsID;
			pParam->LotID = 0;
			pParam->Qtty = fabs(line_rec.Qtty);
			ok = 1;
		}
	}
	if(ok > 0) {
		//
		// Проверяем, не было ли в течении этой сессии (InTSessID) уже расхода по этому серийному номеру.
		// Если был, то возвращаем 2: вызывающая функция сама должна решить, что делать в этой ситуации.
		//
		if(P_Tbl->SearchSerial(pParam->Serial, pParam->InTSesID, -1, 0, &line_rec) > 0) {
			PPSetError(PPERR_SERIALUSED, pParam->Serial);
			ok = 2;
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjTSession::UndoWritingOff(PPID sessID, int use_ta)
{
	int    ok = -1;
	PPObjBill * p_bobj = BillObj;
	TSessionTbl::Rec rec;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(Search(sessID, &rec) > 0 && rec.Flags & TSESF_WRITEDOFF) {
			PPID   bill_id = 0;
			while(p_bobj->P_Tbl->EnumMembersOfPool(PPASS_TSESSBILLPOOL, sessID, &bill_id) > 0) {
				BillTbl::Rec bill_rec;
				if(p_bobj->Search(bill_id, &bill_rec) > 0 && bill_rec.Flags & BILLF_TSESSWROFF)
					THROW(p_bobj->RemovePacket(bill_id, 0));
			}
			rec.Flags &= ~TSESF_WRITEDOFF;
			rec.Incomplete = 5;
			THROW(PutRec(&sessID, &rec, 0));
			if(rec.LinkBillID) {
				//
				// Если сессия привязана к драфт-документу и вид операции списания //
				// этого документа совпадает с видом операции списания по процессору,
				// то откатываем также cписание драфт-документа
				//
				BillTbl::Rec d_rec;
				ProcessorTbl::Rec prc_rec;
				if(p_bobj->Search(rec.LinkBillID, &d_rec) > 0 && IsDraftOp(d_rec.OpID)) {
					PPObjOprKind op_obj;
					PPDraftOpEx doe;
					if(GetPrc(rec.PrcID, &prc_rec, 1, 1) > 0)
						if(op_obj.GetDraftExData(d_rec.OpID, &doe) > 0 && doe.WrOffOpID == prc_rec.WrOffOpID)
							THROW(p_bobj->RollbackWrOffDraft(rec.LinkBillID, 0));
				}
			}
			ok = 1;
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjTSession::SnapshotRest(PPID sessID, PPLogger & rLogger, int use_ta)
{
	int    ok = 1;
	TSessionTbl::Rec tses_rec;
	if(Search(sessID, &tses_rec) > 0) {
		ProcessorTbl::Rec prc_rec;
		if(PrcObj.Search(tses_rec.PrcID, &prc_rec) > 0 && prc_rec.Flags & PRCF_STOREGOODSREST) {
			{
				SString msg_buf, ses_name;
				MakeName(&tses_rec, ses_name);
				PPLoadText(PPTXT_TSESRESTSNAPSHOOTING, msg_buf);
				PPWaitMsg(msg_buf);
				rLogger.Log(msg_buf.CatDiv(':', 2).Cat(ses_name));
			}
			LAssocArray goods_list;
			{
				TSessLineTbl * p_lt = &P_Tbl->Lines;
				TSessLineTbl::Key0 k0;
				BExtQuery q(p_lt, 0);
				q.select(p_lt->GoodsID, p_lt->OprNo, 0L).where(p_lt->TSessID == sessID);
				MEMSZERO(k0);
				k0.TSessID = sessID;
				for(q.initIteration(0, &k0, spGe); q.nextIteration() > 0;)
					if(p_lt->data.Flags & TSESLF_OUTREST)
						goods_list.Add(p_lt->data.GoodsID, p_lt->data.OprNo, 0);
			}
			PPViewGoodsRest v_gr;
			GoodsRestFilt f_gr;
			GoodsRestViewItem i_gr;
			f_gr.Date = tses_rec.FinDt;
			f_gr.GoodsGrpID = prc_rec.RestAltGrpID;
			f_gr.LocList.Add(prc_rec.LocID);
			THROW(v_gr.Init_(&f_gr));
			{
				PPTransaction tra(use_ta);
				THROW(tra);
				for(v_gr.InitIteration(); v_gr.NextIteration(&i_gr) > 0;) {
					TSessLineTbl::Rec line_rec;
					long   oprno = 0;
					if(goods_list.Search(i_gr.GoodsID, &oprno, 0) && P_Tbl->SearchLine(sessID, oprno, &line_rec) > 0) {
						if(!(line_rec.Flags & TSESLF_FIXEDREST)) {
							line_rec.Qtty = i_gr.Rest;
							THROW(PutLine(sessID, &oprno, &line_rec, 0));
						}
					}
					else {
						MEMSZERO(line_rec);
						oprno = 0;
						line_rec.TSessID = sessID;
						line_rec.GoodsID = i_gr.GoodsID;
						line_rec.Qtty    = i_gr.Rest;
						line_rec.Flags  |= TSESLF_OUTREST;
						line_rec.Sign    = 0;
						getcurdatetime(&line_rec.Dt, &line_rec.Tm);
						P_Tbl->AdjustLineTime(&line_rec);
						THROW(PutLine(sessID, &oprno, &line_rec, 0));
					}
				}
				THROW(tra.Commit());
			}
		}
	}
	CATCHZOK
	return ok;
}

SString & SLAPI PPObjTSession::MakeListName(const PPIDArray * pList, SString & rBuf)
{
	rBuf.Z();
	if(pList) {
		TSessionTbl::Rec tses_rec;
		SString ses_name;
		for(uint i = 0; i < pList->getCount(); i++)
			if(Search(pList->at(i), &tses_rec) > 0) {
				MakeName(&tses_rec, ses_name);
				rBuf.CatDivIfNotEmpty(';', 2).Cat(ses_name);
			}
	}
	return rBuf;
}

int SLAPI PPObjTSession::LoadExistedDeficitBills(PPID sessID, TSCollection <PPBillPacket> * pList, PPLogger & rLogger)
{
	int    ok = 1, r;
	PPObjBill * p_bobj = BillObj;
	PPBillPacket * p_pack = 0;
	SString bill_code;
	for(PPID bill_id = 0; p_bobj->EnumMembersOfPool(PPASS_TSDBILLPOOL, sessID, &bill_id) > 0;) {
		THROW_MEM(p_pack = new PPBillPacket);
		if((r = p_bobj->ExtractPacket(bill_id, p_pack)) > 0) {
			PPObjBill::MakeCodeString(&p_pack->Rec, 1, bill_code);
			if(p_pack->IsDraft()) {
				if(p_pack->Rec.Flags & BILLF_WRITEDOFF) {
					rLogger.LogMsgCode(mfInfo, PPINF_WRDOFFBILLINDFCTPOOL, bill_code);
					ok = -1;
				}
				THROW(p_pack->RemoveRows(0, 0));
				THROW_SL(pList->insert(p_pack));
			}
			else {
				rLogger.LogMsgCode(mfInfo, PPINF_NOTDRAFTINDFCTPOOL, bill_code);
				ZDELETE(p_pack);
			}
		}
		THROW(r);
	}
	CATCH
		ok = 0;
		pList->freeAll();
	ENDCATCH
	return ok;
}

int SLAPI PPObjTSession::ConvertWrOffDeficit(PPID sessID, PPID locID, const PUGL * pDfctList, PPLogger & rLogger)
{
	int    ok = -1, r;
	PPObjBill * p_bobj = BillObj;
	PPEquipConfig eq_cfg;
	ReadEquipConfig(&eq_cfg);
	if(eq_cfg.OpOnDfctThisLoc || eq_cfg.OpOnDfctOthrLoc) {
		TSCollection <PPBillPacket> pack_list;
		THROW_PP(!eq_cfg.OpOnDfctThisLoc || IsDraftOp(eq_cfg.OpOnDfctThisLoc), PPERR_INVCSESSDFCTOP);
		THROW_PP(!eq_cfg.OpOnDfctOthrLoc || IsDraftOp(eq_cfg.OpOnDfctOthrLoc), PPERR_INVCSESSDFCTOP);
		THROW(r = LoadExistedDeficitBills(sessID, &pack_list, rLogger));
		if(r > 0) {
			const  LDATE save_oper_date = LConfig.OperDate;
			uint   i = 0, j;
			PUGI * p_pugi;
			StrAssocArray goods_name_list;
			GoodsToObjAssoc g2la(PPASS_GOODS2LOC, PPOBJ_LOCATION);
			THROW(g2la.Load());
			for(i = 0; pDfctList->enumItems(&i, (void **)&p_pugi);)
				if(!goods_name_list.Search(p_pugi->GoodsID)) {
					Goods2Tbl::Rec goods_rec;
					if(GObj.Fetch(p_pugi->GoodsID, &goods_rec) > 0)
						THROW_SL(goods_name_list.Add(p_pugi->GoodsID, goods_rec.Name));
				}
			goods_name_list.SortByText();
			for(i = 0; i < goods_name_list.getCount(); i++) {
				PUGI   item;
				uint   pos = 0;
				PPID   goods_id = goods_name_list.Get(i).Id;
				if(pDfctList->SearchGoods(goods_id, &pos, &item)) {
					int    is_other_loc = 0;
					PPID   op_id = 0;
					PPID   loc_id = 0;
					PPBillPacket * p_pack = 0;
					int    r = g2la.Get(goods_id, &loc_id);
					THROW(r);
					if(r < 0 || loc_id == 0)
						loc_id = locID;
					if(loc_id != locID) {
						op_id = eq_cfg.OpOnDfctOthrLoc;
						is_other_loc = 1;
					}
					else
						op_id = eq_cfg.OpOnDfctThisLoc;
					if(op_id) {
						ReceiptTbl::Rec lot_rec;
						PPTransferItem ti;
						PPOprKind op_rec;
						for(j = 0; !p_pack && j < pack_list.getCount(); j++) {
							PPBillPacket * p = pack_list.at(j);
							if(p && p->Rec.LocID == loc_id)
								p_pack = p;
						}
						if(!p_pack) {
							THROW_MEM(p_pack = new PPBillPacket);
							THROW(p_pack->CreateBlank2(op_id, pDfctList->Dt, loc_id, 0));
							PPGetWord(PPWORD_AT_AUTO, 0, p_pack->Rec.Memo, sizeof(p_pack->Rec.Memo));
							p_pack->SetPoolMembership(PPBillPacket::bpkTSessDfct, sessID);
							THROW_SL(pack_list.insert(p_pack));
						}
						if(GetOpData(p_pack->Rec.OpID, &op_rec) > 0) {
							if(op_rec.AccSheetID == LConfig.LocAccSheetID)
								p_pack->Rec.Object = PPObjLocation::WarehouseToObj(locID);
							if(op_rec.AccSheet2ID == LConfig.LocAccSheetID)
								p_pack->Rec.Object2 = PPObjLocation::WarehouseToObj(locID);
						}
						THROW(ti.Init(&p_pack->Rec));
						ti.GoodsID  = goods_id;
						ti.Cost     = TR5(item.Cost);
						ti.Price    = TR5(item.Price);
						ti.Quantity_ = item.DeficitQty;
						ti.SetupSign(p_pack->Rec.OpID);
						DS.SetOperDate(pDfctList->Dt);
						if(::GetCurGoodsPrice(goods_id, loc_id, GPRET_MOSTRECENT, 0, &lot_rec) > 0) {
							ti.UnitPerPack = lot_rec.UnitPerPack;
							ti.Cost = TR5(lot_rec.Cost);
							if(ti.Price == 0)
								ti.Price = TR5(lot_rec.Price);
						}
						DS.SetOperDate(save_oper_date);
						THROW(p_pack->InsertRow(&ti, 0));
					}
				}
			}
			for(i = 0; i < pack_list.getCount(); i++) {
				PPBillPacket * p_pack = pack_list.at(i);
				if(p_pack) {
					THROW(p_pack->InitAmounts(0));
					if(p_pack->Rec.ID) {
						THROW(p_bobj->UpdatePacket(p_pack, 0));
						rLogger.LogAcceptMsg(PPOBJ_BILL, p_pack->Rec.ID, 1);
					}
					else {
						THROW(p_bobj->TurnPacket(p_pack, 0));
						rLogger.LogAcceptMsg(PPOBJ_BILL, p_pack->Rec.ID, 0);
					}
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjTSession::CalcBalance(PPID sessID, double * pDebt, double * pBillPaym, double * pCcPaym)
{
	int    ok = 1;
	PPObjBill * p_bobj = BillObj;
	double debt = 0.0;
	double bill_paym = 0.0;
	double cc_paym = 0.0;
	TSessionTbl::Rec rec;
	if(sessID && Search(sessID, &rec) > 0) {
		PPIDArray bill_list;
		p_bobj->P_Tbl->GetPoolMembersList(PPASS_TSESSBILLPOOL, sessID, &bill_list);
		for(uint i = 0; i < bill_list.getCount(); i++) {
			const PPID bill_id = bill_list.get(i);
			BillTbl::Rec bill_rec;
			if(p_bobj->Fetch(bill_id, &bill_rec) > 0) {
				if(bill_rec.Flags2 & BILLF2_TSESSPAYM) {
					bill_paym += bill_rec.Amount;
				}
			}
		}
		debt += rec.Amount;
		if(rec.CCheckID_) {
			CCheckTbl::Rec cc_rec;
			if(ScObj.P_CcTbl->Search(rec.CCheckID_, &cc_rec) > 0) {
				cc_paym += MONEYTOLDBL(cc_rec.Amount);
			}
		}
	}
	else
		ok = -1;
	ASSIGN_PTR(pDebt, debt);
	ASSIGN_PTR(pBillPaym, bill_paym);
	ASSIGN_PTR(pCcPaym, cc_paym);
	return ok;
}

int SLAPI PPObjTSession::WriteOff(const PPIDArray * pSessList, PUGL * pDfctList, int use_ta)
{
	int    ok = 1, r;
	uint   i;
	SString msg_buf, ses_list_buf;
	PPLogger logger;
	PPIDArray sess_list, arranged_sess_list;
	PPIDArray fixrest_sess_list;
	TSessWrOffOrder woo;
	TSessionTbl::Rec tses_rec;
	PPWait(1);
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		for(i = 0; i < pSessList->getCount(); i++) {
			const PPID sess_id = pSessList->at(i);
			if(Search(sess_id, &tses_rec) > 0) {
				ProcessorTbl::Rec prc_rec;
				if(tses_rec.Flags & TSESF_SUPERSESS) {
					THROW(P_Tbl->GetChildIDList(sess_id, 0, &sess_list));
				}
				else {
					THROW(sess_list.addUnique(sess_id));
				}
				if(PrcObj.Search(tses_rec.PrcID, &prc_rec) > 0 && prc_rec.Flags & PRCF_STOREGOODSREST)
					THROW(fixrest_sess_list.addUnique(sess_id));
			}
		}
		if(sess_list.getCount()) {
			PPLoadText(PPTXT_TSESSWRITINGOFF, msg_buf);
			PPWaitMsg(msg_buf);
			logger.Log(msg_buf.CatDiv(':', 2).Cat(MakeListName(&sess_list, ses_list_buf)));
		}
		GetWrOffOrder(&woo);
		THROW(r = woo.ArrangeTSessList(&sess_list, &arranged_sess_list));
		if(r > 0) {
			PPLoadText(PPTXT_TSESSLISTARRANGED, msg_buf);
			logger.Log(msg_buf.CatDiv(':', 2).Cat(MakeListName(&arranged_sess_list, ses_list_buf)));
		}
		for(i = 0; i < arranged_sess_list.getCount(); i++) {
			PUGL   local_pugl;
			THROW(Helper_WriteOff(arranged_sess_list.at(i), &local_pugl, logger, 0));
			THROW(pDfctList->Add__(&local_pugl));
		}
		for(i = 0; i < fixrest_sess_list.getCount(); i++) {
			THROW(SnapshotRest(fixrest_sess_list.at(i), logger, 0));
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	PPWait(0);
	return ok;
}

int SLAPI PPObjTSession::GetWrOffAttrib(const TSessionTbl::Rec * pRec, WrOffAttrib * pAttr)
{
	int    ok = 1;
	PPID   agent_acs_id = 0;
	PPObjArticle ar_obj;
	ProcessorTbl::Rec prc_rec;
	PPOprKind op_rec;
	memzero(pAttr, sizeof(*pAttr));
	pAttr->ArID = pRec->ArID;
	THROW(PrcObj.GetRecWithInheritance(pRec->PrcID, &prc_rec, 1) > 0);
	THROW(GetOpData(prc_rec.WrOffOpID, &op_rec) > 0);
	if(!op_rec.AccSheet2ID && pRec->Ar2ID && prc_rec.Flags & PRCF_ADDEDOBJASAGENT) {
		agent_acs_id = NZOR(agent_acs_id, GetAgentAccSheet());
		ArticleTbl::Rec ar2_rec;
		if(ar_obj.Fetch(pRec->Ar2ID, &ar2_rec) > 0 && ar2_rec.AccSheetID == agent_acs_id)
			pAttr->AgentID = pRec->Ar2ID;
	}
	else
		pAttr->Ar2ID = pRec->Ar2ID;
	//
	// Если в документе поле агента не заполнено и процессор связан с персоналией,
	// являющейся агентом, то привязываем соответствующую ей статью как агента по документу
	//
	if(!pAttr->AgentID && prc_rec.LinkObjType == PPOBJ_PERSON && prc_rec.LinkObjID) {
		agent_acs_id = NZOR(agent_acs_id, GetAgentAccSheet());
		PPID   ar_id = 0;
		if(ar_obj.GetByPerson(agent_acs_id, prc_rec.LinkObjID, &ar_id) > 0)
			pAttr->AgentID = ar_id;
	}
	pAttr->SCardID = pRec->SCardID;
	CATCHZOK
	return ok;
}

int SLAPI PPObjTSession::Helper_WriteOff(PPID sessID, PUGL * pDfctList, PPLogger & rLogger, int use_ta)
{
	struct RecomplItem {
		explicit RecomplItem(int t) : IsRecompl(t), Count(0), Qtty(0.0), WtQtty(0.0)
		{
		}
		operator int() const { return IsRecompl; }
		int    IsRecompl;
		uint   Count;
		double Qtty;
		double WtQtty;
		SString Serial;
	};
	int    ok = -1;
	int    incomplete = 0;
	long   hdl_ln_enum = -1;
	PUGL   local_pugl;
	PPBillPacket * p_link_bill_pack = 0;
	TSessionTbl::Rec tses_rec;
	ProcessorTbl::Rec prc_rec;
	ReceiptTbl::Rec lot_rec;
	PPObjGoodsStruc gs_obj;
	PPObjArticle ar_obj;
	PPGoodsStruc gs;
	SString ses_name, fmt_buf, msg_buf;
	PPObjBill * p_bobj = BillObj;
	ReceiptCore & rcpt_core = p_bobj->trfr->Rcpt;

	PPLoadText(PPTXT_LOG_TSES_WROFF_LINE, fmt_buf);
	THROW(Search(sessID, &tses_rec) > 0);
	MakeName(&tses_rec, ses_name);
	THROW(PrcObj.GetRecWithInheritance(tses_rec.PrcID, &prc_rec, 1) > 0);
	if(tses_rec.Flags & TSESF_WRITEDOFF)
		rLogger.LogString(PPTXT_TSESSWRITEDOFF, ses_name);
	else if(prc_rec.Flags & PRCF_LOCKWROFF)
		rLogger.LogString(PPTXT_PRCWROFFLOCKED, prc_rec.Name);
	else {
		PPID   op_type_id = GetOpType(prc_rec.WrOffOpID);
		PPID   tec_goods_id = 0;
		PPID   tec_struc_id = 0;
		double tec_goods_qtty = 0.0;
		double order_price = 0.0;
		WrOffAttrib attrib;
		TechTbl::Rec tec_rec;
		RAssocArray price_list; // Кэш цен реализации (ассоциация {GoodsID, Price})
		RAssocArray pack_list;  // Кэш емкостей упаковки (ассоциация {GoodsID, UnitPerPack})
		if(TecObj.Search(tses_rec.TechID, &tec_rec) > 0) {
			tec_goods_id = tec_rec.GoodsID;
			tec_struc_id = tec_rec.GStrucID;
			if(rcpt_core.Search(tses_rec.OrderLotID, &lot_rec) > 0)
				order_price = lot_rec.Price;
		}
		else
			MEMSZERO(tec_rec);

		PPIDArray goods_id_list;
		long   oprno = 0;
		TSessLineTbl::Rec line_rec;
		PPBillPacket bill_pack;
		LDATE  wr_off_dt = ZERODATE;
		RecomplItem recompl_item(tec_rec.Flags & TECF_RECOMPLMAINGOODS && op_type_id == PPOPT_GOODSMODIF);

		THROW_PP(tses_rec.Status == TSESST_CLOSED, PPERR_WROFFUNCLOSEDTSESS);
		{
			PPTransaction tra(use_ta);
			THROW(tra);
			if(prc_rec.Flags & PRCF_WROFFDT_BYSUPER && tses_rec.ParentID) {
				TSessionTbl::Rec super_rec;
				THROW(Search(tses_rec.ParentID, &super_rec) > 0);
				wr_off_dt = (prc_rec.Flags & PRCF_WROFFDT_START) ? super_rec.StDt : super_rec.FinDt;
			}
			SETIFZ(wr_off_dt, (prc_rec.Flags & PRCF_WROFFDT_START) ? tses_rec.StDt : tses_rec.FinDt);
			//
			if(tses_rec.LinkBillID) {
				THROW_MEM(p_link_bill_pack = new PPBillPacket);
				if(p_bobj->ExtractPacket(tses_rec.LinkBillID, p_link_bill_pack) <= 0 || !IsDraftOp(p_link_bill_pack->Rec.OpID))
					ZDELETE(p_link_bill_pack);
			}
			THROW(GetWrOffAttrib(&tses_rec, &attrib));
			THROW(bill_pack.CreateBlank2(prc_rec.WrOffOpID, wr_off_dt, prc_rec.LocID, 0));
			bill_pack.Rec.Object = attrib.ArID;
			bill_pack.Rec.Object2 = attrib.Ar2ID;
			bill_pack.Ext.AgentID = attrib.AgentID;
			bill_pack.Rec.SCardID  = attrib.SCardID;
			bill_pack.SetPoolMembership(PPBillPacket::bpkTSess, sessID);
			if(p_link_bill_pack) {
				if(p_link_bill_pack->P_Freight)
					bill_pack.SetFreight(p_link_bill_pack->P_Freight);
				bill_pack.Ext.AgentID = p_link_bill_pack->Ext.AgentID;
				LDATE pay_date = ZERODATE;
				if(p_link_bill_pack->GetLastPayDate(&pay_date) > 0)
					bill_pack.SetPayDate(pay_date, 0);
			}
			{
				PPIDArray sess_id_list;
				THROW(P_Tbl->GetChildIDList(sessID, TSessionCore::gclfSubSess|TSessionCore::gclfRecursive, &sess_id_list));
				sess_id_list.addUnique(sessID);
				for(uint sc = 0; sc < sess_id_list.getCount(); sc++) {
					const PPID sess_id = sess_id_list.get(sc);
					THROW(P_Tbl->InitLineEnum(sess_id, &hdl_ln_enum));
					for(oprno = 0; P_Tbl->NextLineEnum(hdl_ln_enum, &line_rec) > 0;) {
						LongArray rows;
						ILTI   ilti;
						int    is_last_lot = 0;
						goods_id_list.addUnique(line_rec.GoodsID);
						if(CConfig.Flags & CCFLG_DEBUG) {
							PPFormat(fmt_buf, &msg_buf, line_rec.GoodsID, line_rec.Serial, line_rec.Qtty);
							rLogger.Log(msg_buf);
						}
						if(line_rec.GoodsID == tec_goods_id) {
							tec_goods_qtty = faddwsign(tec_goods_qtty, line_rec.Qtty, line_rec.Sign);
							if(line_rec.Sign > 0 && recompl_item) {
								THROW_PP(strip(line_rec.Serial)[0], PPERR_TSESRECOMPLNOSER);
								if(recompl_item.Serial.Empty())
									recompl_item.Serial = line_rec.Serial;
								else
									THROW_PP(recompl_item.Serial.Cmp(line_rec.Serial, 0) == 0, PPERR_TSESRECOMPLDIFSER);
								recompl_item.Count++;
								recompl_item.Qtty   += line_rec.Qtty;
								recompl_item.WtQtty += line_rec.WtQtty;
								continue;
							}
						}
						//
						if(line_rec.Sign == 0)
							continue;
						else if(op_type_id != PPOPT_GOODSMODIF)
							if(line_rec.Sign > 0 && op_type_id != PPOPT_GOODSRECEIPT)
								continue;
							else if(line_rec.Sign < 0 && op_type_id != PPOPT_GOODSEXPEND)
								continue;
						ilti.Setup(line_rec.GoodsID, line_rec.Sign, line_rec.Qtty, 0, 0);
						if(checkdate(line_rec.Expiry))
							ilti.Expiry = line_rec.Expiry;
						if(line_rec.Sign > 0)
							ilti.Flags |= PPTFR_RECEIPT;
						else if(line_rec.Sign < 0) {
							double rest_qtty = 0.0;
							if(strip(line_rec.Serial)[0]) {
								TSessLineTbl::Rec rest_rec;
								if(P_Tbl->SearchSerial(line_rec.Serial, line_rec.TSessID, 0, TSessionCore::sserRest, &rest_rec) > 0)
									rest_qtty = fabs(rest_rec.Qtty);
							}
							ilti.SetQtty(-(fabs(line_rec.Qtty) - rest_qtty));
						}
						{
							//
							// Определяем цены
							//
							double cost = 0.0, price = 0.0;
							double lot_cost = 0.0, lot_price = 0.0;
							double lot_pack = 0.0;
							uint   link_pos = 0;
							if(line_rec.Sign > 0) {
								if(rcpt_core.GetLastLot(ilti.GoodsID, -bill_pack.Rec.LocID, bill_pack.Rec.Dt, &lot_rec) > 0)
									is_last_lot = 1;
							}
							if(p_link_bill_pack && p_link_bill_pack->SearchGoods(ilti.GoodsID, &link_pos) > 0) {
								const PPTransferItem & ti = p_link_bill_pack->ConstTI(link_pos);
								cost  = ti.Cost;
								price = ti.NetPrice();
								if(line_rec.Sign > 0) {
									if(ti.UnitPerPack > 0 && ilti.UnitPerPack == 0)
										ilti.UnitPerPack = ti.UnitPerPack;
									SETIFZ(ilti.QCert, ti.QCert);
								}
							}
							else if(line_rec.Sign < 0)
								price = R5(fabs(line_rec.Price) - line_rec.Discount);
							else if(line_rec.Sign > 0)
								cost  = R5(fabs(line_rec.Price) - line_rec.Discount);
							// Для ускорения процедуры, определив цену, сохраняем ее в списке,
							// для того, чтобы при следующей встрече с этим товаром, просто извлечь цену из списка.
							if(price == 0 && price_list.Search(ilti.GoodsID, &price, 0) <= 0) {
								if(ilti.GoodsID == tec_goods_id && order_price > 0)
									price = order_price;
								else {
									const QuotIdent qi(bill_pack.Rec.LocID, PPQUOTK_BASE);
									GObj.GetQuotExt(ilti.GoodsID, qi, &price, 1);
								}
								price_list.Add(ilti.GoodsID, price, 0);
							}
							ilti.Cost  = cost;
							ilti.Price = price;
						}
						if(line_rec.Sign > 0 && ilti.UnitPerPack == 0) {
							//
							// Определяем емкость упаковки для исходящих позиций (если связанный документ
							//   не снабдил нас такой информацией)
							//
							double package = 0;
							GoodsStockExt gse;
							if(pack_list.Search(ilti.GoodsID, &package, 0) > 0)
								ilti.UnitPerPack = package;
							else if(GObj.GetStockExt(ilti.GoodsID, &gse) > 0 && gse.Package) {
								pack_list.Add(ilti.GoodsID, gse.Package, 0);
								ilti.UnitPerPack = gse.Package;
							}
						}
						if(is_last_lot) {
							if(ilti.Cost <= 0.0)
								ilti.Cost = R5(lot_rec.Cost);
							if(ilti.Price <= 0.0)
								ilti.Price = R5(lot_rec.Price);
							if(ilti.UnitPerPack <= 0.0)
								ilti.UnitPerPack = lot_rec.UnitPerPack;
						}
						THROW(p_bobj->ConvertILTI(&ilti, &bill_pack, &rows, CILTIF_USESUBST|CILTIF_SUBSTSERIAL, strip(line_rec.Serial)));
						// @v9.4.9 if(R6(ilti.Rest) != 0) {
						if(ilti.HasDeficit()) { // @v9.4.9
  							THROW(local_pugl.Add(&ilti, bill_pack.Rec.LocID, (uint)oprno, bill_pack.Rec.Dt));
  							incomplete = 1;
						}
						else if(rows.getCount() && line_rec.Flags & TSESLF_INDEPPHQTTY) {
							PPTransferItem & r_ti = bill_pack.TI(rows.at(0));
							r_ti.WtQtty = faddwsign(r_ti.WtQtty, line_rec.WtQtty, line_rec.Sign);
						}
					}
					P_Tbl->DestroyIter(hdl_ln_enum);
					hdl_ln_enum = -1;
				}
			}
			if(recompl_item && recompl_item.Count) {
				LongArray rows;
				PPTransferItem ti(&bill_pack.Rec, TISIGN_PLUS);
				PPIDArray lot_list;
				THROW(ti.SetupGoods(tec_goods_id));
				ti.Flags |= (PPTFR_PLUS|PPTFR_MODIF|PPTFR_REVAL);
				ti.Flags &= ~PPTFR_RECEIPT;
				if(ti.Flags & PPTFR_INDEPPHQTTY)
					ti.WtQtty = (float)R6(recompl_item.WtQtty);
				if(p_bobj->SearchLotsBySerial(recompl_item.Serial, &lot_list) > 0) {
					for(uint i = 0; !ti.LotID && i < lot_list.getCount(); i++)
						if(rcpt_core.Search(lot_list.at(i), &lot_rec) > 0)
							if(lot_rec.GoodsID == tec_goods_id && lot_rec.LocID == bill_pack.Rec.LocID && lot_rec.Rest > 0)
								THROW(ti.SetupLot(lot_rec.ID, &lot_rec, 0));
					if(ti.LotID)
						THROW(bill_pack.InsertRow(&ti, &rows, 0));
				}
				if(!ti.LotID)
					rLogger.LogMsgCode(mfError, PPERR_INADEQSERIAL, recompl_item.Serial);
			}
			//
			// Списание автоматических рассчитываемых компонентов
			//
			if(op_type_id == PPOPT_GOODSMODIF && tec_goods_id && tec_struc_id && tec_goods_qtty) {
				uint   gs_pos = 0;
				PPGoodsStrucItem gs_item;
				int    r;
				double qtty = 0.0;
				THROW(gs_obj.Get(tec_struc_id, &gs));
				while((r = gs.EnumItemsExt(&gs_pos, &gs_item, tec_goods_id, tec_goods_qtty, &qtty)) > 0)
					if(gs_item.Flags & GSIF_AUTOTSWROFF && !goods_id_list.lsearch(gs_item.GoodsID)) {
						qtty = -qtty;
						LongArray rows;
						ILTI   ilti;
						ilti.GoodsID = gs_item.GoodsID;
						ilti.SetQtty(qtty, 0, (qtty > 0) ? (PPTFR_PLUS | PPTFR_RECEIPT) : PPTFR_MINUS);
						THROW(p_bobj->ConvertILTI(&ilti, &bill_pack, &rows, CILTIF_USESUBST, 0));
						// @v9.4.9 if(R6(ilti.Rest) != 0) {
						if(ilti.HasDeficit()) { // @v9.4.9
							THROW(local_pugl.Add(&ilti, bill_pack.Rec.LocID, 0, bill_pack.Rec.Dt));
							incomplete = 1;
						}
					}
				THROW(r);
			}
			local_pugl.Log(&rLogger);
			if(!incomplete || prc_rec.Flags & PRCF_TURNINCOMPLBILL) {
				if(incomplete)
					//
					// Информируем оператора о том, что сессия списана несмотря на дефицит
					//
					rLogger.LogString(PPTXT_TSESWROFFDEFICIT, ses_name);
				if(bill_pack.OpTypeID == PPOPT_GOODSMODIF)
					bill_pack.CalcModifCost();
				if(p_link_bill_pack && p_link_bill_pack->IsDraft()) {
					//
					// Если сессия привязана к драфт-документу и вид операции списания //
					// этого документа совпадает с видом операции списания по процессору,
					// то документ списания сессии одновременно становится документом
					// списания драфт-документа
					//
					BillTbl::Rec d_rec, link_rec;
					if(p_bobj->Search(tses_rec.LinkBillID, &d_rec) > 0) {
						PPObjOprKind op_obj;
						PPDraftOpEx doe;
						if(op_obj.GetDraftExData(d_rec.OpID, &doe) > 0 && doe.WrOffOpID == prc_rec.WrOffOpID &&
							!(d_rec.Flags & BILLF_WRITEDOFF)) {
							if(p_bobj->Search(d_rec.LinkBillID, &link_rec) > 0 && GetOpType(link_rec.OpID) == PPOPT_GOODSORDER) {
								if(CheckOpFlags(bill_pack.Rec.OpID, OPKF_ONORDER)) {
									PPBillPacket ord_pack;
									THROW(p_bobj->ExtractPacket(link_rec.ID, &ord_pack) > 0); // @v5.3.0 LinkBillID-->ID
									THROW(bill_pack.AttachToOrder(&ord_pack));
								}
							}
							bill_pack.Rec.LinkBillID = tses_rec.LinkBillID;
							d_rec.Flags |= BILLF_WRITEDOFF;
							THROW(p_bobj->P_Tbl->EditRec(&tses_rec.LinkBillID, &d_rec, 0));
						}
					}
				}
				else if(tses_rec.OrderLotID) {
					//
					// Привязка к заказу строк списания, соответствующих заказанной позиции.
					// Привязываются к заказу только позиции, в которых товар расходуется (то есть,
					// собственно, производство продукции закрыть заказ не может, ибо для этого требуется //
					// продукцию отгрузить.
					//
					ReceiptTbl::Rec ord_lot;
					if(p_bobj->trfr->Rcpt.Search(tses_rec.OrderLotID, &ord_lot) > 0) {
						PPBillPacket ord_pack;
						if(p_bobj->ExtractPacket(ord_lot.BillID, &ord_pack) > 0)
							for(uint p2 = 0; bill_pack.SearchGoods(labs(ord_lot.GoodsID), &p2); p2++)
								if(bill_pack.ConstTI(p2).Flags & PPTFR_MINUS)
									THROW(bill_pack.AttachRowToOrder(p2, &ord_pack));
					}
				}
				if(p_bobj->SubstMemo(&bill_pack) < 0) {
					if(*strip(tses_rec.Memo))
						STRNSCPY(bill_pack.Rec.Memo, tses_rec.Memo);
					else if(p_link_bill_pack)
						STRNSCPY(bill_pack.Rec.Memo, p_link_bill_pack->Rec.Memo);
				}
				{
					int    turn_result = 0;
					THROW(turn_result = p_bobj->__TurnPacket(&bill_pack, 0, 1, 0));
					if(turn_result > 0)
						rLogger.LogAcceptMsg(PPOBJ_BILL, bill_pack.Rec.ID, 0);
				}
				THROW(Search(sessID, &tses_rec) > 0);
				tses_rec.Flags |= TSESF_WRITEDOFF;
				tses_rec.Incomplete = 0;
				THROW(PutRec(&sessID, &tses_rec, 0));
				if(tses_rec.ParentID)
					THROW(P_Tbl->UpdateSuperSessCompleteness(tses_rec.ParentID, 0));
				rLogger.LogString(PPTXT_TSESSWRITEDOFF, ses_name);
				ok = 1;
			}
			else {
				THROW(ConvertWrOffDeficit(sessID, prc_rec.LocID, &local_pugl, rLogger));
				ok = -2;
			}
			THROW(tra.Commit());
		}
	}
	CATCH
		rLogger.LogLastError();
		ok = 0;
	ENDCATCH
	if(hdl_ln_enum >= 0)
		P_Tbl->DestroyIter(hdl_ln_enum);
	CALLPTRMEMB(pDfctList, Add__(&local_pugl));
	delete p_link_bill_pack;
	return ok;
}

//static
int SLAPI PPObjTSession::PutWrOffOrder(const TSessWrOffOrder * pData, int use_ta)
{
	return PPRef->PutPropArray(PPOBJ_TSESSION, 0, TSESPRP_WROFFORDER, pData, use_ta);
}

//static
int SLAPI PPObjTSession::GetWrOffOrder(TSessWrOffOrder * pData)
{
	int    ok = -1;
	if(pData) {
		pData->freeAll();
		ok = PPRef->GetPropArray(PPOBJ_TSESSION, 0, TSESPRP_WROFFORDER, pData);
		if(ok > 0)
			pData->IsLoaded = 1;
	}
	return ok;
}

SLAPI TSessWrOffOrder::TSessWrOffOrder() : ObjRestrictArray(), IsLoaded(0)
{
}

TSessWrOffOrder & FASTCALL TSessWrOffOrder::operator = (const TSessWrOffOrder & rSrc)
{
	copy(rSrc);
	IsLoaded = rSrc.IsLoaded;
	return *this;
}

int SLAPI TSessWrOffOrder::GetPos(PPObjProcessor * pPrcObj, PPID prcID, uint * pPos) const
{
	int    ok = -1;
	uint   pos = 0;
	if(SearchItemByID(prcID, &pos)) {
		ok = 1;
	}
	else {
		PPIDArray parent_list;
		pPrcObj->GetParentsList(prcID, &parent_list);
		for(uint i = 0; ok < 0 && i < parent_list.getCount(); i++)
			if(SearchItemByID(parent_list.at(i), &(pos = 0)))
				ok = 1;
	}
	if(ok < 0)
		pos = getCount();
	ASSIGN_PTR(pPos, pos);
	return ok;
}

int SLAPI TSessWrOffOrder::CompareProcessors(PPObjProcessor * pPrcObj, PPID prc1ID, PPID prc2ID) const
{
	int    ok = 0;
	uint   pos1 = 0, pos2 = 0;
	GetPos(pPrcObj, prc1ID, &pos1);
	GetPos(pPrcObj, prc2ID, &pos2);
	if(pos1 < pos2)
		ok = -1;
	else if(pos1 > pos2)
		ok = 1;
	return ok;
}

void SLAPI TSessWrOffOrder::ShortSort(SArray * pPrcList) const
{
	if(pPrcList && pPrcList->getCount()) {
		uint   _max = 0;
		PPObjProcessor prc_obj;
		for(uint lo = 0, hi = pPrcList->getCount()-1; hi > lo; hi--) {
			_max = lo;
			for(uint p = lo+1; p <= hi; p++) {
				const ArrngItem * p_item1 = static_cast<const ArrngItem *>(pPrcList->at(p));
				const ArrngItem * p_item2 = static_cast<const ArrngItem *>(pPrcList->at(_max));
				if(p_item1->Dt > p_item2->Dt || CompareProcessors(&prc_obj, p_item1->PrcID, p_item2->PrcID) > 0)
					_max = p;
			}
			pPrcList->swap(_max, hi);
		}
	}
}

int SLAPI TSessWrOffOrder::ArrangeTSessList(const PPIDArray * pSrcList, PPIDArray * pDestList) const
{
	int    ok = -1;
	if(pSrcList && pDestList) {
		uint   i;
		PPObjTSession tses_obj;
		TSessionTbl::Rec rec;
		SArray prc_list(sizeof(ArrngItem));
		pDestList->clear();
		for(i = 0; i < pSrcList->getCount(); i++)
			if(tses_obj.Search(pSrcList->at(i), &rec) > 0) {
				if(!(rec.Flags & TSESF_SUBSESS && rec.ParentID)) {
					ArrngItem ai;
					ai.SessID = rec.ID;
					ai.PrcID  = rec.PrcID;
					ai.Dt     = rec.FinDt;
					THROW_SL(prc_list.insert(&ai));
				}
			}
		ShortSort(&prc_list);
		for(i = 0; i < prc_list.getCount(); i++)
			THROW(pDestList->add(static_cast<const ArrngItem *>(prc_list.at(i))->SessID));
		ok = pDestList->IsEqual(pSrcList) ? -1 : 1;
	}
	CATCHZOK
	return ok;
}
//
//
//
//static
int SLAPI PPObjTSession::EditWrOffOrder()
{
	class TSessWrOffOrderDialog : public ObjRestrictListDialog {
		DECL_DIALOG_DATA(TSessWrOffOrder);
	public:
		TSessWrOffOrderDialog() : ObjRestrictListDialog(DLG_TSESSWROFFORD, CTL_TSESSWROFFORD_LIST)
		{
		}
		DECL_DIALOG_SETDTS()
		{
			RVALUEPTR(Data, pData);
			setParams(PPOBJ_PROCESSOR, &Data);
			updateList(-1);
			return 1;
		}
		DECL_DIALOG_GETDTS()
		{
			ASSIGN_PTR(pData, Data);
			return 1;
		}
	private:
		//virtual int getObjName(PPID objID, long objFlags, SString &);
		//virtual int getExtText(PPID objID, long objFlags, SString &);
		virtual int editItemDialog(ObjRestrictItem * pItem)
		{
			const  int prc_ctrl_group = 1;
			int    ok = -1;
			TDialog * dlg = new TDialog(DLG_TSESWOORDI);
			if(CheckDialogPtrErr(&dlg)) {
				dlg->addGroup(prc_ctrl_group, new PrcCtrlGroup(CTLSEL_TSESWOORDI_PRC));
				PrcCtrlGroup::Rec cg_rec(pItem->ObjID);
				dlg->setGroupData(prc_ctrl_group, &cg_rec);
				for(int valid_data = 0; !valid_data && ExecView(dlg) == cmOK;) {
					dlg->getGroupData(prc_ctrl_group, &cg_rec);
					if(!cg_rec.PrcID) {
						PPError(PPERR_PRCNEEDED, 0);
					}
					else {
						pItem->ObjID = cg_rec.PrcID;
						pItem->Flags = 0;
						ok = valid_data = 1;
					}
				}
			}
			else
				ok = 0;
			delete dlg;
			return ok;
		}
	};
	int    ok = -1;
	TSessWrOffOrderDialog * dlg = new TSessWrOffOrderDialog;
	if(CheckDialogPtrErr(&dlg)) {
		TSessWrOffOrder woo;
		GetWrOffOrder(&woo);
		dlg->setDTS(&woo);
		while(ok < 0 && ExecView(dlg) == cmOK)
			if(dlg->getDTS(&woo))
				if(PutWrOffOrder(&woo, 1))
					ok = 1;
				else
					PPError();
	}
	else
		ok = 0;
	delete dlg;
	return ok;
}
//
//
//
SLAPI PPObjTSession::BhtCurSessData::BhtCurSessData()
{
	Reset();
	Ta = 0;
}

int SLAPI PPObjTSession::BhtCurSessData::Set(PPID sessID, PPID prcID, PPID arID, const LDATETIME & rDtm)
{
	TSessID = sessID;
	IsProperSess = 0;
	PrcID = prcID;
	ArID = arID;
	Dtm = rDtm;
	return 1;
}

void SLAPI PPObjTSession::BhtCurSessData::Reset()
{
	TSessID = 0;
	IsProperSess = 0;
	PrcID = 0;
	ArID = 0;
	Dtm.Z();
}
//
//
//
int SLAPI BhtTSess::Add(const TSessionTbl::Rec * pSessRec, int isProper)
{
	PSE    entry;
	uint   pos = 0;
	if(SearchBySess(pSessRec->ID, &pos)) {
		LastUsedEntryPos = pos;
		return 2;
	}
	else {
		entry.PrcID  = pSessRec->PrcID;
		entry.SessID = pSessRec->ID;
		entry.ArID   = pSessRec->ArID;
		entry.BillID = pSessRec->LinkBillID;
		entry.Flags  = isProper ? fProperSess : 0;
		if(insert(&entry)) {
			LastUsedEntryPos = getCount()-1;
			return 1;
		}
		else {
			LastUsedEntryPos = UINT_MAX;
			return PPSetErrorSLib();
		}
	}
}

int SLAPI BhtTSess::CreateSess(PPID * pSessID, const BhtTSessRec * pRec, const ProcessorTbl::Rec * pPrcRec)
{
	int    ok = 1;
	TSessionTbl::Rec ses_rec;
	PPID   super_sess_id = 0;
	PPID   par_prc_id = pPrcRec->ParentID;
	//
	// Сначала, выясняем существует ли подходящая суперсессия //
	//
	while(!super_sess_id && par_prc_id) {
		ProcessorTbl::Rec prc_rec;
		if(TSesObj.P_Tbl->SearchByPrcTime(par_prc_id, TSESK_SUPERSESS, pRec->Dtm, &ses_rec) > 0)
			super_sess_id = ses_rec.ID;
		else if(TSesObj.GetPrc(par_prc_id, &prc_rec, 0, 1) > 0)
			par_prc_id = prc_rec.ParentID;
	}
	THROW(TSesObj.InitRec(&ses_rec, TSESK_SESSION, pPrcRec->ID, super_sess_id, TSESST_INPROCESS));
	ses_rec.StDt  = pRec->Dtm.d;
	ses_rec.StTm  = pRec->Dtm.t;
	ses_rec.FinDt = pRec->Dtm.d;
	ses_rec.FinTm = pRec->Dtm.t;
	ses_rec.FinTm.addhs(99);
	ses_rec.ArID  = atol(pRec->ArCode);
	ses_rec.LinkBillID = atol(pRec->BillCode);
	if(!ses_rec.ArID && ses_rec.LinkBillID) {
		BillTbl::Rec bill_rec;
		if(BillObj->Search(ses_rec.LinkBillID, &bill_rec) > 0)
			ses_rec.ArID = bill_rec.Object;
	}
	THROW(TSesObj.PutRec(pSessID, &ses_rec, 0));
	Add(&ses_rec, 1);
	if(P_Logger) {
		SString msg_buf;
		TSesObj.MakeName(&ses_rec, msg_buf);
		P_Logger->LogString(PPTXT_LOG_BHTTSESS_CREATED, msg_buf);
	}
	CATCHZOK
	return ok;
}

int SLAPI BhtTSess::SwitchPrc(ProcessorTbl::Rec * pPrcRec, PPID destPrcID)
{
	int    ok = 1;
	SString msg_buf;
	if(P_Logger)
		msg_buf.Cat(pPrcRec->Name).CatCharN('-', 2).CatChar('>');
	THROW(TSesObj.GetPrc(destPrcID, pPrcRec, 0, 1) > 0);
	if(P_Logger) {
		msg_buf.Cat(pPrcRec->Name);
		P_Logger->LogString(PPTXT_LOG_BHTTSESS_PRCSWITCH, msg_buf);
	}
	CATCHZOK
	return ok;
}

int SLAPI BhtTSess::SelectSession(const BhtTSessRec * pRec)
{
	int    ok = -1, r;
	uint   prev_entry_pos = LastUsedEntryPos;
	uint   pos = 0, i;
	PPID   sess_id = 0;
	PPID   prc_id  = 0;
	PPID   ar_id   = 0;
	PPID   bill_id = atol(pRec->BillCode);
	PPIDArray spl;
	SString msg_buf;
	TSessionTbl::Rec ses_rec;
	ProcessorTbl::Rec prc_rec;
	if(P_Logger) {
		msg_buf.Z().CatEq("PRC", pRec->PrcCode).CatDiv(',', 2).
			CatEq("BILL", pRec->BillCode).CatDiv(',', 2).CatEq("AR", pRec->ArCode);
		P_Logger->LogString(PPTXT_LOG_BHTTSESS_SIGNAL, msg_buf);
	}
	THROW_PP_S(TSesObj.GetPrcByCode(pRec->PrcCode, &prc_rec) > 0, PPERR_UNKNOWNPRCNAME, pRec->PrcCode);
	prc_id = prc_rec.ID;
	if(bill_id) {
		if(SearchByBill(bill_id, &pos)) {
			//
			// Пул сессий содержит искомый документ
			//
			PPID s_prc_id = Get(pos).PrcID;
			if(s_prc_id != prc_rec.ID) {
				//
				// Если процессор, на котором обрабатывается документ отличен от
				// того, который задан, то пытаемся осуществить переключение.
				//
				THROW(r = TSesObj.IsPrcSwitchable(prc_rec.ID, &spl));
				if(r > 0 && spl.lsearch(s_prc_id)) {
					THROW(SwitchPrc(&prc_rec, s_prc_id));
				}
				else {
					PPSetAddedMsgObjName(PPOBJ_PROCESSOR, s_prc_id);
					CALLEXCEPT_PP(PPERR_FOREIGNPRCBILL);
				}
			}
			LastUsedEntryPos = pos;
			ok = 1;
		}
		else if(TSesObj.SearchByLinkBillID(bill_id, &ses_rec) > 0) {
			//
			// Искомого документа в пуле сессий нет, но сессия по нему уже существует
			//
			if(P_Logger) {
				TSesObj.MakeName(&ses_rec, msg_buf);
				P_Logger->LogString(PPTXT_LOG_BHTTSESS_BILLSESSEXISTS, msg_buf);
			}
			if(oneof2(ses_rec.Status, TSESST_CLOSED, TSESST_CANCELED)) {
				//
				// Сессия по документу уже закрыта или отменена: мы с ней уже ничего не сможем сделать.
				//
				CALLEXCEPT_PP(PPERR_BILLPRCSESSCLOSED);
			}
			else {
				if(ses_rec.PrcID != prc_rec.ID) {
					THROW(r = TSesObj.IsPrcSwitchable(prc_rec.ID, &spl));
					if(r > 0 && spl.lsearch(ses_rec.PrcID)) {
						THROW(Add(&ses_rec, 0));
						ok = 1;
					}
					else {
						PPSetAddedMsgObjName(PPOBJ_PROCESSOR, ses_rec.PrcID);
						CALLEXCEPT_PP(PPERR_FOREIGNPRCBILL);
					}
				}
				if(oneof2(ses_rec.Status, TSESST_PLANNED, TSESST_PENDING)) {
					THROW(TSesObj.SetSessionState(&ses_rec, TSESST_INPROCESS, 0));
					THROW(TSesObj.PutRec(&sess_id, &ses_rec, 1));
					if(ok < 0) {
						THROW(Add(&ses_rec, 0));
						ok = 1;
					}
				}
			}
		}
		else {
			//
			// В пуле нужной сессии нет, нет ее среди существующих сессий.
			//
			if(SearchByPrc(prc_rec.ID, &pos)) {
				//
				// Мы нашли сессию по процессору в пуле, но она не подходит по документу.
				// Пытаемся переключить процессор
				//
				THROW(r = TSesObj.IsPrcSwitchable(prc_rec.ID, &spl));
				prc_id = 0;
				for(i = 0; i < spl.getCount(); i++) {
					PPID   temp_prc_id = spl.at(i);
					if(temp_prc_id != prc_rec.ID && !SearchByPrc(temp_prc_id, 0)) {
						PPID   temp_sess_id = 0;
						THROW(r = TSesObj.IsProcessorBusy(temp_prc_id, 0, TSESK_SESSION, pRec->Dtm, 1, &temp_sess_id));
						if(r < 0) {
							THROW(SwitchPrc(&prc_rec, temp_prc_id));
							prc_id = temp_prc_id;
							break;
						}
					}
				}
				THROW_PP(prc_id, PPERR_NOFREEPRC);
				THROW(CreateSess(&sess_id, pRec, &prc_rec));
				ok = 1;
			}
			else {
				//
				// В пуле нет сессии по нужному процессору и нет по нужному документу
				//
			}
		}
	}
	if(ok < 0) {
		if(SearchByPrc(prc_rec.ID, &pos)) {
			LastUsedEntryPos = pos;
			ok = 1;
		}
		else if(TSesObj.P_Tbl->SearchByPrcTime(prc_rec.ID, TSESK_SESSION, pRec->Dtm, &ses_rec) > 0) {
			//
			// На указанный момент по заданному процессору существует сессия //
			//
			if(bill_id && ses_rec.LinkBillID != bill_id) {
				//
				// Эта сессия не подходит нам по документу:
				// пытаемся переключиться на другой процессор
				//
				THROW(r = TSesObj.IsPrcSwitchable(prc_rec.ID, &spl));
				THROW_PP_S(r > 0, PPERR_PRCISBUSY, prc_rec.Name); // Процессор не допускает переключения.
					// Просто сообщаем, что он занят.
				prc_id = 0;
				for(i = 0; i < spl.getCount(); i++) {
					PPID   temp_prc_id = spl.at(i);
					if(temp_prc_id != prc_rec.ID) {
						PPID temp_sess_id = 0;
						THROW(r = TSesObj.IsProcessorBusy(temp_prc_id, 0, TSESK_SESSION, pRec->Dtm, 1, &temp_sess_id));
						if(r < 0) {
							THROW(SwitchPrc(&prc_rec, temp_prc_id));
							prc_id = temp_prc_id;
							break;
						}
					}
				}
				THROW_PP(prc_id, PPERR_NOFREEPRC);
				THROW(CreateSess(&sess_id, pRec, &prc_rec));
				ok = 1;
			}
			else {
				//
				// Привязываемся к существующей сессии
				//
				THROW(Add(&ses_rec, 0));
				ok = 1;
				if(P_Logger) {
					TSesObj.MakeName(&ses_rec, msg_buf);
					P_Logger->LogString(PPTXT_LOG_BHTTSESS_USED, msg_buf);
				}
			}
		}
		else {
			THROW(CreateSess(&sess_id, pRec, &prc_rec));
			ok = 1;
		}
	}
	if(ok > 0 && LastUsedEntryPos != prev_entry_pos && prev_entry_pos < getCount()) {
		PSE & r_entry = Get(prev_entry_pos);
		if(!r_entry.BillID) { // Сессии, привязанные к документам оставляем в пуле
			THROW(r = CloseSess(prev_entry_pos));
			if(r > 0) {
				atFree(prev_entry_pos);
				if(prev_entry_pos < LastUsedEntryPos)
					LastUsedEntryPos--;
			}
		}
	}
	CATCHZOK
	if(ok <= 0)
		LastUsedEntryPos = UINT_MAX;
	return ok;
}

int SLAPI BhtTSess::CloseSess(uint entryPos)
{
	int    ok = -1;
	if(entryPos < getCount()) {
		PSE & r_entry = Get(entryPos);
		if(r_entry.SessID && r_entry.Flags & fProperSess) {
			TSessionTbl::Rec ses_rec;
			if(TSesObj.Search(r_entry.SessID, &ses_rec) > 0 && ses_rec.Status != TSESST_CLOSED) {
				THROW(TSesObj.SetSessionState(&ses_rec, TSESST_CLOSED, 0));
				THROW(TSesObj.PutRec(&r_entry.SessID, &ses_rec, 0));
				if(P_Logger) {
					SString msg_buf;
					TSesObj.MakeName(&ses_rec, msg_buf);
					P_Logger->LogString(PPTXT_LOG_BHTTSESS_CLOSED, msg_buf);
				}
				ok = 1;
			}
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI BhtTSess::Finish()
{
	int    ok = -1, r;
	for(uint i = 0; i < getCount(); i++) {
		THROW(r = CloseSess(i));
		if(r > 0)
			ok = 1;
	}
	THROW(PPCommitWork(&Ta));
	CATCHZOK
	return ok;
}
//
// ARG(signal IN):
//   1 - начало процесса обработки данных. pRec инициализирована первой записью потока.
//   0 - продолжение процесса обработки данных. pRec инициализирована очередной записью потока.
//   2 - штатное завершение процесса обработки данных.  pRec не инициализирована.
//   3 - отмена предыдущей строки с кодом 0
//  -1 - нештатное (по ошибке) завершение процесса обработки данных. pRec не инициализирована.
//
int SLAPI PPObjTSession::ProcessBhtRec(int signal, const BhtTSessRec * pRec, PPLogger * pLogger, int use_ta)
{
	int    ok = 1, r;
	SString add_info_buf;
	if(signal != 1)
		THROW_PP(P_BhtCurSess, PPERR_BHTTSESSNINIT);
	if(signal == 1 || signal == 0) {
		if(signal == 1) {
			// start process
			if(P_BhtCurSess == 0)
				P_BhtCurSess = new BhtTSess(pLogger);
			else {
				P_BhtCurSess->Reset();
				P_BhtCurSess->Ta = 0;
			}
			P_BhtCurSess->SetLastLine(0, 0);
			THROW(PPStartTransaction(&P_BhtCurSess->Ta, use_ta));
		}
		if(pRec->PrcCode[0]) {
			THROW(r = P_BhtCurSess->SelectSession(pRec));
		}
		else {
			//
			// Строка сессии
			//
			PPID   sess_id = P_BhtCurSess->GetLastUsedSessID();
			if(sess_id && pRec->Barcode[0]) {
				int    r = -1, err_code = PPERR_BARCODEORSERNFOUND;
				SString code = pRec->Barcode;
				TSessLineTbl::Rec line_rec;
				if(P_Tbl->SearchLineByTime(sess_id, pRec->Dtm, &line_rec) > 0) {
					;
				}
				else {
					MEMSZERO(line_rec);
					line_rec.TSessID = sess_id;
					line_rec.UserID  = LConfig.User;
					line_rec.Dt = pRec->Dtm.d;
					line_rec.Tm = pRec->Dtm.t;
					line_rec.Sign = -1;

					BarcodeTbl::Rec bc_rec;
					PPSetAddedMsgString(code);
					if(GObj.SearchByBarcode(code, &bc_rec, 0, 1) > 0) {
						line_rec.GoodsID = bc_rec.GoodsID;
						line_rec.Qtty = (pRec->Qtty > 0) ? pRec->Qtty : ((bc_rec.Qtty > 1) ? bc_rec.Qtty : 1);
						r = 1;
					}
					else {
						SelectBySerialParam ssp(sess_id, code);
						int    r2 = SelectBySerial(&ssp);
						if(r2 == 1) {
							line_rec.GoodsID = ssp.GoodsID;
							line_rec.Qtty    = (pRec->Qtty > 0) ? pRec->Qtty : ((ssp.Qtty > 1) ? ssp.Qtty : 1);
							line_rec.LotID   = ssp.LotID;
							STRNSCPY(line_rec.Serial, ssp.Serial);
							r = 1;
						}
						else if(r2 == 2) {
							if(P_BhtCurSess->IsDupSerialAllowed()) {
								//
								// Процессор допускает дублирование серийных номеров в одной сессии
								//
								line_rec.GoodsID = ssp.GoodsID;
								line_rec.Qtty    = (pRec->Qtty > 0) ? pRec->Qtty : ((ssp.Qtty > 1) ? ssp.Qtty : 1);
								line_rec.LotID   = ssp.LotID;
								STRNSCPY(line_rec.Serial, ssp.Serial);
								r = 1;
							}
							else
								PPSetError(PPERR_SERIALUSED, code);
						}
						else if(r2 == -2) {
							//
							// Лот с серийным номером найден, но находится на другом складе либо
							// закончился. Следовательно, даем сообщение в журнал, и инициализируем
							// ИД товара, количество. Серийный номер не используем.
							//
							line_rec.GoodsID = ssp.GoodsID;
							// @v5.2.12 line_rec.Qtty    = (ssp.Qtty > 1) ? ssp.Qtty : 1;
							line_rec.Qtty    = (pRec->Qtty > 0) ? pRec->Qtty : ((ssp.Qtty > 1) ? ssp.Qtty : 1);
							memzero(line_rec.Serial, sizeof(line_rec.Serial));
							if(pLogger)
								pLogger->LogLastError();
							r = 1;
						}
					}
					PPSetAddedMsgString(code);
					THROW(r > 0);
					{
						long   oprno = 0;
						line_rec.Flags |= TSESLF_EXPANDSESS;
						THROW(PutLine(sess_id, &oprno, &line_rec, 0));
						P_BhtCurSess->SetLastLine(sess_id, oprno);
					}
				}
			}
		}
	}
	else if(signal == 2 || signal == -1) {
		// end process
		if(signal == 2) {
			if(P_BhtCurSess)
				THROW(P_BhtCurSess->Finish());
		}
		else {
			THROW(PPRollbackWork(&P_BhtCurSess->Ta));
		}
		ZDELETE(P_BhtCurSess);
	}
	else if(signal == 3) {
		int    r = -1;
		if(P_BhtCurSess) {
			THROW(r = P_BhtCurSess->RemoveLastLine(0));
			P_BhtCurSess->SetLastLine(0, 0);
		}
		CALLPTRMEMB(pLogger, LogString((r > 0) ? PPTXT_LOG_BHTTSESS_LASTLNRMVD : PPTXT_LOG_BHTRSESS_CANTRMVLASTLN, 0));
	}
	CATCH
		ok = 0;
		CALLPTRMEMB(pLogger, LogLastError());
	ENDCATCH
	return ok;
}
//
//
//
int SLAPI PPObjTSession::HandleMsg(int msg, PPID _obj, PPID _id, void * extraPtr)
{
	int    ok = DBRPL_OK;
	if(msg == DBMSG_OBJDELETE) {
		PPID   sess_id = 0;
		if(P_Tbl->SearchAnyRef(_obj, _id, &sess_id) > 0)
			ok = RetRefsExistsErr(Obj, sess_id);
	}
	else if(msg == DBMSG_OBJREPLACE)
		if(_obj == PPOBJ_ARTICLE)
			ok = P_Tbl->ReplaceArticle(_id, reinterpret_cast<long>(extraPtr)) ? DBRPL_OK : DBRPL_ERROR;
		else if(_obj == PPOBJ_GOODS)
			ok = P_Tbl->ReplaceGoods(_id, reinterpret_cast<long>(extraPtr)) ? DBRPL_OK : DBRPL_ERROR;
	return ok;
}

int SLAPI PPObjTSession::GetSerialListByGoodsID(PPID goodsID, PPID locID, SVector * pList)
{
	PPObjBill * p_bobj = BillObj;
	SerialByGoodsListItem item;
	ReceiptCore & r_rcpt = p_bobj->trfr->Rcpt;
	SString temp_buf;
	LotArray lot_list;
	r_rcpt.GetListOfOpenedLots(1, goodsID, locID, ZERODATE, &lot_list);
	for(uint i = 0; i < lot_list.getCount(); i++) {
		const ReceiptTbl::Rec & r_lot_rec = lot_list.at(i);
		if(p_bobj->GetSerialNumberByLot(r_lot_rec.ID, temp_buf, 1) > 0) {
			temp_buf.CopyTo(item.Serial, sizeof(item.Serial));
			p_bobj->MakeLotText(&r_lot_rec, 0, temp_buf);
			item.LotID   = r_lot_rec.ID;
			item.TSessID = 0;
			item.Qtty    = r_lot_rec.Rest;
			temp_buf.CopyTo(item.Text, sizeof(item.Text));
			pList->insert(&item);
		}
	}
	{
		TSessLineTbl * p_ln = &P_Tbl->Lines;
		TSessLineTbl::Key2 k2;
		BExtQuery q(p_ln, 2);
		q.select(p_ln->TSessID, p_ln->Serial, p_ln->Flags, p_ln->Qtty, 0).
			where(p_ln->GoodsID == labs(goodsID) && p_ln->Sign > 0L);
		MEMSZERO(k2);
		k2.GoodsID = labs(goodsID);
		for(q.initIteration(0, &k2, spGe); q.nextIteration() > 0;)
			if(*strip(p_ln->data.Serial) != 0) {
				TSessionTbl::Rec ses_rec;
				if(!(p_ln->data.Flags & TSESLF_RECOMPL) && Search(p_ln->data.TSessID, &ses_rec) > 0) {
					STRNSCPY(item.Serial, p_ln->data.Serial);
					MakeName(&ses_rec, temp_buf);
					temp_buf.CopyTo(item.Text, sizeof(item.Text));
					item.LotID   = 0;
					item.TSessID = ses_rec.ID;
					item.Qtty    = p_ln->data.Qtty;
					pList->insert(&item);
				}
			}
	}
	return (pList->getCount() ? 1 : -1);
}

int SLAPI PPObjTSession::SelectSerialByGoods(PPID goodsID, PPID locID, SerialByGoodsListItem * pItem)
{
	int    ok = -1;
	PPListDialog * dlg = 0;
	SVector list(sizeof(SerialByGoodsListItem)); // @v10.7.7 SArray-->SVector
	if(GetSerialListByGoodsID(goodsID, locID, &list) > 0) {
		if(CheckDialogPtrErr(&(dlg = new PPListDialog(DLG_SELSERIAL, CTL_SELSERIAL_LIST, PPListDialog::fOnDblClkOk)))) {
			SString goods_name;
			StringSet ss(SLBColumnDelim);
			dlg->setCtrlString(CTL_SELSERIAL_GOODSNAME, GetGoodsName(goodsID, goods_name));
			SerialByGoodsListItem * p_item;
			for(uint i = 0; list.enumItems(&i, (void **)&p_item);) {
				ss.clear();
				ss.add(p_item->Serial);
				ss.add(p_item->Text);
				dlg->addStringToList(i, ss.getBuf());
			}
			dlg->Draw_();
			if(ExecView(dlg) == cmOK) {
				PPID   id = 0;
				if(dlg->getSelection(&id) > 0 && id > 0 && id <= list.getCountI()) {
					ASSIGN_PTR(pItem, *static_cast<const SerialByGoodsListItem *>(list.at(id-1)));
					ok = 1;
				}
			}
		}
		else
			ok = 0;
	}
	delete dlg;
	return ok;
}

int SLAPI PPObjTSession::CallCheckPaneBySess(PPID sessID, int verifyOnly)
{
	int    ok = -1;
	PPCashMachine * p_cm = 0;
	TSessionTbl::Rec rec;
	if(BillObj->CheckRights(BILLRT_CASH) && Search(sessID, &rec) > 0 &&
		rec.Status == TSESST_CLOSED && !rec.CCheckID_ && !(rec.Flags & TSESF_IDLE)) {
		ProcessorTbl::Rec prc_rec;
		if(GetPrc(rec.PrcID, &prc_rec, 1, 1) > 0) {
			PPID   cn_id = PPObjCashNode::Select(prc_rec.LocID, 1, 0, BIN(verifyOnly));
			PPObjCashNode cn_obj;
			PPCashNode cn_rec;
			if(cn_id > 0 && cn_obj.Search(cn_id, &cn_rec) > 0) {
				if(!verifyOnly) {
					SString init_str;
					THROW(p_cm = PPCashMachine::CreateInstance(cn_id));
					THROW(p_cm->SyncBrowseCheckList((init_str = "TSN").Cat(sessID), cchkpanfOnce));
				}
				ok = 1;
			}
		}
	}
	CATCHZOKPPERR
	delete p_cm;
	return ok;
}

int SLAPI PPObjTSession::GetPlaceStatus(PPID tsessID, const char * pPlaceCode, PPID quotKindID, PPID quotLocID, PlaceStatus & rStatus)
{
	int    ok = -1;
	TSessionTbl::Rec tsess_rec;
    rStatus.Init();
	THROW(Search(tsessID, &tsess_rec) > 0);
	rStatus.TSessID = tsessID;
    if(tsess_rec.PrcID) {
        SString temp_buf;
        PPProcessorPacket prc_pack;
        PPProcessorPacket::PlaceDescription pd;
        THROW(PrcObj.GetPacket(tsess_rec.PrcID, &prc_pack) > 0);
        ProcessorPlaceCodeTemplate::NormalizeCode(rStatus.PlaceCode = pPlaceCode);
		if(prc_pack.Ext.GetPlaceDescriptionByCode(pPlaceCode, pd) > 0) {
            rStatus.GoodsID = pd.GoodsID;
            rStatus.Descr = pd.Descr;
			rStatus.Status = 1;
		}
		if(rStatus.Status != 0) {
			PPCheckInPersonMngr ci_mgr;
			PPCheckInPersonArray cilist;
			THROW(ci_mgr.GetList(PPCheckInPersonItem::kTSession, tsessID, cilist));
			for(uint i = 0; i < cilist.GetCount(); i++) {
				const PPCheckInPersonItem & r_ci = cilist.Get(i);
				ProcessorPlaceCodeTemplate::NormalizeCode(temp_buf = r_ci.PlaceCode);
				const int current_status = r_ci.GetStatus();
				if(temp_buf == rStatus.PlaceCode && current_status != PPCheckInPersonItem::statusCanceled) {
					if(current_status == PPCheckInPersonItem::statusCheckedIn)
						rStatus.Status = -2;
					else if(current_status == PPCheckInPersonItem::statusRegistered)
						rStatus.Status = -1;
					else // Строго говоря, это состояние невозможно
						rStatus.Status = -1;
					rStatus.RegPersonID = r_ci.GetPerson();
					rStatus.CipID = r_ci.ID;
					r_ci.CalcPinCode(rStatus.PinCode);
					break;
				}
			}
			if(pd.GoodsID && quotKindID) {
                PPObjGoods goods_obj;
                QuotIdent qi(getcurdate_(), NZOR(quotLocID, prc_pack.Rec.LocID), quotKindID, 0, 0);
                double quot = 0.0;
                if(goods_obj.GetQuotExt(pd.GoodsID, qi, &quot, 1) > 0)
					rStatus.Price = quot;
			}
			ok = 1;
		}
    }
	CATCHZOK
	return ok;
}
//
//
//
int SLAPI PPObjTSession::NormalizePacket(TSessionPacket * pPack, long options)
{
	int    ok = 1;
	if(pPack) {
		if(options & npoDBX) {
			pPack->Rec.OrderLotID = 0;
		}
		pPack->CiList.Normalize(PPCheckInPersonItem::kTSession, pPack->Rec.ID);
		for(uint i = 0; i < pPack->Lines.getCount(); i++) {
			TSessLineTbl::Rec & r_line = pPack->Lines.at(i);
			r_line.TSessID = pPack->Rec.ID;
			if(options & npoDBX) {
				r_line.LotID = 0;
				r_line.UserID = 0;
			}
		}
	}
	return ok;
}

IMPL_DESTROY_OBJ_PACK(PPObjTSession, TSessionPacket);

int SLAPI PPObjTSession::SerializePacket(int dir, TSessionPacket * pPack, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	THROW_SL(pSCtx->Serialize(dir, pPack->Flags, rBuf));
	THROW_SL(P_Tbl->SerializeRecord(dir, &pPack->Rec, rBuf, pSCtx));
	THROW(pPack->CiList.Serialize(dir, rBuf, pSCtx));
	THROW(pSCtx->Serialize(dir, &pPack->Lines, rBuf));
	CATCHZOK
	return ok;
}

int SLAPI PPObjTSession::Read(PPObjPack * p, PPID id, void * stream, ObjTransmContext * pCtx)
{
	int    ok = 1;
	TSessionPacket * p_pack = new TSessionPacket;
	THROW_MEM(p_pack);
	if(stream == 0) {
		THROW(GetPacket(id, p_pack, gpoLoadLines) > 0);
	}
	else {
		SBuffer buffer;
		THROW_SL(buffer.ReadFromFile(static_cast<FILE *>(stream), 0))
		THROW(SerializePacket(-1, p_pack, buffer, &pCtx->SCtx));
	}
	p->Data = p_pack;
	CATCH
		ok = 0;
		delete p_pack;
	ENDCATCH
	return ok;
}

int SLAPI PPObjTSession::Write(PPObjPack * p, PPID * pID, void * stream, ObjTransmContext * pCtx)
{
	int    ok = 1;
	TSessionPacket * p_pack = p ? static_cast<TSessionPacket *>(p->Data) : 0;
	if(p_pack)
		if(stream == 0) {
			PPID   same_id = 0;
			TSessionTbl::Rec same_rec;
			THROW(NormalizePacket(p_pack, npoDBX));
			if(*pID == 0 && SearchAnalog(p_pack->Rec, &same_id, &same_rec) > 0) {
				*pID = same_id;
				p_pack->Rec.Num = 0;
			}
			else if(*pID && Search(*pID, &same_rec) > 0) {
				same_id = *pID;
				p_pack->Rec.Num = 0;
			}
			const int is_new = BIN(*pID == 0);
			int    r = PutPacket(pID, p_pack, 1);
			if(!r) {
				SString name_buf;
				MakeName(&p_pack->Rec, name_buf);
				pCtx->OutputAcceptObjErrMsg(Obj, p_pack->Rec.ID, name_buf);
				ok = -1;
			}
			else if(r > 0)
				ok = is_new ? 101 : 102; // @ObjectCreated : @ObjectUpdated
		}
		else {
			SBuffer buffer;
			THROW(SerializePacket(+1, p_pack, buffer, &pCtx->SCtx));
			THROW_SL(buffer.WriteToFile(static_cast<FILE *>(stream), 0, 0))
		}
	CATCHZOK
	return ok;
}

int SLAPI PPObjTSession::ProcessObjRefs(PPObjPack * p, PPObjIDArray * ary, int replace, ObjTransmContext * pCtx)
{
	int    ok = 1;
	if(p && p->Data) {
		TSessionPacket * p_pack = static_cast<TSessionPacket *>(p->Data);
		uint   i;
		THROW(ProcessObjRefInArray(PPOBJ_TSESSION, &p_pack->Rec.ParentID, ary, replace));
		THROW(ProcessObjRefInArray(PPOBJ_TECH, &p_pack->Rec.TechID, ary, replace));
		THROW(ProcessObjRefInArray(PPOBJ_PROCESSOR, &p_pack->Rec.PrcID, ary, replace));
		THROW(ProcessObjRefInArray(PPOBJ_ARTICLE, &p_pack->Rec.ArID, ary, replace));
		THROW(ProcessObjRefInArray(PPOBJ_ARTICLE, &p_pack->Rec.Ar2ID, ary, replace));
		THROW(ProcessObjRefInArray(PPOBJ_TSESSION, &p_pack->Rec.PrevSessID, ary, replace));
		THROW(ProcessObjRefInArray(PPOBJ_BILL, &p_pack->Rec.LinkBillID, ary, replace));
		THROW(ProcessObjRefInArray(PPOBJ_SCARD, &p_pack->Rec.SCardID, ary, replace));
		THROW(p_pack->CiList.ProcessObjRefs(ary, replace, pCtx));
		for(i = 0; i < p_pack->Lines.getCount(); i++) {
			TSessLineTbl::Rec & r_line = p_pack->Lines.at(i);
			THROW(ProcessObjRefInArray(PPOBJ_GOODS, &r_line.GoodsID, ary, replace));
		}
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}
//
// Descr: Автоматическая обработка технологических сессий
//
SLAPI PrcssrTSessMaintenance::Param::Param()
{
	Init();
}

void SLAPI PrcssrTSessMaintenance::Param::Init()
{
	THISZERO();
	Ver = DS.GetVersion();
}

int SLAPI PrcssrTSessMaintenance::Param::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	THROW_SL(Ver.Serialize(dir, rBuf, pSCtx));
	THROW(pSCtx->Serialize(dir, Period, rBuf));
	THROW(pSCtx->Serialize(dir, Flags, rBuf));
	CATCHZOK
	return ok;
}

SLAPI PrcssrTSessMaintenance::PrcssrTSessMaintenance()
{
}

int SLAPI PrcssrTSessMaintenance::InitParam(PrcssrTSessMaintenance::Param * pP)
{
	int    ok = 1;
	CALLPTRMEMB(pP, Init());
	return ok;
}

int SLAPI PrcssrTSessMaintenance::EditParam(PrcssrTSessMaintenance::Param * pP)
{
	class TSessMaintDialog : public TDialog {
		DECL_DIALOG_DATA(PrcssrTSessMaintenance::Param);
	public:
		TSessMaintDialog() : TDialog(DLG_TSESMNT)
		{
			SetupCalPeriod(CTLCAL_TSESMNT_PERIOD, CTL_TSESMNT_PERIOD);
		}
		DECL_DIALOG_SETDTS()
		{
			int    ok = 1;
			RVALUEPTR(Data, pData);
			SetPeriodInput(this, CTL_TSESMNT_PERIOD, &Data.Period);
			AddClusterAssoc(CTL_TSESMNT_ACTION, 0, PrcssrTSessMaintenance::Param::fCancelCip);
			SetClusterData(CTL_TSESMNT_ACTION, Data.Flags);
			return ok;
		}
		DECL_DIALOG_GETDTS()
		{
			int    ok = 1;
			uint   sel = 0;
			THROW(GetPeriodInput(this, sel = CTL_TSESMNT_PERIOD, &Data.Period));
			GetClusterData(CTL_TSESMNT_ACTION, &Data.Flags);
			ASSIGN_PTR(pData, Data);
			CATCHZOKPPERRBYDLG
			return ok;
		}
	};
	DIALOG_PROC_BODY(TSessMaintDialog, pP);
}

int SLAPI PrcssrTSessMaintenance::Init(const PrcssrTSessMaintenance::Param * pP)
{
	int    ok = 1;
	RVALUEPTR(P, pP);
	P.Period.Actualize(ZERODATE);
	return ok;
}

int SLAPI PrcssrTSessMaintenance::Run()
{
    int    ok = -1;
    const  LDATETIME curdtm = getcurdatetime_();
	PPLogger logger;
    if(P.Flags & Param::fCancelCip) {
		SString fmt_buf, msg_buf, temp_buf;
		PPCheckInPersonMngr ci_mgr;
		TSessionFilt filt;
		PPViewTSession view;
		TSessionViewItem item;
		PPCheckInPersonArray ci_list;
		filt.StPeriod = P.Period;
		THROW(view.Init_(&filt));
		for(view.InitIteration(0); view.NextIteration(&item) > 0;) {
			ProcessorTbl::Rec prc_rec;
			if(item.PrcID && TSesObj.GetPrc(item.PrcID, &prc_rec, 1, 0) > 0 && prc_rec.Flags & PRCF_ALLOWCIP) {
				PPProcessorPacket::ExtBlock prc_ext;
				if(TSesObj.PrcObj.GetExtention(item.PrcID, &prc_ext) > 0) {
					const long cto = prc_ext.GetCipCancelTimeout();
					const long lto = prc_ext.GetCipLockTimeout();
					if(cto > 0 || lto > 0) {
						ci_list.Z();
						int    ci_list_updated = 0;
						if(ci_mgr.GetList(PPCheckInPersonItem::kTSession, item.ID, ci_list) > 0) {
							LDATETIME stdtm;
							stdtm.Set(item.StDt, item.StTm);
							for(uint i = 0; i < ci_list.GetCount(); i++) {
								const PPCheckInPersonItem & r_ci_item = ci_list.Get(i);
								if(!(r_ci_item.Flags & (PPCheckInPersonItem::fCanceled|PPCheckInPersonItem::fCheckedIn))) {
									int   do_cancel_reserve = 0;
									LDATETIME rtm = r_ci_item.RegDtm;
									long diffsec = 0;
									if(cto > 0) {
										if(rtm.d) {
											diffsec = diffdatetimesec(curdtm, rtm);
											if(diffsec > cto)
												do_cancel_reserve = 1;
										}
									}
									if(lto > 0) {
										diffsec = diffdatetimesec(stdtm, curdtm);
										if(diffsec < lto)
											do_cancel_reserve = 2;
									}
									if(do_cancel_reserve) {
										PPCheckInPersonItem ci_item = r_ci_item;
										ci_item.Flags |= PPCheckInPersonItem::fCanceled;
                                        if(ci_list.UpdateItem(i, ci_item, 0)) {
											ci_list_updated = 1;
											msg_buf.Z();
											if(do_cancel_reserve == 1)
												PPLoadText(PPTXT_LOG_TSESCIPCANCEL_CTO, msg_buf);
											else if(do_cancel_reserve == 2)
												PPLoadText(PPTXT_LOG_TSESCIPCANCEL_LTO, msg_buf);
											if(msg_buf.NotEmptyS()) {
												TSesObj.MakeName(&item, temp_buf);
												msg_buf.CatDiv(':', 2).Cat(temp_buf);
												msg_buf.CatDiv('-', 1).Cat(r_ci_item.RegDtm, DATF_DMY, TIMF_HM);
												if(r_ci_item.PlaceCode[0])
													msg_buf.CatDiv('-', 1).Cat(r_ci_item.PlaceCode);
												ci_item.GetPersonName(temp_buf.Z());
												if(temp_buf.NotEmptyS())
													msg_buf.CatDiv('-', 1).Cat(temp_buf);
												msg_buf.CatDiv('-', 1).CatEq("timeout", (do_cancel_reserve == 2) ? lto : cto);
											}
											logger.Log(msg_buf);
                                        }
									}
								}
							}
						}
						if(ci_list_updated) {
							THROW(ci_mgr.Put(ci_list, 1));
						}
					}
				}
			}
		}
    }
    CATCH
		ok = 0;
		logger.LogLastError();
    ENDCATCH
    logger.Save(PPFILNAM_INFO_LOG, 0);
    return ok;
}
//
// Implementation of PPALDD_UhttTSession
//
struct UhttTSessionBlock {
	enum {
		stFetch = 0,
		stSet
	};
	UhttTSessionBlock() : LinePos(0), CipPos(0), PlacePos(0), TagPos(0), State(stFetch)
	{
	}
	void Clear()
	{
		Pack.destroy();
		LinePos = 0;
		CipPos = 0;
		PlacePos = 0;
		TagPos = 0;
		State = stFetch;
	}
	PPObjTSession TSesObj;
	PPObjTag TagObj;
	TSessionPacket Pack;
	uint   LinePos;
	uint   CipPos;
	uint   PlacePos;
	uint   TagPos;
	int    State;
};

PPALDD_CONSTRUCTOR(UhttTSession)
{
	Extra[0].Ptr = new UhttTSessionBlock();
	InitFixData(rscDefHdr, &H, sizeof(H));
	InitFixData("iter@Lines", &I_Lines, sizeof(I_Lines));
	InitFixData("iter@Cips", &I_Cips, sizeof(I_Cips));
	InitFixData("iter@Places", &I_Places, sizeof(I_Places));
	InitFixData("iter@TagList", &I_TagList, sizeof(I_TagList));
}

PPALDD_DESTRUCTOR(UhttTSession)
{
	Destroy();
	delete static_cast<UhttTSessionBlock *>(Extra[0].Ptr);
}

int PPALDD_UhttTSession::InitData(PPFilt & rFilt, long rsrv)
{
	int    ok = -1;
	if(rFilt.ID == H.ID)
		ok = DlRtm::InitData(rFilt, rsrv);
	else {
		MEMSZERO(H);
		H.ID = rFilt.ID;
		UhttTSessionBlock & r_blk = *static_cast<UhttTSessionBlock *>(Extra[0].Ptr);
		if(r_blk.TSesObj.GetPacket(rFilt.ID, &r_blk.Pack, PPObjTSession::gpoLoadLines) > 0) {
			LDATETIME dtm;
			SString temp_buf;
			H.ID     = r_blk.Pack.Rec.ID;
			H.Num    = r_blk.Pack.Rec.Num;
			H.PrcID  = r_blk.Pack.Rec.PrcID;
			H.TechID = r_blk.Pack.Rec.TechID;
			H.ParentID = r_blk.Pack.Rec.ParentID;
			H.Status  = r_blk.Pack.Rec.Status;
			H.Flags  = r_blk.Pack.Rec.Flags;

			dtm.Set(r_blk.Pack.Rec.StDt, r_blk.Pack.Rec.StTm);
			temp_buf.Z().Cat(dtm, DATF_ISO8601|DATF_CENTURY, 0);
			STRNSCPY(H.StTime, temp_buf);
			dtm.Set(r_blk.Pack.Rec.FinDt, r_blk.Pack.Rec.FinTm);
			temp_buf.Z().Cat(dtm, DATF_ISO8601|DATF_CENTURY, 0);
			STRNSCPY(H.StTime, temp_buf);
			STRNSCPY(H.Memo, r_blk.Pack.Rec.Memo);
			r_blk.Pack.Ext.GetExtStrData(PRCEXSTR_DETAILDESCR, temp_buf.Z());
			STRNSCPY(H.Detail, temp_buf);
			ok = DlRtm::InitData(rFilt, rsrv);
		}
	}
	return ok;
}

int PPALDD_UhttTSession::InitIteration(long iterId, int sortId, long rsrv)
{
	IterProlog(iterId, 1);
	UhttTSessionBlock & r_blk = *static_cast<UhttTSessionBlock *>(Extra[0].Ptr);
	if(iterId == GetIterID("iter@Lines"))
		r_blk.LinePos = 0;
	if(iterId == GetIterID("iter@Cips"))
		r_blk.CipPos = 0;
	else if(iterId == GetIterID("iter@Places"))
		r_blk.PlacePos = 0;
	else if(iterId == GetIterID("iter@TagList"))
		r_blk.TagPos = 0;
	return 1;
}

int PPALDD_UhttTSession::NextIteration(long iterId)
{
	int     ok = -1;
	LDATETIME dtm;
	SString temp_buf;
	IterProlog(iterId, 0);
	UhttTSessionBlock & r_blk = *static_cast<UhttTSessionBlock *>(Extra[0].Ptr);
	if(iterId == GetIterID("iter@Lines")) {
        if(r_blk.LinePos < r_blk.Pack.Lines.getCount()) {
			const TSessLineTbl::Rec & r_item = r_blk.Pack.Lines.at(r_blk.LinePos);
			#define CPY_FLD(f) I_Lines.f = r_item.f
			CPY_FLD(TSessID);
			CPY_FLD(OprNo);
			CPY_FLD(GoodsID);
			CPY_FLD(LotID);
			CPY_FLD(UserID);
			CPY_FLD(Sign);
			CPY_FLD(Flags);
			CPY_FLD(Qtty);
			CPY_FLD(WtQtty);
			CPY_FLD(Price);
			CPY_FLD(Discount);
			#undef CPY_FLD

			dtm.Set(r_item.Dt, r_item.Tm);
			temp_buf.Z().Cat(dtm, DATF_ISO8601|DATF_CENTURY, 0).CopyTo(I_Lines.Tm, sizeof(I_Lines.Tm));
			dtm.Set(r_item.Expiry, ZEROTIME);
			temp_buf.Z().Cat(dtm, DATF_ISO8601|DATF_CENTURY, 0).CopyTo(I_Lines.Expiry, sizeof(I_Lines.Expiry));

			STRNSCPY(I_Lines.Serial, r_item.Serial);
			ok = DlRtm::NextIteration(iterId);
        }
        r_blk.LinePos++;
	}
	else if(iterId == GetIterID("iter@Cips")) {
		if(r_blk.CipPos < r_blk.Pack.CiList.GetCount()) {
			const PPCheckInPersonItem & r_item = r_blk.Pack.CiList.Get(r_blk.CipPos);
			#define CPY_FLD(f) I_Cips.f = r_item.f
			CPY_FLD(ID);
			CPY_FLD(Kind);
			CPY_FLD(PrmrID);
			CPY_FLD(Num);
			CPY_FLD(RegCount);
			CPY_FLD(CiCount);
			CPY_FLD(Flags);
			CPY_FLD(Amount);
			CPY_FLD(CCheckID);
			CPY_FLD(SCardID);
			#undef CPY_FLD

			I_Cips.PersonID = r_item.GetPerson();

			temp_buf.Z().Cat(r_item.RegDtm, DATF_ISO8601|DATF_CENTURY, 0).CopyTo(I_Cips.RegTm, sizeof(I_Cips.RegTm));
			temp_buf.Z().Cat(r_item.CiDtm, DATF_ISO8601|DATF_CENTURY, 0).CopyTo(I_Cips.CiTm, sizeof(I_Cips.CiTm));

			r_blk.Pack.CiList.GetMemo(r_blk.CipPos, temp_buf.Z());
			STRNSCPY(I_Cips.Memo, temp_buf);
			STRNSCPY(I_Cips.PlaceCode, r_item.PlaceCode);
			ok = DlRtm::NextIteration(iterId);
		}
		r_blk.CipPos++;
	}
	else if(iterId == GetIterID("iter@Places")) {
		PPProcessorPacket::PlaceDescription pd_item;
		if(r_blk.Pack.Ext.GetPlaceDescription(r_blk.PlacePos++, pd_item)) {
            I_Places.GoodsID = pd_item.GoodsID;
            STRNSCPY(I_Places.Range, pd_item.Range);
            STRNSCPY(I_Places.Descr, pd_item.Descr);
			ok = DlRtm::NextIteration(iterId);
		}
	}
	else if(iterId == GetIterID("iter@TagList")) {
		if(r_blk.TagPos < r_blk.Pack.TagL.GetCount()) {
			MEMSZERO(I_TagList);
			const ObjTagItem * p_item = r_blk.Pack.TagL.GetItemByPos(r_blk.TagPos);
			I_TagList.TagTypeID = p_item->TagDataType;
			{
				PPObjectTag rec;
				if(r_blk.TagObj.Fetch(p_item->TagID, &rec) > 0)
					STRNSCPY(I_TagList.TagSymb, rec.Symb);
			}
			switch(p_item->TagDataType) {
				case OTTYP_STRING:
				case OTTYP_GUID:
					p_item->GetStr(temp_buf.Z());
					STRNSCPY(I_TagList.StrVal, temp_buf);
					break;
				case OTTYP_NUMBER: p_item->GetReal(&I_TagList.RealVal); break;
				case OTTYP_BOOL:
				case OTTYP_INT: p_item->GetInt(&I_TagList.IntVal); break;
				case OTTYP_DATE: p_item->GetDate(&I_TagList.DateVal); break;
			}
			ok = DlRtm::NextIteration(iterId);
		}
		r_blk.TagPos++;
	}
	return ok;
}

int PPALDD_UhttTSession::Set(long iterId, int commit)
{
	int    ok = 1;
	UhttTSessionBlock & r_blk = *static_cast<UhttTSessionBlock *>(Extra[0].Ptr);
	if(r_blk.State != UhttTSessionBlock::stSet) {
		r_blk.Clear();
		r_blk.State = UhttTSessionBlock::stSet;
	}
	if(commit == 0) {
		LDATETIME dtm;
		if(iterId == 0) {
			ProcessorTbl::Rec prc_rec;
			r_blk.Pack.Rec.ID = H.ID;
			r_blk.Pack.Rec.Num = H.Num;
			THROW_PP(H.PrcID, PPERR_PRCNEEDED);
			THROW(r_blk.TSesObj.GetPrc(H.PrcID, &prc_rec, 0, 1) > 0);
			THROW_PP_S(prc_rec.Kind == PPPRCK_PROCESSOR, PPERR_REALPRCNEEDED, prc_rec.Name);
			r_blk.Pack.Rec.PrcID = H.PrcID;
			{
				TechTbl::Rec tec_rec;
				if(H.TechID) {
					THROW(r_blk.TSesObj.GetTech(H.TechID, &tec_rec, 1) > 0);
					r_blk.Pack.Rec.TechID = H.TechID;
				}
				else {
					PPIDArray tec_list;
					r_blk.TSesObj.TecObj.GetListByPrcGoods(r_blk.Pack.Rec.PrcID, 0, &tec_list);
					if(tec_list.getCount() == 1)
						r_blk.Pack.Rec.TechID = tec_list.get(0);
				}
			}
			THROW_PP(r_blk.Pack.Rec.TechID, PPERR_CANTIDENTTSESSTECH);
			r_blk.Pack.Rec.ParentID = H.ParentID;
			//r_blk.Pack.Rec.Status = (int16)H.Status;
			r_blk.Pack.Rec.Flags = H.Flags;
			r_blk.Pack.Rec.Incomplete = 10;
			strtodatetime(H.StTime, &dtm, DATF_ISO8601, 0);
			r_blk.Pack.Rec.StDt = dtm.d;
			r_blk.Pack.Rec.StTm = dtm.t;

			strtodatetime(H.FinTime, &dtm, DATF_ISO8601, 0);
			r_blk.Pack.Rec.FinDt = dtm.d;
			r_blk.Pack.Rec.FinTm = dtm.t;

			STRNSCPY(r_blk.Pack.Rec.Memo, H.Memo);
			r_blk.Pack.Ext.PutExtStrData(PRCEXSTR_DETAILDESCR, H.Detail);
			THROW(r_blk.TSesObj.SetSessionState(&r_blk.Pack.Rec, H.Status, 0));
		}
		else if(iterId == GetIterID("iter@Lines")) {
			TSessLineTbl::Rec item;
			#define CPY_FLD(f) item.f = I_Lines.f
			CPY_FLD(TSessID);
			CPY_FLD(OprNo);
			CPY_FLD(GoodsID);
			CPY_FLD(LotID);
			CPY_FLD(UserID);
			item.Sign = (int16)I_Lines.Sign;
			CPY_FLD(Flags);
			CPY_FLD(Qtty);
			CPY_FLD(WtQtty);
			CPY_FLD(Price);
			CPY_FLD(Discount);
			#undef CPY_FLD

			dtm.Set(I_Lines.Tm, DATF_ISO8601, TIMF_HMS);
			item.Dt = dtm.d;
			item.Tm = dtm.t;
			dtm.Set(I_Lines.Expiry, DATF_ISO8601, TIMF_HMS);
			item.Expiry = dtm.d;

			STRNSCPY(item.Serial, I_Lines.Serial);
            THROW_SL(r_blk.Pack.Lines.insert(&item));
		}
		else if(iterId == GetIterID("iter@Cips")) {
			PPCheckInPersonItem item;
			#define CPY_FLD(f) item.f = I_Cips.f
			CPY_FLD(ID);
			CPY_FLD(Kind);
			CPY_FLD(PrmrID);
			CPY_FLD(Num);
			item.RegCount = (uint16)I_Cips.RegCount;
			item.CiCount = (uint16)I_Cips.CiCount;
			CPY_FLD(Flags);
			CPY_FLD(Amount);
			CPY_FLD(CCheckID);
			CPY_FLD(SCardID);
			#undef CPY_FLD

			item.SetPerson(I_Cips.PersonID);
			item.RegDtm.Set(I_Cips.RegTm, DATF_ISO8601, TIMF_HMS);
			item.CiDtm.Set(I_Cips.CiTm, DATF_ISO8601, TIMF_HMS);
			STRNSCPY(item.PlaceCode, I_Cips.PlaceCode);

			uint   new_pos = 0;
			THROW(r_blk.Pack.CiList.AddItem(item, 0, &new_pos));
			r_blk.Pack.CiList.SetMemo(new_pos, I_Cips.Memo);
		}
		else if(iterId == GetIterID("iter@Places")) {
		}
		else if(iterId == GetIterID("iter@TagList")) {
			PPID   id = 0;
			ObjTagItem item;
			PPObjectTag rec;
			if(r_blk.TagObj.SearchBySymb(I_TagList.TagSymb, &id, &rec) > 0) {
				item.TagID = rec.ID;
				switch(rec.TagDataType) {
					case OTTYP_STRING:
					case OTTYP_GUID: THROW(item.SetStr(item.TagID, I_TagList.StrVal)); break;
					case OTTYP_NUMBER: THROW(item.SetReal(item.TagID, I_TagList.RealVal)); break;
					case OTTYP_INT: THROW(item.SetInt(item.TagID, I_TagList.IntVal)); break;
					case OTTYP_DATE: THROW(item.SetDate(item.TagID, I_TagList.DateVal)); break;
				}
				THROW(r_blk.Pack.TagL.PutItem(rec.ID, &item));
			}
		}
	}
	else {
		PPID  id = r_blk.Pack.Rec.ID;
		THROW(r_blk.TSesObj.PutPacket(&id, &r_blk.Pack, 1));
		Extra[4].Ptr = reinterpret_cast<void *>(id);
	}
	CATCHZOK
	if(commit || !ok)
		r_blk.Clear();
	return ok;
}
//
// Implementation of PPALDD_UhttTSessPlaceStatusList
//
PPALDD_CONSTRUCTOR(UhttTSessPlaceStatusList)
{
	InitFixData(rscDefHdr, &H, sizeof(H));
	InitFixData(rscDefIter, &I, sizeof(I));
}

PPALDD_DESTRUCTOR(UhttTSessPlaceStatusList) { Destroy(); }

int PPALDD_UhttTSessPlaceStatusList::InitData(PPFilt & rFilt, long rsrv)
{
	TSCollection <PPObjTSession::PlaceStatus> * p_list = static_cast<TSCollection <PPObjTSession::PlaceStatus> *>(rFilt.Ptr);
	Extra[0].Ptr = p_list;
	return DlRtm::InitData(rFilt, rsrv);
}

int PPALDD_UhttTSessPlaceStatusList::InitIteration(long iterId, int sortId, long rsrv)
{
	IterProlog(iterId, 1);
	TSCollection <PPObjTSession::PlaceStatus> * p_list = static_cast<TSCollection <PPObjTSession::PlaceStatus> *>(Extra[0].Ptr);
	CALLPTRMEMB(p_list, setPointer(0));
	return -1;
}

int PPALDD_UhttTSessPlaceStatusList::NextIteration(long iterId)
{
	int    ok = -1;
	IterProlog(iterId, 0);
	TSCollection <PPObjTSession::PlaceStatus> * p_list = static_cast<TSCollection <PPObjTSession::PlaceStatus> *>(Extra[0].Ptr);
	if(p_list && p_list->getPointer() < p_list->getCount()) {
		PPObjTSession::PlaceStatus * p_item = p_list->at(p_list->getPointer());
		I.TSessID = p_item->TSessID;
		I.GoodsID = p_item->GoodsID;
		I.Price = p_item->Price;
		I.Status = p_item->Status;
		I.PersonID = p_item->RegPersonID;
		I.CipID = p_item->CipID;
		STRNSCPY(I.Code, p_item->PlaceCode);
		STRNSCPY(I.PinCode, p_item->PinCode);
		STRNSCPY(I.Descr, p_item->Descr);
		p_list->incPointer();
		ok = DlRtm::NextIteration(iterId);
	}
	return ok;
}
