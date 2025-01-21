// PDFVIEW.CPP
// Copyright (c) A.Sobolev 2024, 2025
// @codepage UTF-8
// @construction
// Экспериментальный модуль для отображения pdf-документов
//
#include <pp.h>
#pragma hdrstop
#include <mupdf/fitz.h>
//#include <mupdf/pdf.h>

class PdfView : public TWindowBase {
public:
	PdfView() : TWindowBase(L"PapyrusPdfView", 0)
	{
	}
private:
	DECL_HANDLE_EVENT
	{
		TWindowBase::handleEvent(event);
		if(TVKEYDOWN) {
			//switch(TVKEY) {
			//}
		}
		else if(TVINFOPTR) {
			if(event.isCmd(cmInit)) {
				CreateBlock * p_blk = static_cast<CreateBlock *>(TVINFOPTR);
				//W.SetArea(getClientRect());
				//{
				//	IntRange rx, ry;
					//W.GetScrollRange(&rx, &ry);
					//ScrlB.SetRangeX(rx);
					//ScrlB.SetRangeY(ry);
				//}
				//ScrlB.SetupWindow(H());
			}
			else if(event.isCmd(cmPaint)) {
				PaintEvent * p_blk = static_cast<PaintEvent *>(TVINFOPTR);
				//CreateFont_();
				if(oneof2(p_blk->PaintType, PaintEvent::tPaint, PaintEvent::tEraseBackground)) {
					/*
					if(GetWbCapability() & wbcDrawBuffer) {
						// Если используется буферизованная отрисовка, то фон нужно перерисовать в любом случае а на событие PaintEvent::tEraseBackground
						// не реагировать
						if(p_blk->PaintType == PaintEvent::tPaint) {
							SPaintToolBox * p_tb = APPL->GetUiToolBox();
							if(p_tb) {
								TCanvas2 canv(*p_tb, static_cast<HDC>(p_blk->H_DeviceContext));
								canv.Rect(p_blk->Rect, 0, TProgram::tbiListBkgBrush);
								DrawLayout(canv, P_Lfc);
							}
						}
					}
					else {
						SPaintToolBox * p_tb = APPL->GetUiToolBox();
						if(p_tb) {
							TCanvas2 canv(*p_tb, static_cast<HDC>(p_blk->H_DeviceContext));
							if(p_blk->PaintType == PaintEvent::tEraseBackground)
								canv.Rect(p_blk->Rect, 0, TProgram::tbiListBkgBrush);
							if(p_blk->PaintType == PaintEvent::tPaint)
								DrawLayout(canv, P_Lfc);
						}
					}
					*/
					clearEvent(event);
				}
			}
		}
	}
	struct PdfApp {
		PdfApp()
		{
			THISZERO();
			ctx = fz_new_context(NULL, NULL, FZ_STORE_DEFAULT);
			scrw = 640;
			scrh = 480;
			resolution = 72;
			layout_w = FZ_DEFAULT_LAYOUT_W;
			layout_h = FZ_DEFAULT_LAYOUT_H;
			layout_em = FZ_DEFAULT_LAYOUT_EM;
			layout_css = NULL;
			layout_use_doc_css = 1;
			transition.duration = 0.25f;
			transition.type = FZ_TRANSITION_FADE;
		#ifdef _WIN32
			colorspace = fz_device_bgr(ctx);
		#else
			colorspace = fz_device_rgb(ctx);
		#endif
			tint_white = 0xFFFAF0;
			useicc = 1;
			useseparations = 0;
			aalevel = 8;
		}
		~PdfApp()
		{
			fz_drop_context(ctx);
		}
		// current document params 
		fz_document * doc;
		char * docpath;
		char * doctitle;
		fz_outline *outline;
		int    outline_deferred;
		float  layout_w;
		float  layout_h;
		float  layout_em;
		char * layout_css;
		int    layout_use_doc_css;
		int    pagecount;
		// current view params
		float  default_resolution;
		float  resolution;
		int    rotate;
		fz_pixmap * image;
		int    imgw;
		int    imgh;
		int    grayscale;
		fz_colorspace * colorspace;
		int    invert;
		int    tint;
		int    tint_white;
		int    useicc;
		int    useseparations;
		int    aalevel;
		// presentation mode 
		int    presentation_mode;
		int    transitions_enabled;
		fz_pixmap *old_image;
		fz_pixmap *new_image;
		clock_t start_time;
		int    in_transit;
		float  duration;
		fz_transition transition;
		/* current page params */
		int    pageno;
		fz_page * page;
		fz_rect page_bbox;
		fz_display_list * page_list;
		fz_display_list * annotations_list;
		fz_stext_page * page_text;
		fz_link * page_links;
		int    errored;
		int    incomplete;
		// separations 
		fz_separations * seps;
		// snapback history 
		int    hist[256];
		int    histlen;
		int    marks[10];
		// window system sizes 
		int    winw;
		int    winh;
		int    scrw;
		int    scrh;
		int    shrinkwrap;
		int    fullscreen;
		/* event handling state */
		char   number[256];
		int    numberlen;
		int    ispanning;
		int    panx;
		int    pany;
		int    iscopying;
		int    selx;
		int    sely;
		/* TODO - While sely keeps track of the relative change in
		 * cursor position between two ticks/events, beyondy shall keep
		 * track of the relative change in cursor position from the
		 * point where the user hits a scrolling limit. This is ugly.
		 * Used in pdfapp.c:pdfapp_onmouse.
		 */
		int    beyondy;
		fz_rect selr;
		int    nowaitcursor;
		/* search state */
		int    issearching;
		int    searchdir;
		char   search[512];
		int    searchpage;
		fz_quad hit_bbox[512];
		int    hit_count;
		/* client context storage */
		void * userdata;
		fz_context * ctx;
	//#ifdef HAVE_CURL
		fz_stream * stream;
	//#endif
	};

	PdfApp Pb;
	SPaintToolBox Tb;
};

class PdfBrowser : public TBaseBrowserWindow {
public:
	static int RegWindowClass(HINSTANCE hInst);
	static LPCTSTR WndClsName;
	PdfBrowser();
	~PdfBrowser();
private:
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	int    WMHCreate();

};

/*static*/LPCTSTR PdfBrowser::WndClsName = _T("SPdfBrowser"); // @global

/*static*/int PdfBrowser::RegWindowClass(HINSTANCE hInst)
{
	WNDCLASSEX wc;
	INITWINAPISTRUCT(wc);
	wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC | CS_DBLCLKS;
	wc.lpfnWndProc   = PdfBrowser::WndProc;
	wc.cbClsExtra    = BRWCLASS_CEXTRA;
	wc.cbWndExtra    = BRWCLASS_WEXTRA;
	wc.hInstance     = hInst;
	wc.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(/*ICON_TIMEGRID*/172));
	wc.hCursor       = NULL; // LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = ::CreateSolidBrush(RGB(0xEE, 0xEE, 0xEE));
	wc.lpszClassName = PdfBrowser::WndClsName;
#if !defined(_PPDLL) && !defined(_PPSERVER)
	//Scintilla_RegisterClasses(hInst);
#endif
	return RegisterClassEx(&wc);
}

PdfBrowser::PdfBrowser() : TBaseBrowserWindow(WndClsName)
{
}
	
PdfBrowser::~PdfBrowser()
{
}

int PdfBrowser::WMHCreate()
{
	RECT rc;
	GetWindowRect(H(), &rc);
	P_Toolbar = new TToolbar(H(), TBS_NOMOVE);
	if(P_Toolbar && LoadToolbarResource(ToolbarID) > 0) {
		P_Toolbar->Init(ToolbarID, &ToolbarL);
		if(P_Toolbar->IsValid()) {
			RECT tbr;
			::GetWindowRect(P_Toolbar->H(), &tbr);
			ToolBarWidth = tbr.bottom - tbr.top;
		}
	}
	//HwndSci = ::CreateWindowEx(WS_EX_CLIENTEDGE, _T("Scintilla"), _T(""), WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_CLIPCHILDREN,
	//	0, ToolBarWidth, rc.right - rc.left, rc.bottom - rc.top, H(), 0/*(HMENU)GuiID*/, APPL->GetInst(), 0);
	//SScEditorBase::Init(HwndSci, 1/*preserveFileName*/);
	//TView::SetWindowProp(HwndSci, GWLP_USERDATA, this);
	//OrgScintillaWndProc = static_cast<WNDPROC>(TView::SetWindowProp(HwndSci, GWLP_WNDPROC, ScintillaWindowProc));
	// @v8.6.2 (SCI_SETKEYSUNICODE deprecated in sci 3.5.5) CallFunc(SCI_SETKEYSUNICODE, 1, 0);
	//CallFunc(SCI_SETCARETLINEVISIBLE, 1);
	//CallFunc(SCI_SETCARETLINEBACK, RGB(232,232,255));
	//CallFunc(SCI_SETSELBACK, 1, RGB(117,217,117));
	//CallFunc(SCI_SETFONTQUALITY, SC_EFF_QUALITY_LCD_OPTIMIZED); // @v9.8.2 SC_EFF_QUALITY_ANTIALIASED-->SC_EFF_QUALITY_LCD_OPTIMIZED
	// CallFunc(SCI_SETTECHNOLOGY, /*SC_TECHNOLOGY_DIRECTWRITERETAIN*/SC_TECHNOLOGY_DIRECTWRITEDC, 0); // @v9.8.2
	//
	//CallFunc(SCI_SETMOUSEDWELLTIME, 500);
	//CallFunc(SCI_SETMARGINSENSITIVEN, scmargeFolder, true); // @v11.1.12
	//
	{
		/*
		KeyAccel.clear();
		{
			for(uint i = 0; i < OuterKeyAccel.getCount(); i++) {
				const LAssoc & r_accel_item = OuterKeyAccel.at(i);
				const KeyDownCommand & r_k = *reinterpret_cast<const KeyDownCommand *>(&r_accel_item.Key);
				KeyAccel.Set(r_k, r_accel_item.Val);
			}
		}
		*/
		if(P_Toolbar) {
			const uint tbc = P_Toolbar->getItemsCount();
			for(uint i = 0; i < tbc; i++) {
				const ToolbarItem & r_tbi = P_Toolbar->getItem(i);
				if(!(r_tbi.Flags & r_tbi.fHidden) && r_tbi.KeyCode && r_tbi.KeyCode != TV_MENUSEPARATOR && r_tbi.Cmd) {
					KeyDownCommand k;
					if(k.SetTvKeyCode(r_tbi.KeyCode)) {
						//KeyAccel.Set(k, r_tbi.Cmd);
					}
				}
			}
		}
		//KeyAccel.Sort();
	}
	//FileLoad(Doc.FileName, cpUTF8, 0);
	//return BIN(P_SciFn && P_SciPtr);
	return 1;
}

/*static*/LRESULT CALLBACK PdfBrowser::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	CREATESTRUCT * p_init_data;
	PdfBrowser * p_view = 0;
	switch(message) {
		case WM_CREATE:
			p_view = static_cast<PdfBrowser *>(Helper_InitCreation(lParam, (void **)&p_init_data));
			if(p_view) {
				p_view->HW = hWnd;
				TView::SetWindowProp(hWnd, GWLP_USERDATA, p_view);
				::SetFocus(hWnd);
				::SendMessage(hWnd, WM_NCACTIVATE, TRUE, 0L);
				p_view->WMHCreate();
				PostMessage(hWnd, WM_PAINT, 0, 0);
				{
					SString temp_buf;
					TView::SGetWindowText(hWnd, temp_buf);
					APPL->AddItemToMenu(temp_buf, p_view);
				}
				//::SetFocus(p_view->HwndSci);
				return 0;
			}
			else
				return -1;
		case WM_COMMAND:
			{
				p_view = static_cast<PdfBrowser *>(TView::GetWindowUserData(hWnd));
				if(p_view) {
					if(HIWORD(wParam) == 0) {
						/*if(p_view->KeyAccel.getCount()) {
							long   cmd = 0;
							KeyDownCommand k;
							k.SetTvKeyCode(LOWORD(wParam));
							if(p_view->KeyAccel.BSearch(*reinterpret_cast<const long *>(&k), &cmd, 0)) {
								//p_view->ProcessCommand(cmd, 0, p_view);
							}
						}*/
					}
					/*
					if(LOWORD(wParam))
						p_view->ProcessCommand(LOWORD(wParam), 0, p_view);
					*/
				}
			}
			break;
		case WM_DESTROY:
			p_view = static_cast<PdfBrowser *>(TView::GetWindowUserData(hWnd));
			if(p_view) {
				//p_view->SaveChanges();
				TWindowBase::Helper_Finalize(hWnd, p_view);
			}
			return 0;
		case WM_SETFOCUS:
			if(!(TView::SGetWindowStyle(hWnd) & WS_CAPTION)) {
				SetWindowPos(hWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);
				APPL->NotifyFrame(0);
			}
			p_view = static_cast<PdfBrowser *>(TView::GetWindowUserData(hWnd));
			if(p_view) {
				//::SetFocus(p_view->HwndSci);
				APPL->SelectTabItem(p_view);
				TView::messageBroadcast(p_view, cmReceivedFocus);
				p_view->select();
			}
			break;
		case WM_KILLFOCUS:
			if(!(TView::SGetWindowStyle(hWnd) & WS_CAPTION))
				APPL->NotifyFrame(0);
			p_view = static_cast<PdfBrowser *>(TView::GetWindowUserData(hWnd));
			if(p_view) {
				TView::messageBroadcast(p_view, cmReleasedFocus);
				p_view->ResetOwnerCurrent();
			}
			break;
		case WM_KEYDOWN:
			if(wParam == VK_ESCAPE) {
				p_view = static_cast<PdfBrowser *>(TView::GetWindowUserData(hWnd));
				if(p_view) {
					p_view->endModal(cmCancel);
					return 0;
				}
			}
			else if(wParam == VK_TAB) {
				p_view = static_cast<PdfBrowser *>(TView::GetWindowUserData(hWnd));
				if(p_view && GetKeyState(VK_CONTROL) & 0x8000 && !p_view->IsInState(sfModal)) {
					SetFocus(GetNextBrowser(hWnd, (GetKeyState(VK_SHIFT) & 0x8000) ? 0 : 1));
					return 0;
				}
			}
			return 0;
		case WM_SIZE:
			p_view = static_cast<PdfBrowser *>(TView::GetWindowUserData(hWnd));
			if(lParam && p_view) {
				HWND hw = p_view->P_Toolbar ? p_view->P_Toolbar->H() : 0;
				if(IsWindowVisible(hw)) {
					MoveWindow(hw, 0, 0, LOWORD(lParam), p_view->ToolBarWidth, 0);
					TView::messageCommand(p_view, cmResize);
				}
				//p_view->Resize();
			}
			break;
		case WM_NOTIFY:
			{
				p_view = static_cast<PdfBrowser *>(TView::GetWindowUserData(hWnd));
				if(p_view) {
					;
				}
			}
			break;
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}
