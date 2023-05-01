// SDRAW.CPP
// Copyright (c) A.Sobolev 2010, 2011, 2012, 2013, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022
// @codepage UTF-8
//
#include <slib-internal.h>
#pragma hdrstop
//#include <setjmp.h> // jpeg && png
//
#define CAIRO_WIN32_STATIC_BUILD 1
#include <cairo-1160\cairo.h>
#include <cairo-1160\cairo-win32.h>

#define DEFAULT_UCTX_FONTSIZE 10
//
//
//
SDrawContext::UC::UC() : Dpi(0.0f), FontSize(0.0f)
{
}

int SDrawContext::UC::Describe(int unitId, int dir, int * pCls, double * pToBase, SString * pName) const
{
	int    cls = 0;
	double to_base = 0.0;
	const  char * p_name = 0;
	if(unitId == UNIT_GR_PIXEL) {
		to_base = (dir == DIREC_VERT) ? fdivnz(2.54e-2, Dpi.y) : fdivnz(2.54e-2, Dpi.x);
		cls = SUnit::clsLength;
		p_name = "pixel";
	}
	else if(unitId == UNIT_GR_PT) {
		to_base = 2.54e-2 / 72.0;
		cls = SUnit::clsLength;
		p_name = "pt";
	}
	else if(unitId == UNIT_GR_EM) {
		to_base = (dir == DIREC_VERT) ? fdivnz(FontSize * 2.54e-2, Dpi.y) : fdivnz(FontSize * 2.54e-2, Dpi.x);
		cls = SUnit::clsLength;
		p_name = "em";
	}
	else if(unitId == UNIT_GR_EX) {
		to_base = (dir == DIREC_VERT) ? fdivnz((FontSize / 2.0) * 2.54e-2, Dpi.y) : fdivnz((FontSize / 2.0)  * 2.54e-2, Dpi.x);
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

/*static*/uint SDrawContext::CalcScreenFontSizePt(uint pt)
{
	uint   s = 0;
	SDrawContext dctx(static_cast<cairo_t *>(0));
	SDrawContext::UC * p_uc = dctx.GetUnitContext();
	if(p_uc) {
		USize pt_size;
		USize px_size;
		SUnit::Convert(pt_size.Set(pt, UNIT_GR_PT, DIREC_VERT), px_size.Set(1, UNIT_GR_PIXEL, DIREC_VERT), p_uc);
		s = fceili(px_size);
		delete p_uc;
	}
	return s;
}

SDrawContext::SDrawContext(cairo_t * pCr) : S(dsysCairo), P(pCr)
{
}

SDrawContext::SDrawContext(HDC hDc) : S(dsysWinGdi), P(static_cast<void *>(hDc))
{
}

SDrawContext::operator cairo_t * () const
{
	return (S == dsysCairo) ? static_cast<cairo_t *>(P) : 0;
}

SDrawContext::UC * SDrawContext::GetUnitContext() const
{
	SDrawContext::UC * p_uc = 0;
	if(oneof2(S, dsysWinGdi, dsysCairo)) {
		p_uc = new SDrawContext::UC;
		if(p_uc) {
			HDC    h_dc = (S == dsysWinGdi && P) ? static_cast<HDC>(P) : SLS.GetTLA().GetFontDC();
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
SViewPort::SViewPort(uint flags) : FRect(), ParX(parMid), ParY(parMid), Flags(fEmpty|flags)
{
}

LMatrix2D & SViewPort::GetMatrix(const FRect & rBounds, LMatrix2D & rMtx) const
{
	const double w0 = Width();
	const double h0 = Height();
	double w_c = (Flags & fDontScale) ? w0 : rBounds.Width();
	double h_c = (Flags & fDontScale) ? h0 : rBounds.Height();
	const double r0 = h0 / w0;
	const double r = h_c / w_c;
	SPoint2F diff = rBounds.a - a;
	LMatrix2D transl_mtx;
	if((ParX == parNone && ParY == parNone) || r == r0) {
		if(Flags & fDontEnlarge) {
			if(ParX == parMax)
				diff = diff.AddX(static_cast<float>(w_c - w0));
			else if(oneof2(ParX, parMid, parNone))
				diff = diff.AddX(static_cast<float>((w_c - w0)/2.0));
			if(ParY == parMax)
				diff = diff.AddY(static_cast<float>(h_c - h0));
			else if(oneof2(ParY, parMid, parNone))
				diff = diff.AddY(static_cast<float>((h_c - h0)/2.0));
			h_c = h0;
			w_c = w0;
		}
		rMtx.InitScale(w_c/w0, h_c/h0);
	}
	else {
		if((r > r0 && Flags & fSlice) || (r < r0 && !(Flags & fSlice))) {
			if(Flags & fDontEnlarge && h_c > h0) {
				if(ParY == parMax)
					diff = diff.AddY(static_cast<float>(h_c - h0));
				else if(oneof2(ParY, parMid, parNone))
					diff = diff.AddY(static_cast<float>((h_c - h0)/2.0));
				h_c = h0;
			}
			rMtx.InitScale(h_c/h0, h_c/h0);
			if(ParX == parMax)
				diff = diff.AddX(static_cast<float>(w_c - h_c / r0));
			else if(oneof2(ParX, parMid, parNone))
				diff = diff.AddX(static_cast<float>((w_c - h_c / r0)/2.0));
		}
		else {
			if(Flags & fDontEnlarge && w_c > w0) {
				if(ParX == parMax)
					diff = diff.AddX(static_cast<float>(w_c - w0));
				else if(oneof2(ParX, parMid, parNone))
					diff = diff.AddX(static_cast<float>((w_c - w0)/2.0));
				w_c = w0;
			}
			rMtx.InitScale(w_c/w0, w_c/w0);
			if(ParY == parMax)
				diff = diff.AddY(static_cast<float>(h_c - w_c * r0));
			else if(oneof2(ParY, parMid, parNone))
				diff = diff.AddY(static_cast<float>((h_c - w_c * r0)/2.0));
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
		a.x = temp_buf.ToFloat();
		SETMAX(a.x, 0.0f); // @v10.8.1 я не знаю как правильно разрулить отрицательную координату - потому просто сдвигаю ее в ноль
		scan.SkipOptionalDiv(',');
		THROW(scan.GetNumber(temp_buf));
		a.y = temp_buf.ToFloat();
		SETMAX(a.y, 0.0f); // @v10.8.1 я не знаю как правильно разрулить отрицательную координату - потому просто сдвигаю ее в ноль
		scan.SkipOptionalDiv(',');
		THROW(scan.GetNumber(temp_buf));
		b.x = a.x + temp_buf.ToFloat();
		scan.SkipOptionalDiv(',');
		THROW(scan.GetNumber(temp_buf));
		b.y = a.y + temp_buf.ToFloat();
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
		p_dup->Copy(static_cast<const F &>(*pThis));
	else
		SLS.SetError(SLERR_NOMEM);
	return p_dup;
}

/*static*/int FASTCALL SDrawFigure::CheckKind(int kind)
{
	if(oneof6(kind, kShape, kPath, kGroup, kImage, kText, kRef)) // @v10.4.5 kRef
		return 1;
	else {
		SString msg_buf;
		return SLS.SetError(SLERR_INVSDRAWFIGKIND, msg_buf.Cat(kind));
	}
}

/*static*/SDrawFigure * SDrawFigure::CreateFromFile(const char * pFileName, const char * pSid)
{
	SDrawFigure * p_fig = 0;
	SFileFormat fmt;
	THROW(fileExists(pFileName));
	{
		const int fir = fmt.Identify(pFileName);
		THROW(fir);
		if(fmt == SFileFormat::Svg) {
			SDrawContext dctx(static_cast<HDC>(0));
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
	}
	CATCH
		ZDELETE(p_fig);
	ENDCATCH
	return p_fig;
}

SDrawFigure::SDrawFigure(int kind, const char * pSid) : Kind(kind), IdPen(0), IdBrush(0), Flags(0), Sid(pSid), P_Parent(0)
{
	assert(CheckKind(kind));
}

SDrawFigure::~SDrawFigure()
{
}

/*static*/SDrawFigure * SDrawFigure::Unserialize(SBuffer & rBuf, SSerializeContext * pCtx)
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
			p_instance = (flags & SDrawFigure::fDraw) ? new SDraw : new SDrawGroup(0, (flags & SDrawFigure::fSymbolGroup) ? SDrawGroup::dgtSymbol : SDrawGroup::dgtOrdinary);
			break;
		case kRef: p_instance = new SDrawRef; break; // @v10.4.5
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

void SDrawFigure::SetTransform(const LMatrix2D * pMtx)
{
	if(!RVALUEPTR(Tf, pMtx))
		Tf.InitUnit();
}

void SDrawFigure::SetViewPort(const SViewPort * pVp)
{
	if(!RVALUEPTR(Vp, pVp)) {
		Vp.a = 0.0f;
		Vp.b = 0.0f;
		Vp.Flags |= SViewPort::fEmpty;
	}
}

void SDrawFigure::SetSize(SPoint2F sz)
{
	if(sz.x || sz.y) {
		Size = sz;
		Flags |= fDefinedSize;
	}
	else {
		Size.SetZero();
		Flags &= ~fDefinedSize;
	}
}

int FASTCALL SDrawFigure::GetViewPort(SViewPort * pVp) const
{
	int    ok = -1;
	SViewPort vp = Vp;
	vp.Flags &= ~SViewPort::fEmpty;
	if(vp.Width() > 0.0f && vp.Height() > 0.0f) {
		ok = 1;
	}
	else if(Size.IsPositive()) {
		*static_cast<FRect *>(&vp) = Size;
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

int SDrawFigure::TransformToImage(const SViewPort * pVp, SImageBuffer & rImg) const
{
	int    ok = 1;
	SPoint2F size = rImg.GetDimF();
	if(size.x > 0 && size.y > 0) {
		LMatrix2D mtx;
		SPaintToolBox tb;
		SPaintToolBox * p_tb = GetToolBox();
		TCanvas2 canv(*NZOR(p_tb, &tb), rImg);
		SViewPort vp;
		if(!RVALUEPTR(vp, pVp))
			GetViewPort(&vp);
		FRect rc(size.x, size.y);
		canv.SetTransform(vp.GetMatrix(rc, mtx));
		canv.Rect(rc);
		canv.Fill(SColor(255, 255, 255, 0), 0); // Прозрачный фон
		canv.Draw(this);
	}
	else
		ok = -1;
	return ok;
}

SDrawImage * SDrawFigure::DupToImage(SPoint2S size, const SViewPort * pVp, const char * pSid)
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

const SDrawFigure * SDrawFigure::SearchRef(const char * pSid) const
{
	const SDrawFigure * p_result = 0;
	if(Sid.IsEqiAscii(pSid))
		p_result = this;
	else {
		const SDrawFigure * p_root = 0;
		for(const SDrawFigure * p_par = P_Parent; p_par; p_par = p_par->P_Parent) {
			p_root = p_par;
		}
		SETIFZ(p_root, this);
		assert(p_root);
		if(p_root) {
			if(p_root->Kind == kGroup && !(p_root->Flags & fSymbolGroup)) {
				const SDrawGroup * p_group = static_cast<const SDrawGroup *>(p_root);
				p_result = p_group->Find(pSid, 1);
			}
		}
	}
	return p_result;
}

void SDrawFigure::SetStyle(int identPen, int identBrush, long flags)
{
	IdPen = identPen;
	IdBrush = identBrush;
	if(flags & fNullBrush)
		Flags |= fNullBrush;
}

int    SDrawFigure::GetKind() const { return Kind; }
long   SDrawFigure::GetFlags() const { return Flags; }
const  SString & SDrawFigure::GetSid() const { return Sid; }
const  SPoint2F & SDrawFigure::GetSize() const { return Size; }
const  SViewPort & SDrawFigure::GetViewPort() const { return Vp; }
void   SDrawFigure::SetSid(const char * pSid) { Sid = pSid; }
int    SDrawFigure::GetPen() const { return IdPen; }
int    SDrawFigure::GetBrush() const { return IdBrush; }
const  LMatrix2D & SDrawFigure::GetTransform() const { return Tf; }

SPaintToolBox * SDrawFigure::GetToolBox() const
{
	return (Flags & fDraw) ? static_cast<const SDraw *>(this)->P_Tb : (P_Parent ? P_Parent->GetToolBox() : 0); // @recursion
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
SDrawRef::SDrawRef(const char * pSid) : SDrawFigure(kRef, pSid)
{
}

SDrawFigure * SDrawRef::Dup() const { return DupDrawFigure <SDrawRef> (this); }

int SDrawRef::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	THROW(SDrawFigure::Serialize(dir, rBuf, pCtx));
	THROW(pCtx->Serialize(dir, Ref, rBuf));
	THROW(pCtx->Serialize(dir, Origin, rBuf));
	CATCHZOK
	return ok;
}

int FASTCALL SDrawRef::Copy(const SDrawRef & rS)
{
	int    ok = SDrawFigure::Copy(rS);
	Ref = rS.Ref;
	Origin = rS.Origin;
	return ok;
}
//
//
//
SDrawText::SDrawText(const char * pSid) : SDrawFigure(kText, pSid), IdFont(0), Begin(0.0f)
{
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
SDrawGroup::SDrawGroup(const char * pSid, int dgt) : SDrawFigure(kGroup, pSid)
{
	if(dgt == dgtSymbol)
		Flags |= SDrawFigure::fSymbolGroup;
}

SDrawFigure * SDrawGroup::Dup() const { return DupDrawFigure <SDrawGroup> (this); }

int SDrawGroup::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	THROW(SDrawFigure::Serialize(dir, rBuf, pCtx));
	{
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
			SDrawFigure * p_item = at(i);
			if(p_item) {
				if(p_item->GetSid() == pSid) {
					atFree(i);
					ok = 1;
				}
				else if(recur && p_item->GetKind() == kGroup)
					ok = static_cast<SDrawGroup *>(p_item)->Remove(pSid, recur); // @recursion
			}
		}
	}
	return ok;
}

uint   SDrawGroup::GetCount() const { return SCollection::getCount(); }
const  SDrawFigure * SDrawGroup::Get(uint pos) const { return (pos < getCount()) ? at(pos) : 0; }

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
				const SDrawFigure * p_inner_item = static_cast<const SDrawGroup *>(p_item)->Find(pSid, 1); // @recursion
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

const SPoint2F & SDrawPath::GetCurrent()
{
	if(HasCurrent()) {
		uint   rc = ArgList.getCount();
		assert(rc >= 2);
		//Cur.Set(ArgList[rc-2], ArgList[rc-1]); // @erik v10.4.11
	}
	else
		Cur.Set(0.0);
	return Cur;
}

bool   SDrawPath::IsEmpty() const { return (OpList.getCount() == 0); }
uint   SDrawPath::GetCount() const { return OpList.getCount(); }
int    FASTCALL SDrawPath::CheckOp(int op) const { return BIN(oneof7(op, opNop, opMove, opLine, opCurve, opQuad, opArcSvg, opClose)); }

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

int FASTCALL SDrawPath::Move(const SPoint2F & rP)
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

int FASTCALL SDrawPath::Line(const SPoint2F & rP)
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

int SDrawPath::Curve(const SPoint2F & rMiddle1, const SPoint2F & rMiddle2, const SPoint2F & rEnd)
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

int SDrawPath::Quad(const SPoint2F & rMiddle, const SPoint2F & rEnd)
{
	int    ok = 1;
	OpList.Add(opQuad, ArgList.getCount(), 0);
	ArgList.add(rMiddle);
	ArgList.add(rEnd);
	Cur = rEnd;
	return ok;
}

int SDrawPath::ArcSvg(const SPoint2F & rCenter, float xAxRotation, int largeFlag, int sweepFlag, const SPoint2F & rEnd)
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
	if(rScan.Skip().GetDotPrefixedNumber(rTempBuf)) {
		float temp_val = rTempBuf.ToFloat();
		// @v10.4.10 rF = rTempBuf.ToFloat();
		rF = static_cast<float>(satof(rTempBuf)); // @v10.4.10 // @v10.7.9 atof-->satof
		assert(rF == temp_val);
	}
	else
		ok = 0;
	return ok;
}

static int GetSvgPathBinaryFlag(SStrScan & rScan, SString & rTempBuf, int & rF)
{
	int    ok = 1;
	rScan.Skip();
	int    c = rScan[0];
	if(c == '0' || c == '1') {
		rScan.Incr(1);
		rF = (c == '1') ? 1 : 0;
	}
	else
		ok = 0;
	return ok;
}

static int GetSvgPathPoint(SStrScan & rScan, SString & rTempBuf, SPoint2F & rP)
{
	int    ok = 1;
	float  temp_val = 0.0f;
	THROW(rScan.Skip().GetDotPrefixedNumber(rTempBuf));
	// @v10.4.10 rP.x = rTempBuf.ToFloat();
	temp_val = rTempBuf.ToFloat();
	rP.x = static_cast<float>(satof(rTempBuf)); // @v10.4.10 // @v10.7.9 atof-->satof
	assert(temp_val == rP.x);
	rScan.Skip().IncrChr(',');
	THROW(rScan.Skip().GetDotPrefixedNumber(rTempBuf));
	// @v10.4.10 rP.y = rTempBuf.ToFloat();
	temp_val = rTempBuf.ToFloat();
	rP.y = static_cast<float>(satof(rTempBuf)); // @v10.4.10 // @v10.7.9 atof-->satof
	assert(temp_val == rP.y);
	CATCHZOK
	return ok;
}

int SDrawPath::FromStr(const char * pStr, int fmt)
{
	int    ok = 1;
	if(fmt == fmtSVG) {
		SString temp_buf;
		SStrScan scan(pStr);
		SPoint2F pnt;
		SPoint2F pa[4];
		float  nmb;
		for(int c = scan.Skip()[0]; c != 0; c = scan.Skip()[0]) {
			scan.Incr();
			switch(c) {
				case 'M': // 2, SVG_PATH_CMD_MOVE_TO
					for(int first = 1; scan.Skip().IsDotPrefixedNumber(); first = 0) {
						THROW(GetSvgPathPoint(scan, temp_buf, pnt));
						scan.Skip().IncrChr(','); // @v10.7.8
						if(first)
							Move(pnt);
						else
							Line(pnt);
					}
					break;
				//case 'm': // 2, SVG_PATH_CMD_REL_MOVE_TO
				//	for(int first = 1; scan.Skip().IsDotPrefixedNumber(); first = 0) {
				//		THROW(GetSvgPathPoint(scan, temp_buf, pnt));
				//		const SPoint2F cur = GetCurrent();
				//		if(first)
				//			Move(cur + pnt);
				//		else
				//			Line(cur + pnt);
				//	}
				//	break;
				case 'm': // 2, SVG_PATH_CMD_REL_MOVE_TO
					for(int first = 1; scan.Skip().IsDotPrefixedNumber(); first = 0) {
						THROW(GetSvgPathPoint(scan, temp_buf, pnt));
						scan.Skip().IncrChr(','); // @v10.7.8
						const SPoint2F cur = GetCurrent();
						if(first)
							Move(cur + pnt);
						else
							Line(cur + pnt);
					}
					break;
				//case 'm': // 2, SVG_PATH_CMD_REL_MOVE_TO
				//{
				//	const SPoint2F cur = GetCurrent();
				//	int first = 1;
				//	while(scan.Skip().IsDotPrefixedNumber()) {
				//		THROW(GetSvgPathPoint(scan, temp_buf, pnt));
				//		//const SPoint2F cur = GetCurrent();
				//		if(first) {
				//			Move(cur + pnt);
				//			first = 0;
				//		}
				//		else
				//			Line(cur + pnt);
				//	}
				//}
				//break;
				case 'L': // 2, SVG_PATH_CMD_LINE_TO
					while(scan.Skip().IsDotPrefixedNumber()) {
						THROW(GetSvgPathPoint(scan, temp_buf, pnt));
						scan.Skip().IncrChr(','); // @v10.7.8
						Line(pnt);
					}
					break;
				case 'l': // 2, SVG_PATH_CMD_REL_LINE_TO
					while(scan.Skip().IsDotPrefixedNumber()) {
						THROW(GetSvgPathPoint(scan, temp_buf, pnt));
						scan.Skip().IncrChr(','); // @v10.7.8
						const SPoint2F cur = GetCurrent();
						Line(cur + pnt);
					}
					break;
				case 'H': // 1, SVG_PATH_CMD_HORIZONTAL_LINE_TO
					while(scan.Skip().IsDotPrefixedNumber()) {
						THROW(GetSvgPathNumber(scan, temp_buf, nmb));
						scan.Skip().IncrChr(','); // @v10.7.8
						const SPoint2F cur = GetCurrent();
						Line(pnt.Set(nmb, cur.y));
					}
					break;
				case 'h': // 1, SVG_PATH_CMD_REL_HORIZONTAL_LINE_TO
					while(scan.Skip().IsDotPrefixedNumber()) {
						THROW(GetSvgPathNumber(scan, temp_buf, nmb));
						scan.Skip().IncrChr(','); // @v10.7.8
						pnt = GetCurrent();
						Line(pnt.AddX(nmb));
					}
					break;
				case 'V': // 1, SVG_PATH_CMD_VERTICAL_LINE_TO
					while(scan.Skip().IsDotPrefixedNumber()) {
						THROW(GetSvgPathNumber(scan, temp_buf, nmb));
						scan.Skip().IncrChr(','); // @v10.7.8
						const SPoint2F cur = GetCurrent();
						Line(pnt.Set(cur.x, nmb));
					}
					break;
				case 'v': // 1, SVG_PATH_CMD_REL_VERTICAL_LINE_TO
					while(scan.Skip().IsDotPrefixedNumber()) {
						THROW(GetSvgPathNumber(scan, temp_buf, nmb));
						scan.Skip().IncrChr(','); // @v10.7.8
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
						scan.Skip().IncrChr(','); // @v10.7.8
						Curve(pa[0], pa[1], pa[2]);
					}
					break;
				case 'c': // 6, SVG_PATH_CMD_REL_CURVE_TO
					{
						while(scan.Skip().IsDotPrefixedNumber()) {
							THROW(GetSvgPathPoint(scan, temp_buf, pa[0]));
							scan.Skip().IncrChr(',');
							THROW(GetSvgPathPoint(scan, temp_buf, pa[1]));
							scan.Skip().IncrChr(',');
							THROW(GetSvgPathPoint(scan, temp_buf, pa[2]));
							scan.Skip().IncrChr(','); // @v10.7.8
							const SPoint2F cur = GetCurrent();
							Curve(cur + pa[0], cur + pa[1], cur + pa[2]);
						}
					}
					break;
				case 'S': // 4, SVG_PATH_CMD_SMOOTH_CURVE_TO
					while(scan.Skip().IsDotPrefixedNumber()) {
						THROW(GetSvgPathPoint(scan, temp_buf, pa[0]));
						scan.Skip().IncrChr(',');
						THROW(GetSvgPathPoint(scan, temp_buf, pa[1]));
						scan.Skip().IncrChr(','); // @v10.7.8
						if(OpList.getCount() && OpList.at(OpList.getCount()-1).Key == opCurve) {
							pnt = ArgList.getPoint(ArgList.getCount()-2);
							SPoint2F pnt_1 = ArgList.getPoint(ArgList.getCount()-4);
							SPoint2F rpnt(2.0f * pnt.x - pnt_1.x, 2.0f * pnt.y - pnt_1.y);
							Curve(rpnt, pa[0], pa[1]);
						}
						else {
							const SPoint2F cur = GetCurrent();
							Curve(cur, pa[0], pa[1]);
						}
					}
					break;
				case 's': // 4, SVG_PATH_CMD_REL_SMOOTH_CURVE_TO
					while(scan.Skip().IsDotPrefixedNumber()) {
						THROW(GetSvgPathPoint(scan, temp_buf, pa[0]));
						scan.Skip().IncrChr(',');
						THROW(GetSvgPathPoint(scan, temp_buf, pa[1]));
						scan.Skip().IncrChr(','); // @v10.7.8
						if(OpList.getCount() && OpList.at(OpList.getCount()-1).Key == opCurve) {
							pnt = ArgList.getPoint(ArgList.getCount()-2);
							SPoint2F pnt_1 = ArgList.getPoint(ArgList.getCount()-4);
							SPoint2F rpnt(2.0f * pnt.x - pnt_1.x, 2.0f * pnt.y - pnt_1.y);
							const SPoint2F cur = GetCurrent();
							Curve(rpnt, cur + pa[0], cur + pa[1]);
						}
						else {
							const SPoint2F cur = GetCurrent();
							Curve(cur, cur + pa[0], cur + pa[1]);
						}
					}
					break;
				case 'Q': // 4, SVG_PATH_CMD_QUADRATIC_CURVE_TO
					THROW(GetSvgPathPoint(scan, temp_buf, pa[0]));
					scan.Skip().IncrChr(',');
					THROW(GetSvgPathPoint(scan, temp_buf, pa[1]));
					scan.Skip().IncrChr(','); // @v10.7.8
					Quad(pa[0], pa[1]);
					break;
				case 'q': // 4, SVG_PATH_CMD_REL_QUADRATIC_CURVE_TO
					{
						THROW(GetSvgPathPoint(scan, temp_buf, pa[0]));
						scan.Skip().IncrChr(',');
						THROW(GetSvgPathPoint(scan, temp_buf, pa[1]));
						scan.Skip().IncrChr(','); // @v10.7.8
						const SPoint2F cur = GetCurrent();
						Quad(cur + pa[0], cur + pa[1]);
					}
					break;
				case 'T': // 2, SVG_PATH_CMD_SMOOTH_QUADRATIC_CURVE_TO
					{
						THROW(GetSvgPathPoint(scan, temp_buf, pa[0]));
						scan.Skip().IncrChr(','); // @v10.7.8
						if(OpList.getCount() && OpList.at(OpList.getCount()-1).Key == opQuad) {
							//SPoint2F refl_p2 = ArgList[ArgList.getCount()-1];
							//SPoint2F refl_p1 = ArgList[ArgList.getCount()-2];
							//pnt = refl_p2 + refl_p2 - refl_p1;
							pnt = ArgList.getPoint(ArgList.getCount()-4);
							Quad(pnt, pa[0]);
						}
						else {
							const SPoint2F cur = GetCurrent();
							Quad(cur, pa[0]);
						}
					}
					break;
				case 't': // 2, SVG_PATH_CMD_REL_SMOOTH_QUADRATIC_CURVE_TO
					{
						THROW(GetSvgPathPoint(scan, temp_buf, pa[0]));
						scan.Skip().IncrChr(','); // @v10.7.8
						if(OpList.getCount() && OpList.at(OpList.getCount()-1).Key == opQuad) {
							pnt = ArgList.getPoint(ArgList.getCount()-4);
							const SPoint2F cur = GetCurrent();
							Quad(pnt, cur+pa[0]);
						}
						else {
							const SPoint2F cur = GetCurrent();
							Quad(cur, cur+pa[0]);
						}
					}
					break;
				case 'A': // 7, SVG_PATH_CMD_ARC_TO
					while(scan.Skip().IsDotPrefixedNumber()) { // @v10.9.6 (while in front of '{')
						float x_axis_rotation = 0.0f;
						int   large_arc_flag = 0;
						int   sweep_flag = 0;
						THROW(GetSvgPathPoint(scan, temp_buf, pa[0])); // radius
						scan.Skip().IncrChr(','); // @v10.7.8
						THROW(GetSvgPathNumber(scan, temp_buf, x_axis_rotation));
						scan.Skip().IncrChr(','); // @v10.7.8
						// @v10.8.6 THROW(GetSvgPathNumber(scan, temp_buf, large_arc_flag));
						THROW(GetSvgPathBinaryFlag(scan, temp_buf, large_arc_flag)); // @v10.8.6 
						scan.Skip().IncrChr(','); // @v10.7.8
						// @v10.8.6 THROW(GetSvgPathNumber(scan, temp_buf, sweep_flag));
						THROW(GetSvgPathBinaryFlag(scan, temp_buf, sweep_flag)); // @v10.8.6 
						scan.Skip().IncrChr(','); // @v10.7.8
						THROW(GetSvgPathPoint(scan, temp_buf, pa[1])); // start point
						scan.Skip().IncrChr(','); // @v10.7.8
						ArcSvg(pa[0], x_axis_rotation, large_arc_flag, sweep_flag, pa[1]);
					}
					break;
				case 'a': // 7, SVG_PATH_CMD_REL_ARC_TO
					while(scan.Skip().IsDotPrefixedNumber()) { // @v10.7.8
						float x_axis_rotation = 0.0f;
						int   large_arc_flag = 0;
						int   sweep_flag = 0;
						THROW(GetSvgPathPoint(scan, temp_buf, pa[0])); // radius
						scan.Skip().IncrChr(','); // @v10.7.8
						THROW(GetSvgPathNumber(scan, temp_buf, x_axis_rotation));
						scan.Skip().IncrChr(','); // @v10.7.8
						// @v10.8.6 THROW(GetSvgPathNumber(scan, temp_buf, large_arc_flag));
						THROW(GetSvgPathBinaryFlag(scan, temp_buf, large_arc_flag)); // @v10.8.6 
						scan.Skip().IncrChr(','); // @v10.7.8
						// @v10.8.6 THROW(GetSvgPathNumber(scan, temp_buf, sweep_flag));
						THROW(GetSvgPathBinaryFlag(scan, temp_buf, sweep_flag)); // @v10.8.6 
						scan.Skip().IncrChr(','); // @v10.7.8
						THROW(GetSvgPathPoint(scan, temp_buf, pa[1])); // start point
						scan.Skip().IncrChr(','); // @v10.7.8
						const SPoint2F cur = GetCurrent();
						ArcSvg(pa[0], x_axis_rotation, large_arc_flag, sweep_flag, cur + pa[1]);
					}
					break;
				case 'Z': // 0, SVG_PATH_CMD_CLOSE_PATH
				case 'z': // 0, SVG_PATH_CMD_CLOSE_PATH
					Close();
					break;
				default:
					assert(0);
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

SDraw::SDraw() : SDrawGroup(0, SDrawGroup::dgtOrdinary), P_Tb(0)
{
	Flags |= fDraw;
}

SDraw::SDraw(const char * pSid) : SDrawGroup(pSid, SDrawGroup::dgtOrdinary), P_Tb(0)
{
	Flags |= fDraw;
}

SDraw::SDraw(const char * pSid, SPaintToolBox * pOuterTb) : SDrawGroup(pSid, SDrawGroup::dgtOrdinary)
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

int _ParseSvgFile(const char * pFileName, SDraw & rResult); // @prototype @defined(ssvg.cpp)

int SDraw::ParseSvgFile(const char * pFileName)
{
	return _ParseSvgFile(pFileName, *this);
}
//
//
//
SImageBuffer::Palette::Palette() : Count(0), P_Buf(0)
{
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

uint SImageBuffer::Palette::GetCount() const { return Count; }
size_t SImageBuffer::Palette::GetSize() const { return (Count * sizeof(uint32)); }
uint32 FASTCALL SImageBuffer::Palette::GetColor(uint idx) const { return (idx < Count) ? P_Buf[idx] : 0; }
void * SImageBuffer::Palette::GetBuffer() { return P_Buf; } // really private 
const uint32 * SImageBuffer::Palette::GetBufferC() const { return (const uint32 *)P_Buf; } // really private
//
//
//
SImageBuffer::PixF::PixF(int s) : S(s)
{
}

bool SImageBuffer::PixF::IsValid() const { return (GetBpp() != 0); }

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

/*static*/uint32 FASTCALL SImageBuffer::PixF::UniformToGrayscale(uint32 u)
{
	const uint32 _r = (u & 0x00ff0000) >> 16;
	const uint32 _g = (u & 0x0000ff00) >> 8;
	const uint32 _b = (u & 0x000000ff);
	return (_r == _g && _r == _b) ? _r : ((_r * 307 + _g * 604 + _b * 113) >> 10);
}

/*static*/COLORREF FASTCALL SImageBuffer::PixF::UniformToRGB(uint32 u)
{
	//const uint32 _r = (u & 0x00ff0000) >> 16;
	//const uint32 _g = (u & 0x0000ff00) >> 8;
	//const uint32 _b = (u & 0x000000ff);
	//return RGB(_r, _g, _b);
	return RGB(((u & 0x00ff0000) >> 16), ((u & 0x0000ff00) >> 8), (u & 0x000000ff));
}

int SImageBuffer::PixF::SetUniform(const void * pUniformBuf, void * pDest, uint width, SImageBuffer::Palette * pPalette) const
{
	int    ok = 0;
	const  uint bpp = GetBpp();
	const  uint32 * p_ufb = static_cast<const uint32 *>(pUniformBuf);
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
									*p++ = static_cast<uint8>(SImageBuffer::PixF::UniformToGrayscale(*p_ufb++));
									*p++ = static_cast<uint8>(SImageBuffer::PixF::UniformToGrayscale(*p_ufb++));
									*p++ = static_cast<uint8>(SImageBuffer::PixF::UniformToGrayscale(*p_ufb++));
									*p++ = static_cast<uint8>(SImageBuffer::PixF::UniformToGrayscale(*p_ufb++));
									*p++ = static_cast<uint8>(SImageBuffer::PixF::UniformToGrayscale(*p_ufb++));
									*p++ = static_cast<uint8>(SImageBuffer::PixF::UniformToGrayscale(*p_ufb++));
									*p++ = static_cast<uint8>(SImageBuffer::PixF::UniformToGrayscale(*p_ufb++));
									*p++ = static_cast<uint8>(SImageBuffer::PixF::UniformToGrayscale(*p_ufb++));
								}
								for(uint i = 0; i < dr; ++i) {
									*p++ = static_cast<uint8>(SImageBuffer::PixF::UniformToGrayscale(*p_ufb++));
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
									*p++ = static_cast<uint16>(SImageBuffer::PixF::UniformToGrayscale(*p_ufb++));
									*p++ = static_cast<uint16>(SImageBuffer::PixF::UniformToGrayscale(*p_ufb++));
									*p++ = static_cast<uint16>(SImageBuffer::PixF::UniformToGrayscale(*p_ufb++));
									*p++ = static_cast<uint16>(SImageBuffer::PixF::UniformToGrayscale(*p_ufb++));
									*p++ = static_cast<uint16>(SImageBuffer::PixF::UniformToGrayscale(*p_ufb++));
									*p++ = static_cast<uint16>(SImageBuffer::PixF::UniformToGrayscale(*p_ufb++));
									*p++ = static_cast<uint16>(SImageBuffer::PixF::UniformToGrayscale(*p_ufb++));
									*p++ = static_cast<uint16>(SImageBuffer::PixF::UniformToGrayscale(*p_ufb++));
								}
								for(uint i = 0; i < dr; ++i) {
									*p++ = static_cast<uint16>(SImageBuffer::PixF::UniformToGrayscale(*p_ufb++));
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
							{
								uint8 * p = (uint8 *)pDest;
								const uint dq = width / 8;
								const uint dr = width % 8;
								uint32 uf;
								#define OP uf = p_ufb[0]; p[0] = (uint8)((uf>>16)&0xff); p[1] = (uint8)((uf>>8)&0xff); p[2] = (uint8)(uf&0xff);
								for(uint i = 0; i < dq; ++i) {
									OP; p += 3; p_ufb++;
									OP; p += 3; p_ufb++;
									OP; p += 3; p_ufb++;
									OP; p += 3; p_ufb++;
									OP; p += 3; p_ufb++;
									OP; p += 3; p_ufb++;
									OP; p += 3; p_ufb++;
									OP; p += 3; p_ufb++;
								}
								for(uint i = 0; i < dr; ++i) {
									OP; p += 3; p_ufb++;
								}
								#undef OP
								ok = 1;
							}
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
							memcpy(pDest, p_ufb, width * sizeof(uint32));
							ok = 1;
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
				*p_ufb++ = pPalette->GetColor(PTR16C(pSrc)[i]);
		}
		else if(S == s8Idx) {
			const uint oct_count = width / 8;
			const uint oct_tail = (oct_count * 8) + (width % 8);
			const uint32 * p_palette_buf = pPalette->GetBufferC();
			const uint8 * p_src8 = PTR8C(pSrc);
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
				uint8 b = PTR8C(pSrc)[i];
				*p_ufb++ = pPalette->GetColor((b & 0xf0) >> 4);
				*p_ufb++ = pPalette->GetColor(b & 0x0f);
			}
		}
		else if(S == s2Idx) {
			for(i = 0; i < q; ++i) {
				uint8 b = PTR8C(pSrc)[i];
				*p_ufb++ = pPalette->GetColor((b & 0xc0) >> 6);
				*p_ufb++ = pPalette->GetColor((b & 0x30) >> 4);
				*p_ufb++ = pPalette->GetColor((b & 0x0c) >> 2);
				*p_ufb++ = pPalette->GetColor(b & 0x03);
			}
		}
		else if(S == s1Idx) {
			for(i = 0; i < width/8; ++i) {
				uint8 b = PTR8C(pSrc)[i];
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
					const uint16 * p = static_cast<const uint16 *>(pSrc);
					uint i;
					switch(S) {
						case s16ARGB1555:
	#define OP(c) (((uint32)((uint8)(0-(((c)&0x8000)>>15)))<<24)|((((c)&0x7c00)|(((c)&0x7000)>>5))<<9)|((((c)&0x3e0)|(((c)&0x380)>>5))<<6)|((((c)&0x1c)|(((c)&0x1f)<<5))>>2))
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
	#define OP(c) (0xff000000|((((c)&0x7c00)|(((c)&0x7000)>>5))<<9)|((((c)&0x3e0)|(((c)&0x380)>>5))<<6)|((((c)&0x1c)|(((c)&0x1f)<<5))>>2))
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
	#define OP(c) (0xff000000|((((c)&0xf800)|(((c)&0xe000)>>5))<<8)|((((c)&0x7e0)|(((c)&0x600)>>6))<<5)|((((c)&0x1c)|(((c)&0x1f)<<5))>>2))
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
	#define __ALPHA(c) (((c)&0xff000000)>>24)
	#define __R(c)     (((c)&0x00ff0000)>>16)
	#define __G(c)     (((c)&0x0000ff00)>>8)
	#define __B(c)     (((c)&0x000000ff))
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

SImageBuffer::StoreParam::StoreParam(int fmt) : Fmt(fmt), Flags(0), Quality(0)
{
}

SImageBuffer::SImageBuffer() : F(0)
{
	SBaseBuffer::Init();
	S.Z();
}

SImageBuffer::SImageBuffer(const SImageBuffer & rS) : F(rS.F)
{
	SBaseBuffer::Init();
	S.Z();
	Copy(rS);
}

SImageBuffer::SImageBuffer(uint w, uint h, PixF f) : F(f)
{
	SBaseBuffer::Init();
	S.Z();
	Init(w, h, f);
}

SImageBuffer::SImageBuffer(uint w, uint h) : F(PixF::s32ARGB)
{
	SBaseBuffer::Init();
	S.Z();
	Init(w, h, PixF(PixF::s32ARGB));
}

SImageBuffer::~SImageBuffer()
{
	Destroy();
}

void SImageBuffer::Destroy()
{
	S.Z();
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
		p_result = cairo_image_surface_create_for_data(reinterpret_cast<uchar *>(P_Buf), CAIRO_FORMAT_ARGB32, GetDim().x, GetDim().y, F.GetStride(GetDim().x));
	}
	return p_result;
}

int SImageBuffer::TransformToBounds(SPoint2S size, const SViewPort * pVp)
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
		cairo_transform(p_cr, reinterpret_cast<const cairo_matrix_t *>(&mtx));
		{
			cairo_surface_t * p_img_surf = static_cast<cairo_surface_t *>(CreateSurface(dsysCairo));
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
			THROW(rBuf.Write(*static_cast<SBuffer *>(file)));
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

bool FASTCALL SImageBuffer::Copy(const SImageBuffer & rS)
{
	S = rS.S;
	F = rS.F;
	return SBaseBuffer::Copy(rS);
}

bool FASTCALL SImageBuffer::IsEq(const SImageBuffer & rS) const
{
	if(F.S != rS.F.S)
		return false;
	else if(S != rS.S)
		return false;
	else if(Size != rS.Size)
		return false;
	else if(Size && memcmp(P_Buf, rS.P_Buf, Size) != 0)
		return false;
	return true;
}

SImageBuffer::PixF SImageBuffer::GetFormat() const { return F; }
uint   SImageBuffer::GetWidth() const { return (uint)S.x; }
uint   SImageBuffer::GetHeight() const { return (uint)S.y; }
SPoint2S SImageBuffer::GetDim() const { return S; }
SPoint2F SImageBuffer::GetDimF() const { return (SPoint2F)S; }
const  uint8 * SImageBuffer::GetData() const { return reinterpret_cast<const uint8 *>(P_Buf); }

size_t SImageBuffer::GetNominalBufSize() const
{
	return (GetHeight() * GetFormat().GetStride(GetWidth()));
}

int SImageBuffer::Store(const StoreParam & rP, SFile & rF) const
{
	int    ok = 1;
	THROW(rF.IsValid());
	if(rP.Fmt == SFileFormat::Png) {
		THROW(StorePng(rP, rF));
	}
	else if(rP.Fmt == SFileFormat::Jpeg) {
		THROW(StoreJpeg(rP, rF));
	}
	else if(rP.Fmt == SFileFormat::Webp) { // @v11.3.4
		THROW(StoreWebp(rP, rF));
	}
	else if(rP.Fmt == SFileFormat::Bmp) { // @v11.3.4
		THROW(StoreBmp(rP, rF));
	}
	else {
		CALLEXCEPT_S(SLERR_UNSUPPIMGFILEFORMAT);
	}
	CATCHZOK
	return ok;
}

int SImageBuffer::StoreMime_Base64(const StoreParam & rP, SString & rBuf) const
{
	rBuf.Z();
	int    ok = 1;
	SFile  f_out(SBuffer(), SFile::mWrite|SFile::mBinary);
	THROW(f_out.IsValid());
	THROW(Store(rP, f_out));
	{
		SBuffer * p_buffer = f_out;
		THROW(p_buffer && p_buffer->GetAvailableSize());
		rBuf.EncodeMime64(p_buffer->GetBufC(), p_buffer->GetAvailableSize());
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
		case SFileFormat::Webp: // @v11.3.4
			THROW(LoadWebp(rF));
			break;
		case SFileFormat::Tiff:
			//THROW(LoadTiff(rF, ff));
			//break;
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

/*static*/bool FASTCALL SImageBuffer::IsSupportedFormat(int fm)
{
	bool   ok = true;
	switch(fm) {
		case SFileFormat::Jpeg:
		case SFileFormat::Bmp:
		case SFileFormat::Png:
		case SFileFormat::Ico:
		case SFileFormat::Cur:
		case SFileFormat::Gif:
		case SFileFormat::Webp: // @v11.3.4
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
		memzero(P_Buf, Size);
		F = f;
		S.Set((int)w, (int)h);
		ok = 1;
	}
	return ok;
}

int SImageBuffer::Init(uint w, uint h)
{
	int    ok = 0;
	SImageBuffer::PixF f(PixF::s32ARGB);
	uint   stride = f.GetStride(w);
	if(stride && Alloc(stride * h)) {
		memzero(P_Buf, Size);
		F = f;
		S.Set((int)w, (int)h);
		ok = 1;
	}
	return ok;
}

uint8 * FASTCALL SImageBuffer::GetScanline(uint lineNo) const
{
	uint8 * p_ret = 0;
	if((int)lineNo < S.y) {
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
			THROW(s.GetUniform(PTR8C(pSrc) + j * src_stride, PTR32(P_Buf)+((S.y+j)*S.x), S.x, pPalette));
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

int SImageBuffer::GetSubImage(SImageBuffer & rDest, SPoint2S start, SPoint2S size) const
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

void * SImageBuffer::TransformToIcon() const
{
	HICON h_icon = 0;
	HDC hDC        = ::GetDC(NULL);
	HDC hMainDC    = ::CreateCompatibleDC(hDC);
	HDC hAndMaskDC = ::CreateCompatibleDC(hDC);
	HDC hXorMaskDC = ::CreateCompatibleDC(hDC);
	const int _w = (int)S.x;
	const int _h = (int)S.y;
	//
	// Получаем размеры битмапа
	//
	//BITMAP bm;
	//::GetObject(hBitmap, sizeof(BITMAP), &bm);
	HBITMAP hAndMaskBitmap  = ::CreateCompatibleBitmap(hDC, _w, _h);
	HBITMAP hXorMaskBitmap  = ::CreateCompatibleBitmap(hDC, _w, _h);

	//Select the bitmaps to DC

	//HBITMAP hOldMainBitmap     = (HBITMAP)::SelectObject(hMainDC,    hBitmap);
	HBITMAP hOldAndMaskBitmap  = static_cast<HBITMAP>(::SelectObject(hAndMaskDC, hAndMaskBitmap));
	HBITMAP hOldXorMaskBitmap  = static_cast<HBITMAP>(::SelectObject(hXorMaskDC, hXorMaskBitmap));
	SetBkMode(hAndMaskDC, TRANSPARENT); // @v10.7.8
	SetBkMode(hXorMaskDC, TRANSPARENT); // @v10.7.8
	//Scan each pixel of the souce bitmap and create the masks
	//COLORREF MainBitPixel;

	STempBuffer uniform_buf(0);
	//int    row_stride;
	THROW_S((_w >= 1 && _w <= 30000) && (_h >= 1 && _h <= 30000), SLERR_INVIMAGESIZE); // no image
	THROW(uniform_buf.Alloc(S.x * 4));

	for(int y = 0; y < _h; ++y) {
		size_t uboffs = 0;
		THROW(F.GetUniform(GetScanline(y), uniform_buf, S.x, 0));
		for(int x = 0; x < _w; x++) {
			COLORREF xor_mask = SImageBuffer::PixF::UniformToRGB(*PTR32C(uniform_buf.ucptr() + uboffs));
			::SetPixel(hAndMaskDC, x, y, RGB(0x00, 0x00, 0x00));
			::SetPixel(hXorMaskDC, x, y, xor_mask);
			uboffs += 4;
		}
	}
	::SelectObject(hAndMaskDC, hOldAndMaskBitmap); // @v10.7.8
	::SelectObject(hXorMaskDC, hOldXorMaskBitmap); // @v10.7.8
	{
		ICONINFO iconinfo = {0};
		iconinfo.fIcon    = TRUE; // icon (FALSE - cursor)
		iconinfo.xHotspot = 0;
		iconinfo.yHotspot = 0;
		iconinfo.hbmMask  = hAndMaskBitmap;
		iconinfo.hbmColor = hXorMaskBitmap;
		h_icon = ::CreateIconIndirect(&iconinfo);
		//::SelectObject(hMainDC,    hOldMainBitmap);
		//::SelectObject(hAndMaskDC, hOldAndMaskBitmap);
		//::SelectObject(hXorMaskDC, hOldXorMaskBitmap);

		::DeleteDC(hXorMaskDC);
		::DeleteDC(hAndMaskDC);
		::DeleteDC(hMainDC);

		::ReleaseDC(NULL, hDC);

		::DeleteObject(hAndMaskBitmap);
		::DeleteObject(hXorMaskBitmap);
		//::DeleteObject(hOldMainBitmap);
		//::DeleteObject(hOldAndMaskBitmap);
		//::DeleteObject(hOldXorMaskBitmap);
	}
	CATCH
		h_icon = 0;
	ENDCATCH
	return h_icon;
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
	SImageBuffer::PixF ff(0);

	Destroy();
	THROW(rBuf.Read(&fh, sizeof(fh)));
	THROW_S_S(fh.bfType == BMP_SIGN, SLERR_IMAGEFILENOTBMP, pAddedErrorInfo); // "424D"
	THROW(rBuf.Read(&ih, sizeof(ih.biSize)));
	THROW_S_S(oneof2(ih.biSize, 40, 64), SLERR_INVBMPHEADER, pAddedErrorInfo);
	THROW(rBuf.Read(PTR8(&ih)+sizeof(ih.biSize), sizeof(ih)-sizeof(ih.biSize)));
	THROW_S(ih.biCompression == 0, SLERR_BMPCOMPRNSUPPORTED);
	switch(ih.biBitCount) {
		case  0: 
			break;
		case  1: 
			map_entry_size = 4; 
			ff.S = PixF::s1Idx; 
			break;
		case  4: 
			map_entry_size = 4; 
			ff.S = PixF::s4Idx; 
			break;
		case  8: 
			map_entry_size = 4; 
			ff.S = PixF::s8Idx; 
			break;
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
		INITWINBITMAPINFO(bi);
		::GetDIBits(hDc, hBmp, 0, 0, 0, &bi, DIB_RGB_COLORS);
		{
			BmpFileHeader fh;
			fh.Init();
			const uint width  = (uint)labs(bi.bmiHeader.biWidth);
			const uint height = (uint)labs(bi.bmiHeader.biHeight);
			STempBuffer temp_buf(width * height * sizeof(uint32)); // Буфер для изображения (с запасом)
			bi.bmiHeader.biCompression = 0;
			::GetDIBits(hDc, hBmp, 0, height, temp_buf, &bi, DIB_RGB_COLORS);
			THROW(buffer.Write(&fh, sizeof(fh)));
			THROW(buffer.Write(&bi, bi.bmiHeader.biSize));
			THROW(buffer.Write(temp_buf, temp_buf.GetSize()));
			THROW(Helper_LoadBmp(buffer, ""));
			if(subImgSqIdx) {
				if(subImgSqSide && subImgSqSide < width && subImgSqSide < height) {
					SPoint2S sz, p;
					sz = static_cast<int>(subImgSqSide);
					p.Z();
					for(uint i = 1; i < subImgSqIdx; i++) {
						p.x += sz.x;
						if(p.x >= static_cast<int>(width)) {
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
//
//
//
static ulong BmpSize(long width, long height)
{
	long pad = ((width % 4) * -3UL) & 3; // Overflow-safe 
	if(width < 1 || height < 1) {
		return 0; // Illegal size 
	} 
	else if(width > static_cast<long>(((0x7fffffffL - sizeof(BmpFileHeader) - sizeof(BmpInfoHeader)) / height - pad) / 3)) {
		return 0; // Overflow 
	} 
	else {
		return height * (width * 3 + pad) + sizeof(BmpFileHeader) + sizeof(BmpInfoHeader);
	}
}

void * SImageBuffer::TransformToBitmap() const
{
	HBITMAP h_result = 0;
	BITMAP  bmp;
	const uint _w = static_cast<uint>(S.x);
	const uint _h = static_cast<uint>(S.y);
	THROW_S((_w >= 1 && _w <= 30000) && (_h >= 1 && _h <= 30000), SLERR_INVIMAGESIZE); // no image
	{
		const SImageBuffer::PixF pf(SImageBuffer::PixF::s32ARGB);
		const uint   stride = pf.GetStride(_w);//ALIGNSIZE(_w * 3, 1);
		STempBuffer uniform_buf(_w * 4);
		STempBuffer bmp_image(stride * _h);
		THROW(uniform_buf.IsValid());
		memzero(&bmp, sizeof(bmp));
		bmp.bmType = 0;
		bmp.bmWidth = static_cast<long>(_w);
		bmp.bmHeight = static_cast<long>(_h);
		bmp.bmWidthBytes = stride;
		bmp.bmPlanes = 1;
		bmp.bmBitsPixel = pf.GetBpp();
		bmp.bmBits = bmp_image;
		//uint8 * p = 0;
		for(uint y = 0; y < _h; y++) {
			THROW(F.GetUniform(GetScanline(y), uniform_buf, S.x, 0)); // ***
			THROW(pf.SetUniform(uniform_buf, PTR8(bmp.bmBits) + (y * stride), _w, 0));
			/*for(uint x = 0; x < _w; x++) {
				const COLORREF pix = SImageBuffer::PixF::UniformToRGB(PTR32C(uniform_buf.ucptr())[x]);
				{
					p = PTR8(bmp.bmBits) + (y * stride) + (x * 3);
					p[0]  = static_cast<uchar>(GetRValue(pix));
					p[1]  = static_cast<uchar>(GetGValue(pix));
					p[2]  = static_cast<uchar>(GetBValue(pix));
				}
			}*/
		}
		h_result = ::CreateBitmapIndirect(&bmp);
	}
	CATCH
		h_result = 0;
	ENDCATCH
	return h_result;
}

int SImageBuffer::StoreBmp(const StoreParam & rP, SFile & rF) const
{
	// 24-битный bmp без компрессии
	int    ok = 1;
	//BmpFileHeader fh; // ***
	//BmpInfoHeader ih;
	const uint _w = S.x;
	const uint _h = S.y;
	const uint pad = (_w * -3UL) & 3;
	THROW_S((_w >= 1 && _w <= 30000) && (_h >= 1 && _h <= 30000), SLERR_INVIMAGESIZE); // no image
	{
		const uint _bmp_size = BmpSize(_w, _h);
		assert(_bmp_size > 0);
		STempBuffer bmp_image(_bmp_size);
		STempBuffer uniform_buf(_w * 4);
		THROW(bmp_image.IsValid());
		THROW(uniform_buf.IsValid());
		BmpFileHeader * p_fh = reinterpret_cast<BmpFileHeader *>(static_cast<char *>(bmp_image));
		BmpInfoHeader * p_ih = reinterpret_cast<BmpInfoHeader *>(static_cast<char *>(bmp_image)+sizeof(BmpFileHeader));
		p_fh->Init();
		p_fh->bfSize = _h * (_w * 3 + pad) + 14 + 40;
		p_fh->bfOffBits = 0x36;
		p_ih->biSize = sizeof(BmpInfoHeader);
		p_ih->biWidth = _w;
		p_ih->biHeight = -static_cast<int>(_h);
		p_ih->biPlanes = 1;
		p_ih->biBitCount = 24;
		p_ih->biCompression = 0;
		p_ih->biSizeImage = 0;
		p_ih->biXPelsPerMeter = 0;
		p_ih->biYPelsPerMeter = 0;
		p_ih->biClrUsed = 0;
		p_ih->biClrImportant = 0;
		uint8 * p_bmp_pix_start = reinterpret_cast<uint8 *>(static_cast<char *>(bmp_image)+sizeof(BmpFileHeader)+sizeof(BmpInfoHeader));
		for(uint y = 0; y < _h; y++) {
			size_t uboffs = 0;
			THROW(F.GetUniform(GetScanline(y), uniform_buf, S.x, 0)); // ***
			for(uint x = 0; x < _w; x++) {
				const COLORREF pix = SImageBuffer::PixF::UniformToRGB(*PTR32C(uniform_buf.ucptr() + uboffs));
				//::SetPixel(hAndMaskDC, x, y, RGB(0x00, 0x00, 0x00));
				//::SetPixel(hXorMaskDC, x, y, xor_mask);
				//static void bmp_set(void *buf, long x, long y, ulong color)
				{
					//uchar * p;
					//uchar * hdr = (uchar *)buf;
					//ulong width = (ulong)hdr[18] << 0 | (ulong)hdr[19] << 8 | (ulong)hdr[20] << 16 | (ulong)hdr[21] << 24;
					long pad = (_w * -3UL) & 3;
				#ifdef BMP_COMPAT
					//ulong height = (ulong)hdr[22] <<  0 | (ulong)hdr[23] <<  8 | (ulong)hdr[24] << 16 | (ulong)hdr[25] << 24;
					y = _h - y - 1;
				#endif
					uint8 * p = p_bmp_pix_start + y * (_w * 3 + pad) + x * 3;
					p[0]  = static_cast<uchar>(GetRValue(pix));
					p[1]  = static_cast<uchar>(GetGValue(pix));
					p[2]  = static_cast<uchar>(GetBValue(pix));
				}
				uboffs += 4;
			}
		}
		THROW(rF.Write(bmp_image, _bmp_size));
	}
	CATCHZOK
	return ok;
}
//
//
//

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
		THROW(p_idir = static_cast<IconDirEntry *>(SAlloc::M(dir_entry_size)));
		THROW(rF.ReadV(p_idir, dir_entry_size));
		THROW(rF.Seek(p_idir[pageIdx].dwImageOffset));
		THROW(rF.ReadV(&bm_hdr, sizeof(bm_hdr)));
		total_rc_size += sizeof(bm_hdr);
		{
			const uint bit_count = bm_hdr.biBitCount;
			const int  revert = BIN(bm_hdr.biWidth > 0);
			const uint width  = abs(bm_hdr.biWidth);
			const uint height = abs(bm_hdr.biHeight/2);   // height == xor + and mask
			const uint line_size = (((width*bit_count)+7)/8 + 3) & ~3;
			STempBuffer line_buf(line_size * height);
			SImageBuffer::PixF ff(0);
			THROW(Init(width, 0, PixF(PixF::s32ARGB)));
			switch(bit_count) {
				case 1:
				case 2:
				case 4:
				case 8:
				case 16:
					{
						switch(bit_count) {
							case 1: ff = PixF(PixF::s1Idx); break;
							case 2: ff = PixF(PixF::s2Idx); break;
							case 4: ff = PixF(PixF::s4Idx); break;
							case 8: ff = PixF(PixF::s8Idx); break;
							case 16: ff = PixF(PixF::s16Idx); break;
						}
						SImageBuffer::Palette palette;
						const size_t palette_size = (1 << bit_count) * sizeof(uint32);
						THROW(palette.Alloc(palette_size));
						THROW(rF.ReadV(palette.GetBuffer(), palette_size));
						total_rc_size += palette_size;
						THROW(rF.ReadV(line_buf, line_size * height));
						total_rc_size += (line_size * height);
						for(uint i = 0; i < height; ++i)
							THROW(AddLines(PTR8C(line_buf.vcptr())+line_size*i, ff, 1, &palette));
					}
					break;
				case 24:
				case 32:
					{
						switch(bit_count) {
							case 24: ff = PixF(PixF::s24RGB); break;
							case 32: ff = PixF(PixF::s32ARGB); break;
						}
						THROW(rF.ReadV(line_buf, line_size * height));
						total_rc_size += (line_size * height);
						for(uint i = 0; i < height; ++i)
							THROW(AddLines(PTR8C(line_buf.vcptr())+line_size*i, ff, 1, 0));
					}
					break; // @v10.3.11 @fix
				default:
					ok = 0;
			}
			{
				const size_t width_and = (((width)+31)>>5)<<2;
				uint32 * p_data = (uint32 *)(GetData());
				const uint _s = GetWidth() * GetHeight();
				THROW(rF.ReadV(line_buf, width_and * height));
				total_rc_size += (width_and * height);
				for(uint i = 0; i < height; ++i) {
					const size_t step = i*width;
					const size_t step_and = i*width_and;
					for(uint j = 0; j < width; ++j) {
						int a = (PTR8C(line_buf.vcptr())[step_and + (j>>3)]&(0x80>>(j&0x07))) != 0 ? 0 : 0xFF;
						if(!a)
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
	//#include <../osf/libjpeg/jerror.h>
#endif

int SImageBuffer::LoadJpeg(SFile & rF, int fileFmt)
{
	struct JpegErr {
		static void ExitFunc(j_common_ptr pCInfo)
		{
			int    err_code = 0;
			JpegErr * p_err = reinterpret_cast<JpegErr *>(pCInfo->err);
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
		MEMSZERO(di); // @v11.7.0
		di.err = jpeg_std_error(&jpeg_err.pub);
		jpeg_err.pub.error_exit = JpegErr::ExitFunc;
		err_code = setjmp(jpeg_err.setjmp_buf);
		if(err_code) {
			SLS.SetAddedMsgString(rF.GetName());
			CALLEXCEPT();
		}
		else {
			jpeg_create_decompress(&di);
			if(static_cast<FILE *>(rF) != 0)
				jpeg_stdio_src(&di, static_cast<FILE *>(rF));
			else {
				SBaseBuffer buf;
				if(rF.GetBuffer(buf)) {
					jpeg_mem_src(&di, reinterpret_cast<const uchar *>(buf.P_Buf), buf.Size);
				}
			}
			jpeg_read_header(&di, TRUE);
			jpeg_start_decompress(&di);
			const uint out_width = di.output_width;
			if(Init(out_width, 0)) {
				PixF fmt(0);
				if(di.output_components == 1)
					fmt = PixF(PixF::s8GrayScale);
				else if(di.output_components == 3)
					fmt = PixF(PixF::s24RGB);
				else if(di.output_components == 4)
					fmt = PixF(PixF::s32ARGB);
				{
					const uint max_lines = 1;
					const size_t line_size = di.output_width * di.output_components;
					p_row_buf = static_cast<uint8 *>(SAlloc::M(line_size * max_lines));
					if(p_row_buf) {
						{
							const uint _stride = F.GetStride(S.x);
							THROW(_stride);
							THROW(Alloc(_stride * di.output_height));
						}
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

int SImageBuffer::StoreJpeg(const StoreParam & rP, SFile & rF) const
{
	struct JpegErr {
		static void ExitFunc(j_common_ptr pCInfo)
		{
			int    err_code = 0;
			JpegErr * p_err = reinterpret_cast<JpegErr *>(pCInfo->err);
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
	struct jpeg_compress_struct cinfo;
	uchar * p_outbuf = 0;
	ulong  outbuf_size = 0;
	JSAMPROW row_pointer[1];
	STempBuffer row_buf(0);
	STempBuffer uniform_buf(0);
	int    row_stride;
	THROW_S((S.x >= 1 && S.x <= 30000) && (S.y >= 1 && S.y <= 30000), SLERR_INVIMAGESIZE); // no image
	THROW(row_buf.Alloc(S.x * 3));
	THROW(uniform_buf.Alloc(S.x * 4));
	row_pointer[0] = (JSAMPROW)(char *)row_buf;
	cinfo.err = jpeg_std_error(&jpeg_err.pub);
	jpeg_err.pub.error_exit = JpegErr::ExitFunc;
	err_code = setjmp(jpeg_err.setjmp_buf);
	if(err_code) {
		SLS.SetAddedMsgString(rF.GetName());
		CALLEXCEPT();
	}
	else {
		PixF   px(PixF::s24RGB);
		jpeg_create_compress(&cinfo);
		jpeg_mem_dest(&cinfo, &p_outbuf, &outbuf_size);
		cinfo.image_width = S.x; // image width and height, in pixels 
		cinfo.image_height = S.y;
		cinfo.input_components = 3; // # of color components per pixel 
		cinfo.in_color_space = JCS_RGB; // colorspace of input image 
		jpeg_set_defaults(&cinfo);
		jpeg_set_quality(&cinfo, NZOR(rP.Quality, 75), TRUE /* limit to baseline-JPEG values */);
		jpeg_start_compress(&cinfo, TRUE);
		row_stride = S.x * 3; // JSAMPLEs per row in image_buffer 
		while(cinfo.next_scanline < cinfo.image_height) {
			THROW(F.GetUniform(GetScanline(cinfo.next_scanline), uniform_buf, S.x, 0));
			THROW(px.SetUniform(uniform_buf, row_buf, S.x, 0));
			(void)jpeg_write_scanlines(&cinfo, row_pointer, 1);
		}
		jpeg_finish_compress(&cinfo);
		THROW(rF.Write(p_outbuf, (size_t)outbuf_size));
	}
	CATCHZOK
	jpeg_destroy_compress(&cinfo);
	SAlloc::F(p_outbuf);
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
		SFile * p_file = static_cast<SFile *>(png_get_io_ptr(pPng));
		if(!p_file || !p_file->ReadV(pData, length))
			png_error(pPng, 0);
	}
	static void PNGAPI WriteFunc(png_structp pPng, png_bytep pData, size_t length)
	{
		SFile * p_file = static_cast<SFile *>(png_get_io_ptr(pPng));
		if(!p_file || !p_file->Write(pData, length))
			png_error(pPng, 0);
	}
	static void PNGAPI FlushFunc(png_structp pPng)
	{
		SFile * p_file = static_cast<SFile *>(png_get_io_ptr(pPng));
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
				b[0] = 0;
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
		if(oneof2(color_type, PNG_COLOR_TYPE_GRAY, PNG_COLOR_TYPE_GRAY_ALPHA))
			png_set_gray_to_rgb(p_png);
		if(interlace != PNG_INTERLACE_NONE)
			png_set_interlace_handling(p_png);
		png_set_bgr(p_png);
		png_set_filler(p_png, 0xff, PNG_FILLER_AFTER);
		png_read_update_info(p_png, p_info);
		THROW(Init(width, 0));
		THROW(p_row_buf = (uint8 *)SAlloc::M(width*sizeof(uint32)));
		{
			PixF fmt(PixF::s32ARGB);
			int  pass_count = png_set_interlace_handling(p_png);
			int  i = pass_count;
			if(i > 0) {
				uint j;
				{
					const uint _stride = F.GetStride(S.x);
					THROW(_stride);
					THROW(Alloc(_stride * height));
				}
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

int SImageBuffer::StorePng(const StoreParam & rP, SFile & rF) const
{
	int    ok = 1;
	int    err_code = 0;
	png_struct * p_png = 0;
	png_info * p_info = 0;
	uint8 ** volatile pp_rows = 0;
	const uint stride = F.GetStride(S.x);
	THROW_S((S.x >= 1 && S.x <= 30000) && (S.y >= 1 && S.y <= 30000), SLERR_INVIMAGESIZE); // no image
	THROW(pp_rows = static_cast<uint8 **>(SAlloc::M(S.y * sizeof(uint8 *))));
	for(int i = 0; i < S.y; i++) {
		pp_rows[i] = PTR8(P_Buf)+i*stride;
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
			// 
			// We have to call png_write_info() before setting up the write
			// transformation, since it stores data internally in 'png'
			// that is needed for the write transformation functions to work.
			// 
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

static int GifReadFunc(GifFileType * pF, uint8 * pData, int size)
{
	size_t actual_size = 0;
	SFile * p_f = static_cast<SFile *>(pF->UserData);
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
		THROW(p_buffer = static_cast<GifRowType *>(SAlloc::M(p_gf->SHeight * sizeof(GifRowType))));
		size = p_gf->SWidth * sizeof(GifPixelType); // Size in bytes one row
		for(i = 0; i < p_gf->SHeight; i++) {
			// Allocate rows, and set their color to background
			THROW(p_buffer[i] = static_cast<GifRowType>(SAlloc::M(size)));
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
					//slfprintf_stderr("Image %d is not confined to screen dimension, aborted.\n",ImageNum);
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
					uint8 * p_extension = 0;
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
		{
			const uint _stride = F.GetStride(S.x);
			THROW(_stride);
			THROW(Alloc(_stride * p_gf->SHeight));
		}
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
#if(_MSC_VER >= 1900)
	#include <../osf/libwebp/src/webp/decode.h>
	#include <../osf/libwebp/src/webp/encode.h>
#endif

int SImageBuffer::LoadWebp(SFile & rF)
{
#if(_MSC_VER >= 1900)
	int    ok = 1;
	int    width = 0;
	int    height = 0;
	uint8 * p_result = 0;
	int64  fsize = 0;
	THROW(rF.CalcSize(&fsize));
	THROW_S(fsize <= SMEGABYTE(64), SLERR_FILETOOBIG);
	{
		STempBuffer fbuf(static_cast<size_t>(fsize) + 512);
		size_t actual_size = 0;
		const SImageBuffer::PixF fmt(PixF::s32ARGB);
		THROW(fbuf.IsValid());
		THROW(rF.Read(fbuf, static_cast<size_t>(fsize), &actual_size));
		p_result = /*WebPDecodeARGB*//*WebPDecodeRGBA*/WebPDecodeBGRA(fbuf.ucptr(), actual_size, &width, &height);
		THROW(p_result);
		THROW(Init(width, height, fmt));
		assert(SBaseBuffer::Size >= static_cast<size_t>((width * height) * (fmt.GetBpp()/8)));
		memcpy(P_Buf, p_result, (width * height) * (fmt.GetBpp()/8));
	}
	CATCHZOK
	SAlloc::F(p_result);
#else
	int    ok = 0;
#endif
	return ok;
}

int SImageBuffer::StoreWebp(const StoreParam & rP, SFile & rF) const
{
#if(_MSC_VER >= 1900)
	int    ok = 1;
	uint8 * p_result_buf = 0;
	size_t result_buf_size = 0;
	THROW_S((S.x >= 1 && S.x <= 30000) && (S.y >= 1 && S.y <= 30000), SLERR_INVIMAGESIZE); // no image
	{
		//WebPEncodeBGRA(const uint8* bgra, int width, int height, int stride, float quality_factor, uint8** output);
		uint stride = F.GetStride(S.x);
		const float quality = (rP.Quality == 0) ? 100.0f : sclamp(static_cast<float>(rP.Quality), 0.0f, 100.0f);
		THROW(stride > 0 && stride < static_cast<uint>(S.x * 6));
		result_buf_size = WebPEncodeBGRA(reinterpret_cast<const uint8 *>(P_Buf), S.x, S.y, stride, quality, &p_result_buf);
		THROW(result_buf_size);
		THROW(rF.Write(p_result_buf, result_buf_size));
	}
	CATCHZOK
	SAlloc::F(p_result_buf);
#else
	int    ok = 0;
#endif
	return ok;
}
//
//
//
/*#include <..\osf\tiff\libtiff\tiffio.h>

int SImageBuffer::LoadTiff(SFile & rF, int fileFmt)
{
	int    ok = 0;
	TIFF * tif = TIFFOpen(argv[1], "r");
	if(tif) {
		uint32 w, h;
		size_t npixels;
		uint32 * raster;
		TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &w);
		TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &h);
		npixels = w * h;
		raster = (uint32 *)SAlloc::M(npixels * sizeof (uint32));
		if(raster) {
			if(TIFFReadRGBAImage(tif, w, h, raster, 0)) {
				...process raster data...
			}
			SAlloc::F(raster);
		}
		TIFFClose(tif);
	}
	return ok;
}*/
//
//
//
SDrawImage::SDrawImage(const char * pSid) : SDrawFigure(SDrawFigure::kImage, pSid)
{
	Buf.Init(0, 0);
}

SDrawImage::SDrawImage(const SImageBuffer & rBuf, const char * pSid) : SDrawFigure(SDrawFigure::kImage, pSid)
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

int SDrawImage::Store(const SImageBuffer::StoreParam & rP, SFile & rF)
{
	return Buf.Store(rP, rF);
}

int SDrawImage::TransformToBounds(SPoint2S size, const SViewPort * pVp)
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
#if 0 // {
// 
// Pointers to unaligned-bits.  These are necessary for handling bitmap-info's loaded from file.
// 
typedef BITMAPINFO       UNALIGNED *UPBITMAPINFO;
typedef BITMAPINFOHEADER UNALIGNED *UPBITMAPINFOHEADER;
typedef BITMAPCOREHEADER UNALIGNED *UPBITMAPCOREHEADER;
// 
// Bitmap resource IDs
// 
#define BMR_ICON    1
#define BMR_BITMAP  2
#define BMR_CURSOR  3
#define BPP01_MAXCOLORS     2
#define BPP04_MAXCOLORS    16
#define BPP08_MAXCOLORS   256
#define RESCLR_BLACK      0x00000000
#define RESCLR_WHITE      0x00FFFFFF
#define MR_FAILFOR40    0x01
#define MR_MONOCHROME   0x02

//#define GETINITDC() (gfSystemInitialized ? NtUserGetDC(NULL) : CreateDCW(L"DISPLAY", L"", NULL, NULL))
//#define RELEASEINITDC(hdc) (gfSystemInitialized ? ReleaseDC(NULL, hdc) : DeleteDC(hdc))
#define ISRIFFFORMAT(p) (((UNALIGNED RTAG *)(p))->ckID == FOURCC_RIFF)
#define IS_PTR(p)       ((((ULONG_PTR)(p)) & ~USHRT_MAX/*MAXUSHORT*/) != 0)
#define PTR_TO_ID(p)    ((USHORT)(((ULONG_PTR)(p)) & USHRT_MAX/*MAXUSHORT*/))

#define BitmapSize(cx, cy, planes, bits) (BitmapWidth(cx, bits) * (cy) * (planes))
#define BitmapWidth(cx, bpp)             (((((cx)*(bpp)) + 31) & ~31) >> 3)
#define RGBX(rgb)  RGB(GetBValue(rgb), GetGValue(rgb), GetRValue(rgb))
//#define SYSRGB(i)  gpsi->argbSystem[COLOR_##i]

typedef struct _OLDCURSOR {
	BYTE bType;
	BYTE bFormat;
	WORD xHotSpot;  // 0 for icons
	WORD yHotSpot;  // 0 for icons
	WORD cx;
	WORD cy;
	WORD cxBytes;
	WORD wReserved2;
	BYTE abBitmap[1];
} OLDCURSOR, *POLDCURSOR;

typedef OLDCURSOR UNALIGNED * UPOLDCURSOR;

DWORD HowManyColors(IN UPBITMAPINFOHEADER upbih, IN BOOL fOldFormat, OUT OPTIONAL LPBYTE * ppColorTable)
{
#define upbch ((UPBITMAPCOREHEADER)upbih)
	if(fOldFormat) {
		if(ppColorTable) {
			*ppColorTable = (LPBYTE)(upbch + 1);
	}
	if(upbch->bcBitCount <= 8)
		return (1 << upbch->bcBitCount);
	} 
	else {
		if(ppColorTable) {
			*ppColorTable = (LPBYTE)(upbih + 1);
		}
		if(upbih->biClrUsed)
			return (DWORD)upbih->biClrUsed;
		else if(upbih->biBitCount <= 8)
			return (1 << upbih->biBitCount);
		else if((upbih->biBitCount == 16) || (upbih->biBitCount == 32))
			return 3;
	}
	return 0;
#undef upbch
}
// 
// Checks to see if a DIB colro table is truly monochrome.  ie: the color table has black & white entries only.
// 
BOOL TrulyMonochrome(LPVOID lpColorTable, BOOL fOldFormat)
{
#define lpRGB  ((UNALIGNED LONG *)lpColorTable)
#define lpRGBw ((UNALIGNED WORD *)lpColorTable)
	if(fOldFormat) {
		// Honey - its triplets.
		if(lpRGBw[0] == 0x0000)
			return (lpRGBw[1] == 0xFF00) && (lpRGBw[2] == 0xFFFF);
		else if(lpRGBw[0] == 0xFFFF)
			return (lpRGBw[1] == 0x00FF) && (lpRGBw[2] == 0x0000);
	} 
	else {
		// Honey - its quadruplets!
		if(lpRGB[0] == RESCLR_BLACK)
			return (lpRGB[1] == RESCLR_WHITE);
		else if(lpRGB[0] == RESCLR_WHITE)
			return (lpRGB[1] == RESCLR_BLACK);
	}
#undef lpRGB
#undef lpRGBw
	return FALSE;
}
/***************************************************************************\
* CopyDibHdr
*
* Copies and converts a DIB resource header
*
* Handles conversion of OLDICON, OLDCURSOR and BITMAPCOREHEADER
* structures to BITMAPINFOHEADER headers.
*
* Note: fSingleHeightMasks is set for OLDICON and OLDCURSOR formats.
*       This identifies that a monochrome AND/Color mask
*       is NOT double height as it is in the newer formats.
*
* NOTE:  On the off chance that LR_LOADTRANSPARENT is used, we want to
*     copy a DWORD of the bits.  Since DIB bits are DWORD aligned, we know
*     at least a DWORD is there, even if the thing is a 1x1 mono bmp.
*
* The returned buffer is allocated in this function and needs to be
* freed by the caller.
*
* 22-Oct-1995 SanfordS  Revised
\***************************************************************************/
LPBITMAPINFOHEADER CopyDibHdr(IN UPBITMAPINFOHEADER upbih, OUT LPSTR * lplpBits, OUT LPBOOL lpfMono)
{
#define upbch ((UPBITMAPCOREHEADER)upbih)
	DWORD              cColors;
	DWORD              i;
	LPBITMAPINFOHEADER lpbihNew;
	DWORD              cbAlloc;
	LPBYTE             lpColorTable;
	struct {
		BITMAPINFOHEADER bih;
		DWORD  rgb[256];
		DWORD  dwBuffer;
	} Fake;
	switch(upbih->biSize) {
		case sizeof(BITMAPINFOHEADER):
			// Cool.  No conversion needed.
			cColors   = HowManyColors(upbih, FALSE, &lpColorTable);
			*lplpBits = (LPSTR)(((LPDWORD)lpColorTable) + cColors);
			break;
		case sizeof(BITMAPCOREHEADER):
			// Convert the BITMAPCOREHEADER to a BITMAPINFOHEADER
			Fake.bih.biSize  = sizeof(BITMAPINFOHEADER);
			Fake.bih.biWidth = upbch->bcWidth;
			Fake.bih.biHeight        = upbch->bcHeight;
			Fake.bih.biPlanes        = upbch->bcPlanes;
			Fake.bih.biBitCount      = upbch->bcBitCount;
			Fake.bih.biCompression   =
			Fake.bih.biXPelsPerMeter =
			Fake.bih.biYPelsPerMeter =
			Fake.bih.biClrImportant  = 0;
			Fake.bih.biClrUsed       = cColors = HowManyColors(upbih, TRUE, &lpColorTable);
			Fake.bih.biSizeImage     = BitmapWidth(Fake.bih.biWidth, Fake.bih.biBitCount) * Fake.bih.biHeight;
			// Copy and convert tripplet color table to rgbQuad color table.
			for(i = 0; i < cColors; i++, lpColorTable += 3) {
				Fake.rgb[i] = lpColorTable[0] + (lpColorTable[1] << 8) + (lpColorTable[2] << 16);
			}
			Fake.rgb[i] = *(DWORD UNALIGNED *)lpColorTable;  // For LR_LOADTRANSPARENT
			upbih       = (UPBITMAPINFOHEADER)&Fake;
			*lplpBits   = (LPSTR)lpColorTable;
			break;
		default:
#define upOldIcoCur ((UPOLDCURSOR)upbih)
			if(upOldIcoCur->bType == BMR_ICON || upOldIcoCur->bType == BMR_CURSOR) {
				// Convert OLDICON/OLDCURSOR header to BITMAPINFHEADER
				//RIPMSG0(RIP_WARNING, "USER32:Converting a OLD header. - email sanfords if you see this");
				Fake.bih.biSize  = sizeof(BITMAPINFOHEADER);
				Fake.bih.biWidth = upOldIcoCur->cx;
				Fake.bih.biHeight        = upOldIcoCur->cy * 2;
				Fake.bih.biPlanes        =
				Fake.bih.biBitCount      = 1;
				Fake.bih.biCompression   =
				Fake.bih.biXPelsPerMeter =
				Fake.bih.biYPelsPerMeter =
				Fake.bih.biClrImportant  = 0;
				Fake.bih.biClrUsed       = cColors = BPP01_MAXCOLORS;
				Fake.bih.biSizeImage     = BitmapWidth(upOldIcoCur->cx, 1) * upOldIcoCur->cy;
				Fake.rgb[0] = RESCLR_BLACK;
				Fake.rgb[1] = RESCLR_WHITE;
				upbih       = (LPBITMAPINFOHEADER)&Fake;
				*lplpBits   = (LPSTR)upOldIcoCur->abBitmap;
				Fake.rgb[2] = *((LPDWORD)*lplpBits);  // For LR_LOADTRANSPARENT
			} 
			else {
				//RIPMSG0(RIP_WARNING, "ConvertDIBBitmap: not a valid format");
				return NULL;
			}
#undef pOldIcoCur
			break;
	}
	*lpfMono = (cColors == BPP01_MAXCOLORS) && TrulyMonochrome((LPBYTE)upbih + sizeof(BITMAPINFOHEADER), FALSE);
	cbAlloc = sizeof(BITMAPINFOHEADER) + (cColors * sizeof(RGBQUAD)) + 4;
	if(lpbihNew = UserLocalAlloc(0, cbAlloc))
		RtlCopyMemory(lpbihNew, upbih, cbAlloc);
	return lpbihNew;
#undef upbch
}
// 
// Given a DIB, processes LR_MONOCHROME, LR_LOADTRANSPARENT and
// LR_LOADMAP3DCOLORS flags on the given header and colortable.
// 
VOID ChangeDibColors(IN LPBITMAPINFOHEADER lpbih, IN UINT LR_flags)
{
	LPDWORD lpColorTable;
	DWORD  rgb;
	UINT   iColor;
	UINT cColors = HowManyColors(lpbih, FALSE, (LPBYTE *)&lpColorTable);
	/*
	* NT Bug 366661: Don't check the color count here b/c we will do different
	* things depending on what type of change we are performing.  For example,
	* when loading hi-color/true-color icons, we always need to do the 
	* monochrome conversion in order to properly get an icon-mask.
	*/
	// 
	// LR_MONOCHROME is the only option that handles PM dibs.
	// 
	if(LR_flags & LR_MONOCHROME) {
		/*
		* LR_MONOCHROME is the only option that handles PM dibs.
		*
		* DO THIS NO MATTER WHETHER WE HAVE A COLOR TABLE!  We need 
		* to do this for mono conversion and for > 8 BPP 
		* icons/cursors.  In CopyDibHdr, we already made a copy of 
		* the header big enough to hold 2 colors even on 16 and 24 BPP images.
		*/
		lpbih->biBitCount = lpbih->biPlanes = 1;
		lpColorTable[0] = RESCLR_BLACK;
		lpColorTable[1] = RESCLR_WHITE;
	} 
	else if(LR_flags & LR_LOADTRANSPARENT) {
		LPBYTE pb;
		// No color table!  Do nothing.
		if(cColors == 0) {
			//RIPMSG0(RIP_WARNING, "ChangeDibColors: DIB doesn't have a color table");
			return;
		}
		pb = (LPBYTE)(lpColorTable + cColors);
		// 
		// Change the first pixel's color table entry to RGB_WINDOW
		// Gosh, I love small-endian
		// 
		if(lpbih->biCompression == 0)
			iColor = (UINT)pb[0];
		else
			iColor = (UINT)(pb[0] == 0 ? pb[2] : pb[1]); // RLE bitmap, will start with cnt,clr  or  0,cnt,clr
		switch(cColors) {
			case BPP01_MAXCOLORS: iColor &= 0x01; break;
			case BPP04_MAXCOLORS: iColor &= 0x0F; break;
			case BPP08_MAXCOLORS: iColor &= 0xFF; break;
		}
		rgb = (LR_flags & LR_LOADMAP3DCOLORS ? SYSRGB(3DFACE) : SYSRGB(WINDOW));
		lpColorTable[iColor] = RGBX(rgb);
	} 
	else if(LR_flags & LR_LOADMAP3DCOLORS) {
		// 
		// Fix up the color table, mapping shades of grey to the current 3D colors.
		// 
		for(iColor = 0; iColor < cColors; iColor++) {
			switch(*lpColorTable & 0x00FFFFFF) {
				case RGBX(RGB(223, 223, 223)): rgb = SYSRGB(3DLIGHT); goto ChangeColor;
				case RGBX(RGB(192, 192, 192)): rgb = SYSRGB(3DFACE); goto ChangeColor;
				case RGBX(RGB(128, 128, 128)): rgb = SYSRGB(3DSHADOW);
					// NOTE: byte-order is different in DIBs than in RGBs
		ChangeColor:
					*lpColorTable = RGBX(rgb);
					break;
			}
			lpColorTable++;
		}
	}
}

/***************************************************************************\
* ConvertDIBBitmap
*
* This takes a BITMAPCOREHEADER, OLDICON, OLDCURSOR or BITMAPINFOHEADER DIB
* specification and creates a physical object from it.
* Handles Color fixups, DIB sections, color depth, and stretching options.
*
* Passes back: (if lplpbih is not NULL)
*   lplpbih = copy of given header converted to BITMAPINFOHEADER form.
*   lplpBits = pointer to next mask bits, or NULL if no second mask.
*   Caller must free lplpbih returned.
*
* If lplpBits is not NULL and points to a non-NULL value, it supplies
* the location of the DIB bits allowing the header to be from a different
* location.
*
* 04-Oct-1995 SanfordS  Recreated.
\***************************************************************************/

HBITMAP ConvertDIBBitmap(IN /*UPBITMAPINFOHEADER*/LPBITMAPINFOHEADER upbih, IN  DWORD cxDesired, IN  DWORD cyDesired, IN  UINT LR_flags,
    OUT OPTIONAL LPBITMAPINFOHEADER *lplpbih, IN OUT OPTIONAL LPSTR *lplpBits)
{
	LPBITMAPINFOHEADER lpbihNew;
	BOOL   fMono, fMonoGiven;
	BYTE   bPlanesDesired;
	BYTE   bppDesired;
	LPSTR   lpBits;
	HBITMAP hBmpRet;
	/*
	* Make a copy of the DIB-Header.  This returns a pointer
	* which was allocated, so it must be freed later.
	* The also converts the header to BITMAPINFOHEADER format.
	*/
	if((lpbihNew = CopyDibHdr(upbih, &lpBits, &fMono)) == NULL)
		return NULL;
	/*
	* When loading a DIB file, we may need to use a different
	* bits pointer.  See RtlRes.c/RtlLoadObjectFromDIBFile.
	*/
	if(lplpBits && *lplpBits)
		lpBits = *lplpBits;
	fMonoGiven = fMono;
	if(!fMono) {
		if(LR_flags & (LR_LOADTRANSPARENT | LR_LOADMAP3DCOLORS))
			ChangeDibColors(lpbihNew, LR_flags & ~LR_MONOCHROME);
		bPlanesDesired = gpsi->Planes;
		bppDesired     = gpsi->BitsPixel;
		fMono  = LR_flags & LR_MONOCHROME;
	}
	if(fMono) {
		bPlanesDesired =
		bppDesired     = 1;
	}
	// 
	// HACK area
	// 
	if(lplpbih) {
		// 
		// pass back the translated/copied header
		// 
		*lplpbih = lpbihNew;
		// 
		// When loading icon/cursors on a system with multiple monitors
		// with different color depths, always convert to VGA color.
		// 
		if(!fMono && !SYSMET(SAMEDISPLAYFORMAT)) {
			bPlanesDesired = 1;
			bppDesired = 4;
		}
		/*
		* Return a ponter to the bits following this set of bits
		* if there are any there.
		*
		* Note that the header given with an ICON DIB always reflects
		* twice the height of the icon desired but the COLOR bitmap
		* (if there is one) will only be half that high.  We need to
		* fixup cyDesired for monochrome icons so that the mask isnt
		* stretched to half the height its supposed to be.  Color
		* bitmaps, however, must have the header corrected to reflect
		* the bits actual height which is half what the header said.
		* The correction must later be backed out so that the returned
		* header reflects the dimensions of the XOR mask that immediately
		* follows the color mask.
		*/
		if(fMonoGiven) {
			*lplpBits = NULL;
			if(cyDesired)
				cyDesired <<= 1;    // mono icon bitmaps are double high.
		} 
		else {
			//UserAssert(!(lpbihNew->biHeight & 1));
			lpbihNew->biHeight >>= 1;  // color icon headers are off by 2
			// 
			// Gross calculation!  We subtract the XOR part of the mask
			// for this calculation so that we submit a double-high mask.
			// The first half of this is garbage, but for icons its not
			// used.  This may be a bug for cursor use of icons.
			// 
			*lplpBits = lpBits + (BitmapWidth(lpbihNew->biWidth, lpbihNew->biBitCount) - BitmapWidth(lpbihNew->biWidth, 1)) * lpbihNew->biHeight;
		}
	}
	if(cxDesired == 0)
		cxDesired = lpbihNew->biWidth;
	if(cyDesired == 0)
		cyDesired = lpbihNew->biHeight;
	hBmpRet = BitmapFromDIB(cxDesired, cyDesired, bPlanesDesired, bppDesired, LR_flags, lpbihNew->biWidth, lpbihNew->biHeight, lpBits, (LPBITMAPINFO)lpbihNew, NULL);
	if(lplpbih == NULL || hBmpRet == NULL) {
		UserLocalFree(lpbihNew);
	} 
	else if(!fMonoGiven) {
		lpbihNew->biHeight <<= 1;   // restore header for next mask
	}
	return hBmpRet;
}


HANDLE ObjectFromDIBResource(HINSTANCE hmod, LPCWSTR lpName, LPWSTR type, DWORD cxDesired, DWORD cyDesired, UINT LR_flags)
{
	HANDLE  hObj = NULL;
	if(LR_flags & LR_LOADFROMFILE) {
		hObj = RtlLoadObjectFromDIBFile(lpName, type, cxDesired, cyDesired, LR_flags);
	} 
	else {
		HANDLE hdib = LoadDIB(hmod, lpName, type, cxDesired, cyDesired, LR_flags);
		if(hdib) {
			LPBITMAPINFOHEADER lpbih;
			// 
			// We cast the resource-bits to a BITMAPINFOHEADER.  If the
			// resource is a CURSOR type, then there are actually two
			// WORDs preceeding the BITMAPINFOHDEADER indicating the
			// hot-spot.  Be careful in assuming you have a real dib in this case.
			// 
			if(lpbih = (LPBITMAPINFOHEADER)LOCKRESOURCE(hdib, hmod)) {
				switch(PTR_TO_ID(type)) {
					case PTR_TO_ID(RT_BITMAP):
						// 
						// Create a physical bitmap from the DIB.
						// 
						hObj = ConvertDIBBitmap(lpbih, cxDesired, cyDesired, LR_flags, NULL, NULL);
						break;
					case PTR_TO_ID(RT_ICON):
					case PTR_TO_ID(RT_CURSOR):
					case PTR_TO_ID(RT_ANICURSOR):
					case PTR_TO_ID(RT_ANIICON):
						// 
						// Animated icon\cursors resources use the RIFF format
						// 
						if(ISRIFFFORMAT(lpbih)) {
							hObj = LoadCursorIconFromResource((PBYTE)lpbih, lpName, cxDesired, cyDesired, LR_flags);
						} 
						else {
							// Create the object from the DIB.
							hObj = ConvertDIBIcon(lpbih, hmod, lpName, (type == RT_ICON), cxDesired, cyDesired, LR_flags);
						}
						break;
				}
				UNLOCKRESOURCE(hdib, hmod);
			}
			// 
			// DO THIS TWICE!  The resource compiler always makes icon images
			// (RT_ICON) in a group icon discardable, whether the group dude
			// is or not!  So the first free won't really free the thing;
			// it'll just set the ref count to 0 and let the discard logic go on its merry way.
			// 
			// We take care of shared guys, so we don't need this dib no more.
			// Don't need this DIB no more no more, no more no more no more don't need this DIB no more.
			// 
			SplFreeResource(hdib, hmod, LR_flags);
		}
	}
	return hObj;
}

HICON LoadIcoCur(HINSTANCE hmod, LPCWSTR pszResName, LPWSTR type, DWORD cxDesired, DWORD cyDesired, UINT LR_flags)
{
	HICON     hico;
	LPWSTR    pszModName;
	WCHAR     achModName[MAX_PATH];
	ConnectIfNecessary();
	/*
	* Setup module name and handles for lookup.
	*/
	if(hmod == NULL) {
		hmod = hmodUser;
		pszModName = szUSER32;
	} 
	else {
		WowGetModuleFileName(hmod, achModName, sizeof(achModName) / sizeof(WCHAR));
		pszModName = achModName;
	}
	if(LR_flags & LR_CREATEDIBSECTION)
		LR_flags = (LR_flags & ~LR_CREATEDIBSECTION) | LR_CREATEREALDIB;
	/*
	* Setup defaults.
	*/
	if((hmod == hmodUser) && !IS_PTR(pszResName)) {
		int      imapMax;
		LPMAPRES lpMapRes;
		/*
		* Map some old OEM IDs for people.
		*/
		if(type == RT_ICON) {
			static MAPRES MapOemOic[] = { {OCR_ICOCUR, OIC_WINLOGO, MR_FAILFOR40} };
			lpMapRes = MapOemOic;
			imapMax  = 1;
		} 
		else {
			static MAPRES MapOemOcr[] = { {OCR_ICON, OCR_ICON, MR_FAILFOR40}, {OCR_SIZE, OCR_SIZE, MR_FAILFOR40} };
			lpMapRes = MapOemOcr;
			imapMax  = 2;
		}
		while(--imapMax >= 0) {
			if(lpMapRes->idDisp == PTR_TO_ID(pszResName)) {
				if((lpMapRes->bFlags & MR_FAILFOR40) && GETAPPVER() >= VER40) {
					RIPMSG1(RIP_WARNING, "LoadIcoCur: Old ID 0x%x not allowed for 4.0 apps", PTR_TO_ID(pszResName));
					return NULL;
				}
				pszResName = MAKEINTRESOURCE(lpMapRes->idUser);
				break;
			}
			++lpMapRes;
		}
	}
	// 
	// Determine size of requested object.
	// 
	cxDesired = GetIcoCurWidth(cxDesired , (type == RT_ICON), LR_flags, 0);
	cyDesired = GetIcoCurHeight(cyDesired, (type == RT_ICON), LR_flags, 0);
	// 
	// See if this is a cached icon/cursor, and grab it if we have one already.
	// 
	if(LR_flags & LR_SHARED) {
		CURSORFIND cfSearch;
		/*
		* Note that win95 fails to load any USER resources unless
		* LR_SHARED is specified - so we do too.  Also, win95 will
		* ignore your cx, cy and LR_flag parameters and just give
		* you whats in the cache so we do too.
		* A shame but thats life...
		*
		* Setup search criteria.  Since this is a load, we will have
		* no source-cursor to lookup.  Find something respectable.
		*/
		cfSearch.hcur = (HCURSOR)NULL;
		cfSearch.rt   = PtrToUlong(type);
		if(hmod == hmodUser) {
			cfSearch.cx  = 0;
			cfSearch.cy  = 0;
			cfSearch.bpp = 0;
		} 
		else {
			cfSearch.cx  = cxDesired;
			cfSearch.cy  = cyDesired;
			/*
			* On NT we have a more strict cache-lookup.  By passing in (zero), we
			* will tell the cache-lookup to ignore the bpp.  This fixes a problem
			* in Crayola Art Studio where the coloring-book cursor was being created
			* as an invisible cursor.  This lookup is compatible with Win95.
			*/
			#if 0
			cfSearch.bpp = GetIcoCurBpp(LR_flags);
			#else
			cfSearch.bpp = 0;
			#endif
		}
		hico = FindExistingCursorIcon(pszModName, pszResName, &cfSearch);
		if(hico)
			goto IcoCurFound;
	}
#ifdef LATER // SanfordS
	/*
	* We need to handle the case where a configurable icon has been
	* loaded from some arbitrary module or file and someone now wants
	* to load the same thing in a different size or color content.
	*
	* A cheezier alternative is to just call CopyImage on what we
	* found.
	*/
	if(hmod == hmodUser) {
		hico = FindExistingCursorIcon(NULL, szUSER, type, pszResName, 0, 0, 0);
		if(hico) {
			/*
			* Find out where the original came from and load it.
			* This may require some redesign to remember the
			* filename that LR_LOADFROMFILE images came from.
			*/
			_GetIconInfo(....);
			return LoadIcoCur(....);
		}
	}
#endif
	hico = (HICON)ObjectFromDIBResource(hmod, pszResName, type, cxDesired, cyDesired, LR_flags);
IcoCurFound:
	return hico;
}
#endif // } 0
//
//
//
#if SLTEST_RUNNING // {

int Test_LCMS2(const char * pTestBedPath, const char * pOutputFileName, bool exhaustive); // @v10.9.7 (Экспериментальное внедрение тестирования библиотеки lcms2) 

SLTEST_R(lcms2)
{
#if _MSC_VER >= 1910
	SString temp_buf;
	SString testbed_path;
	(testbed_path = GetInputPath()).SetLastSlash().Cat("lcms2");
	(temp_buf = "lcms2").CatChar('-').Cat("test").CatChar('-').Cat(getcurdate_(), DATF_YMD|DATF_NODIV|DATF_CENTURY).Cat(getcurtime_(), TIMF_HMS|TIMF_NODIV).Dot().Cat("out");
	const char * p_out_file_name = MakeOutputFilePath(temp_buf);
	SLTEST_CHECK_Z(Test_LCMS2(testbed_path, p_out_file_name, true));	
#else
	;
#endif
	return CurrentStatus;
}

SLTEST_R(SDraw)
{
	int    ok = 1;
	SString temp_buf;
	SImageBuffer img_buf;
	SString input_file_name(MakeInputFilePath("test24.png"));
	THROW(SLTEST_CHECK_NZ(img_buf.Load(input_file_name)));
	{
		SPathStruc ps(input_file_name);
		ps.Ext = "webp";
		ps.Merge(SPathStruc::fNam|SPathStruc::fExt, temp_buf);
		SImageBuffer::StoreParam sp(SFileFormat::Webp);
		SFile out_file(MakeOutputFilePath(temp_buf), SFile::mWrite|SFile::mBinary);
		THROW(out_file.IsValid());
		THROW(SLTEST_CHECK_NZ(img_buf.Store(sp, out_file)));
	}
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
			THROW(SLTEST_CHECK_NZ(img_buf2.IsEq(img_buf)));
		}
		{
			THROW(SLTEST_CHECK_NZ(img_buf.Load(MakeInputFilePath("test10.jpg"))));
			SImageBuffer::StoreParam sp_jpeg(SFileFormat::Jpeg);
			sp_jpeg.Quality = 50;
			SFile out_file_jpeg(MakeInputFilePath("test10-out.jpg"), SFile::mWrite|SFile::mBinary);
			THROW(SLTEST_CHECK_NZ(img_buf.Store(sp_jpeg, out_file_jpeg)))
		}
	}
	CATCHZOK
	return CurrentStatus;
}

#endif // } SLTEST_RUNNING
	