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
		try {
			OpenXLSX::XLDocument doc;
			doc.create(path.cptr(), OpenXLSX::XLForceOverwrite);			
			OpenXLSX::XLWorksheet wks = doc.workbook().worksheet("Sheet1");
			{
				// Выводим название групп столбцов
				for(uint i = 0; i < p_def->GetGroupCount(); i++) {
					const BroGroup * p_grp = p_def->GetGroup(i);
					(temp_buf = p_grp->P_Text).Transf(CTRANSF_INNER_TO_UTF8);
					wks.cell(beg_row, p_grp->First+1).value() = temp_buf.cptr();
					//THROW(p_sheet->SetBold(beg_row, p_grp->First + 1, 1) > 0);
				}
				if(p_def->GetGroupCount())
					beg_row++;
			}
			{
				// Выводим название столбцов
				for(uint i = 0; i < cn_count; i++) {
					const BroColumn & r_c = p_def->at(i);
					const long type = GETSTYPE(r_c.T);
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
					wks.cell(beg_row, i+1).value() = temp_buf.cptr();
					//THROW(p_sheet->SetBold(beg_row, i + 1, 1) > 0);
				}
				beg_row++;
			}
			{
				if(p_def->top() > 0) {
					long row = 0;
					SString val_buf;
					do {
						PROFILE_START
						for(long cn = 0; cn < static_cast<long>(cn_count); cn++) {
							p_def->getFullText(p_def->_curItem(), cn, val_buf);
							val_buf.Strip().Transf(CTRANSF_INNER_TO_UTF8);
							/*if(GETSTYPE(p_def->at(cn).T) == S_FLOAT) {
								val_buf.ReplaceChar('.', dec.C(0));
							}*/
							//THROW(p_sheet->SetValue(row + beg_row + 1, cn + 1, val_buf) > 0);
							wks.cell(row + beg_row, cn+1).value() = val_buf.cptr();
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
						row++;
						PROFILE_END
					} while(p_def->step(1) > 0 && row < 1000000);
					for(uint i = 0; i < width_ary.getCount(); i++) {
						//THROW(p_sheet->SetColumnWidth(i + 1, width_ary.at(i) + 2) > 0);
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