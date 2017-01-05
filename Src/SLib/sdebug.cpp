// SDEBUG.CPP
// Copyright (c) A.Sobolev 2005, 2007, 2010, 2016
//
#include <slib.h>
#include <tv.h>
#pragma hdrstop
#include <malloc.h>
#include <crtdbg.h>

SLAPI MemLeakTracer::MemLeakTracer()
{
	P_State = new _CrtMemState;
	_CrtMemCheckpoint((_CrtMemState *)P_State); // @debug
}

SLAPI MemLeakTracer::~MemLeakTracer()
{
	_CrtMemDumpAllObjectsSince((_CrtMemState *)P_State);
	delete ((_CrtMemState *)P_State);
}
//
//
//
SLAPI MemHeapTracer::MemHeapTracer()
{
}

int SLAPI MemHeapTracer::CalcStat(Stat * pStat)
{
	memzero(pStat, sizeof(*pStat));
	_HEAPINFO blk;
	int    r = 0;
	blk._pentry = NULL;
	while((r = _heapwalk(&blk)) == _HEAPOK)
		if(blk._useflag == _USEDENTRY) {
			pStat->UsedBlockCount++;
			pStat->UsedSize += blk._size;
		}
		else {
			pStat->UnusedBlockCount++;
			pStat->UnusedSize += blk._size;
		}
	return (r == _HEAPBADPTR || r == _HEAPBADBEGIN || r == _HEAPBADNODE) ? 0 : 1;
}
//
//
//
int SLAPI TraceFunc(const char * pFuncName, const char * pAddedMsg)
{
	SString text;
	text = pFuncName;
	if(pAddedMsg)
		text.CatDiv(':', 1).Cat(pAddedMsg);
	SLS.LogMessage(0, text, 0);
	return 1;
}
