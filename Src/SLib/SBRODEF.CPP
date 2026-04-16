// SBRODEF.CPP
// Copyright (c) Sobolev A. 1995, 1996, 1997, 2003, 2004, 2005, 2007, 2008, 2009, 2010, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022, 2023, 2024, 2025
// @codepage UTF-8
//
#include <slib-internal.h>
#pragma hdrstop
//
//
//
SBrowserDataProcBlock::SBrowserDataProcBlock() : ExtraPtr(0), ColumnN(0), TypeID(0), P_SrcData(0), DestBufSize(0), Color(0), P_DestData(0)
{
}

SBrowserDataProcBlock & SBrowserDataProcBlock::Z()
{
	ExtraPtr = 0;
	ColumnN = 0;
	TypeID = 0;
	P_SrcData = 0;
	DestBufSize = 0;
	Color = 0;
	P_DestData = 0;
	TempBuf.Z();
	return *this;
}

SBrowserDataProcBlock & SBrowserDataProcBlock::Setup(const BroColumn & rC, const void * pSrcData, void * pDestBuf, size_t destBufSize, void * extraPtr)
{
	Z();
	ExtraPtr = extraPtr;
	ColumnN   = rC.Offs;
	TypeID    = rC.T;
	P_SrcData = pSrcData;
	P_DestData = pDestBuf;
	DestBufSize = destBufSize; // @v12.3.6
	return *this;
}

void   SBrowserDataProcBlock::SetZero() { PTR8(P_DestData)[0] = 0; }
void   FASTCALL SBrowserDataProcBlock::Set(int32 i) { *static_cast<int32 *>(P_DestData) = i; }
void   SBrowserDataProcBlock::Set(int64 i) { *static_cast<int64 *>(P_DestData) = i; } // @v12.3.9
void   SBrowserDataProcBlock::Set(double r) { *static_cast<double *>(P_DestData) = r; }
void   FASTCALL SBrowserDataProcBlock::Set(const char * pS) { strnzcpy(static_cast<char *>(P_DestData), pS, stsize(TypeID)); }
void   FASTCALL SBrowserDataProcBlock::Set(const SString & rS) { rS.CopyTo(static_cast<char *>(P_DestData), stsize(TypeID)); }
void   FASTCALL SBrowserDataProcBlock::Set(LDATE dt) { *static_cast<LDATE *>(P_DestData) = dt; }
void   FASTCALL SBrowserDataProcBlock::Set(LTIME tm) { *static_cast<LTIME *>(P_DestData) = tm; }
void   FASTCALL SBrowserDataProcBlock::Set(LDATETIME dtm) { *static_cast<LDATETIME *>(P_DestData) = dtm; } // @v11.1.7
//
//
//
BroGroup::BroGroup()
{
	THISZERO();
}

uint BroGroup::NextColumn() const { return (First+Count); }
//
//
//
BrowserDef::BrowserDef(uint signature, int captionHight, uint aOptions, void * extraPtr /*=0*/) : Signature(signature), 
	SArray(sizeof(BroColumn), aryDataOwner|aryEachItem), capHight(captionHight), options(aOptions), NumGroups(0), 
	P_Groups(0), viewHight(1), topItem(0), curItem(0), scrollDelta(0), P_CtList(0),
	ExtraPtr(extraPtr), UserProc(0), isBOQ(false), isEOQ(false)
{
}

BrowserDef::~BrowserDef()
{
	freeAll();
	if(P_Groups) {
		for(uint i = 0; i < NumGroups; i++)
			delete P_Groups[i].P_Text;
		delete P_Groups;
	}
	if(P_CtList) {
		for(uint i = 0; i < P_CtList->getCount(); i++)
			delete static_cast<BroCrosstab *>(P_CtList->at(i))->P_Text;
		delete P_CtList;
	}
}

int BrowserDef::addColumn(const BroColumn * pCol, int pos)
{
	int    ok = 1;
	if(pos == UNDEF)
		pos = count;
	BroColumn col = *pCol; // @attention Передача указателя BroColumn::text //
	SString subst;
	if(SLS.SubstString(col.P_Text, 0, subst) > 0) {
		delete col.P_Text;
		col.P_Text = newStr(subst);
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
	col.CWidth = SFMTLEN(f);
	col.CWidthOrg = col.CWidth; // @v12.4.9
	col.format = f;
	if(atInsert(pos, &col))
		initOffset(pos);
	else
		ok = 0;
	return ok;
}

bool STDCALL BrowserDef::IsColInGroup(uint col, uint * pIdx) const
{
	bool   result = false;
	uint   idx = 0;
	for(uint i = 0, c = NumGroups; !result && i < c; i++) {
		const BroGroup * p_group = GetGroup(i);
		if(col >= p_group->First && col < p_group->NextColumn()) {
			idx = i;
			result = true;
		}
	}
	ASSIGN_PTR(pIdx, idx);
	return result;
}

BroColumn & FASTCALL BrowserDef::at(uint pos) const { return *static_cast<BroColumn *>(SArray::at(pos)); }
/*virtual*/int BrowserDef::insertColumn(int, const char *, uint, TYPEID, long, uint) { return 0; }
/*virtual*/int BrowserDef::insertColumn(int atPos, const char * pTxt, const char * pFldName, TYPEID typ, long fmt, uint opt) { return 0; }
int BrowserDef::removeColumn(int pos) { return atFree(pos); }

int BrowserDef::setColumnTitle(int colN, const char * pText)
{
	if(colN >= 0 && colN < static_cast<int>(getCount())) {
		BroColumn & r_column = at(colN);
		ZDELETE(r_column.P_Text);
		r_column.P_Text = newStr(pText);
		return 1;
	}
	else
		return 0;
}

void BrowserDef::initOffset(int pos)
{
	BroColumn & r_column = at(pos);
	if(r_column.Offs == UNDEF)
		if(pos > 0) {
			const BroColumn & r_prev = at(pos-1);
			r_column.Offs = r_prev.Offs + stsize(r_prev.T);
		}
		else
			r_column.Offs = 0;
}

int BrowserDef::AddColumnGroup(BroGroup * pGroup)
{
	int    ok = 1;
	if(pGroup) {
		SArray array(P_Groups, sizeof(BroGroup), NumGroups);
		SString subst;
		if(SLS.SubstString(pGroup->P_Text, 0, subst) > 0) {
			delete pGroup->P_Text;
			pGroup->P_Text = newStr(subst);
		}
		ok = array.ordInsert(pGroup, 0, PTR_CMPFUNC(int));
		P_Groups = static_cast<BroGroup *>(array.dataPtr());
		NumGroups = array.getCount();
	}
	return ok;
}

int    BrowserDef::AddCrosstab(BroCrosstab * pCt) { return SETIFZ(P_CtList, new SArray(sizeof(BroCrosstab))) ? P_CtList->insert(pCt) : 0; }
uint   BrowserDef::GetCrosstabCount() const { return SVectorBase::GetCount(P_CtList); }
const  BroCrosstab * BrowserDef::GetCrosstab(uint ctN) const { return (P_CtList && ctN < P_CtList->getCount()) ? static_cast<const BroCrosstab *>(P_CtList->at(ctN)) : 0; }
int    BrowserDef::GetCapHeight() const { return capHight; }
void   BrowserDef::VerifyCapHeight() { capHight = MIN(2, capHight); }
uint   BrowserDef::GetGroupCount() const { return NumGroups; }
const  BroGroup * FASTCALL BrowserDef::GetGroup(uint grpN) const { return (grpN < NumGroups) ? (P_Groups+grpN) : 0; }
uint * BrowserDef::GetGroupIndexPtr(uint grpN) { return (grpN < NumGroups) ? &P_Groups[grpN].Index : 0; }
bool   BrowserDef::IsBOQ() const { return isBOQ; }
bool   BrowserDef::IsEOQ() const { return isEOQ; }
bool   BrowserDef::CheckFlag(uint f) const { return LOGIC(options & f); }
// @v12.4.8 (unused) uint   BrowserDef::groupWidth(uint grpN, uint atColumn) const { return groupWidth(P_Groups + grpN, atColumn); }
void   BrowserDef::setViewHight(int newHight) { viewHight = newHight; }

void BrowserDef::FreeAllCrosstab()
{
	if(P_CtList) {
		for(uint i = 0; i < P_CtList->getCount(); i++)
			delete static_cast<BroCrosstab *>(P_CtList->at(i))->P_Text;
		P_CtList->freeAll();
	}
}

void BrowserDef::getScrollData(long * pScrollDelta, long * pScrollPos)
{
	ASSIGN_PTR(pScrollDelta, scrollDelta);
	ASSIGN_PTR(pScrollPos, curItem);
}

int BrowserDef::FindColumnByOrgOffs(uint orgOffs) const // @v12.4.2
{
	int    result = -1;
	for(uint i = 0; result < 0 && i < getCount(); i++) {
		const BroColumn & r_col = at(i);
		if(r_col.OrgOffs == orgOffs)
			result = static_cast<int>(i);
	}
	return result;
}

bool BrowserDef::SetCapHeight(int height) // @v12.2.2
{
	bool   ok = true;
	if(height > 0)
		capHight = height;
	else
		ok = false;
	return ok;
}

void BrowserDef::ClearGroupIndexies()
{
	for(uint i = 0; i < NumGroups; i++)
		P_Groups[i].Index = 0;
}

void BrowserDef::SetUserProc(SBrowserDataProc proc, void * extraPtr)
{
	UserProc = proc;
	ExtraPtr = extraPtr;
}

/*virtual*/void FASTCALL BrowserDef::freeItem(void * pItem)
{
	if(pItem)
		delete static_cast<BroColumn *>(pItem)->P_Text;
}

const BroGroup * BrowserDef::groupOf(uint column, uint * pGrpPos) const
{
	for(uint i = 0; i < NumGroups; i++) {
		BroGroup * p_gr = P_Groups + i;
		if(column >= p_gr->First && column < p_gr->NextColumn()) {
			ASSIGN_PTR(pGrpPos, i);
			return p_gr;
		}
	}
	ASSIGN_PTR(pGrpPos, 0);
	return 0;
}

/* @v12.4.8 (unused) uint BrowserDef::groupWidth(const BroGroup * pGroup, uint atColumn) const
{
	int    _n = 0;
	int    wd = 0;
	const  uint c = pGroup->NextColumn();
	for(uint i = atColumn; i < c; i++) {
		wd += at(i).CWidth;
		_n++;
	}
	wd += (_n - 1) * 3;
	return wd;
}*/

const void * BrowserDef::Helper_GetCellData(const void * pRowData, int columnIdx, TYPEID * pTypeID, long * pFmt, uint * pCOptions, void * pOuterBuf, size_t outerBufSize)
{
	const void * p_result = 0;
	const char * p_src_data = static_cast<const char *>(pRowData);
	TYPEID typ = 0;
	long   fmt = 0;
	uint   coptions = 0;
	if(p_src_data && columnIdx >= 0 && columnIdx < getCountI()) {
		const  BroColumn & r_column = at(columnIdx);
		if(r_column.Options & BCO_USERPROC) {
			if(pOuterBuf) {
				int    r = 0;
				DpB.Setup(r_column, p_src_data, pOuterBuf, outerBufSize, ExtraPtr);
				if(r_column._ColumnUserProc)
					r = r_column._ColumnUserProc(&DpB);
				else if(UserProc)
					r = UserProc(&DpB);
				else
					r = 0;
				if(r)
					p_result = pOuterBuf;
			}
		}
		else
			p_result = p_src_data + r_column.Offs;
		typ = r_column.T;
		fmt = r_column.format;
		coptions = r_column.Options;
	}
	ASSIGN_PTR(pTypeID, typ);
	ASSIGN_PTR(pFmt, fmt);
	ASSIGN_PTR(pCOptions, coptions);
	return p_result;
}

int BrowserDef::GetCellData(const void * pRowData, int column, TYPEID * pType, void * pDataBuf, size_t dataBufLen)
{
	int    ok = 0;
	TYPEID typ = 0;
	char   dest_data[1024];
	const  void * p_final_data = Helper_GetCellData(pRowData, column, &typ, 0/*format*/, 0/*coptions*/, dest_data, sizeof(dest_data));
	if(p_final_data) {
		// @v12.3.11 @fix (это, похоже, лишнее) TYPEID typ = DpB.TypeID;
		memcpy(pDataBuf, p_final_data, smin(dataBufLen, static_cast<size_t>(stsize(typ))));
		ASSIGN_PTR(pType, typ);
		ok = 1;
	}
	return ok;
}

int BrowserDef::GetCellData(long row, int column, TYPEID * pType, void * pDataBuf, size_t dataBufLen)
{
	return GetCellData(getRow(row), column, pType, pDataBuf, dataBufLen);
}

SString & BrowserDef::GetCellText(long row, int column, bool dontRestrictByFmtLen, SString & rBuf)
{
	rBuf.Z();
	TYPEID typ = 0;
	long   fmt = 0;
	char   dest_data[4096]; // @v12.6.0 [1024]-->[4096]
	const  void * p_final_data = Helper_GetCellData(getRow(row), column, &typ, &fmt, 0/*coptions*/, dest_data, sizeof(dest_data));
	if(p_final_data) {
		// @v12.4.12 {
		if(dontRestrictByFmtLen) {
			if(GETSTYPE(typ) == S_ZSTRING) {
				const  uint typ_sz = GETSSIZE(typ);
				if(typ_sz) {
					uint fmt_len = SFMTLEN(fmt);
					if(fmt_len > 0 && fmt_len < (typ_sz-1)) {
						fmt_len = (typ_sz-1);
						SETSFMTLEN(fmt, fmt_len);
					}
				}
			}
		}
		// } @v12.4.12 
		sttosstr(typ, p_final_data, fmt, rBuf);
	}
	return rBuf;
}

SString & BrowserDef::getFullText(const void * pRowData, int column, SString & rBuf)
{
	rBuf.Z();
	TYPEID typ = 0;
	long   fmt = 0;
	char   dest_data[1024];
	const  void * p_final_data = Helper_GetCellData(pRowData, column, &typ, &fmt, 0/*coptions*/, dest_data, sizeof(dest_data));
	if(p_final_data) {
		/* @v12.5.10
		char   dest_text[1024];
		dest_text[0] = 0;
		SETSFMTLEN(fmt, 0);
		sttostr(typ, p_final_data, fmt, dest_text);
		rBuf = dest_text;		
		*/
		// @v12.5.10 {
		SETSFMTLEN(fmt, 0);
		sttosstr(typ, p_final_data, fmt, rBuf);
		// } @v12.5.10 
	}
	return rBuf;
}

SString & BrowserDef::getFullText(long row, int column, SString & rBuf)
{
	return getFullText(getRow(row), column, rBuf);
}

static uint FASTCALL GetTextHeight_(SString & rBuf, size_t strLen) // AHTOXA
{
	uint   text_height = 1;
	if(rBuf.Len() > strLen) {
		SString head;
		SString tail;
		if(rBuf.Wrap(strLen, head, tail) > 0) {
			SString result;
			do {
				text_height++;
				result.Cat(head).CR();
				rBuf = tail; // !rBuf дальше работает как временная переменная!
			} while(rBuf.Wrap(strLen, head, tail) > 0);
			if(rBuf.NotEmpty()) {
				result.Cat(rBuf);
			}
			rBuf = result;
		}
	}
	return text_height;
}

static uint FASTCALL GetTextHeight(char * pBuf, size_t strLen) // AHTOXA
{
	uint   text_height = 1;
	const  size_t buf_len = sstrlen(pBuf);
	if(buf_len > strLen) {
		char * p_buf = pBuf;
		uint   prev_pos = 0;
		size_t pos = 0;
		size_t len = 0;
		size_t real_pos = 0;
		SSrchParam p(" ", 0, 0);
		while(p_buf && searchstr(p_buf, p, &pos, &len) > 0) {
			if(pos > strLen) {
				const size_t cut_pos = (prev_pos) ? prev_pos : pos;
				text_height++;
				prev_pos = pos = 0;
				real_pos += cut_pos;
				replacestr(pBuf, "\n", &real_pos, &len, 0);
				real_pos++;
				if(sstrlen(p_buf) > (cut_pos + 1))
					p_buf += cut_pos + 1;
				else
					p_buf = 0;
			}
			else {
				prev_pos = pos;
				pos++;
			}
		}
		if(sstrlen(p_buf) > strLen && prev_pos) {
			text_height++;
			real_pos += prev_pos;
			replacestr(pBuf, "\n", &real_pos, &len, 0);
		}
	}
	return text_height;
}

static void FASTCALL RemoveCtrlNFromStr(char * pBuf, size_t bufLen)
{
	size_t s = sstrlen(pBuf);
	if(s && bufLen) {
		if(s >= bufLen) {
			s = (bufLen-1);
			pBuf[bufLen-1] = 0;
		}
		if(smemchr(pBuf, '\n', s) || smemchr(pBuf, '\r', s)) {
			char   c;
			for(size_t i = 0; (c = pBuf[i]) != 0; i++) {
				if((c == '\r' && pBuf[i+1] == '\n') || (c == '\n' && pBuf[i+1] == '\r')) {
					const size_t len = sstrlen(pBuf+i+1)+1;
					memcpy(pBuf+i, pBuf+i+1, len);
					pBuf[i] = ' ';
				}
				else if(oneof2(c, '\n', '\t'))
					pBuf[i] = ' ';
			}
		}
	}
}

static void FASTCALL RemoveCtrlNFromStr_(SString & rBuf)
{
	rBuf.ReplaceCR();
	rBuf.ReplaceChar('\t', ' ');
}

SString & BrowserDef::getMultiLinesText(long row, int column, /*char * pBuf*/SString & rBuf, uint heightMult, uint * pCalcHeight)
{
	rBuf.Z();
	//char   buf[4096];
	//buf[0] = 0;
	//SString temp_buf;
	TYPEID typ = 0;
	long   fmt = 0;
	uint   coptions = 0;
	char   dest_data[4096]; // @v12.6.0 [1024]-->[4096]
	const  void * p_final_data = Helper_GetCellData(getRow(row), column, &typ, &fmt, &coptions, dest_data, sizeof(dest_data));
	if(p_final_data) {
		// @v12.5.10 sttostr(typ, p_final_data, fmt, buf);
		sttosstr(typ, p_final_data, fmt, rBuf); // @v12.5.10 
		if((heightMult > 1 || pCalcHeight) && coptions & BCO_RESIZEABLE) {
			SString iter_buf;
			uint   text_height = 0;
			//strip(buf);
			rBuf.Strip();
			RemoveCtrlNFromStr_(rBuf);
			text_height = GetTextHeight_(rBuf, SFMTLEN(fmt));
			StringSet ss('\n', rBuf);
			rBuf.Z();
			for(uint i = 0, j = 0; ss.get(&i, iter_buf/*dest_data, sizeof(dest_data)*/); j++) {
				//strcat(buf, strip(dest_data));
				rBuf.Cat(iter_buf);
				if(j < ss.getCount()-1) {
					if(j < (heightMult - 1) || pCalcHeight) {
						//strcat(buf, "\n");
						rBuf.CR();
					}
					else {
						//strcat(buf, " ");
						rBuf.Space();
					}
				}
			}
			ASSIGN_PTR(pCalcHeight, text_height);
		}
		else {
			RemoveCtrlNFromStr_(rBuf);
			ASSIGN_PTR(pCalcHeight, 1);
		}
	}
	//return strnzcpy(pBuf, buf, 0);
	return rBuf;
}

int BrowserDef::go(long row)
{
	int    result = 1;
	if(IsValid()) {
		const long rc = GetRecsCount();
		if(rc) {
			if(row < 0) {
				row = 0;
				result = -1;
			}
			else if(row >= rc) {
				row = rc - 1;
				result = -1;
			}
		}
		else {
			row = 0;
			result = -1;
		}
		scrollDelta = curItem - row;
		curItem = row;
		if(row < topItem)
			topItem = curItem;
		else if(row >= topItem + viewHight)
			topItem = (curItem - viewHight + 1);
		SETMAX(topItem, 0);
		SETMAX(curItem, 0);
	}
	else
		result = 0;
	return result;
}

int BrowserDef::search2(const void * data, CompFunc cmpFunc, int srchMode, size_t offs, void * pExtraData)
{
	int    found = 0;
	if(data) {
		int    r = (srchMode == srchFirst) ? top() : step(1);
		if(r) {
			do {
				const char * p = static_cast<const char *>(getRow(_curItem()));
				if(p && cmpFunc(p + offs, data, pExtraData) == 0) {
					found = 1;
				}
			} while(!found && step(1) > 0);
		}
	}
	return found;
}

bool   BrowserDef::IsValid() const { return true; }
int    BrowserDef::step(long delta) { return go(curItem + delta); }
int    BrowserDef::top() { return go(0); }
int    BrowserDef::bottom() { return go(GetRecsCount() - 1); }
long   BrowserDef::GetRecsCount() const { return 0; }
const  void * BrowserDef::getRow(long) const { return 0; }
int    BrowserDef::refresh() { return 1; }

int BrowserDef::search(const void * pPattern, CompFunc/*cfn*/, int srchMode, int srchCol)
{
	//char   str[2048]; // @v11.7.11 [256]-->[2048]
	int   r = 0;
	const  char * p = static_cast<const char *>(pPattern);
	const  size_t pattern_len = sstrlen(p);
	if(pattern_len) {
		SString text;
		r = (srchMode == srchFirst) ? top() : step(1);
		if(r) do {
			GetCellText(_curItem(), srchCol, true, text);
			text.Strip();
			if(p[0] != '*') {
				//str[sstrlen(p)] = 0;
				//r = stricmp866(str, p) ? 0 : 1;
				text.Trim(pattern_len);
				r = text.IsEqNC(p);
			}
			else {
				//bool   debug_mark = false; // @debug
				//const  int r2 = BIN(stristr866(text.cptr(), p+1)); // @debug
				r = text.Search(p+1, 0, 1, 0) ? 1 : 0;
				//else if(r2) {
					//debug_mark = true;
				//}
			}
		} while(!r && step(1) > 0);
	}
	return r;
}
//
//
//
AryBrowserDef::AryBrowserDef(SArray * pAry, const BNFieldList2 * pFl, int captionHight, uint aOptions, void * extraPtr /*=0*/) :
	BrowserDef(BrowserDefSignature_ARY, captionHight, aOptions, extraPtr), P_Array(0), P_Fields(0)
{
	setArray(pAry, pFl, 1);
}

AryBrowserDef::~AryBrowserDef()
{
	delete P_Fields;
	if(P_Array && (options & BRO_OWNER))
		delete P_Array;
}

int AryBrowserDef::setArray(SArray * pAry, const BNFieldList2 * pFl, int setupPosition /*= 1*/)
{
	int    ok = 1;
	ZDELETE(P_Fields);
	if(P_Array && (options & BRO_OWNER)) {
		ZDELETE(P_Array);
	}
	if(pFl) {
		P_Fields = new BNFieldList2(*pFl);
	}
	if(pAry) {
		P_Array = pAry;
		ok = setupPosition ? top() : 1;
	}
	return ok;
}

const SArray * AryBrowserDef::getArray() const { return P_Array; }
bool  AryBrowserDef::IsValid() const { return (P_Array != 0); }

/*virtual*/int AryBrowserDef::insertColumn(int atPos, const char * pTxt, uint fldNo, TYPEID typ, long fmt, uint opt)
{
	int    ok = 1;
	BroColumn c;
	c.OrgOffs = fldNo;
	c.T = typ;
	c.Offs = fldNo;
	c.format = fmt;
	c.Options = (opt | BCO_CAPLEFT);
	c.P_Text = newStr(pTxt);
	if(addColumn(&c, (atPos >= 0) ? atPos : UNDEF)) {
		if(P_Groups) {
			for(uint i = 0; i < NumGroups; i++) {
				BroGroup & grp = P_Groups[i];
				if(atPos <= static_cast<int>(grp.First))
					grp.First++;
				else if(atPos > static_cast<int>(grp.First) && atPos <= static_cast<int>(grp.NextColumn()))
					grp.Count++;
			}
		}
	}
	else
		ok = 0;
	return ok;
}

long AryBrowserDef::GetRecsCount() const { return static_cast<long>(SVectorBase::GetCount(P_Array)); }

const void * AryBrowserDef::getRow(long row) const
{
	return (P_Array && row >= 0 && row < P_Array->getCountI()) ? P_Array->at((uint)row) : 0;
}
