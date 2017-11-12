/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */
#include <ngx_config.h>
#include <ngx_core.h>
#pragma hdrstop

#define NGX_CONF_BUFFER  4096

static ngx_int_t ngx_conf_add_dump(ngx_conf_t * cf, ngx_str_t * filename);
static ngx_int_t ngx_conf_handler(ngx_conf_t * cf, ngx_int_t last);
static ngx_int_t ngx_conf_read_token(ngx_conf_t * cf);
static void ngx_conf_flush_files(ngx_cycle_t * cycle);

static ngx_command_t ngx_conf_commands[] = {
	{ ngx_string("include"), NGX_ANY_CONF|NGX_CONF_TAKE1, ngx_conf_include, 0, 0, NULL },
	ngx_null_command
};

ngx_module_t ngx_conf_module = {
	NGX_MODULE_V1,
	NULL,                              /* module context */
	ngx_conf_commands,                 /* module directives */
	NGX_CONF_MODULE,                   /* module type */
	NULL,                              /* init master */
	NULL,                              /* init module */
	NULL,                              /* init process */
	NULL,                              /* init thread */
	NULL,                              /* exit thread */
	ngx_conf_flush_files,              /* exit process */
	NULL,                              /* exit master */
	NGX_MODULE_V1_PADDING
};

/* The eight fixed arguments */

static ngx_uint_t argument_number[] = {
	NGX_CONF_NOARGS,
	NGX_CONF_TAKE1,
	NGX_CONF_TAKE2,
	NGX_CONF_TAKE3,
	NGX_CONF_TAKE4,
	NGX_CONF_TAKE5,
	NGX_CONF_TAKE6,
	NGX_CONF_TAKE7
};

char * ngx_conf_param(ngx_conf_t * cf)
{
	char * rv;
	ngx_buf_t b;
	ngx_conf_file_t conf_file;
	ngx_str_t * param = &cf->cycle->conf_param;
	if(param->len == 0) {
		return NGX_CONF_OK;
	}
	memzero(&conf_file, sizeof(ngx_conf_file_t));
	memzero(&b, sizeof(ngx_buf_t));
	b.start = param->data;
	b.pos = param->data;
	b.last = param->data + param->len;
	b.end = b.last;
	b.temporary = 1;
	conf_file.file.fd = NGX_INVALID_FILE;
	conf_file.file.name.data = NULL;
	conf_file.line = 0;
	cf->conf_file = &conf_file;
	cf->conf_file->buffer = &b;
	rv = ngx_conf_parse(cf, NULL);
	cf->conf_file = NULL;
	return rv;
}

static ngx_int_t ngx_conf_add_dump(ngx_conf_t * cf, ngx_str_t * filename)
{
	nginx_off_t size;
	u_char * p;
	ngx_buf_t * buf;
	ngx_conf_dump_t  * cd;
	uint32_t hash = ngx_crc32_long(filename->data, filename->len);
	ngx_str_node_t * sn = ngx_str_rbtree_lookup(&cf->cycle->config_dump_rbtree, filename, hash);
	if(sn) {
		cf->conf_file->dump = NULL;
		return NGX_OK;
	}
	p = ngx_pstrdup(cf->cycle->pool, filename);
	if(!p) {
		return NGX_ERROR;
	}
	cd = (ngx_conf_dump_t *)ngx_array_push(&cf->cycle->config_dump);
	if(cd == NULL) {
		return NGX_ERROR;
	}
	size = ngx_file_size(&cf->conf_file->file.info);
	buf = ngx_create_temp_buf(cf->cycle->pool, (size_t)size);
	if(!buf) {
		return NGX_ERROR;
	}
	cd->name.data = p;
	cd->name.len = filename->len;
	cd->buffer = buf;
	cf->conf_file->dump = buf;
	sn = (ngx_str_node_t *)ngx_palloc(cf->temp_pool, sizeof(ngx_str_node_t));
	if(sn == NULL) {
		return NGX_ERROR;
	}
	sn->node.key = hash;
	sn->str = cd->name;
	ngx_rbtree_insert(&cf->cycle->config_dump_rbtree, &sn->node);
	return NGX_OK;
}

char * ngx_conf_parse(ngx_conf_t * cf, ngx_str_t * filename)
{
	const char * rv;
	ngx_fd_t fd;
	ngx_int_t rc;
	ngx_buf_t buf;
	ngx_conf_file_t  * prev, conf_file;
	enum {
		parse_file = 0,
		parse_block,
		parse_param
	} type;
#if (NGX_SUPPRESS_WARN)
	fd = NGX_INVALID_FILE;
	prev = NULL;
#endif
	if(filename) {
		/* open configuration file */
		fd = ngx_open_file(filename->data, NGX_FILE_RDONLY, NGX_FILE_OPEN, 0);
		if(fd == NGX_INVALID_FILE) {
			ngx_conf_log_error(NGX_LOG_EMERG, cf, ngx_errno, ngx_open_file_n " \"%s\" failed", filename->data);
			return NGX_CONF_ERROR;
		}
		prev = cf->conf_file;
		cf->conf_file = &conf_file;
		if(ngx_fd_info(fd, &cf->conf_file->file.info) == NGX_FILE_ERROR) {
			ngx_log_error(NGX_LOG_EMERG, cf->log, ngx_errno, ngx_fd_info_n " \"%s\" failed", filename->data);
		}
		cf->conf_file->buffer = &buf;
		buf.start = (u_char*)ngx_alloc(NGX_CONF_BUFFER, cf->log);
		if(buf.start == NULL) {
			goto failed;
		}
		buf.pos = buf.start;
		buf.last = buf.start;
		buf.end = buf.last + NGX_CONF_BUFFER;
		buf.temporary = 1;
		cf->conf_file->file.fd = fd;
		cf->conf_file->file.name.len = filename->len;
		cf->conf_file->file.name.data = filename->data;
		cf->conf_file->file.offset = 0;
		cf->conf_file->file.log = cf->log;
		cf->conf_file->line = 1;
		type = parse_file;
		if(ngx_dump_config__
#if (NGX_DEBUG)
		    || 1
#endif
		    ) {
			if(ngx_conf_add_dump(cf, filename) != NGX_OK) {
				goto failed;
			}
		}
		else {
			cf->conf_file->dump = NULL;
		}
	}
	else if(cf->conf_file->file.fd != NGX_INVALID_FILE) {
		type = parse_block;
	}
	else {
		type = parse_param;
	}
	for(;; ) {
		rc = ngx_conf_read_token(cf);
		/*
		 * ngx_conf_read_token() may return
		 *
		 *    NGX_ERROR             there is error
		 *    NGX_OK                the token terminated by ";" was found
		 *    NGX_CONF_BLOCK_START  the token terminated by "{" was found
		 *    NGX_CONF_BLOCK_DONE   the "}" was found
		 *    NGX_CONF_FILE_DONE    the configuration file is done
		 */
		if(rc == NGX_ERROR) {
			goto done;
		}
		if(rc == NGX_CONF_BLOCK_DONE) {
			if(type != parse_block) {
				ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "unexpected \"}\"");
				goto failed;
			}
			goto done;
		}
		if(rc == NGX_CONF_FILE_DONE) {
			if(type == parse_block) {
				ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "unexpected end of file, expecting \"}\"");
				goto failed;
			}
			goto done;
		}
		if(rc == NGX_CONF_BLOCK_START) {
			if(type == parse_param) {
				ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "block directives are not supported in -g option");
				goto failed;
			}
		}
		// rc == NGX_OK || rc == NGX_CONF_BLOCK_START 
		if(cf->handler) {
			//
			// the custom handler, i.e., that is used in the http's "types { ... }" directive
			//
			if(rc == NGX_CONF_BLOCK_START) {
				ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "unexpected \"{\"");
				goto failed;
			}
			rv = (*cf->handler)(cf, NULL, cf->handler_conf);
			if(rv == NGX_CONF_OK) {
				continue;
			}
			if(rv == NGX_CONF_ERROR) {
				goto failed;
			}
			ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, rv);
			goto failed;
		}
		rc = ngx_conf_handler(cf, rc);
		if(rc == NGX_ERROR) {
			goto failed;
		}
	}
failed:
	rc = NGX_ERROR;
done:
	if(filename) {
		if(cf->conf_file->buffer->start) {
			ngx_free(cf->conf_file->buffer->start);
		}
		if(ngx_close_file(fd) == NGX_FILE_ERROR) {
			ngx_log_error(NGX_LOG_ALERT, cf->log, ngx_errno, ngx_close_file_n " %s failed", filename->data);
			rc = NGX_ERROR;
		}
		cf->conf_file = prev;
	}
	return (rc == NGX_ERROR) ? NGX_CONF_ERROR : NGX_CONF_OK;
}

static ngx_int_t ngx_conf_handler(ngx_conf_t * cf, ngx_int_t last)
{
	const char * rv;
	void * conf, ** confp;
	ngx_uint_t i;
	const ngx_str_t * name = (const ngx_str_t *)cf->args->elts;
	ngx_uint_t found = 0;
	for(i = 0; cf->cycle->modules[i]; i++) {
		const ngx_command_t * cmd = cf->cycle->modules[i]->commands;
		if(cmd) {
			for(/* void */; cmd->Name.len; cmd++) {
				if(name->len == cmd->Name.len && sstreq(name->data, cmd->Name.data)) {
					found = 1;
					if(cf->cycle->modules[i]->type != NGX_CONF_MODULE && cf->cycle->modules[i]->type != cf->module_type) {
						continue;
					}
					// is the directive's location right ? 
					if(!(cmd->type & cf->cmd_type)) {
						continue;
					}
					if(!(cmd->type & NGX_CONF_BLOCK) && last != NGX_OK) {
						ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "directive \"%s\" is not terminated by \";\"", name->data);
						return NGX_ERROR;
					}
					if((cmd->type & NGX_CONF_BLOCK) && last != NGX_CONF_BLOCK_START) {
						ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "directive \"%s\" has no opening \"{\"", name->data);
						return NGX_ERROR;
					}
					// is the directive's argument count right ? 
					if(!(cmd->type & NGX_CONF_ANY)) {
						if(cmd->type & NGX_CONF_FLAG) {
							if(cf->args->nelts != 2) {
								goto invalid;
							}
						}
						else if(cmd->type & NGX_CONF_1MORE) {
							if(cf->args->nelts < 2) {
								goto invalid;
							}
						}
						else if(cmd->type & NGX_CONF_2MORE) {
							if(cf->args->nelts < 3) {
								goto invalid;
							}
						}
						else if(cf->args->nelts > NGX_CONF_MAX_ARGS) {
							goto invalid;
						}
						else if(!(cmd->type & argument_number[cf->args->nelts - 1])) {
							goto invalid;
						}
					}
					// set up the directive's configuration context 
					conf = NULL;
					if(cmd->type & NGX_DIRECT_CONF) {
						conf = ((void**)cf->ctx)[cf->cycle->modules[i]->index];
					}
					else if(cmd->type & NGX_MAIN_CONF) {
						conf = &(((void**)cf->ctx)[cf->cycle->modules[i]->index]);
					}
					else if(cf->ctx) {
						confp = (void **)*(void**)((char*)cf->ctx + cmd->conf); // @sobolev (void **)
						if(confp)
							conf = confp[cf->cycle->modules[i]->ctx_index];
					}
					rv = cmd->F_SetHandler(cf, cmd, conf);
					if(rv == NGX_CONF_OK)
						return NGX_OK;
					else if(rv == NGX_CONF_ERROR)
						return NGX_ERROR;
					else {
						ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "\"%s\" directive %s", name->data, rv);
						return NGX_ERROR;
					}
				}
			}
		}
	}
	if(found) {
		ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "\"%s\" directive is not allowed here", name->data);
		return NGX_ERROR;
	}
	ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "unknown directive \"%s\"", name->data);
	return NGX_ERROR;
invalid:
	ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "invalid number of arguments in \"%s\" directive", name->data);
	return NGX_ERROR;
}

static ngx_int_t ngx_conf_read_token(ngx_conf_t * cf)
{
	u_char * start, ch, * src, * dst;
	nginx_off_t file_size;
	size_t len;
	ssize_t n, size;
	ngx_uint_t start_line;
	ngx_str_t * word;
	ngx_buf_t * b, * dump;
	ngx_uint_t found = 0;
	ngx_uint_t need_space = 0;
	ngx_uint_t last_space = 1;
	ngx_uint_t sharp_comment = 0;
	ngx_uint_t variable = 0;
	ngx_uint_t quoted = 0;
	ngx_uint_t s_quoted = 0;
	ngx_uint_t d_quoted = 0;
	cf->args->nelts = 0;
	b = cf->conf_file->buffer;
	dump = cf->conf_file->dump;
	start = b->pos;
	start_line = cf->conf_file->line;
	file_size = ngx_file_size(&cf->conf_file->file.info);
	for(;; ) {
		if(b->pos >= b->last) {
			if(cf->conf_file->file.offset >= file_size) {
				if(cf->args->nelts > 0 || !last_space) {
					if(cf->conf_file->file.fd == NGX_INVALID_FILE) {
						ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "unexpected end of parameter, expecting \";\"");
						return NGX_ERROR;
					}
					ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "unexpected end of file, expecting \";\" or \"}\"");
					return NGX_ERROR;
				}
				return NGX_CONF_FILE_DONE;
			}
			len = b->pos - start;
			if(len == NGX_CONF_BUFFER) {
				cf->conf_file->line = start_line;
				if(d_quoted) {
					ch = '"';
				}
				else if(s_quoted) {
					ch = '\'';
				}
				else {
					ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "too long parameter \"%*s...\" started", 10, start);
					return NGX_ERROR;
				}
				ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "too long parameter, probably missing terminating \"%c\" character", ch);
				return NGX_ERROR;
			}
			if(len) {
				memmove(b->start, start, len);
			}
			size = (ssize_t)(file_size - cf->conf_file->file.offset);
			if(size > b->end - (b->start + len)) {
				size = b->end - (b->start + len);
			}
			n = ngx_read_file(&cf->conf_file->file, b->start + len, size, cf->conf_file->file.offset);
			if(n == NGX_ERROR) {
				return NGX_ERROR;
			}
			if(n != size) {
				ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, ngx_read_file_n " returned only %z bytes instead of %z", n, size);
				return NGX_ERROR;
			}
			b->pos = b->start + len;
			b->last = b->pos + n;
			start = b->start;
			if(dump) {
				dump->last = ngx_cpymem(dump->last, b->pos, size);
			}
		}
		ch = *b->pos++;
		if(ch == LF) {
			cf->conf_file->line++;
			if(sharp_comment) {
				sharp_comment = 0;
			}
		}
		if(sharp_comment) {
			continue;
		}
		if(quoted) {
			quoted = 0;
			continue;
		}
		if(need_space) {
			if(oneof4(ch, ' ', '\t', __CR, LF)) {
				last_space = 1;
				need_space = 0;
				continue;
			}
			if(ch == ';') {
				return NGX_OK;
			}
			if(ch == '{') {
				return NGX_CONF_BLOCK_START;
			}
			if(ch == ')') {
				last_space = 1;
				need_space = 0;
			}
			else {
				ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "unexpected \"%c\"", ch);
				return NGX_ERROR;
			}
		}
		if(last_space) {
			if(oneof4(ch, ' ', '\t', __CR, LF)) {
				continue;
			}
			start = b->pos - 1;
			start_line = cf->conf_file->line;
			switch(ch) {
				case ';':
				case '{':
				    if(cf->args->nelts == 0) {
					    ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "unexpected \"%c\"", ch);
					    return NGX_ERROR;
				    }
				    if(ch == '{') {
					    return NGX_CONF_BLOCK_START;
				    }
				    return NGX_OK;
				case '}':
				    if(cf->args->nelts != 0) {
					    ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "unexpected \"}\"");
					    return NGX_ERROR;
				    }
				    return NGX_CONF_BLOCK_DONE;
				case '#':
				    sharp_comment = 1;
				    continue;
				case '\\':
				    quoted = 1;
				    last_space = 0;
				    continue;
				case '"':
				    start++;
				    d_quoted = 1;
				    last_space = 0;
				    continue;
				case '\'':
				    start++;
				    s_quoted = 1;
				    last_space = 0;
				    continue;
				default:
				    last_space = 0;
			}
		}
		else {
			if(ch == '{' && variable) {
				continue;
			}
			variable = 0;
			if(ch == '\\') {
				quoted = 1;
				continue;
			}
			if(ch == '$') {
				variable = 1;
				continue;
			}
			if(d_quoted) {
				if(ch == '"') {
					d_quoted = 0;
					need_space = 1;
					found = 1;
				}
			}
			else if(s_quoted) {
				if(ch == '\'') {
					s_quoted = 0;
					need_space = 1;
					found = 1;
				}
			}
			else if(oneof6(ch, ' ', '\t', __CR, LF, ';', '{')) {
				last_space = 1;
				found = 1;
			}
			if(found) {
				word = (ngx_str_t*)ngx_array_push(cf->args);
				if(word == NULL) {
					return NGX_ERROR;
				}
				word->data = (u_char*)ngx_pnalloc(cf->pool, b->pos - 1 - start + 1);
				if(word->data == NULL) {
					return NGX_ERROR;
				}
				for(dst = word->data, src = start, len = 0;
				    src < b->pos - 1;
				    len++) {
					if(*src == '\\') {
						switch(src[1]) {
							case '"':
							case '\'':
							case '\\':
							    src++;
							    break;
							case 't':
							    *dst++ = '\t';
							    src += 2;
							    continue;
							case 'r':
							    *dst++ = '\r';
							    src += 2;
							    continue;
							case 'n':
							    *dst++ = '\n';
							    src += 2;
							    continue;
						}
					}
					*dst++ = *src++;
				}
				*dst = '\0';
				word->len = len;
				if(ch == ';') {
					return NGX_OK;
				}
				if(ch == '{') {
					return NGX_CONF_BLOCK_START;
				}
				found = 0;
			}
		}
	}
}

const char * ngx_conf_include(ngx_conf_t * cf, const ngx_command_t * cmd, void * conf) // F_SetHandler
{
	const char * rv;
	ngx_int_t n;
	ngx_str_t name;
	ngx_glob_t gl;
	ngx_str_t * value = (ngx_str_t*)cf->args->elts;
	ngx_str_t file = value[1];
	ngx_log_debug1(NGX_LOG_DEBUG_CORE, cf->log, 0, "include %s", file.data);
	if(ngx_conf_full_name(cf->cycle, &file, 1) != NGX_OK) {
		return NGX_CONF_ERROR;
	}
	if(strpbrk((char*)file.data, "*?[") == NULL) {
		ngx_log_debug1(NGX_LOG_DEBUG_CORE, cf->log, 0, "include %s", file.data);
		return ngx_conf_parse(cf, &file);
	}
	memzero(&gl, sizeof(ngx_glob_t));
	gl.pattern = file.data;
	gl.log = cf->log;
	gl.test = 1;
	if(ngx_open_glob(&gl) != NGX_OK) {
		ngx_conf_log_error(NGX_LOG_EMERG, cf, ngx_errno, ngx_open_glob_n " \"%s\" failed", file.data);
		return NGX_CONF_ERROR;
	}
	rv = NGX_CONF_OK;
	for(;; ) {
		n = ngx_read_glob(&gl, &name);
		if(n != NGX_OK) {
			break;
		}
		file.len = name.len++;
		file.data = ngx_pstrdup(cf->pool, &name);
		if(file.data == NULL) {
			return NGX_CONF_ERROR;
		}
		ngx_log_debug1(NGX_LOG_DEBUG_CORE, cf->log, 0, "include %s", file.data);
		rv = ngx_conf_parse(cf, &file);
		if(rv != NGX_CONF_OK) {
			break;
		}
	}
	ngx_close_glob(&gl);
	return rv;
}

ngx_int_t ngx_conf_full_name(ngx_cycle_t * cycle, ngx_str_t * name, ngx_uint_t conf_prefix)
{
	ngx_str_t * prefix = conf_prefix ? &cycle->conf_prefix : &cycle->prefix;
	return ngx_get_full_name(cycle->pool, prefix, name);
}

ngx_open_file_t * FASTCALL ngx_conf_open_file(ngx_cycle_t * cycle, const ngx_str_t * name)
{
	ngx_str_t full;
	ngx_uint_t i;
	ngx_list_part_t * part;
	ngx_open_file_t * file;
#if (NGX_SUPPRESS_WARN)
	ngx_str_null(&full);
#endif
	if(name->len) {
		full = *name;
		if(ngx_conf_full_name(cycle, &full, 0) != NGX_OK) {
			return NULL;
		}
		part = &cycle->open_files.part;
		file = (ngx_open_file_t *)part->elts;
		for(i = 0; /* void */; i++) {
			if(i >= part->nelts) {
				if(part->next == NULL) {
					break;
				}
				part = part->next;
				file = (ngx_open_file_t *)part->elts;
				i = 0;
			}
			if(full.len == file[i].name.len && sstreq(full.data, file[i].name.data)) {
				return &file[i];
			}
		}
	}
	file = (ngx_open_file_t *)ngx_list_push(&cycle->open_files);
	if(file) {
		if(name->len) {
			file->fd = NGX_INVALID_FILE;
			file->name = full;
		}
		else {
			file->fd = ngx_stderr;
			file->name = *name;
		}
		file->flush = NULL;
		file->data = NULL;
	}
	return file;
}

static void ngx_conf_flush_files(ngx_cycle_t * cycle)
{
	ngx_uint_t i;
	ngx_list_part_t  * part;
	ngx_open_file_t  * file;
	ngx_log_debug0(NGX_LOG_DEBUG_CORE, cycle->log, 0, "flush files");
	part = &cycle->open_files.part;
	file = (ngx_open_file_t *)part->elts;
	for(i = 0; /* void */; i++) {
		if(i >= part->nelts) {
			if(part->next == NULL) {
				break;
			}
			part = part->next;
			file = (ngx_open_file_t *)part->elts;
			i = 0;
		}
		if(file[i].flush) {
			file[i].flush(&file[i], cycle->log);
		}
	}
}

void ngx_cdecl ngx_conf_log_error(ngx_uint_t level, ngx_conf_t * cf, ngx_err_t err, const char * fmt, ...)
{
	u_char errstr[NGX_MAX_CONF_ERRSTR], * p;
	va_list args;
	u_char * last = errstr + NGX_MAX_CONF_ERRSTR;
	va_start(args, fmt);
	p = ngx_vslprintf(errstr, last, fmt, args);
	va_end(args);
	if(err) {
		p = ngx_log_errno(p, last, err);
	}
	if(cf->conf_file == NULL)
		ngx_log_error(level, cf->log, 0, "%*s", p - errstr, errstr);
	else if(cf->conf_file->file.fd == NGX_INVALID_FILE)
		ngx_log_error(level, cf->log, 0, "%*s in command line", p - errstr, errstr);
	else
		ngx_log_error(level, cf->log, 0, "%*s in %s:%ui", p - errstr, errstr, cf->conf_file->file.name.data, cf->conf_file->line);
}

const char * ngx_conf_set_flag_slot(ngx_conf_t * cf, const ngx_command_t * cmd, void * conf) // F_SetHandler
{
	char * p = (char *)conf;
	ngx_flag_t * fp = (ngx_flag_t *)(p + cmd->offset);
	if(*fp != NGX_CONF_UNSET) {
		return "is duplicate";
	}
	else {
		const ngx_str_t * value = (const ngx_str_t *)cf->args->elts;
		if(sstreqi_ascii(value[1].data, (u_char*)"on")) {
			*fp = 1;
		}
		else if(sstreqi_ascii(value[1].data, (u_char*)"off")) {
			*fp = 0;
		}
		else {
			ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "invalid value \"%s\" in \"%s\" directive, it must be \"on\" or \"off\"", value[1].data, cmd->Name.data);
			return NGX_CONF_ERROR;
		}
		if(cmd->P_Post) {
			ngx_conf_post_t * post = (ngx_conf_post_t *)cmd->P_Post;
			return post->post_handler(cf, post, fp);
		}
		return NGX_CONF_OK;
	}
}

const char * ngx_conf_set_str_slot(ngx_conf_t * cf, const ngx_command_t * cmd, void * conf) // F_SetHandler
{
	char  * p = (char *)conf;
	ngx_str_t * field = (ngx_str_t*)(p + cmd->offset);
	if(field->data) {
		return "is duplicate";
	}
	else {
		const ngx_str_t * value = (const ngx_str_t *)cf->args->elts;
		*field = value[1];
		if(cmd->P_Post) {
			ngx_conf_post_t * post = (ngx_conf_post_t *)cmd->P_Post;
			return post->post_handler(cf, post, field);
		}
		else
			return NGX_CONF_OK;
	}
}

const char * ngx_conf_set_str_array_slot(ngx_conf_t * cf, const ngx_command_t * cmd, void * conf) // F_SetHandler
{
	char * p = (char *)conf;
	ngx_str_t * value, * s;
	ngx_array_t ** a = (ngx_array_t**)(p + cmd->offset);
	if(*a == NGX_CONF_UNSET_PTR) {
		*a = ngx_array_create(cf->pool, 4, sizeof(ngx_str_t));
		if(*a == NULL) {
			return NGX_CONF_ERROR;
		}
	}
	s = (ngx_str_t*)ngx_array_push(*a);
	if(s == NULL) {
		return NGX_CONF_ERROR;
	}
	value = (ngx_str_t*)cf->args->elts;
	*s = value[1];
	if(cmd->P_Post) {
		ngx_conf_post_t * post = (ngx_conf_post_t *)cmd->P_Post;
		return post->post_handler(cf, post, s);
	}
	return NGX_CONF_OK;
}

const char * ngx_conf_set_keyval_slot(ngx_conf_t * cf, const ngx_command_t * cmd, void * conf) // F_SetHandler
{
	char * p = (char *)conf;
	ngx_str_t  * value;
	ngx_keyval_t * kv;
	ngx_array_t ** a = (ngx_array_t**)(p + cmd->offset);
	if(*a == NULL) {
		*a = ngx_array_create(cf->pool, 4, sizeof(ngx_keyval_t));
		if(*a == NULL) {
			return NGX_CONF_ERROR;
		}
	}
	kv = (ngx_keyval_t *)ngx_array_push(*a);
	if(kv == NULL) {
		return NGX_CONF_ERROR;
	}
	value = (ngx_str_t*)cf->args->elts;
	kv->key = value[1];
	kv->value = value[2];
	if(cmd->P_Post) {
		ngx_conf_post_t * post = (ngx_conf_post_t *)cmd->P_Post;
		return post->post_handler(cf, post, kv);
	}
	else
		return NGX_CONF_OK;
}

const char * ngx_conf_set_num_slot(ngx_conf_t * cf, const ngx_command_t * cmd, void * conf) // F_SetHandler
{
	char * p = (char *)conf;
	ngx_int_t * np = (ngx_int_t*)(p + cmd->offset);
	if(*np != NGX_CONF_UNSET) {
		return "is duplicate";
	}
	else {
		const ngx_str_t * value = (ngx_str_t*)cf->args->elts;
		*np = ngx_atoi(value[1].data, value[1].len);
		if(*np == NGX_ERROR) {
			return "invalid number";
		}
		if(cmd->P_Post) {
			ngx_conf_post_t * post = (ngx_conf_post_t *)cmd->P_Post;
			return post->post_handler(cf, post, np);
		}
		else
			return NGX_CONF_OK;
	}
}

const char * ngx_conf_set_size_slot(ngx_conf_t * cf, const ngx_command_t * cmd, void * conf) // F_SetHandler
{
	char * p = (char*)conf;
	size_t * sp = (size_t*)(p + cmd->offset);
	if(*sp != NGX_CONF_UNSET_SIZE) {
		return "is duplicate";
	}
	else {
		const ngx_str_t * value = (const ngx_str_t *)cf->args->elts;
		*sp = ngx_parse_size(&value[1]);
		if(*sp == (size_t)NGX_ERROR) {
			return "invalid value";
		}
		else if(cmd->P_Post) {
			ngx_conf_post_t * post = (ngx_conf_post_t *)cmd->P_Post;
			return post->post_handler(cf, post, sp);
		}
		else
			return NGX_CONF_OK;
	}
}

const char * ngx_conf_set_off_slot(ngx_conf_t * cf, const ngx_command_t * cmd, void * conf) // F_SetHandler
{
	char  * p = (char*)conf;
	nginx_off_t * op = (nginx_off_t*)(p + cmd->offset);
	if(*op != NGX_CONF_UNSET) {
		return "is duplicate";
	}
	else {
		const ngx_str_t * value = (const ngx_str_t *)cf->args->elts;
		*op = ngx_parse_offset(&value[1]);
		if(*op == (nginx_off_t)NGX_ERROR) {
			return "invalid value";
		}
		else if(cmd->P_Post) {
			ngx_conf_post_t * post = (ngx_conf_post_t *)cmd->P_Post;
			return post->post_handler(cf, post, op);
		}
		else
			return NGX_CONF_OK;
	}
}

const char * ngx_conf_set_msec_slot(ngx_conf_t * cf, const ngx_command_t * cmd, void * conf) // F_SetHandler
{
	char  * p = (char*)conf;
	ngx_msec_t * msp = (ngx_msec_t*)(p + cmd->offset);
	if(*msp != NGX_CONF_UNSET_MSEC) {
		return "is duplicate";
	}
	else {
		const ngx_str_t * value = (const ngx_str_t *)cf->args->elts;
		*msp = ngx_parse_time(&value[1], 0);
		if(*msp == (ngx_msec_t)NGX_ERROR) {
			return "invalid value";
		}
		if(cmd->P_Post) {
			ngx_conf_post_t * post = (ngx_conf_post_t *)cmd->P_Post;
			return post->post_handler(cf, post, msp);
		}
		else
			return NGX_CONF_OK;
	}
}

const char * ngx_conf_set_sec_slot(ngx_conf_t * cf, const ngx_command_t * cmd, void * conf) // F_SetHandler
{
	char  * p = (char *)conf;
	time_t * sp = (time_t*)(p + cmd->offset);
	if(*sp != NGX_CONF_UNSET) {
		return "is duplicate";
	}
	else {
		ngx_str_t * value = (ngx_str_t*)cf->args->elts;
		*sp = ngx_parse_time(&value[1], 1);
		if(*sp == (time_t)NGX_ERROR) {
			return "invalid value";
		}
		else if(cmd->P_Post) {
			ngx_conf_post_t * post = (ngx_conf_post_t *)cmd->P_Post;
			return post->post_handler(cf, post, sp);
		}
		else
			return NGX_CONF_OK;
	}
}

const char * ngx_conf_set_bufs_slot(ngx_conf_t * cf, const ngx_command_t * cmd, void * conf) // F_SetHandler
{
	char * p = (char*)conf;
	ngx_bufs_t  * bufs = (ngx_bufs_t*)(p + cmd->offset);
	if(bufs->num) {
		return "is duplicate";
	}
	else {
		ngx_str_t * value = (ngx_str_t*)cf->args->elts;
		bufs->num = ngx_atoi(value[1].data, value[1].len);
		if(bufs->num == NGX_ERROR || bufs->num == 0) {
			return "invalid value";
		}
		bufs->size = ngx_parse_size(&value[2]);
		if(bufs->size == (size_t)NGX_ERROR || bufs->size == 0) {
			return "invalid value";
		}
		return NGX_CONF_OK;
	}
}

const char * ngx_conf_set_enum_slot(ngx_conf_t * cf, const ngx_command_t * cmd, void * conf) // F_SetHandler
{
	char  * p = (char*)conf;
	ngx_uint_t * np = (ngx_uint_t*)(p + cmd->offset);
	if(*np != NGX_CONF_UNSET_UINT) {
		return "is duplicate";
	}
	else {
		ngx_str_t * value = (ngx_str_t*)cf->args->elts;
		ngx_conf_enum_t * e = (ngx_conf_enum_t  *)cmd->P_Post;
		for(ngx_uint_t i = 0; e[i].name.len != 0; i++) {
			if(e[i].name.len == value[1].len && sstreqi_ascii(e[i].name.data, value[1].data)) {
				*np = e[i].value;
				return NGX_CONF_OK;
			}
		}
		ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "invalid value \"%s\"", value[1].data);
		return NGX_CONF_ERROR;
	}
}

const char * ngx_conf_set_bitmask_slot(ngx_conf_t * cf, const ngx_command_t * cmd, void * conf) // F_SetHandler
{
	char * p = (char *)conf;
	ngx_uint_t i, m;
	ngx_uint_t * np = (ngx_uint_t*)(p + cmd->offset);
	ngx_str_t  * value = (ngx_str_t*)cf->args->elts;
	ngx_conf_bitmask_t * mask = (ngx_conf_bitmask_t  *)cmd->P_Post;
	for(i = 1; i < cf->args->nelts; i++) {
		for(m = 0; mask[m].name.len != 0; m++) {
			if(mask[m].name.len == value[i].len && sstreqi_ascii(mask[m].name.data, value[i].data)) {
				if(*np & mask[m].mask) {
					ngx_conf_log_error(NGX_LOG_WARN, cf, 0, "duplicate value \"%s\"", value[i].data);
				}
				else {
					*np |= mask[m].mask;
				}
				break;
			}
		}
		if(mask[m].name.len == 0) {
			ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "invalid value \"%s\"", value[i].data);
			return NGX_CONF_ERROR;
		}
	}
	return NGX_CONF_OK;
}

#if 0
char * ngx_conf_unsupported(ngx_conf_t * cf, ngx_command_t * cmd, void * conf)
{
	return "unsupported on this platform";
}
#endif

char * ngx_conf_deprecated(ngx_conf_t * cf, void * post, void * data)
{
	ngx_conf_deprecated_t  * d = (ngx_conf_deprecated_t  *)post;
	ngx_conf_log_error(NGX_LOG_WARN, cf, 0, "the \"%s\" directive is deprecated, use the \"%s\" directive instead", d->old_name, d->new_name);
	return NGX_CONF_OK;
}

char * ngx_conf_check_num_bounds(ngx_conf_t * cf, void * post, void * data)
{
	ngx_conf_num_bounds_t  * bounds = (ngx_conf_num_bounds_t  *)post;
	ngx_int_t  * np = (ngx_int_t *)data;
	if(bounds->high == -1) {
		if(*np >= bounds->low) {
			return NGX_CONF_OK;
		}
		ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "value must be equal to or greater than %i", bounds->low);
		return NGX_CONF_ERROR;
	}
	if(*np >= bounds->low && *np <= bounds->high) {
		return NGX_CONF_OK;
	}
	ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "value must be between %i and %i", bounds->low, bounds->high);
	return NGX_CONF_ERROR;
}
