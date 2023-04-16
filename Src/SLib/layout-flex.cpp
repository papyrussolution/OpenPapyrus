// LAYOUT-FLEX.CPP
// Copyright (c) A.Sobolev 2020, 2021, 2022, 2023
// @codepage UTF-8
//
// The code of Microsoft Flex is partialy used (https://github.com/xamarin/flex.git)
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See the LICENSE.txt file in the project root for the license information.
//
#include <slib-internal.h>
#pragma hdrstop

SUiLayoutParam::SUiLayoutParam()
{
	SetDefault();
}

SUiLayoutParam::SUiLayoutParam(int direction, uint justifyContent, uint alignContent)
{
	assert(oneof2(direction, DIREC_HORZ, DIREC_VERT));
	assert(oneof8(justifyContent, alignAuto, alignStretch, alignCenter, alignStart, alignEnd, alignSpaceBetween, alignSpaceAround, alignSpaceEvenly));
	assert(oneof8(alignContent, alignAuto, alignStretch, alignCenter, alignStart, alignEnd, alignSpaceBetween, alignSpaceAround, alignSpaceEvenly));
	SetDefault();
	SetContainerDirection(direction);
	if(justifyContent != alignAuto && oneof7(justifyContent, alignStretch, alignCenter, alignStart, alignEnd, alignSpaceBetween, alignSpaceAround, alignSpaceEvenly)) {
		JustifyContent = justifyContent;
	}
	if(alignContent != alignAuto && oneof7(alignContent, alignStretch, alignCenter, alignStart, alignEnd, alignSpaceBetween, alignSpaceAround, alignSpaceEvenly)) {
		AlignContent = alignContent;
	}
}

SUiLayoutParam & SUiLayoutParam::SetDefault()
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

bool FASTCALL SUiLayoutParam::operator == (const SUiLayoutParam & rS) const { return IsEq(rS); }
bool FASTCALL SUiLayoutParam::operator != (const SUiLayoutParam & rS) const { return !IsEq(rS); }

bool FASTCALL SUiLayoutParam::IsEq(const SUiLayoutParam & rS) const
{
	#define I(f) if(f != rS.f) return false;
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
	return true;
}

int SUiLayoutParam::Validate() const
{
	int    ok = 1;
	if(SzX == szFixed && IsNominalFullDefinedX()) {
		if(!feqeps(Size.x, Nominal.Width(), 0.01))
			ok = 0;
	}
	if(SzX == szByContainer) {
		if(Size.x < 0.0f || Size.x > 1.0f)
			ok = 0;
	}
	if(SzY == szFixed && IsNominalFullDefinedY()) {
		if(!feqeps(Size.y, Nominal.Height(), 0.01))
			ok = 0;
	}
	if(SzY == szByContainer) {
		if(Size.y < 0.0f || Size.y > 1.0f)
			ok = 0;
	}	
	if(!oneof4(GravityX, 0, SIDE_LEFT, SIDE_RIGHT, SIDE_CENTER))
		ok = 0;
	if(!oneof4(GravityY, 0, SIDE_TOP, SIDE_BOTTOM, SIDE_CENTER))
		ok = 0;
	return ok;
}

/*static*/uint32 SUiLayoutParam::GetSerializeSignature() { return 0x15DE0522U; }

int SUiLayoutParam::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	//const  uint32 valid_signature = 0x15DE0522U;
	const  uint32 valid_version = 0U;
	uint32 signature = SUiLayoutParam::GetSerializeSignature(); // Сигнатура для сериализации
	uint32 version = 0;   // Версия сериализации
	THROW(pSCtx->Serialize(dir, signature, rBuf));
	if(dir < 0) {
		THROW(signature == SUiLayoutParam::GetSerializeSignature());
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

int SUiLayoutParam::GetSizeX(float * pS) const
{
	int   result = szInvalid;
	float s = 0.0f;
	if(SzX == szFixed) {
		s = Size.x;
		result = szFixed;
	}
	if(SzX == szByContainer) {
		if(Size.x > 0.0f && Size.x <= 1.0f)
			s = Size.x;
		result = SzX;
	}
	else if(oneof2(SzX, szByContent, szUndef))
		result = SzX;
	ASSIGN_PTR(pS, s);
	return result;
}

int SUiLayoutParam::GetSizeY(float * pS) const
{
	int   result = szInvalid;
	float s = 0.0f;
	if(SzY == szFixed) {
		s = Size.y;
		result = szFixed;
	}
	else if(SzY == szByContainer) {
		if(Size.y > 0.0f && Size.y <= 1.0f)
			s = Size.y;
		result = SzY;
	}
	else if(oneof2(SzY, szByContent, szUndef))
		result = SzY;
	ASSIGN_PTR(pS, s);
	return result;
}

int SUiLayoutParam::GetSizeByContainerX(float containerSize, float * pS) const
{
	int   ok = 0;
	float result_size = 0.0f;
	if(SzX == szByContainer) {
		if(containerSize > 0.0f) {
			if(Size.x > 0.0f && Size.x <= 1.0f) {
				result_size = (containerSize * Size.x) - (Margin.a.x + Margin.b.x); // @v11.0.7 (- (Margin.a.x + Margin.b.x))
				ok = 1;
			}
			else if(Size.x == 0.0f) {
				result_size = (containerSize);
				ok = 1;
			}
		}
	}
	if(ok)
		ASSIGN_PTR(pS, result_size);
	return ok;
}

int SUiLayoutParam::GetSizeByContainerY(float containerSize, float * pS) const
{
	int   ok = 0;
	float result_size = 0.0f;
	if(SzY == szByContainer) {
		if(containerSize > 0.0f) {
			if(Size.y > 0.0f && Size.y <= 1.0f) {
				result_size = (containerSize * Size.y) - (Margin.a.y + Margin.b.y); // @v11.0.7 (- (Margin.a.y + Margin.b.y))
				ok = 1;
			}
			else if(Size.y == 0.0f) {
				result_size = (containerSize);
				ok = 1;
			}
		}
	}
	if(ok)
		ASSIGN_PTR(pS, result_size);
	return ok;
}

SPoint2F SUiLayoutParam::CalcEffectiveSizeXY(float containerSizeX, float containerSizeY) const
{
	SPoint2F result;
	result.x = CalcEffectiveSizeX(containerSizeX);
	result.y = CalcEffectiveSizeY(containerSizeY);
	if(result.x == 0.0f && AspectRatio > 0.0f && result.y > 0.0f) {
		result.x = result.y / AspectRatio;
	}
	else if(result.y == 0.0f && AspectRatio > 0.0f && result.x > 0.0f) {
		result.y = result.x * AspectRatio;
	}
	return result;
}

float SUiLayoutParam::CalcEffectiveSizeX(float containerSize) const
{
	float result = 0.0f;
	if(SzX == SUiLayoutParam::szFixed) {
		result = Size.x;
	}
	else if(GetSizeByContainerX(containerSize, &result)) {
		; // @todo Тут, вероятно, надо поля и отступы учесть
	}
	else if(SzX == SUiLayoutParam::szByContent) {
		result = 0.0f; // @todo
	}
	else {
		//
		// @todo Если установлен AspectRatio, то следует учесть вариант рассчитанного перед вызовом этой функции кросс-размера
		//
		if(SzY == SUiLayoutParam::szFixed && AspectRatio > 0.0f) {
			result = Size.y / AspectRatio;
		}
		else {
			result = 0.0f; // @todo
		}
	}
	return result;
}

float SUiLayoutParam::CalcEffectiveSizeY(float containerSize) const
{
	float result = 0.0f;
	if(SzY == SUiLayoutParam::szFixed) {
		result = Size.y;
	}
	else if(GetSizeByContainerY(containerSize, &result)) {
		; // @todo Тут, вероятно, надо поля и отступы учесть
	}
	else if(SzY == SUiLayoutParam::szByContent) {
		result = 0.0f; // @todo 
	}
	else {
		//
		// @todo Если установлен AspectRatio, то следует учесть вариант рассчитанного перед вызовом этой функции кросс-размера
		//
		if(SzX == SUiLayoutParam::szFixed && AspectRatio > 0.0f) {
			result = Size.x * AspectRatio;
		}
		else {
			result = 0.0f; // @todo 
		}
	}
	return result;
}

void SUiLayoutParam::SetFixedSizeX(float s)
{
	//assert(!fisnanf(s) && s >= 0.0f);
	if(fisnan(s) || s < 0.0f)
		s = 0.0f;
	Size.x = s;
	SzX = szFixed;
}

void SUiLayoutParam::SetVariableSizeX(uint var/* szXXX */, float s)
{
	assert(oneof3(var, szUndef, szByContent, szByContainer));
	if(var == szByContainer && (s > 0.0f && s <= 1.0f))
		Size.x = s;
	else if(var == szByContainer && s == 0.0f)
		Size.x = 1.0f;
	else
		Size.x = 0.0f;
	SzX = var;
}

void SUiLayoutParam::SetFixedSizeY(float s)
{
	//assert(!fisnanf(s) && s >= 0.0f);
	if(fisnanf(s) || s < 0.0f)
		s = 0.0f;
	Size.y = s;
	SzY = szFixed;
}

void SUiLayoutParam::SetFixedSize(const TRect & rR)
{
	SetFixedSizeX(static_cast<float>(rR.width()));
	SetFixedSizeY(static_cast<float>(rR.height()));
}

void SUiLayoutParam::SetVariableSizeY(uint var/* szXXX */, float s)
{
	assert(oneof3(var, szUndef, szByContent, szByContainer));
	if(var == szByContainer && (s > 0.0f && s <= 1.0f))
		Size.y = s;
	else if(var == szByContainer && s == 0.0f)
		Size.y = 1.0f;
	else
		Size.y = 0.0f;
	SzY = var;
}

int SUiLayoutParam::GetContainerDirection() const // returns DIREC_HORZ || DIREC_VERT || DIREC_UNKN
{
	if((Flags & (fContainerRow|fContainerCol)) == fContainerRow)
		return DIREC_HORZ;
	else if((Flags & (fContainerRow|fContainerCol)) == fContainerCol)
		return DIREC_VERT;
	else
		return DIREC_UNKN;
}

void SUiLayoutParam::SetContainerDirection(int direc /*DIREC_XXX*/)
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

/*static*/SString & SUiLayoutParam::MarginsToString(const FRect & rR, SString & rBuf)
{
	rBuf.Z();
	if(!rR.IsEmpty()) {
		if(rR.a == rR.b) {
			if(rR.a.x == rR.a.y)
				rBuf.Cat(rR.a.x);
			else
				rBuf.Cat(rR.a.x).Comma().Cat(rR.a.y);
		}
		else
			rBuf.Cat(rR.a.x).Comma().Cat(rR.a.y).Comma().Cat(rR.b.x).Comma().Cat(rR.b.y);
	}
	return rBuf;
}

/*static*/int SUiLayoutParam::MarginsFromString(const char * pBuf, FRect & rR)
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
							rR.a.x = v;
							rR.b.x = v;
						}
						else // _c == 4
							rR.a.x = v;
					}
					else if(tokn == 2) {
						if(_c == 2) {
							rR.a.y = v;
							rR.b.y = v;
						}
						else // _c == 4
							rR.a.y = v;
					}
					else if(tokn == 3)
						rR.b.x = v;
					else if(tokn == 4)
						rR.b.y = v;
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

SString & SUiLayoutParam::SizeToString(SString & rBuf) const
{
	rBuf.Z();
	SPoint2F s;
	int  szx = GetSizeX(&s.x);
	int  szy = GetSizeY(&s.y);
	if(szx != szUndef || szy != szUndef) {
		switch(szx) {
			case szFixed: rBuf.Cat(s.x, MKSFMTD(0, 3, NMBF_NOTRAILZ)); break;
			case szUndef: rBuf.Cat("undef"); break;
			case szByContent: rBuf.Cat("content"); break;
			case szByContainer: 
				if(s.x > 0.0f && s.x <= 1.0f)
					rBuf.Cat(s.x * 100.0f, MKSFMTD(0, 3, NMBF_NOTRAILZ)).CatChar('%');
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
			case szFixed: rBuf.Cat(s.y, MKSFMTD(0, 3, NMBF_NOTRAILZ)); break;
			case szUndef: rBuf.Cat("undef"); break;
			case szByContent: rBuf.Cat("content"); break;
			case szByContainer: 
				if(s.y > 0.0f && s.y <= 1.0f)
					rBuf.Cat(s.y * 100.0f, MKSFMTD(0, 3, NMBF_NOTRAILZ)).CatChar('%');
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

int SUiLayoutParam::ParseSizeStr(const SString & rStr, float & rS) const
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
				if(SIEEE754::Scan(temp_buf.cptr(), &p_end, &val, &erange) && !fisnan(val)) {
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

int SUiLayoutParam::SizeFromString(const char * pBuf)
{
	int    ok = 1;
	SString input(pBuf);
	input.Strip();
	SString x_buf, y_buf;
	if(input.Divide(',', x_buf, y_buf) > 0 || input.Divide(';', x_buf, y_buf) > 0 || input.Divide(' ', x_buf, y_buf) > 0) {
		SPoint2F s;
		int szx = ParseSizeStr(x_buf, s.x);
		int szy = ParseSizeStr(y_buf, s.y);
		if(szx == szFixed)
			SetFixedSizeX(s.x);
		else if(szx != szInvalid)
			SetVariableSizeX(szx, s.x);
		else
			SetVariableSizeX(szUndef, 0.0f);
		if(szy == szFixed)
			SetFixedSizeY(s.y);
		else if(szy != szInvalid)
			SetVariableSizeY(szy, s.y);
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
bool SUiLayoutParam::IsNominalFullDefinedX() const { return ((Flags & (fNominalDefL|fNominalDefR)) == (fNominalDefL|fNominalDefR)); }
//
// Descr: Определяет являются ли координаты по оси Y фиксированными.
//
bool SUiLayoutParam::IsNominalFullDefinedY() const { return ((Flags & (fNominalDefT|fNominalDefB)) == (fNominalDefT|fNominalDefB)); }
//
// Descr: Вспомогательная функция, возвращающая кросс-направление относительно заданного
//   направления direction.
//   Если direction == DIREC_HORZ, то возвращает DIREC_VERT; если direction == DIREC_VERT, то возвращает DIREC_HORZ.
//   Если !oneof2(direction, DIREC_HORZ, DIREC_VERT) то возвращает DIREC_UNKN.
//
/*static*/int SUiLayoutParam::GetCrossDirection(int direction)
{
	assert(oneof2(direction, DIREC_HORZ, DIREC_VERT));
	return (direction == DIREC_HORZ) ? DIREC_VERT : ((direction == DIREC_VERT) ? DIREC_HORZ : DIREC_UNKN);
}
//
// Descr: Определяет является ли позиция элемента абсолютной вдоль направления direction.
// ARG(direction IN): DIREC_HORZ || DIREC_VERT
//
bool SUiLayoutParam::IsPositionAbsolute(int direction) const
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
bool SUiLayoutParam::IsPositionAbsoluteX() const
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

float SUiLayoutParam::GetAbsoluteLowX() const
{
	float result = 0.0f;
	if(Flags & fNominalDefL)
		result = Nominal.a.x;
	else if(Flags & fNominalDefR && SzX == szFixed)
		result = (Nominal.b.x - Size.x);
	return result;
}

float SUiLayoutParam::GetAbsoluteLowY() const
{
	float result = 0.0f;
	if(Flags & fNominalDefT)
		result = Nominal.a.y;
	else if(Flags & fNominalDefB && SzY == szFixed)
		result = (Nominal.b.y - Size.y);
	return result;
}

float SUiLayoutParam::GetAbsoluteSizeX() const
{
	float result = 0.0f;
	if(SzX == szFixed)
		result = Size.x;
	else if(IsNominalFullDefinedX())
		result = Nominal.Width();
	return result;
}

float SUiLayoutParam::GetAbsoluteSizeY() const
{
	float result = 0.0f;
	if(SzY == szFixed)
		result = Size.y;
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
bool SUiLayoutParam::IsPositionAbsoluteY() const
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
SUiLayout::RefCollection::RefCollection() : SCollection(aryPtrContainer|aryDataOwner)
{
}

SUiLayout::RefCollection & SUiLayout::RefCollection::Z()
{
	SCollection::freeAll();
	return *this;
}

void SUiLayout::RefCollection::Add(const SUiLayout * pLo)
{
	if(pLo)
		SCollection::insert(pLo);
}

uint SUiLayout::RefCollection::GetCount() const { return SCollection::getCount(); }

const SUiLayout * SUiLayout::RefCollection::Get(uint idx) const
{
	return static_cast<const SUiLayout *>(SCollection::at(idx));
}
//
//
//
SUiLayout::Result::Result() : Flags(0), P_Scrlr(0)
{
	memzero(Frame, sizeof(Frame));
}

SUiLayout::Result::Result(const Result & rS) : Flags(rS.Flags), P_Scrlr(0)
{
	memcpy(Frame, rS.Frame, sizeof(Frame));
	if(rS.P_Scrlr) {
		P_Scrlr = new SScroller(*rS.P_Scrlr);
	}
}

SUiLayout::Result::~Result()
{
	delete P_Scrlr;
}

SUiLayout::Result & FASTCALL SUiLayout::Result::operator = (const Result & rS)
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

SUiLayout::Result::operator FRect() const
{
	return FRect(Frame[0], Frame[1], Frame[0] + Frame[2], Frame[1] + Frame[3]);
}

SUiLayout::Result & SUiLayout::Result::CopyWithOffset(const SUiLayout::Result & rS, float offsX, float offsY)
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

SUiLayout::SUiLayout() : Signature(_SlConst.SUiLayoutSignature), P_Parent(0), P_Link(0), managed_ptr(0), 
	CbSelfSizing(0), CbSetup(0), State(0), ALB(), P_HgL(0), P_Children(0), ID(0)
{
}

SUiLayout::SUiLayout(const SUiLayoutParam & rP) : Signature(_SlConst.SUiLayoutSignature), P_Parent(0), P_Link(0), managed_ptr(0), 
	CbSelfSizing(0), CbSetup(0), State(0), ALB(), P_HgL(0), P_Children(0), ID(0)
{
	SetLayoutBlock(rP);
}

SUiLayout::~SUiLayout()
{
	ZDELETE(P_Children);
	ZDELETE(P_HgL);
	managed_ptr = 0; // @v11.0.0
	CbSelfSizing = 0; // @v11.0.0
	CbSetup = 0; // @v11.0.0
	P_Link = 0;
}

bool SUiLayout::IsConsistent() const { return (this != 0 && Signature == _SlConst.SUiLayoutSignature); }

int SUiLayout::GetID() const { return ID; }

int SUiLayout::SetID(int id)
{
	if(id > 0) {
		ID = id;
		return ID;
	}
	else
		return 0;
}
//
// Descr: Если идентификатора элемента (ID) нулевой, то инициализирует его так, чтобы он был уникальным
//   в области определения контейнера верхнего уровня.
//   Если идентфикатор уже не нулевой, то просто возвращает его значение.
//
int SUiLayout::SetupUniqueID()
{
	if(ID == 0) {
		SUiLayout * p_root = GetRoot();
		if(p_root) {
			const int max_id = p_root->GetMaxComponentID();
			ID = (max_id > 0) ? (max_id + 1) : 1;
		}
		else
			ID = 1;
	}
	return ID;
}

int SUiLayout::GetMaxComponentID() const
{
	int    result = GetID();
	const uint _cc = GetChildrenCount();
	for(uint i = 0; i < _cc; i++) {
		const SUiLayout * p_child = GetChildC(i);
		if(p_child) {
			const int ci = p_child->GetMaxComponentID(); // @recursion
			SETMAX(result, ci);
		}
	}
	return result;
}

const SUiLayout * SUiLayout::FindByID(int id) const
{
	const SUiLayout * p_result = 0;
	if(id > 0) {
		if(ID == id)
			p_result = this;
		else {
			const uint _cc = GetChildrenCount();
			if(_cc) {
				for(uint i = 0; !p_result && i < _cc; i++) {
					const SUiLayout * p_child = GetChildC(i);
					if(p_child)
						p_result = p_child->FindByID(id);
				}
			}
		}
	}
	return p_result;
}

/*static*/void * SUiLayout::GetManagedPtr(SUiLayout * pItem)
{
	return pItem ? pItem->managed_ptr : 0;
}

/*static*/void * SUiLayout::GetParentsManagedPtr(SUiLayout * pItem)
{
	return pItem ? GetManagedPtr(pItem->P_Parent) : 0;
}

/*static*/SUiLayout * SUiLayout::CreateComplexLayout(int type/*cmplxtXXX*/, uint flags/*clfXXX*/, float baseFixedMeasure, SUiLayout * pTopLevel)
{
	const float lbl_to_inp_height_ratio = 0.75f;
	const float lbl_to_inp_width_ratio  = 1.5f;
	SUiLayout * p_result = 0/*pTopLevel*/;
	switch(type) {
		case cmplxtInpLbl:
			{
				p_result = pTopLevel ? pTopLevel->InsertItem() : new SUiLayout();
				if(flags & clfLabelLeft) {
					SUiLayoutParam alb(DIREC_HORZ, 0, SUiLayoutParam::alignStart);
					alb.SetVariableSizeX(SUiLayoutParam::szByContainer, 1.0f);
					p_result->SetLayoutBlock(alb);
					{ // Label
						SUiLayoutParam alb;
						alb.GrowFactor = lbl_to_inp_width_ratio;
						if(baseFixedMeasure > 0.0f)
							alb.SetFixedSizeY(baseFixedMeasure * lbl_to_inp_height_ratio);
						else
							alb.SetVariableSizeY(SUiLayoutParam::szByContainer, 1.0f);
						//alb.Margin.Set(4);
						SUiLayout * p_lo_item = p_result->InsertItem(0, &alb);
						p_lo_item->CplxComponentId = cmlxcLabel;
					}
					{ // Input
						SUiLayoutParam alb;
						alb.GrowFactor = 1.0f;
						if(baseFixedMeasure > 0.0f)
							alb.SetFixedSizeY(baseFixedMeasure);
						else
							alb.SetVariableSizeY(SUiLayoutParam::szByContainer, 1.0f);
						//alb.Margin.Set(4);
						SUiLayout * p_lo_item = p_result->InsertItem(0, &alb);
						p_lo_item->CplxComponentId = cmlxcInput;
					}		
				}
				else { // label above
					SUiLayoutParam alb(DIREC_VERT, 0, SUiLayoutParam::alignStart);
					alb.SetVariableSizeX(SUiLayoutParam::szByContainer, 1.0f);
					p_result->SetLayoutBlock(alb);
					{ // Label
						SUiLayoutParam alb;
						alb.SetVariableSizeX(SUiLayoutParam::szByContainer, 1.0f);
						if(baseFixedMeasure > 0.0f)
							alb.SetFixedSizeY(baseFixedMeasure * lbl_to_inp_height_ratio);
						else
							alb.GrowFactor = lbl_to_inp_height_ratio;
						alb.Margin.a.x = 4.0f;
						alb.Margin.a.y = 2.0f;
						alb.Margin.b.x = 4.0f;
						alb.Margin.b.y = 1.0f;
						SUiLayout * p_lo_item = p_result->InsertItem(0, &alb);
						p_lo_item->CplxComponentId = cmlxcLabel;
					}
					{ // Input
						SUiLayoutParam alb;
						alb.SetVariableSizeX(SUiLayoutParam::szByContainer, 1.0f);
						if(baseFixedMeasure > 0.0f)
							alb.SetFixedSizeY(baseFixedMeasure);
						else
							alb.GrowFactor = 1.0f;
						alb.Margin.a.x = 4.0f;
						alb.Margin.a.y = 1.0f;
						alb.Margin.b.x = 4.0f;
						alb.Margin.b.y = 2.0f;
						SUiLayout * p_lo_item = p_result->InsertItem(0, &alb);
						p_lo_item->CplxComponentId = cmlxcInput;
					}		
				}
			}
			break;
		case cmplxtInpLblBtn:
			{
				p_result = pTopLevel ? pTopLevel->InsertItem() : new SUiLayout();
				if(flags & clfLabelLeft) {
					SUiLayoutParam alb(DIREC_HORZ, 0, SUiLayoutParam::alignStart);
					alb.SetVariableSizeX(SUiLayoutParam::szByContainer, 1.0f);
					p_result->SetLayoutBlock(alb);
					{ // Label
						SUiLayoutParam alb;
						alb.GrowFactor = lbl_to_inp_width_ratio;
						if(baseFixedMeasure > 0.0f)
							alb.SetFixedSizeY(baseFixedMeasure * lbl_to_inp_height_ratio);
						else
							alb.SetVariableSizeY(SUiLayoutParam::szByContainer, 1.0f);
						//alb.Margin.Set(4);
						SUiLayout * p_lo_item = p_result->InsertItem(0, &alb);
						p_lo_item->CplxComponentId = cmlxcLabel;
					}
					{ // Input
						SUiLayoutParam alb;
						alb.GrowFactor = 1.0f;
						if(baseFixedMeasure > 0.0f)
							alb.SetFixedSizeY(baseFixedMeasure);
						else
							alb.SetVariableSizeY(SUiLayoutParam::szByContainer, 1.0f);
						//alb.Margin.Set(4);
						SUiLayout * p_lo_item = p_result->InsertItem(0, &alb);
						p_lo_item->CplxComponentId = cmlxcInput;
					}		
					{ // Button
						SUiLayoutParam alb;
						alb.SetVariableSizeX(SUiLayoutParam::szUndef, 0.0f);
						alb.ShrinkFactor = 0.0f;
						alb.AspectRatio = 1.0f;
						alb.SetVariableSizeY(SUiLayoutParam::szByContainer, 1.0f);
						//alb.Margin.Set(4);
						SUiLayout * p_lo_item = p_result->InsertItem(0, &alb);
						p_lo_item->CplxComponentId = cmlxcButton1;
					}
				}
				else { // label above
					SUiLayoutParam alb(DIREC_VERT, 0, SUiLayoutParam::alignStart);
					alb.SetVariableSizeX(SUiLayoutParam::szByContainer, 1.0f);
					p_result->SetLayoutBlock(alb);
					{ // Label
						SUiLayoutParam alb;
						alb.SetVariableSizeX(SUiLayoutParam::szByContainer, 1.0f);
						if(baseFixedMeasure > 0.0f)
							alb.SetFixedSizeY(baseFixedMeasure * lbl_to_inp_height_ratio);
						else
							alb.GrowFactor = lbl_to_inp_height_ratio;
						//alb.Margin.Set(4);
						SUiLayout * p_lo_item = p_result->InsertItem(0, &alb);
						p_lo_item->CplxComponentId = cmlxcLabel;
					}
					{
						SUiLayoutParam alb_frame(DIREC_HORZ, 0, SUiLayoutParam::alignStart);
						alb_frame.SetVariableSizeX(SUiLayoutParam::szByContainer, 1.0f);
						if(baseFixedMeasure > 0.0f)
							alb_frame.SetFixedSizeY(baseFixedMeasure);
						else
							alb_frame.GrowFactor = 1.0f;
						//alb.Margin.Set(4);
						SUiLayout * p_lo_frame = p_result->InsertItem(0, &alb_frame);
						{ // Input
							SUiLayoutParam alb;
							alb.SetVariableSizeX(SUiLayoutParam::szByContainer, 1.0f);
							alb.SetVariableSizeY(SUiLayoutParam::szByContainer, 1.0f);
							//alb.Margin.Set(4);
							SUiLayout * p_lo_item = p_lo_frame->InsertItem(0, &alb);
							p_lo_item->CplxComponentId = cmlxcInput;
						}		
						{ // Button
							SUiLayoutParam alb;
							alb.SetVariableSizeX(SUiLayoutParam::szUndef, 0.0f);
							alb.ShrinkFactor = 0.0f;
							alb.AspectRatio = 1.0f;
							alb.SetVariableSizeY(SUiLayoutParam::szByContainer, 1.0f);
							//alb.Margin.Set(4);
							SUiLayout * p_lo_item = p_lo_frame->InsertItem(0, &alb);
							p_lo_item->CplxComponentId = cmlxcButton1;
						}
					}
				}
			}
			break;
		case cmplxtInpLblBtn2:
			{
				SETIFZ(p_result, new SUiLayout());
				if(flags & clfLabelLeft) {
					SUiLayoutParam alb(DIREC_HORZ, 0, SUiLayoutParam::alignStart);
					alb.SetVariableSizeX(SUiLayoutParam::szByContainer, 1.0f);
					p_result->SetLayoutBlock(alb);
					{ // Label
						SUiLayoutParam alb;
						alb.GrowFactor = 1.0f;
						alb.SetVariableSizeY(SUiLayoutParam::szByContainer, 1.0f);
						//alb.Margin.Set(4);
						SUiLayout * p_lo_item = p_result->InsertItem(0, &alb);
						p_lo_item->CplxComponentId = cmlxcLabel;
					}
					{ // Input
						SUiLayoutParam alb;
						alb.GrowFactor = 1.0f;
						alb.SetVariableSizeY(SUiLayoutParam::szByContainer, 1.0f);
						//alb.Margin.Set(4);
						SUiLayout * p_lo_item = p_result->InsertItem(0, &alb);
						p_lo_item->CplxComponentId = cmlxcInput;
					}		
					{ // Button
						SUiLayoutParam alb;
						alb.SetVariableSizeX(SUiLayoutParam::szUndef, 0.0f);
						alb.ShrinkFactor = 0.0f;
						alb.AspectRatio = 1.0f;
						alb.SetVariableSizeY(SUiLayoutParam::szByContainer, 1.0f);
						//alb.Margin.Set(4);
						SUiLayout * p_lo_item = p_result->InsertItem(0, &alb);
						p_lo_item->CplxComponentId = cmlxcButton1;
					}		
					{ // Button
						SUiLayoutParam alb;
						alb.SetVariableSizeX(SUiLayoutParam::szUndef, 0.0f);
						alb.ShrinkFactor = 0.0f;
						alb.AspectRatio = 1.0f;
						alb.SetVariableSizeY(SUiLayoutParam::szByContainer, 1.0f);
						//alb.Margin.Set(4);
						SUiLayout * p_lo_item = p_result->InsertItem(0, &alb);
						p_lo_item->CplxComponentId = cmlxcButton2;
					}		
				}
				else { // Label above
					SUiLayoutParam alb(DIREC_VERT, 0, SUiLayoutParam::alignStart);
					alb.SetVariableSizeX(SUiLayoutParam::szByContainer, 1.0f);
					p_result->SetLayoutBlock(alb);
					{ // Label
						SUiLayoutParam alb;
						alb.GrowFactor = 1.0f;
						alb.SetVariableSizeX(SUiLayoutParam::szByContainer, 1.0f);
						//alb.Margin.Set(4);
						SUiLayout * p_lo_item = p_result->InsertItem(0, &alb);
						p_lo_item->CplxComponentId = cmlxcLabel;
					}
					{
						SUiLayoutParam alb_frame(DIREC_HORZ, 0, SUiLayoutParam::alignStart);
						alb_frame.GrowFactor = 1.0f;
						alb_frame.SetVariableSizeX(SUiLayoutParam::szByContainer, 1.0f);
						//alb.Margin.Set(4);
						SUiLayout * p_lo_frame = p_result->InsertItem(0, &alb_frame);
						{ // Input
							SUiLayoutParam alb;
							alb.SetVariableSizeX(SUiLayoutParam::szByContainer, 1.0f);
							alb.SetVariableSizeY(SUiLayoutParam::szByContainer, 1.0f);
							//alb.Margin.Set(4);
							SUiLayout * p_lo_item = p_lo_frame->InsertItem(0, &alb);
							p_lo_item->CplxComponentId = cmlxcInput;
						}		
						{ // Button
							SUiLayoutParam alb;
							alb.SetVariableSizeX(SUiLayoutParam::szUndef, 0.0f);
							alb.ShrinkFactor = 0.0f;
							alb.AspectRatio = 1.0f;
							alb.SetVariableSizeY(SUiLayoutParam::szByContainer, 1.0f);
							//alb.Margin.Set(4);
							SUiLayout * p_lo_item = p_lo_frame->InsertItem(0, &alb);
							p_lo_item->CplxComponentId = cmlxcButton1;
						}
						{ // Button2
							SUiLayoutParam alb;
							alb.SetVariableSizeX(SUiLayoutParam::szUndef, 0.0f);
							alb.ShrinkFactor = 0.0f;
							alb.AspectRatio = 1.0f;
							alb.SetVariableSizeY(SUiLayoutParam::szByContainer, 1.0f);
							//alb.Margin.Set(4);
							SUiLayout * p_lo_item = p_lo_frame->InsertItem(0, &alb);
							p_lo_item->CplxComponentId = cmlxcButton2;
						}
					}
				}
			}
			break;
	}
	return p_result;
}

int SUiLayout::FatherKillMe()
{
	int    ok = 0;
	if(P_Parent) {
		const uint _pcc = P_Parent->GetChildrenCount();
		for(uint i = 0; !ok && i < _pcc; i++) {
			const SUiLayout * p_sibl = P_Parent->GetChildC(i);
			if(p_sibl == this) {
				P_Parent->P_Children->atFree(i);
				ok = 1;
			}
		}
		assert(ok); // Если !ok то значит this имеет родителя, который о this ничего не знает.
	}
	return ok;
}

uint SUiLayout::GetChildrenCount() const
{
	return SVectorBase::GetCount(P_Children);
}

SUiLayout * SUiLayout::GetChild(uint idx)
{
	return (idx < SVectorBase::GetCount(P_Children)) ? P_Children->at(idx) : 0;
}

const  SUiLayout * SUiLayout::GetChildC(uint idx) const
{
	return (idx < SVectorBase::GetCount(P_Children)) ? P_Children->at(idx) : 0;
}

void SUiLayout::SetCallbacks(FlexSelfSizingProc selfSizingCb, FlexSetupProc setupCb, void * managedPtr)
{
	CbSelfSizing = selfSizingCb;
	CbSetup = setupCb;
	managed_ptr = managedPtr;
}

int SUiLayout::SetLayoutBlock(const SUiLayoutParam & rAlb)
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

int SUiLayout::GetOrder() const
{
	return ALB.Order;
}

void SUiLayout::SetOrder(int o)
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
int SUiLayout::GetFullWidth(float * pS) const
{
	int    ok = 1;
	float  s = 0.0f;
	//if(fisnanf(Size.x))
	if(ALB.SzX != SUiLayoutParam::szFixed)
		ok = 0;
	else {
		s += ALB.Size.x;
		s += ALB.Margin.a.x;
		s += ALB.Margin.b.x;
		s += ALB.Padding.a.x;
		s += ALB.Padding.b.x;
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
int SUiLayout::GetFullHeight(float * pS) const
{
	int    ok = 1;
	float  s = 0.0f;
	//if(fisnanf(Size.y))
	if(ALB.SzY != SUiLayoutParam::szFixed)
		ok = 0;
	else {
		s += ALB.Size.y;
		s += ALB.Margin.a.y;
		s += ALB.Margin.b.y;
		s += ALB.Padding.a.y;
		s += ALB.Padding.b.y;
		ASSIGN_PTR(pS, s);
	}
	return ok;
}
//
// Descr: Возвращает финальный расчетный прямоугольник элемента.
//
FRect SUiLayout::GetFrame() const
{
	return R;
}

FRect SUiLayout::GetFrameAdjustedToParent() const
{
	FRect f = R;
	const SUiLayout * p_parent = P_Parent;
	if(p_parent) {
		const FRect parent_frame = p_parent->GetFrameAdjustedToParent(); // @recursion
		f.Move__(parent_frame.a.x, parent_frame.a.y);
	}
	return f;
}

const SUiLayout * SUiLayout::FindMinimalItemAroundPoint(float x, float y) const
{
	const SUiLayout * p_result = 0;
	FRect f = GetFrameAdjustedToParent();
	if(f.IsEmpty() || f.Contains(SPoint2F(x, y))) {
		FRect min_child_f(0.0f, 0.0f);
		if(!f.IsEmpty())
			p_result = this;
		for(uint i = 0; i < GetChildrenCount(); i++) {
			const SUiLayout * p_child = GetChildC(i);
			if(p_child) {
				const SUiLayout * p_inner_result = p_child->FindMinimalItemAroundPoint(x, y); // @recursion
				if(p_inner_result) {
					FRect f = GetFrameAdjustedToParent();
					const double mcfs = min_child_f.Square();
					if(mcfs <= 0.0 || mcfs > f.Square()) {
						min_child_f = f;
						p_result = p_inner_result;
					}
				}
			}
		}
	}
	return p_result;
}

int SUiLayout::GetInnerCombinedFrame(FRect * pResult) const
{
	int    ok = 0;
	FRect  rf;
	const  uint _cc = GetChildrenCount();
	if(_cc) {
		bool is_first = true;
		for(uint i = 0; i < _cc; i++) {
			const SUiLayout * p_child = GetChildC(i);
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

SUiLayout * FASTCALL SUiLayout::FindComplexComponentId(uint id)
{
	SUiLayout * p_result = 0;
	if(id) {
		for(uint i = 0; !p_result && i < GetChildrenCount(); i++) {
			SUiLayout * p_child = GetChild(i);
			if(p_child) {
				if(p_child->CplxComponentId == id)
					p_result = p_child;
				else
					p_result = p_child->FindComplexComponentId(id)/* @recursion */; 
			}
		}
	}
	return p_result;
}
//
// Descr: Возвращает корневой элемент дерева, компонентом которого является this.
//
SUiLayout * SUiLayout::GetRoot()
{
	SUiLayout * p_root = this;
	while(p_root->P_Parent) {
		p_root = p_root->P_Parent;
	}
	return p_root;
}

SUiLayout * SUiLayout::InsertItem(void * pManagedPtr, const SUiLayoutParam * pAlb)
{
	SUiLayout * p_result_item = 0;
	if(!P_Children) {
		P_Children = new TSCollection <SUiLayout>;
	}
	if(P_Children) {
		p_result_item = P_Children->CreateNewItem();
		if(p_result_item) {
			p_result_item->P_Parent = this;
			p_result_item->managed_ptr = pManagedPtr;
			if(pAlb)
				p_result_item->SetLayoutBlock(*pAlb);
			p_result_item->UpdateShouldOrderChildren();		
		}
	}
	return p_result_item;
}

SUiLayout * SUiLayout::InsertItem()
{
	return InsertItem(0/*pManagedPtr*/, 0);
}

void SUiLayout::DeleteItem(uint idx)
{
	const uint _cc = GetChildrenCount();
	assert(idx < _cc);
	assert(_cc);
	if(idx < _cc) {
		SUiLayout * p_child = GetChild(idx);
		p_child->P_Parent = 0;
		P_Children->atFree(idx);
	}
}

void SUiLayout::DeleteAllItems()
{
	uint i = GetChildrenCount();
	if(i) do {
		DeleteItem(--i);
	} while(i);
}

SUiLayout::IndexEntry::IndexEntry() : ItemIdx(0), HglIdx(0)
{
}

SUiLayout::IterIndex::IterIndex() : TSVector <IndexEntry>()
{
}

SUiLayout * SUiLayout::IterIndex::GetHomogeneousItem(const SUiLayout * pBaseItem, uint hgeIdx) const
{
	assert(pBaseItem);
	SUiLayout * p_new_item = HomogeneousEntryPool.CreateNewItem();
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

void SUiLayout::IterIndex::ReleaseHomogeneousItem(SUiLayout * pItem) const
{
	bool   found = false;
	uint   i = HomogeneousEntryPool.getCount();
	if(i) do {
		SUiLayout * p = HomogeneousEntryPool.at(--i);
		if(p == pItem) {
			HomogeneousEntryPool.atFree(i);
			found = true;
		}
	} while(i && !found);
	assert(found);
}

static IMPL_CMPFUNC(LayoutFlexItemIndexEntry, i1, i2)
{
	const SUiLayout::IndexEntry * p1 = static_cast<const SUiLayout::IndexEntry *>(i1);
	const SUiLayout::IndexEntry * p2 = static_cast<const SUiLayout::IndexEntry *>(i2);
	const SUiLayout * p_extra = static_cast<const SUiLayout *>(pExtraData);
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
void SUiLayout::MakeIndex(IterIndex & rIndex) const
{
	rIndex.clear();
	const uint _cc = GetChildrenCount();
	for(uint i = 0; i < _cc; i++) {
		const SUiLayout * p_child = GetChildC(i);
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
		rIndex.sort(PTR_CMPFUNC(LayoutFlexItemIndexEntry), const_cast<SUiLayout *>(this)); // @badcase See IMPL_CMPFUNC(LayoutFlexItemIndexEntry)
	}
}

// @construction {
const SUiLayout * SUiLayout::GetChildByIndex(const IterIndex & rIndex, uint idxPos) const
{
	const SUiLayout * p_result = 0;
	if(idxPos < rIndex.getCount()) {
		const IndexEntry & r_ie = rIndex.at(idxPos);
		const uint _cc = GetChildrenCount();
		assert(r_ie.ItemIdx < _cc);
		if(r_ie.ItemIdx < _cc) {
			const SUiLayout * p_inner_item = GetChildC(r_ie.ItemIdx);
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
int SUiLayout::CommitChildResult(const IterIndex & rIndex, uint idxPos, const SUiLayout * pItem)
{
	int    ok = 0;
	if(idxPos < rIndex.getCount()) {
		const IndexEntry & r_ie = rIndex.at(idxPos);
		const uint _cc = GetChildrenCount();
		assert(r_ie.ItemIdx < _cc);
		if(r_ie.ItemIdx < _cc) {
			const SUiLayout * p_inner_item = GetChildC(r_ie.ItemIdx);
		}
	}
	return ok;
}
// } @construction 

class LayoutFlexProcessor {
public:
	static uint DetermineFlags(const SUiLayout * pItem)
	{
		uint   flags = 0;
		if(pItem) {
			if(pItem->ALB.GetContainerDirection() == DIREC_VERT)
				flags |= fVertical;
			if(pItem->ALB.Flags & SUiLayoutParam::fContainerReverseDir)
				flags |= fReverse;
			if(pItem->ALB.Flags & SUiLayoutParam::fContainerWrap) {
				flags |= fWrap;
				if(pItem->ALB.Flags & SUiLayoutParam::fContainerWrapReverse)
					flags |= fReverse2;
				// @v11.0.0 @experimental if(pItem->ALB.AlignContent != SUiLayoutParam::alignStart)
					flags |= fNeedLines;
			}
		}
		return flags;
	}
	LayoutFlexProcessor(const SUiLayout * pItem, const SUiLayout::Param & rP) : Flags(DetermineFlags(pItem)),
		SizeDim(0.0f), AlignDim(0.0f), FramePos1i(0), FramePos2i(0), FrameSz1i(0), FrameSz2i(0),
		ordered_indices(0), LineDim(0.0f), FlexDim(0.0f), ExtraFlexDim(0.0f), FlexGrows(0.0f), FlexShrinks(0.0f),
		Pos2(0.0f), P(rP)
	{
		assert(pItem);
		assert(pItem->ALB.Padding.a.x >= 0.0f);
		assert(pItem->ALB.Padding.b.x >= 0.0f);
		assert(pItem->ALB.Padding.a.y >= 0.0f);
		assert(pItem->ALB.Padding.b.y >= 0.0f);
		const float __width  = smax(0.0f, rP.ForceWidth  - (pItem->ALB.Padding.a.x + pItem->ALB.Padding.b.x));
		const float __height = smax(0.0f, rP.ForceHeight - (pItem->ALB.Padding.a.y + pItem->ALB.Padding.b.y));
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
		if((pItem->State & SUiLayout::stShouldOrderChildren) && _item_cc) {
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
			if(pItem->ALB.Flags & SUiLayoutParam::fContainerWrapReverse)
				Pos2 = AlignDim;
		}
		else
			Pos2 = (Flags & fVertical) ? pItem->ALB.Padding.a.x : pItem->ALB.Padding.a.y;
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
	const SUiLayout & GetChildByIndex(const SUiLayout * pContainer, uint idx) const
	{
		const uint _pos = ordered_indices ? ordered_indices[idx] : idx;
		assert(_pos < pContainer->GetChildrenCount());
		return *pContainer->GetChildC(_pos);
	}
	void   ProcessLines(const SUiLayout & rItem);
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
	const  SUiLayout::Param P;
};

bool SUiLayout::LayoutAlign(/*flex_align*/int align, float flexDim, uint childrenCount, float * pPos, float * pSpacing, bool stretchAllowed) const
{
	assert(flexDim > 0);
	float pos = 0.0f;
	float spacing = 0.0f;
	switch(align) {
		case SUiLayoutParam::alignStart: break;
		case SUiLayoutParam::alignEnd: pos = flexDim; break;
		case SUiLayoutParam::alignCenter: pos = flexDim / 2; break;
		case SUiLayoutParam::alignSpaceBetween:
		    if(childrenCount > 1) {
			    spacing = flexDim / (childrenCount-1);
		    }
		    break;
		case SUiLayoutParam::alignSpaceAround:
		    if(childrenCount) {
			    spacing = flexDim / childrenCount;
			    pos = spacing / 2;
		    }
		    break;
		case SUiLayoutParam::alignSpaceEvenly:
		    if(childrenCount) {
			    spacing = flexDim / (childrenCount+1);
			    pos = spacing;
		    }
		    break;
		case SUiLayoutParam::alignStretch:
		    if(stretchAllowed) {
			    spacing = flexDim / childrenCount;
			    break;
		    }
			else { // by default: SUiLayoutParam::alignStart
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

SUiLayout::HomogeneousArray::HomogeneousArray() : VariableFactor(vfNone)
{
}

int SUiLayout::InitHomogeneousArray(uint variableFactor /* HomogeneousArray::vfXXX */)
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

int SUiLayout::AddHomogeneousEntry(long id, float vf)
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

void SUiLayout::UpdateShouldOrderChildren()
{
	if(ALB.Order != 0 && P_Parent)
		P_Parent->State |= stShouldOrderChildren;
}

/*flex_align*/int FASTCALL SUiLayout::GetChildAlign(const SUiLayout & rChild) const
{
	return (rChild.ALB.AlignSelf == ALB.alignAuto) ? ALB.AlignItems : rChild.ALB.AlignSelf;
}

#define CHILD_MARGIN_XY_(ptrLayout, child, pnt) ((ptrLayout->Flags & LayoutFlexProcessor::fVertical) ? child.ALB.Margin.pnt.x : child.ALB.Margin.pnt.y)
#define CHILD_MARGIN_YX_(ptrLayout, child, pnt) ((ptrLayout->Flags & LayoutFlexProcessor::fVertical) ? child.ALB.Margin.pnt.y : child.ALB.Margin.pnt.x)

void SUiLayout::DoLayoutChildren(uint childBeginIdx, uint childEndIdx, uint childrenCount, void * pLayout, SScroller::SetupBlock * pSsb) const
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
			pos -= (p_layout->Flags & LayoutFlexProcessor::fVertical) ? ALB.Padding.b.y : ALB.Padding.b.x;
		else 
			pos += (p_layout->Flags & LayoutFlexProcessor::fVertical) ? ALB.Padding.a.y : ALB.Padding.a.x;
		if((p_layout->Flags & LayoutFlexProcessor::fWrap) && (p_layout->Flags & LayoutFlexProcessor::fReverse2)) {
			p_layout->Pos2 -= p_layout->LineDim;
		}
		for(uint i = childBeginIdx; i < childEndIdx; i++) {
			const SUiLayout & r_child = p_layout->GetChildByIndex(this, i);
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
						case SUiLayoutParam::alignEnd:
							{
								const float mar_b = CHILD_MARGIN_XY_(p_layout, r_child, b);
								align_pos += (p_layout->LineDim - align_size - mar_b);
							}
							break;
						case SUiLayoutParam::alignCenter:
							{
								const float mar_a = CHILD_MARGIN_XY_(p_layout, r_child, a);
								const float mar_b = CHILD_MARGIN_XY_(p_layout, r_child, b);
								align_pos += (p_layout->LineDim / 2.0f) - (align_size / 2.0f) + (mar_a - mar_b);
							}
							break;
						case SUiLayoutParam::alignStretch:
							if(align_size == 0) {
								const float mar_a = CHILD_MARGIN_XY_(p_layout, r_child, a);
								const float mar_b = CHILD_MARGIN_XY_(p_layout, r_child, b);
								r_child.R.Frame[p_layout->FrameSz2i] = p_layout->LineDim - (mar_a + mar_b);
							}
						// @fallthrough
						case SUiLayoutParam::alignStart:
							{
								const float mar_a = CHILD_MARGIN_XY_(p_layout, r_child, a);
								align_pos += mar_a;
							}
							break;
						default:
							//assert(false && "incorrect align_self");
							{
								const float mar_a = CHILD_MARGIN_XY_(p_layout, r_child, a);
								align_pos += mar_a; // По умолчанию пусть будет SUiLayoutParam::alignStart
							}
							break;
					}
				}
				r_child.R.Frame[p_layout->FramePos2i] = align_pos;
				// Set the main axis position.
				{
					const float mar_a = CHILD_MARGIN_YX_(p_layout, r_child, a);
					const float mar_b = CHILD_MARGIN_YX_(p_layout, r_child, b);
					const float item_size_1 = r_child.R.Frame[p_layout->FrameSz1i];
					const float margin_yx_a = mar_a;
					const float margin_yx_b = mar_b;
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
					if(r_child.R.Frame[2] == 0.0f && r_child.ALB.SzX == SUiLayoutParam::szUndef && r_child.R.Frame[3] > 0.0f) {
						r_child.R.Frame[2] = r_child.R.Frame[3] / r_child.ALB.AspectRatio;
					}
					if(r_child.R.Frame[3] == 0.0f && r_child.ALB.SzY == SUiLayoutParam::szUndef && r_child.R.Frame[2] > 0.0f) {
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

int SUiLayout::SetupResultScroller(SScroller::SetupBlock * pSb) const
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

void SUiLayout::Commit_() const
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

void SUiLayout::DoLayout(const Param & rP) const
{
	const uint _cc = GetChildrenCount();
	const int  _direction = ALB.GetContainerDirection();
	const int  _cross_direction = SUiLayoutParam::GetCrossDirection(_direction);
	if(_cc && oneof2(_direction, DIREC_HORZ, DIREC_VERT)) {
		LayoutFlexProcessor layout_s(this, rP);
		uint last_layout_child = 0;
		uint relative_children_count = 0;
		for(uint i = 0; i < _cc; i++) {
			const SUiLayout & r_child = layout_s.GetChildByIndex(this, i);
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
				r_child.R.Frame[2] = efsxy.x;
				r_child.R.Frame[3] = efsxy.y;
				//
				// Main axis size defaults to 0.
				//
				// Cross axis size defaults to the parent's size (or line size in wrap mode, which is calculated later on).
				if(fisnan(r_child.R.Frame[layout_s.FrameSz2i])) {
					if(layout_s.Flags & LayoutFlexProcessor::fWrap)
						layout_s.Flags |= LayoutFlexProcessor::fNeedLines;
					else {
						const float full_size = ((layout_s.Flags & LayoutFlexProcessor::fVertical) ? rP.ForceWidth : rP.ForceHeight);
						const float mar_a = CHILD_MARGIN_XY_((&layout_s), r_child, a);
						const float mar_b = CHILD_MARGIN_XY_((&layout_s), r_child, b);
						r_child.R.Frame[layout_s.FrameSz2i] = full_size - mar_a - mar_b;
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
						if(size_off != layout_s.FrameSz2i || GetChildAlign(r_child) != SUiLayoutParam::alignStretch) {
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
				{
					assert(r_child.ALB.GrowFactor >= 0.0f);
					assert(r_child.ALB.ShrinkFactor >= 0.0f);
					layout_s.FlexGrows   += r_child.ALB.GrowFactor;
					layout_s.FlexShrinks += r_child.ALB.ShrinkFactor;
					const float mar_a = CHILD_MARGIN_YX_((&layout_s), r_child, a);
					const float mar_b = CHILD_MARGIN_YX_((&layout_s), r_child, b);
					layout_s.FlexDim     -= (child_size + (mar_a + mar_b));
					relative_children_count++;
					if(child_size > 0.0f && r_child.ALB.GrowFactor > 0.0f)
						layout_s.ExtraFlexDim += child_size;
				}
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
					const bool is_align_stretch = (ALB.AlignContent == SUiLayoutParam::alignStretch);
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
							const SUiLayout & r_child = layout_s.GetChildByIndex(this, j);
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

int SUiLayoutParam::SetVArea(int area)
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

int SUiLayoutParam::GetVArea(/*const SUiLayout & rItem*/) const
{
	int   a = -1;
	const int gx = /*rItem.ALB.*/GravityX;
	const int gy = /*rItem.ALB.*/GravityY;
	switch(gx) {
		case 0:
			switch(gy) {
				case 0: a = SUiLayoutParam::areaCenter; break;
				case SIDE_TOP: a = SUiLayoutParam::areaSideU; break;
				case SIDE_BOTTOM: a = SUiLayoutParam::areaSideB; break;
				case SIDE_CENTER: a = SUiLayoutParam::areaCenter; break;
				default: assert(0);
			}
			break;
		case SIDE_LEFT:
			switch(gy) {
				case 0: a = SUiLayoutParam::areaSideL; break;
				case SIDE_TOP: a = SUiLayoutParam::areaCornerLU; break;
				case SIDE_BOTTOM: a = SUiLayoutParam::areaCornerLB; break;
				case SIDE_CENTER: a = SUiLayoutParam::areaSideL; break;
				default: assert(0);
			}
			break;
		case SIDE_RIGHT:
			switch(gy) {
				case 0: a = SUiLayoutParam::areaSideR; break;
				case SIDE_TOP: a = SUiLayoutParam::areaCornerRU; break;
				case SIDE_BOTTOM: a = SUiLayoutParam::areaCornerRB; break;
				case SIDE_CENTER: a = SUiLayoutParam::areaSideR; break;
				default: assert(0);
			}
			break;
		case SIDE_CENTER:
			switch(gy) {
				case 0: a = SUiLayoutParam::areaCenter; break;
				case SIDE_TOP: a = SUiLayoutParam::areaSideU; break;
				case SIDE_BOTTOM: a = SUiLayoutParam::areaSideB; break;
				case SIDE_CENTER: a = SUiLayoutParam::areaCenter; break;
				default: assert(0);
			}
			break;
		default:
			assert(0);
	}
	return a;
}

/*static*/int SUiLayoutParam::RestrictVArea(int restrictingArea, const FRect & rRestrictingRect, int restrictedArea, FRect & rRestrictedRect)
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
					SETMAX(rRestrictedRect.a.x, rRestrictingRect.b.x);
					break;
				case areaCornerLB:
				case areaSideB:
				case areaSideL:
					SETMAX(rRestrictedRect.a.y, rRestrictingRect.b.y);
					break;
				case areaCornerRB:
				case areaCenter: 
					SETMAX(rRestrictedRect.a.x, rRestrictingRect.b.x);
					SETMAX(rRestrictedRect.a.y, rRestrictingRect.b.y);
					break;
			}
			break;
		case areaCornerRU:
			switch(restrictedArea) {
				case areaCornerLU:
				case areaSideU:
				case areaSideL:
					SETMIN(rRestrictedRect.b.x, rRestrictingRect.a.x);
					break;
				case areaCornerRB:
				case areaSideB:
				case areaSideR:
					SETMAX(rRestrictedRect.a.y, rRestrictingRect.b.y);
					break;
				case areaCornerLB:
				case areaCenter: 
					SETMIN(rRestrictedRect.b.x, rRestrictingRect.a.x);
					SETMAX(rRestrictedRect.a.y, rRestrictingRect.b.y);
					break;
			}
			break;
		case areaCornerRB: 
			switch(restrictedArea) {
				case areaCornerLB:
				case areaSideL:
				case areaSideB:
					SETMIN(rRestrictedRect.b.x, rRestrictingRect.a.x);
					break;
				case areaSideU:
				case areaCornerRU:
				case areaSideR:
					SETMIN(rRestrictedRect.b.y, rRestrictingRect.a.y);
					break;
				case areaCornerLU:
				case areaCenter: 
					SETMIN(rRestrictedRect.b.x, rRestrictingRect.a.x);
					SETMIN(rRestrictedRect.b.y, rRestrictingRect.a.y);
					break;
			}
			break;
		case areaCornerLB: 
			switch(restrictedArea) {
				case areaSideU:
				case areaSideL:
				case areaCornerLU:
					SETMIN(rRestrictedRect.b.y, rRestrictingRect.a.y);
					break;
				case areaSideB:
				case areaSideR:
				case areaCornerRB:
					SETMAX(rRestrictedRect.a.x, rRestrictingRect.b.x);
					break;
				case areaCornerRU:				
				case areaCenter: 
					SETMAX(rRestrictedRect.a.x, rRestrictingRect.b.x);
					SETMIN(rRestrictedRect.b.y, rRestrictingRect.a.y);
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
					SETMAX(rRestrictedRect.a.y, rRestrictingRect.b.y);
					break;
				case areaCornerLU:
					SETMIN(rRestrictedRect.b.x, rRestrictingRect.a.x);
					break;
				case areaCornerRU:				
					SETMAX(rRestrictedRect.a.x, rRestrictingRect.b.x);
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
					SETMIN(rRestrictedRect.b.y, rRestrictingRect.a.y);
					break;
				case areaCornerLB:
					SETMIN(rRestrictedRect.b.x, rRestrictingRect.a.x);
					break;
				case areaCornerRB:
					SETMAX(rRestrictedRect.a.x, rRestrictingRect.b.x);
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
					SETMIN(rRestrictedRect.b.x, rRestrictingRect.a.x);
					break;
				case areaCornerRU:
					SETMIN(rRestrictedRect.b.y, rRestrictingRect.a.y);
					break;
				case areaCornerRB:
					SETMAX(rRestrictedRect.a.y, rRestrictingRect.b.y);
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
					SETMAX(rRestrictedRect.a.x, rRestrictingRect.b.x);
					break;
				case areaCornerLU:
					SETMIN(rRestrictedRect.b.y, rRestrictingRect.a.y);
					break;
				case areaCornerLB:
					SETMAX(rRestrictedRect.a.y, rRestrictingRect.b.y);
					break;
			}
			break;
		case areaCenter:
			switch(restrictedArea) {
				case areaCornerLB:
					SETMIN(rRestrictedRect.b.x, rRestrictingRect.a.x);
					SETMAX(rRestrictedRect.a.y, rRestrictingRect.b.y);
					break;
				case areaSideB:
					SETMAX(rRestrictedRect.a.y, rRestrictingRect.b.y);
					break;
				case areaCornerLU:
					SETMIN(rRestrictedRect.b.x, rRestrictingRect.a.x);
					SETMIN(rRestrictedRect.b.y, rRestrictingRect.a.y);
					break;
				case areaSideR:
					SETMAX(rRestrictedRect.a.x, rRestrictingRect.b.x);
					break;
				case areaSideU:
					SETMIN(rRestrictedRect.b.y, rRestrictingRect.a.y);
					break;
				case areaCornerRB:
					SETMAX(rRestrictedRect.a.x, rRestrictingRect.b.x);
					SETMAX(rRestrictedRect.a.y, rRestrictingRect.b.y);
					break;
				case areaCornerRU:
					SETMAX(rRestrictedRect.a.x, rRestrictingRect.b.x);
					SETMIN(rRestrictedRect.b.y, rRestrictingRect.a.y);
					break;
				case areaSideL: 
					SETMIN(rRestrictedRect.b.x, rRestrictingRect.a.x);
					break;
			}
			break;
	}	
	CATCHZOK
	return ok;
}

void SUiLayout::Helper_CommitInnerFloatLayout(SUiLayout * pCurrentLayout, const SPoint2F & rOffs) const
{
	//const float offs_x = area_rect[prev_area].a.x;
	//const float offs_y = area_rect[prev_area].a.y;
	const uint _cl_cc = pCurrentLayout->GetChildrenCount();
	for(uint ci = 0; ci < _cl_cc; ci++) {
		const SUiLayout * p_layout_item = pCurrentLayout->GetChildC(ci);
		if(p_layout_item && p_layout_item->P_Link) {
			p_layout_item->P_Link->R.CopyWithOffset(p_layout_item->R, rOffs.x, rOffs.y);
			p_layout_item->P_Link->Commit_();
		}
	}
	delete pCurrentLayout;
}

void SUiLayout::DoFloatLayout(const Param & rP)
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
					const SUiLayout & r_child = layout_s.GetChildByIndex(this, cidx);
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
				SUiLayout * p_current_layout = 0;
				LongArray actual_area_list; // Список идентификаторов областей, которые подлежат рассмотрению
				LongArray seen_area_list; // Список уже обработанных идентификаторов областей
				item_map.GetValList(actual_area_list);
				for(uint midx = 0; midx < item_map.getCount(); midx++) {
					const LAssoc & r_ai = item_map.at(midx);
					const uint cidx = static_cast<uint>(r_ai.Key);
					const int  area = static_cast<int>(r_ai.Val);
					assert(area >= 0 && area < SIZEOFARRAY(area_rect));
					assert(cidx >= 0 && cidx < _cc);
					const SUiLayout & r_child = layout_s.GetChildByIndex(this, cidx);
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
									cf.Move__(area_rect[prev_area].a.x, area_rect[prev_area].a.y);
									for(uint ai = 0; ai < actual_area_list.getCount(); ai++) {
										const int local_area = actual_area_list.get(ai);
										if(local_area != prev_area && !seen_area_list.lsearch(local_area)) {
											const int rvar = SUiLayoutParam::RestrictVArea(prev_area, cf, local_area, area_rect[local_area]);
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
							p_current_layout = new SUiLayout();
							struct LocalLayoutEntry {
								LocalLayoutEntry() : ContainerDirec(DIREC_UNKN), ContainerFlags(SUiLayoutParam::fContainerWrap), JustifyContent(0), AlignContent(0)
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
								uint16 JustifyContent; // SUiLayoutParam::alignXXX Выравнивание внутренних элементов вдоль основной оси
								uint16 AlignContent;   // SUiLayoutParam::alignXXX Выравнивание внутренних элементов по кросс-оси
							};
							LocalLayoutEntry lle;
							switch(area) {
								case SUiLayoutParam::areaCornerLU:
									lle.Set(DIREC_HORZ, 0, SUiLayoutParam::alignStart, SUiLayoutParam::alignStart);
									break;
								case SUiLayoutParam::areaCornerRU:
									lle.Set(DIREC_HORZ, SUiLayoutParam::fContainerReverseDir, SUiLayoutParam::alignStart, SUiLayoutParam::alignStart);
									break;
								case SUiLayoutParam::areaCornerRB:
									lle.Set(DIREC_HORZ, SUiLayoutParam::fContainerReverseDir|SUiLayoutParam::fContainerWrapReverse,
										SUiLayoutParam::alignStart, SUiLayoutParam::alignStart);
									break;
								case SUiLayoutParam::areaCornerLB:
									lle.Set(DIREC_HORZ, 0/*SUiLayoutParam::fContainerWrapReverse*/, SUiLayoutParam::alignStart, SUiLayoutParam::alignEnd);
									break;
								case SUiLayoutParam::areaSideU:
									lle.Set(DIREC_HORZ, 0, SUiLayoutParam::alignStart, SUiLayoutParam::alignStart);
									break;
								case SUiLayoutParam::areaSideR:
									lle.Set(DIREC_VERT, 0, SUiLayoutParam::alignStart, SUiLayoutParam::alignEnd);
									break;
								case SUiLayoutParam::areaSideB:
									lle.Set(DIREC_HORZ, 0, SUiLayoutParam::alignStart, SUiLayoutParam::alignEnd);
									break;
								case SUiLayoutParam::areaSideL:
									lle.Set(DIREC_VERT, 0, SUiLayoutParam::alignStart, SUiLayoutParam::alignStart);
									break;
								case SUiLayoutParam::areaCenter:
									lle.Set(DIREC_HORZ, 0, SUiLayoutParam::alignStart, SUiLayoutParam::alignStart); // @?
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
						SUiLayout * p_layout_item = p_current_layout->InsertItem();
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

void SUiLayout::Setup(uint flags)
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

int SUiLayout::Evaluate(const Param * pP)
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
				stag_x = SUiLayoutParam::szFixed;
			}
			else
				local_evaluate_param.ForceWidth = sx;
			if(pP->ForceHeight > 0.0f) {
				local_evaluate_param.ForceHeight = pP->ForceHeight;
				stag_y = SUiLayoutParam::szFixed;
			}
			else
				local_evaluate_param.ForceHeight = sy;
		}
		else {
			local_evaluate_param.ForceWidth = sx;
			local_evaluate_param.ForceHeight = sy;
		}
		if(oneof2(cdir, DIREC_HORZ, DIREC_VERT)) {
			if(stag_x == SUiLayoutParam::szFixed && stag_y == SUiLayoutParam::szFixed && !CbSelfSizing) {
				assert(P_Parent == NULL);
				assert(CbSelfSizing == NULL);
				DoLayout(local_evaluate_param);
				Setup(setupfChildrenOnly);
				ok = 1;
			}
		}
		else {
			if(stag_x == SUiLayoutParam::szFixed && stag_y == SUiLayoutParam::szFixed && !CbSelfSizing) {
				DoFloatLayout(local_evaluate_param);
				Setup(setupfChildrenOnly);
				ok = 1;
			}
		}
	}
	return ok;
}
