// STCHBRW.CPP
// Copyright (c) A.Sobolev 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2015, 2016, 2017, 2018
// @codepage UTF-8
// TimeChunkBrowser
//
#include <slib.h>
#include <tv.h>
#pragma hdrstop
//
//
//
STimeChunkGrid::HolidayArray::HolidayArray(long id) : Id(id)
{
}

STimeChunkGrid::STimeChunkGrid() : TSCollection <STimeChunkAssocArray> (), P_CollapseList(0)
{
}

STimeChunkGrid::~STimeChunkGrid()
{
	delete P_CollapseList;
}

STimeChunkGrid & FASTCALL STimeChunkGrid::operator = (const STimeChunkGrid & s)
{
	if(&s) {
		freeAll();
		NameList = s.NameList;
		for(uint i = 0; i < s.getCount(); i++) {
			const STimeChunkAssocArray * p_row = s.at(i);
			STimeChunkAssocArray * p_new_row = new STimeChunkAssocArray(p_row->Id);
			*p_new_row = *p_row;
			p_new_row->Id = p_row->Id;
			insert(p_new_row);
		}
	}
	return *this;
}

STimeChunkAssocArray * STimeChunkGrid::Get(long id, uint * pPos) const
{
	uint   pos = 0;
	if(lsearch(&id, &pos, CMPF_LONG, offsetof(STimeChunkAssocArray, Id))) {
		ASSIGN_PTR(pPos, pos);
		return at(pos);
	}
	else
		return 0;
}

int STimeChunkGrid::GetChunk(long chunkId, long * pRowId, STimeChunkAssoc * pItem) const
{
	int    ok = 0;
	for(uint i = 0; !ok && i < getCount(); i++) {
		const STimeChunkAssocArray * p_row = at(i);
		if(p_row->Get(chunkId, 0, pItem)) {
			ASSIGN_PTR(pRowId, p_row->Id);
			ok = 1;
		}
	}
	return ok;
}

int STimeChunkGrid::GetChunksByTime(const STimeChunk & rRange, STimeChunkAssocArray & rList) const
{
	int    ok = -1;
	for(uint i = 0; i < getCount(); i++) {
		const STimeChunkAssocArray * p_row = at(i);
		for(uint j = 0; j < p_row->getCount(); j++) {
			const STimeChunkAssoc * p_item = (const STimeChunkAssoc *)p_row->at(j);
			STimeChunk intersection;
			if(p_item->Chunk.Intersect(rRange, &intersection) > 0) {
				const long _dur = intersection.GetDuration();
				if(_dur != 0) {
					rList.Add(p_item->Id, p_item->Status, &p_item->Chunk, 0);
					ok = 1;
				}
			}
		}
	}
	return ok;
}

int STimeChunkGrid::SetRow(STimeChunkAssocArray * pArray, const char * pText)
{
	if(pArray) {
		uint   pos = 0;
		if(pArray->Id && Get(pArray->Id, &pos))
			atFree(pos);
		else
			pos = getCount();
		atInsert(pos, pArray);
		if(pText && pArray->Id)
			NameList.Add(pArray->Id, pText, 1);
	}
	return 1;
}

int STimeChunkGrid::SetRowText(long rowID, const char * pText, int replace)
{
	return NameList.Add(rowID, pText, replace);
}

int STimeChunkGrid::RemoveRow(long rowID)
{
	uint   pos = 0;
	if(lsearch(&rowID, &pos, CMPF_LONG, offsetof(STimeChunkAssocArray, Id))) {
		atFree(pos);
		return 1;
	}
	else
		return 0;
}

int STimeChunkGrid::RemoveChunk(long rowID, long chunkID)
{
	STimeChunkAssocArray * p_list = Get(rowID, 0);
	return (p_list && p_list->Remove(chunkID)) ? 1 : -1;
}

int STimeChunkGrid::SetChunk(long rowID, long chunkID, long status, const STimeChunk * pChunk)
{
	int    ok = 1;
	STimeChunkAssocArray * p_list = Get(rowID, 0);
	if(!p_list) {
		p_list = new STimeChunkAssocArray(rowID);
		insert(p_list);
		ok = 2;
	}
	if(p_list) {
		STimeChunkAssoc item;
		item.Id = chunkID;
		item.Status = status;
		item.Chunk = *pChunk;
		uint   pos = 0;
		if(p_list->Get(chunkID, &pos, 0) > 0)
			*(STimeChunkAssoc *)p_list->at(pos) = item;
		else
			p_list->insert(&item);
	}
	return ok;
}

int STimeChunkGrid::SetHolidayList(long rowID, const STimeChunkArray * pList)
{
	int    ok = 1;
	uint   pos = 0;
	if(HL.lsearch(&rowID, &pos, CMPF_LONG, offsetof(HolidayArray, Id)))
		HL.atFree(pos);
	if(pList) {
		HolidayArray * p_new_item = new HolidayArray(rowID);
		p_new_item->copy(*pList);
		HL.insert(p_new_item);
	}
	return ok;
}

const STimeChunkArray * FASTCALL STimeChunkGrid::GetHolidayList(long rowID) const
{
	uint   pos = 0;
	return HL.lsearch(&rowID, &pos, CMPF_LONG, offsetof(HolidayArray, Id)) ? (STimeChunkArray *)HL.at(pos) : 0;
}

int STimeChunkGrid::SetCollapseList(const STimeChunkArray * pList)
{
	int    ok = 1;
	if(pList) {
		if(SETIFZ(P_CollapseList, new STimeChunkArray))
			*P_CollapseList = *pList;
		else
			ok = 0;
	}
	else {
		ZDELETE(P_CollapseList);
	}
	return ok;
}

const STimeChunkArray * STimeChunkGrid::GetCollapseList() const
{
	return P_CollapseList;
}

int FASTCALL STimeChunkGrid::GetBounds(STimeChunk & rBounds) const
{
	int    r = 0;
	rBounds.Start.SetZero();
	rBounds.Finish.SetFar();
	for(uint i = 0; i < getCount(); i++)
		r |= at(i)->GetBounds(&rBounds, 1);
	return r;
}

int STimeChunkGrid::GetText(int item, long id, SString & rBuf)
{
	int    ok = 1;
	rBuf.Z();
	if(item == iRow) {
		if(NameList.GetText(id, rBuf))
			ok = 1;
	}
	return ok;
}

int STimeChunkGrid::GetColor(long id, Color * pClr)
{
	return -1;
}

int STimeChunkGrid::Edit(int item, long rowID, const LDATETIME & rTm, long * pID)
{
	return -1;
}

int STimeChunkGrid::MoveChunk(int mode, long id, long rowId, const STimeChunk & rNewChunk)
{
	return -1;
}
//
//
//
// static
const char * STimeChunkBrowser::WndClsName = "STimeChunkBrowser"; // @global

// static
int STimeChunkBrowser::RegWindowClass(HINSTANCE hInst)
{
	WNDCLASSEX wc;
	MEMSZERO(wc);
	wc.cbSize        = sizeof(wc);
	wc.lpszClassName = STimeChunkBrowser::WndClsName;
	wc.hInstance     = hInst;
	wc.lpfnWndProc   = STimeChunkBrowser::WndProc;
	wc.style         = CS_HREDRAW|CS_VREDRAW|CS_OWNDC|CS_DBLCLKS;
	wc.hIcon         = LoadIcon(hInst, MAKEINTRESOURCE(/*ICON_TIMEGRID*/172));
	wc.hbrBackground = ::CreateSolidBrush(RGB(0xEE, 0xEE, 0xEE));
	wc.cbClsExtra    = BRWCLASS_CEXTRA;
	wc.cbWndExtra    = BRWCLASS_WEXTRA;
	return ::RegisterClassEx(&wc); // @unicodeproblem
}

void STimeChunkBrowser::RegisterMouseTracking()
{
	if(!(Flags & stMouseTrackRegistered)) {
		TWindow::RegisterMouseTracking(1, /*-1*/ 250);
		Flags |= stMouseTrackRegistered;
	}
}

int FASTCALL STimeChunkBrowser::InvalidateChunk(long chunkId)
{
	int    ok = -1;
	if(chunkId > 0) {
		for(uint i = 0; i < RL.getCount(); i++) {
			const SRect & r_sr = RL.at(i);
			if(r_sr.C.Id == chunkId) {
				invalidateRect(r_sr, 0);
				ok = 1;
			}
		}
	}
	return ok;
}

// static
LRESULT CALLBACK STimeChunkBrowser::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	TPoint p;
	STimeChunkBrowser::Loc loc;
	CREATESTRUCT * p_init_data;
	STimeChunkBrowser * p_view = 0;
	switch(message) {
		case WM_CREATE:
			p_init_data = (CREATESTRUCT *)lParam;
			if(TWindow::IsMDIClientWindow(p_init_data->hwndParent)) {
				p_view = (STimeChunkBrowser *)((LPMDICREATESTRUCT)(p_init_data->lpCreateParams))->lParam;
				p_view->BbState |= bbsIsMDI;
			}
			else {
				p_view = (STimeChunkBrowser *)p_init_data->lpCreateParams;
				p_view->BbState &= ~bbsIsMDI;
			}
			if(p_view) {
				p_view->HW = hWnd;
				TView::SetWindowProp(hWnd, GWLP_USERDATA, p_view);
				::SetFocus(hWnd);
				::SendMessage(hWnd, WM_NCACTIVATE, TRUE, 0);
				p_view->SetupScroll();
				p_view->invalidateAll(1);
				PostMessage(hWnd, WM_PAINT, 0, 0);
				{
					// @v9.1.5 char   cap[256];
					// @v9.1.5 ::GetWindowText(hWnd, cap, sizeof(cap));
					SString temp_buf;
					TView::SGetWindowText(hWnd, temp_buf); // @v9.1.5
					APPL->AddItemToMenu(temp_buf, p_view);
				}
				p_view->RegisterMouseTracking();
				return 0;
			}
			else
				return -1;
		case WM_DESTROY:
			p_view = (STimeChunkBrowser *)TView::GetWindowUserData(hWnd);
			if(p_view) {
				p_view->SaveParameters();
				SETIFZ(p_view->EndModalCmd, cmCancel);
				APPL->DelItemFromMenu(p_view);
				p_view->ResetOwnerCurrent();
				if(!p_view->IsInState(sfModal)) {
					APPL->P_DeskTop->remove(p_view);
					delete p_view;
					//SetWindowLong(hWnd, GWLP_USERDATA, 0);
					TView::SetWindowProp(hWnd, GWLP_USERDATA, (void *)0);
				}
			}
			return 0;
		case WM_SETFOCUS:
			if(!(TView::GetWindowStyle(hWnd) & WS_CAPTION)) {
				SetWindowPos(hWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
				APPL->NotifyFrame(0);
			}
			p_view = (STimeChunkBrowser *)TView::GetWindowUserData(hWnd);
			if(p_view) {
				APPL->SelectTabItem(p_view);
				TView::messageBroadcast(p_view, cmReceivedFocus);
				p_view->select();
			}
			break;
		case WM_KILLFOCUS:
			if(!(TView::GetWindowStyle(hWnd) & WS_CAPTION))
				APPL->NotifyFrame(0);
			p_view = (STimeChunkBrowser *)TView::GetWindowUserData(hWnd);
			if(p_view) {
				TView::messageBroadcast(p_view, cmReleasedFocus);
				p_view->ResetOwnerCurrent();
			}
			break;
		case WM_SIZE:
			p_view = (STimeChunkBrowser *)TView::GetWindowUserData(hWnd);
			CALLPTRMEMB(p_view, SetupScroll());
			break;
		case WM_PAINT:
			p_view = (STimeChunkBrowser *)TView::GetWindowUserData(hWnd);
			CALLPTRMEMB(p_view, Paint());
			break;
		case WM_HSCROLL:
		case WM_VSCROLL:
			p_view = (STimeChunkBrowser *)TView::GetWindowUserData(hWnd);
			CALLPTRMEMB(p_view, Scroll((message == WM_HSCROLL) ? SB_HORZ : SB_VERT, LoWord(wParam), HiWord(wParam)));
			break;
		case WM_KEYDOWN:
			{
				KeyDownCommand kdc;
				kdc.SetWinMsgCode(wParam);
				const uint16 tvk = kdc.GetTvKeyCode();
				if(/*wParam == VK_ESCAPE*/tvk == kbEsc) {
					p_view = (STimeChunkBrowser *)TView::GetWindowUserData(hWnd);
					if(p_view) {
						p_view->endModal(cmCancel);
						return 0;
					}
				}
				else if(/*wParam == VK_TAB*/tvk == kbCtrlTab) {
					p_view = (STimeChunkBrowser *)TView::GetWindowUserData(hWnd);
					if(/*GetKeyState(VK_CONTROL) & 0x8000 &&*/ p_view && !p_view->IsInState(sfModal)) {
						SetFocus(GetNextBrowser(hWnd, (GetKeyState(VK_SHIFT) & 0x8000) ? 0 : 1));
						return 0;
					}
				}
				else if(/*LOWORD(wParam) == VK_INSERT && 0x8000 & GetKeyState(VK_CONTROL)*/tvk == kbCtrlIns) {
					p_view = (STimeChunkBrowser *)TView::GetWindowUserData(hWnd);
					CALLPTRMEMB(p_view, CopyToClipboard());
				}
				// @v9.8.7 {
				else if(/*LOWORD(wParam) == VK_F10 && 0x8000 & GetKeyState(VK_CONTROL)*/tvk == kbCtrlF11) {
					p_view = (STimeChunkBrowser *)TView::GetWindowUserData(hWnd);
					CALLPTRMEMB(p_view, ExportToExcel());
				}
				// } @v9.8.7 
			}
			return 0;
		case WM_LBUTTONDBLCLK:
			p_view = (STimeChunkBrowser *)TView::GetWindowUserData(hWnd);
			CALLPTRMEMB(p_view, ProcessDblClk(p.setwparam(lParam)));
			break;
		case WM_LBUTTONDOWN:
			p_view = (STimeChunkBrowser *)TView::GetWindowUserData(hWnd);
			if(p_view) {
				if(hWnd != GetFocus())
					SetFocus(hWnd);
				SetCapture(hWnd);
				p_view->Resize(1, p.setwparam(lParam));
			}
			return 0;
		case WM_LBUTTONUP:
			ReleaseCapture();
			p_view = (STimeChunkBrowser *)TView::GetWindowUserData(hWnd);
			CALLPTRMEMB(p_view, Resize(0, p.setwparam(lParam)));
			break;
		case WM_MOUSELEAVE:
			p_view = (STimeChunkBrowser *)TView::GetWindowUserData(hWnd);
			if(p_view) {
				if(p_view->InvalidateChunk(p_view->St.SelChunkId) > 0) {
					p_view->St.SelChunkId = -1;
					UpdateWindow(hWnd);
				}
				else
					p_view->St.SelChunkId = -1;
				p_view->Flags &= ~stMouseTrackRegistered;
			}
			return 0;
		case WM_MOUSEWHEEL:
			p_view = (STimeChunkBrowser *)TView::GetWindowUserData(hWnd);
			if(p_view) {
				short delta = (short)HIWORD(wParam);
				if(p_view->P.ViewType == STimeChunkBrowser::Param::vHourDay) {
					p_view->Scroll(SB_HORZ, (delta > 0) ? SB_LINEUP : SB_LINEDOWN, 0);
				}
				else {
					p_view->Scroll(SB_HORZ, (delta > 0) ? SB_PAGEUP : SB_PAGEDOWN, 0);
				}
			}
			break;
		case WM_MOUSEMOVE:
			p_view = (STimeChunkBrowser *)TView::GetWindowUserData(hWnd);
			if(p_view) {
				TPoint tp;
				tp.setwparam(lParam);
				if(p_view->PrevMouseCoord != tp) {
					p_view->PrevMouseCoord = tp;
					p_view->Flags &= ~stMouseTrackRegistered;
					p_view->RegisterMouseTracking();
				}
				if(wParam == 0) {
					Area a2;
					p_view->GetArea(a2);
					if(p_view->Locate(p.setwparam(lParam), &loc) > 0) {
						HCURSOR cursor = 0;
						if(loc.Kind == Loc::kChunk && oneof2(loc.Pos, Loc::pLeftEdge, Loc::pRightEdge))
							cursor = p_view->Ptb.GetCursor(curResizeHorz);
						//else if(loc.Kind == Loc::kChunk && loc.Pos == Loc::pInner && p_view->St.Rsz.Kind == ResizeState::kMoveChunk)
						else if(loc.Kind == Loc::kChunk && loc.Pos == Loc::pMoveSpot)
							cursor = p_view->Ptb.GetCursor(curResizeRoze);
						else if(loc.Kind == Loc::kHeader && loc.Pos == Loc::pLeftEdge)
							cursor = p_view->Ptb.GetCursor(curResizeHorz);
						else if(loc.Kind == Loc::kHeader)
							cursor = p_view->Ptb.GetCursor(curCalendar);
						else if(loc.Kind == Loc::kSeparator)
							cursor = p_view->Ptb.GetCursor(curResizeHorz);
						else if(loc.Kind == Loc::kLeftZone && loc.Pos == Loc::pBottomEdge)
							cursor = p_view->Ptb.GetCursor(curResizeVert);
						else if(loc.Kind == Loc::kPicMode)
							cursor = p_view->Ptb.GetCursor(curHand);
						else
							cursor = ::LoadCursor(0, IDC_ARROW);
						::SetCapture(hWnd);
						::SetCursor(cursor);
						if(p_view->St.SelChunkId != loc.EntryId) {
							p_view->InvalidateChunk(p_view->St.SelChunkId);
							p_view->InvalidateChunk(loc.EntryId);
							p_view->St.SelChunkId = loc.EntryId;
							UpdateWindow(hWnd);
						}
					}
					else {
						if(p_view->InvalidateChunk(p_view->St.SelChunkId) > 0) {
							p_view->St.SelChunkId = -1;
							UpdateWindow(hWnd);
						}
						else
							p_view->St.SelChunkId = -1;
					}
				}
				else if(wParam & MK_LBUTTON && hWnd == GetCapture())
					p_view->Resize(2, p.setwparam(lParam));
			}
			return 0; // @exit
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}

STimeChunkBrowser::Param::Param()
{
	Clear();
}

STimeChunkBrowser::Param & STimeChunkBrowser::Param::Clear()
{
	memzero(this, offsetof(STimeChunkBrowser::Param, RegSaveParam));
	SingleRowIdx = -1;
	return *this;
}

STimeChunkBrowser::Area::Area()
{
	THISZERO();
}

STimeChunkBrowser::Area & STimeChunkBrowser::Area::Clear()
{
	THISZERO();
	return *this;
}

STimeChunkBrowser::SRect::SRect() : TRect(), RowId(0), DayN(0)
{
}

STimeChunkBrowser::SRect & STimeChunkBrowser::SRect::Clear()
{
	THISZERO();
	return *this;
}

STimeChunkBrowser::SRectArray::SRectArray() : TSVector <SRect> () // @v9.8.4 TSArray-->TSVector
{
}

const STimeChunkBrowser::SRect * FASTCALL STimeChunkBrowser::SRectArray::SearchPoint(TPoint p) const
{
	for(uint i = 0; i < getCount(); i++) {
		const STimeChunkBrowser::SRect & r_item = at(i);
		if(r_item.contains(p))
			return &r_item;
	}
	return 0;
}

STimeChunkBrowser::STimeChunkBrowser() : TBaseBrowserWindow(WndClsName), BmpId_ModeGantt(0), BmpId_ModeHourDay(0), P_Tt(0), Flags(0)
{
	P.Quant = 12 * 60;
	P.PixQuant = 20;
	P.PixRow = 20;
	P_Data = &DataStub;
	MEMSZERO(St);
	Ptb.SetColor(colorHeader,     RGB(0x77, 0x88, 0x99) /*темно-темно-серый*/);  // Цвет отрисовки заголовка таблицы
	Ptb.SetColor(colorMain,       RGB(0xEE, 0xEE, 0xEE));  // Основной цвет фона
	Ptb.SetColor(colorInterleave, RGB(0xDD, 0xDD, 0xDD));  // Цвет фона черезстрочных линий
	Ptb.SetColor(colorWhiteText,  GetColorRef(SClrWhite)); // Белый цвет текста
	Ptb.SetColor(colorBlackText,  GetColorRef(SClrBlack)); // Черный цвет текста
	Ptb.SetPen(penMainQuantSeparator, SPaintObj::psDot,   1, GetColorRef(SClrSilver));
	Ptb.SetPen(penQuantSeparator,     SPaintObj::psSolid, 1, GetColorRef(SClrSilver));
	Ptb.SetPen(penDaySeparator,     SPaintObj::psSolid, 1, GetColorRef(SClrDarkgrey));
	Ptb.SetPen(penMainSeparator,    SPaintObj::psSolid, 1, GetColorRef(SClrSilver));
	Ptb.SetPen(penDefChunk,         SPaintObj::psNull,  1, GetColorRef(SClrSilver));
	Ptb.SetPen(penSelectedChunk,    SPaintObj::psInsideFrame, 1, GetColorRef(SClrCoral));
	Ptb.SetPen(penChunk,            SPaintObj::psInsideFrame, 1, GetColorRef(SClrDimgrey));
	Ptb.SetPen(penResizedChunk,     SPaintObj::psInsideFrame, 3, GetColorRef(SClrBlack));
	Ptb.SetPen(penCurrent,          SPaintObj::psDot,   1, GetColorRef(SClrRed));
	Ptb.SetBrush(brushNull,         SPaintObj::bsNull,  0, 0);
	Ptb.SetBrush(brushMain,         SPaintObj::bsSolid, Ptb.GetColor(colorMain), 0);
	Ptb.SetBrush(brushHeader,       SPaintObj::bsSolid, Ptb.GetColor(colorHeader), 0);
	Ptb.SetBrush(brushDefChunk,     SPaintObj::bsSolid, RGB(/*0x8C, 0xB6, 0xCE*//*0x00, 0x89, 0xC0*/0xE8, 0xEF, 0xF7), 0);
	Ptb.SetBrush(brushInterleave,   SPaintObj::bsSolid, Ptb.GetColor(colorInterleave), 0);
	Ptb.SetBrush(brushRescaleQuant, SPaintObj::bsSolid, GetColorRef(SClrCoral), 0);
	Ptb.SetBrush(brushMovedChunk,   SPaintObj::bsSolid, GetColorRef(SClrWhite), 0);
	Ptb.SetBrush(brushHoliday,      SPaintObj::bsHatched,      RGB(0x09, 0x09, 0x09), SPaintObj::bhsFDiagonal);
	Ptb.SetBrush(brushHolidayInterleave, SPaintObj::bsHatched, RGB(0x09, 0x09, 0x09), SPaintObj::bhsBDiagonal);
	Ptb.CreateCursor(curResizeHorz, IDC_TWOARR_HORZ);
	Ptb.CreateCursor(curResizeVert, IDC_TWOARR_VERT);
	Ptb.CreateCursor(curResizeRoze, IDC_ARR_ROZE);
	Ptb.CreateCursor(curCalendar,   IDC_CALENDAR);
	Ptb.CreateCursor(curHand,       IDC_PPYPOINT);
	options |= ofSelectable;
}

STimeChunkBrowser::~STimeChunkBrowser()
{
	if(BbState & bbsDataOwner)
		delete P_Data;
	for(uint i = 0; i < ColorBrushList.getCount(); i++) {
		HBRUSH b = (HBRUSH)ColorBrushList.at(i).Val;
		ZDeleteWinGdiObject(&b);
	}
	ZDELETE(P_Tt);
}

//virtual
TBaseBrowserWindow::IdentBlock & STimeChunkBrowser::GetIdentBlock(TBaseBrowserWindow::IdentBlock & rBlk)
{
	rBlk.IdBias = IdBiasTimeChunkBrowser;
	rBlk.ClsName = STimeChunkBrowser::WndClsName;
	rBlk.InstanceIdent.Z().Cat(GetResID());
	return rBlk;
}

enum {
	kpPixQuant = 1,
	kpPixRow,
	kpPixRowMargin,
	kpTextZonePart
};

static void FASTCALL MakeParamKeyList(StrAssocArray & rList)
{
	rList.Add(kpPixQuant,     "PixQuant");
	rList.Add(kpPixRow,       "PixRow");
	rList.Add(kpPixRowMargin, "PixRowMargin");
	rList.Add(kpTextZonePart, "TextZonePart");
}

int STimeChunkBrowser::SaveParameters()
{
	int    ok = 1;
	if(P.RegSaveParam.NotEmpty()) {
		StrAssocArray param_list;
		SString temp_buf, sub_key;
		MakeParamKeyList(param_list);
		for(uint i = 0; i < param_list.getCount(); i++) {
			StrAssocArray::Item item = param_list.Get(i);
			switch(item.Id) {
				case kpPixQuant: temp_buf.CatEq(item.Txt, (long)P.PixQuant).Semicol(); break;
				case kpPixRow:   temp_buf.CatEq(item.Txt, (long)P.PixRow).Semicol(); break;
				case kpPixRowMargin: temp_buf.CatEq(item.Txt, (long)P.PixRowMargin).Semicol(); break;
				case kpTextZonePart: temp_buf.CatEq(item.Txt, (long)St.TextZonePart).Semicol(); break;
			}
		}
		(sub_key = "Software").SetLastSlash().Cat(SLS.GetAppName()).SetLastSlash().Cat("STimeChunkBrowser");
		WinRegKey reg_key(HKEY_CURRENT_USER, sub_key, 0);
		ok = reg_key.PutString(P.RegSaveParam, temp_buf) ? 1 : 0;
	}
	else
		ok = -1;
	return ok;
}

int STimeChunkBrowser::RestoreParameters(STimeChunkBrowser::Param & rParam)
{
	int    ok = -1;
	if(rParam.RegSaveParam.NotEmpty()) {
		char   val[512];
		StrAssocArray param_list;
		SString temp_buf, sub_key, left, right;
		MakeParamKeyList(param_list);
		(sub_key = "Software").SetLastSlash().Cat(SLS.GetAppName()).SetLastSlash().Cat("STimeChunkBrowser");
		WinRegKey reg_key(HKEY_CURRENT_USER, sub_key, 1); // @v9.2.0 readonly 0-->1
		if(reg_key.GetString(rParam.RegSaveParam, val, sizeof(val)) > 0) {
			SStrScan scan(val);
			while(scan.SearchChar(';')) {
				scan.Get(temp_buf);
				scan.IncrLen(1 /* character ';' */);
				for(uint i = 0; i < param_list.getCount(); i++) {
					StrAssocArray::Item item = param_list.Get(i);
					if(temp_buf.Divide('=', left, right) > 0 && left.Strip().CmpNC(item.Txt) == 0) {
						switch(item.Id) {
							case kpPixQuant:     rParam.PixQuant = (uint)right.ToLong(); break;
							case kpPixRow:       rParam.PixRow = (uint)right.ToLong(); break;
							case kpPixRowMargin: rParam.PixRowMargin = (uint)right.ToLong(); break;
							case kpTextZonePart: rParam.TextZonePart = (uint)right.ToLong(); break;
						}
					}
				}
			}
			ok = 1;
		}
	}
	return ok;
}

int STimeChunkBrowser::SetData(STimeChunkGrid * pGrid, int takeAnOwnership)
{
	if(pGrid) {
		if(BbState & bbsDataOwner)
			delete P_Data;
		P_Data = pGrid;
		SETFLAG(BbState, bbsDataOwner, takeAnOwnership);
	}
	else {
		P_Data = &DataStub;
		SETFLAG(BbState, bbsDataOwner, 0);
	}
	OnUpdateData();
	SetupDate(getcurdate_());
	return 1;
}

TRect STimeChunkBrowser::GetMargin() const { return TRect(2, 0, 2, 0); }
int FASTCALL STimeChunkBrowser::IsKeepingData(const STimeChunkGrid * pGrid) const { return BIN(P_Data == pGrid); }

void STimeChunkBrowser::UpdateData()
{
	OnUpdateData();
	invalidateAll(1);
	::UpdateWindow(H());
}

void STimeChunkBrowser::SetupScroll()
{
	Area a2;
	GetArea(a2);
	St.ScrollLimitY = 0;
	uint   x_scroll_max = 0;
	if(P.ViewType == Param::vHourDay) {
		uint  min_hour, max_hour;
		DateRange period;
		CalcHdTimeBounds(a2, period, min_hour, max_hour);
		uint   hour_per_screen = a2.Right.height() / a2.PixPerHour;
		if(hour_per_screen <= (max_hour - min_hour))
			St.ScrollLimitY = (max_hour - min_hour + 1) - hour_per_screen;
		uint   days_per_screen = NZOR(P.DaysCount, 7);
		x_scroll_max = St.QBounds-days_per_screen;
	}
	else {
		const  uint dc = P_Data->getCount();
		if(P_Data) {
			uint   decrement = 0;
			int    height = 0;
			uint   dc = P_Data->getCount();
			if(dc) do {
				decrement++;
				const  STimeChunkAssocArray * p_row = P_Data->at(--dc);
				const  RowState & r_rowst = GetRowState(p_row->Id);
				const  uint row_full_height = (P.PixRow * NZOR(r_rowst.Order, 1)) + 2 * P.PixRowMargin;
				height += row_full_height;
			} while(dc && height < a2.Right.height());
			if(decrement)
				decrement--;
			if(decrement < P_Data->getCount())
				St.ScrollLimitY = P_Data->getCount() - decrement;
		}
		x_scroll_max = St.QBounds-1;
	}
	SETMIN(St.ScrollY, St.ScrollLimitY);

	SCROLLINFO si;
	MEMSZERO(si);
	si.cbSize = sizeof(si);
	si.fMask = SIF_POS | SIF_RANGE;
	si.nMin = 0;
	si.nMax = St.ScrollLimitY;
	si.nPos = MIN(si.nMax, (int)St.ScrollY);
	SetScrollInfo(H(), SB_VERT, &si, TRUE);
	si.nMax = MIN(32000, x_scroll_max);
	si.nPos = MIN(si.nMax, (int)St.ScrollX);
	::SetScrollInfo(H(), SB_HORZ, &si, TRUE);
	{
		GetArea(a2.Clear());
		CalcChunkRect(&a2, RL);
	}
}

void STimeChunkBrowser::SetBmpId(int ident, uint bmpId)
{
	switch(ident) {
		case bmpModeGantt: BmpId_ModeGantt = bmpId; break;
		case bmpModeHourDay: BmpId_ModeHourDay = bmpId; break;
		case bmpBack: BmpId_Back = bmpId; break;
	}
}

int STimeChunkBrowser::SetParam(const Param * pParam)
{
	SString temp_buf;
	RVALUEPTR(P, pParam);
 	SETIFZ(P.Quant, 12 * 60);
	SETIFZ(P.PixQuant,   20);
	SETIFZ(P.PixRow,     10);
	St.HdrLevelCount = 0;
	memzero(St.HdrLevel, sizeof(St.HdrLevel));
	if(P.ViewType == Param::vHourDay) {
		St.HdrLevel[St.HdrLevelCount++] = 3600 * 24;
	}
	else {
		St.HdrLevel[St.HdrLevelCount++] = P.Quant;
		if(P.Quant < 3600) {
			for(uint i = 1; i <= 12; i++)
				if((i * 3600) % P.Quant == 0) {
					St.HdrLevel[St.HdrLevelCount++] = i * 3600;
					break;
				}
			St.HdrLevel[St.HdrLevelCount++] = 3600 * 24;
		}
		else
			St.HdrLevel[St.HdrLevelCount++] = 3600 * 24;
	}
	SETIFZ(P.HdrLevelHeight, 20);
	St.TextZonePart = P.TextZonePart;
	if(P_Data->GetText(STimeChunkGrid::iRow, -1, temp_buf) > 0) {
		if(St.TextZonePart == 0 || St.TextZonePart > 80)
			St.TextZonePart = 20;
	}
	else
		St.TextZonePart = 0;
	LOGFONT lf;
	{
		MEMSZERO(lf);
		lf.lfCharSet = DEFAULT_CHARSET;
		lf.lfHeight  = MIN(18, P.HdrLevelHeight * 5 / 6);
		STRNSCPY(lf.lfFaceName, _T("Arial"));
		Ptb.SetFont(fontHeader, ::CreateFontIndirect(&lf));
	}
	{
		MEMSZERO(lf);
		lf.lfCharSet = DEFAULT_CHARSET;
		lf.lfHeight  = 14;
		STRNSCPY(lf.lfFaceName, _T("Arial"));
		Ptb.SetFont(fontLeftText, ::CreateFontIndirect(&lf));
	}
	{
		MEMSZERO(lf);
		lf.lfCharSet = DEFAULT_CHARSET;
		lf.lfHeight  = MIN(14, P.PixRow * 5 / 6);
		STRNSCPY(lf.lfFaceName, _T("Arial"));
		Ptb.SetFont(fontChunkText, ::CreateFontIndirect(&lf));
	}
	return 1;
}

void STimeChunkBrowser::OnUpdateData()
{
	RowStateList.freeAll();
	for(uint i = 0; i < P_Data->getCount(); i++) {
		const STimeChunkAssocArray * p_row = P_Data->at(i);
		LAssocArray order_list;
		RowState * p_s = RowStateList.CreateNewItem();
		p_s->Id = p_row->Id;
		p_s->Order = p_row->GetIntersectionOrder(&order_list, 1000);
		if(p_s->Order > 1)
			p_s->OrderList = order_list;
	}
	RowStateList.sort(CMPF_LONG);
	int    r = P_Data->GetBounds(St.Bounds);
	if(!(r & 0x01))
		St.Bounds.Start.Set(NZOR(P.DefBounds.low, getcurdate_()), ZEROTIME);
	if(!(r & 0x02))
		St.Bounds.Finish.Set(NZOR(P.DefBounds.upp, getcurdate_()), encodetime(23, 59, 59, 0));
	if(P.DefBounds.low && St.Bounds.Start.d > P.DefBounds.low)
		St.Bounds.Start.d = P.DefBounds.low;
	if(P.DefBounds.upp && St.Bounds.Finish.d < P.DefBounds.upp)
		St.Bounds.Finish.d = P.DefBounds.upp;
	if(St.Bounds.Finish.IsFar()) {
		St.Bounds.Finish.d = plusdate(getcurdate_(), 30);
		St.Bounds.Finish.t = ZEROTIME;
	}
	St.Bounds.Start.t = ZEROTIME;
	//
	if(P.ViewType == Param::vHourDay) {
		long   dur = St.Bounds.GetDurationDays();
		St.QBounds = (dur > 0) ? dur : 365;
	}
	else {
		long   dur = St.Bounds.GetDuration();
		St.QBounds = (dur > 0) ? (dur / P.Quant) : 1000;
	}
	ChunkTextCache.Clear();
	SetupScroll();
}

const STimeChunkBrowser::RowState & FASTCALL STimeChunkBrowser::GetRowState(long id) const
{
	uint   rs_pos = 0;
	int    rsf = RowStateList.bsearch(&id, &rs_pos, CMPF_LONG); // Функция OnUpdateData отсортировала список
	assert(rsf);
	return *RowStateList.at(rs_pos);
}

const STimeChunkArray * STimeChunkBrowser::GetCollapseList_() const
{
	//return (P.ViewType == Param::vHourDay) ? 0 : (P_Data ? P_Data->GetCollapseList() : 0);
	return (P_Data ? P_Data->GetCollapseList() : 0);
}

int FASTCALL STimeChunkBrowser::IsQuantVisible(long q) const
{
	int    yes = 1;
	const STimeChunkArray * p_collapse_list = GetCollapseList_();
	if(p_collapse_list) {
		if(P.ViewType == Param::vHourDay) {
			const LDATE dt = plusdate(St.Bounds.Start.d, q);
			LDATETIME dtm1, dtm2;
			STimeChunk chunk(dtm1.Set(dt, ZEROTIME), dtm2.Set(dt, encodetime(23, 59, 59, 99)));
			STimeChunkArray isect_list;
			if(p_collapse_list->Intersect(chunk, &isect_list)) {
				for(uint i = 0; yes && i < isect_list.getCount(); i++) {
					const STimeChunk * p_ic = (const STimeChunk *)isect_list.at(i);
					if(*p_ic == chunk)
						yes = 0;
				}
			}
		}
		else {
			LDATETIME dtm;
			(dtm = St.Bounds.Start).addsec(q * P.Quant);
			if(!p_collapse_list->IsFreeEntry(dtm, P.Quant, 0))
				yes = 0;
		}
	}
	return yes;
}

uint STimeChunkBrowser::GetScrollLimitY() const
{
	return St.ScrollLimitY;
}

void STimeChunkBrowser::Scroll(int sbType, int sbEvent, int thumbPos)
{
	uint   prev_scr_x = St.ScrollX;
	uint   prev_scr_y = St.ScrollY;
	int    delta;
	Area   a2;
	SCROLLINFO si;
	if(sbType == SB_VERT) {
		switch(sbEvent) {
			case SB_TOP:    St.ScrollY = 0; break;
			case SB_BOTTOM: St.ScrollY = GetScrollLimitY(); break;
			case SB_LINEUP:
				if(St.ScrollY > 0)
					--St.ScrollY;
				break;
			case SB_LINEDOWN:
				if(St.ScrollY < GetScrollLimitY())
					++St.ScrollY;
				break;
			case SB_PAGEUP:
				if(St.ScrollY > 0) {
					int    set_to_zero = 1;
					for(uint i = St.ScrollY; i > 0;)
						if(GetBottomRowIdx(i) <= (St.ScrollY-1)) {
							St.ScrollY = i;
							set_to_zero = 0;
							break;
						}
						else
							--i;
					if(set_to_zero)
						St.ScrollY = 0;
				}
				break;
			case SB_PAGEDOWN:
				{
					uint idx = GetBottomRowIdx(St.ScrollY);
					if(idx < GetScrollLimitY())
						St.ScrollY = idx+1;
					else
						St.ScrollY = GetScrollLimitY();
				}
				break;
			case SB_THUMBPOSITION:
			case SB_THUMBTRACK:
				if(thumbPos < 0)
					St.ScrollY = 0;
				else if(thumbPos > (int)GetScrollLimitY())
					St.ScrollY = GetScrollLimitY();
				else
					St.ScrollY = thumbPos;
				break;
			default:
				break;
		}
		{
			MEMSZERO(si);
			si.cbSize = sizeof(si);
			si.fMask = SIF_POS;
			si.nPos = St.ScrollY;
			SetScrollInfo(H(), SB_VERT, &si, TRUE);
		}
	}
	if(sbType == SB_HORZ) {
		switch(sbEvent) {
			case SB_TOP:    St.ScrollX = 0; break;
			case SB_BOTTOM: St.ScrollX = St.QBounds-1; break;
			case SB_PAGEUP:
				{
					GetArea(a2);
					delta = a2.Right.width() / a2.PixQuant; // Ширина области видимости в квантах
					if(St.QBounds > 32000)
						delta = (delta * St.QBounds / 32000);
					else {
						for(int i = 1; i <= delta; i++)
							if((int)St.ScrollX > i && !IsQuantVisible(St.ScrollX-i))
								delta++;
					}
					if((int)St.ScrollX > delta)
						St.ScrollX -= delta;
					else
						St.ScrollX = 0;
				}
				break;
			case SB_LINEUP:
				if(St.QBounds > 32000)
					delta = (int)(St.QBounds / 32000);
				else {
					for(delta = 1; (int)St.ScrollX > delta && !IsQuantVisible(St.ScrollX-delta);)
						delta++;
				}
				if((int)St.ScrollX > delta)
					St.ScrollX -= delta;
				else
					St.ScrollX = 0;
				break;
			case SB_PAGEDOWN:
				{
					GetArea(a2);
					delta = a2.Right.width() / a2.PixQuant; // Ширина области видимости в квантах
					if(St.QBounds > 32000)
						delta = (delta * St.QBounds / 32000);
					else {
						for(int i = 1; i <= delta; i++)
							if(St.ScrollX < (St.QBounds-i-1) && !IsQuantVisible(St.ScrollX+i))
								delta++;
					}
					if(St.ScrollX < (St.QBounds-delta-1))
						St.ScrollX += delta;
					else
						St.ScrollX = St.QBounds-1;
				}
				break;
			case SB_LINEDOWN:
				if(St.QBounds > 32000)
					delta = (int)(St.QBounds / 32000);
				else {
					for(delta = 1; St.ScrollX < (St.QBounds-delta-1) && !IsQuantVisible(St.ScrollX+delta);)
						delta++;
				}
				if(St.ScrollX < (St.QBounds-delta-1))
					St.ScrollX += delta;
				else
					St.ScrollX = St.QBounds-1;
				break;
			case SB_THUMBPOSITION:
			case SB_THUMBTRACK:
				if(St.QBounds > 32000)
					St.ScrollX = (int)(thumbPos * St.QBounds / 32000);
				else if(GetCollapseList_()) {
					long delta = (long)thumbPos - (long)St.ScrollX;
					if(delta > 0) {
						while(St.ScrollX < (St.QBounds-delta-1) && !IsQuantVisible(St.ScrollX+delta)) {
							delta++;
						}
					}
					else if(delta < 0) {
						delta = -delta;
						while((int)St.ScrollX > delta && !IsQuantVisible(St.ScrollX-delta))
							delta++;
						delta = -delta;
					}
					St.ScrollX += delta;
				}
				else
					St.ScrollX = thumbPos;
				break;
			default:
				break;
		}
		{
			MEMSZERO(si);
			si.cbSize = sizeof(si);
			si.fMask = SIF_POS;
			si.nPos = (St.QBounds > 32000) ? ((32000 * St.ScrollX) / St.QBounds) : St.ScrollX;
			SetScrollInfo(H(), SB_HORZ, &si, TRUE);
		}
	}
	/*if(St.ScrollX != prev_scr_x || St.ScrollY != prev_scr_y)*/ { // @v9.8.0
		GetArea(a2);
		CalcChunkRect(&a2, RL);
		if(sbType == SB_HORZ) {
			invalidateRect(a2.RightHeader, 1);
			invalidateRect(a2.Right, 1);
		}
		else
			invalidateAll(1);
		UpdateWindow(H());
	}
}

void FASTCALL STimeChunkBrowser::GetStartPageDate(LDATE * pDt)
{
	Area a2;
	GetArea(a2);
	STimeChunk view_time_bounds = GetBoundsTime(a2);
	ASSIGN_PTR(pDt, view_time_bounds.Start.d);
}

int ExecDateCalendar(/*HWND*/uint32 hParent, LDATE * pDt); // @prototype

int STimeChunkBrowser::SetupDate(LDATE dt)
{
	int    ok = -1;
	if(dt >= St.Bounds.Start.d && dt <= St.Bounds.Finish.d) {
		int    thumb = 0;
		if(P.ViewType == Param::vHourDay) {
			thumb = diffdate(dt, St.Bounds.Start.d);
		}
		else {
			LDATETIME t;
			t.Set(dt, (dt == getcurdate_()) ? getcurtime_() : encodetime(8, 0, 0, 0));
			//
			// Здесь следует использовать diffdatetime (а не STimeChunkBrowser::DiffTime) поскольку функция Scroll самостоятельно
			// сделает поправку на коллапсированные периоды
			//
			long s = diffdatetimesec(t, St.Bounds.Start);
			//
			thumb = (St.QBounds > 32000) ? ((s * 32000) / St.QBounds) : (s / P.Quant);
		}
		Scroll(SB_HORZ, SB_THUMBPOSITION, thumb);
		ok = 1;
	}
	return ok;
}

int STimeChunkBrowser::ProcessDblClk(TPoint p)
{
	int    ok = -1, done = 0;
	Loc    loc;
	if(Locate(p, &loc) > 0) {
		if(loc.Kind == Loc::kChunk) {
			if(P_Data->Edit(P_Data->iChunk, loc.RowId, loc.Tm, &loc.EntryId) > 0)
				ok = 1;
		}
		else if(loc.Kind == Loc::kWorkspace) {
			long   id = 0;
			LDATETIME t = loc.Tm;
			if(P.Flags & Param::fSnapToQuant) {
				long s = t.t.totalsec();
				long r = (s % P.Quant);
				t.t.settotalsec(s - r); // Округление в меньшую сторону
				/* округление до ближайшего
				if(r < (long)(P.Quant / 2))
					t.t.settotalsec(s - r);
				else
					t.t.settotalsec(((s / P.Quant) + 1) * P.Quant);
				*/
			}
			long   row_id = 0;
			if(P.ViewType == Param::vHourDay && P.SingleRowIdx >= 0 && P.SingleRowIdx <= (long)P_Data->getCount())
				row_id = P_Data->at(P.SingleRowIdx)->Id;
			else
				row_id = loc.RowId;
			if(P_Data->Edit(P_Data->iChunk, row_id, t, &id) > 0)
				ok = 1;
		}
		else if(loc.Kind == Loc::kHeader && !oneof2(loc.Pos, Loc::pLeftEdge, Loc::pRightEdge)) {
			LDATE dt = ZERODATE;
			GetStartPageDate(&dt);
			/* @v9.1.3
			if(ExecDateCalendar(H(), &dt) > 0)
				ok = SetupDate(dt);
			*/
			// @v9.1.3 {
			{
				SlExtraProcBlock epb;
				SLS.GetExtraProcBlock(&epb);
				if(epb.F_CallCalendar && epb.F_CallCalendar((uint32)H(), &dt) > 0)
					ok = SetupDate(dt);
			}
			// } @v9.1.3
		}
		else if(loc.Kind == Loc::kLeftZone) {
			if(P.ViewType == Param::vPrcTime && loc.RowId) {
				long   row_idx = -1;
				for(uint i = 0; i < P_Data->getCount(); i++) {
					if(P_Data->at(i)->Id == loc.RowId) {
						row_idx = (long)i;
						break;
					}
				}
				if(row_idx >= 0) {
					Area   a2;
					GetArea(a2);
					STimeChunk bt = GetBoundsTime(a2);
					P.ViewType = Param::vHourDay;
					P.SingleRowIdx = row_idx;
					SetParam(0);
					St.ScrollX = 0;
					St.ScrollY = 0;
					UpdateData();
					SetupDate(bt.Start.d);
					done = 1;
					ok = 1;
				}
			}
		}
	}
	if(ok > 0 && !done) {
		OnUpdateData();
		invalidateAll(1);
		::UpdateWindow(H());
	}
	return ok;
}

void STimeChunkBrowser::ResizeState::Setup(int kind, TPoint p)
{
	Kind = kind;
	Org = p;
	Shift = 0;
}

int STimeChunkBrowser::Resize(int mode, TPoint p)
{
	int    ok = -1;
	int    do_redraw = 0;
	int    new_mode = 0;
	STimeChunkBrowser::Loc loc;
	if(mode == 1) {
		//
		// Запуск режима изменения размеров
		//
		if(St.Rsz.Kind == 0) {
			if(Locate(p, &loc) > 0) {
				long cursor = curResizeHorz;
				if(loc.Kind == Loc::kPicMode) {
					St.Rsz.Setup(ResizeState::kSwitchMode, p);
					cursor = 0;
					ok = 1;
				}
				else if(loc.Kind == Loc::kHeader && loc.Pos == Loc::pLeftEdge) {
					assert(loc.HdrLevel < (int)St.HdrLevelCount);
					St.Rsz.Setup(ResizeState::kRescale, p);
					St.Rsz.HdrLevel = (int16)loc.HdrLevel;
					St.Rsz.Quant = DiffTime(loc.TmQuant, St.Bounds.Start) / St.HdrLevel[loc.HdrLevel];
					ok = 1;
				}
				else if(loc.Kind == Loc::kChunk) {
					if(loc.Pos == Loc::pMoveSpot) {
						if(P_Data->MoveChunk(STimeChunkGrid::mmCanMove, loc.EntryId, loc.RowId, loc.Chunk) > 0) {
							St.Rsz.Setup(ResizeState::kMoveChunk, p);
							St.Rsz.ChunkId = loc.EntryId;
							St.Rsz.RowId = loc.RowId;
							cursor = curResizeRoze;
							ok = 1;
						}
					}
					else if(loc.Pos == Loc::pLeftEdge) {
						if(P_Data->MoveChunk(STimeChunkGrid::mmCanResizeLeft, loc.EntryId, loc.RowId, loc.Chunk) > 0) {
							St.Rsz.Setup(ResizeState::kChunkLeft, p);
							St.Rsz.ChunkId = loc.EntryId;
							St.Rsz.RowId = loc.RowId;
							ok = 1;
						}
					}
					else if(loc.Pos == Loc::pRightEdge) {
						if(P_Data->MoveChunk(STimeChunkGrid::mmCanResizeRight, loc.EntryId, loc.RowId, loc.Chunk) > 0) {
							St.Rsz.Setup(ResizeState::kChunkRight, p);
							St.Rsz.ChunkId = loc.EntryId;
							St.Rsz.RowId = loc.RowId;
							ok = 1;
						}
					}
				}
				else if(loc.Kind == Loc::kSeparator) {
					St.Rsz.Setup(ResizeState::kSplit, p);
					ok = 1;
				}
				else if(loc.Kind == Loc::kLeftZone && loc.Pos == Loc::pBottomEdge) {
					St.Rsz.Setup(ResizeState::kRowHeight, p);
					St.Rsz.RowId = loc.RowId;
					cursor = curResizeVert;
					ok = 1;
				}
				/* @construction
				else if(loc.Kind == Loc::kWorkspace) {
					St.Rsz.Setup(ResizeState::kScroll, p);
					cursor = curResizeRoze;
					ok = 1;
				}
				*/
				if(ok > 0) {
					if(cursor) {
						::SetCapture(H());
						::SetCursor(Ptb.GetCursor(cursor));
					}
					InvalidateResizeArea();
					::UpdateWindow(H());
				}
			}
		}
	}
	else if(mode == 0) {
		//
		// Завершение режима изменения размеров
		//
		if(St.Rsz.Kind) {
			if(St.Rsz.Kind == ResizeState::kSwitchMode) {
				Area   a2;
				GetArea(a2);
				STimeChunk bt = GetBoundsTime(a2);
				if(P.ViewType == Param::vHourDay) {
					P.SingleRowIdx = -1;
					P.ViewType = Param::vPrcTime;
				}
				else {
					P.ViewType = Param::vHourDay;
				}
				SetParam(0);
				St.ScrollX = 0;
				St.ScrollY = 0;
				UpdateData();
				SetupDate(bt.Start.d);
			}
			else if(St.Rsz.Kind == ResizeState::kRescale) {
				int    delta = -(p.x - St.Rsz.Org.x);
				if(delta) {
					long   new_pix_quant = P.PixQuant + (delta * (int)P.Quant) / (int)St.HdrLevel[St.Rsz.HdrLevel];
					SETMINMAX(new_pix_quant, 2, 100);
					P.PixQuant = new_pix_quant;
				}
				do_redraw = 1;
			}
			else if(St.Rsz.Kind == ResizeState::kMoveChunk) {
				STimeChunkAssoc grid_item;
				if(P_Data->GetChunk(St.Rsz.ChunkId, 0, &grid_item)) {
					long   delta_sec = PixToSec((int)(p.x - St.Rsz.Org.x));
					grid_item.Chunk.Start.addsec(delta_sec);
					grid_item.Chunk.Finish.addsec(delta_sec);
					if(P_Data->MoveChunk(STimeChunkGrid::mmCommit, St.Rsz.ChunkId, St.Rsz.RowId, grid_item.Chunk) > 0)
						OnUpdateData();
				}
				do_redraw = 1;
			}
			else if(oneof2(St.Rsz.Kind, ResizeState::kChunkLeft, ResizeState::kChunkRight)) {
				STimeChunkAssoc grid_item;
				if(P_Data->GetChunk(St.Rsz.ChunkId, 0, &grid_item)) {
					long   delta_sec = PixToSec((int)(p.x - St.Rsz.Org.x));
					if(St.Rsz.Kind == ResizeState::kChunkLeft)
						grid_item.Chunk.Start.addsec(delta_sec);
					else
						grid_item.Chunk.Finish.addsec(delta_sec);
					if(cmp(grid_item.Chunk.Start, grid_item.Chunk.Finish) < 0)
						if(P_Data->MoveChunk(STimeChunkGrid::mmCommit, St.Rsz.ChunkId, St.Rsz.RowId, grid_item.Chunk) > 0)
							OnUpdateData();
				}
				do_redraw = 1;
			}
			else if(St.Rsz.Kind == ResizeState::kSplit) {
				if(p.x != St.Rsz.Org.x) {
					Area   a2;
					GetArea(a2);
					uint   new_part = p.x * 100 / a2.Full.width();
					if(new_part >= 1 && new_part < 80)
						St.TextZonePart = new_part;
				}
				do_redraw = 1;
			}
			else if(St.Rsz.Kind == ResizeState::kRowHeight) {
				TRect  rect;
				int    order = 0;
				if(GetRowRect(St.Rsz.RowId, 1, &order, &rect) > 0) {
					uint row_full_height = (P.PixRow * order) + 2 * P.PixRowMargin;
					uint new_height  = row_full_height + (int)(p.y - St.Rsz.Org.y);
					uint new_pix_row = P.PixRow * new_height / row_full_height;
					uint new_pix_mgn = P.PixRowMargin * new_height / row_full_height;
					if(new_pix_row >= 3 && new_pix_row <= 100) {
						P.PixRow = new_pix_row;
						P.PixRowMargin = new_pix_mgn;
						{
							LOGFONT lf;
							MEMSZERO(lf);
							lf.lfCharSet = DEFAULT_CHARSET;
							lf.lfHeight  = MIN(14, P.PixRow * 5 / 6);
							STRNSCPY(lf.lfFaceName, _T("Arial"));
							Ptb.SetFont(fontChunkText, ::CreateFontIndirect(&lf));
						}
					}
				}
				do_redraw = 1;
			}
			St.Rsz.Kind = 0;
			if(do_redraw) {
				{
					Area a2;
					GetArea(a2);
					CalcChunkRect(&a2, RL);
				}
				invalidateAll(1);
				::UpdateWindow(H());
			}
			::SetCapture(H());
			::SetCursor(::LoadCursor(0, IDC_ARROW));
			ok = -1;
		}
	}
	else if(mode == 2) {
		//
		// изменение размеров
		//
		if(St.Rsz.Kind == ResizeState::kRescale) {
			int    delta = -(int)(p.x - St.Rsz.Org.x);
			long   new_pix_quant = P.PixQuant + (delta * (int)P.Quant) / (int)St.HdrLevel[St.Rsz.HdrLevel];
			if(new_pix_quant >= 2 && new_pix_quant <= 100)
				St.Rsz.Shift = delta;
			do_redraw = 1;
		}
		else if(St.Rsz.Kind == ResizeState::kMoveChunk) {
			if(Locate(p, &loc) > 0 && oneof2(loc.Kind, Loc::kWorkspace, Loc::kChunk)) {
				int    delta = (int)(p.x - St.Rsz.Org.x);
				STimeChunkAssoc grid_item;
				if(P_Data->GetChunk(St.Rsz.ChunkId, 0, &grid_item)) {
					grid_item.Chunk.Start.addsec(PixToSec(delta));
					grid_item.Chunk.Finish.addsec(PixToSec(delta));
					int r = P_Data->MoveChunk(STimeChunkGrid::mmCanMove, St.Rsz.ChunkId, /*St.Rsz.RowId*/loc.RowId, grid_item.Chunk);
					St.Rsz.RowId = loc.RowId;
					St.Rsz.Shift = delta;
					do_redraw = 1;
				}
			}
		}
		else if(oneof2(St.Rsz.Kind, ResizeState::kChunkLeft, ResizeState::kChunkRight)) {
			int    delta = (int)(p.x - St.Rsz.Org.x);
			STimeChunkAssoc grid_item;
			if(P_Data->GetChunk(St.Rsz.ChunkId, 0, &grid_item)) {
				int mm;
				if(St.Rsz.Kind == ResizeState::kChunkLeft) {
					grid_item.Chunk.Start.addsec(PixToSec(delta));
					mm = STimeChunkGrid::mmCanResizeLeft;
				}
				else {
					grid_item.Chunk.Finish.addsec(PixToSec(delta));
					mm = STimeChunkGrid::mmCanResizeRight;
				}
				if(cmp(grid_item.Chunk.Start, grid_item.Chunk.Finish) < 0) {
					int r = P_Data->MoveChunk(mm, St.Rsz.ChunkId, St.Rsz.RowId, grid_item.Chunk);
					St.Rsz.Shift = delta;
					do_redraw = 1;
				}
			}
		}
		else if(St.Rsz.Kind == ResizeState::kSplit) {
			int    delta = (int)(p.x - St.Rsz.Org.x);
			Area   a2;
			GetArea(a2);
			uint   new_part = p.x * 100 / a2.Full.width();
			if(new_part >= 1 && new_part < 80) {
				St.Rsz.Shift = delta;
				do_redraw = 1;
			}
			invalidateRect(St.Rsz.Prev, 1);
			ok = 1;
		}
		else if(St.Rsz.Kind == ResizeState::kRowHeight) {
			int    delta = (p.y - St.Rsz.Org.y);
			TRect  rect;
			int    order = 0;
			if(GetRowRect(St.Rsz.RowId, 1, &order, &rect) > 0) {
				uint row_full_height = (P.PixRow * order) + 2 * P.PixRowMargin;
				uint new_height = row_full_height + delta;
				uint new_pix_row = P.PixRow * new_height / row_full_height;
				uint new_pix_mgn = P.PixRowMargin * new_height / row_full_height;
				if(new_pix_row >= 3 && new_pix_row <= 100) {
					St.Rsz.Shift = delta;
					do_redraw = 1;
				}
			}
		}
		else if(St.Rsz.Kind == ResizeState::kScroll) {
			Area   a2;
			TPoint delta = p - St.Rsz.Org;
			if(delta.x) {
				GetArea(a2);
				int scroll_delta = delta.x / a2.PixQuant;
				Scroll(SB_HORZ, SB_THUMBPOSITION, St.ScrollX + scroll_delta);
			}
			if(delta.y) {
			}
			St.Rsz.Org = p;
		}
		if(do_redraw) {
			{
				Area a2;
				GetArea(a2);
				CalcChunkRect(&a2, RL);
			}
			InvalidateResizeArea();
			::UpdateWindow(H());
		}
		ok = 1;
	}
	return ok;
}

int STimeChunkBrowser::Locate(TPoint p, Loc * pLoc) const
{
	int    ok = -1;
	Loc    loc;
	MEMSZERO(loc);
	loc.EntryId = -1;
	long   s;
	Area a2;
	GetArea(a2);
	STimeChunk view_time_bounds = GetBoundsTime(a2);
	TRect  rect;
	const  uint left_edge = a2.Right.a.x;
	if(!a2.PicMode.IsEmpty() && a2.PicMode.contains(p)) {
		loc.Kind = Loc::kPicMode;
		loc.Pos  = Loc::pInner;
		ok = 1;
	}
	else if(a2.RightHeader.contains(p)) {
		//
		// Header
		//
		if(St.HdrLevelCount) {
			int    y = a2.RightHeader.a.y;
			uint   i = St.HdrLevelCount-1;
			uint   dot_per_sec = (P.PixQuant / P.Quant);
			do {
				const  uint unit = St.HdrLevel[i];
				uint   dot_per_unit = dot_per_sec / unit;
				y += P.HdrLevelHeight;
				if(p.y >= (int)(y - P.HdrLevelHeight) && p.y <= y) {
					//
					// Вертикальные засечки
					//
					for(LDATETIME dtm = St.Bounds.Start; ok < 0 && cmp(dtm, view_time_bounds.Finish) <= 0; dtm.addsec(unit)) {
						s = DiffTime(dtm, view_time_bounds.Start);
						if(s >= 0) {
							const int x = left_edge + SecToPix(s);
							const int next_x = (int)(x + SecToPix(unit)) - 1;
							if(p.y >= (y-5) && p.y <= y && p.x >= (x-3) && p.x <= (x+3)) { // @v6.9.8 (+-1)-->(+-)3
								loc.Kind = Loc::kHeader;
								loc.Pos  = Loc::pLeftEdge;
								loc.Tm   = loc.TmQuant = dtm;
								loc.HdrLevel = i;
								ok = 1;
							}
							else if(p.x >= x && p.x < next_x) {
								loc.Kind = Loc::kHeader;
								loc.Pos  = Loc::pInner;
								loc.Tm   = loc.TmQuant = dtm;
								loc.Tm.addsec(PixToSec(p.x - x));
								loc.HdrLevel = i;
								ok = 1;
							}
						}
					}
				}
			} while(ok < 0 && i--);
		}
	}
	else if(a2.Right.contains(p)) {
		//
		// Main area
		//
		const  SRect * p_sr = RL.SearchPoint(p);
		if(p_sr) {
			loc.Kind = Loc::kChunk;
			loc.Tm   = loc.TmQuant = view_time_bounds.Start;
			loc.Tm.addsec(PixToSec(p.x - left_edge));
			loc.TmQuant.addsec((PixToSec(p.x - left_edge) / P.Quant) * P.Quant);
			loc.RowId   = p_sr->RowId;
			loc.EntryId = p_sr->C.Id;
			loc.Chunk   = p_sr->C.Chunk;
			loc.Pos = 0;
			if(p_sr->C.Id == St.SelChunkId) {
				int x_ = p_sr->a.x;
				int y_ = p_sr->a.y+2;
				#define CP(x1, x2, y1) (p.x >= x_+x1 && p.x <= x_+x2 && p.y == y_+y1)
				if(
					CP(6, 6, 0) ||
					CP(5, 7, 1) ||
					CP(4, 8, 2) ||
					CP(3, 9, 3) ||
					CP(2,10, 4) ||
					CP(3, 9, 5) ||
					CP(4, 8, 6) ||
					CP(5, 7, 7) ||
					CP(6, 6, 8)) {
					loc.Pos = Loc::pMoveSpot;
				}
				#undef  CP
			}
			if(loc.Pos == 0) {
				if(p.x == p_sr->a.x)
					loc.Pos = Loc::pLeftEdge;
				else if(p.x == rect.b.x)
					loc.Pos = Loc::pRightEdge;
				else if(p.y == rect.a.y)
					loc.Pos = Loc::pTopEdge;
				else if(p.y == rect.b.y)
					loc.Pos = Loc::pBottomEdge;
				else
					loc.Pos = Loc::pInner;
			}
		}
		else {
			loc.Kind = Loc::kWorkspace;
			loc.RowId = 0;
			if(P.ViewType == Param::vHourDay) {
				const  long qc = (p.x - a2.Right.a.x) / a2.PixQuant;
				const  long days_per_screen = a2.Right.width() / a2.PixQuant;
				long   i = 0, q = 0;
				while(q < qc && i <= days_per_screen) {
					i++;
					if(IsQuantVisible(St.ScrollX + i))
						q++;
				}
				if(i < days_per_screen)
					loc.Tm.d = plusdate(view_time_bounds.Start.d, i);
				else
					loc.Tm.d = ZERODATE;
				double rt = view_time_bounds.Start.t.hour() + (double)(p.y - a2.Right.a.y) / (double)a2.PixPerHour;
				loc.Tm.t = encodetime((int)rt, (int)(ffrac(rt) * 60.0), (int)ffrac(ffrac(rt) * 60.0), 0);
				loc.TmQuant = loc.Tm;
			}
			else {
				loc.Tm = loc.TmQuant = view_time_bounds.Start;
				loc.Tm = AddTime(view_time_bounds.Start, PixToSec(p.x - left_edge));
				loc.TmQuant = AddTime(view_time_bounds.Start, (PixToSec(p.x - left_edge) / P.Quant) * P.Quant);
				uint   upp_edge = a2.Right.a.y;
				for(uint i = St.ScrollY; !loc.RowId && upp_edge <= (uint)a2.Right.b.y && i < P_Data->getCount(); i++) {
					const STimeChunkAssocArray * p_row = P_Data->at(i);
					const  RowState & r_rowst = GetRowState(p_row->Id);
					const  uint row_full_height = (P.PixRow * NZOR(r_rowst.Order, 1)) + 2 * P.PixRowMargin;
					if(p.y >= (int)upp_edge && p.y < (int)(upp_edge + row_full_height))
						loc.RowId   = p_row->Id;
					upp_edge += row_full_height;
				}
			}
			loc.EntryId = -1;
			loc.Pos = 0;
		}
		ok = 1;
	}
	else if(a2.LeftHeader.contains(p)) {
		loc.Kind = Loc::kLeftHeader;
		ok = 1;
	}
	else if(a2.Left.contains(p)) {
		loc.Kind = Loc::kLeftZone;
		uint   upp_edge = a2.Left.a.y;
		for(uint i = St.ScrollY; ok < 0 && upp_edge <= (uint)a2.Left.b.y && i < P_Data->getCount(); i++) {
			const STimeChunkAssocArray * p_row = P_Data->at(i);
			const  RowState & r_rowst = GetRowState(p_row->Id);
			const  uint row_full_height = (P.PixRow * NZOR(r_rowst.Order, 1)) + 2 * P.PixRowMargin;
			if(p.y == (int)(upp_edge + row_full_height)) {
				loc.RowId = p_row->Id;
				loc.EntryId = -1;
				loc.Pos = Loc::pBottomEdge;
				ok = 1;
			}
			else if(p.y >= (int)upp_edge && p.y < (int)(upp_edge + row_full_height)) {
				loc.RowId = p_row->Id;
				loc.EntryId = -1;
				loc.Pos = 0;
				ok = 1;
			}
			upp_edge += row_full_height;
		}
	}
	else if(a2.Separator.contains(p)) {
		loc.Kind = Loc::kSeparator;
		ok = 1;
	}
	ASSIGN_PTR(pLoc, loc);
	return ok;
}

int FASTCALL STimeChunkBrowser::GetArea(Area & rArea) const
{
	int    ok = -1;
	if(!(rArea.Flags & Area::fInited)) {
		const  int half_separator_width = 3;
		TRect  margin = GetMargin();
		rArea.Full = getClientRect();
		if(St.TextZonePart > 1 && St.TextZonePart <= 80) { // left + separator + right
			const  int x = rArea.Full.a.x + (rArea.Full.width() * St.TextZonePart) / 100;
			rArea.Separator = rArea.Full;
			rArea.Separator.a.x = x - half_separator_width + 1;
			rArea.Separator.b.x = x + half_separator_width - 1;
			rArea.Separator.setmarginy(margin);
			rArea.Left = rArea.Full;
			rArea.Left.b.x = x - half_separator_width;
			rArea.Left.setmarginx(margin);
			rArea.Right = rArea.Full;
			rArea.Right.a.x = x + half_separator_width;
			rArea.Right.setmarginx(margin);

			rArea.LeftHeader = rArea.Left;
			rArea.LeftHeader.b.y = rArea.LeftHeader.a.y + (P.HdrLevelHeight * St.HdrLevelCount);

			rArea.Left.a.y = rArea.LeftHeader.b.y + 1;
			rArea.Left.setmarginy(margin);

			if(rArea.LeftHeader.width() >= 20 && rArea.LeftHeader.height() >= 20) {
				rArea.PicMode.setwidthrel(rArea.LeftHeader.a.x, 16);
				rArea.PicMode.setheightrel(rArea.LeftHeader.a.x, 16);
				rArea.PicMode.move(2, 2);
			}
			ok = 1;
		}
		else if(St.TextZonePart == 1) { // separator + right
			rArea.Separator = rArea.Full;
			rArea.Separator.b.x = half_separator_width * 2 - 1;
			rArea.Separator.setmarginy(margin);
			rArea.Right = rArea.Full;
			rArea.Right.a.x += half_separator_width * 2;
			rArea.Right.setmarginx(margin);
			MEMSZERO(rArea.Left);
			MEMSZERO(rArea.LeftHeader);
			ok = 2;
		}
		else { // right only
			rArea.Right = rArea.Full;
			rArea.Right.setmarginx(margin);
			MEMSZERO(rArea.Left);
			MEMSZERO(rArea.LeftHeader);
			MEMSZERO(rArea.Separator);
			ok = 3;
		}
		rArea.RightHeader = rArea.Right;
		rArea.RightHeader.b.y = rArea.RightHeader.a.y + (P.HdrLevelHeight * St.HdrLevelCount);
		rArea.Right.a.y = rArea.RightHeader.b.y + 1;
		rArea.Right.setmarginy(margin);
		//
		//
		//
		if(P.ViewType == Param::vHourDay) {
			const  uint min_pix_per_day = 20;
			const  uint pix_per_hour = 60;
			uint   days_per_screen = NZOR(P.DaysCount, 7);
			uint   pix_per_day = rArea.Right.width() / days_per_screen;
			if(pix_per_day < min_pix_per_day) {
				pix_per_day = min_pix_per_day;
				days_per_screen = rArea.Right.width() / pix_per_day;
			}
			rArea.Quant = 3600 * 24;
			rArea.PixQuant = pix_per_day;
			rArea.PixPerHour = 60;
		}
		else {
			rArea.Quant = P.Quant;
			rArea.PixQuant = P.PixQuant;
			rArea.PixPerHour = 60;
		}
		//
		rArea.Flags |= Area::fInited;
	}
	return ok;
}

int STimeChunkBrowser::InvalidateResizeArea()
{
	int    ok = 0;
	Area   a2;
	TRect rect;
	if(St.Rsz.Kind == ResizeState::kRescale) {
		GetArea(a2);
		if(St.HdrLevelCount) {
			uint   i = St.HdrLevelCount-1;
			int    y = a2.Full.a.y;
			do {
				y += P.HdrLevelHeight;
				if(St.Rsz.HdrLevel == i) {
					invalidateRect(rect.setwidth(a2.Right).setheightrel(y-5, 5), 0);
					ok = 1;
				}
			} while(!ok && i--);
		}
	}
	else if(St.Rsz.Kind == ResizeState::kMoveChunk) {
		GetRowRect(St.Rsz.RowId, 2, 0, &rect);
		invalidateRect(rect, 0);
		invalidateRect(St.Rsz.Prev, 0);
		ok = 1;
	}
	else if(oneof2(St.Rsz.Kind, ResizeState::kChunkLeft, ResizeState::kChunkRight)) {
		GetArea(a2);
		if(St.Rsz.ChunkId > 0) {
			for(uint i = 0; i < RL.getCount(); i++) {
				const SRect & r_sr = RL.at(i);
				if(r_sr.C.Id == St.Rsz.ChunkId) {
					TRect cr = r_sr;
					invalidateRect(cr.setwidth(a2.Right), 0);
					ok = 1;
				}
			}
		}
	}
	else if(St.Rsz.Kind == ResizeState::kSplit) {
		GetArea(a2);
		int    c = (int)(St.Rsz.Org.x + St.Rsz.Shift);
		invalidateRect(rect.setwidthrel(MAX(0, c - 6), 12).setheight(a2.Full), 0);
		ok = 1;
	}
	else if(St.Rsz.Kind == ResizeState::kRowHeight) {
		ok = GetRowRect(St.Rsz.RowId, 1, 0, &rect);
		if(ok > 0) {
			if(St.Rsz.Shift > 0)
				rect.b.y += (int16)((rect.height() + St.Rsz.Shift) * 2);
			else
				rect.b.y += (rect.height() * 2);
			invalidateRect(rect, 0);
		}
	}
	else if(St.Rsz.Kind == ResizeState::kSwitchMode) {
		ok = 1;
	}
	if(ok <= 0)
		invalidateAll(1);
	return ok;
}
//
// ARG(side IN): 0 - full, 1 - left, 2 - right
//
int STimeChunkBrowser::GetRowRect(long rowId, int side, int * pOrder, TRect * pRect) const
{
	int    ok = -1;
	Area a2;
	GetArea(a2);
	uint   upp_edge = a2.Left.a.y;
	for(uint i = St.ScrollY; ok < 0 && upp_edge <= (uint)a2.Left.b.y && i < P_Data->getCount(); i++) {
		const STimeChunkAssocArray * p_row = P_Data->at(i);
		const  RowState & r_rowst = GetRowState(p_row->Id);
		const  uint row_full_height = (P.PixRow * NZOR(r_rowst.Order, 1)) + 2 * P.PixRowMargin;
		if(p_row->Id == St.Rsz.RowId) {
			TRect rect;
			if(side == 0)
				rect.setwidth(a2.Full);
			else if(side == 1)
				rect.setwidth(a2.Left);
			else if(side == 2)
				rect.setwidth(a2.Right);
			rect.setheightrel(upp_edge, row_full_height);
			ASSIGN_PTR(pOrder, NZOR(r_rowst.Order, 1));
			ASSIGN_PTR(pRect, rect);
			ok = 1;
		}
		upp_edge += row_full_height;
	}
	return ok;
}

static int AddIntersectRectPair(uint p1, uint p2, TSCollection <LongArray> & rList)
{
	int    ok = 0;
	int    s1 = -1, s2 = -1;
	uint   i;
	assert(p1 < p2);
	for(i = 0; i < rList.getCount(); i++) {
		LongArray & r_item = *rList.at(i);
		if(r_item.lsearch(p1)) {
			assert(s1 == -1);
			s1 = (int)i;
		}
		if(r_item.lsearch(p2)) {
			assert(s2 == -1);
			s2 = (int)i;
		}
	}
	if(s1 >= 0) {
		if(s2 >= 0) {
			if(s1 == s2) {
				; // done
			}
			else {
				//
				// Объединяем списки s1 и s2
				//
				LongArray & r_item = *rList.at(s1);
				LongArray * p_item2 = rList.at(s2);
				r_item.addUnique(p_item2);
				p_item2 = 0;
				rList.atFree(s2);
			}
		}
		else {
			LongArray & r_item = *rList.at(s1);
			assert(!r_item.lsearch(p2));
			r_item.add(p2);
		}
	}
	else if(s2 >= 0) {
		LongArray & r_item = *rList.at(s2);
		assert(!r_item.lsearch(p1));
		r_item.add(p1);
	}
	else {
		LongArray * p_new_item = rList.CreateNewItem();
		p_new_item->add(p1);
		p_new_item->add(p2);
	}
	//
	// @test {
	//
	{
		const uint lc = rList.getCount();
		for(i = 0; i < lc; i++) {
			LongArray & r_item = *rList.at(i);
			const uint ic = r_item.getCount();
			for(uint k = 0; k < ic; k++) {
				const long test = r_item.get(k);
				for(uint j = i+1; j < lc; j++) {
					assert(!rList.at(j)->lsearch(test));
				}
			}
		}
	}
	// } @test
	return ok;
}

int STimeChunkBrowser::CalcChunkRect(const Area * pArea, SRectArray & rRectList)
{
	int    ok = 0;
	Area   a2;
	if(!pArea) {
		GetArea(a2);
		pArea = &a2;
	}
	STimeChunk view_time_bounds = GetBoundsTime(*pArea);
	const  uint dc = P_Data->getCount();
	const  uint left_edge = pArea->Right.a.x;
	uint   upp_edge  = pArea->Right.a.y;
	rRectList.clear();
	if(P.ViewType == Param::vHourDay) {
		const  int x_gap = 5;
		uint   i;
		STimeChunk sect;
		LTIME  start_tm = view_time_bounds.Start.t;
		LTIME  end_tm = view_time_bounds.Finish.t;
		const  double vpix_per_sec = (double)pArea->PixPerHour / 3600.0;
		uint   last_i = 0;
		if(P.SingleRowIdx >= 0 && P.SingleRowIdx < (long)dc) {
			i = (uint)P.SingleRowIdx;
			last_i = (uint)P.SingleRowIdx + 1;
		}
		else {
			i = 0;
			last_i = dc;
		}
		for(; i < last_i; i++) {
			const  STimeChunkAssocArray * p_row = P_Data->at(i);
			SRect  srect;
			srect.setheightrel(upp_edge + P.PixRowMargin, P.PixRow);
			for(uint k = 0; k < p_row->getCount(); k++) {
				const  STimeChunkAssoc * p_chunk = (STimeChunkAssoc *)p_row->at(k);
				srect.Clear();
				if(p_chunk->Chunk.Intersect(view_time_bounds, &sect) > 0) {
					uint   day_n = 0;
					for(long quant = St.ScrollX; ; quant++) {
						const  LDATE  dt = plusdate(St.Bounds.Start.d, quant);
						if(dt <= view_time_bounds.Finish.d) {
							if(IsQuantVisible(quant)) {
								STimeChunk day_chunk, day_sect;
								day_chunk.Start.Set(dt, start_tm);
								day_chunk.Finish.Set(dt, end_tm);
								if(p_chunk->Chunk.Intersect(day_chunk, &day_sect) > 0) {
									assert(day_sect.Start.d == dt);
									assert(day_sect.Finish.d == dt);

									srect.a.y = upp_edge + (int)(vpix_per_sec * ::DiffTime(day_sect.Start.t, start_tm, 3));
									srect.b.y = upp_edge + (int)(vpix_per_sec * ::DiffTime(day_sect.Finish.t, start_tm, 3));
									srect.a.x = left_edge + pArea->PixQuant * day_n + x_gap;
									srect.b.x = left_edge + pArea->PixQuant * (day_n+1) - x_gap;
									srect.C = *p_chunk;
									srect.DayN = day_n;
									srect.RowId = p_row->Id;
									rRectList.insert(&srect);
									ok = 1;
								}
								day_n++;
							}
						}
						else
							break;
					}
				}
			}
		}
		{
			const uint c = rRectList.getCount();
			TSCollection <LongArray> intersect_list;
			for(i = 0; i < c; i++) {
				SRect & r1 = rRectList.at(i);
				for(uint j = i+1; j < c; j++) {
					SRect & r2 = rRectList.at(j);
					TRect isect_rect;
					if(r1.Intersect(r2, &isect_rect) > 0 && isect_rect.height() > 1) {
						AddIntersectRectPair(i, j, intersect_list);
					}
				}
			}
			for(i = 0; i < intersect_list.getCount(); i++) {
				LongArray & r_item = *intersect_list.at(i);
				uint ic = r_item.getCount();
				assert(ic);
				if(ic) {
					int    iw = (pArea->PixQuant - 2*x_gap) / ic;
					for(uint j = 0; j < ic; j++) {
						SRect & r1 = rRectList.at(r_item.get(j));
						r1.a.x = left_edge + pArea->PixQuant * r1.DayN + (iw * j) + x_gap;
						r1.b.x = r1.a.x + iw;
					}
				}
			}
		}
	}
	else {
		for(uint i = St.ScrollY; upp_edge <= (uint)pArea->Right.b.y && i < dc; i++) {
			const  STimeChunkAssocArray * p_row = P_Data->at(i);
			SRect  srect;
			srect.setheightrel(upp_edge + P.PixRowMargin, P.PixRow);
			const  RowState & r_rowst = GetRowState(p_row->Id);
			const  uint row_full_height = (P.PixRow * NZOR(r_rowst.Order, 1)) + 2 * P.PixRowMargin;
			for(uint k = 0; k < p_row->getCount(); k++) {
				const  STimeChunkAssoc * p_chunk = (STimeChunkAssoc *)p_row->at(k);
				STimeChunk sect;
				if(p_chunk->Chunk.Intersect(view_time_bounds, &sect) > 0) {
					long   o = 0;
					srect.a.y = upp_edge + P.PixRowMargin;
					if(r_rowst.Order > 1 && r_rowst.OrderList.BSearch((long)(k+1), &o, 0) && o)
						srect.a.y += (int16)(P.PixRow * o);
					srect.b.y = srect.a.y + P.PixRow;
					srect.C = *p_chunk;
					srect.DayN = 0;
					srect.RowId = p_row->Id;
					ChunkToRectX(left_edge, sect, view_time_bounds.Start, srect);
					rRectList.insert(&srect);
					ok = 1;
				}
			}
			upp_edge += row_full_height;
		}
	}
	{
		//
		// ToolTips
		//
		if(H()) {
			if(!P_Tt)
				P_Tt = new TToolTip(H(), 400);
			if(P_Tt) {
				SString msg_buf;
				P_Tt->RemoveAllTools();
				for(uint i = 0; i < rRectList.getCount(); i++) {
					const SRect & r_sr = rRectList.at(i);
					if(P_Data->GetText(STimeChunkGrid::iChunkBallon, r_sr.C.Id, msg_buf) > 0) {
						TToolTip::ToolItem tt_item;
						tt_item.H = H();
						tt_item.R = r_sr;
						tt_item.Param = r_sr.C.Id;
						msg_buf.ReplaceChar('\003', ' ').Strip().Transf(CTRANSF_INNER_TO_OUTER);
						tt_item.Text = msg_buf;
						P_Tt->AddTool(tt_item);
					}
				}
			}
		}
	}
	return ok;
}

uint STimeChunkBrowser::GetBottomRowIdx(uint startIdx) const
{
	Area   a2;
	GetArea(a2);
	uint   i;
	uint   upp_edge  = a2.Right.a.y;
	for(i = startIdx; upp_edge <= (uint)a2.Right.b.y && i < P_Data->getCount(); i++) {
		const  STimeChunkAssocArray * p_row = P_Data->at(i);
		const  RowState & r_rowst = GetRowState(p_row->Id);
		upp_edge += (P.PixRow * NZOR(r_rowst.Order, 1)) + 2 * P.PixRowMargin;
	}
	if(i > startIdx)
		i = MAX(i-1, startIdx);
	else
		i = startIdx;
	return i;
}

int STimeChunkBrowser::ChunkToRectX(uint leftEdge, const STimeChunk & rChunk, const LDATETIME & rStart, TRect & rRect) const
{
	rRect.a.x = (int16)(leftEdge + SecToPix(DiffTime(rChunk.Start,  rStart)));
	rRect.b.x = (int16)(leftEdge + SecToPix(DiffTime(rChunk.Finish, rStart)));
	if(rRect.a.x > rRect.b.x) {
		int16 t = rRect.a.x;
		rRect.a.x = rRect.b.x;
		rRect.b.x = t;
		return 2;
	}
	else
		return 1;
}

int FASTCALL STimeChunkBrowser::SecToPix(long t) const  { return (((long)P.PixQuant) * t) / (long)P.Quant; }
long FASTCALL STimeChunkBrowser::PixToSec(int  p) const { return (((long)P.Quant) * p) / (long)P.PixQuant; }

long STimeChunkBrowser::DiffTime(const LDATETIME & rEnd, const LDATETIME & rStart) const
{
	long   diff = diffdatetimesec(rEnd, rStart);
	const  STimeChunkArray * p_collapse_list = GetCollapseList_();
	if(p_collapse_list) {
		STimeChunkArray intsect;
		STimeChunk chunk(rStart, rEnd);
		if(p_collapse_list->Intersect(chunk, &intsect)) {
			for(uint i = 0; i < intsect.getCount(); i++) {
				const STimeChunk * p_intsect_item = (const STimeChunk *)intsect.at(i);
				diff -= p_intsect_item->GetDuration();
			}
		}
	}
	return diff;
}

LDATETIME STimeChunkBrowser::AddTime(const LDATETIME & rStart, long sec) const
{
	long   adjusted_sec = sec;
	LDATETIME end;
	const  STimeChunkArray * p_collapse_list = GetCollapseList_();
	if(p_collapse_list && sec > 0) {
		int    more = 0;
		long   prev_sec = 0;
		STimeChunkArray intsect;
		STimeChunkArray temp_list;
		STimeChunk chunk;
		do {
			LDATETIME start;
			chunk.Init((start = rStart).addsec(prev_sec), (end = rStart).addsec(adjusted_sec));
			prev_sec = adjusted_sec;
			temp_list.clear();
			temp_list.Add(&chunk, 0);
			if(p_collapse_list->Intersect(&temp_list, &intsect)) {
				for(uint i = 0; i < intsect.getCount(); i++) {
					const STimeChunk * p_intsect_item = (const STimeChunk *)intsect.at(i);
					adjusted_sec += p_intsect_item->GetDuration();
					more = 1;
				}
			}
			else
				more = 0;
		} while(more);
	}
	return (end = rStart).addsec(adjusted_sec);
}

void STimeChunkBrowser::CalcHdTimeBounds(const Area & rArea, DateRange & rPeriod, uint & rMinHour, uint & rMaxHour) const
{
	const uint days_per_screen = rArea.Right.width() / rArea.PixQuant;
	rPeriod.low = plusdate(St.Bounds.Start.d, St.ScrollX);
	rPeriod.upp = (days_per_screen > 0) ? plusdate(rPeriod.low, days_per_screen-1) : rPeriod.low;
	rMinHour = 24;
	rMaxHour = 0;
	const STimeChunkArray * p_collapse_list = GetCollapseList_();
	if(p_collapse_list) {
		STimeChunkArray intersect, free_list;
		p_collapse_list->GetFreeList(&free_list);
		LDATE  limit_date = St.Bounds.Finish.d;
		SETIFZ(limit_date, encodedate(31, 12, 2100));
		for(LDATE dt = rPeriod.low; dt <= rPeriod.upp && dt <= limit_date; dt = plusdate(dt, 1)) {
			LDATETIME dtm1, dtm2;
			STimeChunk chunk(dtm1.Set(dt, ZEROTIME), dtm2.Set(dt, encodetime(23, 59, 59, 99)));
			intersect.clear();
			int    skip_date = 0;
			if(p_collapse_list->Intersect(chunk, &intersect)) {
				uint   i;
				for(i = 0; !skip_date && i < intersect.getCount(); i++) {
					const STimeChunk * p_ic = (const STimeChunk *)intersect.at(i);
					if(*p_ic == chunk) {
						if(dt == rPeriod.low)
							rPeriod.low = plusdate(rPeriod.low, 1);
						rPeriod.upp = plusdate(rPeriod.upp, 1);
						skip_date = 1;
					}
				}
				if(!skip_date) {
					int    r = free_list.Intersect(chunk, &intersect);
					assert(r);
					for(i = 0; i < intersect.getCount(); i++) {
						const STimeChunk * p_ic = (const STimeChunk *)intersect.at(i);
						SETMIN(rMinHour, (uint)p_ic->Start.t.hour());
						SETMAX(rMaxHour, (uint)p_ic->Finish.t.hour());
					}
				}
			}
		}
	}
	if(rMinHour == 24)
		rMinHour = 0;
	if(rMaxHour == 0)
		rMaxHour = 23;
}

STimeChunk FASTCALL STimeChunkBrowser::GetBoundsTime(const Area & rArea) const
{
	STimeChunk view_time_bounds;
	const STimeChunkArray * p_collapse_list = GetCollapseList_();
	//const uint cc = p_collapse_list ? p_collapse_list->getCount() : 0;
	const uint cc = SVectorBase::GetCount(p_collapse_list);
	if(P.ViewType == Param::vHourDay) {
		/*
		const uint  days_per_screen = rArea.Right.width() / rArea.PixQuant;
		LDATE start_dt = plusdate(St.Bounds.Start.d, St.ScrollX);
		view_time_bounds.Start.d = start_dt;
		view_time_bounds.Start.t = encodetime(St.ScrollY, 0, 0, 0);
		view_time_bounds.Finish.d = (days_per_screen > 0) ? plusdate(start_dt, days_per_screen-1) : start_dt;
		view_time_bounds.Finish.t = encodetime(MIN(23, St.ScrollY + rArea.Left.height()/rArea.PixPerHour), 59, 59, 99);
		*/
		//
		uint  min_hour, max_hour;
		DateRange period;
		CalcHdTimeBounds(rArea, period, min_hour, max_hour);
		view_time_bounds.Start.d = period.low;
		view_time_bounds.Start.t = encodetime(min_hour + St.ScrollY, 0, 0, 0);
		view_time_bounds.Finish.d = period.upp;
		view_time_bounds.Finish.t = encodetime(MIN(max_hour, min_hour + St.ScrollY + rArea.Left.height()/rArea.PixPerHour), 59, 59, 99);
	}
	else {
		const uint area_pix_width = rArea.Right.width();
		uint width = area_pix_width / P.PixQuant; // Ширина области видимости в квантах
		(view_time_bounds.Start = St.Bounds.Start).addsec(St.ScrollX * P.Quant);
		if(p_collapse_list) {
			uint   added_quant_width = 0;
			uint   pix_width = 0;
			LDATETIME start_tm = view_time_bounds.Start;
			for(uint i = 0; i < cc; i++) {
				const STimeChunk * p_chunk = (const STimeChunk *)p_collapse_list->at(i);
				if(cmp(p_chunk->Start, view_time_bounds.Start) >= 0) {
					long   dur = p_chunk->GetDuration();
					if(dur > 0) {
						added_quant_width += (dur / P.Quant);
						pix_width += diffdatetimesec(p_chunk->Start, start_tm) / P.Quant * P.PixQuant;
						start_tm = p_chunk->Finish;
					}
					if(pix_width > (area_pix_width + 64)) // (+64) - допуск для безопасности
						break;
				}
			}
			width += added_quant_width;
		}
		(view_time_bounds.Finish = St.Bounds.Start).addsec((St.ScrollX + width) * P.Quant);
	}
	return view_time_bounds;
}

int STimeChunkBrowser::GetChunkText(long chunkId, SString & rBuf)
{
	int    ok = -1;
	if(ChunkTextCache.GetText(chunkId, rBuf))
		ok = 1;
	else if(P_Data->GetText(STimeChunkGrid::iChunk, chunkId, rBuf) > 0) {
		ChunkTextCache.Add(chunkId, rBuf./* @v8.9.10 ReplaceCR().*/ToChar(), 1);
		ok = 1;
	}
	return ok;
}

int STimeChunkBrowser::GetChunkColor(const STimeChunkAssoc * pChunk, STimeChunkGrid::Color * pClr) const
{
	int    ok = 0;
	STimeChunkGrid::Color clr;
	clr.Status = -1;
	clr.C = GetColorRef(SClrBlack);
	if(P_Data->GetColor(pChunk->Id, &clr) > 0) {
		ok = 1;
	}
	ASSIGN_PTR(pClr, clr);
	return ok;
}


int STimeChunkBrowser::SelectChunkColor(const STimeChunkAssoc * pChunk, HBRUSH * pBrush)
{
	int    ok = 1;
	HBRUSH b = 0;
	long   val = 0;
	if(pChunk->Status >= 0 && ChunkColorCache.Search(pChunk->Status, &val, 0) > 0) {
		b = (HBRUSH)val;
	}
	else {
		STimeChunkGrid::Color clr;
		if(GetChunkColor(pChunk, &clr)) {
			if(ColorBrushList.Search((long)clr.C, &val, 0) > 0)
				b = (HBRUSH)val;
			else {
				b = ::CreateSolidBrush(clr.C);
				ColorBrushList.Add((long)clr.C, (long)b, 0);
			}
			if(clr.Status >= 0)
				ChunkColorCache.Add(clr.Status, (long)b, 0);
		}
		else
			ok = 0;
	}
	ASSIGN_PTR(pBrush, b);
	return ok;
}

void STimeChunkBrowser::DrawMoveSpot(TCanvas & rCanv, TPoint p)
{
	if(p.x || p.y) {
		rCanv.SelectObjectAndPush(Ptb.Get(penSelectedChunk));
		int x = p.x;
		int y = p.y;
		rCanv.LineHorz(x + 6, x +  6, y++);
		rCanv.LineHorz(x + 5, x +  7, y++);
		rCanv.LineHorz(x + 4, x +  8, y++);
		rCanv.LineHorz(x + 3, x +  9, y++);
		rCanv.LineHorz(x + 2, x + 10, y++);
		rCanv.LineHorz(x + 3, x +  9, y++);
		rCanv.LineHorz(x + 4, x +  8, y++);
		rCanv.LineHorz(x + 5, x +  7, y++);
		rCanv.LineHorz(x + 6, x +  6, y++);
		rCanv.PopObject();
	}
}

int STimeChunkBrowser::ExportToExcel()
{
	return -1;
}

int STimeChunkBrowser::CopyToClipboard()
{
	int    ok = 1;
	if(P.ViewType == P.vHourDay) {
		const char * p_fontface_tnr = "Times New Roman";
		long   column = 0;
		long   row = 0;
		SString temp_buf;
		SString out_buf;
		SString dow_buf;
		SString dec;
		SylkWriter sw(0);
		sw.PutRec("ID", "PPapyrus");
		{
			char   buf[64];
			::GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL, buf, sizeof(buf));
			dec.Cat(buf);
		}
		OpenClipboard(APPL->H_MainWnd);
		EmptyClipboard();
		sw.PutFont('F', p_fontface_tnr, 10, slkfsBold);
		sw.PutFont('F', p_fontface_tnr, 8,  0);
		sw.PutRec('F', "G");
		{
			row = 1;
			column = 2;
			for(long quant = 0; ; quant++) {
				const  LDATE  dt = plusdate(St.Bounds.Start.d, quant);
				if(dt <= St.Bounds.Finish.d) {
					if(IsQuantVisible(quant)) {
						GetDayOfWeekText(dowtRuFull, dayofweek(&dt, 1), dow_buf);
						temp_buf.Z().Cat(dt, DATF_DMY).Space().Cat(dow_buf);

						sw.PutFormat("FC0L", 1, column, row);
						sw.PutFont('F', p_fontface_tnr, 10, slkfsBold);
						sw.PutVal(temp_buf, 0);
						column++;
					}
				}
				else
					break;
			}
		}
		{
			uint   time_quant = 15 * 60; // 15 minuts
			STimeChunkAssocArray chunk_list(0);
			SString cell_buf;
			row = 2;
			for(uint time_band = 0; time_band < 24 * 3600; time_band += time_quant, row++) {
				LTIME   tm_start;
				LTIME   tm_end;
				tm_start.settotalsec(time_band);
				tm_end.settotalsec(time_band+time_quant-1);

				sw.PutFormat("FC0L", row, 1, row);
				sw.PutFont('F', p_fontface_tnr, 10, slkfsBold);
				sw.PutVal(temp_buf.Z().Cat(tm_start, TIMF_HM), 1);

				column = 1;
				for(long quant = 0; ; quant++) {
					const  LDATE  dt = plusdate(St.Bounds.Start.d, quant);
					column++;
					if(dt <= St.Bounds.Finish.d) {
						STimeChunk range;
						range.Start.Set(dt, tm_start);
						range.Finish.Set(dt, tm_end);
						chunk_list.clear();
						if(P_Data->GetChunksByTime(range, chunk_list) > 0) {
							cell_buf.Z();
							for(uint i = 0; i < chunk_list.getCount(); i++) {
								const STimeChunkAssoc * p_chunk = (const STimeChunkAssoc *)chunk_list.at(i);
								GetChunkText(p_chunk->Id, temp_buf);
								if(cell_buf.NotEmpty())
									cell_buf.CR();
								cell_buf.Cat(temp_buf);
							}
							if(cell_buf.NotEmpty()) {
								sw.PutFormat("FC0L", row, column, row);
								sw.PutFont('F', p_fontface_tnr, 10, slkfsBold);
								sw.PutVal(cell_buf, 0);
							}
						}
					}
					else
						break;
				}
			}
		}
		sw.PutLine("E");
		sw.GetBuf(&out_buf);
		{
			HGLOBAL h_glb = ::GlobalAlloc(GMEM_MOVEABLE, (out_buf.Len() + 1));
			char * p_buf = (char *)GlobalLock(h_glb);
			out_buf.CopyTo(p_buf, out_buf.Len());
			p_buf[out_buf.Len()] = '\0';
			GlobalUnlock(h_glb);
			SetClipboardData(CF_SYLK, h_glb);
		}
		CloseClipboard();
	}
	else
		ok = -1;
	return ok;
}

void STimeChunkBrowser::Paint()
{
	PAINTSTRUCT ps;
	Area   a2;
	HDC    h_dc = ::BeginPaint(H(), &ps);
	const  int  use_buffer = 0; // @debug
	HDC    h_dc_mem = 0;
	HBITMAP h_bmp = 0;
	HBITMAP h_old_bmp = 0;
	RECT   cr;
	uint   i, k;
	TRect  rect;
	SString temp_buf, dow_buf;
	LDATETIME current = getcurdatetime_();
	if(use_buffer) {
		GetClientRect(H(), &cr);
		h_dc_mem = CreateCompatibleDC(ps.hdc);
		h_bmp = CreateCompatibleBitmap(ps.hdc, cr.right - cr.left, cr.bottom - cr.top);
		h_old_bmp = (HBITMAP)SelectObject(h_dc_mem, h_bmp);
		h_dc = h_dc_mem;
	}
	TCanvas canv(h_dc);
	GetArea(a2);
	canv.SetBounds(a2.Full);
	const  STimeChunk view_time_bounds = GetBoundsTime(a2);
	const  uint left_edge = a2.Right.a.x;
	const  int  hinterlace = BIN(P.Flags & P.fInterlaced);
	const  STimeChunkArray * p_collapse_list = GetCollapseList_();
	uint   upp_edge  = a2.Right.a.y;
	::SetBkMode(canv, TRANSPARENT);
	if(P.ViewType == P.vHourDay) {
		int    x, y;
		uint   days_per_screen = a2.Right.width() / a2.PixQuant;
		STimeChunkArray isect_list;
		canv.SelectObjectAndPush(Ptb.Get(fontHeader));
		//
		// Main area background
		// Отрисовываем здесь из-за того, что при отрисовке заголовка выводятся вертикальные линии
		// высотой в полное окно - они не должны затираться отрисовкой фона
		//
		canv.SetBkColor(Ptb.GetColor(colorMain));
		canv.SetTextColor(Ptb.GetColor(colorBlackText));
		x = a2.Left.a.x;
		y = a2.Left.a.y;
		const int start_hour = view_time_bounds.Start.t.hour();
		const int end_hour = view_time_bounds.Finish.t.hour();
		for(i = start_hour; i <= (uint)end_hour; i++) {
			TRect ir(x, y, a2.Right.b.x, y + a2.PixPerHour);
			if(!hinterlace)
				ir.a.y++;
			canv.FillRect(ir, (HBRUSH)Ptb.Get((!hinterlace || (i % 2)) ? brushMain : brushInterleave));

			canv.SelectObjectAndPush(Ptb.Get(penMainQuantSeparator));
			canv.LineHorz(x, a2.Full.b.x, y + a2.PixPerHour/2);
			canv.PopObject();
			if(!hinterlace) {
				canv.SelectObjectAndPush(Ptb.Get(penDaySeparator));
				canv.LineHorz(x, a2.Full.b.x, y + a2.PixPerHour);
				canv.PopObject();
			}
			canv.SetBkColor(Ptb.GetColor((!hinterlace || (i % 2)) ? colorMain : colorInterleave));
			temp_buf.Z().Cat(encodetime(i, 0, 0, 0), TIMF_HM);
			TRect tr(x + 5, y + 2, x + a2.Right.b.x - 5, y + a2.PixPerHour - 2);
			canv.DrawText(tr, temp_buf, DT_LEFT | DT_END_ELLIPSIS);
			y += a2.PixPerHour;
		}
		//
		//
		//
		{
			//
			// Заливка фона заголовка
			//
			TRect ir(a2.LeftHeader.a.x, a2.RightHeader.a.y, a2.RightHeader.b.x, a2.RightHeader.b.y);
			canv.FillRect(ir, (HBRUSH)Ptb.Get(brushHeader));
			canv.SetBkColor(Ptb.GetColor(colorHeader));
		}
		//
		// Заголовок (дни)
		//
		canv.SetTextColor(Ptb.GetColor(colorWhiteText));

		x = left_edge;
		y = a2.Full.a.y;
		for(long quant = St.ScrollX; ; quant++) {
			const  LDATE  dt = plusdate(St.Bounds.Start.d, quant);
			if(dt <= view_time_bounds.Finish.d) {
				//assert(quant != St.ScrollX || dt == view_time_bounds.Start.d);
				if(IsQuantVisible(quant)) {
					x += a2.PixQuant;
					canv.SelectObjectAndPush(Ptb.Get(penDaySeparator));
					canv.LineVert(x, y, a2.Full.b.y);
					canv.PopObject();

					GetDayOfWeekText(4, dayofweek(&dt, 1), dow_buf);
					temp_buf.Z().Cat(dt, DATF_DMY).Space().Cat(dow_buf);
					TRect tr(x - a2.PixQuant + 2, y + 2, x - 2, y + P.HdrLevelHeight - 2);
					canv.DrawText(tr, temp_buf, DT_CENTER|DT_END_ELLIPSIS);
				}
			}
			else
				break;
		}
		//
		// Сепаратор
		//
		if(a2.Separator.b.x) {
			canv.FillRect(a2.Separator, (HBRUSH)Ptb.Get(brushHeader));
		}
		//
		// Main area
		//
		{
			TPoint move_spot; // Пятно в активном отрезке, при нажатии на которое его можно двигать
			move_spot = 0;    // Отрисовывается после общего цикла рисования основной области, дабы текст не затирал пятно.
			canv.SelectObjectAndPush(Ptb.Get(fontChunkText));
			canv.SelectObjectAndPush(Ptb.Get(penDefChunk));
			canv.SelectObjectAndPush(Ptb.Get(brushDefChunk)); // {
			for(i = 0; i < RL.getCount(); i++) {
				const  SRect & r_sr = RL.at(i);
				HBRUSH brush_chunk;
				//
				// Установить цвет заливки отрезка
				int    bs = SelectChunkColor(&r_sr.C, &brush_chunk);
				if(bs)
					canv.SelectObjectAndPush(brush_chunk);
				//
				if(r_sr.C.Id == St.SelChunkId) {
					canv.SelectObjectAndPush(Ptb.Get(penSelectedChunk));
					canv.Rectangle(r_sr);
					canv.PopObject();
					move_spot.Set(r_sr.a.x, r_sr.a.y+2);
				}
				else {
					canv.SelectObjectAndPush(Ptb.Get(penChunk));
					canv.Rectangle(r_sr);
					canv.PopObject();
				}
				//
				// Восстановление после установки цвета заливки отрезка
				if(bs)
					canv.PopObject();
				//
				if(GetChunkText(r_sr.C.Id, temp_buf) > 0) {
					TRect tr = r_sr;
					tr.setmarginx(2);
					tr.setmarginy(1);
					canv.DrawText(tr, temp_buf, DT_LEFT|DT_END_ELLIPSIS);
				}
			}
			DrawMoveSpot(canv, move_spot);
			canv.PopObjectN(3); // }
		}
		//
		canv.PopObject();
	}
	else {
		long   s;
		//
		// Main area background
		// Отрисовываем здесь из-за того, что при отрисовке заголовка выводятся вертикальные линии
		// высотой в полное окно - они не должны затираться отрисовкой фона
		//
		canv.SetBkColor(Ptb.GetColor(colorMain));
		for(i = St.ScrollY; upp_edge <= (uint)a2.Right.b.y && i < P_Data->getCount(); i++) {
			const  STimeChunkAssocArray * p_row = P_Data->at(i);
			const  RowState & r_rowst = GetRowState(p_row->Id);
			const  uint row_full_height = (P.PixRow * NZOR(r_rowst.Order, 1)) + 2 * P.PixRowMargin;
			TRect ir(a2.Left.a.x, upp_edge, a2.Right.b.x, upp_edge + row_full_height);
			if(!hinterlace)
				ir.a.y++;
			canv.FillRect(ir, (HBRUSH)Ptb.Get((!hinterlace || (i % 2)) ? brushMain : brushInterleave));
			canv.SetBkColor(Ptb.GetColor((!hinterlace || (i % 2)) ? colorMain : colorInterleave));
			if(!hinterlace) {
				canv.SelectObjectAndPush(Ptb.Get(penDaySeparator));
				canv.LineHorz(a2.Left.a.x, a2.Right.b.x, upp_edge + row_full_height);
				canv.PopObject();
			}
			upp_edge += row_full_height;
		}
		//
		// Завершающая нижняя горизонтальная полоса
		//
		if(upp_edge < (uint)a2.Right.b.y) {
			const  uint row_full_height = P.PixRow + 2 * P.PixRowMargin;
			TRect ir(a2.Left.a.x, upp_edge, a2.Right.b.x, upp_edge + row_full_height);
			if(!hinterlace)
				ir.a.y++;
			canv.FillRect(ir, (HBRUSH)Ptb.Get((!hinterlace || (i&2)) ? brushMain : brushInterleave));
			canv.SetBkColor(Ptb.GetColor((!hinterlace || (i&2)) ? colorMain : colorInterleave));
		}
		//
		// Header
		//
		canv.SetBkColor(Ptb.GetColor(colorHeader));
		canv.SetTextColor(Ptb.GetColor(colorWhiteText));
		canv.SelectObjectAndPush(Ptb.Get(penMainSeparator));
		if(St.HdrLevelCount) {
			i = St.HdrLevelCount-1;
			int    y = a2.Full.a.y;
			uint   dot_per_sec = (P.PixQuant / P.Quant);
			{
				//
				// Заливка фона заголовка
				//
				TRect ir(a2.LeftHeader.a.x, a2.RightHeader.a.y, a2.RightHeader.b.x, a2.RightHeader.b.y);
				canv.FillRect(ir, (HBRUSH)Ptb.Get(brushHeader));
				canv.SetBkColor(Ptb.GetColor(colorHeader));
			}
			do {
				const  uint unit = St.HdrLevel[i];
				uint   dot_per_unit = dot_per_sec / unit;
				y += P.HdrLevelHeight;
				//
				// Left area (horizontal line)
				//
				if(i == 0 && a2.Left.b.x)
					canv.LineHorz(a2.Left.a.x, a2.Right.b.x, y);
				else
					canv.LineHorz(a2.Right.a.x, a2.Right.b.x, y);
				//
				// Вертикальные засечки
				//
				canv.SelectObjectAndPush(Ptb.Get(fontHeader));
				//
				// Следующие три переменные необходимы для идентификации крайнего левого элемента шкалы,
				// если его левый край не находится в области отрисовки
				//
				uint   first_x = a2.Right.b.x;
				long   prev_s = MAXLONG;
				LDATETIME prev_dtm;
				prev_dtm.SetZero();
				LDATE  prev_date = ZERODATE; // Используется для идентификации нового дня.

				LAssocArray last_collapsed_day_list; // key - x, val - date

				for(LDATETIME dtm = St.Bounds.Start; cmp(dtm, view_time_bounds.Finish) <= 0; dtm.addsec(unit)) {
					if(!p_collapse_list || p_collapse_list->IsFreeEntry(dtm, unit, 0)) {
						s = DiffTime(dtm, view_time_bounds.Start);
						if(s >= 0) {
							const int x = left_edge + SecToPix(s);
							first_x = MIN((int)first_x, x);
							canv.LineVert(x, y, y-5);
							if(i != 0) {
								if(unit >= (3600 * 24)) {
									GetDayOfWeekText(4, dayofweek(&dtm.d, 1), dow_buf);
									temp_buf.Z().Cat(dtm.d, DATF_DMY).Space().Cat(dow_buf);
								}
								else
									temp_buf.Z().Cat(dtm.t, (unit % 60) ? TIMF_HMS : TIMF_HM);
								TRect tr(x+2, y - P.HdrLevelHeight*5/6, left_edge + SecToPix(s+unit), y);
								canv.DrawText(tr, temp_buf, DT_LEFT|DT_END_ELLIPSIS);
							}
							{
								HGDIOBJ h_brush = (dtm.d != prev_date /*dtm.t == ZEROTIME || dtm.t == encodetime(24, 0, 0, 0)*/) ?
									Ptb.Get(penDaySeparator) : Ptb.Get(i ? penQuantSeparator : penMainQuantSeparator);
								canv.SelectObjectAndPush(h_brush);
								canv.LineVert(x, y, a2.Right.b.y);
								canv.PopObject();
							}
							if(St.Rsz.Kind == ResizeState::kRescale && St.Rsz.HdrLevel == i) {
								if(St.Rsz.Quant == DiffTime(dtm, St.Bounds.Start) / unit) {
									TRect rescale_rect;
									rescale_rect.a.Set(x - St.Rsz.Shift, y-5);
									rescale_rect.b.Set(left_edge + SecToPix(s+unit), y);
									canv.SelectObjectAndPush(Ptb.Get(brushRescaleQuant));
									canv.Rectangle(rescale_rect);
									canv.PopObject();
								}
							}
						}
						else {
							prev_s = s;
							prev_dtm = dtm;
						}
						prev_date = dtm.d;
					}
					else if(unit >= (3600 * 24) && i != 0) {
						//
						// Для "коллапсированных" периодов необходимо отрисовывать переход от даты к дате.
						//
						s = DiffTime(dtm, view_time_bounds.Start);
						if(s >= 0) {
							const int x = left_edge + SecToPix(s);
							first_x = MIN((int)first_x, x);
							last_collapsed_day_list.Update(x, dtm.d.v, 0);
							/*
							canv.LineVert(x, y, y-5);
							GetDayOfWeekText(4, dayofweek(&dtm.d, 1), dow_buf);
							temp_buf.Z().Cat(dtm.d).Space().Cat(dow_buf);
							TRect tr(x+2, y - P.HdrLevelHeight*5/6, left_edge + SecToPix(s+unit), y);
							canv.DrawText(tr, temp_buf, DT_LEFT | DT_END_ELLIPSIS);
							{
								canv.SelectObjectAndPush(Ptb.Get(penDaySeparator));
								canv.LineVert(x, y, a2.Right.b.y);
								canv.PopObject();
							}
							*/
						}
						else {
							prev_s = s;
							prev_dtm = dtm;
						}
					}
				}
				{
					for(uint n = 0; n < last_collapsed_day_list.getCount(); n++) {
						const LAssoc & ra = last_collapsed_day_list.at(n);
						const int x = ra.Key;
						LDATE dt;
						dt.v = ra.Val;

						canv.LineVert(x, y, y-5);
						GetDayOfWeekText(4, dayofweek(&dt, 1), dow_buf);
						temp_buf.Z().Cat(dt, DATF_DMY).Space().Cat(dow_buf);
						TRect tr(x+2, y - P.HdrLevelHeight*5/6, left_edge + SecToPix(s+unit), y);
						canv.DrawText(tr, temp_buf, DT_LEFT | DT_END_ELLIPSIS);
						{
							canv.SelectObjectAndPush(Ptb.Get(penDaySeparator));
							canv.LineVert(x, y, a2.Right.b.y);
							canv.PopObject();
						}
					}
				}
				//
				// Отрисовка подписей засечки шкалы, которая находится левее крайнего обреза области. {
				//
				if(i != 0 && first_x > (left_edge+4) && prev_s < 0) {
					if(unit >= 3600 * 24) {
						GetDayOfWeekText(4, dayofweek(&prev_dtm.d, 1), dow_buf);
						temp_buf.Z().Cat(prev_dtm.d, DATF_DMY).Space().Cat(dow_buf);
					}
					else
						temp_buf.Z().Cat(prev_dtm.t, (unit % 60) ? TIMF_HMS : TIMF_HM);
					TRect tr(left_edge+2, y - P.HdrLevelHeight*5/6, first_x-2, y);
					canv.DrawText(tr, temp_buf, DT_LEFT|DT_END_ELLIPSIS);
				}
				//
				// }
				//
				canv.PopObject();
			} while(i--);
		}
		if(view_time_bounds.Has(current)) {
			//
			// Отрисовка вертикальной красной черты, обозначающей текущий момент
			//
			const int x = left_edge + SecToPix(DiffTime(current, view_time_bounds.Start));
			canv.SelectObjectAndPush(Ptb.Get(penCurrent));
			canv.LineVert(x, a2.RightHeader.a.y, a2.Right.b.y);
			canv.PopObject();
		}
		//
		// Main area
		//
		upp_edge = a2.Right.a.y;
		canv.SetBkColor(Ptb.GetColor(colorMain));
		canv.SelectObjectAndPush(Ptb.Get(fontLeftText));
		canv.SelectObjectAndPush(Ptb.Get(penDefChunk));
		canv.SelectObjectAndPush(Ptb.Get(brushDefChunk));
		TPoint move_spot; // Пятно в активном отрезке, при нажатии на которое его можно двигать
		move_spot = 0;    // Отрисовывается после общего цикла рисования основной области, дабы текст не затирал пятно.
		int    is_move_rect = 0;
		TRect  move_rect; // Область перемещаемого отрезка (рисуется в самом конце процедуры для того, чтобы быть на переднем плане)
		for(i = St.ScrollY; upp_edge <= (uint)a2.Right.b.y && i < P_Data->getCount(); i++) {
			const  STimeChunkAssocArray * p_row = P_Data->at(i);
			const  STimeChunkArray * p_holidays = P_Data->GetHolidayList(p_row->Id);
			rect.setheightrel(upp_edge + P.PixRowMargin, P.PixRow);
			const  RowState & r_rowst = GetRowState(p_row->Id);
			const  uint row_full_height = (P.PixRow * NZOR(r_rowst.Order, 1)) + 2 * P.PixRowMargin;
			//
			// Left area
			//
			if(a2.Left.b.x) {
				canv.SetTextColor(Ptb.GetColor(colorBlackText));
				rect.setwidth(a2.Left);
				if(P_Data->GetText(STimeChunkGrid::iRow, p_row->Id, temp_buf) > 0) {
					rect.setheightrel(upp_edge + P.PixRowMargin, row_full_height - P.PixRowMargin);
					canv.DrawText(rect, temp_buf.Transf(CTRANSF_INNER_TO_OUTER), DT_LEFT|DT_VCENTER|DT_END_ELLIPSIS);
				}
				if(St.Rsz.Kind == ResizeState::kRowHeight && p_row->Id == St.Rsz.RowId) {
					move_rect.setheightrel(upp_edge, row_full_height + St.Rsz.Shift);
					move_rect.setwidthrel(a2.Left.b.x - 10, 5);
					is_move_rect = 2; // see below
				}
			}
			//
			// Right area
			//
			if(p_holidays) {
				canv.SelectObjectAndPush(Ptb.Get(penDefChunk));
				canv.SelectObjectAndPush(Ptb.Get((i % 2) ? brushHoliday : brushHolidayInterleave));
				rect.setheightrel(upp_edge, row_full_height);
				for(k = 0; k < p_holidays->getCount(); k++) {
					const  STimeChunk * p_chunk = (const STimeChunk *)p_holidays->at(k);
					STimeChunk sect;
					if(p_chunk->Intersect(view_time_bounds, &sect) > 0) {
						ChunkToRectX(left_edge, sect, view_time_bounds.Start, rect);
						canv.Rectangle(rect);
					}
				}
				canv.PopObjectN(2);
			}
			if(St.Rsz.Kind == ResizeState::kMoveChunk && St.Rsz.RowId == p_row->Id) {
				//
				// Отрисовываем перемещаемый отрезок
				//
				STimeChunk sect;
				STimeChunkAssoc mc;
				P_Data->GetChunk(St.Rsz.ChunkId, 0, &mc);
				long   sec = PixToSec(St.Rsz.Shift);
				mc.Chunk.Start.addsec(sec);
				mc.Chunk.Finish.addsec(sec);
				if(mc.Chunk.Intersect(view_time_bounds, &sect) > 0) {
					move_rect.setheightrel(upp_edge + P.PixRowMargin, P.PixRow);
					ChunkToRectX(left_edge, sect, view_time_bounds.Start, move_rect);
					St.Rsz.Prev = move_rect;
					is_move_rect = 1;
				}
			}
			upp_edge += row_full_height;
		}
		{
			canv.SetTextColor(Ptb.GetColor(colorBlackText));
			canv.SelectObjectAndPush(Ptb.Get(fontChunkText)); // {
			for(i = 0; i < RL.getCount(); i++) {
				const  SRect & r_sr = RL.at(i);
				int    bs;
				HBRUSH brush_chunk;
				//
				// Установить цвет заливки отрезка
				bs = SelectChunkColor(&r_sr.C, &brush_chunk);
				if(bs)
					canv.SelectObjectAndPush(brush_chunk);
				//
				if(r_sr.C.Id == St.SelChunkId) {
					canv.SelectObjectAndPush(Ptb.Get(penSelectedChunk));
					canv.Rectangle(r_sr);
					canv.PopObject();
					move_spot.Set(r_sr.a.x, r_sr.a.y+2);
				}
				else {
					canv.SelectObjectAndPush(Ptb.Get(penChunk));
					canv.Rectangle(r_sr);
					canv.PopObject();
				}
				//
				// Восстановление после установки цвета заливки отрезка
				if(bs)
					canv.PopObject();
				//
				if(GetChunkText(r_sr.C.Id, temp_buf) > 0) {
					TRect tr = r_sr;
					tr.setmarginx(2);
					tr.setmarginy(1);
					canv.DrawText(tr, temp_buf, DT_LEFT | DT_END_ELLIPSIS);
				}
				if(St.Rsz.ChunkId == r_sr.C.Id) {
					if(oneof2(St.Rsz.Kind, ResizeState::kChunkRight, ResizeState::kChunkLeft)) {
						move_rect = r_sr;
						if(St.Rsz.Kind == ResizeState::kChunkRight)
							move_rect.b.x += (int16)St.Rsz.Shift;
						else
							move_rect.a.x += (int16)St.Rsz.Shift;
						is_move_rect = 1;
					}
					else if(St.Rsz.Kind == ResizeState::kMoveChunk) {
						canv.SelectObjectAndPush(Ptb.Get(penSelectedChunk));
						canv.SelectObjectAndPush(Ptb.Get(brushMovedChunk));
						canv.Rectangle(r_sr);
						canv.PopObjectN(2);
					}
				}
			}
			canv.PopObject(); // } fontChunkText
		}
		if(is_move_rect) {
			canv.SelectObjectAndPush(Ptb.Get((is_move_rect == 2) ? brushRescaleQuant : brushNull));
			if(is_move_rect != 2)
				canv.SelectObjectAndPush(Ptb.Get(penResizedChunk));
			canv.Rectangle(move_rect);
			if(is_move_rect != 2)
				canv.PopObject();
			canv.PopObject();
		}
		DrawMoveSpot(canv, move_spot);
		//
		// Vertical separator
		//
		if(a2.Separator.b.x) {
			canv.FillRect(a2.Separator, (HBRUSH)Ptb.Get(brushHeader));
			//canv.Rectangle(a2.Separator);
		}
		if(St.Rsz.Kind == ResizeState::kSplit) {
			rect.setwidthrel(St.Rsz.Org.x + St.Rsz.Shift - 1, 3);
			rect.setheight(a2.Full);
			St.Rsz.Prev.setwidthrel(St.Rsz.Org.x + St.Rsz.Shift - 3, 6);
			St.Rsz.Prev.setheight(rect);
			canv.SelectObjectAndPush(Ptb.Get(brushNull));
			canv.SelectObjectAndPush(Ptb.Get(penResizedChunk));
			canv.Rectangle(rect);
			canv.PopObjectN(2);
		}
		canv.PopObjectN(4);
	}
	if(!a2.PicMode.IsEmpty()) {
		uint   bmp_id = 0;
		if(P.ViewType == P.vHourDay) {
			if(P.SingleRowIdx >= 0)
				bmp_id = BmpId_Back;
			else
				bmp_id = BmpId_ModeGantt;
		}
		else
			bmp_id = BmpId_ModeHourDay;
		if(bmp_id) {
			HBITMAP    h_bmp = APPL->FetchBitmap(bmp_id);
			if(h_bmp) {
				POINT bmp_size;
				RECT r_ = (RECT)a2.PicMode;
				TProgram::DrawTransparentBitmap(h_dc, h_bmp, r_, 0, 0, 0, -1, 0, &bmp_size);
			}
		}
	}
	if(use_buffer) {
		BitBlt(ps.hdc, 0, 0, cr.right - cr.left, cr.bottom - cr.top, h_dc_mem, 0, 0, SRCCOPY);
		SelectObject(h_dc_mem, h_old_bmp);
		ZDeleteWinGdiObject(&h_bmp);
		DeleteDC(h_dc_mem);
	}
	::EndPaint(H(), &ps);
}
//
//
//
#if SLTEST_RUNNING // {

/*
SLTEST_R()
{
	return 1;
}*/

#endif // } SLTEST_RUNNING
