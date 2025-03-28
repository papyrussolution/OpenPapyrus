/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */
#include <ngx_config.h>
#include <ngx_core.h>
#pragma hdrstop
//#include <ngx_event.h>

#define NGX_WSABUFS  8

ssize_t ngx_wsarecv_chain(ngx_connection_t * c, ngx_chain_t * chain, nginx_off_t limit)
{
	int rc;
	size_t n;
	ngx_err_t err;
	ngx_array_t vec;
	ngx_event_t  * rev;
	WSABUF wsabufs[NGX_WSABUFS];
	uchar * prev = NULL;
	LPWSABUF wsabuf = NULL;
	ulong flags = 0;
	size_t size = 0;
	ulong bytes = 0;
	vec.elts = wsabufs;
	vec.nelts = 0;
	vec.size = sizeof(WSABUF);
	vec.nalloc = NGX_WSABUFS;
	vec.pool = c->pool;
	/* coalesce the neighbouring bufs */
	while(chain) {
		n = chain->buf->end - chain->buf->last;
		if(limit) {
			if(size >= (size_t)limit) {
				break;
			}
			if(size + n > (size_t)limit) {
				n = (size_t)limit - size;
			}
		}
		if(prev == chain->buf->last) {
			wsabuf->len += n;
		}
		else {
			wsabuf = (LPWSABUF)ngx_array_push(&vec);
			if(wsabuf == NULL) {
				return NGX_ERROR;
			}
			wsabuf->buf = (char *)chain->buf->last;
			wsabuf->len = n;
		}
		size += n;
		prev = chain->buf->end;
		chain = chain->next;
	}
	ngx_log_debug2(NGX_LOG_DEBUG_EVENT, c->log, 0, "WSARecv: %d:%d", vec.nelts, wsabuf->len);
	rc = WSARecv(c->fd, (LPWSABUF)vec.elts, vec.nelts, &bytes, &flags, NULL, NULL);
	rev = c->P_EvRd;
	if(rc == -1) {
		rev->ready = 0;
		err = ngx_socket_errno;
		if(err == WSAEWOULDBLOCK) {
			ngx_log_debug0(NGX_LOG_DEBUG_EVENT, c->log, err, "WSARecv() not ready");
			return NGX_AGAIN;
		}
		rev->error = 1;
		ngx_connection_error(c, err, "WSARecv() failed");
		return NGX_ERROR;
	}
	if(bytes < size) {
		rev->ready = 0;
	}
	if(bytes == 0) {
		rev->eof = 1;
	}
	return bytes;
}

