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
	if(SzY == szFixed && IsNominalFullDefinedY()) {
		if(!feqeps(Size.Y, Nominal.Height(), 0.01))
			ok = 0;
	}
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
	else if(oneof3(SzX, szByContainer, szByContent, szUndef))
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
	else if(oneof3(SzY, szByContainer, szByContent, szUndef))
		result = SzY;
	ASSIGN_PTR(pS, s);
	return result;
}

void AbstractLayoutBlock::SetFixedSizeX(float s)
{
	assert(!fisnanf(s) && s >= 0.0f);
	Size.X = s;
	SzX = szFixed;
}

void AbstractLayoutBlock::SetVariableSizeX(uint var/* szXXX */)
{
	assert(oneof3(var, szUndef, szByContent, szByContainer));
	Size.X = 0.0f;
	SzX = var;
}

void AbstractLayoutBlock::SetFixedSizeY(float s)
{
	assert(!fisnanf(s) && s >= 0.0f);
	Size.Y = s;
	SzY = szFixed;
}

void AbstractLayoutBlock::SetFixedSize(const TRect & rR)
{
	SetFixedSizeX(static_cast<float>(rR.width()));
	SetFixedSizeY(static_cast<float>(rR.height()));
}

void AbstractLayoutBlock::SetVariableSizeY(uint var/* szXXX */)
{
	assert(oneof3(var, szUndef, szByContent, szByContainer));
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
			case szByContainer: rBuf.Cat("parent"); break;
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
			case szByContainer: rBuf.Cat("parent"); break;
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
				rS = temp_buf.ToFloat();
				if(fisnan(rS) || rS < 0.0f) {
					rS = 0.0f;
					result = szInvalid;
				}
				else
					result = szFixed;
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
			SetVariableSizeX(szx);
		else
			SetVariableSizeX(szUndef);
		if(szy == szFixed)
			SetFixedSizeY(s.Y);
		else if(szy != szInvalid)
			SetVariableSizeY(szy);
		else
			SetVariableSizeY(szUndef);
	}
	else {
		float v;
		int szxy = ParseSizeStr(x_buf, v);
		if(szxy == szFixed) {
			SetFixedSizeX(v);
			SetFixedSizeY(v);
		}
		else if(szxy != szInvalid) {
			SetVariableSizeX(szxy);
			SetVariableSizeY(szxy);
		}
		else {
			SetVariableSizeX(szUndef);
			SetVariableSizeY(szUndef);			
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
LayoutFlexItem::LayoutFlexItem() : P_Parent(0), managed_ptr(0), CbSelfSizing(0), CbSetup(0), State(0), ALB()
	//Size(fgetnanf(), fgetnanf()), 
	//N(fgetnanf(), fgetnanf(), fgetnanf(), fgetnanf()),
	//JustifyContent(FLEX_ALIGN_START), 
	//AlignContent(FLEX_ALIGN_STRETCH), 
	//AlignItems(FLEX_ALIGN_STRETCH),
	//AlignSelf(FLEX_ALIGN_AUTO), 
	//Direction(FLEX_DIRECTION_COLUMN),
	//GrowFactor(0.0f), 
	//ShrinkFactor(1.0f), 
	//Order(0), 
	//Basis(fgetnanf()), 
	//AspectRatio(0.0f), 
	//Flags(0)
{
	memzero(frame, sizeof(frame));
}

LayoutFlexItem::~LayoutFlexItem()
{
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

void LayoutFlexItem::UpdateShouldOrderChildren()
{
	if(ALB.Order != 0 && P_Parent) {
		P_Parent->State |= stShouldOrderChildren;
	}
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
		/*
		if(!fisnan(Margin.a.X))
			s += Margin.a.X;
		if(!fisnan(Margin.b.X))
			s += Margin.b.X;
		if(!fisnan(Padding.a.X))
			s += Padding.a.X;
		if(!fisnan(Padding.b.X))
			s += Padding.b.X;
		*/
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
		/*
		if(!fisnan(Margin.a.Y))
			s += Margin.a.Y;
		if(!fisnan(Margin.b.Y))
			s += Margin.b.Y;
		if(!fisnan(Padding.a.Y))
			s += Padding.a.Y;
		if(!fisnan(Padding.b.Y))
			s += Padding.b.Y;
		*/
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
	FRect r;
	r.a.X = frame[0];
	r.a.Y = frame[1];
	r.b.X = frame[0] + frame[2];
	r.b.Y = frame[1] + frame[3];
	return r;
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

struct LayoutFlex {
	LayoutFlex() : wrap(false), reverse(false), reverse2(false), vertical(false), need_lines(false),
		size_dim(0.0f), align_dim(0.0f), frame_pos_i(0), frame_pos2_i(0), frame_size_i(0), frame_size2_i(0),
		ordered_indices(0), line_dim(0.0f), flex_dim(0.0f), extra_flex_dim(0.0f), flex_grows(0.0f), flex_shrinks(0.0f),
		pos2(0.0f), P_Lines(0), lines_count(0), lines_sizes(0.0f)
	{
	}
	void Init(const LayoutFlexItem * pItem, float _width, float _height)
	{
		assert(pItem->ALB.Padding.a.X >= 0.0f);
		assert(pItem->ALB.Padding.b.X >= 0.0f);
		assert(pItem->ALB.Padding.a.Y >= 0.0f);
		assert(pItem->ALB.Padding.b.Y >= 0.0f);
		_width  -= (pItem->ALB.Padding.a.X + pItem->ALB.Padding.b.X);
		_height -= (pItem->ALB.Padding.a.Y + pItem->ALB.Padding.b.Y);
		assert(_width >= 0.0f);
		assert(_height >= 0.0f);
		reverse = false;
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
				assert(false && "incorrect direction");
		}
		ordered_indices = NULL;
		if(/*item->should_order_children*/(pItem->State & LayoutFlexItem::stShouldOrderChildren) && pItem->getCount()) {
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
		flex_dim = 0.0f;
		flex_grows = 0.0f;
		flex_shrinks = 0.0f;
		reverse2 = false;
		//wrap = (pItem->WrapMode != FLEX_WRAP_NO_WRAP);
		//wrap = LOGIC(pItem->Flags & LayoutFlexItem::fWrap);
		wrap = LOGIC(pItem->ALB.Flags & AbstractLayoutBlock::fContainerWrap);
		if(wrap) {
			//if(pItem->WrapMode == FLEX_WRAP_WRAP_REVERSE) {
			//if(pItem->Flags & LayoutFlexItem::fWrapReverse) {
			if(pItem->ALB.Flags & AbstractLayoutBlock::fContainerWrapReverse) {
				reverse2 = true;
				pos2 = align_dim;
			}
		}
		else {
			pos2 = vertical ? pItem->ALB.Padding.a.X : pItem->ALB.Padding.a.Y;
		}
		need_lines = (wrap && pItem->ALB.AlignContent != /*FLEX_ALIGN_START*/AbstractLayoutBlock::alignStart);
		P_Lines = NULL;
		lines_count = 0;
		lines_sizes = 0;
	}
	LayoutFlex & Z()
	{
		line_dim = wrap ? 0 : align_dim;
		flex_dim = size_dim;
		extra_flex_dim = 0.0f;
		flex_grows = 0.0f;
		flex_shrinks = 0.0f;
		return *this;
	}
	//#define LAYOUT_CHILD_AT_(ptrLayout, item, i) (item->at((ptrLayout->ordered_indices ? ptrLayout->ordered_indices[i] : i)))
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
	uint8  Reserve[3];       // @alignment
	float  size_dim;         // main axis parent size
	float  align_dim;        // cross axis parent size
	uint   frame_pos_i; // main axis position
	uint   frame_pos2_i; // cross axis position
	uint   frame_size_i; // main axis size
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

//#define LAYOUT_CHILD_AT_(ptrLayout, item, i) (item->at((ptrLayout->ordered_indices ? ptrLayout->ordered_indices[i] : i)))
#define _LAYOUT_FRAME_(ptrLayout, child, name) child.frame[ptrLayout->frame_ ## name ## _i]
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
		case AbstractLayoutBlock::alignStart:
		    break;
		case AbstractLayoutBlock::alignEnd:
		    pos = flex_dim;
		    break;
		case AbstractLayoutBlock::alignCenter:
		    pos = flex_dim / 2;
		    break;
		case AbstractLayoutBlock::alignSpaceBetween:
		    if(children_count > 0) {
			    spacing = flex_dim / (children_count - 1);
		    }
		    break;
		case AbstractLayoutBlock::alignSpaceAround:
		    if(children_count > 0) {
			    spacing = flex_dim / children_count;
			    pos = spacing / 2;
		    }
		    break;
		case AbstractLayoutBlock::alignSpaceEvenly:
		    if(children_count > 0) {
			    spacing = flex_dim / (children_count + 1);
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
			// If the container has a positive flexible space, let's add to it
			// the sizes of all flexible children.
			p_layout->flex_dim += p_layout->extra_flex_dim;
		}
		// Determine the main axis initial position and optional spacing.
		float pos = 0.0f;
		float spacing = 0.0f;
		if(p_layout->flex_grows == 0 && p_layout->flex_dim > 0) {
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
			//if(r_child.PositionMode == FLEX_POSITION_ABSOLUTE) {
			//if(!(r_child.Flags & fPositionAbsolute)) { // Isn't already positioned
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
				switch(GetChildAlign(r_child)) {
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
						assert(false && "incorrect align_self");
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
				// Now that the item has a frame, we can layout its children.
				r_child.DoLayout(r_child.frame[2], r_child.frame[3]);
			}
		}
		if(p_layout->wrap && !p_layout->reverse2) {
			p_layout->pos2 += p_layout->line_dim;
		}
		if(p_layout->need_lines) {
			p_layout->P_Lines = static_cast<LayoutFlex::Line *>(SAlloc::R(p_layout->P_Lines, sizeof(LayoutFlex::Line) * (p_layout->lines_count + 1)));
			assert(p_layout->P_Lines != NULL);
			LayoutFlex::Line * line = &p_layout->P_Lines[p_layout->lines_count];
			line->child_begin = childBeginIdx;
			line->child_end = childEndIdx;
			line->size = p_layout->line_dim;
			p_layout->lines_count++;
			p_layout->lines_sizes += line->size;
		}
	}
}

void LayoutFlexItem::DoLayout(float _width, float _height)
{
	if(getCount() && oneof2(ALB.GetContainerDirection(), DIREC_HORZ, DIREC_VERT)) {
		LayoutFlex layout_s;
		//LayoutFlex * p_layout = &layout_s;
		layout_s.Init(this, _width, _height);
		layout_s.Z(); //LAYOUT_RESET();
		uint last_layout_child = 0;
		uint relative_children_count = 0;
		for(uint i = 0; i < getCount(); i++) {
			LayoutFlexItem & r_child = layout_s.GetChildByIndex(this, i);
			// Items with an absolute position have their frames determined
			// directly and are skipped during layout.
			//if(r_child.PositionMode == FLEX_POSITION_ABSOLUTE) {
			//if(r_child.Flags & fPositionAbsolute) {
			if(r_child.ALB.IsPositionAbsoluteX() && r_child.ALB.IsPositionAbsoluteY()) {
//#define ABSOLUTE_SIZE(val, pos1, pos2, dim) (!fisnanf(val)  ? val  : (!fisnanf(pos1) && !fisnanf(pos2) ? (dim - pos2 - pos1) : 0))
//#define ABSOLUTE_POS(pos1, pos2, size, dim) (!fisnanf(pos1) ? pos1 : (!fisnanf(pos2) ? (dim - size - pos2) : 0))
				float child_width = r_child.ALB.GetAbsoluteSizeX();
				float child_height = r_child.ALB.GetAbsoluteSizeY();
				float child_x = r_child.ALB.GetAbsoluteLowX();
				float child_y = r_child.ALB.GetAbsoluteLowY();
				r_child.frame[0] = child_x;
				r_child.frame[1] = child_y;
				r_child.frame[2] = child_width;
				r_child.frame[3] = child_height;
				// Now that the item has a frame, we can layout its children.
				r_child.DoLayout(r_child.frame[2], r_child.frame[3]); // @recursion
//#undef ABSOLUTE_POS
//#undef ABSOLUTE_SIZE
				continue;
			}
			else {
				// Initialize frame.
				r_child.frame[0] = 0.0f;
				r_child.frame[1] = 0.0f;
				r_child.frame[2] = (r_child.ALB.SzX == AbstractLayoutBlock::szFixed) ? r_child.ALB.Size.X : fgetnanf();
				r_child.frame[3] = (r_child.ALB.SzY == AbstractLayoutBlock::szFixed) ? r_child.ALB.Size.Y : fgetnanf();
				// Main axis size defaults to 0.
				if(fisnanf(CHILD_SIZE_((&layout_s), r_child))) {
					CHILD_SIZE_((&layout_s), r_child) = 0;
				}
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
					float size[2] = { r_child.frame[2], r_child.frame[3] };
					r_child.CbSelfSizing(&r_child, size);
					for(uint j = 0; j < 2; j++) {
						const uint size_off = j + 2;
						if(size_off == layout_s.frame_size2_i && GetChildAlign(r_child) == /*FLEX_ALIGN_STRETCH*/AbstractLayoutBlock::alignStretch) {
							continue;
						}
						float val = size[j];
						if(!fisnanf(val))
							r_child.frame[size_off] = val;
					}
				}
				// Honor the `basis' property which overrides the main-axis size.
				if(r_child.ALB.Basis > 0.0f) {
					//assert(r_child.ALB.Basis >= 0.0f);
					CHILD_SIZE_((&layout_s), r_child) = r_child.ALB.Basis;
				}
				float child_size = CHILD_SIZE_((&layout_s), r_child);
				if(layout_s.wrap) {
					if(layout_s.flex_dim < child_size) {
						// Not enough space for this child on this line, layout the
						// remaining items and move it to a new line.
						//layout_items(this, last_layout_child, i, relative_children_count, layout);
						DoLayoutChildren(last_layout_child, i, relative_children_count, &layout_s);
						layout_s.Z(); //LAYOUT_RESET();
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
		//layout_items(this, last_layout_child, getCount(), relative_children_count, layout);
		DoLayoutChildren(last_layout_child, getCount(), relative_children_count, &layout_s);
		// In wrap mode we may need to tweak the position of each line according to
		// the align_content property as well as the cross-axis size of items that
		// haven't been set yet.
		if(layout_s.need_lines && layout_s.lines_count > 0) {
			float pos = 0.0f;
			float spacing = 0.0f;
			float flex_dim = layout_s.align_dim - layout_s.lines_sizes;
			if(flex_dim > 0) {
				assert(layout_align(ALB.AlignContent, flex_dim, layout_s.lines_count, &pos, &spacing, true) && "incorrect align_content");
			}
			float old_pos = 0.0f;
			if(layout_s.reverse2) {
				pos = layout_s.align_dim - pos;
				old_pos = layout_s.align_dim;
			}
			for(uint i = 0; i < layout_s.lines_count; i++) {
				LayoutFlex::Line * p_line = &layout_s.P_Lines[i];
				if(layout_s.reverse2) {
					pos -= p_line->size;
					pos -= spacing;
					old_pos -= p_line->size;
				}
				// Re-position the children of this line, honoring any child
				// alignment previously set within the line.
				for(uint j = p_line->child_begin; j < p_line->child_end; j++) {
					//LayoutFlexItem * child = LAYOUT_CHILD_AT_((&layout_s), this, j);
					LayoutFlexItem & r_child = layout_s.GetChildByIndex(this, j);
					//if(r_child.PositionMode == FLEX_POSITION_ABSOLUTE) {
					//if(!(r_child.Flags & fPositionAbsolute)) { // Should not be re-positioned.
					if(!r_child.ALB.IsPositionAbsolute(AbstractLayoutBlock::GetCrossDirection(ALB.GetContainerDirection()))) {
						if(fisnanf(CHILD_SIZE2_((&layout_s), r_child))) {
							// If the child's cross axis size hasn't been set it, it defaults to the line size.
							CHILD_SIZE2_((&layout_s), r_child) = p_line->size + ((ALB.AlignContent == /*FLEX_ALIGN_STRETCH*/AbstractLayoutBlock::alignStretch) ? spacing : 0);
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

static void FASTCALL SetupItem(LayoutFlexItem & rItem)
{
	if(rItem.CbSetup) {
		rItem.CbSetup(&rItem, rItem.frame);
	}
	for(uint i = 0; i < rItem.getCount(); i++) {
		SetupItem(*rItem.at(i)); // @recursion
	}
}

int LayoutFlexItem::Evaluate()
{
	int    ok = -1;
	if(getCount() && oneof2(ALB.GetContainerDirection(), DIREC_HORZ, DIREC_VERT)) {
		if(!P_Parent) {
			float sx = 0.0f;
			float sy = 0.0f;
			const int stag_x = ALB.GetSizeX(&sx);
			const int stag_y = ALB.GetSizeY(&sy);
			//if(!fisnan(Size.X) && !fisnan(Size.Y) && !CbSelfSizing) {
			if(stag_x == AbstractLayoutBlock::szFixed && stag_y == AbstractLayoutBlock::szFixed && !CbSelfSizing) {
				assert(P_Parent == NULL);
				//assert(!fisnan(Size.X));
				assert(!fisnan(sx));
				//assert(!fisnan(Size.Y));
				assert(!fisnan(sy));
				assert(CbSelfSizing == NULL);
				DoLayout(/*Size.X*/sx, /*Size.Y*/sy);
				//SetupItem(pItem);
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

