// DBQBRO.CPP
// Copyright (c) Sobolev A. 1996, 1997-2000, 2003, 2005, 2008, 2010, 2011, 2013, 2018, 2019, 2020
//
#include <slib-internal.h>
#pragma hdrstop
#include <db.h>

DBQBrowserDef::DBQBrowserDef(DBQuery & rQuery, int captionHight, uint aOptions, uint aBufSize) :
	BrowserDef(captionHight, aOptions), query(0)
{
	setQuery(rQuery, aBufSize);
}

DBQBrowserDef::~DBQBrowserDef()
{
	if(options & BRO_OWNER)
		delete query;
}

int DBQBrowserDef::setQuery(DBQuery & rQuery, uint aBufSize)
{
	uint   prev_view_height = 1;
	if(query && query->P_Frame)
		prev_view_height = query->P_Frame->hight;
	if(options & BRO_OWNER)
		ZDELETE(query);
	if(&rQuery) {
		query = &rQuery;
		query->setFrame(prev_view_height, aBufSize, 1);
		query->P_Frame->srange = 1000;
		query->top();
	}
	return 1;
}

/*virtual*/int DBQBrowserDef::insertColumn(int atPos, const char * pTxt, uint fldNo, TYPEID typ, long fmt, uint opt)
{
	BroColumn bc;
	bc.OrgOffs = fldNo;
	if(fldNo == UNDEF)
		fldNo = getCount();
	assert(fldNo<query->fldCount);
	bc.T = query->flds[fldNo].type;
	bc.Offs = 0;
	for(uint i = 0; i < fldNo; i++)
		bc.Offs += stsize(query->flds[i].type);
	bc.format = fmt;
	bc.Options = (opt | BCO_CAPLEFT);
	bc.text = newStr(pTxt);
	if(addColumn(&bc, (atPos >= 0) ? atPos : UNDEF)) {
		if(GETSTYPE(typ))
			at(getCount()-1).T = typ;
		if(atPos >= 0 && P_Groups)
			for(uint j = 0; j < NumGroups; j++) {
				BroGroup & grp = P_Groups[j];
				if(atPos <= static_cast<int>(grp.First))
					grp.First++;
				else if(atPos > static_cast<int>(grp.First) && atPos <= static_cast<int>(grp.NextColumn()))
					grp.Count++;
			}
		return 1;
	}
	else
		return 0;
}

/*virtual*/int DBQBrowserDef::insertColumn(int atPos, const char * pTxt, const char * pFldName, TYPEID typ, long fmt, uint opt)
{
	uint   fld_no = 0;
	return (query->getFieldPosByName(pFldName, &fld_no) > 0) ? insertColumn(atPos, pTxt, fld_no, typ, fmt, opt) : 0;
}

void DBQBrowserDef::setViewHight(int h)
{
	query->setFrame(h, UNDEF, UNDEF);
	BrowserDef::setViewHight(h);
}

void DBQBrowserDef::getScrollData(long * pScrollDelta, long * pScrollPos)
{
	*pScrollDelta = static_cast<long>(query->P_Frame->sdelta);
	//*pScrollPos = (long)query->P_Frame->spos;
	*pScrollPos = 500L;
}

void DBQBrowserDef::setupView()
{
	topItem = query->P_Frame->top;
	curItem = query->P_Frame->cur;
	isBOQ = query->P_Frame->state & DBQuery::Frame::Top;
	isEOQ = query->P_Frame->state & DBQuery::Frame::Bottom;
}

int FASTCALL DBQBrowserDef::step(long d)
{
	int r = query->step(d);
	setupView();
	return r;
}

int DBQBrowserDef::top()
{
	int r = query->top();
	setupView();
	return r;
}

int DBQBrowserDef::bottom()
{
	int r = query->bottom();
	setupView();
	return r;
}

int DBQBrowserDef::refresh()
{
	int r = query->refresh();
	setupView();
	return r;
}

int    DBQBrowserDef::valid() { return !query->error; }
int    FASTCALL DBQBrowserDef::go(long p) { return step(p-curItem); }
long   DBQBrowserDef::getRecsCount() { return query->P_Frame->srange+1; }
const  void * FASTCALL DBQBrowserDef::getRow(long r) const { return query->getRecord(static_cast<uint>(r)); }
// @v10.9.0 int    FASTCALL DBQBrowserDef::getData(void *) { return 1; }
// @v10.9.0 int    FASTCALL DBQBrowserDef::setData(void *) { return 1; }
