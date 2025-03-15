// SDEBUG.CPP
// Copyright (c) A.Sobolev 2005, 2007, 2010, 2016, 2017, 2020, 2021, 2023, 2024, 2025
//
#include <slib-internal.h>
#pragma hdrstop
// @v11.7.1 #include <malloc.h>
#include <crtdbg.h>

bool SlBreakpointCondition[16];

MemLeakTracer::MemLeakTracer()
{
	P_State = new _CrtMemState;
	_CrtMemCheckpoint((_CrtMemState *)P_State); // @debug
}

MemLeakTracer::~MemLeakTracer()
{
	_CrtMemDumpAllObjectsSince((_CrtMemState *)P_State);
	delete ((_CrtMemState *)P_State);
}
//
//
//
/*static*/bool MemHeapTracer::Check()
{
	MemHeapTracer mht;
	MemHeapTracer::Stat mht_stat;
	return mht.CalcStat(&mht_stat);
}

MemHeapTracer::MemHeapTracer()
{
}

bool MemHeapTracer::CalcStat(Stat * pStat)
{
	memzero(pStat, sizeof(*pStat));
	_HEAPINFO blk;
	int    r = 0;
	blk._pentry = NULL;
	while((r = _heapwalk(&blk)) == _HEAPOK) {
		if(blk._useflag == _USEDENTRY) {
			pStat->UsedBlockCount++;
			pStat->UsedSize += blk._size;
		}
		else {
			pStat->UnusedBlockCount++;
			pStat->UnusedSize += blk._size;
		}
	}
	return !oneof3(r, _HEAPBADPTR, _HEAPBADBEGIN, _HEAPBADNODE);
}
//
//
//
void FASTCALL TraceFunc(const char * pFuncName, const char * pAddedMsg)
{
	if(isempty(pAddedMsg)) {
		SLS.LogMessage(0, pFuncName, 0);
	}
	else {
		SString text(pFuncName);
		text.CatDiv(':', 1).Cat(pAddedMsg);
		SLS.LogMessage(0, text, 0);
	}
}

int  FASTCALL DummyFunc(void * ptr)
{
	(void)ptr;
	return 1;
}
