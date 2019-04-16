// OBJTIMESERIES.CPP
// Copyright (c) A.Sobolev 2018, 2019
// @codepage UTF-8
// Модуль, управляющий объектом данных PPObjTimeSeries
//
#include <pp.h>
#pragma hdrstop
#include <fann2.h>

SLAPI PPTimeSeries::PPTimeSeries() : Tag(PPOBJ_TIMESERIES), ID(0), Flags(0), BuyMarg(0.0), SellMarg(0.0), 
	SpikeQuant(0.0), Prec(0), AvgSpread(0.0), OptMaxDuck(0), OptMaxDuck_S(0), PeakAvgQuant(0), PeakAvgQuant_S(0)
{
	PTR32(Name)[0] = 0;
	PTR32(Symb)[0] = 0;
	PTR32(CurrencySymb)[0] = 0;
	memzero(Reserve, sizeof(Reserve));
	Reserve2[0] = 0;
	Reserve2[1] = 0;
}

int FASTCALL PPTimeSeries::IsEqual(const PPTimeSeries & rS) const
{
	int    eq = 1;
	if(!sstreq(Name, rS.Name))
		eq = 0;
	else if(!sstreq(Symb, rS.Symb))
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
	else if(!sstreq(CurrencySymb, rS.CurrencySymb))
		eq = 0;
	return eq;	
}

SLAPI PPObjTimeSeries::Config::Entry::Entry() : TsID(0), Flags(0)
{
	memzero(Reserve, sizeof(Reserve));
}

PPObjTimeSeries::Config::ExtBlock::ExtBlock() : MaxAvgTimeSec(0), TsFlashTimer(0)
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
public:
	TimSerCfgDialog() : PPListDialog(DLG_TIMSERCFG, CTL_TIMSERCFG_LIST)
	{
		updateList(-1);
	}
	int    setDTS(const PPObjTimeSeries::Config * pData)
	{
		int    ok = 1;
		RVALUEPTR(Data, pData);
		setCtrlData(CTL_TIMSERCFG_MAXSTAKE, &Data.MaxStakeCount);
		setCtrlReal(CTL_TIMSERCFG_ALIMPART, Data.AvailableLimitPart);
		setCtrlReal(CTL_TIMSERCFG_ALIMABS, Data.AvailableLimitAbs);
		setCtrlData(CTL_TIMSERCFG_MINPOTENT, &Data.MinPerDayPotential);
		setCtrlData(CTL_TIMSERCFG_TSFLSHTMR, &Data.E.TsFlashTimer); // @v10.3.3
		AddClusterAssoc(CTL_TIMSERCFG_FLAGS, 0, PPObjTimeSeries::Config::fTestMode);
		AddClusterAssoc(CTL_TIMSERCFG_FLAGS, 1, PPObjTimeSeries::Config::fUseStakeMode2); // @v10.3.3
		AddClusterAssoc(CTL_TIMSERCFG_FLAGS, 2, PPObjTimeSeries::Config::fUseStakeMode3); // @v10.3.3
		SetClusterData(CTL_TIMSERCFG_FLAGS, Data.Flags);
		updateList(-1);
		return ok;
	}
	int    getDTS(PPObjTimeSeries::Config * pData)
	{
		int    ok = 1;
		getCtrlData(CTL_TIMSERCFG_MAXSTAKE, &Data.MaxStakeCount);
		getCtrlData(CTL_TIMSERCFG_ALIMPART, &Data.AvailableLimitPart);
		getCtrlData(CTL_TIMSERCFG_ALIMABS, &Data.AvailableLimitAbs);
		getCtrlData(CTL_TIMSERCFG_MINPOTENT, &Data.MinPerDayPotential);
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
	PPObjTimeSeries::Config Data;
};

//static 
int SLAPI PPObjTimeSeries::EditConfig(const PPObjTimeSeries::Config * pCfg)
{
	int    ok = -1;
	PPObjTimeSeries::Config config;
	if(pCfg)
		config = *pCfg;
	else {
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

int SLAPI PPObjTimeSeries::EditDialog(PPTimeSeries * pEntry)
{
	class TimeSeriesDialog : public TDialog {
	public:
		TimeSeriesDialog() : TDialog(DLG_TIMSER)
		{
		}
		int    setDTS(const PPTimeSeries * pData)
		{
			RVALUEPTR(Data, pData);
			int    ok = 1;
			SString temp_buf;
			setCtrlLong(CTL_TIMSER_ID, Data.ID);
			setCtrlString(CTL_TIMSER_NAME, temp_buf = Data.Name);
			setCtrlString(CTL_TIMSER_SYMB, temp_buf = Data.Symb);
			AddClusterAssoc(CTL_TIMSER_FLAGS, 0, PPCommObjEntry::fPassive);
			SetClusterData(CTL_TIMSER_FLAGS, Data.Flags);
			disableCtrl(CTL_TIMSER_ID, (!PPMaster || Data.ID));

			setCtrlData(CTL_TIMSER_PREC, &Data.Prec);
			setCtrlReal(CTL_TIMSER_BUYMARG, Data.BuyMarg);
			setCtrlReal(CTL_TIMSER_SELLMARG, Data.SellMarg);
			setCtrlReal(CTL_TIMSER_QUANT, Data.SpikeQuant);
			setCtrlReal(CTL_TIMSER_AVGSPRD, Data.AvgSpread);
			setCtrlData(CTL_TIMSER_OPTMD, &Data.OptMaxDuck);
			setCtrlData(CTL_TIMSER_OPTMDS, &Data.OptMaxDuck_S);
			return ok;
		}
		int    getDTS(PPTimeSeries * pData)
		{
			int    ok = 1;
			uint   sel = 0;
			PPID   _id = Data.ID;
			SString temp_buf;
			getCtrlData(CTL_TIMSER_ID, &_id);
			getCtrlString(sel = CTL_TIMSER_NAME, temp_buf);
			THROW(Obj.CheckName(_id, temp_buf, 1));
			STRNSCPY(Data.Name, temp_buf);
			getCtrlString(sel = CTL_TIMSER_SYMB, temp_buf);
			THROW(Obj.ref->CheckUniqueSymb(Obj.Obj, _id, temp_buf, offsetof(ReferenceTbl::Rec, Symb)));
			STRNSCPY(Data.Symb, temp_buf);
			GetClusterData(CTL_TIMSER_FLAGS, &Data.Flags);
			ASSIGN_PTR(pData, Data);
			CATCH
				ok = PPErrorByDialog(this, sel);
			ENDCATCH
			return ok;
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isCmd(cmTimSerStat)) {
				SString temp_buf;
				SString out_buf;
				if(Data.ID) {
					STimeSeries ts;
					if(Obj.GetTimeSeries(Data.ID, ts) > 0) {
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
							out_buf.CatEq("spread-avg", Data.AvgSpread, MKSFMTD(0, 1, 0)).Space().CRB();
							out_buf.CatEq("spike-quant", Data.SpikeQuant, MKSFMTD(0, 9, 0)).Space().CRB();
							out_buf.CatEq("buy-margin", Data.BuyMarg, MKSFMTD(0, 6, 0)).Space().CRB();
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
									if(!Obj.SetTimeSeries(Data.ID, &ts, 1))
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
		PPTimeSeries Data;
	};
	int    ok = -1;
	TimeSeriesDialog * p_dlg = 0;
	if(pEntry) {
		SString obj_title;
		THROW(CheckDialogPtr(&(p_dlg = new TimeSeriesDialog())));
		THROW(EditPrereq(&pEntry->ID, p_dlg, 0));
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
	PPTimeSeries rec;
	THROW(EditPrereq(pID, 0, &is_new));
	MEMSZERO(rec);
	if(!is_new) {
		THROW(Search(*pID, &rec) > 0);
	}
	{
		THROW(ok = EditDialog(&rec));
		if(ok > 0) {
			THROW(is_new || CheckRights(PPR_MOD));
			if(*pID)
				*pID = rec.ID;
			THROW(PutPacket(pID, &rec, 1));
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

int SLAPI PPObjTimeSeries::PutPacket(PPID * pID, PPTimeSeries * pPack, int use_ta)
{
	int    ok = 1;
	PPID   _id = DEREFPTRORZ(pID);
	const  int is_new = (_id == 0);
	const  int is_removing = BIN(*pID != 0 && pPack == 0);
	PPID   hid = 0;
	PPTimeSeries org_pack;
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
			THROW(CheckRightsModByID(pID));
			if(_id) {
				if(pPack->IsEqual(org_pack))
					ok = -1;
				else {
					THROW(ref->UpdateItem(Obj, _id, pPack, 1, 0));
					//THROW(ref->Ot.PutList(Obj, _id, &pPack->TagL, 0));
					DS.LogAction(PPACN_OBJUPD, Obj, _id, 0, 0);
				}
			}
			else {
				THROW(ref->AddItem(Obj, &_id, pPack, 0));
				pPack->ID = _id;
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
				pPack->ID = 0;
		}
		ok = 0;
	ENDCATCH
	return ok;
}

int SLAPI PPObjTimeSeries::GetPacket(PPID id, PPTimeSeries * pPack)
{
	return Search(id, pPack);
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

int SLAPI PPObjTimeSeries::Browse(void * extraPtr)
{
	class TsViewDialog : public ObjViewDialog {
	public:
		TsViewDialog(PPObjTimeSeries * pObj) : ObjViewDialog(DLG_TSVIEW, pObj, 0)
		{
		}
	private:
		virtual void extraProc(long id)
		{
			PPObjTimeSeries * p_obj = static_cast<PPObjTimeSeries *>(P_Obj);
			if(p_obj) {
				PPTimeSeries ts_rec;
				if(p_obj->Search(id, &ts_rec) > 0) {
					SString temp_buf;
					SString file_name;
					SString line_buf;
					temp_buf.Z().Cat("ts-raw").CatChar('-').Cat(ts_rec.Symb).Dot().Cat("txt").ToLower();
					PPGetFilePath(PPPATH_OUT, temp_buf, file_name);
					SFile f_out(file_name, SFile::mWrite);
					if(f_out.IsValid()) {
						STimeSeries ts;
						StrategyContainer scontainer;
						if(p_obj->GetTimeSeries(id, ts) > 0) {
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
									line_buf.Z().Cat(t, DATF_ISO8601, 0).Tab().Cat(v, MKSFMTD(10, 5, 0)).Tab().Cat(td);
									f_out.WriteLine(line_buf.CR());
									t_prev = t;
								}
							}
							if(p_obj->GetStrategies(id, scontainer) > 0) {
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
										CatEq("OptDeltaCount", r_s.OptDeltaCount).Space();
									line_buf.CatEq("OptDelta2Stride", r_s.OptDelta2Stride).Space();
									temp_buf.Z().Cat(r_s.OptDelta2Range.low, MKSFMTD(0, 9, 0)).Dot().Dot().Cat(r_s.OptDelta2Range.upp, MKSFMTD(0, 9, 0));
									line_buf.CatEq("OptDelta2Range", temp_buf).Space().
										CatEq("OptDelta2Count", r_s.OptDelta2Count).Space().
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
		}
	};
	//return RefObjView(this, 0, 0);
	//int SLAPI RefObjView(PPObject * pObj, long charryID, void * extraPtr)
	{
		int    ok = 1;
		TDialog * dlg = new TsViewDialog(this);
		if(CheckDialogPtrErr(&dlg))
			ExecViewAndDestroy(dlg);
		else
			ok = 0;
		return ok;
	}
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
				PPTimeSeries ts_rec;
				STimeSeries ex_ts;
				int gts = 0;
				if(SearchBySymb("eurusd", &id, &ts_rec) > 0) {
					gts = GetTimeSeries(id, ex_ts);
				}
				else {
					MEMSZERO(ts_rec);
					STRNSCPY(ts_rec.Name, "eurusd");
					STRNSCPY(ts_rec.Symb, "eurusd");
					THROW(PutPacket(&id, &ts_rec, 1));
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
	SLAPI  StrategyOptEntry(double factor, double result) : Factor(factor), Result(result)
	{
	}
	double Factor;
	double Result;
};

static double CalcFactorRangeResultSum(const TSVector <StrategyOptEntry> & rList, uint firstIdx, uint lastIdx)
{
	/*
	int   sum_err = 0;
	double sum = 0.0;
	const uint _c = rList.getCount();
	assert(firstIdx <= lastIdx);
	assert(lastIdx <= _c);
	for(uint i = firstIdx; i < _c && i <= lastIdx; i++) {
		sum += rList.at(i).Result;
	}
	double s2 = rList.sumDouble(offsetof(StrategyOptEntry, Result), firstIdx, lastIdx); // @debug
	if(sum != s2) {
		sum_err = 1;
	}
	//assert(sum == s2); // @debug
	return sum;
	*/
	return rList.sumDouble(offsetof(StrategyOptEntry, Result), firstIdx, lastIdx);
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
//
// useInitialSplitting:
//   0 - первая итерация, как и последующие, делит список на две равные части
//   1 - первая итерация делит список на положительную и отрицательную части
//   2 - первая итерация запускает обработку только положительных значений списка
//   3 - первая итерация запускает обработку только отрицательных значений списка
//   4 - последовательное сканирование всего диапазона фраймами для нахождения максимального
//
static int SLAPI FindOptimalFactorRange(const TSVector <StrategyOptEntry> & rList, int useInitialSplitting, RealRange & rR, uint32 & rRangeCount, double & rResult)
{
	int    ok = -1;
	const  uint _c = rList.getCount();
	rR.Z();
	rRangeCount = 0;
	rResult = 0.0;
	//const  int interval_exp_direction = 1; // 0 - не раздвигать интервалы, 1 - только расширять, 2 - только сжимать, 3 - расширять и сжимать
	const  int  interval_exp_direction = (useInitialSplitting == 4) ? 3 : 1; // @20190402 // @20190413 (3 : 1)-->(3 : 3)
	const  uint maxprobe = 15; // @v10.3.5 20-->30 // @v10.3.6 30-->100 // @v10.3.8 100-->200 // @v10.3.9 200-->50 // @v20190413 50-->15
	//const  uint sfdelta  = 100; // @v10.4.0 Шаг итерации при последовательном поиске оптимального диапазона (useInitialSplitting == 4)
	double _sfd_max_result = 0.0;
	uint   _sfd_max_loidx = 0;
	uint   _sfd_max_upidx = 0;
	uint   _sfd_best = 0;
	if(_c > 1) {
		const  double total_sum = CalcFactorRangeResultSum(rList, 0, _c-1);
		for(uint sfdelta = 50; sfdelta <= 1200; sfdelta += 50) { // @v20190413 (<=1000)-->(<=1200)
			int    do_break = 0; // Сигнал для выхода из функции из-за того, что приемлемых значений больше нет
			// (must be done by caller) rList.sort(PTR_CMPFUNC(double));
			double prev_result = total_sum;
			uint   lo_idx = 0;
			uint   up_idx = _c-1;
			uint   iter_no = 0;
			if(useInitialSplitting == 4) {
				double _max_result = 0.0;
				uint   _max_sfidx = 0;
				uint   _max_sfidx_up = 0;
				for(uint sfidx = 0; ; sfidx += (sfdelta*3/4)) { // @v20190413 (+=sfdelta)--(+=(sfdelta*3/4))
					uint sfidx_up = MIN(sfidx+sfdelta-1, rList.getCount()-1);
					if(sfidx_up > sfidx) {
						const double _result = CalcFactorRangeResultSum(rList, sfidx, sfidx_up);
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
			}
			else {
				while(!do_break && lo_idx < up_idx) {
					uint   mid_idx = 0;
					mid_idx = (lo_idx + up_idx) / 2;
					//
					// Специальная обработка первой итерации
					//
					if(iter_no == 0) {
						const double first_val = rList.at(0).Factor;
						const double last_val = rList.at(_c-1).Factor;
						if(useInitialSplitting == 1) { // первая итерация делит список на положительную и отрицательную части
							if(first_val < 0.0 && last_val > 0.0) {
								for(uint i = 0; i < _c; i++) {
									const StrategyOptEntry & r_entry = rList.at(i);
									if(r_entry.Factor > 0.0) {
										mid_idx = i-1;
										break;
									}
								}
							}
						}
						else if(useInitialSplitting == 2) { // первая итерация запускает обработку только положительных значений списка
							if(first_val < 0.0 && last_val > 0.0) {
								for(uint i = 0; i < _c; i++) {
									const StrategyOptEntry & r_entry = rList.at(i);
									if(r_entry.Factor > 0.0) {
										lo_idx = i-1;
										mid_idx = (lo_idx + up_idx) / 2;
										break;
									}
								}
							}
							else if(last_val < 0.0)
								do_break = 1;
						}
						else if(useInitialSplitting == 3) { // первая итерация запускает обработку только отрицательных значений списка
							if(first_val < 0.0 && last_val > 0.0) {
								for(uint i = 0; i < _c; i++) {
									const StrategyOptEntry & r_entry = rList.at(i);
									if(r_entry.Factor >= 0.0) {
										if(i > 0) {
											up_idx = i-1;
											mid_idx = (lo_idx + up_idx) / 2;
										}
										break;
									}
								}
							}
							else if(first_val > 0.0)
								do_break = 1;
						}
					}
					if(!do_break) {
						const double lo_result = CalcFactorRangeResultSum(rList, lo_idx, mid_idx);
						const double up_result = CalcFactorRangeResultSum(rList, mid_idx+1, up_idx);
						if(MAX(lo_result, up_result) < prev_result) {
							break;
						}
						else {
							if(lo_result > up_result) {
								prev_result = lo_result;
								up_idx = mid_idx;
							}
							else {
								prev_result = up_result;
								lo_idx = mid_idx+1;
							}
						}
						iter_no++;
					}
				}
			}
			if(!do_break) {
				if(interval_exp_direction & 1) { // расширять интервал
					while(lo_idx > 0) {
						int done = 0;
						for(uint probedeep = 1; !done && probedeep <= maxprobe && probedeep <= lo_idx; probedeep++) { // @v10.3.12 @fix (probedeep > lo_idx)-->(probedeep <= lo_idx)
							const double temp_result = CalcFactorRangeResultSum(rList, lo_idx-probedeep, up_idx);
							if(prev_result < temp_result) {
								prev_result = temp_result;
								lo_idx -= probedeep;
								done = 1;
							}
						}
						if(!done)
							break;
					}
				}
				if(interval_exp_direction & 1) { // расширять интервал
					while((up_idx+1) < _c) {
						int done = 0;
						for(uint probedeep = 1; !done && probedeep <= maxprobe && (up_idx+probedeep) < _c; probedeep++) {
							const double temp_result = CalcFactorRangeResultSum(rList, lo_idx, up_idx+probedeep);
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
							const double temp_result = CalcFactorRangeResultSum(rList, lo_idx+probedeep, up_idx);
							if(prev_result < temp_result) {
								prev_result = temp_result;
								lo_idx += probedeep;
								done = 1;
							}
						}
						if(!done)
							break;
					}
				}
				if(interval_exp_direction & 2) { // сжимать интервал
					while(lo_idx < up_idx) { // Сдвигаем верхнюю границу вниз
						int done = 0;
						for(uint probedeep = 1; !done && probedeep <= maxprobe && (up_idx) >= (lo_idx+probedeep); probedeep++) {
							const double temp_result = CalcFactorRangeResultSum(rList, lo_idx, up_idx-probedeep);
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
			rResult = _sfd_max_result;
			rRangeCount = (_sfd_max_upidx - _sfd_max_loidx + 1);
			if(_sfd_max_loidx > 0 || _sfd_max_upidx < (_c-1))
				ok = 1;
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

SLAPI PPObjTimeSeries::StrategyResultValueEx::StrategyResultValueEx() : StrategyResultValue(), Peak(0.0), Bottom(0.0), LastPoint(0)
{
}

PPObjTimeSeries::StrategyResultValueEx & SLAPI PPObjTimeSeries::StrategyResultValueEx::Z()
{
	StrategyResultValue::Z();
	Peak = 0.0;
	Bottom = 0.0;
	LastPoint = 0;
	return *this;
}

SLAPI PPObjTimeSeries::StrategyResultEntry::StrategyResultEntry() : Strategy(), 
	LastResultIdx(0), OptDeltaResult(0.0), OptDelta2Result(0), SumPeak(0.0), SumBottom(0.0), MaxPeak(0.0), TotalSec(0)
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
	OptDeltaCount = rS.OptDeltaCount;
	OptDelta2Count = rS.OptDelta2Count;
	PeakAvgQuant = rS.PeakAvgQuant;
	BottomAvgQuant = rS.BottomAvgQuant;
	PeakMaxQuant = rS.PeakMaxQuant;
	TrendErrAvg = rS.TrendErrAvg; // @v10.3.12
	TrendErrLim = rS.TrendErrLim; // @v10.3.12
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
	OptDeltaCount = rTnnp.OptDeltaCount;
	OptDelta2Count = rTnnp.OptDelta2Count;
	PeakAvgQuant = rTnnp.PeakAvgQuant;
	BottomAvgQuant = rTnnp.BottomAvgQuant;
	PeakMaxQuant = rTnnp.PeakMaxQuant;
	TrendErrAvg = rTnnp.TrendErrAvg; // @v10.3.12
	TrendErrLim = rTnnp.TrendErrLim; // @v10.3.12
}

SLAPI PPObjTimeSeries::TrendEntry::TrendEntry(uint stride, uint nominalCount) : Stride(stride), NominalCount(nominalCount), ErrAvg(0.0)
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

SLAPI PPObjTimeSeries::BestStrategyBlock::BestStrategyBlock() : MaxResult(0.0), MaxResultIdx(-1), TvForMaxResult(0.0), Tv2ForMaxResult(0.0)
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
int SLAPI PPObjTimeSeries::MatchStrategy(const PPObjTimeSeries::TrendEntry * pTrendEntry, int lastIdx, const Strategy & rS, double & rResult, double & rTv, double & rTv2)
{
	int   ok = 0;
	const uint tlc = pTrendEntry ? pTrendEntry->TL.getCount() : 0;
	if(tlc && rS.V.TmSec > 0) { // Если rS.V.TmSec == 0, то результат не был вычислен на этапе подготовки модели, либо не встретилось ни одного случая.
		assert(rS.InputFrameSize == pTrendEntry->Stride);
		const uint trend_idx = (lastIdx < 0) ? (tlc-1) : static_cast<uint>(lastIdx);
		const double trend_err = pTrendEntry->ErrL.at(trend_idx); // @v10.3.12
		const double trend_err_limit = (rS.TrendErrLim * rS.TrendErrAvg); // @v10.3.12
		switch(rS.StakeMode) {
			case 1:
				rTv = pTrendEntry->TL.at(trend_idx);
				if(trend_err_limit <= 0.0 || (trend_err > 0.0 && trend_err <= trend_err_limit)) {
					if(rS.OptDeltaRange.Check(rTv)) {
						rResult = rS.V.GetResultPerDay();
						ok = 1;
					}
				}
				break;
			case 2:
				if(tlc >= rS.OptDelta2Stride) {
					rTv = pTrendEntry->TL.StrideDifference(trend_idx, rS.OptDelta2Stride);
					if(rS.OptDelta2Range.Check(rTv)) {
						rResult = rS.V.GetResultPerDay();
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
								ok = 1;
							}
						}
					}
				}
				break;
		}
	}
	return ok;
}

//static 
int SLAPI PPObjTimeSeries::MatchStrategy(const TSCollection <PPObjTimeSeries::TrendEntry> & rTrendList, int lastIdx, const Strategy & rS, double & rResult, double & rTv, double & rTv2)
{
	return MatchStrategy(SearchTrendEntry(rTrendList, rS.InputFrameSize), lastIdx, rS, rResult, rTv, rTv2);
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
		{
			const double spread_adjustment = 1.1; // Поправка запаса прочности для размера комиссии
			const double spread = (rS.Prec > 0 && rS.SpreadAvg > 0.0) ? (rS.SpreadAvg * fpow10i(-rS.Prec) * spread_adjustment) : 0.0;
			const double margin = rS.Margin;
			uint k = valueIdx+1;
			if(is_short) {
				const double target_value = rS.TargetQuant ? round(stake_value * (1.0 - (rS.TargetQuant * rS.SpikeQuant)), prec) : 0.0;
				const double org_sl = Implement_CalcSL_Short(mdv, prec, peak);
				double sl = org_sl;
				while(k < tsc) {
					const double value = rValList.at(k);
					const double value_with_spread = (value + spread);
					const double stake_delta = -((value - stake_value) / stake_value);
					rV.Result = -((value_with_spread - stake_value) / stake_value) / margin;
					SETMAX(bottom, value); // @v10.3.2
					if(value >= sl) {
						ok = 2;
						break;
					}
					else if(rS.TargetQuant && value <= target_value) {
						ok = 2;
						break;
					}
					else {
						assert(bottom <= org_sl);
						if(peak > value) {
							peak = value;
							sl = Implement_CalcSL_Short(mdv, prec, peak);
						}
						k++;
					}
				}
			}
			else {
				const double target_value = rS.TargetQuant ? round(stake_value * (1.0 + (rS.TargetQuant * rS.SpikeQuant)), prec) : 0.0;
				const double org_sl = Implement_CalcSL_Long(mdv, prec, peak);
				double sl = org_sl;
				while(k < tsc) {
					const double value = rValList.at(k);
					const double value_with_spread = (value - spread);
					const double stake_delta = ((value - stake_value) / stake_value);
					rV.Result = ((value_with_spread - stake_value) / stake_value) / margin;
					SETMIN(bottom, value); // @v10.3.2
					if(value <= sl) {
						ok = 2;
						break;
					}
					else if(rS.TargetQuant && value >= target_value) {
						ok = 2;
						break;
					}
					else {
						assert(bottom >= org_sl);
						if(peak < value) {
							peak = value;
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
			}
		}
	}
	return ok;
}

static void SLAPI TsSimulateStrategyContainer(const DateTimeArray & rTmList, const RealArray & rValList, 
	const TSCollection <PPObjTimeSeries::TrendEntry> & rTrendList, const PPObjTimeSeries::StrategyContainer & rSc, PPObjTimeSeries::StrategyResultEntry & rSre)
{
	const uint tsc = rTmList.getCount();
	assert(tsc == rValList.getCount());
	if(tsc && tsc == rValList.getCount()) {
		uint max_ifs = 0;
		uint max_delta2_stride = 0;
		SCollection trend_to_st_assoc_list(O_COLLECTION & ~aryDataOwner);
		for(uint tidx = 0; tidx < rTrendList.getCount(); tidx++) {
			const PPObjTimeSeries::TrendEntry * p_te = rTrendList.at(tidx);
			assert(p_te);
			if(p_te) {
				assert(p_te->TL.getCount() == tsc);
				SETMAX(max_ifs, p_te->Stride);
			}
		}
		for(uint si = 0; si < rSc.getCount(); si++) {
			const PPObjTimeSeries::Strategy & r_s = rSc.at(si);
			PPObjTimeSeries::StrategyResultEntry empty_result_entry;
			if(oneof3(r_s.StakeMode, 2, 3, 4)) // @v10.3.9 (4)
				SETMAX(max_delta2_stride, r_s.OptDelta2Stride);
			{
				const PPObjTimeSeries::TrendEntry * p_te = PPObjTimeSeries::SearchTrendEntry(rTrendList, r_s.InputFrameSize);
				trend_to_st_assoc_list.insert(p_te);
			}
		}
		assert(trend_to_st_assoc_list.getCount() == rSc.getCount()); // @paranoic 
		const uint _start_offset = max_ifs+max_delta2_stride;
		for(uint init_offset = _start_offset, test_no = 0; test_no < 100; (init_offset += 173), (test_no++)) {
			const LDATETIME start_tm = rTmList.at(init_offset);
			LDATETIME finish_tm = ZERODATETIME;
			for(uint i = init_offset; i < tsc; i++) {
				PPObjTimeSeries::BestStrategyBlock _best_result;
				for(uint si = 0; si < rSc.getCount(); si++) {
					const PPObjTimeSeries::Strategy & r_s = rSc.at(si);
					const PPObjTimeSeries::TrendEntry * p_te = static_cast<const PPObjTimeSeries::TrendEntry *>(trend_to_st_assoc_list.at(si)); // @v10.3.10
					if(p_te) {
						double cr = 0.0;
						double _tv = 0.0; // Трендовое значение (вычисление зависит от модели)
						double _tv2 = 0.0; // Второе трендовое значение (вычисляется для комбинированной модели)
						// @v10.3.10 {
						if(PPObjTimeSeries::MatchStrategy(p_te, static_cast<int>(i), r_s, cr, _tv, _tv2))
							_best_result.SetResult(cr, si, _tv, _tv2);
						// } @v10.3.10 
						/* @v10.3.10 if(PPObjTimeSeries::MatchStrategy(rTrendList, static_cast<int>(i), r_s, cr, _tv, _tv2))
							_best_result.SetResult(cr, si, _tv, _tv2);*/
					}
				}
				if(_best_result.MaxResultIdx >= 0) {
					const PPObjTimeSeries::Strategy & r_s = rSc.at(_best_result.MaxResultIdx);
					PPObjTimeSeries::StrategyResultValueEx rv_ex;
					const int csr = TsCalcStrategyResult2(rTmList, rValList, r_s, i, rv_ex);
					if(csr == 2) {
						rSre.SetValue(rv_ex);
						const LDATETIME local_finish_tm = rTmList.at(rv_ex.LastPoint);
						if(cmp(finish_tm, local_finish_tm) < 0)
							finish_tm = local_finish_tm;
						i = rv_ex.LastPoint+1; // Дальше продолжим со следующей точки 
					}
					else {
						break; // Ряд оборвался - дальше анализировать нельзя: некоторые стратегии не имеют достаточно данных.
					}
				}
			}
			if(!!finish_tm)
				rSre.TotalSec += diffdatetimesec(finish_tm, start_tm);
		}
	}
}

static int SLAPI FindOptimalFactorRange_SecondKind(/*const RealArray & rTrendList*/const PPObjTimeSeries::TrendEntry & rTe, const RealArray & rResultList, PPObjTimeSeries::StrategyResultEntry & rSre)
{
	assert(rResultList.getCount() == rTe.TL.getCount());
	int    ok = -1;
	const uint ifs = rSre.InputFrameSize;
	// @v10.3.10 const uint max_stride = 20; // @v10.3.1 4-->6 // 6-->10 // @v10.3.5 10-->20
	const uint min_stride = ifs / 16; // @v10.3.10 3-->ifs / 16
	const uint max_stride = ifs / 4;
	TSVector <StrategyOptEntry> so_list;
	double max_result = 0.0;
	for(uint stride = min_stride; stride <= max_stride; stride++) { // @v10.3.9 (stride = 1)-->(stride = 3)
		so_list.clear();
		for(uint i = 0; i <= rSre.LastResultIdx; i++) {
			if(i >= (/*first_correl_idx*/ifs+stride)) {
				StrategyOptEntry entry(rTe.TL.StrideDifference(i, stride), rResultList.at(i));
				THROW_SL(so_list.insert(&entry));
			}
		}
		RealRange opt_range;
		uint32 opt_count;
		double opt_result;
		so_list.sort(PTR_CMPFUNC(double));
		//const int initial_splitting = 1;
		//const int initial_splitting = (rSre.BaseFlags & rSre.bfShort) ? 3 : 2;
		const int initial_splitting = 0;
		FindOptimalFactorRange(so_list, initial_splitting, opt_range, opt_count, opt_result);
		if(max_result < opt_result) {
			max_result = opt_result;
			rSre.OptDelta2Result = opt_result;
			rSre.OptDelta2Count = opt_count;
			rSre.OptDelta2Range = opt_range;
			rSre.OptDelta2Stride = stride;
			ok = 1;
		}
	}
	CATCHZOK
	return ok;
}

static void FASTCALL AddEntryToResultList(const PPObjTimeSeries::StrategyResultValueEx & rRvEx, RealArray & rResultList)
{
	rResultList.add(rRvEx.Result/*rRvEx.GetResultPerDay()*/); // @optvector
}

static int Helper_TsTestStrategy2_Mode1(const DateTimeArray & rTmList, const RealArray & rValList, /*const RealArray & rTrendList,*/const PPObjTimeSeries::TrendEntry & rTe, 
	const PPObjTimeSeries::Strategy & rS, RealArray & rResultList, PPObjTimeSeries::StrategyResultEntry & rSre, uint & rSignalCount)
{
	rSignalCount = 0;
	const uint tsc = rTmList.getCount();
	const uint ifs = rS.InputFrameSize;
	int    ok = 1;
	const  double trend_err_limit = (rS.TrendErrLim * rS.TrendErrAvg); // @v10.3.12
	for(uint i = 0; i < tsc; i++) {
		PPObjTimeSeries::StrategyResultValueEx rv_ex;
		int    is_signal = 0;
		if(i >= (ifs+1)) {
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
			rSignalCount++;
			AddEntryToResultList(rv_ex, rResultList);
			rSre.SetValue(rv_ex);
		}
		else
			rResultList.add(0.0);
	}
	CATCHZOK
	return ok;
}

static int SLAPI TsTestStrategy2(const DateTimeArray & rTmList, const RealArray & rValList, /*const RealArray & rTrendList,*/const PPObjTimeSeries::TrendEntry & rTe,
	const PPObjTimeSeries::Strategy & rS, int doFindOptimalFactorRange, PPObjTimeSeries::StrategyResultEntry & rSre)
{
	int    ok = 1;
	const  uint tsc = rTmList.getCount();
	const  uint ifs = rS.InputFrameSize;
	const  double trend_err_limit = (rS.TrendErrLim * rS.TrendErrAvg); // @v10.3.12
	assert(tsc == rValList.getCount());
	if(tsc && tsc == rValList.getCount()) {
		TSVector <StrategyOptEntry> so_list;
		RealArray result_list;
		uint   signal_count = 0;
		rSre.LastResultIdx = 0;
		assert(tsc == rTe.TL.getCount());
		THROW(rTe.TL.getCount() == tsc);
		PROFILE_START
		if(rSre.StakeMode == 0) {
			for(uint i = 0; i < tsc; i++) {
				PPObjTimeSeries::StrategyResultValueEx rv_ex;
				int    is_signal = 1;
				const  int csr = TsCalcStrategyResult2(rTmList, rValList, rS, i, rv_ex);
				THROW(csr);
				is_signal = BIN(csr == 2);
				if(is_signal) {
					signal_count++;
					AddEntryToResultList(rv_ex, result_list);
					rSre.SetValue(rv_ex);
					rSre.LastResultIdx = i;
				}
				else
					result_list.add(0.0);
			}
		}
		else if(rSre.StakeMode == 1) {
			THROW(Helper_TsTestStrategy2_Mode1(rTmList, rValList, /*rTrendList*/rTe, rS, result_list, rSre, signal_count));
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
					AddEntryToResultList(rv_ex, result_list);
					rSre.SetValue(rv_ex);
				}
				else
					result_list.add(0.0);
			}
		}
		else if(oneof2(rSre.StakeMode, 3, 4)) {
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
					AddEntryToResultList(rv_ex, result_list);
					rSre.SetValue(rv_ex);
				}
				else
					result_list.add(0.0);
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
		/* @v10.3.9 {
			const double tr = result_list.Sum();
			assert(feqeps(rSre.V.Result, tr, 1e-7));
		}*/
		PROFILE_END
		if(doFindOptimalFactorRange) {
			const uint  first_correl_idx = ifs;
			if(rSre.StakeMode == 0) {
				PROFILE_START
				so_list.clear();
				for(uint i = 0; i <= rSre.LastResultIdx; i++) {
					if(i >= (first_correl_idx+1)) {
						const double local_result = result_list.at(i);
						if(local_result != 0.0) { // @v10.3.9
							const double trend_err = rTe.ErrL.at(i); // @v10.3.12
							if(trend_err_limit <= 0.0 || (trend_err > 0.0 && trend_err <= trend_err_limit)) { // @v10.3.12
								StrategyOptEntry entry(rTe.TL.at(i), local_result);
								THROW_SL(so_list.insert(&entry));
							}
						}
					}
				}
				so_list.sort(PTR_CMPFUNC(double));
				// @v10.3.12 Все-таки снова попробуем разбивать интервал только в соответствии со знаком тренда (initial_splitting 0-->(3:2))
				//const int initial_splitting = (rSre.BaseFlags & rSre.bfShort) ? 3 : 2;
				const int initial_splitting = 4; // @20190402
				//const int initial_splitting = 1;
				//const int initial_splitting = 0;
				FindOptimalFactorRange(so_list, initial_splitting, rSre.OptDeltaRange, rSre.OptDeltaCount, rSre.OptDeltaResult);
				PROFILE_END
			}
			if(oneof2(rSre.StakeMode, 0, 1) && doFindOptimalFactorRange == 2) {
				THROW(FindOptimalFactorRange_SecondKind(/*rTrendList*/rTe, result_list, rSre));
			}
		}
	}
	CATCHZOK
	return ok;
}

static SString & FASTCALL _TsFineOptFactorMakeFinishEvntName(const char * pUniq, SString & rBuf)
{
    size_t len = sstrlen(pUniq);
    uint32 hash = BobJencHash(pUniq, len);
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
	TSVector <FactorToResultRelation> & rSet, FactorToResultRelation & rResult)
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
			if((the_first_call && what == 0) && mdr_list.getCount() > 1) {
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
				int   rr = FindOptimalFactor(rTmList, rValList, rS, what, local_md_range, local_md_step, rSet, rResult); // @recursion
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

SLAPI PPObjTimeSeries::Strategy::Strategy() : InputFrameSize(0), Prec(0), TargetQuant(0), MaxDuckQuant(0), OptDelta2Stride(0), StakeMode(0),
	BaseFlags(0), Margin(0.0), SpikeQuant(0.0), StakeThreshold(0.0), SpreadAvg(0.0), OptDeltaCount(0), OptDelta2Count(0), StakeCount(0), WinCount(0),
	StakeCloseMode(0), PeakAvgQuant(0), BottomAvgQuant(0), PeakMaxQuant(0), ID(0), TrendErrAvg(0.0), TrendErrLim(0.0)
{
	OptDeltaRange.SetVal(0.0);
	OptDelta2Range.SetVal(0.0);
	memzero(Reserve, sizeof(Reserve));
}

void SLAPI PPObjTimeSeries::Strategy::Reset()
{
	SpikeQuant = 0.0;
	MaxDuckQuant = 0;
}

SLAPI PPObjTimeSeries::StrategyContainer::StrategyContainer() : TSVector <Strategy>(), Ver(2), StorageTm(ZERODATETIME), LastValTm(ZERODATETIME)
{
}

SLAPI PPObjTimeSeries::StrategyContainer::StrategyContainer(const PPObjTimeSeries::StrategyContainer & rS) : TSVector <Strategy>(rS), 
	Ver(rS.Ver), StorageTm(rS.StorageTm), LastValTm(rS.LastValTm)
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

int SLAPI PPObjTimeSeries::StrategyContainer::GetBestSubset(long flags, uint maxCount, StrategyContainer & rScDest) const
{
	rScDest.clear();
	int    ok = 1;
	RAssocArray range_list;
	const uint _c = getCount();
	for(uint i = 0; i < _c; i++) {
		const Strategy & r_item = at(i);
		const double result = r_item.V.GetResultPerDay();
		if(result > 0.0) {
			double crit = (flags & gbsfCritProfitMultProb) ? (result * r_item.GetWinCountRate()) : result;
			if(((r_item.BaseFlags & r_item.bfShort) && (flags & gbsfShort)) || (!(r_item.BaseFlags & r_item.bfShort) && (flags & gbsfLong))) {
				if((r_item.StakeMode == 1 && flags & gbsfStakeMode1) || (r_item.StakeMode == 2 && flags & gbsfStakeMode2) || 
					(r_item.StakeMode == 3 && flags & gbsfStakeMode3) || (r_item.StakeMode == 4 && flags & gbsfStakeMode4)) {
					THROW_SL(range_list.Add(i+1, crit));
				}
			}
		}
	}
	range_list.SortByValRev();
	for(uint ridx = 0; ridx < range_list.getCount() && rScDest.getCount() < maxCount; ridx++) {
		const RAssoc & r_range_item = range_list.at(ridx);
		const uint pos = static_cast<uint>(r_range_item.Key-1);
		assert(pos < _c);
		THROW_SL(rScDest.insert(&at(pos)));
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

int SLAPI PPObjTimeSeries::StrategyContainer::GetInputFramSizeList(LongArray & rList, uint * pMaxOptDelta2Stride) const
{
	int    ok = -1;
	uint   max_opt_delta2_stride = 0;
	rList.clear();
	for(uint i = 0; i < getCount(); i++) {
		const Strategy & r_item = at(i);
		rList.addnz(static_cast<long>(r_item.InputFrameSize));
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

int SLAPI PPObjTimeSeries::PutStrategies(PPID id, StrategyContainer * pL, int use_ta)
{
	int    ok = 1;
	PPTimeSeries ts_rec;
	SBuffer buffer;
	const uint _c = SVectorBase::GetCount(pL);
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(Search(id, &ts_rec) > 0);
		if(_c) {
			SSerializeContext sctx;
			THROW(pL->Serialize(+1, buffer, &sctx));
		}
		THROW(ref->PutPropSBuffer(Obj, id, TIMSERPRP_STAKEMODEL, buffer, 0));
		DS.LogAction(PPACN_TSSTRATEGYUPD, Obj, id, static_cast<long>(_c), 0); // @v10.3.3
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjTimeSeries::GetStrategies(PPID id, StrategyContainer & rL)
{
	int    ok = -1;
	SBuffer buffer;
	rL.clear();
	THROW(ref->GetPropSBuffer(Obj, id, TIMSERPRP_STAKEMODEL, buffer));
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

SLAPI PPObjTimeSeries::TrainNnParam::TrainNnParam(const PPTimeSeries & rTsRec, long flags) : Strategy(), Symb(rTsRec.Symb), 
	ActionFlags(flags), ForwardFrameSize(0), HiddenLayerDim(0), EpochCount(1), LearningRate(0.1f)
{
	Prec = rTsRec.Prec;
	Margin = rTsRec.BuyMarg;
	SpreadAvg = rTsRec.AvgSpread;
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
	public:
		PrcssrTssaDialog() : TDialog(DLG_TSSA)
		{
		}
		int    setDTS(const PrcssrTsStrategyAnalyzeFilt * pData)
		{
			int    ok = 1;
			Data = *pData;
			SetupPPObjCombo(this, CTLSEL_TSSA_TS, PPOBJ_TIMESERIES, Data.TsList.GetSingle(), OLW_LOADDEFONOPEN, 0);
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
		int    getDTS(PrcssrTsStrategyAnalyzeFilt * pData)
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
			if(TVCOMMAND && TVCMD == cmTsList) {
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
		PrcssrTsStrategyAnalyzeFilt Data;
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
	const long trange_fmt = MKSFMTD(0, 10, NMBF_NOTRAILZ);
	rBuf.Cat("Strategy").CatChar('(').CatLongZ(rS.ID, 3).CatChar(')').CatDiv(':', 2).Cat((rS.BaseFlags & rS.bfShort) ? "S" : "B").CatChar('/');
	rBuf.CatLongZ(rS.InputFrameSize, 3).CatChar('/').Cat(rS.MaxDuckQuant).CatChar(':').Cat(rS.TargetQuant).CatChar('/').Cat(rS.StakeMode);
	rBuf.Space().Cat("Potential").CatDiv(':', 2).
		Cat(rS.V.GetResultPerDay(), MKSFMTD(0, 4, 0)).CatChar('/').
		CatChar('W').Cat(rS.GetWinCountRate(), MKSFMTD(0, 3, 0)).CatChar('/').
		CatChar('#').Cat(rS.StakeCount).CatChar('/').
		CatChar('T').Cat(rS.StakeCount ? (rS.V.TmSec / rS.StakeCount) : 0);
	if(oneof4(rS.StakeMode, 1, 2, 3, 4)) {
		rBuf.Space().CatChar('[');
		if(pBestResult) {
			if(rS.StakeMode == 1)
				rBuf.Cat(pBestResult->TvForMaxResult, trange_fmt).CatChar('|').Cat(rS.OptDeltaRange, trange_fmt);
			else if(rS.StakeMode == 2)
				rBuf.Cat(pBestResult->TvForMaxResult, trange_fmt).CatChar('|').Cat(rS.OptDelta2Stride).CatChar('/').Cat(rS.OptDelta2Range, trange_fmt);
			else if(oneof2(rS.StakeMode, 3, 4)) {
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
			else if(oneof2(rS.StakeMode, 3, 4)) {
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
		Cat("total-tm-count").Space().
		Cat("total-tm-sec").Space().
		Cat("result-per-day").Space().
		Cat("peak-avg-quant:peak-max-quant:bottom-avg-quant").Space().
		Cat("opt-delta-freq").Space().
		Cat("opt-delta-result").Space().
		Cat("opt-delta2-freq").Space().
		Cat("opt-delta2-result").Space();
	}
	else {
		rBuf.Cat(pSre->Symb);
		if(pSre->BaseFlags & pSre->bfShort)
			rBuf.CatChar('-').Cat("REV");
		if(pSre->StakeMode)
			rBuf.CatChar('/').Cat(pSre->StakeMode);
		else
			rBuf.Space().Space();
		rBuf.Space().
			Cat(pSre->ID).Space().
			Cat(pSre->SpikeQuant, MKSFMTD(15, 8, 0)).Space().
			Cat(pSre->MaxDuckQuant).CatChar(':').Cat(pSre->TargetQuant).Space().
			Cat(pSre->InputFrameSize).Space().
			Cat(pSre->V.Result, MKSFMTD(15, 5, 0)).Space().
			Cat(pSre->StakeCount).Space().
			Cat(pSre->GetWinCountRate(), MKSFMTD(15, 5, 0)).Space().
			Cat(pSre->V.TmCount).Space().
			Cat(pSre->V.TmSec).Space().
			Cat(pSre->V.GetResultPerDay()).Space().
			Cat(pSre->PeakAvgQuant).CatChar(':').Cat(pSre->PeakMaxQuant).CatChar(':').Cat(pSre->BottomAvgQuant).Space();
		{
			if(pSre->StakeMode != 2) {
				rBuf.Cat(pSre->OptDeltaRange, MKSFMTD(0, 12, 0)).
				Cat(pSre->OptDeltaCount).CatChar('/').Cat(pSre->LastResultIdx-pSre->InputFrameSize+1);
			}
			else
				rBuf.Cat("---");
			rBuf.Space();
			if(pSre->StakeMode == 0)
				rBuf.Cat(pSre->OptDeltaResult, MKSFMTD(15, 5, 0));
			else
				rBuf.Cat("---");
			rBuf.Space();
			if(pSre->StakeMode != 1) {
				rBuf.Cat(pSre->OptDelta2Range, MKSFMTD(0, 12, 0)).Space().
				Cat(pSre->OptDelta2Stride).CatChar('/').Cat(pSre->OptDelta2Count).CatChar('/').Cat(pSre->LastResultIdx-pSre->InputFrameSize+1);
			}
			else
				rBuf.Cat("---");
			rBuf.Space();
			if(pSre->StakeMode == 0)
				rBuf.Cat(pSre->OptDelta2Result, MKSFMTD(15, 5, 0));
			else
				rBuf.Cat("---");
			rBuf.Space();
		}
	}
	return rBuf;
}

static int TuneStrategyTP(const DateTimeArray & rTmList, const RealArray & rValList, const TSCollection <PPObjTimeSeries::TrendEntry> & rTrendListSet, 
	uint leftShift, uint rightShift, PPObjTimeSeries::Strategy & rS, SFile * pFOut)
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
			THROW(TsTestStrategy2(rTmList, rValList, /*p_trend_entry->TL*/*p_trend_entry, rS, 0, sre_test));
			assert(tq != org_target_quant || feqeps(sre_test.V.GetResultPerDay(), org_result, 1E-7));
			if(best_result < sre_test.V.GetResultPerDay()) {
				best_result = sre_test.V.GetResultPerDay();
				best_tq = tq;
				best_sre = sre_test;
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

SLAPI PrcssrTsStrategyAnalyze::ModelParam::ModelParam() : InitTrendErrLimit(0.0), 
	BestSubsetDimention(0), BestSubsetMaxPhonyIters(0), BestSubsetOptChunk(0), UseDataSince(ZERODATE)
{
}

PrcssrTsStrategyAnalyze::ModelParam & SLAPI PrcssrTsStrategyAnalyze::ModelParam::Z()
{
	InitTrendErrLimit = 0.0;
	BestSubsetDimention = 0;
	BestSubsetMaxPhonyIters = 0;
	BestSubsetOptChunk = 0;
	MaxDuckQuantList.clear();
	InputFrameSizeList.clear();
	UseDataSince = ZERODATE;
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
	if(rMp.InitTrendErrLimit <= 0.0 || rMp.InitTrendErrLimit > 10.0)
		rMp.InitTrendErrLimit = 1.0;
	if(ini_file.Get(PPINISECT_TSSTAKE, PPINIPARAM_TSSTAKE_BESTSUBSETDIMESION, temp_buf) > 0)
		rMp.BestSubsetDimention = static_cast<uint>(temp_buf.ToLong());
	if(rMp.BestSubsetDimention == 0 || rMp.BestSubsetDimention > 300)
		rMp.BestSubsetDimention = 50;
	if(ini_file.Get(PPINISECT_TSSTAKE, PPINIPARAM_TSSTAKE_BESTSUBSETMAXPHONY, temp_buf) > 0)
		rMp.BestSubsetMaxPhonyIters = static_cast<uint>(temp_buf.ToLong());
	if(rMp.BestSubsetMaxPhonyIters == 0 || rMp.BestSubsetMaxPhonyIters > 1000)
		rMp.BestSubsetMaxPhonyIters = 7;
	if(ini_file.Get(PPINISECT_TSSTAKE, PPINIPARAM_TSSTAKE_BESTSUBSETOPTCHUNK, temp_buf) > 0)
		rMp.BestSubsetOptChunk = static_cast<uint>(temp_buf.ToLong());
	if(!oneof2(rMp.BestSubsetOptChunk, 3, 7))
		rMp.BestSubsetOptChunk = 3;
	// @v10.4.1 {
	if(ini_file.Get(PPINISECT_TSSTAKE, PPINIPARAM_TSSTAKE_USEDATASINCE, temp_buf) > 0)
		strtodate(temp_buf.Strip(), DATF_DMY, &rMp.UseDataSince);
	if(!checkdate(rMp.UseDataSince))
		rMp.UseDataSince = ZERODATE;
	// } @v10.4.1
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
		rMp.MaxDuckQuantList.add(28);
		rMp.MaxDuckQuantList.add(30);
		rMp.MaxDuckQuantList.add(32);
		rMp.MaxDuckQuantList.add(34);
		rMp.MaxDuckQuantList.add(36);
		rMp.MaxDuckQuantList.add(38);
		rMp.MaxDuckQuantList.add(40);
		rMp.MaxDuckQuantList.add(42);
	}
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

int SLAPI PrcssrTsStrategyAnalyze::Run()
{
	const int force_fixed_maxduck_values = 1;
	//const double initial_trend_err_limit = 1.0;
	int    ok = 1;
	ModelParam model_param;
	PPObjTimeSeries ts_obj;
	SString temp_buf;
	SString symb_buf;
	SString save_file_name;
	SString msg_buf;
	PPIDArray id_pre_list;
	RealArray temp_real_list;
	PPTimeSeries ts_rec;
	TSVector <PPObjTimeSeries::QuoteReqEntry> quote_req_list;
	if(P.TsList.GetCount()) {
		P.TsList.Get(id_pre_list);
	}
	else {
		for(SEnum en = ts_obj.Enum(0); en.Next(&ts_rec) > 0;)
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
				if(ts_obj.Search(id, &ts_rec) > 0) {
					PPObjTimeSeries::StrategyContainer sc;
					TempEntry new_entry;
					new_entry.ID = id;
					if(ts_obj.GetStrategies(id, sc) > 0)
						new_entry.StorageDtm = sc.GetStorageTm();
					else
						new_entry.StorageDtm = ZERODATETIME;
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
		TSCollection <PPObjTimeSeries::TrendEntry> trend_list_set;
		SString out_file_name;
		PPGetFilePath(PPPATH_OUT, "AnalyzeTsStrategy2.txt", out_file_name);
		const int is_out_file_exists = fileExists(out_file_name);
		SFile f_out(out_file_name, SFile::mAppend);
		f_out.WriteLine(msg_buf.Z().CatCharN('-', 20).CR());
		{
			msg_buf.Z().Cat(getcurdatetime_(), DATF_ISO8601|DATF_CENTURY, 0);
			msg_buf.CR();
			msg_buf.CatEq("init_trend_err_limit", model_param.InitTrendErrLimit, MKSFMTD(0, 3, 0));
			msg_buf.CR();
			{
				msg_buf.Cat("input_frame_size").CatChar('=');
				for(uint ii = 0; ii < model_param.InputFrameSizeList.getCount(); ii++) {
					if(ii)
						msg_buf.Comma();
					msg_buf.Cat(model_param.InputFrameSizeList.get(ii));
				}
			}
			msg_buf.CR();
			{
				msg_buf.Cat("max_duck_quant").CatChar('=');
				for(uint ii = 0; ii < model_param.MaxDuckQuantList.getCount(); ii++) {
					if(ii)
						msg_buf.Comma();
					msg_buf.Cat(model_param.MaxDuckQuantList.get(ii));
				}
			}
			msg_buf.CR();
			msg_buf.CatEq("best_subset_dimension", model_param.BestSubsetDimention);
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
				if(ts_obj.Search(id, &ts_rec) > 0) {
					PPObjTimeSeries::StrategyContainer sc;
					LDATETIME strg_dtm = ZERODATETIME;
					LDATETIME lastval_dtm = ZERODATETIME;
					uint   sver = 0;
					if(ts_obj.GetStrategies(id, sc) > 0) {
						strg_dtm = sc.GetStorageTm();
						lastval_dtm = sc.GetLastValTm();
						sver = sc.GetVersion();
					}
					msg_buf.Z().Cat(ts_rec.Symb).Space().Cat(sver).Space().Cat(strg_dtm, DATF_ISO8601|DATF_CENTURY, 0).Space().
						Cat(lastval_dtm, DATF_ISO8601|DATF_CENTURY);
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
			if(ts_obj.Search(id, &ts_rec) > 0) {
				STimeSeries ts;
				int gtsr = ts_obj.GetTimeSeries(id, ts);
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
				}
				if(/*ts_obj.GetTimeSeries(id, ts) > 0*/gtsr > 0) {
					const uint tsc = ts.GetCount();
					STimeSeries::Stat st(0);
					uint   vec_idx = 0;
					THROW_SL(ts.GetValueVecIndex("close", &vec_idx));
					ts.Analyze("close", st);
					if(ts_rec.SpikeQuant <= 0.0 || (P.Flags & P.fForce)) {
						ts_rec.SpikeQuant = st.DeltaAvg / 2.0;
						PPID   temp_id = id;
						THROW(ts_obj.PutPacket(&temp_id, &ts_rec, 1));
					}
					{
						const  double spike_quant = ts_rec.SpikeQuant;
						uint32 last_strategy_id = 0;
						DateTimeArray ts_tm_list;
						RealArray ts_val_list;
						PPObjTimeSeries::StrategyContainer scontainer;
						THROW_SL(ts.GetRealArray(vec_idx, 0, tsc, ts_val_list));
						THROW_SL(ts.GetTimeArray(0, tsc, ts_tm_list));
						assert(ts_tm_list.getCount() == tsc);
						assert(ts_val_list.getCount() == tsc);
						const LDATETIME last_val_dtm = ts_tm_list.get(tsc-1);
						scontainer.SetLastValTm(last_val_dtm);
						if(P.Flags & P.fFindStrategies) {
							trend_list_set.freeAll();
							for(uint ifsidx = 0; ifsidx < model_param.InputFrameSizeList.getCount(); ifsidx++) {
								const uint input_frame_size = static_cast<uint>(model_param.InputFrameSizeList.get(ifsidx));
								assert(PPObjTimeSeries::SearchTrendEntry(trend_list_set, input_frame_size) == 0);
								PPObjTimeSeries::TrendEntry * p_new_trend_entry = new PPObjTimeSeries::TrendEntry(input_frame_size, tsc);
								THROW_SL(p_new_trend_entry);
								PROFILE_START
								{
									StatBase trls(0);
									STimeSeries::AnalyzeFitParam afp(input_frame_size, 0, 0);
									THROW(ts.AnalyzeFit("close", afp, &p_new_trend_entry->TL, 0, &temp_real_list, 0, 0));
									assert(p_new_trend_entry->TL.getCount() == tsc);
									assert(temp_real_list.getCount() == tsc);
									for(uint trlidx = 0; trlidx < temp_real_list.getCount(); trlidx++) {
										double err = temp_real_list.at(trlidx);
										assert(err >= 0.0);
										if(err > 0.0) {
											double sqr_err = sqrt(err);
											trls.Step(sqr_err);
											temp_real_list.at(trlidx) = sqr_err;
										}
									}
									trls.Finish();
									p_new_trend_entry->ErrAvg = trls.GetExp();
									p_new_trend_entry->ErrL = temp_real_list;
								}
								PROFILE_END
								trend_list_set.insert(p_new_trend_entry);
							}
						}
						//
						// stake_side: 0 - long, 1 - short
						// 
						for(int stake_side = 0; stake_side < 2; stake_side++) {
							if((stake_side == 0 && P.Flags & P.fProcessLong) || (stake_side == 1 && P.Flags & P.fProcessShort)) {
								const bool is_short = (stake_side == 1);
								const uint32 org_opt_max_duck_val = is_short ? ts_rec.OptMaxDuck_S : ts_rec.OptMaxDuck;
								uint32 cur_opt_max_duck_val = org_opt_max_duck_val;
								if(!force_fixed_maxduck_values) {
									if(P.Flags & P.fFindOptMaxDuck && (org_opt_max_duck_val <= 0 || (P.Flags & P.fForce))) {
										PPObjTimeSeries::TrainNnParam tnnp(ts_rec, PPObjTimeSeries::TrainNnParam::afAnalyzeFrame);
										tnnp.SpikeQuant = spike_quant;
										tnnp.EpochCount = 1;
										tnnp.InputFrameSize = 0;
										tnnp.MaxDuckQuant = 0;
										tnnp.StakeThreshold = 0.05;
										assert(oneof2(P.CloseMode, tnnp.clsmodFullMaxDuck, tnnp.clsmodAdjustLoss));
										tnnp.StakeCloseMode = static_cast<uint16>(P.CloseMode);
										SETFLAG(tnnp.BaseFlags, tnnp.bfShort, is_short);
										{
											PPObjTimeSeries::FactorToResultRelation opt_max_duck;
											PPObjTimeSeries::FactorToResultRelation opt_peak;
											{
												IntRange md_range;
												md_range.Set(50, 500);
												TSVector <PPObjTimeSeries::FactorToResultRelation> opt_max_duck_set;
												THROW(ts_obj.FindOptimalFactor(ts_tm_list, ts_val_list, tnnp, 0/*what*/, md_range, 50, opt_max_duck_set, opt_max_duck));
												cur_opt_max_duck_val = opt_max_duck.FactorQuant;
											}
										}
										{
											PPID   temp_id = id;
											PPTimeSeries ts_rec_to_upd;
											THROW(ts_obj.GetPacket(temp_id, &ts_rec_to_upd) > 0);
											if(is_short)
												ts_rec_to_upd.OptMaxDuck_S = cur_opt_max_duck_val;
											else
												ts_rec_to_upd.OptMaxDuck = cur_opt_max_duck_val;
											THROW(ts_obj.PutPacket(&temp_id, &ts_rec_to_upd, 1));
										}
									}
								}
								if(P.Flags & P.fFindStrategies) {
									LAssocArray opt_peak_by_maxduck_list; // Кэш соответствий maxduck->opt_peak дабы не пересчитывать снова при каждом значении InputFrameSize
									for(uint mdidx = 0; mdidx < model_param.MaxDuckQuantList.getCount(); mdidx++) {
										PPObjTimeSeries::TrainNnParam tnnp(ts_rec, PPObjTimeSeries::TrainNnParam::afAnalyzeFrame);
										symb_buf = ts_rec.Symb;
										if(stake_side == 1)
											symb_buf.CatChar('-').Cat("REV");
										tnnp.SpikeQuant = spike_quant;
										tnnp.EpochCount = 1;
										tnnp.InputFrameSize = 0; // input_frame_size;
										tnnp.MaxDuckQuant = static_cast<uint16>(model_param.MaxDuckQuantList.get(mdidx));
										tnnp.StakeThreshold = 0.05;
										assert(oneof2(P.CloseMode, tnnp.clsmodFullMaxDuck, tnnp.clsmodAdjustLoss));
										tnnp.StakeCloseMode = static_cast<uint16>(P.CloseMode);
										SETFLAG(tnnp.BaseFlags, tnnp.bfShort, stake_side == 1);
										{
											long   cv = 0;
											uint   local_target_quant = 0;
											if(opt_peak_by_maxduck_list.Search(tnnp.MaxDuckQuant, &cv, 0) && cv > 0) {
												local_target_quant = static_cast<uint>(cv);
											}
											else {
												// Рассчитываем средний максимум пиков в квантах (local_result не нужен)
												double local_result = 0.0;
												uint   local_peak_quant = 0;
												THROW(Ts_Helper_FindOptimalFactor(ts_tm_list, ts_val_list, tnnp, local_result, local_peak_quant));
												if(local_peak_quant) {
													IntRange peak_range;
													TSVector <PPObjTimeSeries::FactorToResultRelation> opt_peak_set;
													PPObjTimeSeries::FactorToResultRelation opt_peak;
													peak_range.upp = 3 * local_peak_quant / 2;
													const int peak_step = peak_range.upp / 5;
													peak_range.low = peak_step;
													int rr = ts_obj.FindOptimalFactor(ts_tm_list, ts_val_list, tnnp, 1/*what*/, peak_range, peak_step, opt_peak_set, opt_peak);
													THROW(rr);
													if(rr > 0) {
														local_target_quant = opt_peak.FactorQuant;
														opt_peak_by_maxduck_list.Add(tnnp.MaxDuckQuant, local_target_quant);
													}
												}
											}
											if(local_target_quant > 0)
												tnnp.TargetQuant = local_target_quant;
										}
										const uint org_target_quant = tnnp.TargetQuant;
										for(uint ifsidx = 0; ifsidx < model_param.InputFrameSizeList.getCount(); ifsidx++) {
											const uint input_frame_size = static_cast<uint>(model_param.InputFrameSizeList.get(ifsidx));
											const PPObjTimeSeries::TrendEntry * p_trend_entry = PPObjTimeSeries::SearchTrendEntry(trend_list_set, input_frame_size);
											assert(p_trend_entry);
											assert(p_trend_entry->Stride == input_frame_size); // @paranoic
											tnnp.InputFrameSize = input_frame_size;
											tnnp.TrendErrAvg = p_trend_entry->ErrAvg;   // @v10.3.12
											tnnp.TrendErrLim = model_param.InitTrendErrLimit; // @v10.3.12
											//
											uint prev_target = tnnp.TargetQuant;
											for(int do_repeat_on_target_factor = 1, tfiter_no = 0; do_repeat_on_target_factor; tfiter_no++) {
												do_repeat_on_target_factor = 0;
												tnnp.TargetQuant = prev_target;
												int  do_optimal_factor_range = 1;
												PPObjTimeSeries::StrategyResultEntry sre(tnnp, 0);
												THROW(TsTestStrategy2(ts_tm_list, ts_val_list, *p_trend_entry, tnnp, 1/*do_optimal_factor_range*/, sre));
												STRNSCPY(sre.Symb, ts_rec.Symb);
												f_out.WriteLine(PPObjTimeSeries::StrategyOutput(&sre, msg_buf).CR());
												f_out.Flush();
												{
													tnnp.OptDeltaRange = sre.OptDeltaRange;
													tnnp.OptDeltaCount = sre.OptDeltaCount;
													tnnp.OptDelta2Range =  sre.OptDelta2Range;
													tnnp.OptDelta2Stride = sre.OptDelta2Stride;
													tnnp.OptDelta2Count = sre.OptDelta2Count;
													tnnp.PeakAvgQuant = sre.PeakAvgQuant;
													tnnp.BottomAvgQuant = sre.BottomAvgQuant;
													tnnp.PeakMaxQuant = sre.PeakMaxQuant;
												}
												{
													PPObjTimeSeries::StrategyResultEntry sre_test(tnnp, 1/*stake_mode*/);
													THROW(TsTestStrategy2(ts_tm_list, ts_val_list, *p_trend_entry, tnnp, 1/*do_optimal_factor_range*/, sre_test));
													sre_test.ID = ++last_strategy_id;
													f_out.WriteLine(PPObjTimeSeries::StrategyOutput(&sre_test, msg_buf).CR());
													if(sre_test.StakeCount > 0 && sre_test.WinCount > 0) {
														const uint tstp_lo_limit = 18; // @20190416 12-->18
														uint tstp_left_shift  = 2 * sre_test.TargetQuant / 3; // Количество квантов вниз от текущего оптимума
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
														THROW(TuneStrategyTP(ts_tm_list, ts_val_list, trend_list_set, tstp_left_shift, tstp_right_shift, sre_test, &f_out));
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
									}
								}
							}
						}
						if(P.Flags & P.fFindStrategies) {
							double best_sc_result = 0.0;
							uint   best_sc_idx = 0;
							double prev_result = 0.0;
							long   ssflags = PPObjTimeSeries::StrategyContainer::gbsfLong|PPObjTimeSeries::StrategyContainer::gbsfShort|
								/*PPObjTimeSeries::StrategyContainer::gbsfStakeMode4|*/
								PPObjTimeSeries::StrategyContainer::gbsfStakeMode3|
								PPObjTimeSeries::StrategyContainer::gbsfStakeMode2|
								PPObjTimeSeries::StrategyContainer::gbsfStakeMode1;
							PPObjTimeSeries::StrategyContainer sc_selection;
							if(1) {
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
								assert(oneof2(model_param.BestSubsetOptChunk, 3, 7));
								//
								f_out.WriteLine(msg_buf.Z().CR());
								f_out.WriteLine((msg_buf = "--- TsSimulateStrategyContainer").Space().Cat(ts_rec.Symb).CR());
								//
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
										TsSimulateStrategyContainer(B.R_TmList, B.R_ValList, B.R_TrendList, B.R_Sc, *B.P_Result);
									}
								private:
									virtual void SLAPI Startup()
									{
										SlThread::Startup();
										SignalStartup();
									}
									InitBlock B;
								};
								PPObjTimeSeries::StrategyContainer sc_process; 
								ssflags |= scontainer.gbsfCritProfitMultProb; // @20190413
								if(scontainer.GetBestSubset(ssflags, model_param.BestSubsetDimention, sc_process) > 0) {
									double best_result = 0.0;
									uint   phony_iter_no = 0;
									const  uint scp_inc = (model_param.BestSubsetOptChunk == 3) ? 2 : ((model_param.BestSubsetOptChunk == 7) ? 3 : 0);
									for(uint scpidx = 0; scpidx < sc_process.getCount() && phony_iter_no < model_param.BestSubsetMaxPhonyIters; scpidx += scp_inc) {
										PPObjTimeSeries::StrategyResultEntry sre[7];
										PPObjTimeSeries::StrategyContainer sc[7]; // 7 - max of model_param.BestSubsetOptChunk
										HANDLE objs_to_wait[16];
										size_t objs_to_wait_count = 0;
										MEMSZERO(objs_to_wait);
										uint thr_idx;
										if(model_param.BestSubsetOptChunk == 7) {
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
											msg_buf.Z().
												CatEq("Result", sre[best_thr_idx].V.Result, MKSFMTD(15, 5, 0)).Tab().
												CatEq("StakeCount", sre[best_thr_idx].StakeCount).Tab().
												CatEq("WinCountRate", sre[best_thr_idx].GetWinCountRate(), MKSFMTD(15, 5, 0)).Tab().
												CatEq("TmCount", sre[best_thr_idx].V.TmCount).Tab().
												CatEq("TmSec", sre[best_thr_idx].V.TmSec).Tab().
												CatEq("TotalSec", sre[best_thr_idx].TotalSec).Tab().
												CatEq("ResultPerTotalDay", fdivnz(3600.0 * 24.0 * sre[best_thr_idx].V.Result, 
													static_cast<double>(sre[best_thr_idx].TotalSec)), MKSFMTD(0, 12, 0));
											f_out.WriteLine(msg_buf.CR());
											f_out.Flush();
										}
										else
											phony_iter_no++;
									}
								}
							}
							else
								sc_selection = scontainer;
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
								TsSimulateStrategyContainer(ts_tm_list, ts_val_list, trend_list_set, sc_selection, sre);
								THROW(ts_obj.PutStrategies(id, &sc_selection, 1)); // Сохранение отобранных стратегий в базе данных
								msg_buf.Z().
									CatEq("Result", sre.V.Result, MKSFMTD(15, 5, 0)).Tab().
									CatEq("StakeCount", sre.StakeCount).Tab().
									CatEq("WinCountRate", sre.GetWinCountRate(), MKSFMTD(15, 5, 0)).Tab().
									CatEq("TmCount", sre.V.TmCount).Tab().
									CatEq("TmSec", sre.V.TmSec).Tab().
									CatEq("TotalSec", sre.TotalSec).Tab().
									CatEq("ResultPerTotalDay", fdivnz(3600.0 * 24.0 * sre.V.Result, static_cast<double>(sre.TotalSec)), MKSFMTD(0, 12, 0));
								f_out.WriteLine(msg_buf.CR());
								f_out.Flush();
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
	return ok;
}
