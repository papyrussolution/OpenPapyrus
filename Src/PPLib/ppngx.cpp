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