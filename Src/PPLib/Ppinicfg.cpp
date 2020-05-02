// PPINICFG.CPP
// Copyright (c) A.Sobolev 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2004, 2005, 2006, 2007, 2012, 2013, 2015, 2016, 2017, 2018, 2019, 2020
// @Kernel
//
#include <pp.h>
#pragma hdrstop
//
// PPIniFile
//

//static
int FASTCALL PPIniFile::GetParamSymb(int idx, SString & rBuf) { return PPLoadText(idx, rBuf.Z()); }
//static
int FASTCALL PPIniFile::GetSectSymb(int idx, SString & rBuf) { return PPLoadText(idx, rBuf.Z()); }

SLAPI PPIniFile::PPIniFile(const char * pFileName, int fcreate, int winCoding, int useIniBuf) :
	SIniFile(pFileName, fcreate, winCoding, useIniBuf)
{
	if(!pFileName)
		Init(getExecPath(TempBuf).SetLastSlash().Cat("PP.INI"), fcreate, winCoding, useIniBuf);
}

SLAPI PPIniFile::PPIniFile() : SIniFile(0, 0, 0, 0)
{
	Init(getExecPath(TempBuf).SetLastSlash().Cat("PP.INI"), 0, 0, 0);
}

int SLAPI PPIniFile::ParamIdToStrings(uint sectId, uint paramId, SString * pSectName, SString * pParam)
{
	if(pSectName) {
		pSectName->Z();
		PPIniFile::GetSectSymb(sectId, *pSectName);
	}
	if(pParam) {
		pParam->Z();
		PPIniFile::GetParamSymb(paramId, *pParam);
	}
	return 1;
}

int SLAPI PPIniFile::Get(uint sectId, uint paramId, SString & rBuf)
{
	SString & r_sect_name = SLS.AcquireRvlStr(); // @v9.9.12
	SString & r_param_name = SLS.AcquireRvlStr(); // @v9.9.12
	ParamIdToStrings(sectId, paramId, &r_sect_name, &r_param_name);
	return GetParam(r_sect_name, r_param_name, rBuf);
}

int SLAPI PPIniFile::Get(uint sectId, const char * pParamName, SString & rBuf)
{
	SString & r_sect_name = SLS.AcquireRvlStr(); // @v9.9.12
	ParamIdToStrings(sectId, 0, &r_sect_name, 0);
	return GetParam(r_sect_name, pParamName, rBuf);
}

int SLAPI PPIniFile::Get(const char * pSectName, uint paramId, SString & rBuf)
{
	SString & r_param_name = SLS.AcquireRvlStr(); // @v9.9.12
	ParamIdToStrings(0, paramId, 0, &r_param_name);
	return GetParam(pSectName, r_param_name, rBuf);
}

int SLAPI PPIniFile::GetInt(uint sectId, uint paramId, int * pVal)
{
	SString & r_sect_name = SLS.AcquireRvlStr(); // @v9.9.12
	SString & r_param_name = SLS.AcquireRvlStr(); // @v9.9.12
	ParamIdToStrings(sectId, paramId, &r_sect_name, &r_param_name);
	return GetIntParam(r_sect_name, r_param_name, pVal);
}

int SLAPI PPIniFile::GetInt(const char * pSectName, uint paramId, int * pVal)
{
	SString & r_param_name = SLS.AcquireRvlStr(); // @v9.9.12
	ParamIdToStrings(0, paramId, 0, &r_param_name);
	return GetIntParam(pSectName, r_param_name, pVal);
}

int SLAPI PPIniFile::GetDataSize(const char * pSectName, uint paramId, int64 * pVal)
{
	SString & r_param_name = SLS.AcquireRvlStr();
	ParamIdToStrings(0, paramId, 0, &r_param_name);
	return GetDataSizeParam(pSectName, r_param_name, pVal);
}

int SLAPI PPIniFile::GetDataSize(uint sectId, uint paramId, int64 * pVal)
{
	SString & r_sect_name = SLS.AcquireRvlStr();
	SString & r_param_name = SLS.AcquireRvlStr();
	ParamIdToStrings(sectId, paramId, &r_sect_name, &r_param_name);
	return GetDataSizeParam(r_sect_name, r_param_name, pVal);
}

int SLAPI PPIniFile::GetEntryList(uint sectId, StringSet * pEntries, int storeAllString)
{
	SString sect_name; // don't use SLS.AcquireRvlStr() (GetEnties uses loop)
	ParamIdToStrings(sectId, 0, &sect_name, 0);
	return SIniFile::GetEntries(sect_name, pEntries, storeAllString);
}

int SLAPI PPIniFile::Append(uint sectId, uint paramId, const char * pVal, int overwrite)
{
	SString sect_name;
	SString param_name;
	ParamIdToStrings(sectId, paramId, &sect_name, &param_name);
	return AppendParam(sect_name, param_name, pVal, overwrite);
}

int SLAPI PPIniFile::IsWinCoding()
{
	int    is_win_coding = BIN(Flags & fWinCoding);
	if(!is_win_coding) {
		int    code_page = 0;
		GetInt(PPINISECT_SYSTEM, PPINIPARAM_CODEPAGE, &code_page);
		is_win_coding = BIN(code_page == 1251);
	}
	return is_win_coding;
}

int SLAPI PPIniFile::Backup(uint maxCopies)
{
	int    ok = 1;
	struct FE { // @flat
		LDATETIME Dtm;
		long   C;
	};
	SString file_name = GetFileName();
	SPathStruc ps(file_name);
	SString new_name, temp_name;
	SString nam = ps.Nam;
	long   c = 0;
	SVector fe_list(sizeof(FE)); // @v9.8.11 SArray-->SVector
	do {
		(ps.Nam = nam).CatChar('-').CatLongZ(++c, 3);
		ps.Merge(temp_name);
		if(fileExists(temp_name)) {
			SFileUtil::Stat st;
			SFileUtil::GetStat(temp_name, &st);
			FE fe;
			fe.Dtm = st.ModTime;
			fe.C = c;
			fe_list.insert(&fe);
		}
		else 
			new_name.SetIfEmpty(temp_name);
	} while(c < 999);
	if(new_name.NotEmpty()) {
		fe_list.sort(PTR_CMPFUNC(LDATETIME));
		if(SCopyFile(file_name, new_name, 0, FILE_SHARE_READ, 0)) {
			if(maxCopies > 0) {
				while(fe_list.getCount() > (maxCopies-1)) {
					(ps.Nam = nam).CatChar('-').CatLongZ(static_cast<const FE *>(fe_list.at(0))->C, 3);
					ps.Merge(new_name);
					SFile::Remove(new_name);
					fe_list.atFree(0);
				}
			}
		}
		else
			ok = 0;
	}
	else
		ok = 0;
	return ok;
}

int SLAPI PPIniFile::UpdateFromFile(const char * pSrcFileName)
{
	int    ok = -1;
	int    backup_done = 0;
	if(fileExists(pSrcFileName)) {
		PPIniFile src_ini(pSrcFileName);
		StringSet sect_set, entry_set;
		SString sect_buf, entry_buf, val_buf;
		THROW_SL(src_ini.GetSections(&sect_set));
		for(uint sp = 0; sect_set.get(&sp, sect_buf);) {
			entry_set.clear();
			THROW(src_ini.GetEntries(sect_buf, &entry_set, 0));
			for(uint ep = 0; entry_set.get(&ep, entry_buf);) {
				src_ini.GetParam(sect_buf, entry_buf, val_buf);
				if(!backup_done) {
					THROW(Backup());
					backup_done = 1;
				}
				THROW(AppendParam(sect_buf, entry_buf, val_buf, 1));
				ok = 1;
			}
		}
	}
	CATCHZOK
	return ok;
}
//
//
//
#if 1 // @construction {

PPConfigDatabase::CObjHeader::CObjHeader() : ID(0), Ver(0), Type(0), Flags(0)
{
}

int PPConfigDatabase::CObjHeader::Cmp(const CObjHeader & rS, long flags) const
{
	int    c = 0;
	if(!(flags & 0x0001)) {
		c = CMPSIGN(ID, rS.ID);
	}
	if(c == 0) {
		CMPCASCADE3(c, this, &rS, Ver, Type, Flags);
		if(c == 0) {
			c = Name.Cmp(rS.Name, 1);
			if(c == 0) {
				c = SubSymb.Cmp(rS.SubSymb, 1);
				if(c == 0) {
					c = DbSymb.Cmp(rS.DbSymb, 1);
					if(c == 0)
						c = OwnerSymb.Cmp(rS.OwnerSymb, 1);
				}
			}
		}
	}
	return c;
}

SLAPI PPConfigDatabase::CObjTbl::CObjTbl(BDbDatabase * pDb) : BDbTable(BDbTable::ConfigHash("ppconfig.db->cobj", 0, 0, 0), pDb), SeqID(0)
{
	//
	// Индекс по идент
	//
	class Idx01 : public SecondaryIndex {
	public:
		virtual int Cb(const BDbTable::Buffer & rKey, const BDbTable::Buffer & rData, BDbTable::Buffer & rResult)
		{
			int32  id = 0;
			rData.Get(&id, sizeof(id));
			rResult.Set(&id, sizeof(id));
			return 0;
		}
	};
	//
	// Индекс по типу
	//
	class Idx02 : public SecondaryIndex {
	public:
		virtual int Cb(const BDbTable::Buffer & rKey, const BDbTable::Buffer & rData, BDbTable::Buffer & rResult)
		{
			SBuffer buf;
			rKey.Get(buf);
			CObjHeader hdr;
			static_cast<PPConfigDatabase::CObjTbl *>(P_MainT)->SerializeKeyBuf(-1, &hdr, buf);
			rResult = hdr.Type;
			return 0;
		}
	};
	new BDbTable(BDbTable::ConfigHash("ppconfig.db->cobj_idx01", 0, 0, 0), pDb, new Idx01, this);
	new BDbTable(BDbTable::ConfigHash("ppconfig.db->cobj_idx02", cfDup, 0, 0), pDb, new Idx02, this);
	if(P_Db)
		THROW(P_Db->CreateSequence("seq_cobj_id", 0, &SeqID));
	CATCH
		Close();
	ENDCATCH
}

SLAPI PPConfigDatabase::CObjTbl::~CObjTbl()
{
	CALLPTRMEMB(P_Db, CloseSequence(SeqID));
}

//virtual
int PPConfigDatabase::CObjTbl::Implement_Cmp(const BDbTable::Buffer * pKey1, const BDbTable::Buffer * pKey2)
{
	int    c = 0;
	if(pKey1 && pKey2) {
		CObjHeader key1, key2;
		SBuffer buf;
		pKey1->Get(buf);
		SerializeKeyBuf(-1, &key1, buf);
		pKey2->Get(buf);
		SerializeKeyBuf(-1, &key2, buf);
		//c = key1.Cmp(key2, 0x0001);
		c = CMPSIGN(key1.Type, key2.Type);
		if(c == 0) {
			c = key1.Name.Cmp(key2.Name, 1);
			if(c == 0) {
				c = key1.SubSymb.Cmp(key2.SubSymb, 1);
				if(c == 0) {
					c = key1.DbSymb.Cmp(key2.DbSymb, 1);
					if(c == 0) {
						c = key1.OwnerSymb.Cmp(key2.OwnerSymb, 1);
					}
				}
			}
		}
	}
	else if(pKey1 == 0 && pKey2 == 0)
		c = 1; // Класс поддерживает собственное сравнение ключей
	return c;
}

int SLAPI PPConfigDatabase::CObjTbl::SerializeKeyBuf(int dir, CObjHeader * pRec, SBuffer & rBuf)
{
	int    ok = 1;
	SSerializeContext * p_sctx = GetSCtx();
	THROW_DB(p_sctx);
	THROW_SL(p_sctx->Serialize(dir, pRec->Ver, rBuf));
	THROW_SL(p_sctx->Serialize(dir, pRec->Type, rBuf));
	THROW_SL(p_sctx->Serialize(dir, pRec->Flags, rBuf));
	THROW_SL(p_sctx->Serialize(dir, pRec->Name, rBuf));
	THROW_SL(p_sctx->Serialize(dir, pRec->SubSymb, rBuf));
	THROW_SL(p_sctx->Serialize(dir, pRec->DbSymb, rBuf));
	THROW_SL(p_sctx->Serialize(dir, pRec->OwnerSymb, rBuf));
	CATCHZOK
	return ok;
}

int SLAPI PPConfigDatabase::AddStringHistory(const char * pKey, const char * pTextUtf8)
{
	int    ok = 0;
	SString key;
	StringHistoryPool * p_pool = 0;
	int    is_pool_allocated = 0;
	(key = pKey).Strip().Utf8ToLower();
	for(uint i = 0; !p_pool && i < ShL.getCount(); i++) {
		StringHistoryPool * p_local_pool = ShL.at(i);
		if(p_local_pool->GetKey() == key) {
			p_pool = p_local_pool;
		}
	}
	if(!p_pool) {
		THROW_SL(p_pool = new StringHistoryPool(key));
		is_pool_allocated = 1;
		THROW(LoadStringHistory(*p_pool));
		ShL.insert(p_pool);
		is_pool_allocated = 0; // to avoid destruction at the end of the function
	}
	assert(p_pool);
	ok = p_pool->Add(pTextUtf8);
	CATCHZOK
	if(is_pool_allocated)
		delete p_pool;
	return ok;
}

int SLAPI PPConfigDatabase::GetRecentStringHistory(const char * pKey, uint maxItems, StringSet & rList)
{
	rList.clear();
	int    ok = 0;
	SString key;
	StringHistoryPool * p_pool = 0;
	int    is_pool_allocated = 0;
	(key = pKey).Strip().Utf8ToLower();
	for(uint i = 0; !p_pool && i < ShL.getCount(); i++) {
		StringHistoryPool * p_local_pool = ShL.at(i);
		if(p_local_pool->GetKey() == key)
			p_pool = p_local_pool;
	}
	if(!p_pool) {
		THROW_SL(p_pool = new StringHistoryPool(key));
		is_pool_allocated = 1;
		THROW(LoadStringHistory(*p_pool));
		ShL.insert(p_pool);
		is_pool_allocated = 0; // to avoid destruction at the end of the function
	}
	assert(p_pool);
	ok = p_pool->GetRecent(maxItems, rList);
	CATCHZOK
	if(is_pool_allocated)
		delete p_pool;
	return ok;
}

int SLAPI PPConfigDatabase::GetStringHistory(const char * pKey, const char * pSubUtf8, long flags, StringSet & rList)
{
	rList.clear();
	int    ok = 0;
	SString key;
	StringHistoryPool * p_pool = 0;
	int    is_pool_allocated = 0;
	(key = pKey).Strip().Utf8ToLower();
	for(uint i = 0; !p_pool && i < ShL.getCount(); i++) {
		StringHistoryPool * p_local_pool = ShL.at(i);
		if(p_local_pool->GetKey() == key)
			p_pool = p_local_pool;
	}
	if(!p_pool) {
		THROW_SL(p_pool = new StringHistoryPool(key));
		is_pool_allocated = 1;
		THROW(LoadStringHistory(*p_pool));
		ShL.insert(p_pool);
		is_pool_allocated = 0; // to avoid destruction at the end of the function
	}
	assert(p_pool);
	ok = p_pool->Get(pSubUtf8, flags, rList);
	CATCHZOK
	if(is_pool_allocated)
		delete p_pool;
	return ok;
}

int SLAPI PPConfigDatabase::LoadStringHistory(StringHistoryPool & rPool)
{
	int    ok = -1;
	BDbTable::Buffer key_buf, data_buf;
	SString temp_buf;
	data_buf.Alloc(4096);
	(temp_buf = rPool.GetKey()).Strip().Utf8ToLower();
	key_buf = temp_buf;
	if(P_ShT->Search(0, key_buf, data_buf)) {
		SBuffer cbuf;
		SSerializeContext sctx;
		data_buf.Get(cbuf);
		{
			const size_t actual_size = cbuf.GetAvailableSize();
			const size_t cs_size = SSerializeContext::GetCompressPrefix(0);
			if(actual_size > cs_size && SSerializeContext::IsCompressPrefix(cbuf.GetBuf(cbuf.GetRdOffs()))) {
				SCompressor compr(SCompressor::tZLib);
				SBuffer sbuf;
				THROW_SL(compr.DecompressBlock(cbuf.GetBuf(cbuf.GetRdOffs()+cs_size), actual_size-cs_size, sbuf));
				THROW_SL(rPool.Serialize(-1, sbuf, &sctx));
			}
			else {
				THROW_SL(rPool.Serialize(-1, cbuf, &sctx));
			}
			THROW(rPool.BuildHash());
		}
		//THROW_SL(rPool.Serialize(-1, buf, &sctx));
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int SLAPI PPConfigDatabase::SaveStringHistory(StringHistoryPool * pPool, int use_ta)
{
	int    ok = -1;
	SString temp_buf;
	if(pPool == 0) {
		if(ShL.getCount()) {
			BDbTransaction tra(P_Db, use_ta);
			THROW_DB(tra);
			for(uint i = 0; i < ShL.getCount(); i++) {
				StringHistoryPool * p_local_pool = ShL.at(i);
				if(p_local_pool) {
					int    r;
					THROW(r = SaveStringHistory(p_local_pool, 0)); // @recursion
					if(r > 0)
						ok = 1;
				}
			}
			THROW_DB(tra.Commit(1));
		}
	}
	else if(pPool->IsSavingNeeded()) {
		temp_buf = pPool->GetKey();
		temp_buf.Strip().Utf8ToLower();
		if(temp_buf.NotEmpty()) {
			StringHistoryPool org_pool(temp_buf);
			BDbTable::Buffer key_buf, data_buf;
			SBuffer sbuf;
			SBuffer cbuf;
			SSerializeContext sctx;
			data_buf.Alloc(4096);
			key_buf = temp_buf;
			int   do_update = 0; // if 1 then update rec else insert
			BDbTransaction tra(P_Db, use_ta);
			THROW_DB(tra);
			if(LoadStringHistory(org_pool) > 0) {
				THROW(pPool->Merge(org_pool));
				THROW_SL(org_pool.Serialize(+1, sbuf.Z(), &sctx));
				do_update = 1;
			}
			else {
				THROW(pPool->Serialize(+1, sbuf, &sctx));
			}
			{
				SCompressor compr(SCompressor::tZLib);
				if(sbuf.GetAvailableSize() > 128) {
					uint8 cs[32];
					const size_t cs_size = SSerializeContext::GetCompressPrefix(cs);
					THROW_SL(cbuf.Write(cs, cs_size));
					THROW_SL(compr.CompressBlock(sbuf.GetBuf(0), sbuf.GetAvailableSize(), cbuf, 0, 0));
				}
				else {
					cbuf = sbuf;
				}
				data_buf = cbuf;
				if(do_update) {
					THROW_DB(P_ShT->UpdateRec(key_buf, data_buf));
				}
				else {
					THROW_DB(P_ShT->InsertRec(key_buf, data_buf));
				}
			}
			THROW_DB(tra.Commit(1));
			pPool->OnSave();
			ok = 1;
		}
	}
	CATCHZOK
	return ok;
}

SLAPI PPConfigDatabase::CObjTailTbl::CObjTailTbl(BDbDatabase * pDb) : BDbTable(BDbTable::ConfigHash("ppconfig.db->cobjtail", 0, 0, 0), pDb)
{
}

SLAPI PPConfigDatabase::CObjTailTbl::~CObjTailTbl()
{
}

SLAPI PPConfigDatabase::StringHistoryTbl::StringHistoryTbl(BDbDatabase * pDb) : BDbTable(BDbTable::ConfigHash("ppconfig.db->stringhistory", 0, 0, 0), pDb)
{
}

SLAPI PPConfigDatabase::StringHistoryTbl::~StringHistoryTbl()
{
}

SLAPI  PPConfigDatabase::StringHistoryPool::StringHistoryPool(const char * pKey) : Key(pKey), Ht(8192)
{
}

const SString & SLAPI PPConfigDatabase::StringHistoryPool::GetKey() const
{
	return Key;
}

int SLAPI PPConfigDatabase::StringHistoryPool::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int   ok = 1;
	THROW_SL(pSCtx->Serialize(dir, Key, rBuf));
	THROW_SL(pSCtx->Serialize(dir, &L, rBuf));
	THROW_SL(SStrGroup::SerializeS(dir, rBuf, pSCtx));
	CATCHZOK
	return ok;
}

int SLAPI PPConfigDatabase::StringHistoryPool::SearchEntries(const char * pSubUtf8, long flags, LongArray & rPosList) const
{
	int    ok = 0;
	rPosList.clear();
	SStringU ci_text;
	SStringU temp_buf_u;
	ci_text.CopyFromUtf8(pSubUtf8, sstrlen(pSubUtf8));
	ci_text.ToLower();
	SString temp_buf;
	if(flags & sefFullString) {
		ci_text.CopyToUtf8(temp_buf, 1);
		uint   val = 0;
		if(Ht.Search(temp_buf, &val, 0)) {
			assert(val > 0 && val <= L.getCount());
			rPosList.add(static_cast<long>(val-1));
			ok = 1;
		}
	}
	else {
		HashTableBase::Iter hti;
		if(Ht.InitIteration(&hti)) {
			uint val = 0;
			while(Ht.NextIteration(&hti, &val, 0, &temp_buf)) {
				assert(val > 0 && val <= L.getCount());
				temp_buf_u.CopyFromUtf8(temp_buf);
				if(flags & sefFromBegin) {
					if(temp_buf_u.CmpPrefix(ci_text) == 0) {
						rPosList.add(static_cast<long>(val-1));
						ok = 1;
					}
				}
				else if(temp_buf_u.Search(ci_text, 0, 0)) {
					rPosList.add(static_cast<long>(val-1));
					ok = 1;
				}
			}
		}
	}
	return ok;
}

int SLAPI PPConfigDatabase::StringHistoryPool::Add(const char * pTextUtf8)
{
	int   ok = 0;
	LongArray pos_list;
	SString text_buf = pTextUtf8;
	text_buf.Strip();
	if(text_buf.Len()) {
		if(SearchEntries(text_buf, sefFullString, pos_list)) {
			assert(pos_list.getCount() == 1);
			if(pos_list.getCount()) {
				InnerEntry & r_entry = L.at(pos_list.get(0));
				r_entry.UsageCount++;
				r_entry.Dtm = getcurdatetime_();
				St.Dirty = 1;
			}
			ok = 2;
		}
		else {
			InnerEntry new_entry;
			new_entry.UsageCount = 1;
			new_entry.OrgUsageCount = 0;
			new_entry.Dtm = getcurdatetime_();
			SStringU ci_text;
			AddS(text_buf, &new_entry.TextP);
			ci_text.CopyFromUtf8(text_buf, text_buf.Len());
			ci_text.ToLower();
			ci_text.CopyToUtf8(text_buf, 1);
			Ht.Add(text_buf, L.getCount()+1, 0);
			//AddS(text_buf, &new_entry.TextCiP);
			L.insert(&new_entry);
			St.Dirty = 1;
			ok = 1;
		}
	}
	else
		ok = -1;
	return ok;
}

int SLAPI PPConfigDatabase::StringHistoryPool::BuildHash()
{
	Ht.Clear();
	int    ok = 1;
	SString temp_buf;
	SStringU temp_buf_u;
	for(uint i = 0; i < L.getCount(); i++) {
		const InnerEntry & r_this_entry = L.at(i);
		GetS(r_this_entry.TextP, temp_buf);
		temp_buf.Strip();
		temp_buf_u.CopyFromUtf8(temp_buf);
		temp_buf_u.ToLower();
		temp_buf_u.CopyToUtf8(temp_buf, 1);
		THROW_SL(Ht.Add(temp_buf, i+1));
	}
	CATCHZOK
	return ok;
}

int SLAPI PPConfigDatabase::StringHistoryPool::Merge(StringHistoryPool & rS)
{
	int    ok = 1;
	SString temp_buf;
	LongArray pos_list;
	Ht.BuildAssoc();
	for(uint i = 0; i < L.getCount(); i++) {
		const InnerEntry & r_this_entry = L.at(i);
		Ht.GetByAssoc(i+1, temp_buf);
		//GetS(r_this_entry.TextCiP, temp_buf);
		if(rS.SearchEntries(temp_buf, sefFullString, pos_list) > 0) {
			assert(pos_list.getCount());
			for(uint pidx = 0; pidx < pos_list.getCount(); pidx++) {
				const uint other_pos = static_cast<uint>(pos_list.get(pidx));
				InnerEntry & r_other_entry = rS.L.at(other_pos);
				if(cmp(r_this_entry.Dtm, r_other_entry.Dtm) > 0) {
					r_other_entry.Dtm = r_this_entry.Dtm;
					St.Dirty = 1;
				}
				if(r_this_entry.UsageCount > r_this_entry.OrgUsageCount) {
					r_other_entry.UsageCount += (r_this_entry.UsageCount - r_this_entry.OrgUsageCount);
					St.Dirty = 1;
				}
			}
		}
		else {
			InnerEntry new_entry;
			new_entry.Dtm = r_this_entry.Dtm;
			new_entry.UsageCount = r_this_entry.UsageCount;
			new_entry.OrgUsageCount = 0;
			rS.Ht.Add(temp_buf, rS.L.getCount()+1);
			//rS.AddS(temp_buf, &new_entry.TextCiP);
			GetS(r_this_entry.TextP, temp_buf);
			rS.AddS(temp_buf, &new_entry.TextP);
			rS.L.insert(&new_entry);
			St.Dirty = 1;
		}
	}
	return ok;
}

void SLAPI PPConfigDatabase::StringHistoryPool::OnSave()
{
	St.Dirty = 0;
	St.LastSaveTm = getcurdatetime_();
}

int SLAPI PPConfigDatabase::StringHistoryPool::IsSavingNeeded() const
{
	return St.Dirty;
}

int FASTCALL PPConfigDatabase::StringHistoryPool::CmpEntryIndices(const void * p1, const void * p2) const
{
	uint idx1 = *static_cast<const uint *>(p1);
	uint idx2 = *static_cast<const uint *>(p2);
	const InnerEntry & r_e1 = L.at(idx1);
	const InnerEntry & r_e2 = L.at(idx2);
	return cmp(r_e2.Dtm, r_e1.Dtm); // Сортировка в обратном порядке (большее время - наверх)
}

static IMPL_CMPFUNC(StringHistoryPool_EntryIndex, p1, p2)
{
	PPConfigDatabase::StringHistoryPool * p_pool = static_cast<PPConfigDatabase::StringHistoryPool *>(pExtraData);
	return p_pool ? p_pool->CmpEntryIndices(p1, p2) : 0;
}

int SLAPI PPConfigDatabase::StringHistoryPool::GetRecent(uint maxItems, StringSet & rList) const
{
	int    ok = 0;
	rList.clear();
	if(maxItems > 0 && L.getCount()) {
		LongArray pos_list;
		SString temp_buf;
		for(uint i = 0; i < L.getCount(); i++) {
			pos_list.add(static_cast<long>(i));
		}
		pos_list.SVectorBase::sort(PTR_CMPFUNC(StringHistoryPool_EntryIndex), const_cast<PPConfigDatabase::StringHistoryPool *>(this)); // @badcast
		for(uint i = 0; i < pos_list.getCount() && i < maxItems; i++) {
			const InnerEntry & r_entry = L.at(pos_list.get(i));
			GetS(r_entry.TextP, temp_buf);
			if(temp_buf.Len())
				rList.add(temp_buf);
		}
		ok = 1;
	}
	return ok;
}
		
int SLAPI PPConfigDatabase::StringHistoryPool::Get(const char * pSubUtf8, long flags, StringSet & rList) const
{
	int   ok = 0;
	rList.clear();
	LongArray pos_list;
	SString text_buf = pSubUtf8;
	text_buf.Strip();
	if(text_buf.Len()) {
		if(SearchEntries(text_buf, flags, pos_list)) {
			SString temp_buf;
			pos_list.SVectorBase::sort(PTR_CMPFUNC(StringHistoryPool_EntryIndex), const_cast<PPConfigDatabase::StringHistoryPool *>(this)); // @badcast
			for(uint i = 0; i < pos_list.getCount(); i++) {
				const InnerEntry & r_entry = L.at(pos_list.get(i));
				GetS(r_entry.TextP, temp_buf);
				if(temp_buf.Len())
					rList.add(temp_buf);
			}
			ok = 1;
		}
	}
	return ok;
}

SLAPI PPConfigDatabase::PPConfigDatabase(const char * pDbDir) : P_OT(0), P_OtT(0), P_ShT(0), P_Db(0)
{
	if(pDbDir)
		Open(pDbDir);
}

SLAPI PPConfigDatabase::~PPConfigDatabase()
{
	Close();
}

int SLAPI PPConfigDatabase::Open(const char * pDbPath)
{
	int    ok = 1;
	Close();
	if(!isempty(pDbPath) && pathValid(pDbPath, 1)) {
		BDbDatabase::Config cfg;
		cfg.CacheSize   = SMEGABYTE(8);
		cfg.MaxLockers  = SKILOBYTE(2);
		cfg.MaxLocks    = SKILOBYTE(2);
		cfg.MaxLockObjs = SKILOBYTE(2);
		cfg.LogBufSize  = SMEGABYTE(1);
		cfg.Flags |= (cfg.fLogNoSync|cfg.fLogAutoRemove);
		{
			long   db_options = BDbDatabase::oPrivate/*|BDbDatabase::oRecover*/;
			THROW_MEM(P_Db = new BDbDatabase(pDbPath, &cfg, db_options));
			THROW(!!*P_Db);
		}
		//
		THROW_MEM(P_OT = new CObjTbl(P_Db));
		THROW_MEM(P_OtT = new CObjTailTbl(P_Db));
		THROW_MEM(P_ShT = new StringHistoryTbl(P_Db)); // @v10.7.6
	}
	CATCH
		ok = 0;
		Close();
	ENDCATCH
	return ok;
}

void SLAPI PPConfigDatabase::Close()
{
	ZDELETE(P_OT);
	ZDELETE(P_OtT);
	ZDELETE(P_ShT); // @v10.7.6
	ZDELETE(P_Db);
}

int SLAPI PPConfigDatabase::GetObjList(int type, const char * pSubSymb, const char * pDbSymb, const char * pOwner, StrAssocArray & rList)
{
	int    ok = 1;
	BDbTable::Buffer key_buf, data_buf;
	SBuffer buf;
	CObjHeader rec;
	key_buf = (int32)type;
	data_buf.Alloc(256);
	BDbCursor curs(*P_OT, 2);
	if(curs.Search(key_buf, data_buf, spEq)) {
		do {
			int32  id = 0;
			buf.Z();
			key_buf.Get(buf);
			P_OT->SerializeKeyBuf(-1, &rec, buf);
			if(rec.Type == type) {
				if((!pDbSymb || rec.DbSymb.Cmp(pDbSymb, 1) == 0) && (!pOwner || rec.OwnerSymb.Cmp(pOwner, 1) == 0) && (!pSubSymb || rec.SubSymb.Cmp(pSubSymb, 1) == 0)) {
					data_buf.Get(&id);
					rList.Add(id, rec.Name);
				}
				key_buf = static_cast<int32>(type);
			}
			else
				break;
		} while(curs.Search(key_buf, data_buf, spNextDup));
	}
	else
		ok = -1;
	return ok;
}

int SLAPI PPConfigDatabase::DeleteObj(int32 id, int use_ta)
{
	int    ok = 1;
	BDbTable::Buffer key_buf, data_buf;
	BDbTransaction tra(P_Db, use_ta);
	THROW_DB(tra);
	if(id) {
		data_buf.Alloc(1024);
		THROW_PP_S(P_OT->Search(1, key_buf = id, data_buf), PPERR_CDBOBJNFOUND, id);
		THROW_DB(P_OT->DeleteRec(key_buf));
	}
	THROW_DB(tra.Commit(1));
	CATCHZOK
	return ok;
}

int SLAPI PPConfigDatabase::PutObj(int32 * pID, CObjHeader & rHdr, SBuffer & rData, int use_ta)
{
	int    ok = 1;
	BDbTable::Buffer key_buf, data_buf;
	SBuffer buf;
	BDbTransaction tra(P_Db, use_ta);
	THROW_DB(tra);
	if(*pID == 0) {
		P_OT->SerializeKeyBuf(+1, &rHdr, buf.Z());
		key_buf = buf;
		data_buf.Alloc(16);
		if(P_OT->Search(key_buf, data_buf)) {
			data_buf.Get(pID);
		}
	}
	if(*pID) {
		data_buf.Alloc(1024);
		if(P_OT->Search(1, key_buf = *pID, data_buf)) {
			CObjHeader rec;
			key_buf.Get(buf);
			THROW(P_OT->SerializeKeyBuf(-1, &rec, buf));
			if(rHdr.Cmp(rec, 0x0001) != 0) {
				THROW_DB(P_OT->DeleteRec(key_buf));
				{
					P_OT->SerializeKeyBuf(+1, &rHdr, buf.Z());
					key_buf = buf;
					data_buf = *pID;
					THROW_DB(P_OT->InsertRec(key_buf, data_buf));
				}
			}
		}
	}
	else {
		int64 _id = 0;
		THROW_DB(P_Db->GetSequence(P_OT->SeqID, &_id));
		*pID = (int32)_id;
		P_OT->SerializeKeyBuf(+1, &rHdr, buf.Z());
		key_buf = buf;
		data_buf = *pID;
		THROW_DB(P_OT->InsertRec(key_buf, data_buf));
	}
	THROW_DB(P_OtT->InsertRec(key_buf = *pID, data_buf = rData));
	THROW_DB(tra.Commit(1));
	CATCHZOK
	return ok;
}

int SLAPI PPConfigDatabase::GetObj(int32 id, CObjHeader * pHdr, SBuffer * pData)
{
	int    ok = -1;
	BDbTable::Buffer key_buf, data_buf;
	SBuffer buf;
	data_buf.Alloc(4096);
	if(P_OT->Search(1, key_buf = id, data_buf)) {
		ok = 1;
		if(pHdr) {
			key_buf.Get(buf);
			P_OT->SerializeKeyBuf(-1, pHdr, buf);
			pHdr->ID = id;
		}
		if(pData) {
			if(P_OtT->Search(key_buf = id, data_buf)) {
				data_buf.Get(*pData);
				ok = 2;
			}
			else
				ok = 0;
		}
	}
	return ok;
}

int SLAPI TestConfigDatabase_StringHistory_Interactive()
{
	int    ok = -1;
	const char * p_key = "test-string-history-key";
	TDialog * dlg = new TDialog(DLG_INPUT);
	if(CheckDialogPtr(&dlg)) {
		dlg->SetupWordSelector(CTL_INPUT_STR, new TextHistorySelExtra(p_key), 0, 3, WordSel_ExtraBlock::fFreeText);
		ExecView(dlg);
	}
	delete dlg;
	return ok;
}

int SLAPI TestConfigDatabase_StringHistory()
{
	int    ok = 1;
	SString temp_buf;
	SString inp_file_name;
	StringSet ss_result;
	const char * p_key = "test-string-history-key";
	PPGetPath(PPPATH_TESTROOT, inp_file_name);
	inp_file_name.SetLastSlash().Cat("data").SetLastSlash().Cat("email-list.txt");
	SFile f_inp(inp_file_name, SFile::mRead);
	if(f_inp.IsValid()) {
		//PPGetPath(PPPATH_WORKSPACE, temp_buf);
		//temp_buf.SetLastSlash().Cat("bdbconfig-test");
		//PPConfigDatabase cdb(temp_buf);
		SStrGroup ssg;
		LongArray ssg_pos_list;
		uint   line_no = 0;
		while(f_inp.ReadLine(temp_buf)) {
			line_no++;
			temp_buf.Chomp().Strip();
			temp_buf.ToUtf8();
			if(DS.AddStringHistory(p_key, temp_buf)) {
				uint sp = 0;
				ssg.AddS(temp_buf, &sp);
				ssg_pos_list.add(static_cast<long>(sp));
			}
		}
		ssg_pos_list.shuffle();
		temp_buf = "YAHOO.COM";
		if(DS.GetStringHistory(p_key, temp_buf, PPConfigDatabase::StringHistoryPool::sefSubString, ss_result) > 0) {
			for(uint j = 0; ss_result.get(&j, temp_buf);) {
				;
			}
		}
		for(uint i = 0; i < ssg_pos_list.getCount(); i++) {
			uint sp = static_cast<uint>(ssg_pos_list.get(i));
			if(ssg.GetS(sp, temp_buf)) {
				if(DS.GetStringHistory(p_key, temp_buf, PPConfigDatabase::StringHistoryPool::sefFullString, ss_result) > 0) {
					;
				}
			}
		}
		THROW(DS.SaveStringHistory());
	}
	/*{
		PPGetPath(PPPATH_WORKSPACE, temp_buf);
		temp_buf.SetLastSlash().Cat("bdbconfig-test");
		PPConfigDatabase cdb(temp_buf);
		temp_buf = "GmAil.com";
		if(cdb.GetStringHistory(p_key, temp_buf, PPConfigDatabase::StringHistoryPool::sefSubString, ss_result) > 0) {
			for(uint j = 0; ss_result.get(&j, temp_buf);) {
				;
			}
		}
	}*/
	CATCHZOK
	return ok;
}

#endif // } 0 @construction
