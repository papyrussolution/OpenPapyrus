// TPROGRAM_EXP.CPP
// Copyright (c) A.Sobolev 2016, 2018, 2019, 2020
//
#include <slib-internal.h>
#pragma hdrstop

TBaseBrowserWindow * TProgram::FindBrowser(uint resID, int kind, const char * pFileName/*=0*/)
{
	TBaseBrowserWindow * brw = 0;
	HWND   hw = ::GetTopWindow(GetFrameWindow());
	if(hw == H_CloseWnd)
		hw = GetNextWindow(hw, GW_HWNDNEXT);
	if(hw == APPL->H_Desktop)
		hw = GetNextWindow(hw, GW_HWNDNEXT);

	uint   res_id = resID;
	uint   res_offs = 0;
	if(kind == 1) // STimeChunkBrowser
		res_offs = TBaseBrowserWindow::IdBiasTimeChunkBrowser;
	else if(kind == 3) // STextBrowser
		res_offs = TBaseBrowserWindow::IdBiasTextBrowser;
	else if(!res_id) // Browser
		res_offs = TBaseBrowserWindow::IdBiasBrowser;
	if(res_id == 0) {
		if(hw) {
			brw = static_cast<TBaseBrowserWindow *>(TView::GetWindowUserData(hw));
			if(brw)
				res_id = brw->GetResID();
		}
	}
	//if(res_id < TBaseBrowserWindow::IdBiasBrowser)
		res_id += res_offs;
	while(hw) {
		if(hw != APPL->H_Desktop)  {
			brw = static_cast<TBaseBrowserWindow *>(TView::GetWindowUserData(hw));
			if(brw) {
				const long _r = brw->GetResID();
				if(kind == 3 && _r >= TBaseBrowserWindow::IdBiasTextBrowser) {
					if(static_cast<STextBrowser *>(brw)->CmpFileName(pFileName) == 0)
						return static_cast<TBaseBrowserWindow *>(brw);
				}
				else if(_r == res_id /*|| _r == (res_id - res_offs)*/)
					return static_cast<TBaseBrowserWindow *>(brw);
			}
		}
		hw = GetNextWindow(hw, GW_HWNDNEXT);
		if(hw == H_CloseWnd)
			hw = GetNextWindow(hw, GW_HWNDNEXT);
	}
	return 0;
}
