/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */
#include <ngx_config.h>
#include <ngx_core.h>
#pragma hdrstop
//#include <ngx_event.h>

ssize_t ngx_udp_wsarecv(ngx_connection_t * c, uchar * buf, size_t size)
{
	int rc;
	ulong bytes, flags;
	WSABUF wsabuf[1];
	ngx_err_t err;
	ngx_event_t  * rev;
	wsabuf[0].buf = (char *)buf;
	wsabuf[0].len = size;
	flags = 0;
	bytes = 0;
	rc = WSARecv(c->fd, wsabuf, 1, &bytes, &flags, NULL, NULL);
	ngx_log_debug4(NGX_LOG_DEBUG_EVENT, c->log, 0, "WSARecv: fd:%d rc:%d %ul of %z", c->fd, rc, bytes, size);
	rev = c->P_EvRd;
	if(rc == -1) {
		rev->ready = 0;
		err = ngx_socket_errno;
		if(err == WSAEWOULDBLOCK) {
			ngx_log_debug0(NGX_LOG_DEBUG_EVENT, c->log, err, "WSARecv() not ready");
			return NGX_AGAIN;
		}
		else {
			rev->error = 1;
			ngx_connection_error(c, err, "WSARecv() failed");
			return NGX_ERROR;
		}
	}
	else
		return bytes;
}

ssize_t ngx_udp_overlapped_wsarecv(ngx_connection_t * c, uchar * buf, size_t size)
{
	int rc;
	ulong bytes, flags;
	ngx_event_t * rev = c->P_EvRd;
	if(!rev->ready) {
		ngx_log_error(NGX_LOG_ALERT, c->log, 0, "second wsa post");
		return NGX_AGAIN;
	}
	else {
		ngx_log_debug1(NGX_LOG_DEBUG_EVENT, c->log, 0, "rev->complete: %d", rev->complete);
		if(rev->complete) {
			rev->complete = 0;
			if(ngx_event_flags & NGX_USE_IOCP_EVENT) {
				if(rev->ovlp.error) {
					ngx_connection_error(c, rev->ovlp.error, "WSARecv() failed");
					return NGX_ERROR;
				}
				else {
					ngx_log_debug3(NGX_LOG_DEBUG_EVENT, c->log, 0, "WSARecv ovlp: fd:%d %ul of %z", c->fd, rev->available, size);
					return rev->available;
				}
			}
			else if(WSAGetOverlappedResult(c->fd, (LPWSAOVERLAPPED)&rev->ovlp, &bytes, 0, NULL) == 0) {
				ngx_connection_error(c, ngx_socket_errno, "WSARecv() or WSAGetOverlappedResult() failed");
				return NGX_ERROR;
			}
			else {
				ngx_log_debug3(NGX_LOG_DEBUG_EVENT, c->log, 0, "WSARecv: fd:%d %ul of %z", c->fd, bytes, size);
				return bytes;
			}
		}
		else {
			WSABUF wsabuf[1];
			LPWSAOVERLAPPED ovlp = (LPWSAOVERLAPPED)&rev->ovlp;
			memzero(ovlp, sizeof(WSAOVERLAPPED));
			wsabuf[0].buf = (char *)buf;
			wsabuf[0].len = size;
			flags = 0;
			bytes = 0;
			rc = WSARecv(c->fd, wsabuf, 1, &bytes, &flags, ovlp, NULL);
			rev->complete = 0;
			ngx_log_debug4(NGX_LOG_DEBUG_EVENT, c->log, 0, "WSARecv ovlp: fd:%d rc:%d %ul of %z", c->fd, rc, bytes, size);
			if(rc == -1) {
				ngx_err_t err = ngx_socket_errno;
				if(err == WSA_IO_PENDING) {
					rev->active = 1;
					ngx_log_debug0(NGX_LOG_DEBUG_EVENT, c->log, err, "WSARecv() posted");
					return NGX_AGAIN;
				}
				else {
					rev->error = 1;
					ngx_connection_error(c, err, "WSARecv() failed");
					return NGX_ERROR;
				}
			}
			else if(ngx_event_flags & NGX_USE_IOCP_EVENT) {
				/*
				 * if a socket was bound with I/O completion port
				 * then GetQueuedCompletionStatus() would anyway return its status
				 * despite that WSARecv() was already complete
				 */
				rev->active = 1;
				return NGX_AGAIN;
			}
			else {
				rev->active = 0;
				return bytes;
			}
		}
	}
}

