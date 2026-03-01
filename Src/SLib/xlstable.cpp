// XLSTABLE.CPP
// Copyright (c) A.Sobolev 2006, 2007, 2008, 2009, 2010, 2012, 2013, 2015, 2016, 2017, 2018, 2020, 2021, 2022, 2023, 2024, 2025, 2026
// @codepage UTF-8
//
#include <slib-internal.h>
#pragma hdrstop
#include <..\osf\OpenXLSX\SRC\headers\OpenXLSX.hpp> // @v12.5.3
//
// @ModuleDef(ExcelDbFile)
//
#define MAX_COLUMN              100L
#define MAX_ROW               65000L
#define EXCELDBFILEPARAM_SVER     0

ExcelIoParam::ExcelIoParam(long flags/*=0*/)
{
	Init();
	Flags = flags;
}

void ExcelIoParam::Init()
{
	Ver = EXCELDBFILEPARAM_SVER; // serialize version
	HdrLinesCount = 0;
	ColumnsCount = 0;
	SheetNum = 0;
	DateFormat = 0;
	TimeFormat = 0;
	RealFormat = 0;
	Flags = 0;
	SheetName_.Z();
	EndStr_.Z();
}

int ExcelIoParam::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx)
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

ExcelDbFile::ExcelDbFile() : P_WkBook(0), P_Sheet(0), P_App(0), P_Sheets(0), ReadOnly(0), CurRec(-1)
{
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

int ExcelDbFile::Open(const char * pFileName, const ExcelIoParam * pParam, int readOnly)
{
	int    ok = 1;
	Close();
	FileName = pFileName;
	// @v11.6.5 {
	if(FileName.IsLegalUtf8()) {
		FileName.Transf(CTRANSF_UTF8_TO_OUTER);
	}
	// } @v11.6.5 
	RVALUEPTR(P, pParam);
	P.SheetNum = smax(P.SheetNum, 1L);
	THROW(P_App = new ComExcelApp);
	THROW(P_App->Init() > 0);
	P_App->_DisplayAlerts(0);
	P_WkBook = P_App->OpenWkBook(FileName);
	if(!P_WkBook) {
		THROW(!readOnly);
		THROW(P_WkBook = P_App->AddWkbook());
	}
	THROW(P_Sheets = P_WkBook->Get());
	if(!readOnly) {
		long   sheets_count = P_Sheets->GetCount();
		SString name;
		while(sheets_count < P.SheetNum) {
			(name = "Sheet").Cat(++sheets_count);
			THROW(P_Sheets->_Add(0, 0, name));
		}
		THROW(P_Sheet = P_Sheets->Get(P.SheetNum));
		if(P.SheetName_.NotEmpty()) {
			(name = P.SheetName_).Transf(CTRANSF_INNER_TO_OUTER);
			THROW(P_Sheet->SetName(name));
		}
		THROW_S_S(P_WkBook->_SaveAs(FileName), SLERR_EXCL_SAVEFAULT, FileName);
	}
	else {
		long   idx = 1;
		SString name;
		ComExcelWorksheet * p_sheet = 0;
		while(!P_Sheet && (p_sheet = P_Sheets->Enum(&idx))) {
			// @v12.5.7 {
			if(p_sheet->GetName(name)) {
				name.Transf(CTRANSF_OUTER_TO_INNER);
				if(name.IsEqNC(P.SheetName_)) {
					P_Sheet = p_sheet;
				}
			}
			if(!P_Sheet) {
				ZDELETE(p_sheet);
			}
			// } @v12.5.7 
			/* @v12.5.7 if(p_sheet->GetName(name) && (name.ToOem()).Cmp(P.SheetName_, 0) == 0 && name.Len() == P.SheetName_.Len())
				P_Sheet = p_sheet;
			else
				ZDELETE(p_sheet);*/
		}
		SETIFZQ(P_Sheet, P_Sheets->Get(P.SheetNum));
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
	const  bool is_vert = LOGIC(P.Flags & ExcelIoParam::fVerticalRec);
	int    stop = 0;
	long   col = P.ColumnsCount + 1;
	long   row = P.HdrLinesCount + 1;
	SString temp_buf;
	FldNames.Z();
	for(int i = 1; i <= max_items; i++) {
		if(P_Sheet->GetValue(row, col, temp_buf.Z()) > 0 && temp_buf.NotEmptyS())
			FldNames.add(temp_buf.ToUpper1251());
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
	const  bool is_vert = LOGIC(P.Flags & ExcelIoParam::fVerticalRec);
	const  bool end_str_decl = P.EndStr_.NotEmpty();
	int    ok = 1;
	bool   stop = false;
	long   max_col = 0;
	long   max_row = 0;
	long   row = 1 + P.HdrLinesCount;
	long   col = 1 + P.ColumnsCount;
	SString temp_buf, end_str;
	if(P.Flags & ExcelIoParam::fFldNameRec)
		GetFldNames();
	if(!FldNames.getCount()) {
		while(!stop) {
			temp_buf.Z();
			if(P_Sheet->GetValue(row, col, temp_buf) > 0 && temp_buf.NotEmptyS()) {
				max_row = is_vert ? 0 : row;
				max_col = is_vert ? col : 0;
			}
			if(is_vert) {
				col++;
				stop = (col >= MAX_COLUMN);
			}
			else {
				row++;
				stop = (row >= MAX_COLUMN);
			}
			if(is_vert && !stop)
				stop = temp_buf.IsEmpty();
		}
	}
	else {
		max_col = is_vert ? 0 : FldNames.getCount();
		max_row = is_vert ? FldNames.getCount() : 0;
	}
	stop = false;
	row = is_vert ? (1 + P.HdrLinesCount) : (1 + P.HdrLinesCount + BIN(P.Flags & ExcelIoParam::fFldNameRec));
	col = is_vert ? (1 + P.ColumnsCount + BIN(P.Flags & ExcelIoParam::fFldNameRec)) : (1 + P.ColumnsCount);
	(end_str = P.EndStr_);
	while(!stop) {
		int    found_data = 0;
		int    end_rec = 0;
		int    empty_count = 0;
		row = is_vert ? 1 + P.HdrLinesCount : row;
		col = is_vert ? col : 1 + P.ColumnsCount;
		while(/*!found_data && */!end_rec) {
			P_Sheet->GetValue(row, col, temp_buf.Z());
			const  bool is_empty = temp_buf.IsEmpty();
			if(!is_empty || end_str_decl)
				empty_count = 0;
			else
				empty_count++;
			if(empty_count < max_empty) {
				temp_buf.Strip().Transf(CTRANSF_OUTER_TO_INNER); // @v12.5.7 ToOem()-->Transf(CTRANSF_OUTER_TO_INNER)
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
			stop = true;
		else
			stop = is_vert ? (col >= MAX_ROW) : (row >= MAX_ROW);
	}
	RecCount = is_vert ? (max_col - P.ColumnsCount) : (max_row - P.HdrLinesCount);
	RecCount -= BIN(FldNames.getCount());
	if(P.Flags & ExcelIoParam::fOneRecPerFile)
		RecCount = BIN(RecCount > 0);
	if(RecCount <= 0)
		ok = -1;
	return ok;
}

ulong ExcelDbFile::GetNumRecords() const { return RecCount; }

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
	ExcelDbFile::PutFieldDataToBuf_(P, rFld, rTextData, pRecBuf);
}

void ExcelDbFile::GetFieldDataFromBuf(const SdbField & rFld, SString & rTextData, const void * pRecBuf)
{
	ExcelDbFile::GetFieldDataFromBuf_(P, rFld, rTextData, pRecBuf);
}

/*static*/void ExcelDbFile::PutFieldDataToBuf_(const ExcelIoParam & rP, const SdbField & rFld, const SString & rTextData, void * pRecBuf)
{
	SFormatParam fp;
	fp.FDate = rP.DateFormat;
	fp.FTime = rP.TimeFormat;
	SETFLAG(fp.Flags, SFormatParam::fQuotText, rP.Flags & ExcelIoParam::fQuotText);
	rFld.PutFieldDataToBuf(rTextData, pRecBuf, fp);
}

/*static*/void ExcelDbFile::GetFieldDataFromBuf_(const ExcelIoParam & rP, const SdbField & rFld, SString & rTextData, const void * pRecBuf)
{
	SFormatParam fp;
	fp.FDate = rP.DateFormat;
	fp.FTime = rP.TimeFormat;
	fp.FReal = rP.RealFormat;
	if(rP.Flags & ExcelIoParam::fQuotText)
		fp.Flags |= SFormatParam::fQuotText;
	rFld.GetFieldDataFromBuf(rTextData, pRecBuf, fp);
}

int ExcelDbFile::GetRecord(const SdRecord & rRec, void * pDataBuf)
{
	int    ok = -1;
	THROW(CheckParam(rRec));
	if(CurRec >= 0 && CurRec < (long)RecCount) {
		const  bool is_vert = LOGIC(P.Flags & ExcelIoParam::fVerticalRec);
		long   cur_rec = 1 + CurRec + (is_vert ? P.ColumnsCount : P.HdrLinesCount);
		long   row = 0;
		long   col = 0;
		SString temp_buf;
		SString str_row_no;
		SString str_col_no;
		SString field_buf;
		SdbField fld;
		if(FldNames.getCount() == 0) {
			const char * p_celln = "cell";
			for(uint fld_pos = 0; fld_pos < rRec.GetCount(); fld_pos++) {
				if(rRec.GetFieldByPos(fld_pos, &fld) > 0) {
					row = col = 0;
					if(fld.Name.HasPrefixIAscii(p_celln)) {
						(temp_buf = fld.Name).ShiftLeft(sstrlen(p_celln));
						temp_buf.Divide('_', str_row_no, str_col_no);
						row = str_row_no.ToLong();
						col = str_col_no.ToLong();
					}
					SETIFZ(row, is_vert ? (1 + fld_pos + P.HdrLinesCount) : cur_rec);
					SETIFZ(col, is_vert ? cur_rec : (1 + fld_pos + P.ColumnsCount));
					THROW(P_Sheet->GetValue(row, col, field_buf.Z()));
					PutFieldDataToBuf(fld, field_buf.Strip(), pDataBuf);
				}
			}
		}
		else {
			cur_rec++; // Пропустим наименования столбцов/строк
			for(uint p = 0, fld_pos = 0; FldNames.get(&p, temp_buf); fld_pos++) {
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
	const  bool is_vert = LOGIC(P.Flags & ExcelIoParam::fVerticalRec);
	long   cur_rec = (CurRec >= 0) ? CurRec : 0;
	SdbField fld;
	SString  field_buf;
	THROW(CheckParam(rRec));
	cur_rec += (1 + BIN(P.Flags & ExcelIoParam::fFldNameRec));
	if(!is_vert)
		cur_rec += P.HdrLinesCount;
	else
		cur_rec += P.ColumnsCount;
	if(CurRec <= 0) {
		P_Sheet->_Clear(1, MAX_COLUMN);
		//
		// Если файл пустой, то добавляем специфицированное количество пустых строк
		// и (если необходимо) запись, содержащую наименования полей.
		//
		CurRec = 0;
		if(P.Flags & ExcelIoParam::fFldNameRec) {
			for(i = 0; i < rRec.GetCount(); i++) {
				THROW(rRec.GetFieldByPos(i, &fld));
				field_buf = fld.Name;
				if(!field_buf.NotEmptyS())
					field_buf.Cat(fld.ID);
				_commfmt(fld.OuterFormat, field_buf);
				const long row = is_vert ? (1 + i + P.HdrLinesCount) : (cur_rec - 1);
				const long col = is_vert ? (cur_rec - 1) : (1 + i + P.ColumnsCount);
				THROW(P_Sheet->SetValue(row, col, field_buf.Strip().cptr()) > 0);
				THROW(P_Sheet->SetBold(row, col, 1));
				if(WidthList.getCountI() < (col - P.ColumnsCount))
					WidthList.add(field_buf.LenI());
				if(WidthList.at(col - 1 - P.ColumnsCount) < field_buf.LenI())
		 			WidthList.at(col - 1 - P.ColumnsCount) = field_buf.LenI();
			}
		}
	}
	for(i = 0; i < rRec.GetCount(); i++) {
		THROW(rRec.GetFieldByPos(i, &fld));
		const long row = is_vert ? (1 + i + P.HdrLinesCount) : cur_rec;
		const long col = is_vert ? cur_rec : (1 + i + P.ColumnsCount);
		{
			const  TYPEID st = fld.T.GetDbFieldType();
			int    base_type = stbase(st);
			const  void * p_fld_data = PTR8C(pDataBuf)+fld.InnerOffs;
			if(base_type == BTS_REAL) {
				double real_val = 0.0;
				sttobase(st, p_fld_data, &real_val);
				THROW(P_Sheet->SetValue(row, col, real_val) > 0);
			}
			else {
				GetFieldDataFromBuf(fld, field_buf.Z(), pDataBuf);
				THROW(P_Sheet->SetValue(row, col, field_buf.Strip().cptr()) > 0);
			}
		}
		if(WidthList.getCount() < static_cast<uint>(col - P.ColumnsCount))
			WidthList.add(field_buf.LenI());
		if(WidthList.at(col - 1 - P.ColumnsCount) < field_buf.LenI())
 			WidthList.at(col - 1 - P.ColumnsCount) = field_buf.LenI();
	}
	CurRec++;
	CATCHZOK
	return ok;
}
//
//
//
class SWorksheetData { // @construction
public:
	static constexpr uint MaxColumnIdx = 0x00004000U; // [1..MaxColumnIdx]
	static constexpr uint MaxRowIdx    = 0x00100000U; // [1..MaxRowIdx]
	struct CellRef {
		// 20 bit - row, 16 bit - column, 12 bit - type (BTS_XXX), 16 bit - format ref
		CellRef() : V(0)
		{
		}
		static bool IsValidType(uint type) 
		{
			return oneof8(type, BTS_VOID, BTS_INT, BTS_REAL, BTS_DATE, BTS_TIME, BTS_DATETIME, BTS_STRING, BTS_BOOL);
		}
		bool   Compose(uint rowIdx, uint colIdx, uint type)
		{
			bool   ok = false;
			if((colIdx > 0 && colIdx <= MaxColumnIdx) && (rowIdx > 0 && rowIdx <= MaxRowIdx) && IsValidType(type)) {
				V = static_cast<uint64>((rowIdx-1) << 28) | static_cast<uint64>((colIdx-1) << 12) | static_cast<uint64>(type);
			}
			return ok;
		}
		uint   GetCol() const { return static_cast<uint>((V & 0x00000ffff000U) >> 12) + 1; }
		uint   GetRow() const { return static_cast<uint>((V & 0xfffff0000000U) >> 28) + 1; }
		uint   GetType() const { return static_cast<uint>(V & 0x000000000fffU); }
		bool   SetCol(uint colIdx/*[1..MaxColumnIdx]*/)
		{
			bool   ok = false;
			if(colIdx > 0 && colIdx <= MaxColumnIdx) {
				V |= static_cast<uint64>((colIdx-1) << 12);
				ok = true;
			}
			return ok;
		}
		bool   SetRow(uint rowIdx/*[1..MaxRowIdx]*/)
		{
			bool   ok = false;
			if(rowIdx > 0 && rowIdx <= MaxRowIdx) {
				V |= static_cast<uint64>((rowIdx-1) << 28);
				ok = true;
			}
			return ok;
		}
		bool   SetType(uint type)
		{
			bool   ok = false;
			if(IsValidType(type)) {
				V |= type;
				ok = true;
			}
			return ok;
		}
		uint64 V;
	};
	static DECL_CMPFUNC(CellRef_RowCol)
	{
		const CellRef * i1 = static_cast<const CellRef *>(p1);
		const CellRef * i2 = static_cast<const CellRef *>(p2);
		RET_CMPCASCADE2(i1, i2, GetRow(), GetCol());
	}
	struct Entry {
		CellRef Cr;
		uint64 DataP;
	};

	SWorksheetData()
	{
		DataBuf.Init();
	}
	~SWorksheetData()
	{
		DataBuf.Destroy();
	}
	int    PutCell(uint row, uint col, uint type, const void * pData)
	{
		int    ok = 0;
		Entry * p_ex_cell = SearchCell(row, col, 0);
		if(p_ex_cell) {
			
		}
		else {
			
		}
		return ok;
	}
private:
	Entry * SearchCell(uint row, uint col, uint * pPos)
	{
		Entry * p_result = 0;
		CellRef key;
		uint   pos = 0;
		if(key.SetRow(row) && key.SetCol(col)) {
			if(L.lsearch(&key, &pos, PTR_CMPFUNC(CellRef_RowCol))) {
				p_result = &L.at(pos);
			}
		}
		ASSIGN_PTR(pPos, pos);
		return p_result;
	}

	TSVector <Entry> L;
	SBaseBuffer DataBuf;
};
//
//
//
ImpExpExcelWorkbook::ImpExpExcelWorkbook() : P_Doc(0), P_Sheet(0), State(0), CurRec(_FFFF32), RecCount(0), RegFontIdx(_FFFF32), HdrFontIdx(_FFFF32)
{
}

ImpExpExcelWorkbook::~ImpExpExcelWorkbook()
{
	Close();
}

int ImpExpExcelWorkbook::CheckParam(const SdRecord & rRec)
{
	for(uint i = 0; i < rRec.GetCount(); i++)
		if(SFMTLEN(rRec.GetFieldOuterFormat(i)) == 0)
			return (SLibError = SLERR_TXTDB_ZEROLENFIXEDFLD, 0);
	return 1;
}

int ImpExpExcelWorkbook::InitWorkSheet()
{
	int    ok = 0;
	OpenXLSX::XLDocument * p_doc = static_cast<OpenXLSX::XLDocument *>(P_Doc);
	if(p_doc) {
		OpenXLSX::XLWorksheet * p_sheet = static_cast<OpenXLSX::XLWorksheet *>(P_Sheet);
		if(p_sheet) {
			ZDELETE(p_sheet);
			P_Sheet = 0;
		}
		try {
			p_sheet = new OpenXLSX::XLWorksheet(p_doc->workbook().sheet(1));
			P_Sheet = p_sheet;
			ok = 1;
		} catch(OpenXLSX::XLException exn) {
			ok = 0;
		}
		/*if(State & stReadOnly) {
		}
		else {
		}*/
	}
	return ok;
}

int ImpExpExcelWorkbook::Open(const char * pFileName, const ExcelIoParam * pParam, bool readOnly)
{
	int    ok = 1;
	SString err_msg;
	SString file_path(pFileName);
	OpenXLSX::XLDocument * p_doc = 0;
	Close();
	RVALUEPTR(P, pParam);
	THROW(file_path.NotEmptyS()); // @todo @err
	SFsPath::ReplaceExt(file_path, "xlsx", 0);
	try {
		if(fileExists(file_path)) {
			p_doc = new OpenXLSX::XLDocument();
			p_doc->open(file_path.cptr());
			P_Doc = p_doc;
			SETFLAG(State, stReadOnly, readOnly);
		}
		else {
			THROW(!readOnly); // @todo @err
			{
				p_doc = new OpenXLSX::XLDocument();
				p_doc->create(file_path.cptr(), OpenXLSX::XLForceOverwrite);
				P_Doc = p_doc;
			}
		}
		THROW(InitWorkSheet());
		{
			OpenXLSX::XLWorksheet * p_sheet = static_cast<OpenXLSX::XLWorksheet *>(P_Sheet);
			OpenXLSX::XLCellFormats & r_cell_formats = p_doc->styles().cellFormats();
			OpenXLSX::XLNumberFormats & r_num_formats = p_doc->styles().numberFormats();
			//
			OpenXLSX::XLStyleIndex a1_fmt_idx = p_sheet->cell(1, 1).cellFormat(); // get index of cell format
			OpenXLSX::XLStyleIndex a1_font_idx = r_cell_formats[a1_fmt_idx].fontIndex(); // get index of used font
			//
			OpenXLSX::XLFonts & r_fonts = p_doc->styles().fonts();
			OpenXLSX::XLFills & r_fills = p_doc->styles().fills();
			//
			RegFontIdx = r_fonts.create(r_fonts[a1_font_idx]);
			r_fonts[RegFontIdx].setFontSize(9);
			HdrFontIdx = r_fonts.create(r_fonts[a1_font_idx]);
			r_fonts[HdrFontIdx].setFontSize(10);
			r_fonts[HdrFontIdx].setBold();
		}
		THROW(Scan());
	} catch(OpenXLSX::XLException exn) {
		err_msg = exn.what();
		CALLEXCEPT();
	}
	CATCHZOK
	return ok;
}

int ImpExpExcelWorkbook::GetFldNames()
{
	FldNames.Z();
	const  int max_items = 100;
	const  bool is_vert = LOGIC(P.Flags & ExcelIoParam::fVerticalRec);
	int    stop = 0;
	long   col = P.ColumnsCount + 1;
	long   row = P.HdrLinesCount + 1;
	SString temp_buf;
	OpenXLSX::XLWorksheet * p_sheet = static_cast<OpenXLSX::XLWorksheet *>(P_Sheet);
	if(p_sheet) {
		for(int i = 1; i <= max_items; i++) {
			temp_buf.Z();
			std::string cell_value_text = p_sheet->cell(row, col).getString();
			temp_buf.CatN(cell_value_text.data(), cell_value_text.size());			
			//if(P_Sheet->GetValue(row, col, temp_buf.Z()) > 0 && temp_buf.NotEmptyS())
			if(temp_buf.NotEmptyS()) {
				temp_buf.Transf(CTRANSF_UTF8_TO_OUTER); 
				FldNames.add(temp_buf.ToUpper1251());
			}
			else if(is_vert)
				FldNames.add(temp_buf.Z().CatChar('$').Cat(row));
			else
				FldNames.add(temp_buf.Z().CatChar('$').Cat(col));
			if(is_vert)
				row++;
			else
				col++;
		}
	}
	return 1;
}

int ImpExpExcelWorkbook::Scan()
{
	const int max_empty = 5; // Максимальное количество пустых ячеек, просмотр после которых считается избыточным.
		// То есть, после max_empty пустых ячеек функция считает, что дальше ничего интересного нет.
	const int is_vert = BIN(P.Flags & ExcelIoParam::fVerticalRec);
	const int end_str_decl = BIN(P.EndStr_.NotEmpty());
	int    ok = 1;
	bool   stop = false;
	long   max_col = 0;
	long   max_row = 0;
	long   row = 1 + P.HdrLinesCount;
	long   col = 1 + P.ColumnsCount;
	SString temp_buf;
	SString end_str;
	OpenXLSX::XLWorksheet * p_sheet = static_cast<OpenXLSX::XLWorksheet *>(P_Sheet);
	RecCount = 0;
	if(p_sheet) {
		if(P.Flags & ExcelIoParam::fFldNameRec)
			GetFldNames();
		if(!FldNames.getCount()) {
			while(!stop) {
				temp_buf.Z();
				std::string cell_value_text = p_sheet->cell(row, col).getString();
				temp_buf.CatN(cell_value_text.data(), cell_value_text.size());
				//if(p_sheet->GetValue(row, col, temp_buf) > 0 && temp_buf.NotEmptyS()) {
				if(temp_buf.NotEmptyS()) {
					max_row = is_vert ? 0 : row;
					max_col = is_vert ? col : 0;
				}
				if(is_vert) {
					col++;
					stop = (col >= MAX_COLUMN);
				}
				else {
					row++;
					stop = (row >= MAX_COLUMN);
				}
				if(is_vert && !stop)
					stop = temp_buf.IsEmpty();
			}
		}
		else {
			max_col = is_vert ? 0 : FldNames.getCount();
			max_row = is_vert ? FldNames.getCount() : 0;
		}
		stop = false;
		row = is_vert ? (1 + P.HdrLinesCount) : (1 + P.HdrLinesCount + BIN(P.Flags & ExcelIoParam::fFldNameRec));
		col = is_vert ? (1 + P.ColumnsCount + BIN(P.Flags & ExcelIoParam::fFldNameRec)) : (1 + P.ColumnsCount);
		(end_str = P.EndStr_);
		while(!stop) {
			int    found_data = 0;
			int    end_rec = 0;
			int    empty_count = 0;
			row = is_vert ? 1 + P.HdrLinesCount : row;
			col = is_vert ? col : 1 + P.ColumnsCount;
			while(/*!found_data && */!end_rec) {
				temp_buf.Z();
				std::string cell_value_text = p_sheet->cell(row, col).getString();
				temp_buf.CatN(cell_value_text.data(), cell_value_text.size());
				//p_sheet->GetValue(row, col, temp_buf);
				const  bool is_empty = temp_buf.IsEmpty();
				if(!is_empty || end_str_decl)
					empty_count = 0;
				else
					empty_count++;
				if(empty_count < max_empty) {
					temp_buf.Strip().Transf(CTRANSF_UTF8_TO_INNER);
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
				stop = true;
			else
				stop = is_vert ? (col >= MAX_ROW) : (row >= MAX_ROW);
		}
		RecCount = is_vert ? (max_col - P.ColumnsCount) : (max_row - P.HdrLinesCount);
		RecCount -= BIN(FldNames.getCount());
	}
	if(P.Flags & ExcelIoParam::fOneRecPerFile)
		RecCount = BIN(RecCount > 0);
	if(RecCount <= 0)
		ok = -1;
	return ok;
}

int ImpExpExcelWorkbook::Save()
{
	int    ok = -1;
	if(P_Doc && !(State & stReadOnly)) {
		try {
			OpenXLSX::XLDocument * p_doc = static_cast<OpenXLSX::XLDocument *>(P_Doc);
			p_doc->save();
			ok = 1;
		} catch(OpenXLSX::XLException exn) {
			ok = 0;
		}
	}
	return ok;
}

int ImpExpExcelWorkbook::Close()
{
	int    ok = -1;
	if(P_Sheet) {
		OpenXLSX::XLWorksheet * p_sheet = static_cast<OpenXLSX::XLWorksheet *>(P_Sheet);
		if(!(State & stReadOnly)) {
			for(long i = 0; i < WidthList.getCountI(); i++) {
				OpenXLSX::XLColumn column = p_sheet->column(static_cast<uint16>(1+i+P.ColumnsCount));
				column.setWidth(static_cast<float>(WidthList.at(i)));
				//THROW(P_Sheet->SetColumnWidth(1 + i + P.ColumnsCount, WidthList.at(i)) > 0);
			}
			Save();
		}
		delete p_sheet;
		P_Sheet = 0;
		ok = 1;
	}
	if(P_Doc) {
		delete static_cast<OpenXLSX::XLDocument *>(P_Doc);
		P_Doc = 0;
		ok = 1;
	}
	RegFontIdx = _FFFF32;
	HdrFontIdx = _FFFF32;
	CurRec   = _FFFF32;
	RecCount = 0;
	State &= ~stReadOnly;
	return ok;
}

void ImpExpExcelWorkbook::PutFieldDataToBuf(const SdbField & rFld, const SString & rTextData, void * pRecBuf)
{
	ExcelDbFile::PutFieldDataToBuf_(P, rFld, rTextData, pRecBuf);
}

void ImpExpExcelWorkbook::GetFieldDataFromBuf(const SdbField & rFld, SString & rTextData, const void * pRecBuf)
{
	ExcelDbFile::GetFieldDataFromBuf_(P, rFld, rTextData, pRecBuf);
}

uint32 ImpExpExcelWorkbook::GetNumRecords() const { return RecCount; }

int ImpExpExcelWorkbook::GoToRecord(uint32 recNo/*, int rel = relAbs*/)
{
	int    ok = -1;
	if(recNo >= 0 && recNo < RecCount) {
		CurRec = recNo;
		ok = 1;
	}
	return ok;
}

int ImpExpExcelWorkbook::GetRecord(const SdRecord & rRec, void * pDataBuf)
{
	int    ok = -1;
	OpenXLSX::XLWorksheet * p_sheet = static_cast<OpenXLSX::XLWorksheet *>(P_Sheet);
	if(p_sheet) {
		THROW(CheckParam(rRec));
		if(CurRec >= 0 && CurRec < RecCount) {
			const  bool is_vert = LOGIC(P.Flags & ExcelIoParam::fVerticalRec);
			long   cur_rec = 1 + CurRec + (is_vert ? P.ColumnsCount : P.HdrLinesCount);
			long   row = 0;
			long   col = 0;
			SString temp_buf;
			SString str_row_no;
			SString str_col_no;
			SString field_buf;
			SdbField fld;
			if(FldNames.getCount() == 0) {
				const char * p_celln = "cell";
				for(uint fld_pos = 0; fld_pos < rRec.GetCount(); fld_pos++) {
					if(rRec.GetFieldByPos(fld_pos, &fld) > 0) {
						row = col = 0;
						if(fld.Name.HasPrefixIAscii(p_celln)) {
							(temp_buf = fld.Name).ShiftLeft(sstrlen(p_celln));
							temp_buf.Divide('_', str_row_no, str_col_no);
							row = str_row_no.ToLong();
							col = str_col_no.ToLong();
						}
						SETIFZ(row, is_vert ? (1 + fld_pos + P.HdrLinesCount) : cur_rec);
						SETIFZ(col, is_vert ? cur_rec : (1 + fld_pos + P.ColumnsCount));
						{
							field_buf.Z();
							std::string cell_value_text = p_sheet->cell(row, col).getString();
							field_buf.CatN(cell_value_text.data(), cell_value_text.size()).Transf(CTRANSF_UTF8_TO_OUTER);
							//THROW(P_Sheet->GetValue(row, col, field_buf));
						}
						PutFieldDataToBuf(fld, field_buf.Strip(), pDataBuf);
					}
				}
			}
			else {
				cur_rec++; // Пропустим наименования столбцов/строк
				for(uint p = 0, fld_pos = 0; FldNames.get(&p, temp_buf); fld_pos++) {
					if(rRec.GetFieldByName(temp_buf, &fld) > 0) {
						row = (is_vert) ? 1 + fld_pos + P.HdrLinesCount : cur_rec;
						col = (is_vert) ? cur_rec : 1 + fld_pos + P.ColumnsCount;
						{
							field_buf.Z();
							std::string cell_value_text = p_sheet->cell(row, col).getString();
							field_buf.CatN(cell_value_text.data(), cell_value_text.size()).Transf(CTRANSF_UTF8_TO_OUTER);
							//THROW(P_Sheet->GetValue(row, col, field_buf));
						}
						PutFieldDataToBuf(fld, field_buf.Strip(), pDataBuf);
					}
				}
			}
			ok = 1;
		}
	}
	CATCHZOK
	return ok;
}

int ImpExpExcelWorkbook::AppendRecord(const SdRecord & rRec, const void * pDataBuf) // @construction
{
	int    ok = 1;
	uint   i = 0;
	const  bool is_vert = LOGIC(P.Flags & ExcelIoParam::fVerticalRec);
	long   cur_rec = (CurRec != _FFFF32) ? CurRec : 0;
	SdbField fld;
	SString  field_buf;
	OpenXLSX::XLDocument * p_doc = static_cast<OpenXLSX::XLDocument *>(P_Doc);
	OpenXLSX::XLWorksheet * p_sheet = static_cast<OpenXLSX::XLWorksheet *>(P_Sheet);
	THROW(p_doc); // @todo @err
	THROW(p_sheet); // @todo @err
	THROW(CheckParam(rRec));
	{
		OpenXLSX::XLCellFormats & r_cell_formats = p_doc->styles().cellFormats();
		//OpenXLSX::XLNumberFormats & r_num_formats = p_doc->styles().numberFormats();
		//
		//OpenXLSX::XLStyleIndex a1_fmt_idx = p_sheet->cell(1, 1).cellFormat(); // get index of cell format
		//OpenXLSX::XLStyleIndex a1_font_idx = r_cell_formats[a1_fmt_idx].fontIndex(); // get index of used font
		//
		//OpenXLSX::XLFonts & r_fonts = p_doc->styles().fonts();
		//OpenXLSX::XLFills & r_fills = p_doc->styles().fills();
		//
		//const OpenXLSX::XLStyleIndex reg_font_idx = r_fonts.create(r_fonts[a1_font_idx]);
		//r_fonts[reg_font_idx].setFontSize(9);
		//
		cur_rec += (1 + BIN(P.Flags & ExcelIoParam::fFldNameRec));
		if(!is_vert)
			cur_rec += P.HdrLinesCount;
		else
			cur_rec += P.ColumnsCount;
		if(oneof2(CurRec, 0, _FFFF32)) {
			//P_Sheet->_Clear(1, MAX_COLUMN);
			if(p_sheet->rowCount() && p_sheet->columnCount())
				p_sheet->range().clear();
			//
			// Если файл пустой, то добавляем специфицированное количество пустых строк
			// и (если необходимо) запись, содержащую наименования полей.
			//
			CurRec = 0;
			if(P.Flags & ExcelIoParam::fFldNameRec) {
				IntRange _row_range;
				IntRange _col_range;
				for(i = 0; i < rRec.GetCount(); i++) {
					THROW(rRec.GetFieldByPos(i, &fld));
					field_buf = fld.Name;
					if(!field_buf.NotEmptyS())
						field_buf.Cat(fld.ID);
					_commfmt(fld.OuterFormat, field_buf);
					const long row = is_vert ? (1 + i + P.HdrLinesCount) : (cur_rec - 1);
					const long col = is_vert ? (cur_rec - 1) : (1 + i + P.ColumnsCount);
					_row_range.Expand(row);
					_col_range.Expand(col);
					//THROW(P_Sheet->SetValue(row, col, field_buf.Strip().cptr()) > 0);
					field_buf.Strip().Transf(CTRANSF_OUTER_TO_UTF8);
					p_sheet->cell(row, static_cast<uint16>(col)).value().set(field_buf.cptr());
					//THROW(P_Sheet->SetBold(row, col, 1));
					//(p_sheet->cell(row, col).cellFormat())
					if(WidthList.getCountI() < (col - P.ColumnsCount))
						WidthList.add(field_buf.LenI());
					if(WidthList.at(col - 1 - P.ColumnsCount) < field_buf.LenI())
		 				WidthList.at(col - 1 - P.ColumnsCount) = field_buf.LenI();
				}
				if(!_row_range.IsZero())
					SETIFZQ(_row_range.low, 1);
				if(!_col_range.IsZero())
					SETIFZQ(_col_range.low, 1);
				if(!_row_range.IsZero() && !_col_range.IsZero()) {
					if(HdrFontIdx != _FFFF32) {
						OpenXLSX::XLCellRange _cell_range = p_sheet->range(OpenXLSX::XLCellReference(_row_range.low, static_cast<uint16>(_col_range.low)), 
							OpenXLSX::XLCellReference(_row_range.upp, static_cast<uint16>(_col_range.upp)));
						OpenXLSX::XLStyleIndex _style_idx = r_cell_formats.create();
						r_cell_formats[_style_idx].setFontIndex(HdrFontIdx);
						_cell_range.setFormat(_style_idx);
					}
				}
			}
		}
		{
			IntRange _row_range;
			IntRange _col_range;
			for(i = 0; i < rRec.GetCount(); i++) {
				THROW(rRec.GetFieldByPos(i, &fld));
				const long row = is_vert ? (1 + i + P.HdrLinesCount) : cur_rec;
				const long col = is_vert ? cur_rec : (1 + i + P.ColumnsCount);
				_row_range.Expand(row);
				_col_range.Expand(col);
				{
					const  TYPEID st = fld.T.GetDbFieldType();
					int    base_type = stbase(st);
					const  void * p_fld_data = PTR8C(pDataBuf)+fld.InnerOffs;
					if(base_type == BTS_REAL) {
						double real_val = 0.0;
						sttobase(st, p_fld_data, &real_val);
						//THROW(P_Sheet->SetValue(row, col, real_val) > 0);
						p_sheet->cell(row, static_cast<uint16>(col)).value().set(real_val);
					}
					else {
						GetFieldDataFromBuf(fld, field_buf.Z(), pDataBuf);
						//THROW(P_Sheet->SetValue(row, col, field_buf.Strip().cptr()) > 0);
						field_buf.Strip().Transf(CTRANSF_OUTER_TO_UTF8);
						p_sheet->cell(row, static_cast<uint16>(col)).value().set(field_buf.cptr());
					}
				}
				if(WidthList.getCount() < static_cast<uint>(col - P.ColumnsCount))
					WidthList.add(field_buf.LenI());
				if(WidthList.at(col - 1 - P.ColumnsCount) < field_buf.LenI())
 					WidthList.at(col - 1 - P.ColumnsCount) = field_buf.LenI();
			}
			if(!_row_range.IsZero())
				SETIFZQ(_row_range.low, 1);
			if(!_col_range.IsZero())
				SETIFZQ(_col_range.low, 1);
			/* очень медленно!
			if(!_row_range.IsZero() && !_col_range.IsZero()) {
				if(RegFontIdx != _FFFF32) {
					OpenXLSX::XLCellRange _cell_range = p_sheet->range(OpenXLSX::XLCellReference(_row_range.low, static_cast<uint16>(_col_range.low)), 
						OpenXLSX::XLCellReference(_row_range.upp, static_cast<uint16>(_col_range.upp)));
					OpenXLSX::XLStyleIndex _style_idx = r_cell_formats.create();
					r_cell_formats[_style_idx].setFontIndex(RegFontIdx);
					_cell_range.setFormat(_style_idx);
				}
			}*/
		}
		CurRec++;
	}
	CATCHZOK
	return ok;
}