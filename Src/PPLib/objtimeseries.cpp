// OBJTIMESERIES.CPP
// Copyright (c) A.Sobolev 2018
// @codepage UTF-8
// Модуль, управляющий объектом данных PPObjTimeSeries
//
#include <pp.h>
#pragma hdrstop
#include <fann.h>

SLAPI PPTimeSeries::PPTimeSeries() : Tag(PPOBJ_TIMESERIES), ID(0), Flags(0)
{
	PTR32(Name)[0] = 0;
	PTR32(Symb)[0] = 0;
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
							out_buf.Cat((stat.State & stat.stSorted) ? "sorted" : "unsorted").Space().CRB();
							out_buf.Cat((stat.State & stat.stHasTmDup) ? "has dup" : "no dup").Space().CRB();
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
				p_obj->AnalyzeTsTradeFrames();
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
	StatBase max_duck_stat;
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

int SLAPI PPObjTimeSeries::AnalyzeTsTradeFrames()
{
	int    ok = 1;
	PPIDArray id_list;
	PPTimeSeries ts_rec;
	for(SEnum en = Enum(0); en.Next(&ts_rec) > 0;) {
		id_list.add(ts_rec.ID);
	}
	if(id_list.getCount()) {
		id_list.sortAndUndup();
		SString out_buf;
		SString temp_buf;
		PPGetFilePath(PPPATH_OUT, "AnalyzeTsTradeFrames.txt", temp_buf);
		SFile f_out(temp_buf, SFile::mWrite);
		if(f_out.IsValid()) {
			out_buf.Z().Cat("symb").Tab().Cat("frame").Tab().Cat("target").Tab().Cat("freq").Tab().Cat("max_duck_avg").Tab().Cat("profile").CR();
			f_out.WriteLine(out_buf);
			for(uint i = 0; i < id_list.getCount(); i++) {
				const PPID id = id_list.get(i);
				if(Search(id, &ts_rec) > 0) {
					STimeSeries ts;
					if(GetTimeSeries(id, ts) > 0) {
						static const uint frame_size_list[] = { 5, 10, 15, 20 };
						static const double target_list[] = { 1.0005, 1.001, 1.0025, 1.005, 1.0075, 1.01 };
						for(uint fi = 0; fi < SIZEOFARRAY(frame_size_list); fi++) {
							const uint frame_size = frame_size_list[fi];
							for(uint ti = 0; ti < SIZEOFARRAY(target_list); ti++) {
								const double target = target_list[ti];
								double freq = 0.0;
								double max_duck_avg = 0.0;
								SProfile::Measure pm;
								if(AnalyzeTsTradeFrame(ts, frame_size, target, &freq, &max_duck_avg) > 0) {
									uint64 pt = pm.Get();
									out_buf.Z().Cat(ts_rec.Symb).Tab().Cat(frame_size).Tab().Cat(target, MKSFMTD(0, 4, 0)).Tab().
										Cat(freq, MKSFMTD(0, 8, 0)).Tab().Cat((1.0 - max_duck_avg), MKSFMTD(0, 8, 0)).Tab().Cat(pt).CR();
									f_out.WriteLine(out_buf);
								}
							}
						}
					}
				}
			}
		}
	}
	return ok;
}

int SLAPI PPObjTimeSeries::TrainNN(const STimeSeries & rTs, uint frameSize, double target, double maxDuck)
{
	const uint tsc = rTs.GetCount();
	float nn_inp[500];
	float nn_outp[3];
	const uint input_frame_size = SIZEOFARRAY(nn_inp);
	const long input_dim = (long)input_frame_size;
	const long outp_dim = SIZEOFARRAY(nn_outp);

	int    ok = 1;
	Fann * p_ann = 0;
	if(tsc > (input_frame_size + frameSize)) {
		LongArray _layers;
		RealArray inp_frame;
		RealArray result_frame;
		_layers.addzlist(input_dim, input_dim * 3, outp_dim, 0);
		//p_ann = fann_create_standard_array(/*SIZEOFARRAY(layers), layers*/_layers);
		p_ann = new Fann(Fann::FANN_NETTYPE_LAYER, 1, _layers);
		p_ann->SetTrainingAlgorithm(Fann::FANN_TRAIN_BATCH);
		p_ann->SetLearningRate(0.06f);	
		for(uint fs = input_frame_size; fs < (tsc - input_frame_size - frameSize); fs++) {
			rTs.GetFrame("close", fs-input_frame_size, input_frame_size, STimeSeries::nfOne|STimeSeries::nfBaseAvg, inp_frame);
			if(inp_frame.getCount() == input_frame_size) {
				rTs.GetFrame("close", fs, frameSize, STimeSeries::nfOne|STimeSeries::nfBaseStart, result_frame);
				if(result_frame.getCount() == frameSize) {
					int   target_reached = 0;
					int   max_duck_reached = 0;
					for(uint i = 0; !target_reached && !max_duck_reached && i < result_frame.getCount(); i++) {
						const double fv = result_frame.at(i);
						if(fv > target)
							target_reached = 1;
						else if(fv < maxDuck)
							max_duck_reached = 1;
					}
					if(target_reached) {
						nn_outp[0] = 1.0f;
						nn_outp[1] = 0.0f;
						nn_outp[2] = 0.0f;
					}
					else if(max_duck_reached) {
						nn_outp[0] = 0.0f;
						nn_outp[1] = 1.0f;
						nn_outp[2] = 0.0f;
					}
					else {
						nn_outp[0] = 0.0f;
						nn_outp[1] = 0.0f;
						nn_outp[2] = 1.0f;
					}
					p_ann->Train(nn_inp, nn_outp);
				}
			}
		}
	}
	delete p_ann;
	return ok;
}
