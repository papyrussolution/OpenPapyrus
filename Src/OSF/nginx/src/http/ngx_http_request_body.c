/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */
#include <ngx_config.h>
#include <ngx_core.h>
#pragma hdrstop

static void ngx_http_read_client_request_body_handler(ngx_http_request_t * r);
/* @sobolev static*/ngx_int_t ngx_http_do_read_client_request_body(ngx_http_request_t * r);
static ngx_int_t ngx_http_write_request_body(ngx_http_request_t * r);
static ngx_int_t ngx_http_read_discarded_request_body(ngx_http_request_t * r);
static ngx_int_t ngx_http_discard_request_body_filter(ngx_http_request_t * r, ngx_buf_t * b);
static ngx_int_t ngx_http_test_expect(ngx_http_request_t * r);
static ngx_int_t ngx_http_request_body_filter(ngx_http_request_t * r, ngx_chain_t * in);
static ngx_int_t ngx_http_request_body_length_filter(ngx_http_request_t * r, ngx_chain_t * in);
static ngx_int_t ngx_http_request_body_chunked_filter(ngx_http_request_t * r, ngx_chain_t * in);

ngx_int_t ngx_http_read_client_request_body(ngx_http_request_t * pReq, ngx_http_client_body_handler_pt post_handler)
{
	size_t preread;
	ssize_t size;
	ngx_int_t rc;
	ngx_buf_t * b;
	ngx_http_request_body_t * rb;
	ngx_http_core_loc_conf_t  * clcf;
	pReq->main->count++;
	if(pReq != pReq->main || pReq->request_body || pReq->discard_body) {
		pReq->request_body_no_buffering = 0;
		post_handler(pReq);
		return NGX_OK;
	}
	if(ngx_http_test_expect(pReq) != NGX_OK) {
		rc = NGX_HTTP_INTERNAL_SERVER_ERROR;
		goto done;
	}
	rb = (ngx_http_request_body_t *)ngx_pcalloc(pReq->pool, sizeof(ngx_http_request_body_t));
	if(rb == NULL) {
		rc = NGX_HTTP_INTERNAL_SERVER_ERROR;
		goto done;
	}
	/*
	 * set by ngx_pcalloc():
	 *
	 *   rb->bufs = NULL;
	 *   rb->buf = NULL;
	 *   rb->free = NULL;
	 *   rb->busy = NULL;
	 *   rb->chunked = NULL;
	 */
	rb->rest = -1;
	rb->post_handler = post_handler;
	pReq->request_body = rb;
	if(pReq->headers_in.content_length_n < 0 && !pReq->headers_in.chunked) {
		pReq->request_body_no_buffering = 0;
		post_handler(pReq);
		return NGX_OK;
	}
#if (NGX_HTTP_V2)
	if(pReq->stream) {
		rc = ngx_http_v2_read_request_body(pReq);
		goto done;
	}
#endif
	preread = pReq->header_in->last - pReq->header_in->pos;
	if(preread) {
		// there is the pre-read part of the request body 
		ngx_log_debug1(NGX_LOG_DEBUG_HTTP, pReq->connection->log, 0, "http client request body preread %uz", preread);
		ngx_chain_t out(pReq->header_in, 0);
		//out.buf = pReq->header_in;
		//out.next = NULL;
		rc = ngx_http_request_body_filter(pReq, &out);
		if(rc != NGX_OK) {
			goto done;
		}
		pReq->request_length += preread - (pReq->header_in->last - pReq->header_in->pos);
		if(!pReq->headers_in.chunked && rb->rest > 0 && rb->rest <= (nginx_off_t)(pReq->header_in->end - pReq->header_in->last)) {
			/* the whole request body may be placed in r->header_in */
			b = (ngx_buf_t*)ngx_calloc_buf(pReq->pool);
			if(!b) {
				rc = NGX_HTTP_INTERNAL_SERVER_ERROR;
				goto done;
			}
			b->temporary = 1;
			b->start = pReq->header_in->pos;
			b->pos = pReq->header_in->pos;
			b->last = pReq->header_in->last;
			b->end = pReq->header_in->end;
			rb->buf = b;
			pReq->read_event_handler = ngx_http_read_client_request_body_handler;
			pReq->write_event_handler = ngx_http_request_empty_handler;
			rc = ngx_http_do_read_client_request_body(pReq);
			goto done;
		}
	}
	else {
		// set rb->rest 
		if(ngx_http_request_body_filter(pReq, NULL) != NGX_OK) {
			rc = NGX_HTTP_INTERNAL_SERVER_ERROR;
			goto done;
		}
	}
	if(rb->rest == 0) {
		// the whole request body was pre-read 
		pReq->request_body_no_buffering = 0;
		post_handler(pReq);
		return NGX_OK;
	}
	if(rb->rest < 0) {
		ngx_log_error(NGX_LOG_ALERT, pReq->connection->log, 0, "negative request body rest");
		rc = NGX_HTTP_INTERNAL_SERVER_ERROR;
		goto done;
	}
	clcf = (ngx_http_core_loc_conf_t *)ngx_http_get_module_loc_conf(pReq, ngx_http_core_module);
	size = clcf->client_body_buffer_size;
	size += size >> 2;
	// @todo honor r->request_body_in_single_buf 
	if(!pReq->headers_in.chunked && rb->rest < size) {
		size = (ssize_t)rb->rest;
		if(pReq->request_body_in_single_buf) {
			size += preread;
		}
	}
	else {
		size = clcf->client_body_buffer_size;
	}
	rb->buf = ngx_create_temp_buf(pReq->pool, size);
	if(rb->buf == NULL) {
		rc = NGX_HTTP_INTERNAL_SERVER_ERROR;
		goto done;
	}
	pReq->read_event_handler = ngx_http_read_client_request_body_handler;
	pReq->write_event_handler = ngx_http_request_empty_handler;
	rc = ngx_http_do_read_client_request_body(pReq);
done:
	if(pReq->request_body_no_buffering && oneof2(rc, NGX_OK, NGX_AGAIN)) {
		if(rc == NGX_OK)
			pReq->request_body_no_buffering = 0;
		else // rc == NGX_AGAIN 
			pReq->reading_body = 1;
		pReq->read_event_handler = ngx_http_block_reading;
		post_handler(pReq);
	}
	if(rc >= NGX_HTTP_SPECIAL_RESPONSE) {
		pReq->main->count--;
	}
	return rc;
}

ngx_int_t ngx_http_read_unbuffered_request_body(ngx_http_request_t * r)
{
	ngx_int_t rc;
#if (NGX_HTTP_V2)
	if(r->stream) {
		rc = ngx_http_v2_read_unbuffered_request_body(r);
		if(rc == NGX_OK) {
			r->reading_body = 0;
		}
		return rc;
	}
#endif
	if(r->connection->P_EvRd->timedout) {
		r->connection->timedout = 1;
		return NGX_HTTP_REQUEST_TIME_OUT;
	}
	rc = ngx_http_do_read_client_request_body(r);
	if(rc == NGX_OK) {
		r->reading_body = 0;
	}
	return rc;
}

static void ngx_http_read_client_request_body_handler(ngx_http_request_t * r)
{
	ngx_int_t rc;
	if(r->connection->P_EvRd->timedout) {
		r->connection->timedout = 1;
		ngx_http_finalize_request(r, NGX_HTTP_REQUEST_TIME_OUT);
		return;
	}
	rc = ngx_http_do_read_client_request_body(r);
	if(rc >= NGX_HTTP_SPECIAL_RESPONSE) {
		ngx_http_finalize_request(r, rc);
	}
}

/* @sobolev static*/ngx_int_t ngx_http_do_read_client_request_body(ngx_http_request_t * r)
{
	nginx_off_t rest;
	size_t size;
	ssize_t n;
	ngx_int_t rc;
	ngx_http_core_loc_conf_t  * clcf;
	ngx_connection_t * c = r->connection;
	ngx_http_request_body_t * rb = r->request_body;
	ngx_log_debug0(NGX_LOG_DEBUG_HTTP, c->log, 0, "http read client request body");
	for(;;) {
		for(;;) {
			if(rb->buf->last == rb->buf->end) {
				if(rb->buf->pos != rb->buf->last) {
					// pass buffer to request body filter chain 
					ngx_chain_t out(rb->buf, 0);
					//out.buf = rb->buf;
					//out.next = NULL;
					rc = ngx_http_request_body_filter(r, &out);
					if(rc != NGX_OK) {
						return rc;
					}
				}
				else {
					// update chains 
					rc = ngx_http_request_body_filter(r, NULL);
					if(rc != NGX_OK) {
						return rc;
					}
				}
				if(rb->busy) {
					if(r->request_body_no_buffering) {
						if(c->P_EvRd->timer_set) {
							ngx_del_timer(c->P_EvRd);
						}
						if(ngx_handle_read_event(c->P_EvRd, 0) != NGX_OK) {
							return NGX_HTTP_INTERNAL_SERVER_ERROR;
						}
						return NGX_AGAIN;
					}
					return NGX_HTTP_INTERNAL_SERVER_ERROR;
				}
				rb->buf->pos = rb->buf->start;
				rb->buf->last = rb->buf->start;
			}
			{
				size = rb->buf->end - rb->buf->last;
				rest = rb->rest - (rb->buf->last - rb->buf->pos); 
				if((nginx_off_t)size > rest) {
					size = (size_t)rest;
				}
				n = c->recv(c, rb->buf->last, size);
				ngx_log_debug1(NGX_LOG_DEBUG_HTTP, c->log, 0, "http client request body recv %z", n);
				if(n == NGX_AGAIN) {
					break;
				}
				if(n == 0) {
					ngx_log_error(NGX_LOG_INFO, c->log, 0, "client prematurely closed connection");
				}
				if(n == 0 || n == NGX_ERROR) {
					c->error = 1;
					return NGX_HTTP_BAD_REQUEST;
				}
				rb->buf->last += n;
				r->request_length += n;
				if(n == rest) {
					// pass buffer to request body filter chain 
					ngx_chain_t out(rb->buf, 0);
					//out.buf = rb->buf;
					//out.next = NULL;
					rc = ngx_http_request_body_filter(r, &out);
					if(rc != NGX_OK) {
						return rc;
					}
				}
				if(rb->rest == 0) {
					break;
				}
				if(rb->buf->last < rb->buf->end) {
					break;
				}
			}
		}
		ngx_log_debug1(NGX_LOG_DEBUG_HTTP, c->log, 0, "http client request body rest %O", rb->rest);
		if(rb->rest == 0) {
			break;
		}
		if(!c->P_EvRd->ready) {
			if(r->request_body_no_buffering && rb->buf->pos != rb->buf->last) {
				// pass buffer to request body filter chain 
				ngx_chain_t out(rb->buf, 0);
				//out.buf = rb->buf;
				//out.next = NULL;
				rc = ngx_http_request_body_filter(r, &out);
				if(rc != NGX_OK) {
					return rc;
				}
			}
			clcf = (ngx_http_core_loc_conf_t *)ngx_http_get_module_loc_conf(r, ngx_http_core_module);
			ngx_add_timer(c->P_EvRd, clcf->client_body_timeout);
			if(ngx_handle_read_event(c->P_EvRd, 0) != NGX_OK) {
				return NGX_HTTP_INTERNAL_SERVER_ERROR;
			}
			return NGX_AGAIN;
		}
	}
	if(c->P_EvRd->timer_set) {
		ngx_del_timer(c->P_EvRd);
	}
	if(!r->request_body_no_buffering) {
		r->read_event_handler = ngx_http_block_reading;
		rb->post_handler(r);
	}
	return NGX_OK;
}

static ngx_int_t ngx_http_write_request_body(ngx_http_request_t * r)
{
	ssize_t n;
	ngx_chain_t * cl, * ln;
	ngx_temp_file_t * tf;
	ngx_http_core_loc_conf_t  * clcf;
	ngx_http_request_body_t * rb = r->request_body;
	ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "http write client request body, bufs %p", rb->bufs);
	if(rb->temp_file == NULL) {
		tf = (ngx_temp_file_t *)ngx_pcalloc(r->pool, sizeof(ngx_temp_file_t));
		if(tf == NULL) {
			return NGX_ERROR;
		}
		clcf = (ngx_http_core_loc_conf_t *)ngx_http_get_module_loc_conf(r, ngx_http_core_module);
		tf->file.fd = NGX_INVALID_FILE;
		tf->file.log = r->connection->log;
		tf->path = clcf->client_body_temp_path;
		tf->pool = r->pool;
		tf->warn = "a client request body is buffered to a temporary file";
		tf->log_level = r->request_body_file_log_level;
		tf->persistent = r->request_body_in_persistent_file;
		tf->clean = r->request_body_in_clean_file;
		if(r->request_body_file_group_access) {
			tf->access = 0660;
		}
		rb->temp_file = tf;
		if(rb->bufs == NULL) {
			/* empty body with r->request_body_in_file_only */
			if(ngx_create_temp_file(&tf->file, tf->path, tf->pool, tf->persistent, tf->clean, tf->access) != NGX_OK) {
				return NGX_ERROR;
			}
			return NGX_OK;
		}
	}
	if(rb->bufs == NULL) {
		return NGX_OK;
	}
	n = ngx_write_chain_to_temp_file(rb->temp_file, rb->bufs);
	/* @todo n == 0 or not complete and level event */
	if(n == NGX_ERROR) {
		return NGX_ERROR;
	}
	rb->temp_file->offset += n;
	/* mark all buffers as written */
	for(cl = rb->bufs; cl; /* void */) {
		cl->buf->pos = cl->buf->last;
		ln = cl;
		cl = cl->next;
		ngx_free_chain(r->pool, ln);
	}
	rb->bufs = NULL;
	return NGX_OK;
}

ngx_int_t ngx_http_discard_request_body(ngx_http_request_t * r)
{
	ssize_t size;
	ngx_int_t rc;
	ngx_event_t  * rev;
	if(r != r->main || r->discard_body || r->request_body) {
		return NGX_OK;
	}
#if (NGX_HTTP_V2)
	if(r->stream) {
		r->stream->skip_data = 1;
		return NGX_OK;
	}
#endif
	if(ngx_http_test_expect(r) != NGX_OK) {
		return NGX_HTTP_INTERNAL_SERVER_ERROR;
	}
	rev = r->connection->P_EvRd;
	ngx_log_debug0(NGX_LOG_DEBUG_HTTP, rev->log, 0, "http set discard body");
	if(rev->timer_set) {
		ngx_del_timer(rev);
	}
	if(r->headers_in.content_length_n <= 0 && !r->headers_in.chunked) {
		return NGX_OK;
	}
	size = r->header_in->last - r->header_in->pos;
	if(size || r->headers_in.chunked) {
		rc = ngx_http_discard_request_body_filter(r, r->header_in);
		if(rc != NGX_OK) {
			return rc;
		}
		if(r->headers_in.content_length_n == 0) {
			return NGX_OK;
		}
	}
	rc = ngx_http_read_discarded_request_body(r);
	if(rc == NGX_OK) {
		r->lingering_close = 0;
		return NGX_OK;
	}
	if(rc >= NGX_HTTP_SPECIAL_RESPONSE) {
		return rc;
	}
	/* rc == NGX_AGAIN */
	r->read_event_handler = ngx_http_discarded_request_body_handler;
	if(ngx_handle_read_event(rev, 0) != NGX_OK) {
		return NGX_HTTP_INTERNAL_SERVER_ERROR;
	}
	r->count++;
	r->discard_body = 1;
	return NGX_OK;
}

void ngx_http_discarded_request_body_handler(ngx_http_request_t * r)
{
	ngx_int_t rc;
	ngx_msec_t timer;
	ngx_http_core_loc_conf_t  * clcf;
	ngx_connection_t * c = r->connection;
	ngx_event_t * rev = c->P_EvRd;
	if(rev->timedout) {
		c->timedout = 1;
		c->error = 1;
		ngx_http_finalize_request(r, NGX_ERROR);
		return;
	}
	if(r->lingering_time) {
		timer = (ngx_msec_t)r->lingering_time - (ngx_msec_t)ngx_time();
		if((ngx_msec_int_t)timer <= 0) {
			r->discard_body = 0;
			r->lingering_close = 0;
			ngx_http_finalize_request(r, NGX_ERROR);
			return;
		}
	}
	else {
		timer = 0;
	}
	rc = ngx_http_read_discarded_request_body(r);
	if(rc == NGX_OK) {
		r->discard_body = 0;
		r->lingering_close = 0;
		ngx_http_finalize_request(r, NGX_DONE);
		return;
	}
	if(rc >= NGX_HTTP_SPECIAL_RESPONSE) {
		c->error = 1;
		ngx_http_finalize_request(r, NGX_ERROR);
		return;
	}
	/* rc == NGX_AGAIN */
	if(ngx_handle_read_event(rev, 0) != NGX_OK) {
		c->error = 1;
		ngx_http_finalize_request(r, NGX_ERROR);
		return;
	}
	if(timer) {
		clcf = (ngx_http_core_loc_conf_t *)ngx_http_get_module_loc_conf(r, ngx_http_core_module);
		timer *= 1000;
		if(timer > clcf->lingering_timeout) {
			timer = clcf->lingering_timeout;
		}
		ngx_add_timer(rev, timer);
	}
}

static ngx_int_t ngx_http_read_discarded_request_body(ngx_http_request_t * r)
{
	size_t size;
	ssize_t n;
	ngx_int_t rc;
	ngx_buf_t b;
	uchar buffer[NGX_HTTP_DISCARD_BUFFER_SIZE];
	ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "http read discarded body");
	memzero(&b, sizeof(ngx_buf_t));
	b.temporary = 1;
	for(;;) {
		if(r->headers_in.content_length_n == 0) {
			r->read_event_handler = ngx_http_block_reading;
			return NGX_OK;
		}
		if(!r->connection->P_EvRd->ready) {
			return NGX_AGAIN;
		}
		size = (size_t)MIN(r->headers_in.content_length_n, NGX_HTTP_DISCARD_BUFFER_SIZE);
		n = r->connection->recv(r->connection, buffer, size);
		if(n == NGX_ERROR) {
			r->connection->error = 1;
			return NGX_OK;
		}
		if(n == NGX_AGAIN) {
			return NGX_AGAIN;
		}
		if(n == 0) {
			return NGX_OK;
		}
		b.pos = buffer;
		b.last = buffer + n;
		rc = ngx_http_discard_request_body_filter(r, &b);
		if(rc != NGX_OK) {
			return rc;
		}
	}
}

static ngx_int_t ngx_http_discard_request_body_filter(ngx_http_request_t * r, ngx_buf_t * b)
{
	size_t size;
	ngx_int_t rc;
	ngx_http_request_body_t  * rb;
	if(r->headers_in.chunked) {
		rb = r->request_body;
		if(rb == NULL) {
			rb = (ngx_http_request_body_t *)ngx_pcalloc(r->pool, sizeof(ngx_http_request_body_t));
			if(rb == NULL) {
				return NGX_HTTP_INTERNAL_SERVER_ERROR;
			}
			rb->chunked = (ngx_http_chunked_t *)ngx_pcalloc(r->pool, sizeof(ngx_http_chunked_t));
			if(rb->chunked == NULL) {
				return NGX_HTTP_INTERNAL_SERVER_ERROR;
			}
			r->request_body = rb;
		}
		for(;;) {
			rc = ngx_http_parse_chunked(r, b, rb->chunked);
			if(rc == NGX_OK) {
				// a chunk has been parsed successfully 
				size = b->last - b->pos;
				if((nginx_off_t)size > rb->chunked->size) {
					b->pos += (size_t)rb->chunked->size;
					rb->chunked->size = 0;
				}
				else {
					rb->chunked->size -= size;
					b->pos = b->last;
				}
				continue;
			}
			if(rc == NGX_DONE) {
				// a whole response has been parsed successfully 
				r->headers_in.content_length_n = 0;
				break;
			}
			if(rc == NGX_AGAIN) {
				// set amount of data we want to see next time 
				r->headers_in.content_length_n = rb->chunked->length;
				break;
			}
			// invalid 
			ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "client sent invalid chunked body");
			return NGX_HTTP_BAD_REQUEST;
		}
	}
	else {
		size = b->last - b->pos;
		if((nginx_off_t)size > r->headers_in.content_length_n) {
			b->pos += (size_t)r->headers_in.content_length_n;
			r->headers_in.content_length_n = 0;
		}
		else {
			b->pos = b->last;
			r->headers_in.content_length_n -= size;
		}
	}
	return NGX_OK;
}

static ngx_int_t ngx_http_test_expect(ngx_http_request_t * r)
{
	ngx_int_t n;
	ngx_str_t  * expect;
	if(r->expect_tested || r->headers_in.expect == NULL || r->http_version < NGX_HTTP_VERSION_11
#if (NGX_HTTP_V2)
	   || r->stream != NULL
#endif
	    ) {
		return NGX_OK;
	}
	r->expect_tested = 1;
	expect = &r->headers_in.expect->value;
	if(expect->len != sizeof("100-continue") - 1 || ngx_strncasecmp(expect->data, (uchar *)"100-continue", sizeof("100-continue") - 1) != 0) {
		return NGX_OK;
	}
	ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "send 100 Continue");
	n = r->connection->send(r->connection, (uchar *)"HTTP/1.1 100 Continue" CRLF CRLF, sizeof("HTTP/1.1 100 Continue" CRLF CRLF) - 1);
	if(n == sizeof("HTTP/1.1 100 Continue" CRLF CRLF) - 1) {
		return NGX_OK;
	}
	// we assume that such small packet should be send successfully 
	r->connection->error = 1;
	return NGX_ERROR;
}

static ngx_int_t ngx_http_request_body_filter(ngx_http_request_t * r, ngx_chain_t * in)
{
	return r->headers_in.chunked ? ngx_http_request_body_chunked_filter(r, in) : ngx_http_request_body_length_filter(r, in);
}

static ngx_int_t ngx_http_request_body_length_filter(ngx_http_request_t * r, ngx_chain_t * in)
{
	size_t size;
	ngx_int_t rc;
	ngx_buf_t * b;
	ngx_chain_t * cl, * tl, * out, ** ll;
	ngx_http_request_body_t * rb = r->request_body;
	if(rb->rest == -1) {
		ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "http request body content length filter");
		rb->rest = r->headers_in.content_length_n;
	}
	out = NULL;
	ll = &out;
	for(cl = in; cl; cl = cl->next) {
		if(rb->rest == 0) {
			break;
		}
		tl = ngx_chain_get_free_buf(r->pool, &rb->free);
		if(tl == NULL) {
			return NGX_HTTP_INTERNAL_SERVER_ERROR;
		}
		b = tl->buf;
		memzero(b, sizeof(ngx_buf_t));
		b->temporary = 1;
		b->tag = (ngx_buf_tag_t)&ngx_http_read_client_request_body;
		b->start = cl->buf->pos;
		b->pos = cl->buf->pos;
		b->last = cl->buf->last;
		b->end = cl->buf->end;
		b->flush = r->request_body_no_buffering;
		size = cl->buf->last - cl->buf->pos;
		if((nginx_off_t)size < rb->rest) {
			cl->buf->pos = cl->buf->last;
			rb->rest -= size;
		}
		else {
			cl->buf->pos += (size_t)rb->rest;
			rb->rest = 0;
			b->last = cl->buf->pos;
			b->last_buf = 1;
		}
		*ll = tl;
		ll = &tl->next;
	}
	rc = ngx_http_top_request_body_filter(r, out);
	ngx_chain_update_chains(r->pool, &rb->free, &rb->busy, &out, (ngx_buf_tag_t)&ngx_http_read_client_request_body);
	return rc;
}

static ngx_int_t ngx_http_request_body_chunked_filter(ngx_http_request_t * r, ngx_chain_t * in)
{
	size_t size;
	ngx_int_t rc;
	ngx_buf_t * b;
	ngx_chain_t * cl, * out, * tl, ** ll;
	ngx_http_core_loc_conf_t  * clcf;
	ngx_http_request_body_t * rb = r->request_body;
	if(rb->rest == -1) {
		ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "http request body chunked filter");
		rb->chunked = (ngx_http_chunked_t *)ngx_pcalloc(r->pool, sizeof(ngx_http_chunked_t));
		if(rb->chunked == NULL) {
			return NGX_HTTP_INTERNAL_SERVER_ERROR;
		}
		r->headers_in.content_length_n = 0;
		rb->rest = 3;
	}
	out = NULL;
	ll = &out;
	for(cl = in; cl; cl = cl->next) {
		for(;;) {
			ngx_log_debug7(NGX_LOG_DEBUG_EVENT, r->connection->log, 0, "http body chunked buf t:%d f:%d %p, pos %p, size: %z file: %O, size: %O",
			    cl->buf->temporary, cl->buf->in_file, cl->buf->start, cl->buf->pos, cl->buf->last - cl->buf->pos, cl->buf->file_pos,
			    cl->buf->file_last - cl->buf->file_pos);
			rc = ngx_http_parse_chunked(r, cl->buf, rb->chunked);
			if(rc == NGX_OK) {
				// a chunk has been parsed successfully 
				clcf = (ngx_http_core_loc_conf_t *)ngx_http_get_module_loc_conf(r, ngx_http_core_module);
				if(clcf->client_max_body_size && (clcf->client_max_body_size - r->headers_in.content_length_n) < rb->chunked->size) {
					ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "client intended to send too large chunked body: %O+%O bytes", r->headers_in.content_length_n, rb->chunked->size);
					r->lingering_close = 1;
					return NGX_HTTP_REQUEST_ENTITY_TOO_LARGE;
				}
				tl = ngx_chain_get_free_buf(r->pool, &rb->free);
				if(tl == NULL) {
					return NGX_HTTP_INTERNAL_SERVER_ERROR;
				}
				b = tl->buf;
				memzero(b, sizeof(ngx_buf_t));
				b->temporary = 1;
				b->tag = (ngx_buf_tag_t)&ngx_http_read_client_request_body;
				b->start = cl->buf->pos;
				b->pos = cl->buf->pos;
				b->last = cl->buf->last;
				b->end = cl->buf->end;
				b->flush = r->request_body_no_buffering;
				*ll = tl;
				ll = &tl->next;
				size = cl->buf->last - cl->buf->pos;
				if((nginx_off_t)size > rb->chunked->size) {
					cl->buf->pos += (size_t)rb->chunked->size;
					r->headers_in.content_length_n += rb->chunked->size;
					rb->chunked->size = 0;
				}
				else {
					rb->chunked->size -= size;
					r->headers_in.content_length_n += size;
					cl->buf->pos = cl->buf->last;
				}
				b->last = cl->buf->pos;
				continue;
			}
			if(rc == NGX_DONE) {
				// a whole response has been parsed successfully 
				rb->rest = 0;
				tl = ngx_chain_get_free_buf(r->pool, &rb->free);
				if(tl == NULL) {
					return NGX_HTTP_INTERNAL_SERVER_ERROR;
				}
				b = tl->buf;
				memzero(b, sizeof(ngx_buf_t));
				b->last_buf = 1;
				*ll = tl;
				ll = &tl->next;
				break;
			}
			if(rc == NGX_AGAIN) {
				// set rb->rest, amount of data we want to see next time 
				rb->rest = rb->chunked->length;
				break;
			}
			// invalid 
			ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "client sent invalid chunked body");
			return NGX_HTTP_BAD_REQUEST;
		}
	}
	rc = ngx_http_top_request_body_filter(r, out);
	ngx_chain_update_chains(r->pool, &rb->free, &rb->busy, &out, (ngx_buf_tag_t)&ngx_http_read_client_request_body);
	return rc;
}

ngx_int_t ngx_http_request_body_save_filter(ngx_http_request_t * r, ngx_chain_t * in)
{
	ngx_buf_t * b;
	ngx_chain_t * cl;
	ngx_http_request_body_t * rb = r->request_body;
#if (NGX_DEBUG)
#if 0
	for(cl = rb->bufs; cl; cl = cl->next) {
		ngx_log_debug7(NGX_LOG_DEBUG_EVENT, r->connection->log, 0,
		    "http body old buf t:%d f:%d %p, pos %p, size: %z "
		    "file: %O, size: %O",
		    cl->buf->temporary, cl->buf->in_file,
		    cl->buf->start, cl->buf->pos,
		    cl->buf->last - cl->buf->pos,
		    cl->buf->file_pos,
		    cl->buf->file_last - cl->buf->file_pos);
	}
#endif

	for(cl = in; cl; cl = cl->next) {
		ngx_log_debug7(NGX_LOG_DEBUG_EVENT, r->connection->log, 0,
		    "http body new buf t:%d f:%d %p, pos %p, size: %z file: %O, size: %O",
		    cl->buf->temporary, cl->buf->in_file,
		    cl->buf->start, cl->buf->pos,
		    cl->buf->last - cl->buf->pos,
		    cl->buf->file_pos,
		    cl->buf->file_last - cl->buf->file_pos);
	}
#endif
	/* @todo coalesce neighbouring buffers */
	if(ngx_chain_add_copy(r->pool, &rb->bufs, in) != NGX_OK) {
		return NGX_HTTP_INTERNAL_SERVER_ERROR;
	}
	if(r->request_body_no_buffering) {
		return NGX_OK;
	}
	if(rb->rest > 0) {
		if(rb->buf && rb->buf->last == rb->buf->end && ngx_http_write_request_body(r) != NGX_OK) {
			return NGX_HTTP_INTERNAL_SERVER_ERROR;
		}
		return NGX_OK;
	}
	/* rb->rest == 0 */
	if(rb->temp_file || r->request_body_in_file_only) {
		if(ngx_http_write_request_body(r) != NGX_OK) {
			return NGX_HTTP_INTERNAL_SERVER_ERROR;
		}
		if(rb->temp_file->file.offset != 0) {
			cl = ngx_chain_get_free_buf(r->pool, &rb->free);
			if(cl == NULL) {
				return NGX_HTTP_INTERNAL_SERVER_ERROR;
			}
			b = cl->buf;
			memzero(b, sizeof(ngx_buf_t));
			b->in_file = 1;
			b->file_last = rb->temp_file->file.offset;
			b->file = &rb->temp_file->file;
			rb->bufs = cl;
		}
	}
	return NGX_OK;
}
