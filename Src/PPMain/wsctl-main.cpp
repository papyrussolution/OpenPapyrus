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
//
// Descr: Очередь команд потоку, передающему эти команды серверу
//
class WsCtlReqQueue : private SQueue { //req_queue(1024)
public:
	//
	// Descr: Запрос к серверу. Помещается в WsCtlReqQueue потому должен быть @flat.
	//
	struct Req { // @flat
		Req() : Cmd(0), Dummy(0)
		{
		}
		Req(uint cmd) : Cmd(cmd), Dummy(0)
		{
		}
		uint   Cmd;
		uint32 Dummy;
		struct Param {
			S_GUID Uuid;
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
	struct QuotKindEntry {
		QuotKindEntry() : ID(0)
		{
		}
		PPID   ID;
		SString NameUtf8;
	};
	struct QuotEntry {
		QuotEntry() : QkID(0), Value(0.0)
		{
		}
		PPID   QkID;
		double Value;
	};
	struct GoodsEntry {
		GoodsEntry() : GoodsID(0)
		{
		}
		PPID   GoodsID;
		SString NameUtf8;
		TSVector <QuotEntry> QuotList;
	};
	//
	struct DTest {
		SString Reply;
	};
	//
	// Descr: Информационный блок о состоянии процессора на сервере, с которым ассоциирована данная рабочая станция //
	//
	struct DPrc {
		DPrc() : PrcID(0), CurrentTSessID(0), ReservedTSessID(0)
		{
		}
		S_GUID PrcUuid;
		PPID   PrcID;
		PPID   CurrentTSessID;  // Текущая сессия процессора
		PPID   ReservedTSessID; // Ближайшая (будущая) зарезервированная сессия процессора
		SString PrcName;
	};
	struct DAccount {
	};
	struct DPrices {
		DPrices()
		{
		}
		DPrices(const DPrices & rS)
		{
			Copy(rS);
		}
		DPrices & FASTCALL operator == (const DPrices & rS)
		{
			return Copy(rS);
		}
		DPrices & FASTCALL Copy(const DPrices & rS)
		{
			TSCollection_Copy(QkList, rS.QkList);
			TSCollection_Copy(GoodsList, rS.GoodsList);
			return *this;
		}
		DPrices & Z()
		{
			QkList.clear();
			GoodsList.clear();
			return *this;
		}
		int    FromJsonObject(const SJson * pJsObj)
		{
			/*
				{
					"quotkind_list" : [
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
						if(p_cur->Text.IsEqiAscii("quotkind_list")) {
							if(SJson::IsArray(p_cur->P_Child)) {
								for(const SJson * p_js_qk = p_cur->P_Child->P_Child; p_js_qk; p_js_qk = p_js_qk->P_Next) {
									if(SJson::IsObject(p_js_qk)) {
										for(const SJson * p_qk_cur = p_js_qk->P_Child; p_qk_cur; p_qk_cur = p_qk_cur->P_Next) {
											if(p_qk_cur->Text.IsEqiAscii("id")) {
											}
											else if(p_qk_cur->Text.IsEqiAscii("nm")) {
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
										for(const SJson * p_g_cur = p_js_gl->P_Child; p_g_cur; p_g_cur = p_g_cur->P_Next) {
											if(p_g_cur->Text.IsEqiAscii("id")) {
											}
											else if(p_g_cur->Text.IsEqiAscii("nm")) {
											}
											else if(p_g_cur->Text.IsEqiAscii("quot_list")) {
												if(SJson::IsArray(p_g_cur->P_Child)) {
													for(const SJson * p_quot_cur = p_g_cur->P_Child; p_quot_cur; p_quot_cur = p_quot_cur->P_Next) {
														if(SJson::IsObject(p_quot_cur->P_Child)) {
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
		TSCollection <QuotKindEntry> QkList;
		TSCollection <GoodsEntry> GoodsList;
	};
	struct DTSess {
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
			syncdataTest,            // DTest    Тестовый блок данных для отладки взаимодействия с сервером 
			syncdataPrc,             // DPrc
			syncdataAccount,         // DAccount
			syncdataPrices,          // DPrices
			syncdataTSess,           // DTSess
			syncdataJobSrvConnStatus // int статус соединения с сервером
		};
		//
		// Descr: Элемент состояния, который получен от сервера.
		//   Кроме собственно данных содержит блокировку, время актуализации и время истечения срока действия.
		//
		template <class T> class SyncEntry {
		public:
			SyncEntry(int syncDataId) : SyncDataId(syncDataId), TmActual(0), TmExpiry(0)
			{
			}
			void SetData(const T & rData)
			{
				Lck.Lock();
				Data = rData;
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
		SyncEntry <DPrc>     D_Prc;
		SyncEntry <DAccount> D_Acc;
		SyncEntry <DTest>    D_Test;
		SyncEntry <DPrices>  D_Prices;
		SyncEntry <DTSess>   D_TSess;
		SyncEntry <int> D_ConnStatus;

		State() : D_Prc(syncdataPrc), D_Test(syncdataTest), D_Acc(syncdataAccount), D_Prices(syncdataPrices), D_TSess(syncdataTSess), D_ConnStatus(syncdataJobSrvConnStatus)
		{
		}
		int    SetupSyncUpdateTime(int syncDataId, uint msecToUpdate)
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
		int    CheckSyncUpdateTimers(LongArray & rList) const
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
		int    SetSyncUpdateTimerLastReqTime(int syncDataId)
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
	};
private:
	//
	// Descr: Поток, реализующий запросы к серверу.
	//   Так как концепция imgui предполагает перманентную отрисовку сетевые запросы придется //
	//   делать строго асинхронно.
	//
	class WsCtl_CliSession : public PPThread {
	public:
		WsCtl_CliSession(const JobSrvParam & rJsP, WsCtl_ImGuiSceneBlock::State * pSt, WsCtlReqQueue * pQ) : PPThread(PPThread::kWsCtl, 0, 0), JsP(rJsP), P_St(pSt), P_Queue(pQ)
		{
			InitStartupSignal();
		}
		virtual void Run()
		{
			SString temp_buf;
			Evnt   stop_event(SLS.GetStopEventName(temp_buf), Evnt::modeOpen);
			const  uint32 wait_timeout = 500;

			PPJobSrvClient cli;
			PPJobSrvReply reply;
			int srv_conn_r = Connect(cli);
			P_St->D_ConnStatus.SetData(srv_conn_r);
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
							srv_conn_r = cli.Connect(JsP.Addr, JsP.Port);							
							P_St->D_ConnStatus.SetData(srv_conn_r);
						}
						if(srv_conn_r > 0) {
							
						}
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
	private:
		virtual void Startup()
		{
			PPThread::Startup();
			SignalStartup();
		}
		int    Connect(PPJobSrvClient & rCli)
		{
			int    ok = 1;
			THROW(rCli.Connect(JsP.Addr, JsP.Port));
			if(JsP.DbSymb.NotEmpty() && JsP.User.NotEmpty()) {
				THROW(rCli.Login(JsP.DbSymb, JsP.User, JsP.Password));
			}
			CATCHZOK
			return ok;
		}
		void   SendRequest(PPJobSrvClient & rCli, const WsCtlReqQueue::Req & rReq)
		{
			PPJobSrvReply reply;
			switch(rReq.Cmd) {
				case PPSCMD_HELLO:
					if(P_St) {
						WsCtl_ImGuiSceneBlock::DTest st_data;
						SString temp_buf;
						if(rCli.Exec("HELLO", reply) && reply.StartReading(&temp_buf)) {
							if(reply.CheckRepError()) {
								(st_data.Reply = "OK").CatDiv(':', 2).Cat(temp_buf);
							}
							else {
								(st_data.Reply = "ERR").CatDiv(':', 2).Cat(temp_buf);
							}
							P_St->D_Test.SetData(st_data);
						}
						else {
							st_data.Reply = "Send-Request-To-Server-Error";
							P_St->D_Test.SetData(st_data);
						}
					}
					break;
				case PPSCMD_WSCTL_INIT:
					if(P_St) {
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
									WsCtl_ImGuiSceneBlock::DPrc st_data;
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
									P_St->D_Prc.SetData(st_data);
								}
								else {
									; // @err
								}
							}
							else {
								; // @err
							}
						}
						else {
							; // @err
						}
					}
					break;
				case PPSCMD_WSCTL_GETQUOTLIST:
					if(P_St) {
						WsCtl_ImGuiSceneBlock::DPrc st_data;
						P_St->D_Prc.GetData(st_data);
						if(st_data.PrcID) {
							PPJobSrvCmd cmd;
							cmd.StartWriting(PPSCMD_WSCTL_GETQUOTLIST);
							cmd.Write(&st_data.PrcID, sizeof(st_data.PrcID));
							cmd.FinishWriting();
							if(rCli.Exec(cmd, reply)) {
								SString reply_buf;
								reply.StartReading(&reply_buf);
								if(reply.CheckRepError()) {
									WsCtl_ImGuiSceneBlock::DPrices st_data_prices;
									SJson * p_js = SJson::Parse(reply_buf);
									st_data_prices.FromJsonObject(p_js);
								}
							}
						}
					}
					break;
			}
		}
		// Указатель на состояние блока управления панелью. При получении ответа от сервера
		// наш поток будет вносить изменения в это состояние (защита блокировками подразумевается).
		WsCtl_ImGuiSceneBlock::State * P_St; // @notowned
		WsCtlReqQueue * P_Queue; // @notowned
		JobSrvParam JsP;
	};
	bool   ShowDemoWindow; // @sobolev true-->false
	bool   ShowAnotherWindow;
	ImVec4 ClearColor;
	SUiLayout Lo01; // = new SUiLayout(SUiLayoutParam(DIREC_VERT, 0, SUiLayoutParam::alignStretch));
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
	static int CbInput(ImGuiInputTextCallbackData * pInputData)
	{
		return 0;
	}
	//
	char   TestInput[128];
public:
	WsCtl_ImGuiSceneBlock() : ShowDemoWindow(false), ShowAnotherWindow(false), ClearColor(0.45f, 0.55f, 0.60f, 1.00f), P_CmdQ(0)
	{
		TestInput[0] = 0;
		MakeLayout();
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
			P_CmdQ = new WsCtlReqQueue;
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
	void BuildScene()
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
						FRect r = p_lo->GetFrameAdjustedToParent();
						SPoint2F s = r.GetSize();
						ImVec2 lu(r.a.x, r.a.y);
						ImVec2 sz(s.x, s.y);
						ImGui::SetNextWindowPos(lu);
						ImGui::SetNextWindowSize(sz);
						ImGui::Begin("CTL-01", 0, ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove);
						ImGui::Text("CTL-01");
						ImGui::InputText("Кое-что по-русски", TestInput, sizeof(TestInput), 0, CbInput, this);
						ImGui::End();
					}
				}
				{
					const SUiLayout * p_lo = Lo01.FindByID(loidCtl02);
					if(p_lo) {
						FRect r = p_lo->GetFrameAdjustedToParent();
						SPoint2F s = r.GetSize();
						ImVec2 lu(r.a.x, r.a.y);
						ImVec2 sz(s.x, s.y);
						ImGui::SetNextWindowPos(lu);
						ImGui::SetNextWindowSize(sz);
						ImGui::Begin("CTL-02", 0, ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove);
						ImGui::Text("CTL-02");
						
						ImGui::Text(SLS.AcquireRvlStr().Cat("Сервер").CatDiv(':', 2).Cat(JsP.Addr).CatChar(':').Cat(JsP.Port));
						{
							int conn_status = 0;
							St.D_ConnStatus.GetData(conn_status);
							ImGui::Text(SLS.AcquireRvlStr().Cat("Connection status").CatDiv(':', 2).Cat(conn_status));
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
						ImGui::End();
					}
				}
				{
					const SUiLayout * p_lo = Lo01.FindByID(loidAdv01);
					if(p_lo) {
						FRect r = p_lo->GetFrameAdjustedToParent();
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
						FRect r = p_lo->GetFrameAdjustedToParent();
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
						FRect r = p_lo->GetFrameAdjustedToParent();
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
		//assert(font != nullptr);
		// Our state
		//bool show_demo_window = false; // @sobolev true-->false
		//bool show_another_window = false;
		//ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
		WsCtl_ImGuiSceneBlock scene_blk;
		scene_blk.Init();
		{
			static const ImWchar ranges[] = {
				0x0020, 0x00FF, // Basic Latin + Latin Supplement
				0x0400, 0x044F, // Cyrillic
				0,
			};
			///Papyrus/Src/Rsrc/Font/imgui/Roboto-Medium.ttf
			//C:/Windows/Fonts/Tahoma.ttf
			ImFont * p_font = io.Fonts->AddFontFromFileTTF("/Papyrus/Src/Rsrc/Font/imgui/Roboto-Medium.ttf", 14.0f, nullptr, ranges);
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
