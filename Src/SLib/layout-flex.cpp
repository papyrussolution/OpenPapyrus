// LAYOUT-FLEX.CPP
// Copyright (c) A.Sobolev 2020, 2021
//
// The code of Microsoft Flex is partialy used (https://github.com/xamarin/flex.git)
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See the LICENSE.txt file in the project root for the license information.
//
#include <slib-internal.h>
#pragma hdrstop

AbstractLayoutBlock::AbstractLayoutBlock()
{
	SetDefault();
}

AbstractLayoutBlock & AbstractLayoutBlock::SetDefault()
{
	Flags = 0;
	SzX = szUndef;
	SzY = szUndef;
	JustifyContent = alignStart;
	AlignContent = alignStretch;
	AlignItems = alignStretch;
	AlignSelf = alignAuto;
	GravityX = 0;
	GravityY = 0;
	Order = 0;
	Nominal.a.SetZero();
	Nominal.b.SetZero();
	Size.SetZero();
	Padding.a.SetZero();
	Padding.b.SetZero();
	Margin.a.SetZero();
	Margin.b.SetZero();
	GrowFactor = 0.0f;
	ShrinkFactor = 1.0f;
	Basis = 0.0f;
	AspectRatio = 0.0f;
	return *this;
}

int FASTCALL AbstractLayoutBlock::operator == (const AbstractLayoutBlock & rS) const { return IsEqual(rS); }
int FASTCALL AbstractLayoutBlock::operator != (const AbstractLayoutBlock & rS) const { return !IsEqual(rS); }

int FASTCALL AbstractLayoutBlock::IsEqual(const AbstractLayoutBlock & rS) const
{
	#define I(f) if(f != rS.f) return 0;
	I(Flags);
	I(SzX);            
	I(SzY);            
	I(JustifyContent); 
	I(AlignContent);   
	I(AlignItems);     
	I(AlignSelf);      
	I(GravityX);       
	I(GravityY);       
	I(Order);          
	I(Nominal);        
	I(Size);           
	I(Padding);        
	I(Margin);   
	I(GrowFactor);     
	I(ShrinkFactor);   
	I(Basis);          
	I(AspectRatio);    
	#undef I
	return 1;
}

int AbstractLayoutBlock::Validate() const
{
	int    ok = 1;
	if(SzX == szFixed && IsNominalFullDefinedX()) {
		if(!feqeps(Size.X, Nominal.Width(), 0.01))
			ok = 0;
	}
	if(SzX == szByContainer) {
		if(Size.X < 0.0f || Size.X > 1.0f)
			ok = 0;
	}
	if(SzY == szFixed && IsNominalFullDefinedY()) {
		if(!feqeps(Size.Y, Nominal.Height(), 0.01))
			ok = 0;
	}
	if(SzY == szByContainer) {
		if(Size.Y < 0.0f || Size.Y > 1.0f)
			ok = 0;
	}	
	if(!oneof4(GravityX, 0, SIDE_LEFT, SIDE_RIGHT, SIDE_CENTER))
		ok = 0;
	if(!oneof4(GravityY, 0, SIDE_TOP, SIDE_BOTTOM, SIDE_CENTER))
		ok = 0;
	return ok;
}

/*static*/uint32 AbstractLayoutBlock::GetSerializeSignature() { return 0x15DE0522U; }

int AbstractLayoutBlock::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	//const  uint32 valid_signature = 0x15DE0522U;
	const  uint32 valid_version = 0U;
	uint32 signature = AbstractLayoutBlock::GetSerializeSignature(); // Сигнатура для сериализации
	uint32 version = 0;   // Версия сериализации
	THROW(pSCtx->Serialize(dir, signature, rBuf));
	if(dir < 0) {
		THROW(signature == AbstractLayoutBlock::GetSerializeSignature());
	}
	THROW(pSCtx->Serialize(dir, version, rBuf));
	THROW(pSCtx->Serialize(dir, Flags, rBuf));
	THROW(pSCtx->Serialize(dir, SzX, rBuf));
	THROW(pSCtx->Serialize(dir, SzY, rBuf));
	THROW(pSCtx->Serialize(dir, JustifyContent, rBuf));
	THROW(pSCtx->Serialize(dir, AlignContent, rBuf));
	THROW(pSCtx->Serialize(dir, AlignItems, rBuf));
	THROW(pSCtx->Serialize(dir, AlignSelf, rBuf));
	THROW(pSCtx->Serialize(dir, GravityX, rBuf));
	THROW(pSCtx->Serialize(dir, GravityY, rBuf));
	THROW(pSCtx->Serialize(dir, Order, rBuf));
	THROW(pSCtx->Serialize(dir, Nominal, rBuf));
	THROW(pSCtx->Serialize(dir, Size, rBuf));
	THROW(pSCtx->Serialize(dir, Padding, rBuf));
	THROW(pSCtx->Serialize(dir, Margin, rBuf));
	THROW(pSCtx->Serialize(dir, GrowFactor, rBuf));
	THROW(pSCtx->Serialize(dir, ShrinkFactor, rBuf));
	THROW(pSCtx->Serialize(dir, Basis, rBuf));
	THROW(pSCtx->Serialize(dir, AspectRatio, rBuf));
	CATCHZOK
	return ok;
}

int AbstractLayoutBlock::GetSizeX(float * pS) const
{
	int   result = szInvalid;
	float s = 0.0f;
	if(SzX == szFixed) {
		s = Size.X;
		result = szFixed;
	}
	if(SzX == szByContainer) {
		if(Size.X > 0.0f && Size.X <= 1.0f)
			s = Size.X;
		result = SzX;
	}
	else if(oneof2(SzX, szByContent, szUndef))
		result = SzX;
	ASSIGN_PTR(pS, s);
	return result;
}

int AbstractLayoutBlock::GetSizeY(float * pS) const
{
	int   result = szInvalid;
	float s = 0.0f;
	if(SzY == szFixed) {
		s = Size.Y;
		result = szFixed;
	}
	else if(SzY == szByContainer) {
		if(Size.Y > 0.0f && Size.Y <= 1.0f)
			s = Size.Y;
		result = SzY;
	}
	else if(oneof2(SzY, szByContent, szUndef))
		result = SzY;
	ASSIGN_PTR(pS, s);
	return result;
}

int AbstractLayoutBlock::GetSizeByContainerX(float containerSize, float * pS) const
{
	int   ok = 0;
	float result_size = 0.0f;
	if(SzX == szByContainer) {
		if(containerSize > 0.0f) {
			if(Size.X > 0.0f && Size.X <= 1.0f) {
				result_size = (containerSize * Size.X);
				ok = 1;
			}
			else if(Size.X == 0.0f) {
				result_size = (containerSize);
				ok = 1;
			}
		}
	}
	if(ok)
		ASSIGN_PTR(pS, result_size);
	return ok;
}

int AbstractLayoutBlock::GetSizeByContainerY(float containerSize, float * pS) const
{
	int   ok = 0;
	float result_size = 0.0f;
	if(SzY == szByContainer) {
		if(containerSize > 0.0f) {
			if(Size.Y > 0.0f && Size.Y <= 1.0f) {
				result_size = (containerSize * Size.Y);
				ok = 1;
			}
			else if(Size.Y == 0.0f) {
				result_size = (containerSize);
				ok = 1;
			}
		}
	}
	if(ok)
		ASSIGN_PTR(pS, result_size);
	return ok;
}

SPoint2F AbstractLayoutBlock::CalcEffectiveSizeXY(float containerSizeX, float containerSizeY) const
{
	SPoint2F result;
	result.X = CalcEffectiveSizeX(containerSizeX);
	result.Y = CalcEffectiveSizeY(containerSizeY);
	if(result.X == 0.0f && AspectRatio > 0.0f && result.Y > 0.0f) {
		result.X = result.Y / AspectRatio;
	}
	else if(result.Y == 0.0f && AspectRatio > 0.0f && result.X > 0.0f) {
		result.Y = result.X * AspectRatio;
	}
	return result;
}

float AbstractLayoutBlock::CalcEffectiveSizeX(float containerSize) const
{
	float result = 0.0f;
	if(SzX == AbstractLayoutBlock::szFixed) {
		result = Size.X;
	}
	else if(GetSizeByContainerX(containerSize, &result)) {
		; // @todo Тут, вероятно, надо поля и отступы учесть
	}
	else if(SzX == AbstractLayoutBlock::szByContent) {
		result = 0.0f; // @todo
	}
	else {
		//
		// @todo Если установлен AspectRatio, то следует учесть вариант рассчитанного перед вызовом этой функции кросс-размера
		//
		if(SzY == AbstractLayoutBlock::szFixed && AspectRatio > 0.0f) {
			result = Size.Y / AspectRatio;
		}
		else {
			result = 0.0f; // @todo
		}
	}
	return result;
}

float AbstractLayoutBlock::CalcEffectiveSizeY(float containerSize) const
{
	float result = 0.0f;
	if(SzY == AbstractLayoutBlock::szFixed) {
		result = Size.Y;
	}
	else if(GetSizeByContainerY(containerSize, &result)) {
		; // @todo Тут, вероятно, надо поля и отступы учесть
	}
	else if(SzY == AbstractLayoutBlock::szByContent) {
		result = 0.0f; // @todo 
	}
	else {
		//
		// @todo Если установлен AspectRatio, то следует учесть вариант рассчитанного перед вызовом этой функции кросс-размера
		//
		if(SzX == AbstractLayoutBlock::szFixed && AspectRatio > 0.0f) {
			result = Size.X * AspectRatio;
		}
		else {
			result = 0.0f; // @todo 
		}
	}
	return result;
}

void AbstractLayoutBlock::SetFixedSizeX(float s)
{
	//assert(!fisnanf(s) && s >= 0.0f);
	if(fisnan(s) || s < 0.0f)
		s = 0.0f;
	Size.X = s;
	SzX = szFixed;
}

void AbstractLayoutBlock::SetVariableSizeX(uint var/* szXXX */, float s)
{
	assert(oneof3(var, szUndef, szByContent, szByContainer));
	if(var == szByContainer && (s > 0.0f && s <= 1.0f))
		Size.X = s;
	else if(var == szByContainer && s == 0.0f)
		Size.X = 1.0f;
	else
		Size.X = 0.0f;
	SzX = var;
}

void AbstractLayoutBlock::SetFixedSizeY(float s)
{
	//assert(!fisnanf(s) && s >= 0.0f);
	if(fisnanf(s) || s < 0.0f)
		s = 0.0f;
	Size.Y = s;
	SzY = szFixed;
}

void AbstractLayoutBlock::SetFixedSize(const TRect & rR)
{
	SetFixedSizeX(static_cast<float>(rR.width()));
	SetFixedSizeY(static_cast<float>(rR.height()));
}

void AbstractLayoutBlock::SetVariableSizeY(uint var/* szXXX */, float s)
{
	assert(oneof3(var, szUndef, szByContent, szByContainer));
	if(var == szByContainer && (s > 0.0f && s <= 1.0f))
		Size.Y = s;
	else if(var == szByContainer && s == 0.0f)
		Size.Y = 1.0f;
	else
		Size.Y = 0.0f;
	SzY = var;
}

int AbstractLayoutBlock::GetContainerDirection() const // returns DIREC_HORZ || DIREC_VERT || DIREC_UNKN
{
	if((Flags & (fContainerRow|fContainerCol)) == fContainerRow)
		return DIREC_HORZ;
	else if((Flags & (fContainerRow|fContainerCol)) == fContainerCol)
		return DIREC_VERT;
	else
		return DIREC_UNKN;
}

void AbstractLayoutBlock::SetContainerDirection(int direc /*DIREC_XXX*/)
{
	if(direc == DIREC_HORZ) {
		Flags &= ~fContainerCol;
		Flags |= fContainerRow;
	}
	else if(direc == DIREC_VERT) {
		Flags |= fContainerCol;
		Flags &= ~fContainerRow;
	}
	else {
		Flags &= ~(fContainerCol|fContainerRow);
	}
}

/*static*/SString & AbstractLayoutBlock::MarginsToString(const FRect & rR, SString & rBuf)
{
	rBuf.Z();
	if(!rR.IsEmpty()) {
		if(rR.a.X == rR.b.X && rR.a.Y == rR.b.Y) {
			if(rR.a.X == rR.a.Y)
				rBuf.Cat(rR.a.X);
			else
				rBuf.Cat(rR.a.X).Comma().Cat(rR.a.Y);
		}
		else
			rBuf.Cat(rR.a.X).Comma().Cat(rR.a.Y).Comma().Cat(rR.b.X).Comma().Cat(rR.b.Y);
	}
	return rBuf;
}

/*static*/int AbstractLayoutBlock::MarginsFromString(const char * pBuf, FRect & rR)
{
	int    ok = 1;
	SString temp_buf(pBuf);
	temp_buf.Strip();
	StringSet ss;
	temp_buf.Tokenize(",;", ss);
	const uint _c = ss.getCount();
	if(oneof4(_c, 0, 1, 2, 4)) {
		if(_c == 0) {
			rR.Z();
			ok = -1;
		}
		else {
			STokenRecognizer tr;
			SNaturalTokenArray nta;
			uint tokn = 0;
			for(uint ssp = 0; ok && ss.get(&ssp, temp_buf);) {
				temp_buf.Strip();
				tr.Run(temp_buf.ucptr(), temp_buf.Len(), nta, 0);
				if(nta.Has(SNTOK_NUMERIC_DOT)) {
					tokn++;
					float v = temp_buf.ToFloat();
					//const int iv = static_cast<int>(v);
					assert(oneof4(tokn, 1, 2, 3, 4));
					if(tokn == 1) {
						if(_c == 1) {
							 rR.Set(v);
						}
						else if(_c == 2) { 
							rR.a.X = v;
							rR.b.X = v;
						}
						else // _c == 4
							rR.a.X = v;
					}
					else if(tokn == 2) {
						if(_c == 2) {
							rR.a.Y = v;
							rR.b.Y = v;
						}
						else // _c == 4
							rR.a.Y = v;
					}
					else if(tokn == 3)
						rR.b.X = v;
					else if(tokn == 4)
						rR.b.Y = v;
				}
				else
					ok = 0;
			}
		}
	}
	else
		ok = 0;
	return ok;
}

SString & AbstractLayoutBlock::SizeToString(SString & rBuf) const
{
	rBuf.Z();
	SPoint2F s;
	int  szx = GetSizeX(&s.X);
	int  szy = GetSizeY(&s.Y);
	if(szx != szUndef || szy != szUndef) {
		switch(szx) {
			case szFixed: rBuf.Cat(s.X, MKSFMTD(0, 3, NMBF_NOTRAILZ)); break;
			case szUndef: rBuf.Cat("undef"); break;
			case szByContent: rBuf.Cat("content"); break;
			case szByContainer: 
				if(s.X > 0.0f && s.X <= 1.0f)
					rBuf.Cat(s.X * 100.0f, MKSFMTD(0, 3, NMBF_NOTRAILZ)).CatChar('%');
				else
					rBuf.Cat("parent"); 
				break;
			default:
				assert(szx == szInvalid);
				rBuf.Cat("invalid");
				break;
		}
		rBuf.CatDiv(',', 2);
		//
		switch(szy) {
			case szFixed: rBuf.Cat(s.Y, MKSFMTD(0, 3, NMBF_NOTRAILZ)); break;
			case szUndef: rBuf.Cat("undef"); break;
			case szByContent: rBuf.Cat("content"); break;
			case szByContainer: 
				if(s.Y > 0.0f && s.Y <= 1.0f)
					rBuf.Cat(s.Y * 100.0f, MKSFMTD(0, 3, NMBF_NOTRAILZ)).CatChar('%');
				else
					rBuf.Cat("parent"); break;
				break;
			default:
				assert(szy == szInvalid);
				rBuf.Cat("invalid");
				break;
		}
	}
	return rBuf;
}

int AbstractLayoutBlock::ParseSizeStr(const SString & rStr, float & rS) const
{
	rS = 0.0f;
	int    result = szUndef;
	SString temp_buf(rStr);
	if(temp_buf.NotEmptyS()) {
		if(rStr.IsEqiAscii("undef")) {
			result = szUndef;
		}
		else if(rStr.IsEqiAscii("invalid")) {
			result = szInvalid;
		}
		else if(rStr.IsEqiAscii("content")) {
			result = szByContent;
		}
		else if(rStr.IsEqiAscii("parent") || rStr.IsEqiAscii("container")) {
			result = szByContainer;
		}
		else {
			char first_c = temp_buf.C(0);
			if(isdec(first_c) || oneof4(first_c, '.', '-', 'e', 'E')) {
				const char * p_end = 0;
				int   erange = 0;
				double val = 0.0;
				if(SRealConversion::Scan(temp_buf.cptr(), &p_end, &val, &erange) && !fisnan(val)) {
					result = szFixed;
					rS = static_cast<float>(val);
					if(p_end) {
						while(*p_end && oneof2(*p_end, ' ', '\t'))
							p_end++;
						if(*p_end == '%') {
							if(val > 0.0 && val <= 100.0) {
								rS = static_cast<float>(val / 100.0);
								result = szByContainer;
							}
						}

					}
				}
				else {
					rS = 0.0f;
					result = szInvalid;
				}
			}
			else
				result = szInvalid;
		}
	}
	return result;
}

int AbstractLayoutBlock::SizeFromString(const char * pBuf)
{
	int    ok = 1;
	SString input(pBuf);
	input.Strip();
	SString x_buf, y_buf;
	if(input.Divide(',', x_buf, y_buf) > 0 || input.Divide(';', x_buf, y_buf) > 0 || input.Divide(' ', x_buf, y_buf) > 0) {
		SPoint2F s;
		int szx = ParseSizeStr(x_buf, s.X);
		int szy = ParseSizeStr(y_buf, s.Y);
		if(szx == szFixed)
			SetFixedSizeX(s.X);
		else if(szx != szInvalid)
			SetVariableSizeX(szx, s.X);
		else
			SetVariableSizeX(szUndef, 0.0f);
		if(szy == szFixed)
			SetFixedSizeY(s.Y);
		else if(szy != szInvalid)
			SetVariableSizeY(szy, s.Y);
		else
			SetVariableSizeY(szUndef, 0.0f);
	}
	else {
		float v;
		int szxy = ParseSizeStr(x_buf, v);
		if(szxy == szFixed) {
			SetFixedSizeX(v);
			SetFixedSizeY(v);
		}
		else if(szxy != szInvalid) {
			SetVariableSizeX(szxy, v);
			SetVariableSizeY(szxy, v);
		}
		else {
			SetVariableSizeX(szUndef, 0.0f);
			SetVariableSizeY(szUndef, 0.0f);			
		}
	}
	return ok;
}
//
// Descr: Определяет являются ли координаты по оси X фиксированными.
//
bool AbstractLayoutBlock::IsNominalFullDefinedX() const { return LOGIC((Flags & (fNominalDefL|fNominalDefR)) == (fNominalDefL|fNominalDefR)); }
//
// Descr: Определяет являются ли координаты по оси Y фиксированными.
//
bool AbstractLayoutBlock::IsNominalFullDefinedY() const { return LOGIC((Flags & (fNominalDefT|fNominalDefB)) == (fNominalDefT|fNominalDefB)); }
//
// Descr: Вспомогательная функция, возвращающая кросс-направление относительно заданного
//   направления direction.
//   Если direction == DIREC_HORZ, то возвращает DIREC_VERT; если direction == DIREC_VERT, то возвращает DIREC_HORZ.
//   Если !oneof2(direction, DIREC_HORZ, DIREC_VERT) то возвращает DIREC_UNKN.
//
/*static*/int AbstractLayoutBlock::GetCrossDirection(int direction)
{
	assert(oneof2(direction, DIREC_HORZ, DIREC_VERT));
	return (direction == DIREC_HORZ) ? DIREC_VERT : ((direction == DIREC_VERT) ? DIREC_HORZ : DIREC_UNKN);
}
//
// Descr: Определяет является ли позиция элемента абсолютной вдоль направления direction.
// ARG(direction IN): DIREC_HORZ || DIREC_VERT
//
bool AbstractLayoutBlock::IsPositionAbsolute(int direction) const
{
	assert(oneof2(direction, DIREC_HORZ, DIREC_VERT));
	return (direction == DIREC_HORZ) ? IsPositionAbsoluteX() : IsPositionAbsoluteY();
}
//
// Descr: Определяет является ли позиция элемента по оси Y абсолютной.
//   Понятие "абсолютная позиция по оси" подразумевает, что либо заданы фиксированные 
//   начальная и конечная координаты по оси, либо размер элемента по оси фиксирован (Sz(X|Y)==szFixed) и фиксирована
//   хотя бы одна из координат по оси.
//
bool AbstractLayoutBlock::IsPositionAbsoluteX() const
{
	bool result = false;
	if(SzX == szFixed) {
		if(Flags & (fNominalDefL|fNominalDefR))
			result = true;
	}
	else if(IsNominalFullDefinedX())
		result = true;
	return result;
}

float AbstractLayoutBlock::GetAbsoluteLowX() const
{
	float result = 0.0f;
	if(Flags & fNominalDefL)
		result = Nominal.a.X;
	else if(Flags & fNominalDefR && SzX == szFixed)
		result = (Nominal.b.X - Size.X);
	return result;
}

float AbstractLayoutBlock::GetAbsoluteLowY() const
{
	float result = 0.0f;
	if(Flags & fNominalDefT)
		result = Nominal.a.Y;
	else if(Flags & fNominalDefB && SzY == szFixed)
		result = (Nominal.b.Y - Size.Y);
	return result;
}

float AbstractLayoutBlock::GetAbsoluteSizeX() const
{
	float result = 0.0f;
	if(SzX == szFixed)
		result = Size.X;
	else if(IsNominalFullDefinedX())
		result = Nominal.Width();
	return result;
}

float AbstractLayoutBlock::GetAbsoluteSizeY() const
{
	float result = 0.0f;
	if(SzY == szFixed)
		result = Size.Y;
	else if(IsNominalFullDefinedY())
		result = Nominal.Height();
	return result;
}
//
// Descr: Определяет является ли позиция элемента по оси Y абсолютной.
//   Понятие "абсолютная позиция по оси" подразумевает, что либо заданы фиксированные 
//   начальная и конечная координаты по оси, либо размер элемента по оси фиксирован (Sz(X|Y)==szFixed) и фиксирована
//   хотя бы одна из координат по оси.
//
bool AbstractLayoutBlock::IsPositionAbsoluteY() const
{
	bool result = false;
	if(SzY == szFixed) {
		if(Flags & (fNominalDefT|fNominalDefB))
			result = true;
	}
	else if(IsNominalFullDefinedY())
		result = true;
	return result;
}
//
//
//
LayoutFlexItem::Result::Result() : Flags(0), P_Scrlr(0)
{
	memzero(Frame, sizeof(Frame));
}

LayoutFlexItem::Result::Result(const Result & rS) : Flags(rS.Flags), P_Scrlr(0)
{
	memcpy(Frame, rS.Frame, sizeof(Frame));
	if(rS.P_Scrlr) {
		P_Scrlr = new SScroller(*rS.P_Scrlr);
	}
}

LayoutFlexItem::Result::~Result()
{
	delete P_Scrlr;
}

LayoutFlexItem::Result & FASTCALL LayoutFlexItem::Result::operator = (const Result & rS)
{
	memcpy(Frame, rS.Frame, sizeof(Frame));
	Flags = rS.Flags;
	if(rS.P_Scrlr) {
		P_Scrlr = new SScroller(*rS.P_Scrlr);
	}
	else {
		ZDELETE(P_Scrlr);
	}
	return *this;
}

LayoutFlexItem::Result::operator FRect() const
{
	return FRect(Frame[0], Frame[1], Frame[0] + Frame[2], Frame[1] + Frame[3]);
}

LayoutFlexItem::Result & LayoutFlexItem::Result::CopyWithOffset(const LayoutFlexItem::Result & rS, float offsX, float offsY)
{
	Frame[0] = rS.Frame[0] + offsX;
	Frame[1] = rS.Frame[1] + offsY;
	Frame[2] = rS.Frame[2];
	Frame[3] = rS.Frame[3];
	Flags = rS.Flags;
	if(rS.P_Scrlr) {
		P_Scrlr = new SScroller(*rS.P_Scrlr);
	}
	else {
		ZDELETE(P_Scrlr);
	}
	return *this;
}

/*static*/void * LayoutFlexItem::GetManagedPtr(LayoutFlexItem * pItem)
{
	return pItem ? pItem->managed_ptr : 0;
}

/*static*/void * LayoutFlexItem::GetParentsManagedPtr(LayoutFlexItem * pItem)
{
	return pItem ? GetManagedPtr(pItem->P_Parent) : 0;
}

LayoutFlexItem::LayoutFlexItem() : P_Parent(0), P_Link(0), managed_ptr(0), CbSelfSizing(0), CbSetup(0), State(0), ALB(), P_HgL(0), P_Children(0)
{
}

LayoutFlexItem::~LayoutFlexItem()
{
	ZDELETE(P_Children);
	ZDELETE(P_HgL);
	managed_ptr = 0; // @v11.0.0
	CbSelfSizing = 0; // @v11.0.0
	CbSetup = 0; // @v11.0.0
	P_Link = 0;
}

int LayoutFlexItem::FatherKillMe()
{
	int    ok = 0;
	if(P_Parent) {
		const uint _pcc = P_Parent->GetChildrenCount();
		for(uint i = 0; !ok && i < _pcc; i++) {
			const LayoutFlexItem * p_sibl = P_Parent->GetChildC(i);
			if(p_sibl == this) {
				P_Parent->P_Children->atFree(i);
				ok = 1;
			}
		}
		assert(ok); // Если !ok то значит this имеет родителя, который о this ничего не знает.
	}
	return ok;
}

uint LayoutFlexItem::GetChildrenCount() const
{
	return SVectorBase::GetCount(P_Children);
}

LayoutFlexItem * LayoutFlexItem::GetChild(uint idx)
{
	return (idx < SVectorBase::GetCount(P_Children)) ? P_Children->at(idx) : 0;
}

const  LayoutFlexItem * LayoutFlexItem::GetChildC(uint idx) const
{
	return (idx < SVectorBase::GetCount(P_Children)) ? P_Children->at(idx) : 0;
}

void LayoutFlexItem::SetCallbacks(FlexSelfSizingProc selfSizingCb, FlexSetupProc setupCb, void * managedPtr)
{
	CbSelfSizing = selfSizingCb;
	CbSetup = setupCb;
	managed_ptr = managedPtr;
}

int LayoutFlexItem::SetLayoutBlock(const AbstractLayoutBlock & rAlb)
{
	int    ok = 1;
	int    vr = rAlb.Validate();
	assert(vr);
	if(vr)
		ALB = rAlb;
	else
		ok = 0;
	return ok;
}

int LayoutFlexItem::GetOrder() const
{
	return ALB.Order;
}

void LayoutFlexItem::SetOrder(int o)
{
	ALB.Order = o;
	UpdateShouldOrderChildren();
}
//
// Descr: Вычисляет полную ширину элемента без рассмотрения его внутренних компонентов.
//   Полная ширина включает собственно ширину, а так же левые и правые поля и набивки
//   (margin_left, margin_right, padding_left, padding_right).
// Returns:
//   !0 - номинальная ширина элемента представлена валидным числом (!fisnan(width)). 
//      В этом случае по адресу pS присваивается полная ширина элемента.
//    0 - номинальная ширина элемента представлена инвалидным значением (fisnan(width)).
//      В этом случае по адресу pS ничего не присваивается и значение по указателю остается неизменным.
//
int LayoutFlexItem::GetFullWidth(float * pS) const
{
	int    ok = 1;
	float  s = 0.0f;
	//if(fisnanf(Size.X))
	if(ALB.SzX != AbstractLayoutBlock::szFixed)
		ok = 0;
	else {
		s += ALB.Size.X;
		s += ALB.Margin.a.X;
		s += ALB.Margin.b.X;
		s += ALB.Padding.a.X;
		s += ALB.Padding.b.X;
		ASSIGN_PTR(pS, s);
	}
	return ok;
}
//
// Descr: Вычисляет полную высоту элемента без рассмотрения его внутренних компонентов.
//   Полная высота включает собственно ширину, а так же верхние и нижние поля и набивки
//   (margin_top, margin_bottom, padding_top, padding_bottom).
// Returns:
//   !0 - номинальная высота элемента представлена валидным числом (!fisnan(height)). 
//      В этом случае по адресу pS присваивается полная высота элемента.
//    0 - номинальная высота элемента представлена инвалидным значением (fisnan(height)).
//      В этом случае по адресу pS ничего не присваивается и значение по указателю остается неизменным.
//
int LayoutFlexItem::GetFullHeight(float * pS) const
{
	int    ok = 1;
	float  s = 0.0f;
	//if(fisnanf(Size.Y))
	if(ALB.SzY != AbstractLayoutBlock::szFixed)
		ok = 0;
	else {
		s += ALB.Size.Y;
		s += ALB.Margin.a.Y;
		s += ALB.Margin.b.Y;
		s += ALB.Padding.a.Y;
		s += ALB.Padding.b.Y;
		ASSIGN_PTR(pS, s);
	}
	return ok;
}
//
// Descr: Возвращает финальный расчетный прямоугольник элемента.
//
FRect LayoutFlexItem::GetFrame() const
{
	return R;
}

int LayoutFlexItem::GetInnerCombinedFrame(FRect * pResult) const
{
	int    ok = 0;
	FRect  rf;
	const  uint _cc = GetChildrenCount();
	if(_cc) {
		bool is_first = true;
		for(uint i = 0; i < _cc; i++) {
			const LayoutFlexItem * p_child = GetChildC(i);
			assert(p_child);
			if(p_child) {
				if(is_first) {
					rf = p_child->GetFrame();
					is_first = false;
					ok = 1;
				}
				else {
					rf.Union(p_child->GetFrame());
				}
			}
		}
	}
	else
		ok = -1;
	ASSIGN_PTR(pResult, rf);
	return ok;
}
//
// Descr: Возвращает корневой элемент дерева, компонентом которого является this.
//
LayoutFlexItem * LayoutFlexItem::GetRoot()
{
	LayoutFlexItem * p_root = this;
	while(p_root->P_Parent) {
		p_root = p_root->P_Parent;
	}
	return p_root;
}

LayoutFlexItem * LayoutFlexItem::InsertItem()
{
	LayoutFlexItem * p_result_item = 0;
	if(!P_Children) {
		P_Children = new TSCollection <LayoutFlexItem>;
	}
	if(P_Children) {
		p_result_item = P_Children->CreateNewItem();
		if(p_result_item) {
			p_result_item->P_Parent = this;
			p_result_item->UpdateShouldOrderChildren();		
		}
	}
	return p_result_item;
}

void LayoutFlexItem::DeleteItem(uint idx)
{
	const uint _cc = GetChildrenCount();
	assert(idx < _cc);
	assert(_cc);
	if(idx < _cc) {
		LayoutFlexItem * p_child = GetChild(idx);
		p_child->P_Parent = 0;
		P_Children->atFree(idx);
	}
}

void LayoutFlexItem::DeleteAllItems()
{
	uint i = GetChildrenCount();
	if(i) do {
		DeleteItem(--i);
	} while(i);
}

LayoutFlexItem::IndexEntry::IndexEntry() : ItemIdx(0), HglIdx(0)
{
}

LayoutFlexItem::IterIndex::IterIndex() : TSVector <IndexEntry>()
{
}

LayoutFlexItem * LayoutFlexItem::IterIndex::GetHomogeneousItem(const LayoutFlexItem * pBaseItem, uint hgeIdx) const
{
	assert(pBaseItem);
	LayoutFlexItem * p_new_item = HomogeneousEntryPool.CreateNewItem();
	if(p_new_item) {
		if(pBaseItem) {
			const HomogeneousEntry & r_hge = pBaseItem->P_HgL->at(hgeIdx);
			p_new_item->SetLayoutBlock(pBaseItem->GetLayoutBlockC());
			switch(pBaseItem->P_HgL->VariableFactor) {
				case HomogeneousArray::vfFixedSizeX:
					p_new_item->ALB.SetFixedSizeX(r_hge.Vf);
					break;
				case HomogeneousArray::vfFixedSizeY:
					p_new_item->ALB.SetFixedSizeY(r_hge.Vf);
					break;
				case HomogeneousArray::vfGrowFactor:
					p_new_item->ALB.GrowFactor = r_hge.Vf;
					break;
				default:
					break;
			}
			p_new_item->State |= stHomogeneousItem;
			p_new_item->R = pBaseItem->R;
		}
	}
	return p_new_item;
}

void LayoutFlexItem::IterIndex::ReleaseHomogeneousItem(LayoutFlexItem * pItem) const
{
	bool   found = false;
	uint   i = HomogeneousEntryPool.getCount();
	if(i) do {
		LayoutFlexItem * p = HomogeneousEntryPool.at(--i);
		if(p == pItem) {
			HomogeneousEntryPool.atFree(i);
			found = true;
		}
	} while(i && !found);
	assert(found);
}

static IMPL_CMPFUNC(LayoutFlexItemIndexEntry, i1, i2)
{
	const LayoutFlexItem::IndexEntry * p1 = static_cast<const LayoutFlexItem::IndexEntry *>(i1);
	const LayoutFlexItem::IndexEntry * p2 = static_cast<const LayoutFlexItem::IndexEntry *>(i2);
	const LayoutFlexItem * p_extra = static_cast<const LayoutFlexItem *>(pExtraData);
	assert(p1 && p2 && p_extra);
	const uint _cc = p_extra->GetChildrenCount();
	assert(p1->ItemIdx < _cc);
	assert(p2->ItemIdx < _cc);
	const int ord1 = p_extra->GetChildC(p1->ItemIdx)->GetOrder();
	const int ord2 = p_extra->GetChildC(p2->ItemIdx)->GetOrder();
	int   si = CMPSIGN(ord1, ord2);
	if(!si) {
		CMPCASCADE2(si, p1, p2, ItemIdx, HglIdx);
	}
	return si;
}

// @construction
void LayoutFlexItem::MakeIndex(IterIndex & rIndex) const
{
	rIndex.clear();
	const uint _cc = GetChildrenCount();
	for(uint i = 0; i < _cc; i++) {
		const LayoutFlexItem * p_child = GetChildC(i);
		if(p_child) {
			if(SVectorBase::GetCount(p_child->P_HgL)) {
				for(uint j = 0; j < p_child->P_HgL->getCount(); j++) {
					IndexEntry new_entry;
					new_entry.ItemIdx = i;
					new_entry.HglIdx = j+1;
					rIndex.insert(&new_entry);
				}
			}
			else {
				IndexEntry new_entry;
				new_entry.ItemIdx = i;
				new_entry.HglIdx = 0;
				rIndex.insert(&new_entry);
			}
		}
	}
	if(rIndex.getCount() > 1) {
		rIndex.sort(PTR_CMPFUNC(LayoutFlexItemIndexEntry), const_cast<LayoutFlexItem *>(this)); // @badcase See IMPL_CMPFUNC(LayoutFlexItemIndexEntry)
	}
}

// @construction {
const LayoutFlexItem * LayoutFlexItem::GetChildByIndex(const IterIndex & rIndex, uint idxPos) const
{
	const LayoutFlexItem * p_result = 0;
	if(idxPos < rIndex.getCount()) {
		const IndexEntry & r_ie = rIndex.at(idxPos);
		const uint _cc = GetChildrenCount();
		assert(r_ie.ItemIdx < _cc);
		if(r_ie.ItemIdx < _cc) {
			const LayoutFlexItem * p_inner_item = GetChildC(r_ie.ItemIdx);
			assert(p_inner_item);
			const uint hglc = SVectorBase::GetCount(p_inner_item->P_HgL);
			if(hglc) {
				assert(r_ie.HglIdx > 0 && r_ie.HglIdx <= hglc);	
				if(r_ie.HglIdx > 0 && r_ie.HglIdx <= hglc) {
					p_result = rIndex.GetHomogeneousItem(p_inner_item, r_ie.HglIdx-1);
					assert(p_result);
				}
			}
			else {
				assert(r_ie.HglIdx == 0);
				if(r_ie.HglIdx == 0) {
					p_result = p_inner_item;
				}
			}
		}
	}
	return p_result;
}

// @construction
int LayoutFlexItem::CommitChildResult(const IterIndex & rIndex, uint idxPos, const LayoutFlexItem * pItem)
{
	int    ok = 0;
	if(idxPos < rIndex.getCount()) {
		const IndexEntry & r_ie = rIndex.at(idxPos);
		const uint _cc = GetChildrenCount();
		assert(r_ie.ItemIdx < _cc);
		if(r_ie.ItemIdx < _cc) {
			const LayoutFlexItem * p_inner_item = GetChildC(r_ie.ItemIdx);
		}
	}
	return ok;
}
// } @construction 

class LayoutFlexProcessor {
public:
	static uint DetermineFlags(const LayoutFlexItem * pItem)
	{
		uint   flags = 0;
		if(pItem) {
			if(pItem->ALB.GetContainerDirection() == DIREC_VERT)
				flags |= fVertical;
			if(pItem->ALB.Flags & AbstractLayoutBlock::fContainerReverseDir)
				flags |= fReverse;
			if(pItem->ALB.Flags & AbstractLayoutBlock::fContainerWrap) {
				flags |= fWrap;
				if(pItem->ALB.Flags & AbstractLayoutBlock::fContainerWrapReverse)
					flags |= fReverse2;
				// @v11.0.0 @experimental if(pItem->ALB.AlignContent != AbstractLayoutBlock::alignStart)
					flags |= fNeedLines;
			}
		}
		return flags;
	}
	LayoutFlexProcessor(const LayoutFlexItem * pItem, const LayoutFlexItem::Param & rP) : Flags(DetermineFlags(pItem)),
		SizeDim(0.0f), AlignDim(0.0f), FramePos1i(0), FramePos2i(0), FrameSz1i(0), FrameSz2i(0),
		ordered_indices(0), LineDim(0.0f), FlexDim(0.0f), ExtraFlexDim(0.0f), FlexGrows(0.0f), FlexShrinks(0.0f),
		Pos2(0.0f), P(rP)
	{
		assert(pItem);
		assert(pItem->ALB.Padding.a.X >= 0.0f);
		assert(pItem->ALB.Padding.b.X >= 0.0f);
		assert(pItem->ALB.Padding.a.Y >= 0.0f);
		assert(pItem->ALB.Padding.b.Y >= 0.0f);
		const float __width  = smax(0.0f, rP.ForceWidth  - (pItem->ALB.Padding.a.X + pItem->ALB.Padding.b.X));
		const float __height = smax(0.0f, rP.ForceHeight - (pItem->ALB.Padding.a.Y + pItem->ALB.Padding.b.Y));
		// (smax above) assert(__width >= 0.0f);
		// (smax above) assert(__height >= 0.0f);
		switch(pItem->ALB.GetContainerDirection()) {
			case DIREC_HORZ:
				SizeDim  = __width;
				AlignDim = __height;
				FramePos1i   = 0;
				FramePos2i  = 1;
				FrameSz1i  = 2;
				FrameSz2i = 3;
				break;
			case DIREC_VERT:
				SizeDim  = __height;
				AlignDim = __width;
				FramePos1i   = 1;
				FramePos2i  = 0;
				FrameSz1i  = 3;
				FrameSz2i = 2;
				break;
			default:
				//assert(false && "incorrect direction");
				break;
		}
		const uint _item_cc = pItem->GetChildrenCount();
		if((pItem->State & LayoutFlexItem::stShouldOrderChildren) && _item_cc) {
			uint * p_indices = static_cast<uint *>(SAlloc::M(sizeof(uint) * _item_cc));
			assert(p_indices != NULL);
			// Creating a list of item indices sorted using the children's `order'
			// attribute values. We are using a simple insertion sort as we need
			// stability (insertion order must be preserved) and cross-platform
			// support. We should eventually switch to merge sort (or something
			// else) if the number of items becomes significant enough.
			for(uint i = 0; i < _item_cc; i++) {
				p_indices[i] = i;
				for(uint j = i; j > 0; j--) {
					const uint prev = p_indices[j-1];
					const uint curr = p_indices[j];
					if(pItem->GetChildC(prev)->GetOrder() <= pItem->GetChildC(curr)->GetOrder()) {
						break;
					}
					p_indices[j - 1] = curr;
					p_indices[j] = prev;
				}
			}
			ordered_indices = p_indices;
		}
		if(Flags & fWrap) {
			if(pItem->ALB.Flags & AbstractLayoutBlock::fContainerWrapReverse)
				Pos2 = AlignDim;
		}
		else
			Pos2 = (Flags & fVertical) ? pItem->ALB.Padding.a.X : pItem->ALB.Padding.a.Y;
		LineDim = (Flags & fWrap) ? 0.0f : AlignDim;
		FlexDim = SizeDim;
	}
	void Reset()
	{
		LineDim = (Flags & fWrap) ? 0.0f : AlignDim;
		FlexDim = SizeDim;
		ExtraFlexDim = 0.0f;
		FlexGrows = 0.0f;
		FlexShrinks = 0.0f;
	}
	const LayoutFlexItem & GetChildByIndex(const LayoutFlexItem * pContainer, uint idx) const
	{
		const uint _pos = ordered_indices ? ordered_indices[idx] : idx;
		assert(_pos < pContainer->GetChildrenCount());
		return *pContainer->GetChildC(_pos);
	}
	void   ProcessLines(const LayoutFlexItem & rItem);
	// Set during init.
	enum {
		fWrap      = 0x0001,
		fReverse   = 0x0002, // whether main axis is reversed
		fReverse2  = 0x0004, // whether cross axis is reversed (wrap only)
		fVertical  = 0x0008,
		fNeedLines = 0x0010  // Calculated layout lines - only tracked when needed:
			// - if the root's align_content property isn't set to FLEX_ALIGN_START
			// - or if any child item doesn't have a cross-axis size set
	};
	// Поле Flags по идее должно быть const, но флаг fNeedLines может быть установлен позже. Потому, увы, non-const
	/*const*/uint   Flags;
	uint   FramePos1i; // main axis position
	uint   FramePos2i; // cross axis position
	uint   FrameSz1i;  // main axis size
	uint   FrameSz2i;  // cross axis size
	uint * ordered_indices;
	float  SizeDim;    // main axis parent size
	float  AlignDim;   // cross axis parent size
	// Set for each line layout.
	float  LineDim;    // the cross axis size
	float  FlexDim;    // the flexible part of the main axis size
	float  ExtraFlexDim; // sizes of flexible items
	float  FlexGrows;
	float  FlexShrinks;
	float  Pos2;         // cross axis position
	struct Line {
		Line(uint beginChildIdx, uint endChildIdx, float aSize) : ChildBeginIdx(beginChildIdx), ChildEndIdx(endChildIdx), Size(aSize)
		{
		}
		uint   ChildBeginIdx;
		uint   ChildEndIdx;
		float  Size;
	};
	class LineArray : public TSVector <Line> {
	public:
		LineArray() : TSVector <Line>(), TotalSize(0.0f)
		{
		}
		float  GetTotalSize() const { return TotalSize; }
		void   Add(uint beginChildIdx, uint endChildIdx, float aSize)
		{
			Line new_entry(beginChildIdx, endChildIdx, aSize);
			insert(&new_entry);
			TotalSize += aSize;
		}
	private:
		float  TotalSize;
	};
	LineArray Lines;
	const  LayoutFlexItem::Param P;
};

bool LayoutFlexItem::LayoutAlign(/*flex_align*/int align, float flexDim, uint childrenCount, float * pPos, float * pSpacing, bool stretchAllowed) const
{
	assert(flexDim > 0);
	float pos = 0.0f;
	float spacing = 0.0f;
	switch(align) {
		case AbstractLayoutBlock::alignStart: break;
		case AbstractLayoutBlock::alignEnd: pos = flexDim; break;
		case AbstractLayoutBlock::alignCenter: pos = flexDim / 2; break;
		case AbstractLayoutBlock::alignSpaceBetween:
		    if(childrenCount > 1) {
			    spacing = flexDim / (childrenCount-1);
		    }
		    break;
		case AbstractLayoutBlock::alignSpaceAround:
		    if(childrenCount) {
			    spacing = flexDim / childrenCount;
			    pos = spacing / 2;
		    }
		    break;
		case AbstractLayoutBlock::alignSpaceEvenly:
		    if(childrenCount) {
			    spacing = flexDim / (childrenCount+1);
			    pos = spacing;
		    }
		    break;
		case AbstractLayoutBlock::alignStretch:
		    if(stretchAllowed) {
			    spacing = flexDim / childrenCount;
			    break;
		    }
			else { // by default: AbstractLayoutBlock::alignStart
				break;
			}
		// @fallthrough
		default:
		    return false;
	}
	*pPos = pos;
	*pSpacing = spacing;
	return true;
}

int LayoutFlexItem::InitHomogeneousArray(uint variableFactor /* HomogeneousArray::vfXXX */)
{
	int    ok = 1;
	if(!P_HgL) {
		P_HgL = new HomogeneousArray();
		P_HgL->VariableFactor = variableFactor;
	}
	else {
		P_HgL->clear();
	}
	if(P_HgL)
		P_HgL->VariableFactor = variableFactor;
	else
		ok = 0;
	return ok;
}

int LayoutFlexItem::AddHomogeneousEntry(long id, float vf)
{
	int    ok = 1;
	if(P_HgL) {
		HomogeneousEntry new_entry;
		new_entry.ID = id;
		new_entry.Vf = vf;
		P_HgL->insert(&new_entry);
	}
	else
		ok = 0;
	return ok;
}

void LayoutFlexItem::UpdateShouldOrderChildren()
{
	if(ALB.Order != 0 && P_Parent)
		P_Parent->State |= stShouldOrderChildren;
}

/*flex_align*/int FASTCALL LayoutFlexItem::GetChildAlign(const LayoutFlexItem & rChild) const
{
	return (rChild.ALB.AlignSelf == ALB.alignAuto) ? ALB.AlignItems : rChild.ALB.AlignSelf;
}

#define CHILD_MARGIN_XY_(ptrLayout, child, pnt) ((ptrLayout->Flags & LayoutFlexProcessor::fVertical) ? child.ALB.Margin.pnt.X : child.ALB.Margin.pnt.Y)
#define CHILD_MARGIN_YX_(ptrLayout, child, pnt) ((ptrLayout->Flags & LayoutFlexProcessor::fVertical) ? child.ALB.Margin.pnt.Y : child.ALB.Margin.pnt.X)

void LayoutFlexItem::DoLayoutChildren(uint childBeginIdx, uint childEndIdx, uint childrenCount, void * pLayout, SScroller::SetupBlock * pSsb) const
{
	LayoutFlexProcessor * p_layout = static_cast<LayoutFlexProcessor *>(pLayout);
	assert(childrenCount <= (childEndIdx - childBeginIdx));
	if(childrenCount) {
		if(pSsb) {
			pSsb->ViewSize = p_layout->SizeDim; //page_size;
		}
		if(p_layout->FlexDim > 0 && p_layout->ExtraFlexDim > 0) {
			// If the container has a positive flexible space, let's add to it the sizes of all flexible children.
			p_layout->FlexDim += p_layout->ExtraFlexDim;
		}
		// Determine the main axis initial position and optional spacing.
		float pos = 0.0f;
		float spacing = 0.0f;
		if(p_layout->FlexGrows == 0 && p_layout->FlexDim > 0.0f) {
			const bool lar = LayoutAlign(ALB.JustifyContent, p_layout->FlexDim, childrenCount, &pos, &spacing, false);
			assert(lar && "incorrect justify_content");
			if(p_layout->Flags & LayoutFlexProcessor::fReverse)
				pos = p_layout->SizeDim - pos;
		}
		if(p_layout->Flags & LayoutFlexProcessor::fReverse)
			pos -= (p_layout->Flags & LayoutFlexProcessor::fVertical) ? ALB.Padding.b.Y : ALB.Padding.b.X;
		else 
			pos += (p_layout->Flags & LayoutFlexProcessor::fVertical) ? ALB.Padding.a.Y : ALB.Padding.a.X;
		if((p_layout->Flags & LayoutFlexProcessor::fWrap) && (p_layout->Flags & LayoutFlexProcessor::fReverse2)) {
			p_layout->Pos2 -= p_layout->LineDim;
		}
		for(uint i = childBeginIdx; i < childEndIdx; i++) {
			const LayoutFlexItem & r_child = p_layout->GetChildByIndex(this, i);
			if(!r_child.ALB.IsPositionAbsolute(ALB.GetContainerDirection())) { // Isn't already positioned
				// Grow or shrink the main axis item size if needed.
				float flex_size = 0.0f;
				if(p_layout->FlexDim > 0.0f) {
					if(r_child.ALB.GrowFactor != 0.0f) {
						r_child.R.Frame[p_layout->FrameSz1i] = 0.0f; // Ignore previous size when growing.
						flex_size = (p_layout->FlexDim / p_layout->FlexGrows) * r_child.ALB.GrowFactor;
					}
				}
				else if(p_layout->FlexDim < 0.0f) {
					if(r_child.ALB.ShrinkFactor != 0.0f) {
						flex_size = (p_layout->FlexDim / p_layout->FlexShrinks) * r_child.ALB.ShrinkFactor;
					}
				}
				r_child.R.Frame[p_layout->FrameSz1i] += flex_size;
				// Set the cross axis position (and stretch the cross axis size if needed).
				const float align_size = r_child.R.Frame[p_layout->FrameSz2i];
				float align_pos = p_layout->Pos2 + 0.0f;
				{
					const int ca = GetChildAlign(r_child);
					switch(ca) {
						case AbstractLayoutBlock::alignEnd:
							align_pos += (p_layout->LineDim - align_size - CHILD_MARGIN_XY_(p_layout, r_child, b));
							break;
						case AbstractLayoutBlock::alignCenter:
							align_pos += (p_layout->LineDim / 2.0f) - (align_size / 2.0f) + (CHILD_MARGIN_XY_(p_layout, r_child, a) - CHILD_MARGIN_XY_(p_layout, r_child, b));
							break;
						case AbstractLayoutBlock::alignStretch:
							if(align_size == 0) {
								r_child.R.Frame[p_layout->FrameSz2i] = p_layout->LineDim - (CHILD_MARGIN_XY_(p_layout, r_child, a) + CHILD_MARGIN_XY_(p_layout, r_child, b));
							}
						// @fallthrough
						case AbstractLayoutBlock::alignStart:
							align_pos += CHILD_MARGIN_XY_(p_layout, r_child, a);
							break;
						default:
							//assert(false && "incorrect align_self");
							align_pos += CHILD_MARGIN_XY_(p_layout, r_child, a); // По умолчанию пусть будет AbstractLayoutBlock::alignStart
							break;
					}
				}
				r_child.R.Frame[p_layout->FramePos2i] = align_pos;
				// Set the main axis position.
				{
					const float item_size_1 = r_child.R.Frame[p_layout->FrameSz1i];
					const float margin_yx_a = CHILD_MARGIN_YX_(p_layout, r_child, a);
					const float margin_yx_b = CHILD_MARGIN_YX_(p_layout, r_child, b);
					const float _s = (item_size_1 + margin_yx_a + margin_yx_b + spacing);
					if(p_layout->Flags & LayoutFlexProcessor::fReverse) {
						r_child.R.Frame[p_layout->FramePos1i] = (pos - (item_size_1 + margin_yx_b));
						pos -= _s;
					}
					else {
						r_child.R.Frame[p_layout->FramePos1i] = (pos + margin_yx_a);
						pos += _s;
					}
					if(pSsb) {
						pSsb->ItemCount++;
						pSsb->ItemSizeList.add(_s);
					}
				}
				if(r_child.ALB.AspectRatio > 0.0) {
					if(r_child.R.Frame[2] == 0.0f && r_child.ALB.SzX == AbstractLayoutBlock::szUndef && r_child.R.Frame[3] > 0.0f) {
						r_child.R.Frame[2] = r_child.R.Frame[3] / r_child.ALB.AspectRatio;
					}
					if(r_child.R.Frame[3] == 0.0f && r_child.ALB.SzY == AbstractLayoutBlock::szUndef && r_child.R.Frame[2] > 0.0f) {
						r_child.R.Frame[3] = r_child.R.Frame[2] * r_child.ALB.AspectRatio;
					}
				}
				r_child.Commit_(); // Now that the item has a frame, we can layout its children.
			}
		}
		if((p_layout->Flags & LayoutFlexProcessor::fWrap) && !(p_layout->Flags & LayoutFlexProcessor::fReverse2))
			p_layout->Pos2 += p_layout->LineDim;
		if(p_layout->Flags & LayoutFlexProcessor::fNeedLines)
			p_layout->Lines.Add(childBeginIdx, childEndIdx, p_layout->LineDim);
	}
}

int LayoutFlexItem::SetupResultScroller(SScroller::SetupBlock * pSb) const
{
	int    ok = 1;
	if(pSb)	{
		SETIFZ(R.P_Scrlr, new SScroller);
		ok = R.P_Scrlr ? R.P_Scrlr->Setup(*pSb) : 0;
	}
	else {
		ZDELETE(R.P_Scrlr);
	}
	return ok;
}

void LayoutFlexItem::Commit_() const
{
	const FRect bb = R;
	//SETFLAG(R.Flags, R.fNotFit, (P_Parent && !static_cast<FRect>(P_Parent->R).Contains(bb)));
	//SETFLAG(R.Flags, R.fDegradedWidth,  (bb.Width() <= 0.0f));
	//SETFLAG(R.Flags, R.fDegradedHeight, (bb.Height() <= 0.0f));
	if(GetChildrenCount()) {
		Param p;
		p.ForceWidth  = bb.Width();
		p.ForceHeight = bb.Height();
		DoLayout(p);
	}

}

void LayoutFlexItem::DoLayout(const Param & rP) const
{
	const uint _cc = GetChildrenCount();
	const int  _direction = ALB.GetContainerDirection();
	const int  _cross_direction = AbstractLayoutBlock::GetCrossDirection(_direction);
	if(_cc && oneof2(_direction, DIREC_HORZ, DIREC_VERT)) {
		LayoutFlexProcessor layout_s(this, rP);
		uint last_layout_child = 0;
		uint relative_children_count = 0;
		for(uint i = 0; i < _cc; i++) {
			const LayoutFlexItem & r_child = layout_s.GetChildByIndex(this, i);
			// Items with an absolute position have their frames determined
			// directly and are skipped during layout.
			if(r_child.ALB.IsPositionAbsoluteX() && r_child.ALB.IsPositionAbsoluteY()) {
				r_child.R.Frame[0] = r_child.ALB.GetAbsoluteLowX();
				r_child.R.Frame[1] = r_child.ALB.GetAbsoluteLowY();
				r_child.R.Frame[2] = r_child.ALB.GetAbsoluteSizeX();
				r_child.R.Frame[3] = r_child.ALB.GetAbsoluteSizeY();
				r_child.Commit_(); // @recursion // Now that the item has a frame, we can layout its children.
			}
			else {
				// Initialize frame.
				r_child.R.Frame[0] = 0.0f;
				r_child.R.Frame[1] = 0.0f;
				SPoint2F efsxy = r_child.ALB.CalcEffectiveSizeXY(rP.ForceWidth, rP.ForceHeight);
				r_child.R.Frame[2] = efsxy.X;
				r_child.R.Frame[3] = efsxy.Y;
				//
				// Main axis size defaults to 0.
				//
				// Cross axis size defaults to the parent's size (or line size in wrap mode, which is calculated later on).
				if(fisnan(r_child.R.Frame[layout_s.FrameSz2i])) {
					if(layout_s.Flags & LayoutFlexProcessor::fWrap)
						layout_s.Flags |= LayoutFlexProcessor::fNeedLines;
					else {
						const float full_size = ((layout_s.Flags & LayoutFlexProcessor::fVertical) ? rP.ForceWidth : rP.ForceHeight);
						r_child.R.Frame[layout_s.FrameSz2i] = full_size - CHILD_MARGIN_XY_((&layout_s), r_child, a) - CHILD_MARGIN_XY_((&layout_s), r_child, b);
					}
				}
				// Call the self_sizing callback if provided. Only non-NAN values
				// are taken into account. If the item's cross-axis align property
				// is set to stretch, ignore the value returned by the callback.
				if(r_child.CbSelfSizing) {
					float size[2] = { r_child.R.Frame[2], r_child.R.Frame[3] };
					r_child.CbSelfSizing(&r_child, size);
					for(uint j = 0; j < 2; j++) {
						const uint size_off = j + 2;
						if(size_off != layout_s.FrameSz2i || GetChildAlign(r_child) != AbstractLayoutBlock::alignStretch) {
							float val = size[j];
							if(!fisnanf(val))
								r_child.R.Frame[size_off] = val;
						}
					}
				}
				// Honor the `basis' property which overrides the main-axis size.
				if(r_child.ALB.Basis > 0.0f) {
					//assert(r_child.ALB.Basis >= 0.0f);
					r_child.R.Frame[layout_s.FrameSz1i] = r_child.ALB.Basis;
				}
				const float child_size = r_child.R.Frame[layout_s.FrameSz1i];
				if(layout_s.Flags & LayoutFlexProcessor::fWrap) {
					if(layout_s.FlexDim < child_size) {
						// Not enough space for this child on this line, layout the remaining items and move it to a new line.
						DoLayoutChildren(last_layout_child, i, relative_children_count, &layout_s, 0);
						layout_s.Reset();
						last_layout_child = i;
						relative_children_count = 0;
					}
					const float child_size2 = r_child.R.Frame[layout_s.FrameSz2i];
					assert(!fisnanf(child_size2));
					if(child_size2 > layout_s.LineDim)
						layout_s.LineDim = child_size2;
				}
				assert(r_child.ALB.GrowFactor >= 0.0f);
				assert(r_child.ALB.ShrinkFactor >= 0.0f);
				layout_s.FlexGrows   += r_child.ALB.GrowFactor;
				layout_s.FlexShrinks += r_child.ALB.ShrinkFactor;
				layout_s.FlexDim     -= (child_size + (CHILD_MARGIN_YX_((&layout_s), r_child, a) + CHILD_MARGIN_YX_((&layout_s), r_child, b)));
				relative_children_count++;
				if(child_size > 0.0f && r_child.ALB.GrowFactor > 0.0f)
					layout_s.ExtraFlexDim += child_size;
			}
		}
		{
			SScroller::SetupBlock ssb; // @v11.0.3
			// Layout remaining items in wrap mode, or everything otherwise.
			{
				SScroller::SetupBlock * p_ssb = (layout_s.Lines.getCount() || !(ALB.Flags & ALB.fEvaluateScroller)) ? 0 : &ssb;
				DoLayoutChildren(last_layout_child, _cc, relative_children_count, &layout_s, p_ssb);
			}
			//
			// In wrap mode we may need to tweak the position of each line according to
			// the align_content property as well as the cross-axis size of items that haven't been set yet.
			//
			//layout_s.ProcessLines(*this, 0);
			//void LayoutFlexProcessor::ProcessLines(const LayoutFlexItem & rItem, LayoutFlexItem::PagingResult * pPr)
			if(layout_s.Lines.getCount()) {
				const bool is_reverse2 = LOGIC(layout_s.Flags & LayoutFlexProcessor::fReverse2);
				//pr.LineCount = layout_s.Lines.getCount();
				float curr_page_running = 0.0f;
				float page_size = layout_s.AlignDim; // ?
				assert(layout_s.Flags & LayoutFlexProcessor::fNeedLines); // layout_s.Lines.getCount() > 0 может быть только при условии layout_s.need_lines
				float pos = 0.0f;
				float spacing = 0.0f;
				const float flex_dim = layout_s.AlignDim - layout_s.Lines.GetTotalSize();
				if(flex_dim > 0.0f) {
					const bool lar = LayoutAlign(ALB.AlignContent, flex_dim, layout_s.Lines.getCount(), &pos, &spacing, true);
					assert(lar && "incorrect align_content");
				}
				float old_pos = 0.0f;
				if(is_reverse2) {
					pos = layout_s.AlignDim - pos;
					old_pos = layout_s.AlignDim;
				}
				{
					if(ALB.Flags & ALB.fEvaluateScroller) {
						ssb.ViewSize = page_size;
					}
					const bool is_align_stretch = (ALB.AlignContent == AbstractLayoutBlock::alignStretch);
					for(uint i = 0; i < layout_s.Lines.getCount(); i++) {
						const LayoutFlexProcessor::Line & r_line = layout_s.Lines.at(i);
						const float _s = r_line.Size + spacing;
						if(ALB.Flags & ALB.fEvaluateScroller) {
							ssb.ItemSizeList.add(_s);
							ssb.ItemCount++;
						}
						if(is_reverse2) {
							if((curr_page_running + _s) > page_size) {
								//pr.PageCount++;
								curr_page_running = 0.0f;
							}
							curr_page_running += _s;
							//if(pr.PageCount == 1) pr.LastFittedItemIndex = r_line.ChildEndIdx;
							//
							pos -= _s;
							old_pos -= r_line.Size;
						}
						// Re-position the children of this line, honoring any child alignment previously set within the line.
						for(uint j = r_line.ChildBeginIdx; j < r_line.ChildEndIdx; j++) {
							const LayoutFlexItem & r_child = layout_s.GetChildByIndex(this, j);
							if(!r_child.ALB.IsPositionAbsolute(_cross_direction)) { // Should not be re-positioned.
								if(fisnanf(r_child.R.Frame[layout_s.FrameSz2i])) {
									// If the child's cross axis size hasn't been set it, it defaults to the line size.
									r_child.R.Frame[layout_s.FrameSz2i] = r_line.Size + (is_align_stretch ? spacing : 0);
								}
								r_child.R.Frame[layout_s.FramePos2i] = pos + (r_child.R.Frame[layout_s.FramePos2i] - old_pos);
							}
						}
						if(!is_reverse2) {
							if((curr_page_running + _s) > page_size) {
								//pr.PageCount++;
								curr_page_running = 0.0f;
							}
							curr_page_running += _s;
							//if(pr.PageCount == 1) pr.LastFittedItemIndex = r_line.ChildEndIdx;
							//
							pos += _s;
							old_pos += r_line.Size;
						}
					}
				}
				//pr.PageCount = pr.PageCount; // @debug
			}
			SetupResultScroller((ALB.Flags & ALB.fEvaluateScroller && ssb.ItemCount) ? &ssb : 0);
		}
		{
			ZFREE(layout_s.ordered_indices);
			layout_s.Lines.clear();
		}
	}
	//ASSIGN_PTR(pPgR, pr);
}

int AbstractLayoutBlock::SetVArea(int area)
{
	int    ok = 1;
	struct LocalAreaGravity {
		LocalAreaGravity() : Gx(0), Gy(0) {}
		void   Set(uint16 _gx, uint16 _gy) { Gx = _gx; Gy = _gy; }
		uint16 Gx;
		uint16 Gy;
	};
	LocalAreaGravity lag;
	switch(area) {
		case areaCornerLU: lag.Set(SIDE_LEFT,   SIDE_TOP); break;
		case areaCornerRU: lag.Set(SIDE_RIGHT,  SIDE_TOP); break;
		case areaCornerRB: lag.Set(SIDE_RIGHT,  SIDE_BOTTOM); break;
		case areaCornerLB: lag.Set(SIDE_LEFT,   SIDE_BOTTOM); break;
		case areaSideU:    lag.Set(SIDE_CENTER, SIDE_TOP); break;
		case areaSideR:    lag.Set(SIDE_RIGHT,  SIDE_CENTER); break;
		case areaSideB:    lag.Set(SIDE_CENTER, SIDE_BOTTOM); break;
		case areaSideL:    lag.Set(SIDE_LEFT,   SIDE_CENTER); break;
		case areaCenter:   lag.Set(SIDE_CENTER, SIDE_CENTER); break;
		default: ok = 0; break;
	}
	if(ok) {
		if(GravityX != lag.Gx || GravityY != lag.Gy) {
			GravityX = lag.Gx;
			GravityY = lag.Gy;
			ok = 1;
		}
		else
			ok = -1;
	}
	return ok;
}

int AbstractLayoutBlock::GetVArea(/*const LayoutFlexItem & rItem*/) const
{
	int   a = -1;
	const int gx = /*rItem.ALB.*/GravityX;
	const int gy = /*rItem.ALB.*/GravityY;
	switch(gx) {
		case 0:
			switch(gy) {
				case 0: a = AbstractLayoutBlock::areaCenter; break;
				case SIDE_TOP: a = AbstractLayoutBlock::areaSideU; break;
				case SIDE_BOTTOM: a = AbstractLayoutBlock::areaSideB; break;
				case SIDE_CENTER: a = AbstractLayoutBlock::areaCenter; break;
				default: assert(0);
			}
			break;
		case SIDE_LEFT:
			switch(gy) {
				case 0: a = AbstractLayoutBlock::areaSideL; break;
				case SIDE_TOP: a = AbstractLayoutBlock::areaCornerLU; break;
				case SIDE_BOTTOM: a = AbstractLayoutBlock::areaCornerLB; break;
				case SIDE_CENTER: a = AbstractLayoutBlock::areaSideL; break;
				default: assert(0);
			}
			break;
		case SIDE_RIGHT:
			switch(gy) {
				case 0: a = AbstractLayoutBlock::areaSideR; break;
				case SIDE_TOP: a = AbstractLayoutBlock::areaCornerRU; break;
				case SIDE_BOTTOM: a = AbstractLayoutBlock::areaCornerRB; break;
				case SIDE_CENTER: a = AbstractLayoutBlock::areaSideR; break;
				default: assert(0);
			}
			break;
		case SIDE_CENTER:
			switch(gy) {
				case 0: a = AbstractLayoutBlock::areaCenter; break;
				case SIDE_TOP: a = AbstractLayoutBlock::areaSideU; break;
				case SIDE_BOTTOM: a = AbstractLayoutBlock::areaSideB; break;
				case SIDE_CENTER: a = AbstractLayoutBlock::areaCenter; break;
				default: assert(0);
			}
			break;
		default:
			assert(0);
	}
	return a;
}

/*static*/int AbstractLayoutBlock::RestrictVArea(int restrictingArea, const FRect & rRestrictingRect, int restrictedArea, FRect & rRestrictedRect)
{
	int    ok = 1;
	assert(restrictedArea != restrictingArea);
	THROW(restrictedArea != restrictingArea);
	assert(oneof9(restrictedArea, areaCornerLU, areaCornerRU, areaCornerRB, areaCornerLB, areaSideU, areaSideR, areaSideB, areaSideL, areaCenter));
	THROW(oneof9(restrictedArea, areaCornerLU, areaCornerRU, areaCornerRB, areaCornerLB, areaSideU, areaSideR, areaSideB, areaSideL, areaCenter));
	assert(oneof9(restrictingArea, areaCornerLU, areaCornerRU, areaCornerRB, areaCornerLB, areaSideU, areaSideR, areaSideB, areaSideL, areaCenter));
	THROW(oneof9(restrictingArea, areaCornerLU, areaCornerRU, areaCornerRB, areaCornerLB, areaSideU, areaSideR, areaSideB, areaSideL, areaCenter));
	switch(restrictingArea) {
		case areaCornerLU:
			switch(restrictedArea) {
				case areaCornerRU: 
				case areaSideU:
				case areaSideR:
					SETMAX(rRestrictedRect.a.X, rRestrictingRect.b.X);
					break;
				case areaCornerLB:
				case areaSideB:
				case areaSideL:
					SETMAX(rRestrictedRect.a.Y, rRestrictingRect.b.Y);
					break;
				case areaCornerRB:
				case areaCenter: 
					SETMAX(rRestrictedRect.a.X, rRestrictingRect.b.X);
					SETMAX(rRestrictedRect.a.Y, rRestrictingRect.b.Y);
					break;
			}
			break;
		case areaCornerRU:
			switch(restrictedArea) {
				case areaCornerLU:
				case areaSideU:
				case areaSideL:
					SETMIN(rRestrictedRect.b.X, rRestrictingRect.a.X);
					break;
				case areaCornerRB:
				case areaSideB:
				case areaSideR:
					SETMAX(rRestrictedRect.a.Y, rRestrictingRect.b.Y);
					break;
				case areaCornerLB:
				case areaCenter: 
					SETMIN(rRestrictedRect.b.X, rRestrictingRect.a.X);
					SETMAX(rRestrictedRect.a.Y, rRestrictingRect.b.Y);
					break;
			}
			break;
		case areaCornerRB: 
			switch(restrictedArea) {
				case areaCornerLB:
				case areaSideL:
				case areaSideB:
					SETMIN(rRestrictedRect.b.X, rRestrictingRect.a.X);
					break;
				case areaSideU:
				case areaCornerRU:
				case areaSideR:
					SETMIN(rRestrictedRect.b.Y, rRestrictingRect.a.Y);
					break;
				case areaCornerLU:
				case areaCenter: 
					SETMIN(rRestrictedRect.b.X, rRestrictingRect.a.X);
					SETMIN(rRestrictedRect.b.Y, rRestrictingRect.a.Y);
					break;
			}
			break;
		case areaCornerLB: 
			switch(restrictedArea) {
				case areaSideU:
				case areaSideL:
				case areaCornerLU:
					SETMIN(rRestrictedRect.b.Y, rRestrictingRect.a.Y);
					break;
				case areaSideB:
				case areaSideR:
				case areaCornerRB:
					SETMAX(rRestrictedRect.a.X, rRestrictingRect.b.X);
					break;
				case areaCornerRU:				
				case areaCenter: 
					SETMAX(rRestrictedRect.a.X, rRestrictingRect.b.X);
					SETMIN(rRestrictedRect.b.Y, rRestrictingRect.a.Y);
					break;
			}
			break;
		case areaSideU: 
			switch(restrictedArea) {
				case areaSideL:
				case areaSideR:
				case areaSideB:
				case areaCornerLB:
				case areaCornerRB:
				case areaCenter: 
					SETMAX(rRestrictedRect.a.Y, rRestrictingRect.b.Y);
					break;
				case areaCornerLU:
					SETMIN(rRestrictedRect.b.X, rRestrictingRect.a.X);
					break;
				case areaCornerRU:				
					SETMAX(rRestrictedRect.a.X, rRestrictingRect.b.X);
					break;
			}
			break;
		case areaSideB: 
			switch(restrictedArea) {
				case areaSideL:
				case areaSideR:
				case areaSideU:
				case areaCornerLU:
				case areaCornerRU:				
				case areaCenter: 
					SETMIN(rRestrictedRect.b.Y, rRestrictingRect.a.Y);
					break;
				case areaCornerLB:
					SETMIN(rRestrictedRect.b.X, rRestrictingRect.a.X);
					break;
				case areaCornerRB:
					SETMAX(rRestrictedRect.a.X, rRestrictingRect.b.X);
					break;
			}
			break;
		case areaSideR: 
			switch(restrictedArea) {
				case areaCornerLB:
				case areaSideL:
				case areaCornerLU:
				case areaSideB:
				case areaSideU:
				case areaCenter: 
					SETMIN(rRestrictedRect.b.X, rRestrictingRect.a.X);
					break;
				case areaCornerRU:
					SETMIN(rRestrictedRect.b.Y, rRestrictingRect.a.Y);
					break;
				case areaCornerRB:
					SETMAX(rRestrictedRect.a.Y, rRestrictingRect.b.Y);
					break;
			}
			break;
		case areaSideL: 
			switch(restrictedArea) {
				case areaSideB:
				case areaSideR:
				case areaSideU:
				case areaCornerRB:
				case areaCornerRU:				
				case areaCenter: 
					SETMAX(rRestrictedRect.a.X, rRestrictingRect.b.X);
					break;
				case areaCornerLU:
					SETMIN(rRestrictedRect.b.Y, rRestrictingRect.a.Y);
					break;
				case areaCornerLB:
					SETMAX(rRestrictedRect.a.Y, rRestrictingRect.b.Y);
					break;
			}
			break;
		case areaCenter:
			switch(restrictedArea) {
				case areaCornerLB:
					SETMIN(rRestrictedRect.b.X, rRestrictingRect.a.X);
					SETMAX(rRestrictedRect.a.Y, rRestrictingRect.b.Y);
					break;
				case areaSideB:
					SETMAX(rRestrictedRect.a.Y, rRestrictingRect.b.Y);
					break;
				case areaCornerLU:
					SETMIN(rRestrictedRect.b.X, rRestrictingRect.a.X);
					SETMIN(rRestrictedRect.b.Y, rRestrictingRect.a.Y);
					break;
				case areaSideR:
					SETMAX(rRestrictedRect.a.X, rRestrictingRect.b.X);
					break;
				case areaSideU:
					SETMIN(rRestrictedRect.b.Y, rRestrictingRect.a.Y);
					break;
				case areaCornerRB:
					SETMAX(rRestrictedRect.a.X, rRestrictingRect.b.X);
					SETMAX(rRestrictedRect.a.Y, rRestrictingRect.b.Y);
					break;
				case areaCornerRU:
					SETMAX(rRestrictedRect.a.X, rRestrictingRect.b.X);
					SETMIN(rRestrictedRect.b.Y, rRestrictingRect.a.Y);
					break;
				case areaSideL: 
					SETMIN(rRestrictedRect.b.X, rRestrictingRect.a.X);
					break;
			}
			break;
	}	
	CATCHZOK
	return ok;
}

void LayoutFlexItem::Helper_CommitInnerFloatLayout(LayoutFlexItem * pCurrentLayout, const SPoint2F & rOffs) const
{
	//const float offs_x = area_rect[prev_area].a.X;
	//const float offs_y = area_rect[prev_area].a.Y;
	const uint _cl_cc = pCurrentLayout->GetChildrenCount();
	for(uint ci = 0; ci < _cl_cc; ci++) {
		const LayoutFlexItem * p_layout_item = pCurrentLayout->GetChildC(ci);
		if(p_layout_item && p_layout_item->P_Link) {
			p_layout_item->P_Link->R.CopyWithOffset(p_layout_item->R, rOffs.X, rOffs.Y);
			p_layout_item->P_Link->Commit_();
		}
	}
	delete pCurrentLayout;
}

void LayoutFlexItem::DoFloatLayout(const Param & rP)
{
	const uint _cc = GetChildrenCount();
	if(_cc) {
		//
		// Разбиваем всю область на 9 виртуальных зон (углы, стороны, центр)
		//
		FRect area_rect[9];
		int   bypass_direction[9]; // Последовательность обхода зон для заполнения // 
		assert(SIZEOFARRAY(area_rect) == SIZEOFARRAY(bypass_direction));
		{
			// Все виртуальные зоны инициализируем величиной полной области.
			for(uint i = 0; i < SIZEOFARRAY(area_rect); i++) {
				area_rect[i].a.SetZero();
				area_rect[i].b.Set(rP.ForceWidth, rP.ForceHeight);
			}
		}
		{
			// Порядок обхода инициализируем неопределенными значениями
			for(uint i = 0; i < SIZEOFARRAY(bypass_direction); i++)
				bypass_direction[i] = -1;
		}
		{
			LAssocArray item_map; // Карта распределения элементов по областям {itemIdx; area}
			LayoutFlexProcessor layout_s(this, rP);
			{
				for(uint cidx = 0; cidx < _cc; cidx++) {
					const LayoutFlexItem & r_child = layout_s.GetChildByIndex(this, cidx);
					const int va = r_child.ALB.GetVArea();
					assert(va >= 0 && va < SIZEOFARRAY(area_rect));
					if(va >= 0 && va < SIZEOFARRAY(area_rect)) {
						item_map.Add(cidx, va);
					}
				}
			}
			{
				//
				// Сортируем массив так, чтобы элементы, относящиеся к одной области, держались вместе
				// при этом соблюдая порядок следования элементов (приоритет размещения важен)
				// @todo На самом деле, нужен дополнительный критерий сортировки: по порядку в котором области встречаются в оригинальном списке,
				// но на этапе разработки оставим упрощенную схему.
				//
				item_map.SortByValKey();
			}
			{
				int    prev_area = -1;
				LayoutFlexItem * p_current_layout = 0;
				LongArray actual_area_list; // Список идентификаторов областей, которые подлежат рассмотрению
				LongArray seen_area_list; // Список уже обработанных идентификаторов областей
				item_map.GetValList(actual_area_list);
				for(uint midx = 0; midx < item_map.getCount(); midx++) {
					const LAssoc & r_ai = item_map.at(midx);
					const uint cidx = static_cast<uint>(r_ai.Key);
					const int  area = static_cast<int>(r_ai.Val);
					assert(area >= 0 && area < SIZEOFARRAY(area_rect));
					assert(cidx >= 0 && cidx < _cc);
					const LayoutFlexItem & r_child = layout_s.GetChildByIndex(this, cidx);
					if(area != prev_area) {
						if(p_current_layout) {
							assert(prev_area >= 0 && prev_area < SIZEOFARRAY(area_rect));
							Param local_eval_param;
							local_eval_param.Flags = rP.Flags;
							p_current_layout->Evaluate(&local_eval_param);
							{
								// Вычисляем полный прямоугольник занятый областью prev_area и
								// корректируем остальные области в соответствии с этим.
								FRect cf;
								if(p_current_layout->GetInnerCombinedFrame(&cf) > 0) {
									cf.Move__(area_rect[prev_area].a.X, area_rect[prev_area].a.Y);
									for(uint ai = 0; ai < actual_area_list.getCount(); ai++) {
										const int local_area = actual_area_list.get(ai);
										if(local_area != prev_area && !seen_area_list.lsearch(local_area)) {
											const int rvar = AbstractLayoutBlock::RestrictVArea(prev_area, cf, local_area, area_rect[local_area]);
											assert(rvar);
										}
									}
								}
								seen_area_list.add(prev_area);
							}
							Helper_CommitInnerFloatLayout(p_current_layout, area_rect[prev_area].a);
							p_current_layout = 0;
						}
						{
							p_current_layout = new LayoutFlexItem();
							struct LocalLayoutEntry {
								LocalLayoutEntry() : ContainerDirec(DIREC_UNKN), ContainerFlags(AbstractLayoutBlock::fContainerWrap), JustifyContent(0), AlignContent(0)
								{
								}
								void Set(int direc, uint32 cf, uint16 justifyContent, uint16 alignContent)
								{
									ContainerDirec = direc;
									ContainerFlags |= cf;
									JustifyContent = justifyContent;
									AlignContent = alignContent;
								}
								int    ContainerDirec;
								uint32 ContainerFlags;
								uint16 JustifyContent; // AbstractLayoutBlock::alignXXX Выравнивание внутренних элементов вдоль основной оси
								uint16 AlignContent;   // AbstractLayoutBlock::alignXXX Выравнивание внутренних элементов по кросс-оси
							};
							LocalLayoutEntry lle;
							switch(area) {
								case AbstractLayoutBlock::areaCornerLU:
									lle.Set(DIREC_HORZ, 0, AbstractLayoutBlock::alignStart, AbstractLayoutBlock::alignStart);
									break;
								case AbstractLayoutBlock::areaCornerRU:
									lle.Set(DIREC_HORZ, AbstractLayoutBlock::fContainerReverseDir, AbstractLayoutBlock::alignStart, AbstractLayoutBlock::alignStart);
									break;
								case AbstractLayoutBlock::areaCornerRB:
									lle.Set(DIREC_HORZ, AbstractLayoutBlock::fContainerReverseDir|AbstractLayoutBlock::fContainerWrapReverse,
										AbstractLayoutBlock::alignStart, AbstractLayoutBlock::alignStart);
									break;
								case AbstractLayoutBlock::areaCornerLB:
									lle.Set(DIREC_HORZ, 0/*AbstractLayoutBlock::fContainerWrapReverse*/, AbstractLayoutBlock::alignStart, AbstractLayoutBlock::alignEnd);
									break;
								case AbstractLayoutBlock::areaSideU:
									lle.Set(DIREC_HORZ, 0, AbstractLayoutBlock::alignStart, AbstractLayoutBlock::alignStart);
									break;
								case AbstractLayoutBlock::areaSideR:
									lle.Set(DIREC_VERT, 0, AbstractLayoutBlock::alignStart, AbstractLayoutBlock::alignEnd);
									break;
								case AbstractLayoutBlock::areaSideB:
									lle.Set(DIREC_HORZ, 0, AbstractLayoutBlock::alignStart, AbstractLayoutBlock::alignEnd);
									break;
								case AbstractLayoutBlock::areaSideL:
									lle.Set(DIREC_VERT, 0, AbstractLayoutBlock::alignStart, AbstractLayoutBlock::alignStart);
									break;
								case AbstractLayoutBlock::areaCenter:
									lle.Set(DIREC_HORZ, 0, AbstractLayoutBlock::alignStart, AbstractLayoutBlock::alignStart); // @?
									break;
							}
							p_current_layout->ALB.SetContainerDirection(lle.ContainerDirec);
							p_current_layout->ALB.JustifyContent = lle.JustifyContent;
							p_current_layout->ALB.AlignContent = lle.AlignContent;
							p_current_layout->ALB.Flags |= lle.ContainerFlags;
							p_current_layout->ALB.SetFixedSizeX(area_rect[area].Width());
							p_current_layout->ALB.SetFixedSizeY(area_rect[area].Height()); 
						}
					}
					{
						assert(p_current_layout);
						LayoutFlexItem * p_layout_item = p_current_layout->InsertItem();
						p_layout_item->SetLayoutBlock(r_child.ALB);
						p_layout_item->SetCallbacks(r_child.CbSelfSizing, r_child.CbSetup, r_child.managed_ptr);
						p_layout_item->P_Link = &r_child;
					}
					prev_area = area;
				}
				if(p_current_layout) {
					assert(prev_area >= 0 && prev_area < SIZEOFARRAY(area_rect));
					Param local_eval_param;
					local_eval_param.Flags = rP.Flags;
					p_current_layout->Evaluate(&local_eval_param);
					Helper_CommitInnerFloatLayout(p_current_layout, area_rect[prev_area].a);
					p_current_layout = 0;
				}
			}
		}
	}
}

void LayoutFlexItem::Setup(uint flags)
{
	if(!(flags & setupfChildrenOnly)) {
		if(CbSetup) {
			CbSetup(this, R);
		}
	}
	const uint _cc = GetChildrenCount();
	for(uint i = 0; i < _cc; i++) {
		GetChild(i)->Setup(0); // @recursion
	}
}

int LayoutFlexItem::Evaluate(const Param * pP)
{
	int    ok = -1;
	const  uint _cc = GetChildrenCount();
	if(_cc && !P_Parent) {
		float sx = 0.0f;
		float sy = 0.0f;
		int   stag_x = ALB.GetSizeX(&sx);
		int   stag_y = ALB.GetSizeY(&sy);
		const int cdir = ALB.GetContainerDirection();
		Param local_evaluate_param;
		if(pP) {
			local_evaluate_param.Flags = pP->Flags;
			if(pP->ForceWidth > 0.0f) {
				local_evaluate_param.ForceWidth = pP->ForceWidth;
				stag_x = AbstractLayoutBlock::szFixed;
			}
			else
				local_evaluate_param.ForceWidth = sx;
			if(pP->ForceHeight > 0.0f) {
				local_evaluate_param.ForceHeight = pP->ForceHeight;
				stag_y = AbstractLayoutBlock::szFixed;
			}
			else
				local_evaluate_param.ForceHeight = sy;
		}
		else {
			local_evaluate_param.ForceWidth = sx;
			local_evaluate_param.ForceHeight = sy;
		}
		if(oneof2(cdir, DIREC_HORZ, DIREC_VERT)) {
			if(stag_x == AbstractLayoutBlock::szFixed && stag_y == AbstractLayoutBlock::szFixed && !CbSelfSizing) {
				assert(P_Parent == NULL);
				assert(CbSelfSizing == NULL);
				DoLayout(local_evaluate_param);
				Setup(setupfChildrenOnly);
				ok = 1;
			}
		}
		else {
			if(stag_x == AbstractLayoutBlock::szFixed && stag_y == AbstractLayoutBlock::szFixed && !CbSelfSizing) {
				DoFloatLayout(local_evaluate_param);
				Setup(setupfChildrenOnly);
				ok = 1;
			}
		}
	}
	return ok;
}
