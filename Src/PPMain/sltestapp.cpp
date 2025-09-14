// SLTESTAPP.CPP
// Copyright (c) A.Sobolev 2023, 2024, 2025
// @codepage UTF-8
// Тестовое приложение для отработки функций запуска и управления системными процессами
//
#include <pp.h>
#include <wsctl.h>

const char * P_NamedPipeName = "\\\\.\\pipe\\sltestapp-pipe";

class PipeServer : public SlThread_WithStartupSignal { // @construction
	static constexpr uint32 BufSize = 1024;
public:
	struct AppBlock {
		void * Ptr;
	};
	PipeServer(const char * pPipeName, AppBlock * pAppBlk) : SlThread_WithStartupSignal(), P_AppBlk(pAppBlk)
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
					STempBuffer rd_buf(BufSize);
					STempBuffer wr_buf(BufSize);
					bool do_exit = false;
					SString temp_buf;
					do {
						DWORD rd_size = 0;
						boolint rd_ok = ::ReadFile(H_Pipe, rd_buf, rd_buf.GetSize(), &rd_size, NULL/*not overlapped I/O*/);
						if(rd_ok) {
							temp_buf.Z().CatN(rd_buf.cptr(), rd_size);
							// do make reply
							//memcpy(wr_buf.cptr()
							temp_buf.CopyTo(wr_buf, wr_buf.GetSize());
							DWORD reply_size = temp_buf.Len()+1;
							DWORD wr_size = 0;
							boolint wr_ok = ::WriteFile(H_Pipe, wr_buf, reply_size, &wr_size, NULL/*not overlapped I/O*/);
							if(wr_ok) {
								;
							}
							else {
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
		for(bool do_exit = false; !do_exit;) {
			SIntHandle h_pipe = ::CreateNamedPipeA(P_NamedPipeName, PIPE_ACCESS_DUPLEX, PIPE_TYPE_MESSAGE/*message type pipe*/|PIPE_READMODE_MESSAGE |   // message-read mode 
			  PIPE_WAIT/*blocking mode*/, PIPE_UNLIMITED_INSTANCES/*max. instances*/, BufSize/*output buffer size*/, BufSize/*input buffer size*/,
			  0/*client time-out*/, NULL/*default security attribute*/);                    
			if(!h_pipe) {
				// @todo @err
				do_exit = true;
			}
			else {
				if(::ConnectNamedPipe(h_pipe, 0)) {
					_call_count++;
					// do run PipeSession
					PipeSession * p_sess = new PipeSession(h_pipe, P_AppBlk);
					if(p_sess)
						p_sess->Start(true);
				}
				else {
					// @todo @err
					do_exit = true;
				}
			}
		}
	}
private:
	SString PipeName;
	AppBlock * P_AppBlk;
};

class PipeClient : public SlThread_WithStartupSignal {
public:
	PipeClient(const char * pPipeName) : SlThread_WithStartupSignal(), PipeName(pPipeName)
	{
	}
	~PipeClient()
	{
	}
	virtual void Run()
	{
		SIntHandle h_pipe;
		bool do_exit = false;
		do {
			h_pipe = ::CreateFileA(PipeName, GENERIC_READ|GENERIC_WRITE, 0/*no sharing*/,
				NULL/*default security attributes*/, OPEN_EXISTING/*opens existing pipe*/, 0/*default attributes*/, NULL/*no template file*/);
			if(!h_pipe) {
				if(GetLastError() == ERROR_PIPE_BUSY) {
					boolint w_ok = WaitNamedPipeA(PipeName, 20000);
					if(!w_ok) {
						do_exit = true;
					}
				}
				else {
					do_exit = true;
				}
			}
		} while(!h_pipe && !do_exit);
		if(!!h_pipe) {
			DWORD pipe_mode = PIPE_READMODE_MESSAGE; 
			boolint _ok = ::SetNamedPipeHandleState(h_pipe, &pipe_mode/*new pipe mode*/, NULL/*don't set maximum bytes*/, NULL/*don't set maximum time*/);
			if(_ok) {
				STempBuffer wr_buf(1024);
				STempBuffer rd_buf(1024);
				for(uint i = 0; i < 100000; i++) {
					//slfprintf_stderr("Call on named-pipe #%u\n", i+1);
					SString message("Hello, Named Pipe!");
					SString reply;
					DWORD wr_size = 0;
					boolint wr_ok = WriteFile(h_pipe, message.cptr(), message.Len()+1, &wr_size, NULL/*not overlapped*/);
					if(wr_ok) {
						reply.Z();
						bool more_data = false;
						do {
							more_data = false;
							DWORD rd_size = 0;
							rd_buf[0] = 0;
							boolint rd_ok = ReadFile(h_pipe, rd_buf, rd_buf.GetSize(), &rd_size, NULL/*not overlapped*/);
							if(rd_ok) {
								reply.CatN(rd_buf, rd_size);
							}
							else if(GetLastError() == ERROR_MORE_DATA) {
								more_data = true;
							}
							else {
								; // real error
							}
						} while(more_data);
						if(reply == message) {
							//slfprintf_stderr("Success!\n");
							; // ok
						}
					}
				}
			}
		}
	}
private:
	SString PipeName;
};

int main(int argc, char * argv[], char * envp[])
{
	int    result = 0;
	SLS.Init("SlTestApp", 0);
	SIntHandle h_pipe;
	SString temp_buf;
	SStringU temp_buf_u;
	SString out_buf;
	SString policypath;
	SString report_file_name;
	WsCtl_ClientPolicy policy;
	PPGetFilePath(PPPATH_BIN, "sltestapp-report.txt", report_file_name);
	SFile f_rep(report_file_name, SFile::mWrite);
	(out_buf = "SlTestApp: тестовое приложение").Transf(CTRANSF_UTF8_TO_INNER).CR();
	slfprintf_stderr(out_buf);
	if(argc == 1) {
		slfprintf_stderr("There aren't cmdline args\n");
	}
	else {
		out_buf.Z().Cat("Cmdline args").Space().CatParStr(argc).Colon().CR();
		slfprintf_stderr(out_buf);
		for(int i = 1; i < argc; i++) {
			//temp_buf.CopyUtf8FromUnicode(argv[i], sstrlen(argv[i]), 0);
			temp_buf = argv[i];
			temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
			out_buf.Z().Tab().Cat(temp_buf).CR();
			slfprintf_stderr(out_buf);
			if(temp_buf.IsEqiAscii("policypath") && (i+1) < argc) {
				policypath = argv[++i];
			}
		}
	}
	{
		//wchar_t user_name[128];
		//DWORD user_name_buf_len = SIZEOFARRAY(user_name);
		if(SSystem::GetUserName_(temp_buf)) {
		//if(::GetUserNameW(user_name, &user_name_buf_len)) {
			//temp_buf.CopyUtf8FromUnicode(user_name, sstrlen(user_name), 0);
			temp_buf.Transf(CTRANSF_UTF8_TO_INNER);			
			out_buf.Z().Cat("Current User").CatDiv(':', 2).Cat(temp_buf).CR();
			slfprintf_stderr(out_buf);
		}
	}
	{
		wchar_t curdir_u[1024];
		::GetCurrentDirectoryW(sizeof(curdir_u), curdir_u);
		temp_buf.CopyUtf8FromUnicode(curdir_u, sstrlen(curdir_u), 0);
		temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
		out_buf.Z().Cat("Current Directory").CatDiv(':', 2).Cat(temp_buf).CR();
		slfprintf_stderr(out_buf);
	}
	//
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
			PipeServer * p_psrv = new PipeServer(P_NamedPipeName, p_app_blk);
			if(p_psrv)
				p_psrv->Start(true);
		}
		SDelay(500);
		{ // pipe client
			PipeClient * p_pcli = new PipeClient(P_NamedPipeName);
			if(p_pcli)
				p_pcli->Start(true);
		}
	}
	//
	if(!f_rep.IsValid()) {
		out_buf.Z().Cat("Report file").Space().CatParStr(report_file_name).Space().Cat("opening fault").CR();
		slfprintf_stderr(out_buf);
	}
	if(policypath.IsEmpty()) {
		policypath = "\\Papyrus\\Src\\PPTEST\\DATA";
	}
	if(policypath.NotEmpty()) {
		out_buf.Z().Cat("policy path").CatDiv(':', 2).Cat(policypath).CR();
		slfprintf_stderr(out_buf);
		//
		//winpolicy.json 
		temp_buf.Z().Cat(policypath).SetLastSlash().Cat(/*"winpolicy.json"*/"test-file-to-write");
		SFile f_policy(temp_buf, SFile::mWrite);
		if(f_policy.IsValid()) {
			out_buf.Z().Cat("File").Space().Cat(temp_buf).Space().Cat("opened successfully");
		}
		else {
			out_buf.Z().Cat("File").Space().Cat(temp_buf).Space().Cat("opening fault");
		}
		slfprintf_stderr(out_buf.CR());
	}
	{
		uint8 random_buffer[32];
		SLS.GetTLA().Rg.ObfuscateBuffer(random_buffer, sizeof(random_buffer));
		SString random_string;
		random_string.EncodeMime64(random_buffer, sizeof(random_buffer));
		{
			WinRegKey test_key(HKEY_CURRENT_USER, PPConst::WrKey_SlTestApp, 0);
			temp_buf.Z().Cat("CURRENT_USER").SetLastSlash().Cat(PPConst::WrKey_SlTestApp);
			if(test_key.IsValid()) {
				slfprintf_stderr((out_buf = ":) Test register key").Space().CatParStr(temp_buf).Space().Cat("is opened for writing successfully").CR());
				if(test_key.PutString("test-string", random_string)) {
					slfprintf_stderr((out_buf = ":) Writing to the test register key").Space().CatParStr(temp_buf).Space().Cat("is successful").CR());
				}
				else {
					slfprintf_stderr((out_buf = ":( Writing to the test register key").Space().CatParStr(temp_buf).Space().Cat("is fault").CR());
				}
			}
			else {
				slfprintf_stderr((out_buf = ":( Test register key").Space().CatParStr(temp_buf).Space().Cat("isn't opened for writing").CR());
			}
		}
		{
			WinRegKey test_key(HKEY_LOCAL_MACHINE, PPConst::WrKey_SlTestApp, 0);
			temp_buf.Z().Cat("MACHINE").SetLastSlash().Cat(PPConst::WrKey_SlTestApp);
			if(test_key.IsValid()) {
				slfprintf_stderr((out_buf = ":) Test register key").Space().CatParStr(temp_buf).Space().Cat("is opened for writing successfully").CR());
				if(test_key.PutString("test-string", random_string)) {
					slfprintf_stderr((out_buf = ":) Writing to the test register key").Space().CatParStr(temp_buf).Space().Cat("is successful").CR());
				}
				else {
					slfprintf_stderr((out_buf = ":( Writing to the test register key").Space().CatParStr(temp_buf).Space().Cat("is fault").CR());
				}
			}
			else {
				slfprintf_stderr((out_buf = ":( Test register key").Space().CatParStr(temp_buf).Space().Cat("isn't opened for writing").CR());
			}
		}
	}
	{
		//GetEnvironmentStrings()
		if(f_rep.IsValid()) {
			{
				f_rep.WriteLine((temp_buf = "Environment").CR());
				if(envp) {
					for(uint envidx = 0; envp[envidx]; envidx++) {
						temp_buf.Z().Tab().Cat(envp[envidx]).CR();
						f_rep.WriteLine(temp_buf);
					}
				}
			}
			{
				f_rep.WriteLine((temp_buf = "Known folders").CR());
				TSCollection <SKnownFolderEntry> known_folder_list;
				GetKnownFolderList(known_folder_list);
				for(uint i = 0; i < known_folder_list.getCount(); i++) {
					const SKnownFolderEntry * p_entry = known_folder_list.at(i);
					if(p_entry) {
						temp_buf.Z().CatHex(p_entry->Ued).Space().Cat(p_entry->Guid, S_GUID::fmtIDL).Space().Cat(p_entry->Result).Space().Cat(p_entry->PathUtf8).CR();
						f_rep.WriteLine(temp_buf);
					}
				}
			}
		}
	}
	slfprintf_stderr("Press [Enter] to finish...\n");
	getchar();
	return result;
}
