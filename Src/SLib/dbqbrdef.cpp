// DBQBRO.CPP
// Copyright (c) Sobolev A. 1996, 1997-2000, 2003, 2005, 2008, 2010, 2011, 2013, 2018, 2019, 2020, 2022, 2025
//
#include <slib-internal.h>
#pragma hdrstop

DBQBrowserDef::DBQBrowserDef(DBQuery & rQuery, int captionHight, uint aOptions, uint aBufSize) : 
	BrowserDef(BrowserDefSignature_DBQ, captionHight, aOptions), P_Query(0)
{
	setQuery(rQuery, aBufSize);
}

DBQBrowserDef::~DBQBrowserDef()
{
	if(options & BRO_OWNER)
		delete P_Query;
}

int DBQBrowserDef::setQuery(DBQuery & rQuery, uint aBufSize)
{
	const uint prev_view_height = (P_Query && P_Query->P_Frame) ? P_Query->P_Frame->Height : 1;
	if(options & BRO_OWNER)
		ZDELETE(P_Query);
	if(&rQuery) {
		P_Query = &rQuery;
		P_Query->setFrame(prev_view_height, aBufSize, 1);
		P_Query->P_Frame->SRange = 1000;
		P_Query->top();
	}
	return 1;
}

/*virtual*/int DBQBrowserDef::insertColumn(int atPos, const char * pTxt, uint fldNo, TYPEID typ, long fmt, uint opt)
{
	int    ok = 1;
	BroColumn bc;
	bc.OrgOffs = fldNo;
	if(fldNo == UNDEF)
		fldNo = getCount();
	if(opt & BCO_USERPROC) { // @v12.1.8 
		bc.T = typ;
		bc.Offs = fldNo;
	}
	else {
		assert(fldNo < P_Query->fldCount);
		bc.T = P_Query->flds[fldNo].type;
		bc.Offs = 0;
		for(uint i = 0; i < fldNo; i++)
			bc.Offs += stsize(P_Query->flds[i].type);
	}
	bc.format = fmt;
	bc.Options = (opt|BCO_CAPLEFT);
	bc.text = newStr(pTxt);
	if(addColumn(&bc, (atPos >= 0) ? atPos : UNDEF)) {
		if(GETSTYPE(typ))
			at(getCount()-1).T = typ;
		if(atPos >= 0 && P_Groups) {
			for(uint j = 0; j < NumGroups; j++) {
				BroGroup & r_grp = P_Groups[j];
				if(atPos <= static_cast<int>(r_grp.First))
					r_grp.First++;
				else if(atPos > static_cast<int>(r_grp.First) && atPos <= static_cast<int>(r_grp.NextColumn()))
					r_grp.Count++;
			}
		}
	}
	else
		ok = 0;
	return ok;
}

/*virtual*/int DBQBrowserDef::insertColumn(int atPos, const char * pTxt, const char * pFldName, TYPEID typ, long fmt, uint opt)
{
	uint   fld_no = 0;
	return (P_Query->getFieldPosByName(pFldName, &fld_no) > 0) ? insertColumn(atPos, pTxt, fld_no, typ, fmt, opt) : 0;
}

void DBQBrowserDef::setViewHight(int h)
{
	P_Query->setFrame(h, UNDEF, UNDEF);
	BrowserDef::setViewHight(h);
}

void DBQBrowserDef::getScrollData(long * pScrollDelta, long * pScrollPos)
{
	*pScrollDelta = static_cast<long>(P_Query->P_Frame->SDelta);
	//*pScrollPos = (long)query->P_Frame->SPos;
	*pScrollPos = 500L;
}

void DBQBrowserDef::setupView()
{
	topItem = P_Query->P_Frame->Top;
	curItem = P_Query->P_Frame->Cur;
	isBOQ = LOGIC(P_Query->P_Frame->State & DBQuery::Frame::stTop);
	isEOQ = LOGIC(P_Query->P_Frame->State & DBQuery::Frame::stBottom);
}

int DBQBrowserDef::step(long d)
{
	const int r = P_Query->step(d);
	setupView();
	return r;
}

int DBQBrowserDef::top()
{
	const int r = P_Query->top();
	setupView();
	return r;
}

int DBQBrowserDef::bottom()
{
	const int r = P_Query->bottom();
	setupView();
	return r;
}

int DBQBrowserDef::refresh()
{
	const int r = P_Query->refresh();
	setupView();
	return r;
}

bool   DBQBrowserDef::IsValid() const { return !P_Query->Error_; }
int    DBQBrowserDef::go(long p) { return step(p-curItem); }
long   DBQBrowserDef::GetRecsCount() const { return P_Query->P_Frame->SRange+1; }
const  void * DBQBrowserDef::getRow(long r) const { return P_Query->getRecord(static_cast<uint>(r)); }
