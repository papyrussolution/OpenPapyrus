// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See the LICENSE.txt file in the project root for the license information.
//
#include <slib.h>
#include <tv.h>
#pragma hdrstop
#include <layout-flex.h>

#define ENUM_FLEX_ATTRIBUTES \
FLEX_ATTRIBUTE(width, float, NAN) \
FLEX_ATTRIBUTE(height, float, NAN)\
FLEX_ATTRIBUTE(left, float, NAN)\
FLEX_ATTRIBUTE(right, float, NAN)\
FLEX_ATTRIBUTE(top, float, NAN)\
FLEX_ATTRIBUTE(bottom, float, NAN)\
FLEX_ATTRIBUTE(padding_left, float, 0)\
FLEX_ATTRIBUTE(padding_right, float, 0)\
FLEX_ATTRIBUTE(padding_top, float, 0)\
FLEX_ATTRIBUTE(padding_bottom, float, 0)\
FLEX_ATTRIBUTE(margin_left, float, 0)\
FLEX_ATTRIBUTE(margin_right, float, 0)\
FLEX_ATTRIBUTE(margin_top, float, 0)\
FLEX_ATTRIBUTE(margin_bottom, float, 0)\
FLEX_ATTRIBUTE(justify_content, flex_align, FLEX_ALIGN_START)\
FLEX_ATTRIBUTE(align_content, flex_align, FLEX_ALIGN_STRETCH)\
FLEX_ATTRIBUTE(align_items, flex_align, FLEX_ALIGN_STRETCH)\
FLEX_ATTRIBUTE(align_self, flex_align, FLEX_ALIGN_AUTO)\
FLEX_ATTRIBUTE(position, flex_position, FLEX_POSITION_RELATIVE)\
FLEX_ATTRIBUTE(direction, flex_direction, FLEX_DIRECTION_COLUMN)\
FLEX_ATTRIBUTE(wrap, flex_wrap, FLEX_WRAP_NO_WRAP)\
FLEX_ATTRIBUTE(grow, float, 0.0)\
FLEX_ATTRIBUTE(shrink, float, 1.0)\
FLEX_ATTRIBUTE(order, int, 0)\
FLEX_ATTRIBUTE(basis, float, NAN)\
/* An item can store an arbitrary pointer, which can be used by bindings as the address of a managed object. */\
FLEX_ATTRIBUTE(managed_ptr, void *, NULL)\
/* An item can provide a self_sizing callback function that will be called */\
/* during layout and which can customize the dimensions (width and height) of the item.*/\
FLEX_ATTRIBUTE(self_sizing, flex_self_sizing, NULL)

LayoutFlexItem::LayoutFlexItem() : P_Parent(0), should_order_children(false), managed_ptr(0), CbSelfSizing(0), CbSetup(0),
	width(fgetnanf()), height(fgetnanf()), left(fgetnanf()), right(fgetnanf()), top(fgetnanf()), bottom(fgetnanf()),
	padding_left(0.0f), padding_right(0.0f), padding_top(0.0f), padding_bottom(0.0f),
	margin_left(0.0f), margin_right(0.0f), margin_top(0.0f), margin_bottom(0.0f),
	justify_content(FLEX_ALIGN_START), align_content(FLEX_ALIGN_STRETCH), align_items(FLEX_ALIGN_STRETCH),
	align_self(FLEX_ALIGN_AUTO), position(FLEX_POSITION_RELATIVE), direction(FLEX_DIRECTION_COLUMN),
	wrap(FLEX_WRAP_NO_WRAP), grow(0.0f), shrink(1.0f), order(0), basis(fgetnanf())
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
	if(order != 0 && P_Parent)
		P_Parent->should_order_children = true;
}

/*typedef enum {
	#define FLEX_ATTRIBUTE(name, type, def) FLEX_PROPERTY_ ## name,
	//#include <layout-flex.h>
	ENUM_FLEX_ATTRIBUTES
	#undef FLEX_ATTRIBUTE
} flex_property;

static void item_property_changed(LayoutFlexItem * item, flex_property property)
{
	if(property == FLEX_PROPERTY_order) {
		item->UpdateShouldOrderShildren();
	}
}*/

/*#define FLEX_ATTRIBUTE(name, type, def) \
	type flex_item_get_ ## name(LayoutFlexItem * item) { return item->name; } \
	void flex_item_set_ ## name(LayoutFlexItem * item, type value) { \
		item->name = value; \
		item_property_changed(item, FLEX_PROPERTY_ ## name); \
	}
ENUM_FLEX_ATTRIBUTES
#undef FLEX_ATTRIBUTE*/

LayoutFlexItem * flex_item_new()
{
	//LayoutFlexItem * item = (LayoutFlexItem *)SAlloc::M(sizeof(LayoutFlexItem));
	LayoutFlexItem * p_item = new LayoutFlexItem;
	assert(p_item != NULL);
	//#define FLEX_ATTRIBUTE(name, type, def) p_item->name = def;
		//ENUM_FLEX_ATTRIBUTES
	//#undef FLEX_ATTRIBUTE
	return p_item;
}

//void flex_item_free(LayoutFlexItem * pItem) { delete pItem; }

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

//uint flex_item_count(LayoutFlexItem * item) { return item->getCount(); }

LayoutFlexItem * flex_item_child(LayoutFlexItem * item, uint index)                    
{
	assert(index < item->getCount());
	return item->at(index);
}

LayoutFlexItem * flex_item_parent(LayoutFlexItem * item) { return item->P_Parent; }

/*LayoutFlexItem * flex_item_root(LayoutFlexItem * item)                    
{
	while(item->P_Parent) {
		item = item->P_Parent;
	}
	return item;
}*/

#define FRAME_GETTER(name, index) float flex_item_get_frame_ ## name(LayoutFlexItem * item) { return item->frame[index]; }
	FRAME_GETTER(x, 0)
	FRAME_GETTER(y, 1)
	FRAME_GETTER(width, 2)
	FRAME_GETTER(height, 3)
#undef FRAME_GETTER

struct LayoutFlex {
	LayoutFlex() : wrap(false), reverse(false), reverse2(false), vertical(false), need_lines(false),
		size_dim(0.0f), align_dim(0.0f), frame_pos_i(0), frame_pos2_i(0), frame_size_i(0), frame_size2_i(0),
		ordered_indices(0), line_dim(0.0f), flex_dim(0.0f), extra_flex_dim(0.0f), flex_grows(0.0f), flex_shrinks(0.0f),
		pos2(0.0f), P_Lines(0), lines_count(0), lines_sizes(0.0f)
	{
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

static void layout_init(LayoutFlexItem * item, float width, float height, LayoutFlex * layout)
{
	assert(item->padding_left >= 0.0f);
	assert(item->padding_right >= 0.0f);
	assert(item->padding_top >= 0.0f);
	assert(item->padding_bottom >= 0.0f);
	width -= item->padding_left + item->padding_right;
	height -= item->padding_top + item->padding_bottom;
	assert(width >= 0.0f);
	assert(height >= 0.0f);
	layout->reverse = false;
	layout->vertical = true;
	switch(item->direction) {
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
	if(item->should_order_children && item->getCount() > 0) {
		uint * indices = (uint *)SAlloc::M(sizeof(uint) * item->getCount());
		assert(indices != NULL);
		// Creating a list of item indices sorted using the children's `order'
		// attribute values. We are using a simple insertion sort as we need
		// stability (insertion order must be preserved) and cross-platform
		// support. We should eventually switch to merge sort (or something
		// else) if the number of items becomes significant enough.
		for(uint i = 0; i < item->getCount(); i++) {
			indices[i] = i;
			for(uint j = i; j > 0; j--) {
				uint prev = indices[j-1];
				uint curr = indices[j];
				if(item->at(prev)->GetOrder() <= item->at(curr)->GetOrder()) {
					break;
				}
				indices[j - 1] = curr;
				indices[j] = prev;
			}
		}
		layout->ordered_indices = indices;
	}
	layout->flex_dim = 0;
	layout->flex_grows = 0;
	layout->flex_shrinks = 0;
	layout->reverse2 = false;
	layout->wrap = item->wrap != FLEX_WRAP_NO_WRAP;
	if(layout->wrap) {
		if(item->wrap == FLEX_WRAP_WRAP_REVERSE) {
			layout->reverse2 = true;
			layout->pos2 = layout->align_dim;
		}
	}
	else {
		layout->pos2 = layout->vertical ? item->padding_left : item->padding_top;
	}
	layout->need_lines = layout->wrap && item->align_content != FLEX_ALIGN_START;
	layout->P_Lines = NULL;
	layout->lines_count = 0;
	layout->lines_sizes = 0;
}

static void layout_cleanup(LayoutFlex * pLayout)
{
	if(pLayout) {
		ZFREE(pLayout->ordered_indices);
		ZFREE(pLayout->P_Lines);
		pLayout->lines_count = 0;
	}
}

/*#define LAYOUT_RESET() \
	do { \
		layout->line_dim = layout->wrap ? 0 : layout->align_dim; \
		layout->flex_dim = layout->size_dim; \
		layout->extra_flex_dim = 0; \
		layout->flex_grows = 0; \
		layout->flex_shrinks = 0; \
	} while(0)*/

#define LAYOUT_CHILD_AT(item, i) (item->at((layout->ordered_indices ? layout->ordered_indices[i] : i)))
#define _LAYOUT_FRAME(child, name) child->frame[layout->frame_ ## name ## _i]
#define CHILD_POS(child) _LAYOUT_FRAME(child, pos)
#define CHILD_POS2(child) _LAYOUT_FRAME(child, pos2)
#define CHILD_SIZE(child) _LAYOUT_FRAME(child, size)
#define CHILD_SIZE2(child) _LAYOUT_FRAME(child, size2)
#define CHILD_MARGIN(child, if_vertical, if_horizontal) (layout->vertical ? child->margin_ ## if_vertical : child->margin_ ## if_horizontal)

static void layout_item(LayoutFlexItem * item, float width, float height);

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

static flex_align child_align(LayoutFlexItem * child, LayoutFlexItem * parent)
{
	flex_align align = child->align_self;
	if(align == FLEX_ALIGN_AUTO)
		align = parent->align_items;
	return align;
}

static void layout_items(LayoutFlexItem * item, uint child_begin, uint child_end, uint children_count, LayoutFlex * layout)
{
	assert(children_count <= (child_end - child_begin));
	if(children_count > 0) {
		if(layout->flex_dim > 0 && layout->extra_flex_dim > 0) {
			// If the container has a positive flexible space, let's add to it
			// the sizes of all flexible children.
			layout->flex_dim += layout->extra_flex_dim;
		}
		// Determine the main axis initial position and optional spacing.
		float pos = 0.0f;
		float spacing = 0.0f;
		if(layout->flex_grows == 0 && layout->flex_dim > 0) {
			assert(layout_align(item->justify_content, layout->flex_dim, children_count, &pos, &spacing, false) && "incorrect justify_content");
			if(layout->reverse)
				pos = layout->size_dim - pos;
		}
		if(layout->reverse)
			pos -= layout->vertical ? item->padding_bottom : item->padding_right;
		else 
			pos += layout->vertical ? item->padding_top : item->padding_left;
		if(layout->wrap && layout->reverse2) {
			layout->pos2 -= layout->line_dim;
		}
		for(uint i = child_begin; i < child_end; i++) {
			LayoutFlexItem * child = LAYOUT_CHILD_AT(item, i);
			if(child->position == FLEX_POSITION_ABSOLUTE) {
				continue; // Already positioned.
			}
			// Grow or shrink the main axis item size if needed.
			float flex_size = 0.0f;
			if(layout->flex_dim > 0.0f) {
				if(child->grow != 0.0f) {
					CHILD_SIZE(child) = 0; // Ignore previous size when growing.
					flex_size = (layout->flex_dim / layout->flex_grows) * child->grow;
				}
			}
			else if(layout->flex_dim < 0.0f) {
				if(child->shrink != 0) {
					flex_size = (layout->flex_dim / layout->flex_shrinks) * child->shrink;
				}
			}
			CHILD_SIZE(child) += flex_size;
			// Set the cross axis position (and stretch the cross axis size if needed).
			float align_size = CHILD_SIZE2(child);
			float align_pos = layout->pos2 + 0.0f;
			switch(child_align(child, item)) {
				case FLEX_ALIGN_END:
					align_pos += layout->line_dim - align_size - CHILD_MARGIN(child, right, bottom);
					break;
				case FLEX_ALIGN_CENTER:
					align_pos += (layout->line_dim / 2) - (align_size / 2) + (CHILD_MARGIN(child, left, top) - CHILD_MARGIN(child, right, bottom));
					break;
				case FLEX_ALIGN_STRETCH:
					if(align_size == 0) {
						CHILD_SIZE2(child) = layout->line_dim - (CHILD_MARGIN(child, left, top) + CHILD_MARGIN(child, right, bottom));
					}
				// fall through
				case FLEX_ALIGN_START:
					align_pos += CHILD_MARGIN(child, left, top);
					break;
				default:
					assert(false && "incorrect align_self");
			}
			CHILD_POS2(child) = align_pos;
			// Set the main axis position.
			if(layout->reverse) {
				pos -= CHILD_MARGIN(child, bottom, right);
				pos -= CHILD_SIZE(child);
				CHILD_POS(child) = pos;
				pos -= spacing;
				pos -= CHILD_MARGIN(child, top, left);
			}
			else {
				pos += CHILD_MARGIN(child, top, left);
				CHILD_POS(child) = pos;
				pos += CHILD_SIZE(child);
				pos += spacing;
				pos += CHILD_MARGIN(child, bottom, right);
			}
			// Now that the item has a frame, we can layout its children.
			layout_item(child, child->frame[2], child->frame[3]);
		}
		if(layout->wrap && !layout->reverse2) {
			layout->pos2 += layout->line_dim;
		}
		if(layout->need_lines) {
			layout->P_Lines = (LayoutFlex::Line *)SAlloc::R(layout->P_Lines, sizeof(LayoutFlex::Line) * (layout->lines_count + 1));
			assert(layout->P_Lines != NULL);
			LayoutFlex::Line * line = &layout->P_Lines[layout->lines_count];
			line->child_begin = child_begin;
			line->child_end = child_end;
			line->size = layout->line_dim;
			layout->lines_count++;
			layout->lines_sizes += line->size;
		}
	}
}

static void layout_item(LayoutFlexItem * item, float width, float height)
{
	if(item->getCount()) {
		LayoutFlex layout_s;
		LayoutFlex * layout = &layout_s;
		layout_init(item, width, height, &layout_s);
		layout->Z(); //LAYOUT_RESET();
		uint last_layout_child = 0;
		uint relative_children_count = 0;
		for(uint i = 0; i < item->getCount(); i++) {
			LayoutFlexItem * child = LAYOUT_CHILD_AT(item, i);
			// Items with an absolute position have their frames determined
			// directly and are skipped during layout.
			if(child->position == FLEX_POSITION_ABSOLUTE) {
	#define ABSOLUTE_SIZE(val, pos1, pos2, dim) (!fisnanf(val) ? val : (!fisnanf(pos1) && !fisnanf(pos2) ? dim - pos2 - pos1 : 0))
	#define ABSOLUTE_POS(pos1, pos2, size, dim) (!fisnanf(pos1) ? pos1 : (!fisnanf(pos2) ? dim - size - pos2 : 0))
				float child_width = ABSOLUTE_SIZE(child->width, child->left, child->right, width);
				float child_height = ABSOLUTE_SIZE(child->height, child->top, child->bottom, height);
				float child_x = ABSOLUTE_POS(child->left, child->right, child_width, width);
				float child_y = ABSOLUTE_POS(child->top, child->bottom, child_height, height);
				child->frame[0] = child_x;
				child->frame[1] = child_y;
				child->frame[2] = child_width;
				child->frame[3] = child_height;
				// Now that the item has a frame, we can layout its children.
				layout_item(child, child->frame[2], child->frame[3]); // @recursion
	#undef ABSOLUTE_POS
	#undef ABSOLUTE_SIZE
				continue;
			}
			// Initialize frame.
			child->frame[0] = 0.0f;
			child->frame[1] = 0.0f;
			child->frame[2] = child->width;
			child->frame[3] = child->height;
			// Main axis size defaults to 0.
			if(fisnanf(CHILD_SIZE(child))) {
				CHILD_SIZE(child) = 0;
			}
			// Cross axis size defaults to the parent's size (or line size in wrap
			// mode, which is calculated later on).
			if(fisnanf(CHILD_SIZE2(child))) {
				if(layout->wrap)
					layout->need_lines = true;
				else {
					CHILD_SIZE2(child) = (layout->vertical ? width : height) - CHILD_MARGIN(child, left, top) - CHILD_MARGIN(child, right, bottom);
				}
			}
			// Call the self_sizing callback if provided. Only non-NAN values
			// are taken into account. If the item's cross-axis align property
			// is set to stretch, ignore the value returned by the callback.
			if(child->CbSelfSizing) {
				float size[2] = { child->frame[2], child->frame[3] };
				child->CbSelfSizing(child, size);
				for(uint j = 0; j < 2; j++) {
					uint size_off = j + 2;
					if(size_off == layout->frame_size2_i && child_align(child, item) == FLEX_ALIGN_STRETCH) {
						continue;
					}
					float val = size[j];
					if(!fisnanf(val)) {
						child->frame[size_off] = val;
					}
				}
			}
			// Honor the `basis' property which overrides the main-axis size.
			if(!fisnanf(child->basis)) {
				assert(child->basis >= 0.0f);
				CHILD_SIZE(child) = child->basis;
			}
			float child_size = CHILD_SIZE(child);
			if(layout->wrap) {
				if(layout->flex_dim < child_size) {
					// Not enough space for this child on this line, layout the
					// remaining items and move it to a new line.
					layout_items(item, last_layout_child, i, relative_children_count, layout);
					layout->Z(); //LAYOUT_RESET();
					last_layout_child = i;
					relative_children_count = 0;
				}
				float child_size2 = CHILD_SIZE2(child);
				if(!fisnanf(child_size2) && child_size2 > layout->line_dim) {
					layout->line_dim = child_size2;
				}
			}
			assert(child->grow >= 0.0f);
			assert(child->shrink >= 0.0f);
			layout->flex_grows += child->grow;
			layout->flex_shrinks += child->shrink;
			layout->flex_dim -= child_size + (CHILD_MARGIN(child, top, left) + CHILD_MARGIN(child, bottom, right));
			relative_children_count++;
			if(child_size > 0 && child->grow > 0.0f) {
				layout->extra_flex_dim += child_size;
			}
		}
		// Layout remaining items in wrap mode, or everything otherwise.
		layout_items(item, last_layout_child, item->getCount(), relative_children_count, layout);
		// In wrap mode we may need to tweak the position of each line according to
		// the align_content property as well as the cross-axis size of items that
		// haven't been set yet.
		if(layout->need_lines && layout->lines_count > 0) {
			float pos = 0.0f;
			float spacing = 0.0f;
			float flex_dim = layout->align_dim - layout->lines_sizes;
			if(flex_dim > 0) {
				assert(layout_align(item->align_content, flex_dim, layout->lines_count, &pos, &spacing, true) && "incorrect align_content");
			}
			float old_pos = 0.0f;
			if(layout->reverse2) {
				pos = layout->align_dim - pos;
				old_pos = layout->align_dim;
			}
			for(uint i = 0; i < layout->lines_count; i++) {
				LayoutFlex::Line * line = &layout->P_Lines[i];
				if(layout->reverse2) {
					pos -= line->size;
					pos -= spacing;
					old_pos -= line->size;
				}
				// Re-position the children of this line, honoring any child
				// alignment previously set within the line.
				for(uint j = line->child_begin; j < line->child_end; j++) {
					LayoutFlexItem * child = LAYOUT_CHILD_AT(item, j);
					if(child->position == FLEX_POSITION_ABSOLUTE) {
						continue; // Should not be re-positioned.
					}
					if(fisnanf(CHILD_SIZE2(child))) {
						// If the child's cross axis size hasn't been set it, it defaults to the line size.
						CHILD_SIZE2(child) = line->size + (item->align_content == FLEX_ALIGN_STRETCH ? spacing : 0);
					}
					CHILD_POS2(child) = pos + (CHILD_POS2(child) - old_pos);
				}
				if(!layout->reverse2) {
					pos += line->size;
					pos += spacing;
					old_pos += line->size;
				}
			}
		}
		layout_cleanup(layout);
	}
}

#undef CHILD_MARGIN
#undef CHILD_POS
#undef CHILD_POS2
#undef CHILD_SIZE
#undef CHILD_SIZE2
#undef _LAYOUT_FRAME
#undef LAYOUT_CHILD_AT
//#undef LAYOUT_RESET

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
		if(!pItem->P_Parent && !fisnan(pItem->width) && !fisnan(pItem->height) && !pItem->CbSelfSizing) {
			assert(pItem->P_Parent == NULL);
			assert(!fisnan(pItem->width));
			assert(!fisnan(pItem->height));
			assert(pItem->CbSelfSizing == NULL);
			layout_item(pItem, pItem->width, pItem->height);
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
