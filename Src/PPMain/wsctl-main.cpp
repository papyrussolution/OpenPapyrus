// WSCTL-MAIN.CPP
// Copyright (c) A.Sobolev 2023
// @codepage UTF-8
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
#include <..\OSF\DirectXTex\SRC\DirectXTex.h>
#include <..\OSF\DirectXTex\SRC\WICTextureLoader11.h>

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
	if(2 * d < da)
		return 2 * s * d;
	else
		return sa * da - 2 * (da - d) * (sa - s);
}

FORCEINLINE ImVec4 __ColorBlendOverlay(const ImVec4 & rS, const ImVec4 & rD, float alpha)
{
	return ImVec4(
		ColorComponentBlendOverlay(1.0f, rS.x, alpha, rD.x),
		ColorComponentBlendOverlay(1.0f, rS.y, alpha, rD.y),
		ColorComponentBlendOverlay(1.0f, rS.z, alpha, rD.z),
		rS.w);
}

static const ImVec4 MainBackgroundColor(SColor(0x1E, 0x22, 0x28));

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
		ImGui::PushFont(pFont);
		St.push(objFont);
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
			Param() : SCardID(0), GoodsID(0), TechID(0)
			{
				AuthTextUtf8[0] = 0;
				AuthPwUtf8[0] = 0;
			}
			S_GUID Uuid;
			PPID   SCardID; // 
			PPID   GoodsID;
			PPID   TechID;
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
// Descr: Блок само-идентификации. 
//
class WsCtl_SelfIdentityBlock {
public:
	WsCtl_SelfIdentityBlock() : PrcID(0)
	{
	}
	int    GetOwnUuid()
	{
		int    ok = 1;
		bool   found = false;
		S_GUID _uuid;
		{
			WinRegKey reg_key(HKEY_LOCAL_MACHINE, PPConst::WrKey_WsCtl, 1);
			if(reg_key.GetBinary(PPConst::WrParam_WsCtl_MachineUUID, &_uuid, sizeof(_uuid)) > 0) {
				Uuid = _uuid;
				found = true;
			}
		}
		if(!found) {
			WinRegKey reg_key(HKEY_LOCAL_MACHINE, PPConst::WrKey_WsCtl, 0);
			_uuid.Generate();
			if(reg_key.PutBinary(PPConst::WrParam_WsCtl_MachineUUID, &_uuid, sizeof(_uuid))) {
				Uuid = _uuid;
				ok = 2;
			}
			else
				ok = 0;
		}
		if(ok > 0) {
			SString msg_buf;
			msg_buf.Z().Cat("WSCTL own-uuid").CatDiv(':', 2).Cat(Uuid, S_GUID::fmtIDL);
			PPLogMessage("wsctl-debug.log", msg_buf, LOGMSGF_TIME|LOGMSGF_COMP|LOGMSGF_SLSSESSGUID);
		}
		return ok;
	}
	S_GUID   Uuid;
	PPID   PrcID; // Идентификатор процессора на сервере. Инициируется ответом от сервера.
	SString PrcName; // Наименование процессора на сервере. Инициируется ответом от сервера.
};

class WsCtl_ImGuiSceneBlock {
public:
	//
	// Descr: Вариант экранов приложения //
	//
	enum {
		screenUndef = 0,    // неопределенный
		screenConstruction, // Тестовый режим для разработки 
		screenHybernat,     // спящий режим 
		screenRegister,         // регистрация //
		screenLogin,            // авторизация //
		screenAuthSelectSess,   // авторизованный режим - выбор сессии
		screenSession           // рабочая сессия //
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
		DServerError() : _Status(0)
		{
		}
		DServerError(const DServerError & rS)
		{
			Copy(rS);
		}
		DServerError & FASTCALL operator = (const DServerError & rS)
		{
			return Copy(rS);
		}
		DServerError & Copy(const DServerError & rS)
		{
			_Status = rS._Status;
			_Message = rS._Message;
			return *this;
		}
		DServerError & Z()
		{
			_Status = 0;
			_Message.Z();
			return *this;
		}
		DServerError & SetupByLastError()
		{
			_Status = PPErrCode;
			PPGetLastErrorMessage(1, _Message);
			_Message.Transf(CTRANSF_INNER_TO_UTF8);
			return *this;
		}
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
		DConnectionStatus() : S(0)
		{
		}
		int    S;
	};
	//
	// Descr: Информационный блок о состоянии процессора на сервере, с которым ассоциирована данная рабочая станция //
	//
	class DPrc : public DServerError {
	public:
		DPrc() : PrcID(0), CurrentTSessID(0), ReservedTSessID(0)
		{
		}
		S_GUID PrcUuid;
		PPID   PrcID;
		PPID   CurrentTSessID;  // Текущая сессия процессора
		PPID   ReservedTSessID; // Ближайшая (будущая) зарезервированная сессия процессора
		SString PrcName;
	};
	class DAccount : public DServerError {
	public:
		DAccount() : SCardID(0), PersonID(0), ScRest(0.0)
		{
		}
		PPID   SCardID;
		PPID   PersonID;
		double ScRest;
		SString SCardCode;
		SString PersonName;
	};
	class DAuth : public DServerError {
	public:
		DAuth() : State(0), SCardID(0), PersonID(0)
		{
		}
		DAuth & Z()
		{
			State = 0;
			SCardID = 0;
			PersonID = 0;
			DServerError::Z();
			return *this;
		}
		enum {
			stWaitOn = 0x0001 // Объект находится в состоянии ожидания результата авторизации
		};
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

		TSCollection <QuotKindEntry> QkList; // Сервер передает ранжированный список. То есть, значения надо брать исходя из того, что первое приоритетнее следюущего.
		TSCollection <GoodsEntry> GoodsList;
	};
	class DTSess : public DServerError {
	public:
		DTSess() : TSessID(0), GoodsID(0), TechID(0)
		{
		}
		int    FromJsonObject(const SJson * pJsObj)
		{
			int    ok = 0;
			if(pJsObj) {
			}
			return ok;
		}
		PPID   TSessID;
		PPID   GoodsID;
		PPID   TechID;
		STimeChunk TmChunk;
	};
	struct JobSrvParam {
		JobSrvParam() : Port(0), Timeout(0)
		{
		}
		SString Addr;
		int    Port;
		int    Timeout;
		//
		SString DbSymb;   // @debug Это поле необходимо будет инкапуслировать во что-то секретное и решить как инициировать
		SString User;     // @debug Это поле необходимо будет инкапуслировать во что-то секретное и решить как инициировать
		SString Password; // @debug Это поле необходимо будет инкапуслировать во что-то секретное и решить как инициировать
	};
	//
	// Descr: Структура, описывающая текущее состояние системы.
	//   Элементы структуры могут обновляються другими потоками.
	//
	class State {
	public:
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
		};
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
				if(P_OuterState) {
					P_OuterState->D_LastErr.SetData(rData);
				}
				Lck.Unlock();
			}
			void GetData(T & rData)
			{
				Lck.Lock();
				rData = Data;
				Lck.Unlock();
			}
		private:
			const  int SyncDataId; // syncdataXXX
			T      Data;
			SMtLock Lck;
			int64  TmActual;
			int64  TmExpiry;
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

		State() : D_Prc(syncdataPrc), D_Test(syncdataTest), D_Acc(syncdataAccount), D_Prices(syncdataPrices), D_TSess(syncdataTSess), 
			D_ConnStatus(syncdataJobSrvConnStatus), D_Auth(syncdataAuth), SelectedTecGoodsID(0), D_LastErr(syncdataServerError)
		{
		}
		PPID   GetSelectedTecGoodsID() const { return SelectedTecGoodsID; }
		bool   SetSelectedTecGoodsID(PPID goodsID);
		int    SetupSyncUpdateTime(int syncDataId, uint msecToUpdate);
		int    CheckSyncUpdateTimers(LongArray & rList) const;
		int    SetSyncUpdateTimerLastReqTime(int syncDataId);
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
	};
	int GetScreen() const { return Screen; }
	int SetScreen(int scr)
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
private:
	//
	// Descr: Поток, реализующий запросы к серверу.
	//   Так как концепция imgui предполагает перманентную отрисовку сетевые запросы придется //
	//   делать строго асинхронно.
	//
	class WsCtl_CliSession : public PPThread {
	public:
		WsCtl_CliSession(const JobSrvParam & rJsP, WsCtl_ImGuiSceneBlock::State * pSt, WsCtlReqQueue * pQ);
		virtual void Run();
	private:
		virtual void Startup();
		int    Connect(PPJobSrvClient & rCli);
		void   SendRequest(PPJobSrvClient & rCli, const WsCtlReqQueue::Req & rReq);
		// Указатель на состояние блока управления панелью. При получении ответа от сервера
		// наш поток будет вносить изменения в это состояние (защита блокировками подразумевается).
		WsCtl_ImGuiSceneBlock::State * P_St; // @notowned
		WsCtlReqQueue * P_Queue; // @notowned
		JobSrvParam JsP;
	};
	bool   ShowDemoWindow; // @sobolev true-->false
	bool   ShowAnotherWindow;
	int    Screen; // screenXXX
	ImVec4 ClearColor;
	//SUiLayout Lo01; // = new SUiLayout(SUiLayoutParam(DIREC_VERT, 0, SUiLayoutParam::alignStretch));
	TSHashCollection <SUiLayout> Cache_Layout;
	TSHashCollection <SCachedFileEntity> Cache_Texture;
	JobSrvParam JsP;
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
	enum {
		// Значения до 10000 используются для идентификации экранов (screenXXX) и 
		// одновременно как идентификаторы соответствующих лейаутов верхнего уровня.
		//loidRoot = 1,
		loidUpperGroup = 10001,
		loidBottomGroup,
		loidCtl01,
		loidCtl02,
		loidAdv01,
		loidAdv02,
		loidAdv03,
		loidMenuBlock,
		loidMainGroup,
		loidLoginBlock,
		loidPersonInfo,
		loidAccountInfo,
		loidSessionSelection,
		loidSessionButtonGroup,
		loidBottomCtrlGroup,
	};
	//
	void   Render();
	void   MakeLayout();
	static int CbInput(ImGuiInputTextCallbackData * pInputData);
	//
	char   TestInput[128];
	char   LoginText[256];
	char   PwText[128];
	DServerError LastSvrErr;
public:
	WsCtl_ImGuiSceneBlock() : ShowDemoWindow(false), ShowAnotherWindow(false), Screen(screenUndef),
		//ClearColor(0.45f, 0.55f, 0.60f, 1.00f), 
		ClearColor(SColor(0x1E, 0x22, 0x28)),
		P_CmdQ(new WsCtlReqQueue),
		Cache_Layout(512), Cache_Texture(1024)
	{
		TestInput[0] = 0;
		LoginText[0] = 0;
		PwText[0] = 0;
		MakeLayout();
	}
	~WsCtl_ImGuiSceneBlock()
	{
		// P_CmdQ не разрушаем поскольку на него ссылается отдельный поток.
		// Все равно этот объект живет в течении всего жизненного цикла процесса.
	}
	int  Init()
	{
		int    ok = 0;
		PPIniFile ini_file;
		if((ini_file.GetInt(PPINISECT_SERVER, PPINIPARAM_SERVER_PORT, &JsP.Port) <= 0 || JsP.Port <= 0))
			JsP.Port = InetUrl::GetDefProtocolPort(InetUrl::prot_p_PapyrusServer);//DEFAULT_SERVER_PORT;
		if(ini_file.GetInt(PPINISECT_SERVER, PPINIPARAM_CLIENTSOCKETTIMEOUT, &JsP.Timeout) <= 0 || JsP.Timeout <= 0)
			JsP.Timeout = -1;
		ini_file.Get(PPINISECT_SERVER, PPINIPARAM_SERVER_NAME, JsP.Addr);
		St.SidBlk.GetOwnUuid();
		if(JsP.Addr.NotEmpty()) {
			//
			JsP.DbSymb = "wsctl"; // @debug
			JsP.User = "master"; // @debug
			JsP.Password = "";   // @debug 
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
		}
		return ok;
	}
	void EmitEvents()
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
						{
							WsCtlReqQueue::Req req(PPSCMD_WSCTL_GETQUOTLIST);
							P_CmdQ->Push(req);
						}
						break;
					case State::syncdataTSess:
						break;
					case State::syncdataJobSrvConnStatus:
						break;
				}
				St.SetSyncUpdateTimerLastReqTime(sync_data_id); // Отмечаем время отправки запроса
			}
		}
		else {
			assert(SyncReqList.getCount() == 0);
		}
	}
	static SString & InputLabelPrefix(const char * pLabel)
	{
		float width = ImGui::CalcItemWidth();
		float x = ImGui::GetCursorPosX();
		ImGui::Text(pLabel); 
		//ImGui::SameLine(); 
		//ImGui::SetCursorPosX(x + width * 0.5f + ImGui::GetStyle().ItemInnerSpacing.x);
		ImGui::SetNextItemWidth(-1);
		return SLS.AcquireRvlStr().CatCharN('#', 2).Cat(pLabel);
	}
	void BuildScene();
};
//
//
//
WsCtl_ImGuiSceneBlock::DPrices::DPrices()
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
	TSCollection_Copy(QkList, rS.QkList);
	TSCollection_Copy(GoodsList, rS.GoodsList);
	return *this;
}

WsCtl_ImGuiSceneBlock::DPrices & WsCtl_ImGuiSceneBlock::DPrices::Z()
{
	DServerError::Z();
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
				const PPID qk_id = QkList.at(i)->ID;
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
	/*
		{
			"qk_list" : [
				{
					"id" : int
					"nm" : string
				}
			]
			"goods_list" : [
				{
					"id" : int
					"nm" : string
					"quot_list" : [
						{
							"id" : int
							"val" : double
						}
						{
							"id" : int
							"val" : double
						}
					]
				}
			]
		}
	*/
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
WsCtl_ImGuiSceneBlock::WsCtl_CliSession::WsCtl_CliSession(const JobSrvParam & rJsP, WsCtl_ImGuiSceneBlock::State * pSt, WsCtlReqQueue * pQ) : 
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
					srv_conn_status.S = cli.Connect(JsP.Addr, JsP.Port);
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
	THROW(rCli.Connect(JsP.Addr, JsP.Port));
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
				if(rCli.Exec("HELLO", reply) && reply.StartReading(&temp_buf)) {
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
				if(rCli.Exec(cmd, reply)) {
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
							p_c = p_js_obj->FindChildByKey("personid");
							if(SJson::IsNumber(p_c)) {
								st_data.PersonID = p_c->Text.ToLong();
							}
							st_data.State = 0;
							P_St->D_Auth.SetData(st_data);
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
				if(!data_settled)
					P_St->D_Auth.SetData(st_data);
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
				if(rCli.Exec(cmd, reply)) {
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
				if(rCli.Exec(cmd, reply)) {
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
				P_St->D_Prc.SetData(st_data);
			}
			break;
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
					if(rCli.Exec(cmd, reply)) {
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
				P_St->D_Prices.SetData(st_data);
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
				PPJobSrvCmd cmd;
				cmd.StartWriting(PPSCMD_WSCTL_BEGIN_SESS);
				{
					js_param.ToStr(temp_buf);
					SString mime_buf;
					mime_buf.EncodeMime64(temp_buf.ucptr(), temp_buf.Len());
					cmd.Write(mime_buf.ucptr(), mime_buf.Len()+1);
				}
				cmd.FinishWriting();
				if(rCli.Exec(cmd, reply)) {
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
				P_St->D_TSess.SetData(st_data);
			}
			break;
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

void WsCtl_ImGuiSceneBlock::MakeLayout()
{
	const FRect margin_(2.0f, 1.0f, 2.0f, 1.0f);
	{
		//
		// screenLogin
		//
		SUiLayout * p_tl = new SUiLayout();
		SUiLayoutParam alb(DIREC_VERT, SUiLayoutParam::alignCenter, SUiLayoutParam::alignCenter);
		alb.AlignItems = SUiLayoutParam::alignCenter;
		p_tl->SetLayoutBlock(alb);
		p_tl->SetID(screenLogin);
		{
			SUiLayoutParam alb(DIREC_HORZ, SUiLayoutParam::alignCenter, SUiLayoutParam::alignCenter);
			alb.SetFixedSizeX(480).SetFixedSizeY(360);
			p_tl->InsertItem(0, &alb, loidLoginBlock);
		}
		Cache_Layout.Put(p_tl, true);
	}
	{
		//
		// screenAuthSelectSess
		//
		SUiLayout * p_tl = new SUiLayout();
		SUiLayoutParam alb(DIREC_HORZ, SUiLayoutParam::alignCenter, SUiLayoutParam::alignCenter);
		p_tl->SetLayoutBlock(alb);
		p_tl->SetID(screenAuthSelectSess);
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
	}
	{
		//
		// screenSession
		//
	}
	{
		//
		// screenConstruction
		//
		SUiLayout * p_tl = new SUiLayout();
		p_tl->SetLayoutBlock(SUiLayoutParam(DIREC_HORZ, 0, SUiLayoutParam::alignStretch));
		p_tl->SetID(screenConstruction);
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
					{
						SUiLayoutParam alb01(DIREC_VERT, 0, SUiLayoutParam::alignStretch);
						alb01.SetMargin(margin_).SetGrowFactor(1.0f).SetVariableSizeY(SUiLayoutParam::szByContainer, 1.0f);
						p_lo_dn_group->InsertItem(0, &alb01, loidAdv01);
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
					}
				}
			}
		}
		Cache_Layout.Put(p_tl, true);
	}
}

class ImGuiWindowByLayout {
public:
	ImGuiWindowByLayout(const SUiLayout * pLoParent, int loId, const char * pWindowId, ImGuiWindowFlags viewFlags) : 
		P_Lo(pLoParent ? pLoParent->FindByID(loId) : 0)
	{
		if(P_Lo) {
			FRect r = P_Lo->GetFrameAdjustedToParent();
			SPoint2F s = r.GetSize();
			ImVec2 lu(r.a.x, r.a.y);
			ImVec2 sz(s.x, s.y);
			ImGui::SetNextWindowPos(lu);
			ImGui::SetNextWindowSize(sz);
			SString & r_symb = SLS.AcquireRvlStr();
			if(pWindowId)
				r_symb = pWindowId;
			else {
				//char  random_data[20];
				//SLS.GetTLA().Rg.ObfuscateBuffer(random_data, sizeof(random_data));
				//r_symb.EncodeMime64(random_data, sizeof(random_data)).Insert(0, "##");
				r_symb.Cat(loId);
			}
			ImGui::Begin(r_symb, 0, viewFlags);
		}
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
			ImVec2 sz = p_vp->Size;
			const int _screen = GetScreen();
			SUiLayout::Param evp;
			evp.ForceWidth = static_cast<float>(sz.x);
			evp.ForceHeight = static_cast<float>(sz.y);
			if(_screen == screenLogin) {
				SUiLayout * p_tl = Cache_Layout.Get(reinterpret_cast<const char *>(&_screen), sizeof(_screen));
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
							if(is_err) {
								const char * p_popup_title = "Error message";
								ImGui::OpenPopup(p_popup_title);
								if(ImGui::BeginPopup(p_popup_title)) {
									ImGui::Text(LastSvrErr._Message);
									if(ImGui::Button("Close")) {
										LastSvrErr.Z(); // Сбрасываем информацию об ошибке
										ImGui::CloseCurrentPopup();
									}
									ImGui::EndPopup();
								}
							}
							ImGui::InputText(InputLabelPrefix("Текст для авторизации"), LoginText, sizeof(LoginText), ImGuiInputTextFlags_CallbackAlways, CbInput, this);
							ImGui::InputText(InputLabelPrefix("Пароль"), PwText, sizeof(PwText), ImGuiInputTextFlags_CallbackAlways, CbInput, this);
							if(!isempty(LoginText)) {
								if(ImGui::Button("Login") || ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Enter))) {
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
			else if(_screen == screenAuthSelectSess) {
				SUiLayout * p_tl = Cache_Layout.Get(reinterpret_cast<const char *>(&_screen), sizeof(_screen));
				if(p_tl) {
					p_tl->Evaluate(&evp);
					{
						ImGuiWindowByLayout wbl(p_tl, loidMenuBlock, "##MENUBLOCK", view_flags);
						if(wbl.IsValid()) {
						}
					}
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
									const PPID selected_tec_goods_id = St.GetSelectedTecGoodsID();
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
									const PPID selected_tec_goods_id = St.GetSelectedTecGoodsID();
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
									if(ImGui::Button("Start Session...")) {
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
			else if(_screen == screenConstruction) {
				SUiLayout * p_tl = Cache_Layout.Get(reinterpret_cast<const char *>(&_screen), sizeof(_screen));
				if(p_tl) {
					p_tl->Evaluate(&evp);
					ImGuiObjStack __ost;
					__ost.PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
					{
						ImGuiWindowByLayout wbl(p_tl, loidMenuBlock, "##MENUBLOCK", view_flags);
						if(wbl.IsValid()) {
							ImGui::Text("X");
							if(ImGui::Button("Login...")) {
								SetScreen(screenLogin);
							}
							if(ImGui::Button("Select session...")) {
								SetScreen(screenAuthSelectSess);
							}
							if(ImGui::Button("Start...")) {
								;
							}
						}
					}
					{
						ImGuiWindowByLayout wbl(p_tl, loidCtl01, "##CTL-01", view_flags);
						if(wbl.IsValid()) {
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
								ImGui::InputText(InputLabelPrefix("Текст для авторизации"), LoginText, sizeof(LoginText), ImGuiInputTextFlags_CallbackAlways, CbInput, this);
								ImGui::InputText(InputLabelPrefix("Пароль"), PwText, sizeof(PwText), ImGuiInputTextFlags_CallbackAlways, CbInput, this);
								if(!isempty(LoginText)) {
									if(ImGui::Button("Login")) {
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
						}
					}
					{
						ImGuiWindowByLayout wbl(p_tl, loidCtl02, "##CTL-02", view_flags);
						if(wbl.IsValid()) {
							//ImGui::Text("CTL-02");
							ImGui::Text(SLS.AcquireRvlStr().Cat("Сервер").CatDiv(':', 2).Cat(JsP.Addr).CatChar(':').Cat(JsP.Port));
							{
								DConnectionStatus conn_status;
								St.D_ConnStatus.GetData(conn_status);
								if(conn_status.S == 0) {
									ImGui::Text(SLS.AcquireRvlStr().Cat("Connection status").CatDiv(':', 2).Cat(conn_status._Message));
								}
								else {
									ImGui::Text(SLS.AcquireRvlStr().Cat("Connection status").CatDiv(':', 2).Cat("OK"));
								}
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
									const PPID selected_tec_goods_id = St.GetSelectedTecGoodsID();
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
									const PPID selected_tec_goods_id = St.GetSelectedTecGoodsID();
									const GoodsEntry * p_ge = st_data_prices.GetGoodsEntryByID(selected_tec_goods_id);
									SString & r_text_buf = SLS.AcquireRvlStr();
									r_text_buf.Cat("Selected Tec Goods ID").CatDiv(':', 2).Cat(p_ge ? p_ge->NameUtf8 : "undefined");
									ImGui::Text(r_text_buf);
								}
							}
						}
					}
					{
						ImGuiWindowByLayout wbl(p_tl, loidAdv01, "##ADV-01", view_flags);
						if(wbl.IsValid()) {
							//ImGui::Text("ADV-01");
							// @debug {
							if(P_TestImgTexture) {
								ImVec2 sz(128.0f, 128.0f);
								ImGui::Image(P_TestImgTexture, sz);
							}
							// } @debug 
						}
					}
					{
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
					}
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

static void WsCtlStyleColors(ImGuiStyle * dst)
{
	ImGuiStyle * style = dst ? dst : &ImGui::GetStyle();
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
}

int main(int, char**)
{
	int    result = 0;
	//
	DS.Init(PPSession::fWsCtlApp, 0);
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
		WsCtlStyleColors(0);
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
		scene_blk.Init();
		scene_blk.SetScreen(WsCtl_ImGuiSceneBlock::screenConstruction);
		{
			static const ImWchar ranges[] = {
				0x0020, 0x00FF, // Basic Latin + Latin Supplement
				0x0400, 0x044F, // Cyrillic
				0,
			};
			///Papyrus/Src/Rsrc/Font/imgui/Roboto-Medium.ttf
			//C:/Windows/Fonts/Tahoma.ttf
			ImFont * p_font = io.Fonts->AddFontFromFileTTF("/Papyrus/Src/Rsrc/Font/imgui/Roboto-Medium.ttf", 16.0f, nullptr, ranges);
		}
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
