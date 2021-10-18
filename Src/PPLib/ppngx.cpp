// PPNGX.CPP
// Copyright (c) A.Sobolev 2017, 2018, 2019, 2020, 2021
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
				p_conf->State = 0;
				p_conf->Speciality = static_cast<uint>(-1); // unset
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
			ngx_conf_merge_uint_value(conf->Speciality, prev->Speciality, NgxModule_Papyrus::Config::specialityUndef);
			return NGX_CONF_OK;
		}
		enum {
			stInited = 0x0001
		};
		enum {
			specialityUndef = 0,
			specialityPapyrus,
			specialityStyloQ,
		};
		char   Signature[4];
		long   State;
		uint32 Speciality;
		ngx_str_t DbSymb;
		ngx_str_t UserName;
		ngx_str_t Password;
		//ngx_str_t Query;
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
			if(cmd->Name.len) {
				if(sstreq(cmd->Name.data, "papyrus")) {
					p_cfg->Speciality = p_cfg->specialityPapyrus;
				}
				else if(sstreq(cmd->Name.data, "styloq")) {
					p_cfg->Speciality = p_cfg->specialityStyloQ;
				}
			}
		}
		return NGX_CONF_OK;
	}
	//
	// From http://nginx.org/en/docs/dev/development_guide.html example
	//
	static void /*ngx_http_foo_init*/HttpContentInit(ngx_http_request_t * pReq)
	{
		/*
		if(!pReq->request_body) {
			ngx_http_finalize_request(pReq, NGX_HTTP_INTERNAL_SERVER_ERROR);
		}
		else {
			off_t len = 0;
			for(ngx_chain_t * in = pReq->request_body->bufs; in; in = in->next) {
				len += ngx_buf_size(in->buf);
			}
			{
				ngx_buf_t * b = ngx_create_temp_buf(pReq->pool, NGX_OFF_T_LEN);
				if(!b) {
					ngx_http_finalize_request(pReq, NGX_HTTP_INTERNAL_SERVER_ERROR);
				}
				else {
					b->last = ngx_sprintf(b->pos, "%O", len);
					b->last_buf = (pReq == pReq->main) ? 1: 0;
					b->last_in_chain = 1;
					pReq->headers_out.status = NGX_HTTP_OK;
					pReq->headers_out.content_length_n = b->last - b->pos;
					{
						ngx_int_t rc = ngx_http_send_header(pReq);
						if(rc == NGX_ERROR || rc > NGX_OK || pReq->header_only) {
							ngx_http_finalize_request(pReq, rc);
						}
						else {
							ngx_chain_t out;
							out.buf = b;
							out.next = NULL;
							rc = ngx_http_output_filter(pReq, &out);
							ngx_http_finalize_request(pReq, rc);
						}
					}
				}
			}
		}
		*/
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
		int rc = ngx_http_read_client_request_body(pReq, HttpContentInit);
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
	{ ngx_string("papyrus"), NGX_HTTP_LOC_CONF|NGX_CONF_NOARGS, NgxModule_Papyrus::SetHandler, NGX_HTTP_LOC_CONF_OFFSET, 0, NULL },
	{ ngx_string("styloq"), NGX_HTTP_LOC_CONF|NGX_CONF_NOARGS, NgxModule_Papyrus::SetHandler, NGX_HTTP_LOC_CONF_OFFSET, 0, NULL },
	{ ngx_string("papyrus_dbsymb"), NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1, ngx_conf_set_str_slot, NGX_HTTP_LOC_CONF_OFFSET, offsetof(NgxModule_Papyrus::Config, DbSymb), NULL },
	{ ngx_string("papyrus_username"), NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1, ngx_conf_set_str_slot, NGX_HTTP_LOC_CONF_OFFSET, offsetof(NgxModule_Papyrus::Config, UserName), NULL },
	{ ngx_string("papyrus_password"), NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1, ngx_conf_set_str_slot, NGX_HTTP_LOC_CONF_OFFSET, offsetof(NgxModule_Papyrus::Config, Password), NULL },
	//{ ngx_string("papyrus_query"), NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1, ngx_conf_set_str_slot, NGX_HTTP_LOC_CONF_OFFSET, offsetof(NgxModule_Papyrus::Config, Query), NULL },
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
	NgxModule_Papyrus::Config::CreateLocConf, // create location configuration 
	NgxModule_Papyrus::Config::MergeLocConf // merge location configuration 
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
static bool GetHttpReqRtsId(ngx_http_request_t * pReq, SString & rRtsId)
{
	rRtsId.Z();
	bool   ok = false;
	SString temp_buf;
	if(pReq && pReq->args.len > 0 && pReq->args.data) {
		temp_buf.Z().CatN(reinterpret_cast<const char *>(pReq->args.data), pReq->args.len);
		StringSet ss('&', temp_buf);
		SString key_buf, val_buf;
		for(uint ssp = 0; ss.get(&ssp, temp_buf);) {
			if(temp_buf.Divide('=', key_buf, val_buf) > 0 && key_buf.Strip().IsEqiAscii("rtsid")) {
				rRtsId = val_buf.Strip();
				ok = true;
				break;
			}
		}
	}
	return ok;
}
//
//
//
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
	PPWorkingPipeSession(NgxReqQueue * pReqQueue, const DbLoginBlock & rDblBlk, const char * pOuterSignature) : 
		PPWorkerSession(PPThread::kWorkerSession), WakeUpEv(Evnt::modeCreateAutoReset), DblBlk(rDblBlk), P_Queue(pReqQueue), P_OutPipe(0), P_StqRtb(0), P_Ic(0)
	{
		SetOuterSignature(pOuterSignature);
		InitStartupSignal();
	}
	~PPWorkingPipeSession()
	{
		ZDELETE(P_Ic);
		delete P_StqRtb;
	}
	void   WakeUp()
	{
		WakeUpEv.Signal();
	}
private:
	int    Login(uint flags)
	{
		int    ok = 1;
		int    login_result = -1;
		bool   is_logged_id = false;
		char   secret[64];
		SString db_symb;
		SString temp_buf;
		const PPThreadLocalArea & r_tla = DS.GetConstTLA();
		if(DblBlk.GetAttr(DbLoginBlock::attrDbSymb, db_symb)) {
			SString user_name;
			if(DblBlk.GetAttr(DbLoginBlock::attrUserName, temp_buf) > 0 && temp_buf.NotEmptyS()) {
				user_name = temp_buf;
				DblBlk.GetAttr(DbLoginBlock::attrPassword, temp_buf);
				STRNSCPY(secret, temp_buf);
				temp_buf.Obfuscate();
			}
			else if(flags & PPSession::loginfAllowAuthAsJobSrv) {
				user_name = PPSession::P_JobLogin;
				PPVersionInfo vi = DS.GetVersionInfo();
				THROW(vi.GetSecret(secret, sizeof(secret)));
			}
			THROW(user_name.NotEmpty());
			//const SString user_name = temp_buf;
			if(r_tla.State & r_tla.stAuth) {
				DbProvider * p_dict = CurDict;
				if(p_dict) {
					p_dict->GetDbSymb(temp_buf);
					if(temp_buf.IsEqiAscii(db_symb)) {
						is_logged_id = true;
					}
				}
				if(!is_logged_id) {
					Logout();
				}
			}
			if(!is_logged_id) {
				login_result = DS.Login(db_symb, user_name, secret, PPSession::loginfSkipLicChecking|PPSession::loginfInternal);
			}
			else 
				login_result = 1;
			memzero(secret, sizeof(secret));
			if(login_result)
				State_PPws |= stLoggedIn;
			else {
				PPLogMessage(PPFILNAM_SERVER_LOG, 0, LOGMSGF_TIME|LOGMSGF_LASTERR);
			}
		}
		CATCHZOK
		return ok;
	}
	void   Logout()
	{
		ZDELETE(P_Ic);
		DS.Logout();
		State_PPws &= ~stLoggedIn;
	}
	virtual void Startup()
	{
		PPWorkerSession::Startup();
		SignalStartup();
	}
	virtual void Run()
	{
		#define INTERNAL_ERR_INVALID_WAITING_MODE 0
		//
		const int outer_signature_timeout_sec = 120; // секунды // для отладки побольше, в нормальной ситуации должно быть секунд 15
		SString s;
		SString fmt_buf;
		SString log_buf;
		SString temp_buf;
		//PPJobSrvReply reply(0);
		PPServerCmd cmd;
		const int login_result = Login(0);
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
				// При наличии внешней сигратуры надо чаще проверять необходимость ее сброса по таймауту
				// дабы не держать потоки занятыми, если сигнатура более не валидна.
				uint32 timeout = HasOuterSignature() ? 1000 : 60000;
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
					// Если получено задание и установлена внешняя сигнатура, то, скорее всего диспетчер отдал задание потоку 
					// только потому, что у него была эта сигнатура. Таким образом, в этом случае проверять таймаут сигнатуры резона нет.
					if(!p_req) { 
						ResetOuterSignatureByTimeout(time(0), outer_signature_timeout_sec);
					}
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
					ProcessHttpRequest(p_req, cmd);
					SetIdleState();
				}
			}
		}
	}
	virtual CmdRet ProcessCommand_(PPServerCmd * pEv, PPJobSrvReply & rReply)
	{
		CmdRet ok = PPWorkerSession::ProcessCommand_(pEv, rReply);
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
	void   ProcessHttpRequest(ngx_http_request_t * pReq, PPServerCmd & rCmd);
	void   ProcessHttpRequest_StyloQ(ngx_http_request_t * pReq, PPServerCmd & rCmd);
	int    PreprocessContent(const char * pSrc, SBuffer & rResult);
	void   PushNgxResult(ngx_http_request_t * pReq, int ngxReplyCode, uint httpReplyCode, SFileFormat contentFmt, SCodepage contentCp, const SString & rText);

	Evnt   WakeUpEv; // Событие, которым можно разбудить поток что бы он взял запрос из очереди и обработал
	SBufferPipe * P_OutPipe; // @notowned Указатель на этот канал поток получает вместе с командой
	NgxReqQueue * P_Queue; // @notowned Указатель на очередь запросов сервера
	DbLoginBlock DblBlk;
	PPStyloQInterchange::RoundTripBlock * P_StqRtb; // @v11.2.0
	PPStyloQInterchange * P_Ic; // @v11.2.0
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

void PPWorkingPipeSession::PushNgxResult(ngx_http_request_t * pReq, int ngxReplyCode, uint httpReplyCode, SFileFormat contentFmt, SCodepage contentCp, const SString & rText)
{
	pReq->SetContentType(/*SFileFormat::Html, cpUTF8*/contentFmt, contentCp);
	ngx_buf_t * b = ngx_create_temp_buf(pReq->pool, rText.Len());
	// Insertion in the buffer chain. 
	ngx_chain_t out(b, 0/*just one buffer*/);
	memcpy(b->pos, rText.cptr(), rText.Len());
	b->last = b->pos + rText.Len();
	b->memory = 1;
	b->last_buf = 1; // there will be no more buffers in the request 
	// Sending the headers for the reply. 
	pReq->headers_out.status = httpReplyCode/*NGX_HTTP_OK*/; // 200 status code 
	pReq->headers_out.content_length_n = rText.Len(); // Get the content length of the body. 
	{
		NgxReqResult result;
		result.ReplyCode = ngxReplyCode/*NGX_DONE*/;
		result.P_Req = pReq;
		result.Chain.buf = b;
		result.Chain.next = 0;
		NgxPushRequestResult(&result);
	}
}

void PPWorkingPipeSession::ProcessHttpRequest_StyloQ(ngx_http_request_t * pReq, PPServerCmd & rCmd)
{
	SString temp_buf;
	SString out_buf;
	StyloQProtocol sp;
	StyloQProtocol reply_tp;
	SBinaryChunk cli_ident;
	SBinaryChunk sess_secret;
	SBinaryChunk my_public;
	SBinaryChunk other_public;
	SString req_rtsid;
	out_buf.Cat("(empty)"); // @debug
	THROW(pReq->request_body);
	GetHttpReqRtsId(pReq, req_rtsid);
	temp_buf.Z();
	for(ngx_chain_t * p_nb = pReq->request_body->bufs; p_nb; p_nb = p_nb->next) {
		temp_buf.CatN(reinterpret_cast<const char *>(p_nb->buf->pos), ngx_buf_size(p_nb->buf));
	}
	if(temp_buf.IsEqiAscii("test-request-for-connection-checking")) {
		out_buf = "your-test-request-is-accepted";
	}
	else {
		if(P_StqRtb) {
			THROW(P_StqRtb->Sess.Get(SSecretTagPool::tagSessionSecret, &sess_secret));
		}
		THROW(sp.ReadMime64(temp_buf, &sess_secret));
		sp.P.Get(SSecretTagPool::tagClientIdent, &cli_ident);
		if(!P_StqRtb) { // Если инициирующий запрос, то необходимо разобраться с какой базой данных работать
			SString stq_dbsymb;
			SBinaryChunk svc_ident;
			sp.P.Get(SSecretTagPool::tagSvcIdent, &svc_ident);
			THROW(svc_ident);
			THROW(StyloQCore::GetDbMapBySvcIdent(svc_ident, &stq_dbsymb, 0));
			assert(stq_dbsymb.Len());
			DblBlk.SetAttr(DbLoginBlock::attrDbSymb, stq_dbsymb);
		}
		switch(sp.GetH().Type) {
			case PPSCMD_SQ_ACQUAINTANCE: // @init
				{
					//SBinaryChunk sess_secret; // @debug
					THROW_PP(cli_ident.Len(), PPERR_SQ_UNDEFCLIID);
					THROW_PP(sp.P.Get(SSecretTagPool::tagSessionPublicKey, &other_public), PPERR_SQ_UNDEFSESSPUBKEY);
					assert(other_public.Len());
					THROW(Login(PPSession::loginfAllowAuthAsJobSrv));
					THROW(SETIFZ(P_Ic, new PPStyloQInterchange));
					ZDELETE(P_StqRtb);
					THROW_SL(P_StqRtb = new PPStyloQInterchange::RoundTripBlock());
					P_StqRtb->LastRcvCmd = sp.GetH().Type;
					P_StqRtb->Other.Put(SSecretTagPool::tagClientIdent, cli_ident);
					P_StqRtb->Other.Put(SSecretTagPool::tagSessionPublicKey, other_public);
					const int fsks = P_Ic->KexServiceReply(P_StqRtb->Sess, sp.P, /*p_debug_pub_ecp_x*/0, /*p_debug_pub_ecp_y*/0);
					THROW(fsks);
					{
						//
						// Неоднозначность: надо ли обрабатывать запрос на знакомство если мы уже знакомы (fsks != fsksNewEntry)
						//
						THROW(P_StqRtb->Sess.Get(SSecretTagPool::tagSessionPublicKey, &my_public));
						assert(my_public.Len());
						reply_tp.StartWriting(PPSCMD_SQ_ACQUAINTANCE, StyloQProtocol::psubtypeReplyOk);
						reply_tp.P.Put(SSecretTagPool::tagSessionPublicKey, my_public);
						THROW(reply_tp.FinishWriting(0));
						out_buf.Z().EncodeMime64(reply_tp.constptr(), reply_tp.GetAvailableSize());
						P_StqRtb->LastSndCmd = reply_tp.GetH().Type;
					}
				}
				break;
			case PPSCMD_SQ_SESSION: // @init Команда инициации соединения по значению сессии, которая была установлена на предыдущем сеансе обмена
				{
					SBinaryChunk temp_chunk;
					SBinaryChunk * p_sess_secret = 0;
					StyloQCore::StoragePacket pack;
					THROW_PP(sp.P.Get(SSecretTagPool::tagSessionPublicKey, &other_public), PPERR_SQ_INPHASNTSESSPUBKEY); // PPERR_SQ_INPHASNTSESSPUBKEY Входящий запрос сессии не содержит публичного ключа
					THROW_PP(cli_ident.Len(), PPERR_SQ_INPHASNTCLIIDENT); // PPERR_SQ_INPHASNTCLIIDENT Входящий запрос сессии не содержит идентификатора контрагента
					assert(other_public.Len());
					//assert(cli_ident.Len());
					THROW(Login(PPSession::loginfAllowAuthAsJobSrv));
					THROW(SETIFZ(P_Ic, new PPStyloQInterchange));
					THROW(P_Ic->SearchSession(other_public, &pack) > 0); 

					ZDELETE(P_StqRtb);
					THROW_SL(P_StqRtb = new PPStyloQInterchange::RoundTripBlock());
					P_StqRtb->LastRcvCmd = sp.GetH().Type;

					THROW_PP(pack.Pool.Get(SSecretTagPool::tagSessionPublicKey, &my_public), PPERR_SQ_INRSESSHASNTPUBKEY); // PPERR_SQ_INRSESSHASNTPUBKEY      Сохраненная сессия не содержит публичного ключа
					THROW_PP(pack.Pool.Get(SSecretTagPool::tagSessionPublicKeyOther, &temp_chunk), PPERR_SQ_INRSESSHASNTOTHERPUBKEY); // PPERR_SQ_INRSESSHASNTOTHERPUBKEY Сохраненная сессия не содержит публичного ключа контрагента
					THROW_PP(temp_chunk == other_public, PPERR_SQ_INRSESPUBKEYNEQTOOTR); // PPERR_SQ_INRSESPUBKEYNEQTOOTR   Публичный ключ сохраненной сессии не равен полученному от контрагента
					THROW_PP(pack.Pool.Get(SSecretTagPool::tagClientIdent, &temp_chunk), PPERR_SQ_INRSESSHASNTCLIIDENT); // PPERR_SQ_INRSESSHASNTCLIIDENT   Сохраненная сессия не содержит идентификатора контрагента
					THROW_PP(temp_chunk == cli_ident, PPERR_SQ_INRSESCLIIDENTNEQTOOTR); // PPERR_SQ_INRSESCLIIDENTNEQTOOTR Идентификатора контрагента сохраненной сессии не равен полученному от контрагента
					THROW_PP(pack.Pool.Get(SSecretTagPool::tagSessionSecret, &sess_secret), PPERR_SQ_INRSESSHASNTSECRET); // PPERR_SQ_INRSESSHASNTSECRET     Сохраненная сессия не содержит секрета 

					P_StqRtb->Other.Put(SSecretTagPool::tagClientIdent, cli_ident);
					P_StqRtb->Other.Put(SSecretTagPool::tagSessionPublicKey, other_public);
					P_StqRtb->Sess.Put(SSecretTagPool::tagSessionSecret, sess_secret);
					p_sess_secret = &sess_secret;
					//
					reply_tp.StartWriting(PPSCMD_SQ_SESSION, StyloQProtocol::psubtypeReplyOk);
					THROW(reply_tp.FinishWriting(0));
					out_buf.Z().EncodeMime64(reply_tp.constptr(), reply_tp.GetAvailableSize());
					P_StqRtb->LastSndCmd = reply_tp.GetH().Type;
				}
				break;
			case PPSCMD_SQ_SRPAUTH: // @init Команда инициации соединения методом SRP-авторизации по параметрам, установленным ранее 
				{
					int    debug_mark = 0;
					PPID   id = 0; // Внутренний идентификатор записи клиента в DBMS
					SBinaryChunk srp_s;
					SBinaryChunk srp_v;
					SBinaryChunk __a; // A
					SBinaryChunk temp_bc;
					StyloQCore::StoragePacket storage_pack;
					THROW(sp.P.Get(SSecretTagPool::tagSessionPublicKey, &other_public));
					THROW_PP(cli_ident.Len(), PPERR_SQ_UNDEFCLIID);
					THROW(Login(PPSession::loginfAllowAuthAsJobSrv)); // @error (Inner service error: unable to login
					THROW(SETIFZ(P_Ic, new PPStyloQInterchange));
					ZDELETE(P_StqRtb);
					THROW_SL(P_StqRtb = new PPStyloQInterchange::RoundTripBlock());
					P_StqRtb->LastRcvCmd = sp.GetH().Type;
					P_StqRtb->Other.Put(SSecretTagPool::tagClientIdent, cli_ident);
					P_StqRtb->Other.Put(SSecretTagPool::tagSessionPublicKey, other_public);
					THROW(P_Ic->KexServiceReply(P_StqRtb->Sess, sp.P, 0, 0));
					P_StqRtb->Sess.Get(SSecretTagPool::tagSessionPublicKey, &my_public);
					assert(my_public.Len());
					THROW(sp.P.Get(SSecretTagPool::tagSrpA, &__a));
					THROW(P_Ic->SearchGlobalIdentEntry(StyloQCore::kClient, cli_ident, &storage_pack) > 0);
					THROW(storage_pack.Rec.Kind == StyloQCore::kClient);
					THROW(storage_pack.Pool.Get(SSecretTagPool::tagClientIdent, &temp_bc) && temp_bc == cli_ident); // @error (I don't know you)
					THROW(storage_pack.Pool.Get(SSecretTagPool::tagSrpVerifier, &srp_v)); // @error (I havn't got your registration data)
					THROW(storage_pack.Pool.Get(SSecretTagPool::tagSrpVerifierSalt, &srp_s)); // @error (I havn't got your registration data)
					{
						ZDELETE(P_StqRtb->P_SrpV);
						P_StqRtb->P_SrpV = P_Ic->CreateSrpPacket_Svc_Auth(my_public, cli_ident, srp_s, srp_v, __a, reply_tp);
						out_buf.Z().EncodeMime64(reply_tp.constptr(), reply_tp.GetAvailableSize());
					}
				}
				break;
			case PPSCMD_SQ_SRPAUTH_S2:
				if(P_StqRtb) {
					P_StqRtb->LastRcvCmd = sp.GetH().Type;
					if(P_StqRtb->P_SrpV) {
						SBinaryChunk __m; // M
						const uchar * p_bytes_HAMK = 0;
						THROW(sp.P.Get(SSecretTagPool::tagSrpM, &__m));
						P_StqRtb->P_SrpV->VerifySession(static_cast<const uchar *>(__m.PtrC()), &p_bytes_HAMK);
						THROW(p_bytes_HAMK); // @error User authentication failed!
						{
							// Host -> User: (HAMK) 
							const SBinaryChunk srp_hamk(p_bytes_HAMK, P_StqRtb->P_SrpV->GetSessionKeyLength());
							reply_tp.StartWriting(PPSCMD_SQ_SRPAUTH_S2, StyloQProtocol::psubtypeReplyOk);
							reply_tp.P.Put(SSecretTagPool::tagSrpHAMK, srp_hamk);
							reply_tp.FinishWriting(0);
							out_buf.Z().EncodeMime64(reply_tp.constptr(), reply_tp.GetAvailableSize());
						}
					}
				}
				break;
			case PPSCMD_SQ_SRPAUTH_ACK:
				if(P_StqRtb) {
					P_StqRtb->LastRcvCmd = sp.GetH().Type;
					{
						uint32 cli_session_expiry_period = 0;
						uint32 svc_session_expiry_period = P_Ic->GetNominalSessionLifeTimeSec();
						//
						// Теперь все - верификация завершена. Сохраняем сессию и дальше будем ждать полезных сообщений от клиента
						PPID   sess_id = 0;
						SBinaryChunk temp_chunk;
						SBinaryChunk _sess_secret;
						StyloQCore::StoragePacket sess_pack;
						//
						if(sp.P.Get(SSecretTagPool::tagSessionExpirPeriodSec, &temp_chunk)) {
							if(temp_chunk.Len() == sizeof(uint32)) {
								cli_session_expiry_period = PTR32C(temp_chunk.PtrC())[0];
							}
						}
						//
						// Теперь надо сохранить параметры сессии дабы в следующий раз не проделывать столь сложную процедуру
						//
						// Проверки assert'ами (не THROW) реализуются из-за того, что не должно возникнуть ситуации, когда мы
						// попали в этот участок кода с невыполненными условиями (то есть при необходимости THROW должны были быть вызваны выше).
						P_StqRtb->Sess.Get(SSecretTagPool::tagSessionSecret, &_sess_secret);
						P_StqRtb->Sess.Get(SSecretTagPool::tagSessionPublicKey, &my_public);
						P_StqRtb->Other.Get(SSecretTagPool::tagSessionPublicKey, &other_public);
						P_StqRtb->Other.Get(SSecretTagPool::tagClientIdent, &cli_ident);
						//P_StqRtb->StP.Pool.Get(SSecretTagPool::tagSvcIdent, )
						//assert(_sess_secret.Len());
						assert(my_public.Len());
						//assert(other_public.Len());
						assert(P_StqRtb->Sess.Get(SSecretTagPool::tagSessionPrivateKey, 0));
						sess_pack.Pool.Put(SSecretTagPool::tagSessionPublicKey, my_public);
						{
							P_StqRtb->Sess.Get(SSecretTagPool::tagSessionPrivateKey, &temp_chunk);
							sess_pack.Pool.Put(SSecretTagPool::tagSessionPrivateKey, temp_chunk);
						}
						sess_pack.Pool.Put(SSecretTagPool::tagSessionPublicKeyOther, other_public);
						sess_pack.Pool.Put(SSecretTagPool::tagSessionSecret, _sess_secret);
						sess_pack.Pool.Put(SSecretTagPool::tagClientIdent, cli_ident);
						if(cli_session_expiry_period || svc_session_expiry_period) {
							uint32 sep = 0;
							if(!cli_session_expiry_period || !svc_session_expiry_period)
								sep = MAX(cli_session_expiry_period, svc_session_expiry_period);
							else
								sep = MIN(cli_session_expiry_period, svc_session_expiry_period);
							if(sep)
								sess_pack.Rec.Expiration.SetTimeT(time(0) + sep);
						}
						THROW(P_Ic->StoreSession(&sess_id, &sess_pack, 1));
						{
							reply_tp.StartWriting(PPSCMD_SQ_SRPAUTH_ACK, StyloQProtocol::psubtypeReplyOk);
							reply_tp.P.Put(SSecretTagPool::tagSessionPublicKey, my_public);
							if(svc_session_expiry_period)
								reply_tp.P.Put(SSecretTagPool::tagSessionExpirPeriodSec, &svc_session_expiry_period, sizeof(svc_session_expiry_period));
							THROW(reply_tp.FinishWriting(0));
							out_buf.Z().EncodeMime64(reply_tp.constptr(), reply_tp.GetAvailableSize());
							P_StqRtb->LastSndCmd = reply_tp.GetH().Type;
						}
					}
				}
				break;
			case PPSCMD_SQ_SRPREGISTER:
				{
					int32 reply_status = 0;
					SString reply_status_text;
					StyloQProtocol reply_tp;
					THROW(P_StqRtb);
					P_StqRtb->LastRcvCmd = sp.GetH().Type;
					THROW(P_StqRtb->Sess.Get(SSecretTagPool::tagSessionSecret, &sess_secret));
					if(P_Ic->Registration_ServiceReply(*P_StqRtb, sp)) {
						SBinaryChunk bc;
						reply_tp.StartWriting(PPSCMD_SQ_SRPREGISTER, StyloQProtocol::psubtypeReplyOk);
						//
						// В случае успешной регистрации передаем клиенту наш лик и конфигурацию
						//
						if(P_StqRtb->StP.Pool.Get(SSecretTagPool::tagSelfyFace, &bc)) {
							assert(bc.Len());
							StyloQFace face_pack;
							temp_buf.Z().CatN(static_cast<const char *>(bc.PtrC()), bc.Len());
							if(face_pack.FromJson(temp_buf))
								reply_tp.P.Put(SSecretTagPool::tagFace, bc);
						}
						bc.Z();
						if(P_StqRtb->StP.Pool.Get(SSecretTagPool::tagConfig, &bc)) {
							assert(bc.Len());
							StyloQConfig cfg_pack;
							temp_buf.Z().CatN(static_cast<const char *>(bc.PtrC()), bc.Len());
							if(cfg_pack.FromJson(temp_buf)) {
								// Здесь можно удалить те компоненты конфигурации, которые передавать клиенту не следует
								cfg_pack.ToJson(temp_buf);
								bc.Z().Put(temp_buf.cptr(), temp_buf.Len());
								reply_tp.P.Put(SSecretTagPool::tagConfig, bc);
							}
						}
						reply_status_text = "Wellcome!";
					}
					else {
						reply_tp.StartWriting(PPSCMD_SQ_SRPREGISTER, StyloQProtocol::psubtypeReplyError);
						reply_status_text = "Something went wrong";
					}
					{
						reply_tp.P.Put(SSecretTagPool::tagReplyStatus, &reply_status, sizeof(reply_status));
						if(reply_status_text.NotEmpty()) 
							reply_tp.P.Put(SSecretTagPool::tagReplyStatusText, reply_status_text.cptr(), reply_status_text.Len()+1);
					}
					reply_tp.FinishWriting(&sess_secret);
					out_buf.Z().EncodeMime64(reply_tp.constptr(), reply_tp.GetAvailableSize());
					P_StqRtb->LastSndCmd = reply_tp.GetH().Type;
				}
				break;
			case PPSCMD_SQ_COMMAND:
				if(P_StqRtb) {
					P_StqRtb->LastRcvCmd = sp.GetH().Type;
					P_StqRtb->Other.Get(SSecretTagPool::tagClientIdent, &cli_ident);
					if(P_Ic) {
						if(P_Ic->ProcessCommand(sp, cli_ident, &sess_secret, reply_tp)) {	
							out_buf.Z().EncodeMime64(reply_tp.constptr(), reply_tp.GetAvailableSize());
							P_StqRtb->LastSndCmd = reply_tp.GetH().Type;
						}
					}
				}
				break;
			default:
				// Недопустимая команда
				break;
		}
	}
	CATCH
		; // process error
	ENDCATCH
	PushNgxResult(pReq, NGX_DONE, NGX_HTTP_OK, /*SFileFormat::Html*/SFileFormat::Unkn, /*cpUTF8*/cpUndef, out_buf);
}

void PPWorkingPipeSession::ProcessHttpRequest(ngx_http_request_t * pReq, PPServerCmd & rCmd)
{
	NgxReqResult result;
	if(pReq) {
		const NgxModule_Papyrus::Config * p_cfg = static_cast<const NgxModule_Papyrus::Config *>(ngx_http_get_module_loc_conf(pReq, ngx_http_papyrus_test_module));
		SString out_buf;
		SString temp_buf;
		SString cmd_buf;
		int    do_preprocess_content = 0;
		const PPThreadLocalArea & r_tla = DS.GetConstTLA();
		if(p_cfg && p_cfg->Speciality == NgxModule_Papyrus::Config::specialityStyloQ) {
			ProcessHttpRequest_StyloQ(pReq, rCmd);
		}
		else {
			if(r_tla.State & r_tla.stAuth) {
				PPJobSrvReply __reply;
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
					if(rCmd.ParseLine(cmd_buf, (State_PPws & stLoggedIn) ? rCmd.plfLoggedIn : 0)) {
						cmdret = ProcessCommand_(&rCmd, __reply);
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
						if(rCmd.ParseLine(cmd_buf, (State_PPws & stLoggedIn) ? rCmd.plfLoggedIn : 0)) {
							cmdret = ProcessCommand_(&rCmd, __reply);
						}
					}
				}
				{
					size_t reply_size = 0;
					pReq->SetContentType(SFileFormat::Html, cpUTF8);
					__reply.StartReading(0);
					if(__reply.CheckRepError()) {
						PPJobSrvReply::TransmitFileBlock tfb;
						if(__reply.GetH().Type == __reply.htFile) {
							__reply.Read(&tfb, sizeof(tfb));
						}
						reply_size = __reply.GetAvailableSize();
						if(do_preprocess_content) {
							temp_buf.Z().CatN(PTRCHRC(__reply.GetBuf(__reply.GetRdOffs())), reply_size);
							__reply.Z();
							PreprocessContent(temp_buf, __reply);
							reply_size = __reply.GetAvailableSize();
						}
						// @v11.1.12 {
						if(reply_size == 0) {
							__reply.WriteString(SString("Nothing to transmit :("));
							reply_size = __reply.GetAvailableSize();
						}
						// } @v11.1.12 
						out_buf.Z().CatN(__reply.GetBufC(), reply_size);
						//b = ngx_create_temp_buf(pReq->pool, reply_size);
						//ngx_chain_t out(b, 0/*just one buffer*/);
						//__reply.Read(b->pos, reply_size);
						//b->last = b->pos + reply_size;
						//b->memory = 1;
						//b->last_buf = 1; // there will be no more buffers in the request 
					}
					else {
						out_buf = "Error!";
						//reply_size = out_buf.Len();
						//b = ngx_create_temp_buf(pReq->pool, reply_size);
						//ngx_chain_t out(b, 0/*just one buffer*/);
						//__reply.Read(b->pos, reply_size);
						//b->last = b->pos + reply_size;
						//b->memory = 1;
						//b->last_buf = 1; // there will be no more buffers in the request 
					}
					//pReq->headers_out.status = NGX_HTTP_OK; // 200 status code 
					//pReq->headers_out.content_length_n = reply_size; // Get the content length of the body. 
				}
				//result.ReplyCode = NGX_DONE;
				//result.P_Req = pReq;
				//result.Chain.buf = b;
				//result.Chain.next = 0;
				//NgxPushRequestResult(&result);
				PushNgxResult(pReq, NGX_DONE, NGX_HTTP_OK, SFileFormat::Html, cpUTF8, out_buf);
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
				PushNgxResult(pReq, NGX_DONE, NGX_HTTP_OK, SFileFormat::Html, cpUTF8, out_buf);
			}
		}
	}
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
		ngx_http_request_t * p_ngx_req = static_cast<ngx_http_request_t *>(pReq);
		SString temp_buf;
		SString rtsid;
		GetHttpReqRtsId(p_ngx_req, rtsid);
		thread_count = ThreadList.GetCount(PPThread::kWorkerSession);
		while(!p_thread) {
			p_thread = static_cast<PPWorkingPipeSession *>(ThreadList.SearchByOuterSignature(PPThread::kWorkerSession, rtsid));
			if(p_thread) {
				p_thread->SetOuterSignature(rtsid);
			}
			else {
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
					PPWorkingPipeSession * p_new_sess = new PPWorkingPipeSession(P_Queue, dblblk, rtsid);
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
		P_Queue->Push(p_ngx_req);
		CALLPTRMEMB(p_thread, WakeUp());
	}
	LEAVE_CRITICAL_SECTION
	return ok;
}