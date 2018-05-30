// COMDISP.H
// Copyright (c) V.Nasonov, A.Starodub 2003, 2004, 2006, 2007, 2013, 2017, 2018
// @codepage UTF-8
//
#ifndef __COMDISP_H // {
#define __COMDISP_H

#include <process.h>
//
//   Интерфейс IDispatch для работы с COM-приложениями (режим InProcServer, LocalServer) (only WIN32)
//
#ifdef __WIN32__ // {

struct DispIDEntry {
	long   ID;
	DISPID DispID;
	char   Name[64];
};

class ComDispInterface {
public:
	SLAPI  ComDispInterface();
	SLAPI ~ComDispInterface();
	int  SLAPI Init(const char * pProgID, int inProcServer = 1);
	int  SLAPI Init(const wchar_t * pProgID, int inProcServer = 1);
	virtual int  SLAPI Init(IDispatch * pIDisp);
	int  SLAPI AssignIDByName(const char * pName, long id);
	int  SLAPI GetNameByID(long id, SString & rName) const;
	int  SLAPI GetProperty(long propertyID, int    * pBuf);
	int  SLAPI GetProperty(long propertyID, long   * pBuf);
	int  SLAPI GetProperty(long propertyID, double * pBuf);
	int  SLAPI GetProperty(long propertyID, bool   * pBuf);
	//
	// Память под класс pIDisp выделяется внутри функции
	//
	int  SLAPI GetProperty(long propertyID, ComDispInterface * pDisp);
	//
	//   Возвращаемая строка - codepage windows-1251
	//
	int  SLAPI GetProperty(long propertyID, char   * pBuf, size_t bufLen);
	int  SLAPI SetProperty(long propertyID, int      iVal, int writeOnly = 0);
	int  SLAPI SetProperty(long propertyID, long     lVal, int writeOnly = 0);
	int  SLAPI SetProperty(long propertyID, double   dVal, int writeOnly = 0);
	int  SLAPI SetProperty(long propertyID, bool     bVal, int writeOnly = 0);
	int  SLAPI SetProperty(long propertyID, LDATE    dtVal, int writeOnly = 0); // @v7.4.1
	int  SLAPI SetPropertyByParams(long propertyID);
	//
	// Передаваемая строка - codepage windows-1251
	//
	int  SLAPI SetProperty(long propertyID, const char * pStrVal, int writeOnly = 0);
	int  SLAPI SetParam(int    iVal);
	int  SLAPI SetParam(long   lVal);
	int  SLAPI SetParam(double dVal);
	//
	// Передаваемая строка - codepage windows-1251 по умолчанию
	//
	int  SLAPI SetParam(const char * pStrVal, int codepage = 1251);
	int  SLAPI SetParam(ComDispInterface * pParam);
	int  SLAPI CallMethod(long methodID, VARIANTARG * pVarArg = NULL);
	int  SLAPI CallMethod(long methodID, ComDispInterface * pDisp);
	IDispatch * GetDisp() { return P_Disp; }
private:
	const DispIDEntry * FASTCALL GetDispIDEntry(long entryId) const;
	void SLAPI ClearParams();
	int  SLAPI _SetParam(VARIANTARG * pVarArg);
	int  SLAPI _SetProperty(long propertyID, VARIANTARG * pVarArg);
	int  SLAPI _SetPropertyW(long propertyID, VARIANTARG * pVarArg);
	int  SLAPI _GetProperty(long propertyID, VARIANTARG * pVarArg, int sendParams = 0);
	void SLAPI SetErrCode();
	SString ProgIdent; // Для сообщений об ошибках
	IDispatch   * P_Disp;
	TSArray <DispIDEntry> DispIDAry;
	SArray * P_ParamsAry;
	HRESULT HRes;
};

#define ASSIGN_ID_BY_NAME(p_interface, name) (p_interface ? p_interface->AssignIDByName(#name, name) : 0)

int SLAPI GetExcelCellCoordA1(long row, long col, SString & rBuf);
//
// ComExcelFont
//
class ComExcelFont : public ComDispInterface {
public:
	SLAPI  ComExcelFont();
	SLAPI ~ComExcelFont();
	virtual int SLAPI Init(IDispatch * pIDisp);
	int    SLAPI SetBold(int bold);
	int    SLAPI SetColor(long color);
private:
	enum {
		Bold = 1L,
		Color
	};
};

class ComExcelInterior : public ComDispInterface {
public:
	SLAPI  ComExcelInterior();
	SLAPI ~ComExcelInterior();
	virtual int SLAPI Init(IDispatch * pIDisp);
	int    SLAPI SetColor(long color);
private:
	enum {
		Color = 1L
	};
};
//
//
//
class ComExcelShapes : public ComDispInterface {
public:
	SLAPI  ComExcelShapes();
	SLAPI ~ComExcelShapes();
	virtual int SLAPI Init(IDispatch * pIDisp);

	int SLAPI PutPicture(const char * pPath, RECT * pRect);
private:
	enum {
		AddPicture = 1L,
		Filename,
		Left,
		Top,
		Width,
		Height
	};
};
//
// ComExcelRange
//
class ComExcelRange : public ComDispInterface {
public:
	SLAPI  ComExcelRange();
	SLAPI ~ComExcelRange();
	virtual int SLAPI Init(IDispatch * pIDisp);
	ComExcelFont * SLAPI GetFont();
	ComExcelInterior * SLAPI GetInterior();
	int    SLAPI SetValue(const char * pValue);
	int    SLAPI SetValue(double value);
	int    SLAPI GetValue(SString & rValue);
	int    SLAPI SetBold(int bold);
	int    SLAPI SetColor(long color);
	int    SLAPI SetBgColor(long color);
	int    SLAPI SetWidth(long width);
	int    SLAPI SetHeight(long height);
	int    SLAPI SetFormat(const char * pFormat);
	int    SLAPI GetFormat(SString & rFormat);
	int    SLAPI DoClear();
	int    SLAPI DoMerge();
	ComExcelRange * _Columns();
private:
	enum {
		Value = 1L,
		Item,
		NumberFormat,
		NumberFormatLocal,
		Font,
		ColumnWidth,
		RowHeight,
		Columns,
		Clear,
		Interior, // @v9.8.7
		Merge     // @v9.8.7
	};
};

class ComExcelWorksheet : public ComDispInterface {
public:
	SLAPI  ComExcelWorksheet();
	SLAPI ~ComExcelWorksheet();
	virtual int SLAPI Init(IDispatch * pIDisp);
	int SLAPI _Activate();
	int SLAPI _Delete();
	int SLAPI Print();
	int SLAPI Preview();
	int SLAPI SetName(const char * pName);
	int SLAPI GetName(SString & rName);
	ComExcelRange * SLAPI Cell(long row, long col);
	ComExcelRange * SLAPI GetRange(long luRow, long luCol, long rbRow, long rbCol);
	int SLAPI SetBold(long row, long col, int bold);
	int SLAPI SetColor(long row, long col, COLORREF color);
	int SLAPI SetBgColor(long row, long col, COLORREF color);
	int SLAPI AddColumn(long before, long after, const char * pColumnName);
	int SLAPI DelColumn(long pos);
	int SLAPI PutPicture(const char * pPath, RECT * pRect);
	int SLAPI Format(long row1, long col1, long row2, long col2);
	int SLAPI SetValue(long row, long col, const char * pValue);
	int SLAPI SetValue(long row, long col, double value);
	int SLAPI GetValue(long row, long col, SString & rValue);
	int SLAPI _Clear(long row1, long col1, long row2, long col2);
	int SLAPI _Clear(long col1, long colN);
	ComExcelRange  * SLAPI _Select(long row1, long col1, long row2, long col2);
	ComExcelRange  * SLAPI GetColumn(long pos);
	ComExcelRange  * SLAPI GetRow(long pos);
	ComExcelShapes * SLAPI GetShapes();
	int SLAPI SetColumnWidth(long pos, long width);
	int SLAPI SetColumnFormat(long pos, const char * pFormat);
	int SLAPI SetCellFormat(long row, long col, const char * pFormat);
	int SLAPI GetCellFormat(long row, long col, SString & rFormat);
private:
	enum {
		Activate = 1L,
		Name,
		Cells,
		Columns,
		Rows,
		Range, // @v9.8.7
		Delete,
		Shapes,
		PrintOut,
		PrintPreview
	};
};

class ComExcelWorksheets : public ComDispInterface {
public:
	SLAPI  ComExcelWorksheets();
	SLAPI ~ComExcelWorksheets();
	virtual int SLAPI Init(IDispatch * pIDisp);
	ComExcelWorksheet * SLAPI _Add(long before, long after, const char * pName);
	int    SLAPI Delete(long pos);
	int    SLAPI _Move(long before, long after);
	ComExcelWorksheet * SLAPI Enum(long * pPos);
	ComExcelWorksheet * SLAPI Get(long pos);
	ComExcelWorksheet * SLAPI GetActive();
	int    SLAPI Activate(long pos);
	int    SLAPI GetCount();
private:
	enum {
		Item = 1L,
		Add,
		Move,
		Count,
		Select
	};
};

class ComExcelWorkbook : public ComDispInterface {
public:
	SLAPI  ComExcelWorkbook();
	SLAPI ~ComExcelWorkbook();
	virtual int SLAPI Init(IDispatch * pIDisp);
	int SLAPI _Close();
	int SLAPI _SaveAs(const char * pPath);
	int SLAPI _Save();
	int SLAPI _Activate();
	ComExcelWorksheet  * SLAPI _ActiveSheet();
	ComExcelWorksheet  * SLAPI AddWorksheet(long before, long after, const char * pName);
	ComExcelWorksheet  * SLAPI GetWorksheet(long pos);
	ComExcelWorksheets * SLAPI Get();
private:
	enum {
		WorkSheets = 1L,
		Close,
		SaveAs,
		Save,
		Activate,
		ActiveSheet
	};
};

class ComExcelWorkbooks : public ComDispInterface {
public:
	SLAPI  ComExcelWorkbooks();
	SLAPI ~ComExcelWorkbooks();
	virtual int SLAPI Init(IDispatch * pIDisp);
	ComExcelWorkbook * SLAPI _Add();
	int    SLAPI SaveAs(long pos, const char * pPath);
	ComExcelWorkbook  * SLAPI Enum(long * pPos);
	ComExcelWorkbook  * SLAPI Get(long pos);
	ComExcelWorkbook  * SLAPI _Open(const char * pFileName);
	ComExcelWorksheet * SLAPI GetWorksheet(long bookPos, long sheetPos);
	int    SLAPI Close(long pos);
	long   SLAPI GetCount();
private:
	enum {
		Item = 1L,
		Add,
		Count,
		Open
	};
};

class ComExcelApp : public ComDispInterface {
public:
	SLAPI  ComExcelApp();
	SLAPI ~ComExcelApp();
	int    SLAPI Init();
	ComExcelWorkbooks * SLAPI Get();
	ComExcelWorkbook  * SLAPI AddWkbook();
	int    SLAPI SaveAsWkbook(long pos, const char * pPath);
	int    SLAPI CloseWkbook(long pos);
	ComExcelWorkbook  * SLAPI OpenWkBook(const char * pFileName);
	ComExcelWorksheet * SLAPI GetWorksheet(long bookPos, long sheetPos);
	ComExcelShapes    * SLAPI GetShapes(long bookPos, long sheetPos);
	int SLAPI _DisplayAlerts(int yes);
private:
	enum {
		Workbooks = 1L,
		Quit,
		DisplayAlerts
	};
};

#endif // } __WIN32__
#endif // } __COMDISP_H
