// ppexcelapp.cpp
// Copyright (c) A.Sobolev 2025
// @codepage UTF-8
// Функционал, использующий работу с Excel-таблицами (посредством OpenXLSX)
//
#include <pp.h>
#pragma hdrstop
#include <..\osf\OpenXLSX\SRC\headers\OpenXLSX.hpp>
//
// Эксперименты с библиотекой OpenXLSX
//
void Test_OpenXLSX()
{
	SString path;
	SString err_msg;
	PPGetFilePath(PPPATH_OUT, "Test_OpenXLSX.xlsx", path);
	try {
		{
			OpenXLSX::XLDocument doc;
			doc.create(path.cptr(), OpenXLSX::XLForceOverwrite);
			OpenXLSX::XLWorksheet wks = doc.workbook().worksheet("Sheet1");
			//
			wks.cell("A1").value() = 3.14159265358979323846;
			wks.cell("B1").value() = 42;
			wks.cell("C1").value() = "  Hello OpenXLSX!  ";
			wks.cell("D1").value() = true;
			wks.cell("E1").value() = std::sqrt(-2); // Result is NAN, resulting in an error value in the Excel spreadsheet.

			// As mentioned, the .value() method can also be used for getting tha value of a cell.
			// The .value() method returns a proxy object that cannot be copied or assigned, but
			// it can be implicitly converted to an XLCellValue object, as shown below.
			// Unfortunately, it is not possible to use the 'auto' keyword, so the XLCellValue
			// type has to be explicitly stated.
			OpenXLSX::XLCellValue A1 = wks.cell("A1").value();
			OpenXLSX::XLCellValue B1 = wks.cell("B1").value();
			OpenXLSX::XLCellValue C1 = wks.cell("C1").value();
			OpenXLSX::XLCellValue D1 = wks.cell("D1").value();
			OpenXLSX::XLCellValue E1 = wks.cell("E1").value();

			// The cell value can be implicitly converted to a basic c++ type. However, if the type does not
			// match the type contained in the XLCellValue object (if, for example, floating point value is
			// assigned to a std::string), then an XLValueTypeError exception will be thrown.
			// To check which type is contained, use the .type() method, which will return a XLValueType enum
			// representing the type. As a convenience, the .typeAsString() method returns the type as a string,
			// which can be useful when printing to console.
			double vA1 = wks.cell("A1").value();
			int vB1 = wks.cell("B1").value();
			std::string vC1 = wks.cell("C1").value();
			bool vD1 = wks.cell("D1").value();
			double vE1 = wks.cell("E1").value();

			doc.save();
			doc.close();
		}
		{
			OpenXLSX::XLDocument doc;
			doc.open(path.cptr());
			auto wks = doc.workbook().worksheet("Sheet1");
			OpenXLSX::XLCellValue A1 = wks.cell("A1").value();
			OpenXLSX::XLCellValue B1 = wks.cell("B1").value();
			OpenXLSX::XLCellValue C1 = wks.cell("C1").value();
			OpenXLSX::XLCellValue D1 = wks.cell("D1").value();
			OpenXLSX::XLCellValue E1 = wks.cell("E1").value();
			double vA1 = wks.cell("A1").value();
			int vB1 = wks.cell("B1").value();
			std::string vC1 = wks.cell("C1").value();
			bool vD1 = wks.cell("D1").value();
			double vE1 = wks.cell("E1").value();
			doc.close();
		}
	} catch(OpenXLSX::XLException exn) {
		err_msg = exn.what();
	}
}
//
//
//
SString & PPViewBrowser::Helper_Export_MakeResultName(bool toUtf8, SString & rBuf)
{
	rBuf.Z();
	rBuf = getTitle();
	rBuf.Transf(toUtf8 ? CTRANSF_INNER_TO_UTF8 : CTRANSF_INNER_TO_OUTER);
	rBuf.ReplaceChar('*', '#'); // Замена запрещенного символа в названии, если таковой имеется
	return rBuf;
}

SString & PPViewBrowser::Helper_Export_MakeResultFilePath(bool toUtf8, const char * pName, SString & rBuf)
{
	rBuf.Z();
	if(!isempty(pName)) {
		SString name_buf(pName);
		PPGetPath(PPPATH_LOCAL, rBuf);
		if(toUtf8)
			rBuf.Transf(CTRANSF_OUTER_TO_UTF8);
		rBuf.SetLastSlash();
		SFile::CreateDir(rBuf);
		rBuf.Cat(name_buf.ReplaceChar('/', ' ')).Dot().Cat("xlsx");
	}
	return rBuf;
}

int PPViewBrowser::Helper_Export_Excel_OXLSX(SString & rResultFileName)
{
	using namespace OpenXLSX;
	rResultFileName.Z();
	int    ok = 1;
	SString temp_buf;
	SString err_msg;
	SString name;
	SString path;
	BrowserDef * p_def = getDef();
	if(p_def) {
		const  uint cn_count = p_def->getCount();
		long   beg_row = 1;
		PPIDArray width_ary;
		Helper_Export_MakeResultName(true/*toUtf8*/, name);
		Helper_Export_MakeResultFilePath(true/*toUtf8*/, name, path);
		uint32 nfmtid_date = 101;

		try {
			XLDocument doc;
			doc.create(path.cptr(), OpenXLSX::XLForceOverwrite);			
			XLWorksheet wks = doc.workbook().worksheet("Sheet1");
			//
			XLCellFormats & r_cell_formats = doc.styles().cellFormats();
			XLNumberFormats & r_num_formats = doc.styles().numberFormats();
			/* (sample) {
				XLStyleIndex nfmt_idx = r_num_formats.create();
				r_num_formats[nfmt_idx].setNumberFormatId(15);
				r_num_formats[nfmt_idx].setFormatCode("fifteen");
			}*/
			//
			XLStyleIndex a1_fmt_idx = wks.cell(1, 1).cellFormat(); // get index of cell format
			XLStyleIndex a1_font_idx = r_cell_formats[a1_fmt_idx].fontIndex(); // get index of used font
			//
			XLFonts & r_fonts = doc.styles().fonts();
			XLFills & r_fills = doc.styles().fills();
			//
			const XLStyleIndex reg_font_idx = r_fonts.create(r_fonts[a1_font_idx]);
			r_fonts[reg_font_idx].setFontSize(9);
			//
			//XLStyleIndex reg_font_idx = r_fonts.create(r_fonts[a1_font_idx]);
			//r_fonts[reg_font_idx].setFontSize(8);
			{
				//
				// Область заголовка
				//
				IntRange hdr_row_range;
				IntRange hdr_col_range;
				hdr_row_range.low = beg_row;
				hdr_col_range.low = 1;
				{
					//
					// Выводим название групп столбцов
					//
					for(uint i = 0; i < p_def->GetGroupCount(); i++) {
						const BroGroup * p_grp = p_def->GetGroup(i);
						(temp_buf = p_grp->P_Text).Transf(CTRANSF_INNER_TO_UTF8);
						const uint16 column_idx = p_grp->First+1;
						SETMAX(hdr_col_range.upp, column_idx);
						wks.cell(beg_row, column_idx).value() = temp_buf.cptr();
						/*{
							XLStyleIndex cell_fmt_idx = wks.cell(beg_row, column_idx).cellFormat(); // get index of cell format
							XLStyleIndex font_idx = r_cell_formats[cell_fmt_idx].fontIndex(); // get index of used font
							r_fonts[font_idx].setFontSize(14);
							r_fonts[font_idx].setBold();
						}*/
						//THROW(p_sheet->SetBold(beg_row, p_grp->First + 1, 1) > 0);
					}
					if(p_def->GetGroupCount())
						beg_row++;
				}
				{
					//
					// Выводим название столбцов
					//
					for(uint i = 0; i < cn_count; i++) {
						const BroColumn & r_c = p_def->at(i);
						const long type = GETSTYPE(r_c.T);
						const uint16 column_idx = i+1;
						SETMAX(hdr_col_range.upp, column_idx);
						(temp_buf = r_c.text).Transf(CTRANSF_INNER_TO_UTF8);
						width_ary.add((PPID)temp_buf.Len());
						/*
						fmt.Z();
						fmt_rus.Z();
						if(type == S_DATE) {
							PPLoadString("fmt_excel_date", fmt);
							PPLoadString("fmt_excel_date_ru", fmt_rus);
							fmt_rus.Transf(CTRANSF_INNER_TO_UTF8);
						}
						else if(oneof3(type, S_INT, S_UINT, S_AUTOINC))
							fmt.CatChar('0');
						else if(type == S_FLOAT) {
							size_t prec = SFMTPRC(r_c.format);
							fmt_rus = fmt.CatChar('0').CatChar(dec.C(0)).CatCharN('0', prec);
						}
						else
							fmt_rus = fmt.CatChar('@');
						if(p_sheet->SetColumnFormat(i + 1, fmt) <= 0)
							p_sheet->SetColumnFormat(i + 1, fmt_rus);
						THROW(p_sheet->SetCellFormat(beg_row, i + 1, "@") > 0);
						*/
						wks.cell(beg_row, column_idx).value() = temp_buf.cptr();
						/*{
							XLStyleIndex cell_fmt_idx = wks.cell(beg_row, column_idx).cellFormat(); // get index of cell format
							XLStyleIndex font_idx = r_cell_formats[cell_fmt_idx].fontIndex(); // get index of used font
							r_fonts[font_idx].setFontSize(14);
							r_fonts[font_idx].setBold();
						}*/
						//THROW(p_sheet->SetBold(beg_row, i + 1, 1) > 0);
					}
					beg_row++;
				}
				hdr_row_range.upp = beg_row-1;
				//
				XLCellRange hdr_cell_range = wks.range(XLCellReference(hdr_row_range.low, hdr_col_range.low), XLCellReference(hdr_row_range.upp, hdr_col_range.upp));
				{
					/*
					XLStyleIndex fontBold = fonts.create();
					fonts[ fontBold ].setBold();
					XLStyleIndex boldDefault = cellFormats.create();
					cellFormats[ boldDefault ].setFontIndex( fontBold );

					cellRange = wks.range("A5:Z5");
					cellRange = "this is a bold range"; // NEW functionality: XLCellRange now has a direct value assignment overload
					cellRange.setFormat( boldDefault ); // NEW functionality: XLCellRange supports setting format for all cells in the range
					*/
					XLStyleIndex header_style_idx = r_cell_formats.create();
					XLStyleIndex header_font_idx = r_fonts.create(r_fonts[a1_font_idx]);
					r_fonts[header_font_idx].setBold();
					r_fonts[header_font_idx].setFontSize(11);
					r_cell_formats[header_style_idx].setFontIndex(header_font_idx);
					hdr_cell_range.setFormat(header_style_idx);
				}
			}
			{
				if(p_def->top() > 0) {
					long   row = 0;
					SString val_buf;
					uint8  cell_data[2048];
					do {
						PROFILE_START
						for(long cn = 0; cn < static_cast<long>(cn_count); cn++) {
							const BroColumn & r_bc = p_def->at(cn);
							TYPEID typ = 0;
							if(p_def->GetCellData(p_def->_curItem(), cn, &typ, cell_data, sizeof(cell_data))) {
								if(GETSTYPE(typ) == S_FLOAT) {
									double v = 0.0;
									if(GETSSIZE(typ) == 8) {
										v = *reinterpret_cast<const double *>(cell_data);
									}
									else if(GETSSIZE(typ) == 4) {
										v = *reinterpret_cast<const float *>(cell_data);
									}
									wks.cell(row + beg_row, static_cast<uint16>(cn+1)).value() = v;
								}
								else if(GETSTYPE(typ) == S_INT) {
									int64 v = 0;
									if(GETSSIZE(typ) == 8) {
										v = *reinterpret_cast<const int64 *>(cell_data);
									}
									else if(GETSSIZE(typ) == 4) {
										v = *reinterpret_cast<const int32 *>(cell_data);
									}
									else if(GETSSIZE(typ) == 2) {
										v = *reinterpret_cast<const int16 *>(cell_data);
									}
									else if(GETSSIZE(typ) == 1) {
										v = *reinterpret_cast<const int8 *>(cell_data);
									}
									wks.cell(row + beg_row, static_cast<uint16>(cn+1)).value() = v;
								}
								else if(GETSTYPE(typ) == S_INT64) {
									int64 v = 0;
									v = *reinterpret_cast<const int64 *>(cell_data);
									wks.cell(row + beg_row, static_cast<uint16>(cn+1)).value() = v;
								}
								else if(GETSTYPE(typ) == S_DATE) {
									LDATE v = ZERODATE;
									v = *reinterpret_cast<const LDATE *>(cell_data);
									if(checkdate(v)) {
										wks.cell(row + beg_row, static_cast<uint16>(cn+1)).value() = XLDateTime(v.GetOleDate());
									}
								}
								else if(GETSTYPE(typ) == S_TIME) {
									LTIME v = ZEROTIME;
									v = *reinterpret_cast<const LTIME *>(cell_data);
									if(checktime(v)) {
										wks.cell(row + beg_row, static_cast<uint16>(cn+1)).value() = XLDateTime(v.GetOleDate());
									}
								}
								else if(GETSTYPE(typ) == S_DATETIME) {
									LDATETIME v;
									v = *reinterpret_cast<const LDATETIME *>(cell_data);
									if(checkdate(v.d)) {
										wks.cell(row + beg_row, static_cast<uint16>(cn+1)).value() = XLDateTime(v.GetOleDate());
									}
								}
								else {
									//p_def->getFullText(p_def->_curItem(), cn, val_buf);
									char   dest_text[1024];
									dest_text[0] = 0;
									long   fmt = r_bc.format;
									SETSFMTLEN(fmt, 0);
									sttostr(r_bc.T, cell_data, fmt, dest_text);
									val_buf = dest_text;
									//
									val_buf.Strip().Transf(CTRANSF_INNER_TO_UTF8);
									wks.cell(row + beg_row, static_cast<uint16>(cn+1)).value() = val_buf.cptr();
								}
								/*if(GETSTYPE(p_def->at(cn).T) == S_FLOAT) {
									val_buf.ReplaceChar('.', dec.C(0));
								}*/
								//THROW(p_sheet->SetValue(row + beg_row + 1, cn + 1, val_buf) > 0);
								/*
								{
									COLORREF color;
									if(GetCellColor(p_def->_curItem(), cn, &color) > 0)
										THROW(p_sheet->SetColor(row + beg_row + 1, cn + 1, color) > 0);
								}
								*/
								if(width_ary.at(cn) < (long)val_buf.Len())
									width_ary.at(cn) = (PPID)val_buf.Len();
							}
						}
						row++;
						PROFILE_END
					} while(p_def->step(1) > 0 && row < 1000000);
					{
						for(uint i = 0; i < width_ary.getCount(); i++) {
							//THROW(p_sheet->SetColumnWidth(i + 1, width_ary.at(i) + 2) > 0);
						}
					}
					{
						XLCellRange _cell_range = wks.range(XLCellReference(beg_row, static_cast<uint16>(1)), XLCellReference(beg_row+row-1, static_cast<uint16>(cn_count)));
						XLStyleIndex _style_idx = r_cell_formats.create();
						r_cell_formats[_style_idx].setFontIndex(reg_font_idx);
						_cell_range.setFormat(_style_idx);
					}
					{
						uint    seq_fmt_id = nfmtid_date;
						for(long cn = 0; cn < static_cast<long>(cn_count); cn++) {
							const BroColumn & r_c = p_def->at(cn);
							const long type = GETSTYPE(r_c.T);
							if(type == S_FLOAT) {
								const int prec = SFMTPRC(r_c.format);
								temp_buf.Z();
								if(prec > 0) {
									temp_buf.Z().Cat("#0").Dot().CatCharN('0', prec);
								}
								else if(prec == 0) {
									temp_buf.Z().Cat("#0");
								}
								if(temp_buf.NotEmpty()) {
									++seq_fmt_id;
									XLStyleIndex nfmt_c_idx = r_num_formats.create();
									r_num_formats[nfmt_c_idx].setNumberFormatId(seq_fmt_id);
									r_num_formats[nfmt_c_idx].setFormatCode(temp_buf.cptr());
									//
									XLCellRange _cell_range = wks.range(XLCellReference(beg_row, static_cast<uint16>(cn+1)), XLCellReference(beg_row+row-1, static_cast<uint16>(cn+1)));
									XLStyleIndex _style_idx = r_cell_formats.create();
									r_cell_formats[_style_idx].setNumberFormatId(seq_fmt_id);
									r_cell_formats[_style_idx].setFontIndex(reg_font_idx);
									_cell_range.setFormat(_style_idx);
								}
							}
							else if(type == S_DATE) {
								XLStyleIndex nfmt_date_idx = r_num_formats.create();
								r_num_formats[nfmt_date_idx].setNumberFormatId(nfmtid_date);
								r_num_formats[nfmt_date_idx].setFormatCode("dd/mm/yyyy");
								//
								XLCellRange _cell_range = wks.range(XLCellReference(beg_row, static_cast<uint16>(cn+1)), XLCellReference(beg_row+row-1, static_cast<uint16>(cn+1)));
								XLStyleIndex _style_idx = r_cell_formats.create();
								r_cell_formats[_style_idx].setNumberFormatId(nfmtid_date);
								r_cell_formats[_style_idx].setFontIndex(reg_font_idx);
								_cell_range.setFormat(_style_idx);
							}
							else if(type == S_TIME) {
							}
							else if(type == S_DATETIME) {
							}
						}
					}
				}
				WMHScroll(SB_VERT, SB_BOTTOM, 0);
			}
			doc.save();
			doc.close();
			rResultFileName = path;
		} catch(OpenXLSX::XLException exn) {
			err_msg = exn.what();
			ok = 0;
		} catch(std::runtime_error exn) {
			err_msg = exn.what();
			ok = 0;
		}
	}
	//CATCHZOK
	return ok;
}

int PPViewBrowser::Helper_Export_Excel(SString & rResultFileName)
{
	rResultFileName.Z();
	int    ok = 1;
	SString name;
	SString path;
	BrowserDef * p_def = getDef();
	ComExcelApp * p_app = 0;
	ComExcelWorksheet  * p_sheet  = 0;
	ComExcelWorksheets * p_sheets = 0;
	ComExcelWorkbook   * p_wkbook = 0;
	PPWaitStart();
	if(p_def) {
		const  long cn_count = p_def->getCount();
		long   beg_row = 1;
		long   sheets_count = 0;
		long   i = 0;
		SString dec;
		SString fmt;
		SString fmt_rus;
		SString temp_buf;
		PPIDArray width_ary;
		Helper_Export_MakeResultName(false/*toUtf8*/, name);
		{
			TCHAR  li_buf[64];
			::GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL, li_buf, SIZEOFARRAY(li_buf));
			dec.Cat(SUcSwitch(li_buf));
		}
		THROW_MEM(p_app = new ComExcelApp);
		THROW(p_app->Init() > 0);
		THROW(p_wkbook = p_app->AddWkbook());
		THROW(p_sheets = p_wkbook->Get());
		sheets_count = p_sheets->GetCount();
		for(i = sheets_count; i > 1; i--)
			THROW(p_sheets->Delete(i) > 0);
		THROW(p_sheet = p_sheets->Get(1L));
		THROW(p_sheet->SetName(name));
		// Выводим название групп столбцов
		for(i = 0; i < (long)p_def->GetGroupCount(); i++) {
			const BroGroup * p_grp = p_def->GetGroup(i);
			(temp_buf = p_grp->P_Text).Transf(CTRANSF_INNER_TO_OUTER);
			THROW(p_sheet->SetValue(beg_row, p_grp->First + 1, temp_buf) > 0);
			THROW(p_sheet->SetBold(beg_row, p_grp->First + 1, 1) > 0);
			/*
			if(beg_row == 1)
				beg_row++;
			*/
		}
		if(p_def->GetGroupCount())
			beg_row++;
		// Выводим название столбцов
		for(i = 0; i < cn_count; i++) {
			const BroColumn & r_c = p_def->at(i);
			const long type = GETSTYPE(r_c.T);
			(temp_buf = r_c.text).Transf(CTRANSF_INNER_TO_OUTER);
			width_ary.add((PPID)temp_buf.Len());
			fmt.Z();
			fmt_rus.Z();
			if(type == S_DATE) {
				PPLoadString("fmt_excel_date", fmt);
				PPLoadString("fmt_excel_date_ru", fmt_rus);
				fmt_rus.Transf(CTRANSF_INNER_TO_OUTER);
			}
			else if(oneof3(type, S_INT, S_UINT, S_AUTOINC))
				fmt.CatChar('0');
			else if(type == S_FLOAT) {
				size_t prec = SFMTPRC(r_c.format);
				fmt_rus = fmt.CatChar('0').CatChar(dec.C(0)).CatCharN('0', prec);
			}
			else
				fmt_rus = fmt.CatChar('@');
			if(p_sheet->SetColumnFormat(i + 1, fmt) <= 0)
				p_sheet->SetColumnFormat(i + 1, fmt_rus);
			THROW(p_sheet->SetCellFormat(beg_row, i + 1, "@") > 0);
			THROW(p_sheet->SetValue(beg_row, i + 1, temp_buf) > 0);
			THROW(p_sheet->SetBold(beg_row, i + 1, 1) > 0);
		}
		beg_row++;
		if(p_def->top() > 0) {
			long row = 0;
			SString val_buf;
			do {
				PROFILE_START
				for(long cn = 0; cn < cn_count; cn++) {
					COLORREF color;
					p_def->getFullText(p_def->_curItem(), cn, val_buf);
					val_buf.Strip().Transf(CTRANSF_INNER_TO_OUTER);
					if(GETSTYPE(p_def->at(cn).T) == S_FLOAT) {
						val_buf.ReplaceChar('.', dec.C(0));
					}
					THROW(p_sheet->SetValue(row + beg_row + 1, cn + 1, val_buf) > 0);
					if(GetCellColor(p_def->_curItem(), cn, &color) > 0)
						THROW(p_sheet->SetColor(row + beg_row + 1, cn + 1, color) > 0);
					if(width_ary.at(cn) < (long)val_buf.Len())
						width_ary.at(cn) = (PPID)val_buf.Len();
				}
				row++;
				PROFILE_END
			} while(p_def->step(1) > 0 && row < (USHRT_MAX-beg_row));
			for(i = 0; i < (long)width_ary.getCount(); i++)
				THROW(p_sheet->SetColumnWidth(i + 1, width_ary.at(i) + 2) > 0);
		}
		WMHScroll(SB_VERT, SB_BOTTOM, 0);
	}
	CATCHZOK
	if(p_wkbook) {
		Helper_Export_MakeResultFilePath(false/*toUtf8*/, name, path);
		SFile::Remove(path);
		p_wkbook->_SaveAs(path);
		p_wkbook->_Close();
		ZDELETE(p_wkbook);
		rResultFileName = path;
	}
	PPWaitStop();
	ZDELETE(p_sheets);
	ZDELETE(p_sheet);
	ZDELETE(p_app);
	return ok;
}
