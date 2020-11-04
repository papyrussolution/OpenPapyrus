// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See the LICENSE.txt file in the project root
// for the license information.

#ifndef __FLEX_H_
#define __FLEX_H_

struct LayoutFlexItem;

// Create a new flex item.
LayoutFlexItem * flex_item_new(void);
// Free memory associated with a flex item and its children.
// This function can only be called on a root item.
//void flex_item_free(LayoutFlexItem * item);
// Manage items.
void flex_item_add(LayoutFlexItem * item, LayoutFlexItem * child);
void flex_item_delete(LayoutFlexItem * item, uint index);
//uint flex_item_count(LayoutFlexItem * item);
LayoutFlexItem * flex_item_child(LayoutFlexItem * item, uint index);
LayoutFlexItem * flex_item_parent(LayoutFlexItem * item);
//LayoutFlexItem * flex_item_root(LayoutFlexItem * item);
// Layout the items associated with this item, as well as their children.
// This function can only be called on a root item whose `width' and `height'
// properties have been set.
int DoLayoutFlex(LayoutFlexItem * pItem);

// Retrieve the layout frame associated with an item. These functions should
// be called *after* the layout is done.
//float flex_item_get_frame_x(LayoutFlexItem * item);
//float flex_item_get_frame_y(LayoutFlexItem * item);
//float flex_item_get_frame_width(LayoutFlexItem * item);
//float flex_item_get_frame_height(LayoutFlexItem * item);

/*
length-unit: % | mm | m | cm
length-unit-optional: length-unit | ;
measured-value: number unit-optional
range: measured-value..measured-value

box (x, y, x2, y2) // 4 values
box (width, height) // 2 values
box (width, undefined) // 2 values
box (50%, 10) // 2 values
box (40%..50%, 10..30) // 2 values
box (x, y, (width, height)) 

layout abc rowreverse wrap {
}
*/
enum flex_align {
	FLEX_ALIGN_AUTO = 0,
	FLEX_ALIGN_STRETCH,
	FLEX_ALIGN_CENTER,
	FLEX_ALIGN_START,
	FLEX_ALIGN_END,
	FLEX_ALIGN_SPACE_BETWEEN,
	FLEX_ALIGN_SPACE_AROUND,
	FLEX_ALIGN_SPACE_EVENLY
};

enum flex_position {
	FLEX_POSITION_RELATIVE = 0,
	FLEX_POSITION_ABSOLUTE
};

enum flex_direction {
	FLEX_DIRECTION_ROW = 0,
	FLEX_DIRECTION_ROW_REVERSE,
	FLEX_DIRECTION_COLUMN,
	FLEX_DIRECTION_COLUMN_REVERSE
};

enum flex_wrap {
	FLEX_WRAP_NO_WRAP = 0,
	FLEX_WRAP_WRAP,
	FLEX_WRAP_WRAP_REVERSE
};

// size[0] == width, size[1] == height
typedef void (__stdcall * FlexSelfSizingProc)(LayoutFlexItem * pItem, float size[2]);
typedef void (__stdcall * FlexSetupProc)(LayoutFlexItem * pItem, float size[4]);

struct LayoutFlexItem : public TSCollection <LayoutFlexItem> {
	LayoutFlexItem();
	~LayoutFlexItem();
	int    GetOrder() const;
	void   SetOrder(int o);
	void   UpdateShouldOrderShildren();
	FRect  GetFrame() const
	{
		/*
		FRAME_GETTER(x, 0)
		FRAME_GETTER(y, 1)
		FRAME_GETTER(width, 2)
		FRAME_GETTER(height, 3)
		*/
		FRect r;
		r.a.X = frame[0];
		r.a.Y = frame[1];
		r.b.X = frame[0] + frame[2];
		r.b.Y = frame[1] + frame[3];
		return r;
	}
	LayoutFlexItem * GetRoot()
	{
		LayoutFlexItem * p_root = this;
		while(p_root->P_Parent) {
			p_root = p_root->P_Parent;
		}
		return p_root;
	}
	// attributes {
	float  width; // NAN
	float  height; // NAN
	float  left; // NAN
	float  right; // NAN
	float  top; // NAN
	float  bottom; // NAN
	float  padding_left; // 0.0f
	float  padding_right; // 0.0f
	float  padding_top; // 0.0f
	float  padding_bottom; // 0.0f
	float  margin_left; // 0.0f
	float  margin_right; // 0.0f
	float  margin_top; // 0.0f
	float  margin_bottom; // 0.0f
	flex_align justify_content; // FLEX_ALIGN_START
	flex_align align_content; // FLEX_ALIGN_STRETCH
	flex_align align_items; // FLEX_ALIGN_STRETCH
	flex_align align_self; // FLEX_ALIGN_AUTO
	flex_position position; // FLEX_POSITION_RELATIVE
	flex_direction direction; // FLEX_DIRECTION_COLUMN
	flex_wrap wrap; // FLEX_WRAP_NO_WRAP
	float  grow; // 0.0f
	float  shrink; // 1.0
	float  basis; // NAN
	// An item can store an arbitrary pointer, which can be used by bindings as the address of a managed object.
	void * managed_ptr; // NULL
	// An item can provide a self_sizing callback function that will be called 
	// during layout and which can customize the dimensions (width and height) of the item.
	FlexSelfSizingProc CbSelfSizing; // NULL
	FlexSetupProc CbSetup; // NULL
	// } attributes
	float  frame[4];
	LayoutFlexItem * P_Parent;
	bool should_order_children;
private:
	int    order; // 0
};

#ifndef FLEX_ATTRIBUTE
	#define FLEX_ATTRIBUTE(name, type, def) \
		type flex_item_get_ ## name(LayoutFlexItem * item); \
		void flex_item_set_ ## name(LayoutFlexItem * item, type value);
#endif
#else // !__FLEX_H_
	#ifndef FLEX_ATTRIBUTE
		#define FLEX_ATTRIBUTE(name, type, def)
	#endif
#endif
/*
// Following are the list of properties associated with an item.
//
// Each property is exposed with getter and setter functions, for instance
// the `width' property can be get and set using the respective
// `flex_item_get_width()' and `flex_item_set_width()' functions.
//
// You can also see the type and default value for each property.
FLEX_ATTRIBUTE(width, float, NAN)
FLEX_ATTRIBUTE(height, float, NAN)
FLEX_ATTRIBUTE(left, float, NAN)
FLEX_ATTRIBUTE(right, float, NAN)
FLEX_ATTRIBUTE(top, float, NAN)
FLEX_ATTRIBUTE(bottom, float, NAN)
FLEX_ATTRIBUTE(padding_left, float, 0)
FLEX_ATTRIBUTE(padding_right, float, 0)
FLEX_ATTRIBUTE(padding_top, float, 0)
FLEX_ATTRIBUTE(padding_bottom, float, 0)
FLEX_ATTRIBUTE(margin_left, float, 0)
FLEX_ATTRIBUTE(margin_right, float, 0)
FLEX_ATTRIBUTE(margin_top, float, 0)
FLEX_ATTRIBUTE(margin_bottom, float, 0)
FLEX_ATTRIBUTE(justify_content, flex_align, FLEX_ALIGN_START)
FLEX_ATTRIBUTE(align_content, flex_align, FLEX_ALIGN_STRETCH)
FLEX_ATTRIBUTE(align_items, flex_align, FLEX_ALIGN_STRETCH)
FLEX_ATTRIBUTE(align_self, flex_align, FLEX_ALIGN_AUTO)
FLEX_ATTRIBUTE(position, flex_position, FLEX_POSITION_RELATIVE)
FLEX_ATTRIBUTE(direction, flex_direction, FLEX_DIRECTION_COLUMN)
FLEX_ATTRIBUTE(wrap, flex_wrap, FLEX_WRAP_NO_WRAP)
FLEX_ATTRIBUTE(grow, float, 0.0)
FLEX_ATTRIBUTE(shrink, float, 1.0)
FLEX_ATTRIBUTE(order, int, 0)
FLEX_ATTRIBUTE(basis, float, NAN)
// An item can store an arbitrary pointer, which can be used by bindings as the address of a managed object.
FLEX_ATTRIBUTE(managed_ptr, void *, NULL)
// An item can provide a self_sizing callback function that will be called
// during layout and which can customize the dimensions (width and height) of the item.
FLEX_ATTRIBUTE(self_sizing, flex_self_sizing, NULL)
*/
#undef FLEX_ATTRIBUTE

