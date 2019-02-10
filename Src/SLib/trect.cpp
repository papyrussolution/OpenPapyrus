// TRECT.CPP
// ..2007, 2008, 2010, 2011, 2014, 2015, 2016, 2017, 2018, 2019
//
#include <slib.h>
#include <tv.h>
#pragma hdrstop
//
//
//
const SColor ZEROCOLOR(0, 0, 0, 0);
//
//
//
int FASTCALL SIntersectRect(RECT & rDst, const RECT & rSrc1, const RECT & rSrc2)
{
	rDst.left  = MAX(rSrc1.left, rSrc2.left);
	rDst.right = MIN(rSrc1.right, rSrc2.right);
	//
	// check for empty rect
	//
	if(rDst.left < rDst.right) {
		rDst.top = MAX(rSrc1.top, rSrc2.top);
		rDst.bottom = MIN(rSrc1.bottom, rSrc2.bottom);
		//
		// check for empty rect
		//
		if(rDst.top < rDst.bottom)
			return 1; // not empty
	}
	//
	// empty rect
	//
	rDst.left = 0;
	rDst.top = 0;
	rDst.right = 0;
	rDst.bottom = 0;
	return 0;
}

int FASTCALL SIntersectRect(const RECT & rSrc1, const RECT & rSrc2)
{
	RECT   dst;
	dst.left  = MAX(rSrc1.left, rSrc2.left);
	dst.right = MIN(rSrc1.right, rSrc2.right);
	//
	// check for empty rect
	//
	if(dst.left < dst.right) {
		dst.top = MAX(rSrc1.top, rSrc2.top);
		dst.bottom = MIN(rSrc1.bottom, rSrc2.bottom);
		//
		// check for empty rect
		//
		if(dst.top < dst.bottom)
			return 1; // not empty intersection
	}
	//
	// empty intersection
	//
	return 0;
}

void FASTCALL SInflateRect(RECT & rRect, int cx, int cy)
{
    rRect.left   -= cx;
    rRect.right  += cx;
    rRect.top    -= cy;
    rRect.bottom += cy;
}
//
// TPoint
//
uint32 TPoint::towparam() const
{
	return MAKEWPARAM(x, y);
}

TPoint::operator POINT() const
{
	POINT p;
	p.x = x;
	p.y = y;
	return p;
}

TPoint::operator FPoint () const
{
	FPoint p;
	p.Set((float)x, (float)y);
	return p;
}

int TPoint::IsZero() const
{
	return (x == 0 && y == 0);
}

TPoint FASTCALL TPoint::operator = (TPoint p)
{
	x = p.x;
	y = p.y;
	return *this;
}

TPoint FASTCALL TPoint::operator = (int v)
{
	x = y = (int16)v;
	return *this;
}

TPoint FASTCALL TPoint::operator = (POINT p)
{
	x = (int16)p.x;
	y = (int16)p.y;
	return *this;
}

TPoint FASTCALL TPoint::setwparam(uint32 wp)
{
	x = LOWORD(wp);
	y = HIWORD(wp);
	return *this;
}

TPoint TPoint::Set(int _x, int _y)
{
	x = _x;
	y = _y;
	return *this;
}

TPoint TPoint::Add(int add_x, int add_y)
{
	x += add_x;
	y += add_y;
	return *this;
}

TPoint TPoint::operator += (TPoint adder)
{
	x += adder.x;
	y += adder.y;
	return *this;
}

TPoint TPoint::operator -= (TPoint subber)
{
	x -= subber.x;
	y -= subber.y;
	return *this;
}

TPoint operator + (TPoint pnt, int adder)
{
	pnt.x += adder;
	pnt.y += adder;
	return pnt;
}

TPoint operator - (TPoint one, TPoint two)
{
	TPoint result;
	result.x = one.x - two.x;
	result.y = one.y - two.y;
	return result;
}

TPoint operator + (TPoint one, TPoint two)
{
	TPoint result;
	result.x = one.x + two.x;
	result.y = one.y + two.y;
	return result;
}

int operator == (TPoint one, TPoint two)
{
	return (one.x == two.x && one.y == two.y);
}

int operator != (TPoint one, TPoint two)
{
	return (one.x != two.x || one.y != two.y);
}
//
// FRect
//
FRect::FRect()
{
}

FRect::FRect(float width, float height) : a(), b(width, height)
{
}

FRect::FRect(const TRect & r)
{
	a = r.a;
	b = r.b;
}

FRect & FASTCALL FRect::operator = (FPoint p)
{
	a = 0.0f;
	b = p;
	return *this;
}

float FRect::Width() const
{
	return (b.X - a.X);
}

float FRect::Height() const
{
	return (b.Y - a.Y);
}

FPoint FRect::GetSize() const
{
	return FPoint((b.X - a.X), (b.Y - a.Y));
}

FPoint FRect::GetCenter() const
{
	return FPoint((a.X + b.X) / 2.0f, (a.Y + b.Y) / 2.0f);
}

double FRect::Ratio() const
{
	double w = Width();
	return (w != 0.0) ? (Height() / w) : SMathConst::Max;
}

FRect & FRect::Around(FPoint center, FPoint size)
{
	const float hx = (size.X / 2.0f);
	const float hy = (size.Y / 2.0f);
	a.X = center.X - hx;
	a.Y = center.Y - hy;
	b.X = center.X + hx;
	b.Y = center.Y + hy;
	return *this;
}

FRect & FRect::Grow(float aDX, float aDY)
{
	a.X -= aDX;
	a.Y -= aDY;
	b.X += aDX;
	b.Y += aDY;
	return *this;
}

FRect & FASTCALL FRect::MoveCenterTo(FPoint center)
{
	const float  w = Width();
	const float  h = Height();
	float  w2 = w/2;
	float  h2 = h/2;
	a.Set(center.X-w2, center.Y-h2);
	b.Set(center.X+(w-w2), center.Y+(h-h2));
	assert(Width() == w);
	assert(Height() == h);
	return *this;
}

int FRect::Contains(FPoint p) const
{
	return (p.X >= a.X && p.X <= b.X && p.Y >= a.Y && p.Y <= b.Y);
}
//
//
//
FShape::Circle::Circle() : R(0.0f)
{
}

FShape::EllipseArc::EllipseArc() : Start(0.0f), End(0.0f)
{
}

FShape::CircleArc::CircleArc() : Start(0.0f), End(0.0f)
{
}

uint FShape::Polygon::GetVertexCount() const
{
	return (getCount() / 2);
}

FShape::FShape(int kind) : Kind(kind), Flags(0), P_List(0)
{
	memzero(P, sizeof(P));
}

FShape::~FShape()
{
	delete P_List;
}

FShape::FShape(const FShape & rS) : P_List(0)
{
    Copy(rS);
}

FShape & FASTCALL FShape::operator = (const FShape & rS)
{
    Copy(rS);
    return *this;
}

void FShape::destroy()
{
    Kind = 0;
    Flags = 0;
    memzero(P, sizeof(P));
    ZDELETE(P_List);
}

int FASTCALL FShape::Copy(const FShape & rS)
{
    int    ok = 1;
    destroy();
    Kind = rS.Kind;
    Flags = rS.Flags;
    memcpy(P, rS.P, sizeof(P));
    if(rS.P_List) {
        P_List = new FloatArray(*rS.P_List);
    }
    return ok;
}

int FShape::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	THROW(pCtx->Serialize(dir, Kind, rBuf));
	THROW(pCtx->Serialize(dir, Flags, rBuf));
	for(uint i = 0; i < SIZEOFARRAY(P); i++) {
		THROW(pCtx->Serialize(dir, P[i], rBuf));
	}
	if(dir > 0) {
		THROW(pCtx->Serialize(dir, P_List, rBuf));
	}
	else if(dir < 0) {
		FloatArray temp_list;
		THROW(pCtx->Serialize(dir, &temp_list, rBuf));
		if(temp_list.getCount()) {
			THROW_S(SETIFZ(P_List, new FloatArray), SLERR_NOMEM);
			*P_List = temp_list;
		}
		else
			ZDELETE(P_List);
	}
	CATCHZOK
	return ok;
}

int FShape::GetKind() const
{
	return Kind;
}

int FShape::GetCount() const
{
	switch(Kind) {
		case SHAPE_UNDEF: return 0;
		case SHAPE_LINE: return 4;
		case SHAPE_RECT: return 4;
		case SHAPE_TRIANGLE: return 6;
		case SHAPE_TRAPEZ: return 0;
		case SHAPE_CIRCLE: return 3;
		case SHAPE_ELLIPSE: return 4;
		case SHAPE_CIRCLEARC: return 5;
		case SHAPE_ELLIPSEARC: return 6;
		case SHAPE_POLYGON: return P_List ? P_List->getCount() : -1;
		case SHAPE_POLYLINE: return P_List ? P_List->getCount() : -1;
		case SHAPE_ROUNDEDRECT: return 6;
	}
	return 0;
}

FShape & FASTCALL FShape::operator = (const Rect & rS)
{
	Kind = SHAPE_RECT;
	ZDELETE(P_List);
	P[0] = rS.a.X;
	P[1] = rS.a.Y;
	P[2] = rS.b.X;
	P[3] = rS.b.Y;
	return *this;
}

FShape & FASTCALL FShape::operator = (const RoundedRect & rS)
{
	Kind = SHAPE_ROUNDEDRECT;
	ZDELETE(P_List);
	P[0] = rS.a.X;
	P[1] = rS.a.Y;
	P[2] = rS.b.X;
	P[3] = rS.b.Y;
	P[4] = rS.R.X;
	P[5] = rS.R.Y;
	return *this;
}

FShape & FASTCALL FShape::operator = (const Line & rS)
{
	Kind = SHAPE_LINE;
	ZDELETE(P_List);
	P[0] = rS.A.X;
	P[1] = rS.A.Y;
	P[2] = rS.B.X;
	P[3] = rS.B.Y;
	return *this;
}

FShape & FASTCALL FShape::operator = (const Triangle & rS)
{
	Kind = SHAPE_TRIANGLE;
	ZDELETE(P_List);
	P[0] = rS.A.X;
	P[1] = rS.A.Y;
	P[2] = rS.B.X;
	P[3] = rS.B.Y;
	P[4] = rS.C.X;
	P[5] = rS.C.Y;
	return *this;
}

FShape & FASTCALL FShape::operator = (const Circle & rS)
{
	Kind = SHAPE_CIRCLE;
	ZDELETE(P_List);
	P[0] = rS.C.X;
	P[1] = rS.C.Y;
	P[2] = rS.R;
	return *this;
}

FShape & FASTCALL FShape::operator = (const Ellipse & rS)
{
	Kind = SHAPE_ELLIPSE;
	ZDELETE(P_List);
	P[0] = rS.C.X;
	P[1] = rS.C.Y;
	P[2] = rS.R.X;
	P[3] = rS.R.Y;
	return *this;
}

FShape & FASTCALL FShape::operator = (const CircleArc & rS)
{
	Kind = SHAPE_CIRCLEARC;
	ZDELETE(P_List);
	P[0] = rS.C.X;
	P[1] = rS.C.Y;
	P[2] = rS.R;
	P[3] = rS.Start;
	P[4] = rS.End;
	return *this;
}

FShape & FASTCALL FShape::operator = (const EllipseArc & rS)
{
	Kind = SHAPE_ELLIPSEARC;
	ZDELETE(P_List);
	P[0] = rS.C.X;
	P[1] = rS.C.Y;
	P[2] = rS.R.X;
	P[3] = rS.R.Y;
	P[4] = rS.Start;
	P[5] = rS.End;
	return *this;
}

FShape & FASTCALL FShape::operator = (const Polygon & rS)
{
	Kind = SHAPE_POLYGON;
	const uint c = rS.getCount();
	if(SETIFZ(P_List, new FloatArray))
		*P_List = rS;
	else
		SLS.SetError(SLERR_NOMEM);
	return *this;
}

FShape & FASTCALL FShape::operator = (const Polyline & rS)
{
	operator = ((Polygon &)rS);
	Kind = SHAPE_POLYLINE;
	return *this;
}

int FASTCALL FShape::Get(Rect & rS) const
{
	int    ok = Kind;
	if(oneof2(Kind, SHAPE_RECT, SHAPE_ROUNDEDRECT)) {
		rS.a.X = P[0];
		rS.a.Y = P[1];
		rS.b.X = P[2];
		rS.b.Y = P[3];
	}
	else
		ok = 0;
	return ok;
}

int FASTCALL FShape::Get(RoundedRect & rS) const
{
	int    ok = Kind;
	if(oneof2(Kind, SHAPE_RECT, SHAPE_ROUNDEDRECT)) {
		rS.a.X = P[0];
		rS.a.Y = P[1];
		rS.b.X = P[2];
		rS.b.Y = P[3];
		if(Kind == SHAPE_ROUNDEDRECT) {
			rS.R.X = P[4];
			rS.R.Y = P[5];
		}
		else {
			rS.R.Set(0.0f);
		}
	}
	else
		ok = 0;
	return ok;
}

int FASTCALL FShape::Get(Line & rS) const
{
	int    ok = Kind;
	if(Kind == SHAPE_LINE) {
		rS.A.X = P[0];
		rS.A.Y = P[1];
		rS.B.X = P[2];
		rS.B.Y = P[3];
	}
	else
		ok = 0;
	return ok;
}

int FASTCALL FShape::Get(Triangle & rS) const
{
	int    ok = Kind;
	if(Kind == SHAPE_TRIANGLE) {
		rS.A.X = P[0];
		rS.A.Y = P[1];
		rS.B.X = P[2];
		rS.B.Y = P[3];
		rS.C.X = P[4];
		rS.C.Y = P[5];
	}
	else if(Kind == SHAPE_POLYGON && P_List && P_List->getCount() == 6) {
		rS.A = P_List->getPoint(0);
		rS.B = P_List->getPoint(2);
		rS.C = P_List->getPoint(4);
	}
	else
		ok = 0;
	return ok;
}

int FASTCALL FShape::Get(Circle & rS) const
{
	int    ok = Kind;
	if(Kind == SHAPE_CIRCLE) {
		rS.C.X = P[0];
		rS.C.Y = P[1];
		rS.R = P[2];
	}
	else if(Kind == SHAPE_ELLIPSE) {
		Ellipse ellipse;
		if(Get(ellipse) && ellipse.R.X == ellipse.R.Y) {
			rS.C = ellipse.C;
			rS.R = ellipse.R.X;
		}
		else
			ok = 0;
	}
	else
		ok = 0;
	return ok;
}

int FASTCALL FShape::Get(Ellipse & rS) const
{
	int    ok = Kind;
	if(Kind == SHAPE_ELLIPSE) {
		rS.C.X = P[0];
		rS.C.Y = P[1];
		rS.R.X = P[2];
		rS.R.Y = P[3];
	}
	else if(Kind == SHAPE_CIRCLE) {
		rS.C.X = P[0];
		rS.C.Y = P[1];
		rS.R.X = P[2];
		rS.R.Y = P[2];
	}
	else
		ok = 0;
	return ok;
}

int FASTCALL FShape::Get(CircleArc & rS) const
{
	int    ok = Kind;
	if(Kind == SHAPE_CIRCLEARC) {
		rS.C.X   = P[0];
		rS.C.Y   = P[1];
		rS.R     = P[2];
		rS.Start = P[3];
		rS.End   = P[4];
	}
	else if(Get((Circle &)rS)) {
		rS.Start = 0.0f;
		rS.End = (float)SMathConst::Pi2;
	}
	else
		ok = 0;
	return ok;
}

int FASTCALL FShape::Get(EllipseArc & rS) const
{
	int    ok = Kind;
	if(Kind == SHAPE_ELLIPSEARC) {
		rS.C.X   = P[0];
		rS.C.Y   = P[1];
		rS.R.X   = P[2];
		rS.R.Y   = P[3];
		rS.Start = P[4];
		rS.End   = P[5];
	}
	else if(Get((Ellipse &)rS)) {
		rS.Start = 0.0f;
		rS.End = (float)SMathConst::Pi2;
	}
	else
		ok = 0;
	return ok;
}

int FASTCALL FShape::Get(Polygon & rS) const
{
	int    ok = Kind;
	rS.clear();
	if(Kind == SHAPE_POLYGON) {
		if(P_List)
			((FloatArray &)rS) = *P_List;
	}
	else {
		Rect rect;
		ok = Get(rect);
		if(ok) {
			rS.add(rect.a);
			rS.add(FPoint(rect.a.X, rect.b.Y));
			rS.add(rect.b);
			rS.add(FPoint(rect.b.X, rect.a.Y));
		}
		else {
			Triangle triangle;
			ok = Get(triangle);
			if(ok) {
				rS.add(triangle.A);
				rS.add(triangle.B);
				rS.add(triangle.C);
			}
		}
	}
	return ok;
}

int FASTCALL FShape::Get(Polyline & rS) const
{
	int    ok = Kind;
	rS.clear();
	if(Kind == SHAPE_POLYLINE) {
		if(P_List)
			((FloatArray &)rS) = *P_List;
	}
	else {
		Line line;
		ok = Get(line);
		if(ok) {
			rS.add(line.A);
			rS.add(line.B);
		}
	}
	return ok;
}
//
// TRect
//
TRect::TRect(int ax, int ay, int bx, int by)
{
	a.x = ax;
	a.y = ay;
	b.x = bx;
	b.y = by;
}

TRect::TRect(TPoint p1, TPoint p2) : a(p1), b(p2)
{
}

TRect::TRect(TPoint sz)
{
	a = 0;
	b = sz;
}

TRect::TRect()
{
	THISZERO();
}

TRect::TRect(const RECT & rS)
{
	a.Set(rS.left, rS.top);
	b.Set(rS.right, rS.bottom);
}

TRect & FASTCALL TRect::operator = (const RECT & rS)
{
	a.Set(rS.left, rS.top);
	b.Set(rS.right, rS.bottom);
	return *this;
}

TRect & FASTCALL TRect::operator = (TPoint p)
{
	a = 0;
	b = p;
	return *this;
}

TRect::operator RECT() const
{
	RECT r;
	r.left = a.x;
	r.top = a.y;
	r.right = b.x;
	r.bottom = b.y;
	return r;
}

float TRect::CenterX() const
{
	return (b.x + a.x) / 2.0f;
}

float TRect::CenterY() const
{
	return (b.y + a.y) / 2.0f;
}

int TRect::width() const
{
	return (b.x - a.x);
}

int TRect::height() const
{
	return (b.y - a.y);
}

TRect & TRect::set(int x1, int y1, int x2, int y2)
{
	a.Set(x1, y1);
	b.Set(x2, y2);
	return *this;
}

TRect & TRect::set(const FRect & rR)
{
	a.Set(ffloori(rR.a.X), ffloori(rR.a.Y));
	b.Set(fceili(rR.b.X), fceili(rR.b.Y));
	return *this;
}

TRect & FASTCALL TRect::setwidth(const TRect & rR)
{
	a.x = rR.a.x;
	b.x = rR.b.x;
	return *this;
}

TRect & FASTCALL TRect::setheight(const TRect & rR)
{
	a.y = rR.a.y;
	b.y = rR.b.y;
	return *this;
}

TRect & TRect::setwidthrel(int p, int w)
{
	a.x = p;
	b.x = p + w;
	return *this;
}

TRect & TRect::setheightrel(int p, int h)
{
	a.y = p;
	b.y = p + h;
	return *this;
}

TRect & FASTCALL TRect::setmarginx(const TRect & rR)
{
	a.x += rR.a.x;
	b.x -= rR.b.x;
	return *this;
}

TRect & FASTCALL TRect::setmarginy(const TRect & rR)
{
	a.y += rR.a.y;
	b.y -= rR.b.y;
	return *this;
}

TRect & FASTCALL TRect::setmarginx(int v)
{
	a.x += v;
	b.x -= v;
	return *this;
}

TRect & FASTCALL TRect::setmarginy(int v)
{
	a.y += v;
	b.y -= v;
	return *this;
}

TRect & TRect::move(int aDX, int aDY)
{
	a.x += aDX;
	a.y += aDY;
	b.x += aDX;
	b.y += aDY;
	return *this;
}

TRect & FASTCALL TRect::movecenterto(TPoint center)
{
	int    w = width();
	int    h = height();
	int    w2 = w/2;
	int    h2 = h/2;
	a.Set(center.x-w2, center.y-h2);
	b.Set(center.x+(w-w2), center.y+(h-h2));
	assert(width() == w);
	assert(height() == h);
	return *this;
}

TRect & FASTCALL TRect::move(TPoint delta)
{
	a.x += delta.x;
	a.y += delta.y;
	b.x += delta.x;
	b.y += delta.y;
	return *this;
}

TRect & TRect::grow(int aDX, int aDY)
{
	a.x -= aDX;
	a.y -= aDY;
	b.x += aDX;
	b.y += aDY;
	return *this;
}

/*
void TRect::intersect(const TRect& r)
{
	a.x = MAX(a.x, r.a.x);
	a.y = MAX(a.y, r.a.y);
	b.x = MIN(b.x, r.b.x);
	b.y = MIN(b.y, r.b.y);
}
*/

int TRect::Intersect(const TRect & rD, TRect * pResult) const
{
	int    ret = 0;
	IntRange w1, w2, wi;
	IntRange h1, h2, hi;
	int    rw = w1.Set(a.x, b.x).Intersect(w2.Set(rD.a.x, rD.b.x), &wi);
	int    rh = h1.Set(a.y, b.y).Intersect(h2.Set(rD.a.y, rD.b.y), &hi);
	if(rw > 0 && rh > 0) {
		ret = 1;
		if(rw == 2)
			ret |= 0x02;
		if(rh == 2)
			ret |= 0x04;
	}
	else if(rw < 0 || rh < 0)
		ret = -1;
	if(pResult) {
		pResult->a.Set(wi.low, hi.low);
		pResult->b.Set(wi.upp, hi.upp);
	}
	return ret;
}

TRect & FASTCALL TRect::Union(const TRect & r)
{
	a.x = MIN(a.x, r.a.x);
	a.y = MIN(a.y, r.a.y);
	b.x = MAX(b.x, r.b.x);
	b.y = MAX(b.y, r.b.y);
	return *this;
}

int FASTCALL TRect::contains(TPoint p) const
{
	if(p.x >= a.x && p.x <= b.x && p.y >= a.y && p.y <= b.y) {
		if(p.x == a.x)
			return (p.y == a.y) ? SOW_NORDWEST : ((p.y == b.y) ? SOW_SOUTHWEST : SOW_WEST);
		else if(p.x == b.x)
			return (p.y == a.y) ? SOW_NORDEAST : ((p.y == b.y) ? SOW_SOUTHEAST : SOW_EAST);
		else if(p.y == a.y)
			return SOW_NORD;
		else if(p.y == b.y)
			return SOW_SOUTH;
		else
			return 1000;
	}
	else
		return 0;
	//return (p.x >= a.x && p.x <= b.x && p.y >= a.y && p.y <= b.y);
}

int FASTCALL TRect::operator == (const TRect& r) const
{
	return (a == r.a && b == r.b);
}

int FASTCALL TRect::operator != (const TRect& r) const
{
	return (!(*this == r));
}

int TRect::IsEmpty() const
{
	return (a.x == 0 && b.x == 0 && a.y == 0 && b.y == 0);
}

int TRect::IsDegenerated() const
{
	return (a.x == b.x) ? ((a.y == b.y) ? SIDE_CENTER : SIDE_RIGHT) : ((a.y == b.y) ? SIDE_TOP : 0);
}

TRect & TRect::Normalize()
{
	ExchangeToOrder(&a.x, &b.x);
	ExchangeToOrder(&a.y, &b.y);
	return *this;
}
//
//
//
const FPoint ZEROFPOINT;

SLAPI FPoint::FPoint() : X(0.0f), Y(0.0f)
{
}

SLAPI FPoint::FPoint(float xy) : X(xy), Y(xy)
{
}

SLAPI FPoint::FPoint(float x, float y) : X(x), Y(y)
{
}

FPoint & FASTCALL FPoint::operator = (const TPoint & p)
{
	X = (float)p.x;
	Y = (float)p.y;
	return *this;
}

FPoint & FASTCALL FPoint::operator = (float f)
{
	X = Y = f;
	return *this;
}

int SLAPI FPoint::IsZero() const
{
	return (X != 0.0f && Y != 0.0f);
}

int SLAPI FPoint::IsPositive() const
{
	return (X > 0.0f && Y > 0.0f);
}

FPoint SLAPI FPoint::Set(float xy)
{
	X = Y = xy;
	return *this;
}

FPoint SLAPI FPoint::Set(float x, float y)
{
	X = x;
	Y = y;
	return *this;
}

FPoint SLAPI FPoint::SetZero()
{
	X = Y = 0;
	return *this;
}


int FASTCALL FPoint::Write(SBuffer & rBuf) const { return rBuf.Write(this, sizeof(*this)); }
int FASTCALL FPoint::Read(SBuffer & rBuf) { return rBuf.Read(this, sizeof(*this)); }

FPoint SLAPI FPoint::Scale(float factor)
{
	X *= factor;
	Y *= factor;
	return *this;
}

FPoint SLAPI FPoint::Neg()   const { return FPoint(-X, -Y); }
float  SLAPI FPoint::Ratio() const { return (Y / X); }
float  SLAPI FPoint::Add()   const { return X + Y; }
FPoint SLAPI FPoint::AddX(float x) const { return FPoint(X + x, Y); }
FPoint SLAPI FPoint::AddY(float y) const { return FPoint(X, Y + y); }
float  SLAPI FPoint::Sub()  const { return X - Y; }
float  SLAPI FPoint::Sq()   const { return (X * X + Y * Y); }
double SLAPI FPoint::Hypot() const { return _hypot(X, Y); }
float  SLAPI FPoint::Hypotf() const { return (float)_hypot(X, Y); }
FPoint SLAPI FPoint::Swap() const { return FPoint(Y, X); }
FPoint SLAPI FPoint::Combine(FPoint a, FPoint b) const { return FPoint(a.X*X + a.Y*Y, b.X*X + b.Y*Y); }

FPoint FASTCALL operator + (FPoint p,  float addendum) { return FPoint(p.X + addendum, p.Y + addendum); }
FPoint FASTCALL operator + (FPoint p1, FPoint p2)      { return FPoint(p1.X + p2.X, p1.Y + p2.Y); }
FPoint FASTCALL operator - (FPoint p1, FPoint p2)      { return FPoint(p1.X - p2.X, p1.Y - p2.Y); }
FPoint FASTCALL operator * (FPoint p, double mul)      { return FPoint((float)(p.X * mul), (float)(p.Y * mul)); }
FPoint FASTCALL operator * (FPoint p,  float mul)      { return FPoint(p.X * mul, p.Y * mul); }
FPoint FASTCALL operator * (FPoint p1, FPoint p2)      { return FPoint(p1.X * p2.X, p1.Y * p2.Y); }
FPoint FASTCALL operator / (FPoint p,  float divider)  { return FPoint(p.X / divider, p.Y / divider); }
FPoint FASTCALL operator / (FPoint p1, FPoint p2)      { return FPoint(p1.X / p2.X, p1.Y / p2.Y); }

int FASTCALL operator == (FPoint p1, FPoint p2) { return (p1.X == p2.X && p1.Y == p2.Y); }
int FASTCALL operator < (FPoint p1, FPoint p2)  { return (p1.X < p2.X && p1.Y < p2.Y) ? 1 : 0; }
int FASTCALL operator > (FPoint p1, FPoint p2)  { return (p1.X > p2.X && p1.Y > p2.Y) ? 1 : 0; }
FPoint FASTCALL fmin(FPoint p1, FPoint p2)      { return FPoint(MIN(p1.X, p2.X), MIN(p1.Y, p2.Y)); }
FPoint FASTCALL fmax(FPoint p1, FPoint p2)      { return FPoint(MAX(p1.X, p2.X), MAX(p1.Y, p2.Y)); }
float  FASTCALL atan2(FPoint p1, FPoint p2)     { return atan2f(p1.Y-p2.Y, p1.X-p2.X); }
FPoint FASTCALL trans01(FPoint p, FPoint radius, float angle) { return FPoint(p.X + radius.X * sinf(angle), p.Y - radius.Y * cosf(angle)); }
//
//
//
RPoint & RPoint::Set(double _xy)
{
	x = y = _xy;
	return *this;
}

RPoint & RPoint::Set(double _x, double _y)
{
	x = _x;
	y = _y;
	return *this;
}

RPoint & RPoint::SetPolar(double v, double rad)
{
	return Set(v * cos(rad), v * sin(rad));
}

void RPoint::GetInt(int * pX, int * pY) const
{
	ASSIGN_PTR(pX, (int)x);
	ASSIGN_PTR(pY, (int)y);
}

void RPoint::GetUInt(uint * pX, uint * pY) const
{
	ASSIGN_PTR(pX, (uint)x);
	ASSIGN_PTR(pY, (uint)y);
}

int FASTCALL RPoint::FromStr(const char * pStr)
{
	int    ok = 0;
	SString temp_buf;
	SStrScan scan(pStr);
	scan.Skip();
	if(scan.GetNumber(temp_buf)) {
		x = temp_buf.ToReal();
		scan.Skip();
		if(scan.IncrChr(',') || scan.IncrChr(';'))
			scan.Skip();
		if(scan.GetNumber(temp_buf)) {
			y = temp_buf.ToReal();
			ok = 1;
		}
		else {
			y = x;
			ok = 1;
		}
	}
	return ok;
}

static RPoint & Push_RPoint(double x, double y)
{
	RPoint p;
	p.x = x;
	p.y = y;
	return PushRecycledObject <RPoint, 64> (p);
}

static RPoint3 & Push_RPoint3(double x, double y, double z)
{
	RPoint3 p;
	p.x = x;
	p.y = y;
	p.z = z;
	return PushRecycledObject <RPoint3, 64> (p);
}

RPoint & FASTCALL operator - (const RPoint & one, const RPoint & two)
	{ return Push_RPoint(one.x - two.x, one.y - two.y); }
RPoint & FASTCALL operator + (const RPoint & one, const RPoint & two)
	{ return Push_RPoint(one.x + two.x, one.y + two.y); }
RPoint & FASTCALL operator + (const RPoint & p, double a)
	{ return Push_RPoint(p.x + a, p.y + a); }
RPoint & FASTCALL operator * (const RPoint & p, double m)
	{ return Push_RPoint(p.x * m, p.y * m); }
RPoint & FASTCALL operator / (const RPoint & p, double d)
	{ return (d != 0.0) ? Push_RPoint(p.x / d, p.y / d) : Push_RPoint(0.0, 0.0); }
//
//
//
RPoint3 & RPoint3::Set(double _x, double _y, double _z)
{
	x = _x;
	y = _y;
	z = _z;
	return *this;
}

RPoint3 & FASTCALL operator - (const RPoint3 & one, const RPoint3 & two)
{
	return Push_RPoint3(one.x - two.x, one.y - two.y, one.z - two.z);
}

RPoint3 & FASTCALL operator + (const RPoint3 & one, const RPoint3 & two)
{
	return Push_RPoint3(one.x + two.x, one.y + two.y, one.z + two.z);
}

RPoint3 & FASTCALL operator * (const RPoint3 & p, double m)
{
	return Push_RPoint3(p.x * m, p.y * m, p.z * m);
}

RPoint3 & FASTCALL operator / (const RPoint3 & p, double d)
{
	return (d != 0.0) ? Push_RPoint3(p.x / d, p.y / d, p.z / d) : Push_RPoint3(0.0, 0.0, 0.0);
}
//
//
//
int SColorRGB::IsZero() const
{
	return (!R && !G && !B);
}

SColorRGB SColorRGB::Set(uint r, uint g, uint b)
{
	R = r;
	G = g;
	B = b;
	return *this;
}

SColorRGB FASTCALL SColorRGB::Set(uint v)
{
	R = v;
	G = v;
	B = v;
	return *this;
}
//
//
//
struct ColorName {
	uint32 C; // 0x00rrggbb
	const char * N;
};

static const ColorName ColorNameList[] = {
	{ 0x00f0f8ff, "Aliceblue" },
	{ 0x00faebd7, "Antiquewhite" },
	{ 0x0000ffff, "Aqua" },
	{ 0x007fffd4, "Aquamarine" },
	{ 0x00f0ffff, "Azure" },
	{ 0x00f5f5dc, "Beige" },
	{ 0x00ffe4c4, "Bisque" },
	{ 0x00000000, "Black" },
	{ 0x00ffebcd, "Blanchedalmond" },
	{ 0x000000ff, "Blue" },
	{ 0x008a2be2, "Blueviolet" },
	{ 0x00a52a2a, "Brown" },
	{ 0x00deb887, "Burlywood" },
	{ 0x005f9ea0, "Cadetblue" },
	{ 0x007fff00, "Chartreuse" },
	{ 0x00d2691e, "Chocolate" },
	{ 0x00ff7f50, "Coral" },
	{ 0x006495ed, "Cornflowerblue" },
	{ 0x00fff8dc, "Cornsilk" },
	{ 0x00dc143c, "Crimson" },
	{ 0x0000ffff, "Cyan" },
	{ 0x0000008b, "Darkblue" },
	{ 0x00008b8b, "Darkcyan" },
	{ 0x00b8860b, "Darkgoldenrod" },
	{ 0x00555555, "Darkgrey" },
	{ 0x00006400, "Darkgreen" },
	{ 0x00bdb76b, "Darkkhaki" },
	{ 0x008b008b, "Darkmagenta" },
	{ 0x00556b2f, "Darkolivegreen" },
	{ 0x00ff8c00, "Darkorange" },
	{ 0x009932cc, "Darkorchid" },
	{ 0x008b0000, "Darkred" },
	{ 0x00e9967a, "Darksalmon" },
	{ 0x008fbc8f, "Darkseagreen" },
	{ 0x00483d8b, "Darkslateblue" },
	{ 0x002f4f4f, "Darkslategrey" },
	{ 0x0000ced1, "Darkturquoise" },
	{ 0x009400d3, "Darkviolet" },
	{ 0x00ff1493, "Deeppink" },
	{ 0x0000bfff, "Deepskyblue" },
	{ 0x00696969, "Dimgrey" },
	{ 0x001e90ff, "Dodgerblue" },
	{ 0x00b22222, "Firebrick" },
	{ 0x00fffaf0, "Floralwhite" },
	{ 0x00228b22, "Forestgreen" },
	{ 0x00ff00ff, "Fuchsia" },
	{ 0x00dcdcdc, "Gainsboro" },
	{ 0x00ffd700, "Gold" },
	{ 0x00daa520, "Goldenrod" },
	{ 0x00808080, "Grey" },
	{ 0x00008000, "Green" },
	{ 0x00adff2f, "Greenyellow" },
	{ 0x00f0fff0, "Honeydew" },
	{ 0x00ff69b4, "Hotpink" },
	{ 0x00cd5c5c, "Indianred" },
	{ 0x004b0082, "Indigo" },
	{ 0x00fffff0, "Ivory" },
	{ 0x00f0e68c, "Khaki" },
	{ 0x00e6e6fa, "Lavender" },
	{ 0x00fff0f5, "Lavenderblush" },
	{ 0x00fffacd, "Lemonchiffon" },
	{ 0x00add8e6, "Lightblue" },
	{ 0x00f08080, "Lightcoral" },
	{ 0x00e0ffff, "Lightcyan" },
	{ 0x00fafad2, "Lightgoldenrodyellow" },
	{ 0x0090ee90, "Lightgreen" },
	{ 0x00d3d3d3, "Lightgrey" },
	{ 0x00ffb6c1, "Lightpink" },
	{ 0x00ffa07a, "Lightsalmon" },
	{ 0x0020b2aa, "Lightseagreen" },
	{ 0x0087cefa, "Lightskyblue" },
	{ 0x00778899, "Lightslategrey" },
	{ 0x00b0c4de, "Lightsteelblue" },
	{ 0x00ffffe0, "Lightyellow" },
	{ 0x0000ff00, "Lime" },
	{ 0x0032cd32, "Limegreen" },
	{ 0x00faf0e6, "Linen" },
	{ 0x00ff00ff, "Magenta" },
	{ 0x00800000, "Maroon" },
	{ 0x0066cdaa, "Mediumaquamarine" },
	{ 0x000000cd, "Mediumblue" },
	{ 0x00ba55d3, "Mediumorchid" },
	{ 0x009370db, "Mediumpurple" },
	{ 0x003cb371, "Mediumseagreen" },
	{ 0x007b68ee, "Mediumslateblue" },
	{ 0x0000fa9a, "Mediumspringgreen" },
	{ 0x0048d1cc, "Mediumturquoise" },
	{ 0x00c71585, "Mediumvioletred" },
	{ 0x00191970, "Midnightblue" },
	{ 0x00f5fffa, "Mintcream" },
	{ 0x00ffe4e1, "Mistyrose" },
	{ 0x00ffdead, "Navajowhite" },
	{ 0x00000080, "Navy" },
	{ 0x00fdf5e6, "Oldlace" },
	{ 0x00808000, "Olive" },
	{ 0x006b8e23, "Olivedrab" },
	{ 0x00ffa500, "Orange" },
	{ 0x00ff4500, "Orangered" },
	{ 0x00da70d6, "Orchid" },
	{ 0x00eee8aa, "Palegoldenrod" },
	{ 0x0098fb98, "Palegreen" },
	{ 0x00afeeee, "Paleturquoise" },
	{ 0x00db7093, "Palevioletred" },
	{ 0x00ffefd5, "Papayawhip" },
	{ 0x00ffdab9, "Peachpuff" },
	{ 0x00cd853f, "Peru" },
	{ 0x00ffc0cb, "Pink" },
	{ 0x00dda0dd, "Plum" },
	{ 0x00b0e0e6, "Powderblue" },
	{ 0x00800080, "Purple" },
	{ 0x00ff0000, "Red" },
	{ 0x00bc8f8f, "Rosybrown" },
	{ 0x004169e1, "Royalblue" },
	{ 0x008b4513, "Saddlebrown" },
	{ 0x00fa8072, "Salmon" },
	{ 0x00f4a460, "Sandybrown" },
	{ 0x002e8b57, "Seagreen" },
	{ 0x00fff5ee, "Seashell" },
	{ 0x00a0522d, "Sienna" },
	{ 0x00c0c0c0, "Silver" },
	{ 0x0087ceeb, "Skyblue" },
	{ 0x006a5acd, "Slateblue" },
	{ 0x00708090, "Slategrey" },
	{ 0x00fffafa, "Snow" },
	{ 0x0000ff7f, "Springgreen" },
	{ 0x004682b4, "Steelblue" },
	{ 0x00306080, "Steelblue4" },
	{ 0x00d2b48c, "Tan" },
	{ 0x00008080, "Teal" },
	{ 0x00d8bfd8, "Thistle" },
	{ 0x00ff6347, "Tomato" },
	{ 0x0040e0d0, "Turquoise" },
	{ 0x00ee82ee, "Violet" },
	{ 0x00f5deb3, "Wheat" },
	{ 0x00ffffff, "White" },
	{ 0x00f5f5f5, "Whitesmoke" },
	{ 0x00ffff00, "Yellow" },
	{ 0x009acd32, "Yellowgreen" }
};

static char FASTCALL hexdigit(uint d)
{
	return (d >= 0 && d <= 9) ? (d + '0') : ((d >= 10 && d <= 15) ? (d - 10 + 'A') : '0');
}

int FASTCALL SColorBase::FromStr(const char * pStr)
{
	int    ok = 0;
	SStrScan scan(pStr);
	*(uint32 *)this = 0;
	Alpha = 0xff;
	scan.Skip();
	size_t len = sstrlen(scan);
	if(scan[0] == '#') {
		len--;
		scan.Incr();
		if(len >= 6) {
			R = (hex(scan[0]) << 4) | hex(scan[1]);
			G = (hex(scan[2]) << 4) | hex(scan[3]);
			B = (hex(scan[4]) << 4) | hex(scan[5]);
			scan.Incr(6);
			ok = fmtHEX;
		}
		else if(len >= 3) {
			uint8 _h = hex(scan[0]);
			R = (_h << 4) | _h;
			_h = hex(scan[1]);
			G = (_h << 4) | _h;
			_h = hex(scan[2]);
			B = (_h << 4) | _h;
			scan.Incr(3);
			ok = fmtHEX;
		}
	}
	else if(strncmp(pStr, "rgb", 3) == 0) {
		double c = 0.0;
		SString nmb_buf;
		scan.Incr(3);
		scan.Skip();
		scan.IncrChr('(');
		if(scan.GetNumber(nmb_buf)) {
			c = nmb_buf.ToReal();
			if(scan.Skip().IncrChr('%'))
				c *= 2.55;
			R = (uint8)c;
			//
			scan.Skip().IncrChr(',');
			if(scan.Skip().GetNumber(nmb_buf)) {
				c = nmb_buf.ToReal();
				if(scan.Skip().IncrChr('%'))
					c *= 2.55;
				G = (uint8)c;
				//
				scan.Skip().IncrChr(',');
				if(scan.Skip().GetNumber(nmb_buf)) {
					c = nmb_buf.ToReal();
					if(scan.Skip().IncrChr('%'))
						c *= 2.55;
					B = (uint8)c;
					//
					scan.Skip().IncrChr(')');
					ok = fmtRGB;
				}
			}
		}
	}
	else {
		for(uint i = 0; !ok && i < SIZEOFARRAY(ColorNameList); i++) {
			if(stricmp(pStr, ColorNameList[i].N) == 0) {
				uint32 c = ColorNameList[i].C;
				R = (uint8)((c & 0x00ff0000) >> 16);
				G = (uint8)((c & 0x0000ff00) >> 8);
				B = (uint8)(c & 0xff);
				ok = fmtName;
			}
		}
	}
	return ok;
}

SString & SColorBase::ToStr(SString & rBuf, int format) const
{
	rBuf.Z();
	if(format & fmtName) {
		const uint32 c = (R << 16) | (G << 8) | B;
		for(uint i = 0; i < SIZEOFARRAY(ColorNameList); i++) {
			if(ColorNameList[i].C == c) {
				rBuf = ColorNameList[i].N;
			}
		}
	}
	if(rBuf.Empty()) {
		long f = (format & ~fmtName);
		if(f == fmtRGB) {
			rBuf.Cat("rgb").CatChar('(').Cat((uint)R).Comma().Cat((uint)G).Comma().Cat((uint)B).CatChar(')');
		}
		else if(f == fmtRgbHexWithoutPrefix) {
			rBuf.CatChar(hexdigit(R>>4)).CatChar(hexdigit(R&0x0f));
			rBuf.CatChar(hexdigit(G>>4)).CatChar(hexdigit(G&0x0f));
			rBuf.CatChar(hexdigit(B>>4)).CatChar(hexdigit(B&0x0f));
		}
		else { // fmtHEX as default
			rBuf.CatChar('#');
			rBuf.CatChar(hexdigit(R>>4)).CatChar(hexdigit(R&0x0f));
			rBuf.CatChar(hexdigit(G>>4)).CatChar(hexdigit(G&0x0f));
			rBuf.CatChar(hexdigit(B>>4)).CatChar(hexdigit(B&0x0f));
		}
	}
	return rBuf;
}

SColorBase::operator COLORREF() const
{
	return RGB(R, G, B);
}

SColorBase::operator RGBQUAD() const
{
	uint32 q = (*(uint32 *)this & 0x00ffffff);
	return *(RGBQUAD *)&q;
}

float SColorBase::RedF() const { return (R / 255.0f); }
float SColorBase::GreenF() const { return (G / 255.0f); }
float SColorBase::BlueF() const { return (B / 255.0f); }
float SColorBase::AlphaF() const { return (Alpha / 255.0f); }
float SColorBase::OpacityF() const { return (1.0f - (Alpha / 255.0f)); }

SColorBase SColorBase::SetEmpty()
{
	R = 0;
	G = 0;
	B = 0;
	Alpha = 0;
	return *this;
}

SColorBase SColorBase::Set(uint r, uint g, uint b)
{
	R = (uint8)r;
	G = (uint8)g;
	B = (uint8)b;
	Alpha = 0xff;
	return *this;
}

SColorBase FASTCALL SColorBase::Set(uint v)
{
	R = v;
	G = v;
	B = v;
	Alpha = 0xff;
	return *this;
}

SColorBase SColorBase::PremultiplyAlpha()
{
	uint32 c = *(uint32 *)this;
	PREMULTIPLY_ALPHA_ARGB32(c);
	*(uint32 *)this = c;
	return *this;
}

SColor::SColor()
{
	R = 0;
	G = 0;
	B = 0;
	Alpha = 0xff;
}

SColor::SColor(const SColorBase & rS)
{
    *(SColorBase *)this = rS;
}

SColor::SColor(float whitePart)
{
	R = G = B = (uint8)(255.f * whitePart);
	Alpha = 0xff;
}

SColor::SColor(uint r, uint g, uint b, uint alpha)
{
	R = (uint8)r;
	G = (uint8)g;
	B = (uint8)b;
	Alpha = (uint8)alpha;
}

SColor::SColor(uint r, uint g, uint b)
{
	R = (uint8)r;
	G = (uint8)g;
	B = (uint8)b;
	Alpha = 0xff;
}

SColor::SColor(SColourCollection c)
{
	R = (uint8)((c & 0x00ff0000) >> 16);
	G = (uint8)((c & 0x0000ff00) >> 8);
	B = (uint8)(c & 0xff);
	Alpha = 0xff;
}

SColor::SColor(COLORREF c)
{
	R = (uint8)GetRValue(c);
	G = (uint8)GetGValue(c);
	B = (uint8)GetBValue(c);
	Alpha = 0xff;
}

SColor & FASTCALL SColor::operator = (const SColorBase & rS)
{
	*(SColorBase *)this = rS;
	return *this;
}

SColor FASTCALL SColor::Lighten(float factor) const
{
	SColor col = *this;
	if(factor > 0.0f && factor <= 1.0f){
		uint8  x = col.R;
		col.R = (uint8)((factor*(255-x)) + x);
		x = col.G;
		col.G = (uint8)((factor*(255-x)) + x);
		x = col.B;
		col.B = (uint8)((factor*(255-x)) + x);
	}
	return col;
}

SColor FASTCALL SColor::Darken(float factor) const
{
	SColor col = *this;
	if(factor > 0.0f && factor <= 1.0f) {
		uint8  x = col.R;
		col.R = (uint8)(x-(factor*x));
		x = col.G;
		col.G = (uint8)(x-(factor*x));
		x = col.B;
		col.B = (uint8)(x-(factor*x));
	}
	return col;
}
//
//
//
COLORREF FASTCALL GetColorRef(SColourCollection c)
{
	return RGB((c & 0x00ff0000) >> 16, (c & 0x0000ff00) >> 8, c & 0xff);
}

COLORREF FASTCALL GetGrayColorRef(float whitePart)
{
	SColor c(whitePart);
	return (COLORREF)c;
}

COLORREF FASTCALL LightenColor(COLORREF col, float factor)
{
	if(factor > 0.0f && factor <= 1.0f){
		uint8  x = GetRValue(col);
		uint8  r = (uint8)((factor*(255-x)) + x);
		x = GetGValue(col);
		uint8  g = (uint8)((factor*(255-x)) + x);
		x = GetBValue(col);
		uint8  b = (uint8)((factor*(255-x)) + x);
		return RGB(r, g, b);
	}
	else
		return col;
}

COLORREF FASTCALL DarkenColor(COLORREF col, float factor)
{
	if(factor > 0.0f && factor <= 1.0f) {
		uint8  x = GetRValue(col);
		uint8  r = (uint8)(x-(factor*x));
		x = GetGValue(col);
		uint8  g = (uint8)(x-(factor*x));
		x = GetBValue(col);
		uint8  b = (uint8)(x-(factor*x));
		return RGB(r, g, b);
	}
	else
		return col;
}
//
//
//
void UiCoord::Set(int v, int f)
{
	Val = (int16)v;
	Flags = (int16)f;
}

void UiCoord::Reset()
{
	Val = 0;
	Flags = 0;
}

int UiCoord::IsEmpty() const
{
	return (Val == 0 && Flags == 0);
}

UiRelPoint & FASTCALL UiRelPoint::Set(const TPoint & rP)
{
	X.Set(rP.x, 0);
	Y.Set(rP.y, 0);
	return *this;
}

UiRelRect & FASTCALL UiRelRect::Set(const TRect & rR)
{
	L.Set(rR.a);
	R.Set(rR.b);
	return *this;
}

void UiRelRect::Reset()
{
	L.X.Reset();
	L.Y.Reset();
	R.X.Reset();
	R.Y.Reset();
}

int UiRelRect::IsEmpty() const
{
	return (L.X.IsEmpty() && L.Y.IsEmpty() && R.X.IsEmpty() && R.Y.IsEmpty());
}
//
//
//
SRegion::SRegion()
{
	H = (void *)0;
}

SRegion::SRegion(const TRect & rR) 
{
	H = (void *)::CreateRectRgn(rR.a.x, rR.a.y, rR.b.x, rR.b.y);
}

SRegion::~SRegion()
{
	Destroy();
}

void SRegion::Destroy()
{
	if(!!H) {
		::DeleteObject((HRGN)(void *)H);
		H = (void *)0;
	}
}

int SRegion::operator !() const
{
	return (!H);
}

SRegion & FASTCALL SRegion::operator = (const SRegion & rS)
{
	Destroy();
	if(!!rS) {
		HRGN h_rgn = CreateRectRgn(0, 0, 1, 1);
		if(CombineRgn(h_rgn, h_rgn, (HRGN)(void *)rS.H, RGN_COPY)) {
			H = (void *)h_rgn;
		}
	}
	return *this;
}

int FASTCALL SRegion::operator == (const SRegion & rS) const
{
	int    c = 0;
	if(!!H && !!rS.H) {
		c = BIN(EqualRgn((HRGN)(void *)H, (HRGN)(void *)rS.H));
	}
	else if(!H && !rS.H)
		c = 1;
	return c;
}

TRect SRegion::GetBounds() const
{
	TRect r;
	if(H) {
		RECT r_;
		if(GetRgnBox((HRGN)(void *)H, &r_))
			r = r_;
	}
	return r;
}

int SRegion::Add(const TRect & rR, int combine)
{
	SRegion src(rR);
	return Add(src, combine);
}

int SRegion::Add(const SRegion & rS, int combine)
{
	int    ok = SIMPLEREGION;
	if(!!rS) {
		if(!H) {
			HRGN h_rgn = CreateRectRgn(0, 0, 1, 1);
			ok = ::CombineRgn(h_rgn, (HRGN)(void *)rS.H, 0, RGN_COPY);
			if(ok)
				H = (void *)h_rgn;
		}
		else if(oneof6(combine, SCOMBINE_NONE, SCOMBINE_AND, SCOMBINE_OR, SCOMBINE_XOR, SCOMBINE_COPY, SCOMBINE_DIFF)) {
			HRGN h_prev = (HRGN)(void *)H;
			int  _c;
			switch(combine) {
				case SCOMBINE_AND: _c = RGN_AND; break;
				case SCOMBINE_OR:  _c = RGN_OR; break;
				case SCOMBINE_XOR: _c = RGN_XOR; break;
				case SCOMBINE_DIFF: _c = RGN_DIFF; break;
				case SCOMBINE_COPY: _c = RGN_COPY; break;
			}
			ok = ::CombineRgn(h_prev, (HRGN)(void *)rS.H, h_prev, _c);
			if(!ok) {
				SLS.SetOsError(0);
			}
		}
		else
			ok = 0;
	}
	return ok;
}

int SRegion::AddFrame(const TRect & rR, uint halfThick, int combine)
{
	int    ok = 1;
	TRect  rc;
	TRect  nrc = rR;
	nrc.Normalize();
	int    r = nrc.IsDegenerated();
	if(r != 0) {
		(rc = nrc).grow(halfThick, halfThick);
		THROW(Add(rc, combine));
	}
	else {
		rc = nrc;
		rc.b.y = rc.a.y;
		THROW(Add(rc.grow(halfThick, halfThick), combine));
		//
		rc = nrc;
		rc.a.y = rc.b.y;
		THROW(Add(rc.grow(halfThick, halfThick), combine));
		//
		rc = nrc;
		rc.b.x = rc.a.x;
		THROW(Add(rc.grow(halfThick, -(int)halfThick), combine));
		//
		rc = nrc;
		rc.a.x = rc.b.x;
		THROW(Add(rc.grow(halfThick, -(int)halfThick), combine));
	}
	CATCHZOK
	return ok;
}
