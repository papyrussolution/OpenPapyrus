// DBQBRO.CPP
// Copyright (c) Sobolev A. 1996, 1997-2000, 2003, 2005, 2008, 2010, 2011, 2013, 2018
//
#include <db.h>
#pragma hdrstop
#include <tv.h>

SLAPI DBQBrowserDef::DBQBrowserDef(DBQuery & rQuery, int captionHight, uint aOptions, uint aBufSize) :
	BrowserDef(captionHight, aOptions), query(0)
{
	setQuery(rQuery, aBufSize);
}

SLAPI DBQBrowserDef::~DBQBrowserDef()
{
	if(options & BRO_OWNER)
		delete query;
}

int SLAPI DBQBrowserDef::setQuery(DBQuery & rQuery, uint aBufSize)
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

// virtual
int SLAPI DBQBrowserDef::insertColumn(int atPos, const char * pTxt, uint fldNo, TYPEID typ, long fmt, uint opt)
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
				if(atPos <= (int)grp.first)
					grp.first++;
				else if(atPos > (int)grp.first && atPos <= (int)grp.NextColumn())
					grp.count++;
			}
		return 1;
	}
	else
		return 0;
}

//virtual
int SLAPI DBQBrowserDef::insertColumn(int atPos, const char * pTxt, const char * pFldName, TYPEID typ, long fmt, uint opt)
{
	uint   fld_no = 0;
	return (query->getFieldPosByName(pFldName, &fld_no) > 0) ? insertColumn(atPos, pTxt, fld_no, typ, fmt, opt) : 0;
}

void SLAPI DBQBrowserDef::setViewHight(int h)
{
	query->setFrame(h, UNDEF, UNDEF);
	BrowserDef::setViewHight(h);
}

void SLAPI DBQBrowserDef::getScrollData(long * pScrollDelta, long * pScrollPos)
{
	*pScrollDelta = (long)query->P_Frame->sdelta;
	//*pScrollPos = (long)query->P_Frame->spos;
	*pScrollPos = 500L;
}

int SLAPI DBQBrowserDef::valid()
{
	return !query->error;
}

void SLAPI DBQBrowserDef::setupView()
{
	topItem = query->P_Frame->top;
	curItem = query->P_Frame->cur;
	isBOQ = query->P_Frame->state & DBQuery::Frame::Top;
	isEOQ = query->P_Frame->state & DBQuery::Frame::Bottom;
}

int FASTCALL DBQBrowserDef::go(long p)
{
	return step(p-curItem);
}

int FASTCALL DBQBrowserDef::step(long d)
{
	int r = query->step(d);
	setupView();
	return r;
}

int SLAPI DBQBrowserDef::top()
{
	int r = query->top();
	setupView();
	return r;
}

int SLAPI DBQBrowserDef::bottom()
{
	int r = query->bottom();
	setupView();
	return r;
}

int SLAPI DBQBrowserDef::refresh()
{
	int r = query->refresh();
	setupView();
	return r;
}

long  SLAPI DBQBrowserDef::getRecsCount()
{
	return query->P_Frame->srange+1;
}

void * FASTCALL DBQBrowserDef::getRow(long r)
{
	return query->getRecord((uint)r);
}

int FASTCALL DBQBrowserDef::getData(void *)
{
	return 1;
}

int FASTCALL DBQBrowserDef::setData(void *)
{
	return 1;
}
