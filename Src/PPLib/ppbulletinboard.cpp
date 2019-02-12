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
	/*
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
	*/
	struct ImpactEntry {
		ImpactEntry(LDATETIME t, double v) : T(t), Value(v)
		{
		}
		LDATETIME T;
		double Value;
	};
	struct TimeSeriesBlock {
		SLAPI  TimeSeriesBlock() : Flags(0), SpreadSum(0), SpreadCount(0), VolumeMin(0.0), VolumeMax(0.0), VolumeStep(0.0)
		{
		}
		int FASTCALL GetLastValue(double * pValue) const
		{
			int    ok = 0;
			double last_value = 0.0;
			const  uint tsc = T_.GetCount();
			if(tsc) {
				uint   vec_idx = 0;
				const  int  gvvir = T_.GetValueVecIndex("close", &vec_idx);
				if(gvvir && T_.GetValue(tsc-1, vec_idx, &last_value) > 0 && last_value > 0.0)
					ok = 1;
			}
			ASSIGN_PTR(pValue, last_value);
			return ok;
		}
		enum {
			fLong         = 0x0001, // Допускается покупка
			fShort        = 0x0002, // Допускается продажа
			fDisableStake = 0x0004, // Запрет на сделки
			fNoConfig     = 0x0008, // Символ не представлен в конфигурации
			fDirty        = 0x0100  // Данные по серии были изменены, но не сохранены в базе данных
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

		class ImpactBlock {
		public:
			ImpactBlock() : LastMoment(ZERODATETIME), LastValue(0.0)
			{
			}
			void    FASTCALL Update(const TsStakeEnvironment::Tick & rTk, const double quant)
			{
				static const long factor = 20;
				if(quant > 0.0 && rTk.Last > 0.0) {
					if(!LastMoment) {
						LastMoment = rTk.Dtm;
						LastValue = rTk.Last;
					}
					else {
						const long s = diffdatetimesec(rTk.Dtm, LastMoment);
						if(s > 0) {
							if(s <= 12) {
								double min_impact_delta = LastValue * quant * factor;
								if(fabs(rTk.Last-LastValue) >= min_impact_delta) {
									ImpactEntry * p_last_entry = List.getCount() ? &List.at(List.getCount()-1) : 0;
									if(!p_last_entry || p_last_entry->T != rTk.Dtm) {
										ImpactEntry entry1(LastMoment, LastValue);
										List.insert(&entry1);
									}
									{
										ImpactEntry entry2(rTk.Dtm, rTk.Last);
										List.insert(&entry2);
									}
								}
							}
							LastMoment = rTk.Dtm;
							LastValue = rTk.Last;
						}
					}
				}
			}
			const TSVector <ImpactEntry> & GetList() const
			{
				return List;
			}
		private:
			LDATETIME LastMoment;
			double LastValue;
			TSVector <ImpactEntry> List;
		};

		ImpactBlock ImpctB;

		void FASTCALL UpdateImpactList(const TsStakeEnvironment::Tick & rTk)
		{
			ImpctB.Update(rTk, PPTS.SpikeQuant);
		}
		//
		// Descr: Структура, используемая как буфер для управления последним значением
		//   в последовательности T_. Если самое актуальное значение пришло с тиком (TsStakeEnvironment::Tick), 
		//   то регулярное последнее значение (если оно старше тика) сбрасывается в TempLastEntry с тегом 1, а
		//   вместо него в T_ заносится тик.
		//   При получении регулярной последовательности, если последним элементом в T_ был тик, то он
		//   переносится в TempLastEntry с тегом 2.
		//
		struct TempLastEntry {
			SLAPI  TempLastEntry() : Tag(0)
			{
			}
			int    Tag; // 0 - empty, 1 - main series value, 2 - tick value
			STimeSeries::SnapshotEntry Sse;
		};
		TempLastEntry TLE;
		TSCollection <PPObjTimeSeries::TrendEntry> TrendList;
		PPObjTimeSeries::StrategyContainer Strategies;
		void SLAPI Make_T_Regular()
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
		void SLAPI Make_T_Actual()
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
	struct PotentialStakeEntry {
		explicit SLAPI PotentialStakeEntry(const TimeSeriesBlock & rBlk) : R_Blk(rBlk), StrategyPos(0), Volume(0.0), 
			ResultPerDay(0.0), MarginReq(0.0), Tv(0.0), Tv2(0.0)
		{
		}
		double AdjustedResultPerDay; // @firstmember Результат, умноженный на используемую маржу
		double ResultPerDay;         // 
		const  TimeSeriesBlock & R_Blk;
		uint   StrategyPos;
		double Tv;  // Трендовая величина
		double Tv2; // 2-я трендовая величина
		double Volume;
		double MarginReq; // Размер маржины, необходимый на минимальный объем сделки
	};
	static int OnQuartz(int kind, const PPNotifyEvent * pEv, void * procExtPtr)
	{
		TimeSeriesCache * p_this = static_cast<TimeSeriesCache *>(procExtPtr);
		if(p_this) {
			LDATETIME cdtm = getcurdatetime_();
			long sec = diffdatetimesec(cdtm, p_this->LastFlashDtm);
			if(sec >= p_this->GetFlashTimeout()) {
				p_this->Flash();
			}
		}
		return 1;
	}
	static int OnConfigUpdated(int kind, const PPNotifyEvent * pEv, void * procExtPtr)
	{
		TimeSeriesCache * p_this = static_cast<TimeSeriesCache *>(procExtPtr);
		if(p_this && pEv->ObjType == PPCFGOBJ_TIMESERIES) {
			p_this->OpL.Lock();
			p_this->LoadConfig(1);
			p_this->OpL.Unlock();
		}
		return 1;
	}
	TimeSeriesCache();
	~TimeSeriesCache();
	int    SLAPI SetTimeSeries(STimeSeries & rTs);
	int    SLAPI GetReqQuotes(TSVector <PPObjTimeSeries::QuoteReqEntry> & rList);
	int    SLAPI SetCurrentStakeEnvironment(const TsStakeEnvironment * pEnv, TsStakeEnvironment::StakeRequestBlock * pRet);
	int    SLAPI SetVolumeParams(const char * pSymb, double volumeMin, double volumeMax, double volumeStep);
	int    SLAPI Flash();
	int    SLAPI FindOptimalStrategyAtStake(const TimeSeriesBlock & rBlk, const TsStakeEnvironment::Stake & rStk, TsStakeEnvironment::StakeRequestBlock & rResult) const;
	int    SLAPI FindOptimalStrategyForStake(PotentialStakeEntry & rPse) const;
	double SLAPI EvaluateCost(const TimeSeriesBlock & rBlk, bool sell, double volume) const;
	int    SLAPI EvaluateStakes(TsStakeEnvironment::StakeRequestBlock & rResult);
private:
	static void LogStateEnvironment(const TsStakeEnvironment & rEnv);
	virtual int  SLAPI FetchEntry(PPID, ObjCacheEntry * pEntry, long);
	virtual void SLAPI EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const;
	int    SLAPI LoadConfig(int force);
	TimeSeriesBlock * SLAPI SearchBlockBySymb(const char * pSymb, uint * pIdx) const;
	const TimeSeriesBlock * SLAPI SearchRateConvertionBlock(const char * pSymb, int * pRevese) const;
	TimeSeriesBlock * SLAPI InitBlock(PPObjTimeSeries & rTsObj, const char * pSymb);
	int    SLAPI EvaluateTrends(TimeSeriesBlock * pBlk, const STimeSeries * pFullTs);
	long   SLAPI GetFlashTimeout() const
	{
		const long cfg_tmr = Cfg.E.TsFlashTimer;
		return (cfg_tmr > 0 && cfg_tmr <= (24*3600)) ? cfg_tmr : 600;
	}
	int    FASTCALL IsStrategySuited(const PPObjTimeSeries::Strategy & rS) const;

	TSCollection <TimeSeriesBlock> TsC;
	TsStakeEnvironment StkEnv;
	SMtLock OpL; // Блокировка для операций, иных нежели штатные методы ObjCache
	LDATETIME LastFlashDtm;
	enum {
		stConfigLoaded = 0x0001
	};
	long   State;
	PPObjTimeSeries::Config Cfg;
};

//static 
void TimeSeriesCache::LogStateEnvironment(const TsStakeEnvironment & rEnv)
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

TimeSeriesCache::TimeSeriesCache() : ObjCache(PPOBJ_TIMESERIES, sizeof(Data)), LastFlashDtm(getcurdatetime_()), /*P_ReqQList(0),*/ State(0)
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
	{
		long   cookie = 0;
		PPAdviseBlock adv_blk;
		MEMSZERO(adv_blk);
		adv_blk.Kind = PPAdviseBlock::evDirtyCacheBySysJ;
		adv_blk.DbPathID = DBS.GetDbPathID();
		adv_blk.ObjType = PPCFGOBJ_TIMESERIES;
		adv_blk.Proc = TimeSeriesCache::OnConfigUpdated;
		adv_blk.ProcExtPtr = this;
		DS.Advise(&cookie, &adv_blk);
	}
}

TimeSeriesCache::~TimeSeriesCache()
{
}
	
int SLAPI TimeSeriesCache::SetTimeSeries(STimeSeries & rTs)
{
	int    ok = -1;
	STimeSeries::AppendStat apst;
	OpL.Lock();
	SString temp_buf = rTs.GetSymb();
	if(temp_buf.NotEmpty()) {
		TimeSeriesBlock * p_fblk = SearchBlockBySymb(temp_buf, 0);
		if(!p_fblk) {
			PPObjTimeSeries ts_obj;
			THROW(p_fblk = InitBlock(ts_obj, temp_buf));
		}
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

int SLAPI TimeSeriesCache::LoadConfig(int force)
{
	int    ok = 1;
	if(force || !(State & stConfigLoaded)) {
		ok = BIN(PPObjTimeSeries::ReadConfig(&Cfg));
		State |= stConfigLoaded;
		SETMAX(Cfg.MinPerDayPotential, 0.0);
		for(uint j = 0; j < TsC.getCount(); j++) {
			TimeSeriesBlock * p_blk = TsC.at(j);
			if(p_blk) {
				const PPObjTimeSeries::Config::Entry * p_cfg_entry = p_blk->PPTS.ID ? Cfg.SearchEntry(p_blk->PPTS.ID) : 0;
				if(p_cfg_entry) {
					SETFLAG(p_blk->Flags, TimeSeriesBlock::fLong,  p_cfg_entry->Flags & PPObjTimeSeries::Config::efLong);
					SETFLAG(p_blk->Flags, TimeSeriesBlock::fShort, p_cfg_entry->Flags & PPObjTimeSeries::Config::efShort);
					SETFLAG(p_blk->Flags, TimeSeriesBlock::fDisableStake, p_cfg_entry->Flags & PPObjTimeSeries::Config::efDisableStake);
					p_blk->Flags &= ~TimeSeriesBlock::fNoConfig;
				}
				else { 
					p_blk->Flags &= ~(TimeSeriesBlock::fLong|TimeSeriesBlock::fShort|TimeSeriesBlock::fDisableStake);
					p_blk->Flags |= TimeSeriesBlock::fNoConfig;
				}
			}
		}
	}
	return ok;
}
	
int SLAPI TimeSeriesCache::GetReqQuotes(TSVector <PPObjTimeSeries::QuoteReqEntry> & rList)
{
	rList.clear();
	int    ok = -1;
	PPObjTimeSeries ts_obj;
	OpL.Lock();
	if(LoadConfig(0)) {
		for(uint i = 0; i < Cfg.List.getCount(); i++) {
			const PPObjTimeSeries::Config::Entry & r_entry = Cfg.List.at(i);
			PPTimeSeries ts_rec;
			if(Get(r_entry.TsID, &ts_rec, 0) > 0 && ts_rec.Symb[0]) { // @attention Здесь идет обращение к блокируемой внутренней функции
				PPObjTimeSeries::QuoteReqEntry new_entry;
				MEMSZERO(new_entry);
				new_entry.TsID = r_entry.TsID;
				SETFLAG(new_entry.Flags, new_entry.fAllowLong, r_entry.Flags & PPObjTimeSeries::Config::efLong);
				SETFLAG(new_entry.Flags, new_entry.fAllowShort, r_entry.Flags & PPObjTimeSeries::Config::efShort);
				SETFLAG(new_entry.Flags, new_entry.fDisableStake, r_entry.Flags & PPObjTimeSeries::Config::efDisableStake);
				STRNSCPY(new_entry.Ticker, ts_rec.Symb);
				{
					SUniTime last_utm;
					TimeSeriesBlock * p_blk = SearchBlockBySymb(ts_rec.Symb, 0);
					SETIFZ(p_blk, InitBlock(ts_obj, ts_rec.Symb));
					if(p_blk) {
						SETFLAG(p_blk->Flags, TimeSeriesBlock::fLong,  r_entry.Flags & PPObjTimeSeries::Config::efLong);
						SETFLAG(p_blk->Flags, TimeSeriesBlock::fShort, r_entry.Flags & PPObjTimeSeries::Config::efShort);
						SETFLAG(p_blk->Flags, TimeSeriesBlock::fDisableStake, r_entry.Flags & PPObjTimeSeries::Config::efDisableStake);
						p_blk->Flags &= ~TimeSeriesBlock::fNoConfig;
					}
					const uint tc = p_blk ? p_blk->T_.GetCount() : 0;
					if(tc && p_blk->T_.GetTime(tc-1, &last_utm))
						last_utm.Get(new_entry.LastValTime);
					else
						new_entry.LastValTime.Z();
				}
				rList.insert(&new_entry);
			}
		}
	}
	OpL.Unlock();
	return ok;
}
	
int SLAPI TimeSeriesCache::SetCurrentStakeEnvironment(const TsStakeEnvironment * pEnv, TsStakeEnvironment::StakeRequestBlock * pRet)
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
			if(p_fblk) {
				p_fblk->UpdateImpactList(r_tk);
				const uint tsc = p_fblk->T_.GetCount();
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
	
int SLAPI TimeSeriesCache::SetVolumeParams(const char * pSymb, double volumeMin, double volumeMax, double volumeStep)
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
	
int SLAPI TimeSeriesCache::Flash()
{
	int    ok = 1;
	SFile * p_f_imp_out = 0;
	OpL.Lock();
	if(TsC.getCount()) {
		STimeSeries org_ts;
		PPObjTimeSeries ts_obj;
		SString temp_buf;
		for(uint i = 0; i < TsC.getCount(); i++) {
			TimeSeriesBlock * p_blk = TsC.at(i);
			if(p_blk && p_blk->PPTS.ID) {
				if(p_blk->Flags & TimeSeriesBlock::fDirty) {
					PPID   id = p_blk->PPTS.ID;
					PPTimeSeries rec;
					PPTransaction tra(1);
					THROW(tra);
					if(ts_obj.GetPacket(id, &rec) > 0) {
						if(p_blk->SpreadCount && p_blk->SpreadSum) {
							rec.AvgSpread = fdivui(p_blk->SpreadSum, p_blk->SpreadCount);
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
					THROW(tra.Commit());
				}
				{
					const TSVector <ImpactEntry> & r_impact_list = p_blk->ImpctB.GetList();
					if(r_impact_list.getCount()) {
						if(!p_f_imp_out) {
							PPGetFilePath(PPPATH_OUT, "ts-impact-list.txt", temp_buf);
							p_f_imp_out = new SFile(temp_buf, SFile::mAppend);
						}
						if(p_f_imp_out && p_f_imp_out->IsValid()) {
							p_f_imp_out->WriteLine(temp_buf.Z().CR());
							for(uint j = 0; j < r_impact_list.getCount(); j++) {
								const ImpactEntry & r_impact_entry = r_impact_list.at(j);
								temp_buf.Z().Cat(p_blk->PPTS.Symb).Tab().
									Cat(r_impact_entry.T, DATF_ISO8601|DATF_CENTURY, 0).Tab().Cat(r_impact_entry.Value, MKSFMTD(0, 5, 0)).CR();
								p_f_imp_out->WriteLine(temp_buf);
							}
						}
					}
				}
			}
		}
		LastFlashDtm = getcurdatetime_();
	}
	CATCHZOK
	ZDELETE(p_f_imp_out);
	OpL.Unlock();
	return ok;
}

struct BestStrategyBlock {
	BestStrategyBlock() : MaxResult(0.0), MaxResultIdx(-1), TvForMaxResult(0.0), Tv2ForMaxResult(0.0)
	{
	}
	void   SetResult(double localResult, uint strategyIdx, double tv, double tv2)
	{
		if(MaxResult < localResult) {
			MaxResult = localResult;
			MaxResultIdx = static_cast<int>(strategyIdx);
			TvForMaxResult = tv;
			Tv2ForMaxResult = tv2;
		}
	}
	double MaxResult;
	int    MaxResultIdx;
	double TvForMaxResult;
	double Tv2ForMaxResult;
};

int FASTCALL TimeSeriesCache::IsStrategySuited(const PPObjTimeSeries::Strategy & rS) const
{
	int    yes = 0;
	if(rS.V.GetResultPerDay() >= Cfg.MinPerDayPotential) {
		if(Cfg.Flags & (Cfg.fUseStakeMode2|Cfg.fUseStakeMode3)) {
			if(rS.StakeMode == 2 && Cfg.Flags & Cfg.fUseStakeMode2)
				yes = 1;
			else if(rS.StakeMode == 3 && Cfg.Flags & Cfg.fUseStakeMode3)
				yes = 1;
		}
		else if(rS.StakeMode == 3)
			yes = 1;
	}
	return yes;
}

static SString & SLAPI StrategyToString(const PPObjTimeSeries::Strategy & rS, const BestStrategyBlock * pBestResult, SString & rBuf)
{
	rBuf.Z();
	const long trange_fmt = MKSFMTD(0, 10, NMBF_NOTRAILZ);
	rBuf.Cat("Strategy").CatDiv(':', 2).Cat((rS.BaseFlags & rS.bfShort) ? "S" : "B").CatChar('/');
	rBuf.Cat(rS.InputFrameSize).CatChar('/').Cat(rS.MaxDuckQuant).CatChar(':').Cat(rS.TargetQuant).CatChar('/').Cat(rS.StakeMode);
	rBuf.Space().Cat("Potential").CatDiv(':', 2).
		Cat(rS.V.GetResultPerDay(), MKSFMTD(0, 4, 0)).CatChar('/').
		Cat(rS.GetWinCountRate(), MKSFMTD(0, 3, 0)).CatChar('/').
		Cat(rS.V.TmSec / rS.StakeCount);
	if(pBestResult && oneof3(rS.StakeMode, 1, 2, 3)) {
		rBuf.Space().CatChar('[');
		if(rS.StakeMode == 1)
			rBuf.Cat(pBestResult->TvForMaxResult, trange_fmt).CatChar('|').Cat(rS.OptDeltaRange, trange_fmt);
		else if(rS.StakeMode == 2)
			rBuf.Cat(pBestResult->TvForMaxResult, trange_fmt).CatChar('|').Cat(rS.OptDelta2Stride).CatChar('/').Cat(rS.OptDelta2Range, trange_fmt);
		else if(rS.StakeMode == 3) {
			rBuf.Cat(pBestResult->TvForMaxResult, trange_fmt).CatChar('|').Cat(rS.OptDeltaRange, trange_fmt).CatChar(']').Space();
			rBuf.CatChar('[');
			rBuf.Cat(pBestResult->Tv2ForMaxResult, trange_fmt).CatChar('|').Cat(rS.OptDelta2Stride).CatChar('/').Cat(rS.OptDelta2Range, trange_fmt);
		}
		rBuf.CatChar(']').Space();
	}
	return rBuf;
}

int SLAPI TimeSeriesCache::FindOptimalStrategyAtStake(const TimeSeriesBlock & rBlk, const TsStakeEnvironment::Stake & rStk, TsStakeEnvironment::StakeRequestBlock & rResult) const
{
	int    ok = -1;
	const TsStakeEnvironment::Tick * p_tk = StkEnv.SearchTickBySymb(rBlk.T_.GetSymb());
	if(p_tk && rBlk.T_.GetCount() && oneof2(rStk.Type, TsStakeEnvironment::ordtBuy, TsStakeEnvironment::ordtSell)) {
		uint   vec_idx = 0;
		const  int  gvvir = rBlk.T_.GetValueVecIndex("close", &vec_idx);
		if(gvvir) {
			SString stk_symb;
			StkEnv.GetS(rStk.SymbP, stk_symb);
			const bool is_short = (rStk.Type == TsStakeEnvironment::ordtSell);
			const int prec = rBlk.PPTS.Prec;
			SString comment_buf;
			SString temp_buf;
			uint   external_max_duck_quant = 0;
			uint   external_target_quant = 0;
			double external_spike_quant = 0.0;
			BestStrategyBlock _best_result;
			StkEnv.GetS(rStk.CommentP, comment_buf);
			if(comment_buf.NotEmptyS()) {
				//temp_buf.Z().Cat("PPAT").Comma().Cat(r_s.MaxDuckQuant).Comma().Cat(r_s.TargetQuant).Comma().Cat(r_s.SpikeQuant, MKSFMTD(0, 9, 0));
				if(comment_buf.CmpPrefix("PPAT,", 0) == 0) {
					StringSet ss(',', comment_buf);
					for(uint ssp = 0, tn = 0; ss.get(&ssp, temp_buf); tn++) {
						if(tn == 0) {
						}
						else if(tn == 1)
							external_max_duck_quant = static_cast<uint>(temp_buf.ToLong());
						else if(tn == 2)
							external_target_quant = static_cast<uint>(temp_buf.ToLong());
						else if(tn == 3)
							external_spike_quant = temp_buf.ToReal();
					}
				}
			}
			for(uint sidx = 0; sidx < rBlk.Strategies.getCount(); sidx++) {
				const PPObjTimeSeries::Strategy & r_s = rBlk.Strategies.at(sidx);
				if((!is_short && !(r_s.BaseFlags & r_s.bfShort)) || (is_short && (r_s.BaseFlags & r_s.bfShort))) {
					double cr = 0.0;
					double _tv = 0.0; // Трендовое значение (вычисление зависит от модели)
					double _tv2 = 0.0; // Второе трендовое значение 
					if(IsStrategySuited(r_s) && PPObjTimeSeries::MatchStrategy(rBlk.TrendList, r_s, cr, _tv, _tv2)) {
						_best_result.SetResult(cr, sidx, _tv, _tv2);
					}
				}
			}
			const PPObjTimeSeries::Strategy * p_st = (_best_result.MaxResultIdx >= 0) ? &rBlk.Strategies.at(_best_result.MaxResultIdx) : 0;
			if(p_st) {
				double last_value = 0.0;
				rBlk.T_.GetValue(rBlk.T_.GetCount()-1, vec_idx, &last_value);
				const double external_sl = (external_max_duck_quant > 0 && external_spike_quant > 0.0) ?
					PPObjTimeSeries::Strategy::CalcSL_withExternalFactors(last_value, is_short, prec, external_max_duck_quant, external_spike_quant) : 0.0;
				const double external_tp = (external_target_quant > 0 && external_spike_quant > 0.0) ? 
					PPObjTimeSeries::Strategy::CalcTP_withExternalFactors(rStk.PriceOpen, is_short, prec, external_target_quant, external_spike_quant) : 0.0;
				const double __sl = p_st->CalcSL(last_value);
				const double __tp = p_st->CalcTP(last_value);
				//const double eff_sl = (__sl > 0.0 && external_sl > 0.0) ? (is_short ? MIN(__sl, external_sl) : MAX(__sl, external_sl)) : ((__sl > 0.0) ? __sl : external_sl);
				const double eff_sl = (external_sl > 0.0) ? external_sl : ((__sl > 0.0) ? __sl : rStk.SL);
				//const double eff_tp = (__tp > 0.0 && external_tp > 0.0) ? (is_short ? MAX(__tp, external_tp) : MIN(__tp, external_tp)) : ((__tp > 0.0) ? __tp : external_tp);
				const double eff_tp = (rStk.TP > 0.0) ? rStk.TP : ((__tp > 0.0) ? __tp : 0.0); // Установленный TP менять не будем
				if(eff_sl > 0.0 || eff_tp > 0.0) {
					const bool external_sl_used = (eff_sl == external_sl) ? true : false;
					const bool external_tp_used = (eff_tp == external_tp) ? true : false;
					const bool do_update_sl = (rStk.SL <= 0.0 && eff_sl > 0.0) ? true : (is_short ? (eff_sl < rStk.SL) : (eff_sl > rStk.SL));
					const bool do_update_tp = (rStk.TP <= 0.0 && eff_tp > 0.0) ? true : (is_short ? (eff_tp > rStk.TP) : (eff_tp < rStk.TP));
					if(do_update_sl || do_update_tp) {
						{
							SString log_msg;
							const long trange_fmt = MKSFMTD(0, 10, NMBF_NOTRAILZ);
							log_msg.Z().Cat("StakeUpd").CatDiv(':', 2).
								Cat(stk_symb).CatChar('-').
								Cat(is_short ? "S" : "B").CatChar('/').
								Cat(rStk.VolumeCurrent, MKSFMTD(0, 3, NMBF_NOTRAILZ)).CatChar('/').
								Cat(last_value, MKSFMTD(0, 5, NMBF_NOTRAILZ)).CatChar('/').
								Cat(rStk.SL, MKSFMTD(0, 7, NMBF_NOTRAILZ)).CatChar(':').Cat(rStk.TP, MKSFMTD(0, 7, NMBF_NOTRAILZ)).
								Cat(" >> ").
								Cat(eff_sl, MKSFMTD(0, 7, NMBF_NOTRAILZ));
								if(external_sl_used)
									log_msg.Cat("[*]");
								log_msg.CatChar(':').
								Cat(eff_tp, MKSFMTD(0, 7, NMBF_NOTRAILZ));
								if(external_tp_used)
									log_msg.Cat("[*]");
							if(!external_sl_used || !external_tp_used) {
								log_msg.Space().Cat(StrategyToString(*p_st, &_best_result, temp_buf));
							}
							log_msg.Space().Cat(comment_buf);
							PPLogMessage(PPFILNAM_TSSTAKE_LOG, log_msg, LOGMSGF_TIME|LOGMSGF_DBINFO);
						}
						TsStakeEnvironment::StakeRequestBlock::Req req;
						req.Ticket = rStk.Ticket;
						req.Action = TsStakeEnvironment::traSLTP;
						req.Type = rStk.Type;
						req.TsID = rBlk.PPTS.ID;
						rResult.AddS(stk_symb, &req.SymbolP);
						req.Volume = 0;
						req.SL = do_update_sl ? eff_sl : rStk.SL;
						req.TP = do_update_tp ? eff_tp : rStk.TP;
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
	
int SLAPI TimeSeriesCache::FindOptimalStrategyForStake(PotentialStakeEntry & rPse) const
{
	int    ok = -1;
	const TimeSeriesBlock & r_blk = rPse.R_Blk;
	const TsStakeEnvironment::Tick * p_tk = StkEnv.SearchTickBySymb(r_blk.T_.GetSymb());
	if(p_tk && !(r_blk.Flags & r_blk.fDisableStake) && (r_blk.Flags & (r_blk.fLong|r_blk.fShort)) && r_blk.T_.GetCount()) {
		rPse.MarginReq = p_tk->MarginReq;
		const uint   max_stake = (Cfg.MaxStakeCount > 0) ? Cfg.MaxStakeCount : 10;
		const double max_balance_part = (Cfg.AvailableLimitPart > 0.0 && Cfg.AvailableLimitPart <= 1.0) ? Cfg.AvailableLimitPart : 0.5;
		const double max_balance_abs = (Cfg.AvailableLimitAbs > 0.0) ? Cfg.AvailableLimitAbs : 0.0;
		double insurance_balance = 0.0;
		if(max_balance_abs > 0.0)
			insurance_balance = StkEnv.Acc.Balance - max_balance_abs;
		else if(max_balance_part > 0.0 && max_balance_part <= 1.0)
			insurance_balance = StkEnv.Acc.Balance * (1.0 - max_balance_part);
		const double margin_available = (StkEnv.Acc.Margin + StkEnv.Acc.MarginFree - insurance_balance) / max_stake;
		if(margin_available > 0.0 && margin_available < StkEnv.Acc.MarginFree) {
			BestStrategyBlock _best_result;
			for(uint sidx = 0; sidx < r_blk.Strategies.getCount(); sidx++) {
				const PPObjTimeSeries::Strategy & r_s = r_blk.Strategies.at(sidx);
				if(((r_s.BaseFlags & r_s.bfShort) && (r_blk.Flags & r_blk.fShort)) || (!(r_s.BaseFlags & r_s.bfShort) && (r_blk.Flags & r_blk.fLong))) {
					double cr = 0.0;
					double _tv = 0.0; // Трендовое значение (вычисление зависит от модели)
					double _tv2 = 0.0; // Второе трендовое значение (вычисляется для комбинированной модели)
					if(IsStrategySuited(r_s) && PPObjTimeSeries::MatchStrategy(r_blk.TrendList, r_s, cr, _tv, _tv2))
						_best_result.SetResult(cr, sidx, _tv, _tv2);
				}
			}
			if(_best_result.MaxResultIdx >= 0) {
				SString log_msg;
				SString temp_buf;
				assert(_best_result.MaxResult >= Cfg.MinPerDayPotential);
				const PPObjTimeSeries::Strategy & r_s = r_blk.Strategies.at(_best_result.MaxResultIdx);
				const bool is_sell = (r_s.BaseFlags & r_s.bfShort) ? true : false;
				//
				// Блок для анализа возможной "шизофрении" алгоритма, в результате которой будет выбран лучший результат
				// при наличии реверсивного алгоритма, сходного по потенциальному результату.
				// Пока только вывод в журнал.
				//
				BestStrategyBlock _best_result_reverse; // Лучший результат для реверсивной ставки
				{
					for(uint sidx = 0; sidx < r_blk.Strategies.getCount(); sidx++) {
						const PPObjTimeSeries::Strategy & r_s_reverse = r_blk.Strategies.at(sidx);
						if(((r_s_reverse.BaseFlags & r_s_reverse.bfShort) && !is_sell) || (!(r_s_reverse.BaseFlags & r_s_reverse.bfShort) && is_sell)) {
							double cr = 0.0;
							double _tv = 0.0; // Трендовое значение (вычисление зависит от модели)
							double _tv2 = 0.0; // Второе трендовое значение (вычисляется для комбинированной модели)
							if(IsStrategySuited(r_s_reverse) && PPObjTimeSeries::MatchStrategy(r_blk.TrendList, r_s_reverse, cr, _tv, _tv2))
								_best_result_reverse.SetResult(cr, sidx, _tv, _tv2);
						}
					}
				}
				//
				rPse.StrategyPos = (uint)_best_result.MaxResultIdx;
				rPse.ResultPerDay = _best_result.MaxResult;
				const double min_cost = EvaluateCost(r_blk, is_sell, r_blk.VolumeMin);
				double __volume = 0.0;
				double __adjusted_result_per_day = 0.0;
				if(min_cost > 0.0) {
					const double margin = (is_sell ? r_blk.PPTS.SellMarg : r_blk.PPTS.BuyMarg);
					double sc = floor(margin_available / min_cost);
					__volume = R0(r_blk.VolumeStep * sc);
					SETMIN(__volume, r_blk.VolumeMax);
					if(__volume > 0.0) {
						__adjusted_result_per_day = EvaluateCost(r_blk, is_sell, __volume) * rPse.ResultPerDay;
						//Cfg.MinPerDayPotential
						int use_it = 1;
						if(_best_result_reverse.MaxResultIdx >= 0) { use_it = 0; }
						//if(__adjusted_result_per_day >= 10.0) { use_it = 0; }
						if(use_it) {
							rPse.Volume = __volume;
							rPse.AdjustedResultPerDay = __adjusted_result_per_day;
							rPse.Tv  = _best_result.TvForMaxResult;
							rPse.Tv2 = _best_result.Tv2ForMaxResult;
							ok = 1;
						}
					}
				}
				//
				// CatCharN(' ', 8) для выравнивания с вероятным выводом reverse-результата
				log_msg.Z().Cat("Best").CatCharN(' ', 8).CatDiv(':', 2).Cat(r_blk.PPTS.Symb).Space().
					CatEq("min-cost", min_cost, MKSFMTD(0, 5, 0)).Space().
					CatEq("volume", __volume, MKSFMTD(0, 0, 0)).Space().
					CatEq("adjusted-result-per-day", __adjusted_result_per_day, MKSFMTD(0, 5, NMBF_NOTRAILZ)).Space().
					Cat(StrategyToString(r_s, &_best_result, temp_buf));
				PPLogMessage(PPFILNAM_TSSTAKEPOTENTIAL_LOG, log_msg, LOGMSGF_TIME|LOGMSGF_DBINFO);
				//
				if(_best_result_reverse.MaxResultIdx >= 0) {
					const PPObjTimeSeries::Strategy & r_s_reverse = r_blk.Strategies.at(_best_result_reverse.MaxResultIdx);
					log_msg.Z().Cat("Best-Reverse").CatDiv(':', 2).Cat(r_blk.PPTS.Symb).Space().Cat(StrategyToString(r_s_reverse, &_best_result_reverse, temp_buf));
					PPLogMessage(PPFILNAM_TSSTAKEPOTENTIAL_LOG, log_msg, LOGMSGF_TIME|LOGMSGF_DBINFO);
				}
				//
			}
		}
	}
	return ok;
}
	
double SLAPI TimeSeriesCache::EvaluateCost(const TimeSeriesBlock & rBlk, bool sell, double volume) const
{
	const char * p_base_symb = "USD";
	double cost = 0.0;
	double last_value = 0.0;
	if(!isempty(rBlk.PPTS.CurrencySymb) && rBlk.GetLastValue(&last_value)) {
		const SString symb = rBlk.T_.GetSymb();
		const double margin = (sell ? rBlk.PPTS.SellMarg : rBlk.PPTS.BuyMarg);
		if(!sstreqi_ascii(rBlk.PPTS.CurrencySymb, p_base_symb)) {
			if(symb.CmpPrefix(p_base_symb, 1) == 0)
				cost = volume * margin / last_value;
			else {
				int    reverse_rate = 0;
				double last_cvt_value = 0.0;
				const TimeSeriesBlock * p_cvt_blk = SearchRateConvertionBlock(rBlk.PPTS.CurrencySymb, &reverse_rate);
				if(p_cvt_blk && p_cvt_blk->GetLastValue(&last_cvt_value) && last_cvt_value > 0.0) {
					if(reverse_rate)
						cost = volume * margin * last_value / last_cvt_value;
					else
						cost = volume * margin * last_value * last_cvt_value;
				}
			}
		}
		else
			cost = volume * margin * last_value;
	}
	return cost;
}
	
int SLAPI TimeSeriesCache::EvaluateStakes(TsStakeEnvironment::StakeRequestBlock & rResult)
{
	int    ok = -1;
	{
		PPUserFuncProfiler ufp(PPUPRF_TSEVALSTAKES); // @v10.3.3
		const  LDATETIME now = getcurdatetime_();
		SString temp_buf;
		SString log_msg;
		SString tk_symb;
		SString stk_symb;
		SString stk_memo;
		TSVector <PotentialStakeEntry> potential_stake_list;
		const uint current_stake_count = StkEnv.SL.getCount();
		for(uint i = 0; i < StkEnv.TL.getCount(); i++) {
			const TsStakeEnvironment::Tick & r_tk = StkEnv.TL.at(i);
			long d = diffdatetimesec(now, r_tk.Dtm);
			if(d < 10) { // must be 10 (600 - for debug)
				StkEnv.GetS(r_tk.SymbP, tk_symb);
				uint  blk_idx = 0;
				const TimeSeriesBlock * p_blk = SearchBlockBySymb(tk_symb, &blk_idx);
				if(p_blk) {
					int   is_there_stake = 0;
					for(uint si = 0; si < current_stake_count; si++) {
						const TsStakeEnvironment::Stake & r_stk = StkEnv.SL.at(si);
						StkEnv.GetS(r_stk.SymbP, stk_symb);
						if(stk_symb.IsEqiAscii(tk_symb)) {
							FindOptimalStrategyAtStake(*p_blk, r_stk, rResult);
							is_there_stake = 1;
						}
					}
					if(!is_there_stake && current_stake_count < Cfg.MaxStakeCount) {
						PotentialStakeEntry pse(*p_blk);
						if(FindOptimalStrategyForStake(pse) > 0)
							potential_stake_list.insert(&pse);
					}
				}
			}
		}
		{
			const uint pslc = potential_stake_list.getCount();
			if(pslc) {
				uint new_stake_count = 0;
				potential_stake_list.sort(PTR_CMPFUNC(double));
				double margin_available = (StkEnv.Acc.MarginFree - StkEnv.Acc.Balance/2.0);
				if(margin_available > 0.0 && (current_stake_count+new_stake_count) < Cfg.MaxStakeCount) {
					uint _pslp = pslc;
					do {
						PotentialStakeEntry & r_pse = potential_stake_list.at(--_pslp);
						double last_value = 0.0;
						if(r_pse.R_Blk.GetLastValue(&last_value)) {
							const PPObjTimeSeries::Strategy & r_s = r_pse.R_Blk.Strategies.at(r_pse.StrategyPos);
							stk_symb = r_pse.R_Blk.T_.GetSymb();
							const double cost = EvaluateCost(r_pse.R_Blk, ((r_s.BaseFlags & r_s.bfShort) ? true : false), r_pse.Volume);
							if(cost > 0.0) {
								//r_pse.R_Blk.T_.GetValue(r_pse.R_Blk.T_.GetCount()-1, vec_idx, &last_value);
								const double sl = r_s.CalcSL(last_value);
								const double tp = r_s.CalcTP(last_value);
								const long trange_fmt = MKSFMTD(0, 10, NMBF_NOTRAILZ);
								log_msg.Z().Cat("Stake").CatDiv(':', 2).
									Cat(stk_symb).CatChar('-').
									Cat((r_s.BaseFlags & r_s.bfShort) ? "S" : "B").CatChar('/').
									Cat(r_pse.Volume, MKSFMTD(0, 3, NMBF_NOTRAILZ)).CatChar('/').
									Cat(last_value, MKSFMTD(0, 5, NMBF_NOTRAILZ)).CatChar('/').
									Cat(sl, MKSFMTD(0, 7, NMBF_NOTRAILZ)).CatChar('/').
									Cat(tp, MKSFMTD(0, 7, NMBF_NOTRAILZ));

								{
									BestStrategyBlock fake_bsb;
									fake_bsb.TvForMaxResult = r_pse.Tv;
									fake_bsb.Tv2ForMaxResult = r_pse.Tv2;
									log_msg.Space().Cat(StrategyToString(r_s, &fake_bsb, temp_buf));
								}
								temp_buf.Z().Cat("PPAT").Comma().Cat(r_s.MaxDuckQuant).Comma().Cat(r_s.TargetQuant).Comma().Cat(r_s.SpikeQuant, MKSFMTD(0, 9, 0));
								log_msg.Space().Cat(temp_buf);
								PPLogMessage(PPFILNAM_TSSTAKE_LOG, log_msg, LOGMSGF_TIME|LOGMSGF_DBINFO);
								if(!(Cfg.Flags & Cfg.fTestMode)) {
									TsStakeEnvironment::StakeRequestBlock::Req req;
									req.Action = TsStakeEnvironment::traDeal;
									req.Type = (r_s.BaseFlags & r_s.bfShort) ? TsStakeEnvironment::ordtSell : TsStakeEnvironment::ordtBuy;
									req.TsID = r_pse.R_Blk.PPTS.ID;
									rResult.AddS(stk_symb, &req.SymbolP);
									req.Volume = r_pse.Volume;
									req.SL = sl;
									req.TP = tp;
									rResult.AddS(temp_buf, &req.CommentP);
									rResult.L.insert(&req);
								}
								new_stake_count++;
								margin_available -= cost;
							}
						}
					} while(_pslp && margin_available > 0.0);
				}
			}
		}
		ufp.SetFactor(0, static_cast<double>(StkEnv.TL.getCount()));
		ufp.Commit();
	}
	return ok;
}

int SLAPI TimeSeriesCache::FetchEntry(PPID id, ObjCacheEntry * pEntry, long)
{
	int    ok = 1;
	Data * p_cache_rec = static_cast<Data *>(pEntry);
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
	const Data * p_cache_rec = static_cast<const Data *>(pEntry);
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

const TimeSeriesCache::TimeSeriesBlock * SLAPI TimeSeriesCache::SearchRateConvertionBlock(const char * pSymb, int * pRevese) const
{
	TimeSeriesBlock * p_result = 0;
	int    reverse = 0;
	const size_t symb_len = sstrlen(pSymb);
	if(symb_len) {
		const char * p_base_symb = "USD";
		const size_t base_symb_len = sstrlen(p_base_symb);
		SString & r_temp_buf = SLS.AcquireRvlStr();
		for(uint i = 0; !p_result && i < TsC.getCount(); i++) {
			TimeSeriesBlock * p_blk = TsC.at(i);
			if(p_blk) {
				r_temp_buf = p_blk->T_.GetSymb();
				if(r_temp_buf.Len() == (symb_len+base_symb_len)) {
					if(r_temp_buf.CmpPrefix(pSymb, 1) == 0 && r_temp_buf.CmpSuffix(p_base_symb, 1) == 0) {
						p_result = p_blk;
						reverse = 0;
					}
					else if(r_temp_buf.CmpPrefix(p_base_symb, 1) == 0 && r_temp_buf.CmpSuffix(pSymb, 1) == 0) {
						p_result = p_blk;
						reverse = 1;
					}
				}
			}
		}
	}
	ASSIGN_PTR(pRevese, reverse);
	return p_result;
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
				PPObjTimeSeries::TrendEntry * p_entry = 0;
				for(uint j = 0; !p_entry && j < pBlk->TrendList.getCount(); j++) {
					PPObjTimeSeries::TrendEntry * p_iter = pBlk->TrendList.at(j);
					if(p_iter && p_iter->Stride == ifs)
						p_entry = p_iter;
				}
				if(!p_entry) {
					THROW_SL(p_entry = new PPObjTimeSeries::TrendEntry(ifs, trend_nominal_count));
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

TimeSeriesCache::TimeSeriesBlock * SLAPI TimeSeriesCache::InitBlock(PPObjTimeSeries & rTsObj, const char * pSymb)
{
	TimeSeriesBlock * p_fblk = 0;
	PROFILE_START
	PPID   id = 0;
	PPTimeSeries ts_rec;
	STimeSeries ts_full;
	int r = rTsObj.SearchBySymb(pSymb, &id, &ts_rec);
	THROW(r);
	if(r < 0) {
		id = 0;
		MEMSZERO(ts_rec);
		STRNSCPY(ts_rec.Name, pSymb);
		STRNSCPY(ts_rec.Symb, pSymb);
		THROW(rTsObj.PutPacket(&id, &ts_rec, 1));
		ts_rec.ID = id;
	}
	THROW_SL(p_fblk = TsC.CreateNewItem());
	p_fblk->T_.SetSymb(pSymb);
	p_fblk->PPTS = ts_rec;
	THROW(r = rTsObj.GetTimeSeries(id, ts_full));
	if(r > 0) {
		const uint full_tsc = ts_full.GetCount();
		ts_full.GetChunkRecentCount(MIN(10000, full_tsc), p_fblk->T_);
		rTsObj.GetStrategies(id, p_fblk->Strategies);
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
