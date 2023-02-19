// SIMAGE.CPP
// Copytight (c) A.Sobolev ..2023
//
#include <slib-internal.h>
#pragma hdrstop

#ifndef __GENERIC_MAIN_CONDUIT__ // {

#define max MAX // @v9.8.11
#define min MIN // @v9.8.11
#include <gdiplus.h>
using namespace Gdiplus;

int SlSession::InitGdiplus()
{
	int    ok = 1;
	if(!GdiplusToken) {
		GdiplusStartupInput gdi_inp;
		ok = BIN(GdiplusStartup(&GdiplusToken, &gdi_inp, NULL) == Ok);
	}
	return ok;
}

void SlSession::ShutdownGdiplus()
{
	if(GdiplusToken) {
		GdiplusShutdown(GdiplusToken);
		GdiplusToken = 0;
	}
}

SImage::SImage() : P_Image(0), ClearColor(RGB(0xD4, 0xD0, 0xC8))
{
}

SImage::~SImage()
{
	delete static_cast<Gdiplus::Image *>(P_Image);
}

int    SImage::Init() { return SLS.InitGdiplus(); }
void   SImage::SetClearColor(COLORREF color) { ClearColor = color; }
bool   SImage::IsValid() const { return LOGIC(P_Image); }
double SImage::GetWidth() { return P_Image ? static_cast<Gdiplus::Image *>(P_Image)->GetWidth() : 0.0; }
double SImage::GetHeight() { return P_Image ? static_cast<Gdiplus::Image *>(P_Image)->GetHeight() : 0.0; }
double SImage::GetHRes() { return P_Image ? static_cast<Gdiplus::Image *>(P_Image)->GetHorizontalResolution() : 0.0; }
double SImage::GetVRes() { return P_Image ? static_cast<Gdiplus::Image *>(P_Image)->GetVerticalResolution() : 0.0; }

int SImage::DrawPartUnchanged(HDC hdc, int offsX, int offsY, const RECT * pImgPart)
{
	int    ok = 1;
	Gdiplus::Image * p_image = static_cast<Gdiplus::Image *>(P_Image);
	if(p_image && pImgPart) {
		Gdiplus::Graphics graph(hdc);
		Gdiplus::REAL w = (Gdiplus::REAL)p_image->GetWidth(), h = (Gdiplus::REAL)p_image->GetHeight();
		if(w && h) {
			RectF rect;
			rect.Width  = (Gdiplus::REAL)pImgPart->right;
			rect.Height = (Gdiplus::REAL)pImgPart->bottom;
			rect.X      = (Gdiplus::REAL)offsX;
			rect.Y      = (Gdiplus::REAL)offsY;
			ok = (graph.DrawImage(p_image, rect, (Gdiplus::REAL)pImgPart->left, (Gdiplus::REAL)pImgPart->top,
				(Gdiplus::REAL)pImgPart->right, (Gdiplus::REAL)pImgPart->bottom, Gdiplus::UnitPixel, 0, 0, 0) == Ok) ? 1 : 0;
		}
	}
	return ok;
}

int SImage::DrawPart(HDC hdc, const RECT * pCliRect, const RECT * pDestRect, const RECT * pImgPart)
{
	int    ok = 1;
	Gdiplus::Image * p_image = static_cast<Gdiplus::Image *>(P_Image);
 	if(p_image && pCliRect && pDestRect && pImgPart) {
		Gdiplus::Graphics graph(hdc);
		Gdiplus::REAL w = (Gdiplus::REAL)p_image->GetWidth(), h = (Gdiplus::REAL)p_image->GetHeight();
		if(w && h) {
			RectF rect, img_part;
			Gdiplus::REAL k = 0, k2 = 0;
			{
				rect.Y      = (Gdiplus::REAL)pCliRect->top;
				rect.X      = (Gdiplus::REAL)pCliRect->left;
				rect.Height = (Gdiplus::REAL)pCliRect->bottom;
				rect.Width  = (Gdiplus::REAL)pCliRect->right;
				k  = rect.Width  / w;
				k2 = rect.Height / h;
				k  = (k <= k2) ? k : k2;
				w = k * p_image->GetWidth();
				h = k * p_image->GetHeight();
			}
			{
				img_part.Y = (Gdiplus::REAL)(pImgPart->top  - (rect.Height  - h) / 2) / k;
				img_part.X = (Gdiplus::REAL)(pImgPart->left - (rect.Width   - w) / 2) / k;
				img_part.Height = (Gdiplus::REAL)(pImgPart->bottom / k);
				img_part.Width  = (Gdiplus::REAL)(pImgPart->right  / k);
			}
			{
				rect.Y      = (Gdiplus::REAL)pDestRect->top;
				rect.X      = (Gdiplus::REAL)pDestRect->left;
				rect.Height = (Gdiplus::REAL)pDestRect->bottom;
				rect.Width  = (Gdiplus::REAL)pDestRect->right;
			}
			ok = (graph.DrawImage(p_image, rect, (Gdiplus::REAL)img_part.X, (Gdiplus::REAL)img_part.Y,
				(Gdiplus::REAL)img_part.Width, (Gdiplus::REAL)img_part.Height, Gdiplus::UnitPixel, NULL, NULL, NULL) == Ok) ? 1 : 0;
		}
	}
	return ok;
}

int SImage::Draw(HDC hdc, RECT * pRect, int clear, int use2Koeff)
{
	int    ok = 1;
	Gdiplus::Image * p_image = static_cast<Gdiplus::Image *>(P_Image);
	RECT   cl_rect;
	Gdiplus::RectF  rect;
	Gdiplus::Color  c(255, 255, 255, 255);
	Gdiplus::Pen    pen(c, 2);
	Gdiplus::Graphics graph(hdc);
	cl_rect = *pRect;
	rect.X      = (Gdiplus::REAL)cl_rect.left;
	rect.Y      = (Gdiplus::REAL)cl_rect.top;
	rect.Width  = (Gdiplus::REAL)cl_rect.right;
	rect.Height = (Gdiplus::REAL)cl_rect.bottom;
	if(clear) {
		Gdiplus::Color c2(255, GetRValue(ClearColor), GetGValue(ClearColor), GetBValue(ClearColor));
		graph.Clear(c2);
	}
	graph.DrawRectangle(&pen, rect);
	if(p_image) {
		Gdiplus::REAL w = (Gdiplus::REAL)p_image->GetWidth(), h = (Gdiplus::REAL)p_image->GetHeight();
		if(w && h) {
			Gdiplus::REAL k = 0, k2 = 0;
			rect.X      += 2;
			rect.Y      += 2;
			rect.Width  -= 4;
			rect.Height -= 4;
			k  = rect.Width  / w;
			k2 = rect.Height / h;
			if(!use2Koeff) {
				k  = (k <= k2) ? k : k2;
				k2 = k;
			}
			w = k * p_image->GetWidth();
			h = k2 * p_image->GetHeight();
			rect.X += (rect.Width - w)  / 2;
			rect.Y += (rect.Height - h) / 2;
			rect.Width  = w;
			rect.Height = h;
			ok = BIN(graph.DrawImage(p_image, rect) == Ok);
		}
	}
	return ok;
}

int SImage::Draw(HWND hWnd, RECT * pRect, int clear, int use2Koeff)
{
	int    ok = 1;
	RECT   r;
	HDC    hdc = GetDC(hWnd);
	if(!RVALUEPTR(r, pRect))
		::GetClientRect(hWnd, &r);
	ok = Draw(hdc, &r, clear, use2Koeff);
	ReleaseDC(hWnd, hdc);
	return ok;
}

int SImage::Draw(HWND hWnd, const char * pPicPath, RECT * pRect, int clear, int use2Koeff)
{
	int    ok = 0;
	if(hWnd && pPicPath && fileExists(pPicPath)) {
		Load(pPicPath);
		ok = Draw(hWnd, pRect, clear, use2Koeff);
	}
	return ok;
}

int SImage::Load(const char * pPicPath)
{
	int    ok = 0;
	delete static_cast<Image *>(P_Image);
	P_Image = 0;
	FileName = 0;
	if(pPicPath && fileExists(pPicPath)) {
		OLECHAR wstr[MAXPATH];
		MultiByteToWideChar(1251, MB_PRECOMPOSED, pPicPath, -1, wstr, SIZEOFARRAY(wstr) - 1);
		P_Image = new Image(wstr);
		FileName.CopyFrom(pPicPath);
		ok = 1;
	}
	return ok;
}

int SImage::LoadThumbnailImage(const char * pPicPath, int width, int height)
{
	int    ok = 0;
	delete static_cast<Gdiplus::Image *>(P_Image);
	P_Image = 0;
	FileName = 0;
	if(pPicPath && fileExists(pPicPath)) {
		Gdiplus::Image * p_img = 0;
		OLECHAR wstr[MAXPATH];
		MultiByteToWideChar(1251, MB_PRECOMPOSED, pPicPath, -1, wstr, SIZEOFARRAY(wstr) - 1);
		p_img = new Gdiplus::Image(wstr);
		P_Image = p_img->GetThumbnailImage(width, height, 0, 0);
		ZDELETE(p_img);
		FileName.CopyFrom(pPicPath);
		ok = 1;
	}
	return ok;
}

SString & SImage::GetFileName(SString & rBuf)
{
	return (rBuf = FileName);
}

int SImage::InsertBitmap(HWND hwnd, const char * pPath, COLORREF bkgnd)
{
	int    ok = 0;
	Load(pPath);
	if(P_Image) {
		UINT new_width = 0, new_height = 0;
		Gdiplus::Image * p_image = static_cast<Gdiplus::Image *>(P_Image);
		UINT height   = p_image->GetHeight();
		UINT width    = p_image->GetWidth();
		RECT cli_rect;
		Gdiplus::REAL k = 0, k2 = 0;

		::GetClientRect(hwnd, &cli_rect);
		new_width  = cli_rect.right  - cli_rect.left;
		new_height = cli_rect.bottom - cli_rect.top;
		k  = (Gdiplus::REAL)fdivui(new_width, width);
		k2 = (Gdiplus::REAL)fdivui(new_height, height);
		k  = (k <= k2) ? k : k2;
        new_height = (UINT)(static_cast<Gdiplus::REAL>(height) * k);
		new_width  = (UINT)(static_cast<Gdiplus::REAL>(width) * k);
		{
			HBITMAP hbmp = 0;
			Gdiplus::Bitmap * p_sized_img = static_cast<Gdiplus::Bitmap *>(p_image)->Clone(0, 0, new_width, new_height, p_image->GetPixelFormat());
			ARGB argb = Color::MakeARGB(0, GetRValue(bkgnd), GetGValue(bkgnd), GetBValue(bkgnd));
			Gdiplus::Color color(argb);

			static_cast<Gdiplus::Bitmap *>(p_sized_img)->GetHBITMAP(color, &hbmp);
			delete static_cast<Gdiplus::Image *>(P_Image);
			P_Image = p_sized_img;
			::SendMessageW(hwnd, BM_SETIMAGE, IMAGE_BITMAP, reinterpret_cast<LPARAM>(hbmp));
		}
		ok = 1;
	}
	return ok;
}

int FASTCALL GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
	int   idx = -1;
	uint  num = 0;          // number of image encoders
	uint  size = 0;         // size of the image encoder array in bytes
	Gdiplus::ImageCodecInfo * p_img_codec_info = 0;
	GetImageEncodersSize(&num, &size);
	THROW(size);
	THROW(p_img_codec_info = static_cast<Gdiplus::ImageCodecInfo *>(SAlloc::M(size)));
	GetImageEncoders(num, size, p_img_codec_info);
	for(uint j = 0; idx < 0 && j < num; ++j) {
		if(wcscmp(p_img_codec_info[j].MimeType, format) == 0) {
			*pClsid = p_img_codec_info[j].Clsid;
			idx = (int)j;  // Success
		}
	}
	CATCH
		idx = -1;
	ENDCATCH
	SAlloc::F(p_img_codec_info);
	return idx;  // Failure
}

int SClipboard::CopyPaste(HWND hWnd, int copy, const char * pPath)
{
	Gdiplus::Bitmap * p_bmp = 0;
	OLECHAR wstr[MAXPATH];
	HBITMAP h_bmp = 0;
	if(::OpenClipboard(hWnd)) {
		if(copy) {
			::EmptyClipboard();
			MultiByteToWideChar(1251, MB_PRECOMPOSED, pPath, -1, wstr, SIZEOFARRAY(wstr) - 1);
			p_bmp = new Gdiplus::Bitmap(wstr);
			p_bmp->GetHBITMAP(0, &h_bmp);
			SetClipboardData(CF_BITMAP, h_bmp);
			CloseClipboard();
		}
		else if(IsClipboardFormatAvailable(CF_DIB)) {
			h_bmp = static_cast<HBITMAP>(::GetClipboardData(CF_BITMAP));
			p_bmp = new Gdiplus::Bitmap(h_bmp, 0);
			CloseClipboard();
			CLSID enc_clsid;
			GetEncoderClsid(L"image/jpeg", &enc_clsid);
			MultiByteToWideChar(1251, MB_PRECOMPOSED, pPath, -1, wstr, SIZEOFARRAY(wstr) - 1);
			p_bmp->Save(wstr, &enc_clsid, 0);
		}
		else
			CloseClipboard();
	}
	ZDELETE(p_bmp);
	return 1;
}

#endif // } __GENERIC_MAIN_CONDUIT__ /
