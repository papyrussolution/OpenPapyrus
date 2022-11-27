// OBJCACHE.CPP
// Copyright (c) A.Sobolev 1998, 1999, 2000, 2004, 2005, 2006, 2007, 2009, 2010, 2012, 2013, 2014, 2015, 2016б 2017, 2018, 2019, 2020, 2021, 2022
// @codepage UTF-8
// @Kernel
//
#include <pp.h>
#pragma hdrstop
//
// Максимальная доля (%) неиспользуемого пространства, занятого для хранения //
// наименований объектов, при превышении которой класс ObjCache запускает
// функцию уплотнения этого пространства (ObjCache::PackNames).
//
#define UNUSED_NAME_SPACE_THRESOULD 80 // @v8.1.4 30-->80
//
//
//
ObjCache::MultTextBlock::MultTextBlock() : /*Ss("/&")*/R_Ss(DS.AcquireRvlSsSCD()), RdMode(0), CurPos(0)
{
}

ObjCache::MultTextBlock::MultTextBlock(const ObjCache * pCache, const ObjCacheEntry * pEntry) : /*Ss("/&")*/R_Ss(DS.AcquireRvlSsSCD()), RdMode(1), CurPos(0)
{
	char   temp_buf[4096];
	pCache->GetName(pEntry, temp_buf, sizeof(temp_buf));
	R_Ss.setBuf(temp_buf, sstrlen(temp_buf)+1);
}

int FASTCALL ObjCache::MultTextBlock::Add(const char * pStr)
{
	assert(RdMode == 0);
	return RdMode ? 0 : R_Ss.add(pStr);
}

int STDCALL ObjCache::MultTextBlock::Get(char * pBuf, size_t bufLen)
{
	assert(RdMode);
	return RdMode ? R_Ss.get(&CurPos, pBuf, bufLen) : 0;
}

int FASTCALL ObjCache::MultTextBlock::Get(SString & rBuf)
{
	assert(RdMode);
	return RdMode ? R_Ss.get(&CurPos, rBuf) : 0;
}

int STDCALL ObjCache::PutTextBlock(const MultTextBlock & rBlk, ObjCacheEntry * pEntry)
{
	assert(rBlk.RdMode == 0);
	return rBlk.RdMode ? 0 : PutName(rBlk.R_Ss.getBuf(), pEntry);
}
//
//
//
void FASTCALL ObjCache::ExtTextBlock::Dirty(PPID id)
{
	{
		SRWLOCKER(Lock, SReadWriteLocker::Write);
		MissingList.Remove(id);
		List.Remove(id);
	}
}

int ObjCache::ExtTextBlock::Fetch(PPID id, SString & rBuf, void * extraPtr)
{
	int    ok = -1;
	if(id > 0) {
		SRWLOCKER(Lock, SReadWriteLocker::Read);
		if(!MissingList.Has(id)) {
			if(List.GetText(id, rBuf) > 0)
				ok = 1;
			else {
				SRWLOCKER_TOGGLE(SReadWriteLocker::Write);
				if(Implement_Get(id, rBuf, extraPtr) > 0 && rBuf.NotEmpty()) {
					MissingList.Remove(id);
					List.AddFast(id, rBuf);
					ok = 1;
				}
				else
					MissingList.Add(id);
			}
		}
	}
	return ok;
}

/*virtual*/int ObjCache::ExtTextBlock::Implement_Get(PPID id, SString & rBuf, void * extraPtr)
{
	rBuf.Z();
	return -1;
}
//
//
//
ObjCache::ObjCache(PPID objType, size_t entrySize, long flags) : ObjType(objType), EntrySize(entrySize), UnusedNameSpace(0), 
	Flags(flags), LastPackNamesClock(0), EntryToDataFailed(0)
{
	P_Ary = (flags & fOwnerArray) ? 0 : new SArray(EntrySize, O_ARRAY);
	PPLoadText(PPTXT_STATLOG_OBJCACHE, StatLogMsg);
	MEMSZERO(StatData);
	Ss.add("$"); // zero index - is empty string
}

ObjCache::~ObjCache()
{
	delete P_Ary;
}
/*
int ObjCache::LogStat() const
{
	// "STAT  Cache (ObjType=%ld): hits=%ld misses=%ld count=%ld; data size=%u; data count=%u; string size=%u"
	int    ok = -1;
	if(StatLogMsg.NotEmpty()) {
		SString msg_buf;
		msg_buf.Printf(StatLogMsg, ObjType, StatData.Hits, StatData.Misses, StatData.Count,
			GetCount() * EntrySize, GetCount(), Ss.getDataLen());
		PPLogMessage(PPFILNAM_INFO_LOG, msg_buf, LOGMSGF_TIME);
		ok = 1;
	}
	return ok;
}
*/

int STDCALL ObjCache::PutName(const char * pName, ObjCacheEntry * pEntry)
{
	int ok = Ss.add(pName, &pEntry->NameIdx) ? 1 : PPSetErrorSLib();
	return ok;
}

void STDCALL ObjCache::GetName(const ObjCacheEntry * pEntry, char * pBuf, size_t bufLen) const
{
	if(pBuf && pEntry) { // @v10.4.3
		uint   sp = pEntry->NameIdx;
		if(sp)
			Ss.get(&sp, pBuf, bufLen);
		else
			pBuf[0] = 0;
	}
}

int ObjCache::PackNames()
{
	int    ok = -1;
	const size_t ss_len = Ss.getDataLen();
	if(ss_len > SMEGABYTE(4)) { // @v11.0.4 SMEGABYTE(1)-->SMEGABYTE(4)
		double p = fdivui(UnusedNameSpace, ss_len) * 100.0; // @v11.0.4 @fix fdivui(UnusedNameSpace*100, ss_len)-->fdivui(UnusedNameSpace, ss_len) * 100.0
		if(p > UNUSED_NAME_SPACE_THRESOULD) {
			const uint64 pt = SLS.GetProfileTime();
			if(!LastPackNamesClock || (pt - LastPackNamesClock) > 1000000000i64) { // 1sec (чистое процессорное время) @v8.1.5 // @v9.0.8 60-->1
				PPUserFuncProfiler ufp(PPUPRF_PACKCACHETEXT);
				SString temp_buf;
				StringSet tempss;
				tempss.add("$"); // zero index - is empty string
				const uint c = GetCount();
				for(uint i = 0; i < c; i++) {
					ObjCacheEntry * p_entry = SearchByPos(i, 0);
					if(p_entry) {
						uint   pos = p_entry->NameIdx;
						if(pos) {
							Ss.get(&pos, temp_buf);
							tempss.add(temp_buf, &p_entry->NameIdx);
						}
					}
				}
				Ss = tempss;
				UnusedNameSpace = 0;
				ufp.SetFactor(0, static_cast<double>(c));
				ufp.Commit();
				ok = 1;
				LastPackNamesClock = SLS.GetProfileTime();
			}
		}
	}
	return ok;
}

uint ObjCache::GetCount() const
{
	return SVectorBase::GetCount(P_Ary);
}

int FASTCALL ObjCache::Search(PPID id, uint * pPos) const
{
	if(P_Ary && P_Ary->bsearch(&id, pPos, CMPF_LONG))
		return 1;
	else {
		ASSIGN_PTR(pPos, UNDEFPOS);
		return 0;
	}
}

ObjCacheEntry * FASTCALL ObjCache::SearchByPos(uint pos, int incr) const
{
	ObjCacheEntry * p_entry = 0;
	if(pos < SVectorBase::GetCount(P_Ary)) {
		p_entry = static_cast<ObjCacheEntry *>(P_Ary->at(pos));
		if(incr)
			p_entry->Counter.Incr();
	}
	return p_entry;
}

int ObjCache::AddItem(const ObjCacheEntry * pEntry, uint * pPos)
{
	int    ok = -1;
	uint   idx = *pPos;
	if(P_Ary) {
		ObjCacheEntry * p_entry = 0;
		if(Search(pEntry->ID, &idx) && (p_entry = (ObjCacheEntry *)P_Ary->at(idx)) != 0) {
			if(p_entry->NameIdx && Ss.getDataLen() > 3) // (3) - нулевая заглушка и терминатор
				UnusedNameSpace += Ss.getLen(p_entry->NameIdx)+1; // (+1) - разделитель строк
			//
			// Чтобы не затереть счетчик обращений копируем только прикладную часть буфера
			// и индекс строки наименования //
			//
			memcpy(p_entry+1, pEntry+1, EntrySize-sizeof(ObjCacheEntry));
			p_entry->F &= ~ObjCacheEntry::fDirty;
			p_entry->NameIdx = pEntry->NameIdx;
			PackNames();
			ok = 2;
		}
		else
			ok = P_Ary->ordInsert(pEntry, &(idx = 0), CMPF_LONG) ? 1 : PPSetErrorSLib();
	}
	*pPos = idx;
	return ok;
}
//
//
//
int FASTCALL ObjCache::Helper_Dirty(PPID id)
{
	int    ok = -1;
	uint   p = 0;
	if(Search(id, &p)) {
		ObjCacheEntry * p_entry = SearchByPos(p, 0);
		if(p_entry)
			p_entry->F |= ObjCacheEntry::fDirty;
	}
	if(Flags & fUseUndefList) {
		UndefList.Remove(id);
	}
	return ok;
}

int FASTCALL ObjCache::Dirty(PPID id)
{
	int    ok = 1;
	{
		SRWLOCKER(RwL, SReadWriteLocker::Write);
		ok = Helper_Dirty(id);
	}
	return ok;
}

int ObjCache::Put(PPID id, uint * p, long extraData)
{
	int    ok = 1;
	char   temp_buf[256];
	ObjCacheEntry * p_entry = 0;
	int    new_buf = 0;
	if(EntrySize > sizeof(temp_buf)) {
		THROW_MEM(p_entry = static_cast<ObjCacheEntry *>(SAlloc::M(EntrySize)));
		new_buf = 1;
	}
	else
		p_entry = reinterpret_cast<ObjCacheEntry *>(temp_buf);
	memzero(p_entry, EntrySize);
	THROW(ok = FetchEntry(id, p_entry, extraData));
	if(ok > 0) {
		p_entry->ID = id;
		p_entry->F &= ~ObjCacheEntry::fDirty;
		THROW(AddItem(p_entry, p));
	}
	else if(ok == -100 && Flags & fUseUndefList) {
		UndefList.Add(id);
	}
	CATCHZOK
	if(new_buf)
		SAlloc::F(p_entry);
	return ok;
}

int FASTCALL ObjCache::Helper_GetByPos(uint pos, void * pDataRec)
{
	int    ok = 1;
	ObjCacheEntry * p_entry = SearchByPos(pos, 1);
	if(p_entry) {
		if(p_entry->F & ObjCacheEntry::fDirty)
			ok = -1;
		else {
			EntryToDataFailed = 0; // @v10.4.3
			EntryToData(p_entry, pDataRec);
			if(EntryToDataFailed) // @v10.4.3
				ok = -1;
		}
	}
	else
		ok = 0;
	return ok;
}

int ObjCache::GetBySrchFunc(const void * pKey, CompFunc f, void * pDataRec)
{
	int    ok = -1;
	uint   p = 0;
	if(pKey && f) {
		SRWLOCKER(RwL, SReadWriteLocker::Read);
		StatData.Count.Incr();
		if(P_Ary && P_Ary->lsearch(pKey, &p, f)) {
			ok = Helper_GetByPos(p, pDataRec);
			if(ok > 0)
				StatData.Hits.Incr();
		}
	}
	return ok;
}

int FASTCALL ObjCache::Helper_Get(PPID id, void * pDataRec)
{
	int    ok = -1;
	uint   p = 0;
	if(Flags & fUseUndefList && UndefList.Has(id)) {
		char   undef_buf[256];
		ObjCacheEntry * p_undef_entry = 0;
		int    new_temp_buf = 0;
		if(EntrySize > sizeof(undef_buf)) {
			p_undef_entry = reinterpret_cast<ObjCacheEntry *>(new uint8[EntrySize]);
			new_temp_buf = 1;
		}
		else
			p_undef_entry = reinterpret_cast<ObjCacheEntry *>(undef_buf);
		p_undef_entry->ID = id;
		p_undef_entry->F  = ObjCacheEntry::fUndef;
		EntryToData(p_undef_entry, pDataRec);
		if(new_temp_buf)
			delete [] p_undef_entry;
		ok = 100;
	}
	else if(Search(id, &p)) {
		ok = Helper_GetByPos(p, pDataRec);
	}
	if(ok > 0)
		StatData.Hits.Incr();
	return ok;
}

int ObjCache::Get(PPID id, void * pDataRec, long extraData)
{
	int    ok = -1;
	uint   p = 0;
	if(id) {
		SRWLOCKER(RwL, SReadWriteLocker::Read);
		StatData.Count.Incr();
		ok = Helper_Get(id, pDataRec);
		if(ok < 0) {
			SRWLOCKER_TOGGLE(SReadWriteLocker::Write);
			//
			// Пока мы ждали своей очереди на запись
			// другой поток мог занести нужный нам элемент в кэш.
			// По-этому снова проверяем наличие этого элемента.
			//
			ok = Helper_Get(id, pDataRec);
			if(ok < 0) {
				int    r = Put(id, &p, extraData);
				if(r > 0) {
					StatData.Misses.Incr();
					ok = Helper_GetByPos(p, pDataRec);
				}
				else
					ok = r;
			}
		}
	}
	else
		ok = (PPSetObjError(PPERR_OBJNFOUND, ObjType, id), -1);
	return ok;
}

int ObjCache::GetStat(ObjCacheStat * pStat)
{
	if(pStat) {
		memzero(pStat, sizeof(*pStat));
		pStat->Count = StatData.Count;
		pStat->Hits = StatData.Hits;
		pStat->Misses = StatData.Misses;
		pStat->Collisions = StatData.Collisions;

		SRWLOCKER(RwL, SReadWriteLocker::Read);
		uint c = GetCount();
		long max_cntr = 0;
		long min_cntr = MAXLONG;
		for(uint i = 0; i < c; i++) {
			ObjCacheEntry * p_entry = SearchByPos(i, 0);
			if(p_entry) {
				if(p_entry->Counter > max_cntr)
					max_cntr = p_entry->Counter;
				if(p_entry->Counter < min_cntr)
					min_cntr = p_entry->Counter;
				if(p_entry->ID == 0)
					pStat->UnusedEntries++;
				else if(p_entry->F & ObjCacheEntry::fDirty)
					pStat->DirtyEntries++;
			}
		}
		pStat->NumEntries = c;
		pStat->SsSize = Ss.getSize(); // @v9.9.5
		pStat->SsDataLen = Ss.getDataLen(); // @v9.9.5
	}
	return 1;
}
//
//
//
ObjCache::RefSymbArray::RefSymbArray(PPID objType) : StrAssocArray(), ObjType(objType), Inited(0)
{
	assert(ObjType > 0 && ObjType < 1000);
}

void FASTCALL ObjCache::RefSymbArray::Dirty(PPID id) // @sync_w
{
	{
		SRWLOCKER(SlLock, SReadWriteLocker::Write);
		if(Inited) {
			//
			// Ради ускорения не используем здесь addUnique. Метод FetchBySymb
			// выполнит sortAndUndup прежде чем использовать массив DirtyTable.
			//
			DirtyTable.add(id);
		}
	}
}

int ObjCache::RefSymbArray::FetchBySymb(const char * pSymb, PPID * pID)
{
	int    ok = -1;
	PPID   id = 0;
	{
		SRWLOCKER(SlLock, SReadWriteLocker::Read);
		if(!Inited || DirtyTable.getCount()) {
			SRWLOCKER_TOGGLE(SReadWriteLocker::Write);
			if(!Inited || DirtyTable.getCount()) {
				Reference * p_ref = PPRef;
				Reference2Tbl::Rec rec;
				if(!Inited) {
					Z();
					for(SEnum en = p_ref->Enum(ObjType, 0); ok && en.Next(&rec) > 0;) {
						if(!Add(rec.ObjID, rec.Symb))
							ok = 0;
					}
				}
				else {
					DirtyTable.sortAndUndup();
					for(uint i = 0; ok && i < DirtyTable.getCount(); i++) {
						PPID   id = DirtyTable.get(i);
						if(p_ref->GetItem(ObjType, id, &rec) > 0) {
							if(!Add(id, rec.Symb, 1))
								ok = 0;
						}
						else
							Remove(id);
					}
				}
				if(ok) {
					DirtyTable.clear();
					Inited = 1;
				}
			}
		}
		if(ok) {
			uint   pos = 0;
			if(SearchByText(pSymb, 1, &pos)) {
				id = at_WithoutParent(pos).Id;
				ok = 1;
			}
		}
	}
	if(ok < 0) {
		SString msg_buf;
		PPSetError(PPERR_OBJBYSYMBNFOUND, GetObjectTitle(ObjType, msg_buf).CatDiv(':', 1).Cat(pSymb));
	}
	ASSIGN_PTR(pID, id);
	return ok;
}
//
//
//
ObjCacheHash::ObjCacheHash(PPID objType, size_t entrySize, size_t cacheSize, uint maxTries, long flags) :
	ObjCache(objType, entrySize, flags), P_Items(0)
{
	Init(cacheSize, maxTries);
}

ObjCacheHash::~ObjCacheHash()
{
	delete P_Items;
}

int ObjCacheHash::Init(size_t cacheSize, uint maxTries)
{
	int    ok = 1;
	ZDELETE(P_Ary);
	ZDELETE(P_Items);
	Size = (cacheSize > 0 && cacheSize < UINT_MAX) ? cacheSize : DEF_OBJCACHEHASH_SIZE;
	MaxItems = Size / EntrySize;
	MaxTries = (maxTries > 0 && maxTries < MaxItems / 4) ? maxTries : DEF_OBJCACHEHASH_MAXTRIES;
	const  uint delta = MaxItems / 16;
	do {
		P_Items = static_cast<ObjCacheEntry *>(SAlloc::C(MaxItems, EntrySize));
	} while(!P_Items && (MaxItems -= delta) > delta);
	if(P_Items == 0) {
		MaxItems = 0;
		Size = 0;
		MaxTries = 0;
		ok = PPSetErrorNoMem();
	}
	else if(MaxTries > (MaxItems / 4))
		MaxTries = MaxItems / 4;
	return ok;
}

uint FASTCALL ObjCacheHash::Hash(PPID id, uint n) const
{
	//
	// Результаты сравнения эффективности хэш-функций до v7.8.3 (закомментировано ниже) и v7.8.3 для товаров
	// Общее улучшение эффективности попаданий ~0.4%
	// before 7.8.3 1009;1206872;1124424;82448;[0.931685];0;16384;3;0.000183;0;
	// 7.8.3        1009;1206872;1128818;78054;[0.935325];0;16384;0;0;0;
	// Надо еще сопоставить среднее количество попыток для нахождения цели.
	//
	// @v7.8.3 {
	uint32 d[2];
	d[0] = id;
	d[1] = n;
	return (SlHash::BobJenc(d, sizeof(d)) % (long)MaxItems);
	// } @v7.8.3
	// @v7.8.3 return (uint)((id + (long)(n * n)) % (long)MaxItems);
}

uint ObjCacheHash::GetCount() const { return MaxItems; }

ObjCacheEntry * FASTCALL ObjCacheHash::SearchByPos(uint pos, int incr) const
{
	ObjCacheEntry * p_entry = 0;
	if(pos < MaxItems) {
		p_entry = reinterpret_cast<ObjCacheEntry *>(PTR8(P_Items) + pos * EntrySize);
		if(incr)
			p_entry->Counter.Incr();
	}
	return p_entry;
}

int FASTCALL ObjCacheHash::Search(PPID id, uint * pIndex) const
{
	int    found = 0;
	uint   idx = UNDEFPOS;
	if(id)
		for(uint t = 0; !found && t < MaxTries; t++) {
			const  uint f = Hash(id, t);
			ObjCacheEntry * p_entry = SearchByPos(f, 0);
			if(p_entry && p_entry->ID == id) {
				idx = f;
				found = 1;
			}
		}
	ASSIGN_PTR(pIndex, idx);
	return found;
}

int ObjCacheHash::AddItem(const ObjCacheEntry * pEntry, uint * pPos)
{
	int    ok = 0;
	int    pos_undefined = 1; // @debug
	uint   idx = *pPos;
	long   min_count = MAXLONG;
	for(uint t = 0; min_count && !ok && t < MaxTries; t++) {
		const  uint f = Hash(pEntry->ID, t);
		ObjCacheEntry * p_entry = SearchByPos(f, 0);
		if(p_entry) {
			if(p_entry->ID == pEntry->ID) {
				idx = f;
				pos_undefined = 0;
				if(p_entry->NameIdx && Ss.getDataLen() > 3)
					UnusedNameSpace += Ss.getLen(p_entry->NameIdx)+1; // (+1) - разделитель строк
				//
				// Чтобы не затереть счетчик обращений копируем только прикладную часть буфера
				// и индекс строки наименования //
				//
				memcpy(p_entry+1, pEntry+1, EntrySize-sizeof(ObjCacheEntry));
				p_entry->F &= ~ObjCacheEntry::fDirty;
				p_entry->NameIdx = pEntry->NameIdx;
				PackNames();
				ok = 2;
			}
			else if(p_entry->Counter < min_count) {
				idx = f;
				min_count = p_entry->Counter;
				pos_undefined = 0;
			}
		}
	}
	assert(pos_undefined == 0); // @debug @v6.4.11
	if(ok <= 0 && idx < GetCount()) {
		ObjCacheEntry * p_entry = SearchByPos(idx, 0);
		if(p_entry) {
			if(p_entry->NameIdx && Ss.getDataLen() > 3)
				UnusedNameSpace += Ss.getLen(p_entry->NameIdx)+1; // (+1) - разделитель строк
			memcpy(p_entry, pEntry, EntrySize);
			PackNames();
			ok = 1;
		}
	}
	ASSIGN_PTR(pPos, idx);
	return ok;
}
//
// Менеджер локальных по отношению к базе данных объектных кэшей
//
DbLocalCacheMng::DbLocalCacheMng() : TSArray <DbLocalCacheEntry> (aryDataOwner | aryEachItem), DbEntryList(sizeof(CDbEntry))
{
}

DbLocalCacheMng::~DbLocalCacheMng()
{
	{
		SRWLOCKER(RwL, SReadWriteLocker::Write);
		freeAll();
	}
}

int DbLocalCacheMng::CreateDbEntry(long dbPathID)
{
	int    ok = -1;
	uint   pos = 0;
	{
		SRWLOCKER(RwL, SReadWriteLocker::Read);
		if(!DbEntryList.lsearch(&dbPathID, &pos, CMPF_LONG)) {
			SRWLOCKER_TOGGLE(SReadWriteLocker::Write);
			if(!DbEntryList.lsearch(&dbPathID, &pos, CMPF_LONG)) {
				CDbEntry entry;
				entry.DbPathId = dbPathID;
				entry.State = 0;
				entry.LastCacheUpdate = getcurdatetime_();
				DbEntryList.insert(&entry);
				ok = 1;
			}
		}
	}
	return ok;
}

int DbLocalCacheMng::SetDeferredState(long dbPathID, int set)
{
	int    ok = 0;
	{
		SRWLOCKER(RwL, SReadWriteLocker::Write);
		uint   pos = 0;
		if(DbEntryList.lsearch(&dbPathID, &pos, CMPF_LONG)) {
			CDbEntry * p_entry = static_cast<CDbEntry *>(DbEntryList.at(pos));
			SETFLAG(p_entry->State, stDeferred, set);
			ok = 1;
		}
	}
	return ok;
}

int FASTCALL DbLocalCacheMng::IsDeferredState(long dbPathID)
{
	int    result = 0;
	{
		SRWLOCKER(RwL, SReadWriteLocker::Read);
		uint   pos = 0;
		if(DbEntryList.lsearch(&dbPathID, &pos, CMPF_LONG)) {
			result = BIN(static_cast<CDbEntry *>(DbEntryList.at(pos))->State & stDeferred);
		}
	}
	return result;
}

int DbLocalCacheMng::HasDbEntry(long dbPathID)
{
	int    ok = 1;
	{
		SRWLOCKER(RwL, SReadWriteLocker::Read);
		uint   pos = 0;
		ok = DbEntryList.lsearch(&dbPathID, &pos, CMPF_LONG);
	}
	return ok;
}

/*virtual*/void FASTCALL DbLocalCacheMng::freeItem(void * pItem)
{
	DbLocalCacheEntry * p_item = static_cast<DbLocalCacheEntry *>(pItem);
	if(p_item) {
		if(p_item->ObjType == PPOBJ_ADVEVENTQUEUE) {
			delete static_cast<PPAdviseEventQueue *>(p_item->P_Cache);
		}
		else {
			delete static_cast<ObjCache *>(p_item->P_Cache);
		}
		p_item->P_Cache = 0;
	}
}

int DbLocalCacheMng::AddCache(long dbPathID, ObjCache * pCache)
{
	int    ok = 0;
	if(dbPathID) {
		if(!GetCache(dbPathID, pCache->GetObjType())) {
			SRWLOCKER(RwL, SReadWriteLocker::Write);
			DbLocalCacheEntry entry;
			entry.DbPathID = dbPathID;
			entry.ObjType = pCache->GetObjType();
			entry.P_Cache = pCache;
			ok = ordInsert(&entry, 0, PTR_CMPFUNC(_2long)) ? 1 : PPSetErrorSLib();
		}
		else {
			SString msg_buf;
			ok = PPSetError(PPERR_DUPDBLOCALCACHE, msg_buf.Cat(pCache->GetObjType()));
		}
	}
	return ok;
}

void * DbLocalCacheMng::Helper_GetEntry(long dbPathID, PPID objType, int lock)
{
	void * p_entry = 0;
	if(dbPathID) {
		SRWLOCKER(RwL, lock ? SReadWriteLocker::Read : SReadWriteLocker::None);
		uint   pos = 0;
		DbLocalCacheEntry key;
		key.DbPathID = dbPathID;
		key.ObjType = objType;
		p_entry = bsearch(&key, &pos, PTR_CMPFUNC(_2long)) ? at(pos).P_Cache : 0;
	}
	return p_entry;
}

ObjCache * FASTCALL DbLocalCacheMng::GetCache(long dbPathID, PPID objType)
{
	assert(objType != PPOBJ_ADVEVENTQUEUE);
	return static_cast<ObjCache *>(Helper_GetEntry(dbPathID, objType, 1));
}

PPAdviseEventQueue * FASTCALL DbLocalCacheMng::GetAdviseEventQueue(long dbPathID)
{
	PPAdviseEventQueue * p_queue = 0;
	if(dbPathID) {
		p_queue = static_cast<PPAdviseEventQueue *>(Helper_GetEntry(dbPathID, PPOBJ_ADVEVENTQUEUE, 1));
		if(!p_queue) {
			SRWLOCKER(RwL, SReadWriteLocker::Write);
			//
			// Возможно, пока мы ждали, объект уже был создан
			//
			p_queue = static_cast<PPAdviseEventQueue *>(Helper_GetEntry(dbPathID, PPOBJ_ADVEVENTQUEUE, 0 /* without locking */));
			if(!p_queue) {
				p_queue = new PPAdviseEventQueue;
				if(p_queue) {
					DbLocalCacheEntry entry;
					entry.DbPathID = dbPathID;
					entry.ObjType = PPOBJ_ADVEVENTQUEUE;
					entry.P_Cache = p_queue;
					if(!ordInsert(&entry, 0, PTR_CMPFUNC(_2long))) {
						ZDELETE(p_queue);
						PPSetErrorSLib();
					}
				}
				else {
					PPSetErrorNoMem();
				}
			}
		}
	}
	return p_queue;
}

void ObjCacheStat::Init()
{
	DbPathID = 0;
	ObjType = 0;
	Hits = 0;
	Misses = 0;
	Count = 0;
	Collisions = 0;
	NumEntries = 0;
	UnusedEntries = 0;
	DirtyEntries = 0;
	MaxCounter = 0;
	MinCounter = 0;
	SsSize = 0;    // @v9.9.5
	SsDataLen = 0; // @v9.9.5
	DbPath.Z();
	ObjTypeName.Z();
}

LDATETIME FASTCALL DbLocalCacheMng::GetLastUpdate(long dbPathID)
{
	uint   pos = 0;
	LDATETIME dtm = ZERODATETIME;
	{
		SRWLOCKER(RwL, SReadWriteLocker::Read);
		if(DbEntryList.lsearch(&dbPathID, &pos, CMPF_LONG))
			dtm = static_cast<CDbEntry *>(DbEntryList.at(pos))->LastCacheUpdate;
	}
	return dtm;
}

int DbLocalCacheMng::SetLastUpdate(long dbPathID, LDATETIME dtm)
{
	uint   pos = 0;
	{
		SRWLOCKER(RwL, SReadWriteLocker::Write);
		if(DbEntryList.lsearch(&dbPathID, &pos, CMPF_LONG)) {
			static_cast<CDbEntry *>(DbEntryList.at(pos))->LastCacheUpdate = dtm;
		}
		else {
			CDbEntry entry;
			entry.DbPathId = dbPathID;
			entry.LastCacheUpdate = dtm;
			DbEntryList.insert(&entry);
		}
	}
	return 1;
}

int DbLocalCacheMng::GetCacheStat(uint pos, ObjCacheStat * pStat)
{
	int    ok = 0;
	{
		SRWLOCKER(RwL, SReadWriteLocker::Read);
		if(pos < getCount()) {
			pStat->Init();
			DbLocalCacheEntry * p_entry = &at(pos);
			if(p_entry && p_entry->P_Cache) {
				if(p_entry->ObjType == PPOBJ_ADVEVENTQUEUE) {
					PPAdviseEventQueue * p_queue = static_cast<PPAdviseEventQueue *>(p_entry->P_Cache);
					// @todo
				}
				else {
					static_cast<ObjCache *>(p_entry->P_Cache)->GetStat(pStat);
					pStat->DbPathID = p_entry->DbPathID;
					pStat->ObjType  = p_entry->ObjType;
					DBS.GetDbPath(p_entry->DbPathID, pStat->DbPath);
				}
				ok = 1;
			}
		}
	}
	return 1;
}

int DbLocalCacheMng::LogCacheStat()
{
	if(getCount()) {
		int    is_new_file = 1;
		SString out_buf, file_name, dtm_buf;
		PPGetFilePath(PPPATH_LOG, PPFILNAM_OBJCACHESTAT, file_name);
		if(fileExists(file_name))
			is_new_file = 0;
		SFile f_out(file_name, SFile::mAppend);
		if(f_out.IsValid()) {
			if(is_new_file) {
				f_out.WriteLine(PPLoadTextS(PPTXT_LOGCACHEHEADER, out_buf));
			}
			ObjCacheStat stat;
			LDATETIME dtm = getcurdatetime_();
			dtm_buf.Cat(dtm);
			for(uint i = 0; i < getCount(); i++) {
				if(GetCacheStat(i, &stat) > 0) {
					out_buf.Z();
					out_buf.Cat(dtm_buf).Semicol();
					out_buf.Cat(stat.DbPath).Semicol();
					out_buf.Cat(stat.ObjType).Semicol();
					out_buf.Cat(stat.Count).Semicol();
					out_buf.Cat(stat.Hits).Semicol();
					out_buf.Cat(stat.Misses).Semicol();
					out_buf.Cat(fdivi(stat.Hits, stat.Count), MKSFMTD(0, 6, NMBF_NOTRAILZ)).Semicol();
					out_buf.Cat(stat.Collisions).Semicol();
					out_buf.Cat(stat.NumEntries).Semicol();
					out_buf.Cat(stat.UnusedEntries).Semicol();
					out_buf.Cat(fdivi(stat.UnusedEntries, stat.NumEntries), MKSFMTD(0, 6, NMBF_NOTRAILZ)).Semicol();
					out_buf.Cat(stat.DirtyEntries).Semicol();
					out_buf.Cat(stat.SsSize).Semicol(); // @v9.9.5
					out_buf.Cat(stat.SsDataLen).Semicol(); // @v9.9.5
					out_buf.CR();
					f_out.WriteLine(out_buf);
				}
			}
		}
	}
	return 1;
}
