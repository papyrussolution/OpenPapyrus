// IMGUI-SUPPORT.H
// Copytight (c) A.Sobolev 2024
//
#ifndef IMGUI_SUPPORT_H
#define IMGUI_SUPPORT_H

#include <imgui.h>
#include <imgui_internal.h>
#include <backends\imgui_impl_dx9.h>
#include <backends\imgui_impl_dx11.h>
#include <backends\imgui_impl_win32.h>
#define HMONITOR_DECLARED
#include <d3d9.h>
#include <d3d11.h>
#include <..\OSF\DirectXTex\SRC\DirectXTex.h>
#include <..\OSF\DirectXTex\SRC\WICTextureLoader11.h>

class ImGuiRuntimeBlock {
public:
	static constexpr uint IconSize = 32;

	ImGuiRuntimeBlock();
	bool   CreateDeviceD3D(HWND hWnd);
	void   CreateRenderTarget();
	void   CleanupRenderTarget();
	void   CleanupDeviceD3D();
	void * LoadTexture(const char * pFileName);
	void * MakeIconTexture(uint iconId); // @v11.9.2
	void   ReleaseTexture(void * pTextureView);
	//
	// Descr: Общий метод, отрабатывающий Win-message WM_SIZE
	//
	void   OnWindowResize(WPARAM wParam, LPARAM lParam);

	// Data
	ID3D11Device           * g_pd3dDevice;
	ID3D11DeviceContext    * g_pd3dDeviceContext;
	IDXGISwapChain         * g_pSwapChain;
	ID3D11RenderTargetView * g_mainRenderTargetView;
};

class ImGuiObjStack {
public:
	enum {
		objUndef = 0,
		objFont,
		objStyleColor,
		objStyleVar,
		objTabStop,
		objButtonRepeat,
		objItemWidth,
		objTextWrapPos,
		objID,
		objAllowKeyboardFocus, // PushAllowKeyboardFocus
	};
	ImGuiObjStack();
	~ImGuiObjStack();
	void FASTCALL PushFont(ImFont * pFont);
	void PushStyleColor(ImGuiCol idx, ImU32 col);
	void PushStyleColor(ImGuiCol idx, const ImVec4 & rColor);
	void PushStyleVar(ImGuiStyleVar idx, float val);
	void PushStyleVar(ImGuiStyleVar idx, const ImVec2 & rVal);
	void FASTCALL PushTabStop(bool tabSstop);
	void FASTCALL PushButtonrepeat(bool repeat);
	void FASTCALL PushItemWidth(float itemWidth);
	void FASTCALL PushTextWrapPos(float wrapLocalPos);
	void FASTCALL PushID(const char * pStrIdent);
	void PushID(const char* pStrIdBegin, const char * pStrIdEnd);
	void FASTCALL PushID(const void * pIdent);
	void FASTCALL PushID(int id);
	void FASTCALL PushAllowKeyboardFocus(bool tabStop);
private:
	TSStack <int> St;
};

class ImGuiWindowByLayout {
public:
	ImGuiWindowByLayout(const SUiLayout * pLoParent, int loId, const char * pWindowId, ImGuiWindowFlags viewFlags);
	ImGuiWindowByLayout(const SUiLayout * pLo, const char * pWindowId, ImGuiWindowFlags viewFlags);
	ImGuiWindowByLayout(const SUiLayout * pLo, SPoint2F & rOffset, const char * pWindowId, ImGuiWindowFlags viewFlags);
	~ImGuiWindowByLayout();
	bool IsValid() const { return (P_Lo != 0); }
private:
	void   Helper_Ctr(const SUiLayout * pLo, const SPoint2F * pOffset, const char * pWindowId, ImGuiWindowFlags viewFlags);
	const SUiLayout * P_Lo;
};

class SImFontDescription {
public:
	SImFontDescription(ImGuiIO & rIo, const char * pSymb, const char * pPath, float sizePx, const ImFontConfig * pFontCfg, const SColor * pClr);
	operator ImFont * () { return P_Font; };
	bool   IsValid() const { return (P_Font != 0); }
	bool   HasColor() const { return !Clr.IsEmpty(); }
	SColor GetColor() const { return Clr; }
	//
	// Descr: Каноническая функция возвращающая ключ экземпляра для хэширования.
	//
	const void * GetHashKey(const void * pCtx, uint * pKeyLen) const;
private:
	SString Symbol;
	uint   State;
	SColor Clr; // 
	ImFont * P_Font;
};

class ImGuiSceneBase {
public:
	ImGuiSceneBase();
	~ImGuiSceneBase();
	virtual int Init(ImGuiIO & rIo);
protected:
	void   Render(ImGuiRuntimeBlock & rRtb);
	void   BuildSceneProlog();
	int    LoadUiDescription(ImGuiIO & rIo);
	int    CreateFontEntry(ImGuiIO & rIo, const char * pSymb, const char * pPath, float sizePx, const ImFontConfig * pFontCfg, const SColor * pClr);
	int    PushFontEntry(ImGuiObjStack & rStk, const char * pSymb);
	
	class CommonTextureCacheEntry {
	public:
		CommonTextureCacheEntry() : P_Texture(0)
		{
		}
		explicit CommonTextureCacheEntry(void * pTexture) : P_Texture(pTexture)
		{
		}
		void * P_Texture;
		void Destroy()
		{
			if(P_Texture) {
				static_cast<IUnknown *>(P_Texture)->Release();
				P_Texture = 0;
			}
		}
	};
	class Texture_IconEntity : public CommonTextureCacheEntry {
	public:
		Texture_IconEntity() : CommonTextureCacheEntry(), Id(0)
		{
		}
		Texture_IconEntity(uint id, void * pTexture) : CommonTextureCacheEntry(pTexture), Id(id)
		{
		}
		const void * GetHashKey(const void * pCtx, uint * pSize) const // hash-table support
		{
			ASSIGN_PTR(pSize, sizeof(Id));
			return &Id;
		}
		uint    Id;
	};
	class IconTextureCache : private TSHashCollection <Texture_IconEntity> { // @v11.9.2
	public:
		IconTextureCache() : TSHashCollection <Texture_IconEntity>(1024, 0)
		{
		}
		void * Get(ImGuiRuntimeBlock & rRtb, uint id)
		{
			void * p_texture = 0;
			if(id) {
				Texture_IconEntity * p_entry = static_cast<Texture_IconEntity *>(TSHashCollection <Texture_IconEntity>::Get(&id, sizeof(id)));
				if(p_entry) {
					p_texture = p_entry->P_Texture;
				}
				else {
					void * p_icon = rRtb.MakeIconTexture(id);
					if(p_icon) {
						Texture_IconEntity * p_new_entry = new Texture_IconEntity(id, p_icon);
						if(p_new_entry) {
							if(TSHashCollection <Texture_IconEntity>::Put(p_new_entry, true)) {
								p_entry = static_cast<Texture_IconEntity *>(TSHashCollection <Texture_IconEntity>::Get(&id, sizeof(id)));
								assert(p_entry && p_entry == p_new_entry);
								p_texture = p_entry->P_Texture;
							}
						}
					}
				}
			}
			return p_texture;
		}
	};
	IconTextureCache Cache_Icon;
	ImVec4 ClearColor;
	TSHashCollection <SUiLayout> Cache_Layout;
	TSHashCollection <SImFontDescription> Cache_Font;
};

#endif // !IMGUI_SUPPORT_H


