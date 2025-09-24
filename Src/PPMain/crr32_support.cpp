// crr32_support.cpp
// Copyright (c) A.Sobolev 2023, 2024, 2025
// Модуль для обслуживания вызовов к 32-разрядной библиотеке Crystal Reports
// @codepage UTF-8
//
#include <pp.h>
#pragma hdrstop
#include <crpe.h>
/*
	Этот сервер разрабатывается для того, чтобы вынести 32-разрядный Crystal Reports за пределы основных процессов Papyrus,
	дабы Papyrus можно было бы сделать 64-разрядным.

	Существует 2 различных режима использования этого сервера:
		1. Для печати и экспорта. То есть работа без интерактивности (здесь есть один нюанс)
		2. Для предварительного просмотра отчета - в этом случае интерактивность необходима.

	Для не-интерактивного режима запускается единственный экземпляр сервера с универсальным символом именованного канала (named pipe).
	Этот процесс работает сколько угодно и обслуживает сколько угодно процессов на своей машине.
	Универсальный символ named pipe можно получить вызовом функции GetCrr32ProxiPipeName().

	Для интерактивного режима каждый процесс, когда ему необходимо обратиться к функционалу предпросмотра,
	создает новый процесс crr32_support с уникальным символом именованного канала (named pipe), через 
	который клиентский процесс будет общаться с порожденным экземпляром crr32_support.
*/ 
#define CMDID_TO_RUN_PREVIEW_BY_MAIN_THREAD 10101
//
// Descr: Класс сервера, коммуницирующего с клиентом по именованному каналу (NamedPipe)
//
class PipeServer : public SlThread_WithStartupSignal {
	static constexpr uint32 BufSize = 1024;
	//SIntHandle H_Pipe;
	HWND HwParent;
	SString ExternalPipeName;
	//AppBlock * P_AppBlk;
public:
	/*struct AppBlock {
		AppBlock(HWND hwParent, const char * pExternalPipeName) : HwParent(hwParent), ExternalPipeName(pExternalPipeName)
		{
			memzero(Dummy, sizeof(Dummy));
		}
		HWND HwParent;
		SString ExternalPipeName;
		uint8 Dummy[64];
	};*/
	PipeServer(HWND hwParent, const char * pExternalPipeName) : SlThread_WithStartupSignal(), HwParent(hwParent), ExternalPipeName(pExternalPipeName)
	{
	}
	~PipeServer()
	{
	}
	virtual void Run()
	{
		class PipeSession : public PPThread {
			SIntHandle H_Pipe;
			HWND HwParent;
		public:
			PipeSession(SIntHandle hPipe, HWND hwParent) : PPThread(kCrr32Support, 0, 0), H_Pipe(hPipe), HwParent(hwParent)
			{
			}
			~PipeSession()
			{
			}
			virtual void Run()
			{
				if(!!H_Pipe) {
					SString msg_buf;
					STempBuffer rd_buf(BufSize);
					STempBuffer wr_buf(BufSize);
					bool   do_exit = false;
					bool   is_pipe_ended = false;
					SString temp_buf;
					SString cmd_buf;
					if(PEOpenEngine()) {
						do {
							DWORD rd_size = 0;
							const boolint rd_ok = ::ReadFile(H_Pipe, rd_buf, rd_buf.GetSize(), &rd_size, NULL/*not overlapped I/O*/);
							if(rd_ok) {
								temp_buf.Z().CatN(rd_buf.cptr(), rd_size);
								//printf(msg_buf.Z().Cat("Q").CatDiv(':', 2).Cat(temp_buf).CR().cptr()); // @debug
								SJson * p_js_query = SJson::Parse(temp_buf);
								if(p_js_query) {
									bool   do_reply = true;
									bool   rep_status_ok = true;
									SString rep_info_text;
									int    pr_call_state = 0; // 0 - CrystalReportPrint2_Server не была вызвана; 
										// 1 - была вызвана функция CrystalReportPrint2_Server; 2 - вызов перенаправлен для предпросмотра
									CrystalReportPrintReply pr_reply;
									//
									cmd_buf.Z();
									const SJson * p_c = 0;
									p_c = p_js_query->FindChildByKey("cmd");
									if(SJson::IsString(p_c)) {
										(cmd_buf = p_c->Text).Unescape();
									}
									if(cmd_buf.IsEqiAscii("ping")) {
										//js_reply.InsertString("status", "ok");
										//js_reply.InsertString("info", "pong");
										rep_info_text = "pong";
									}
									else if(cmd_buf.IsEqiAscii("run")) {
										SSerializeContext sctx;
										p_c = p_js_query->FindChildByKey("param");
										if(p_c) {
											(temp_buf = p_c->Text).Unescape();
											const bool quit_after = SJson::IsTrue(p_js_query->FindChildByKey("quitafter"));
											SBuffer sbuf;
											if(sbuf.AppendMime64(temp_buf)) {
												CrystalReportPrintParamBlock * p_blk = new CrystalReportPrintParamBlock();
												if(p_blk && p_blk->Serialize(-1, sbuf, &sctx)) {
													if(p_blk->Action == CrystalReportPrintParamBlock::actionPreview) {
														if(HwParent) {
															::PostMessageW(HwParent, WM_COMMAND, CMDID_TO_RUN_PREVIEW_BY_MAIN_THREAD, reinterpret_cast<LPARAM>(p_blk));
															//js_reply.InsertString("status", "ok");
															pr_call_state = 2;
														}
														else {
															ZDELETE(p_blk);
														}
													}
													else {
														const int pr = CrystalReportPrint2_Server(*p_blk, pr_reply, 0/*hParentWindowForPreview*//*, H_Pipe*/);
														pr_call_state = 1;
														if(!pr) {
															rep_status_ok = false;
														}
														// Функция CrystalReportPrint2_Server сама отправила ответ клиенту через H_Pipe, который мы ей передали.
														ZDELETE(p_blk);
														//do_reply = false;
													}
													if(quit_after)
														do_exit = true;
												}
											}
											else {
												; // @todo @err
												//js_reply.InsertString("status", "fail");
												rep_status_ok = false;
											}
										}
										else {
											//js_reply.InsertString("status", "fail");
											rep_status_ok = false;
										}
									}
									else {
										//js_reply.InsertString("status", "ok");
										(rep_info_text = "I have got your message").CatDiv(':', 2).Cat(cmd_buf);
										//js_reply.InsertString("info", temp_buf);
									}
									// do make reply
									//memcpy(wr_buf.cptr()
									if(do_reply) {
										SJson js_reply(SJson::tOBJECT);
										js_reply.InsertString("status", rep_status_ok ? "ok" : "fail");
										if(rep_info_text.NotEmpty()) {
											js_reply.InsertString("info", rep_info_text);
										}
										if(pr_call_state == 1) {
											if(pr_reply.Code != 0) {
												SJson * p_js_pr_reply = pr_reply.ToJsonObj();
												if(p_js_pr_reply)
													js_reply.Insert("err", p_js_pr_reply);
											}
										}
										else if(pr_call_state == 2) {
											if(rep_info_text.IsEmpty()) {
												js_reply.InsertString("info", "Report preview launched");
											}
										}
										js_reply.ToStr(temp_buf);
										temp_buf.CopyTo(wr_buf, wr_buf.GetSize());
										DWORD reply_size = temp_buf.Len()+1;
										DWORD wr_size = 0;
										boolint wr_ok = ::WriteFile(H_Pipe, wr_buf, reply_size, &wr_size, NULL/*not overlapped I/O*/);
										if(wr_ok) {
											//printf(msg_buf.Z().Cat("R").CatDiv(':', 2).Cat(temp_buf).CR().cptr()); // @debug
											;
										}
										else {
										}
									}
									if(cmd_buf.IsEqiAscii("quit")) {
										do_exit = true;
									}
								}
							}
							else {
								if(GetLastError() == ERROR_BROKEN_PIPE) {
									is_pipe_ended = true;
									do_exit = true;
								}
								else {
									;
								}
							}
						} while(!do_exit);
						PECloseEngine();
					}
					if(!is_pipe_ended) {
						FlushFileBuffers(H_Pipe);
						DisconnectNamedPipe(H_Pipe);
						CloseHandle(H_Pipe);
					}
					H_Pipe.Z();
				}
			}
		};

		uint _call_count = 0;
		DWORD last_werr = 0;
		SString pipe_name;
		if(ExternalPipeName.NotEmpty())
			pipe_name = ExternalPipeName;
		else
			GetCrr32ProxiPipeName(pipe_name);
		for(bool do_exit = false; !do_exit;) {
			SIntHandle h_pipe = ::CreateNamedPipeA(pipe_name, PIPE_ACCESS_DUPLEX,
				PIPE_TYPE_MESSAGE /*message type pipe*/|PIPE_READMODE_MESSAGE/*message-read mode*/|PIPE_WAIT/*blocking mode*/,PIPE_UNLIMITED_INSTANCES/*max. instances*/,
				BufSize /*output buffer size*/, BufSize /*input buffer size*/, 0 /*client time-out*/, NULL /*default security attribute*/);
			if(!h_pipe) {
				// @todo @err
				do_exit = true;
			}
			else {
				const BOOL cnpr = ::ConnectNamedPipe(h_pipe, 0);
				if(cnpr) {
					_call_count++;
					PipeSession * p_sess = new PipeSession(h_pipe, HwParent);
					if(p_sess)
						p_sess->Start(false);
				}
				else {
					// @todo @err
					last_werr = ::GetLastError();
					do_exit = true;
				}
			}
		}
	}
};
//
//
//
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	struct ProcessBlock {
		static LRESULT CALLBACK MainWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
		{
			LRESULT result = 0;
			switch(message) {
				case WM_CREATE:
					break;
				case WM_COMMAND:
					if(LOWORD(CMDID_TO_RUN_PREVIEW_BY_MAIN_THREAD)) {
						CrystalReportPrintParamBlock * p_blk = reinterpret_cast<CrystalReportPrintParamBlock *>(lParam);
						if(p_blk) {
							CrystalReportPrintReply reply;
							if(!CrystalReportPrint2_Server(*p_blk, reply, hWnd)) {
								::DestroyWindow(hWnd); // Завершаем процесс
							}
						}
					}
					break;
				case WM_DESTROY:
					::PostQuitMessage(0);
					break;
				default:
					result = DefWindowProc(hWnd, message, wParam, lParam);
					break;
			}
			return result;
		}
		ProcessBlock() : HMainWindow(0)
		{
			INITWINAPISTRUCT(NotifyIconData);
		}
		void ShowTrayIcon()
		{
			if(HMainWindow) {
				INITWINAPISTRUCT(NotifyIconData);
				NotifyIconData.hWnd = HMainWindow;
				NotifyIconData.uID = 1;
				NotifyIconData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
				NotifyIconData.uCallbackMessage = WM_APP + 1;
				NotifyIconData.hIcon = LoadIcon(NULL, IDI_APPLICATION);
				wcscpy_s(NotifyIconData.szTip, L"CRR32_SUPPORT");
				::Shell_NotifyIconW(NIM_ADD, &NotifyIconData);
			}
		}
		void RemoveTrayIcon()
		{
			::Shell_NotifyIconW(NIM_DELETE, &NotifyIconData);
		}
		HWND HMainWindow;
		NOTIFYICONDATAW NotifyIconData;
	};
	int    result = 0;
	if(!PPSession::CheckExecutionLocking()) {
		result = -1;
	}
	else {
		class ExitEventCatcher : public PPThread {
		public:
			ExitEventCatcher(HWND hMainWindow) : PPThread(kCrr32Support, 0, 0), H_MainWindow(hMainWindow)
			{
			}
			virtual void Run()
			{
				SString file_name;
				makeExecPathFileName(PPConst::FnNam_PPLock, 0, file_name);
				do {
					if(fileExists(file_name)) {
						// Завершаем процесс
						/*if(H_MainWindow) {
							::DestroyWindow(H_MainWindow);
						}
						else*/{
							exit(2);
						}
					}
					else {
						SDelay(1000);
					}
				} while(true);
			}
		private:
			HWND   H_MainWindow;
		};
		SString temp_buf;
		SString external_pipe_name;
		ProcessBlock pb;
		const  wchar_t * p_wnd_cls = L"Crr32_Support_MainWindow_Class";
		DS.Init(PPSession::internalappCrr32Support, PPSession::fInitPaths, 0, 0);
		{
			const SString pipe_symb_arg_name("/pipesymb");
			for(int i = 1; i < _argc; i++) {
				temp_buf = _argv[i];
				if(external_pipe_name.IsEmpty() && temp_buf.HasPrefixIAscii(pipe_symb_arg_name)) {
					if(temp_buf.C(pipe_symb_arg_name.Len()) == ':') {
						temp_buf.ShiftLeft(pipe_symb_arg_name.Len()+1);
						if(temp_buf.NotEmptyS()) {
							SFile::MakeNamedPipeName(temp_buf, external_pipe_name);
						}
					}
				}
			}
		}
		if(external_pipe_name.NotEmpty()) { // Специальное имя канала применяется для одноразового процесса предварительного просмотра
			if(!PEOpenEngine()) {
				result = -1;
			}
			else {
				WNDCLASSEX wc;
				INITWINAPISTRUCT(wc);
				wc.lpfnWndProc = ProcessBlock::MainWndProc;
				wc.hInstance = hInstance;
				wc.lpszClassName = p_wnd_cls;
				if(!::RegisterClassExW(&wc))
					result = -1;
				else {
					// Создаем невидимое главное окно
					pb.HMainWindow = ::CreateWindowExW(0, p_wnd_cls, L"crr32_support_main_windows", WS_OVERLAPPEDWINDOW,
						CW_USEDEFAULT, CW_USEDEFAULT, 10, 10, NULL, NULL, hInstance, NULL);
					if(!pb.HMainWindow)
						result = -1;
					else {
						// Показываем иконку в системном трее
						pb.ShowTrayIcon();
						{
							ExitEventCatcher * p_eec = new ExitEventCatcher(pb.HMainWindow);
							if(p_eec)
								p_eec->Start(false);
						}
						{
							PipeServer * p_psrv = new PipeServer(pb.HMainWindow, external_pipe_name);
							if(p_psrv) {
								p_psrv->Start(/*true*/false);
								//::WaitForSingleObject(*p_psrv, INFINITE);
							}
						}
						{
							MSG msg;
							while(::GetMessageW(&msg, NULL, 0, 0)) {
								::TranslateMessage(&msg);
								::DispatchMessageW(&msg);
							}
							result = (int)msg.wParam;
						}
						pb.RemoveTrayIcon();
					}
				}
				PECloseEngine();
			}
		}
		else {
			{
				ExitEventCatcher * p_eec = new ExitEventCatcher(0);
				if(p_eec)
					p_eec->Start(false);
			}
			PipeServer * p_psrv = new PipeServer(0, 0);
			if(p_psrv) {
				p_psrv->Start(true);
				::WaitForSingleObject(*p_psrv, INFINITE);
			}
		}
	}
	return result;
}
