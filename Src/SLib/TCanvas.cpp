// TCANVAS.CPP
// Copyright (c) A.Sobolev 2007, 2008, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017
//
#include <slib.h>
#include <tv.h>
#pragma hdrstop
#include <WinUser.h>
//
#define CAIRO_WIN32_STATIC_BUILD 1
#include <cairo\1_14_2\cairo.h>
#include <cairo\1_14_2\cairo-win32.h>
//
//
//
typedef BOOL (STDAPICALLTYPE * ProcDllSetLayeredWindowAttributes)(HWND hwnd, COLORREF crKey, BYTE bAlpha, DWORD dwFlags);

int SetWindowTransparent(HWND hWnd, int transparent /*0..100*/)
{
	int    ok = 0;
	long   exstyle = TView::GetWindowExStyle(hWnd);
	SDynLibrary lib("user32.dll");
	ProcDllSetLayeredWindowAttributes proc = (ProcDllSetLayeredWindowAttributes)lib.GetProcAddr("SetLayeredWindowAttributes");
	if(proc) {
		TView::SetWindowProp(hWnd, GWL_EXSTYLE, (exstyle | WS_EX_LAYERED));
   		proc(hWnd, 0 , (255 * transparent) / 100, LWA_ALPHA);
		ok = 1;
	}
	return ok;
}
//
//
//
TCanvas2::ColorReplacement::ColorReplacement()
{
	Reset();
}

void TCanvas2::ColorReplacement::Reset()
{
	Flags = 0;
	Original.Set(0);
	Replacement.Set(0);
}

void TCanvas2::ColorReplacement::Set(SColor org, SColor rpl)
{
	if(org != rpl) {
		Original = org;
		Replacement = rpl;
		Flags |= fActive;
	}
	else
		Reset();
}
//
//
//
void TCanvas2::Init()
{
	S.HCtx = 0;
	P_Cr = 0;
	P_CrS = 0;
	Flags = 0;
	P_SelectedFont = 0;
}

SLAPI TCanvas2::TCanvas2(SPaintToolBox & rTb, HDC hDc) : GdiObjStack(sizeof(HGDIOBJ)), R_Tb(rTb)
{
	Init();
	S.HCtx = (uint32)hDc;
	Flags |= fOuterSurface;
	P_CrS = cairo_win32_surface_create(hDc);
	assert(P_CrS);
	P_Cr = cairo_create(P_CrS);
	// cairo_set_antialias(P_Cr, CAIRO_ANTIALIAS_BEST); // @v9.1.5 @construction
	assert(P_Cr);
}

SLAPI TCanvas2::TCanvas2(SPaintToolBox & rTb, SImageBuffer & rBuf) : GdiObjStack(sizeof(HGDIOBJ)), R_Tb(rTb)
{
	Init();
	S.P_Img = &rBuf;
	Flags |= fOuterSurface;
	P_CrS = (cairo_surface_t *)S.P_Img->CreateSurface(dsysCairo);
	assert(P_CrS);
	P_Cr = cairo_create(P_CrS);
	assert(P_Cr);
}

SLAPI TCanvas2::~TCanvas2()
{
	SelectFont(0);
	if(P_CrS)
		cairo_surface_destroy(P_CrS);
	if(P_Cr)
		cairo_destroy(P_Cr);
	if(!(Flags & fOuterSurface)) {
		if(S.P_Img) {
			ZDELETE(S.P_Img);
		}
		else if(S.HCtx) {
			DeleteDC((HDC)S.HCtx);
			S.HCtx = 0;
		}
	}
}

TCanvas2::operator HDC() const
{
	return (HDC)S.HCtx;
}

TCanvas2::operator SDrawContext () const
{
	return SDrawContext(P_Cr);
}

TCanvas2::operator const SImageBuffer * () const
{
	return S.P_Img;
}

int TCanvas2::GetCapability(Capability * pCaps) const
{
	int    ok = 1;
	Capability c;
	MEMSZERO(c);
	HDC h_dc = (HDC)*this;
	if(h_dc) {
		const float  mm_h = (float)GetDeviceCaps(h_dc, HORZSIZE);
		const float  mm_v = (float)GetDeviceCaps(h_dc, VERTSIZE);
		const int    pt_h = GetDeviceCaps(h_dc, HORZRES);
		const int    pt_v = GetDeviceCaps(h_dc, VERTRES);
		const float  pt_per_inch_h = (float)GetDeviceCaps(h_dc, LOGPIXELSX);
		const float  pt_per_inch_v = (float)GetDeviceCaps(h_dc, LOGPIXELSY);
		c.SizePt.Set(pt_h, pt_v);
		c.SizeMm.Set(mm_h, mm_v);
		c.PtPerInch.Set(pt_per_inch_h, pt_per_inch_v);
	}
	else
		ok = 0;
	ASSIGN_PTR(pCaps, c);
	return ok;
}

int FASTCALL TCanvas2::SelectObjectAndPush(HGDIOBJ hObj)
{
	int    ok = 1;
	HDC    h_dc = (HDC)*this;
	HGDIOBJ h_old_obj = ::SelectObject(h_dc, hObj);
	if(h_old_obj)
		GdiObjStack.push(&h_old_obj);
	else
		ok = 0;
	return ok;
}

int SLAPI TCanvas2::PopObject()
{
	HDC    h_dc = (HDC)*this;
	HGDIOBJ * p_h_obj = (HGDIOBJ *)GdiObjStack.pop();
	assert(p_h_obj);
	return p_h_obj ? (int)::SelectObject(h_dc, *p_h_obj) : 0;
}

int FASTCALL TCanvas2::PopObjectN(uint c)
{
	HDC    h_dc = (HDC)*this;
	for(uint i = 0; i < c; i++) {
		HGDIOBJ * p_h_obj = (HGDIOBJ *)GdiObjStack.pop();
		assert(p_h_obj);
		if(!p_h_obj)
			return 0;
		else
			::SelectObject(h_dc, *p_h_obj);
	}
	return 1;
}

FPoint TCanvas2::GetCurPoint()
{
	double x, y;
	cairo_get_current_point(P_Cr, &x, &y);
	FPoint p((float)x, (float)y);
	return p;
}

void FASTCALL TCanvas2::MoveTo(FPoint p)
{
	cairo_move_to(P_Cr, p.X, p.Y);
}

void FASTCALL TCanvas2::Line(FPoint p)
{
	cairo_line_to(P_Cr, p.X, p.Y);
}

void FASTCALL TCanvas2::LineV(float yTo)
{
	FPoint p = GetCurPoint();
	p.Y = yTo;
	Line(p);
}

void FASTCALL TCanvas2::LineH(float xTo)
{
	FPoint p = GetCurPoint();
	p.X = xTo;
	Line(p);
}

void FASTCALL TCanvas2::Rect(const TRect & rRect)
{
	FRect r(rRect);
	Rect(r);
}

void FASTCALL TCanvas2::Rect(const FRect & rRect)
{
	FPoint p;
	MoveTo(rRect.a);
	Line(p.Set(rRect.b.X, rRect.a.Y));
	Line(rRect.b);
	Line(p.Set(rRect.a.X, rRect.b.Y));
	ClosePath();
}

void SLAPI TCanvas2::RoundRect(const FRect & rRect, float radius)
{
	/*
	cairo_new_sub_path (cr);
	cairo_arc (cr, x + width - radius, y + radius,          radius, -90 * degrees, 0 * degrees);
	cairo_arc (cr, x + width - radius, y + height - radius, radius, 0 * degrees, 90 * degrees);
	cairo_arc (cr, x + radius,         y + height - radius, radius, 90 * degrees, 180 * degrees);
	cairo_arc (cr, x + radius,         y + radius,          radius, 180 * degrees, 270 * degrees);
	cairo_close_path (cr);
	*/
	FPoint p;
	const float degrees = SMathConst::Pi_f / 180.0f;
	Arc(p.Set(rRect.b.X - radius, rRect.a.Y + radius), radius, -90.0f * degrees, 0.0f);
	Arc(p.Set(rRect.b.X - radius, rRect.b.Y - radius), radius, 0.0f,             90.0f * degrees);
	Arc(p.Set(rRect.a.X + radius, rRect.b.Y - radius), radius, 90.0f * degrees,  180.0f * degrees);
	Arc(p.Set(rRect.a.X + radius, rRect.a.Y + radius), radius, 180.0f * degrees, 270.0f * degrees);
	ClosePath();
}

#if 0 // @sample(ReactOS) {
//
// Rounds a floating point number to integer. The world-to-viewport
// transformation process is done in floating point internally. This function
// is then used to round these coordinates to integer values.
//
static __inline INT GDI_ROUND(FLOAT val)
{
	return (int)floor(val + 0.5);
}
//
// PATH_ScaleNormalizedPoint
//
// Scales a normalized point (x, y) with respect to the box whose corners are
// passed in "corners". The point is stored in "*pPoint". The normalized
// coordinates (-1.0, -1.0) correspond to corners[0], the coordinates
// (1.0, 1.0) correspond to corners[1].
//
void FASTCALL PATH_ScaleNormalizedPoint(FPoint corners[], double x, double y, POINT * pPoint)
{
    assert(corners);
    assert(pPoint);
    pPoint->x = GDI_ROUND((double)corners[0].X + (double)(corners[1].X - corners[0].X) * 0.5 * (x + 1.0));
    pPoint->y = GDI_ROUND((double)corners[0].Y + (double)(corners[1].Y - corners[0].Y) * 0.5 * (y + 1.0));
}

/* PATH_AddEntry
 *
 * Adds an entry to the path. For "flags", pass either PT_MOVETO, PT_LINETO
 * or PT_BEZIERTO, optionally ORed with PT_CLOSEFIGURE. Returns TRUE if
 * successful, FALSE otherwise (e.g. if not enough memory was available).
 */
BOOL FASTCALL PATH_AddEntry(PPATH pPath, const POINT * pPoint, BYTE flags)
{
    assert(pPath != NULL);
    /* FIXME: If newStroke is true, perhaps we want to check that we're
     * getting a PT_MOVETO
     */
    /* Check that path is open */
    if(pPath->state != PATH_Open)
        return FALSE;
    /* Reserve enough memory for an extra path entry */
    if(!PATH_ReserveEntries(pPath, pPath->numEntriesUsed + 1))
        return FALSE;
    /* Store information in path entry */
    pPath->pPoints[pPath->numEntriesUsed] = *pPoint;
    pPath->pFlags[pPath->numEntriesUsed] = flags;
    /* If this is PT_CLOSEFIGURE, we have to start a new stroke next time */
    if((flags & PT_CLOSEFIGURE) == PT_CLOSEFIGURE)
        pPath->newStroke = TRUE;
    /* Increment entry count */
    pPath->numEntriesUsed++;
    return TRUE;
}
//
// PATH_DoArcPart
//
// Creates a Bezier spline that corresponds to part of an arc and appends the
// corresponding points to the path. The start and end angles are passed in
// "angleStart" and "angleEnd"; these angles should span a quarter circle
// at most. If "startEntryType" is non-zero, an entry of that type for the first
// control point is added to the path; otherwise, it is assumed that the current
// position is equal to the first control point.
//
BOOL FASTCALL PATH_DoArcPart(PPATH pPath, FPoint corners[], double angleStart, double angleEnd, BYTE startEntryType)
{
    double halfAngle, a;
    double xNorm[4], yNorm[4];
    POINT  point;
    int i;
    assert(fabs(angleEnd - angleStart) <= M_PI_2);
    // FIXME: Is there an easier way of computing this?
    // Compute control points
    halfAngle = (angleEnd - angleStart) / 2.0;
    if(fabs(halfAngle) > 1e-8) {
        a = 4.0 / 3.0 * (1 - cos(halfAngle)) / sin(halfAngle);
        xNorm[0] = cos(angleStart);
        yNorm[0] = sin(angleStart);
        xNorm[1] = xNorm[0] - a * yNorm[0];
        yNorm[1] = yNorm[0] + a * xNorm[0];
        xNorm[3] = cos(angleEnd);
        yNorm[3] = sin(angleEnd);
        xNorm[2] = xNorm[3] + a * yNorm[3];
        yNorm[2] = yNorm[3] - a * xNorm[3];
    }
	else {
        for(i = 0; i < 4; i++) {
            xNorm[i] = cos(angleStart);
            yNorm[i] = sin(angleStart);
        }
	}
    // Add starting point to path if desired
    if(startEntryType) {
        PATH_ScaleNormalizedPoint(corners, xNorm[0], yNorm[0], &point);
        if(!PATH_AddEntry(pPath, &point, startEntryType))
            return FALSE;
    }
    // Add remaining control points
    for(i = 1; i < 4; i++) {
        PATH_ScaleNormalizedPoint(corners, xNorm[i], yNorm[i], &point);
        if(!PATH_AddEntry(pPath, &point, PT_BEZIERTO))
            return FALSE;
    }
    return TRUE;
}
#endif // } 0 @sample(ReactOS)


void SLAPI TCanvas2::Arc(FPoint center, float radius, float startAngleRad, float endAngleRad)
{
	if(radius >= 0.0f)
		cairo_arc(P_Cr, center.X, center.Y, radius, startAngleRad, endAngleRad);
	else
		cairo_arc_negative(P_Cr, center.X, center.Y, -radius, startAngleRad, endAngleRad);
}

void SLAPI TCanvas2::Bezier(FPoint middle1, FPoint middle2, FPoint end)
{
	cairo_curve_to(P_Cr, middle1.X, middle1.Y, middle2.X, middle2.Y, end.X, end.Y);
}

int SLAPI TCanvas2::Text(const char * pText, int identFont)
{
	int    ok = 1;
	SPaintObj * p_obj = R_Tb.GetObj(identFont);
	if(p_obj && p_obj->GetType() == SPaintObj::tFont) {
		p_obj->CreateInnerHandle(this->operator SDrawContext());
		SPaintObj::Font * p_font = p_obj->GetFont();
		if(p_font) {
			cairo_font_face_t * p_face = (cairo_font_face_t *)*p_font;
			if(p_face)
				cairo_set_font_face(P_Cr, p_face);
			cairo_set_font_size(P_Cr, p_font->Size);
		}
	}
	(TempBuf = pText).ToUtf8();
	cairo_show_text(P_Cr, TempBuf);
	return 1;
}

void SLAPI TCanvas2::ClosePath()
{
	cairo_close_path(P_Cr);
}

void SLAPI TCanvas2::SubPath()
{
	cairo_new_sub_path(P_Cr);
}

void FASTCALL TCanvas2::GetClipExtents(FRect & rR)
{
	double x1, y1, x2, y2;
	cairo_clip_extents(P_Cr, &x1, &y1, &x2, &y2);
	rR.a.Set((float)x1, (float)y1);
	rR.b.Set((float)x2, (float)y2);
}

int FASTCALL TCanvas2::SetCairoColor(SColor c)
{
	if(P_Cr) {
		if(ClrRpl.Flags & ClrRpl.fActive && c == ClrRpl.Original)
			c = ClrRpl.Replacement;
		cairo_set_source_rgba(P_Cr, c.RedF(), c.GreenF(), c.BlueF(), c.AlphaF());
		return 1;
	}
	else
		return 0;
}

int SLAPI TCanvas2::Helper_SelectPen(SPaintToolBox * pTb, int penId)
{
	int    ok = 1;
	int    r = 0;
	SPaintToolBox * p_tb = NZOR(pTb, &R_Tb);
	SPaintObj * p_obj = p_tb->GetObj(penId);
	if(p_obj) {
		if(p_obj->GetType() == SPaintObj::tPen) {
			p_obj->CreateInnerHandle(this->operator SDrawContext());
			SPaintObj::Pen * p_pen = p_obj->GetPen();
			if(p_pen) {
				SetCairoColor(p_pen->C);
				if(p_pen->S == SPaintObj::psNull) {
					cairo_set_line_width(P_Cr, 0.0);
				}
				else {
					double dashed[8];
					cairo_set_line_width(P_Cr, p_pen->W);
					if(p_pen->S == SPaintObj::psSolid) {
						cairo_set_dash(P_Cr, dashed, 0, 0);
					}
					else if(p_pen->S == SPaintObj::psDash) {
						dashed[0] = 3.0;
						dashed[1] = 2.0;
						cairo_set_dash(P_Cr, dashed, 2, 0);
					}
					else if(p_pen->S == SPaintObj::psDot) {
						dashed[0] = 1.0;
						dashed[1] = 2.0;
						cairo_set_dash(P_Cr, dashed, 2, 0);
					}
					else if(p_pen->S == SPaintObj::psDashDot) {
						dashed[0] = 4.0;
						dashed[1] = 1.0;
						dashed[2] = 1.0;
						dashed[3] = 1.0;
						cairo_set_dash(P_Cr, dashed, 4, 0);
					}
					else if(p_pen->S == SPaintObj::psDashDotDot) {
						dashed[0] = 4.0;
						dashed[1] = 1.0;
						dashed[2] = 1.0;
						dashed[3] = 1.0;
						dashed[4] = 1.0;
						dashed[5] = 1.0;
						cairo_set_dash(P_Cr, dashed, 6, 0);
					}
				}
				r = 1;
			}
		}
		else if(p_obj->GetType() == SPaintObj::tColor) {
			SetCairoColor(*p_obj);
			cairo_set_line_width(P_Cr, 1.0);
			cairo_set_dash(P_Cr, 0, 0, 0);
			r = 1;
		}
	}
	if(!r) {
		SColor def_color;
		SetCairoColor(def_color);
		cairo_set_line_width(P_Cr, 1.0);
		cairo_set_dash(P_Cr, 0, 0, 0);
	}
	return ok;
}

int FASTCALL TCanvas2::Implement_Stroke(int preserve)
{
	int    ok = 1;
	if(Flags & fScopeRecording) {
		double x1, y1, x2, y2;
		cairo_stroke_extents(P_Cr, &x1, &y1, &x2, &y2);
		cairo_user_to_device(P_Cr, &x1, &y1);
		cairo_user_to_device(P_Cr, &x2, &y2);
		TRect rc((int)floor(x1), (int)floor(y1), (int)ceil(x2), (int)ceil(y2));
		Scope.Add(rc, SCOMBINE_OR);
	}
	if(preserve)
		cairo_stroke_preserve(P_Cr);
	else
		cairo_stroke(P_Cr);
	return ok;
}

int SLAPI TCanvas2::Implement_Stroke(SPaintToolBox * pTb, int paintObjIdent, int preserve)
{
	return Helper_SelectPen(pTb, paintObjIdent) ?  Implement_Stroke(preserve) : 0;
}

TCanvas2::PatternWrapper::PatternWrapper()
{
	P = 0;
}

TCanvas2::PatternWrapper::~PatternWrapper()
{
	if(P)
		cairo_pattern_destroy((cairo_pattern_t *)P);
}

int SLAPI TCanvas2::Helper_SelectBrush(SPaintToolBox * pTb, int brushId, PatternWrapper & rPw)
{
	int    ok = 1;
	int    r = 0;
	cairo_pattern_t * p_pattern = 0;
	SPaintToolBox * p_tb = NZOR(pTb, &R_Tb);
	SPaintObj * p_obj = p_tb->GetObj(brushId);
	if(p_obj) {
		if(p_obj->GetType() == SPaintObj::tBrush) {
			p_obj->CreateInnerHandle(this->operator SDrawContext());
			SPaintObj::Brush * p_brush = p_obj->GetBrush();
			if(p_brush) {
				if(p_brush->IdPattern) {
					SPaintObj * p_pat_obj = p_tb->GetObj(p_brush->IdPattern);
					if(p_pat_obj && p_pat_obj->CreateInnerHandle(this->operator SDrawContext())) {
						if(p_pat_obj->GetType() == SPaintObj::tGradient) {
							SPaintObj::Gradient * p_grad = p_pat_obj->GetGradient();
							if(p_grad) {
								int    pct;
								if(p_grad->Kind == SPaintObj::Gradient::kLinear) {
									float x1 = p_grad->GetLinearCoord(SPaintObj::Gradient::lcX1, &pct);
									float y1 = p_grad->GetLinearCoord(SPaintObj::Gradient::lcY1, &pct);
									float x2 = p_grad->GetLinearCoord(SPaintObj::Gradient::lcX2, &pct);
									float y2 = p_grad->GetLinearCoord(SPaintObj::Gradient::lcY2, &pct);
									p_pattern = cairo_pattern_create_linear(x1, y1, x2, y2);
								}
								else if(p_grad->Kind == SPaintObj::Gradient::kRadial) {
									float cx = p_grad->GetRadialCoord(SPaintObj::Gradient::rcCX, &pct);
									float cy = p_grad->GetRadialCoord(SPaintObj::Gradient::rcCY, &pct);
									float r0 = p_grad->GetRadialCoord(SPaintObj::Gradient::rcR, &pct);
									float fx = p_grad->GetRadialCoord(SPaintObj::Gradient::rcFX, &pct);
									float fy = p_grad->GetRadialCoord(SPaintObj::Gradient::rcFY, &pct);
									float fr = p_grad->GetRadialCoord(SPaintObj::Gradient::rcFR, &pct);
		    						p_pattern = cairo_pattern_create_radial(fx, fy, fr, cx, cy, r0);
								}
								else if(p_grad->Kind == SPaintObj::Gradient::kConical) {
								}
								if(p_pattern) {
									//
									LMatrix2D mtx, mtx2, mtx3;
									if(p_grad->GetUnits() == SPaintObj::Gradient::uBB) {
		    							double x1, y1, x2, y2;
		    							cairo_fill_extents(P_Cr, &x1, &y1, &x2, &y2);
										mtx = mtx3.InitScale(x2-x1, y2-y1) * mtx2.InitTranslate(x1, x2);
									}
									mtx = (mtx * p_grad->Tf);
									mtx.Invert();
									cairo_pattern_set_matrix(p_pattern, (cairo_matrix_t *)&mtx);
									//
									for(uint i = 0; i < p_grad->GetStopCount(); i++) {
										const SPaintObj::Gradient::Stop * p_stop = p_grad->GetStop(i);
										if(p_stop) {
											SColor c = p_stop->C;
											c.PremultiplyAlpha();
											cairo_pattern_add_color_stop_rgba(p_pattern, p_stop->Offs, c.RedF(), c.GreenF(), c.BlueF(), c.AlphaF());
										}
									}
									switch(p_grad->Spread) {
	    								case SPaintObj::Gradient::sRepeat:
											cairo_pattern_set_extend(p_pattern, CAIRO_EXTEND_REPEAT);
											break;
	    								case SPaintObj::Gradient::sReflect:
											cairo_pattern_set_extend(p_pattern, CAIRO_EXTEND_REFLECT);
											break;
	    								default:
											cairo_pattern_set_extend(p_pattern, CAIRO_EXTEND_PAD);
											break;
									}
									cairo_pattern_set_filter(p_pattern, CAIRO_FILTER_BILINEAR);
									cairo_set_source(P_Cr, p_pattern);
									r = 1;
								}
							}
						}
					}
				}
				if(!r)
					SetCairoColor(p_brush->C);
				r = 1;
			}
		}
		else if(p_obj->GetType() == SPaintObj::tColor) {
			SetCairoColor(*p_obj);
			r = 1;
		}
	}
	rPw.P = p_pattern;
	return ok;
}

int SLAPI TCanvas2::Implement_Fill(SPaintToolBox * pTb, int paintObjIdent, int preserve)
{
	int    ok = 1;
	PatternWrapper pw;
	if(Helper_SelectBrush(pTb, paintObjIdent, pw)) {
		if(Flags & fScopeRecording) {
			double x1, y1, x2, y2;
			cairo_fill_extents(P_Cr, &x1, &y1, &x2, &y2);
			cairo_user_to_device(P_Cr, &x1, &y1);
			cairo_user_to_device(P_Cr, &x2, &y2);
			TRect rc((int)floor(x1), (int)floor(y1), (int)ceil(x2), (int)ceil(y2));
			Scope.Add(rc, SCOMBINE_OR);
		}
		if(preserve)
			cairo_fill_preserve(P_Cr);
		else
			cairo_fill(P_Cr);
	}
	else
		ok = 0;
	return ok;
}

int SLAPI TCanvas2::Fill(SColor c, int preserve)
{
	int    ok = 1;
	SetCairoColor(c);
	if(Flags & fScopeRecording) {
		double x1, y1, x2, y2;
		cairo_fill_extents(P_Cr, &x1, &y1, &x2, &y2);
		cairo_user_to_device(P_Cr, &x1, &y1);
		cairo_user_to_device(P_Cr, &x2, &y2);
		TRect rc((int)floor(x1), (int)floor(y1), (int)ceil(x2), (int)ceil(y2));
		Scope.Add(rc, SCOMBINE_OR);
	}
	if(preserve)
		cairo_fill_preserve(P_Cr);
	else
		cairo_fill(P_Cr);
	return ok;
}

int SLAPI TCanvas2::Stroke(int paintObjIdent, int preserve)
{
	return Implement_Stroke(0, paintObjIdent, preserve);
}

int SLAPI TCanvas2::Fill(int paintObjIdent, int preserve)
{
	return Implement_Fill(0, paintObjIdent, preserve);
}

void SLAPI TCanvas2::SetColorReplacement(SColor original, SColor replacement)
{
	ClrRpl.Set(original, replacement);
}

void SLAPI TCanvas2::ResetColorReplacement()
{
	ClrRpl.Reset();
}

int FASTCALL TCanvas2::SetOperator(int opr)
{
	int    prev = cairo_get_operator(P_Cr);
	cairo_set_operator(P_Cr, (cairo_operator_t)opr);
	return prev;
}

int SLAPI TCanvas2::GetOperator() const
{
	return (int)cairo_get_operator(P_Cr);
}

void FASTCALL TCanvas2::GetTransform(LMatrix2D & rMtx) const
{
	cairo_get_matrix(P_Cr, (cairo_matrix_t *)&rMtx);
}

void FASTCALL TCanvas2::SetTransform(const LMatrix2D & rMtx)
{
	cairo_set_matrix(P_Cr, (cairo_matrix_t *)&rMtx);
}

void FASTCALL TCanvas2::AddTransform(const LMatrix2D & rMtx)
{
	cairo_transform(P_Cr, (cairo_matrix_t *)&rMtx);
}

void SLAPI TCanvas2::PushTransform()
{
	LMatrix2D mtx;
	GetTransform(mtx);
	TmStk.push(mtx);
}

int SLAPI TCanvas2::PopTransform()
{
	LMatrix2D mtx;
	int    ok = TmStk.pop(mtx);
	assert(ok);
	SetTransform(mtx);
	return ok;
}

int SLAPI TCanvas2::BeginScope()
{
	if(Flags & fScopeRecording)
		return 0;
	else {
		Scope.Destroy();
		Flags |= fScopeRecording;
		return 1;
	}
}

int FASTCALL TCanvas2::EndScope(SRegion & rR)
{
	if(Flags & fScopeRecording) {
		rR = Scope;
		Flags &= ~fScopeRecording;
		return 1;
	}
	else
		return 0;
}

int SLAPI TCanvas2::Implement_StrokeAndFill(SPaintToolBox * pTb, int penIdent, int brushIdent)
{
	int    ok = 1;
	if(brushIdent)
		THROW(Implement_Fill(pTb, brushIdent, BIN(penIdent)/*0*/));
	if(penIdent)
		THROW(Implement_Stroke(pTb, penIdent, /*BIN(brushIdent)*/0));
	CATCHZOK
	return ok;
}

void SLAPI TCanvas2::Rect(const TRect & rRect, int penIdent, int brushIdent)
{
	Rect(rRect);
	Implement_StrokeAndFill(0, penIdent, brushIdent);
}

void SLAPI TCanvas2::Rect(const FRect & rRect, int penIdent, int brushIdent)
{
	Rect(rRect);
	Implement_StrokeAndFill(0, penIdent, brushIdent);
}

void SLAPI TCanvas2::RoundRect(const FRect & rRect, float radius, int penIdent, int brushIdent)
{
	RoundRect(rRect, radius);
	Implement_StrokeAndFill(0, penIdent, brushIdent);
}

#if 0 // {

BOOL FASTCALL IntDrawRoundRect( PDC dc, INT Left, INT Top, INT Right, INT Bottom, INT Wellipse, INT Hellipse, PBRUSH pbrushPen)
{
	Rect r;
	int rx, ry; /* Radius in x and y directions */
	int w = pbrushPen->lWidth;
	r = rect( Left, Top, abs(Right-Left), abs(Bottom-Top));
	rx = Wellipse/2;
	ry = Hellipse/2;
	if(Wellipse > r.width) {
		if(Hellipse > r.height) // > W > H
			app_draw_ellipse(dc, r, pbrushPen);
		else { // > W < H
			app_draw_arc(dc, rect( r.x, r.y, r.width - 1, Hellipse - 1), 0, 180, pbrushPen, FALSE);
			app_draw_arc(dc, rect(r.x, Bottom - Hellipse, r.width - 1, Hellipse - 1), 180, 360, pbrushPen, FALSE);
		}
	}
	else if(Hellipse > r.height) { // < W > H
		app_draw_arc(dc, rect(r.x, r.y, Wellipse - 1, r.height - 1), 90, 270, pbrushPen, FALSE);
		app_draw_arc(dc, rect(Right - Wellipse, r.y, Wellipse - 1, r.height - 1), 270, 90, pbrushPen, FALSE);
	}
	else { // < W < H
		app_draw_arc(dc, rect(r.x, r.y, rx+rx, ry+ry), 90, 180, pbrushPen, FALSE);
		app_draw_arc(dc, rect(r.x,r.y+r.height-ry-ry,rx+rx,ry+ry), 180, 270, pbrushPen, FALSE);
		app_draw_arc(dc, rect(r.x+r.width-rx-rx, r.y+r.height-ry-ry, rx+rx, ry+ry), 270, 360, pbrushPen, FALSE);
		app_draw_arc(dc, rect(r.x+r.width-rx-rx,r.y,rx+rx,ry+ry), 0, 90, pbrushPen, FALSE);
	}
	if(Hellipse < r.height) {
		app_fill_rect(dc, rect(r.x, r.y+ry+1, w, r.height-ry-ry), pbrushPen, TRUE);
		app_fill_rect(dc, rect(r.x+r.width-w, r.y+ry+1, w, r.height-ry-ry), pbrushPen, TRUE);
	}
	if(Wellipse < r.width) {
		app_fill_rect(dc, rect(r.x+rx, r.y+r.height-w, r.width-rx-rx, w),
		pbrushPen, TRUE);
		app_fill_rect(dc, rect(r.x+rx, r.y, r.width-rx-rx, w), pbrushPen, TRUE);
	}
	return TRUE;
}

BOOL FASTCALL IntRoundRect(PDC  dc, int  Left, int  Top, int  Right, int  Bottom, int  xCurveDiameter, int  yCurveDiameter)
{
	PDC_ATTR pdcattr;
	PBRUSH   pbrLine, pbrFill;
	RECTL RectBounds;
	LONG PenWidth, PenOrigWidth;
	BOOL ret = TRUE; // Default to success
	BRUSH brushTemp;
	ASSERT ( dc ); // Caller's responsibility to set this up
	if( PATH_IsPathOpen(dc->dclevel) )
		return PATH_RoundRect ( dc, Left, Top, Right, Bottom, xCurveDiameter, yCurveDiameter );
	if((Left == Right) || (Top == Bottom))
		return TRUE;
	xCurveDiameter = max(abs( xCurveDiameter ), 1);
	yCurveDiameter = max(abs( yCurveDiameter ), 1);
	if(Right < Left) {
		INT tmp = Right;
		Right = Left;
		Left = tmp;
	}
	if(Bottom < Top) {
		INT tmp = Bottom;
		Bottom = Top;
		Top = tmp;
	}
	pdcattr = dc->pdcattr;
	if(pdcattr->ulDirty_ & (DIRTY_FILL | DC_BRUSH_DIRTY))
		DC_vUpdateFillBrush(dc);
	if(pdcattr->ulDirty_ & (DIRTY_LINE | DC_PEN_DIRTY))
		DC_vUpdateLineBrush(dc);
	pbrLine = PEN_ShareLockPen(pdcattr->hpen);
	if(!pbrLine) {
		/* Nothing to do, as we don't have a bitmap */
		EngSetLastError(ERROR_INTERNAL_ERROR);
		return FALSE;
	}
	PenOrigWidth = PenWidth = pbrLine->lWidth;
	if(pbrLine->ulPenStyle == PS_NULL)
		PenWidth = 0;
	if(pbrLine->ulPenStyle == PS_INSIDEFRAME) {
		if(2*PenWidth > (Right - Left))
			PenWidth = (Right -Left + 1)/2;
		if(2*PenWidth > (Bottom - Top))
			PenWidth = (Bottom -Top + 1)/2;
		Left   += PenWidth / 2;
		Right  -= (PenWidth - 1) / 2;
		Top    += PenWidth / 2;
		Bottom -= (PenWidth - 1) / 2;
	}
	if(!PenWidth)
		PenWidth = 1;
	pbrLine->lWidth = PenWidth;
	RectBounds.left = Left;
	RectBounds.top = Top;
	RectBounds.right = Right;
	RectBounds.bottom = Bottom;
	IntLPtoDP(dc, (LPPOINT)&RectBounds, 2);
	RectBounds.left   += dc->ptlDCOrig.x;
	RectBounds.top    += dc->ptlDCOrig.y;
	RectBounds.right  += dc->ptlDCOrig.x;
	RectBounds.bottom += dc->ptlDCOrig.y;
	pbrFill = BRUSH_ShareLockBrush(pdcattr->hbrush);
	if(!pbrFill) {
		DPRINT1("FillRound Fail\n");
		EngSetLastError(ERROR_INTERNAL_ERROR);
		ret = FALSE;
	}
	else {
		DC_vPrepareDCsForBlit(dc, &RectBounds, 0, 0);
		RtlCopyMemory(&brushTemp, pbrFill, sizeof(brushTemp));
		brushTemp.ptOrigin.x += RectBounds.left - Left;
		brushTemp.ptOrigin.y += RectBounds.top - Top;
		ret = IntFillRoundRect( dc, RectBounds.left, RectBounds.top, RectBounds.right, RectBounds.bottom, xCurveDiameter, yCurveDiameter, &brushTemp);
		BRUSH_ShareUnlockBrush(pbrFill);
		if(ret) {
			ret = IntDrawRoundRect(dc, RectBounds.left, RectBounds.top, RectBounds.right, RectBounds.bottom, xCurveDiameter, yCurveDiameter, pbrLine);
		}
		DC_vFinishBlit(dc, 0);
	}
	pbrLine->lWidth = PenOrigWidth;
	PEN_ShareUnlockPen(pbrLine);
	return ret;
}
#endif // 0

void SLAPI TCanvas2::LineVert(int x, int yFrom, int yTo)
{
	::MoveToEx((HDC)S.HCtx, x, yFrom, 0);
	::LineTo((HDC)S.HCtx, x, yTo);
}

void SLAPI TCanvas2::LineHorz(int xFrom, int xTo, int y)
{
	::MoveToEx((HDC)S.HCtx, xFrom, y, 0);
	::LineTo((HDC)S.HCtx, xTo, y);
}

TPoint SLAPI TCanvas2::GetTextSize(const char * pStr)
{
	int    len;
	char   zero[16];
	if(pStr)
		len = strlen(pStr);
	else {
		len = 0;
		memzero(zero, sizeof(zero));
		pStr = zero;
	}
	TPoint p;
	SIZE   sz;
	return ::GetTextExtentPoint32((HDC)S.HCtx, pStr, len, &sz) ? p.Set(sz.cx, sz.cy) : p.Set(0, 0);
}

int FASTCALL TCanvas2::SetBkColor(COLORREF c)
{
	COLORREF old_color = ::SetBkColor((HDC)S.HCtx, c);
	return (old_color == CLR_INVALID) ? 0 : 1;
}

int FASTCALL TCanvas2::SetTextColor(COLORREF c)
{
	COLORREF old_color = ::SetTextColor((HDC)S.HCtx, c);
	return (old_color == CLR_INVALID) ? 0 : 1;
}

void SLAPI TCanvas2::SetBkTranparent()
{
	::SetBkMode((HDC)S.HCtx, TRANSPARENT);
}

int SLAPI TCanvas2::_DrawText(const TRect & rRect, const char * pText, uint options)
{
	int    len;
	char   zero[16];
	if(pText)
		len = strlen(pText);
	else {
		len = 0;
		memzero(zero, sizeof(zero));
		pText = zero;
	}
	RECT   rect = rRect;
	return ::DrawText((HDC)S.HCtx, pText, len, &rect, options) ? 1 : 0;
}

int SLAPI TCanvas2::TextOut(TPoint p, const char * pText)
{
	int    len;
	char   zero[16];
	if(pText)
		len = strlen(pText);
	else {
		len = 0;
		memzero(zero, sizeof(zero));
		pText = zero;
	}
	return ::TextOut((HDC)S.HCtx, p.x, p.y, pText, len) ? 1 : 0;
}

TCanvas2::DrawingProcFrame::DrawingProcFrame(TCanvas2 * pCanv, const SDrawFigure * pFig)
{
	P_Canv = pCanv;
	P_Fig = pFig;
	MtxAppl = 0;
	if(P_Fig) {
		if(!P_Fig->GetTransform().IsIdentical()) {
			P_Canv->PushTransform();
			MtxAppl = 1;
			P_Canv->AddTransform(P_Fig->GetTransform());
		}
	}
}

TCanvas2::DrawingProcFrame::~DrawingProcFrame()
{
	if(P_Fig && MtxAppl)
		P_Canv->PopTransform();
}

int FASTCALL TCanvas2::Draw(const SDrawText * pText)
{
	int    ok = 1;
	DrawingProcFrame __frame(this, pText);
	// CATCHZOK
	return ok;
}

#if 0 // @sample {
void CCShapeDrawNode::drawEllipse(const cocos2d::CCPoint &leftTop, const cocos2d::CCPoint &rightBottom, float thick, const ccColor4F &color)
{
    float x_radius = fabs(rightBottom.x - leftTop.x)/2;
    float y_radius = fabs(rightBottom.y - leftTop.y)/2;
    float centerX = leftTop.x < rightBottom.x ? (leftTop.x + x_radius) : (rightBottom.x + x_radius);
    float centerY = leftTop.y < rightBottom.y ? (leftTop.y + y_radius) : (rightBottom.y + y_radius);

    int segs = 500;  //分段越多, 画的越细
    float a = M_PI;
    float dotRadius = thick;
    const float coef = 2.0f * (float)M_PI / segs;
    float radius, distance, angle, pointX, pointY;
    for(int i = 0; i <= segs; ++i)
    {
        radius = i * coef;
        distance = sqrt(pow(sinf(radius) * x_radius, 2) + pow(cosf(radius) * y_radius, 2));
        angle = atan2(sinf(radius) * x_radius, cosf(radius) * y_radius);
        pointX = distance * cosf(angle + a) + centerX;
        pointY = distance * sinf(angle + a) + centerY;

        this->drawDot(ccp(pointX, pointY), dotRadius, color);
    }
}
#endif // } 0 @sample

int FASTCALL TCanvas2::Ellipse(const FRect & rRect)
{
	float  x_radius = fabsf(rRect.Width())/2;
	float  y_radius = fabsf(rRect.Height())/2;
	float  centerX = rRect.a.X < rRect.b.X ? (rRect.a.X + x_radius) : (rRect.b.X + x_radius);
	float  centerY = rRect.a.Y < rRect.b.Y ? (rRect.a.Y + y_radius) : (rRect.b.Y + y_radius);
	const  uint segs = 500;
	//float  a = SMathConst::Pi_f;
	//float  dotRadius = thick;
	const  float coef = 2.0f * SMathConst::Pi_f / segs;
	for(uint i = 0; i <= segs; ++i) {
		float radius = i * coef;
		const float cosrad = cosf(radius);
		const float sinrad = sinf(radius);
		/*
		float distance = sqrt(pow(sinrad * x_radius, 2) + pow(cosrad * y_radius, 2));
		float angle = atan2(sinrad * x_radius, cosrad * y_radius);
		*/
		float distance = (float)sqrt(pow(cosrad * x_radius, 2) + pow(sinrad * y_radius, 2));
		float angle = atan2f(cosrad * x_radius, sinrad * y_radius);
		//
		//FPoint pt(distance * cosf(angle + SMathConst::Pi_f) + centerX, distance * sinf(angle + SMathConst::Pi_f) + centerY);
		FPoint pt(distance * sinf(angle + SMathConst::Pi_f) + centerX, distance * cosf(angle + SMathConst::Pi_f) + centerY);
		if(i == 0)
			MoveTo(pt);
		else {
			Line(pt);
		}
	}
	//ClosePath();
	return 1;
}

int FASTCALL TCanvas2::Draw(const SDrawShape * pShape)
{
	int    ok = 1;
	DrawingProcFrame __frame(this, pShape);
	if(oneof2(pShape->S.GetKind(), SHAPE_CIRCLE, SHAPE_CIRCLEARC)) {
		FShape::CircleArc arc;
		THROW(pShape->S.Get(arc));
		Arc(arc.C, arc.R, arc.Start, arc.End);
	}
	else if(oneof2(pShape->S.GetKind(), SHAPE_ELLIPSE, SHAPE_ELLIPSEARC)) {
		LMatrix2D mtx, mtx2;
		FShape::EllipseArc arc;
		THROW(pShape->S.Get(arc));
		{
			//
			// @todo Ellips() - bad function
			//
			/*
			PushTransform();
			SetTransform(mtx2.InitScale(1.0, arc.R.Ratio()) * mtx.InitTranslate(arc.C));
			FPoint p(arc.R.X, 0.0f);
			MoveTo(p);
			Arc(p.Set(0.0f), arc.R.X, arc.Start, arc.End);
			PopTransform();
			*/
			//FPoint p(arc.C.X, arc.C.Y);
			//MoveTo(p);
			//SetTransform(mtx2.InitScale(1.0, arc.R.Ratio()));

			//Arc(arc.C, arc.R.Y, arc.Start, arc.End);
			{
				FRect er;
				er.a.X = arc.C.X - arc.R.X;
				er.a.Y = arc.C.Y - arc.R.Y;
				er.b.X = arc.C.X + arc.R.X;
				er.b.Y = arc.C.Y + arc.R.Y;
				Ellipse(er);
			}
			//
		}
	}
	else if(pShape->S.GetKind() == SHAPE_ROUNDEDRECT) {
		FShape::RoundedRect rect;
		THROW(pShape->S.Get(rect));
		SETMIN(rect.R.X, rect.Width()/2.0f);
		SETMIN(rect.R.Y, rect.Height()/2.0f);
		if(rect.R.X > 0.0f || rect.R.Y > 0.0f) {
			FPoint p = rect.a;
			MoveTo(p.AddX(rect.R.X));
			p.X += rect.Width();
			Line(p.AddX(rect.R.X));
			Implement_ArcSvg(rect.R, 0.0f, 0, 1, p.AddY(rect.R.Y));
			p.Y += rect.Height();
			Line(p.AddY(-rect.R.Y));
			Implement_ArcSvg(rect.R, 0.0f, 0, 1, p.AddX(-rect.R.X));
			p.X -= rect.Width();
			Line(p.AddX(rect.R.X));
			Implement_ArcSvg(rect.R, 0.0f, 0, 1, p.AddY(-rect.R.Y));
			p.Y -= rect.Height();
			Line(p.AddY(rect.R.Y));
			Implement_ArcSvg(rect.R, 0.0f, 0, 1, p.AddX(rect.R.X));
			ClosePath();
		}
	}
	else {
		FShape::Polygon pg;
		if(pShape->S.Get(pg)) {
			const uint c = pg.GetVertexCount();
			if(c) {
				MoveTo(pg.getPoint(0));
				for(uint i = 1; i < c; i++) {
					Line(pg.getPoint(i*2));
				}
				ClosePath();
			}
		}
		else {
			FShape::Polyline pl;
			if(pShape->S.Get(pl)) {
				const uint c = pl.GetVertexCount();
				if(c) {
					MoveTo(pl.getPoint(0));
					for(uint i = 1; i < c; i++) {
						Line(pl.getPoint(i*2));
					}
				}
			}
			else
				ok = -1;
		}
	}
	if(ok > 0) {
		Implement_StrokeAndFill(pShape->GetToolBox(), pShape->GetPen(), pShape->GetBrush());
	}
	CATCHZOK
	return ok;
}

int FASTCALL TCanvas2::Draw(const SDrawPath * pPath)
{
	int    ok = 1;
	DrawingProcFrame __frame(this, pPath);
	for(uint j = 0; j < pPath->GetCount(); j++) {
		SDrawPath::Item dpitem;
		uint  p = 0;
		if(pPath->Get(j, dpitem)) {
			switch(dpitem.Op) {
				case SDrawPath::opNop:
					break;
				case SDrawPath::opMove:
					while(p < dpitem.ArgCount) {
						MoveTo(dpitem.Pnt(p));
						p += 2;
					}
					break;
				case SDrawPath::opLine:
					while(p < dpitem.ArgCount) {
						Line(dpitem.Pnt(p));
						p += 2;
					}
					break;
				case SDrawPath::opCurve:
					while(p < dpitem.ArgCount) {
						Bezier(dpitem.Pnt(p), dpitem.Pnt(p+2), dpitem.Pnt(p+4));
						p += 6;
					}
					break;
				case SDrawPath::opQuad:
					while(p < dpitem.ArgCount) {
						FPoint cur = GetCurPoint();
						Bezier(cur + (dpitem.Pnt(p)-cur) * 2.0/3.0,
							(dpitem.Pnt(p)-dpitem.Pnt(p+2)) * 2.0/3.0 + dpitem.Pnt(p+2), dpitem.Pnt(p+2));
						p += 4;
					}
					break;
				case SDrawPath::opArcSvg:
					while(p < dpitem.ArgCount) {
						Implement_ArcSvg(dpitem.Pnt(p),
							dpitem.P_ArgList[p+2], (int)dpitem.P_ArgList[p+3], (int)dpitem.P_ArgList[p+4],
							dpitem.Pnt(p+5));
						p += 7;
					}
					break;
				case SDrawPath::opClose:
					ClosePath();
					break;
			}
		}
	}
	if(ok > 0) {
		SPaintToolBox * p_tb = pPath->GetToolBox();
		int32  id_pen = pPath->GetPen();
		if(id_pen < 0)
			id_pen = p_tb->GetDefaultPen();
		Implement_StrokeAndFill(p_tb, id_pen, pPath->GetBrush());
	}
	return ok;
}

int FASTCALL TCanvas2::Draw(const SImageBuffer * pImg)
{
	int    ok = -1;
	if(pImg) {
		TPoint dim = pImg->GetDim();
		if(dim.x > 0 && dim.y > 0) {
			cairo_surface_t * p_img_surf = (cairo_surface_t *)pImg->CreateSurface(dsysCairo);
			THROW(p_img_surf);
			if(Flags & fScopeRecording) {
				double x1 = 0;
				double y1 = 0;
				double x2 = dim.x;
				double y2 = dim.y;
				cairo_user_to_device(P_Cr, &x1, &y1);
				cairo_user_to_device(P_Cr, &x2, &y2);
				TRect rc((int)floor(x1), (int)floor(y1), (int)ceil(x2), (int)ceil(y2));
				Scope.Add(rc, SCOMBINE_OR);
			}
			cairo_move_to(P_Cr, 0.0, 0.0);
			cairo_set_source_surface(P_Cr, p_img_surf, 0, 0);
			cairo_paint_with_alpha(P_Cr, 1.0);
			cairo_surface_destroy(p_img_surf);
			ok = 1;
		}
	}
	CATCHZOK
	return ok;
}

int FASTCALL TCanvas2::Draw(const SDrawImage * pImg)
{
	int    ok = 1;
	DrawingProcFrame __frame(this, pImg);
	if(pImg)
		ok = Draw(&pImg->GetBuffer());
	return ok;
}

int FASTCALL TCanvas2::Draw(const SDrawGroup * pDraw)
{
	int    ok = 1;
	DrawingProcFrame __frame(this, pDraw);
	if(pDraw)
		for(uint i = 0; ok && i < pDraw->GetCount(); i++)
			if(!Draw(pDraw->Get(i))) // @recursion
				ok = 0;
	return ok;
}

int FASTCALL TCanvas2::Draw(const SDrawFigure * pDraw)
{
	int    ok = 1;
	if(pDraw) {
		switch(pDraw->GetKind()) {
			case SDrawFigure::kShape: THROW(Draw((const SDrawShape *)pDraw)); break;
			case SDrawFigure::kPath:  THROW(Draw((const SDrawPath *)pDraw));  break;
			case SDrawFigure::kText:  THROW(Draw((const SDrawText *)pDraw));  break;
			case SDrawFigure::kGroup: THROW(Draw((const SDrawGroup *)pDraw)); break;
			case SDrawFigure::kImage: THROW(Draw((const SDrawImage *)pDraw)); break;
			default: ok = -1; break;
		}
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

int SLAPI TCanvas2::Implement_ArcSvg(FPoint radius, float xAxisRotation, int large_arc_flag, int sweep_flag, FPoint toPoint)
{
	int    ok = 1;
	radius.X = (float)fabs(radius.X);
	radius.Y = (float)fabs(radius.Y);
	const FPoint cur = GetCurPoint();
	const float sin_th = sinf(degtorad(xAxisRotation));
	const float cos_th = cosf(degtorad(xAxisRotation));
	{
		FPoint d_ = (cur - toPoint) / 2.0f;
		FPoint d1(cos_th*d_.X+sin_th*d_.Y, -sin_th*d_.X+cos_th*d_.Y);
		float check = ((d1 * d1) / (radius * radius)).Add();
		if(check > 1.0f) {
			radius.Scale(sqrtf(check));
		}
	}
	/*
		(x0, y0) is current point in transformed coordinate space.
		(x1, y1) is new point in transformed coordinate space.
		The arc fits a unit-radius circle in this space.
	 */
	{
		FPoint pc;
		float  th_arc, th0;
		{
			FPoint p1;
			{
				FPoint p0;
				{
					FPoint a0(cos_th/radius.X, sin_th/radius.X);
					FPoint a1(-sin_th/radius.Y, cos_th/radius.Y);
					p0 = cur.Combine(a0, a1);
					p1 = toPoint.Combine(a0, a1);
				}
				float sfactor_sq = 1.0f/(p1 - p0).Sq() - 0.25f;
				float sfactor = (sfactor_sq > 0.0f) ? sqrtf(sfactor_sq) : 0.0f;
				if(sweep_flag == large_arc_flag)
					sfactor = -sfactor;
				pc.X = 0.5f*(p0.X+p1.X)-sfactor*(p1.Y-p0.Y);
				pc.Y = 0.5f*(p0.Y+p1.Y)+sfactor*(p1.X-p0.X);
				/* pc is center of the circle. */
				th0 = atan2(p0, pc);
			}
			{
				th_arc = atan2(p1, pc)-th0;
				if(th_arc < 0 && sweep_flag)
					th_arc += (float)SMathConst::Pi2;
				else if(th_arc > 0 && !sweep_flag)
					th_arc -= (float)SMathConst::Pi2;
			}
		}
		/*
			XXX: I still need to evaluate the math performed in this
			function. The critical behavior desired is that the arc must be
			approximated within an arbitrary error tolerance, (which the
			user should be able to specify as well). I don't yet know the
			bounds of the error from the following computation of
			n_segs. Plus the "+ 0.001" looks just plain fishy. -cworth
		*/
		int    n_segs = (int)ceil(fabs(th_arc/(SMathConst::PiDiv2+0.001)));
		const  float th_part = th_arc/n_segs;
		const  FPoint a_(cos_th*radius.X, -sin_th*radius.Y);
		const  FPoint b_(sin_th*radius.X,  cos_th*radius.Y);
		for(int i = 0; i < n_segs; i++) {
			float _th0 = th0+i*th_part;
			float _th1 = th0+(i+1)*th_part;
			// inverse transform compared with rsvg_path_arc
			float th_half = 0.5f*(_th1-_th0);
			float t = (8.0f/3.0f)*sinf(th_half*0.5f)*sinf(th_half*0.5f)/sinf(th_half);
			FPoint p1(pc.X + cosf(_th0) - t * sinf(_th0), pc.Y + sinf(_th0) + t * cosf(_th0));
			FPoint p3(pc.X + cosf(_th1),                 pc.Y + sinf(_th1));
			FPoint p2(p3.X + t * sinf(_th1),             p3.Y - t * cosf(_th1));
			Bezier(p1.Combine(a_, b_), p2.Combine(a_, b_), p3.Combine(a_, b_));
		}
	}
	return ok;
}

int SLAPI TCanvas2::PatBlt(const TRect & rR, int brushId, int opr)
{
	int    ok = 1;
	int    preserve_opr = SetOperator(opr);
	Rect(rR, 0, brushId);
	SetOperator(preserve_opr);
	return ok;
}

int SLAPI TCanvas2::DrawEdge(TRect & rR, long edge, long flags)
{
	int    ok = 1;
	const  int _cxborder = GetSystemMetrics(SM_CXBORDER);
	const  int _cyborder = GetSystemMetrics(SM_CYBORDER);
	int    brush_tl = 0;
	int    brush_br = 0;

	TRect rc;
	uint  bdr_type;

	R_Tb.CreateReservedObjects();
	//
	// Enforce monochromicity and flatness
	//
	if(flags & borderMono)
		flags |= borderFlat;
	rc = rR;
	//
	// Draw the border segment(s), and calculate the remaining space as we go.
	//
	if(bdr_type = (edge & edgeOuter)) {
DrawBorder:
		//
		// Get brushes.  Note the symmetry between raised outer, sunken inner and sunken outer, raised inner.
		//
		if(flags & borderFlat) {
			if(flags & borderMono)
				brush_br = (bdr_type & edgeOuter) ? SPaintToolBox::rbrWindowFrame : SPaintToolBox::rbrWindow;
			else
				brush_br = (bdr_type & edgeOuter) ? SPaintToolBox::rbr3DShadow : SPaintToolBox::rbr3DFace;
			brush_tl = brush_br;
		}
		else {
			switch(bdr_type) {
			    // +2 above surface
			    case edgeRaisedOuter:
					brush_tl = (flags & borderSoft) ? SPaintToolBox::rbr3DHilight : SPaintToolBox::rbr3DLight;
					brush_br = SPaintToolBox::rbr3DDkShadow; // 1
					break;
			    // +1 above surface
			    case edgeRaisedInner:
					brush_tl = (flags & borderSoft) ? SPaintToolBox::rbr3DLight : SPaintToolBox::rbr3DHilight;
					brush_br = SPaintToolBox::rbr3DShadow; // 2
					break;
			    // -1 below surface
			    case edgeSunkenOuter:
					brush_tl = (flags & borderSoft) ? SPaintToolBox::rbr3DDkShadow : SPaintToolBox::rbr3DShadow;
					brush_br = SPaintToolBox::rbr3DHilight; // 5
					break;
			    // -2 below surface
			    case edgeSunkenInner:
					brush_tl = (flags & borderSoft) ? SPaintToolBox::rbr3DShadow : SPaintToolBox::rbr3DDkShadow;
					brush_br = SPaintToolBox::rbr3DLight; // 4
					break;
			    default:
					return FALSE;
			}
		}
		//
		// Draw the sides of the border.  NOTE THAT THE ALGORITHM FAVORS THE
		// BOTTOM AND RIGHT SIDES, since the light source is assumed to be top
		// left.  If we ever decide to let the user set the light source to a
		// particular corner, then change this algorithm.
		//
		if(flags & borderDiagonal) {
			ok = 1; // DrawDiagonal(hdc, &rc, brush_tl, brush_br, flags);
		}
		else {
			//
			// reset ppbData index
			//
			TRect _r;
			if(flags & borderRight) {
				rc.b.x -= _cxborder;
				PatBlt(_r.setwidthrel(rc.b.x, _cxborder).setheightrel(rc.a.y, rc.height()), brush_br, oprOVER);
			}
			if(flags & borderBottom) {
				rc.b.y -= _cyborder;
				PatBlt(_r.setwidthrel(rc.a.x, rc.width()).setheightrel(rc.b.y, _cyborder), brush_br, oprOVER);
			}
			if(flags & borderLeft) {
				PatBlt(_r.setwidthrel(rc.a.x, _cxborder).setheightrel(rc.a.y, rc.height()), brush_tl, oprOVER);
				rc.a.x += _cxborder;
			}
			if(flags & borderTop) {
				PatBlt(_r.setwidthrel(rc.a.x, rc.width()).setheightrel(rc.a.y, _cyborder), brush_tl, oprOVER);
				rc.a.y += _cyborder;
			}
		}
	}
	if((bdr_type = (edge & edgeInner)) != 0) {
		//
		// Strip this so the next time through, bdr_type will be 0.
		// Otherwise, we'll loop forever.
		//
		edge &= ~edgeInner;
		goto DrawBorder;
	}
	//
	// Select old brush back in, if we changed it.
	//
	// Fill the middle & clean up if asked
	//
	if(flags & borderMiddle) {
		if(flags & borderDiagonal)
			ok = 1; // FillTriangle(hdc, &rc, ((flags & borderMono) ? (HBRUSH)SYSHBR(WINDOW) : (HBRUSH)SYSHBR(3DFACE)), flags);
		else {
			Rect(rc, 0, (flags & borderMono) ? SPaintToolBox::rbrWindow : SPaintToolBox::rbr3DFace);
		}
	}
	if(flags & borderAdjust)
		rR = rc;
	return ok;
}

int SLAPI TCanvas2::DrawFrame(const TRect & rR, int clFrame, int brushId)
{
	int    ok = 1;
	int    x = rR.a.x;
	int    y = rR.a.y;
	const  int _cxborder = GetSystemMetrics(SM_CXBORDER)*clFrame;
	const  int _cyborder = GetSystemMetrics(SM_CYBORDER)*clFrame;
	int    cx = rR.width()-_cxborder;
	int    cy = rR.height()-_cyborder;
	TRect _r;
	PatBlt(_r.setwidthrel(x, _cxborder).setheightrel(y, cy), brushId, oprOVER);
	PatBlt(_r.setwidthrel(x+_cxborder, cx).setheightrel(y, _cyborder), brushId, oprOVER);
	PatBlt(_r.setwidthrel(x, cx).setheightrel(y+cy, _cyborder), brushId, oprOVER);
	PatBlt(_r.setwidthrel(x+cx, _cxborder).setheightrel(y+_cyborder, cy), brushId, oprOVER);
	return ok;
}
//
//
//
SLAPI TCanvas::TCanvas(HDC hDc) : ObjStack(sizeof(HGDIOBJ))
{
	H_Dc = hDc;
	Flags |= fOuterDC;
}

SLAPI TCanvas::~TCanvas()
{
}

TCanvas::operator HDC() const
{
	return H_Dc;
}

void FASTCALL TCanvas::SetBounds(const TRect & rBounds)
{
	Bounds = rBounds;
}

int FASTCALL TCanvas::SelectObjectAndPush(HGDIOBJ hObj)
{
	int    ok = 1;
	HGDIOBJ h_old_obj = ::SelectObject(H_Dc, hObj);
	if(h_old_obj)
		ObjStack.push(&h_old_obj);
	else
		ok = 0;
	return ok;
}

int SLAPI TCanvas::PopObject()
{
	HGDIOBJ * p_h_obj = (HGDIOBJ *)ObjStack.pop();
	assert(p_h_obj);
	return p_h_obj ? (int)::SelectObject(H_Dc, *p_h_obj) : 0;
}

int FASTCALL TCanvas::PopObjectN(uint c)
{
	for(uint i = 0; i < c; i++) {
		HGDIOBJ * p_h_obj = (HGDIOBJ *)ObjStack.pop();
		assert(p_h_obj);
		if(!p_h_obj)
			return 0;
		else
			::SelectObject(H_Dc, *p_h_obj);
	}
	return 1;
}

int FASTCALL TCanvas::Rectangle(const TRect & rRect)
{
	return ::Rectangle(H_Dc, rRect.a.x, rRect.a.y, rRect.b.x, rRect.b.y);
}

int SLAPI TCanvas::RoundRect(const TRect & rRect, const TPoint & rRoundPt)
{
	return ::RoundRect(H_Dc, rRect.a.x, rRect.a.y, rRect.b.x, rRect.b.y, rRoundPt.x, rRoundPt.y);
}

int SLAPI TCanvas::FillRect(const TRect & rRect, HBRUSH brush)
{
	RECT r = rRect;
	return ::FillRect(H_Dc, &r, brush);
}

void SLAPI TCanvas::LineVert(int x, int yFrom, int yTo)
{
	::MoveToEx(H_Dc, x, yFrom, 0);
	::LineTo(H_Dc, x, yTo);
}

void SLAPI TCanvas::LineHorz(int xFrom, int xTo, int y)
{
	::MoveToEx(H_Dc, xFrom, y, 0);
	::LineTo(H_Dc, xTo, y);
}

TPoint FASTCALL TCanvas::GetTextSize(const char * pStr)
{
	const int len = sstrlen(pStr);
	char   zero[16];
	if(!len) {
		memzero(zero, sizeof(zero));
		pStr = zero;
	}
	TPoint p;
	SIZE   sz;
	return ::GetTextExtentPoint32(H_Dc, pStr, len, &sz) ?  p.Set(sz.cx, sz.cy) : p.Set(0, 0);
}

void SLAPI TCanvas::SetBkTranparent()
{
	SetBkMode(H_Dc, TRANSPARENT);
}

int FASTCALL TCanvas::SetBkColor(COLORREF c)
{
	COLORREF old_color = ::SetBkColor(H_Dc, c);
	return (old_color == CLR_INVALID) ? 0 : 1;
}

int FASTCALL TCanvas::SetTextColor(COLORREF c)
{
	COLORREF old_color = ::SetTextColor(H_Dc, c);
	return (old_color == CLR_INVALID) ? 0 : 1;
}

int SLAPI TCanvas::DrawText(const TRect & rRect, const char * pText, uint options)
{
	int    len;
	char   zero[16];
	if(pText)
		len = strlen(pText);
	else {
		len = 0;
		memzero(zero, sizeof(zero));
		pText = zero;
	}
	RECT   rect = rRect;
	return BIN(::DrawText(H_Dc, pText, -1, &rect, options));
}

int SLAPI TCanvas::TextOut(TPoint p, const char * pText)
{
	int    len;
	char   zero[16];
	if(pText)
		len = strlen(pText);
	else {
		len = 0;
		memzero(zero, sizeof(zero));
		pText = zero;
	}
	return BIN(::TextOut(H_Dc, p.x, p.y, pText, len));
}
//
//
//
SPaintObj::Base::Base()
{
	Handle = 0;
	Sys = dsysNone;
}

int SPaintObj::Base::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	// @os64problem: преобразование (void*) <-> uint32 не будет работать на 64-битной системе.
	// @note При решении проблемы не забыть о совместимости форматов для уже сохраненных данных!
	if(dir > 0) {
		uint32 temp = (uint32)Handle;
		THROW(pCtx->Serialize(dir, temp, rBuf));
	}
	else if(dir < 0) {
		uint32 temp = 0;
		THROW(pCtx->Serialize(dir, temp, rBuf));
		Handle = (void *)temp;
	}
	THROW(pCtx->Serialize(dir, (int &)Sys, rBuf));
	CATCHZOK
	return ok;
}

SPaintObj::Pen::Pen() : Base()
{
	W = 1.0;
	S = SPaintObj::psSolid;
	LineCap = SPaintObj::lcButt;
	Join = SPaintObj::ljMiter;
	Reserve = 0;
	MiterLimit = SPaintObj::DefaultMiterLimit;
	DashOffs = 0.0f;
	P_DashRule = 0;
}

SPaintObj::Pen::~Pen()
{
	ZDELETE(P_DashRule);
}

int FASTCALL SPaintObj::Pen::Copy(const Pen & rS)
{
	int    ok = 1;
	C = rS.C;
	W = rS.W;
	S = rS.S;
	LineCap = rS.LineCap;
	Join = rS.Join;
	MiterLimit = rS.MiterLimit;
	DashOffs = rS.DashOffs;
	if(rS.P_DashRule) {
		SETIFZ(P_DashRule, new FloatArray);
		*P_DashRule = *rS.P_DashRule;
	}
	else
		ZDELETE(P_DashRule);
	return ok;
}

int SPaintObj::Pen::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	THROW(Base::Serialize(dir, rBuf, pCtx));
	THROW(pCtx->SerializeBlock(dir, sizeof(C), &C, rBuf, 1));
	THROW(pCtx->Serialize(dir, W, rBuf));
	THROW(pCtx->Serialize(dir, S, rBuf));
	THROW(pCtx->Serialize(dir, LineCap, rBuf));
	THROW(pCtx->Serialize(dir, Join, rBuf));
	THROW(pCtx->Serialize(dir, Reserve, rBuf));
	THROW(pCtx->Serialize(dir, MiterLimit, rBuf));
	THROW(pCtx->Serialize(dir, DashOffs, rBuf));
	if(dir > 0) {
		THROW(pCtx->Serialize(dir, P_DashRule, rBuf));
	}
	else if(dir < 0) {
		FloatArray temp_list;
		THROW(pCtx->Serialize(dir, &temp_list, rBuf));
		if(temp_list.getCount()) {
			THROW_S(SETIFZ(P_DashRule, new FloatArray), SLERR_NOMEM);
			*P_DashRule = temp_list;
		}
		else {
			ZDELETE(P_DashRule);
		}
	}
	CATCHZOK
	return ok;
}

SPaintObj::Pen & FASTCALL SPaintObj::Pen::operator = (const Pen & rS)
{
	Copy(rS);
	return *this;
}

int SPaintObj::Pen::IsDashed() const
{
	return ((P_DashRule && P_DashRule->getCount()) || oneof4(S, SPaintObj::psDash,
		SPaintObj::psDot, SPaintObj::psDashDot, SPaintObj::psDashDotDot));
}

int SPaintObj::Pen::AddDashItem(float f)
{
	if(SETIFZ(P_DashRule, new FloatArray)) {
		P_DashRule->add(f);
		return 1;
	}
	else
		return SLS.SetError(SLERR_NOMEM);
}

int FASTCALL SPaintObj::Pen::IsEqual(const Pen & rS) const
{
	if(!(C == rS.C && W == rS.W && LineCap == rS.LineCap && Join == rS.Join &&
		MiterLimit == rS.MiterLimit && DashOffs == rS.DashOffs))
		return 0;
	else if(P_DashRule != 0 && rS.P_DashRule != 0)
		return P_DashRule->IsEqual(rS.P_DashRule);
	else if(P_DashRule == 0 && rS.P_DashRule == 0)
		return 1;
	else
		return 0;
}

int SPaintObj::Pen::IsSimple() const
{
	return (W == 1.0 && !IsDashed() && S == SPaintObj::psSolid &&
		oneof2(LineCap, 0, SPaintObj::lcButt) && oneof2(Join, 0, SPaintObj::ljMiter) &&
		MiterLimit == 0.0f || MiterLimit == SPaintObj::DefaultMiterLimit);
}

int FASTCALL SPaintObj::Pen::SetSimple(SColor c)
{
	C = c;
	W = 1.0;
	S = SPaintObj::psSolid;
	LineCap = 0;
	Join = 0;
	Reserve = 0;
	MiterLimit = 0.0f;
	DashOffs = 0.0f;
	ZDELETE(P_DashRule);
	return 1;
}

SPaintObj::Brush::Brush() : Base()
{
	S = bsSolid;
	Hatch = 0;
	Rule = 0;
	Reserve = 0;
	IdPattern = 0;
}

int FASTCALL SPaintObj::Brush::operator == (const Brush & rS) const
{
	return IsEqual(rS);
}

int FASTCALL SPaintObj::Brush::IsEqual(const Brush & rS) const
{
	return BIN(C == rS.C && S == rS.S && Hatch == rS.Hatch && Rule == rS.Rule && IdPattern == rS.IdPattern);
}

SPaintObj::Brush & FASTCALL SPaintObj::Brush::operator = (const Brush & rS)
{
	Copy(rS);
	return *this;
}

int FASTCALL SPaintObj::Brush::Copy(const Brush & rS)
{
	C = rS.C;
	S = rS.S;
	Hatch = rS.Hatch;
	Rule = rS.Rule;
	IdPattern = rS.IdPattern;
	return 1;
}

int SPaintObj::Brush::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	THROW(Base::Serialize(dir, rBuf, pCtx));
	THROW(pCtx->SerializeBlock(dir, sizeof(C), &C, rBuf, 1));
	THROW(pCtx->Serialize(dir, S, rBuf));
	THROW(pCtx->Serialize(dir, Hatch, rBuf));
	THROW(pCtx->Serialize(dir, Rule, rBuf));
	THROW(pCtx->Serialize(dir, Reserve, rBuf));
	THROW(pCtx->Serialize(dir, IdPattern, rBuf));
	CATCHZOK
	return ok;
}

int SPaintObj::Brush::IsSimple() const
{
	return (S == 0 || S == SPaintObj::bsSolid && Hatch == 0 && oneof2(Rule, 0, SPaintObj::frNonZero) && IdPattern == 0);
}

int FASTCALL SPaintObj::Brush::SetSimple(SColor c)
{
	C = c;
	S = SPaintObj::psSolid;
	Hatch = 0;
	Rule = 0;
	IdPattern = 0;
	return 1;
}
//
//
//
SFontDescr::SFontDescr(const char * pFace, int size, int flags)
{
	Init();
	Face = pFace;
	Size = (int16)size;
	Flags = (flags & (fItalic|fUnderline|fStrikeOut|fBold|fAntialias));
}

void SFontDescr::Init()
{
	Face = 0;
	Size = 0;
	Flags = 0;
	Weight = 0.0f;
	CharSet = DEFAULT_CHARSET;
	memzero(Reserve, sizeof(Reserve));
}

int FASTCALL SFontDescr::IsEqual(const SFontDescr & rS) const
{
	return BIN(Face.CmpNC(rS.Face) == 0 && Size == rS.Size && Flags == rS.Flags &&
		Weight == rS.Weight && CharSet == rS.CharSet);
}

int SFontDescr::ToStr(SString & rBuf, long fmt) const
{
	rBuf.Z();
	rBuf.Cat(Face);
	if(Size > 0 || (Flags & (fItalic|fUnderline|fStrikeOut|fBold))) {
		rBuf.CatChar('(').Cat(Size);
		if(Flags & fBold)
			rBuf.CatChar('B');
		if(Flags & fItalic)
			rBuf.CatChar('I');
		if(Flags & fUnderline)
			rBuf.CatChar('U');
		if(Flags & fStrikeOut)
			rBuf.CatChar('S');
		rBuf.CatChar(')');
	}
	return 1;
}

int FASTCALL SFontDescr::FromStr(const char * pStr)
{
	int    ok = 1;
	Init();
	if(pStr) {
		const char * p = strchr(pStr, '(');
		if(p) {
			size_t len = (p - pStr);
			Face.CatN(pStr, len).Strip();
			p++;
			while(oneof2(*p, ' ', '\t'))
				p++;
			SString temp_buf;
			while(isdigit(*p))
				temp_buf.CatChar(*p++);
			Size = (int16)SDrawContext::CalcScreenFontSizePt((uint)temp_buf.ToLong());
			while(*p != ')' && *p != 0) {
				char u = toupper(*p++);
				if(u == 'B')
					Flags |= fBold;
				else if(u == 'I')
					Flags |= fItalic;
				else if(u == 'U')
					Flags |= fUnderline;
				else if(u == 'S')
					Flags |= fStrikeOut;
			}
		}
	}
	else
		ok = -1;
	return ok;
}

int SFontDescr::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	THROW(pCtx->SerializeBlock(dir, offsetof(SFontDescr, Face) - offsetof(SPaintObj::Font, Size), &Size, rBuf, 1));
	THROW(pCtx->Serialize(dir, Face, rBuf));
	CATCHZOK
	return ok;
}

int FASTCALL SFontDescr::Helper_SetLogFont(const void * pLf)
{
	int    ok = 1;
	Init();
	LOGFONTW * p_lf = (LOGFONTW *)pLf;
	if(p_lf) {
		Size = (int16)labs(p_lf->lfHeight);
		if(p_lf->lfWeight >= 700)
			Flags |= fBold;
		if(p_lf->lfItalic)
			Flags |= fItalic;
		if(p_lf->lfUnderline)
			Flags |= fUnderline;
		if(p_lf->lfStrikeOut)
			Flags |= fStrikeOut;
	}
	else
		ok = -1;
	return ok;
}

int FASTCALL SFontDescr::SetLogFont(const LOGFONTA * pLf)
{
	int    ok = Helper_SetLogFont(pLf);
	if(ok)
		Face = pLf->lfFaceName;
	return ok;
}

int FASTCALL SFontDescr::SetLogFont(const LOGFONTW * pLf)
{
	int    ok = Helper_SetLogFont(pLf);
	if(ok) {
		SStringU ustr = pLf->lfFaceName;
		ustr.CopyToUtf8(Face, 1);
		Face.Utf8ToChar();
	}
	return ok;
}

/*
typedef struct tagLOGFONT {
  LONG lfHeight;
  LONG lfWidth;
  LONG lfEscapement;
  LONG lfOrientation;
  LONG lfWeight;
  BYTE lfItalic;
  BYTE lfUnderline;
  BYTE lfStrikeOut;
  BYTE lfCharSet;
  BYTE lfOutPrecision;
  BYTE lfClipPrecision;
  BYTE lfQuality;
  BYTE lfPitchAndFamily;
  TCHAR lfFaceName[LF_FACESIZE];
} LOGFONT, *PLOGFONT;
*/

int FASTCALL SFontDescr::Helper_MakeLogFont(void * pLf) const
{
	int    ok = 1;
	LOGFONTW * p_lf = (LOGFONTW *)pLf;
	if(p_lf) {
		p_lf->lfHeight = -labs(NZOR(Size, 1));
		if(Weight <= 0.0f)
			p_lf->lfWeight = (Flags & fBold) ? 700 : FW_DONTCARE;
		else if(Weight >= 2.0f)
			p_lf->lfWeight = FW_HEAVY;
		else if(Weight == 1.0f)
			p_lf->lfWeight = FW_NORMAL;
		else {
			p_lf->lfWeight = (long)round(Weight * ((FW_HEAVY - FW_THIN) / 2.0f), -2);
			SETIFZ(p_lf->lfWeight, FW_THIN);
		}
		p_lf->lfCharSet = NZOR(CharSet, DEFAULT_CHARSET);
		p_lf->lfOutPrecision = OUT_TT_ONLY_PRECIS;
		p_lf->lfClipPrecision = CLIP_DEFAULT_PRECIS;
		p_lf->lfQuality = PROOF_QUALITY; // DEFAULT_QUALITY;
		p_lf->lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
		if(Flags & fItalic)
			p_lf->lfItalic = 1;
		if(Flags & fUnderline)
			p_lf->lfUnderline = 1;
		if(Flags & fStrikeOut)
			p_lf->lfStrikeOut = 1;
	}
	else
		ok = 0;
	return ok;
}

LOGFONTA * FASTCALL SFontDescr::MakeLogFont(LOGFONTA * pLf) const
{
	if(pLf) {
		memzero(pLf, sizeof(*pLf));
		if(Helper_MakeLogFont(pLf)) {
			Face.CopyTo(pLf->lfFaceName, SIZEOFARRAY(pLf->lfFaceName));
			pLf->lfQuality = CLEARTYPE_QUALITY; // @v8.6.8
		}
	}
	return pLf;
}

LOGFONTW * FASTCALL SFontDescr::MakeLogFont(LOGFONTW * pLf) const
{
	if(pLf) {
		memzero(pLf, sizeof(*pLf));
		if(Helper_MakeLogFont(pLf)) {
			SString face;
			SStringU face_u;
			face_u.CopyFromUtf8((face = Face).ToUtf8());
			face_u.CopyTo(pLf->lfFaceName, SIZEOFARRAY(pLf->lfFaceName));
			pLf->lfQuality = CLEARTYPE_QUALITY; // @v8.6.8
		}
	}
	return pLf;
}

SPaintObj::Font::Font() : SPaintObj::Base(), SFontDescr(0, 0, 0)
{
	LineAdv = 0.0f;
}

SPaintObj::Font::~Font()
{
}

extern "C" void FASTCALL _cairo_scaled_font_reset_cache(cairo_scaled_font_t * scaled_font);

struct InnerFontDescr {
	InnerFontDescr()
	{
		Hf = 0;
		P_CrFace = 0;
		P_CrScFont = 0;
	}
	~InnerFontDescr()
	{
		if(P_CrScFont) {
			//
			// cairo, видимо в следствии ошибки, не очищает кэш символов шрифта.
			// Приходится ей помочь в этом.
			//
			_cairo_scaled_font_reset_cache(P_CrScFont);
			//
			/*
			uint ref_count = cairo_scaled_font_get_reference_count(P_CrScFont);
			while(ref_count >= 2) {
				cairo_scaled_font_destroy(P_CrScFont);
				ref_count = (ref_count > 1) ? cairo_scaled_font_get_reference_count(P_CrScFont) : 0;
			}
			*/
			cairo_scaled_font_destroy(P_CrScFont);
		}
		if(P_CrFace) {
			cairo_font_face_destroy(P_CrFace);
			/*
			uint ref_count = cairo_font_face_get_reference_count(P_CrFace);
			while(ref_count >= 1) {
				cairo_font_face_destroy(P_CrFace);
				ref_count = (ref_count > 1) ? cairo_font_face_get_reference_count(P_CrFace) : 0;
			}
			*/
		}
		ZDeleteWinGdiObject(&Hf);
	}
	HFONT  Hf;
	cairo_font_face_t * P_CrFace;
	cairo_scaled_font_t * P_CrScFont;
};

SPaintObj::Font::operator HFONT () const
{
	if(GetSys() == dsysWinGdi) {
		return (HFONT)GetHandle();
	}
	else if(GetSys() == dsysCairo) {
		InnerFontDescr * p_descr = (InnerFontDescr *)GetHandle();
		if(p_descr && p_descr->Hf)
			return p_descr->Hf;
	}
	return 0;
}

SPaintObj::Font::operator cairo_font_face_t * () const
{
	if(GetSys() == dsysCairo) {
		InnerFontDescr * p_descr = (InnerFontDescr *)GetHandle();
		if(p_descr)
			return p_descr->P_CrFace;
	}
	return 0;
}

SPaintObj::Font::operator cairo_scaled_font_t * () const
{
	if(GetSys() == dsysCairo) {
		InnerFontDescr * p_descr = (InnerFontDescr *)GetHandle();
		if(p_descr)
			return p_descr->P_CrScFont;
	}
	return 0;
}

int SPaintObj::Font::GetGlyph(SDrawContext & rCtx, uint16 chr, SGlyph * pGlyph)
{
	int    ok = 0;
	pGlyph->Chr = chr;
	pGlyph->LineAdv = LineAdv;
	if(GetSys() == dsysWinGdi) {
		HFONT hf = (HFONT)GetHandle();
		HDC    hdc = SLS.GetTLA().GetFontDC();
		HFONT  preserve_hf = (HFONT)::SelectObject(hdc, hf);
		wchar_t t[2];
		t[0] = chr;
		t[1] = 0;
		WORD   idx_list[1];
		uint   c = GetGlyphIndicesW(hdc, t, 1, idx_list, GGI_MARK_NONEXISTING_GLYPHS);
		if(c > 0) {
			pGlyph->Idx = (int16)idx_list[0];
			ok = 1;
		}
		::SelectObject(hdc, preserve_hf);
	}
	else if(GetSys() == dsysCairo) {
		InnerFontDescr * p_descr = (InnerFontDescr *)GetHandle();
		if(p_descr && p_descr->Hf) {
			HDC    hdc = SLS.GetTLA().GetFontDC();
			HFONT  preserve_hf = (HFONT)::SelectObject(hdc, p_descr->Hf);
			wchar_t t[2];
			t[0] = chr;
			t[1] = 0;
			WORD   idx_list[1];
			uint   c = GetGlyphIndicesW(hdc, t, 1, idx_list, GGI_MARK_NONEXISTING_GLYPHS);
			if(c > 0) {
				pGlyph->Idx = (int16)idx_list[0];
				cairo_t * p_cr = rCtx;
				if(p_cr) {
					LMatrix2D preserve_mtx, mtx;
					cairo_get_matrix(p_cr, (cairo_matrix_t *)&preserve_mtx);
					if(p_descr->P_CrScFont)
						cairo_set_scaled_font(p_cr, p_descr->P_CrScFont);
					else if(p_descr->P_CrFace)
						cairo_set_font_face(p_cr, p_descr->P_CrFace);
					{
						cairo_set_matrix(p_cr, (cairo_matrix_t *)&mtx);
						cairo_glyph_t glyph;
						glyph.index = pGlyph->Idx;
						glyph.x = 0.0;
						glyph.y = 0.0;
						cairo_text_extents_t ext;
						cairo_glyph_extents(p_cr, &glyph, 1, &ext);
						pGlyph->Sz.Set((float)ext.width, (float)ext.height);
						pGlyph->Org.Set((float)ext.x_bearing, (float)ext.y_bearing);
						pGlyph->Advance.Set((float)ext.x_advance, (float)ext.y_advance);
						if(pGlyph->LineAdv == 0.0)
							pGlyph->LineAdv = pGlyph->Sz.Y;
					}
					cairo_set_matrix(p_cr, (cairo_matrix_t *)&preserve_mtx);
				}
				ok = 1;
			}
			::SelectObject(hdc, preserve_hf);
		}
	}
	return ok;
}

int SPaintObj::Font::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	THROW(Base::Serialize(dir, rBuf, pCtx));
	THROW(SFontDescr::Serialize(dir, rBuf, pCtx));
	CATCHZOK
	return ok;
}
//
//
//
SPaintObj::CStyle::CStyle() : SPaintObj::Base()
{
	FontId = 0;
	PenId = 0;
	BrushId = 0;
	memzero(Reserve, sizeof(Reserve));
}

int FASTCALL SPaintObj::CStyle::IsEqual(const CStyle & rS) const
{
	return BIN(FontId == rS.FontId && PenId == rS.PenId && BrushId == rS.BrushId);
}

int SPaintObj::CStyle::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	THROW(Base::Serialize(dir, rBuf, pCtx));
	THROW(pCtx->Serialize(dir, FontId, rBuf));
	THROW(pCtx->Serialize(dir, PenId, rBuf));
	THROW(pCtx->Serialize(dir, BrushId, rBuf));
	THROW(pCtx->SerializeBlock(dir, sizeof(Reserve), Reserve, rBuf, 1));
	CATCHZOK
	return ok;
}
//
//
//
STextLayout::RenderGroup::RenderGroup()
{
	P_Font = 0;
	PenId = 0;
	BrushId = 0;
}

STextLayout::STextLayout()
{
	DefParaStyleIdent = 0;
	DefCStyleIdent = 0;
	Flags = 0;
	State = 0;
	Bounds = 0.0f;
	EndPoint = 0.0f;
}

int STextLayout::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	THROW(pCtx->Serialize(dir, DefParaStyleIdent, rBuf));
	THROW(pCtx->Serialize(dir, DefCStyleIdent, rBuf));
	THROW(pCtx->Serialize(dir, Flags, rBuf));
	THROW(pCtx->Serialize(dir, Bounds.a, rBuf));
	THROW(pCtx->Serialize(dir, Bounds.b, rBuf));
	THROW(pCtx->Serialize(dir, Text, rBuf));
	THROW(pCtx->Serialize(dir, &CStyleList, rBuf));
	THROW(pCtx->Serialize(dir, &ParaList, rBuf));
	CATCHZOK
	return ok;
}

void STextLayout::Reset()
{
	DefParaStyleIdent = 0;
	DefCStyleIdent = 0;
	Flags = 0;
	State = 0;
	Bounds = FPoint(0.0f);
	EndPoint = 0.0f;
	Text = 0;
	CStyleList.clear();
	ParaList.clear();
	List.clear();
	SgList.clear();
}

int STextLayout::Copy(const STextLayout & rS)
{
	Reset();
	DefParaStyleIdent = rS.DefParaStyleIdent;
	DefCStyleIdent = rS.DefCStyleIdent;
	Flags = rS.Flags;
	Bounds = rS.Bounds;
	EndPoint = rS.EndPoint;
	Text = rS.Text;
	CStyleList = rS.CStyleList;
	ParaList = rS.ParaList;
	return 1;
}

const FRect & STextLayout::GetBounds() const
{
	return Bounds;
}

FRect STextLayout::GetBkgBounds() const
{
	if(Flags & fPrecBkg) {
		FRect r = Bounds;
		r.b = EndPoint;
		return r;
	}
	else
		return Bounds;
}

int FASTCALL STextLayout::SetBounds(const FRect & rBounds)
{
	Bounds = rBounds;
	State &= ~stArranged;
	return 1;
}

int STextLayout::SetText(const char * pText)
{
	FRect preserve_bounds = Bounds;
	long  preserve_flags = Flags;
	Reset();
	SString temp_buf;
	Text.CopyFromUtf8((temp_buf = pText).Transf(CTRANSF_INNER_TO_UTF8));
	ParaList.Add(0, 0, 0);
	Bounds = preserve_bounds;
	Flags = preserve_flags;
	return 1;
}

int STextLayout::SetOptions(long flags, int defParaStyleId, int defCStyleId)
{
	Flags |= (flags & (fWrap|fUnlimX|fUnlimY|fNoClip|fPrecBkg));
	// @v9.1.8 {
	if(flags & fVCenter && !(flags & fVBottom))
		Flags |= fVCenter;
	else if(flags & fVBottom && !(flags & fVCenter))
		Flags |= fVBottom;
	// } @v9.1.8
	if(defParaStyleId >= 0) {
		DefParaStyleIdent = defParaStyleId;
		State &= ~stPreprocessed;
	}
	if(defCStyleId >= 0) {
		DefCStyleIdent = defCStyleId;
		State &= ~stPreprocessed;
	}
	State &= ~stArranged;
	return 1;
}

int FASTCALL STextLayout::HasOption(long f) const
{
	return BIN(Flags & f);
}

const SStringU & STextLayout::GetText() const
{
	return Text;
}

int FASTCALL STextLayout::CanWrap(uint pos)
{
	return 0;
}

int STextLayout::SetTextStyle(uint startPos, uint len, int cstyleId)
{
	int    ok = 1;
	if(cstyleId && startPos < Text.Len() && len >= 0) {
		CStyle style;
		style.Start = startPos;
		style.Len = len;
		style.StyleIdent = cstyleId;
		if(CStyleList.ordInsert(&style, 0, CMPF_LONG, 0)) {
			State &= ~(stPreprocessed | stArranged);
		}
		else
			ok = 0;
	}
	else
		ok = -1;
	return ok;
}

int STextLayout::Preprocess(SDrawContext & rCtx, SPaintToolBox & rTb)
{
	int    ok = 1;
	const uint cc = Text.Len();
	assert(!(State & stPreprocessed) || cc == GlyphIdList.getCount());
	if(!(State & stPreprocessed)) {
		const uint cslc = CStyleList.getCount();
		GlyphIdList.clear();
		SgList.clear();
		StyleGroup sg;
		MEMSZERO(sg);
		int    prev_cstyle_id = 0;
		int    prev_font_id = 0;
		int    prev_pen_id = 0;
		int    prev_brush_id = 0;
		SPaintObj * p_obj = 0;
		SPaintObj::CStyle * p_cs = 0;
		for(uint i = 0; i < cc; i++) {
			int    cstyle_id = DefCStyleIdent;
			for(uint j = 0; j < cslc; j++) {
				const CStyle & r_style = CStyleList.at(j);
				if(i < r_style.Start)
					break;
				else if(i >= r_style.Start && i < (r_style.Start+r_style.Len))
					cstyle_id = r_style.StyleIdent;
			}
			if(!cstyle_id) {
				int    font_id = rTb.CreateFont(0, "Arial", SDrawContext::CalcScreenFontSizePt(10), 0);
				int    pen_id = rTb.CreateColor(0, SColor(SClrBlack));
				cstyle_id = rTb.CreateCStyle(0, font_id, pen_id, 0);
				DefCStyleIdent = cstyle_id;
			}
			if(cstyle_id && ((prev_cstyle_id == cstyle_id && p_cs) || ((p_obj = rTb.GetObj(cstyle_id)) != 0 && (p_cs = p_obj->GetCStyle()) != 0))) {
				int    gid = rTb.GetGlyphId(rCtx, p_cs->FontId, Text[i]);
				GlyphIdList.add(gid);
				if(p_cs->FontId != prev_font_id || p_cs->PenId != prev_pen_id || p_cs->BrushId != prev_brush_id) {
					sg.Pos = i;
					sg.P_Font = rTb.GetFont(rCtx, prev_font_id = p_cs->FontId);
					prev_pen_id = sg.PenId = p_cs->PenId;
					prev_brush_id = sg.BrushId = p_cs->BrushId;
					SgList.insert(&sg);
				}
				prev_cstyle_id = cstyle_id;
			}
		}
		State |= stPreprocessed;
	}
	else
		ok = -1;
	return ok;
}

#define SPACE (L' ')

class TloRowState {
public:
	struct StkItem {
		float X;    // X-координата символа
		float EndX; // X-координата конца символа
		float H;    // Высота символа (для первой строки - высота символа, для последующих - высота строки)
	};
	uint   N; // Номер строки
	uint   S; // Стартовая позиция текущей строки
	int    Overflow;
	FRect  Bounds;
	TSStack <StkItem> Stk;
	TSVector <STextLayout::Item> & R_List; // @v9.8.4 TSArray-->TSVector

	TloRowState(TSVector <STextLayout::Item> & rList, const FRect & rBounds) : R_List(rList) // @v9.8.4 TSArray-->TSVector
	{
		N = 0;
		S = 0;
		Overflow = 0;
		Bounds = rBounds;
	}
	uint GetCount() const
	{
		return Stk.getPointer();
	}
	float FASTCALL AdjustRowPos(int justif)
	{
		const uint c = GetCount();
		float ht = 0.0f;
		if(c) {
			uint i;
			int   start_inited = 0;
			float start = 0.0f;
			for(i = 0; i < c; i++) {
				float ch = ((StkItem *)Stk.at(i))->H;
				SETMAX(ht, ch);
			}
			uint lc = R_List.getCount();
			assert(lc >= c);
			for(i = 0; i < c; i++) {
				STextLayout::Item & r_item = R_List.at(lc-c+i);
				if(!start_inited && r_item.GlyphIdx >= 0) {
					start = r_item.P.X;
					start_inited = 1;
				}
				r_item.P.Y += ht;
			}
			if(start_inited && oneof2(justif, ADJ_RIGHT, ADJ_CENTER)) {
				float end = ((StkItem *)Stk.at(c-1))->EndX;
				float delta = Bounds.Width() - (end-start);
				if(justif == ADJ_CENTER)
					delta /= 2.0f;
				for(i = 0; i < c; i++)
					R_List.at(lc-c+i).P.X += delta;
			}
		}
		return ht;
	}
	void FASTCALL NewRow(uint curTxtPos)
	{
		N++;
		S = curTxtPos;
		Stk.clear();
		Overflow = 0;
	}
	float FASTCALL PopGlyph(uint c)
	{
		float x = Stk.peek().X;
		for(uint i = 0; i < c; i++) {
			StkItem si;
			int r = Stk.pop(si);
			assert(r);
			assert(R_List.getCount());
			R_List.atFree(R_List.getCount()-1);
			x = si.X;
		}
		return x;
	}
	int PushGlyph(const SGlyph * pGlyph, FPoint p, uint16 flags)
	{
		{
			STextLayout::Item item;
			item.P = p;
			item.GlyphIdx = pGlyph ? pGlyph->Idx : -1;
			item.Flags = flags;
			R_List.insert(&item);
		}
		{
			StkItem si;
			si.X = p.X;
			si.EndX = pGlyph ? (p.X + pGlyph->Sz.X) : p.X;
			if(pGlyph)
				si.H = (N == 0) ? pGlyph->Sz.Y : pGlyph->LineAdv;
			else
				si.H = 0.0f;
			Stk.push(si);
		}
		return 1;
	}
	int Wrap(const SStringU & rText, uint & rCurTxtPos)
	{
		// rCurTxtPos - Позиция символа не помещающегося в границы и не внесенного в стек
		int    wrap = 0;
		uint   txt_pos = rCurTxtPos;
		if(GetCount()) {
			int    force_wrap = 0;
			do {
				uint   i = GetCount();
				if(i) {
					do {
						--i;
						wchar_t ch = rText[--txt_pos];
						if(force_wrap)
							wrap = 1;
						else if(ch == SPACE)
							wrap = 2;
						else if(oneof4(ch, L'-', L',', L'.', L';'))
							wrap = 1;
						if(wrap) {
							PopGlyph(GetCount()-i-1);
							if(wrap == 2) {
								R_List.at(txt_pos).GlyphIdx = -1;
							}
							break;
						}
					} while(i);
				}
				if(!wrap) {
					force_wrap++;
					txt_pos = rCurTxtPos;
				}
			} while(!wrap && force_wrap < 2);
			if(wrap)
				rCurTxtPos = txt_pos;
		}
		return wrap;
	}
};

int STextLayout::Arrange(SDrawContext & rCtx, SPaintToolBox & rTb)
{
	int    ok = 1;
	assert(!(State & stArranged) || (State & stPreprocessed));
	if(!(State & stArranged)) {
		Preprocess(rCtx, rTb);
		List.clear();
		const uint pc = ParaList.getCount();
		TloRowState row_state(List, Bounds);
		int    justif = ADJ_LEFT; // Выравнивание по последнему определителю параграфа
		FPoint cur = Bounds.a;
		for(uint i = 0; i < pc; i++) {
			const LAssoc & r_pitem = ParaList.at(i);
			const uint para_pos = (uint)r_pitem.Key;
			const uint cc = (i == (pc-1)) ? (Text.Len() - para_pos) : (ParaList.at(i+1).Key - para_pos);
			int para_style_ident = NZOR(r_pitem.Val, DefParaStyleIdent);
			if(!para_style_ident) {
				SParaDescr pd;
				pd.LuIndent = 0;
				pd.RlIndent = 0;
				pd.StartIndent = 0;
				para_style_ident = rTb.CreateParagraph(0, &pd);
				DefParaStyleIdent = para_style_ident;
			}
			SPaintObj * p_obj_para = rTb.GetObj(para_style_ident);
			if(p_obj_para && p_obj_para->GetType() == SPaintObj::tParagraph) {
				p_obj_para->CreateInnerHandle(rCtx);
				const SPaintObj::Para * p_para_style = p_obj_para->GetParagraph();
				const TPoint _indent = p_para_style->LuIndent;
				justif = p_para_style->GetJustif();
				cur.X += (_indent.x + p_para_style->StartIndent);
				cur.Y += _indent.y;
				row_state.S = para_pos;
				for(uint j = 0; j < cc; j++) {
					uint text_pos = para_pos+j;
					if(cur.Y > Bounds.b.Y) {
						row_state.PushGlyph(0, cur, 0);
					}
					else if(Text[text_pos] == L'\n') {
						float ht = row_state.AdjustRowPos(justif);
						row_state.PushGlyph(0, cur, 0);
						row_state.NewRow(text_pos+1);
						cur.X = Bounds.a.X + _indent.x;
						cur.Y += ht;
					}
					else if(Text[text_pos] == L'&' && j < (cc-1) && Text[text_pos+1] != L'&') {
						row_state.PushGlyph(0, cur, 0); // Специальный символ - следующий будет подчеркнут
					}
					else {
						const SGlyph * p_glyph = rTb.GetGlyph(GlyphIdList.get(text_pos));
						if(p_glyph && (cur.Y + (row_state.N ? p_glyph->LineAdv : p_glyph->Sz.Y)) <= Bounds.b.Y) { // @!
							if(row_state.Overflow || (cur.X + p_glyph->Sz.X) > Bounds.b.X) {
								row_state.Overflow = 1;
								if(!(Flags & fUnlimX) && Flags & fWrap && j && row_state.Wrap(Text, text_pos)) {
									float ht = row_state.AdjustRowPos(justif);
									row_state.NewRow(text_pos+1);
									cur.X = Bounds.a.X + _indent.x;
									cur.Y += ht;
									j = text_pos-para_pos;
								}
								else
									row_state.PushGlyph(0, cur, 0);
							}
							else {
								uint16 item_flags = 0;
								if(j > 0 && Text[text_pos-1] == L'&' && Text[text_pos] != L'&') {
									item_flags |= Item::fUnderscore;
								}
								row_state.PushGlyph(p_glyph, cur, item_flags);
								cur = cur + p_glyph->Advance;
							}
						}
						else
							row_state.PushGlyph(0, cur, 0);
					}
				}
			}
		}
		{
			const float ht = row_state.AdjustRowPos(justif);
			EndPoint = cur;
			EndPoint.Y += ht;
		}
		if(Flags & (fVCenter|fVBottom)) {
			const float _full_height = EndPoint.Y - Bounds.a.Y;
			const float _gap = Bounds.Height() - _full_height;
			if(_gap > 0.0f) {
                const float _voffs = (Flags & fVCenter) ? (_gap / 2.0f) : _gap;
                if(_voffs >= 0.5f) {
					for(uint ci = 0; ci < List.getCount(); ci++) {
                        List.at(ci).P.Y += _voffs;
					}
					EndPoint.Y += _voffs;
                }
			}
		}
		State |= stArranged;
	}
	assert(List.getCount() == 0 || List.getCount() <= Text.Len());
	return ok;
}

int STextLayout::EnumGroups(uint * pI, RenderGroup * pGroup)
{
	int    ok = 1;
	if(pGroup) {
		pGroup->Items.clear();
		pGroup->P_Font = 0;
		pGroup->PenId = 0;
		pGroup->BrushId = 0;
	}
	const uint sgc = SgList.getCount();
	uint i = pI ? *pI : 0;
	if(i < sgc) {
		if(pGroup) {
			StyleGroup & r_sg = SgList.at(i);
			const uint end = (i < (sgc-1)) ? SgList.at(i+1).Pos : List.getCount();
			for(uint j = r_sg.Pos; j < end; j++) {
				const Item & r_item = List.at(j);
				if(r_item.GlyphIdx >= 0)
					pGroup->Items.insert(&r_item);
			}
			pGroup->P_Font = r_sg.P_Font;
			pGroup->PenId = r_sg.PenId;
			pGroup->BrushId = r_sg.BrushId;
		}
		ASSIGN_PTR(pI, i+1);
	}
	else
		ok = 0;
	return ok;
}

int FASTCALL TCanvas2::SelectFont(SPaintObj::Font * pFont)
{
	int    ok = -1;
	if(P_SelectedFont) {
		if(P_SelectedFont != pFont) {
			/*
			cairo_font_face_t * p_ff = *P_SelectedFont;
			cairo_scaled_font_t * p_scf = *P_SelectedFont;
			if(p_scf)
				cairo_set_scaled_font(P_Cr, 0);
			else if(p_ff)
				cairo_set_font_face(P_Cr, 0);
			*/
			P_SelectedFont = 0;
		}
		else
			ok = 1;
	}
	if(ok < 0 && pFont) {
		cairo_font_face_t * p_ff = *pFont;
		cairo_scaled_font_t * p_scf = *pFont;
		if(p_scf)
			cairo_set_scaled_font(P_Cr, p_scf);
		else if(p_ff)
			cairo_set_font_face(P_Cr, p_ff);
		P_SelectedFont = pFont;
		ok = 1;
	}
	return ok;
}

int TCanvas2::DrawTextLayout(STextLayout * pTlo)
{
	int    ok = 1;
	if(pTlo) {
		const int do_clip = 0; // pTlo->HasOption(STextLayout::fNoClip) ? 0 : 1;
		STextLayout::RenderGroup re;
		SArray glyph_list(sizeof(cairo_glyph_t));
		if(do_clip) {
			cairo_save(P_Cr);
			const FRect & r_fb = pTlo->GetBounds();
			cairo_new_path(P_Cr);
			cairo_rectangle(P_Cr, r_fb.a.X, r_fb.a.Y, r_fb.Width(), r_fb.Height());
			cairo_clip(P_Cr);
		}
		LongArray special_positions; // Индексы в массиве glyph_list, для которых необходимы доп действия (подчеркивание, например)
		for(uint i = 0; pTlo->EnumGroups(&i, &re);) {
			const uint rcnt = re.Items.getCount();
			if(re.P_Font && rcnt) {
				special_positions.clear();
				glyph_list.clear();
				SelectFont(re.P_Font);
				float prev_y = re.Items.at(i-1).P.Y;
				for(uint j = 0; j < rcnt; j++) {
					cairo_glyph_t glyph;
					const STextLayout::Item & r_item = re.Items.at(j);
					glyph.index = (ulong)r_item.GlyphIdx;
					glyph.x = r_item.P.X;
					//
					// Не понятно почему, но cairo выводит символы по Y-координате
					// с перевернутым знаком отступа от Y-координаты предыдущего символа.
					//
					/* (В версии cairo 1.14.2 проблемы уже нет) if(r_item.P.Y != prev_y)
						glyph.y = prev_y - (r_item.P.Y - prev_y);
					else*/
						glyph.y = r_item.P.Y;
					glyph_list.insert(&glyph);
					if(r_item.Flags & STextLayout::Item::fUnderscore)
						special_positions.add(glyph_list.getCount()-1);
				}
				if(re.BrushId) {
					FRect pb = pTlo->GetBkgBounds();
					Rect(pb, 0, re.BrushId);
				}
				Helper_SelectPen(0, re.PenId);
				if(Flags & fScopeRecording) {
					cairo_text_extents_t te;
					cairo_glyph_extents(P_Cr, (cairo_glyph_t *)glyph_list.at(0), glyph_list.getCount(), &te);
					double x1 = te.x_bearing;
					double y1 = te.y_bearing;
					double x2 = te.x_bearing+te.width;
					double y2 = te.y_bearing+te.height;
					cairo_user_to_device(P_Cr, &x1, &y1);
					cairo_user_to_device(P_Cr, &x2, &y2);
					TRect rc((int)floor(x1), (int)floor(y1), (int)ceil(x2), (int)ceil(y2));
					Scope.Add(rc, SCOMBINE_OR);
				}
				cairo_show_glyphs(P_Cr, (cairo_glyph_t *)glyph_list.at(0), glyph_list.getCount());
				if(special_positions.getCount()) {
					for(uint spi = 0; spi < special_positions.getCount(); spi++) {
						cairo_text_extents_t te;
						const uint gli = (uint)special_positions.get(spi);
						cairo_glyph_t * p_cgl = (cairo_glyph_t *)glyph_list.at(gli);
						cairo_glyph_extents(P_Cr, p_cgl, 1, &te);
						MoveTo(FPoint((float)p_cgl->x+1.0f, (float)p_cgl->y+2.0f));
						LineH((float)(p_cgl->x+te.width-2.0f));
						Implement_Stroke(1);
					}
				}
				SelectFont(0);
			}
		}
		if(do_clip) {
			cairo_restore(P_Cr);
		}
	}
	return ok;
}
//
//
//
SPaintObj::Gradient::Gradient(int kind, int units)
{
	Kind = kind;
	Spread = sPad;
	Unit = units;
	PctUf = 0;
	memzero(Coord, sizeof(Coord));
}

SPaintObj::Gradient & SPaintObj::Gradient::operator = (const SPaintObj::Gradient & rS)
{
	Kind = rS.Kind;
	Spread = rS.Spread;
	Unit = rS.Unit;
	PctUf = rS.PctUf;
	memcpy(Coord, rS.Coord, sizeof(Coord));
	Tf = rS.Tf;
	StopList = rS.StopList;
	return *this;
}

int SPaintObj::Gradient::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	THROW(Base::Serialize(dir, rBuf, pCtx));
	THROW(pCtx->Serialize(dir, Kind, rBuf));
	THROW(pCtx->Serialize(dir, Spread, rBuf));
	THROW(Tf.Serialize(dir, rBuf, pCtx));
	THROW(pCtx->Serialize(dir, Unit, rBuf));
	THROW(pCtx->Serialize(dir, PctUf, rBuf));
	for(uint i = 0; i < SIZEOFARRAY(Coord); i++) {
		THROW(pCtx->Serialize(dir, Coord[i], rBuf));
	}
	THROW(pCtx->Serialize(dir, &StopList, rBuf));
	CATCHZOK
	return ok;
}

int SPaintObj::Gradient::SetPrototype(const Gradient & rS)
{
	Unit = rS.Unit;
	StopList = rS.StopList;
	if(rS.Kind == Kind) {
		memcpy(Coord, rS.Coord, sizeof(Coord));
		PctUf = rS.PctUf;
	}
	return 1;
}

int SPaintObj::Gradient::SetLinearCoord(int coord, float val, int pct)
{
	if(Kind == kLinear && coord >= lcX1 && coord <= lcY2) {
		Coord[coord] = val;
		SETFLAG(PctUf, (1 << coord), pct);
		return 1;
	}
	else
		return 0;
}

int SPaintObj::Gradient::SetRadialCoord(int coord, float val, int pct)
{
	if(Kind == kRadial && coord >= rcCX && coord <= rcFR) {
		Coord[coord] = val;
		SETFLAG(PctUf, (1 << coord), pct);
		return 1;
	}
	else
		return 0;
}

float SPaintObj::Gradient::GetLinearCoord(int coord, int * pPct) const
{
	if(Kind == kLinear && coord >= lcX1 && coord <= lcY2) {
		ASSIGN_PTR(pPct, BIN(PctUf & (1 << coord)));
		return Coord[coord];
	}
	else
		return 0.0f;
}

float SPaintObj::Gradient::GetRadialCoord(int coord, int * pPct) const
{
	if(Kind == kRadial && coord >= rcCX && coord <= rcFR) {
		ASSIGN_PTR(pPct, BIN(PctUf & (1 << coord)));
		return Coord[coord];
	}
	else
		return 0.0f;
}

int SPaintObj::Gradient::GetUnits() const
{
	return (int)Unit;
}

int SPaintObj::Gradient::AddStop(float offs, SColor c)
{
	Stop s;
	s.Offs = offs;
	s.C = c;
	return StopList.insert(&s);
}

uint SPaintObj::Gradient::GetStopCount() const
	{ return StopList.getCount(); }
const SPaintObj::Gradient::Stop * SPaintObj::Gradient::GetStop(uint idx) const
	{ return (idx < StopList.getCount()) ? (Stop *)&StopList.at(idx) : 0; }
//
//
//
SParaDescr::SParaDescr()
{
	LuIndent = 0;
	RlIndent = 0;
	StartIndent = 0;
	Spacing = 0;
	Flags = 0;
	memzero(Reserve, sizeof(Reserve));
}

int SParaDescr::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	THROW(pCtx->Serialize(dir, LuIndent, rBuf));
	THROW(pCtx->Serialize(dir, RlIndent, rBuf));
	THROW(pCtx->Serialize(dir, StartIndent, rBuf));
	THROW(pCtx->Serialize(dir, Spacing, rBuf));
	THROW(pCtx->Serialize(dir, Flags, rBuf));
	CATCHZOK
	return ok;
}

int FASTCALL SParaDescr::IsEqual(const SParaDescr & rS) const
{
	return BIN(LuIndent == rS.LuIndent && RlIndent == rS.RlIndent && StartIndent == rS.StartIndent &&
		Spacing == rS.Spacing && Flags == rS.Flags);
}

int SParaDescr::GetJustif() const
{
	if(Flags & fJustRight)
		return ADJ_RIGHT;
	else if(Flags & fJustCenter)
		return ADJ_CENTER;
	else
		return ADJ_LEFT;
}

SPaintObj::Para::Para() : Base(), SParaDescr()
{
}

int SPaintObj::Para::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	THROW(Base::Serialize(dir, rBuf, pCtx));
	THROW(SParaDescr::Serialize(dir, rBuf, pCtx));
	CATCHZOK
	return ok;
}
//
//
//
const float SPaintObj::DefaultMiterLimit = 4.0f;

SPaintObj::SPaintObj(int id)
{
	Id = id;
	T = 0;
	F = 0;
	H = 0;
}

SPaintObj::~SPaintObj()
{
	Destroy();
}

void SPaintObj::ResetOwnership()
{
	F |= fNotOwner;
}

HGDIOBJ DupWinGdiObject(HGDIOBJ src)
{
	HGDIOBJ dest = 0;
	if(src) {
		int ot = ::GetObjectType(src);
		switch(ot) {
			case OBJ_BRUSH:
				{
					LOGBRUSH b;
					if(::GetObject(src, sizeof(b), &b)) {
						dest = ::CreateBrushIndirect(&b);
					}
				}
				break;
			case OBJ_PEN:
				{
					LOGPEN p;
					if(::GetObject(src, sizeof(p), &p)) {
						dest = ::CreatePenIndirect(&p);
					}
				}
				break;
			case OBJ_EXTPEN:
				{
					uint8 ep_buf[512];
					EXTLOGPEN * p_ep = (EXTLOGPEN *)ep_buf;
					if(::GetObject(src, sizeof(ep_buf), ep_buf)) {
						LOGBRUSH b;
						MEMSZERO(b);
						b.lbStyle = p_ep->elpBrushStyle;
						b.lbColor = p_ep->elpColor;
						b.lbHatch = p_ep->elpHatch;
						dest = ::ExtCreatePen(p_ep->elpPenStyle, p_ep->elpWidth, &b, p_ep->elpNumEntries, p_ep->elpStyleEntry);
					}
				}
				break;
			case OBJ_FONT:
				{
					LOGFONT f;
					if(::GetObject(src, sizeof(f), &f)) {
						dest = ::CreateFontIndirect(&f);
					}
				}
				break;
			case OBJ_BITMAP:
				{
					BITMAP bmp;
					if(::GetObject(src, sizeof(bmp), &bmp)) {
						dest = ::CreateBitmapIndirect(&bmp);
					}
				}
				break;
			case OBJ_COLORSPACE:
			case OBJ_DC:
			case OBJ_ENHMETADC:
			case OBJ_ENHMETAFILE:
			case OBJ_MEMDC:
			case OBJ_METAFILE:
			case OBJ_METADC:
			case OBJ_PAL:
			case OBJ_REGION:
				break;
		}
	}
	return dest;
}

int SPaintObj::Copy(const SPaintObj & rS, long flags)
{
	int    ok = 1;
	const  int32 save_id = Id;
	Destroy();
	Id = (flags & cfLeaveId) ? save_id : rS.Id;
	T = rS.T;
	F = (rS.F & ~fNotOwner);
	switch(T) {
		case tColor:
			H = rS.H;
			break;
		case tPen:
			if(F & fInner) {
				Pen * p_pen = new Pen;
				THROW_S(p_pen, SLERR_NOMEM);
				*p_pen = *(Pen *)rS.H;
				H = p_pen;
			}
			else {
				H = DupWinGdiObject((HGDIOBJ)rS.H);
			}
			break;
		case tBrush:
			if(F & fInner) {
				Brush * p_brush = new Brush;
				THROW_S(p_brush, SLERR_NOMEM);
				*p_brush = *(Brush *)rS.H;
				H = p_brush;
			}
			else {
				H = DupWinGdiObject((HGDIOBJ)rS.H);
			}
			break;
		case tFont:
			if(F & fInner) {
				Font * p_font = new Font;
				THROW_S(p_font, SLERR_NOMEM);
				*p_font = *(Font *)rS.H;
				H = p_font;
			}
			else {
				H = DupWinGdiObject((HGDIOBJ)rS.H);
			}
			break;
		case tGradient:
			if(F & fInner) {
				Gradient * p_grad = new Gradient(Gradient::kLinear);
				THROW_S(p_grad, SLERR_NOMEM);
				*p_grad = *(Gradient *)rS.H;
				H = p_grad;
			}
			break;
		case tBitmap:
			if(F & fInner) {
			}
			else {
				H = DupWinGdiObject((HGDIOBJ)rS.H);
			}
			break;
		case tCursor:
			if(F & fInner) {
			}
			else {
				H = CopyCursor((HCURSOR)H);
			}
			break;
	}
	CATCHZOK
	return ok;
}

int SPaintObj::CopyTo(SPaintObj * pDest)
{
	int    ok = 1;
	if(pDest) {
		pDest->Destroy();
		pDest->Id = Id;
		pDest->T = T;
		pDest->F = F;
		pDest->H = H;
		ResetOwnership();
	}
	else
		ok = 0;
	return ok;
}

int SPaintObj::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx)
{
	const  uint32 signature_mask = 0xC0B00000;

	int    ok = 1;
	uint8  ind = 0;
	uint32 signature = 0;
	uint32 version = 0;
	if(dir < 0) {
		Destroy();
		const size_t preserve_offs = rBuf.GetRdOffs();
		THROW(pCtx->Serialize(dir, signature, rBuf));
		if((signature & signature_mask) != signature_mask) {
			signature = 0;
			rBuf.SetRdOffs(preserve_offs);
		}
		else {
			version = (signature & 0x0000ffff);
		}
	}
	else if(dir > 0) {
		signature = (signature_mask | 0x0001); // version 1
		THROW(pCtx->Serialize(dir, signature, rBuf));
	}
	THROW(pCtx->Serialize(dir, Id, rBuf));
	THROW(pCtx->Serialize(dir, T, rBuf));
	THROW(pCtx->Serialize(dir, F, rBuf));
	if(dir > 0) {
		if(T == tColor) {
			uint32 _c = (uint32)H;
			THROW(pCtx->Serialize(dir, _c, rBuf));
		}
		else if(F & fInner) {
			if(H) {
				THROW(rBuf.Write(ind = 0));
				switch(T) {
					case tPen:
						THROW(((SPaintObj::Pen *)H)->Serialize(dir, rBuf, pCtx));
						break;
					case tBrush:
						THROW(((SPaintObj::Brush *)H)->Serialize(dir, rBuf, pCtx));
						break;
					case tFont:
						THROW(((SPaintObj::Font *)H)->Serialize(dir, rBuf, pCtx));
						break;
					case tGradient:
						THROW(((SPaintObj::Gradient *)H)->Serialize(dir, rBuf, pCtx));
						break;
					case tCStyle:
						THROW(((SPaintObj::CStyle *)H)->Serialize(dir, rBuf, pCtx));
						break;
					case tParagraph:
						THROW(((SPaintObj::Para *)H)->Serialize(dir, rBuf, pCtx));
						break;
				}
			}
			else {
				THROW(rBuf.Write(ind = 1));
			}
		}
	}
	else if(dir < 0) {
		if(T == tColor) {
			if(version >= 1) {
				uint32 _c = 0;
				THROW(pCtx->Serialize(dir, _c, rBuf));
				H = (void *)_c;
			}
		}
		else if(F & fInner) {
			THROW(rBuf.Read(ind));
			if(ind == 0) {
				switch(T) {
					case tPen:
						THROW_S(H = new SPaintObj::Pen, SLERR_NOMEM);
						THROW(((SPaintObj::Pen *)H)->Serialize(dir, rBuf, pCtx));
						break;
					case tBrush:
						THROW_S(H = new SPaintObj::Brush, SLERR_NOMEM);
						THROW(((SPaintObj::Brush *)H)->Serialize(dir, rBuf, pCtx));
						break;
					case tFont:
						THROW_S(H = new SPaintObj::Font, SLERR_NOMEM);
						THROW(((SPaintObj::Font *)H)->Serialize(dir, rBuf, pCtx));
						break;
					case tGradient:
						THROW_S(H = new SPaintObj::Gradient, SLERR_NOMEM);
						THROW(((SPaintObj::Gradient *)H)->Serialize(dir, rBuf, pCtx));
						break;
					case tCStyle:
						THROW(((SPaintObj::CStyle *)H)->Serialize(dir, rBuf, pCtx));
						break;
					case tParagraph:
						THROW(((SPaintObj::Para *)H)->Serialize(dir, rBuf, pCtx));
						break;
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

void SPaintObj::Destroy()
{
	if(F & fCairoPattern) {
		if(H) {
			if(!(F & fNotOwner))
				cairo_pattern_destroy((cairo_pattern_t *)H);
		}
	}
	else {
		if(!(F & fNotOwner)) {
			if(F & fInner)
				DestroyInnerHandle();
			switch(T) {
				case tPen:
					if(F & fInner)
						delete ((SPaintObj::Pen *)H);
					else
						::DeleteObject((HGDIOBJ)H);
					break;
				case tBrush:
					if(F & fInner)
						delete ((SPaintObj::Brush *)H);
					else
						::DeleteObject((HGDIOBJ)H);
					break;
				case tFont:
					if(F & fInner)
						delete ((SPaintObj::Font *)H);
					else
						::DeleteObject((HGDIOBJ)H);
					break;
				case tGradient:
					if(F & fInner)
						delete ((SPaintObj::Gradient *)H);
					break;
				case tBitmap:
					::DeleteObject((HGDIOBJ)H);
					break;
				case tCursor:
					::DestroyCursor((HCURSOR)H);
					break;
				case tCStyle:
					if(F & fInner)
						delete ((SPaintObj::CStyle *)H);
					break;
				case tParagraph:
					if(F & fInner)
						delete ((SPaintObj::Para *)H);
					break;
			}
		}
	}
	T = 0;
	F = 0;
	H = 0;
}

int SPaintObj::GetId() const
	{ return Id; }
int SPaintObj::GetType() const
	{ return T; }

SPaintObj::Pen * SPaintObj::GetPen() const
{
	return (T == tPen && F & fInner) ? (SPaintObj::Pen *)H : 0;
}

SPaintObj::Brush * SPaintObj::GetBrush() const
	{ return (T == tBrush && F & fInner) ? (SPaintObj::Brush *)H : 0; }
SPaintObj::Font * SPaintObj::GetFont() const
	{ return (T == tFont && F & fInner) ? (SPaintObj::Font *)H : 0; }
SPaintObj::Gradient * SPaintObj::GetGradient() const
	{ return (T == tGradient && F & fInner) ? (SPaintObj::Gradient *)H : 0; }
SPaintObj::Para * SPaintObj::GetParagraph() const
	{ return (T == tParagraph && F & fInner) ? (SPaintObj::Para *)H : 0; }
SPaintObj::CStyle * SPaintObj::GetCStyle() const
	{ return (T == tCStyle && F & fInner) ? (SPaintObj::CStyle *)H : 0; }
SPaintObj::operator cairo_pattern_t * () const
	{ return 0; }
SPaintObj::operator HCURSOR () const
	{ return (T == tCursor) ? (HCURSOR)H : (HCURSOR)0; }
SPaintObj::operator HBITMAP () const
	{ return (T == tBitmap) ? (HBITMAP)H : (HBITMAP)0; }
SPaintObj::operator COLORREF () const
	{ return oneof3(T, tColor, tPen, tBrush) ? (COLORREF)*((SColor *)&H) : GetColorRef(SClrBlack); }

SPaintObj::operator SColor () const
{
	SColor c = SColor(SClrBlack);
	if(T == tColor)
		c = *(SColor *)&H;
	else if(T == tPen) {
		Pen * p_pen = GetPen();
		if(p_pen)
			c = p_pen->C;
	}
	else if(T == tBrush) {
		Brush * p_brush = GetBrush();
		if(p_brush)
			c = p_brush->C;
	}
	return c;
}

SPaintObj::operator HGDIOBJ () const
{
	HGDIOBJ h_gdiobj = (HGDIOBJ)0;
	if(H) {
		if(T == tBitmap)
			h_gdiobj = (HGDIOBJ)H;
		else if(oneof3(T, tPen, tBrush, tFont)) {
			if(F & fInner) {
				const SPaintObj::Base * p_base = (const SPaintObj::Base *)H;
				if(p_base->GetSys() == dsysWinGdi)
					h_gdiobj = (HGDIOBJ)p_base->GetHandle();
			}
			else
				h_gdiobj = (HGDIOBJ)H;
		}
	}
	return h_gdiobj;
}

int SPaintObj::CreateHPen(int style, float width, SColor c)
{
	int    ok = 1;
	HPEN   handle = ::CreatePen(style, (int)round(width, 0), (COLORREF)c);
	if(handle) {
		Destroy();
		T = tPen;
		H = handle;
	}
	else
		ok = SLS.SetOsError();
	return ok;
}

int SPaintObj::CreateHBrush(int style, SColor c, int32 hatch)
{
	int    ok = 1;
	LOGBRUSH b;
	b.lbStyle = style;
	b.lbColor = (COLORREF)c;
	b.lbHatch = hatch;
	HBRUSH handle = ::CreateBrushIndirect(&b);
	if(handle) {
		Destroy();
		T = tBrush;
		H = handle;
	}
	else
		ok = SLS.SetOsError();
	return ok;
}

int SPaintObj::CreatePen(const Pen * pPen)
{
	int    ok = 1;
	if(pPen) {
		Pen * p_pen = new Pen;
		THROW_S(p_pen, SLERR_NOMEM);
		*p_pen = *pPen;
		Destroy();
		T = tPen;
		H = p_pen;
		F |= fInner;
	}
	else {
		Destroy();
		T = tUndef;
	}
	CATCHZOK
	return ok;
}

int SPaintObj::CreateBrush(const Brush * pBrush)
{
	int    ok = 1;
	if(pBrush) {
		Brush * p_brush = new Brush;
		THROW_S(p_brush, SLERR_NOMEM);
		*p_brush = *pBrush;
		Destroy();
		T = tBrush;
		H = p_brush;
		F |= fInner;
	}
	else {
		Destroy();
		T = tUndef;
	}
	CATCHZOK
	return ok;
}

int SPaintObj::CreateGradient(const Gradient * pGradient)
{
	int    ok = 1;
	if(pGradient) {
		Gradient * p_gradient = new Gradient(Gradient::kLinear);
		THROW_S(p_gradient, SLERR_NOMEM);
		*p_gradient = *pGradient;
		Destroy();
		T = tGradient;
		H = p_gradient;
		F |= fInner;
	}
	else {
		Destroy();
		T = tUndef;
	}
	CATCHZOK
	return ok;
}

int SPaintObj::CreatePen(int style, float width, SColor c)
{
	int    ok = 1;
	Pen * p_pen = new Pen;
	if(p_pen) {
		p_pen->C = c;
		p_pen->S = style;
		p_pen->W = width;
		Destroy();
		T = tPen;
		H = p_pen;
		F |= fInner;
	}
	else
		ok = SLS.SetError(SLERR_NOMEM);
	return ok;
}

int SPaintObj::CreateBrush(int style, SColor c, int32 hatch)
{
	int    ok = 1;
	Brush * p_brush = new Brush;
	if(p_brush) {
		p_brush->C = c;
		p_brush->S = style;
		p_brush->Hatch = (int8)hatch;
		Destroy();
		T = tBrush;
		H = p_brush;
		F |= fInner;
	}
	else
		ok = SLS.SetError(SLERR_NOMEM);
	return ok;
}

int SPaintObj::CreateFont(const char * pFace, int height, int flags)
{
	int    ok = 1;
	Font * p_font = new Font;
	if(p_font) {
		p_font->Face = pFace;
		p_font->Size = (int16)height;
		p_font->Flags = flags;
		Destroy();
		T = tFont;
		H = p_font;
		F |= fInner;
	}
	else
		ok = SLS.SetError(SLERR_NOMEM);
	return ok;
}

int SPaintObj::CreateParagraph()
{
	int    ok = 1;
	Para * p_para = new Para;
	if(p_para) {
		Destroy();
		T = tParagraph;
		H = p_para;
		F |= fInner;
	}
	else
		ok = SLS.SetError(SLERR_NOMEM);
	return ok;
}

int SPaintObj::CreateCStyle(int fontId, int penId, int brushId)
{
	int    ok = 1;
	CStyle * p_cs = new CStyle;
	if(p_cs) {
		p_cs->FontId = fontId;
		p_cs->PenId = penId;
		p_cs->BrushId = brushId;
		Destroy();
		T = tCStyle;
		H = p_cs;
		F |= fInner;
	}
	else
		ok = SLS.SetError(SLERR_NOMEM);
	return ok;
}

int SPaintObj::CreateGradientLinear(const FRect & rBound)
{
	int    ok = 1;
	Gradient * p_grad = new Gradient(Gradient::kLinear);
	if(p_grad) {
		p_grad->Kind = Gradient::kLinear;
		Destroy();
		T = tGradient;
		H = p_grad;
		F |= fInner;
	}
	else
		ok = SLS.SetError(SLERR_NOMEM);
	return ok;
}

int SPaintObj::CreateGradientRadial(const FShape::Circle & rBound)
{
	int    ok = 1;
	Gradient * p_grad = new Gradient(Gradient::kRadial);
	if(p_grad) {
		p_grad->Kind = Gradient::kRadial;
		Destroy();
		T = tGradient;
		H = p_grad;
		F |= fInner;
	}
	else
		ok = SLS.SetError(SLERR_NOMEM);
	return ok;
}

int SPaintObj::AddGradientStop(float offs, SColor c)
{
	int    ok = 0;
	if(T == tGradient) {
		if(F & fInner) {
			Gradient * p_grad = (Gradient *)H;
			if(p_grad) {
				p_grad->AddStop(offs, c);
				ok = 1;
			}
		}
	}
	return ok;
}

int FASTCALL _SetPaintObjInnerHandle(SPaintObj::Base * pBase, SDrawSystem sys, /*uint32*/void * h)
{
	if(pBase) {
		pBase->Handle = h;
		pBase->Sys = sys;
		return h ? 1 : 0;
	}
	else
		return 0;
}

int FASTCALL SPaintObj::ProcessInnerHandle(SDrawContext * pCtx, int create)
{
	int    ok = -1;
	if(H && F & fInner) {
		Base * p_base = (Base *)H;
		void * hdl = p_base->GetHandle();
		if((create && !hdl) || (!create && hdl)) {
			SDrawSystem dsys = create ? (pCtx ? *pCtx : dsysNone) : p_base->GetSys();
			switch(T) {
				case tPen:
					if(dsys == dsysWinGdi) {
						if(create) {
							SPaintObj::Pen * p_pen = (SPaintObj::Pen *)H;
							HPEN   handle = ::CreatePen(p_pen->S, (int)round(p_pen->W, 0), (COLORREF)p_pen->C);
							ok = _SetPaintObjInnerHandle(p_pen, dsys, /*(uint32)*/handle);
						}
						else {
							::DeleteObject((HGDIOBJ)hdl);
							_SetPaintObjInnerHandle(p_base, dsysNone, 0);
							ok = 1;
						}
					}
					break;
				case tBrush:
					if(dsys == dsysWinGdi) {
						if(create) {
							SPaintObj::Brush * p_brush = (SPaintObj::Brush *)H;
							LOGBRUSH b;
							b.lbStyle = p_brush->S;
							b.lbColor = (COLORREF)p_brush->C;
							b.lbHatch = p_brush->Hatch;
							ok = _SetPaintObjInnerHandle(p_brush, dsys, /*(uint32)*/::CreateBrushIndirect(&b));
						}
						else {
							::DeleteObject((HGDIOBJ)hdl);
							_SetPaintObjInnerHandle(p_base, dsysNone, 0);
							ok = 1;
						}
					}
					break;
				case tFont:
					if(dsys == dsysWinGdi) {
						if(create) {
							SPaintObj::Font * p_font = (SPaintObj::Font *)H;
							LOGFONTW log_fontw;
							ok = _SetPaintObjInnerHandle(p_font, dsys, /*(uint32)*/CreateFontIndirectW(p_font->MakeLogFont(&log_fontw)));
						}
						else {
							::DeleteObject((HGDIOBJ)hdl);
							_SetPaintObjInnerHandle(p_base, dsysNone, 0);
							ok = 1;
						}
					}
					else if(dsys == dsysCairo) {
						if(create) {
							LMatrix2D mtx_font, mtx_ctm;
							InnerFontDescr * p_descr = new InnerFontDescr;
							if(p_descr) {
								SPaintObj::Font * p_font = (SPaintObj::Font *)H;
								LOGFONTW log_fontw;
								p_font->MakeLogFont(&log_fontw);
								log_fontw.lfHeight = 0;
								p_descr->Hf = ::CreateFontIndirectW(&log_fontw);
								log_fontw.lfWidth = 0;
								log_fontw.lfOrientation = 0;
								log_fontw.lfEscapement = 0;
								p_descr->P_CrFace = cairo_win32_font_face_create_for_logfontw_hfont(&log_fontw, p_descr->Hf);
								cairo_font_options_t * p_options = cairo_font_options_create();
								if(p_font->Flags & SFontDescr::fAntialias) {
									cairo_font_options_set_antialias(p_options, /*CAIRO_ANTIALIAS_DEFAULT*/CAIRO_ANTIALIAS_BEST);
								}
								mtx_font.InitScale(p_font->Size, p_font->Size);
								p_descr->P_CrScFont = cairo_scaled_font_create(p_descr->P_CrFace, (cairo_matrix_t *)&mtx_font, (cairo_matrix_t *)&mtx_ctm, p_options);
								if(p_descr->P_CrScFont) {
									cairo_font_extents_t font_ext;
									cairo_scaled_font_extents(p_descr->P_CrScFont, &font_ext);
									p_font->LineAdv = (float)font_ext.height;
								}
								cairo_font_options_destroy(p_options);
								ok = _SetPaintObjInnerHandle(p_font, dsys, /*(uint32)*/p_descr);
							}
						}
						else {
							InnerFontDescr * p_descr = (InnerFontDescr *)hdl;
							delete p_descr;
							_SetPaintObjInnerHandle(p_base, dsysNone, 0);
							ok = 1;
						}
					}
					break;
				case tGradient:
					if(dsys == dsysCairo) {
						if(create) {
#if 0 // {
							SPaintObj::Gradient * p_grad = (SPaintObj::Gradient *)H;
							cairo_pattern_t * p_pattern = 0;
							int    pct;
							if(p_grad->Kind == SPaintObj::Gradient::kLinear) {
								float x1 = p_grad->GetLinearCoord(SPaintObj::Gradient::lcX1, &pct);
								float y1 = p_grad->GetLinearCoord(SPaintObj::Gradient::lcY1, &pct);
								float x2 = p_grad->GetLinearCoord(SPaintObj::Gradient::lcX2, &pct);
								float y2 = p_grad->GetLinearCoord(SPaintObj::Gradient::lcY2, &pct);
								p_pattern = cairo_pattern_create_linear(x1, y1, x2, y2);
							}
							else if(p_grad->Kind == SPaintObj::Gradient::kRadial) {
								float cx = p_grad->GetRadialCoord(SPaintObj::Gradient::rcCX, &pct);
								float cy = p_grad->GetRadialCoord(SPaintObj::Gradient::rcCY, &pct);
								float r  = p_grad->GetRadialCoord(SPaintObj::Gradient::rcR, &pct);
								float fx = p_grad->GetRadialCoord(SPaintObj::Gradient::rcFX, &pct);
								float fy = p_grad->GetRadialCoord(SPaintObj::Gradient::rcFY, &pct);
								float fr = p_grad->GetRadialCoord(SPaintObj::Gradient::rcFR, &pct);
		    					p_pattern = cairo_pattern_create_radial(fx, fy, fr, cx, cy, r);
							}
							else if(p_grad->Kind == SPaintObj::Gradient::kConical) {
							}
							if(p_pattern) {
								for(uint i = 0; i < p_grad->GetStopCount(); i++) {
									const SPaintObj::Gradient::Stop * p_stop = p_grad->GetStop(i);
									if(p_stop) {
										cairo_pattern_add_color_stop_rgba(p_pattern, p_stop->Offs,
											p_stop->C.RedF(), p_stop->C.GreenF(), p_stop->C.BlueF(), p_stop->C.AlphaF());
									}
								}
								switch(p_grad->Spread) {
	    							case SPaintObj::Gradient::sRepeat:
										cairo_pattern_set_extend(p_pattern, CAIRO_EXTEND_REPEAT);
										break;
	    							case SPaintObj::Gradient::sReflect:
										cairo_pattern_set_extend(p_pattern, CAIRO_EXTEND_REFLECT);
										break;
	    							default:
										cairo_pattern_set_extend(p_pattern, CAIRO_EXTEND_NONE);
										break;
								}
								cairo_pattern_set_filter(p_pattern, CAIRO_FILTER_BILINEAR);
								_SetPaintObjInnerHandle(p_grad, sys, (uint32)p_pattern);
								ok = 1;
							}
#endif // } 0
						}
						else {
							/*
							cairo_pattern_destroy((cairo_pattern_t *)p_base->GetHandle());
							_SetPaintObjInnerHandle(p_base, dsysNone, 0);
							ok = 1;
							*/
						}
					}
					break;
			}
		}
	}
	return ok;
}

int FASTCALL SPaintObj::CreateInnerHandle(SDrawContext & rCtx)
{
	return ProcessInnerHandle(&rCtx, 1);
}

int FASTCALL SPaintObj::DestroyInnerHandle()
{
	return ProcessInnerHandle(0, 0);
}

int SPaintObj::CreateColor(SColor c)
{
	Destroy();
	T = tColor;
	memcpy(&H, &c, sizeof(c));
	return 1;
}

int SPaintObj::SetBitmap(uint bmpId)
{
	int    ok = 1;
	HBITMAP handle = APPL->LoadBitmap(bmpId);
	if(handle) {
		Destroy();
		T = tBitmap;
		H = handle;
	}
	else
		ok = SLS.SetOsError();
	return ok;
}

int SPaintObj::SetFont(HFONT handle)
{
	int    ok = 1;
	if(handle) {
		Destroy();
		T = tFont;
		H = handle;
	}
	return ok;
}

int SPaintObj::CreateCursor(uint cursorId)
{
	int    ok = 1;
	HCURSOR handle = (HCURSOR)LoadImage(TProgram::GetInst(), MAKEINTRESOURCE(cursorId), IMAGE_CURSOR,
		0, 0, LR_DEFAULTSIZE);
	if(handle) {
		Destroy();
		T = tCursor;
		H = handle;
	}
	else
		ok = SLS.SetOsError();
	return ok;
}
//
//
//
#define INIT_DYN_IDENT 100000

SPaintToolBox::SPaintToolBox() : TSArray <SPaintObj> (aryDataOwner|aryEachItem), Hash(128, 1),
	GlyphList(sizeof(GlyphEntry))
{
	Init();
}

SPaintToolBox::~SPaintToolBox()
{
	freeAll();
}

SPaintToolBox & SPaintToolBox::Init()
{
	freeAll();
	DynIdentCount = INIT_DYN_IDENT;
	Hash.Clear();
	State = 0;
	DefaultPenId = 0;
	return *this;
}

int SPaintToolBox::Copy(const SPaintToolBox & rS)
{
	int    ok = 1;
	DynIdentCount = rS.DynIdentCount;
	THROW(Hash.Copy(rS.Hash));
	for(uint i = 0; i < rS.getCount(); i++) {
		const SPaintObj & r_src_obj = rS.at(i);
		SPaintObj * p_obj = CreateObj(r_src_obj.GetId());
		THROW(p_obj);
		if(p_obj)
			THROW(p_obj->Copy(r_src_obj));
	}
	State = rS.State;
	CATCHZOK
	return ok;
}

int SPaintToolBox::CopyToolFrom(const SPaintToolBox & rS, int toolIdent)
{
	int    new_ident = 0;
	SString symb;
	SPaintObj * p_new_obj = 0;
	const SPaintObj * p_src_obj = rS.GetObj(toolIdent);
	THROW(p_src_obj);
	if(rS.GetSymb(toolIdent, symb) && (p_new_obj = GetObjBySymb(symb, p_src_obj->GetType())) != 0)
		new_ident = p_new_obj->GetId();
	else {
		THROW(new_ident = CreateDynIdent(symb.NotEmpty() ? symb.cptr() : 0));
		THROW(p_new_obj = CreateObj(new_ident));
		THROW(p_new_obj->Copy(*p_src_obj, SPaintObj::cfLeaveId));
	}
	{
		//
		// Кисть (SPaintObj::Brush) может содержать ссылку на pattern. Этот
		// объект также необходимо перенести.
		//
		const SPaintObj::Brush * p_brush = p_src_obj->GetBrush();
		if(p_brush && p_brush->IdPattern) {
			SPaintObj::Brush * p_new_brush = p_new_obj->GetBrush();
			if(p_new_brush)
				THROW(p_new_brush->IdPattern = CopyToolFrom(rS, p_brush->IdPattern)); // @recursion
		}
	}
	CATCH
		new_ident = 0;
	ENDCATCH
	return new_ident;
}

int SPaintToolBox::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	uint32 c = getCount();
	THROW(pCtx->Serialize(dir, c, rBuf));
	THROW(pCtx->Serialize(dir, DynIdentCount, rBuf));
	THROW(Hash.Serialize(dir, rBuf, pCtx));
	if(dir > 0) {
		for(uint i = 0; i < c; ++i) {
			THROW(at(i).Serialize(dir, rBuf, pCtx));
		}
	}
	else if(dir < 0) {
		freeAll();
		for(uint i = 0; i < c; ++i) {
			//
			// Объект создается пустым и сразу заносится в массив.
			// Важно то, что деструктор ~SPaintObj не разрушит объект, уже находящийся в контейнере.
			//
			SPaintObj obj;
			THROW(insert(&obj));
			THROW(at(i).Serialize(dir, rBuf, pCtx));
		}
		sort(CMPF_LONG);
	}
	CATCHZOK
	return ok;
}

int SPaintToolBox::CreateReservedObjects()
{
	int    ok = 1;
	if(State & stReservedObjects) {
		ok = -1;
	}
	else {
		CreateBrush(rbrWindow, SPaintObj::bsSolid, SColor(GetSysColor(COLOR_WINDOW)), 0);
		CreateBrush(rbrWindowFrame, SPaintObj::bsSolid, SColor(GetSysColor(COLOR_WINDOWFRAME)), 0);
		CreateBrush(rbr3DDkShadow, SPaintObj::bsSolid, SColor(GetSysColor(COLOR_3DDKSHADOW)), 0);
		CreateBrush(rbr3DLight, SPaintObj::bsSolid, SColor(GetSysColor(COLOR_3DLIGHT)), 0);
		CreateBrush(rbr3DFace, SPaintObj::bsSolid, SColor(GetSysColor(COLOR_3DFACE)), 0);
		CreateBrush(rbr3DShadow, SPaintObj::bsSolid, SColor(GetSysColor(COLOR_3DSHADOW)), 0);
		CreateBrush(rbr3DHilight, SPaintObj::bsSolid, SColor(GetSysColor(COLOR_3DHILIGHT)), 0);
		State |= stReservedObjects;
	}
	return ok;
}

int FASTCALL SPaintToolBox::CreateDynIdent(const char * pSymb)
{
	int    ident = SearchSymb(pSymb);
	if(!ident) {
		ident = ++DynIdentCount;
		if(!isempty(pSymb)) {
			uint   uid = (uint)ident;
			if(!Hash.Add(pSymb, uid, 0))
				ident = 0;
		}
	}
	return ident;
}

int FASTCALL SPaintToolBox::SearchSymb(const char * pSymb) const
{
	uint   ident = 0;
	return (!isempty(pSymb) && Hash.Search(pSymb, &ident, 0)) ? (int)ident : 0;
}

int SPaintToolBox::GetSymb(int ident, SString & rSymb) const
{
	return Hash.GetByAssoc((uint)ident, rSymb);
}

int FASTCALL SPaintToolBox::SearchColor(SColor c) const
{
	for(uint i = 0; i < getCount(); i++) {
		SPaintObj & r_obj = at(i);
		if(r_obj.GetType() == SPaintObj::tColor && r_obj == c)
			return r_obj.GetId();
	}
	return 0;
}

//virtual
void FASTCALL SPaintToolBox::freeItem(void * pItem)
{
	if(pItem)
		((SPaintObj *)pItem)->Destroy();
}

SPaintObj * FASTCALL SPaintToolBox::GetObj(int ident) const
{
	uint   pos = 0;
	return (bsearch(&ident, &pos, CMPF_LONG)) ? &at(pos) : 0;
}

SPaintObj * FASTCALL SPaintToolBox::GetObjBySymb(const char * pSymb, int type) const
{
	SPaintObj * p_obj = 0;
	int    ident = SearchSymb(pSymb);
	if(ident) {
		p_obj = GetObj(ident);
		if(p_obj && type && p_obj->GetType() != type)
			p_obj = 0; // @error
	}
	return p_obj;
}

HGDIOBJ FASTCALL SPaintToolBox::Get(int ident) const
{
	SPaintObj * p_obj = GetObj(ident);
	return p_obj ? (HGDIOBJ)*p_obj : 0;
}

int FASTCALL SPaintToolBox::GetType(int ident) const
{
	SPaintObj * p_obj = GetObj(ident);
	return p_obj ? p_obj->GetType() : 0;
}

SPaintObj * FASTCALL SPaintToolBox::CreateObj(int ident)
{
	SPaintObj * p_obj = GetObj(ident);
	if(!p_obj) {
		SPaintObj obj(ident);
		uint   pos = 0;
		if(ordInsert(&obj, &pos, CMPF_LONG))
			p_obj = &at(pos);
	}
	return p_obj;
}

int FASTCALL SPaintToolBox::DeleteObj(int ident)
{
	int    ok = 1;
	uint   pos = 0;
	if(bsearch(&ident, &pos, CMPF_LONG)) {
		atFree(pos);
	}
	else
		ok = 0;
	return ok;
}

int SPaintToolBox::SetPen(int ident, int style, int width, COLORREF c)
{
	SPaintObj * p_obj = CreateObj(ident);
	return BIN(p_obj && p_obj->CreateHPen(style, (float)width, SColor(c)));
}

int SPaintToolBox::SetDefaultPen(int style, int width, SColor c)
{
	int    ok = 1;
	int32  pen_id = 0;
	SPaintObj * p_new_obj = 0;
	THROW(pen_id = CreateDynIdent("$pen-default"));
	THROW(p_new_obj = CreateObj(pen_id));
	THROW(p_new_obj->CreatePen(style, (float)width, c));
	DefaultPenId = pen_id;
	CATCHZOK
	return ok;
}

int32 SPaintToolBox::GetDefaultPen()
{
	int32  pen_id = 0;
	if(DefaultPenId == 0) {
		THROW(SetDefaultPen(SPaintObj::psSolid, 1, SClrBlack));
	}
	pen_id = DefaultPenId;
	CATCH
		pen_id = 0;
	ENDCATCH
	return pen_id;
}

int SPaintToolBox::SetBrush(int ident, int style, COLORREF c, int32 hatch)
{
	SPaintObj * p_obj = CreateObj(ident);
	return BIN(p_obj && p_obj->CreateHBrush(style, SColor(c), hatch));
}

int SPaintToolBox::CreatePen(int ident, int style, float width, SColor c)
{
	/*
	SPaintObj * p_obj = CreateObj(ident);
	return BIN(p_obj && p_obj->CreatePen(style, width, c));
	*/
	SPaintObj * p_obj = 0;
	if(!ident) {
		SPaintObj::Pen pen;
		pen.C = c;
		pen.S = style;
		pen.W = width;
		const uint co = getCount();
		for(uint i = 0; !ident && i < co; i++) {
			const SPaintObj::Pen * p_pen = at(i).GetPen();
			if(p_pen && p_pen->IsEqual(pen))
				ident = at(i).GetId();
		}
		if(!ident) {
			THROW(ident = CreateDynIdent(0));
			THROW(p_obj = CreateObj(ident));
		}
	}
	else {
		THROW(p_obj = CreateObj(ident));
	}
	if(p_obj) {
		THROW(p_obj->CreatePen(style, width, c));
	}
	CATCH
		ident = 0;
	ENDCATCH
	return ident;
}

int SPaintToolBox::CreateBrush(int ident, int style, SColor c, int32 hatch, int patternId)
{
	/*
	SPaintObj * p_obj = CreateObj(ident);
	return BIN(p_obj && p_obj->CreateBrush(style, c, hatch));
	*/
	SPaintObj * p_obj = 0;
	SPaintObj::Brush brush;
	brush.C = c;
	brush.S = patternId ? SPaintObj::bsPattern : style;
	brush.Hatch = (int8)hatch;
	brush.IdPattern = patternId; // @v9.1.11
	if(!ident) {
		const uint co = getCount();
		for(uint i = 0; !ident && i < co; i++) {
			const SPaintObj::Brush * p_brush = at(i).GetBrush();
			if(p_brush && p_brush->IsEqual(brush))
				ident = at(i).GetId();
		}
		if(!ident) {
			THROW(ident = CreateDynIdent(0));
			THROW(p_obj = CreateObj(ident));
		}
	}
	else {
		THROW(p_obj = CreateObj(ident));
	}
	if(p_obj) {
		if(patternId) {
			THROW(p_obj->CreateBrush(&brush));
		}
		else {
			THROW(p_obj->CreateBrush(style, c, hatch));
		}
	}
	CATCH
		ident = 0;
	ENDCATCH
	return ident;
}

int SPaintToolBox::CreateFont(int ident, const char * pFace, int height, int flags)
{
	SPaintObj * p_obj = 0;
	if(!ident) {
		SFontDescr fd(pFace, height, flags);
		const uint co = getCount();
		for(uint i = 0; !ident && i < co; i++) {
			const SPaintObj::Font * p_font = at(i).GetFont();
			if(p_font && p_font->IsEqual(fd))
				ident = at(i).GetId();
		}
		if(!ident) {
			THROW(ident = CreateDynIdent(0));
			THROW(p_obj = CreateObj(ident));
		}
	}
	else {
		THROW(p_obj = CreateObj(ident));
	}
	if(p_obj)
		THROW(p_obj->CreateFont(pFace, height, flags));
	CATCH
		ident = 0;
	ENDCATCH
	return ident;
}

SPaintObj::Para * SPaintToolBox::GetParagraph(int ident)
{
	SPaintObj::Para * p_para = 0;
	SPaintObj * p_obj = GetObj(ident);
	if(p_obj && p_obj->GetType() == SPaintObj::tParagraph) {
		SDrawContext ctx((cairo_t *)0);
		p_obj->CreateInnerHandle(ctx);
		p_para = p_obj->GetParagraph();
	}
	return p_para;
}

int SPaintToolBox::CreateParagraph(int ident, const SParaDescr * pDescr)
{
	SPaintObj * p_obj = 0;
	if(!ident) {
		SParaDescr fd;
		RVALUEPTR(fd, pDescr);
		for(uint i = 0; !ident && i < getCount(); i++) {
			const SPaintObj::Para * p_para = at(i).GetParagraph();
			if(p_para && p_para->IsEqual(fd))
				ident = at(i).GetId();
		}
		if(!ident) {
			THROW(ident = CreateDynIdent(0));
			THROW(p_obj = CreateObj(ident));
		}
	}
	else {
		THROW(p_obj = CreateObj(ident));
	}
	if(p_obj) {
		THROW(p_obj->CreateParagraph());
		SPaintObj::Para * p_para = GetParagraph(ident);
		if(p_para && pDescr) {
			*((SParaDescr *)p_para) = *pDescr;
		}
	}
	CATCH
		ident = 0;
	ENDCATCH
	return ident;
}

int SPaintToolBox::CreateColor(int ident, SColor c)
{
	SPaintObj * p_obj = 0;
	if(!ident) {
		const uint co = getCount();
		for(uint i = 0; !ident && i < co; i++) {
			SPaintObj & r_item = at(i);
			//if(r_item.GetType() == SPaintObj::tColor && (SColor)r_item == c)
			if (r_item.GetType() == SPaintObj::tColor && r_item.operator SColor() == c)
				ident = r_item.GetId();
		}
		if(!ident) {
			THROW(ident = CreateDynIdent(0));
			THROW(p_obj = CreateObj(ident));
			THROW(p_obj->CreateColor(c));
		}
	}
	else {
		THROW(p_obj = CreateObj(ident));
		THROW(p_obj->CreateColor(c));
	}
	CATCH
		ident = 0;
	ENDCATCH
	return ident;
}

int SPaintToolBox::CreateCStyle(int ident, int fontId, int penId, int brushId)
{
	SPaintObj * p_obj = 0;
	if(!ident) {
		SPaintObj::CStyle key;
		key.FontId = fontId;
		key.PenId = penId;
		key.BrushId = brushId;
		for(uint i = 0; !ident && i < getCount(); i++) {
			const SPaintObj::CStyle * p_cs = at(i).GetCStyle();
			if(p_cs && p_cs->IsEqual(key))
				ident = at(i).GetId();
		}
		if(!ident) {
			THROW(ident = CreateDynIdent(0));
			THROW(p_obj = CreateObj(ident));
			THROW(p_obj->CreateCStyle(fontId, penId, brushId));
		}
	}
	else {
		THROW(p_obj = CreateObj(ident));
		THROW(p_obj->CreateCStyle(fontId, penId, brushId));
	}
	CATCH
		ident = 0;
	ENDCATCH
	return ident;
}

int SPaintToolBox::CreateGradientLinear(int ident, const FRect & rBounds)
{
	//SPaintObj * p_obj = CreateObj(ident);
	//return BIN(p_obj && p_obj->CreateGradientLinear(rBounds));
	SPaintObj * p_obj = 0;
	if(!ident) {
		THROW(ident = CreateDynIdent(0));
	}
	THROW(p_obj = CreateObj(ident));
	THROW(p_obj->CreateGradientLinear(rBounds));
	CATCH
		ident = 0;
	ENDCATCH
	return ident;
}

int SPaintToolBox::CreateGradientRadial(int ident, const FShape::Circle & rBounds)
{
	SPaintObj * p_obj = CreateObj(ident);
	return BIN(p_obj && p_obj->CreateGradientRadial(rBounds));
}

int SPaintToolBox::AddGradientStop(int ident, float offs, SColor c)
{
	SPaintObj * p_obj = GetObj(ident);
	return p_obj ? p_obj->AddGradientStop(offs, c) : 0;
}

int SPaintToolBox::SetFont(int ident, HFONT handle)
{
	SPaintObj * p_obj = CreateObj(ident);
	return BIN(p_obj && p_obj->SetFont(handle));
}

int SPaintToolBox::SetColor(int ident, COLORREF c)
{
	SPaintObj * p_obj = CreateObj(ident);
	return BIN(p_obj && p_obj->CreateColor(SColor(c)));
}

int SPaintToolBox::SetBitmap(int ident, uint bmpId)
{
	SPaintObj * p_obj = CreateObj(ident);
	return BIN(p_obj && p_obj->SetBitmap(bmpId));
}

HBITMAP FASTCALL SPaintToolBox::GetBitmap(int ident) const
{
	SPaintObj * p_obj = GetObj(ident);
	return p_obj ? (HBITMAP)*p_obj : 0;
}

COLORREF FASTCALL SPaintToolBox::GetColor(int ident) const
{
	SPaintObj * p_obj = GetObj(ident);
	return p_obj ? (COLORREF)*p_obj : GetColorRef(SClrBlack);
}

int SPaintToolBox::GetColor(int ident, COLORREF * pC) const
{
	int    ok = 1;
	SPaintObj * p_obj = GetObj(ident);
	if(p_obj && p_obj->GetType() == SPaintObj::tColor) {
		ASSIGN_PTR(pC, (COLORREF)*p_obj);
	}
	else {
		ASSIGN_PTR(pC, GetColorRef(SClrBlack));
		ok = 0;
	}
	return ok;
}

int SPaintToolBox::CreateCursor(int ident, uint cursorId)
{
	SPaintObj * p_obj = CreateObj(ident);
	return BIN(p_obj && p_obj->CreateCursor(cursorId));
}

HCURSOR FASTCALL SPaintToolBox::GetCursor(int ident) const
{
	SPaintObj * p_obj = GetObj(ident);
	return p_obj ? (HCURSOR)*p_obj : 0;
}

struct __GlyphKey {
	int32  I;
	uint16 C;
};

IMPL_CMPFUNC(__GlyphKey, i1, i2) { RET_CMPCASCADE2((const __GlyphKey *)i1, (const __GlyphKey *)i2, I, C); }

SPaintObj::Font * SPaintToolBox::GetFont(SDrawContext & rCtx, int fontIdent)
{
	SPaintObj::Font * p_font = 0;
	SPaintObj * p_obj = GetObj(fontIdent);
	if(p_obj && p_obj->GetType() == SPaintObj::tFont) {
		p_obj->CreateInnerHandle(rCtx);
		p_font = p_obj->GetFont();
	}
	return p_font;
}

int SPaintToolBox::GetGlyphId(SDrawContext & rCtx, int fontIdent, wchar_t chr)
{
	int    id = 0;
	__GlyphKey key;
	key.I = fontIdent;
	key.C = chr;
	uint   pos = 0;
	if(GlyphList.lsearch(&key, &pos, PTR_CMPFUNC(__GlyphKey))) {
		id = (int)(pos+1);
	}
	else {
		SGlyph glyph;
		SPaintObj::Font * p_font = GetFont(rCtx, fontIdent);
		if(p_font && p_font->GetGlyph(rCtx, chr, &glyph)) {
			GlyphEntry entry;
			entry.I = fontIdent;
			entry.G = glyph;
			GlyphList.insert(&entry);
			id = GlyphList.getCount();
		}
	}
	return id;
}

const SGlyph * FASTCALL SPaintToolBox::GetGlyph(int glyphId) const
{
	if(glyphId > 0 && glyphId <= (int)GlyphList.getCount())
		return &(((const GlyphEntry *)GlyphList.at(glyphId-1))->G);
	else
		return 0;
}
