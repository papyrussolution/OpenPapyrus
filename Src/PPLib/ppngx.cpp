// PPNGX.CPP
// Copyright (c) A.Sobolev 2017
// @codepage UTF-8
// Взаимодействие с NGINX
//
#include <pp.h>
#pragma hdrstop
#include <ngx_core.h>
//#include <ngx_http.h>

int NgxStartUp(const NgxStartUpOptions & rO); // prototype
ngx_thread_value_t __stdcall ngx_worker_thread(void * data);

int SLAPI RunNginxServer()
{
	NgxStartUpOptions o;
	return (NgxStartUp(o) == 0) ? 1 : 0;
}

int SLAPI RunNginxWorker()
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
// @param r Pointer to the request structure. See http_request.h.
// @return The status of the response generation.
// 
static ngx_int_t ngx_http_papyrus_test_handler(ngx_http_request_t * pReq)
{
	// The hello world string 
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
		ngx_buf_t * b = (ngx_buf_t *)ngx_pcalloc(pReq->pool, sizeof(ngx_buf_t));
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
		DS.DispatchNgxRequest(pReq);
		return NGX_DELEGATED/*NGX_DONE*/;
	}
}
/**
 * Configuration setup function that installs the content handler.
 *
 * @param cf Module configuration structure pointer.
 * @param cmd Module directives structure pointer.
 * @param conf Module configuration structure pointer.
 * @return string Status of the configuration setup.
 */
static char * ngx_http_papyrus_test(ngx_conf_t * cf, ngx_command_t * cmd, void * conf)
{
	// Install the papyrus_test handler. 
	ngx_http_core_loc_conf_t * clcf = (ngx_http_core_loc_conf_t *)ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module); // pointer to core location configuration 
	clcf->F_HttpHandler = ngx_http_papyrus_test_handler;
	return NGX_CONF_OK;
}
/**
 * This module provided directive: hello world.
 *
 */
static ngx_command_t ngx_http_papyrus_test_commands[] = {
	{ ngx_string("papyrus_test"), // directive 
	  NGX_HTTP_LOC_CONF|NGX_CONF_NOARGS, // location context and takes no arguments
	  ngx_http_papyrus_test, // configuration setup function 
	  0, // No offset. Only one context is supported. 
	  0, // No offset when storing the module configuration on struct. 
	  NULL},
	ngx_null_command // command termination 
};
//
// The module context
//
static ngx_http_module_t ngx_http_papyrus_test_module_ctx = {
	NULL, /* preconfiguration */
	NULL, /* postconfiguration */
	NULL, /* create main configuration */
	NULL, /* init main configuration */
	NULL, /* create server configuration */
	NULL, /* merge server configuration */
	NULL, /* create location configuration */
	NULL /* merge location configuration */
};
//
// Module definition
//
ngx_module_s ngx_http_papyrus_test_module = {
	NGX_MODULE_V1,
	&ngx_http_papyrus_test_module_ctx, /* module context */
	ngx_http_papyrus_test_commands, /* module directives */
	NGX_HTTP_MODULE, /* module type */
	NULL, /* init master */
	NULL, /* init module */
	NULL, /* init process */
	NULL, /* init thread */
	NULL, /* exit thread */
	NULL, /* exit process */
	NULL, /* exit master */
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
		p_res = (NgxReqResult *)SQueue::pop();
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

static int ProcessNgxHttpRequest(ngx_http_request_t * pReq)
{
	int    ok = 1;
	if(pReq) {
		SString out_buf;
		SString temp_buf;
		char   sb[256];

		PPVersionInfo vi = DS.GetVersionInfo();
		vi.GetProductName(temp_buf);
		out_buf.Cat(temp_buf);
		vi.GetVersionText(sb, sizeof(sb));
		out_buf.Space().Cat(sb);
		vi.GetTeam(sb, sizeof(sb));
		out_buf.Space().Cat((temp_buf = sb).Transf(CTRANSF_INNER_TO_UTF8));
		out_buf.CR();
		out_buf.CatEq("Идентификатор потока", DS.GetConstTLA().GetThreadID());
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
			// Get the content length of the body. 
			pReq->headers_out.content_length_n = out_buf.Len();
			{
				NgxReqResult result;
				result.ReplyCode = NGX_DONE;
				result.P_Req = pReq;
				result.Chain.buf = b;
				result.Chain.next = 0;
				NgxPushRequestResult(&result);
			}
		}
	}
	return ok;
}

int SLAPI PPSession::DispatchNgxRequest(void * pReq)
{
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
			p_req = (ngx_http_request_t *)SQueue::pop();
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
	class PPWorkingPipeSession : public /*PPThread*/PPWorkerSession {
	public:
		SLAPI  PPWorkingPipeSession(NgxReqQueue * pReqQueue) : 
			PPWorkerSession(PPThread::kWorkerSession), 
			WakeUpEv(Evnt::modeCreateAutoReset)
		{
			P_Queue = pReqQueue;
			P_OutPipe = 0;
			InitStartupSignal();
		}
		SLAPI ~PPWorkingPipeSession()
		{
		}
		void   WakeUp()
		{
			WakeUpEv.Signal();
		}
	private:
		virtual void SLAPI Startup()
		{
			PPWorkerSession::Startup();
			SignalStartup();
		}
		virtual void SLAPI Run()
		{
			#define INTERNAL_ERR_INVALID_WAITING_MODE 0
			//
			SString s, fmt_buf, log_buf, temp_buf;
			PPJobSrvReply reply(0);
			PPServerCmd cmd;
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
					ProcessNgxHttpRequest(p_req);
					SetIdleState();
				}
			}
		}
		virtual CmdRet SLAPI ProcessCommand(PPServerCmd * pEv, PPJobSrvReply & rReply)
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
		Evnt   WakeUpEv; // Событие, которым можно разбудить поток что бы он взял запрос из очереди и обработал
		SBufferPipe * P_OutPipe; // @notowned Указатель на этот канал поток получает вместе с командой
		NgxReqQueue * P_Queue; // @notowned Указатель на очередь запросов сервера
	};

	static NgxReqQueue * P_Queue = 0;

	int    ok = -1;
	const  uint max_threads = 64;
	uint   thread_count = 0;
	ENTER_CRITICAL_SECTION
	PPWorkingPipeSession * p_thread = 0;
	SETIFZ(P_Queue, new NgxReqQueue);
	if(P_Queue) {
		thread_count = ThreadList.GetCount(PPThread::kWorkerSession);
		while(!p_thread) {
			p_thread = (PPWorkingPipeSession *)ThreadList.SearchIdle(PPThread::kWorkerSession);
			if(!p_thread) {
				if(thread_count < max_threads) {
					PPWorkingPipeSession * p_new_sess = new PPWorkingPipeSession(P_Queue);
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
		P_Queue->Push((ngx_http_request_t *)pReq);
		if(p_thread) {
			p_thread->WakeUp();
		}
	}
	LEAVE_CRITICAL_SECTION
	return ok;
}