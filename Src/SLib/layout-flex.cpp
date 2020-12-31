// LAYOUT-FLEX.CPP
// Copyright (c) A.Sobolev 2020
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
	FPoint s;
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
		FPoint s;
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
LayoutFlexItem::Result::Result() : Flags(0)
{
	memzero(Frame, sizeof(Frame));
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
	return *this;
}

LayoutFlexItem::LayoutFlexItem() : P_Parent(0), P_Link(0), managed_ptr(0), CbSelfSizing(0), CbSetup(0), State(0), ALB()
{
	//memzero(frame, sizeof(frame));
}

LayoutFlexItem::~LayoutFlexItem()
{
	P_Link = 0;
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
	if(getCount()) {
		bool is_first = true;
		for(uint i = 0; i < getCount(); i++) {
			const LayoutFlexItem * p_child = at(i);
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
	LayoutFlexItem * p_result_item = CreateNewItem();
	if(p_result_item) {
		p_result_item->P_Parent = this;
		p_result_item->UpdateShouldOrderChildren();		
	}
	return p_result_item;
}

void LayoutFlexItem::DeleteItem(uint idx)
{
	assert(idx < getCount());
	assert(getCount());
	if(idx < getCount()) {
		LayoutFlexItem * p_child = at(idx);
		p_child->P_Parent = 0;
		atFree(idx);
	}
}

class LayoutFlex {
public:
	LayoutFlex(const LayoutFlexItem * pItem, float _width, float _height) : wrap(false), reverse(false), reverse2(false), vertical(false), need_lines(false),
		size_dim(0.0f), align_dim(0.0f), frame_pos_i(0), frame_pos2_i(0), frame_size_i(0), frame_size2_i(0),
		ordered_indices(0), line_dim(0.0f), flex_dim(0.0f), extra_flex_dim(0.0f), flex_grows(0.0f), flex_shrinks(0.0f),
		pos2(0.0f), P_Lines(0), lines_count(0), lines_sizes(0.0f)
	{
		assert(pItem->ALB.Padding.a.X >= 0.0f);
		assert(pItem->ALB.Padding.b.X >= 0.0f);
		assert(pItem->ALB.Padding.a.Y >= 0.0f);
		assert(pItem->ALB.Padding.b.Y >= 0.0f);
		_width  -= (pItem->ALB.Padding.a.X + pItem->ALB.Padding.b.X);
		_height -= (pItem->ALB.Padding.a.Y + pItem->ALB.Padding.b.Y);
		assert(_width >= 0.0f);
		assert(_height >= 0.0f);
		// @ctr reverse = false;
		vertical = true;
		switch(pItem->ALB.GetContainerDirection()) {
			case DIREC_HORZ:
				vertical = false;
				size_dim = _width;
				align_dim = _height;
				frame_pos_i = 0;
				frame_pos2_i = 1;
				frame_size_i = 2;
				frame_size2_i = 3;
				if(pItem->ALB.Flags & AbstractLayoutBlock::fContainerReverseDir)
					reverse = true;
				break;
			case DIREC_VERT:
				size_dim = _height;
				align_dim = _width;
				frame_pos_i = 1;
				frame_pos2_i = 0;
				frame_size_i = 3;
				frame_size2_i = 2;
				if(pItem->ALB.Flags & AbstractLayoutBlock::fContainerReverseDir)
					reverse = true;
				break;
			default:
				//assert(false && "incorrect direction");
				break;
		}
		// @ctr ordered_indices = NULL;
		if((pItem->State & LayoutFlexItem::stShouldOrderChildren) && pItem->getCount()) {
			uint * p_indices = static_cast<uint *>(SAlloc::M(sizeof(uint) * pItem->getCount()));
			assert(p_indices != NULL);
			// Creating a list of item indices sorted using the children's `order'
			// attribute values. We are using a simple insertion sort as we need
			// stability (insertion order must be preserved) and cross-platform
			// support. We should eventually switch to merge sort (or something
			// else) if the number of items becomes significant enough.
			for(uint i = 0; i < pItem->getCount(); i++) {
				p_indices[i] = i;
				for(uint j = i; j > 0; j--) {
					const uint prev = p_indices[j-1];
					const uint curr = p_indices[j];
					if(pItem->at(prev)->GetOrder() <= pItem->at(curr)->GetOrder()) {
						break;
					}
					p_indices[j - 1] = curr;
					p_indices[j] = prev;
				}
			}
			ordered_indices = p_indices;
		}
		// @ctr flex_dim = 0.0f;
		// @ctr flex_grows = 0.0f;
		// @ctr flex_shrinks = 0.0f;
		// @ctr reverse2 = false;
		wrap = LOGIC(pItem->ALB.Flags & AbstractLayoutBlock::fContainerWrap);
		if(wrap) {
			if(pItem->ALB.Flags & AbstractLayoutBlock::fContainerWrapReverse) {
				reverse2 = true;
				pos2 = align_dim;
			}
		}
		else {
			pos2 = vertical ? pItem->ALB.Padding.a.X : pItem->ALB.Padding.a.Y;
		}
		need_lines = (wrap && pItem->ALB.AlignContent != AbstractLayoutBlock::alignStart);
		// @ctr P_Lines = NULL;
		// @ctr lines_count = 0;
		// @ctr lines_sizes = 0;
		line_dim = wrap ? 0 : align_dim;
		flex_dim = size_dim;
	}
	void Reset()
	{
		line_dim = wrap ? 0 : align_dim;
		flex_dim = size_dim;
		extra_flex_dim = 0.0f;
		flex_grows = 0.0f;
		flex_shrinks = 0.0f;
	}
	LayoutFlexItem & GetChildByIndex(LayoutFlexItem * pContainer, uint idx) const
	{
		return *(pContainer->at(ordered_indices ? ordered_indices[idx] : idx));
	}
	// Set during init.
	bool   wrap;
	bool   reverse;           // whether main axis is reversed
	bool   reverse2;          // whether cross axis is reversed (wrap only)
	bool   vertical;
	// Calculated layout lines - only tracked when needed:
	//   - if the root's align_content property isn't set to FLEX_ALIGN_START
	//   - or if any child item doesn't have a cross-axis size set
	bool   need_lines;
	uint8  Reserve[3];    // @alignment
	float  size_dim;      // main axis parent size
	float  align_dim;     // cross axis parent size
	uint   frame_pos_i;   // main axis position
	uint   frame_pos2_i;  // cross axis position
	uint   frame_size_i;  // main axis size
	uint   frame_size2_i; // cross axis size
	uint * ordered_indices;
	// Set for each line layout.
	float  line_dim;         // the cross axis size
	float  flex_dim;         // the flexible part of the main axis size
	float  extra_flex_dim;   // sizes of flexible items
	float  flex_grows;
	float  flex_shrinks;
	float  pos2;             // cross axis position
	struct Line {
		uint   child_begin;
		uint   child_end;
		float  size;
	};
	Line * P_Lines;
	uint  lines_count;
	float lines_sizes;
};

#define _LAYOUT_FRAME_(ptrLayout, child, name) child.R.Frame[ptrLayout->frame_ ## name ## _i]
#define CHILD_POS_(ptrLayout, child)   _LAYOUT_FRAME_(ptrLayout, child, pos)
#define CHILD_POS2_(ptrLayout, child)  _LAYOUT_FRAME_(ptrLayout, child, pos2)
#define CHILD_SIZE_(ptrLayout, child)  _LAYOUT_FRAME_(ptrLayout, child, size)
#define CHILD_SIZE2_(ptrLayout, child) _LAYOUT_FRAME_(ptrLayout, child, size2)
#define CHILD_MARGIN_XY_(ptrLayout, child, pnt) (ptrLayout->vertical ? child.ALB.Margin.pnt.X : child.ALB.Margin.pnt.Y)
#define CHILD_MARGIN_YX_(ptrLayout, child, pnt) (ptrLayout->vertical ? child.ALB.Margin.pnt.Y : child.ALB.Margin.pnt.X)

static bool layout_align(/*flex_align*/int align, float flex_dim, uint children_count, float * pos_p, float * spacing_p, bool stretch_allowed)
{
	assert(flex_dim > 0);
	float pos = 0.0f;
	float spacing = 0.0f;
	switch(align) {
		case AbstractLayoutBlock::alignStart: break;
		case AbstractLayoutBlock::alignEnd: pos = flex_dim; break;
		case AbstractLayoutBlock::alignCenter: pos = flex_dim / 2; break;
		case AbstractLayoutBlock::alignSpaceBetween:
		    if(children_count) {
			    spacing = flex_dim / (children_count - 1);
		    }
		    break;
		case AbstractLayoutBlock::alignSpaceAround:
		    if(children_count) {
			    spacing = flex_dim / children_count;
			    pos = spacing / 2;
		    }
		    break;
		case AbstractLayoutBlock::alignSpaceEvenly:
		    if(children_count) {
			    spacing = flex_dim / (children_count+1);
			    pos = spacing;
		    }
		    break;
		case AbstractLayoutBlock::alignStretch:
		    if(stretch_allowed) {
			    spacing = flex_dim / children_count;
			    break;
		    }
		// fall through
		default:
		    return false;
	}
	*pos_p = pos;
	*spacing_p = spacing;
	return true;
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

void LayoutFlexItem::DoLayoutChildren(uint childBeginIdx, uint childEndIdx, uint childrenCount, /*LayoutFlex*/void * pLayout)
{
	LayoutFlex * p_layout = static_cast<LayoutFlex *>(pLayout);
	assert(childrenCount <= (childEndIdx - childBeginIdx));
	if(childrenCount > 0) {
		if(p_layout->flex_dim > 0 && p_layout->extra_flex_dim > 0) {
			// If the container has a positive flexible space, let's add to it the sizes of all flexible children.
			p_layout->flex_dim += p_layout->extra_flex_dim;
		}
		// Determine the main axis initial position and optional spacing.
		float pos = 0.0f;
		float spacing = 0.0f;
		if(p_layout->flex_grows == 0 && p_layout->flex_dim > 0.0f) {
			const bool lar = layout_align(ALB.JustifyContent, p_layout->flex_dim, childrenCount, &pos, &spacing, false);
			assert(lar && "incorrect justify_content");
			if(p_layout->reverse)
				pos = p_layout->size_dim - pos;
		}
		if(p_layout->reverse)
			pos -= p_layout->vertical ? ALB.Padding.b.Y : ALB.Padding.b.X;
		else 
			pos += p_layout->vertical ? ALB.Padding.a.Y : ALB.Padding.a.X;
		if(p_layout->wrap && p_layout->reverse2) {
			p_layout->pos2 -= p_layout->line_dim;
		}
		for(uint i = childBeginIdx; i < childEndIdx; i++) {
			LayoutFlexItem & r_child = p_layout->GetChildByIndex(this, i);
			if(!r_child.ALB.IsPositionAbsolute(ALB.GetContainerDirection())) { // Isn't already positioned
				// Grow or shrink the main axis item size if needed.
				float flex_size = 0.0f;
				if(p_layout->flex_dim > 0.0f) {
					if(r_child.ALB.GrowFactor != 0.0f) {
						CHILD_SIZE_(p_layout, r_child) = 0; // Ignore previous size when growing.
						flex_size = (p_layout->flex_dim / p_layout->flex_grows) * r_child.ALB.GrowFactor;
					}
				}
				else if(p_layout->flex_dim < 0.0f) {
					if(r_child.ALB.ShrinkFactor != 0.0f) {
						flex_size = (p_layout->flex_dim / p_layout->flex_shrinks) * r_child.ALB.ShrinkFactor;
					}
				}
				CHILD_SIZE_(p_layout, r_child) += flex_size;
				// Set the cross axis position (and stretch the cross axis size if needed).
				float align_size = CHILD_SIZE2_(p_layout, r_child);
				float align_pos = p_layout->pos2 + 0.0f;
				{
					const int ca = GetChildAlign(r_child);
					switch(ca) {
						case AbstractLayoutBlock::alignEnd:
							align_pos += (p_layout->line_dim - align_size - CHILD_MARGIN_XY_(p_layout, r_child, b));
							break;
						case AbstractLayoutBlock::alignCenter:
							align_pos += (p_layout->line_dim / 2.0f) - (align_size / 2.0f) + (CHILD_MARGIN_XY_(p_layout, r_child, a) - CHILD_MARGIN_XY_(p_layout, r_child, b));
							break;
						case AbstractLayoutBlock::alignStretch:
							if(align_size == 0) {
								CHILD_SIZE2_(p_layout, r_child) = p_layout->line_dim - (CHILD_MARGIN_XY_(p_layout, r_child, a) + CHILD_MARGIN_XY_(p_layout, r_child, b));
							}
						// fall through
						case AbstractLayoutBlock::alignStart:
							align_pos += CHILD_MARGIN_XY_(p_layout, r_child, a);
							break;
						default:
							//assert(false && "incorrect align_self");
							align_pos += CHILD_MARGIN_XY_(p_layout, r_child, a); // По умолчанию пусть будет AbstractLayoutBlock::alignStart
							break;
					}
				}
				CHILD_POS2_(p_layout, r_child) = align_pos;
				// Set the main axis position.
				if(p_layout->reverse) {
					pos -= CHILD_MARGIN_YX_(p_layout, r_child, b);
					pos -= CHILD_SIZE_(p_layout, r_child);
					CHILD_POS_(p_layout, r_child) = pos;
					pos -= spacing;
					pos -= CHILD_MARGIN_YX_(p_layout, r_child, a);
				}
				else {
					pos += CHILD_MARGIN_YX_(p_layout, r_child, a);
					CHILD_POS_(p_layout, r_child) = pos;
					pos += CHILD_SIZE_(p_layout, r_child);
					pos += spacing;
					pos += CHILD_MARGIN_YX_(p_layout, r_child, b);
				}
				if(r_child.R.Frame[2] == 0.0f && r_child.ALB.SzX == AbstractLayoutBlock::szUndef) {
					if(r_child.ALB.AspectRatio > 0.0 && r_child.R.Frame[3] > 0.0f) {
						r_child.R.Frame[2] = r_child.R.Frame[3] / r_child.ALB.AspectRatio;
					}
				}
				if(r_child.R.Frame[3] == 0.0f && r_child.ALB.SzY == AbstractLayoutBlock::szUndef) {
					if(r_child.ALB.AspectRatio > 0.0 && r_child.R.Frame[2] > 0.0f) {
						r_child.R.Frame[3] = r_child.R.Frame[2] * r_child.ALB.AspectRatio;
					}
				}
				// Now that the item has a frame, we can layout its children.
				r_child.DoLayoutSelf();
			}
		}
		if(p_layout->wrap && !p_layout->reverse2) {
			p_layout->pos2 += p_layout->line_dim;
		}
		if(p_layout->need_lines) {
			p_layout->P_Lines = static_cast<LayoutFlex::Line *>(SAlloc::R(p_layout->P_Lines, sizeof(LayoutFlex::Line) * (p_layout->lines_count + 1)));
			assert(p_layout->P_Lines != NULL);
			LayoutFlex::Line * p_new_line = &p_layout->P_Lines[p_layout->lines_count];
			p_new_line->child_begin = childBeginIdx;
			p_new_line->child_end = childEndIdx;
			p_new_line->size = p_layout->line_dim;
			p_layout->lines_count++;
			p_layout->lines_sizes += p_new_line->size;
		}
	}
}

void LayoutFlexItem::DoLayoutSelf()
{
	DoLayout(R.Frame[2], R.Frame[3]);
}

void LayoutFlexItem::DoLayout(float _width, float _height)
{
	if(getCount() && oneof2(ALB.GetContainerDirection(), DIREC_HORZ, DIREC_VERT)) {
		LayoutFlex layout_s(this, _width, _height);
		// @ctr layout_s.Init(this, _width, _height);
		// @ctr layout_s.Z(); //LAYOUT_RESET();
		uint last_layout_child = 0;
		uint relative_children_count = 0;
		for(uint i = 0; i < getCount(); i++) {
			LayoutFlexItem & r_child = layout_s.GetChildByIndex(this, i);
			// Items with an absolute position have their frames determined
			// directly and are skipped during layout.
			if(r_child.ALB.IsPositionAbsoluteX() && r_child.ALB.IsPositionAbsoluteY()) {
				float child_width = r_child.ALB.GetAbsoluteSizeX();
				float child_height = r_child.ALB.GetAbsoluteSizeY();
				float child_x = r_child.ALB.GetAbsoluteLowX();
				float child_y = r_child.ALB.GetAbsoluteLowY();
				r_child.R.Frame[0] = child_x;
				r_child.R.Frame[1] = child_y;
				r_child.R.Frame[2] = child_width;
				r_child.R.Frame[3] = child_height;
				// Now that the item has a frame, we can layout its children.
				r_child.DoLayoutSelf(); // @recursion
			}
			else {
				// Initialize frame.
				r_child.R.Frame[0] = 0.0f;
				r_child.R.Frame[1] = 0.0f;
				r_child.R.Frame[2] = r_child.ALB.CalcEffectiveSizeX(_width);
				r_child.R.Frame[3] = r_child.ALB.CalcEffectiveSizeY(_height);
				//
				// Main axis size defaults to 0.
				// if(fisnanf(CHILD_SIZE_((&layout_s), r_child))) { CHILD_SIZE_((&layout_s), r_child) = 0; }
				//
				// Cross axis size defaults to the parent's size (or line size in wrap
				// mode, which is calculated later on).
				if(fisnanf(CHILD_SIZE2_((&layout_s), r_child))) {
					if(layout_s.wrap)
						layout_s.need_lines = true;
					else {
						CHILD_SIZE2_((&layout_s), r_child) = (layout_s.vertical ? _width : _height) - CHILD_MARGIN_XY_((&layout_s), r_child, a) - CHILD_MARGIN_XY_((&layout_s), r_child, b);
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
						if(size_off != layout_s.frame_size2_i || GetChildAlign(r_child) != AbstractLayoutBlock::alignStretch) {
							float val = size[j];
							if(!fisnanf(val))
								r_child.R.Frame[size_off] = val;
						}
					}
				}
				// Honor the `basis' property which overrides the main-axis size.
				if(r_child.ALB.Basis > 0.0f) {
					//assert(r_child.ALB.Basis >= 0.0f);
					CHILD_SIZE_((&layout_s), r_child) = r_child.ALB.Basis;
				}
				const float child_size = CHILD_SIZE_((&layout_s), r_child);
				if(layout_s.wrap) {
					if(layout_s.flex_dim < child_size) {
						// Not enough space for this child on this line, layout the
						// remaining items and move it to a new line.
						DoLayoutChildren(last_layout_child, i, relative_children_count, &layout_s);
						layout_s.Reset();
						last_layout_child = i;
						relative_children_count = 0;
					}
					float child_size2 = CHILD_SIZE2_((&layout_s), r_child);
					if(!fisnanf(child_size2) && child_size2 > layout_s.line_dim) {
						layout_s.line_dim = child_size2;
					}
				}
				assert(r_child.ALB.GrowFactor >= 0.0f);
				assert(r_child.ALB.ShrinkFactor >= 0.0f);
				layout_s.flex_grows   += r_child.ALB.GrowFactor;
				layout_s.flex_shrinks += r_child.ALB.ShrinkFactor;
				layout_s.flex_dim     -= (child_size + (CHILD_MARGIN_YX_((&layout_s), r_child, a) + CHILD_MARGIN_YX_((&layout_s), r_child, b)));
				relative_children_count++;
				if(child_size > 0 && r_child.ALB.GrowFactor > 0.0f)
					layout_s.extra_flex_dim += child_size;
			}
		}
		// Layout remaining items in wrap mode, or everything otherwise.
		DoLayoutChildren(last_layout_child, getCount(), relative_children_count, &layout_s);
		// In wrap mode we may need to tweak the position of each line according to
		// the align_content property as well as the cross-axis size of items that
		// haven't been set yet.
		if(layout_s.need_lines && layout_s.lines_count > 0) {
			float pos = 0.0f;
			float spacing = 0.0f;
			const float flex_dim = layout_s.align_dim - layout_s.lines_sizes;
			if(flex_dim > 0) {
				const bool lar = layout_align(ALB.AlignContent, flex_dim, layout_s.lines_count, &pos, &spacing, true);
				assert(lar && "incorrect align_content");
			}
			float old_pos = 0.0f;
			if(layout_s.reverse2) {
				pos = layout_s.align_dim - pos;
				old_pos = layout_s.align_dim;
			}
			for(uint i = 0; i < layout_s.lines_count; i++) {
				const LayoutFlex::Line * p_line = &layout_s.P_Lines[i];
				if(layout_s.reverse2) {
					pos -= p_line->size;
					pos -= spacing;
					old_pos -= p_line->size;
				}
				// Re-position the children of this line, honoring any child
				// alignment previously set within the line.
				for(uint j = p_line->child_begin; j < p_line->child_end; j++) {
					LayoutFlexItem & r_child = layout_s.GetChildByIndex(this, j);
					if(!r_child.ALB.IsPositionAbsolute(AbstractLayoutBlock::GetCrossDirection(ALB.GetContainerDirection()))) { // Should not be re-positioned.
						if(fisnanf(CHILD_SIZE2_((&layout_s), r_child))) {
							// If the child's cross axis size hasn't been set it, it defaults to the line size.
							CHILD_SIZE2_((&layout_s), r_child) = p_line->size + ((ALB.AlignContent == AbstractLayoutBlock::alignStretch) ? spacing : 0);
						}
						CHILD_POS2_((&layout_s), r_child) = pos + (CHILD_POS2_((&layout_s), r_child) - old_pos);
					}
				}
				if(!layout_s.reverse2) {
					pos += p_line->size;
					pos += spacing;
					old_pos += p_line->size;
				}
			}
		}
		{
			ZFREE(layout_s.ordered_indices);
			ZFREE(layout_s.P_Lines);
			layout_s.lines_count = 0;
		}
	}
}

int AbstractLayoutBlock::SetVArea(int area)
{
	int    ok = 1;
	uint16 gx = 0;
	uint16 gy = 0;
	switch(area) {
		case areaCornerLU:
			gx = SIDE_LEFT;
			gy = SIDE_TOP;
			break;
		case areaCornerRU:
			gx = SIDE_RIGHT;
			gy = SIDE_TOP;
			break;
		case areaCornerRB:
			gx = SIDE_RIGHT;
			gy = SIDE_BOTTOM;
			break;
		case areaCornerLB:
			gx = SIDE_LEFT;
			gy = SIDE_BOTTOM;
			break;
		case areaSideU:
			gx = SIDE_CENTER;
			gy = SIDE_TOP;
			break;
		case areaSideR:
			gx = SIDE_RIGHT;
			gy = SIDE_CENTER;
			break;
		case areaSideB:
			gx = SIDE_CENTER;
			gy = SIDE_BOTTOM;
			break;
		case areaSideL:
			gx = SIDE_LEFT;
			gy = SIDE_CENTER;
			break;
		case areaCenter:
			gx = SIDE_CENTER;
			gy = SIDE_CENTER;
			break;
		default:
			ok = 0;
			break;
	}
	if(ok) {
		if(GravityX != gx || GravityY != gy) {
			GravityX = gx;
			GravityY = gy;
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

void LayoutFlexItem::DoFloatLayout(float _width, float _height) // @construction
{
	if(getCount()) {
		//
		// Разбиваем всю область на 9 виртуальных зон (углы, стороны, центр)
		//
		FRect area_rect[9];
		int   bypass_direction[9]; // Последовательность обхода зон для заполнения // 
		{
			// Все виртуальные зоны инициализируем величиной полной области.
			for(uint i = 0; i < SIZEOFARRAY(area_rect); i++) {
				area_rect[i].a.SetZero();
				area_rect[i].b.Set(_width, _height);
			}
		}
		{
			// Порядок обхода инициализируем неопределенными значениями
			for(uint i = 0; i < SIZEOFARRAY(bypass_direction); i++)
				bypass_direction[i] = -1;
		}
		{
			LAssocArray item_map; // Карта распределения элементов по областям {itemIdx; area}
			LayoutFlex layout_s(this, _width, _height);
			{
				for(uint cidx = 0; cidx < getCount(); cidx++) {
					const LayoutFlexItem & r_child = layout_s.GetChildByIndex(this, cidx);
					int va = r_child.ALB.GetVArea();
					if(va >= 0 && va <= 9) {
						item_map.Add(cidx, va);
					}
				}
			}
			{
				//
				// Сортируем массив так, чтобы элементы, относящиеся к одинаковой области держались вместе
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
					assert(cidx >= 0 && cidx < getCount());
					LayoutFlexItem & r_child = layout_s.GetChildByIndex(this, cidx);
					if(area != prev_area) {
						if(p_current_layout) {
							assert(prev_area >= 0 && prev_area < SIZEOFARRAY(area_rect));
							p_current_layout->Evaluate();
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
							const float offs_x = area_rect[prev_area].a.X;
							const float offs_y = area_rect[prev_area].a.Y;
							for(uint ci = 0; ci < p_current_layout->getCount(); ci++) {
								LayoutFlexItem * p_layout_item = p_current_layout->at(ci);
								if(p_layout_item && p_layout_item->P_Link) {
									p_layout_item->P_Link->R.CopyWithOffset(p_layout_item->R, offs_x, offs_y);
									p_layout_item->P_Link->DoLayoutSelf();
								}
							}
							ZDELETE(p_current_layout);
						}
						{
							p_current_layout = new LayoutFlexItem();
							int    container_direc = DIREC_UNKN;
							uint32 container_flags = AbstractLayoutBlock::fContainerWrap;
							uint16 justify_content = 0; // AbstractLayoutBlock::alignXXX Выравнивание внутренних элементов вдоль основной оси
							uint16 align_content = 0;   // AbstractLayoutBlock::alignXXX Выравнивание внутренних элементов по кросс-оси
							switch(area) {
								case AbstractLayoutBlock::areaCornerLU:
									container_direc = DIREC_HORZ;
									justify_content = AbstractLayoutBlock::alignStart;
									align_content = AbstractLayoutBlock::alignStart;
									break;
								case AbstractLayoutBlock::areaCornerRU:
									container_direc = DIREC_HORZ;
									container_flags |= AbstractLayoutBlock::fContainerReverseDir;
									justify_content = AbstractLayoutBlock::alignStart;
									align_content = AbstractLayoutBlock::alignStart;
									break;
								case AbstractLayoutBlock::areaCornerRB:
									container_direc = DIREC_HORZ;
									container_flags |= AbstractLayoutBlock::fContainerReverseDir;
									container_flags |= AbstractLayoutBlock::fContainerWrapReverse;
									justify_content = AbstractLayoutBlock::alignStart;
									align_content = AbstractLayoutBlock::alignStart;
									break;
								case AbstractLayoutBlock::areaCornerLB:
									container_direc = DIREC_HORZ;
									justify_content = AbstractLayoutBlock::alignStart;
									align_content = AbstractLayoutBlock::alignEnd;
									//container_flags |= AbstractLayoutBlock::fContainerWrapReverse;
									break;
								case AbstractLayoutBlock::areaSideU:
									container_direc = DIREC_HORZ;
									justify_content = AbstractLayoutBlock::alignStart;
									align_content = AbstractLayoutBlock::alignStart;
									break;
								case AbstractLayoutBlock::areaSideR:
									container_direc = DIREC_VERT;
									justify_content = AbstractLayoutBlock::alignStart;
									align_content = AbstractLayoutBlock::alignEnd;
									break;
								case AbstractLayoutBlock::areaSideB:
									container_direc = DIREC_HORZ;
									justify_content = AbstractLayoutBlock::alignStart;
									align_content = AbstractLayoutBlock::alignEnd;
									break;
								case AbstractLayoutBlock::areaSideL:
									container_direc = DIREC_VERT;
									justify_content = AbstractLayoutBlock::alignStart;
									align_content = AbstractLayoutBlock::alignStart;
									break;
								case AbstractLayoutBlock::areaCenter:
									container_direc = DIREC_HORZ; // @?
									justify_content = AbstractLayoutBlock::alignStart; // @?
									align_content = AbstractLayoutBlock::alignStart; // @?
									break;
							}
							p_current_layout->ALB.SetContainerDirection(container_direc);
							p_current_layout->ALB.JustifyContent = justify_content;
							p_current_layout->ALB.AlignContent = align_content;
							p_current_layout->ALB.Flags |= container_flags;
							p_current_layout->ALB.SetFixedSizeX(area_rect[area].Width());
							p_current_layout->ALB.SetFixedSizeY(area_rect[area].Height()); 
						}
					}
					{
						assert(p_current_layout);
						LayoutFlexItem * p_layout_item = p_current_layout->CreateNewItem();
						p_layout_item->ALB = r_child.ALB;
						p_layout_item->CbSelfSizing = r_child.CbSelfSizing;
						p_layout_item->CbSetup = r_child.CbSetup;
						p_layout_item->managed_ptr = r_child.managed_ptr;
						p_layout_item->P_Link = &r_child;
					}
					prev_area = area;
				}
				if(p_current_layout) {
					assert(prev_area >= 0 && prev_area < SIZEOFARRAY(area_rect));
					p_current_layout->Evaluate();
					const float offs_x = area_rect[prev_area].a.X;
					const float offs_y = area_rect[prev_area].a.Y;
					for(uint ci = 0; ci < p_current_layout->getCount(); ci++) {
						LayoutFlexItem * p_layout_item = p_current_layout->at(ci);
						if(p_layout_item && p_layout_item->P_Link) {
							p_layout_item->P_Link->R.CopyWithOffset(p_layout_item->R, offs_x, offs_y);
							p_layout_item->P_Link->DoLayoutSelf();
						}
					}
					// @todo Здесь с помощью assert'ов необходимо проверить что мы обошли все зоны
					ZDELETE(p_current_layout);
				}
			}
		}
	}
}

static void FASTCALL SetupItem(LayoutFlexItem & rItem)
{
	if(rItem.CbSetup) {
		rItem.CbSetup(&rItem, rItem.R.Frame);
	}
	for(uint i = 0; i < rItem.getCount(); i++) {
		SetupItem(*rItem.at(i)); // @recursion
	}
}

int LayoutFlexItem::Evaluate()
{
	int    ok = -1;
	if(getCount() && !P_Parent) {
		float sx = 0.0f;
		float sy = 0.0f;
		const int stag_x = ALB.GetSizeX(&sx);
		const int stag_y = ALB.GetSizeY(&sy);
		const int cdir = ALB.GetContainerDirection();
		if(oneof2(cdir, DIREC_HORZ, DIREC_VERT)) {
			//if(!fisnan(Size.X) && !fisnan(Size.Y) && !CbSelfSizing) {
			if(stag_x == AbstractLayoutBlock::szFixed && stag_y == AbstractLayoutBlock::szFixed && !CbSelfSizing) {
				assert(P_Parent == NULL);
				assert(CbSelfSizing == NULL);
				DoLayout(sx, sy);
				//SetupItem(pItem);
				{
					for(uint i = 0; i < getCount(); i++) {
						SetupItem(*at(i));
					}
				}
				ok = 1;
			}
		}
		else { // @construction					
			if(stag_x == AbstractLayoutBlock::szFixed && stag_y == AbstractLayoutBlock::szFixed && !CbSelfSizing) {
				DoFloatLayout(sx, sy);
				{
					for(uint i = 0; i < getCount(); i++) {
						SetupItem(*at(i));
					}
				}
				ok = 1;
			}
		}
	}
	return ok;
}
