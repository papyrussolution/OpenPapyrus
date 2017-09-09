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

int SLAPI RunNginxServer()
{
	NgxStartUpOptions o;
	return (NgxStartUp(o) == 0) ? 1 : 0;
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
	ngx_chain_t out;
	// Set the Content-Type header. 
	pReq->headers_out.content_type.len = sizeof("text/html; charset=UTF-8") - 1;
	pReq->headers_out.content_type.data = (u_char *)"text/html; charset=UTF-8";
	{
		//pReq->headers_out.charset.len = sizeof("utf-8") - 1;
		//pReq->headers_out.charset.data = (u_char *)"utf-8";
		//SETIFZ(pReq->headers_out.override_charset, (ngx_str_t *)ngx_palloc(pReq->pool, sizeof(ngx_str_t)));
		//pReq->headers_out.override_charset->len = sizeof("utf-8") - 1;
		//pReq->headers_out.override_charset->data = (u_char *)"utf-8";
	}
	{
		// Allocate a new buffer for sending out the reply. 
		ngx_buf_t * b = (ngx_buf_t *)ngx_pcalloc(pReq->pool, sizeof(ngx_buf_t));
		// Insertion in the buffer chain. 
		out.buf = b;
		out.next = NULL; // just one buffer 
		b->pos = ngx_papyrus_test; /* first position in memory of the data */
		b->last = ngx_papyrus_test + sizeof(ngx_papyrus_test); /* last position in memory of the data */
		b->memory = 1; /* content is in read-only memory */
		b->last_buf = 1; /* there will be no more buffers in the request */
		// Sending the headers for the reply. 
		pReq->headers_out.status = NGX_HTTP_OK; // 200 status code 
		// Get the content length of the body. 
		pReq->headers_out.content_length_n = sizeof(ngx_papyrus_test);
		ngx_http_send_header(pReq); // Send the headers 
	}
	// Send the body, and return the status code of the output filter chain. 
	return ngx_http_output_filter(pReq, &out);
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
	clcf->handler = ngx_http_papyrus_test_handler;
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

static int ProcessNgxHttpRequest(ngx_http_request_t * pReq)
{
	int    ok = 1;
	return ok;
}

int SLAPI PPSession::DispatchNgxRequest(void * pReq)
{
	class NgxReqQueue : private SQueue { //req_queue(1024)
	public:
		NgxReqQueue() : SQueue(sizeof(ngx_http_request_t), 1024)
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
		int    FASTCALL Pop(ngx_http_request_t * pReq)
		{
			int    ok = 0;
			ngx_http_request_t * p_req = 0;
			Lck.Lock();
			p_req = (ngx_http_request_t *)SQueue::pop();
			Lck.Unlock();
			if(p_req) {
				if(pReq)
					memcpy(pReq, p_req, sizeof(*pReq));
				ok = 1;
			}
			return ok;
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
			ngx_http_request_t request;
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
				int    pop_r = 0; // Результат 
				if(r == WAIT_TIMEOUT) {
					pop_r = P_Queue->Pop(&request);
				}
				else if(r == wait_obj_wakeup) {
					pop_r = P_Queue->Pop(&request);
				}
				else if(r == wait_obj_localstop) {
					// @todo log
					break;
				}
				else if(r == wait_obj_stop) {
					// @todo log
					break;
				}
				if(pop_r) {
					SetIdleState();
					ProcessNgxHttpRequest(&request);
					ResetIdleState();
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