// HASHTAB.CPP
// Copyright (c) A.Sobolev 2006, 2007, 2008, 2009, 2010, 2012, 2013, 2014, 2015, 2016, 2017
//
#include <slib.h>
#include <tv.h>
#pragma hdrstop

uint32 FASTCALL DJBHash(const void * pData, size_t len);     // @prototype

HashTableBase::HashTableBase(size_t sz)
{
	size_t i = NZOR(sz, 1024);
	if(i) do {
		if(IsPrime(i)) {
			sz = i;
			break;
		}
	} while(--i);
	Size = sz;
	P_Tab = 0;
	Flags = 0;
	AddCount  = 0;
	CollCount = 0;
	MaxTail   = 0;
}

HashTableBase::~HashTableBase()
{
	DestroyTabItems();
	ZFREE(P_Tab);
}

void HashTableBase::Clear()
{
	DestroyTabItems();
	ZFREE(P_Tab);
	AddCount  = 0;
	CollCount = 0;
	MaxTail   = 0;
	Assoc.freeAll();
}

int FASTCALL HashTableBase::Copy(const HashTableBase & rSrc)
{
	EXCEPTVAR(SLibError);
	int    ok = 1;
	HashTableBase::Clear();
	Size = rSrc.Size;
	if(rSrc.P_Tab) {
		THROW(InitTab());
		memcpy(P_Tab, rSrc.P_Tab, Size * sizeof(Entry));
		for(size_t i = 0; i < Size; i++)
			THROW(P_Tab[i].Copy(rSrc.P_Tab[i]));
	}
	Flags = rSrc.Flags;
	AddCount = rSrc.AddCount;
	CollCount = rSrc.CollCount;
	MaxTail = rSrc.MaxTail;
	Assoc = rSrc.Assoc;
	CATCHZOK
	return ok;
}

int HashTableBase::InitTab()
{
	SETIFZ(P_Tab, (Entry *)SAlloc::C(Size, sizeof(Entry)));
	return BIN(P_Tab);
}

void HashTableBase::DestroyTabItems()
{
	if(P_Tab)
		for(size_t i = 0; i < Size; i++)
			if(P_Tab[i].P_Ext)
				ZFREE(P_Tab[i].P_Ext);
}

int HashTableBase::CalcStat(Stat & rResult) const
{
	int    ok = 1;
	MEMSZERO(rResult);
	if(P_Tab) {
		StatBase stat;
		rResult.NumEntries = Size;
		for(uint i = 0; i < Size; i++) {
			const Entry & r_entry = P_Tab[i];
			const uint _c = r_entry.Count;
			stat.Step((double)_c);
			rResult.CountItems += _c;
			if(_c == 0) {
				rResult.CountEmpty++;
			}
			else if(_c == 1) {
				rResult.CountSingle++;
			}
			else {
				rResult.CountMult++;
			}
		}
		stat.Finish();
		rResult.Min = (uint)stat.GetMin();
		rResult.Max = (uint)stat.GetMax();
		rResult.Average = stat.GetExp();
		rResult.StdDev  = stat.GetStdDev();
	}
	return ok;
}

int FASTCALL HashTableBase::InitIteration(Iter * pI) const
{
	if(pI) {
		memzero(pI, sizeof(*pI));
		return 1;
	}
	else
		return 0;
}

#define ORDER_ASSOC 1

int HashTableBase::Entry::SetVal(uint key, uint val)
{
	int    ok = 1;
	if(Count == 0) {
		Val.Key = (long)key;
		Val.Val = (long)val;
		Count = 1;
	}
	else {
		P_Ext = (LAssoc *)SAlloc::R(P_Ext, sizeof(LAssoc) * Count);
		if(P_Ext == 0)
			ok = 0;
		else {
			P_Ext[Count-1].Key = (long)key;
			P_Ext[Count-1].Val = (long)val;
			Count++;
			ok = Count;
		}
	}
	return ok;
}

int FASTCALL HashTableBase::Entry::Remove(uint pos)
{
	int    ok = 0;
	if(pos < Count) {
		if(pos == 0) {
			if(Count > 1) {
				Val = P_Ext[0];
				if(Count > 2)
					memcpy(P_Ext, P_Ext+1, sizeof(Val) * (Count-2));
			}
			else {
				Val.Key = 0;
				Val.Val = 0;
			}
		}
		else if(pos < (uint)(Count-1))
			memcpy(P_Ext+pos-1, P_Ext+pos, sizeof(Val) * (Count-pos-1));
		Count--;
		ok = 1;
	}
	return ok;
}

int FASTCALL HashTableBase::Entry::Copy(const SymbHashTable::Entry & rSrc)
{
	int    ok = 1;
	Val = rSrc.Val;
	Count = rSrc.Count;
	if(Count > 1) {
		P_Ext = (LAssoc *)SAlloc::M(sizeof(LAssoc) * Count-1);
		if(P_Ext == 0) {
			Count = 1;
			ok = 0;
		}
		else
			for(size_t i = 0; i < (uint)(Count-1); i++)
				P_Ext[i] = rSrc.P_Ext[i];
	}
	else
		P_Ext = 0;
	return ok;
}

int HashTableBase::Entry::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	uint16 c = Count;
	THROW(pCtx->Serialize(dir, c, rBuf));
	if(dir > 0) {
		if(c > 0) {
			THROW(pCtx->Serialize(dir, Val.Key, rBuf));
			THROW(pCtx->Serialize(dir, Val.Val, rBuf));
			for(uint i = 1; i < c; ++i) {
				THROW(pCtx->Serialize(dir, P_Ext[i-1].Key, rBuf));
				THROW(pCtx->Serialize(dir, P_Ext[i-1].Val, rBuf));
			}
		}
	}
	else if(dir < 0) {
		Count = 0;
		Val.Key = 0;
		Val.Val = 0;
		ZDELETE(P_Ext);
		for(uint i = 0; i < c; ++i) {
			int32 key, val;
			THROW(pCtx->Serialize(dir, key, rBuf));
			THROW(pCtx->Serialize(dir, val, rBuf));
			THROW(SetVal((uint)key, (uint)val));
		}
	}
	CATCHZOK
	return ok;
}
//
//
//
SymbHashTable::SymbHashTable(size_t sz, int useAssoc) : HashTableBase(sz)
{
	NamePool.add("$");
	SETFLAG(Flags, fUseAssoc, useAssoc);
}

SymbHashTable & FASTCALL SymbHashTable::operator = (const SymbHashTable & rS)
{
	Copy(rS);
	return *this;
}

int FASTCALL SymbHashTable::Copy(const SymbHashTable & rSrc)
{
	EXCEPTVAR(SLibError);
	int    ok = 1;
	Clear();
	THROW(HashTableBase::Copy(rSrc));
	CATCHZOK
	return ok;
}

int SymbHashTable::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	THROW(pCtx->Serialize(dir, Size, rBuf));
	THROW(pCtx->Serialize(dir, Flags, rBuf));
	THROW(pCtx->Serialize(dir, AddCount, rBuf));
	THROW(pCtx->Serialize(dir, CollCount, rBuf));
	THROW(pCtx->Serialize(dir, MaxTail, rBuf));
	THROW(NamePool.Serialize(dir, rBuf, pCtx));
	THROW(pCtx->Serialize(dir, &Assoc, rBuf));
	if(dir < 0) {
		DestroyTabItems();
		// @v9.1.3 (Перенесено ниже, ибо инициализировать таблицу необходимо и при записи в поток) THROW(InitTab());
	}
	THROW(InitTab()); // @v9.1.3
	for(uint i = 0; i < Size; ++i) {
		THROW(P_Tab[i].Serialize(dir, rBuf, pCtx));
	}
	CATCHZOK
	return ok;
}

void SymbHashTable::Clear()
{
	HashTableBase::Clear();
	NamePool.clear();
	NamePool.add("$");
}

size_t FASTCALL SymbHashTable::Hash(const char * pSymb) const
{
	const size_t len = strlen(pSymb);
	uint32 __h = BobJencHash(pSymb, len);
	//uint32 __h = DJBHash(pSymb, len);
	return (size_t)(__h % Size);
	/*
	ulong __h = 0;
	for(; *pSymb; pSymb++)
		__h = 5 * __h + *pSymb;
	return (size_t)(__h % Size);
	*/
}

int FASTCALL SymbHashTable::InitIteration(Iter * pI) const
{
	return HashTableBase::InitIteration(pI);
}

int SymbHashTable::NextIteration(Iter * pI, uint * pVal, uint * pPos, SString * pStr) const
{
	if(pI && P_Tab) {
		while(pI->P < Size) {
			if(pI->E < P_Tab[pI->P].Count) {
				uint   pos = 0;
				if(pI->E == 0) {
					ASSIGN_PTR(pVal, P_Tab[pI->P].Val.Val);
					if(pPos || pStr) {
						pos = P_Tab[pI->P].Val.Key;
						if(pStr)
							NamePool.get(&pos, *pStr);
					}
				}
				else {
					ASSIGN_PTR(pVal, P_Tab[pI->P].P_Ext[pI->E-1].Val);
					if(pPos || pStr) {
						pos = P_Tab[pI->P].P_Ext[pI->E-1].Key;
						if(pStr)
							NamePool.get(&pos, *pStr);
					}
				}
				if(pI->E == P_Tab[pI->P].Count) {
					pI->P++;
					pI->E = 0;
				}
				else
					pI->E++;
				ASSIGN_PTR(pPos, pos);
				return 1;
			}
			else {
				pI->P++;
				pI->E = 0;
			}
		}
	}
	return 0;
}

void SymbHashTable::ResetAssoc()
{
	Assoc.clear();
	Flags &= ~fUseAssoc;
}

int SymbHashTable::BuildAssoc()
{
	int    ok = 1;
	Iter   i;
	uint   val = 0;
	uint   pos = 0;
	Assoc.clear();
	for(InitIteration(&i); NextIteration(&i, &val, &pos, 0);) {
		THROW(Assoc.Add((long)val, (long)pos, 0, 0 /*not binary*/));
	}
	Assoc.Sort();
	Flags |= fUseAssoc;
	CATCHZOK
	return ok;
}

uint SymbHashTable::GetMaxVal() const
{
	Iter i;
	uint   val = 0;
	uint   max_val = 0;
	for(InitIteration(&i); NextIteration(&i, &val, 0, 0);) {
		SETMAX(max_val, val);
	}
	return max_val;
}

int SymbHashTable::Add(const char * pSymb, uint val, uint * pPos)
{
	int    c = 1;
	uint   pos = 0;
	THROW(InitTab());
	THROW(NamePool.add(pSymb, &pos));
	size_t h = Hash(pSymb);
	c = P_Tab[h].SetVal(pos, val);
	if(Flags & fUseAssoc)
		THROW(Assoc.Add((long)val, (long)pos, 0, ORDER_ASSOC));
	AddCount++;
	if(c > 1) {
		CollCount++;
		if((c-1) > MaxTail)
			MaxTail = c-1;
	}
	CATCH
		c = 0;
	ENDCATCH
	ASSIGN_PTR(pPos, pos);
	return c;
}

int SymbHashTable::Add(const char * pSymb, uint val)
{
	return Add(pSymb, val, 0);
}

int SymbHashTable::Get(uint pos, SString & rBuf) const
{
	uint   p = pos;
	return NamePool.get(&p, rBuf);
}

int SymbHashTable::GetByAssoc(uint val, SString & rBuf) const
{
	int    ok = 1;
	rBuf.Z();
	if(Flags & fUseAssoc) {
		long   p = 0;
		if(Assoc.BSearch((long)val, &p, 0))
			ok = Get((uint)p, rBuf);
		else
			ok = SLS.SetError(SLERR_NOFOUND);
	}
	else {
		// @v9.8.1 ok = (SLibError = SLERR_HT_NOASSOC, 0);
		// @v9.8.1 {
		ok = SLS.SetError(SLERR_NOFOUND);
		Iter it;
		if(InitIteration(&it)) {
			uint   _v = 0;
			uint   pos = 0;
			SString temp_buf;
			while(!ok && NextIteration(&it, &_v, &pos, &temp_buf) > 0) {
                if(_v == val) {
                    rBuf = temp_buf;
                    ok = 1;
                }
			}
		}
		// } @v9.8.1
	}
	return ok;
}

int SymbHashTable::Search(const char * pSymb, uint * pVal, uint * pPos) const
{
	int    ok = 0;
	if(P_Tab) {
		size_t h  = Hash(pSymb);
		const  Entry & r_entry = P_Tab[h];
		if(r_entry.Count > 0) {
			SString temp_buf;
			uint   pos = (uint)r_entry.Val.Key;
			if(NamePool.get(pos, temp_buf) && temp_buf.Cmp(pSymb, 0) == 0) {
				ASSIGN_PTR(pVal, r_entry.Val.Val);
				ASSIGN_PTR(pPos, pos);
				ok = 1;
			}
			else
				for(uint i = 1; !ok && i < r_entry.Count; i++) {
					pos = (uint)r_entry.P_Ext[i-1].Key;
					if(NamePool.get(pos, temp_buf) && temp_buf.Cmp(pSymb, 0) == 0) {
						ASSIGN_PTR(pVal, r_entry.P_Ext[i-1].Val);
						ASSIGN_PTR(pPos, pos);
						ok = 1;
					}
				}
		}
	}
	return ok;
}

int SymbHashTable::Del(const char * pSymb, uint * pVal)
{
	int    ok = 0;
	uint   val = 0;
	if(P_Tab) {
		size_t h  = Hash(pSymb);
		uint   pos;
		Entry & r_entry = P_Tab[h];
		if(r_entry.Count > 0) {
			SString temp_buf;
			pos = (uint)r_entry.Val.Key;
			if(NamePool.get(&pos, temp_buf) && temp_buf.Cmp(pSymb, 0) == 0) {
				val = r_entry.Val.Val;
				r_entry.Remove(0);
				ok = 1;
			}
			else
				for(uint i = 1; !ok && i < r_entry.Count; i++) {
					pos = (uint)r_entry.P_Ext[i-1].Key;
					if(NamePool.get(&pos, temp_buf) && temp_buf.Cmp(pSymb, 0) == 0) {
						val = r_entry.P_Ext[i-1].Val;
						r_entry.Remove(i);
						ok = 1;
					}
				}
		}
		if(ok && Flags & fUseAssoc && Assoc.BSearch((long)val, 0, &(pos = 0)))
			Assoc.atFree(pos);
	}
	ASSIGN_PTR(pVal, val);
	return ok;
}

int SymbHashTable::Test_Cmp(const SymbHashTable & rPat) const
{
	int    ok = 1;
	Iter   iter;
	uint   val = 0, val_;
	SString symb, symb_;
	THROW((Flags & fUseAssoc) == (rPat.Flags & fUseAssoc));
	for(InitIteration(&iter); NextIteration(&iter, &val, 0, &symb) > 0;) {
		if(Flags & fUseAssoc) {
			THROW(rPat.GetByAssoc(val, symb_) && symb.Cmp(symb_, 0) == 0);
		}
		THROW(rPat.Search(symb, &val_, 0));
		THROW(val == val_);
	}

	for(rPat.InitIteration(&iter); rPat.NextIteration(&iter, &val, 0, &symb) > 0;) {
		if(Flags & fUseAssoc) {
			THROW(GetByAssoc(val, symb_) && symb.Cmp(symb_, 0) == 0);
		}
		THROW(Search(symb, &val_, 0));
		THROW(val == val_);
	}
	CATCHZOK
	return ok;
}
//
//
//
GuidHashTable::GuidHashTable(size_t sz, int useAssoc) : HashTableBase(sz)
{
	//Pool.setDelta(1024);
	SETFLAG(Flags, fUseAssoc, useAssoc);
}

GuidHashTable & FASTCALL GuidHashTable::operator = (const GuidHashTable & rS)
{
	Copy(rS);
	return *this;
}

int FASTCALL GuidHashTable::Copy(const GuidHashTable & rSrc)
{
	EXCEPTVAR(SLibError);
	int    ok = 1;
	Clear();
	THROW(HashTableBase::Copy(rSrc));
	Pool = rSrc.Pool;
	CATCHZOK
	return ok;
}

int GuidHashTable::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	THROW(pCtx->Serialize(dir, Size, rBuf));
	THROW(pCtx->Serialize(dir, Flags, rBuf));
	THROW(pCtx->Serialize(dir, AddCount, rBuf));
	THROW(pCtx->Serialize(dir, CollCount, rBuf));
	THROW(pCtx->Serialize(dir, MaxTail, rBuf));
	THROW(pCtx->Serialize(dir, &Pool, rBuf));
	THROW(pCtx->Serialize(dir, &Assoc, rBuf));
	if(dir < 0) {
		DestroyTabItems();
		THROW(InitTab());
	}
	for(uint i = 0; i < Size; ++i) {
		THROW(P_Tab[i].Serialize(dir, rBuf, pCtx));
	}
	CATCHZOK
	return ok;
}

void GuidHashTable::Clear()
{
	HashTableBase::Clear();
	Pool.clear();
}

size_t FASTCALL GuidHashTable::Hash(const S_GUID & rUuid) const
{
	//uint32 __h = BobJencHash(&rUuid, sizeof(rUuid));
	uint32 __h = DJBHash(&rUuid, sizeof(rUuid));
	return (size_t)(__h % Size);
}

int FASTCALL GuidHashTable::InitIteration(Iter * pI) const
{
	return HashTableBase::InitIteration(pI);
}

int GuidHashTable::NextIteration(Iter * pI, uint * pVal, S_GUID & rUuid) const
{
	if(pI && P_Tab) {
		while(pI->P < Size) {
			if(pI->E < P_Tab[pI->P].Count) {
				if(pI->E == 0) {
					ASSIGN_PTR(pVal, P_Tab[pI->P].Val.Val);
					uint pos = P_Tab[pI->P].Val.Key;
					rUuid = Pool.at(pos);
				}
				else {
					ASSIGN_PTR(pVal, P_Tab[pI->P].P_Ext[pI->E-1].Val);
					uint pos = P_Tab[pI->P].P_Ext[pI->E-1].Key;
					rUuid = Pool.at(pos);
				}
				if(pI->E == P_Tab[pI->P].Count) {
					pI->P++;
					pI->E = 0;
				}
				else
					pI->E++;
				return 1;
			}
			else {
				pI->P++;
				pI->E = 0;
			}
		}
	}
	return 0;
}

uint GuidHashTable::GetMaxVal() const
{
	Iter i;
	uint   val = 0;
	uint   max_val = 0;
	S_GUID uuid;
	for(InitIteration(&i); NextIteration(&i, &val, uuid);) {
		SETMAX(max_val, val);
	}
	return max_val;
}

int GuidHashTable::Add(const S_GUID & rUuid, uint val, uint * pPos)
{
	int    c = 1;
	uint   pos = 0;
	THROW(InitTab());
	pos = Pool.getCount();
	THROW(Pool.insert(&rUuid));
	size_t h = Hash(rUuid);
	c = P_Tab[h].SetVal(pos, val);
	if(Flags & fUseAssoc)
		THROW(Assoc.Add((long)val, (long)pos, 0, 1/*binary*/));
	AddCount++;
	if(c > 1) {
		CollCount++;
		if((c-1) > MaxTail)
			MaxTail = c-1;
	}
	CATCH
		c = 0;
	ENDCATCH
	ASSIGN_PTR(pPos, pos);
	return c;
}

int GuidHashTable::Get(uint pos, S_GUID & rUuid) const
{
	if(pos < Pool.getCount()) {
		rUuid = Pool.at(pos);
		return 1;
	}
	else {
		rUuid.SetZero();
		return 0;
	}
}

int GuidHashTable::GetByAssoc(uint val, S_GUID & rUuid) const
{
	int    ok = 1;
	rUuid.SetZero();
	if(Flags & fUseAssoc) {
		long   p = 0;
		if(Assoc.BSearch((long)val, &p, 0))
			ok = Get((uint)p, rUuid);
		else
			ok = (SLibError = SLERR_NOFOUND, 0);
	}
	else
		ok = (SLibError = SLERR_HT_NOASSOC, 0);
	return ok;
}

int GuidHashTable::Search(const S_GUID & rUuid, uint * pVal, uint * pPos) const
{
	int    ok = 0;
	if(P_Tab) {
		size_t h  = Hash(rUuid);
		const  Entry & r_entry = P_Tab[h];
		if(r_entry.Count > 0) {
			uint   pos = (uint)r_entry.Val.Key;
			if(pos < Pool.getCount() && Pool.at(pos) == rUuid) {
				ASSIGN_PTR(pVal, r_entry.Val.Val);
				ASSIGN_PTR(pPos, pos);
				ok = 1;
			}
			else
				for(uint i = 1; !ok && i < r_entry.Count; i++) {
					pos = (uint)r_entry.P_Ext[i-1].Key;
					if(pos < Pool.getCount() && Pool.at(pos) == rUuid) {
						ASSIGN_PTR(pVal, r_entry.P_Ext[i-1].Val);
						ASSIGN_PTR(pPos, pos);
						ok = 1;
					}
				}
		}
	}
	return ok;
}

int GuidHashTable::Del(const S_GUID & rUuid, uint * pVal)
{
	int    ok = 0;
	uint   val = 0;
	if(P_Tab) {
		size_t h  = Hash(rUuid);
		uint   pos;
		Entry & r_entry = P_Tab[h];
		if(r_entry.Count > 0) {
			SString temp_buf;
			pos = (uint)r_entry.Val.Key;
			if(pos < Pool.getCount() && Pool.at(pos) == rUuid) {
				val = r_entry.Val.Val;
				r_entry.Remove(0);
				ok = 1;
			}
			else
				for(uint i = 1; !ok && i < r_entry.Count; i++) {
					pos = (uint)r_entry.P_Ext[i-1].Key;
					if(pos < Pool.getCount() && Pool.at(pos) == rUuid) {
						val = r_entry.P_Ext[i-1].Val;
						r_entry.Remove(i);
						ok = 1;
					}
				}
		}
		if(ok && Flags & fUseAssoc && Assoc.BSearch((long)val, 0, &(pos = 0)))
			Assoc.atFree(pos);
	}
	ASSIGN_PTR(pVal, val);
	return ok;
}

int GuidHashTable::Test_Cmp(const GuidHashTable & rPat) const
{
	int    ok = 1;
	Iter   iter;
	uint   val = 0, val_;
	S_GUID uuid, uuid_;
	THROW((Flags & fUseAssoc) == (rPat.Flags & fUseAssoc));
	for(InitIteration(&iter); NextIteration(&iter, &val, uuid) > 0;) {
		if(Flags & fUseAssoc) {
			THROW(rPat.GetByAssoc(val, uuid_) && uuid_ == uuid);
		}
		THROW(rPat.Search(uuid, &val_, 0));
		THROW(val == val_);
	}

	for(rPat.InitIteration(&iter); rPat.NextIteration(&iter, &val, uuid) > 0;) {
		if(Flags & fUseAssoc) {
			THROW(GetByAssoc(val, uuid_) && uuid_ == uuid);
		}
		THROW(Search(uuid, &val_, 0));
		THROW(val == val_);
	}
	CATCHZOK
	return ok;
}
//
//
//
PtrHashTable::PtrHashTable(size_t sz, int useAssoc) : HashTableBase(sz)
{
	SETFLAG(Flags, fUseAssoc, useAssoc);
}

PtrHashTable & FASTCALL PtrHashTable::operator = (const PtrHashTable & rS)
{
	Copy(rS);
	return *this;
}

int FASTCALL PtrHashTable::Copy(const PtrHashTable & rSrc)
{
	EXCEPTVAR(SLibError);
	int    ok = 1;
	Clear();
	THROW(HashTableBase::Copy(rSrc));
	Pool = rSrc.Pool;
	CATCHZOK
	return ok;
}

void PtrHashTable::Clear()
{
	HashTableBase::Clear();
	Pool.clear();
}

int PtrHashTable::Add(void * ptr, uint val, uint * pPos)
{
	int    c = 1;
	uint   pos = 0;
	THROW(InitTab());
	pos = Pool.getCount();
	THROW(Pool.insert(&ptr));
	size_t h = Hash(ptr);
	c = P_Tab[h].SetVal(pos, val);
	if(Flags & fUseAssoc)
		THROW(Assoc.Add((long)val, (long)pos, 0, 1/*binary*/));
	AddCount++;
	if(c > 1) {
		CollCount++;
		if((c-1) > MaxTail)
			MaxTail = c-1;
	}
	CATCH
		c = 0;
	ENDCATCH
	ASSIGN_PTR(pPos, pos);
	return c;
}

int PtrHashTable::Del(void * ptr, uint * pVal)
{
	int    ok = 0;
	uint   val = 0;
	if(P_Tab) {
		size_t h = Hash(ptr);
		uint   pos;
		Entry & r_entry = P_Tab[h];
		if(r_entry.Count > 0) {
			pos = (uint)r_entry.Val.Key;
			if(pos < Pool.getCount() && Pool.at(pos) == ptr) {
				val = r_entry.Val.Val;
				r_entry.Remove(0);
				ok = 1;
			}
			else
				for(uint i = 1; !ok && i < r_entry.Count; i++) {
					pos = (uint)r_entry.P_Ext[i-1].Key;
					if(pos < Pool.getCount() && Pool.at(pos) == ptr) {
						val = r_entry.P_Ext[i-1].Val;
						r_entry.Remove(i);
						ok = 1;
					}
				}
		}
		if(ok && Flags & fUseAssoc && Assoc.BSearch((long)val, 0, &(pos = 0)))
			Assoc.atFree(pos);
	}
	ASSIGN_PTR(pVal, val);
	return ok;
}

void * FASTCALL PtrHashTable::Get(uint pos) const
{
	return (pos < Pool.getCount()) ? Pool.at(pos) : 0;
}

void * FASTCALL PtrHashTable::GetByAssoc(uint val) const
{
	void * p_result = 0;
	if(Flags & fUseAssoc) {
		long   p = 0;
		if(Assoc.BSearch((long)val, &p, 0))
			p_result = Get((uint)p);
		else
			SLS.SetError(SLERR_NOFOUND);
	}
	else
		SLS.SetError(SLERR_HT_NOASSOC);
	return p_result;
}

int PtrHashTable::Search(const void * ptr, uint * pVal, uint * pPos) const
{
	int    ok = 0;
	if(P_Tab) {
		size_t h  = Hash(ptr);
		const  Entry & r_entry = P_Tab[h];
		if(r_entry.Count > 0) {
			uint   pos = (uint)r_entry.Val.Key;
			void * p_try = Pool.at(pos);
			if(p_try == ptr) {
				ASSIGN_PTR(pVal, r_entry.Val.Val);
				ASSIGN_PTR(pPos, pos);
				ok = 1;
			}
			else {
				for(uint i = 1; !ok && i < r_entry.Count; i++) {
					pos = (uint)r_entry.P_Ext[i-1].Key;
					p_try = Pool.at(pos);
					if(p_try == ptr) {
						ASSIGN_PTR(pVal, r_entry.P_Ext[i-1].Val);
						ASSIGN_PTR(pPos, pos);
						ok = 1;
					}
				}
			}
		}
	}
	return ok;
}

int FASTCALL PtrHashTable::InitIteration(Iter * pI) const
{
	return HashTableBase::InitIteration(pI);
}

int PtrHashTable::NextIteration(Iter * pI, uint * pVal, void ** ppPtr) const
{
	if(pI && P_Tab) {
		while(pI->P < Size) {
			if(pI->E < P_Tab[pI->P].Count) {
				uint   pos = 0;
				if(pI->E == 0) {
					ASSIGN_PTR(pVal, P_Tab[pI->P].Val.Val);
					if(ppPtr) {
						pos = P_Tab[pI->P].Val.Key;
						if(ppPtr)
							*ppPtr = Pool.at(pos);
					}
				}
				else {
					ASSIGN_PTR(pVal, P_Tab[pI->P].P_Ext[pI->E-1].Val);
					if(ppPtr) {
						pos = P_Tab[pI->P].P_Ext[pI->E-1].Key;
						if(ppPtr)
							*ppPtr = Pool.at(pos);
					}
				}
				if(pI->E == P_Tab[pI->P].Count) {
					pI->P++;
					pI->E = 0;
				}
				else
					pI->E++;
				return 1;
			}
			else {
				pI->P++;
				pI->E = 0;
			}
		}
	}
	return 0;
}
/*
uint PtrHashTable::GetMaxVal() const
{
}
*/
size_t FASTCALL PtrHashTable::Hash(const void * ptr) const
{
	uint32 __h = DJBHash(&ptr, sizeof(void *));
	return (size_t)(__h % Size);
}
//
//
//
struct UhtBlock {
	ulong  Start;
	uint32 Busy;
};

UintHashTable::UintHashTable() : List(sizeof(UhtBlock))
{
}

UintHashTable & FASTCALL UintHashTable::operator = (const UintHashTable & rS)
{
	Copy(&rS);
	return *this;
}

int FASTCALL UintHashTable::Copy(const UintHashTable * pS)
{
	if(pS)
		List = pS->List;
	else
		Clear();
	return 1;
}

void * UintHashTable::GetBlock(ulong val, int cr)
{
	void * p = 0;
	ulong  start = val / 32;
	uint   pos = 0;
	if(List.bsearch(&start, &pos, CMPF_LONG)) {
		p = List.at(pos);
	}
	else if(cr) {
		UhtBlock blk;
		blk.Start = start;
		blk.Busy = 0;
		p = List.ordInsert(&blk, &pos, CMPF_LONG) ? List.at(pos) : 0;
	}
	return p;
}

void UintHashTable::Clear()
{
	List.clear();
}

void UintHashTable::Destroy()
{
	List.freeAll();
}

int UintHashTable::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx)
{
	return pCtx->Serialize(dir, &List, rBuf);
}

int FASTCALL UintHashTable::Add(ulong val)
{
	UhtBlock * p_blk = (UhtBlock *)GetBlock(val, 1);
	if(p_blk) {
		uint   shift = (1 << (val % 32));
		if(p_blk->Busy & shift)
			return -1;
		else {
			p_blk->Busy |= shift;
			return 1;
		}
	}
	else
		return 0;
}

int FASTCALL UintHashTable::Add(const UintHashTable & rS)
{
	for(ulong i = 0; rS.Enum(&i);) {
		if(!Add(i))
			return 0;
	}
	return 1;
}

int FASTCALL UintHashTable::Intersect(const UintHashTable & rS)
{
	int    ok = -1;
	for(ulong i = 0; Enum(&i);) {
		if(!rS.Has(i)) {
			Remove(i);
			ok = 1;
		}
	}
	return ok;
}

int FASTCALL UintHashTable::Remove(ulong val)
{
	UhtBlock * p_blk = (UhtBlock *)GetBlock(val, 1);
	if(p_blk) {
		uint   shift = (1 << (val % 32));
		if(p_blk->Busy & shift) {
			p_blk->Busy &= ~shift;
			return 1;
		}
		else
			return -1;
	}
	else
		return 0;
}

int FASTCALL UintHashTable::Has(ulong val) const
{
	void * p = 0;
	ulong  start = val / 32;
	uint   pos = 0;
	if(List.bsearch(&start, &pos, CMPF_LONG)) {
		const UhtBlock * p_blk = (UhtBlock *)List.at(pos);
		return BIN(p_blk->Busy & (1 << (val % 32)));
	}
	return 0;
}

uint UintHashTable::GetCount() const
{
	uint   c = 0;
	const uint lcnt = List.getCount();
	for(uint i = 0; i < lcnt; i++) {
		uint32 busy = ((UhtBlock *)List.at(i))->Busy;
		//
		// Трюк с подсчетом ненулевых битов посредством обнуления последнего единичного бита X & (X-1) {
		//
		uint   popc = 0;
		for(; busy != 0; busy &= (busy-1))
			popc++;
		// }
		c += popc;
	}
	return c;
}

int FASTCALL UintHashTable::Enum(ulong * pVal) const
{
	void * p = 0;
	ulong  val = (*pVal) + 1;
	ulong  start = val / 32;
	uint   pos = 0;
	if(List.bsearch(&start, &pos, CMPF_LONG)) {
		const uint32 busy = ((UhtBlock *)List.at(pos))->Busy;
		for(uint i = val % 32; i < 32; i++)
			if(busy & (1 << i)) {
				*pVal = (((UhtBlock *)List.at(pos))->Start * 32) + i;
				return 1;
			}
		++pos;
	}
	else
		pos = 0;
	for(; pos < List.getCount(); pos++) {
		const UhtBlock * p_blk = (const UhtBlock *)List.at(pos);
		if(p_blk->Start >= (val / 32)) {
			const uint32 busy = p_blk->Busy;
			for(uint i = 0; i < 32; i++)
				if(busy & (1 << i)) {
					*pVal = (p_blk->Start * 32) + i;
					return 1;
				}
		}
	}
	return 0;
}

#if SLTEST_RUNNING // {

SLTEST_R(HASHTAB)
{
	SString in_buf;
	SString line_buf;
	{
		const uint test_iter_count = 1000000;
		const size_t ht_size_tab[] = { 10, 100, 1000, 100000 };
		for(uint hts_idx = 0; hts_idx < SIZEOFARRAY(ht_size_tab); hts_idx++) {
			size_t ht_size = ht_size_tab[hts_idx];
			uint   _count = 0;
			SStrCollection ptr_collection;
			PtrHashTable ht(ht_size);

			(in_buf = GetSuiteEntry()->InPath).SetLastSlash().Cat("email-list.txt");
			SFile inf(in_buf, SFile::mRead);
			THROW(SLTEST_CHECK_NZ(inf.IsValid()));
			while(inf.ReadLine(line_buf)) {
				line_buf.Chomp();
				char * p_str = newStr(line_buf);
				THROW(SLTEST_CHECK_NZ(ptr_collection.insert(p_str)));
				//
				// Нечетные позиции вставляем в кэш, четные - нет
				//
				if(_count % 2) {
					THROW(SLTEST_CHECK_NZ(ht.Add(p_str, _count+1, 0)));
				}
				else {
					//
				}
				_count++;
			}
			THROW(SLTEST_CHECK_EQ(ptr_collection.getCount(), _count));
			for(uint i = 0; i < test_iter_count; i++) {
				uint idx = SLS.GetTLA().Rg.GetUniformInt(_count);
				THROW(SLTEST_CHECK_LT((long)idx, (long)_count));
				char * p_str = ptr_collection.at(idx);
				{
					uint val = 0;
					uint pos = 0;
					if(idx % 2) {
						SLTEST_CHECK_NZ(ht.Search(p_str, &val, &pos));
						void * ptr = ht.Get(pos);
						SLTEST_CHECK_NZ(ptr);
						SLTEST_CHECK_EQ(ptr, (const void *)p_str);
						SLTEST_CHECK_EQ(val, idx+1);
					}
					else {
						SLTEST_CHECK_Z(ht.Search(p_str, &val, &pos));
					}
				}
			}
		}
	}
	CATCH
		CurrentStatus = 0;
	ENDCATCH
	return CurrentStatus;
}

#endif // } SLTEST_RUNNING
//
//
//
#if 0 // @construction {

struct UT_hash_bucket {
	struct UT_hash_handle * hh_head;
	uint   count;
	//
	// expand_mult is normally set to 0. In this situation, the max chain length
	// threshold is enforced at its default value, HASH_BKT_CAPACITY_THRESH. (If
	// the bucket's chain exceeds this length, bucket expansion is triggered).
	// However, setting expand_mult to a non-zero value delays bucket expansion
	// that would be triggered by additions to this particular bucket)
	// until its chain length reaches a *multiple* of HASH_BKT_CAPACITY_THRESH.
	// (The multiplier is simply expand_mult+1). The whole idea of this
	// multiplier is to reduce bucket expansions, since they are expensive, in
	// situations where we know that a particular bucket tends to be overused.
	// It is better to let its chain length grow to a longer yet-still-bounded
	// value, than to do an O(n) bucket expansion too often.
	//
	uint   expand_mult;
};
//
// random signature used only to find hash tables in external analysis
//
#define HASH_SIGNATURE       0xa0111fe1
#define HASH_BLOOM_SIGNATURE 0xb12220f2

struct UT_hash_table {
	UT_hash_bucket * buckets;
	uint   num_buckets;
	uint   log2_num_buckets;
	uint   num_items;
	struct UT_hash_handle * tail; // tail hh in app order, for fast append
	ptrdiff_t hho; // hash handle offset (byte pos of hash handle in element
		// in an ideal situation (all buckets used equally), no bucket would have
	 	// more than ceil(#items/#buckets) items. that's the ideal chain length.
	uint   ideal_chain_maxlen;
	//
	// nonideal_items is the number of items in the hash whose chain position
	// exceeds the ideal chain maxlen. these items pay the penalty for an uneven
	// hash distribution; reaching them in a chain traversal takes >ideal steps
	//
	uint   nonideal_items;
	/* ineffective expands occur when a bucket doubling was performed, but
	 * afterward, more than half the items in the hash had nonideal chain
	 * positions. If this happens on two consecutive expansions we inhibit any
	 * further expansion, as it's not helping; this happens when the hash
	 * function isn't a good fit for the key domain. When expansion is inhibited
	 * the hash will still work, albeit no longer in constant time. */
	uint   ineff_expands, noexpand;
	uint32 signature; /* used only to find hash tables in external analysis */
#ifdef HASH_BLOOM
	uint32 bloom_sig; /* used only to test bloom exists in external analysis */
	uint8 * bloom_bv;
	char   bloom_nbits;
#endif
};

struct UT_hash_handle {
	void   Destroy()
	{
		// HASH_CLEAR
		if(tbl) {
			SAlloc::F(tbl->buckets);
			HASH_BLOOM_FREE(tbl);
			ZFREE(tbl);
		}
	}
	struct UT_hash_table * tbl;
	void * prev;                 // prev element in app order
	void * next;                 // next element in app order
	struct UT_hash_handle * hh_prev; // previous hh in bucket order
	struct UT_hash_handle * hh_next; // next hh in bucket order
	void * key;                  // ptr to enclosing struct's key
	unsigned keylen;             // enclosing struct's key len
	unsigned hashv;              // result of hash-fcn(key)
};

/*
#define HASH_CLEAR(hh, head)							  \
	do {										 \
		if(head) {								      \
			SAlloc::F((head)->hh.tbl->buckets);  \
			HASH_BLOOM_FREE((head)->hh.tbl);					     \
			SAlloc::F((head)->hh.tbl);			     \
			(head) = NULL;								       \
		}									       \
	} while(0)
*/


/* These macros use decltype or the earlier __typeof GNU extension.
   As decltype is only available in newer compilers (VS2010 or gcc 4.3+
   when compiling c++ source) this code uses whatever method is needed
   or, for VS2008 where neither is available, uses casting workarounds. */
#ifdef _MSC_VER         /* MS compiler */
#if _MSC_VER >= 1600 && defined(__cplusplus)  /* VS2010 or newer in C++ mode */
#define DECLTYPE(x) (decltype(x))
#else                   /* VS2008 or older (or VS2010 in C mode) */
#define NO_DECLTYPE
#define DECLTYPE(x)
#endif
#else                   /* GNU, Sun and other compilers */
#define DECLTYPE(x) (__typeof(x))
#endif

#ifdef NO_DECLTYPE
#define DECLTYPE_ASSIGN(dst, src)						  \
	do {										 \
		char ** _da_dst = (char**)(&(dst));						\
		*_da_dst = (char*)(src);						       \
	} while(0)
#else
#define DECLTYPE_ASSIGN(dst, src)						  \
	do {										 \
		(dst) = DECLTYPE(dst) (src);							\
	} while(0)
#endif

/* a number of the hash function use uint32_t which isn't defined on win32 */
#ifdef _MSC_VER
typedef unsigned int uint32_t;
typedef unsigned char uint8_t;
#else
#include <inttypes.h>   /* uint32_t */
#endif

#define UTHASH_VERSION 1.9 .7

#ifndef uthash_fatal
	#define uthash_fatal(msg) exit(-1)        /* fatal error (out of memory,etc) */
#endif
#ifndef uthash_noexpand_fyi
	#define uthash_noexpand_fyi(tbl)          /* can be defined to log noexpand  */
#endif
#ifndef uthash_expand_fyi
	#define uthash_expand_fyi(tbl)            /* can be defined to log expands   */
#endif

/* initial number of buckets */
#define HASH_INITIAL_NUM_BUCKETS 32      /* initial number of buckets        */
#define HASH_INITIAL_NUM_BUCKETS_LOG2 5  /* lg2 of initial number of buckets */
#define HASH_BKT_CAPACITY_THRESH 10      /* expand when bucket count reaches */

/* calculate the element whose hash handle address is hhe */
#define ELMT_FROM_HH(tbl, hhp) ((void*)(((char*)(hhp)) - ((tbl)->hho)))

#define HASH_FIND(hh, head, keyptr, keylen, out)				     \
	do {										 \
		unsigned _hf_bkt, _hf_hashv;							\
		out = NULL;									 \
		if(head) {								      \
			HASH_FCN(keyptr, keylen, (head)->hh.tbl->num_buckets, _hf_hashv, _hf_bkt);   \
			if(HASH_BLOOM_TEST((head)->hh.tbl, _hf_hashv)) {			   \
				HASH_FIND_IN_BKT((head)->hh.tbl, hh, (head)->hh.tbl->buckets[ _hf_bkt ], keyptr, keylen, out); \
			}									    \
		}									       \
	} while(0)

#ifdef HASH_BLOOM
#define HASH_BLOOM_BITLEN (1ULL << HASH_BLOOM)
#define HASH_BLOOM_BYTELEN (HASH_BLOOM_BITLEN/8) + ((HASH_BLOOM_BITLEN%8) ? 1 : 0)
#define HASH_BLOOM_MAKE(tbl)							 \
	do {										 \
		(tbl)->bloom_nbits = HASH_BLOOM;					       \
		(tbl)->bloom_bv = (uint8_t*)SAlloc::M(HASH_BLOOM_BYTELEN);		       \
		if(!((tbl)->bloom_bv))  { uthash_fatal("out of memory"); }		     \
		memzero((tbl)->bloom_bv, HASH_BLOOM_BYTELEN);				       \
		(tbl)->bloom_sig = HASH_BLOOM_SIGNATURE;				       \
	} while(0)

#define HASH_BLOOM_FREE(tbl) do { SAlloc::F((tbl)->bloom_bv); } while(0)
#define HASH_BLOOM_BITSET(bv, idx)  (bv[(idx)/8] |= (1U << ((idx)%8)))
#define HASH_BLOOM_BITTEST(bv, idx) (bv[(idx)/8] & (1U << ((idx)%8)))
#define HASH_BLOOM_ADD(tbl, hashv)  HASH_BLOOM_BITSET((tbl)->bloom_bv, (hashv & (uint32_t)((1ULL << (tbl)->bloom_nbits) - 1)))
#define HASH_BLOOM_TEST(tbl, hashv) HASH_BLOOM_BITTEST((tbl)->bloom_bv, (hashv & (uint32_t)((1ULL << (tbl)->bloom_nbits) - 1)))

#else
#define HASH_BLOOM_MAKE(tbl)
#define HASH_BLOOM_FREE(tbl)
#define HASH_BLOOM_ADD(tbl, hashv)
#define HASH_BLOOM_TEST(tbl, hashv) (1)
#endif

#define HASH_MAKE_TABLE(hh, head)						  \
	do {										 \
		(head)->hh.tbl = (UT_hash_table*)SAlloc::M(sizeof(UT_hash_table)); \
		if(!((head)->hh.tbl))  { uthash_fatal("out of memory"); }		     \
		memzero((head)->hh.tbl, sizeof(UT_hash_table));			       \
		(head)->hh.tbl->tail = &((head)->hh);					       \
		(head)->hh.tbl->num_buckets = HASH_INITIAL_NUM_BUCKETS;			       \
		(head)->hh.tbl->log2_num_buckets = HASH_INITIAL_NUM_BUCKETS_LOG2;	       \
		(head)->hh.tbl->hho = (char*)(&(head)->hh) - (char*)(head);		       \
		(head)->hh.tbl->buckets = (UT_hash_bucket*)SAlloc::M(HASH_INITIAL_NUM_BUCKETS*sizeof(struct UT_hash_bucket)); \
		if(!(head)->hh.tbl->buckets) { uthash_fatal("out of memory"); }		    \
		memzero((head)->hh.tbl->buckets, HASH_INITIAL_NUM_BUCKETS*sizeof(struct UT_hash_bucket)); \
		HASH_BLOOM_MAKE((head)->hh.tbl);					       \
		(head)->hh.tbl->signature = HASH_SIGNATURE;				       \
	} while(0)

#define HASH_ADD(hh, head, fieldname, keylen_in, add) HASH_ADD_KEYPTR(hh, head, &((add)->fieldname), keylen_in, add)

#define HASH_ADD_KEYPTR(hh, head, keyptr, keylen_in, add)			     \
	do {										 \
		unsigned _ha_bkt;								\
		(add)->hh.next = NULL;								\
		(add)->hh.key = (char*)keyptr;							\
		(add)->hh.keylen = (unsigned)keylen_in;							  \
		if(!(head)) {								       \
			head = (add);								     \
			(head)->hh.prev = NULL;							     \
			HASH_MAKE_TABLE(hh, head);						      \
		} else {									\
			(head)->hh.tbl->tail->next = (add);					     \
			(add)->hh.prev = ELMT_FROM_HH((head)->hh.tbl, (head)->hh.tbl->tail);	     \
			(head)->hh.tbl->tail = &((add)->hh);					     \
		}										\
		(head)->hh.tbl->num_items++;							\
		(add)->hh.tbl = (head)->hh.tbl;							\
		HASH_FCN(keyptr, keylen_in, (head)->hh.tbl->num_buckets, (add)->hh.hashv, _ha_bkt); \
		HASH_ADD_TO_BKT((head)->hh.tbl->buckets[_ha_bkt], &(add)->hh);			 \
		HASH_BLOOM_ADD((head)->hh.tbl, (add)->hh.hashv);				 \
		HASH_EMIT_KEY(hh, head, keyptr, keylen_in);					   \
		HASH_FSCK(hh, head);								 \
	} while(0)

#define HASH_TO_BKT(hashv, num_bkts, bkt) do { bkt = ((hashv) & ((num_bkts) - 1)); } while(0)

/* delete "delptr" from the hash table.
 * "the usual" patch-up process for the app-order doubly-linked-list.
 * The use of _hd_hh_del below deserves special explanation.
 * These used to be expressed using (delptr) but that led to a bug
 * if someone used the same symbol for the head and deletee, like
 *  HASH_DELETE(hh,users,users);
 * We want that to work, but by changing the head (users) below
 * we were forfeiting our ability to further refer to the deletee (users)
 * in the patch-up process. Solution: use scratch space to
 * copy the deletee pointer, then the latter references are via that
 * scratch pointer rather than through the repointed (users) symbol.
 */
#define HASH_DELETE(hh, head, delptr)						   \
	do {										 \
		unsigned _hd_bkt;							     \
		struct UT_hash_handle * _hd_hh_del;					      \
		if(((delptr)->hh.prev == NULL) && ((delptr)->hh.next == NULL))  {	    \
			SAlloc::F((head)->hh.tbl->buckets); \
			HASH_BLOOM_FREE((head)->hh.tbl);					 \
			SAlloc::F((head)->hh.tbl);			 \
			head = NULL;								 \
		} else {								     \
			_hd_hh_del = &((delptr)->hh);						 \
			if((delptr) == ELMT_FROM_HH((head)->hh.tbl, (head)->hh.tbl->tail)) {	 \
				(head)->hh.tbl->tail = (UT_hash_handle*)((ptrdiff_t)((delptr)->hh.prev) + (head)->hh.tbl->hho); \
			}									 \
			if((delptr)->hh.prev) {							\
				((UT_hash_handle*)((ptrdiff_t)((delptr)->hh.prev) + (head)->hh.tbl->hho))->next = (delptr)->hh.next; \
			} else {								 \
				DECLTYPE_ASSIGN(head, (delptr)->hh.next);			      \
			}									 \
			if(_hd_hh_del->next) {							\
				((UT_hash_handle*)((ptrdiff_t)_hd_hh_del->next + (head)->hh.tbl->hho))->prev = _hd_hh_del->prev; \
			}									 \
			HASH_TO_BKT(_hd_hh_del->hashv, (head)->hh.tbl->num_buckets, _hd_bkt);	\
			HASH_DEL_IN_BKT(hh, (head)->hh.tbl->buckets[_hd_bkt], _hd_hh_del);	  \
			(head)->hh.tbl->num_items--;						 \
		}									     \
		HASH_FSCK(hh, head);							      \
	} while(0)

/* convenience forms of HASH_FIND/HASH_ADD/HASH_DEL */
#define HASH_FIND_STR(head, findstr, out) HASH_FIND(hh, head, findstr, strlen(findstr), out)
#define HASH_ADD_STR(head, strfield, add) HASH_ADD(hh, head, strfield, strlen(add->strfield), add)
#define HASH_FIND_INT(head, findint, out) HASH_FIND(hh, head, findint, sizeof(int), out)
#define HASH_ADD_INT(head, intfield, add) HASH_ADD(hh, head, intfield, sizeof(int), add)
#define HASH_FIND_PTR(head, findptr, out) HASH_FIND(hh, head, findptr, sizeof(void *), out)
#define HASH_ADD_PTR(head, ptrfield, add) HASH_ADD(hh, head, ptrfield, sizeof(void *), add)
#define HASH_DEL(head, delptr)            HASH_DELETE(hh, head, delptr)

/* HASH_FSCK checks hash integrity on every add/delete when HASH_DEBUG is defined.
 * This is for uthash developer only; it compiles away if HASH_DEBUG isn't defined.
 */
#ifdef HASH_DEBUG
#define HASH_OOPS(...) do { fprintf(stderr, __VA_ARGS__); exit(-1); } while(0)
#define HASH_FSCK(hh, head)							  \
	do {										 \
		unsigned _bkt_i;							     \
		unsigned _count, _bkt_count;						     \
		char * _prev;								      \
		struct UT_hash_handle * _thh;						      \
		if(head) {								    \
			_count = 0;								 \
			for(_bkt_i = 0; _bkt_i < (head)->hh.tbl->num_buckets; _bkt_i++) {	\
				_bkt_count = 0;							     \
				_thh = (head)->hh.tbl->buckets[_bkt_i].hh_head;			     \
				_prev = NULL;							     \
				while(_thh) {							    \
					if(_prev != (char*)(_thh->hh_prev)) {				 \
						HASH_OOPS("invalid hh_prev %p, actual %p\n", _thh->hh_prev, _prev); \
					}								  \
					_bkt_count++;							  \
					_prev = (char*)(_thh);						  \
					_thh = _thh->hh_next;						  \
				}								     \
				_count += _bkt_count;						     \
				if((head)->hh.tbl->buckets[_bkt_i].count !=  _bkt_count) {	    \
					HASH_OOPS("invalid bucket count %d, actual %d\n", (head)->hh.tbl->buckets[_bkt_i].count, _bkt_count); \
				}								     \
			}									 \
			if(_count != (head)->hh.tbl->num_items) {				\
				HASH_OOPS("invalid hh item count %d, actual %d\n", (head)->hh.tbl->num_items, _count); \
			}									 \
			/* traverse hh in app order; check next/prev integrity, count */	 \
			_count = 0;								 \
			_prev = NULL;								 \
			_thh =  &(head)->hh;							 \
			while(_thh) {								\
				_count++;							      \
				if(_prev !=(char*)(_thh->prev)) {				     \
					HASH_OOPS("invalid prev %p, actual %p\n", _thh->prev, _prev); \
				}								      \
				_prev = (char*)ELMT_FROM_HH((head)->hh.tbl, _thh);		      \
				_thh = (_thh->next ?  (UT_hash_handle*)((char*)(_thh->next) + (head)->hh.tbl->hho) : NULL); \
			}									 \
			if(_count != (head)->hh.tbl->num_items) {				\
				HASH_OOPS("invalid app item count %d, actual %d\n", (head)->hh.tbl->num_items, _count); \
			}									 \
		}									     \
	} while(0)
#else
#define HASH_FSCK(hh, head)
#endif

/* When compiled with -DHASH_EMIT_KEYS, length-prefixed keys are emitted to
 * the descriptor to which this macro is defined for tuning the hash function.
 * The app can #include <unistd.h> to get the prototype for write(2). */
#ifdef HASH_EMIT_KEYS
#define HASH_EMIT_KEY(hh, head, keyptr, fieldlen)				    \
	do {										 \
		unsigned _klen = fieldlen;						     \
		write(HASH_EMIT_KEYS, &_klen, sizeof(_klen));				     \
		write(HASH_EMIT_KEYS, keyptr, fieldlen);				     \
	} while(0)
#else
#define HASH_EMIT_KEY(hh, head, keyptr, fieldlen)
#endif

/* default to Jenkin's hash unless overridden e.g. DHASH_FUNCTION=HASH_SAX */
#ifdef HASH_FUNCTION
#define HASH_FCN HASH_FUNCTION
#else
#define HASH_FCN HASH_JEN
#endif

/* The Bernstein hash function, used in Perl prior to v5.6 */
#define HASH_BER(key, keylen, num_bkts, hashv, bkt)				     \
	do {										 \
		unsigned _hb_keylen = keylen;							 \
		char * _hb_key = (char*)(key);							  \
		(hashv) = 0;								       \
		while(_hb_keylen--)  { (hashv) = ((hashv) * 33) + *_hb_key++; }		      \
		bkt = (hashv) & (num_bkts-1);						       \
	} while(0)

/* SAX/FNV/OAT/JEN hash functions are macro variants of those listed at
 * http://eternallyconfuzzled.com/tuts/algorithms/jsw_tut_hashing.aspx */
#define HASH_SAX(key, keylen, num_bkts, hashv, bkt)				     \
	do {										 \
		unsigned _sx_i;								       \
		char * _hs_key = (char*)(key);							  \
		hashv = 0;								       \
		for(_sx_i = 0; _sx_i < keylen; _sx_i++)						 \
			hashv ^= (hashv << 5) + (hashv >> 2) + _hs_key[_sx_i];			   \
		bkt = hashv & (num_bkts-1);						       \
	} while(0)

#define HASH_FNV(key, keylen, num_bkts, hashv, bkt)				     \
	do {										 \
		unsigned _fn_i;								       \
		char * _hf_key = (char*)(key);							  \
		hashv = 2166136261UL;							       \
		for(_fn_i = 0; _fn_i < keylen; _fn_i++)						 \
			hashv = (hashv * 16777619) ^ _hf_key[_fn_i];				   \
		bkt = hashv & (num_bkts-1);						       \
	} while(0)

#define HASH_OAT(key, keylen, num_bkts, hashv, bkt)				     \
	do {										 \
		unsigned _ho_i;								       \
		char * _ho_key = (char*)(key);							  \
		hashv = 0;								       \
		for(_ho_i = 0; _ho_i < keylen; _ho_i++) {					 \
			hashv += _ho_key[_ho_i];						   \
			hashv += (hashv << 10);							   \
			hashv ^= (hashv >> 6);							   \
		}									       \
		hashv += (hashv << 3);							       \
		hashv ^= (hashv >> 11);							       \
		hashv += (hashv << 15);							       \
		bkt = hashv & (num_bkts-1);						       \
	} while(0)

#define HASH_JEN_MIX(a, b, c)							   \
	do {										 \
		a -= b; a -= c; a ^= (c >> 13);					       \
		b -= c; b -= a; b ^= (a << 8);					       \
		c -= a; c -= b; c ^= (b >> 13);					       \
		a -= b; a -= c; a ^= (c >> 12);					       \
		b -= c; b -= a; b ^= (a << 16);					       \
		c -= a; c -= b; c ^= (b >> 5);					       \
		a -= b; a -= c; a ^= (c >> 3);					       \
		b -= c; b -= a; b ^= (a << 10);					       \
		c -= a; c -= b; c ^= (b >> 15);					       \
	} while(0)

void HashJenMix(uint32 & rA, uint32 & rB, uint32 & rC)
{
	uint32 a = rA;
	uint32 b = rB;
	uint32 c = rC;
	a -= b; a -= c; a ^= (c >> 13);
	b -= c; b -= a; b ^= (a << 8);
	c -= a; c -= b; c ^= (b >> 13);
	a -= b; a -= c; a ^= (c >> 12);
	b -= c; b -= a; b ^= (a << 16);
	c -= a; c -= b; c ^= (b >> 5);
	a -= b; a -= c; a ^= (c >> 3);
	b -= c; b -= a; b ^= (a << 10);
	c -= a; c -= b; c ^= (b >> 15);
	rA = a;
	rB = b;
	rC = c;
}

uint32 HashJen(const void * pKey, size_t keyLen, uint numBkts, uint * pBkt)
{
	uint   bkt = 0;
	uint32 hashv = 0xfeedbeef;
	uint32 _hj_i, _hj_j;
	char * _hj_key = (char*)pKey;
	_hj_i = _hj_j = 0x9e3779b9;
	size_t _hj_k = keyLen;
	for(; _hj_k >= 12; _hj_k -= 12) {
		_hj_i += (_hj_key[0] + ((uint)_hj_key[1] << 8) + ((uint)_hj_key[2]  << 16) + ((uint)_hj_key[3]  << 24));
		_hj_j += (_hj_key[4] + ((uint)_hj_key[5] << 8) + ((uint)_hj_key[6]  << 16) + ((uint)_hj_key[7]  << 24));
		hashv += (_hj_key[8] + ((uint)_hj_key[9] << 8) + ((uint)_hj_key[10] << 16) + ((uint)_hj_key[11] << 24));
		HashJenMix(_hj_i, _hj_j, hashv);
		_hj_key += 12;
	}
	hashv += keylen;
	switch(_hj_k) {
		case 11: hashv += ((uint)_hj_key[10] << 24);
		case 10: hashv += ((uint)_hj_key[9] << 16);
		case 9:  hashv += ((uint)_hj_key[8] << 8);
		case 8:  _hj_j += ((uint)_hj_key[7] << 24);
		case 7:  _hj_j += ((uint)_hj_key[6] << 16);
		case 6:  _hj_j += ((uint)_hj_key[5] << 8);
		case 5:  _hj_j += _hj_key[4];
		case 4:  _hj_i += ((uint)_hj_key[3] << 24);
		case 3:  _hj_i += ((uint)_hj_key[2] << 16);
		case 2:  _hj_i += ((uint)_hj_key[1] << 8);
		case 1:  _hj_i += _hj_key[0];
	}
	HashJenMix(_hj_i, _hj_j, hashv);
	bkt = hashv & (num_bkts-1);
	ASSIGN_PTR(pBkt, bkt);
	return hashv;
}

#define HASH_JEN(key, keylen, num_bkts, hashv, bkt)				     \
	do {										 \
		unsigned _hj_i, _hj_j, _hj_k;							 \
		char * _hj_key = (char*)(key);							  \
		hashv = 0xfeedbeef;							       \
		_hj_i = _hj_j = 0x9e3779b9;						       \
		_hj_k = (uint)keylen;								 \
		while(_hj_k >= 12) {							      \
			_hj_i += (_hj_key[0] + ((uint)_hj_key[1] << 8) + ((uint)_hj_key[2]  << 16) + ((uint)_hj_key[3]  << 24)); \
			_hj_j += (_hj_key[4] + ((uint)_hj_key[5] << 8) + ((uint)_hj_key[6]  << 16) + ((uint)_hj_key[7]  << 24)); \
			hashv += (_hj_key[8] + ((uint)_hj_key[9] << 8) + ((uint)_hj_key[10] << 16) + ((uint)_hj_key[11] << 24)); \
			HASH_JEN_MIX(_hj_i, _hj_j, hashv);					    \
			_hj_key += 12;								    \
			_hj_k -= 12;								    \
		}									       \
		hashv += keylen;							       \
		switch(_hj_k) {								    \
			case 11: hashv += ((uint)_hj_key[10] << 24);			    \
			case 10: hashv += ((uint)_hj_key[9] << 16);			    \
			case 9:  hashv += ((uint)_hj_key[8] << 8);			    \
			case 8:  _hj_j += ((uint)_hj_key[7] << 24);			    \
			case 7:  _hj_j += ((uint)_hj_key[6] << 16);			    \
			case 6:  _hj_j += ((uint)_hj_key[5] << 8);			    \
			case 5:  _hj_j += _hj_key[4];						    \
			case 4:  _hj_i += ((uint)_hj_key[3] << 24);			    \
			case 3:  _hj_i += ((uint)_hj_key[2] << 16);			    \
			case 2:  _hj_i += ((uint)_hj_key[1] << 8);			    \
			case 1:  _hj_i += _hj_key[0];						    \
		}									       \
		HASH_JEN_MIX(_hj_i, _hj_j, hashv);					       \
		bkt = hashv & (num_bkts-1);						       \
	} while(0)

/* The Paul Hsieh hash function */
#undef get16bits
#if (defined(__GNUC__) && defined(__i386__)) || defined(__WATCOMC__)		 \
	|| defined(_MSC_VER) || defined (__BORLANDC__) || defined (__TURBOC__)
#define get16bits(d) (*((const uint16_t*)(d)))
#endif

#if !defined (get16bits)
#define get16bits(d) ((((uint32_t)(((const uint8_t*)(d))[1])) << 8)		\
	    +(uint32_t)(((const uint8_t*)(d))[0]))
#endif
#define HASH_SFH(key, keylen, num_bkts, hashv, bkt)				     \
	do {										 \
		char * _sfh_key = (char*)(key);							  \
		uint32_t _sfh_tmp, _sfh_len = keylen;					       \
										 \
		int _sfh_rem = _sfh_len & 3;						       \
		_sfh_len >>= 2;								       \
		hashv = 0xcafebabe;							       \
										 \
		/* Main loop */								       \
		for(; _sfh_len > 0; _sfh_len--) {					       \
			hashv    += get16bits(_sfh_key);					    \
			_sfh_tmp       = (get16bits(_sfh_key+2) << 11) ^ hashv;			    \
			hashv     = (hashv << 16) ^ _sfh_tmp;					     \
			_sfh_key += 2*sizeof(uint16_t);						    \
			hashv    += hashv >> 11;						     \
		}									       \
										 \
		/* Handle end cases */							       \
		switch(_sfh_rem) {							      \
			case 3: hashv += get16bits(_sfh_key);					    \
			    hashv ^= hashv << 16;						 \
			    hashv ^= _sfh_key[sizeof(uint16_t)] << 18;				\
			    hashv += hashv >> 11;						 \
			    break;								 \
			case 2: hashv += get16bits(_sfh_key);					    \
			    hashv ^= hashv << 11;						 \
			    hashv += hashv >> 17;						 \
			    break;								 \
			case 1: hashv += *_sfh_key;						     \
			    hashv ^= hashv << 10;						 \
			    hashv += hashv >> 1;						 \
		}									       \
										 \
		/* Force "avalanching" of final 127 bits */				     \
		hashv ^= hashv << 3;							     \
		hashv += hashv >> 5;							     \
		hashv ^= hashv << 4;							     \
		hashv += hashv >> 17;							     \
		hashv ^= hashv << 25;							     \
		hashv += hashv >> 6;							     \
		bkt = hashv & (num_bkts-1);						     \
	} while(0)

#ifdef HASH_USING_NO_STRICT_ALIASING
/* The MurmurHash exploits some CPU's (x86,x86_64) tolerance for unaligned reads.
 * For other types of CPU's (e.g. Sparc) an unaligned read causes a bus error.
 * MurmurHash uses the faster approach only on CPU's where we know it's safe.
 *
 * Note the preprocessor built-in defines can be emitted using:
 *
 *   gcc -m64 -dM -E - < /dev/null                  (on gcc)
 *   cc -## a.c (where a.c is a simple test file)   (Sun Studio)
 */
#if (defined(__i386__) || defined(__x86_64__)  || defined(_M_IX86))
#define MUR_GETBLOCK(p, i) p[i]
#else /* non intel */
#define MUR_PLUS0_ALIGNED(p) (((unsigned long)p & 0x3) == 0)
#define MUR_PLUS1_ALIGNED(p) (((unsigned long)p & 0x3) == 1)
#define MUR_PLUS2_ALIGNED(p) (((unsigned long)p & 0x3) == 2)
#define MUR_PLUS3_ALIGNED(p) (((unsigned long)p & 0x3) == 3)
#define WP(p) ((uint32_t*)((unsigned long)(p) & ~3UL))
#if (defined(__BIG_ENDIAN__) || defined(SPARC) || defined(__ppc__) || defined(__ppc64__))
#define MUR_THREE_ONE(p) ((((*WP(p))&0x00ffffff) << 8) | (((*(WP(p)+1))&0xff000000) >> 24))
#define MUR_TWO_TWO(p)   ((((*WP(p))&0x0000ffff) <<16) | (((*(WP(p)+1))&0xffff0000) >> 16))
#define MUR_ONE_THREE(p) ((((*WP(p))&0x000000ff) <<24) | (((*(WP(p)+1))&0xffffff00) >>  8))
#else /* assume little endian non-intel */
#define MUR_THREE_ONE(p) ((((*WP(p))&0xffffff00) >> 8) | (((*(WP(p)+1))&0x000000ff) << 24))
#define MUR_TWO_TWO(p)   ((((*WP(p))&0xffff0000) >>16) | (((*(WP(p)+1))&0x0000ffff) << 16))
#define MUR_ONE_THREE(p) ((((*WP(p))&0xff000000) >>24) | (((*(WP(p)+1))&0x00ffffff) <<  8))
#endif
#define MUR_GETBLOCK(p, i) (MUR_PLUS0_ALIGNED(p) ? ((p)[i]) : (MUR_PLUS1_ALIGNED(p) ? MUR_THREE_ONE(p) : (MUR_PLUS2_ALIGNED(p) ? MUR_TWO_TWO(p) : MUR_ONE_THREE(p))))
#endif
#define MUR_ROTL32(x, r) (((x) << (r)) | ((x) >> (32 - (r))))
#define MUR_FMIX(_h) \
	do {		     \
		_h ^= _h >> 16;	   \
		_h *= 0x85ebca6b;  \
		_h ^= _h >> 13;	   \
		_h *= 0xc2b2ae35l; \
		_h ^= _h >> 16;	   \
	} while(0)

#define HASH_MUR(key, keylen, num_bkts, hashv, bkt)			   \
	do {								       \
		const uint8_t * _mur_data = (const uint8_t*)(key);		      \
		const int _mur_nblocks = (keylen) / 4;				     \
		uint32_t _mur_h1 = 0xf88D5353;					     \
		uint32_t _mur_c1 = 0xcc9e2d51;					     \
		uint32_t _mur_c2 = 0x1b873593;					     \
		uint32_t _mur_k1 = 0;						     \
		const uint8_t * _mur_tail;					      \
		const uint32_t * _mur_blocks = (const uint32_t*)(_mur_data+_mur_nblocks*4); \
		int _mur_i;							     \
		for(_mur_i = -_mur_nblocks; _mur_i; _mur_i++) {			     \
			_mur_k1 = MUR_GETBLOCK(_mur_blocks, _mur_i);			    \
			_mur_k1 *= _mur_c1;						   \
			_mur_k1 = MUR_ROTL32(_mur_k1, 15);				    \
			_mur_k1 *= _mur_c2;						   \
								       \
			_mur_h1 ^= _mur_k1;						   \
			_mur_h1 = MUR_ROTL32(_mur_h1, 13);				    \
			_mur_h1 = _mur_h1*5+0xe6546b64;					   \
		}								     \
		_mur_tail = (const uint8_t*)(_mur_data + _mur_nblocks*4);	     \
		_mur_k1 = 0;							       \
		switch((keylen) & 3) {						     \
			case 3: _mur_k1 ^= _mur_tail[2] << 16;				   \
			case 2: _mur_k1 ^= _mur_tail[1] << 8;				   \
			case 1: _mur_k1 ^= _mur_tail[0];				   \
			    _mur_k1 *= _mur_c1;						       \
			    _mur_k1 = MUR_ROTL32(_mur_k1, 15);					\
			    _mur_k1 *= _mur_c2;						       \
			    _mur_h1 ^= _mur_k1;						       \
		}								     \
		_mur_h1 ^= (keylen);						     \
		MUR_FMIX(_mur_h1);						     \
		hashv = _mur_h1;						     \
		bkt = hashv & (num_bkts-1);					     \
	} while(0)
#endif  /* HASH_USING_NO_STRICT_ALIASING */

/* key comparison function; return 0 if keys equal */
#define HASH_KEYCMP(a, b, len) memcmp(a, b, len)

/* iterate over items in a known bucket to find desired item */
#define HASH_FIND_IN_BKT(tbl, hh, head, keyptr, keylen_in, out)			      \
	do {										 \
		if(head.hh_head) DECLTYPE_ASSIGN(out, ELMT_FROM_HH(tbl, head.hh_head));		 \
		else out = NULL;								  \
		while(out) {								       \
			if((out)->hh.keylen == keylen_in) {					      \
				if((HASH_KEYCMP((out)->hh.key, keyptr, keylen_in)) == 0) break;		    \
			}									     \
			if((out)->hh.hh_next) DECLTYPE_ASSIGN(out, ELMT_FROM_HH(tbl, (out)->hh.hh_next)); \
			else out = NULL;							     \
		}										\
	} while(0)

/* add an item to a bucket  */
#define HASH_ADD_TO_BKT(head, addhh)						  \
	do {										 \
		head.count++;									\
		(addhh)->hh_next = head.hh_head;						\
		(addhh)->hh_prev = NULL;							\
		if(head.hh_head) { (head).hh_head->hh_prev = (addhh); }			       \
		(head).hh_head = addhh;								  \
		if(head.count >= ((head.expand_mult+1) * HASH_BKT_CAPACITY_THRESH) && (addhh)->tbl->noexpand != 1) { \
			HASH_EXPAND_BUCKETS((addhh)->tbl);					  \
		}										\
	} while(0)

/* remove an item from a given bucket */
#define HASH_DEL_IN_BKT(hh, head, hh_del)					   \
	(head).count--;								     \
	if((head).hh_head == hh_del) {						    \
		(head).hh_head = hh_del->hh_next;					   \
	}									     \
	if(hh_del->hh_prev) {							    \
		hh_del->hh_prev->hh_next = hh_del->hh_next;				 \
	}									     \
	if(hh_del->hh_next) {							    \
		hh_del->hh_next->hh_prev = hh_del->hh_prev;				 \
	}

/* Bucket expansion has the effect of doubling the number of buckets
 * and redistributing the items into the new buckets. Ideally the
 * items will distribute more or less evenly into the new buckets
 * (the extent to which this is true is a measure of the quality of
 * the hash function as it applies to the key domain).
 *
 * With the items distributed into more buckets, the chain length
 * (item count) in each bucket is reduced. Thus by expanding buckets
 * the hash keeps a bound on the chain length. This bounded chain
 * length is the essence of how a hash provides constant time lookup.
 *
 * The calculation of tbl->ideal_chain_maxlen below deserves some
 * explanation. First, keep in mind that we're calculating the ideal
 * maximum chain length based on the *new* (doubled) bucket count.
 * In fractions this is just n/b (n=number of items,b=new num buckets).
 * Since the ideal chain length is an integer, we want to calculate
 * ceil(n/b). We don't depend on floating point arithmetic in this
 * hash, so to calculate ceil(n/b) with integers we could write
 *
 *      ceil(n/b) = (n/b) + ((n%b)?1:0)
 *
 * and in fact a previous version of this hash did just that.
 * But now we have improved things a bit by recognizing that b is
 * always a power of two. We keep its base 2 log handy (call it lb),
 * so now we can write this with a bit shift and logical AND:
 *
 *      ceil(n/b) = (n>>lb) + ((n & (b-1)) ? 1:0)
 *
 */
#define HASH_EXPAND_BUCKETS(tbl)						 \
	do {										 \
		unsigned _he_bkt;							     \
		unsigned _he_bkt_i;							     \
		struct UT_hash_handle * _he_thh, * _he_hh_nxt;				       \
		UT_hash_bucket * _he_new_buckets, * _he_newbkt;				       \
		_he_new_buckets = (UT_hash_bucket*)SAlloc::M(2 * tbl->num_buckets * sizeof(struct UT_hash_bucket)); \
		if(!_he_new_buckets) { uthash_fatal("out of memory"); }			   \
		memzero(_he_new_buckets, 2 * tbl->num_buckets * sizeof(struct UT_hash_bucket));		 \
		tbl->ideal_chain_maxlen =						     \
		    (tbl->num_items >> (tbl->log2_num_buckets+1)) +			      \
		    ((tbl->num_items & ((tbl->num_buckets*2)-1)) ? 1 : 0);		      \
		tbl->nonideal_items = 0;						     \
		for(_he_bkt_i = 0; _he_bkt_i < tbl->num_buckets; _he_bkt_i++) { \
			_he_thh = tbl->buckets[ _he_bkt_i ].hh_head;				 \
			while(_he_thh) {							\
				_he_hh_nxt = _he_thh->hh_next;					      \
				HASH_TO_BKT(_he_thh->hashv, tbl->num_buckets*2, _he_bkt);	     \
				_he_newbkt = &(_he_new_buckets[ _he_bkt ]);			      \
				if(++(_he_newbkt->count) > tbl->ideal_chain_maxlen) {		     \
					tbl->nonideal_items++;						    \
					_he_newbkt->expand_mult = _he_newbkt->count / tbl->ideal_chain_maxlen; \
				}								      \
				_he_thh->hh_prev = NULL;					      \
				_he_thh->hh_next = _he_newbkt->hh_head;				      \
				if(_he_newbkt->hh_head) _he_newbkt->hh_head->hh_prev = _he_thh; \
				_he_newbkt->hh_head = _he_thh;					      \
				_he_thh = _he_hh_nxt;						      \
			}									 \
		}									     \
		SAlloc::F(tbl->buckets); \
		tbl->num_buckets *= 2;							     \
		tbl->log2_num_buckets++;						     \
		tbl->buckets = _he_new_buckets;						     \
		tbl->ineff_expands = (tbl->nonideal_items > (tbl->num_items >> 1)) ? (tbl->ineff_expands+1) : 0; \
		if(tbl->ineff_expands > 1) {						    \
			tbl->noexpand = 1;							   \
			uthash_noexpand_fyi(tbl);						 \
		}									     \
		uthash_expand_fyi(tbl);							     \
	} while(0)

/* This is an adaptation of Simon Tatham's O(n log(n)) mergesort */
/* Note that HASH_SORT assumes the hash handle name to be hh.
 * HASH_SRT was added to allow the hash handle name to be passed in. */
#define HASH_SORT(head, cmpfcn) HASH_SRT(hh, head, cmpfcn)
#define HASH_SRT(hh, head, cmpfcn)						   \
	do {										 \
		unsigned _hs_i;								       \
		unsigned _hs_looping, _hs_nmerges, _hs_insize, _hs_psize, _hs_qsize;		   \
		struct UT_hash_handle * _hs_p, * _hs_q, * _hs_e, * _hs_list, * _hs_tail;	    \
		if(head) {								      \
			_hs_insize = 1;								   \
			_hs_looping = 1;							   \
			_hs_list = &((head)->hh);						   \
			while(_hs_looping) {							  \
				_hs_p = _hs_list;						       \
				_hs_list = NULL;						       \
				_hs_tail = NULL;						       \
				_hs_nmerges = 0;						       \
				while(_hs_p) {							      \
					_hs_nmerges++;							   \
					_hs_q = _hs_p;							   \
					_hs_psize = 0;							   \
					for(_hs_i = 0; _hs_i  < _hs_insize; _hs_i++) {			\
						_hs_psize++;						       \
						_hs_q = (UT_hash_handle*)((_hs_q->next) ? ((void*)((char*)(_hs_q->next) + (head)->hh.tbl->hho)) : NULL); \
						if(!(_hs_q)) break;					     \
					}								   \
					_hs_qsize = _hs_insize;						   \
					while((_hs_psize > 0) || ((_hs_qsize > 0) && _hs_q)) {		  \
						if(_hs_psize == 0) {					      \
							_hs_e = _hs_q;						   \
							_hs_q = (UT_hash_handle*)((_hs_q->next) ? ((void*)((char*)(_hs_q->next) + (head)->hh.tbl->hho)) : NULL); \
							_hs_qsize--;						   \
						} else if((_hs_qsize == 0) || !(_hs_q)) {		      \
							_hs_e = _hs_p;						   \
							_hs_p = (UT_hash_handle*)((_hs_p->next) ? ((void*)((char*)(_hs_p->next) + (head)->hh.tbl->hho)) : NULL); \
							_hs_psize--;						   \
						} else if((cmpfcn(DECLTYPE(head) (ELMT_FROM_HH((head)->hh.tbl, _hs_p)), DECLTYPE(head) (ELMT_FROM_HH((head)->hh.tbl, _hs_q)))) <= 0) { \
							_hs_e = _hs_p;						   \
							_hs_p = (UT_hash_handle*)((_hs_p->next) ? ((void*)((char*)(_hs_p->next) + (head)->hh.tbl->hho)) : NULL); \
							_hs_psize--;						   \
						} else {						       \
							_hs_e = _hs_q;						   \
							_hs_q = (UT_hash_handle*)((_hs_q->next) ? ((void*)((char*)(_hs_q->next) + (head)->hh.tbl->hho)) : NULL); \
							_hs_qsize--;						   \
						}							       \
						if(_hs_tail) {						    \
							_hs_tail->next = ((_hs_e) ? ELMT_FROM_HH((head)->hh.tbl, _hs_e) : NULL);	  \
						} else {						       \
							_hs_list = _hs_e;					   \
						}							       \
						_hs_e->prev = ((_hs_tail) ? ELMT_FROM_HH((head)->hh.tbl, _hs_tail) : NULL);		 \
						_hs_tail = _hs_e;					       \
					}								   \
					_hs_p = _hs_q;							   \
				}								       \
				_hs_tail->next = NULL;						       \
				if(_hs_nmerges <= 1) {						    \
					_hs_looping = 0;						     \
					(head)->hh.tbl->tail = _hs_tail;				   \
					DECLTYPE_ASSIGN(head, ELMT_FROM_HH((head)->hh.tbl, _hs_list));	    \
				}								       \
				_hs_insize *= 2;						       \
			}									   \
			HASH_FSCK(hh, head);							    \
		}										\
	} while(0)

/* This function selects items from one hash into another hash.
 * The end result is that the selected items have dual presence
 * in both hashes. There is no copy of the items made; rather
 * they are added into the new hash through a secondary hash
 * hash handle that must be present in the structure. */
#define HASH_SELECT(hh_dst, dst, hh_src, src, cond)				 \
	do {										 \
		unsigned _src_bkt, _dst_bkt;						       \
		void * _last_elt = NULL, * _elt;						   \
		UT_hash_handle * _src_hh, * _dst_hh, * _last_elt_hh = NULL;			    \
		ptrdiff_t _dst_hho = ((char*)(&(dst)->hh_dst) - (char*)(dst));		       \
		if(src) {								      \
			for(_src_bkt = 0; _src_bkt < (src)->hh_src.tbl->num_buckets; _src_bkt++) {     \
				for(_src_hh = (src)->hh_src.tbl->buckets[_src_bkt].hh_head;		   \
				    _src_hh;								   \
				    _src_hh = _src_hh->hh_next) {					   \
					_elt = ELMT_FROM_HH((src)->hh_src.tbl, _src_hh);		       \
					if(cond(_elt)) {						      \
						_dst_hh = (UT_hash_handle*)(((char*)_elt) + _dst_hho);		     \
						_dst_hh->key = _src_hh->key;					     \
						_dst_hh->keylen = _src_hh->keylen;				     \
						_dst_hh->hashv = _src_hh->hashv;				     \
						_dst_hh->prev = _last_elt;					     \
						_dst_hh->next = NULL;						     \
						if(_last_elt_hh) { _last_elt_hh->next = _elt; }			    \
						if(!dst) {							    \
							DECLTYPE_ASSIGN(dst, _elt);					    \
							HASH_MAKE_TABLE(hh_dst, dst);					    \
						} else {							     \
							_dst_hh->tbl = (dst)->hh_dst.tbl;				   \
						}								     \
						HASH_TO_BKT(_dst_hh->hashv, _dst_hh->tbl->num_buckets, _dst_bkt);    \
						HASH_ADD_TO_BKT(_dst_hh->tbl->buckets[_dst_bkt], _dst_hh);	      \
						(dst)->hh_dst.tbl->num_items++;					     \
						_last_elt = _elt;						     \
						_last_elt_hh = _dst_hh;						     \
					}								       \
				}									   \
			}									     \
		}									       \
		HASH_FSCK(hh_dst, dst);								\
	} while(0)

#ifdef NO_DECLTYPE
#define HASH_ITER(hh, head, el, tmp)						    \
	for((el) = (head), (*(char**)(&(tmp))) = (char*)((head) ? (head)->hh.next : NULL);	 \
	    el; (el) = (tmp), (*(char**)(&(tmp))) = (char*)((tmp) ? (tmp)->hh.next : NULL))
#else
#define HASH_ITER(hh, head, el, tmp)						    \
	for((el) = (head), (tmp) = DECLTYPE(el) ((head) ? (head)->hh.next : NULL);		   \
	    el; (el) = (tmp), (tmp) = DECLTYPE(el) ((tmp) ? (tmp)->hh.next : NULL))
#endif

/* obtain a count of items in the hash */
#define HASH_COUNT(head) HASH_CNT(hh, head)
#define HASH_CNT(hh, head) ((head) ? ((head)->hh.tbl->num_items) : 0)

#endif // } 0
