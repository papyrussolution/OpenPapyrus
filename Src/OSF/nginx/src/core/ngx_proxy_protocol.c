/*
 * Copyright (C) Roman Arutyunyan
 * Copyright (C) Nginx, Inc.
 */
#include <ngx_config.h>
#include <ngx_core.h>
#pragma hdrstop

uchar * ngx_proxy_protocol_read(ngx_connection_t * c, uchar * buf, uchar * last)
{
	uchar ch, * addr, * port;
	ngx_int_t n;
	uchar * p = buf;
	size_t len = last - buf;
	if(len < 8 || ngx_strncmp(p, "PROXY ", 6) != 0) {
		goto invalid;
	}
	p += 6;
	len -= 6;
	if(len >= 7 && ngx_strncmp(p, "UNKNOWN", 7) == 0) {
		ngx_log_debug0(NGX_LOG_DEBUG_CORE, c->log, 0, "PROXY protocol unknown protocol");
		p += 7;
		goto skip;
	}
	if(len < 5 || ngx_strncmp(p, "TCP", 3) != 0 || (p[3] != '4' && p[3] != '6') || p[4] != ' ') {
		goto invalid;
	}
	p += 5;
	addr = p;
	for(;;) {
		if(p == last) {
			goto invalid;
		}
		ch = *p++;
		if(ch == ' ') {
			break;
		}
		if(ch != ':' && ch != '.' && (ch < 'a' || ch > 'f') && (ch < 'A' || ch > 'F') && !isdec(ch)) {
			goto invalid;
		}
	}
	len = p - addr - 1;
	c->proxy_protocol_addr.data = (uchar *)ngx_pnalloc(c->pool, len);
	if(c->proxy_protocol_addr.data == NULL) {
		return NULL;
	}
	memcpy(c->proxy_protocol_addr.data, addr, len);
	c->proxy_protocol_addr.len = len;
	for(;;) {
		if(p == last) {
			goto invalid;
		}
		if(*p++ == ' ') {
			break;
		}
	}
	port = p;
	for(;;) {
		if(p == last) {
			goto invalid;
		}
		if(*p++ == ' ') {
			break;
		}
	}
	len = p - port - 1;
	n = ngx_atoi(port, len);
	if(n < 0 || n > 65535) {
		goto invalid;
	}
	c->proxy_protocol_port = (in_port_t)n;
	ngx_log_debug2(NGX_LOG_DEBUG_CORE, c->log, 0, "PROXY protocol address: %V %i", &c->proxy_protocol_addr, n);
skip:
	for(/* void */; p < last - 1; p++) {
		if(p[0] == __CR && p[1] == LF) {
			return p + 2;
		}
	}
invalid:
	ngx_log_error(NGX_LOG_ERR, c->log, 0, "broken header: \"%*s\"", (size_t)(last - buf), buf);
	return NULL;
}

uchar * ngx_proxy_protocol_write(ngx_connection_t * c, uchar * buf, uchar * last)
{
	ngx_uint_t port, lport;
	if((last - buf) < NGX_PROXY_PROTOCOL_MAX_HEADER) {
		return NULL;
	}
	if(ngx_connection_local_sockaddr(c, NULL, 0) != NGX_OK) {
		return NULL;
	}
	switch(c->sockaddr->sa_family) {
		case AF_INET:
		    buf = ngx_cpymem(buf, "PROXY TCP4 ", sizeof("PROXY TCP4 ") - 1);
		    break;
#if (NGX_HAVE_INET6)
		case AF_INET6:
		    buf = ngx_cpymem(buf, "PROXY TCP6 ", sizeof("PROXY TCP6 ") - 1);
		    break;
#endif
		default:
		    return ngx_cpymem(buf, "PROXY UNKNOWN" CRLF, sizeof("PROXY UNKNOWN" CRLF) - 1);
	}
	buf += ngx_sock_ntop(c->sockaddr, c->socklen, buf, last - buf, 0);
	*buf++ = ' ';
	buf += ngx_sock_ntop(c->local_sockaddr, c->local_socklen, buf, last - buf, 0);
	port = ngx_inet_get_port(c->sockaddr);
	lport = ngx_inet_get_port(c->local_sockaddr);
	return ngx_slprintf(buf, last, " %ui %ui" CRLF, port, lport);
}
