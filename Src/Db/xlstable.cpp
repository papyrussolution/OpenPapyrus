// XLSTABLE.CPP
// Copyright (c) A.Sobolev 2006, 2007, 2008, 2009, 2010, 2012, 2013, 2015, 2016, 2017, 2018
//
#include <slib.h>
#include <tv.h>
#pragma hdrstop
#include <db.h>
//
// @ModuleDef(ExcelDbFile)
//
ExcelDbFile::Param::Param(long flags /* = 0*/)
{
	Init();
	Flags = flags;
}

#define EXCELDBFILEPARAM_SVER 0

void ExcelDbFile::Param::Init()
{
	Ver = EXCELDBFILEPARAM_SVER; // serialize version
	HdrLinesCount = 0;
	ColumnsCount = 0;
	SheetNum = 0;
	DateFormat = 0;
	TimeFormat = 0;
	RealFormat = 0;
	Flags = 0;
	SheetName_ = 0;
	EndStr_ = 0;
}

int ExcelDbFile::Param::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	THROW(pCtx->Serialize(dir, Ver, rBuf));
	if(dir < 0) {
		THROW_S_S(Ver == EXCELDBFILEPARAM_SVER, SLERR_INVSERIALIZEVER, "ExcelDbFile");
	}
	THROW(pCtx->Serialize(dir, HdrLinesCount, rBuf));
	THROW(pCtx->Serialize(dir, ColumnsCount, rBuf));
	THROW(pCtx->Serialize(dir, SheetNum, rBuf));
	THROW(pCtx->Serialize(dir, DateFormat, rBuf));
	THROW(pCtx->Serialize(dir, TimeFormat, rBuf));
	THROW(pCtx->Serialize(dir, RealFormat, rBuf));
	THROW(pCtx->Serialize(dir, Flags, rBuf));
	THROW(pCtx->Serialize(dir, SheetName_, rBuf));
	THROW(pCtx->Serialize(dir, EndStr_, rBuf));
	CATCHZOK
	return ok;
}

ExcelDbFile::ExcelDbFile()
{
	MEMSZERO(P);
	P_WkBook = 0;
	P_Sheet = 0;
	P_App = 0;
	P_Sheets = 0;
	ReadOnly = 0;
	CurRec = -1;
}

ExcelDbFile::~ExcelDbFile()
{
	Close();
}

int ExcelDbFile::CheckParam(const SdRecord & rRec)
{
	for(uint i = 0; i < rRec.GetCount(); i++)
		if(SFMTLEN(rRec.GetFieldOuterFormat(i)) == 0)
			return (SLibError = SLERR_TXTDB_ZEROLENFIXEDFLD, 0);
	return 1;
}

#define MAX_COLUMN 100L
#define MAX_ROW    65000L

int ExcelDbFile::Open(const char * pFileName, const Param * pParam, int readOnly)
{
	int    ok = 1;
	Close();
	FileName = pFileName;
	RVALUEPTR(P, pParam);
	P.SheetNum = MAX(P.SheetNum, 1);
	THROW(P_App = new ComExcelApp);
	THROW(P_App->Init() > 0);
	P_App->_DisplayAlerts(0);
	if((P_WkBook = P_App->OpenWkBook(FileName)) == 0) {
		THROW(!readOnly);
		THROW(P_WkBook = P_App->AddWkbook());
	}
	THROW(P_Sheets = P_WkBook->Get());
	if(!readOnly) {
		long   sheets_count = P_Sheets->GetCount();
		SString name;
		while(sheets_count < P.SheetNum) {
			(name = "Sheet").Cat(++sheets_count); // @v7.6.12 "Лист"-->"Sheet"
			THROW(P_Sheets->_Add(0, 0, name));
		}
		THROW(P_Sheet = P_Sheets->Get(P.SheetNum));
		if(P.SheetName_.NotEmpty()) {
			(name = P.SheetName_).Transf(CTRANSF_INNER_TO_OUTER);
			THROW(P_Sheet->SetName(name));
		}
		THROW(P_WkBook->_SaveAs(FileName));
	}
	else {
		long   idx = 1;
		SString name;
		ComExcelWorksheet * p_sheet = 0;
		while(!P_Sheet && (p_sheet = P_Sheets->Enum(&idx))) {
			if(p_sheet->GetName(name) && (name.ToOem()).Cmp(P.SheetName_, 0) == 0 && name.Len() == P.SheetName_.Len())
				P_Sheet = p_sheet;
			else
				ZDELETE(p_sheet);
		}
		if(!P_Sheet)
			P_Sheet = P_Sheets->Get(P.SheetNum);
	}
	THROW(P_Sheet);
	CurRec   = -1;
	RecCount = 0;
	ReadOnly = BIN(readOnly);
	THROW(Scan());
	CATCHZOK
	return ok;
}

int ExcelDbFile::Close()
{
	int    ok = 1;
	if(P_WkBook) {
		if(!ReadOnly) {
			if(P_Sheet) {
				for(long i = 0; i < WidthList.getCountI(); i++)
					THROW(P_Sheet->SetColumnWidth(1 + i + P.ColumnsCount, WidthList.at(i)) > 0);
			}
			P_WkBook->_SaveAs(FileName);
		}
		P_WkBook->_Close();
	}
	CATCHZOK
	ZDELETE(P_Sheet);
	ZDELETE(P_Sheets);
	ZDELETE(P_WkBook);
	ZDELETE(P_App);
	WidthList.freeAll();
	return ok;
}

int ExcelDbFile::GetFldNames()
{
	const  int max_items = 100;

	int    stop = 0;
	int    is_vert = BIN(P.Flags & fVerticalRec);
	long   col = 1 + P.ColumnsCount;
	long   row = 1 + P.HdrLinesCount;
	SString temp_buf;
	FldNames.clear();
	for(int i = 1; i <= max_items; i++) {
		if(P_Sheet->GetValue(row, col, temp_buf.Z()) > 0 && temp_buf.NotEmptyS())
			FldNames.add(temp_buf.ToUpper());
		else if(is_vert)
			FldNames.add(temp_buf.Z().CatChar('$').Cat(row));
		else
			FldNames.add(temp_buf.Z().CatChar('$').Cat(col));
		if(is_vert)
			row++;
		else
			col++;
	}
	return 1;
}

int ExcelDbFile::Scan()
{
	const int max_empty = 5; // Максимальное количество пустых ячеек, просмотр после которых считается избыточным.
		// То есть, после max_empty пустых ячеек функция считает, что дальше ничего интересного нет.
	const int is_vert = BIN(P.Flags & fVerticalRec);
	const int end_str_decl = BIN(P.EndStr_.NotEmpty());
	int    ok = 1, stop = 0;
	long   max_col = 0, max_row = 0;
	long   row = 1 + P.HdrLinesCount;
	long   col = 1 + P.ColumnsCount;
	SString temp_buf, end_str;
	if(P.Flags & fFldNameRec)
		GetFldNames();
	if(!FldNames.getCount()) {
		while(!stop) {
			temp_buf.Z();
			if(P_Sheet->GetValue(row, col, temp_buf) > 0 && temp_buf.NotEmptyS()) {
				max_row = is_vert ? row : 0;
				max_col = is_vert ? 0 : col;
			}
			row = is_vert ? row + 1 : row;
			col = is_vert ? col : col + 1;
			stop = is_vert ? BIN(row >= MAX_COLUMN) : BIN(col >= MAX_COLUMN);
			if(is_vert && stop == 0)
				stop = BIN(temp_buf.Empty());
		}
	}
	else {
		max_col = is_vert ? 0 : FldNames.getCount();
		max_row = is_vert ? FldNames.getCount() : 0;
	}
	stop = 0;
	row = is_vert ? (1 + P.HdrLinesCount) : (1 + P.HdrLinesCount + BIN(P.Flags & fFldNameRec));
	col = is_vert ? (1 + P.ColumnsCount + BIN(P.Flags & fFldNameRec)) : (1 + P.ColumnsCount);
	(end_str = P.EndStr_);
	while(!stop) {
		int    found_data = 0, end_rec = 0;
		int    empty_count = 0;
		row = is_vert ? 1 + P.HdrLinesCount : row;
		col = is_vert ? col : 1 + P.ColumnsCount;
		while(/*!found_data && */!end_rec) {
			P_Sheet->GetValue(row, col, temp_buf.Z());
			const  int is_empty = BIN(!temp_buf.NotEmpty());
			if(!is_empty || end_str_decl)
				empty_count = 0;
			else
				empty_count++;
			if(empty_count < max_empty) {
				temp_buf.Strip().ToOem();
				if(end_str_decl && temp_buf.CmpNC(end_str) == 0) {
					if(is_vert) {
						if(found_data)
							max_col--;
					}
					else if(found_data)
						max_row--;
					found_data = 0;
					end_rec = 1;
				}
				else if(end_str_decl || !is_empty) {
					found_data = 1;
					max_col = is_vert ? col : max_col;
					max_row = is_vert ? max_row : row;
				}
				row = is_vert ? row + 1 : row;
				col = is_vert ? col : col + 1;
				if(!end_rec)
					end_rec = is_vert ? BIN(row >= max_row) : BIN(col >= max_col);
			}
			else
				end_rec = 1;
		}
		row = is_vert ? row : row + 1;
		col = is_vert ? col + 1 : col;
		if(!found_data)
			stop = 1;
		else
			stop = is_vert ? BIN(col >= MAX_ROW) : BIN(row >= MAX_ROW);
	}
	RecCount = is_vert ? (max_col - P.ColumnsCount) : (max_row - P.HdrLinesCount);
	RecCount -= BIN(FldNames.getCount());
	if(P.Flags & fOneRecPerFile)
		RecCount = BIN(RecCount > 0);
	if(RecCount <= 0)
		ok = -1;
	return ok;
}

/*
int ExcelDbFile::Scan()
{
	int ok = 1, is_vert = BIN(P.Flags & fVerticalRec);
	long max_col = 0, max_row = 0, row = (is_vert) ? 1 + P.HdrLinesCount : 1, col = (is_vert) ? 1 + P.HdrLinesCount : 1;
	SString temp_buf;
	if(P.Flags & fFldNameRec) {
		GetFldNames();
	 	row++;
	}
	if(P.Flags & fOneRecPerFile)
		RecCount = 1;
	else {
		if(!FldNames.getCount()) {
			for(col; col < MAX_COLUMN; col++)
				if(P_Sheet->GetValue((is_vert) ? col : row, (is_vert) ? row : col, temp_buf) > 0 && temp_buf.Strip().Len())
					max_col = col;
		}
		else {
			max_col = (is_vert) ? P.HdrLinesCount + FldNames.getCount() : FldNames.getCount();
			SString end_str;
			end_str = P.EndStr;
			for(row; row < MAX_ROW; row++) {
				int found_data = 0;
				for(col = (is_vert) ? 1 + P.HdrLinesCount : 1; !found_data && col < max_col; col++)
					if(P_Sheet->GetValue((is_vert) ? col : row, (is_vert) ? row : col, temp_buf.Z()) > 0 && temp_buf.Strip().Len() > 0 && (end_str.Len() == 0 || temp_buf.Cmp(end_str, 0) != 0)) {
						max_row = row;
						found_data = 1;
					}
				if(!found_data)
					break;
			}
		}
		RecCount = max_row - BIN(FldNames.getCount());
		if(!is_vert)
			RecCount -= P.HdrLinesCount;
	}
	return ok;
}
*/

ulong ExcelDbFile::GetNumRecords() const
{
	return RecCount;
}

int ExcelDbFile::GoToRecord(ulong recNo, int rel)
{
	int    ok = -1;
	if((uint)rel == relAbs) {
		if(recNo >= 0 && recNo < (ulong)RecCount) {
			CurRec = recNo;
			ok = 1;
		}
	}
	else if(rel == relFirst)
		ok = GoToRecord(0, relAbs); // @recursion
	else if(rel == relLast)
		ok = GoToRecord(RecCount, relAbs); // @recursion
	else if(rel == relNext) {
		if(CurRec >= 0 && CurRec < RecCount - 1)
			ok = GoToRecord(CurRec + 1, relAbs); // @recursion
	}
	else if(rel == relPrev) {
		if(CurRec > 0)
			ok = GoToRecord(CurRec - 1, relAbs); // @recursion
	}
	return ok;
}

void ExcelDbFile::PutFieldDataToBuf(const SdbField & rFld, const SString & rTextData, void * pRecBuf)
{
	SFormatParam fp;
	fp.FDate = P.DateFormat;
	fp.FTime = P.TimeFormat;
	SETFLAG(fp.Flags, SFormatParam::fQuotText, P.Flags & fQuotText);
	rFld.PutFieldDataToBuf(rTextData, pRecBuf, fp);
}

int ExcelDbFile::GetFieldDataFromBuf(const SdbField & rFld, SString & rTextData, const void * pRecBuf)
{
	SFormatParam fp;
	fp.FDate = P.DateFormat;
	fp.FTime = P.TimeFormat;
	fp.FReal = P.RealFormat;
	if(P.Flags & fQuotText)
		fp.Flags |= SFormatParam::fQuotText;
	return rFld.GetFieldDataFromBuf(rTextData, pRecBuf, fp);
}

int ExcelDbFile::GetRecord(const SdRecord & rRec, void * pDataBuf)
{
	int    ok = -1;
	THROW(CheckParam(rRec));
	if(CurRec >= 0 && CurRec < (long)RecCount) {
		const  int    is_vert = BIN(P.Flags & fVerticalRec);
		long   cur_rec = 1 + CurRec + (is_vert ? P.ColumnsCount : P.HdrLinesCount);
		long   row = 0, col = 0;
		SString temp_buf, str_row_no, str_col_no;
		SString field_buf;
		SdbField fld;
		if(FldNames.getCount() == 0) {
			const char * p_celln = "cell";
			for(uint fld_pos = 0; fld_pos < rRec.GetCount(); fld_pos++) {
				if(rRec.GetFieldByPos(fld_pos, &fld) > 0) {
					row = col = 0;
					if(fld.Name.CmpPrefix(p_celln, 1) == 0) {
						(temp_buf = fld.Name).ShiftLeft(sstrlen(p_celln));
						temp_buf.Divide('_', str_row_no, str_col_no);
						row = str_row_no.ToLong();
						col = str_col_no.ToLong();
					}
					if(row == 0)
						row = is_vert ? (1 + fld_pos + P.HdrLinesCount) : cur_rec;
					if(col == 0)
						col = is_vert ? cur_rec : (1 + fld_pos + P.ColumnsCount);
					THROW(P_Sheet->GetValue(row, col, field_buf));
					PutFieldDataToBuf(fld, field_buf.Strip(), pDataBuf);
				}
			}
		}
		else {
			//SString fn;
			cur_rec++; // Пропустим наименования столбцов/строк
			for(uint p = 0, fld_pos = 0; FldNames.get(&p, temp_buf) > 0; fld_pos++) {
				if(rRec.GetFieldByName(temp_buf, &fld) > 0) {
					row = (is_vert) ? 1 + fld_pos + P.HdrLinesCount : cur_rec;
					col = (is_vert) ? cur_rec : 1 + fld_pos + P.ColumnsCount;
					THROW(P_Sheet->GetValue(row, col, field_buf.Z()));
					PutFieldDataToBuf(fld, field_buf.Strip(), pDataBuf);
				}
			}
		}
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int ExcelDbFile::AppendRecord(const SdRecord & rRec, const void * pDataBuf)
{
	int    ok = 1;
	uint   i = 0;
	const  int is_vert = BIN(P.Flags & fVerticalRec);
	long   cur_rec = (CurRec >= 0) ? CurRec : 0;
	SdbField fld;
	SString  field_buf;
	THROW(CheckParam(rRec));
	cur_rec += 1 + BIN(P.Flags & fFldNameRec);
	if(!is_vert)
		 cur_rec += P.HdrLinesCount;
	else
		cur_rec += P.ColumnsCount;
	if(CurRec <= 0) {
		THROW(P_Sheet->_Clear(1, MAX_COLUMN));
		//
		// Если файл пустой, то добавляем специфицированное количество пустых строк
		// и (если необходимо) запись, содержащую наименования полей.
		//
		CurRec = 0;
		if(P.Flags & fFldNameRec) {
			for(i = 0; i < rRec.GetCount(); i++) {
				THROW(rRec.GetFieldByPos(i, &fld));
				field_buf = fld.Name;
				if(!field_buf.NotEmptyS())
					field_buf.Cat(fld.ID);
				_commfmt(fld.OuterFormat, field_buf);
				const long row = (is_vert) ? 1 + i + P.HdrLinesCount : cur_rec - 1;
				const long col = (is_vert) ? cur_rec - 1 : 1 + i + P.ColumnsCount;
				THROW(P_Sheet->SetValue(row, col, field_buf.Transf(CTRANSF_INNER_TO_OUTER).Strip().cptr()) > 0);
				THROW(P_Sheet->SetBold(row, col, 1));
				if(WidthList.getCountI() < (col - P.ColumnsCount))
					WidthList.add(field_buf.Len());
				if(WidthList.at(col - 1 - P.ColumnsCount) < (long)field_buf.Len())
		 			WidthList.at(col - 1 - P.ColumnsCount) = (long)field_buf.Len();
			}
		}
	}
	for(i = 0; i < rRec.GetCount(); i++) {
		THROW(rRec.GetFieldByPos(i, &fld));
		const long row = (is_vert) ? 1 + i + P.HdrLinesCount : cur_rec;
		const long col = (is_vert) ? cur_rec : 1 + i + P.ColumnsCount;
		{
			const  TYPEID st = fld.T.GetDbFieldType();
			int    base_type = stbase(st);
			const  void * p_fld_data = PTR8C(pDataBuf)+fld.InnerOffs;
			if(base_type == BTS_REAL) {
				double real_val = 0;
				sttobase(st, p_fld_data, &real_val);
				THROW(P_Sheet->SetValue(row, col, real_val) > 0);
			}
			else {
				THROW(GetFieldDataFromBuf(fld, field_buf.Z(), pDataBuf));
				THROW(P_Sheet->SetValue(row, col, field_buf.Strip().cptr()) > 0);
			}
		}
		if(WidthList.getCount() < (uint)(col - P.ColumnsCount))
			WidthList.add(field_buf.Len());
		if(WidthList.at(col - 1 - P.ColumnsCount) < (long)field_buf.Len())
 			WidthList.at(col - 1 - P.ColumnsCount) = (long)field_buf.Len();
	}
	CurRec++;
	CATCHZOK
	return ok;
}
