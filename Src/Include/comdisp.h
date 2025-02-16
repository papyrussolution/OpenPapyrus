// COMDISP.H
// Copyright (c) V.Nasonov, A.Starodub 2003, 2004, 2006, 2007, 2013, 2017, 2018, 2019, 2020, 2021, 2023, 2024, 2025
// @codepage UTF-8
//
#ifndef __COMDISP_H // {
#define __COMDISP_H
//
//   Интерфейс IDispatch для работы с COM-приложениями (режим InProcServer, LocalServer) (only WIN32)
//
struct DispIDEntry { // @flat
	int    ID; // @v11.6.8 long-->int
	DISPID DispID;
	char   Name[64];
};

class ComDispInterface {
public:
	ComDispInterface();
	~ComDispInterface();
	int  Init(const char * pProgID, int inProcServer = 1);
	int  Init(const wchar_t * pProgID, int inProcServer = 1);
	//
	// Descr: Виртуальная функция инициализации интерфейса.
	//   Порожденные классы реализуют в нем инициализацию конкретных методов и свойств.
	//
	virtual int Init(IDispatch * pIDisp);
	int  AssignIDByName(const char * pName, long id);
	SString & GetNameByID(int id, SString & rName) const;
	int  GetProperty(int propertyID, int    * pBuf);
	int  GetProperty(int propertyID, long   * pBuf);
	int  GetProperty(int propertyID, double * pBuf);
	int  GetProperty(int propertyID, bool   * pBuf);
	//
	// Память под класс pIDisp выделяется внутри функции
	//
	int  GetProperty(int propertyID, ComDispInterface * pDisp);
	//
	//   Возвращаемая строка - codepage windows-1251
	//
	int  GetProperty(int propertyID, char   * pBuf, size_t bufLen);
	int  SetProperty(int propertyID, int      iVal, int writeOnly = 0);
	int  SetProperty(int propertyID, long     lVal, int writeOnly = 0);
	int  SetProperty(int propertyID, double   dVal, int writeOnly = 0);
	int  SetProperty(int propertyID, bool     bVal, int writeOnly = 0);
	int  SetProperty(int propertyID, LDATE    dtVal, int writeOnly = 0);
	int  SetPropertyByParams(int propertyID);
	//
	// ARG(pStrVal IN) Передаваемая строка - codepage windows-1251
	//
	int  SetProperty(int propertyID, const char * pStrVal, int writeOnly = 0);
	int  SetParam(int    iVal);
	int  SetParam(long   lVal);
	int  SetParam(double dVal);
	//
	// Передаваемая строка - codepage windows-1251 по умолчанию
	//
	int  SetParam(const char * pStrVal, int codepage = 1251);
	int  SetParam(ComDispInterface * pParam);
	int  CallMethod(int methodID, VARIANTARG * pVarArg = NULL);
	int  CallMethod(int methodID, ComDispInterface * pDisp);
	IDispatch * GetDisp() { return P_Disp; }
private:
	const  DispIDEntry * FASTCALL GetDispIDEntry(long entryId) const;
	void   ClearParams();
	int    _SetParam(VARIANTARG * pVarArg);
	int    _SetProperty(int propertyID, VARIANTARG * pVarArg);
	int    _SetPropertyW(int propertyID, VARIANTARG * pVarArg);
	int    _GetProperty(int propertyID, VARIANTARG * pVarArg, int sendParams = 0);
	void   SetErrCode();
	SString ProgIdent; // Для сообщений об ошибках
	IDispatch * P_Disp;
	TSVector <DispIDEntry> DispIDAry;
	SArray * P_ParamsAry;
	HRESULT HRes;
};

// @v10.2.2 #define ASSIGN_ID_BY_NAME(p_interface, name) (p_interface ? p_interface->AssignIDByName(#name, name) : 0)
#define ASSIGN_ID_BY_NAME(p_interface, name) p_interface->AssignIDByName(#name, name) // @v10.2.2 Менее безопасный вариант, зато более быстрый и экономный.
	// Вызывающая функция должна сама проверить условие (p_interface != 0)

int GetExcelCellCoordA1(long row, long col, SString & rBuf);
//
// ComExcelFont
//
class ComExcelFont : public ComDispInterface {
public:
	ComExcelFont();
	~ComExcelFont();
	virtual int Init(IDispatch * pIDisp);
	int    SetBold(int bold);
	int    SetColor(long color);
private:
	enum {
		Bold = 1L,
		Color
	};
};

class ComExcelInterior : public ComDispInterface {
public:
	ComExcelInterior();
	~ComExcelInterior();
	virtual int Init(IDispatch * pIDisp);
	int    SetColor(long color);
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
	ComExcelShapes();
	~ComExcelShapes();
	virtual int Init(IDispatch * pIDisp);
	int    PutPicture(const char * pPath, RECT * pRect);
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
	ComExcelRange();
	~ComExcelRange();
	virtual int Init(IDispatch * pIDisp);
	ComExcelFont * GetFont();
	ComExcelInterior * GetInterior();
	int    SetValue(const char * pValue);
	int    SetValue(double value);
	int    GetValue(SString & rValue);
	int    SetBold(int bold);
	int    SetColor(long color);
	int    SetBgColor(long color);
	int    SetWidth(long width);
	int    SetHeight(long height);
	int    SetFormat(const char * pFormat);
	int    GetFormat(SString & rFormat);
	int    DoClear();
	int    DoMerge();
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
		Interior,
		Merge
	};
};

class ComExcelWorksheet : public ComDispInterface {
public:
	ComExcelWorksheet();
	~ComExcelWorksheet();
	virtual int Init(IDispatch * pIDisp);
	int _Activate();
	int _Delete();
	int Print();
	int Preview();
	int SetName(const char * pName);
	int GetName(SString & rName);
	ComExcelRange * Cell(long row, long col);
	ComExcelRange * GetRange(long luRow, long luCol, long rbRow, long rbCol);
	int SetBold(long row, long col, int bold);
	int SetColor(long row, long col, COLORREF color);
	int SetBgColor(long row, long col, COLORREF color);
	int AddColumn(long before, long after, const char * pColumnName);
	int DelColumn(long pos);
	int PutPicture(const char * pPath, RECT * pRect);
	int Format(long row1, long col1, long row2, long col2);
	int SetValue(long row, long col, const char * pValue);
	int SetValue(long row, long col, double value);
	int GetValue(long row, long col, SString & rValue);
	void _Clear(long row1, long col1, long row2, long col2);
	void _Clear(long col1, long colN);
	ComExcelRange  * _Select(long row1, long col1, long row2, long col2);
	ComExcelRange  * GetColumn(long pos);
	ComExcelRange  * GetRow(long pos);
	ComExcelShapes * GetShapes();
	int SetColumnWidth(long pos, long width);
	int SetColumnFormat(long pos, const char * pFormat);
	int SetCellFormat(long row, long col, const char * pFormat);
	int GetCellFormat(long row, long col, SString & rFormat);
private:
	enum {
		Activate = 1L,
		Name,
		Cells,
		Columns,
		Rows,
		Range,
		Delete,
		Shapes,
		PrintOut,
		PrintPreview
	};
};

class ComExcelWorksheets : public ComDispInterface {
public:
	ComExcelWorksheets();
	~ComExcelWorksheets();
	virtual int Init(IDispatch * pIDisp);
	ComExcelWorksheet * _Add(long before, long after, const char * pName);
	int    Delete(long pos);
	int    _Move(long before, long after);
	ComExcelWorksheet * Enum(long * pPos);
	ComExcelWorksheet * Get(long pos);
	ComExcelWorksheet * GetActive();
	int    Activate(long pos);
	int    GetCount();
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
	ComExcelWorkbook();
	~ComExcelWorkbook();
	virtual int Init(IDispatch * pIDisp);
	int _Close();
	int _SaveAs(const char * pPath);
	int _Save();
	int _Activate();
	ComExcelWorksheet  * _ActiveSheet();
	ComExcelWorksheet  * AddWorksheet(long before, long after, const char * pName);
	ComExcelWorksheet  * GetWorksheet(long pos);
	ComExcelWorksheets * Get();
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
	ComExcelWorkbooks();
	~ComExcelWorkbooks();
	virtual int Init(IDispatch * pIDisp);
	ComExcelWorkbook * _Add();
	int    SaveAs(long pos, const char * pPath);
	ComExcelWorkbook  * Enum(long * pPos);
	ComExcelWorkbook  * Get(long pos);
	ComExcelWorkbook  * _Open(const char * pFileName);
	ComExcelWorksheet * GetWorksheet(long bookPos, long sheetPos);
	int    Close(long pos);
	long   GetCount();
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
	ComExcelApp();
	~ComExcelApp();
	int    Init();
	ComExcelWorkbooks * Get();
	ComExcelWorkbook  * AddWkbook();
	int    SaveAsWkbook(long pos, const char * pPath);
	int    CloseWkbook(long pos);
	ComExcelWorkbook  * OpenWkBook(const char * pFileName);
	ComExcelWorksheet * GetWorksheet(long bookPos, long sheetPos);
	ComExcelShapes    * GetShapes(long bookPos, long sheetPos);
	int _DisplayAlerts(int yes);
private:
	enum {
		Workbooks = 1L,
		Quit,
		DisplayAlerts
	};
};

#endif // } __COMDISP_H
