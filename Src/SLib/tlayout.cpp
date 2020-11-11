// TLAYOUT.CPP
// Copyright (c) A.Sobolev 2019, 2020
//
#include <slib-internal.h>
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
		ctx->P_Items = static_cast<TLayout::Item *>(SAlloc::R(ctx->P_Items, ctx->capacity * item_size));
		const TLayout::Item * past_last = ctx->P_Items + ctx->capacity;
		ctx->P_Rects = (lay_vec4 *)past_last;
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
	TLayout::Item * p_item = ctx->GetItem(item);
	p_item->FirstChild = p_item->FirstChild & ~LAY_BREAK;
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

//static LAY_FORCE_INLINE void lay_append_by_ptr(TLayout::Item * LAY_RESTRICT pearlier, lay_id later, TLayout::Item * LAY_RESTRICT plater)
int TLayout::AppendByPtr(Item * pEarlier, lay_id laterId, Item * pLater)
{
	int    ok = 1;
	pLater->NextSibling = pEarlier->NextSibling;
	pLater->Flags |= LAY_ITEM_INSERTED;
	pEarlier->NextSibling = laterId;
	return ok;
}

//lay_id lay_last_child(const TLayout::Context * ctx, lay_id parent)
lay_id TLayout::GetLastChild(lay_id parentId) const
{
	const TLayout::Item * pparent = Ctx.GetItemC(parentId);
	lay_id child = pparent->FirstChild;
	if(child == LAY_INVALID_ID) 
		return LAY_INVALID_ID;
	const TLayout::Item * pchild = Ctx.GetItemC(child);
	lay_id result = child;
	for(;; ) {
		lay_id next = pchild->NextSibling;
		if(next == LAY_INVALID_ID) 
			break;
		result = next;
		pchild = Ctx.GetItemC(next);
	}
	return result;
}

int TLayout::Append(lay_id earlierId, lay_id laterId) // void lay_append(TLayout::Context * ctx, lay_id earlier, lay_id later)
{
	int    ok = 1;
	assert(laterId != 0); // Must not be root item
	assert(earlierId != laterId); // Must not be same item id
	TLayout::Item * LAY_RESTRICT pearlier = Ctx.GetItem(earlierId);
	TLayout::Item * LAY_RESTRICT plater = Ctx.GetItem(laterId);
	AppendByPtr(pearlier, laterId, plater);
	return ok;
}

int TLayout::Insert(lay_id parentId, lay_id newItemId) //void lay_insert(TLayout::Context * ctx, lay_id parent, lay_id child)
{
	int    ok = 1;
	assert(newItemId != 0); // Must not be root item
	assert(parentId != newItemId); // Must not be same item id
	TLayout::Item * LAY_RESTRICT pparent = Ctx.GetItem(parentId);
	TLayout::Item * LAY_RESTRICT pchild = Ctx.GetItem(newItemId);
	assert(!(pchild->Flags & LAY_ITEM_INSERTED));
	// Parent has no existing children, make inserted item the first child.
	if(pparent->FirstChild == LAY_INVALID_ID) {
		pparent->FirstChild = newItemId;
		pchild->Flags |= LAY_ITEM_INSERTED;
		// Parent has existing items, iterate to find the last child and append the
		// inserted item after it.
	}
	else {
		lay_id next = pparent->FirstChild;
		TLayout::Item * LAY_RESTRICT pnext = Ctx.GetItem(next);
		for(;; ) {
			next = pnext->NextSibling;
			if(next == LAY_INVALID_ID) 
				break;
			pnext = Ctx.GetItem(next);
		}
		AppendByPtr(pnext, newItemId, pchild);
	}
	return ok;
}

int TLayout::Push(lay_id parentId, lay_id newItemId) //void lay_push(TLayout::Context * ctx, lay_id parent, lay_id new_child)
{
	int    ok = 1;
	assert(newItemId != 0); // Must not be root item
	assert(parentId != newItemId); // Must not be same item id
	TLayout::Item * LAY_RESTRICT pparent = Ctx.GetItem(parentId);
	lay_id old_child = pparent->FirstChild;
	TLayout::Item * LAY_RESTRICT pchild = Ctx.GetItem(newItemId);
	assert(!(pchild->Flags & LAY_ITEM_INSERTED));
	pparent->FirstChild = newItemId;
	pchild->Flags |= LAY_ITEM_INSERTED;
	pchild->NextSibling = old_child;
	return ok;
}

//lay_vec2 lay_get_size(TLayout::Context * ctx, lay_id item)
lay_vec2 TLayout::GetItemSize(lay_id itemId) const
{
	const TLayout::Item * pitem = Ctx.GetItemC(itemId);
	return pitem->Size;
}

//void lay_get_size_xy(TLayout::Context * ctx, lay_id item, lay_scalar * x, lay_scalar * y)
int TLayout::GetItemSize(lay_id itemId, lay_scalar * pX, lay_scalar * pY) const
{
	int    ok = 1;
	const TLayout::Item * p_item = Ctx.GetItemC(itemId);
	if(p_item) {
		lay_vec2 size = p_item->Size;
		*pX = size[0];
		*pY = size[1];
	}
	else {
		*pX = 0;
		*pY = 0;
		ok = 0;
	}
	return ok;
}

//void lay_set_size(TLayout::Context * ctx, lay_id item, lay_vec2 size)
void TLayout::SetItemSize(lay_id itemId, lay_vec2 size)
{
	TLayout::Item * pitem = Ctx.GetItem(itemId);
	pitem->Size = size;
	uint32 flags = pitem->Flags;
	if(size[0] == 0)
		flags &= ~LAY_ITEM_HFIXED;
	else
		flags |= LAY_ITEM_HFIXED;
	if(size[1] == 0)
		flags &= ~LAY_ITEM_VFIXED;
	else
		flags |= LAY_ITEM_VFIXED;
	pitem->Flags = flags;
}

//void lay_set_size_xy(TLayout::Context * ctx, lay_id item, lay_scalar width, lay_scalar height)
void TLayout::SetItemSizeXy(lay_id itemIdx, lay_scalar width, lay_scalar height)
{
	TLayout::Item * p_item = Ctx.GetItem(itemIdx);
	if(p_item) {
		p_item->Size[0] = width;
		p_item->Size[1] = height;
		// Kinda redundant, whatever
		uint32 flags = p_item->Flags;
		if(width == 0)
			flags &= ~LAY_ITEM_HFIXED;
		else
			flags |= LAY_ITEM_HFIXED;
		if(height == 0)
			flags &= ~LAY_ITEM_VFIXED;
		else
			flags |= LAY_ITEM_VFIXED;
		p_item->Flags = flags;
	}
}

//void lay_set_behave(TLayout::Context * ctx, lay_id item, uint32 flags)
void TLayout::SetBehaveOptions(lay_id itemIdx, uint flags)
{
	assert((flags & LAY_ITEM_LAYOUT_MASK) == flags);
	TLayout::Item * p_item = Ctx.GetItem(itemIdx);
	if(p_item)
		p_item->Flags = (p_item->Flags & ~LAY_ITEM_LAYOUT_MASK) | flags;
}

//void lay_set_contain(TLayout::Context * ctx, lay_id item, uint32 flags)
void TLayout::SetContainOptions(lay_id itemId, uint flags)
{
	assert((flags & LAY_ITEM_BOX_MASK) == flags);
	TLayout::Item * p_item = Ctx.GetItem(itemId);
	if(p_item)
		p_item->Flags = (p_item->Flags & ~LAY_ITEM_BOX_MASK) | flags;
}

/*void lay_set_margins(TLayout::Context * ctx, lay_id item, lay_vec4 ltrb)
{
	TLayout::Item * pitem = ctx->GetItem(item);
	pitem->Margins = ltrb;
}

void lay_set_margins_ltrb(TLayout::Context * ctx, lay_id item, lay_scalar l, lay_scalar t, lay_scalar r, lay_scalar b)
{
	TLayout::Item * pitem = ctx->GetItem(item);
	// Alternative, uses stack and addressed writes
	//pitem->margins = lay_vec4_xyzw(l, t, r, b);
	// Alternative, uses rax and left-shift
	//pitem->margins = (lay_vec4){l, t, r, b};
	// Fewest instructions, but uses more addressed writes?
	pitem->Margins[0] = l;
	pitem->Margins[1] = t;
	pitem->Margins[2] = r;
	pitem->Margins[3] = b;
}*/

int TLayout::SetItemMargins(lay_id itemIdx, const TRect & rM)
{
	int    ok = 1;
	TLayout::Item * p_item = Ctx.GetItem(itemIdx);
	if(p_item) {
		p_item->Margins[0] = rM.a.x;
		p_item->Margins[1] = rM.a.y;
		p_item->Margins[2] = rM.b.x;
		p_item->Margins[3] = rM.b.y;
	}
	else
		ok = 0;
	return ok;
}

int TLayout::GetItemMargins(lay_id itemIdx, TRect * pM) const
{
	int    ok = 1;
	const TLayout::Item * p_item = Ctx.GetItemC(itemIdx);
	if(p_item) {
		pM->a.x = p_item->Margins[0];
		pM->a.y = p_item->Margins[1];
		pM->b.x = p_item->Margins[2];
		pM->b.y = p_item->Margins[3];
	}
	else
		ok = 0;
	return ok;
}

/*lay_vec4 lay_get_margins(TLayout::Context * ctx, lay_id item)
{
	return ctx->GetItem(item)->Margins;
}

void lay_get_margins_ltrb(TLayout::Context * ctx, lay_id item, lay_scalar * l, lay_scalar * t, lay_scalar * r, lay_scalar * b)
{
	TLayout::Item * pitem = ctx->GetItem(item);
	lay_vec4 margins = pitem->Margins;
	*l = margins[0];
	*t = margins[1];
	*r = margins[2];
	*b = margins[3];
}*/

//static LAY_FORCE_INLINE lay_scalar lay_calc_wrapped_overlayed_size(TLayout::Context * ctx, lay_id item, int dim)
lay_scalar TLayout::CalcWrappedOverlayedSize(lay_id item, int dim) const
{
	const int wdim = dim + 2;
	const TLayout::Item * LAY_RESTRICT pitem = Ctx.GetItemC(item);
	lay_scalar need_size = 0;
	lay_scalar need_size2 = 0;
	lay_id child = pitem->FirstChild;
	while(child != LAY_INVALID_ID) {
		const TLayout::Item * p_child = Ctx.GetItemC(child);
		lay_vec4 rect = Ctx.P_Rects[child];
		if(p_child->Flags & LAY_BREAK) {
			need_size2 += need_size;
			need_size = 0;
		}
		lay_scalar child_size = rect[dim] + rect[2 + dim] + p_child->Margins[wdim];
		need_size = lay_scalar_max(need_size, child_size);
		child = p_child->NextSibling;
	}
	return (need_size2 + need_size);
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
	lay_id child = pitem->FirstChild;
	while(child != LAY_INVALID_ID) {
		const TLayout::Item * pchild = Ctx.GetItemC(child);
		lay_vec4 rect = Ctx.P_Rects[child];
		if(pchild->Flags & LAY_BREAK) {
			need_size2 = lay_scalar_max(need_size2, need_size);
			need_size = 0;
		}
		need_size += rect[dim] + rect[2 + dim] + pchild->Margins[wdim];
		child = pchild->NextSibling;
	}
	return lay_scalar_max(need_size2, need_size);
}
//
//
//
TLayout::EntryBlock::EntryBlock() : ContainerDirection(DIREC_UNKN), ContainerAdjustment(ADJ_LEFT), ContainerFlags(0), BehaveFlags(0)
{
	Sz = 0;
	memzero(Reserve, sizeof(Reserve));
}

SString & TLayout::EntryBlock::SizeToString(SString & rBuf) const
{
	rBuf.Z();
	if(!Sz.IsZero()) {
		if(Sz.x == Sz.y)
			rBuf.Cat(Sz.x);			
		else
			rBuf.Cat(Sz.x).Comma().Cat(Sz.y);
	}
	return rBuf;
}

int TLayout::EntryBlock::SizeFromString(const char * pBuf)
{
	int    ok = 1;
	SString temp_buf(pBuf);
	temp_buf.Strip();
	StringSet ss;
	temp_buf.Tokenize(",;", ss);
	const uint _c = ss.getCount();
	if(oneof3(_c, 0, 1, 2)) {
		if(_c == 0) {
			Sz = 0;
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
					double v = temp_buf.ToReal();
					assert(oneof2(tokn, 1, 2));
					if(tokn == 1) {
						if(_c == 1) 
							Sz = static_cast<int16>(v);
						else // _c == 2
							Sz.x = static_cast<int16>(v);
					}
					else if(tokn == 2)
						Sz.y = static_cast<int16>(v);
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

SString & TLayout::EntryBlock::MarginsToString(SString & rBuf) const
{
	rBuf.Z();
	if(!Margins.IsEmpty()) {
		if(Margins.a.x == Margins.b.x && Margins.a.y == Margins.b.y) {
			if(Margins.a.x == Margins.a.y)
				rBuf.Cat(Margins.a.x);
			else
				rBuf.Cat(Margins.a.x).Comma().Cat(Margins.a.y);
		}
		else
			rBuf.Cat(Margins.a.x).Comma().Cat(Margins.a.y).Comma().Cat(Margins.b.x).Comma().Cat(Margins.b.y);
	}
	return rBuf;
}

int TLayout::EntryBlock::MarginsFromString(const char * pBuf)
{
	int    ok = 1;
	SString temp_buf(pBuf);
	temp_buf.Strip();
	StringSet ss;
	temp_buf.Tokenize(",;", ss);
	const uint _c = ss.getCount();
	if(oneof4(_c, 0, 1, 2, 4)) {
		if(_c == 0) {
			Margins.set(0, 0, 0, 0);
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
					double v = temp_buf.ToReal();
					const int iv = static_cast<int>(v);
					assert(oneof4(tokn, 1, 2, 3, 4));
					if(tokn == 1) {
						if(_c == 1) {
							 Margins.set(iv, iv, iv, iv);
						}
						else if(_c == 2) { 
							Margins.a.x = iv;
							Margins.b.x = iv;
						}
						else { // _c == 4
							Margins.a.x = iv;
						}
					}
					else if(tokn == 2) {
						if(_c == 2) {
							Margins.a.y = iv;
							Margins.b.y = iv;
						}
						else { // _c == 4
							Margins.a.y = iv;
						}
					}
					else if(tokn == 3) {
						Margins.b.x = iv;
					}
					else if(tokn == 4) {
						Margins.b.y = iv;
					}
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

TLayout::TLayout()
{
	//lay_init_context(&Ctx);
}

TLayout::~TLayout()
{
	//lay_destroy_context(&Ctx);
	if(Ctx.P_Items) {
		SAlloc::F(Ctx.P_Items);
		Ctx.P_Items = NULL;
		Ctx.P_Items = NULL;
	}
}

TLayout::Context::Context() : P_Items(0), P_Rects(0), capacity(0), count(0)
{
}

TLayout::Item * FASTCALL TLayout::Context::GetItem(lay_id itemIdx) // TLayout::Item * lay_get_item(const TLayout::Context * ctx, lay_id id)
{
	assert(itemIdx != LAY_INVALID_ID && itemIdx < count);
	return (P_Items + itemIdx);
}

const TLayout::Item * FASTCALL TLayout::Context::GetItemC(lay_id itemId) const // TLayout::Item * lay_get_item(const TLayout::Context * ctx, lay_id id)
{
	assert(itemId != LAY_INVALID_ID && itemId < count);
	return (P_Items + itemId);
}

lay_id TLayout::CreateItem() // lay_id lay_item(TLayout::Context * ctx)
{
	lay_id idx = Ctx.count++;
	if(idx >= Ctx.capacity) {
		Ctx.capacity = (Ctx.capacity < 1) ? 32 : (Ctx.capacity * 4);
		const size_t item_size = sizeof(TLayout::Item) + sizeof(lay_vec4);
		Ctx.P_Items = static_cast<TLayout::Item *>(SAlloc::R(Ctx.P_Items, Ctx.capacity * item_size));
		Ctx.P_Rects = reinterpret_cast<lay_vec4 *>(Ctx.P_Items + Ctx.capacity);
	}
	TLayout::Item * p_item = Ctx.GetItem(idx);
	// We can either do this here, or when creating/resetting buffer
	memzero(p_item, sizeof(TLayout::Item));
	p_item->FirstChild = LAY_INVALID_ID;
	p_item->NextSibling = LAY_INVALID_ID;
	// hmm
	memzero(&Ctx.P_Rects[idx], sizeof(lay_vec4));
	return idx;
}
//
// @todo restrict item ptrs correctly
//
//static LAY_FORCE_INLINE lay_scalar lay_calc_overlayed_size(TLayout::Context * ctx, lay_id item, int dim)
lay_scalar TLayout::CalcOverlayedSize(lay_id itemIdx, int dim) const
{
	const int wdim = dim + 2;
	const TLayout::Item * LAY_RESTRICT p_item = Ctx.GetItemC(itemIdx);
	lay_scalar need_size = 0;
	lay_id child = p_item->FirstChild;
	while(child != LAY_INVALID_ID) {
		const TLayout::Item * pchild = Ctx.GetItemC(child);
		lay_vec4 rect = Ctx.P_Rects[child];
		// width = start margin + calculated width + end margin
		lay_scalar child_size = rect[dim] + rect[2 + dim] + pchild->Margins[wdim];
		need_size = lay_scalar_max(need_size, child_size);
		child = pchild->NextSibling;
	}
	return need_size;
}

//static LAY_FORCE_INLINE lay_scalar lay_calc_stacked_size(TLayout::Context * ctx, lay_id item, int dim)
lay_scalar TLayout::CalcStackedSize(lay_id itemIdx, int dim) const
{
	const int wdim = dim + 2;
	const TLayout::Item * LAY_RESTRICT p_item = Ctx.GetItemC(itemIdx);
	lay_scalar need_size = 0;
	lay_id child = p_item->FirstChild;
	while(child != LAY_INVALID_ID) {
		const TLayout::Item * pchild = Ctx.GetItemC(child);
		lay_vec4 rect = Ctx.P_Rects[child];
		need_size += rect[dim] + rect[2 + dim] + pchild->Margins[wdim];
		child = pchild->NextSibling;
	}
	return need_size;
}

void TLayout::CalcSize(lay_id itemId, int dim)
{
	TLayout::Item * pitem = Ctx.GetItem(itemId);
	lay_id child = pitem->FirstChild;
	while(child != LAY_INVALID_ID) {
		// NOTE: this is recursive and will run out of stack space if items are
		// nested too deeply.
		CalcSize(child, dim); // @recursion
		TLayout::Item * pchild = Ctx.GetItem(child);
		child = pchild->NextSibling;
	}
	// Set the mutable rect output data to the starting input data
	Ctx.P_Rects[itemId][dim] = pitem->Margins[dim];
	// If we have an explicit input size, just set our output size (which other
	// calc_size and arrange procedures will use) to it.
	if(pitem->Size[dim] != 0) {
		Ctx.P_Rects[itemId][2 + dim] = pitem->Size[dim];
		return;
	}
	// Calculate our size based on children items. Note that we've already
	// called lay_calc_size on our children at this point.
	lay_scalar cal_size;
	switch(pitem->Flags & LAY_ITEM_BOX_MODEL_MASK) {
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
		    if((pitem->Flags & 1) == (uint32)dim) // direction
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
	Ctx.P_Rects[itemId][2 + dim] = cal_size;
}

//static LAY_FORCE_INLINE void lay_arrange_stacked(TLayout::Context * ctx, lay_id item, int dim, bool wrap)
void TLayout::ArrangeStacked(lay_id itemIdx, int dim, bool wrap)
{
	const int wdim = dim + 2;
	TLayout::Item * p_item = Ctx.GetItem(itemIdx);
	const uint32 item_flags = p_item->Flags;
	lay_vec4 rect = Ctx.P_Rects[itemIdx];
	lay_scalar space = rect[2 + dim];
	float max_x2 = (float)(rect[dim] + space);
	lay_id start_child = p_item->FirstChild;
	while(start_child != LAY_INVALID_ID) {
		lay_scalar used = 0;
		uint32 count = 0; // count of fillers
		uint32 squeezed_count = 0; // count of squeezable elements
		uint32 total = 0;
		bool   hardbreak = false;
		// first pass: count items that need to be expanded,
		// and the space that is used
		lay_id child_idx = start_child;
		lay_id end_child = LAY_INVALID_ID;
		while(child_idx != LAY_INVALID_ID) {
			TLayout::Item * pchild = Ctx.GetItem(child_idx);
			const uint32 child_flags = pchild->Flags;
			const uint32 flags = (child_flags & LAY_ITEM_LAYOUT_MASK) >> dim;
			const uint32 fflags = (child_flags & LAY_ITEM_FIXED_MASK) >> dim;
			const lay_vec4 child_margins = pchild->Margins;
			lay_vec4 child_rect = Ctx.P_Rects[child_idx];
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
				end_child = child_idx;
				hardbreak = (child_flags & LAY_BREAK) == LAY_BREAK;
				// add marker for subsequent queries
				pchild->Flags = child_flags | LAY_BREAK;
				break;
			}
			else {
				used = extend;
				child_idx = pchild->NextSibling;
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
		child_idx = start_child;
		while(child_idx != end_child) {
			lay_scalar ix0, ix1;
			TLayout::Item * pchild = Ctx.GetItem(child_idx);
			const uint32 child_flags = pchild->Flags;
			const uint32 flags = (child_flags & LAY_ITEM_LAYOUT_MASK) >> dim;
			const uint32 fflags = (child_flags & LAY_ITEM_FIXED_MASK) >> dim;
			const lay_vec4 child_margins = pchild->Margins;
			lay_vec4 child_rect = Ctx.P_Rects[child_idx];
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
			Ctx.P_Rects[child_idx] = child_rect;
			x = x1 + (float)child_margins[wdim];
			child_idx = pchild->NextSibling;
			extra_margin = spacer;
		}
		start_child = end_child;
	}
}

//static LAY_FORCE_INLINE void lay_arrange_overlay(TLayout::Context * ctx, lay_id item, int dim)
void TLayout::ArrangeOverlay(lay_id itemIdx, int dim) const
{
	const int wdim = dim + 2;
	const TLayout::Item * pitem = Ctx.GetItemC(itemIdx);
	const lay_vec4 rect = Ctx.P_Rects[itemIdx];
	const lay_scalar offset = rect[dim];
	const lay_scalar space = rect[2 + dim];
	lay_id child = pitem->FirstChild;
	while(child != LAY_INVALID_ID) {
		const TLayout::Item * pchild = Ctx.GetItemC(child);
		const uint32 b_flags = (pchild->Flags & LAY_ITEM_LAYOUT_MASK) >> dim;
		const lay_vec4 child_margins = pchild->Margins;
		lay_vec4 child_rect = Ctx.P_Rects[child];
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
		Ctx.P_Rects[child] = child_rect;
		child = pchild->NextSibling;
	}
}

//static LAY_FORCE_INLINE void lay_arrange_overlay_squeezed_range(TLayout::Context * ctx, int dim, lay_id start_item, lay_id end_item, lay_scalar offset, lay_scalar space)
void TLayout::ArrangeOverlaySqueezedRange(int dim, lay_id startItemIdx, lay_id endItemIdx, lay_scalar offset, lay_scalar space)
{
	int wdim = dim + 2;
	lay_id item = startItemIdx;
	while(item != endItemIdx) {
		const TLayout::Item * pitem = Ctx.GetItemC(item);
		const uint32 b_flags = (pitem->Flags & LAY_ITEM_LAYOUT_MASK) >> dim;
		const lay_vec4 margins = pitem->Margins;
		lay_vec4 rect = Ctx.P_Rects[item];
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
		Ctx.P_Rects[item] = rect;
		item = pitem->NextSibling;
	}
}

//static LAY_FORCE_INLINE lay_scalar lay_arrange_wrapped_overlay_squeezed(TLayout::Context * ctx, lay_id item, int dim)
lay_scalar TLayout::ArrangeWrappedOverlaySqueezed(lay_id itemIdx, int dim)
{
	const int wdim = dim + 2;
	const TLayout::Item * pitem = Ctx.GetItemC(itemIdx);
	lay_scalar offset = Ctx.P_Rects[itemIdx][dim];
	lay_scalar need_size = 0;
	lay_id child = pitem->FirstChild;
	lay_id start_child = child;
	while(child != LAY_INVALID_ID) {
		const TLayout::Item * p_child = Ctx.GetItemC(child);
		if(p_child->Flags & LAY_BREAK) {
			ArrangeOverlaySqueezedRange(dim, start_child, child, offset, need_size);
			offset += need_size;
			start_child = child;
			need_size = 0;
		}
		const lay_vec4 rect = Ctx.P_Rects[child];
		lay_scalar child_size = rect[dim] + rect[2 + dim] + p_child->Margins[wdim];
		need_size = lay_scalar_max(need_size, child_size);
		child = p_child->NextSibling;
	}
	ArrangeOverlaySqueezedRange(dim, start_child, LAY_INVALID_ID, offset, need_size);
	offset += need_size;
	return offset;
}

void TLayout::Arrange(lay_id itemIdx, int dim)
{
	TLayout::Item * pitem = Ctx.GetItem(itemIdx);
	const uint32 flags = pitem->Flags;
	switch(flags & LAY_ITEM_BOX_MODEL_MASK) {
		case LAY_COLUMN|LAY_WRAP:
		    if(dim != 0) {
			    ArrangeStacked(itemIdx, 1, true);
			    lay_scalar offset = ArrangeWrappedOverlaySqueezed(itemIdx, 0);
			    Ctx.P_Rects[itemIdx][2 + 0] = offset - Ctx.P_Rects[itemIdx][0];
		    }
		    break;
		case LAY_ROW|LAY_WRAP:
		    if(dim == 0)
			    ArrangeStacked(itemIdx, 0, true);
		    else {
			    // discard return value
			    ArrangeWrappedOverlaySqueezed(itemIdx, 1);
			}
		    break;
		case LAY_COLUMN:
		case LAY_ROW:
		    if((flags & 1) == static_cast<uint32>(dim)) {
			    ArrangeStacked(itemIdx, dim, false);
		    }
		    else {
			    const lay_vec4 rect = Ctx.P_Rects[itemIdx];
			    ArrangeOverlaySqueezedRange(dim, pitem->FirstChild, LAY_INVALID_ID, rect[dim], rect[2 + dim]);
		    }
		    break;
		default:
		    ArrangeOverlay(itemIdx, dim);
		    break;
	}
	lay_id child_idx = pitem->FirstChild;
	while(child_idx != LAY_INVALID_ID) {
		// NOTE: this is recursive and will run out of stack space if items are
		// nested too deeply.
		Arrange(child_idx, dim); // @recursion
		TLayout::Item * p_child = Ctx.GetItem(child_idx);
		child_idx = p_child->NextSibling;
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
