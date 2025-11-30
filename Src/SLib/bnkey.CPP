// BNKEY.CPP
// Copyright (c) Sobolev A. 1995, 1996, 1997, 1998, 2003, 2004, 2007, 2008, 2009, 2010, 2013, 2015, 2016, 2017, 2018, 2019, 2020, 2022, 2024, 2025
// @codepage UTF-8
//
#include <slib-internal.h>
#pragma hdrstop

struct BNKeyHeader { // @flat
	int32  numSeg;
	int16  keyNumber;
	int16  acsNumber;
};

struct BNKeySeg { // @flat
	int32  fldID;          // Field index in DBTable::fields
	int32  flags;
};

struct BNIndexHeader { // @flat
	int32  numKeys;
	uint32 tblBackOffset; // offsetof(DBTable, indexes)
};

union BNKeyCell { // @flat
	BNIndexHeader ih;
	BNKeyHeader   h;
	BNKeySeg      s;
};
//
// BNKey implementation
//
#define SEG(i)   static_cast<BNKeyCell *>(data)[(i)+1].s
#define HEAD     static_cast<BNKeyCell *>(data)[0].h
#define NUMCELLS (data ? (HEAD.numSeg + 1) : 0)
#define DATALEN  (NUMCELLS * sizeof(BNKeyCell))

BNKey::BNKey() : data(0)
{
}

void BNKey::destroy()
{
	ZFREE(data);
}

int BNKey::reset()
{
	data = SAlloc::R(data, sizeof(BNKeyCell));
	if(data) {
		HEAD.numSeg = HEAD.keyNumber = HEAD.acsNumber = 0;
		return 1;
	}
	else
		return 0;
}

int BNKey::addSegment(int fldID, int flags)
{
	BNKeyCell cell;
	if(!data)
		reset();
	const uint pos = NUMCELLS;
	for(int i = pos - 2; i >= 0; i--) {
		if(SEG(i).fldID == fldID)
			return SLS.SetError(SLERR_DUPIDXSEG);
	}
	cell.s.fldID = fldID;
	cell.s.flags = flags;
	data = SAlloc::R(data, sizeof(BNKeyCell) * (pos + 1));
	if(data) {
		memcpy(PTR8(data) + sizeof(BNKeyCell) * pos, &cell, sizeof(BNKeyCell));
		HEAD.numSeg++;
		return 1;
	}
	else
		return 0;
}

int BNKey::setFieldID(int seg, int newID)
{
	if(data && seg >= 0 && seg < HEAD.numSeg) {
		SEG(seg).fldID = newID;
		return 1;
	}
	else
		return 0;
}

int BNKey::setKeyParams(int keyNumber, int acsNumber)
{
	if(data) {
		if(keyNumber != (int)UNDEF)
			HEAD.keyNumber = static_cast<int16>(keyNumber);
		if(acsNumber != (int)UNDEF)
			HEAD.acsNumber = static_cast<int16>(acsNumber);
		return 1;
	}
	else
		return 0;
}

int BNKey::getNumSeg() const { return data ? HEAD.numSeg : 0; }
int BNKey::getKeyNumber() const { return data ? HEAD.keyNumber : UNDEF; }
int BNKey::getACSNumber() const { return data ? HEAD.acsNumber : UNDEF; }
int BNKey::getFieldID(int seg) const { return (data && seg >= 0 && seg < HEAD.numSeg) ? SEG(seg).fldID : UNDEF; }

int BNKey::getFlags(int seg) const
{
	if(seg == static_cast<int>(UNDEF))
		seg = 0;
	return (data && seg >= 0 && seg < HEAD.numSeg) ? SEG(seg).flags : UNDEF;
}

int BNKey::setFlags(int flags, int seg)
{
	int    ok = 0;
	if(data) {
		const int _ns = HEAD.numSeg;
		if(seg == static_cast<int>(UNDEF)) {
			for(int i = 0; i < _ns; i++) {
				int32 * f = &SEG(i).flags;
				*f &= ~XIF_ALLSEGFLAGS;
				*f |= flags;
				SETFLAG(*f, XIF_SEG, i < (_ns - 1));
			}
			ok = 1;
		}
		else if(seg >= 0 && seg < _ns) {
			if(seg < (_ns - 1))
				flags |= XIF_SEG;
			SEG(seg).flags |= (flags & ~XIF_ALLSEGFLAGS);
			ok = 1;
		}
	}
	return ok;
}

int BNKey::containsField(int fldID, int * pSeg) const
{
	if(data) {
		const int _ns = HEAD.numSeg;
		for(int i = 0; i < _ns; i++)
			if(SEG(i).fldID == fldID) {
				*pSeg = i;
				return 1;
			}
	}
	return 0;
}
//
// BNKeyList implementation
//
BNKeyList::BNKeyList() : P_Data(SAlloc::M(sizeof(BNKeyCell)))
{
	if(P_Data) {
		BNKeyCell * p_cell = static_cast<BNKeyCell *>(P_Data);
		p_cell->ih.numKeys = 0;
		p_cell->ih.tblBackOffset = 0;
	}
}

BNKeyList::BNKeyList(const BNKeyList & rS) : P_Data(0)
{
	copy(&rS);
}

BNKeyList::~BNKeyList()
{
	ZFREE(P_Data);
}

BNKeyList & BNKeyList::Z()
{
	ZFREE(P_Data);
	return *this;
}

BNKeyList & FASTCALL BNKeyList::operator = (const BNKeyList & s)
{
	copy(&s);
	return *this;
}

int FASTCALL BNKeyList::copy(const BNKeyList * src)
{
	Z();
	const uint sz = sizeof(BNKeyCell) * src->GetNumCells();
	if(sz) {
		P_Data = SAlloc::M(sz);
		if(P_Data)
			memcpy(P_Data, src->P_Data, sz);
		else
			return 0;
	}
	return 1;
}

int BNKeyList::Serialize(int dir, SBuffer & rBuf, SSerializeContext * /*pCtx*/)
{
	int    ok = 1;
	uint32 sz;
	if(dir > 0) {
		sz = sizeof(BNKeyCell) * GetNumCells();
		THROW(rBuf.Write(sz));
		if(sz)
			THROW(rBuf.Write(P_Data, sz));
	}
	else if(dir < 0) {
		Z();
		THROW(rBuf.Read(sz));
		if(sz) {
			P_Data = SAlloc::R(P_Data, sz);
			THROW(rBuf.Read(P_Data, sz));
		}
	}
	CATCHZOK
	return ok;
}

uint BNKeyList::GetNumCells() const
{
	uint   count = 0;
	if(P_Data) {
		const  BNKeyCell * cells = static_cast<const BNKeyCell *>(P_Data);
		int    num_keys = cells[count++].ih.numKeys;
		for(int i = 0; i < num_keys; i++)
			count += (cells[count].h.numSeg + 1);
	}
	return count;
}

int FASTCALL BNKeyList::findKeyByNumber(int keyNumber) const
{
	int    count = 0;
	if(P_Data) {
#define cells (static_cast<const BNKeyCell *>(P_Data))
		const int num_keys = cells[count++].ih.numKeys;
		for(int i = 0; i < num_keys; i++)
			if(keyNumber == cells[count].h.keyNumber)
				return count;
			else
				count += (cells[count].h.numSeg + 1);
#undef cells
	}
	return count;
}

int FASTCALL BNKeyList::findKey(int key) const
{
	int    count = 0;
	if(P_Data) {
#define cells (static_cast<const BNKeyCell *>(P_Data))
		const int num_keys = cells[count++].ih.numKeys;
		for(int i = 0; i < num_keys; i++)
			if(key == i)
				return count;
			else
				count += (cells[count].h.numSeg + 1);
#undef cells
	}
	return 0;
}

uint BNKeyList::getNumKeys() const
{
	return P_Data ? static_cast<uint>((static_cast<BNKeyCell *>(P_Data))->ih.numKeys) : 0;
}

int FASTCALL BNKeyList::addKey(BNKey & key)
{
	int    ok = 1;
	BNKeyCell * kc = static_cast<BNKeyCell *>(key.data);
	if(kc) {
		SArray ary(P_Data, sizeof(BNKeyCell), GetNumCells());
		if(P_Data == 0) {
			BNKeyCell cell;
			cell.ih.numKeys = 0;
			cell.ih.tblBackOffset = 0;
			ok = ary.insert(&cell);
		}
		for(int i = 0; ok && i < kc->h.numSeg + 1; i++) {
			if(i > 0) {
				if(i < kc->h.numSeg)
					kc[i].s.flags |= XIF_SEG;
				else
					kc[i].s.flags &= ~XIF_SEG;
			}
			ok = ary.insert(kc + i);
			if(!ok) {
				for(int j = i-1; j >= 0; i--)	// Rollback work
					ary.atFree(j);
			}
		}
		P_Data = ary.dataPtr();
		if(ok)
			(static_cast<BNKeyCell *>(P_Data))->ih.numKeys++;
	}
	else
		ok = -1;
	key.destroy();
	return ok;
}

int BNKeyList::removeKey(int key)
{
	int    ok = 1;
	int    pos = findKey(key);
	if(pos) {
		SArray ary(P_Data, sizeof(BNKeyCell), GetNumCells());
		const int num_seg = (static_cast<BNKeyCell *>(P_Data))[pos].h.numSeg;
		for(int i = 0; i < num_seg + 1; i++)
			ary.atFree(pos);
		P_Data = ary.dataPtr();
	}
	else
		ok = 0;
	return ok;
}

BNKey FASTCALL BNKeyList::getKey(int key) const
{
	int    pos = findKey(key);
	BNKey  k;
	k.data = pos ? ((static_cast<BNKeyCell *>(P_Data)) + pos) : 0;
	return k;
}

void FASTCALL BNKeyList::setTableRef(uint ofs)
{
	if(P_Data)
		static_cast<BNKeyCell *>(P_Data)->ih.tblBackOffset = ofs;
}

const DBTable & BNKeyList::table() const
{
	assert(P_Data);
	return *reinterpret_cast<const DBTable *>(reinterpret_cast<const char *>(this) - static_cast<BNKeyCell *>(P_Data)->ih.tblBackOffset);
}

const BNField & BNKeyList::field(int key, int seg) const
{
	if(P_Data) {
		const  DBTable * p_tbl = reinterpret_cast<const DBTable *>(reinterpret_cast<const char *>(this) - static_cast<BNKeyCell *>(P_Data)->ih.tblBackOffset);
		const  BNKey _k = getKey(key);
		const  int fld_id = _k.getFieldID(seg);
		const  BNFieldList2 & r_fld_list = p_tbl->GetFields();
		uint   fld_pos = 0;
		return r_fld_list.GetFieldPosition(fld_id, &fld_pos) ? r_fld_list.GetFieldByPosition(fld_pos) : *static_cast<const BNField *>(0);
	}
	else
		return *static_cast<BNField *>(0);
}

bool BNKeyList::HasAcsSegWithField(int fldId) const
{
	bool    ok = false;
	const   uint kc = getNumKeys();
	for(uint i = 0; !ok && i < kc; i++) {
		const BNKey key = getKey(i);
		for(uint si = 0; !ok && si < static_cast<uint>(key.getNumSeg()); si++) {
			const BNField & r_fld = field(i, si);
			if(r_fld.Id == fldId && key.getFlags(static_cast<int>(si)) & XIF_ACS)
				ok = true;
		}
	}
	return ok;
}

bool BNKeyList::GetKeyPosListByField(int fldId, LongArray * pPosList) const
{
	CALLPTRMEMB(pPosList, Z());
	bool    ok = false;
	const   uint kc = getNumKeys();
	for(uint i = 0; i < kc; i++) {
		const BNKey key = getKey(i);
		for(uint si = 0; si < static_cast<uint>(key.getNumSeg()); si++) {
			const BNField & r_fld = field(i, si);
			if(r_fld.Id == fldId) {
				CALLPTRMEMB(pPosList, add(static_cast<long>(i)));
				ok = true;
				break;
			}
		}
		if(ok && !pPosList) { // Если не куда скидывать позиции индексов, то, видимо, достатоно единственного прецедента дабы ответить клиент "да"
			break;
		}
	}
	return ok;
}

uint FASTCALL BNKeyList::getKeySize(int key) const
{
	BNKey  k = getKey(key);
	if(!k)
		return 0;
	else {
		uint   len = 0;
		const  int  ns = k.getNumSeg();
		for(int i = 0; i < ns; i++) {
#ifdef NDEBUG
			len += field(key, i).size();
#else
			const BNField & r_fld = field(key, i);
			len += r_fld.size();
#endif
		}
		return len;
	}
}

uint BNKeyList::getSegOffset(int key, int seg) const
{
	if(seg == 0)
		return 0;
	else {
		BNKey  k = getKey(key);
		if(!k)
			return 0;
		else {
			uint   ofs = 0;
			const int n = MIN(seg, k.getNumSeg()-1);
			//
			// Для ускорения закладываемся на то, что в индексе обычно
			// малое количество сегментов
			//
			#define MAX_CASE_SEG 5
			switch(n) {
				default:
					{
						for(int i = MAX_CASE_SEG; i < n; i++)
							ofs += field(key, i).size();
					}
				case (MAX_CASE_SEG-0): ofs += field(key, 4).size();
				case (MAX_CASE_SEG-1): ofs += field(key, 3).size();
				case (MAX_CASE_SEG-2): ofs += field(key, 2).size();
				case (MAX_CASE_SEG-3): ofs += field(key, 1).size();
				case (MAX_CASE_SEG-4): ofs += field(key, 0).size();
					break;
			}
			return ofs;
		}
	}
}
//
// Returns length of filled rest of key
//
uint BNKeyList::setBound(int key, int seg, int min_max /* 0 - min, !0 - max */ , void * pDest) const
{
	assert(key >= 0 && key < static_cast<int>(getNumKeys()));
	BNKey  k = getKey(key);
	uint   ofs = 0;
	const  int ns = k.getNumSeg();
	for(int i = seg; i < ns; i++) {
		const BNField & r_fld = table().GetFields()[k.getFieldID(i)];
		if(((k.getFlags(i) & XIF_DESC) ? !min_max : min_max) != 0)
			stmaxval(r_fld.T, PTR8(pDest) + ofs);
		else
			stminval(r_fld.T, PTR8(pDest) + ofs);
		ofs += r_fld.size();
	}
	return ofs;
}

#define INVALID_COMPARE_OPERATION 0

int FASTCALL _invertComp(int cmp)
{
	switch(cmp) {
		case _EQ_: return _EQ_;
		case _OUTER_EQ_: return _OUTER_EQ_;
		case _GT_: return _LT_;
		case _LT_: return _GT_;
		case _GE_: return _LE_;
		case _LE_: return _GE_;
		case _NE_: return _NE_;
	}
	//CHECK(INVALID_COMPARE_OPERATION);
	return 0;
}

int BNKeyList::makeKey(int key, int seg, int cmp, const void * val, void * dest) const
{
	struct I {
		uchar  low;
		uchar  upp;
	};
	union {
		char * d;
		I    * h;
	};
	int    use = 0;
	uint   ofs = 0;
	uint   segOfs = 0;
	BNKey  k  = getKey(key);
	uint   ss = table().GetFields()[k.getFieldID(seg)].size();
	int    ns = k.getNumSeg();
	uint   keyLen = 0;
	d = static_cast<char *>(dest);
	if(seg == 0) {
		use    = 1;
		segOfs = 0;
		ofs    = sizeof(I);
	}
	/*
		Второй и следующие сегменты могут быть использованы в поиске по ключу только
		в том случае, если существует двустороннее ограничение, причем предыдущие
		сегменты с обеих сторон этого ограничения равны между собой
	*/
	else if((h->low == _GT_ || h->low == _GE_) && (h->upp == _LT_ || h->upp == _LE_)) {
		uint o = sizeof(I);
		use = 2;
		keyLen = getKeySize(key);
		for(int i = 0; i < seg; i++) {
			const BNField & r_fld = table().GetFields()[k.getFieldID(i)];
			if(stcomp(r_fld.T, d + o, d + o + keyLen) != 0) {
				use = 0;
				break;
			}
			o += stsize(r_fld.T);
		}
		segOfs = o - sizeof(I);
		ofs = o;
	}
	if(use) {
		h->low = h->upp = 0;
		if(cmp == _EQ_) {
			memmove(d + ofs, val, ss);
			ofs += ss;
			if(ns > (seg + 1)) {
				ofs += setBound(key, seg + 1, 0, d + ofs);
				ofs += segOfs;
				memmove(d + ofs, val, ss);
				ofs += ss;
				ofs += setBound(key, seg + 1, 1, d + ofs);
				h->low = _GE_;
				h->upp = _LE_;
			}
			else
				h->low = _EQ_;
		}
		else {
			if(k.getFlags(seg) & XIF_DESC)
				cmp = _invertComp(cmp);
			if(oneof2(cmp, _GT_, _GE_)) {
				memmove(d + ofs, val, ss);
				ofs += ss;
				if(ns > (seg + 1))
					ofs += setBound(key, seg + 1, 0, d + ofs);
				h->low = static_cast<uchar>(cmp);
			}
			else if(oneof2(cmp, _LT_, _LE_)) {
				if(use == 2)
					ofs += keyLen;
				memmove(d + ofs, val, ss);
				ofs += ss;
				if(ns > (seg + 1))
					ofs += setBound(key, seg + 1, 1, d + ofs);
				if(use == 2)
					memmove(d + sizeof(I), d + sizeof(I) + keyLen, keyLen);
				h->upp = static_cast<uchar>(cmp);
			}
			else
				return 0;	// cmp == _NE_
		}
		return 1;
	}
	return 0;
}

int FASTCALL BNKeyList::compareKey(int key, const void * k1, const void * k2) const
{
	return getKey(key).compareKey(table(), k1, k2);
}

int BNKey::compareKey(const DBTable & rTbl, const void * k1, const void * k2) const
{
	int    ns = HEAD.numSeg;
	uint   ofs = 0;
	for(int i = 0; i < ns; i++) {
		BNKeySeg s = SEG(i);
		const  BNField & r_fld = rTbl.GetFields()[s.fldID];
		const  uint fld_data_size = r_fld.size();
		int    c = 0;
		/*@v12.4.7{*/
		if(GETSTYPE(r_fld.T) == S_ZSTRING && (s.flags & XIF_ACS)) {
			c = strnicmp866(PTRCHRC_(k1) + ofs, PTRCHRC_(k2) + ofs, fld_data_size);
		}
		else /*}@v12.4.7*/{
			c = stcomp(r_fld.T, PTR8C(k1) + ofs, PTR8C(k2) + ofs);
		}
		if(c)
			return (s.flags & XIF_DESC) ? -c : c;
		if(i < (ns - 1))
			ofs += fld_data_size;
	}
	return 0;
}

