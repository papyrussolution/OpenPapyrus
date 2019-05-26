// TLAYOUT.CPP
// Copyright (c) A.Sobolev 2019
//
#include <slib.h>
#include <tv.h>
#pragma hdrstop
#define LAY_IMPLEMENTATION
#include <layout-2d.h>
//
//
//
// Useful math utilities
static LAY_FORCE_INLINE lay_scalar lay_scalar_max(lay_scalar a, lay_scalar b) { return a > b ? a : b; }
static LAY_FORCE_INLINE lay_scalar lay_scalar_min(lay_scalar a, lay_scalar b) { return a < b ? a : b; }
static LAY_FORCE_INLINE float lay_float_max(float a, float b) { return a > b ? a : b; }
static LAY_FORCE_INLINE float lay_float_min(float a, float b) { return a < b ? a : b; }

/*void lay_init_context(TLayout::Context * ctx)
{
	ctx->capacity = 0;
	ctx->count = 0;
	ctx->items = NULL;
	ctx->rects = NULL;
}*/

void lay_reserve_items_capacity(TLayout::Context * ctx, lay_id count)
{
	if(count >= ctx->capacity) {
		ctx->capacity = count;
		const size_t item_size = sizeof(TLayout::Item) + sizeof(lay_vec4);
		ctx->items = static_cast<TLayout::Item *>(SAlloc::R(ctx->items, ctx->capacity * item_size));
		const TLayout::Item * past_last = ctx->items + ctx->capacity;
		ctx->rects = (lay_vec4 *)past_last;
	}
}

void lay_reset_context(TLayout::Context * ctx)
{
	ctx->count = 0;
}

//static void lay_calc_size(TLayout::Context * ctx, lay_id item, int dim);
//static void lay_arrange(TLayout::Context * ctx, lay_id item, int dim);
//
// Alternatively, we could use a flag bit to indicate whether an item's children
// have already been wrapped and may need re-wrapping. If we do that, in the
// future, this would become deprecated and we could make it a no-op.
//
void lay_clear_item_break(TLayout::Context * ctx, lay_id item)
{
	assert(ctx != NULL);
	TLayout::Item * pitem = ctx->GetItem(item);
	pitem->flags = pitem->flags & ~LAY_BREAK;
}

lay_id lay_items_count(TLayout::Context * ctx)
{
	assert(ctx != NULL);
	return ctx->count;
}

lay_id lay_items_capacity(TLayout::Context * ctx)
{
	assert(ctx != NULL);
	return ctx->capacity;
}

static LAY_FORCE_INLINE void lay_append_by_ptr(TLayout::Item * LAY_RESTRICT pearlier, lay_id later, TLayout::Item * LAY_RESTRICT plater)
{
	plater->next_sibling = pearlier->next_sibling;
	plater->flags |= LAY_ITEM_INSERTED;
	pearlier->next_sibling = later;
}

//lay_id lay_last_child(const TLayout::Context * ctx, lay_id parent)
lay_id TLayout::GetLastChild(lay_id parentId) const
{
	const TLayout::Item * pparent = Ctx.GetItemC(parentId);
	lay_id child = pparent->first_child;
	if(child == LAY_INVALID_ID) 
		return LAY_INVALID_ID;
	const TLayout::Item * pchild = Ctx.GetItemC(child);
	lay_id result = child;
	for(;; ) {
		lay_id next = pchild->next_sibling;
		if(next == LAY_INVALID_ID) 
			break;
		result = next;
		pchild = Ctx.GetItemC(next);
	}
	return result;
}

void lay_append(TLayout::Context * ctx, lay_id earlier, lay_id later)
{
	assert(later != 0); // Must not be root item
	assert(earlier != later); // Must not be same item id
	TLayout::Item * LAY_RESTRICT pearlier = ctx->GetItem(earlier);
	TLayout::Item * LAY_RESTRICT plater = ctx->GetItem(later);
	lay_append_by_ptr(pearlier, later, plater);
}

void lay_insert(TLayout::Context * ctx, lay_id parent, lay_id child)
{
	assert(child != 0); // Must not be root item
	assert(parent != child); // Must not be same item id
	TLayout::Item * LAY_RESTRICT pparent = ctx->GetItem(parent);
	TLayout::Item * LAY_RESTRICT pchild = ctx->GetItem(child);
	assert(!(pchild->flags & LAY_ITEM_INSERTED));
	// Parent has no existing children, make inserted item the first child.
	if(pparent->first_child == LAY_INVALID_ID) {
		pparent->first_child = child;
		pchild->flags |= LAY_ITEM_INSERTED;
		// Parent has existing items, iterate to find the last child and append the
		// inserted item after it.
	}
	else {
		lay_id next = pparent->first_child;
		TLayout::Item * LAY_RESTRICT pnext = ctx->GetItem(next);
		for(;; ) {
			next = pnext->next_sibling;
			if(next == LAY_INVALID_ID) 
				break;
			pnext = ctx->GetItem(next);
		}
		lay_append_by_ptr(pnext, child, pchild);
	}
}

void lay_push(TLayout::Context * ctx, lay_id parent, lay_id new_child)
{
	assert(new_child != 0); // Must not be root item
	assert(parent != new_child); // Must not be same item id
	TLayout::Item * LAY_RESTRICT pparent = ctx->GetItem(parent);
	lay_id old_child = pparent->first_child;
	TLayout::Item * LAY_RESTRICT pchild = ctx->GetItem(new_child);
	assert(!(pchild->flags & LAY_ITEM_INSERTED));
	pparent->first_child = new_child;
	pchild->flags |= LAY_ITEM_INSERTED;
	pchild->next_sibling = old_child;
}

//lay_vec2 lay_get_size(TLayout::Context * ctx, lay_id item)
lay_vec2 TLayout::GetItemSize(lay_id itemId) const
{
	const TLayout::Item * pitem = Ctx.GetItemC(itemId);
	return pitem->size;
}

void lay_get_size_xy(TLayout::Context * ctx, lay_id item, lay_scalar * x, lay_scalar * y)
{
	const TLayout::Item * pitem = ctx->GetItemC(item);
	lay_vec2 size = pitem->size;
	*x = size[0];
	*y = size[1];
}

//void lay_set_size(TLayout::Context * ctx, lay_id item, lay_vec2 size)
void TLayout::SetItemSize(lay_id itemId, lay_vec2 size)
{
	TLayout::Item * pitem = Ctx.GetItem(itemId);
	pitem->size = size;
	uint32_t flags = pitem->flags;
	if(size[0] == 0)
		flags &= ~LAY_ITEM_HFIXED;
	else
		flags |= LAY_ITEM_HFIXED;
	if(size[1] == 0)
		flags &= ~LAY_ITEM_VFIXED;
	else
		flags |= LAY_ITEM_VFIXED;
	pitem->flags = flags;
}

//void lay_set_size_xy(TLayout::Context * ctx, lay_id item, lay_scalar width, lay_scalar height)
void TLayout::SetItemSizeXy(lay_id itemId, lay_scalar width, lay_scalar height)
{
	TLayout::Item * pitem = Ctx.GetItem(itemId);
	pitem->size[0] = width;
	pitem->size[1] = height;
	// Kinda redundant, whatever
	uint32_t flags = pitem->flags;
	if(width == 0)
		flags &= ~LAY_ITEM_HFIXED;
	else
		flags |= LAY_ITEM_HFIXED;
	if(height == 0)
		flags &= ~LAY_ITEM_VFIXED;
	else
		flags |= LAY_ITEM_VFIXED;
	pitem->flags = flags;
}

void lay_set_behave(TLayout::Context * ctx, lay_id item, uint32_t flags)
{
	assert((flags & LAY_ITEM_LAYOUT_MASK) == flags);
	TLayout::Item * pitem = ctx->GetItem(item);
	pitem->flags = (pitem->flags & ~LAY_ITEM_LAYOUT_MASK) | flags;
}

void lay_set_contain(TLayout::Context * ctx, lay_id item, uint32_t flags)
{
	assert((flags & LAY_ITEM_BOX_MASK) == flags);
	TLayout::Item * pitem = ctx->GetItem(item);
	pitem->flags = (pitem->flags & ~LAY_ITEM_BOX_MASK) | flags;
}

void lay_set_margins(TLayout::Context * ctx, lay_id item, lay_vec4 ltrb)
{
	TLayout::Item * pitem = ctx->GetItem(item);
	pitem->margins = ltrb;
}

void lay_set_margins_ltrb(TLayout::Context * ctx, lay_id item, lay_scalar l, lay_scalar t, lay_scalar r, lay_scalar b)
{
	TLayout::Item * pitem = ctx->GetItem(item);
	// Alternative, uses stack and addressed writes
	//pitem->margins = lay_vec4_xyzw(l, t, r, b);
	// Alternative, uses rax and left-shift
	//pitem->margins = (lay_vec4){l, t, r, b};
	// Fewest instructions, but uses more addressed writes?
	pitem->margins[0] = l;
	pitem->margins[1] = t;
	pitem->margins[2] = r;
	pitem->margins[3] = b;
}

lay_vec4 lay_get_margins(TLayout::Context * ctx, lay_id item)
{
	return ctx->GetItem(item)->margins;
}

void lay_get_margins_ltrb(TLayout::Context * ctx, lay_id item, lay_scalar * l, lay_scalar * t, lay_scalar * r, lay_scalar * b)
{
	TLayout::Item * pitem = ctx->GetItem(item);
	lay_vec4 margins = pitem->margins;
	*l = margins[0];
	*t = margins[1];
	*r = margins[2];
	*b = margins[3];
}

//static LAY_FORCE_INLINE lay_scalar lay_calc_wrapped_overlayed_size(TLayout::Context * ctx, lay_id item, int dim)
lay_scalar TLayout::CalcWrappedOverlayedSize(lay_id item, int dim) const
{
	const int wdim = dim + 2;
	const TLayout::Item * LAY_RESTRICT pitem = Ctx.GetItemC(item);
	lay_scalar need_size = 0;
	lay_scalar need_size2 = 0;
	lay_id child = pitem->first_child;
	while(child != LAY_INVALID_ID) {
		const TLayout::Item * pchild = Ctx.GetItemC(child);
		lay_vec4 rect = Ctx.rects[child];
		if(pchild->flags & LAY_BREAK) {
			need_size2 += need_size;
			need_size = 0;
		}
		lay_scalar child_size = rect[dim] + rect[2 + dim] + pchild->margins[wdim];
		need_size = lay_scalar_max(need_size, child_size);
		child = pchild->next_sibling;
	}
	return need_size2 + need_size;
}
//
// Equivalent to uiComputeWrappedStackedSize
//
//static LAY_FORCE_INLINE lay_scalar lay_calc_wrapped_stacked_size(TLayout::Context * ctx, lay_id item, int dim)
lay_scalar TLayout::CalcWrappedStackedSize(lay_id item, int dim) const
{
	const int wdim = dim + 2;
	const TLayout::Item * LAY_RESTRICT pitem = Ctx.GetItemC(item);
	lay_scalar need_size = 0;
	lay_scalar need_size2 = 0;
	lay_id child = pitem->first_child;
	while(child != LAY_INVALID_ID) {
		const TLayout::Item * pchild = Ctx.GetItemC(child);
		lay_vec4 rect = Ctx.rects[child];
		if(pchild->flags & LAY_BREAK) {
			need_size2 = lay_scalar_max(need_size2, need_size);
			need_size = 0;
		}
		need_size += rect[dim] + rect[2 + dim] + pchild->margins[wdim];
		child = pchild->next_sibling;
	}
	return lay_scalar_max(need_size2, need_size);
}
//
//
//
TLayout::EntryBlock::EntryBlock() : Id(-1), ContainFlags(0), BehaveFlags(0)
{
	Sz = 0;
	memzero(Reserve, sizeof(Reserve));
}

TLayout::TLayout()
{
	//lay_init_context(&Ctx);
}

TLayout::~TLayout()
{
	//lay_destroy_context(&Ctx);
	if(Ctx.items) {
		SAlloc::F(Ctx.items);
		Ctx.items = NULL;
		Ctx.rects = NULL;
	}
}

TLayout::Context::Context() : items(0), rects(0), capacity(0), count(0)
{
}

TLayout::Item * FASTCALL TLayout::Context::GetItem(lay_id itemId) // TLayout::Item * lay_get_item(const TLayout::Context * ctx, lay_id id)
{
	assert(itemId != LAY_INVALID_ID && itemId < count);
	return (items + itemId);
}

const TLayout::Item * FASTCALL TLayout::Context::GetItemC(lay_id itemId) const // TLayout::Item * lay_get_item(const TLayout::Context * ctx, lay_id id)
{
	assert(itemId != LAY_INVALID_ID && itemId < count);
	return (items + itemId);
}

lay_id TLayout::CreateItem() // lay_id lay_item(TLayout::Context * ctx)
{
	lay_id idx = Ctx.count++;
	if(idx >= Ctx.capacity) {
		Ctx.capacity = (Ctx.capacity < 1) ? 32 : (Ctx.capacity * 4);
		const size_t item_size = sizeof(TLayout::Item) + sizeof(lay_vec4);
		Ctx.items = (TLayout::Item *)SAlloc::R(Ctx.items, Ctx.capacity * item_size);
		const TLayout::Item * past_last = Ctx.items + Ctx.capacity;
		Ctx.rects = (lay_vec4 *)past_last;
	}
	TLayout::Item * p_item = Ctx.GetItem(idx);
	// We can either do this here, or when creating/resetting buffer
	memzero(p_item, sizeof(TLayout::Item));
	p_item->first_child = LAY_INVALID_ID;
	p_item->next_sibling = LAY_INVALID_ID;
	// hmm
	memzero(&Ctx.rects[idx], sizeof(lay_vec4));
	return idx;
}
//
// @todo restrict item ptrs correctly
//
//static LAY_FORCE_INLINE lay_scalar lay_calc_overlayed_size(TLayout::Context * ctx, lay_id item, int dim)
lay_scalar TLayout::CalcOverlayedSize(lay_id itemId, int dim) const
{
	const int wdim = dim + 2;
	const TLayout::Item * LAY_RESTRICT pitem = Ctx.GetItemC(itemId);
	lay_scalar need_size = 0;
	lay_id child = pitem->first_child;
	while(child != LAY_INVALID_ID) {
		const TLayout::Item * pchild = Ctx.GetItemC(child);
		lay_vec4 rect = Ctx.rects[child];
		// width = start margin + calculated width + end margin
		lay_scalar child_size = rect[dim] + rect[2 + dim] + pchild->margins[wdim];
		need_size = lay_scalar_max(need_size, child_size);
		child = pchild->next_sibling;
	}
	return need_size;
}

//static LAY_FORCE_INLINE lay_scalar lay_calc_stacked_size(TLayout::Context * ctx, lay_id item, int dim)
lay_scalar TLayout::CalcStackedSize(lay_id item, int dim) const
{
	const int wdim = dim + 2;
	const TLayout::Item * LAY_RESTRICT pitem = Ctx.GetItemC(item);
	lay_scalar need_size = 0;
	lay_id child = pitem->first_child;
	while(child != LAY_INVALID_ID) {
		const TLayout::Item * pchild = Ctx.GetItemC(child);
		lay_vec4 rect = Ctx.rects[child];
		need_size += rect[dim] + rect[2 + dim] + pchild->margins[wdim];
		child = pchild->next_sibling;
	}
	return need_size;
}

void TLayout::CalcSize(lay_id itemId, int dim)
{
	TLayout::Item * pitem = Ctx.GetItem(itemId);
	lay_id child = pitem->first_child;
	while(child != LAY_INVALID_ID) {
		// NOTE: this is recursive and will run out of stack space if items are
		// nested too deeply.
		CalcSize(child, dim); // @recursion
		TLayout::Item * pchild = Ctx.GetItem(child);
		child = pchild->next_sibling;
	}
	// Set the mutable rect output data to the starting input data
	Ctx.rects[itemId][dim] = pitem->margins[dim];
	// If we have an explicit input size, just set our output size (which other
	// calc_size and arrange procedures will use) to it.
	if(pitem->size[dim] != 0) {
		Ctx.rects[itemId][2 + dim] = pitem->size[dim];
		return;
	}
	// Calculate our size based on children items. Note that we've already
	// called lay_calc_size on our children at this point.
	lay_scalar cal_size;
	switch(pitem->flags & LAY_ITEM_BOX_MODEL_MASK) {
		case LAY_COLUMN|LAY_WRAP:
		    // flex model
		    if(dim) // direction
			    cal_size = CalcStackedSize(itemId, 1);
		    else
			    cal_size = CalcOverlayedSize(itemId, 0);
		    break;
		case LAY_ROW|LAY_WRAP:
		    // flex model
		    if(!dim) // direction
			    cal_size = CalcWrappedStackedSize(itemId, 0);
		    else
			    cal_size = CalcWrappedOverlayedSize(itemId, 1);
		    break;
		case LAY_COLUMN:
		case LAY_ROW:
		    // flex model
		    if((pitem->flags & 1) == (uint32_t)dim) // direction
			    cal_size = CalcStackedSize(itemId, dim);
		    else
			    cal_size = CalcOverlayedSize(itemId, dim);
		    break;
		default:
		    // layout model
		    cal_size = CalcOverlayedSize(itemId, dim);
		    break;
	}
	// Set our output data size. Will be used by parent calc_size procedures.,
	// and by arrange procedures.
	Ctx.rects[itemId][2 + dim] = cal_size;
}

//static LAY_FORCE_INLINE void lay_arrange_stacked(TLayout::Context * ctx, lay_id item, int dim, bool wrap)
void TLayout::ArrangeStacked(lay_id itemId, int dim, bool wrap)
{
	const int wdim = dim + 2;
	TLayout::Item * pitem = Ctx.GetItem(itemId);
	const uint32_t item_flags = pitem->flags;
	lay_vec4 rect = Ctx.rects[itemId];
	lay_scalar space = rect[2 + dim];
	float max_x2 = (float)(rect[dim] + space);
	lay_id start_child = pitem->first_child;
	while(start_child != LAY_INVALID_ID) {
		lay_scalar used = 0;
		uint32_t count = 0; // count of fillers
		uint32_t squeezed_count = 0; // count of squeezable elements
		uint32_t total = 0;
		bool hardbreak = false;
		// first pass: count items that need to be expanded,
		// and the space that is used
		lay_id child = start_child;
		lay_id end_child = LAY_INVALID_ID;
		while(child != LAY_INVALID_ID) {
			TLayout::Item * pchild = Ctx.GetItem(child);
			const uint32_t child_flags = pchild->flags;
			const uint32_t flags = (child_flags & LAY_ITEM_LAYOUT_MASK) >> dim;
			const uint32_t fflags = (child_flags & LAY_ITEM_FIXED_MASK) >> dim;
			const lay_vec4 child_margins = pchild->margins;
			lay_vec4 child_rect = Ctx.rects[child];
			lay_scalar extend = used;
			if((flags & LAY_HFILL) == LAY_HFILL) {
				++count;
				extend += child_rect[dim] + child_margins[wdim];
			}
			else {
				if((fflags & LAY_ITEM_HFIXED) != LAY_ITEM_HFIXED)
					++squeezed_count;
				extend += child_rect[dim] + child_rect[2 + dim] + child_margins[wdim];
			}
			// wrap on end of line or manual flag
			if(wrap && (total && ((extend > space) || (child_flags & LAY_BREAK)))) {
				end_child = child;
				hardbreak = (child_flags & LAY_BREAK) == LAY_BREAK;
				// add marker for subsequent queries
				pchild->flags = child_flags | LAY_BREAK;
				break;
			}
			else {
				used = extend;
				child = pchild->next_sibling;
			}
			++total;
		}
		lay_scalar extra_space = space - used;
		float filler = 0.0f;
		float spacer = 0.0f;
		float extra_margin = 0.0f;
		float eater = 0.0f;
		if(extra_space > 0) {
			if(count > 0)
				filler = (float)extra_space / (float)count;
			else if(total > 0) {
				switch(item_flags & LAY_JUSTIFY) {
					case LAY_JUSTIFY:
					    // justify when not wrapping or not in last line,
					    // or not manually breaking
					    if(!wrap || ((end_child != LAY_INVALID_ID) && !hardbreak))
						    spacer = (float)extra_space / (float)(total - 1);
					    break;
					case LAY_START:
					    break;
					case LAY_END:
					    extra_margin = extra_space;
					    break;
					default:
					    extra_margin = extra_space / 2.0f;
					    break;
				}
			}
		}
#ifdef LAY_FLOAT
		// In floating point, it's possible to end up with some small negative
		// value for extra_space, while also have a 0.0 squeezed_count. This
		// would cause divide by zero. Instead, we'll check to see if
		// squeezed_count is > 0. I believe this produces the same results as
		// the original oui int-only code. However, I don't have any tests for
		// it, so I'll leave it if-def'd for now.
		else if(!wrap && (squeezed_count > 0))
#else
		// This is the original oui code
		else if(!wrap && (extra_space < 0))
#endif
			eater = (float)extra_space / (float)squeezed_count;

		// distribute width among items
		float x = (float)rect[dim];
		float x1;
		// second pass: distribute and rescale
		child = start_child;
		while(child != end_child) {
			lay_scalar ix0, ix1;
			TLayout::Item * pchild = Ctx.GetItem(child);
			const uint32_t child_flags = pchild->flags;
			const uint32_t flags = (child_flags & LAY_ITEM_LAYOUT_MASK) >> dim;
			const uint32_t fflags = (child_flags & LAY_ITEM_FIXED_MASK) >> dim;
			const lay_vec4 child_margins = pchild->margins;
			lay_vec4 child_rect = Ctx.rects[child];
			x += (float)child_rect[dim] + extra_margin;
			if((flags & LAY_HFILL) == LAY_HFILL) // grow
				x1 = x + filler;
			else if((fflags & LAY_ITEM_HFIXED) == LAY_ITEM_HFIXED)
				x1 = x + (float)child_rect[2 + dim];
			else // squeeze
				x1 = x + lay_float_max(0.0f, (float)child_rect[2 + dim] + eater);

			ix0 = (lay_scalar)x;
			if(wrap)
				ix1 = (lay_scalar)lay_float_min(max_x2 - (float)child_margins[wdim], x1);
			else
				ix1 = (lay_scalar)x1;
			child_rect[dim] = ix0; // pos
			child_rect[dim + 2] = ix1 - ix0; // size
			Ctx.rects[child] = child_rect;
			x = x1 + (float)child_margins[wdim];
			child = pchild->next_sibling;
			extra_margin = spacer;
		}
		start_child = end_child;
	}
}

//static LAY_FORCE_INLINE void lay_arrange_overlay(TLayout::Context * ctx, lay_id item, int dim)
void TLayout::ArrangeOverlay(lay_id itemId, int dim) const
{
	const int wdim = dim + 2;
	const TLayout::Item * pitem = Ctx.GetItemC(itemId);
	const lay_vec4 rect = Ctx.rects[itemId];
	const lay_scalar offset = rect[dim];
	const lay_scalar space = rect[2 + dim];
	lay_id child = pitem->first_child;
	while(child != LAY_INVALID_ID) {
		const TLayout::Item * pchild = Ctx.GetItemC(child);
		const uint32_t b_flags = (pchild->flags & LAY_ITEM_LAYOUT_MASK) >> dim;
		const lay_vec4 child_margins = pchild->margins;
		lay_vec4 child_rect = Ctx.rects[child];
		switch(b_flags & LAY_HFILL) {
			case LAY_HCENTER:
			    child_rect[dim] += (space - child_rect[2 + dim]) / 2 - child_margins[wdim];
			    break;
			case LAY_RIGHT:
			    child_rect[dim] += space - child_rect[2 + dim] - child_margins[wdim];
			    break;
			case LAY_HFILL:
			    child_rect[2 + dim] = lay_scalar_max(0, space - child_rect[dim] - child_margins[wdim]);
			    break;
			default:
			    break;
		}
		child_rect[dim] += offset;
		Ctx.rects[child] = child_rect;
		child = pchild->next_sibling;
	}
}

//static LAY_FORCE_INLINE void lay_arrange_overlay_squeezed_range(TLayout::Context * ctx, int dim, lay_id start_item, lay_id end_item, lay_scalar offset, lay_scalar space)
void TLayout::ArrangeOverlaySqueezedRange(int dim, lay_id start_item, lay_id end_item, lay_scalar offset, lay_scalar space)
{
	int wdim = dim + 2;
	lay_id item = start_item;
	while(item != end_item) {
		const TLayout::Item * pitem = Ctx.GetItemC(item);
		const uint32_t b_flags = (pitem->flags & LAY_ITEM_LAYOUT_MASK) >> dim;
		const lay_vec4 margins = pitem->margins;
		lay_vec4 rect = Ctx.rects[item];
		lay_scalar min_size = lay_scalar_max(0, space - rect[dim] - margins[wdim]);
		switch(b_flags & LAY_HFILL) {
			case LAY_HCENTER:
			    rect[2 + dim] = lay_scalar_min(rect[2 + dim], min_size);
			    rect[dim] += (space - rect[2 + dim]) / 2 - margins[wdim];
			    break;
			case LAY_RIGHT:
			    rect[2 + dim] = lay_scalar_min(rect[2 + dim], min_size);
			    rect[dim] = space - rect[2 + dim] - margins[wdim];
			    break;
			case LAY_HFILL:
			    rect[2 + dim] = min_size;
			    break;
			default:
			    rect[2 + dim] = lay_scalar_min(rect[2 + dim], min_size);
			    break;
		}
		rect[dim] += offset;
		Ctx.rects[item] = rect;
		item = pitem->next_sibling;
	}
}

//static LAY_FORCE_INLINE lay_scalar lay_arrange_wrapped_overlay_squeezed(TLayout::Context * ctx, lay_id item, int dim)
lay_scalar TLayout::ArrangeWrappedOverlaySqueezed(lay_id itemId, int dim)
{
	const int wdim = dim + 2;
	const TLayout::Item * pitem = Ctx.GetItemC(itemId);
	lay_scalar offset = Ctx.rects[itemId][dim];
	lay_scalar need_size = 0;
	lay_id child = pitem->first_child;
	lay_id start_child = child;
	while(child != LAY_INVALID_ID) {
		const TLayout::Item * pchild = Ctx.GetItemC(child);
		if(pchild->flags & LAY_BREAK) {
			ArrangeOverlaySqueezedRange(dim, start_child, child, offset, need_size);
			offset += need_size;
			start_child = child;
			need_size = 0;
		}
		const lay_vec4 rect = Ctx.rects[child];
		lay_scalar child_size = rect[dim] + rect[2 + dim] + pchild->margins[wdim];
		need_size = lay_scalar_max(need_size, child_size);
		child = pchild->next_sibling;
	}
	ArrangeOverlaySqueezedRange(dim, start_child, LAY_INVALID_ID, offset, need_size);
	offset += need_size;
	return offset;
}

void TLayout::Arrange(lay_id item, int dim)
{
	TLayout::Item * pitem = Ctx.GetItem(item);
	const uint32_t flags = pitem->flags;
	switch(flags & LAY_ITEM_BOX_MODEL_MASK) {
		case LAY_COLUMN | LAY_WRAP:
		    if(dim != 0) {
			    ArrangeStacked(item, 1, true);
			    lay_scalar offset = ArrangeWrappedOverlaySqueezed(item, 0);
			    Ctx.rects[item][2 + 0] = offset - Ctx.rects[item][0];
		    }
		    break;
		case LAY_ROW | LAY_WRAP:
		    if(dim == 0)
			    ArrangeStacked(item, 0, true);
		    else {
			    // discard return value
			    ArrangeWrappedOverlaySqueezed(item, 1);
			}
		    break;
		case LAY_COLUMN:
		case LAY_ROW:
		    if((flags & 1) == (uint32_t)dim) {
			    ArrangeStacked(item, dim, false);
		    }
		    else {
			    const lay_vec4 rect = Ctx.rects[item];
			    ArrangeOverlaySqueezedRange(dim, pitem->first_child, LAY_INVALID_ID, rect[dim], rect[2 + dim]);
		    }
		    break;
		default:
		    ArrangeOverlay(item, dim);
		    break;
	}
	lay_id child = pitem->first_child;
	while(child != LAY_INVALID_ID) {
		// NOTE: this is recursive and will run out of stack space if items are
		// nested too deeply.
		Arrange(child, dim); // @recursion
		TLayout::Item * pchild = Ctx.GetItem(child);
		child = pchild->next_sibling;
	}
}

void TLayout::RunItem(lay_id itemId) //void lay_run_item(TLayout::Context * ctx, lay_id item)
{
	CalcSize(itemId, 0);
	Arrange(itemId, 0);
	CalcSize(itemId, 1);
	Arrange(itemId, 1);
}

void TLayout::Run() // void lay_run_context(TLayout::Context * ctx)
{
	if(Ctx.count > 0)
		RunItem(0);
}
