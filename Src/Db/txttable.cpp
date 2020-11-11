// TXTTABLE.CPP
// Copyright (c) A.Sobolev 2006, 2007, 2008, 2009, 2010, 2012, 2015, 2017, 2018, 2019, 2020
// @codepage UTF-8
//
#include <slib-internal.h>
#pragma hdrstop
#include <db.h>
//
// @ModuleDef(TextDbFile)
//
#define TEXTDBFILEPARAM_SVER 0

TextDbFile::Param::Param(long flags, const char * pFldDiv, const char * pVertRecTerm)
{
	Init();
	Flags = flags;
	FldDiv = pFldDiv;
	if(FldDiv.Cmp("\\t", 0) == 0 || FldDiv.Cmp("tab", 0) == 0)
		FldDiv.Z().Tab();
	if(FldDiv.Strip().Empty())
		FldDiv.Semicol();
	VertRecTerm = pVertRecTerm;
}

TextDbFile::Param::Param(long flags, int fldDivChr, const char * pVertRecTerm)
{
	Init();
	Flags = flags;
	FldDiv.CatChar(fldDivChr ? fldDivChr : ';');
	VertRecTerm = pVertRecTerm;
}

void TextDbFile::Param::Init()
{
	Ver = TEXTDBFILEPARAM_SVER;
	HdrLinesCount = 0;
	DateFormat = 0;
	TimeFormat = 0;
	RealFormat = 0;
	Flags = 0;
	VertRecTerm.Z();
	FldDiv.Z();
	FooterLine.Z();
	DefFileName.Z();
}

int TextDbFile::Param::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	THROW(pCtx->Serialize(dir, Ver, rBuf));
	if(dir < 0) {
		THROW_S_S(Ver == TEXTDBFILEPARAM_SVER, SLERR_INVSERIALIZEVER, "TextDbFile");
	}
	THROW(pCtx->Serialize(dir, HdrLinesCount, rBuf));
	THROW(pCtx->Serialize(dir, DateFormat, rBuf));
	THROW(pCtx->Serialize(dir, TimeFormat, rBuf));
	THROW(pCtx->Serialize(dir, RealFormat, rBuf));
	THROW(pCtx->Serialize(dir, Flags, rBuf));
	THROW(pCtx->Serialize(dir, VertRecTerm, rBuf));
	THROW(pCtx->Serialize(dir, FldDiv, rBuf));
	THROW(pCtx->Serialize(dir, FooterLine, rBuf));
	THROW(pCtx->Serialize(dir, DefFileName, rBuf));
	CATCHZOK
	return ok;
}

TextDbFile::TextDbFile() : State(0), EndPos(0), CurRec(-1)
{
	//@v10.3.12 (redundunt) P.Init();
}

TextDbFile::~TextDbFile()
{
	Close();
}

int TextDbFile::CheckParam(const SdRecord & rRec)
{
	if(P.Flags & fVerticalRec) {
		if(P.VertRecTerm.Empty())
			return (SLibError = SLERR_TXTDB_EMPTYVERTTERM, 0);
	}
	else {
		if(P.Flags & fFixedFields) {
			for(uint i = 0; i < rRec.GetCount(); i++)
				if(SFMTLEN(rRec.GetFieldOuterFormat(i)) == 0)
					return (SLibError = SLERR_TXTDB_ZEROLENFIXEDFLD, 0);
		}
		else if(P.FldDiv.Empty())
			return (SLibError = SLERR_TXTDB_EMPTYFLDDIV, 0);
	}
	return 1;
}

int TextDbFile::Open(const char * pFileName, const Param * pParam, int readOnly)
{
	int    ok = 1;
	F.Close();
	if(pParam) {
		P = *pParam;
		if(P.FldDiv.Cmp("\\t", 0) == 0 || P.FldDiv.Cmp("tab", 0) == 0)
			P.FldDiv.Z().Tab();
	}
	EndPos = 0;
	CurRec = -1;
	RecPosList.freeAll();
	if(!fileExists(pFileName)) {
		THROW(!readOnly);
		{
			SFile cr_f(pFileName, SFile::mWrite);
			THROW(cr_f.IsValid());
		}
	}
	const long mode = readOnly ? (SFile::mRead|SFile::mBinary|SFile::mNoStd) : SFile::mReadWrite; // @v10.4.1 SFile::mNoStd
	THROW(F.Open(pFileName, mode));
	THROW(Scan());
	SETFLAG(State, stReadOnly, readOnly);
	CATCHZOK
	return ok;
}

int TextDbFile::Close()
{
	if(!(State & stReadOnly) && F.IsValid()) {
		if(P.Flags & fVerticalRec && P.FooterLine.NotEmpty()) {
			SString line_buf;
			(line_buf = P.FooterLine).Strip().CR();
			F.WriteLine(line_buf);
		}
	}
	return F.Close();
}

int TextDbFile::ParseFieldNameRec(const SString & rLine)
{
	int    ok = 1;
	SString temp_buf;
	if(P.Flags & fVerticalRec) {
		temp_buf = rLine;
		if(temp_buf.NotEmptyS())
			FldNames.add(temp_buf.ToUpper());
		else
			FldNames.add(temp_buf.Space());
	}
	else {
		StringSet ss(P.FldDiv);
		ss.setBuf(rLine, rLine.Len()+1);
		for(uint p = 0; ss.get(&p, temp_buf);)
			if(temp_buf.NotEmptyS())
				FldNames.add(temp_buf.ToUpper());
			else
				FldNames.add(temp_buf.Space());
	}
	return ok;
}

int TextDbFile::IsTerminalLine(const SString & rLine, uint fldNo) const
{
	if(P.VertRecTerm.Empty() || P.VertRecTerm.Cmp("\\n", 0) == 0)
		return BIN(rLine.Empty());
	else {
		const uint num_flds = (P.VertRecTerm[0] == ':' && P.VertRecTerm.Last() == ':') ? atoi(P.VertRecTerm.cptr()+1) : 0;
		if(num_flds > 0 && num_flds < 1000)
			return (fldNo == num_flds) ? 2 : 0;
		else
			return BIN(rLine.CmpPrefix(P.VertRecTerm, 0) == 0);
	}
	/*
	return (P.VertRecTerm.Empty() || P.VertRecTerm.Cmp("\\n", 0) == 0) ?
		rLine.Empty() : (rLine.CmpPrefix(P.VertRecTerm, 0) == 0);
	*/
}

int TextDbFile::Scan()
{
	int    ok = 1;
	State &= ~stSignUtf8;
	if(F.IsValid()) {
		SString line;
		long   line_no = 0;
		F.Seek(0, SEEK_SET);
		int    is_terminator = 0; // Если !0, то предыдущая строка была терминатором вертикальной записи.
		int    is_first_rec = 1;
		uint   vert_fld_no = 0;
		{
			uint8 sign[8];
			size_t actual_size = 0;
			if(F.Read(sign, 4, &actual_size) && actual_size == 4) {
				if(sign[0] == 0xEF && sign[1] == 0xBB && sign[2] == 0xBF) {
					State |= stSignUtf8;
					F.Seek(3);
				}
				else {
					F.Seek(0);
				}
			}
		}
		int    pos = F.Tell();
		while(F.ReadLine(line)) {
			line_no++;
			if(line_no > P.HdrLinesCount) {
				line.Chomp().Strip();
				if(P.Flags & fVerticalRec) {
					if(is_terminator) {
						//
						// Предыдущая строка была терминатором.
						// Следовательно:
						THROW(RecPosList.insert(&pos)); // и мы можем зафиксировать позицию очередной записи
						is_terminator = 0;
					}
					is_terminator = IsTerminalLine(line, vert_fld_no);
					//
					// is_terminator == 2 означает неявный конец записи по количеству полей
					//
					if(is_terminator != 1) {
						if(is_first_rec) {
							if(P.Flags & fFldNameRec) {
								THROW(ParseFieldNameRec(line));
							}
							else {
								//
								// Первая строка (терминатора, естественно, до этого не было):
								if(RecPosList.getCount() == 0)
									THROW(RecPosList.insert(&pos)); // зафиксируем позицию первой записи
							}
						}
						else if(is_terminator == 2) {
							THROW(RecPosList.insert(&pos)); // и мы можем зафиксировать позицию очередной записи
							vert_fld_no = 0;
						}
						is_terminator = 0;
						vert_fld_no++;
					}
				}
				else {
					if(is_first_rec) {
						if(P.Flags & fFldNameRec) {
							THROW(ParseFieldNameRec(line));
						}
						else {
							if(line.NotEmpty()) { // @v10.7.9
								THROW(RecPosList.insert(&pos));
							}
						}
					}
					else {
						if(line.NotEmpty()) { // @v10.7.9
							THROW(RecPosList.insert(&pos));
						}
					}
				}
				is_first_rec = 0;
			}
			pos = F.Tell();
		}
		EndPos = pos;
	}
	CATCHZOK
	return ok;
}

ulong TextDbFile::GetNumRecords() const
{
	return RecPosList.getCount();
}

int TextDbFile::GoToRecord(ulong recNo, int rel)
{
	int    ok = -1;
	const  uint c = RecPosList.getCount();
	if(rel == relAbs) {
		if(recNo >= 0 && recNo < c) {
			F.Seek(RecPosList.at(recNo), SEEK_SET);
			CurRec = recNo;
			ok = 1;
		}
	}
	else if(rel == relFirst) {
		ok = GoToRecord(0, relAbs); // @recursion
	}
	else if(rel == relLast) {
		if(c)
			ok = GoToRecord(c-1, relAbs); // @recursion
	}
	else if(rel == relNext) {
		if(c && CurRec >= 0 && CurRec < static_cast<long>(c-1))
			ok = GoToRecord(CurRec+1, relAbs); // @recursion
	}
	else if(rel == relPrev) {
		if(CurRec > 0)
			ok = GoToRecord(CurRec-1, relAbs); // @recursion
	}
	return ok;
}

void TextDbFile::PutFieldDataToBuf(const SdbField & rFld, const SString & rTextData, void * pRecBuf)
{
	SFormatParam fp;
	fp.FDate = P.DateFormat;
	fp.FTime = P.TimeFormat;
	SETFLAG(fp.Flags, SFormatParam::fQuotText, P.Flags & fQuotText);
	rFld.PutFieldDataToBuf(rTextData, pRecBuf, fp);
}

void TextDbFile::GetFieldDataFromBuf(const SdbField & rFld, SString & rTextData, const void * pRecBuf)
{
	SFormatParam fp;
	fp.FDate = P.DateFormat;
	fp.FTime = P.TimeFormat;
	fp.FReal = P.RealFormat;
	if(!(P.Flags & fFixedFields))
		fp.Flags |= SFormatParam::fFloatSize;
	if(P.Flags & fQuotText)
		fp.Flags |= SFormatParam::fQuotText;
	rFld.GetFieldDataFromBuf(rTextData, pRecBuf, fp);
}

int TextDbFile::GetRecord(const SdRecord & rRec, void * pDataBuf)
{
	int    ok = -1;
	THROW(CheckParam(rRec));
	if(CurRec >= 0 && CurRec < static_cast<long>(RecPosList.getCount())) {
		SString line, field_buf, fn, fv;
		SdbField fld;
		STextEncodingStat tes;
		if(P.Flags & fVerticalRec) {
			F.Seek(RecPosList.at(CurRec), SEEK_SET);
			for(uint fld_pos = 0; F.ReadLine(line) && !IsTerminalLine(line.Chomp().Strip(), fld_pos); fld_pos++) {
				if(State & stSignUtf8) {
					line.Utf8ToChar();
				}
				else {
					tes.Init();
					tes.Add(line, line.Len());
					if(tes.CheckFlag(tes.fLegalUtf8Only))
						line.Utf8ToChar();
				}
				if(P.Flags & fFldEqVal) {
					if(line.Divide('=', fn, fv) > 0) {
						if(rRec.GetFieldByName(fn, &fld) > 0) {
							PutFieldDataToBuf(fld, fv, pDataBuf);
						}
					}
				}
				else {
					if(rRec.GetFieldByPos(fld_pos, &fld) > 0) {
						PutFieldDataToBuf(fld, line, pDataBuf);
					}
				}
			}
		}
		else {
			F.Seek(RecPosList.at(CurRec), SEEK_SET);
			THROW(F.ReadLine(line));
			line.Chomp();
			{
				tes.Init();
				tes.Add(line, line.Len());
				if(tes.CheckFlag(tes.fLegalUtf8Only))
					line.Utf8ToChar();
			}
			if(P.Flags & fFixedFields) {
				size_t offs = 0;
				for(uint fld_pos = 0; fld_pos < rRec.GetCount(); fld_pos++) {
					if(rRec.GetFieldByPos(fld_pos, &fld) > 0) {
						line.Sub(offs, SFMTLEN(fld.OuterFormat), field_buf);
						field_buf.Strip(); // @v5.3.4 Не уверен, что это правильный оператор.
							// В общем случае следует использовать поле со всеми пробелами, однако
							// случаи, когда лидирующие и хвостовые пробелы могут быть полезны
							// припомнить не смог, в то время как вред от них очевиден.
						offs += SFMTLEN(fld.OuterFormat);
						PutFieldDataToBuf(fld, field_buf, pDataBuf);
					}
				}
			}
			else if(P.FldDiv.NotEmpty()) {
				line.Strip();
				StringSet ss(P.FldDiv);
				ss.setBuf(line, line.Len() + 1);
				for(uint p = 0, fld_pos = 0, fn_pos = 0; ss.get(&p, field_buf) > 0; fld_pos++) {
					if(P.Flags & fFldNameRec) {
						if(FldNames.get(&fn_pos, fn) && rRec.GetFieldByName(fn, &fld) > 0) {
							PutFieldDataToBuf(fld, field_buf, pDataBuf);
						}
					}
					else if(rRec.GetFieldByPos(fld_pos, &fld) > 0) {
						PutFieldDataToBuf(fld, field_buf, pDataBuf);
					}
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int TextDbFile::AppendRecord(const SdRecord & rRec, const void * pDataBuf)
{
	int    ok = 1;
	uint   i;
	long   pos = 0;
	SdbField fld;
	SString line, field_buf;
	THROW(CheckParam(rRec));
	F.Seek(0, SEEK_END);
	pos = F.Tell();
	if(pos == 0) {
		//
		// Если файл пустой, то добавляем специфицированное количество пустых строк
		// и (если необходимо) запись, содержащую наименования полей.
		//
		for(i = 0; i < static_cast<uint>(P.HdrLinesCount); i++)
			THROW(F.WriteBlancLine());
		if(P.Flags & fFldNameRec) {
			line.Z();
			for(i = 0; i < rRec.GetCount(); i++) {
				THROW(rRec.GetFieldByPos(i, &fld));
				field_buf = fld.Name;
				if(!field_buf.NotEmptyS())
					field_buf.Cat(fld.ID);
				if(P.Flags & fVerticalRec) {
					F.WriteLine(field_buf.CR());
				}
				else {
					if(P.Flags & fFixedFields)
						_commfmt(fld.OuterFormat, field_buf);
					line.Cat(field_buf);
					if(!(P.Flags & fFixedFields) && i < rRec.GetCount()-1)
						line.Cat(P.FldDiv);
				}
			}
			if(P.Flags & fVerticalRec) {
				line = (P.VertRecTerm.Cmp("\\n", 0) == 0) ? static_cast<const char *>(0) : P.VertRecTerm;
			}
			THROW(F.WriteLine(line.CR()));
		}
		pos = F.Tell();
	}
	if(P.Flags & fVerticalRec) {
		for(i = 0; i < rRec.GetCount(); i++) {
			THROW(rRec.GetFieldByPos(i, &fld));
			GetFieldDataFromBuf(fld, field_buf.Z(), pDataBuf);
			if(P.Flags & fFldEqVal) {
				if(fld.Name.HasPrefixIAscii("empty"))
					line = field_buf;
				else
					line.Z().CatEq(fld.Name, field_buf);
			}
			else
				line = field_buf;
			THROW(F.WriteLine(line.CR()));
		}
		line = (P.VertRecTerm.Cmp("\\n", 0) == 0) ? static_cast<const char *>(0) : P.VertRecTerm;
	}
	else {
		line.Z();
		for(i = 0; i < rRec.GetCount(); i++) {
			THROW(rRec.GetFieldByPos(i, &fld));
			GetFieldDataFromBuf(fld, field_buf.Z(), pDataBuf);
			line.Cat(field_buf);
			if(!(P.Flags & fFixedFields) && i < rRec.GetCount()-1)
				line.Cat(P.FldDiv);
		}
	}
	THROW(F.WriteLine(line.CR()));
	RecPosList.insert(&pos);
	F.Flush(); // @v9.2.0
	CATCHZOK
	return ok;
}

int TextDbFile::AppendHeader(const SdRecord & rRec, const void * pDataBuf)
{
	int    ok = 1;
	uint   i;
	long   pos = 0;
	SdbField fld;
	SString line, field_buf;
	THROW(CheckParam(rRec));
	F.Seek(0, SEEK_END);
	pos = F.Tell();
	THROW_S_S(pos == 0 && !(State & stHeaderAdded), SLERR_TXTDB_MISSPLHEADER, F.GetName()); // @v7.4.1
	{
		//
		// Если файл пустой, то добавляем специфицированное количество пустых строк
		// и (если необходимо) запись, содержащую наименования полей.
		//
		for(i = 0; i < static_cast<uint>(P.HdrLinesCount); i++)
			THROW(F.WriteBlancLine());
		if(P.Flags & fFldNameRec) {
			line = 0;
			for(i = 0; i < rRec.GetCount(); i++) {
				THROW(rRec.GetFieldByPos(i, &fld));
				field_buf = fld.Name;
				if(!field_buf.NotEmptyS())
					field_buf.Cat(fld.ID);
				if(P.Flags & fVerticalRec) {
					F.WriteLine(field_buf.CR());
				}
				else {
					if(P.Flags & fFixedFields)
						_commfmt(fld.OuterFormat, field_buf);
					line.Cat(field_buf);
					if(!(P.Flags & fFixedFields) && i < rRec.GetCount()-1)
						line.Cat(P.FldDiv);
				}
			}
			if(P.Flags & fVerticalRec) {
				line = (P.VertRecTerm.Cmp("\\n", 0) == 0) ? static_cast<const char *>(0) : P.VertRecTerm;
			}
			THROW(F.WriteLine(line.CR()));
		}
		pos = F.Tell();
	}
	if(P.Flags & fVerticalRec) {
		for(i = 0; i < rRec.GetCount(); i++) {
			THROW(rRec.GetFieldByPos(i, &fld));
			GetFieldDataFromBuf(fld, field_buf.Z(), pDataBuf);
			if(P.Flags & fFldEqVal) {
				if(fld.Name.HasPrefixIAscii("empty"))
					line = field_buf;
				else
					line.Z().CatEq(fld.Name, field_buf);
			}
			else
				line = field_buf;
			THROW(F.WriteLine(line.CR()));
		}
	}
	State |= stHeaderAdded;
	CATCHZOK
	return ok;
}

const char * TextDbFile::GetFileName() const
{
	return F.GetName();
}
//
// TEST
//
#if 0 // {

#include <dbf.h>

static int CreateSdbRecFromDbfTable(const DbfTable * pTbl, SdRecord * pRec)
{
	int    ok = 1;
	pRec->Clear();
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
			pRec->AddField(&fld_id, &fld);
		}
	}
	return ok;
}

int TestTextDbFileReformat(SdRecord * pRec, const char * pSrcName, const char * pDestName,
	const TextDbFile::Param * pSrcP, const TextDbFile::Param * pDestP)
{
	int    ok = 1;
	TextDbFile src_file, dest_file;
	THROW(src_file.Open(pSrcName, pSrcP, 1));
	THROW(dest_file.Open(pDestName, pDestP, 0));
	if(src_file.GoToRecord(0, TextDbFile::relFirst) > 0) {
		do {
			THROW(src_file.GetRecord(*pRec, pRec->GetData()));
			THROW(dest_file.AppendRecord(*pRec, pRec->GetDataC()));
		} while(src_file.GoToRecord(0, TextDbFile::relNext) > 0);
	}
	CATCHZOK
	return ok;
}

int TestTextDbFile(const char * pInDbfFile)
{
	int    ok = 1;
	DbfTable db_tbl(pInDbfFile);
	char   file_name[MAXPATH];
	SdRecord rec;
	THROW(db_tbl.isOpened());
	THROW(CreateSdbRecFromDbfTable(&db_tbl, &rec));
	THROW(rec.AllocDataBuf());
	//
	//
	//
	{
		TextDbFile::Param p(TextDbFile::fFixedFields, ';', ";end");
		p.DateFormat = DATF_DMY;
		p.TimeFormat = TIMF_HMS;

		STRNSCPY(file_name, pInDbfFile);
		replaceExt(file_name, "W", 1);
		{
			TextDbFile f1;
			SFile::Remove(file_name);
			if(f1.Open(file_name, &p, 0)) {
				if(db_tbl.top()) {
					do {
						DbfRecord dbf_rec(&db_tbl);
						db_tbl.getRec(&dbf_rec);
						for(uint i = 1; i <= db_tbl.getNumFields(); i++) {
							THROW(dbf_rec.get(i, rec.GetFieldType(i-1), rec.GetData(i-1)));
						}
						THROW(f1.AppendRecord(rec, rec.GetDataC()));
					} while(db_tbl.next());
				}
			}
		}
		//
		// Reformating
		//
		{
			TextDbFile::Param dest_p(TextDbFile::fFldNameRec, '\t', ";end");
			dest_p.DateFormat  = DATF_DMY;
			dest_p.TimeFormat  = TIMF_HMS;
			char dest_file_name[MAXPATH];
			STRNSCPY(dest_file_name, file_name);
			SFile::Remove(replaceExt(dest_file_name, "R", 1));
			THROW(TestTextDbFileReformat(&rec, file_name, dest_file_name, &p, &dest_p));

			SFile::Remove(replaceExt(file_name, "W2", 1));
			THROW(TestTextDbFileReformat(&rec, dest_file_name, file_name, &dest_p, &p));
		}
	}
	CATCHZOK
	return ok;
}

#endif // } 0
