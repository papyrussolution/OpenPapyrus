// PPBulletinBoard.cpp
// Copyright (c) A.Sobolev 2018, 2019
// @codepage UTF-8
// Концепт доски объявлений, реализующей функционал общедоступных системных задач
//
#include <pp.h>
#pragma hdrstop

// @construction {

class PPBulletinBoard {
public:
	class Sticker {
	public:
		friend class PPBulletinBoard;

		SLAPI  Sticker();
		virtual SLAPI ~Sticker();

		enum {
			fPinned = 0x0001
		};
		long   Flags;  // @flags
		LDATETIME Dtm; // Момент создания 
		LDATETIME SuspendTill; // Время, до которого отложена некоторая работа.
		SString Symb;
	protected:
		virtual Sticker * SLAPI Dup() const;
	private:
		uint   ID;
	};

	SLAPI  PPBulletinBoard();
	uint   SLAPI GetCount();
	//
	// Descr: Находит стикер по идентификатору
	// Note: В случае успешного поиска возвращается указатель на копию найденного
	//   стикера, которая должна быть разрушена после использования.
	// Returns:
	//   0 - стикер по идентификатору не найден
	//  !0 - КОПИЯ найденного стикера
	//
	Sticker * SLAPI SearchStickerByID(uint id);
	Sticker * SLAPI SearchStickerBySymb(const char * pSymb);
	int    SLAPI PutSticker(Sticker * pNewSticker, uint * pId);
	int    SLAPI RemoveStickerByID(uint id);
private:
	uint   LastId;
	TSCollection <Sticker> StL;
	ReadWriteLock RwL;
};

SLAPI PPBulletinBoard::Sticker::Sticker() : ID(0), Flags(0), Dtm(getcurdatetime_()), SuspendTill(ZERODATETIME)
{
}

SLAPI PPBulletinBoard::Sticker::~Sticker()
{
}

//virtual 
PPBulletinBoard::Sticker * SLAPI PPBulletinBoard::Sticker::Dup() const
{
	Sticker * p_new_item = new Sticker;
	if(p_new_item) {
		p_new_item->ID = ID;
		p_new_item->Flags = Flags;
		p_new_item->Dtm = Dtm;
		p_new_item->SuspendTill = SuspendTill;
		p_new_item->Symb = Symb;
	}
	return p_new_item;
}

SLAPI PPBulletinBoard::PPBulletinBoard() : LastId(0)
{
}

int SLAPI PPBulletinBoard::PutSticker(Sticker * pNewSticker, uint * pId)
{
	int    ok = 0;
	uint   new_id = 0;
	if(pNewSticker) {
		SRWLOCKER(RwL, SReadWriteLocker::Write);
		new_id = ++LastId;
		pNewSticker->ID = new_id;
		StL.insert(pNewSticker);
		ASSIGN_PTR(pId, new_id);
		ok = 1;
	}
	return ok;
}

int SLAPI PPBulletinBoard::RemoveStickerByID(uint id)
{
	int    ok = -1;
	if(id) {
		SRWLOCKER(RwL, SReadWriteLocker::Read);
		const uint c = StL.getCount();
		for(uint i = 0; i < c; i++) {
			const Sticker * p_st = StL.at(i);
			if(p_st && p_st->ID == id) {
				SRWLOCKER_TOGGLE(SReadWriteLocker::Write);
				StL.atFree(i);
				ok = 1;
				break;
			}
		}
	}
	return ok;
}

uint SLAPI PPBulletinBoard::GetCount()
{
	uint   c = 0;
	{
		SRWLOCKER(RwL, SReadWriteLocker::Read);
		c = StL.getCount();
	}
	return c;
}

PPBulletinBoard::Sticker * SLAPI PPBulletinBoard::SearchStickerByID(uint id)
{
	Sticker * p_result = 0;
	if(id) {
		SRWLOCKER(RwL, SReadWriteLocker::Read);
		const uint c = StL.getCount();
		for(uint i = 0; !p_result && i < c; i++) {
			const Sticker * p_st = StL.at(i);
			if(p_st && p_st->ID == id)
				p_result = p_st->Dup();
		}
	}
	return p_result;
}

PPBulletinBoard::Sticker * SLAPI PPBulletinBoard::SearchStickerBySymb(const char * pSymb)
{
	Sticker * p_result = 0;
	if(!isempty(pSymb)) {
		SRWLOCKER(RwL, SReadWriteLocker::Read);
		const uint c = StL.getCount();
		for(uint i = 0; !p_result && i < c; i++) {
			const Sticker * p_st = StL.at(i);
			if(p_st && p_st->Symb == pSymb)
				p_result = p_st->Dup();
		}
	}
	return p_result;
}
//
//
//
class TimeSeriesCache : public ObjCache {
public:
	struct Data : public ObjCacheEntry {
		long   Flags;
		double BuyMarg;
		double SellMarg;
		double SpikeQuant;     //
		double AvgSpread;      //
		uint32 OptMaxDuck;     // Оптимальная глубина проседания (в квантах) при длинной ставке
		uint32 OptMaxDuck_S;   // Оптимальная глубина проседания (в квантах) при короткой ставке
		int16  Prec;
		uint16 Reserve; // @alignment
	};
	struct TrendEntry {
		TrendEntry(uint stride, uint nominalCount) : Stride(stride), NominalCount(nominalCount)
		{
			assert(stride > 0 && stride <= 100000);
			assert(NominalCount > 0 && NominalCount <= 100);
		}
		const uint Stride;
		const uint NominalCount;
		RealArray TL;
	};
	struct TimeSeriesBlock {
		SLAPI  TimeSeriesBlock() : Flags(0), SpreadSum(0), SpreadCount(0), VolumeMin(0.0), VolumeMax(0.0), VolumeStep(0.0)
		{
		}
		enum {
			fDirty = 0x0001
		};
		long   Flags;
		// Следующие 2 поля нужны для расчета среднего спреда
		uint   SpreadSum; // Накопленная сумма величин спредов
		uint   SpreadCount; // Количество накопленных величин спредов
		double VolumeMin;
		double VolumeMax;
		double VolumeStep;
		PPTimeSeries PPTS;
		STimeSeries T_;
		//
		// Descr: Структура, используемая как буфер для управления последним значением
		//   в последовательности T_. Если самое актуальное значение пришло с тиком (TsStakeEnvironment::Tick), 
		//   то регулярное последнее значение (если оно старше тика) сбрасывается в TempLastEntry с тегом 1, а
		//   вместо него в T_ заносится тик.
		//   При получении регулярной последовательности, если последним элементом в T_ был тик, то он
		//   переносится в TempLastEntry с тегом 2.
		//
		struct TempLastEntry {
			TempLastEntry() : Tag(0)
			{
			}
			int    Tag; // 0 - empty, 1 - main series value, 2 - tick value
			STimeSeries::SnapshotEntry Sse;
		};
		TempLastEntry TLE;
		TSCollection <TrendEntry> TrendList;
		PPObjTimeSeries::StrategyContainer Strategies;
		const TrendEntry * FASTCALL SearchTrendEntry(uint stride) const
		{
			const TrendEntry * p_result = 0;
			for(uint i = 0; !p_result && i < TrendList.getCount(); i++) {
				const TrendEntry * p_te = TrendList.at(i);
				if(p_te && p_te->Stride == stride)
					p_result = p_te;
			}
			return p_result;
		}
		void Make_T_Regular()
		{
			if(TLE.Tag == 1) { // Если TLE.Tag != 1, то все значения в T_ и так регулярные
				const uint tsc = T_.GetCount();
				assert(tsc > 0);
				STimeSeries::SnapshotEntry actual_entry;
				T_.GetSnapshotEntry(tsc-1, actual_entry);
				T_.SetSnapshotEntry(tsc-1, TLE.Sse);
				int   utcq = 0;
				if(actual_entry.Tm.Compare(TLE.Sse.Tm, &utcq) > 0) {
					TLE.Sse = actual_entry;
					TLE.Tag = 2;
				}
				else
					TLE.Tag = 0; // Последнее регулярное значение новее "актуального" - считаем буфер пустым
			}
		}
		void Make_T_Actual()
		{
			if(TLE.Tag == 2) { 
				const uint tsc = T_.GetCount();
				if(tsc) { // Если временная серия пустая, то делать нам вообще нЕчего!
					STimeSeries::SnapshotEntry regular_entry;
					T_.GetSnapshotEntry(tsc-1, regular_entry);
					int   utcq = 0;
					if(regular_entry.Tm.Compare(TLE.Sse.Tm, &utcq) < 0) {
						T_.SetSnapshotEntry(tsc-1, TLE.Sse);
						TLE.Sse = regular_entry;
						TLE.Tag = 1;
					}
					else
						TLE.Tag = 0; // Последнее значение новее "актуального" - ничего делать не надо кроме как пометить буфер TLE как пустой
				}
			}
		}
	};
	static int OnQuartz(int kind, const PPNotifyEvent * pEv, void * procExtPtr)
	{
		TimeSeriesCache * p_this = (TimeSeriesCache *)procExtPtr;
		if(p_this) {
			LDATETIME cdtm = getcurdatetime_();
			long sec = diffdatetimesec(cdtm, p_this->LastFlashDtm);
			if(sec >= 600) {
				p_this->Flash();
			}
		}
		return 1;
	}
	TimeSeriesCache() : ObjCache(PPOBJ_TIMESERIES, sizeof(Data)), LastFlashDtm(getcurdatetime_()), P_ReqQList(0)
	{
		{
			long   cookie = 0;
			PPAdviseBlock adv_blk;
			MEMSZERO(adv_blk);
			adv_blk.Kind = PPAdviseBlock::evQuartz;
			adv_blk.DbPathID = DBS.GetDbPathID();
			adv_blk.ObjType = 0;
			adv_blk.Proc = TimeSeriesCache::OnQuartz;
			adv_blk.ProcExtPtr = this;
			DS.Advise(&cookie, &adv_blk);
		}
	}
	~TimeSeriesCache()
	{
		delete P_ReqQList;
	}
	int    SLAPI SetTimeSeries(STimeSeries & rTs)
	{
		int    ok = -1;
		STimeSeries::AppendStat apst;
		OpL.Lock();
		SString temp_buf = rTs.GetSymb();
		if(temp_buf.NotEmpty()) {
			TimeSeriesBlock * p_fblk = SearchBlockBySymb(temp_buf, 0);
			THROW(SETIFZ(p_fblk, InitBlock(temp_buf)));
			{
				int    local_ok = 1;
				rTs.Sort();
				p_fblk->Make_T_Regular();
				local_ok = p_fblk->T_.AddItems(rTs, &apst);
				p_fblk->Make_T_Actual();
				EvaluateTrends(p_fblk, 0);
				THROW(local_ok);
				p_fblk->SpreadSum += apst.SpreadSum;
				p_fblk->SpreadCount += apst.SpreadCount;
				p_fblk->Flags |= p_fblk->fDirty;
			}
			if(apst.AppendCount || apst.UpdCount) {
				//PPTXT_SETTIMESERIESSTAT             "SetTimeSeries @zstr: append_count=@int upd-count=@int field-count=@int profile=@int64"
				SString fmt_buf;
				PPLoadText(PPTXT_SETTIMESERIESSTAT, fmt_buf);
				PPFormat(fmt_buf, &temp_buf, (const char *)rTs.GetSymb(), (int)apst.AppendCount, (int)apst.UpdCount, (int)apst.IntersectFldsCount, (int64)apst.TmProfile);
				/*temp_buf.Z().Cat("SetTimeSeries").CatDiv(':', 2).Cat(rTs.GetSymb()).Space().CatEq("append-count", apst.AppendCount).Space().
					CatEq("update-count", apst.UpdCount).Space().CatEq("field-count", apst.IntersectFldsCount).Space().CatEq("profile", apst.TmProfile);*/
				PPLogMessage(PPFILNAM_INFO_LOG, temp_buf, LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_DBINFO);
			}
		}
		CATCHZOK
		OpL.Unlock();
		return ok;
	}
	int    SLAPI GetReqQuotes(TSVector <PPObjTimeSeries::QuoteReqEntry> & rList)
	{
		int    ok = -1;
		PPObjTimeSeries ts_obj;
		OpL.Lock();
		if(P_ReqQList) {
			rList = *P_ReqQList;
		}
		else {
			P_ReqQList = new TSVector <PPObjTimeSeries::QuoteReqEntry>;
			ok = P_ReqQList ? ts_obj.LoadQuoteReqList(*P_ReqQList) : PPSetErrorSLib();
			if(ok > 0) {
				rList = *P_ReqQList;
			}
		}
		for(uint i = 0; i < rList.getCount(); i++) {
			PPObjTimeSeries::QuoteReqEntry & r_entry = rList.at(i);
			SUniTime last_utm;
			TimeSeriesBlock * p_blk = SearchBlockBySymb(r_entry.Ticker, 0);
			SETIFZ(p_blk, InitBlock(r_entry.Ticker));
			const uint tc = p_blk ? p_blk->T_.GetCount() : 0;
			if(tc && p_blk->T_.GetTime(tc-1, &last_utm)) {
				last_utm.Get(r_entry.LastValTime);
			}
			else
				r_entry.LastValTime.Z();
		}
		OpL.Unlock();
		return ok;
	}
	int    SLAPI SetCurrentStakeEnvironment(const TsStakeEnvironment * pEnv, TsStakeEnvironment::StakeRequestBlock * pRet)
	{
		int    ok = 1;
		SString temp_buf;
		OpL.Lock();
		if(pEnv) {
			StkEnv = *pEnv;
			SUniTime ut;
			LDATETIME last_dtm;
			for(uint i = 0; i < StkEnv.TL.getCount(); i++) {
				uint vec_idx = 0;
				TsStakeEnvironment::Tick & r_tk = StkEnv.TL.at(i);
				StkEnv.GetS(r_tk.SymbP, temp_buf);
				TimeSeriesBlock * p_fblk = SearchBlockBySymb(temp_buf, 0);
				const uint tsc = p_fblk ? p_fblk->T_.GetCount() : 0;
				if(tsc && p_fblk->T_.GetTime(tsc-1, &ut) && ut.Get(last_dtm)) {
					if(cmp(last_dtm, r_tk.Dtm) < 0 && p_fblk->T_.GetValueVecIndex("close", &vec_idx)) {
						p_fblk->Make_T_Regular();
						assert(oneof2(p_fblk->TLE.Tag, 0, 2));
						p_fblk->TLE.Sse.Tm.Set(r_tk.Dtm);
						p_fblk->TLE.Sse.Values.clear();
						p_fblk->TLE.Sse.Values.Add(vec_idx, r_tk.Last);
						p_fblk->TLE.Tag = 2;
						p_fblk->Make_T_Actual();
						EvaluateTrends(p_fblk, 0);
					}
				}
			}
			if(pRet) {
				EvaluateStakes(*pRet);
			}
		}
		else {
			ok = -1;
			;//StkEnv
		}
		OpL.Unlock();
		// @debug {
		if(ok > 0)
			LogStateEnvironment(*pEnv); 
		// } @debug
		return ok;
	}
	int    SLAPI SetVolumeParams(const char * pSymb, double volumeMin, double volumeMax, double volumeStep)
	{
		int    ok = 1;
		uint   idx = 0;
		OpL.Lock();
		TimeSeriesBlock * p_blk = SearchBlockBySymb(pSymb, &idx);
		if(p_blk) {
			if(volumeMin > 0.0)
				p_blk->VolumeMin = volumeMin;
			if(volumeMax > 0.0)
				p_blk->VolumeMax = volumeMax;
			if(volumeStep > 0.0)
				p_blk->VolumeStep = volumeStep;
		}
		else
			ok = 0;
		OpL.Unlock();
		return ok;
	}
	int    SLAPI Flash()
	{
		int    ok = 1;
		OpL.Lock();
		if(TsC.getCount()) {
			STimeSeries org_ts;
			PPObjTimeSeries ts_obj;
			PPTransaction tra(1);
			THROW(tra);
			for(uint i = 0; i < TsC.getCount(); i++) {
				TimeSeriesBlock * p_blk = TsC.at(i);
				if(p_blk && p_blk->Flags & TimeSeriesBlock::fDirty && p_blk->PPTS.ID) {
					PPID   id = p_blk->PPTS.ID;
					PPTimeSeries rec;
					if(ts_obj.GetPacket(id, &rec) > 0) {
						if(p_blk->SpreadCount && p_blk->SpreadSum) {
							rec.AvgSpread = (double)p_blk->SpreadSum / (double)p_blk->SpreadCount;
						}
						THROW(ts_obj.PutPacket(&id, &rec, 0));
						{
							int    local_ok = 1;
							p_blk->Make_T_Regular();
							if(ts_obj.GetTimeSeries(id, org_ts) > 0) {
								STimeSeries::AppendStat apst;
								local_ok = org_ts.AddItems(p_blk->T_, &apst);
								if(local_ok && (apst.AppendCount || apst.UpdCount)) {
									local_ok = ts_obj.SetTimeSeries(id, &org_ts, 0);
								}
							}
							else {
								local_ok = ts_obj.SetTimeSeries(id, &p_blk->T_, 0);
							}
							p_blk->Make_T_Actual();
							THROW(local_ok);
						}
						p_blk->PPTS = rec;
						p_blk->Flags &= ~TimeSeriesBlock::fDirty;
					}
				}
			}
			THROW(tra.Commit());
			LastFlashDtm = getcurdatetime_();
		}
		CATCHZOK
		OpL.Unlock();
		return ok;
	}
	int    SLAPI FindOptimalStrategyAtStake(const TimeSeriesBlock & rBlk, const TsStakeEnvironment::Stake & rStk, TsStakeEnvironment::StakeRequestBlock & rResult) const
	{
		int    ok = -1;
		const TsStakeEnvironment::Tick * p_tk = StkEnv.SearchTickBySymb(rBlk.T_.GetSymb());
		if(p_tk && rBlk.T_.GetCount()) {
			double max_result = 0.0;
			int    max_result_sidx = -1;
			for(uint sidx = 0; sidx < rBlk.Strategies.getCount(); sidx++) {
				const PPObjTimeSeries::Strategy & r_s = rBlk.Strategies.at(sidx);
				if((rStk.Type == TsStakeEnvironment::ordtBuy && !(r_s.BaseFlags & r_s.bfShort)) || (rStk.Type == TsStakeEnvironment::ordtSell && (r_s.BaseFlags & r_s.bfShort))) {
					double cr = 0.0;
					int    is_result = 0;
					if(r_s.StakeMode) {
						const TrendEntry * p_te = rBlk.SearchTrendEntry(r_s.InputFrameSize);
						const uint tlc = p_te ? p_te->TL.getCount() : 0;
						if(tlc) {
							if(r_s.StakeMode == 1) {
								const double t = p_te->TL.at(tlc-1);
								if(t >= r_s.OptDeltaRange.low && t <= r_s.OptDeltaRange.upp) {
									cr = r_s.V.GetResultPerDay();
									is_result = 1;
								}
							}
							else if(r_s.StakeMode == 2) {
								if(tlc >= r_s.OptDelta2Stride) {
									const double t = p_te->TL.StrideDifference(tlc-1, r_s.OptDelta2Stride);
									if(t >= r_s.OptDelta2Range.low && t <= r_s.OptDelta2Range.upp) {
										cr = r_s.V.GetResultPerDay();
										is_result = 1;
									}
								}
							}
						}
					}
					else {
						cr = r_s.V.GetResultPerDay();
						is_result = 1;
					}
					if(is_result && max_result < cr) {
						max_result = cr;
						max_result_sidx = (int)sidx;
					}
				}
			}
			if(max_result_sidx >= 0) {
				uint   vec_idx = 0;
				const  int  gvvir = rBlk.T_.GetValueVecIndex("close", &vec_idx);
				if(gvvir) {
					double last_value = 0.0;
					rBlk.T_.GetValue(rBlk.T_.GetCount()-1, vec_idx, &last_value);
					const PPObjTimeSeries::Strategy & r_s = rBlk.Strategies.at(max_result_sidx);
					double sl = r_s.CalcSL(last_value);
					if(r_s.BaseFlags & r_s.bfShort) {
						assert(rStk.Type == TsStakeEnvironment::ordtSell);
						if(rStk.SL == 0.0 || sl < rStk.SL) {
							TsStakeEnvironment::StakeRequestBlock::Req req;
							req.Ticket = rStk.Ticket;
							req.Action = TsStakeEnvironment::traSLTP;
							req.Type = rStk.Type;
							req.TsID = rBlk.PPTS.ID;
							rResult.AddS(rBlk.T_.GetSymb(), &req.SymbolP);
							req.Volume = 0;
							req.SL = sl;
							THROW_SL(rResult.L.insert(&req));
							ok = 1;
						}
					}
					else {
						assert(rStk.Type == TsStakeEnvironment::ordtBuy);
						if(rStk.SL == 0.0 || sl > rStk.SL) {
							TsStakeEnvironment::StakeRequestBlock::Req req;
							req.Ticket = rStk.Ticket;
							req.Action = TsStakeEnvironment::traSLTP;
							req.Type = rStk.Type;
							req.TsID = rBlk.PPTS.ID;
							rResult.AddS(rBlk.T_.GetSymb(), &req.SymbolP);
							req.Volume = 0;
							req.SL = sl;
							THROW_SL(rResult.L.insert(&req));
							ok = 1;
						}
					}
				}
			}
		}
		CATCHZOK
		return ok;
	}
	struct PotentialStakeEntry {
		PotentialStakeEntry(const TimeSeriesBlock & rBlk) : R_Blk(rBlk), StrategyPos(0), Volume(0.0), ResultPerDay(0.0), MarginReq(0.0)
		{
		}
		double ResultPerDay; // @firstmember
		const  TimeSeriesBlock & R_Blk;
		uint   StrategyPos;
		double Volume;
		double MarginReq; // Размер маржины, необходимый на минимальный объем сделки
	};
	int    SLAPI FindOptimalStrategyForStake(PotentialStakeEntry & rPse)
	{
		int    ok = -1;
		const TsStakeEnvironment::Tick * p_tk = StkEnv.SearchTickBySymb(rPse.R_Blk.T_.GetSymb());
		if(p_tk && rPse.R_Blk.T_.GetCount()) {
			rPse.MarginReq = p_tk->MarginReq;
			double margin_available = (StkEnv.Acc.MarginFree - StkEnv.Acc.Balance / 2) / 11.0;
			if(margin_available >= rPse.MarginReq) {
				double max_result = 0.0;
				int    max_result_sidx = -1;
				for(uint sidx = 0; sidx < rPse.R_Blk.Strategies.getCount(); sidx++) {
					const PPObjTimeSeries::Strategy & r_s = rPse.R_Blk.Strategies.at(sidx);
					double cr = 0.0;
					int    is_result = 0;
					if(r_s.StakeMode) {
						const TrendEntry * p_te = rPse.R_Blk.SearchTrendEntry(r_s.InputFrameSize);
						const uint tlc = p_te ? p_te->TL.getCount() : 0;
						if(tlc) {
							if(r_s.StakeMode == 1) {
								const double t = p_te->TL.at(tlc-1);
								if(t >= r_s.OptDeltaRange.low && t <= r_s.OptDeltaRange.upp) {
									cr = r_s.V.GetResultPerDay();
									is_result = 1;
								}
							}
							else if(r_s.StakeMode == 2) {
								if(tlc >= r_s.OptDelta2Stride) {
									const double t = p_te->TL.StrideDifference(tlc-1, r_s.OptDelta2Stride);
									if(t >= r_s.OptDelta2Range.low && t <= r_s.OptDelta2Range.upp) {
										cr = r_s.V.GetResultPerDay();
										is_result = 1;
									}
								}
							}
						}
					}
					if(is_result && max_result < cr) {
						max_result = cr;
						max_result_sidx = (int)sidx;
					}
				}
				if(max_result_sidx >= 0) {
					rPse.StrategyPos = (uint)max_result_sidx;
					rPse.ResultPerDay = max_result;
					double sc = round(margin_available / p_tk->MarginReq, 0, 1);
					rPse.Volume = R0(rPse.R_Blk.VolumeStep * sc);
					if(rPse.Volume > 0.0)
						ok = 1;
				}
			}
		}
		return ok;
	}
	int    SLAPI EvaluateStakes(TsStakeEnvironment::StakeRequestBlock & rResult)
	{
		int    ok = -1;
		const  LDATETIME now = getcurdatetime_();
		SString temp_buf;
		SString tk_symb;
		SString stk_symb;
		SString stk_memo;
		LongArray ex_stake_idx_list; // Список индексов (в StkEnv.SL) уже существующих позиций по символу
		TSVector <PotentialStakeEntry> potential_stake_list;
		for(uint i = 0; i < StkEnv.TL.getCount(); i++) {
			const TsStakeEnvironment::Tick & r_tk = StkEnv.TL.at(i);
			long d = diffdatetimesec(now, r_tk.Dtm);
			if(d < 10) {
				StkEnv.GetS(r_tk.SymbP, tk_symb);
				uint  blk_idx = 0;
				const TimeSeriesBlock * p_blk = SearchBlockBySymb(tk_symb, &blk_idx);
				if(p_blk) {
					int   is_there_stake = 0;
					//uint  stake_idx = 0;
					ex_stake_idx_list.clear();
					for(uint si = 0; si < StkEnv.SL.getCount(); si++) {
						const TsStakeEnvironment::Stake & r_stk = StkEnv.SL.at(si);
						StkEnv.GetS(r_stk.SymbP, stk_symb);
						if(stk_symb.IsEqiAscii(tk_symb)) {
							FindOptimalStrategyAtStake(*p_blk, r_stk, rResult);
							ex_stake_idx_list.add((long)si);
							is_there_stake = 1;
							//stake_idx = si;
						}
					}
					if(!is_there_stake) {
						PotentialStakeEntry pse(*p_blk);
						if(FindOptimalStrategyForStake(pse) > 0) {
							potential_stake_list.insert(&pse);
						}
					}
					// @debug {
					/*if(tk_symb.IsEqiAscii("EURUSD")) {
						if(!is_there_stake) {
							TsStakeEnvironment::StakeRequestBlock::Req req;
							req.Action = TsStakeEnvironment::traDeal;
							req.Type = TsStakeEnvironment::ordtBuy;
							req.TsID = p_blk->PPTS.ID;
							rResult.AddS(tk_symb, &req.SymbolP);
							req.Volume = 1000;
							rResult.AddS("PPAT-TEST", &req.CommentP);
							rResult.L.insert(&req);
						}
						else {
							if(p_blk->PPTS.OptMaxDuck > 0 && p_blk->PPTS.SpikeQuant > 0.0) {
								const TsStakeEnvironment::Stake & r_stk = StkEnv.SL.at(stake_idx);
								TsStakeEnvironment::StakeRequestBlock::Req req;
								req.Ticket = r_stk.Ticket;
								req.Action = TsStakeEnvironment::traSLTP;
								req.Type = TsStakeEnvironment::ordtBuy;
								req.TsID = p_blk->PPTS.ID;
								rResult.AddS(tk_symb, &req.SymbolP);
								req.Volume = 0;
								req.SL = round(r_tk.Last * (1.0 - p_blk->PPTS.OptMaxDuck * p_blk->PPTS.SpikeQuant), p_blk->PPTS.Prec);
								temp_buf.Z().Cat("PPAT-TEST").Space().Cat("SETSL").Space().Cat(req.SL);
								rResult.AddS(temp_buf, &req.CommentP);
								rResult.L.insert(&req);
							}
						}
					}*/
					// } @debug 
				}
			}
		}
		{
			const uint pslc = potential_stake_list.getCount();
			if(pslc) {
				potential_stake_list.sort(PTR_CMPFUNC(double));
				double margin_available = (StkEnv.Acc.MarginFree - StkEnv.Acc.Balance / 2);
				if(margin_available > 0.0) {
					uint _pslp = pslc;
					do {
						PotentialStakeEntry & r_pse = potential_stake_list.at(--_pslp);
						uint   vec_idx = 0;
						const  int  gvvir = r_pse.R_Blk.T_.GetValueVecIndex("close", &vec_idx);
						if(gvvir) {
							const PPObjTimeSeries::Strategy & r_s = r_pse.R_Blk.Strategies.at(r_pse.StrategyPos);
							{
								TsStakeEnvironment::StakeRequestBlock::Req req;
								req.Action = TsStakeEnvironment::traDeal;
								req.Type = (r_s.BaseFlags & r_s.bfShort) ? TsStakeEnvironment::ordtSell : TsStakeEnvironment::ordtBuy;
								req.TsID = r_pse.R_Blk.PPTS.ID;
								rResult.AddS(r_pse.R_Blk.T_.GetSymb(), &req.SymbolP);
								req.Volume = r_pse.Volume;

								double last_value = 0.0;
								r_pse.R_Blk.T_.GetValue(r_pse.R_Blk.T_.GetCount()-1, vec_idx, &last_value);
								double sl = r_s.CalcSL(last_value);
								req.SL = sl;

								rResult.AddS("PPAT", &req.CommentP);
								rResult.L.insert(&req);
							}
							margin_available -= (r_pse.Volume / r_pse.R_Blk.VolumeStep) * r_pse.MarginReq;
						}
					} while(_pslp && margin_available > 0.0);
				}
			}
		}
		return ok;
	}
private:
	virtual int  SLAPI FetchEntry(PPID, ObjCacheEntry * pEntry, long);
	virtual void SLAPI EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const;
	TimeSeriesBlock * SLAPI SearchBlockBySymb(const char * pSymb, uint * pIdx) const;
	TimeSeriesBlock * SLAPI InitBlock(const char * pSymb);
	int    SLAPI EvaluateTrends(TimeSeriesBlock * pBlk, const STimeSeries * pFullTs);

	TSCollection <TimeSeriesBlock> TsC;
	TSVector <PPObjTimeSeries::QuoteReqEntry> * P_ReqQList;
	TsStakeEnvironment StkEnv;
	SMtLock OpL; // Блокировка для операций, иных нежели штатные методы ObjCache
	LDATETIME LastFlashDtm;

	static void LogStateEnvironment(const TsStakeEnvironment & rEnv)
	{
		SString file_name;
		SString line_buf;
		SString temp_buf;
		SString comment_buf;
		PPGetFilePath(PPPATH_LOG, "TsStakeEnvironment.log", file_name);
		SFile f_out(file_name, SFile::mAppend);
		line_buf.Z().Cat(getcurdatetime_(), DATF_ISO8601|DATF_CENTURY, 0);
		f_out.WriteLine(line_buf.CR());
		line_buf.Z().Cat("Account").CatDiv(':', 2).Cat(rEnv.Acc.ActualDtm, DATF_ISO8601|DATF_CENTURY, 0).Space().
			Cat(rEnv.Acc.ID).Space().
			Cat(rEnv.Acc.Balance, MKSFMTD(0, 2, 0)).Space().
			Cat(rEnv.Acc.MarginFree, MKSFMTD(0, 2, 0)).Space().
			Cat(rEnv.Acc.Profit, MKSFMTD(0, 2, 0));
		f_out.WriteLine(line_buf.CR());
		{
			for(uint i = 0; i < rEnv.SL.getCount(); i++) {
				const TsStakeEnvironment::Stake & r_stk = rEnv.SL.at(i);
				rEnv.GetS(r_stk.SymbP, temp_buf);
				rEnv.GetS(r_stk.CommentP, comment_buf);
				line_buf.Z().Cat("Stake").CatDiv(':', 2).
					Cat(temp_buf).Space().
					Cat(r_stk.Type).Space().
					Cat(r_stk.Ticket).Space().
					Cat(r_stk.SetupDtm, DATF_ISO8601|DATF_CENTURY, 0).Space().
					Cat(r_stk.VolumeInit, MKSFMTD(0, 0, 0)).Space().
					Cat(r_stk.PriceOpen, MKSFMTD(0, 5, 0)).Space().
					Cat(r_stk.PriceCurrent).Space().
					Cat(r_stk.SL, MKSFMTD(0, 5, 0)).Space().
					Cat(r_stk.TP, MKSFMTD(0, 5, 0)).Space().
					Cat(comment_buf);
				f_out.WriteLine(line_buf.CR());
			}
		}
		{
			for(uint i = 0; i < rEnv.TL.getCount(); i++) {
				const TsStakeEnvironment::Tick & r_tk = rEnv.TL.at(i);
				rEnv.GetS(r_tk.SymbP, temp_buf);
				line_buf.Z().Cat("Tick").CatDiv(':', 2).
					Cat(temp_buf).Space().
					Cat(r_tk.TsID).Space().
					Cat(r_tk.Dtm, DATF_ISO8601|DATF_CENTURY, 0).Space().
					Cat(r_tk.Bid, MKSFMTD(0, 5, 0)).Space().
					Cat(r_tk.Ask, MKSFMTD(0, 5, 0)).Space().
					Cat(r_tk.Last, MKSFMTD(0, 5, 0)).Space().
					Cat(r_tk.Volume).Space().
					Cat(r_tk.MarginReq, MKSFMTD(0, 5, 0));
				f_out.WriteLine(line_buf.CR());
			}
		}
	}
};

int SLAPI TimeSeriesCache::FetchEntry(PPID id, ObjCacheEntry * pEntry, long)
{
	int    ok = 1;
	Data * p_cache_rec = (Data *)pEntry;
	PPObjTimeSeries ts_obj;
	PPTimeSeries rec;
	if(ts_obj.Search(id, &rec) > 0) {
		p_cache_rec->Flags    = rec.Flags;
		p_cache_rec->BuyMarg  = rec.BuyMarg;
		p_cache_rec->SellMarg = rec.SellMarg;
		p_cache_rec->SpikeQuant = rec.SpikeQuant;
		p_cache_rec->AvgSpread = rec.AvgSpread;
		p_cache_rec->OptMaxDuck = rec.OptMaxDuck;
		p_cache_rec->OptMaxDuck_S = rec.OptMaxDuck_S;
		p_cache_rec->Prec     = rec.Prec;

		MultTextBlock b;
		b.Add(rec.Name);
		b.Add(rec.Symb);
		b.Add(rec.CurrencySymb);
		ok = PutTextBlock(b, p_cache_rec);
	}
	else
		ok = -1;
	return ok;
}

void SLAPI TimeSeriesCache::EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const
{
	PPTimeSeries * p_data_rec = (PPTimeSeries *)pDataRec;
	const Data * p_cache_rec = (const Data *)pEntry;
	memzero(p_data_rec, sizeof(*p_data_rec));
	p_data_rec->Tag   = PPOBJ_TIMESERIES;
	p_data_rec->ID    = p_cache_rec->ID;
	p_data_rec->Flags = p_cache_rec->Flags;
	p_data_rec->BuyMarg  = p_cache_rec->BuyMarg;
	p_data_rec->SellMarg = p_cache_rec->SellMarg;
	p_data_rec->SpikeQuant = p_cache_rec->SpikeQuant;
	p_data_rec->AvgSpread = p_cache_rec->AvgSpread;
	p_data_rec->OptMaxDuck = p_cache_rec->OptMaxDuck;
	p_data_rec->OptMaxDuck_S = p_cache_rec->OptMaxDuck_S;
	p_data_rec->Prec     = p_cache_rec->Prec;
	//
	MultTextBlock b(this, pEntry);
	b.Get(p_data_rec->Name, sizeof(p_data_rec->Name));
	b.Get(p_data_rec->Symb, sizeof(p_data_rec->Symb));
	b.Get(p_data_rec->CurrencySymb, sizeof(p_data_rec->CurrencySymb));
}

TimeSeriesCache::TimeSeriesBlock * SLAPI TimeSeriesCache::SearchBlockBySymb(const char * pSymb, uint * pIdx) const
{
	SString & r_temp_buf = SLS.AcquireRvlStr();
	for(uint i = 0; i < TsC.getCount(); i++) {
		TimeSeriesBlock * p_blk = TsC.at(i);
		if(p_blk) {
			r_temp_buf = p_blk->T_.GetSymb();
			if(r_temp_buf.IsEqiAscii(pSymb)) {
				ASSIGN_PTR(pIdx, i);
				return p_blk;
			}
		}
	}
	return 0;
}

int SLAPI TimeSeriesCache::EvaluateTrends(TimeSeriesBlock * pBlk, const STimeSeries * pFullTs)
{
	int    ok = 1;
	PROFILE_START
	const uint full_tsc = pFullTs ? pFullTs->GetCount() : 0;
	//RealArray test_trend_list; // @debug
	const uint tsc = pBlk->T_.GetCount();
	uint  max_opt_delta2_stride = 0;
	LongArray ifs_list;
	if(tsc > 500 && pBlk->Strategies.GetInputFramSizeList(ifs_list, &max_opt_delta2_stride) > 0) {
		const uint trend_nominal_count = /*10*/max_opt_delta2_stride+1;
		ifs_list.sortAndUndup(); // @paranoic
		for(uint i = 0; i < ifs_list.getCount(); i++) {
			const uint ifs = (uint)ifs_list.get(i);
			if(ifs > 0 && tsc >= (ifs+trend_nominal_count)) {
				TrendEntry * p_entry = 0;
				for(uint j = 0; !p_entry && j < pBlk->TrendList.getCount(); j++) {
					TrendEntry * p_iter = pBlk->TrendList.at(j);
					if(p_iter && p_iter->Stride == ifs)
						p_entry = p_iter;
				}
				if(!p_entry) {
					THROW_SL(p_entry = new TrendEntry(ifs, trend_nominal_count));
					THROW_SL(pBlk->TrendList.insert(p_entry));
				}
				{
					STimeSeries::AnalyzeFitParam afp(ifs, tsc-p_entry->NominalCount, p_entry->NominalCount);
					THROW(pBlk->T_.AnalyzeFit("close", afp, &p_entry->TL, 0));
				}
				/*if(pFullTs) { // @debug
					const uint op_trend_list_count = p_entry->TL.getCount();
					STimeSeries::AnalyzeFitParam afp(ifs, 0, 0);
					THROW(pFullTs->AnalyzeFit("close", afp, &test_trend_list, 0));
					assert(test_trend_list.getCount() == full_tsc);
					for(uint tti = 0; tti < op_trend_list_count; tti++) {
						double trend_value = p_entry->TL.at(tti);
						double test_trend_value = test_trend_list.at(full_tsc-op_trend_list_count+tti);
						assert(trend_value == test_trend_value);
					}
				}*/
				
			}
		}
	}
	PROFILE_END
	CATCHZOK
	return ok;
}

TimeSeriesCache::TimeSeriesBlock * SLAPI TimeSeriesCache::InitBlock(const char * pSymb)
{
	TimeSeriesBlock * p_fblk = 0;
	PROFILE_START
	PPID   id = 0;
	PPObjTimeSeries ts_obj;
	PPTimeSeries ts_rec;
	STimeSeries ts_full;
	int r = ts_obj.SearchBySymb(pSymb, &id, &ts_rec);
	THROW(r);
	if(r < 0) {
		id = 0;
		MEMSZERO(ts_rec);
		STRNSCPY(ts_rec.Name, pSymb);
		STRNSCPY(ts_rec.Symb, pSymb);
		THROW(ts_obj.PutPacket(&id, &ts_rec, 1));
		ts_rec.ID = id;
	}
	THROW_SL(p_fblk = TsC.CreateNewItem());
	p_fblk->T_.SetSymb(pSymb);
	p_fblk->PPTS = ts_rec;
	THROW(r = ts_obj.GetTimeSeries(id, ts_full));
	if(r > 0) {
		const uint full_tsc = ts_full.GetCount();
		ts_full.GetChunkRecentCount(MIN(10000, full_tsc), p_fblk->T_);
		ts_obj.GetStrategies(id, p_fblk->Strategies);
		THROW(EvaluateTrends(p_fblk, 0));
	}
	else {
		uint   vecidx_close = 0;
		uint   vecidx_realvol = 0;
		THROW_SL(p_fblk->T_.AddValueVec("close", T_INT32, 5, &vecidx_close));
		THROW_SL(p_fblk->T_.AddValueVec("real_volume", T_INT32, 0, &vecidx_realvol));
	}
	PROFILE_END
	CATCH
		p_fblk = 0;
	ENDCATCH
	return p_fblk;
}

int SLAPI PPObjTimeSeries::LoadQuoteReqList(TSVector <QuoteReqEntry> & rList)
{
	rList.clear();
	int    ok = -1;
	SString temp_buf;
	PPGetFilePath(PPPATH_DD, "quotereq.txt", temp_buf);
	if(fileExists(temp_buf)) {
		SFile f_in(temp_buf, SFile::mRead);
		if(f_in.IsValid()) {
			uint   line_no = 0;
			SString line_buf;
			StringSet ss("\t");
			while(f_in.ReadLine(line_buf)) {
				line_no++;
				if(line_no > 1) { // The first line is title
					line_buf.Chomp().Strip();
					ss.setBuf(line_buf);
					QuoteReqEntry entry;
					MEMSZERO(entry);
					for(uint ssp = 0, fld_no = 0; ss.get(&ssp, temp_buf); fld_no++) {
						if(fld_no == 0)
							STRNSCPY(entry.Ticker, temp_buf);
						else if(fld_no == 1) {
							if(temp_buf.StrChr('L', 0) || temp_buf.StrChr('l', 0))
								entry.Flags |= entry.fAllowLong;
							if(temp_buf.StrChr('S', 0) || temp_buf.StrChr('s', 0))
								entry.Flags |= entry.fAllowShort;
						}
					}
					if(entry.Ticker[0]) {
						PPTimeSeries ts_rec;
						PPID   ts_id = 0;
						if(SearchBySymb(entry.Ticker, &ts_id, &ts_rec) > 0)
							entry.TsID = ts_rec.ID;
						rList.insert(&entry);
						ok = 1;
					}
				}
			}
		}
	}
	return ok;
}

int SLAPI PPObjTimeSeries::GetReqQuotes(TSVector <PPObjTimeSeries::QuoteReqEntry> & rList)
{
	TimeSeriesCache * p_cache = GetDbLocalCachePtr <TimeSeriesCache> (PPOBJ_TIMESERIES);
	return p_cache ? p_cache->GetReqQuotes(rList) : 0;
}

int SLAPI PPObjTimeSeries::SetExternTimeSeries(STimeSeries & rTs)
{
	TimeSeriesCache * p_cache = GetDbLocalCachePtr <TimeSeriesCache> (PPOBJ_TIMESERIES);
	return p_cache ? p_cache->SetTimeSeries(rTs) : 0;
}

int SLAPI PPObjTimeSeries::SetExternStakeEnvironment(const TsStakeEnvironment & rEnv, TsStakeEnvironment::StakeRequestBlock & rRet)
{
	TimeSeriesCache * p_cache = GetDbLocalCachePtr <TimeSeriesCache> (PPOBJ_TIMESERIES);
	return p_cache ? p_cache->SetCurrentStakeEnvironment(&rEnv, &rRet) : 0;
}

int SLAPI PPObjTimeSeries::SetExternTimeSeriesProp(const char * pSymb, const char * pPropSymb, const char * pPropVal)
{
	int    ok = -1;
	int    do_update = 0;
	PPTimeSeries ts_rec;
	PPID   id = 0;
	if(!isempty(pSymb) && !isempty(pPropSymb)) {
		THROW(SearchBySymb(pSymb, &id, &ts_rec) > 0);
		const SymbHashTable * p_sht = PPGetStringHash(PPSTR_HASHTOKEN);
		if(p_sht) {
			uint   _ut = 0;
			SString temp_buf;
			(temp_buf = pPropSymb).Strip().ToLower();
			p_sht->Search(temp_buf, &_ut, 0);
			(temp_buf = pPropVal).Strip();
			switch(_ut) {
				case PPHS_TIMSER_PROP_PRECISION:
					{
						long prec = temp_buf.NotEmpty() ? temp_buf.ToLong() : 0;
						if(prec != (long)ts_rec.Prec) {
							ts_rec.Prec = (int16)prec;
							do_update = 1;
						}
					}
					break;
				case PPHS_TIMSER_PROP_MARGIN_BUY:
					{
						double marg = temp_buf.ToReal();
						if(marg != (long)ts_rec.BuyMarg) {
							ts_rec.BuyMarg = marg;
							do_update = 1;
						}
					}
					break;
				case PPHS_TIMSER_PROP_MARGIN_SELL:
					{
						double marg = temp_buf.ToReal();
						if(marg != (long)ts_rec.SellMarg) {
							ts_rec.SellMarg = marg;
							do_update = 1;
						}
					}
					break;
				case PPHS_TIMSER_PROP_CURRENCY_BASE:
					// not implemented
					break;
				case PPHS_TIMSER_PROP_CURRENCY_PROFIT:
					if(temp_buf.NotEmpty()) {
						if(!sstreqi_ascii(temp_buf, ts_rec.CurrencySymb) && temp_buf.Len() < sizeof(ts_rec.CurrencySymb)) {
							STRNSCPY(ts_rec.CurrencySymb, temp_buf);
							do_update = 1;
						}
					}
					break;
				case PPHS_TIMSER_PROP_CURRENCY_MARGIN:
					// not implemented
					break;
				case PPHS_TIMSER_PROP_VOLUME_MIN: // "time-series-volume-min"      // минимальный объем сделки
					{
						double val = temp_buf.ToReal();
						if(val > 0.0) {
							TimeSeriesCache * p_cache = GetDbLocalCachePtr <TimeSeriesCache> (PPOBJ_TIMESERIES);
							CALLPTRMEMB(p_cache, SetVolumeParams(pSymb, val, 0.0, 0.0));
						}
					}
					break;
				case PPHS_TIMSER_PROP_VOLUME_MAX: // "time-series-volume-max"      // максимальный объем сделки
					{
						double val = temp_buf.ToReal();
						if(val > 0.0) {
							TimeSeriesCache * p_cache = GetDbLocalCachePtr <TimeSeriesCache> (PPOBJ_TIMESERIES);
							CALLPTRMEMB(p_cache, SetVolumeParams(pSymb, 0.0, val, 0.0));
						}
					}
					break;
				case PPHS_TIMSER_PROP_VOLUME_STEP: // "time-series-volume-step"     // Минимальный шаг изменения объема для заключения сделки
					{
						double val = temp_buf.ToReal();
						if(val > 0.0) {
							TimeSeriesCache * p_cache = GetDbLocalCachePtr <TimeSeriesCache> (PPOBJ_TIMESERIES);
							CALLPTRMEMB(p_cache, SetVolumeParams(pSymb, 0.0, 0.0, val));
						}
					}
					break;
			}
			if(do_update) {
				THROW(PutPacket(&id, &ts_rec, 1));
				ok = 1;
			}
		}
	}
	CATCHZOK
	return ok;
}

// } @construction 
