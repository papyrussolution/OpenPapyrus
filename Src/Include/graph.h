// GRAPH.H
// Copyright (c) A.Starodub 2003, 2020
//
#ifndef __GRAPH_H
#define __GRAPH_H
#include <slib.h>

#define GRAPH_COORDNEGX    0x0001
#define GRAPH_COORDNEGZ    0x0004

struct AbcissaInfo {
	char   Buf[24];
	double NextX;
};

struct PPGraphParam {
	PPGraphParam();
	~PPGraphParam();
	void   AllocMem();
	int    copy(const PPGraphParam &aParam);

	enum {
		gKindBasic = 1,
		gKindCol   = 2,
		gKindPie   = 3
	};
	char   Title[48];
	int    Kind;
	long   Flags;
	uint   NumRows;
	SArray * P_Rows;
	SStrCollection Names;
	TYPEID AbcissaType;
	SArray * P_Abcissa;
};

#ifdef __WIN32__
class PPGraphView {
public:
	PPGraphView();
	int    Init(const PPGraphParam *);
	int    DrawDiagram(HDC hdc);
	int    GetPixelInfo(HDC hdc, long x, long y, char * pBuf, int bufLen);
	const  PPGraphParam * GetGraphParam() const { return &GraphParam; }
private:
	int    DrawBasicDiagram(HDC hdc);
	int    DrawColDiagram(HDC hdc);
	int    DrawPieDiagram(HDC hdc);
	int    DrawAxis(HDC hdc);
	int    PrintAbcissa(HDC hdc);
	int    PrintOrdinata(HDC hdc);
	int    PrintLegenda(HDC hdc);
	int    AngleTextPrint(HDC hdc, double x, double y, char * pBuf, int bufLen,
		COLORREF color, char * pFontName, int fontSize, int angle);
	int    TextPrint(HDC, double x1, double y1, char * pBuf,
		int bufLen, COLORREF, char * pFontName, int fontSize);
	int    DrawCvtLine(HDC, double x1, double y1, double x2, double y2, COLORREF, int lineThickness);
	int    DrawLine(HDC, double x1, double y1, double x2, double y2, COLORREF, int lineThickness);
	int    DrawCvtRectangle(HDC, double x1, double y1, double x2,
		double y2, COLORREF, int lineThickness);
	int    DrawRectangle(HDC, double x1, double y1, double x2,
		double y2, COLORREF, int lineThickness);
	int    DrawCvtPie(HDC, double leftX, double topY, double rightX, double bottomY, double xRadial1,
		double yRadial1, double xRadial2, double yRaidal2 , COLORREF, int lineThickness);
	int    DrawPie(HDC, double leftX, double topY, double rightX, double bottomY, double xRadial1, 
		double yRadial1, double xRadial2, double yRaidal2 , COLORREF, int lineThickness);
	int    AbcissaOpsTypeBasic(uint idx, AbcissaInfo * pInfo, long action);
	int    AbcissaOpsColInit();
	int    AbcissaOpsTypeCol(uint idx, AbcissaInfo * pInfo, long action);
	int    AbcissaOps(uint idx, AbcissaInfo * pInfo, long action);
	int    CalcScaleY();
	int    CalcCoordOnCircle(double x, double y, double sqrSideLen, double * pCircleX, double * pCircleY, int * pAngle);
	double ScaleX(double x) const;
	double ScaleY(double y) const;
	double OffsetX(double x) const;
	double OffsetY(double y) const;
	long   GetFirstX() const;
	long   GetTopY() const;
	long   GetLastX() const;
	long   GetBottomY() const;

	PPGraphParam GraphParam;
	long   RemsCount;
	double YScale;
	double MaxY;
	double MinY;
	double AbcRemsDelta;
	double AbcRemsDeltaX;
	double AbcValsDeltaX;
};

class PPGraphWindow {
public:
	PPGraphWindow();
	~PPGraphWindow();
	int    Init(const PPGraphParam * pParam);
	static BOOL  CALLBACK GraphWndProc(HWND , UINT , WPARAM , LPARAM);
	int    Browse();
	void   OnPaint(HWND hWnd);
	int    GetPixelInfo(HWND hWnd, long x, long y, char * pBuf, int bufLen);
private:
	PPGraphView * P_View;
	HWND   hWnd;
	HWND   hWndTT;
};
#else // __WIN32__
class PPGraphWindow {
};
#endif

class PPGraph {
public:
	PPGraph();
	~PPGraph();
	int    Init(const PPGraphParam * pParam);
	int    Browse();
	int    Print();
private:
	PPGraphWindow * P_Window;
};

#endif // __GRAPH_H
