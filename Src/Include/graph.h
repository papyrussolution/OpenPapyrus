// GRAPH.H
// Copyright (c) A.Starodub 2003
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
	SLAPI  PPGraphParam();
	SLAPI ~PPGraphParam();
	void   SLAPI AllocMem();
	int    SLAPI copy(const PPGraphParam &aParam);

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
	SLAPI  PPGraphView();
	int    SLAPI Init(const PPGraphParam *);
	int    SLAPI DrawDiagram(HDC hdc);
	int    SLAPI GetPixelInfo(HDC hdc, long x, long y, char * pBuf, int bufLen);
	const  PPGraphParam * SLAPI GetGraphParam() const { return &GraphParam; }
private:
	int    SLAPI DrawBasicDiagram(HDC hdc);
	int    SLAPI DrawColDiagram(HDC hdc);
	int    SLAPI DrawPieDiagram(HDC hdc);
	int    SLAPI DrawAxis(HDC hdc);
	int    SLAPI PrintAbcissa(HDC hdc);
	int    SLAPI PrintOrdinata(HDC hdc);
	int    SLAPI PrintLegenda(HDC hdc);
	int    SLAPI AngleTextPrint(HDC hdc, double x, double y, char * pBuf, int bufLen,
		COLORREF color, char * pFontName, int fontSize, int angle);
	int    SLAPI TextPrint(HDC, double x1, double y1, char * pBuf,
		int bufLen, COLORREF, char * pFontName, int fontSize);
	int    SLAPI DrawCvtLine(HDC, double x1, double y1, double x2, double y2, COLORREF, int lineThickness);
	int    SLAPI DrawLine(HDC, double x1, double y1, double x2, double y2, COLORREF, int lineThickness);
	int    SLAPI DrawCvtRectangle(HDC, double x1, double y1, double x2,
		double y2, COLORREF, int lineThickness);
	int    SLAPI DrawRectangle(HDC, double x1, double y1, double x2,
		double y2, COLORREF, int lineThickness);
	int    SLAPI DrawCvtPie(HDC, double leftX, double topY, double rightX, double bottomY, double xRadial1,
		double yRadial1, double xRadial2, double yRaidal2 , COLORREF, int lineThickness);
	int    SLAPI DrawPie(HDC, double leftX, double topY, double rightX, double bottomY, double xRadial1,
		double yRadial1, double xRadial2, double yRaidal2 , COLORREF, int lineThickness);
	int    SLAPI AbcissaOpsTypeBasic(uint idx, AbcissaInfo * pInfo, long action);
	int    SLAPI AbcissaOpsColInit();
	int    SLAPI AbcissaOpsTypeCol(uint idx, AbcissaInfo * pInfo, long action);
	int    SLAPI AbcissaOps(uint idx, AbcissaInfo * pInfo, long action);
	int    SLAPI CalcScaleY();
	int    SLAPI CalcCoordOnCircle(double x, double y, double sqrSideLen,
		double * pCircleX, double * pCircleY, int * pAngle);
	double SLAPI ScaleX(double x) const;
	double SLAPI ScaleY(double y) const;
	double SLAPI OffsetX(double x) const;
	double SLAPI OffsetY(double y) const;
	long   SLAPI GetFirstX() const;
	long   SLAPI GetTopY() const;
	long   SLAPI GetLastX() const;
	long   SLAPI GetBottomY() const;

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
	SLAPI  PPGraphWindow();
	SLAPI ~PPGraphWindow();
	int    SLAPI Init(const PPGraphParam * pParam);
	static BOOL  CALLBACK GraphWndProc(HWND , UINT , WPARAM , LPARAM);
	int    SLAPI Browse();
	void   SLAPI OnPaint(HWND hWnd);
	int    SLAPI GetPixelInfo(HWND hWnd, long x, long y, char * pBuf, int bufLen);
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
	SLAPI  PPGraph();
	SLAPI ~PPGraph();
	int    SLAPI Init(const PPGraphParam * pParam);
	int    SLAPI Browse();
	int    SLAPI Print();
private:
	PPGraphWindow * P_Window;
};

#endif // __GRAPH_H
