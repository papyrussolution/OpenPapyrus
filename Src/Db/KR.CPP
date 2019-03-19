// KR.CPP
// Copyright (c) Sobolev A. 1995, 1996, 1997, 1998, 2003, 2004, 2008, 2009, 2010, 2013, 2016, 2017, 2018, 2019
// @codepage windows-1251
//
#include <slib.h>
#include <tv.h>
#pragma hdrstop
#include <db.h>

SLAPI KR::KR() : P_b(0)
{
}

int SLAPI KR::operator !() const
{
	return (P_b == 0);
}

SLAPI KR::KR(int hdlTable, int keyPosition)
{
	init(hdlTable, keyPosition);
}

int SLAPI KR::init(int hdlTable, int keyPosition)
{
	P_h = static_cast<H *>(SAlloc::M(sizeof(H)));
	if(P_h == 0)
		return 0;
	else {
		P_h->keyNum   = keyPosition;
		P_h->pseg     = 0;
		P_h->len      = sizeof(H);
		P_h->count    = 0;
		P_h->current  = 0;
		P_h->hdlTable = hdlTable;
		//P_h->keyLen   = _GetTable(hdlTable)->indexes.getKeySize(keyPosition);
		P_h->keyLen   = _GetTable(hdlTable)->GetIndices().getKeySize(keyPosition);
		return 1;
	}
}

int SLAPI KR::trim()
{
	P_h = static_cast<H *>(SAlloc::R(P_h, sizeof(H)));
	if(P_h) {
		P_h->count = 0;
		P_h->len  = sizeof(H);
		P_h->current = 0;
		return 1;
	}
	else
		return 0;
}

int SLAPI KR::copy(KR src, int trim_data)
{
	destroy();
	if(src.P_h) {
		uint sz = trim_data ? sizeof(H) : src.P_h->len;
		P_b = static_cast<char *>(SAlloc::M(sz));
		if(P_b == 0)
			return 0;
		memcpy(P_b, src.P_b, sz);
		if(trim_data) {
			P_h->count = 0;
			P_h->len   = sizeof(H);
			P_h->current = 0;
		}
	}
	return 1;
}

void SLAPI KR::destroy()
{
	ZFREE(P_b);
}

uint FASTCALL KR::itemSize(const void * pItem) const
{
	return pItem ? (sizeof(I) + ((((I*)pItem)->low && ((I*)pItem)->upp) ? (P_h->keyLen << 1) : P_h->keyLen)) : 0;
}

int FASTCALL KR::walk(uint16 * pPos) const
{
	if(P_h->count == 0) { 
		*pPos = sizeof(H); 
		return 0; 
	}
	else if(*pPos == 0) { 
		*pPos = sizeof(H); 
		return 1; 
	}
	else
		return BIN(*pPos < P_h->len && (*pPos += itemSize(P_b + *pPos)) < P_h->len);
}

int SLAPI KR::first()
{
	P_h->current = 0;
	return walk(&P_h->current);
}

int SLAPI KR::last()
{
	uint16 pos = 0;
	uint16 prev = sizeof(H);
	while(walk(&pos))
		prev = pos;
	return ((P_h->current = prev) > 0);
}

int SLAPI KR::operator++()
{
	return walk(&P_h->current);
}

int SLAPI KR::operator--()
{
	if(P_h->current <= sizeof(H))
		return 0;
	else {
		uint16 pos  = 0;
		uint16 prev = 0;
		while(walk(&pos) && pos < P_h->current)
			prev = pos;
		P_h->current = prev;
		return 1;
	}
}

int FASTCALL KR::add(const void * item)
{
	uint   size = itemSize(item);
	P_b = static_cast<char *>(SAlloc::R(P_b, P_h->len + size));
	if(P_b == 0)
		return 0;
	if(P_h->current)
		memmove(P_b + P_h->current + size, P_b + P_h->current, P_h->len - P_h->current);
	else
		P_h->current = sizeof(H);
	memcpy(P_b + P_h->current, item, size);
	P_h->count++;
	P_h->len += size;
	return 1;
}

int SLAPI KR::remove()
{
	int    ok = 1;
	if(P_h->count && P_h->current < P_h->len) {
		uint   s = itemSize(P_b+P_h->current);
		memmove(P_b+P_h->current, P_b+P_h->current+s, P_h->len - P_h->current-s);
		P_b = static_cast<char *>(SAlloc::R(P_b, P_h->len-s));
		if(P_b) {
			P_h->count--;
			P_h->len -= s;
		}
		else
			ok = 0;
	}
	return ok;
}

void SLAPI KR::makeRange(void * d, __range & r)
{
	I * ih = (I*)d;
	r.ol = r.ou = 0;
	if(ih->low == 0)
		r.low = 0;
	else {
		r.low = ih+1;
		if(ih->low == _GT_)
			r.ol = 1;
	}
	if(ih->upp == 0)
		r.upp = (ih->low == _EQ_) ? ih+1 : 0;
	else {
		r.upp = ih->low ? ((char *)d + sizeof(I) + P_h->keyLen) : (char *)(ih+1);
		if(ih->upp == _LT_)
			r.ou = 1;
	}
}

void SLAPI KR::rangeToBuf(const __range & C, void * buf, int equal)
{
	KR::I & ih = *(KR::I*)buf;
	char * ptr = ((char *)buf)+sizeof(KR::I);
	uint keylen = P_h->keyLen;
	if(C.low == 0 || C.upp == 0 || equal) {
   		memcpy(ptr, C.low ? C.low : C.upp, keylen);
		if(equal)
			{ ih.low = _EQ_; ih.upp = 0; }
		else if(C.low)
			{ ih.low = C.ol ? _GT_ : _GE_; ih.upp = 0; }
		else
			{ ih.low = 0; ih.upp = C.ou ? _LT_ : _LE_; }
	}
	else {
		memcpy(ptr, C.low, keylen);
		memcpy(ptr+keylen, C.upp, keylen);
		ih.low = C.ol ? _GT_ : _GE_;
		ih.upp = C.ou ? _LT_ : _LE_;
	}
}

int SLAPI KR::conjunction(__range & A /*dest*/, const __range & B, int * comp, int * eq)
{
	int    c;
	__range C;
	const BNKeyList * keyList = &_GetTable(P_h->hdlTable)->GetIndices();
	if(A.low == 0 || B.low == 0)
		if(A.low)
			{ C.low = A.low; C.ol = A.ol; }
		else if(B.low)
			{ C.low = B.low; C.ol = B.ol; }
		else
			{ C.low = 0; C.ol = 0; }
	else if((c = keyList->compareKey(P_h->keyNum, A.low, B.low)) == 0)
		{ C.low = A.low; C.ol = MAX(A.ol, B.ol); }
	else if(c < 0)
		{ C.low = B.low; C.ol = B.ol; }
	else
		{ C.low = A.low; C.ol = A.ol; }
	if(A.upp == 0 || B.upp == 0)
		if(A.upp)
			{ C.upp = A.upp; C.ou = A.ou; }
		else if(B.upp)
			{ C.upp = B.upp; C.ou = B.ou; }
		else
			{ C.upp = 0; C.ou = 0; }
	else if((c = keyList->compareKey(P_h->keyNum, A.upp, B.upp)) == 0)
		{ C.upp = A.upp; C.ou = MAX(A.ou, B.ou); }
	else if(c > 0)
		{ C.upp = B.upp; C.ou = B.ou; }
	else
		{ C.upp = A.upp; C.ou = A.ou; }
	int cmp   = 0; // Result of comparing B with A
	int empty = 0;
	int equal = 0;
	if(C.low && C.upp) {
		if((c = keyList->compareKey(P_h->keyNum, C.low, C.upp)) > 0) {
			empty = 1;
			if(comp)
				if(B.upp == 0 || A.low == 0)
					cmp = 1;
				else if(B.low == 0 || A.upp == 0)
					cmp = -1;
				else {
					c = keyList->compareKey(P_h->keyNum, B.upp, A.low);
					cmp = (c <= 0) ? -1 : 1;
				}
		}
		else if(c == 0)
			if(C.ol != 0 || C.ol != C.ou)
				empty = 1;
			else
				equal = 1;
	}
	ASSIGN_PTR(comp, cmp);
	ASSIGN_PTR(eq, equal);
	A = C;
	return empty ? -1 : 1;
}

int SLAPI KR::conjunction(void * d, int assign, int * comp, int * eq)
{
	__range A, B;
	makeRange(P_b + P_h->current, A);
	makeRange(d, B);
	int empty = (conjunction(A, B, comp, eq) == -1);
	if(assign) {
		if(empty)
			return remove() ? -1 : 0;
		char buf[sizeof(I) + 2 * MAXKEYLEN];
		rangeToBuf(A, buf, *eq);
		return (remove() && add(buf)) ? 1 : 0;
	}
	return (empty ? -1 : 1);
}

int SLAPI KR::disjunction(void * d)
{
	char    buf[sizeof(I) + 2 * MAXKEYLEN];
	int     c, tc, eq;
	int     next;
	const BNKeyList * keyList = &_GetTable(P_h->hdlTable)->GetIndices();
	__range A, B, C;
	makeRange(d, B);
	if(first()) do {
		next = 0;
		makeRange(P_b + P_h->current, A);
		C = A;
		conjunction(C, B, &c, &eq);
		if(c == 0) {
			if(A.low == 0 || B.low == 0)
				{ C.low = 0; C.ol = 0; }
			else if((tc = keyList->compareKey(P_h->keyNum, A.low, B.low)) == 0)
				{ C.low = A.low; C.ol = MIN(A.ol, B.ol); }
			else if(tc < 0)
				{ C.low = A.low; C.ol = A.ol; }
			else
				{ C.low = B.low; C.ol = B.ol; }
			if(A.upp == 0 || B.upp == 0)
				{ C.upp = 0; C.ou = 0; }
			else if((tc = keyList->compareKey(P_h->keyNum, A.upp, B.upp)) == 0)
				{ C.upp = A.upp; C.ou = MIN(A.ou, B.ou); }
			else if(tc < 0)
				{ C.upp = A.upp; C.ou = A.ou; }
			else
				{ C.upp = B.upp; C.ou = B.ou; }
			if(C.low == 0 && C.upp == 0)
				return (trim(), -1);
			else {
				rangeToBuf(C, buf, eq);
				B = C;
				d = buf;
				if(!remove())
					return 0;
			}
		}
		else if(c < 0)
			return add(d);
		else
			next = operator++();
	} while(next);
	return add(d);
}
//
// returns:
// -2 full range
// -1 empty
// 0 error
// 1 ok
//
int SLAPI KR::link(int logic, KR kr)
{
	if(logic == _AND___) {
		if(kr.first()) {
			int    eq, empty = 1;
			KR     temp; 
			temp.copy(*this, 1);
			char   buf[sizeof(I) + 2 * MAXKEYLEN];
			do {
				__range A;
				makeRange(kr.P_b+kr.P_h->current, A);
				if(first()) {
					do {
						__range B;
						makeRange(P_b + P_h->current, B);
						if(conjunction(B, A, 0, &eq) != -1) {
							empty = 0;
							rangeToBuf(B, buf, eq);
							if(!temp.disjunction(buf))
								return (temp.destroy(), 0);
						}
					} while(operator++());
				}
				else {
					temp.destroy();
					return copy(kr) ? 1 : 0;
				}
			} while(++kr);
			int r = copy(temp);
			temp.destroy();
			return r ? (empty ? -1 : 1) : 0;
		}
	}
	// _OR_
	else if(kr.first())
		do {
			switch(disjunction(kr.P_b + kr.P_h->current)) {
				case 0:
					return 0;
				case -1:
					return -2;
				case 1:
					break;
			}
		} while(++kr);
	else
		return trim() ? -2 : 0;
	return 1;
}

#define INVALID_KR_ITEM 0

int SLAPI KR::getKey(void * d, int * sp)
{
	assert(sp && (*sp == spNext || *sp == spPrev));
	if(P_b == 0 || P_h->current == 0)
		return 0;
	else if(P_h->count == 0) {
		*sp = (*sp == spNext) ? spFirst : spLast;
		return 1;
	}
	else {
		const char * c = P_b + P_h->current;
		KR::I & ih = *(KR::I*)c;
		if(ih.low == _EQ_) {
			const int dup = (_GetTable(P_h->hdlTable)->GetIndices()[P_h->keyNum].getFlags() & (XIF_DUP|XIF_REPDUP));
			memcpy(d, c+sizeof(KR::I), P_h->keyLen);
			if(*sp == spNext)
				*sp = dup ? spEq : (spEq | ONLY_ONE_REC);
			else
				*sp = dup ? (spGt | SKIP_ONE_REC) : (spEq | ONLY_ONE_REC);
		}
		else if(ih.low == _GT_ || ih.low == _GE_) {
			if(*sp == spNext) {
				memcpy(d, c+sizeof(KR::I), P_h->keyLen);
				*sp = (ih.low == _GT_) ? spGt : spGe;
			}
			else /* (*sp == spPrev) */ if(ih.upp == _LT_ || ih.upp == _LE_) {
				memcpy(d, c+sizeof(KR::I)+P_h->keyLen, P_h->keyLen);
				*sp = (ih.upp == _LT_) ? spLt : spLe;
			}
			else
				*sp = spLast;
		}
		else { /* (ih.low == 0) */
			if(*sp == spNext)
				*sp = spFirst;
			else if(ih.upp == _LT_ || ih.upp == _LE_) {
				memcpy(d, c+sizeof(KR::I), P_h->keyLen);
				*sp = (ih.upp == _LT_) ? spLt : spLe;
			}
			else
				;//CHECK(INVALID_KR_ITEM); /* Такого быть не должно */
		}
		return 1;
	}
}

int FASTCALL KR::checkKey(const void * d) const
{
	if(P_h == 0 || P_h->current == 0)
		return 0;
	else if(P_h->count == 0)
		return 1;
	else {
		const char * c = P_b + P_h->current;
		const uchar  low = ((KR::I*)c)->low;
		const uchar  upp = ((KR::I*)c)->upp;
		c += sizeof(KR::I);
		const  BNKeyList * idxs = &_GetTable(P_h->hdlTable)->GetIndices();
		int    cmp = idxs->compareKey(P_h->keyNum, d, c);
		if(cmp > 0) {
			if(oneof2(low, _GE_, _GT_)) {
				if(upp == 0)
					return 1;
				else {
					cmp = idxs->compareKey(P_h->keyNum, d, c + P_h->keyLen);
					return (cmp < 0 || (cmp == 0 && upp == _LE_));
				}
			}
			else
				return 0;
		}
		else if(cmp < 0)
			return ((low == 0) && oneof2(upp, _LE_, _LT_));
		else
			return (low == _EQ_ || low == _GE_ || (low == 0 && upp == _LE_));
	}
}
/*
	Шкала предпочтительности ключей:
		  1 Только равенства по уникальному ключу
		201 - (количество сегментов по которым задано равенство)
			Присутствует(ют) равенства по неуникальному ключу
			Двухсторонние ограничения, представляющие равенства
			по первым сегментам ключа оцениваются как равенства
			по неуникальному ключу с поправкой на количество
			заполненных сегментов (это число вычитается из 201)
		201 Только двухсторонние ограничения по уникальному ключу
		301 Только двухсторонние ограничения по неуникальному ключу
		401 Присутствует(ют) односторонние неравенства по уникальному ключу
		501 Присутствует(ют) односторонние неравенства по неуникальному ключу
	Если у двух ключей рейтинг предпочтительности одинаков, то
	выбирается тот, который имеет меньше диапазонов ограничений
	(поле KR::h->count).

	Следующий критерий - длина ключа в байтах.

	Если и после этого ключи оказываются эквивалентны, то для //
	определенности предпочтем ключ с меньшим номером.
*/
int SLAPI KR::rating()
{
	if(this == 0 || P_b == 0 || !first())
		return 0;
	int      mark   = 1;
	DBTable * p_tbl = _GetTable(P_h->hdlTable);
	const BNKey key = p_tbl->GetIndices().getKey(P_h->keyNum);
	const int flags  = key.getFlags();
	const int unique = (flags & (XIF_DUP|XIF_REPDUP)) ? 0 : 1;
	do {
		int g;
		I * ih = (I*)(P_b + P_h->current);
		if(ih->low == _EQ_)
			g = unique ? 1 : (201 - key.getNumSeg());
		else if(ih->low && ih->upp) {
			int s = 0;
			if(ih->low == _GE_ && ih->upp == _LE_) {
				uint   ofs = 0;
				int    ns  = key.getNumSeg();
				char * c1  = P_b + P_h->current+sizeof(I);
				char * c2  = c1 + P_h->keyLen;
				for(int i = 0; i < ns; i++) {
					const BNField & f = p_tbl->GetFields()[key.getFieldID(i)];
					if(stcomp(f.T, c1+ofs, c2+ofs) == 0) {
						s++;
						ofs += stsize(f.T);
					}
					else
						break;
				}
			}
			if(s)
				g = 201 - s;
			else
				g = unique ? 201 : 301;
		}
		else
			g = unique ? 401 : 501;
		if(mark < g)
			mark = g;
	} while(operator++());
	return mark;
}
//
// Возвращает (> 0) если kr1 предпочтительнее чем kr2 и (< 0) в противном
// случае. Если вернул 0, то это означает, что оба ограничения не пригодны
// для ключевки.
//
int FASTCALL compare(KR kr1, KR kr2)
{
	const int r1 = kr1.rating();
	const int r2 = kr2.rating();
	if(r1 == 0)
		return r2 ? -1 : 0;
	else if(r2 == 0)
		return 1;
	else if(r1 == r2) {
		if(kr1.P_h->count == kr2.P_h->count) {
			if(kr1.P_h->keyLen == kr2.P_h->keyLen)
				return (kr2.P_h->keyNum - kr1.P_h->keyNum);
			else
				return (kr2.P_h->keyLen - kr1.P_h->keyLen);
		}
		else
			return (kr2.P_h->count - kr1.P_h->count);
	}
	else
		return (r2 - r1);
}
