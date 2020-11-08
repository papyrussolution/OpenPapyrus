// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See the LICENSE.txt file in the project root for the license information.
//
#include <slib.h>
#include <tv.h>
#pragma hdrstop
#include <layout-flex.h>

LayoutFlexItem::LayoutFlexItem() : P_Parent(0), /*should_order_children(false),*/ managed_ptr(0), CbSelfSizing(0), CbSetup(0),
	Size(fgetnanf(), fgetnanf())/*width(fgetnanf()), height(fgetnanf())*/, N(fgetnanf(), fgetnanf(), fgetnanf(), fgetnanf()),
	//padding_left(0.0f), padding_right(0.0f), padding_top(0.0f), padding_bottom(0.0f),
	//margin_left(0.0f), margin_right(0.0f), margin_top(0.0f), margin_bottom(0.0f),
	JustifyContent(FLEX_ALIGN_START), AlignContent(FLEX_ALIGN_STRETCH), AlignItems(FLEX_ALIGN_STRETCH),
	AlignSelf(FLEX_ALIGN_AUTO), PositionMode(FLEX_POSITION_RELATIVE), Direction(FLEX_DIRECTION_COLUMN),
	WrapMode(FLEX_WRAP_NO_WRAP), grow(0.0f), shrink(1.0f), order(0), basis(fgetnanf()),
	Flags(0)
{
	memzero(frame, sizeof(frame));
}

LayoutFlexItem::~LayoutFlexItem()
{
}

int LayoutFlexItem::GetOrder() const
{
	return order;
}

void LayoutFlexItem::SetOrder(int o)
{
	order = o;
	UpdateShouldOrderShildren();
}

void LayoutFlexItem::UpdateShouldOrderShildren()
{
	if(order != 0 && P_Parent) {
		//P_Parent->should_order_children = true;
		P_Parent->Flags |= fStateShouldOrderChildren;
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
	if(fisnanf(Size.X))
		ok = 0;
	else {
		s += Size.X;
		if(!fisnan(Margin.a.X))
			s += Margin.a.X;
		if(!fisnan(Margin.b.X))
			s += Margin.b.X;
		if(!fisnan(Padding.a.X))
			s += Padding.a.X;
		if(!fisnan(Padding.b.X))
			s += Padding.b.X;
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
	if(fisnanf(Size.Y))
		ok = 0;
	else {
		s += Size.Y;
		if(!fisnan(Margin.a.Y))
			s += Margin.a.Y;
		if(!fisnan(Margin.b.Y))
			s += Margin.b.Y;
		if(!fisnan(Padding.a.Y))
			s += Padding.a.Y;
		if(!fisnan(Padding.b.Y))
			s += Padding.b.Y;
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

static void child_set(LayoutFlexItem * item, LayoutFlexItem * pChild)
{
	assert(pChild->P_Parent == NULL && "child already has a parent");
	pChild->P_Parent = item;
	item->insert(pChild);
	pChild->UpdateShouldOrderShildren();
}

void flex_item_add(LayoutFlexItem * pItem, LayoutFlexItem * pChild)
{
	assert(pChild->P_Parent == NULL && "child already has a parent");
	pChild->P_Parent = pItem;
	pItem->insert(pChild);
	pChild->UpdateShouldOrderShildren();
}

void flex_item_delete(LayoutFlexItem * pItem, uint index)
{
	assert(index < pItem->getCount());
	assert(pItem->getCount() > 0);
	pItem->atFree(index);
	LayoutFlexItem * p_child = pItem->at(index);
	if(p_child)
		p_child->P_Parent = 0;
	pItem->atFree(index);
}

/*LayoutFlexItem * flex_item_child(LayoutFlexItem * item, uint index)                    
{
	assert(index < item->getCount());
	return item->at(index);
}*/
//LayoutFlexItem * flex_item_parent(LayoutFlexItem * item) { return item->P_Parent; }

struct LayoutFlex {
	LayoutFlex() : wrap(false), reverse(false), reverse2(false), vertical(false), need_lines(false),
		size_dim(0.0f), align_dim(0.0f), frame_pos_i(0), frame_pos2_i(0), frame_size_i(0), frame_size2_i(0),
		ordered_indices(0), line_dim(0.0f), flex_dim(0.0f), extra_flex_dim(0.0f), flex_grows(0.0f), flex_shrinks(0.0f),
		pos2(0.0f), P_Lines(0), lines_count(0), lines_sizes(0.0f)
	{
	}
	void Init(const LayoutFlexItem * pItem, float _width, float _height)
	{
		assert(pItem->Padding.a.X >= 0.0f);
		assert(pItem->Padding.b.X >= 0.0f);
		assert(pItem->Padding.a.Y >= 0.0f);
		assert(pItem->Padding.b.Y >= 0.0f);
		_width  -= (pItem->Padding.a.X + pItem->Padding.b.X);
		_height -= (pItem->Padding.a.Y + pItem->Padding.b.Y);
		assert(_width >= 0.0f);
		assert(_height >= 0.0f);
		reverse = false;
		vertical = true;
		switch(pItem->Direction) {
			case FLEX_DIRECTION_ROW_REVERSE:
				reverse = true;
			case FLEX_DIRECTION_ROW:
				vertical = false;
				size_dim = _width;
				align_dim = _height;
				frame_pos_i = 0;
				frame_pos2_i = 1;
				frame_size_i = 2;
				frame_size2_i = 3;
				break;
			case FLEX_DIRECTION_COLUMN_REVERSE:
				reverse = true;
			case FLEX_DIRECTION_COLUMN:
				size_dim = _height;
				align_dim = _width;
				frame_pos_i = 1;
				frame_pos2_i = 0;
				frame_size_i = 3;
				frame_size2_i = 2;
				break;
			default:
				assert(false && "incorrect direction");
		}
		ordered_indices = NULL;
		if(/*item->should_order_children*/(pItem->Flags & LayoutFlexItem::fStateShouldOrderChildren) && pItem->getCount()) {
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
		flex_dim = 0;
		flex_grows = 0;
		flex_shrinks = 0;
		reverse2 = false;
		wrap = (pItem->WrapMode != FLEX_WRAP_NO_WRAP);
		if(wrap) {
			if(pItem->WrapMode == FLEX_WRAP_WRAP_REVERSE) {
				reverse2 = true;
				pos2 = align_dim;
			}
		}
		else {
			pos2 = vertical ? pItem->Padding.a.X : pItem->Padding.a.Y;
		}
		need_lines = wrap && pItem->AlignContent != FLEX_ALIGN_START;
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
	LayoutFlexItem * GetChildByIndex(LayoutFlexItem * pContainer, uint idx) const
	{
		return (pContainer->at(ordered_indices ? ordered_indices[idx] : idx));
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

#if 0 // {
static void layout_init(LayoutFlexItem * item, float width, float height, LayoutFlex * layout)
{
	assert(item->Padding.a.X >= 0.0f);
	assert(item->Padding.b.X >= 0.0f);
	assert(item->Padding.a.Y >= 0.0f);
	assert(item->Padding.b.Y >= 0.0f);
	width  -= (item->Padding.a.X + item->Padding.b.X);
	height -= (item->Padding.a.Y + item->Padding.b.Y);
	assert(width >= 0.0f);
	assert(height >= 0.0f);
	layout->reverse = false;
	layout->vertical = true;
	switch(item->Direction) {
		case FLEX_DIRECTION_ROW_REVERSE:
		    layout->reverse = true;
		case FLEX_DIRECTION_ROW:
		    layout->vertical = false;
		    layout->size_dim = width;
		    layout->align_dim = height;
		    layout->frame_pos_i = 0;
		    layout->frame_pos2_i = 1;
		    layout->frame_size_i = 2;
		    layout->frame_size2_i = 3;
		    break;
		case FLEX_DIRECTION_COLUMN_REVERSE:
		    layout->reverse = true;
		case FLEX_DIRECTION_COLUMN:
		    layout->size_dim = height;
		    layout->align_dim = width;
		    layout->frame_pos_i = 1;
		    layout->frame_pos2_i = 0;
		    layout->frame_size_i = 3;
		    layout->frame_size2_i = 2;
		    break;
		default:
		    assert(false && "incorrect direction");
	}
	layout->ordered_indices = NULL;
	if(/*item->should_order_children*/(item->Flags & LayoutFlexItem::fStateShouldOrderChildren) && item->getCount() > 0) {
		uint * p_indices = static_cast<uint *>(SAlloc::M(sizeof(uint) * item->getCount()));
		assert(p_indices != NULL);
		// Creating a list of item indices sorted using the children's `order'
		// attribute values. We are using a simple insertion sort as we need
		// stability (insertion order must be preserved) and cross-platform
		// support. We should eventually switch to merge sort (or something
		// else) if the number of items becomes significant enough.
		for(uint i = 0; i < item->getCount(); i++) {
			p_indices[i] = i;
			for(uint j = i; j > 0; j--) {
				const uint prev = p_indices[j-1];
				const uint curr = p_indices[j];
				if(item->at(prev)->GetOrder() <= item->at(curr)->GetOrder()) {
					break;
				}
				p_indices[j - 1] = curr;
				p_indices[j] = prev;
			}
		}
		layout->ordered_indices = p_indices;
	}
	layout->flex_dim = 0;
	layout->flex_grows = 0;
	layout->flex_shrinks = 0;
	layout->reverse2 = false;
	layout->wrap = (item->WrapMode != FLEX_WRAP_NO_WRAP);
	if(layout->wrap) {
		if(item->WrapMode == FLEX_WRAP_WRAP_REVERSE) {
			layout->reverse2 = true;
			layout->pos2 = layout->align_dim;
		}
	}
	else {
		layout->pos2 = layout->vertical ? item->Padding.a.X : item->Padding.a.Y;
	}
	layout->need_lines = layout->wrap && item->AlignContent != FLEX_ALIGN_START;
	layout->P_Lines = NULL;
	layout->lines_count = 0;
	layout->lines_sizes = 0;
}
#endif // } 0

//#define LAYOUT_CHILD_AT_(ptrLayout, item, i) (item->at((ptrLayout->ordered_indices ? ptrLayout->ordered_indices[i] : i)))
#define _LAYOUT_FRAME_(ptrLayout, child, name) child->frame[ptrLayout->frame_ ## name ## _i]
#define CHILD_POS_(ptrLayout, child)   _LAYOUT_FRAME_(ptrLayout, child, pos)
#define CHILD_POS2_(ptrLayout, child)  _LAYOUT_FRAME_(ptrLayout, child, pos2)
#define CHILD_SIZE_(ptrLayout, child)  _LAYOUT_FRAME_(ptrLayout, child, size)
#define CHILD_SIZE2_(ptrLayout, child) _LAYOUT_FRAME_(ptrLayout, child, size2)
#define CHILD_MARGIN_XY_(ptrLayout, child, pnt) (ptrLayout->vertical ? child->Margin.pnt.X : child->Margin.pnt.Y)
#define CHILD_MARGIN_YX_(ptrLayout, child, pnt) (ptrLayout->vertical ? child->Margin.pnt.Y : child->Margin.pnt.X)

static bool layout_align(flex_align align, float flex_dim, uint children_count, float * pos_p, float * spacing_p, bool stretch_allowed)
{
	assert(flex_dim > 0);
	float pos = 0.0f;
	float spacing = 0.0f;
	switch(align) {
		case FLEX_ALIGN_START:
		    break;
		case FLEX_ALIGN_END:
		    pos = flex_dim;
		    break;
		case FLEX_ALIGN_CENTER:
		    pos = flex_dim / 2;
		    break;
		case FLEX_ALIGN_SPACE_BETWEEN:
		    if(children_count > 0) {
			    spacing = flex_dim / (children_count - 1);
		    }
		    break;
		case FLEX_ALIGN_SPACE_AROUND:
		    if(children_count > 0) {
			    spacing = flex_dim / children_count;
			    pos = spacing / 2;
		    }
		    break;
		case FLEX_ALIGN_SPACE_EVENLY:
		    if(children_count > 0) {
			    spacing = flex_dim / (children_count + 1);
			    pos = spacing;
		    }
		    break;
		case FLEX_ALIGN_STRETCH:
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

flex_align FASTCALL LayoutFlexItem::GetChildAlign(const LayoutFlexItem * pChild) const
{
	return (pChild->AlignSelf == FLEX_ALIGN_AUTO) ? AlignItems : pChild->AlignSelf;
}

#if 0 // {
static flex_align FASTCALL child_align(LayoutFlexItem * child, LayoutFlexItem * parent)
{
	return (child->AlignSelf == FLEX_ALIGN_AUTO) ? parent->AlignItems : child->AlignSelf;
	/*flex_align align = child->align_self;
	if(align == FLEX_ALIGN_AUTO)
		align = parent->align_items;
	return align;*/
}
#endif // } 0

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
			assert(layout_align(JustifyContent, p_layout->flex_dim, childrenCount, &pos, &spacing, false) && "incorrect justify_content");
			if(p_layout->reverse)
				pos = p_layout->size_dim - pos;
		}
		if(p_layout->reverse)
			pos -= p_layout->vertical ? Padding.b.Y : Padding.b.X;
		else 
			pos += p_layout->vertical ? Padding.a.Y : Padding.a.X;
		if(p_layout->wrap && p_layout->reverse2) {
			p_layout->pos2 -= p_layout->line_dim;
		}
		for(uint i = childBeginIdx; i < childEndIdx; i++) {
			//LayoutFlexItem * child = LAYOUT_CHILD_AT_(p_layout, this, i);
			LayoutFlexItem * child = p_layout->GetChildByIndex(this, i);
			if(child->PositionMode == FLEX_POSITION_ABSOLUTE) {
				continue; // Already positioned.
			}
			// Grow or shrink the main axis item size if needed.
			float flex_size = 0.0f;
			if(p_layout->flex_dim > 0.0f) {
				if(child->grow != 0.0f) {
					CHILD_SIZE_(p_layout, child) = 0; // Ignore previous size when growing.
					flex_size = (p_layout->flex_dim / p_layout->flex_grows) * child->grow;
				}
			}
			else if(p_layout->flex_dim < 0.0f) {
				if(child->shrink != 0.0f) {
					flex_size = (p_layout->flex_dim / p_layout->flex_shrinks) * child->shrink;
				}
			}
			CHILD_SIZE_(p_layout, child) += flex_size;
			// Set the cross axis position (and stretch the cross axis size if needed).
			float align_size = CHILD_SIZE2_(p_layout, child);
			float align_pos = p_layout->pos2 + 0.0f;
			switch(GetChildAlign(child)) {
				case FLEX_ALIGN_END:
					align_pos += (p_layout->line_dim - align_size - CHILD_MARGIN_XY_(p_layout, child, b));
					break;
				case FLEX_ALIGN_CENTER:
					align_pos += (p_layout->line_dim / 2.0f) - (align_size / 2.0f) + (CHILD_MARGIN_XY_(p_layout, child, a) - CHILD_MARGIN_XY_(p_layout, child, b));
					break;
				case FLEX_ALIGN_STRETCH:
					if(align_size == 0) {
						CHILD_SIZE2_(p_layout, child) = p_layout->line_dim - (CHILD_MARGIN_XY_(p_layout, child, a) + CHILD_MARGIN_XY_(p_layout, child, b));
					}
				// fall through
				case FLEX_ALIGN_START:
					align_pos += CHILD_MARGIN_XY_(p_layout, child, a);
					break;
				default:
					assert(false && "incorrect align_self");
			}
			CHILD_POS2_(p_layout, child) = align_pos;
			// Set the main axis position.
			if(p_layout->reverse) {
				pos -= CHILD_MARGIN_YX_(p_layout, child, b);
				pos -= CHILD_SIZE_(p_layout, child);
				CHILD_POS_(p_layout, child) = pos;
				pos -= spacing;
				pos -= CHILD_MARGIN_YX_(p_layout, child, a);
			}
			else {
				pos += CHILD_MARGIN_YX_(p_layout, child, a);
				CHILD_POS_(p_layout, child) = pos;
				pos += CHILD_SIZE_(p_layout, child);
				pos += spacing;
				pos += CHILD_MARGIN_YX_(p_layout, child, b);
			}
			// Now that the item has a frame, we can layout its children.
			child->DoLayout(child->frame[2], child->frame[3]);
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
	if(getCount()) {
		LayoutFlex layout_s;
		//LayoutFlex * p_layout = &layout_s;
		layout_s.Init(this, _width, _height);
		layout_s.Z(); //LAYOUT_RESET();
		uint last_layout_child = 0;
		uint relative_children_count = 0;
		for(uint i = 0; i < getCount(); i++) {
			//LayoutFlexItem * child = LAYOUT_CHILD_AT_((&layout_s), this, i);
			LayoutFlexItem * child = layout_s.GetChildByIndex(this, i);
			// Items with an absolute position have their frames determined
			// directly and are skipped during layout.
			if(child->PositionMode == FLEX_POSITION_ABSOLUTE) {
	#define ABSOLUTE_SIZE(val, pos1, pos2, dim) (!fisnanf(val)  ? val : (!fisnanf(pos1) && !fisnanf(pos2) ? (dim - pos2 - pos1) : 0))
	#define ABSOLUTE_POS(pos1, pos2, size, dim) (!fisnanf(pos1) ? pos1 : (!fisnanf(pos2) ? (dim - size - pos2) : 0))
				float child_width = ABSOLUTE_SIZE(child->Size.X, child->N.a.X, child->N.b.X, _width);
				float child_height = ABSOLUTE_SIZE(child->Size.Y, child->N.a.Y, child->N.b.Y, _height);
				float child_x = ABSOLUTE_POS(child->N.a.X, child->N.b.X, child_width, _width);
				float child_y = ABSOLUTE_POS(child->N.a.Y, child->N.b.Y, child_height, _height);
				child->frame[0] = child_x;
				child->frame[1] = child_y;
				child->frame[2] = child_width;
				child->frame[3] = child_height;
				// Now that the item has a frame, we can layout its children.
				child->DoLayout(child->frame[2], child->frame[3]); // @recursion
	#undef ABSOLUTE_POS
	#undef ABSOLUTE_SIZE
				continue;
			}
			// Initialize frame.
			child->frame[0] = 0.0f;
			child->frame[1] = 0.0f;
			child->frame[2] = child->Size.X;
			child->frame[3] = child->Size.Y;
			// Main axis size defaults to 0.
			if(fisnanf(CHILD_SIZE_((&layout_s), child))) {
				CHILD_SIZE_((&layout_s), child) = 0;
			}
			// Cross axis size defaults to the parent's size (or line size in wrap
			// mode, which is calculated later on).
			if(fisnanf(CHILD_SIZE2_((&layout_s), child))) {
				if(layout_s.wrap)
					layout_s.need_lines = true;
				else {
					CHILD_SIZE2_((&layout_s), child) = (layout_s.vertical ? _width : _height) - CHILD_MARGIN_XY_((&layout_s), child, a) - CHILD_MARGIN_XY_((&layout_s), child, b);
				}
			}
			// Call the self_sizing callback if provided. Only non-NAN values
			// are taken into account. If the item's cross-axis align property
			// is set to stretch, ignore the value returned by the callback.
			if(child->CbSelfSizing) {
				float size[2] = { child->frame[2], child->frame[3] };
				child->CbSelfSizing(child, size);
				for(uint j = 0; j < 2; j++) {
					const uint size_off = j + 2;
					if(size_off == layout_s.frame_size2_i && GetChildAlign(child) == FLEX_ALIGN_STRETCH) {
						continue;
					}
					float val = size[j];
					if(!fisnanf(val))
						child->frame[size_off] = val;
				}
			}
			// Honor the `basis' property which overrides the main-axis size.
			if(!fisnanf(child->basis)) {
				assert(child->basis >= 0.0f);
				CHILD_SIZE_((&layout_s), child) = child->basis;
			}
			float child_size = CHILD_SIZE_((&layout_s), child);
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
				float child_size2 = CHILD_SIZE2_((&layout_s), child);
				if(!fisnanf(child_size2) && child_size2 > layout_s.line_dim) {
					layout_s.line_dim = child_size2;
				}
			}
			assert(child->grow >= 0.0f);
			assert(child->shrink >= 0.0f);
			layout_s.flex_grows   += child->grow;
			layout_s.flex_shrinks += child->shrink;
			layout_s.flex_dim     -= (child_size + (CHILD_MARGIN_YX_((&layout_s), child, a) + CHILD_MARGIN_YX_((&layout_s), child, b)));
			relative_children_count++;
			if(child_size > 0 && child->grow > 0.0f)
				layout_s.extra_flex_dim += child_size;
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
				assert(layout_align(AlignContent, flex_dim, layout_s.lines_count, &pos, &spacing, true) && "incorrect align_content");
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
					LayoutFlexItem * child = layout_s.GetChildByIndex(this, j);
					if(child->PositionMode == FLEX_POSITION_ABSOLUTE) {
						continue; // Should not be re-positioned.
					}
					if(fisnanf(CHILD_SIZE2_((&layout_s), child))) {
						// If the child's cross axis size hasn't been set it, it defaults to the line size.
						CHILD_SIZE2_((&layout_s), child) = p_line->size + (AlignContent == FLEX_ALIGN_STRETCH ? spacing : 0);
					}
					CHILD_POS2_((&layout_s), child) = pos + (CHILD_POS2_((&layout_s), child) - old_pos);
				}
				if(!layout_s.reverse2) {
					pos += p_line->size;
					pos += spacing;
					old_pos += p_line->size;
				}
			}
		}
		//layout_cleanup(layout);
		//static void layout_cleanup(LayoutFlex * pLayout)
		{
			ZFREE(layout_s.ordered_indices);
			ZFREE(layout_s.P_Lines);
			layout_s.lines_count = 0;
		}
	}
}

static void FASTCALL SetupItem(LayoutFlexItem * pItem)
{
	if(pItem) {
		if(pItem->CbSetup) {
			pItem->CbSetup(pItem, pItem->frame);
		}
		for(uint i = 0; i < pItem->getCount(); i++) {
			SetupItem(pItem->at(i)); // @recursion
		}
	}
}

int DoLayoutFlex(LayoutFlexItem * pItem)
{
	int    ok = -1;
	if(pItem && pItem->getCount()) {
		if(!pItem->P_Parent && !fisnan(pItem->Size.X) && !fisnan(pItem->Size.Y) && !pItem->CbSelfSizing) {
			assert(pItem->P_Parent == NULL);
			assert(!fisnan(pItem->Size.X));
			assert(!fisnan(pItem->Size.Y));
			assert(pItem->CbSelfSizing == NULL);
			pItem->DoLayout(pItem->Size.X, pItem->Size.Y);
			//SetupItem(pItem);
			{
				for(uint i = 0; i < pItem->getCount(); i++) {
					SetupItem(pItem->at(i));
				}
			}
			ok = 1;
		}
	}
	return ok;
}
