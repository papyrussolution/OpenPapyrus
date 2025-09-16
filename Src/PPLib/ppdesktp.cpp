// PPDESKTP.CPP
// Copyright (c) A.Starodub 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022, 2023, 2024, 2025
// @codepage UTF-8
//
#include <pp.h>
#pragma hdrstop

#define max MAX // @v9.8.11
#define min MIN // @v9.8.11
#include <gdiplus.h>
using namespace Gdiplus;

static const bool PPDesktop_Use_LayoutFlex = false;

const char * PPDesktop::WndClsName = SlConst::WinClsName_Desktop;
//
//
//
PPDesktopAssocCmd::PPDesktopAssocCmd() : CmdID(0), Flags(0)
{
}

PPDesktopAssocCmd & PPDesktopAssocCmd::Z()
{
	CmdID = 0;
	Flags = 0;
	Code.Z();
	DvcSerial.Z();
	CmdParam.Z();
	return *this;
}

int PPDesktopAssocCmd::ParseCode(CodeBlock & rBlk) const
{
	int    ok = 1;
	int    done = 0;
	rBlk.Type = 0;
	rBlk.Flags = 0;
	rBlk.AddedIntVal = 0;
	rBlk.LenRange.Z();
	rBlk.AddedStrVal.Z();
	rBlk.Key.Z();
	SString temp_buf;
	SStrScan scan(Code);
	do {
		scan.Skip();
		if(scan.Get("@scard", temp_buf) || scan.Get("@card", temp_buf)) {
			SETIFZ(rBlk.Type, cbSCardCode);
		}
		else if(scan.Get("@reg", temp_buf)) { // @reg(regsymb)
			if(!rBlk.Type) {
				rBlk.Type = cbPersonRegister;
				scan.Skip();
				if(scan[0] == '(') {
					scan.Incr();
					if(scan.SearchChar(')')) {
						scan.Get(temp_buf);
						scan.IncrLen();
						scan.Incr(); // Пропускаем завершающий ')'
						rBlk.AddedStrVal = temp_buf.Strip();
					}
				}
			}
		}
		else if(scan.Get("@inn", temp_buf)) {
			if(!rBlk.Type) {
				rBlk.Type = cbPersonRegister;
				rBlk.AddedIntVal = PPREGT_TPID;
			}
		}
		else if(scan.Get("@barcode", temp_buf)) {
			SETIFZ(rBlk.Type, cbGoodsBarcode);
		}
		else if(scan[0] == '[') {
			scan.Incr();
			if(scan.SearchChar(']')) {
				scan.Get(temp_buf);
				scan.IncrLen();
				scan.Incr(); // Пропускаем завершающий ']'
			}
			else {
				temp_buf = scan;
				done = 1;
			}
			if(rBlk.LenRange.IsZero()) {
				double low = 0.0, upp = 0.0;
				strtorrng(temp_buf, &low, &upp);
				SETMAX(low, 0.0);
				SETMAX(upp, 0.0);
				SExchangeForOrder(&low, &upp);
				rBlk.LenRange.low = static_cast<int>(low);
				rBlk.LenRange.upp = static_cast<int>(low);
			}
		}
		else if(isdec(scan[0])) { // @v10.9.0 @fix (scan[0] >= 0)-->(scan[0] >= '0')
			temp_buf.Z();
			while(isalnum(scan[0])) {
				temp_buf.CatChar(scan[0]);
				scan.Incr();
			}
			if(!rBlk.Type) {
				rBlk.Type = cbString;
				rBlk.AddedStrVal = temp_buf;
			}
		}
		else if(scan[0] == '\\' && scan[1] == 'n') {
			rBlk.Flags |= cbfCR;
			scan.Incr(2);
		}
		else {
			uint klen = 0;
			if(rBlk.Key.SetKeyName(scan, &klen) > 0) {
				SETIFZ(rBlk.Type, cbKey);
				scan.Incr(klen);
			}
			else
				scan.Incr();
		}
	} while(scan[0] != 0 && !done);
	return ok;
}

PPDesktopAssocCmdPool::PPDesktopAssocCmdPool() : DesktopID_Obsolete(-1)
{
	P.add("$"); // zero index - is empty string
}

PPDesktopAssocCmdPool::~PPDesktopAssocCmdPool()
{
}

void PPDesktopAssocCmdPool::Init(/*PPID desktopId*/const S_GUID & rUuid)
{
	DesktopID_Obsolete = 0;
	DesktopUuid = rUuid;
	L.clear();
	P.Z();
	P.add("$"); // zero index - is empty string
}

S_GUID PPDesktopAssocCmdPool::GetDesktopUuid() const { return DesktopUuid; }
void   PPDesktopAssocCmdPool::SetDesktopUuid(const S_GUID & rUuid) { DesktopUuid = rUuid; }
uint PPDesktopAssocCmdPool::GetCount() const { return L.getCount(); }
int  PPDesktopAssocCmdPool::AddItem(const PPDesktopAssocCmd * pCmd) { return SetItem(L.getCount(), pCmd); }

int PPDesktopAssocCmdPool::Pack()
{
	int    ok = 1;
	StringSet np;
	np.add("$");
	SString temp_buf;
	for(uint i = 0; i < L.getCount(); i++) {
		Item & r_item = L.at(i);
		P.getnz(r_item.CodeP, temp_buf);
		if(temp_buf.NotEmptyS()) {
			np.add(temp_buf, reinterpret_cast<uint *>(&r_item.CodeP));
		}
		else
			r_item.CodeP = 0;
		P.getnz(r_item.DvcSerialP, temp_buf);
		if(temp_buf.NotEmptyS()) {
			np.add(temp_buf, reinterpret_cast<uint *>(&r_item.DvcSerialP));
		}
		else
			r_item.DvcSerialP = 0;
		P.getnz(r_item.CmdParamP, temp_buf);
		if(temp_buf.NotEmptyS()) {
			np.add(temp_buf, reinterpret_cast<uint *>(&r_item.CmdParamP));
		}
		else
			r_item.CmdParamP = 0;
	}
	P = np;
	return ok;
}

int PPDesktopAssocCmdPool::MakeItem(const PPDesktopAssocCmd & rOuter, Item & rInner)
{
	int    ok = 1;
	SString temp_buf;
	MEMSZERO(rInner);
	rInner.CmdID = rOuter.CmdID;
	rInner.Flags = rOuter.Flags;
	temp_buf = rOuter.Code;
	if(temp_buf.NotEmptyS())
		P.add(temp_buf, reinterpret_cast<uint *>(&rInner.CodeP));
	temp_buf = rOuter.DvcSerial;
	if(temp_buf.NotEmptyS())
		P.add(temp_buf, reinterpret_cast<uint *>(&rInner.DvcSerialP));
	temp_buf = rOuter.CmdParam;
	if(temp_buf.NotEmptyS())
		P.add(temp_buf, reinterpret_cast<uint *>(&rInner.CmdParamP));
	return ok;
}

int PPDesktopAssocCmdPool::SetItem(uint pos, const PPDesktopAssocCmd * pCmd)
{
	int    ok = 1;
	if(pos < L.getCount()) {
		if(pCmd) {
			Item item;
			MakeItem(*pCmd, item);
			L.at(pos) = item;
		}
		else
			L.atFree(pos);
	}
	else if(pos == L.getCount()) {
		assert(pCmd);
		if(pCmd) {
			Item item;
			MakeItem(*pCmd, item);
			L.insert(&item);
		}
		else
			ok = 0;
	}
	return ok;
}

int PPDesktopAssocCmdPool::GetItem(uint pos, PPDesktopAssocCmd & rCmd) const
{
	int    ok = 1;
	if(pos < L.getCount()) {
		const Item & r_item = L.at(pos);
		rCmd.CmdID = r_item.CmdID;
		rCmd.Flags = r_item.Flags;
		P.getnz(r_item.CodeP, rCmd.Code);
		P.getnz(r_item.DvcSerialP, rCmd.DvcSerial);
		P.getnz(r_item.CmdParamP, rCmd.CmdParam);
	}
	else
		ok = 0;
	return ok;
}

int PPDesktopAssocCmdPool::GetByCode(const char * pCode, uint * pPos, PPDesktopAssocCmd * pCmd, SString * pResult) const
{
	int    ok = 0;
	uint   pos = 0;
	SString result_text;
	if(!isempty(pCode)) {
		SString temp_buf;
		const size_t code_len = sstrlen(pCode);
		for(uint i = 0; !ok && i < L.getCount(); i++) {
			const Item & r_item = L.at(i);
			P.getnz(r_item.CodeP, temp_buf);
			if(r_item.Flags & PPDesktopAssocCmd::fSpecCodePrefx) {
				if(temp_buf.CmpL(pCode, 1) == 0) {
					if(code_len > temp_buf.Len())
						(result_text = pCode).ShiftLeft(temp_buf.Len());
					pos = i;
					ok = 1;
				}
			}
			else if(temp_buf.CmpNC(pCode) == 0) {
				pos = i;
				ok = 1;
			}
		}
		if(ok) {
			if(pCmd)
				GetItem(pos, *pCmd);
		}
	}
	ASSIGN_PTR(pPos, pos);
	ASSIGN_PTR(pResult, result_text);
	return ok;
}

struct DesktopAssocCmdPool_Strg {
	int32  ObjType;
	int32  ObjID;
	int32  Prop;
	uint8  Reserve[60];
	uint32 Size;         // Полный размер записи (заголовок + хвост переменной длины)
	int32  Reseve2;
	SVerT  Ver;
};

// @v10.9.3 (replaced with PPConst::CommonCmdAssocDesktopID) #define COMMON_DESKCMDASSOC 100000L

int PPDesktopAssocCmdPool::WriteToProp(int use_ta)
{
	int    ok = 1;
	Reference * p_ref = PPRef;
	DesktopAssocCmdPool_Strg * p_strg = 0;
	SSerializeContext sctx;
	PPID   desktop_id = DesktopID_Obsolete;
	S_GUID desktop_uuid = DesktopUuid;
	if(!!desktop_uuid) {
		UuidRefCore urt;
		long   temp_id = 0;
		if(urt.SearchUuid(desktop_uuid, 0, &temp_id) > 0) {
			desktop_id = temp_id;
		}
	}
	THROW_PP(desktop_id >= 0, PPERR_UNDEFDESKTID_WRITE);
	SETIFZ(desktop_id, /*COMMON_DESKCMDASSOC*/PPConst::CommonCmdAssocDesktopID);
	THROW(Pack());
	{
		size_t sz = sizeof(*p_strg);
		if(L.getCount()) {
			SBuffer tail_buf;
			THROW_SL(sctx.Serialize(+1, &L, tail_buf));
			THROW_SL(P.Serialize(+1, tail_buf, &sctx));
			const  size_t tail_size = tail_buf.GetAvailableSize();
			sz += tail_size;
			THROW_MEM(p_strg = static_cast<DesktopAssocCmdPool_Strg *>(SAlloc::M(sz)));
			memzero(p_strg, sz);
			p_strg->ObjType = PPOBJ_DESKTOP;
			p_strg->ObjID = desktop_id;
			p_strg->Prop  = PPPRP_DESKCMDASSOC;
			p_strg->Size  = sz;
			p_strg->Ver   = DS.GetVersion();
			THROW_SL(tail_buf.Read((p_strg+1), tail_size));
		}
		{
			PPTransaction tra(use_ta);
			THROW(tra);
			THROW(p_ref->PutProp(PPOBJ_DESKTOP, desktop_id, PPPRP_DESKCMDASSOC, p_strg, sz, 0));
			THROW(p_ref->PutProp(PPOBJ_CONFIG,  desktop_id, PPPRP_DESKCMDASSOC, 0, sz, 0)); // Удаляем старую версию свойств
			THROW(tra.Commit());
		}
	}
	CATCHZOK
	SAlloc::F(p_strg);
	return ok;
}

int PPDesktopAssocCmdPool::ReadFromProp(const S_GUID & rDesktopUuid)
{
	int    ok = -1;
	Reference * p_ref = PPRef;
	size_t sz = 0;
	DesktopAssocCmdPool_Strg * p_strg = 0;
	PPID   desktop_id = 0;//(desktopId >= 0) ? desktopId : DesktopID;
	S_GUID desktop_uuid;
	THROW_PP(desktop_id >= 0, PPERR_UNDEFDESKTID_READ);
	if(!!rDesktopUuid) {
		UuidRefCore urt;
		THROW(urt.GetUuid(rDesktopUuid, &desktop_id, 0, -1));
		desktop_uuid = rDesktopUuid;
		assert(desktop_id > 0);
	}
	else {
		desktop_id = PPConst::CommonCmdAssocDesktopID/*COMMON_DESKCMDASSOC*/;
	}
	if(p_ref->GetPropActualSize(PPOBJ_DESKTOP, desktop_id, PPPRP_DESKCMDASSOC, &sz) > 0) {
		p_strg = static_cast<DesktopAssocCmdPool_Strg *>(SAlloc::M(sz));
		THROW(p_ref->GetProperty(PPOBJ_DESKTOP, desktop_id, PPPRP_DESKCMDASSOC, p_strg, sz) > 0);
		{
			SSerializeContext sctx;
			SBuffer tail_buf;
			THROW_SL(tail_buf.Write((p_strg+1), sz-sizeof(*p_strg)));
			THROW_SL(sctx.Serialize(-1, &L, tail_buf));
			THROW_SL(P.Serialize(-1, tail_buf, &sctx));
			DesktopID_Obsolete = desktop_id;
			DesktopUuid = desktop_uuid; // @v10.9.3
			ok = 1;
		}
	}
	// @v10.9.3 {
	else {
		DesktopID_Obsolete = desktop_id;
		DesktopUuid = desktop_uuid;
	}
	// } @v10.9.3 
	//DesktopID = desktopId;
	CATCHZOK
	SAlloc::F(p_strg);
	return ok;
}
//
//
//
static LRESULT CALLBACK EditDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) // @callback(DLGPROC)
{
	WNDPROC prev_window_proc = static_cast<WNDPROC>(TView::GetWindowUserData(hWnd));
	switch(uMsg) {
		case WM_LBUTTONDBLCLK:
		case WM_RBUTTONDOWN: ::SendMessage(GetParent(hWnd), uMsg, wParam, lParam); break;
		case WM_VSCROLL: ::InvalidateRect(hWnd, NULL, true); break;
	}
	return CallWindowProc(prev_window_proc, hWnd, uMsg, wParam, lParam);
}

PPBizScoreWindow::PPBizScoreWindow(HWND hParentWnd) : TWindow(TRect(1,1,50,20)), HParentWnd(hParentWnd), ActualDt(ZERODATE), Dtm(ZERODATETIME)
{
}

PPBizScoreWindow::~PPBizScoreWindow()
{
	Destroy();
}

int PPBizScoreWindow::LoadData()
{
	const  PPID cur_user_id = LConfig.UserID;
	const  LDATE _curdate = getcurdate_();
	uint   _scount = 0; // @debug
	BizScoreTbl::Key0 k0;
	BExtQuery q(&Tbl, 0, 8);
	PPObjBizScore obj_bsc;
	SString buf, name, value;
	MEMSZERO(k0);
	k0.ActualDate = _curdate;
	k0.ScoreID = MAXLONG;
	k0.ObjID   = MAXLONG;
	BizScoreList.Z();
	q.select(Tbl.ScoreID, Tbl.Val, Tbl.Dt, Tbl.Tm, Tbl.ActualDate, Tbl.Str, 0L).where(Tbl.UserID == cur_user_id/* && Tbl.ActualDate >= _curdate*/);
	for(q.initIteration(true, &k0, spLe); q.nextIteration() > 0;) {
		_scount++; // @debug
		BizScoreTbl::Rec rec;
		Tbl.copyBufTo(&rec);
		if((!ActualDt || ActualDt <= rec.ActualDate)) {
			PPBizScorePacket pack;
			if(obj_bsc.Fetch(rec.ScoreID, &pack) > 0) {
				name = pack.Descr.Len() ? pack.Descr : pack.Rec.Name;
				value = rec.Str;
				if(!value.NotEmptyS())
					value.Cat(rec.Val);
				(buf = name).Semicol().Cat(value);
				BizScoreList.Add(rec.ScoreID, buf, 1);
				Dtm.Set(rec.Dt, rec.Tm);
				ActualDt = rec.ActualDate;
			}
		}
	}
	Update();
	return 1;
}

int PPBizScoreWindow::Create()
{
	int    ok = 0;
	Destroy();
	HW = APPL->CreateDlg(DLG_BUSPARAMS, HParentWnd, PPBizScoreWindow::Proc, reinterpret_cast<LPARAM>(this));
	if(H()) {
		Brush = CreateSolidBrush(RGB(0xFF, 0xF7, 0x94));
		Move_W();
		LoadData();
		ShowWindow(H(), SW_SHOWNORMAL);
		UpdateWindow(H());
		ok = 1;
	}
	return ok;
}

void PPBizScoreWindow::Update()
{
	uint   pos = 0;
	SString buf, text, name, value;
	text.Cat(Dtm).CRB();
	PPGetWord(PPWORD_CALCDATE, 0, buf);
	text.Cat(buf).CatDiv(':', 1).Cat(ActualDt).CRB();
	for(uint i = 0; i < BizScoreList.getCount(); i++) {
		(buf = BizScoreList.Get(i).Txt).Divide(';', name, value);
		(buf = name).CatDiv(':', 1).Cat(value).CRB();
		text.Cat(buf);
	}
	TView::SSetWindowText(GetDlgItem(H(), CTL_BUSPARAMS_TEXT), text.Transf(CTRANSF_INNER_TO_OUTER).Strip());
	UpdateWindow(H());
}

void PPBizScoreWindow::Destroy()
{
	TView::SetWindowUserData(H(), 0);
	HW = 0;
	ZDeleteWinGdiObject(&Brush);
}

void PPBizScoreWindow::Move_W()
{
	HWND hw = H();
	if(hw) {
		RECT   rect, parent_rect, text_rect;
		GetClientRect(HParentWnd, &parent_rect);
		int    h = (parent_rect.bottom - parent_rect.top)  / 3;
		int    w = (parent_rect.right  - parent_rect.left) / 3;
		rect.top  = parent_rect.bottom - h;
		rect.left = parent_rect.right  - w;
		SetWindowPos(hw, 0, rect.left, rect.top, w, h, SWP_NOZORDER);
		GetClientRect(hw, &text_rect);
		GetWindowRect(hw, &rect);
		SetWindowPos(GetDlgItem(hw, CTL_TOOLTIP_TEXT), 0, text_rect.left + 5, text_rect.top + 5,
			text_rect.left + text_rect.right - 10, text_rect.top + text_rect.bottom - 10, SWP_NOZORDER);
		::InvalidateRect(hw, &rect, true);
		::UpdateWindow(hw);
	}
}

void PPBizScoreWindow::MoveOnLayout(const FRect & rRect)
{
	HWND hw = H();
	if(hw) {
		RECT   rect, text_rect;
		SetWindowPos(hw, 0, static_cast<int>(rRect.a.x), static_cast<int>(rRect.a.y), static_cast<int>(rRect.Width()), 
			static_cast<int>(rRect.Height()), SWP_NOZORDER);
		GetClientRect(hw, &text_rect);
		GetWindowRect(hw, &rect);
		SetWindowPos(GetDlgItem(hw, CTL_TOOLTIP_TEXT), 0, text_rect.left + 5, text_rect.top + 5,
			text_rect.left + text_rect.right - 10, text_rect.top + text_rect.bottom - 10, SWP_NOZORDER);
		::InvalidateRect(hw, &rect, true);
		::UpdateWindow(hw);
	}
}

int PPBizScoreWindow::DoCommand(SPoint2S p)
{
	int    ok = -1;
	/*
	if(Cmd) {
		static_cast<PPApp *>(APPL)->processCommand(Cmd);
		ok = 1;
	}
	*/
	return ok;
}

/*static*/INT_PTR CALLBACK PPBizScoreWindow::Proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PPBizScoreWindow * p_win = static_cast<PPBizScoreWindow *>(TView::GetWindowUserData(hWnd));
	switch(message) {
		case WM_INITDIALOG:
			{
				HWND ctl_hwnd = GetDlgItem(hWnd, CTL_BUSPARAMS_TEXT);
				WNDPROC prev_window_proc = static_cast<WNDPROC>(TView::SetWindowProp(ctl_hwnd, GWLP_WNDPROC, EditDlgProc));
				TView::SetWindowUserData(ctl_hwnd,  prev_window_proc);
				TView::SetWindowUserData(hWnd, reinterpret_cast<void *>(lParam));
				TView::SetWindowProp(hWnd, DWLP_USER, DLG_TOOLTIP);
			}
			break;
		case WM_DESTROY:
			TView::SetWindowUserData(hWnd, 0);
			ZDELETE(p_win);
			break;
		case WM_LBUTTONUP:
			::SendMessage(GetParent(hWnd), message, lParam, wParam);
			break;
		case WM_LBUTTONDBLCLK:
			if(p_win) {
				SPoint2S p;
				p_win->DoCommand(p.setwparam(lParam));
				::DestroyWindow(hWnd);
			}
			break;
		case WM_RBUTTONDOWN:
			if(p_win) {
				SString menu_text;
				TMenuPopup menu;
				PPLoadStringS("close", menu_text).Transf(CTRANSF_INNER_TO_OUTER);
				menu.Add(menu_text, cmaDelete);
				uint   cmd = 0;
				if(menu.Execute(hWnd, TMenuPopup::efRet, &cmd, 0) && cmd == cmaDelete)
					::DestroyWindow(hWnd);
			}
			break;
		case WM_CTLCOLORSTATIC:
		case WM_CTLCOLORDLG:
			if(p_win) {
				HDC hdc = reinterpret_cast<HDC>(wParam);
				SetBkMode(hdc, TRANSPARENT);
				return (BOOL)(p_win->Brush); // don't modify type cast - Windows needs real brush result
			}
			else
				return FALSE;
		/* @v11.0.0 
		case WM_SIZE:
		case WM_USER_MAINWND_MOVE_SIZE:
			CALLPTRMEMB(p_win, Move_W());
			break;
		*/
	}
	return FALSE;
}
//
// PPDesktop
//
PPDesktop::PPDesktop() : TWindow(TRect(1,1,50,20)), IconSize(0), IconGap(0), HwndTT(0), P_ActiveDesktop(0), State(0), HBizScoreWnd(0),
	P_ScObj(0), P_GObj(0), P_PsnObj(0)
{
	// @v11.9.4 {
	const UiDescription * p_uid = SLS.GetUiDescription();
	if(p_uid) {
		int v = 0;
		if(p_uid->VList.Get(UiValueList::vDesktopIconSize, v) > 0 && v > 0)
			IconSize = v;
		if(p_uid->VList.Get(UiValueList::vDesktopIconGap, v) > 0 && v > 0)
			IconGap = v;
	}
	// } @v11.9.4
	SETIFZQ(IconSize, 32);
	SETIFZQ(IconGap, 8);
}

PPDesktop::~PPDesktop()
{
	ZDELETE(P_ScObj);
	ZDELETE(P_GObj);
	ZDELETE(P_PsnObj);
	Destroy(0);
}

int PPDesktop::Init__(const S_GUID & rDesktopUuid)
{
	Destroy(1);
	int    ok = 1;
	DbProvider * p_dict = CurDict;
	const  PPCommandItem * p_item = 0;
	PPCommandMngr * p_mgr = 0;
	SLS.InitGdiplus();
	if(p_dict) {
		SString db_symb;
		PPCommandGroup desktop_list_from_bin, desktop_list;
		p_dict->GetDbSymb(db_symb);
		{
			//@erik v10.6.6 {
			THROW(p_mgr = GetCommandMngr(PPCommandMngr::ctrfReadOnly, cmdgrpcDesktop, 0));
			p_mgr->Load__2(&desktop_list, db_symb, PPCommandMngr::fRWByXml);
			p_item = desktop_list.SearchByUuid(rDesktopUuid, 0);
			if(!p_item) {
				THROW(p_mgr->Load_Deprecated(&desktop_list_from_bin));
				p_item = desktop_list_from_bin.SearchByUuid(rDesktopUuid, 0);
			}				
			// } @erik
			ZDELETE(p_mgr);
		}
		P_ActiveDesktop = (p_item && p_item->IsKind(PPCommandItem::kGroup)) ? static_cast<PPCommandGroup *>(p_item->Dup()) : 0;
		THROW_PP(P_ActiveDesktop && P_ActiveDesktop->IsDbSymbEq(db_symb), PPERR_DESKNOTFOUND);
		PrivateCp.ReadFromProp(rDesktopUuid);
		{
			S_GUID zero_uuid;
			CommonCp.ReadFromProp(zero_uuid.Z());
		}
		ViewOptions |= ofSelectable;
		Selected = 0;
		State &= ~stIconMove;
		MEMSZERO(MoveIconCoord);
		Selected = ((p_item = P_ActiveDesktop->SearchFirst(0)) != 0 && p_item->IsKind(PPCommandItem::kCommand)) ? p_item->GetID() : 0;
		//
		// Если установлены обои, то создадим временный файл для них, и будем использовать его
		//
		if(P_ActiveDesktop->GetLogo().NotEmpty()) {
			SString path, buf;
			Logotype.Init();
			SFsPath ps(P_ActiveDesktop->GetLogo());
			PPGetPath(PPPATH_BIN, path);
			PPLoadText(PPTXT_DESKIMGDIR, buf);
			path.SetLastSlash().Cat(buf);
			MakeTempFileName(path, "TMP", ps.Ext, 0, buf.Z());
			path = buf;
			copyFileByName(P_ActiveDesktop->GetLogo(), path);
			Logotype.Load(path);
		}
		else
			Logotype.Load(0);
		Advise();
		{
			//
			// Инициализируем список USB-устройств для того, чтобы идентифицировать ввод
			// с серийным номером
			//
			UsbList.freeAll();
			SUsbDevice::GetDeviceList(UsbList);
 		}
	}
	else
		ok = -1;
	CATCH
		ZDELETE(P_ActiveDesktop);
		if(PPErrCode == PPERR_DESKNOTFOUND) {
			DS.GetTLA().Lc.DesktopID_Obsolete = 0;
			DS.GetTLA().Lc.DesktopUuid_.Z(); // @v10.9.3
		}
		ok = 0;
	ENDCATCH
	delete p_mgr;
	return ok;
}

int PPDesktop::Destroy(int dontAssignToDb)
{
	int    ok = 1;
	if(H()) {
		HDC    hdc = GetDC(H());
		ReleaseDC(H(), hdc);
	}
	if(HwndTT) {
		DestroyWindow(HwndTT);
		if(Logotype.IsValid()) {
			SString logo_fname;
			Logotype.GetFileName(logo_fname);
			Logotype.Load(0);
			SFile::Remove(logo_fname);
		}
	}
	if(!dontAssignToDb) {
		const PPConfig & r_cfg = LConfig;
		PPObjSecur sec_obj(PPOBJ_USR, 0);
		SString desk_name;
		if(!!r_cfg.DesktopUuid_)
			PPDesktop::GetDeskName(r_cfg.DesktopUuid_, desk_name);
		if(desk_name.Len()) {
			THROW(sec_obj.AssignPrivateDesktop(r_cfg.UserID, r_cfg.DesktopUuid_, desk_name, 1));
			THROW(SaveDesktop(0, 0));
		}
	}
	CATCH
		ok = (PPErrorTooltip(-1, 0), 0);
	ENDCATCH
	ZDELETE(P_ActiveDesktop);
	State &= ~stChanged;
	Unadvise();
	return ok;
}

void PPDesktop::AddTooltip(long id, SPoint2S coord, const char * pText)
{
	TCHAR  tooltip[512];
	memzero(tooltip, sizeof(tooltip));
	STRNSCPY(tooltip, SUcSwitch(pText));
	TOOLINFOA t_i;
	INITWINAPISTRUCT(t_i);
	t_i.uFlags = TTF_SUBCLASS;
	t_i.hwnd   = H();
	t_i.uId    = id;
	TRect ir;
	CalcIconRect(coord, ir);
	t_i.rect = ir;
	t_i.hinst    = TProgram::GetInst();
	t_i.lpszText = const_cast<char *>(pText); //tooltip
	::SendMessage(HwndTT, TTM_DELTOOLA, 0, reinterpret_cast<LPARAM>(&t_i));
	::SendMessage(HwndTT, TTM_ADDTOOLA, 0, reinterpret_cast<LPARAM>(&t_i));
}

int PPDesktop::DrawText(TCanvas & rC, SPoint2S coord, COLORREF color, const char * pText)
{
	long   text_h = 0;
	SString text(pText);
	RECT   text_rect;
	HFONT  old_font = 0;
	HFONT  curs_over_txt_font = 0;
	TEXTMETRIC tm;
	rC.SetTextColor(color);
	TRect cr = getClientRect();
	text_rect.top    = cr.a.y  + coord.y + IconSize + 2;
	text_rect.left   = cr.a.x + coord.x;
	text_rect.bottom = text_rect.top  + IconSize - 2;
	text_rect.right  = text_rect.left + IconSize * 2;
	{
		rC.SelectObjectAndPush(Ptb.Get(fontText));
		GetTextMetrics(rC, &tm);
		text_h = (IconSize - 2) / tm.tmHeight;
		SplitBuf(rC, text, IconSize * 2 - 4, text_h);
		StringSet ss('\n', text);
		for(uint i = 0; ss.get(&i, text); text_rect.top += tm.tmHeight, text_rect.bottom += tm.tmHeight)
			::DrawText(rC, SUcSwitch(text), text.Len(), &text_rect, DT_CENTER); // @unicodeproblem
		rC.PopObject();
	}
	ZDeleteWinGdiObject(&curs_over_txt_font);
	return 1;
}

void PPDesktop::DrawIcon(TCanvas & rC, long cmdID, int isSelected)
{
	const  PPCommandItem * p_item = P_ActiveDesktop->SearchByID(cmdID, 0);
	PPCommand * p_cmd = (p_item && p_item->IsKind(PPCommandItem::kCommand)) ? static_cast<PPCommand *>(p_item->Dup()) : 0;
    if(p_cmd)
		DrawIcon(rC, p_cmd->GetID(), p_cmd->P, p_cmd->Name, p_cmd->Icon, isSelected);
	ZDELETE(p_cmd);
}

void PPDesktop::DrawIcon(TCanvas & rC, long id, SPoint2S coord, const SString & rText, const SString & rIcon, int isSelected)
{
	RECT   cr;
	GetClientRect(H(), &cr);
	const long x = cr.left + coord.x;
	const long y = cr.top + coord.y;
	if(isSelected) {
		rC.SelectObjectAndPush(Ptb.Get(brushSelTextRect));
		rC.SelectObjectAndPush(Ptb.Get(penSelTextRect));
		TRect text_rect(x, y + IconSize + 2, x + IconSize * 2, y + 2 * IconSize);
	 	rC.Rectangle(text_rect);
	}
	else {
		rC.SelectObjectAndPush(Ptb.Get(brushTextRect));
		rC.SelectObjectAndPush(Ptb.Get(penTextRect));
	}
	{
		HICON  h_icon = 0;
		long   text_color = SClrWhite;
		if(!isSelected) {
			text_color = GetPixel(rC, (int)(x + IconSize * 1.5), (int)(y + IconSize * 1.5));
			text_color = (labs(text_color - SClrBlack) > labs(text_color - SClrWhite)) ? SClrBlack : SClrWhite;
		}
		{
			long icon_id = rIcon.ToLong();
			if(icon_id) {
				if(icon_id & SlConst::VectorImageMask) {
					TWhatmanToolArray::Item tool_item;
					const SDrawFigure * p_fig = APPL->LoadDrawFigureById(icon_id, &tool_item);
					SPaintToolBox * p_tb = APPL->GetUiToolBox();
					if(p_tb && p_fig) {
						const uint _w = IconSize;
						const uint _h = IconSize;
						SImageBuffer ib(_w, _h);
						{
							TCanvas2 canv_temp(*p_tb, ib);
							if(!tool_item.ReplacedColor.IsEmpty()) {
								SColor replacement_color = p_tb->GetColor(TProgram::tbiIconRegColor);
								canv_temp.SetColorReplacement(tool_item.ReplacedColor, replacement_color);
							}
							LMatrix2D mtx;
							SViewPort vp;
							FRect pic_bounds(static_cast<float>(_w), static_cast<float>(_h));
							//pic_bounds.a.SetZero();
							//pic_bounds.b.Set(static_cast<float>(_w), static_cast<float>(_h));
							//
							canv_temp.Rect(pic_bounds);
							//canv.Fill(SColor(255, 255, 255, 255), 0); // Прозрачный фон
							canv_temp.Fill(SColor(0xd4, 0xf0, 0xf0, 255), 0); // Прозрачный фон
							canv_temp.PushTransform();
							p_fig->GetViewPort(&vp);
							canv_temp.AddTransform(vp.GetMatrix(pic_bounds, mtx));
							canv_temp.Draw(p_fig);
						}
						h_icon = static_cast<HICON>(ib.TransformToIcon());
					}
				}
				else
					h_icon = LoadIcon(TProgram::GetInst(), MAKEINTRESOURCE(icon_id));
			}
			else {
				// @v11.2.12 @todo: иногда эта функция работает очень медленно
				h_icon = static_cast<HICON>(::LoadImage(0, SUcSwitch(rIcon), IMAGE_ICON, IconSize, IconSize, LR_LOADFROMFILE));
			}
		}
		SETIFZ(h_icon, LoadIcon(TProgram::GetInst(), MAKEINTRESOURCE(ICON_DEFAULT)));
		if(h_icon) {
			SString text;
			(text = rText).Transf(CTRANSF_INNER_TO_OUTER);
			::SetBkMode(rC, TRANSPARENT); // @v10.7.8
			::DrawIconEx(rC, cr.left + coord.x + IconSize / 2, cr.top + coord.y + 2, h_icon, 0, 0, 0, 0, DI_DEFAULTSIZE|DI_IMAGE|DI_MASK);
			PPDesktop::DrawText(rC, coord, static_cast<COLORREF>(text_color), text);
			AddTooltip(id, coord, text);
			DestroyIcon(h_icon);
		}
	}
	rC.PopObject();
	rC.PopObject();
}

/*static*/COLORREF PPDesktop::GetDefaultBgColor()
{
	return GetColorRef(SClrSteelblue4);
}

void PPDesktop::Paint()
{
	SLS.InitGdiplus();
	const  bool use_buffer = false;
	const  PPCommandItem * p_item = 0;
	PPCommand * p_cmd = 0;
	PAINTSTRUCT ps;
	HBITMAP h_bmp = 0;
	HBITMAP h_old_bmp = 0;
	HDC    h_dc_mem = 0;
	HDC    h_dc = ::BeginPaint(H(), &ps);
	TRect cli_rect = getClientRect();
	if(use_buffer) {
		h_dc_mem = CreateCompatibleDC(ps.hdc);
		h_bmp = CreateCompatibleBitmap(ps.hdc, cli_rect.width(), cli_rect.height());
		h_old_bmp = static_cast<HBITMAP>(::SelectObject(h_dc_mem, h_bmp));
		h_dc = h_dc_mem;
	}
	if(!use_buffer/*|| !ps.fErase*/) { // @v11.2.4
		TCanvas canv(h_dc);
		{
			SColor bkg_color = Ptb.GetColor(colorBkg);
			Graphics graphics(canv);
			if(P_ActiveDesktop->Flags & PPCommandItem::fBkgndGradient) {
				Gdiplus::Color color1, color2; // 0.65..0.3
				color1.SetFromCOLORREF(bkg_color.Lighten(0.8f));
				color2.SetFromCOLORREF(bkg_color);
				LinearGradientBrush gr_brush(Point(cli_rect.a.x, cli_rect.a.y), Point(cli_rect.b.x, cli_rect.b.y), color1, color2);
				Pen pen(&gr_brush);
				graphics.FillRectangle(&gr_brush, cli_rect.a.x, cli_rect.a.y, cli_rect.width(), cli_rect.height());
			}
			else {
				canv.FillRect(cli_rect, static_cast<HBRUSH>(Ptb.Get(brushBkg)));
			}
			if(Logotype.IsValid()) {
				int    w  = static_cast<int>(Logotype.GetWidth());
				int    h  = static_cast<int>(Logotype.GetHeight());
				for(int x = cli_rect.a.x - cli_rect.a.x % w; x < cli_rect.b.x; x+= w) {
					for(int y = cli_rect.a.y - cli_rect.a.y % h; y < cli_rect.b.y; y += h) {
						RECT image_rect;
						RECT dest_rect;
						image_rect.top    = y;
						image_rect.left   = x;
						image_rect.right  = image_rect.left + w;
						image_rect.bottom = image_rect.top + h;
						if(SIntersectRect(dest_rect, cli_rect, image_rect)) {
							RECT part;
							part.top    = dest_rect.top    - image_rect.top;
							part.left   = dest_rect.left   - image_rect.left;
							part.right  = dest_rect.right  - dest_rect.left;
							part.bottom = dest_rect.bottom - dest_rect.top;
							Logotype.DrawPartUnchanged(canv, dest_rect.left, dest_rect.top, &part);
						}
					}
				}
			}
		}
		{
			PPIDArray ico_list;
			SetBkMode(canv, TRANSPARENT);
			P_ActiveDesktop->GetIntersectIDs(cli_rect, *this, &ico_list);
			for(uint i = 0; i < ico_list.getCount(); i++) {
				long ico_id = ico_list.at(i);
				if(ico_id != labs(Selected))
					DrawIcon(canv, ico_id, 0);
			}
			if(Selected > 0) {
				if(State & stIconMove) {
					const PPCommandItem * p_item = P_ActiveDesktop->SearchByID(Selected, 0);
					const PPCommand * p_cmd = (p_item && p_item->IsKind(PPCommandItem::kCommand)) ? static_cast<const PPCommand *>(p_item) : 0;
					if(p_cmd)
						DrawIcon(canv, p_cmd->GetID(), MoveIconCoord, p_cmd->Name, p_cmd->Icon, 1);
				}
				else
					DrawIcon(canv, Selected, 1);
			}
		}
	}
	if(use_buffer) {
		BitBlt(ps.hdc, 0, 0, cli_rect.width(), cli_rect.height(), h_dc_mem, 0, 0, SRCCOPY);
		SelectObject(h_dc_mem, h_old_bmp);
		ZDeleteWinGdiObject(&h_bmp);
		DeleteDC(h_dc_mem);
	}
	::EndPaint(H(), &ps);
}

int PPDesktop::BeginIconMove(SPoint2S coord)
{
	int    ok = -1;
	const  PPCommandItem * p_item = P_ActiveDesktop->SearchByCoord(coord, *this, 0);
	PPCommand * p_cmd = (p_item && p_item->IsKind(PPCommandItem::kCommand)) ? static_cast<PPCommand *>(p_item->Dup()) : 0;
	if(Selected) {
		TRect ir;
		P_ActiveDesktop->GetIconRect(Selected, *this, &ir);
		Selected = 0;
		invalidateRect(ir, false);
	}
	if(p_cmd) {
		TRect ir;
		Selected = p_cmd->GetID();
		MoveIconCoord = p_cmd->P;
		CoordOffs = coord - MoveIconCoord;
		P_ActiveDesktop->GetIconRect(Selected, *this, &ir);
		invalidateRect(ir, false);
		State |= stIconMove;
		ok = 1;
	}
	else {
		Selected = 0;
		State &= ~stIconMove;
	}
	::UpdateWindow(H());
	ZDELETE(p_cmd);
	return ok;
}

TRect & PPDesktop::CalcIconRect(SPoint2S lrp, TRect & rResult) const
{
	rResult.setwidthrel(lrp.x,  IconSize * 2);
	rResult.setheightrel(lrp.y, IconSize * 2);
	return rResult;
}

int PPDesktop::MoveIcon(SPoint2S coord)
{
	int    ok = -1;
	if(State & stIconMove) {
		const PPCommandItem * p_item = static_cast<const PPCommand *>(P_ActiveDesktop->SearchByID(Selected, 0));
		if(p_item && p_item->IsKind(PPCommandItem::kCommand)) {
			TRect ir;
			SRegion inv_reg;
			CalcIconRect(MoveIconCoord, ir);
			//ir.grow(+12, +12); // @v11.2.4
			inv_reg.Add(ir, SCOMBINE_OR);
			Selected = -Selected;
			Selected = labs(Selected);
			MoveIconCoord = coord - CoordOffs;
			CalcIconRect(MoveIconCoord, ir);
			//ir.grow(+12, +12); // @v11.2.4
			inv_reg.Add(ir, SCOMBINE_OR);
			invalidateRegion(inv_reg, true);
			::UpdateWindow(H());
			ok = 1;
		}
	}
	return ok;
}

void PPDesktop::EndIconMove(SPoint2S coord)
{
	if(State & stIconMove) {
		int    intersect = 0;
		uint   pos = 0;
		const PPCommandItem * p_item = P_ActiveDesktop->SearchByID(Selected, &pos);
		PPCommand * p_cmd = (p_item && p_item->IsKind(PPCommandItem::kCommand)) ? static_cast<PPCommand *>(p_item->Dup()) : 0;
		if(p_cmd) {
			PPIDArray ary;
			TRect ir;
			MoveIconCoord = coord - CoordOffs;
			CalcIconRect(MoveIconCoord, ir);
			Selected =- Selected;
			invalidateRect(ir, false);
			Selected = labs(Selected);
			if(P_ActiveDesktop->GetIntersectIDs(MoveIconCoord, *this, &ary) > 0)
				for(uint i = 0; !intersect && i < ary.getCount(); i++)
					if(ary.at(i) != Selected)
						intersect = 1;
			if(!intersect) {
				if(ArrangeIcon(&MoveIconCoord) > 0) {
					if(p_cmd->P != MoveIconCoord) {
						p_cmd->P = MoveIconCoord;
						State |= stChanged;
					}
				}
			}
			P_ActiveDesktop->Update(pos, static_cast<PPCommandItem *>(p_cmd));
			State &= ~stIconMove;
			CalcIconRect(p_cmd->P, ir);
			invalidateRect(ir, false);
			ZDELETE(p_cmd);
			::UpdateWindow(H());
		}
		else
			State &= ~stIconMove;
		MEMSZERO(MoveIconCoord);
		MEMSZERO(CoordOffs);
	}
}

struct DesktopLayoutHelperEntry {
	DesktopLayoutHelperEntry() : SvcView(-1), Ptr(0)
	{
	}
	~DesktopLayoutHelperEntry()
	{
	}
	int    SvcView; // PPDesktop::svcviewXXX, 0 - корневой контейнер, соответствующий всей области рабочего стола. 
	FRect  InitSize;
	void * Ptr;
};

//typedef void (__stdcall * FlexSetupProc)(SUiLayout * pItem, /*float size[4]*/const SUiLayout::Result & rR);

static void __stdcall Desktop_LayoutEntrySetupProc(SUiLayout * pItem, const SUiLayout::Result & rR)
{
	DesktopLayoutHelperEntry * p_entry = static_cast<DesktopLayoutHelperEntry *>(SUiLayout::GetManagedPtr(pItem));
	if(p_entry) {
		switch(p_entry->SvcView) {
			case 0:
				break;
			case PPDesktop::svcviewCommand:
				if(p_entry->Ptr) {
					FRect fr = rR;
					const SUiLayout * p_parent = pItem->GetParent();
					if(p_parent) {
						const FRect parent_frame = p_parent->GetFrame();
						fr.Move__(parent_frame.a.x, parent_frame.a.y);
					}	
					PPCommand * p_cmd = static_cast<PPCommand *>(p_entry->Ptr);
					p_cmd->LoR = rR;
					p_cmd->P = fr.a;
				}
				break;
			case PPDesktop::svcviewBizScore:
				if(p_entry->Ptr) {
					//SUiLayout * p_root = pItem->GetRoot();
					/*if(SUiLayout::GetManagedPtr(p_root))*/ {
						//const FRect desktop_rect = static_cast<DesktopLayoutHelperEntry *>(SUiLayout::GetManagedPtr(p_root))->InitSize;
						PPBizScoreWindow * p_bs_win = static_cast<PPBizScoreWindow *>(p_entry->Ptr);
						p_bs_win->MoveOnLayout(rR);
					}
				}
				break;
		}
	}
}

void PPDesktop::Layout()
{
	const TRect cli_rect = getClientRect();
	TSCollection <DesktopLayoutHelperEntry> layout_helper_entry_list;
	SUiLayout layout;
	{
		SUiLayoutParam alb;
		alb.SetContainerDirection(DIREC_UNKN);
		alb.SetFixedSize(cli_rect);
		layout.SetLayoutBlock(alb);
		{
			DesktopLayoutHelperEntry * p_helper_entry = layout_helper_entry_list.CreateNewItem();
			p_helper_entry->SvcView = 0;
			p_helper_entry->Ptr = this;
			p_helper_entry->InitSize = cli_rect;
			layout.SetCallbacks(0, Desktop_LayoutEntrySetupProc, p_helper_entry);
		}
	}
	{
		TWindow * p_bizsc_win = GetServiceView(svcviewBizScore);
		if(p_bizsc_win) {
			SUiLayoutParam alb;
			alb.SetVariableSizeX(SUiLayoutParam::szByContainer, 0.3f);
			alb.SetVariableSizeY(SUiLayoutParam::szByContainer, 0.3f);
			alb.GravityX = SIDE_RIGHT;
			alb.GravityY = SIDE_BOTTOM;
			SUiLayout * p_lo_item = layout.InsertItem();
			p_lo_item->SetLayoutBlock(alb);
			{
				DesktopLayoutHelperEntry * p_helper_entry = layout_helper_entry_list.CreateNewItem();
				p_helper_entry->SvcView = svcviewBizScore;
				p_helper_entry->Ptr = p_bizsc_win;
				p_lo_item->SetCallbacks(0, Desktop_LayoutEntrySetupProc, p_helper_entry);
			}
		}
	}
	{
		SUiLayout * p_lo_center_frame = layout.InsertItem();
		{
			SUiLayoutParam alb(DIREC_HORZ, SUiLayoutParam::alignStart, SUiLayoutParam::alignStart);
			alb.GravityX = SIDE_CENTER;
			alb.GravityY = SIDE_CENTER;
			alb.Flags |= SUiLayoutParam::fContainerWrap;
			alb.SetVariableSizeX(SUiLayoutParam::szByContainer, 1.0f);
			alb.SetVariableSizeY(SUiLayoutParam::szByContainer, 1.0f);
			p_lo_center_frame->SetLayoutBlock(alb);
		}
		{
			const PPCommandItem * p_item = 0;
			for(uint i = 0; p_item = P_ActiveDesktop->Next(&i);) {
				const PPCommand * p_cmd = p_item->IsKind(PPCommandItem::kCommand) ? static_cast<const PPCommand *>(p_item) : 0;
				if(p_cmd) {
					SUiLayout * p_lo_item = p_lo_center_frame->InsertItem();
					SUiLayoutParam alb;
					alb.SetFixedSizeX(static_cast<float>(IconSize * 2));
					alb.SetFixedSizeY(static_cast<float>(IconSize * 2));
					alb.Margin.Set(static_cast<float>(IconGap));
					//alb.Padding.Set(IconGap);
					p_lo_item->SetLayoutBlock(alb);
					{
						DesktopLayoutHelperEntry * p_helper_entry = layout_helper_entry_list.CreateNewItem();
						p_helper_entry->SvcView = svcviewCommand;
						p_helper_entry->Ptr = const_cast<PPCommand *>(p_cmd); // @badcast
						p_lo_item->SetCallbacks(0, Desktop_LayoutEntrySetupProc, p_helper_entry);
					}
				}
			}
		}
	}
	layout.Evaluate(0);
	Update(0, 1);
}

void PPDesktop::ArrangeIcons()
{
	RECT   cli_rect;
	const PPCommandItem * p_item = 0;
	PPCommand * p_cmd = 0;
	GetClientRect(H(), &cli_rect);
	for(uint i = 0; p_item = P_ActiveDesktop->Next(&i);) {
		p_cmd = p_item->IsKind(PPCommandItem::kCommand) ? static_cast<PPCommand *>(p_item->Dup()) : 0;
		if(p_cmd) {
			const SPoint2S preserve_pnt = p_cmd->P;
			if(ArrangeIcon(p_cmd)) {
				P_ActiveDesktop->Update(i-1, static_cast<const PPCommandItem *>(p_cmd));
				if(preserve_pnt != p_cmd->P)
					State |= stChanged;
			}
			ZDELETE(p_cmd);
		}
	}
	Update(0, 1);
}

int PPDesktop::ArrangeIcon(PPCommand * pCmd)
{
	int    ok = 0;
	if(pCmd) {
		SPoint2S coord = pCmd->P;
		ok = ArrangeIcon(&coord);
		if(ok)
			pCmd->P = coord;
	}
	return ok;
}

int PPDesktop::ArrangeIcon(SPoint2S * pCoord)
{
	int    ok = 1;
	int    intersect = 0;
	TRect  c;
	TRect  cr;
	TRect  desk_rect;
	TRect  bizs_rect;
	TRect  intrs_rect;
	SPoint2S coord;
	coord = *pCoord;
	int    delta_x = -(coord.x % ((IconSize * 2) + IconGap));
	int    delta_y = -(coord.y % ((IconSize * 2) + IconGap));
	coord.Add(delta_x, delta_y);
	coord = coord + IconGap;
	cr = getClientRect();
	CalcIconRect(coord, c);
	const TWindow * p_bsw = GetServiceView(svcviewBizScore); //GetBizScoreWnd();
	if(p_bsw) {
		desk_rect = getRect();
		bizs_rect = p_bsw->getRect();
		bizs_rect.b.x  -= bizs_rect.a.x;
		bizs_rect.b.y  -= bizs_rect.a.y;
		bizs_rect.a.x  -= desk_rect.a.x;
		bizs_rect.a.y  -= desk_rect.a.y;
		bizs_rect.b.x  += bizs_rect.a.x;
		bizs_rect.b.y  += bizs_rect.a.y;
		intersect = bizs_rect.Intersect(c, &intrs_rect);
	}
	if(intersect || c.a.x < 0 || c.a.y < 0 || c.b.x > cr.b.x || c.b.y > cr.b.y) {
		ok = 0;
		if(P_ActiveDesktop->SearchFreeCoord(cr, *this, &coord) > 0)
			ok = -1;
	}
	ASSIGN_PTR(pCoord, coord);
	return ok;
}

void PPDesktop::Update(const TRect * pR, bool drawBackgnd)
{
	if(pR)
		invalidateRect(*pR, drawBackgnd);
	else
		invalidateAll(drawBackgnd);
	::UpdateWindow(H());
}

int PPDesktop::EditIconName(long id)
{
	int    ok = -1;
	uint   pos = 0;
	const  PPCommandItem * p_item = P_ActiveDesktop->SearchByID(id, &pos);
	PPCommand * p_cmd = (p_item && p_item->IsKind(PPCommandItem::kCommand)) ? static_cast<PPCommand *>(p_item->Dup()) : 0;
	if(p_cmd) {
		if(EditName(p_cmd->Name) > 0)
			ok = P_ActiveDesktop->Update(pos, p_cmd);
		ZDELETE(p_cmd);
	}
	return ok;
}

int PPDesktop::DoCommand(SPoint2S coord)
{
	int    ok = -1;
	uint   pos = 0;
	const  PPCommandItem * p_item = P_ActiveDesktop->SearchByCoord(coord, *this, &pos);
	PPCommand * p_cmd = (p_item && p_item->IsKind(PPCommandItem::kCommand)) ? static_cast<PPCommand *>(p_item->Dup()) : 0;
	if(p_cmd) {
		PPCommandDescr cmd_descr;
		ok = cmd_descr.DoCommand(p_cmd, 0);
		if(p_cmd->CmdID != PPCMD_QUIT) {
			P_ActiveDesktop->Update(pos, p_cmd);
			State |= stChanged;
		}
	}
	ZDELETE(p_cmd);
	if(!ok)
		PPError();
	return ok;
}

ushort PPDesktop::Execute()
{
	RECT   r;
	HWND   h_frame = APPL->GetFrameWindow();
	GetWindowRect(h_frame, &r);
	r.bottom = r.bottom - r.top - 2;
	DWORD style = WS_CHILD | WS_CLIPSIBLINGS | WS_TABSTOP;
	SString title = P_ActiveDesktop->Name;
	HW = ::CreateWindowEx(WS_EX_COMPOSITED, SUcSwitch(PPDesktop::WndClsName), SUcSwitch(title.Transf(CTRANSF_INNER_TO_OUTER)), 
		style, 0, 0, r.right - r.left - 18, r.bottom, h_frame, 0, TProgram::GetInst(), this);
	ShowWindow(H(), SW_SHOW);
	UpdateWindow(H());
	HwndTT = ::CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, NULL, WS_POPUP|TTS_NOPREFIX|TTS_ALWAYSTIP,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, H(), NULL, TProgram::GetInst(), 0);
	SetWindowPos(HwndTT, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE | SWP_NOACTIVATE);
	CreateServiceView(svcviewBizScore);
	return (H() != 0);
}

int PPDesktop::Advise()
{
	int    ok = -1;
	// @v11.2.6 UserInterfaceSettings ui_cfg;
	// @v11.2.6 ui_cfg.Restore();
	const ThreadID thr_id = DS.GetConstTLA().GetThreadID();
	// @v11.2.6 if(ui_cfg.Flags & UserInterfaceSettings::fShowBizScoreOnDesktop) {
	if(APPL->GetUiSettings().Flags & UserInterfaceSettings::fShowBizScoreOnDesktop) { // @v11.2.6
		long   cookie = 0;
		PPAdviseBlock adv_blk;
		adv_blk.Kind       = PPAdviseBlock::evBizScoreChanged;
		adv_blk.TId        = thr_id;
		adv_blk.ObjType    = PPOBJ_BIZSCORE;
		adv_blk.Proc       = PPDesktop::HandleNotifyEvent;
		adv_blk.ProcExtPtr = this;
		if((ok = DS.Advise(&cookie, &adv_blk)) > 0)
			Cookies.addUnique(cookie);
	}
	// @v10.9.0 {
	{
		long   cookie = 0;
		PPAdviseBlock adv_blk;
		adv_blk.Kind = PPAdviseBlock::evEventCreated;
		adv_blk.TId = thr_id;
		adv_blk.ObjType = PPOBJ_EVENTSUBSCRIPTION;
		adv_blk.Proc    = PPDesktop::HandleNotifyEvent;
		adv_blk.ProcExtPtr = this;
		if((ok = DS.Advise(&cookie, &adv_blk)) > 0)
			Cookies.addUnique(cookie);
	}
	// } @v10.9.0 
	return ok;
}

void PPDesktop::Unadvise()
{
	for(uint i = 0; i < Cookies.getCount(); i++)
		DS.Unadvise(Cookies.at(i));
}

/*static*/int PPDesktop::HandleNotifyEvent(int kind, const PPNotifyEvent * pEv, void * procExtPtr)
{
	int    ok = -1;
	if(pEv) {
		if(kind == PPAdviseBlock::evBizScoreChanged) {
			if(pEv->ObjType) {
				PPDesktop * p_desk = static_cast<PPDesktop *>(procExtPtr);
				if(p_desk) {
					// @v10.9.0 ok = p_desk->LoadBizScoreData();
					// @v10.9.0 int PPDesktop::LoadBizScoreData()
					// @v10.9.0 {
					{
						PPBizScoreWindow * p_wnd = static_cast<PPBizScoreWindow *>(p_desk->GetServiceView(svcviewBizScore));//p_desk->GetBizScoreWnd();
						ok = p_wnd ? p_wnd->LoadData() : 0;
					}
					// } @v10.9.0 
					ok = 1;
				}
			}
		}
		else if(kind == PPAdviseBlock::evEventCreated) {
			if(pEv->ObjType == PPOBJ_EVENTSUBSCRIPTION && pEv->ObjID && pEv->Action > 0) {
				PPDesktop * p_desk = static_cast<PPDesktop *>(procExtPtr);
				if(p_desk) {
					const  PPID cur_user_id = LConfig.UserID;
					PPObjEventSubscription evs_obj(0);
					PPEventSubscriptionPacket evs_pack;
					if(evs_obj.Fetch(pEv->ObjID, &evs_pack) > 0) {
						if(evs_pack.UserList.Search(cur_user_id, 0, 0)) {
							SString msg_buf;
							SString temp_buf;
							COLORREF clr = GetColorRef(SClrLightskyblue);
							evs_pack.GetExtStrData(PPEventSubscriptionPacket::extssMessage, msg_buf);
							msg_buf.SetIfEmpty(evs_pack.Rec.Name);
							if(pEv->ExtInt_) {
								PPEventCore ec;
								PPEventCore::Packet ec_pack;
								if(ec.Get(pEv->ExtInt_, &ec_pack) > 0) {
									if(ec_pack.Oid.IsFullyDefined()) {
										GetObjectName(ec_pack.Oid.Obj, ec_pack.Oid.Id, temp_buf);
										msg_buf.CR().Cat(temp_buf);
									}
								}
							}
							PPTooltipMessage(msg_buf, 0, p_desk->H(), 30000, clr, SMessageWindow::fTextAlignLeft|
								SMessageWindow::fOpaque|SMessageWindow::fSizeByText|SMessageWindow::fTopmost|SMessageWindow::fShowOnRUCorner);
						}
					}
				}
			}
		}
	}
	return ok;
}
//
//
//
int PPDesktop::CreateServiceView(int svcviewId)
{
	int    ok = 0;
	if(H()) {
		if(svcviewId == svcviewBizScore) {
			// @v11.2.6 UserInterfaceSettings ui_cfg;
			// @v11.2.6 ui_cfg.Restore();
			// @v11.2.6 if(ui_cfg.Flags & UserInterfaceSettings::fShowBizScoreOnDesktop) {
			if(APPL->GetUiSettings().Flags & UserInterfaceSettings::fShowBizScoreOnDesktop) { // @v11.2.6
				PPBizScoreWindow * p_win = new PPBizScoreWindow(H());
				if(p_win) {
					p_win->Create();
					HBizScoreWnd = p_win->H();
					ok = 1;
				}
			}
		}
		else
			ok = -1;
	}
	return ok;
}

TWindow * PPDesktop::GetServiceView(int svcviewId)
{
	TWindow * p_result = 0;
	if(svcviewId == svcviewBizScore) {
		if(HBizScoreWnd)
	   		p_result = static_cast<PPBizScoreWindow *>(TView::GetWindowUserData(HBizScoreWnd));
		if(!p_result)
			HBizScoreWnd = 0;
	}
	return p_result;
}

/* @v11.0.0 replaced with PPDesktop::GetServiceView(int)
PPBizScoreWindow * PPDesktop::GetBizScoreWnd()
{
	PPBizScoreWindow * p_bizscore_wnd = 0;
	if(HBizScoreWnd)
	   	p_bizscore_wnd = static_cast<PPBizScoreWindow *>(TView::GetWindowUserData(HBizScoreWnd));
	if(!p_bizscore_wnd)
		HBizScoreWnd = 0;
	return p_bizscore_wnd;
}*/

/* @v11.0.0 replaced with PPDesktop::CreateServiceView(int)
int PPDesktop::CreateBizScoreWnd()
{
	int    ok = 0;
	if(H()) {
		UserInterfaceSettings ui_cfg;
		ui_cfg.Restore();
		if(ui_cfg.Flags & UserInterfaceSettings::fShowBizScoreOnDesktop) {
			PPBizScoreWindow * p_win = new PPBizScoreWindow(H());
			if(p_win) {
				p_win->Create();
				HBizScoreWnd = p_win->H();
				ok = 1;
			}
		}
	}
	return ok;
}*/

void PPDesktop::WMHCreate()
{
	HDC    hdc = GetDC(H());
	SString temp_buf;
	{
		{
			LOGFONT log_font;
			MEMSZERO(log_font);
			log_font.lfCharSet = DEFAULT_CHARSET;
			PPGetSubStr(PPTXT_FONTFACE, PPFONT_ARIAL, temp_buf);
			STRNSCPY(log_font.lfFaceName, SUcSwitch(temp_buf));
			log_font.lfHeight = 14;
			log_font.lfQuality = CLEARTYPE_QUALITY;
			Ptb.SetFont(fontText, ::CreateFontIndirect(&log_font));
		}
		Ptb.SetBrush(brushTextRect,    SPaintObj::bsSolid, SColor(RGB(0x80, 0x80, 0xC0)), 0);
		Ptb.SetBrush(brushSelTextRect, SPaintObj::bsSolid, SColor(RGB(0x40, 0x00, 0x80)), 0);
		Ptb.SetPen(penTextRect, SPaintObj::psSolid, 1, SColor(RGB(0x80, 0x80, 0xC0)));
		Ptb.SetPen(penSelTextRect, SPaintObj::psSolid, 1, SColor(RGB(0x40, 0x00, 0x80)));
		{
			const long c = P_ActiveDesktop ? (P_ActiveDesktop->Icon.ToLong() - 1) : 0;
			Ptb.SetColor(colorBkg, SColor((COLORREF)(c >= 0 ? c : PPDesktop::GetDefaultBgColor())));
		}
		Ptb.SetBrush(brushBkg, SPaintObj::bsSolid, Ptb.GetColor(colorBkg), 0);
	}
	TView::SetWindowUserData(H(), static_cast<PPDesktop *>(this));
	::SetCursor(::LoadCursor(0, IDC_ARROW));
	SetFocus(H());
	SendMessage(H(), WM_NCACTIVATE, TRUE, 0L);
	ReleaseDC(H(), hdc);
}

/*static*/int PPDesktop::GetDeskName(const S_GUID & rDesktopUuid, SString & rDeskName)
{
	int    ok = -1;
	DbProvider * p_dict = CurDict;
	PPCommandMngr * p_mgr = 0;
	PPCommandGroup * p_desk = 0;
	rDeskName.Z();
	if(!!rDesktopUuid && p_dict) {
		SString db_symb;
		PPCommandGroup desktop_list, desktop_list_from_bin;
		const PPCommandItem * p_item = 0;
		p_dict->GetDbSymb(db_symb);
		// @erik v10.6.7 {
		THROW(p_mgr = GetCommandMngr(PPCommandMngr::ctrfReadOnly, cmdgrpcDesktop, 0));
		p_mgr->Load__2(&desktop_list, db_symb, PPCommandMngr::fRWByXml);
		p_item = desktop_list.SearchByUuid(rDesktopUuid, 0);
		if(!p_item) {
			THROW(p_mgr->Load_Deprecated(&desktop_list_from_bin));
			p_item = desktop_list_from_bin.SearchByUuid(rDesktopUuid, 0);
		}
		// } @erik
		p_desk = (p_item && p_item->IsKind(PPCommandItem::kGroup)) ? static_cast<PPCommandGroup *>(p_item->Dup()) : 0;
		THROW_PP(p_desk && p_desk->IsDbSymbEq(db_symb), PPERR_DESKNOTFOUND);
		rDeskName = p_desk->Name;
	}
	CATCHZOK
	ZDELETE(p_desk);
	ZDELETE(p_mgr);
	return ok;
}

int PPDesktop::SaveDesktop(PPCommandMngr * pMgr, PPCommandGroup * pDeskList)
{
	int    ok = -1;
	PPCommandMngr * p_mgr = 0;
	PPCommandGroup * p_desktop_list = 0;
	if(P_ActiveDesktop && State & stChanged) {
		if(!pMgr) {
			THROW_MEM(p_desktop_list = new PPCommandGroup);
			do {
				//@erik v10.6.7 {
				//const  PPCommandItem * p_item = 0;
				ok = BIN(p_mgr = GetCommandMngr(PPCommandMngr::ctrfSkipObsolete, cmdgrpcDesktop, 0)); 
				p_mgr->Load__2(p_desktop_list, 0, PPCommandMngr::fRWByXml);
				// } @erik
			} while(ok == 0 && CONFIRM(PPCFM_WAITDESKUNLOCK));
		}
		else {
			p_mgr = pMgr;
			p_desktop_list = pDeskList;
		}
		THROW_INVARG(p_desktop_list);
		THROW(ok);
		{
			uint   pos = 0;
			//
			// Сохраняем только изменения в расположении, названии и кол-ве иконок на рабочем столе
			//
			//@erik v10.6.7 {
			const  PPCommandItem * p_item = p_desktop_list->SearchByID(P_ActiveDesktop->GetID(), 0);
			if(!p_item) {
				p_mgr->Load_Deprecated(p_desktop_list);
				p_item = p_desktop_list->SearchByID(P_ActiveDesktop->GetID(), 0);
			}
			// } @erik
			if(p_item && p_item->IsKind(PPCommandItem::kGroup)) {
				PPCommandGroup * p_prev = static_cast<PPCommandGroup *>(p_item->Dup());
				if(p_prev) {
					P_ActiveDesktop->Flags = p_prev->Flags;
					P_ActiveDesktop->Icon  = p_prev->Icon;
					P_ActiveDesktop->SetLogo(p_prev->GetLogo());
					P_ActiveDesktop->Name  = p_prev->Name;
				}
				P_ActiveDesktop->Type = cmdgrpcDesktop; // @v11.0.0
				THROW(p_mgr->Save__2(P_ActiveDesktop, PPCommandMngr::fRWByXml)); // @erik v10.6.1
				ok = 1;
			}
		}
	}
	CATCHZOK
	if(!pMgr) {
		ZDELETE(p_mgr);
		ZDELETE(p_desktop_list);
	}
	State &= ~stChanged;
	return ok;
}

int PPDesktop::WaitCommand()
{
	class WaitCmdDialog : public TDialog {
	public:
		explicit WaitCmdDialog(/*long desktopID*/const S_GUID & rDesktopUuid) : TDialog(DLG_WAITCMD), __Locking(0)
		{
			if(!!rDesktopUuid)
				AssocList.ReadFromProp(/*desktopID*/rDesktopUuid);
			{
				S_GUID zero_uuid;
				CommAssocList.ReadFromProp(/*0L*/zero_uuid.Z());
			}
			DefInputLine = CTL_WAITCMD_INPUT;
		}
		int    getDTS(PPDesktopAssocCmd * pCommand, SString * pAddendum)
		{
			int    ok = -1;
			if(Command.CmdID) {
				ASSIGN_PTR(pCommand, Command);
				ASSIGN_PTR(pAddendum, Addendum);
				ok = 1;
			}
			else if(CodeBuf.NotEmpty()) {
				SetStatus(CodeBuf);
			}
			return ok;
		}
	private:
	    DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isCmd(cmWinKeyDown)) {
				SString code;
				KeyDownCommand * p_cmd = static_cast<KeyDownCommand *>(event.message.infoPtr);
				Command.Z();
				Addendum.Z();
				if(p_cmd && p_cmd->GetKeyName(code, 1) > 0) {
					CodeBuf = code;
					if(AssocList.GetByCode(CodeBuf, 0, &Command, &Addendum) > 0 || CommAssocList.GetByCode(CodeBuf, 0, &Command, &Addendum) > 0)
						endModal(cmOK);
					else
						SetStatus(code);
				}
				clearEvent(event);
			}
			else if(event.isCmd(cmInputUpdated)) {
				if(event.isCtlEvent(CTL_WAITCMD_INPUT)) {
					if(!__Locking) {
						__Locking = 1;
						Command.Z();
						Addendum.Z();
						getCtrlString(CTL_WAITCMD_INPUT, CodeBuf.Z());
						AssocList.GetByCode(CodeBuf, 0, &Command, &Addendum) > 0 || CommAssocList.GetByCode(CodeBuf, 0, &Command, &Addendum) > 0;
						__Locking = 0;
					}
				}
			}
		}
		int    SetStatus(const char * pCode)
		{
			SString buf, msg;
			PPLoadString(PPMSG_ERROR, PPERR_CMDASSCNOTFOUND, buf);
			msg.Printf(buf.cptr(), pCode);
			setStaticText(CTL_WAITCMD_STATUS, msg);
			setCtrlString(CTL_WAITCMD_INPUT, buf.Z());
			return 1;
		}
		PPDesktopAssocCmdPool AssocList;
		PPDesktopAssocCmdPool CommAssocList;
		PPDesktopAssocCmd Command;
		SString Addendum;
		SString CodeBuf;
		int    __Locking;
	};
	int    ok = -1;
	PPDesktopAssocCmd cmd;
	SString addendum;
	WaitCmdDialog * p_dlg = 0;
	MEMSZERO(cmd);
	THROW(CheckDialogPtr(&(p_dlg = new WaitCmdDialog(P_ActiveDesktop->Uuid))));
	while(ok < 0 && ExecView(p_dlg) == cmOK) {
		if(p_dlg->getDTS(&cmd, &addendum) > 0)
			ok = 1;
	}
	ZDELETE(p_dlg);
	if(ok > 0) {
		int    use_buf = 0;
		SBuffer buf;
		PPCommandDescr cmd_descr;
		if(addendum.NotEmptyS()) {
			buf.Write(addendum);
			use_buf = 1;
		}
		THROW(cmd_descr.DoCommandSimple(cmd.CmdID, 0, 0, (use_buf ? &buf : 0)));
	}
	CATCHZOKPPERR
	delete p_dlg;
	return ok;
}

IMPL_HANDLE_EVENT(PPDesktop)
{
	const PPRights & r_orts = ObjRts;
	const int is_master = PPMaster;
	TWindow::handleEvent(event);
	if(TVCOMMAND) {
		switch(TVCMD) {
			case cmaInsert:
				if(is_master || r_orts.CheckDesktopID(P_ActiveDesktop->GetID(), PPR_MOD)) {
					SPoint2S coord;
					coord = *static_cast<const POINT *>(event.message.infoPtr);
					PPCommand cmd;
					if(EditCmdItem(P_ActiveDesktop, &cmd, cmdgrpcDesktop) > 0) {
						uint   pos = 0;
						RECT   cr;
						PPCommandGroup desktop_list;
						//@erik v10.6.7 {
						PPCommandMngr * p_mgr = GetCommandMngr(PPCommandMngr::ctrfSkipObsolete, cmdgrpcDesktop, 0);
						p_mgr->Load__2(&desktop_list, 0, PPCommandMngr::fRWByXml);
						// } @erik
						if(p_mgr) {
							cmd.ID = desktop_list.GetUniqueID();
							GetClientRect(H(), &cr);
							if(ArrangeIcon(&coord)) {
								TRect ir;
								if(P_ActiveDesktop->SearchByCoord(coord, *this, 0))
									P_ActiveDesktop->SearchFreeCoord(cr, *this, &coord);
								cmd.P = coord;
								P_ActiveDesktop->Add(-1, &cmd);
								P_ActiveDesktop->GetIconRect(cmd.GetID(), *this, &ir);
								State |= stChanged;
								if(!SaveDesktop(p_mgr, &desktop_list)) {
									PPErrorTooltip(-1, 0); // @v12.3.12
								}
								Update(&ir, 0);
							}
							ZDELETE(p_mgr);
						}
						else {
							PPErrorTooltip(-1, 0);
						}
					}
				}
				else
					PPErrorTooltip(-1, 0);
				break;
			case cmaEdit: // Редактирование иконки
				if(is_master || r_orts.CheckDesktopID(P_ActiveDesktop->GetID(), PPR_MOD)) {
					uint   pos = 0;
					SPoint2S coord;
					coord = *static_cast<const POINT *>(event.message.infoPtr);
					const  PPCommandItem * p_item = P_ActiveDesktop->SearchByCoord(coord, *this, &pos);
					PPCommand * p_cmd = (p_item && p_item->IsKind(PPCommandItem::kCommand)) ? static_cast<PPCommand *>(p_item->Dup()) : 0;
					if(p_cmd && EditCmdItem(P_ActiveDesktop, p_cmd, cmdgrpcDesktop) > 0) {
						TRect ir;
						P_ActiveDesktop->Update(pos, p_cmd);
						P_ActiveDesktop->GetIconRect(p_cmd->GetID(), *this, &ir);
						State |= stChanged;
						Update(&ir, 0);
					}
					ZDELETE(p_cmd);
				}
				else
					PPErrorTooltip(-1, 0);
				break;
			case cmaDelete:
				if(is_master || r_orts.CheckDesktopID(P_ActiveDesktop->GetID(), PPR_MOD)) {
					uint   pos = 0;
					SPoint2S coord;
					coord = *static_cast<const POINT *>(event.message.infoPtr);
					const  PPCommandItem * p_item = P_ActiveDesktop->SearchByCoord(coord, *this, &pos);
					PPCommand * p_cmd = (p_item && p_item->IsKind(PPCommandItem::kCommand)) ? static_cast<PPCommand *>(p_item->Dup()) : 0;
					if(p_cmd && CONFIRM(PPCFM_DELICON)) {
						TRect ir;
						PPCommandGroup desktop_list;
						//@erik v10.6.7 {
						PPCommandMngr * p_mgr = GetCommandMngr(PPCommandMngr::ctrfSkipObsolete, cmdgrpcDesktop, 0);
						p_mgr->Load__2(&desktop_list, 0, PPCommandMngr::fRWByXml);
						// } @erik
						if(p_mgr) {
							P_ActiveDesktop->Remove(pos);
							Selected = (Selected == p_cmd->GetID()) ? 0 : Selected;
							CalcIconRect(p_cmd->P, ir);
							State |= stChanged;
							SaveDesktop(p_mgr, &desktop_list);
							Update(&ir, 1);
							ZDELETE(p_mgr);
						}
						else
							PPErrorTooltip(-1, 0);
					}
					ZDELETE(p_cmd);
				}
				else
					PPErrorTooltip(-1, 0);
				break;
			case cmaRename:
				{
					uint   pos = 0;
					SPoint2S coord;
					coord = *static_cast<const POINT *>(event.message.infoPtr);
					const  PPCommandItem * p_item = P_ActiveDesktop->SearchByCoord(coord, *this, &pos);
					PPCommand * p_cmd = (p_item && p_item->IsKind(PPCommandItem::kCommand)) ? static_cast<PPCommand *>(p_item->Dup()) : 0;
					if(p_cmd) {
						TRect ir;
						EditIconName(p_cmd->GetID());
						P_ActiveDesktop->GetIconRect(p_cmd->GetID(), *this, &ir);
						State |= stChanged;
						Update(&ir, 1);
					}
					ZDELETE(p_cmd);
				}
				break;
			case cmSelDesktop:
				{
					S_GUID desktop_uuid = P_ActiveDesktop->Uuid;
					if(is_master || r_orts.CheckDesktopID(P_ActiveDesktop->GetID(), PPR_INS)) {
						if(SelectCommandGroup(desktop_uuid, 0, 0, cmdgrpcDesktop, false, 0) /* @v11.0.0 && desktop_uuid != P_ActiveDesktop->Uuid*/)
							PPDesktop::Open(desktop_uuid, 0/*createIfZero*/);
					}
				}
				break;
			case cmEditDesktops: // Редактирование опций рабочего стола
				{
					S_GUID desktop_uuid = P_ActiveDesktop->Uuid;
					if(EditCommandGroup(0, desktop_uuid, cmdgrpcDesktop) > 0)
						PPDesktop::Open(desktop_uuid, 0/*createIfZero*/);
				}
				break;
			case cmShowBizScoreOnDesktop:
				CreateServiceView(svcviewBizScore);
				break;
			default:
				return;
		}
	}
	else if(TVKEYDOWN) {
		switch(TVKEY) {
			case kbF2:
				if(Selected) {
					if(is_master || r_orts.CheckDesktopID(P_ActiveDesktop->GetID(), PPR_MOD)) {
						TRect ir;
						EditIconName(Selected);
						P_ActiveDesktop->GetIconRect(Selected, *this, &ir);
						//IsChanged = 1;
						State |= stChanged;
						Update(&ir, 1);
					}
					else
						PPErrorTooltip(-1, 0);
				}
				break;
			case kbEnter:
				{
					uint   pos = 0;
					const  PPCommandItem * p_item = P_ActiveDesktop->SearchByID(Selected, &pos);
					PPCommand * p_cmd = (p_item && p_item->IsKind(PPCommandItem::kCommand)) ? static_cast<PPCommand *>(p_item->Dup()) : 0;
					if(p_cmd) {
						PPCommandDescr cmd_descr;
						cmd_descr.DoCommand(p_cmd, 0);
						ZDELETE(p_cmd);
					}
				}
				break;
			case kbUp:
			case kbDown:
			case kbLeft:
			case kbRight:
				{
					int is_first = 0;
					const PPCommandItem * p_item = (Selected) ? P_ActiveDesktop->SearchByID(Selected, 0) :
						(is_first = 1, P_ActiveDesktop->SearchFirst(0));
					PPCommand * p_cmd = (p_item && p_item->IsKind(PPCommandItem::kCommand)) ? static_cast<PPCommand *>(p_item->Dup()) : 0;
					if(p_cmd) {
						if(!is_first) {
							POINT coord;
							PPCommandFolder::Direction direction = PPCommandFolder::nextUp; // @v10.3.0 @fix init
							coord.x = p_cmd->P.x;
							coord.y = p_cmd->P.y;
							ZDELETE(p_cmd);
							if(TVKEY == kbLeft)
								direction = PPCommandFolder::nextLeft;
							else if(TVKEY == kbRight)
								direction = PPCommandFolder::nextRight;
							else if(TVKEY == kbUp)
								direction = PPCommandFolder::nextUp;
							else if(TVKEY == kbDown)
								direction = PPCommandFolder::nextDown;
							p_item = P_ActiveDesktop->SearchNextByCoord(coord, *this, direction, 0);
							p_cmd = (p_item && p_item->IsKind(PPCommandItem::kCommand)) ? static_cast<PPCommand *>(p_item->Dup()) : 0;
						}
						if(p_cmd) {
							TRect ir;
							P_ActiveDesktop->GetIconRect(Selected, *this, &ir);
							Selected = p_cmd->GetID();
							Update(&ir, 1);
							P_ActiveDesktop->GetIconRect(Selected, *this, &ir);
							Update(&ir, 0);
							ZDELETE(p_cmd);
						}
					}
				}
				break;
			/*
			case kbDel:
				{
					const PPCommandItem * p_item = (Selected) ? P_ActiveDesktop->SearchByID(Selected, 0) : 0;
						PPCommand * p_cmd = (p_item && p_item->Kind == PPCommandItem::kCommand) ? (PPCommand*)p_item->Dup() : 0;
					if(p_cmd) {
						POINT coord;
						coord.x = p_cmd->X;
						coord.y = p_cmd->Y;
						ZDELETE(p_cmd);
						TView::messageCommand(this, cmaDelete, (void *)&coord);
					}
				}
				break;
			*/
			case kbIns:
				{
					POINT coord;
					MEMSZERO(coord);
					TView::messageCommand(this, cmaInsert, &coord);
				}
				break;
			case kbF11:
				WaitCommand();
				break;
			default:
				return;
		}
	}
	else
		return;
	clearEvent(event);
}

/*static*/LRESULT CALLBACK PPDesktop::DesktopWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PPDesktop * p_desk = static_cast<PPDesktop *>(TView::GetWindowUserData(hWnd));
	CREATESTRUCT * p_init_data = 0;
	TEvent e;
	switch(message) {
		case WM_CREATE:
			p_init_data = reinterpret_cast<CREATESTRUCT *>(lParam);
			p_desk = static_cast<PPDesktop *>(p_init_data->lpCreateParams);
			if(p_desk) {
				APPL->H_Desktop = hWnd;
				p_desk->HW = hWnd;
				p_desk->WMHCreate();
				{
					SRawInputInitArray riia;
					riia.Add(1, 6, 0, 0);
					SRawInputData::Register(&riia);
				}
				InvalidateRect(hWnd, 0, TRUE);
				UpdateWindow(hWnd);
				::PostMessage(hWnd, WM_USER, 0, 0);
				return 0;
			}
			else
				return -1;
		case WM_DESTROY:
			if(p_desk) {
				SETIFZ(p_desk->EndModalCmd, cmCancel);
				APPL->DelItemFromMenu(p_desk);
				p_desk->ResetOwnerCurrent();
				APPL->P_DeskTop->remove(p_desk);
				delete p_desk;
				TView::SetWindowUserData(hWnd, 0);
			}
			SRawInputData::Register(0);
			APPL->NotifyFrame(1);
			return 0;
		case WM_USER:
			{
				SString temp_buf;
				TView::SGetWindowText(hWnd, temp_buf);
				APPL->AddItemToMenu(temp_buf, p_desk);
			}
			return 0;
		case WM_SHOWWINDOW:
			::PostMessage(APPL->H_MainWnd, WM_SIZE, 0, 0);
			break;
		case WM_SIZE:
			if(lParam && p_desk) {
				if(PPDesktop_Use_LayoutFlex) { // @v11.0.0
					p_desk->Layout(); 
				}
				else {
					PPBizScoreWindow * p_bizscore_wnd = static_cast<PPBizScoreWindow *>(p_desk->GetServiceView(svcviewBizScore));//p_desk->GetBizScoreWnd();
					CALLPTRMEMB(p_bizscore_wnd, Move_W());
					p_desk->ArrangeIcons();
				}
			}
			break;
		case WM_RBUTTONUP:
			{
				if(p_desk) {
					SString temp_buf, menu_text;
					if(PPLoadTextWin(PPTXT_MENU_DESKTOP, temp_buf)) {
						POINT  coord;
						coord.x = LOWORD(lParam);
						coord.y = HIWORD(lParam);
						TMenuPopup menu;
						menu.AddSubstr(temp_buf, 0, cmaInsert); // Добавить
						menu.AddSubstr(temp_buf, 1, cmaEdit);   // Редактировать
						menu.AddSubstr(temp_buf, 2, cmaDelete); // Удалить
						menu.AddSubstr(temp_buf, 3, cmaRename); // Переименовать
						menu.AddSubstr(temp_buf, 4, cmSelDesktop); // Выбрать рабочий стол
						menu.AddSubstr(temp_buf, 5, cmEditDesktops);
						if(!p_desk->GetServiceView(svcviewBizScore)) {
							// @v11.2.6 UserInterfaceSettings ui_cfg;
							// @v11.2.6 ui_cfg.Restore();
							// @v11.2.6 if(ui_cfg.Flags & UserInterfaceSettings::fShowBizScoreOnDesktop)
							if(APPL->GetUiSettings().Flags & UserInterfaceSettings::fShowBizScoreOnDesktop) // @v11.2.6
								menu.AddSubstr(temp_buf, 6, cmShowBizScoreOnDesktop); // Показать бизнес-показатели
						}
						uint    cmd = 0;
						if(menu.Execute(hWnd, TMenuPopup::efRet, &cmd, 0) && cmd)
							TView::messageCommand(p_desk, cmd, &coord);
					}
				}
			}
			break;
		case WM_LBUTTONDBLCLK:
			if(p_desk) {
				SPoint2S p;
				p_desk->DoCommand(p.setwparam(lParam));
			}
			break;
		case WM_LBUTTONDOWN:
			if(p_desk) {
				RECT r;
				GetWindowRect(hWnd, &r);
				r.left   += p_desk->IconGap;
				r.right  -= p_desk->IconGap;
				r.top    += p_desk->IconGap;
				r.bottom -= p_desk->IconGap;
				ClipCursor(&r);
				if(hWnd != GetFocus())
					SetFocus(hWnd);
				SPoint2S coord;
				p_desk->BeginIconMove(coord.setwparam(lParam));
			}
			return 0;
		case WM_LBUTTONUP:
			if(p_desk) {
				SPoint2S coord;
				p_desk->EndIconMove(coord.setwparam(lParam));
				ClipCursor(0);
			}
			return 0;
		case WM_MOUSEMOVE:
			if(wParam == MK_LBUTTON && p_desk) {
				SPoint2S coord;
				p_desk->MoveIcon(coord.setwparam(lParam));
			}
			return 0;
		case WM_PAINT:
			CALLPTRMEMB(p_desk, Paint());
			return 0L;
		case WM_NCLBUTTONDOWN:
			if(hWnd != GetFocus())
				SetFocus(hWnd);
			break;
		case WM_SETFOCUS:
			{
				SetWindowPos(hWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);
				APPL->NotifyFrame(0);
				if(p_desk) {
					APPL->SelectTabItem(p_desk);
					e.what = TEvent::evBroadcast;
					e.message.command = cmReceivedFocus;
					e.message.infoView = 0;
					p_desk->handleEvent(e);
					p_desk->select();
					::InvalidateRect(hWnd, 0, TRUE); // @v11.2.8
					::UpdateWindow(hWnd); // @v11.2.8
				}
			}
			break;
		case WM_KILLFOCUS:
			APPL->NotifyFrame(0);
			if(p_desk) {
				e.what = TEvent::evBroadcast;
				e.message.command = cmReleasedFocus;
				e.message.infoView = 0;
				p_desk->handleEvent(e);
				p_desk->ResetOwnerCurrent();
			}
			break;
		case WM_KEYDOWN:
			if(wParam == VK_TAB) {
				if(GetKeyState(VK_CONTROL) & 0x8000 && p_desk != 0) {
					SetFocus(GetNextBrowser(hWnd, (GetKeyState(VK_SHIFT) & 0x8000) ? 0 : 1));
					return 0;
				}
			}
			p_desk->HandleKeyboardEvent(wParam);
			return 0;
		case WM_CHAR:
			if(wParam != VK_RETURN || LOBYTE(HIWORD(lParam)) != 0x1c)
				TView::messageKeyDown(p_desk, wParam);
			return 0;
		// @vmiller {
		case WM_INPUT:
			if(p_desk->ProcessRawInput(reinterpret_cast<void *>(lParam)) > 0)
				return 0;
			break;
		// } @vmiller
		default:
			break;
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}

int PPDesktop::Helper_AcceptInputString(const PPDesktop::InputArray * pInp, const PPDesktopAssocCmd & rCpItem, SString & rBuf)
{
	int    ok = 0;
	rBuf.Z();
	const  uint ic = pInp->getCount();
	if(ic) {
		for(uint i = 0; i < ic; i++) {
			uint   c = pInp->at(i).GetChar();
			rBuf.CatChar(c ? c : '?');
		}
		if(rCpItem.Flags & PPDesktopAssocCmd::cbfCR) {
			if(rBuf.Last() == '\x0D') {
				if(rBuf.TrimRight().NotEmpty())
					ok = 1;
			}
		}
		else if(rBuf.NotEmpty())
			ok = 1;
	}
	return ok;
}

int PPDesktop::ProcessCommandItem(const PPDesktop::InputArray * pInp, const PPDesktopAssocCmd & rCpItem)
{
	int    ok = -1;
	int    suite = 0;
	bool   do_run = false;
	SString param_buf;
	SString input_buf;
	if(rCpItem.DvcSerial.NotEmpty()) {
		if(pInp->DvcSerial.NotEmpty()) {
			//
			// В голове и в хвосте pDvcSerial могут находится служебные символы.
			// По этому, проверяем не точное соответствие, а ищем подстроку без учета регистра.
			//
			const size_t dsl = pInp->DvcSerial.Len();
			const size_t cpl = rCpItem.DvcSerial.Len();
			if(dsl > cpl) {
				for(uint i = 0; !suite && i <= (dsl - cpl); i++) {
					if(rCpItem.DvcSerial.CmpL(pInp->DvcSerial.cptr()+i, 1) == 0)
						suite = 1;
				}
			}
			else if(dsl == cpl && rCpItem.DvcSerial.CmpNC(pInp->DvcSerial) == 0)
				suite = 1;
		}
	}
	else
		suite = 1;
	if(suite) {
		const  uint ic = pInp->getCount();
		SString temp_buf;
		PPID   sc_id = 0;
		PPID   goods_id = 0;
		PPID   psnreg_id = 0;
		PPID   psn_id = 0;
		PPDesktopAssocCmd::CodeBlock cb;
		if(rCpItem.ParseCode(cb) > 0) {
			switch(cb.Type) {
				case PPDesktopAssocCmd::cbString:
					{
						if(Helper_AcceptInputString(pInp, rCpItem, temp_buf) && temp_buf.CmpNC(cb.AddedStrVal) == 0) {
							input_buf = temp_buf;
							do_run = true;
						}
					}
					break;
				case PPDesktopAssocCmd::cbKey:
					if(ic) {
						const KeyDownCommand & r_k = pInp->at(ic-1);
						if(r_k == cb.Key)
							do_run = true;
					}
					break;
				case PPDesktopAssocCmd::cbSCardCode:
					if(Helper_AcceptInputString(pInp, rCpItem, temp_buf)) {
						SETIFZ(P_ScObj, new PPObjSCard);
						if(P_ScObj) {
							SCardTbl::Rec sc_rec;
							if(P_ScObj->SearchCode(0, temp_buf, &sc_rec) > 0) {
								sc_id = sc_rec.ID;
								input_buf = temp_buf;
								do_run = true;
							}
						}
					}
					break;
				case PPDesktopAssocCmd::cbPersonRegister:
					if(Helper_AcceptInputString(pInp, rCpItem, temp_buf)) {
						SETIFZ(P_PsnObj, new PPObjPerson);
						if(P_PsnObj) {
							PPID reg_type_id = 0;
							if(cb.AddedIntVal)
								reg_type_id = cb.AddedIntVal;
							else if(cb.AddedStrVal.NotEmpty()) {
								PPObjRegisterType rt_obj;
								PPID   rt_id = 0;
								if(rt_obj.SearchSymb(&rt_id, cb.AddedStrVal) > 0) {
									reg_type_id = rt_id;
								}
							}
							PPIDArray psn_list;
							P_PsnObj->GetListByRegNumber(reg_type_id, 0, temp_buf, psn_list);
							if(psn_list.getCount()) {
								psn_id = psn_list.get(0);
								input_buf = temp_buf;
								do_run = true;
							}
						}
					}
					break;
				case PPDesktopAssocCmd::cbGoodsBarcode:
					if(Helper_AcceptInputString(pInp, rCpItem, temp_buf)) {
						SETIFZ(P_GObj, new PPObjGoods);
						if(P_GObj) {
							BarcodeTbl::Rec bc_rec;
							if(P_GObj->P_Tbl->SearchBarcode(temp_buf, &bc_rec) > 0) {
								goods_id = bc_rec.GoodsID;
								input_buf = temp_buf;
								do_run = true;
							}
						}
					}
					break;
			}
		}
	}
	if(do_run) {
		if(rCpItem.CmdID) {
			param_buf = rCpItem.CmdParam;
			param_buf.ReplaceStr("@input", input_buf, 0).Strip();
			PPCommandDescr cmd_descr;
			ok = cmd_descr.DoCommandSimple(rCpItem.CmdID, 0, (param_buf.NotEmpty() ? param_buf.cptr() : 0), 0);
		}
	}
	return ok;
}
//
//
//
void PPDesktop::InputArray::Clear()
{
	SVector::clear();
	DvcSerial.Z();
}

PPDesktop::RawInputBlock::RawInputBlock() : RawKeyStatus(0), LastRawKeyTime(ZEROTIME)
{
}

void PPDesktop::RawInputBlock::ClearInput()
{
	uint c = InpList.getCount();
	if(c) do {
		PPDesktop::InputArray * p_inp = InpList.at(--c);
		if(p_inp)
			p_inp->Clear();
		else
			InpList.atFree(c);
	} while(c);
}

PPDesktop::InputArray * PPDesktop::RawInputBlock::AddKeyDownCommand(const KeyDownCommand & rK, const char * pDvcSerial)
{
	InputArray * p_ret = 0;
	uint   i;
	for(i = 0; i < InpList.getCount(); i++) {
		PPDesktop::InputArray * p_inp = InpList.at(i);
		if(p_inp && p_inp->getCount() && p_inp->DvcSerial.CmpNC(pDvcSerial) == 0) {
			p_inp->insert(&rK);
			p_ret = p_inp;
			break;
		}
	}
	if(!p_ret) {
		for(i = 0; i < InpList.getCount(); i++) {
			PPDesktop::InputArray * p_inp = InpList.at(i);
			if(p_inp && p_inp->getCount() == 0) {
				p_inp->DvcSerial = pDvcSerial;
				p_inp->insert(&rK);
				p_ret = p_inp;
				break;
			}
		}
	}
	if(!p_ret) {
		PPDesktop::InputArray * p_inp = new PPDesktop::InputArray;
		p_inp->DvcSerial = pDvcSerial;
		p_inp->insert(&rK);
		InpList.insert(p_inp);
		p_ret = p_inp;
	}
	return p_ret;
}
//
//
//
int PPDesktop::ProcessRawInput(void * rawInputHandle)
{
	int    ok = -1;
	ENTER_CRITICAL_SECTION
	SRawInputData rid;
	if(rid.Get(rawInputHandle)) {
		RAWINPUT * p_ri = rid;
		if(p_ri->header.dwType == RIM_TYPEKEYBOARD) {
			const ushort vkey = p_ri->data.keyboard.VKey;
			const ushort kflags = p_ri->data.keyboard.Flags;
			//
			// Реагируем только на отпускание клавиши, причем SHIFT, CONTROL и ALT игнорируем - потом обработаем на высокоуровневом
			// сообщении - все равно специализированные устройства ввода не имеют обычно таких клавиш.
			//
			if(oneof3(vkey, VK_SHIFT, VK_CONTROL, VK_MENU)) {
				if(kflags == 0) {
					switch(vkey) {
						case VK_SHIFT: Rib.RawKeyStatus |= KeyDownCommand::stateShift; break;
						case VK_CONTROL: Rib.RawKeyStatus |= KeyDownCommand::stateCtrl; break;
						case VK_MENU: Rib.RawKeyStatus |= KeyDownCommand::stateAlt; break;
					}
				}
				else if(kflags & RI_KEY_BREAK) {
					switch(vkey) {
						case VK_SHIFT: Rib.RawKeyStatus &= ~KeyDownCommand::stateShift; break;
						case VK_CONTROL: Rib.RawKeyStatus &= ~KeyDownCommand::stateCtrl; break;
						case VK_MENU: Rib.RawKeyStatus &= ~KeyDownCommand::stateAlt; break;
					}
				}
			}
			else if(kflags & RI_KEY_BREAK || (kflags == 0 && Rib.RawKeyStatus)) {
				if(rid.GetDeviceName(Rib.DvcNameBuf_ = 0) > 0) {
					LTIME tm = getcurtime_();
					if(tm < Rib.LastRawKeyTime || DiffTime(tm, Rib.LastRawKeyTime, 4) > 1000) {
						Rib.ClearInput();
					}
					Rib.LastRawKeyTime = tm;
					KeyDownCommand _k;
					_k.State = Rib.RawKeyStatus;
					_k.Code = vkey;
					{
						uint   i;
						SString dvc_serial;
						UsbBasicDescrSt dev_descr, child_descr;
						if(SUsbDevice::ParsePath(Rib.DvcNameBuf_, dev_descr)) {
							for(i = 0; dvc_serial.IsEmpty() && i < UsbList.getCount(); i++) {
								const SUsbDevice * p_usb_dev = UsbList.at(i);
								if(p_usb_dev) {
									for(uint j = 0; j < p_usb_dev->GetChildren().getCount(); j++) {
										const UsbBasicDescrSt * p_child = p_usb_dev->GetChildren().at(j);
										if(p_child && dev_descr == *p_child) {
											dvc_serial = p_usb_dev->GetDescription().SerialNumber;
											break;
										}
									}
								}
							}
						}
						const char * p_dvc_serial = dvc_serial.NotEmpty() ? dvc_serial.cptr() : 0;
						InputArray * p_inp = Rib.AddKeyDownCommand(_k, p_dvc_serial);
						if(p_inp) {
							PPDesktopAssocCmd cp_item;
							for(i = 0; ok < 0 && i < PrivateCp.GetCount(); i++) {
								if(PrivateCp.GetItem(i, cp_item)) {
									ok = ProcessCommandItem(p_inp, cp_item);
								}
							}
							for(i = 0; ok < 0 && i < CommonCp.GetCount(); i++) {
								if(CommonCp.GetItem(i, cp_item)) {
									ok = ProcessCommandItem(p_inp, cp_item);
								}
							}
							if(ok >= 0)
								p_inp->Clear();
						}
					}
				}
			}
		}
	}
	LEAVE_CRITICAL_SECTION
	return ok;
}

/*static*/int PPDesktop::RegWindowClass(HINSTANCE hInst)
{
	WNDCLASSEX wc;
	INITWINAPISTRUCT(wc);
	wc.lpszClassName = SUcSwitch(PPDesktop::WndClsName);
	wc.hInstance     = hInst;
	wc.lpfnWndProc   = PPDesktop::DesktopWndProc;
	wc.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(102));
	wc.hbrBackground = static_cast<HBRUSH>(GetStockObject(LTGRAY_BRUSH));
	wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC | CS_DBLCLKS;
	return ::RegisterClassEx(&wc);
}

/*static*/int PPDesktop::CreateDefault(S_GUID & rNewUuid)
{
	int    ok = -1;
	S_GUID desktop_uuid;
	SString db_symb;
	PPCommandGroup desktop_list;
	CurDict->GetDbSymb(db_symb);
	//@erik v10.6.7 {
	PPCommandMngr * p_mgr = GetCommandMngr(0, cmdgrpcDesktop, 0);
	p_mgr->Load__2(&desktop_list, db_symb, PPCommandMngr::fRWByXml);
	// } @erik
	THROW(p_mgr);
	if(!(desktop_list.Flags & PPCommandItem::fNotUseDefDesktop)) {
		SString def_desk_name;
		(def_desk_name = "def").CatChar('-').Cat(DS.GetTLA().UserName).CatChar('-').Cat("desktop");
		const PPCommandItem * p_item = desktop_list.SearchByName(def_desk_name, db_symb, 0);
		if(p_item && p_item->IsKind(PPCommandItem::kGroup)) {
			desktop_uuid = static_cast<const PPCommandGroup *>(p_item)->Uuid;
		}
		else {
			PPCommandGroup desk(cmdgrpcDesktop, 0, def_desk_name);
			long   id = desktop_list.GetUniqueID();
			desk.SetUniqueID(&id);
			desktop_list.Add(-1, &desk);
			THROW(p_mgr->Save__2(&desktop_list, PPCommandMngr::fRWByXml)); // @erik v10.6.1
			desktop_uuid = desk.Uuid;
		}
		ok = 1;
	}
	CATCHZOK
	ZDELETE(p_mgr);
	rNewUuid = desktop_uuid;
	return ok;
}

/*static*/int PPDesktop::Open(const S_GUID & rDesktopUuid, int createIfZero)
{
	int    ok = cmError;
	int    r = 0;
	PPDesktop * p_v = 0;
	DS.GetTLA().Lc.DesktopUuid_ = rDesktopUuid;
	if(APPL->H_Desktop) {
		DestroyWindow(APPL->H_Desktop);
		APPL->H_Desktop = 0;
	}
	if(!!rDesktopUuid) {
		THROW_MEM(p_v = new PPDesktop());
		r = p_v->Init__(rDesktopUuid);
		if(r > 0) {
			APPL->P_DeskTop->Insert_(p_v);
			ok = p_v->Execute();
		}
	}
	if(r <= 0) {
		ZDELETE(p_v);
		S_GUID temp_uuid = rDesktopUuid;
		r = PPDesktop::CreateDefault(temp_uuid);
		if(r > 0) {
			THROW_MEM(p_v = new PPDesktop());
			DS.GetTLA().Lc.DesktopUuid_ = temp_uuid;
			THROW(p_v->Init__(temp_uuid));
			APPL->P_DeskTop->Insert_(p_v);
			ok = p_v->Execute();
		}
		else
			THROW(r != 0);
	}
	CATCH
		ZDELETE(p_v);
		ok = (PPErrorTooltip(-1, 0), cmError);
	ENDCATCH
	return ok;
}
