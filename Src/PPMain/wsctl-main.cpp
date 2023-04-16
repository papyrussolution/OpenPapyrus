// WSCTL-MAIN.CPP
// Copyright (c) A.Sobolev 2023
//
#include <pp.h>
#pragma hdrstop
#include "imgui.h"
#include "backends\imgui_impl_dx9.h"
#include "backends\imgui_impl_dx11.h"
#include "backends\imgui_impl_win32.h"
#define HMONITOR_DECLARED
#include <d3d9.h>
#include <d3d11.h>
#include <tchar.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam); // Forward declare message handler from imgui_impl_win32.cpp

class ImGuiRuntimeBlock {
public:
	ImGuiRuntimeBlock() : g_pd3dDevice(0), g_pd3dDeviceContext(0), g_pSwapChain(0), g_mainRenderTargetView(0)
	{
	}
	bool CreateDeviceD3D(HWND hWnd)
	{
		// Setup swap chain
		DXGI_SWAP_CHAIN_DESC sd;
		ZeroMemory(&sd, sizeof(sd));
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
	void CreateRenderTarget()
	{
		ID3D11Texture2D * pBackBuffer;
		g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
		g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
		pBackBuffer->Release();
	}
	void CleanupRenderTarget()
	{
		if(g_mainRenderTargetView) {
			g_mainRenderTargetView->Release(); 
			g_mainRenderTargetView = nullptr;
		}
	}
	void CleanupDeviceD3D()
	{
		CleanupRenderTarget();
		if(g_pSwapChain) {
			g_pSwapChain->Release(); 
			g_pSwapChain = nullptr;
		}
		if(g_pd3dDeviceContext) {
			g_pd3dDeviceContext->Release(); 
			g_pd3dDeviceContext = nullptr;
		}
		if(g_pd3dDevice) {
			g_pd3dDevice->Release(); 
			g_pd3dDevice = nullptr;
		}
	}
	// Data
	ID3D11Device           * g_pd3dDevice;
	ID3D11DeviceContext    * g_pd3dDeviceContext;
	IDXGISwapChain         * g_pSwapChain;
	ID3D11RenderTargetView * g_mainRenderTargetView;
};

static ImGuiRuntimeBlock ImgRtb;

class WsCtl_ImGuiSceneBlock {
private:
	bool   ShowDemoWindow; // @sobolev true-->false
	bool   ShowAnotherWindow;
	ImVec4 ClearColor;
	SUiLayout Lo01; // = new SUiLayout(SUiLayoutParam(DIREC_VERT, 0, SUiLayoutParam::alignStretch));
	//
	enum {
		loidRoot = 1,
		loidUpperGroup,
		loidBottomGroup,
		loidCtl01,
		loidCtl02,
		loidAdv01,
		loidAdv02,
		loidAdv03,
	};
	//
	void   Render()
	{
		ImGui::Render();
		const float clear_color_with_alpha[4] = { ClearColor.x * ClearColor.w, ClearColor.y * ClearColor.w, ClearColor.z * ClearColor.w, ClearColor.w };
		ImgRtb.g_pd3dDeviceContext->OMSetRenderTargets(1, &ImgRtb.g_mainRenderTargetView, nullptr);
		ImgRtb.g_pd3dDeviceContext->ClearRenderTargetView(ImgRtb.g_mainRenderTargetView, clear_color_with_alpha);
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
		ImgRtb.g_pSwapChain->Present(1, 0); // Present with vsync
		//g_pSwapChain->Present(0, 0); // Present without vsync
	}
	void MakeLayout()
	{
		Lo01.SetLayoutBlock(SUiLayoutParam(DIREC_VERT, 0, SUiLayoutParam::alignStretch));
		Lo01.SetID(loidRoot);
		{
			SUiLayoutParam alb(DIREC_HORZ, 0, SUiLayoutParam::alignStretch);
			alb.GrowFactor = 1.2f;
			alb.SetVariableSizeX(SUiLayoutParam::szByContainer, 1.0f);
			SUiLayout * p_lo_up_group = Lo01.InsertItem(0, &alb);
			p_lo_up_group->SetID(loidUpperGroup);
			{
				SUiLayoutParam alb01(DIREC_VERT, 0, SUiLayoutParam::alignStretch);
				alb01.GrowFactor = 1.0f;
				alb01.SetVariableSizeY(SUiLayoutParam::szByContainer, 1.0f);
				SUiLayout * p_lo_ctl1 = p_lo_up_group->InsertItem(0, &alb01);
				p_lo_ctl1->SetID(loidCtl01);
			}
			{
				SUiLayoutParam alb01(DIREC_VERT, 0, SUiLayoutParam::alignStretch);
				alb01.GrowFactor = 1.0f;
				alb01.SetVariableSizeY(SUiLayoutParam::szByContainer, 1.0f);
				SUiLayout * p_lo_ctl2 = p_lo_up_group->InsertItem(0, &alb01);
				p_lo_ctl2->SetID(loidCtl02);
			}
		}
		{
			SUiLayoutParam alb(DIREC_HORZ, 0, SUiLayoutParam::alignStretch);
			alb.GrowFactor = 0.8f;
			alb.SetVariableSizeX(SUiLayoutParam::szByContainer, 1.0f);
			SUiLayout * p_lo_dn_group = Lo01.InsertItem(0, &alb);
			p_lo_dn_group->SetID(loidBottomGroup);
			{
				SUiLayoutParam alb01(DIREC_VERT, 0, SUiLayoutParam::alignStretch);
				alb01.GrowFactor = 1.0f;
				alb01.SetVariableSizeY(SUiLayoutParam::szByContainer, 1.0f);
				SUiLayout * p_lo_adv1 = p_lo_dn_group->InsertItem(0, &alb01);
				p_lo_adv1->SetID(loidAdv01);
			}
			{
				SUiLayoutParam alb01(DIREC_VERT, 0, SUiLayoutParam::alignStretch);
				alb01.GrowFactor = 1.0f;
				alb01.SetVariableSizeY(SUiLayoutParam::szByContainer, 1.0f);
				SUiLayout * p_lo_adv2 = p_lo_dn_group->InsertItem(0, &alb01);
				p_lo_adv2->SetID(loidAdv02);
			}
			{
				SUiLayoutParam alb01(DIREC_VERT, 0, SUiLayoutParam::alignStretch);
				alb01.GrowFactor = 1.0f;
				alb01.SetVariableSizeY(SUiLayoutParam::szByContainer, 1.0f);
				SUiLayout * p_lo_adv3 = p_lo_dn_group->InsertItem(0, &alb01);
				p_lo_adv3->SetID(loidAdv03);
			}
		}
	}
public:
	WsCtl_ImGuiSceneBlock() : ShowDemoWindow(false), ShowAnotherWindow(false), ClearColor(0.45f, 0.55f, 0.60f, 1.00f)
	{
		MakeLayout();
	}
	void BuildScene()
	{
		// Start the Dear ImGui frame
		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
		// 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
		if(ShowDemoWindow) {
			// @sobolev ImGui::ShowDemoWindow(&show_demo_window);
		}
		// 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
		{
			ImGuiViewport * p_vp = ImGui::GetMainViewport();
			if(p_vp) {
				ImVec2 sz = p_vp->Size;
				SUiLayout::Param evp;
				evp.ForceWidth = static_cast<float>(sz.x);
				evp.ForceHeight = static_cast<float>(sz.y);
				Lo01.Evaluate(&evp);
				{
					const SUiLayout * p_lo = Lo01.FindByID(loidCtl01);
					if(p_lo) {
						FRect r = p_lo->GetFrame();
						SPoint2F s = r.GetSize();
						ImVec2 lu(r.a.x, r.a.y);
						ImVec2 sz(s.x, s.y);
						ImGui::SetNextWindowPos(lu);
						ImGui::SetNextWindowSize(sz);
						ImGui::Begin("CTL-01", 0, ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove);
						ImGui::Text("CTL-01");
						ImGui::End();
					}
				}
				{
					const SUiLayout * p_lo = Lo01.FindByID(loidCtl02);
					if(p_lo) {
						FRect r = p_lo->GetFrame();
						SPoint2F s = r.GetSize();
						ImVec2 lu(r.a.x, r.a.y);
						ImVec2 sz(s.x, s.y);
						ImGui::SetNextWindowPos(lu);
						ImGui::SetNextWindowSize(sz);
						ImGui::Begin("CTL-02", 0, ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove);
						ImGui::Text("CTL-02");
						ImGui::End();
					}
				}
				{
					const SUiLayout * p_lo = Lo01.FindByID(loidAdv01);
					if(p_lo) {
						FRect r = p_lo->GetFrame();
						SPoint2F s = r.GetSize();
						ImVec2 lu(r.a.x, r.a.y);
						ImVec2 sz(s.x, s.y);
						ImGui::SetNextWindowPos(lu);
						ImGui::SetNextWindowSize(sz);
						ImGui::Begin("ADV-01", 0, ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove);
						ImGui::Text("ADV-01");
						ImGui::End();
					}
				}
				{
					const SUiLayout * p_lo = Lo01.FindByID(loidAdv02);
					if(p_lo) {
						FRect r = p_lo->GetFrame();
						SPoint2F s = r.GetSize();
						ImVec2 lu(r.a.x, r.a.y);
						ImVec2 sz(s.x, s.y);
						ImGui::SetNextWindowPos(lu);
						ImGui::SetNextWindowSize(sz);
						ImGui::Begin("ADV-02", 0, ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove);
						ImGui::Text("ADV-02");
						ImGui::End();
					}
				}
				{
					const SUiLayout * p_lo = Lo01.FindByID(loidAdv03);
					if(p_lo) {
						FRect r = p_lo->GetFrame();
						SPoint2F s = r.GetSize();
						ImVec2 lu(r.a.x, r.a.y);
						ImVec2 sz(s.x, s.y);
						ImGui::SetNextWindowPos(lu);
						ImGui::SetNextWindowSize(sz);
						ImGui::Begin("ADV-03", 0, ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove);
						ImGui::Text("ADV-03");
						ImGui::End();
					}
				}
			}
		}
		/*
		{
			static float f = 0.0f;
			static int counter = 0;
			{
				ImVec2 wp(0.0f, 0.0f);
				ImGui::SetNextWindowPos(wp);
			}
			ImGui::Begin("Hello, world!", 0, ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove); // Create a window called "Hello, world!" and append into it.
			ImGui::Text("This is some useful text.");   // Display some text (you can use a format strings too)
			ImGui::Checkbox("Demo Window", &ShowDemoWindow); // Edit bools storing our window open/close state
			ImGui::Checkbox("Another Window", &ShowAnotherWindow);
			ImGui::SliderFloat("float", &f, 0.0f, 1.0f); // Edit 1 float using a slider from 0.0f to 1.0f
			ImGui::ColorEdit3("clear color", (float *)&ClearColor); // Edit 3 floats representing a color
			if(ImGui::Button("Button")) // Buttons return true when clicked (most widgets return true when edited/activated)
				counter++;
			ImGui::SameLine();
			ImGui::Text("counter = %d", counter);
			{
				ImGuiIO & r_io = ImGui::GetIO(); 
				ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / r_io.Framerate, r_io.Framerate);
			}
			ImGui::End();
		}
		// 3. Show another simple window.
		if(ShowAnotherWindow) {
			ImGui::Begin("Another Window", &ShowAnotherWindow); // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
			ImGui::Text("Hello from another window!");
			if(ImGui::Button("Close Me"))
				ShowAnotherWindow = false;
			ImGui::End();
		}
		*/
		Render(); // Rendering
	}
};
//
// Win32 message handler
// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
//
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if(ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
		return true;
	switch(msg) {
		case WM_SIZE:
		    if(ImgRtb.g_pd3dDevice != nullptr && wParam != SIZE_MINIMIZED) {
			    ImgRtb.CleanupRenderTarget();
			    ImgRtb.g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
			    ImgRtb.CreateRenderTarget();
		    }
		    return 0;
		case WM_SYSCOMMAND:
		    if((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
			    return 0;
		    break;
		case WM_DESTROY:
		    ::PostQuitMessage(0);
		    return 0;
	}
	return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}

int main(int, char**)
{
	// Create application window
	//ImGui_ImplWin32_EnableDpiAwareness();
	WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"WSCTL_WCLS", nullptr };
	::RegisterClassExW(&wc);
	HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"WSCTL", WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, nullptr, nullptr, wc.hInstance, nullptr);
	// Initialize Direct3D
	if(!ImgRtb.CreateDeviceD3D(hwnd)) {
		ImgRtb.CleanupDeviceD3D();
		::UnregisterClassW(wc.lpszClassName, wc.hInstance);
		return 1;
	}
	else {
		// Show the window
		::ShowWindow(hwnd, SW_SHOWDEFAULT);
		::UpdateWindow(hwnd);
		// Setup Dear ImGui context
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO & io = ImGui::GetIO(); 
		(void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls
		// Setup Dear ImGui style
		ImGui::StyleColorsDark();
		//ImGui::StyleColorsLight();
		// Setup Platform/Renderer backends
		ImGui_ImplWin32_Init(hwnd);
		ImGui_ImplDX11_Init(ImgRtb.g_pd3dDevice, ImgRtb.g_pd3dDeviceContext);
		// Load Fonts
		// - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
		// - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
		// - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
		// - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
		// - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
		// - Read 'docs/FONTS.md' for more instructions and details.
		// - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
		//io.Fonts->AddFontDefault();
		//io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
		//io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
		//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
		//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
		//ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, nullptr, io.Fonts->GetGlyphRangesJapanese());
		//IM_ASSERT(font != nullptr);
		// Our state
		//bool show_demo_window = false; // @sobolev true-->false
		//bool show_another_window = false;
		//ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
		WsCtl_ImGuiSceneBlock scene_blk;
		// Main loop
		bool done = false;
		do {
			// Poll and handle messages (inputs, window resize, etc.)
			// See the WndProc() function below for our to dispatch events to the Win32 backend.
			MSG msg;
			while(::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
				::TranslateMessage(&msg);
				::DispatchMessage(&msg);
				if(msg.message == WM_QUIT)
					done = true;
			}
			if(!done) {
				scene_blk.BuildScene();
			}
		} while(!done);
		// Cleanup
		ImGui_ImplDX11_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();
		ImgRtb.CleanupDeviceD3D();
		::DestroyWindow(hwnd);
		::UnregisterClassW(wc.lpszClassName, wc.hInstance);
		return 0;
	}
}
