// SDRAW.CPP
// Copyright (c) A.Sobolev 2010, 2011, 2012, 2013, 2015, 2016, 2017
//
#include <slib.h>
#include <tv.h>
#pragma hdrstop
#include <setjmp.h> // jpeg && png
//
#define CAIRO_WIN32_STATIC_BUILD 1
#include <cairo\1_14_2\cairo.h>
#include <cairo\1_14_2\cairo-win32.h>

#define DEFAULT_UCTX_FONTSIZE 10
//
//
//
SDrawContext::UC::UC()
{
	Dpi.Set(0.0f);
	FontSize = 0.0f;
}

int SDrawContext::UC::Describe(int unitId, int dir, int * pCls, double * pToBase, SString * pName) const
{
	int    cls = 0;
	double to_base = 0.0;
	const  char * p_name = 0;
	if(unitId == UNIT_GR_PIXEL) {
		to_base = (dir == DIREC_VERT) ? fdivnz(2.54e-2, Dpi.Y) : fdivnz(2.54e-2, Dpi.X);
		cls = SUnit::clsLength;
		p_name = "pixel";
	}
	else if(unitId == UNIT_GR_PT) {
		to_base = 2.54e-2 / 72.0;
		cls = SUnit::clsLength;
		p_name = "pt";
	}
	else if(unitId == UNIT_GR_EM) {
		to_base = (dir == DIREC_VERT) ? fdivnz(FontSize * 2.54e-2, Dpi.Y) : fdivnz(FontSize * 2.54e-2, Dpi.X);
		cls = SUnit::clsLength;
		p_name = "em";
	}
	else if(unitId == UNIT_GR_EX) {
		to_base = (dir == DIREC_VERT) ? fdivnz((FontSize / 2.0) * 2.54e-2, Dpi.Y) : fdivnz((FontSize / 2.0)  * 2.54e-2, Dpi.X);
		cls = SUnit::clsLength;
		p_name = "ex";
	}
	/*
	else if(unitId == UNIT_PERCENT) {
	}
	*/
	if(cls) {
		ASSIGN_PTR(pCls, cls);
		ASSIGN_PTR(pToBase, to_base);
		ASSIGN_PTR(pName, p_name);
		return 1;
	}
	else
		return SUnit::Context::Describe(unitId, dir, pCls, pToBase, pName);
}

// static
uint SDrawContext::CalcScreenFontSizePt(uint pt)
{
	uint   s = 0;
	SDrawContext dctx((cairo_t *)0);
	SDrawContext::UC * p_uc = dctx.GetUnitContext();
	if(p_uc) {
		USize pt_size;
		USize px_size;
		SUnit::Convert(pt_size.Set(pt, UNIT_GR_PT, DIREC_VERT), px_size.Set(1, UNIT_GR_PIXEL, DIREC_VERT), p_uc);
		s = (uint)ceil(px_size);
		delete p_uc;
	}
	return s;
}

SDrawContext::SDrawContext(cairo_t * pCr)
{
	S = dsysCairo;
	P = pCr;
}

SDrawContext::SDrawContext(HDC hDc)
{
	S = dsysWinGdi;
	P = (void *)hDc;
}

SDrawContext::operator cairo_t * () const
{
	return (S == dsysCairo) ? (cairo_t *)P : 0;
}

SDrawContext::UC * SDrawContext::GetUnitContext() const
{
	SDrawContext::UC * p_uc = 0;
	if(oneof2(S, dsysWinGdi, dsysCairo)) {
		p_uc = new SDrawContext::UC;
		if(p_uc) {
			HDC    h_dc = (S == dsysWinGdi && P) ? (HDC)P : SLS.GetTLA().GetFontDC();
			float pt_per_inch_h = (float)GetDeviceCaps(h_dc, LOGPIXELSX);
			float pt_per_inch_v = (float)GetDeviceCaps(h_dc, LOGPIXELSY);
			p_uc->Dpi.Set(pt_per_inch_h, pt_per_inch_v);
			p_uc->FontSize = DEFAULT_UCTX_FONTSIZE;
		}
	}
	return p_uc;
}
//
//
//
SViewPort::SViewPort(uint flags) : FRect()
{
	ParX = parMid;
	ParY = parMid;
	Flags = (fEmpty | flags);
}

LMatrix2D & SViewPort::GetMatrix(const FRect & rBounds, LMatrix2D & rMtx) const
{
	const double w0 = Width();
	const double h0 = Height();
	double w_c = (Flags & fDontScale) ? w0 : rBounds.Width();
	double h_c = (Flags & fDontScale) ? h0 : rBounds.Height();
	const double r0 = h0 / w0;
	const double r = h_c / w_c;
	FPoint diff = rBounds.a - a;
	LMatrix2D transl_mtx;
	if((ParX == parNone && ParY == parNone) || r == r0) {
		if(Flags & fDontEnlarge) {
			if(ParX == parMax)
				diff = diff.AddX((float)(w_c - w0));
			else if(oneof2(ParX, parMid, parNone))
				diff = diff.AddX((float)((w_c - w0)/2.0));
			if(ParY == parMax)
				diff = diff.AddY((float)(h_c - h0));
			else if(oneof2(ParY, parMid, parNone))
				diff = diff.AddY((float)((h_c - h0)/2.0));
			h_c = h0;
			w_c = w0;
		}
		rMtx.InitScale(w_c/w0, h_c/h0);
	}
	else {
		if((r > r0 && Flags & fSlice) || (r < r0 && !(Flags & fSlice))) {
			if(Flags & fDontEnlarge && h_c > h0) {
				if(ParY == parMax)
					diff = diff.AddY((float)(h_c - h0));
				else if(oneof2(ParY, parMid, parNone))
					diff = diff.AddY((float)((h_c - h0)/2.0));
				h_c = h0;
			}
			rMtx.InitScale(h_c/h0, h_c/h0);
			if(ParX == parMax)
				diff = diff.AddX((float)(w_c - h_c / r0));
			else if(oneof2(ParX, parMid, parNone))
				diff = diff.AddX((float)((w_c - h_c / r0)/2.0));
		}
		else {
			if(Flags & fDontEnlarge && w_c > w0) {
				if(ParX == parMax)
					diff = diff.AddX((float)(w_c - w0));
				else if(oneof2(ParX, parMid, parNone))
					diff = diff.AddX((float)((w_c - w0)/2.0));
				w_c = w0;
			}
			rMtx.InitScale(w_c/w0, w_c/w0);
			if(ParY == parMax)
				diff = diff.AddY((float)(h_c - w_c * r0));
			else if(oneof2(ParY, parMid, parNone))
				diff = diff.AddY((float)((h_c - w_c * r0)/2.0));
		}
	}
	transl_mtx.InitTranslate(diff);
	return (rMtx = rMtx * transl_mtx);
}

int SViewPort::FromStr(const char * pStr, int fmt)
{
	int    ok = 1;
	SStrScan scan(pStr);
	SString temp_buf;
	if(scan.GetNumber(temp_buf)) {
		Flags &= ~fEmpty;
		a.X = (float)temp_buf.ToReal();
		scan.SkipOptionalDiv(',');
		THROW(scan.GetNumber(temp_buf));
		a.Y = (float)temp_buf.ToReal();
		scan.SkipOptionalDiv(',');
		THROW(scan.GetNumber(temp_buf));
		b.X = a.X + (float)temp_buf.ToReal();
		scan.SkipOptionalDiv(',');
		THROW(scan.GetNumber(temp_buf));
		b.Y = a.Y + (float)temp_buf.ToReal();
	}
	else {
		while(scan.Skip().GetIdent(temp_buf)) {
			if(temp_buf == "none") { ParX = ParY = parNone; }
			else if(temp_buf == "xMinYMin") { ParX = parMin; ParY = parMin; }
			else if(temp_buf == "xMidYMin") { ParX = parMid; ParY = parMin; }
			else if(temp_buf == "xMaxYMin") { ParX = parMax; ParY = parMin; }
			else if(temp_buf == "xMinYMid") { ParX = parMin; ParY = parMid; }
			else if(temp_buf == "xMidYMid") { ParX = parMid; ParY = parMid; }
			else if(temp_buf == "xMaxYMid") { ParX = parMax; ParY = parMid; }
			else if(temp_buf == "xMinYMax") { ParX = parMin; ParY = parMax; }
			else if(temp_buf == "xMidYMax") { ParX = parMid; ParY = parMax; }
			else if(temp_buf == "xMaxYMax") { ParX = parMax; ParY = parMax; }
			else if(temp_buf == "meet")     { Flags &= ~fSlice; }
			else if(temp_buf == "slice")    { Flags |= fSlice;  }
		}
	}
	CATCHZOK
	return ok;
}
//
//
//
template <class F> SDrawFigure * DupDrawFigure(const SDrawFigure * pThis)
{
	F * p_dup = new F;
	if(p_dup)
		p_dup->Copy((F &)*pThis);
	else
		SLS.SetError(SLERR_NOMEM);
	return p_dup;
}

// static
int SDrawFigure::CheckKind(int kind)
{
	if(oneof5(kind, kShape, kPath, kGroup, kImage, kText))
		return 1;
	else {
		SString msg_buf;
		return SLS.SetError(SLERR_INVSDRAWFIGKIND, msg_buf.Cat(kind));
	}
}

//static
SDrawFigure * SDrawFigure::CreateFromFile(const char * pFileName, const char * pSid)
{
	SDrawFigure * p_fig = 0;
	SFileFormat fmt;
	THROW(fileExists(pFileName));
	const int fir = fmt.Identify(pFileName);
	THROW(fir);
	if(fmt == SFileFormat::Svg) {
		SDrawContext dctx((HDC)0);
		SDraw * p_draw_fig = new SDraw(pSid, 0);
		THROW_S(p_draw_fig, SLERR_NOMEM);
		p_draw_fig->SetupUnitContext(dctx);
		THROW(p_draw_fig->ParseSvgFile(pFileName));
		p_fig = p_draw_fig;
	}
	else {
		THROW(SImageBuffer::IsSupportedFormat(fmt));
		{
			SDrawImage * p_img_fig = new SDrawImage(pSid);
			THROW_S(p_img_fig, SLERR_NOMEM);
			THROW(p_img_fig->LoadFile(pFileName));
			p_fig = p_img_fig;
		}
	}
	CATCH
		ZDELETE(p_fig);
	ENDCATCH
	return p_fig;
}

SDrawFigure::SDrawFigure(int kind, const char * pSid)
{
	assert(CheckKind(kind));
	Kind = kind;
	IdPen = 0;
	IdBrush = 0;
	Flags = 0;
	Sid = pSid;
	P_Parent = 0;
}

SDrawFigure::~SDrawFigure()
{
}

//static
SDrawFigure * SDrawFigure::Unserialize(SBuffer & rBuf, SSerializeContext * pCtx)
{
	SDrawFigure * p_instance = 0;
	int32  kind;
	int32  flags;
	size_t save_offs = rBuf.GetRdOffs();
	THROW(pCtx->Serialize(-1, kind, rBuf));
	THROW(pCtx->Serialize(-1, flags, rBuf));
	rBuf.SetRdOffs(save_offs);
	THROW(CheckKind(kind));
	switch(kind) {
		case kShape: p_instance = new SDrawShape; break;
		case kPath:  p_instance = new SDrawPath; break;
		case kImage: p_instance = new SDrawImage; break;
		case kText:  p_instance = new SDrawText; break;
		case kGroup:
			p_instance = (flags & SDrawFigure::fDraw) ? new SDraw : new SDrawGroup;
			break;
		default:
			#define Invalid_DrawFigure_Kind 0
			assert(Invalid_DrawFigure_Kind);
			#undef Invalid_DrawFigure_Kind
			break;
	}
	THROW_S(p_instance, SLERR_NOMEM);
	THROW(p_instance->Serialize(-1, rBuf, pCtx));
	CATCH
		ZDELETE(p_instance);
	ENDCATCH
	return p_instance;
}

int SDrawFigure::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	//
	// Kind & Flags сериализуются в первую очередь для правильной идентификации
	// считываемого из потока объекта функцией SDrawFigure::Unserialize.
	//
	THROW(pCtx->Serialize(dir, Kind, rBuf));
	THROW(pCtx->Serialize(dir, Flags, rBuf));
	if(dir < 0) {
		THROW(CheckKind(Kind));
	}
	THROW(pCtx->Serialize(dir, IdPen, rBuf));
	THROW(pCtx->Serialize(dir, IdBrush, rBuf));
	THROW(pCtx->Serialize(dir, Size, rBuf));
	THROW(pCtx->SerializeBlock(dir, sizeof(Vp), &Vp, rBuf, 1));
	THROW(Tf.Serialize(dir, rBuf, pCtx));
	THROW(pCtx->Serialize(dir, Sid, rBuf));
	// @? SDrawFigure * P_Parent
	CATCHZOK
	return ok;
}

void SDrawFigure::SetTransform(LMatrix2D * pMtx)
{
	if(!RVALUEPTR(Tf, pMtx))
		Tf.InitUnit();
}

void SDrawFigure::SetViewPort(SViewPort * pVp)
{
	if(!RVALUEPTR(Vp, pVp)) {
		Vp.a = 0.0f;
		Vp.b = 0.0f;
		Vp.Flags |= SViewPort::fEmpty;
	}
}

void SDrawFigure::SetSize(FPoint sz)
{
	if(sz.X || sz.Y) {
		Size = sz;
		Flags |= fDefinedSize;
	}
	else {
		Size.SetZero();
		Flags &= ~fDefinedSize;
	}
}

int SDrawFigure::GetViewPort(SViewPort * pVp) const
{
	int    ok = -1;
	SViewPort vp = Vp;
	vp.Flags &= ~SViewPort::fEmpty;
	if(vp.Width() > 0.0f && vp.Height() > 0.0f) {
		ok = 1;
	}
	else if(Size.IsPositive()) {
		*(FRect *)&vp = Size;
		vp.ParX = SViewPort::parMid;
		vp.ParY = SViewPort::parMid;
		ok = 2;
	}
	else {
		vp.a = 0.0f;
		vp.b = 64.0f;
		ok = -1;
	}
	ASSIGN_PTR(pVp, vp);
	return ok;
}

int SDrawFigure::TransformToImage(const SViewPort * pVp, SImageBuffer & rImg)
{
	int    ok = 1;
	FPoint size = rImg.GetDimF();
	if(size.X > 0 && size.Y > 0) {
		LMatrix2D mtx;
		SPaintToolBox tb;
		SPaintToolBox * p_tb = GetToolBox();
		TCanvas2 canv(*NZOR(p_tb, &tb), rImg);
		SViewPort vp;
		if(!RVALUEPTR(vp, pVp))
			GetViewPort(&vp);
		FRect rc(size.X, size.Y);
		canv.SetTransform(vp.GetMatrix(rc, mtx));
		canv.Rect(rc);
		canv.Fill(SColor(255, 255, 255, 0), 0); // Прозрачный фон
		canv.Draw(this);
	}
	else
		ok = -1;
	return ok;
}

SDrawImage * SDrawFigure::DupToImage(TPoint size, const SViewPort * pVp, const char * pSid)
{
	SDrawImage * p_fig = 0;
	SImageBuffer img_buf;
	THROW(img_buf.Init(size.x, size.y));
	THROW(TransformToImage(pVp, img_buf));
	THROW_S(p_fig = new SDrawImage(pSid), SLERR_NOMEM);
	THROW(p_fig->SetBuffer(&img_buf));
	CATCH
		ZDELETE(p_fig);
	ENDCATCH
	return p_fig;
}

void SDrawFigure::SetStyle(int identPen, int identBrush)
{
	IdPen = identPen;
	IdBrush = identBrush;
}

int SDrawFigure::GetKind() const
	{ return Kind; }
const SString & SDrawFigure::GetSid() const
	{ return Sid; }
const FPoint & SDrawFigure::GetSize() const
	{ return Size; }
const SViewPort & SDrawFigure::GetViewPort() const
	{ return Vp; }
void SDrawFigure::SetSid(const char * pSid)
	{ Sid = pSid; }
int SDrawFigure::GetPen() const
	{ return IdPen; }
int SDrawFigure::GetBrush() const
	{ return IdBrush; }
const LMatrix2D & SDrawFigure::GetTransform() const
	{ return Tf; }

SPaintToolBox * SDrawFigure::GetToolBox() const
{
	return (Flags & fDraw) ? ((SDraw *)this)->P_Tb : (P_Parent ? P_Parent->GetToolBox() : 0); // @recursion
}

int FASTCALL SDrawFigure::Copy(const SDrawFigure & rS)
{
	assert(Kind == rS.Kind);
	IdPen = rS.IdPen;
	IdBrush = rS.IdBrush;
	Flags = rS.Flags;
	Sid = rS.Sid;
	Tf = rS.Tf;
	Vp = rS.Vp;
	Size = rS.Size;
	return 1;
}
//
//
//
SDrawShape::SDrawShape(const char * pSid) : SDrawFigure(kShape, pSid)
{
}

int SDrawShape::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	THROW(SDrawFigure::Serialize(dir, rBuf, pCtx));
	THROW(S.Serialize(dir, rBuf, pCtx));
	CATCHZOK
	return ok;
}

int FASTCALL SDrawShape::Copy(const SDrawShape & rS)
{
	int    ok = SDrawFigure::Copy(rS);
	S = rS.S;
	return ok;
}

SDrawFigure * SDrawShape::Dup() const { return DupDrawFigure <SDrawShape> (this); }
//
//
//
SDrawText::SDrawText(const char * pSid) : SDrawFigure(kText, pSid)
{
	IdFont = 0;
	Begin = 0.0f;
}

int SDrawText::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	THROW(SDrawFigure::Serialize(dir, rBuf, pCtx));
	THROW(pCtx->Serialize(dir, IdFont, rBuf));
	THROW(pCtx->Serialize(dir, Begin, rBuf));
	THROW(pCtx->Serialize(dir, Text, rBuf));
	CATCHZOK
	return ok;
}

int FASTCALL SDrawText::Copy(const SDrawText & rS)
{
	int    ok = 1;
	THROW(SDrawFigure::Copy(rS));
	IdFont = rS.IdFont;
	Begin = rS.Begin;
	Text = rS.Text;
	CATCHZOK
	return ok;
}

SDrawFigure * SDrawText::Dup() const { return DupDrawFigure <SDrawText> (this); }
//
//
//
SDrawGroup::SDrawGroup(const char * pSid) : SDrawFigure(kGroup, pSid)
{
}

SDrawFigure * SDrawGroup::Dup() const { return DupDrawFigure <SDrawGroup> (this); }

int SDrawGroup::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	THROW(SDrawFigure::Serialize(dir, rBuf, pCtx));
	uint32 c = getCount();
	THROW(pCtx->Serialize(dir, c, rBuf));
	if(dir > 0) {
		for(uint i = 0; i < c; i++) {
			SDrawFigure * p_item = at(i);
			if(p_item)
				THROW(p_item->Serialize(dir, rBuf, pCtx));
		}
	}
	else if(dir < 0) {
		for(uint i = 0; i < c; i++) {
			SDrawFigure * p_item = 0;
			THROW(p_item = SDrawFigure::Unserialize(rBuf, pCtx));
			THROW(Add(p_item));
		}
	}
	CATCHZOK
	return ok;
}

int FASTCALL SDrawGroup::Copy(const SDrawGroup & rS)
{
	int    ok = 1;
	SDrawFigure::Copy(rS);
	freeAll();
	for(uint i = 0; i < rS.getCount(); i++) {
		const SDrawFigure * p_fig = rS.at(i);
		THROW(p_fig);
		THROW(Add(p_fig->Dup()));
	}
	CATCHZOK
	return ok;
}

void SDrawGroup::Clear()
{
	freeAll();
}

int SDrawGroup::Add(SDrawFigure * pItem)
{
	int    ok = 0;
	if(pItem) {
		if(pItem->Sid.NotEmpty() && Find(pItem->Sid, 1) != 0) {
			SLS.SetError(SLERR_DUPDRAWGROUPSYMB, pItem->Sid);
		}
		else {
			pItem->P_Parent = this;
			ok = SCollection::insert(pItem);
		}
	}
	return ok;
}

int SDrawGroup::Remove(const char * pSid, int recur)
{
	int    ok = -1;
	if(!isempty(pSid)) {
		for(uint i = 0; ok < 0 && i < getCount(); i++) {
			const SDrawFigure * p_item = at(i);
			if(p_item) {
				if(p_item->GetSid() == pSid) {
					atFree(i);
					ok = 1;
				}
				else if(recur && p_item->GetKind() == kGroup)
					ok = ((SDrawGroup *)p_item)->Remove(pSid, recur); // @recursion
			}
		}
	}
	return ok;
}

uint SDrawGroup::GetCount() const
	{ return SCollection::getCount(); }
const SDrawFigure * SDrawGroup::Get(uint pos) const
	{ return (pos < getCount()) ? at(pos) : 0; }

const SDrawFigure * SDrawGroup::Find(const char * pSid, int recur) const
{
	const SDrawFigure * p_result = 0;
	for(uint i = 0; !p_result && i < getCount(); i++) {
		const SDrawFigure * p_item = at(i);
		if(p_item) {
			if(p_item->GetSid() == pSid) {
				p_result = p_item;
			}
			else if(recur && p_item->GetKind() == kGroup) {
				const SDrawFigure * p_inner_item = ((SDrawGroup *)p_item)->Find(pSid, 1); // @recursion
				if(p_inner_item)
					p_result = p_inner_item;
			}
		}
	}
	return p_result;
}
//
//
//
IMPL_INVARIANT_C(SDrawPath)
{
	S_INVARIANT_PROLOG(pInvP);
	S_INVARIANT_EPILOG(pInvP);
}

SDrawPath::SDrawPath(const char * pSid) : SDrawFigure(kPath, pSid)
{
	Helper_Init();
}

SDrawFigure * SDrawPath::Dup() const { return DupDrawFigure <SDrawPath> (this); }

int SDrawPath::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	THROW(SDrawFigure::Serialize(dir, rBuf, pCtx));
	THROW(pCtx->Serialize(dir, Cur, rBuf));
	THROW(pCtx->Serialize(dir, LastMove, rBuf));
	THROW(pCtx->Serialize(dir, &OpList, rBuf));
	THROW(pCtx->Serialize(dir, &ArgList, rBuf));
	CATCHZOK
	return ok;
}

int FASTCALL SDrawPath::Copy(const SDrawPath & rS)
{
	SDrawFigure::Copy(rS);
	Clear();
	Cur = rS.Cur;
	LastMove = rS.LastMove;
	OpList = rS.OpList;
	ArgList = rS.ArgList;
	return 1;
}

void SDrawPath::Helper_Init()
{
	Flags = 0;
	Cur.Set(0.0);
	LastMove.Set(0.0);
}

void SDrawPath::Clear()
{
	Helper_Init();
	OpList.clear();
	ArgList.clear();
}

int SDrawPath::HasCurrent() const
{
	const uint c = OpList.getCount();
	return BIN(c && OpList.at(c-1).Key != opNop);
}

const FPoint & SDrawPath::GetCurrent()
{
	if(HasCurrent()) {
		uint   rc = ArgList.getCount();
		assert(rc >= 2);
		Cur.Set(ArgList[rc-2], ArgList[rc-1]);
	}
	else
		Cur.Set(0.0);
	return Cur;
}

int SDrawPath::IsEmpty() const
	{ return (OpList.getCount() == 0); }
uint SDrawPath::GetCount() const
	{ return OpList.getCount(); }

const SDrawPath::Item * FASTCALL SDrawPath::Get(uint i, SDrawPath::Item & rItem) const
{
	if(i < OpList.getCount()) {
		rItem.Op = OpList.at(i).Key;
		assert(CheckOp(rItem.Op));
		rItem.ArgCount = GetOpArgCount(rItem.Op);
		if(rItem.ArgCount)
			rItem.P_ArgList = &ArgList[(uint)OpList.at(i).Val];
		else
			rItem.P_ArgList = 0;
		return &rItem;
	}
	else
		return 0;
}

int FASTCALL SDrawPath::CheckOp(int op) const
{
	return BIN(oneof7(op, opNop, opMove, opLine, opCurve, opQuad, opArcSvg, opClose));
}

uint FASTCALL SDrawPath::GetOpArgCount(int op) const
{
	uint   c = 0;
	switch(op) {
		case opNop: c = 0; break;
		case opMove: c = 2; break;
		case opLine: c = 2; break;
		case opCurve: c = 6; break;
		case opQuad: c = 4; break;
		case opArcSvg: c = 7; break;
		case opClose: c = 0; break;
	}
	return c;
}

int SDrawPath::Nop()
{
	Flags &= ~(fHasCur | fHasLastMove);
	return OpList.Add(opNop, -1, 0);
}

int FASTCALL SDrawPath::Move(const FPoint & rP)
{
	int    ok = 1;
	Flags |= fHasLastMove;
	Flags |= fHasCur;
	OpList.Add(opMove, ArgList.getCount(), 0);
	ArgList.add(rP);
	LastMove = rP;
	Cur = rP;
	return ok;
}

int FASTCALL SDrawPath::Line(const FPoint & rP)
{
	int    ok = 1;
	if(Flags & fHasCur) {
		OpList.Add(opLine, ArgList.getCount(), 0);
		ArgList.add(rP);
		Cur = rP;
	}
	else
		ok = Move(rP);
	return ok;
}

int SDrawPath::Curve(const FPoint & rMiddle1, const FPoint & rMiddle2, const FPoint & rEnd)
{
	int    ok = 1;
	if(!(Flags & fHasCur)) {
		Move(rMiddle1);
	}
	OpList.Add(opCurve, ArgList.getCount(), 0);
	ArgList.add(rMiddle1);
	ArgList.add(rMiddle2);
	ArgList.add(rEnd);
	Cur = rEnd;
	return ok;
}

int SDrawPath::Quad(const FPoint & rMiddle, const FPoint & rEnd)
{
	int    ok = 1;
	OpList.Add(opQuad, ArgList.getCount(), 0);
	ArgList.add(rMiddle);
	ArgList.add(rEnd);
	Cur = rEnd;
	return ok;
}

int SDrawPath::ArcSvg(const FPoint & rCenter, float xAxRotation, int largeFlag, int sweepFlag, const FPoint & rEnd)
{
	int    ok = 1;
	// @todo Проверить на существование текущей точки
	OpList.Add(opArcSvg, ArgList.getCount(), 0);
	ArgList.add(rCenter);
	ArgList.add(xAxRotation);
	ArgList.add((float)largeFlag);
	ArgList.add((float)sweepFlag);
	ArgList.add(rEnd);
	Cur = rEnd;
	return ok;
}

int SDrawPath::Close()
{
	int    ok = 1;
	OpList.Add(opClose, -1, 0);
	Cur = LastMove;
	return ok;
}

static int GetSvgPathNumber(SStrScan & rScan, SString & rTempBuf, float & rF)
{
	int    ok = 1;
	if(rScan.Skip().GetDotPrefixedNumber(rTempBuf))
		rF = (float)rTempBuf.ToReal();
	else
		ok = 0;
	return ok;
}

static int GetSvgPathPoint(SStrScan & rScan, SString & rTempBuf, FPoint & rP)
{
	int    ok = 1;
	THROW(rScan.Skip().GetDotPrefixedNumber(rTempBuf));
	rP.X = (float)rTempBuf.ToReal();
	rScan.Skip().IncrChr(',');
	THROW(rScan.Skip().GetDotPrefixedNumber(rTempBuf));
	rP.Y = (float)rTempBuf.ToReal();
	CATCHZOK
	return ok;
}

int SDrawPath::FromStr(const char * pStr, int fmt)
{
	int    ok = 1;
	if(fmt == fmtSVG) {
		SString temp_buf;
		SStrScan scan(pStr);
		FPoint pnt;
		FPoint pa[4];
		float  nmb;
		for(int c = scan.Skip()[0]; c != 0; c = scan.Skip()[0]) {
			scan.Incr();
			switch(c) {
				case 'M': // 2, SVG_PATH_CMD_MOVE_TO
					for(int first = 1; scan.Skip().IsDotPrefixedNumber(); first = 0) {
						THROW(GetSvgPathPoint(scan, temp_buf, pnt));
						if(first)
							Move(pnt);
						else
							Line(pnt);
					}
					break;
				case 'm': // 2, SVG_PATH_CMD_REL_MOVE_TO
					for(int first = 1; scan.Skip().IsDotPrefixedNumber(); first = 0) {
						THROW(GetSvgPathPoint(scan, temp_buf, pnt));
						const FPoint cur = GetCurrent();
						if(first)
							Move(cur + pnt);
						else
							Line(cur + pnt);
					}
					break;
				case 'L': // 2, SVG_PATH_CMD_LINE_TO
					while(scan.Skip().IsDotPrefixedNumber()) {
						THROW(GetSvgPathPoint(scan, temp_buf, pnt));
						Line(pnt);
					}
					break;
				case 'l': // 2, SVG_PATH_CMD_REL_LINE_TO
					while(scan.Skip().IsDotPrefixedNumber()) {
						THROW(GetSvgPathPoint(scan, temp_buf, pnt));
						const FPoint cur = GetCurrent();
						Line(cur + pnt);
					}
					break;
				case 'H': // 1, SVG_PATH_CMD_HORIZONTAL_LINE_TO
					while(scan.Skip().IsDotPrefixedNumber()) {
						THROW(GetSvgPathNumber(scan, temp_buf, nmb));
						const FPoint cur = GetCurrent();
						Line(pnt.Set(nmb, cur.Y));
					}
					break;
				case 'h': // 1, SVG_PATH_CMD_REL_HORIZONTAL_LINE_TO
					while(scan.Skip().IsDotPrefixedNumber()) {
						THROW(GetSvgPathNumber(scan, temp_buf, nmb));
						pnt = GetCurrent();
						Line(pnt.AddX(nmb));
					}
					break;
				case 'V': // 1, SVG_PATH_CMD_VERTICAL_LINE_TO
					while(scan.Skip().IsDotPrefixedNumber()) {
						THROW(GetSvgPathNumber(scan, temp_buf, nmb));
						const FPoint cur = GetCurrent();
						Line(pnt.Set(cur.X, nmb));
					}
					break;
				case 'v': // 1, SVG_PATH_CMD_REL_VERTICAL_LINE_TO
					while(scan.Skip().IsDotPrefixedNumber()) {
						THROW(GetSvgPathNumber(scan, temp_buf, nmb));
						pnt = GetCurrent();
						Line(pnt.AddY(nmb));
					}
					break;
				case 'C': // 6, SVG_PATH_CMD_CURVE_TO
					while(scan.Skip().IsDotPrefixedNumber()) {
						THROW(GetSvgPathPoint(scan, temp_buf, pa[0]));
						scan.Skip().IncrChr(',');
						THROW(GetSvgPathPoint(scan, temp_buf, pa[1]));
						scan.Skip().IncrChr(',');
						THROW(GetSvgPathPoint(scan, temp_buf, pa[2]));
						Curve(pa[0], pa[1], pa[2]);
					}
					break;
				case 'c': // 6, SVG_PATH_CMD_REL_CURVE_TO
					while(scan.Skip().IsDotPrefixedNumber()) {
						THROW(GetSvgPathPoint(scan, temp_buf, pa[0]));
						scan.Skip().IncrChr(',');
						THROW(GetSvgPathPoint(scan, temp_buf, pa[1]));
						scan.Skip().IncrChr(',');
						THROW(GetSvgPathPoint(scan, temp_buf, pa[2]));
						const FPoint cur = GetCurrent();
						Curve(cur + pa[0], cur + pa[1], cur + pa[2]);
					}
					break;
				case 'S': // 4, SVG_PATH_CMD_SMOOTH_CURVE_TO
					while(scan.Skip().IsDotPrefixedNumber()) { // @v8.9.9
						THROW(GetSvgPathPoint(scan, temp_buf, pa[0]));
						scan.Skip().IncrChr(',');
						THROW(GetSvgPathPoint(scan, temp_buf, pa[1]));
						if(OpList.getCount() && OpList.at(OpList.getCount()-1).Key == opCurve) {
							pnt = ArgList.getPoint(ArgList.getCount()-2);
							FPoint pnt_1 = ArgList.getPoint(ArgList.getCount()-4);
							FPoint rpnt(2.0f * pnt.X - pnt_1.X, 2.0f * pnt.Y - pnt_1.Y);
							Curve(rpnt, pa[0], pa[1]);
						}
						else {
							const FPoint cur = GetCurrent();
							Curve(cur, pa[0], pa[1]);
						}
					}
					break;
				case 's': // 4, SVG_PATH_CMD_REL_SMOOTH_CURVE_TO
					while(scan.Skip().IsDotPrefixedNumber()) { // @v8.9.9
						THROW(GetSvgPathPoint(scan, temp_buf, pa[0]));
						scan.Skip().IncrChr(',');
						THROW(GetSvgPathPoint(scan, temp_buf, pa[1]));
						if(OpList.getCount() && OpList.at(OpList.getCount()-1).Key == opCurve) {
							pnt = ArgList.getPoint(ArgList.getCount()-2);
							FPoint pnt_1 = ArgList.getPoint(ArgList.getCount()-4);
							FPoint rpnt(2.0f * pnt.X - pnt_1.X, 2.0f * pnt.Y - pnt_1.Y);
							const FPoint cur = GetCurrent();
							Curve(rpnt, cur + pa[0], cur + pa[1]);
						}
						else {
							const FPoint cur = GetCurrent();
							Curve(cur, cur + pa[0], cur + pa[1]);
						}
					}
					break;
				case 'Q': // 4, SVG_PATH_CMD_QUADRATIC_CURVE_TO
					THROW(GetSvgPathPoint(scan, temp_buf, pa[0]));
					scan.Skip().IncrChr(',');
					THROW(GetSvgPathPoint(scan, temp_buf, pa[1]));
					Quad(pa[0], pa[1]);
					break;
				case 'q': // 4, SVG_PATH_CMD_REL_QUADRATIC_CURVE_TO
					{
						THROW(GetSvgPathPoint(scan, temp_buf, pa[0]));
						scan.Skip().IncrChr(',');
						THROW(GetSvgPathPoint(scan, temp_buf, pa[1]));
						const FPoint cur = GetCurrent();
						Quad(cur + pa[0], cur + pa[1]);
					}
					break;
				case 'T': // 2, SVG_PATH_CMD_SMOOTH_QUADRATIC_CURVE_TO
					{
						THROW(GetSvgPathPoint(scan, temp_buf, pa[0]));
						if(OpList.getCount() && OpList.at(OpList.getCount()-1).Key == opQuad) {
							//FPoint refl_p2 = ArgList[ArgList.getCount()-1];
							//FPoint refl_p1 = ArgList[ArgList.getCount()-2];
							//pnt = refl_p2 + refl_p2 - refl_p1;
							pnt = ArgList.getPoint(ArgList.getCount()-4);
							Quad(pnt, pa[0]);
						}
						else {
							const FPoint cur = GetCurrent();
							Quad(cur, pa[0]);
						}
					}
					break;
				case 't': // 2, SVG_PATH_CMD_REL_SMOOTH_QUADRATIC_CURVE_TO
					{
						THROW(GetSvgPathPoint(scan, temp_buf, pa[0]));
						if(OpList.getCount() && OpList.at(OpList.getCount()-1).Key == opQuad) {
							pnt = ArgList.getPoint(ArgList.getCount()-4);
							const FPoint cur = GetCurrent();
							Quad(pnt, cur+pa[0]);
						}
						else {
							const FPoint cur = GetCurrent();
							Quad(cur, cur+pa[0]);
						}
					}
					break;
				case 'A': // 7, SVG_PATH_CMD_ARC_TO
					{
						float x_axis_rotation = 0.0f;
						float large_arc_flag = 0.0f;
						float sweep_flag = 0.0f;
						THROW(GetSvgPathPoint(scan, temp_buf, pa[0])); // radius
						THROW(GetSvgPathNumber(scan, temp_buf, x_axis_rotation));
						THROW(GetSvgPathNumber(scan, temp_buf, large_arc_flag));
						THROW(GetSvgPathNumber(scan, temp_buf, sweep_flag));
						THROW(GetSvgPathPoint(scan, temp_buf, pa[1])); // start point
						ArcSvg(pa[0], x_axis_rotation, (int)large_arc_flag, (int)sweep_flag, pa[1]);
					}
					break;
				case 'a': // 7, SVG_PATH_CMD_REL_ARC_TO
					{
						float x_axis_rotation = 0.0f;
						float large_arc_flag = 0.0f;
						float sweep_flag = 0.0f;
						THROW(GetSvgPathPoint(scan, temp_buf, pa[0])); // radius
						THROW(GetSvgPathNumber(scan, temp_buf, x_axis_rotation));
						THROW(GetSvgPathNumber(scan, temp_buf, large_arc_flag));
						THROW(GetSvgPathNumber(scan, temp_buf, sweep_flag));
						THROW(GetSvgPathPoint(scan, temp_buf, pa[1])); // start point
						const FPoint cur = GetCurrent();
						ArcSvg(pa[0], x_axis_rotation, (int)large_arc_flag, (int)sweep_flag, cur + pa[1]);
					}
					break;
				case 'Z': // 0, SVG_PATH_CMD_CLOSE_PATH
				case 'z': // 0, SVG_PATH_CMD_CLOSE_PATH
					Close();
					break;
				default:
					CALLEXCEPT();
			}
		}
	}
	else {
		CALLEXCEPT();
	}
	CATCHZOK
	return ok;
}

SDraw::SDraw() : SDrawGroup(0)
{
	Flags |= fDraw;
	P_Tb = 0;
}

SDraw::SDraw(const char * pSid) : SDrawGroup(pSid)
{
	Flags |= fDraw;
	P_Tb = 0;
}

SDraw::SDraw(const char * pSid, SPaintToolBox * pOuterTb) : SDrawGroup(pSid)
{
	Flags |= fDraw;
	if(pOuterTb) {
		P_Tb = pOuterTb;
		Flags |= fOuterToolbox;
	}
	else
		P_Tb = new SPaintToolBox();
}

SDraw::~SDraw()
{
	if(!(Flags & fOuterToolbox))
		ZDELETE(P_Tb);
}

SDrawFigure * SDraw::Dup() const { return DupDrawFigure <SDraw> (this); }

int FASTCALL SDraw::Copy(const SDraw & rS)
{
	int    ok = 1;
	THROW(SDrawGroup::Copy(rS));
	if(rS.Flags & fOuterToolbox) {
		P_Tb = rS.P_Tb;
	}
	else {
		ZDELETE(P_Tb);
		if(rS.P_Tb) {
			THROW(P_Tb = new SPaintToolBox);
			THROW(P_Tb->Copy(*rS.P_Tb));
		}
	}
	UCtx.Dpi = rS.UCtx.Dpi;
	UCtx.FontSize = rS.UCtx.FontSize;
	CATCHZOK
	return ok;
}

int SDraw::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	SDrawGroup::Serialize(dir, rBuf, pCtx);
	uint8 ind = 0;
	if(dir > 0) {
		THROW(rBuf.Write(ind = P_Tb ? 0 : 1));
		if(P_Tb)
			THROW(P_Tb->Serialize(dir, rBuf, pCtx));
	}
	else if(dir < 0) {
		THROW(rBuf.Read(ind));
		THROW(oneof2(ind, 0, 1));
		if(!(Flags & fOuterToolbox))
			ZDELETE(P_Tb);
		if(ind == 0) {
			Flags &= ~fOuterToolbox;
			THROW_S(P_Tb = new SPaintToolBox, SLERR_NOMEM);
			THROW(P_Tb->Serialize(dir, rBuf, pCtx));
		}
	}
	CATCHZOK
	return ok;
}

int SDraw::SetupUnitContext(SDrawContext & rDCtx)
{
	int    ok = 1;
	SDrawContext::UC * p_uc = rDCtx.GetUnitContext();
	if(!RVALUEPTR(UCtx, p_uc))
		ok = 0;
	delete p_uc;
	return ok;
}

int SDraw::ConvertCoord(const USize & rUsz, double * pR) const
{
	int    ok = 1;
	if(rUsz.Unit == 0) {
		ASSIGN_PTR(pR, rUsz);
	}
	else {
		USize s;
		s.Set(0.0, UNIT_GR_PIXEL, rUsz.Dir);
		if(rUsz.Unit && !SUnit::Convert(rUsz, s, &UCtx)) {
			ASSIGN_PTR(pR, 0.0);
			ok = 0;
		}
		else {
			ASSIGN_PTR(pR, s);
		}
	}
	return ok;
}

int SLAPI _ParseSvgFile(const char * pFileName, SDraw & rResult); // @prototype @defined(ssvg.cpp)

int SDraw::ParseSvgFile(const char * pFileName)
{
	return _ParseSvgFile(pFileName, *this);
}
//
//
//
SImageBuffer::Palette::Palette()
{
	Count = 0;
	P_Buf = 0;
}

SImageBuffer::Palette::~Palette()
{
	ZFREE(P_Buf);
}

int SImageBuffer::Palette::Alloc(uint count)
{
	int    ok = 1;
	if(count > Count) {
		P_Buf = (uint32 *)SAlloc::R(P_Buf, count * sizeof(uint32));
		if(P_Buf)
			Count = count;
		else {
			Count = 0;
			ok = 0;
		}
	}
	else
		ok = -1;
	return ok;
}

void SImageBuffer::Palette::SetRGB(size_t idx, uint8 r, uint8 g, uint8 b)
{
	if(idx < Count)
		P_Buf[idx] = ((0xff << 24) | (r << 16) | (g << 8) | b);
}

void SImageBuffer::Palette::SetAlpha(uint8 alpha)
{
	for(uint i = 0; i < Count; i++)
		P_Buf[i] = ((alpha << 24) | (P_Buf[i] & 0x00ffffff));
}

uint SImageBuffer::Palette::GetCount() const
	{ return Count; }
size_t SImageBuffer::Palette::GetSize() const
	{ return (Count * sizeof(uint32)); }
uint32 FASTCALL SImageBuffer::Palette::GetColor(uint idx) const
	{ return (idx < Count) ? P_Buf[idx] : 0; }
void * SImageBuffer::Palette::GetBuffer() // really private
	{ return P_Buf; }
const uint32 * SImageBuffer::Palette::GetBufferC() const // really private
	{ return (const uint32 *)P_Buf; }
//
//
//
SImageBuffer::PixF::PixF(int s)
{
	S = s;
}

int SImageBuffer::PixF::IsValid() const
{
	return (GetBpp() != 0);
}

uint SImageBuffer::PixF::GetBpp() const
{
	switch(S) {
		case s32ARGB:      return 32;
		case s16ARGB1555:  return 16;
		case s16GrayScale: return 16;
		case s16RGB555:    return 16;
		case s16RGB565:    return 16;
		case s1A:          return 1;
		case s24RGB:       return 24;
		case s32PARGB:     return 32;
		case s32RGB:       return 32;
		case s48RGB:       return 48;
		case s64ARGB:      return 64;
		case s64PARGB:     return 64;
		case s8GrayScale:  return 8;
		case s8A:          return 8;
		case s1Idx:        return 1;
		case s2Idx:        return 2;
		case s4Idx:        return 4;
		case s8Idx:        return 8;
		case s16Idx:       return 16;
	}
	return 0;
}

uint FASTCALL SImageBuffer::PixF::GetStride(uint width) const
{
	const  uint bpp = GetBpp();
	if(bpp) {
		const  uint m = (bpp * width);
		/*const*/  uint junk = ((~(m-1)>>3) & 3); // неиспользуемый "хвост" строки
		if(S == s24RGB) {
			junk = 0; // @v9.6.11 Что-то не понятное с этим "хвостиком"
		}
		return (ALIGNSIZE(m, 3) >> 3) + junk;
	}
	else
		return 0;
}

// static
uint32 FASTCALL SImageBuffer::PixF::UniformToGrayscale(uint32 u)
{
	const uint32 _r = (u & 0x00ff0000) >> 16;
	const uint32 _g = (u & 0x0000ff00) >> 8;
	const uint32 _b = (u & 0x000000ff);
	return (_r == _g && _r == _b) ? _r : ((_r * 307 + _g * 604 + _b * 113) >> 10);
}

int SImageBuffer::PixF::SetUniform(const void * pUniformBuf, void * pDest, uint width, SImageBuffer::Palette * pPalette) const
{
	int    ok = 0;
	const  uint bpp = GetBpp();
	const  uint32 * p_ufb = (const uint32 *)pUniformBuf;
	const  uint q = width / 4;
	const  uint r = width % 4;
	if(oneof5(S, s1Idx, s2Idx, s4Idx, s8Idx, s16Idx)) {
	}
	else {
		switch(bpp) {
			case 8:
				{
					uint8 * p = (uint8 *)pDest;
					switch(S) {
						case s8A:
							break;
						case s8GrayScale:
							{
								const uint dq = width / 8;
								const uint dr = width % 8;
								for(uint i = 0; i < dq; ++i) {
									*p++ = (uint8)SImageBuffer::PixF::UniformToGrayscale(*p_ufb++);
									*p++ = (uint8)SImageBuffer::PixF::UniformToGrayscale(*p_ufb++);
									*p++ = (uint8)SImageBuffer::PixF::UniformToGrayscale(*p_ufb++);
									*p++ = (uint8)SImageBuffer::PixF::UniformToGrayscale(*p_ufb++);
									*p++ = (uint8)SImageBuffer::PixF::UniformToGrayscale(*p_ufb++);
									*p++ = (uint8)SImageBuffer::PixF::UniformToGrayscale(*p_ufb++);
									*p++ = (uint8)SImageBuffer::PixF::UniformToGrayscale(*p_ufb++);
									*p++ = (uint8)SImageBuffer::PixF::UniformToGrayscale(*p_ufb++);
								}
								for(uint i = 0; i < dr; ++i) {
									*p++ = (uint8)SImageBuffer::PixF::UniformToGrayscale(*p_ufb++);
								}
							}
							ok = 1;
							break;
						default:
							ok = 0;
							break;
					}

				}
				break;
			case 16:
				{
					uint16 * p = (uint16 *)pDest;
					switch(S) {
						case s16ARGB1555:
							break;
						case s16RGB555:
							break;
						case s16RGB565:
							break;
						case s16GrayScale:
							{
								const uint dq = width / 8;
								const uint dr = width % 8;
								for(uint i = 0; i < dq; ++i) {
									*p++ = (uint16)SImageBuffer::PixF::UniformToGrayscale(*p_ufb++);
									*p++ = (uint16)SImageBuffer::PixF::UniformToGrayscale(*p_ufb++);
									*p++ = (uint16)SImageBuffer::PixF::UniformToGrayscale(*p_ufb++);
									*p++ = (uint16)SImageBuffer::PixF::UniformToGrayscale(*p_ufb++);
									*p++ = (uint16)SImageBuffer::PixF::UniformToGrayscale(*p_ufb++);
									*p++ = (uint16)SImageBuffer::PixF::UniformToGrayscale(*p_ufb++);
									*p++ = (uint16)SImageBuffer::PixF::UniformToGrayscale(*p_ufb++);
									*p++ = (uint16)SImageBuffer::PixF::UniformToGrayscale(*p_ufb++);
								}
								for(uint i = 0; i < dr; ++i) {
									*p++ = (uint16)SImageBuffer::PixF::UniformToGrayscale(*p_ufb++);
								}
							}
							ok = 1;
							break;
						default:
							ok = 0;
							break;
					}
				}
				break;
			case 24:
				{
					switch(S) {
						case s24RGB:
							break;
						default:
							ok = 0;
							break;
					}

				}
				break;
			case 32:
				{
					switch(S) {
						case s32ARGB:
							break;
						case s32PARGB:
							break;
						case s32RGB:
							break;
						default:
							ok = 0;
							break;
					}
				}
				break;
			case 48:
				{
					ok = 0;
				}
				break;
			case 64:
				{
					ok = 0;
				}
				break;
			default:
				ok = 0;
				break;
		}
	}
	return ok;
}

int SImageBuffer::PixF::GetUniform(const void * pSrc, void * pUniformBuf, uint width, const SImageBuffer::Palette * pPalette) const
{
	int    ok = 1;
	uint   bpp = GetBpp();
	uint32 * p_ufb = PTR32(pUniformBuf);
	const  uint q = width / 4;
	const  uint r = width % 4;
	uint   i;
	if(oneof5(S, s1Idx, s2Idx, s4Idx, s8Idx, s16Idx)) {
		if(pPalette == 0)
			ok = 0;
		/* @v7.0.0
		else if(pPalette->GetCount() < (1U << bpp))
			ok = 0;
		*/
		else if(S == s16Idx) {
			for(i = 0; i < width; ++i)
				*p_ufb++ = pPalette->GetColor(PTR16(pSrc)[i]);
		}
		else if(S == s8Idx) {
			const uint oct_count = width / 8;
			const uint oct_tail = (oct_count * 8) + (width % 8);
			const uint32 * p_palette_buf = pPalette->GetBufferC();
			const uint8 * p_src8 = PTR8(pSrc);
			for(i = 0; i < oct_count; i += 8) {
				p_ufb[0] = p_palette_buf[p_src8[i]];
				p_ufb[1] = p_palette_buf[p_src8[i+1]];
				p_ufb[2] = p_palette_buf[p_src8[i+2]];
				p_ufb[3] = p_palette_buf[p_src8[i+3]];
				p_ufb[4] = p_palette_buf[p_src8[i+4]];
				p_ufb[5] = p_palette_buf[p_src8[i+5]];
				p_ufb[6] = p_palette_buf[p_src8[i+6]];
				p_ufb[7] = p_palette_buf[p_src8[i+7]];
				p_ufb += 8;
			}
			while(i < width)
				*p_ufb++ = p_palette_buf[p_src8[i++]];
			/* Медленный, но безопасный вариант
			for(i = 0; i < width; ++i)
				*p_ufb++ = pPalette->GetColor(p_src8[i]);
			*/
		}
		else if(S == s4Idx) {
			for(i = 0; i < width/2; ++i) {
				uint8 b = PTR8(pSrc)[i];
				*p_ufb++ = pPalette->GetColor((b & 0xf0) >> 4);
				*p_ufb++ = pPalette->GetColor(b & 0x0f);
			}
		}
		else if(S == s2Idx) {
			for(i = 0; i < q; ++i) {
				uint8 b = PTR8(pSrc)[i];
				*p_ufb++ = pPalette->GetColor((b & 0xc0) >> 6);
				*p_ufb++ = pPalette->GetColor((b & 0x30) >> 4);
				*p_ufb++ = pPalette->GetColor((b & 0x0c) >> 2);
				*p_ufb++ = pPalette->GetColor(b & 0x03);
			}
		}
		else if(S == s1Idx) {
			for(i = 0; i < width/8; ++i) {
				uint8 b = PTR8(pSrc)[i];
				*p_ufb++ = pPalette->GetColor((b & 0x80) >> 7);
				*p_ufb++ = pPalette->GetColor((b & 0x40) >> 6);
				*p_ufb++ = pPalette->GetColor((b & 0x20) >> 5);
				*p_ufb++ = pPalette->GetColor((b & 0x10) >> 4);
				*p_ufb++ = pPalette->GetColor((b & 0x08) >> 3);
				*p_ufb++ = pPalette->GetColor((b & 0x04) >> 2);
				*p_ufb++ = pPalette->GetColor((b & 0x02) >> 1);
				*p_ufb++ = pPalette->GetColor(b & 0x01);
			}
		}
	}
	else {
		switch(bpp) {
			case 8:
				{
					const uint8 * p = (const uint8 *)pSrc;
					switch(S) {
						case s8A:
							{
								const uint dq = width / 8;
								const uint dr = width % 8;
								const uint bs = width*8;
								for(i = 0; i < dq; i++) {
	#define OP(c) ((c)|((c)<<1)|((c)<<2)|((c)<<3))<<24
									uint32  b[8];
									const uint j = (i*8);
									b[0] = getbit32(pSrc, bs, j+0);
									b[1] = getbit32(pSrc, bs, j+1);
									b[2] = getbit32(pSrc, bs, j+2);
									b[3] = getbit32(pSrc, bs, j+3);
									b[4] = getbit32(pSrc, bs, j+4);
									b[5] = getbit32(pSrc, bs, j+5);
									b[6] = getbit32(pSrc, bs, j+6);
									b[7] = getbit32(pSrc, bs, j+7);
									*p_ufb++ = OP(p[0]);
									*p_ufb++ = OP(p[1]);
									*p_ufb++ = OP(p[2]);
									*p_ufb++ = OP(p[3]);
									*p_ufb++ = OP(p[4]);
									*p_ufb++ = OP(p[5]);
									*p_ufb++ = OP(p[6]);
									*p_ufb++ = OP(p[7]);
								}
								for(i = 0; i < dr; ++i) {
									uint32 b = getbit32(pSrc, bs, i+dq*8);
									*p_ufb++ = OP(b);
								}
							}
	#undef OP
							break;
						case s8GrayScale:
							{
								const uint dq = width / 8;
								const uint dr = width % 8;
								for(i = 0; i < dq; ++i) {
	#define OP(c) (0xff000000|((c)<<16)|((c)<<8)|((c)))
									*p_ufb++ = OP(*p); p++;
									*p_ufb++ = OP(*p); p++;
									*p_ufb++ = OP(*p); p++;
									*p_ufb++ = OP(*p); p++;
									*p_ufb++ = OP(*p); p++;
									*p_ufb++ = OP(*p); p++;
									*p_ufb++ = OP(*p); p++;
									*p_ufb++ = OP(*p); p++;
								}
								for(i = 0; i < dr; ++i) {
									*p_ufb++ = OP(*p); p++;
								}
							}
	#undef OP
							break;
						default:
							ok = 0;
							break;
					}

				}
				break;
			case 16:
				{
					const uint16 * p = (const uint16 *)pSrc;
					uint i;
					switch(S) {
						case s16ARGB1555:
	#define OP(c) (((uint32)((uint8)(0-((c&0x8000)>>15)))<<24)|(((c&0x7c00)|((c&0x7000)>>5))<<9)|(((c&0x3e0)|((c&0x380)>>5))<<6)|(((c&0x1c)|((c&0x1f)<<5))>>2))
							for(i = 0; i < q; ++i) {
								*p_ufb++ = OP(*p); p++;
								*p_ufb++ = OP(*p); p++;
								*p_ufb++ = OP(*p); p++;
								*p_ufb++ = OP(*p); p++;
							}
							for(i = 0; i < r; ++i) {
								*p_ufb++ = OP(*p); p++;
							}
							break;
	#undef OP
						case s16RGB555:
	#define OP(c) (0xff000000|(((c&0x7c00)|((c&0x7000)>>5))<<9)|(((c&0x3e0)|((c&0x380)>>5))<<6)|(((c&0x1c)|((c&0x1f)<<5))>>2))
							for(i = 0; i < q; ++i) {
								*p_ufb++ = OP(*p); p++;
								*p_ufb++ = OP(*p); p++;
								*p_ufb++ = OP(*p); p++;
								*p_ufb++ = OP(*p); p++;
							}
							for(i = 0; i < r; ++i) {
								*p_ufb++ = OP(*p); p++;
							}
							break;
	#undef OP
						case s16RGB565:
	#define OP(c) (0xff000000|(((c&0xf800)|((c&0xe000)>>5))<<8)|(((c&0x7e0)|((c&0x600)>>6))<<5)|(((c&0x1c)|((c&0x1f)<<5))>>2))
							for(i = 0; i < q; ++i) {
								*p_ufb++ = OP(*p); p++;
								*p_ufb++ = OP(*p); p++;
								*p_ufb++ = OP(*p); p++;
								*p_ufb++ = OP(*p); p++;
							}
							for(i = 0; i < r; ++i) {
								*p_ufb++ = OP(*p); p++;
							}
							break;
	#undef OP
						default:
							ok = 0;
							break;
					}
				}
				break;
			case 24:
				{
					const uint8 * p = (const uint8 *)pSrc;
					uint   i;
					switch(S) {
						case s24RGB:
	#define OP (0xff000000|(((uint32)p[0])<<16)|(((uint32)p[1])<<8)|((uint32)p[2]))
	//#define OP (0xff000000|(((uint32)p[2])<<16)|(((uint32)p[1])<<8)|((uint32)p[0])) @v7.4.1 <->
							{
								for(i = 0; i < q; ++i) {
									*p_ufb++ = OP; p += 3;
									*p_ufb++ = OP; p += 3;
									*p_ufb++ = OP; p += 3;
									*p_ufb++ = OP; p += 3;
								}
								for(i = 0; i < r; ++i) {
									*p_ufb++ = OP; p += 3;
								}
							}
							break;
	#undef OP
						default:
							ok = 0;
							break;
					}

				}
				break;
			case 32:
				{
					const uint32 * p = (const uint32 *)pSrc;
					uint i;
					switch(S) {
						case s32ARGB:
							memcpy(p_ufb, p, width * sizeof(uint32));
							break;
						case s32PARGB:
	#define __ALPHA(c) ((c&0xff000000)>>24)
	#define __R(c)     ((c&0x00ff0000)>>16)
	#define __G(c)     ((c&0x0000ff00)>>8)
	#define __B(c)     ((c&0x000000ff))
	#define MULTIPLY_ALPHA(a,c) (uint8)(((((a)*(c))+0x80)+((((a)*(c))+0x80)>>8))>>8)
							for(i = 0; i < width; ++i) {
								uint32 c = *p++;
								uint32 a = __ALPHA(c);
								if(a == 0xff)
									*p_ufb = c;
								else if(a != 0)
									*p_ufb = ((a<<24)|(MULTIPLY_ALPHA(a, __R(c))<<16)|(MULTIPLY_ALPHA(a, __G(c))<<8)|(MULTIPLY_ALPHA(a, __B(c))));
								else
									*p_ufb = 0;
								p_ufb++;
							}
							break;
	#undef __ALPHA
	#undef __R
	#undef __G
	#undef __B
						case s32RGB:
							for(i = 0; i < q; ++i) {
								*p_ufb++ = (*p|0xff000000); p++;
								*p_ufb++ = (*p|0xff000000); p++;
								*p_ufb++ = (*p|0xff000000); p++;
								*p_ufb++ = (*p|0xff000000); p++;
							}
							for(i = 0; i < r; ++i) {
								*p_ufb++ = (*p|0xff000000); p++;
							}
							break;
						default:
							ok = 0;
							break;
					}
				}
				break;
			case 48:
				{
					ok = 0;
				}
				break;
			case 64:
				{
					ok = 0;
				}
				break;
			default:
				ok = 0;
				break;
		}
	}
	return ok;
}

SImageBuffer::StoreParam::StoreParam(int fmt)
{
	Fmt = fmt;
	Flags = 0;
}

SImageBuffer::SImageBuffer()
{
	SBaseBuffer::Init();
	S = 0;
}

SImageBuffer::SImageBuffer(const SImageBuffer & rS)
{
	SBaseBuffer::Init();
	S = 0;
	Copy(rS);
}

SImageBuffer::SImageBuffer(uint w, uint h, PixF f)
{
	SBaseBuffer::Init();
	S = 0;
	Init(w, h, f);
}

SImageBuffer::~SImageBuffer()
{
	Destroy();
}

void SImageBuffer::Destroy()
{
	S = 0;
	SBaseBuffer::Destroy();
}

SImageBuffer & FASTCALL SImageBuffer::operator = (const SImageBuffer & rS)
{
	Copy(rS);
	return *this;
}

void * SImageBuffer::CreateSurface(SDrawSystem sys) const
{
	void * p_result = 0;
	if(sys == dsysCairo) {
		p_result = cairo_image_surface_create_for_data((uchar *)P_Buf, CAIRO_FORMAT_ARGB32, GetDim().x, GetDim().y, F.GetStride(GetDim().x));
	}
	return p_result;
}

int SImageBuffer::TransformToBounds(TPoint size, const SViewPort * pVp)
{
	int    ok = 1;
	cairo_t * p_cr = 0;
	cairo_surface_t * p_surf = 0;
	SViewPort vp;
	RVALUEPTR(vp, pVp);
	vp.a.SetZero();
	vp.b = GetDim();
	if(GetDim().x > 0 && GetDim().y > 0 && size.x > 0 && size.y > 0) {
		LMatrix2D mtx;
		vp.GetMatrix(FRect((float)size.x, (float)size.y), mtx);
		THROW(p_surf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, size.x, size.y));
		THROW(p_cr = cairo_create(p_surf));
		uint8 * p_src2 = cairo_image_surface_get_data(p_surf);
		cairo_transform(p_cr, (cairo_matrix_t *)&mtx);
		{
			cairo_surface_t * p_img_surf = (cairo_surface_t *)CreateSurface(dsysCairo);
			THROW(p_img_surf);
			cairo_set_source_surface(p_cr, p_img_surf, 0, 0);
			cairo_paint(p_cr);
			cairo_surface_destroy(p_img_surf);
			p_img_surf = 0;
			cairo_surface_flush(p_surf);
			{
				const uint w = (uint)size.x;
				const uint h = (uint)size.y;
				const uint stride = F.GetStride(w);
				assert(cairo_image_surface_get_width(p_surf) == w);
				assert(cairo_image_surface_get_height(p_surf) == h);
				assert(cairo_image_surface_get_stride(p_surf) == stride);
				Destroy();
				THROW(Init(w, h, F));
				uint8 * p_src = cairo_image_surface_get_data(p_surf);
				memcpy(P_Buf, p_src, h * stride);
			}
		}
	}
	CATCHZOK
	if(p_cr)
		cairo_destroy(p_cr);
	if(p_surf)
		cairo_surface_destroy(p_surf);
	return ok;
}

int SImageBuffer::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	THROW(pCtx->Serialize(dir, F.S, rBuf));
	THROW(pCtx->Serialize(dir, S, rBuf));
	{
		SBuffer temp_buf;
		SFile file;
		if(dir > 0) {
			StoreParam sp(SFileFormat::Png);
			THROW(file.Open(temp_buf, SFile::mWrite));
			THROW(StorePng(sp, file));
			THROW(rBuf.Write(*(SBuffer *)file));
		}
		else if(dir < 0) {
			THROW(rBuf.Read(temp_buf));
			THROW(file.Open(temp_buf, SFile::mRead));
			THROW(LoadPng(file));
		}
	}
	CATCHZOK
	return ok;
}

int FASTCALL SImageBuffer::Copy(const SImageBuffer & rS)
{
	S = rS.S;
	F = rS.F;
	return SBaseBuffer::Copy(rS);
}

int FASTCALL SImageBuffer::IsEqual(const SImageBuffer & rS) const
{
	if(F.S != rS.F.S)
		return 0;
	else if(S != rS.S)
		return 0;
	else if(Size != rS.Size)
		return 0;
	else if(Size && memcmp(P_Buf, rS.P_Buf, Size) != 0)
		return 0;
	return 1;
}

SImageBuffer::PixF SImageBuffer::GetFormat() const
	{ return F; }
uint SImageBuffer::GetWidth() const
	{ return (uint)S.x; }
uint SImageBuffer::GetHeight() const
	{ return (uint)S.y; }
TPoint SImageBuffer::GetDim() const
	{ return S; }
FPoint SImageBuffer::GetDimF() const
	{ return (FPoint)S; }
const  uint8 * SImageBuffer::GetData() const
	{ return (const uint8 *)P_Buf; }

int SImageBuffer::Store(const StoreParam & rP, SFile & rF)
{
	int    ok = 1;
	THROW(rF.IsValid());
	if(rP.Fmt == SFileFormat::Png) {
		THROW(StorePng(rP, rF));
	}
	else {
		CALLEXCEPT_S(SLERR_UNSUPPIMGFILEFORMAT);
	}
	CATCHZOK
	return ok;
}

int SImageBuffer::Helper_Load(SFile & rF, SFileFormat ff)
{
	int    ok = 1;
	THROW(rF.IsValid());
	switch(ff) {
		case SFileFormat::Jpeg:
			THROW(LoadJpeg(rF, ff));
			break;
		case SFileFormat::Bmp:
			//THROW(LoadJpeg(rF, ff));
			THROW(LoadBmp(rF));
			break;
		case SFileFormat::Png:
			THROW(LoadPng(rF));
			break;
		case SFileFormat::Gif:
			THROW(LoadGif(rF));
			break;
		case SFileFormat::Ico:
		case SFileFormat::Cur:
			THROW(LoadIco(rF, 0));
			break;
		default:
			CALLEXCEPT_S_S(SLERR_UNSUPPIMGFILEFORMAT, 0);
	}
	CATCHZOK
	return ok;
}

int SImageBuffer::Load(const char * pFileName)
{
	int    ok = 1;
	if(pFileName) {
		SFileFormat ff;
		THROW(fileExists(pFileName));
		const int fir = ff.Identify(pFileName);
		if(oneof3(fir, 1, 2, 3)) {
			THROW(IsSupportedFormat(ff));
			THROW(Helper_Load(SFile(pFileName, SFile::mRead|SFile::mBinary), ff));
		}
	}
	CATCHZOK
	return ok;
}

int SImageBuffer::Load(int fm, SBuffer & rInBuf)
{
	int    ok = 1;
	THROW(IsSupportedFormat(fm));
	THROW(Helper_Load(SFile(rInBuf, SFile::mRead|SFile::mBinary), SFileFormat(fm)));
	CATCHZOK
	return ok;
}

// static
int SImageBuffer::IsSupportedFormat(int fm)
{
	int    ok = 1;
	switch(fm) {
		case SFileFormat::Jpeg:
		case SFileFormat::Bmp:
		case SFileFormat::Png:
		case SFileFormat::Ico:
		case SFileFormat::Cur:
		case SFileFormat::Gif:
			break;
		default:
			ok = SLS.SetError(SLERR_UNSUPPIMGFILEFORMAT);
			break;
	}
	return ok;
}

int SImageBuffer::LoadMime_Base64(const char * pFormatStr, const SString & rS)
{
	int    ok = 1;
	SFileFormat ff;
	ff.IdentifyMime(pFormatStr);
	THROW(IsSupportedFormat(ff));
	{
		SBuffer bin_buf;
		size_t bin_size = 0;
		STempBuffer temp_buf(rS.Len() * 2);
		THROW(temp_buf.IsValid());
		THROW(rS.DecodeMime64(temp_buf, temp_buf.GetSize(), &bin_size));
		THROW(bin_buf.Write(temp_buf, bin_size));
		THROW(Load(ff, bin_buf));
	}
	CATCHZOK
	return ok;
}

int SImageBuffer::Init(uint w, uint h, SImageBuffer::PixF f)
{
	int    ok = 0;
	uint   stride = f.GetStride(w);
	if(stride && Alloc(stride * h)) {
		memzero(P_Buf, Size); // @v9.6.1
		F = f;
		S.Set((int)w, (int)h);
		ok = 1;
	}
	return ok;
}

uint8 * FASTCALL SImageBuffer::GetScanline(uint lineNo) const
{
	uint8 * p_ret = 0;
	if((int)lineNo < S.x) {
		uint   stride = F.GetStride(S.x);
		if(stride)
			p_ret = PTR8(P_Buf)+(stride*lineNo);
	}
	return p_ret;
}

int SImageBuffer::AddLines(const void * pSrc, SImageBuffer::PixF s, uint count, const SImageBuffer::Palette * pPalette)
{
	int    ok = 1;
	if(count) {
		uint   new_h = S.y + count;
		//assert(F == PixF::s32ARGB);
		THROW(S.x);
		const uint src_stride = s.GetStride(S.x);
		const uint stride = F.GetStride(S.x);
		THROW(stride);
		THROW(Alloc(stride * new_h));
		for(uint j = 0; j < count; j++) {
			THROW(s.GetUniform(PTR8(pSrc) + j * src_stride, PTR32(P_Buf)+((S.y+j)*S.x), S.x, pPalette));
		}
		S.y = new_h;
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

int SImageBuffer::Flip()
{
	const uint h = (uint)(S.y/2);
	const uint s = F.GetStride(S.x);
	for(uint i = 0; i < h; ++i)
		memswap(PTR8(P_Buf)+(i*s), PTR8(P_Buf)+((S.y-i-1)*s), s);
	return 1;
}

int SImageBuffer::TransformToGrayscale()
{
	uint32 * p = PTR32(P_Buf);
	if(p) {
		const size_t s = (size_t)(S.x * S.y);
		for(uint i = 0; i < s; ++i) {
			const uint32 gray = SImageBuffer::PixF::UniformToGrayscale(*p);
			*p = ((gray << 16) | (gray << 8) | gray) | (*p & 0xff000000);
			p++;
		}
	}
	return 1;
}

int SImageBuffer::PremultiplyAlpha()
{
	uint32 * p = PTR32(P_Buf);
	if(p) {
		const size_t s = (size_t)(S.x * S.y);
		for(uint i = 0; i < s; ++i) {
			uint32 c = *p;
			if((c & 0xff000000) != 0xff000000) {
				PREMULTIPLY_ALPHA_ARGB32(c);
				*p = c;
			}
			p++;
		}
	}
	return 1;
}

int SImageBuffer::GetSubImage(SImageBuffer & rDest, TPoint start, TPoint size) const
{
	int    ok = 1;
	TRect  rect;
	rect.setwidthrel(start.x, size.x);
	rect.setheightrel(start.y, size.y);
	TRect  bounds(S);
	TRect  ir; // inersection of bounds and rect
	rDest.Destroy();
	if(bounds.Intersect(rect, &ir) > 0) {
		rDest.Init(ir.width(), 0, F);
		const uint bpp = F.GetBpp();
		const uint s = F.GetStride(S.x);
		assert((bpp % 8) == 0);
		for(uint j = (uint)ir.a.y; j < (uint)ir.b.y; j++) {
			uint8 * p_scanline = GetScanline(j);
			THROW(p_scanline);
			THROW(rDest.AddLines(p_scanline + (bpp / 8) * ir.a.x, F, 1, 0));
		}
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}
//
//
//
#define BMP_SIGN 0x4D42

struct BmpFileHeader { // BITMAPFILEHEADER
	void   Init()
	{
		THISZERO();
		bfType = BMP_SIGN;
	}
	uint16 bfType;
	uint32 bfSize;
	uint16 bfReserved1;
	uint16 bfReserved2;
	uint32 bfOffBits;
};

struct BmpInfoHeader { // BITMAPINFOHEADER
	uint32 biSize;
	int32  biWidth;
	int32  biHeight;
	uint16 biPlanes;
	uint16 biBitCount;       //
	uint32 biCompression;
	uint32 biSizeImage;
	int32  biXPelsPerMeter;
	int32  biYPelsPerMeter;
	uint32 biClrUsed;
	uint32 biClrImportant;
};

int SImageBuffer::Helper_LoadBmp(SBuffer & rBuf, const char * pAddedErrorInfo)
{
	assert(sizeof(BmpFileHeader) == sizeof(BITMAPFILEHEADER));
	assert(sizeof(BmpInfoHeader) == sizeof(BITMAPINFOHEADER));

	int    ok = 1;
	uint   map_entry_size = 0;
	BmpFileHeader fh;
	BmpInfoHeader ih;
	SImageBuffer::PixF ff;

	Destroy();
	THROW(rBuf.Read(&fh, sizeof(fh)));
	THROW_S_S(fh.bfType == BMP_SIGN, SLERR_IMAGEFILENOTBMP, pAddedErrorInfo); // "424D"
	THROW(rBuf.Read(&ih, sizeof(ih.biSize)));
	THROW_S_S(oneof2(ih.biSize, 40, 64), SLERR_INVBMPHEADER, pAddedErrorInfo);
	THROW(rBuf.Read(PTR8(&ih)+sizeof(ih.biSize), sizeof(ih)-sizeof(ih.biSize)));
	THROW_S(ih.biCompression == 0, SLERR_BMPCOMPRNSUPPORTED);
	switch(ih.biBitCount) {
		case  0: break;
		case  1: map_entry_size = 4; ff.S = PixF::s1Idx; break;
		case  4: map_entry_size = 4; ff.S = PixF::s4Idx; break;
		case  8: map_entry_size = 4; ff.S = PixF::s8Idx; break;
		case 16: ff.S = PixF::s16RGB555; break;
		case 24: ff.S = PixF::s24RGB; break;
		case 32: ff.S = PixF::s32RGB; break;
		default: CALLEXCEPT_S_S(SLERR_INVBMPHEADER, pAddedErrorInfo); break;
	}
	{
		Palette palette;
		if(map_entry_size) {
			const  uint palette_count = NZOR(ih.biClrUsed, (1 << ih.biBitCount));
			const  uint palette_size = palette_count * sizeof(uint32);
			THROW_S_S(ih.biSize >= sizeof(ih), SLERR_INVBMPHEADER, pAddedErrorInfo);
			size_t map_offs = ih.biSize - sizeof(ih);
			if(map_offs > 0)
				rBuf.SetRdOffs(rBuf.GetRdOffs() + map_offs);
			THROW(palette.Alloc(palette_count));
			THROW(rBuf.Read(palette.GetBuffer(), palette_size));
			palette.SetAlpha(0xff);
		}
		THROW(Init(ih.biWidth, 0, PixF(PixF::s32ARGB)));
		{
			uint   height = (uint)labs(ih.biHeight);
			size_t line_size = ff.GetStride(ih.biWidth);
			STempBuffer line_buf(line_size * height);
			THROW(rBuf.Read(line_buf, line_buf.GetSize()));
			for(uint i = 0; i < height; ++i) {
				THROW(AddLines(PTR8((char *)line_buf)+line_size*i, ff, 1, &palette));
			}
			if(ih.biHeight > 0) {
				Flip();
			}
		}
	}
	CATCHZOK
	return ok;
}

int SImageBuffer::LoadBmp(SFile & rF)
{
	int    ok = 1;
	int64  fsize = 0;
	THROW(rF.IsValid());
	THROW(rF.CalcSize(&fsize));
	{
		SBuffer buffer;
		STempBuffer temp_buf(4096);
		while(fsize > 0) {
			size_t actual_size = 0;
			THROW(rF.Read(temp_buf, temp_buf.GetSize(), &actual_size));
			THROW(buffer.Write(temp_buf, actual_size));
			fsize -= actual_size;
		}
		THROW(Helper_LoadBmp(buffer, rF.GetName()));
	}
	CATCHZOK
	return ok;
}

int SImageBuffer::LoadBmp(HDC hDc, HBITMAP hBmp, uint subImgSqIdx, uint subImgSqSide)
{
	int    ok = 1;
	Destroy();
	if(hBmp) {
		SBuffer buffer;
		BITMAPINFO bi;
		SETIFZ(hDc, SLS.GetTLA().GetFontDC());
		MEMSZERO(bi);
		bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		::GetDIBits(hDc, hBmp, 0, 0, 0, &bi, DIB_RGB_COLORS);
		{
			BmpFileHeader fh;
			fh.Init();
			uint   width = (uint)labs(bi.bmiHeader.biWidth);
			uint   height = (uint)labs(bi.bmiHeader.biHeight);
			STempBuffer temp_buf(width * height * sizeof(uint32)); // Буфер для изображения (с запасом)
			bi.bmiHeader.biCompression = 0;
			::GetDIBits(hDc, hBmp, 0, height, temp_buf, &bi, DIB_RGB_COLORS);
			THROW(buffer.Write(&fh, sizeof(fh)));
			THROW(buffer.Write(&bi, bi.bmiHeader.biSize));
			THROW(buffer.Write(temp_buf, temp_buf.GetSize()));
			THROW(Helper_LoadBmp(buffer, ""));
			if(subImgSqIdx) {
				if(subImgSqSide && subImgSqSide < width && subImgSqSide < height) {
					TPoint sz, p;
					sz = (int)subImgSqSide;
					p = (int)0;
					for(uint i = 1; i < subImgSqIdx; i++) {
						p.x += sz.x;
						if(p.x >= (int)width) {
							p.y += sz.y;
							p.x = 0;
						}
					}
					SImageBuffer temp_img;
					ok = GetSubImage(temp_img, p, sz);
					this->Copy(temp_img);
				}
				else {
					Destroy();
					ok = -1;
				}
			}
		}
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

#if 0 // {

int SImageBuffer::LoadXpm(const char * pFileName)
{
}

#endif // } 0

struct IconHeader {
	uint16 idReserved;    // reserved
	uint16 idType;        // resource type (1 for icons, 2 for cursors)
	uint16 idCount;       // how many images?
};

struct IconDirEntry {
	uint8  bWidth;        // width of the image
	uint8  bHeight;       // height of the image (times 2)
	uint8  bColorCount;   // number of colors in image (0 if >=8bpp)
	uint8  bReserved;     // reserved
	union {
		uint16 wPlanes;   // color Planes
		uint16 XHotSpot;  // X location of cursor's hot spot
	};
	union {
		uint16 wBitCount; // bits per pixel
		uint16 YHotSpot;  // Y location of cursor's hot spot
	};
	uint32 dwBytesInRes;  // how many bytes in this resource?
	uint32 dwImageOffset; // where in the file is this image
};

struct IconImage {
	BITMAPINFOHEADER icHeader; // DIB header
	RGBQUAD icColors[1];       // Color table
	uint8  icXOR[1];           // DIB bits for XOR mask
	uint8  icAND[1];           // DIB bits for AND mask
};

int SImageBuffer::LoadIco(SFile & rF, uint pageIdx)
{
	int    ok = 1;
	size_t total_rc_size = 0;
	IconHeader hdr;
	IconDirEntry * p_idir = 0;
	THROW(rF.IsValid());
	THROW(rF.ReadV(&hdr, sizeof(hdr)));
	THROW_S_S(hdr.idReserved == 0 && oneof2(hdr.idType, 1, 2) && (hdr.idCount >= 0 && hdr.idCount < 1000), SLERR_IMAGEFILENOTICO, rF.GetName());
	if(pageIdx < hdr.idCount) {
		BITMAPINFOHEADER bm_hdr;
		size_t dir_entry_size = sizeof(IconDirEntry) * hdr.idCount;
		THROW(p_idir = (IconDirEntry *)SAlloc::M(dir_entry_size));
		THROW(rF.ReadV(p_idir, dir_entry_size));
		THROW(rF.Seek(p_idir[pageIdx].dwImageOffset));
		THROW(rF.ReadV(&bm_hdr, sizeof(bm_hdr)));
		total_rc_size += sizeof(bm_hdr);
		{
			const uint bit_count = bm_hdr.biBitCount;
			const int  revert = (bm_hdr.biWidth > 0) ? 1 : 0;
			const uint width  = abs(bm_hdr.biWidth);
			const uint height = abs(bm_hdr.biHeight/2);   // height == xor + and mask
			const uint line_size = (((width*bit_count)+7)/8 + 3) & ~3;
			STempBuffer line_buf(line_size * height);
			SImageBuffer::PixF ff;
			THROW(Init(width, 0, PixF(PixF::s32ARGB)));
			switch(bit_count) {
				case 1:
				case 2:
				case 4:
				case 8:
				case 16:
					{
						switch(bit_count) {
							case 1: ff = PixF::s1Idx; break;
							case 2: ff = PixF::s2Idx; break;
							case 4: ff = PixF::s4Idx; break;
							case 8: ff = PixF::s8Idx; break;
							case 16: ff = PixF::s16Idx; break;
						}
						SImageBuffer::Palette palette;
						const size_t palette_size = (1 << bit_count) * sizeof(uint32);
						THROW(palette.Alloc(palette_size));
						THROW(rF.ReadV(palette.GetBuffer(), palette_size));
						total_rc_size += palette_size;
						THROW(rF.ReadV(line_buf, line_size * height));
						total_rc_size += (line_size * height);
						for(uint i = 0; i < height; ++i)
							THROW(AddLines(PTR8((char *)line_buf)+line_size*i, ff, 1, &palette));
					}
					break;
				case 24:
				case 32:
					{
						switch(bit_count) {
							case 24: ff = PixF::s24RGB; break;
							case 32: ff = PixF::s32ARGB; break;
						}
						THROW(rF.ReadV(line_buf, line_size * height));
						total_rc_size += (line_size * height);
						for(uint i = 0; i < height; ++i)
							THROW(AddLines(PTR8((char *)line_buf)+line_size*i, ff, 1, 0));
					}
				default:
					ok = 0;
			}
			{
				const size_t width_and = (((width)+31)>>5)<<2;
				uint32 * p_data = (uint32 *)GetData();
				const uint _s = GetWidth() * GetHeight();
				THROW(rF.ReadV(line_buf, width_and * height));
				total_rc_size += (width_and * height);
				for(uint i = 0; i < height; ++i) {
					const size_t step = i*width;
					const size_t step_and = i*width_and;
					for(uint j = 0; j < width; ++j) {
						int a = (PTR8((char *)line_buf)[step_and + (j>>3)]&(0x80>>(j&0x07))) != 0 ? 0 : 0xFF;
						if(a == 0)
							p_data[step+j] &= 0x00ffffff;
						else
							p_data[step+j] |= 0xff000000;
					}
				}
				assert(total_rc_size == p_idir[pageIdx].dwBytesInRes);
			}
			if(revert)
				Flip();
		}
	}
	else
		ok = -1;
	CATCHZOK
	SAlloc::F(p_idir);
	return ok;
}

//#define USE_JPEG_TURBO

#ifdef USE_JPEG_TURBO
	#include <../osf/libjpeg-turbo/cdjpeg.h>
	#include <../osf/libjpeg-turbo/jerror.h>
#else
	#include <../osf/libjpeg/cdjpeg.h>
	#include <../osf/libjpeg/jerror.h>
#endif

int SImageBuffer::LoadJpeg(SFile & rF, int fileFmt)
{
	struct JpegErr {
		static void ExitFunc(j_common_ptr pCInfo)
		{
			int    err_code = 0;
			JpegErr * p_err = (JpegErr *)pCInfo->err;
			// Are there any other error codes we might care about?
			switch(p_err->pub.msg_code) {
	    		case JERR_NO_SOI: err_code = SLERR_IMAGEFILENOTJPEG; break;
	    		default: err_code = SLERR_JPEGLOADFAULT; break;
			}
			longjmp(p_err->setjmp_buf, err_code);
		}
		JpegErr()
		{
			pub.error_exit = ExitFunc;
		}
		struct jpeg_error_mgr pub; // "public" fields
		jmp_buf setjmp_buf;        // for return to caller
	};
	int    ok = 1;
	int    err_code = 0;
	JpegErr jpeg_err;
	uint8 * p_row_buf = 0;
	cjpeg_source_ptr p_src_mgr = 0;
	if(fileFmt == SFileFormat::Jpeg) {
		struct jpeg_decompress_struct di;
		di.err = jpeg_std_error(&jpeg_err.pub);
		jpeg_err.pub.error_exit = JpegErr::ExitFunc; // @v9.6.3
		err_code = setjmp(jpeg_err.setjmp_buf);
		if(err_code) {
			SLS.SetAddedMsgString(rF.GetName());
			CALLEXCEPT();
		}
		else {
			jpeg_create_decompress(&di);
			if(((FILE *)rF) != 0)
				jpeg_stdio_src(&di, (FILE *)rF);
			else {
				SBaseBuffer buf;
				if(rF.GetBuffer(buf)) {
					jpeg_mem_src(&di, (uchar *)buf.P_Buf, buf.Size);
				}
			}
			jpeg_read_header(&di, TRUE);
			jpeg_start_decompress(&di);
			const uint out_width = di.output_width;
			if(Init(out_width, 0)) {
				PixF fmt;
				if(di.output_components == 1)
					fmt = PixF::s8GrayScale;
				else if(di.output_components == 3)
					fmt = PixF::s24RGB;
				else if(di.output_components == 4)
					fmt = PixF::s32ARGB;
				{
					const uint max_lines = 1;
					const size_t line_size = di.output_width * di.output_components;
					p_row_buf = (uint8 *)SAlloc::M(line_size * max_lines);
					if(p_row_buf) {
						// @v9.5.6 {
						{
							const uint _stride = F.GetStride(S.x);
							THROW(_stride);
							THROW(Alloc(_stride * di.output_height));
						}
						// } @v9.5.6 
						while(ok && di.output_scanline < di.output_height) {
							JSAMPROW row_ptr[max_lines];
							uint    step_count = 1;
							if(max_lines == 1) {
								row_ptr[0] = p_row_buf;
							}
							else {
								step_count = MIN(max_lines, (di.output_height - di.output_scanline));
								for(uint ln = 0; ln < step_count; ln++) {
									row_ptr[ln] = p_row_buf + line_size * ln;
								}
							}
							const uint lines_done = jpeg_read_scanlines(&di, row_ptr, step_count);
							ok = AddLines(p_row_buf, fmt, lines_done, 0);
						}
					}
					else
						ok = 0;
				}
			}
			else
				ok = 0;
			jpeg_finish_decompress(&di);
			jpeg_destroy_decompress(&di);
		}
	}
	CATCHZOK
	SAlloc::F(p_row_buf);
	return ok;
}
//
//
//
#include <libpng/png.h>

struct PngSupport {
	static void LoadErrFunc(png_structp pPng, const char * pMsg)
	{
		int    err_code = 0;
		if(pPng) {
			SLS.SetError(err_code = SLERR_PNGLOADFAULT, pMsg);
			png_longjmp(pPng, err_code);
		}
	}
	static void StoreErrFunc(png_structp pPng, const char * pMsg)
	{
		int    err_code = 0;
		if(pPng) {
			SLS.SetError(err_code = SLERR_PNGSTOREFAULT, pMsg);
			png_longjmp(pPng, err_code);
		}
	}
	static void PNGAPI ReadFunc(png_structp pPng, png_bytep pData, size_t length)
	{
		SFile * p_file = (SFile *)png_get_io_ptr(pPng);
		if(!p_file || !p_file->ReadV(pData, length))
			png_error(pPng, 0);
	}
	static void PNGAPI WriteFunc(png_structp pPng, png_bytep pData, size_t length)
	{
		SFile * p_file = (SFile *)png_get_io_ptr(pPng);
		if(!p_file || !p_file->Write(pData, length))
			png_error(pPng, 0);
	}
	static void PNGAPI FlushFunc(png_structp pPng)
	{
		SFile * p_file = (SFile *)png_get_io_ptr(pPng);
		if(!p_file || !p_file->Flush())
			png_error(pPng, 0);
	}
	static void UnpremultiplyDataFunc(png_structp png, png_row_infop row_info, png_bytep data)
	{
		for(uint i = 0; i < row_info->rowbytes; i += 4) {
			uint8 * b = &data[i];
			const uint32 pix = *PTR32(b);
			const uint8  alpha = (uint8)((pix&0xff000000)>>24);
			if(alpha == 0) {
				PTR32(b)[0] = 0;
			}
			else {
				const uint8 a2 = alpha/2;
				b[0] = (uint8)((((pix&0xff0000)>>16)*255+a2)/alpha);
				b[1] = (uint8)((((pix&0x00ff00)>>8)*255+a2)/alpha);
				b[2] = (uint8)(((pix&0x0000ff)*255+a2)/alpha);
				b[3] = alpha;
			}
		}
	}
	PngSupport()
	{
	}
};

int SImageBuffer::LoadPng(SFile & rF)
{
	int    ok = 1;
	int    err_code = 0;
	png_struct * p_png = 0;
	png_info * p_info = 0;
	uint8 * p_row_buf = 0;
	size_t sig_bytes = 0;
	{
		uchar  png_sig[8];
		assert(sizeof(png_sig)==8);
		THROW(rF.Read(png_sig, sizeof(png_sig), &sig_bytes));
		THROW_S_S(sig_bytes == sizeof(png_sig) && png_sig_cmp(png_sig, 0, sig_bytes) == 0, SLERR_IMAGEFILENOTPNG, rF.GetName());
	}
	{
		p_png = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, (png_error_ptr)PngSupport::LoadErrFunc, 0);
		THROW_S_S(p_png, SLERR_PNGLOADFAULT, rF.GetName()); // ?nomem
		// png_longjmp png_setjmp

		err_code = setjmp(png_jmpbuf(p_png));
		THROW(!err_code);
		THROW_S_S(p_info = png_create_info_struct(p_png), SLERR_PNGLOADFAULT, rF.GetName()); // ?nomem
		png_set_read_fn(p_png, &rF, PngSupport::ReadFunc);
		png_set_sig_bytes(p_png, (int)sig_bytes);
		png_read_info(p_png, p_info);
		//
		//
		//
		const uint32 width  = png_get_image_width(p_png, p_info);
		const uint32 height = png_get_image_height(p_png, p_info);
		const int    bit_depth = png_get_bit_depth(p_png, p_info);
		const int    color_type = png_get_color_type(p_png, p_info);
		const int    interlace  = png_get_interlace_type(p_png, p_info);
		png_set_expand(p_png);
		//
		// transform transparency to alpha
		//
		if(png_get_valid(p_png, p_info, PNG_INFO_tRNS))
			png_set_tRNS_to_alpha(p_png);
		//
		if(bit_depth == 16)
			png_set_strip_16(p_png);
		if(bit_depth < 8)
			png_set_packing(p_png);
		//
		// convert grayscale to RGB
		//
		if(color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
			png_set_gray_to_rgb(p_png);
		if(interlace != PNG_INTERLACE_NONE)
			png_set_interlace_handling(p_png);
		png_set_bgr(p_png);
		png_set_filler(p_png, 0xff, PNG_FILLER_AFTER);
		png_read_update_info(p_png, p_info);
		THROW(Init(width, 0));
		THROW(p_row_buf = (uint8 *)SAlloc::M(width*sizeof(uint32)));
		{
			PixF fmt = PixF::s32ARGB;
			int  pass_count = png_set_interlace_handling(p_png);
			int  i = pass_count;
			if(i > 0) {
				uint j;
				// @v9.5.6 {
				{
					const uint _stride = F.GetStride(S.x);
					THROW(_stride);
					THROW(Alloc(_stride * height));
				}
				// } @v9.5.6 
				for(j = 0; j < height; j++) {
					png_read_rows(p_png, &p_row_buf, 0, 1);
					THROW(AddLines(p_row_buf, fmt, 1, 0));
				}
				while(--i) {
					for(j = 0; j < height; j++) {
						uint8 * p_scanline = GetScanline(j);
						if(p_scanline)
							png_read_rows(p_png, &p_scanline, 0, 1);
					}
				}
			}
			PremultiplyAlpha();
		}
		png_read_end(p_png, p_info);
	}
	CATCHZOK
	if(p_png)
		png_destroy_read_struct(&p_png, p_info ? &p_info: 0, 0);
	SAlloc::F(p_row_buf);
	return ok;
}

int SImageBuffer::StorePng(const StoreParam & rP, SFile & rF)
{
	int    ok = 1;
	int    err_code = 0;
	png_struct * p_png = 0;
	png_info * p_info = 0;
	uint8 ** volatile pp_rows = 0;
	const uint stride = F.GetStride(S.x);
	THROW(S.x && S.y); // no image
	THROW(pp_rows = (uint8 **)SAlloc::M(S.y * sizeof(uint8*)));
	for(int i = 0; i < S.y; i++) {
		pp_rows[i] = (uint8 *)P_Buf+i*stride;
	}
	THROW(p_png = png_create_write_struct(PNG_LIBPNG_VER_STRING, 0, PngSupport::StoreErrFunc, 0));
	err_code = setjmp(png_jmpbuf(p_png));
	if(err_code) {
		CALLEXCEPT();
	}
	THROW(p_info = png_create_info_struct(p_png));
	png_set_write_fn(p_png, &rF, PngSupport::WriteFunc, PngSupport::FlushFunc);
	{
		int depth = 8;
		{
			int color_type = PNG_COLOR_TYPE_RGB_ALPHA;
			int interlace = (rP.Flags & StoreParam::fInterlaced) ? PNG_INTERLACE_ADAM7 : PNG_INTERLACE_NONE;
			png_set_IHDR(p_png, p_info, S.x, S.y, depth, color_type, interlace, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
		}
		{
			png_color_16 white;
			white.gray = (1<<depth)-1;
			white.red = white.blue = white.green = white.gray;
			png_set_bKGD(p_png, p_info, &white);
			/*
				We have to call png_write_info() before setting up the write
	 			transformation, since it stores data internally in 'png'
	 			that is needed for the write transformation functions to work.
	 		*/
			png_write_info(p_png, p_info);
			png_set_write_user_transform_fn(p_png, PngSupport::UnpremultiplyDataFunc);
			png_write_image(p_png, pp_rows);
			png_write_end(p_png, p_info);
		}
	}
	CATCHZOK
	png_destroy_write_struct(&p_png, &p_info);
	SAlloc::F(pp_rows);
	return ok;
}
//
//
//
//#include <..\osf\giflib\gif_lib.h>
#include <gif_lib.h>

static int GifReadFunc(GifFileType * pF, GifByteType * pData, int size)
{
	size_t actual_size = 0;
	SFile * p_f = (SFile *)pF->UserData;
	if(p_f && !p_f->Read(pData, (size_t)size, &actual_size))
		actual_size = 0;
	return actual_size;
}

int SImageBuffer::LoadGif(SFile & rF)
{
    static const int8 InterlacedOffset[] = { 0, 4, 2, 1 }; // The way Interlaced image should.
	static const int8 InterlacedJumps[] = { 8, 8, 4, 2 };  // be read - offsets and jumps...

	int    ok = 1;
	int    gif_err_code = 0;
	int    i;
	int    ext_code;
	int    width = 0;
	int    height = 0;
	int    row = 0;
	int    col = 0;
	int    count = 0;
	GifRecordType rec_type;
	GifRowType * p_buffer = 0;
	ColorMapObject * p_color_map = 0;
	GifFileType * p_gf = DGifOpen(&rF, GifReadFunc, &gif_err_code);
	THROW(p_gf);
	{
		//
		// Allocate the screen as vector of column of rows. Note this
		// screen is device independent - it's the screen defined by the
		// GIF file parameters.
		//
		size_t size = 0;
		THROW(p_buffer = (GifRowType *)SAlloc::M(p_gf->SHeight * sizeof(GifRowType)));
		size = p_gf->SWidth * sizeof(GifPixelType); // Size in bytes one row
		for(i = 0; i < p_gf->SHeight; i++) {
			// Allocate rows, and set their color to background
			THROW(p_buffer[i] = (GifRowType)SAlloc::M(size));
			memset(p_buffer[i], p_gf->SBackGroundColor, size); // Set its color to BackGround
		}
	}
	do {
		THROW(DGifGetRecordType(p_gf, &rec_type));
		switch(rec_type) {
			case IMAGE_DESC_RECORD_TYPE:
				THROW(DGifGetImageDesc(p_gf));
				row = p_gf->Image.Top; /* Image Position relative to Screen. */
				col = p_gf->Image.Left;
				width = p_gf->Image.Width;
				height = p_gf->Image.Height;
				//GifQprintf("\n%s: Image %d at (%d, %d) [%dx%d]:     ", PROGRAM_NAME, ++ImageNum, Col, Row, Width, Height);
				THROW((p_gf->Image.Left + p_gf->Image.Width) <= p_gf->SWidth && (p_gf->Image.Top + p_gf->Image.Height) <= p_gf->SHeight);
					//fprintf(stderr, "Image %d is not confined to screen dimension, aborted.\n",ImageNum);
				if(p_gf->Image.Interlace) {
					// Need to perform 4 passes on the images:
					for(count = i = 0; i < 4; i++)
						for(int j = row + InterlacedOffset[i]; j < row + height; j += InterlacedJumps[i]) {
							THROW(DGifGetLine(p_gf, &p_buffer[j][col], width));
						}
				}
				else {
					for(i = 0; i < height; i++) {
						THROW(DGifGetLine(p_gf, &p_buffer[row++][col], width));
					}
				}
				break;
			case EXTENSION_RECORD_TYPE:
				// Skip any p_extension blocks in file:
				{
					GifByteType * p_extension = 0;
					THROW(DGifGetExtension(p_gf, &ext_code, &p_extension));
					while(p_extension) {
						THROW(DGifGetExtensionNext(p_gf, &p_extension));
					}
				}
				break;
			case TERMINATE_RECORD_TYPE:
				break;
			default: // Should be trapped by DGifGetRecordType.
				break;
		}
	} while(rec_type != TERMINATE_RECORD_TYPE);
	{
		p_color_map = NZOR(p_gf->Image.ColorMap, p_gf->SColorMap);
		PixF fmt(PixF::s8Idx);
		Palette palette;
		palette.Alloc(256);
		THROW(Init(p_gf->SWidth, 0));
		for(i = 0; i < 256; i++) {
            const SColorRGB & r_entry = p_color_map->Colors[i];
			palette.SetRGB(i, r_entry.R, r_entry.G, r_entry.B);
		}
		// @v9.6.11 {
		{
			const uint _stride = F.GetStride(S.x);
			THROW(_stride);
			THROW(Alloc(_stride * p_gf->SHeight));
		}
		// } @v9.6.11
		for(i = 0; i < p_gf->SHeight; i++)
			THROW(AddLines(p_buffer[i], fmt, 1, &palette));
	}
	CATCHZOK
	DGifCloseFile(p_gf);
	return ok;
}
//
//
//
SDrawImage::SDrawImage(const char * pSid) : SDrawFigure(SDrawFigure::kImage, pSid)
{
	Buf.Init(0, 0);
}

SDrawImage::SDrawImage(SImageBuffer & rBuf, const char * pSid) : SDrawFigure(SDrawFigure::kImage, pSid)
{
	Buf.Init(0, 0);
	SetBuffer(&rBuf);
}

SDrawImage::~SDrawImage()
{
	Buf.Destroy();
}

SDrawFigure * SDrawImage::Dup() const { return DupDrawFigure <SDrawImage> (this); }

int SDrawImage::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	THROW(SDrawFigure::Serialize(dir, rBuf, pCtx));
	THROW(Buf.Serialize(dir, rBuf, pCtx));
	CATCHZOK
	return ok;
}

int FASTCALL SDrawImage::Copy(const SDrawImage & rS)
{
	int    ok = 1;
	THROW(SDrawFigure::Copy(rS));
	THROW(Buf.Copy(rS.Buf));
	CATCHZOK
	return ok;
}

int SDrawImage::SetBuffer(const SImageBuffer * pBuf)
{
	int    ok = -1;
	if(pBuf) {
		Buf.Copy(*pBuf);
		Size = Buf.GetDimF();
		ok = 1;
	}
	return ok;
}

const SImageBuffer & SDrawImage::GetBuffer() const
{
	return Buf;
}

int SDrawImage::LoadFile(const char * pFileName)
{
	int    ok = Buf.Load(pFileName);
	Size = Buf.GetDimF();
	return ok;
}

int SDrawImage::LoadBuf(int fm, SBuffer & rInBuf)
{
	int    ok = Buf.Load(fm, rInBuf);
	Size = Buf.GetDimF();
	return ok;
}

int SDrawImage::LoadMime_Base64(const char * pFormatStr, const SString & rS)
{
	int    ok = Buf.LoadMime_Base64(pFormatStr, rS);
	Size = Buf.GetDimF();
	return ok;
}

int SDrawImage::Store(SImageBuffer::StoreParam & rP, SFile & rF)
{
	return Buf.Store(rP, rF);
}

int SDrawImage::TransformToBounds(TPoint size, const SViewPort * pVp)
{
	int    ok = Buf.TransformToBounds(size, pVp);
	Size = Buf.GetDimF();
	return ok;
}

int SDrawImage::TransformToGrayscale()
{
	return Buf.TransformToGrayscale();
}
//
//
//
#if SLTEST_RUNNING // {

SLTEST_R(SDraw)
{
	int    ok = 1;
	SImageBuffer img_buf;
	THROW(SLTEST_CHECK_NZ(img_buf.Load(MakeInputFilePath("test24.png"))));
	{
		SBuffer buf;
		SFile out_file(buf, SFile::mWrite);
		SImageBuffer::StoreParam sp(SFileFormat::Png);
		THROW(SLTEST_CHECK_NZ(img_buf.Store(sp, out_file)));
		{
			SImageBuffer img_buf2;
			SBuffer * p_buf = out_file;
			THROW(SLTEST_CHECK_NZ(p_buf));
			THROW(SLTEST_CHECK_NZ(img_buf2.Load(SFileFormat::Png, *p_buf)));
			THROW(SLTEST_CHECK_NZ(img_buf2.IsEqual(img_buf)));
		}
	}
	CATCHZOK
	return ok;
}

#endif // } SLTEST_RUNNING
