// SARTRE_DB.CPP
// Copyright (c) A.Sobolev 2017, 2018
// @codepage UTF-8
//
#include <pp.h>
#pragma hdrstop
#include <sartre.h>
#include <locale.h>
//#include <berkeleydb.h>
#include <berkeleydb-6232.h>
//
//
//
SrGrammarTbl::SrGrammarTbl(BDbDatabase * pDb) : BDbTable(BDbTable::ConfigHash("words.db->gramm", 0, SKILOBYTE(16), 0), pDb), SeqID(0)
{
	class Idx01 : public SecondaryIndex {
		virtual int Cb(const BDbTable::Buffer & rKey, const BDbTable::Buffer & rData, BDbTable::Buffer & rResult)
		{
			SBuffer temp_buf;
			rData.Get(temp_buf);
			rResult = temp_buf;
			return 0;
		}
	};
	new BDbTable(BDbTable::ConfigHash("words.db->gramm_idx01", 0, SKILOBYTE(16), 0), pDb, new Idx01, this);
	if(P_Db)
		THROW(P_Db->CreateSequence("seq_gramm_id", 0, &SeqID));
	CATCH
		Close();
	ENDCATCH
}

SrGrammarTbl::~SrGrammarTbl()
{
	CALLPTRMEMB(P_Db, CloseSequence(SeqID));
}

int SrGrammarTbl::Helper_Add(SrSList * pL, long * pID)
{
	int    ok = 1;
	BDbTable::Buffer key_buf;
	BDbTable::Buffer data_buf;
	int32  id = 0;
	int64  __id = 0;
	THROW_DB(P_Db->GetSequence(SeqID, &__id));
	id = (int32)__id;
	key_buf = id;
	{
		SBuffer l_buf;
		SSerializeContext * p_sctx = GetSCtx();
		THROW_DB(p_sctx);
		THROW(pL->Serialize(+1, l_buf, p_sctx));
		data_buf = l_buf;
	}
	THROW_DB(InsertRec(key_buf, data_buf));
	ASSIGN_PTR(pID, id);
	CATCHZOK
	return ok;
}

int SrGrammarTbl::Helper_Search(SrSList * pL, long * pID)
{
	int    ok = -1;
	BDbTable::Buffer key_buf;
	BDbTable::Buffer data_buf;
	{
		SBuffer l_buf;
		SSerializeContext * p_sctx = GetSCtx();
		THROW_DB(p_sctx);
		THROW(pL->Serialize(+1, l_buf, p_sctx));
		key_buf = l_buf;
	}
	data_buf.Alloc(1024);
	if(BDbTable::Search(1, key_buf, data_buf)) {
		key_buf.Get(pID);
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int SrGrammarTbl::Helper_Search(long id, SrSList * pL)
{
	int    ok = -1;
	BDbTable::Buffer key_buf;
	BDbTable::Buffer data_buf;
	key_buf = id;
	data_buf.Alloc(1024);
	if(BDbTable::Search(key_buf, data_buf)) {
		if(pL) {
			SBuffer wf_buf;
			SSerializeContext * p_sctx = GetSCtx();
			THROW_DB(p_sctx);
			data_buf.Get(wf_buf);
			THROW(pL->Serialize(-1, wf_buf, p_sctx));
		}
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int SrGrammarTbl::Add(SrWordForm * pWf, long * pID) { return Helper_Add(pWf, pID); }
int SrGrammarTbl::Add(SrFlexiaModel * pFm, long * pID) { return Helper_Add(pFm, pID); }

int SrGrammarTbl::Search(long id, SrWordForm * pWf)
{
	int    ok = Helper_Search(id, pWf);
	if(ok > 0) {
		assert(pWf->GetType() == SRGRAMTYP_WORDFORM);
	}
	return ok;
}

int SrGrammarTbl::Search(long id, SrFlexiaModel * pFm)
{
	int    ok = Helper_Search(id, pFm);
	if(ok > 0) {
		assert(pFm->GetType() == SRGRAMTYP_FLEXIAMODEL);
	}
	return ok;
}

int SrGrammarTbl::Search(SrWordForm * pWf, long * pID)
{
	int    ok = Helper_Search(pWf, pID);
	if(ok > 0) {
		assert(pWf->GetType() == SRGRAMTYP_WORDFORM);
	}
	return ok;
}

int SrGrammarTbl::Search(SrFlexiaModel * pFm, long * pID)
{
	int    ok = Helper_Search(pFm, pID);
	if(ok > 0) {
		assert(pFm->GetType() == SRGRAMTYP_FLEXIAMODEL);
	}
	return ok;
}
//
// {id;word-id;grammar-rule-id}
//
/*class SrWordGrammarTbl : public BDbTable {
public:
	SrWordGrammarTbl(BDbDatabase * pDb);
};*/

SrWordTbl::SrWordTbl(BDbDatabase * pDb) : BDbTable(BDbTable::ConfigHash("words.db->word", 0, SKILOBYTE(16), 0), pDb), SeqID(0)
{
	class Idx01 : public SecondaryIndex {
		virtual int Cb(const BDbTable::Buffer & rKey, const BDbTable::Buffer & rData, BDbTable::Buffer & rResult)
		{
			LEXID  id = 0;
			rData.Get(&id, sizeof(id));
			rResult.Set(&id, sizeof(id));
			return 0;
		}
	};
	new BDbTable(BDbTable::ConfigHash("words.db->word_idx01", 0, SKILOBYTE(16), 0), pDb, new Idx01, this);
	if(P_Db)
		THROW(P_Db->CreateSequence("seq_word_id", 0, &SeqID));
	CATCH
		Close();
	ENDCATCH
}

SrWordTbl::~SrWordTbl()
{
	CALLPTRMEMB(P_Db, CloseSequence(SeqID));
}

int SrWordTbl::Add(const char * pWordUtf8, LEXID * pID)
{
	int    ok = 1;
	LEXID  id = 0;
	BDbTable::Buffer key_buf, data_buf;
	key_buf = pWordUtf8;
	data_buf.Alloc(32);
	if(BDbTable::Search(key_buf, data_buf)) {
		data_buf.Get(&id, sizeof(id));
		ok = -1;
	}
	else {
		int64 __id = 0;
		THROW_DB(P_Db->GetSequence(SeqID, &__id));
		id = (LEXID)__id;
		data_buf.Set(&id, sizeof(id));
		THROW_DB(InsertRec(key_buf, data_buf));
	}
	CATCH
		id = 0;
		ok = 0;
	ENDCATCH
	ASSIGN_PTR(pID, id);
	return ok;
}

int SrWordTbl::AddSpecial(int spcTag, const char * pWordUtf8, LEXID * pID)
{
	int    ok = 0;
	SString temp_buf;
	if(SrDatabase::MakeSpecialWord(spcTag, pWordUtf8, temp_buf))
		ok = Add(temp_buf, pID);
	return ok;
}

int SrWordTbl::Search(const char * pWordUtf8, LEXID * pID)
{
	int    ok = -1;
	LEXID  id = 0;
	BDbTable::Buffer key_buf, data_buf;
	key_buf = pWordUtf8;
	data_buf.Alloc(32);
	if(BDbTable::Search(key_buf, data_buf)) {
		data_buf.Get(&id, sizeof(id));
		ok = 1;
	}
	else {
		SString msg_buf;
		PPSetError(PPERR_SR_WORDNFOUND, (msg_buf = pWordUtf8).Transf(CTRANSF_UTF8_TO_INNER));
	}
	ASSIGN_PTR(pID, id);
	return ok;
}

int SrWordTbl::Search(LEXID id, SString & rBuf)
{
	int    ok = -1;
	rBuf.Z();
	BDbTable::Buffer key_buf, data_buf;
	key_buf.Set(&id, sizeof(id));
	data_buf.Alloc(32);
	if(BDbTable::Search(1, key_buf, data_buf)) {
		key_buf.Get(rBuf);
		ok = 1;
	}
	else
		PPSetErrorDB();
	return ok;
}
//
//
//
SrWordAssocTbl::SrWordAssocTbl(BDbDatabase * pDb) : BDbTable(BDbTable::ConfigHash("words.db->wordassoc", 0, SKILOBYTE(16), 0), pDb), SeqID(0)
{
	//
	// Индекс по идентификатору лексемы. Неуникальный.
	//
	class Idx01 : public SecondaryIndex {
	public:
		virtual int Cb(const BDbTable::Buffer & rKey, const BDbTable::Buffer & rData, BDbTable::Buffer & rResult)
		{
			SBuffer buf;
			rData.Get(buf);
			SrWordAssoc rec;
			((SrWordAssocTbl *)P_MainT)->SerializeRecBuf(-1, &rec, buf);
			rResult = rec.WordID;
			return 0;
		}
	};
	//
	// Индекс по полной записи (без ее идентификатора). Уникальный.
	//
	class Idx02 : public SecondaryIndex {
		virtual int Cb(const BDbTable::Buffer & rKey, const BDbTable::Buffer & rData, BDbTable::Buffer & rResult)
		{
			SBuffer buf;
			rData.Get(buf);
			rResult = buf;
			return 0;
		}
	};
	THROW_SL(new BDbTable(BDbTable::ConfigHash("words.db->wordassoc_idx01", cfDup, SKILOBYTE(16), 0), pDb, new Idx01, this));
	THROW_SL(new BDbTable(BDbTable::ConfigHash("words.db->wordassoc_idx02", 0, SKILOBYTE(16), 0), pDb, new Idx02, this));
	if(P_Db)
		THROW_DB(P_Db->CreateSequence("seq_wordassoc_id", 0, &SeqID));
	CATCH
		Close();
	ENDCATCH
}

SrWordAssocTbl::~SrWordAssocTbl()
{
	CALLPTRMEMB(P_Db, CloseSequence(SeqID));
}

int SrWordAssocTbl::Add(SrWordAssoc * pWa, int32 * pID)
{
	int    ok = 1;
	LEXID  id = 0;
	BDbTable::Buffer key_buf;
	BDbTable::Buffer data_buf;
	SBuffer buf;
	THROW(SerializeRecBuf(+1, pWa, buf));
	key_buf = buf;
	data_buf.Alloc(128);
	if(BDbTable::Search(2, key_buf, data_buf)) {
		key_buf.Get(&id);
		ok = -1;
	}
	else {
		int64 __id = 0;
		THROW_DB(P_Db->GetSequence(SeqID, &__id));
		id = (LEXID)__id;
		key_buf = id;
		data_buf = buf;
		THROW_DB(InsertRec(key_buf, data_buf));
	}
	pWa->ID = id;
	CATCH
		id = 0;
		ok = 0;
	ENDCATCH
	ASSIGN_PTR(pID, id);
	return ok;
}

int SrWordAssocTbl::Update(SrWordAssoc & rRec)
{
	int    ok = 1;
	SrWordAssoc org_rec;
	THROW(Search(rRec.ID, &org_rec) > 0);
	if(org_rec.IsEqual(rRec))
		ok = -1;
	else {
		assert(rRec.ID == org_rec.ID);
		BDbTable::Buffer key_buf, data_buf;
		SBuffer buf;
		THROW(SerializeRecBuf(+1, &rRec, buf));
		key_buf = rRec.ID;
		data_buf = buf;
		THROW_DB(UpdateRec(key_buf, data_buf));
	}
	CATCHZOK
	return ok;
}

int SrWordAssocTbl::Search(int32 id, SrWordAssoc * pWa)
{
	int    ok = -1;
	BDbTable::Buffer key_buf, data_buf;
	SBuffer buf;
	key_buf = id;
	data_buf.Alloc(128);
	if(BDbTable::Search(key_buf, data_buf)) {
		if(pWa) {
			buf.Clear();
			data_buf.Get(buf);
			SerializeRecBuf(-1, pWa, buf);
			key_buf.Get(&pWa->ID);
		}
		ok = 1;
	}
	return ok;
}

int SrWordAssocTbl::Search(LEXID wordID, TSVector <SrWordAssoc> & rList) // @v9.8.4 TSArray-->TSVector
{
	int    ok = 1;
	BDbTable::Buffer key_buf, data_buf;
	SBuffer buf;
	key_buf = wordID;
	data_buf.Alloc(128);
	rList.clear();
	BDbCursor curs(*this, 1);
	if(curs.Search(key_buf, data_buf, spEq)) {
		do {
			int32  id = 0;
			SrWordAssoc wa;
			buf.Clear();
			data_buf.Get(buf);
			SerializeRecBuf(-1, &wa, buf);
			if(wa.WordID == wordID) {
				key_buf.Get(&id);
				wa.ID = id;
				rList.insert(&wa);
				key_buf = wordID;
			}
			else
				break;
		} while(curs.Search(key_buf, data_buf, spNext));
	}
	else
		ok = -1;
	// CATCHZOK
	return ok;
}

int SrWordAssocTbl::SerializeRecBuf(int dir, SrWordAssoc * pWa, SBuffer & rBuf)
{
	int    ok = 1;
	SSerializeContext * p_sctx = GetSCtx();
	THROW_DB(p_sctx);
	THROW_SL(p_sctx->Serialize(dir, pWa->Flags, rBuf));
	THROW_SL(p_sctx->Serialize(dir, pWa->WordID, rBuf));
	THROW_SL(p_sctx->Serialize(dir, pWa->BaseFormID, rBuf));
	{
		const long f = pWa->Flags;
		if(f & SrWordAssoc::fHasFlexiaModel) { THROW_SL(p_sctx->Serialize(dir, pWa->FlexiaModelID, rBuf)); } else if(dir < 0) pWa->FlexiaModelID = 0;
		if(f & SrWordAssoc::fHasAccentModel) { THROW_SL(p_sctx->Serialize(dir, pWa->AccentModelID, rBuf)); } else if(dir < 0) pWa->AccentModelID = 0;
		if(f & SrWordAssoc::fHasPrefix)      { THROW_SL(p_sctx->Serialize(dir, pWa->PrefixID,      rBuf)); } else if(dir < 0) pWa->PrefixID = 0;
		if(f & SrWordAssoc::fHasAffixModel)  { THROW_SL(p_sctx->Serialize(dir, pWa->AffixModelID,  rBuf)); } else if(dir < 0) pWa->AffixModelID = 0;
		if(f & SrWordAssoc::fHasAbbrExp)     { THROW_SL(p_sctx->Serialize(dir, pWa->AbbrExpID,     rBuf)); } else if(dir < 0) pWa->AbbrExpID = 0;
	}
	CATCHZOK
	return ok;
}
//
//
//
SrNGramTbl::SrNGramTbl(BDbDatabase * pDb) : BDbTable(BDbTable::ConfigHash("words.db->ngram", 0, SKILOBYTE(16), 0), pDb), SeqID(0)
{
	class Idx01 : public SecondaryIndex {
		virtual int Cb(const BDbTable::Buffer & rKey, const BDbTable::Buffer & rData, BDbTable::Buffer & rResult)
		{
			SrNGram ng_rec;
			SBuffer rec_buf;
			rData.Get(rec_buf);
			((SrNGramTbl *)P_MainT)->SerializeRecBuf(-1, &ng_rec, rec_buf);
			const LongArray & r_list = ng_rec.WordIdList;
			rResult.Set(r_list.dataPtr(), r_list.getCount() * r_list.getItemSize());
			return 0;
		}
	};
	new BDbTable(BDbTable::ConfigBTree("words.db->ngram_idx01", 0, SKILOBYTE(16), 0), pDb, new Idx01, this);
	if(P_Db)
		THROW(P_Db->CreateSequence("seq_ngram_id", 0, &SeqID));
	CATCH
		Close();
	ENDCATCH
}

SrNGramTbl::~SrNGramTbl()
{
	CALLPTRMEMB(P_Db, CloseSequence(SeqID));
}

int SrNGramTbl::SerializeRecBuf(int dir, SrNGram * pRec, SBuffer & rBuf)
{
	int    ok = 1;
	uint32 _c = 0;
	SSerializeContext * p_sctx = GetSCtx();
	THROW_DB(p_sctx);
	THROW_SL(p_sctx->Serialize(dir, pRec->Ver, rBuf));
	THROW_SL(p_sctx->Serialize(dir, &pRec->WordIdList, rBuf));
	CATCHZOK
	return ok;
}

int SrNGramTbl::Add(SrNGram & rRec)
{
	int    ok = 1;
	SBuffer rec_buf;
	BDbTable::Buffer key_buf, data_buf;
	int64 __id = 0;
	THROW_DB(P_Db->GetSequence(SeqID, &__id));
	rRec.ID = __id;
	// @v9.9.2 key_buf = __id;
	// @v9.9.2 {
	{
		uint8  _key[16];
		size_t _key_sz = sshrinkuint64(__id, _key);
		key_buf.Set(_key, _key_sz);
	}
	// } @v9.9.2
	THROW(SerializeRecBuf(+1, &rRec, rec_buf));
	data_buf = rec_buf;
	THROW_DB(InsertRec(key_buf, data_buf));
	CATCH
		rRec.ID = 0;
		ok = 0;
	ENDCATCH
	return ok;
}

int SrNGramTbl::Search(NGID id, SrNGram * pRec)
{
	int    ok = -1;
	SBuffer rec_buf;
	BDbTable::Buffer key_buf, data_buf;
	{
		uint8  _key[16];
		size_t _key_sz = sshrinkuint64(id, _key);
		key_buf.Set(_key, _key_sz);
	}
	data_buf.Alloc(512);
	if(BDbTable::Search(key_buf, data_buf)) {
		if(pRec) {
			data_buf.Get(rec_buf);
			THROW(SerializeRecBuf(-1, pRec, rec_buf));
			// @v9.9.9 @paranoic {
			{
				NGID   temp_id = 0;
				{
					size_t dbsz = 0;
					const void * p_dbptr = key_buf.GetPtr(&dbsz);
					temp_id = sexpanduint64(p_dbptr, dbsz);
				}
				assert(temp_id == id);
			}
			// } @v9.9.9 @paranoic 
			pRec->ID = id; // @v9.7.11 @fix
		}
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int SrNGramTbl::Search(const SrNGram & rKey, NGID * pID)
{
	int    ok = -1;
	SBuffer rec_buf;
	BDbTable::Buffer key_buf, data_buf;
	{
		const LongArray & r_list = rKey.WordIdList;
		key_buf.Set(r_list.dataPtr(), r_list.getCount() * r_list.getItemSize());
	}
	data_buf.Alloc(512);
	if(BDbTable::Search(1, key_buf, data_buf)) {
		if(pID) {
			size_t dbsz = 0;
			const void * p_dbptr = key_buf.GetPtr(&dbsz);
			*pID = sexpanduint64(p_dbptr, dbsz);
		}
		ok = 1;
	}
	return ok;
}

int SrNGramTbl::SearchByPrefix(const SrNGram & rKey, TSVector <NGID> & rList) // @v9.8.4 TSArray-->TSVect
{
	int    ok = -1;
	const  LongArray & r_key_list = rKey.WordIdList;
	const  uint key_count = r_key_list.getCount();
	if(key_count) {
		SrNGram rec;
		SBuffer rec_buf;
		BDbCursor curs(*this, 1);
		BDbTable::Buffer key_buf, data_buf;
		key_buf.Set(r_key_list.dataPtr(), key_count * r_key_list.getItemSize());
		if(curs.Search(key_buf, data_buf, spGe)) {
			do {
				data_buf.Get(rec_buf);
				THROW(SerializeRecBuf(-1, &rec, rec_buf)); // @v9.7.11 @fix +1-->-1
				if(rec.WordIdList.getCount() >= key_count) {
					int    eq_prefix = 1;
					for(uint i = 0; eq_prefix && i < key_count; i++) {
						if(rec.WordIdList.get(i) != r_key_list.get(i))
							eq_prefix = 0;
					}
					if(eq_prefix) {
						NGID id = 0;
						{
							size_t dbsz = 0;
							const void * p_dbptr = key_buf.GetPtr(&dbsz);
							id = sexpanduint64(p_dbptr, dbsz);
						}
						rList.insert(&id);
						ok = 1;
					}
					else
						break;
				}
				else
					break;
			} while(curs.Search(key_buf, data_buf, spNext));
		}
	}
	CATCHZOK
	return ok;
}
//
//
//
SrConceptTbl::SrConceptTbl(BDbDatabase * pDb) : BDbTable(BDbTable::ConfigHash("concept.db->concept", 0, SKILOBYTE(16), 0), pDb), SeqID(0)
{
	class Idx01 : public SecondaryIndex {
		virtual int Cb(const BDbTable::Buffer & rKey, const BDbTable::Buffer & rData, BDbTable::Buffer & rResult)
		{
			SrConcept rec;
			SBuffer rec_buf;
			rData.Get(rec_buf);
			((SrConceptTbl *)P_MainT)->SerializeRecBuf(-1, &rec, rec_buf);
			rResult = rec.SymbID;
			//
			// Нулевой сидентификатор символа (аноминая концепция) не индексируем
			//
			return (rec.SymbID == 0) ? DB_DONOTINDEX : 0;
		}
	};
	new BDbTable(BDbTable::ConfigHash("concept.db->concept_idx01", 0, SKILOBYTE(16), 0), pDb, new Idx01, this);
	if(P_Db)
		THROW(P_Db->CreateSequence("seq_concept_id", 0, &SeqID));
	CATCH
		Close();
	ENDCATCH
}

SrConceptTbl::~SrConceptTbl()
{
	CALLPTRMEMB(P_Db, CloseSequence(SeqID));
}

int SrConceptTbl::SerializeRecBuf(int dir, SrConcept * pRec, SBuffer & rBuf)
{
	int    ok = 1;
	uint32 _c = 0;
	SSerializeContext * p_sctx = GetSCtx();
	THROW_DB(p_sctx);
	THROW_SL(p_sctx->Serialize(dir, pRec->SymbID, rBuf));
	THROW_SL(p_sctx->Serialize(dir, pRec->Ver, rBuf));
	THROW(pRec->Pdl.Serialize(dir, rBuf, p_sctx));
	CATCHZOK
	return ok;
}

int SrConceptTbl::SearchByID(CONCEPTID id, SrConcept * pRec)
{
	int    ok = -1;
	SBuffer rec_buf;
	BDbTable::Buffer key_buf, data_buf;
	{
		uint8  _key[16];
		size_t _key_sz = sshrinkuint64(id, _key);
		key_buf.Set(_key, _key_sz);
	}
	data_buf.Alloc(512);
	if(BDbTable::Search(key_buf, data_buf)) {
		if(pRec) {
			data_buf.Get(rec_buf);
			THROW(SerializeRecBuf(-1, pRec, rec_buf));
			{
				size_t dbsz = 0;
				const void * p_dbptr = key_buf.GetPtr(&dbsz);
				pRec->ID = sexpanduint64(p_dbptr, dbsz);
			}
			assert(pRec->ID == id);
		}
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int SrConceptTbl::SearchBySymb(LEXID symbId, SrConcept * pRec)
{
	int    ok = -1;
	SBuffer rec_buf;
	BDbTable::Buffer key_buf, data_buf;
	key_buf = symbId;
	data_buf.Alloc(512);
	if(BDbTable::Search(1, key_buf, data_buf)) {
		if(pRec) {
			data_buf.Get(rec_buf);
			THROW(SerializeRecBuf(-1, pRec, rec_buf));
			{
				size_t dbsz = 0;
				const void * p_dbptr = key_buf.GetPtr(&dbsz);
				pRec->ID = sexpanduint64(p_dbptr, dbsz);
			}
		}
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int SrConceptTbl::Add(SrConcept & rRec)
{
	int    ok = 1;
	SBuffer rec_buf;
	BDbTable::Buffer key_buf, data_buf;
	int64 __id = 0;
	THROW_DB(P_Db->GetSequence(SeqID, &__id));
	rRec.ID = __id;
	{
		uint8  _key[16];
		size_t _key_sz = sshrinkuint64(__id, _key);
		key_buf.Set(_key, _key_sz);
	}
	THROW(SerializeRecBuf(+1, &rRec, rec_buf));
	data_buf = rec_buf;
	THROW_DB(InsertRec(key_buf, data_buf));
	CATCH
		rRec.ID = 0;
		ok = 0;
	ENDCATCH
	return ok;
}

int SrConceptTbl::Update(SrConcept & rRec)
{
	int    ok = 1;
	SrConcept org_rec;
	SBuffer rec_buf;
	BDbTable::Buffer key_buf, data_buf;
	{
		uint8  _key[16];
		size_t _key_sz = sshrinkuint64(rRec.ID, _key);
		key_buf.Set(_key, _key_sz);
	}
	data_buf.Alloc(512);
	THROW_DB(BDbTable::Search(key_buf, data_buf));
	data_buf.Get(rec_buf);
	THROW(SerializeRecBuf(-1, &org_rec, rec_buf));
	org_rec.ID = rRec.ID;
	if(rRec.IsEqual(org_rec))
		ok = -1;
	else {
		rec_buf.Clear();
		THROW(SerializeRecBuf(+1, &rRec, rec_buf));
		data_buf = rec_buf;
		THROW_DB(UpdateRec(key_buf, data_buf));
	}
	CATCHZOK
	return ok;
}

int SrConceptTbl::Remove(CONCEPTID id)
{
	int    ok = 1;
	BDbTable::Buffer key_buf;
	{
		uint8  _key[16];
		size_t _key_sz = sshrinkuint64(id, _key);
		key_buf.Set(_key, _key_sz);
	}
	THROW(DeleteRec(key_buf));
	CATCHZOK
	return ok;
}

int SrConceptTbl::SetPropDeclList(CONCEPTID id, SrCPropDeclList * pPdl)
{
	int    ok = 1;
	if(pPdl && pPdl->GetCount()) {
		SrConcept rec;
		THROW(SearchByID(id, &rec) > 0);
		rec.Pdl.Merge(*pPdl);
		THROW(Update(rec));
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

//static
void FASTCALL SrConceptPropTbl::EncodePrimeKey(BDbTable::Buffer & rKeyBuf, const SrCProp & rRec)
	{ rKeyBuf.Set(&rRec.CID, sizeof(rRec.CID)+sizeof(rRec.PropID)); }
//static
void FASTCALL SrConceptPropTbl::DecodePrimeKey(const BDbTable::Buffer & rKeyBuf, SrCProp & rRec)
	{ rKeyBuf.Get(&rRec.CID, sizeof(rRec.CID)+sizeof(rRec.PropID)); }

SrConceptPropTbl::SrConceptPropTbl(SrDatabase & rSr) : BDbTable(BDbTable::ConfigHash("concept.db->conceptprop", 0, SKILOBYTE(16), 0), rSr.P_Db), R_Sr(rSr)
{
	class Idx01 : public SecondaryIndex {
		virtual int Cb(const BDbTable::Buffer & rKey, const BDbTable::Buffer & rData, BDbTable::Buffer & rResult)
		{
			SrCProp rec;
			SrConceptPropTbl::DecodePrimeKey(rKey, rec);
			rResult = rec.CID;
			return 0;
		}
	};
	new BDbTable(BDbTable::ConfigHash("concept.db->conceptprop_idx01", BDbTable::cfDup, SKILOBYTE(16), 0), rSr.P_Db, new Idx01, this);
}

int SrConceptPropTbl::Search(SrCProp & rRec)
{
	int    ok = 1;
	BDbTable::Buffer key_buf, data_buf;
	EncodePrimeKey(key_buf, rRec);
	data_buf.Alloc(512);
	rRec.Value.Clear();
	if(BDbTable::Search(key_buf, data_buf)) {
		SBuffer rec_buf;
		data_buf.Get(rec_buf);
		THROW(SerializeRecBuf(-1, &rRec, rec_buf));
		DecodePrimeKey(key_buf, rRec);
		ok = 1;
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

int SrConceptPropTbl::GetPropIdList(CONCEPTID cID, Int64Array & rPropIdList)
{
	rPropIdList.clear();

	int    ok = -1;
	SrCProp prop;
	BDbTable::Buffer key_buf, data_buf;
	key_buf = cID;
	data_buf.Alloc(512);
	BDbCursor curs(*this, 1);
	if(curs.Search(key_buf, data_buf, spEq)) {
		do {
			DecodePrimeKey(key_buf, prop);
			THROW(rPropIdList.add(prop.PropID));
			ok = 1;
		} while(curs.Search(key_buf, data_buf, spNext));
	}
	CATCHZOK
	return ok;
}

int SrConceptPropTbl::GetPropList(CONCEPTID cID, SrCPropList & rPropList)
{
	rPropList.Z();

	int    ok = -1;
	SrCProp prop;
	BDbTable::Buffer key_buf, data_buf;
	SBuffer rec_buf;
	key_buf = cID;
	data_buf.Alloc(512);
	BDbCursor curs(*this, 1);
	if(curs.Search(key_buf, data_buf, spEq)) {
		do {
			DecodePrimeKey(key_buf, prop.Z());
			if(prop.CID == cID) {
				data_buf.Get(rec_buf.Clear());
				SerializeRecBuf(-1, &prop, rec_buf);
				rPropList.Set(prop.CID, prop.PropID, prop.Value.GetBuf(prop.Value.GetRdOffs()), prop.Value.GetAvailableSize());
				ok = 1;
			}
			else
				break;
		} while(curs.Search(key_buf, data_buf, spNext));
	}
	return ok;
}

int SrConceptPropTbl::Set(SrCProp & rProp)
{
	int    ok = 1;
	BDbTable::Buffer key_buf, data_buf;
	SBuffer rec_buf;
	SrCProp org_rec;
	THROW_PP_S(rProp.CID, PPERR_INVPARAM_EXT, "SrConceptPropTbl::Set CID");
	THROW_PP_S(rProp.PropID, PPERR_INVPARAM_EXT, "SrConceptPropTbl::Set PropID");
	EncodePrimeKey(key_buf, rProp);
	data_buf.Alloc(512);
	if(BDbTable::Search(key_buf, data_buf)) {
		data_buf.Get(rec_buf);
		org_rec.CID = rProp.CID;
		org_rec.PropID = rProp.PropID;
		THROW(SerializeRecBuf(-1, &org_rec, rec_buf));
		org_rec.CID = rProp.CID;
		org_rec.PropID = rProp.PropID;
		if(!org_rec.IsEqual(rProp)) {
			THROW(SerializeRecBuf(+1, &rProp, rec_buf.Clear()));
			THROW_DB(UpdateRec(key_buf, data_buf = rec_buf));
		}
		else
			ok = -1;
	}
	else {
		EncodePrimeKey(key_buf, rProp);
		THROW(SerializeRecBuf(+1, &rProp, rec_buf.Clear()));
		THROW_DB(InsertRec(key_buf, data_buf = rec_buf));
	}
	CATCHZOK
	return ok;
}

int SrConceptPropTbl::SerializeRecBuf(int dir, SrCProp * pRec, SBuffer & rBuf)
{
	int    ok = 1;
	uint32 _c = 0;
	int    type = 0;
	SSerializeContext * p_sctx = GetSCtx();
	THROW_DB(p_sctx);
	//THROW(p_sctx->Serialize(dir, pRec->CID, rBuf));
	//THROW(p_sctx->Serialize(dir, pRec->PropID, rBuf));
	type = R_Sr.GetPropType(pRec->PropID);
	switch(type) {
		case SRPROPT_INT:
			{
				int64 val;
				if(dir < 0) {
					THROW_SL(p_sctx->Serialize(dir, val, rBuf));
					pRec->Value.Write(val);
				}
				else if(dir > 0) {
					pRec->Value.Read(val);
					THROW_SL(p_sctx->Serialize(dir, val, rBuf));
				}
			}
			break;
		case SRPROPT_STRING:
			{
				SString val;
				if(dir < 0) {
					THROW_SL(p_sctx->Serialize(dir, val, rBuf));
					pRec->Value.Write(val);
				}
				else if(dir > 0) {
					pRec->Value.Read(val);
					THROW_SL(p_sctx->Serialize(dir, val, rBuf));
				}
			}
			break;
		case SRPROPT_REAL:
			{
				double val;
				if(dir < 0) {
					THROW_SL(p_sctx->Serialize(dir, val, rBuf));
					pRec->Value.Write(val);
				}
				else if(dir > 0) {
					pRec->Value.Read(val);
					THROW_SL(p_sctx->Serialize(dir, val, rBuf));
				}
			}
			break;
		default:
			if(dir < 0) {
				pRec->Value = rBuf;
			}
			else if(dir > 0) {
				rBuf = pRec->Value;
			}
			break;
	}
	CATCHZOK
	return ok;
}
//
//
//
struct SrConceptNg {
	CONCEPTID CID;
	NGID   NGID;
};

SrConceptNgTbl::SrConceptNgTbl(BDbDatabase * pDb) : BDbTable(BDbTable::ConfigHash("concept.db->conceptng", 0, SKILOBYTE(16), 0), pDb)
{
	class Idx01 : public SecondaryIndex {
		virtual int Cb(const BDbTable::Buffer & rKey, const BDbTable::Buffer & rData, BDbTable::Buffer & rResult)
		{
			SrConceptNg rec;
			rKey.Get(&rec, sizeof(rec));
			rResult = rec.CID;
			return 0;
		}
	};
	class Idx02 : public SecondaryIndex {
		virtual int Cb(const BDbTable::Buffer & rKey, const BDbTable::Buffer & rData, BDbTable::Buffer & rResult)
		{
			SrConceptNg rec;
			rKey.Get(&rec, sizeof(rec));
			rResult = rec.NGID;
			return 0;
		}
	};
	new BDbTable(BDbTable::ConfigHash("concept.db->conceptng_idx01", BDbTable::cfDup, SKILOBYTE(16), 0), pDb, new Idx01, this);
	new BDbTable(BDbTable::ConfigHash("concept.db->conceptng_idx02", BDbTable::cfDup, SKILOBYTE(16), 0), pDb, new Idx02, this);
}

int SrConceptNgTbl::Set(CONCEPTID cID, NGID ngID)
{
	int    ok = 1;
	BDbTable::Buffer key_buf, data_buf;
	SrConceptNg rec;
	rec.CID = cID;
	rec.NGID = ngID;
	key_buf.Set(&rec, sizeof(rec));
	data_buf.Alloc(32);
	if(BDbTable::Search(key_buf, data_buf)) {
		ok = -1;
	}
	else {
		key_buf.Set(&rec, sizeof(rec));
		data_buf.Reset();
		THROW_DB(InsertRec(key_buf, data_buf));
	}
	CATCHZOK
	return ok;
}

int SrConceptNgTbl::GetConceptList(NGID ngID, Int64Array & rConceptList)
{
	int    ok = -1;
	rConceptList.clear();
	BDbCursor curs(*this, 2);
	BDbTable::Buffer key_buf, data_buf;
	key_buf.Alloc(32);
	data_buf.Alloc(32);
	key_buf = ngID;
	if(curs.Search(key_buf, data_buf, spEq)) do {
		SrConceptNg rec;
		key_buf.Get(&rec, sizeof(rec));
		if(rec.NGID == ngID) {
			rConceptList.add(rec.CID);
			ok = 1;
		}
		else
			break;
	} while(curs.Search(key_buf, data_buf, spNext));
	return ok;
}

int SrConceptNgTbl::GetNgList(CONCEPTID cID, Int64Array & rNgList)
{
	int    ok = -1;
	rNgList.clear();
	BDbCursor curs(*this, 1);
	BDbTable::Buffer key_buf;
	BDbTable::Buffer data_buf;
	key_buf.Alloc(32);
	data_buf.Alloc(32);
	key_buf = cID;
	if(curs.Search(key_buf, data_buf, spEq)) do {
		SrConceptNg rec;
		key_buf.Get(&rec, sizeof(rec));
		if(rec.CID == cID) {
			rNgList.add(rec.NGID);
			ok = 1;
		}
		else
			break;
	} while(curs.Search(key_buf, data_buf, spNext));
	return ok;
}

//virtual
uint FASTCALL SrGeoNodeTbl::Implement_PartitionFunc(DBT * pKey)
{
	uint64 node_id = sexpanduint64(pKey->data, pKey->size);
	return (uint)(node_id % Cfg.PartitionCount);
}
//
// avg-key-size = 4.094674922
// avg-rec-size = 42.18095422
// page-size = 2048
// ffactor = (2048 - 32) / (4.1 + 42.2 + 8) = 37.12
//
SrGeoNodeTbl::SrGeoNodeTbl(BDbDatabase * pDb) : BDbTable(BDbTable::ConfigHash("geomap.db->node", 0, SKILOBYTE(16), 0), pDb)
{
	/*
	class Idx01 : public SecondaryIndex {
		virtual int Cb(const BDbTable::Buffer & rKey, const BDbTable::Buffer & rData, BDbTable::Buffer & rResult)
		{
			size_t rec_size = 0;
			size_t key_size = 0;
			PPOsm::Tile tile;
			//PPOsm::NodeCluster clu;
			const void * p_key = rKey.GetPtr(&key_size);
			const void * p_data = rData.GetPtr(&rec_size);
			uint64 outer_id = 0;
			if(p_key)
				if(key_size == 4)
					outer_id = *(uint32 *)p_key;
				else if(key_size == 8)
					outer_id = *(uint64 *)p_key;
			Cb_Clu.SetBuffer(p_data, rec_size);
			Cb_Clu.GetTile(outer_id, &tile);
			rResult.Set(&tile.V, sizeof(tile.V));
			return 0;
		}
		PPOsm::NodeCluster Cb_Clu;
	};
	new BDbTable(BDbTable::Config("geomap.db->node_idx01", BDbTable::idxtypBTree, BDbTable::cfDup, 2048, 4*1024), pDb, new Idx01, this);
	*/
}

SrGeoNodeTbl::~SrGeoNodeTbl()
{
}

IMPL_CMPCFUNC(PPOsm_Node_ByWay, p1, p2)
{
	uint64 id1 = *(uint64 *)p1;
	uint64 id2 = *(uint64 *)p2;
	uint   pos1 = 0;
	uint   pos2 = 0;
	const PPOsm::Way * p_way = pExtraData ? (const PPOsm::Way *)pExtraData : 0;
    if(p_way) {
        if(!p_way->NodeRefs.lsearch(id1, &pos1))
			pos1 = UINT_MAX;
        if(!p_way->NodeRefs.lsearch(id2, &pos2))
			pos2 = UINT_MAX;
    }
	else {
		pos1 = UINT_MAX;
		pos2 = UINT_MAX;
	}
    return CMPSIGN(pos1, pos2);
}

int SLAPI SrGeoNodeTbl::GetWayNodes(const PPOsm::Way & rWay, TSVector <PPOsm::Node> & rNodeList) // @v9.8.6 TSArray-->TSVector
{
	int    ok = -1;
	const  uint _c = rWay.NodeRefs.getCount();

	rNodeList.clear();
	PPOsm::NodeCluster clu;
	TSVector <PPOsm::Node> clu_node_list; // @v9.8.4 TSArray-->TSVector
	UintHashTable processed_pos_list;
	//
	// Для замкнутого контура последняя точка равна первой - просто добавим ее в список в самом конце функции
	//
	const int is_contur = BIN(_c > 1 && rWay.NodeRefs.get(_c-1) == rWay.NodeRefs.get(0));
	if(is_contur)
		processed_pos_list.Add(_c-1);
    for(uint i = 0; i < _c; i++) {
		if(!processed_pos_list.Has(i)) {
			const uint64 node_id = (uint64)rWay.NodeRefs.get(i);
			uint64 logical_id = 0;
			PPOsm::Node node;
			if(Helper_Search(node_id, &clu, &node, 0, &logical_id) > 0) {
				rNodeList.insert(&node);
				processed_pos_list.Add(i);
				clu.Get(logical_id, clu_node_list, 0);
				for(uint k = 0; k < clu_node_list.getCount(); k++) {
					const PPOsm::Node & r_node = clu_node_list.at(k);
					uint  w_pos = 0;
					if(r_node.ID != node.ID && rWay.NodeRefs.lsearch(r_node.ID, &w_pos) && !processed_pos_list.Has(w_pos)) {
						rNodeList.insert(&r_node);
						processed_pos_list.Add(w_pos);
					}
				}
			}
		}
    }
    rNodeList.sort(PTR_CMPCFUNC(PPOsm_Node_ByWay), (void *)&rWay); // @badcast
	//
	// Последнюю точку контура вставляем после сортировки - иначе сортировка сбойнет (вда одинаковых идентификатора)
	//
	if(is_contur) {
		const PPOsm::Node & r_first_node = clu_node_list.at(0);
		rNodeList.insert(&r_first_node);
	}
    if(rNodeList.getCount() == rWay.NodeRefs.getCount())
		ok = 1;
    return ok;
}

int SLAPI SrGeoNodeTbl::Search(uint64 id, PPOsm::Node * pNode, PPOsm::NodeRefs * pNrList, uint64 * pLogicalID)
	{ return Helper_Search(id, 0, pNode, pNrList, pLogicalID); }
int SLAPI SrGeoNodeTbl::Search(uint64 id, PPOsm::NodeCluster * pCluster, uint64 * pLogicalID)
	{ return Helper_Search(id, pCluster, 0, 0, pLogicalID); }

int SLAPI SrGeoNodeTbl::Helper_Search(uint64 id, PPOsm::NodeCluster * pCluster, PPOsm::Node * pNode, PPOsm::NodeRefs * pNrList, uint64 * pLogicalID)
{
/*
LogicalCount=  1; ClusterCount=64512077; ActualCount=64512077; Size=838657001;
LogicalCount=128; ClusterCount=  285217; ActualCount=29844065; Size=169060869;
LogicalCount=  2; ClusterCount=10242038; ActualCount=20484076; Size=191137226;
LogicalCount= 64; ClusterCount=  369007; ActualCount=19787392; Size=112540879;
LogicalCount= 32; ClusterCount=  694227; ActualCount=19543439; Size=112406576;
LogicalCount=  4; ClusterCount= 5003717; ActualCount=18204549; Size=139336132;
LogicalCount= 16; ClusterCount= 1262121; ActualCount=17685283; Size=106760761;
LogicalCount=  8; ClusterCount= 2389896; ActualCount=17049582; Size=112131954;
*/
	const uchar logical_count_priority[]      = {    1,  128,    2,   64,   32,    4,   16,    8 };
	const uchar logical_count_priority_bits[] = { 0x00, 0x7f, 0x01, 0x3f, 0x1f, 0x03, 0x0f, 0x07 };
	int    ok = -1;
	uint64 ret_logical_id = 0;
	PPOsm::NodeCluster nc;
	TSVector <PPOsm::Node> nc_list; // @v9.8.4 TSArray-->TSVector
	PPOsm::NodeRefs nr_list;
	DataBuf.Alloc(1024);
	for(uint i = 0; ok < 0 && i < SIZEOFARRAY(logical_count_priority); /* see end of loop */) {
		const uint64 logical_id = (id & ~logical_count_priority_bits[i]);
		{
			uint8  _key[16];
			size_t _key_sz = sshrinkuint64(logical_id, _key);
			KeyBuf.Set(_key, _key_sz);
		}
		uint   next_i = UINT_MAX;
		if(BDbTable::Search(0, KeyBuf, DataBuf)) {
			{
				size_t dbsz = 0;
				const void * p_dbptr = DataBuf.GetPtr(&dbsz);
				nc.SetBuffer(p_dbptr, dbsz);
			}
			PPOsm::Node ex_head;
			uint   ex_count_logic = 0;
			uint   ex_count_actual = 0;
			THROW(nc.Get(logical_id, nc_list, &nr_list, &ex_head, &ex_count_logic, &ex_count_actual));
			assert(logical_id == ex_head.ID); // @paranoic
			assert(id >= logical_id); // @paranoic
			if(id < (logical_id + ex_count_logic)) {
				for(uint ncidx = 0; ok < 0 && ncidx < nc_list.getCount(); ncidx++) {
					const PPOsm::Node & r_node = nc_list.at(ncidx);
					if(r_node.ID == id) {
						ret_logical_id = logical_id;
						ASSIGN_PTR(pNode, r_node);
						ASSIGN_PTR(pCluster, nc);
						ok = (int)logical_count_priority[i];
					}
				}
				if(ok < 0) {
					ret_logical_id = logical_id;
					const uchar tp = logical_count_priority[i];
					for(uint j = i+1; j < SIZEOFARRAY(logical_count_priority); j++) {
						if(logical_count_priority[j] > tp) {
							next_i = j;
							break;
						}
					}
				}
			}
			else {
				ret_logical_id = 0;
				next_i = i+1;
			}
		}
		else {
			ret_logical_id = 0;
			next_i = i+1;
		}
		i = next_i;
	}
	CATCHZOK
	ASSIGN_PTR(pLogicalID, ret_logical_id);
	return ok;
}

int SLAPI SrGeoNodeTbl::Helper_Set(PPOsm::NodeCluster & rNc, uint64 outerID, int update)
{
	int    ok = 1;
	uint64 hid = 0;
	int    his = 0;
	{
		if(outerID) {
			hid = outerID;
		}
		else {
			THROW(his = rNc.GetHeaderID(&hid));
		}
		uint8  _key[16];
		size_t _key_sz = sshrinkuint64(hid, _key);
		KeyBuf.Set(_key, _key_sz);
	}
	{
		size_t buf_size = 0;
		const void * p_buf = rNc.GetBuffer(&buf_size);
		THROW(p_buf);
		THROW(DataBuf.Set(p_buf, buf_size));
	}
	if(update) {
		THROW_DB(UpdateRec(KeyBuf, DataBuf));
	}
	else {
		THROW_DB(InsertRec(KeyBuf, DataBuf));
	}
	CATCHZOK
	return ok;
}

int SLAPI SrGeoNodeTbl::Add(PPOsm::NodeCluster & rNc, uint64 outerID) { return Helper_Set(rNc, outerID, 0); }
int SLAPI SrGeoNodeTbl::Update(PPOsm::NodeCluster & rNc, uint64 outerID) { return Helper_Set(rNc, outerID, 1); }
//
//
//
SLAPI SrGeoWayTbl::SrGeoWayTbl(BDbDatabase * pDb) : BDbTable(BDbTable::ConfigHash("geomap.db->way", 0, SKILOBYTE(2), 0), pDb)
{
}

SLAPI SrGeoWayTbl::~SrGeoWayTbl()
{
}

int SLAPI SrGeoWayTbl::Add(PPOsm::Way & rW, PPOsm::WayBuffer * pBuffer)
{
	int    ok = 1;
	{
		uint8  _key[16];
		size_t _key_sz = sshrinkuint64(rW.ID, _key);
		KeyBuf.Set(_key, _key_sz);
	}
	{
		PPOsm::WayBuffer wbuf__;
		SETIFZ(pBuffer, &wbuf__);
		uint64 outer_id = rW.ID;
		THROW(pBuffer->Put(&rW, &outer_id));
		{
			//
			// Test
			//
            PPOsm::Way test_w;
            assert(pBuffer->Get(rW.ID, &test_w));
            assert(test_w == rW);
		}
		size_t buf_size = 0;
		const void * p_buf = pBuffer->GetBuffer(&buf_size);
		THROW(p_buf);
		THROW(DataBuf.Set(p_buf, buf_size));
	}
	THROW_DB(InsertRec(KeyBuf, DataBuf));
	CATCHZOK
	return ok;
}

int SLAPI SrGeoWayTbl::Search(uint64 id, PPOsm::Way * pW)
{
	int    ok = -1;
	DataBuf.Alloc(12*1024);
	{
		{
			uint8  _key[16];
			size_t _key_sz = sshrinkuint64(id, _key);
			KeyBuf.Set(_key, _key_sz);
		}
		if(BDbTable::Search(0, KeyBuf, DataBuf)) {
			PPOsm::WayBuffer wbuf;
			{
				size_t dbsz = 0;
				const void * p_dbptr = DataBuf.GetPtr(&dbsz);
				wbuf.SetBuffer(p_dbptr, dbsz);
			}
			if(pW) {
				THROW(wbuf.Get(id, pW));
			}
			ok = 1;
		}
	}
	CATCHZOK
	return ok;
}
//
//
//
//static
int SrDatabase::MakeSpecialWord(int spcTag, const char * pWordUtf8, SString & rBuf)
{
	int    ok = 1;
	rBuf.Z();
	switch(spcTag) {
		case SrWordTbl::spcEmpty: rBuf.Cat("/#"); break;
		case SrWordTbl::spcPrefix: rBuf.Cat(pWordUtf8).Cat("/-"); break;
		case SrWordTbl::spcAffix: rBuf.Cat("/-").Cat(pWordUtf8); break;
		case SrWordTbl::spcConcept: rBuf.Cat("/:").Cat(pWordUtf8); break;
		case SrWordTbl::spcCPropSymb: rBuf.Cat("/.").Cat(pWordUtf8); break;
		default:
			ok = PPSetError(PPERR_SR_INVSPCWORDTAG, (long)spcTag);
			break;
	}
	return ok;
}

SrDatabase::SrDatabase() : WordCache(SKILOBYTE(1024), 0), PropInstance(0), PropSubclass(0), PropType(0), PropHMember(0),
	P_Db(0), P_WdT(0), P_GrT(0), P_WaT(0), P_CT(0), P_CpT(0), P_NgT(0), P_CNgT(0),
	P_GnT(0), P_GwT(0), ZeroWordID(0), Flags(0)/*, RvlBuf(128)*/
{
}

SrDatabase::~SrDatabase()
{
	Close();
}

int SrDatabase::Open(const char * pDbPath, long flags)
{
	int    ok = 1;
	BDbDatabase::Config cfg;
	PPIniFile ini_file;
	int64  ini_val;
	if(flags & oReadOnly) {
		cfg.CacheSize   = (ini_file.GetDataSize(PPINISECT_CONFIG, PPINIPARAM_SARTREDB_CACHESIZE_RO, &ini_val) > 0)   ? ini_val : SMEGABYTE(128);
		cfg.MaxLockers  = (ini_file.GetDataSize(PPINISECT_CONFIG, PPINIPARAM_SARTREDB_MAXLOCKERS_RO, &ini_val) > 0)  ? ini_val : SKILOBYTE(64);
		cfg.MaxLocks    = (ini_file.GetDataSize(PPINISECT_CONFIG, PPINIPARAM_SARTREDB_MAXLOCKS_RO, &ini_val) > 0)    ? ini_val : SKILOBYTE(32);
		cfg.MaxLockObjs = (ini_file.GetDataSize(PPINISECT_CONFIG, PPINIPARAM_SARTREDB_MAXLOCKOBJS_RO, &ini_val) > 0) ? ini_val : SKILOBYTE(32);
	}
	else {
		cfg.CacheSize   = (ini_file.GetDataSize(PPINISECT_CONFIG, PPINIPARAM_SARTREDB_CACHESIZE, &ini_val) > 0)   ? ini_val : SMEGABYTE(512+512);
		cfg.MaxLockers  = (ini_file.GetDataSize(PPINISECT_CONFIG, PPINIPARAM_SARTREDB_MAXLOCKERS, &ini_val) > 0)  ? ini_val : SKILOBYTE(512);
		cfg.MaxLocks    = (ini_file.GetDataSize(PPINISECT_CONFIG, PPINIPARAM_SARTREDB_MAXLOCKS, &ini_val) > 0)    ? ini_val : SKILOBYTE(512);
		cfg.MaxLockObjs = (ini_file.GetDataSize(PPINISECT_CONFIG, PPINIPARAM_SARTREDB_MAXLOCKOBJS, &ini_val) > 0) ? ini_val : SKILOBYTE(512);
	}
	cfg.LogBufSize = (ini_file.GetDataSize(PPINISECT_CONFIG, PPINIPARAM_SARTREDB_LOGBUFSIZE, &ini_val) > 0) ? ini_val : SMEGABYTE(8);
	//cfg.CacheSize   = (flags & oReadOnly) ? SMEGABYTE(128) : SMEGABYTE(512+512); // @v10.1.5 512-->512+512
	//cfg.CacheCount  = 1; // @v9.6.4 20-->
	//cfg.MaxLockers  = (flags & oReadOnly) ? SKILOBYTE(64) : SKILOBYTE(512); // @v9.6.2 20000-->256*1024 // @v10.0.01 256-->512
	//cfg.MaxLocks    = (flags & oReadOnly) ? SKILOBYTE(32) : SKILOBYTE(512); // @v9.6.4 // @v10.0.01 128-->256 // @v10.0.12 256-->512
	//cfg.MaxLockObjs = (flags & oReadOnly) ? SKILOBYTE(32) : SKILOBYTE(512); // @v9.6.4 // @v10.0.01 128-->256 // @v10.0.12 256-->512
	//cfg.LogBufSize  = SMEGABYTE(8);
	//cfg.LogFileSize = 256*1024*1024;
	//cfg.LogSubDir = "LOG";
	cfg.Flags |= (cfg.fLogNoSync|cfg.fLogAutoRemove/*|cfg.fLogInMemory*/); // @v9.6.6 // @v10.0.01 /*cfg.fLogNoSync*/
	//
	Flags |= (flags & (oReadOnly|oWriteStatOnClose|oExclusive)); // @v9.7.11
	//
	Close();
	{
		SString db_path = pDbPath;
		if(db_path.Empty()) {
			PPGetPath(PPPATH_SARTREDB, db_path);
		}
		THROW_PP(db_path.NotEmpty() && pathValid(db_path, 1), PPERR_SARTREDBUNDEF);
		{
			long   db_options = BDbDatabase::oPrivate/*|BDbDatabase::oRecover*/;
			if(Flags & oReadOnly)
				db_options |= BDbDatabase::oReadOnly;
			if(Flags & oWriteStatOnClose)
				db_options |= BDbDatabase::oWriteStatOnClose;
			if(Flags & oExclusive)
				db_options |= BDbDatabase::oExclusive;
			THROW_S(P_Db = new BDbDatabase(db_path, &cfg, db_options), SLERR_NOMEM);
			THROW(!!*P_Db);
		}
		THROW_S(P_WdT = new SrWordTbl(P_Db), SLERR_NOMEM);
		THROW_S(P_GrT = new SrGrammarTbl(P_Db), SLERR_NOMEM);
		THROW_S(P_WaT = new SrWordAssocTbl(P_Db), SLERR_NOMEM);
		THROW_S(P_CT = new SrConceptTbl(P_Db), SLERR_NOMEM);
		THROW_S(P_CpT = new SrConceptPropTbl(*this), SLERR_NOMEM);
		THROW_S(P_NgT = new SrNGramTbl(P_Db), SLERR_NOMEM);
		THROW_S(P_CNgT = new SrConceptNgTbl(P_Db), SLERR_NOMEM);
		THROW_S(P_GnT = new SrGeoNodeTbl(P_Db), SLERR_NOMEM);
		THROW_S(P_GwT = new SrGeoWayTbl(P_Db), SLERR_NOMEM);
		{
			CONCEPTID prop_instance = ResolveReservedConcept(rcInstance);
			CONCEPTID prop_subclass = ResolveReservedConcept(rcSubclass);
			CONCEPTID prop_crtype   = ResolveReservedConcept(rcType);
			CONCEPTID prop_hmember  = ResolveReservedConcept(rcHMember);
			THROW(prop_instance);
			THROW(prop_subclass);
			THROW(prop_crtype);
			THROW(prop_hmember);
		}
		{
			SString err_file_name;
			(err_file_name = pDbPath).SetLastSlash().Cat("bdberr.log");
			P_Db->SetupErrLog(err_file_name);
		}
	}
	CATCH
		ok = 0;
		Close();
	ENDCATCH
	return ok;
}

int SrDatabase::CreateAnonymConcept(CONCEPTID * pID)
{
	int    ok = 1;
	SrConcept c;
	THROW(P_CT->Add(c));
	ASSIGN_PTR(pID, c.ID);
	CATCHZOK
	return ok;
}

int SrDatabase::GetPropType(CONCEPTID propID)
{
	int    type = SRPROPT_INT;
	if(PropType) {
		if(oneof3(propID, PropInstance, PropSubclass, PropType))
			type = SRPROPT_INT;
		else if(P_CpT) {
			SrCProp cp_type(propID, PropType);
			if(P_CpT->Search(cp_type) > 0) {
				int64 _t = 0;
				if(cp_type.Get(_t) > 0)
					type = (int)_t;
				else
					type = 0;
			}
		}
		else
			type = 0;
	}
	return type;
}

int SrDatabase::SearchConcept(const char * pSymbUtf8, CONCEPTID * pID)
{
	int    ok = -1;
	CONCEPTID cid = 0;
	LEXID  lex_id = 0;
	if(FetchSpecialWord(SrWordTbl::spcConcept, pSymbUtf8, &lex_id) > 0) {
		SrConcept c;
		if(P_CT->SearchBySymb(lex_id, &c) > 0) {
			cid = c.ID;
			ok = 1;
		}
		else {
			SString & r_temp_buf = SLS.AcquireRvlStr();
			(r_temp_buf = pSymbUtf8).Transf(CTRANSF_UTF8_TO_INNER);
			PPSetError(PPERR_SR_CONCEPTNFOUND, r_temp_buf);
			ok = -2;
		}
	}
	ASSIGN_PTR(pID, cid);
	return ok;
}

int SrDatabase::ResolveConcept(const char * pSymbUtf8, CONCEPTID * pID)
{
	int    ok = -1;
	LEXID  lex_id = 0;
	if(SearchSpecialWord(SrWordTbl::spcConcept, pSymbUtf8, &lex_id) > 0) {
		SrConcept c;
		if(P_CT->SearchBySymb(lex_id, &c) > 0) {
			ASSIGN_PTR(pID, c.ID);
			ok = 1;
		}
		else {
			c.ID = 0;
			c.SymbID  = lex_id;
			THROW(P_CT->Add(c));
			ASSIGN_PTR(pID, c.ID);
			ok = 2;
		}
	}
	else {
		THROW(P_WdT->AddSpecial(SrWordTbl::spcConcept, pSymbUtf8, &lex_id));
		{
			SrConcept c;
			c.ID = 0;
			c.SymbID  = lex_id;
			THROW(P_CT->Add(c));
			ASSIGN_PTR(pID, c.ID);
			ok = 3;
		}
	}
	CATCHZOK
	return ok;
}

int SrDatabase::ResolveCPropSymb(const char * pSymbUtf8, LEXID * pID)
{
	int    ok = -1;
	LEXID  lex_id = 0;
	if(SearchSpecialWord(SrWordTbl::spcCPropSymb, pSymbUtf8, &lex_id) > 0) {
		ok = 1;
	}
	else {
		THROW(P_WdT->AddSpecial(SrWordTbl::spcCPropSymb, pSymbUtf8, &lex_id));
	}
	CATCH
		ok = 0;
		lex_id = 0;
	ENDCATCH
	ASSIGN_PTR(pID, lex_id);
	return ok;
}

int SrDatabase::ResolveWord(const char * pWordUtf8, LEXID * pID)
{
	int    ok = -1;
	LEXID  lex_id = 0;
	if(P_WdT->Search(pWordUtf8, &lex_id) > 0) {
		ASSIGN_PTR(pID, lex_id);
		ok = 1;
	}
	else {
		THROW(P_WdT->Add(pWordUtf8, &lex_id));
		ASSIGN_PTR(pID, lex_id);
		ok = 2;
	}
	CATCHZOK
	return ok;
}

int SrDatabase::ResolveNGram(const LongArray & rList, NGID * pID)
{
	int    ok = -1;
	NGID   ng_id = 0;
	SrNGram ng;
	ng.WordIdList = rList;
	if(P_NgT->Search(ng, &ng_id) > 0) {
		ASSIGN_PTR(pID, ng_id);
		ok = 1;
	}
	else {
		ng.ID = 0;
		THROW(P_NgT->Add(ng));
		ASSIGN_PTR(pID, ng.ID);
		ok = 2;
	}
	CATCHZOK
	return ok;
}

void SrDatabase::Close()
{
	PropInstance = 0;
	PropSubclass = 0;
	PropType = 0;
	PropHMember = 0;
	ZDELETE(P_WdT);
	ZDELETE(P_GrT);
	ZDELETE(P_WaT);
	ZDELETE(P_CT);
	ZDELETE(P_CpT);
	ZDELETE(P_NgT);
	ZDELETE(P_CNgT);
	ZDELETE(P_GnT);
	ZDELETE(P_GwT);
	CALLPTRMEMB(P_Db, RemoveUnusedLogs());
	ZDELETE(P_Db);
}

CONCEPTID FASTCALL SrDatabase::GetReservedConcept(int rc) const
{
	CONCEPTID prop = 0;
	switch(rc) {
		case rcInstance: prop = PropInstance; break;
		case rcSubclass: prop = PropSubclass; break;
		case rcType: prop = PropType; break;
		case rcHMember: prop = PropHMember; break;
		default: PPSetError(PPERR_SR_RSVRCONCEPTNFOUND, rc); break;
	}
	return prop;
}

CONCEPTID FASTCALL SrDatabase::ResolveReservedConcept(int rc)
{
	CONCEPTID prop = 0;
	switch(rc) {
		case rcInstance: if(!PropInstance) { THROW(ResolveConcept("crp_instance", &PropInstance)); } prop = PropInstance; break;
		case rcSubclass: if(!PropSubclass) { THROW(ResolveConcept("crp_subclass", &PropSubclass)); } prop = PropSubclass; break;
		case rcType:     if(!PropType)     { THROW(ResolveConcept("crp_type", &PropType)); } prop = PropType; break;
		case rcHMember:  if(!PropHMember)  { THROW(ResolveConcept("crp_hmember", &PropHMember)); } prop = PropHMember; break;
	}
	CATCH
		prop = 0;
	ENDCATCH
	return prop;
}

int SrDatabase::SetSimpleWordFlexiaModel_Express(LEXID wordID, long baseFormID, int32 * pResultWaId)
{
	int    ok = -1;
	int    r;
	int32  result_wa_id = 0;
	if(baseFormID) {
		SrWordAssoc wa;
		//SString word_utf8;
		//THROW(P_WdT->Search(wordID, word_utf8) > 0);
		wa.WordID = wordID;
		wa.BaseFormID = baseFormID;
		{
			//
			// Находим или создаем модель по содержанию и получаем ее идентификатор
			//
			SrFlexiaModel fm;
			SrFlexiaModel::Item fmi;
			fmi.WordFormID = wa.BaseFormID;
			fm.Add(fmi);
			fm.Normalize();
			THROW(r = P_GrT->Search(&fm, &wa.FlexiaModelID));
			if(r < 0)
				THROW(P_GrT->Add(&fm, &wa.FlexiaModelID));
		}
		{
			assert(wa.BaseFormID);
			assert(wa.FlexiaModelID);
			int32   wa_id = 0;
			TSVector <SrWordAssoc> wa_list;
			P_WaT->Search(wordID, wa_list);
			for(uint i = 0; !wa_id && i < wa_list.getCount(); i++) {
				const SrWordAssoc & r_wa = wa_list.at(i);
				if(r_wa.WordID == wa.WordID && r_wa.BaseFormID == wa.BaseFormID && r_wa.FlexiaModelID == wa.FlexiaModelID) {
					wa_id = r_wa.ID;
				}
			}
			if(wa_id)
				ok = 2;
			else {
				THROW(r = P_WaT->Add(&wa.Normalize(), &wa_id));
				ok = 1;
			}
			result_wa_id = wa_id;
		}
	}
	CATCHZOK
	ASSIGN_PTR(pResultWaId, result_wa_id);
	return ok;
}

int SrDatabase::ResolveWordForm(const SrWordForm & rWf, long * pWordFormID)
{
	int    ok = 1;
	int    r;
	long   form_id = 0;
	SrWordForm wf_key = rWf;
	wf_key.Normalize();
	THROW(r = P_GrT->Search(&wf_key, &form_id));
	if(r > 0) {
		ok = 1;
	}
	else {
		THROW(P_GrT->Add(&wf_key, &form_id));
		ok = 2;
	}
	CATCHZOK
	ASSIGN_PTR(pWordFormID, form_id);
	return ok;
}

int SrDatabase::SetSimpleWordFlexiaModel(LEXID wordID, const SrWordForm & rWf, int32 * pResultWaId)
{
	int    ok = -1;
	//int    r;
	int32  result_wa_id = 0;
	if(rWf.GetLength()) {
		//SrWordAssoc wa;
		long    base_form_id = 0;
		SString word_utf8;
		THROW(P_WdT->Search(wordID, word_utf8) > 0);
		//wa.WordID = wordID;
		//
		// Находим или создаем словоформу по содержанию и получаем ее идентификатор
		//
		THROW(ResolveWordForm(rWf, &base_form_id));
		assert(base_form_id != 0);
		ok = SetSimpleWordFlexiaModel_Express(wordID, base_form_id, &result_wa_id); // @v10.0.01
		/* @v10.0.01
		{
			//
			// Находим или создаем модель по содержанию и получаем ее идентификатор
			//
			SrFlexiaModel fm;
			SrFlexiaModel::Item fmi;
			fmi.WordFormID = wa.BaseFormID;
			fm.Add(fmi);
			fm.Normalize();
			THROW(r = P_GrT->Search(&fm, &wa.FlexiaModelID));
			if(r < 0)
				THROW(P_GrT->Add(&fm, &wa.FlexiaModelID));
		}
		{
			assert(wa.BaseFormID);
			assert(wa.FlexiaModelID);
			int32   wa_id = 0;
			TSVector <SrWordAssoc> wa_list;
			P_WaT->Search(wordID, wa_list);
			for(uint i = 0; !wa_id && i < wa_list.getCount(); i++) {
				const SrWordAssoc & r_wa = wa_list.at(i);
				if(r_wa.WordID == wa.WordID && r_wa.BaseFormID == wa.BaseFormID && r_wa.FlexiaModelID == wa.FlexiaModelID) {
					wa_id = r_wa.ID;
				}
			}
			if(wa_id)
				ok = 2;
			else {
				THROW(r = P_WaT->Add(&wa.Normalize(), &wa_id));
				ok = 1;
			}
			result_wa_id = wa_id;
		}
		*/
	}
	CATCHZOK
	ASSIGN_PTR(pResultWaId, result_wa_id);
	return ok;
}

int SrDatabase::GetBaseWordInfo(LEXID wordID, LEXID pfxID, LEXID afxID, TSVector <SrWordAssoc> & rWaList, TSVector <SrWordInfo> & rInfo)
{
	int    ok = -1;
	LongArray wf_list;
	SrFlexiaModel fm;
	rWaList.clear();
	P_WaT->Search(wordID, rWaList);
	for(uint i = 0; i < rWaList.getCount(); i++) {
		const SrWordAssoc & r_wa = rWaList.at(i);
		if(r_wa.FlexiaModelID) {
			fm.Clear();
			if(P_GrT->Search(r_wa.FlexiaModelID, &fm) > 0) {
				wf_list.clear();
				if(fm.Search(afxID, pfxID, wf_list) > 0) {
					for(uint j = 0; j < wf_list.getCount(); j++) {
						SrWordInfo ii;
						ii.BaseID = wordID;
						ii.PrefixID = pfxID;
						ii.AffixID = afxID;
						ii.BaseFormID = r_wa.BaseFormID;
						ii.FormID = wf_list.get(j);
						ii.WaID = r_wa.ID;
						ii.AbbrExpID = r_wa.AbbrExpID; // @v9.8.12
						rInfo.insert(&ii);
						ok = 1;
					}
				}
			}
		}
	}
	return ok;
}

int SrDatabase::IsWordInForm(const char * pWordUtf8, const SrWordForm & rForm)
{
	int    ok = -1;
	TSVector <SrWordInfo> info_list;
	SrWordForm base_wf, var_wf, wf;
	GetWordInfo(pWordUtf8, 0, info_list);
	for(uint i = 0; ok < 0 && i < info_list.getCount(); i++) {
		const SrWordInfo & r_wi = info_list.at(i);
		if(r_wi.FormID || r_wi.BaseFormID) {
			base_wf.Clear();
			var_wf.Clear();
			wf.Clear();
			if(r_wi.BaseFormID)
				P_GrT->Search(r_wi.BaseFormID, &base_wf);
			if(r_wi.FormID)
				P_GrT->Search(r_wi.FormID, &var_wf);
			wf.Merge_(base_wf, var_wf, 0);
			if(rForm.IsSubsetOf(wf))
				ok = 1;
		}
	}
	return ok;
}

int SrDatabase::GetWordInfo(const char * pWordUtf8, long /*flags*/, TSVector <SrWordInfo> & rInfo)
{
	int    ok = -1;
	SStringU word_buf, base_buf_u, afx_buf_u, pfx_buf_u;
	SString temp_buf;
	word_buf.CopyFromUtf8(pWordUtf8, sstrlen(pWordUtf8));
	int    tc = word_buf.AnalyzeCase();
	base_buf_u = word_buf.ToLower();
	const  uint len = word_buf.Len();
	StrAssocArray afx_list;
	TSVector <SrWordAssoc> wa_list; // @v9.8.4 TSArray-->TSVector
	for(uint pfx_len = 0; pfx_len < len; pfx_len++) {
		int    inv_pfx = 0; // Если !0, то префикс не допустимый
		LEXID  pfx_id = 0;
		if(pfx_len) {
			word_buf.Sub(0, pfx_len, pfx_buf_u);
			pfx_buf_u.CopyToUtf8(temp_buf, 0);
			if(FetchSpecialWord(SrWordTbl::spcPrefix, temp_buf, &pfx_id) > 0) {
				;
			}
			else
				inv_pfx = 1;
		}
		else
			pfx_buf_u.Z();
		if(!inv_pfx) { // Если префикс не пустой и не содержится в БД, то перебирать оставшуюся часть слова нет смысла
			const  uint __len = len-pfx_len;
			for(uint afx_len = 0; afx_len <= __len; afx_len++) {
				int    inv_afx = 0; // Если !0, то аффикс не допустимый
				LEXID  base_id = 0, afx_id = 0;
				const  uint base_len = __len-afx_len;
				if(base_len == 0) {
					base_buf_u.Z();
					if(ZeroWordID)
						base_id = ZeroWordID;
					else if(FetchSpecialWord(SrWordTbl::spcEmpty, temp_buf, &base_id) > 0)
						ZeroWordID = base_id;
				}
				else {
					word_buf.Sub(pfx_len, base_len, base_buf_u);
					base_buf_u.CopyToUtf8(temp_buf, 0);
					if(FetchWord(temp_buf, &base_id) > 0) {
						;
					}
				}
				if(afx_len != 0) {
					word_buf.Sub(pfx_len+base_len, afx_len, afx_buf_u);
					afx_buf_u.CopyToUtf8(temp_buf, 0);
					uint   afx_pos = 0;
					if(afx_list.SearchByText(temp_buf, 0, &afx_pos))
						afx_id = afx_list.Get(afx_pos).Id;
					else if(pfx_len == 0 && FetchSpecialWord(SrWordTbl::spcAffix, temp_buf, &afx_id) > 0) {
						//
						// Для pfx_len > 0 все возможные окончания уже найдены на итерации (pfx == 0)
						//
						afx_list.Add(afx_id, temp_buf);
					}
					else
						inv_afx = 1;
				}
				else
					afx_id = 0;
				if(base_id && !inv_afx) {
					if(GetBaseWordInfo(base_id, pfx_id, afx_id, wa_list, rInfo) > 0)
						ok = 1;
				}
			}
		}
	}
	return ok;
}

int SrDatabase::Transform_(const char * pWordUtf8, const SrWordForm & rDestForm, TSVector <SrWordInfo> & rResult)
{
	int    ok = -1;
	rResult.clear();
	TSVector <SrWordInfo> info_list;
	if(GetWordInfo(pWordUtf8, 0, info_list) > 0) {
		SrWordForm base_wf, wf, test_wf;
		TSVector <SrWordAssoc> wa_list; // @v9.8.4 TSArray-->TSVector
		for(uint j = 0; j < info_list.getCount(); j++) {
			const SrWordInfo & r_item = info_list.at(j);
			if(r_item.FormID) {
				P_GrT->Search(r_item.FormID, &base_wf);
				test_wf.Merge_(base_wf, rDestForm, 1);
			}
			else if(r_item.BaseFormID) {
				P_GrT->Search(r_item.BaseFormID, &base_wf);
				test_wf.Merge_(base_wf, rDestForm, 1);
			}
			else
				test_wf = rDestForm;
			if(P_WaT->Search(r_item.BaseID, wa_list) > 0) {
				for(uint i = 0; i < wa_list.getCount(); i++) {
					const SrWordAssoc & r_wa = wa_list.at(i);
					if(r_wa.FlexiaModelID) {
						SrFlexiaModel model;
						if(P_GrT->Search(r_wa.FlexiaModelID, &model) > 0) {
							SrFlexiaModel::Item model_item;
							for(size_t fp = 0; model.GetNext(&fp, model_item) > 0;) {
								if(model_item.WordFormID && P_GrT->Search(model_item.WordFormID, &wf) > 0) {
									double score1 = test_wf.MatchScore(wf);
									double score2 = rDestForm.MatchScore(wf);
									double score = score1+score2;
									if(score > 0.0) {
										uint rc = rResult.getCount();
										int  do_insert = 0;
										if(rc) {
											do {
												SrWordInfo & r_res_item = rResult.at(--rc);
												if(score > r_res_item.Score) {
													// if(r_res_item.Score < 1.0) // @debug
														rResult.atFree(rc);
													do_insert = 1;
												}
												else if(score == r_res_item.Score)
													do_insert = 1;
											} while(rc);
										}
										else
											do_insert = 1;
										if(do_insert) {
											SrWordInfo ii;
											ii.BaseID = r_wa.WordID;
											ii.PrefixID = model_item.PrefixID;
											ii.AffixID = model_item.AffixID;
											ii.BaseFormID = r_wa.BaseFormID;
											ii.FormID = model_item.WordFormID;
											ii.WaID = r_wa.ID;
											ii.Score = score;
											rResult.insert(&ii);
											ok = 1;
										}
									}
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

int SrDatabase::WordInfoToStr(const SrWordInfo & rWi, SString & rBuf)
{
	int    ok = 1;
	rBuf.Z();
	SString temp_buf;
	rBuf.CatChar('\'');
	if(rWi.PrefixID) {
		if(P_WdT->Search(rWi.PrefixID, temp_buf) > 0)
			rBuf.Cat(temp_buf);
		else
			rBuf.CatChar('=').Cat(rWi.PrefixID);
	}
	rBuf.CatChar('|');
	if(rWi.BaseID) {
		if(rWi.BaseID != ZeroWordID) {
			if(P_WdT->Search(rWi.BaseID, temp_buf) > 0)
				rBuf.Cat(temp_buf);
			else
				rBuf.CatChar('=').Cat(rWi.BaseID);
		}
	}
	rBuf.CatChar('|');
	if(rWi.AffixID) {
		if(P_WdT->Search(rWi.AffixID, temp_buf) > 0)
			rBuf.Cat(temp_buf);
		else
			rBuf.CatChar('=').Cat(rWi.AffixID);
	}
	rBuf.CatChar('\'');
	rBuf.Space();
	if(rWi.FormID || rWi.BaseFormID) {
		SrWordForm base_wf, var_wf, wf;
		if(rWi.BaseFormID)
			P_GrT->Search(rWi.BaseFormID, &base_wf);
		if(rWi.FormID)
			P_GrT->Search(rWi.FormID, &var_wf);
		wf.Merge_(base_wf, var_wf, 0);
		wf.ToStr(temp_buf);
	}
	else
		temp_buf.Z();
	rBuf.CatBrackStr(temp_buf);
	if(rWi.AbbrExpID) {
		SString word_buf;
		temp_buf.Z().Cat("abbr").CatChar('(');
		SrNGram ng;
		if(P_NgT->Search(rWi.AbbrExpID, &ng) > 0) {
			for(uint i = 0; i < ng.WordIdList.getCount(); i++) {
				if(i)
					temp_buf.Space();
				if(P_WdT->Search(ng.WordIdList.get(i), word_buf) > 0)
					temp_buf.Cat(word_buf);
				else
					temp_buf.CatChar('#').Cat(ng.WordIdList.get(i));
			}
		}
		else
			temp_buf.Cat("#ng=").Cat(rWi.AbbrExpID);
		temp_buf.CatChar(')');
		rBuf.Space().Cat(temp_buf);
	}
	//
	temp_buf.Z();
	if(rWi.WaID) {
		SrWordAssoc wa;
		if(P_WaT->Search(rWi.WaID, &wa) > 0)
			wa.ToStr(temp_buf);
		else
			temp_buf.CatChar('[').CatChar('=').Cat(rWi.WaID).CatChar(']');
	}
	else
		temp_buf.CatChar('[').CatChar(']');
	if(rWi.Score != 0.0) {
		temp_buf.Space().CatEq("Score", rWi.Score, MKSFMTD(0, 1, 0));
	}
	rBuf.Cat(temp_buf);
	return ok;
}

int SrDatabase::SearchWord(const char * pWordUtf8, LEXID * pID)
{
	int    ok = -1;
	LEXID  lex_id = 0;
	if(P_WdT->Search(pWordUtf8, &lex_id) > 0) {
		WordCache.Add(pWordUtf8, lex_id);
		ok = 1;
	}
	ASSIGN_PTR(pID, lex_id);
	return ok;
}

int SrDatabase::SearchSpecialWord(int special, const char * pWordUtf8, LEXID * pID)
{
	int    ok = -1;
	LEXID  lex_id = 0;
	SString temp_buf;
	THROW(SrDatabase::MakeSpecialWord(special, pWordUtf8, temp_buf));
	ok = P_WdT->Search(temp_buf, &lex_id);
	if(ok > 0) {
		WordCache.Add(temp_buf, lex_id);
	}
	CATCHZOK
	ASSIGN_PTR(pID, lex_id);
	return ok;
}

int SrDatabase::FetchWord(const char * pWordUtf8, LEXID * pID)
{
	int    ok = -1;
	LEXID  lex_id = 0;
	uint   _id = 0;
	if(WordCache.Search(pWordUtf8, &_id, 0)) {
		lex_id = _id;
		ok = 1;
	}
	else if(P_WdT->Search(pWordUtf8, &lex_id) > 0) {
		WordCache.Add(pWordUtf8, lex_id);
		ok = 1;
	}
	ASSIGN_PTR(pID, lex_id);
	return ok;
}

int SrDatabase::FetchSpecialWord(int special, const char * pWordUtf8, LEXID * pID)
{
	int    ok = -1;
	LEXID  lex_id = 0;
	uint   _id = 0;
	SString temp_buf;
	THROW(SrDatabase::MakeSpecialWord(special, pWordUtf8, temp_buf));
	if(WordCache.Search(temp_buf, &_id, 0)) {
		lex_id = _id;
		ok = 1;
	}
	else if(P_WdT->Search(temp_buf, &lex_id) > 0) {
		WordCache.Add(temp_buf, lex_id);
		ok = 1;
	}
	CATCHZOK
	ASSIGN_PTR(pID, lex_id);
	return ok;
}

int SrDatabase::SearchNGram(const LongArray & rNg, NGID * pID)
{
	int    ok = -1;
	NGID   ng_id = 0;
	SrNGram ng;
	ng.WordIdList = rNg;
	if(P_NgT->Search(ng, &ng_id) > 0)
		ok = 1;
	ASSIGN_PTR(pID, ng_id);
	return ok;
}

int SrDatabase::GetNgConceptList(NGID ngID, long flags, Int64Array & rConceptList)
{
	int    r = P_CNgT->GetConceptList(ngID, rConceptList);
	if(r > 0 && flags & ngclAnonymOnly) {
		SrConcept c;
		uint i = rConceptList.getCount();
		if(i) do {
			CONCEPTID cid = rConceptList.get(--i);
			if(P_CT->SearchByID(cid, &c.Z()) > 0) {
				if(c.SymbID != 0) {
					rConceptList.atFree(i);
				}
			}
		} while(i);
		if(!rConceptList.getCount())
			r = -1;
	}
	return r;
}

int SrDatabase::Helper_GetConceptHier(int rcIdent, int firstOnly, CONCEPTID cID, Int64Array & rConceptHier)
{
	int    ok = -1;
	CONCEPTID prop_instance = (!rcIdent || rcIdent == rcInstance) ? GetReservedConcept(rcInstance) : 0;
	CONCEPTID prop_subclass = (!rcIdent || rcIdent == rcSubclass) ? GetReservedConcept(rcSubclass) : 0;
	SrCProp prop;
	if(ok < 0 && prop_instance) {
		prop.CID = cID;
		prop.PropID = prop_instance;
		if(P_CpT->Search(prop) > 0) {
			CONCEPTID cid = 0;
			THROW(prop.Get(cid));
			rConceptHier.add(cid);
			if(!firstOnly) {
				THROW(Helper_GetConceptHier(rcIdent, 0, cid, rConceptHier)); // @recursion
			}
			ok = 2;
		}
	}
	if(ok < 0 && prop_subclass) {
		prop.CID = cID;
		prop.PropID = prop_subclass;
		if(P_CpT->Search(prop) > 0) {
			CONCEPTID cid = 0;
			THROW(prop.Get(cid));
			rConceptHier.add(cid);
			if(!firstOnly) {
				THROW(Helper_GetConceptHier(rcIdent, 0, cid, rConceptHier)); // @recursion
			}
			ok = 1;
		}
	}
	CATCHZOK
	return ok;
}

int SrDatabase::GetConceptHier(CONCEPTID cID, Int64Array & rConceptHier)
{
	rConceptHier.clear();
	return Helper_GetConceptHier(0, 0, cID, rConceptHier);
}

int SrDatabase::GetFirstConceptAncestor(int rcIdent, CONCEPTID cID, CONCEPTID & rAncestor)
{
	Int64Array hier;
	int    ok = Helper_GetConceptHier(rcIdent, 1, cID, hier);
	if(hier.getCount()) {
		assert(ok > 0);
		rAncestor = hier.get(0);
	}
	else
		rAncestor = 0;
	return ok;
}

int SrDatabase::GetConceptPropList(CONCEPTID cID, SrCPropList & rPl)
{
	return P_CpT->GetPropList(cID, rPl);
}

int SrDatabase::GetPropDeclList(CONCEPTID cID, SrCPropDeclList & rPdl)
{
	int    ok = -1;
	rPdl.Z();
	Int64Array chier;
	Helper_GetConceptHier(0, 0, cID, chier);
	uint c = chier.getCount();
	if(c) do {
		CONCEPTID cid = chier.get(--c);
		SrConcept cp;
		THROW(P_CT->SearchByID(cid, &cp) > 0);
		THROW(rPdl.Merge(cp.Pdl));
	} while(c);
	if(rPdl.GetCount())
		ok = 1;
	CATCHZOK
	return ok;
}

int SrDatabase::GetConceptSymb(CONCEPTID cID, SString & rSymbUtf8)
{
	rSymbUtf8.Z();
	int    ok = -1;
	SrConcept c;
	if(P_CT->SearchByID(cID, &c) > 0) {
		if(c.SymbID) {
			THROW(P_WdT->Search(c.SymbID, rSymbUtf8));
			ok = 1;
		}
		else {
			rSymbUtf8.CatEq("Anonym", cID);
			ok = 2;
		}
	}
	CATCHZOK
	return ok;
}

int SrDatabase::Helper_MakeConceptProp(const SrCPropDeclList & rPdl, const char * pPropSymb, SrCProp & rProp, CONCEPTID cID)
{
	int    ok = 1;
	SrCPropDecl pd;
	SrConcept prop_cp;
	Int64Array chier;
	THROW(GetConceptHier(cID, chier));
	if(!isempty(pPropSymb)) {
		LEXID symb_id = 0;
		THROW(SearchSpecialWord(SrWordTbl::spcCPropSymb, pPropSymb, &symb_id) > 0);
		THROW(rPdl.GetBySymbID(symb_id, pd) > 0);
		if(chier.lsearch(pd.PropID)) {
			rProp.PropID = pd.PropID;
			rProp = cID;
		}
		else {
			ok = -1; // Значение не соответствует типу свойства
		}
	}
	else {
		uint   suited_pd_idx = 0;
		CONCEPTID prop_id = 0;
		for(uint i = 0; ok > 0 && i < rPdl.GetCount(); i++) {
			rPdl.Get(i, pd);
			if(chier.lsearch(pd.PropID)) {
				if(!suited_pd_idx) {
					suited_pd_idx = i+1;
					prop_id = pd.PropID;
				}
				else {
					ok = -1; // Неоднозначность в определении свойства концепции по значению
				}
			}
		}
		if(ok > 0) {
			if(suited_pd_idx) {
				rProp.PropID = prop_id;
				rProp = cID;
			}
			else {
				ok = -1; // Не удалось идентифицировать свойство по значению
			}
		}
	}
	CATCHZOK
	return ok;
}

int SrDatabase::MakeConceptPropN(const SrCPropDeclList & rPdl, const char * pPropSymb, SrCProp & rProp, double value)
{
	int    ok = 1;
	int   cp_type = 0;
	SrCPropDecl pd;
	SrConcept prop_cp;
	if(!isempty(pPropSymb)) {
		LEXID symb_id = 0;
		THROW(SearchSpecialWord(SrWordTbl::spcCPropSymb, pPropSymb, &symb_id) > 0);
		THROW(rPdl.GetBySymbID(symb_id, pd) > 0);
		cp_type = GetPropType(pd.PropID);
		if(cp_type == SRPROPT_REAL) {
			rProp.PropID = pd.PropID;
			rProp = value;
		}
		else if(cp_type == SRPROPT_INT) {
			if(ffrac(value) == 0.0) {
				rProp.PropID = pd.PropID;
				rProp = (int64)value;
			}
			else
				ok = -1; // Попытка установить вещественное значение для свойства, имеющего тип #int
		}
		else if(cp_type == 0)
			ok = -1; // Неопределенный тип свойства
		else
			ok = -1; // Значение не соответствует типу свойства
	}
	else {
		uint   suited_pd_idx = 0;
		CONCEPTID prop_id = 0;
		for(uint i = 0; ok > 0 && i < rPdl.GetCount(); i++) {
			rPdl.Get(i, pd);
			cp_type = GetPropType(pd.PropID);
			if(cp_type == SRPROPT_REAL) {
				if(!suited_pd_idx) {
					suited_pd_idx = i+1;
					prop_id = pd.PropID;
				}
				else {
					ok = -1; // Неоднозначность в определении свойства концепции по значению
				}
			}
		}
		if(ok > 0) {
			if(suited_pd_idx) {
				rProp.PropID = prop_id;
				rProp = value;
			}
			else {
				ok = -1; // Не удалось идентифицировать свойство по значению
			}
		}
	}
	CATCHZOK
	return ok;
}

int SrDatabase::MakeConceptPropC(const SrCPropDeclList & rPdl, const char * pPropSymb, SrCProp & rProp, const char * pConceptSymb)
{
	int    ok = 1;
	SrConcept concept;
	LEXID  cs_id = 0;
	THROW(SearchSpecialWord(SrWordTbl::spcConcept, pConceptSymb, &cs_id));
	THROW(P_CT->SearchBySymb(cs_id, &concept) > 0);
	THROW(Helper_MakeConceptProp(rPdl, pPropSymb, rProp, concept.ID) > 0);
	CATCHZOK
	return ok;
}

int SrDatabase::MakeConceptPropNg(const SrCPropDeclList & rPdl, const char * pPropSymb, SrCProp & rProp, const LongArray & rNg)
{
	int    ok = 0;
	NGID   ng_id = 0;
	THROW(SearchNGram(rNg, &ng_id) > 0);
	{
		Int64Array clist, hlist;
		THROW(GetNgConceptList(ng_id, 0, clist) > 0);
		for(uint j = 0; !ok && j < clist.getCount(); j++) {
			int r = Helper_MakeConceptProp(rPdl, pPropSymb, rProp, clist.get(j));
			THROW(r);
			if(r > 0) {
				ok = 1;
			}
		}
	}
	CATCHZOK
	return ok;
}

int SrDatabase::FormatProp(const SrCProp & rCp, long flags, SString & rBuf)
{
	rBuf.Z();
	if(rCp.PropID) {
		SString symb;
		if(GetConceptSymb(rCp.PropID, symb) > 0)
			symb.Utf8ToChar();
		else
			symb = "#unknprop#";
		rBuf.Cat(symb).CatChar('=');
		int    typ = GetPropType(rCp.PropID);
		if(typ == SRPROPT_INT) {
			int64 iv = 0;
			rCp.Get(iv);
			rBuf.Cat(iv);
		}
		else if(typ == SRPROPT_REAL) {
			double rv = 0.0;
			rCp.Get(rv);
			rBuf.Cat(rv);
		}
		else
			rBuf.Cat("#unkn#");
	}
	else
		rBuf.Cat("#zeroprop");
	return 1;
}

CONCEPTID SrDatabase::TryNgForConcept(NGID ngID, CONCEPTID targetCID, long tryOption)
{
	CONCEPTID result_id = 0;
	Int64Array concept_list; // Список концепций, соответствующих отдельной N-грамме
	GetNgConceptList(ngID, 0, concept_list);
	if(concept_list.getCount()) {
		uint   cpos = 0;
		if(tryOption == tryconceptGeneric && concept_list.lsearch(targetCID, &cpos))
			result_id = concept_list.get(cpos);
		else {
			Int64Array concept_hier_list; // Иерархия отдельной концепции
			for(uint cidx = 0; !result_id && cidx < concept_list.getCount(); cidx++) {
				const CONCEPTID cid = concept_list.get(cidx);
				CONCEPTID ancestor_id;
				switch(tryOption) {
					case tryconceptGeneric:
						GetConceptHier(cid, concept_hier_list);
						if(concept_hier_list.lsearch(targetCID))
							result_id = cid;
						break;
					case tryconceptInstance:
						if(GetFirstConceptAncestor(SrDatabase::rcInstance, cid, ancestor_id) > 0 && ancestor_id == targetCID)
							result_id = cid;
						break;
					case tryconceptSubclass:
						if(GetFirstConceptAncestor(SrDatabase::rcSubclass, cid, ancestor_id) > 0 && ancestor_id == targetCID)
							result_id = cid;
						break;
				}
			}
		}
	}
	return result_id;
}

CONCEPTID SrDatabase::TryOneWordForConcept(const char * pWord, CONCEPTID targetCID, long tryOption)
{
	CONCEPTID result_cid = 0;
	LEXID  word_id = 0;
	if(FetchWord(pWord, &word_id) > 0) {
		TSVector <SrWordAssoc> wa_list;
		TSVector <SrWordInfo> wi_list;
		GetBaseWordInfo(word_id, 0, 0, wa_list, wi_list);
		for(uint wiidx = 0; !result_cid && wiidx < wi_list.getCount(); wiidx++) {
			const SrWordInfo & r_wi = wi_list.at(wiidx);
			if(r_wi.AbbrExpID && P_NgT->Search(r_wi.AbbrExpID, 0) > 0) {
				CONCEPTID cid = TryNgForConcept(r_wi.AbbrExpID, targetCID, tryOption);
				if(cid)
					result_cid = cid;
			}
		}
		if(!result_cid) {
			NGID   ng_id = 0;
			SrNGram sng;
			sng.WordIdList.add(word_id);
			if(P_NgT->Search(sng, &ng_id) > 0) {
				CONCEPTID cid = TryNgForConcept(ng_id, targetCID, tryOption);
				if(cid)
					result_cid = cid;
			}
		}
	}
	CATCH
		result_cid = 0;
	ENDCATCH
	return result_cid;
}

CONCEPTID SrDatabase::SearchConceptInTokenizer(CONCEPTID targetCID, const STokenizer & rT, uint idxFirst, uint idxLast, long tryOption, STokenizer::ResultPosition & rR)
{
	rR.Start = 0;
	rR.Count = 0;
	SString synth_word; // Синтетическое слово для специальных случаев (сокращения вида "д.т.н.")
	CONCEPTID result_cid = 0;
	TSVector <SrWordAssoc> wa_list;
	TSVector <SrWordInfo> wi_list;
	TSVector <NGID> abbr_ng_list; // Список N-грамм, соответствующих аббревиатуре word_id (если это-таки аббревиатура)
	STokenizer::Item titem;
	STokenizer::Item titem_prev;
	STokenizer::Item titem_next;
	SrNGram sng;
	TSVector <NGID> ng_list;
	for(uint tidx = idxFirst; !result_cid && tidx <= idxLast; tidx++) {
		if(rT.Get(tidx, titem) && !(titem.Token == STokenizer::tokDelim && (titem.Text == " " || titem.Text == "\n"))) {
			LEXID  word_id = 0;
			if(titem.Token == STokenizer::tokWord) {
				wa_list.clear();
				wi_list.clear();
				{
					titem_prev = titem;
					synth_word = titem_prev.Text;
					uint   tidx2 = tidx+1;
					uint   synth_word_count = 1; // Количество атомических "слов" в синезированном слове
					for(; tidx2 <= idxLast; tidx2++) {
						if(rT.Get(tidx2, titem_next)) {
							if(titem_prev.Token == STokenizer::tokWord && titem_next.Text == ".") {
								synth_word.CatChar('.');
								titem_prev = titem_next;
								synth_word_count++;
							}
							else if(titem_next.Token == STokenizer::tokWord && titem_prev.Text == ".") {
								synth_word.Cat(titem_next.Text);
								titem_prev = titem_next;
							}
							else
								break;
						}
						else
							break;
					}
					if(synth_word_count > 1 && synth_word.NotEmpty() && synth_word != titem.Text) {
						LEXID synth_word_id = 0;
						if(FetchWord(synth_word, &synth_word_id) > 0) {
							GetBaseWordInfo(synth_word_id, 0, 0, wa_list, wi_list);
							for(uint wiidx = 0; !result_cid && wiidx < wi_list.getCount(); wiidx++) {
								const SrWordInfo & r_wi = wi_list.at(wiidx);
								if(r_wi.AbbrExpID && P_NgT->Search(r_wi.AbbrExpID, 0) > 0) {
									result_cid = TryNgForConcept(r_wi.AbbrExpID, targetCID, tryOption);
									rR.Start = tidx;
									rR.Count = tidx2-tidx;
								}
							}
						}
					}
				}
				if(!result_cid && FetchWord(titem.Text, &word_id) > 0) {
					GetBaseWordInfo(word_id, 0, 0, wa_list, wi_list);
					for(uint wiidx = 0; !result_cid && wiidx < wi_list.getCount(); wiidx++) {
						const SrWordInfo & r_wi = wi_list.at(wiidx);
						if(r_wi.AbbrExpID && P_NgT->Search(r_wi.AbbrExpID, 0) > 0) {
							CONCEPTID cid = TryNgForConcept(r_wi.AbbrExpID, targetCID, tryOption);
							if(cid) {
								result_cid = cid;
								rR.Start = tidx;
								rR.Count = 1;
								// Мы нашли соответствие по аббревиатуре. Пропускаем последующую точку, если она есть.
								// Здесь неточность: точку надо пропускать только если аббревиатура это предполагает, но пока так.
								/*if(rB.R_T.Get(rB.TextIdx, rB.TItemBuf) && rB.TItemBuf.Text == ".") {
									rB.TextIdx++;
								}*/
							}
						}
					}
					if(!result_cid) {
						ng_list.clear();
						sng.WordIdList.Z().add(word_id);
						if(P_NgT->SearchByPrefix(sng, ng_list) > 0) {
							SrNGramCollection sng_list;
							for(uint i = 0; i < ng_list.getCount(); i++) {
								SrNGram * p_ng = sng_list.CreateNewItem();
								THROW_SL(p_ng);
								THROW(P_NgT->Search(ng_list.at(i), p_ng) > 0); // Не может такого быть, что этой n-граммы не было (мы только что ее нашли)
							}
							sng_list.SortByLength();
							const  uint local_preserve_tidx = tidx;
							uint   _c = sng_list.getCount();
							if(_c) do {
								SrNGram * p_ng = sng_list.at(--_c);
								THROW(p_ng->WordIdList.get(0) == word_id);
								int    is_match = 1;
								tidx = local_preserve_tidx;
								for(uint j = 1 /*! 0-й элемент уже проверен*/; is_match && j < p_ng->WordIdList.getCount(); j++) {
									const LEXID ng_word_id = p_ng->WordIdList.get(j);
									if(rT.Get(++tidx, titem)) {
										if(titem.Token == STokenizer::tokWord) {
											LEXID next_word_id = 0;
											if(FetchWord(titem.Text, &next_word_id) > 0 && next_word_id == ng_word_id) {
												; // ok
											}
											else
												is_match = 0; // break loop
										}
										else if(titem.Token == STokenizer::tokDelim) {
											if(titem.Text == " " || titem.Text == "\t" || titem.Text == "\n")
												continue;
											else
												is_match = 0; // break loop
										}
									}
								}
								if(is_match) {
									CONCEPTID cid = TryNgForConcept(p_ng->ID, targetCID, tryOption);
									if(cid) {
										assert(tidx >= local_preserve_tidx);
										result_cid = cid;
										rR.Start = local_preserve_tidx;
										rR.Count = (tidx - local_preserve_tidx + 1);
									}
								}
							} while(!result_cid && _c);
							tidx = local_preserve_tidx;
						}
					}
				}
			}
		}
	}
	CATCH
		result_cid = 0;
	ENDCATCH
	return result_cid;
}

int SrDatabase::SetConceptProp(CONCEPTID cID, CONCEPTID propID, long flags, int64 propVal)
{
	int    ok = 1;
	THROW(P_CpT);
	if(cID && propID) {
		SrCProp cp(cID, propID);
		cp = propVal;
		THROW(P_CpT->Set(cp));
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

int SrDatabase::SetConceptProp(CONCEPTID cID, CONCEPTID propID, long flags, int propVal)
{
	int    ok = 1;
	THROW(P_CpT);
	if(cID && propID) {
		SrCProp cp(cID, propID);
		cp = propVal;
		THROW(P_CpT->Set(cp));
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

int SrDatabase::SetConceptProp(CONCEPTID cID, CONCEPTID propID, long flags, double propVal)
{
	int    ok = 1;
	THROW(P_CpT);
	if(cID && propID) {
		SrCProp cp(cID, propID);
		cp = propVal;
		THROW(P_CpT->Set(cp));
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

int SrDatabase::StoreGeoNodeList(const TSVector <PPOsm::Node> & rList, const LLAssocArray * pNodeToWayAsscList, int dontCheckExist, TSVector <PPOsm::NodeClusterStatEntry> * pStat)
{
	const  uint max_cluster_per_tx = 1024;
	const  int  use_transaction = 1;
	//const  int  use_outer_id = 1;
	int    ok = 1;
	const  uint _count = rList.getCount();
	if(_count) {
		TSCollection <PPOsm::NodeCluster> cluster_list;
		TSVector <uint64> outer_id_list; // @v9.8.4 TSArray-->TSVector
		TSVector <PPOsm::Node> test_list; // @v9.8.4 TSArray-->TSVector
		PPOsm::NodeCluster ex_cluster;
		TSVector <PPOsm::Node> ex_list; // @v9.8.4 TSArray-->TSVector
		uint   next_node_to_way_assc_pos = 0;
		size_t offs = 0;
		{
			while(offs < _count) {
				uint   actual_count_ = 0;
				const  PPOsm::Node * p_node = &rList.at(offs);
                uint64 fault_logical_id = 0;
                PPOsm::Node found_node;
				//int   sr = P_GnT->Search(p_node->ID, &found_node, 0, &fault_logical_id);
				int   sr = dontCheckExist ? -1 : P_GnT->Search(p_node->ID, &ex_cluster, &fault_logical_id);
				THROW(sr);
				if(sr > 0) {
					PROFILE_START
					{
						uint   ex_count_logic = 0;
						uint   ex_count_actual = 0;
						ex_list.clear();
						const  int ecgr = ex_cluster.Get(fault_logical_id, ex_list, 0, 0, &ex_count_logic, &ex_count_actual);
						assert(ecgr);
						for(uint   forward_idx = offs; forward_idx < _count; forward_idx++) {
							uint64 forward_id = rList.at(forward_idx).ID;
							uint   fpos = 0;
							if(ex_list.lsearch(&forward_id, &fpos, CMPF_INT64)) {
								assert(rList.at(forward_idx) == ex_list.at(fpos));
								if(pStat) {
									PPOsm::SetProcessedNodeStat((uint)ex_count_logic, 1, *pStat);
								}
								actual_count_++;
							}
							else {
								assert(actual_count_);
								if(!actual_count_)
									sr = -1;
								break;
							}
						}
					}
					PROFILE_END
				}
				if(sr < 0) {
					assert(fault_logical_id == 0);
					uint64 outer_id = 0;
					PPOsm::NodeCluster::Put__Param  clu_put_param(p_node, _count - offs);
					PPOsm::NodeCluster::Put__Result clu_put_result;
					PPOsm::NodeCluster * p_cluster = cluster_list.CreateNewItem();
					THROW(p_cluster);
					if(pNodeToWayAsscList && next_node_to_way_assc_pos < pNodeToWayAsscList->getCount()) {
						clu_put_param.P_NrWayRefs = &pNodeToWayAsscList->at(next_node_to_way_assc_pos);
						clu_put_param.NrWayRefsCount = pNodeToWayAsscList->getCount() - next_node_to_way_assc_pos;
					}
					PROFILE_START
					THROW(p_cluster->Put__(clu_put_param, &outer_id, &clu_put_result, 0));
					THROW(outer_id_list.insert(&outer_id));
					PROFILE_END
					actual_count_ = clu_put_result.ActualCount;
					assert(outer_id_list.getCount() == cluster_list.getCount());
					next_node_to_way_assc_pos += clu_put_result.NrWayShift;
					if(pStat) {
						PROFILE(PPOsm::SetNodeClusterStat(*p_cluster, *pStat));
					}
					if(0) { // @debug
						test_list.clear();
						p_cluster->Get(outer_id, test_list, 0 /*NodeRefs*/);
						for(uint i = 0; i < test_list.getCount(); i++) {
							assert(test_list.at(i) == p_node[i]);
						}
					}
				}
				offs += actual_count_;
				if(cluster_list.getCount() >= max_cluster_per_tx) {
					assert(outer_id_list.getCount() == cluster_list.getCount());
					PROFILE_START
					BDbTransaction tra(P_Db, use_transaction);
					THROW(tra);
					for(uint i = 0; i < cluster_list.getCount(); i++) {
						PPOsm::NodeCluster * p_item = cluster_list.at(i);
						uint64 local_outer_id = outer_id_list.at(i);
						THROW(P_GnT->Add(*p_item, local_outer_id));
					}
					THROW(tra.Commit(1));
					PROFILE_END
					PROFILE(cluster_list.clear());
					outer_id_list.clear();
				}
			}
			assert(offs == _count);
			if(cluster_list.getCount()) {
				assert(outer_id_list.getCount() == cluster_list.getCount());
				PROFILE_START
				BDbTransaction tra(P_Db, use_transaction);
				THROW(tra);
				for(uint i = 0; i < cluster_list.getCount(); i++) {
					PPOsm::NodeCluster * p_item = cluster_list.at(i);
					uint64 local_outer_id = outer_id_list.at(i);
					THROW(P_GnT->Add(*p_item, local_outer_id));
				}
				THROW(tra.Commit(1));
				PROFILE_END
				PROFILE(cluster_list.freeAll());
			}
		}
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

int SrDatabase::StoreGeoNodeWayRefList(const LLAssocArray & rList)
{
	const  uint max_entries_per_tx = 1024;
	const  int  use_transaction = 1;
	int    ok = 1;
	const  uint _count = rList.getCount();
	if(_count) {
		int64 prev_node_id = 0;
		PPOsm::NodeCluster nc;
		TSVector <PPOsm::Node> node_list; // @v9.8.4 TSArray-->TSVector
		TSVector <PPOsm::Node> node_list_test;
		PPOsm::NodeRefs ref_list;
		PPOsm::NodeRefs ref_list_test;
		for(uint _current_pos = 0; _current_pos < _count;) {
			PROFILE_START
			BDbTransaction tra(P_Db, use_transaction);
			THROW(tra);
			const uint _local_finish = MIN(_count, (_current_pos + max_entries_per_tx));
			for(uint i = _current_pos; i < _local_finish;) {
				const LLAssoc & r_assoc = rList.at(i);
				assert(!i || r_assoc.Key >= prev_node_id); // Тест на упорядоченность входящего массива
				uint64 _logical_id = 0;
				ref_list.Clear();
				node_list.clear();
				const  int sr = P_GnT->Search(r_assoc.Key, &nc, &_logical_id);
				THROW(sr);
				i++;
				if(sr > 0) {
					uint   _force_logical_count = 0;
					uint   _count_actual = 0;
					THROW(nc.Get(_logical_id, node_list, &ref_list, 0, &_force_logical_count, &_count_actual));
					THROW(ref_list.AddWayRef(r_assoc.Key, r_assoc.Val));
					while(i < _count) {
						const LLAssoc & r_assoc_next = rList.at(i);
						uint   nl_pos = 0;
						if(node_list.lsearch(&r_assoc_next.Key, &nl_pos, CMPF_INT64)) {
							THROW(ref_list.AddWayRef(r_assoc_next.Key, r_assoc_next.Val));
							i++;
						}
						else
							break;
					}
					{
						uint64 _outer_id = _logical_id;
						PPOsm::NodeCluster::Put__Param  clu_put_param(&node_list.at(0), node_list.getCount());
						PPOsm::NodeCluster::Put__Result clu_put_result;
						ref_list.Sort();
						clu_put_param.P_NrWayRefs = &ref_list.WayRefs.at(0);
						clu_put_param.NrWayRefsCount = ref_list.WayRefs.getCount();
						clu_put_param.P_NrRelRefs = &ref_list.RelRefs.at(0);
						clu_put_param.NrRelRefsCount = ref_list.RelRefs.getCount();
						THROW(nc.Put__(clu_put_param, &_outer_id, &clu_put_result, _force_logical_count));
						assert(_outer_id == _logical_id);
						assert(clu_put_result.ActualCount == node_list.getCount());
					}
					{
						//
						// Test
						//
						ref_list_test.Clear();
						node_list_test.clear();
						THROW(nc.Get(_logical_id, node_list_test, &ref_list_test));
						assert(node_list_test.IsEqual(node_list));
						assert(ref_list_test.IsEqual(ref_list));

					}
					THROW(P_GnT->Update(nc, _logical_id));
				}
				else {
					// @todo log message (точка не найдена)
				}
			}
			_current_pos = _local_finish;
			THROW(tra.Commit(1));
			PROFILE_END
		}
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

int SrDatabase::StoreGeoWayList(const TSCollection <PPOsm::Way> & rList, TSVector <PPOsm::WayStatEntry> * pStat)
{
	const  uint max_entries_per_tx = 1024;
	const  int  use_transaction = 1;
	int    ok = 1;
	const  uint _count = rList.getCount();
	if(_count) {
		PPOsm::WayBuffer way_buf;
        for(uint _current_pos = 0; _current_pos < _count;) {
			PROFILE_START
			BDbTransaction tra(P_Db, use_transaction);
			THROW(tra);
			const uint _local_finish = MIN(_count, (_current_pos + max_entries_per_tx));
			for(uint i = _current_pos; i < _local_finish; i++) {
				PPOsm::Way * p_way = rList.at(i);
				assert(p_way);
				if(p_way) {
					PPOsm::Way found_way;
					if(P_GwT->Search(p_way->ID, &found_way) > 0) {
						assert(found_way == *p_way);
						if(pStat) {
							PPOsm::SetProcessedWayStat(found_way.NodeRefs.getCount(), 1, *pStat);
						}
					}
					else {
						THROW(P_GwT->Add(*p_way, &way_buf));
						if(pStat) {
							PPOsm::SetWayStat(way_buf, *pStat);
						}
					}
				}
			}
			_current_pos = _local_finish;
			THROW(tra.Commit(1));
			PROFILE_END
		}
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

struct StoreFiasAddrBlock {
	class TextHash : public SymbHashTable {
	public:
		enum {
			fUndefType = 0x0001, // Символ не удалось идентифицировать как тип объекта
			fAmbigType = 0x0002  // Символ не однозначно идентифицируется как тип объекта
		};
		struct LocTypeEntry {
			LocTypeEntry() : FiasLevel(0), Flags(0), SrTypeID(0)
			{
			}
			uint16  FiasLevel;
			uint16  Flags;
			CONCEPTID SrTypeID;
		};
		struct ExtData {
			ExtData()
			{
			}
			int    SetEntryByFiasLevel(uint level, uint16 flags, CONCEPTID srTypeID)
			{
				int    ok = 0;
				uint   first_empty_slot_incd = 0;
				for(uint i = 0; i < SIZEOFARRAY(LTL); i++) {
					if(LTL[i].FiasLevel == level) {
						LTL[i].Flags |= flags;
						LTL[i].SrTypeID = srTypeID;
						ok = 1;
					}
					else if(LTL[i].FiasLevel == 0 && !first_empty_slot_incd)
						first_empty_slot_incd = i+1;
				}
				if(!ok && first_empty_slot_incd) {
					LTL[first_empty_slot_incd-1].FiasLevel |= level;
					LTL[first_empty_slot_incd-1].Flags |= flags;
					LTL[first_empty_slot_incd-1].SrTypeID = srTypeID;
					ok = 1;
				}
				assert(ok);
				return ok;
			}
			const  LocTypeEntry * FASTCALL GetEntryByFiasLevel(uint level) const
			{
				for(uint i = 0; i < SIZEOFARRAY(LTL); i++) {
					if(LTL[i].FiasLevel == level)
						return &LTL[i];
				}
				return 0;
			}
			LocTypeEntry LTL[8];
		};
		TextHash() : SymbHashTable(SKILOBYTE(8))
		{
		}
		int    Add(const char * pSymb, const ExtData * pData)
		{
			assert(pData);
			int    ok = -1;
			ExtData ex_blk;
			uint   ex_blk_pos = 0;
			if(SymbHashTable::Search(pSymb, &ex_blk_pos, 0)) {
				assert(ex_blk_pos < ExtVect.getCount());
				ExtVect.at(ex_blk_pos) = *pData;
				ok = 2;
			}
			else {
				uint   pos = ExtVect.getCount();
				THROW_SL(ExtVect.insert(pData));
				THROW_SL(SymbHashTable::Add(pSymb, pos));
				ok = 1;
			}
			CATCHZOK
			return ok;
		}
		int    Search(const char * pSymb, ExtData * pData) const
		{
			int    ok = 0;
			uint   ex_blk_pos = 0;
			if(SymbHashTable::Search(pSymb, &ex_blk_pos, 0)) {
				assert(ex_blk_pos < ExtVect.getCount());
				ASSIGN_PTR(pData, ExtVect.at(ex_blk_pos));
				ok = 1;
			}
			return ok;
		}
	private:
		TSVector <ExtData> ExtVect;
	};

	static const uint SignatureValue;
	StoreFiasAddrBlock() : Signature(SignatureValue), CRegion(0), CUrbs(0), CVici(0), CAedificium(0), CApartment(0), CRussia(0), ProcessedSymbols(4096, 1)
	{
	}
	const uint32 Signature;
	CONCEPTID CRegion;
	CONCEPTID CUrbs;
	CONCEPTID CVici;
	CONCEPTID CAedificium;
	CONCEPTID CApartment;
	CONCEPTID CRussia; // Концепция, представляющая государство Россия (:statu_ru)
	TSVector <Sdr_FiasRawAddrObj> SrcList;
	SymbHashTable ProcessedSymbols;
	//STokenizer T;
	TextHash   H;
};

//static 
const uint StoreFiasAddrBlock::SignatureValue = 0x43a5e9b7;

void * SrDatabase::CreateStoreFiasAddrBlock()
{
	StoreFiasAddrBlock * p_blk = new StoreFiasAddrBlock;
	if(p_blk) {
		assert(p_blk->Signature == StoreFiasAddrBlock::SignatureValue);
		SearchConcept("cregion", &p_blk->CRegion);
		SearchConcept("urbs", &p_blk->CUrbs);
		SearchConcept("gener_vici", &p_blk->CVici);
		SearchConcept("gener_aedificium", &p_blk->CAedificium);
		SearchConcept("gener_apartment", &p_blk->CApartment);
		SearchConcept("statu_ru", &p_blk->CRussia);
	}
	return p_blk;
}

void SrDatabase::DestroyStoreFiasAddrBlock(void * pBlk)
{
	StoreFiasAddrBlock * p_blk = (StoreFiasAddrBlock *)pBlk;
	if(p_blk && p_blk->Signature == StoreFiasAddrBlock::SignatureValue)
		delete p_blk;
}

static IMPL_CMPFUNC(Sdr_FiasRawAddrObj_AOGUID_LIVESTATUS, p1, p2)
{
	const Sdr_FiasRawAddrObj * p_item1 = (const Sdr_FiasRawAddrObj *)p1;
	const Sdr_FiasRawAddrObj * p_item2 = (const Sdr_FiasRawAddrObj *)p2;
	int   s = memcmp(&p_item1->AOGUID, &p_item2->AOGUID, sizeof(p_item1->AOGUID));
	SETIFZ(s, CMPSIGN(p_item2->LIVESTATUS, p_item1->LIVESTATUS)); // descending order
	return s;
}

int SrDatabase::ResolveNgFromTokenizer(const STokenizer & rTknz, uint idxFirst, uint idxCount, const SrWordForm * pWfToSet, long options, SrNGram & rNg)
{
	int    ok = -1;
	//uint   idx_first = 0;
	//uint   idx_count = 0;
	const  uint wftoset_count = pWfToSet ? pWfToSet->GetTagCount() : 0;
	STokenizer::Item titem, titem_next;
	rNg.Z();
	for(uint tidx = idxFirst; tidx < idxCount; tidx++) {
		LEXID word_id = 0;
		if(rTknz.Get(idxFirst+tidx, titem)) {
			if(titem.Token == STokenizer::tokWord) {
				if(options & rngftoSkipDotAfterWord) {
					if(tidx < (idxCount-1) && rTknz.Get(idxFirst+tidx+1, titem_next)) {
						if(titem_next.Token == STokenizer::tokDelim && titem_next.Text.Single() == '.')
							tidx++; // Следующую за словом точку пропускаем (например "а.невского"-->"а" "невского")
					}
				}
				if(FetchWord(titem.Text, &word_id) < 0) {
					THROW(P_WdT->Add(titem.Text, &word_id));
					if(wftoset_count) {
						// Было создано новое слово - добавим к нему известные нам признаки (пока только язык)
						THROW(SetSimpleWordFlexiaModel(word_id, *pWfToSet, 0));
					}
				}
				assert(word_id);
			}
			else if(titem.Token == STokenizer::tokDelim) {
				const char dc = titem.Text.Single();
				if(oneof4(dc, ' ', '\t', '\n', '\r'))
					; // просто пропускаем
				else if(titem.Text.Len()) {
					if(FetchWord(titem.Text, &word_id) < 0) {
						THROW(P_WdT->Add(titem.Text, &word_id));
					}
					assert(word_id);
				}
			}
		}
		rNg.WordIdList.addnz(word_id);
	}
	if(rNg.WordIdList.getCount()) {
		NGID   ngram_id = 0;
		ok = ResolveNGram(rNg.WordIdList, &ngram_id);
		THROW(ok);
		rNg.ID = ngram_id;
		assert(ngram_id);
	}
	CATCHZOK
	return ok;
}

int SrDatabase::StoreFiasAddr(void * pBlk, uint passN, const Sdr_FiasRawAddrObj * pEntry)
{
	const  uint max_entries_per_tx = 256;
	assert(oneof2(passN, 1, 2));
	int    ok = 1;
	BDbTransaction * p_ta = 0;
	StoreFiasAddrBlock * p_blk = (StoreFiasAddrBlock *)pBlk;
	assert(p_blk && p_blk->Signature == StoreFiasAddrBlock::SignatureValue);
	THROW(p_blk && p_blk->Signature == StoreFiasAddrBlock::SignatureValue);
	if(pEntry) {
		THROW_SL(p_blk->SrcList.insert(pEntry));
	}
	const uint src_list_count = p_blk->SrcList.getCount();
	if(!pEntry || src_list_count >= SKILOBYTE(128)) {
		uint   cur_entries_per_tx = 0; // Текущее количество элементов, обработанных внутри одной транзакции
		SString temp_buf;
		SString msg_buf;
		SString main_concept_symb;
		SString text;
		StringSet ss;
		StringSet ss_name; // Коллекция вариантов наименований 
		SrWordForm wordform;
		TSVector <SrWordInfo> info_list;
		STokenizer::Item titem, titem_next;
		Int64Array concept_list;
		Int64Array candidate_concept_list;
		Int64Array local_concept_list;
		Int64Array concept_hier;
		SrNGram ng;
		STokenizer tknz(STokenizer::Param(STokenizer::fEachDelim, cpUTF8, " \t\n\r(){}[]<>,.:;\\/&$#@!?*^\"+=%")); // "-" здесь не является разделителем
		// Сортируем входной массив так, чтобы все элементы с одинаковыми AOGUID были вместе и актуальный (LIVESTATUS=1) был самым первым
		p_blk->SrcList.sort(PTR_CMPFUNC(Sdr_FiasRawAddrObj_AOGUID_LIVESTATUS));
		for(uint lidx = 0; lidx < src_list_count; lidx++) {
			const Sdr_FiasRawAddrObj & r_item = p_blk->SrcList.at(lidx);
			/*
				struct Sdr_FiasRawAddrObj
					S_GUID AOGUID;
					char   FORMALNAME[128];
					char   REGIONCODE[16];
					char   AUTOCODE[16];
					char   AREACODE[16];
					char   CITYCODE[16];
					char   CTARCODE[16];
					char   PLACECODE[16];
					char   STREETCODE[16];
					char   EXTRCODE[16];
					char   SEXTCODE[16];
					char   OFFNAME[128];
					char   POSTALCODE[16];
					char   IFNSFL[16];
					char   TERRIFNSFL[16];
					char   IFNSUL[16];
					char   TERRIFNSUL[16];
					char   OKATO[16];
					char   OKTMO[16];
					LDATE  UPDATEDATE;
					char   SHORTNAME[16]; // сокращение сущности (г, ул, etc)
					char   AOLEVEL[16];   // 
					S_GUID PARENTGUID;
					S_GUID AOID;
					S_GUID PREVID;
					S_GUID NEXTID;
					char   CODE[20];
					char   PLAINCODE[20];
					int32  ACTSTATUS;
					int32  CENTSTATUS;
					int32  OPERSTATUS;
					int32  CURRSTATUS;
					LDATE  STARTDATE;
					LDATE  ENDDATE;
					char   NORMDOC[40];
					int32  LIVESTATUS;
			*/
			CONCEPTID parent_cid = 0; // Концепция, экземпляром которой является создаваемая запись
			const int is_actual = r_item.LIVESTATUS;
			/*
				Условно выделены следующие уровни адресных объектов:
				1 – уровень региона
				2 – уровень автономного округа (устаревшее)
				3 – уровень района
				35 – уровень городских и сельских поселений
				4 – уровень города
				5 – уровень внутригородской территории (устаревшее)
				6 – уровень населенного пункта
				65 – планировочная структура
				7 – уровень улицы
				75 – земельный участок
				8 – здания, сооружения, объекта незавершенного строительства
				9 – уровень помещения в пределах здания, сооружения
				90 – уровень дополнительных территорий (устаревшее)
				91 – уровень объектов на дополнительных территориях (устаревшее)
			*/
			const int aolevel = atoi(r_item.AOLEVEL);
			/*
				:cregion
				:urbs
				:gener_vici
				:gener_aedificium
				:gener_apartment
			*/
			(text = r_item.SHORTNAME).Strip().ToLower1251().Transf(CTRANSF_OUTER_TO_UTF8);
			(temp_buf = r_item.FORMALNAME).Strip().ToLower1251().Transf(CTRANSF_OUTER_TO_UTF8);
			const SString name_buf = temp_buf;
			uint   last_identical_idx = lidx; // Индекс последнего элемента идентичного по AOGUID r_item
			{
				// ss_name будет хранить список имен объекта, отличающихся от name_buf
				ss_name.clear();
				ss_name.add(name_buf);
				(temp_buf = r_item.OFFNAME).Strip().ToLower1251().Transf(CTRANSF_OUTER_TO_UTF8);
				if(temp_buf.NotEmpty() && temp_buf != name_buf)
					ss_name.add(temp_buf);
				for(uint inner_lidx = lidx+1; inner_lidx < src_list_count; inner_lidx++) {
					const Sdr_FiasRawAddrObj & r_next_item = p_blk->SrcList.at(inner_lidx);
					if(r_next_item.AOGUID == r_item.AOGUID) {
						(temp_buf = r_next_item.FORMALNAME).Strip().ToLower1251().Transf(CTRANSF_OUTER_TO_UTF8);
						if(temp_buf.NotEmpty() && temp_buf != name_buf)
							ss_name.add(temp_buf);
						(temp_buf = r_next_item.OFFNAME).Strip().ToLower1251().Transf(CTRANSF_OUTER_TO_UTF8);
						if(temp_buf.NotEmpty() && temp_buf != name_buf)
							ss_name.add(temp_buf);
						//
						last_identical_idx = inner_lidx;
					}
					else
						break;
				}
			}
			ng.Z();
			info_list.clear();
			concept_list.clear();
			candidate_concept_list.clear();
			local_concept_list.clear();
			concept_hier.clear();
			text.TrimRightChr('.').Strip();

			CONCEPTID sr_type_cid = 0; // ИД концепции-типа локации (город, улица, поселок и т.д.)
			CONCEPTID main_cid = 0; // Ид концепции, соответствующей текущей записи (AOGUID)
			StoreFiasAddrBlock::TextHash::ExtData hed;
			StoreFiasAddrBlock::TextHash::LocTypeEntry __lte;
			if(!p_ta) {
				THROW_MEM(p_ta = new BDbTransaction(this->P_Db, 1));
			}
			if(cur_entries_per_tx++ >= max_entries_per_tx) {
				THROW_DB(p_ta->Commit(1));
				THROW_DB(p_ta->Start(1));
				cur_entries_per_tx = 0;
			}
			SrConcept::MakeSurrogateSymb(SrConcept::surrsymbsrcFIAS, &r_item.AOGUID, sizeof(r_item.AOGUID), main_concept_symb);
			if(passN == 2) {
				if(PropHMember) {
					if(SearchConcept(main_concept_symb, &main_cid) > 0) {
						CONCEPTID parent_cid = 0;
						if(!r_item.PARENTGUID.IsZero()) {
							SrConcept::MakeSurrogateSymb(SrConcept::surrsymbsrcFIAS, &r_item.PARENTGUID, sizeof(r_item.PARENTGUID), temp_buf);
							SearchConcept(temp_buf, &parent_cid);
						}
						else {
							parent_cid = p_blk->CRussia;
						}
						if(parent_cid) {
							THROW(SetConceptProp(main_cid, PropHMember, 0, parent_cid));
						}
					}
				}
			}
			else if(passN == 1) {
				//
				// Специальная обработка ситуаций, вызывающих неоднозначность
				// Note: этот исходный файл - в кодировке UTF-8
				//
				if(text == "п") {
					if(aolevel == 3) {
						text = "поселение";
					}
					else {
						text = "поселок";
					}
				}
				if(p_blk->H.Search(text, &hed)) {
					const StoreFiasAddrBlock::TextHash::LocTypeEntry * p_lte = hed.GetEntryByFiasLevel(aolevel);
					RVALUEPTR(__lte, p_lte);
				}
				if(__lte.SrTypeID) {
					assert(__lte.FiasLevel == aolevel);
					sr_type_cid = __lte.SrTypeID;
				}
				else if(__lte.FiasLevel && __lte.Flags & StoreFiasAddrBlock::TextHash::fUndefType) {
					;
				}
				else if(__lte.FiasLevel && __lte.Flags & StoreFiasAddrBlock::TextHash::fAmbigType) {
					;
				}
				else {
					assert(__lte.FiasLevel == 0);
					LEXID single_word_id = 0;
					if(FetchWord(text, &single_word_id) > 0) {
						ng.Z();
						ng.WordIdList.add(single_word_id);
						NGID local_ng_id = 0;
						if(P_NgT->Search(ng, &local_ng_id) > 0) {
							local_concept_list.clear();
							GetNgConceptList(local_ng_id, 0, local_concept_list);
							concept_list.add(&local_concept_list);
						}
					}
					if(GetWordInfo(text, 0, info_list) > 0) {
						for(uint i = 0; i < info_list.getCount(); i++) {
							const SrWordInfo & r_wi = info_list.at(i);
							local_concept_list.clear();
							if(r_wi.AbbrExpID) {
								if(P_NgT->Search(r_wi.AbbrExpID, &ng) > 0) {
									GetNgConceptList(ng.ID, 0, local_concept_list);
									concept_list.add(&local_concept_list);
								}
							}
						}
					}
					candidate_concept_list.clear();
					concept_list.sortAndUndup();
					for(uint cidx = 0; cidx < concept_list.getCount(); cidx++) {
						CONCEPTID item_id = concept_list.get(cidx);
						if(GetConceptHier(item_id, concept_hier) > 0) {
							assert(concept_hier.getCount());
							const CONCEPTID first_hier_cid = concept_hier.get(0);
							switch(aolevel) {
								case 1: 
									if(first_hier_cid == p_blk->CUrbs) {
										candidate_concept_list.add(item_id);
										break;
									}
									// @fallthrough
								case 2: 
								case 3:
									if(first_hier_cid == p_blk->CRegion)
										candidate_concept_list.add(item_id);
									break;
								case 4: case 6:
									if(first_hier_cid == p_blk->CUrbs)
										candidate_concept_list.add(item_id);
									break;
								case 7:
									if(first_hier_cid == p_blk->CVici)
										candidate_concept_list.add(item_id);
									break;
								case 8:
									if(first_hier_cid == p_blk->CAedificium)
										candidate_concept_list.add(item_id);
									break;
								case 9:
									if(first_hier_cid == p_blk->CApartment)
										candidate_concept_list.add(item_id);
									break;
							}
						}
					}
					candidate_concept_list.sortAndUndup();
					{
						const StoreFiasAddrBlock::TextHash::LocTypeEntry * p_lte = hed.GetEntryByFiasLevel(aolevel);
						RVALUEPTR(__lte, p_lte);
						assert(!__lte.FiasLevel || __lte.FiasLevel == aolevel);
						__lte.FiasLevel = aolevel;
						if(candidate_concept_list.getCount() == 0) {
							(temp_buf = r_item.SHORTNAME).Transf(CTRANSF_OUTER_TO_INNER);
							PPLogMessage(PPFILNAM_IMPEXP_LOG, PPFormatS(PPSTR_TEXT, PPTXT_FIASTOSR_UNKNOBJTYPE, &msg_buf, temp_buf.cptr(), aolevel), /*LOGMSGF_TIME*/0);
							THROW(hed.SetEntryByFiasLevel(aolevel, StoreFiasAddrBlock::TextHash::fUndefType, 0));
						}
						else if(candidate_concept_list.getCount() > 1) {
							(temp_buf = r_item.SHORTNAME).Transf(CTRANSF_OUTER_TO_INNER);
							PPLogMessage(PPFILNAM_IMPEXP_LOG, PPFormatS(PPSTR_TEXT, PPTXT_FIASTOSR_AMBIGOBJTYPE, &msg_buf, temp_buf.cptr(), aolevel), /*LOGMSGF_TIME*/0);
							for(uint ai = 0; ai < candidate_concept_list.getCount(); ai++) {
								CONCEPTID ccid = candidate_concept_list.get(ai);
								GetConceptSymb(ccid, temp_buf);
								msg_buf.Z().Tab().Cat(temp_buf);
								if(GetConceptHier(ccid, concept_hier) > 0) {
									for(uint hi = 0; hi < concept_hier.getCount(); hi++) {
										GetConceptSymb(concept_hier.get(hi), temp_buf);
										msg_buf.Cat("->").Cat(temp_buf);
									}
								}
								msg_buf.Transf(CTRANSF_UTF8_TO_INNER);
								PPLogMessage(PPFILNAM_IMPEXP_LOG, msg_buf, 0);
							}
							THROW(hed.SetEntryByFiasLevel(aolevel, StoreFiasAddrBlock::TextHash::fAmbigType, 0));
						}
						else { // candidate_concept_list.getCount() exactly equal 1
							sr_type_cid = candidate_concept_list.get(0);
							THROW(hed.SetEntryByFiasLevel(aolevel, 0, sr_type_cid));
						}
						THROW(p_blk->H.Add(text, &hed));
					}
				}
				if(sr_type_cid) {
					wordform.Clear();
					wordform.SetTag(SRWG_LANGUAGE, slangRU);
					THROW(ResolveConcept(main_concept_symb, &main_cid));
					for(uint namessp = 0; ss_name.get(&namessp, text);) {
						uint   idx_first = 0;
						uint   idx_count = 0;
						tknz.RunSString(0, 0, text, &idx_first, &idx_count);
						// @v10.0.11 {
						const int gngftr = ResolveNgFromTokenizer(tknz, idx_first, idx_count, &wordform, rngftoSkipDotAfterWord, ng);
						THROW(gngftr);
						if(gngftr > 0) {
							assert(ng.ID != 0);
							THROW(P_CNgT->Set(main_cid, ng.ID));
							{
								assert(PropInstance);
								assert(sr_type_cid);
								THROW(SetConceptProp(main_cid, PropInstance, 0, sr_type_cid));
							}
						}
						// } @v10.0.11 
						/* @v10.0.11 
						ng.Z();
						for(uint tidx = 0; tidx < idx_count; tidx++) {
							LEXID word_id = 0;
							if(tknz.Get(idx_first+tidx, titem)) {
								if(titem.Token == STokenizer::tokWord) {
									if(tidx < (idx_count-1) && tknz.Get(idx_first+tidx+1, titem_next)) {
										if(titem_next.Token == STokenizer::tokDelim && titem_next.Text.Single() == '.')
											tidx++; // Следующую за словом точку пропускаем (например "а.невского"-->"а" "невского")
									}
									if(FetchWord(titem.Text, &word_id) < 0) {
										THROW(P_WdT->Add(titem.Text, &word_id));
										// Было создано новое слово - добавим к нему известные нам признаки (пока только язык)
										wordform.Clear();
										wordform.SetTag(SRWG_LANGUAGE, slangRU);
										THROW(SetSimpleWordFlexiaModel(word_id, wordform, 0));
									}
									assert(word_id);
								}
								else if(titem.Token == STokenizer::tokDelim) {
									const char dc = titem.Text.Single();
									if(oneof4(dc, ' ', '\t', '\n', '\r'))
										; // просто пропускаем
									else if(titem.Text.Len()) {
										if(FetchWord(titem.Text, &word_id) < 0) {
											THROW(P_WdT->Add(titem.Text, &word_id));
										}
										assert(word_id);
									}
								}
							}
							ng.WordIdList.addnz(word_id);
						}
						if(ng.WordIdList.getCount()) {
							NGID   ngram_id = 0;
							THROW(ResolveNGram(ng.WordIdList, &ngram_id));
							assert(ngram_id);
							THROW(P_CNgT->Set(main_cid, ngram_id));
							{
								assert(PropInstance);
								assert(sr_type_cid);
								THROW(SetConceptProp(main_cid, PropInstance, 0, sr_type_cid));
							}
						}
						*/
					}
				}
				else {
					//PPTXT_FIASTOSR_OBJSKIPPED_TYPE    "ФИАС->Sartre: объект '@zstr' не акцептирован из-за неопределенного типа '@zstr'"
					(temp_buf = r_item.FORMALNAME).Transf(CTRANSF_OUTER_TO_INNER);
					(text = r_item.SHORTNAME).Transf(CTRANSF_OUTER_TO_INNER);
					PPLogMessage(PPFILNAM_IMPEXP_LOG, PPFormatS(PPSTR_TEXT, PPTXT_FIASTOSR_OBJSKIPPED_TYPE, &msg_buf, temp_buf.cptr(), text.cptr(), aolevel), /*LOGMSGF_TIME*/0);
				}
			}
			lidx = last_identical_idx;
			PPWaitPercent(lidx, src_list_count, "Storage");
		}
		p_blk->SrcList.clear();
		if(p_ta) {
			THROW(p_ta->Commit(1));
			ZDELETE(p_ta);
		}
	}
	CATCHZOK
	delete p_ta;
	return ok;
}
