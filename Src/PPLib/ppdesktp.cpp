// PPDESKTP.CPP
// Copyright (c) A.Starodub, A.Sobolev 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022, 2023, 2024, 2025, 2026
// @codepage UTF-8
//
#include <pp.h>
#pragma hdrstop

#define max MAX
#define min MIN
#include <gdiplus.h>
using namespace Gdiplus;

const wchar_t * PPDesktop::WndClsName = SlConst::WinClsName_Desktop;
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
				double low = 0.0;
				double upp = 0.0;
				strtorrng(temp_buf, &low, &upp);
				SETMAX(low, 0.0);
				SETMAX(upp, 0.0);
				SExchangeForOrder(&low, &upp);
				rBlk.LenRange.low = static_cast<int>(low);
				rBlk.LenRange.upp = static_cast<int>(low);
			}
		}
		else if(isdec(scan[0])) {
			temp_buf.Z();
			while(isasciialnum(scan[0])) {
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

PPDesktopAssocCmdPool & PPDesktopAssocCmdPool::Z()
{
	DesktopID_Obsolete = -1;
	DesktopUuid.Z();
	L.clear();
	P.Z();
	P.add("$"); // zero index - is empty string
	return *this;
}

void PPDesktopAssocCmdPool::Init(/*PPID desktopId*/const S_GUID & rUuid)
{
	Z();
	DesktopID_Obsolete = 0;
	DesktopUuid = rUuid;
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

int PPDesktopAssocCmdPool::WriteToProp(int use_ta)
{
	int    ok = 1;
	Reference * p_ref(PPRef);
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
			p_strg->Size  = static_cast<uint32>(sz);
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
	Reference * p_ref(PPRef);
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
			DesktopUuid = desktop_uuid;
			ok = 1;
		}
	}
	else {
		DesktopID_Obsolete = desktop_id;
		DesktopUuid = desktop_uuid;
	}
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
		Tbl.CopyBufTo(&rec);
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
				TView::SetWindowProp(hWnd, DWLP_USER, reinterpret_cast<void *>(DLG_TOOLTIP));
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
				p_win->DoCommand(p.setwparam(static_cast<uint32>(lParam)));
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
				HDC    hdc = reinterpret_cast<HDC>(wParam);
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
			//@erik{
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
			SString path;
			SString buf;
			Logotype.Init();
			SFsPath ps(P_ActiveDesktop->GetLogo());
			PPGetPath(PPPATH_BIN, path);
			PPLoadText(PPTXT_DESKIMGDIR, buf);
			path.SetLastSlash().Cat(buf);
			MakeTempFileName(path, "TMP", ps.Ext, buf);
			path = buf;
			copyFileByName(P_ActiveDesktop->GetLogo(), path);
			Logotype.Load(path);
		}
		else
			Logotype.Load(0);
		Advise();
		{
			//
			// Инициализируем список USB-устройств для того, чтобы идентифицировать ввод с серийным номером
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
			DS.GetTLA().Lc.DesktopUuid_.Z();
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
			::DrawTextW(rC, SUcSwitchW(text), text.LenI(), &text_rect, DT_CENTER);
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
	const  long x = cr.left + coord.x;
	const  long y = cr.top + coord.y;
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
					h_icon = LoadIconW(TProgram::GetInst(), MAKEINTRESOURCEW(icon_id));
			}
			else {
				// @v11.2.12 @todo: иногда эта функция работает очень медленно
				h_icon = static_cast<HICON>(::LoadImageW(0, SUcSwitchW(rIcon), IMAGE_ICON, IconSize, IconSize, LR_LOADFROMFILE));
			}
		}
		SETIFZ(h_icon, LoadIconW(TProgram::GetInst(), MAKEINTRESOURCEW(ICON_DEFAULT)));
		if(h_icon) {
			SString text;
			(text = rText).Transf(CTRANSF_INNER_TO_OUTER);
			::SetBkMode(rC, TRANSPARENT);
			::DrawIconEx(rC, cr.left + coord.x + IconSize / 2, cr.top + coord.y + 2, h_icon, 0, 0, 0, 0, DI_DEFAULTSIZE|DI_IMAGE|DI_MASK);
			PPDesktop::DrawText(rC, coord, static_cast<COLORREF>(text_color), text);
			AddTooltip(id, coord, text);
			DestroyIcon(h_icon);
		}
	}
	rC.PopObject();
	rC.PopObject();
}

/*static*/COLORREF PPDesktop::GetDefaultBgColor() { return GetColorRef(SClrSteelblue4); }

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
	const  DWORD style = WS_CHILD|WS_CLIPSIBLINGS|WS_TABSTOP;
	SString title(P_ActiveDesktop->Name);
	HW = ::CreateWindowExW(WS_EX_COMPOSITED, PPDesktop::WndClsName, SUcSwitchW(title.Transf(CTRANSF_INNER_TO_OUTER)), 
		style, 0, 0, r.right - r.left - 18, r.bottom, h_frame, 0, TProgram::GetInst(), this);
	ShowWindow(H(), SW_SHOW);
	UpdateWindow(H());
	HwndTT = ::CreateWindowExW(WS_EX_TOPMOST, TOOLTIPS_CLASSW, NULL, WS_POPUP|TTS_NOPREFIX|TTS_ALWAYSTIP,
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
					{
						PPBizScoreWindow * p_wnd = static_cast<PPBizScoreWindow *>(p_desk->GetServiceView(svcviewBizScore));//p_desk->GetBizScoreWnd();
						ok = p_wnd ? p_wnd->LoadData() : 0;
					}
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
										GetObjectName(ec_pack.Oid, temp_buf);
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

void PPDesktop::WMHCreate()
{
	//
	// @todo @20260111 Перевести управление цветами и шрифтами на централизованную конфигурацию (uid-papyrus.json) //
	//
	HWND   h_wnd = H();
	HDC    hdc = GetDC(h_wnd);
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
		// @todo @20251211 Перевести на определения цветов в js-файле конфигурации.
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
	TView::SetWindowUserData(h_wnd, static_cast<PPDesktop *>(this));
	::SetCursor(::LoadCursorW(0, IDC_ARROW));
	::SetFocus(h_wnd);
	::SendMessageW(h_wnd, WM_NCACTIVATE, TRUE, 0L);
	ReleaseDC(h_wnd, hdc);
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
				P_ActiveDesktop->Type = cmdgrpcDesktop;
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
			SString buf;
			SString msg;
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
						//@erik {
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
						if(SelectCommandGroup(desktop_uuid, 0, 0, cmdgrpcDesktop, false, 0) > 0) // @v12.5.0 (!=0)-->(>0)
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
							PPCommandFolder::Direction direction = PPCommandFolder::nextUp;
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
				SETIFZQ(p_desk->EndModalCmd, cmCancel);
				APPL->DelItemFromMenu(p_desk);
				p_desk->ResetOwnerCurrent();
				APPL->P_DeskTop->RemoveChild(p_desk);
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
				if(false) { // @construction PPDesktop_Use_LayoutFlex 
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
					SString temp_buf;
					SString menu_text;
					if(PPLoadTextAnsi(PPTXT_MENU_DESKTOP, temp_buf)) {
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
				p_desk->DoCommand(p.setwparam(static_cast<uint32>(lParam)));
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
				p_desk->BeginIconMove(coord.setwparam(static_cast<uint32>(lParam)));
			}
			return 0;
		case WM_LBUTTONUP:
			if(p_desk) {
				SPoint2S coord;
				p_desk->EndIconMove(coord.setwparam(static_cast<uint32>(lParam)));
				ClipCursor(0);
			}
			return 0;
		case WM_MOUSEMOVE:
			if(wParam == MK_LBUTTON && p_desk) {
				SPoint2S coord;
				p_desk->MoveIcon(coord.setwparam(static_cast<uint32>(lParam)));
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
					::InvalidateRect(hWnd, 0, TRUE);
					::UpdateWindow(hWnd);
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
				TView::messageKeyDown(p_desk, static_cast<uint>(wParam));
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
	WNDCLASSEXW wc;
	INITWINAPISTRUCT(wc);
	wc.lpszClassName = PPDesktop::WndClsName;
	wc.hInstance     = hInst;
	wc.lpfnWndProc   = PPDesktop::DesktopWndProc;
	wc.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(102));
	wc.hbrBackground = static_cast<HBRUSH>(GetStockObject(LTGRAY_BRUSH));
	wc.style = CS_HREDRAW|CS_VREDRAW|CS_OWNDC|CS_DBLCLKS;
	return ::RegisterClassExW(&wc);
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

/*static*/int PPDesktop::EditAssocCmdList(/*long desktopID*/const S_GUID & rDesktopUuid)
{
	class DesktopAssocCmdsDialog : public PPListDialog {
		DECL_DIALOG_DATA(PPDesktopAssocCmdPool);
	public:
		DesktopAssocCmdsDialog() : PPListDialog(DLG_DESKCMDA, CTL_DESKCMDA_LIST)
		{
			PPCommandDescr cmd_descr;
			cmd_descr.GetResourceList(1, CmdList);
			disableCtrl(CTLSEL_DESKCMDA_DESKTOP, true);
		}
		DECL_DIALOG_SETDTS()
		{
			StrAssocArray list;
			if(!RVALUEPTR(Data, pData)) {
				Data.Init(ZEROGUID);
			}
			PPCommandFolder::GetCommandGroupList(0, cmdgrpcDesktop, DesktopList);
			DesktopList.GetStrAssocList(list);
			long  surr_id = DesktopList.GetSurrIdByUuid(Data.GetDesktopUuid());
			SetupStrAssocCombo(this, CTLSEL_DESKCMDA_DESKTOP, list, /*Data.GetDesktopID()*/surr_id, 0, 0);
			updateList(-1);
			return 1;
		}
		DECL_DIALOG_GETDTS()
		{
			const  PPID surr_id = getCtrlLong(CTLSEL_DESKCMDA_DESKTOP);
			S_GUID uuid = DesktopList.GetUuidBySurrId(surr_id);
			Data.SetDesktopUuid(uuid);
			ASSIGN_PTR(pData, Data);
			return 1;
		}
	private:
		virtual int setupList()
		{
			int    ok = 1;
			PPDesktopAssocCmd assci;
			StringSet ss(SLBColumnDelim);
			for(uint i = 0; ok && i < Data.GetCount(); i++) {
				if(Data.GetItem(i, assci)) {
					uint pos = 0;
					if(CmdList.Search(assci.CmdID, &pos) > 0) {
						ss.Z();
						ss.add(assci.Code);
						ss.add(CmdList.Get(pos).Txt);
						if(!addStringToList(i+1, ss.getBuf()))
							ok = 0;
					}
				}
			}
			return ok;
		}
		virtual int addItem(long * pPos, long * pID)
		{
			int    ok = -1;
			uint   pos = 0;
			PPDesktopAssocCmd cmd_assc;
			if(EditCommandAssoc(-1, &cmd_assc, &Data) > 0) {
				ok = Data.AddItem(&cmd_assc);
				if(ok > 0) {
					pos = Data.GetCount()-1;
					ASSIGN_PTR(pPos, pos);
					ASSIGN_PTR(pID, pos+1);
				}
			}
			return ok;
		}
		virtual int editItem(long pos, long id)
		{
			int    ok = -1;
			PPDesktopAssocCmd cmd_assc;
			if(Data.GetItem(pos, cmd_assc)) {
				ok = EditCommandAssoc(pos, &cmd_assc, &Data);
				if(ok > 0) {
					Data.SetItem(pos, &cmd_assc);
				}
			}
			return ok;
		}
		virtual int delItem(long pos, long id)
		{
			return (pos >= 0 && pos < (long)Data.GetCount()) ? Data.SetItem(pos, 0) : -1;
		}
		int    EditCommandAssoc(long pos, PPDesktopAssocCmd * pAsscCmd, PPDesktopAssocCmdPool * pCmdList)
		{
			class DesktopAssocCommandDialog : public TDialog {
				DECL_DIALOG_DATA(PPDesktopAssocCmd);
			public:
				DesktopAssocCommandDialog(long pos, PPDesktopAssocCmdPool * pCmdList) : TDialog(DLG_DESKCMDAI), Pos(pos), P_CmdList(pCmdList)
				{
				}
				DECL_DIALOG_SETDTS()
				{
					StrAssocArray cmd_list;
					PPCommandDescr cmd_descr;
					if(!RVALUEPTR(Data, pData))
						Data.Z();
					cmd_descr.GetResourceList(1, cmd_list);
					cmd_list.SortByText();
					setCtrlString(CTL_DESKCMDAI_CODE, Data.Code);
					setCtrlString(CTL_DESKCMDAI_DVCSERIAL, Data.DvcSerial);
					setCtrlString(CTL_DESKCMDAI_PARAM, Data.CmdParam);
					SetupStrAssocCombo(this, CTLSEL_DESKCMDAI_COMMAND, cmd_list, Data.CmdID, 0, 0);
					AddClusterAssoc(CTL_DESKCMDAI_FLAGS, 0, PPDesktopAssocCmd::fSpecCode);
					AddClusterAssoc(CTL_DESKCMDAI_FLAGS, 1, PPDesktopAssocCmd::fSpecCodePrefx);
					AddClusterAssoc(CTL_DESKCMDAI_FLAGS, 2, PPDesktopAssocCmd::fNonInteractive);
					SetClusterData(CTL_DESKCMDAI_FLAGS, Data.Flags);
					SetupCtrls();
					return 1;
				}
				DECL_DIALOG_GETDTS()
				{
					int    ok = 1;
					uint   sel = 0;
					uint   pos = 0;
					SString buf;
					GetClusterData(CTL_DESKCMDAI_FLAGS, &Data.Flags);
					sel = CTL_DESKCMDAI_COMMAND;
					getCtrlData(CTLSEL_DESKCMDAI_COMMAND, &Data.CmdID);
					THROW_PP(Data.CmdID, PPERR_INVCOMMAND);
					getCtrlString(sel = CTL_DESKCMDAI_CODE, Data.Code);
					THROW_PP(Data.Code.Len(), PPERR_INVSPECCODE);
					getCtrlString(CTL_DESKCMDAI_DVCSERIAL, Data.DvcSerial);
					getCtrlString(CTL_DESKCMDAI_PARAM, Data.CmdParam);
					ASSIGN_PTR(pData, Data);
					CATCH
						sel = (sel == CTL_DESKCMDAI_CODE && !(Data.Flags & PPDesktopAssocCmd::fSpecCode)) ? CTL_DESKCMDAI_COMMAND : sel;
						ok = (selectCtrl(sel), 0);
					ENDCATCH
					return ok;
				}
			private:
				DECL_HANDLE_EVENT
				{
					TDialog::handleEvent(event);
					if(event.isCmd(cmClusterClk) && event.isCtlEvent(CTL_DESKCMDAI_FLAGS)) {
						SetupCtrls();
					}
					else if(event.isCmd(cmWinKeyDown)) {
						long    flags = 0;
						GetClusterData(CTL_DESKCMDAI_FLAGS, &flags);
						if(!(flags & PPDesktopAssocCmd::fSpecCode)) {
							SString buf;
							const KeyDownCommand * p_cmd = static_cast<const KeyDownCommand *>(event.message.infoPtr);
							if(p_cmd && p_cmd->GetKeyName(buf, 1) > 0)
								p_cmd->GetKeyName(buf);
							setCtrlString(CTL_DESKCMDAI_CODE, buf);
						}
					}
					else
						return;
					clearEvent(event);
				}
				void SetupCtrls()
				{
					const long flags = GetClusterData(CTL_DESKCMDAI_FLAGS);
					disableCtrl(CTL_DESKCMDAI_CODE, !(flags & PPDesktopAssocCmd::fSpecCode));
					DisableClusterItem(CTL_DESKCMDAI_FLAGS, 1, !(flags & PPDesktopAssocCmd::fSpecCode));
				}
				const long Pos;
				const PPDesktopAssocCmdPool * P_CmdList;
			};
			DIALOG_PROC_BODY_P2ERR(DesktopAssocCommandDialog, pos, pCmdList, pAsscCmd)
		}
		StrAssocArray CmdList;
		PPCommandFolder::CommandGroupList DesktopList;
	};
	int    ok = -1;
	PPDesktopAssocCmdPool list;
	DesktopAssocCmdsDialog * p_dlg = 0;
	THROW(list.ReadFromProp(/*desktopID*/rDesktopUuid));
	THROW(CheckDialogPtr(&(p_dlg = new DesktopAssocCmdsDialog())));
	p_dlg->setDTS(&list);
	for(int valid_data = 0; !valid_data && ExecView(p_dlg) == cmOK;) {
		if(p_dlg->getDTS(&list) > 0) {
			THROW(list.WriteToProp(1));
			valid_data = ok = 1;
		}
	}
	CATCHZOKPPERR
	delete p_dlg;
	return ok;
}
//
// Centrigo
// 
//
// Descr: Список навигации для проекта Centrigo
//
class CentrigoNavBlock { // @v12.5.6 @centrigo
public:
	enum {
		ccatUndef      = 0,
		ccatCommand    = 1,
		ccatNotes      = 2,
		ccatActualToDo = 3,
		ccatWallet     = 4,
	};
	struct Entry {
		Entry() : Category(ccatUndef), LastAccsDtm(ZERODATETIME)
		{
		}
		int    Id; // Локальный ид для сопоставления с визуальным отображением //
		int    Category; // ccatXXX
		SObjID Oid; // Для команд (Category==ccatCommand) Oid.Obj==PPOBJ_UXCMD. Родительский узел для команд имеет Oid=={PPOBJ_UXCMD; 0}
		LDATETIME LastAccsDtm;
		SString Title;
		TSCollection <Entry> Children;
	};
	CentrigoNavBlock() : LastId(0)
	{
	}
	~CentrigoNavBlock()
	{
	}
	Entry * SearchEntry(int id)
	{
		return Helper_SearchEntry(L, id);
	}
	int    AddEntry(int parentId, int category)
	{
		SObjID zero_oid;
		LDATETIME zero_dtm = ZERODATETIME;
		return Helper_AddEntry(parentId, category, zero_oid, zero_dtm, 0);
	}
	int    AddEntry(int parentId, int category, const SObjID & rOid, const char * pTitle)
	{
		LDATETIME zero_dtm = ZERODATETIME;
		return Helper_AddEntry(parentId, category, rOid, zero_dtm, pTitle);
	}
	int    MakeStrAssocArray(StrAssocArray & rResult)
	{
		rResult.Z();
		return Helper_MakeStrAssocArray(L, 0, rResult);
	}
private:
	int    Helper_MakeStrAssocArray(const TSCollection <Entry> & rL, long parentId, StrAssocArray & rResult)
	{
		int    ok = 1;
		SString temp_buf;
		for(uint i = 0; i < rL.getCount(); i++) {
			const Entry * p_entry = rL.at(i);
			if(p_entry) {
				if(p_entry->Title.NotEmpty())
					temp_buf = p_entry->Title;
				else {
					temp_buf.Z().CatChar('#').Cat(p_entry->Id);
				}
				rResult.Add(p_entry->Id, parentId, temp_buf);
				if(p_entry->Children.getCount()) {
					Helper_MakeStrAssocArray(p_entry->Children, p_entry->Id, rResult); // @recursion
				}
			}
		}
		return ok;
	}
	Entry * Helper_SearchEntry(TSCollection <Entry> & rL, int id)
	{
		Entry * p_result = 0;
		if(id) {
			for(uint i = 0; !p_result && i < rL.getCount(); i++) {
				Entry * p_entry = rL.at(i);
				p_result = (p_entry->Id == id) ? p_entry : Helper_SearchEntry(p_entry->Children, id); // @recursion
			}
		}
		return p_result;
	}
	//
	// Descr: Реализуе создание нового элемента в иерархии навигации.
	// Returns:
	//   0 - error
	//  >0 - внутрений идентификатор созданного элемента
	//
	int    Helper_AddEntry(int parentId, int category, const SObjID & rOid, const LDATETIME & rLastAccsDtm, const char * pTitle)
	{
		int    result = 0;
		Entry * p_parent_entry = SearchEntry(parentId);
		TSCollection <Entry> & r_list = p_parent_entry ? p_parent_entry->Children : L;
		Entry * p_new_entry = r_list.CreateNewItem();
		if(p_new_entry) {
			p_new_entry->Id = ++LastId;
			p_new_entry->Category = category;
			p_new_entry->Oid = rOid;
			p_new_entry->LastAccsDtm = rLastAccsDtm;
			p_new_entry->Title = pTitle;
			result = p_new_entry->Id;
		}
		return result;
	}
	int    LastId;
	TSCollection <Entry> L;
};
//
//
//
class LocalStateBinderySelExtra : public WordSel_ExtraBlock {
public:
	explicit LocalStateBinderySelExtra(const LocalStateBinderyCore::StateIdent & rIdent);
	virtual StrAssocArray * GetList(const char * pText);
	virtual StrAssocArray * GetRecentList();
	virtual int Search(long id, SString & rBuf);
	virtual int SearchText(const char * pText, long * pID, SString & rBuf);
	virtual void OnAcceptInput(const char * pText, long id);
private:
	const LocalStateBinderyCore::StateIdent StI;
};

LocalStateBinderySelExtra::LocalStateBinderySelExtra(const LocalStateBinderyCore::StateIdent & rIdent) : WordSel_ExtraBlock(), StI(rIdent)
{
}

/*virtual*/StrAssocArray * LocalStateBinderySelExtra::GetList(const char * pText)
{
	StrAssocArray * p_result = 0;
	if(!isempty(pText)) {
		LocalStateBinderyCore * p_lstb = DS.GetTLA().GetLocalStateBindery();
		if(p_lstb) {
			SString temp_buf;
			SString key(pText);
			key.Transf(CTRANSF_INNER_TO_UTF8);
			TSCollection <LocalStateBinderyCore::SerialEntry> serial_list;
			LongArray pos_list; // [+1]
			p_lstb->FetchStateSerial(StI, &serial_list);
			LocalStateBinderyCore::SearchInSerial(serial_list, key, LocalStateBinderyCore::treatsSubStringUtf8List, pos_list);
			if(pos_list.getCount()) {
				for(uint i = 0; i < pos_list.getCount(); i++) {
					const  long iter_idx = pos_list.get(i);
					const  uint iter_pos = static_cast<uint>(iter_idx-1);
					const  LocalStateBinderyCore::SerialEntry * p_entry = serial_list.at(iter_pos);
					if(p_entry && p_entry->Buf.GetAvailableSize()) {
						if(!p_result) {
							p_result = new StrAssocArray();
						}
						LocalStateBinderyCore::GetStringFromStateBuf(LocalStateBinderyCore::treatbStringUtf8, p_entry->Buf, temp_buf);
						if(temp_buf.NotEmpty()) {
							if(!p_result->SearchByTextNcUtf8(temp_buf, 0))
								p_result->AddFast(p_entry->ID, temp_buf);
						}
					}
				}
			}
		}
	}
	return p_result;
}

/*virtual*/StrAssocArray * LocalStateBinderySelExtra::GetRecentList()
{
	StrAssocArray * p_result = 0;
	return p_result;
}

/*virtual*/int LocalStateBinderySelExtra::Search(long id, SString & rBuf)
{
	rBuf.Z();
	int    ok = 0;
	if(id) {
		LocalStateBinderyCore * p_lstb = DS.GetTLA().GetLocalStateBindery();
		if(p_lstb) {
			TSCollection <LocalStateBinderyCore::SerialEntry> serial_list;
			p_lstb->FetchStateSerial(StI, &serial_list);
			uint   pos = 0;
			if(serial_list.lsearch(&id, &pos, CMPF_LONG)) {
				const  LocalStateBinderyCore::SerialEntry * p_entry = serial_list.at(pos);
				if(p_entry) {
					LocalStateBinderyCore::GetStringFromStateBuf(LocalStateBinderyCore::treatbStringUtf8, p_entry->Buf, rBuf);
					ok = 1;
				}
			}
		}
	}
	return ok;
}

/*virtual*/int LocalStateBinderySelExtra::SearchText(const char * pText, long * pID, SString & rBuf)
{
	rBuf.Z();
	int    ok = 0;
	long   result_id = 0;
	if(!isempty(pText)) {
		LocalStateBinderyCore * p_lstb = DS.GetTLA().GetLocalStateBindery();
		if(p_lstb) {
			SString key(pText);
			TSCollection <LocalStateBinderyCore::SerialEntry> serial_list;
			LongArray pos_list; // [+1]
			p_lstb->FetchStateSerial(StI, &serial_list);
			LocalStateBinderyCore::SearchInSerial(serial_list, key, LocalStateBinderyCore::treatsStringUtf8List, pos_list);
			if(pos_list.getCount()) {
				const  long _idx = pos_list.get(0);
				const  uint _pos = static_cast<uint>(_idx-1);
				const  LocalStateBinderyCore::SerialEntry * p_entry = serial_list.at(_pos);
				if(p_entry && p_entry->Buf.GetAvailableSize()) {
					LocalStateBinderyCore::GetStringFromStateBuf(LocalStateBinderyCore::treatbStringUtf8, p_entry->Buf, rBuf);
					if(rBuf.NotEmpty()) {
						result_id = p_entry->ID;
						ok = 1;
					}
				}
			}
		}
	}
	ASSIGN_PTR(pID, result_id);
	return ok;
}

/*virtual*/void LocalStateBinderySelExtra::OnAcceptInput(const char * pText, long id)
{
	int    lstb_reg_state_result = 0;
	if(!isempty(pText)) {
		SString temp_buf(pText);
		if(temp_buf.NotEmptyS()) {
			LocalStateBinderyCore * p_lstb = DS.GetTLA().GetLocalStateBindery();
			if(p_lstb) {
				temp_buf.Transf(CTRANSF_INNER_TO_UTF8);
				//LocalStateBinderyCore::StateIdent state_ident;
				//state_ident.Kind = LocalStateBinderyCore::kInput;
				//state_ident.Subj = UED::SetRaw_UXControlIdent(SObjID(WNDID_FACADEWINDOW, CTL_FACADEWINDOW_MAININPUT));
				//if(state_ident.Subj) {
				{
					PPID   sid = 0;
					SBuffer state_input_data;
					state_input_data.Write(temp_buf.cptr(), temp_buf.Len()+1);
					lstb_reg_state_result = p_lstb->RegisterState(&sid, StI, state_input_data, 1);
				}
			}
		}
	}
}

class TFacadeWindow : public TBaseBrowserWindow {
public:
	static constexpr int ViewId_Primary = 10001;

	TFacadeWindow();
	~TFacadeWindow();
private:
	DECL_HANDLE_EVENT;

	static const wchar_t * WndClsName;
	static int   RegWindowClass(HINSTANCE hInst);
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	int    WMHCreate();
	void   InitLayout();
	int    InsertWorkWindow(int ppviewId, const PPBaseFilt * pFilt); // @debug
	int    MakeNavList(CentrigoNavBlock & rBlk);
	int    DoNote(SObjID & rOid);
	int    DoContacts(const PersonFilt * pFilt);
	int    DoTasks(const PrjTaskFilt * pFilt);
	int    DoSecrets();
	int    HandleInputEnter(const SString & rInput);
	int    RemoveWorkingPanel();
	int    DrawNavTreeItem(void * pCustomDrawDescriptor);
	int    GetSecretsFilePath(SString & rBuf);
	int    LoadSecrets(SString * pFilePath, bool interactive);
	int    CloseSecrets();

	CentrigoNavBlock NavBlk;
	//
	PPObjWorkbook WbObj;
	SUiLayout * P_Lo_NavItem; // really const
	HWND  H_RecentFocusedChild; // @v12.6.0
	PPSecretSegmentPool SecPool; // @v12.6.9
};

int TFacadeWindow::MakeNavList(CentrigoNavBlock & rBlk)
{
	int    ok = 1;
	SString temp_buf;
	LongArray parent_list;
	{
		int    parent_id = 0;
		PPLoadStringUtf8("cmd_pl", temp_buf);
		parent_id = rBlk.AddEntry(0, CentrigoNavBlock::ccatCommand, SObjID(PPOBJ_UXCMD, 0), temp_buf);
		if(parent_id) {
			// @attention номера команд сейчас фиктивные только для отладки списка - потом установить правильные значения//
			static const SIntToSymbTabEntry cmd_list[] = {
				{ cmCentrigoNotes, "centrigo_cmd_notes" },
				{ cmCentrigoContacts, "centrigo_cmd_contacts" },
				{ cmCentrigoToDo, "centrigo_cmd_todo" },
				{ cmCentrigoWallet, "centrigo_cmd_wallet" },
				{ cmCentrigoSecrets, "centrigo_cmd_secrets" },
			};
			for(uint i = 0; i < SIZEOFARRAY(cmd_list); i++) {
				PPLoadStringUtf8(cmd_list[i].P_Symb, temp_buf);
				rBlk.AddEntry(parent_id, CentrigoNavBlock::ccatCommand, SObjID(PPOBJ_UXCMD, cmd_list[i].Id), temp_buf);
			}
			parent_list.add(parent_id);
		}
	}
	{
		struct IterNoteItem {
			PPID   ID;
			LDATETIME Dtm;
			char   Name[256];
		};
		SArray note_list(sizeof(IterNoteItem));
		WorkbookTbl::Rec rec;
		for(SEnum en = WbObj.P_Tbl->EnumByType(PPWBTYP_NOTE, 0); en.Next(&rec) > 0;) {
			IterNoteItem new_item;
			new_item.ID = rec.ID;
			new_item.Dtm.Set(rec.Dt, rec.Tm);
			(temp_buf = rec.Name).Transf(CTRANSF_INNER_TO_UTF8);
			STRNSCPY(new_item.Name, temp_buf);
			note_list.insert(&new_item);
		}
		if(note_list.getCount()) {
			CompFunc cf = [](const void * p1, const void * p2, void * pExtraData) -> int
			{
				const IterNoteItem * p_i1 = static_cast<const IterNoteItem *>(p1);
				const IterNoteItem * p_i2 = static_cast<const IterNoteItem *>(p2);
				int   ret = 0;
				if(p_i1->Dtm < p_i2->Dtm)
					ret = +1; // descending
				else if(p_i1->Dtm > p_i2->Dtm)
					ret = -1;
				else {
					ret = strcmp(p_i1->Name, p_i2->Name); // @todo Здесь должно быть сравнение без учета регистров в формате utf8
				}
				return ret;
			};

			note_list.sort2(cf);
			int    parent_id = 0;
			PPLoadStringUtf8("centrigo_cmd_notes", temp_buf);
			parent_id = rBlk.AddEntry(0, CentrigoNavBlock::ccatNotes, SObjID(PPOBJ_WORKBOOK, 0), temp_buf);
			if(parent_id) {
				for(uint i = 0; i < note_list.getCount(); i++) {
					const IterNoteItem * p_item = static_cast<const IterNoteItem *>(note_list.at(i));
					rBlk.AddEntry(parent_id, CentrigoNavBlock::ccatNotes, SObjID(PPOBJ_WORKBOOK, p_item->ID), p_item->Name);
				}
				parent_list.add(parent_id);
			}
		}
	}
	{
		SmartListBox * p_lb = static_cast<SmartListBox *>(getCtrlView(CTL_FACADEWINDOW_NAVPANE));
		if(p_lb) {
			StrAssocArray * p_list = new StrAssocArray();
			if(p_list) {
				rBlk.MakeStrAssocArray(*p_list);
				StdTreeListBoxDef * p_lb_def = new StdTreeListBoxDef(p_list, lbtDisposeData|lbtDblClkNotify|lbtTextUtf8, 0);
				if(p_lb_def) {
					p_lb->setDef(p_lb_def);
					p_lb->SetExpandedTreeBranchList(&parent_list);
					if(P_Lo_NavItem) {
						float  height = 0.0f;
						const  int gfhr = P_Lo_NavItem->GetFullHeight(&height);
						if(gfhr) {
							HWND h_wnd_list = p_lb->getHandle();
							if(h_wnd_list) {
								::SendMessageW(h_wnd_list, TVM_SETITEMHEIGHT, static_cast<int>(height), 0);
							}
						}
					}
					p_lb->Draw_();
				}
			}
		}
	}
	return ok;
}

void TFacadeWindow::InitLayout()
{
	if(true) {
		SUiLayout * p_lo_main = new SUiLayout();
		SUiLayoutParam alb;
		p_lo_main->SetLayoutBlock(alb);
		p_lo_main->SetCallbacks(0, TWindowBase::SetupLayoutItemFrame, this);
		{
			// Ориентация горизонтальная потому что справа фиксированного размера полоса изменения размера, а слева - список, занимающий все оставшееся место
			SUiLayoutParam alb_left(DIREC_HORZ, 0, SUiLayoutParam::alignStretch);
			alb_left.GravityX = SIDE_LEFT;
			alb_left.GravityY = SIDE_CENTER;
			alb_left.SetFixedSizeX(128.0f);
			alb_left.SetVariableSizeY(SUiLayoutParam::szByContainer, 1.0f);
			SUiLayout * p_lo = new SUiLayout(alb_left);
			p_lo->SetSymb("Facade_Left");
			p_lo_main->Insert(p_lo);
		}
		{
			SUiLayoutParam alb_ul(DIREC_HORZ, 0, SUiLayoutParam::alignStretch);
			alb_ul.GravityX = SIDE_LEFT;
			alb_ul.GravityY = SIDE_TOP;
			alb_ul.SetFixedSizeX(128.0f);
			alb_ul.SetFixedSizeY(64.0f);
			SUiLayout * p_lo = new SUiLayout(alb_ul);
			p_lo->SetSymb("Facade_UpperLeft");
			p_lo_main->Insert(p_lo);
		}
		{
			SUiLayoutParam alb_top(DIREC_VERT, 0, SUiLayoutParam::alignStretch);
			alb_top.GravityX = SIDE_CENTER;
			alb_top.GravityY = SIDE_TOP;
			alb_top.JustifyContent = SUiLayoutParam::alignCenter;
			alb_top.AlignItems = SUiLayoutParam::alignCenter;
			alb_top.SetVariableSizeX(SUiLayoutParam::szByContainer, 1.0f);
			alb_top.SetFixedSizeY(96.0f);
			SUiLayout * p_lo = new SUiLayout(alb_top);
			p_lo->SetSymb("Facade_Top");
			p_lo_main->Insert(p_lo);
		}
		{
			SUiLayoutParam alb_center(DIREC_HORZ, 0, SUiLayoutParam::alignStretch);
			alb_center.GravityX = SIDE_CENTER;
			alb_center.GravityY = SIDE_CENTER;
			alb_center.SetVariableSizeX(SUiLayoutParam::szByContainer, 1.0f);
			alb_center.SetVariableSizeY(SUiLayoutParam::szByContainer, 1.0f);
			SUiLayout * p_lo = new SUiLayout(alb_center);
			p_lo->SetSymb("Facade_Center");
			p_lo_main->Insert(p_lo);
		}
		SetLayout(p_lo_main);
	}
}

int TFacadeWindow::DoContacts(const PersonFilt * pFilt)
{
	int    ok = -1;
	ok = InsertWorkWindow(PPVIEW_PERSON, pFilt);
	return ok;
}

int TFacadeWindow::DoTasks(const PrjTaskFilt * pFilt)
{
	int    ok = -1;
	ok = InsertWorkWindow(PPVIEW_PRJTASK, pFilt);
	return ok;
}

int TFacadeWindow::DoNote(SObjID & rOid)
{
	int    ok = -1;
	const  LDATETIME now_dtm = getcurdatetime_();
	SString temp_buf;
	if(rOid.Obj == PPOBJ_WORKBOOK) {
		TBaseBrowserWindow * p_brw = 0;
		PPWorkbookPacket pack;
		if(!rOid.Id) {
			PPID   new_id = 0;
			PPWorkbookPacket new_pack;
			PPTransaction tra(1);
			THROW(tra);
			new_pack.Rec.Type = PPWBTYP_NOTE;
			new_pack.Rec.Dt = now_dtm.d;
			new_pack.Rec.Tm = now_dtm.t;
			THROW(WbObj.MakeUniqueName(temp_buf, 0, 0/*use_ta*/));
			STRNSCPY(new_pack.Rec.Name, temp_buf);
			THROW(WbObj.MakeUniqueCode(temp_buf, 0/*use_ta*/));
			STRNSCPY(new_pack.Rec.Symb, temp_buf);
			THROW(WbObj.PutPacket(&new_id, &new_pack, 0/*use_ta*/));
			THROW(tra.Commit());
			rOid.Id = new_id;
		}
		{
			WorkbookTbl::Rec rec;
			THROW(WbObj.Search(rOid.Id, &rec) > 0);
		}
		RemoveWorkingPanel();
		{
			SUiLayout * p_lo = P_Lfc->FindBySymb("Facade_Center");
			if(p_lo) {
				// @construction
				SObjTextRefIdent tri(rOid, PPTRPROP_MEMO);
				STextBrowser * p_tb = new STextBrowser(tri, /*pLexerSymb*/0, /*toolbarId*/-1);
				if(p_tb) {
					STextBrowser::Config cfg;
					p_tb->GetConfig(cfg);
					//fAutoBackupText  = 0x0001,
					//fAutoBackupState = 0x0002,
					//fAutoSaveText    = 0x0004,
					//fAutoSaveState   = 0x0008,
					cfg.Flags |= (STextBrowser::Config::fAutoSaveText|STextBrowser::Config::fAutoSaveState);
					//cfg.Flags |= (STextBrowser::Config::fAutoBackupText|STextBrowser::Config::fAutoBackupState);
					p_tb->SetConfig(cfg);
					p_tb->SetIdlePeriod(3);
					p_brw = p_tb;
				}
				if(p_brw) {
					InsertCtlWithCorrespondingNativeItem(p_brw, ViewId_Primary, 0, /*extraPtr*/0);
					{
						SUiLayoutParam alb_;
						alb_.GrowFactor = 1.0;
						alb_.SetVariableSizeY(SUiLayoutParam::szByContainer, 1.0f);
						{
							SUiLayout * p_result = 0;
							p_result = p_lo->InsertItem(p_brw, &alb_);
							if(p_result) {
								p_result->SetCallbacks(0, TView::SetupLayoutItemFrameProc, p_brw);
								p_brw->Launch_(this);
								{
									EvaluateLayout(getClientRect());
									invalidateAll(true);
									::UpdateWindow(H());
								}
								::PostMessageW(p_brw->H(), WM_SETFOCUS, 0, 0);
							}
						}
					}
				}
			}
		}
	}
	CATCH
		ok = PPErrorZ();
	ENDCATCH
	return ok;
}

int TFacadeWindow::InsertWorkWindow(int ppviewId, const PPBaseFilt * pFilt)
{
	int    ok = -1;
	if(ppviewId) {
		SUiLayout * p_lo = P_Lfc->FindBySymb("Facade_Center");
		if(p_lo) {
			PPView * p_view = 0;
			RemoveWorkingPanel();
			if(PPView::Execute(ppviewId, pFilt, PPView::exefModeless|PPView::exefDontLaunchWindow, &p_view, 0)) { // @v12.5.4
				SUiLayoutParam alb_;
				alb_.GrowFactor = 1.0;
				alb_.SetVariableSizeY(SUiLayoutParam::szByContainer, 1.0f);
				//alb_.SetMargin(8.0f);
				//p_view->Browse(true/*modeless*/);
				p_view->BrowseInLayout(this, "Facade_Center", alb_, /*10002*/ViewId_Primary); // @v12.5.4
				ok = 1;
			}
		}
	}
	return ok;
}

int TFacadeWindow::WMHCreate()
{
	SPaintToolBox * p_tb = APPL->GetUiToolBox();
	InitLayout();
	if(P_Lfc) {
		{
			SUiLayout * p_lo_top = P_Lfc->FindBySymb("Facade_Top");
			SUiLayout * p_lo_left = P_Lfc->FindBySymb("Facade_Left");
			if(p_lo_top) {
				const float input_fixed_y = SlConst::UiFixedInputY * 2.5f;
				TInputLine * p_il = 0;
				TInputLine * p_il_info = 0;
				{
					p_il = new TInputLine(TRect::_defr_, TInputLine::spcfSendReturnToOwner/*spcFlags*/, MKSTYPE(S_ZSTRING, 1024), MKSFMT(1024, 0));
					InsertCtlWithCorrespondingNativeItem(p_il, CTL_FACADEWINDOW_MAININPUT, 0, /*extraPtr*/0);
					{
						SUiLayoutParam alb_;
						alb_.SetVariableSizeX(SUiLayoutParam::szByContainer, 1.0f);
						alb_.SetFixedSizeY(input_fixed_y);
						alb_.SetMargin(FRect(8.0f, 8.0f, 8.0f, 2.0f));
						{
							//
							// Если pView == 0, то мы вставляем лейаут без привязки к конкретному control'у. Обычно это
							// делается для вспомогательных разметочных лейаутов-контейнеров
							//
							SUiLayout * p_result = 0;
							p_result = p_lo_top->InsertItem(p_il, &alb_);
							if(p_result && p_il) {
								p_result->SetCallbacks(0, TView::SetupLayoutItemFrameProc, p_il);
								p_il->handleWindowsMessage(WM_INITDIALOG, 0, 0);
							}
						}
					}
					{
						LocalStateBinderyCore::StateIdent state_ident;
						state_ident.Kind = LocalStateBinderyCore::kInput;
						state_ident.Subj = UED::SetRaw_UXControlIdent(SObjID(WNDID_FACADEWINDOW, CTL_FACADEWINDOW_MAININPUT));
						SetupWordSelector(CTL_FACADEWINDOW_MAININPUT, new LocalStateBinderySelExtra(state_ident), 0, 1, 
							WordSel_ExtraBlock::fUtf8|WordSel_ExtraBlock::fFreeText/*|WordSel_ExtraBlock::fAlwaysSearchBySubStr*/);
					}
				}
				{
					p_il_info = new TInputLine(TRect::_defr_, TInputLine::spcfReadOnly, MKSTYPE(S_ZSTRING, 1024), MKSFMT(1024, 0));
					InsertCtlWithCorrespondingNativeItem(p_il_info, CTL_FACADEWINDOW_INFOLINE, 0, /*extraPtr*/0);
					{
						SUiLayoutParam alb_;
						alb_.SetVariableSizeX(SUiLayoutParam::szByContainer, 1.0f);
						alb_.SetFixedSizeY(input_fixed_y);
						alb_.SetMargin(FRect(8.0f, 2.0f, 8.0f, 8.0f));
						{
							//
							// Если pView == 0, то мы вставляем лейаут без привязки к конкретному control'у. Обычно это
							// делается для вспомогательных разметочных лейаутов-контейнеров
							//
							SUiLayout * p_result = 0;
							p_result = p_lo_top->InsertItem(p_il_info, &alb_);
							if(p_result && p_il_info) {
								p_result->SetCallbacks(0, TView::SetupLayoutItemFrameProc, p_il_info);
								p_il_info->handleWindowsMessage(WM_INITDIALOG, 0, 0);
							}
						}
					}
				}
				{
					SPaintObj::Font * p_f = p_tb ? p_tb->GetFont(SDrawContext(static_cast<HDC>(0)), TProgram::tbiAccentInputFont) : 0;
					if(p_f) {
						HFONT f = static_cast<HFONT>(*p_f);
						if(p_il) {
							HWND local_hw = p_il->getHandle();
							::SendMessageW(local_hw, WM_SETFONT, reinterpret_cast<WPARAM>(f), TRUE);
						}
						if(p_il_info) {
							HWND local_hw = p_il_info->getHandle();
							::SendMessageW(local_hw, WM_SETFONT, reinterpret_cast<WPARAM>(f), TRUE);
						}
					}
				}
				//
				//InsertWorkWindow(PPVIEW_GOODSREST);

			}
			if(p_lo_left) {
				{
					StdTreeListBoxDef * p_lb_def = new StdTreeListBoxDef(0, lbtDisposeData|lbtDblClkNotify/*|lbtOwnerDraw*/, 0);
					SmartListBox * p_lb = new SmartListBox(TRect::_defr_, p_lb_def, true/*is_tree*/);
					//p_lb->SetOwnerDrawState();
					/*if(p_lb->P_Def) {
						p_lb->P_Def->addItem(1, "The first tree-list item!");
					}*/
					InsertCtlWithCorrespondingNativeItem(p_lb, CTL_FACADEWINDOW_NAVPANE, 0, 0);
					{
						SUiLayoutParam alb_;
						alb_.SetGrowFactor(1.0f); 
						//alb_.SetVariableSizeX(SUiLayoutParam::szByContainer, 1.0f);
						alb_.SetVariableSizeY(SUiLayoutParam::szByContainer, 1.0f);
						//alb_.SetMargin(FRect(8.0f, 2.0f, 8.0f, 8.0f));
						{
							//
							// Если pView == 0, то мы вставляем лейаут без привязки к конкретному control'у. Обычно это
							// делается для вспомогательных разметочных лейаутов-контейнеров
							//
							SUiLayout * p_result = 0;
							p_result = p_lo_left->InsertItem(p_lb, &alb_);
							if(p_result && p_lb) {
								p_result->SetCallbacks(0, TView::SetupLayoutItemFrameProc, p_lb);
								p_lb->handleWindowsMessage(WM_INITDIALOG, 0, 0);
							}
						}
					}
				}
				{
					TFrame * p_gb = new TFrame(TFrame::fkSizeBar);
					p_gb->SetupSizing(H(), p_lo_left, DIREC_HORZ);
					InsertCtlWithCorrespondingNativeItem(p_gb, CTL_FACADEWINDOW_NAVPANE_SZF, 0, 0);
					{
						SUiLayoutParam alb_;
						alb_.SetFixedSizeX(6.0f);
						alb_.SetVariableSizeY(SUiLayoutParam::szByContainer, 1.0f);
						//alb_.SetMargin(FRect(8.0f, 2.0f, 8.0f, 8.0f));
						{
							//
							// Если pView == 0, то мы вставляем лейаут без привязки к конкретному control'у. Обычно это
							// делается для вспомогательных разметочных лейаутов-контейнеров
							//
							SUiLayout * p_result = 0;
							p_result = p_lo_left->InsertItem(p_gb, &alb_);
							if(p_result && p_gb) {
								p_result->SetCallbacks(0, TView::SetupLayoutItemFrameProc, p_gb);
								p_gb->handleWindowsMessage(WM_INITDIALOG, 0, 0);
							}
						}						
					}
				}
			}
		}
		//InsertWorkWindow(0); // @debug
		{
			MakeNavList(NavBlk);
		}
	}
	return 1;
}

/*static*/const wchar_t * TFacadeWindow::WndClsName = L"TFacadeWindow"; // @global

int TFacadeWindow::DrawNavTreeItem(void * pCustomDrawDescriptor)
{
	int    result = 0;
	bool   debug_mark = false; // @debug
	if(pCustomDrawDescriptor) {
		NMTVCUSTOMDRAW * p_cd = reinterpret_cast<NMTVCUSTOMDRAW *>(pCustomDrawDescriptor);
		if(p_cd->nmcd.dwDrawStage == CDDS_ITEMPREPAINT) {
			if(P_Lo_NavItem) {
				HTREEITEM h_item = reinterpret_cast<HTREEITEM>(p_cd->nmcd.dwItemSpec);
				TVITEMW item;
				RECT   rc_item;
				RECT   rc_cli;
				wchar_t _text[512];
				item.hItem = h_item;
				item.mask = TVIF_TEXT|TVIF_PARAM|TVIF_STATE|TVIF_CHILDREN|TVIF_HANDLE|TVIF_IMAGE;
				item.stateMask = TVIS_EXPANDED|TVIS_EXPANDEDONCE;
				item.pszText = _text;
				item.cchTextMax = SIZEOFARRAY(_text);
				if(TreeView_GetItem(p_cd->nmcd.hdr.hwndFrom, &item)) {
					// code: Value specifying the portion of the item for which to retrieve the bounding rectangle. 
					//   If this parameter is TRUE, the bounding rectangle includes only the text of the item. Otherwise, 
					//   it includes the entire line that the item occupies in the tree-view control.
					BOOL   has_valid_rect = TreeView_GetItemRect(p_cd->nmcd.hdr.hwndFrom, h_item, &rc_item, FALSE/*code*/);
					if(!has_valid_rect || rc_item.right <= rc_item.left || rc_item.bottom <= rc_item.top) { // Область элемента не определена - предварительная фаза
						HWND   h_focus = ::GetFocus();
						MEMSZERO(rc_cli);
					}
					else {
						SPaintToolBox * p_tb = APPL->GetUiToolBox();
						if(p_tb) {
							SUiLayout * p_lo = new SUiLayout(*P_Lo_NavItem);
							TCanvas2 canv(*p_tb, p_cd->nmcd.hdc);
							bool   is_there_image = false;
							int    item_state = TProgram::tbisBase;
							::GetClientRect(p_cd->nmcd.hdr.hwndFrom, &rc_cli);
							// Получение уровня вложенности элемента
							int    level = 0;
							{
								HTREEITEM h_parent = TreeView_GetParent(p_cd->nmcd.hdr.hwndFrom, h_item);
								while(h_parent) {
									level++;
									h_parent = TreeView_GetParent(p_cd->nmcd.hdr.hwndFrom, h_parent);
								}
							}
							const  int indent = TreeView_GetIndent(p_cd->nmcd.hdr.hwndFrom);
							if(p_cd->nmcd.uItemState & CDIS_FOCUS) {
								item_state = TProgram::tbisFocus;
							}
							else if(p_cd->nmcd.uItemState & CDIS_SELECTED) {
								item_state = TProgram::tbisSelect;
							}
							else if(p_cd->nmcd.uItemState & CDIS_HOT) {
								item_state = TProgram::tbisHover;
							}
							/*
								view LAYOUT_LI_CENTRIGONAV [horizontal] {
									view LOITEM_LI_CENTRIGONAV_HIND [size: (16, 16) margin: 4]; // Индикатор иерархии
									view LOITEM_LI_CENTRIGONAV_IMG [size: (16, 16) margin: 4]; // Изображение
									view LOITEM_LI_CENTRIGONAV_MAINTEXT [growfactor: 1 height: bycontainer margin: 4]; // Основной текст
								}
							*/ 
							SUiLayout * p_lo_hind = p_lo->FindById(LOITEM_LI_CENTRIGONAV_HIND);
							SUiLayout * p_lo_img = p_lo->FindById(LOITEM_LI_CENTRIGONAV_IMG);
							SUiLayout * p_lo_text = p_lo->FindById(LOITEM_LI_CENTRIGONAV_MAINTEXT);
							if(p_lo_img && !is_there_image) {
								p_lo_img->SetExcludedStatus();
							}
							{
								SUiLayout::Param evp;
								evp.ForceSize.x = static_cast<float>(rc_item.right - rc_item.left);
								evp.ForceSize.y = static_cast<float>(rc_item.bottom - rc_item.top);
								SUiLayoutParam & r_lp = p_lo->GetLayoutBlock();
								r_lp.Padding.a.x = static_cast<float>(indent * level);
								p_lo->Evaluate(&evp);
							}
							{
								int   state_brush_id = TProgram::tbiListBkgBrush;
								int   state_pen_id = TProgram::tbiListBkgPen;
								if(item_state == TProgram::tbisSelect) {
									state_brush_id = TProgram::tbiListSelBrush;
									state_pen_id = TProgram::tbiListSelPen;
								}
								else if(item_state == TProgram::tbisFocus) {
									state_brush_id = TProgram::tbiListFocBrush;
									state_pen_id = TProgram::tbiListFocPen;
								}
								else if(item_state == TProgram::tbisHover) {
									state_brush_id = TProgram::tbiListFocBrush;
									state_pen_id = TProgram::tbiListFocPen;
								}
								{
									FRect rect_elem_f(rc_item);
									rect_elem_f.Grow(-0.5f, -0.5f);
									// canv.RoundRect(rect_elem_f, 3, pen_id, brush_id);
									canv.Rect(rect_elem_f, state_pen_id, state_brush_id);
								}
								if(p_lo_hind && item.cChildren) {
									const  bool is_expanded = LOGIC(item.state & TVIS_EXPANDED);
									uint   dv_id = is_expanded ? PPDV_TRIANGLELEFT03 : PPDV_TRIANGLEDOWN03;
									if(dv_id) {
										TWhatmanToolArray::Item tool_item;
										const SDrawFigure * p_fig = APPL->LoadDrawFigureById(dv_id, &tool_item);
										if(p_fig) {
											if(!tool_item.ReplacedColor.IsEmpty()) {
												SColor replacement_color = p_tb->GetColor(TProgram::tbiIconRegColor);
												canv.SetColorReplacement(tool_item.ReplacedColor, replacement_color);
											}
											FRect fr = p_lo_hind->GetFrameAdjustedToParent();
											fr.Move__(static_cast<float>(rc_item.left), static_cast<float>(rc_item.top));
											LMatrix2D mtx;
											SViewPort vp;
											canv.PushTransform();
											p_fig->GetViewPort(&vp);
											canv.AddTransform(vp.GetMatrix(fr, mtx));
											canv.Draw(p_fig);
											canv.PopTransform();
											canv.ResetColorReplacement();
										}
									}
								}
								if(p_lo_text) {
									HFONT  hf = (HFONT)::GetStockObject(DEFAULT_GUI_FONT);
									int    temp_font_id = p_tb->CreateFont_(0, hf, 0);
									if(temp_font_id) {
										STextLayout tlo;
										SDrawContext dctx = canv;
										int   text_pen_id = TProgram::tbiListFgPen;
										if(item_state == TProgram::tbisSelect) {
											text_pen_id = TProgram::tbiListSelFgPen;
										}
										else if(item_state == TProgram::tbisFocus) {
											text_pen_id = TProgram::tbiListFocFgPen;
										}
										else if(item_state == TProgram::tbisHover) {
											text_pen_id = TProgram::tbiListFocPen;
										}
										if(APPL->GetDialogTextLayoutU(_text, temp_font_id, text_pen_id, tlo, ADJ_LEFT) > 0) {
											FRect fr = p_lo_text->GetFrameAdjustedToParent();
											fr.Move__(static_cast<float>(rc_item.left), static_cast<float>(rc_item.top));
											tlo.SetBounds(fr);
											tlo.SetOptions(tlo.fVCenter, -1, -1);
											tlo.Arrange(dctx, *p_tb);
											canv.DrawTextLayout(&tlo);
										}
										//*/
									}
								}
							}
							debug_mark = true; // @debug
							// (если мы все сами нарисовали, то возвращаем это) 
							result = CDRF_SKIPDEFAULT;
							delete p_lo;
						}
					}
					debug_mark = true; // @debug
				}
			}
		}
	}
	return result;
}

/*static*/LRESULT CALLBACK TFacadeWindow::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	CREATESTRUCT * p_init_data;
	TFacadeWindow * p_view = 0;
	bool   debug_mark = false; // @debug
	switch(message) {
		case WM_CREATE:
			{
				int    ret = TWindowBase::OnCreate(hWnd, message, wParam, lParam);
				if(ret == 0) {
					p_view = static_cast<TFacadeWindow *>(Helper_InitCreation(lParam, (void **)&p_init_data));
					assert(p_view); // Функция TWindowBase::OnCreate должна была это проверить 
					if(p_view) {
						//p_view->HW = hWnd;
						//TView::SetWindowProp(hWnd, GWLP_USERDATA, p_view);
						::SetFocus(hWnd);
						::SendMessageW(hWnd, WM_NCACTIVATE, TRUE, 0L);
						p_view->WMHCreate();
						::PostMessageW(hWnd, WM_PAINT, 0, 0);
						{
							SString temp_buf;
							TView::SGetWindowText(hWnd, temp_buf);
							APPL->AddItemToMenu(temp_buf, p_view);
						}
					}
				}
				return ret;
			}
		case WM_NOTIFY:
			{
				// @todo @20260217 Здесь обработать NM_CUSTOMDRAW!
				p_view = static_cast<TFacadeWindow *>(TView::GetWindowUserData(hWnd));
				if(p_view) {
					NMHDR * p_nm = reinterpret_cast<NMHDR *>(lParam);
					if(p_nm) {
						if(p_nm->code == NM_CUSTOMDRAW) {
							TView * p_ctl = p_view->getCtrlView(p_nm->idFrom);
							if(p_ctl && p_ctl->IsSubSign(TV_SUBSIGN_LISTBOX)) {
								NMTVCUSTOMDRAW * p_cd = reinterpret_cast<NMTVCUSTOMDRAW *>(p_nm);
								long   result = CDRF_DODEFAULT;
								switch(p_cd->nmcd.dwDrawStage) {
	    							case CDDS_PREPAINT:
										result = CDRF_NOTIFYITEMDRAW;
										break;
									case CDDS_ITEMPREPAINT:
										result = p_view->DrawNavTreeItem(p_cd);
										break;
								}
								return result;
							}
						}
						else {
							TView * p_iter_view = p_view->P_Last;
							if(p_iter_view != 0) {
								do {
									if(p_iter_view->TestId(wParam)) {
										p_iter_view->handleWindowsMessage(message, wParam, lParam);
										break;
									}
								} while((p_iter_view = p_iter_view->prev()) != p_view->P_Last);
							}
						}
					}
				}
			}
			break;
		case WM_COMMAND:
			{
				// Блок почти один-в-один скопирован из TDialog::DialogProc
				uint16 hiw = HIWORD(wParam);
				uint16 low = LOWORD(wParam);
				p_view = static_cast<TFacadeWindow *>(TView::GetWindowUserData(hWnd));
				if(GetKeyState(VK_CONTROL) & 0x8000 && low != cmaCalculate && hiw != EN_UPDATE && hiw != EN_CHANGE)
					return 0;
				else if(p_view) {
					if(hiw == 0 && low == IDCANCEL) {
						TView::messageCommand(p_view, cmCancel, p_view);
						return 0;
					}
					else {
						if(!lParam) {
							if(hiw == 0) // from menu
								TView::messageKeyDown(p_view, low);
							else if(hiw == 1) { // from accelerator
								TEvent event;
								event.what = TEvent::evCommand;
								event.message.command = low;
								p_view->handleEvent(event);
							}
						}
						TView * local_p_view = p_view->CtrlIdToView(CLUSTER_ID(low));
						if(local_p_view && local_p_view->IsConsistent())
							local_p_view->handleWindowsMessage(message, wParam, lParam);
					}
				}
				else
					return 0;
			}
			break;
		case WM_USER_KEYDOWN:
			{
				p_view = static_cast<TFacadeWindow *>(TView::GetWindowUserData(hWnd));
				if(p_view) {
					uint   key = static_cast<uint>(wParam);
					HWND   h_ctl = reinterpret_cast<HWND>(lParam);
					TView * p_ctl = static_cast<TView *>(TView::GetWindowUserData(h_ctl));
					if(key == VK_RETURN && p_ctl && p_ctl->IsSubSign(TV_SUBSIGN_INPUTLINE)) {
						TInputLine * p_il = static_cast<TInputLine *>(p_ctl);
						char   temp_b[1024];
						p_il->TransmitData(-1, temp_b);
						SString input_buf(temp_b);
						p_view->HandleInputEnter(input_buf);
					}
				}
			}
			return 0;
		case WM_DESTROY:
			p_view = static_cast<TFacadeWindow *>(TView::GetWindowUserData(hWnd));
			if(p_view && p_view->IsConsistent()) {
				//p_view->SaveChanges();
				TWindowBase::Helper_Finalize(hWnd, p_view);
			}
			return 0;
		case WM_SETFOCUS:
			p_view = static_cast<TFacadeWindow *>(TView::GetWindowUserData(hWnd));
			TWindowBase::Helper_SetFocus(hWnd, p_view);
			if(p_view) {
				if(p_view->H_RecentFocusedChild && p_view->H_RecentFocusedChild != hWnd) {
					::SetFocus(p_view->H_RecentFocusedChild);
				}
			}
			break;
		case WM_KILLFOCUS:
			if(!(TView::SGetWindowStyle(hWnd) & WS_CAPTION))
				APPL->NotifyFrame(0);
			p_view = static_cast<TFacadeWindow *>(TView::GetWindowUserData(hWnd));
			if(p_view) {
				TView::messageBroadcast(p_view, cmReleasedFocus);
				p_view->ResetOwnerCurrent();
			}
			break;
		case WM_KEYDOWN:
			if(wParam == VK_TAB) {
				p_view = static_cast<TFacadeWindow *>(TView::GetWindowUserData(hWnd));
				if(p_view && GetKeyState(VK_CONTROL) & 0x8000 && !p_view->IsInState(sfModal)) {
					SetFocus(GetNextBrowser(hWnd, (GetKeyState(VK_SHIFT) & 0x8000) ? 0 : 1));
					return 0;
				}
			}
			return 0;
		case WM_SIZE:
			p_view = static_cast<TFacadeWindow *>(TView::GetWindowUserData(hWnd));
			if(!TView::Helper_SendCmSizeAsReplyOnWmSize(p_view, wParam, lParam))
				return 0;
			break;
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}

/*static*/int TFacadeWindow::RegWindowClass(HINSTANCE hInst)
{
	WNDCLASSEXW wc;
	TBaseBrowserWindow::MakeDefaultWindowClassBlock(&wc, hInst);
	{
		const UiDescription * p_uid = SLS.GetUiDescription();
		const SColorSet * p_cs = p_uid ? p_uid->GetColorSetC("papyrus_style") : 0;
		SColor clr = UiDescription::GetColorR(p_uid, p_cs, "main_window_bg", SColor(0xEE, 0xEE, 0xEE));
		wc.hbrBackground = ::CreateSolidBrush(static_cast<COLORREF>(clr));
	}
	wc.lpfnWndProc   = TFacadeWindow::WndProc;
	wc.hIcon         = LoadIconW(hInst, MAKEINTRESOURCE(/*ICON_TIMEGRID*/172));
	wc.lpszClassName = TFacadeWindow::WndClsName;
	return RegisterClassExW(&wc);
}

TFacadeWindow::TFacadeWindow() : TBaseBrowserWindow(WndClsName), P_Lo_NavItem(0), H_RecentFocusedChild(0)
{
	BbState |= (bbsWoScrollbars|bbsCtlParent);
	static bool win_cls_registered = false;
	if(!win_cls_registered) {
		TFacadeWindow::RegWindowClass(TProgram::GetInst());
		win_cls_registered = true;
	}
	{ // @v12.5.7 @construction
		SString temp_buf;
		temp_buf.Z().Cat(LAYOUT_LI_CENTRIGONAV);
		P_Lo_NavItem = PPLoadDl600Layout(temp_buf, 0); 
	}
}

TFacadeWindow::~TFacadeWindow()
{
	ZDELETE(P_Lo_NavItem);
	H_RecentFocusedChild = 0;
}

int TFacadeWindow::RemoveWorkingPanel()
{
	int    ok = -1;
	TView * p_view = getCtrlView(ViewId_Primary);
	if(p_view) {
		removeView(p_view);
		P_Lfc->DeleteItemByManagedPtr(p_view);
		delete p_view;
		ok = 1;
	}
	return ok;
}

int TFacadeWindow::HandleInputEnter(const SString & rInput)
{
	int    ok = -1;
	const  SString org_input(rInput);
	SString _input(org_input);
	SString _result_text;
	if(_input.NotEmptyS()) {
		/*
		int    lstb_reg_state_result = 0;
		_input.Transf(CTRANSF_INNER_TO_UTF8);
		LocalStateBinderyCore * p_lstb = DS.GetTLA().GetLocalStateBindery();
		if(p_lstb) {
			LocalStateBinderyCore::StateIdent state_ident;
			state_ident.Kind = LocalStateBinderyCore::kInput;
			state_ident.Subj = UED::SetRaw_UXControlIdent(SObjID(WNDID_FACADEWINDOW, CTL_FACADEWINDOW_MAININPUT));
			if(state_ident.Subj) {
				PPID   sid = 0;
				SBuffer state_input_data;
				state_input_data.Write(_input.cptr(), _input.Len()+1);
				lstb_reg_state_result = p_lstb->RegisterState(&sid, state_ident, state_input_data, 1);
			}
		}
		*/
		TView::CallOnAcceptInputForWordSelExtraBlocks(this); // @v12.5.10
		if(_input.IsEqiAscii("close")) { // @debug
			RemoveWorkingPanel();
		}
		else if(_input.HasPrefixIAscii("=")) {
			_input.ShiftLeft();
			double result = 0.0;
			if(PPExprParser::CalcExpression(_input, &result, 0, 0)) {
				_result_text.Cat(_input).CatDiv('=', 1).Cat(result, MKSFMTD(0, 6, NMBF_NOTRAILZ));
			}
			else {
				_result_text.Cat("Bad expression to evaluate").CatDiv(':', 2).Cat(_input);
			}
		}
		else {
			// @construction @2020226
			const uint32 fav_nt_list[] =  {
				SNTOK_GUID, SNTOK_EMAIL, 
			};
			STokenRecognizer tr;
			SNaturalTokenArray nta;
			uint32 fav_nt_id = 0;
			tr.Run(_input.ucptr(), _input.Len(), nta, 0);
			for(uint i = 0; !fav_nt_id && i < nta.getCount(); i++) {
				const SNaturalToken & r_nt = nta.at(i);
				switch(r_nt.ID) {
					case SNTOK_GUID: fav_nt_id = r_nt.ID; break;
					case SNTOK_EMAIL: fav_nt_id = r_nt.ID; break;
				}
			}
		}
	}
	setCtrlString(CTL_FACADEWINDOW_MAININPUT, _input.Z());
	setCtrlString(CTL_FACADEWINDOW_INFOLINE, _result_text);
	return ok;
}

IMPL_HANDLE_EVENT(TFacadeWindow)
{
	bool   debug_mark = false; // @debug
	TWindowBase::handleEvent(event);
	if(TVKEYDOWN) {
		if(oneof4(TVKEY, kbAltLeft, kbAltRight, kbAltUp, kbAltDown)) {
			HWND   h_focus = ::GetFocus();
			if(h_focus) {
				//TView * p_view_p = getCtrlView(ViewId_Primary);
				const TView * p_view_ = getCtrlViewByHandleC(h_focus);
				if(p_view_) {
					if(p_view_->GetId() == ViewId_Primary) {
						if(TVKEY == kbAltLeft) {
							TView * p_new_focus = getCtrlView(CTL_FACADEWINDOW_NAVPANE);
							if(p_new_focus) {
								::SetFocus(p_new_focus->getHandle());
							}
						}
						else if(TVKEY == kbAltUp) {
							TView * p_new_focus = getCtrlView(CTL_FACADEWINDOW_MAININPUT);
							if(p_new_focus) {
								::SetFocus(p_new_focus->getHandle());
							}
						}
					}
					else if(p_view_->GetId() == CTL_FACADEWINDOW_NAVPANE) {
						if(TVKEY == kbAltRight) {
							TView * p_new_focus = getCtrlView(ViewId_Primary);
							if(p_new_focus) {
								HWND   h_new_focus = p_new_focus->getHandle();
								::SetFocus(h_new_focus);
							}
						}
						else if(TVKEY == kbAltUp) {
							TView * p_new_focus = getCtrlView(CTL_FACADEWINDOW_MAININPUT);
							if(p_new_focus) {
								HWND   h_new_focus = p_new_focus->getHandle();
								::SetFocus(h_new_focus);
							}
						}
					}
					else if(p_view_->GetId() == CTL_FACADEWINDOW_MAININPUT) {
						if(TVKEY == kbAltDown) {
							TView * p_new_focus = getCtrlView(ViewId_Primary);
							if(p_new_focus) {
								::SetFocus(p_new_focus->getHandle());
							}
						}
						else if(TVKEY == kbAltLeft) {
							TView * p_new_focus = getCtrlView(CTL_FACADEWINDOW_NAVPANE);
							if(p_new_focus) {
								::SetFocus(p_new_focus->getHandle());
							}
						}
					}
				}
				//getCtrlView
				debug_mark = true; // @debug
			}
		}
	}
	else if(TVINFOPTR) {
		if(event.isCmd(cmInit)) {
			;
		}
		else if(event.isCmd(cmPaint)) {
			;
		}
		else if(event.isCmd(cmSize)) {
			;
		}
		else if(event.isCmd(cmMouse)) {
			//MouseEvent * p_me = static_cast<MouseEvent *>(TVINFOPTR);
			;
		}
		else if(event.isCmd(cmChildFocusReceived)) { // @v12.6.0
			TView * p_child = static_cast<TView *>(TVINFOPTR);
			HWND   h_c = p_child->getHandle();
			if(h_c && h_c != H()) {
				H_RecentFocusedChild = h_c;
			}
		}
		/* (тупиковая ветка) else if(event.isCmd(cmCustomDraw)) {
			NMTVCUSTOMDRAW * p_tv_blk = reinterpret_cast<NMTVCUSTOMDRAW *>(event.message.infoPtr);
			if(p_tv_blk && p_tv_blk->nmcd.hdr.idFrom == CTL_FACADEWINDOW_NAVPANE) {
				TView * p_view = getCtrlView(CTL_FACADEWINDOW_NAVPANE);
				if(p_view) {
					switch(p_tv_blk->nmcd.dwDrawStage) {
						case CDDS_PREPAINT:
							clearEvent(event);
							event.message.infoLong = CDRF_NOTIFYITEMDRAW;
							break;
						case CDDS_ITEMPREPAINT:
							{
								HTREEITEM h_item = reinterpret_cast<HTREEITEM>(p_tv_blk->nmcd.dwItemSpec);
								debug_mark = true; // @debug
							}
							break;
					}
				}
			}
			debug_mark = true;
		}*/
		/*
		else if(event.isCmd(cmDrawItem)) {
			if(false) {
				TDrawItemData * p_draw_item = static_cast<TDrawItemData *>(TVINFOPTR);
				if(p_draw_item && p_draw_item->P_View) {
					PPID   list_ctrl_id = p_draw_item->P_View->GetId();
					if(list_ctrl_id == CTL_FACADEWINDOW_NAVPANE) {
						SmartListBox * p_lbx = static_cast<SmartListBox *>(p_draw_item->P_View);
						if(p_draw_item->ItemAction & TDrawItemData::iaBackground) {
							debug_mark = true; // @debug
							//canv.Rect(_rc, 0, clrBkgnd);
							//p_draw_item->ItemAction = 0; // Мы перерисовали фон
						}
						else if(p_draw_item->ItemID != _FFFF32) {
							debug_mark = true; // @debug
						}
					}
				}
			}
		}
		*/
		else if(event.isCmd(cmLBDblClk)) {
			TView * p_view = static_cast<TView *>(TVINFOPTR);
			if(p_view->IsConsistent() && p_view->IsSubSign(TV_SUBSIGN_LISTBOX)) {
				SmartListBox * p_lb = static_cast<SmartListBox *>(p_view);
				int    _id = 0;
				if(p_lb->getCurID(&_id)) {
					if(_id) {
						const CentrigoNavBlock::Entry * p_entry = NavBlk.SearchEntry(_id);
						if(p_entry) {
							debug_mark = true; // @debug
							switch(p_entry->Oid.Obj) {
								case PPOBJ_UXCMD:
									switch(p_entry->Oid.Id) {
										case cmCentrigoNotes:
											{
												SObjID oid(PPOBJ_WORKBOOK, 0);
												DoNote(oid);
											}
											break;
										case cmCentrigoContacts:
											{
												PersonFilt filt;
												filt.Flags |= (PersonFilt::fInMemView|PersonFilt::fCentrigoContacts);
												DoContacts(&filt);
											}
											break;
										case cmCentrigoToDo:
											{
												PrjTaskFilt filt;
												filt.Flags |= PrjTaskFilt::fInMemView;
												DoTasks(&filt);
											}
											break;
										case cmCentrigoWallet:
											break;
										case cmCentrigoSecrets:
											{
												DoSecrets();
											}
											break;
									}
									break;
								case PPOBJ_WORKBOOK:
									{
										SObjID oid(p_entry->Oid);
										DoNote(oid);
									}
									break;
							}
						}
					}
				}
				
			}
		}
	}
}

int Launch_TFacadeWindow()
{
	int    ok = -1;
	TFacadeWindow * p_win = new TFacadeWindow();
	InsertView(p_win);
	return ok;
}
//
//
//
static SIntToSymbTabEntry SecSegTypeList[] = {
	{ PPSecretSegment::sectypUndef, "undef" },
	{ PPSecretSegment::sectypFolder, "folder" },
	{ PPSecretSegment::sectypGeneric, "generic" },
	{ PPSecretSegment::sectypPassword, "password" },
	{ PPSecretSegment::sectypAuthSecret, "authsecret" },
	{ PPSecretSegment::sectypOpenKey, "openkey" },
	{ PPSecretSegment::sectypBankCard, "bankcard" },
	{ PPSecretSegment::sectypSSH, "ssh" },
	{ PPSecretSegment::sectypESignature, "esignature" },
};

PPSecretSegment::CoreEntry::CoreEntry()
{
	THISZERO();
}
		
PPSecretSegment::CoreEntry & PPSecretSegment::CoreEntry::Z()
{
	THISZERO();
	return *this;
}
		
bool PPSecretSegment::CoreEntry::IsEmpty() const
{
	return (!STextOpenP && !STextHiddenP);
}

PPSecretSegment::PPSecretSegment() : P_OwnSg(0), P_OuterSg(0)
{
	memzero(&InternalID, offsetof(PPSecretSegment, Reserve) + sizeof(Reserve) - offsetof(PPSecretSegment, InternalID));
}

PPSecretSegment::PPSecretSegment(const PPSecretSegment & rS) : P_OwnSg(0), P_OuterSg(0)
{
	Copy(rS);
}
	
PPSecretSegment::PPSecretSegment(SStrGroup * pOuterSg) : P_OwnSg(0), P_OuterSg(pOuterSg)
{
	memzero(&InternalID, offsetof(PPSecretSegment, Reserve) + sizeof(Reserve) - offsetof(PPSecretSegment, InternalID));
}
	
PPSecretSegment::~PPSecretSegment()
{
	delete P_OwnSg;
}

bool PPSecretSegment::IsEmpty() const 
{
	return (!SecType && !NameP);
}

PPSecretSegment & PPSecretSegment::Z()
{
	memzero(&InternalID, offsetof(PPSecretSegment, Reserve) + sizeof(Reserve) - offsetof(PPSecretSegment, InternalID));
	History.clear();
	if(P_OwnSg) {
		P_OwnSg->ClearS();
	}
	return *this;
}

bool FASTCALL PPSecretSegment::IsEq(const PPSecretSegment & rS) const
{
	bool   eq = true;
	if(InternalID != rS.InternalID)
		eq = false;
	else if(ParentID != rS.ParentID)
		eq = false;
	else if(SecType != rS.SecType)
		eq = false;
	else if(UedRefOid != rS.UedRefOid)
		eq = false;
	else if(UedEnterTm != rS.UedEnterTm)
		eq = false;
	else if(UedHashAlg != rS.UedHashAlg)
		eq = false;
	else if(CE.UedBeforeTm != rS.CE.UedBeforeTm)
		eq = false;
	else if(History.getCount() != rS.History.getCount())
		eq = false;
	else {
		{
			for(uint i = 0; eq && i < History.getCount(); i++) {
				const  CoreEntry & r_ce = History.at(i);
				const  CoreEntry & r_ce2 = rS.History.at(i);
				if(r_ce.UedBeforeTm != r_ce2.UedBeforeTm) {
					eq = false;
				}
			}
		}
		if(eq) {
			TSVector <uint> spl1;
			TSVector <uint> spl2;
			spl1.insert(&NameP);
			spl1.insert(&DescrP);
			spl1.insert(&ExecCmdLineP);
			spl1.insert(&CE.STextOpenP);
			spl1.insert(&CE.STextHiddenP);
			spl1.insert(&CE.STextExpiryP);
			spl1.insert(&CE.STextExt1P);
			spl1.insert(&CE.STextExt2P);
			spl1.insert(&CE.STextExt3P);
			assert(History.getCount() == rS.History.getCount()); // Проверено выше!
			{
				for(uint i = 0; i < History.getCount(); i++) {
					const  CoreEntry & r_ce = History.at(i);
					spl1.insert(&r_ce.STextOpenP);
					spl1.insert(&r_ce.STextHiddenP);
					spl1.insert(&r_ce.STextExpiryP);
					spl1.insert(&r_ce.STextExt1P);
					spl1.insert(&r_ce.STextExt2P);
					spl1.insert(&r_ce.STextExt3P);
				}
			}
			spl2.insert(&rS.NameP);
			spl2.insert(&rS.DescrP);
			spl2.insert(&rS.ExecCmdLineP);
			spl2.insert(&rS.CE.STextOpenP);
			spl2.insert(&rS.CE.STextHiddenP);
			spl2.insert(&rS.CE.STextExpiryP);
			spl2.insert(&rS.CE.STextExt1P);
			spl2.insert(&rS.CE.STextExt2P);
			spl2.insert(&rS.CE.STextExt3P);
			{
				for(uint i = 0; i < rS.History.getCount(); i++) {
					const  CoreEntry & r_ce = rS.History.at(i);
					spl2.insert(&r_ce.STextOpenP);
					spl2.insert(&r_ce.STextHiddenP);
					spl2.insert(&r_ce.STextExpiryP);
					spl2.insert(&r_ce.STextExt1P);
					spl2.insert(&r_ce.STextExt2P);
					spl2.insert(&r_ce.STextExt3P);
				}
			}
			assert(spl1.getCount() == spl2.getCount()); // Если не выполняется, то я ошибся где-то выше!
			if(spl1.getCount() != spl2.getCount()) {
				eq = false;
			}
			else {
				const  SStrGroup & r_sg1 = GetSgC();
				const  SStrGroup & r_sg2 = rS.GetSgC();
				SString t1;
				SString t2;
				for(uint i = 0; eq && i < spl1.getCount(); i++) {
					const  uint p1 = spl1.at(i);
					const  uint p2 = spl2.at(i);
					r_sg1.GetS(p1, t1);
					r_sg2.GetS(p2, t2);
					if(t1 != t2)
						eq = false;
				}
			}
		}
	}
	return eq;
}

PPSecretSegment & FASTCALL PPSecretSegment::operator = (const PPSecretSegment & rS)
{
	Copy(rS);
	return *this;
}

bool FASTCALL PPSecretSegment::Copy(const PPSecretSegment & rS)
{
	Z();
	ZDELETE(P_OwnSg);
	bool   ok = true;
	#define CPYFLD(f) f = rS.f
	CPYFLD(InternalID);
	CPYFLD(ParentID);
	CPYFLD(SecType);
	CPYFLD(UedRefOid);
	CPYFLD(UedEnterTm);
	CPYFLD(UedHashAlg);
	CPYFLD(NameP);        // Наименование сегмента (позиция в SStrGroup) 
	CPYFLD(CE);
	CPYFLD(DescrP);
	CPYFLD(ExecCmdLineP);
	CPYFLD(History);
	#undef CPYFLD
	if(rS.P_OwnSg) {
		P_OwnSg = new SStrGroup(*rS.P_OwnSg);
	}
	if(rS.P_OuterSg) {
		P_OuterSg = rS.P_OuterSg;
	}
	return ok;
}

bool PPSecretSegment::GetText(uint32 textP, SString & rBuf) const
{
	return GetSgC().GetS(textP, rBuf);
}

SStrGroup & PPSecretSegment::GetSg()
{
	SStrGroup * p_result = NZOR(P_OuterSg, P_OwnSg);
	assert(p_result);
	return *p_result;
}

const SStrGroup & PPSecretSegment::GetSgC() const
{
	SStrGroup * p_result = NZOR(P_OuterSg, P_OwnSg);
	assert(p_result);
	return *p_result;
}

SJson * PPSecretSegment::ToJsonObj() const
{
	SJson * p_result = SJson::CreateObj();
	if(p_result) {
		SString temp_buf;
		if(InternalID)
			p_result->InsertUInt("InternalID", InternalID);
		if(ParentID)
			p_result->InsertUInt("ParentID", ParentID);
		if(SecType)
			p_result->InsertUInt("SecType", SecType);
		if(UedRefOid)
			p_result->InsertUInt64("UedRefOid", UedRefOid);
		if(UedEnterTm)
			p_result->InsertUInt64("UedEnterTm", UedEnterTm);
		if(CE.UedBeforeTm)
			p_result->InsertUInt64("UedExpiryTm", CE.UedBeforeTm);
		if(UedHashAlg)
			p_result->InsertUInt64("UedHashAlg", UedHashAlg);
		{
			struct SecretSegment_TextFldMapEntry_Out {
				const  char * P_Symb;
				uint32 Var;
			};
			{
				/*non-static*/const SecretSegment_TextFldMapEntry_Out tfm[] = {
					{ "Name", NameP },
					{ "Descr", DescrP },
					{ "ExecCmdLine", ExecCmdLineP},
					{ "STextOpen", CE.STextOpenP },
					{ "STextHidden", CE.STextHiddenP },
					{ "STextExpiry", CE.STextExpiryP },
					{ "STextExt1", CE.STextExt1P },
					{ "STextExt2", CE.STextExt2P },
					{ "STextExt3", CE.STextExt3P },
				};
				for(uint i = 0; i < SIZEOFARRAY(tfm); i++) {
					const  SecretSegment_TextFldMapEntry_Out & r_tfmi = tfm[i];
					if(GetText(r_tfmi.Var, temp_buf)) {
						temp_buf.Escape();
						p_result->InsertStringNe(r_tfmi.P_Symb, temp_buf);
					}
				}
			}
			if(History.getCount()) {
				SJson * p_js_hist = SJson::CreateArr();
				if(p_js_hist) {
					for(uint hi = 0; hi < History.getCount(); hi++) {
						const CoreEntry & r_he = History.at(hi);
						if(!r_he.IsEmpty()) {
							SJson * p_js_hi = SJson::CreateObj();
							if(p_js_hi) {
								/*non-static*/const SecretSegment_TextFldMapEntry_Out tfm[] = {
									{ "STextOpen", r_he.STextOpenP },
									{ "STextHidden", r_he.STextHiddenP },
									{ "STextExpiry", r_he.STextExpiryP },
									{ "STextExt1", r_he.STextExt1P },
									{ "STextExt2", r_he.STextExt2P },
									{ "STextExt3", r_he.STextExt3P },
								};
								if(r_he.UedBeforeTm)
									p_js_hi->InsertUInt64("UedExpiryTm", r_he.UedBeforeTm);
								for(uint i = 0; i < SIZEOFARRAY(tfm); i++) {
									const  SecretSegment_TextFldMapEntry_Out & r_tfmi = tfm[i];
									if(GetText(r_tfmi.Var, temp_buf)) {
										temp_buf.Escape();
										p_js_hi->InsertStringNe(r_tfmi.P_Symb, temp_buf);
									}
								}
								p_js_hist->InsertChild(p_js_hi);
							}
						}
					}
					p_result->Insert("history", p_js_hist);
				}
			}
		}
	}
	return p_result;
}
	
bool PPSecretSegment::FromJsonObj(const SJson * pJs)
{
	Z();
	if(SJson::IsObject(pJs)) {
		SString temp_buf;
		struct SecretSegment_TextFldMapEntry_In {
			const  char * P_Symb;
			uint32 * P_Var;
		};
		SecretSegment_TextFldMapEntry_In tfm[] = {
			{ "Name", &NameP },
			{ "Descr", &DescrP },
			{ "ExecCmdLine", &ExecCmdLineP},
			{ "STextOpen", &CE.STextOpenP },
			{ "STextHidden", &CE.STextHiddenP },
			{ "STextExpiry", &CE.STextExpiryP },
			{ "STextExt1", &CE.STextExt1P },
			{ "STextExt2", &CE.STextExt2P },
			{ "STextExt3", &CE.STextExt3P },
		};
		SStrGroup & r_sg = GetSg();
		enum {
			occfEnterTm  = 0x0001,
			occfExpiryTm = 0x0002,
		};
		uint   occur_flags = 0;
		for(const SJson * p_cur = pJs->P_Child; p_cur; p_cur = p_cur->P_Next) {
			bool   local_done = false;
			{
				for(uint i = 0; !local_done && i < SIZEOFARRAY(tfm); i++) {
					const  SecretSegment_TextFldMapEntry_In & r_tfmi = tfm[i];
					if(p_cur->Text.IsEqiAscii(r_tfmi.P_Symb)) {
						(temp_buf = p_cur->P_Child->Text).Unescape();
						r_sg.AddS(temp_buf, r_tfmi.P_Var);
						local_done = true;
					}
				}
			}
			if(!local_done) {
				if(p_cur->Text.IsEqiAscii("InternalID")) {
					InternalID = p_cur->P_Child->Text.ToULong();
				}
				else if(p_cur->Text.IsEqiAscii("ParentID")) {
					ParentID = p_cur->P_Child->Text.ToULong();
				}
				else if(p_cur->Text.IsEqiAscii("SecType")) {
					if(p_cur->P_Child->Text.IsDec()) {
						SecType = p_cur->P_Child->Text.ToULong();
					}
					else {
						int   st = SIntToSymbTab_GetId(SecSegTypeList, SIZEOFARRAY(SecSegTypeList), p_cur->P_Child->Text);
						if(st) {
							SecType = st;
						}
						else {
							// @todo @err
						}
					}
				}
				else if(p_cur->Text.IsEqiAscii("UedRefOid")) {
					UedRefOid = p_cur->P_Child->Text.ToUInt64();
				}
				else if(p_cur->Text.IsEqiAscii("UedEnterTm")) {
					UedEnterTm = p_cur->P_Child->Text.ToUInt64();
					occur_flags |= occfEnterTm;
				}
				else if(p_cur->Text.IsEqiAscii("UedEnter")) {
					(temp_buf = p_cur->P_Child->Text).Unescape();
					LDATETIME dtm;
					if(strtodatetime(temp_buf, dtm, DATF_ISO8601CENT, 0)) {
						SUniTime_Internal uti(dtm);
						UedEnterTm = UED::_SetRaw_Time(UED_META_TIME_MSEC, uti);
					}
					occur_flags |= occfEnterTm;
				}
				else if(p_cur->Text.IsEqiAscii("UedExpiryTm")) {
					CE.UedBeforeTm = p_cur->P_Child->Text.ToUInt64();
					occur_flags |= occfExpiryTm;
				}
				else if(p_cur->Text.IsEqiAscii("UedExpiry")) {
					(temp_buf = p_cur->P_Child->Text).Unescape();
					LDATETIME dtm;
					if(strtodatetime(temp_buf, dtm, DATF_ISO8601CENT, 0)) {
						SUniTime_Internal uti(dtm);
						CE.UedBeforeTm = UED::_SetRaw_Time(UED_META_TIME_MSEC, uti);
					}
					occur_flags |= occfExpiryTm;
				}
				else if(p_cur->Text.IsEqiAscii("UedHashAlg")) {
					UedHashAlg = p_cur->P_Child->Text.ToUInt64();
				}
				else if(p_cur->Text.IsEqiAscii("history")) {
					if(SJson::IsArray(p_cur->P_Child)) {
						for(const SJson * p_js_hi = p_cur->P_Child->P_Child; p_js_hi; p_js_hi = p_js_hi->P_Next) {
							if(SJson::IsObject(p_js_hi)) {
								uint   hist_occur_flags = 0;
								for(const SJson * p_js_f = p_js_hi->P_Child; p_js_f; p_js_f = p_js_f->P_Next) {
									if(p_js_f->P_Child) {
										CoreEntry hi;
										if(p_js_f->Text.IsEqiAscii("UedExpiryTm")) {
											hi.UedBeforeTm = p_js_f->P_Child->Text.ToUInt64();
											hist_occur_flags |= occfExpiryTm;
										}
										else if(p_js_f->Text.IsEqiAscii("ExpiryTm")) {
											(temp_buf = p_js_f->P_Child->Text).Unescape();
											LDATETIME dtm;
											if(strtodatetime(temp_buf, dtm, DATF_ISO8601CENT, 0)) {
												SUniTime_Internal uti(dtm);
												hi.UedBeforeTm = UED::_SetRaw_Time(UED_META_TIME_MSEC, uti);
											}
											hist_occur_flags |= occfExpiryTm;
										}
										else {
											SecretSegment_TextFldMapEntry_In htfm[] = {
												{ "STextOpen", &hi.STextOpenP },
												{ "STextHidden", &hi.STextHiddenP },
												{ "STextExpiry", &hi.STextExpiryP },
												{ "STextExt1", &hi.STextExt1P },
												{ "STextExt2", &hi.STextExt2P },
												{ "STextExt3", &hi.STextExt3P },
											};
											for(uint i = 0; i < SIZEOFARRAY(htfm); i++) {
												const  SecretSegment_TextFldMapEntry_In & r_tfmi = htfm[i];
												if(p_js_f->Text.IsEqiAscii(r_tfmi.P_Symb)) {
													(temp_buf = p_js_f->P_Child->Text).Unescape();
													r_sg.AddS(temp_buf, r_tfmi.P_Var);
													break;
												}
											}
										}
										if(!hi.IsEmpty()) {
											History.insert(&hi);
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
	return !IsEmpty();
}

PPSecretSegmentPool::PPSecretSegmentPool() : P_Vault(0)
{
}

PPSecretSegmentPool::~PPSecretSegmentPool()
{
	delete P_Vault;
}

bool PPSecretSegmentPool::IsInWork() const
{
	return (P_Vault && P_Vault->GetKeyRef());
}

bool FASTCALL PPSecretSegmentPool::IsEq(const PPSecretSegmentPool & rS) const
{
	return TSCollection_IsEq(this, &rS);
}

bool PPSecretSegmentPool::PutText(const char * pText, uint * pTextP)
{
	return LOGIC(Sg.AddS(pText, pTextP));
}

bool PPSecretSegmentPool::GetText(uint textP, SString & rBuf) const
{
	return Sg.GetS(textP, rBuf);
}

static const uint64 SecretSegmentPool_Signature = 0xA0C7E0A913D9C152ULL;

int PPSecretSegmentPool::DescryptSegments(const char * pMasterPassword, size_t masterPasswordLen)
{
	freeAll();
	int    ok = -1;
	SString temp_buf;
	SJson * p_js = 0;
	THROW(P_Vault); // @todo @err
	THROW_SL(P_Vault->CheckInPrimaryPassword(pMasterPassword, masterPasswordLen));
	THROW(P_Vault->GetKeyRef()); // @todo @err
	{
		const  uint64 key_ref = P_Vault->GetKeyRef();
		uint32 vp_id = 0;
		SBinaryChunk bc;
		for(size_t vp = 0; P_Vault->Enum(&vp, &vp_id, 0);) {
			const  uint32 outer_id = P_Vault->MakeOuterId(vp_id);
			if(outer_id) {
				const  int re = P_Vault->GetEncrypted(vp_id, reinterpret_cast<void *>(key_ref), &bc);
				if(re) {
					temp_buf.Z().CatN(static_cast<const char *>(bc.PtrC()), bc.Len());
					uint   new_seg_pos = 0;
					PPSecretSegment * p_new_seg = CreateNewSegment(&new_seg_pos);
					THROW(p_new_seg);
					{
						p_js = SJson::Parse(temp_buf);
						THROW(p_js);
						const  int fjr = p_new_seg->FromJsonObj(p_js);
						THROW(fjr);
						p_js->Destroy(true);
						ZDELETE(p_js);
						p_new_seg->InternalID = outer_id;
						ok = 1;
					}
				}
			}
		}
	}
	CATCH
		freeAll();
		ok = 0;
	ENDCATCH
	if(p_js) {
		p_js->Destroy(true);
		ZDELETE(p_js);
	}
	temp_buf.Obfuscate();
	temp_buf.Z();
	return ok;
}

int PPSecretSegmentPool::LoadStorage(const char * pFileName, const char * pMasterPassword, size_t masterPasswordLen)
{
	int   ok = 1;
	freeAll();
	ZDELETE(P_Vault);
	THROW(fileExists(pFileName));
	{
		SSerializeContext sctx;
		SBuffer sbuf;
		SFile f(pFileName, SFile::mRead|SFile::mBinary|SFile::mNoStd);
		THROW(f.IsValid());
		{
			uint64 signature = 0;
			uint64 data_size = 0;
			THROW_SL(f.Read(&signature, sizeof(signature)));
			THROW(signature == SecretSegmentPool_Signature); // @todo @err
			THROW_SL(f.Read(&data_size, sizeof(data_size)));
			if(data_size) {
				STempBuffer temp_buf(SMEGABYTE(1));
				uint64 read_size = 0;
				THROW_SL(temp_buf.IsValid());
				while(read_size < data_size) {
					size_t actual_size = 0;
					THROW_SL(f.Read(temp_buf, temp_buf.GetSize(), &actual_size));
					THROW_SL(sbuf.Write(temp_buf, actual_size));
					read_size += actual_size;
				}
				THROW(P_Vault = new SVaultPool());
				THROW_SL(P_Vault->Serialize(-1, sbuf, &sctx));
				if(pMasterPassword) {
					THROW(DescryptSegments(pMasterPassword, masterPasswordLen));
				}
			}
		}
	}
	CATCH
		ZDELETE(P_Vault);
		ok = 0;
	ENDCATCH
	return ok;
}

PPSecretSegment * PPSecretSegmentPool::CreateNewSegment(uint * pPos)
{
	uint   pos = 0;
	PPSecretSegment * p_result = new PPSecretSegment(&Sg);
	if(p_result) {
		uint   max_id = 0;
		for(uint i = 0; i < getCount(); i++) {
			const  PPSecretSegment * p_item = at(i);
			if(p_item) {
				SETMAX(max_id, p_item->InternalID);
			}
		}
		p_result->InternalID = (max_id > 0) ? (max_id+1) : 1;
		{
			SUniTime_Internal ut;
			ut.SetCurrent();
			p_result->UedEnterTm = UED::_SetRaw_Time(UED_META_TIME_MSEC, ut);
		}
		if(insert(p_result)) {
			assert(getCount());
			pos = getCount() - 1;
		}
		else {
			ZDELETE(p_result);
		}
	}
	ASSIGN_PTR(pPos, pos);
	return p_result;
}

PPSecretSegment * PPSecretSegmentPool::SearchSegmentByID(uint id, uint * pPos)
{
	PPSecretSegment * p_result = 0;
	uint   pos = 0;
	if(id) {
		for(uint i = 0; !p_result && i < getCount(); i++) {
			PPSecretSegment * p_item = at(i);
			if(p_item && p_item->InternalID == id) {
				p_result = p_item;
				pos = i;
			}
		}
	}
	ASSIGN_PTR(pPos, pos);
	return p_result;
}

PPSecretSegment * PPSecretSegmentPool::SearchSegmentByName(const char * pKey, uint * pPos)
{
	PPSecretSegment * p_result = 0;
	uint   pos = 0;
	if(!isempty(pKey)) {
		SString temp_buf;
		for(uint i = 0; !p_result && i < getCount(); i++) {
			PPSecretSegment * p_item = at(i);
			if(p_item && GetText(p_item->NameP, temp_buf)) {
				if(temp_buf.IsEqiUtf8(pKey)) {
					p_result = p_item;
					pos = i;
				}
			}
		}
	}
	return p_result;
}

int PPSecretSegmentPool::IsThereStorage(const char * pFileName)
{
	int    ok = -1;
	THROW(!isempty(pFileName));
	if(fileExists(pFileName)) {
		SFile f(pFileName, SFile::mRead|SFile::mBinary|SFile::mNoStd);
		uint64 signature = 0;
		if(f.IsValid() && f.Read(&signature, sizeof(signature)) && signature == SecretSegmentPool_Signature) {
			ok = 1;
		}
	}
	CATCHZOK
	return ok;
}

int PPSecretSegmentPool::CreateStorage(const char * pFileName, const char * pMasterPassword, size_t masterPasswordLen)
{
	ZDELETE(P_Vault);
	int   ok = 1;
	bool  is_file_created = false;
	THROW(!isempty(pFileName));
	THROW(!fileExists(pFileName));
	{
		SFile f(pFileName, SFile::mWrite|SFile::mBinary|SFile::mNoStd);
		THROW(f.IsValid());
		is_file_created = true;
		{
			THROW_SL(P_Vault = new SVaultPool());
			THROW(P_Vault->SetupPrimaryPassword(pMasterPassword, masterPasswordLen));
		}
		{
			SSerializeContext sctx;
			SBuffer sbuf;
			THROW_SL(P_Vault->Serialize(+1, sbuf, &sctx));
			THROW_SL(f.Write(&SecretSegmentPool_Signature, sizeof(SecretSegmentPool_Signature)));
			const  uint64 data_size = sbuf.GetAvailableSize();
			THROW_SL(f.Write(&data_size, sizeof(data_size)));
			THROW_SL(f.Write(sbuf.GetBufC(), static_cast<size_t>(data_size)));
		}
	}
	CATCH
		if(is_file_created) {
			SFile::Remove(pFileName);
		}
		ZDELETE(P_Vault);
		ok = 0;
	ENDCATCH
	return ok;
}

int PPSecretSegmentPool::SaveStorage(const char * pFileName) const
{
	int   ok = 1;
	bool  is_file_created = false;
	if(!P_Vault) {
		ok = -1;
	}
	else {
		THROW(!isempty(pFileName));
		if(fileExists(pFileName)) {
			// @todo Создать резервную копию
		}
		{
			SFile f(pFileName, SFile::mWrite|SFile::mBinary|SFile::mNoStd);
			THROW(f.IsValid());
			{
				SSerializeContext sctx;
				SBuffer sbuf;
				THROW_SL(P_Vault->Serialize(+1, sbuf, &sctx));
				THROW_SL(f.Write(&SecretSegmentPool_Signature, sizeof(SecretSegmentPool_Signature)));
				const  uint64 data_size = sbuf.GetAvailableSize();
				THROW_SL(f.Write(&data_size, sizeof(data_size)));
				THROW_SL(f.Write(sbuf.GetBufC(), static_cast<size_t>(data_size)));
			}
		}
	}
	CATCH
		ok = 0;
	ENDCATCH
	return ok;
}

int PPSecretSegmentPool::PutSegmentIntoPool(const PPSecretSegment * pSeg)
{
	int    ok = -1;
	SJson * p_js = 0;
	if(pSeg && !pSeg->IsEmpty()) {
		SString temp_buf;
		THROW(P_Vault); // @todo @errr
		THROW(P_Vault->GetKeyRef()); // @todo @errr
		THROW(p_js = pSeg->ToJsonObj());
		p_js->ToStr(temp_buf);
		{
			uint32   _id = SVaultPool::MakeInternalId(pSeg->InternalID);
			SBinarySet::DeflateStrategy ds(128);
			int    r = P_Vault->PutEncrypted(_id, temp_buf.cptr(), temp_buf.Len(), P_Vault->GetUedSymmCipher(), reinterpret_cast<void *>(P_Vault->GetKeyRef()), &ds);
			THROW(r);
			ok = 1;
		}
	}
	CATCHZOK
	delete p_js;
	return ok;
}

int PPSecretSegmentPool::ReadTestJson(const char * pJsFileName)
{
	/*
		{
		  "version": 2,
		  "generated_at": "2026-06-16T18:28:27",
		  "total_segments": 1500,
		  "note": "Folders (SecType=1) come first in hierarchical order, then secrets in random order. All ParentID values reference existing folder InternalIDs or 0 (root).",
		  "segments": [
			{
			  "InternalID": 1,
			  "ParentID": 0,
			  "SecType": 1,
			  "Name": "Работа",
			  "EnterTm": "2022-08-27T13:35:25"
			},
			{
			  "InternalID": 6,
			  "ParentID": 0,
			  "SecType": 1,
			  "Name": "Документы",
			  "EnterTm": "2026-02-04T12:47:13"
			},
	*/ 
	int    ok = 0;
	SJson * p_js = SJson::ParseFile(pJsFileName);
	if(SJson::IsObject(p_js)) {
		LongArray cidlist;
		for(const SJson * p_cur = p_js->P_Child; p_cur; p_cur = p_cur->P_Next) {
			if(p_cur->Text.IsEqiAscii("segments")) {
				if(SJson::IsArray(p_cur->P_Child)) {
					for(const SJson * p_js_item = p_cur->P_Child->P_Child; p_js_item; p_js_item = p_js_item->P_Next) {
						if(SJson::IsObject(p_js_item)) {
							uint   new_seg_pos = 0;
							PPSecretSegment * p_new_seg = CreateNewSegment(&new_seg_pos);
							if(p_new_seg) {
								if(p_new_seg->FromJsonObj(p_js_item)) {
									if(P_Vault) {
										if(!p_new_seg->InternalID) {
											uint32 new_id = 0;
											THROW_SL(P_Vault->GetChunkIdList(cidlist));
											if(cidlist.getCount()) {
												cidlist.sortAndUndup();
												const  long max_internal_id = smax(cidlist.getLast(), static_cast<long>(SVaultPool::GetMinInternalId()-1));
												new_id = SVaultPool::MakeOuterId(max_internal_id+1);
											}
											else {
												new_id = SVaultPool::MakeOuterId(SVaultPool::GetMinInternalId());
											}
											THROW(new_id); // @todo @err
											p_new_seg->InternalID = new_id;
										}
										THROW(PutSegmentIntoPool(p_new_seg));
									}
									ok = 1;
								}
								else {
									atFree(new_seg_pos);
								}
							}
						}
					}
				}
			}
		}
	}
	CATCHZOK
	delete p_js;
	return ok;
}

int TFacadeWindow::GetSecretsFilePath(SString & rBuf)
{
	rBuf.Z();
	int    ok = 0;
	//PPGetFilePath(PPPATH_DAT)
	DBS.GetDbPath(DBS.GetDbPathID(), rBuf);
	if(rBuf.NotEmpty()) {
		rBuf.SetLastSlash().Cat("sesf").DotCat("bin");
		ok = 1;
	}
	return ok;
}

int TFacadeWindow::LoadSecrets(SString * pFilePath, bool interactive)
{
	int    ok = -1;
	SString file_path;
	if(GetSecretsFilePath(file_path) > 0) {
		char    _password[128];
		_password[0] = 0;
		if(fileExists(file_path)) {
			PasswordDialogParam param;
			param.MinLen = 0;
			param.Flags |= (PasswordDialogParam::fWithoutEncrypt|PasswordDialogParam::fNoConfirmation);
			if(PasswordDialog2(DLG_SECRETPOOLPASSWORD, _password, sizeof(_password), param) > 0) {
				if(!isempty(_password)) {
					if(SecPool.LoadStorage(file_path, _password, sstrlen(_password))) {
						ok = 1;
					}
					else {
						ok = PPError();
					}
				}
			}
		}
		else if(interactive) {
			// Создание нового хранилища
			PasswordDialogParam param;
			param.MinLen = 0;
			param.Flags |= PasswordDialogParam::fWithoutEncrypt;
			if(PasswordDialog2(DLG_SECRETPOOLPASSWORD, _password, sizeof(_password), param) > 0) {
				if(!isempty(_password)) {
					if(SecPool.CreateStorage(file_path, _password, sstrlen(_password))) {
						ok = 2;
					}
					else {
						ok = PPError();
					}
				}
			}
		}
	}
	ASSIGN_PTR(pFilePath, file_path);
	return ok;
}

int TFacadeWindow::CloseSecrets()
{
	int    ok = -1;

	return ok;
}
//
// Descr: [Важно: это - модель. Продуктивный вариант предполагает вставку окна в общую панель centrigo].
//   Диалог просмотра и редактирования секретов.
//
class CentrigoSecretsDialog : public TDialog, public PPListDialogBaseInterface {
public:
	CentrigoSecretsDialog(PPSecretSegmentPool & rSecPool, const SString & rFilePath) : TDialog(DLG_SECRETPOOL), R_SecPool(rSecPool), CurrentSegIdx(0),
		FilePath(rFilePath)
	{
		P_Box = static_cast<SmartListBox *>(getCtrlView(CTL_SECRETPOOL_LIST));
		if(!SetupStrListBox(P_Box))
			PPError();
		updateList(-1);
		{
			SetupStringCombo(this, CTLSEL_SECRETPOOL_TYPE, PPTXT_SECSEGTYPES, 0);
		}
	}
private:
	bool   MakeNewSegName(SString & rBuf) const
	{
		long   _counter = 1;
		SString name_template("New secret");
		rBuf.Z().Cat(name_template);
		while(R_SecPool.SearchSegmentByName(rBuf, 0)) {
			rBuf.Z().Cat(name_template).Space().CatChar('#').CatLongZ(++_counter, 3);
		}
		return true;
	}
	virtual int  addItem(long * pPos, long * pID)
	{
		int    ok = 0;
		uint   new_seg_pos = 0;
		PPSecretSegment * p_new_seg = R_SecPool.CreateNewSegment(&new_seg_pos);
		if(p_new_seg) {
			SString name_buf;
			MakeNewSegName(name_buf);
			R_SecPool.PutText(name_buf, &p_new_seg->NameP);
			p_new_seg->SecType = PPSecretSegment::sectypGeneric;
			{
				if(FilePath.NotEmpty()) {
					// @todo Тут надо перенести сегменты в секретный пул
					if(!R_SecPool.SaveStorage(FilePath))
						PPError();
				}
			}
			ASSIGN_PTR(pPos, static_cast<long>(new_seg_pos));
			ASSIGN_PTR(pID, static_cast<long>(p_new_seg->InternalID));
			ok = 1;
		}
		return ok;
	}
	virtual int  editItem(long pos, long id)
	{
		int    ok = -1;
		return ok;
	}
	virtual int  delItem(long pos, long id)
	{
		int    ok = -1;
		if(pos >= 0 && pos < R_SecPool.getCountI()) {
			// @todo Надо не забыть упаковать пул строк иначе все старые секреты останутся в хранилище, а это - плохо.
			R_SecPool.atFree(pos);
			ok = 1;
		}
		return ok;
	}
	void    ClearInputBlock()
	{
	}
	void    SetupSelectedSegment(uint segIdx/*[0..]*/)
	{
		SString temp_buf;
		if(segIdx < R_SecPool.getCount()) {
			const  PPSecretSegment * p_item = R_SecPool.at(segIdx);
			if(p_item) {
				CurrentSegIdx = segIdx+1;
				R_SecPool.GetText(p_item->NameP, temp_buf);
				setCtrlString(CTL_SECRETPOOL_NAME, temp_buf);
				setCtrlLong(CTL_SECRETPOOL_ID, p_item->InternalID);
				setCtrlLong(CTLSEL_SECRETPOOL_TYPE, p_item->SecType);
				{
					SUniTime_Internal ut;
					UED::_GetRaw_Time(p_item->UedEnterTm, ut);
					setCtrlString(CTL_SECRETPOOL_CRTM, ut.ToStr(DATF_ISO8601CENT, TIMF_HMS, temp_buf));
				}
				{
					SUniTime_Internal ut;
					UED::_GetRaw_Time(p_item->CE.UedBeforeTm, ut);
					LDATE  dt = ZERODATE;
					ut.GetDate(&dt);
					setCtrlDate(CTL_SECRETPOOL_EXPIRY, dt);
				}
				// input CTL_SECRETPOOL_NAME [growfactor: 1 height: 21 margin: 4 tabstop label: "@appellation"] string[128];
				// input CTL_SECRETPOOL_ID [width: 60 height: 21 margin: 4 tabstop readonly fmtf: (nozero) label: "@id"] uint64;
				// combobox CTLSEL_SECRETPOOL_TYPE [width: bycontainer height: 21 margin: 4 tabstop label: "@type" cblinesymb: CTL_SECRETPOOL_TYPE];
				// combobox CTLSEL_SECRETPOOL_PARENT [width: bycontainer height: 21 margin: 4 tabstop label: "Parent" cblinesymb: CTL_SECRETPOOL_PARENT];
				// input CTL_SECRETPOOL_CRTM [width: 80 height: 21 margin: 4 tabstop readonly label: "Creation Time"] string[64];
				// input CTL_SECRETPOOL_EXPIRY [width: 80 height: 21 margin: 4 tabstop label: "Expiry"] date;
				// input CTL_SECRETPOOL_TOPEN [width: bycontainer height: 21 margin: 4 tabstop label: "Open Text"] string[128];
				// input CTL_SECRETPOOL_THIDDEN [width: bycontainer height: 21 margin: 4 tabstop label: "Hidden Text"] string[128];
				// input CTL_SECRETPOOL_TEXPIRY [growfactor: 1 height: 21 margin: 4 tabstop label: "Expiry Text"] string[128];
				// input CTL_SECRETPOOL_TEXT1 [growfactor: 1 height: 21 margin: 4 tabstop label: "Ext 1"] string[128];
				// input CTL_SECRETPOOL_TEXT2 [growfactor: 1 height: 21 margin: 4 tabstop label: "Ext 2"] string[128];
				// input CTL_SECRETPOOL_TEXT3 [growfactor: 1 height: 21 margin: 4 tabstop label: "Ext 3"] string[128];
				// input CTL_SECRETPOOL_DESCR [width: bycontainer growfactor: 1 margin: 4 tabstop multiline wantreturn label: "@memo"] string[252];
			}
		}
	}
	void    GetCurrentInput()
	{
		SString temp_buf;
		if(CurrentSegIdx && CurrentSegIdx <= R_SecPool.getCount()) {
			PPSecretSegment * p_item = R_SecPool.at(CurrentSegIdx-1);
			if(p_item) {
				const  PPSecretSegment preserve_segment(*p_item);
				p_item->SecType = getCtrlLong(CTLSEL_SECRETPOOL_TYPE);
				getCtrlString(CTL_SECRETPOOL_NAME, temp_buf);
				R_SecPool.PutText(temp_buf, &p_item->NameP);
				{
					LDATE  dt = getCtrlDate(CTL_SECRETPOOL_EXPIRY);
					SUniTime_Internal ut;
					ut.SetDate(dt);
					p_item->CE.UedBeforeTm = UED::_SetRaw_Time(UED_META_DATE_DAY, ut);
				}
				//
				if(!p_item->IsEq(preserve_segment)) {
					if(FilePath.NotEmpty()) {
						if(!R_SecPool.SaveStorage(FilePath))
							PPError();
					}
					SString preserve_name;
					R_SecPool.GetText(preserve_segment.NameP, preserve_name);
					if(temp_buf != preserve_name) {
						updateList(-1); // -1 принципиально, поскольку функция возможно была вызвана в ответ на изменение фокус списка.
					}
				}
			}
		}
	}
	DECL_HANDLE_EVENT
	{
		SmartListBox * p_box = P_Box;
		long   p;
		long   i;
		TDialog::handleEvent(event);
		if(TVCOMMAND) {
			switch(TVCMD) {
				case cmLBItemFocused:
					if(event.isCtlEvent(CTL_SECRETPOOL_LIST)) {
						long   pos = 0;
						long   id;
						if(getCurItem(&pos, &id)) {
							if(pos >= 0 && pos < R_SecPool.getCountI()) {
								if(CurrentSegIdx && CurrentSegIdx != (pos+1)) {
									GetCurrentInput();	
								}
								SetupSelectedSegment(pos);
							}
						}
					}
					break;
				case cmaInsert:
					if(p_box) {
						p = i = 0;
						int    r = addItem(&p, &i);
						if(r > 0)
							updateList(p);
					}
					break;
				case cmaDelete:
					if(getCurItem(&p, &i) && delItem(p, i) > 0) {
						updateList(-1);
					}
					break;
				case cmaEdit:
					if(getCurItem(&p, &i) && editItem(p, i) > 0) {
						const bool is_tree_list = (p_box && p_box->IsTreeList());
						const long id = is_tree_list ? i : p;
						if(is_tree_list)
							updateListById(id);
						else
							updateList(id);
					}
					break;
				default:
					return;
			}
		}
		clearEvent(event);
	}
	virtual SmartListBox * GetListBoxCtl() const { return P_Box; }
	virtual int  setupList()
	{
		int    ok = 1;
		if(P_Box) {
			StrAssocArray * p_list = MakeStrAssocList();
			if(p_list) {
				ListBoxDef * p_def = new StdTreeListBoxDef(p_list, lbtDblClkNotify|lbtFocNotify|lbtDisposeData, MKSTYPE(S_ZSTRING, 128));
				P_Box->setDef(p_def);
			}
		}
		return ok;
	}
	int    Helper_MakeStrAssocList(uint32 parentId, StrAssocArray * pList)
	{
		int    ok = -1;
		if(pList) {
			SString temp_buf;
			for(uint i = 0; i < R_SecPool.getCount(); i++) {
				const  PPSecretSegment * p_item = R_SecPool.at(i);
				if(p_item && p_item->ParentID == parentId) {
					if(R_SecPool.GetText(p_item->NameP, temp_buf)) {
						;
					}
					else {
						temp_buf.Z().CatChar('#').CatLongZ(p_item->InternalID, 6);
					}
					pList->Add(p_item->InternalID, p_item->ParentID, temp_buf);
					ok = 1;
					if(p_item->InternalID) {
						Helper_MakeStrAssocList(p_item->InternalID, pList); // @recursion
					}
				}
			}
		}
		return ok;
	}
	StrAssocArray * MakeStrAssocList()
	{
		StrAssocArray * p_result = new StrAssocArray;
		Helper_MakeStrAssocList(0, p_result);
		return p_result;
	}

	PPSecretSegmentPool & R_SecPool;
	const  SString FilePath;
	uint   CurrentSegIdx; // [1..], 0 - undef
	uint   CtlList;
	SmartListBox * P_Box;
};

int TFacadeWindow::DoSecrets()
{
	//PPListDialog aaa;
	int    ok = -1;
	SString file_path;
	CentrigoSecretsDialog * dlg = 0;
	if(LoadSecrets(&file_path, true/*interactive*/)) {
		dlg = new CentrigoSecretsDialog(SecPool, file_path);
		ExecView(dlg);
	}
	delete dlg;
	return ok;
}