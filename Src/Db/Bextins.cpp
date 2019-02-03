// BEXTINS.CPP
// Copyright (c) A.Sobolev 1997-1999, 2000, 2004, 2008, 2009, 2010, 2015, 2017, 2018
// @codepage UTF-8
// Поддержка операции расширенной вставки записей
//
#include <slib.h>
#include <tv.h>
#pragma hdrstop

const size_t BExtInsert::DefBufSize = (28*1024U);

SLAPI BExtInsert::BExtInsert(DBTable * pTbl, size_t aBufSize) : SdRecordBuffer(NZOR(aBufSize, DefBufSize)), State(stValid), P_Tbl(pTbl), ActualCount(0xffffU)
{
	if(!GetBuf().P_Buf)
		State &= ~stValid;
	if(!P_Tbl) {
		FixRecSize = 0;
		State &= ~stValid;
	}
	else {
		FixRecSize = P_Tbl->getRecSize();
		if(P_Tbl->HasNote(0) > 0)
			State |= stHasNote;
	}
}

SLAPI BExtInsert::~BExtInsert()
{
	flash();
}

int FASTCALL BExtInsert::insert(const void * b)
{
	if(State & stValid) {
		size_t s;
		if(State & stHasNote) {
			const uchar * p_note = PTR8C(b) + FixRecSize;
			size_t note_len = p_note[0] ? (sstrlen(p_note)+1) : 0;
			s = FixRecSize + note_len;
		}
		else
			s = FixRecSize;
		int    r = Add(b, s);
		if(r < 0)
			r = flash() ? Add(b, s) : 0;
		if(!r)
			State &= ~stValid;
	}
	return BIN(State & stValid);
}

int SLAPI BExtInsert::flash()
{
	if(State & stValid && GetCount()) {
		int    ok = 1;
		if(P_Tbl->GetDb()) {
			ok = P_Tbl->GetDb()->Implement_BExtInsert(this);
		}
		else {
			ok = P_Tbl->Btr_Implement_BExtInsert(this);
		}
		SETFLAG(State, stValid, ok);
		ActualCount = (State & stValid) ? GetCount() : 0xffffU;
		Reset();
	}
	return BIN(State & stValid);
}

DBTable * SLAPI BExtInsert::getTable()
{
	return P_Tbl;
}

uint SLAPI BExtInsert::getActualCount() const
{
	return ActualCount;
}

