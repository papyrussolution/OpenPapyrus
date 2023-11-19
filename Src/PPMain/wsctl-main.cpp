// WSCTL-MAIN.CPP
// Copyright (c) A.Sobolev 2023
// @codepage UTF-8
//
#include <pp.h>
#pragma hdrstop
#include <wsctl.h>
#include "imgui.h"
#include "imgui_internal.h"
#include "backends\imgui_impl_dx9.h"
#include "backends\imgui_impl_dx11.h"
#include "backends\imgui_impl_win32.h"
#define HMONITOR_DECLARED
#include <d3d9.h>
#include <d3d11.h>
#include <..\OSF\DirectXTex\SRC\DirectXTex.h>
#include <..\OSF\DirectXTex\SRC\WICTextureLoader11.h>

ImVec2 FPointToImVec2(const SPoint2F & rP) { return ImVec2(rP.x, rP.y); }
ImRect FRectToImRect(const FRect & rR) { return ImRect(FPointToImVec2(rR.a), FPointToImVec2(rR.b)); }

namespace ImGui {
	bool ScrollbarEx(const ImRect & bb_frame, ImGuiID id, ImGuiAxis axis, int64* p_scroll_v, int64 size_avail_v, int64 size_contents_v, ImDrawFlags flags);
}
/*
	Style tokens:

	ColorText
	ColorTextDisabled
	ColorTextDarker // npp
	ColorTextLink   // npp
	Window (oneof: Genric | Popup | Child)
		Color
		ColorSofter // npp
		ColorHot // npp
		ColorPure // npp
		ColorError // npp
	Surface (oneof: Frame | Title | Button | Header | Separator | ResizeGrip | Tab | ScrollbarGrab | Scrollbar)
		Color
		ColorHovered
		ColorActive
		ColorCollapsed
		ColorUnfocused       // Tab
		ColorUnfocusedActive // Tab
	ColorBorder
	ColorBorderShadow
	ColorMenuBarBg
	ColorScrollbarBg
	ColorCheckMark
	ColorSliderGrab
	ColorSliderGrabActive
	ColorPlotLines
	ColorPlotLinesHovered
	ColorPlotHistogram
	ColorPlotHistogramHovered
	ColorTableHeaderBg
	ColorTableBorderStrong
	ColorTableBorderLight
	ColorTableRowBg
	ColorTableRowBgAlt
	ColorTextSelectedBg
	ColorDragDropTarget
	ColorNavHighlight
	ColorNavWindowingHighlight
	ColorNavWindowingDimBg
	ColorModalWindowDimBg
*/
static FORCEINLINE float ColorComponentBlendOverlay(float sa, float s, float da, float d)
{
	return ((2 * d) < da) ? (2 * s * d) : (sa * da - 2 * (da - d) * (sa - s));
}

FORCEINLINE ImVec4 __ColorBlendOverlay(const ImVec4 & rS, const ImVec4 & rD, float alpha)
{
	return ImVec4(ColorComponentBlendOverlay(1.0f, rS.x, alpha, rD.x),
		ColorComponentBlendOverlay(1.0f, rS.y, alpha, rD.y), ColorComponentBlendOverlay(1.0f, rS.z, alpha, rD.z), rS.w);
}

static const ImVec4 MainBackgroundColor(SColor(0x1E, 0x22, 0x28));
static const ImVec2 ButtonSize_Std(64.0f, 24.0f);
static const ImVec2 ButtonSize_Double(128.0f, 24.0f);

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
	ImGuiObjStack()
	{
	}
	~ImGuiObjStack()
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
	void PushFont(ImFont * pFont)
	{
		if(pFont) {
			ImGui::PushFont(pFont);
			St.push(objFont);
		}
	}
	void PushStyleColor(ImGuiCol idx, ImU32 col)
	{
		ImGui::PushStyleColor(idx, col);
		St.push(objStyleColor);
	}
	void PushStyleColor(ImGuiCol idx, const ImVec4 & rColor)
	{
		ImGui::PushStyleColor(idx, rColor);
		St.push(objStyleColor);
	}
	void PushStyleVar(ImGuiStyleVar idx, float val)
	{
		ImGui::PushStyleVar(idx, val);
		St.push(objStyleVar);
	}
	void PushStyleVar(ImGuiStyleVar idx, const ImVec2 & rVal)
	{
		ImGui::PushStyleVar(idx, rVal);
		St.push(objStyleVar);
	}
	void PushTabStop(bool tabSstop)
	{
		ImGui::PushTabStop(tabSstop);
		St.push(objTabStop);
	}
	void PushButtonrepeat(bool repeat)
	{
		ImGui::PushButtonRepeat(repeat);
		St.push(objButtonRepeat);
	}
	void PushItemWidth(float itemWidth)
	{
		ImGui::PushItemWidth(itemWidth);
		St.push(objItemWidth);
	}
	void PushTextWrapPos(float wrapLocalPos)
	{
		ImGui::PushTextWrapPos(wrapLocalPos);
		St.push(objTextWrapPos);
	}
	void PushID(const char * pStrIdent)
	{
		ImGui::PushID(pStrIdent);
		St.push(objID);
	}
	void PushID(const char* pStrIdBegin, const char * pStrIdEnd)
	{
		ImGui::PushID(pStrIdBegin, pStrIdEnd);
		St.push(objID);
	}
	void PushID(const void * pIdent)
	{
		ImGui::PushID(pIdent);
		St.push(objID);
	}
	void PushID(int id)
	{
		ImGui::PushID(id);
		St.push(objID);
	}
	void PushAllowKeyboardFocus(bool tabStop)
	{
		ImGui::PushAllowKeyboardFocus(tabStop);
		St.push(objAllowKeyboardFocus);
	}
private:
	TSStack <int> St;
};

static void * P_TestImgTexture = 0; // @debug

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
	void CreateRenderTarget()
	{
		ID3D11Texture2D * pBackBuffer;
		g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
		g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
		pBackBuffer->Release();
	}
	void CleanupRenderTarget()
	{
		SCOMOBJRELEASE(g_mainRenderTargetView); 
	}
	void CleanupDeviceD3D()
	{
		CleanupRenderTarget();
		SCOMOBJRELEASE(g_pSwapChain); 
		SCOMOBJRELEASE(g_pd3dDeviceContext); 
		SCOMOBJRELEASE(g_pd3dDevice); 
	}
	void * LoadTexture(const char * pFileName)
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
	void ReleaseTexture(void * pTextureView)
	{
		if(pTextureView) {
			static_cast<IUnknown *>(pTextureView)->Release();
		}
	}
	// Data
	ID3D11Device           * g_pd3dDevice;
	ID3D11DeviceContext    * g_pd3dDeviceContext;
	IDXGISwapChain         * g_pSwapChain;
	ID3D11RenderTargetView * g_mainRenderTargetView;
};

static ImGuiRuntimeBlock ImgRtb;
//
// Descr: Очередь команд потоку, передающему эти команды серверу
//
class WsCtlReqQueue : private SQueue { //req_queue(1024)
public:
	//
	// Descr: Запрос к серверу. Помещается в WsCtlReqQueue потому должен быть @flat.
	//
	struct Req { // @flat
		Req() : Cmd(0)
		{
		}
		Req(uint cmd) : Cmd(cmd)
		{
		}
		uint   Cmd;
		struct Param {
			Param() : SCardID(0), GoodsID(0), TechID(0), TSessID(0), Amount(0.0)
			{
				AuthTextUtf8[0] = 0;
				AuthPwUtf8[0] = 0;
			}
			S_GUID Uuid;
			PPID   SCardID; // 
			PPID   GoodsID;
			PPID   TechID;
			PPID   TSessID;
			double Amount;
			char   AuthTextUtf8[128];
			char   AuthPwUtf8[128];
		};
		Param P;
	};
	WsCtlReqQueue() : SQueue(sizeof(Req), 1024, aryDataOwner), NonEmptyEv(Evnt::modeCreateAutoReset)
	{
	}
	int    FASTCALL Push(const Req & rReq)
	{
		int    ok = 0;
		Lck.Lock();
		ok = SQueue::push(&rReq);
		Lck.Unlock();
		return ok;
	}
	int    FASTCALL Pop(Req & rReq)
	{
		int    ok = -1;
		Lck.Lock();
		Req  * p_item = static_cast<Req *>(SQueue::pop());
		if(p_item) {
			rReq = *p_item;
			ok = 1;
		}
		Lck.Unlock();
		return ok;
	}

	Evnt   NonEmptyEv; // Событие поможет "разбудить" поток, принимающий данные из этой очереди. 
private:
	SMtLock Lck;
};
//
// 
// 
class ImDialogState {
protected:
	ImDialogState(void * pCtx) : Launched(false), P_Ctx(pCtx)
	{
	}
	virtual ~ImDialogState()
	{
	}
	//
	// Returns:
	//   >0 - user pressed [ok]
	//   <0 - user pressed [cancel] or [close]
	//    0 - continue
	//
	virtual int Build()
	{
		return -1;
	}
	virtual bool CommitData()
	{
		return true;
	}
	bool   Launched;
	uint8  Reserve[3]; // @alignment
	void * P_Ctx;
};

class SImFontDescription {
public:
	SImFontDescription(ImGuiIO & rIo, const char * pSymb, const char * pPath, float sizePx, const ImFontConfig * pFontCfg, const SColor * pClr) : P_Font(0), Symbol(pSymb)
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
	operator ImFont * () { return P_Font; };
	bool   IsValid() const { return (P_Font != 0); }
	bool   HasColor() const { return !Clr.IsEmpty(); }
	SColor GetColor() const { return Clr; }
	//
	// Descr: Каноническая функция возвращающая ключ экземпляра для хэширования.
	//
	const void * GetHashKey(const void * pCtx, uint * pKeyLen) const
	{
		ASSIGN_PTR(pKeyLen, Symbol.Len());
		return Symbol.cptr();
	}
private:
	SString Symbol;
	uint   State;
	SColor Clr; // 
	ImFont * P_Font;
};
//
//
//
class ImGuiWindowByLayout {
	void   Helper_Ctr(const SUiLayout * pLo, const SPoint2F * pOffset, const char * pWindowId, ImGuiWindowFlags viewFlags)
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
public:
	ImGuiWindowByLayout(const SUiLayout * pLoParent, int loId, const char * pWindowId, ImGuiWindowFlags viewFlags) : P_Lo(0)
	{
		Helper_Ctr((pLoParent ? pLoParent->FindByIdC(loId) : 0), 0, pWindowId, viewFlags);
	}
	ImGuiWindowByLayout(const SUiLayout * pLo, const char * pWindowId, ImGuiWindowFlags viewFlags) : P_Lo(pLo)
	{
		Helper_Ctr(pLo, 0, pWindowId, viewFlags);
	}
	ImGuiWindowByLayout(const SUiLayout * pLo, SPoint2F & rOffset, const char * pWindowId, ImGuiWindowFlags viewFlags) : P_Lo(pLo)
	{
		Helper_Ctr(pLo, &rOffset, pWindowId, viewFlags);
	}
	~ImGuiWindowByLayout()
	{
		if(P_Lo) {
			ImGui::End();
			P_Lo = 0;
		}
	}
	bool IsValid() const { return (P_Lo != 0); }
	const SUiLayout * P_Lo;
};

class WsCtl_ImGuiSceneBlock {
public:
	//
	// Descr: Вариант экранов приложения //
	//
	enum {
		screenUndef          = 0, // неопределенный
		screenConstruction   = 1, // Тестовый режим для разработки 
		screenHybernat       = 2, // спящий режим 
		screenRegister       = 3, // регистрация //
		screenLogin          = 4, // авторизация //
		screenAuthSelectSess = 5, // авторизованный режим - выбор сессии
		screenSession        = 6  // рабочая сессия //
	};
	//
	enum {
		// Значения до 10000 используются для идентификации экранов (screenXXX) и 
		// одновременно как идентификаторы соответствующих лейаутов верхнего уровня.
		//loidRoot = 1,
		loidUpperGroup                = 10001,
		loidBottomGroup               = 10002,
		loidCtl01                     = 10003,
		loidCtl02                     = 10004,
		loidTestProgramGallery        = 10005,
		//loidAdv02                   = 10006,
		//loidAdv03                   = 10007,
		loidMenuBlock                 = 10008,
		loidMainGroup                 = 10009,
		loidLoginBlock                = 10010,
		loidPersonInfo                = 10011,
		loidAccountInfo               = 10012,
		loidSessionSelection          = 10013,
		loidSessionButtonGroup        = 10014,
		loidBottomCtrlGroup           = 10015,
		loidSessionInfo               = 10016,
		loidSessionProgramGallery     = 10017,
		loidProgramEntryTemplate      = 10018, // Шаблон для элемента выбора программы
		loidInternalProgramGallery    = 10019, // Внутренний лейаут программной галлереи. Формируется динамически (не включен в json-описание)!
		loidProgramGalleryScrollbar   = 10020, // Область для размещения скроллбара программной галлереи
		loidProgramGalleryCatSelector = 10021, // Область для размещения комбо-бокса выбора категории
		//
		loidStartProgramEntry         = 20000  // Стартовый идентификатор для иконок выбора программ. Первый layout идентифицируется как (loidStartProgramEntry+1)
	};

	struct QuotKindEntry {
		QuotKindEntry() : ID(0), Rank(0), DaysOfWeek(0)
		{
			ApplyTm.Z();
		}
		bool   HasWeekDayRestriction() const { return (DaysOfWeek && ((DaysOfWeek & 0x7f) != 0x7f)); }
		bool   CheckWeekDay(LDATE dt) const { return (!DaysOfWeek || !dt || (DaysOfWeek & (1 << (dayofweek(&dt, 1)-1)))); }

		PPID   ID;
		int    Rank;
		int    DaysOfWeek;
		TimeRange ApplyTm;
		SString NameUtf8;
	};
	struct QuotEntry {
		QuotEntry() : QkID(0), Value(0.0)
		{
		}
		PPID   QkID;
		double Value;
	};
	struct TechEntry {
		TechEntry() : ID(0)
		{
			CodeUtf8[0] = 0;
		}
		PPID   ID;
		char   CodeUtf8[48];
	};
	struct GoodsEntry {
		GoodsEntry() : ID(0)
		{
		}
		GoodsEntry(const GoodsEntry & rS) : ID(rS.ID), NameUtf8(rS.NameUtf8), QuotList(rS.QuotList)
		{
		}
		GoodsEntry & FASTCALL operator = (const GoodsEntry & rS)
		{
			ID = rS.ID;
			NameUtf8 = rS.NameUtf8;
			TechList = rS.TechList;
			QuotList = rS.QuotList;
			return *this;
		}
		PPID   ID;
		SString NameUtf8;
		TSVector <TechEntry> TechList;
		TSVector <QuotEntry> QuotList;
	};
	//
	class DServerError {
	public:
		DServerError();
		DServerError(const DServerError & rS);
		DServerError & FASTCALL operator = (const DServerError & rS);
		DServerError & Copy(const DServerError & rS);
		DServerError & Z();
		DServerError & SetupByLastError();

		int    _Status; // 0 - ok, -1 - abstract error, >0 - error code
		SString _Message;
	};
	class DTest : public DServerError {
	public:
		SString Reply;
	};
	//
	//
	//
	class DConnectionStatus : public DServerError {
	public:	
		DConnectionStatus() : S(0), DtmActual(ZERODATETIME)
		{
		}
		LDATETIME DtmActual; // Момент последней актуализации данных
		int    S;
	};
	//
	//
	//
	/*class DClientPolicy : public DServerError {
	public:
		DClientPolicy() : DtmActual(ZERODATETIME), Dirty(false)
		{
		}
		LDATETIME DtmActual; // Момент последней актуализации данных
		WsCtl_ClientPolicy P;
		bool Dirty; // Специальный флаг, индицирующий обновление данных со стороны потока обмена с сервером.
	};*/
	/*
	class DProgramList : public DServerError {
	public:
		DProgramList() : DtmActual(ZERODATETIME), Dirty(false)
		{
		}
		LDATETIME DtmActual; // Момент последней актуализации данных
		WsCtl_ProgramCollection L;
		bool Dirty; // Специальный флаг, индицирующий обновление данных со стороны потока обмена с сервером.
	};*/
	//
	// Descr: Информационный блок о состоянии процессора на сервере, с которым ассоциирована данная рабочая станция //
	//
	class DPrc : public DServerError {
	public:
		DPrc() : PrcID(0), CurrentTSessID(0), ReservedTSessID(0), DtmActual(ZERODATETIME)
		{
		}
		LDATETIME DtmActual; // Момент последней актуализации данных
		S_GUID PrcUuid;
		PPID   PrcID;
		PPID   CurrentTSessID;  // Текущая сессия процессора
		PPID   ReservedTSessID; // Ближайшая (будущая) зарезервированная сессия процессора
		SString PrcName;
	};
	class DAccount : public DServerError {
	public:
		DAccount();
		DAccount & Z();
		LDATETIME DtmActual; // Момент последней актуализации данных
		PPID   SCardID;
		PPID   PersonID;
		double ScRest;
		SString SCardCode;
		SString PersonName;
	};
	class DAuth : public DServerError {
	public:
		DAuth();
		DAuth & Z();
		enum {
			stWaitOn = 0x0001 // Объект находится в состоянии ожидания результата авторизации
		};
		LDATETIME DtmActual; // Момент последней актуализации данных
		PPID   SCardID;
		PPID   PersonID;
		uint   State;
	};
	class DPrices : public DServerError {
	public:
		DPrices();
		DPrices(const DPrices & rS);
		DPrices & FASTCALL operator = (const DPrices & rS);
		DPrices & FASTCALL Copy(const DPrices & rS);
		DPrices & Z();
		const  GoodsEntry * GetGoodsEntryByID(PPID goodsID) const;
		int    FromJsonObject(const SJson * pJsObj);
		bool   GetGoodsPrice(PPID goodsID, double * pPrice) const;

		LDATETIME DtmActual; // Момент последней актуализации данных
		TSCollection <QuotKindEntry> QkList; // Сервер передает ранжированный список. То есть, значения надо брать исходя из того, что первое приоритетнее следюущего.
		TSCollection <GoodsEntry> GoodsList;
	};
	class DTSess : public DServerError {
	public:
		DTSess();
		DTSess & Z();
		int    FromJsonObject(const SJson * pJsObj);

		LDATETIME DtmActual; // Момент последней актуализации данных
		PPID   TSessID;
		PPID   GoodsID;
		PPID   TechID;
		PPID   SCardID;
		double WrOffAmount;
		LDATETIME TmScOp;
		STimeChunk TmChunk;
	};
	//
	// Descr: Структура, описывающая текущее состояние системы.
	//   Элементы структуры могут обновляються другими потоками.
	//
	class State {
	public:
		//
		// Descr: Элемент состояния, который получен от сервера.
		//   Кроме собственно данных содержит блокировку, время актуализации и время истечения срока действия.
		//
		template <class T> class SyncEntry {
			friend class WsCtl_ImGuiSceneBlock::State;
			WsCtl_ImGuiSceneBlock::State * P_OuterState;
			SyncEntry(int syncDataId, WsCtl_ImGuiSceneBlock::State * pOuterState) : SyncDataId(syncDataId), P_OuterState(pOuterState), TmActual(0), TmExpiry(0)
			{
			}
		public:
			SyncEntry(int syncDataId) : SyncDataId(syncDataId), P_OuterState(0), TmActual(0), TmExpiry(0)
			{
			}
			void SetData(const T & rData)
			{
				Lck.Lock();
				Data = rData;
				if(P_OuterState)
					P_OuterState->D_LastErr.SetData(rData);
				Lck.Unlock();
			}
			void GetData(T & rData)
			{
				Lck.Lock();
				rData = Data;
				Lck.Unlock();
			}
			//
			// Descr: Возвращает заблокированноу ссылку на объект.
			// Attention: После того, как ссылка использована необходимо как можно скорее вызвать UnlockRef()
			//   в противном случае вся работа системы остановится!
			//
			const T & GetRef() const
			{
				const T * ptr = 0;
				Lck.Lock();
				ptr = &Data;
				return *ptr;
			}
			void UnlockRef()
			{
				Lck.Unlock();
			}
		private:
			const  int SyncDataId; // syncdataXXX
			T      Data;
			mutable SMtLock Lck; 
			int64  TmActual;
			int64  TmExpiry;
		};	
		enum {
			syncdataUndef = 0,
			syncdataTest,             // DTest    Тестовый блок данных для отладки взаимодействия с сервером 
			syncdataServerError,      // DServerError           
			syncdataPrc,              // DPrc
			syncdataAccount,          // DAccount
			syncdataPrices,           // DPrices
			syncdataTSess,            // DTSess
			syncdataJobSrvConnStatus, // int статус соединения с сервером
			syncdataAuth,             // DAuth
			syncdataAutonomousTSess,  // DTSess Поддержка актуальности автономных данных о текущей сессии
			syncdataClientPolicy,     // DPolicy Политика ограничений пользовательского сеанса
			syncdataProgramList       // D_PgmList Список программ, которые могут быть запущены из оболочки
		};

		WsCtl_SelfIdentityBlock SidBlk;
		SyncEntry <DServerError> D_LastErr;
		SyncEntry <DPrc>     D_Prc;
		SyncEntry <DAccount> D_Acc;
		SyncEntry <DTest>    D_Test;
		SyncEntry <DPrices>  D_Prices;
		SyncEntry <DTSess>   D_TSess;
		SyncEntry <DConnectionStatus> D_ConnStatus;
		SyncEntry <DAuth>    D_Auth;
		// @v11.8.6 SyncEntry <DClientPolicy> D_Policy;
		// @v11.8.6 SyncEntry <DProgramList> D_PgmList; // @v11.8.5

		State() : D_Prc(syncdataPrc), D_Test(syncdataTest), D_Acc(syncdataAccount), D_Prices(syncdataPrices), D_TSess(syncdataTSess), 
			D_ConnStatus(syncdataJobSrvConnStatus), D_Auth(syncdataAuth), SelectedTecGoodsID(0), D_LastErr(syncdataServerError)
			/*D_Policy(syncdataClientPolicy), D_PgmList(syncdataProgramList)*/
		{
		}
		PPID   GetSelectedTecGoodsID() const { return SelectedTecGoodsID; }
		bool   SetSelectedTecGoodsID(PPID goodsID);
		int    SetupSyncUpdateTime(int syncDataId, uint msecToUpdate);
		int    CheckSyncUpdateTimers(LongArray & rList) const;
		int    SetSyncUpdateTimerLastReqTime(int syncDataId);
		const  DTSess & GetAutonomousTSessEntry() const { return AutonomousTSessEntry; }
		void   UpdateAutonomousTSessEntry()
		{
			if(AutonomousTSessEntry.TSessID) {
			}
		}
	private:
		class SyncUpdateTimer {
		public:
			SyncUpdateTimer(int syncDataId, uint msecToUpdate) : SyncDataId(syncDataId), MsecToUpdate(msecToUpdate), LastReqTime(0)
			{
			}
			const int SyncDataId; // syncdataXXX
			uint  MsecToUpdate;   // Периодичность обновления данных в миллисекундах  
			time_t LastReqTime;   // Время последного запроса на обновление
		};
		TSVector <SyncUpdateTimer> SutList;
		PPID   SelectedTecGoodsID; // Выбранные товар для техсессии
		DTSess AutonomousTSessEntry; // На случай, если связь с сервером будет потеряна во время рабочей сессии,
			// это поле обязано поддерживать информацию о сеансе для своевременной остановки и прочих рабочих функций.
	};
	int GetScreen() const { return Screen; }
	int SetScreen(int scr);
	int CreateFontEntry(ImGuiIO & rIo, const char * pSymb, const char * pPath, float sizePx, const ImFontConfig * pFontCfg, const SColor * pClr);
	int PushFontEntry(ImGuiObjStack & rStk, const char * pSymb);
	SString & InputLabelPrefix(const char * pLabel)
	{
		float width = ImGui::CalcItemWidth();
		float x = ImGui::GetCursorPosX();
		ImGuiObjStack stk;
		PushFontEntry(stk, "FontSecondary");
		ImGui::Text(pLabel); 
		//ImGui::SameLine(); 
		//ImGui::SetCursorPosX(x + width * 0.5f + ImGui::GetStyle().ItemInnerSpacing.x);
		// @v11.7.8 ImGui::SetNextItemWidth(-1);
		return SLS.AcquireRvlStr().CatCharN('#', 2).Cat(pLabel);
	}
	class ImDialog_WsCtlConfig : public ImDialogState {
	public:
		ImDialog_WsCtlConfig(WsCtl_ImGuiSceneBlock & rBlk, WsCtl_Config * pCtx);
		~ImDialog_WsCtlConfig();
		virtual int Build();
		virtual bool CommitData();
	private:
		WsCtl_ImGuiSceneBlock & R_Blk;
		WsCtl_Config Data;
		char   SubstTxt_Server[128];
		char   SubstTxt_DbSymb[128];
		char   SubstTxt_User[128];
		char   SubstTxt_Password[128];
		//char   SubstTxt_
	};
private:
	//
	// Descr: Поток, реализующий запросы к серверу.
	//   Так как концепция imgui предполагает перманентную отрисовку сетевые запросы придется //
	//   делать строго асинхронно.
	//
	class WsCtl_CliSession : public PPThread {
	public:
		WsCtl_CliSession(const WsCtl_Config & rJsP, WsCtl_ImGuiSceneBlock::State * pSt, WsCtlReqQueue * pQ);
		virtual void Run();
	private:
		virtual void Startup();
		int    Connect(PPJobSrvClient & rCli);
		void   SendRequest(PPJobSrvClient & rCli, const WsCtlReqQueue::Req & rReq);
		// Указатель на состояние блока управления панелью. При получении ответа от сервера
		// наш поток будет вносить изменения в это состояние (защита блокировками подразумевается).
		WsCtl_ImGuiSceneBlock::State * P_St; // @notowned
		WsCtlReqQueue * P_Queue; // @notowned
		WsCtl_Config JsP;
	};
	bool   ShowDemoWindow; // @sobolev true-->false
	bool   ShowAnotherWindow;
	int    Screen; // screenXXX
	ImVec4 ClearColor;
	//SUiLayout Lo01; // = new SUiLayout(SUiLayoutParam(DIREC_VERT, 0, SUiLayoutParam::alignStretch));

	class Texture_CachedFileEntity : public SCachedFileEntity {
	public:
		Texture_CachedFileEntity();
		void * P_Texture;
	private:
		virtual bool InitEntity(void * extraPtr);
		virtual void DestroyEntity();
	};
	//
	class TextureCache : private TSHashCollection <Texture_CachedFileEntity> {
	public:
		TextureCache(uint initCount, const void * pCtx);
		void SetBasePath(const char * pPath);
		void MakeKey(const char * pFileName, SString & rKey);
		int    Put(Texture_CachedFileEntity * pEntry);
		Texture_CachedFileEntity * Get(const char * pSymb);
	private:
		SMtLock Lck;
		SString BasePath;
	};

	TextureCache Cache_Texture;
	TSHashCollection <SUiLayout> Cache_Layout;
	TSHashCollection <SImFontDescription> Cache_Font; // @v11.7.8
	//
	// Следующие 2 объекта загружаются при инициализации сеанса запросом к серверу или из кэша.
	//
	WsCtl_ClientPolicy PolicyL;   // @v11.8.6
	WsCtl_ProgramCollection PgmL; // @v11.7.12 Список программ, которые клиент может запустить из нашей оболочки
	
	WsCtl_Config JsP;
	UiDescription Uid; // @v11.7.12
	State  St;
	LongArray SyncReqList; // @fastreuse Список объектов состояния, для которых необходимо запросить обновление у сервера (по таймеру)
	WsCtlReqQueue * P_CmdQ; // Очередь команд для сервера. Указатель передается в совместное владение потоку обработки команд
	struct TestBlock { // @debug 
		TestBlock() : DtmLastQuerySent(ZERODATETIME), QuerySentCount(0)
		{
		}
		LDATETIME DtmLastQuerySent; // Время последней отправки тестового запроса серверу
		uint   QuerySentCount; // Количество отправленных запросов
	};
	TestBlock TestBlk;
	//
	void   Render();
	void   MakeLayout(SJson ** ppJsList);
	SUiLayout * MakePgmListLayout(const WsCtl_ProgramCollection & rPgmL);
	void   PreprocessProgramList();
	static bool GetLocalCachePath(SString & rPath);
	int    QueryProgramList2(WsCtl_ProgramCollection & rPgmL, WsCtl_ClientPolicy & rPolicyL);
	int    GetProgramListFromCache(WsCtl_ProgramCollection & rPgmL, WsCtl_ClientPolicy & rPolicyL);
	void   LoadProgramList2();
	static void LoadProgramImages(WsCtl_ProgramCollection & rPgmL, TextureCache & rTextureCache);
	void   EmitProgramGallery(ImGuiWindowByLayout & rW, SUiLayout & rTl);
	void   ErrorPopup(bool isErr);
	static int CbInput(ImGuiInputTextCallbackData * pInputData);

	//
	char   TestInput[128];
	char   LoginText[256];
	char   PwText[128];
	DServerError LastSvrErr;
	ImDialog_WsCtlConfig * P_Dlg_Cfg;
	SScroller::Position PgmGalleryScrollerPosition_Develop; // @v11.7.12
	WsCtl_SessionFrame SessF; // @v11.8.5 Блок, отвечающий за системные процедуры работы сессии (политики безопасности, ограничения ресурсов,
		// отслеживание изменений в системе etc)
public:
	WsCtl_ImGuiSceneBlock();
	~WsCtl_ImGuiSceneBlock();
	int  LoadUiDescription();
	int  Init(ImGuiIO & rIo);
	//int  LoadProgramList();
	int  ExecuteProgram(const WsCtl_ProgramEntry * pPe);
	void EmitEvents();
	void BuildScene();
};
//
//
//
WsCtl_ImGuiSceneBlock::Texture_CachedFileEntity::Texture_CachedFileEntity() : SCachedFileEntity(), P_Texture(0)
{
}

/*virtual*/bool WsCtl_ImGuiSceneBlock::Texture_CachedFileEntity::InitEntity(void * extraPtr)
{
	bool   ok = false;
	if(extraPtr) {
		ImGuiRuntimeBlock * p_rtb = static_cast<ImGuiRuntimeBlock *>(extraPtr);
		P_Texture = p_rtb->LoadTexture(GetFilePath());
		if(P_Texture)
			ok = true;
	}
	return ok;
}
		
/*virtual*/void WsCtl_ImGuiSceneBlock::Texture_CachedFileEntity::DestroyEntity()
{
	if(P_Texture) {
		static_cast<IUnknown *>(P_Texture)->Release();
		P_Texture = 0;
	}
}

WsCtl_ImGuiSceneBlock::TextureCache::TextureCache(uint initCount, const void * pCtx) : TSHashCollection <Texture_CachedFileEntity>(initCount, pCtx)
{
}
		
void WsCtl_ImGuiSceneBlock::TextureCache::SetBasePath(const char * pPath)
{
	SFsPath::NormalizePath(pPath, SFsPath::npfSlash|SFsPath::npfCompensateDotDot, BasePath);
}
		
void WsCtl_ImGuiSceneBlock::TextureCache::MakeKey(const char * pFileName, SString & rKey)
{
	rKey.Z();
	SString & r_temp_buf = SLS.AcquireRvlStr();
	(r_temp_buf = BasePath).SetLastSlash().Cat(pFileName);
	SFsPath::NormalizePath(r_temp_buf, SFsPath::npfSlash|SFsPath::npfCompensateDotDot, rKey);
}
		
int WsCtl_ImGuiSceneBlock::TextureCache::Put(Texture_CachedFileEntity * pEntry)
{
	int    result = 0;
	Lck.Lock();
	result = TSHashCollection <Texture_CachedFileEntity>::Put(pEntry, true);
	Lck.Unlock();
	return result;
}
		
WsCtl_ImGuiSceneBlock::Texture_CachedFileEntity * WsCtl_ImGuiSceneBlock::TextureCache::Get(const char * pSymb)
{
	Texture_CachedFileEntity * p_result = 0;
	if(!isempty(pSymb)) {
		Lck.Lock();
		SString & r_temp_buf = SLS.AcquireRvlStr();
		MakeKey(pSymb, r_temp_buf);
		p_result = TSHashCollection <Texture_CachedFileEntity>::Get(r_temp_buf, r_temp_buf.Len());
		Lck.Unlock();
	}
	return p_result;
}
//
//
//
WsCtl_ImGuiSceneBlock::ImDialog_WsCtlConfig::ImDialog_WsCtlConfig(WsCtl_ImGuiSceneBlock & rBlk, WsCtl_Config * pCtx) : R_Blk(rBlk), ImDialogState(pCtx)
{
	if(pCtx) {
		Data = *pCtx;
	}
	STRNSCPY(SubstTxt_Server, Data.Server);
	STRNSCPY(SubstTxt_DbSymb, Data.DbSymb);
	STRNSCPY(SubstTxt_User, Data.User);
	STRNSCPY(SubstTxt_Password, Data.Password);
}
		
WsCtl_ImGuiSceneBlock::ImDialog_WsCtlConfig::~ImDialog_WsCtlConfig()
{
	memzero(SubstTxt_Server, sizeof(SubstTxt_Server));
	memzero(SubstTxt_DbSymb, sizeof(SubstTxt_DbSymb));
	memzero(SubstTxt_User, sizeof(SubstTxt_User));
	memzero(SubstTxt_Password, sizeof(SubstTxt_Password));
}
		
/*virtual*/int WsCtl_ImGuiSceneBlock::ImDialog_WsCtlConfig::Build()
{
	int    result = 0;
	const char * p_popup_title = "Config";
	ImGui::OpenPopup(p_popup_title);
	if(ImGui::BeginPopup(p_popup_title)) {
		ImGui::InputText(R_Blk.InputLabelPrefix("server"), SubstTxt_Server, sizeof(SubstTxt_Server));
		ImGui::InputInt(R_Blk.InputLabelPrefix("port"), &Data.Port);
		ImGui::InputText(R_Blk.InputLabelPrefix("dbsymb"), SubstTxt_DbSymb, sizeof(SubstTxt_DbSymb));
		ImGui::InputText(R_Blk.InputLabelPrefix("user"), SubstTxt_User, sizeof(SubstTxt_User));
		ImGui::InputText(R_Blk.InputLabelPrefix("password"), SubstTxt_Password, sizeof(SubstTxt_Password));
		ImGui::NewLine();
		if(ImGui::Button("ok", ButtonSize_Std)) {
			ImGui::CloseCurrentPopup();
			result = 1;
		}
		ImGui::SameLine();
		if(ImGui::Button("cancel", ButtonSize_Std)) {
			ImGui::CloseCurrentPopup();
			result = -1;
		}
		ImGui::EndPopup();
	}
	return result;
}
		
/*virtual*/bool WsCtl_ImGuiSceneBlock::ImDialog_WsCtlConfig::CommitData()
{
	bool   ok = true;
	if(P_Ctx) {
		Data.Server = SubstTxt_Server;
		Data.DbSymb = SubstTxt_DbSymb;
		Data.User = SubstTxt_User;
		Data.Password = SubstTxt_Password;
		*static_cast<WsCtl_Config *>(P_Ctx) = Data;
	}
	else
		ok = false;
	return ok;
}

WsCtl_ImGuiSceneBlock::DTSess::DTSess() : TSessID(0), GoodsID(0), TechID(0), SCardID(0), WrOffAmount(0.0), DtmActual(ZERODATETIME)
{
}
		
WsCtl_ImGuiSceneBlock::DTSess & WsCtl_ImGuiSceneBlock::DTSess::Z()
{
	DServerError::Z();
	DtmActual.Z();
	TSessID = 0;
	GoodsID = 0;
	TechID = 0;
	SCardID = 0;
	WrOffAmount = 0.0;
	TmChunk.Z();
	TmScOp.Z();
	return *this;
}

int WsCtl_ImGuiSceneBlock::DTSess::FromJsonObject(const SJson * pJsObj)
{
	int    ok = 0;
	Z();
	if(SJson::IsObject(pJsObj)) {
		bool is_there_tsess_id = false;
		for(const SJson * p_cur = pJsObj->P_Child; p_cur; p_cur = p_cur->P_Next) {
			if(p_cur->P_Child) {
				if(p_cur->Text.IsEqiAscii("tsessid")) {
					TSessID = p_cur->P_Child->Text.ToLong();
					is_there_tsess_id = true;
				}
				else if(p_cur->Text.IsEqiAscii("techid")) {
					TechID = p_cur->P_Child->Text.ToLong();
				}
				else if(p_cur->Text.IsEqiAscii("goodsid")) {
					GoodsID = p_cur->P_Child->Text.ToLong();
				}
				else if(p_cur->Text.IsEqiAscii("tm_start")) {
					strtodatetime(p_cur->P_Child->Text, &TmChunk.Start, DATF_ISO8601CENT, TIMF_HMS);
				}
				else if(p_cur->Text.IsEqiAscii("tm_finish")) {
					strtodatetime(p_cur->P_Child->Text, &TmChunk.Finish, DATF_ISO8601CENT, TIMF_HMS);
				}
				else if(p_cur->Text.IsEqiAscii("scardid")) {
					SCardID = p_cur->P_Child->Text.ToLong();
				}
				else if(p_cur->Text.IsEqiAscii("wroffamt")) {
					WrOffAmount = p_cur->P_Child->Text.ToReal();
				}
				else if(p_cur->Text.IsEqiAscii("tm_scop")) {
					strtodatetime(p_cur->P_Child->Text, &TmScOp, DATF_ISO8601CENT, TIMF_HMS);
				}
			}
		}
		if(is_there_tsess_id) {
			ok = 1;
		}
	}
	return ok;
}

WsCtl_ImGuiSceneBlock::DServerError::DServerError() : _Status(0)
{
}

WsCtl_ImGuiSceneBlock::DServerError::DServerError(const DServerError & rS)
{
	Copy(rS);
}

WsCtl_ImGuiSceneBlock::DServerError & FASTCALL WsCtl_ImGuiSceneBlock::DServerError::operator = (const DServerError & rS)
{
	return Copy(rS);
}

WsCtl_ImGuiSceneBlock::DServerError & WsCtl_ImGuiSceneBlock::DServerError::Copy(const DServerError & rS)
{
	_Status = rS._Status;
	_Message = rS._Message;
	return *this;
}

WsCtl_ImGuiSceneBlock::DServerError & WsCtl_ImGuiSceneBlock::DServerError::Z()
{
	_Status = 0;
	_Message.Z();
	return *this;
}

WsCtl_ImGuiSceneBlock::DServerError & WsCtl_ImGuiSceneBlock::DServerError::SetupByLastError()
{
	_Status = PPErrCode;
	PPGetLastErrorMessage(1, _Message);
	_Message.Transf(CTRANSF_INNER_TO_UTF8);
	return *this;
}

WsCtl_ImGuiSceneBlock::DAuth::DAuth() : State(0), SCardID(0), PersonID(0), DtmActual(ZERODATETIME)
{
}
		
WsCtl_ImGuiSceneBlock::DAuth & WsCtl_ImGuiSceneBlock::DAuth::Z()
{
	DServerError::Z();
	DtmActual.Z();
	State = 0;
	SCardID = 0;
	PersonID = 0;
	DServerError::Z();
	return *this;
}

WsCtl_ImGuiSceneBlock::DAccount::DAccount() : SCardID(0), PersonID(0), ScRest(0.0), DtmActual(ZERODATETIME)
{
}
		
WsCtl_ImGuiSceneBlock::DAccount & WsCtl_ImGuiSceneBlock::DAccount::Z()
{
	DServerError::Z();
	SCardID = 0;
	PersonID = 0;
	ScRest = 0.0;
	SCardCode.Z();
	PersonName.Z();
	return *this;
}

WsCtl_ImGuiSceneBlock::DPrices::DPrices() : DtmActual(ZERODATETIME)
{
}
		
WsCtl_ImGuiSceneBlock::DPrices::DPrices(const DPrices & rS)
{
	Copy(rS);
}

WsCtl_ImGuiSceneBlock::DPrices & FASTCALL WsCtl_ImGuiSceneBlock::DPrices::operator = (const DPrices & rS)
{
	return Copy(rS);
}
		
WsCtl_ImGuiSceneBlock::DPrices & FASTCALL WsCtl_ImGuiSceneBlock::DPrices::Copy(const DPrices & rS)
{
	DServerError::Copy(rS);
	DtmActual = rS.DtmActual;
	TSCollection_Copy(QkList, rS.QkList);
	TSCollection_Copy(GoodsList, rS.GoodsList);
	return *this;
}

WsCtl_ImGuiSceneBlock::DPrices & WsCtl_ImGuiSceneBlock::DPrices::Z()
{
	DServerError::Z();
	DtmActual.Z();
	QkList.clear();
	GoodsList.clear();
	return *this;
}
		
const WsCtl_ImGuiSceneBlock::GoodsEntry * WsCtl_ImGuiSceneBlock::DPrices::GetGoodsEntryByID(PPID goodsID) const
{
	const  GoodsEntry * p_result = 0;
	for(uint i = 0; !p_result && i < GoodsList.getCount(); i++) {
		GoodsEntry * p_iter = GoodsList.at(i);
		if(p_iter && p_iter->ID == goodsID)
			p_result = p_iter;
	}
	return p_result;
}

bool WsCtl_ImGuiSceneBlock::DPrices::GetGoodsPrice(PPID goodsID, double * pPrice) const
{
	bool   ok = false;
	double price = 0.0;
	const GoodsEntry * p_goods_entry = GetGoodsEntryByID(goodsID);
	if(p_goods_entry) {
		const uint gqlc_ = p_goods_entry->QuotList.getCount();
		if(gqlc_) {
			for(uint i = 0; !ok && i < QkList.getCount(); i++) {
				const  PPID qk_id = QkList.at(i)->ID;
				for(uint j = 0; !ok && j < gqlc_; j++) {
					const QuotEntry & r_qe = p_goods_entry->QuotList.at(j);
					if(r_qe.QkID == qk_id && r_qe.Value > 0.0) {
						price = r_qe.Value;
						ok = true;
					}
				}
			}
		}
	}
	ASSIGN_PTR(pPrice, price);
	return ok;
}
		
int WsCtl_ImGuiSceneBlock::DPrices::FromJsonObject(const SJson * pJsObj)
{
	int    ok = 1;
	Z();
	if(pJsObj && pJsObj->Type == SJson::tOBJECT) {
		const SJson * p_next = 0;
		for(const SJson * p_cur = pJsObj->P_Child; p_cur; p_cur = p_next) {
			p_next = p_cur->P_Next;
			if(p_cur->P_Child) {
				if(p_cur->Text.IsEqiAscii("qk_list")) {
					if(SJson::IsArray(p_cur->P_Child)) {
						for(const SJson * p_js_qk = p_cur->P_Child->P_Child; p_js_qk; p_js_qk = p_js_qk->P_Next) {
							if(SJson::IsObject(p_js_qk)) {
								QuotKindEntry * p_entry = QkList.CreateNewItem();
								for(const SJson * p_qk_cur = p_js_qk->P_Child; p_qk_cur; p_qk_cur = p_qk_cur->P_Next) {
									if(p_qk_cur->Text.IsEqiAscii("id")) {
										p_entry->ID = p_qk_cur->P_Child->Text.ToLong();
									}
									else if(p_qk_cur->Text.IsEqiAscii("nm")) {
										(p_entry->NameUtf8 = p_qk_cur->P_Child->Text).Unescape();
									}
									else if(p_qk_cur->Text.IsEqiAscii("rank")) {
										p_entry->Rank = p_qk_cur->P_Child->Text.ToLong();
									}
									else if(p_qk_cur->Text.IsEqiAscii("daysofweek")) {
										p_entry->DaysOfWeek = p_qk_cur->P_Child->Text.ToLong();
									}
									else if(p_qk_cur->Text.IsEqiAscii("begintm")) {
										strtotime(p_qk_cur->P_Child->Text, TIMF_HM, &p_entry->ApplyTm.low);
									}
									else if(p_qk_cur->Text.IsEqiAscii("endtm")) {
										strtotime(p_qk_cur->P_Child->Text, TIMF_HM, &p_entry->ApplyTm.upp);
									}
								}
							}
						}
					}
				}
				else if(p_cur->Text.IsEqiAscii("goods_list")) {
					if(SJson::IsArray(p_cur->P_Child)) {
						for(const SJson * p_js_gl = p_cur->P_Child->P_Child; p_js_gl; p_js_gl = p_js_gl->P_Next) {
							if(SJson::IsObject(p_js_gl)) {
								GoodsEntry * p_entry = GoodsList.CreateNewItem();
								for(const SJson * p_g_cur = p_js_gl->P_Child; p_g_cur; p_g_cur = p_g_cur->P_Next) {
									if(p_g_cur->Text.IsEqiAscii("id")) {
										p_entry->ID = p_g_cur->P_Child->Text.ToLong();
									}
									else if(p_g_cur->Text.IsEqiAscii("nm")) {
										(p_entry->NameUtf8 = p_g_cur->P_Child->Text).Unescape();
									}
									else if(p_g_cur->Text.IsEqiAscii("tech_list")) {
										if(SJson::IsArray(p_g_cur->P_Child)) {
											for(const SJson * p_tec_cur = p_g_cur->P_Child; p_tec_cur; p_tec_cur = p_tec_cur->P_Next) {
												if(SJson::IsObject(p_tec_cur->P_Child)) {
													for(const SJson * p_t_cur = p_tec_cur->P_Child; p_t_cur; p_t_cur = p_t_cur->P_Next) {
														if(p_t_cur->IsObject()) {
															TechEntry t_entry;
															for(const SJson * p_gt_obj = p_t_cur->P_Child; p_gt_obj; p_gt_obj = p_gt_obj->P_Next) {
																if(p_gt_obj->Text.IsEqiAscii("id")) {
																	t_entry.ID = p_gt_obj->P_Child->Text.ToLong();
																}
																else if(p_gt_obj->Text.IsEqiAscii("cod")) {
																	SString & r_temp_buf = SLS.AcquireRvlStr();
																	STRNSCPY(t_entry.CodeUtf8, (r_temp_buf = p_gt_obj->P_Child->Text).Unescape());
																}
															}
															if(t_entry.ID) {
																p_entry->TechList.insert(&t_entry);
															}
														}
													}
												}
											}
										}
									}
									else if(p_g_cur->Text.IsEqiAscii("quot_list")) {
										if(SJson::IsArray(p_g_cur->P_Child)) {
											for(const SJson * p_quot_cur = p_g_cur->P_Child; p_quot_cur; p_quot_cur = p_quot_cur->P_Next) {
												if(SJson::IsObject(p_quot_cur->P_Child)) {
													for(const SJson * p_q_cur = p_quot_cur->P_Child; p_q_cur; p_q_cur = p_q_cur->P_Next) {
														if(p_q_cur->IsObject()) {
															QuotEntry q_entry;
															for(const SJson * p_gq_obj = p_q_cur->P_Child; p_gq_obj; p_gq_obj = p_gq_obj->P_Next) {
																if(p_gq_obj->Text.IsEqiAscii("id")) {
																	q_entry.QkID = p_gq_obj->P_Child->Text.ToLong();
																}
																else if(p_gq_obj->Text.IsEqiAscii("val")) {
																	q_entry.Value = p_gq_obj->P_Child->Text.ToReal_Plain();
																}
															}
															if(q_entry.QkID && q_entry.Value > 0.0) {
																p_entry->QuotList.insert(&q_entry);
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
					}
				}
			}
		}
	}
	if(!ok)
		PPSetError(PPERR_SQ_JSONTOCFGFAULT);
	return ok;
}
//
//
//
bool WsCtl_ImGuiSceneBlock::State::SetSelectedTecGoodsID(PPID goodsID)
{
	if(goodsID != SelectedTecGoodsID) {
		SelectedTecGoodsID = goodsID;
		return true;
	}
	else
		return false;
}

int WsCtl_ImGuiSceneBlock::State::SetupSyncUpdateTime(int syncDataId, uint msecToUpdate)
{
	int    ok = -1;
	bool   found = false;
	for(uint i = 0; !found && i < SutList.getCount(); i++) {
		SyncUpdateTimer & r_entry = SutList.at(i);
		if(r_entry.SyncDataId == syncDataId) {
			if(r_entry.MsecToUpdate != msecToUpdate) {
				r_entry.MsecToUpdate = msecToUpdate;
				ok = 1;
			}
			found = true;
		}
	}
	if(!found) {
		SyncUpdateTimer new_entry(syncDataId, msecToUpdate);
		SutList.insert(&new_entry);
		ok = 2;
	}
	return ok;
}

int WsCtl_ImGuiSceneBlock::State::CheckSyncUpdateTimers(LongArray & rList) const
{
	rList.Z();
	int    ok = -1;
	const  time_t now_time = time(0);
	for(uint i = 0; i < SutList.getCount(); i++) {
		const SyncUpdateTimer & r_entry = SutList.at(i);
		if(r_entry.LastReqTime == 0 || now_time >= (r_entry.LastReqTime + (r_entry.MsecToUpdate / 1000))) {
			rList.insert(&r_entry.SyncDataId);
			ok = 1;
		}
	}
	return ok;
}
		
int WsCtl_ImGuiSceneBlock::State::SetSyncUpdateTimerLastReqTime(int syncDataId)
{
	int    ok = -1;
	bool   found = false;
	for(uint i = 0; !found && i < SutList.getCount(); i++) {
		SyncUpdateTimer & r_entry = SutList.at(i);
		if(r_entry.SyncDataId == syncDataId) {
			r_entry.LastReqTime = time(0);
			ok = 1;
			found = true;
		}
	}
	return ok;
}
//
//
//
WsCtl_ImGuiSceneBlock::WsCtl_CliSession::WsCtl_CliSession(const WsCtl_Config & rJsP, WsCtl_ImGuiSceneBlock::State * pSt, WsCtlReqQueue * pQ) : 
	PPThread(PPThread::kWsCtl, 0, 0), JsP(rJsP), P_St(pSt), P_Queue(pQ)
{
	InitStartupSignal();
}

/*virtual*/void WsCtl_ImGuiSceneBlock::WsCtl_CliSession::Run()
{
	SString temp_buf;
	Evnt   stop_event(SLS.GetStopEventName(temp_buf), Evnt::modeOpen);
	const  uint32 wait_timeout = 500;

	PPJobSrvClient cli;
	PPJobSrvReply reply;
	{
		DConnectionStatus srv_conn_status;
		srv_conn_status.S = Connect(cli);
		if(!srv_conn_status.S)
			srv_conn_status.SetupByLastError();
		P_St->D_ConnStatus.SetData(srv_conn_status);
	}
	for(int stop = 0; !stop;) {
		int    h_count = 0;
		int    evidx_stop = -1;
		int    evidx_dcn = -1;
		int    evidx_forceperiod = -1;
		int    evidx_localforceperiod = -1;
		HANDLE h_list[32];
		{
			evidx_stop = h_count++;
			h_list[evidx_stop] = stop_event;     // #0
		}
		h_list[h_count++] = P_Queue->NonEmptyEv; // #1
		const  uint r = ::WaitForMultipleObjects(h_count, h_list, 0, wait_timeout);
		bool   do_check_queue = false;
		switch(r) {
			case WAIT_TIMEOUT:
				do_check_queue = true;
				break;
			case (WAIT_OBJECT_0 + 0): // stop event
				stop = 1; // quit loop
				break;
			case (WAIT_OBJECT_0 + 1): // NonEmptyEv
				do_check_queue = true;
				break;
			case WAIT_FAILED:
				; // @error
				break;
		}
		if(do_check_queue) {
			uint32 single_ev_count = 0;
			WsCtlReqQueue::Req req;
			while(P_Queue->Pop(req) > 0) {
				single_ev_count++;
				SendRequest(cli, req);
				//if(PPSession::Helper_Log(msg_item, lb) > 0) {
					//S.OutputCount++;
				//}
			}
			if(single_ev_count) {
				if(!(cli.GetState() & PPJobSrvClient::stConnected)) {
					DConnectionStatus srv_conn_status;
					srv_conn_status.S = cli.Connect(JsP.Server, JsP.Port);
					if(!srv_conn_status.S)
						srv_conn_status.SetupByLastError();
					P_St->D_ConnStatus.SetData(srv_conn_status);
				}
				/*if(srv_conn_r > 0) {
							
				}*/
				/*
				if(single_ev_count > S.MaxSingleOutputCount)
					S.MaxSingleOutputCount = single_ev_count;
				*/
			}
			else {
				//S.FalseNonEmptyEvSwitchCount++;
			}					
		}
	}
}

/*virtual*/void WsCtl_ImGuiSceneBlock::WsCtl_CliSession::Startup()
{
	PPThread::Startup();
	SignalStartup();
}

int WsCtl_ImGuiSceneBlock::WsCtl_CliSession::Connect(PPJobSrvClient & rCli)
{
	int    ok = 1;
	THROW(rCli.Connect(JsP.Server, JsP.Port));
	if(JsP.DbSymb.NotEmpty() && JsP.User.NotEmpty()) {
		THROW(rCli.Login(JsP.DbSymb, JsP.User, JsP.Password));
	}
	CATCHZOK
	return ok;
}

void WsCtl_ImGuiSceneBlock::WsCtl_CliSession::SendRequest(PPJobSrvClient & rCli, const WsCtlReqQueue::Req & rReq)
{
	PPJobSrvReply reply;
	SString temp_buf;
	switch(rReq.Cmd) {
		case PPSCMD_HELLO:
			if(P_St) {
				WsCtl_ImGuiSceneBlock::DTest st_data;
				if(rCli.ExecSrvCmd("HELLO", PPConst::DefSrvCmdTerm, reply) && reply.StartReading(&temp_buf)) {
					if(reply.CheckRepError()) {
						(st_data.Reply = "OK").CatDiv(':', 2).Cat(temp_buf);
					}
					else
						st_data.SetupByLastError();
				}
				else
					st_data.SetupByLastError();
				P_St->D_Test.SetData(st_data);
			}
			break;
		case PPSCMD_WSCTL_END_SESS:
			if(P_St) {
				WsCtl_ImGuiSceneBlock::DPrc st_prc;
				WsCtl_ImGuiSceneBlock::DAccount st_acc;
				WsCtl_ImGuiSceneBlock::DAuth st_auth;
				WsCtl_ImGuiSceneBlock::DTSess st_tsess;
				P_St->D_Prc.GetData(st_prc);
				if(!!st_prc.PrcUuid && rReq.P.TSessID && rReq.P.SCardID) {
					PPJobSrvCmd cmd;
					cmd.StartWriting(PPSCMD_WSCTL_END_SESS);
					{
						SJson js_param(SJson::tOBJECT);
						js_param.InsertString("wsctluuid", temp_buf.Z().Cat(st_prc.PrcUuid, S_GUID::fmtIDL));
						js_param.InsertInt("scardid", rReq.P.SCardID);
						js_param.InsertInt("tsessid", rReq.P.TSessID);
						js_param.ToStr(temp_buf);
						SString mime_buf;
						mime_buf.EncodeMime64(temp_buf.ucptr(), temp_buf.Len());
						cmd.Write(mime_buf.ucptr(), mime_buf.Len()+1);
					}
					cmd.FinishWriting();
					if(rCli.ExecSrvCmd(cmd, PPConst::DefSrvCmdTerm, reply)) {
						SString reply_buf;
						reply.StartReading(&reply_buf);
						if(reply.CheckRepError()) {
							st_tsess.Z();							
						}
						else
							st_tsess.SetupByLastError();
					}
					else
						st_tsess.SetupByLastError();
					st_tsess.DtmActual = getcurdatetime_();
					P_St->D_TSess.SetData(st_tsess);
				}
			}
			break;
		case PPSCMD_WSCTL_LOGOUT:
			if(P_St) {
				WsCtl_ImGuiSceneBlock::DPrc st_prc;
				WsCtl_ImGuiSceneBlock::DAccount st_acc;
				WsCtl_ImGuiSceneBlock::DAuth st_auth;
				WsCtl_ImGuiSceneBlock::DTSess st_tsess;
				P_St->D_Prc.GetData(st_prc);
				P_St->D_Acc.GetData(st_acc);
				if(!!st_prc.PrcUuid && st_acc.SCardID) {
					PPJobSrvCmd cmd;
					cmd.StartWriting(PPSCMD_WSCTL_LOGOUT);
					{
						SJson js_param(SJson::tOBJECT);
						js_param.InsertString("wsctluuid", temp_buf.Z().Cat(st_prc.PrcUuid, S_GUID::fmtIDL));
						js_param.InsertInt("scardid", st_acc.SCardID);
						js_param.ToStr(temp_buf);
						SString mime_buf;
						mime_buf.EncodeMime64(temp_buf.ucptr(), temp_buf.Len());
						cmd.Write(mime_buf.ucptr(), mime_buf.Len()+1);
					}
					cmd.FinishWriting();
					if(rCli.ExecSrvCmd(cmd, PPConst::DefSrvCmdTerm, reply)) {
						SString reply_buf;
						reply.StartReading(&reply_buf);
						if(reply.CheckRepError()) {
							st_acc.Z();
							st_auth.Z();
							st_tsess.Z();
						}
						else {
							st_acc.Z();
							st_auth.Z();
							st_tsess.Z();
							st_acc.SetupByLastError();
						}
					}
					else {
						st_acc.Z();
						st_auth.Z();
						st_tsess.Z();
						st_acc.SetupByLastError();
					}
					const LDATETIME now_dtm = getcurdatetime_();
					st_acc.DtmActual = now_dtm;
					P_St->D_Acc.SetData(st_acc);
					st_auth.DtmActual = now_dtm;
					P_St->D_Auth.SetData(st_auth);
					st_tsess.DtmActual = now_dtm;
					P_St->D_TSess.SetData(st_tsess);
				}
			}
			break;
		case PPSCMD_WSCTL_AUTH:
			if(P_St) {
				bool   data_settled = false; // Признак того, что данные в блоке состояния инициализированы 
				WsCtl_ImGuiSceneBlock::DAuth st_data;
				WsCtl_ImGuiSceneBlock::DPrc st_prc;
				P_St->D_Prc.GetData(st_prc); 
				//P_St->D_Auth.GetData(st_data);
				PPJobSrvCmd cmd;
				cmd.StartWriting(PPSCMD_WSCTL_AUTH);
				{
					SJson js_param(SJson::tOBJECT);
					js_param.InsertString("login", rReq.P.AuthTextUtf8);
					js_param.InsertString("pw", rReq.P.AuthPwUtf8);
					temp_buf.Z().Cat(st_prc.PrcUuid, S_GUID::fmtIDL);
					js_param.InsertString("wsctluuid", temp_buf);
					js_param.ToStr(temp_buf);
					SString mime_buf;
					mime_buf.EncodeMime64(temp_buf.ucptr(), temp_buf.Len());
					cmd.Write(mime_buf.ucptr(), mime_buf.Len()+1);
				}
				cmd.FinishWriting();
				if(rCli.ExecSrvCmd(cmd, PPConst::DefSrvCmdTerm, reply)) {
					SString reply_buf;
					reply.StartReading(&reply_buf);
					if(reply.CheckRepError()) {
						SJson * p_js_obj = SJson::Parse(reply_buf);
						const SJson * p_c = 0;
						if(p_js_obj) {
							DTSess st_tsess_data;
							bool is_tsess_data_valid = false;
							p_c = p_js_obj->FindChildByKey("scardid");
							if(SJson::IsNumber(p_c)) {
								st_data.SCardID = p_c->Text.ToLong();
							}
							p_c = p_js_obj->FindChildByKey("personid");
							if(SJson::IsNumber(p_c)) {
								st_data.PersonID = p_c->Text.ToLong();
							}
							p_c = p_js_obj->FindChildByKey("tsess");
							if(SJson::IsObject(p_c)) {
								if(st_tsess_data.FromJsonObject(p_c)) {
									is_tsess_data_valid = false;
								}

							}
							st_data.State = 0;
							P_St->D_Auth.SetData(st_data);
							if(is_tsess_data_valid) {
								st_tsess_data.DtmActual = getcurdatetime_();
								P_St->D_TSess.SetData(st_tsess_data);
							}
							data_settled = true; 
							{
								// Сразу отправляем запрос на получение сведений об аккаунте
								WsCtlReqQueue::Req inner_req(PPSCMD_WSCTL_GETACCOUNTSTATE);
								inner_req.P.SCardID = st_data.SCardID;
								SendRequest(rCli, inner_req); // @recursion
							}
						}
						else {
							PPSetErrorSLib();
							st_data.SetupByLastError();
						}
						ZDELETE(p_js_obj);
					}
					else {
						st_data.SetupByLastError();
					}
				}
				else
					st_data.SetupByLastError();
				if(!data_settled) {
					st_data.DtmActual = getcurdatetime_();
					P_St->D_Auth.SetData(st_data);
				}
			}
			break;
		case PPSCMD_WSCTL_GETACCOUNTSTATE:
			if(P_St && rReq.P.SCardID) {
				//WsCtl_ImGuiSceneBlock::DAuth st_auth;
				WsCtl_ImGuiSceneBlock::DAccount st_data;
				//P_St->D_Auth.GetData(st_auth);
				PPJobSrvCmd cmd;
				cmd.StartWriting(PPSCMD_WSCTL_GETACCOUNTSTATE);
				cmd.Write(&rReq.P.SCardID, sizeof(rReq.P.SCardID));
				cmd.FinishWriting();
				if(rCli.ExecSrvCmd(cmd, PPConst::DefSrvCmdTerm, reply)) {
					SString reply_buf;
					reply.StartReading(&reply_buf);
					if(reply.CheckRepError()) {
						SJson * p_js_obj = SJson::Parse(reply_buf);
						const SJson * p_c = 0;
						if(p_js_obj) {
							p_c = p_js_obj->FindChildByKey("scardid");
							if(SJson::IsNumber(p_c)) {
								st_data.SCardID = p_c->Text.ToLong();
							}
							p_c = p_js_obj->FindChildByKey("scardcode");
							if(SJson::IsString(p_c)) {
								(st_data.SCardCode = p_c->Text).Unescape();
							}
							p_c = p_js_obj->FindChildByKey("personid");
							if(SJson::IsNumber(p_c)) {
								st_data.PersonID = p_c->Text.ToLong();
							}
							p_c = p_js_obj->FindChildByKey("personnm");
							if(SJson::IsString(p_c)) {
								(st_data.PersonName = p_c->Text).Unescape();
							}										
							p_c = p_js_obj->FindChildByKey("screst");
							if(SJson::IsNumber(p_c)) {
								st_data.ScRest = p_c->Text.ToReal();
							}
						}
						else {
							PPSetErrorSLib();
							st_data.SetupByLastError();
						}
						ZDELETE(p_js_obj);
					}
					else
						st_data.SetupByLastError();
				}
				else
					st_data.SetupByLastError();
				st_data.DtmActual = getcurdatetime_();
				P_St->D_Acc.SetData(st_data);
			}
			break;
		case PPSCMD_WSCTL_INIT:
			if(P_St) {
				WsCtl_ImGuiSceneBlock::DPrc st_data;
				PPJobSrvCmd cmd;
				cmd.StartWriting(PPSCMD_WSCTL_INIT);
				cmd.Write(&rReq.P.Uuid, sizeof(rReq.P.Uuid));
				cmd.FinishWriting();
				if(rCli.ExecSrvCmd(cmd, PPConst::DefSrvCmdTerm, reply)) {
					SString reply_buf;
					reply.StartReading(&reply_buf);
					if(reply.CheckRepError()) {
						SJson * p_js_obj = SJson::Parse(reply_buf);
						const SJson * p_c = 0;
						if(p_js_obj) {
							p_c = p_js_obj->FindChildByKey("prcid");
							if(SJson::IsNumber(p_c)) {
								st_data.PrcID = p_c->Text.ToLong();
							}
							p_c = p_js_obj->FindChildByKey("prcnm");
							if(SJson::IsString(p_c)) {
								st_data.PrcName = p_c->Text;
							}
							p_c = p_js_obj->FindChildByKey("wsctluuid");
							if(SJson::IsString(p_c)) {
								st_data.PrcUuid.FromStr(p_c->Text);
							}
							// @v11.7.12 {
							{
								// Сразу отправляем запрос на получение клиентской политики
								WsCtlReqQueue::Req inner_req(PPSCMD_WSCTL_QUERYPOLICY);
								inner_req.P.Uuid = rReq.P.Uuid;
								SendRequest(rCli, inner_req); // @recursion
							}
							// } @v11.7.12 
							{
								// И следом запрос на список программ для запуска
								WsCtlReqQueue::Req inner_req(PPSCMD_WSCTL_QUERYPGMLIST);
								inner_req.P.Uuid = rReq.P.Uuid;
								SendRequest(rCli, inner_req); // @recursion
							}
						}
						else {
							PPSetErrorSLib();
							st_data.SetupByLastError();
						}
						ZDELETE(p_js_obj);
					}
					else
						st_data.SetupByLastError();
				}
				else
					st_data.SetupByLastError();
				st_data.DtmActual = getcurdatetime_();
				P_St->D_Prc.SetData(st_data);
			}
			break;
		/* @v11.8.6
		case PPSCMD_WSCTL_QUERYPGMLIST: // @v11.8.5
			if(P_St) {
				WsCtl_ImGuiSceneBlock::DProgramList st_data;
				WsCtl_ImGuiSceneBlock::DProgramList st_data_org;
				P_St->D_PgmList.GetData(st_data_org);
				PPJobSrvCmd cmd;
				cmd.StartWriting(PPSCMD_WSCTL_QUERYPGMLIST);
				cmd.Write(&rReq.P.Uuid, sizeof(rReq.P.Uuid));
				cmd.FinishWriting();
				if(rCli.ExecSrvCmd(cmd, PPConst::DefSrvCmdTerm, reply)) {
					SString reply_buf;
					reply.StartReading(&reply_buf);
					if(reply.CheckRepError()) {
						SJson * p_js_obj = SJson::Parse(reply_buf);
						if(st_data.L.FromJsonObj(p_js_obj)) {
							if(st_data.L != st_data_org.L) {
								st_data.DtmActual = getcurdatetime_();
								st_data.Dirty = true;
								P_St->D_PgmList.SetData(st_data);
							}
						}
						else {
							PPSetErrorSLib();
							st_data.SetupByLastError();
						}
						ZDELETE(p_js_obj);						
					}
				}
			}
			break;
		case PPSCMD_WSCTL_QUERYPOLICY: // @v11.7.12 @construction
			if(P_St) {
				WsCtl_ImGuiSceneBlock::DClientPolicy st_data;
				WsCtl_ImGuiSceneBlock::DClientPolicy st_data_org;
				P_St->D_Policy.GetData(st_data_org);
				PPJobSrvCmd cmd;
				cmd.StartWriting(PPSCMD_WSCTL_QUERYPOLICY);
				cmd.Write(&rReq.P.Uuid, sizeof(rReq.P.Uuid));
				cmd.FinishWriting();
				if(rCli.ExecSrvCmd(cmd, PPConst::DefSrvCmdTerm, reply)) {
					SString reply_buf;
					reply.StartReading(&reply_buf);
					if(reply.CheckRepError()) {
						SJson * p_js_obj = SJson::Parse(reply_buf);
						if(st_data.P.FromJsonObj(p_js_obj)) {
							if(st_data.P != st_data_org.P) {
								st_data.DtmActual = getcurdatetime_();
								st_data.Dirty = true;
								P_St->D_Policy.SetData(st_data);
							}
						}
						else {
							PPSetErrorSLib();
							st_data.SetupByLastError();
						}
						ZDELETE(p_js_obj);						
					}
				}
			}
			break;
		*/
		case PPSCMD_WSCTL_GETQUOTLIST:
			if(P_St) {
				WsCtl_ImGuiSceneBlock::DPrices st_data;
				WsCtl_ImGuiSceneBlock::DPrc st_prc_data;
				P_St->D_Prc.GetData(st_prc_data);
				if(st_prc_data.PrcID) {
					PPJobSrvCmd cmd;
					cmd.StartWriting(PPSCMD_WSCTL_GETQUOTLIST);
					cmd.Write(&st_prc_data.PrcID, sizeof(st_prc_data.PrcID));
					cmd.FinishWriting();
					if(rCli.ExecSrvCmd(cmd, PPConst::DefSrvCmdTerm, reply)) {
						SString reply_buf;
						reply.StartReading(&reply_buf);
						if(reply.CheckRepError()) {
							SJson * p_js = SJson::Parse(reply_buf);
							if(st_data.FromJsonObject(p_js)) {
								;
							}
							else
								st_data.SetupByLastError();
							ZDELETE(p_js);
						}
						else
							st_data.SetupByLastError();
					}
				}
				else {
					// @todo Сделать нормальную ошибку
					st_data._Status = -1;
					st_data._Message = "Processor is undefined";
				}
				st_data.DtmActual = getcurdatetime_();
				P_St->D_Prices.SetData(st_data);
			}
			break;
		case PPSCMD_WSCTL_TSESS:
			if(P_St) {
				DTSess st_data;
				WsCtl_ImGuiSceneBlock::DPrc st_prc_data;
				P_St->D_Prc.GetData(st_prc_data);
				P_St->D_TSess.GetData(st_data);
				if(st_prc_data.PrcID) {
					PPJobSrvCmd cmd;

					SJson js_param(SJson::tOBJECT);
					js_param.InsertString("wsctluuid", temp_buf.Z().Cat(st_prc_data.PrcUuid, S_GUID::fmtIDL).Escape());
					js_param.InsertInt("prcid", st_prc_data.PrcID);
					js_param.InsertInt("tsessid", st_data.TSessID);
					cmd.StartWriting(PPSCMD_WSCTL_TSESS);
					{
						js_param.ToStr(temp_buf);
						SString mime_buf;
						mime_buf.EncodeMime64(temp_buf.ucptr(), temp_buf.Len());
						cmd.Write(mime_buf.ucptr(), mime_buf.Len()+1);
					}
					cmd.FinishWriting();
					if(rCli.ExecSrvCmd(cmd, PPConst::DefSrvCmdTerm, reply)) {
						SString reply_buf;
						reply.StartReading(&reply_buf);
						if(reply.CheckRepError()) {
							SJson * p_js = SJson::Parse(reply_buf);
							if(st_data.FromJsonObject(p_js)) {
								;
							}
							else
								st_data.SetupByLastError();
							ZDELETE(p_js);
						}
						else
							st_data.SetupByLastError();
					}
					st_data.DtmActual = getcurdatetime_();
					P_St->D_TSess.SetData(st_data);
				}
			}
			break;
		case PPSCMD_WSCTL_BEGIN_SESS:
			if(P_St) {
				DTSess st_data;
				SJson js_param(SJson::tOBJECT);
				js_param.InsertInt("scardid", rReq.P.SCardID);
				temp_buf.Z().Cat(rReq.P.Uuid, S_GUID::fmtIDL);
				js_param.InsertString("wsctluuid", temp_buf.Escape());
				js_param.InsertInt("goodsid", rReq.P.GoodsID);
				if(rReq.P.TechID) {
					js_param.InsertInt("techid", rReq.P.TechID);
				}
				if(rReq.P.Amount > 0.0) {
					js_param.InsertDouble("amt", rReq.P.Amount, MKSFMTD(0, 2, 0));
				}
				PPJobSrvCmd cmd;
				cmd.StartWriting(PPSCMD_WSCTL_BEGIN_SESS);
				{
					js_param.ToStr(temp_buf);
					SString mime_buf;
					mime_buf.EncodeMime64(temp_buf.ucptr(), temp_buf.Len());
					cmd.Write(mime_buf.ucptr(), mime_buf.Len()+1);
				}
				cmd.FinishWriting();
				if(rCli.ExecSrvCmd(cmd, PPConst::DefSrvCmdTerm, reply)) {
					SString reply_buf;
					reply.StartReading(&reply_buf);
					if(reply.CheckRepError()) {
						SJson * p_js = SJson::Parse(reply_buf);
						if(st_data.FromJsonObject(p_js)) {
							;
						}
						else
							st_data.SetupByLastError();
						P_St->D_TSess.SetData(st_data);
						ZDELETE(p_js);
					}
					else
						st_data.SetupByLastError();
				}
				st_data.DtmActual = getcurdatetime_();
				P_St->D_TSess.SetData(st_data);
			}
			break;
	}
}

WsCtl_ImGuiSceneBlock::WsCtl_ImGuiSceneBlock() : ShowDemoWindow(false), ShowAnotherWindow(false), Screen(screenUndef),
	//ClearColor(0.45f, 0.55f, 0.60f, 1.00f), 
	ClearColor(SColor(0x1E, 0x22, 0x28)),
	P_CmdQ(new WsCtlReqQueue),
	Cache_Layout(512, 0), Cache_Texture(1024, 0), Cache_Font(101, 0),
	P_Dlg_Cfg(0)
{
	TestInput[0] = 0;
	LoginText[0] = 0;
	PwText[0] = 0;
}

WsCtl_ImGuiSceneBlock::~WsCtl_ImGuiSceneBlock()
{
	// P_CmdQ не разрушаем поскольку на него ссылается отдельный поток.
	// Все равно этот объект живет в течении всего жизненного цикла процесса.
}

int WsCtl_ImGuiSceneBlock::SetScreen(int scr)
{
	int    ok = 0;
	if(oneof6(scr, screenConstruction, screenHybernat, screenRegister, screenLogin, screenAuthSelectSess, 
		screenSession)) {
		if(Screen != scr) {
			Screen = scr;
			ok = 1;
		}
		else
			ok = -1;
	}
	return ok;
}

int WsCtl_ImGuiSceneBlock::CreateFontEntry(ImGuiIO & rIo, const char * pSymb, const char * pPath, float sizePx, const ImFontConfig * pFontCfg, const SColor * pClr)
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
	
int WsCtl_ImGuiSceneBlock::PushFontEntry(ImGuiObjStack & rStk, const char * pSymb)
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

int WsCtl_ImGuiSceneBlock::LoadUiDescription()
{
	int    ok = 0;
	SJson * p_js = 0;
	SString temp_buf;
	PPGetPath(PPPATH_WORKSPACE, temp_buf);
	temp_buf.SetLastSlash().Cat("wsctl").SetLastSlash().Cat("wsctl-ui.json");
	SFile f_in(temp_buf, SFile::mRead);
	{
		STempBuffer in_buf(8192);
		size_t actual_size = 0;
		THROW(in_buf.IsValid());
		THROW(f_in.ReadAll(in_buf, 0, &actual_size));
		temp_buf.Z().CatN(in_buf, actual_size);
	}
	{
		p_js = SJson::Parse(temp_buf);
		THROW(p_js);
		THROW(Uid.FromJsonObj(p_js));
	}
	{
		SColorSet * p_cs = Uid.GetColorSet("imgui-style");
		if(p_cs) {
			p_cs->Resolve();
		}
	}
	{
		const bool use_outer_layout_description = true;
		if(use_outer_layout_description) {
			for(uint i = 0; i < Uid.LoList.getCount(); i++) {
				const SUiLayout * p_lo = Uid.LoList.at(i);
				if(p_lo) {
					SUiLayout * p_new_lo = new SUiLayout(*p_lo);
					Cache_Layout.Put(p_new_lo, true);
				}
			}
		}
		else {
			SJson * p_js_lo_list = 0;
			MakeLayout(&p_js_lo_list);
			if(p_js_lo_list) {
				SString temp_buf;
				PPGetFilePath(PPPATH_OUT, "wsctl_ui.json", temp_buf);
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
	}
	CATCHZOK
	delete p_js;
	return ok;
}

SUiLayout * WsCtl_ImGuiSceneBlock::MakePgmListLayout(const WsCtl_ProgramCollection & rPgmL)
{
	const int pgm_entry_template_id = loidProgramEntryTemplate;
	const SUiLayout * p_pe_template = Cache_Layout.Get(&pgm_entry_template_id, sizeof(pgm_entry_template_id));
	SString filt_cat_text;		
	SUiLayoutParam lop_head;
	lop_head.SetContainerDirection(DIREC_VERT);
	lop_head.SetVariableSizeX(SUiLayoutParam::szByContainer, 1.0f);
	lop_head.SetVariableSizeY(SUiLayoutParam::szByContainer, 1.0f);
	lop_head.Flags |= (SUiLayoutParam::fEvaluateScroller|SUiLayoutParam::fContainerWrap);
	SUiLayout * p_lo_head = new SUiLayout(lop_head);
	SUiLayoutParam lop_entry;
	if(p_pe_template) {
		lop_entry = p_pe_template->GetLayoutBlockC();
	}
	else {
		lop_entry.SetFixedSizeX(128.0f);
		lop_entry.SetFixedSizeY(128.0f);
	}
	if(rPgmL.GetSelectedCatSurrogateId()) {
		uint sel_idx = 0;
		const StrAssocArray & r_cat_list = rPgmL.GetCatList();
		if(r_cat_list.Search(rPgmL.GetSelectedCatSurrogateId(), &sel_idx)) {
			filt_cat_text = r_cat_list.Get(sel_idx).Txt;
		}
	}
	for(uint i = 0; i < rPgmL.getCount(); i++) {
		const WsCtl_ProgramEntry * p_pe = rPgmL.at(i);
		if(p_pe) {
			if(filt_cat_text.IsEmpty() || filt_cat_text.IsEqiUtf8(p_pe->Category))
				p_lo_head->InsertItem(const_cast<WsCtl_ProgramEntry *>(p_pe), &lop_entry, loidStartProgramEntry+i+1);
		}
	}
	p_lo_head->SetID(loidInternalProgramGallery);
	//Cache_Layout.Put(p_lo_head, true);
	return p_lo_head;
}

int WsCtl_ImGuiSceneBlock::ExecuteProgram(const WsCtl_ProgramEntry * pPe)
{
	int    ok = 0;
	if(pPe && pPe->FullResolvedPath.NotEmpty()) {
		if(SessF.LaunchProcess(pPe)) {
			ok = 1;
		}
	}
	return ok;
}

int WsCtl_ImGuiSceneBlock::Init(ImGuiIO & rIo)
{
	int    ok = 0;
	PPIniFile ini_file;
	LoadUiDescription();
	if((ini_file.GetInt(PPINISECT_SERVER, PPINIPARAM_SERVER_PORT, &JsP.Port) <= 0 || JsP.Port <= 0))
		JsP.Port = InetUrl::GetDefProtocolPort(InetUrl::prot_p_PapyrusServer);//DEFAULT_SERVER_PORT;
	if(ini_file.GetInt(PPINISECT_SERVER, PPINIPARAM_CLIENTSOCKETTIMEOUT, &JsP.Timeout) <= 0 || JsP.Timeout <= 0)
		JsP.Timeout = -1;
	ini_file.Get(PPINISECT_SERVER, PPINIPARAM_SERVER_NAME, JsP.Server);
	St.SidBlk.GetOwnUuid();
	JsP.Read();
	if(JsP.Server.NotEmpty()) {
		//
		JsP.DbSymb.SetIfEmpty("wsctl");
		JsP.User.SetIfEmpty("master");
		JsP.Password.SetIfEmpty("");
		//
		WsCtl_CliSession * p_sess = new WsCtl_CliSession(JsP, &St, P_CmdQ);
		p_sess->Start(1);
		if(!!St.SidBlk.Uuid) {
			WsCtlReqQueue::Req req(PPSCMD_WSCTL_INIT);
			req.P.Uuid = St.SidBlk.Uuid;
			P_CmdQ->Push(req);
		}
		ok = 1;
	}
	{
		St.SetupSyncUpdateTime(State::syncdataPrices, 30000);
		St.SetupSyncUpdateTime(State::syncdataAccount, 29000);
		St.SetupSyncUpdateTime(State::syncdataTSess, 15000);
		St.SetupSyncUpdateTime(State::syncdataAutonomousTSess, 2000);
		St.SetupSyncUpdateTime(State::syncdataClientPolicy, 600000); // @v11.7.12
	}
	//WsCtlStyleColors(true, 0);
	//void WsCtlStyleColors(bool useUiDescription, ImGuiStyle * pDest)
	{
		const bool use_ui_descripton = true;
		ImGuiStyle * p_dest_style = 0;
		ImGuiStyle * style = p_dest_style ? p_dest_style : &ImGui::GetStyle();
		ImVec4 * colors = style->Colors;
		colors[ImGuiCol_Text]                   = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
		colors[ImGuiCol_TextDisabled]           = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
		colors[ImGuiCol_WindowBg]               = SColor(0x2B, 0x30, 0x38);//ImVec4(0.06f, 0.06f, 0.06f, 0.94f);
		colors[ImGuiCol_ChildBg]                = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_PopupBg]                = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
		colors[ImGuiCol_Border]                 = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
		colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_FrameBg]                = SColor(SClrBlack, 0.95f); //ImVec4(0.16f, 0.29f, 0.48f, 0.54f);
		colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
		colors[ImGuiCol_FrameBgActive]          = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
		colors[ImGuiCol_TitleBg]                = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
		colors[ImGuiCol_TitleBgActive]          = ImVec4(0.16f, 0.29f, 0.48f, 1.00f);
		colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
		colors[ImGuiCol_MenuBarBg]              = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
		colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
		colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
		colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
		colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
		colors[ImGuiCol_CheckMark]              = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
		colors[ImGuiCol_SliderGrab]             = ImVec4(0.24f, 0.52f, 0.88f, 1.00f);
		colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
		colors[ImGuiCol_Button]                 = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
		colors[ImGuiCol_ButtonHovered]          = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
		colors[ImGuiCol_ButtonActive]           = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
		colors[ImGuiCol_Header]                 = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
		colors[ImGuiCol_HeaderHovered]          = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
		colors[ImGuiCol_HeaderActive]           = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
		colors[ImGuiCol_Separator]              = colors[ImGuiCol_Border];
		colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
		colors[ImGuiCol_SeparatorActive]        = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
		colors[ImGuiCol_ResizeGrip]             = ImVec4(0.26f, 0.59f, 0.98f, 0.20f);
		colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
		colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
		colors[ImGuiCol_Tab]                    = ImLerp(colors[ImGuiCol_Header], colors[ImGuiCol_TitleBgActive], 0.80f);
		colors[ImGuiCol_TabHovered]             = colors[ImGuiCol_HeaderHovered];
		colors[ImGuiCol_TabActive]              = ImLerp(colors[ImGuiCol_HeaderActive], colors[ImGuiCol_TitleBgActive], 0.60f);
		colors[ImGuiCol_TabUnfocused]           = ImLerp(colors[ImGuiCol_Tab], colors[ImGuiCol_TitleBg], 0.80f);
		colors[ImGuiCol_TabUnfocusedActive]     = ImLerp(colors[ImGuiCol_TabActive], colors[ImGuiCol_TitleBg], 0.40f);
		colors[ImGuiCol_PlotLines]              = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
		colors[ImGuiCol_PlotLinesHovered]       = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
		colors[ImGuiCol_PlotHistogram]          = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
		colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
		colors[ImGuiCol_TableHeaderBg]          = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
		colors[ImGuiCol_TableBorderStrong]      = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);// Prefer using Alpha=1.0 here
		colors[ImGuiCol_TableBorderLight]       = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);// Prefer using Alpha=1.0 here
		colors[ImGuiCol_TableRowBg]             = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_TableRowBgAlt]          = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
		colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
		colors[ImGuiCol_DragDropTarget]         = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
		colors[ImGuiCol_NavHighlight]           = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
		colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
		colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
		colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
		if(use_ui_descripton) {
			SColorSet * p_cs = Uid.GetColorSet("imgui-style");	
			if(p_cs) {
				for(uint i = 0; i < SIZEOFARRAY(style->Colors); i++) {
					const char * p_color_name = ImGui::GetStyleColorName(i);
					if(!isempty(p_color_name)) {
						SColor c;
						if(p_cs->Get(p_color_name, c)) {
							style->Colors[i] = c;
						}
					}
				}
			}
		}
	}
	{
		SColorSet * p_cs = Uid.GetColorSet("imgui-style");	
		{
			///Papyrus/Src/Rsrc/Font/imgui/Roboto-Medium.ttf
			//C:/Windows/Fonts/Tahoma.ttf
			ImFontConfig f;
			SColor primary_font_color;
			SColor secondary_font_color;
			SColor substrat_color;
			if(p_cs) {
				p_cs->Get("TextPrimary", primary_font_color);
				p_cs->Get("TextSecondary", secondary_font_color);
				p_cs->Get("Substrat", substrat_color);
			}
			else {
				substrat_color.Set(0x1E, 0x22, 0x28);
				primary_font_color = SColor(SClrWhite);
				secondary_font_color = SColor(SClrSilver);
			}
			ClearColor = substrat_color;
			CreateFontEntry(rIo, "FontSecondary", "/Papyrus/Src/Rsrc/Font/imgui/Roboto-Medium.ttf", 14.0f, 0, &secondary_font_color);
			CreateFontEntry(rIo, "FontPrimary", "/Papyrus/Src/Rsrc/Font/imgui/Roboto-Medium.ttf", 16.0f, 0, &primary_font_color);
		}
	}
	LoadProgramList2(); // 
	return ok;
}

void WsCtl_ImGuiSceneBlock::EmitEvents()
{
	if(St.CheckSyncUpdateTimers(SyncReqList) > 0) {
		assert(SyncReqList.getCount() > 0);
		for(uint i = 0; i < SyncReqList.getCount(); i++) {
			const int sync_data_id = SyncReqList.get(i);
			switch(sync_data_id) {
				case State::syncdataUndef:
					break;
				case State::syncdataTest:
					break;
				case State::syncdataPrc:
					break;
				case State::syncdataAccount:
					{
						DAuth auth_data;
						St.D_Auth.GetData(auth_data);
						if(auth_data.SCardID && !(auth_data.State & auth_data.stWaitOn)) {
							WsCtlReqQueue::Req req(PPSCMD_WSCTL_GETACCOUNTSTATE);
							req.P.SCardID = auth_data.SCardID;
							P_CmdQ->Push(req);
						}
					}
					break;
				case State::syncdataPrices:
					P_CmdQ->Push(WsCtlReqQueue::Req(PPSCMD_WSCTL_GETQUOTLIST));
					break;
				case State::syncdataAutonomousTSess:
					{
							
					}
					break;
				case State::syncdataTSess:
					P_CmdQ->Push(WsCtlReqQueue::Req(PPSCMD_WSCTL_TSESS));
					break;
				case State::syncdataJobSrvConnStatus:
					break;
				case State::syncdataClientPolicy: // @v11.7.12
					//P_CmdQ->Push(WsCtlReqQueue::Req(PPSCMD_WSCTL_QUERYPOLICY));
					break;
			}
			St.SetSyncUpdateTimerLastReqTime(sync_data_id); // Отмечаем время отправки запроса
		}
	}
	else {
		assert(SyncReqList.getCount() == 0);
	}
}
//
//
//
void WsCtl_ImGuiSceneBlock::Render()
{
	ImGui::Render();
	const float clear_color_with_alpha[4] = { 
		ClearColor.x * ClearColor.w, 
		ClearColor.y * ClearColor.w, 
		ClearColor.z * ClearColor.w, 
		ClearColor.w 
	};
	ImgRtb.g_pd3dDeviceContext->OMSetRenderTargets(1, &ImgRtb.g_mainRenderTargetView, nullptr);
	ImgRtb.g_pd3dDeviceContext->ClearRenderTargetView(ImgRtb.g_mainRenderTargetView, clear_color_with_alpha);
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
	ImgRtb.g_pSwapChain->Present(1, 0); // Present with vsync
	//g_pSwapChain->Present(0, 0); // Present without vsync
}

/*static*/int WsCtl_ImGuiSceneBlock::CbInput(ImGuiInputTextCallbackData * pInputData)
{
	bool debug_mark = false;
	if(pInputData) {
		WsCtl_ImGuiSceneBlock * p_this = static_cast<WsCtl_ImGuiSceneBlock *>(pInputData->UserData);
		if(p_this) {
			if(pInputData->EventKey != 0) {
				debug_mark = true;
			}
			//if(pInputData->Buf == p_this->LoginText) {
				//debug_mark = true;
			//}
		}
	}
	return 0;
}

void WsCtl_ImGuiSceneBlock::ErrorPopup(bool isErr)
{
	if(isErr) {
		const char * p_popup_title = "Error message";
		ImGui::OpenPopup(p_popup_title);
		if(ImGui::BeginPopup(p_popup_title)) {
			ImGui::Text(LastSvrErr._Message);
			if(ImGui::Button("Close", ButtonSize_Std)) {
				LastSvrErr.Z(); // Сбрасываем информацию об ошибке
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}
	}
}

void WsCtl_ImGuiSceneBlock::MakeLayout(SJson ** ppJsList)
{
	const FRect margin_(2.0f, 1.0f, 2.0f, 1.0f);
	LongArray lo_id_list;
	SString temp_buf;
	{
		//
		// screenLogin
		//
		const int lo_id = screenLogin;
		const char * p_lo_symb = "screenLogin";
		SUiLayout * p_tl = new SUiLayout();
		SUiLayoutParam alb(DIREC_VERT, SUiLayoutParam::alignCenter, SUiLayoutParam::alignCenter);
		alb.AlignItems = SUiLayoutParam::alignCenter;
		p_tl->SetLayoutBlock(alb);
		p_tl->SetID(lo_id);
		p_tl->SetSymb(p_lo_symb);
		{
			SUiLayoutParam alb(DIREC_HORZ, SUiLayoutParam::alignCenter, SUiLayoutParam::alignCenter);
			alb.SetFixedSizeX(480).SetFixedSizeY(360);
			p_tl->InsertItem(0, &alb, loidLoginBlock);
		}
		Cache_Layout.Put(p_tl, true);
		lo_id_list.add(lo_id);
	}
	{
		//
		// screenAuthSelectSess
		//
		const int lo_id = screenAuthSelectSess;
		const char * p_lo_symb = "screenAuthSelectSess";
		SUiLayout * p_tl = new SUiLayout();
		SUiLayoutParam alb(DIREC_HORZ, SUiLayoutParam::alignCenter, SUiLayoutParam::alignCenter);
		p_tl->SetLayoutBlock(alb);
		p_tl->SetID(lo_id);
		p_tl->SetSymb(p_lo_symb);
		{
			// Left menu
			SUiLayoutParam alb01(DIREC_VERT, 0, SUiLayoutParam::alignStretch);
			alb01.SetMargin(margin_).SetFixedSizeX(128.0f).SetVariableSizeY(SUiLayoutParam::szByContainer, 1.0f);
			p_tl->InsertItem(0, &alb01, loidMenuBlock);			
		}
		{
			SUiLayoutParam alb_mg(DIREC_VERT, 0, SUiLayoutParam::alignStretch);
			alb_mg.SetMargin(margin_).SetGrowFactor(1.0f).SetVariableSizeY(SUiLayoutParam::szByContainer, 1.0f);
			SUiLayout * p_lo_main_group = p_tl->InsertItem(0, &alb_mg, loidMainGroup);
			{
				SUiLayoutParam alb_ig(DIREC_HORZ, 0, SUiLayoutParam::alignStretch);
				alb_ig.SetMargin(margin_).SetGrowFactor(1.0f).SetVariableSizeX(SUiLayoutParam::szByContainer, 1.0f);
				SUiLayout * p_lo_info_group = p_lo_main_group->InsertItem(0, &alb_ig, loidMainGroup);
				{
					// Person info
					SUiLayoutParam alb01(DIREC_VERT, 0, SUiLayoutParam::alignStretch);
					alb01.SetMargin(margin_).SetGrowFactor(1.0f).SetVariableSizeY(SUiLayoutParam::szByContainer, 1.0f);
					p_lo_info_group->InsertItem(0, &alb01, loidPersonInfo);
				}
				{
					// Account info
					SUiLayoutParam alb01(DIREC_VERT, 0, SUiLayoutParam::alignStretch);
					alb01.SetMargin(margin_).SetGrowFactor(1.0f).SetVariableSizeY(SUiLayoutParam::szByContainer, 1.0f);
					p_lo_info_group->InsertItem(0, &alb01, loidAccountInfo);
				}
				{
					SUiLayoutParam albg(DIREC_VERT, 0, SUiLayoutParam::alignStretch);
					albg.SetMargin(margin_).SetGrowFactor(1.0f).SetVariableSizeY(SUiLayoutParam::szByContainer, 1.0f);
					SUiLayout * p_lo_grp = p_lo_info_group->InsertItem(0, &albg, 0);
					{
						// Session selection
						SUiLayoutParam alb01(DIREC_VERT, 0, SUiLayoutParam::alignStretch);
						alb01.SetMargin(margin_).SetGrowFactor(1.0f).SetVariableSizeX(SUiLayoutParam::szByContainer, 1.0f);
						p_lo_grp->InsertItem(0, &alb01, loidSessionSelection);
					}
					{
						SUiLayoutParam alb01(DIREC_HORZ, 0, SUiLayoutParam::alignStretch);
						alb01.SetMargin(margin_).SetFixedSizeY(128.0f).SetVariableSizeX(SUiLayoutParam::szByContainer, 1.0f);
						p_lo_grp->InsertItem(0, &alb01, loidSessionButtonGroup);
					}
				}
			}
			{
				// Buttons group
				SUiLayoutParam alb01(DIREC_HORZ, 0, SUiLayoutParam::alignStretch);
				alb01.SetMargin(margin_).SetFixedSizeY(128.0f).SetVariableSizeX(SUiLayoutParam::szByContainer, 1.0f);
				p_lo_main_group->InsertItem(0, &alb01, loidBottomCtrlGroup);
			}
		}
		Cache_Layout.Put(p_tl, true);
		lo_id_list.add(lo_id);
	}
	{
		//
		// screenSession
		//
		const int lo_id = screenSession;
		const char * p_lo_symb = "screenSession";
		SUiLayout * p_tl = new SUiLayout();
		p_tl->SetLayoutBlock(SUiLayoutParam(DIREC_HORZ, 0, SUiLayoutParam::alignStretch));
		p_tl->SetID(lo_id);
		p_tl->SetSymb(p_lo_symb);
		{
			{
				SUiLayoutParam alb(DIREC_VERT, 0, SUiLayoutParam::alignStretch);
				alb.SetVArea(SUiLayoutParam::areaSideL);
				alb.SetFixedSizeX(256).SetVariableSizeY(SUiLayoutParam::szByContainer, 1.0f).SetMargin(1.0f);
				p_tl->InsertItem(0, &alb, loidMenuBlock);
			}
			{
				SUiLayoutParam alb_mg(DIREC_VERT, 0, SUiLayoutParam::alignStretch);
				alb_mg.SetVArea(SUiLayoutParam::areaSideL);
				alb_mg.SetVariableSizeX(SUiLayoutParam::szByContainer, 1.0f).SetVariableSizeY(SUiLayoutParam::szByContainer, 1.0f).SetMargin(1.0f);
				SUiLayout * p_lo_main_group = p_tl->InsertItem(0, &alb_mg, loidMainGroup);
				{
					SUiLayoutParam alb01(DIREC_HORZ, 0, SUiLayoutParam::alignStretch);
					alb01.SetMargin(margin_).SetGrowFactor(1.0f).SetFixedSizeY(128.0f);
					p_lo_main_group->InsertItem(0, &alb01, loidSessionInfo);
				}
				{
					SUiLayoutParam alb01(DIREC_HORZ, 0, SUiLayoutParam::alignStretch);
					alb01.SetMargin(margin_).SetGrowFactor(1.0f).SetVariableSizeX(SUiLayoutParam::szByContainer, 1.0f);
					p_lo_main_group->InsertItem(0, &alb01, loidSessionProgramGallery);
				}
			}
		}
		Cache_Layout.Put(p_tl, true);
		lo_id_list.add(lo_id);
	}
	{
		//
		// screenConstruction
		//
		const int lo_id = screenConstruction;
		const char * p_lo_symb = "screenConstruction";
		SUiLayout * p_tl = new SUiLayout();
		p_tl->SetLayoutBlock(SUiLayoutParam(DIREC_HORZ, 0, SUiLayoutParam::alignStretch));
		p_tl->SetID(lo_id);
		p_tl->SetSymb(p_lo_symb);
		{
			{
				SUiLayoutParam alb(DIREC_VERT, 0, SUiLayoutParam::alignStretch);
				alb.SetVArea(SUiLayoutParam::areaSideL);
				alb.SetFixedSizeX(256).SetVariableSizeY(SUiLayoutParam::szByContainer, 1.0f).SetMargin(1.0f);
				p_tl->InsertItem(0, &alb, loidMenuBlock);
			}
			{
				SUiLayoutParam alb_mg(DIREC_VERT, 0, SUiLayoutParam::alignStretch);
				alb_mg.SetVArea(SUiLayoutParam::areaSideL);
				alb_mg.SetVariableSizeX(SUiLayoutParam::szByContainer, 1.0f).SetVariableSizeY(SUiLayoutParam::szByContainer, 1.0f).SetMargin(1.0f);
				SUiLayout * p_lo_main_group = p_tl->InsertItem(0, &alb_mg, loidMainGroup);
				{
					SUiLayoutParam alb(DIREC_HORZ, 0, SUiLayoutParam::alignStretch);
					alb.SetMargin(margin_).SetGrowFactor(1.2f).SetVariableSizeX(SUiLayoutParam::szByContainer, 1.0f);
					SUiLayout * p_lo_up_group = p_lo_main_group->InsertItem(0, &alb, loidUpperGroup);
					{
						SUiLayoutParam alb01(DIREC_VERT, 0, SUiLayoutParam::alignStretch);
						alb01.SetMargin(margin_).SetGrowFactor(1.0f).SetVariableSizeY(SUiLayoutParam::szByContainer, 1.0f);
						p_lo_up_group->InsertItem(0, &alb01, loidCtl01);
					}
					{
						SUiLayoutParam alb01(DIREC_VERT, 0, SUiLayoutParam::alignStretch);
						alb01.SetMargin(margin_).SetGrowFactor(1.0f).SetVariableSizeY(SUiLayoutParam::szByContainer, 1.0f);
						p_lo_up_group->InsertItem(0, &alb01, loidCtl02);
					}
				}
				{
					SUiLayoutParam alb(DIREC_HORZ, 0, SUiLayoutParam::alignStretch);
					alb.SetMargin(margin_).SetGrowFactor(0.8f).SetVariableSizeX(SUiLayoutParam::szByContainer, 1.0f);
					SUiLayout * p_lo_dn_group = p_lo_main_group->InsertItem(0, &alb, loidBottomGroup);
					/*{
						SUiLayoutParam alb01(DIREC_VERT, 0, SUiLayoutParam::alignStretch);
						alb01.SetMargin(margin_).SetGrowFactor(1.0f).SetVariableSizeY(SUiLayoutParam::szByContainer, 1.0f);
						p_lo_dn_group->InsertItem(0, &alb01, loidTestProgramGallery);
					}
					{
						SUiLayoutParam alb01(DIREC_VERT, 0, SUiLayoutParam::alignStretch);
						alb01.SetMargin(margin_).SetGrowFactor(1.0f).SetVariableSizeY(SUiLayoutParam::szByContainer, 1.0f);
						p_lo_dn_group->InsertItem(0, &alb01, loidAdv02);
					}
					{
						SUiLayoutParam alb01(DIREC_VERT, 0, SUiLayoutParam::alignStretch);
						alb01.SetMargin(margin_).SetGrowFactor(1.0f).SetVariableSizeY(SUiLayoutParam::szByContainer, 1.0f);
						p_lo_dn_group->InsertItem(0, &alb01, loidAdv03);
					}*/
				}
			}
		}
		Cache_Layout.Put(p_tl, true);
		lo_id_list.add(lo_id);
	}
	if(ppJsList) {
		SJson * p_js_lo_list = 0;
		if(lo_id_list.getCount()) {
			p_js_lo_list = SJson::CreateArr();
			for(uint i = 0; i < lo_id_list.getCount(); i++) {
				int lo_id = lo_id_list.get(i);
				const SUiLayout * p_lo = Cache_Layout.Get(&lo_id, sizeof(lo_id));
				if(p_lo) {
					SJson * p_js_item = p_lo->ToJsonObj();
					if(p_js_item) {
						p_js_lo_list->InsertChild(p_js_item);
					}
				}
			}
		}
		*ppJsList = p_js_lo_list;
	}
}

void WsCtl_ImGuiSceneBlock::EmitProgramGallery(ImGuiWindowByLayout & rW, SUiLayout & rTl)
{
	const int view_flags = ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoDecoration;
	if(rW.IsValid()) {
		float total_size = 0.0f;
		SUiLayout * p_lo_ipg = rTl.FindById(loidInternalProgramGallery); // non-const!
		SUiLayout::Result * p_lor = 0;
		SScroller * p_scr = 0;
		if(p_lo_ipg) {
			p_lor = &p_lo_ipg->GetResult_(); // non-const!
			p_scr = p_lor->P_Scrlr;
			if(p_scr) {
				p_scr->SetPosition(PgmGalleryScrollerPosition_Develop);
				total_size = p_scr->GetSize();
				//ImGui::SetNextWindowContentSize(ImVec2(total_size, 0.0f));
			}
			{
				SString temp_buf;
				const bool is_line_content_valid = p_scr ? (p_scr->CheckLineContentIndex(-1) >= 0) : false;
				SPoint2F offset;
				if(p_scr) {
					offset.x = -p_scr->GetCurrentPageTopPoint();
				}
				const WsCtl_ProgramEntry * p_clicked_entry = 0;
				for(uint loidx = 0; loidx < p_lo_ipg->GetChildrenCount(); loidx++) {
					SUiLayout * p_lo_entry = p_lo_ipg->GetChild(loidx);
					if(p_lo_entry) {
						if(!is_line_content_valid || p_scr->CheckLineContentIndex(loidx) > 0) {
							ImGuiWindowByLayout wbl_entry(p_lo_entry, offset, temp_buf.Z().Cat("##GALLERYENTRY").Cat(p_lo_entry->GetID()), view_flags);
							if(wbl_entry.IsValid()) {
								//const uint pe_idx = i-(loidStartProgramEntry+1);
								const WsCtl_ProgramEntry * p_pe_ = static_cast<const WsCtl_ProgramEntry *>(SUiLayout::GetManagedPtr(p_lo_entry));
								if(p_pe_) {
									Texture_CachedFileEntity * p_te = Cache_Texture.Get(p_pe_->PicSymb);
									if(p_te && p_te->P_Texture) {
										if(p_pe_->Title.NotEmpty())
											ImGui::Text(p_pe_->Title);
										const SPoint2F __s = p_lo_entry->GetFrame().GetSize();
										ImVec2 sz(__s.x-4.0f, __s.y-16.0f);
										ImGui::Image(p_te->P_Texture, sz);
									}
									else {
										if(p_pe_->Category.NotEmpty())
											ImGui::Text(p_pe_->Category);
										if(p_pe_->Title.NotEmpty())
											ImGui::Text(p_pe_->Title);
									}
									if(ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
										p_clicked_entry = p_pe_;
									}
								}
								/*
								assert(loidx < PgmL.getCount());
								if(loidx < PgmL.getCount()) {
									const WsCtl_ProgramEntry * p_pe = PgmL.at(loidx);
									if(p_pe->Category.NotEmpty())
										ImGui::Text(p_pe->Category);
									if(p_pe->Title.NotEmpty())
										ImGui::Text(p_pe->Title);
								}*/
							}
							else
								break;
						}
					}
					else
						break;
				}
				if(p_clicked_entry) {
					ExecuteProgram(p_clicked_entry);
				}
			}
			if(p_lor) {
				{
					ImGuiWindowByLayout wsb(&rTl, loidProgramGalleryCatSelector, "##ProgramCatSelector",
						view_flags/*|ImGuiWindowFlags_NoMouseInputs|ImGuiWindowFlags_HorizontalScrollbar|ImGuiWindowFlags_AlwaysHorizontalScrollbar*/);
					const StrAssocArray & r_cat_list = PgmL.GetCatList();
					if(r_cat_list.getCount()) {
						const char * p_selected_text = 0;
						uint    selected_idx = 0;
						if(r_cat_list.Search(PgmL.GetSelectedCatSurrogateId(), &selected_idx) > 0) {
							StrAssocArray::Item item = r_cat_list.Get(selected_idx);
							p_selected_text = item.Txt;
						}
						if(ImGui::BeginCombo("##pgmcatlist", p_selected_text)) {
							for(uint catidx = 0; catidx < r_cat_list.getCount(); catidx++) {
								StrAssocArray::Item item = r_cat_list.Get(catidx);
								if(ImGui::Selectable(item.Txt, item.Id == PgmL.GetSelectedCatSurrogateId()))
									PgmL.SetSelectedCatSurrogateId(item.Id);
							}
							ImGui::EndCombo();
						}
					}
				}
				{
					ImGuiWindowByLayout wsb(&rTl, loidProgramGalleryScrollbar, "##ProgramScrollbar",
						view_flags/*|ImGuiWindowFlags_NoMouseInputs|ImGuiWindowFlags_HorizontalScrollbar|ImGuiWindowFlags_AlwaysHorizontalScrollbar*/);
					int64 scroll_value = 0;
					int64 scroll_size = p_lor->P_Scrlr ? p_lor->P_Scrlr->GetCount() : 0;
					int64 scroll_frame = 1; // @?
					bool debug_mark = false; // @debug
					if(scroll_size > 0) {
						const SUiLayout * p_lo_sb = rTl.FindByIdC(loidProgramGalleryScrollbar);
						if(p_lo_sb) {
							//SPoint2F s = r.GetSize();
							//if(p_ipg->P_)
							SScroller::Position scrp;
							if(p_scr)
								p_scr->GetPosition(scrp);
							ImDrawFlags sb_flags = 0;
							ImRect imr = FRectToImRect(p_lo_sb->GetFrameAdjustedToParent());
							scroll_value = scrp.ItemIdxCurrent;
							bool sbr = ImGui::ScrollbarEx(imr, loidProgramGalleryScrollbar, ImGuiAxis_X, &scroll_value, scroll_frame, scroll_size, sb_flags);
							// @v11.8.6 {
							const ImVec2 mouse_delta = ImGui::GetIO().MouseDelta;
							if(mouse_delta.x != 0.0f) {
								//ScrollWhenDraggingOnVoid(ImVec2(-mouse_delta.x, /*-mouse_delta.y*/0.0f));
								//static void ScrollWhenDraggingOnVoid(const ImVec2 & rDelta) // @v11.8.6
								{
									ImGuiContext & g = *ImGui::GetCurrentContext();
									ImGuiWindow * p_window = g.CurrentWindow;
									bool hovered = false;
									bool held = false;
									if(g.HoveredId == 0) // If nothing hovered so far in the frame (not same as IsAnyItemHovered()!)
										ImGui::ButtonBehavior(p_window->Rect(), p_window->GetID("##scrolldraggingoverlay"), &hovered, &held, ImGuiButtonFlags_MouseButtonLeft);
									if(held) {
										p_window->Scroll.x += mouse_delta.x;
										p_window->Scroll.y += mouse_delta.y;
									}
								}
							}
							// } @v11.8.6
							if(sbr) {
								if(scroll_value >= 0) {
									PgmGalleryScrollerPosition_Develop.ItemIdxCurrent = static_cast<uint>(scroll_value);
								}
								debug_mark = true; // @debug
							}
						}
					}
					//ImGui::Scrollbar(ImGuiAxis_X);
				}
			}
		}
	}
}

#if 0 // {
int WsCtl_ImGuiSceneBlock::LoadProgramList()
{
	int    ok = 1;
	//PgmL
	//D:\Papyrus\ppy\workspace\wsctl\wsctl-program.json 
	SString temp_buf;
	PPGetPath(PPPATH_WORKSPACE, temp_buf);
	temp_buf.SetLastSlash().Cat("wsctl").SetLastSlash().Cat("wsctl-program.json"); // По этому пути находится тестовый фейковый список программ
	SJson * p_js = SJson::ParseFile(temp_buf);
	THROW_SL(p_js);
	THROW(PgmL.FromJsonObj(p_js));
	PgmL.MakeCatList();
	{
		SString pic_base_path;
		PPGetPath(PPPATH_WORKSPACE, temp_buf);
		(pic_base_path = temp_buf).SetLastSlash().Cat("cache").SetLastSlash().Cat("img").SetLastSlash();
		if(pathValid(pic_base_path, 1)) {
			Cache_Texture.SetBasePath(pic_base_path);
			for(uint i = 0; i < PgmL.getCount(); i++) {
				WsCtl_ProgramEntry * p_pe = PgmL.at(i);
				if(p_pe) {
					if(p_pe->PicSymb.NotEmpty()) {
						(temp_buf = pic_base_path).Cat(p_pe->PicSymb);
						if(fileExists(temp_buf)) {
							Texture_CachedFileEntity * p_cfe = new Texture_CachedFileEntity();
							if(p_cfe && p_cfe->Init(temp_buf)) {
								if(p_cfe->Reload(true, &ImgRtb)) {
									Cache_Texture.Put(p_cfe);
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
#endif // } 0

int WsCtl_ImGuiSceneBlock::GetProgramListFromCache(WsCtl_ProgramCollection & rPgmL, WsCtl_ClientPolicy & rPolicyL)
{
	int    ok = 0;
	SString temp_buf;
	SString cache_path;
	WsCtl_ImGuiSceneBlock::GetLocalCachePath(cache_path);
	cache_path.SetLastSlash().Cat("data");
	if(SFile::CreateDir(cache_path)) {
		(temp_buf = cache_path).SetLastSlash().Cat("wsctl-program.json");
		if(fileExists(temp_buf)) {
			SJson * p_js_obj = SJson::ParseFile(temp_buf);
			if(p_js_obj) {
				WsCtl_ProgramCollection pgml_local_instance;
				if(pgml_local_instance.FromJsonObj(p_js_obj)) {
					rPgmL = pgml_local_instance;
					ok |= 0x01;
				}
			}
			delete p_js_obj;
		}
		(temp_buf = cache_path).SetLastSlash().Cat("wsctl-policy.json");
		if(fileExists(temp_buf)) {
			SJson * p_js_obj = SJson::ParseFile(temp_buf);
			if(p_js_obj) {
				WsCtl_ClientPolicy policyl_local_instance;
				if(policyl_local_instance.FromJsonObj(p_js_obj)) {
					rPolicyL = policyl_local_instance;
					ok |= 0x02;
				}
			}
			delete p_js_obj;			
		}
	}
	return ok;
}

int WsCtl_ImGuiSceneBlock::QueryProgramList2(WsCtl_ProgramCollection & rPgmL, WsCtl_ClientPolicy & rPolicyL)
{
	int    ok = 0;
	SString temp_buf;
	THROW(St.SidBlk.Uuid);
	THROW(JsP.Server.NotEmpty());
	{
		const S_GUID ws_uuid(St.SidBlk.Uuid);
		//SString cache_path;
		PPJobSrvClient cli;
		PPJobSrvReply reply;
		//THROW(WsCtl_ImGuiSceneBlock::GetLocalCachePath(cache_path));
		{
			DConnectionStatus srv_conn_status;
			{
				THROW(cli.Connect(JsP.Server, JsP.Port));
				if(JsP.DbSymb.NotEmpty() && JsP.User.NotEmpty()) {
					THROW(cli.Login(JsP.DbSymb, JsP.User, JsP.Password));
					srv_conn_status.S = 1;
				}
			}
			if(!srv_conn_status.S)
				srv_conn_status.SetupByLastError();
			St.D_ConnStatus.SetData(srv_conn_status);
		}
		{
			PPJobSrvCmd cmd;
			cmd.StartWriting(PPSCMD_WSCTL_QUERYPGMLIST);
			cmd.Write(&ws_uuid, sizeof(ws_uuid));
			cmd.FinishWriting();
			if(cli.ExecSrvCmd(cmd, PPConst::DefSrvCmdTerm, reply)) {
				SString reply_buf;
				reply.StartReading(&reply_buf);
				if(reply.CheckRepError()) {
					SJson * p_js_obj = SJson::Parse(reply_buf);
					WsCtl_ProgramCollection pgml_local_instance;
					if(pgml_local_instance.FromJsonObj(p_js_obj)) {
						rPgmL = pgml_local_instance;
						ok |= 0x01;
						//(temp_buf = cache_path).SetLastSlash().Cat("data").SetLastSlash().Cat("wsctl-program.json");
					}
					ZDELETE(p_js_obj);						
				}
			}
		}
		{
			PPJobSrvCmd cmd;
			cmd.StartWriting(PPSCMD_WSCTL_QUERYPOLICY);
			cmd.Write(&ws_uuid, sizeof(ws_uuid));
			cmd.FinishWriting();
			if(cli.ExecSrvCmd(cmd, PPConst::DefSrvCmdTerm, reply)) {
				SString reply_buf;
				reply.StartReading(&reply_buf);
				if(reply.CheckRepError()) {
					SJson * p_js_obj = SJson::Parse(reply_buf);
					WsCtl_ClientPolicy policyl_local_instance;
					if(policyl_local_instance.FromJsonObj(p_js_obj)) {
						rPolicyL = policyl_local_instance;
						ok |= 0x02;
					}
					else {
						PPSetErrorSLib();
					}
					ZDELETE(p_js_obj);						
				}
			}
		}
	}
	CATCH
		ok = 0;
	ENDCATCH
	return ok;
}

/*static*/bool WsCtl_ImGuiSceneBlock::GetLocalCachePath(SString & rPath)
{
	rPath.Z();
	bool    ok = true;
	SString temp_buf;
	SString cache_path;
	PPGetPath(PPPATH_WORKSPACE, temp_buf);	
	(cache_path = temp_buf).SetLastSlash().Cat("cache");
	if(SFile::CreateDir(cache_path)) {
		rPath = cache_path;
	}
	else
		ok = false;
	return ok;
}

void WsCtl_ImGuiSceneBlock::LoadProgramList2()
{
	SString temp_buf;
	{
		bool do_resolve = false;
		WsCtl_ProgramCollection  _pgm_l_from_server;
		WsCtl_ClientPolicy _policy_l_from_server;
		WsCtl_ProgramCollection  _pgm_l_from_cache;
		WsCtl_ClientPolicy _policy_l_from_cache;
		int r_cache = GetProgramListFromCache(_pgm_l_from_cache, _policy_l_from_cache);
		int r_srv = QueryProgramList2(_pgm_l_from_server, _policy_l_from_server);
		if(r_srv & 0x02) { // WsCtl_ClientPolicy is loaded successfully
			if(!(r_cache & 0x02) || _policy_l_from_server != _policy_l_from_cache) {
				PolicyL = _policy_l_from_server;
				do_resolve = true;
			}
			else {
				PolicyL = _policy_l_from_cache;
			}
		}
		else if(r_cache & 0x02) {
			PolicyL = _policy_l_from_cache;
		}
		if(r_srv & 0x01) { // WsCtl_ProgramCollection is loaded successfully
			if(!(r_cache & 0x01) || _pgm_l_from_server != _pgm_l_from_cache) {
				PgmL = _pgm_l_from_server;
				do_resolve = true;
			}
			else {
				PgmL = _pgm_l_from_cache;
			}
		}
		else if(r_cache & 0x01) {
			PgmL = _pgm_l_from_cache;
		}
		if(do_resolve) {
			PgmL.Resolve(PolicyL);
			{
				SString cache_path;
				SString _path;
				WsCtl_ImGuiSceneBlock::GetLocalCachePath(cache_path);
				cache_path.SetLastSlash().Cat("data");
				if(PgmL.getCount()) {
					SJson * p_js_obj = PgmL.ToJsonObj(true);
					if(p_js_obj) {
						p_js_obj->ToStr(temp_buf);
						(_path = cache_path).SetLastSlash().Cat("wsctl-program.json");
						SFile f_out(_path, SFile::mWrite);
						if(f_out.IsValid()) {
							f_out.Write(temp_buf.cptr(), temp_buf.Len());
						}
					}
					ZDELETE(p_js_obj);
				}
				{
					SJson * p_js_obj = PolicyL.ToJsonObj();
					if(p_js_obj) {
						p_js_obj->ToStr(temp_buf);
						(_path = cache_path).SetLastSlash().Cat("wsctl-policy.json");
						SFile f_out(_path, SFile::mWrite);
						if(f_out.IsValid()) {
							f_out.Write(temp_buf.cptr(), temp_buf.Len());
						}
					}
					ZDELETE(p_js_obj);
				}
			}
		}
		WsCtl_ImGuiSceneBlock::LoadProgramImages(PgmL, Cache_Texture);
		PgmL.MakeCatList();
	}
}

/*static*/void WsCtl_ImGuiSceneBlock::LoadProgramImages(WsCtl_ProgramCollection & rPgmL, TextureCache & rTextureCache)
{
	SString pic_base_path;
	SString temp_buf;
	WsCtl_ImGuiSceneBlock::GetLocalCachePath(pic_base_path);
	pic_base_path.SetLastSlash().Cat("img").SetLastSlash();
	if(pathValid(pic_base_path, 1)) {
		rTextureCache.SetBasePath(pic_base_path);
		for(uint i = 0; i < rPgmL.getCount(); i++) {
			WsCtl_ProgramEntry * p_pe = rPgmL.at(i);
			if(p_pe) {
				if(p_pe->PicSymb.NotEmpty()) {
					(temp_buf = pic_base_path).Cat(p_pe->PicSymb);
					if(fileExists(temp_buf)) {
						Texture_CachedFileEntity * p_cfe = new Texture_CachedFileEntity();
						if(p_cfe && p_cfe->Init(temp_buf)) {
							if(p_cfe->Reload(true, &ImgRtb)) {
								rTextureCache.Put(p_cfe);
								p_cfe = 0; // ! prevent deletion below
							}
						}
						delete p_cfe;
					}
				}
			}
		}
	}
}

void WsCtl_ImGuiSceneBlock::PreprocessProgramList()
{
	class Worker_PgmListResolver : public PPThread {
	public:
		Worker_PgmListResolver(WsCtl_ProgramCollection & rPgmL, const WsCtl_ClientPolicy & rPolicy, TextureCache & rTextureCache) : 
			PPThread(PPThread::kCasualJob, 0, 0), R_PgmL(rPgmL), Policy(rPolicy), R_TextureCache(rTextureCache)
		{
		}
		virtual void Run()
		{
			R_PgmL.Lck.Lock();
			R_PgmL.Resolve(Policy);
			R_PgmL.MakeCatList();
			WsCtl_ImGuiSceneBlock::LoadProgramImages(R_PgmL, R_TextureCache);
			R_PgmL.Lck.Unlock();
		}
	private:
		WsCtl_ProgramCollection & R_PgmL;
		WsCtl_ClientPolicy Policy; // copy of the original
		TextureCache & R_TextureCache;
	};	
	/*
	if(PgmL.Lck.TryLock()) {
		if(!PgmL.getCount()) {
			const DProgramList & dpl = St.D_PgmList.GetRef();
			if(dpl.L.getCount()) {
				PgmL = dpl.L;
			}
			St.D_PgmList.UnlockRef(); // !
		}
		if(PgmL.getCount() && !PgmL.IsResolved()) {
			PgmL.Lck.Unlock(); // ! Далее, возможно, мы запустим поток, который самостоятельно заблокирует PgmL
			DClientPolicy policy;
			St.D_Policy.GetData(policy);
			const bool is_there_app_paths = (policy.P.SsAppPaths.getCount() > 0);
			//if(is_there_app_paths) {
				Worker_PgmListResolver * p_thread = new Worker_PgmListResolver(PgmL, policy.P, Cache_Texture);
				if(p_thread)
					p_thread->Start();
			//}
		}
		else 
			PgmL.Lck.Unlock(); // !
	}
	*/
}

void WsCtl_ImGuiSceneBlock::BuildScene()
{
	const LDATETIME now_dtm = getcurdatetime_();
	if(!TestBlk.DtmLastQuerySent || diffdatetimesec(now_dtm, TestBlk.DtmLastQuerySent) > 5) {
		if(P_CmdQ) {
			WsCtlReqQueue::Req qr(PPSCMD_HELLO);
			P_CmdQ->Push(qr);
			TestBlk.DtmLastQuerySent = now_dtm;
			TestBlk.QuerySentCount++;
		}
	}
	//
	// Start the Dear ImGui frame
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	// 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
	ShowDemoWindow = false; // @debug
	if(ShowDemoWindow) {
		// @sobolev 
		ImGui::ShowDemoWindow(/*&show_demo_window*/);
	}
	else {
		// 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
		const int view_flags = ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoDecoration|
			0;
		ImGuiViewport * p_vp = ImGui::GetMainViewport();
		if(p_vp) {

			//PreprocessProgramList();

			ImVec2 sz = p_vp->Size;
			const int _screen = GetScreen();
			SUiLayout::Param evp;
			evp.ForceWidth = static_cast<float>(sz.x);
			evp.ForceHeight = static_cast<float>(sz.y);
			if(_screen == screenLogin) {
				SUiLayout * p_tl = Cache_Layout.Get(&_screen, sizeof(_screen));
				if(p_tl) {
					p_tl->Evaluate(&evp);
					//loidLoginBlock
					ImGuiWindowByLayout wbl(p_tl, loidLoginBlock, "##LOGINBLOCK", view_flags);
					if(wbl.IsValid()) {
						DAuth data_auth;
						St.D_Auth.GetData(data_auth);
						if(data_auth.SCardID && data_auth._Status == 0) {
							SetScreen(screenAuthSelectSess);
						}
						else {
							{
								//
								// Handle an error
								//
								bool is_err = false;
								if(!data_auth.SCardID && data_auth._Status != 0) {
									LastSvrErr = data_auth;
									// Сбрасываем информацию об ошибке {
									data_auth.DServerError::Z();
									St.D_Auth.SetData(data_auth);
									// }
									if(LastSvrErr._Message.NotEmpty())
										is_err = true;
								}
								else if(LastSvrErr._Status != 0) {
									if(LastSvrErr._Message.NotEmpty())
										is_err = true;
								}
								ErrorPopup(is_err);
							}
							ImGui::InputText(InputLabelPrefix("Текст для авторизации"), LoginText, sizeof(LoginText), ImGuiInputTextFlags_CallbackAlways, CbInput, this);
							ImGui::InputText(InputLabelPrefix("Пароль"), PwText, sizeof(PwText), ImGuiInputTextFlags_CallbackAlways, CbInput, this);
							if(!isempty(LoginText)) {
								if(ImGui::Button("Login", ButtonSize_Std) || ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Enter))) {
									WsCtlReqQueue::Req req(PPSCMD_WSCTL_AUTH);
									STRNSCPY(req.P.AuthTextUtf8, LoginText);
									STRNSCPY(req.P.AuthPwUtf8, PwText);
									P_CmdQ->Push(req);
								}
							}
						}
					}
				}
			}
			else if(_screen == screenSession) {
				//SUiLayout * p_tl = Cache_Layout.Get(&_screen, sizeof(_screen));
				const SUiLayout * p_tl_ = Cache_Layout.Get(&_screen, sizeof(_screen));
				if(p_tl_) {
					DAccount st_data_acc;
					DTSess st_data_tses;
					St.D_Acc.GetData(st_data_acc);
					St.D_TSess.GetData(st_data_tses);
					if(st_data_acc.SCardID == 0) {
						SetScreen(screenConstruction); // @todo Здесь надо перейти на какой-то вменяемый экран, а не на отладочную панель!
						SessF.Finish(); // @v11.8.7
					}
					else if(st_data_tses.TSessID == 0) {
						SessF.Finish(); // @v11.8.5
						SetScreen(screenAuthSelectSess);
					}
					else {
						SUiLayout tl(*p_tl_);
						/*if(PgmL.Lck.TryLock())*/ {
							if(PgmL.getCount()) {
								SUiLayout * p_tpg = tl.FindById(loidTestProgramGallery);
								SUiLayout * p_ipg_ = p_tpg ? MakePgmListLayout(PgmL) : 0;
								if(p_ipg_)
									p_tpg->Insert(p_ipg_);
							}
							//PgmL.Lck.Unlock();
						}
						tl.Evaluate(&evp);
						{
							ImGuiWindowByLayout wbl(&tl, loidMenuBlock, "##MENUBLOCK", view_flags);
							if(wbl.IsValid()) {
							}
						}
						{
							ImGuiWindowByLayout wbl(&tl, loidSessionInfo, "##SESSIONINFO", view_flags);
							if(wbl.IsValid()) {
								SString & r_text_buf = SLS.AcquireRvlStr();
								r_text_buf.Cat(st_data_acc.SCardCode).CatDiv('-', 1).Cat(st_data_acc.PersonName);
								ImGui::Text(r_text_buf);
								ImGui::SameLine();
								r_text_buf.Z().Cat(st_data_tses.TmChunk.Start, DATF_DMY, TIMF_HM).Dot().Dot().Cat(st_data_tses.TmChunk.Finish, DATF_DMY, TIMF_HM);
								ImGui::Text(r_text_buf);
								ImGui::SameLine();
								if(ImGui::Button("Logout", ButtonSize_Double)) {
									WsCtlReqQueue::Req req(PPSCMD_WSCTL_LOGOUT);
									P_CmdQ->Push(req);
								}
								ImGui::SameLine();
								if(ImGui::Button("End Session", ButtonSize_Double)) {
									WsCtlReqQueue::Req req(PPSCMD_WSCTL_END_SESS);
									req.P.TSessID = st_data_tses.TSessID;
									req.P.SCardID = st_data_tses.SCardID;
									P_CmdQ->Push(req);
								}
							}
						}
						{
							ImGuiWindowByLayout wbl(&tl, loidSessionProgramGallery, "##PROGRAMGALLERY", view_flags);
							EmitProgramGallery(wbl, tl);
							//if(wbl.IsValid()) {
							//}
						}
					}
				}
			}
			else if(_screen == screenAuthSelectSess) {
				SUiLayout * p_tl = Cache_Layout.Get(&_screen, sizeof(_screen));
				if(p_tl) {
					DTSess st_data_tses;
					St.D_TSess.GetData(st_data_tses);
					if(st_data_tses.TSessID && st_data_tses._Status == 0) {
						SessF.SetClientPolicy(PolicyL); // @v11.8.6
						if(SessF.Start()) { // @v11.8.5
							SetScreen(screenSession);
						}
						else {
							// @todo @err
						}
					}
					else {
						{
							//
							// Handle an error
							//
							bool is_err = false;
							if(!st_data_tses.TSessID && st_data_tses._Status != 0) {
								LastSvrErr = st_data_tses;
								// Сбрасываем информацию об ошибке {
								st_data_tses.DServerError::Z();
								St.D_TSess.SetData(st_data_tses);
								// }
								if(LastSvrErr._Message.NotEmpty())
									is_err = true;
							}
							else if(LastSvrErr._Status != 0) {
								if(LastSvrErr._Message.NotEmpty())
									is_err = true;
							}
							ErrorPopup(is_err);
						}
						p_tl->Evaluate(&evp);
						{
							ImGuiWindowByLayout wbl(p_tl, loidMenuBlock, "##MENUBLOCK", view_flags);
							if(wbl.IsValid()) {
							}
						}
						//
						DAccount st_data_acc;
						St.D_Acc.GetData(st_data_acc);
						{
							ImGuiWindowByLayout wbl(p_tl, loidPersonInfo, "##PERSONINFO", view_flags);
							if(wbl.IsValid()) {
								if(st_data_acc.PersonName) {
									ImGui::Text(st_data_acc.PersonName);
								}
							}
						}
						{
							ImGuiWindowByLayout wbl(p_tl, loidAccountInfo, "##ACCOUNTINFO", view_flags);
							if(wbl.IsValid()) {
								SString & r_text_buf = SLS.AcquireRvlStr();
								ImGui::Text(PPLoadStringUtf8S("account", r_text_buf).CatDiv(':', 2).Cat(st_data_acc.SCardCode));
								ImGui::Text(PPLoadStringUtf8S("rest", r_text_buf).CatDiv(':', 2).Cat(st_data_acc.ScRest, MKSFMTD(0, 2, 0)));
							}
						}
						{
							DPrices st_data_prices;
							St.D_Prices.GetData(st_data_prices);
							{
								ImGuiWindowByLayout wbl(p_tl, loidSessionSelection, "##SESSIONSELECTION", view_flags);
								if(wbl.IsValid()) {
									if(st_data_prices.GoodsList.getCount()) {
										const  PPID selected_tec_goods_id = St.GetSelectedTecGoodsID();
										//PPID   new_selected_tec_goods_id = 0;
										if(ImGui::BeginTable("Prices", 1)) {
											for(uint i = 0; i < st_data_prices.GoodsList.getCount(); i++) {
												const GoodsEntry * p_entry = st_data_prices.GoodsList.at(i);
												if(p_entry) {
													double price = 0.0;
													if(st_data_prices.GetGoodsPrice(p_entry->ID, &price)) {
														//ImGui::RadioButton(
														ImGui::TableNextColumn();
														SString & r_temp_buf = SLS.AcquireRvlStr();
														r_temp_buf.Cat(p_entry->NameUtf8).Space().CatChar('|').Space().Cat(price, MKSFMTD(0, 2, 0));
														if(ImGui::/*RadioButton*/Selectable(r_temp_buf, (p_entry->ID == selected_tec_goods_id)))
															St.SetSelectedTecGoodsID(p_entry->ID);
															//new_selected_tec_goods_id = p_entry->ID;
													}
												}
											}
											ImGui::EndTable();
										}
									}
									{
										const  PPID selected_tec_goods_id = St.GetSelectedTecGoodsID();
										const GoodsEntry * p_ge = st_data_prices.GetGoodsEntryByID(selected_tec_goods_id);
										SString & r_text_buf = SLS.AcquireRvlStr();
										r_text_buf.Cat("Selected Tec Goods ID").CatDiv(':', 2).Cat(p_ge ? p_ge->NameUtf8 : "undefined");
										ImGui::Text(r_text_buf);
									}
								}
							}
							{
								//loidSessionButtonGroup
								ImGuiWindowByLayout wbl(p_tl, loidSessionButtonGroup, "##SESSIONBUTTONGROUP", view_flags);
								if(wbl.IsValid()) {
									PPID  sel_goods_id = St.GetSelectedTecGoodsID();
									const GoodsEntry * p_goods_entry = st_data_prices.GetGoodsEntryByID(sel_goods_id);
									if(p_goods_entry) {
										if(ImGui::Button("Start Session...", ButtonSize_Double)) {
											WsCtl_ImGuiSceneBlock::DPrc prc_data;
											St.D_Prc.GetData(prc_data); 
											if(!!prc_data.PrcUuid) {
												WsCtlReqQueue::Req req(PPSCMD_WSCTL_BEGIN_SESS);
												req.P.SCardID = st_data_acc.SCardID;
												req.P.Uuid = prc_data.PrcUuid;
												req.P.GoodsID = sel_goods_id;
												if(p_goods_entry->TechList.getCount()) {
													req.P.TechID = p_goods_entry->TechList.at(0).ID;
												}
												{
													double price = 0.0;
													if(st_data_prices.GetGoodsPrice(sel_goods_id, &price) && price > 0.0) {
														req.P.Amount = price * 1.0;
													}
												}
												P_CmdQ->Push(req);
											}
										}
									}
								}
							}
						}
						{
							ImGuiWindowByLayout wbl(p_tl, loidBottomCtrlGroup, "##BOTTOMCTRLGROUP", view_flags);
							if(wbl.IsValid()) {
							}
						}
					}
				}
			}
			else if(_screen == screenConstruction) {
				//
				// Здесь рабочий экземпляр лейаута мы создаем на стеке (tl) ибо необходимо вставить в него динамический лейаут галлереи!
				//
				const SUiLayout * p_tl_ = Cache_Layout.Get(&_screen, sizeof(_screen));
				if(p_tl_) {
					SUiLayout tl(*p_tl_);
					/*if(PgmL.Lck.TryLock())*/{
						if(PgmL.getCount()) {
							SUiLayout * p_tpg = tl.FindById(loidTestProgramGallery);
							SUiLayout * p_ipg_ = p_tpg ? MakePgmListLayout(PgmL) : 0;
							if(p_ipg_)
								p_tpg->Insert(p_ipg_);
						}
						//PgmL.Lck.Unlock();
					}
					tl.Evaluate(&evp);
					ImGuiObjStack __ost;
					__ost.PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
					{
						ImGuiWindowByLayout wbl(&tl, loidMenuBlock, "##MENUBLOCK", view_flags);
						if(wbl.IsValid()) {
							FRect rect_frame = tl.GetFrameAdjustedToParent();
							ImVec2 button_size;
							button_size.x = rect_frame.Width() - 2.0f;
							button_size.y = 0.0f;
							ImGui::Text("X");
							if(ImGui::Button("Login...", button_size)) {
								SetScreen(screenLogin);
							}
							if(ImGui::Button("Select session...", button_size)) {
								SetScreen(screenAuthSelectSess);
							}
							if(ImGui::Button("Start...", button_size)) {
								;
							}
							if(ImGui::Button("Config...", button_size)) {
								SETIFZQ(P_Dlg_Cfg, new ImDialog_WsCtlConfig(*this, &JsP));
							}
						}
					}
					{
						ImGuiWindowByLayout wbl(&tl, loidCtl01, "##CTL-01", view_flags);
						if(wbl.IsValid()) {
							{
								if(P_Dlg_Cfg) {
									int r = P_Dlg_Cfg->Build();
									if(r != 0) {
										if(r > 0) {
											if(P_Dlg_Cfg->CommitData()) {
												JsP.Write();
											}
										}
										ZDELETE(P_Dlg_Cfg);
									}
								}
							}
							DAccount data_acc;
							St.D_Acc.GetData(data_acc);
							if(data_acc.SCardID) {
								SString & r_temp_buf = SLS.AcquireRvlStr();
								ImGui::Text(r_temp_buf.Z().Cat("Аккаунт").CatDiv(':', 2).Cat(data_acc.SCardCode));
								ImGui::Text(r_temp_buf.Z().Cat(data_acc.PersonName));
								ImGui::Text(r_temp_buf.Z().Cat("Остаток").CatDiv(':', 2).Cat(data_acc.ScRest, MKSFMTD(0, 2, 0)));
							}
							else {
								//ImGui::SetKeyboardFocusHere();
								DTSess data_sess;
								St.D_TSess.GetData(data_sess);
								if(data_sess.TSessID) {
									SString & r_temp_buf = SLS.AcquireRvlStr();
									ImGui::Text(r_temp_buf.Z().Cat("Processor is buisy till").Space().Cat(data_sess.TmChunk.Finish, DATF_DMY, TIMF_HM));
								}
								ImGui::InputText(InputLabelPrefix("Текст для авторизации"), LoginText, sizeof(LoginText), ImGuiInputTextFlags_CallbackAlways, CbInput, this);
								ImGui::InputText(InputLabelPrefix("Пароль"), PwText, sizeof(PwText), ImGuiInputTextFlags_CallbackAlways, CbInput, this);
								if(!isempty(LoginText)) {
									if(ImGui::Button("Login", ButtonSize_Std)) {
										WsCtlReqQueue::Req req(PPSCMD_WSCTL_AUTH);
										STRNSCPY(req.P.AuthTextUtf8, LoginText);
										STRNSCPY(req.P.AuthPwUtf8, PwText);
										P_CmdQ->Push(req);
									}
								}
							}
							{
								//ImGui::PopAllowKeyboardFocus
							}
							// @debug {
							if(P_TestImgTexture) {
								ImVec2 sz(128.0f, 128.0f);
								ImGui::Image(P_TestImgTexture, sz);
							}
							// } @debug 
						}
					}
					{
						ImGuiWindowByLayout wbl(&tl, loidCtl02, "##CTL-02", view_flags);
						if(wbl.IsValid()) {
							//ImGui::Text("CTL-02");
							ImGui::Text(SLS.AcquireRvlStr().Cat("Сервер").CatDiv(':', 2).Cat(JsP.Server).CatChar(':').Cat(JsP.Port));
							{
								DConnectionStatus conn_status;
								St.D_ConnStatus.GetData(conn_status);
								ImGui::Text(SLS.AcquireRvlStr().Cat("Connection status").CatDiv(':', 2).Cat((conn_status.S == 0) ? conn_status._Message.cptr() : "OK"));
							}
							{
								DTest test_result;
								St.D_Test.GetData(test_result);
								ImGui::Text(SLS.AcquireRvlStr().Cat("Test status").Space().CatParStr(TestBlk.QuerySentCount).CatDiv(':', 2).Cat(test_result.Reply));
							}
							{
								DPrc prc_data;
								St.D_Prc.GetData(prc_data);
								if(prc_data.PrcName.NotEmpty()) {
									ImGui::Text(SLS.AcquireRvlStr().Cat("Processor Name").CatDiv(':', 2).Cat(prc_data.PrcName));
								}
							}
							{
								DPrices st_data_prices;
								St.D_Prices.GetData(st_data_prices);
								if(st_data_prices.GoodsList.getCount()) {
									const  PPID selected_tec_goods_id = St.GetSelectedTecGoodsID();
									//PPID   new_selected_tec_goods_id = 0;
									if(ImGui::BeginTable("Prices", 1)) {
										for(uint i = 0; i < st_data_prices.GoodsList.getCount(); i++) {
											const GoodsEntry * p_entry = st_data_prices.GoodsList.at(i);
											if(p_entry) {
												//ImGui::RadioButton(
												ImGui::TableNextColumn();
												const char * p_item_text = p_entry->NameUtf8;
												if(ImGui::/*RadioButton*/Selectable(p_item_text, (p_entry->ID == selected_tec_goods_id)))
													St.SetSelectedTecGoodsID(p_entry->ID);
													//new_selected_tec_goods_id = p_entry->ID;
											}
										}
										ImGui::EndTable();
									}
								}
								{
									const  PPID selected_tec_goods_id = St.GetSelectedTecGoodsID();
									const GoodsEntry * p_ge = st_data_prices.GetGoodsEntryByID(selected_tec_goods_id);
									SString & r_text_buf = SLS.AcquireRvlStr();
									r_text_buf.Cat("Selected Tec Goods ID").CatDiv(':', 2).Cat(p_ge ? p_ge->NameUtf8 : "undefined");
									ImGui::Text(r_text_buf);
								}
							}
						}
					}
					{
						ImGuiWindowByLayout wbl(&tl, loidTestProgramGallery, "##TestProgramGallery", 
							view_flags/*|ImGuiWindowFlags_NoMouseInputs|ImGuiWindowFlags_HorizontalScrollbar|ImGuiWindowFlags_AlwaysHorizontalScrollbar*/);
						EmitProgramGallery(wbl, tl);
					}
					/*{
						ImGuiWindowByLayout wbl(p_tl, loidAdv02, "##ADV-02", view_flags);
						if(wbl.IsValid()) {
							//ImGui::Text("ADV-02");
						}
					}
					{
						ImGuiWindowByLayout wbl(p_tl, loidAdv03, "##ADV-03", view_flags);
						if(wbl.IsValid()) {
							//ImGui::Text("ADV-03");
						}
					}*/
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

static int MakeColorSet(UiDescription & rUiDescr)
{
	int    ok = 1;
	const  char * p_cset_symb = "ImGuiStyle";
	const ImGuiStyle * p_style = &ImGui::GetStyle();
	if(p_style) {
		SColorSet * p_ex_cset = rUiDescr.GetColorSet(p_cset_symb);
		SColorSet * p_cset = NZOR(p_ex_cset, new SColorSet(p_cset_symb));
		uint    _c = 0;
		for(uint i = 0; i < ImGuiCol_COUNT; i++) {
			const ImVec4 & r_src_color = p_style->Colors[i];
			const char * p_symb = ImGui::GetStyleColorName(i);
			if(!isempty(p_symb)) {
				SColorF cf = r_src_color;
				/*
				if(p_cset->Put(p_symb, cf))
					_c++;
				*/
			}
		}
		if(_c) {
			if(!p_ex_cset)
				rUiDescr.ClrList.insert(p_cset);
		}
		else if(!p_ex_cset)
			ZDELETE(p_cset);
	}
	return ok;
}

int main(int, char**)
{
	int    result = 0;
	//
	DS.Init(PPSession::fWsCtlApp|PPSession::fInitPaths, 0);
	// Create application window
	//ImGui_ImplWin32_EnableDpiAwareness();
	WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"WSCTL_WCLS", nullptr };
	::RegisterClassExW(&wc);
	HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"WSCTL", WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, nullptr, nullptr, wc.hInstance, nullptr);
	// Initialize Direct3D
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
		(void)io;
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
		WsCtl_ImGuiSceneBlock scene_blk;
		scene_blk.Init(io);
		//scene_blk.LoadProgramList(); // @v11.7.12
		scene_blk.SetScreen(WsCtl_ImGuiSceneBlock::screenConstruction);
		{
			const char * p_img_path = "/Papyrus/Src/PPTEST/DATA/test-gif.gif";
			P_TestImgTexture = ImgRtb.LoadTexture(p_img_path);
		}
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
				scene_blk.EmitEvents();
				scene_blk.BuildScene();
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
