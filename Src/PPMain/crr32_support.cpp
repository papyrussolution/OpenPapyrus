// crr32_support.cpp
// Copyright (c) A.Sobolev 2023, 2024
// Модуль для обслуживания вызовов к 32-разрядной библиотеке Crystal Reports
// @codepage UTF-8
//
#include <pp.h>
#pragma hdrstop
#include <crpe.h>
//
// @construction
// 

//
// Descr: Класс сервера, коммуницирующего с клиентом по именованному каналу (NamedPipe)
//
class PipeServer : public SlThread_WithStartupSignal { // @construction
	static constexpr uint32 BufSize = 1024;
public:
	struct AppBlock {
		void * Ptr;
	};
	PipeServer(AppBlock * pAppBlk) : SlThread_WithStartupSignal(), P_AppBlk(pAppBlk), DebugMark(false)
	{
	}
	~PipeServer()
	{
	}
	virtual void Run()
	{
		class PipeSession : public SlThread_WithStartupSignal {
		public:
			PipeSession(SIntHandle hPipe, AppBlock * pAppBlk) : SlThread_WithStartupSignal(), H_Pipe(hPipe), P_AppBlk(pAppBlk)
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
					bool do_exit = false;
					SString temp_buf;
					SString cmd_buf;
					do {
						DWORD rd_size = 0;
						boolint rd_ok = ::ReadFile(H_Pipe, rd_buf, rd_buf.GetSize(), &rd_size, NULL/*not overlapped I/O*/);
						if(rd_ok) {
							temp_buf.Z().CatN(rd_buf.cptr(), rd_size);
							printf(msg_buf.Z().Cat("Q").CatDiv(':', 2).Cat(temp_buf).CR().cptr()); // @debug
							SJson * p_js_query = SJson::Parse(temp_buf);
							if(p_js_query) {
								cmd_buf.Z();
								const SJson * p_c = 0;
								p_c = p_js_query->FindChildByKey("cmd");
								if(SJson::IsString(p_c)) {
									(cmd_buf = p_c->Text).Unescape();
								}

								// do make reply
								//memcpy(wr_buf.cptr()
								SJson js_reply(SJson::tOBJECT);
								js_reply.InsertString("status", "ok");

								(temp_buf = "I have got your message").CatDiv(':', 2).Cat(cmd_buf);
								js_reply.InsertString("info", temp_buf);
								//
								js_reply.ToStr(temp_buf);
								temp_buf.CopyTo(wr_buf, wr_buf.GetSize());
								DWORD reply_size = temp_buf.Len()+1;
								DWORD wr_size = 0;
								boolint wr_ok = ::WriteFile(H_Pipe, wr_buf, reply_size, &wr_size, NULL/*not overlapped I/O*/);
								if(wr_ok) {
									printf(msg_buf.Z().Cat("R").CatDiv(':', 2).Cat(temp_buf).CR().cptr()); // @debug
									;
								}
								else {
								}
								if(cmd_buf.IsEqiAscii("quit")) {
									do_exit = true;
								}
							}
						}
						else {
							if(GetLastError() == ERROR_BROKEN_PIPE) {
								;
							}
							else {
								;
							}
						}
					} while(!do_exit);
					FlushFileBuffers(H_Pipe);
					DisconnectNamedPipe(H_Pipe); 
					CloseHandle(H_Pipe); 
					H_Pipe.Z();
				}
			}
		private:
			SIntHandle H_Pipe;
			AppBlock * P_AppBlk;
		};
		uint _call_count = 0;
		DWORD last_werr = 0;
		SString pipe_name;
		GetCrr32ProxiPipeName(pipe_name);
		for(bool do_exit = false; !do_exit;) {
			SIntHandle h_pipe = ::CreateNamedPipeA(pipe_name, PIPE_ACCESS_DUPLEX, PIPE_TYPE_MESSAGE/*message type pipe*/|PIPE_READMODE_MESSAGE |   // message-read mode 
			  PIPE_WAIT/*blocking mode*/, PIPE_UNLIMITED_INSTANCES/*max. instances*/, BufSize/*output buffer size*/, BufSize/*input buffer size*/,
			  0/*client time-out*/, NULL/*default security attribute*/);                    
			if(!h_pipe) {
				// @todo @err
				do_exit = true;
			}
			else {
				const BOOL cnpr = ::ConnectNamedPipe(h_pipe, 0);
				if(cnpr) {
					DebugMark = false;
					_call_count++;
					// do run PipeSession
					PipeSession * p_sess = new PipeSession(h_pipe, P_AppBlk);
					if(p_sess)
						p_sess->Start(1);
				}
				else {
					DebugMark = true;
					// @todo @err
					last_werr = ::GetLastError();
					do_exit = true;
				}
			}
		}
	}
private:
	AppBlock * P_AppBlk;
	bool   DebugMark;
};

int main(int argc, char ** argv)
{
	int    result = 0;
	SLS.Init("crr32-support-server", 0);
	{
		{ // pipe server
			struct AppBlock : public PipeServer::AppBlock {
				AppBlock() : PipeServer::AppBlock()
				{
					memzero(Dummy, sizeof(Dummy));
				}
				uint8 Dummy[64];
			};
			AppBlock * p_app_blk = new AppBlock();
			PipeServer * p_psrv = new PipeServer(p_app_blk);
			if(p_psrv) {
				p_psrv->Start(1);
				::WaitForSingleObject(*p_psrv, INFINITE);
			}
		}
		//SDelay(500);
		/*{ // pipe client
			PipeClient * p_pcli = new PipeClient(P_NamedPipeName);
			if(p_pcli)
				p_pcli->Start(1);
		}*/
	}
	return result;
}
