// SBRODEF.CPP
// Copyright (c) Sobolev A. 1995, 1996, 1997, 2003, 2004, 2005, 2007, 2008, 2009, 2010, 2013, 2014, 2015, 2016, 2017
//
#include <slib.h>
#include <tv.h>
#pragma hdrstop
#include <db.h>
//
int SBrowserDataProcBlock::SetZero()
{
	PTR8(P_DestData)[0] = 0;
	return 1;
}

int FASTCALL SBrowserDataProcBlock::Set(int32 i)
{
    *(int32 *)P_DestData = i;
    return 1;
}

int SBrowserDataProcBlock::Set(double r)
{
    *(double *)P_DestData = r;
    return 1;
}

int FASTCALL SBrowserDataProcBlock::Set(const char * pS)
{
	strnzcpy((char *)P_DestData, pS, stsize(TypeID));
	return 1;
}

int FASTCALL SBrowserDataProcBlock::Set(const SString & rS)
{
	rS.CopyTo((char *)P_DestData, stsize(TypeID));
	return 1;
}

int FASTCALL SBrowserDataProcBlock::Set(LDATE dt)
{
	*(LDATE *)P_DestData = dt;
	return 1;
}

int FASTCALL SBrowserDataProcBlock::Set(LTIME tm)
{
	*(LTIME *)P_DestData = tm;
	return 1;
}
//
//
//
SLAPI BroGroup::BroGroup()
{
	THISZERO();
}

uint SLAPI BroGroup::NextColumn() const
{
	return (first+count);
}
//
//
//
SLAPI BrowserDef::BrowserDef(int captionHight, unsigned aOptions, void * extraPtr /*=0*/) :
	SArray(sizeof(BroColumn), 8, aryDataOwner|aryEachItem)
{
	capHight = captionHight;
	options = aOptions;
	NumGroups = 0;
	P_Groups = 0;
	viewHight = 1;
	topItem = curItem = 0;
	scrollDelta = 0;
	P_CtList = 0;
	ExtraPtr = extraPtr;
	UserProc = 0;
}

SLAPI BrowserDef::~BrowserDef()
{
	freeAll();
	if(P_Groups) {
		for(uint i = 0; i < NumGroups; i++)
			delete P_Groups[i].text;
		delete P_Groups;
	}
	if(P_CtList) {
		for(uint i = 0; i < P_CtList->getCount(); i++)
			delete ((BroCrosstab *)P_CtList->at(i))->P_Text;
		delete P_CtList;
	}
}

BroColumn & FASTCALL BrowserDef::at(uint pos) const
{
	return *(BroColumn *)SArray::at(pos);
}

#pragma warn -rng // Prevent warn "Constant out of range..." at (pos == UNDEF)

int SLAPI BrowserDef::addColumn(const BroColumn * pCol, int pos)
{
	if(pos == UNDEF)
		pos = count;
	BroColumn col = *pCol; // @attention Передача указателя BroColumn::text //
	SString subst;
	if(SLS.SubstString(col.text, 0, subst) > 0) {
		delete col.text;
		col.text = newStr(subst);
	}
	long   f = col.format;
	if(SFMTLEN(f) == 0)
		SETSFMTLEN(f, MIN_COLUMN_WIDTH);
	if(SFMTALIGN(f) == 0) {
		const int base = stbase(col.T);
		if(oneof2(base, BTS_INT, BTS_REAL))
			SETSFMTFLAG(f, SFMTFLAG(f) | ALIGN_RIGHT);
		else
			SETSFMTFLAG(f, SFMTFLAG(f) | ALIGN_LEFT);
	}
	col.width = SFMTLEN(f);
	col.format = f;
	return atInsert(pos, &col) ? initOffset(pos) : 0;
}

#pragma warn +rng

int FASTCALL BrowserDef::isColInGroup(uint col, uint * pIdx) const
{
	for(uint i = 0, c = NumGroups; i < c; i++) {
		const BroGroup * p_group = GetGroup(i);
		if(col >= p_group->first && col < p_group->NextColumn()) {
			*pIdx = i;
			return 1;
		}
	}
	return 0;
}

//virtual
int SLAPI BrowserDef::insertColumn(int, const char *, uint, TYPEID, long, uint)
	{ return 0; }
//virtual
int SLAPI BrowserDef::insertColumn(int atPos, const char * pTxt, const char * pFldName, TYPEID typ, long fmt, uint opt)
	{ return 0; }

int SLAPI BrowserDef::removeColumn(int pos)
{
	return atFree(pos);
}

int SLAPI BrowserDef::setColumnTitle(int colN, const char * pText)
{
	if(colN >= 0 && colN < (int)getCount()) {
		BroColumn & r_column = at(colN);
		ZDELETE(r_column.text);
		r_column.text = newStr(pText);
		return 1;
	}
	else
		return 0;
}

int SLAPI BrowserDef::initOffset(int pos)
{
	BroColumn & r_column = at(pos);
	if(r_column.Offs == UNDEF)
		if(pos > 0) {
			const BroColumn & r_prev = at(pos-1);
			r_column.Offs = r_prev.Offs + stsize(r_prev.T);
		}
		else
			r_column.Offs = 0;
	return 1;
}

int SLAPI BrowserDef::addGroup(BroGroup * pGroup)
{
	int    ok = 1;
	if(pGroup) {
		SArray array(P_Groups, sizeof(BroGroup), NumGroups);
		SString subst;
		if(SLS.SubstString(pGroup->text, 0, subst) > 0) {
			delete pGroup->text;
			pGroup->text = newStr(subst);
		}
		ok = array.ordInsert(pGroup, 0, PTR_CMPFUNC(int));
		P_Groups = (BroGroup *)array.dataPtr();
		NumGroups = array.getCount();
	}
	return ok;
}

int SLAPI BrowserDef::AddCrosstab(BroCrosstab * pCt)
{
	if(P_CtList == 0) {
		P_CtList = new SArray(sizeof(BroCrosstab));
		if(!P_CtList)
			return (SLibError = SLERR_NOMEM, 0);
	}
	return P_CtList->insert(pCt);
}

uint  SLAPI BrowserDef::GetCrosstabCount() const
	{ return P_CtList ? P_CtList->getCount() : 0; }
const BroCrosstab * SLAPI BrowserDef::GetCrosstab(uint ctN) const
	{ return (P_CtList && ctN < P_CtList->getCount()) ? (const BroCrosstab *)P_CtList->at(ctN) : 0; }

int SLAPI BrowserDef::FreeAllCrosstab()
{
	if(P_CtList) {
		for(uint i = 0; i < P_CtList->getCount(); i++)
			delete ((BroCrosstab *)P_CtList->at(i))->P_Text;
		P_CtList->freeAll();
		return 1;
	}
	else
		return -1;
}

void SLAPI BrowserDef::getScrollData(long * pScrollDelta, long * pScrollPos)
{
	*pScrollDelta = scrollDelta;
	*pScrollPos = curItem;
}

int SLAPI BrowserDef::GetCapHeight() const
	{ return capHight; }
void SLAPI BrowserDef::VerifyCapHeight()
	{ capHight = MIN(2, capHight); }
uint SLAPI BrowserDef::GetGroupCount() const
	{ return NumGroups; }
const BroGroup * FASTCALL BrowserDef::GetGroup(uint grpN) const
	{ return (grpN < NumGroups) ? (P_Groups+grpN) : 0; }

void SLAPI BrowserDef::ClearGroupIndexies()
{
	for(uint i = 0; i < NumGroups; i++)
		P_Groups[i].index = 0;
}

uint * SLAPI BrowserDef::GetGroupIndexPtr(uint grpN)
	{ return (grpN < NumGroups) ? &P_Groups[grpN].index : 0; }
int SLAPI BrowserDef::IsBOQ() const
	{ return isBOQ; }
int SLAPI BrowserDef::IsEOQ() const
	{ return isEOQ; }
int SLAPI BrowserDef::CheckFlag(uint f) const
	{ return (options & f) ? 1 : 0; }

//virtual
void FASTCALL BrowserDef::freeItem(void * pItem)
{
	if(pItem)
		delete ((BroColumn *)pItem)->text;
}

const BroGroup * SLAPI BrowserDef::groupOf(uint column, uint * pGrpPos) const
{
	for(uint i = 0; i < NumGroups; i++) {
		BroGroup * p_gr = P_Groups + i;
		if(column >= p_gr->first && column < p_gr->NextColumn()) {
			ASSIGN_PTR(pGrpPos, i);
			return p_gr;
		}
	}
	ASSIGN_PTR(pGrpPos, 0);
	return 0;
}

uint SLAPI BrowserDef::groupWidth(const BroGroup * pGroup, uint atColumn) const
{
	int    _n = 0, wd = 0;
	for(uint i = atColumn, c = pGroup->NextColumn(); i < c; i++) {
		wd += at(i).width;
		_n++;
	}
	wd += (_n - 1) * 3;
	return wd;
}

uint SLAPI BrowserDef::groupWidth(uint grpN, uint atColumn) const
	{ return groupWidth(P_Groups + grpN, atColumn); }
void SLAPI BrowserDef::setViewHight(int newHight)
	{ viewHight = newHight; }

int SLAPI BrowserDef::GetCellData(long row, int column, TYPEID * pType, void * pDataBuf, size_t dataBufLen)
{
	int    ok = 0;
	TYPEID typ = 0;
	char * p_src_data = (char *)getRow(row);
	if(p_src_data && column >= 0 && column < (int)getCount()) {
		const  BroColumn & c = at(column);
		int    r = 1;
		char   dest_data[256];
		DpB.ExtraPtr = ExtraPtr;
		DpB.ColumnN   = c.Offs;
		DpB.TypeID    = c.T;
		DpB.P_SrcData = p_src_data;
		DpB.Color     = 0;
		DpB.P_DestData = dest_data;
		if(c.Options & BCO_USERPROC) {
			if(UserProc)
				r = UserProc(&DpB);
			else if(c.UserProc)
				r = c.UserProc(&DpB);
			else
				r = 0;
		}
		else
			DpB.P_DestData = p_src_data + c.Offs;
		typ = DpB.TypeID;
		memcpy(pDataBuf, DpB.P_DestData, MIN(dataBufLen, stsize(typ)));
		ok = 1;
	}
	return ok;
}

char * SLAPI BrowserDef::getText(long row, int column, char * pBuf)
{
	if(pBuf) {
		*pBuf = 0;
		if(column >= 0 && column < (int)getCount()) {
			char * p_src_data = (char *)getRow(row);
			if(p_src_data) {
				const  BroColumn & c = at(column);
				int    r = 1;
				char   dest_data[256];
				DpB.ExtraPtr  = ExtraPtr;
				DpB.ColumnN   = c.Offs;
				DpB.TypeID    = c.T;
				DpB.P_SrcData = p_src_data;
				DpB.Color     = 0;
				DpB.P_DestData = dest_data;
				if(c.Options & BCO_USERPROC) {
					if(UserProc)
						r = UserProc(&DpB);
					else if(c.UserProc)
						r = c.UserProc(&DpB);
					else
						r = 0;
				}
				else
					DpB.P_DestData = p_src_data + c.Offs;
				if(r)
					sttostr(c.T, DpB.P_DestData, c.format, pBuf);
			}
		}
	}
	return pBuf;
}

SString & SLAPI BrowserDef::getFullText(const void * pRowData, int column, SString & rBuf)
{
	rBuf = 0;
	if(pRowData && column >= 0 && column < (int)getCount()) {
		const  BroColumn & r_c = at(column);
		int    r = 1;
		char   dest_data[1024];
		DpB.ExtraPtr  = ExtraPtr;
		DpB.ColumnN   = r_c.Offs;
		DpB.TypeID    = r_c.T;
		DpB.P_SrcData = pRowData;
		DpB.Color     = 0;
		DpB.P_DestData = dest_data;
		{
			const uint8 * p_bin_data = 0;
			if(r_c.Options & BCO_USERPROC) {
				if(UserProc)
					r = UserProc(&DpB);
				else if(r_c.UserProc)
					r = r_c.UserProc(&DpB);
				else
					r = 0;
				p_bin_data = PTR8(DpB.P_DestData);
			}
			else {
				//DpB.P_DestData = p_src_data + c.Offs;
				p_bin_data = PTR8(pRowData) + r_c.Offs;
			}
			if(r) {
				char   dest_text[1024];
				dest_text[0] = 0;
				long   fmt = r_c.format;
				SETSFMTLEN(fmt, 0);
				sttostr(r_c.T, p_bin_data, fmt, dest_text);
				rBuf = dest_text;
			}
		}
	}
	return rBuf;
}

SString & SLAPI BrowserDef::getFullText(long row, int column, SString & rBuf)
{
	return getFullText(getRow(row), column, rBuf); // @v9.5.5
	/* @v9.5.5
	rBuf = 0;
	const char * p_src_data = (const char *)getRow(row);
	if(p_src_data && column >= 0 && column < (int)getCount()) {
		const  BroColumn & c = at(column);
		int    r = 1;
		char   dest_data[1024];
		DpB.ExtraPtr  = ExtraPtr;
		DpB.ColumnN   = c.Offs;
		DpB.TypeID    = c.T;
		DpB.P_SrcData = p_src_data;
		DpB.Color     = 0;
		DpB.P_DestData = dest_data;
		{
			const char * p_bin_data = 0;
			if(c.Options & BCO_USERPROC) {
				if(UserProc)
					r = UserProc(&DpB);
				else if(c.UserProc)
					r = c.UserProc(&DpB);
				else
					r = 0;
				p_bin_data = (const char *)DpB.P_DestData;
			}
			else {
				//DpB.P_DestData = p_src_data + c.Offs;
				p_bin_data = p_src_data + c.Offs;
			}
			if(r) {
				char   dest_text[1024];
				dest_text[0] = 0;
				long   fmt = c.format;
				SETSFMTLEN(fmt, 0);
				sttostr(c.T, p_bin_data, fmt, dest_text);
				rBuf = dest_text;
			}
		}
	}
	return rBuf;
	*/
}

// AHTOXA {
static uint FASTCALL GetTextHeight(char * pBuf, size_t strLen)
{
	uint   text_height = 1;
	size_t buf_len = sstrlen(pBuf);
	if(buf_len > strLen) {
		char * p_buf = pBuf;
		uint   prev_pos = 0;
		size_t pos = 0;
		size_t len = 0;
		size_t real_pos = 0;
		SSrchParam p;
		p.P_Pattern = (char*)onecstr(' ');
		p.P_WordChars = 0;
		p.Flags = 0;
		while(p_buf && searchstr(p_buf, p, &pos, &len) > 0) {
			if(pos > strLen) {
				size_t cut_pos = (prev_pos) ? prev_pos : pos;
				text_height++;
				prev_pos = pos = 0;
				real_pos += cut_pos;
				replacestr(pBuf, "\n", &real_pos, &len, 0);
				real_pos++;
				if(strlen(p_buf) > cut_pos + 1)
					p_buf += cut_pos + 1;
				else
					p_buf = 0;
			}
			else {
				prev_pos = pos;
				pos++;
			}
		}
		if(p_buf && strlen(p_buf) > strLen && prev_pos) {
			text_height++;
			real_pos += prev_pos;
			replacestr(pBuf, "\n", &real_pos, &len, 0);
		}
	}
	return text_height;
}
// } AHTOXA

static void FASTCALL RemoveCtrlNFromStr(char * pBuf, size_t bufLen)
{
	if(pBuf && bufLen) {
		size_t s = strlen(pBuf);
		if(s >= bufLen) {
			s = (bufLen-1);
			pBuf[bufLen-1] = 0;
		}
		if(memchr(pBuf, '\n', s) || memchr(pBuf, '\r', s)) {
			char   c;
			for(size_t i = 0; (c = pBuf[i]) != 0; i++) {
				if((c == '\r' && pBuf[i+1] == '\n') || (c == '\n' && pBuf[i+1] == '\r')) {
					size_t len = strlen(pBuf+i+1)+1;
					memcpy(pBuf+i, pBuf+i+1, len);
					pBuf[i] = ' ';
				}
				else if(c == '\n' || c == '\t')
					pBuf[i] = ' ';
			}
		}
	}
}

char * SLAPI BrowserDef::getMultiLinesText(long row, int column, char * pBuf, uint heightMult, uint * pCalcHeight)
{
	char   ptr[1024]; // @v9.0.5 [256]-->[1024]
	char   buf[1024];
	buf[0] = 0;
	char * p_src_data = (char *)getRow(row);
	if(p_src_data && column >= 0 && column < (int)getCount()) {
		const  BroColumn & c = at(column);
		int    r = 1;
		DpB.ExtraPtr  = ExtraPtr;
		DpB.ColumnN   = c.Offs;
		DpB.TypeID    = c.T;
		DpB.P_SrcData = p_src_data;
		DpB.Color     = 0;
		DpB.P_DestData = ptr;
		if(c.Options & BCO_USERPROC) {
			if(UserProc)
				r = UserProc(&DpB);
			else if(c.UserProc)
				r = c.UserProc(&DpB);
			else
				r = 0;
		}
		else
			DpB.P_DestData = p_src_data + c.Offs;
		if(r) {
			sttostr(c.T, DpB.P_DestData, c.format, buf);
			if((heightMult > 1 || pCalcHeight) && c.Options & BCO_RESIZEABLE) {
				uint   text_height = 0;
				strip(buf);
				RemoveCtrlNFromStr(buf, sizeof(buf));
				text_height = GetTextHeight(buf, SFMTLEN(c.format));
				StringSet ss('\n', buf);
				buf[0] = 0;
				for(uint i = 0, j = 0; ss.get(&i, ptr, sizeof(ptr)); j++) {
					strcat(buf, strip(ptr));
					if(j < ss.getCount() - 1)
						if(j < (heightMult - 1) || pCalcHeight)
							strcat(buf, "\n");
						else
							strcat(buf, " ");
				}
				ASSIGN_PTR(pCalcHeight, text_height);
			}
			else {
				RemoveCtrlNFromStr(buf, sizeof(buf));
				ASSIGN_PTR(pCalcHeight, 1);
			}
		}
	}
	return strnzcpy(pBuf, buf, 0);
}

int SLAPI BrowserDef::setText(long row, int column, const char * b)
{
	int    ok = 1;
	if(column >= 0 && column < (int)getCount()) {
		const BroColumn & c = at(column);
		if(c.Options & BCO_USERPROC)
			ok = 0;
		else {
			char * ptr = (char *)getRow(row);
			ok = ptr ? stfromstr(c.T, ptr + c.Offs, c.format, b) : 0;
		}
	}
	else
		ok = 0;
	return ok;
}

int FASTCALL BrowserDef::go(long row)
{
	int    reply = 1;
	if(valid()) {
		long   rc = getRecsCount();
		if(rc) {
			if(row < 0) {
				row = 0;
				reply = -1;
			}
			else if(row >= rc) {
				row = rc - 1;
				reply = -1;
			}
		}
		else {
			row = 0;
			reply = -1;
		}
		scrollDelta = curItem - row;
		curItem = row;
		if(row < topItem)
			topItem = curItem;
		else if(row >= topItem + viewHight)
			topItem = (curItem - viewHight + 1);
		if(topItem < 0)
			topItem = 0;
		if(curItem < 0)
			curItem = 0;
		return reply;
	}
	return 0;
}

int SLAPI BrowserDef::search2(const void * data, CompFunc cmpFunc, int srchMode, size_t offs)
{
	int    found = 0;
	if(data) {
		int    r = (srchMode == srchFirst) ? top() : step(1);
		if(r)
			do {
				const char * p = (char*)getRow(_curItem());
				if(p && cmpFunc(p + offs, data, 0) == 0)
					found = 1;
			} while(!found && step(1) > 0);
	}
	return found;
}

int   SLAPI BrowserDef::valid()
{
	return 1;
}

int   FASTCALL BrowserDef::step(long delta)
{
	return go(curItem + delta);
}

int   SLAPI BrowserDef::top()
{
	return go(0);
}

int   SLAPI BrowserDef::bottom()
{
	return go(getRecsCount() - 1);
}

long  SLAPI BrowserDef::getRecsCount()
{
	return 0;
}

void * FASTCALL BrowserDef::getRow(long)
{
	return 0;
}

int   SLAPI BrowserDef::refresh()
{
	return 1;
}

int   FASTCALL BrowserDef::setData(void *)
{
	return 1;
}

int   FASTCALL BrowserDef::getData(void *)
{
	return 1;
}

int SLAPI BrowserDef::search(void * pattern, CompFunc /*cfn*/, int srchMode, int srchCol)
{
	char   str[256];
	const  char * p = (const char *)pattern;
	int    r = (srchMode == srchFirst) ? top() : step(1);
	if(r)
		do {
			getText(_curItem(), srchCol, str);
			strip(str);
			if(p[0] != '*') {
			   	str[strlen(p)] = 0;
			   	r = stricmp866(str, p) ? 0 : 1;
			}
			else
				r = stristr866(str, p + 1) ? 1 : 0;
		} while(!r && step(1) > 0);
	return r;
}
//
//
//
SLAPI AryBrowserDef::AryBrowserDef(SArray * pAry, const BNFieldList * pFl, int captionHight, uint aOptions, void * extraPtr /*=0*/) :
	BrowserDef(captionHight, aOptions, extraPtr)
{
	P_Array = 0;
	P_Fields = 0;
	setArray(pAry, pFl, 1);
}

SLAPI AryBrowserDef::~AryBrowserDef()
{
	delete P_Fields;
	if(P_Array && (options & BRO_OWNER))
		delete P_Array;
}

int SLAPI AryBrowserDef::setArray(SArray * pAry, const BNFieldList * pFl, int setupPosition /*= 1*/)
{
	int    ok = 1;
	ZDELETE(P_Fields);
	if(P_Array && (options & BRO_OWNER)) {
		ZDELETE(P_Array);
	}
	if(pFl) {
		P_Fields = new BNFieldList(*pFl);
	}
	if(pAry) {
		P_Array = pAry;
		ok = setupPosition ? top() : 1;
	}
	return ok;
}

const SArray * SLAPI AryBrowserDef::getArray() const
{
	return P_Array;
}

int SLAPI AryBrowserDef::valid()
{
	return (P_Array != 0);
}

//virtual
int SLAPI AryBrowserDef::insertColumn(int atPos, const char * pTxt, uint fldNo, TYPEID typ, long fmt, uint opt)
{
	BroColumn c;
	c.OrgOffs = fldNo;
	c.T = typ;
	c.Offs = fldNo;
	c.format = fmt;
	c.Options = (opt | BCO_CAPLEFT);
	c.text = newStr(pTxt);
	if(addColumn(&c, (atPos >= 0) ? atPos : UNDEF)) {
		if(P_Groups)
			for(uint i = 0; i < NumGroups; i++) {
				BroGroup & grp = P_Groups[i];
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

long SLAPI AryBrowserDef::getRecsCount()
{
	return (long)(P_Array ? P_Array->getCount() : 0);
}

void * FASTCALL AryBrowserDef::getRow(long row)
{
	return (P_Array && row >= 0 && row < (long)P_Array->getCount()) ? P_Array->at((uint)row) : 0;
}

int FASTCALL AryBrowserDef::getData(void * pData)
{
	*(SArray **)pData = P_Array;
	return 1;
}

int FASTCALL AryBrowserDef::setData(void * pNewData)
{
	return setArray((SArray *)pNewData, 0, 1);
}
