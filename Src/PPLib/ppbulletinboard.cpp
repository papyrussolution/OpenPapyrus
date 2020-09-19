// PPBulletinBoard.cpp
// Copyright (c) A.Sobolev 2018, 2019, 2020
// @codepage UTF-8
// Концепт доски объявлений, реализующей функционал общедоступных системных задач
//
#include <pp.h>
#pragma hdrstop

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

/*virtual*/PPBulletinBoard::Sticker * SLAPI PPBulletinBoard::Sticker::Dup() const
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
static const int UseRegularValuesOnlyForStakes = 1; // @v10.4.6 // @20200124 0-->1

class TimeSeriesCache : public ObjCache {
public:
	struct Data : public ObjCacheEntry {
		long   Flags;
		double BuyMarg;
		double SellMarg;
		double SpikeQuant_tc;  //
		double AvgSpread;      //
		uint32 OptMaxDuck;     // Оптимальная глубина проседания (в квантах) при длинной ставке
		uint32 OptMaxDuck_S;   // Оптимальная глубина проседания (в квантах) при короткой ставке
		int16  Prec;
		uint16 TargetQuant;    // @v10.4.2
		long   Type;           // @v10.5.6
	};
	struct ImpactEntry {
		ImpactEntry(LDATETIME t, double v) : T(t), Value(v)
		{
		}
		LDATETIME T;
		double Value;
	};
	struct TimeSeriesBlock {
		SLAPI  TimeSeriesBlock() : Flags(0), SpreadSum(0), SpreadCount(0), VolumeMin(0.0), VolumeMax(0.0), VolumeStep(0.0),
			Actual_Regular_Parity(0), LastStakeTime(ZERODATETIME)
		{
		}
		double SLAPI GetAverageSpread() const
		{
			return fdivui(SpreadSum, SpreadCount);
		}
		double SLAPI GetAverageSpread_WithAdjustment() const
		{
			// @20190703 2.0-->1.2 // @20190707 1.2-->1.5 // @20190708 1.5-->2.0 // @20200207 2.0-->1.2 // @20200329 1.2-->1.0
			return fdivui(SpreadSum, SpreadCount) * 1.0; 
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
			fLong           = 0x0001, // Допускается покупка
			fShort          = 0x0002, // Допускается продажа
			fDisableStake   = 0x0004, // Запрет на сделки
			fNoConfig       = 0x0008, // Символ не представлен в конфигурации
			fDirty          = 0x0100, // Данные по серии были изменены, но не сохранены в базе данных
			fVolumeMult1000 = 0x0200  // @v10.5.6 Параметра объема ставок заданы в масштабе 1/1000 (то есть, значения на сервере поделены на 1000)
				// Если макс объем 100 это значит, что мы должны умножить его на 1000 а при передаче на сервер поделить на 1000
		};
		long   Flags;
		// Следующие 2 поля нужны для расчета среднего спреда
		uint   SpreadSum; // Накопленная сумма величин спредов
		uint   SpreadCount; // Количество накопленных величин спредов
		double VolumeMin;
		double VolumeMax;
		double VolumeStep;
		LDATETIME LastStakeTime; // @v10.7.6 Время установки последней ставки - для предупреждения двойных ставок при задержках сети
		PPTimeSeriesPacket PPTS;
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
			SLAPI  TempLastEntry() : Tag(0)
			{
			}
			int    Tag; // 0 - empty, 1 - main series value, 2 - tick value
			STimeSeries::SnapshotEntry Sse;
		};
		TempLastEntry TLE;
		TSCollection <PPObjTimeSeries::TrendEntry> TrendList;
		PPObjTimeSeries::StrategyContainer Strategies;
		PPObjTimeSeries::StrategyContainer::Index1 StratIndex;

		long   Actual_Regular_Parity;
		void SLAPI Make_T_Regular()
		{
			if(!UseRegularValuesOnlyForStakes) {
				Actual_Regular_Parity--;
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
		}
		void SLAPI Make_T_Actual()
		{
			if(!UseRegularValuesOnlyForStakes) {
				Actual_Regular_Parity++;
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
		}
	};
	struct PotentialStakeEntry {
		explicit SLAPI PotentialStakeEntry(const TimeSeriesBlock & rBlk) : R_Blk(rBlk), Flags(0), StrategyPos(0), Volume(0.0),
			ResultPerDay(0.0), MarginReq(0.0), Tv(0.0), Tv2(0.0), ArrangeCritValue(0.0), AdjustedResultPerDay(0.0)
		{
		}
		enum {
			fForce        = 0x0001, // Форсированная ставка (должна быть применена обязательно)
			fLackOfMargin = 0x0002  // Стратегия найдена, но для ее реализации не достаточно доступных ресурсов
		};
		double ArrangeCritValue;     // @firstmember Значение критерия арранжировки списка для приоритета использования (больше - лучше)
		double AdjustedResultPerDay; // Результат, умноженный на используемую маржу
		double ResultPerDay;         //
		const  TimeSeriesBlock & R_Blk;
		long   Flags;
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
			const LDATETIME cdtm = getcurdatetime_();
			const long sec = diffdatetimesec(cdtm, p_this->LastFlashDtm);
			if(sec >= p_this->GetFlashTimeout())
				p_this->Flash();
		}
		return 1;
	}
	static int OnConfigUpdated(int kind, const PPNotifyEvent * pEv, void * procExtPtr)
	{
		TimeSeriesCache * p_this = static_cast<TimeSeriesCache *>(procExtPtr);
		if(p_this && pEv->ObjType == PPCFGOBJ_TIMESERIES) {
			if(!pEv->IsFinish()) {
				p_this->OpL.Lock();
				p_this->LoadConfig(1);
				p_this->OpL.Unlock();
			}
		}
		return 1;
	}
	static int OnStrategyUpdated(int kind, const PPNotifyEvent * pEv, void * procExtPtr)
	{
		TimeSeriesCache * p_this = static_cast<TimeSeriesCache *>(procExtPtr);
		if(p_this && pEv->ObjType == PPOBJ_TIMESERIES && pEv->ObjID) {
			if(!pEv->IsFinish()) {
				PPObjTimeSeries ts_obj;
				p_this->OpL.Lock();
				p_this->LoadStrategies(ts_obj, pEv->ObjID);
				p_this->OpL.Unlock();
			}
		}
		return 1;
	}
	TimeSeriesCache();
	~TimeSeriesCache();
	int    SLAPI SetTimeSeries(STimeSeries & rTs);
	int    SLAPI GetReqQuotes(TSVector <PPObjTimeSeries::QuoteReqEntry> & rList);
	int    SLAPI SetCurrentStakeEnvironment(const TsStakeEnvironment * pEnv, TsStakeEnvironment::StakeRequestBlock * pRet);
	int    SLAPI SetTransactionNotification(const TsStakeEnvironment::TransactionNotification * pTan);
	int    SLAPI SetVolumeParams(const char * pSymb, double volumeMin, double volumeMax, double volumeStep);
	int    SLAPI Flash();
	int    SLAPI FindOptimalStrategyAtStake(const TimeSeriesBlock & rBlk, const TsStakeEnvironment::Stake & rStk, 
		PotentialStakeEntry * pPse, TsStakeEnvironment::StakeRequestBlock & rResult) const;
	int    SLAPI FindOptimalStrategyForStake(const double evaluatedUsedMargin, PotentialStakeEntry & rPse) const;
	enum {
		ecfTrickChf = 0x0001 // Специальный флаг, предписывающий увеличивать плечо для CHF-пар на форексе
	};
	double SLAPI EvaluateCost(const TimeSeriesBlock & rBlk, bool sell, double volume, uint flags) const;
	int    SLAPI EvaluateStakes(TsStakeEnvironment::StakeRequestBlock & rResult) const;
	double SLAPI EvaluateUsedMargin() const;
private:
	void   SLAPI LogStateEnvironment(const TsStakeEnvironment & rEnv);
	virtual int  SLAPI FetchEntry(PPID, ObjCacheEntry * pEntry, long);
	virtual void SLAPI EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const;
	int    SLAPI LoadConfig(int force);
	int    SLAPI Helper_LoadStrategies(PPObjTimeSeries & rTsObj, PPID tsID, TimeSeriesBlock * pBlk)
	{
		int    ok = 1;
		if(pBlk) {
			assert(tsID == pBlk->PPTS.Rec.ID);
			THROW(tsID == pBlk->PPTS.Rec.ID)
			THROW(rTsObj.GetStrategies(tsID, PPObjTimeSeries::sstSelection, pBlk->Strategies));
			pBlk->Strategies.CreateIndex1(pBlk->StratIndex);
			pBlk->TrendList.freeAll();
			THROW(EvaluateTrends(pBlk, 0, 0/*onActualTicks*/));
			{
				SString log_buf;
				log_buf.Cat("Strategis for").Space().Cat(pBlk->PPTS.GetSymb()).Space().Cat("have been loaded");
				log_buf.CatDiv(':', 2).CatEq("Ver", pBlk->Strategies.GetVersion());
				log_buf.Space().CatEq("Count", pBlk->Strategies.getCount());
				if(!!pBlk->Strategies.GetStorageTm())
					log_buf.Space().CatEq("storage-time", pBlk->Strategies.GetStorageTm(), DATF_ISO8601|DATF_CENTURY, 0);
				if(!!pBlk->Strategies.GetLastValTm())
					log_buf.Space().CatEq("last-val-time", pBlk->Strategies.GetLastValTm(), DATF_ISO8601|DATF_CENTURY, 0);
				if(!!pBlk->Strategies.GetUseDataForStrategiesTill())
					log_buf.Space().CatEq("use-data-for-strategies-till", pBlk->Strategies.GetUseDataForStrategiesTill(), DATF_ISO8601|DATF_CENTURY);
				PPLogMessage(PPFILNAM_TSSTAKE_LOG, log_buf, LOGMSGF_TIME|LOGMSGF_DBINFO);
			}
		}
		else
			ok = -1;
		CATCHZOK
		return ok;
	}
	int    SLAPI LoadStrategies(PPObjTimeSeries & rTsObj, PPID tsID)
	{
		int    ok = -1;
		for(uint i = 0; ok < 0 && i < TsC.getCount(); i++) {
			TimeSeriesBlock * p_local_blk = TsC.at(i);
			if(p_local_blk && p_local_blk->PPTS.Rec.ID == tsID) {
				THROW(Helper_LoadStrategies(rTsObj, tsID, p_local_blk));
				ok = 1;
				break;
			}
		}
		CATCHZOK
		return ok;
	}
	TimeSeriesBlock * SLAPI SearchBlockBySymb(const char * pSymb, uint * pIdx) const;
	const TimeSeriesBlock * SLAPI SearchRateConvertionBlock(const char * pSymb, int * pRevese) const;
	TimeSeriesBlock * SLAPI InitBlock(PPObjTimeSeries & rTsObj, const char * pSymb);
	//
	// ARG(onActualTicks IN): Расчет ведется на данных, полученных из последних актуальных тиков (вероятно, необходимо внесение поправки в результаты расчетов)
	//
	int    SLAPI EvaluateTrends(TimeSeriesBlock * pBlk, const STimeSeries * pFullTs, int onActualTicks);
	long   SLAPI GetFlashTimeout() const
	{
		const long cfg_tmr = Cfg.E.TsFlashTimer;
		return checkirange(cfg_tmr, 1, (24*3600)) ? cfg_tmr : 600;
	}
	int    FASTCALL IsStrategySuited(const PPObjTimeSeries::Strategy & rS) const;

	TSCollection <TimeSeriesBlock> TsC;
	TsStakeEnvironment StkEnv;
	SMtLock OpL; // Блокировка для операций, иных нежели штатные методы ObjCache
	LDATETIME LastFlashDtm;
	enum {
		stConfigLoaded = 0x0001,
		stDataTestMode = 0x0002 // @v10.4.0 Режим тестирования данных. Отправляется запрос на большее количество значений и 
			// после получения результата тестируются внутренние последовательности.
	};
	long   State;
	PPObjTimeSeries::Config Cfg;
};

void SLAPI TimeSeriesCache::LogStateEnvironment(const TsStakeEnvironment & rEnv)
{
	SString file_name;
	SString line_buf;
	SString temp_buf;
	SString comment_buf;
	PPGetFilePath(PPPATH_LOG, "TsStakeEnvironment.log", file_name);
	SFile f_out(file_name, SFile::mAppend);
	const double evaluated_used_margin = EvaluateUsedMargin();
	line_buf.Z().Cat(getcurdatetime_(), DATF_ISO8601|DATF_CENTURY, 0).Space().Cat("Account").CatDiv(':', 2)/*.Cat(rEnv.Acc.ActualDtm, DATF_ISO8601|DATF_CENTURY, 0)*/.Space().
		Cat(rEnv.Acc.ID).Space().
		CatEq("Balance", rEnv.Acc.Balance, MKSFMTD(0, 2, 0)).Space().
		CatEq("MarginFree", rEnv.Acc.MarginFree, MKSFMTD(0, 2, 0)).Space().
		CatEq("EvaluatedUsedMargin", evaluated_used_margin, MKSFMTD(0, 5, 0)).Space().
		CatEq("Profit", rEnv.Acc.Profit, MKSFMTD(0, 2, 0));
	f_out.WriteLine(line_buf.CR());
	{
		for(uint i = 0; i < rEnv.SL.getCount(); i++) {
			const TsStakeEnvironment::Stake & r_stk = rEnv.SL.at(i);
			rEnv.GetS(r_stk.SymbP, temp_buf);
			rEnv.GetS(r_stk.CommentP, comment_buf);
			double profit = (r_stk.PriceCurrent - r_stk.PriceOpen) * r_stk.VolumeInit;
			if(r_stk.Type == TsStakeEnvironment::ordtSell)
				profit = -profit;
			line_buf.Z().Cat("Stake").CatDiv(':', 2).
				Cat(temp_buf.Align(10, ADJ_LEFT)).Space().
				Cat(r_stk.Type).Space().
				Cat(r_stk.Ticket).Space().
				Cat(r_stk.SetupDtm, DATF_ISO8601|DATF_CENTURY, 0).Space().
				Cat(r_stk.VolumeInit,   MKSFMTD(10, 3, 0)).Space().
				Cat(r_stk.PriceOpen,    MKSFMTD(10, 5, 0)).Space().
				Cat(r_stk.SL, MKSFMTD(10, 5, 0)).Space().
				Cat(r_stk.TP, MKSFMTD(10, 5, 0)).Space().
				Cat(r_stk.PriceCurrent, MKSFMTD(10, 5, 0)).Space().
				Cat(r_stk.Profit,       MKSFMTD(6, 2, NMBF_FORCEPOS)).Space().
				Cat(comment_buf);
			f_out.WriteLine(line_buf.CR());
		}
	}
#if 0 // {
	{
		for(uint i = 0; i < rEnv.TL.getCount(); i++) {
			const TsStakeEnvironment::Tick & r_tk = rEnv.TL.at(i);
			rEnv.GetS(r_tk.SymbP, temp_buf);
			TimeSeriesBlock * p_fblk = SearchBlockBySymb(temp_buf, 0);
			line_buf.Z().Cat("Tick").CatDiv(':', 2).
				Cat(temp_buf.Align(10, ADJ_LEFT)).Space().
				Cat(r_tk.TsID).Space().
				Cat(r_tk.Dtm, DATF_ISO8601|DATF_CENTURY, 0).Space().
				Cat(r_tk.Bid,  MKSFMTD(10, 5, 0)).Space().
				Cat(r_tk.Ask,  MKSFMTD(10, 5, 0)).Space().
				Cat(r_tk.Last, MKSFMTD(10, 5, 0)).Space().
				Cat(static_cast<double>(r_tk.Volume), MKSFMTD(6, 0, 0)).Space().
				Cat(r_tk.MarginReq, MKSFMTD(8, 2, 0));
			if(p_fblk) {
				temp_buf.Z();
				for(uint j = 0; j < p_fblk->TrendList.getCount(); j++) {
					const PPObjTimeSeries::TrendEntry * p_iter = p_fblk->TrendList.at(j);
					if(p_iter && p_iter->TL.getCount()) {
						temp_buf.CatLongZ(p_iter->Stride, 3).CatChar(':');
						temp_buf.Cat(p_iter->TL[p_iter->TL.getCount()-1], MKSFMTD(14, 10, 0)).CatChar('|');
						/*for(uint k = 0; k < p_iter->TL.getCount(); k++) {
							temp_buf.Cat(p_iter->TL[k], MKSFMTD(14, 10, 0)).CatChar('|');
						}*/
					}
				}
				line_buf.Space().Cat(temp_buf);
			}
			f_out.WriteLine(line_buf.CR());
		}
	}
#endif // } 0
}

TimeSeriesCache::TimeSeriesCache() : ObjCache(PPOBJ_TIMESERIES, sizeof(Data)), LastFlashDtm(getcurdatetime_()), /*P_ReqQList(0),*/ State(0)
{
	{
		long   cookie = 0;
		PPAdviseBlock adv_blk;
		// @v10.6.4 MEMSZERO(adv_blk);
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
		// @v10.6.4 MEMSZERO(adv_blk);
		adv_blk.Kind = PPAdviseBlock::evDirtyCacheBySysJ;
		adv_blk.DbPathID = DBS.GetDbPathID();
		adv_blk.ObjType = PPCFGOBJ_TIMESERIES;
		adv_blk.Action = PPACN_CONFIGUPDATED; // @v10.3.11
		adv_blk.Proc = TimeSeriesCache::OnConfigUpdated;
		adv_blk.ProcExtPtr = this;
		DS.Advise(&cookie, &adv_blk);
	}
	{
		long   cookie = 0;
		PPAdviseBlock adv_blk;
		// @v10.6.4 MEMSZERO(adv_blk);
		adv_blk.Kind = PPAdviseBlock::evDirtyCacheBySysJ;
		adv_blk.DbPathID = DBS.GetDbPathID();
		adv_blk.ObjType = 0; // @v10.8.8 @fix PPCFGOBJ_TIMESERIES-->0
		adv_blk.Action = PPACN_TSSTRATEGYUPD;
		adv_blk.Proc = TimeSeriesCache::OnStrategyUpdated;
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
	SString temp_buf(rTs.GetSymb());
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
#if 0 // @construction {
			if(State & stDataTestMode) {
				uint   outer_vec_idx = 0;
				uint   my_vec_idx = 0;
				if(rTs.GetValueVecIndex("close", &outer_vec_idx) && p_fblk->T_.GetValueVecIndex("close", &my_vec_idx)) {
					const uint outer_c = rTs.GetCount();
					const uint my_c = p_fblk->T_.GetCount();
					int   first_outer_x_idx = -1;
					int   last_outer_x_idx = -1;
					SUniTime ut;
					SUniTime outer_ut;
					for(uint midx = 0; midx < my_c; midx++) {
						if(p_fblk->T_.GetTime(midx, &ut)) {
							uint ii = 0;
							const int  ser = rTs.SearchEntryBinary(ut, &ii);
							if(ser > 0) {
								if(first_outer_x_idx < 0)
									first_outer_x_idx = static_cast<int>(ii);
								last_outer_x_idx = static_cast<int>(ii);
								//
								double outer_val = 0.0;
								double my_val = 0.0;
								rTs.GetTime(ii, &outer_ut);
								rTs.GetValue(ii, outer_vec_idx, &outer_val);
								p_fblk->T_.GetValue(midx, my_vec_idx, &my_val);
								if(!feqeps(my_val, outer_val, 1E-6)) {
									// ...
								}
							}
							else {
								// ...
							}
						}
					}
					if(first_outer_x_idx >= 0) {
						for(uint oidx = static_cast<uint>(first_outer_x_idx); oidx <= static_cast<uint>(last_outer_x_idx); oidx++) {
							if(rTs.GetTime(oidx, &outer_ut)) {
								uint ii = 0;
								const int  ser = p_fblk->T_.SearchEntryBinary(outer_ut, &ii);
								if(ser > 0) {
									// ok
								}
								else {
									// Отсутствующее значение
								}
							}
						}
					}
				}
			}
#endif // } 0 @construction
			uint   upd_stat_vec_idx = 0;
			if(p_fblk->T_.GetValueVecIndex("close", &upd_stat_vec_idx))
				apst.UpdCountVecIdx = static_cast<int>(upd_stat_vec_idx);
			else
				apst.UpdCountVecIdx = -1;
			local_ok = p_fblk->T_.AddItems(rTs, &apst);
			p_fblk->Make_T_Actual();
			EvaluateTrends(p_fblk, 0, 0/*onActualTicks*/);
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
				const PPObjTimeSeries::Config::Entry * p_cfg_entry = p_blk->PPTS.Rec.ID ? Cfg.SearchEntry(p_blk->PPTS.Rec.ID) : 0;
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
		PPLogMessage(PPFILNAM_TSSTAKE_LOG, "Config has loaded", LOGMSGF_TIME|LOGMSGF_DBINFO);
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
				SETFLAG(new_entry.Flags, new_entry.fTestPurpose, State & stDataTestMode);
				STRNSCPY(new_entry.Ticker, ts_rec.Symb);
				{
					int    is_first_call = 0;
					SUniTime last_utm;
					TimeSeriesBlock * p_blk = SearchBlockBySymb(ts_rec.Symb, 0);
					if(!p_blk) {
						is_first_call = 1;
						p_blk = InitBlock(ts_obj, ts_rec.Symb);
					}
					if(p_blk) {
						SETFLAG(p_blk->Flags, TimeSeriesBlock::fLong,  r_entry.Flags & PPObjTimeSeries::Config::efLong);
						SETFLAG(p_blk->Flags, TimeSeriesBlock::fShort, r_entry.Flags & PPObjTimeSeries::Config::efShort);
						SETFLAG(p_blk->Flags, TimeSeriesBlock::fDisableStake, r_entry.Flags & PPObjTimeSeries::Config::efDisableStake);
						p_blk->Flags &= ~TimeSeriesBlock::fNoConfig;
					}
					const uint tc = p_blk ? p_blk->T_.GetCount() : 0;
					if(tc && p_blk->T_.GetTime(tc-1, &last_utm)) {
						if(is_first_call && (Cfg.Flags & Cfg.fVerifMode)) {
							new_entry.LastValTime.Z();
						}
						else if(State & stDataTestMode) {
							LDATETIME test_data_start_moment;
							test_data_start_moment.Set(plusdate(getcurdate_(), -60), ZEROTIME);
							last_utm.Get(test_data_start_moment);
						}
						else
							last_utm.Get(new_entry.LastValTime);
					}
					else
						new_entry.LastValTime.Z();
					if(new_entry.Flags & new_entry.fTestPurpose) {
						new_entry.ExtraCount = 10000;
					}
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
				const uint tsc = p_fblk->T_.GetCount();
				if(tsc && p_fblk->T_.GetTime(tsc-1, &ut) && ut.Get(last_dtm) && cmp(last_dtm, r_tk.Dtm) < 0 && p_fblk->T_.GetValueVecIndex("close", &vec_idx)) {
					// @v10.6.8 {
					if(r_tk.Last == 0 && r_tk.Ask > 0 && r_tk.Bid > 0) {
						r_tk.Last = (r_tk.Ask + r_tk.Bid) / 2.0;
					}
					// } @v10.6.8
					if(r_tk.Last > 0.0) { // @v10.6.8
						if(!UseRegularValuesOnlyForStakes) {
							p_fblk->Make_T_Regular();
							assert(oneof2(p_fblk->TLE.Tag, 0, 2));
							p_fblk->TLE.Sse.Tm.Set(r_tk.Dtm);
							p_fblk->TLE.Sse.Values.clear();
							p_fblk->TLE.Sse.Values.Add(vec_idx, r_tk.Last);
							p_fblk->TLE.Tag = 2;
							p_fblk->Make_T_Actual();
							EvaluateTrends(p_fblk, 0, /*1*/0/*onActualTicks*/);
						}
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

int SLAPI TimeSeriesCache::SetTransactionNotification(const TsStakeEnvironment::TransactionNotification * pTan)
{
	int    ok = 1;
	SString log_msg;
	SString temp_buf;
	if(pTan) {
		for(uint i = 0; i < pTan->L.getCount(); i++) {
			const TsStakeEnvironment::TransactionNotification::Ta & r_ta = pTan->L.at(i);
			log_msg.Z().Cat("TaNotification").CatDiv(':', 2);
			log_msg.CatEq("Time", temp_buf.Z().Cat(r_ta.NotifyTime, DATF_ISO8601|DATF_CENTURY, 0)).Space();
			switch(r_ta.TaType) {
				case TsStakeEnvironment::ttratOrderAdd:      temp_buf = "ORDER-ADD"; break;
				case TsStakeEnvironment::ttratOrderUpdate:   temp_buf = "ORDER-UPD"; break;
				case TsStakeEnvironment::ttratOrderDelete:   temp_buf = "ORDER-DEL"; break;
				case TsStakeEnvironment::ttratHistoryAdd:    temp_buf = "HISTORY-ADD"; break;
				case TsStakeEnvironment::ttratHistoryUpdate: temp_buf = "HISTORY-UPD"; break;
				case TsStakeEnvironment::ttratHistoryDelete: temp_buf = "HISTORY-DEL"; break;
				case TsStakeEnvironment::ttratDealAdd:       temp_buf = "DEAL-ADD"; break;
				case TsStakeEnvironment::ttratDealUpdate:    temp_buf = "DEAL-UPD"; break;
				case TsStakeEnvironment::ttratDealDelete:    temp_buf = "DEAL-DEL"; break;
				case TsStakeEnvironment::ttratPosition:      temp_buf = "POSITION"; break;
				case TsStakeEnvironment::ttratRequest:       temp_buf = "REQUEST"; break;
				default: temp_buf.Z().CatChar('#').Cat(r_ta.TaType); break;
			}
			log_msg.Cat(temp_buf).Space();
			//log_msg.CatEq("TaType", r_ta.TaType).Space();
			pTan->GetS(r_ta.SymbP, temp_buf.Z());
			if(temp_buf.NotEmpty())
				log_msg.CatEq("Symb", temp_buf).Space();
			if(r_ta.Deal)
				log_msg.CatEq("Deal", r_ta.Deal).Space();
			if(r_ta.Order)
				log_msg.CatEq("Order", r_ta.Order).Space();
			log_msg.CatEq("OrdType", r_ta.OrdType).Space();
			log_msg.CatEq("OrdState", r_ta.OrdState).Space();
			log_msg.CatEq("DealType", r_ta.DealType).Space();
			log_msg.CatEq("OrdTypeTime", r_ta.OrdTypeTime).Space();
			if(r_ta.Expiration.d.year() < 1980)
				log_msg.CatEq("Expiration", temp_buf.Z().Cat(r_ta.Expiration, DATF_ISO8601|DATF_CENTURY, 0)).Space();
			if(r_ta.Price > 0.0)
				log_msg.CatEq("Price", r_ta.Price, MKSFMTD(0, 5, 0)).Space();
			if(r_ta.PriceTrigger > 0.0)
				log_msg.CatEq("PriceTrigger", r_ta.PriceTrigger, MKSFMTD(0, 5, 0)).Space();
			if(r_ta.PriceSL > 0.0)
			log_msg.CatEq("PriceSL", r_ta.PriceSL, MKSFMTD(0, 5, 0)).Space();
			if(r_ta.PriceTP > 0.0)
				log_msg.CatEq("PriceTP", r_ta.PriceTP, MKSFMTD(0, 5, 0)).Space();
			if(r_ta.Volume > 0.0)
				log_msg.CatEq("Volume", r_ta.Volume, MKSFMTD(0, 3, 0)).Space();
			if(r_ta.Position)
				log_msg.CatEq("Position", r_ta.Position).Space();
			if(r_ta.PositionBy)
				log_msg.CatEq("PositionBy", r_ta.PositionBy);
			PPLogMessage(PPFILNAM_TSTA_LOG, log_msg, LOGMSGF_TIME|LOGMSGF_DBINFO);
		}
	}
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
		// @v10.5.6 {
		if(p_blk->PPTS.Rec.Type == PPTimeSeries::tForex) {
			if((p_blk->VolumeMax == 100.0 || p_blk->VolumeMax == 100000.0) && p_blk->VolumeMin == 0.01) {
				p_blk->Flags |= TimeSeriesBlock::fVolumeMult1000;
				p_blk->VolumeMax *= 1000.0;
				p_blk->VolumeMin *= 1000.0;
				p_blk->VolumeStep *= 1000.0;
			}
		}
		// } @v10.5.6 
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
			if(p_blk && p_blk->PPTS.Rec.ID) {
				if(p_blk->Flags & TimeSeriesBlock::fDirty) {
					PPID   id = p_blk->PPTS.Rec.ID;
					PPTimeSeriesPacket ts_pack;
					PPTransaction tra(1);
					THROW(tra);
					if(ts_obj.GetPacket(id, &ts_pack) > 0) {
						if(p_blk->SpreadCount && p_blk->SpreadSum) {
							ts_pack.Rec.AvgSpread = p_blk->GetAverageSpread();
						}
						THROW(ts_obj.PutPacket(&id, &ts_pack, 0));
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
						p_blk->PPTS = ts_pack;
						p_blk->Flags &= ~TimeSeriesBlock::fDirty;
					}
					THROW(tra.Commit());
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

int FASTCALL TimeSeriesCache::IsStrategySuited(const PPObjTimeSeries::Strategy & rS) const
{
	int    yes = 0;
	if(rS.V.GetResultPerDay() >= Cfg.MinPerDayPotential) {
		if(oneof3(rS.StakeMode, 1, 2, 3))
			yes = 1;
	}
	return yes;
}

struct StakeCommentBlock {
	//temp_buf.Z().Cat("PPAT").Comma().Cat(r_s.MaxDuckQuant).Comma().Cat(r_s.TargetQuant).Comma().Cat(r_s.SpikeQuant, MKSFMTD(0, 9, 0));
	StakeCommentBlock() : P_Signature("PPAT"), SpikeQuant(0.0), MaxDuckQuant(0), TargetQuant(0), Flags(0)
	{
	}
	void  MakeText(const PPObjTimeSeries::Strategy & rS, double outerSpikeQuant, SString & rBuf)
	{
		SpikeQuant = (outerSpikeQuant > 0.0) ? outerSpikeQuant : rS.SpikeQuant_s;
		MaxDuckQuant = rS.MaxDuckQuant;
		TargetQuant = rS.TargetQuant;
		Flags = 0;
		if(rS.BaseFlags & rS.bfShort) {
			if(rS.StakeMode == 1) {
				if(rS.OptDeltaRange.GreaterThan(0.0))
					Flags |= fReverse;
			}
		}
		else {
			if(rS.StakeMode == 1) {
				if(rS.OptDeltaRange.LessThan(0.0))
					Flags |= fReverse;
			}
		}
		rBuf.Z().Cat(P_Signature);
		if(Flags & fReverse)
			rBuf.Cat("-R");
		rBuf.Comma().Cat(MaxDuckQuant).Comma().Cat(TargetQuant).Comma().Cat(SpikeQuant, MKSFMTD(0, 9, 0));
	}
	int   ParseText(SString & rBuf)
	{
		int    ok = 0;
		Flags = 0;
		MaxDuckQuant = 0;
		TargetQuant = 0;
		SpikeQuant = 0.0;
		if(rBuf.NotEmptyS()) {
			//temp_buf.Z().Cat("PPAT").Comma().Cat(r_s.MaxDuckQuant).Comma().Cat(r_s.TargetQuant).Comma().Cat(r_s.SpikeQuant, MKSFMTD(0, 9, 0));
			int    signature = 0;
			if(rBuf.CmpPrefix(P_Signature, 0) == 0) {
				const size_t sl = sstrlen(P_Signature);
				if(rBuf.C(sl) == '-' && rBuf.C(sl+1) == 'R' && rBuf.C(sl+2) == ',')
					signature = 2;
				else if(rBuf.C(sl) == ',')
					signature = 1;
			}
			if(signature) {
				SString temp_buf;
				StringSet ss(',', rBuf);
				for(uint ssp = 0, tn = 0; ss.get(&ssp, temp_buf); tn++) {
					if(tn == 0) {
					}
					else if(tn == 1)
						MaxDuckQuant = static_cast<uint>(temp_buf.ToLong());
					else if(tn == 2)
						TargetQuant = static_cast<uint>(temp_buf.ToLong());
					else if(tn == 3)
						SpikeQuant = temp_buf.ToReal();
				}
				if(signature == 2)
					Flags |= fReverse;
				ok = 1;
			}
		}
		return ok;
	}

	enum {
		fReverse = 0x0001 // Реверсивная ставка. То есть, направлена в обратную сторону от тренда
	};
	const char * P_Signature;
	double SpikeQuant;
	uint   MaxDuckQuant;
	uint   TargetQuant;
	long   Flags;
};

int SLAPI TimeSeriesCache::FindOptimalStrategyAtStake(const TimeSeriesBlock & rBlk, const TsStakeEnvironment::Stake & rStk, 
	PotentialStakeEntry * pPse, TsStakeEnvironment::StakeRequestBlock & rResult) const
{
	int    ok = -1;
	const TsStakeEnvironment::Tick * p_tk = StkEnv.SearchTickBySymb(rBlk.T_.GetSymb());
	if(p_tk && rBlk.T_.GetCount() && oneof2(rStk.Type, TsStakeEnvironment::ordtBuy, TsStakeEnvironment::ordtSell)) {
		uint   vec_idx = 0;
		const  int  gvvir = rBlk.T_.GetValueVecIndex("close", &vec_idx);
		const bool is_short = (rStk.Type == TsStakeEnvironment::ordtSell);
		const int  prec = rBlk.PPTS.Rec.Prec;
		const long trange_fmt = MKSFMTD(0, 10, NMBF_NOTRAILZ);
		if(gvvir) {
			SString log_msg;
			SString stk_symb;
			StkEnv.GetS(rStk.SymbP, stk_symb);
			SString comment_buf;
			SString temp_buf;
			int    do_stop = 0; // Сигнал на закрытие данной позиции
			StkEnv.GetS(rStk.CommentP, comment_buf);
			StakeCommentBlock scb;
			scb.ParseText(comment_buf);
			double last_value = 0.0;
			const  double avg_spread = rBlk.GetAverageSpread_WithAdjustment();
			rBlk.T_.GetValue(rBlk.T_.GetCount()-1, vec_idx, &last_value);
			if(pPse && scb.TargetQuant > 0.0 && scb.SpikeQuant > 0.0) {
				// Если переданная функции наилучшая текущая стратегия является обратной к данной, то посмотрим не следует ли
				// нам закрыть данную позицию дабы снизить потенциальные потери.
				const PPObjTimeSeries::Strategy & r_s = pPse->R_Blk.Strategies.at(pPse->StrategyPos);
				if(stk_symb.IsEqiAscii(pPse->R_Blk.T_.GetSymb()) && r_s.StakeMode == 1) {
					//
					// Считаем, что закрыть данную позицию есть смысл только в том случае, если
					// текущая оптимальная стратегия направлена в строну тренда.
					//
					int    may_be_considered = 0;
					if(r_s.BaseFlags & r_s.bfShort && !is_short && r_s.OptDeltaRange.LessThan(0.0))
						may_be_considered = 1;
					else if(!(r_s.BaseFlags & r_s.bfShort) && is_short && r_s.OptDeltaRange.GreaterThan(0.0))
						may_be_considered = 1;
					if(may_be_considered) {
						const double local_current_price = rStk.PriceCurrent;
						const double local_open_price = rStk.PriceOpen;
						const uint target_quant_for_stop = (Cfg.E.MinLossQuantForReverse > 0) ? static_cast<uint>(Cfg.E.MinLossQuantForReverse) : scb.TargetQuant;
						//double vsl = PPObjTimeSeries::Strategy::CalcSL_withExternalFactors(local_open_price, is_short, prec, target_quant_for_stop, scb.SpikeQuant, avg_spread);
						double vsl = ((rStk.SL + rStk.PriceOpen) / 2.0);
						if(scb.Flags & scb.fReverse) {
							do_stop = 1;
						}
						else if(rStk.Profit < 0.0) {
							if(is_short) {
								if(local_current_price >= vsl)
									do_stop = 1;
							}
							else {
								if(local_current_price <= vsl)
									do_stop = 1;
							}
						}
						{
							log_msg.Z().Cat("StakeDelete-candidate").CatChar('(').Cat(do_stop).CatChar(')');
							log_msg.CatDiv(':', 2).
								Cat(stk_symb).CatChar('-').
								Cat(is_short ? "S" : "B").CatChar('/').
								Cat(rStk.VolumeCurrent, MKSFMTD(0, 3, NMBF_NOTRAILZ)).CatChar('/').
								Cat(last_value, MKSFMTD(0, 5, NMBF_NOTRAILZ)).CatChar('/').
								Cat(rStk.SL, MKSFMTD(0, 7, NMBF_NOTRAILZ)).CatChar(':').Cat(rStk.TP, MKSFMTD(0, 7, NMBF_NOTRAILZ));
								log_msg.CatChar(':').Cat(rStk.TP, MKSFMTD(0, 7, NMBF_NOTRAILZ)).Space();
								log_msg.CatEq("Profit", rStk.Profit, MKSFMTD(0, 2, 0)).Space().
									CatEq("local_current_price", local_current_price, MKSFMTD(0, 5, 0)).Space().
									CatEq("vsl", vsl, MKSFMTD(0, 5, 0));
							log_msg.Space().Cat(comment_buf);
							PPLogMessage(PPFILNAM_TSSTAKE_LOG, log_msg, LOGMSGF_TIME|LOGMSGF_DBINFO);
						}
					}
				}
			}
			if(do_stop) {
				log_msg.Z().Cat("StakeDelete");
				if(!(Cfg.Flags & Cfg.fAllowReverse))
					log_msg.Space().CatParStr("disabled");
				log_msg.CatDiv(':', 2).
					Cat(stk_symb).CatChar('-').
					Cat(is_short ? "S" : "B").CatChar('/').
					Cat(rStk.VolumeCurrent, MKSFMTD(0, 3, NMBF_NOTRAILZ)).CatChar('/').
					Cat(last_value, MKSFMTD(0, 5, NMBF_NOTRAILZ)).CatChar('/').
					Cat(rStk.SL, MKSFMTD(0, 7, NMBF_NOTRAILZ)).CatChar(':').Cat(rStk.TP, MKSFMTD(0, 7, NMBF_NOTRAILZ));
					log_msg.CatChar(':').Cat(rStk.TP, MKSFMTD(0, 7, NMBF_NOTRAILZ));
					log_msg.CatEq("Profit", rStk.Profit);
				/*if(p_st && (!external_sl_used || !external_tp_used)) {
					log_msg.Space().Cat(PPObjTimeSeries::StrategyToString(*p_st, &_best_result, temp_buf));
				}*/
				log_msg.Space().Cat(comment_buf);
				PPLogMessage(PPFILNAM_TSSTAKE_LOG, log_msg, LOGMSGF_TIME|LOGMSGF_DBINFO);
				if(Cfg.Flags & Cfg.fAllowReverse) {
					TsStakeEnvironment::StakeRequestBlock::Req req;
					req.Ticket = rStk.Ticket;
					req.Action = TsStakeEnvironment::traDeal;
					req.Type = is_short ? TsStakeEnvironment::ordtBuy : TsStakeEnvironment::ordtSell;
					req.TsID = rBlk.PPTS.Rec.ID;
					rResult.AddS(stk_symb, &req.SymbolP);
					if(rBlk.Flags & TimeSeriesBlock::fVolumeMult1000) // @v10.5.6
						req.Volume = rStk.VolumeInit / 100.0; // @v10.5.7 1000.0-->100.0
					else
						req.Volume = rStk.VolumeInit;
					req.SL = 0;
					req.TP = 0;
					rResult.AddS(stk_symb, &req.CommentP);
					THROW_SL(rResult.L.insert(&req));
					ok = 100;
				}
				else
					do_stop = 0;
			}
			if(!do_stop) {
				const uint extra_maxduck_quant = 12;
				// @v10.3.11 {
				/* плохо работает
				int    use_extremal_tp = 0;
				if(rStk.Profit > 0.0) {
					//
					// Экспериментальная модификация SL в случае выигрыша до extra_maxduck_quant квантов
					//
					const double spike_quant = (external_spike_quant > 0.0) ? external_spike_quant : rBlk.PPTS.SpikeQuant;
					if(spike_quant > 0.0) {
						double current_diff = is_short ? (rStk.PriceOpen - last_value) : (last_value - rStk.PriceOpen);
						if(current_diff > 0.0) {
							//assert(current_diff > 0.0);
							int current_diff_quant = ffloori(current_diff / spike_quant)-1; // -1 страховка дабы не ставить экстремальный STOP ниже начальной точки (с учетом комиссии)
							if(current_diff_quant >= extra_maxduck_quant) {
								external_max_duck_quant = extra_maxduck_quant;
								use_extremal_tp = 1;
							}
						}
					}
				}
				*/
				// } @v10.3.11
				uint   max_duck_quant_for_sl = scb.MaxDuckQuant;
				uint   sl_adj = 0;
				int    do_use_accelerated_sl = 0; // @v10.4.5 // @v10.4.6 0-->1 // @20190526 1-->0
				// @20190424 {
				if(do_use_accelerated_sl && rStk.Profit > 0.0) {
					if(is_short) {
						sl_adj = ffloori((rStk.PriceOpen - rStk.PriceCurrent) / (rStk.PriceCurrent * scb.SpikeQuant));
					}
					else {
						sl_adj = ffloori((rStk.PriceCurrent - rStk.PriceOpen) / (rStk.PriceCurrent * scb.SpikeQuant));
					}
					if((sl_adj + extra_maxduck_quant) < max_duck_quant_for_sl)
						max_duck_quant_for_sl -= sl_adj;
					else
						max_duck_quant_for_sl = extra_maxduck_quant;
				}
				// } @20190424 
				const int    allow_stake_upd = 0;
				const double external_sl = (max_duck_quant_for_sl > 0 && scb.SpikeQuant > 0.0) ?
					PPObjTimeSeries::Strategy::CalcSL_withExternalFactors(last_value, is_short, prec, max_duck_quant_for_sl, scb.SpikeQuant, avg_spread) : 0.0;
				const double external_tp = (scb.TargetQuant > 0 && scb.SpikeQuant > 0.0) ?
					PPObjTimeSeries::Strategy::CalcTP_withExternalFactors(rStk.PriceOpen, is_short, prec, scb.TargetQuant, scb.SpikeQuant, avg_spread) : 0.0;
				//const PPObjTimeSeries::Strategy * p_st = (_best_result.MaxResultIdx >= 0) ? &rBlk.Strategies.at(_best_result.MaxResultIdx) : 0;
				//const double __sl = p_st ? p_st->CalcSL(last_value, avg_spread) : 0.0;
				//const double __tp = p_st ? p_st->CalcTP(last_value, avg_spread) : 0.0;
				//const double __sl = 0.0;
				//const double __tp = 0.0;
				//const double eff_sl = (__sl > 0.0 && external_sl > 0.0) ? (is_short ? MIN(__sl, external_sl) : MAX(__sl, external_sl)) : ((__sl > 0.0) ? __sl : external_sl);
				//const double eff_sl = (external_sl > 0.0) ? external_sl : ((__sl > 0.0) ? __sl : rStk.SL);
				//const double eff_tp = (__tp > 0.0 && external_tp > 0.0) ? (is_short ? MAX(__tp, external_tp) : MIN(__tp, external_tp)) : ((__tp > 0.0) ? __tp : external_tp);
				//const double eff_tp = (rStk.TP > 0.0) ? rStk.TP : ((__tp > 0.0) ? __tp : 0.0); // Установленный TP менять не будем
				const double eff_sl = (external_sl > 0.0) ? external_sl : rStk.SL;
				const double eff_tp = (rStk.TP > 0.0) ? rStk.TP : 0.0; // Установленный TP менять не будем
				if(eff_sl > 0.0 || eff_tp > 0.0) {
					const bool external_sl_used = LOGIC(eff_sl == external_sl);
					const bool external_tp_used = LOGIC(eff_tp == external_tp);
					const bool do_update_sl = (rStk.SL <= 0.0 && eff_sl > 0.0) ? true : (is_short ? (eff_sl < rStk.SL) : (eff_sl > rStk.SL));
					const bool do_update_tp = (rStk.TP <= 0.0 && eff_tp > 0.0) ? true : (is_short ? (eff_tp > rStk.TP) : (eff_tp < rStk.TP));
					if(do_update_sl || do_update_tp) {
						if(allow_stake_upd) { // @20190603
							log_msg.Z().Cat("StakeUpd");
							if(!allow_stake_upd)
								log_msg.Space().CatParStr("disabled");
							log_msg.CatDiv(':', 2).
								Cat(stk_symb).CatChar('-').
								Cat(is_short ? "S" : "B").CatChar('/').
								Cat(rStk.VolumeCurrent, MKSFMTD(0, 3, NMBF_NOTRAILZ)).CatChar('/').
								Cat(last_value, MKSFMTD(0, 5, NMBF_NOTRAILZ)).CatChar('/').
								Cat(rStk.SL, MKSFMTD(0, 7, NMBF_NOTRAILZ)).CatChar(':').Cat(rStk.TP, MKSFMTD(0, 7, NMBF_NOTRAILZ)).
								Cat(" >> ").
								Cat(eff_sl, MKSFMTD(0, 7, NMBF_NOTRAILZ));
								if(external_sl_used)
									log_msg.Cat("[*]");
								log_msg.CatChar(':');
								log_msg.Cat(eff_tp, MKSFMTD(0, 7, NMBF_NOTRAILZ));
								if(external_tp_used)
									log_msg.Cat("[*]");
							/*if(p_st && (!external_sl_used || !external_tp_used)) {
								log_msg.Space().Cat(PPObjTimeSeries::StrategyToString(*p_st, &_best_result, temp_buf));
							}*/
							if(sl_adj > 0) {
								log_msg.Space().CatEq("sl_adj", sl_adj);
							}
							log_msg.Space().Cat(comment_buf);
							PPLogMessage(PPFILNAM_TSSTAKE_LOG, log_msg, LOGMSGF_TIME|LOGMSGF_DBINFO);
						}
						if(allow_stake_upd) {
							TsStakeEnvironment::StakeRequestBlock::Req req;
							req.Ticket = rStk.Ticket;
							req.Action = TsStakeEnvironment::traSLTP;
							req.Type = rStk.Type;
							req.TsID = rBlk.PPTS.Rec.ID;
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
	}
	CATCHZOK
	return ok;
}

int SLAPI TimeSeriesCache::FindOptimalStrategyForStake(const double evaluatedUsedMargin, PotentialStakeEntry & rPse) const
{
	int    ok = -1;
	const TimeSeriesBlock & r_blk = rPse.R_Blk;
	const TsStakeEnvironment::Tick * p_tk = StkEnv.SearchTickBySymb(r_blk.T_.GetSymb());
	if(p_tk && !(r_blk.Flags & r_blk.fDisableStake) && (r_blk.Flags & (r_blk.fLong|r_blk.fShort)) && r_blk.T_.GetCount()) {
		SString log_msg;
		SString temp_buf;
		const uint current_stake_count = StkEnv.SL.getCount();
		rPse.MarginReq = p_tk->MarginReq;
		const uint   cfg_max_stake = (Cfg.MaxStakeCount > 0) ? Cfg.MaxStakeCount : 10;
		const uint   max_stake = (cfg_max_stake > current_stake_count) ? (cfg_max_stake - current_stake_count) : 0;
		const double max_balance_part = (Cfg.AvailableLimitPart > 0.0 && Cfg.AvailableLimitPart <= 1.0) ? Cfg.AvailableLimitPart : 0.5;
		const double max_balance_abs = (Cfg.AvailableLimitAbs > 0.0) ? Cfg.AvailableLimitAbs : 0.0;
		double insurance_balance = 0.0;
		if(max_balance_abs > 0.0)
			insurance_balance = StkEnv.Acc.Balance - max_balance_abs;
		else if(max_balance_part > 0.0 && max_balance_part <= 1.0)
			insurance_balance = StkEnv.Acc.Balance * (1.0 - max_balance_part);
		// @v10.3.5 const double margin_available = (StkEnv.Acc.Margin + StkEnv.Acc.MarginFree - insurance_balance) / max_stake;
		const double margin_available = max_stake ? ((StkEnv.Acc.Balance - evaluatedUsedMargin - insurance_balance) / cfg_max_stake) : 0.0; // @v10.3.5 // @v10.8.8 max_stake-->cfg_max_stake
		const bool there_is_enough_margin = (margin_available > 0.0 && margin_available < StkEnv.Acc.MarginFree);
		/*
		{
			log_msg.Z().Cat("Balance Evaluation").CatDiv(':', 2).Cat(r_blk.PPTS.Symb).Space().
				CatEq("margin-req", p_tk->MarginReq, MKSFMTD(0, 5, 0)).Space().
				CatEq("max-stake", max_stake).Space().
				CatEq("insurance-balance", insurance_balance, MKSFMTD(0, 2, 0)).Space().
				CatEq("margin-available", margin_available, MKSFMTD(0, 2, 0)).Space().
				CatEq("there-is-enough-margin", there_is_enough_margin ? ":)TRUE" : ":(FALSE");
			PPLogMessage(PPFILNAM_TSSTAKEPOTENTIAL_LOG, log_msg, LOGMSGF_TIME|LOGMSGF_DBINFO);
		}
		*/
		/* @v10.4.2 (если !there_is_enough_margin, то в блоке rPse будет установлен флаг rLackOfMargin) if(there_is_enough_margin)*/ 
		{
			PPObjTimeSeries::StrategyContainer::SelectBlock scsb(r_blk.TrendList, r_blk.StratIndex);
			scsb.LastTrendIdx = -1;
			// @v10.7.10 scsb.ChaosFactor = 0;
			scsb.DevPtCount = Cfg.E.LocalDevPtCount; // @v10.7.1
			scsb.LDMT_Factor = Cfg.E.LDMT_Factor; // @v10.7.1
			scsb.MainTrendMaxErrRel = Cfg.E.MainTrendMaxErrRel; // @v10.7.2
			scsb.P_Ts = &r_blk.T_; // @v10.7.1
			// @v10.7.3 scsb.Criterion = PPObjTimeSeries::StrategyContainer::selcritWinRatio | PPObjTimeSeries::StrategyContainer::selcritfSkipAmbiguous;
			scsb.Criterion = PPObjTimeSeries::StrategyContainer::selcritResult | PPObjTimeSeries::StrategyContainer::selcritfSkipAmbiguous; // @v10.7.3
			if(!(r_blk.Flags & r_blk.fShort))
				scsb.Criterion |= PPObjTimeSeries::StrategyContainer::selcritfSkipShort;
			if(!(r_blk.Flags & r_blk.fLong))
				scsb.Criterion |= PPObjTimeSeries::StrategyContainer::selcritfSkipLong;
			if(r_blk.Strategies.SelectS2(scsb) > 0) {
				/*LongArray matched_strategies_idx_list;
				long sel_criterion = PPObjTimeSeries::StrategyContainer::selcritWinRatio | PPObjTimeSeries::StrategyContainer::selcritfSkipAmbiguous;
				// @20190708 long sel_criterion = PPObjTimeSeries::StrategyContainer::selcritWinRatio | PPObjTimeSeries::StrategyContainer::selcritfWeightAmbiguous;
				if(!(r_blk.Flags & r_blk.fShort))
					sel_criterion |= PPObjTimeSeries::StrategyContainer::selcritfSkipShort;
				if(!(r_blk.Flags & r_blk.fLong))
					sel_criterion |= PPObjTimeSeries::StrategyContainer::selcritfSkipLong;
				PPObjTimeSeries::BestStrategyBlock _best_result;
				int    last_trend_idx = -1; // @v10.4.11 -1 --> -2 // @v10.5.6 (-2)-->(-1)
				long   chaos_factor = 0; // @v10.7.1
				if(r_blk.Strategies.SelectS(r_blk.TrendList, last_trend_idx, sel_criterion, chaos_factor, &r_blk.StratIndex, _best_result, &matched_strategies_idx_list) > 0) {*/
				// @20190515 assert(_best_result.MaxResult >= Cfg.MinPerDayPotential);
				assert(scsb.MaxResultIdx >= 0 && scsb.MaxResultIdx < static_cast<int>(r_blk.Strategies.getCount()));
				const PPObjTimeSeries::Strategy & r_s = r_blk.Strategies.at(scsb.MaxResultIdx);
				const bool is_sell = LOGIC(r_s.BaseFlags & r_s.bfShort);
				//
				// Блок для анализа возможной "шизофрении" алгоритма, в результате которой будет выбран лучший результат
				// при наличии реверсивного алгоритма, сходного по потенциальному результату.
				// Пока только вывод в журнал.
				//
				PPObjTimeSeries::StrategyContainer::SelectBlock scsb_reverse(r_blk.TrendList, r_blk.StratIndex);
				scsb_reverse.Criterion = scsb.Criterion & ~(PPObjTimeSeries::StrategyContainer::selcritfSkipAmbiguous|PPObjTimeSeries::StrategyContainer::selcritfWeightAmbiguous);
				if(is_sell) {
					scsb_reverse.Criterion |= PPObjTimeSeries::StrategyContainer::selcritfSkipShort;
					scsb_reverse.Criterion &= ~PPObjTimeSeries::StrategyContainer::selcritfSkipLong;
				}
				else {
					scsb_reverse.Criterion |= PPObjTimeSeries::StrategyContainer::selcritfSkipLong;
					scsb_reverse.Criterion &= ~PPObjTimeSeries::StrategyContainer::selcritfSkipShort;
				}
				// @v10.7.10 scsb_reverse.ChaosFactor = scsb.ChaosFactor;
				scsb_reverse.DevPtCount = Cfg.E.LocalDevPtCount; // @v10.7.1
				scsb_reverse.LDMT_Factor = Cfg.E.LDMT_Factor; // @v10.7.1
				scsb_reverse.MainTrendMaxErrRel = Cfg.E.MainTrendMaxErrRel; // @v10.7.2
				scsb_reverse.P_Ts = &r_blk.T_; // @v10.7.1
				scsb_reverse.LastTrendIdx = scsb.LastTrendIdx;
				r_blk.Strategies.SelectS2(scsb_reverse);

				/*PPObjTimeSeries::BestStrategyBlock _best_result_reverse; // Лучший результат для реверсивной ставки
				long sel_criterion_reverse = scsb.Criterion & ~(PPObjTimeSeries::StrategyContainer::selcritfSkipAmbiguous|PPObjTimeSeries::StrategyContainer::selcritfWeightAmbiguous);
				if(is_sell) {
					sel_criterion_reverse |= PPObjTimeSeries::StrategyContainer::selcritfSkipShort;
					sel_criterion_reverse &= ~PPObjTimeSeries::StrategyContainer::selcritfSkipLong;
				}
				else {
					sel_criterion_reverse |= PPObjTimeSeries::StrategyContainer::selcritfSkipLong;
					sel_criterion_reverse &= ~PPObjTimeSeries::StrategyContainer::selcritfSkipShort;
				}
				r_blk.Strategies.SelectS(r_blk.TrendList, last_trend_idx, sel_criterion_reverse, chaos_factor, &r_blk.StratIndex, _best_result_reverse, 0);*/
				//
				rPse.StrategyPos = static_cast<uint>(scsb.MaxResultIdx);
				// @20190515 rPse.ResultPerDay = _best_result.MaxResult;
				rPse.ResultPerDay = r_s.V.GetResultPerDay(); // @20190515
				const double min_cost = EvaluateCost(r_blk, is_sell, r_blk.VolumeMin, ecfTrickChf); // @v10.4.10 ecfTrickChf
				double __volume = 0.0;
				double __adjusted_result_per_day = 0.0;
				double __arrange_crit_value = 0.0;
				if(min_cost > 0.0) {
					const double margin = r_blk.PPTS.GetMargin(is_sell);
					double sc = floor(margin_available / min_cost);
					__volume = R0(r_blk.VolumeStep * sc);
					SETMIN(__volume, r_blk.VolumeMax);
					if(__volume > 0.0) {
						__adjusted_result_per_day = EvaluateCost(r_blk, is_sell, __volume, ecfTrickChf) * rPse.ResultPerDay; // @v10.4.10 ecfTrickChf
						__arrange_crit_value = __adjusted_result_per_day;
						//Cfg.MinPerDayPotential
						int use_it = 1;
						// @v10.4.0 if(_best_result_reverse.MaxResultIdx >= 0) { use_it = 0; }
						//if(__adjusted_result_per_day >= 10.0) { use_it = 0; }
						if(use_it) {
							rPse.Volume = __volume;
							rPse.AdjustedResultPerDay = __adjusted_result_per_day;
							rPse.ArrangeCritValue = __arrange_crit_value; // !
							rPse.Tv  = scsb.TvForMaxResult;
							rPse.Tv2 = scsb.Tv2ForMaxResult;
							if(there_is_enough_margin)
								ok = 1;
							else {
								rPse.Flags |= rPse.fLackOfMargin;
								ok = 2;
							}
						}
					}
				}
				//
				// CatCharN(' ', 8) для выравнивания с вероятным выводом reverse-результата
				SString msl_buf;
				msl_buf.CatChar('{');
				for(uint mslidx = 0; mslidx < scsb.AllSuitedPosList.getCount(); mslidx++) {
					const uint si = static_cast<uint>(scsb.AllSuitedPosList.get(mslidx));
					if(si < r_blk.Strategies.getCount()) {
						const PPObjTimeSeries::Strategy & r_ms = r_blk.Strategies.at(si);
						msl_buf.CatChar((r_ms.BaseFlags & r_ms.bfShort) ? '-' : '+');
					}
				}
				msl_buf.CatChar('}');
				log_msg.Z().Cat("Best").CatCharN(' ', 8).Cat(msl_buf).CatDiv(':', 2).Cat(r_blk.PPTS.GetSymb()).Space().
					CatEq("min-cost", min_cost, MKSFMTD(0, 5, 0)).Space().
					CatEq("volume", __volume, MKSFMTD(0, 0, 0)).Space().
					// @v10.7.1 CatEq("adjusted-result-per-day", __adjusted_result_per_day, MKSFMTD(0, 5, NMBF_NOTRAILZ)).Space().
					// @v10.7.1 CatEq("arrange-crit-value", __arrange_crit_value, MKSFMTD(0, 5, NMBF_NOTRAILZ)).Space().
					// @v10.7.1 {
					CatChar('[').
						Cat(scsb.TrendErrRel, MKSFMTD(0, 4, 0)).Space().
						Cat(scsb.MainTrendErrRel, MKSFMTD(0, 4, 0)).// @v10.8.9 Space().
						// @v10.8.9 Cat(scsb.LocalDeviation, MKSFMTD(0, 4, 0)).Space().
						// @v10.8.9 Cat(scsb.LocalDeviation2, MKSFMTD(0, 4, 0)).
					CatChar(']').Space().
					// } @v10.7.1 
					Cat(PPObjTimeSeries::StrategyToString(r_s, &r_blk.Strategies, &scsb.TvForMaxResult, &scsb.Tv2ForMaxResult, temp_buf));
				PPLogMessage(PPFILNAM_TSSTAKEPOTENTIAL_LOG, log_msg, LOGMSGF_TIME|LOGMSGF_DBINFO);
				if(scsb_reverse.MaxResultIdx >= 0) {
					const PPObjTimeSeries::Strategy & r_s_reverse = r_blk.Strategies.at(scsb_reverse.MaxResultIdx);
					log_msg.Z().Cat("Best-Reverse").CatDiv(':', 2).Cat(r_blk.PPTS.GetSymb()).Space().Cat(
						PPObjTimeSeries::StrategyToString(r_s_reverse, &r_blk.Strategies, &scsb_reverse.TvForMaxResult, &scsb.Tv2ForMaxResult, temp_buf));
					PPLogMessage(PPFILNAM_TSSTAKEPOTENTIAL_LOG, log_msg, LOGMSGF_TIME|LOGMSGF_DBINFO);
				}
			}
		}
	}
	return ok;
}

double SLAPI TimeSeriesCache::EvaluateCost(const TimeSeriesBlock & rBlk, bool sell, double volume, uint flags) const
{
	const char * p_base_symb = "USD";
	double cost = 0.0;
	double last_value = 0.0;
	if(!isempty(rBlk.PPTS.Rec.CurrencySymb) && rBlk.GetLastValue(&last_value)) {
		const SString org_symb(rBlk.T_.GetSymb());
		SString symb;
		size_t dot_pos = 0;
		if(org_symb.SearchChar('.', &dot_pos) && dot_pos > 0)
			org_symb.Sub(0, dot_pos, symb);
		else
			symb = org_symb;
		double temp_margin = rBlk.PPTS.GetMargin(sell);
		// @v10.4.10 {
		if(flags & ecfTrickChf && symb.Len() == 6 && (symb.HasPrefixIAscii("CHF") || symb.CmpSuffix("CHF", 1) == 0)) {
			if(temp_margin > 0.005)
				temp_margin = 0.005;
		}
		// } @v10.4.10 
		const double margin = temp_margin;
		if(!sstreqi_ascii(rBlk.PPTS.Rec.CurrencySymb, p_base_symb)) {
			if(symb.CmpPrefix(p_base_symb, 1) == 0) {
				//cost = volume * margin / last_value;
				cost = volume * margin;
			}
			else {
				/*
				int    reverse_rate = 0;
				double last_cvt_value = 0.0;
				const TimeSeriesBlock * p_cvt_blk = SearchRateConvertionBlock(rBlk.PPTS.CurrencySymb, &reverse_rate);
				if(p_cvt_blk && p_cvt_blk->GetLastValue(&last_cvt_value) && last_cvt_value > 0.0) {
					if(reverse_rate) {
						cost = volume * margin * last_value / last_cvt_value;
					}
					else {
						cost = volume * margin * last_value * last_cvt_value;
					}
				}
				*/
				cost = volume * margin;
			}
		}
		else
			cost = volume * margin * last_value;
	}
	return cost;
}

double SLAPI TimeSeriesCache::EvaluateUsedMargin() const
{
	double evaluated_used_margin = 0.0;
	const uint current_stake_count = StkEnv.SL.getCount();
	SString stk_symb;
	for(uint si = 0; si < current_stake_count; si++) {
		const TsStakeEnvironment::Stake & r_stk = StkEnv.SL.at(si);
		StkEnv.GetS(r_stk.SymbP, stk_symb);
		const TimeSeriesBlock * p_blk = SearchBlockBySymb(stk_symb, 0);
		if(p_blk) {
			const bool is_short = (r_stk.Type == TsStakeEnvironment::ordtSell);
			double stk_cost = EvaluateCost(*p_blk, is_short, r_stk.VolumeInit, ecfTrickChf); // @v10.4.10 ecfTrickChf
			evaluated_used_margin += stk_cost;
		}
	}
	return evaluated_used_margin;
}

int SLAPI TimeSeriesCache::EvaluateStakes(TsStakeEnvironment::StakeRequestBlock & rResult) const
{
	int    ok = -1;
	PPUserFuncProfiler ufp(PPUPRF_TSEVALSTAKES); // @v10.3.3
	const  LDATETIME now = getcurdatetime_();
	SString temp_buf;
	SString log_msg;
	SString log_file_name;
	SString trend_err_buf;
	SString tk_symb;
	SString stk_symb;
	SString stk_memo;
	LongArray seen_ts_list; // @v10.7.6 Список серий, которые уже были обработаны. Введен из-за того, что (вероятно) иногда в обработку попадает двойной набор.
	TSVector <PotentialStakeEntry> potential_stake_list;
	const uint current_stake_count = StkEnv.SL.getCount();
	const double evaluated_used_margin = EvaluateUsedMargin();
	PPGetFilePath(PPPATH_LOG, (temp_buf = "EvaluateStakes").Dot().Cat("log"), log_file_name);
	for(uint i = 0; i < StkEnv.TL.getCount(); i++) {
		const TsStakeEnvironment::Tick & r_tk = StkEnv.TL.at(i);
		const long max_diff_sec = UseRegularValuesOnlyForStakes ? 130 : 40;
		// -10800
		LDATETIME tktime = r_tk.Dtm; // Время тика скорректированное на поправку из конфигурации
		tktime.addsec(Cfg.E.TerminalTimeAdjustment);
		const long cur_diff_sec = diffdatetimesec(now, tktime);
		uint  blk_idx = 0;
		StkEnv.GetS(r_tk.SymbP, tk_symb);
		//
		// @v10.7.6 Следующая переменная p_blk должны быть const, но, увы, из-за того что мы вынуждены
		// менять в ней время последней рассчитанной ставки (иначе может случиться двойное срабатывание ставки по причине задержки в сети)
		//
		/*const*/TimeSeriesBlock * p_blk = SearchBlockBySymb(tk_symb, &blk_idx);
		//
		if(p_blk && !seen_ts_list.lsearch(p_blk->PPTS.Rec.ID)) { // @v10.7.6 (&& !seen_ts_list.lsearch(p_blk->PPTS.Rec.ID))
			seen_ts_list.add(p_blk->PPTS.Rec.ID); // @v10.7.6
			// @v10.6.8 {
			if(Cfg.Flags & Cfg.fLogStakeEvaluation) {
				const double test_cost = EvaluateCost(*p_blk, 0, p_blk->VolumeMin, ecfTrickChf); // @v10.6.10
				double last_regular = 0.0; // @v10.6.10
				p_blk->GetLastValue(&last_regular); // @v10.6.10
				log_msg.Z().Cat(tk_symb).Space().CatEq("org-tick-time", r_tk.Dtm, DATF_ISO8601|DATF_CENTURY, 0).Space().
					CatEq("current-diff-sec", cur_diff_sec).Space().
					CatEq("last", r_tk.Last, MKSFMTD(0, 5, 0)).Space().
					CatEq("bid", r_tk.Bid, MKSFMTD(0, 5, 0)).Space().
					CatEq("ask", r_tk.Ask, MKSFMTD(0, 5, 0)).Space().
					CatEq("regular", last_regular, MKSFMTD(0, 5, 0)).Space().
					CatEq("test-cost", test_cost, MKSFMTD(0, 5, 0));
				PPLogMessage(log_file_name, log_msg, 0);
				log_msg.Z();
				trend_err_buf.Z();
				for(uint tridx = 0; tridx < p_blk->TrendList.getCount(); tridx++) {
					const PPObjTimeSeries::TrendEntry * p_tre = p_blk->TrendList.at(tridx);
					if(p_tre) {
						const uint tre_tlc = p_tre->TL.getCount();
						const uint tre_erc = p_tre->ErrL.getCount();
						if(trend_err_buf.NotEmpty())
							trend_err_buf.Space();
						trend_err_buf.CatChar('[');
						trend_err_buf.Cat(p_tre->Stride).CatDiv(':', 0);
						if(tre_tlc == 0) {
							trend_err_buf.Cat("zero-tl-count");
						}
						else {
							if(tre_tlc != tre_erc)
								trend_err_buf.Cat("tl-count!=te-count").Space();
							if(tre_erc == 0)
								trend_err_buf.Cat("zero-te-count").Space();
							else {
								const double last_err = p_tre->ErrL.at(tre_erc-1);
								//const double last_1_err = (tre_erc > 1) ? p_tre->ErrL.at(tre_erc-2) : 0.0;
								const double avg_err = p_tre->ErrAvg;
								trend_err_buf.Cat(fdivnz(last_err, avg_err), MKSFMTD(0, 3, 0));
							}
						}
						trend_err_buf.CatChar(']');
					}
				}
				if(trend_err_buf.NotEmpty()) {
					PPLogMessage(log_file_name, trend_err_buf, 0);
				}
			}
			// } @v10.6.8 
			if(abs(cur_diff_sec) <= max_diff_sec) { // @v10.6.8 cur_diff_sec-->abs(cur_diff_sec)
				if(!p_blk->LastStakeTime || diffdatetimesec(now, p_blk->LastStakeTime) >= 20) { // @v10.7.6
					// @v10.4.10 {
					/* @v10.6.8 if(0) {
						log_msg.Z().Cat(tktime, DATF_ISO8601|DATF_CENTURY, 0).Space().Cat(r_tk.Last, MKSFMTD(0, 5, 0));
						for(uint tridx = 0; tridx < p_blk->TrendList.getCount(); tridx++) {
							const PPObjTimeSeries::TrendEntry * p_tre = p_blk->TrendList.at(tridx);
							if(p_tre) {
								const uint tre_tlc = p_tre->TL.getCount();
								const uint tre_erc = p_tre->ErrL.getCount();
								log_msg.CR().Tab();
								log_msg.Cat(p_tre->ErrAvg * 10E9, MKSFMTD(0, 5, 0));
								log_msg.Tab().Cat(p_tre->Stride);
								if(tre_tlc == 0) {
									log_msg.Tab().Cat("zero-tl-count");
								}
								else {
									if(tre_tlc != tre_erc)
										log_msg.Tab().Cat("tl-count != te-count");
									log_msg.Tab().Cat(p_tre->TL.at(tre_tlc-1) * 10E9, MKSFMTD(0, 5, 0));
									if(tre_erc == 0)
										log_msg.Tab().Cat("zero-te-count");
									else 
										log_msg.Tab().Cat(p_tre->ErrL.at(tre_erc-1) * 10E9, MKSFMTD(0, 5, 0));
								}
							}
						}
						(temp_buf = "tst").CatChar('-').Cat(tk_symb).Dot().Cat("log");
						PPGetFilePath(PPPATH_LOG, temp_buf, log_file_name);
						PPLogMessage(log_file_name, log_msg, 0);
					}*/
					// } @v10.4.10 
					int   is_there_stake = 0;
					PotentialStakeEntry pse(*p_blk);
					const int fosfsr = FindOptimalStrategyForStake(evaluated_used_margin, pse);
					for(uint si = 0; si < current_stake_count; si++) {
						const TsStakeEnvironment::Stake & r_stk = StkEnv.SL.at(si);
						StkEnv.GetS(r_stk.SymbP, stk_symb);
						if(stk_symb.IsEqiAscii(tk_symb)) {
							int fosasr = FindOptimalStrategyAtStake(*p_blk, r_stk, ((fosfsr > 0) ? &pse : 0), rResult);
							if(fosasr != 100) // @v10.4.6
								is_there_stake = 1;
						}
					}
					if(fosfsr > 0 && !is_there_stake && current_stake_count < Cfg.MaxStakeCount) {
						potential_stake_list.insert(&pse);
						p_blk->LastStakeTime = now; // @v10.7.6
					}
				}
			}
		}
	}
	{
		//
		// За одну итерацию формируем не более одной ставки. Несколько ставок сразу формировать
		// нельзя из-за того, что в этом случае возможно превышение лимитов по количеству ставок и
		// используемой сумме по причине задержки в срабатывании сервиса провайдера.
		//
		const uint pslc = potential_stake_list.getCount();
		if(pslc) {
			//
			// Здесь считать доступный для ставки баланс смысла нет - при расчете ставки в FindOptimalStrategyForStake() такой анализ уже был сделан
			//
			potential_stake_list.sort(PTR_CMPFUNC(double));
			int    ps_idx = -1;
			uint   ps_iter_idx = pslc;
			do {
				const PotentialStakeEntry & r_pse_iter = potential_stake_list.at(--ps_iter_idx);
				if(!(r_pse_iter.Flags & PotentialStakeEntry::fLackOfMargin)) {
					ps_idx = static_cast<int>(ps_iter_idx);
				}
			} while(ps_iter_idx && ps_idx < 0);
			if(ps_idx >= 0) {
				const PotentialStakeEntry & r_pse = potential_stake_list.at(ps_idx);
				double last_value = 0.0;
				if(r_pse.R_Blk.GetLastValue(&last_value)) {
					const PPObjTimeSeries::Strategy & r_s = r_pse.R_Blk.Strategies.at(r_pse.StrategyPos);
					stk_symb = r_pse.R_Blk.T_.GetSymb();
					const double cost = EvaluateCost(r_pse.R_Blk, LOGIC(r_s.BaseFlags & r_s.bfShort), r_pse.Volume, 0);
					if(cost > 0.0) {
						const PPObjTimeSeries::TrendEntry * p_te = PPObjTimeSeries::SearchTrendEntry(r_pse.R_Blk.TrendList, r_s.InputFrameSize);
						// @v10.4.11 const double spike_quant = p_te ? p_te->SpikeQuant : r_s.SpikeQuant;
						const double spike_quant = r_s.SpikeQuant_s; // @v10.4.11
						const double avg_spread_sl = r_pse.R_Blk.GetAverageSpread_WithAdjustment();
						const double avg_spread_tp = r_pse.R_Blk.GetAverageSpread_WithAdjustment();
						const double sl = r_s.CalcSL(last_value, spike_quant, avg_spread_sl);
						const double tp = r_s.CalcTP(last_value, spike_quant, avg_spread_tp);
						const long trange_fmt = MKSFMTD(0, 10, NMBF_NOTRAILZ);
						log_msg.Z().Cat("Stake").CatDiv(':', 2).
							Cat(stk_symb).CatChar('-').
							Cat((r_s.BaseFlags & r_s.bfShort) ? "S" : "B").CatChar('/').
							Cat(r_pse.Volume, MKSFMTD(0, 3, NMBF_NOTRAILZ)).CatChar('/').
							Cat(last_value, MKSFMTD(0, 5, NMBF_NOTRAILZ)).CatChar('/').
							Cat(sl, MKSFMTD(0, 7, NMBF_NOTRAILZ)).CatChar('/').
							Cat(tp, MKSFMTD(0, 7, NMBF_NOTRAILZ));
						log_msg.Space().Cat("SP").CatChar('<').Cat(spike_quant, MKSFMTD(0, 9, 0)).CatChar('/').Cat(r_s.SpikeQuant_s, MKSFMTD(0, 9, 0)).CatChar('>'); // @v10.4.10
						log_msg.Space().Cat(PPObjTimeSeries::StrategyToString(r_s, &r_pse.R_Blk.Strategies, &r_pse.Tv, &r_pse.Tv2, temp_buf));
						{
							//temp_buf.Z().Cat("PPAT").Comma().Cat(r_s.MaxDuckQuant).Comma().Cat(r_s.TargetQuant).Comma().Cat(r_s.SpikeQuant, MKSFMTD(0, 9, 0));
							StakeCommentBlock scb;
							scb.MakeText(r_s, spike_quant, temp_buf);
						}
						log_msg.Space().Cat(temp_buf);
						PPLogMessage(PPFILNAM_TSSTAKE_LOG, log_msg, LOGMSGF_TIME|LOGMSGF_DBINFO);
						if(!(Cfg.Flags & Cfg.fTestMode)) {
							TsStakeEnvironment::StakeRequestBlock::Req req;
							req.Action = TsStakeEnvironment::traDeal;
							req.Type = (r_s.BaseFlags & r_s.bfShort) ? TsStakeEnvironment::ordtSell : TsStakeEnvironment::ordtBuy;
							req.TsID = r_pse.R_Blk.PPTS.Rec.ID;
							rResult.AddS(stk_symb, &req.SymbolP);
							if(r_pse.R_Blk.Flags & TimeSeriesBlock::fVolumeMult1000) // @v10.5.6
								req.Volume = r_pse.Volume / 100.0; // @v10.5.7 1000.0-->100.0
							else
								req.Volume = r_pse.Volume;
							req.SL = sl;
							req.TP = tp;
							rResult.AddS(temp_buf, &req.CommentP);
							rResult.L.insert(&req);
						}
					}
				}
			}
		}
	}
	ufp.SetFactor(0, static_cast<double>(StkEnv.TL.getCount()));
	ufp.Commit();
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
		p_cache_rec->SpikeQuant_tc = rec.SpikeQuant_t;
		p_cache_rec->AvgSpread = rec.AvgSpread;
		p_cache_rec->OptMaxDuck = rec.OptMaxDuck;
		p_cache_rec->OptMaxDuck_S = rec.OptMaxDuck_S;
		p_cache_rec->Prec     = rec.Prec;
		p_cache_rec->TargetQuant = rec.TargetQuant; // @v10.4.2
		p_cache_rec->Type = rec.Type; // @v10.5.6

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
	PPTimeSeries * p_data_rec = static_cast<PPTimeSeries *>(pDataRec);
	const Data * p_cache_rec = static_cast<const Data *>(pEntry);
	memzero(p_data_rec, sizeof(*p_data_rec));
	p_data_rec->Tag   = PPOBJ_TIMESERIES;
	p_data_rec->ID    = p_cache_rec->ID;
	p_data_rec->Flags = p_cache_rec->Flags;
	p_data_rec->BuyMarg  = p_cache_rec->BuyMarg;
	p_data_rec->SellMarg = p_cache_rec->SellMarg;
	p_data_rec->SpikeQuant_t = p_cache_rec->SpikeQuant_tc;
	p_data_rec->AvgSpread = p_cache_rec->AvgSpread;
	p_data_rec->OptMaxDuck = p_cache_rec->OptMaxDuck;
	p_data_rec->OptMaxDuck_S = p_cache_rec->OptMaxDuck_S;
	p_data_rec->Prec     = p_cache_rec->Prec;
	p_data_rec->TargetQuant = p_cache_rec->TargetQuant; // @v10.4.2
	p_data_rec->Type = p_cache_rec->Type; // @v10.5.6
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
		SString & r_org_symb = SLS.AcquireRvlStr();
		SString & r_symb = SLS.AcquireRvlStr(); // @v10.6.10
		for(uint i = 0; !p_result && i < TsC.getCount(); i++) {
			TimeSeriesBlock * p_blk = TsC.at(i);
			if(p_blk) {
				r_org_symb = p_blk->T_.GetSymb();
				// @v10.6.10 {
				size_t dot_pos = 0;
				if(r_org_symb.SearchChar('.', &dot_pos) && dot_pos > 0)
					r_org_symb.Sub(0, dot_pos, r_symb);
				else
					r_symb = r_org_symb;
				// } @v10.6.10 
				if(r_symb.Len() == (symb_len+base_symb_len)) {
					if(r_symb.CmpPrefix(pSymb, 1) == 0 && r_symb.CmpSuffix(p_base_symb, 1) == 0) {
						p_result = p_blk;
						reverse = 0;
					}
					else if(r_symb.CmpPrefix(p_base_symb, 1) == 0 && r_symb.CmpSuffix(pSymb, 1) == 0) {
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

int SLAPI TimeSeriesCache::EvaluateTrends(TimeSeriesBlock * pBlk, const STimeSeries * pFullTs, int onActualTicks)
{
	int    ok = 1;
	PROFILE_START
	const uint full_tsc = pFullTs ? pFullTs->GetCount() : 0;
	const uint tsc = pBlk->T_.GetCount();
	uint  max_opt_delta2_stride = 0;
	LongArray ifs_list;
	RealArray temp_err_list;
	if(tsc > 500 && pBlk->Strategies.GetInputFramSizeList(ifs_list, &max_opt_delta2_stride) > 0) {
		const uint trend_nominal_count = max_opt_delta2_stride + 3; // @v10.4.11 (+1)-->(+3)
		ifs_list.sortAndUndup(); // @paranoic
		for(uint i = 0; i < ifs_list.getCount(); i++) {
			const uint ifs = static_cast<uint>(ifs_list.get(i));
			if(ifs > 0 && tsc >= (ifs+trend_nominal_count)) {
				PPObjTimeSeries::TrendEntry * p_entry = 0;
				for(uint j = 0; !p_entry && j < pBlk->TrendList.getCount(); j++) {
					PPObjTimeSeries::TrendEntry * p_iter = pBlk->TrendList.at(j);
					if(p_iter && p_iter->Stride == ifs)
						p_entry = p_iter;
				}
				if(!p_entry) {
					THROW_SL(p_entry = new PPObjTimeSeries::TrendEntry(ifs, trend_nominal_count));
					// @v10.6.8 {
					{
						// Для вывода информации о расчете ставок нам понадобится средняя ошибка каждого тренда, учитываемая стратегиями.
						double err_avg = 0.0;
						uint   err_avg_count = 0;
						for(uint sidx = 0; sidx < pBlk->Strategies.getCount(); sidx++) {
							const PPObjTimeSeries::Strategy & r_sitem = pBlk->Strategies.at(sidx);
							if(r_sitem.InputFrameSize == ifs && r_sitem.TrendErrAvg > 0.0) {
								err_avg += r_sitem.TrendErrAvg;
								err_avg_count++;
							}
							// @v10.7.3 {
							else if(r_sitem.MainFrameSize == ifs && r_sitem.MainTrendErrAvg > 0.0) {
								err_avg += r_sitem.TrendErrAvg;
								err_avg_count++;
							}
							// } @v10.7.3 
						}
						if(err_avg_count)
							p_entry->ErrAvg = err_avg / static_cast<double>(err_avg_count);
					}
					// } @v10.6.8 
					THROW_SL(pBlk->TrendList.insert(p_entry));
				}
				{
					STimeSeries::AnalyzeFitParam afp(ifs, tsc-p_entry->NominalCount, p_entry->NominalCount);
					THROW(pBlk->T_.AnalyzeFit("close", afp, &p_entry->TL, 0, &temp_err_list, 0, 0));
					{
						const uint err_list_count = temp_err_list.getCount();
						for(uint eridx = 0; eridx < err_list_count; eridx++) {
							const double err = temp_err_list.at(eridx);
							assert(err >= 0.0);
							if(err > 0.0)
								temp_err_list.at(eridx) = sqrt(err);
						}
						// @v10.6.8 {
						//
						// В случае расчета трендов на основании актуальных тиков ошибку регрессии берем из предпоследнего 
						// значения так как последнее значение сильно искажено.
						//
						if(onActualTicks && err_list_count > 1)
							temp_err_list.at(err_list_count-1) = temp_err_list.at(err_list_count-2);
						// } @v10.6.8 
						p_entry->ErrL = temp_err_list;
					}
					// @v10.4.10 {
					{
						STimeSeries::Stat st(0);
						THROW(pBlk->T_.Analyze("close", tsc-p_entry->Stride, p_entry->Stride, st));
						p_entry->SpikeQuant_r = st.DeltaAvg / 2.0;
					}
					// } @v10.4.10 
				}
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
	PPTimeSeriesPacket ts_pack;
	STimeSeries ts_full;
	int r = rTsObj.SearchBySymb(pSymb, &id, 0);
	THROW(r);
	if(r < 0) {
		id = 0;
		STRNSCPY(ts_pack.Rec.Name, pSymb);
		STRNSCPY(ts_pack.Rec.Symb, pSymb);
		THROW(rTsObj.PutPacket(&id, &ts_pack, 1));
		ts_pack.Rec.ID = id;
	}
	else {
		THROW(rTsObj.GetPacket(id, &ts_pack) > 0);
	}
	THROW_SL(p_fblk = TsC.CreateNewItem());
	p_fblk->T_.SetSymb(pSymb);
	p_fblk->PPTS = ts_pack;
	THROW(r = rTsObj.GetTimeSeries(id, ts_full));
	if(r > 0) {
		const uint full_tsc = ts_full.GetCount();
		ts_full.GetChunkRecentCount(MIN(10000, full_tsc), p_fblk->T_);
		if(0) { // @debug
			SString temp_buf;
			SString file_name;
			SString line_buf;
			temp_buf.Z().Cat("ts-raw-work").CatChar('-').Cat(ts_pack.GetSymb()).Dot().Cat("txt").ToLower();
			PPGetFilePath(PPPATH_OUT, temp_buf, file_name);
			SFile f_out(file_name, SFile::mWrite);
			if(f_out.IsValid()) {
				const uint tsc = p_fblk->T_.GetCount();
				uint vec_idx = 0;
				if(p_fblk->T_.GetValueVecIndex("close", &vec_idx)) {
					LDATETIME t_prev = ZERODATETIME;
					for(uint j = 0; j < tsc; j++) {
						LDATETIME t = ZERODATETIME;
						double v = 0.0;
						SUniTime ut;
						p_fblk->T_.GetTime(j, &ut);
						ut.Get(t);
						p_fblk->T_.GetValue(j, vec_idx, &v);
						long td = j ? diffdatetimesec(t, t_prev) : 0;
						line_buf.Z().Cat(t, DATF_ISO8601|DATF_CENTURY, 0).Tab().Cat(v, MKSFMTD(10, 5, 0)).Tab().Cat(td);
						f_out.WriteLine(line_buf.CR());
						t_prev = t;
					}
				}
			}
		}
		// @v10.8.8 rTsObj.GetStrategies(id, PPObjTimeSeries::sstSelection, p_fblk->Strategies);
		// @v10.8.8 p_fblk->Strategies.CreateIndex1(p_fblk->StratIndex);
		// @v10.8.8 THROW(EvaluateTrends(p_fblk, 0, 0/*onActualTicks*/));
		THROW(Helper_LoadStrategies(rTsObj, id, p_fblk)); // @v10.8.8 
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

int SLAPI PPObjTimeSeries::SetExternTransactionNotification(const TsStakeEnvironment::TransactionNotification & rTa)
{
	TimeSeriesCache * p_cache = GetDbLocalCachePtr <TimeSeriesCache> (PPOBJ_TIMESERIES);
	return p_cache ? p_cache->SetTransactionNotification(&rTa) : 0;
}

int SLAPI PPObjTimeSeries::SetExternTimeSeriesProp(const char * pSymb, const char * pPropSymb, const char * pPropVal)
{
	int    ok = -1;
	int    do_update = 0;
	PPTimeSeriesPacket ts_pack;
	PPID   id = 0;
	if(!isempty(pSymb) && !isempty(pPropSymb)) {
		{
			PPTimeSeries _tsr;
			THROW(SearchBySymb(pSymb, &id, &_tsr) > 0);
			THROW(GetPacket(id, &ts_pack) > 0);
		}
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
						const long prec = temp_buf.NotEmpty() ? temp_buf.ToLong() : 0;
						if(prec != static_cast<long>(ts_pack.Rec.Prec)) {
							ts_pack.Rec.Prec = static_cast<int16>(prec);
							do_update = 1;
						}
					}
					break;
				case PPHS_TIMSER_PROP_MARGIN_BUY:
					{
						const double marg = temp_buf.ToReal();
						if(marg != static_cast<long>(ts_pack.Rec.BuyMarg)) {
							ts_pack.Rec.BuyMarg = marg;
							do_update = 1;
						}
					}
					break;
				case PPHS_TIMSER_PROP_MARGIN_SELL:
					{
						const double marg = temp_buf.ToReal();
						if(marg != static_cast<long>(ts_pack.Rec.SellMarg)) {
							ts_pack.Rec.SellMarg = marg;
							do_update = 1;
						}
					}
					break;
				case PPHS_TIMSER_PROP_CURRENCY_BASE:
					// not implemented
					break;
				case PPHS_TIMSER_PROP_CURRENCY_PROFIT:
					if(temp_buf.NotEmpty()) {
						if(!sstreqi_ascii(temp_buf, ts_pack.Rec.CurrencySymb) && temp_buf.Len() < sizeof(ts_pack.Rec.CurrencySymb)) {
							STRNSCPY(ts_pack.Rec.CurrencySymb, temp_buf);
							do_update = 1;
						}
					}
					break;
				case PPHS_TIMSER_PROP_CURRENCY_MARGIN:
					// not implemented
					break;
				case PPHS_TIMSER_PROP_VOLUME_MIN: // "time-series-volume-min"      // минимальный объем сделки
					{
						const double val = temp_buf.ToReal();
						if(val > 0.0) {
							TimeSeriesCache * p_cache = GetDbLocalCachePtr <TimeSeriesCache> (PPOBJ_TIMESERIES);
							CALLPTRMEMB(p_cache, SetVolumeParams(pSymb, val, 0.0, 0.0));
						}
					}
					break;
				case PPHS_TIMSER_PROP_VOLUME_MAX: // "time-series-volume-max"      // максимальный объем сделки
					{
						const double val = temp_buf.ToReal();
						if(val > 0.0) {
							TimeSeriesCache * p_cache = GetDbLocalCachePtr <TimeSeriesCache> (PPOBJ_TIMESERIES);
							CALLPTRMEMB(p_cache, SetVolumeParams(pSymb, 0.0, val, 0.0));
						}
					}
					break;
				case PPHS_TIMSER_PROP_VOLUME_STEP: // "time-series-volume-step"     // Минимальный шаг изменения объема для заключения сделки
					{
						const double val = temp_buf.ToReal();
						if(val > 0.0) {
							TimeSeriesCache * p_cache = GetDbLocalCachePtr <TimeSeriesCache> (PPOBJ_TIMESERIES);
							CALLPTRMEMB(p_cache, SetVolumeParams(pSymb, 0.0, 0.0, val));
						}
					}
					break;
			}
			if(do_update) {
				THROW(PutPacket(&id, &ts_pack, 1));
				ok = 1;
			}
		}
	}
	CATCHZOK
	return ok;
}
