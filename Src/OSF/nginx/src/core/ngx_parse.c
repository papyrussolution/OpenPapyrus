/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */
#include <ngx_config.h>
#include <ngx_core.h>
#pragma hdrstop

ssize_t FASTCALL ngx_parse_size(const ngx_str_t * line)
{
	ssize_t size, scale, max;
	size_t len = line->len;
	if(!len) {
		return NGX_ERROR;
	}
	else {
		uchar unit = line->data[len-1];
		switch(unit) {
			case 'K':
			case 'k':
				len--;
				max = NGX_MAX_SIZE_T_VALUE / 1024;
				scale = 1024;
				break;
			case 'M':
			case 'm':
				len--;
				max = NGX_MAX_SIZE_T_VALUE / (1024 * 1024);
				scale = 1024 * 1024;
				break;

			default:
				max = NGX_MAX_SIZE_T_VALUE;
				scale = 1;
		}
		size = ngx_atosz(line->data, len);
		if(size == NGX_ERROR || size > max) {
			return NGX_ERROR;
		}
		size *= scale;
		return size;
	}
}

nginx_off_t FASTCALL ngx_parse_offset(const ngx_str_t * line)
{
	uchar unit;
	nginx_off_t offset, scale, max;
	size_t len = line->len;
	if(!len) {
		return NGX_ERROR;
	}
	unit = line->data[len-1];
	switch(unit) {
		case 'K':
		case 'k':
		    len--;
		    max = NGX_MAX_OFF_T_VALUE / 1024;
		    scale = 1024;
		    break;
		case 'M':
		case 'm':
		    len--;
		    max = NGX_MAX_OFF_T_VALUE / (1024 * 1024);
		    scale = 1024 * 1024;
		    break;
		case 'G':
		case 'g':
		    len--;
		    max = NGX_MAX_OFF_T_VALUE / (1024 * 1024 * 1024);
		    scale = 1024 * 1024 * 1024;
		    break;
		default:
		    max = NGX_MAX_OFF_T_VALUE;
		    scale = 1;
	}
	offset = ngx_atoof(line->data, len);
	if(offset == NGX_ERROR || offset > max) {
		return NGX_ERROR;
	}
	offset *= scale;
	return offset;
}

ngx_int_t FASTCALL ngx_parse_time(const ngx_str_t * line, ngx_uint_t is_sec)
{
	uchar * p, * last;
	ngx_int_t value, total, scale;
	ngx_int_t max, cutoff, cutlim;
	ngx_uint_t valid;
	enum {
		st_start = 0,
		st_year,
		st_month,
		st_week,
		st_day,
		st_hour,
		st_min,
		st_sec,
		st_msec,
		st_last
	} step;
	valid = 0;
	value = 0;
	total = 0;
	cutoff = NGX_MAX_INT_T_VALUE / 10;
	cutlim = NGX_MAX_INT_T_VALUE % 10;
	step = is_sec ? st_start : st_month;
	p = line->data;
	last = p + line->len;
	while(p < last) {
		if(isdec(*p)) {
			if(value >= cutoff && (value > cutoff || *p - '0' > cutlim)) {
				return NGX_ERROR;
			}
			value = value * 10 + (*p++ - '0');
			valid = 1;
			continue;
		}
		switch(*p++) {
			case 'y':
			    if(step > st_start) {
				    return NGX_ERROR;
			    }
			    step = st_year;
			    max = NGX_MAX_INT_T_VALUE / (60 * 60 * 24 * 365);
			    scale = 60 * 60 * 24 * 365;
			    break;
			case 'M':
			    if(step >= st_month) {
				    return NGX_ERROR;
			    }
			    step = st_month;
			    max = NGX_MAX_INT_T_VALUE / (60 * 60 * 24 * 30);
			    scale = 60 * 60 * 24 * 30;
			    break;
			case 'w':
			    if(step >= st_week) {
				    return NGX_ERROR;
			    }
			    step = st_week;
			    max = NGX_MAX_INT_T_VALUE / (60 * 60 * 24 * 7);
			    scale = 60 * 60 * 24 * 7;
			    break;
			case 'd':
			    if(step >= st_day) {
				    return NGX_ERROR;
			    }
			    step = st_day;
			    max = NGX_MAX_INT_T_VALUE / (60 * 60 * 24);
			    scale = 60 * 60 * 24;
			    break;
			case 'h':
			    if(step >= st_hour) {
				    return NGX_ERROR;
			    }
			    step = st_hour;
			    max = NGX_MAX_INT_T_VALUE / (60 * 60);
			    scale = 60 * 60;
			    break;
			case 'm':
			    if(p < last && *p == 's') {
				    if(is_sec || step >= st_msec) {
					    return NGX_ERROR;
				    }
				    p++;
				    step = st_msec;
				    max = NGX_MAX_INT_T_VALUE;
				    scale = 1;
				    break;
			    }
			    if(step >= st_min) {
				    return NGX_ERROR;
			    }
			    step = st_min;
			    max = NGX_MAX_INT_T_VALUE / 60;
			    scale = 60;
			    break;
			case 's':
			    if(step >= st_sec) {
				    return NGX_ERROR;
			    }
			    step = st_sec;
			    max = NGX_MAX_INT_T_VALUE;
			    scale = 1;
			    break;
			case ' ':
			    if(step >= st_sec) {
				    return NGX_ERROR;
			    }
			    step = st_last;
			    max = NGX_MAX_INT_T_VALUE;
			    scale = 1;
			    break;
			default:
			    return NGX_ERROR;
		}
		if(step != st_msec && !is_sec) {
			scale *= 1000;
			max /= 1000;
		}
		if(value > max) {
			return NGX_ERROR;
		}
		value *= scale;
		if(total > NGX_MAX_INT_T_VALUE - value) {
			return NGX_ERROR;
		}
		total += value;
		value = 0;
		while(p < last && *p == ' ') {
			p++;
		}
	}
	if(!valid) {
		return NGX_ERROR;
	}
	if(!is_sec) {
		if(value > NGX_MAX_INT_T_VALUE / 1000) {
			return NGX_ERROR;
		}
		value *= 1000;
	}
	if(total > NGX_MAX_INT_T_VALUE - value) {
		return NGX_ERROR;
	}
	return total + value;
}
