// LBXDEF.CPP
// Copyright (c) Sobolev A. 1996, 1997, 2000, 2003, 2005, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2015, 2016, 2017, 2018, 2019
//
#include <slib.h>
#include <tv.h>
#pragma hdrstop

static int FASTCALL substFCMP(const void * i1, const void * i2, void * pExtraData)
	{ return stcomp((TYPEID)pExtraData, (const void *)i1, (const void *)i2); }

#define ILLEGAL_LISTBOX_BUF_TYPE 0
//
// ListBoxDef
//
SLAPI ListBoxDef::ListBoxDef(uint aOptions, TYPEID aType) : Options(aOptions), Format(0), Type(aType), 
	ViewHight(1), TopItem(0), CurItem(0), CFlags(0), ScrollDelta(0)
{
	SetCapability(0, LBDEFSIGN_DEFAULT);
	UserData.Init();
}

SLAPI ListBoxDef::~ListBoxDef()
{
	Sign = 0; // Помечаем instance как разрушенный
	UserData.Destroy();
}

void ListBoxDef::SetCapability(long c, uint32 sign)
{
	CFlags |= c;
	Sign = sign;
}

int SLAPI ListBoxDef::SetUserData(const void * pData, size_t size)
{
	if(UserData.Alloc(size)) {
		memcpy(UserData.P_Buf, pData, size);
		return 1;
	}
	else
		return 0;
}

int SLAPI ListBoxDef::GetUserData(void * pData, size_t * pSize) const
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

void SLAPI ListBoxDef::getScrollData(long * pScrollDelta, long * pScrollPos)
{
	ASSIGN_PTR(pScrollDelta, ScrollDelta);
	ASSIGN_PTR(pScrollPos, CurItem);
}

const char * SLAPI ListBoxDef::getText(long item, SString & rBuf)
{
	const char * p = (const char *)getRow_(item);
	if(p) {
		uint   h = (Options & lbtAutoID) ? 0 : ((Options & lbtWordID) ? sizeof(int16) : sizeof(int32));
		char   temp_buf[1024];
		sttostr(Type, p+h, Format, temp_buf);
		rBuf = temp_buf;
	}
	else
		rBuf.Z();
	return rBuf;
}

int SLAPI ListBoxDef::getCurID(long * pId)
{
	union {
		void * vp;
		long * lp;
		int  * ip;
	};
	if(Options & lbtAutoID)
		*pId = _curItem();
	else if((vp = getRow_(_curItem())) != 0)
		*pId = (Options & lbtWordID) ? (long)*ip : *lp;
	else
		return 0;
	return 1;
}

int SLAPI ListBoxDef::getCurString(SString & rBuf)
{
	getText(_curItem(), rBuf);
	return 1;
}

int SLAPI ListBoxDef::getCurData(void * b)
{
	char * p = (char *)getRow_(_curItem());
	if(p) {
		if(!(Options & lbtAutoID))
			p += (Options & lbtWordID) ? sizeof(int16) : sizeof(int32);
		memcpy(b, p, stsize(Type));
		return 1;
	}
	else
		return 0;
}

void   SLAPI ListBoxDef::SetOption(uint option, int set) { SETFLAG(Options, option, set); }
long   SLAPI ListBoxDef::_topItem() const { return TopItem; }
long   SLAPI ListBoxDef::_curItem() const { return CurItem; }
int    SLAPI ListBoxDef::_isTreeList() const { return BIN(CFlags & cTree); }
void   SLAPI ListBoxDef::setViewHight(int vh) { ViewHight = vh; }
int    SLAPI ListBoxDef::GetFrameSize() { return -1; }
int    SLAPI ListBoxDef::GetFrameState() { return 0; }
int    SLAPI ListBoxDef::getIdList(LongArray &) { return -1; }
int    SLAPI ListBoxDef::search(const void *, CompFunc, int srchMode) { return 0; }
int    SLAPI ListBoxDef::valid() { return 1; }
int    SLAPI ListBoxDef::addItem(long id, const char *, long * pPos) { return -1; }
int    SLAPI ListBoxDef::removeItem(long pos) { return -1; }
void   SLAPI ListBoxDef::freeAll() {}
int    SLAPI ListBoxDef::step(long delta) { return go(CurItem + delta); }
int    SLAPI ListBoxDef::top() { return go(0); }
int    SLAPI ListBoxDef::bottom() { return go(getRecsCount()-1); }
long   SLAPI ListBoxDef::getRecsCount() { return 0; }
void * FASTCALL ListBoxDef::getRow_(long) { return 0; }
int    SLAPI ListBoxDef::refresh() { return 1; }
int    SLAPI ListBoxDef::GetImageIdxByID(long id, long * pIDx) { return ImageIdxAssoc.Search(id, pIDx, 0); }
int    SLAPI ListBoxDef::AddImageAssoc(long itemID, long imageID) { return ImageAssoc.Add(itemID, (imageID & ~0x40000000), 0); }
int    SLAPI ListBoxDef::AddVecImageAssoc(long itemID, long imageID) { return ImageAssoc.Add(itemID, (imageID | 0x40000000), 0); }
int    SLAPI ListBoxDef::HasItemColorSpec() const { return BIN(ColorAssoc.getCount()); }

#define RANGE(val,min,max) ((val)<(min)?(min):((val)>(max)?(max):(val)))

int SLAPI ListBoxDef::go(long row)
{
	if(valid()) {
		long   c = getRecsCount();
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
			s = (int)stsize(Type);
		else
			s = (Options & lbtWordID) ? sizeof(int16) : sizeof(int32);
	}
	else if(dir > 0) {
		s = (Options & lbtNoData || !pData) ? 0 : search(pData, 0, (Options & lbtTransferData) ? srchFirst : srchFirst | lbSrchByID);
	}
	else if(dir < 0) {
		if(!(Options & lbtNoData))
			if(Options & lbtTransferData)
				getCurData(pData);
			else {
				long   i = 0;
				if(!getCurID(&i))
					i = 0;
				if(Options & lbtWordID)
					*(short*)pData = (short)i;
				else
					*(long*)pData = i;
			}
		s = 1;
	}
	return s;
}

HIMAGELIST SLAPI ListBoxDef::CreateImageList(HINSTANCE hInst)
{
	HIMAGELIST himl = NULL;
	LAssocArray added_list;
	ImageIdxAssoc.freeAll();
	if(ImageAssoc.getCount()) {
		himl = ImageList_Create(GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), ILC_COLOR8, 16, 64);
		for(uint i = 0; i < ImageAssoc.getCount(); i++) {
			long item_id  = ImageAssoc.at(i).Key;
			long image_id = ImageAssoc.at(i).Val;
			long image_idx = -1;
			if(added_list.Search(image_id, &image_idx, 0) <= 0) {
				if(image_id & 0x40000000) {
					{
						//SETIFZ(dv_id, PPDV_TESTFLOWER);
						TWhatmanToolArray::Item tool_item;
						//const SDrawFigure * p_fig = APPL->DvToolList.GetFigById(1, dv_id, &tool_item);
						const SDrawFigure * p_fig = APPL->LoadDrawFigureById((image_id & ~0x40000000), &tool_item);
						if(p_fig) {
							const uint _w = 16;
							const uint _h = 16;
							SImageBuffer ib(_w, _h);
							{
								//p_fig->TransformToImage(0, ib);
								TCanvas2 canv(APPL->GetUiToolBox(), ib);
								//canv.SetBkColor(GetColorRef(SClrWhite));
								//canv.Fill(SClrWhite, 1);
								//canv.SetBkTranparent();
								if(!tool_item.ReplacedColor.IsEmpty()) {
									SColor replacement_color;
									replacement_color = APPL->GetUiToolBox().GetColor(TProgram::tbiIconRegColor);
									canv.SetColorReplacement(tool_item.ReplacedColor, replacement_color);
								}
								FRect pic_bounds;
								pic_bounds.a.X = 0.0f;
								pic_bounds.a.Y = 0.0f;
								pic_bounds.b.X = (float)_w;
								pic_bounds.b.Y = (float)_h;
								//
								LMatrix2D mtx;
								SViewPort vp;
								
								canv.Rect(pic_bounds);
								canv.Fill(SColor(255, 255, 255, 255), 0); // Прозрачный фон

								canv.PushTransform();
								p_fig->GetViewPort(&vp);
								canv.AddTransform(vp.GetMatrix(pic_bounds, mtx));
								canv.Draw(p_fig);
								//canv.PopTransform();
								//canv.ResetColorReplacement();
							}
							HICON h_icon = (HICON)ib.TransformToIcon();
							if(h_icon) {
								image_idx = ImageList_AddIcon(himl, h_icon);
								added_list.Add(image_id, image_idx, 0);
								DestroyIcon(h_icon);
							}
						}
					}
				}
				else {
					HICON h_icon = LoadIcon(hInst, MAKEINTRESOURCE(image_id));
					image_idx = ImageList_AddIcon(himl, h_icon);
					added_list.Add(image_id, image_idx, 0);
					DestroyIcon(h_icon);
				}
			}
			if(image_idx >= 0)
				ImageIdxAssoc.Add(item_id, image_idx, 0);
		}
	}
	return himl;
}

void SLAPI ListBoxDef::ClearImageAssocList()
{
	ImageAssoc.freeAll();
	ImageIdxAssoc.freeAll();
}

int SLAPI ListBoxDef::ResetItemColor(long itemID)
{
	int    ok = -1;
	uint   pos = 0;
	if(ColorAssoc.lsearch(&itemID, &pos, CMPF_LONG)) {
		ColorAssoc.atFree(pos);
		ok = 1;
	}
	return ok;
}

int SLAPI ListBoxDef::SetItemColor(long itemID, SColor fgColor, SColor bckgColor)
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

int SLAPI ListBoxDef::GetItemColor(long itemID, SColor * pFgColor, SColor * pBckgColor) const
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
SLAPI StdListBoxDef::StdListBoxDef(SArray * pArray, uint aOptions, TYPEID t) : ListBoxDef(aOptions, t), P_Data(pArray)
{
	SetCapability(ListBoxDef::cCountable|ListBoxDef::cFullInMem, LBDEFSIGN_STD);
	top();
}

SLAPI StdListBoxDef::~StdListBoxDef()
{
	if(Options & lbtDisposeData)
		delete P_Data;
}

int    SLAPI StdListBoxDef::GetFrameSize() { return (int)getRecsCount(); }
int    SLAPI StdListBoxDef::GetFrameState() { return -1; }
int    SLAPI StdListBoxDef::valid() { return (P_Data != 0); }
long   SLAPI StdListBoxDef::getRecsCount() { return SVectorBase::GetCount(P_Data); }

int SLAPI StdListBoxDef::setArray(SArray * pArray)
{
	if(Options & lbtDisposeData)
		delete P_Data;
	P_Data = pArray;
	top();
	return 1;
}

int SLAPI StdListBoxDef::getIdList(LongArray & rList)
{
	int    ok = -1;
	if(P_Data) {
		for(uint i = 0; i < P_Data->getCount(); i++) {
			const void * p_item = P_Data->at(i);
			rList.addUnique((Options & lbtWordID) ? (int32)*(int16 *)p_item : *(int32 *)p_item);
			ok = 1;
		}
	}
	return ok;
}

void * FASTCALL StdListBoxDef::getRow_(long r)
{
	return (!P_Data || r >= (long)P_Data->getCount()) ? 0 : P_Data->at((uint)r);
}

int SLAPI StdListBoxDef::search(const void * pPattern, CompFunc fcmp, int srchMode)
{
	int    r   = 0;
	int    dir = (srchMode & ~srchFlags);
	long   c   = P_Data->getCount();
	int    h   = (Options & lbtWordID) ? sizeof(int16) : sizeof(int32);
	uint   ofs = (Options & lbtAutoID) ? 0 : h;
	uint   pos;
	if(srchMode & lbSrchByID) {
		if(Options & lbtAutoID) {
			pos = (h == sizeof(int16)) ? (uint)*(const int16 *)pPattern : (uint)*(const int32 *)pPattern;
			r = BIN((long)pos < c);
		}
		else {
			pos = 0;
			if(fcmp == 0)
				fcmp = (h == sizeof(int16)) ? PTR_CMPFUNC(int) : CMPF_LONG;
			r = (srchMode & srchBinary) ?
				P_Data->bsearch(pPattern, &pos, fcmp) : P_Data->lsearch(pPattern, &pos, fcmp);
		}
	}
	else {
		pos = (dir == srchNext) ? (uint)_curItem() + 1 : 0U;
		SETIFZ(fcmp, substFCMP);
		r = (srchMode & srchBinary) ?
			P_Data->bsearch(pPattern, &pos, fcmp, ofs, (void *)Type) : P_Data->lsearch(pPattern, &pos, fcmp, ofs, (void *)Type);
	}
	if(r)
		go(pos);
	return r;
}
//
//
//
SLAPI StrAssocListBoxDef::StrAssocListBoxDef(StrAssocArray * pList, uint options) :
	ListBoxDef(options, MKSTYPE(S_ZSTRING, 252)), P_Data(pList)
{
	OneItem.Init();
	SetCapability(ListBoxDef::cCountable|ListBoxDef::cFullInMem, LBDEFSIGN_STRASSOC);
	top();
}

SLAPI StrAssocListBoxDef::~StrAssocListBoxDef()
{
	OneItem.Destroy();
	if(Options & lbtDisposeData)
		delete P_Data;
}

int SLAPI StrAssocListBoxDef::search(const void * pPattern, CompFunc fcmp, int srchMode)
{
	int    r   = 0;
	int    dir = (srchMode & ~srchFlags);
	long   c   = P_Data->getCount();
	int    h   = (Options & lbtWordID) ? sizeof(int16) : sizeof(int32);
	uint   ofs = (Options & lbtAutoID) ? 0 : sizeof(int32);
	uint   pos;
	if(srchMode & lbSrchByID) {
		if(Options & lbtAutoID) {
			pos = (uint)*(const int32 *)pPattern;
			r = BIN((long)pos < c);
		}
		else {
			pos = 0;
			r = P_Data->Search(*(const long *)pPattern, &pos);
		}
	}
	else {
		pos = (dir == srchNext) ? ((uint)_curItem() + 1) : 0U;
		SETIFZ(fcmp, substFCMP);
		r = P_Data->SearchByText((const char *)pPattern, fcmp, &pos);
	}
	if(r)
		go(pos);
	return r;
}

int    SLAPI StrAssocListBoxDef::valid() { return (P_Data != 0); }
long   SLAPI StrAssocListBoxDef::getRecsCount() { return P_Data ? P_Data->getCount() : 0; }
int    SLAPI StrAssocListBoxDef::GetFrameSize() { return P_Data ? P_Data->getCount() : 0; }
int    SLAPI StrAssocListBoxDef::GetFrameState() { return -1; }
int    SLAPI StrAssocListBoxDef::removeItem(long pos) { return (P_Data && pos >= 0) ? P_Data->AtFree((uint)pos) : 0; }
void   SLAPI StrAssocListBoxDef::freeAll() { CALLPTRMEMB(P_Data, Z()); }

int SLAPI StrAssocListBoxDef::getIdList(LongArray & rList)
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
	if(P_Data && r < (long)P_Data->getCount()) {
		StrAssocArray::Item item = P_Data->at_WithoutParent(r); // at-->at_WithoutParent
		size_t sz = sizeof(item.Id) + sstrlen(item.Txt) + 1;
		sz = MAX(sz, 512);
		if(OneItem.Alloc(sz)) {
			*(long *)OneItem.P_Buf = item.Id;
			strnzcpy(OneItem.P_Buf+sizeof(long), item.Txt, 0);
			p_row = OneItem.P_Buf;
		}
	}
	return p_row;
}

int SLAPI StrAssocListBoxDef::setArray(StrAssocArray * pData)
{
	if(Options & lbtDisposeData)
		delete P_Data;
	P_Data = pData;
	top();
	return 1;
}

int SLAPI StrAssocListBoxDef::addItem(long id, const char * pStr, long * pPos)
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
SLAPI StdTreeListBoxDef::StdTreeListBoxDef(StrAssocArray * pList, uint aOptions, TYPEID t) : ListBoxDef(aOptions, t), T(sizeof(TreeItem)), P_SaList(0)
{
	SetCapability(ListBoxDef::cTree|ListBoxDef::cFullInMem, LBDEFSIGN_STDTREE);
	setArray(pList);
}

SLAPI StdTreeListBoxDef::~StdTreeListBoxDef()
{
	if(Options & lbtDisposeData)
		delete P_SaList;
}

int SLAPI StdTreeListBoxDef::Helper_AddTreeItem(uint idx, UintHashTable & rAddedIdxList, uint32 * pPos)
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

int SLAPI StdTreeListBoxDef::Helper_CreateTree()
{
	int    ok = 1;
	if(P_SaList) {
		UintHashTable added_idx_list;
		for(uint i = 0; i < P_SaList->getCount(); i++) {
			//StrAssocArray::Item item_ = P_SaList->at(i); // @debug
			uint32 pos = 0;
			THROW(Helper_AddTreeItem(i, added_idx_list, &pos));
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI StdTreeListBoxDef::setArray(StrAssocArray * pList)
{
	if(Options & lbtDisposeData) {
		delete P_SaList;
	}
	P_SaList = pList;
	T.Clear();
	Helper_CreateTree();
	top();
	return 1;
}

int SLAPI StdTreeListBoxDef::getCurString(SString & rBuf)
{
	long   cur_id = 0L;
	getCurID(&cur_id);
	SETIFZ(cur_id, CurItem);
	return GetStringByID(cur_id, rBuf);
}

int SLAPI StdTreeListBoxDef::getCurID(long * pId)
{
	int    ok = 0;
	if(P_SaList) {
		const void * p = (const void *)getRow_(_curItem());
		if(p) {
			*pId = ((long *)p)[0];
			ok = 1;
		}
	}
	return ok;
}

int SLAPI StdTreeListBoxDef::getCurData(void * b)
{
	if(P_SaList) {
		const void * p = (const void *)getRow_(_curItem());
		if(p)
			strnzcpy((char *)b, ((const Item *)p)->Txt, 255);
	}
	return 0;
}

int  SLAPI StdTreeListBoxDef::valid() { return BIN(P_SaList); }
long SLAPI StdTreeListBoxDef::getRecsCount() { return P_SaList ? P_SaList->getCount() : 0; }

int SLAPI StdTreeListBoxDef::getIdList(LongArray & rList)
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

int SLAPI StdTreeListBoxDef::go(long pos)
{
	if(valid() && pos >= 0 && pos < getRecsCount()) {
		CurItem = pos;
		return 1;
	}
	else
		return -1;
}

int SLAPI StdTreeListBoxDef::step(long delta) { return go(CurItem + delta); }

int SLAPI StdTreeListBoxDef::top()
{
	CurItem = 0;
	return 1;
}

int SLAPI StdTreeListBoxDef::bottom()
{
	CurItem = getRecsCount();
	return 1;
}

void SLAPI StdTreeListBoxDef::getScrollData(long * pScrollDelta, long * pScrollPos)
{
	ASSIGN_PTR(pScrollPos, 0);
	if(P_SaList) {
		uint32 pos = 0;
		long * p_id = (long *)getRow_(_curItem());
		if(p_id && *p_id && T.Search(p_id, &pos, CMPF_LONG)) {
			TreeItem * p_item = (TreeItem *)T.GetData(pos);
			if(p_item)
				ASSIGN_PTR(pScrollPos, (long)p_item->H);
		}
	}
	ASSIGN_PTR(pScrollDelta, 0);
}

int  SLAPI StdTreeListBoxDef::GetFrameState() { return -1; }
int  SLAPI StdTreeListBoxDef::GetFrameSize() { return -1; }
void SLAPI StdTreeListBoxDef::setViewHight(int h) { ListBoxDef::setViewHight(h); }

int  SLAPI StdTreeListBoxDef::search(const void * pPattern, CompFunc fcmp, int srchMode)
{
	int    r   = 0;
	if(valid()) {
		int    dir = (srchMode & ~srchFlags);
		uint   ofs = sizeof(long);
		uint   pos;
		if(P_SaList) {
			if(srchMode & lbSrchByID) {
				pos = 0;
				r = P_SaList->Search(*(long *)pPattern, &pos);
			}
			else {
				pos = (dir == srchNext) ? (uint)_curItem() + 1 : 0U;
				SETIFZ(fcmp, substFCMP);
				r = P_SaList->SearchByText((const char *)pPattern, fcmp, &pos);
			}
		}
		if(r)
			CurItem = pos;
	}
	return r;
}

int SLAPI StdTreeListBoxDef::GetStringByID(long id, SString & rBuf)
{
	int    ok = -1;
	uint   pos = 0;
	rBuf.Z();
	if(valid()) {
		if(P_SaList) {
			ok = BIN(P_SaList->GetText(id, rBuf));
		}
	}
	return ok;
}

int SLAPI StdTreeListBoxDef::GoByID(long id)
{
	uint   pos = 0;
	if(valid() && P_SaList && P_SaList->Search(id, &pos))
		go(pos);
	return 1;
}

int FASTCALL StdTreeListBoxDef::HasChild(long id) const
	{ return P_SaList ? P_SaList->HasChild(id) : 0; }
int SLAPI StdTreeListBoxDef::GetListByParent(long parentId, LongArray & rList) const
	{ return P_SaList ? P_SaList->GetListByParent(parentId, 0, rList) : -1; }
int SLAPI StdTreeListBoxDef::GetParent(long child, long * pParent) const
	{ return P_SaList ? (P_SaList->GetParent(child, pParent) > 0) : 0; }
//
// StringListBoxDef
//
SLAPI StringListBoxDef::StringListBoxDef(uint stringSize, uint aOptions) :
	StdListBoxDef(new SArray(sizeof(long) + stringSize), aOptions, MKSTYPE(S_ZSTRING, stringSize))
{
	SetCapability(ListBoxDef::cCountable|ListBoxDef::cFullInMem, LBDEFSIGN_STRING);
}

int SLAPI StringListBoxDef::addItem(long id, const char * s, long * pPos)
{
	struct {
		long   id;
		char   str[256];
	} entry;
	if(P_Data) {
		uint   p = (pPos && *pPos >= 0 && *pPos < (long)P_Data->getCount()) ? (uint)*pPos : P_Data->getCount();
		entry.id = id;
		STRNSCPY(entry.str, s);
		if(P_Data->atInsert(p, &entry)) {
			ASSIGN_PTR(pPos, p);
			return 1;
		}
	}
   	return 0;
}

int SLAPI StringListBoxDef::removeItem(long pos)
	{ return (P_Data && pos >= 0) ? P_Data->atFree((uint)pos) : 0; }
void SLAPI StringListBoxDef::freeAll()
	{ CALLPTRMEMB(P_Data, freeAll()); }
//
// DBQListBoxDef
//
SLAPI DBQListBoxDef::DBQListBoxDef(DBQuery & rQuery, uint aOptions, uint aBufSize) : ListBoxDef(aOptions, 0), query(0)
{
	SetCapability(0, LBDEFSIGN_DBQ);
	setQuery(rQuery, aBufSize);
}

SLAPI DBQListBoxDef::~DBQListBoxDef()
{
	if(Options & lbtDisposeData)
		delete query;
}

int SLAPI DBQListBoxDef::GetFrameSize()
{
	return query->P_Frame->count;
}

int SLAPI DBQListBoxDef::GetFrameState()
{
	int    st = query->P_Frame->state;
	return (st == 3) ? -1 : st;
}

int SLAPI DBQListBoxDef::setQuery(DBQuery & rQuery, uint aBufSize)
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
		query->P_Frame->srange = 1000;
		query->top();
		return 1;
	}
	return 0;
}

int SLAPI DBQListBoxDef::setRestrict(DBQ & rQ)
{
	return query ? (query->where(rQ), query->top(), 1) : 0;
}

void SLAPI DBQListBoxDef::setViewHight(int h)
{
	CALLPTRMEMB(query, setFrame(h, UNDEF, UNDEF));
	ListBoxDef::setViewHight(h);
}

void SLAPI DBQListBoxDef::getScrollData(long * pScrollDelta, long * pScrollPos)
{
	*pScrollDelta = query ? (long)query->P_Frame->sdelta : 0;
	*pScrollPos = query ? (long)query->P_Frame->spos : 0;
}

int SLAPI DBQListBoxDef::valid()
{
	return (query && !query->error);
}

void SLAPI DBQListBoxDef::setupView()
{
	TopItem = query ? query->P_Frame->top : 0;
	CurItem = query ? query->P_Frame->cur : 0;
}

int SLAPI DBQListBoxDef::go(long p)
{
	return step(p-CurItem);
}

int SLAPI DBQListBoxDef::step(long d)
{
	int    ok = 0;
	if(query) {
		ok = query->step(d);
		setupView();
	}
	return ok;
}

int SLAPI DBQListBoxDef::top()
{
	int    ok = 0;
	if(query) {
		ok = query->top();
		setupView();
	}
	return ok;
}

int SLAPI DBQListBoxDef::bottom()
{
	int    ok = 0;
	if(query) {
		ok = query->bottom();
		setupView();
	}
	return ok;
}

int SLAPI DBQListBoxDef::refresh()
{
	int    ok = 0;
	if(query) {
		ok = query->refresh();
		setupView();
	}
	return 0;
}

long  SLAPI DBQListBoxDef::getRecsCount()
{
	return query ? (query->P_Frame->srange+1) : 0;
}

int SLAPI DBQListBoxDef::getIdList(LongArray & rList)
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
		query->P_Frame->spos = (uint)getRecsCount() / 2;
	return s;
}

int SLAPI DBQListBoxDef::search(const void * pPattern, CompFunc fcmp, int srchMode)
{
	int    ok = 1;
	int    f;
	void * p_srch_extra = 0;
	if(srchMode & lbSrchByID) {
		if(!(Options & lbtAutoID)) {
			p_srch_extra = (void *)(((Options & lbtWordID) ? MKSTYPE(S_INT, 2) : MKSTYPE(S_INT, 4)));
			SETIFZ(fcmp, substFCMP);
			f = 0;
		}
		else
			ok = 0;
	}
	else {
		p_srch_extra = (void *)Type;
		SETIFZ(fcmp, substFCMP);
		f = (Options & lbtAutoID) ? 0 : 1;
	}
	if(ok) {
		if((ok = query->search(pPattern, fcmp, f, srchMode, p_srch_extra)) != 0)
			query->P_Frame->spos = (uint)getRecsCount() / 2;
		setupView();
	}
	return ok;
}
