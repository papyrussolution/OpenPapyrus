// PPIFCIMP.H
// Copyright (c) A.Sobolev, 2007
//
#ifndef __PPIFCIMP_H // {
#define __PPIFCIMP_H

#include <..\Rsrc\DL600\ppifc.h>

SDateRange FASTCALL DateRangeToOleDateRange(DateRange period);
DateRange  FASTCALL OleDateRangeToDateRange(const SDateRange & rRnd);
SIterCounter GetPPViewIterCounter(long ppviewPtr, int * pAppError);
IUnknown * GetPPObjIStrAssocList(SCoClass * pCls, PPObject * pObj, long extraParam);

#endif // __PPIFCIMP_H
