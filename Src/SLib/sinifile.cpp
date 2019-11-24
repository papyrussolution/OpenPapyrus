// SINIFILE.CPP
// Copyright (c) A.Sobolev 2007, 2010, 2011, 2014, 2015, 2016, 2017, 2018, 2019
//
#include <slib.h>
#include <tv.h>
#pragma hdrstop

class SIniSectBuffer : public TSCollection <StringSet> {
public:
	SLAPI  SIniSectBuffer(const char * pName = 0);
	int    SLAPI SetParam(const char * pParam, const char * pVal, int overwrite);
	int    SLAPI GetParam(const char * pParam, SString & rBuf) const;
	int    SLAPI RemoveParam(const char * pParam);
	int    SLAPI EnumParams(uint * pPos, SString *, SString *) const;
	int    SLAPI ShiftParam(uint pos, int up); // (up != 0) - вверх, (up == 0) - вниз

	SString Name;
private:
	StringSet * SLAPI SearchParam(const char * pParam, uint * pPos) const;
};

class SIniFileBuffer : public TSCollection <SIniSectBuffer> {
public:
	SLAPI  SIniFileBuffer();
	SIniSectBuffer * SLAPI GetSect(const char * pName, uint * pPos = 0);
	int    SLAPI EnumSections(uint * pPos, SIniSectBuffer ** pSectBuf) const;
	int    SLAPI AddSect(SIniSectBuffer *);
	int    SLAPI AddSect(const char * pSectName);
	int    SLAPI RemoveSect(const char * pSectName);
	int    SLAPI ClearSect(const char * pSectName);
	int    SLAPI SetParam(const char * pSectName, const char * pParam, const char * pVal, int overwrite);
	int    SLAPI GetParam(const char * pSectName, const char * pParam, SString & rBuf);
	int    SLAPI RemoveParam(const char * pSectName, const char * pParam);
};

SLAPI SIniFileBuffer::SIniFileBuffer() : TSCollection <SIniSectBuffer> ()
{
}

SIniSectBuffer * SLAPI SIniFileBuffer::GetSect(const char * pName, uint * pPos)
{
	SIniSectBuffer * p_result = 0;
	for(uint i = 0; !p_result && i < getCount(); i++) {
		SIniSectBuffer * p_sect_buf = static_cast<SIniSectBuffer *>(at(i));
		if(p_sect_buf->Name.CmpNC(pName) == 0) {
			ASSIGN_PTR(pPos, i);
			p_result = p_sect_buf;
		}
	}
	return p_result;
}

int SLAPI SIniFileBuffer::EnumSections(uint * pPos, SIniSectBuffer ** ppSectBuf) const
{
	int    ok = 0;
	if(pPos && *pPos < getCount()) {
		*ppSectBuf = at(*pPos);
		(*pPos)++;
		ok = 1;
	}
	return ok;
}

int SLAPI SIniFileBuffer::SetParam(const char * pSectName, const char * pParam, const char * pVal, int overwrite)
{
	int    ok = -1;
	SIniSectBuffer * p_sect_buf = GetSect(pSectName);
	if(!p_sect_buf) {
		AddSect(pSectName);
		p_sect_buf = GetSect(pSectName);
	}
	if(p_sect_buf)
		ok = p_sect_buf->SetParam(pParam, pVal, overwrite);
	return ok;
}

int SLAPI SIniFileBuffer::GetParam(const char * pSectName, const char * pParam, SString & rBuf)
{
	SIniSectBuffer * p_sect_buf = GetSect(pSectName);
	return p_sect_buf ? p_sect_buf->GetParam(pParam, rBuf) : -1;
}

int SLAPI SIniFileBuffer::RemoveParam(const char * pSectName, const char * pParam)
{
	SIniSectBuffer * p_sect_buf = GetSect(pSectName, 0);
	return p_sect_buf ? p_sect_buf->RemoveParam(pParam) : -1;
}

int SLAPI SIniFileBuffer::AddSect(SIniSectBuffer * pSectBuf)
{
	int    ok = -1;
	if(pSectBuf) {
		SIniSectBuffer * p_sect_buf = GetSect(pSectBuf->Name);
		if(!p_sect_buf)
            ok = insert(pSectBuf);
		else {
			SString par, val;
			for(uint pos = 0; pSectBuf->EnumParams(&pos, &par, &val) > 0;)
				p_sect_buf->SetParam(par, val, 1);
			delete pSectBuf;
			ok = 1;
		}
	}
	return ok;
}

int SLAPI SIniFileBuffer::AddSect(const char * pSectName)
{
	int    ok = -1;
	if(pSectName) {
		SIniSectBuffer * p_sect_buf = new SIniSectBuffer(pSectName);
		if(p_sect_buf)
			ok = AddSect(p_sect_buf);
	}
	return ok;
}

int SLAPI SIniFileBuffer::RemoveSect(const char * pSectName)
{
	uint   pos = 0;
	SIniSectBuffer * p_sect_buf = GetSect(pSectName, &pos);
	return p_sect_buf ? atFree(pos) : -1;
}

int SLAPI SIniFileBuffer::ClearSect(const char * pSectName)
{
	SIniSectBuffer * p_sect_buf = GetSect(pSectName);
	return p_sect_buf ? (p_sect_buf->freeAll(), 1) : -1;
}
//
// SIniSectBuffer
//
SLAPI SIniSectBuffer::SIniSectBuffer(const char * pName) : TSCollection <StringSet> (), Name(pName)
{
}

StringSet * SLAPI SIniSectBuffer::SearchParam(const char * pParam, uint * pPos) const
{
	for(uint i = 0; i < getCount(); i++) {
		StringSet * p_ss = at(i);
		if(stricmp(p_ss->getBuf(), pParam) == 0) {
			ASSIGN_PTR(pPos, i);
			return p_ss;
		}
	}
	return 0;
}

int SLAPI SIniSectBuffer::EnumParams(uint * pPos, SString * pParam, SString * pVal) const
{
	if(pPos && *pPos < getCount()) {
		uint pos = 0;
		const StringSet * p_ss = at(*pPos);
		if(pParam)
			p_ss->get(&pos, *pParam);
		if(pVal)
			p_ss->get(&pos, *pVal);
		(*pPos)++;
		return 1;
	}
	else
		return 0;
}

int SLAPI SIniSectBuffer::GetParam(const char * pParam, SString & rBuf) const
{
	StringSet * p_ss = SearchParam(pParam, 0);
	if(p_ss) {
		uint   pos = 0;
		if(!p_ss->get(&pos, 0, 0) || !p_ss->get(&pos, rBuf))
			rBuf.Z();
		return 1;
	}
	else
		return 0;
}

int SLAPI SIniSectBuffer::SetParam(const char * pParam, const char * pVal, int overwrite)
{
	uint   pos = 0;
	StringSet * p_ss = SearchParam(pParam, &pos);
	if(p_ss) {
		StringSet * p_nss = 0;
		if(pVal) {
			p_nss = new StringSet;
			p_nss->add(pParam);
			p_nss->add(pVal);
		}
		if(overwrite) {
			atFree(pos);
			if(p_nss) {
				p_nss->add(pParam);
				p_nss->add(pVal);
				atInsert(pos, p_nss);
			}
		}
		else if(p_nss)
			insert(p_nss);
	}
	else if(pVal) {
		StringSet * p_nss = CreateNewItem();
		if(p_nss) {
			p_nss->add(pParam);
			p_nss->add(pVal);
		}
	}
	return 1;
}

int SLAPI SIniSectBuffer::RemoveParam(const char * pParam)
{
	uint   pos = 0;
	return SearchParam(pParam, &pos) ? atFree(pos) : -1;
}

int SLAPI SIniSectBuffer::ShiftParam(uint pos, int up) // (up != 0) - вверх, (up == 0) - вниз
{
	int    ok = -1;
	if(pos < getCount()) {
		if(up) {
			if(pos > 0) {
				StringSet * p_temp_ss = new StringSet(*at(pos));
				atFree(pos);
				atInsert(pos-1, p_temp_ss);
				ok = 1;
			}
		}
		else {
			if(pos < (getCount()-1)) {
				StringSet * p_temp_ss = new StringSet(*at(pos));
				atFree(pos);
				atInsert(pos+1, p_temp_ss);
				ok = 1;
			}
		}
	}
	return ok;
}
//
//
//
SLAPI SIniFile::SIniFile(const char * pFileName, int fcreate, int winCoding, int useIniBuf) : P_IniBuf(0), Flags(0), Cp(cpUndef)
{
	Init(pFileName, fcreate, winCoding, useIniBuf);
}

int SLAPI SIniFile::WasModified() const
{
	SFileUtil::Stat fs;
	if(!SFileUtil::GetStat(FileName, &fs))
		return 2;
	else if(cmp(fs.ModTime, LoadingTime) > 0)
		return 1;
	else
		return 0;
}

// protected
int SLAPI SIniFile::Init(const char * pFileName, int fcreate, int winCoding, int useIniBuf)
{
	int    ok = 1;
	Flags = 0;
	SETFLAG(Flags, fWinCoding, winCoding);
	FileName = pFileName;
	ZDELETE(P_IniBuf);
	P_IniBuf = useIniBuf ? new SIniFileBuffer() : 0;
	if(pFileName) {
		if(fcreate || !fileExists(FileName))
			ok = Create(FileName);
		else
			ok = Open(FileName);
		if(P_IniBuf)
			InitIniBuf();
	}
	else
		ok = -1;
	return ok;
}

SLAPI SIniFile::~SIniFile()
{
	//FlashIniBuf();
	delete P_IniBuf;
	Close();
}

int  SLAPI SIniFile::IsValid() const { return (File.IsValid() || (Flags & fIniBufInited)) ? 1 : SLS.SetError(SLERR_INIOPENFAULT, FileName); }
long SLAPI SIniFile::GetFlags() const { return Flags; }
const SString & SLAPI SIniFile::GetFileName() const { return FileName; }
int  SLAPI SIniFile::Close() { return File.Close(); }
int  SLAPI SIniFile::GetParam(const char * pSect, const char * pParam, SString & rBuf)
	{ return (Flags & fIniBufInited) ? P_IniBuf->GetParam(pSect, pParam, rBuf) : SearchParam(pSect, pParam, rBuf); }
int  SLAPI SIniFile::AppendIntParam(const char * pSect, const char * pParam, int val, int overwrite)
	{ return AppendParam(pSect, pParam, TempBuf.Z().Cat((long)val), BIN(overwrite)); }
int  SLAPI SIniFile::RemoveSection(const char * pSect)
	{ return SetParam(pSect, 0, 0, 0); }
int  SLAPI SIniFile::ClearSection(const char * pSect)
	{ return (Flags & fIniBufInited) ? P_IniBuf->ClearSect(pSect) : RemoveSection(pSect); }

long SLAPI SIniFile::SetFlag(long f, int set)
{
	const long prev = Flags;
	SETFLAG(Flags, f, set);
	return prev;
}

SString & FASTCALL SIniFile::EncodeText(SString & rBuf) const
{
	if(Cp == cp1251)
		rBuf.Transf(CTRANSF_INNER_TO_OUTER);
	else if(Cp == cpUTF8)
		rBuf.Transf(CTRANSF_INNER_TO_UTF8);
	return rBuf;
}

SString & FASTCALL SIniFile::DecodeText(SString & rBuf) const
{
	if(Cp == cp1251)
		rBuf.Transf(CTRANSF_OUTER_TO_INNER);
	else if(Cp == cpUTF8)
		rBuf.Transf(CTRANSF_UTF8_TO_INNER);
	return rBuf;
}

int SLAPI SIniFile::FlashIniBuf()
{
	int    ok = 1;
	if(P_IniBuf) {
		SString par, val, temp_buf;
		SIniSectBuffer * p_sect = 0;
		Close();
		THROW(Create(FileName));
		Flags &= ~fIniBufInited;
		for(uint i = 0; P_IniBuf->EnumSections(&i, &p_sect) > 0;) {
			temp_buf.Z().CR().CatBrackStr(p_sect->Name).CR();
			/*if(Flags & fWinCoding) temp_buf.Transf(CTRANSF_INNER_TO_OUTER);*/
			EncodeText(temp_buf);
			THROW(File.WriteLine(temp_buf));
			for(uint j = 0; p_sect->EnumParams(&j, &par, &val) > 0;) {
				/*if(Flags & fWinCoding) par.Transf(CTRANSF_INNER_TO_OUTER);*/
				EncodeText(par);
				EncodeText(val); // @v10.3.11
				THROW(File.WriteLine(temp_buf.Z().CatEq(par, val).CR()));
			}
		}
		Flags |= fIniBufInited;
	}
	CATCHZOK
	Close();
	return ok;
}

int SLAPI SIniFile::InitIniBuf()
{
	int    ok = 1;
	THROW(File.IsValid());
	if(P_IniBuf) {
		StringSet ss;
		StringSet se;
		SString sect_buf, par_buf, par, val;
		GetSections(&ss);
		for(uint i = 0; ss.get(&i, sect_buf);) {
			se.clear();
			THROW(P_IniBuf->AddSect(sect_buf));
			GetEntries(sect_buf, &se, 1);
			for(uint j = 0; se.get(&j, par_buf);) {
				par_buf.Divide('=', par, val);
				//if(Flags & fWinCoding) val.Transf(CTRANSF_INNER_TO_OUTER);
				// EncodeText(val);
				THROW(P_IniBuf->SetParam(sect_buf, par, val, 0));
			}
		}
		Flags |= fIniBufInited;
	}
	CATCHZOK
	return ok;
}
// } AHTOXA

int SLAPI SIniFile::Open(const char * pFileName)
{
	const  int already_opened = File.IsValid();
	int    ok = already_opened ? -1 : File.Open(pFileName, SFile::mRead);
	if(ok) {
		// @v10.3.11 {
		if(!already_opened) {
			LoadingTime = getcurdatetime_();
			Cp = cpUndef;
			STextEncodingStat tes(STextEncodingStat::fUseUCharDet);
			SString line_buf;
			SString big_line_buf;
			size_t total_len = 0;
			while(File.ReadLine(line_buf)) {
				total_len += line_buf.Len();
				big_line_buf.Cat(line_buf);
				if(big_line_buf.Len() >= SKILOBYTE(1)) {
					tes.Add(big_line_buf.cptr(), big_line_buf.Len());
					big_line_buf.Z();
				}
			}
			File.Seek(0);
			if(big_line_buf.Len())
				tes.Add(big_line_buf.cptr(), big_line_buf.Len());
			tes.Finish();
			if(tes.CheckFlag(tes.fLegalUtf8Only))
				Cp = cpUTF8;
			else
				Cp = tes.GetAutodetectedCp();
			if(Cp == cpUndef) {
				Cp = (Flags & fWinCoding) ? cp1251 : cp866;
			}
		}
		// } @v10.3.11
	}
	return ok;
}

int SLAPI SIniFile::Create(const char * pFileName)
{
	int    ok = 1;
	if(!File.IsValid()) {
		ok = File.Open(pFileName, SFile::mWrite);
		LoadingTime = getcurdatetime_();
	}
	return ok;
}

#pragma warn -aus

int SLAPI SIniFile::IsSection(const SString & rLineBuf, const char * pPattern, SString * pRet)
{
	int    ok = -1;
	SString sect_name;
	Scan.Set(rLineBuf, 0);
	Scan.Skip();
	if(*Scan == '[') {
		Scan.Incr(1);
		if(Scan.SearchChar(']')) {
			Scan.Get(sect_name);
			if(sect_name.NotEmptyS()) {
				//if(Flags & fWinCoding) sect_name.Transf(CTRANSF_OUTER_TO_INNER);
				DecodeText(sect_name);
				ok = (pPattern && sect_name.CmpNC(pPattern) == 0) ? 2 : 1;
				ASSIGN_PTR(pRet, sect_name);
			}
		}
	}
	return ok;
}

int SLAPI SIniFile::IsSectExists(const char * pSect)
{
	int    ok = 0;
	if(pSect) {
		if(Flags & fIniBufInited)
			ok = (P_IniBuf->GetSect(pSect) == 0) ? 0 : 1;
		else {
			SString line_buf, sect_name;
			for(File.Seek(0); !ok && File.ReadLine(line_buf);)
				if(IsSection(line_buf, 0, &sect_name) > 0 && sect_name.CmpNC(pSect) == 0)
					ok = 1;
		}
	}
	return ok;
}

int SLAPI SIniFile::GetSections(StringSet * pSects)
{
	int    ok = 1;
	int    do_close = 0;
	if(Flags & fIniBufInited) {
		if(pSects) {
			SIniSectBuffer * p_sect = 0;
			for(uint i = 0; P_IniBuf->EnumSections(&i, &p_sect) > 0;)
				pSects->add(p_sect->Name);
		}
	}
	else {
		SString line_buf, sect_name;
		const int opnr = Open(FileName);
		THROW(opnr);
		do_close = BIN(opnr > 0);
		for(File.Seek(0); File.ReadLine(line_buf);)
			if(IsSection(line_buf, 0, &sect_name) > 0)
				pSects->add(sect_name);
	}
	CATCHZOK
	if(do_close)
		Close();
	return ok;
}

int SLAPI SIniFile::GetEntries(const char * pSect, StringSet * pEntries, int storeAllString)
{
	int    ok = 1;
	int    do_close = 0;
	SString line_buf, temp_buf, val;
	if(Flags & fIniBufInited) {
		if(pEntries) {
			SIniSectBuffer * p_sect_buf = P_IniBuf->GetSect(pSect);
			if(p_sect_buf) {
				for(uint pos = 0; p_sect_buf->EnumParams(&pos, &temp_buf, &val) > 0;) {
					if(storeAllString) {
						//if(Flags & fWinCoding) val.Transf(CTRANSF_OUTER_TO_INNER);
						// (В буфере текст уже декодирован) DecodeText(val);
						temp_buf.Eq().Cat(val);
					}
					pEntries->add(temp_buf);
				}
			}
		}
	}
	else {
		int    this_sect = 0;
		const  int opnr = Open(FileName);
		THROW(opnr);
		do_close = BIN(opnr > 0);
		for(File.Seek(0); File.ReadLine(line_buf);) {
			int    r = IsSection(line_buf, pSect, 0);
			if(r > 0)
				this_sect = BIN(r == 2);
			else if(this_sect) {
				Scan.Set(line_buf.Chomp().Strip(), 0);
				Scan.Skip();
				if(*Scan && *Scan != ';') {
					if(storeAllString)
						temp_buf = line_buf;
					else if(line_buf.Divide('=', temp_buf, val) <= 0)
						temp_buf.Z();
					if(temp_buf.NotEmptyS()) {
						//if(Flags & fWinCoding) temp_buf.Transf(CTRANSF_OUTER_TO_INNER);
						DecodeText(temp_buf);
						THROW(pEntries->add(temp_buf));
					}
				}
			}
		}
	}
	CATCHZOK
	if(do_close)
		Close();
	return ok;
}

int SLAPI SIniFile::SearchParam(const char * pSect, const char * pParam, SString & rVal)
{
	int    ok = -1;
	int    do_close_file = 0;
	rVal.Z();
	if(Flags & fIniBufInited)
		ok = (P_IniBuf->GetParam(pSect, pParam, rVal) > 0) ? 1 : -1;
	else {
		int    this_sect = 0;
		SString sect = pSect;
		SString key = pParam;
		SString line_buf, temp_buf, val;
		const int opnr = Open(FileName);
		THROW(opnr);
		do_close_file = BIN(opnr > 0);
		/*if(Flags & fWinCoding) {
			sect.Transf(CTRANSF_OUTER_TO_INNER);
			key.Transf(CTRANSF_OUTER_TO_INNER);
		}*/
		DecodeText(sect);
		DecodeText(key);
		for(File.Seek(0); ok < 0 && File.ReadLine(line_buf);) {
			int    r = IsSection(line_buf, sect, 0);
			if(r > 0)
				this_sect = (r == 2) ? 1 : 0;
			else if(this_sect || isempty(pSect)) {
				Scan.Set(line_buf.Chomp().Strip(), 0);
				Scan.Skip();
				if(*Scan != ';') {
					line_buf.Divide('=', temp_buf, val);
					if(temp_buf.Strip().CmpNC(key) == 0) {
						rVal = DecodeText(val.Strip());
						ok = 1; // end of loop
					}
				}
			}
		}
	}
	CATCHZOK
	if(do_close_file)
		Close();
	return ok;
}

#pragma warn .aus

int SLAPI SIniFile::GetIntParam(const char * pSect, const char * pParam, int * pVal)
{
	int    r = SearchParam(pSect, pParam, TempBuf);
	if(pVal)
		*pVal = r ? TempBuf.ToLong() : 0;
	return r;
}

static int FASTCALL ParseDataSizeString(const char * pText, int64 * pSize, int64 * pMult)
{
	int    ok = 0; 
	int64  value = 0;
	int64  mult = 1;
	SString temp_buf;
	const char * p = pText;
	while(oneof2(*p, ' ', '\t'))
		p++;
	while(isdec(*p)) {
		temp_buf.CatChar(*p++);
		ok = 1;
	}
	value = temp_buf.ToInt64();
	if(value != 0) {
		while(oneof2(*p, ' ', '\t'))
			p++;
		temp_buf.Z();
		while(*p)
			temp_buf.CatChar(*p++);
		if(temp_buf.NotEmptyS()) {
			if(temp_buf.IsEqiAscii("b"))
				mult = 1;
			else if(temp_buf.IsEqiAscii("kb") || temp_buf.IsEqiAscii("k"))
				mult = 1024;
			else if(temp_buf.IsEqiAscii("mb") || temp_buf.IsEqiAscii("m"))
				mult = 1024*1024;
			else if(temp_buf.IsEqiAscii("gb") || temp_buf.IsEqiAscii("g"))
				mult = 1024*1024*1024;
			else
				ok = 0;
		}
	}
	ASSIGN_PTR(pSize, value);
	ASSIGN_PTR(pMult, mult);
	return ok;
}

int SLAPI SIniFile::GetDataSizeParam(const char * pSect, const char * pParam, int64 * pVal)
{
	int    r = SearchParam(pSect, pParam, TempBuf);
	if(pVal) {
		if(r) {
			int64  s = 0;
			int64  m = 0;
			if(ParseDataSizeString(TempBuf, &s, &m))
				*pVal = s * m;
			else
				*pVal = 0;
		}
		else
			*pVal = 0;
	}
	return r;
}

int SLAPI SIniFile::SetParam(const char * pSect, const char * pParam, const char * pVal, int overwrite)
{
	int    ok = 1;
	int    do_close_file = 0;
	if(Flags & fIniBufInited) {
		if(pParam == 0)
			ok = P_IniBuf->RemoveSect(pSect);
		else if(pVal) {
			/* @v10.3.11 if(Flags & fWinCoding) {
				SString val = pVal;
				val.Transf(CTRANSF_INNER_TO_OUTER);
				ok = P_IniBuf->SetParam(pSect, pParam, val, overwrite);
			}
			else
				ok = P_IniBuf->SetParam(pSect, pParam, pVal, overwrite);*/
			// @v10.3.11 {
			//
			// Буфер держит строки в INNER-кодировке!
			//
			//SString & r_temp_buf = SLS.AcquireRvlStr();
			//r_temp_buf = pVal;
			//EncodeText(r_temp_buf);
			ok = P_IniBuf->SetParam(pSect, pParam, pVal, overwrite);
			// } @v10.3.11 
		}
		else
			ok = P_IniBuf->RemoveParam(pSect, pParam);
	}
	else {
		SString sect  = pSect;
		SString param = pParam;
		SString line_buf, temp_buf, temp_key, temp_val;
		SFile  out_file;
		SPathStruc ps;
		int    this_sect = 0, is_sect_founded = 0, param_added = 0;
		const  int opnr = Open(FileName);
		THROW(opnr);
		do_close_file = BIN(opnr > 0);
		/*if(Flags & fWinCoding) {
			sect.Transf(CTRANSF_INNER_TO_OUTER);
			param.Transf(CTRANSF_INNER_TO_OUTER);
		}*/
		EncodeText(sect);
		EncodeText(param);
		ps.Split(FileName);
		ps.Merge(0, SPathStruc::fNam|SPathStruc::fExt, temp_buf);
		MakeTempFileName(temp_buf, 0, 0, 0, temp_buf);
		THROW(out_file.Open(temp_buf, SFile::mWrite));
		for(File.Seek(0); File.ReadLine(line_buf);) {
			int    do_write_org_line = 1;
			int    r = IsSection(line_buf, sect, 0);
			if(r > 0) {
				if(this_sect && !param_added && pVal) {
					THROW(out_file.WriteLine(temp_buf.Z().CatEq(param, pVal).CR().CR()));
					param_added = 1;
				}
				this_sect = (r == 2) ? 1 : 0;
				if(this_sect) {
					is_sect_founded = 1;
					if(!pParam)
						do_write_org_line = 0;
				}
			}
			else if(this_sect) {
				if(!pParam)
					do_write_org_line = 0;
				else if(!param_added && (!pVal || overwrite)) {
					temp_buf = line_buf;
					Scan.Set(temp_buf.Chomp().Strip(), 0);
					Scan.Skip();
					if(*Scan != ';' && temp_buf.Divide('=', temp_key, temp_val) > 0) {
						if(temp_key.CmpNC(param) == 0) {
							if(pVal)
								THROW(out_file.WriteLine(temp_buf.Z().CatEq(param, pVal).CR()));
							do_write_org_line = 0;
						}
					}
				}
			}
			if(do_write_org_line)
				THROW(out_file.WriteLine(line_buf));
		}
		if(!param_added && pParam && pVal) {
			if(!is_sect_founded)
				THROW(out_file.WriteLine(temp_buf.Z().CR().CatBrackStr(sect).CR()));
			THROW(out_file.WriteLine(temp_buf.Z().CatEq(param, pVal).CR()));
		}
		Close();
		temp_buf = out_file.GetName();
		out_file.Close();
		THROW(SFile::Remove(FileName));
		SDelay(10);
		THROW(SFile::Rename(temp_buf, FileName));
	}
	CATCHZOK
	if(do_close_file)
		Close();
	return ok;
}

int SLAPI SIniFile::AppendParam(const char * pSect, const char * pParam, const char * pVal, int overwrite)
{
	SString val = pVal;
	if(val.cptr() == 0)
		val.Space().Z();
	return SetParam(pSect, pParam, val, BIN(overwrite));
}

int SLAPI SIniFile::RemoveParam(const char * pSect, const char * pParam)
{
	SString param = pParam;
	if(param.cptr() == 0)
		param.Space().Z();
	return SetParam(pSect, pParam, 0, 1);
}

