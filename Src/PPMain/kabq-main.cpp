// KABQ-MAIN.CPP
// Copyright (c) A.Sobolev 2024
//
#include <pp.h>
#pragma hdrstop
#include <imgui-support.h>

static ImGuiRuntimeBlock ImgRtb;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam); // Forward declare message handler from imgui_impl_win32.cpp

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if(ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
		return true;
	switch(msg) {
		case WM_SIZE:
			/*
		    if(ImgRtb.g_pd3dDevice != nullptr && wParam != SIZE_MINIMIZED) {
			    ImgRtb.CleanupRenderTarget();
			    ImgRtb.g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
			    ImgRtb.CreateRenderTarget();
		    }
			*/
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

int main(int, char **)
{
	int    result = 0;
	DS.Init(PPSession::fWsCtlApp|PPSession::fInitPaths, 0, /*pUiDescriptionFileName*//*"uid-wsctl.json"*/0);
	WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"WSCTL_WCLS", nullptr };
	::RegisterClassExW(&wc);
	HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"KAB-Q", WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, nullptr, nullptr, wc.hInstance, nullptr);
	if(!ImgRtb.CreateDeviceD3D(hwnd)) {
		result = 1;
	}
	else {
		// Show the window
		::ShowWindow(hwnd, SW_SHOWDEFAULT);
		::UpdateWindow(hwnd);
		// Setup Dear ImGui context
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO & io = ImGui::GetIO(); 
		//(void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls
		// Setup Dear ImGui style
		//ImGui::StyleColorsDark();
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
		//assert(font != nullptr);
		// Our state
		//bool show_demo_window = false; // @sobolev true-->false
		//bool show_another_window = false;
		//ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
		
		//WsCtl_ImGuiSceneBlock scene_blk;
		//scene_blk.Init(io);
		//scene_blk.SetScreen(WsCtl_ImGuiSceneBlock::screenConstruction);
		
		/*{
			const char * p_img_path = "/Papyrus/Src/PPTEST/DATA/test-gif.gif";
			P_TestImgTexture = ImgRtb.LoadTexture(p_img_path);
		}*/
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
				//scene_blk.EmitEvents();
				//scene_blk.BuildScene();
			}
		} while(!done);
		// Cleanup
		ImGui_ImplDX11_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();
		//ImgRtb.CleanupDeviceD3D();
		::DestroyWindow(hwnd);
		//::UnregisterClassW(wc.lpszClassName, wc.hInstance);
		result = 0;
	}
	ImgRtb.CleanupDeviceD3D();
	::UnregisterClassW(wc.lpszClassName, wc.hInstance);
	return result;
}
