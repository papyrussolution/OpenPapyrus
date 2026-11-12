// ORTOOLS_MODULE_CLIENT.CPP
// Copyright (c) A.Sobolev 2026
// @codepage UTF-8
// Модуль, реализующий работу с or-tools через "прокладку" ortoos_support через IPC (named-pipe)
//
#include <pp.h>
#pragma hdrstop
//
// Note: Практически, дословно повторяет class Crr32SupportClient (ppreport.cpp)
//
class OrToolsSupportClient {
public:
	OrToolsSupportClient(const char * pPipeNameUtf8) : BusyPipeTimeoutMs(20000), WrBuf(SKILOBYTE(4)), RdBuf(SKILOBYTE(4))
	{
		PipeNameU.CopyFromUtf8Strict(pPipeNameUtf8, sstrlen(pPipeNameUtf8));
	}
	SIntHandle Connect()
	{
		SIntHandle result;
		if(PipeNameU.NotEmpty()) {
			bool do_exit = false;
			do {
				result = ::CreateFileW(PipeNameU, GENERIC_READ|GENERIC_WRITE, 0/*no sharing*/,
					NULL/*default security attributes*/, OPEN_EXISTING/*opens existing pipe*/, 0/*default attributes*/, NULL/*no template file*/);
				if(!result) {
					if(GetLastError() == ERROR_PIPE_BUSY) {
						boolint w_ok = ::WaitNamedPipeW(PipeNameU, BusyPipeTimeoutMs);
						if(!w_ok) {
							do_exit = true;
						}
					}
					else {
						do_exit = true;
					}
				}
			} while(!result && !do_exit);
			if(!!result) {
				DWORD pipe_mode = PIPE_READMODE_MESSAGE; 
				boolint _ok = ::SetNamedPipeHandleState(result, &pipe_mode/*new pipe mode*/, NULL/*don't set maximum bytes*/, NULL/*don't set maximum time*/);
				if(!_ok) {
					::CloseHandle(result);
					result.Z();
				}
					
			}
		}
		return result;
	}
	bool   SendQuitCommand(SIntHandle hPipe)
	{
		bool   ok = false;
		SJson * p_js_reply = SendCommand(hPipe, -1, "quit", 0);
		if(p_js_reply) {
			delete p_js_reply;
			ok = true;
		}
		return ok;
	}
	SJson * SendCommand(SIntHandle hPipe, int waitTimeout, const char * pCmdUtf8, const char * pParam)
	{
		SJson * p_js_reply = 0;
		SString temp_buf;
		SString reply_buf;
		THROW(hPipe); // @todo @err
		THROW(!isempty(pCmdUtf8)); // @todo @err
		{
			SJson js_query(SJson::tOBJECT);
			js_query.InsertString("cmd", pCmdUtf8);
			if(!isempty(pParam)) {
				js_query.InsertString("param", pParam);
			}
			if(sstreqi_ascii(pCmdUtf8, "run")) {
				js_query.InsertBool("quitafter", true);
			}
			//
			DWORD wr_size = 0;
			js_query.ToStr(temp_buf);
			boolint wr_ok = ::WriteFile(hPipe, temp_buf.cptr(), temp_buf.Len32()+1, &wr_size, NULL/*not overlapped*/);
			if(wr_ok) {
				reply_buf.Z();
				bool more_data = false;
				do {
					more_data = false;
					union {;
						DWORD rd_size;
						size_t actual_size;
					} rs;
					rs.actual_size = 0;
					RdBuf[0] = 0;
					int rd_result = SFile::AsyncReadFileWithTimeout(hPipe, RdBuf, RdBuf.GetSize(), waitTimeout, &rs.actual_size);
					if(rd_result > 0) {
						reply_buf.CatN(RdBuf, rs.actual_size);
					}
					else if(rd_result < 0) {
						p_js_reply = new SJson(SJson::tOBJECT);
						p_js_reply->InsertString("result", "timeout");
					}
					else if(GetLastError() == ERROR_MORE_DATA) {
						more_data = true;
					}
					else {
						; // real error
					}
				} while(more_data);
				if(!p_js_reply)
					p_js_reply = SJson::Parse(reply_buf);
			}
		}
		CATCH
			ZDELETE(p_js_reply);
		ENDCATCH
		return p_js_reply;
	}
private:
	SStringU PipeNameU;
	uint   BusyPipeTimeoutMs;
	STempBuffer WrBuf;
	STempBuffer RdBuf;
};

/*static*/PPOptiModule::BaseExecutionResult * PPOptiModule::Helper_ClientExecution_Task(SIntHandle hPipe, BaseExecutionBlock & rEB)
{
	const  int pipe_wait_timeout = 120000; // ms
	PPOptiModule::BaseExecutionResult * p_result = 0;
	SJson * p_js_reply = 0;
	SString temp_buf;
	SString reply_buf;
	if(!!hPipe) {
		bool   do_process = false;
		SBuffer eb_buf;
		SSerializeContext sctx;
		SJson js_query(SJson::tOBJECT);
		switch(rEB.GetTask()) {
			case taskCuttingStock2D:
				{
					if(rEB.SerializeV(+1, eb_buf, &sctx)) {
						js_query.InsertInt("cmd", taskCuttingStock2D);
						temp_buf.Z().EncodeMime64(eb_buf.constptr(), eb_buf.GetAvailableSize());
						js_query.InsertString("param", temp_buf);
						p_result = new CuttingStock2D::Result();
						do_process = true;
					}
				}
				break;
			default:
				break;
		}
		if(do_process) {
			//result = InternalBlock::SendPrintCommandToServer(cli, h_pipe, rBlk, rReply);

			DWORD wr_size = 0;
			js_query.ToStr(temp_buf);
			boolint wr_ok = ::WriteFile(hPipe, temp_buf.cptr(), temp_buf.Len()+1, &wr_size, NULL/*not overlapped*/);
			if(wr_ok) {
				
				bool more_data = false;
				STempBuffer rd_buf(SKILOBYTE(512));
				do {
					more_data = false;
					union {;
						DWORD rd_size;
						size_t actual_size;
					} rs;
					rs.actual_size = 0;
					rd_buf[0] = 0;
					int rd_result = SFile::AsyncReadFileWithTimeout(hPipe, rd_buf, rd_buf.GetSize(), pipe_wait_timeout, &rs.actual_size);
					if(rd_result > 0) {
						reply_buf.CatN(rd_buf, rs.actual_size);
					}
					else if(rd_result < 0) {
						p_js_reply = new SJson(SJson::tOBJECT);
						p_js_reply->InsertString("result", "timeout");
					}
					else if(GetLastError() == ERROR_MORE_DATA) {
						more_data = true;
					}
					else {
						; // real error
					}
				} while(more_data);
				if(!p_js_reply) {
					p_js_reply = SJson::Parse(reply_buf);
				}
				if(SJson::IsObject(p_js_reply)) {
					for(const SJson * p_cur = p_js_reply->P_Child; p_cur; p_cur = p_cur->P_Next) {
						if(p_cur->Text.IsEqiAscii("status")) {
						}
						else if(p_cur->Text.IsEqiAscii("info")) {
						}
						else if(p_cur->Text.IsEqiAscii("result")) {
							SBuffer sbuf;
							if(sbuf.AssignMime64(p_cur->P_Child->Text)) {
								if(!p_result) {
									p_result = new CuttingStock2D::Result();
								}
								p_result->SerializeV(-1, sbuf, &sctx);
							}
						}
					}
				}
			}
		}
	}
	delete p_js_reply;
	return p_result;
}

/*static*/PPOptiModule::BaseExecutionResult * PPOptiModule::ClientExecution(BaseExecutionBlock & rEB)
{
	PPOptiModule::BaseExecutionResult * p_result = 0;
	int    ok = -1;
	int    result = 0;
	const  char * p_ext_process_name = "ortools_support.exe";
	SJson * p_js_reply = 0;
	bool   is_new_process = false;
	SString temp_buf;
	SString pipe_name;
	SString module_path;
	SString reply_buf;
	PPGetPath(PPPATH_BIN, module_path);
	module_path.SetLastSlash().Cat("or-tools").SetLastSlash().Cat(p_ext_process_name);
	GetORToolsProxiPipeName(pipe_name);
	uint   prc_id = SlProcess::SearchProcessByName(p_ext_process_name);
	if(prc_id) {
		// Пытаемся отправить команду ping для того, чтоб удостовериться, что существует
		// общий процесс для обработки не-интерактивных команд печати
		OrToolsSupportClient cli(pipe_name);
		SIntHandle h_pipe(cli.Connect());
		if(!!h_pipe) {
			p_js_reply = cli.SendCommand(h_pipe, 1000, "ping", 0);
			if(p_js_reply) {
				// ...
				//result = 1; // ???
				p_js_reply->ToStr(temp_buf); // @debug
				ZDELETE(p_js_reply);
				p_result = Helper_ClientExecution_Task(h_pipe, rEB);
			}
			::CloseHandle(h_pipe);
		}
	}
	if(!p_result) {
		if(fileExists(module_path)) {
			SlProcess::Result prc_result;
			SlProcess process;
			process.SetPath(module_path);
			SFsPath ps(module_path);
			ps.Merge(SFsPath::fDrv|SFsPath::fDir, temp_buf);
			process.SetWorkingDir(temp_buf);
			if(process.Run(&prc_result)) {
				SDelay(200);
				is_new_process = true;
				//
				OrToolsSupportClient cli(pipe_name);
				SIntHandle h_pipe(cli.Connect());
				uint  try_to_connect_count = 1;
				if(!h_pipe) {
					for(uint i = 0; !h_pipe && i < 10; i++) {
						SDelay(50);
						h_pipe = cli.Connect();
						try_to_connect_count++;
					}
				}
				if(!h_pipe) {
					;
					// @todo @log
					//do_use_local_func = true;
				}
				else {
					p_js_reply = cli.SendCommand(h_pipe, 1000, "ping", 0);
					if(p_js_reply) {
						// ...
						//result = 1; // ???
						p_js_reply->ToStr(temp_buf); // @debug
						ZDELETE(p_js_reply);
						p_result = Helper_ClientExecution_Task(h_pipe, rEB);
					}
					::CloseHandle(h_pipe);
				}
			}
		}
		else {
			//assert(!do_use_local_func);
			//do_use_local_func = true;
		}
	}
	return p_result;
}

