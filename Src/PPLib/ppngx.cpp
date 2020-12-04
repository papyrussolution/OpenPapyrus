// PPNGX.CPP
// Copyright (c) A.Sobolev 2017, 2018, 2019, 2020
// @codepage UTF-8
// Взаимодействие с NGINX
//
#include <pp.h>
#pragma hdrstop
#include <ngx_core.h>

int NgxStartUp(const NgxStartUpOptions & rO); // prototype
ngx_thread_value_t __stdcall ngx_worker_thread(void * data);

int RunNginxServer()
{
	NgxStartUpOptions o;
	SString temp_buf;
	SLS.QueryPath("workspace", temp_buf);
	if(temp_buf.NotEmpty()) {
		temp_buf.SetLastDSlash().Cat("nginx").SetLastDSlash();
		SPathStruc::NormalizePath(temp_buf, SPathStruc::npfSlash, o.Prefix);
	}
	return BIN(NgxStartUp(o) == 0);
}

int RunNginxWorker()
{
	class NgxWorkerThread : public PPThread {
	public:
		NgxWorkerThread() : PPThread(kNginxWorker, 0, 0)
		{
		}
	private:
		virtual void Run()
		{
			ngx_worker_thread(0);
		}
	};
	NgxWorkerThread * p_thread = new NgxWorkerThread();
	p_thread->Start();
	return 1;
}
//
// Test module
// Тестовый модуль NGINX для того, чтобы понять как эта штука работает и, возможно в дальнейшем, для тестирования. 
//
// 
// Content handler.
// 
extern ngx_module_t ngx_http_papyrus_test_module;

struct NgxModule_Papyrus {
	struct Config {
		static void * CreateLocConf(ngx_conf_t * cf)
		{
			NgxModule_Papyrus::Config * p_conf = static_cast<NgxModule_Papyrus::Config *>(ngx_pcalloc(cf->pool, sizeof(NgxModule_Papyrus::Config)));
			if(p_conf) {
				memcpy(p_conf->Signature, "PPLC", 4);
			}
			return p_conf;
		}
		static char * MergeLocConf(ngx_conf_t * cf, void * parent, void * child)
		{
			const NgxModule_Papyrus::Config * prev = static_cast<const NgxModule_Papyrus::Config *>(parent);
			NgxModule_Papyrus::Config * conf = static_cast<NgxModule_Papyrus::Config *>(child);
			ngx_conf_merge_str_value(conf->DbSymb, prev->DbSymb, "");
			ngx_conf_merge_str_value(conf->UserName, prev->UserName, "");
			ngx_conf_merge_str_value(conf->Password, prev->Password, "");
			return NGX_CONF_OK;
		}
		enum {
			stInited = 0x0001
		};
		char   Signature[4];
		long   State;
		ngx_str_t DbSymb;
		ngx_str_t UserName;
		ngx_str_t Password;
		ngx_str_t Query;
	};
	static const char * SetHandler(ngx_conf_t * cf, const ngx_command_t * cmd, void * conf)
	{
		// Install the papyrus_test handler. 
		NgxModule_Papyrus::Config * p_cfg = static_cast<NgxModule_Papyrus::Config *>(conf);
		ngx_http_core_loc_conf_t * clcf = static_cast<ngx_http_core_loc_conf_t *>(ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module)); // pointer to core location configuration 
		clcf->F_HttpHandler = HttpHandler;
		if(p_cfg) {
			if(p_cfg->State & p_cfg->stInited)
				return "is duplicate";
			else
				p_cfg->State |= p_cfg->stInited;
		}
		return NGX_CONF_OK;
	}
	static ngx_int_t HttpHandler(ngx_http_request_t * pReq)
	{
		Config * p_cfg = static_cast<Config *>(ngx_http_get_module_loc_conf(pReq, ngx_http_papyrus_test_module));
		static u_char ngx_papyrus_test[] = "papyrus test module! Здравствуй, брат!";
		// Set the Content-Type header. 
		pReq->SetContentType(SFileFormat::Html, cpUTF8);
		//pReq->headers_out.content_type.len = sizeof("text/html; charset=UTF-8") - 1;
		//pReq->headers_out.content_type.data = (u_char *)"text/html; charset=UTF-8";
		{
			//pReq->headers_out.charset.len = sizeof("utf-8") - 1;
			//pReq->headers_out.charset.data = (u_char *)"utf-8";
			//SETIFZ(pReq->headers_out.override_charset, (ngx_str_t *)ngx_palloc(pReq->pool, sizeof(ngx_str_t)));
			//pReq->headers_out.override_charset->len = sizeof("utf-8") - 1;
			//pReq->headers_out.override_charset->data = (u_char *)"utf-8";
		}
		if(0) {
			// Allocate a new buffer for sending out the reply. 
			ngx_buf_t * b = static_cast<ngx_buf_t *>(ngx_pcalloc(pReq->pool, sizeof(ngx_buf_t)));
			// Insertion in the buffer chain. 
			ngx_chain_t out(b, 0/*just one buffer*/);
			b->pos = ngx_papyrus_test; /* first position in memory of the data */
			b->last = ngx_papyrus_test + sizeof(ngx_papyrus_test) - 1; /* last position in memory of the data */
			b->memory = 1; // content is in read-only memory 
			b->last_buf = 1; // there will be no more buffers in the request 
			// Sending the headers for the reply. 
			pReq->headers_out.status = NGX_HTTP_OK; // 200 status code 
			// Get the content length of the body. 
			pReq->headers_out.content_length_n = sizeof(ngx_papyrus_test) - 1;
			ngx_http_send_header(pReq); // Send the headers 
			// Send the body, and return the status code of the output filter chain. 
			return ngx_http_output_filter(pReq, &out);
		}
		else {
			DS.DispatchNgxRequest(pReq, p_cfg);
			return NGX_DELEGATED/*NGX_DONE*/;
		}
	}
};

static ngx_command_t ngx_http_papyrus_test_commands[] = {
	{ ngx_string("papyrus_test"), NGX_HTTP_LOC_CONF|NGX_CONF_NOARGS, NgxModule_Papyrus::SetHandler, 0, 0, NULL },
	{ ngx_string("papyrus_dbsymb"), NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1, 
		ngx_conf_set_str_slot, NGX_HTTP_LOC_CONF_OFFSET, offsetof(NgxModule_Papyrus::Config, DbSymb), NULL },
	{ ngx_string("papyrus_username"), NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1, 
		ngx_conf_set_str_slot, NGX_HTTP_LOC_CONF_OFFSET, offsetof(NgxModule_Papyrus::Config, UserName), NULL },
	{ ngx_string("papyrus_password"), NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1, 
		ngx_conf_set_str_slot, NGX_HTTP_LOC_CONF_OFFSET, offsetof(NgxModule_Papyrus::Config, Password), NULL },
	{ ngx_string("papyrus_query"), NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1, 
		ngx_conf_set_str_slot, NGX_HTTP_LOC_CONF_OFFSET, offsetof(NgxModule_Papyrus::Config, Query), NULL },
	ngx_null_command // command termination 
};
//
// The module context
//
static ngx_http_module_t ngx_http_papyrus_test_module_ctx = {
	NULL, // preconfiguration
	NULL, // postconfiguration
	NULL, // create main configuration
	NULL, // init main configuration 
	NULL, // create server configuration
	NULL, // merge server configuration 
	NgxModule_Papyrus::Config::CreateLocConf, /* create location configuration */
	NgxModule_Papyrus::Config::MergeLocConf /* merge location configuration */
};
//
// Module definition
//
ngx_module_t ngx_http_papyrus_test_module = {
	NGX_MODULE_V1,
	&ngx_http_papyrus_test_module_ctx, // module context 
	ngx_http_papyrus_test_commands, // module directives 
	NGX_HTTP_MODULE, // module type 
	NULL, // init master
	NULL, // init module
	NULL, // init process
	NULL, // init thread
	NULL, // exit thread
	NULL, // exit process
	NULL, // exit master 
	NGX_MODULE_V1_PADDING
};
//
//
//
// @construction
class NgxReqResultQueue : private SQueue { //req_queue(1024)
public:
	NgxReqResultQueue() : SQueue(sizeof(NgxReqResult), 1024, aryDataOwner)
	{
	}
	int    FASTCALL Push(const NgxReqResult * pRes)
	{
		int    ok = 0;
		Lck.Lock();
		ok = SQueue::push(pRes);
		Lck.Unlock();
		return ok;
	}
	int    FASTCALL Pop(NgxReqResult * pRes)
	{
		NgxReqResult * p_res = 0;
		int    ok = 0;
		Lck.Lock();
		p_res = static_cast<NgxReqResult *>(SQueue::pop());
		Lck.Unlock();
		if(p_res) {
			ASSIGN_PTR(pRes, *p_res);
			ok = 1;
		}
		return ok;
	}
private:
	SMtLock Lck;
};

static NgxReqResultQueue * P_NgxReqResultQ = 0;

int FASTCALL NgxPushRequestResult(NgxReqResult * pR)
{
	int    ok = 0;
	if(SETIFZ(P_NgxReqResultQ, new NgxReqResultQueue)) {
		ok = P_NgxReqResultQ->Push(pR);
	}
	return ok;
}

int FASTCALL NgxPopRequestResult(NgxReqResult * pR)
{
	int    ok = 0;
	if(P_NgxReqResultQ) {
		ok = P_NgxReqResultQ->Pop(pR);
	}
	return ok;
}

class NgxReqQueue : private SQueue { //req_queue(1024)
public:
	NgxReqQueue() : SQueue(sizeof(ngx_http_request_t *), 1024, aryDataOwner|aryPtrContainer)
	{
	}
	int    FASTCALL Push(const ngx_http_request_t * pReq)
	{
		int    ok = 0;
		Lck.Lock();
		ok = SQueue::push(pReq);
		Lck.Unlock();
		return ok;
	}
	ngx_http_request_t * FASTCALL Pop()
	{
		ngx_http_request_t * p_req = 0;
		Lck.Lock();
		p_req = static_cast<ngx_http_request_t *>(SQueue::pop());
		Lck.Unlock();
		return p_req;
	}
private:
	SMtLock Lck;
};
//
// Descr: Сессия ориентированная на работу в том же процессе, что и клиентские сессии.
//   Для обмена данными использует SBufferPipe.
// Note: Разрабатывается в рамках включения WEB-сервера в состав процесса Papyrus
//
class PPWorkingPipeSession : public PPWorkerSession {
public:
	PPWorkingPipeSession(NgxReqQueue * pReqQueue, const DbLoginBlock & rDblBlk) : 
		PPWorkerSession(PPThread::kWorkerSession), WakeUpEv(Evnt::modeCreateAutoReset), DblBlk(rDblBlk), P_Queue(pReqQueue), P_OutPipe(0)
	{
		InitStartupSignal();
	}
	~PPWorkingPipeSession()
	{
	}
	void   WakeUp()
	{
		WakeUpEv.Signal();
	}
private:
	virtual void Startup()
	{
		PPWorkerSession::Startup();
		SignalStartup();
	}
	virtual void Run()
	{
		#define INTERNAL_ERR_INVALID_WAITING_MODE 0
		//
		int    login_result = -1;
		SString s, fmt_buf, log_buf, temp_buf;
		PPJobSrvReply reply(0);
		PPServerCmd cmd;
		if(DblBlk.GetAttr(DbLoginBlock::attrDbSymb, temp_buf) > 0 && temp_buf.NotEmptyS()) {
			SString db_symb = temp_buf;
			if(DblBlk.GetAttr(DbLoginBlock::attrUserName, temp_buf) > 0 && temp_buf.NotEmptyS()) {
				const SString user_name = temp_buf;
				DblBlk.GetAttr(DbLoginBlock::attrPassword, temp_buf);
				login_result = DS.Login(db_symb, user_name, temp_buf, PPSession::loginfSkipLicChecking);
				temp_buf.Obfuscate();
				if(login_result) {
					State |= stLoggedIn;
				}
				else {
					PPLogMessage(PPFILNAM_SERVER_LOG, 0, LOGMSGF_TIME|LOGMSGF_LASTERR);
				}
			}
		}
		/*if(login_result)*/ {
			SString _s, log_file_name, debug_log_file_name;
			uint32 wait_obj_localstop = INFINITE;
			uint32 wait_obj_stop = INFINITE;
			uint32 wait_obj_wakeup = INFINITE;
			Evnt   stop_event(SLS.GetStopEventName(_s), Evnt::modeOpen);
			PPGetFilePath(PPPATH_LOG, PPFILNAM_SERVER_LOG, log_file_name);
			PPGetFilePath(PPPATH_LOG, PPFILNAM_SERVERDEBUG_LOG, debug_log_file_name);
			while(1) {
				uint   h_count = 0;
				HANDLE h_list[32];
				uint32 timeout = 60000;
				h_list[h_count++] = EvLocalStop;
				wait_obj_localstop = WAIT_OBJECT_0+h_count-1;
				h_list[h_count++] = stop_event;
				wait_obj_stop = WAIT_OBJECT_0+h_count-1;
				h_list[h_count++] = WakeUpEv;
				wait_obj_wakeup = WAIT_OBJECT_0+h_count-1;
				//
				uint   r = WaitForMultipleObjects(h_count, h_list, 0, timeout);
				ngx_http_request_t * p_req = 0;
				if(r == WAIT_TIMEOUT) {
					p_req = P_Queue->Pop();
				}
				else if(r == wait_obj_wakeup) {
					p_req = P_Queue->Pop();
				}
				else if(r == wait_obj_localstop) {
					// @todo log
					break;
				}
				else if(r == wait_obj_stop) {
					// @todo log
					break;
				}
				if(p_req) {
					ResetIdleState();
					ProcessHttpRequest(p_req, cmd, reply);
					SetIdleState();
				}
			}
		}
	}
	virtual CmdRet ProcessCommand(PPServerCmd * pEv, PPJobSrvReply & rReply)
	{
		CmdRet ok = PPWorkerSession::ProcessCommand(pEv, rReply);
		if(ok == cmdretUnprocessed) {
			/*
			int    disable_err_reply = 0;
			int    r = 0;
			SString reply_buf, temp_buf, name, db_symb;
			// (StartWriting уже был выполнен вызовом PPWorkerSession::ProcessCommand) THROW(rReply.StartWriting());
			switch(pEv->GetH().Type) {
				default:
					CALLEXCEPT_PP(PPERR_INVSERVERCMD);
					break;
			}
			CATCH
				if(!disable_err_reply)
					rReply.SetError();
				PPErrorZ();
				ok = cmdretError;
			ENDCATCH
			if(ok != cmdretResume)
				rReply.FinishWriting();
			*/
		} // Если родительский класс вернул что-то отличное от cmdretUnprocessed, то - немедленный выход с этим результатом
		return ok;
	}
	int    ProcessHttpRequest(ngx_http_request_t * pReq, PPServerCmd & rCmd, PPJobSrvReply & rReply);
	int    PreprocessContent(const char * pSrc, SBuffer & rResult);

	Evnt   WakeUpEv; // Событие, которым можно разбудить поток что бы он взял запрос из очереди и обработал
	SBufferPipe * P_OutPipe; // @notowned Указатель на этот канал поток получает вместе с командой
	NgxReqQueue * P_Queue; // @notowned Указатель на очередь запросов сервера
	const DbLoginBlock DblBlk;
};

int PPWorkingPipeSession::PreprocessContent(const char * pSrc, SBuffer & rResult)
{
	int    ok = 1;
	SString temp_buf;
	SString txt_buf;
	SString file_name;
	//StringSet in_line(',', rOutTemplate);
	StringSet ext_param_list;
	uint   ip = 0;
	//THROW_PP(in_line.get(&ip, file_name), PPERR_CMDSEL_MISSTDDOTEMPLATE);
	//THROW(Tddo::LoadFile(file_name, temp_buf));
	/*
	while(in_line.get(&ip, txt_buf))
		ext_param_list.add(txt_buf.Strip());
	*/
	{
		Tddo t;
		t.SetInputFileName(file_name);
		DlRtm::ExportParam ep;
		PPFilt _pf;
		_pf.ID = 0/*dataId*/;
		_pf.Ptr = 0/*pAry*/;
		ep.P_F = &_pf;
		THROW(t.Process("HttpPreprocessBase", pSrc, ep, &ext_param_list, rResult)); // @badcast
		//rResult.SetDataType(PPJobSrvReply::htGenericText, 0);
	}
	CATCHZOK
	return ok;
}

int PPWorkingPipeSession::ProcessHttpRequest(ngx_http_request_t * pReq, PPServerCmd & rCmd, PPJobSrvReply & rReply)
{
	int    ok = 1;
	NgxReqResult result;
	if(pReq) {
		const NgxModule_Papyrus::Config * p_cfg = (const NgxModule_Papyrus::Config *)ngx_http_get_module_loc_conf(pReq, ngx_http_papyrus_test_module);
		SString out_buf;
		SString temp_buf;
		SString cmd_buf;
		//char   sb[256];
		int    do_preprocess_content = 0;
		const PPThreadLocalArea & r_tla = DS.GetConstTLA();
		if(r_tla.State & r_tla.stAuth) {
			rReply.Z();
			SHttpProtocol::Auth a;
			int cmdret = 0;
			ngx_buf_t * b = 0;
			if(pReq->GetArg("command", temp_buf)) {
				temp_buf.DecodeUrl(cmd_buf);
				if(pReq->GetArg("Authorization", temp_buf)) {
					SString auth_buf;
					temp_buf.DecodeUrl(auth_buf);
					if(SHttpProtocol::ParseAuth(auth_buf, a)) {
						;
					}
				}
				rCmd.Z();
				if(rCmd.ParseLine(cmd_buf, (State & stLoggedIn) ? rCmd.plfLoggedIn : 0)) {
					cmdret = ProcessCommand(&rCmd, rReply);
				}
			}
			else {
				PPObjWorkbook wb_obj;
				PPID   wb_id = 0;
				WorkbookTbl::Rec wb_rec;
				do_preprocess_content = 1;
				if(wb_obj.SearchBySymb("PETROGLIF", &wb_id, &wb_rec) > 0) {
					cmd_buf.Z().Cat("GETWORKBOOKCONTENT").Space().Cat(wb_id);
					rCmd.Z();
					if(rCmd.ParseLine(cmd_buf, (State & stLoggedIn) ? rCmd.plfLoggedIn : 0)) {
						cmdret = ProcessCommand(&rCmd, rReply);
					}
				}
			}
			{
				size_t reply_size = 0;
				pReq->SetContentType(SFileFormat::Html, cpUTF8);
				rReply.StartReading(0);
				if(rReply.CheckRepError()) {
					PPJobSrvReply::TransmitFileBlock tfb;
					if(rReply.GetH().Type == rReply.htFile) {
						rReply.Read(&tfb, sizeof(tfb));
					}
					reply_size = rReply.GetAvailableSize();
					if(do_preprocess_content) {
						temp_buf.Z().CatN(PTRCHRC(rReply.GetBuf(rReply.GetRdOffs())), reply_size);
						rReply.Z();
						PreprocessContent(temp_buf, rReply);
						reply_size = rReply.GetAvailableSize();
					}
					b = ngx_create_temp_buf(pReq->pool, reply_size);
					ngx_chain_t out(b, 0/*just one buffer*/);
					rReply.Read(b->pos, reply_size);
					b->last = b->pos + reply_size;
					b->memory = 1;
					b->last_buf = 1; // there will be no more buffers in the request 
				}
				else {
					out_buf = "Error!";
					reply_size = out_buf.Len();
					b = ngx_create_temp_buf(pReq->pool, reply_size);
					ngx_chain_t out(b, 0/*just one buffer*/);
					rReply.Read(b->pos, reply_size);
					b->last = b->pos + reply_size;
					b->memory = 1;
					b->last_buf = 1; // there will be no more buffers in the request 
				}
				pReq->headers_out.status = NGX_HTTP_OK; // 200 status code 
				pReq->headers_out.content_length_n = reply_size; // Get the content length of the body. 
			}
			result.ReplyCode = NGX_DONE;
			result.P_Req = pReq;
			result.Chain.buf = b;
			result.Chain.next = 0;
			NgxPushRequestResult(&result);
		}
		else {
			PPVersionInfo vi = DS.GetVersionInfo();
			//vi.GetProductName(temp_buf);
			vi.GetTextAttrib(vi.taiProductName, temp_buf);
			out_buf.Cat(temp_buf);
			//vi.GetVersionText(sb, sizeof(sb));
			vi.GetTextAttrib(vi.taiVersionText, temp_buf);
			out_buf.Space().Cat(temp_buf);
			//vi.GetTeam(sb, sizeof(sb));
			vi.GetTextAttrib(vi.taiTeam, temp_buf);
			out_buf.Space().Cat(temp_buf.Transf(CTRANSF_INNER_TO_UTF8));
			out_buf.CR();
			out_buf.CatEq(/*"Идентификатор потока"*/"Thread ident", DS.GetConstTLA().GetThreadID());
			//
			pReq->SetContentType(SFileFormat::Html, cpUTF8);
			{
				ngx_buf_t * b = ngx_create_temp_buf(pReq->pool, out_buf.Len());
				// Insertion in the buffer chain. 
				ngx_chain_t out(b, 0/*just one buffer*/);
				memcpy(b->pos, out_buf.cptr(), out_buf.Len());
				b->last = b->pos + out_buf.Len();
				b->memory = 1;
				b->last_buf = 1; // there will be no more buffers in the request 
				// Sending the headers for the reply. 
				pReq->headers_out.status = NGX_HTTP_OK; // 200 status code 
				pReq->headers_out.content_length_n = out_buf.Len(); // Get the content length of the body. 
				{
					result.ReplyCode = NGX_DONE;
					result.P_Req = pReq;
					result.Chain.buf = b;
					result.Chain.next = 0;
					NgxPushRequestResult(&result);
				}
			}
		}
	}
	return ok;
}

int PPSession::DispatchNgxRequest(void * pReq, const void * pCfg)
{
	static NgxReqQueue * P_Queue = 0;

	int    ok = -1;
	const  uint max_threads = 64;
	uint   thread_count = 0;
	ENTER_CRITICAL_SECTION
	PPWorkingPipeSession * p_thread = 0;
	SETIFZ(P_Queue, new NgxReqQueue);
	if(P_Queue) {
		SString temp_buf;
		thread_count = ThreadList.GetCount(PPThread::kWorkerSession);
		while(!p_thread) {
			p_thread = static_cast<PPWorkingPipeSession *>(ThreadList.SearchIdle(PPThread::kWorkerSession));
			if(!p_thread) {
				if(thread_count < max_threads) {
					const NgxModule_Papyrus::Config * p_cfg = static_cast<const NgxModule_Papyrus::Config *>(pCfg);
					DbLoginBlock dblblk;
					if(p_cfg) {
						if(p_cfg->DbSymb.len) {
							temp_buf.Z().CatN(PTRCHRC_(p_cfg->DbSymb.data), p_cfg->DbSymb.len);
							dblblk.SetAttr(DbLoginBlock::attrDbSymb, temp_buf);
						}
						if(p_cfg->UserName.len) {
							temp_buf.Z().CatN(PTRCHRC_(p_cfg->UserName.data), p_cfg->UserName.len);
							dblblk.SetAttr(DbLoginBlock::attrUserName, temp_buf);
						}
						if(p_cfg->Password.len) {
							temp_buf.Z().CatN(PTRCHRC_(p_cfg->Password.data), p_cfg->Password.len);
							dblblk.SetAttr(DbLoginBlock::attrPassword, temp_buf);
						}
					}
					PPWorkingPipeSession * p_new_sess = new PPWorkingPipeSession(P_Queue, dblblk);
					p_new_sess->Start(1); // (1) - Подождать запуска
					uint new_thread_count = ThreadList.GetCount(PPThread::kWorkerSession);
					assert(new_thread_count == thread_count+1);
					thread_count = new_thread_count;
					p_thread = p_new_sess;
				}
				else {
					break;
				}
			}
		};
		P_Queue->Push(static_cast<ngx_http_request_t *>(pReq));
		if(p_thread) {
			p_thread->WakeUp();
		}
	}
	LEAVE_CRITICAL_SECTION
	return ok;
}