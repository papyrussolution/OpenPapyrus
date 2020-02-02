// OBJTIMESERIES.CPP
// Copyright (c) A.Sobolev 2018, 2019, 2020
// @codepage UTF-8
// Модуль, управляющий объектом данных PPObjTimeSeries
//
#include <pp.h>
#pragma hdrstop
#include <fann2.h>

SLAPI PPTimeSeries::PPTimeSeries() : Tag(PPOBJ_TIMESERIES), ID(0), Flags(0), Type(tUnkn), BuyMarg(0.0), SellMarg(0.0),
	SpikeQuant(0.0), Prec(0), AvgSpread(0.0), OptMaxDuck(0), OptMaxDuck_S(0), PeakAvgQuant(0), PeakAvgQuant_S(0), TargetQuant(0)
{
	PTR32(Name)[0] = 0;
	PTR32(Symb)[0] = 0;
	PTR32(CurrencySymb)[0] = 0;
	//memzero(Reserve, sizeof(Reserve));
	Reserve2 = 0;
}

int FASTCALL PPTimeSeries::IsEqual(const PPTimeSeries & rS) const
{
	int    eq = 1;
	if(!sstreq(Name, rS.Name))
		eq = 0;
	else if(!sstreq(Symb, rS.Symb))
		eq = 0;
	else if(Type != rS.Type) // @v10.5.6
		eq = 0;
	else if(Flags != rS.Flags)
		eq = 0;
	else if(BuyMarg != rS.BuyMarg)
		eq = 0;
	else if(SellMarg != rS.SellMarg)
		eq = 0;
	else if(Prec != rS.Prec)
		eq = 0;
	else if(SpikeQuant != rS.SpikeQuant)
		eq = 0;
	else if(AvgSpread != rS.AvgSpread)
		eq = 0;
	else if(OptMaxDuck != rS.OptMaxDuck)
		eq = 0;
	else if(OptMaxDuck_S != rS.OptMaxDuck_S)
		eq = 0;
	else if(PeakAvgQuant != rS.PeakAvgQuant)
		eq = 0;
	else if(PeakAvgQuant_S != rS.PeakAvgQuant_S)
		eq = 0;
	else if(TargetQuant != rS.TargetQuant) // @v10.4.2
		eq = 0;
	else if(!sstreq(CurrencySymb, rS.CurrencySymb))
		eq = 0;
	return eq;
}

SLAPI PPTimeSeriesPacket::Extension::Extension() : MarginManual(0.0), FixedStakeVolume(0.0)
{
}

int SLAPI PPTimeSeriesPacket::Extension::IsEmpty() const
{
	return (MarginManual == 0.0 && FixedStakeVolume == 0.0);
}

int FASTCALL PPTimeSeriesPacket::Extension::IsEqual(const PPTimeSeriesPacket::Extension & rS) const
{
	int    eq = 1;
	if(MarginManual != rS.MarginManual)
		eq = 0;
	else if(FixedStakeVolume != rS.FixedStakeVolume) // @v10.6.3
		eq = 0;
	return eq;
}

SLAPI PPTimeSeriesPacket::PPTimeSeriesPacket()
{
}

int FASTCALL PPTimeSeriesPacket::IsEqual(const PPTimeSeriesPacket & rS) const
{
	int    eq = 1;
	if(!Rec.IsEqual(rS.Rec))
		eq = 0;
	else if(!E.IsEqual(rS.E))
		eq = 0;
	return eq;
}

double SLAPI PPTimeSeriesPacket::GetMargin(int sell) const
	{ return inrangeordefault(E.MarginManual, 1E-8, 1.0, (sell ? Rec.SellMarg : Rec.BuyMarg)); }
const char * SLAPI PPTimeSeriesPacket::GetSymb() const
	{ return Rec.Symb; }
SLAPI PPObjTimeSeries::Config::Entry::Entry() : TsID(0), Flags(0)
	{ memzero(Reserve, sizeof(Reserve)); }

PPObjTimeSeries::Config::ExtBlock::ExtBlock() : MaxAvgTimeSec(0), TsFlashTimer(0), MinLossQuantForReverse(0), MinAgeSecondsForReverse(0),
	TerminalTimeAdjustment(0)
{
	memzero(Reserve, sizeof(Reserve));
}

SLAPI PPObjTimeSeries::Config::Config() : /*Tag(PPOBJ_CONFIG), ID(PPCFG_MAIN), Prop(PPPRP_TSSTAKECFG),*/Ver(DS.GetVersion()), Flags(0), MaxStakeCount(0),
	AvailableLimitPart(0.0), AvailableLimitAbs(0.0), MinPerDayPotential(0.0)
{
}

int FASTCALL PPObjTimeSeries::Config::IsEqual(const PPObjTimeSeries::Config & rS) const
{
	int    eq = 1;
	if(Ver != rS.Ver)
		eq = 0;
	else if(Flags != rS.Flags)
		eq = 0;
	else if(MaxStakeCount != rS.MaxStakeCount)
		eq = 0;
	else if(AvailableLimitPart != rS.AvailableLimitPart)
		eq = 0;
	else if(AvailableLimitAbs != rS.AvailableLimitAbs)
		eq = 0;
	else if(MinPerDayPotential != rS.MinPerDayPotential)
		eq = 0;
	else if(E.MaxAvgTimeSec != rS.E.MaxAvgTimeSec)
		eq = 0;
	else if(E.TsFlashTimer != rS.E.TsFlashTimer)
		eq = 0;
	else if(E.MinLossQuantForReverse != rS.E.MinLossQuantForReverse) // @v10.4.2
		eq = 0;
	else if(E.MinAgeSecondsForReverse != rS.E.MinAgeSecondsForReverse) // @v10.4.2
		eq = 0;
	else if(E.TerminalTimeAdjustment != rS.E.TerminalTimeAdjustment) // @v10.5.5
		eq = 0;
	else if(!List.IsEqual(rS.List))
		eq = 0;
	return eq;
}

PPObjTimeSeries::Config & SLAPI PPObjTimeSeries::Config::Z()
{
	//Tag = PPOBJ_CONFIG;
	//ID = PPCFG_MAIN;
	//Prop = PPPRP_TSSTAKECFG;
	Ver = DS.GetVersion();
	Flags = 0;
	MaxStakeCount = 0;
	AvailableLimitPart = 0.0;
	AvailableLimitAbs = 0.0;
	MinPerDayPotential = 0.0;
	E.TsFlashTimer = 0; // @v10.4.2
	E.MinLossQuantForReverse  = 0; // @v10.4.2
	E.MinAgeSecondsForReverse = 0; // @v10.4.2
	memzero(E.Reserve, sizeof(E.Reserve));
	List.clear();
	return *this;
}

const PPObjTimeSeries::Config::Entry * FASTCALL PPObjTimeSeries::Config::SearchEntry(PPID tsID) const
{
	uint    idx = 0;
	return List.lsearch(&tsID, &idx, CMPF_LONG) ? &List.at(idx) : 0;
}

int SLAPI PPObjTimeSeries::Config::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	if(dir > 0) {
		Ver = DS.GetVersion();
	}
	THROW_SL(Ver.Serialize(dir, rBuf, pSCtx));
	THROW_SL(pSCtx->Serialize(dir, Flags, rBuf));
	THROW_SL(pSCtx->Serialize(dir, MaxStakeCount, rBuf));
	THROW_SL(pSCtx->Serialize(dir, AvailableLimitPart, rBuf));
	THROW_SL(pSCtx->Serialize(dir, AvailableLimitAbs, rBuf));
	THROW_SL(pSCtx->Serialize(dir, MinPerDayPotential, rBuf));
	THROW_SL(pSCtx->SerializeBlock(dir, sizeof(E), &E, rBuf, 0));
	THROW_SL(pSCtx->Serialize(dir, &List, rBuf));
	if(dir > 0) {
		/*if(!Ver.IsLt(10, 3, 1)) {
		}*/
	}
	else if(dir < 0) {
		/*if(Ver.IsLt(10, 3, 1)) {
		}
		else {
		}*/
	}
	CATCHZOK
	return ok;
}

class TimSerCfgDialog : public PPListDialog {
	DECL_DIALOG_DATA(PPObjTimeSeries::Config);
public:
	TimSerCfgDialog() : PPListDialog(DLG_TIMSERCFG, CTL_TIMSERCFG_LIST)
	{
		updateList(-1);
	}
	DECL_DIALOG_SETDTS()
	{
		int    ok = 1;
		RVALUEPTR(Data, pData);
		setCtrlData(CTL_TIMSERCFG_MAXSTAKE, &Data.MaxStakeCount);
		setCtrlReal(CTL_TIMSERCFG_ALIMPART, Data.AvailableLimitPart);
		setCtrlReal(CTL_TIMSERCFG_ALIMABS, Data.AvailableLimitAbs);
		setCtrlData(CTL_TIMSERCFG_MINPOTENT, &Data.MinPerDayPotential);
		setCtrlData(CTL_TIMSERCFG_RVRMLQ,    &Data.E.MinLossQuantForReverse); // @v10.4.2
		setCtrlData(CTL_TIMSERCFG_RVRMAS,    &Data.E.MinAgeSecondsForReverse); // @v10.4.2
		setCtrlData(CTL_TIMSERCFG_TERMTMADJ, &Data.E.TerminalTimeAdjustment); // @v10.5.5
		setCtrlData(CTL_TIMSERCFG_TSFLSHTMR, &Data.E.TsFlashTimer); // @v10.3.3
		AddClusterAssoc(CTL_TIMSERCFG_FLAGS, 0, PPObjTimeSeries::Config::fTestMode);
		AddClusterAssoc(CTL_TIMSERCFG_FLAGS, 1, PPObjTimeSeries::Config::fUseStakeMode2); // @v10.3.3
		AddClusterAssoc(CTL_TIMSERCFG_FLAGS, 2, PPObjTimeSeries::Config::fUseStakeMode3); // @v10.3.3
		AddClusterAssoc(CTL_TIMSERCFG_FLAGS, 3, PPObjTimeSeries::Config::fAllowReverse); // @v10.4.2
		AddClusterAssoc(CTL_TIMSERCFG_FLAGS, 4, PPObjTimeSeries::Config::fVerifMode); // @v10.4.7
		AddClusterAssoc(CTL_TIMSERCFG_FLAGS, 5, PPObjTimeSeries::Config::fIgnoreStrangeStakes); // @v10.6.3
		AddClusterAssoc(CTL_TIMSERCFG_FLAGS, 6, PPObjTimeSeries::Config::fLogStakeEvaluation); // @v10.6.8
		SetClusterData(CTL_TIMSERCFG_FLAGS, Data.Flags);
		updateList(-1);
		return ok;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		getCtrlData(CTL_TIMSERCFG_MAXSTAKE, &Data.MaxStakeCount);
		getCtrlData(CTL_TIMSERCFG_ALIMPART, &Data.AvailableLimitPart);
		getCtrlData(CTL_TIMSERCFG_ALIMABS,  &Data.AvailableLimitAbs);
		getCtrlData(CTL_TIMSERCFG_MINPOTENT, &Data.MinPerDayPotential);
		getCtrlData(CTL_TIMSERCFG_RVRMLQ,    &Data.E.MinLossQuantForReverse); // @v10.4.2
		getCtrlData(CTL_TIMSERCFG_RVRMAS,    &Data.E.MinAgeSecondsForReverse); // @v10.4.2
		getCtrlData(CTL_TIMSERCFG_TERMTMADJ, &Data.E.TerminalTimeAdjustment); // @v10.5.5
		getCtrlData(CTL_TIMSERCFG_TSFLSHTMR, &Data.E.TsFlashTimer); // @v10.3.3
		GetClusterData(CTL_TIMSERCFG_FLAGS, &Data.Flags);
		ASSIGN_PTR(pData, Data);
		return ok;
	}
private:
	virtual int setupList()
	{
		int    ok = 1;
		SString temp_buf;
		PPTimeSeries rec;
		StringSet ss(SLBColumnDelim);
		for(uint i = 0; i < Data.List.getCount(); i++) {
			PPObjTimeSeries::Config::Entry & r_entry = Data.List.at(i);
			ss.clear();
			temp_buf.Z();
			if(TsObj.Search(r_entry.TsID, &rec) > 0) {
				temp_buf.Cat(rec.Symb);
			}
			else {
				rec.Name[0] = 0;
				ideqvalstr(r_entry.TsID, temp_buf);
			}
			ss.add(temp_buf);
			//
			temp_buf.Z();
			if(r_entry.Flags & Data.efDisableStake)
				temp_buf.CatChar('D');
			if(r_entry.Flags & Data.efLong)
				temp_buf.CatChar('L');
			if(r_entry.Flags & Data.efShort)
				temp_buf.CatChar('S');
			ss.add(temp_buf);
			ss.add(rec.Name);
			addStringToList(i+1, ss.getBuf());
		}
		return ok;
	}
	virtual int addItem(long * pPos, long * pID)
	{
		int    ok = -1;
		long   pos = 0;
		long   id = 0;
		PPObjTimeSeries::Config::Entry entry;
		if(EditItem(&entry) > 0) {
			for(uint i = 0; i < Data.List.getCount(); i++) {
				PPObjTimeSeries::Config::Entry & r_temp_entry = Data.List.at(i);
				if(r_temp_entry.TsID == entry.TsID) {
					Data.List.at(i) = entry;
					id = i+1;
					pos = i;
					ok = 1;
					break;
				}
			}
			if(!id) {
				Data.List.insert(&entry);
				id = Data.List.getCount();
				pos = id-1;
				ok = 1;
			}
		}
		ASSIGN_PTR(pPos, pos);
		ASSIGN_PTR(pID, id);
		return ok;
	}
	virtual int editItem(long pos, long id)
	{
		int    ok = -1;
		if(pos >= 0 && pos < static_cast<long>(Data.List.getCount())) {
			PPObjTimeSeries::Config::Entry entry = Data.List.at(pos);
			if(EditItem(&entry) > 0) {
				Data.List.at(pos) = entry;
				ok = 1;
			}
		}
		return ok;
	}
	virtual int delItem(long pos, long id)
	{
		int    ok = -1;
		if(pos >= 0 && pos < static_cast<long>(Data.List.getCount())) {
			Data.List.atFree(pos);
			ok = 1;
		}
		return ok;
	}
	int EditItem(PPObjTimeSeries::Config::Entry * pData)
	{
		int    ok = -1;
		TDialog * dlg = new TDialog(DLG_TIMSERCFGITEM);
		if(CheckDialogPtrErr(&dlg)) {
			SetupPPObjCombo(dlg, CTLSEL_TIMSERCFGITEM_TS, PPOBJ_TIMESERIES, pData->TsID, 0, 0);
			dlg->disableCtrl(CTLSEL_TIMSERCFGITEM_TS, pData->TsID != 0);
			dlg->AddClusterAssoc(CTL_TIMSERCFGITEM_FLAGS, 0, PPObjTimeSeries::Config::efLong);
			dlg->AddClusterAssoc(CTL_TIMSERCFGITEM_FLAGS, 1, PPObjTimeSeries::Config::efShort);
			dlg->AddClusterAssoc(CTL_TIMSERCFGITEM_FLAGS, 2, PPObjTimeSeries::Config::efDisableStake);
			dlg->SetClusterData(CTL_TIMSERCFGITEM_FLAGS, pData->Flags);
			while(ok < 0 && ExecView(dlg) == cmOK) {
				dlg->getCtrlData(CTLSEL_TIMSERCFGITEM_TS, &pData->TsID);
				dlg->GetClusterData(CTL_TIMSERCFGITEM_FLAGS, &pData->Flags);
				if(pData->TsID) {
					ok = 1;
				}
				else {
					PPErrorByDialog(dlg, CTLSEL_TIMSERCFGITEM_TS, PPERR_TIMSERNEEDED);
				}
			}
		}
		delete dlg;
		return ok;
	}
	PPObjTimeSeries TsObj;
};

//static
int SLAPI PPObjTimeSeries::EditConfig(const PPObjTimeSeries::Config * pCfg)
{
	int    ok = -1;
	PPObjTimeSeries::Config config;
	if(!RVALUEPTR(config, pCfg)) {
		THROW(PPObjTimeSeries::ReadConfig(&config));
	}
	ok = PPDialogProcBody <TimSerCfgDialog, PPObjTimeSeries::Config> (&config);
	if(ok > 0 && !pCfg) {
		THROW(PPObjTimeSeries::WriteConfig(&config, 1));
	}
	CATCH
		ok = PPErrorZ();
	ENDCATCH
	return ok;
}

//static
int SLAPI PPObjTimeSeries::WriteConfig(PPObjTimeSeries::Config * pCfg, int use_ta)
{
	int    ok = 1;
	int    is_new = 0;
	int    is_upd = 1;
	PPObjTimeSeries::Config ex_cfg;
	if(ReadConfig(&ex_cfg) > 0) {
		if(pCfg && pCfg->IsEqual(ex_cfg))
			is_upd = 0;
	}
	else
		is_new = 1;
	if(is_upd) {
		SBuffer buffer;
		if(pCfg) {
			SSerializeContext sctx;
			THROW(pCfg->Serialize(1, buffer, &sctx));
		}
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(PPRef->PutPropSBuffer(PPOBJ_CONFIG, PPCFG_MAIN, PPPRP_TSSTAKECFG, buffer, 0));
		DS.LogAction(is_new ? PPACN_CONFIGCREATED : PPACN_CONFIGUPDATED, PPCFGOBJ_TIMESERIES, 0, 0, 0);
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

//static
int SLAPI PPObjTimeSeries::ReadConfig(PPObjTimeSeries::Config * pCfg)
{
	int    ok = -1;
	SBuffer buffer;
	pCfg->Z();
	SSerializeContext sctx;
	if(PPRef->GetPropSBuffer(PPOBJ_CONFIG, PPCFG_MAIN, PPPRP_TSSTAKECFG, buffer) > 0) {
		if(pCfg->Serialize(-1, buffer, &sctx))
			ok = 1;
		else {
			pCfg->Z();
			ok = 0;
		}
	}
	return ok;
}

SLAPI PPObjTimeSeries::PPObjTimeSeries(void * extraPtr) : PPObjReference(PPOBJ_TIMESERIES, extraPtr)
{
}

int SLAPI PPObjTimeSeries::EditDialog(PPTimeSeriesPacket * pEntry)
{
	class TimeSeriesDialog : public TDialog {
		DECL_DIALOG_DATA(PPTimeSeriesPacket);
	public:
		TimeSeriesDialog() : TDialog(DLG_TIMSER)
		{
		}
		DECL_DIALOG_SETDTS()
		{
			RVALUEPTR(Data, pData);
			int    ok = 1;
			SString temp_buf;
			setCtrlLong(CTL_TIMSER_ID, Data.Rec.ID);
			setCtrlString(CTL_TIMSER_NAME, temp_buf = Data.Rec.Name);
			setCtrlString(CTL_TIMSER_SYMB, temp_buf = Data.Rec.Symb);
			AddClusterAssoc(CTL_TIMSER_FLAGS, 0, PPCommObjEntry::fPassive);
			SetClusterData(CTL_TIMSER_FLAGS, Data.Rec.Flags);
			disableCtrl(CTL_TIMSER_ID, (!PPMaster || Data.Rec.ID));
			// @v10.5.6 {
			AddClusterAssocDef(CTL_TIMSER_TYPE, 0, PPTimeSeries::tUnkn);
			AddClusterAssoc(CTL_TIMSER_TYPE, 1, PPTimeSeries::tForex);
			AddClusterAssoc(CTL_TIMSER_TYPE, 2, PPTimeSeries::tStocks);
			SetClusterData(CTL_TIMSER_TYPE, Data.Rec.Type);
			// } @v10.5.6
			setCtrlData(CTL_TIMSER_PREC, &Data.Rec.Prec);
			setCtrlReal(CTL_TIMSER_BUYMARG, Data.Rec.BuyMarg);
			setCtrlReal(CTL_TIMSER_SELLMARG, Data.Rec.SellMarg);
			setCtrlReal(CTL_TIMSER_MANMARG, Data.E.MarginManual); // @v10.5.5
			setCtrlReal(CTL_TIMSER_FXSTAKE, Data.E.FixedStakeVolume); // @v10.6.3
			setCtrlReal(CTL_TIMSER_QUANT, Data.Rec.SpikeQuant);
			setCtrlReal(CTL_TIMSER_AVGSPRD, Data.Rec.AvgSpread);
			setCtrlData(CTL_TIMSER_OPTMD, &Data.Rec.OptMaxDuck);
			setCtrlData(CTL_TIMSER_OPTMDS, &Data.Rec.OptMaxDuck_S);
			setCtrlData(CTL_TIMSER_TGTQUANT, &Data.Rec.TargetQuant); // @v10.4.2
			return ok;
		}
		DECL_DIALOG_GETDTS()
		{
			int    ok = 1;
			uint   sel = 0;
			PPID   _id = Data.Rec.ID;
			SString temp_buf;
			getCtrlData(CTL_TIMSER_ID, &_id);
			getCtrlString(sel = CTL_TIMSER_NAME, temp_buf);
			THROW(Obj.CheckName(_id, temp_buf, 1));
			STRNSCPY(Data.Rec.Name, temp_buf);
			getCtrlString(sel = CTL_TIMSER_SYMB, temp_buf);
			THROW(Obj.ref->CheckUniqueSymb(Obj.Obj, _id, temp_buf, offsetof(ReferenceTbl::Rec, Symb)));
			STRNSCPY(Data.Rec.Symb, temp_buf);
			GetClusterData(CTL_TIMSER_TYPE,  &Data.Rec.Type); // @v10.5.6
			GetClusterData(CTL_TIMSER_FLAGS, &Data.Rec.Flags);
			getCtrlData(CTL_TIMSER_TGTQUANT, &Data.Rec.TargetQuant); // @v10.4.2
			getCtrlData(sel = CTL_TIMSER_MANMARG, &Data.E.MarginManual); // @v10.5.5
			THROW_PP(Data.E.MarginManual >= 0.0 && Data.E.MarginManual <= 2.0, PPERR_USERINPUT); // @v10.5.5
			getCtrlData(CTL_TIMSER_FXSTAKE, &Data.E.FixedStakeVolume); // @v10.6.3
			ASSIGN_PTR(pData, Data);
			CATCHZOKPPERRBYDLG
			return ok;
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isCmd(cmTimSerStat)) {
				SString temp_buf;
				SString out_buf;
				if(Data.Rec.ID) {
					STimeSeries ts;
					if(Obj.GetTimeSeries(Data.Rec.ID, ts) > 0) {
						const uint tsc = ts.GetCount();
						if(tsc) {
							STimeSeries::Stat stat(0);
							//ts.Sort(); // @debug
							ts.Analyze("close", stat);
							out_buf.CatEq("count", stat.GetCount()).Space().CRB();
							out_buf.CatEq("min", stat.GetMin(), MKSFMTD(0, 5, 0)).Space().CRB();
							out_buf.CatEq("max", stat.GetMax(), MKSFMTD(0, 5, 0)).Space().CRB();
							out_buf.CatEq("avg", stat.GetExp(), MKSFMTD(0, 5, 0)).Space().CRB();
							out_buf.CatEq("delta-avg", stat.DeltaAvg, MKSFMTD(0, 8, 0)).Space().CRB();
							temp_buf.Z().Cat((stat.State & stat.stSorted) ? "sorted" : "unsorted").Space().
								Cat((stat.State & stat.stHasTmDup) ? "has-dup" : "no-dup");
							out_buf.Cat(temp_buf).Space().CRB();
							out_buf.CatEq("spread-avg", Data.Rec.AvgSpread, MKSFMTD(0, 1, 0)).Space().CRB();
							out_buf.CatEq("spike-quant", Data.Rec.SpikeQuant, MKSFMTD(0, 9, 0)).Space().CRB();
							out_buf.CatEq("buy-margin", Data.Rec.BuyMarg, MKSFMTD(0, 6, 0)).Space().CRB();
							SUniTime utm;
							LDATETIME dtm;
							ts.GetTime(0, &utm);
							utm.Get(dtm);
							out_buf.CatEq("first_time", temp_buf.Z().Cat(dtm, DATF_DMY|DATF_CENTURY, TIMF_HMS)).Space().CRB();
							ts.GetTime(tsc-1, &utm);
							utm.Get(dtm);
							out_buf.CatEq("last_time", temp_buf.Z().Cat(dtm, DATF_DMY|DATF_CENTURY, TIMF_HMS)).Space().CRB();
							if(!(stat.State & stat.stSorted) || (stat.State & stat.stHasTmDup)) {
								int rr = ts.Repair("close");
								if(rr > 0) {
									if(!Obj.SetTimeSeries(Data.Rec.ID, &ts, 1))
										PPError();
								}
							}
						}
						else
							out_buf = "empty series";
					}
					else
						out_buf = "no series";
				}
				else
					out_buf = "no series";
				setCtrlString(CTL_TIMSER_STAT, out_buf);
			}
			else
				return;
			clearEvent(event);
		}
		PPObjTimeSeries Obj;
	};
	int    ok = -1;
	TimeSeriesDialog * p_dlg = 0;
	if(pEntry) {
		SString obj_title;
		THROW(CheckDialogPtr(&(p_dlg = new TimeSeriesDialog())));
		THROW(EditPrereq(&pEntry->Rec.ID, p_dlg, 0));
		p_dlg->setTitle(GetObjectTitle(Obj, obj_title));
		p_dlg->setDTS(pEntry);
		while(ok < 0 && ExecView(p_dlg) == cmOK) {
			if(p_dlg->getDTS(pEntry)) {
				ok = 1;
			}
		}
	}
	CATCHZOK
	delete p_dlg;
	return ok;
}

//virtual
int SLAPI PPObjTimeSeries::Edit(PPID * pID, void * extraPtr)
{
	int    ok = cmCancel;
	int    is_new = 0;
	PPTimeSeriesPacket pack;
	THROW(EditPrereq(pID, 0, &is_new));
	if(!is_new) {
		THROW(GetPacket(*pID, &pack) > 0);
	}
	{
		THROW(ok = EditDialog(&pack));
		if(ok > 0) {
			THROW(is_new || CheckRights(PPR_MOD));
			if(*pID)
				*pID = pack.Rec.ID;
			THROW(PutPacket(pID, &pack, 1));
			ok = cmOK;
		}
	}
	CATCHZOKPPERR
	return ok;
}

int  SLAPI PPObjTimeSeries::RemoveObjV(PPID id, ObjCollection * pObjColl, uint options/* = rmv_default*/, void * pExtraParam)
{
	int    r = -1, conf = 1;
	THROW(CheckRights(PPR_DEL));
	if(options & PPObject::user_request) {
		conf = CONFIRM(PPCFM_DELETE);
	}
	if(conf) {
		PPWait(1);
		THROW(PutPacket(&id, 0, BIN(options & PPObject::use_transaction)));
		r = 1;
	}
	CATCH
		r = 0;
		if(options & PPObject::user_request)
			PPError();
	ENDCATCH
	PPWait(0);
	return r;
}

struct TimeSeriesExtention {
	PPID   Tag;              // Const=PPOBJ_TIMESERIES
	PPID   ID;               // @id
	PPID   Prop;             // Const=TIMSERPRP_EXTENSION
	SVerT  Ver;              // Версия системы, создавшей запись
	uint8  Reserve[44];      // @v10.6.3 [52]-->[44]
	double FixedStakeVolume; // @v10.6.3
	double MarginManual;     //
	long   Reserve1;         //
	long   Reserve2;         //
};

int SLAPI PPObjTimeSeries::PutPacket(PPID * pID, PPTimeSeriesPacket * pPack, int use_ta)
{
	//TIMSERPRP_EXTENSION
	int    ok = 1;
	PPID   _id = DEREFPTRORZ(pID);
	const  int is_new = (_id == 0);
	const  int is_removing = BIN(*pID != 0 && pPack == 0);
	PPID   hid = 0;
	PPTimeSeriesPacket org_pack;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(_id) {
			THROW(GetPacket(_id, &org_pack) > 0);
		}
		if(pPack == 0) {
			if(*pID) {
				TextRefIdent tri(Obj, _id, PPTRPROP_TIMESERIES);
				THROW(CheckRights(PPR_DEL));
				THROW(ref->RemoveItem(Obj, _id, 0));
				THROW(ref->RemoveProperty(Obj, _id, 0, 0));
				//THROW(ref->Ot.PutList(Obj, _id, 0, 0));
				THROW(ref->UtrC.SetTimeSeries(tri, 0, 0));
				//THROW(RemoveSync(_id));
				DS.LogAction(PPACN_OBJRMV, Obj, *pID, hid, 0);
			}
		}
		else {
			const TimeSeriesExtention * p_ext = 0;
			TimeSeriesExtention ext_blk;
			if(!pPack->E.IsEmpty()) {
				MEMSZERO(ext_blk);
				ext_blk.Ver = DS.GetVersion();
				ext_blk.MarginManual = pPack->E.MarginManual;
				ext_blk.FixedStakeVolume = pPack->E.FixedStakeVolume; // @v10.6.3
				p_ext = &ext_blk;
			}
			THROW(CheckRightsModByID(pID));
			if(_id) {
				if(pPack->IsEqual(org_pack))
					ok = -1;
				else {
					THROW(ref->UpdateItem(Obj, _id, pPack, 1, 0));
					THROW(ref->PutProp(Obj, _id, TIMSERPRP_EXTENSION, p_ext, sizeof(*p_ext), 0)); // @v10.5.5
					//THROW(ref->Ot.PutList(Obj, _id, &pPack->TagL, 0));
					DS.LogAction(PPACN_OBJUPD, Obj, _id, 0, 0);
				}
			}
			else {
				THROW(ref->AddItem(Obj, &_id, pPack, 0));
				pPack->Rec.ID = _id;
				THROW(ref->PutProp(Obj, _id, TIMSERPRP_EXTENSION, p_ext, sizeof(*p_ext), 0)); // @v10.5.5
				//THROW(ref->Ot.PutList(Obj, _id, &pPack->TagL, 0));
				DS.LogAction(PPACN_OBJADD, Obj, _id, 0, 0);
				ASSIGN_PTR(pID, _id);
			}
		}
		THROW(tra.Commit());
	}
	CATCH
		if(is_new) {
			*pID = 0;
			if(pPack)
				pPack->Rec.ID = 0;
		}
		ok = 0;
	ENDCATCH
	return ok;
}

int SLAPI PPObjTimeSeries::GetPacket(PPID id, PPTimeSeriesPacket * pPack)
{
	int    ok = Search(id, &pPack->Rec);
	if(ok > 0) {
		// @v10.5.5 {
		TimeSeriesExtention ext_blk;
		if(ref->GetProperty(Obj, id, TIMSERPRP_EXTENSION, &ext_blk, sizeof(ext_blk)) > 0) {
			if(ext_blk.MarginManual > 0.0 && ext_blk.MarginManual <= 2.0) {
				pPack->E.MarginManual = ext_blk.MarginManual;
			}
			pPack->E.FixedStakeVolume = ext_blk.FixedStakeVolume; // @v10.6.3
		}
		// } @v10.5.5
	}
	return ok;
}

int SLAPI PPObjTimeSeries::SetTimeSeries(PPID id, STimeSeries * pTs, int use_ta)
{
	int    ok = 1;
	PPTimeSeries ts_rec;
	{
		long   stored_ts_count = 0;
		PPUserFuncProfiler ufp(PPUPRF_TIMSERWRITE); // @v10.3.3
		{
			TextRefIdent tri(Obj, id, PPTRPROP_TIMESERIES);
			PPTransaction tra(use_ta);
			THROW(tra);
			THROW(Search(id, &ts_rec) > 0);
			THROW(ref->UtrC.SetTimeSeries(tri, pTs, 0));
			stored_ts_count = pTs ? static_cast<long>(pTs->GetCount()) : 0L;
			DS.LogAction(PPACN_TSSERIESUPD, Obj, id, stored_ts_count, 0); // @v10.3.3
			THROW(tra.Commit());
		}
		ufp.SetFactor(0, static_cast<double>(stored_ts_count)); // @v10.3.3
		ufp.Commit(); // @v10.3.3
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjTimeSeries::GetTimeSeries(PPID id, STimeSeries & rTs)
{
	TextRefIdent tri(Obj, id, PPTRPROP_TIMESERIES);
	return ref->UtrC.Search(tri, rTs);
}

IMPLEMENT_PPFILT_FACTORY(TimeSeries); SLAPI TimeSeriesFilt::TimeSeriesFilt() : PPBaseFilt(PPFILT_TIMESERIES, 0, 0)
{
	SetFlatChunk(offsetof(TimeSeriesFilt, ReserveStart),
		offsetof(TimeSeriesFilt, Reserve)-offsetof(TimeSeriesFilt, ReserveStart)+sizeof(Reserve));
	Init(1, 0);
}

SLAPI PPViewTimeSeries::PPViewTimeSeries() : PPView(&Obj, &Filt, PPVIEW_TIMESERIES), P_DsList(0)
{
	ImplementFlags |= (implBrowseArray|implOnAddSetupPos|implDontEditNullFilter);
}

SLAPI PPViewTimeSeries::~PPViewTimeSeries()
{
	ZDELETE(P_DsList);
}

int SLAPI PPViewTimeSeries::Init_(const PPBaseFilt * pBaseFilt)
{
	int    ok = 1;
	THROW(Helper_InitBaseFilt(pBaseFilt));
	BExtQuery::ZDelete(&P_IterQuery);
	Obj.ReadConfig(&Cfg);
	Counter.Init();
	CATCHZOK
	return ok;
}

int SLAPI PPViewTimeSeries::EditBaseFilt(PPBaseFilt * pBaseFilt)
{
	return -1;
}

int SLAPI PPViewTimeSeries::InitIteration()
{
	int    ok = 1;
	if(!P_DsList) {
		THROW(MakeList(0));
	}
	CALLPTRMEMB(P_DsList, setPointer(0));
	CATCHZOK
	return ok;
}

int FASTCALL PPViewTimeSeries::NextIteration(TimeSeriesViewItem * pItem)
{
	int    ok = -1;
	while(ok < 0 && P_DsList && P_DsList->getPointer() < P_DsList->getCount()) {
		const BrwItem * p_item = static_cast<const BrwItem *>(P_DsList->at(P_DsList->getPointer()));
		if(p_item && Obj.Search(p_item->ID, pItem) > 0)
			ok = 1;
		P_DsList->incPointer();
	}
	return ok;
}

//static
int FASTCALL PPViewTimeSeries::GetDataForBrowser(SBrowserDataProcBlock * pBlk)
{
	PPViewTimeSeries * p_v = static_cast<PPViewTimeSeries *>(pBlk->ExtraPtr);
	return p_v ? p_v->_GetDataForBrowser(pBlk) : 0;
}

PPViewTimeSeries::BrwItem::BrwItem(const PPTimeSeriesPacket * pTs)
{
	if(pTs) {
		ID = pTs->Rec.ID;
		STRNSCPY(Name, pTs->Rec.Name);
		STRNSCPY(Symb, pTs->Rec.Symb);
		STRNSCPY(CurrencySymb, pTs->Rec.CurrencySymb);
		BuyMarg = pTs->Rec.BuyMarg;
		SellMarg = pTs->Rec.SellMarg;
		ManualMarg = pTs->E.MarginManual; // @v10.5.6
		Type = static_cast<int16>(pTs->Rec.Type); // @v10.5.6
		Prec = pTs->Rec.Prec;
		SpikeQuant = pTs->Rec.SpikeQuant;
		AvgSpread = pTs->Rec.AvgSpread;
		OptMaxDuck = pTs->Rec.OptMaxDuck;
		OptMaxDuck_S = pTs->Rec.OptMaxDuck_S;
		PeakAvgQuant = pTs->Rec.PeakAvgQuant;
		PeakAvgQuant_S = pTs->Rec.PeakAvgQuant_S;
		TargetQuant = pTs->Rec.TargetQuant;
		Flags = pTs->Rec.Flags;
		CfgFlags = 0;
	}
	else {
		THISZERO();
	}
}

int SLAPI PPViewTimeSeries::CmpSortIndexItems(PPViewBrowser * pBrw, const PPViewTimeSeries::BrwItem * pItem1, const PPViewTimeSeries::BrwItem * pItem2)
{
	int    sn = 0;
	AryBrowserDef * p_def = static_cast<AryBrowserDef *>(pBrw->getDef());
	if(p_def) {
		for(uint i = 0; !sn && i < pBrw->GetSettledOrderList().getCount(); i++) {
			int    col = pBrw->GetSettledOrderList().get(i);
			TYPEID typ1 = 0;
			TYPEID typ2 = 0;
			uint8  dest_data1[512];
			uint8  dest_data2[512];
			if(p_def->GetCellData(pItem1, labs(col)-1, &typ1, &dest_data1, sizeof(dest_data1)) && p_def->GetCellData(pItem2, labs(col)-1, &typ2, &dest_data2, sizeof(dest_data2))) {
				assert(typ1 == typ2);
				if(typ1 == typ2) {
					sn = stcomp(typ1, dest_data1, dest_data2);
					if(sn && col < 0)
						sn = -sn;
				}
			}
		}
	}
	return sn;
}

static IMPL_CMPFUNC(PPViewTimeSeriesBrwItem, i1, i2)
{
	int    si = 0;
	PPViewBrowser * p_brw = static_cast<PPViewBrowser *>(pExtraData);
	if(p_brw) {
		PPViewTimeSeries * p_view = static_cast<PPViewTimeSeries *>(p_brw->P_View);
		if(p_view) {
			const PPViewTimeSeries::BrwItem * p_item1 = static_cast<const PPViewTimeSeries::BrwItem *>(i1);
			const PPViewTimeSeries::BrwItem * p_item2 = static_cast<const PPViewTimeSeries::BrwItem *>(i2);
			si = p_view->CmpSortIndexItems(p_brw, p_item1, p_item2);
		}
	}
	return si;
}

int SLAPI PPViewTimeSeries::MakeList(PPViewBrowser * pBrw)
{
	int    ok = 1;
	PPTimeSeries item;
	const  int is_sorting_needed = BIN(pBrw && pBrw->GetSettledOrderList().getCount()); // @v10.6.4
	if(P_DsList)
		P_DsList->clear();
	else
		P_DsList = new SArray(sizeof(BrwItem));
	for(SEnum en = Obj.Enum(0); en.Next(&item) > 0;) {
		PPTimeSeriesPacket pack;
		if(Obj.GetPacket(item.ID, &pack) > 0) {
			BrwItem new_item(&pack);
			const PPObjTimeSeries::Config::Entry * p_ce = Cfg.SearchEntry(item.ID);
			if(p_ce)
				new_item.CfgFlags = p_ce->Flags;
			THROW_SL(P_DsList->insert(&new_item));
		}
	}
	// @v10.6.4 {
	if(pBrw) {
		BrowserDef * p_def = pBrw->getDef();
		for(uint cidx = 0; cidx < p_def->getCount(); cidx++) {
			p_def->at(cidx).Options |= BCO_SORTABLE;
		}
		if(is_sorting_needed)
			P_DsList->sort(PTR_CMPFUNC(PPViewTimeSeriesBrwItem), pBrw);
	}
	// } @v10.6.4 
	CATCHZOK
	return ok;
}

int SLAPI PPViewTimeSeries::_GetDataForBrowser(SBrowserDataProcBlock * pBlk)
{
	int    ok = 0;
	if(pBlk->P_SrcData && pBlk->P_DestData) {
		ok = 1;
		const  BrwItem * p_item = static_cast<const BrwItem *>(pBlk->P_SrcData);
		int    r = 0;
		switch(pBlk->ColumnN) {
			case 0: pBlk->Set(p_item->ID); break; // @id
			case 1: pBlk->Set(p_item->Symb); break; // @symb
			case 2: pBlk->Set(p_item->Name); break; // @name
			case 3: pBlk->Set(p_item->CurrencySymb); break;
			case 4: pBlk->Set(static_cast<long>(p_item->Prec)); break;
			case 5: pBlk->Set(p_item->BuyMarg); break;
			case 6: pBlk->Set(p_item->SellMarg); break;
			case 7: pBlk->Set(p_item->SpikeQuant); break;
			case 8: pBlk->Set(p_item->AvgSpread); break;
			case 9: pBlk->Set(p_item->ManualMarg); break;
		}
	}
	return ok;
}

static int CellStyleFunc(const void * pData, long col, int paintAction, BrowserWindow::CellStyle * pStyle, void * extraPtr)
{
	int    ok = -1;
	PPViewBrowser * p_brw = static_cast<PPViewBrowser *>(extraPtr);
	if(p_brw) {
		PPViewTimeSeries * p_view = static_cast<PPViewTimeSeries *>(p_brw->P_View);
		ok = p_view ? p_view->CellStyleFunc_(pData, col, paintAction, pStyle, p_brw) : -1;
	}
	return ok;
}

int SLAPI PPViewTimeSeries::CellStyleFunc_(const void * pData, long col, int paintAction, BrowserWindow::CellStyle * pCellStyle, PPViewBrowser * pBrw)
{
	int    ok = -1;
	if(pBrw && pData && pCellStyle && col >= 0) {
		const BrowserDef * p_def = pBrw->getDef();
		if(col < static_cast<long>(p_def->getCount())) {
			const BroColumn & r_col = p_def->at(col);
			if(col == 0) { // id
				const long cfg_flags = static_cast<const BrwItem *>(pData)->CfgFlags;
				if(cfg_flags & PPObjTimeSeries::Config::efDisableStake)
					ok = pCellStyle->SetLeftTopCornerColor(GetColorRef(SClrGrey));
				if(cfg_flags & PPObjTimeSeries::Config::efLong && cfg_flags & PPObjTimeSeries::Config::efShort)
					ok = pCellStyle->SetRightFigCircleColor(GetColorRef(SClrOrange));
				else if(cfg_flags & PPObjTimeSeries::Config::efLong)
					ok = pCellStyle->SetRightFigCircleColor(GetColorRef(SClrGreen));
				else if(cfg_flags & PPObjTimeSeries::Config::efShort)
					ok = pCellStyle->SetRightFigCircleColor(GetColorRef(SClrRed));
			}
			else if(col == 2) { // name
				const int16 type = static_cast<const BrwItem *>(pData)->Type;
				if(type == PPTimeSeries::tForex)
					ok = pCellStyle->SetRightFigCircleColor(GetColorRef(SClrPink));
				else if(type == PPTimeSeries::tStocks)
					ok = pCellStyle->SetRightFigCircleColor(GetColorRef(SClrLightblue));
			}
		}
	}
	return ok;
}

void SLAPI PPViewTimeSeries::PreprocessBrowser(PPViewBrowser * pBrw)
{
	if(pBrw) {
		pBrw->SetDefUserProc(PPViewTimeSeries::GetDataForBrowser, this);
		pBrw->SetCellStyleFunc(CellStyleFunc, pBrw);
		// @v10.6.4 {
		BrowserDef * p_def = pBrw->getDef();
		if(p_def) {
			for(uint cidx = 0; cidx < p_def->getCount(); cidx++) {
				p_def->at(cidx).Options |= BCO_SORTABLE;
			}
		}
		// } @v10.6.4 
	}
}

SArray * SLAPI PPViewTimeSeries::CreateBrowserArray(uint * pBrwId, SString * pSubTitle)
{
	uint   brw_id = BROWSER_TIMESERIES;
	SArray * p_array = 0;
	PPTimeSeries ds_item;
	THROW(MakeList(0));
	p_array = new SArray(*P_DsList);
	CATCH
		ZDELETE(P_DsList);
	ENDCATCH
	ASSIGN_PTR(pBrwId, brw_id);
	return p_array;
}

int SLAPI PPViewTimeSeries::OnExecBrowser(PPViewBrowser *)
{
	return -1;
}

int SLAPI PPViewTimeSeries::Detail(const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = -1;
	PPID   id = pHdr ? *static_cast<const PPID *>(pHdr) : 0;
	if(id) {
		TimSerDetailFilt filt;
		filt.TsID = id;
		PPView::Execute(PPVIEW_TIMSERDETAIL, &filt, /*PPView::exefModeless*/0, 0);
	}
	return ok;
}

enum {
	removetimeseriesStrategies = 0x0001,
	removetimeseriesTimSer     = 0x0002,
	removetimeseriesCompletely = 0x0004,
};

static int CallRemoveTimeSeriesDialog(const char * pWarnTextSign, long * pData)
{
	class RemoveTimeSeriesDialog : public TDialog {
		DECL_DIALOG_DATA(long);
	public:
		RemoveTimeSeriesDialog(const char * pWarnTextSign) : TDialog(DLG_RMVTIMSER), Data(0)
		{
			setStaticText(CTL_RMVTIMSER_WARN, PPLoadStringS(pWarnTextSign, SLS.AcquireRvlStr()));
		}
		DECL_DIALOG_SETDTS()
		{
			int    ok = 1;
			RVALUEPTR(Data, pData);
			AddClusterAssoc(CTL_RMVTIMSER_WHAT, 0, removetimeseriesStrategies);
			AddClusterAssoc(CTL_RMVTIMSER_WHAT, 1, removetimeseriesTimSer);
			AddClusterAssoc(CTL_RMVTIMSER_WHAT, 2, removetimeseriesCompletely);
			SetClusterData(CTL_RMVTIMSER_WHAT, Data);
			DisableClusterItem(CTL_RMVTIMSER_WHAT, 0, Data & removetimeseriesCompletely);
			DisableClusterItem(CTL_RMVTIMSER_WHAT, 1, Data & removetimeseriesCompletely);
			return ok;
		}
		DECL_DIALOG_GETDTS()
		{
			int    ok = 1;
			GetClusterData(CTL_RMVTIMSER_WHAT, &Data);
			ASSIGN_PTR(pData, Data);
			return ok;
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isClusterClk(CTL_RMVTIMSER_WHAT)) {
				const long preserve_data = Data;
				GetClusterData(CTL_RMVTIMSER_WHAT, &Data);
				if(preserve_data != Data) {
					DisableClusterItem(CTL_RMVTIMSER_WHAT, 0, Data & removetimeseriesCompletely);
					DisableClusterItem(CTL_RMVTIMSER_WHAT, 1, Data & removetimeseriesCompletely);
					if(Data & removetimeseriesCompletely) {
						Data |= (removetimeseriesStrategies|removetimeseriesTimSer);
					}
				}
				clearEvent(event);
			}
		}
	};
	DIALOG_PROC_BODY_P1(RemoveTimeSeriesDialog, pWarnTextSign, pData);
}

int SLAPI PPViewTimeSeries::DeleteItem(PPID id)
{
	int    ok = -1;
	long   flags = 0;
	PPTimeSeries rec;
	if(id && Obj.Search(id, &rec) > 0) {
		if(CallRemoveTimeSeriesDialog(0, &flags) > 0) {
			PPTransaction tra(1);
			THROW(tra);
			if(flags & removetimeseriesCompletely) {
				THROW(Obj.PutPacket(&id, 0, 0));
			}
			else {
				if(flags & removetimeseriesStrategies) {
					THROW(Obj.PutStrategies(id, PPObjTimeSeries::sstAll, 0, 0));
					THROW(Obj.PutStrategies(id, PPObjTimeSeries::sstSelection, 0, 0));
				}
				if(flags & removetimeseriesTimSer) {
					THROW(Obj.SetTimeSeries(id, 0, 0));
				}
			}
			THROW(tra.Commit());
		}
	}
	CATCHZOKPPERR
	return ok;
}

int SLAPI PPViewTimeSeries::DeleteAll()
{
	int    ok = -1;
	long   flags = 0;
	PPTimeSeries rec;
	if(CallRemoveTimeSeriesDialog("removetimeseries_warn", &flags) > 0) {
		TimeSeriesViewItem item;
		PPTransaction tra(1);
		THROW(tra);
		for(InitIteration(); NextIteration(&item) > 0;) {
			if(flags & removetimeseriesCompletely) {
				THROW(Obj.PutPacket(&item.ID, 0, 0));
			}
			else {
				if(flags & removetimeseriesStrategies) {
					THROW(Obj.PutStrategies(item.ID, PPObjTimeSeries::sstAll, 0, 0));
					THROW(Obj.PutStrategies(item.ID, PPObjTimeSeries::sstSelection, 0, 0));
				}
				if(flags & removetimeseriesTimSer) {
					THROW(Obj.SetTimeSeries(item.ID, 0, 0));
				}
			}
		}
		THROW(tra.Commit());
	}
	CATCHZOKPPERR
	return ok;
}

int SLAPI PPViewTimeSeries::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = -2;
	PPID   id = pHdr ? *static_cast<const PPID *>(pHdr) : 0;
	const  PPID preserve_id = id;
	if(ppvCmd == PPVCMD_DELETEITEM) { // Перехват обработки команды до PPView::ProcessCommand
		ok = DeleteItem(id);
	}
	else {
		ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	}
	if(ok == -2) {
		switch(ppvCmd) {
			case PPVCMD_USERSORT:
				ok = 1; // The rest will be done below
				break;
			case PPVCMD_DELETEALL:
				ok = DeleteAll();
				break;
			case PPVCMD_CONFIG:
				ok = -1;
				if(PPObjTimeSeries::EditConfig(0) > 0) {
					PPObjTimeSeries::ReadConfig(&Cfg);
					ok = 1;
				}
				break;
			case PPVCMD_EXPORT:
				ok = -1;
				Obj.Export(id);
				break;
			//case PPVCMD_TRYSTRATEGIES:
			case PPVCMD_TEST:
				ok = -1;
				if(id) {
					PPObjTimeSeries::TryStrategies(id);
				}
				break;
			case PPVCMD_EVALOPTMAXDUCK:
				ok = -1;
				if(id) {
					if(PPObjTimeSeries::EvaluateOptimalMaxDuck(id) > 0)
						ok = 1;
				}
				break;
			case PPVCMD_REFRESH:
				ok = 1;
				break;
		}
	}
	if(ok > 0) {
		MakeList(pBrw);
		if(pBrw) {
			AryBrowserDef * p_def = static_cast<AryBrowserDef *>(pBrw->getDef());
			if(p_def) {
				SArray * p_array = new SArray(*P_DsList);
				p_def->setArray(p_array, 0, 1);
				pBrw->setRange(p_array->getCount());
				uint   temp_pos = 0;
				long   update_pos = -1;
				if(preserve_id > 0 && P_DsList->lsearch(&preserve_id, &temp_pos, CMPF_LONG))
					update_pos = temp_pos;
				if(update_pos >= 0)
					pBrw->go(update_pos);
				else if(update_pos == MAXLONG)
					pBrw->go(p_array->getCount()-1);
			}
			pBrw->Update();
		}
	}
	return ok;
}

int SLAPI PPObjTimeSeries::Export(PPID id)
{
	int   ok = 1;
	PPTimeSeries ts_rec;
	if(Search(id, &ts_rec) > 0) {
		SString temp_buf;
		SString file_name;
		SString line_buf;
		temp_buf.Z().Cat("ts-raw").CatChar('-').Cat(ts_rec.Symb).Dot().Cat("txt").ToLower();
		PPGetFilePath(PPPATH_OUT, temp_buf, file_name);
		SFile f_out(file_name, SFile::mWrite);
		if(f_out.IsValid()) {
			STimeSeries ts;
			StrategyContainer scontainer;
			if(GetTimeSeries(id, ts) > 0) {
				const uint tsc = ts.GetCount();
				uint vec_idx = 0;
				if(ts.GetValueVecIndex("close", &vec_idx)) {
					LDATETIME t_prev = ZERODATETIME;
					for(uint j = 0; j < tsc; j++) {
						LDATETIME t = ZERODATETIME;
						double v = 0;
						SUniTime ut;
						ts.GetTime(j, &ut);
						ut.Get(t);
						ts.GetValue(j, vec_idx, &v);
						long td = j ? diffdatetimesec(t, t_prev) : 0;
						line_buf.Z().Cat(t, DATF_ISO8601|DATF_CENTURY, 0).Tab().Cat(v, MKSFMTD(10, 5, 0)).Tab().Cat(td);
						f_out.WriteLine(line_buf.CR());
						t_prev = t;
					}
				}
				const PPObjTimeSeries::StrategySetType sst_list[] = { PPObjTimeSeries::sstAll, PPObjTimeSeries::sstSelection };
				for(uint sst_idx = 0; sst_idx < SIZEOFARRAY(sst_list); sst_idx++) {
					if(GetStrategies(id, sst_list[sst_idx], scontainer) > 0) {
						PPObjTimeSeries::GetStrategySetTypeName(sst_list[sst_idx], line_buf);
						f_out.WriteLine(line_buf.CR());
						for(uint i = 0; i < scontainer.getCount(); i++) {
							const Strategy & r_s = scontainer.at(i);
							line_buf.Z().
								CatEq("ID", r_s.ID).Space().
								CatEq("InputFrameSize", r_s.InputFrameSize).Space().
								CatEq("Prec", static_cast<long>(r_s.Prec)).Space().
								//CatEq("TargetQuant", r_s.TargetQuant).Space().
								CatEq("MaxDuckQuant", r_s.MaxDuckQuant).Space().
								CatEq("PeakAvgQuant", static_cast<long>(r_s.PeakAvgQuant)).Space().
								CatEq("PeakMaxQuant", static_cast<long>(r_s.PeakMaxQuant)).Space().
								CatEq("BottomAvgQuant", static_cast<long>(r_s.BottomAvgQuant)).Space().
								CatEq("StakeMode", static_cast<long>(r_s.StakeMode)).Space().
								CatEq("BaseFlags", r_s.BaseFlags).Space().
								CatEq("Margin", r_s.Margin).Space().
								CatEq("SpikeQuant", r_s.SpikeQuant).Space().
								CatEq("SpreadAvg", r_s.SpreadAvg).Space().
								CatEq("TrendErrAvg", r_s.TrendErrAvg, MKSFMTD(0, 6, 0)).Space(). // @v10.3.12
								CatEq("TrendErrLim", r_s.TrendErrLim, MKSFMTD(0, 1, 0)).Space() // @v10.3.12
								// @v10.3.12 CatEq("StakeThreshold", r_s.StakeThreshold).Space()
								;
							temp_buf.Z().Cat(r_s.OptDeltaRange.low, MKSFMTD(0, 9, 0)).Dot().Dot().Cat(r_s.OptDeltaRange.upp, MKSFMTD(0, 9, 0));
							line_buf.CatEq("OptDeltaRange", temp_buf).Space().
								CatEq("OptDeltaCount", r_s.OptDeltaRange.Count).Space();
							line_buf.CatEq("OptDelta2Stride", r_s.OptDelta2Stride).Space();
							temp_buf.Z().Cat(r_s.OptDelta2Range.low, MKSFMTD(0, 9, 0)).Dot().Dot().Cat(r_s.OptDelta2Range.upp, MKSFMTD(0, 9, 0));
							line_buf.CatEq("OptDelta2Range", temp_buf).Space().
								CatEq("OptDelta2Count", r_s.OptDelta2Range.Count).Space().
								CatEq("StakeCloseMode", r_s.StakeCloseMode).Space().
								CatEq("StakeCount", r_s.StakeCount).Space().
								CatEq("V.Result", r_s.V.Result, MKSFMTD(0, 8, 0)).Space().
								CatEq("V.TmCount", r_s.V.TmCount).Space().
								CatEq("V.TmSec", r_s.V.TmSec);
							f_out.WriteLine(line_buf.CR());
						}
					}
				}
			}
		}
	}
	return ok;
}

int SLAPI PPObjTimeSeries::Browse(void * extraPtr)
{
	return PPView::Execute(PPVIEW_TIMESERIES, 0, 1, 0);
}

int SLAPI PPObjTimeSeries::Test() // @experimental
{
	int    ok = 1;
	SString temp_buf;
	SString src_file_name;
	SString test_file_name;
	SLS.QueryPath("testroot", src_file_name);
	src_file_name.SetLastSlash().Cat("data").SetLastSlash().Cat("ts").SetLastSlash().Cat("test-symb-export-eurusd.csv");
	SFile f_in(src_file_name, SFile::mRead);
	if(f_in.IsValid()) {
		test_file_name = src_file_name;
		SPathStruc::ReplaceExt(test_file_name, "out", 1);
		SString line_buf;
		StringSet ss_in(",");
		STimeSeries ts;
		LDATETIME dtm;
		double open = 0.0;
		double close = 0.0;
		long   tick_vol = 0;
		long   real_vol = 0;
		long   spread = 0;
		//uint   vecidx_open = 0;
		uint   vecidx_close = 0;
		//uint   vecidx_ticvol = 0;
		uint   vecidx_realvol = 0;
		//uint   vecidx_spread = 0;
		//THROW_SL(ts.AddValueVec("open", T_DOUBLE, 0, &vecidx_open));
		//THROW_SL(ts.AddValueVec("open", T_INT32, 5, &vecidx_open));
		//THROW_SL(ts.AddValueVec("close", T_DOUBLE, 0, &vecidx_close));
		THROW_SL(ts.AddValueVec("close", T_INT32, 5, &vecidx_close));
		//THROW_SL(ts.AddValueVec("tick_volume", T_INT32, 0, &vecidx_ticvol));
		THROW_SL(ts.AddValueVec("real_volume", T_INT32, 0, &vecidx_realvol));
		//THROW_SL(ts.AddValueVec("spread", T_INT32, 0, &vecidx_spread));
		{
			uint8 sign[8];
			size_t actual_size = 0;
			if(f_in.Read(sign, 4, &actual_size) && actual_size == 4) {
				if(sign[0] == 0xEF && sign[1] == 0xBB && sign[2] == 0xBF)
					f_in.Seek(3);
				else
					f_in.Seek(0);
			}
		}
		while(f_in.ReadLine(line_buf)) {
			line_buf.Chomp().Strip();
			if(line_buf.NotEmpty()) {
				ss_in.setBuf(line_buf);
				dtm.Z();
				open = 0.0;
				close = 0.0;
				tick_vol = 0;
				real_vol = 0;
				spread = 0;
				for(uint ssp = 0, fldn = 0; ss_in.get(&ssp, temp_buf); fldn++) {
					switch(fldn) {
						case 0: strtodate(temp_buf, DATF_YMD, &dtm.d); break;
						case 1: strtotime(temp_buf, TIMF_HMS, &dtm.t); break;
						case 2: open = temp_buf.ToReal(); break;
						case 3: close = temp_buf.ToReal(); break;
						case 4: tick_vol = temp_buf.ToLong(); break;
						case 5: real_vol = temp_buf.ToLong(); break;
						case 6: spread = temp_buf.ToLong(); break;
					}
				}
				if(checkdate(&dtm) && close > 0.0) {
					SUniTime ut;
					ut.Set(dtm, SUniTime::indMin);
					uint   item_idx = 0;
					THROW_SL(ts.AddItem(ut, &item_idx));
					//THROW_SL(ts.SetValue(item_idx, vecidx_open, open));
					THROW_SL(ts.SetValue(item_idx, vecidx_close, close));
					//THROW_SL(ts.SetValue(item_idx, vecidx_ticvol, tick_vol));
					THROW_SL(ts.SetValue(item_idx, vecidx_realvol, real_vol));
					//THROW_SL(ts.SetValue(item_idx, vecidx_spread, spread));
				}
			}
		}
		{
			//SFile f_out(test_file_name, SFile::mWrite);
			//
			{
				STimeSeries dts;
				SBuffer sbuf; // serialize buf
				SBuffer cbuf; // compress buf
				SBuffer dbuf; // decompress buf
				SSerializeContext sctx;
				THROW_SL(ts.Serialize(+1, sbuf, &sctx));
				{
					{
						SCompressor c(SCompressor::tZLib);
						THROW_SL(c.CompressBlock(sbuf.GetBuf(sbuf.GetRdOffs()), sbuf.GetAvailableSize(), cbuf, 0, 0));
					}
					{
						SCompressor c(SCompressor::tZLib);
						THROW_SL(c.DecompressBlock(cbuf.GetBuf(cbuf.GetRdOffs()), cbuf.GetAvailableSize(), dbuf));
					}
					THROW_SL(dts.Serialize(-1, dbuf, &sctx));
				}
				dts.GetCount();
				THROW(dts.Sort());
				{
					SFile f_out(test_file_name, SFile::mWrite);
					THROW(f_out.IsValid());
					for(uint i = 0; i < dts.GetCount(); i++) {
						SUniTime ut;
						dts.GetTime(i, &ut);
						ut.Get(dtm);
						//THROW(dts.GetValue(i, vecidx_open, &open));
						THROW(dts.GetValue(i, vecidx_close, &close));
						//THROW(dts.GetValue(i, vecidx_ticvol, &tick_vol));
						THROW(dts.GetValue(i, vecidx_realvol, &real_vol));
						//THROW(dts.GetValue(i, vecidx_spread, &spread));
						line_buf.Z().Cat(dtm.d, DATF_ANSI|DATF_CENTURY).Comma().Cat(dtm.t, TIMF_HM).Comma().
							/*Cat(open, MKSFMTD(0, 5, 0)).Comma().*/Cat(close, MKSFMTD(0, 5, 0)).Comma()./*Cat(tick_vol).Comma().*/Cat(real_vol)/*.Comma().Cat(spread)*/.CR();
						THROW(f_out.WriteLine(line_buf));
					}
				}
			}
			{
				PPID    id = 0;
				PPTimeSeriesPacket ts_pack;
				STimeSeries ex_ts;
				int gts = 0;
				if(SearchBySymb("eurusd", &id, &ts_pack.Rec) > 0) {
					gts = GetTimeSeries(id, ex_ts);
				}
				else {
					STRNSCPY(ts_pack.Rec.Name, "eurusd");
					STRNSCPY(ts_pack.Rec.Symb, "eurusd");
					THROW(PutPacket(&id, &ts_pack, 1));
				}
				THROW(SetTimeSeries(id, &ts, 1));
			}
		}
	}
	CATCHZOK
	return ok;
}

static int SLAPI AnalyzeTsTradeFrame(const STimeSeries & rTs, uint frameSize, double target, double * pFreq, double * pMaxDuckAvg)
{
	int    ok = -1;
	const  uint tsc = rTs.GetCount();
	RealArray frame;
	uint   frame_count = 0;
	uint   target_count = 0;
	StatBase max_duck_stat(0);
	const double zero = 0.0;
	for(uint fs = 0; fs < (tsc-frameSize); fs++) {
		rTs.GetFrame("close", fs, frameSize, STimeSeries::nfOne|STimeSeries::nfBaseStart, frame);
		int   target_reached = 0;
		double max_duck = 1.0;
		if(frame.getCount() == frameSize) {
			for(uint i = 0; !target_reached && i < frame.getCount(); i++) {
				const double fv = frame.at(i);
				SETMIN(max_duck, fv);
				if(fv > target)
					target_reached = 1;
			}
			frame_count++;
			ok = 1;
		}
		if(target_reached) {
			max_duck_stat.Step(max_duck);
			target_count++;
		}
	}
	max_duck_stat.Finish();
	ASSIGN_PTR(pFreq, ((double)target_count) / ((double)frame_count));
	ASSIGN_PTR(pMaxDuckAvg, max_duck_stat.GetExp());
	return ok;
}

struct StrategyOptEntry {
	SLAPI  StrategyOptEntry(double factor, double result) : Factor(factor), Result1(result)
	{
	}
	SLAPI  StrategyOptEntry(double factor, double result, double result2) : Factor(factor), Result1(result), Result2(result2)
	{
	}
	double Factor;
	double Result1;
	double Result2;
};

IMPL_CMPFUNC(PPObjTimeSeries_OptimalFactorRange_Result, p1, p2)
{
	const PPObjTimeSeries::OptimalFactorRange * p_i1 = static_cast<const PPObjTimeSeries::OptimalFactorRange *>(p1);
	const PPObjTimeSeries::OptimalFactorRange * p_i2 = static_cast<const PPObjTimeSeries::OptimalFactorRange *>(p2);
	return CMPSIGN(p_i1->Result, p_i2->Result);
}

static double CalcFactorRangeResultSum(const TSVector <StrategyOptEntry> & rList, uint firstIdx, uint lastIdx) // CalcFactorRangeResult_Func
{
	return rList.sumDouble(offsetof(StrategyOptEntry, Result1), firstIdx, lastIdx);
}

static double CalcFactorRangeResult_R1DivR2(const TSVector <StrategyOptEntry> & rList, uint firstIdx, uint lastIdx) // CalcFactorRangeResult_Func
{
	double s1 = rList.sumDouble(offsetof(StrategyOptEntry, Result1), firstIdx, lastIdx);
	double s2 = rList.sumDouble(offsetof(StrategyOptEntry, Result2), firstIdx, lastIdx);
	return fdivnz(s1, s2) /* * (lastIdx - firstIdx + 1) */;
}

static double CalcFactorRangeResult_R2DivC(const TSVector <StrategyOptEntry> & rList, uint firstIdx, uint lastIdx) // CalcFactorRangeResult_Func
{
	double s2 = rList.sumDouble(offsetof(StrategyOptEntry, Result2), firstIdx, lastIdx);
	double c = static_cast<double>(lastIdx - firstIdx + 1);
	return fdivnz(s2, c) /* * (lastIdx - firstIdx + 1) */;
}

static double CalcFactorRangeResult2Sum(const TSVector <StrategyOptEntry> & rList, const TSVector <IntRange> & rPosRangeList) // CalcFactorRangeResult2_Func
{
	double result = 0.0;
	for(uint i = 0; i < rPosRangeList.getCount(); i++) {
		const IntRange & r_range = rPosRangeList.at(i);
		result += rList.sumDouble(offsetof(StrategyOptEntry, Result1), r_range.low, r_range.upp);
	}
	return result;
}

static double CalcFactorRangeResult2_R1DivR2(const TSVector <StrategyOptEntry> & rList, const TSVector <IntRange> & rPosRangeList) // CalcFactorRangeResult2_Func
{
	double s1 = 0.0;
	double s2 = 0.0;
	for(uint i = 0; i < rPosRangeList.getCount(); i++) {
		const IntRange & r_range = rPosRangeList.at(i);
		s1 += rList.sumDouble(offsetof(StrategyOptEntry, Result1), r_range.low, r_range.upp);
		s2 += rList.sumDouble(offsetof(StrategyOptEntry, Result2), r_range.low, r_range.upp);
	}
	return fdivnz(s1, s2) /* * (lastIdx - firstIdx + 1) */;
}

static double CalcFactorRangeResult2_R2DivC(const TSVector <StrategyOptEntry> & rList, const TSVector <IntRange> & rPosRangeList) // CalcFactorRangeResult2_Func
{
	double s2 = 0.0;
	double c = 0.0;
	for(uint i = 0; i < rPosRangeList.getCount(); i++) {
		const IntRange & r_range = rPosRangeList.at(i);
		//s1 += rList.sumDouble(offsetof(StrategyOptEntry, Result1), r_range.low, r_range.upp);
		s2 += rList.sumDouble(offsetof(StrategyOptEntry, Result2), r_range.low, r_range.upp);
		c += static_cast<double>(r_range.upp - r_range.low + 1);
	}
	return fdivnz(s2, c) /* * (lastIdx - firstIdx + 1) */;
}

/*static void SLAPI PrepareStrategyOptList(TSVector <StrategyOptEntry> & rList, TSVector <StrategyOptEntry> & rNegList, TSVector <StrategyOptEntry> & rPosList)
{
	//rList.sort(PTR_CMPFUNC(double));
	rNegList.clear();
	rPosList.clear();
	const  uint _c = rList.getCount();
	for(uint i = 0; i < _c; i++) {
		const StrategyOptEntry & r_entry = rList.at(i);
		if(r_entry.Factor < 0.0)
			rNegList.insert(&r_entry);
		else
			rPosList.insert(&r_entry);
	}
	rNegList.sort(PTR_CMPFUNC(double));
	rPosList.sort(PTR_CMPFUNC(double));
}*/

typedef double (* CalcFactorRangeResult_Func)(const TSVector <StrategyOptEntry> & rList, uint firstIdx, uint lastIdx);
typedef double (* CalcFactorRangeResult2_Func)(const TSVector <StrategyOptEntry> & rList, const TSVector <IntRange> & rPosRangeList);
//
// useInitialSplitting:
//   0 - первая итерация, как и последующие, делит список на две равные части
//   1 - первая итерация делит список на положительную и отрицательную части
//   2 - первая итерация запускает обработку только положительных значений списка
//   3 - первая итерация запускает обработку только отрицательных значений списка
//   4 - последовательное сканирование всего диапазона фреймами для нахождения максимального
//   5 - последовательное сканирование всего диапазона фреймами для нахождения максимального (только положительные значения)
//   6 - последовательное сканирование всего диапазона фреймами для нахождения максимального (только отрицательные значения)
//
static int SLAPI FindOptimalFactorRange(const PrcssrTsStrategyAnalyze::ModelParam & rMp, const TSVector <StrategyOptEntry> & rList, int useInitialSplitting, PPObjTimeSeries::OptimalFactorRange & rR)
{
	int    ok = -1;
	CalcFactorRangeResult_Func cfrrFunc = 0; //(rMp.Flags & rMp.fOptRangeTarget_Velocity) ? CalcFactorRangeResult_R1DivR2 : CalcFactorRangeResultSum;
	if(rMp.OptTargetCriterion == rMp.tcAmount)
		cfrrFunc = CalcFactorRangeResultSum;
	else if(rMp.OptTargetCriterion == rMp.tcVelocity)
		cfrrFunc = CalcFactorRangeResult_R1DivR2;
	else if(rMp.OptTargetCriterion == rMp.tcWinRatio)
		cfrrFunc = CalcFactorRangeResult_R2DivC;
	assert(cfrrFunc != 0);
	const  uint _c = rList.getCount();
	rR.Z();
	//rRangeCount = 0;
	//rResult = 0.0;
	// interval_exp_direction: 0 - не раздвигать интервалы, 1 - только расширять, 2 - только сжимать, 3 - расширять и сжимать
	const  int  interval_exp_direction = 1; // @20190402 // @20190413 (3 : 3)-->(3 : 1) // @20190421 (3 : 1)-->(2 : 1)
	const  uint maxprobe = 1; // @v10.3.5 20-->30 // @v10.3.6 30-->100 // @v10.3.8 100-->200 // @v10.3.9 200-->50 // @20190413 50-->15 // @20190421 15-->24
		// @20190426 24-->1
	//const  uint sfdelta  = 100; // @v10.4.0 Шаг итерации при последовательном поиске оптимального диапазона (useInitialSplitting == 4)
	double _sfd_max_result = 0.0;
	uint   _sfd_max_loidx = 0;
	uint   _sfd_max_upidx = 0;
	uint   _sfd_best = 0;
	assert(oneof3(useInitialSplitting, 4, 5, 6));
	if(_c > 1 && oneof3(useInitialSplitting, 4, 5, 6)) {
		const  double total_sum = cfrrFunc(rList, 0, _c-1);
		//const uint sf_step = 288; // @20190417 50-->48 // @20190426 48-->144 // @20190429 144-->288
		uint sf_step = 0;
		if(rMp.OptRangeStepMkPart > 0 && rMp.OptRangeStepMkPart <= 1000000)
			sf_step = ffloori(static_cast<double>(_c) * static_cast<double>(rMp.OptRangeStepMkPart) / 1000000.0);
		else
			sf_step = (rMp.OptRangeStep > 0) ? rMp.OptRangeStep : 288;
		const uint sf_limit = sf_step * 3; // @20190413 (<=1000)-->(<=1200) // @20190421 (*25)-->(*30) // @20190425 (*30)-->(*40) // @20190426 40-->5
			// @20190429 (*5)-->(*3)
		for(uint sfdelta = sf_step; sfdelta <= sf_limit; sfdelta += sf_step) {
			int    do_break = 0; // Сигнал для выхода из функции из-за того, что приемлемых значений больше нет
			// (must be done by caller) rList.sort(PTR_CMPFUNC(double));
			double prev_result = total_sum;
			uint   lo_idx = 0;
			uint   up_idx = _c-1;
			uint   iter_no = 0;
			double _max_result = 0.0;
			uint   _max_sfidx = 0;
			uint   _max_sfidx_up = 0;
			uint   _first_idx = 0;
			uint   _last_idx = _c-1;
			if(oneof2(useInitialSplitting, 5, 6)) {
				if(rList.at(0).Factor >= 0.0 && useInitialSplitting == 6) {
					_last_idx = 0;
					do_break = 1; // nothing to do
				}
				else if(rList.at(_c-1).Factor <= 0.0 && useInitialSplitting == 5) {
					_last_idx = 0;
					do_break = 1; // nothing to do
				}
				else {
					for(uint i = 0; i < _c; i++) {
						const StrategyOptEntry & r_entry = rList.at(i);
						if(useInitialSplitting == 5 && r_entry.Factor > 0.0) {
							_first_idx = i;
							break;
						}
						else if(useInitialSplitting == 6 && r_entry.Factor >= 0.0) {
							assert(i > 0);
							_last_idx = i-1;
							break;
						}
					}
				}
			}
			for(uint sfidx = _first_idx; sfidx < (_last_idx-sfdelta); sfidx += (sfdelta*1/16)) { // @20190413 (+=sfdelta)-->(+=(sfdelta*3/4)) // @20190421 (+=(sfdelta*3/4))-->(+=(sfdelta*1/4))
				// @20190425 (+=(sfdelta*1/4))-->(+=(sfdelta*1/8)) // @20190427 (+=(sfdelta*1/8))-->(+=(sfdelta*1/16))
				uint sfidx_up = MIN(sfidx+sfdelta-1, _last_idx);
				if(sfidx_up > sfidx) {
					const double _result = cfrrFunc(rList, sfidx, sfidx_up);
					if(_result > _max_result) {
						_max_result = _result;
						_max_sfidx = sfidx;
						_max_sfidx_up = sfidx_up;
					}
				}
				else
					break;
			}
			if(_max_result > 0.0) {
				lo_idx = _max_sfidx;
				up_idx = _max_sfidx_up;
			}
			if(!do_break) {
				if(interval_exp_direction & 1) { // расширять интервал
					while(lo_idx > 0) {
						int done = 0;
						for(uint probedeep = 1; !done && probedeep <= maxprobe && probedeep <= lo_idx; probedeep++) { // @v10.3.12 @fix (probedeep > lo_idx)-->(probedeep <= lo_idx)
							const double temp_result = cfrrFunc(rList, lo_idx-probedeep, up_idx);
							if(prev_result < temp_result) {
								prev_result = temp_result;
								lo_idx -= probedeep;
								done = 1;
							}
						}
						if(!done)
							break;
					}
					while((up_idx+1) < _c) {
						int done = 0;
						for(uint probedeep = 1; !done && probedeep <= maxprobe && (up_idx+probedeep) < _c; probedeep++) {
							const double temp_result = cfrrFunc(rList, lo_idx, up_idx+probedeep);
							if(prev_result < temp_result) {
								prev_result = temp_result;
								up_idx += probedeep;
								done = 1;
							}
						}
						if(!done)
							break;
					}
				}
				if(interval_exp_direction & 2) { // сжимать интервал
					while(lo_idx < up_idx) { // Сдвигаем нижнюю границу вверх
						int done = 0;
						for(uint probedeep = 1; !done && probedeep <= maxprobe && (lo_idx+probedeep) <= up_idx; probedeep++) {
							const double temp_result = cfrrFunc(rList, lo_idx+probedeep, up_idx);
							if(prev_result < temp_result) {
								prev_result = temp_result;
								lo_idx += probedeep;
								done = 1;
							}
						}
						if(!done)
							break;
					}
					while(lo_idx < up_idx) { // Сдвигаем верхнюю границу вниз
						int done = 0;
						for(uint probedeep = 1; !done && probedeep <= maxprobe && (up_idx) >= (lo_idx+probedeep); probedeep++) {
							const double temp_result = cfrrFunc(rList, lo_idx, up_idx-probedeep);
							if(prev_result < temp_result) {
								prev_result = temp_result;
								up_idx -= probedeep;
								done = 1;
							}
						}
						if(!done)
							break;
					}
				}
				if(_sfd_max_result < prev_result) {
					_sfd_max_result = prev_result;
					_sfd_max_loidx = lo_idx;
					_sfd_max_upidx = up_idx;
					_sfd_best = sfdelta;
				}
			}
		}
		if(_sfd_max_result > 0.0) {
			rR.Set(rList.at(_sfd_max_loidx).Factor, rList.at(_sfd_max_upidx).Factor);
			rR.Result = _sfd_max_result;
			rR.Count = (_sfd_max_upidx - _sfd_max_loidx + 1);
			if(_sfd_max_loidx > 0 || _sfd_max_upidx < (_c-1))
				ok = 1;
		}
	}
	return ok;
}

static int FASTCALL IsTherIntRangeIntersection(const TSVector <IntRange> & rList, const IntRange & rR)
{
	for(uint i = 0; i < rList.getCount(); i++) {
		if(rR.Intersect(rList.at(i), 0))
			return 1;
	}
	return 0;
}

static int SLAPI FindOptimalFactorRange2(const PrcssrTsStrategyAnalyze::ModelParam & rMp,
	const TSVector <StrategyOptEntry> & rList, int useInitialSplitting, TSVector<PPObjTimeSeries::OptimalFactorRange> & rRc)
{
	int    ok = -1;
	const  uint _c = rList.getCount();
	CalcFactorRangeResult2_Func cfrrFunc = 0; //(rMp.Flags & rMp.fOptRangeTarget_Velocity) ? CalcFactorRangeResult2_R1DivR2 : CalcFactorRangeResult2Sum;
	if(rMp.OptTargetCriterion == rMp.tcAmount)
		cfrrFunc = CalcFactorRangeResult2Sum;
	else if(rMp.OptTargetCriterion == rMp.tcVelocity)
		cfrrFunc = CalcFactorRangeResult2_R1DivR2;
	else if(rMp.OptTargetCriterion == rMp.tcWinRatio)
		cfrrFunc = CalcFactorRangeResult2_R2DivC;
	assert(cfrrFunc != 0);
	rRc.clear();
	// interval_exp_direction: 0 - не раздвигать интервалы, 1 - только расширять, 2 - только сжимать, 3 - расширять и сжимать
	const  int  interval_exp_direction = 1; // @20190402 // @20190413 (3 : 3)-->(3 : 1) // @20190421 (3 : 1)-->(2 : 1)
	const  uint maxprobe = 1; // @v10.3.5 20-->30 // @v10.3.6 30-->100 // @v10.3.8 100-->200 // @v10.3.9 200-->50 // @20190413 50-->15 // @20190421 15-->24
		// @20190426 24-->1
	//const  uint sfdelta  = 100; // @v10.4.0 Шаг итерации при последовательном поиске оптимального диапазона (useInitialSplitting == 4)
	assert(oneof3(useInitialSplitting, 4, 5, 6));
	if(_c > 1 && oneof3(useInitialSplitting, 4, 5, 6)) {
		double total_max_result = 0.0;
		TSVector <IntRange> pos_range_list;
		TSVector <IntRange> work_pos_range_list;
		IntRange work_range;
		work_range.Set(0, _c-1);
		work_pos_range_list.clear();
		work_pos_range_list.insert(&work_range);
		const  double total_sum = cfrrFunc(rList, work_pos_range_list);
		uint   _first_idx = 0;
		uint   _last_idx = _c-1;
		int    do_break = 0; // Сигнал для выхода из функции из-за того, что приемлемых значений больше нет
		if(oneof2(useInitialSplitting, 5, 6)) {
			if(rList.at(0).Factor >= 0.0 && useInitialSplitting == 6) {
				_last_idx = 0;
				do_break = 1; // nothing to do
			}
			else if(rList.at(_c-1).Factor <= 0.0 && useInitialSplitting == 5) {
				_last_idx = 0;
				do_break = 1; // nothing to do
			}
			else {
				for(uint i = 0; i < _c; i++) {
					const StrategyOptEntry & r_entry = rList.at(i);
					if(useInitialSplitting == 5 && r_entry.Factor > 0.0) {
						_first_idx = i;
						break;
					}
					else if(useInitialSplitting == 6 && r_entry.Factor >= 0.0) {
						assert(i > 0);
						_last_idx = i-1;
						break;
					}
				}
			}
		}
		if(!do_break) {
			const uint _real_item_count = (_last_idx - _first_idx + 1);
			// @v10.4.7 const uint sf_step = 288; // @20190417 50-->48 // @20190426 48-->144 // @20190429 144-->288
			uint   sf_step = 0;
			if(rMp.OptRangeStepMkPart > 0 && rMp.OptRangeStepMkPart <= 1000000)
				sf_step = ffloori(static_cast<double>(_real_item_count) * static_cast<double>(rMp.OptRangeStepMkPart) / 1000000.0);
			else
				sf_step = (rMp.OptRangeStep > 0) ? rMp.OptRangeStep : 288;
			const uint sf_limit = sf_step * 3; // @20190413 (<=1000)-->(<=1200) // @20190421 (*25)-->(*30) // @20190425 (*30)-->(*40) // @20190426 40-->5
				// @20190429 (*5)-->(*3)
			const uint max_range_count = (rMp.OptRangeMultiLimit > 0) ? rMp.OptRangeMultiLimit : 6;
			for(int do_next_iter = 1; do_next_iter;) {
				do_next_iter = 0;
				double _sfd_max_result = 0.0;
				uint   _sfd_max_loidx = 0;
				uint   _sfd_max_upidx = 0;
				uint   _sfd_best = 0;
				for(uint sfdelta = sf_step; sfdelta <= sf_limit; sfdelta += sf_step) {
					// (must be done by caller) rList.sort(PTR_CMPFUNC(double));
					double prev_result = total_sum;
					uint   lo_idx = 0;
					uint   up_idx = _c-1;
					uint   iter_no = 0;
					double _max_result = 0.0;
					uint   _max_sfidx = 0;
					uint   _max_sfidx_up = 0;
					for(uint sfidx = _first_idx; sfidx < (_last_idx-sfdelta); sfidx += 3/*(sfdelta*1/16)*/) {
						uint sfidx_up = MIN(sfidx+sfdelta-1, _last_idx);
						if(sfidx_up > sfidx) {
							work_range.Set(sfidx, sfidx_up);
							if(!IsTherIntRangeIntersection(pos_range_list, work_range)) {
								// @v10.4.8 work_pos_range_list = pos_range_list;
								work_pos_range_list.clear(); // @v10.4.8
								work_pos_range_list.insert(&work_range);
								const double _result = cfrrFunc(rList, work_pos_range_list);
								if(_result > _max_result) {
									_max_result = _result;
									_max_sfidx = sfidx;
									_max_sfidx_up = sfidx_up;
								}
							}
						}
						else
							break;
					}
					if(_max_result > 0.0) {
						lo_idx = _max_sfidx;
						up_idx = _max_sfidx_up;
					}
					if(interval_exp_direction & 1) { // расширять интервал
						while(lo_idx > 0) {
							int done = 0;
							for(uint probedeep = 1; !done && probedeep <= maxprobe && probedeep <= lo_idx; probedeep++) { // @v10.3.12 @fix (probedeep > lo_idx)-->(probedeep <= lo_idx)
								work_range.Set(lo_idx-probedeep, up_idx);
								if(!IsTherIntRangeIntersection(pos_range_list, work_range)) {
									// @v10.4.8 work_pos_range_list = pos_range_list;
									work_pos_range_list.clear(); // @v10.4.8
									work_pos_range_list.insert(&work_range);
									const double temp_result = cfrrFunc(rList, work_pos_range_list);
									if(prev_result < temp_result) {
										prev_result = temp_result;
										lo_idx -= probedeep;
										done = 1;
									}
								}
							}
							if(!done)
								break;
						}
						while((up_idx+1) < _c) {
							int done = 0;
							for(uint probedeep = 1; !done && probedeep <= maxprobe && (up_idx+probedeep) < _c; probedeep++) {
								work_range.Set(lo_idx, up_idx+probedeep);
								if(!IsTherIntRangeIntersection(pos_range_list, work_range)) {
									// @v10.4.8 work_pos_range_list = pos_range_list;
									work_pos_range_list.clear(); // @v10.4.8
									work_pos_range_list.insert(&work_range);
									const double temp_result = cfrrFunc(rList, work_pos_range_list);
									if(prev_result < temp_result) {
										prev_result = temp_result;
										up_idx += probedeep;
										done = 1;
									}
								}
							}
							if(!done)
								break;
						}
					}
					if(interval_exp_direction & 2) { // сжимать интервал
						while(lo_idx < up_idx) { // Сдвигаем нижнюю границу вверх
							int done = 0;
							for(uint probedeep = 1; !done && probedeep <= maxprobe && (lo_idx+probedeep) <= up_idx; probedeep++) {
								work_range.Set(lo_idx+probedeep, up_idx);
								if(!IsTherIntRangeIntersection(pos_range_list, work_range)) {
									// @v10.4.8 work_pos_range_list = pos_range_list;
									work_pos_range_list.clear(); // @v10.4.8
									work_pos_range_list.insert(&work_range);
									const double temp_result = cfrrFunc(rList, work_pos_range_list);
									if(prev_result < temp_result) {
										prev_result = temp_result;
										lo_idx += probedeep;
										done = 1;
									}
								}
							}
							if(!done)
								break;
						}
						while(lo_idx < up_idx) { // Сдвигаем верхнюю границу вниз
							int done = 0;
							for(uint probedeep = 1; !done && probedeep <= maxprobe && (up_idx) >= (lo_idx+probedeep); probedeep++) {
								work_range.Set(lo_idx, up_idx-probedeep);
								if(!IsTherIntRangeIntersection(pos_range_list, work_range)) {
									// @v10.4.8 work_pos_range_list = pos_range_list;
									work_pos_range_list.clear(); // @v10.4.8
									work_pos_range_list.insert(&work_range);
									const double temp_result = cfrrFunc(rList, work_pos_range_list);
									if(prev_result < temp_result) {
										prev_result = temp_result;
										up_idx -= probedeep;
										done = 1;
									}
								}
							}
							if(!done)
								break;
						}
					}
					if(_sfd_max_result < prev_result) {
						_sfd_max_result = prev_result;
						_sfd_max_loidx = lo_idx;
						_sfd_max_upidx = up_idx;
						_sfd_best = sfdelta;
					}
				}
				if(_sfd_max_result > 0.0 && (_sfd_max_loidx > 0 || _sfd_max_upidx < (_c-1))) {
					int   do_reckon = 0;
					if(total_max_result == 0) {
						total_max_result = _sfd_max_result;
						do_reckon = 1;
					}
					else {
						//if(rMp.Flags & rMp.fOptRangeTarget_Velocity) {
						if(max_range_count)
							do_reckon = 1;
						else if(rMp.OptTargetCriterion == rMp.tcVelocity) {
							if((_sfd_max_result / total_max_result) >= 0.5) {
								// total_max_result не обновляем: нам нужен самый первый результат в качестве такового, ибо он - максимальный (следующие меньше)
								do_reckon = 1;
							}
						}
						else if(rMp.OptTargetCriterion == rMp.tcWinRatio) {
							//if(_sfd_max_result >= 0.65/*total_max_result*/) {
							if((total_max_result / _sfd_max_result) <= 1.1) {
								// total_max_result не обновляем: нам нужен самый первый результат в качестве такового, ибо он - максимальный (следующие меньше)
								do_reckon = 1;
							}
						}
						else if(_sfd_max_result > total_max_result && (_sfd_max_result / total_max_result) >= 1.1) {
							total_max_result = _sfd_max_result;
							do_reckon = 1;
						}
					}
					if(do_reckon) {
						work_range.Set(_sfd_max_loidx, _sfd_max_upidx);
						PPObjTimeSeries::OptimalFactorRange new_range;
						new_range.Set(rList.at(_sfd_max_loidx).Factor, rList.at(_sfd_max_upidx).Factor);
						new_range.Result = _sfd_max_result;
						new_range.Count = (_sfd_max_upidx - _sfd_max_loidx + 1);
						pos_range_list.insert(&work_range);
						rRc.insert(&new_range);
						if(!max_range_count || rRc.getCount() < max_range_count)
							do_next_iter = 1;
						ok = 1;
					}
				}
			}
		}
	}
	return ok;
}

SLAPI PPObjTimeSeries::StrategyResultValue::StrategyResultValue() : Result(0.0), TmCount(0), TmSec(0)
{
}

void FASTCALL PPObjTimeSeries::StrategyResultValue::Append(const StrategyResultValue & rV)
{
	Result += rV.Result;
	TmCount += rV.TmCount;
	TmSec += rV.TmSec;
}

void FASTCALL PPObjTimeSeries::StrategyResultEntry::SetValue(const StrategyResultValue & rV)
{
	V.Append(rV);
	StakeCount++;
	if(rV.Result > 0.0)
		WinCount++;
}

void FASTCALL PPObjTimeSeries::StrategyResultEntry::SetValue(const StrategyResultValueEx & rV)
{
	SetValue(static_cast<const StrategyResultValue &>(rV));
	SumPeak += rV.Peak;
	SumBottom += rV.Bottom;
	SETMAX(MaxPeak, rV.Peak);
}

void FASTCALL PPObjTimeSeries::StrategyResultEntry::SetOuter(const StrategyResultEntry & rS)
{
	ENTER_CRITICAL_SECTION
	V.Append(rS.V);
	StakeCount += rS.StakeCount;
	WinCount += rS.WinCount;
	SumPeak += rS.SumPeak;
	SumBottom += rS.SumBottom;
	SETMAX(MaxPeak, rS.MaxPeak);
	LEAVE_CRITICAL_SECTION
}

PPObjTimeSeries::StrategyResultValue & SLAPI PPObjTimeSeries::StrategyResultValue::Z()
{
	Result = 0.0;
	TmCount = 0;
	TmSec = 0;
	return *this;
}

double SLAPI PPObjTimeSeries::StrategyResultValue::GetResultPerSec() const { return fdivnz(Result, static_cast<double>(TmSec)); }
double SLAPI PPObjTimeSeries::StrategyResultValue::GetResultPerDay() const { return fdivnz(3600.0 * 24.0 * Result, static_cast<double>(TmSec)); }

SLAPI PPObjTimeSeries::StrategyResultValueEx::StrategyResultValueEx() : StrategyResultValue(), Peak(0.0), Bottom(0.0), LastPoint(0), StrategyIdx(-1),
	TrendErr(0.0), TrendErrRel(0.0)
{
}

PPObjTimeSeries::StrategyResultValueEx & SLAPI PPObjTimeSeries::StrategyResultValueEx::Z()
{
	StrategyResultValue::Z();
	Peak = 0.0;
	Bottom = 0.0;
	LastPoint = 0;
	TmR.Z(); // @v10.4.11
	StrategyIdx = -1;
	TrendErr = 0.0; // @v10.4.11
	TrendErrRel = 0.0; // @v10.4.11
	return *this;
}

SLAPI PPObjTimeSeries::StrategyResultEntry::StrategyResultEntry() : Strategy(),
	LastResultIdx(0), /*OptDeltaResult(0.0), OptDelta2Result(0),*/ SumPeak(0.0), SumBottom(0.0), MaxPeak(0.0), TotalSec(0)
{
	PTR32(Symb)[0] = 0;
}

SLAPI PPObjTimeSeries::StrategyResultEntry::StrategyResultEntry(const PPObjTimeSeries::Strategy & rS, int stakeMode) : Strategy(),
	SumPeak(0.0), SumBottom(0.0), MaxPeak(0.0), TotalSec(0), LastResultIdx(0)
{
	PTR32(Symb)[0] = 0;
	InputFrameSize = rS.InputFrameSize;
	Prec = rS.Prec;
	TargetQuant = rS.TargetQuant;
	MaxDuckQuant = rS.MaxDuckQuant;
	OptDelta2Stride = rS.OptDelta2Stride;
	StakeMode = stakeMode;
	BaseFlags = rS.BaseFlags;
	Margin = rS.Margin;
	SpikeQuant = rS.SpikeQuant;
	SpreadAvg = rS.SpreadAvg;
	StakeThreshold = rS.StakeThreshold;
	OptDeltaRange = rS.OptDeltaRange;
	OptDelta2Range = rS.OptDelta2Range;
	PeakAvgQuant = rS.PeakAvgQuant;
	BottomAvgQuant = rS.BottomAvgQuant;
	PeakMaxQuant = rS.PeakMaxQuant;
	TrendErrAvg = rS.TrendErrAvg; // @v10.3.12
	TrendErrLim = rS.TrendErrLim; // @v10.3.12
	MainFrameSize = rS.MainFrameSize; // @v10.4.9
}

SLAPI PPObjTimeSeries::StrategyResultEntry::StrategyResultEntry(const PPObjTimeSeries::TrainNnParam & rTnnp, int stakeMode) : Strategy(),
	SumPeak(0.0), SumBottom(0.0), MaxPeak(0.0), TotalSec(0), LastResultIdx(0)
{
	STRNSCPY(Symb, rTnnp.Symb);
	InputFrameSize = rTnnp.InputFrameSize;
	Prec = rTnnp.Prec;
	TargetQuant = rTnnp.TargetQuant;
	MaxDuckQuant = rTnnp.MaxDuckQuant;
	OptDelta2Stride = rTnnp.OptDelta2Stride;
	StakeMode = stakeMode;
	BaseFlags = rTnnp.BaseFlags;
	Margin = rTnnp.Margin;
	SpikeQuant = rTnnp.SpikeQuant;
	SpreadAvg = rTnnp.SpreadAvg;
	StakeThreshold = rTnnp.StakeThreshold;
	OptDeltaRange = rTnnp.OptDeltaRange;
	OptDelta2Range = rTnnp.OptDelta2Range;
	PeakAvgQuant = rTnnp.PeakAvgQuant;
	BottomAvgQuant = rTnnp.BottomAvgQuant;
	PeakMaxQuant = rTnnp.PeakMaxQuant;
	TrendErrAvg = rTnnp.TrendErrAvg; // @v10.3.12
	TrendErrLim = rTnnp.TrendErrLim; // @v10.3.12
	MainFrameSize = rTnnp.MainFrameSize; // @v10.4.9
}

SLAPI PPObjTimeSeries::TrendEntry::TrendEntry(uint stride, uint nominalCount) : Stride(stride), NominalCount(nominalCount), SpikeQuant(0.0), ErrAvg(0.0)
{
	assert(stride > 0 && stride <= 100000);
	//assert(NominalCount > 0 && NominalCount <= 100);
}

//static
const PPObjTimeSeries::TrendEntry * FASTCALL PPObjTimeSeries::SearchTrendEntry(const TSCollection <PPObjTimeSeries::TrendEntry> & rTrendList, uint stride)
{
	const PPObjTimeSeries::TrendEntry * p_result = 0;
	for(uint i = 0; !p_result && i < rTrendList.getCount(); i++) {
		const PPObjTimeSeries::TrendEntry * p_te = rTrendList.at(i);
		if(p_te && p_te->Stride == stride)
			p_result = p_te;
	}
	return p_result;
}

SLAPI PPObjTimeSeries::BestStrategyBlock::BestStrategyBlock() : MaxResult(0.0), MaxResultIdx(-1), TvForMaxResult(0.0), Tv2ForMaxResult(0.0),
	TrendErr(0.0), TrendErrRel(0.0)
{
}

void SLAPI PPObjTimeSeries::BestStrategyBlock::SetResult(double localResult, uint strategyIdx, double tv, double tv2)
{
	if(MaxResult < localResult) {
		MaxResult = localResult;
		MaxResultIdx = static_cast<int>(strategyIdx);
		TvForMaxResult = tv;
		Tv2ForMaxResult = tv2;
	}
}

//static
int SLAPI PPObjTimeSeries::MatchStrategy(const PPObjTimeSeries::TrendEntry * pTrendEntry, const PPObjTimeSeries::TrendEntry * pMainTrendEntry,
	int lastIdx, const Strategy & rS, double & rResult, double & rWinRatio, double & rTv, double & rTv2)
{
	int   ok = 0;
	const uint tlc = pTrendEntry ? pTrendEntry->TL.getCount() : 0;
	double winratio = 0.0;
	if(tlc && rS.V.TmSec > 0) { // Если rS.V.TmSec == 0, то результат не был вычислен на этапе подготовки модели, либо не встретилось ни одного случая.
		assert(rS.InputFrameSize == pTrendEntry->Stride);
		const uint trend_idx = (lastIdx < 0) ? (tlc-1) : static_cast<uint>(lastIdx);
		const double trend_err = pTrendEntry->ErrL.at(trend_idx); // @v10.3.12
		const double trend_err_limit = (rS.TrendErrLim * rS.TrendErrAvg); // @v10.3.12
		// @v10.4.9 {
		double main_trend_val = 0.0;
		int    skip = 0;
		if(rS.MainFrameSize) {
			assert(rS.MainFrameSize == pMainTrendEntry->Stride);
			if(pMainTrendEntry) {
				main_trend_val = pMainTrendEntry->TL.at(trend_idx);
				skip = rS.OptDelta2Range.Check(main_trend_val);
			}
			else
				skip = 1;
		}
		if(!skip) { // } @v10.4.9
			switch(rS.StakeMode) {
				case 1:
					rTv = pTrendEntry->TL.at(trend_idx);
					if(trend_err_limit <= 0.0 || (trend_err > 0.0 && trend_err <= trend_err_limit)) {
						if(rS.OptDeltaRange.Check(rTv)) {
							rResult = rS.V.GetResultPerDay();
							winratio = rS.GetWinCountRate(); // @v10.4.6
							ok = 1;
						}
					}
					break;
				case 2:
					if(tlc >= rS.OptDelta2Stride) {
						rTv = pTrendEntry->TL.StrideDifference(trend_idx, rS.OptDelta2Stride);
						if(rS.OptDelta2Range.Check(rTv)) {
							rResult = rS.V.GetResultPerDay();
							winratio = rS.GetWinCountRate(); // @v10.4.6
							ok = 1;
						}
					}
					break;
				case 3:
				case 4:
					if(tlc >= rS.OptDelta2Stride) {
						rTv = pTrendEntry->TL.at(trend_idx);
						if(trend_err_limit <= 0.0 || (trend_err > 0.0 && trend_err <= trend_err_limit)) { // @v10.3.12
							if(rS.OptDeltaRange.Check(rTv)) {
								rTv2 = pTrendEntry->TL.StrideDifference(trend_idx, rS.OptDelta2Stride);
								if(rS.OptDelta2Range.Check(rTv2)) {
									rResult = rS.V.GetResultPerDay();
									winratio = rS.GetWinCountRate(); // @v10.4.6
									ok = 1;
								}
							}
						}
					}
					break;
			}
		}
	}
	rWinRatio = winratio;
	return ok;
}

SLAPI  PPObjTimeSeries::FactorToResultRelation::FactorToResultRelation() : FactorQuant(0), PeakAvg(0), Result(0.0)
{
}

SLAPI  PPObjTimeSeries::FactorToResultRelation::FactorToResultRelation(uint factorQuant, double result) : FactorQuant(factorQuant), PeakAvg(0), Result(result)
{
}

static FORCEINLINE double Implement_CalcSL(int sell, double mdv, int prec, double peak)
	{ return sell ? round(peak * (1.0 + mdv), prec) : round(peak * (1.0 - mdv), prec); }
static FORCEINLINE double Implement_CalcSL_Short(double mdv, int prec, double peak)
	{ return round(peak * (1.0 + mdv), prec); }
static FORCEINLINE double Implement_CalcSL_Long(double mdv, int prec, double peak)
	{ return round(peak * (1.0 - mdv), prec); }

static int SLAPI TsCalcStrategyResult2(const DateTimeArray & rTmList, const RealArray & rValList,
	const PPObjTimeSeries::Strategy & rS, uint valueIdx, PPObjTimeSeries::StrategyResultValueEx & rV)
{
	rV.Z();
	int    ok = 1;
	const  uint tsc = rTmList.getCount();
	assert(tsc == rValList.getCount());
	if(tsc && tsc == rValList.getCount() && valueIdx < tsc) {
		const LDATETIME start_tm = rTmList.get(valueIdx);
		const double stake_value = rValList.at(valueIdx);
		double peak = stake_value;
		double bottom = stake_value;
		const  uint org_max_duck_quant = rS.MaxDuckQuant;
		const  uint max_duck_quant = org_max_duck_quant; // Возможно, будет меняться в течении жизни ставки (пока нет)
		const  double mdv = (rS.MaxDuckQuant * rS.SpikeQuant);
		const  int is_short = BIN(rS.BaseFlags & rS.bfShort);
		const  int prec = rS.Prec;
		const  int adjust_sl = BIN(rS.StakeCloseMode == rS.clsmodAdjustLoss); // @v10.4.8
		{
			static const double spread_adjustment = 1.1; // Поправка запаса прочности для размера комиссии
			const uint _target_quant = rS.TargetQuant;
			const double spread = (rS.Prec > 0 && rS.SpreadAvg > 0.0) ? (rS.SpreadAvg * fpow10i(-rS.Prec) * spread_adjustment) : 0.0;
			const double margin = rS.Margin;
			uint k = valueIdx+1;
			if(is_short) {
				const double target_value = _target_quant ? round(stake_value * (1.0 - (_target_quant * rS.SpikeQuant)), prec) : 0.0;
				const double org_sl = Implement_CalcSL_Short(mdv, prec, peak);
				double sl = org_sl;
				while(k < tsc) {
					const double value = rValList.at(k);
					const double value_with_spread = (value + spread);
					const double stake_delta = -((value - stake_value) / stake_value);
					rV.Result = -((value_with_spread - stake_value) / stake_value) / margin;
					SETMAX(bottom, value);
					if(value >= sl) {
						ok = 2;
						break;
					}
					else if(_target_quant && value <= target_value) {
						ok = 2;
						break;
					}
					else {
						assert(bottom <= org_sl);
						if(peak > value) {
							peak = value;
							if(adjust_sl) // @v10.4.8
								sl = Implement_CalcSL_Short(mdv, prec, peak);
						}
						k++;
					}
				}
			}
			else {
				const double target_value = _target_quant ? round(stake_value * (1.0 + (_target_quant * rS.SpikeQuant)), prec) : 0.0;
				const double org_sl = Implement_CalcSL_Long(mdv, prec, peak);
				double sl = org_sl;
				while(k < tsc) {
					const double value = rValList.at(k);
					const double value_with_spread = (value - spread);
					const double stake_delta = ((value - stake_value) / stake_value);
					rV.Result = ((value_with_spread - stake_value) / stake_value) / margin;
					SETMIN(bottom, value);
					if(value <= sl) {
						ok = 2;
						break;
					}
					else if(_target_quant && value >= target_value) {
						ok = 2;
						break;
					}
					else {
						assert(bottom >= org_sl);
						if(peak < value) {
							peak = value;
							if(adjust_sl) // @v10.4.8
								sl = Implement_CalcSL_Long(mdv, prec, peak);
						}
						k++;
					}
				}
			}
			rV.TmCount = k - valueIdx;
			if(k == tsc) {
				const LDATETIME local_tm = rTmList.get(k-1);
				rV.TmSec = diffdatetimesec(local_tm, start_tm);
				rV.LastPoint = k-1;
				ok = 3;
			}
			else {
				const LDATETIME local_tm = rTmList.get(k);
				rV.TmSec = diffdatetimesec(local_tm, start_tm);
				rV.LastPoint = k;
				if(is_short) {
					rV.Peak = (stake_value - peak) / stake_value;
					rV.Bottom = (bottom - stake_value) / stake_value;
				}
				else {
					rV.Peak = (peak - stake_value) / stake_value;
					rV.Bottom = (stake_value - bottom) / stake_value;
				}
				rV.TmR.Start = start_tm; // @v10.4.11
				rV.TmR.Finish = local_tm; // @v10.4.11
			}
		}
	}
	return ok;
}

static void SLAPI TsSimulateStrategyContainer(const DateTimeArray & rTmList, const RealArray & rValList,
	const TSCollection <PPObjTimeSeries::TrendEntry> & rTrendList, const PPObjTimeSeries::StrategyContainer & rSc,
	uint testCount /* 0 - default */, PPObjTimeSeries::StrategyResultEntry & rSre, TSVector <PPObjTimeSeries::StrategyResultValueEx> * pDetailsList)
{
	const uint tsc = rTmList.getCount();
	assert(tsc == rValList.getCount());
	if(tsc && tsc == rValList.getCount()) {
		uint max_ifs = 0;
		uint max_delta2_stride = 0;
		PPObjTimeSeries::StrategyContainer::Index1 sc_index1;
		//SCollection trend_to_st_assoc_list(O_COLLECTION & ~aryDataOwner);
		rSc.CreateIndex1(sc_index1);
		for(uint tidx = 0; tidx < rTrendList.getCount(); tidx++) {
			const PPObjTimeSeries::TrendEntry * p_te = rTrendList.at(tidx);
			assert(p_te);
			if(p_te) {
				assert(p_te->TL.getCount() == tsc);
				SETMAX(max_ifs, p_te->Stride);
			}
		}
		/*
		for(uint si = 0; si < rSc.getCount(); si++) {
			const PPObjTimeSeries::Strategy & r_s = rSc.at(si);
			PPObjTimeSeries::StrategyResultEntry empty_result_entry;
			if(oneof2(r_s.StakeMode, 2, 3)) // @v10.3.9 (4) // @v10.4.5 -(4)
				SETMAX(max_delta2_stride, r_s.OptDelta2Stride);
			{
				const PPObjTimeSeries::TrendEntry * p_te = PPObjTimeSeries::SearchTrendEntry(rTrendList, r_s.InputFrameSize);
				trend_to_st_assoc_list.insert(p_te);
			}
		}
		assert(trend_to_st_assoc_list.getCount() == rSc.getCount()); // @paranoic
		*/
		const uint _start_offset = max_ifs+max_delta2_stride;
		const uint _max_test_count = NZOR(testCount, 10); // @20190422 100-->50 // @20191220 50-->10
		const uint _offs_inc_list[] = { 173, 269, 313, 401, 439 }; // @20191221
		//const uint _offset_increment = 173;
		long  sel_criterion = PPObjTimeSeries::StrategyContainer::selcritWinRatio | PPObjTimeSeries::StrategyContainer::selcritfSkipAmbiguous;
		//long  sel_criterion = PPObjTimeSeries::StrategyContainer::selcritWinRatio | PPObjTimeSeries::StrategyContainer::selcritfWeightAmbiguous;
		for(uint init_offset = _start_offset, test_no = 0; test_no < _max_test_count; (init_offset += _offs_inc_list[test_no % SIZEOFARRAY(_offs_inc_list)]), (test_no++)) {
			const LDATETIME start_tm = rTmList.at(init_offset);
			LDATETIME finish_tm = ZERODATETIME;
			for(uint i = init_offset; i < tsc; i++) {
				PPObjTimeSeries::BestStrategyBlock _best_result;
				if(rSc.Select(rTrendList, static_cast<int>(i), sel_criterion, &sc_index1, _best_result, 0) > 0) {
					assert(_best_result.MaxResultIdx >= 0 && _best_result.MaxResultIdx < static_cast<int>(rSc.getCount()));
					const PPObjTimeSeries::Strategy & r_s = rSc.at(_best_result.MaxResultIdx);
					PPObjTimeSeries::StrategyResultValueEx rv_ex;
					const int csr = TsCalcStrategyResult2(rTmList, rValList, r_s, i, rv_ex);
					if(csr == 2) {
						rSre.SetValue(rv_ex);
						const LDATETIME local_finish_tm = rTmList.at(rv_ex.LastPoint);
						assert(local_finish_tm == rv_ex.TmR.Finish); // @v10.4.11
						if(cmp(finish_tm, local_finish_tm) < 0)
							finish_tm = local_finish_tm;
						rv_ex.StrategyIdx = static_cast<int>(_best_result.MaxResultIdx);
						rv_ex.TrendErr = _best_result.TrendErr; // @v10.4.11
						rv_ex.TrendErrRel = _best_result.TrendErrRel; // @v10.4.11
						CALLPTRMEMB(pDetailsList, insert(&rv_ex)); // @v10.4.11
						i = rv_ex.LastPoint; // Дальше продолжим со следующей точки // @v10.5.1 (rv_ex.LastPoint+1)-->(rv_ex.LastPoint) цикл for сделает инкремент
					}
					else
						break; // Ряд оборвался - дальше анализировать нельзя: некоторые стратегии не имеют достаточно данных.
				}
			}
			if(!!finish_tm)
				rSre.TotalSec += diffdatetimesec(finish_tm, start_tm);
		}
	}
}

static void FASTCALL AddEntryToResultList(const PPObjTimeSeries::StrategyResultValueEx & rRvEx, long criterion, RealArray & rResultList, RealArray * pResultTmSecList)
{
	rResultList.add(rRvEx.Result); // @optvector
	if(criterion == PrcssrTsStrategyAnalyze::ModelParam::tcVelocity) {
		assert(pResultTmSecList);
		pResultTmSecList->add(static_cast<double>(rRvEx.TmSec));
	}
	else if(criterion == PrcssrTsStrategyAnalyze::ModelParam::tcWinRatio) {
		assert(pResultTmSecList);
		pResultTmSecList->add((rRvEx.Result > 0.0) ? 1.0 : 0.0);
	}
}

enum TsTestStrategyOfrOption {
	tstsofroMode1    = 0x0001,
	tstsofroMode2    = 0x0002,
	tstsofroPositive = 0x1000,
	tstsofroNegative = 0x2000,
};

static int SLAPI TsTestStrategy2(const PrcssrTsStrategyAnalyze::ModelParam & rMp,
	const DateTimeArray & rTmList, const RealArray & rValList, const PPObjTimeSeries::TrendEntry & rTe,
	const PPObjTimeSeries::TrendEntry * pMainTrendEntry,
	const PPObjTimeSeries::Strategy & rS, int options, PPObjTimeSeries::StrategyResultEntry & rSre,
	TSVector <PPObjTimeSeries::StrategyResultEntry> * pOptResultList)
{
	int    ok = 1;
	const  uint tsc = rTmList.getCount();
	const  uint ifs = rS.InputFrameSize;
	const  uint main_ifs = (pMainTrendEntry && rS.MainFrameSize) ? rS.MainFrameSize : 0;
	const  double trend_err_limit = (rS.TrendErrLim * rS.TrendErrAvg); // @v10.3.12
	assert(tsc == rValList.getCount());
	CALLPTRMEMB(pOptResultList, clear());
	if(tsc && tsc == rValList.getCount()) {
		RealArray result_list;
		RealArray result_addendum_list;
		uint   signal_count = 0;
		rSre.LastResultIdx = 0;
		assert(tsc == rTe.TL.getCount());
		THROW(rTe.TL.getCount() == tsc);
		PROFILE_START
		if(rSre.StakeMode == 0) {
			for(uint i = 0; i < tsc; i++) {
				int    is_signal = 1;
				PPObjTimeSeries::StrategyResultValueEx rv_ex;
				const double main_trend_val = pMainTrendEntry ? pMainTrendEntry->TL.at(i) : 0.0;
				if(!main_ifs || (i >= (main_ifs+1) && rS.OptDelta2Range.Check(main_trend_val))) {
					const  int csr = TsCalcStrategyResult2(rTmList, rValList, rS, i, rv_ex);
					THROW(csr);
					is_signal = BIN(csr == 2);
				}
				if(is_signal) {
					signal_count++;
					AddEntryToResultList(rv_ex, rMp.OptTargetCriterion, result_list, &result_addendum_list);
					rSre.SetValue(rv_ex);
					rSre.LastResultIdx = i;
				}
				else {
					result_list.add(0.0);
					if(oneof2(rMp.OptTargetCriterion, rMp.tcVelocity, rMp.tcWinRatio))
						result_addendum_list.add(0.0);
				}
			}
		}
		else if(rSre.StakeMode == 1) {
			const uint ifs_plus_one_max = MAX((ifs+1), (main_ifs+1));
			for(uint i = 0; i < tsc; i++) {
				PPObjTimeSeries::StrategyResultValueEx rv_ex;
				int    is_signal = 0;
				if(i >= ifs_plus_one_max && (!main_ifs || rS.OptDelta2Range.Check(pMainTrendEntry->TL.at(i)))) {
					const  double trend_err = rTe.ErrL.at(i); // @v10.3.12
					if((trend_err_limit <= 0.0 || (trend_err > 0.0 && trend_err <= trend_err_limit)) && rS.OptDeltaRange.Check(rTe.TL.at(i))) {
						const  int csr = TsCalcStrategyResult2(rTmList, rValList, rS, i, rv_ex);
						THROW(csr);
						if(csr == 2)
							rSre.LastResultIdx = i;
						is_signal = BIN(csr == 2);
					}
				}
				if(is_signal) {
					signal_count++;
					AddEntryToResultList(rv_ex, rMp.OptTargetCriterion, result_list, &result_addendum_list);
					rSre.SetValue(rv_ex);
				}
				else {
					result_list.add(0.0);
					if(oneof2(rMp.OptTargetCriterion, rMp.tcVelocity, rMp.tcWinRatio))
						result_addendum_list.add(0.0);
				}
			}
		}
		else if(rSre.StakeMode == 2) {
			for(uint i = 0; i < tsc; i++) {
				PPObjTimeSeries::StrategyResultValueEx rv_ex;
				int    is_signal = 0;
				if(i >= (ifs+rS.OptDelta2Stride)) {
					const double d2 = rTe.TL.StrideDifference(i, rS.OptDelta2Stride);
					if(rS.OptDelta2Range.Check(d2)) {
						const  int csr = TsCalcStrategyResult2(rTmList, rValList, rS, i, rv_ex);
						THROW(csr);
						if(csr == 2)
							rSre.LastResultIdx = i;
						is_signal = BIN(csr == 2);
					}
				}
				if(is_signal) {
					signal_count++;
					AddEntryToResultList(rv_ex, rMp.OptTargetCriterion, result_list, &result_addendum_list);
					rSre.SetValue(rv_ex);
				}
				else {
					result_list.add(0.0);
					if(oneof2(rMp.OptTargetCriterion, rMp.tcVelocity, rMp.tcWinRatio))
						result_addendum_list.add(0.0);
				}
			}
		}
		else if(rSre.StakeMode == 3) { // @v10.4.5 -(4)
			for(uint i = 0; i < tsc; i++) {
				PPObjTimeSeries::StrategyResultValueEx rv_ex;
				int    is_signal = 0;
				if(i >= (ifs+1) && i >= (ifs+rS.OptDelta2Stride)) {
					const double trend_err = rTe.ErrL.at(i); // @v10.3.12
					if((trend_err_limit <= 0.0 || (trend_err > 0.0 && trend_err <= trend_err_limit)) && rS.OptDeltaRange.Check(rTe.TL.at(i))) {
						const double d2 = rTe.TL.StrideDifference(i, rS.OptDelta2Stride);
						if(rS.OptDelta2Range.Check(d2)) {
							const  int csr = TsCalcStrategyResult2(rTmList, rValList, rS, i, rv_ex);
							THROW(csr);
							if(csr == 2)
								rSre.LastResultIdx = i;
							is_signal = BIN(csr == 2);
						}
					}
				}
				if(is_signal) {
					signal_count++;
					AddEntryToResultList(rv_ex, rMp.OptTargetCriterion, result_list, &result_addendum_list);
					rSre.SetValue(rv_ex);
				}
				else {
					result_list.add(0.0);
					if(oneof2(rMp.OptTargetCriterion, rMp.tcVelocity, rMp.tcWinRatio))
						result_addendum_list.add(0.0);
				}
			}
		}
		if(signal_count) {
			assert(signal_count == rSre.StakeCount);
			const double peak_avg = rSre.GetPeakAverage();
			const double bottom_avg = rSre.GetBottomAverage();
			assert(peak_avg >= 0.0); // @v10.3.5 (peak_avg > 0.0)-->(peak_avg >= 0.0)
			assert(bottom_avg >= 0.0); // @v10.3.5 (bottom_avg > 0.0)-->(bottom_avg >= 0.0)
			if(rSre.StakeMode == 0) {
				rSre.PeakAvgQuant = static_cast<uint16>(R0i(peak_avg / rSre.SpikeQuant));
				rSre.BottomAvgQuant = static_cast<uint16>(R0i(bottom_avg / rSre.SpikeQuant));
			}
			rSre.PeakMaxQuant = static_cast<uint16>(R0i(rSre.MaxPeak / rSre.SpikeQuant)); // Для каждого из режимов
		}
		assert(result_list.getCount() == tsc);
		assert(result_addendum_list.getCount() == (oneof2(rMp.OptTargetCriterion, rMp.tcVelocity, rMp.tcWinRatio) ? tsc : 0));
		/* @v10.3.9 {
			const double tr = result_list.Sum();
			assert(feqeps(rSre.V.Result, tr, 1e-7));
		}*/
		PROFILE_END
		if(pOptResultList && options & (tstsofroMode1|tstsofroMode2)) {
			const uint  first_correl_idx = ifs;
			TSVector <StrategyOptEntry> so_list;
			if(rSre.StakeMode == 0 && (options & tstsofroMode1)) {
				PROFILE_START
				so_list.clear();
				for(uint i = 0; i <= rSre.LastResultIdx; i++) {
					if(i >= (first_correl_idx+1)) {
						const double local_result = result_list.at(i);
						if(local_result != 0.0) { // @v10.3.9
							const double trend_err = rTe.ErrL.at(i); // @v10.3.12
							if(trend_err_limit <= 0.0 || (trend_err > 0.0 && trend_err <= trend_err_limit)) { // @v10.3.12
								const double local_factor = rTe.TL.at(i);
								if(oneof2(rMp.OptTargetCriterion, rMp.tcVelocity, rMp.tcWinRatio)) {
									const double local_addendum = result_addendum_list.at(i);
									if(rMp.OptTargetCriterion == rMp.tcWinRatio || local_addendum > 0.0) {
										StrategyOptEntry entry(local_factor, local_result, local_addendum);
										THROW_SL(so_list.insert(&entry));
									}
								}
								else {
									StrategyOptEntry entry(local_factor, local_result);
									THROW_SL(so_list.insert(&entry));
								}
							}
						}
					}
				}
				so_list.sort(PTR_CMPFUNC(double));
				// @v10.3.12 Все-таки снова попробуем разбивать интервал только в соответствии со знаком тренда (initial_splitting 0-->(3:2))
				//const int initial_splitting = (rSre.BaseFlags & rSre.bfShort) ? 3 : 2;
				int initial_splitting = 4; // @20190402
				//const int initial_splitting = 1;
				//const int initial_splitting = 0;
				// @20190417 {
				if(options & tstsofroPositive)
					initial_splitting = 5;
				else if(options & tstsofroNegative)
					initial_splitting = 6;
				else
					initial_splitting = 4;
				// } @20190417
				{
					TSVector<PPObjTimeSeries::OptimalFactorRange> ofr_list;
					if(rMp.Flags & rMp.fOptRangeMulti) {
						FindOptimalFactorRange2(rMp, so_list, initial_splitting, ofr_list);
					}
					else {
						PPObjTimeSeries::OptimalFactorRange ofr;
						FindOptimalFactorRange(rMp, so_list, initial_splitting, ofr/*rSre.OptDeltaRange*/);
						ofr_list.insert(&ofr);
					}
					for(uint ofridx = 0; ofridx < ofr_list.getCount(); ofridx++) {
						PPObjTimeSeries::OptimalFactorRange & r_ofr = ofr_list.at(ofridx);
						PPObjTimeSeries::StrategyResultEntry sre_temp(rS, rS.StakeMode/*stake_mode*/);
						sre_temp.OptDeltaRange = r_ofr;
						if(main_ifs) {
							sre_temp.OptDelta2Range = rS.OptDelta2Range;
							sre_temp.MainFrameSize = main_ifs;
						}
						else
							sre_temp.OptDelta2Range.Z();
						sre_temp.PeakAvgQuant = rSre.PeakAvgQuant;
						sre_temp.BottomAvgQuant = rSre.BottomAvgQuant;
						sre_temp.PeakMaxQuant = rSre.PeakMaxQuant;
						pOptResultList->insert(&sre_temp);
					}
				}
				PROFILE_END
			}
			if(oneof2(rSre.StakeMode, 0, 1) && (options & tstsofroMode2)) {
				//THROW(FindOptimalFactorRange_SecondKind(rTe, result_list, rSre));
				//static int SLAPI FindOptimalFactorRange_SecondKind(const PPObjTimeSeries::TrendEntry & rTe, const RealArray & rResultList, PPObjTimeSeries::StrategyResultEntry & rSre)
				assert(result_list.getCount() == rTe.TL.getCount());
				const uint ifs = rSre.InputFrameSize;
				// @v10.3.10 const uint max_stride = 20; // @v10.3.1 4-->6 // 6-->10 // @v10.3.5 10-->20
				const uint min_stride = ifs / 16; // @v10.3.10 3-->ifs / 16
				const uint max_stride = ifs / 4;
				double max_result = 0.0;
				//IMPL_CMPFUNC(PPObjTimeSeries_OptimalFactorRange_Result, p1, p2)
				TSVector<PPObjTimeSeries::OptimalFactorRange> overall_ofr_list;
				for(uint stride = min_stride; stride <= max_stride; stride++) { // @v10.3.9 (stride = 1)-->(stride = 3)
					so_list.clear();
					for(uint i = 0; i <= rSre.LastResultIdx; i++) {
						if(i >= (/*first_correl_idx*/ifs+stride)) {
							const double local_result = result_list.at(i);
							if(local_result != 0.0) {
								const double trend_err = rTe.ErrL.at(i); // @v10.4.7
								if(trend_err_limit <= 0.0 || (trend_err > 0.0 && trend_err <= trend_err_limit)) { // @v10.4.7
									const double local_factor = rTe.TL.StrideDifference(i, stride);
									if(oneof2(rMp.OptTargetCriterion, rMp.tcVelocity, rMp.tcWinRatio)) {
										const double local_addendum = result_addendum_list.at(i);
										if(rMp.OptTargetCriterion == rMp.tcWinRatio || local_addendum > 0.0) {
											StrategyOptEntry entry(local_factor, local_result, local_addendum);
											THROW_SL(so_list.insert(&entry));
										}
									}
									else {
										StrategyOptEntry entry(local_factor, local_result);
										THROW_SL(so_list.insert(&entry));
									}
								}
							}
						}
					}
					PPObjTimeSeries::OptimalFactorRange opt_range;
					so_list.sort(PTR_CMPFUNC(double));
					/* @v10.4.7
					const int initial_splitting = 4;
					FindOptimalFactorRange(rMp, so_list, initial_splitting, opt_range);
					if(max_result < opt_range.Result) {
						max_result = opt_range.Result;
						rSre.OptDelta2Range = opt_range;
						rSre.OptDelta2Stride = stride;
					}*/
					// @v10.4.7 {
					{
						int initial_splitting = 4;
						if(options & tstsofroPositive)
							initial_splitting = 5;
						else if(options & tstsofroNegative)
							initial_splitting = 6;
						else
							initial_splitting = 4;
						TSVector<PPObjTimeSeries::OptimalFactorRange> ofr_list;
						if(rMp.Flags & rMp.fOptRangeMulti) {
							FindOptimalFactorRange2(rMp, so_list, initial_splitting, ofr_list);
						}
						else {
							PPObjTimeSeries::OptimalFactorRange ofr;
							FindOptimalFactorRange(rMp, so_list, initial_splitting, ofr/*rSre.OptDeltaRange*/);
							ofr_list.insert(&ofr);
						}
						for(uint ofridx = 0; ofridx < ofr_list.getCount(); ofridx++) {
							PPObjTimeSeries::OptimalFactorRange & r_ofr = ofr_list.at(ofridx);
							r_ofr.Opt2Stride = stride;
							overall_ofr_list.insert(&r_ofr);
						}
					}
					// } @v10.4.7
				}
				{
					overall_ofr_list.sort(PTR_CMPFUNC(PPObjTimeSeries_OptimalFactorRange_Result));
					const uint max_range_count = (rMp.OptRangeMultiLimit > 0) ? rMp.OptRangeMultiLimit : 6;
					uint ofridx = overall_ofr_list.getCount();
					uint result_count = 0;
					if(ofridx) do {
						PPObjTimeSeries::OptimalFactorRange & r_ofr = overall_ofr_list.at(--ofridx);
						PPObjTimeSeries::StrategyResultEntry sre_temp(rS, rS.StakeMode/*stake_mode*/);
						sre_temp.OptDeltaRange.Z();// = r_ofr;
						sre_temp.OptDelta2Range = r_ofr;
						sre_temp.OptDelta2Stride = r_ofr.Opt2Stride;
						sre_temp.PeakAvgQuant = rSre.PeakAvgQuant;
						sre_temp.BottomAvgQuant = rSre.BottomAvgQuant;
						sre_temp.PeakMaxQuant = rSre.PeakMaxQuant;
						pOptResultList->insert(&sre_temp);
						result_count++;
					} while(ofridx && result_count < max_range_count);
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

static SString & FASTCALL _TsFineOptFactorMakeFinishEvntName(const char * pUniq, SString & rBuf)
{
    size_t len = sstrlen(pUniq);
    uint32 hash = SlHash::BobJenc(pUniq, len);
	(rBuf = "TSFINDOPTFACTORFINISHEVNT").CatChar('-').Cat(hash);
	return rBuf;
}

//static
int SLAPI Ts_Helper_FindOptimalFactor(const DateTimeArray & rTmList, const RealArray & rValList,
	const PPObjTimeSeries::TrainNnParam & rS, double & rResult, uint & rPeakQuant)
{
	class CalcResultThread : public SlThread {
	public:
		struct InitBlock {
			InitBlock(const DateTimeArray & rTmList, const RealArray & rValList, const PPObjTimeSeries::TrainNnParam & rS,
				uint firstIdx, uint lastIdx, PPObjTimeSeries::StrategyResultEntry * pResult, ACount * pCounter, const SString & rUniq) :
				R_TmList(rTmList), R_ValList(rValList), R_S(rS), FirstIdx(firstIdx), LastIdx(lastIdx), P_Result(pResult), P_Counter(pCounter),
				Uniq(rUniq)
			{
			}
			const DateTimeArray & R_TmList;
			const RealArray & R_ValList;
			const PPObjTimeSeries::TrainNnParam & R_S;
			const uint FirstIdx;
			const uint LastIdx;
			const SString Uniq;
			PPObjTimeSeries::StrategyResultEntry * P_Result;
			ACount * P_Counter;
		};
		CalcResultThread(InitBlock * pBlk) : SlThread(0), B(*pBlk)
		{
			InitStartupSignal();
		}
		virtual void Run()
		{
			assert(SLS.GetConstTLA().Id == GetThreadID());
			B.P_Counter->Incr();
			const uint tsc = B.R_TmList.getCount();
			assert(B.FirstIdx >= 0 && B.FirstIdx < tsc && B.LastIdx >= B.FirstIdx && B.LastIdx < tsc);
			PPObjTimeSeries::StrategyResultEntry sre(B.R_S, 0);
			sre.LastResultIdx = 0;
			for(uint i = B.FirstIdx; i <= B.LastIdx; i++) {
				PPObjTimeSeries::StrategyResultValueEx rv_ex;
				const  int csr = TsCalcStrategyResult2(B.R_TmList, B.R_ValList, B.R_S, i, rv_ex);
				//THROW(csr);
				assert(csr);
				if(csr == 2) {
					sre.SetValue(rv_ex);
					sre.LastResultIdx = i;
				}
			}
			B.P_Result->SetOuter(sre);
            long c = B.P_Counter->Decr();
            if(c <= 0) {
				SString temp_buf;
				Evnt evnt_finish(_TsFineOptFactorMakeFinishEvntName(B.Uniq, temp_buf), Evnt::modeOpen);
				evnt_finish.Signal();
            }
		}
	private:
		virtual void SLAPI Startup()
		{
			SlThread::Startup();
			SignalStartup();
		}
		InitBlock B;
	};
	int    ok = 1;
	const  uint max_thread = 4;
	const  uint tsc = rTmList.getCount();
	Evnt * p_ev_finish = 0;
	SString temp_buf;
	PPObjTimeSeries::StrategyResultEntry sre(rS, 0);
	sre.LastResultIdx = 0;
	HANDLE objs_to_wait[16];
	size_t objs_to_wait_count = 0;
	MEMSZERO(objs_to_wait);
	assert(max_thread <= SIZEOFARRAY(objs_to_wait));
	if(max_thread > 1 && tsc >= 10000) {
		const uint chunk_size = tsc / max_thread;
		uint next_chunk_idx = 0;
		ACount thread_counter;
		S_GUID uuid;
		uuid.Generate();
		SString uniq;
		uuid.ToStr(S_GUID::fmtPlain, uniq);
		THROW_S(p_ev_finish = new Evnt(_TsFineOptFactorMakeFinishEvntName(uniq, temp_buf), Evnt::modeCreateAutoReset), SLERR_NOMEM);
		thread_counter.Incr();
		for(uint tidx = 0; tidx < max_thread; tidx++) {
			const uint last_chunk_idx = (tidx == (max_thread-1)) ? (tsc-1) : (next_chunk_idx+chunk_size-1);
			CalcResultThread::InitBlock tb(rTmList, rValList, rS, next_chunk_idx, last_chunk_idx, &sre, &thread_counter, uniq);
			CalcResultThread * p_thread = new CalcResultThread(&tb);
			THROW_S(p_thread, SLERR_NOMEM);
			p_thread->Start(1/*0*/);
			objs_to_wait[objs_to_wait_count++] = *p_thread;
			next_chunk_idx += chunk_size;
		}
		WaitForMultipleObjects(objs_to_wait_count, objs_to_wait, TRUE, INFINITE);
		//if(thread_counter.Decr() > 0)
			//p_ev_finish->Wait(-1);
	}
	else {
		for(uint i = 0; i < tsc; i++) {
			PPObjTimeSeries::StrategyResultValueEx rv_ex;
			const  int csr = TsCalcStrategyResult2(rTmList, rValList, rS, i, rv_ex);
			THROW(csr);
			if(csr == 2) {
				sre.SetValue(rv_ex);
				sre.LastResultIdx = i;
			}
		}
	}
	rResult = sre.V.GetResultPerDay();
	const double peak_avg = sre.GetPeakAverage();
	rPeakQuant = static_cast<uint>(R0i(peak_avg / sre.SpikeQuant));
	CATCHZOK
	ZDELETE(p_ev_finish);
	return ok;
}

int SLAPI PPObjTimeSeries::FindOptimalFactor(const DateTimeArray & rTmList, const RealArray & rValList, const TrainNnParam & rS, int what, const IntRange & rMdRange, int mdStep,
	int entireRange, TSVector <FactorToResultRelation> & rSet, FactorToResultRelation & rResult)
{
	int    ok = -1;
	assert(oneof2(what, 0, 1));
	assert(rMdRange.low > 0 && rMdRange.upp > rMdRange.low && mdStep > 0);
	SString log_buf;
	SString temp_buf;
	SString log_file_name;
	SString symb_buf;
	rResult.FactorQuant = 0;
	rResult.PeakAvg = 0;
	rResult.Result = 0.0;
	PPGetFilePath(PPPATH_LOG, "Ts-FindOptimalLevel.log", log_file_name);
	TrainNnParam local_s(rS);
	const bool the_first_call = (rSet.getCount() == 0);
	const uint tsc = rTmList.getCount();
	TSVector <FactorToResultRelation> mdr_list;
	assert(rValList.getCount() == tsc);
	for(uint md = rMdRange.low; (int)md <= rMdRange.upp; md += mdStep) {
		int    done = 0;
		double local_result = 0.0;
		uint   local_peak_quant = 0;
		for(uint sidx = 0; !done && sidx < rSet.getCount(); sidx++) {
			const FactorToResultRelation & r_set_item = rSet.at(sidx);
			if(r_set_item.FactorQuant == md) {
				local_result = r_set_item.Result;
				done = 1;
			}
		}
		if(!done) {
			if(what == 0)
				local_s.MaxDuckQuant = md;
			else
				local_s.TargetQuant = md;
			THROW(Ts_Helper_FindOptimalFactor(rTmList, rValList, local_s, local_result, local_peak_quant));
			{
				FactorToResultRelation new_set_entry(md, local_result);
				new_set_entry.PeakAvg = local_peak_quant;
				THROW_SL(rSet.insert(&new_set_entry));
			}
		}
		{
			symb_buf = rS.Symb;
			if(rS.BaseFlags & rS.bfShort)
				symb_buf.CatChar('-').Cat("REV");
			if(what == 0)
				log_buf.Z().Cat("FindOptimalMaxDuck");
			else
				log_buf.Z().CatChar('[').CatEq("FindOptimalMaxDuck", rS.MaxDuckQuant).CatChar(']').Space().Cat("FindOptimalPeak");
			log_buf.CatDiv(':', 2).Cat(symb_buf).Space().CatEq((what == 0) ? "MaxDuckQuant" : "PeakQuant", md).Space().
				CatEq("Result", local_result, MKSFMTD(15, 5, 0));
			PPLogMessage(log_file_name, log_buf, LOGMSGF_TIME);
		}
		{
			FactorToResultRelation mdr_entry(md, local_result);
			mdr_entry.PeakAvg = local_peak_quant;
			const uint last_mdr_pos = mdr_list.getCount();
			THROW_SL(mdr_list.insert(&mdr_entry));
			if(!entireRange && (the_first_call && what == 0) && mdr_list.getCount() > 1) {
				const double prev_result = mdr_list.at(last_mdr_pos-1).Result;
				if(local_result < prev_result)
					break;
			}
		}
	}
	{
		for(uint sidx = 0; sidx < rSet.getCount(); sidx++) {
			const FactorToResultRelation & r_outer_item = rSet.at(sidx);
			uint pos = 0;
			if(!mdr_list.lsearch(&r_outer_item.FactorQuant, &pos, PTR_CMPFUNC(uint)))
				mdr_list.insert(&r_outer_item);
			else
				assert(mdr_list.at(pos).FactorQuant == r_outer_item.FactorQuant);
		}
		mdr_list.sort(PTR_CMPFUNC(uint));
		const uint mdrc = mdr_list.getCount();
		if(mdrc > 1) {
			IntRange local_md_range;
			double max_result = -MAXDOUBLE;
			int    best_idx = -1;
			for(uint j = 0; j < mdrc; j++) {
				const double local_mdr_result = mdr_list.at(j).Result;
				if(max_result < local_mdr_result) {
					max_result = local_mdr_result;
					best_idx = static_cast<int>(j);
				}
			}
			assert(best_idx >= 0);
			const uint low_idx = static_cast<uint>((best_idx > 0) ? (best_idx-1) : best_idx);
			const uint upp_idx = static_cast<uint>(((best_idx+1) < (int)mdrc) ? (best_idx+1) : best_idx);
			local_md_range.Set(mdr_list.at(low_idx).FactorQuant, mdr_list.at(upp_idx).FactorQuant);
			assert(local_md_range.upp > local_md_range.low);
			THROW(local_md_range.upp > local_md_range.low);
			const  int range_divider = (local_md_range == rMdRange) ? 5 : 4;
			int    local_md_step = (local_md_range.upp - local_md_range.low) / range_divider;
			if(local_md_step > 0) {
				int   rr = FindOptimalFactor(rTmList, rValList, rS, what, local_md_range, local_md_step, 0, rSet, rResult); // @recursion
				THROW(rr);
				ok = rr;
			}
			else {
				rResult = mdr_list.at((best_idx >= 0 && best_idx < (int)mdrc) ? best_idx : 0);
				ok = 1;
			}
		}
		else if(mdrc) {
			rResult = mdr_list.at(0);
			ok = 1;
		}
		if(ok > 0 && the_first_call) {
			if(what == 0)
				log_buf.Z().Cat("!OptimalMaxDuck");
			else
				log_buf.Z().Cat("!OptimalPeak").Space().CatChar('[').CatEq("MaxDuck", rS.MaxDuckQuant).CatChar(']');
			log_buf.CatDiv(':', 2).Cat(rS.Symb).Space().CatEq((what == 0) ? "MaxDuckQuant" : "PeakQuant", rResult.FactorQuant).Space().
				CatEq("Result", rResult.Result, MKSFMTD(15, 5, 0));
			PPLogMessage(log_file_name, log_buf, LOGMSGF_TIME);
		}
	}
	CATCHZOK
	return ok;
}

PPObjTimeSeries::OptimalFactorRange::OptimalFactorRange() : Count(0), Result(0.0), Opt2Stride(0)
{
	RealRange::Z();
}

PPObjTimeSeries::OptimalFactorRange & PPObjTimeSeries::OptimalFactorRange::Z()
{
	RealRange::Z();
	Opt2Stride = 0;
	Count = 0;
	Result = 0.0;
	return *this;
}

SLAPI PPObjTimeSeries::Strategy::Strategy() : InputFrameSize(0), Prec(0), TargetQuant(0), MaxDuckQuant(0), OptDelta2Stride(0), StakeMode(0),
	BaseFlags(0), Margin(0.0), SpikeQuant(0.0), StakeThreshold(0.0), SpreadAvg(0.0), /*OptDeltaCount(0), OptDelta2Count(0),*/ StakeCount(0), WinCount(0),
	StakeCloseMode(0), PeakAvgQuant(0), BottomAvgQuant(0), PeakMaxQuant(0), ID(0), TrendErrAvg(0.0), TrendErrLim(0.0), MainFrameSize(0)
{
	// @v10.4.5 OptDeltaRange.SetVal(0.0);
	// @v10.4.5 OptDelta2Range.SetVal(0.0);
	// @v10.4.9 memzero(Reserve, sizeof(Reserve));
}

void SLAPI PPObjTimeSeries::Strategy::Reset()
{
	SpikeQuant = 0.0;
	MaxDuckQuant = 0;
}

SLAPI PPObjTimeSeries::StrategyContainer::StrategyContainer() : TSVector <Strategy>(),
	Ver(3), StorageTm(ZERODATETIME), LastValTm(ZERODATETIME), LastStrategyId(0) // @v10.4.5 Ver(2)-->(3)
{
}

SLAPI PPObjTimeSeries::StrategyContainer::StrategyContainer(const PPObjTimeSeries::StrategyContainer & rS) : TSVector <Strategy>(rS),
	Ver(rS.Ver), StorageTm(rS.StorageTm), LastValTm(rS.LastValTm), LastStrategyId(rS.LastStrategyId)
{
}

PPObjTimeSeries::StrategyContainer & FASTCALL PPObjTimeSeries::StrategyContainer::operator = (const PPObjTimeSeries::StrategyContainer & rS)
{
	Copy(rS);
	return *this;
}

int FASTCALL PPObjTimeSeries::StrategyContainer::Copy(const PPObjTimeSeries::StrategyContainer & rS)
{
	int    ok = TSVector <PPObjTimeSeries::Strategy>::copy(rS);
	Ver = rS.Ver;
	StorageTm = rS.StorageTm;
	LastValTm = rS.LastValTm;
	return ok;
}

void PPObjTimeSeries::StrategyContainer::SetLastValTm(LDATETIME dtm)
{
	LastValTm = dtm;
}

PPObjTimeSeries::StrategyContainer::IndexEntry1::Range::Range(const RealRange & rR, uint idx) : Idx(idx), RealRange(rR)
{
}

PPObjTimeSeries::StrategyContainer::IndexEntry1::IndexEntry1() : Stride(0)
{
}

int SLAPI PPObjTimeSeries::StrategyContainer::CreateIndex1(PPObjTimeSeries::StrategyContainer::Index1 & rIndex) const
{
	int    ok = 1;
	rIndex.freeAll();
	rIndex.TmFrmToMaxErrList.freeAll();
	for(uint si = 0; si < getCount(); si++) {
		const PPObjTimeSeries::Strategy & r_s = at(si);
		if(r_s.StakeMode == 1 && r_s.V.TmSec > 0) {
			IndexEntry1 * p_entry = 0;
			for(uint i = 0; !p_entry && i < rIndex.getCount(); i++) {
				IndexEntry1 * p_temp_entry = rIndex.at(i);
				if(p_temp_entry->Stride == r_s.InputFrameSize)
					p_entry = p_temp_entry;
			}
			if(!p_entry) {
				THROW_SL(p_entry = rIndex.CreateNewItem());
				p_entry->Stride = r_s.InputFrameSize;
			}
			{
				IndexEntry1::Range rng(r_s.OptDeltaRange, si);
				p_entry->RangeList.insert(&rng);
			}
			/* @v10.5.6
			// @v10.5.0 {
			{
				const double trend_err_limit = (r_s.TrendErrLim * r_s.TrendErrAvg);
				double max_err = 0.0;
				uint   max_err_pos = 0;
				if(rIndex.TmFrmToMaxErrList.Search(r_s.InputFrameSize, &max_err, &max_err_pos, 0)) {
					if(max_err < trend_err_limit) {
						rIndex.TmFrmToMaxErrList.at(max_err_pos).Val = trend_err_limit;
					}
				}
				else {
					rIndex.TmFrmToMaxErrList.Add(r_s.InputFrameSize, trend_err_limit);
				}
			}
			// } @v10.5.0
			@v10.5.6 */
		}
	}
	// @v10.5.6 rIndex.TmFrmToMaxErrList.SortByKey(); // @v10.5.0
	if(rIndex.getCount()) {
		rIndex.sort(PTR_CMPFUNC(uint));
		for(uint i = 0; i < rIndex.getCount(); i++) {
			IndexEntry1 * p_temp_entry = rIndex.at(i);
			p_temp_entry->RangeList.sort(PTR_CMPFUNC(double));
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjTimeSeries::StrategyContainer::Select(const TSCollection <TrendEntry> & rTrendList, int lastTrendIdx, long criterion,
	const Index1 * pIndex1, BestStrategyBlock & rBsb, LongArray * pAllSuitedPosList) const
{
	int    ok = -1;
	uint   potential_long_count = 0;
	uint   potential_short_count = 0;
	BestStrategyBlock _best_result;
	LongArray candidate_pos_list;
	if(pIndex1) {
		const TrendEntry * p_main_te_last = 0;
		for(uint tidx = 0; tidx < rTrendList.getCount(); tidx++) {
			const TrendEntry * p_te = rTrendList.at(tidx);
			uint  ip = 0;
			int   do_skip_trend = 0;
			// @v10.5.0 {
			/* @v10.5.6
			const uint tlc = p_te->TL.getCount();
			assert((lastTrendIdx < 0 && (-lastTrendIdx) <= static_cast<int>(tlc)) || (lastTrendIdx >= 0 && lastTrendIdx < static_cast<int>(tlc)));
			const uint trend_idx = (lastTrendIdx < 0) ? (tlc+lastTrendIdx) : static_cast<uint>(lastTrendIdx);
			const double trend_err = p_te->ErrL.at(trend_idx);
			if(trend_err > 0.0 && pIndex1->TmFrmToMaxErrList.getCount()) {
				double max_err = 0.0;
				uint   max_err_pos = 0;
				if(pIndex1->TmFrmToMaxErrList.Search(p_te->Stride, &max_err, &max_err_pos, 1) && max_err < trend_err)
					do_skip_trend = 1;
			} */
			// } @v10.5.0
			if(!do_skip_trend && pIndex1->bsearch(&p_te->Stride, &ip, CMPF_LONG)) {
				const IndexEntry1 * p_ie = pIndex1->at(ip);
				assert(p_ie->Stride == p_te->Stride);
				// @v10.5.6 /* @v10.5.0
				const uint tlc = p_te->TL.getCount();
				assert((lastTrendIdx < 0 && (-lastTrendIdx) <= static_cast<int>(tlc)) || (lastTrendIdx >= 0 && lastTrendIdx < static_cast<int>(tlc)));
				const uint trend_idx = (lastTrendIdx < 0) ? (tlc+lastTrendIdx) : static_cast<uint>(lastTrendIdx);
				const double trend_err = p_te->ErrL.at(trend_idx);
				// @v10.5.6 */
				const double tv = p_te->TL.at(trend_idx);
				for(uint ridx = 0; ridx < p_ie->RangeList.getCount(); ridx++) {
					const IndexEntry1::Range & r_ri = p_ie->RangeList.at(ridx);
					if(r_ri.low > tv) {
						break;
					}
					else if(r_ri.low <= tv) {
						if(r_ri.upp >= tv) {
							const Strategy & r_s = at(r_ri.Idx);
							assert(r_s.InputFrameSize == p_ie->Stride);
							assert(r_s.OptDeltaRange.Check(tv));
							const double trend_err_limit = (r_s.TrendErrLim * r_s.TrendErrAvg);
							if(trend_err_limit <= 0.0 || (trend_err > 0.0 && trend_err <= trend_err_limit)) {
								bool do_skip = false;
								if(r_s.BaseFlags & r_s.bfShort) {
									//is_there_short = true;
									potential_short_count++;
									if(criterion & selcritfSkipShort)
										do_skip = true;
								}
								else {
									//is_there_long = true;
									potential_long_count++;
									if(criterion & selcritfSkipLong)
										do_skip = true;
								}
								if(!do_skip) {
									// @v10.4.9 {
									if(r_s.MainFrameSize) {
										const TrendEntry * p_main_te = 0;
										if(p_main_te_last && p_main_te_last->Stride == r_s.MainFrameSize)
											p_main_te = p_main_te_last;
										else
											p_main_te = SearchTrendEntry(rTrendList, r_s.MainFrameSize);
										if(p_main_te) {
											p_main_te_last = p_main_te;
											double main_trend_val = p_main_te->TL.at(trend_idx);
											if(!r_s.OptDelta2Range.Check(main_trend_val))
												do_skip = true;
										}
										else
											do_skip = true;
									}
									// } @v10.4.9
									if(!do_skip) {
										long lpos = static_cast<long>(r_ri.Idx);
										candidate_pos_list.insert(&lpos);
									}
								}
							}
						}
					}
				}
			}
		}
	}
	else {
		for(uint si = 0; si < getCount(); si++) {
			const Strategy & r_s = at(si);
			const TrendEntry * p_te = SearchTrendEntry(rTrendList, r_s.InputFrameSize);
			const TrendEntry * p_main_te = r_s.MainFrameSize ? SearchTrendEntry(rTrendList, r_s.MainFrameSize) : 0;
			if(p_te) {
				double cr = 0.0;
				double winratio = 0.0;
				double _tv = 0.0; // Трендовое значение (вычисление зависит от модели)
				double _tv2 = 0.0; // Второе трендовое значение (вычисляется для комбинированной модели)
				if(MatchStrategy(p_te, p_main_te, lastTrendIdx, r_s, cr, winratio, _tv, _tv2)) {
					//_best_result.SetResult(cr, si, _tv, _tv2);
					bool do_skip = false;
					if(r_s.BaseFlags & r_s.bfShort) {
						//is_there_short = true;
						potential_short_count++;
						if(criterion & selcritfSkipShort)
							do_skip = true;
					}
					else {
						//is_there_long = true;
						potential_long_count++;
						if(criterion & selcritfSkipLong)
							do_skip = true;
					}
					if(!do_skip) {
						const long lpos = static_cast<long>(si);
						candidate_pos_list.insert(&lpos);
					}
				}
			}
		}
	}
	if(candidate_pos_list.getCount()) {
		bool skip_short = false;
		bool skip_long = false;
		if(potential_long_count && potential_short_count) {
			if(criterion & selcritfSkipAmbiguous) {
				skip_short = true;
				skip_long = true;
				ok = -2;
			}
			else if(criterion & selcritfWeightAmbiguous) {
				if(potential_long_count > potential_short_count)
					skip_short = true;
				else if(potential_short_count > potential_long_count)
					skip_long = true;
				else {
					skip_short = true;
					skip_long = true;
					ok = -2;
				}
			}
		}
		/*if(criterion & selcritfSkipAmbiguous && is_there_short && is_there_long)
			ok = -2;
		else {*/
		if(!skip_long || !skip_short) {
			for(uint clidx = 0; clidx < candidate_pos_list.getCount(); clidx++) {
				const uint sidx = static_cast<uint>(candidate_pos_list.get(clidx));
				const Strategy & r_s = at(sidx);
				if((r_s.BaseFlags & r_s.bfShort && !skip_short) || (!(r_s.BaseFlags & r_s.bfShort) && !skip_long)) {
					double local_result = 0.0;
					switch(criterion & 0xffL) {
						case selcritVelocity: local_result = r_s.V.GetResultPerDay(); break;
						case selcritWinRatio: local_result = r_s.GetWinCountRate(); break;
					}
					if(local_result > 0.0) {
						const TrendEntry * p_te = SearchTrendEntry(rTrendList, r_s.InputFrameSize);
						const uint tlc = p_te ? p_te->TL.getCount() : 0;
						const uint trend_idx = (lastTrendIdx < 0) ? (tlc-1) : static_cast<uint>(lastTrendIdx);
						const double trend_err = p_te->ErrL.at(trend_idx);
						const double tv = p_te ? p_te->TL.at(trend_idx) : 0.0;
						_best_result.SetResult(local_result, sidx, tv, 0.0);
						_best_result.TrendErr = trend_err; // @v10.4.11
						_best_result.TrendErrRel = fdivnz(trend_err, r_s.TrendErrAvg); // @v10.4.11
					}
				}
			}
			if(_best_result.MaxResult > 0.0)
				ok = 1;
		}
	}
	rBsb = _best_result;
	ASSIGN_PTR(pAllSuitedPosList, candidate_pos_list);
	return ok;
}

struct StrategyCritEntry {
	StrategyCritEntry(uint idx) : Idx(idx), Crit1(0.0), Crit2(0.0)
	{
	}
	uint   Idx;
	double Crit1;
	double Crit2;
};

static IMPL_CMPFUNC(StrategyCritEntry, i1, i2)
{
	const StrategyCritEntry * p1 = static_cast<const StrategyCritEntry *>(i1);
	const StrategyCritEntry * p2 = static_cast<const StrategyCritEntry *>(i2);
	int   si = 0;
	CMPCASCADE2(si, p2, p1, Crit1, Crit2); // Нам нужно чтобы большие значения были вверху (потому (p2, p1) вместо (p1, p2))
	return si;
}

int SLAPI PPObjTimeSeries::StrategyContainer::GetBestSubset(long flags, uint maxCount, double minWinRate, StrategyContainer & rScDest, StrategyContainer * pScSkipDueDup) const
{
	rScDest.clear();
	int    ok = 1;
	TSArray <StrategyCritEntry> range_list_;
	const uint _c = getCount();
	for(uint i = 0; i < _c; i++) {
		const Strategy & r_item = at(i);
		const double result = r_item.V.GetResultPerDay();
		if(result > 0.0 && r_item.GetWinCountRate() >= minWinRate) {
			int    skip = 0;
			if(flags & gbsfTrendFollowing && r_item.StakeMode == 1) {
				if(r_item.BaseFlags & r_item.bfShort) {
					if(!r_item.OptDeltaRange.LessThan(0.0))
						skip = 1;
				}
				else {
					if(!r_item.OptDeltaRange.GreaterThan(0.0))
						skip = 1;
				}
			}
			if(!skip) {
				double crit1 = 0.0;
				double crit2 = 0.0;
				if(flags & gbsfCritProb) {
					crit1 = r_item.GetWinCountRate();
					crit2 = result;
				}
				else if(flags & gbsfCritProfitMultProb)
					crit1 = result * r_item.GetWinCountRate();
				else
					crit1 = result;
				if(((r_item.BaseFlags & r_item.bfShort) && (flags & gbsfShort)) || (!(r_item.BaseFlags & r_item.bfShort) && (flags & gbsfLong))) {
					if((r_item.StakeMode == 1 && flags & gbsfStakeMode1) || (r_item.StakeMode == 2 && flags & gbsfStakeMode2) || (r_item.StakeMode == 3 && flags & gbsfStakeMode3)) {
						StrategyCritEntry crit_entry(i+1);
						crit_entry.Crit1 = crit1;
						crit_entry.Crit2 = crit2;
						THROW_SL(range_list_.insert(&crit_entry));
					}
				}
			}
		}
	}
	range_list_.sort(PTR_CMPFUNC(StrategyCritEntry));
	for(uint ridx = 0; ridx < range_list_.getCount() && rScDest.getCount() < maxCount; ridx++) {
		const StrategyCritEntry & r_range_item_ = range_list_.at(ridx);
		const uint pos = static_cast<uint>(r_range_item_.Idx-1);
		assert(pos < _c);
		const Strategy & r_item = at(pos);
		int   do_skip = 0;
		const double _eps = 1E-11;
		if(flags & gbsfEliminateDups && r_item.StakeMode == 1) {
			for(uint j = 0; !do_skip && j < rScDest.getCount(); j++) {
				const Strategy & r_j_item = rScDest.at(j);
				if(r_j_item.StakeMode == r_item.StakeMode && r_j_item.InputFrameSize == r_item.InputFrameSize) {
					if(feqeps(r_j_item.OptDeltaRange.low, r_item.OptDeltaRange.low, _eps) && feqeps(r_j_item.OptDeltaRange.upp, r_item.OptDeltaRange.upp, _eps)) {
						if(r_j_item.MainFrameSize == r_item.MainFrameSize) { // @v10.6.9
							if(feqeps(r_j_item.OptDelta2Range.low, r_item.OptDelta2Range.low, _eps) && feqeps(r_j_item.OptDelta2Range.upp, r_item.OptDelta2Range.upp, _eps)) { // @v10.6.9
								// @v10.6.9 {
								if(pScSkipDueDup) {
									THROW_SL(pScSkipDueDup->insert(&r_item));
								}
								// } @v10.6.9 
								do_skip = 1;
							}
						}
					}
				}
			}
		}
		if(!do_skip) {
			THROW_SL(rScDest.insert(&r_item));
		}
	}
	CATCHZOK
	return ok;
}

const PPObjTimeSeries::Strategy * FASTCALL PPObjTimeSeries::StrategyContainer::SearchByID(uint32 id) const
{
	const PPObjTimeSeries::Strategy * p_result = 0;
	if(id) {
		for(uint i = 0; !p_result && i < getCount(); i++)
			if(at(i).ID == id)
				p_result = &at(i);
	}
	return p_result;
}

uint32 SLAPI PPObjTimeSeries::StrategyContainer::GetNewStrategyId()
{
	++LastStrategyId;
	return LastStrategyId;
}

int SLAPI PPObjTimeSeries::StrategyContainer::GetInputFramSizeList(LongArray & rList, uint * pMaxOptDelta2Stride) const
{
	int    ok = -1;
	uint   max_opt_delta2_stride = 0;
	rList.clear();
	for(uint i = 0; i < getCount(); i++) {
		const Strategy & r_item = at(i);
		rList.addnz(static_cast<long>(r_item.InputFrameSize));
		rList.addnz(static_cast<long>(r_item.MainFrameSize)); // @v10.4.10
		SETMAX(max_opt_delta2_stride, r_item.OptDelta2Stride);
	}
	ASSIGN_PTR(pMaxOptDelta2Stride, max_opt_delta2_stride);
	if(rList.getCount()) {
		rList.sortAndUndup();
		ok = 1;
	}
	return ok;
}

int SLAPI PPObjTimeSeries::StrategyContainer::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	uint32 ver = Ver;
	if(!StorageTm.d)
		StorageTm = getcurdatetime_();
	THROW_SL(pSCtx->Serialize(dir, ver, rBuf));
	THROW_SL(pSCtx->Serialize(dir, StorageTm, rBuf));
	THROW_SL(pSCtx->Serialize(dir, LastValTm, rBuf));
	THROW_SL(pSCtx->Serialize(dir, static_cast<SVector *>(this), rBuf));
	CATCHZOK
	return ok;
}


//static
SString & FASTCALL PPObjTimeSeries::GetStrategySetTypeName(StrategySetType sst, SString & rBuf)
{
	rBuf.Z();
	switch(sst) {
		case sstAll: rBuf = "StrategySetAll"; break;
		case sstSelection: rBuf = "StrategySetSelection"; break;
		//case sstSelectionTrendFollowing: rBuf = "StrategySetSelectionTrendFollowing"; break;
		default: rBuf = "StrategySetUnkn"; break;
	}
	return rBuf;
}

int SLAPI PPObjTimeSeries::PutStrategies(PPID id, StrategySetType sst, StrategyContainer * pL, int use_ta)
{
	int    ok = 1;
	assert(oneof2(sst, sstAll, sstSelection));
	PPTimeSeries ts_rec;
	SBuffer buffer;
	const uint _c = SVectorBase::GetCount(pL);
	PPID   prop = 0;
	THROW(oneof2(sst, sstAll, sstSelection));
	switch(sst) {
		case sstAll: prop = TIMSERPRP_STAKEMODEL_ALL; break;
		case sstSelection: prop = TIMSERPRP_STAKEMODEL_SEL; break;
		//case sstSelectionTrendFollowing: prop = TIMSERPRP_STAKEMODEL_SELTF; break;
		default: assert(0); break;
	}
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(Search(id, &ts_rec) > 0);
		if(_c) {
			SSerializeContext sctx;
			THROW(pL->Serialize(+1, buffer, &sctx));
		}
		THROW(ref->PutPropSBuffer(Obj, id, prop, buffer, 0));
		DS.LogAction(PPACN_TSSTRATEGYUPD, Obj, id, static_cast<long>(_c), 0); // @v10.3.3
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjTimeSeries::GetStrategies(PPID id, StrategySetType sst, StrategyContainer & rL)
{
	int    ok = -1;
	assert(oneof2(sst, sstAll, sstSelection));
	SBuffer buffer;
	PPID   prop = 0;
	THROW(oneof2(sst, sstAll, sstSelection));
	switch(sst) {
		case sstAll: prop = TIMSERPRP_STAKEMODEL_ALL; break;
		case sstSelection: prop = TIMSERPRP_STAKEMODEL_SEL; break;
		//case sstSelectionTrendFollowing: prop = TIMSERPRP_STAKEMODEL_SELTF; break;
		default: assert(0); break;
	}
	rL.clear();
	THROW(ref->GetPropSBuffer(Obj, id, prop, buffer));
	{
		size_t sd_size = buffer.GetAvailableSize();
		if(sd_size) {
			STempBuffer temp_buf(sd_size);
			SSerializeContext sctx;
			THROW(rL.Serialize(-1, buffer, &sctx));
			if(rL.getCount())
				ok = 1;
		}
	}
	CATCH
		rL.clear();
		ok = 0;
	ENDCATCH
	return ok;
}

SLAPI PPObjTimeSeries::TrainNnParam::TrainNnParam(const char * pSymb, long flags) : Strategy(), Symb(pSymb),
	ActionFlags(flags), ForwardFrameSize(0), HiddenLayerDim(0), EpochCount(1), LearningRate(0.1f)
{
}

SLAPI PPObjTimeSeries::TrainNnParam::TrainNnParam(const PPTimeSeriesPacket & rTsPack, long flags) : Strategy(), Symb(rTsPack.Rec.Symb),
	ActionFlags(flags), ForwardFrameSize(0), HiddenLayerDim(0), EpochCount(1), LearningRate(0.1f)
{
	Prec = rTsPack.Rec.Prec;
	Margin = (rTsPack.E.MarginManual > 0.0) ? rTsPack.E.MarginManual : rTsPack.Rec.BuyMarg; // @v10.5.6 rTsPack.E.MarginManual
	SpreadAvg = rTsPack.Rec.AvgSpread;
}

void SLAPI PPObjTimeSeries::TrainNnParam::Reset()
{
	Strategy::Reset();
}

SString & SLAPI PPObjTimeSeries::TrainNnParam::MakeFileName(SString & rBuf) const
{
	return rBuf.Z().Cat(Symb).CatChar('-').Cat(InputFrameSize).CatChar('-').Cat(MaxDuckQuant).Dot().Cat("fann").ToLower();
}

double SLAPI PPObjTimeSeries::Strategy::GetWinCountRate() const { return fdivnz((double)WinCount, (double)StakeCount); }
double SLAPI PPObjTimeSeries::StrategyResultEntry::GetPeakAverage() const { return fdivnz(SumPeak, (double)StakeCount); }
double SLAPI PPObjTimeSeries::StrategyResultEntry::GetBottomAverage() const { return fdivnz(SumBottom, (double)StakeCount); }

//static
double SLAPI PPObjTimeSeries::Strategy::CalcSlTpAdjustment(int prec, double averageSpreadForAdjustment)
{
	const double __spread = (prec > 0 && averageSpreadForAdjustment > 0.0) ? (averageSpreadForAdjustment * fpow10i(-prec)) : 0.0;
	return __spread;
}

double SLAPI PPObjTimeSeries::Strategy::CalcSL(double peak, double averageSpreadForAdjustment) const
{
	const  double mdv = (MaxDuckQuant * SpikeQuant);
	const  double __spread = CalcSlTpAdjustment(Prec, averageSpreadForAdjustment);
	if(BaseFlags & bfShort) {
		return round(peak * (1.0 + mdv) + __spread, Prec);
	}
	else {
		return round(peak * (1.0 - mdv) - __spread, Prec);
	}
}

double SLAPI PPObjTimeSeries::Strategy::CalcSL(double peak, double externalSpikeQuant, double averageSpreadForAdjustment) const
{
	const  double mdv = (MaxDuckQuant * ((externalSpikeQuant > 0.0) ? externalSpikeQuant : SpikeQuant));
	const  double __spread = CalcSlTpAdjustment(Prec, averageSpreadForAdjustment);
	if(BaseFlags & bfShort) {
		return round(peak * (1.0 + mdv) + __spread, Prec);
	}
	else {
		return round(peak * (1.0 - mdv) - __spread, Prec);
	}
}

double SLAPI PPObjTimeSeries::Strategy::CalcTP(double stakeBase, double averageSpreadForAdjustment) const
{
	if(TargetQuant) {
		const  double tv = (TargetQuant * SpikeQuant);
		const  double __spread = CalcSlTpAdjustment(Prec, averageSpreadForAdjustment);
		if(BaseFlags & bfShort) {
			return round(stakeBase * (1.0 - tv) + __spread, Prec);
		}
		else {
			return round(stakeBase * (1.0 + tv) - __spread, Prec);
		}
	}
	else
		return 0.0;
}

double SLAPI PPObjTimeSeries::Strategy::CalcTP(double stakeBase, double externalSpikeQuant, double averageSpreadForAdjustment) const
{
	if(TargetQuant) {
		const  double tv = (TargetQuant * ((externalSpikeQuant > 0.0) ? externalSpikeQuant : SpikeQuant));
		const  double __spread = CalcSlTpAdjustment(Prec, averageSpreadForAdjustment);
		if(BaseFlags & bfShort) {
			return round(stakeBase * (1.0 - tv) + __spread, Prec);
		}
		else {
			return round(stakeBase * (1.0 + tv) - __spread, Prec);
		}
	}
	else
		return 0.0;
}

//static
double SLAPI PPObjTimeSeries::Strategy::CalcSL_withExternalFactors(double peak, bool isShort, int prec, uint maxDuckQuant, double spikeQuant, double averageSpreadForAdjustment)
{
	const  double mdv = (maxDuckQuant * spikeQuant);
	const  double __spread = CalcSlTpAdjustment(prec, averageSpreadForAdjustment);
	if(isShort) {
		return round(peak * (1.0 + mdv) + __spread, prec);
	}
	else {
		return round(peak * (1.0 - mdv) - __spread, prec);
	}
}

//static
double SLAPI PPObjTimeSeries::Strategy::CalcTP_withExternalFactors(double stakeBase, bool isShort, int prec, uint targetQuant, double spikeQuant, double averageSpreadForAdjustment)
{
	if(targetQuant) {
		const  double tv = (targetQuant * spikeQuant);
		const  double __spread = CalcSlTpAdjustment(prec, averageSpreadForAdjustment);
		if(isShort) {
			return round(stakeBase * (1.0 - tv) + __spread, prec);
		}
		else {
			return round(stakeBase * (1.0 + tv) - __spread, prec);
		}
	}
	else
		return 0.0;
}
/*
time-series-precision: точность представления значений
time-series-margin-buy:  маржина при покупке инструмента, представленного символом
time-series-margin-sell: маржина при продаже инструмента, представленного символом
time-series-currency-base: основная валюта
time-series-currency-profit: валюта прибыли
time-series-currency-margin: валюта маржины
time-series-volume-min: минимальный объем сделки
time-series-volume-max: максимальный объем сделки
time-series-volume-step: Минимальный шаг изменения объема для заключения сделки
*/
IMPLEMENT_PPFILT_FACTORY(PrcssrTsStrategyAnalyze); SLAPI PrcssrTsStrategyAnalyzeFilt::PrcssrTsStrategyAnalyzeFilt() : PPBaseFilt(PPFILT_PRCSSRTSSTRATEGYANALYZE, 0, 0)
{
	SetFlatChunk(offsetof(PrcssrTsStrategyAnalyzeFilt, ReserveStart),
		offsetof(PrcssrTsStrategyAnalyzeFilt, TsList)-offsetof(PrcssrTsStrategyAnalyzeFilt, ReserveStart));
	SetBranchObjIdListFilt(offsetof(PrcssrTsStrategyAnalyzeFilt, TsList));
	Init(1, 0);
}

PrcssrTsStrategyAnalyzeFilt & FASTCALL PrcssrTsStrategyAnalyzeFilt::operator = (const PrcssrTsStrategyAnalyzeFilt & rS)
{
	Copy(&rS, 0);
	return *this;
}

SLAPI PrcssrTsStrategyAnalyze::PrcssrTsStrategyAnalyze()
{
}

SLAPI PrcssrTsStrategyAnalyze::~PrcssrTsStrategyAnalyze()
{
}

int SLAPI PrcssrTsStrategyAnalyze::InitParam(PPBaseFilt * pBaseFilt)
{
	int    ok = 1;
	if(P.IsA(pBaseFilt)) {
		PrcssrTsStrategyAnalyzeFilt * p_filt = static_cast<PrcssrTsStrategyAnalyzeFilt *>(pBaseFilt);
		if(p_filt->Flags == 0)
			p_filt->Flags = (PrcssrTsStrategyAnalyzeFilt::fFindOptMaxDuck|PrcssrTsStrategyAnalyzeFilt::fFindStrategies|
				PrcssrTsStrategyAnalyzeFilt::fProcessLong|PrcssrTsStrategyAnalyzeFilt::fProcessShort);
	}
	else
		ok = 0;
	return ok;
}

int SLAPI PrcssrTsStrategyAnalyze::EditParam(PPBaseFilt * pBaseFilt)
{
	class PrcssrTssaDialog : public TDialog {
		DECL_DIALOG_DATA(PrcssrTsStrategyAnalyzeFilt);
	public:
		PrcssrTssaDialog() : TDialog(DLG_TSSA)
		{
		}
		DECL_DIALOG_SETDTS()
		{
			int    ok = 1;
			RVALUEPTR(Data, pData);
			SetupPPObjCombo(this, CTLSEL_TSSA_TS, PPOBJ_TIMESERIES, Data.TsList.GetSingle(), 0/*OLW_LOADDEFONOPEN*/, 0);
			AddClusterAssoc(CTL_TSSA_FLAGS, 0, Data.fFindOptMaxDuck);
			AddClusterAssoc(CTL_TSSA_FLAGS, 1, Data.fFindStrategies);
			AddClusterAssoc(CTL_TSSA_FLAGS, 2, Data.fForce);
			AddClusterAssoc(CTL_TSSA_FLAGS, 3, Data.fProcessLong);
			AddClusterAssoc(CTL_TSSA_FLAGS, 4, Data.fProcessShort);
			AddClusterAssoc(CTL_TSSA_FLAGS, 5, Data.fAutodetectTargets);
			SetClusterData(CTL_TSSA_FLAGS, Data.Flags);
			AddClusterAssocDef(CTL_TSSA_CLOSEMODE, 0, PPObjTimeSeries::Strategy::clsmodFullMaxDuck);
			AddClusterAssoc(CTL_TSSA_CLOSEMODE, 1, PPObjTimeSeries::Strategy::clsmodAdjustLoss);
			SetClusterData(CTL_TSSA_CLOSEMODE, Data.CloseMode);
			Setup();
			return ok;
		}
		DECL_DIALOG_GETDTS()
		{
			int    ok = 1;
			PPID   id = getCtrlLong(CTLSEL_TSSA_TS);
			Data.TsList.Add(id);
			GetClusterData(CTL_TSSA_FLAGS, &Data.Flags);
			GetClusterData(CTL_TSSA_CLOSEMODE, &Data.CloseMode);
			ASSIGN_PTR(pData, Data);
			return ok;
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isCmd(cmTsList)) {
				PPID   id = 0;
				PPIDArray list;
				Data.TsList.Get(list);
				getCtrlData(CTLSEL_TSSA_TS, &id);
				if(id)
					list.addUnique(id);
				ListToListData data(PPOBJ_TIMESERIES, 0, &list);
				data.TitleStrID = PPTXT_SELTIMSERLIST;
				if(ListToListDialog(&data) > 0) {
					Data.TsList.Set(&list);
					Setup();
				}
				clearEvent(event);
			}
		}
		void   Setup()
		{
			PPID   id = Data.TsList.GetSingle();
			PPID   prev_id = getCtrlLong(CTLSEL_TSSA_TS);
			if(id != prev_id)
				setCtrlData(CTLSEL_TSSA_TS, &id);
			if(Data.TsList.GetCount() > 1)
				SetComboBoxListText(this, CTLSEL_TSSA_TS);
			disableCtrl(CTLSEL_TSSA_TS, BIN(Data.TsList.GetCount() > 1));
		}
	};
	if(!P.IsA(pBaseFilt))
		return 0;
	PrcssrTsStrategyAnalyzeFilt * p_filt = static_cast<PrcssrTsStrategyAnalyzeFilt *>(pBaseFilt);
	DIALOG_PROC_BODY(PrcssrTssaDialog, p_filt);
}

int SLAPI PrcssrTsStrategyAnalyze::Init(const PPBaseFilt * pBaseFilt)
{
	int    ok = 1;
	THROW(P.IsA(pBaseFilt));
	P = *static_cast<const PrcssrTsStrategyAnalyzeFilt *>(pBaseFilt);
	CATCHZOK
	return ok;
}

//static
SString & SLAPI PPObjTimeSeries::StrategyToString(const PPObjTimeSeries::Strategy & rS, const PPObjTimeSeries::BestStrategyBlock * pBestResult, SString & rBuf)
{
	rBuf.Z();
	const long trange_fmt = MKSFMTD(0, 10, NMBF_NOTRAILZ|NMBF_FORCEPOS); // @v10.6.8 NMBF_FORCEPOS
	rBuf.Cat("Strategy").CatChar('(').CatLongZ(rS.ID, 5).CatChar(')').CatDiv(':', 2).Cat((rS.BaseFlags & rS.bfShort) ? "S" : "B").CatChar('/');
	rBuf.CatLongZ(rS.InputFrameSize, 3).CatChar('/').Cat(rS.MaxDuckQuant).CatChar(':').Cat(rS.TargetQuant).CatChar('/').Cat(rS.StakeMode);
	rBuf.Space().Cat("Potential").CatDiv(':', 2).
		Cat(rS.V.GetResultPerDay(), MKSFMTD(0, 4, 0)).CatChar('/').
		CatChar('W').Cat(rS.GetWinCountRate(), MKSFMTD(0, 3, 0)).CatChar('/').
		CatChar('#').Cat(rS.StakeCount).CatChar('/').
		CatChar('T').CatLongZ(static_cast<long>(rS.StakeCount ? (rS.V.TmSec / rS.StakeCount) : 0), 6);
	if(rS.MainFrameSize) {
		rBuf.Space().Cat("MF").Cat(rS.MainFrameSize).CatChar('[').Cat(rS.OptDelta2Range, trange_fmt).CatChar(']');
	}
	if(oneof3(rS.StakeMode, 1, 2, 3)) { // @v10.4.5 -(4)
		rBuf.Space().CatChar('[');
		if(pBestResult) {
			if(rS.StakeMode == 1)
				rBuf.Cat(pBestResult->TvForMaxResult, trange_fmt).CatChar('|').Cat(rS.OptDeltaRange, trange_fmt);
			else if(rS.StakeMode == 2)
				rBuf.Cat(pBestResult->TvForMaxResult, trange_fmt).CatChar('|').Cat(rS.OptDelta2Stride).CatChar('/').Cat(rS.OptDelta2Range, trange_fmt);
			else if(rS.StakeMode == 3) { // @v10.4.5 -(4)
				rBuf.Cat(pBestResult->TvForMaxResult, trange_fmt).CatChar('|').Cat(rS.OptDeltaRange, trange_fmt).CatChar(']').Space();
				rBuf.CatChar('[');
				rBuf.Cat(pBestResult->Tv2ForMaxResult, trange_fmt).CatChar('|').Cat(rS.OptDelta2Stride).CatChar('/').Cat(rS.OptDelta2Range, trange_fmt);
			}
		}
		else {
			if(rS.StakeMode == 1)
				rBuf.Cat(rS.OptDeltaRange, trange_fmt);
			else if(rS.StakeMode == 2)
				rBuf.Cat(rS.OptDelta2Stride).CatChar('/').Cat(rS.OptDelta2Range, trange_fmt);
			else if(rS.StakeMode == 3) { // @v10.4.5 -(4)
				rBuf.Cat(rS.OptDeltaRange, trange_fmt).CatChar(']').Space();
				rBuf.CatChar('[');
				rBuf.Cat(rS.OptDelta2Stride).CatChar('/').Cat(rS.OptDelta2Range, trange_fmt);
			}
		}
		rBuf.CatChar(']').Space();
	}
	return rBuf;
}

//static
SString & SLAPI PPObjTimeSeries::StrategyOutput(const PPObjTimeSeries::StrategyResultEntry * pSre, SString & rBuf)
{
	rBuf.Z();
	if(pSre == 0) {
		rBuf.
		Cat("symbol").Space().
		Cat("id").Space().
		Cat("spike-quant").Space().
		Cat("max-duck:target").Space().
		Cat("input-frame-size").Space().
		Cat("result").Space().
		Cat("stake-count").Space().
		Cat("win-count-rel").Space().
		//Cat("total-tm-count").Space().
		Cat("total-tm-hr").Space().
		Cat("result-per-day").Space().
		Cat("peak-avg-quant:peak-max-quant:bottom-avg-quant").Space().
		Cat("opt-delta-freq").Space().
		Cat("opt-delta-result").Space().
		Cat("opt-delta2-freq").Space().
		Cat("opt-delta2-result").Space();
	}
	else {
		SString temp_buf;
		rBuf.Cat(pSre->Symb);
		if(pSre->BaseFlags & pSre->bfShort)
			rBuf.CatChar('-').Cat("REV");
		if(pSre->StakeMode)
			rBuf.CatChar('/').Cat(pSre->StakeMode);
		else
			rBuf.Space().Space();
		rBuf.Space().
			CatLongZ(pSre->ID, 4).Space().
			Cat(pSre->SpikeQuant, MKSFMTD(15, 8, 0)).Space().
			Cat(pSre->MaxDuckQuant).CatChar(':').Cat(pSre->TargetQuant).Space().
			Cat(pSre->InputFrameSize).Space();
		if(pSre->MainFrameSize) {
			rBuf.Cat("MF").Cat(pSre->MainFrameSize).CatChar('[').Cat(pSre->OptDelta2Range, MKSFMTD(0, 12, 0)).CatChar(']').Space();
		}
		rBuf.Cat(pSre->V.Result, MKSFMTD(15, 5, 0)).Space();
		rBuf.Cat(temp_buf.Z().Cat(pSre->StakeCount).Align(7, ADJ_RIGHT)).Space().
			Cat(pSre->GetWinCountRate(), MKSFMTD(15, 5, 0)).Space().
			//Cat(pSre->V.TmCount).Space().
			Cat(pSre->V.TmSec / 3600.0, MKSFMTD(9, 1, 0)).Space().
			Cat(pSre->V.GetResultPerDay(), MKSFMTD(9, 6, 0)).Space().
			CatLongZ(pSre->PeakAvgQuant, 2).CatChar(':').CatLongZ(pSre->PeakMaxQuant, 2).CatChar(':').CatLongZ(pSre->BottomAvgQuant, 2).Space();
		{
			if(pSre->StakeMode != 2) {
				rBuf.Cat(pSre->OptDeltaRange, MKSFMTD(0, 12, NMBF_FORCEPOS)).
					CatChar('/').Cat(pSre->OptDeltaRange.Count).CatChar('/').Cat(pSre->LastResultIdx-pSre->InputFrameSize+1);
			}
			else 
				rBuf.Cat("---");
			rBuf.Space();
			if(pSre->StakeMode == 0)
				rBuf.Cat(pSre->OptDeltaRange.Result, MKSFMTD(15, 5, 0));
			// else rBuf.Cat("---");
			rBuf.Space();
			if(pSre->StakeMode != 1) {
				rBuf.Cat(pSre->OptDelta2Range, MKSFMTD(0, 12, 0)).Space().
				Cat(pSre->OptDelta2Stride).CatChar('/').Cat(pSre->OptDelta2Range.Count).CatChar('/').Cat(pSre->LastResultIdx-pSre->InputFrameSize+1);
			}
			// else rBuf.Cat("---");
			rBuf.Space();
			if(pSre->StakeMode == 0)
				rBuf.Cat(pSre->OptDelta2Range.Result, MKSFMTD(15, 5, 0));
			// else rBuf.Cat("---");
			rBuf.Space();
		}
	}
	return rBuf;
}

static int TuneStrategyTP(const PrcssrTsStrategyAnalyze::ModelParam & rMp, const DateTimeArray & rTmList, const RealArray & rValList,
	const TSCollection <PPObjTimeSeries::TrendEntry> & rTrendListSet, uint leftShift, uint rightShift, PPObjTimeSeries::Strategy & rS, SFile * pFOut)
{
	//const uint16 left_shift = 6; // Количество квантов вниз от текущего оптимуму
	//const uint16 right_shift = 6; // Количество квантов вверх от текущего оптимуму
	// Общий диапазон перебора: [(Strategy::TargetQuant-left_shift)..(Strategy::TargetQuant+right_shift)]
	int    ok = 1;
	//PPObjTimeSeries::Strategy & r_st = sc_selection.at(si);
	const PPObjTimeSeries::TrendEntry * p_trend_entry = PPObjTimeSeries::SearchTrendEntry(rTrendListSet, rS.InputFrameSize);
	if(p_trend_entry) {
		double best_result = 0.0;
		uint16 best_tq = 0;
		//const  uint16 left_shift  = 2 * rS.TargetQuant / 3; // Количество квантов вниз от текущего оптимума
		//const  uint16 right_shift = 0; // Количество квантов вверх от текущего оптимуму
		const  uint16 left_shift  = leftShift; // Количество квантов вниз от текущего оптимума
		const  uint16 right_shift = rightShift; // Количество квантов вверх от текущего оптимуму
		const  uint16 org_target_quant = rS.TargetQuant;
		const  double org_result = rS.V.GetResultPerDay();
		const  uint16 lo_tq = MAX(0, static_cast<int>(org_target_quant)-left_shift);
		const  uint16 up_tq = org_target_quant+right_shift;
		PPObjTimeSeries::StrategyResultEntry best_sre;
		for(uint16 tq = lo_tq; tq <= up_tq; tq++) {
			rS.TargetQuant = tq;
			PPObjTimeSeries::StrategyResultEntry sre_test(rS, rS.StakeMode);
			THROW(TsTestStrategy2(rMp, rTmList, rValList, *p_trend_entry, 0/*main_trend_entry*/, rS, 0, sre_test, 0));
			assert(tq != org_target_quant || feqeps(sre_test.V.GetResultPerDay(), org_result, 1E-7));
			if(sre_test.V.GetResultPerDay() > 0.0) {
				//const double current_result = sre_test.V.GetResultPerDay();
				const double current_result = static_cast<double>(sre_test.WinCount) / static_cast<double>(sre_test.StakeCount);
				if(best_result < current_result) {
					best_result = current_result;
					best_tq = tq;
					best_sre = sre_test;
				}
			}
		}
		rS.TargetQuant = NZOR(best_tq, org_target_quant);
		rS.StakeCount = best_sre.StakeCount;
		rS.WinCount = best_sre.WinCount;
		rS.V = best_sre.V;
		if(pFOut) {
			SString msg_buf;
			SString temp_buf;
			PPObjTimeSeries::StrategyToString(rS, 0, temp_buf);
			msg_buf.Z().Cat("Optimize TargetQuant").CatDiv(':', 2).
				Cat("best-target").Eq().Cat(best_tq).CatChar('/').Cat(org_target_quant).Space().
				Cat("best-result").Eq().Cat(best_result, MKSFMTD(0, 5, 0)).CatChar('/').Cat(org_result, MKSFMTD(0, 5, 0)).Space().
				Cat(temp_buf);
			pFOut->WriteLine(msg_buf.CR());
		}
	}
	CATCHZOK
	return ok;
}

SLAPI PrcssrTsStrategyAnalyze::ModelParam::ModelParam() : Flags(0), OptTargetCriterion(tcAmount), InitTrendErrLimit(0.0),
	BestSubsetDimention(0), BestSubsetMaxPhonyIters(0), BestSubsetOptChunk(0), UseDataSince(ZERODATE), DefTargetQuant(0), MinWinRate(0.0),
	OptRangeStep(0), OptRangeStepMkPart(0), OptRangeMultiLimit(0), MainFrameRangeCount(0)
{
}

PrcssrTsStrategyAnalyze::ModelParam & SLAPI PrcssrTsStrategyAnalyze::ModelParam::Z()
{
	Flags = 0;
	InitTrendErrLimit = 0.0;
	BestSubsetDimention = 0;
	BestSubsetMaxPhonyIters = 0;
	BestSubsetOptChunk = 0;
	MaxDuckQuantList.clear();
	InputFrameSizeList.clear();
	UseDataSince = ZERODATE;
	DefTargetQuant = 0;
	// @v10.5.0 MainFrameSize = 0; // @v10.5.0
	MainFrameRangeCount = 0; // @v10.5.0
	MinWinRate = 0.0;
	OptRangeStep = 0; // @v10.4.7
	OptRangeStepMkPart = 0; // @v10.4.7
	OptRangeMultiLimit = 0; // @v10.4.7
	return *this;
}

int SLAPI PrcssrTsStrategyAnalyze::ReadModelParam(ModelParam & rMp)
{
	int    ok = 1;
	SString temp_buf;
	PPIniFile ini_file;
	rMp.Z();
	if(ini_file.Get(PPINISECT_TSSTAKE, PPINIPARAM_TSSTAKE_INITTRENDERRLIMIT, temp_buf) > 0)
		rMp.InitTrendErrLimit = temp_buf.ToReal();
	rMp.InitTrendErrLimit = inrangeordefault(rMp.InitTrendErrLimit, 1E-6, 10.0, 1.0);
	if(ini_file.Get(PPINISECT_TSSTAKE, PPINIPARAM_TSSTAKE_BESTSUBSETDIMESION, temp_buf) > 0)
		rMp.BestSubsetDimention = static_cast<uint>(temp_buf.ToLong());
	rMp.BestSubsetDimention = inrangeordefault(rMp.BestSubsetDimention, 1, 3000, 100);
	// @v10.4.3 {
	if(ini_file.Get(PPINISECT_TSSTAKE, PPINIPARAM_TSSTAKE_BESTSUBSETTF, temp_buf) > 0) {
		if(temp_buf.IsEqiAscii("true") || temp_buf.IsEqiAscii("yes") || temp_buf.IsEqual("1"))
			rMp.Flags |= rMp.fBestSubsetTrendFollowing;
	}
	// } @v10.4.3
	// @v10.4.5
	{
		if(ini_file.Get(PPINISECT_TSSTAKE, PPINIPARAM_TSSTAKE_OPTRANGETARGET, temp_buf) > 0) {
			if(temp_buf.IsEqiAscii("velocity")) {
				//rMp.Flags |= rMp.fOptRangeTarget_Velocity;
				rMp.OptTargetCriterion = rMp.tcVelocity;
			}
			else if(temp_buf.IsEqiAscii("winratio")) { // @v10.4.6
				//rMp.Flags |= rMp.fOptRangeTarget_WinRatio;
				rMp.OptTargetCriterion = rMp.tcWinRatio;
			}
			else
				rMp.OptTargetCriterion = rMp.tcAmount;
		}
		if(ini_file.Get(PPINISECT_TSSTAKE, PPINIPARAM_TSSTAKE_OPTRANGEMULTI, temp_buf) > 0) {
			if(temp_buf.IsEqiAscii("true") || temp_buf.IsEqiAscii("yes") || temp_buf.IsEqual("1"))
				rMp.Flags |= rMp.fOptRangeMulti;
		}
	}
	// @v10.4.5
	if(ini_file.Get(PPINISECT_TSSTAKE, PPINIPARAM_TSSTAKE_BESTSUBSETMAXPHONY, temp_buf) > 0)
		rMp.BestSubsetMaxPhonyIters = static_cast<uint>(temp_buf.ToLong());
	if(rMp.BestSubsetMaxPhonyIters == 0 || rMp.BestSubsetMaxPhonyIters > 1000)
		rMp.BestSubsetMaxPhonyIters = 7;
	if(ini_file.Get(PPINISECT_TSSTAKE, PPINIPARAM_TSSTAKE_BESTSUBSETOPTCHUNK, temp_buf) > 0)
		rMp.BestSubsetOptChunk = static_cast<uint>(temp_buf.ToLong());
	if(!oneof3(rMp.BestSubsetOptChunk, 3, 7, 15)) // @v10.6.9 (15)
		rMp.BestSubsetOptChunk = 0;
	// @v10.4.2 {
	if(ini_file.Get(PPINISECT_TSSTAKE, PPINIPARAM_TSSTAKE_DEFTARGETQUANT, temp_buf) > 0)
		rMp.DefTargetQuant = static_cast<uint>(temp_buf.ToLong());
	rMp.DefTargetQuant = inrangeordefault(rMp.DefTargetQuant, 1, 200, 18);
	// } @v10.4.2
	// @v10.4.7 {
	if(ini_file.Get(PPINISECT_TSSTAKE, PPINIPARAM_TSSTAKE_OPTRANGE_STEP, temp_buf) > 0) {
		rMp.OptRangeStep = static_cast<uint>(temp_buf.ToLong());
	}
	if(rMp.OptRangeStep > 1000000)
		rMp.OptRangeStep = 0;
	if(ini_file.Get(PPINISECT_TSSTAKE, PPINIPARAM_TSSTAKE_OPTRANGE_STEPMKPART, temp_buf) > 0) {
		rMp.OptRangeStepMkPart = static_cast<uint>(temp_buf.ToLong());
	}
	if(rMp.OptRangeStepMkPart > 100000) {
		rMp.OptRangeStepMkPart = 0;
	}
	if(ini_file.Get(PPINISECT_TSSTAKE, PPINIPARAM_TSSTAKE_OPTRANGEMULTI_LIMIT, temp_buf) > 0) {
		rMp.OptRangeMultiLimit = static_cast<uint>(temp_buf.ToLong());
	}
	if(rMp.OptRangeMultiLimit > 100)
		rMp.OptRangeMultiLimit = 0;
	// } @v10.4.7

	// @v10.4.1 {
	if(ini_file.Get(PPINISECT_TSSTAKE, PPINIPARAM_TSSTAKE_USEDATASINCE, temp_buf) > 0)
		strtodate(temp_buf.Strip(), DATF_DMY, &rMp.UseDataSince);
	if(!checkdate(rMp.UseDataSince))
		rMp.UseDataSince = ZERODATE;
	// } @v10.4.1
	// @v10.4.2 {
	if(ini_file.Get(PPINISECT_TSSTAKE, PPINIPARAM_TSSTAKE_MINWINRATE, temp_buf) > 0) {
		rMp.MinWinRate = temp_buf.ToReal();
	}
	rMp.MinWinRate = inrangeordefault(rMp.MinWinRate, 0.0, 100.0, 0.0);
	// } @v10.4.2
	// @v10.4.9 {
	{
		if(ini_file.Get(PPINISECT_TSSTAKE, PPINIPARAM_TSSTAKE_MAINFRAMESIZE, temp_buf) > 0) {
			StringSet ss(',', temp_buf);
			for(uint ssp = 0; ss.get(&ssp, temp_buf);) {
				const long md = temp_buf.ToLong();
				if(md > 0 && md <= (1440*180))
					rMp.MainFrameSizeList.add(md);
			}
			rMp.MainFrameSizeList.sortAndUndup();
			//rMp.MainFrameSize = static_cast<uint>(temp_buf.ToLong());
		}
		/*if(rMp.MainFrameSize > 10000) {
			rMp.MainFrameSize = 0;
		}*/
		if(rMp.MainFrameSizeList.getCount() && ini_file.Get(PPINISECT_TSSTAKE, PPINIPARAM_TSSTAKE_MAINFRAMERANGECOUNT, temp_buf) > 0) {
			rMp.MainFrameRangeCount = static_cast<uint>(temp_buf.ToLong());
			if(rMp.MainFrameRangeCount > 12)
				rMp.MainFrameRangeCount = 3;
		}
		else
			rMp.MainFrameRangeCount = 0;
	}
	// } @v10.4.9
	if(ini_file.Get(PPINISECT_TSSTAKE, PPINIPARAM_TSSTAKE_MAXDUCKQUANT, temp_buf) > 0) {
		StringSet ss(',', temp_buf);
		for(uint ssp = 0; ss.get(&ssp, temp_buf);) {
			const long md = temp_buf.ToLong();
			if(md > 0 && md <= 300)
				rMp.MaxDuckQuantList.add(md);
		}
		rMp.MaxDuckQuantList.sortAndUndup();
	}
	if(rMp.MaxDuckQuantList.getCount() == 0) {
		// @v10.4.3 rMp.MaxDuckQuantList.add(28);
		// @v10.4.3 rMp.MaxDuckQuantList.add(30);
		// @v10.4.3 rMp.MaxDuckQuantList.add(32);
		// @v10.4.3 rMp.MaxDuckQuantList.add(34);
		// @v10.4.3 rMp.MaxDuckQuantList.add(36);
		// @v10.4.3 rMp.MaxDuckQuantList.add(38);
		// @v10.4.3 rMp.MaxDuckQuantList.add(40);
		// @v10.4.3 rMp.MaxDuckQuantList.add(42);
		rMp.MaxDuckQuantList.add(30); // @v10.4.3
	}
	// @v10.4.3 {
	if(ini_file.Get(PPINISECT_TSSTAKE, PPINIPARAM_TSSTAKE_TARGETQUANT, temp_buf) > 0) {
		StringSet ss(',', temp_buf);
		for(uint ssp = 0; ss.get(&ssp, temp_buf);) {
			const long md = temp_buf.ToLong();
			if(md > 0 && md <= 300)
				rMp.TargetQuantList.add(md);
		}
		rMp.TargetQuantList.sortAndUndup();
	}
	// } @v10.4.3
	if(ini_file.Get(PPINISECT_TSSTAKE, PPINIPARAM_TSSTAKE_INPUTFRAMESIZE, temp_buf) > 0) {
		StringSet ss(',', temp_buf);
		for(uint ssp = 0; ss.get(&ssp, temp_buf);) {
			const long ifs = temp_buf.ToLong();
			if(ifs > 0 && ifs <= 10080)
				rMp.InputFrameSizeList.add(ifs);
		}
		rMp.InputFrameSizeList.sortAndUndup();
	}
	if(rMp.InputFrameSizeList.getCount() == 0) {
		rMp.InputFrameSizeList.add(15);
		rMp.InputFrameSizeList.add(30);
		rMp.InputFrameSizeList.add(60);
		rMp.InputFrameSizeList.add(90);
		rMp.InputFrameSizeList.add(120);
		rMp.InputFrameSizeList.add(150);
		rMp.InputFrameSizeList.add(280);
		rMp.InputFrameSizeList.add(440);
		rMp.InputFrameSizeList.add(600);
	}
	return ok;
}

int SLAPI PrcssrTsStrategyAnalyze::GetTimeSeries(PPID tsID, ModelParam & rMp, STimeSeries & rTs)
{
	int gtsr = TsObj.GetTimeSeries(tsID, rTs);
	if(gtsr > 0) {
		if(checkdate(rMp.UseDataSince)) {
			STimeSeries temp_ts;
			SUniTime ut_since;
			ut_since.Set(rMp.UseDataSince);
			if(rTs.GetChunkRecentSince(ut_since, temp_ts) > 0)
				rTs = temp_ts;
			else
				gtsr = 0;
		}
	}
	return gtsr;
}

struct TsMainFrameRange {
	uint   MainFrameSize;
	RealRange R;
};

static SString & OutputStategyResultEntry(const PPObjTimeSeries::StrategyResultEntry & rEntry, SString & rBuf)
{
	rBuf.Z().
		CatEq("Result", rEntry.V.Result, MKSFMTD(15, 5, 0)).Tab().
		CatEq("StakeCount", rEntry.StakeCount).Tab().
		CatEq("WinCountRate", rEntry.GetWinCountRate(), MKSFMTD(15, 5, 0)).Tab().
		CatEq("TmCount", rEntry.V.TmCount).Tab().
		CatEq("TmSec", rEntry.V.TmSec).Tab().
		CatEq("TotalSec", rEntry.TotalSec).Tab().
		CatEq("ResultPerTotalDay", fdivnz(3600.0 * 24.0 * rEntry.V.Result, static_cast<double>(rEntry.TotalSec)), MKSFMTD(0, 12, 0));
	return rBuf;
}

static SString & OutputLongArrayValues(const LongArray & rList, const char * pTitle, SString & rBuf)
{
	rBuf.Cat(pTitle).CatChar('=');
	for(uint ii = 0; ii < rList.getCount(); ii++)
		rBuf.CatDivConditionally(',', 0, LOGIC(ii)).Cat(rList.get(ii));
	return rBuf;
}

// @v10.6.5 @construction
int SLAPI PrcssrTsStrategyAnalyze::FindOptimalMaxDuck(const PPTimeSeriesPacket & rTsPack, const DateTimeArray & rTsTmList, const RealArray & rTsValList, uint flags, uint * pResult)
{
	int    ok = 1;
	const  int is_short = BIN(flags & fomdfShort);
	const  double spike_quant = rTsPack.Rec.SpikeQuant;
	const  uint32 org_opt_max_duck_val = is_short ? rTsPack.Rec.OptMaxDuck_S : rTsPack.Rec.OptMaxDuck;
	uint32 cur_opt_max_duck_val = org_opt_max_duck_val;
	PPObjTimeSeries::TrainNnParam tnnp(rTsPack, PPObjTimeSeries::TrainNnParam::afAnalyzeFrame);
	tnnp.SpikeQuant = spike_quant;
	tnnp.EpochCount = 1;
	tnnp.InputFrameSize = 0;
	tnnp.MaxDuckQuant = 0;
	tnnp.StakeThreshold = 0.05;
	//assert(oneof2(P.CloseMode, tnnp.clsmodFullMaxDuck, tnnp.clsmodAdjustLoss));
	//tnnp.StakeCloseMode = static_cast<uint16>(P.CloseMode);
	tnnp.StakeCloseMode = tnnp.clsmodAdjustLoss;
	SETFLAG(tnnp.BaseFlags, tnnp.bfShort, is_short);
	{
		PPObjTimeSeries::FactorToResultRelation opt_max_duck;
		PPObjTimeSeries::FactorToResultRelation opt_peak;
		{
			IntRange md_range;
			int    md_step = 0;
			if(flags & fomdfEntireRange) {
				md_range.Set(20, 1000);
				md_step = 20;
			}
			else {
				md_range.Set(50, 500);
				md_step = 50;
			}
			TSVector <PPObjTimeSeries::FactorToResultRelation> opt_max_duck_set;
			THROW(TsObj.FindOptimalFactor(rTsTmList, rTsValList, tnnp, 0/*what*/, md_range, md_step, BIN(flags & fomdfEntireRange), opt_max_duck_set, opt_max_duck));
			cur_opt_max_duck_val = opt_max_duck.FactorQuant;
		}
	}
	if(flags & fomdfStoreResult) {
		PPID   temp_id = rTsPack.Rec.ID;
		PPTimeSeriesPacket ts_pack_to_upd;
		THROW(TsObj.GetPacket(temp_id, &ts_pack_to_upd) > 0);
		if(is_short)
			ts_pack_to_upd.Rec.OptMaxDuck_S = cur_opt_max_duck_val;
		else
			ts_pack_to_upd.Rec.OptMaxDuck = cur_opt_max_duck_val;
		THROW(TsObj.PutPacket(&temp_id, &ts_pack_to_upd, 1));
	}
	CATCHZOK
	ASSIGN_PTR(pResult, cur_opt_max_duck_val);
	return ok;
}

void PPObjTimeSeries::TrendEntry::SqrtErrList(StatBase * pS)
{
	for(uint trlidx = 0; trlidx < ErrL.getCount(); trlidx++) {
		double err = ErrL.at(trlidx);
		assert(err >= 0.0);
		if(err > 0.0) {
			double sqr_err = sqrt(err);
			CALLPTRMEMB(pS, Step(sqr_err));
			ErrL.at(trlidx) = sqr_err;
		}
	}
}

int SLAPI PrcssrTsStrategyAnalyze::MakeArVectors(const STimeSeries & rTs, const LongArray & rFrameSizeList, uint flags, TSCollection <PPObjTimeSeries::TrendEntry> & rTrendListSet)
{
	class MakeArVectorTask : public SlThread {
	public:
		struct InitBlock {
			InitBlock(const STimeSeries & rTs, uint inputFrameSize, long flags, PPObjTimeSeries::TrendEntry * pResult) :
				R_Ts(rTs), InputFrameSize(inputFrameSize), Flags(flags), P_Result(pResult)
			{
			}
			const STimeSeries & R_Ts;
			const uint InputFrameSize;
			const long Flags;
			PPObjTimeSeries::TrendEntry * P_Result;
		};
		MakeArVectorTask(InitBlock * pBlk) : SlThread(0), B(*pBlk)
		{
			InitStartupSignal();
		}
		virtual void Run()
		{
			assert(SLS.GetConstTLA().Id == GetThreadID());
			const  uint tsc = B.R_Ts.GetCount();
			RealArray temp_real_list;
			STimeSeries::AnalyzeFitParam afp(B.InputFrameSize, 0, 0);
			if(B.R_Ts.AnalyzeFit("close", afp, &B.P_Result->TL, 0, &temp_real_list, 0, 0)) {
				assert(B.P_Result->TL.getCount() == tsc);
				assert(temp_real_list.getCount() == tsc);
				{
					StatBase trls(0);
					B.P_Result->ErrL = temp_real_list;
					if(B.Flags & mavfDontSqrtErrList) {
						for(uint trlidx = 0; trlidx < B.P_Result->ErrL.getCount(); trlidx++) {
							double err = B.P_Result->ErrL.at(trlidx);
							assert(err >= 0.0);
							if(err > 0.0) {
								double sqr_err = sqrt(err);
								trls.Step(sqr_err);
							}
						}
					}
					else {
						B.P_Result->SqrtErrList(&trls);
					}
					trls.Finish();
					B.P_Result->ErrAvg = trls.GetExp();
				}
			}
			else {
				// @todo @error
			}
		}
	private:
		virtual void SLAPI Startup()
		{
			SlThread::Startup();
			SignalStartup();
		}
		InitBlock B;
	};
	int    ok = 1;
	const  uint tsc = rTs.GetCount();
	SString msg_buf;
	RealArray temp_real_list;
	rTrendListSet.freeAll();

	const size_t thread_limit = 32;
	PPObjTimeSeries::TrendEntry * thread_result_list[thread_limit];
	uint   thread_inpfrmsz_list[thread_limit];
	const  uint max_threads = 4;
	uint   thr_idx = 0;
	memzero(thread_result_list, sizeof(thread_result_list));
	memzero(thread_inpfrmsz_list, sizeof(thread_inpfrmsz_list));
	for(uint ifsidx = 0; ifsidx < rFrameSizeList.getCount(); ifsidx++) {
		const uint input_frame_size = static_cast<uint>(rFrameSizeList.get(ifsidx));
		assert(PPObjTimeSeries::SearchTrendEntry(rTrendListSet, input_frame_size) == 0);
		PPObjTimeSeries::TrendEntry * p_new_trend_entry = new PPObjTimeSeries::TrendEntry(input_frame_size, tsc);
		THROW_SL(p_new_trend_entry);
		if(max_threads == 0) {
			PROFILE_START
			{
				{
					msg_buf.Z().Cat("AnalyzeFit").Space().Cat(/*ts_pack*/rTs.GetSymb()).Space().Cat(input_frame_size);
					PPWaitMsg(msg_buf);
				}
				STimeSeries::AnalyzeFitParam afp(input_frame_size, 0, 0);
				THROW(rTs.AnalyzeFit("close", afp, &p_new_trend_entry->TL, 0, &temp_real_list, 0, 0));
				assert(p_new_trend_entry->TL.getCount() == tsc);
				assert(temp_real_list.getCount() == tsc);
				{
					StatBase trls(0);
					p_new_trend_entry->ErrL = temp_real_list;
					if(flags & mavfDontSqrtErrList) {
						for(uint trlidx = 0; trlidx < p_new_trend_entry->ErrL.getCount(); trlidx++) {
							double err = p_new_trend_entry->ErrL.at(trlidx);
							assert(err >= 0.0);
							if(err > 0.0) {
								double sqr_err = sqrt(err);
								trls.Step(sqr_err);
							}
						}
					}
					else {
						p_new_trend_entry->SqrtErrList(&trls);
					}
					trls.Finish();
					p_new_trend_entry->ErrAvg = trls.GetExp();
				}
				{
					PPWait(0);
				}
			}
			PROFILE_END
			rTrendListSet.insert(p_new_trend_entry);
		}
		else {
			assert(thr_idx <= max_threads);
			if(thr_idx >= max_threads) {
				HANDLE objs_to_wait[thread_limit];
				size_t objs_to_wait_count = 0;
				{
					for(uint i = 0; i < thr_idx; i++) {
						MakeArVectorTask::InitBlock tb(rTs, thread_inpfrmsz_list[i], flags, thread_result_list[i]);
						MakeArVectorTask * p_thread = new MakeArVectorTask(&tb);
						THROW_S(p_thread, SLERR_NOMEM);
						p_thread->Start(1/*0*/);
						objs_to_wait[objs_to_wait_count++] = *p_thread;
					}
				}
				::WaitForMultipleObjects(objs_to_wait_count, objs_to_wait, TRUE, INFINITE);
				{
					for(uint i = 0; i < thr_idx; i++) {
						rTrendListSet.insert(thread_result_list[i]);
					}
				}
				thr_idx = 0;
				memzero(thread_result_list, sizeof(thread_result_list));
				memzero(thread_inpfrmsz_list, sizeof(thread_inpfrmsz_list));
			}
			thread_result_list[thr_idx] = p_new_trend_entry;
			thread_inpfrmsz_list[thr_idx] = input_frame_size;
			thr_idx++;
		}
	}
	if(thr_idx) {
		assert(max_threads > 0);
		HANDLE objs_to_wait[thread_limit];
		size_t objs_to_wait_count = 0;
		{
			for(uint i = 0; i < thr_idx; i++) {
				MakeArVectorTask::InitBlock tb(rTs, thread_inpfrmsz_list[i], flags, thread_result_list[i]);
				MakeArVectorTask * p_thread = new MakeArVectorTask(&tb);
				THROW_S(p_thread, SLERR_NOMEM);
				p_thread->Start(1/*0*/);
				objs_to_wait[objs_to_wait_count++] = *p_thread;
			}
		}
		::WaitForMultipleObjects(objs_to_wait_count, objs_to_wait, TRUE, INFINITE);
		{
			for(uint i = 0; i < thr_idx; i++) {
				rTrendListSet.insert(thread_result_list[i]);
			}
		}
		thr_idx = 0;
		memzero(thread_result_list, sizeof(thread_result_list));
		memzero(thread_inpfrmsz_list, sizeof(thread_inpfrmsz_list));
	}
	CATCHZOK
	return ok;
}

int SLAPI PrcssrTsStrategyAnalyze::FindStrategies(const ModelParam & rModelParam, PPObjTimeSeries::TrainNnParam & rTnnp2, uint targetQuant, const DateTimeArray & rTsTmList, const RealArray & rTsValList, 
	const PPObjTimeSeries::TrendEntry & rTe, const PPObjTimeSeries::TrendEntry * pMainTrendEntry, const int optFactorSide, PPObjTimeSeries::StrategyContainer & rSContainer)
{
	int    ok = 1;
#if 0 // @construction {
	SString temp_buf;
	SString msg_buf;
	rTnnp2.TargetQuant = targetQuant; //static_cast<uint16>(target_quant_list.at(tqidx));
	{
		PPObjTimeSeries::StrategyResultEntry sre(rTnnp2, 0);
		TSVector <PPObjTimeSeries::StrategyResultEntry> sr_raw_list;
		int    tstso = tstsofroMode1;
		if(optFactorSide == 0)
			tstso |= tstsofroPositive;
		else if(optFactorSide == 1)
			tstso |= tstsofroNegative;
		THROW(TsTestStrategy2(rModelParam, rTsTmList, rTsValList, rTe, pMainTrendEntry, rTnnp2, tstso, sre, &sr_raw_list));
		for(uint srridx = 0; srridx < sr_raw_list.getCount(); srridx++) {
			PPObjTimeSeries::StrategyResultEntry & r_sr_raw = sr_raw_list.at(srridx);
			PPObjTimeSeries::StrategyResultEntry sre_test(r_sr_raw, 1/*stake_mode*/);
			STRNSCPY(sre_test.Symb, ts_pack.GetSymb());
			THROW(TsTestStrategy2(rModelParam, rTsTmList, rTsValList, rTe, pMainTrendEntry, r_sr_raw, tstso, sre_test, 0));
			sre_test.ID = rSContainer.GetNewStrategyId(); // @v10.6.11 
			{
				// "Symb" "Dir" "OFS" "Frame" "MainFrame" "Duck" "Target" "RPD" "WinRt" "StkCnt" "AvgTm" "RngLo" "RngUp"
				msg_buf.Z().Cat(ts_pack.GetSymb()).Semicol();
					msg_buf.Cat((stake_side == 1) ? "sale" : "buy").Semicol();
					msg_buf.Cat((optFactorSide == 0) ? "M1+" : ((optFactorSide == 1) ? "M1-" : "M1*")).Semicol();
					msg_buf.Cat(sre_test.InputFrameSize).Semicol();
					msg_buf.Cat(sre_test.MainFrameSize).Semicol(); // @v10.4.12
					msg_buf.Cat(sre_test.MaxDuckQuant).Semicol();
					msg_buf.Cat(sre_test.TargetQuant).Semicol();
					msg_buf.Cat(sre_test.V.GetResultPerDay(), MKSFMTD(9, 6, 0)).Semicol();
					msg_buf.Cat(sre_test.GetWinCountRate(), MKSFMTD(15, 5, 0)).Semicol();
					msg_buf.Cat(sre_test.StakeCount).Semicol();
					msg_buf.Cat(sre_test.V.TmSec).Semicol();
					msg_buf.Cat(sre_test.OptDeltaRange.low, MKSFMTD(0, 12, 0)).Semicol();
					msg_buf.Cat(sre_test.OptDeltaRange.upp, MKSFMTD(0, 12, 0)).Semicol();
					if(sre_test.MainFrameSize) {
						msg_buf.Cat(sre_test.OptDelta2Range.low, MKSFMTD(0, 12, 0)).Semicol();
						msg_buf.Cat(sre_test.OptDelta2Range.upp, MKSFMTD(0, 12, 0)).Semicol();
					}
					msg_buf.Cat(sre_test.TrendErrLim, MKSFMTD(0, 12, 0)).Semicol(); // @v10.5.0
					msg_buf.Cat(sre_test.TrendErrAvg, MKSFMTD(0, 12, 0)).Semicol(); // @v10.5.0
				f_dump.WriteLine(msg_buf.CR());
			}
			f_out.WriteLine(msg_buf.Z().Cat((optFactorSide == 0) ? "M1+" : ((optFactorSide == 1) ? "M1-" : "M1*")).Space().
				Cat(PPObjTimeSeries::StrategyOutput(&sre_test, temp_buf)).CR());
			if(sre_test.StakeCount > 0) {
				THROW_SL(rSContainer.insert(dynamic_cast <PPObjTimeSeries::Strategy *>(&sre_test)));
			}
			f_out.Flush();
			// @v10.6.10 {
			if(rModelParam.MinWinRate > 0.0 && sre_test.GetWinCountRate() <= rModelParam.MinWinRate) {
				//
				// Нет смысла тестировать последующие стратегии в этом наборе поскольку уровень нажедности 
				// для каждого следующего кандидата снижается.
				//
				break;
			}
			// } @v10.6.10 
		}
	}
	CATCHZOK
#endif // } 0 @construction
	return ok;
}

int SLAPI PrcssrTsStrategyAnalyze::Run()
{
	const int force_fixed_maxduck_values = 1;
	// @v10.6.10 (strictly 'yes') const int fixed_target_quant = 1;
	int    ok = 1;
	const  LDATETIME now = getcurdatetime_();
	ModelParam model_param;
	SString temp_buf;
	SString symb_buf;
	SString save_file_name;
	SString msg_buf;
	PPIDArray id_pre_list;
	RealArray temp_real_list;
	PPTimeSeriesPacket ts_pack;
	TSVector <PPObjTimeSeries::QuoteReqEntry> quote_req_list;
	if(P.TsList.GetCount()) {
		P.TsList.Get(id_pre_list);
	}
	else {
		PPTimeSeries ts_rec;
		for(SEnum en = TsObj.Enum(0); en.Next(&ts_rec) > 0;)
			id_pre_list.add(ts_rec.ID);
	}
	THROW(ReadModelParam(model_param));
	if(id_pre_list.getCount()) {
		id_pre_list.sortAndUndup();
		PPIDArray id_list;
		if(P.Flags & P.fAutodetectTargets) {
			//
			// При автодетекте серий, требующих пересчета формируем список в порядке убывания времени
			// последнего изменения модели. Те серии, у которых нет модели вообще окажутся в начале списка
			// (будут пересчитаны в первую очередь)
			//
			struct TempEntry {
				LDATETIME StorageDtm;
				PPID   ID;
			};
			SVector temp_list(sizeof(TempEntry));
			for(uint i1 = 0; i1 < id_pre_list.getCount(); i1++) {
				const PPID id = id_pre_list.get(i1);
				PPTimeSeries ts_rec;
				if(TsObj.Search(id, &ts_rec) > 0) {
					PPObjTimeSeries::StrategyContainer sc;
					TempEntry new_entry;
					new_entry.ID = id;
					new_entry.StorageDtm = (TsObj.GetStrategies(id, PPObjTimeSeries::sstSelection, sc) > 0) ? sc.GetStorageTm() : ZERODATETIME;
					THROW_SL(temp_list.insert(&new_entry));
				}
			}
			temp_list.sort(PTR_CMPFUNC(LDATETIME));
			for(uint i2 = 0; i2 < temp_list.getCount(); i2++) {
				THROW_SL(id_list.add(static_cast<const TempEntry *>(temp_list.at(i2))->ID));
			}
		}
		else
			id_list = id_pre_list;
		//
		PPWait(1);
		TSCollection <PPObjTimeSeries::TrendEntry> trend_list_set;
		SString out_file_name;
		SString out_total_file_name; // @v10.5.8
		SString strategy_dump_file_name;
		PPGetFilePath(PPPATH_OUT, "AnalyzeTsStrategy2.txt", out_file_name);
		PPGetFilePath(PPPATH_OUT, "AnalyzeTsStrategy2-total.txt", out_total_file_name);
		temp_buf.Z().Cat("TsStrategyDump").CatChar('-').Cat(now.d, DATF_YMD|DATF_CENTURY|DATF_NODIV).Cat(now.t, TIMF_HMS|TIMF_NODIV).Dot().Cat("txt");
		PPGetFilePath(PPPATH_OUT, temp_buf, strategy_dump_file_name);
		const int is_out_file_exists = fileExists(out_file_name);
		SFile f_out(out_file_name, SFile::mAppend);
		SFile f_out_total(out_total_file_name, SFile::mAppend); // @v10.5.8
		SFile f_dump(strategy_dump_file_name, SFile::mWrite);
		{
			msg_buf.Z().Cat("Symb").Semicol().Cat("Dir").Semicol().Cat("OFS").Semicol().Cat("Frame").Semicol().Cat("MainFrame").Semicol().Cat("Duck").Semicol().Cat("Target").Semicol().
				Cat("RPD").Semicol().Cat("WinRt").Semicol().Cat("StkCnt").Semicol().Cat("AvgTm").Semicol().Cat("RngLo").Semicol().Cat("RngUp").Semicol().
				Cat("TrendErrLim").Semicol().Cat("TrendErrAvg");
			f_dump.WriteLine(msg_buf.CR());
		}
		f_out.WriteLine(msg_buf.Z().CatCharN('-', 20).CR());
		{
			msg_buf.Z().Cat(getcurdatetime_(), DATF_ISO8601|DATF_CENTURY, 0);
			msg_buf.CR();
			msg_buf.CatEq("init_trend_err_limit", model_param.InitTrendErrLimit, MKSFMTD(0, 3, 0));
			msg_buf.CR();
			OutputLongArrayValues(model_param.InputFrameSizeList, "input_frame_size", msg_buf).CR();
			OutputLongArrayValues(model_param.MainFrameSizeList, "main_frame_size", msg_buf).CR();
			msg_buf.CatEq("main_frame_range_count", model_param.MainFrameRangeCount);
			msg_buf.CR();
			OutputLongArrayValues(model_param.MaxDuckQuantList, "max_duck_quant", msg_buf).CR();
			OutputLongArrayValues(model_param.TargetQuantList, "target_quant", msg_buf).CR();
			msg_buf.CatEq("def_target_quant", model_param.DefTargetQuant);
			msg_buf.CR();
			{
				msg_buf.Cat("opt_range_target").CatChar('=');
				if(model_param.OptTargetCriterion == model_param.tcWinRatio)
					msg_buf.Cat("winratio");
				else if(model_param.OptTargetCriterion == model_param.tcVelocity)
					msg_buf.Cat("velocity");
				else
					msg_buf.Cat("amount");
			}
			{
				msg_buf.CR();
				msg_buf.CatEq("opt_range_multi", STextConst::GetBool(model_param.Flags & model_param.fOptRangeMulti));
				msg_buf.CR();
				msg_buf.CatEq("opt_range_multi_limit", model_param.OptRangeMultiLimit);
				msg_buf.CR();
				msg_buf.CatEq("opt_range_step", model_param.OptRangeStep);
				msg_buf.CR();
				msg_buf.CatEq("opt_range_stepmkpart", model_param.OptRangeStepMkPart);
			}
			msg_buf.CR();
			msg_buf.CatEq("min_win_rate", model_param.MinWinRate, MKSFMTD(0, 2, 0));
			msg_buf.CR();
			msg_buf.CatEq("best_subset_dimension", model_param.BestSubsetDimention);
			msg_buf.CR();
			msg_buf.CatEq("best_subset_trendfollowing", STextConst::GetBool(model_param.Flags & model_param.fBestSubsetTrendFollowing));
			msg_buf.CR();
			msg_buf.CatEq("best_subset_opt_chunk", model_param.BestSubsetOptChunk);
			msg_buf.CR();
			msg_buf.CatEq("best_subset_max_phony_iters", model_param.BestSubsetMaxPhonyIters);
			// @v10.4.1 {
			if(checkdate(model_param.UseDataSince)) {
				msg_buf.CR();
				msg_buf.CatEq("use_data_since", model_param.UseDataSince, DATF_DMY|DATF_CENTURY);
			}
			// } @v10.4.1
			msg_buf.CR();
			f_out.WriteLine(msg_buf);
			for(uint j = 0; j < id_list.getCount(); j++) {
				const PPID id = id_list.get(j);
				if(TsObj.GetPacket(id, &ts_pack) > 0) {
					PPObjTimeSeries::StrategyContainer sc;
					LDATETIME strg_dtm = ZERODATETIME;
					LDATETIME lastval_dtm = ZERODATETIME;
					uint   sver = 0;
					if(TsObj.GetStrategies(id, PPObjTimeSeries::sstSelection, sc) > 0) {
						strg_dtm = sc.GetStorageTm();
						lastval_dtm = sc.GetLastValTm();
						sver = sc.GetVersion();
					}
					const uint target_quant = (ts_pack.Rec.TargetQuant > 0 && ts_pack.Rec.TargetQuant <= 200) ? ts_pack.Rec.TargetQuant : model_param.DefTargetQuant;
					msg_buf.Z().Cat(ts_pack.Rec.Symb).Space().Cat(sver).Space().Cat(strg_dtm, DATF_ISO8601|DATF_CENTURY, 0).Space().
						Cat(lastval_dtm, DATF_ISO8601|DATF_CENTURY).Space().CatEq("TargetQuant", target_quant);
					f_out.WriteLine(msg_buf.CR());
				}
			}
		}
		{
			f_out.WriteLine(msg_buf.Z().CR());
			f_out.WriteLine(PPObjTimeSeries::StrategyOutput(0, msg_buf).CR());
		}
		f_out.Flush();
		for(uint i = 0; i < id_list.getCount(); i++) {
			const PPID id = id_list.get(i);
			if(TsObj.GetPacket(id, &ts_pack) > 0) {
				STimeSeries ts;
				/* @v10.4.11 int gtsr = TsObj.GetTimeSeries(id, ts);
				if(gtsr > 0) {
					if(checkdate(model_param.UseDataSince)) {
						STimeSeries temp_ts;
						SUniTime ut_since;
						ut_since.Set(model_param.UseDataSince);
						if(ts.GetChunkRecentSince(ut_since, temp_ts) > 0)
							ts = temp_ts;
						else
							gtsr = 0;
					}
				}*/
				const int gtsr = GetTimeSeries(id, model_param, ts); // @v10.4.11
				if(gtsr > 0) {
					const uint tsc = ts.GetCount();
					const uint target_quant = (ts_pack.Rec.TargetQuant > 0 && ts_pack.Rec.TargetQuant <= 200) ? ts_pack.Rec.TargetQuant : model_param.DefTargetQuant;
					//const uint main_frame_size = (model_param.MainFrameSize && model_param.MainFrameRangeCount) ? model_param.MainFrameSize : 0;
					const bool use_main_frame = LOGIC(model_param.MainFrameSizeList.getCount() && model_param.MainFrameRangeCount);
					STimeSeries::Stat st(0);
					uint   vec_idx = 0;
					THROW_SL(ts.GetValueVecIndex("close", &vec_idx));
					ts.Analyze("close", st);
					if(ts_pack.Rec.SpikeQuant <= 0.0 || (P.Flags & P.fForce)) {
						ts_pack.Rec.SpikeQuant = st.DeltaAvg / 2.0;
						PPID   temp_id = id;
						THROW(TsObj.PutPacket(&temp_id, &ts_pack, 1));
					}
					{
						const  double spike_quant = ts_pack.Rec.SpikeQuant;
						//uint32 last_strategy_id = 0;
						DateTimeArray ts_tm_list;
						RealArray ts_val_list;
						PPObjTimeSeries::StrategyContainer scontainer;
						TSVector <PPObjTimeSeries::StrategyResultEntry> sr_raw_list;
						TSVector <TsMainFrameRange> main_frame_range_list;
						THROW_SL(ts.GetRealArray(vec_idx, 0, tsc, ts_val_list));
						THROW_SL(ts.GetTimeArray(0, tsc, ts_tm_list));
						assert(ts_tm_list.getCount() == tsc);
						assert(ts_val_list.getCount() == tsc);
						const LDATETIME last_val_dtm = ts_tm_list.get(tsc-1);
						scontainer.SetLastValTm(last_val_dtm);
						if(P.Flags & P.fFindStrategies) {
							trend_list_set.freeAll();
							LongArray frame_size_list = model_param.InputFrameSizeList;
							if(use_main_frame) {
								for(uint mfsidx = 0; mfsidx < model_param.MainFrameSizeList.getCount(); mfsidx++) {
									frame_size_list.addUnique(model_param.MainFrameSizeList.get(mfsidx));
								}
							}
							{
								THROW(MakeArVectors(ts, frame_size_list, 0, trend_list_set));
								if(use_main_frame) {
									for(uint tlsidx = 0; tlsidx < trend_list_set.getCount(); tlsidx++) {
										const PPObjTimeSeries::TrendEntry * p_trend_entry = trend_list_set.at(tlsidx);
										const uint input_frame_size = p_trend_entry->Stride;
										if(model_param.MainFrameSizeList.lsearch(static_cast<long>(input_frame_size))) {
											temp_real_list = p_trend_entry->TL;
											temp_real_list.Sort();
											uint trlidx = temp_real_list.getCount();
											if(trlidx) do {
												if(temp_real_list.at(--trlidx) == 0.0)
													temp_real_list.atFree(trlidx);
											} while(trlidx);
											uint part_count = temp_real_list.getCount() / model_param.MainFrameRangeCount;
											for(uint partidx = 0; partidx < model_param.MainFrameRangeCount; partidx++) {
												TsMainFrameRange main_frame_range;
												main_frame_range.MainFrameSize = input_frame_size;
												main_frame_range.R.low = temp_real_list.at(partidx * part_count);
												uint upp_idx = ((partidx == (model_param.MainFrameRangeCount-1)) ? temp_real_list.getCount() : ((partidx + 1) * part_count)) - 1;
												main_frame_range.R.upp = temp_real_list.at(upp_idx);
												main_frame_range_list.insert(&main_frame_range);
											}
										}
									}
								}
								if(main_frame_range_list.getCount() == 0) {
									RealRange fake_main_frame_range;
									fake_main_frame_range.Set(0.0, 0.0);
									main_frame_range_list.insert(&fake_main_frame_range);
								}
							}
						}
						//
						// stake_side: 0 - long, 1 - short
						//
						for(int stake_side = 0; stake_side < 2; stake_side++) {
							if((stake_side == 0 && P.Flags & P.fProcessLong) || (stake_side == 1 && P.Flags & P.fProcessShort)) {
								for(uint mfrlidx = 0; mfrlidx < main_frame_range_list.getCount(); mfrlidx++) {
									const TsMainFrameRange * p_main_frame_range = use_main_frame ? &main_frame_range_list.at(mfrlidx) : 0;
									const PPObjTimeSeries::TrendEntry * p_main_trend_entry = use_main_frame ? PPObjTimeSeries::SearchTrendEntry(trend_list_set, p_main_frame_range->MainFrameSize) : 0;
									const bool is_short = (stake_side == 1);
									const uint32 org_opt_max_duck_val = is_short ? ts_pack.Rec.OptMaxDuck_S : ts_pack.Rec.OptMaxDuck;
									uint  cur_opt_max_duck_val = org_opt_max_duck_val;
									if(!force_fixed_maxduck_values && (P.Flags & P.fFindOptMaxDuck && (org_opt_max_duck_val <= 0 || (P.Flags & P.fForce)))) {
										const uint fomdflags = (is_short ? fomdfShort : 0) | fomdfStoreResult;
										THROW(FindOptimalMaxDuck(ts_pack, ts_tm_list, ts_val_list, fomdflags, &cur_opt_max_duck_val));
									}
									if(P.Flags & P.fFindStrategies) {
										LAssocArray opt_peak_by_maxduck_list; // Кэш соответствий maxduck->opt_peak дабы не пересчитывать снова при каждом значении InputFrameSize
										for(uint mdidx = 0; mdidx < model_param.MaxDuckQuantList.getCount(); mdidx++) {
											PPObjTimeSeries::TrainNnParam org_tnnp(ts_pack, PPObjTimeSeries::TrainNnParam::afAnalyzeFrame);
											symb_buf = ts_pack.GetSymb();
											if(stake_side == 1)
												symb_buf.CatChar('-').Cat("REV");
											org_tnnp.SpikeQuant = spike_quant;
											org_tnnp.EpochCount = 1;
											org_tnnp.InputFrameSize = 0; // input_frame_size;
											org_tnnp.MaxDuckQuant = static_cast<uint16>(model_param.MaxDuckQuantList.get(mdidx));
											org_tnnp.StakeThreshold = 0.05;
											assert(oneof2(P.CloseMode, org_tnnp.clsmodFullMaxDuck, org_tnnp.clsmodAdjustLoss));
											org_tnnp.StakeCloseMode = static_cast<uint16>(P.CloseMode);
											SETFLAG(org_tnnp.BaseFlags, org_tnnp.bfShort, stake_side == 1);
											if(p_main_frame_range) {
												org_tnnp.MainFrameSize = p_main_frame_range->MainFrameSize;
												org_tnnp.OptDelta2Range.low = p_main_frame_range->R.low;
												org_tnnp.OptDelta2Range.upp = p_main_frame_range->R.upp;
											}
											org_tnnp.TargetQuant = target_quant; // @v10.6.10 
#if 0 // @v10.6.10 {
											if(fixed_target_quant) {
												org_tnnp.TargetQuant = target_quant;
											}
											else {
												long   cv = 0;
												uint   local_target_quant = 0;
												if(opt_peak_by_maxduck_list.Search(org_tnnp.MaxDuckQuant, &cv, 0) && cv > 0) {
													local_target_quant = static_cast<uint>(cv);
												}
												else {
													// Рассчитываем средний максимум пиков в квантах (local_result не нужен)
													double local_result = 0.0;
													uint   local_peak_quant = 0;
													THROW(Ts_Helper_FindOptimalFactor(ts_tm_list, ts_val_list, org_tnnp, local_result, local_peak_quant));
													if(local_peak_quant) {
														IntRange peak_range;
														TSVector <PPObjTimeSeries::FactorToResultRelation> opt_peak_set;
														PPObjTimeSeries::FactorToResultRelation opt_peak;
														peak_range.upp = 3 * local_peak_quant / 2;
														const int peak_step = peak_range.upp / 5;
														peak_range.low = peak_step;
														int rr = TsObj.FindOptimalFactor(ts_tm_list, ts_val_list, org_tnnp, 1/*what*/, peak_range, peak_step, 0, opt_peak_set, opt_peak);
														THROW(rr);
														if(rr > 0) {
															local_target_quant = opt_peak.FactorQuant;
															opt_peak_by_maxduck_list.Add(org_tnnp.MaxDuckQuant, local_target_quant);
														}
													}
												}
												if(local_target_quant > 0)
													org_tnnp.TargetQuant = local_target_quant;
											}
#endif // } 0 @v10.6.10
											const uint org_target_quant = org_tnnp.TargetQuant;
											for(uint ifsidx = 0; ifsidx < model_param.InputFrameSizeList.getCount(); ifsidx++) {
												const uint input_frame_size = static_cast<uint>(model_param.InputFrameSizeList.get(ifsidx));
												const PPObjTimeSeries::TrendEntry * p_trend_entry = PPObjTimeSeries::SearchTrendEntry(trend_list_set, input_frame_size);
												assert(p_trend_entry);
												assert(p_trend_entry->Stride == input_frame_size); // @paranoic
												// @v10.6.10 const int process_stake_mode_2 = 0;
												// opt_factor_side:
												// 0 - оптимизировать только в положительной области
												// 1 - оптимизировать только в отрицательной области
												// -1 - оптимизировать по всему множеству значений
												const int opt_factor_side = (model_param.Flags & model_param.fBestSubsetTrendFollowing) ? (is_short ? 1 : 0) : -1;
												PPObjTimeSeries::TrainNnParam tnnp2(org_tnnp);
												tnnp2.InputFrameSize = input_frame_size;
												tnnp2.TrendErrAvg = p_trend_entry->ErrAvg;   // @v10.3.12
												tnnp2.TrendErrLim = model_param.InitTrendErrLimit; // @v10.3.12
												// @v10.4.9 {
												if(p_main_frame_range) {
													tnnp2.MainFrameSize = /*main_frame_size*/p_main_frame_range->MainFrameSize;
													tnnp2.OptDelta2Range.low = p_main_frame_range->R.low;
													tnnp2.OptDelta2Range.upp = p_main_frame_range->R.upp;
												}
												// } @v10.4.9
												/* @v10.6.10 if(fixed_target_quant)*/ { // Фиксированный TargetQuant (since v10.6.10 strictly yes)
													LongArray target_quant_list;
													if(model_param.TargetQuantList.getCount())
														target_quant_list = model_param.TargetQuantList;
													else {
														const uint target_quant_step = 2;
														const uint target_quant_max_probes = 1; // Увеличение TargetQuatn почти ничего не дает - по этому только одно значение
														for(uint current_target_quant = org_target_quant, target_quant_probe_no = 0; target_quant_probe_no < target_quant_max_probes; target_quant_probe_no++)
															target_quant_list.add(static_cast<long>(org_target_quant + target_quant_step * target_quant_probe_no));
													}
													target_quant_list.sortAndUndup();
													for(uint tqidx = 0; tqidx < target_quant_list.getCount(); tqidx++) {
														tnnp2.TargetQuant = static_cast<uint16>(target_quant_list.at(tqidx));
														{
															PPObjTimeSeries::StrategyResultEntry sre(tnnp2, 0);
															int    tstso = tstsofroMode1;
															if(opt_factor_side == 0)
																tstso |= tstsofroPositive;
															else if(opt_factor_side == 1)
																tstso |= tstsofroNegative;
															THROW(TsTestStrategy2(model_param, ts_tm_list, ts_val_list, *p_trend_entry, p_main_trend_entry, tnnp2, tstso, sre, &sr_raw_list));
															for(uint srridx = 0; srridx < sr_raw_list.getCount(); srridx++) {
																PPObjTimeSeries::StrategyResultEntry & r_sr_raw = sr_raw_list.at(srridx);
																PPObjTimeSeries::StrategyResultEntry sre_test(r_sr_raw, 1/*stake_mode*/);
																STRNSCPY(sre_test.Symb, ts_pack.GetSymb());
																THROW(TsTestStrategy2(model_param, ts_tm_list, ts_val_list, *p_trend_entry, p_main_trend_entry, r_sr_raw, tstso, sre_test, 0));
																// @v10.6.11 sre_test.ID = ++last_strategy_id;
																sre_test.ID = scontainer.GetNewStrategyId(); // @v10.6.11 
																{
																	// "Symb" "Dir" "OFS" "Frame" "MainFrame" "Duck" "Target" "RPD" "WinRt" "StkCnt" "AvgTm" "RngLo" "RngUp"
																	msg_buf.Z().Cat(ts_pack.GetSymb()).Semicol();
																		msg_buf.Cat((stake_side == 1) ? "sale" : "buy").Semicol();
																		msg_buf.Cat((opt_factor_side == 0) ? "M1+" : ((opt_factor_side == 1) ? "M1-" : "M1*")).Semicol();
																		msg_buf.Cat(sre_test.InputFrameSize).Semicol();
																		msg_buf.Cat(sre_test.MainFrameSize).Semicol(); // @v10.4.12
																		msg_buf.Cat(sre_test.MaxDuckQuant).Semicol();
																		msg_buf.Cat(sre_test.TargetQuant).Semicol();
																		msg_buf.Cat(sre_test.V.GetResultPerDay(), MKSFMTD(9, 6, 0)).Semicol();
																		msg_buf.Cat(sre_test.GetWinCountRate(), MKSFMTD(15, 5, 0)).Semicol();
																		msg_buf.Cat(sre_test.StakeCount).Semicol();
																		msg_buf.Cat(sre_test.V.TmSec).Semicol();
																		msg_buf.Cat(sre_test.OptDeltaRange.low, MKSFMTD(0, 12, 0)).Semicol();
																		msg_buf.Cat(sre_test.OptDeltaRange.upp, MKSFMTD(0, 12, 0)).Semicol();
																		if(sre_test.MainFrameSize) {
																			msg_buf.Cat(sre_test.OptDelta2Range.low, MKSFMTD(0, 12, 0)).Semicol();
																			msg_buf.Cat(sre_test.OptDelta2Range.upp, MKSFMTD(0, 12, 0)).Semicol();
																		}
																		msg_buf.Cat(sre_test.TrendErrLim, MKSFMTD(0, 12, 0)).Semicol(); // @v10.5.0
																		msg_buf.Cat(sre_test.TrendErrAvg, MKSFMTD(0, 12, 0)).Semicol(); // @v10.5.0
																	f_dump.WriteLine(msg_buf.CR());
																}
																f_out.WriteLine(msg_buf.Z().Cat((opt_factor_side == 0) ? "M1+" : ((opt_factor_side == 1) ? "M1-" : "M1*")).Space().
																	Cat(PPObjTimeSeries::StrategyOutput(&sre_test, temp_buf)).CR());
																if(sre_test.StakeCount > 0) {
																	THROW_SL(scontainer.insert(dynamic_cast <PPObjTimeSeries::Strategy *>(&sre_test)));
																}
																f_out.Flush();
																// @v10.6.10 {
																if(model_param.MinWinRate > 0.0 && sre_test.GetWinCountRate() <= model_param.MinWinRate) {
																	//
																	// Нет смысла тестировать последующие стратегии в этом наборе поскольку уровень нажедности 
																	// для каждого следующего кандидата снижается.
																	//
																	break;
																}
																// } @v10.6.10 
															}
														}
#if 0 // @v10.6.10 {
														if(process_stake_mode_2) {
															PPObjTimeSeries::StrategyResultEntry sre(tnnp2, 0);
															int    tstso = tstsofroMode2;
															if(opt_factor_side == 0)
																tstso |= tstsofroPositive;
															else if(opt_factor_side == 1)
																tstso |= tstsofroNegative;
															THROW(TsTestStrategy2(model_param, ts_tm_list, ts_val_list, *p_trend_entry, p_main_trend_entry, tnnp2, tstso, sre, &sr_raw_list));
															for(uint srridx = 0; srridx < sr_raw_list.getCount(); srridx++) {
																PPObjTimeSeries::StrategyResultEntry & r_sr_raw = sr_raw_list.at(srridx);
																PPObjTimeSeries::StrategyResultEntry sre_test(r_sr_raw, 2/*stake_mode*/);
																STRNSCPY(sre_test.Symb, ts_pack.GetSymb());
																THROW(TsTestStrategy2(model_param, ts_tm_list, ts_val_list, *p_trend_entry, p_main_trend_entry, r_sr_raw, tstso, sre_test, 0));
																sre_test.ID = ++last_strategy_id;
																{
																	// "Symb" "Dir" "OFS" "Frame" "Duck" "Target" "RPD" "WinRt" "StkCnt" "AvgTm" "RngLo" "RngUp"
																	msg_buf.Z().Cat(ts_pack.GetSymb()).Semicol();
																		msg_buf.Cat((stake_side == 1) ? "sale" : "buy").Semicol();
																		msg_buf.Cat((opt_factor_side == 0) ? "M2+" : ((opt_factor_side == 1) ? "M2-" : "M2*")).Semicol();
																		msg_buf.Cat(sre_test.InputFrameSize).Semicol();
																		msg_buf.Cat(sre_test.MaxDuckQuant).Semicol();
																		msg_buf.Cat(sre_test.TargetQuant).Semicol();
																		msg_buf.Cat(sre_test.V.GetResultPerDay(), MKSFMTD(9, 6, 0)).Semicol();
																		msg_buf.Cat(sre_test.GetWinCountRate(), MKSFMTD(15, 5, 0)).Semicol();
																		msg_buf.Cat(sre_test.StakeCount).Semicol();
																		msg_buf.Cat(sre_test.V.TmSec).Semicol();
																		msg_buf.Cat(sre_test.OptDelta2Stride).Semicol();
																		msg_buf.Cat(sre_test.OptDelta2Range.low, MKSFMTD(0, 12, 0)).Semicol();
																		msg_buf.Cat(sre_test.OptDelta2Range.upp, MKSFMTD(0, 12, 0));
																	f_dump.WriteLine(msg_buf.CR());
																}
																f_out.WriteLine(msg_buf.Z().Cat((opt_factor_side == 0) ? "M2+" : ((opt_factor_side == 1) ? "M2-" : "M2*")).Space().
																	Cat(PPObjTimeSeries::StrategyOutput(&sre_test, temp_buf)).CR());
																if(sre_test.StakeCount > 0) {
																	THROW_SL(scontainer.insert(dynamic_cast <PPObjTimeSeries::Strategy *>(&sre_test)));
																}
																f_out.Flush();
															}
														}
#endif // } 0 @v10.6.10
													}
												}
#if 0 // @v10.6.10 {
												else {
													uint prev_target = tnnp2.TargetQuant;
													for(int do_repeat_on_target_factor = 1, tfiter_no = 0; do_repeat_on_target_factor; tfiter_no++) {
														do_repeat_on_target_factor = 0;
														tnnp2.TargetQuant = prev_target;
														PPObjTimeSeries::StrategyResultEntry sre(tnnp2, 0);
														int    tstso = tstsofroMode1;
														if(opt_factor_side == 0)
															tstso |= tstsofroPositive;
														else if(opt_factor_side == 1)
															tstso |= tstsofroNegative;
														THROW(TsTestStrategy2(model_param, ts_tm_list, ts_val_list, *p_trend_entry, p_main_trend_entry, tnnp2, tstso, sre, &sr_raw_list));
														for(uint srridx = 0; srridx < sr_raw_list.getCount(); srridx++) {
															PPObjTimeSeries::StrategyResultEntry & r_sr_raw = sr_raw_list.at(srridx);
															PPObjTimeSeries::StrategyResultEntry sre_test(r_sr_raw, 1/*stake_mode*/);
															STRNSCPY(sre_test.Symb, ts_pack.GetSymb());
															/*
															f_out.WriteLine(msg_buf.Z().Cat((opt_factor_side == 0) ? "I0+" : ((opt_factor_side == 1) ? "I0-" : "I0*")).Space().
																Cat(PPObjTimeSeries::StrategyOutput(&sre_test, temp_buf)).CR());
															f_out.Flush();
															*/
															THROW(TsTestStrategy2(model_param, ts_tm_list, ts_val_list, *p_trend_entry, p_main_trend_entry, r_sr_raw, tstso, sre_test, 0));
															sre_test.ID = ++last_strategy_id;
															f_out.WriteLine(msg_buf.Z().Cat((opt_factor_side == 0) ? "M1+" : ((opt_factor_side == 1) ? "M1-" : "M1*")).Space().
																Cat(PPObjTimeSeries::StrategyOutput(&sre_test, temp_buf)).CR());
															if(sre_test.StakeCount > 0 && sre_test.WinCount > 0) {
																const uint tstp_lo_limit = 18; // @20190416 12-->18
																// Количество квантов вниз от текущего оптимума {
																// @20190417 uint tstp_left_shift  = 2 * sre_test.TargetQuant / 3;
																uint tstp_left_shift  = 3 * sre_test.TargetQuant / 4; // @20190417
																// }
																uint tstp_right_shift = 0; // Количество квантов вверх от текущего оптимума
																if(sre_test.TargetQuant <= tstp_lo_limit)
																	tstp_left_shift = 0;
																else if((sre_test.TargetQuant - tstp_left_shift) < tstp_lo_limit) {
																	tstp_left_shift = sre_test.TargetQuant - tstp_lo_limit;
																}
																/* Похоже, поднимать Target не стоит (ожидаемый профит может вырасти, но вероятность выигрыша падает).
																if(tfiter_no > 0) {
																	if(sre_test.TargetQuant < (org_target_quant-1))
																		tstp_right_shift = MIN(sre_test.TargetQuant / 4, (org_target_quant - 1 - sre_test.TargetQuant));
																}*/
																tstp_right_shift = sre_test.TargetQuant / 4; // @20190417
																THROW(TuneStrategyTP(model_param, ts_tm_list, ts_val_list, trend_list_set, tstp_left_shift, tstp_right_shift, sre_test, 0/*&f_out*/));
																f_out.WriteLine(msg_buf.Z().Cat("OPT").Space().Cat(PPObjTimeSeries::StrategyOutput(&sre_test, temp_buf)).CR());
																if(sre_test.StakeCount > 0) {
																	if(sre_test.TargetQuant == prev_target) {
																		THROW_SL(scontainer.insert(dynamic_cast <PPObjTimeSeries::Strategy *>(&sre_test)));
																	}
																	else {
																		prev_target = sre_test.TargetQuant;
																		do_repeat_on_target_factor = 1;
																	}
																}
															}
															f_out.Flush();
														}
													}
												}
#endif // } 0 @v10.6.10
											}
										}
									}
								}
							}
						}
						if(P.Flags & P.fFindStrategies) {
							double best_sc_result = 0.0;
							uint   best_sc_idx = 0;
							double prev_result = 0.0;
							PPObjTimeSeries::StrategyContainer sc_selection;
							PPObjTimeSeries::StrategyContainer sc_process;
							PPObjTimeSeries::StrategyContainer sc_skip_due_dup; // @v10.6.9 стратегии, вынесенные из рассмотрения, поскольку для них есть более удачливые дубликаты
							long   ssflags = PPObjTimeSeries::StrategyContainer::gbsfLong|PPObjTimeSeries::StrategyContainer::gbsfShort|
								PPObjTimeSeries::StrategyContainer::gbsfStakeMode3|
								PPObjTimeSeries::StrategyContainer::gbsfStakeMode2|
								PPObjTimeSeries::StrategyContainer::gbsfStakeMode1;
							// ssflags |= scontainer.gbsfCritProfitMultProb; // @20190413
							// ssflags |= scontainer.gbsfCritProb; // @20190419
							ssflags |= scontainer.gbsfEliminateDups; // @20190416
							// @20190425 {
							if(model_param.Flags & model_param.fBestSubsetTrendFollowing)
								ssflags |= scontainer.gbsfTrendFollowing;
							// } @20190425
							ssflags |= scontainer.gbsfCritProb; // @20190514
							if(scontainer.GetBestSubset(ssflags, model_param.BestSubsetDimention, model_param.MinWinRate, sc_process, &sc_skip_due_dup) > 0) {
								{
									f_out.WriteLine(msg_buf.Z().CR());
									f_out.WriteLine((msg_buf = "--- TsSimulateStrategyContainer").Space().Cat(ts_pack.GetSymb()).CR());
									msg_buf.Z().Cat("Full Subset").CatDiv(':', 2).Cat(sc_process.getCount());
									f_out.WriteLine(msg_buf.CR());
									for(uint si = 0; si < sc_process.getCount(); si++) {
										PPObjTimeSeries::StrategyToString(sc_process.at(si), 0, msg_buf);
										f_out.WriteLine(msg_buf.CR());
									}
								}
								// @v10.6.9 {
								{
									f_out.WriteLine(msg_buf.Z().CR());
									f_out.WriteLine((msg_buf = "--- TsSkippedDueDupStrategyContainer").Space().Cat(ts_pack.GetSymb()).CR());
									msg_buf.Z().Cat("Full Subset").CatDiv(':', 2).Cat(sc_skip_due_dup.getCount());
									f_out.WriteLine(msg_buf.CR());
									for(uint si = 0; si < sc_skip_due_dup.getCount(); si++) {
										PPObjTimeSeries::StrategyToString(sc_skip_due_dup.at(si), 0, msg_buf);
										f_out.WriteLine(msg_buf.CR());
									}
								}
								sc_skip_due_dup.freeAll();
								// } @v10.6.9 
								if(oneof3(model_param.BestSubsetOptChunk, 3, 7, 15)) { // @v10.6.9 (15)
									//
									// Многопоточный подбор оптимальной комбинации стратегий
									//
									//const uint best_subset_dimension = 100; // Количество стратегий извлекаемых из общего множества, полученного выше,
										// для дальнейшего выбора оптимальной комбинации. Эти best_subset_dimension отбираются по тривиальному
										// критерию - максимальный выигрыш в сутки (Strategy::V::GetResultPerDay()).
										// Увеличение параметра приведет к замедлению расчетов, уменьшение - к вероятной потере выигрышных стратегий.
										// @v10.3.10 50-->60
										// @v10.3.11 50-->80
										// @v10.3.12 80-->100
									//const uint max_phony_iters = 7; // Максимальное количество новых наборов стратегий, не дающих улучшения, после которого надо заканчивать с подбором
										// @v10.3.12 5-->7
									//const uint opt_chunk = 3;       // Количество стратегий в одном наборе, над которым осуществляется полный перебор вариантов.
										// Возможно только 2 значения: 3 и 7 (большие величины приведут к очень сильной задержке в вычислениях - комбинаторный взрыв).
										// @v10.3.12 3-->7
									assert(oneof3(model_param.BestSubsetOptChunk, 3, 7, 15)); // @v10.6.9 (15)
									class StrategySetSimulationTask : public SlThread {
									public:
										struct InitBlock {
											InitBlock(const DateTimeArray & rTmList, const RealArray & rValList,
												const TSCollection <PPObjTimeSeries::TrendEntry> & rTrendList,
												const PPObjTimeSeries::StrategyContainer & rSc,
												PPObjTimeSeries::StrategyResultEntry * pResult) :
												R_TmList(rTmList), R_ValList(rValList), R_TrendList(rTrendList), R_Sc(rSc), P_Result(pResult)
											{
											}
											const DateTimeArray & R_TmList;
											const RealArray & R_ValList;
											const TSCollection <PPObjTimeSeries::TrendEntry> & R_TrendList;
											const PPObjTimeSeries::StrategyContainer & R_Sc;
											PPObjTimeSeries::StrategyResultEntry * P_Result;
										};
										StrategySetSimulationTask(InitBlock * pBlk) : SlThread(0), B(*pBlk)
										{
											InitStartupSignal();
										}
										virtual void Run()
										{
											assert(SLS.GetConstTLA().Id == GetThreadID());
											TsSimulateStrategyContainer(B.R_TmList, B.R_ValList, B.R_TrendList, B.R_Sc, 0, *B.P_Result, 0);
										}
									private:
										virtual void SLAPI Startup()
										{
											SlThread::Startup();
											SignalStartup();
										}
										InitBlock B;
									};
									double best_result = 0.0;
									uint   phony_iter_no = 0;
									const  uint scp_inc = (model_param.BestSubsetOptChunk == 3) ? 2 : ((model_param.BestSubsetOptChunk == 7) ? 3 : ((model_param.BestSubsetOptChunk == 15) ? 4 : 0));
									for(uint scpidx = 0; scpidx < sc_process.getCount() && phony_iter_no < model_param.BestSubsetMaxPhonyIters; scpidx += scp_inc) {
										PPObjTimeSeries::StrategyResultEntry sre[15]; // @v10.6.9 [7]-->[15]
										PPObjTimeSeries::StrategyContainer sc[15]; // 7 - max of model_param.BestSubsetOptChunk // @v10.6.9 [7]-->[15]
										// @v10.6.9 @construction [15]
										HANDLE objs_to_wait[32]; // @v10.6.9 [16]-->[32]
										size_t objs_to_wait_count = 0;
										MEMSZERO(objs_to_wait);
										uint thr_idx;
										// @v10.6.9 @construction {
										if(model_param.BestSubsetOptChunk == 15) {
											assert(0); // @construction
											for(thr_idx = 0; thr_idx < model_param.BestSubsetOptChunk; thr_idx++) {
												sc[thr_idx] = sc_selection;
												// 0001, 0010, 0011, 0100, 0101, 0110, 0111, 1000, 1001, 1010, 1011, 1100, 1101, 1110, 1111
												if(thr_idx == 0) {
													sc[thr_idx].insert(&sc_process.at(scpidx)); // 0001
												}
												else if(scpidx < (sc_process.getCount()-1)) {
													if(thr_idx == 1) { // 0010
														sc[thr_idx].insert(&sc_process.at(scpidx+1));
													}
													else if(thr_idx == 2) { // 0011
														sc[thr_idx].insert(&sc_process.at(scpidx));
														sc[thr_idx].insert(&sc_process.at(scpidx+1));
													}
													else if(scpidx < (sc_process.getCount()-2)) {
														if(thr_idx == 3) { // 0100
															sc[thr_idx].insert(&sc_process.at(scpidx+2));
														}
														else if(thr_idx == 4) { // 0101
															sc[thr_idx].insert(&sc_process.at(scpidx));
															sc[thr_idx].insert(&sc_process.at(scpidx+2));
														}
														else if(thr_idx == 5) { // 0110
															sc[thr_idx].insert(&sc_process.at(scpidx+1));
															sc[thr_idx].insert(&sc_process.at(scpidx+2));
														}
														else if(thr_idx == 6) { // 0111
															sc[thr_idx].insert(&sc_process.at(scpidx));
															sc[thr_idx].insert(&sc_process.at(scpidx+1));
															sc[thr_idx].insert(&sc_process.at(scpidx+2));
														}
														else if(scpidx < (sc_process.getCount()-3)) {
															if(thr_idx == 7) { // 1000
																sc[thr_idx].insert(&sc_process.at(scpidx+3));
															}
															else if(thr_idx == 8) { // 1001
																sc[thr_idx].insert(&sc_process.at(scpidx));
																sc[thr_idx].insert(&sc_process.at(scpidx+3));
															}
															else if(thr_idx == 9) { // 1010
																sc[thr_idx].insert(&sc_process.at(scpidx+1));
																sc[thr_idx].insert(&sc_process.at(scpidx+3));
															}
															else if(thr_idx == 10) { // 1011
																sc[thr_idx].insert(&sc_process.at(scpidx));
																sc[thr_idx].insert(&sc_process.at(scpidx+1));
																sc[thr_idx].insert(&sc_process.at(scpidx+3));
															}

															else if(thr_idx == 11) { // 1100
																sc[thr_idx].insert(&sc_process.at(scpidx+2));
																sc[thr_idx].insert(&sc_process.at(scpidx+3));
															}
															else if(thr_idx == 12) { // 1101
																sc[thr_idx].insert(&sc_process.at(scpidx));
																sc[thr_idx].insert(&sc_process.at(scpidx+2));
																sc[thr_idx].insert(&sc_process.at(scpidx+3));
															}
															else if(thr_idx == 13) { // 1110
																sc[thr_idx].insert(&sc_process.at(scpidx+1));
																sc[thr_idx].insert(&sc_process.at(scpidx+2));
																sc[thr_idx].insert(&sc_process.at(scpidx+3));
															}
															else if(thr_idx == 14) { // 1111
																sc[thr_idx].insert(&sc_process.at(scpidx));
																sc[thr_idx].insert(&sc_process.at(scpidx+1));
																sc[thr_idx].insert(&sc_process.at(scpidx+2));
																sc[thr_idx].insert(&sc_process.at(scpidx+3));
															}
														}
													}
												}
												StrategySetSimulationTask::InitBlock tb(ts_tm_list, ts_val_list, trend_list_set, sc[thr_idx], &sre[thr_idx]);
												StrategySetSimulationTask * p_thread = new StrategySetSimulationTask(&tb);
												THROW_S(p_thread, SLERR_NOMEM);
												p_thread->Start(1/*0*/);
												objs_to_wait[objs_to_wait_count++] = *p_thread;
											}
										}
										// } @v10.6.9 @construction 
										else if(model_param.BestSubsetOptChunk == 7) {
											for(thr_idx = 0; thr_idx < model_param.BestSubsetOptChunk; thr_idx++) {
												sc[thr_idx] = sc_selection;
												// 001, 010, 011, 100, 101, 110, 111
												if(thr_idx == 0) {
													sc[thr_idx].insert(&sc_process.at(scpidx));
												}
												else if(scpidx < (sc_process.getCount()-1)) {
													if(thr_idx == 1) {
														sc[thr_idx].insert(&sc_process.at(scpidx+1));
													}
													else if(thr_idx == 2) {
														sc[thr_idx].insert(&sc_process.at(scpidx));
														sc[thr_idx].insert(&sc_process.at(scpidx+1));
													}
													else if(scpidx < (sc_process.getCount()-2)) {
														if(thr_idx == 3) {
															sc[thr_idx].insert(&sc_process.at(scpidx+2));
														}
														else if(thr_idx == 4) {
															sc[thr_idx].insert(&sc_process.at(scpidx));
															sc[thr_idx].insert(&sc_process.at(scpidx+2));
														}
														else if(thr_idx == 5) {
															sc[thr_idx].insert(&sc_process.at(scpidx+1));
															sc[thr_idx].insert(&sc_process.at(scpidx+2));
														}
														else if(thr_idx == 6) {
															sc[thr_idx].insert(&sc_process.at(scpidx));
															sc[thr_idx].insert(&sc_process.at(scpidx+1));
															sc[thr_idx].insert(&sc_process.at(scpidx+2));
														}
													}
												}
												StrategySetSimulationTask::InitBlock tb(ts_tm_list, ts_val_list, trend_list_set, sc[thr_idx], &sre[thr_idx]);
												StrategySetSimulationTask * p_thread = new StrategySetSimulationTask(&tb);
												THROW_S(p_thread, SLERR_NOMEM);
												p_thread->Start(1/*0*/);
												objs_to_wait[objs_to_wait_count++] = *p_thread;
											}
										}
										else if(model_param.BestSubsetOptChunk == 3) {
											for(thr_idx = 0; thr_idx < model_param.BestSubsetOptChunk; thr_idx++) {
												sc[thr_idx] = sc_selection;
												// 01, 10, 11
												if(thr_idx == 0) {
													sc[thr_idx].insert(&sc_process.at(scpidx));
												}
												else if(scpidx < (sc_process.getCount()-1)) {
													if(thr_idx == 1) {
														sc[thr_idx].insert(&sc_process.at(scpidx+1));
													}
													else if(thr_idx == 2) {
														sc[thr_idx].insert(&sc_process.at(scpidx));
														sc[thr_idx].insert(&sc_process.at(scpidx+1));
													}
												}
												StrategySetSimulationTask::InitBlock tb(ts_tm_list, ts_val_list, trend_list_set, sc[thr_idx], &sre[thr_idx]);
												StrategySetSimulationTask * p_thread = new StrategySetSimulationTask(&tb);
												THROW_S(p_thread, SLERR_NOMEM);
												p_thread->Start(1/*0*/);
												objs_to_wait[objs_to_wait_count++] = *p_thread;
											}
										}
										::WaitForMultipleObjects(objs_to_wait_count, objs_to_wait, TRUE, INFINITE);
										int best_thr_idx = -1;
										double local_best_result = best_result;
										for(thr_idx = 0; thr_idx < model_param.BestSubsetOptChunk; thr_idx++) {
											if(local_best_result < sre[thr_idx].V.Result) {
												local_best_result = sre[thr_idx].V.Result;
												best_thr_idx = static_cast<int>(thr_idx);
											}
										}
										if(best_thr_idx >= 0) {
											phony_iter_no = 0;
											sc_selection = sc[best_thr_idx];
											best_result = local_best_result;
											msg_buf.Z().Cat("Local Good Subset").CatDiv(':', 2).Cat("BC-LS").CatChar('-').Cat(scpidx+1);
											f_out.WriteLine(msg_buf.CR());
											for(uint si = 0; si < sc_selection.getCount(); si++) {
												PPObjTimeSeries::StrategyToString(sc_selection.at(si), 0, msg_buf.Z());
												f_out.WriteLine(msg_buf.CR());
											}
											OutputStategyResultEntry(sre[best_thr_idx], msg_buf);
											f_out.WriteLine(msg_buf.CR());
											f_out.Flush();
										}
										else
											phony_iter_no++;
									}
								}
								else
									sc_selection = sc_process;
							}
							if(sc_selection.getCount()) {
								//
								// Контрольный тест для отборного контейнера sc_selection
								//
								sc_selection.SetLastValTm(last_val_dtm);
								msg_buf.Z().Cat("Selected-Subset").CatDiv(':', 2).Cat("BC-LS").CatChar('-').Cat(sc_selection.getCount());
								f_out.WriteLine(msg_buf.CR());
								for(uint si = 0; si < sc_selection.getCount(); si++) {
									PPObjTimeSeries::StrategyToString(sc_selection.at(si), 0, msg_buf.Z());
									f_out.WriteLine(msg_buf.CR());
								}
								PPObjTimeSeries::StrategyResultEntry sre;
								TsSimulateStrategyContainer(ts_tm_list, ts_val_list, trend_list_set, sc_selection, 0, sre, 0);
								THROW(TsObj.PutStrategies(id, PPObjTimeSeries::sstSelection, &sc_selection, 1)); // Сохранение отобранных стратегий в базе данных
								OutputStategyResultEntry(sre, msg_buf);
								f_out.WriteLine(msg_buf.CR());
								f_out.Flush();
								if(f_out_total.IsValid()) {
									struct MainFrameCountEntry {
										RealRange R;
										long   Count;
									};
									LAssocArray ifs_count_list;
									LAssocArray md_count_list;
									LAssocArray target_count_list;
									SVector main_frame_count_list(sizeof(MainFrameCountEntry));
									for(uint scidx = 0; scidx < sc_selection.getCount(); scidx++) {
										const PPObjTimeSeries::Strategy & r_sc = sc_selection.at(scidx);
										{
											const  long key = static_cast<long>(r_sc.InputFrameSize);
											uint   pos = 0;
											long   c = 0;
											if(ifs_count_list.Search(key, &c, &pos))
												ifs_count_list.at(pos).Val = (c+1);
											else
												ifs_count_list.Add(key, 1);
										}
										{
											const  long key = static_cast<long>(r_sc.MaxDuckQuant);
											uint   pos = 0;
											long   c = 0;
											if(md_count_list.Search(key, &c, &pos))
												md_count_list.at(pos).Val = (c+1);
											else
												md_count_list.Add(key, 1);
										}
										{
											const  long key = static_cast<long>(r_sc.TargetQuant);
											uint   pos = 0;
											long   c = 0;
											if(target_count_list.Search(key, &c, &pos))
												target_count_list.at(pos).Val = (c+1);
											else
												target_count_list.Add(key, 1);
										}
										// @v10.6.8 {
										if(r_sc.MainFrameSize) {
											int   mf_found = 0;
											for(uint i = 0; i < main_frame_count_list.getCount(); i++) {
												if(static_cast<MainFrameCountEntry *>(main_frame_count_list.at(i))->R.IsEqual(r_sc.OptDelta2Range)) {
													static_cast<MainFrameCountEntry *>(main_frame_count_list.at(i))->Count++;
													mf_found = 1;
												}
											}
											if(!mf_found) {
												MainFrameCountEntry new_entry;
												new_entry.R = r_sc.OptDelta2Range;
												new_entry.Count = 1;
												main_frame_count_list.insert(&new_entry);
											}
										}
										// } @v10.6.8 
									}
									msg_buf.Z().Cat(getcurdatetime_(), DATF_YMD|DATF_CENTURY, TIMF_HMS).Space().Cat(ts_pack.GetSymb());
									f_out_total.WriteLine(msg_buf.CR());
									for(uint si = 0; si < sc_selection.getCount(); si++) {
										PPObjTimeSeries::StrategyToString(sc_selection.at(si), 0, msg_buf.Z());
										f_out_total.WriteLine(msg_buf.CR());
									}
									OutputStategyResultEntry(sre, msg_buf);
									f_out_total.WriteLine(msg_buf.CR());
									{
										msg_buf.Z().Cat("InputFrameSize").CatDiv(':', 2);
										ifs_count_list.Sort();
										for(uint clidx = 0; clidx < ifs_count_list.getCount(); clidx++) {
											const LAssoc & r_assoc = ifs_count_list.at(clidx);
											if(clidx)
												msg_buf.Space();
											msg_buf.Cat(r_assoc.Key).CatChar('/').Cat(r_assoc.Val);
										}
										f_out_total.WriteLine(msg_buf.CR());
									}
									{
										msg_buf.Z().Cat("MaxDuckQuant").CatDiv(':', 2);
										md_count_list.Sort();
										for(uint clidx = 0; clidx < md_count_list.getCount(); clidx++) {
											const LAssoc & r_assoc = md_count_list.at(clidx);
											if(clidx)
												msg_buf.Space();
											msg_buf.Cat(r_assoc.Key).CatChar('/').Cat(r_assoc.Val);
										}
										f_out_total.WriteLine(msg_buf.CR());
									}
									{
										msg_buf.Z().Cat("TargetQuant").CatDiv(':', 2);
										target_count_list.Sort();
										for(uint clidx = 0; clidx < target_count_list.getCount(); clidx++) {
											const LAssoc & r_assoc = target_count_list.at(clidx);
											if(clidx)
												msg_buf.Space();
											msg_buf.Cat(r_assoc.Key).CatChar('/').Cat(r_assoc.Val);
										}
										f_out_total.WriteLine(msg_buf.CR());
									}
									// @v10.6.8 {
									{
										const long trange_fmt = MKSFMTD(0, 10, NMBF_NOTRAILZ);
										msg_buf.Z().Cat("MainFrameRange").CatDiv(':', 2);
										for(uint clidx = 0; clidx < main_frame_count_list.getCount(); clidx++) {
											const MainFrameCountEntry * p_entry = static_cast<const MainFrameCountEntry *>(main_frame_count_list.at(clidx));
											if(clidx)
												msg_buf.Space();
											msg_buf.CatChar('[').Cat(p_entry->R, trange_fmt).CatChar(']').CatChar('/').Cat(p_entry->Count);
										}
										f_out_total.WriteLine(msg_buf.CR());
									}
									// } @v10.6.8 
									f_out_total.Flush();
								}
							}
						}
					}
				}
			}
		}
	}
	CATCH
		PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_TIME|LOGMSGF_DBINFO);
		ok = 0;
	ENDCATCH
	PPWait(0);
	return ok;
}

int SLAPI PrcssrTsStrategyAnalyze::AnalyzeRegression(PPID tsID, const LongArray & rStepList)
{
	int    ok = -1;
	SString temp_buf;
	SString msg_buf;
	PPTimeSeries ts_rec;
	PPWait(1);
	if(TsObj.Search(tsID, &ts_rec) > 0) {
		STimeSeries ts;
		STimeSeries ts_last_chunk;
		TSCollection <PPObjTimeSeries::TrendEntry> trend_list_set;
		int gtsr = TsObj.GetTimeSeries(tsID, ts);
		if(gtsr > 0) {
			const uint tsc = ts.GetCount();
			SString out_file_name;
			(temp_buf = "tsregression").CatChar('-').Cat(ts_rec.Symb);
			PPGetFilePath(PPPATH_OUT, temp_buf, out_file_name);
			SFile f_out(out_file_name, SFile::mWrite);	
			THROW_SL(f_out.IsValid());
			{
				uint   vec_idx = 0;
				RealArray cov00_list;
				RealArray cov01_list;
				RealArray cov11_list;
				THROW_SL(ts.GetValueVecIndex("close", &vec_idx));
				for(uint ifsidx = 0; ifsidx < rStepList.getCount(); ifsidx++) {
					const uint input_frame_size = static_cast<uint>(rStepList.get(ifsidx));
					PPObjTimeSeries::TrendEntry trend_entry(input_frame_size, tsc);
					{
						StatBase cov00_stat(0);
						StatBase cov00sqr_stat(0);
						STimeSeries::AnalyzeFitParam afp(input_frame_size, 0, 0);
						THROW(ts.AnalyzeFit("close", afp, &trend_entry.TL, 0, &cov00_list, &cov01_list, &cov11_list));
						assert(trend_entry.TL.getCount() == tsc);
						assert(cov00_list.getCount() == tsc);
						assert(cov01_list.getCount() == tsc);
						assert(cov11_list.getCount() == tsc);

						for(uint trlidx = 0; trlidx < tsc; trlidx++) {
							const double cov00 = cov00_list.at(trlidx);
							const double cov00sqr = sqrt(cov00);
							assert(cov00 >= 0.0);
							if(cov00 > 0.0) { // strictly 'greater than'
								cov00_stat.Step(cov00);
								cov00sqr_stat.Step(cov00sqr);
							}
						}
						cov00_stat.Finish();
						cov00sqr_stat.Finish();
						const double cov00_avg = cov00_stat.GetExp();
						const double cov00sqr_avg = cov00sqr_stat.GetExp();

						for(uint trlidx = 0; trlidx < tsc; trlidx++) {
							const double cov00 = cov00_list.at(trlidx);
							const double cov00sqr = sqrt(cov00);
							assert(cov00 >= 0.0);
							double ts_value = 0.0;
							ts.GetValue(trlidx, vec_idx, &ts_value);
							msg_buf.Z().Cat(ts_value, MKSFMTD(0, 5, 0));
							msg_buf.Space().Cat(cov00 * 10e9, MKSFMTD(0, 3, 0));
							if(cov00_avg > 0.0)
								msg_buf.Space().Cat(cov00 / cov00_avg, MKSFMTD(0, 3, 0));
							else
								msg_buf.Space().Cat("---");
							msg_buf.Space().Cat(cov00sqr * 10e9, MKSFMTD(0, 3, 0));
							if(cov00sqr_avg > 0.0)
								msg_buf.Space().Cat(cov00sqr / cov00sqr_avg, MKSFMTD(0, 3, 0));
							else
								msg_buf.Space().Cat("---");
							msg_buf.Space().Cat(cov01_list.at(trlidx) * 10e9, MKSFMTD(0, 3, 0));
							msg_buf.Space().Cat(cov11_list.at(trlidx) * 10e9, MKSFMTD(0, 3, 0));
							f_out.WriteLine(msg_buf.CR());
						}
						msg_buf.Z().CatEq("count", tsc).Space().CatEq("frame", input_frame_size).Space().
							CatEq("cov00-avg", cov00_avg * 10e9, MKSFMTD(0, 3, 0)).Space().
							CatEq("cov00sqr-avg", cov00sqr_avg * 10e9, MKSFMTD(0, 3, 0));
						f_out.WriteLine(msg_buf.CR());
					}
				}
			}
		}
	}
	CATCHZOK
	PPWait(0);
	return ok;
}

int SLAPI PrcssrTsStrategyAnalyze::TryStrategyContainer(PPID tsID)
{
	int    ok = -1;
	SString temp_buf;
	SString msg_buf;
	PPTimeSeries ts_rec;
	ModelParam model_param;
	PPWait(1);
	if(TsObj.Search(tsID, &ts_rec) > 0) {
		STimeSeries ts;
		STimeSeries ts_last_chunk;
		TSCollection <PPObjTimeSeries::TrendEntry> trend_list_set;
		THROW(ReadModelParam(model_param));
		const int gtsr = GetTimeSeries(tsID, model_param, ts); // @v10.4.11
		if(gtsr > 0) {
			const uint tsc = ts.GetCount();
			PPObjTimeSeries::StrategyContainer sc;
			TsObj.GetStrategies(tsID, PPObjTimeSeries::sstSelection, sc);
			{
				PROFILE_START
				SString out_file_name;
				(temp_buf = "tsscsim").CatChar('-').Cat(ts_rec.Symb);
				PPGetFilePath(PPPATH_OUT, temp_buf, out_file_name);
				SFile f_out(out_file_name, SFile::mWrite);
				uint  max_opt_delta2_stride = 0;
				DateTimeArray ts_tm_list;
				RealArray ts_val_list;
				LongArray ifs_list;
				RealArray temp_real_list;
				uint   vec_idx = 0;
				THROW_SL(ts.GetValueVecIndex("close", &vec_idx));
				if(tsc > 500 && sc.GetInputFramSizeList(ifs_list, &max_opt_delta2_stride) > 0) {
					const uint trend_nominal_count = max_opt_delta2_stride + 3;
					ifs_list.sortAndUndup(); // @paranoic
					if(tsc > 10000) {
						ts.GetChunkRecentCount(10000, ts_last_chunk);
					}
					const uint tsc_test_chunk = ts_last_chunk.GetCount();
					// @v10.6.11 {
					{
						THROW(MakeArVectors(ts, ifs_list, mavfDontSqrtErrList, trend_list_set));
						if(tsc_test_chunk) {
							for(uint tlsidx = 0; tlsidx < trend_list_set.getCount(); tlsidx++) {
								PPObjTimeSeries::TrendEntry * p_trend_entry = trend_list_set.at(tlsidx);
								assert(p_trend_entry);
								if(p_trend_entry) {
									const uint input_frame_size = p_trend_entry->Stride;
									//
									// Тестовый блок призванный проверить эквивалентность расчета трендов для построения стратегии и
									// при использовании рядов для реального трейдинга.
									//
									STimeSeries::AnalyzeFitParam afp2(input_frame_size, tsc_test_chunk-trend_nominal_count, trend_nominal_count);
									RealArray test_trend_list;
									RealArray test_err_list;
									THROW(ts_last_chunk.AnalyzeFit("close", afp2, &test_trend_list, 0, &test_err_list, 0, 0));
									uint tcidx = test_trend_list.getCount();
									uint wcidx = p_trend_entry->TL.getCount();
									assert(wcidx >= tcidx);
									if(tcidx) do {
										const double tv = test_trend_list.at(--tcidx);
										const double wv = p_trend_entry->TL.at(--wcidx);
										assert(tv == wv);
									} while(tcidx);
									//
									uint teidx = test_err_list.getCount();
									uint weidx = p_trend_entry->ErrL.getCount();
									assert(weidx >= teidx);
									if(teidx) do {
										const double tv = test_err_list.at(--teidx);
										const double wv = p_trend_entry->ErrL.at(--weidx);
										assert(tv == wv);
									} while(teidx);
									p_trend_entry->SqrtErrList(0); // see flag mavfDontSqrtErrList in MakeArVectors
								}
							}
						}
					}
					// } @v10.6.11 
#if 0 // @v10.6.11 {
					for(uint ifsidx = 0; ifsidx < ifs_list.getCount(); ifsidx++) {
						const uint input_frame_size = static_cast<uint>(ifs_list.get(ifsidx));
						assert(PPObjTimeSeries::SearchTrendEntry(trend_list_set, input_frame_size) == 0);
						PPObjTimeSeries::TrendEntry * p_new_trend_entry = new PPObjTimeSeries::TrendEntry(input_frame_size, tsc);
						THROW_SL(p_new_trend_entry);
						{
							StatBase trls(0);
							STimeSeries::AnalyzeFitParam afp(input_frame_size, 0, 0);
							THROW(ts.AnalyzeFit("close", afp, &p_new_trend_entry->TL, 0, &temp_real_list, 0, 0));
							assert(p_new_trend_entry->TL.getCount() == tsc);
							assert(temp_real_list.getCount() == tsc);
							if(tsc_test_chunk) {
								//
								// Тестовый блок призванный проверить эквивалентность расчета трендов для построения стратегии и
								// при использовании рядов для реального трейдинга.
								//
								STimeSeries::AnalyzeFitParam afp2(input_frame_size, tsc_test_chunk-trend_nominal_count, trend_nominal_count);
								RealArray test_trend_list;
								RealArray test_err_list;
								THROW(ts_last_chunk.AnalyzeFit("close", afp2, &test_trend_list, 0, &test_err_list, 0, 0));
								uint tcidx = test_trend_list.getCount();
								uint wcidx = p_new_trend_entry->TL.getCount();
								assert(wcidx >= tcidx);
								if(tcidx) do {
									const double tv = test_trend_list.at(--tcidx);
									const double wv = p_new_trend_entry->TL.at(--wcidx);
									assert(tv == wv);
								} while(tcidx);
								//
								uint teidx = test_err_list.getCount();
								uint weidx = temp_real_list.getCount();
								assert(weidx >= teidx);
								if(teidx) do {
									const double tv = test_err_list.at(--teidx);
									const double wv = temp_real_list.at(--weidx);
									assert(tv == wv);
								} while(teidx);
							}
							for(uint trlidx = 0; trlidx < temp_real_list.getCount(); trlidx++) {
								double err = temp_real_list.at(trlidx);
								assert(err >= 0.0);
								if(err > 0.0) {
									const double sqr_err = sqrt(err);
									trls.Step(sqr_err);
									temp_real_list.at(trlidx) = sqr_err;
								}
							}
							trls.Finish();
							p_new_trend_entry->ErrAvg = trls.GetExp();
							p_new_trend_entry->ErrL = temp_real_list;
						}
						trend_list_set.insert(p_new_trend_entry);
					}
#endif // } 0 @v10.6.11
					{
						TSVector <PPObjTimeSeries::StrategyResultEntry> sr_raw_list;
						TSVector <RealRange> main_frame_range_list;
						THROW_SL(ts.GetRealArray(vec_idx, 0, tsc, ts_val_list));
						THROW_SL(ts.GetTimeArray(0, tsc, ts_tm_list));
						assert(ts_tm_list.getCount() == tsc);
						assert(ts_val_list.getCount() == tsc);
					}
					msg_buf.Z().Cat("Selected-Subset").CatDiv(':', 2).Cat("BC-LS").CatChar('-').Cat(sc.getCount());
					f_out.WriteLine(msg_buf.CR());
					for(uint si = 0; si < sc.getCount(); si++) {
						PPObjTimeSeries::StrategyToString(sc.at(si), 0, msg_buf.Z());
						f_out.WriteLine(msg_buf.CR());
					}
					{
						TSVector <PPObjTimeSeries::StrategyResultValueEx> details_list;
						PPObjTimeSeries::StrategyResultEntry sre;
						PROFILE_START
						TsSimulateStrategyContainer(ts_tm_list, ts_val_list, trend_list_set, sc, 0, sre, &details_list);
						PROFILE_END
						OutputStategyResultEntry(sre, msg_buf);
						f_out.WriteLine(msg_buf.CR());
						for(uint dlidx = 0; dlidx < details_list.getCount(); dlidx++) {
							PPObjTimeSeries::StrategyResultValueEx & r_dli = details_list.at(dlidx);
							PPObjTimeSeries::StrategyToString(sc.at(r_dli.StrategyIdx), 0, temp_buf);
							msg_buf.Z().
								Cat(r_dli.TmR.Start, DATF_ISO8601|DATF_CENTURY, 0).
								Tab().Cat(r_dli.TmR.Finish, DATF_ISO8601|DATF_CENTURY, 0).
								Tab().Cat(r_dli.Result, MKSFMTD(0, 5, 0)).
								Tab().Cat(r_dli.TrendErrRel, MKSFMTD(0, 4, 0)).
								Tab().Cat(temp_buf);
							f_out.WriteLine(msg_buf.CR());
						}
						f_out.Flush();
						ok = 1;
					}
				}
				PROFILE_END
			}
		}
	}
	CATCHZOK
	PPWait(0);
	return ok;
}

class TryStrategyContainerResult {
public:
	SLAPI  TryStrategyContainerResult() : TsID(0)
	{
	}
	PPID   TsID;
	PPObjTimeSeries::StrategyContainer Sc;
	TSVector <PPObjTimeSeries::StrategyResultValueEx> DetailsList;
};

class TryStrategyContainerResultCollection : public TSCollection <TryStrategyContainerResult> {
public:
	struct IndexEntry {
		STimeChunk TmR;
		uint   EntryIdx;
		uint   DlIdx;
	};
	SLAPI  TryStrategyContainerResultCollection()
	{
	}
	void   SLAPI GetTsList(PPIDArray & rTsList) const
	{
		rTsList.clear();
		for(uint i = 0; i < getCount(); i++) {
			const TryStrategyContainerResult * p_item = at(i);
			assert(p_item);
			if(p_item)
				rTsList.add(p_item->TsID);
		}
		rTsList.sortAndUndup();
	}
	int    SLAPI MakeIndex();
	uint   SLAPI GetIndexCount() const { return Index.getCount(); }
	const  TryStrategyContainerResult * SLAPI GetEntryByIndex(uint idx, uint * pDetailEntryIdx) const
	{
		const  TryStrategyContainerResult * p_ret = 0;
		if(idx < Index.getCount()) {
			const IndexEntry & r_ie = Index.at(idx);
			assert(r_ie.EntryIdx < getCount());
			if(r_ie.EntryIdx < getCount()) {
				const TryStrategyContainerResult * p_item = at(r_ie.EntryIdx);
				assert(p_item);
				if(p_item) {
					assert(r_ie.DlIdx < p_item->DetailsList.getCount());
					if(r_ie.DlIdx < p_item->DetailsList.getCount()) {
						ASSIGN_PTR(pDetailEntryIdx, r_ie.DlIdx);
						p_ret = p_item;
					}
				}
			}
		}
		return p_ret;
	}
private:
	TSVector <IndexEntry> Index;
};

static IMPL_CMPFUNC(TryStrategyContainerResultCollectionIndexEntry, i1, i2)
{
	const TryStrategyContainerResultCollection::IndexEntry * p1 = static_cast<const TryStrategyContainerResultCollection::IndexEntry *>(i1);
	const TryStrategyContainerResultCollection::IndexEntry * p2 = static_cast<const TryStrategyContainerResultCollection::IndexEntry *>(i2);
	return p1->TmR.cmp(p2->TmR);
}

int SLAPI TryStrategyContainerResultCollection::MakeIndex()
{
	Index.clear();
	int    ok = 1;
	for(uint i = 0; i < getCount(); i++) {
		const TryStrategyContainerResult * p_item = at(i);
		assert(p_item);
		if(p_item) {
			for(uint j = 0; j < p_item->DetailsList.getCount(); j++) {
				PPObjTimeSeries::StrategyResultValueEx & r_dentry = p_item->DetailsList.at(j);
				IndexEntry new_entry;
				new_entry.TmR = r_dentry.TmR;
				new_entry.EntryIdx = i;
				new_entry.DlIdx = j;
				THROW_SL(Index.insert(&new_entry));
			}
		}
	}
	Index.sort(PTR_CMPFUNC(TryStrategyContainerResultCollectionIndexEntry));
	CATCHZOK
	return ok;
}

int SLAPI PrcssrTsStrategyAnalyze::TryStrategyContainers(const PPIDArray & rTsList)
{
	int    ok = -1;
	SString temp_buf;
	SString msg_buf;
	PPTimeSeries ts_rec;
	STimeSeries ts;
	STimeSeries ts_last_chunk;
	ModelParam model_param;
	TryStrategyContainerResultCollection result_collection;
	PPWait(1);
	THROW(ReadModelParam(model_param));
	for(uint ts_idx = 0; ts_idx < rTsList.getCount(); ts_idx++) {
		const PPID ts_id = rTsList.get(ts_idx);
		if(TsObj.Search(ts_id, &ts_rec) > 0) {
			TSCollection <PPObjTimeSeries::TrendEntry> trend_list_set;
			const int gtsr = GetTimeSeries(ts_id, model_param, ts);
			if(gtsr > 0) {
				const uint tsc = ts.GetCount();
				TryStrategyContainerResult * p_result_entry = 0;
				//PPObjTimeSeries::StrategyContainer sc;
				THROW_SL(p_result_entry = result_collection.CreateNewItem());
				p_result_entry->TsID = ts_id;
				TsObj.GetStrategies(ts_id, PPObjTimeSeries::sstSelection, p_result_entry->Sc);
				{
					PROFILE_START
					SString out_file_name;
					(temp_buf = "tsscsim").CatChar('-').Cat(ts_rec.Symb);
					PPGetFilePath(PPPATH_OUT, temp_buf, out_file_name);
					SFile f_out(out_file_name, SFile::mWrite);
					uint  max_opt_delta2_stride = 0;
					DateTimeArray ts_tm_list;
					RealArray ts_val_list;
					LongArray ifs_list;
					RealArray temp_real_list;
					uint   vec_idx = 0;
					THROW_SL(ts.GetValueVecIndex("close", &vec_idx));
					if(tsc > 500 && p_result_entry->Sc.GetInputFramSizeList(ifs_list, &max_opt_delta2_stride) > 0) {
						const uint trend_nominal_count = max_opt_delta2_stride + 3;
						ifs_list.sortAndUndup(); // @paranoic
						ts_last_chunk.Z();
						if(tsc > 10000) {
							ts.GetChunkRecentCount(10000, ts_last_chunk);
						}
						const uint tsc_test_chunk = ts_last_chunk.GetCount();
						{
							THROW(MakeArVectors(ts, ifs_list, mavfDontSqrtErrList, trend_list_set));
							if(tsc_test_chunk) {
								for(uint tlsidx = 0; tlsidx < trend_list_set.getCount(); tlsidx++) {
									PPObjTimeSeries::TrendEntry * p_trend_entry = trend_list_set.at(tlsidx);
									assert(p_trend_entry);
									if(p_trend_entry) {
										const uint input_frame_size = p_trend_entry->Stride;
										//
										// Тестовый блок призванный проверить эквивалентность расчета трендов для построения стратегии и
										// при использовании рядов для реального трейдинга.
										//
										STimeSeries::AnalyzeFitParam afp2(input_frame_size, tsc_test_chunk-trend_nominal_count, trend_nominal_count);
										RealArray test_trend_list;
										RealArray test_err_list;
										THROW(ts_last_chunk.AnalyzeFit("close", afp2, &test_trend_list, 0, &test_err_list, 0, 0));
										uint tcidx = test_trend_list.getCount();
										uint wcidx = p_trend_entry->TL.getCount();
										assert(wcidx >= tcidx);
										if(tcidx) do {
											const double tv = test_trend_list.at(--tcidx);
											const double wv = p_trend_entry->TL.at(--wcidx);
											assert(tv == wv);
										} while(tcidx);
										//
										uint teidx = test_err_list.getCount();
										uint weidx = p_trend_entry->ErrL.getCount();
										assert(weidx >= teidx);
										if(teidx) do {
											const double tv = test_err_list.at(--teidx);
											const double wv = p_trend_entry->ErrL.at(--weidx);
											assert(tv == wv);
										} while(teidx);
										p_trend_entry->SqrtErrList(0); // see flag mavfDontSqrtErrList in MakeArVectors
									}
								}
							}
						}
						{
							TSVector <PPObjTimeSeries::StrategyResultEntry> sr_raw_list;
							TSVector <RealRange> main_frame_range_list;
							THROW_SL(ts.GetRealArray(vec_idx, 0, tsc, ts_val_list));
							THROW_SL(ts.GetTimeArray(0, tsc, ts_tm_list));
							assert(ts_tm_list.getCount() == tsc);
							assert(ts_val_list.getCount() == tsc);
						}
						msg_buf.Z().Cat("Selected-Subset").CatDiv(':', 2).Cat("BC-LS").CatChar('-').Cat(p_result_entry->Sc.getCount());
						f_out.WriteLine(msg_buf.CR());
						for(uint si = 0; si < p_result_entry->Sc.getCount(); si++) {
							PPObjTimeSeries::StrategyToString(p_result_entry->Sc.at(si), 0, msg_buf.Z());
							f_out.WriteLine(msg_buf.CR());
						}
						{
							//TSVector <PPObjTimeSeries::StrategyResultValueEx> details_list;
							PPObjTimeSeries::StrategyResultEntry sre;
							PROFILE_START
							TsSimulateStrategyContainer(ts_tm_list, ts_val_list, trend_list_set, p_result_entry->Sc, 1, sre, &p_result_entry->DetailsList);
							PROFILE_END
							OutputStategyResultEntry(sre, msg_buf);
							f_out.WriteLine(msg_buf.CR());
							for(uint dlidx = 0; dlidx < p_result_entry->DetailsList.getCount(); dlidx++) {
								PPObjTimeSeries::StrategyResultValueEx & r_dli = p_result_entry->DetailsList.at(dlidx);
								PPObjTimeSeries::StrategyToString(p_result_entry->Sc.at(r_dli.StrategyIdx), 0, temp_buf);
								msg_buf.Z().
									Cat(r_dli.TmR.Start, DATF_ISO8601|DATF_CENTURY, 0).
									Tab().Cat(r_dli.TmR.Finish, DATF_ISO8601|DATF_CENTURY, 0).
									Tab().Cat(r_dli.Result, MKSFMTD(0, 5, 0)).
									Tab().Cat(r_dli.TrendErrRel, MKSFMTD(0, 4, 0)).
									Tab().Cat(temp_buf);
								f_out.WriteLine(msg_buf.CR());
							}
							f_out.Flush();
							ok = 1;
						}
					}
					PROFILE_END
				}
			}
		}
	}
	{
		THROW(result_collection.MakeIndex());
		{
			PPIDArray result_ts_list;
			SString out_file_name;
			SString symb;
			(temp_buf = "tsscsim").CatChar('-').Cat("collection").CatChar('-').Cat(getcurdatetime_(), DATF_ISO8601|DATF_CENTURY, TIMF_HMS|TIMF_NODIV);
			PPGetFilePath(PPPATH_OUT, temp_buf, out_file_name);
			SFile f_out(out_file_name, SFile::mWrite);

			result_collection.GetTsList(result_ts_list);
			msg_buf.Z();
			for(uint tsidx = 0; tsidx < result_ts_list.getCount(); tsidx++) {
				const PPID result_ts_id = result_ts_list.get(tsidx);
				if(TsObj.Search(result_ts_id, &ts_rec) > 0) {
					msg_buf.Cat(ts_rec.ID).Space().Cat(ts_rec.Symb).CR();
				}
			}
			f_out.WriteLine(msg_buf.CR());
			LDATE prev_date = ZERODATE;
			uint  win_per_day = 0;
			uint  lose_per_day = 0;
			uint  stake_per_day = 0;
			for(uint rcidx = 0; rcidx < result_collection.GetIndexCount(); rcidx++) {
				uint   dlidx = 0;
				const  TryStrategyContainerResult * p_ri = result_collection.GetEntryByIndex(rcidx, &dlidx);
				if(p_ri) {
					PPObjTimeSeries::StrategyResultValueEx & r_dli = p_ri->DetailsList.at(dlidx);
					{
						if(prev_date && prev_date != r_dli.TmR.Start.d) {
							msg_buf.Z().Cat("--------").
							Tab().Cat(prev_date, DATF_ISO8601|DATF_CENTURY).
							Tab().Cat(stake_per_day).
							Tab().Cat(win_per_day).
							Tab().Cat(lose_per_day);
							f_out.WriteLine(msg_buf.CR());
							win_per_day = 0;
							lose_per_day = 0;
							stake_per_day = 0;
						}
						prev_date = r_dli.TmR.Start.d;
						stake_per_day++;
						if(r_dli.Result > 0.0)
							win_per_day++;
						else if(r_dli.Result < 0.0)
							lose_per_day++;
					}
					PPObjTimeSeries::StrategyToString(p_ri->Sc.at(r_dli.StrategyIdx), 0, temp_buf);
					symb.Z();
					if(TsObj.Search(p_ri->TsID, &ts_rec) > 0)
						symb = ts_rec.Symb;
					if(symb.Empty())
						ideqvalstr(p_ri->TsID, symb);
					msg_buf.Z().
						Cat(symb).
						Tab().Cat(r_dli.TmR.Start, DATF_ISO8601|DATF_CENTURY, 0).
						Tab().Cat(r_dli.TmR.Finish, DATF_ISO8601|DATF_CENTURY, 0).
						Tab().Cat(r_dli.Result, MKSFMTD(0, 5, NMBF_FORCEPOS)).
						Tab().Cat(r_dli.TrendErrRel, MKSFMTD(0, 4, 0)).
						Tab().Cat(temp_buf);
					f_out.WriteLine(msg_buf.CR());
				}
			}
			if(prev_date) {
				msg_buf.Z().Cat("--------").
				Tab().Cat(prev_date, DATF_ISO8601|DATF_CENTURY).
				Tab().Cat(stake_per_day).
				Tab().Cat(win_per_day).
				Tab().Cat(lose_per_day);
				f_out.WriteLine(msg_buf.CR());
				win_per_day = 0;
				lose_per_day = 0;
				stake_per_day = 0;
			}
		}
	}
	CATCHZOK
	PPWait(0);
	return ok;
}

struct TimeSeriesTestParam : public PPExtStrContainer {
	SLAPI  TimeSeriesTestParam() : PPExtStrContainer(), Action(acnNone)
	{
	}
	enum {
		extssParam = 1,
		extssInfo  = 2,
	};
	enum {
		acnNone              = 0,
		acnAnalyzeRegression = 1,
		acnTestStrategies    = 2,
		acnEvalOptMaxDuck    = 3 
	};
	//PPID   TsID;
	long   Action;
	ObjIdListFilt TsList; // @anchor
};

int SLAPI EditTimeSeriesTestParam(TimeSeriesTestParam * pData)
{
	class TimeSeriesTestParamDialog : public TDialog {
		DECL_DIALOG_DATA(TimeSeriesTestParam);
	public:
		TimeSeriesTestParamDialog() : TDialog(DLG_TIMSERTEST)
		{
		}
		DECL_DIALOG_SETDTS()
		{
			int    ok = 1;
			SString temp_buf;
			RVALUEPTR(Data, pData);
			SetupPPObjCombo(this, CTLSEL_TIMSERTEST_TS, PPOBJ_TIMESERIES, Data.TsList.GetSingle(), 0, 0);
			AddClusterAssocDef(CTL_TIMSERTEST_WHAT, 0, Data.acnAnalyzeRegression);
			AddClusterAssoc(CTL_TIMSERTEST_WHAT, 1, Data.acnTestStrategies);
			AddClusterAssoc(CTL_TIMSERTEST_WHAT, 2, Data.acnEvalOptMaxDuck);
			SetClusterData(CTL_TIMSERTEST_WHAT, Data.Action);
			Data.GetExtStrData(Data.extssParam, temp_buf);
			setCtrlString(CTL_TIMSERTEST_PARAM, temp_buf);
			Data.GetExtStrData(Data.extssInfo, temp_buf);
			setCtrlString(CTL_TIMSERTEST_INFO, temp_buf);
			return ok;
		}
		DECL_DIALOG_GETDTS()
		{
			int    ok = 1;
			uint   sel = 0;
			SString temp_buf;
			//getCtrlData(sel = CTLSEL_TIMSERTEST_TS, &Data.TsID);
			PPID   id = getCtrlLong(sel = CTLSEL_TIMSERTEST_TS);
			Data.TsList.Add(id);
			THROW_PP(Data.TsList.GetCount(), PPERR_TIMSERNEEDED);
			GetClusterData(CTL_TIMSERTEST_WHAT, &Data.Action);
			getCtrlString(CTL_TIMSERTEST_PARAM, temp_buf);
			Data.PutExtStrData(Data.extssParam, temp_buf);
			ASSIGN_PTR(pData, Data);
			CATCHZOKPPERRBYDLG
			return ok;
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isCmd(cmTsList)) {
				PPID   id = 0;
				PPIDArray list;
				Data.TsList.Get(list);
				getCtrlData(CTLSEL_TIMSERTEST_TS, &id);
				if(id)
					list.addUnique(id);
				ListToListData data(PPOBJ_TIMESERIES, 0, &list);
				data.TitleStrID = PPTXT_SELTIMSERLIST;
				if(ListToListDialog(&data) > 0) {
					Data.TsList.Set(&list);
					Setup();
				}
				clearEvent(event);
			}
		}
		void   Setup()
		{
			PPID   id = Data.TsList.GetSingle();
			PPID   prev_id = getCtrlLong(CTLSEL_TIMSERTEST_TS);
			if(id != prev_id)
				setCtrlData(CTLSEL_TIMSERTEST_TS, &id);
			if(Data.TsList.GetCount() > 1)
				SetComboBoxListText(this, CTLSEL_TIMSERTEST_TS);
			disableCtrl(CTLSEL_TIMSERTEST_TS, BIN(Data.TsList.GetCount() > 1));
		}
	};
	DIALOG_PROC_BODY(TimeSeriesTestParamDialog, pData);
}

//static
int SLAPI PPObjTimeSeries::TryStrategies(PPID id)
{
	int    ok = -1;
	TimeSeriesTestParam param;
	param.TsList.Add(id);
	if(EditTimeSeriesTestParam(&param) > 0) {
		if(param.Action == TimeSeriesTestParam::acnTestStrategies) {
			PrcssrTsStrategyAnalyze prc;
			if(param.TsList.GetCount()) {
				THROW(prc.TryStrategyContainers(param.TsList.Get()));
			}
			/*for(uint i = 0; i < param.TsList.GetCount(); i++) {
				const PPID ts_id = param.TsList.Get(i);
				THROW(prc.TryStrategyContainer(ts_id));
			}*/
		}
		else if(param.Action == TimeSeriesTestParam::acnAnalyzeRegression) {
			PrcssrTsStrategyAnalyze prc;
			LongArray step_list;
			step_list.add(840);
			for(uint i = 0; i < param.TsList.GetCount(); i++) {
				const PPID ts_id = param.TsList.Get(i);
				THROW(prc.AnalyzeRegression(ts_id, step_list));
			}
		}
		else if(param.Action == TimeSeriesTestParam::acnEvalOptMaxDuck) {
			for(uint i = 0; i < param.TsList.GetCount(); i++) {
				const PPID ts_id = param.TsList.Get(i);
				THROW(PPObjTimeSeries::EvaluateOptimalMaxDuck(ts_id));
			}
		}
	}
	CATCHZOKPPERR
	return ok;
}

//static 
int SLAPI PPObjTimeSeries::EvaluateOptimalMaxDuck(PPID id)
{
	int    ok = -1;
	int    do_update_packet = 0;
	PrcssrTsStrategyAnalyze prc;
	PPTimeSeriesPacket ts_pack;
	PPObjTimeSeries ts_obj;
	uint    result_long = 0;
	uint    result_short = 0;
	if(ts_obj.GetPacket(id, &ts_pack) > 0) {
		STimeSeries ts;
		const int gtsr = ts_obj.GetTimeSeries(id, ts);
		if(gtsr > 0) {
			const uint tsc = ts.GetCount();
			STimeSeries::Stat st(0);
			uint   vec_idx = 0;
			THROW_SL(ts.GetValueVecIndex("close", &vec_idx));
			ts.Analyze("close", st);
			if(ts_pack.Rec.SpikeQuant <= 0.0) {
				ts_pack.Rec.SpikeQuant = st.DeltaAvg / 2.0;
				do_update_packet = 1;
			}
			{
				DateTimeArray ts_tm_list;
				RealArray ts_val_list;
				uint    flags = 0;
				THROW_SL(ts.GetRealArray(vec_idx, 0, tsc, ts_val_list));
				THROW_SL(ts.GetTimeArray(0, tsc, ts_tm_list));
				assert(ts_tm_list.getCount() == tsc);
				assert(ts_val_list.getCount() == tsc);
				THROW(prc.FindOptimalMaxDuck(ts_pack, ts_tm_list, ts_val_list, PrcssrTsStrategyAnalyze::fomdfEntireRange, &result_long));
				THROW(prc.FindOptimalMaxDuck(ts_pack, ts_tm_list, ts_val_list, PrcssrTsStrategyAnalyze::fomdfEntireRange|PrcssrTsStrategyAnalyze::fomdfShort, &result_short));
				if(result_long != ts_pack.Rec.OptMaxDuck) {
					ts_pack.Rec.OptMaxDuck = result_long;
					do_update_packet = 1;
				}
				if(result_short != ts_pack.Rec.OptMaxDuck_S) {
					ts_pack.Rec.OptMaxDuck_S = result_short;
					do_update_packet = 1;
				}
			}
			if(0/*do_update_packet*/) {
				PPID   temp_id = id;
				THROW(ts_obj.PutPacket(&temp_id, &ts_pack, 1));
			}
		}
	}
	CATCHZOK
	return ok;
}
//
//
//
TimSerDetailViewItem::TimSerDetailViewItem() : ItemIdx(0)
{
}

IMPLEMENT_PPFILT_FACTORY(TimSerDetail); SLAPI TimSerDetailFilt::TimSerDetailFilt() : PPBaseFilt(PPFILT_TIMSERDETAIL, 0, 0)
{
	SetFlatChunk(offsetof(TimSerDetailFilt, ReserveStart),
		offsetof(TimSerDetailFilt, Reserve)-offsetof(TimSerDetailFilt, ReserveStart)+sizeof(Reserve));
	Init(1, 0);
}

SLAPI PPViewTimSerDetail::PPViewTimSerDetail() : PPView(0, &Filt, PPVIEW_TIMSERDETAIL), P_DsList(0)
{
	ImplementFlags |= (implBrowseArray);
}

SLAPI PPViewTimSerDetail::~PPViewTimSerDetail()
{
	ZDELETE(P_DsList);
}

int SLAPI PPViewTimSerDetail::Init_(const PPBaseFilt * pBaseFilt)
{
	int    ok = 1;
	THROW(Helper_InitBaseFilt(pBaseFilt));
	Ts.Z();
	CALLPTRMEMB(P_DsList, clear());
	if(Filt.TsID) {
		Obj.GetTimeSeries(Filt.TsID, Ts);
	}
	CATCHZOK
	return ok;
}

int SLAPI PPViewTimSerDetail::MakeList(PPViewBrowser * pBrw)
{
	int    ok = 1;
	if(P_DsList)
		P_DsList->clear();
	else {
		THROW_SL(P_DsList = new SArray(sizeof(uint)));
	}
	for(uint i = 0; i < Ts.GetCount(); i++) {
		uint idx = i+1;
		THROW_SL(P_DsList->insert(&idx));
	}
	CATCHZOK
	return ok;
}

int SLAPI PPViewTimSerDetail::_GetDataForBrowser(SBrowserDataProcBlock * pBlk)
{
	int    ok = 1;
	if(pBlk->P_SrcData && pBlk->P_DestData) {
		ok = 1;
		const  uint idx = *static_cast<const uint *>(pBlk->P_SrcData) - 1;
		int    r = 0;
		if(pBlk->ColumnN == 0)
			pBlk->Set(static_cast<int32>(idx));
		else {
			if(pBlk->ColumnN == 1) {
				SUniTime ut;
				LDATETIME dtm;
				Ts.GetTime(idx, &ut);
				ut.Get(dtm);
				SString & r_temp_buf = SLS.AcquireRvlStr();
				r_temp_buf.Cat(dtm, DATF_ISO8601|DATF_CENTURY, 0);
				pBlk->Set(r_temp_buf);
			}
			else if(pBlk->ColumnN == 2) {
				int32 diffsec = 0;
				if(idx > 0) {
					SUniTime ut;
					LDATETIME dtm;
					LDATETIME dtm2;
					Ts.GetTime(idx, &ut);
					ut.Get(dtm);
					Ts.GetTime(idx-1, &ut);
					ut.Get(dtm2);
					diffsec = diffdatetimesec(dtm, dtm2);
				}
				pBlk->Set(diffsec);
			}
			else if(pBlk->ColumnN > 2 && pBlk->ColumnN < static_cast<int>(Ts.GetValueVecCount()+3)) {
				double value = 0.0;
				Ts.GetValue(idx, pBlk->ColumnN-3, &value);
				pBlk->Set(value);
			}
		}
	}
	return ok;
}

//static
int FASTCALL PPViewTimSerDetail::GetDataForBrowser(SBrowserDataProcBlock * pBlk)
{
	PPViewTimSerDetail * p_v = static_cast<PPViewTimSerDetail *>(pBlk->ExtraPtr);
	return p_v ? p_v->_GetDataForBrowser(pBlk) : 0;
}

void SLAPI PPViewTimSerDetail::PreprocessBrowser(PPViewBrowser * pBrw)
{
	if(pBrw) {
		STimeSeries d;
		const uint tsvc = Ts.GetValueVecCount();
		if(tsvc) {
			SString symb_buf;
			uint   fld_no = 2;
			for(uint i = 0; i < tsvc; i++) {
				int    fxprec = 0;
				TYPEID typ = 0;
				if(Ts.GetValueVecParam(i, &typ, &symb_buf, &fxprec, 0)) {
					const int  prec = (GETSTYPE(typ) == S_FLOAT) ? 5 : fxprec;
					long fmt = MKSFMTD(0, prec, 0);
					fld_no++;
					pBrw->InsColumn(-1, symb_buf, fld_no, T_DOUBLE, fmt, BCO_USERPROC);
				}
			}
		}
		pBrw->SetDefUserProc(PPViewTimSerDetail::GetDataForBrowser, this);
		//pBrw->SetCellStyleFunc(CellStyleFunc, pBrw);
	}
}

int SLAPI PPViewTimSerDetail::OnExecBrowser(PPViewBrowser * pBrw)
{
	return -1;
}

int SLAPI PPViewTimSerDetail::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	PPID   id = pHdr ? *static_cast<const PPID *>(pHdr) : 0;
	const  PPID preserve_id = id;
	if(ok == -2) {
		switch(ppvCmd) {
			case PPVCMD_EXPORT:
				ok = -1;
				//Obj.Export(id);
				break;
			case PPVCMD_REFRESH:
				ok = 1;
				break;
		}
	}
	if(ok > 0) {
		MakeList(pBrw);
		if(pBrw) {
			AryBrowserDef * p_def = static_cast<AryBrowserDef *>(pBrw->getDef());
			if(p_def) {
				SArray * p_array = new SArray(*P_DsList);
				p_def->setArray(p_array, 0, 1);
				pBrw->setRange(p_array->getCount());
				uint   temp_pos = 0;
				long   update_pos = -1;
				if(preserve_id > 0 && P_DsList->lsearch(&preserve_id, &temp_pos, CMPF_LONG))
					update_pos = temp_pos;
				if(update_pos >= 0)
					pBrw->go(update_pos);
				else if(update_pos == MAXLONG)
					pBrw->go(p_array->getCount()-1);
			}
			pBrw->Update();
		}
	}
	return ok;
}

SArray * SLAPI PPViewTimSerDetail::CreateBrowserArray(uint * pBrwId, SString * pSubTitle)
{
	uint   brw_id = BROWSER_TIMSERDETAIL;
	SArray * p_array = 0;
	THROW(MakeList(0));
	p_array = new SArray(*P_DsList);
	CATCH
		ZDELETE(P_DsList);
	ENDCATCH
	ASSIGN_PTR(pBrwId, brw_id);
	return p_array;
}

int SLAPI PPViewTimSerDetail::EditBaseFilt(PPBaseFilt * pBaseFilt)
{
	int    ok = -1;
	return ok;
}

int SLAPI PPViewTimSerDetail::InitIteration()
{
	return -1;
}

int FASTCALL PPViewTimSerDetail::NextIteration(TimSerDetailViewItem * pItem)
{
	return -1;
}
// } @construction
