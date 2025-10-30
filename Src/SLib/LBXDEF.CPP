// LBXDEF.CPP
// Copyright (c) Sobolev A. 1996, 1997, 2000, 2003, 2005, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022, 2023, 2024, 2025
// @codepage UTF-8
//
#include <slib-internal.h>
#pragma hdrstop

static int substFCMP(const void * i1, const void * i2, void * pExtraData) { return stcomp(reinterpret_cast<TYPEID>(pExtraData), (const void *)i1, (const void *)i2); }

#define ILLEGAL_LISTBOX_BUF_TYPE 0
//
// ListBoxDef
//
ListBoxDef::ListBoxDef(uint aOptions, TYPEID aType) : Options(aOptions), Format(0), Type(aType),
	ViewHight(1), TopItem(0), CurItem(0), CFlags(0), ScrollDelta(0)
{
	SetCapability(0, SlConst::ListBoxDefSignature_DEFAULT);
	UserData.Init();
}

ListBoxDef::~ListBoxDef()
{
	Sign = 0; // Помечаем instance как разрушенный
	UserData.Destroy();
}

void ListBoxDef::SetCapability(long c, uint32 sign)
{
	CFlags |= c;
	Sign = sign;
}

int ListBoxDef::SetUserData(const void * pData, size_t size)
{
	if(UserData.Alloc(size)) {
		memcpy(UserData.P_Buf, pData, size);
		return 1;
	}
	else
		return 0;
}

int ListBoxDef::GetUserData(void * pData, size_t * pSize) const
{
	size_t s = *pSize;
	if(UserData.Size > s) {
		*pSize = UserData.Size;
		return -1;
	}
	else {
		if(pData)
			memcpy(pData, UserData.P_Buf, UserData.Size);
		*pSize = UserData.Size;
		return 1;
	}
}

void ListBoxDef::getScrollData(long * pScrollDelta, long * pScrollPos)
{
	ASSIGN_PTR(pScrollDelta, ScrollDelta);
	ASSIGN_PTR(pScrollPos, CurItem);
}

const char * ListBoxDef::getText(long item, SString & rBuf)
{
	const char * p = static_cast<const char *>(getRow_(item));
	if(p) {
		const  uint h = (Options & lbtAutoID) ? 0 : ((Options & lbtWordID) ? sizeof(int16) : sizeof(int32));
		char   temp_buf[1024];
		sttostr(Type, p+h, Format, temp_buf);
		rBuf = temp_buf;
	}
	else
		rBuf.Z();
	return rBuf;
}

int ListBoxDef::getCurID(long * pId)
{
	union {
		void * vp;
		long * lp;
		int  * ip;
	};
	if(Options & lbtAutoID)
		*pId = _curItem();
	else if((vp = getRow_(_curItem())) != 0)
		*pId = (Options & lbtWordID) ? static_cast<long>(*ip) : *lp;
	else
		return 0;
	return 1;
}

int ListBoxDef::getCurString(SString & rBuf)
{
	getText(_curItem(), rBuf);
	return 1;
}

int ListBoxDef::getCurData(void * b)
{
	const char * p = static_cast<const char *>(getRow_(_curItem()));
	if(p) {
		if(!(Options & lbtAutoID))
			p += (Options & lbtWordID) ? sizeof(int16) : sizeof(int32);
		memcpy(b, p, stsize(Type));
		return 1;
	}
	else
		return 0;
}

void   ListBoxDef::SetOption(uint option, int set) { SETFLAG(Options, option, set); }
long   ListBoxDef::_topItem() const { return TopItem; }
long   ListBoxDef::_curItem() const { return CurItem; }
void   ListBoxDef::setViewHight(int vh) { ViewHight = vh; }
int    ListBoxDef::GetFrameSize() const { return -1; }
int    ListBoxDef::GetFrameState() { return 0; }
int    ListBoxDef::getIdList(LongArray &) { return -1; }
int    ListBoxDef::search(const void * pPattern, CompFunc, int srchMode) { return 0; }
bool   ListBoxDef::IsValid() const { return true; }
int    ListBoxDef::addItem(long id, const char *, long * pPos) { return -1; }
int    ListBoxDef::removeItem(long pos) { return -1; }
void   ListBoxDef::freeAll() {}
int    ListBoxDef::step(long delta) { return go(CurItem + delta); }
int    ListBoxDef::top() { return go(0); }
int    ListBoxDef::bottom() { return go(GetRecsCount()-1); }
long   ListBoxDef::GetRecsCount() const { return 0; }
void * FASTCALL ListBoxDef::getRow_(long idx) { return 0; }
int    ListBoxDef::refresh() { return 1; }
int    ListBoxDef::GetImageIdxByID(long id, long * pIDx) { return ImageIdxAssoc.Search(id, pIDx, 0); }
int    ListBoxDef::AddImageAssoc(long itemID, long imageID) { return ImageAssoc.Add(itemID, (imageID & ~SlConst::VectorImageMask), 0); }
int    ListBoxDef::AddVecImageAssoc(long itemID, long imageID) { return ImageAssoc.Add(itemID, (imageID | SlConst::VectorImageMask), 0); }
bool   ListBoxDef::HasItemColorSpec() const { return (ColorAssoc.getCount() > 0); }

#define RANGE(val,min,max) ((val)<(min)?(min):((val)>(max)?(max):(val)))

int ListBoxDef::go(long row)
{
	if(IsValid()) {
		const long  c = GetRecsCount();
		row = c ? RANGE(row, 0, c-1) : 0;
		ScrollDelta = row - CurItem;
		CurItem = row;
		if(row <= TopItem)
			TopItem = CurItem;
		else if(CurItem && (row >= TopItem + ViewHight))
			TopItem = (CurItem - ViewHight + BIN(ViewHight));
		return 1;
	}
	return 0;
}

int ListBoxDef::TransmitData(int dir, void * pData)
{
	int    s = 0;
	if(dir == 0) {
		if(Options & lbtNoData)
			s = 0;
		if(Options & lbtTransferData)
			s = static_cast<int>(stsize(Type));
		else
			s = (Options & lbtWordID) ? sizeof(int16) : sizeof(int32);
	}
	else if(dir > 0) {
		s = (Options & lbtNoData || !pData) ? 0 : search(pData, 0, (Options & lbtTransferData) ? srchFirst : srchFirst | lbSrchByID);
	}
	else if(dir < 0) {
		if(!(Options & lbtNoData)) {
			if(Options & lbtTransferData)
				getCurData(pData);
			else {
				long   i = 0;
				if(!getCurID(&i))
					i = 0;
				if(Options & lbtWordID)
					*static_cast<short *>(pData) = static_cast<short>(i);
				else
					*static_cast<long *>(pData) = i;
			}
		}
		s = 1;
	}
	return s;
}

void * ListBoxDef::CreateImageList(HINSTANCE hInst) // @v10.9.4 HIMAGELIST-->(void *)
{
	HIMAGELIST himl = NULL;
	LAssocArray added_list;
	ImageIdxAssoc.freeAll();
	if(ImageAssoc.getCount()) {
		himl = ImageList_Create(GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), ILC_COLOR8, 16, 64);
		for(uint i = 0; i < ImageAssoc.getCount(); i++) {
			long item_id  = ImageAssoc.at(i).Key;
			uint image_id = static_cast<uint>(ImageAssoc.at(i).Val);
			long image_idx = -1;
			if(!added_list.Search(image_id, &image_idx, 0)) {
				if(image_id & SlConst::VectorImageMask) {
					TWhatmanToolArray::Item tool_item;
					const SDrawFigure * p_fig = APPL->LoadDrawFigureById(image_id, &tool_item); // @v10.5.0 (image_id & ~0x40000000)-->image_id
					// @v11.9.2 {
					SPtrHandle h_icon = TCanvas2::TransformDrawFigureToIcon(APPL->GetUiToolBox(), p_fig, SPoint2S(16, 16), tool_item.ReplacedColor, SColor(255, 255, 255, 255));
					if(h_icon) {
						image_idx = ImageList_AddIcon(himl, reinterpret_cast<HICON>((void *)h_icon));
						added_list.Add(image_id, image_idx, 0);
						::DestroyIcon(reinterpret_cast<HICON>((void *)h_icon));
					}
					// } @v11.9.2 
					/* @v11.9.2
					if(p_fig) {
						const uint _w = 16;
						const uint _h = 16;
						SImageBuffer ib(_w, _h);
						SPaintToolBox * p_tb = APPL->GetUiToolBox();
						if(p_tb) {
							TCanvas2 canv(*p_tb, ib);
							if(!tool_item.ReplacedColor.IsEmpty()) {
								SColor replacement_color = p_tb->GetColor(TProgram::tbiIconRegColor);
								canv.SetColorReplacement(tool_item.ReplacedColor, replacement_color);
							}
							LMatrix2D mtx;
							SViewPort vp;
							FRect pic_bounds(static_cast<float>(_w), static_cast<float>(_h));
							//
							canv.Rect(pic_bounds);
							canv.Fill(SColor(255, 255, 255, 255), 0); // Прозрачный фон
							canv.PushTransform();
							p_fig->GetViewPort(&vp);
							canv.AddTransform(vp.GetMatrix(pic_bounds, mtx));
							canv.Draw(p_fig);
						}
						HICON h_icon = static_cast<HICON>(ib.TransformToIcon());
						if(h_icon) {
							image_idx = ImageList_AddIcon(himl, h_icon);
							added_list.Add(image_id, image_idx, 0);
							::DestroyIcon(h_icon);
						}
					}*/
				}
				else {
					HICON h_icon = LoadIcon(hInst, MAKEINTRESOURCE(image_id));
					image_idx = ImageList_AddIcon(himl, h_icon);
					added_list.Add(image_id, image_idx, 0);
					::DestroyIcon(h_icon);
				}
			}
			if(image_idx >= 0)
				ImageIdxAssoc.Add(item_id, image_idx, 0);
		}
	}
	return himl;
}

void ListBoxDef::ClearImageAssocList()
{
	ImageAssoc.clear(); // @v12.3.3 freeAll()-->clear()
	ImageIdxAssoc.clear(); // @v12.3.3 freeAll()-->clear()
}

int ListBoxDef::ResetItemColor(long itemID)
{
	int    ok = -1;
	uint   pos = 0;
	if(ColorAssoc.lsearch(&itemID, &pos, CMPF_LONG)) {
		ColorAssoc.atFree(pos);
		ok = 1;
	}
	return ok;
}

int ListBoxDef::SetItemColor(long itemID, SColor fgColor, SColor bckgColor)
{
	int    ok = 0;
	uint   pos = 0;
	if(ColorAssoc.lsearch(&itemID, &pos, CMPF_LONG)) {
		ColorItem & r_item = ColorAssoc.at(pos);
		r_item.F = fgColor;
		r_item.B = bckgColor;
		ok = 2;
	}
	else {
		ColorItem item;
		item.Id = itemID;
		item.F = fgColor;
		item.B = bckgColor;
		ColorAssoc.insert(&item);
		ok = 1;
	}
	return ok;
}

int ListBoxDef::GetItemColor(long itemID, SColor * pFgColor, SColor * pBckgColor) const
{
	int    ok = 0;
	uint   pos = 0;
	if(ColorAssoc.lsearch(&itemID, &pos, CMPF_LONG)) {
		const ColorItem & r_item = ColorAssoc.at(pos);
		ASSIGN_PTR(pFgColor, r_item.F);
		ASSIGN_PTR(pBckgColor, r_item.B);
		ok = 1;
	}
	return ok;
}

StrAssocArray * ListBoxDef::GetListByPattern(const char * pText)
{
	int    srch_mode = srchFirst;
	uint   prev_pos = _curItem();
	long   id = 0L;
	SString buf;
	StrAssocArray * p_list = new StrAssocArray();
	while(search(pText, PTR_CMPFUNC(_PcharNoCase), srch_mode)) {
		srch_mode = srchNext;
		getCurID(&id);
		getCurString(buf);
		p_list->Add(id, 0, buf);
	}
	go(prev_pos);
	return p_list;
}
//
// StdListBoxDef
//
StdListBoxDef::StdListBoxDef(SArray * pArray, uint aOptions, TYPEID t) : ListBoxDef(aOptions, t), P_Data(pArray)
{
	SetCapability(ListBoxDef::cCountable|ListBoxDef::cFullInMem, SlConst::ListBoxDefSignature_STD);
	top();
}

StdListBoxDef::~StdListBoxDef()
{
	if(Options & lbtDisposeData)
		delete P_Data;
}

int    StdListBoxDef::GetFrameSize() const { return GetRecsCount(); }
int    StdListBoxDef::GetFrameState() { return -1; }
bool   StdListBoxDef::IsValid() const { return (P_Data != 0); }
long   StdListBoxDef::GetRecsCount() const { return SVectorBase::GetCount(P_Data); }

int StdListBoxDef::setArray(SArray * pArray)
{
	if(Options & lbtDisposeData)
		delete P_Data;
	P_Data = pArray;
	top();
	return 1;
}

int StdListBoxDef::getIdList(LongArray & rList)
{
	int    ok = -1;
	if(P_Data) {
		for(uint i = 0; i < P_Data->getCount(); i++) {
			const void * p_item = P_Data->at(i);
			rList.addUnique((Options & lbtWordID) ? static_cast<int32>(*static_cast<const int16 *>(p_item)) : *static_cast<const int32 *>(p_item));
			ok = 1;
		}
	}
	return ok;
}

void * FASTCALL StdListBoxDef::getRow_(long r)
{
	return (!P_Data || r >= P_Data->getCountI()) ? 0 : P_Data->at((uint)r);
}

int StdListBoxDef::search(const void * pPattern, CompFunc fcmp, int srchMode)
{
	int    r   = 0;
	int    dir = (srchMode & ~srchFlags);
	long   c   = P_Data->getCount();
	int    h   = (Options & lbtWordID) ? sizeof(int16) : sizeof(int32);
	uint   ofs = (Options & lbtAutoID) ? 0 : h;
	uint   pos;
	if(srchMode & lbSrchByID) {
		if(Options & lbtAutoID) {
			pos = (h == sizeof(int16)) ? (uint)*static_cast<const int16 *>(pPattern) : (uint)*static_cast<const int32 *>(pPattern);
			r = BIN(static_cast<long>(pos) < c);
		}
		else {
			pos = 0;
			if(fcmp == 0)
				fcmp = (h == sizeof(int16)) ? PTR_CMPFUNC(int) : CMPF_LONG;
			r = (srchMode & srchBinary) ? P_Data->bsearch(pPattern, &pos, fcmp) : P_Data->lsearch(pPattern, &pos, fcmp);
		}
	}
	else {
		pos = (dir == srchNext) ? (uint)_curItem() + 1 : 0U;
		SETIFZ(fcmp, substFCMP);
		r = (srchMode & srchBinary) ?
			P_Data->bsearch(pPattern, &pos, fcmp, ofs, reinterpret_cast<void *>(Type)) :
			P_Data->lsearch(pPattern, &pos, fcmp, ofs, reinterpret_cast<void *>(Type));
	}
	if(r)
		go(pos);
	return r;
}
//
//
//
StrAssocListBoxDef::StrAssocListBoxDef(StrAssocArray * pList, uint options) :
	ListBoxDef(options, MKSTYPE(S_ZSTRING, 252)), P_Data(pList)
{
	OneItem.Init();
	SetCapability(ListBoxDef::cCountable|ListBoxDef::cFullInMem, SlConst::ListBoxDefSignature_STRASSOC);
	top();
}

StrAssocListBoxDef::~StrAssocListBoxDef()
{
	OneItem.Destroy();
	if(Options & lbtDisposeData)
		delete P_Data;
}

int StrAssocListBoxDef::search(const void * pPattern, CompFunc fcmp, int srchMode)
{
	int    r   = 0;
	int    dir = (srchMode & ~srchFlags);
	long   c   = P_Data->getCount();
	int    h   = (Options & lbtWordID) ? sizeof(int16) : sizeof(int32);
	uint   ofs = (Options & lbtAutoID) ? 0 : sizeof(int32);
	uint   pos;
	if(srchMode & lbSrchByID) {
		if(Options & lbtAutoID) {
			pos = static_cast<uint>(*static_cast<const int32 *>(pPattern));
			r = BIN(static_cast<long>(pos) < c);
		}
		else {
			pos = 0;
			r = P_Data->Search(*static_cast<const long *>(pPattern), &pos);
		}
	}
	else {
		pos = (dir == srchNext) ? ((uint)_curItem() + 1) : 0U;
		SETIFZ(fcmp, substFCMP);
		r = P_Data->SearchByText(static_cast<const char *>(pPattern), fcmp, &pos);
	}
	if(r)
		go(pos);
	return r;
}

bool   StrAssocListBoxDef::IsValid() const { return (P_Data != 0); }
long   StrAssocListBoxDef::GetRecsCount() const { return P_Data ? P_Data->getCount() : 0; }
int    StrAssocListBoxDef::GetFrameSize() const { return P_Data ? P_Data->getCount() : 0; }
int    StrAssocListBoxDef::GetFrameState() { return -1; }
int    StrAssocListBoxDef::removeItem(long pos) { return (P_Data && pos >= 0) ? P_Data->AtFree(static_cast<uint>(pos)) : 0; }
void   StrAssocListBoxDef::freeAll() { CALLPTRMEMB(P_Data, Z()); }

int StrAssocListBoxDef::getIdList(LongArray & rList)
{
	int    ok = -1;
	if(P_Data) {
		for(uint i = 0; i < P_Data->getCount(); i++) {
			rList.addUnique(P_Data->Get(i).Id);
			ok = 1;
		}
	}
	return ok;
}

void * FASTCALL StrAssocListBoxDef::getRow_(long r)
{
	void * p_row = 0;
	if(P_Data && r < static_cast<long>(P_Data->getCount())) {
		StrAssocArray::Item item = P_Data->at_WithoutParent(r); // at-->at_WithoutParent
		const size_t sz = smax(sizeof(item.Id) + sstrlen(item.Txt) + 1, static_cast<size_t>(512));
		if(OneItem.Alloc(sz)) {
			*reinterpret_cast<long *>(OneItem.P_Buf) = item.Id;
			strnzcpy(OneItem.P_Buf+sizeof(long), item.Txt, 0);
			p_row = OneItem.P_Buf;
		}
	}
	return p_row;
}

int StrAssocListBoxDef::setArray(StrAssocArray * pData)
{
	if(Options & lbtDisposeData)
		delete P_Data;
	P_Data = pData;
	top();
	return 1;
}

int StrAssocListBoxDef::addItem(long id, const char * pStr, long * pPos)
{
	if(P_Data) {
		P_Data->Add(id, pStr);
		ASSIGN_PTR(pPos, P_Data->getCount()-1);
		return 1;
	}
	else
		return -1;
}
//
// StdTreeListBoxDef
//
StdTreeListBoxDef::StdTreeListBoxDef(StrAssocArray * pList, uint aOptions, TYPEID t) : ListBoxDef(aOptions, t), T(sizeof(TreeItem)), P_SaList(0)
{
	SetCapability(ListBoxDef::cTree|ListBoxDef::cFullInMem, SlConst::ListBoxDefSignature_STDTREE);
	setArray(pList);
}

StdTreeListBoxDef::~StdTreeListBoxDef()
{
	if(Options & lbtDisposeData)
		delete P_SaList;
}

int StdTreeListBoxDef::Helper_AddTreeItem(uint idx, UintHashTable & rAddedIdxList, uint32 * pPos)
{
	int    ok = 1;
	if(!rAddedIdxList.Has(idx)) {
		StrAssocArray::Item item_ = P_SaList->Get(idx);
		TreeItem ti;
		ti.Id = item_.Id;
		ti.ParentId = item_.ParentId;
		ti.H = 0;
		ti.P = idx;
		uint32 pos = 0;
		uint32 parent_p = 0;
		if(item_.ParentId) {
			if(!T.Search(&item_.ParentId, &parent_p, CMPF_LONG)) {
				//
				// Если родительский элемент еще не попал в дерево, то
				// рекурсивно вызываем функцию Helper_AddTreeItem для вставки родительского элемента.
				//
				uint   sa_parent_p = 0;
				if(P_SaList->Search(item_.ParentId, &sa_parent_p)) {
					int    r;
					THROW(r = Helper_AddTreeItem(sa_parent_p, rAddedIdxList, &parent_p)); // @recursion
					assert(r > 0); // Не может такого быть, чтобы не обнаруженный до этого элемент, уже был помечен как созданный.
				}
				else
					parent_p = 0;
			}
		}
		THROW(T.Insert(&ti, parent_p, &pos));
		rAddedIdxList.Add(idx);
		ASSIGN_PTR(pPos, pos);
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

int StdTreeListBoxDef::Helper_CreateTree()
{
	int    ok = 1;
	if(P_SaList) {
		UintHashTable added_idx_list;
		for(uint i = 0; i < P_SaList->getCount(); i++) {
			uint32 pos = 0;
			THROW(Helper_AddTreeItem(i, added_idx_list, &pos));
		}
	}
	CATCHZOK
	return ok;
}

int StdTreeListBoxDef::setArray(StrAssocArray * pList)
{
	if(Options & lbtDisposeData) {
		delete P_SaList;
	}
	P_SaList = pList;
	T.Z();
	Helper_CreateTree();
	top();
	return 1;
}

int StdTreeListBoxDef::getCurString(SString & rBuf)
{
	long   cur_id = 0L;
	getCurID(&cur_id);
	SETIFZ(cur_id, CurItem);
	return GetStringByID(cur_id, rBuf);
}

int StdTreeListBoxDef::getCurID(long * pId)
{
	int    ok = 0;
	if(P_SaList) {
		const void * p = getRow_(_curItem());
		if(p) {
			*pId = static_cast<const long *>(p)[0];
			ok = 1;
		}
	}
	return ok;
}

int StdTreeListBoxDef::getCurData(void * b)
{
	if(P_SaList) {
		const void * p = getRow_(_curItem());
		if(p)
			strnzcpy(static_cast<char *>(b), ((const Item *)p)->Txt, 255);
	}
	return 0;
}

bool   StdTreeListBoxDef::IsValid() const { return LOGIC(P_SaList); }
long   StdTreeListBoxDef::GetRecsCount() const { return P_SaList ? P_SaList->getCount() : 0; }

int StdTreeListBoxDef::getIdList(LongArray & rList)
{
	int    ok = -1;
	if(P_SaList) {
		for(uint i = 0; i < P_SaList->getCount(); i++) {
			rList.addUnique(P_SaList->Get(i).Id);
			ok = 1;
		}
	}
	return ok;
}

void * FASTCALL StdTreeListBoxDef::getRow_(long r)
{
	if(P_SaList && r < (long)P_SaList->getCount()) {
		StrAssocArray::Item item = P_SaList->Get(r);
		TempItem.Id = item.Id;
		TempItem.ParentId = item.ParentId;
		STRNSCPY(TempItem.Txt, item.Txt);
		return &TempItem;
	}
	else
		return 0;
}

int StdTreeListBoxDef::go(long pos)
{
	if(IsValid() && pos >= 0 && pos < GetRecsCount()) {
		CurItem = pos;
		return 1;
	}
	else
		return -1;
}

int StdTreeListBoxDef::step(long delta) { return go(CurItem + delta); }

int StdTreeListBoxDef::top()
{
	CurItem = 0;
	return 1;
}

int StdTreeListBoxDef::bottom()
{
	CurItem = GetRecsCount();
	return 1;
}

void StdTreeListBoxDef::getScrollData(long * pScrollDelta, long * pScrollPos)
{
	ASSIGN_PTR(pScrollPos, 0);
	if(P_SaList) {
		uint32 pos = 0;
		const long * p_id = static_cast<const long *>(getRow_(_curItem()));
		if(p_id && *p_id && T.Search(p_id, &pos, CMPF_LONG)) {
			const TreeItem * p_item = static_cast<const TreeItem *>(T.GetData(pos));
			if(p_item)
				ASSIGN_PTR(pScrollPos, (long)p_item->H);
		}
	}
	ASSIGN_PTR(pScrollDelta, 0);
}

int  StdTreeListBoxDef::GetFrameState() { return -1; }
int  StdTreeListBoxDef::GetFrameSize() const { return -1; }
void StdTreeListBoxDef::setViewHight(int h) { ListBoxDef::setViewHight(h); }

int  StdTreeListBoxDef::search(const void * pPattern, CompFunc fcmp, int srchMode)
{
	int    r   = 0;
	if(IsValid()) {
		int    dir = (srchMode & ~srchFlags);
		uint   ofs = sizeof(long);
		uint   pos;
		if(P_SaList) {
			if(srchMode & lbSrchByID) {
				pos = 0;
				r = P_SaList->Search(*static_cast<const long *>(pPattern), &pos);
			}
			else {
				pos = (dir == srchNext) ? (uint)_curItem() + 1 : 0U;
				SETIFZ(fcmp, substFCMP);
				r = P_SaList->SearchByText(static_cast<const char *>(pPattern), fcmp, &pos);
			}
		}
		if(r)
			CurItem = pos;
	}
	return r;
}

int StdTreeListBoxDef::GetStringByID(long id, SString & rBuf)
{
	int    ok = -1;
	uint   pos = 0;
	rBuf.Z();
	if(IsValid()) {
		if(P_SaList) {
			ok = BIN(P_SaList->GetText(id, rBuf));
		}
	}
	return ok;
}

int StdTreeListBoxDef::GoByID(long id)
{
	uint   pos = 0;
	if(IsValid() && P_SaList && P_SaList->Search(id, &pos))
		go(pos);
	return 1;
}

bool FASTCALL StdTreeListBoxDef::HasChildren(long id) const
	{ return P_SaList ? P_SaList->HasChildren(id) : false; }
int StdTreeListBoxDef::GetListByParent(long parentId, LongArray & rList) const
	{ return P_SaList ? P_SaList->GetListByParent(parentId, false, rList) : -1; }
int StdTreeListBoxDef::GetParent(long child, long * pParent) const
	{ return P_SaList ? (P_SaList->GetParent(child, pParent) > 0) : 0; }
//
//
//
StdTreeListBoxDef2_::SearchBlock::SearchBlock() : P_LastResult(0)
{
}

StdTreeListBoxDef2_::SearchBlock::~SearchBlock()
{
	delete P_LastResult;
}

StdTreeListBoxDef2_::SearchBlock & StdTreeListBoxDef2_::SearchBlock::Z()
{
	ZDELETE(P_LastResult);
	LastPattern.Z();
	return *this;
}

StdTreeListBoxDef2_::StdTreeListBoxDef2_(StrAssocTree * pList, uint aOptions, TYPEID t) : ListBoxDef(aOptions, t), /*T(sizeof(TreeItem)),*/ P_SaList(0),
	P_TopLevelRestrictionIdList(0) /* @v11.4.0 */
{
	SetCapability(ListBoxDef::cTree|ListBoxDef::cFullInMem, SlConst::ListBoxDefSignature_STDTREE2);
	setArray(pList);
}

StdTreeListBoxDef2_::~StdTreeListBoxDef2_()
{
	if(Options & lbtDisposeData)
		delete P_SaList;
	delete P_TopLevelRestrictionIdList; // @v11.4.0
}

int StdTreeListBoxDef2_::setArray(StrAssocTree * pList)
{
	if(Options & lbtDisposeData) {
		delete P_SaList;
	}
	Sb.Z();
	P_SaList = pList;
	if(P_SaList) {
		P_SaList->GetList(LL);
		P_SaList->MakeStringIndex(StrIndex);
	}
	//T.Z();
	//Helper_CreateTree();
	top();
	return 1;
}

int StdTreeListBoxDef2_::AddTopLevelRestrictionId(long id) // @v11.4.0
{
	int    ok = 1;
	THROW_S(id, SLERR_INVPARAM);
	THROW(SETIFZ(P_TopLevelRestrictionIdList, new LongArray));
	if(!P_TopLevelRestrictionIdList->lsearch(id))
		P_TopLevelRestrictionIdList->add(id);
	else
		ok = -1;
	CATCHZOK
	return ok;
}

bool StdTreeListBoxDef2_::BelongToTopLevelResriction(long id) const // @v11.4.0
{
	return (!P_TopLevelRestrictionIdList || P_TopLevelRestrictionIdList->lsearch(id));
}

int StdTreeListBoxDef2_::getCurString(SString & rBuf)
{
	long   cur_id = 0L;
	getCurID(&cur_id);
	SETIFZ(cur_id, CurItem);
	return GetStringByID(cur_id, rBuf);
}

int StdTreeListBoxDef2_::getCurID(long * pId)
{
	int    ok = 0;
	if(P_SaList) {
		const void * p = getRow_(_curItem());
		if(p) {
			*pId = static_cast<const long *>(p)[0];
			ok = 1;
		}
	}
	return ok;
}

int StdTreeListBoxDef2_::getCurData(void * b)
{
	if(P_SaList) {
		const void * p = getRow_(_curItem());
		if(p)
			strnzcpy(static_cast<char *>(b), ((const Item *)p)->Txt, 255);
	}
	return 0;
}

bool   StdTreeListBoxDef2_::IsValid() const { return LOGIC(P_SaList); }
long   StdTreeListBoxDef2_::GetRecsCount() const { return P_SaList ? LL.getCount() : 0; }

int StdTreeListBoxDef2_::getIdList(LongArray & rList)
{
	int    ok = -1;
	if(P_SaList) {
		P_SaList->GetList(rList);
		rList.sortAndUndup();
		ok = 1;
	}
	return ok;
}

void * FASTCALL StdTreeListBoxDef2_::getRow_(long r)
{
	void * p_result = 0;
	if(P_SaList && r >= 0 && r < LL.getCountI()) {
		long _id = LL.get(r);
		long _parent_id = 0;
		SString & r_temp_buf = SLS.AcquireRvlStr();
		if(P_SaList->Search(_id, &_parent_id, &r_temp_buf)) {
			TempItem.Id = _id;
			TempItem.ParentId = _parent_id;
			STRNSCPY(TempItem.Txt, r_temp_buf);
			p_result = &TempItem;
		}
		/*
		StrAssocArray::Item item = P_SaList->Get(r);
		TempItem.Id = item.Id;
		TempItem.ParentId = item.ParentId;
		STRNSCPY(TempItem.Txt, item.Txt);
		p_result = &TempItem;
		*/
	}
	return p_result;
}

int StdTreeListBoxDef2_::go(long pos)
{
	if(IsValid() && pos >= 0 && pos < GetRecsCount()) {
		CurItem = pos;
		return 1;
	}
	else
		return -1;
}

int StdTreeListBoxDef2_::step(long delta) { return go(CurItem + delta); }

int StdTreeListBoxDef2_::top()
{
	CurItem = 0;
	return 1;
}

int StdTreeListBoxDef2_::bottom()
{
	CurItem = GetRecsCount();
	return 1;
}

void StdTreeListBoxDef2_::getScrollData(long * pScrollDelta, long * pScrollPos)
{
	ASSIGN_PTR(pScrollPos, 0);
	if(P_SaList) {
		uint   pos = 0;
		long * p_id = static_cast<long *>(getRow_(_curItem()));
		if(p_id && *p_id /*&& LL.lsearch(*p_id, &pos)*/) {
			SPtrHandle h = P_SaList->Search(*p_id);
			if(h) {
				uintptr_t extra = P_SaList->GetNodeExtraPtr(h);
				ASSIGN_PTR(pScrollPos, static_cast<long>(extra));
			}
			/* @todo replace
			TreeItem * p_item = static_cast<TreeItem *>(T.GetData(pos));
			if(p_item)
				ASSIGN_PTR(pScrollPos, (long)p_item->H);
			*/
		}
	}
	ASSIGN_PTR(pScrollDelta, 0);
}

int  StdTreeListBoxDef2_::GetFrameState() { return -1; }
int  StdTreeListBoxDef2_::GetFrameSize() const { return -1; }
void StdTreeListBoxDef2_::setViewHight(int h) { ListBoxDef::setViewHight(h); }

static IMPL_CMPFUNC(INTVECBYINTVER, i1, i2)
{
	const LongArray * p_list = static_cast<const LongArray *>(pExtraData);
	if(p_list) {
		long v1 = *static_cast<const long *>(i1);
		long v2 = *static_cast<const long *>(i2);
		uint pos1 = 0;
		uint pos2 = 0;
		if(p_list->lsearch(v1, &pos1) && p_list->lsearch(v2, &pos2))
			return CMPSIGN(pos1, pos2);
		else
			return 0;
	}
	else
		return 0;
}

int  StdTreeListBoxDef2_::search(const void * pPattern, CompFunc fcmp, int srchMode)
{
	int    r = 0;
	if(IsValid()) {
		int    dir = (srchMode & ~srchFlags);
		uint   ofs = sizeof(long);
		uint   pos = 0;
		if(P_SaList) {
			if(srchMode & lbSrchByID) {
				//r = P_SaList->Search(*(long *)pPattern, &pos);
				r = LL.lsearch(*static_cast<const long *>(pPattern), &pos);
			}
			else if(!isempty(static_cast<const char *>(pPattern))) {
				const char * p_pattern = static_cast<const char *>(pPattern);
				bool search_by_substr = false;
				/*if(p_pattern[0] == '*') {
					p_pattern = p_pattern+1;
					search_by_substr = true;
				}*/
				if(p_pattern[0]) {
					bool do_setup_pos = false;
					TSVector <SPtrHandle> result_list;
					SETIFZ(fcmp, substFCMP);
					/*if(dir == srchFirst && Sb.P_LastResult && Sb.LastPattern != p_pattern) {
						const uint lsrp = Sb.P_LastResult->getPointer();
						Sb.P_LastResult->setPointer(0);
						if(Sb.P_LastResult->getPointer() != lsrp && Sb.P_LastResult->getPointer() < Sb.P_LastResult->getCount()) {
							do_setup_pos = true;
						}
					}
					else*/if(dir == srchFirst || !Sb.P_LastResult || Sb.LastPattern != p_pattern) {
						if(Sb.P_LastResult)
							Sb.P_LastResult->Z();
						else
							Sb.P_LastResult = new LongArray;
						if(P_SaList->SearchByText(static_cast<const char *>(pPattern), fcmp, result_list)) {
							for(uint i = 0; i < result_list.getCount(); i++) {
								long key = P_SaList->GetNodeKey(result_list.at(i));
								Sb.P_LastResult->add(key);
							}
							Sb.P_LastResult->SVector::sort(PTR_CMPFUNC(INTVECBYINTVER), &LL);
							do_setup_pos = true;
						}
					}
					else {
						const uint lsrc = Sb.P_LastResult->getCount();
						if(Sb.P_LastResult->getPointer() < (lsrc-1))
							Sb.P_LastResult->incPointer();
						else
							Sb.P_LastResult->setPointer(0);
						do_setup_pos = true;
					}
					Sb.LastPattern = p_pattern;
					if(do_setup_pos) {
						assert(Sb.P_LastResult->testPointer());
						const uint slrp = Sb.P_LastResult->getPointer();
						long id = Sb.P_LastResult->get(slrp);
						if(LL.lsearch(id, &pos)) {
							r = 1;
						}
					}
				}
			}
		}
		if(r)
			CurItem = pos;
	}
	return r;
}

int StdTreeListBoxDef2_::GetStringByID(long id, SString & rBuf)
{
	int    ok = -1;
	uint   pos = 0;
	rBuf.Z();
	if(IsValid()) {
		if(P_SaList) {
			long strp = 0;
			if(StrIndex.BSearch(id, &strp, 0)) {
				if(P_SaList->GetTextByPos(strp, rBuf))
					ok = 1;
			}
			/*
			long parent_id = 0;
			if(P_SaList->Search(id, &parent_id, &rBuf))
				ok = 1;
			*/
			//ok = BIN(P_SaList->GetText(id, rBuf));
		}
	}
	return ok;
}

int StdTreeListBoxDef2_::GoByID(long id)
{
	uint   pos = 0;
	if(IsValid() && P_SaList && LL.lsearch(id, &pos)) {
		go(pos);
		return 1;
	}
	else
		return 0;
}

bool FASTCALL StdTreeListBoxDef2_::HasChildren(long id) const
	{ return P_SaList ? P_SaList->HasChildren(id) : false; }
int StdTreeListBoxDef2_::GetListByParent(long parentId, LongArray & rList) const
	{ return P_SaList ? P_SaList->GetListByParent(parentId, false, rList) : -1; }
int StdTreeListBoxDef2_::GetParent(long childId, long * pParentId) const
	{ return P_SaList ? P_SaList->Search(childId, pParentId, 0) : 0; }
#if 0 // @v11.2.5 {
//
// StringListBoxDef
//
StringListBoxDef::StringListBoxDef(uint stringSize, uint aOptions) :
	StdListBoxDef(new SArray(sizeof(long) + stringSize), aOptions, MKSTYPE(S_ZSTRING, stringSize))
{
	SetCapability(ListBoxDef::cCountable|ListBoxDef::cFullInMem, LBDEFSIGN_STRING);
}

int StringListBoxDef::addItem(long id, const char * s, long * pPos)
{
	struct {
		long   id;
		char   str[256];
	} entry;
	if(P_Data) {
		uint   p = (pPos && *pPos >= 0 && *pPos < P_Data->getCountI()) ? static_cast<uint>(*pPos) : P_Data->getCount();
		entry.id = id;
		STRNSCPY(entry.str, s);
		if(P_Data->atInsert(p, &entry)) {
			ASSIGN_PTR(pPos, p);
			return 1;
		}
	}
   	return 0;
}

int StringListBoxDef::removeItem(long pos)
	{ return (P_Data && pos >= 0) ? P_Data->atFree(static_cast<uint>(pos)) : 0; }
void StringListBoxDef::freeAll()
	{ CALLPTRMEMB(P_Data, freeAll()); }
#endif // } 0 @v11.2.5
//
// DBQListBoxDef
//
DBQListBoxDef::DBQListBoxDef(DBQuery & rQuery, uint aOptions, uint aBufSize) : ListBoxDef(aOptions, 0), query(0)
{
	SetCapability(0, SlConst::ListBoxDefSignature_DBQ);
	setQuery(rQuery, aBufSize);
}

DBQListBoxDef::~DBQListBoxDef()
{
	if(Options & lbtDisposeData)
		delete query;
}

int DBQListBoxDef::GetFrameSize() const { return query->P_Frame->CountF; }

int DBQListBoxDef::GetFrameState()
{
	int    st = query->P_Frame->State;
	return (st == 3) ? -1 : st;
}

int DBQListBoxDef::setQuery(DBQuery & rQuery, uint aBufSize)
{
	if(Options & lbtDisposeData)
		delete query;
	query = &rQuery;
	if(query) {
		query->setFrame(1, aBufSize, 1);
 		int    c;
		if(Options & lbtAutoID)
			c = 0;
		else {
			c = 1;
			TYPEID t = query->flds[0].type;
			if(t == MKSTYPE(S_INT, 2) || t == MKSTYPE(S_AUTOINC, 2))
				Options |= lbtWordID;
			else if(t == MKSTYPE(S_INT, 4) || t == MKSTYPE(S_AUTOINC, 4))
				Options &= ~lbtWordID;
		}
		assert((int)query->fldCount > c);
		Type = query->flds[c].type;
		query->P_Frame->SRange = 1000;
		query->top();
		return 1;
	}
	return 0;
}

int    DBQListBoxDef::setRestrict(DBQ & rQ) { return query ? (query->where(rQ), query->top(), 1) : 0; }
bool   DBQListBoxDef::IsValid() const { return (query && !query->error); }
int    DBQListBoxDef::go(long p) { return step(p-CurItem); }

void DBQListBoxDef::setViewHight(int h)
{
	CALLPTRMEMB(query, setFrame(h, UNDEF, UNDEF));
	ListBoxDef::setViewHight(h);
}

void DBQListBoxDef::getScrollData(long * pScrollDelta, long * pScrollPos)
{
	*pScrollDelta = query ? (long)query->P_Frame->SDelta : 0;
	*pScrollPos = query ? (long)query->P_Frame->SPos : 0;
}

void DBQListBoxDef::setupView()
{
	TopItem = query ? query->P_Frame->Top : 0;
	CurItem = query ? query->P_Frame->Cur : 0;
}

int DBQListBoxDef::step(long d)
{
	int    ok = 0;
	if(query) {
		ok = query->step(d);
		setupView();
	}
	return ok;
}

int DBQListBoxDef::top()
{
	int    ok = 0;
	if(query) {
		ok = query->top();
		setupView();
	}
	return ok;
}

int DBQListBoxDef::bottom()
{
	int    ok = 0;
	if(query) {
		ok = query->bottom();
		setupView();
	}
	return ok;
}

int DBQListBoxDef::refresh()
{
	int    ok = 0;
	if(query) {
		ok = query->refresh();
		setupView();
	}
	return 0;
}

long  DBQListBoxDef::GetRecsCount() const { return query ? (query->P_Frame->SRange+1) : 0; }

int DBQListBoxDef::getIdList(LongArray & rList)
{
	// @todo Реализовать метод
	return -1;
}

void * FASTCALL DBQListBoxDef::getRow_(long r)
{
	return query ? query->getRecord((uint)r) : 0;
}

int DBQListBoxDef::TransmitData(int dir, void * pData)
{
	int    s = ListBoxDef::TransmitData(dir, pData);
	if(dir > 0 && s)
		query->P_Frame->SPos = (uint)GetRecsCount() / 2;
	return s;
}

int DBQListBoxDef::search(const void * pPattern, CompFunc fcmp, int srchMode)
{
	int    ok = 1;
	int    f;
	void * p_srch_extra = 0;
	if(srchMode & lbSrchByID) {
		if(!(Options & lbtAutoID)) {
			p_srch_extra = reinterpret_cast<void *>(((Options & lbtWordID) ? MKSTYPE(S_INT, 2) : MKSTYPE(S_INT, 4)));
			SETIFZ(fcmp, substFCMP);
			f = 0;
		}
		else
			ok = 0;
	}
	else {
		p_srch_extra = reinterpret_cast<void *>(Type);
		SETIFZ(fcmp, substFCMP);
		f = (Options & lbtAutoID) ? 0 : 1;
	}
	if(ok) {
		if((ok = query->search(pPattern, fcmp, f, srchMode, p_srch_extra)) != 0)
			query->P_Frame->SPos = (uint)GetRecsCount() / 2;
		setupView();
	}
	return ok;
}
