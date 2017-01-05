// _REPORT.H
// Copyright (c) A.Sobolev 1996
//
#ifndef ___REPORT_H
#define ___REPORT_H

#include <stdio.h>
#ifndef __SLIB_H
	#include <slib.h>
#endif

struct SRptVar {
	char   name[20];
	int    id;
	TYPEID type;
	long   format;
	uint   fldfmt;
};

inline IMPL_CMPFUNC(SRptVar, p1, p2)
{
	return stricmp((char*)p1, (char*)p2);
}

class VarTable : public TSArray <SRptVar> {
public:
	int SLAPI searchVar(char * name, uint * pos)
	{
		return bsearch(name, pos, PTR_CMPFUNC(SRptVar));
	}
	int SLAPI addVar(SRptVar * var, uint * pos)
	{
		return searchVar(var->name, pos) ? -1 :
			(var->id = 0, ordInsert(var, pos, PTR_CMPFUNC(SRptVar)));
	}
};

extern VarTable vartab;

void SLAPI accept_report(SReport *);

#endif /* ___REPORT_H */

