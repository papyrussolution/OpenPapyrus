// SCLIPBOARD.CPP
// Copyright (c) A.Sobolev 2019
//
#include <slib.h>
#include <tv.h>
#pragma hdrstop

//static 
int FASTCALL SClipboard::OpenClipboardRetry(void * hWnd)
{
	int    ok = 0;
	for(uint attempt = 0; !ok && attempt < 8; attempt++) {
		if(attempt)
			SDelay(1 << (attempt-1));
		if(::OpenClipboard(static_cast<HWND>(hWnd)))
			ok = 1;
	}
	return ok;
}

//static 
int FASTCALL SClipboard::Helper_OpenClipboardForCopy(int & rHasBeenOpened)
{
	int    ok = 1;
	rHasBeenOpened = 0;
	THROW_S(OpenClipboardRetry(APPL->H_MainWnd), SLERR_WINDOWS);
	rHasBeenOpened = 1;
	THROW_S(::EmptyClipboard(), SLERR_WINDOWS);
	CATCHZOK
	return ok;
}

//static 
int FASTCALL SClipboard::Helper_CloseClipboard(int hasBeenOpened)
{
	if(hasBeenOpened)
		::CloseClipboard();
	return 1;
}

//static 
int FASTCALL SClipboard::Copy_Text(const char * pText, size_t len)
{
	int    ok = 1;
	int    cb_has_been_openen = 0;
	THROW(Helper_OpenClipboardForCopy(cb_has_been_openen));
	{
		HGLOBAL h_glb = ::GlobalAlloc(GMEM_MOVEABLE, sizeof(char) * (len+1));
		THROW_S(h_glb, SLERR_WINDOWS);
		char * p_buf = static_cast<char *>(::GlobalLock(h_glb));
		THROW_S(p_buf, SLERR_WINDOWS);
		memcpy(p_buf, pText, sizeof(char) * len);
		p_buf[len] = 0;
		::GlobalUnlock(h_glb);
		::SetClipboardData(CF_TEXT, h_glb);
	}
	CATCHZOK
	Helper_CloseClipboard(cb_has_been_openen);
	return ok;
}

//static 
int FASTCALL SClipboard::Copy_TextUnicode(const wchar_t * pText, size_t len)
{
	int    ok = 1;
	int    cb_has_been_openen = 0;
	THROW(Helper_OpenClipboardForCopy(cb_has_been_openen));
	{
		HGLOBAL h_glb = ::GlobalAlloc(GMEM_MOVEABLE, sizeof(wchar_t) * (len+1));
		THROW_S(h_glb, SLERR_WINDOWS);
		wchar_t * p_buf = static_cast<wchar_t *>(::GlobalLock(h_glb));
		THROW_S(p_buf, SLERR_WINDOWS);
		memcpy(p_buf, pText, sizeof(wchar_t) * len);
		p_buf[len] = 0;
		::GlobalUnlock(h_glb);
		::SetClipboardData(CF_UNICODETEXT, h_glb);
	}
	CATCHZOK
	Helper_CloseClipboard(cb_has_been_openen);
	return ok;
}

//static
int FASTCALL SClipboard::Copy_SYLK(const SString & rText)
{
	int    ok = 1;
	int    cb_has_been_openen = 0;
	if(rText.Len()) {
		THROW(Helper_OpenClipboardForCopy(cb_has_been_openen));
		{
			HGLOBAL h_glb = ::GlobalAlloc(GMEM_MOVEABLE, (rText.Len() + 1));
			THROW_S(h_glb, SLERR_WINDOWS);
			char * p_buf = static_cast<char *>(::GlobalLock(h_glb));
			THROW_S(p_buf, SLERR_WINDOWS);
			rText.CopyTo(p_buf, 0);
			::GlobalUnlock(h_glb);
			::SetClipboardData(CF_SYLK, h_glb);
		}
	}
	CATCHZOK
	Helper_CloseClipboard(cb_has_been_openen);
	return ok;
}

//static 
int FASTCALL SClipboard::Past_Text(SStringU & rBuf)
{
	rBuf.Z();
	int    ok = -1;
	int    cb_has_been_openen = 0;
	SString symb;
	THROW_S(OpenClipboardRetry(0), SLERR_WINDOWS);
	cb_has_been_openen = 1;
	{
		enum {
			hasUnicode = 0x0001,
			hasANSI    = 0x0002,
			hasOEM     = 0x0004
		};
		uint   has_formats = 0;
		for(uint cf = ::EnumClipboardFormats(0); cf; cf = ::EnumClipboardFormats(cf)) {
			switch(cf) {
				case CF_UNICODETEXT: has_formats |= hasUnicode; break;
				case CF_TEXT: has_formats |= hasANSI; break;
				case CF_OEMTEXT: has_formats |= hasOEM; break;
			}
		}
		if(has_formats & (hasUnicode|hasANSI|hasOEM)) {
			if(has_formats & hasUnicode) {
				HANDLE h_cb = ::GetClipboardData(CF_UNICODETEXT);
				if(h_cb) {
					void * p_data = ::GlobalLock(h_cb); // @unicodeproblem
					if(p_data) {
						//(symb = p_str).Transf(CTRANSF_OUTER_TO_INNER); // @unicodeproblem
						rBuf = static_cast<wchar_t *>(p_data);
						::GlobalUnlock(h_cb);
						ok = 1;
					}
				}
			}
			else if(has_formats & hasANSI) {
				HANDLE h_cb = ::GetClipboardData(CF_UNICODETEXT);
				if(h_cb) {
					void * p_data = ::GlobalLock(h_cb);
					if(p_data) {
						SString & r_temp_buf = SLS.AcquireRvlStr();
						r_temp_buf = static_cast<char *>(p_data);
						rBuf.CopyFromMb(cpANSI, r_temp_buf, r_temp_buf.Len());
						::GlobalUnlock(h_cb);
						ok = 1;
					}
				}
			}
			else if(has_formats & hasOEM) {
				HANDLE h_cb = ::GetClipboardData(CF_UNICODETEXT);
				if(h_cb) {
					void * p_data = ::GlobalLock(h_cb);
					if(p_data) {
						SString & r_temp_buf = SLS.AcquireRvlStr();
						r_temp_buf = static_cast<char *>(p_data);
						rBuf.CopyFromMb(cpOEM, r_temp_buf, r_temp_buf.Len());
						::GlobalUnlock(h_cb);
						ok = 1;
					}
				}
			}
		}
	}
	CATCHZOK
	if(cb_has_been_openen)
		::CloseClipboard();
	return ok;
}
