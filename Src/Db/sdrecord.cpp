// SDRECORD.CPP
// Copyright (c) A.Sobolev 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2014, 2015, 2016, 2017, 2018, 2019, 2020
//
#include <slib.h>
#include <tv.h>
#pragma hdrstop
#include <db.h>
#include <dbf.h>
//
// @ModuleDef(SdRecord)
//
SdbField::SdbField() : ID(0), OuterFormat(0), InnerOffs(0)
{
	T.Init();
}

void SdbField::Init()
{
	ID = 0;
	T.Init();
	OuterFormat = 0;
	InnerOffs = 0;
	Name.Z();
	Descr.Z();
	Formula.Z();
}

int FASTCALL SdbField::IsEqual(const SdbField & rPat) const
{
	int    ok = 1;
	THROW(ID == rPat.ID);
	THROW(OuterFormat == rPat.OuterFormat);
	THROW(InnerOffs == rPat.InnerOffs);
	THROW(memcmp(&T, &rPat.T, sizeof(T)) == 0);
	THROW(Name == rPat.Name);
	THROW(Descr == rPat.Descr);
	THROW(Formula == rPat.Formula);
	CATCHZOK
	return ok;
}

int SdbField::GetFieldDataFromBuf(SString & rTextData, const void * pRecBuf, const SFormatParam & rFmt) const
{
	size_t ns = T.GetBinSize();
	const  TYPEID st = T.GetDbFieldType();
	int    base_type = stbase(st);
	const  void * p_fld_data = PTR8C(pRecBuf)+InnerOffs;
	long   fmt = 0;
	if(OuterFormat) {
		fmt = OuterFormat;
		if(base_type == BTS_DATE && SFMTFLAG(fmt) == 0) {
			SETSFMTFLAG(fmt, DATF_GERMAN|DATF_CENTURY);
		}
		if(rFmt.Flags & SFormatParam::fFloatSize) {
			SETSFMTLEN(fmt, 0);
			SETSFMTFLAG(fmt, SFMTFLAG(fmt) | ALIGN_LEFT);
		}
	}
	else if(base_type == BTS_DATE)
		fmt = rFmt.FDate;
	else if(base_type == BTS_TIME)
		fmt = rFmt.FTime;
	else if(base_type == BTS_REAL)
		fmt = rFmt.FReal;
	char   temp_buf[8192];
	temp_buf[0] = 0;
	sttostr(st, p_fld_data, fmt, temp_buf);
	rTextData = temp_buf;
	if(base_type == BTS_STRING && rFmt.Flags & SFormatParam::fQuotText)
		rTextData.Quot('\"', '\"');
	return 1;
}

void SdbField::PutFieldDataToBuf(const SString & rTextData, void * pRecBuf, const SFormatParam & rFmt) const
{
	size_t ns = T.GetBinSize();
	const  TYPEID st = T.GetDbFieldType();
	const  int    base_type = stbase(st);
	const SString * p_text_data = &rTextData;
	SString stub_text_data;
	void * p_fld_data = PTR8(pRecBuf)+InnerOffs;
	long   fmt = 0;
	if(base_type == BTS_STRING) {
		if(rFmt.Flags & SFormatParam::fQuotText) {
			stub_text_data = rTextData;
			stub_text_data.TrimRightChr('\"').ShiftLeftChr('\"');
			p_text_data = &stub_text_data;
		}
	}
	else if(base_type == BTS_DATE)
		fmt = rFmt.FDate;
	else if(base_type == BTS_TIME)
		fmt = rFmt.FTime;
	stfromstr(st, p_fld_data, fmt, *p_text_data);
}
//
//Code = NDOC,     C,  25, 0
//Date = DATEDOC,  D,   8, 0
//
// type symb [size]
// symb type [size]
//
#define TOK_TYPE   1
#define TOK_SYMB   2
#define TOK_SIZE   3
#define TOK_SLASH  4

struct ReList {
	ReList()
	{
		ReSize[0].Compile("\\[[ \t]*[0-9]+(\\.[0-9]*)?[ \t]*\\]"); // [len.prec]
		ReSize[1].Compile("\\([ \t]*[0-9]+(\\.[0-9]*)?[ \t]*\\)"); // (len.prec)
		ReSize[2].Compile("[0-9]+[ \t]*(,[ \t]*[0-9]+)?");          // len, prec
		ReSymb.Compile("[a-zA-Z_а-яёА-ЯЁ]+[0-9a-zA-Z_а-яёА-ЯЁ]*");     // @v6.2.x AHTOXA а-яА-Я
	}
	CRegExp ReSize[3];
	CRegExp ReSymb;
};

static int GetNextToken(SStrScan & rScan, ReList & rRl, long * pVal, SString & rBuf)
{
	rScan.Skip(SStrScan::wsSpace|SStrScan::wsTab|SStrScan::wsComma);
	if(oneof2(rScan[0], '/', '\\')) {
		rBuf = "\\";
		rScan.Incr();
		return TOK_SLASH;
	}
	else {
		TYPEID t = IsSTypeName(rScan);
		if(t) {
			{
				//
				// Пытаемся разрешить коллизию, возникающую в том случае, если
				// символ распознается как тип данных. В этом случае считаем, что
				// первым по порядку идет символ, а за ним тип данных.
				//
				long   lv;
				SString temp_symb;
				SStrScan temp_scan = rScan;
				temp_scan.IncrLen();
				if(GetNextToken(temp_scan, rRl, &lv, temp_symb) == TOK_TYPE) { // @recursion
					if(rRl.ReSymb.Find(&rScan)) {
						rScan.Get(rBuf);
						rScan.IncrLen();
						return TOK_SYMB;
					}
					//
					// Если символ не распознается, то "чему быть - того не миновать":
					// следуем штатным маршрутом.
					//
				}
			}
			rScan.IncrLen();
			ASSIGN_PTR(pVal, t);
			return TOK_TYPE;
		}
		else {
			//
			// Так как регулярное выражение может находиться далее по тексту,
			// то проверяем только то, что находится по смещению rScan.Offs.
			//
			SStrScan temp_scan = rScan;
			if(rRl.ReSymb.Find(&temp_scan) && temp_scan.Offs == rScan.Offs) {
				rScan = temp_scan;
				rScan.Get(rBuf);
				rScan.IncrLen();
				return TOK_SYMB;
			}
			else {
				for(uint i = 0; i < 3; i++) {
					temp_scan = rScan;
					if(rRl.ReSize[i].Find(&temp_scan) && temp_scan.Offs == rScan.Offs) {
						rScan = temp_scan;
						SString size_buf, len_buf, prc_buf;
						rScan.Get(size_buf);
						size_buf.ShiftLeftChr('[');
						size_buf.ShiftLeftChr('(');
						size_buf.TrimRightChr(']');
						size_buf.TrimRightChr(')');
						size_buf.Divide((i == 2) ? ',' : '.', len_buf, prc_buf);
						long f = MKSFMTD(len_buf.ToLong(), prc_buf.ToLong(), 0);
						ASSIGN_PTR(pVal, f);
						return TOK_SIZE;
					}
				}
			}
		}
		return 0;
	}
}

int SdbField::Helper_TranslateString(SStrScan & rScan, void * pData)
{
	EXCEPTVAR(SLibError);
	int    ok = 1;
	ReList * p_re_list = static_cast<ReList *>(pData);
	int    tok = 0, prev_tok = 0;
	long   tok_val = 0;
	SString tok_buf;
	T.Typ = 0;
	Name = 0;
	OuterFormat = 0;
	size_t org_offs = rScan.Offs;
	size_t prev_offs = rScan.Offs;
	while(ok && (tok = GetNextToken(rScan, *p_re_list, &tok_val, tok_buf)) != 0) {
		if(tok == TOK_TYPE) {
			if(T.Typ)
				ok = SLS.SetError(SLERR_SDREC_SYNTAX, rScan.GetBuf(prev_offs));
			else
				T.Typ = static_cast<TYPEID>(tok_val);
		}
		else if(tok == TOK_SYMB) {
			if(prev_tok == TOK_SLASH)
				Name.Cat(tok_buf);
			else if(Name.NotEmpty())
				ok = SLS.SetError(SLERR_SDREC_SYNTAX, rScan.GetBuf(prev_offs));
			else
				Name = tok_buf;
		}
		else if(tok == TOK_SLASH) {
			if(prev_tok != TOK_SYMB)
				ok = SLS.SetError(SLERR_SDREC_SYNTAX, rScan.GetBuf(prev_offs));
			else {
				Name.CatChar('\\');
			}
		}
		else if(tok == TOK_SIZE) {
			if(OuterFormat)
				ok = SLS.SetError(SLERR_SDREC_SYNTAX, rScan.GetBuf(prev_offs));
			else
				OuterFormat = tok_val;
		}
		prev_offs = rScan.Offs;
		prev_tok = tok;
		if(T.Typ && OuterFormat && Name.NotEmpty())
			break;
	}
	if(!T.Typ || Name.Empty())
		ok = SLS.SetError(SLERR_SDREC_SYNTAX, rScan.GetBuf(org_offs));
	if(!ok) {
		rScan.Offs = org_offs;
		rScan.Len = 0;
	}
	else {
		rScan.Len = rScan.Offs - org_offs;
	}
	return ok;
}

int SdbField::TranslateString(SStrScan & rScan)
{
	ReList rl;
	return Helper_TranslateString(rScan, &rl);
}
//
// style:
// 1 :: type name [ size ]
// 2 :: type name ( size )
// 3 :: name type [ size ]
// 4 :: name type ( size )
// 5 :: name, type, len, prec
// 6 :: type name [binary_size_of_array]
//
int SdbField::PutToString(int style, SString & rBuf) const
{
	SString temp_buf, type_buf, size_buf;
	GetBinaryTypeString(T.Typ, 0, type_buf, 0, 0);
	int    fmt_len = SFMTLEN(OuterFormat);
	int    fmt_prc = SFMTPRC(OuterFormat);
	if(oneof4(style, 1, 2, 3, 4)) {
		if(fmt_len || fmt_prc) {
			size_buf.Cat(fmt_len);
			if(fmt_prc)
				size_buf.Dot().Cat(fmt_prc);
			if(oneof2(style, 1, 3))
				size_buf.Quot('[', ']');
			else
				size_buf.Quot('(', ')');
		}
	}
	else if(style == 5)
		size_buf.Cat(fmt_len).CatDiv(',', 2).Cat(fmt_prc);
	else if(style == 6) {
		if(GETSTYPE(T.Typ) == S_ZSTRING && GETSSIZE(T.Typ) > 1) {
			size_buf.CatChar('[').Cat(GETSSIZE(T.Typ)).CatChar(']');
		}
	}
	if(oneof2(style, 1, 2))
		temp_buf.Cat(type_buf).Space().Cat(Name).Cat(size_buf);
	else if(oneof2(style, 3, 4))
		temp_buf.Cat(Name).Space().Cat(type_buf).Cat(size_buf);
	else if(style == 5)
		temp_buf.Cat(Name).CatDiv(',', 2).Cat(type_buf).CatDiv(',', 2).Cat(size_buf);
	else if(style == 6)
		temp_buf.Cat(type_buf.Align(6, ADJ_LEFT)).Space().Cat(Name).Cat(size_buf);
	rBuf = temp_buf;
	return 1;
}

int SdbField::ConvertToDbfField(DBFCreateFld * pDbfFld) const
{
	int    ok = 1;
	DBFCreateFld cf;
	size_t len = SFMTLEN(OuterFormat);
	size_t prc = SFMTPRC(OuterFormat);
	if(len > 255)
		len = 0;
	if(prc > 12)
		prc = 0;
	MEMSZERO(cf);
	Name.CopyTo(cf.Name, sizeof(cf.Name));
	switch(GETSTYPE(T.Typ)) {
		case S_UINT:
		case S_INT:
			cf.Type = 'N';
			cf.Size = (uint8)((len > 0) ? len : 10);
			cf.Prec = (uint8)prc;
			break;
		case S_FLOAT:
			cf.Type = 'N';
			cf.Size = (uint8)((len > 0) ? len : 16);
			cf.Prec = (uint8)((prc > 0) ? prc : 0);
			break;
		case S_BOOL:
			cf.Type = 'L'; // @v8.1.2 @fix 'B'-->'L'
			cf.Size = 1;
			break;
		case S_ZSTRING :
			cf.Type = 'C';
			cf.Size = (uint8)((len > 0) ? len : (GETSSIZE(T.Typ) ? GETSSIZE(T.Typ) : 128));
			break;
		case S_DATE:
			cf.Type = 'D';
			cf.Size = 8;
			break;
		case S_TIME:
			cf.Type = 'C';
			cf.Size = (uint8)((len > 0) ? len : 10);
			break;
		default:
			ok = 0; // not supported
	}
	if(ok)
		ASSIGN_PTR(pDbfFld, cf);
	return ok;
}
//
//
//
#define SDRECORD_SVER 0

void SdRecord::Init()
{
	// Функция не изменяет занчение поля Flags
	Ver = SDRECORD_SVER;
	ID = 0;
	DescrPos = 0;
	RecSize = 0;
	P_DataBuf = 0;
	IsDataBufOwner = 0;
	OuterDataBufSize = 0;
	Items.freeAll();
	StringPool.clear();
	StringPool.add("$."); // Zero position is invalid
}

SdRecord::SdRecord(long flags) : Items(sizeof(F))
{
	Init();
	Flags = flags;
}

SdRecord::SdRecord(const SdRecord & s) : Items(sizeof(F))
{
	Init();
	Flags = 0;
	Copy(s);
}

SdRecord::~SdRecord()
{
	DestroyDataBuf();
}

SdRecord & FASTCALL SdRecord::operator = (const SdRecord & s)
{
	Copy(s);
	return *this;
}

int SdRecord::DestroyDataBuf()
{
	if(IsDataBufOwner) {
		ZFREE(P_DataBuf);
		IsDataBufOwner = 0;
		return 1;
	}
	else
		return -1;
}

int FASTCALL SdRecord::IsEqual(const SdRecord & rPat) const
{
	int    ok = 1;
	uint   c;
	SdbField f1, f2;
	THROW(ID == rPat.ID);
	THROW(Flags == rPat.Flags);
	THROW(Name.Cmp(rPat.Name, 0) == 0);
	c = GetCount();
	THROW(c == rPat.GetCount());
	if(c) do {
		--c;
		THROW(GetFieldByPos(c, &f1));
		THROW(rPat.GetFieldByPos(c, &f2));
		THROW(f1.IsEqual(f2));
	} while(c);
	CATCHZOK
	return ok;
}

int FASTCALL SdRecord::Copy(const SdRecord & rSrc)
{
	int    ok = 1;
	ID    = rSrc.ID;
	Flags = rSrc.Flags;
	DescrPos = rSrc.DescrPos;
	Name  = rSrc.Name;
	THROW(Items.copy(rSrc.Items));
	StringPool = rSrc.StringPool;
	RecSize = rSrc.RecSize;
	TempBuf = rSrc.TempBuf;
	if(rSrc.IsDataBufOwner) {
		if(rSrc.P_DataBuf) {
			THROW(AllocDataBuf());
		}
		else
			P_DataBuf = 0;
	}
	else
		SetDataBuf(rSrc.P_DataBuf, rSrc.OuterDataBufSize);
	CATCHZOK
	return ok;
}

int FASTCALL SdRecord::Write_(SBuffer & rBuf) const
{
	int    ok = 1;
	THROW(rBuf.Write(&ID, sizeof(ID)));
	THROW(rBuf.Write(Name));
	THROW(rBuf.Write(&Flags, sizeof(Flags)));
	THROW(rBuf.Write(&DescrPos, sizeof(DescrPos)));
	THROW(rBuf.Write(&Items, 0));
	THROW(StringPool.Write(rBuf));
	CATCHZOK
	return ok;
}

int FASTCALL SdRecord::Read_(SBuffer & rBuf)
{
	int    ok = 1;
	THROW(rBuf.Read(&ID, sizeof(ID)));
	THROW(rBuf.Read(Name));
	THROW(rBuf.Read(&Flags, sizeof(Flags)));
	THROW(rBuf.Read(&DescrPos, sizeof(DescrPos)));
	THROW(rBuf.Read(&Items, 0));
	THROW(StringPool.Read(rBuf));
	SetupOffsets();
	CATCHZOK
	return ok;
}

int SdRecord::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	THROW(pSCtx->Serialize(dir, Ver, rBuf));
	if(dir < 0) {
		THROW_S_S(Ver == SDRECORD_SVER, SLERR_INVSERIALIZEVER, "SdRecord");
	}
	THROW(pSCtx->Serialize(dir, ID, rBuf));
	THROW(pSCtx->Serialize(dir, Name, rBuf));
	THROW(pSCtx->Serialize(dir, Flags, rBuf));
	THROW(pSCtx->Serialize(dir, DescrPos, rBuf));
	THROW(pSCtx->Serialize(dir, &Items, rBuf));
	THROW(StringPool.Serialize(dir, rBuf, pSCtx));
	if(dir < 0) {
		SetupOffsets();
	}
	CATCHZOK
	return ok;
}

void FASTCALL SdRecord::SetDescription(const char * pDescr)
{
	if(!isempty(pDescr))
		StringPool.add(pDescr, (uint *)&DescrPos);
	else
		DescrPos = 0;
}

int SdRecord::GetDescription(SString & rDescr) const
{
	int    ok = -1;
	if(DescrPos) {
		if(StringPool.get(DescrPos, rDescr))
			ok = 1;
	}
	else
		rDescr.Z();
	return ok;
}

void   SdRecord::Clear() { Init(); }
const  SdRecord::F * FASTCALL SdRecord::GetC(uint pos) const { return static_cast<F *>(Items.at(pos)); }
// private
SdRecord::F * FASTCALL SdRecord::Get(uint pos) const { return static_cast<F *>(Items.at(pos)); }
uint   SdRecord::GetCount() const { return Items.getCount(); }
size_t SdRecord::GetRecSize() const { return RecSize; }

void SdRecord::SetDataBuf(void * pBuf, size_t bufSize)
{
	DestroyDataBuf();
	P_DataBuf = pBuf;
	OuterDataBufSize = bufSize;
}

int SdRecord::AllocDataBuf()
{
	int    ok = 1;
	if(!IsDataBufOwner) {
		P_DataBuf = 0;
		OuterDataBufSize = 0;
	}
	P_DataBuf = SAlloc::R(P_DataBuf, RecSize);
	IsDataBufOwner = 1;
	if(P_DataBuf == 0 && RecSize > 0)
		ok = 0;
	else
		memzero(P_DataBuf, RecSize);
	return ok;
}

int SdRecord::ClearDataBuf()
{
	int   ok = 1;
	if(P_DataBuf) {
		memzero(P_DataBuf, RecSize);
	}
	else
		ok = 0;
	return ok;
}

const void * FASTCALL SdRecord::GetDataC(uint fldPos) const
{
	if(P_DataBuf && (fldPos < Items.getCount())) {
		const uint8 * p = static_cast<const uint8 *>(P_DataBuf);
		return (p + Get(fldPos)->InnerOffs);
	}
	else
		return 0;
}

void * FASTCALL SdRecord::GetData(uint fldPos) const
{
	return const_cast<void *>(GetDataC(fldPos));
}

int SdRecord::ScanName(SStrScan & rScan, uint * pPos, uint excludePos) const
{
	F * p_item;
	SString temp_buf;
	SString scan_buf = rScan;
	rScan.Len = 0;
	for(uint i = 0; Items.enumItems(&i, (void **)&p_item);) {
		if((i-1) != excludePos) {
			StringPool.get(p_item->NamePos, temp_buf);
			if(scan_buf.HasPrefixNC(temp_buf)) {
				rScan.Len = temp_buf.Len();
				ASSIGN_PTR(pPos, i-1);
				return 1;
			}
		}
	}
	ASSIGN_PTR(pPos, 0); // @v9.8.7
	return 0;
}

int SdRecord::SearchName(const char * pName, uint * pPos, uint excludePos) const
{
	if(!isempty(pName)) {
		F * p_item;
		SString temp_buf;
		const int is_ascii_ = sisascii(pName, sstrlen(pName));
		for(uint i = 0; Items.enumItems(&i, (void **)&p_item);) {
			if((i-1) != excludePos) {
				StringPool.get(p_item->NamePos, temp_buf);
				if(is_ascii_) {
					if(temp_buf.IsEqiAscii(pName)) {
						ASSIGN_PTR(pPos, i-1);
						return 1;
					}
				}
				else if(temp_buf.CmpNC(pName) == 0) {
					ASSIGN_PTR(pPos, i-1);
					return 1;
				}
			}
		}
	}
	ASSIGN_PTR(pPos, 0); // @v9.8.7
	return 0;
}

int SdRecord::SetupOffsets()
{
	if(Flags & fNoData) {
		RecSize = 0;
	}
	else {
		F *  p_item;
		size_t offs = 0, rec_size = 0;
		for(uint i = 0; Items.enumItems(&i, (void **)&p_item);) {
			p_item->InnerOffs = offs;
			if(!(Flags & fEnum)) {
				size_t sz = p_item->T.GetBinSize();
				offs += sz;
				rec_size += sz;
			}
		}
		RecSize = (Flags & fEnum) ? sizeof(uint32) : rec_size;
	}
	return 1;
}

int SdRecord::SetText(uint * pPos, const char * pText)
{
	int    ok = -1;
	TempBuf = pText;
	if(TempBuf.NotEmptyS()) {
		StringPool.add(TempBuf, pPos);
		ok = 1;
	}
	else {
		ASSIGN_PTR(pPos, 0);
	}
	return ok;
}

int SdRecord::GetText(uint pos, SString & rText) const
{
	int    ok = -1;
	rText.Z();
	if(!pos || StringPool.get(pos, rText))
		ok = 1;
	return ok;
}

int SdRecord::AddField(uint * pID, const SdbField * pFld)
{
	EXCEPTVAR(SLibError);
	int    ok = 1;
	SString msg_buf;
	F      f;
	f.ID  = pFld->ID;
	f.T   = pFld->T;
	f.OuterFormat = pFld->OuterFormat;
	f.InnerOffs = 0;
	f.ID = 0;
	f.NamePos = 0;
	f.DescrPos = 0;
	f.FormulaPos = 0;
	if(pID && (*pID || (pFld->T.Flags & STypEx::fZeroID))) {
		uint   pos = 0;
		if(Items.lsearch(&pID, &pos, CMPF_LONG) && !(Flags & fAllowDupID)) {
			StringPool.get(Get(pos)->NamePos, msg_buf);
			SLS.SetAddedMsgString(msg_buf);
			CALLEXCEPTV(SLERR_SDREC_DUPFLDID);
		}
		else
			f.ID = *pID;
	}
	else {
		F * p_item;
		for(uint i = 0; Items.enumItems(&i, (void **)&p_item);)
			if(p_item->ID > f.ID)
				f.ID = p_item->ID;
		f.ID++;
	}
	TempBuf = pFld->Name;
	if(Flags & fNamesToUpper)
		TempBuf.ToUpper();
	if(SearchName(TempBuf, 0) && !(Flags & fAllowDupName)) {
		SLS.SetAddedMsgString(TempBuf.Transf(CTRANSF_OUTER_TO_INNER)); // @v10.1.11 .Transf(CTRANSF_OUTER_TO_INNER)
		CALLEXCEPTV(SLERR_SDREC_DUPFLDNAME);
	}
	else {
		StringPool.add(TempBuf, &f.NamePos);
		TempBuf = pFld->Descr;
		if(TempBuf.NotEmptyS())
			StringPool.add(TempBuf, &f.DescrPos);
		TempBuf = pFld->Formula;
		if(TempBuf.NotEmptyS())
			StringPool.add(TempBuf, &f.FormulaPos);
		THROW(Items.insert(&f));
		SetupOffsets();
		ASSIGN_PTR(pID, f.ID);
	}
	CATCHZOK
	return ok;
}

int SdRecord::RemoveField(uint pos)
{
	int    ok = 1;
	if(Items.atFree(pos))
		SetupOffsets();
	else
		ok = 0;
	return ok;
}

int SdRecord::UpdateField(uint pos, const SdbField * pFld)
{
	int    ok = 1;
	F    * p_item = 0;
	THROW(checkupper(pos, GetCount()));
	p_item = Get(pos);
	TempBuf = pFld->Name;
	if(Flags & fNamesToUpper)
		TempBuf.ToUpper();
	if(!(Flags & fAllowDupID) || !(Flags & fAllowDupName)) {
		SString name_buf;
		for(uint i = 0; i < Items.getCount(); i++) {
			F * p_f = Get(i);
			if(pos != i) {
				StringPool.get(p_f->NamePos, name_buf);
				if(pFld->ID != 0 || !(pFld->T.Flags & STypEx::fZeroID)) { // @v7.4.1
					THROW_S_S((p_f->ID != pFld->ID || Flags & fAllowDupID), SLERR_SDREC_DUPFLDID, (TempBuf = name_buf).ToOem());
				}
				if(!(Flags & fAllowDupName)) {
					THROW_S(name_buf.CmpNC(TempBuf), SLERR_SDREC_DUPFLDNAME);
				}
			}
		}
	}
	StringPool.add(TempBuf, &p_item->NamePos);
	TempBuf = pFld->Descr;
	if(TempBuf.NotEmptyS())
		StringPool.add(TempBuf, &p_item->DescrPos);
	TempBuf = pFld->Formula;
	if(TempBuf.NotEmptyS())
		StringPool.add(TempBuf, &p_item->FormulaPos);
	p_item->ID  = pFld->ID;
	p_item->T.Typ = pFld->T.Typ;
	p_item->T.Flags = pFld->T.Flags;
	p_item->OuterFormat = pFld->OuterFormat;
	SetupOffsets();
	CATCHZOK
	return ok;
}

int SdRecord::MoveField(uint pos, int up, uint * pNewPos)
{
	return Items.moveItem(pos, up, pNewPos);
}

int FASTCALL SdRecord::GetFieldByPos(uint pos, SdbField * pFld) const
{
	int    ok = 1;
	CALLPTRMEMB(pFld, Init());
	if(pos < Items.getCount()) {
		if(pFld) {
			const F * p_item = static_cast<const F *>(Items.at(pos));
			pFld->ID  = p_item->ID;
			pFld->T   = p_item->T;
			pFld->T.Flags = p_item->T.Flags;
			pFld->OuterFormat = p_item->OuterFormat;
			pFld->InnerOffs = p_item->InnerOffs;
			StringPool.getnz(p_item->NamePos, pFld->Name);
			StringPool.getnz(p_item->DescrPos, pFld->Descr);
			StringPool.getnz(p_item->FormulaPos, pFld->Formula);
		}
	}
	else
		ok = 0;
	return ok;
}

int FASTCALL SdRecord::GetFieldByPos_Fast(uint pos /*0..*/, SdbField * pFld) const
{
	int    ok = 1;
	CALLPTRMEMB(pFld, Init());
	if(pos < Items.getCount()) {
		if(pFld) {
			const F * p_item = (const F *)Items.at(pos);
			pFld->ID  = p_item->ID;
			pFld->T   = p_item->T;
			pFld->T.Flags = p_item->T.Flags;
			pFld->OuterFormat = p_item->OuterFormat;
			pFld->InnerOffs = p_item->InnerOffs;
		}
	}
	else
		ok = 0;
	return ok;
}

int SdRecord::EnumFields(uint * pPos, SdbField * pFld) const
{
	int    ok = 0;
	if(pPos) {
		ok = GetFieldByPos(*pPos, pFld);
		if(ok)
			(*pPos)++;
	}
	return ok;
}

int SdRecord::GetFieldByID(uint id, uint * pPos, SdbField * pFld) const
{
	uint   pos = 0;
	CALLPTRMEMB(pFld, Init());
	if(Items.lsearch(&id, &pos, CMPF_LONG)) {
		ASSIGN_PTR(pPos, pos);
		return GetFieldByPos(pos, pFld);
	}
	else {
		char   temp_buf[32];
		SLS.SetAddedMsgString(itoa(id, temp_buf, 10));
		return (SLibError = SLERR_SDREC_FLDIDNFOUND, 0);
	}
}

int SdRecord::GetFieldByName(const char * pName, SdbField * pFld) const
{
	uint   pos = 0;
	TempBuf = pName;
	CALLPTRMEMB(pFld, Init());
	return (SearchName(TempBuf, &pos) > 0) ? GetFieldByPos(pos, pFld) : 0;
}

int SdRecord::GetFieldByName_Fast(const char * pName, SdbField * pFld) const
{
	uint   pos = 0;
	TempBuf = pName;
	CALLPTRMEMB(pFld, Init());
	return (SearchName(TempBuf, &pos) > 0) ? GetFieldByPos_Fast(pos, pFld) : 0;
}

TYPEID SdRecord::GetFieldType(uint pos /* 0.. */) const { return (pos < Items.getCount()) ? Get(pos)->T.Typ : 0; }
const  STypEx * SdRecord::GetFieldExType(uint pos /*0..*/) const { return (pos < Items.getCount()) ? &Get(pos)->T : 0; }
long   SdRecord::GetFieldOuterFormat(uint pos /*0..*/) const { return (pos < Items.getCount()) ? Get(pos)->OuterFormat : 0; }

void SdRecord::CreateRecFromDbfTable(const DbfTable * pTbl)
{
	Clear();
	uint   c = pTbl->getNumFields();
	for(uint i = 1; i <= c; i++) {
		DBFF   df;
		TYPEID typ = 0;
		long   fmt = 0;
		uint   fld_id = 0;
		if(pTbl->getField(i, &df)) {
			SdbField fld;
			df.GetSType(&typ, &fmt);
			fld.T.Typ = typ;
			fld.OuterFormat = fmt;
			fld.Name = df.fname;
			AddField(&fld_id, &fld);
		}
	}
}

int SdRecord::ConvertDataFields(int cvt, void * pBuf) const
{
	int    ok = -1;
	if(pBuf && oneof4(cvt, CTRANSF_INNER_TO_OUTER, CTRANSF_OUTER_TO_INNER, CTRANSF_UTF8_TO_OUTER, CTRANSF_UTF8_TO_INNER)) {
		SdbField fld;
		SString temp_buf;
		const uint _c = GetCount();
		for(uint i = 0; i < _c; i++) {
			GetFieldByPos_Fast(i, &fld);
			void * p_fld_data = PTR8(pBuf) + fld.InnerOffs;
			size_t len;
			if(fld.T.IsZStr(&len)) {
				temp_buf = (const char *)p_fld_data;
				temp_buf.Transf(cvt);
				temp_buf.CopyTo((char *)p_fld_data, len);
				ok = 1;
			}
		}
	}
	else
		ok = 0;
	return ok;
}
//
//
//
SLAPI SdRecordBuffer::SdRecordBuffer(size_t maxSize) : MaxSize(NZOR(maxSize, 4096)), MaxRecSize(0), Flags(fEqRec)
{
	SBaseBuffer::Init();
	if(Alloc(MaxSize)) {
		PTR16(P_Buf)[0] = 0; // Обнуляем счетчик записей.
		Pos = sizeof(uint16);
	}
	else
		Pos = 0;
}

SLAPI SdRecordBuffer::~SdRecordBuffer()
{
	SBaseBuffer::Destroy();
}

int SLAPI SdRecordBuffer::Reset()
{
	MaxRecSize = 0;
	Flags = fEqRec;
	if(P_Buf) {
		PTR16(P_Buf)[0] = 0;
		Pos = sizeof(uint16);
		return 1;
	}
	else {
		Pos = 0;
		return 0;
	}
}

int SLAPI SdRecordBuffer::Add(const void * pRecData, size_t recSize)
{
	int    ok = 0;
	if(P_Buf && recSize < SKILOBYTE(32)) {
		if((Pos+recSize+sizeof(uint16)) < MaxSize) { 
			*PTR16(P_Buf+Pos) = static_cast<uint16>(recSize);
			Pos += sizeof(uint16);
			memcpy(P_Buf+Pos, pRecData, recSize);
			Pos += recSize;
			(*PTR16(P_Buf))++; // Увеличивает счетчик записей.
			if(MaxRecSize == 0)
				MaxRecSize = recSize;
			else if(recSize != MaxRecSize) {
				Flags &= ~fEqRec; // @v10.4.3 @fix fEqRec-->~fEqRec 
				if(recSize > MaxRecSize)
					MaxRecSize = recSize;
			}
			ok = 1;
		}
		else
			ok = -1;
	}
	return ok;
}

int  SLAPI SdRecordBuffer::IsEqRec() const { return BIN(Flags & fEqRec); }
uint SLAPI SdRecordBuffer::GetCount() const { return P_Buf ? *PTR16(P_Buf) : 0; }

SBaseBuffer FASTCALL SdRecordBuffer::Get(uint recNo) const
{
	SBaseBuffer ret_buf;
	ret_buf.Init();
	const uint c = GetCount();
	if(recNo < c) {
		size_t pos = sizeof(uint16);
		size_t sz  = *PTR16(P_Buf+pos);
		pos += sizeof(uint16);
		for(uint i = 0; i < recNo; i++) {
			pos += sz;
			sz = *PTR16(P_Buf+pos);
			pos += sizeof(uint16);
		}
		ret_buf.Size = sz;
		ret_buf.P_Buf = P_Buf+pos;
	}
	return ret_buf;
}

SBaseBuffer SLAPI SdRecordBuffer::GetBuf() const
{
	SBaseBuffer ret_buf;
	ret_buf.P_Buf = P_Buf;
	ret_buf.Size = Pos;
	return ret_buf;
}
