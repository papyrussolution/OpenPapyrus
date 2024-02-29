// IMGUI-SUPPORT.CPP
// Copyright (c) A.Sobolev 2024
//
#include <pp.h>
#pragma hdrstop
#include <imgui-support.h>
#include <Scintilla.h>

ImGuiRuntimeBlock::ImGuiRuntimeBlock() : g_pd3dDevice(0), g_pd3dDeviceContext(0), g_pSwapChain(0), g_mainRenderTargetView(0)
{
}

bool ImGuiRuntimeBlock::CreateDeviceD3D(HWND hWnd)
{
	// Setup swap chain
	DXGI_SWAP_CHAIN_DESC sd;
	// @sobolev ZeroMemory(&sd, sizeof(sd));
	MEMSZERO(sd); // @sobolev
	sd.BufferCount = 2;
	sd.BufferDesc.Width = 0;
	sd.BufferDesc.Height = 0;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = hWnd;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;
	sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	UINT createDeviceFlags = 0;
	//createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
	D3D_FEATURE_LEVEL featureLevel;
	const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
	HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray,
		2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
	if(res == DXGI_ERROR_UNSUPPORTED) // Try high-performance WARP software driver if hardware is not available.
		res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray,
			2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
	if(res != S_OK)
		return false;
	CreateRenderTarget();
	return true;
}

void ImGuiRuntimeBlock::CreateRenderTarget()
{
	ID3D11Texture2D * p_back_buffer;
	g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&p_back_buffer));
	g_pd3dDevice->CreateRenderTargetView(p_back_buffer, nullptr, &g_mainRenderTargetView);
	p_back_buffer->Release();
}

void ImGuiRuntimeBlock::CleanupRenderTarget()
{
	SCOMOBJRELEASE(g_mainRenderTargetView); 
}

void ImGuiRuntimeBlock::CleanupDeviceD3D()
{
	CleanupRenderTarget();
	SCOMOBJRELEASE(g_pSwapChain); 
	SCOMOBJRELEASE(g_pd3dDeviceContext); 
	SCOMOBJRELEASE(g_pd3dDevice); 
}

void * ImGuiRuntimeBlock::LoadTexture(const char * pFileName)
{
	void * p_result = 0;
	ID3D11Resource * p_texture = 0;
	ID3D11ShaderResourceView * p_texture_view = 0;
	SStringU & r_fn = SLS.AcquireRvlStrU();
	r_fn.CopyFromUtf8R(pFileName, sstrlen(pFileName), 0);
	HRESULT hr = DirectX::CreateWICTextureFromFile(g_pd3dDevice, g_pd3dDeviceContext, _In_z_ r_fn.ucptr(), &p_texture, &p_texture_view, /*maxsize*/0);
	if(SUCCEEDED(hr)) {
		//p_result = p_texture;
		p_result = p_texture_view;
	}
	return p_result;
}

void * ImGuiRuntimeBlock::MakeIconTexture(uint iconId) // @v11.9.2
{
	void * p_result = 0;
	SPaintToolBox & r_tb = DS.GetUiToolBox();
	const TWhatmanToolArray & r_vt = DS.GetVectorTools();
	TWhatmanToolArray::Item vt_item;
	const SDrawFigure * p_fig = r_vt.GetFigById(1, iconId, &vt_item);
	if(p_fig) {
		SBuffer bmp_buf;
		if(TCanvas2::TransformDrawFigureToBitmap(&r_tb, p_fig, SPoint2S(ImGuiRuntimeBlock::IconSize), vt_item.ReplacedColor, SColor(0, 0, 0, 0), bmp_buf)) {
			ID3D11Resource * p_texture = 0;
			ID3D11ShaderResourceView * p_texture_view = 0;
			HRESULT hr = DirectX::CreateWICTextureFromMemory(g_pd3dDevice, g_pd3dDeviceContext, 
				reinterpret_cast<const uint8 *>(bmp_buf.GetBufC()), bmp_buf.GetAvailableSize(), &p_texture, &p_texture_view, /*maxsize*/0);
			if(SUCCEEDED(hr)) {
				p_result = p_texture_view;
			}
		}
	}
	return p_result;
}

void ImGuiRuntimeBlock::ReleaseTexture(void * pTextureView)
{
	if(pTextureView) {
		static_cast<IUnknown *>(pTextureView)->Release();
	}
}

void ImGuiRuntimeBlock::OnWindowResize(WPARAM wParam, LPARAM lParam)
{
	if(g_pd3dDevice && wParam != SIZE_MINIMIZED) {
		CleanupRenderTarget();
		g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
		CreateRenderTarget();
	}
}
//
//
//
ImGuiObjStack::ImGuiObjStack()
{
}
	
ImGuiObjStack::~ImGuiObjStack()
{
	int obj_type = 0;
	while(St.pop(obj_type)) {
		switch(obj_type) {
			case objFont: ImGui::PopFont(); break;
			case objStyleColor: ImGui::PopStyleColor(); break;
			case objStyleVar: ImGui::PopStyleVar(); break;
			case objTabStop: ImGui::PopTabStop(); break;
			case objButtonRepeat: ImGui::PopButtonRepeat(); break;
			case objItemWidth: ImGui::PopItemWidth(); break;
			case objTextWrapPos: ImGui::PopTextWrapPos(); break;
			case objID: ImGui::PopID(); break;
			case objAllowKeyboardFocus: ImGui::PopAllowKeyboardFocus(); break;
		}
	}
}
	
void FASTCALL ImGuiObjStack::PushFont(ImFont * pFont)
{
	if(pFont) {
		ImGui::PushFont(pFont);
		St.push(objFont);
	}
}

void ImGuiObjStack::PushStyleColor(ImGuiCol idx, ImU32 col)
{
	ImGui::PushStyleColor(idx, col);
	St.push(objStyleColor);
}

void ImGuiObjStack::PushStyleColor(ImGuiCol idx, const ImVec4 & rColor)
{
	ImGui::PushStyleColor(idx, rColor);
	St.push(objStyleColor);
}

void ImGuiObjStack::PushStyleVar(ImGuiStyleVar idx, float val)
{
	ImGui::PushStyleVar(idx, val);
	St.push(objStyleVar);
}

void ImGuiObjStack::PushStyleVar(ImGuiStyleVar idx, const ImVec2 & rVal)
{
	ImGui::PushStyleVar(idx, rVal);
	St.push(objStyleVar);
}

void FASTCALL ImGuiObjStack::PushTabStop(bool tabSstop)
{
	ImGui::PushTabStop(tabSstop);
	St.push(objTabStop);
}

void FASTCALL ImGuiObjStack::PushButtonrepeat(bool repeat)
{
	ImGui::PushButtonRepeat(repeat);
	St.push(objButtonRepeat);
}

void FASTCALL ImGuiObjStack::PushItemWidth(float itemWidth)
{
	ImGui::PushItemWidth(itemWidth);
	St.push(objItemWidth);
}

void FASTCALL ImGuiObjStack::PushTextWrapPos(float wrapLocalPos)
{
	ImGui::PushTextWrapPos(wrapLocalPos);
	St.push(objTextWrapPos);
}

void FASTCALL ImGuiObjStack::PushID(const char * pStrIdent)
{
	ImGui::PushID(pStrIdent);
	St.push(objID);
}

void ImGuiObjStack::PushID(const char* pStrIdBegin, const char * pStrIdEnd)
{
	ImGui::PushID(pStrIdBegin, pStrIdEnd);
	St.push(objID);
}

void FASTCALL ImGuiObjStack::PushID(const void * pIdent)
{
	ImGui::PushID(pIdent);
	St.push(objID);
}

void FASTCALL ImGuiObjStack::PushID(int id)
{
	ImGui::PushID(id);
	St.push(objID);
}

void FASTCALL ImGuiObjStack::PushAllowKeyboardFocus(bool tabStop)
{
	ImGui::PushAllowKeyboardFocus(tabStop);
	St.push(objAllowKeyboardFocus);
}
//
//
//
ImGuiWindowByLayout::ImGuiWindowByLayout(const SUiLayout * pLoParent, int loId, const char * pWindowId, ImGuiWindowFlags viewFlags) : P_Lo(0)
{
	Helper_Ctr((pLoParent ? pLoParent->FindByIdC(loId) : 0), 0, pWindowId, viewFlags);
}

ImGuiWindowByLayout::ImGuiWindowByLayout(const SUiLayout * pLo, const char * pWindowId, ImGuiWindowFlags viewFlags) : P_Lo(pLo)
{
	Helper_Ctr(pLo, 0, pWindowId, viewFlags);
}
	
ImGuiWindowByLayout::ImGuiWindowByLayout(const SUiLayout * pLo, SPoint2F & rOffset, const char * pWindowId, ImGuiWindowFlags viewFlags) : P_Lo(pLo)
{
	Helper_Ctr(pLo, &rOffset, pWindowId, viewFlags);
}

ImGuiWindowByLayout::~ImGuiWindowByLayout()
{
	if(P_Lo) {
		ImGui::End();
		P_Lo = 0;
	}
}

void ImGuiWindowByLayout::Helper_Ctr(const SUiLayout * pLo, const SPoint2F * pOffset, const char * pWindowId, ImGuiWindowFlags viewFlags)
{
	P_Lo = pLo;
	if(P_Lo) {
		FRect r = P_Lo->GetFrameAdjustedToParent();
		if(pOffset) {
			r.Move__(pOffset->x, pOffset->y);
		}
		SPoint2F s = r.GetSize();
		ImVec2 lu(r.a.x, r.a.y);
		ImVec2 sz(s.x, s.y);
		ImGui::SetNextWindowPos(lu);
		ImGui::SetNextWindowSize(sz);
		SString & r_symb = SLS.AcquireRvlStr();
		if(pWindowId)
			r_symb = pWindowId;
		else
			r_symb.Cat(pLo->GetID());
		ImGui::Begin(r_symb, 0, viewFlags);
	}
}
//
//
//
SImFontDescription::SImFontDescription(ImGuiIO & rIo, const char * pSymb, const char * pPath, float sizePx, const ImFontConfig * pFontCfg, const SColor * pClr) : 
	P_Font(0), Symbol(pSymb)
{
	assert(Symbol.NotEmptyS());
	Clr.Z();
	static const ImWchar ranges[] = {
		0x0020, 0x00FF, // Basic Latin + Latin Supplement
		0x0400, 0x044F, // Cyrillic
		0,
	};
	P_Font = rIo.Fonts->AddFontFromFileTTF(/*"/Papyrus/Src/Rsrc/Font/imgui/Roboto-Medium.ttf"*/pPath, sizePx, pFontCfg, ranges);
	RVALUEPTR(Clr, pClr);
}

const void * SImFontDescription::GetHashKey(const void * pCtx, uint * pKeyLen) const
{
	ASSIGN_PTR(pKeyLen, Symbol.Len());
	return Symbol.cptr();
}
//
//
//
ImGuiSceneBase::ImGuiSceneBase() : ClearColor(SColor(0x1E, 0x22, 0x28)), Cache_Layout(512, 0), Cache_Font(101, 0)
{
}
	
ImGuiSceneBase::~ImGuiSceneBase()
{
}

/*virtual*/int ImGuiSceneBase::Init(ImGuiIO & rIo)
{
	return -1;
}

int ImGuiSceneBase::LoadUiDescription()
{
	int    ok = 0;
	SString temp_buf;
	const UiDescription * p_uid = SLS.GetUiDescription(); // @v11.9.3
	if(p_uid) {
		{
			const bool use_outer_layout_description = true;
			if(use_outer_layout_description) {
				for(uint i = 0; i < p_uid->LoList.getCount(); i++) {
					const SUiLayout * p_lo = p_uid->LoList.at(i);
					if(p_lo) {
						SUiLayout * p_new_lo = new SUiLayout(*p_lo);
						Cache_Layout.Put(p_new_lo, true);
					}
				}
			}
#if 0 // Этот участок кода был актуален в составе класса WsCtl_ImGuiSceneBlock до использования механизма внешнего описания лейаутов {
			else {
				SJson * p_js_lo_list = 0;
				MakeLayout(&p_js_lo_list);
				if(p_js_lo_list) {
					SString temp_buf;
					long fn_n = 0;
					const char * p_fn = "layout";
					SString fn;
					do {
						(fn = p_fn).CatLongZ(++fn_n, 5).Dot().Cat("json");
						PPGetFilePath(PPPATH_OUT, fn, temp_buf);
					} while(fileExists(temp_buf));
					SFile f_out(temp_buf, SFile::mWrite);
					if(f_out.IsValid()) {
						SJson js_ui(SJson::tOBJECT);
						js_ui.Insert("layout_list", p_js_lo_list);
						p_js_lo_list = 0;
						SString js_fmt_buf;
						js_ui.ToStr(temp_buf);
						SJson::FormatText(temp_buf, js_fmt_buf);
						f_out.Write(js_fmt_buf, js_fmt_buf.Len());
					}
					ZDELETE(p_js_lo_list);
				}
			}
#endif // } 0
		}
	}
	//CATCHZOK
	return ok;
}

int ImGuiSceneBase::CreateFontEntry(ImGuiIO & rIo, const char * pSymb, const char * pPath, float sizePx, const ImFontConfig * pFontCfg, const SColor * pClr)
{
	static const ImWchar ranges[] = {
		0x0020, 0x00FF, // Basic Latin + Latin Supplement
		0x0400, 0x044F, // Cyrillic
		0,
	};
	int    ok = 1;
	SImFontDescription * p_fd = new SImFontDescription(rIo, pSymb, pPath, sizePx, pFontCfg, pClr);
	if(p_fd) {
		Cache_Font.Put(p_fd, true);
	}
	else
		ok = 0;
	return ok;
}

int ImGuiSceneBase::PushFontEntry(ImGuiObjStack & rStk, const char * pSymb)
{
	int    ok = 1;
	SImFontDescription * p_fd = Cache_Font.Get(pSymb, sstrlen(pSymb));
	if(p_fd && p_fd->IsValid()) {
		rStk.PushFont(*p_fd);
		if(p_fd->HasColor()) {
			SColor clr = p_fd->GetColor();
			rStk.PushStyleColor(ImGuiCol_Text, IM_COL32(clr.R, clr.G, clr.B, clr.Alpha));
		}
	}
	else
		ok = 0;
	return ok;
}

void ImGuiSceneBase::BuildSceneProlog()
{
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
}

void ImGuiSceneBase::Render(ImGuiRuntimeBlock & rRtb)
{
	ImGui::Render();
	const float clear_color_with_alpha[4] = { 
		ClearColor.x * ClearColor.w, 
		ClearColor.y * ClearColor.w, 
		ClearColor.z * ClearColor.w, 
		ClearColor.w 
	};
	rRtb.g_pd3dDeviceContext->OMSetRenderTargets(1, &rRtb.g_mainRenderTargetView, nullptr);
	rRtb.g_pd3dDeviceContext->ClearRenderTargetView(rRtb.g_mainRenderTargetView, clear_color_with_alpha);
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
	rRtb.g_pSwapChain->Present(1, 0); // Present with vsync
}
//
//
//
