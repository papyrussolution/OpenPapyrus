// UI.CPP
// Copyright (c) A.Sobolev 2011, 2016, 2018, 2020, 2023, 2024
// @codepage UTF-8
//
#include <slib-internal.h>
#pragma hdrstop
//
//
//
IMPL_INVARIANT_C(SScroller)
{
	S_INVARIANT_PROLOG(pInvP);
	S_ASSERT_P(!P_LineContent || P_LineContent->getCount() == ItemCount, pInvP);
	//S_ASSERT_P(!P_PageContent || P_PageContent->getCount() == PageCount, pInvP);
	S_ASSERT_P(!ItemCount || P.ItemIdxPageTop < ItemCount, pInvP);
	S_ASSERT_P(ItemCount || P.ItemIdxPageTop == ItemCount, pInvP);
	S_ASSERT_P(!ItemCount || P.ItemIdxCurrent < ItemCount, pInvP);
	S_ASSERT_P(ItemCount || P.ItemIdxCurrent == ItemCount, pInvP);
	S_INVARIANT_EPILOG(pInvP);
}

SScroller::SetupBlock::SetupBlock() : ItemCount(0), ViewSize(0.0f), FixedItemSize(0.0f)
{
}

SScroller::SScroller() : P_ItemSizeList(0), Flags(0), ItemCount(0), /*PageCount(0),*/PageCurrent(0), ViewSize(0.0f), FixedItemSize(0.0f),
	P_LineContent(0), P_PageContent(0)
{
}

SScroller::SScroller(const SScroller & rS) :  P_ItemSizeList(0), P_LineContent(0), P_PageContent(0),
	Flags(rS.Flags), ItemCount(rS.ItemCount), /*PageCount(rS.PageCount),*/PageCurrent(rS.PageCurrent), ViewSize(rS.ViewSize),
	FixedItemSize(rS.FixedItemSize), P(rS.P)
{
	if(rS.P_ItemSizeList) {
		SETIFZ(P_ItemSizeList, new FloatArray(*rS.P_ItemSizeList));
	}
	if(rS.P_LineContent) {
		if(SETIFZ(P_LineContent, new TSCollection <LongArray>))
			TSCollection_Copy(*P_LineContent, *rS.P_LineContent);
	}
	if(rS.P_PageContent) {
		if(SETIFZ(P_PageContent, new TSCollection <LongArray>)) {
			TSCollection_Copy(*P_PageContent, *rS.P_PageContent);
		}
	}
}

SScroller::~SScroller()
{
	delete P_ItemSizeList;
	delete P_LineContent;
	delete P_PageContent;
}

SScroller & SScroller::Z()
{
	Flags = 0;
	ItemCount = 0;
	PageCurrent = 0;
	ViewSize = 0.0f;
	FixedItemSize = 0.0f;
	P.Z();
	ZDELETE(P_ItemSizeList);
	ZDELETE(P_LineContent);
	ZDELETE(P_PageContent);
	return *this;
}

SScroller & FASTCALL SScroller::operator = (const SScroller & rS)
{
	return Copy(rS);
}

SScroller & FASTCALL SScroller::Copy(const SScroller & rS)
{
	Flags = rS.Flags;
	ItemCount = rS.ItemCount;
	//PageCount = rS.PageCount;
	PageCurrent = rS.PageCurrent;
	ViewSize = rS.ViewSize;
	FixedItemSize = rS.FixedItemSize;
	P = rS.P;
	if(rS.P_ItemSizeList) {
		if(!P_ItemSizeList)
			P_ItemSizeList = new FloatArray(*rS.P_ItemSizeList);
		else
			*P_ItemSizeList = *rS.P_ItemSizeList;
	}
	else {
		ZDELETE(P_ItemSizeList);
	}
	if(rS.P_LineContent) {
		if(SETIFZ(P_LineContent, new TSCollection <LongArray>))
			TSCollection_Copy(*P_LineContent, *rS.P_LineContent);
	}
	else {
		ZDELETE(P_LineContent);
	}
	if(rS.P_PageContent) {
		if(SETIFZ(P_PageContent, new TSCollection <LongArray>)) {
			TSCollection_Copy(*P_PageContent, *rS.P_PageContent);
		}
	}
	else {
		ZDELETE(P_PageContent);
	}
	return *this;
}

int SScroller::Setup(const SetupBlock & rBlk)
{
	int    ok = 1;
	assert(rBlk.ViewSize > 0.0f);
	assert(rBlk.FixedItemSize >= 0.0f);
	assert(rBlk.FixedItemSize == 0.0f || rBlk.ItemSizeList.getCount() == rBlk.ItemCount);
	if(rBlk.ViewSize > 0.0f && rBlk.FixedItemSize >= 0.0f && (rBlk.FixedItemSize == 0.0f || rBlk.ItemSizeList.getCount() == rBlk.ItemCount)) {
		ItemCount = rBlk.ItemCount;
		ViewSize = rBlk.ViewSize;
		if(rBlk.ItemCount && rBlk.ItemSizeList.getCount() == rBlk.ItemCount) {
			if(P_ItemSizeList)
				*P_ItemSizeList = rBlk.ItemSizeList;
			else
				P_ItemSizeList = new FloatArray(rBlk.ItemSizeList);
			assert(rBlk.LineContent.getCount() <= rBlk.ItemCount); // @todo Надо разбираться в каких случаях (rBlk.LineContent.getCount() < rBlk.ItemCount)
			P_LineContent = new TSCollection <LongArray>();
			if(P_LineContent)
				TSCollection_Copy(*P_LineContent, rBlk.LineContent);
		}
	}
	else
		ok = 0;
	return ok;
}

uint SScroller::GetCount() const { return ItemCount; }
uint SScroller::GetCurrentIndex() const { return P.ItemIdxCurrent; }

float SScroller::GetAbsolutePosition(uint idx) const
{
	assert(idx < ItemCount);
	float result = 0.0f;
	if(FixedItemSize > 0.0f) {
		result = (FixedItemSize * idx);
	}
	else if(P_ItemSizeList) {
		assert(P_ItemSizeList->getCount() == ItemCount);
		for(uint i = 0; i < idx; i++)
			result += P_ItemSizeList->at(i);
	}	
	return result;
}

float SScroller::GetCurrentPoint() const { return GetAbsolutePosition(P.ItemIdxCurrent); }
float SScroller::GetCurrentPageTopPoint() const { return GetAbsolutePosition(P.ItemIdxPageTop); }
uint SScroller::GetCurrentPageTopIndex() const { return P.ItemIdxPageTop; }

uint SScroller::GetPageBottomIndex(uint topIdx) const
{
	uint    result_idx = topIdx;
	assert(topIdx < ItemCount);
	if(ViewSize > 0.0f) {
		uint c = 0;
		if(FixedItemSize > 0.0f) {
			c = fceili(ViewSize / FixedItemSize);
		}
		else if(P_ItemSizeList) {
			assert(P_ItemSizeList->getCount() == ItemCount);
			float s = 0.0f;
			for(uint i = topIdx; i < ItemCount; i++) {
				if(s < ViewSize) {
					c++;
					s += P_ItemSizeList->at(i);
				}
				else
					break;
			}
		}
		if(c)
			result_idx = MIN(topIdx + c - 1, ItemCount-1);
	}
	assert(result_idx < ItemCount);
	return result_idx;
}

uint SScroller::GetPageTopIndex(uint bottomIdx) const
{
	uint    result_idx = 0;
	if(ItemCount) {
		const uint _local_bottom_idx = MIN(bottomIdx, (ItemCount-1));
		result_idx = _local_bottom_idx;
		if(ViewSize > 0.0f) {
			uint c = 0;
			if(FixedItemSize > 0.0f) {
				c = fceili(ViewSize / FixedItemSize);
			}
			else if(P_ItemSizeList) {
				assert(P_ItemSizeList->getCount() == ItemCount);
				float s = 0.0f;
				uint  i = _local_bottom_idx;
				do {
					assert(i < ItemCount);
					if(s < ViewSize) {
						c++;
						s += P_ItemSizeList->at(i);
					}
					else
						break;
				} while(i--); // !Опасное условие из-за uint: вверху цикла стоит assert с целью гарантировать корректность цикла
			}
			if(c) {
				if(_local_bottom_idx > (c - 1))
					result_idx = _local_bottom_idx - (c - 1);
				else
					result_idx = 0;
			}
		}
	}
	return result_idx;
}

uint SScroller::AdjustTopIdx(uint idx) const
{
	const uint temp_bottom_idx = GetPageBottomIndex(idx);
	const uint temp_top_idx = GetPageTopIndex(temp_bottom_idx);
	if(temp_top_idx < idx) {
		//
		// Если после перемещения фрейма последняя страница оказалась заполнена лишь частично,
		// то смещаем результирующий page-top так чтобы страница стала заполнена максимально.
		//
		idx = temp_top_idx;
	}
	return idx;
}

int FASTCALL SScroller::CheckLineContentIndex(long idx) const
{
	int    ok = 0;
	if(P_LineContent && P_LineContent->getCount() == ItemCount) {
		const uint top_idx = P.ItemIdxPageTop;
		const uint bottom_idx = GetPageBottomIndex(P.ItemIdxPageTop);
		for(uint i = top_idx; !ok && i <= bottom_idx && i < ItemCount; i++) {
			const LongArray * p_item = P_LineContent->at(i);
			if(p_item && p_item->lsearch(idx))
				ok = 1;
		}
	}
	else
		ok = -1;
	return ok;
}

SScroller::Position::Position() : ItemIdxPageTop(0), ItemIdxCurrent(0)
{
}
		
SScroller::Position & SScroller::Position::Z()
{
	ItemIdxPageTop = 0;
	ItemIdxCurrent = 0;
	return *this;
}

int SScroller::GetPosition(Position & rP) const
{
	rP = P;
	return 1;
}

int SScroller::SetPosition(const Position & rP)
{
	SetCurrentIndex(rP.ItemIdxCurrent);
	return 1;
}

int SScroller::SetCurrentIndex(uint idx)
{
	int    updated = 0;
	const uint prev_page_top_idx = P.ItemIdxPageTop;
	const uint prev_cur_idx = P.ItemIdxCurrent;
	SETMIN(idx, (ItemCount-1));
	if(idx != prev_cur_idx) {
		const uint prev_bottom_idx = GetPageBottomIndex(P.ItemIdxPageTop);
		if(idx >= P.ItemIdxPageTop && idx <= prev_bottom_idx) { // Фрейм не смещается поскольку мы переместились в его пределах
			P.ItemIdxCurrent = idx;
			updated = 1;
		}
		else if(idx > P.ItemIdxPageTop) { // Перемещаемся вниз
			P.ItemIdxPageTop = idx;
			P.ItemIdxCurrent = idx;
			updated = 1;
		}
		else { // Перемещаемся вверх
			P.ItemIdxPageTop = GetPageTopIndex(idx);
			P.ItemIdxCurrent = idx;
			updated = 1;
		}
	}
	return updated;
}

int SScroller::LineDown(uint ic, bool moveCursor)
{
	int    updated = 0;
	if(ic && ItemCount) {
		if(moveCursor) {
			uint   new_idx = MIN((P.ItemIdxCurrent + ic), (ItemCount-1));
			if(new_idx != P.ItemIdxCurrent) {
				const uint bottom_idx = GetPageBottomIndex(P.ItemIdxPageTop);
				if(new_idx <= bottom_idx) {
					if(new_idx < P.ItemIdxPageTop) {
						// сдвинуть фрейм так, чтобы ItemIdxCurrent стал равен ItemIdxPageTop
						P.ItemIdxPageTop = new_idx;
					}
				}
				else {
					// сдвинуть фрейм вниз
					uint   bottom_idx = new_idx;
					P.ItemIdxPageTop = GetPageTopIndex(bottom_idx);
				}
				P.ItemIdxCurrent = new_idx;
				updated = 1;
			}
		}
		else {
			uint new_top_idx = AdjustTopIdx(MIN(P.ItemIdxPageTop + ic, (ItemCount-1)));
			if(new_top_idx != P.ItemIdxPageTop) {
				P.ItemIdxPageTop = new_top_idx;
				updated = 1;
			}
		}
	}
	return updated;
}

int SScroller::LineUp(uint ic, bool moveCursor)
{
	int    updated = 0;
	if(ic && ItemCount) {
		if(moveCursor) {
			uint   new_idx = (P.ItemIdxCurrent > ic) ? (P.ItemIdxCurrent - ic) : 0;
			if(new_idx != P.ItemIdxCurrent) {
				if(new_idx >= P.ItemIdxPageTop) {
					const uint bottom_idx = GetPageBottomIndex(P.ItemIdxPageTop);
					if(new_idx > bottom_idx) {
						// new_idx становится нижним элементом страницы
						P.ItemIdxPageTop = GetPageTopIndex(new_idx);
					}
				}
				else {
					// new_idx становится верхним элементом страницы
					P.ItemIdxPageTop = new_idx;
				}
				P.ItemIdxCurrent = new_idx;
				updated = 1;
			}
		}
		else {
			const uint cur_bottom_idx = GetPageBottomIndex(P.ItemIdxPageTop);
			uint    new_bottom_idx = cur_bottom_idx ? MIN(cur_bottom_idx-1, (ItemCount-1)) : 0;
			uint    new_top_idx = GetPageTopIndex(new_bottom_idx);
			//uint    new_top_idx = (ItemIdxCurrent > ic) ? (ItemIdxCurrent - ic) : 0;
			if(new_top_idx != P.ItemIdxPageTop) {
				P.ItemIdxPageTop = new_top_idx;
				updated = 1;
			}
		}
	}
	return updated;
}

int SScroller::PageDown(uint pc)
{
	int    updated = 0;
	if(pc && ItemCount) {
		uint page_top_idx = P.ItemIdxPageTop;
		uint cur_idx = P.ItemIdxCurrent;
		for(uint pi = 0; pi < pc; pi++) {
			const uint bottom_idx = GetPageBottomIndex(page_top_idx);
			const uint prev_page_top_idx = page_top_idx;
			const uint prev_cur_idx = cur_idx;
			if(Flags & fUseCursor) {
				if(Flags & fFirstPageMoveToEdge && cur_idx < bottom_idx) {
					cur_idx = bottom_idx;
				}
				else {
					const uint cur_delta = (cur_idx > page_top_idx) ? (cur_idx - page_top_idx) : 0;
					page_top_idx = AdjustTopIdx(MIN(bottom_idx+1, ItemCount-1));
					if(Flags & fFirstPageMoveToEdge)
						cur_idx = GetPageBottomIndex(page_top_idx);
					else
						cur_idx = page_top_idx + cur_delta;
				}
			}
			else {
				page_top_idx = AdjustTopIdx(MIN(bottom_idx+1, ItemCount-1));
			}
			if(cur_idx != prev_cur_idx || page_top_idx != prev_page_top_idx) {
				updated = 1;
			}
			else
				break;
		}
		P.ItemIdxPageTop = page_top_idx;
		P.ItemIdxCurrent = cur_idx;
	}
	return updated;
}

int SScroller::PageUp(uint pc)
{
	int    updated = 0;
	if(pc && ItemCount) {
		uint page_top_idx = P.ItemIdxPageTop;
		uint cur_idx = P.ItemIdxCurrent;
		for(uint pi = 0; pi < pc; pi++) {
			uint bottom_idx = GetPageBottomIndex(page_top_idx);
			const uint prev_page_top_idx = page_top_idx;
			const uint prev_cur_idx = cur_idx;
			if(Flags & fUseCursor) {
				if(Flags & fFirstPageMoveToEdge && cur_idx > page_top_idx) {
					cur_idx = page_top_idx;
				}
				else {
					const uint cur_delta = (cur_idx > page_top_idx) ? (cur_idx - page_top_idx) : 0;
					bottom_idx = page_top_idx ? (page_top_idx - 1) : 0;
					page_top_idx = GetPageTopIndex(bottom_idx);
					if(Flags & fFirstPageMoveToEdge)
						cur_idx = page_top_idx;
					else
						cur_idx = page_top_idx + cur_delta;
				}
			}
			else {
				bottom_idx = page_top_idx ? (page_top_idx - 1) : 0;
				page_top_idx = GetPageTopIndex(bottom_idx);
			}
			if(cur_idx != prev_cur_idx || page_top_idx != prev_page_top_idx) {
				updated = 1;
			}
			else
				break;
		}
		P.ItemIdxPageTop = page_top_idx;
		P.ItemIdxCurrent = cur_idx;
	}
	return updated;
}

int SScroller::Top()
{
	int    updated = 0;
	const uint prev_page_top_idx = P.ItemIdxPageTop;
	const uint prev_cur_idx = P.ItemIdxCurrent;
	P.Z();
	if(P.ItemIdxPageTop != prev_page_top_idx)
		updated = 1;
	else if(Flags & fUseCursor && P.ItemIdxCurrent != prev_cur_idx)
		updated = 1;
	return updated;
}

int SScroller::Bottom()
{
	int    updated = 0;
	const uint prev_page_top_idx = P.ItemIdxPageTop;
	const uint prev_cur_idx = P.ItemIdxCurrent;
	if(ItemCount) {
		P.ItemIdxPageTop = GetPageTopIndex(ItemCount-1);
		P.ItemIdxCurrent = ItemCount-1;
	}
	else {
		P.ItemIdxPageTop = 0;
		P.ItemIdxCurrent = 0;		
	}
	if(P.ItemIdxPageTop != prev_page_top_idx)
		updated = 1;
	else if(Flags & fUseCursor && P.ItemIdxCurrent != prev_cur_idx)
		updated = 1;
	return updated;
}
//
//
//
/*static*/int UiItemKind::GetTextList(StrAssocArray & rList)
{
	int    ok = 1;
	rList.Z();
	UiItemKind item;
	for(int i = kDialog; i < kCount; i++) {
		if(item.Init(i)) {
			rList.Add(i, item.Text);
		}
	}
	return ok;
}

/*static*/int UiItemKind::GetIdBySymb(const char * pSymb)
{
	int    id = 0;
	UiItemKind item;
	for(int i = kDialog; !id && i < kCount; i++) {
		if(item.Init(i) && item.Symb.Cmp(pSymb, 0) == 0)
			id = i;
	}
	return id;
}

UiItemKind::UiItemKind(int kind)
{
	Init(kind);
}

int UiItemKind::Init(int kind)
{
	int    ok = 1;
	Id = 0;
	P_Cls = 0;
	Symb.Z();
	Text.Z();
	const char * p_text_sign = 0;
	switch(kind) {
		case kUnkn:
			break;
		case kDialog:
			p_text_sign = "ui_dialog";
			Symb = "dialog";
			break;
		case kInput:
			p_text_sign = "ui_input";
			Symb = "input";
			break;
		case kStatic:
			p_text_sign = "ui_static";
			Symb = "statictext";
			break;
		case kPushbutton:
			p_text_sign = "ui_pushbutton";
			Symb = "button";
			break;
		case kCheckbox:
			p_text_sign = "ui_checkbox";
			Symb = "checkbox";
			break;
		case kRadioCluster:
			p_text_sign = "ui_radiocluster";
			Symb = "radiocluster";
			break;
		case kCheckCluster:
			p_text_sign = "ui_checkcluster";
			Symb = "checkcluster";
			break;
		case kCombobox:
			p_text_sign = "ui_combobox";
			Symb = "combobox";
			break;
		case kListbox:
			p_text_sign = "ui_listbox";
			Symb = "listbox";
			break;
		case kTreeListbox:
			p_text_sign = "ui_treelistbox";
			Symb = "treelistbox";
			break;
		case kFrame:
			p_text_sign = "ui_frame";
			Symb = "framebox";
			break;
		default:
			ok = 0;
			break;
	}
	if(p_text_sign) {
		Id = kind;
		SLS.LoadString_(p_text_sign, Text);
	}
	return ok;
}
//
// @v11.7.10 @construction {
//
SColorSet::InnerEntry::InnerEntry() : C(ZEROCOLOR), CcbP(0)
{
}

const void * SColorSet::InnerEntry::GetHashKey(const void * pCtx, uint * pKeyLen) const // Descr: Каноническая функция возвращающая ключ экземпляра для хэширования.
{
	const  void * p_result = 0;
	uint   key_len = 0;
	assert(pCtx);
	if(pCtx) {
		//
		// Здесь я применяю довольно рискованную тактику получения ключа: формирую его в револьверном буфере SString.
		// Однако, учитывая то, что результат работы GetHashKey очень короткоживущий, вероятность перезаписи этого
		// буфера в течении времени, когда он реально нужен, мизерная.
		//
		SString & r_key_buf = SLS.AcquireRvlStr();
		static_cast<const SColorSet *>(pCtx)->GetS(SymbP, r_key_buf);
		r_key_buf.Utf8ToLower();
		p_result = r_key_buf.cptr();
		key_len = r_key_buf.Len();
	}
	ASSIGN_PTR(pKeyLen, key_len);
	return p_result;
}

SColorSet::SColorSet(const char * pSymb) : L(256, this), Symb(pSymb), State(0)
{
}

bool FASTCALL SColorSet::IsEq(const SColorSet & rS) const
{
	bool   eq = true;
	if(!Symb.IsEqiAscii(rS.Symb))
		eq = false;
	else {
		const uint _c = L.GetCount();
		if(_c != rS.L.GetCount())
			eq = false;
		else {
			InnerEntry * p_e1;
			InnerEntry * p_e2;
			{
				for(uint idx1 = 0; eq && L.Enum(&idx1, &p_e1);) {
					bool found = false;
					assert(p_e1);
					for(uint idx2 = 0; !found && rS.L.Enum(&idx2, &p_e2);) {
						assert(p_e2);
						if(IsInnerEntryEq(*p_e1, rS, *p_e2))
							found = true;
					}
					if(!found)
						eq = false;
				}
			}
			{
				for(uint idx1 = 0; eq && rS.L.Enum(&idx1, &p_e1);) {
					bool found = false;
					assert(p_e1);
					for(uint idx2 = 0; !found && L.Enum(&idx2, &p_e2);) {
						assert(p_e2);
						if(rS.IsInnerEntryEq(*p_e1, *this, *p_e2))
							found = true;
					}
					if(!found)
						eq = false;
				}
			}
		}
	}
	return eq;
}

SColorSet & SColorSet::Z()
{
	State = 0;
	Symb.Z();
	L.Z();
	return *this;
}

int SColorSet::SetSymb(const char * pSymb)
{
	Symb = pSymb;
	return 1;
}
	
const void * SColorSet::GetHashKey(const void * pCtx, uint * pKeyLen) const // Descr: Каноническая функция возвращающая ключ экземпляра для хэширования
{
	ASSIGN_PTR(pKeyLen, Symb.Len());
	return Symb.cptr();
}

SColorSet::ColorArg::ColorArg() : C(ZEROCOLOR)
{
}

SColorSet::ColorArg & SColorSet::ColorArg::Z()
{
	C = ZEROCOLOR;
	F = 0.0f;
	RefSymb.Z();
	return *this;
}

int SColorSet::ColorArg::GetType() const
{
	int   t = argtNone;
	if(RefSymb.NotEmpty())
		t = argtRefColor;
	else if(!C.IsEmpty())
		t = argtAbsoluteColor;
	else if(F > 0.0f)
		t = argtNumber;
	return t;
}

SString & SColorSet::ColorArg::ToStr(SString & rBuf) const
{
	rBuf.Z();
	if(RefSymb.NotEmpty()) {
		rBuf.CatChar('$').Cat(RefSymb);
	}
	else if(!C.IsEmpty()) {
		C.ToStr(rBuf, SColor::fmtHEX|SColor::fmtName);
		if(C.Alpha < 255) {
			rBuf.CatChar('|').Cat(C.Alpha);
		}
	}
	else if(F > 0.0f)
		rBuf.Cat(F, MKSFMTD(0, 2, NMBF_NOTRAILZ|NMBF_OMITEPS));
	return rBuf;
}

SColorSet::ComplexColorBlock::ComplexColorBlock() : C(ZEROCOLOR), Func(funcNone)
{
}

SColorSet::ComplexColorBlock::ComplexColorBlock(const ComplexColorBlock & rS)
{
	Copy(rS);
}

bool FASTCALL SColorSet::ComplexColorBlock::IsEq(const ComplexColorBlock & rS) const
{
	bool eq = true;
	if(C != rS.C)
		eq = false;
	else if(Func != rS.Func)
		eq = false;
	else if(!TSCollection_IsEq(&ArgList, &rS.ArgList))
		eq = false;
	return eq;
}

SColorSet::ComplexColorBlock & FASTCALL SColorSet::ComplexColorBlock::operator = (const ComplexColorBlock & rS)
{
	return Copy(rS);
}
		
SColorSet::ComplexColorBlock & SColorSet::ComplexColorBlock::Copy(const ComplexColorBlock & rS)
{
	C = rS.C;
	RefSymb = rS.RefSymb;
	Func = rS.Func;
	TSCollection_Copy(ArgList, rS.ArgList);
	return *this;
}

SColorSet::ComplexColorBlock & SColorSet::ComplexColorBlock::Z()
{
	C = ZEROCOLOR;
	RefSymb.Z();
	Func = funcNone;
	ArgList.clear();
	return *this;
}

struct SColorSet_FuncEntry : public SIntToSymbTabEntry {
	uint   ArcCount;
};

static const SIntToSymbTabEntry SColorSet_FuncList[] = {
	{ SColorSet::funcEmpty, "empty" },
	{ SColorSet::funcLerp, "lerp" },
	{ SColorSet::funcLighten, "lighten" },
	{ SColorSet::funcDarken, "darken" },
	{ SColorSet::funcGrey, "grey" },
};

static uint SColorSet_GetFuncArcCount(int func)
{
	static const LAssoc FuncArcCountList[] = {
		{ SColorSet::funcEmpty, 0 },
		{ SColorSet::funcLerp, 3 },
		{ SColorSet::funcLighten, 2 },
		{ SColorSet::funcDarken, 2 },
		{ SColorSet::funcGrey, 1 },
	};
	uint result = 0;
	for(uint i = 0; i < SIZEOFARRAY(FuncArcCountList); i++) {
		if(FuncArcCountList[i].Key == func) {
			result = static_cast<uint>(FuncArcCountList[i].Val);
			break;
		}
	}
	return result;
}
		
SString & SColorSet::ComplexColorBlock::ToStr(SString & rBuf) const
{
	rBuf.Z();
	SString temp_buf;
	if(Func) {
		if(SIntToSymbTab_GetSymb(SColorSet_FuncList, SIZEOFARRAY(SColorSet_FuncList), Func, temp_buf)) {
			assert(temp_buf.NotEmpty());
			rBuf.Cat(temp_buf);
			for(uint i = 0; i < ArgList.getCount(); i++) {
				const ColorArg * p_arg = ArgList.at(i);
				if(p_arg) {
					p_arg->ToStr(temp_buf);
					rBuf.Space().Cat(temp_buf);
				}
			}
		}
		else {
			; // @todo @err
		}
	}
	else if(RefSymb.NotEmpty()) {
		rBuf.CatChar('$').Cat(RefSymb);
		if(C.Alpha > 0 && C.Alpha < 255)
			rBuf.CatChar('|').Cat(C.Alpha);
	}
	else if(!C.IsEmpty()) {
		C.ToStr(rBuf, SColor::fmtHEX|SColor::fmtName);
		if(C.Alpha < 255)
			rBuf.CatChar('|').Cat(C.Alpha);
	}
	else {
		;
	}
	return rBuf;
}

int SColorSet::Helper_ParsePrimitive(SStrScan & rScan, ColorArg & rItem) const
{
	rItem.Z();
	int    ok = 1;
	bool   debug_mark = false;
	bool   syntax_err = false;
	SString temp_buf;
	rScan.Skip();
	if(rScan.GetNumber(temp_buf)) {
		rItem.F = static_cast<float>(temp_buf.ToReal_Plain());
		ok = 2;
	}
	else if(rScan[0] == '#') {
		rScan.Incr();
		rScan.GetUntil('|', temp_buf);
		temp_buf.Insert(0, "#");
		THROW(rItem.C.FromStr(temp_buf)); // @todo @err
		if(rScan[0] == '|') {
			// alpha
			rScan.Incr();
			THROW(rScan.GetNumber(temp_buf)); // @todo @err (alpha value needed)
			const double alpha = temp_buf.ToReal_Plain();
			if(alpha > 1.0 && alpha < 256.0)
				rItem.C.SetAlpha(static_cast<uint8>(alpha));
			else if(alpha >= 0.0 && alpha <= 1.0)
				rItem.C.SetAlphaF(static_cast<float>(alpha));
			else {
				CALLEXCEPT(); // @todo @err (invalid alpha value)
			}
			if(rItem.C.IsEmpty()) {
				// Эксклюзивный случай: валидное текстовое представление цвета
				// определяет пустое с точки зрения slib значение {0,0,0,0}
				// для того, что бы другие слои подсистемы не трактовали его
				// как инвалидное определим alpha как 1, что в реальности 
				// почти не отличается от нуля (я, по крайней мере, так надеюсь).
				//rItem.C.Alpha = 1;
				debug_mark = true;
			}
		}
	}
	else if(rScan[0] == '$') {
		rScan.Incr();
		THROW(rScan.GetIdent(temp_buf)); // @todo @err (invalid color ref symb)
		// ссылка на другой цвет набора
		if(rScan[0] == '.') {
			rScan.Incr();
			const SString set_symb(temp_buf);
			THROW(rScan.GetIdent(temp_buf)); // @todo @err (invalid colorset symb)
			(rItem.RefSymb = set_symb).Dot().Cat(temp_buf);
		}
		else {
			rItem.RefSymb = temp_buf;
		}
		if(rScan[0] == '|') {
			// alpha
			rScan.Incr();
			THROW(rScan.GetNumber(temp_buf)); // @todo @err (alpha value needed)
			const double alpha = temp_buf.ToReal_Plain();
			if(alpha > 1.0 && alpha < 256.0)
				rItem.C.SetAlpha(static_cast<uint8>(alpha));
			else if(alpha >= 0.0 && alpha <= 1.0)
				rItem.C.SetAlphaF(static_cast<float>(alpha));
			else {
				CALLEXCEPT(); // @todo @err (invalid alpha value)
			}
		}
		ok = 3;
	}
	else {
		CALLEXCEPT(); // @todo @err (syntax error)
	}
	CATCHZOK
	return ok;
}

int SColorSet::ParseComplexColorBlock(const char * pText, ComplexColorBlock & rBlk) const
{
	// #xxxxxx
	// #xxxxxx|xx - with alpha
	// $primary - referece
	// lighten $secondary 0.2
	// darken $primary|xx 0.35
	// lerp $primary #xxxxxx
	// lerp $secondary 0.7
	// 
	int    ok = 1;
	SString src_buf(pText);
	if(!src_buf.NotEmptyS()) {
		ok = -1;
	}
	else {
		SString temp_buf;
		SStrScan scan(src_buf);
		if(scan.GetIdent(temp_buf)) {
			//funcLerp,      // (color, color, factor)
			//funcLighten,   // (color, factor)
			//funcDarken,    // (color, factor)
			//funcGrey,      // (whitePart)
			const int func = SIntToSymbTab_GetId(SColorSet_FuncList, SIZEOFARRAY(SColorSet_FuncList), temp_buf);
			THROW(func != funcNone); // @todo @err
			rBlk.Func = func;
			scan.Skip();
			while(ok && !scan.IsEnd()) {
				ColorArg primitive;
				int gpr = Helper_ParsePrimitive(scan, primitive);
				if(gpr > 0) {
					ColorArg * p_new_arg = rBlk.ArgList.CreateNewItem();
					*p_new_arg = primitive;
				}
				else {
					; // @todo @err
					ok = 0;
				}
				scan.Skip().IncrChr(',');
			}
			if(ok) {
				THROW(SColorSet_GetFuncArcCount(func) == rBlk.ArgList.getCount()); // @todo @err
				switch(func) {
					case funcEmpty:
						break;
					case funcLerp:
						{
							const ColorArg * p_arg1 = rBlk.ArgList.at(0);
							const int argt = p_arg1->GetType();
							THROW(oneof2(argt, argtAbsoluteColor, argtRefColor)); // @todo @err
						}
						{
							const ColorArg * p_arg2 = rBlk.ArgList.at(1);
							const int argt = p_arg2->GetType();
							THROW(oneof2(argt, argtAbsoluteColor, argtRefColor)); // @todo @err
						}
						{
							const ColorArg * p_arg3 = rBlk.ArgList.at(2);
							const int argt = p_arg3->GetType();
							THROW(argt == argtNumber); // @todo @err
						}
						break;
					case funcLighten:
					case funcDarken:
						{
							const ColorArg * p_arg1 = rBlk.ArgList.at(0);
							THROW(!p_arg1->C.IsEmpty() || p_arg1->RefSymb.NotEmpty());
						}
						{
							const ColorArg * p_arg2 = rBlk.ArgList.at(1);
							THROW(p_arg2->F > 0.0f);
						}
						break;
					case funcGrey:
						{
							const ColorArg * p_arg1 = rBlk.ArgList.at(0);
							THROW(p_arg1->F > 0.0f);
						}
						break;
					default:
						assert(0); // unknown function
						break;
				}
			}
		}
		else {
			ColorArg primitive;
			int gpr = Helper_ParsePrimitive(scan, primitive);
			switch(gpr) {
				case 1: // color
					rBlk.C = primitive.C;
					break;
				case 2: // number
					CALLEXCEPT(); // @todo @err // Число здесь не допускается //
					break;
				case 3: // reference
					rBlk.RefSymb = primitive.RefSymb;
					rBlk.C = primitive.C;
					break;
				default:
					CALLEXCEPT(); // @todo @err
					break;
			}
		}
	}
	CATCHZOK
	return ok;
}

int SColorSet::Resolve(const TSCollection <SColorSet> * pSetList)
{
	int    ok = 1;
	if(State & stateResolved) {
		ok = -1;
	}
	else {
		InnerEntry * p_entry = 0;
		for(uint i = 0; L.Enum(&i, &p_entry);) {
			if(p_entry->CcbP) {
				if(p_entry->CcbP <= CcC.getCount()) {
					const ComplexColorBlock * p_ccb = CcC.at(p_entry->CcbP-1);
					if(p_ccb) {
						SColor c;
						StringSet recur_symb_list;
						if(ResolveComplexColorBlock(*p_ccb, pSetList, c, recur_symb_list)) {
							p_entry->C = c;
						}
						else {
							; // @todo @err
							ok = 0;
						}
					}
					else {
						; // @todo @err
						ok = 0;
					}
				}
				else {
					; // @todo @err
					ok = 0;
				}
			}
			else {
			}
		}
		State |= stateResolved;
	}
	return ok;
}

int SColorSet::ResolveComplexColorBlock(const ComplexColorBlock & rBlk, const TSCollection <SColorSet> * pSetList, SColor & rC, StringSet & rRecurSymbList) const
{
	int    ok = 1;
	if(rBlk.Func) {
		THROW(SColorSet_GetFuncArcCount(rBlk.Func) == rBlk.ArgList.getCount()); // @todo @err
		switch(rBlk.Func) {
			case funcEmpty:
				rC = ZEROCOLOR;
				break;
			case funcLerp:
				{
					SColor arg1_c(ZEROCOLOR);
					SColor arg2_c(ZEROCOLOR);
					float arg3_n = 0.0f;
					{
						const ColorArg * p_arg = rBlk.ArgList.at(0);
						THROW(p_arg); // @todo @err
						const int argt = p_arg->GetType();
						if(argt == argtAbsoluteColor) {
							arg1_c = p_arg->C;
						}
						else if(argt == argtRefColor) {
							THROW(Helper_Get(p_arg->RefSymb, pSetList, arg1_c, &rRecurSymbList));
						}
						else {
							CALLEXCEPT(); // @todo @err
						}
					}
					{
						const ColorArg * p_arg = rBlk.ArgList.at(1);
						THROW(p_arg); // @todo @err
						const int argt = p_arg->GetType();
						if(argt == argtAbsoluteColor) {
							arg2_c = p_arg->C;
						}
						else if(argt == argtRefColor) {
							THROW(Helper_Get(p_arg->RefSymb, pSetList, arg2_c, &rRecurSymbList));
						}
						else {
							CALLEXCEPT(); // @todo @err
						}
					}
					{
						const ColorArg * p_arg = rBlk.ArgList.at(2);
						THROW(p_arg); // @todo @err
						const int argt = p_arg->GetType();
						if(argt == argtNumber) {
							arg3_n = p_arg->F;
						}
						else {
							CALLEXCEPT(); // @todo @err
						}
					}
					//
					rC = SColor::Lerp(arg1_c, arg2_c, arg3_n);
				}
				break;
			case funcLighten:
				{
					SColor arg1_c(ZEROCOLOR);
					float arg2_n = 0.0f;
					{
						const ColorArg * p_arg = rBlk.ArgList.at(0);
						THROW(p_arg); // @todo @err
						const int argt = p_arg->GetType();
						if(argt == argtAbsoluteColor) {
							arg1_c = p_arg->C;
						}
						else if(argt == argtRefColor) {
							THROW(Helper_Get(p_arg->RefSymb, pSetList, arg1_c, &rRecurSymbList));
						}
						else {
							CALLEXCEPT(); // @todo @err
						}
					}
					{
						const ColorArg * p_arg = rBlk.ArgList.at(1);
						THROW(p_arg); // @todo @err
						const int argt = p_arg->GetType();
						if(argt == argtNumber) {
							arg2_n = p_arg->F;
						}
						else {
							CALLEXCEPT(); // @todo @err
						}
					}
					rC = arg1_c.Lighten(arg2_n);
				}
				break;
			case funcDarken:
				{
					SColor arg1_c(ZEROCOLOR);
					float arg2_n = 0.0f;	
					{
						const ColorArg * p_arg = rBlk.ArgList.at(0);
						THROW(p_arg); // @todo @err
						const int argt = p_arg->GetType();
						if(argt == argtAbsoluteColor) {
							arg1_c = p_arg->C;
						}
						else if(argt == argtRefColor) {
							THROW(Helper_Get(p_arg->RefSymb, pSetList, arg1_c, &rRecurSymbList));
						}
						else {
							CALLEXCEPT(); // @todo @err
						}
					}
					{
						const ColorArg * p_arg = rBlk.ArgList.at(1);
						THROW(p_arg); // @todo @err
						const int argt = p_arg->GetType();
						if(argt == argtNumber) {
							arg2_n = p_arg->F;
						}
						else {
							CALLEXCEPT(); // @todo @err
						}
					}
					rC = arg1_c.Darken(arg2_n);
				}
				break;
			case funcGrey:
				{
					float arg1_n = 0.0f;
					{
						const ColorArg * p_arg = rBlk.ArgList.at(0);
						THROW(p_arg); // @todo @err
						const int argt = p_arg->GetType();
						if(argt == argtNumber) {
							arg1_n = p_arg->F;
						}
						else {
							CALLEXCEPT(); // @todo @err
						}
					}
					rC = SColor(arg1_n);
				}
				break;
			default:
				CALLEXCEPT(); // @todo @err
				break;
		}
	}
	else if(rBlk.RefSymb.NotEmpty()) {
		THROW(Helper_Get(rBlk.RefSymb, pSetList, rC, &rRecurSymbList));
		{
			float alpha = rBlk.C.AlphaF();
			if(alpha > 0.0f)
				rC.SetAlphaF(alpha);
		}
		//Get(rBlk.RefSymb, 0);
	}
	else if(!rBlk.C.IsEmpty()) {
		rC = rBlk.C;
	}
	CATCHZOK
	return ok;
}

SJson * SColorSet::ToJsonObj() const
{
	SJson * p_result = SJson::CreateObj();
	InnerEntry * p_entry = 0;
	SString symb_buf;
	SString val_buf;
	p_result->InsertStringNe("symb", GetSymb());
	for(uint idx = 0; L.Enum(&idx, &p_entry);) {
		GetS(p_entry->SymbP, symb_buf);
		if(symb_buf.NotEmptyS()) {
			if(p_entry->CcbP) {
				if(p_entry->CcbP <= CcC.getCount()) {
					const ComplexColorBlock * p_ccb = CcC.at(p_entry->CcbP-1);
					if(p_ccb) {
						p_ccb->ToStr(val_buf);
					}
					else {
						; // @todo @err
					}
				}
				else {
					; // @todo @err
				}
			}
			else {
				p_entry->C.ToStr(val_buf, SColor::fmtName|SColor::fmtHEX|SColor::fmtForceHashPrefix);
				if(p_entry->C.Alpha < 255)
					val_buf.CatChar('|').Cat(p_entry->C.Alpha);
			}
			p_result->InsertString(symb_buf.Escape(), val_buf);
		}
	}
	return p_result;
}

bool SColorSet::ValidateRefs(const TSCollection <SColorSet> * pSetList) const
{
	// Проверка ссылок во всем наборе:
	// -- символы, на которые ссылаются элементы должны существовать
	// -- не должно быть рекурсивных зависимостей
	// все проверки сделает функция Get() - она должна для каждого элемента набора вычислить эффективный цвет и вернуть значение больше нуля.
	bool   ok = true;
	SString temp_buf;
	InnerEntry * p_entry = 0;
	for(uint idx = 0; L.Enum(&idx, &p_entry);) {
		if(p_entry) {
			SColor c;
			GetS(p_entry->SymbP, temp_buf);
			THROW(Get(temp_buf, pSetList, c) > 0);
		}
	}
	CATCHZOK
	return ok;
}
	
int SColorSet::FromJsonObj(const SJson * pJs)
{
	int    ok = 1;
	SString symb_buf;
	SString val_buf;
	THROW(SJson::IsObject(pJs)); // @todo @err
	Z();
	for(const SJson * p_jsn = pJs->P_Child; p_jsn; p_jsn = p_jsn->P_Next) {
		if(p_jsn->P_Child) {
			(symb_buf = p_jsn->Text).Unescape();
			val_buf = p_jsn->P_Child->Text;
			if(symb_buf.IsEqiAscii("symb")) {
				SetSymb(val_buf.Unescape());
			}
			else {
				ComplexColorBlock ccb;
				THROW(ParseComplexColorBlock(val_buf, ccb));
				THROW(Put(symb_buf, new ComplexColorBlock(ccb)));
			}
		}
	}
	// Теперь мы должны проверить ссылки во всем наборе
	// @v11.9.10 (проверка выполняется в экземпляре UiDescription после загрузки всех цветовых наборов) THROW(ValidateRefs(pSetList));
	CATCHZOK
	return ok;
}
	
int SColorSet::Put(const char * pSymb, ComplexColorBlock * pBlk)
{
	int    ok = 1;
	bool   do_delete_blk = true;
	THROW_S_S(!isempty(pSymb), SLERR_INVPARAM, __FUNCTION__"/pSymb");
	THROW(pBlk);
	{
		InnerEntry * p_new_entry = 0;
		if(pBlk->Func || pBlk->RefSymb.NotEmpty()) {
			CcC.insert(pBlk);
			p_new_entry = new InnerEntry;
			p_new_entry->CcbP = CcC.getCount(); // Индекс получаем после вставки так как поле значений этих индексов начинается с единицы ([1..CcC.getCount()])
			do_delete_blk = false;
		}
		else if(pBlk->C.IsEmpty()) {
			pBlk->Func = funcEmpty;
			CcC.insert(pBlk);
			p_new_entry = new InnerEntry;
			p_new_entry->CcbP = CcC.getCount(); // Индекс получаем после вставки так как поле значений этих индексов начинается с единицы ([1..CcC.getCount()])
			do_delete_blk = false;
		}
		else {
			//THROW(!pBlk->C.IsEmpty()); // @todo @err
			p_new_entry = new InnerEntry;
			p_new_entry->C = pBlk->C;
		}
		AddS(pSymb, &p_new_entry->SymbP);
		ok = L.Put(p_new_entry, true/*forceUpdate*/);
	}
	CATCHZOK
	if(do_delete_blk)
		delete pBlk;
	return ok;
}

int SColorSet::Get(const char * pSymb, ComplexColorBlock * pBlk) const
{
	return Get(pSymb, 0, pBlk);
}

int SColorSet::Get(const char * pSymb, const TSCollection <SColorSet> * pSetList, ComplexColorBlock * pBlk) const
{
	int    ok = -1;
	if(!isempty(pSymb)) {
		SString & r_key_buf = SLS.AcquireRvlStr();
		r_key_buf.Cat(pSymb).Utf8ToLower();
		// @v11.9.10 Резолвинг символа цвета, относящегося к другому сету {
		if(r_key_buf.HasChr('.')) {
			SString & r_left = SLS.AcquireRvlStr();
			SString & r_right = SLS.AcquireRvlStr();
			const int dr = r_key_buf.Divide('.', r_left, r_right);
			assert(dr > 0); // Мы только что выше убедились, что в исходной строке есть точка ('.')
			if(r_left.IsEqiAscii(Symb)) {
				ok = Get(r_right, pSetList, pBlk); // @recursion
			}
			else {
				if(pSetList) {
					const SColorSet * p_target_set = 0;
					for(uint i = 0; !p_target_set && i < pSetList->getCount(); i++) {
						const SColorSet * p_internal_set = pSetList->at(i);
						if(p_internal_set && p_internal_set->Symb.IsEqiAscii(r_left))
							p_target_set = p_internal_set;
					}
					if(p_target_set) {
						ok = p_target_set->Get(r_right, pSetList, pBlk);
					}
					else
						ok = 0; // unable to resolve symbol of another SColorSet
				}
				else
					ok = 0; // unable to resolve symbol of another SColorSet
			}
		} // } @v11.9.10
		else {
			const InnerEntry * p_entry = L.Get(r_key_buf.cptr(), r_key_buf.Len());
			if(p_entry) {
				if(p_entry->CcbP) {
					THROW(p_entry->CcbP <= CcC.getCount()); // @todo @err
					{
						const ComplexColorBlock * p_inner_blk = CcC.at(p_entry->CcbP-1);
						THROW(p_inner_blk); // @todo @err
						ASSIGN_PTR(pBlk, *p_inner_blk);
					}
				}
				else {
					if(pBlk) {
						pBlk->Z();
						pBlk->C = p_entry->C;
					}
				}
				ok = 1;
			}
		}
	}
	CATCHZOK
	return ok;
}

int SColorSet::Helper_Get(const char * pSymb, const TSCollection <SColorSet> * pSetList, SColor & rC, StringSet * pRecurSymbList) const
{
	int    ok = 1;
	ComplexColorBlock blk;
	SColor c(ZEROCOLOR);
	if(pRecurSymbList) {
		THROW(!pRecurSymbList->search(pSymb, 0, 1)); // @todo @err
		pRecurSymbList->add(pSymb);
	}
	ok = Get(pSymb, pSetList, &blk);
	if(ok > 0) {
		StringSet ss_recur;
		StringSet * p_recur_symb_list = NZOR(pRecurSymbList, &ss_recur);
		ok = ResolveComplexColorBlock(blk, pSetList, c, *p_recur_symb_list);
	}
	CATCHZOK
	rC = c;
	return ok;
}

bool FASTCALL SColorSet::IsInnerEntryEq(const InnerEntry & rE1, const SColorSet & rS, const InnerEntry & rE2) const
{
	bool   eq = true;
	SString & r_buf1 = SLS.AcquireRvlStr();
	SString & r_buf2 = SLS.AcquireRvlStr();
	GetS(rE1.SymbP, r_buf1);
	rS.GetS(rE2.SymbP, r_buf2);
	if(!r_buf1.IsEqiAscii(r_buf2))
		eq = false;
	else if(rE1.C != rE2.C)
		eq = false;
	else {
		const ComplexColorBlock * p_ccb1 = (rE1.CcbP > 0 && rE1.CcbP <= CcC.getCount()) ? CcC.at(rE1.CcbP-1) : 0;
		const ComplexColorBlock * p_ccb2 = (rE2.CcbP > 0 && rE2.CcbP <= rS.CcC.getCount()) ? rS.CcC.at(rE2.CcbP-1) : 0;
		if(p_ccb1 && p_ccb2) {
			if(!p_ccb1->IsEq(*p_ccb2))
				eq = false;
		}
		else if(LOGIC(p_ccb1) != LOGIC(p_ccb2))
			eq = false;
	}
	return eq;
}
	
int SColorSet::Get(const char * pSymb, const TSCollection <SColorSet> * pSetList, SColor & rC) const
{
	return Helper_Get(pSymb, pSetList, rC, 0);
}
//
//
//
UiValueList::ValueUnion::ValueUnion()
{
	THISZERO();
}

bool FASTCALL UiValueList::ValueUnion::IsEq(const ValueUnion & rS) const
{
	return (I == rS.I && R == rS.R && sstreq(T, rS.T));
}

UiValueList::Entry::Entry() : Id(0)
{
}

bool FASTCALL UiValueList::Entry::IsEq(const Entry & rS) const
{
	return (Id == rS.Id && V.IsEq(rS.V));
}

UiValueList::UiValueList()
{
}
	
UiValueList::~UiValueList()
{
}

UiValueList & UiValueList::Z()
{
	L.clear();
	return *this;
}

bool FASTCALL UiValueList::IsEq(const UiValueList & rS) const
{
	bool   eq = true;
	if(L.getCount() == rS.L.getCount()) {
		uint max_id1 = 0;
		uint max_id2 = 0;
		{
			for(uint i = 0; i < L.getCount(); i++) {
				SETMAX(max_id1, L.at(i).Id);
			}
		}
		{
			for(uint i = 0; i < rS.L.getCount(); i++) {
				SETMAX(max_id2, rS.L.at(i).Id);
			}
		}
		if(max_id1 == max_id2) {
			for(uint id_ = 1; eq && id_ <= max_id1; id_++) {
				Entry entry1;
				Entry entry2;
				bool r1 = Implement_Get(id_, entry1);
				bool r2 = rS.Implement_Get(id_, entry2);
				if(r1 && r2) {
					if(!entry1.IsEq(entry2))
						eq = false;
				}
				else if(r1 != r2)
					eq = false;
			}
		}
		else
			eq = false;
	}
	else
		eq = false;
	return eq;
}

int UiValueList::Implement_Put(const Entry & rEntry)
{
	int    ok = 1;
	if(rEntry.Id) {
		uint   idx = 0;
		if(L.lsearch(&rEntry.Id, &idx, CMPF_LONG)) {
			L.at(idx) = rEntry;
			ok = 2;
		}
		else
			L.insert(&rEntry);
	}
	else
		ok = 0;
	return ok;
}

bool UiValueList::Implement_Get(uint id, Entry & rEntry) const
{
	bool   ok = false;
	uint   idx = 0;
	if(id && L.lsearch(&id, &idx, CMPF_LONG)) {
		rEntry = L.at(idx);
		assert(rEntry.Id == id);
		ok = true;
	}
	return ok;
}
	
int UiValueList::Put(uint id, double v)
{
	Entry entry;
	entry.Id = id;
	entry.V.R = v;
	return Implement_Put(entry);
}
	
int UiValueList::Get(uint id, double & rV) const
{
	int    ok = 0;
	Entry entry;
	if(Implement_Get(id, entry)) {
		rV = entry.V.R;
		ok = 1;
	}
	else
		rV = 0.0;
	return ok;
}
	
int UiValueList::Put(uint id, int v)
{
	Entry entry;
	entry.Id = id;
	entry.V.I = v;
	return Implement_Put(entry);
}
	
int UiValueList::Get(uint id, int & rV) const
{
	int    ok = 0;
	Entry entry;
	if(Implement_Get(id, entry)) {
		rV = entry.V.I;
		ok = 1;
	}
	else
		rV = 0;
	return ok;
}
	
int UiValueList::Put(uint id, const char * pV)
{
	Entry entry;
	entry.Id = id;
	STRNSCPY(entry.V.T, pV);
	return Implement_Put(entry);
}
	
int UiValueList::Get(uint id, SString & rV) const
{
	int    ok = 0;
	Entry entry;
	if(Implement_Get(id, entry)) {
		rV = entry.V.T;
		ok = 1;
	}
	else
		rV.Z();
	return ok;
}

struct UiValueDescr {
	uint   Id;
	const char * P_Symb;
	TYPEID Type; // T_INT32 || T_DOUBLE || MKSTYPE(S_ZSTRING, 0)
};

static const UiValueDescr UiValueDescrList[] = {
	{ UiValueList::vStandaloneListWidth, "standalone_list_width", T_INT32 },
	{ UiValueList::vStandaloneListHeight, "standalone_list_height", T_INT32 },
	{ UiValueList::vDesktopIconSize, "desktop_icon_size", T_INT32 },
	{ UiValueList::vDesktopIconGap, "desktop_icon_gap", T_INT32 },
	{ UiValueList::vButtonStdHeight, "button_std_height", T_INT32 }, 
	{ UiValueList::vButtonStdWidth, "button_std_width", T_INT32 },
	{ UiValueList::vButtonDoubleWidth, "button_double_width", T_INT32 },
};
	
SJson * UiValueList::ToJsonObj() const
{
	SJson * p_result = SJson::CreateObj();
	SString temp_buf;
	for(uint i = 0; i < L.getCount(); i++) {
		const Entry & r_entry = L.at(i);
		for(uint j = 0; j < SIZEOFARRAY(UiValueDescrList); j++) {
			if(UiValueDescrList[j].Id == r_entry.Id) {
				switch(GETSTYPE(UiValueDescrList[j].Type)) {
					case S_INT: p_result->InsertInt(UiValueDescrList[j].P_Symb, r_entry.V.I); break;
					case S_FLOAT: p_result->InsertDouble(UiValueDescrList[j].P_Symb, r_entry.V.R, MKSFMTD(0, 6, NMBF_NOTRAILZ|NMBF_OMITEPS)); break;
					case S_ZSTRING: p_result->InsertString(UiValueDescrList[j].P_Symb, r_entry.V.T); break;
				}
				break;
			}
		}
	}
	return p_result;
}
	
int UiValueList::FromJsonObj(const SJson * pJsObj)
{
	int    ok = 1;
	if(SJson::IsObject(pJsObj)) {
		for(const SJson * p_jsn = pJsObj->P_Child; p_jsn; p_jsn = p_jsn->P_Next) {
			if(p_jsn->P_Child) {
				for(uint j = 0; j < SIZEOFARRAY(UiValueDescrList); j++) {
					if(p_jsn->Text.IsEqiAscii(UiValueDescrList[j].P_Symb)) {
						switch(GETSTYPE(UiValueDescrList[j].Type)) {
							case S_INT: 
								Put(UiValueDescrList[j].Id, p_jsn->P_Child->Text.ToLong());
								break;
							case S_FLOAT: 
								Put(UiValueDescrList[j].Id, p_jsn->P_Child->Text.ToReal());
								break;
							case S_ZSTRING: 
								Put(UiValueDescrList[j].Id, p_jsn->P_Child->Text);
								break;
						}
						break;
					}
				}
			}
		}
	}
	return ok;
}

UiDescription::UiDescription()
{
}

UiDescription::~UiDescription()
{
}

UiDescription & UiDescription::Z()
{
	FontList.clear();
	ClrList.clear();
	LoList.clear();
	SourceFileName.Z(); // @v11.9.7
	return *this;
}

bool FASTCALL UiDescription::IsEq(const UiDescription & rS) const
{
	// Поле SourceFileName не принимает участие в сравнении эквивалентности!
	bool eq = true;
	if(!TSCollection_IsEq(&FontList, &rS.FontList))
		eq = false;
	if(!TSCollection_IsEq(&FontDescrList, &rS.FontDescrList))
		eq = false;
	else if(!TSCollection_IsEq(&ClrList, &rS.ClrList))	
		eq = false;
	else if(!TSCollection_IsEq(&LoList, &rS.LoList))	
		eq = false;
	return eq;
}

UiDescription & UiDescription::Copy(const UiDescription & rS)
{
	TSCollection_Copy(FontList, rS.FontList);
	TSCollection_Copy(FontDescrList, rS.FontDescrList); // @v11.9.10
	TSCollection_Copy(ClrList, rS.ClrList);
	TSCollection_Copy(LoList, rS.LoList);
	SourceFileName = rS.SourceFileName; // @v11.9.7
	return *this;
}

void  UiDescription::SetSourceFileName(const char * pFileName) { SourceFileName = pFileName; }
const char * UiDescription::GetSourceFileName() const { return SourceFileName; }

SColorSet * UiDescription::GetColorSet(const char * pCsSymb)
{
	return const_cast<SColorSet *>(GetColorSetC(pCsSymb));
}

const SColorSet * UiDescription::GetColorSetC(const char * pCsSymb) const
{
	const SColorSet * p_result = 0;
	for(uint i = 0; !p_result && i < ClrList.getCount(); i++) {
		const SColorSet * p_cset = ClrList.at(i);
		if(p_cset && p_cset->GetSymb().IsEqiAscii(pCsSymb))
			p_result = p_cset;
	}
	return p_result;
}

int UiDescription::GetColor(const SColorSet * pColorSet, const char * pColorSymb, SColor & rC) const
{
	return pColorSet ? pColorSet->Get(pColorSymb, &ClrList, rC) : 0;
}

SColor UiDescription::GetColorR(const SColorSet * pColorSet, const char * pColorSymb, const SColor defaultC) const
{
	SColor result;
	if(!GetColor(pColorSet, pColorSymb, result))
		result = defaultC;
	return result;
}

/*static*/SColor UiDescription::GetColorR(const UiDescription * pUid, const SColorSet * pColorSet, const char * pColorSymb, const SColor defaultC)
{
	SColor result;
	if(!pUid || !pUid->GetColor(pColorSet, pColorSymb, result))
		result = defaultC;
	return result;
}

int UiDescription::GetColor(const char * pColorSetSymb, const char * pColorSymb, SColor & rC) const
{
	return GetColor(GetColorSetC(pColorSetSymb), pColorSymb, rC);
}

bool UiDescription::ValidateColorSetList()
{
	bool   ok = true;
	for(uint i = 0; i < ClrList.getCount(); i++) {
		const SColorSet * p_set = ClrList.at(i);
		if(p_set) {
			if(!p_set->ValidateRefs(&ClrList))
				ok = false;
		}
	}
	return ok;
}

const SFontSource * UiDescription::GetFontSourceC(const char * pSymb) const
{
	const SFontSource * p_result = 0;
	if(!isempty(pSymb)) {
		for(uint i = 0; !p_result && i < FontList.getCount(); i++) {
			const SFontSource * p_fs = FontList.at(i);
			if(p_fs && p_fs->Face.IsEqiAscii(pSymb))
				p_result = p_fs;
		}
	}
	return p_result;
}

SJson * UiDescription::ToJsonObj() const
{
	SJson * p_result = SJson::CreateObj();
	SString temp_buf;
	p_result->InsertString("symb", "ui");
	if(VList.GetCount()) { // @v11.9.4
		SJson * p_js_valuelist = VList.ToJsonObj();
		if(p_js_valuelist) {
			p_result->Insert("value_list", p_js_valuelist);
		}
	}
	if(FontList.getCount()) {
		SJson * p_js_fontsrc_list = SJson::CreateArr();
		for(uint i = 0; i < FontList.getCount(); i++) {
			const SFontSource * p_item = FontList.at(i);
			if(p_item) {
				SJson * p_js_item = p_item->ToJsonObj();
				THROW(p_js_item);
				p_js_fontsrc_list->InsertChild(p_js_item);
				p_js_item = 0;
			}
		}
		p_result->Insert("fontsrc_list", p_js_fontsrc_list);
		p_js_fontsrc_list = 0;
	}
	if(ClrList.getCount()) {
		bool is_there_anonym = false;
		SJson * p_js_colorset_list = SJson::CreateArr();
		for(uint i = 0; i < ClrList.getCount(); i++) {
			const SColorSet * p_item = ClrList.at(i);
			if(p_item) {
				const SString & r_symb = p_item->GetSymb();
				if(r_symb.IsEmpty()) {
					THROW(!is_there_anonym); // @todo @err
					temp_buf = "default";
					is_there_anonym = true;
				}
				else
					temp_buf = r_symb;
				SJson * p_js_item = p_item->ToJsonObj();
				THROW(p_js_item);
				p_js_colorset_list->InsertChild(p_js_item);
				p_js_item = 0;
			}
		}
		p_result->Insert("colorset_list", p_js_colorset_list);
		p_js_colorset_list = 0;
	}
	if(LoList.getCount()) {
		SJson * p_js_lo_list = SJson::CreateArr();
		for(uint i = 0; i < LoList.getCount(); i++) {
			const SUiLayout * p_item = LoList.at(i);
			if(p_item) {
				SJson * p_js_item = p_item->ToJsonObj();
				THROW(p_js_item);
				p_js_lo_list->InsertChild(p_js_item);
				p_js_item = 0;
			}
		}
		p_result->Insert("layout_list", p_js_lo_list);
		p_js_lo_list = 0;
	}
	CATCH
		ZDELETE(p_result);
	ENDCATCH
	return p_result;
}

int UiDescription::FromJsonObj(const SJson * pJsObj)
{
	int    ok = 1;
	THROW(SJson::IsObject(pJsObj)); // @todo @err
	Z();
	for(const SJson * p_jsn = pJsObj->P_Child; p_jsn; p_jsn = p_jsn->P_Next) {
		if(p_jsn->P_Child) {
			if(p_jsn->Text.IsEqiAscii("symb")) {
			}
			else if(p_jsn->Text.IsEqiAscii("fontsrc_list")) {
				if(SJson::IsArray(p_jsn->P_Child)) {
					for(const SJson * p_js_inner = p_jsn->P_Child->P_Child; p_js_inner; p_js_inner = p_js_inner->P_Next) {
						SFontSource * p_item = FontList.CreateNewItem();
						THROW(p_item);								
						THROW(p_item->FromJsonObj(p_js_inner));
					}
				}
			}
			else if(p_jsn->Text.IsEqiAscii("colorset_list")) {
				if(SJson::IsArray(p_jsn->P_Child)) {
					for(const SJson * p_js_inner = p_jsn->P_Child->P_Child; p_js_inner; p_js_inner = p_js_inner->P_Next) {
						SColorSet * p_item = ClrList.CreateNewItem();
						THROW(p_item);								
						THROW(p_item->FromJsonObj(p_js_inner));
					}
					THROW(ValidateColorSetList());
				}
			}
			else if(p_jsn->Text.IsEqiAscii("layout_list")) {
				if(SJson::IsArray(p_jsn->P_Child)) {
					for(const SJson * p_js_inner = p_jsn->P_Child->P_Child; p_js_inner; p_js_inner = p_js_inner->P_Next) {
						SUiLayout * p_item = LoList.CreateNewItem();
						THROW(p_item);								
						THROW(p_item->FromJsonObj(p_js_inner));
					}
				}
			}
			else if(p_jsn->Text.IsEqiAscii("value_list")) { // @v11.9.4
				VList.FromJsonObj(p_jsn->P_Child);
			}
		}
	}
	CATCHZOK
	return ok;
}

// } } @v11.7.10 @construction