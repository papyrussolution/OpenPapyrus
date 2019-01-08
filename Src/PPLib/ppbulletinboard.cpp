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
		//
		double SpikeQuant;     //
		double AvgSpread;      //
		uint32 OptMaxDuck;     // Оптимальная глубина проседания (в квантах) при длинной ставке
		uint32 OptMaxDuck_S;   // Оптимальная глубина проседания (в квантах) при короткой ставке
		//
		int16  Prec;
		uint16 Reserve; // @alignment
	};
	static int OnQuartz(int kind, const PPNotifyEvent * pEv, void * procExtPtr)
	{
		TimeSeriesCache * p_this = (TimeSeriesCache *)procExtPtr;
		if(p_this) {
			LDATETIME cdtm = getcurdatetime_();
			long sec = diffdatetimesec(cdtm, p_this->LastFlashDtm);
			if(sec >= 120) {
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
	int    SLAPI SetTimeSeries(STimeSeries & rTs);
	int    SLAPI GetReqQuotes(TSVector <PPObjTimeSeries::QuoteReqEntry> & rList);
	int    SLAPI SetCurrentStakeEnvironment(const TsStakeEnvironment * pEnv, TsStakeEnvironment::StakeRequestBlock * pRet);
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
	int    SLAPI Flash();
	int    SLAPI EvaluateStakes(TsStakeEnvironment::StakeRequestBlock & rResult)
	{
		int    ok = -1;
		const  LDATETIME now = getcurdatetime_();
		SString temp_buf;
		SString tk_symb;
		SString stk_symb;
		SString stk_memo;
		for(uint i = 0; i < StkEnv.TL.getCount(); i++) {
			const TsStakeEnvironment::Tick & r_tk = StkEnv.TL.at(i);
			long d = diffdatetimesec(now, r_tk.Dtm);
			if(d < 10) {
				StkEnv.GetS(r_tk.SymbP, tk_symb);
				uint  blk_idx = 0;
				const TimeSeriesBlock * p_blk = SearchBlockBySymb(tk_symb, &blk_idx);
				if(p_blk) {
					int   is_there_stake = 0;
					uint  stake_idx = 0;
					for(uint si = 0; si < StkEnv.SL.getCount(); si++) {
						const TsStakeEnvironment::Stake & r_stk = StkEnv.SL.at(si);
						StkEnv.GetS(r_stk.SymbP, stk_symb);
						if(stk_symb.IsEqiAscii(tk_symb)) {
							is_there_stake = 1;
							stake_idx = si;
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
		return ok;
	}
private:
	struct TimeSeriesBlock {
		SLAPI  TimeSeriesBlock();
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
		STimeSeries T;
	};
	virtual int  SLAPI FetchEntry(PPID, ObjCacheEntry * pEntry, long);
	virtual void SLAPI EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const;
	TimeSeriesBlock * SLAPI SearchBlockBySymb(const char * pSymb, uint * pIdx) const;
	TimeSeriesBlock * SLAPI InitBlock(const char * pSymb);

	TSCollection <TimeSeriesBlock> TsC;
	TSVector <PPObjTimeSeries::QuoteReqEntry> * P_ReqQList;
	TsStakeEnvironment StkEnv;
	SMtLock OpL; // Блокировка для операций, иных нежели штатные методы ObjCache
	LDATETIME LastFlashDtm;
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

SLAPI TimeSeriesCache::TimeSeriesBlock::TimeSeriesBlock() : Flags(0), SpreadSum(0), SpreadCount(0), VolumeMin(0.0), VolumeMax(0.0), VolumeStep(0.0)
{
}

int SLAPI TimeSeriesCache::GetReqQuotes(TSVector <PPObjTimeSeries::QuoteReqEntry> & rList)
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
		const uint tc = p_blk ? p_blk->T.GetCount() : 0;
		if(tc && p_blk->T.GetTime(tc-1, &last_utm)) {
			last_utm.Get(r_entry.LastValTime);
		}
		else
			r_entry.LastValTime.Z();
	}
	OpL.Unlock();
	return ok;
}

TimeSeriesCache::TimeSeriesBlock * SLAPI TimeSeriesCache::SearchBlockBySymb(const char * pSymb, uint * pIdx) const
{
	SString temp_buf;
	for(uint i = 0; i < TsC.getCount(); i++) {
		TimeSeriesBlock * p_blk = TsC.at(i);
		if(p_blk) {
			temp_buf = p_blk->T.GetSymb();
			if(temp_buf.IsEqiAscii(pSymb)) {
				ASSIGN_PTR(pIdx, i);
				return p_blk;
			}
		}
	}
	return 0;
}

TimeSeriesCache::TimeSeriesBlock * SLAPI TimeSeriesCache::InitBlock(const char * pSymb)
{
	TimeSeriesBlock * p_fblk = 0;
	PPID   id = 0;
	PPObjTimeSeries ts_obj;
	PPTimeSeries ts_rec;
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
	p_fblk->T.SetSymb(pSymb);
	p_fblk->PPTS = ts_rec;
	THROW(r = ts_obj.GetTimeSeries(id, p_fblk->T));
	if(r < 0) {
		//uint   vecidx_open = 0;
		uint   vecidx_close = 0;
		//uint   vecidx_ticvol = 0;
		uint   vecidx_realvol = 0;
		//uint   vecidx_spread = 0;
		//THROW_SL(p_fblk->T.AddValueVec("open", T_INT32, 5, &vecidx_open));
		//THROW_SL(p_fblk->T.AddValueVec("close", T_DOUBLE, 0, &vecidx_close));
		THROW_SL(p_fblk->T.AddValueVec("close", T_INT32, 5, &vecidx_close));
		//THROW_SL(p_fblk->T.AddValueVec("tick_volume", T_INT32, 0, &vecidx_ticvol));
		THROW_SL(p_fblk->T.AddValueVec("real_volume", T_INT32, 0, &vecidx_realvol));
		//THROW_SL(p_fblk->T.AddValueVec("spread", T_INT32, 0, &vecidx_spread));
	}
	CATCH
		p_fblk = 0;
	ENDCATCH
	return p_fblk;
}

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

int SLAPI TimeSeriesCache::SetCurrentStakeEnvironment(const TsStakeEnvironment * pEnv, TsStakeEnvironment::StakeRequestBlock * pRet)
{
	int    ok = 1;
	OpL.Lock();
	if(pEnv) {
		StkEnv = *pEnv;
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

int SLAPI TimeSeriesCache::SetTimeSeries(STimeSeries & rTs)
{
	int    ok = -1;
	STimeSeries::AppendStat apst;
	OpL.Lock();
	SString temp_buf = rTs.GetSymb();
	if(temp_buf.NotEmpty()) {
		TimeSeriesBlock * p_fblk = SearchBlockBySymb(temp_buf, 0);
		THROW(SETIFZ(p_fblk, InitBlock(temp_buf)));
		{
			rTs.Sort();
			THROW_SL(p_fblk->T.AddItems(rTs, &apst));
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

int SLAPI TimeSeriesCache::Flash()
{
	int    ok = 1;
	OpL.Lock();
	if(TsC.getCount()) {
		STimeSeries t;
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
					THROW(ts_obj.SetTimeSeries(id, &p_blk->T, 0));
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
