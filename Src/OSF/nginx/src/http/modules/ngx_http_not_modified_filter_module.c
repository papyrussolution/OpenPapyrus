/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */
#include <ngx_config.h>
#include <ngx_core.h>
#pragma hdrstop

static ngx_uint_t ngx_http_test_if_unmodified(ngx_http_request_t * r);
static ngx_uint_t ngx_http_test_if_modified(ngx_http_request_t * r);
static ngx_uint_t ngx_http_test_if_match(ngx_http_request_t * r, ngx_table_elt_t * header, ngx_uint_t weak);
static ngx_int_t ngx_http_not_modified_filter_init(ngx_conf_t * cf);

static ngx_http_module_t ngx_http_not_modified_filter_module_ctx = {
	NULL,                              /* preconfiguration */
	ngx_http_not_modified_filter_init, /* postconfiguration */
	NULL,                              /* create main configuration */
	NULL,                              /* init main configuration */
	NULL,                              /* create server configuration */
	NULL,                              /* merge server configuration */
	NULL,                              /* create location configuration */
	NULL                               /* merge location configuration */
};

ngx_module_t ngx_http_not_modified_filter_module = {
	NGX_MODULE_V1,
	&ngx_http_not_modified_filter_module_ctx, /* module context */
	NULL,                              /* module directives */
	NGX_HTTP_MODULE,                   /* module type */
	NULL,                              /* init master */
	NULL,                              /* init module */
	NULL,                              /* init process */
	NULL,                              /* init thread */
	NULL,                              /* exit thread */
	NULL,                              /* exit process */
	NULL,                              /* exit master */
	NGX_MODULE_V1_PADDING
};

static ngx_http_output_header_filter_pt ngx_http_next_header_filter;

static ngx_int_t ngx_http_not_modified_header_filter(ngx_http_request_t * pReq)
{
	if(pReq->headers_out.status != NGX_HTTP_OK || pReq != pReq->main || pReq->disable_not_modified) {
		return ngx_http_next_header_filter(pReq);
	}
	else if(pReq->headers_in.if_unmodified_since && !ngx_http_test_if_unmodified(pReq)) {
		return ngx_http_filter_finalize_request(pReq, NULL, NGX_HTTP_PRECONDITION_FAILED);
	}
	else if(pReq->headers_in.if_match && !ngx_http_test_if_match(pReq, pReq->headers_in.if_match, 0)) {
		return ngx_http_filter_finalize_request(pReq, NULL, NGX_HTTP_PRECONDITION_FAILED);
	}
	else {
		if(pReq->headers_in.if_modified_since || pReq->headers_in.if_none_match) {
			if(!pReq->headers_in.if_modified_since || !ngx_http_test_if_modified(pReq)) {
				if(!pReq->headers_in.if_none_match || ngx_http_test_if_match(pReq, pReq->headers_in.if_none_match, 1)) {
					// not modified 
					pReq->headers_out.status = NGX_HTTP_NOT_MODIFIED;
					pReq->headers_out.status_line.len = 0;
					pReq->headers_out.content_type.len = 0;
					ngx_http_clear_content_length(pReq);
					ngx_http_clear_accept_ranges(pReq);
					if(pReq->headers_out.content_encoding) {
						pReq->headers_out.content_encoding->hash = 0;
						pReq->headers_out.content_encoding = NULL;
					}
				}
			}
		}
		return ngx_http_next_header_filter(pReq);
	}
}

static ngx_uint_t ngx_http_test_if_unmodified(ngx_http_request_t * r)
{
	time_t iums;
	if(r->headers_out.last_modified_time == (time_t)-1) {
		return 0;
	}
	iums = ngx_parse_http_time(r->headers_in.if_unmodified_since->value.data, r->headers_in.if_unmodified_since->value.len);
	ngx_log_debug2(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "http iums:%T lm:%T", iums, r->headers_out.last_modified_time);
	if(iums >= r->headers_out.last_modified_time) {
		return 1;
	}
	return 0;
}

static ngx_uint_t ngx_http_test_if_modified(ngx_http_request_t * r)
{
	time_t ims;
	ngx_http_core_loc_conf_t  * clcf;
	if(r->headers_out.last_modified_time == (time_t)-1) {
		return 1;
	}
	clcf = (ngx_http_core_loc_conf_t*)ngx_http_get_module_loc_conf(r, ngx_http_core_module);
	if(clcf->if_modified_since == NGX_HTTP_IMS_OFF) {
		return 1;
	}
	ims = ngx_parse_http_time(r->headers_in.if_modified_since->value.data, r->headers_in.if_modified_since->value.len);
	ngx_log_debug2(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "http ims:%T lm:%T", ims, r->headers_out.last_modified_time);
	if(ims == r->headers_out.last_modified_time) {
		return 0;
	}
	if(clcf->if_modified_since == NGX_HTTP_IMS_EXACT || ims < r->headers_out.last_modified_time) {
		return 1;
	}
	return 0;
}

static ngx_uint_t ngx_http_test_if_match(ngx_http_request_t * r, ngx_table_elt_t * header, ngx_uint_t weak)
{
	uchar   * start, * end, ch;
	ngx_str_t etag;
	ngx_str_t * list = &header->value;
	if(list->len == 1 && list->data[0] == '*') {
		return 1;
	}
	if(r->headers_out.etag == NULL) {
		return 0;
	}
	etag = r->headers_out.etag->value;
	ngx_log_debug2(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "http im:\"%V\" etag:%V", list, &etag);
	if(weak
	    && etag.len > 2
	    && etag.data[0] == 'W'
	    && etag.data[1] == '/') {
		etag.len -= 2;
		etag.data += 2;
	}
	start = list->data;
	end = list->data + list->len;
	while(start < end) {
		if(weak && end - start > 2 && start[0] == 'W' && start[1] == '/') {
			start += 2;
		}
		if(etag.len > (size_t)(end - start)) {
			return 0;
		}
		if(ngx_strncmp(start, etag.data, etag.len) != 0) {
			goto skip;
		}
		start += etag.len;
		while(start < end) {
			ch = *start;
			if(ch == ' ' || ch == '\t') {
				start++;
				continue;
			}
			break;
		}
		if(start == end || *start == ',') {
			return 1;
		}
skip:
		while(start < end && *start != ',') {
			start++;
		}
		while(start < end) {
			ch = *start;
			if(ch == ' ' || ch == '\t' || ch == ',') {
				start++;
				continue;
			}
			break;
		}
	}
	return 0;
}

static ngx_int_t ngx_http_not_modified_filter_init(ngx_conf_t * cf)
{
	ngx_http_next_header_filter = ngx_http_top_header_filter;
	ngx_http_top_header_filter = ngx_http_not_modified_header_filter;
	return NGX_OK;
}
