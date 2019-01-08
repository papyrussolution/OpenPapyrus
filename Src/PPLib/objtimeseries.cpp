// OBJTIMESERIES.CPP
// Copyright (c) A.Sobolev 2018, 2019
// @codepage UTF-8
// Модуль, управляющий объектом данных PPObjTimeSeries
//
#include <pp.h>
#pragma hdrstop
#include <fann2.h>

SLAPI PPTimeSeries::PPTimeSeries() : Tag(PPOBJ_TIMESERIES), ID(0), Flags(0), BuyMarg(0.0), SellMarg(0.0), 
	SpikeQuant(0.0), Prec(0), AvgSpread(0.0), OptMaxDuck(0)
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
	else if(!sstreq(CurrencySymb, rS.CurrencySymb))
		eq = 0;
	return eq;	
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
							//
#if 0 // @experimental {
							if(tsc >= 1000) {
								RealArray frame;
								StatBase sb;
								double max_vlct_mn = 0.0;
								uint   max_vlct_mn_frame_size = 0;
								double max_vlct_mx = 0.0;
								uint   max_vlct_mx_frame_size = 0;
								for(uint frame_size = 5; frame_size <= 120; frame_size++) {
									StatBase fsb_mn;
									StatBase fsb_mx;
									for(uint fs = 0; fs < (tsc-frame_size); fs++) {
										ts.GetFrame("close", fs, frame_size, STimeSeries::nfOne|STimeSeries::nfBaseStart, frame);
										if(frame.getCount() == frame_size) {
											sb.Init(0);
											for(uint i = 0; i < frame.getCount(); i++) {
												sb.Step(frame.at(i));
											}
											sb.Finish();
											SUniTime utm_f_beg;
											SUniTime utm_f_end;
											LDATETIME dtm_beg;
											LDATETIME dtm_end;
											ts.GetTime(fs, &utm_f_beg);
											ts.GetTime(fs+frame_size-1, &utm_f_end);
											utm_f_beg.Get(dtm_beg);
											utm_f_end.Get(dtm_end);
											long sec = diffdatetimesec(dtm_end, dtm_beg) + 60;
											double _mn = sb.GetMin();
											double _mx = sb.GetMax();
											fsb_mn.Step(_mn / sec);
											fsb_mx.Step(_mx / sec);
										}
									}
									fsb_mn.Finish();
									fsb_mx.Finish();									
									if(fsb_mn.GetExp() > max_vlct_mn) {
										max_vlct_mn = fsb_mn.GetExp();
										max_vlct_mn_frame_size = frame_size;
									}
									if(fsb_mx.GetExp() > max_vlct_mx) {
										max_vlct_mx = fsb_mx.GetExp();
										max_vlct_mx_frame_size = frame_size;
									}
								}
								out_buf.CatEq("max_vlct_mx_frame_size", max_vlct_mx_frame_size).Space().CRB();
								out_buf.CatEq("max_vlct_mx", max_vlct_mx).Space().CRB();
								out_buf.CatEq("max_vlct_mn_frame_size", max_vlct_mx_frame_size).Space().CRB();
								out_buf.CatEq("max_vlct_mn", max_vlct_mx).Space().CRB();
							}
#endif // } 0 @experimental
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
		TextRefIdent tri(Obj, id, PPTRPROP_TIMESERIES);
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(Search(id, &ts_rec) > 0);
		THROW(PPRef->UtrC.SetTimeSeries(tri, pTs, 0));
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjTimeSeries::GetTimeSeries(PPID id, STimeSeries & rTs)
{
	TextRefIdent tri(Obj, id, PPTRPROP_TIMESERIES);
	return PPRef->UtrC.Search(tri, rTs);
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
			PPObjTimeSeries * p_obj = (PPObjTimeSeries *)P_Obj;
			if(p_obj) {
				//p_obj->Test();
				//p_obj->AnalyzeTsAftershocks();
				//p_obj->AnalyzeStrategies();
				StrategyContainer scontainer;
				if(p_obj->GetStrategies(id, scontainer) > 0) {
					for(uint i = 0; i < scontainer.getCount(); i++) {
						const Strategy & r_s = scontainer.at(i);
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

int SLAPI PPObjTimeSeries::AnalyzeTsAftershocks()
{
	int    ok = 1;
	PPIDArray id_list;
	PPTimeSeries ts_rec;
	for(SEnum en = Enum(0); en.Next(&ts_rec) > 0;) {
		id_list.add(ts_rec.ID);
	}
	if(id_list.getCount()) {
		id_list.sortAndUndup();
		for(uint i = 0; i < id_list.getCount(); i++) {
			const PPID id = id_list.get(i);
			if(Search(id, &ts_rec) > 0) {
				STimeSeries ts;
				if(GetTimeSeries(id, ts) > 0) {
					TrainNnParam tnnp(ts_rec, TrainNnParam::afAnalyzeFrame);
					AnalyzeAftershock(ts, tnnp);
				}
			}
		}
	}
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

static int SLAPI FindOptimalFactorRange(TSVector <StrategyOptEntry> & rList, RealRange & rR, uint32 & rRangeCount, double & rResult)
{
	int    ok = -1;
	const  uint _c = rList.getCount();
	rR.Clear();
	rRangeCount = 0;
	rResult = 0.0;
	if(_c > 1) {
		const uint maxprobe = 20;
		rList.sort(PTR_CMPFUNC(double));
		const  double total_sum = CalcFactorRangeResultSum(rList, 0, _c-1);
		double prev_result = total_sum;
		uint   lo_idx = 0;
		uint   up_idx = _c-1;
		uint   iter_no = 0;
		while(lo_idx < up_idx) {
			uint   mid_idx = 0;
			//
			// При первом делении списка на 2 части режем по нулевой точке
			//
			if(iter_no == 0 && rList.at(0).Factor < 0.0 && rList.at(_c-1).Factor > 0.0) {
				for(uint i = 0; i < _c; i++) {
					const StrategyOptEntry & r_entry = rList.at(i);
					if(r_entry.Factor > 0.0) {
						mid_idx = i-1;
						break;
					}
				}
			}
			else 
				mid_idx = (lo_idx + up_idx) / 2;
			const double lo_result = CalcFactorRangeResultSum(rList, lo_idx, mid_idx);
			const double up_result = CalcFactorRangeResultSum(rList, mid_idx+1, up_idx);
			if(MAX(lo_result, up_result) < prev_result) {
				break;
			}
			else if(lo_result > up_result) {
				prev_result = lo_result;
				up_idx = mid_idx;
			}
			else {
				prev_result = up_result;
				lo_idx = mid_idx+1;
			}
			iter_no++;
		}
		while(lo_idx > 0) {
			int done = 0;
			for(uint probedeep = 1; !done && probedeep <= maxprobe && probedeep > lo_idx; probedeep++) {
				double temp_result = CalcFactorRangeResultSum(rList, lo_idx-probedeep, up_idx);
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
				double temp_result = CalcFactorRangeResultSum(rList, lo_idx, up_idx+probedeep);
				if(prev_result < temp_result) {
					prev_result = temp_result;
					up_idx += probedeep;
					done = 1;
				}
			}
			if(!done)
				break;
		}
		while(lo_idx < up_idx) {
			int done = 0;
			for(uint probedeep = 1; !done && probedeep <= maxprobe && (lo_idx+probedeep) <= up_idx; probedeep++) {
				double temp_result = CalcFactorRangeResultSum(rList, lo_idx+probedeep, up_idx);
				if(prev_result < temp_result) {
					prev_result = temp_result;
					lo_idx += probedeep;
					done = 1;
				}
			}
			if(!done)
				break;
		}
		while(lo_idx < up_idx) {
			int done = 0;
			for(uint probedeep = 1; !done && probedeep <= maxprobe && (up_idx-probedeep) >= lo_idx; probedeep++) {
				double temp_result = CalcFactorRangeResultSum(rList, lo_idx, up_idx-probedeep);
				if(prev_result < temp_result) {
					prev_result = temp_result;
					up_idx -= probedeep;
					done = 1;
				}
			}
			if(!done)
				break;
		}
		rR.Set(rList.at(lo_idx).Factor, rList.at(up_idx).Factor);
		rResult = prev_result;
		rRangeCount = (up_idx - lo_idx + 1);
		if(lo_idx > 0 || up_idx < (_c-1)) {
			ok = 1;
		}
	}
	return ok;
}

SLAPI PPObjTimeSeries::StrategyResultValue::StrategyResultValue() : Result(0.0), TmCount(0), TmSec(0)
{
}

void FASTCALL PPObjTimeSeries::Strategy::SetValue(const StrategyResultValue & rV)
{
	V.Result += rV.Result;
	V.TmCount += rV.TmCount;
	V.TmSec += rV.TmSec;
	StakeCount++;
	if(rV.Result > 0.0)
		WinCount++;
}

PPObjTimeSeries::StrategyResultValue & SLAPI PPObjTimeSeries::StrategyResultValue::Z()
{
	Result = 0.0;
	TmCount = 0;
	TmSec = 0;
	return *this;
}

double SLAPI PPObjTimeSeries::StrategyResultValue::GetResultPerSec() const
	{ return (Result / ((double)TmSec)); }
double SLAPI PPObjTimeSeries::StrategyResultValue::GetResultPerDay() const
	{ return ((3600.0 * 24.0 * Result) / ((double)TmSec)); }

SLAPI PPObjTimeSeries::StrategyResultEntry::StrategyResultEntry(const PPObjTimeSeries::TrainNnParam & rTnnp, int stakeMode) : Strategy()
{
	//THISZERO();
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
}

int SLAPI PPObjTimeSeries::TestStrategy(const STimeSeries & rTs, uint vecIdx, const RealArray & rTrendList, const Strategy & rS, StrategyResultEntry & rSre)
{
	int    ok = 1;
	TSVector <StrategyOptEntry> so_list;
	RealArray result_list;
	rSre.LastResultIdx = 0;
	const uint tsc = rTs.GetCount();
	assert(tsc == rTrendList.getCount());
	THROW(rTrendList.getCount() == tsc);
	PROFILE_START
	for(uint i = 0; i < tsc; i++) {
		StrategyResultValue rv;
		int    is_signal = 1;
		if(rSre.StakeMode == 1) {
			is_signal = 0;
			if(i >= (rS.InputFrameSize+1) && rS.OptDeltaRange.Check(rTrendList.at(i))) {
				const  int csr = CalcStrategyResult(rTs, rS, vecIdx, i, rv);
				THROW(csr);
				if(csr != 2)
					rSre.LastResultIdx = i;
				is_signal = BIN(csr == 2);
			}
		}
		else if(rSre.StakeMode == 2) {
			is_signal = 0;
			if(i >= (rS.InputFrameSize+rS.OptDelta2Stride)) {
				double d2 = rTrendList.StrideDifference(i, rS.OptDelta2Stride);
				if(rS.OptDelta2Range.Check(d2)) {
					const  int csr = CalcStrategyResult(rTs, rS, vecIdx, i, rv);
					THROW(csr);
					if(csr != 2)
						rSre.LastResultIdx = i;
					is_signal = BIN(csr == 2);
				}
			}
		}
		else {
			const  int csr = CalcStrategyResult(rTs, rS, vecIdx, i, rv);
			THROW(csr);
			if(csr != 2)
				rSre.LastResultIdx = i;
			is_signal = BIN(csr == 2);
		}
		if(is_signal) {
			result_list.add(rv.Result);
			rSre.SetValue(rv);
		}
		else
			result_list.add(0.0);
	}
	assert(result_list.getCount() == tsc);
	{
		const double tr = result_list.Sum();
		assert(feqeps(rSre.V.Result, tr, 1e-7));
	}
	PROFILE_END
	if(rSre.StakeMode == 0) {
		const uint  first_correl_idx = rS.InputFrameSize;
		PROFILE_START
		{
			so_list.clear();
			for(uint i = 0; i <= rSre.LastResultIdx; i++) {
				if(i >= (first_correl_idx+1)) {
					StrategyOptEntry entry(rTrendList.at(i), result_list.at(i));
					THROW_SL(so_list.insert(&entry));
				}
			}
			FindOptimalFactorRange(so_list, rSre.OptDeltaRange, rSre.OptDeltaCount, rSre.OptDeltaResult);
		}
		PROFILE_END
		PROFILE_START
		{
			double max_result = 0.0;
			for(uint stride = 1; stride <= 4; stride++) {
				so_list.clear();
				for(uint i = 0; i <= rSre.LastResultIdx; i++) {
					if(i >= (first_correl_idx+stride)) {
						StrategyOptEntry entry(rTrendList.StrideDifference(i, stride), result_list.at(i));
						THROW_SL(so_list.insert(&entry));
					}
				}
				RealRange opt_range;
				uint32 opt_count;
				double opt_result;
				FindOptimalFactorRange(so_list, opt_range, opt_count, opt_result);
				if(max_result < opt_result) {
					max_result = opt_result;
					rSre.OptDelta2Result = opt_result;
					rSre.OptDelta2Count = opt_count;
					rSre.OptDelta2Range = opt_range;
					rSre.OptDelta2Stride = stride;
				}
			}
		}
		PROFILE_END
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjTimeSeries::FindOptimalMaxDuck(const STimeSeries & rTs, uint vecIdx, const TrainNnParam & rS, 
	const IntRange & rMdRange, int mdStep, TSVector <MaxDuckToResultRelation> * pSet, MaxDuckToResultRelation & rResult)
{
	int    ok = -1;
	assert(rMdRange.low > 0 && rMdRange.upp > rMdRange.low && mdStep > 0);
	SString log_buf;
	SString temp_buf;
	SString log_file_name;
	SString symb_buf;
	rResult.MaxDuckQuant = 0;
	rResult.Result = 0.0;
	PPGetFilePath(PPPATH_LOG, "Ts-FindOptimalMaxDuck.log", log_file_name);
	TrainNnParam local_s(rS);
	const bool the_first_call = (!pSet || pSet->getCount() == 0);
	const uint tsc = rTs.GetCount();
	TSVector <MaxDuckToResultRelation> mdr_list;
	for(uint md = rMdRange.low; (int)md <= rMdRange.upp; md += mdStep) {
		int    done = 0;
		double local_result = 0.0;
		if(pSet) {
			for(uint i = 0; !done && i < pSet->getCount(); i++) {
				const MaxDuckToResultRelation & r_set_item = pSet->at(i);
				if(r_set_item.MaxDuckQuant == (int)md) {
					local_result = r_set_item.Result;
					done = 1;
				}
			}
		}
		if(!done) {
			local_s.MaxDuckQuant = md;
			StrategyResultEntry sre(local_s, 0);
			sre.LastResultIdx = 0;
			for(uint i = 0; i < tsc; i++) {
				StrategyResultValue rv;
				int    is_signal = 1;
				{
					const  int csr = CalcStrategyResult(rTs, local_s, vecIdx, i, rv);
					THROW(csr);
					if(csr != 2)
						sre.LastResultIdx = i;
					is_signal = BIN(csr == 2);
				}
				if(is_signal) {
					sre.SetValue(rv);
				}
			}
			local_result = sre.V.GetResultPerSec() * 3600 * 24;
			if(pSet) {
				MaxDuckToResultRelation new_set_entry;
				new_set_entry.MaxDuckQuant = md;
				new_set_entry.Result = local_result;
				THROW_SL(pSet->insert(&new_set_entry));
			}
		}
		{
			symb_buf = rTs.GetSymb();
			if(rS.BaseFlags & rS.bfShort)
				symb_buf.CatChar('-').Cat("REV");
			log_buf.Z().Cat("FindOptimalMaxDuck").CatDiv(':', 2).Cat(symb_buf).Space().CatEq("MaxDuckQuant", md).Space().
				CatEq("Result", local_result, MKSFMTD(15, 5, 0));
			PPLogMessage(log_file_name, log_buf, LOGMSGF_TIME);
		}
		{
			MaxDuckToResultRelation mdr_entry;
			mdr_entry.MaxDuckQuant = md;
			mdr_entry.Result = local_result;
			uint  last_mdr_pos = mdr_list.getCount();
			THROW_SL(mdr_list.insert(&mdr_entry));
			if(the_first_call && mdr_list.getCount() > 1) {
				const double prev_result = mdr_list.at(last_mdr_pos-1).Result;
				if(local_result < prev_result)
					break;
			}
		}
	}
	{
		const uint mdrc = mdr_list.getCount();
		if(mdrc > 1) {
			IntRange local_md_range;
			double max_result = -MAXDOUBLE;
			int    best_idx = -1;
			for(uint j = 0; j < mdrc; j++) {
				if(max_result < mdr_list.at(j).Result) {
					max_result = mdr_list.at(j).Result;
					best_idx = (int)j;
				}
			}
			assert(best_idx >= 0);
			const uint low_idx = (uint)((best_idx > 0) ? (best_idx-1) : best_idx);
			const uint upp_idx = (uint)(((best_idx+1) < (int)mdrc) ? (best_idx+1) : best_idx);
			local_md_range.Set(mdr_list.at(low_idx).MaxDuckQuant, mdr_list.at(upp_idx).MaxDuckQuant);
			assert(local_md_range.upp > local_md_range.low);
			THROW(local_md_range.upp > local_md_range.low);
			const  int range_divider = (local_md_range == rMdRange) ? 5 : 4;
			int    local_md_step = (local_md_range.upp - local_md_range.low) / range_divider;
			if(local_md_step > 0) {
				THROW(FindOptimalMaxDuck(rTs, vecIdx, rS, local_md_range, local_md_step, pSet, rResult)); // @recursion
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
		if(ok > 0) {
			log_buf.Z().Cat("!OptimalMaxDuck").CatDiv(':', 2).Cat(rTs.GetSymb()).Space().CatEq("MaxDuckQuant", rResult.MaxDuckQuant).Space().
				CatEq("Result", rResult.Result, MKSFMTD(15, 5, 0));
			PPLogMessage(log_file_name, log_buf, LOGMSGF_TIME);
		}
	}
	CATCHZOK
	return ok;
}

#if 0 // {
int SLAPI PPObjTimeSeries::AnalyzeStrategies()
{
	int    ok = 1;
	enum {
		processfFindOptMaxDuck = 0x0001,
		processfFindStrategies = 0x0002,
		processfForce          = 0x0004,
	};
	long   process_flags = (processfFindOptMaxDuck | processfFindStrategies | processfForce);
	Fann2 * p_ann = 0;
	SString temp_buf;
	SString symb_buf;
	SString save_file_name;
	SString msg_buf;
	//static const char * pp_symbols[] = { /*"GOOGL",*/ "eurusd" /*"amzn"*/ /*, "USDJPY", "USDCHF", "GBPUSD", "AUDUSD", "EURCAD", "GBPJPY"*/ };
	static const char * pp_symbols[] = { "" };
	PPIDArray id_list;
	PPTimeSeries ts_rec;
	TSVector <QuoteReqEntry> quote_req_list;
	LoadQuoteReqList(quote_req_list);
	for(SEnum en = Enum(0); en.Next(&ts_rec) > 0;) {
		if(pp_symbols && !isempty(pp_symbols[0])) {
			for(uint i = 0; i < SIZEOFARRAY(pp_symbols); i++) {
				if(sstreqi_ascii(ts_rec.Symb, pp_symbols[i])) {
					id_list.add(ts_rec.ID);
					break;
				}
			}
		}
		else
			id_list.add(ts_rec.ID);
	}
	if(id_list.getCount()) {
		id_list.sortAndUndup();
		RealArray trend_list;
		RealArray temp_trendinc_list;
		RealArray temp_result_list;
		TSVector <StrategyOptEntry> so_list;
		SString out_file_name;
		PPGetFilePath(PPPATH_OUT, "AnalyzeTsStrategy2.txt", out_file_name);
		const int is_out_file_exists = fileExists(out_file_name);
		SFile f_out(out_file_name, SFile::mAppend);
		if(!is_out_file_exists) {
			msg_buf.Z().Cat(getcurdatetime_(), DATF_YMD|DATF_CENTURY, TIMF_HMS);
			f_out.WriteLine(msg_buf.CR());
			msg_buf.Z().
				Cat("symbol").Tab().
				Cat("spike-quant").Tab().
				Cat("max-duck").Tab().
				Cat("input-frame-size").Tab().
				Cat("result").Tab().
				Cat("stake-count").Tab().
				Cat("win-count-rel").Tab().
				Cat("total-tm-count").Tab().
				Cat("total-tm-sec").Tab().
				Cat("result-per-day").Tab().
				Cat("opt-delta-freq").Tab().
				Cat("opt-delta-result").Tab().
				Cat("opt-delta2-freq").Tab().
				Cat("opt-delta2-result").Tab();
			f_out.WriteLine(msg_buf.CR());
		}
		for(uint i = 0; i < id_list.getCount(); i++) {
			const PPID id = id_list.get(i);
			if(Search(id, &ts_rec) > 0) {
				STimeSeries ts;
				if(GetTimeSeries(id, ts) > 0) {
					const uint tsc = ts.GetCount();
					static const uint input_frame_size_list[] = { /*100,*/ 500, 1000, 1500, 2000, 2500 };
					STimeSeries::Stat st(0);
					MaxDuckToResultRelation opt_max_duck;
					const QuoteReqEntry * p_req_entry = 0;
					for(uint qreidx = 0; !p_req_entry && qreidx < quote_req_list.getCount(); qreidx++) {
						const QuoteReqEntry & r_entry = quote_req_list.at(qreidx);
						if(sstreqi_ascii(r_entry.Ticker, ts.GetSymb()))
							p_req_entry = &r_entry;
					}
					const  int is_short_permitted = p_req_entry ? BIN(p_req_entry->Flags & p_req_entry->fAllowShort) : 0;
					assert(oneof2(is_short_permitted, 0, 1));
					uint   vec_idx = 0;
					THROW_SL(ts.GetValueVecIndex("close", &vec_idx));
					ts.Analyze("close", st);
					if(ts_rec.SpikeQuant <= 0.0 || (process_flags & processfForce)) {
						ts_rec.SpikeQuant = st.DeltaAvg / 2.0;
						PPID   temp_id = id;
						THROW(PutPacket(&temp_id, &ts_rec, 1));
					}
					const double spike_quant = ts_rec.SpikeQuant;
					LDATETIME last_val_dtm = ZERODATETIME;
					{
						SUniTime utm;
						ts.GetTime(tsc-1, &utm);
						utm.Get(last_val_dtm);
					}
					//
					// stake_side: 0 - long, 1 - short
					// 
					for(int stake_side = 0; stake_side < (is_short_permitted+1); stake_side++) {
						uint32 org_opt_max_duck_val = 0;
						if(stake_side == 0) {
							org_opt_max_duck_val = ts_rec.OptMaxDuck;
						}
						else if(stake_side == 1) {
							org_opt_max_duck_val = ts_rec.OptMaxDuck_S;
						}
						uint32 cur_opt_max_duck_val = org_opt_max_duck_val;
						if(process_flags & processfFindOptMaxDuck && (org_opt_max_duck_val <= 0 || (process_flags & processfForce))) {
							TrainNnParam tnnp(ts_rec, TrainNnParam::afAnalyzeFrame);
							tnnp.SpikeQuant = spike_quant;
							tnnp.TargetQuant = 0;
							tnnp.EpochCount = 1;
							tnnp.InputFrameSize = 0;
							tnnp.MaxDuckQuant = 0;
							tnnp.StakeThreshold = 0.05;
							SETFLAG(tnnp.BaseFlags, tnnp.bfShort, stake_side == 1);
							IntRange md_range;
							md_range.Set(50, 500);
							TSVector <MaxDuckToResultRelation> opt_max_duck_set;
							THROW(FindOptimalMaxDuck(ts, vec_idx, tnnp, md_range, 50, &opt_max_duck_set, opt_max_duck));
							cur_opt_max_duck_val = opt_max_duck.MaxDuckQuant;
							{
								PPID   temp_id = id;
								PPTimeSeries ts_rec_to_upd;
								THROW(GetPacket(temp_id, &ts_rec_to_upd) > 0);
								if(stake_side == 0) {
									ts_rec_to_upd.OptMaxDuck = cur_opt_max_duck_val;
								}
								else if(stake_side == 1) {
									ts_rec_to_upd.OptMaxDuck_S = cur_opt_max_duck_val;
								}
								THROW(PutPacket(&temp_id, &ts_rec_to_upd, 1));
							}
						}
						if(process_flags & processfFindStrategies) {
							StrategyContainer scontainer;
							scontainer.SetLastValTm(last_val_dtm);
							if(process_flags & processfForce) {
								THROW(PutStrategies(id, 0, 1));
							}
							for(uint ifsidx = 0; ifsidx < SIZEOFARRAY(input_frame_size_list); ifsidx++) {
								const uint input_frame_size = input_frame_size_list[ifsidx];
								trend_list.clear();
								PROFILE_START
								{
									STimeSeries::AnalyzeFitParam afp(input_frame_size, 0, 0);
									THROW(ts.AnalyzeFit("close", afp, &trend_list, 0));
									assert(trend_list.getCount() == tsc);
								}
								PROFILE_END
								if(1) {
									uint __max_duck_quant_list[64];
									uint __max_duck_quant_count = 0;
									if(cur_opt_max_duck_val > 0 && cur_opt_max_duck_val < 1000) {
										const uint gr_count = 7;
										uint gr = cur_opt_max_duck_val / gr_count;
										for(uint i = 0; i < gr_count; i++) {
											__max_duck_quant_list[__max_duck_quant_count++] = cur_opt_max_duck_val - (gr * i);
										}
									}
									else {
										__max_duck_quant_list[__max_duck_quant_count++] = 50;
										__max_duck_quant_list[__max_duck_quant_count++] = 60;
										__max_duck_quant_list[__max_duck_quant_count++] = 70;
										__max_duck_quant_list[__max_duck_quant_count++] = 80;
										__max_duck_quant_list[__max_duck_quant_count++] = 90;
										__max_duck_quant_list[__max_duck_quant_count++] = 100;
										__max_duck_quant_list[__max_duck_quant_count++] = 150;
									}
									uint phase_n = 1;
									for(uint mdidx = 0; mdidx < __max_duck_quant_count; mdidx++) {
										TrainNnParam tnnp(ts_rec, TrainNnParam::afAnalyzeFrame);
										symb_buf = ts_rec.Symb;
										if(stake_side == 1)
											symb_buf.CatChar('-').Cat("REV");
										tnnp.SpikeQuant = spike_quant;
										tnnp.TargetQuant = 0;
										tnnp.EpochCount = 1;
										tnnp.InputFrameSize = input_frame_size;
										tnnp.MaxDuckQuant = __max_duck_quant_list[mdidx];
										tnnp.StakeThreshold = 0.05;
										SETFLAG(tnnp.BaseFlags, tnnp.bfShort, stake_side == 1);
										{
											StrategyResultEntry sre(tnnp, 0);
											THROW(TestStrategy(ts, vec_idx, trend_list, tnnp, sre));
											THROW_SL(scontainer.insert(dynamic_cast <Strategy *>(&sre)));
											{
												msg_buf.Z().
													Cat(symb_buf).Space().Space().Tab().
													Cat(sre.SpikeQuant, MKSFMTD(15, 8, 0)).Tab().
													Cat(sre.MaxDuckQuant).Tab().
													Cat(sre.InputFrameSize).Tab().
													Cat(sre.V.Result, MKSFMTD(15, 5, 0)).Tab().
													Cat(sre.StakeCount).Tab().
													Cat((double)sre.WinCount / (double)sre.StakeCount, MKSFMTD(15, 5, 0)).Tab().
													Cat(sre.V.TmCount).Tab().
													Cat(sre.V.TmSec).Tab().
													Cat(sre.V.GetResultPerDay()).Tab().
													//Cat(opt_delta_range.low, MKSFMTD(15, 10, 0)).Dot().Dot().Cat(opt_delta_range.upp, MKSFMTD(0, 10, 0)).Tab().
													Cat(sre.OptDeltaCount).CatChar('/').Cat(sre.LastResultIdx-sre.InputFrameSize+1).Tab().
													Cat(sre.OptDeltaResult, MKSFMTD(15, 5, 0)).Tab().
													//Cat(opt_delta2_range.low, MKSFMTD(15, 10, 0)).Dot().Dot().Cat(opt_delta2_range.upp, MKSFMTD(0, 10, 0)).Tab().
													Cat(sre.OptDelta2Stride).CatChar('/').Cat(sre.OptDelta2Count).CatChar('/').Cat(sre.LastResultIdx-sre.InputFrameSize+1).Tab().
													Cat(sre.OptDelta2Result, MKSFMTD(15, 5, 0)).Tab();
												f_out.WriteLine(msg_buf.CR());
												f_out.Flush();
												//
												tnnp.OptDeltaRange = sre.OptDeltaRange;
												tnnp.OptDelta2Range =  sre.OptDelta2Range;
												tnnp.OptDelta2Stride = sre.OptDelta2Stride;
											}
										}
										{
											StrategyResultEntry sre_test(tnnp, 1);
											THROW(TestStrategy(ts, vec_idx, trend_list, tnnp, sre_test));
											THROW_SL(scontainer.insert(dynamic_cast <Strategy *>(&sre_test)));
											{
												msg_buf.Z().
													Cat(symb_buf).CatChar('/').Cat("1").Tab().
													Cat(sre_test.SpikeQuant, MKSFMTD(15, 8, 0)).Tab().
													Cat(sre_test.MaxDuckQuant).Tab().
													Cat(sre_test.InputFrameSize).Tab().
													Cat(sre_test.V.Result, MKSFMTD(15, 5, 0)).Tab().
													Cat(sre_test.StakeCount).Tab().
													Cat((double)sre_test.WinCount / (double)sre_test.StakeCount, MKSFMTD(15, 5, 0)).Tab().
													Cat(sre_test.V.TmCount).Tab().
													Cat(sre_test.V.TmSec).Tab().
													Cat(sre_test.V.GetResultPerDay()).Tab().
													//Cat(opt_delta_range.low, MKSFMTD(15, 10, 0)).Dot().Dot().Cat(opt_delta_range.upp, MKSFMTD(0, 10, 0)).Tab().
													//Cat(sre_test.OptDeltaCount).CatChar('/').Cat(sre_test.LastResultIdx-sre_test.InputFrameSize+1).Tab().
													//Cat(sre_test.OptDeltaResult, MKSFMTD(15, 5, 0)).Tab().
													Cat("").Tab().
													Cat("").Tab().
													//Cat(opt_delta2_range.low, MKSFMTD(15, 10, 0)).Dot().Dot().Cat(opt_delta2_range.upp, MKSFMTD(0, 10, 0)).Tab().
													//Cat(sre_test.OptDelta2Stride).CatChar('/').Cat(sre_test.OptDelta2Count).CatChar('/').Cat(sre_test.LastResultIdx-sre_test.InputFrameSize+1).Tab().
													//Cat(sre_test.OptDelta2Result, MKSFMTD(15, 5, 0)).Tab();
													Cat("").Tab().
													Cat("").Tab();
												f_out.WriteLine(msg_buf.CR());
												f_out.Flush();
											}
										}
										{
											StrategyResultEntry sre_test(tnnp, 2);
											THROW(TestStrategy(ts, vec_idx, trend_list, tnnp, sre_test));
											THROW_SL(scontainer.insert(dynamic_cast <Strategy *>(&sre_test)));
											{
												msg_buf.Z().
													Cat(symb_buf).CatChar('/').Cat("2").Tab().
													Cat(sre_test.SpikeQuant, MKSFMTD(15, 8, 0)).Tab().
													Cat(sre_test.MaxDuckQuant).Tab().
													Cat(sre_test.InputFrameSize).Tab().
													Cat(sre_test.V.Result, MKSFMTD(15, 5, 0)).Tab().
													Cat(sre_test.StakeCount).Tab().
													Cat((double)sre_test.WinCount / (double)sre_test.StakeCount, MKSFMTD(15, 5, 0)).Tab().
													Cat(sre_test.V.TmCount).Tab().
													Cat(sre_test.V.TmSec).Tab().
													Cat(sre_test.V.GetResultPerDay()).Tab().
													//Cat(opt_delta_range.low, MKSFMTD(15, 10, 0)).Dot().Dot().Cat(opt_delta_range.upp, MKSFMTD(0, 10, 0)).Tab().
													//Cat(sre_test.OptDeltaCount).CatChar('/').Cat(sre_test.LastResultIdx-sre_test.InputFrameSize+1).Tab().
													//Cat(sre_test.OptDeltaResult, MKSFMTD(15, 5, 0)).Tab().
													Cat("").Tab().
													Cat("").Tab().
													//Cat(opt_delta2_range.low, MKSFMTD(15, 10, 0)).Dot().Dot().Cat(opt_delta2_range.upp, MKSFMTD(0, 10, 0)).Tab().
													//Cat(sre_test.OptDelta2Stride).CatChar('/').Cat(sre_test.OptDelta2Count).CatChar('/').Cat(sre_test.LastResultIdx-sre_test.InputFrameSize+1).Tab().
													//Cat(sre_test.OptDelta2Result, MKSFMTD(15, 5, 0)).Tab();
													Cat("").Tab().
													Cat("").Tab();
												f_out.WriteLine(msg_buf.CR());
												f_out.Flush();
											}
										}
										if(0) {
											int   ann_loaded = 0;
											{
												save_file_name.Z();
												tnnp.MakeFileName(temp_buf);
												PPGetFilePath(PPPATH_OUT, temp_buf, save_file_name);
												if(fileExists(save_file_name)) {
													p_ann = fann_create_from_file(save_file_name);
													if(p_ann) {
														ann_loaded = 1;
														msg_buf.Z().Cat("NN has been created from file").Space().Cat(save_file_name);
														PPLogMessage(PPFILNAM_NNTRAIN_LOG, msg_buf, LOGMSGF_TIME);
													}
												}
											}
											SETFLAG(tnnp.ActionFlags, TrainNnParam::afTrainNN, !ann_loaded);
											if(tnnp.ActionFlags & TrainNnParam::afTrainNN) {
												for(uint tridx = 0; tridx < tnnp.EpochCount; tridx++) {
													const clock_t cstart = clock();
													THROW(AnalyzeStrategy(ts, tnnp, trend_list, &p_ann));
													const clock_t cend = clock();
													if(p_ann) {
														msg_buf.Z().Cat(ts_rec.Symb).Space().CatEq("Phase", phase_n).Space().CatEq("MaxDuckQuant", tnnp.MaxDuckQuant).
															Space().CatEq("MSE", fann_get_MSE(p_ann), MKSFMTD(0, 6, 0)).Space().CatEq("Clock", cend-cstart);
														PPLogMessage(PPFILNAM_NNTRAIN_LOG, msg_buf, LOGMSGF_TIME);
														if(fann_save(p_ann, save_file_name) == 0) {
															msg_buf.Z().Cat("NN has been saved to file").Space().Cat(save_file_name);
															PPLogMessage(PPFILNAM_NNTRAIN_LOG, msg_buf, LOGMSGF_TIME);
														}
													}
													phase_n++;
												}
											}
											/*if(p_ann) {
												SETFLAG(tnnp.Flags, TrainNnParam::fTrainNN, 0);
												THROW(AnalyzeStrategy(ts, tnnp, trend_list, &p_ann));
											}*/
										}
									}
								}
							}
							THROW(PutStrategies(id, &scontainer, 1));
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
	fann_destroy(p_ann);
	return ok;
}
#endif // } 0

SLAPI PPObjTimeSeries::Strategy::Strategy() : InputFrameSize(0), Prec(0), TargetQuant(0), MaxDuckQuant(0), OptDelta2Stride(0), StakeMode(0),
	BaseFlags(0), Margin(0.0), SpikeQuant(0.0), StakeThreshold(0.0), SpreadAvg(0.0), OptDeltaCount(0), OptDelta2Count(0), StakeCount(0), WinCount(0),
	StakeCloseMode(0)
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

void PPObjTimeSeries::StrategyContainer::SetLastValTm(LDATETIME dtm)
{
	LastValTm = dtm;
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
	THROW_SL(pSCtx->Serialize(dir, dynamic_cast<SVector *>(this), rBuf));
	CATCHZOK
	return ok;
}

int SLAPI PPObjTimeSeries::PutStrategies(PPID id, StrategyContainer * pL, int use_ta)
{
	int    ok = 1;
	PPTimeSeries ts_rec;
	SBuffer buffer;
	THROW(Search(id, &ts_rec) > 0);
	if(pL && pL->getCount()) {
		SSerializeContext sctx;
		THROW(pL->Serialize(+1, buffer, &sctx));
	}
	THROW(ref->PutPropSBuffer(Obj, id, TIMSERPRP_STAKEMODEL, buffer, use_ta));
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
	CATCHZOK
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

struct __TsProbEntry {
	uint   MeanMult;
	uint   Duration;
	uint   CaseCount;
	uint   WinCount;
	uint   LossCount;
	double WinSum;
	double LossSum;
};

/*int SLAPI PPObjTimeSeries::IsCase(const STimeSeries & rTs, const TrainNnParam & rP, uint vecIdx, uint lastIdx) const
{
	int    ok = -1;
	const  uint tsc = rTs.GetCount();
	const  uint seqc = rP.SeqCount;
	if(lastIdx < tsc && lastIdx > seqc && rP.SpikeQuant > 0.0) {
		double seq_values[SIZEOFARRAY(rP.Seq)+1];
		{
			for(uint i = (lastIdx-seqc-1), svidx = 0; i <= lastIdx; i++, svidx++)
				rTs.GetValue(i, vecIdx, &seq_values[svidx]);
		}
		double prev_value = seq_values[0];
		bool   qc = true;
		for(uint i = 1; qc && i <= seqc; i++) {
			int quant_count;
			int condition;
			assert(rP.GetSeq(i-1, &quant_count, &condition));
			const double value = seq_values[i];
			const double delta = ((value - prev_value) / prev_value);
			const long rdelta = (long)R0(delta);
			const long rquant = (long)R0(rP.SpikeQuant * quant_count);
			switch(condition) {
				case _GT_: qc = (rdelta > rquant); break;
				case _GE_: qc = (rdelta >= rquant); break;
				case _EQ_: qc = (rdelta == rquant); break;
				case _LT_: qc = (rdelta < rquant); break;
				case _LE_: qc = (rdelta <= rquant); break;
				default: qc = false;
			}
			prev_value = value;
		}
		ok = BIN(qc);
	}
	return ok;
}*/

int SLAPI PPObjTimeSeries::AnalyzeAftershock(const STimeSeries & rTs, const TrainNnParam & rP)
{
	int    ok = 1;
	uint   vec_idx = 0;
	SString temp_buf;
	SString out_file_name;
	SString msg_buf;
	StatBase stat_plus(StatBase::fStoreVals/*|StatBase::fGammaTest|StatBase::fGaussianTest*/);
	StatBase stat_minus(StatBase::fStoreVals/*|StatBase::fGammaTest|StatBase::fGaussianTest*/);
	TSVector <__TsProbEntry> prob_result_list;
	const  uint lss_dist = /*2000*/0; // Дистанция для отсчета назад с целью вычисления тренда
	const  uint tsc = rTs.GetCount();
	LVect lss_vect_x(lss_dist);
	LVect lss_vect_y(lss_dist);
	lss_vect_x.FillWithSequence(1.0, 1.0); // Заполняем ось ординат последовательными значениями 1.0..lss_dim
	if(tsc > (1+lss_dist)) {
		const double margin = (rP.Margin > 0.0) ? rP.Margin : 1.0;
		THROW_SL(rTs.GetValueVecIndex("close", &vec_idx));
		{
			PPGetFilePath(PPPATH_OUT, "AnalyzeTsAfetershock.txt", out_file_name);
			const int is_out_file_exists = fileExists(out_file_name);
			SFile f_out(out_file_name, SFile::mAppend);
			if(!is_out_file_exists) {
				msg_buf.Z().Cat(getcurdatetime_(), DATF_YMD|DATF_CENTURY, TIMF_HMS);
				f_out.WriteLine(msg_buf.CR());
				// symb count stat-plus-max  stat-plus-mean  stat-plus-disp  stat-plus-gauss-test  stat-plus-gamma-test
				msg_buf.Z().Cat("symb").
					Tab().Cat("margin").
					Tab().Cat("count").
					Tab().Cat("sign").
					Tab().Cat("stat-max").
					Tab().Cat("stat-mean").
					Tab().Cat("stat-disp");
				f_out.WriteLine(msg_buf.CR());
			}
			double prev_value;
			rTs.GetValue(0, vec_idx, &prev_value);
			for(uint i = 1; i < tsc; i++) {
				double value;
				rTs.GetValue(i, vec_idx, &value);
				if(prev_value != 0.0) {
					const double delta = ((value - prev_value) / prev_value) / margin;
					if(delta > 0.0)
						stat_plus.Step(delta);
					else if(delta < 0.0)
						stat_minus.Step(-delta);
				}
				prev_value = value;
			}
			stat_plus.Finish();
			stat_minus.Finish();
			// symb count stat-plus-max  stat-plus-mean  stat-plus-disp  stat-plus-gauss-test  stat-plus-gamma-test
			// symb count stat-minus-max stat-minus-mean stat-minus-disp stat-minus-gauss-test stat-minus-gamma-test
			msg_buf.Z().Cat((temp_buf = rP.Symb).Align(12, ADJ_LEFT)).
				Tab().Cat(margin, MKSFMTD(0, 5, 0)).
				Tab().Cat(tsc).
				Tab().Cat("plus").
				Tab().Cat(stat_plus.GetMax(), MKSFMTD(0, 8, 0)).
				Tab().Cat(stat_plus.GetExp(), MKSFMTD(0, 8, 0)).
				Tab().Cat(stat_plus.GetStdDev(), MKSFMTD(0, 8, 0));
			f_out.WriteLine(msg_buf.CR());
			msg_buf.Z().Cat((temp_buf = rP.Symb).Align(12, ADJ_LEFT)).
				Tab().Cat(margin, MKSFMTD(0, 5, 0)).
				Tab().Cat(tsc).
				Tab().Cat("minus").
				Tab().Cat(stat_minus.GetMax(), MKSFMTD(0, 8, 0)).
				Tab().Cat(stat_minus.GetExp(), MKSFMTD(0, 8, 0)).
				Tab().Cat(stat_minus.GetStdDev(), MKSFMTD(0, 8, 0)).
				Tab().Cat(stat_minus.GetTestGaussian(), MKSFMTD(0, 8, 0)).
				Tab().Cat(stat_minus.GetTestGamma(), MKSFMTD(0, 8, 0));
			f_out.WriteLine(msg_buf.CR());
			if(1) { 
				//
				// Grid: mean-mult; target; duck; duration
				//
				const double mean_plus = stat_plus.GetExp();
				static const uint   mean_mult_grid[] = { 2, 5, 10, 20, 30, 40, 50 };         // #0
				static const uint   duration_grid[] = { 1, 2, 3, 4, 5, 10, 15, 20, 30 };   // #1
				static const double target_grid[] = { 0.1, 0.5, 1.0, 1.5 };            // #2
				static const double duck_part_grid[] = { 0.1, 0.25, 0.50, 0.75, 1.0 }; // #3

				RealArray trend_vect;
				uint plus_trend_count = 0;
				uint minus_trend_count = 0;
				if(lss_dist) {
					for(uint j = lss_dist+1; j < tsc; j++) {
						assert(j >= lss_dist);
						LssLin lss;
						rTs.GetLVect(vec_idx, j-lss_dist, lss_vect_y);
						lss.Solve(lss_vect_x, lss_vect_y);
						double trend = lss.B;
						if(trend > 0.0) {
							plus_trend_count++;
						}
						else {
							minus_trend_count++;
						}
						THROW_SL(trend_vect.insert(&trend));
					}
				}
				msg_buf.Z().CR().
					Cat("mean-mult").
					Tab().Cat("duration").
					//Tab().Cat("target").
					//Tab().Cat("duck").
					Tab().Cat("case-count").
					Tab().Cat("trend-avg").
					Tab().Cat("win-count").
					Tab().Cat("win-sum").
					Tab().Cat("loss-count").
					Tab().Cat("loss-summ").
					Tab().Cat("win-loss-count").
					Tab().Cat("win-loss-sum");
				f_out.WriteLine(msg_buf.CR());

				for(uint mmi = 0; mmi < SIZEOFARRAY(mean_mult_grid); mmi++) {
					const uint mean_mult = mean_mult_grid[mmi];
					const double threshold = mean_plus * mean_mult;
					for(uint duri = 0; duri < SIZEOFARRAY(duration_grid); duri++) {
						const uint duration = duration_grid[duri];
						//for(uint ti = 0; ti < SIZEOFARRAY(target_grid); ti++) {
							//const double target = target_grid[ti];
							//for(uint dpi = 0; dpi < SIZEOFARRAY(duck_part_grid); dpi++) {
								//const double duck_part = duck_part_grid[dpi];
								rTs.GetValue(lss_dist/*0*/, vec_idx, &prev_value);
								StatBase stat_trend(0);
								__TsProbEntry prob_result_entry;
								MEMSZERO(prob_result_entry);
								prob_result_entry.MeanMult = mean_mult;
								prob_result_entry.Duration = duration;
								for(uint j = lss_dist+1; j < tsc; j++) {
									double value;
									rTs.GetValue(j, vec_idx, &value);
									const double delta = ((value - prev_value) / prev_value) / margin;
									if(delta >= threshold && (j+duration) < tsc) {
										assert(j >= lss_dist);
										double trend = lss_dist ? trend_vect.at(j-lss_dist-1) : 1.0;
										if(trend > 0.0) {
											stat_trend.Step(trend);
											plus_trend_count++;
											prob_result_entry.CaseCount++;
											double next_value;
											/*for(uint k = 1; k <= duration; k++) {
												rTs.GetValue(j+k, vec_idx, &next_value);
												if(next_value > value) {
												}
												else if(next_value < value) {
												}
											}*/
											rTs.GetValue(j+duration, vec_idx, &next_value);
											const double result_delta = (next_value - value) / margin;
											if(result_delta > 0.0) {
												prob_result_entry.WinCount++;
												prob_result_entry.WinSum += result_delta;
											}
											else {
												prob_result_entry.LossCount++;
												prob_result_entry.LossSum += -result_delta;
											}
										}
									}
								}
								prob_result_list.insert(&prob_result_entry);
								stat_trend.Finish();
								msg_buf.Z().
									Cat(mean_mult, MKSFMTD(0, 3, 0)).
									Tab().Cat(duration).
									//Tab().Cat(target, MKSFMTD(0, 5, 0)).
									//Tab().Cat(duck, MKSFMTD(0, 5, 0)).
									Tab().Cat(prob_result_entry.CaseCount).
									Tab().Cat(stat_trend.GetExp(), MKSFMTD(0, 9, 0)).
									Tab().Cat(prob_result_entry.WinCount).
									Tab().Cat(prob_result_entry.WinSum, MKSFMTD(0, 5, 0)).
									Tab().Cat(prob_result_entry.LossCount).
									Tab().Cat(prob_result_entry.LossSum, MKSFMTD(0, 5, 0)).
									Tab().Cat((int)prob_result_entry.WinCount-(int)prob_result_entry.LossCount).
									Tab().Cat(prob_result_entry.WinSum-prob_result_entry.LossSum, MKSFMTD(0, 5, 0));
								f_out.WriteLine(msg_buf.CR());
							//}
						//}
					}
				}
				{
					int    best_count = -1000000;
					double best_result = -1E9;
					uint   best_count_idx = 0;
					uint   best_result_idx = 0;
					for(uint bi = 0; bi < prob_result_list.getCount(); bi++) {
						const __TsProbEntry & r_pe = prob_result_list.at(bi);
						int c = ((int)r_pe.WinCount-(int)r_pe.LossCount);
						double r = (r_pe.WinSum-r_pe.LossSum);
						if(best_count < c) {
							best_count = c;
							best_count_idx = bi;
						}
						if(best_result < r) {
							best_result = r;
							best_result_idx = bi;
						}
					}
					const __TsProbEntry & r_pe_best_count = prob_result_list.at(best_count_idx);
					const __TsProbEntry & r_pe_best_result = prob_result_list.at(best_result_idx);
					msg_buf.Z().Cat("The best count").Tab().
						Cat(r_pe_best_count.MeanMult).
						Tab().Cat(r_pe_best_count.Duration).
						//Tab().Cat(target, MKSFMTD(0, 5, 0)).
						//Tab().Cat(duck, MKSFMTD(0, 5, 0)).
						Tab().Cat(r_pe_best_count.CaseCount).
						Tab().Cat("").
						Tab().Cat(r_pe_best_count.WinCount).
						Tab().Cat(r_pe_best_count.WinSum, MKSFMTD(0, 5, 0)).
						Tab().Cat(r_pe_best_count.LossCount).
						Tab().Cat(r_pe_best_count.LossSum, MKSFMTD(0, 5, 0)).
						Tab().Cat((int)r_pe_best_count.WinCount-(int)r_pe_best_count.LossCount).
						Tab().Cat(r_pe_best_count.WinSum-r_pe_best_count.LossSum, MKSFMTD(0, 5, 0));
					f_out.WriteLine(msg_buf.CR());
					//
					msg_buf.Z().Cat("The best result").Tab().
						Cat(r_pe_best_result.MeanMult).
						Tab().Cat(r_pe_best_result.Duration).
						//Tab().Cat(target, MKSFMTD(0, 5, 0)).
						//Tab().Cat(duck, MKSFMTD(0, 5, 0)).
						Tab().Cat(r_pe_best_result.CaseCount).
						Tab().Cat("").
						Tab().Cat(r_pe_best_result.WinCount).
						Tab().Cat(r_pe_best_result.WinSum, MKSFMTD(0, 5, 0)).
						Tab().Cat(r_pe_best_result.LossCount).
						Tab().Cat(r_pe_best_result.LossSum, MKSFMTD(0, 5, 0)).
						Tab().Cat((int)r_pe_best_result.WinCount-(int)r_pe_best_result.LossCount).
						Tab().Cat(r_pe_best_result.WinSum-r_pe_best_result.LossSum, MKSFMTD(0, 5, 0));
					f_out.WriteLine(msg_buf.CR());
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

double SLAPI PPObjTimeSeries::Strategy::CalcSL(double stakeValue, double peak, double bottom) const
{
	double sl = 0.0;
	const  double mdv = (MaxDuckQuant * SpikeQuant);
	if(BaseFlags & bfShort) {
		if(StakeCloseMode == 1) {
			double dp = stakeValue - peak;
			double db = bottom - stakeValue;
			if(dp > db)
				sl = peak * (1.0 + mdv);
			else 
				sl = (stakeValue - db) * (1.0 + mdv);
		}
		else
			sl = peak * (1.0 + mdv);
	}
	else {
		if(StakeCloseMode == 1) {
			double dp = peak - stakeValue;
			double db = stakeValue - bottom;
			if(dp > db)
				sl = peak * (1.0 - mdv);
			else 
				sl = (stakeValue + db) * (1.0 - mdv);
		}
		else
			sl = peak * (1.0 - mdv);
	}
	return round(sl, Prec);
}

int SLAPI PPObjTimeSeries::CalcStrategyResult(const STimeSeries & rTs, const Strategy & rS, uint vecIdx, uint valueIdx, StrategyResultValue & rV) const
{
	int    ok = 1;
	const  uint tsc = rTs.GetCount();
	rV.Z();
	if(valueIdx < tsc) {
		double prev_value;
		LDATETIME start_tm;
		SUniTime ut;
		THROW(rTs.GetValue(valueIdx, vecIdx, &prev_value));
		rTs.GetTime(valueIdx, &ut);
		ut.Get(start_tm);
		const double stake_value = prev_value;
		double peak = prev_value;
		double bottom = prev_value;
		const  uint org_max_duck_quant = rS.MaxDuckQuant;
		const  uint max_duck_quant = org_max_duck_quant; // Возможно, будет меняться в течении жизни ставки (пока нет)
		{
			const int is_short = BIN(rS.BaseFlags & rS.bfShort);
			const double spike_quant = rS.SpikeQuant;
			const double target_value = rS.TargetQuant * spike_quant;
			const double spread = (rS.Prec > 0 && rS.SpreadAvg > 0.0) ? (rS.SpreadAvg * fpow10i(-rS.Prec)) : 0.0;
			double sl = rS.CalcSL(stake_value, peak, bottom);
			uint k = valueIdx+1;
			while(k < tsc) {
				double value;
				THROW(rTs.GetValue(k, vecIdx, &value));
				if(is_short) {
					const double value_with_spread = (value + spread);
					const double stake_delta = -((value - stake_value) / stake_value);
					rV.Result = -((value_with_spread - stake_value) / stake_value) / rS.Margin;
					if(value >= sl) {
						ok = 2;
						break;
					}
					else if(rS.TargetQuant && (stake_delta > target_value)) {
						ok = 2;
						break;									
					}
					else {
						SETMIN(peak, value);
						SETMAX(bottom, value);
						prev_value = value;
						sl = rS.CalcSL(stake_value, peak, bottom);
						k++;
					}
				}
				else {
					const double value_with_spread = (value - spread);
					const double stake_delta = ((value - stake_value) / stake_value);
					rV.Result = ((value_with_spread - stake_value) / stake_value) / rS.Margin;
					if(value <= sl) {
						ok = 2;
						break;
					}
					else if(rS.TargetQuant && (stake_delta > target_value)) {
						ok = 2;
						break;									
					}
					else {
						SETMAX(peak, value);
						SETMIN(bottom, value);
						prev_value = value;
						sl = rS.CalcSL(stake_value, peak, bottom);
						k++;
					}
				}
			}
			if(k == tsc)
				ok = 3;
			rV.TmCount = k - valueIdx;
			SUniTime local_ut;
			LDATETIME local_tm;
			rTs.GetTime(k, &local_ut);
			local_ut.Get(local_tm);
			rV.TmSec = diffdatetimesec(local_tm, start_tm);
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjTimeSeries::AnalyzeStrategy(const STimeSeries & rTs, const TrainNnParam & rP, const RealArray & rTrendList, Fann2 ** ppAnn)
{
	int    ok = 1;
	uint   vec_idx = 0;
	Fann2 * p_ann = 0;
	SString temp_buf;
	SString out_file_name;
	SString out_win_file_name;
	SString stake_out_file_name;
	SString result_out_file_name;
	SString msg_buf;
	const  uint tsc = rTs.GetCount();
	const  uint __seqc = 5; // Анализируемое количество дельт, предшествующих текущему значению
	//
	// Вход нейронной сети:
	//   -- (0) ид серии
	//   -- (1) количество точек для расчета тренда
	//   -- (2) тренд
	//   -- (3) max-duck-quant
	//   -- (4..4+__seqc-1) __seqc последних изменений значений, нормализованных относительно кванта SpikeQuant
	//
	float  ann_input[4+__seqc];
	float  ann_output[1];
	float  ann_train_output[SIZEOFARRAY(ann_output)];
	if(ppAnn) {
		p_ann = *ppAnn;
		if(!p_ann) {
			if(__seqc && rP.SpikeQuant > 0.0) {
				p_ann = fann_create_standard_2(4, SIZEOFARRAY(ann_input), 64, 32, SIZEOFARRAY(ann_output));
				THROW(p_ann);
				fann_set_activation_function_hidden(p_ann, Fann2::FANN_SIGMOID_SYMMETRIC);
				fann_set_activation_function_output(p_ann, Fann2::FANN_SIGMOID_SYMMETRIC);
				fann_set_learning_rate(p_ann, 0.3f);
				fann_randomize_weights(p_ann, -1.0f, 1.0f);
			}
		}
	}
	if(tsc > (1+rP.InputFrameSize)) {
		assert(rTrendList.getCount() == tsc);
		THROW(rTrendList.getCount() == tsc);
		THROW_SL(rTs.GetValueVecIndex("close", &vec_idx));
		{
			struct ResultEntry {
				SString & MakeResultHeader(SString & rBuf) const
				{
					rBuf.Z().Cat("symb").
						Tab().Cat("time").
						Tab().Cat("close").
						Tab().Cat("delta").
						Tab().Cat("trend").
						Tab().Cat("seq").
						Tab().Cat("stake").
						Tab().Cat("stake-time").
						Tab().Cat("win").
						Tab().Cat("loss").
						Tab().Cat("ann-result").
						Tab().Cat("ann-mse");
					return rBuf;
				}
				SString & MakeResultLine(const SString & rSymb, SString & rBuf) const
				{
					rBuf.Z().Cat(rSymb).
						Tab().Cat(Tm, DATF_ISO8601|DATF_CENTURY, 0).
						Tab().Cat(Value, MKSFMTD(12,  5, 0)).
						Tab().Cat(Delta, MKSFMTD(25, 20, 0)).
						Tab().Cat(Trend, MKSFMTD(15, 10, 0)).
						Tab().Cat(SeqTxt).
						Tab().Cat(Stake, MKSFMTD(10,  5, 0)).
						Tab().Cat(V.TmCount).
						Tab().Cat(Win,   MKSFMTD(8, 5, 0)).
						Tab().Cat(Loss,  MKSFMTD(8, 5, 0)).
						Tab().Cat(AnnResult, MKSFMTD(8, 5, 0)).
						Tab().Cat(AnnMse,    MKSFMTD(8, 5, 0));
					return rBuf;
				}
				ResultEntry()
				{
					Tm.Z();
					PrevValue = 0.0;
					Value = 0.0;
					Delta = 0.0;
					Trend = 0.0;
					SeqTxt.Z();
					Stake = 0.0;
					StakeCount = 0;
					V.Z();
					WinCount = 0;
					LossCount = 0;
					Win = 0.0;
					Loss = 0.0;
					WinTotal = 0.0;
					LossTotal = 0.0;
					AnnResult = 0.0;
					AnnMse = 0.0f;
				}
				void   FASTCALL SetValue(StrategyResultValue & rV)
				{
					V = rV;
					const double result = rV.Result;
					if(result > 0.0) {
						WinCount++;
						Win += result;
						WinTotal += result;
					}
					else if(result < 0.0) {
						LossCount++;
						Loss -= result;
						LossTotal -= result;
					}
				}
				LDATETIME Tm;
				double PrevValue;
				double Value;
				double Delta;
				double Trend;
				SString SeqTxt;
				double Stake;
				uint   StakeCount;
				StrategyResultValue V;
				uint   WinCount;
				uint   LossCount;
				double Win;
				double Loss;
				double WinTotal;
				double LossTotal;
				double AnnResult; // Результат предсказания нейронной сетью
				float  AnnMse;    // Ошибка обучения нейронной сети
			};
			//
			SString save_file_name;
			save_file_name.Z();
			rP.MakeFileName(temp_buf);
			PPGetFilePath(PPPATH_OUT, temp_buf, save_file_name);

			ResultEntry result_entry;
			PPGetFilePath(PPPATH_OUT, "AnalyzeTsStrategy.txt", out_file_name);
			PPGetFilePath(PPPATH_OUT, "AnalyzeTsStrategy-win.txt", out_win_file_name);
			PPGetFilePath(PPPATH_OUT, "AnalyzeTsStrategy-stake.txt", stake_out_file_name);
			PPGetFilePath(PPPATH_OUT, "AnalyzeTsStrategy-result.txt", result_out_file_name);
			const int is_out_file_exists = fileExists(out_file_name);
			const int is_out_win_file_exists = fileExists(out_win_file_name);
			const int is_stake_out_file_exists = fileExists(stake_out_file_name);
			const int is_result_out_file_exists = fileExists(result_out_file_name);
			SFile f_out(out_file_name, SFile::mAppend);
			SFile f_out_win(out_win_file_name, SFile::mAppend);
			SFile f_stake_out(stake_out_file_name, SFile::mAppend);
			SFile f_result_out(result_out_file_name, SFile::mAppend);
			if(!is_out_file_exists) {
				msg_buf.Z().Cat(getcurdatetime_(), DATF_YMD|DATF_CENTURY, TIMF_HMS);
				f_out.WriteLine(msg_buf.CR());
				f_out.WriteLine(result_entry.MakeResultHeader(msg_buf).CR());
			}
			if(!is_out_win_file_exists) {
				msg_buf.Z().Cat(getcurdatetime_(), DATF_YMD|DATF_CENTURY, TIMF_HMS);
				f_out_win.WriteLine(msg_buf.CR());
				f_out_win.WriteLine(result_entry.MakeResultHeader(msg_buf).CR());
			}
			if(!is_stake_out_file_exists) {
				msg_buf.Z().Cat(getcurdatetime_(), DATF_YMD|DATF_CENTURY, TIMF_HMS);
				f_stake_out.WriteLine(msg_buf.CR());
				f_stake_out.WriteLine(result_entry.MakeResultHeader(msg_buf).CR());
			}
			if(!is_result_out_file_exists) {
				msg_buf.Z().Cat("Symbol").
					Tab().Cat("Time").
					//Tab().Cat("TrendGt").
					Tab().Cat("MaxDuckQuant").
					Tab().Cat("WinCount").
					Tab().Cat("LossCount").
					Tab().Cat("WinTotal").
					Tab().Cat("LossTotal");
				f_result_out.WriteLine(msg_buf.CR());
			}
			uint   train_iter_count = 0;
			SUniTime ut;
			rTs.GetTime(0, &ut);
			ut.Get(result_entry.Tm);
			rTs.GetValue(0, vec_idx, &result_entry.Value);
			f_out.WriteLine(result_entry.MakeResultLine(rTs.GetSymb(), msg_buf).CR());
			f_stake_out.WriteLine(result_entry.MakeResultLine(rTs.GetSymb(), msg_buf).CR());
			result_entry.PrevValue = result_entry.Value;
			for(uint i = 1; i < tsc; i++) {
				rTs.GetTime(i, &ut);
				ut.Get(result_entry.Tm);
				rTs.GetValue(i, vec_idx, &result_entry.Value);
				result_entry.Delta = ((result_entry.Value - result_entry.PrevValue) / result_entry.PrevValue);
				result_entry.SeqTxt.Z();
				int    do_ann_learn_iter = 0;
				if(__seqc && i >= __seqc && rP.SpikeQuant > 0.0) {
					double seq_values[/*SIZEOFARRAY(rP.Seq)*/32+1];
					{
						for(uint j = (i-__seqc), svidx = 0; j <= i; j++, svidx++)
							rTs.GetValue(j, vec_idx, &seq_values[svidx]);
					}
					double local_prev_value = seq_values[0];
					if(p_ann && (rP.ActionFlags & rP.afTrainNN)) {
						do_ann_learn_iter = 1;
					}
					for(uint j = 1; j <= __seqc; j++) {
						const double local_value = seq_values[j];
						const double local_delta = ((local_value - local_prev_value) / local_prev_value);
						if(j == __seqc) {
							assert(local_delta == result_entry.Delta);
						}
						if(j > 1)
							result_entry.SeqTxt.Space();
						const long qd = (long)R0(local_delta / rP.SpikeQuant);
						if(qd == 0)
							result_entry.SeqTxt.Space().Space();
						else {
							if(qd > 0)
								result_entry.SeqTxt.CatChar('+');
							else 
								result_entry.SeqTxt.CatChar('-');
							if(abs(qd) < 10)
								result_entry.SeqTxt.Space();
						}
						result_entry.SeqTxt.Cat(abs(qd));
						local_prev_value = local_value;
						if(p_ann)
							ann_input[4+j-1] = (float)qd;
					}
				}
				if(i >= rP.InputFrameSize) {
					result_entry.Trend = rTrendList.at(i);
					if(p_ann/*|| (rP.TrendGe == 0.0 || result_entry.Trend >= rP.TrendGe)*/) {
						{
							ann_input[0] = 0.0f; // ид серии
							ann_input[1] = (float)rP.InputFrameSize;
							ann_input[2] = (float)result_entry.Trend;
							ann_input[3] = (float)rP.MaxDuckQuant;
						}
						int icr = 0; //(p_ann || IsCase(rTs, rP, vec_idx, i));
						if(rP.ActionFlags & rP.afTrainNN) {
							icr = 1;
						}
						else {
							const float * p_local_ann_out = fann_run(p_ann, ann_input);
							if(p_local_ann_out) { 
								result_entry.AnnResult = *p_local_ann_out;
								if(result_entry.AnnResult > rP.StakeThreshold) 
									icr = 1;
							}
						}
						if(icr > 0) {
							result_entry.StakeCount++;
							result_entry.Stake = 1.0;
							StrategyResultValue rv;
							int csr = CalcStrategyResult(rTs, rP, vec_idx, i, rv);
							THROW(csr);
							result_entry.SetValue(rv);
							if(do_ann_learn_iter) {
								ann_output[0] = (float)rv.Result;
								/*if((tsc-i) <= 1000) {
									const float * p_local_ann_out = fann_run(p_ann, ann_input);
									result_entry.AnnResult = *p_local_ann_out;
								}
								else*/ {
									fann_train(p_ann, ann_input, ann_output, ann_train_output);
									result_entry.AnnResult = ann_train_output[0];
									result_entry.AnnMse = fann_get_MSE(p_ann);
								}
								train_iter_count++;
							}
						}
					}
					f_out.WriteLine(result_entry.MakeResultLine(rTs.GetSymb(), msg_buf).CR());
					if(result_entry.Win > 0.09) {
						f_out_win.WriteLine(result_entry.MakeResultLine(rTs.GetSymb(), msg_buf).CR());
					}
					if(!(rP.ActionFlags & rP.afTrainNN) && result_entry.Stake != 0.0) {
						f_stake_out.WriteLine(result_entry.MakeResultLine(rTs.GetSymb(), msg_buf).CR());
					}
				}
				//
				result_entry.Tm.Z();
				result_entry.Delta = 0.0;
				result_entry.Trend = 0.0;
				result_entry.SeqTxt.Z();
				result_entry.Win = 0.0;
				result_entry.Loss = 0.0;
				result_entry.Stake = 0.0;
				result_entry.V.Z();
				result_entry.AnnResult = 0.0;
				result_entry.AnnMse = 0.0f;
				result_entry.PrevValue = result_entry.Value;
				result_entry.Value = 0.0;
			}
			if(!(rP.ActionFlags & rP.afTrainNN)) {
				{
					msg_buf.Z().Cat(result_entry.WinCount).
						Tab().Cat(result_entry.LossCount).
						Tab().Cat(result_entry.WinTotal,   MKSFMTD(8, 5, 0)).
						Tab().Cat(result_entry.LossTotal,  MKSFMTD(8, 5, 0));
					f_out.WriteLine(msg_buf.CR());
					f_stake_out.WriteLine(msg_buf.CR());
				}
				{
					msg_buf.Z().Cat(rTs.GetSymb()).
						Tab().Cat(getcurdatetime_(), DATF_ISO8601|DATF_CENTURY, 0).
						//Tab().Cat(rP.TrendGe, MKSFMTD(10, 7, 0)).
						Tab().Cat(rP.MaxDuckQuant).
						Tab().Cat(result_entry.WinCount).
						Tab().Cat(result_entry.LossCount).
						Tab().Cat(result_entry.WinTotal,   MKSFMTD(8, 5, 0)).
						Tab().Cat(result_entry.LossTotal,  MKSFMTD(8, 5, 0));
					f_result_out.WriteLine(msg_buf.CR());
				}
			}
		}
	}
	CATCHZOK
	ASSIGN_PTR(ppAnn, p_ann);
	return ok;
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
		PrcssrTsStrategyAnalyzeFilt * p_filt = (PrcssrTsStrategyAnalyzeFilt *)pBaseFilt;
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
	PrcssrTsStrategyAnalyzeFilt * p_filt = (PrcssrTsStrategyAnalyzeFilt *)pBaseFilt;
	DIALOG_PROC_BODY(PrcssrTssaDialog, p_filt);
}

int SLAPI PrcssrTsStrategyAnalyze::Init(const PPBaseFilt * pBaseFilt)
{
	int    ok = 1;
	THROW(P.IsA(pBaseFilt));
	P = *(PrcssrTsStrategyAnalyzeFilt *)pBaseFilt;
	CATCHZOK
	return ok;
}

int SLAPI PrcssrTsStrategyAnalyze::Run()
{
	int    ok = 1;
	PPObjTimeSeries ts_obj;
	SString temp_buf;
	SString symb_buf;
	SString save_file_name;
	SString msg_buf;
	PPIDArray id_pre_list;
	PPTimeSeries ts_rec;
	TSVector <PPObjTimeSeries::QuoteReqEntry> quote_req_list;
	ts_obj.LoadQuoteReqList(quote_req_list);
	if(P.TsList.GetCount()) {
		P.TsList.Get(id_pre_list);
	}
	else {
		for(SEnum en = ts_obj.Enum(0); en.Next(&ts_rec) > 0;) {
			id_pre_list.add(ts_rec.ID);
		}
	}
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
				THROW_SL(id_list.add(((TempEntry *)temp_list.at(i2))->ID));
			}
		}
		else
			id_list = id_pre_list;
		RealArray trend_list;
		RealArray temp_trendinc_list;
		RealArray temp_result_list;
		TSVector <StrategyOptEntry> so_list;
		SString out_file_name;
		PPGetFilePath(PPPATH_OUT, "AnalyzeTsStrategy2.txt", out_file_name);
		const int is_out_file_exists = fileExists(out_file_name);
		SFile f_out(out_file_name, SFile::mAppend);
		if(!is_out_file_exists) {
			msg_buf.Z().Cat(getcurdatetime_(), DATF_YMD|DATF_CENTURY, TIMF_HMS);
			f_out.WriteLine(msg_buf.CR());
			msg_buf.Z().
				Cat("symbol").Tab().
				Cat("spike-quant").Tab().
				Cat("max-duck").Tab().
				Cat("input-frame-size").Tab().
				Cat("result").Tab().
				Cat("stake-count").Tab().
				Cat("win-count-rel").Tab().
				Cat("total-tm-count").Tab().
				Cat("total-tm-sec").Tab().
				Cat("result-per-day").Tab().
				Cat("opt-delta-freq").Tab().
				Cat("opt-delta-result").Tab().
				Cat("opt-delta2-freq").Tab().
				Cat("opt-delta2-result").Tab();
			f_out.WriteLine(msg_buf.CR());
		}
		for(uint i = 0; i < id_list.getCount(); i++) {
			const PPID id = id_list.get(i);
			if(ts_obj.Search(id, &ts_rec) > 0) {
				STimeSeries ts;
				if(ts_obj.GetTimeSeries(id, ts) > 0) {
					const uint tsc = ts.GetCount();
					static const uint input_frame_size_list[] = { /*100,*/ 500, 1000, 1500, 2000, 2500 };
					STimeSeries::Stat st(0);
					PPObjTimeSeries::MaxDuckToResultRelation opt_max_duck;
					const PPObjTimeSeries::QuoteReqEntry * p_req_entry = 0;
					for(uint qreidx = 0; !p_req_entry && qreidx < quote_req_list.getCount(); qreidx++) {
						const PPObjTimeSeries::QuoteReqEntry & r_entry = quote_req_list.at(qreidx);
						if(sstreqi_ascii(r_entry.Ticker, ts.GetSymb()))
							p_req_entry = &r_entry;
					}
					const  int is_short_permitted = p_req_entry ? BIN(p_req_entry->Flags & p_req_entry->fAllowShort) : 0;
					assert(oneof2(is_short_permitted, 0, 1));
					uint   vec_idx = 0;
					THROW_SL(ts.GetValueVecIndex("close", &vec_idx));
					ts.Analyze("close", st);
					if(ts_rec.SpikeQuant <= 0.0 || (P.Flags & P.fForce)) {
						ts_rec.SpikeQuant = st.DeltaAvg / 2.0;
						PPID   temp_id = id;
						THROW(ts_obj.PutPacket(&temp_id, &ts_rec, 1));
					}
					const double spike_quant = ts_rec.SpikeQuant;
					LDATETIME last_val_dtm = ZERODATETIME;
					{
						SUniTime utm;
						ts.GetTime(tsc-1, &utm);
						utm.Get(last_val_dtm);
					}
					//
					// stake_side: 0 - long, 1 - short
					// 
					for(int stake_side = 0; stake_side < (is_short_permitted+1); stake_side++) {
						if((stake_side == 0 && P.Flags & P.fProcessLong) || (stake_side == 1 && P.Flags & P.fProcessShort)) {
							uint32 org_opt_max_duck_val = 0;
							if(stake_side == 0) {
								org_opt_max_duck_val = ts_rec.OptMaxDuck;
							}
							else if(stake_side == 1) {
								org_opt_max_duck_val = ts_rec.OptMaxDuck_S;
							}
							uint32 cur_opt_max_duck_val = org_opt_max_duck_val;
							if(P.Flags & P.fFindOptMaxDuck && (org_opt_max_duck_val <= 0 || (P.Flags & P.fForce))) {
								PPObjTimeSeries::TrainNnParam tnnp(ts_rec, PPObjTimeSeries::TrainNnParam::afAnalyzeFrame);
								tnnp.SpikeQuant = spike_quant;
								tnnp.TargetQuant = 0;
								tnnp.EpochCount = 1;
								tnnp.InputFrameSize = 0;
								tnnp.MaxDuckQuant = 0;
								tnnp.StakeThreshold = 0.05;
								assert(oneof2(P.CloseMode, tnnp.clsmodFullMaxDuck, tnnp.clsmodAdjustLoss));
								tnnp.StakeCloseMode = (uint16)P.CloseMode;
								SETFLAG(tnnp.BaseFlags, tnnp.bfShort, stake_side == 1);
								IntRange md_range;
								md_range.Set(50, 500);
								TSVector <PPObjTimeSeries::MaxDuckToResultRelation> opt_max_duck_set;
								THROW(ts_obj.FindOptimalMaxDuck(ts, vec_idx, tnnp, md_range, 50, &opt_max_duck_set, opt_max_duck));
								cur_opt_max_duck_val = opt_max_duck.MaxDuckQuant;
								{
									PPID   temp_id = id;
									PPTimeSeries ts_rec_to_upd;
									THROW(ts_obj.GetPacket(temp_id, &ts_rec_to_upd) > 0);
									if(stake_side == 0) {
										ts_rec_to_upd.OptMaxDuck = cur_opt_max_duck_val;
									}
									else if(stake_side == 1) {
										ts_rec_to_upd.OptMaxDuck_S = cur_opt_max_duck_val;
									}
									THROW(ts_obj.PutPacket(&temp_id, &ts_rec_to_upd, 1));
								}
							}
							if(P.Flags & P.fFindStrategies) {
								PPObjTimeSeries::StrategyContainer scontainer;
								scontainer.SetLastValTm(last_val_dtm);
								if(P.Flags & P.fForce) {
									THROW(ts_obj.PutStrategies(id, 0, 1));
								}
								for(uint ifsidx = 0; ifsidx < SIZEOFARRAY(input_frame_size_list); ifsidx++) {
									const uint input_frame_size = input_frame_size_list[ifsidx];
									trend_list.clear();
									PROFILE_START
									{
										STimeSeries::AnalyzeFitParam afp(input_frame_size, 0, 0);
										THROW(ts.AnalyzeFit("close", afp, &trend_list, 0));
										assert(trend_list.getCount() == tsc);
									}
									PROFILE_END
									uint __max_duck_quant_list[64];
									uint __max_duck_quant_count = 0;
									if(cur_opt_max_duck_val > 0 && cur_opt_max_duck_val < 1000) {
										const uint gr_count = 7;
										uint gr = cur_opt_max_duck_val / gr_count;
										for(uint i = 0; i < gr_count; i++) {
											__max_duck_quant_list[__max_duck_quant_count++] = cur_opt_max_duck_val - (gr * i);
										}
									}
									else {
										__max_duck_quant_list[__max_duck_quant_count++] = 50;
										__max_duck_quant_list[__max_duck_quant_count++] = 60;
										__max_duck_quant_list[__max_duck_quant_count++] = 70;
										__max_duck_quant_list[__max_duck_quant_count++] = 80;
										__max_duck_quant_list[__max_duck_quant_count++] = 90;
										__max_duck_quant_list[__max_duck_quant_count++] = 100;
										__max_duck_quant_list[__max_duck_quant_count++] = 150;
									}
									for(uint mdidx = 0; mdidx < __max_duck_quant_count; mdidx++) {
										PPObjTimeSeries::TrainNnParam tnnp(ts_rec, PPObjTimeSeries::TrainNnParam::afAnalyzeFrame);
										symb_buf = ts_rec.Symb;
										if(stake_side == 1)
											symb_buf.CatChar('-').Cat("REV");
										tnnp.SpikeQuant = spike_quant;
										tnnp.TargetQuant = 0;
										tnnp.EpochCount = 1;
										tnnp.InputFrameSize = input_frame_size;
										tnnp.MaxDuckQuant = __max_duck_quant_list[mdidx];
										tnnp.StakeThreshold = 0.05;
										assert(oneof2(P.CloseMode, tnnp.clsmodFullMaxDuck, tnnp.clsmodAdjustLoss));
										tnnp.StakeCloseMode = (uint16)P.CloseMode;
										SETFLAG(tnnp.BaseFlags, tnnp.bfShort, stake_side == 1);
										{
											PPObjTimeSeries::StrategyResultEntry sre(tnnp, 0);
											THROW(ts_obj.TestStrategy(ts, vec_idx, trend_list, tnnp, sre));
											THROW_SL(scontainer.insert(dynamic_cast <PPObjTimeSeries::Strategy *>(&sre)));
											{
												msg_buf.Z().
													Cat(symb_buf).Space().Space().Tab().
													Cat(sre.SpikeQuant, MKSFMTD(15, 8, 0)).Tab().
													Cat(sre.MaxDuckQuant).Tab().
													Cat(sre.InputFrameSize).Tab().
													Cat(sre.V.Result, MKSFMTD(15, 5, 0)).Tab().
													Cat(sre.StakeCount).Tab().
													Cat((double)sre.WinCount / (double)sre.StakeCount, MKSFMTD(15, 5, 0)).Tab().
													Cat(sre.V.TmCount).Tab().
													Cat(sre.V.TmSec).Tab().
													Cat(sre.V.GetResultPerDay()).Tab().
													//Cat(opt_delta_range.low, MKSFMTD(15, 10, 0)).Dot().Dot().Cat(opt_delta_range.upp, MKSFMTD(0, 10, 0)).Tab().
													Cat(sre.OptDeltaCount).CatChar('/').Cat(sre.LastResultIdx-sre.InputFrameSize+1).Tab().
													Cat(sre.OptDeltaResult, MKSFMTD(15, 5, 0)).Tab().
													//Cat(opt_delta2_range.low, MKSFMTD(15, 10, 0)).Dot().Dot().Cat(opt_delta2_range.upp, MKSFMTD(0, 10, 0)).Tab().
													Cat(sre.OptDelta2Stride).CatChar('/').Cat(sre.OptDelta2Count).CatChar('/').Cat(sre.LastResultIdx-sre.InputFrameSize+1).Tab().
													Cat(sre.OptDelta2Result, MKSFMTD(15, 5, 0)).Tab();
												f_out.WriteLine(msg_buf.CR());
												f_out.Flush();
												//
												tnnp.OptDeltaRange = sre.OptDeltaRange;
												tnnp.OptDelta2Range =  sre.OptDelta2Range;
												tnnp.OptDelta2Stride = sre.OptDelta2Stride;
											}
										}
										{
											PPObjTimeSeries::StrategyResultEntry sre_test(tnnp, 1);
											THROW(ts_obj.TestStrategy(ts, vec_idx, trend_list, tnnp, sre_test));
											THROW_SL(scontainer.insert(dynamic_cast <PPObjTimeSeries::Strategy *>(&sre_test)));
											{
												msg_buf.Z().
													Cat(symb_buf).CatChar('/').Cat("1").Tab().
													Cat(sre_test.SpikeQuant, MKSFMTD(15, 8, 0)).Tab().
													Cat(sre_test.MaxDuckQuant).Tab().
													Cat(sre_test.InputFrameSize).Tab().
													Cat(sre_test.V.Result, MKSFMTD(15, 5, 0)).Tab().
													Cat(sre_test.StakeCount).Tab().
													Cat((double)sre_test.WinCount / (double)sre_test.StakeCount, MKSFMTD(15, 5, 0)).Tab().
													Cat(sre_test.V.TmCount).Tab().
													Cat(sre_test.V.TmSec).Tab().
													Cat(sre_test.V.GetResultPerDay()).Tab().
													//Cat(opt_delta_range.low, MKSFMTD(15, 10, 0)).Dot().Dot().Cat(opt_delta_range.upp, MKSFMTD(0, 10, 0)).Tab().
													//Cat(sre_test.OptDeltaCount).CatChar('/').Cat(sre_test.LastResultIdx-sre_test.InputFrameSize+1).Tab().
													//Cat(sre_test.OptDeltaResult, MKSFMTD(15, 5, 0)).Tab().
													Cat("").Tab().
													Cat("").Tab().
													//Cat(opt_delta2_range.low, MKSFMTD(15, 10, 0)).Dot().Dot().Cat(opt_delta2_range.upp, MKSFMTD(0, 10, 0)).Tab().
													//Cat(sre_test.OptDelta2Stride).CatChar('/').Cat(sre_test.OptDelta2Count).CatChar('/').Cat(sre_test.LastResultIdx-sre_test.InputFrameSize+1).Tab().
													//Cat(sre_test.OptDelta2Result, MKSFMTD(15, 5, 0)).Tab();
													Cat("").Tab().
													Cat("").Tab();
												f_out.WriteLine(msg_buf.CR());
												f_out.Flush();
											}
										}
										{
											PPObjTimeSeries::StrategyResultEntry sre_test(tnnp, 2);
											THROW(ts_obj.TestStrategy(ts, vec_idx, trend_list, tnnp, sre_test));
											THROW_SL(scontainer.insert(dynamic_cast <PPObjTimeSeries::Strategy *>(&sre_test)));
											{
												msg_buf.Z().
													Cat(symb_buf).CatChar('/').Cat("2").Tab().
													Cat(sre_test.SpikeQuant, MKSFMTD(15, 8, 0)).Tab().
													Cat(sre_test.MaxDuckQuant).Tab().
													Cat(sre_test.InputFrameSize).Tab().
													Cat(sre_test.V.Result, MKSFMTD(15, 5, 0)).Tab().
													Cat(sre_test.StakeCount).Tab().
													Cat((double)sre_test.WinCount / (double)sre_test.StakeCount, MKSFMTD(15, 5, 0)).Tab().
													Cat(sre_test.V.TmCount).Tab().
													Cat(sre_test.V.TmSec).Tab().
													Cat(sre_test.V.GetResultPerDay()).Tab().
													//Cat(opt_delta_range.low, MKSFMTD(15, 10, 0)).Dot().Dot().Cat(opt_delta_range.upp, MKSFMTD(0, 10, 0)).Tab().
													//Cat(sre_test.OptDeltaCount).CatChar('/').Cat(sre_test.LastResultIdx-sre_test.InputFrameSize+1).Tab().
													//Cat(sre_test.OptDeltaResult, MKSFMTD(15, 5, 0)).Tab().
													Cat("").Tab().
													Cat("").Tab().
													//Cat(opt_delta2_range.low, MKSFMTD(15, 10, 0)).Dot().Dot().Cat(opt_delta2_range.upp, MKSFMTD(0, 10, 0)).Tab().
													//Cat(sre_test.OptDelta2Stride).CatChar('/').Cat(sre_test.OptDelta2Count).CatChar('/').Cat(sre_test.LastResultIdx-sre_test.InputFrameSize+1).Tab().
													//Cat(sre_test.OptDelta2Result, MKSFMTD(15, 5, 0)).Tab();
													Cat("").Tab().
													Cat("").Tab();
												f_out.WriteLine(msg_buf.CR());
												f_out.Flush();
											}
										}
									}
								}
								THROW(ts_obj.PutStrategies(id, &scontainer, 1));
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
