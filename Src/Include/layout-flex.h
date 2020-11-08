// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See the LICENSE.txt file in the project root
// for the license information.

#ifndef __FLEX_H_
#define __FLEX_H_

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

struct LayoutFlexItem : public TSCollection <LayoutFlexItem> {
	// size[0] == width, size[1] == height
	typedef void (__stdcall * FlexSelfSizingProc)(LayoutFlexItem * pItem, float size[2]);
	typedef void (__stdcall * FlexSetupProc)(LayoutFlexItem * pItem, float size[4]);

	LayoutFlexItem();
	~LayoutFlexItem();
	int    GetOrder() const;
	void   SetOrder(int o);
	void   UpdateShouldOrderShildren();
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
	int    GetFullWidth(float * pS) const;
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
	int    GetFullHeight(float * pS) const;
	//
	// Descr: Возвращает финальный расчетный прямоугольник элемента.
	//
	FRect  GetFrame() const;
	//
	// Descr: Возвращает корневой элемент дерева, компонентом которого является this.
	//
	LayoutFlexItem * GetRoot();
	//
	// attributes {
	FPoint Size;    // { NAN, NAN }
	FRect  N;       // Номинальные границы элемента {NAN, NAN, NAN, NAN}
	FRect  Padding; // { 0.0f, 0.0f, 0.0f, 0.0f }
	FRect  Margin;  // { 0.0f, 0.0f, 0.0f, 0.0f }
	flex_align JustifyContent; // FLEX_ALIGN_START
	flex_align AlignContent; // FLEX_ALIGN_STRETCH
	flex_align AlignItems; // FLEX_ALIGN_STRETCH
	flex_align AlignSelf; // FLEX_ALIGN_AUTO
	flex_position PositionMode; // FLEX_POSITION_RELATIVE
	flex_direction Direction; // FLEX_DIRECTION_COLUMN
	flex_wrap WrapMode; // FLEX_WRAP_NO_WRAP
	enum {
		fPositionAbsolute         = 0x0001, // else Relative
		fWrap                     = 0x0002,
		fWrapReverse              = 0x0004, // ignored if !(Flags & fWrap)
		fStateShouldOrderChildren = 0x0100
	};
	float  grow;   // 0.0f
	float  shrink; // 1.0
	float  basis;  // NAN
	void * managed_ptr; // NULL // An item can store an arbitrary pointer, which can be used by bindings as the address of a managed object.
	// An item can provide a self_sizing callback function that will be called 
	// during layout and which can customize the dimensions (width and height) of the item.
	FlexSelfSizingProc CbSelfSizing; // NULL
	FlexSetupProc CbSetup; // NULL
	// } attributes
	float  frame[4];
	LayoutFlexItem * P_Parent;
	//bool   should_order_children;
	//uint8  Reserve[3]; // @alignment
	uint   Flags;  // fXXX
protected:
	int    order;  // 0
public:
	void   DoLayout(float width, float height);
	void   DoLayoutChildren(uint childBeginIdx, uint childEndIdx, uint childrenCount, /*LayoutFlex*/void * pLayout);
	flex_align FASTCALL GetChildAlign(const LayoutFlexItem * pChild) const;
};

// Free memory associated with a flex item and its children.
// This function can only be called on a root item.
//void flex_item_free(LayoutFlexItem * item);
// Manage items.
void flex_item_add(LayoutFlexItem * item, LayoutFlexItem * child);
void flex_item_delete(LayoutFlexItem * item, uint index);
//uint flex_item_count(LayoutFlexItem * item);
//LayoutFlexItem * flex_item_child(LayoutFlexItem * item, uint index);
//LayoutFlexItem * flex_item_parent(LayoutFlexItem * item);
//LayoutFlexItem * flex_item_root(LayoutFlexItem * item);
//
// Descr: Layout the items associated with this item, as well as their children.
//   This function can only be called on a root item whose `width' and `height'
//   properties have been set.
//
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

#endif 