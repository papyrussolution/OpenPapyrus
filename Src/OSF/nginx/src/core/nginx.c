/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */
#include <ngx_config.h>
#include <ngx_core.h>
#pragma hdrstop
//#include <nginx.h>

//static void ngx_show_version_info(void);
static ngx_int_t ngx_add_inherited_sockets(ngx_cycle_t * cycle);
static void ngx_cleanup_environment(void * data);
//static ngx_int_t ngx_get_options(int argc, char * const * argv);
//static ngx_int_t ngx_process_options(ngx_cycle_t * cycle);
//static ngx_int_t ngx_save_argv(ngx_cycle_t * cycle, int argc, char * const * argv);
static void * ngx_core_module_create_conf(ngx_cycle_t * cycle);
static const char * ngx_core_module_init_conf(ngx_cycle_t * cycle, void * conf);
static const char * ngx_set_user(ngx_conf_t * cf, const ngx_command_t * cmd, void * conf); // F_SetHandler
static const char * ngx_set_env(ngx_conf_t * cf, const ngx_command_t * cmd, void * conf); // F_SetHandler
static const char * ngx_set_priority(ngx_conf_t * cf, const ngx_command_t * cmd, void * conf); // F_SetHandler
static const char * ngx_set_cpu_affinity(ngx_conf_t * cf, const ngx_command_t * cmd, void * conf); // F_SetHandler
static const char * ngx_set_worker_processes(ngx_conf_t * cf, const ngx_command_t * cmd, void * conf); // F_SetHandler
static const char * ngx_load_module(ngx_conf_t * cf, const ngx_command_t * cmd, void * conf); // F_SetHandler
#if (NGX_HAVE_DLOPEN)
	static void ngx_unload_module(void * data);
#endif

static ngx_conf_enum_t ngx_debug_points[] = {
	{ ngx_string("stop"), NGX_DEBUG_POINTS_STOP },
	{ ngx_string("abort"), NGX_DEBUG_POINTS_ABORT },
	{ ngx_null_string, 0 }
};

static ngx_command_t ngx_core_commands[] = {
	{ ngx_string("daemon"),                  NGX_MAIN_CONF|NGX_DIRECT_CONF|NGX_CONF_FLAG,   ngx_conf_set_flag_slot, 0, offsetof(ngx_core_conf_t, daemon), NULL },
	{ ngx_string("master_process"),          NGX_MAIN_CONF|NGX_DIRECT_CONF|NGX_CONF_FLAG,   ngx_conf_set_flag_slot, 0, offsetof(ngx_core_conf_t, master), NULL },
	{ ngx_string("timer_resolution"),        NGX_MAIN_CONF|NGX_DIRECT_CONF|NGX_CONF_TAKE1,  ngx_conf_set_msec_slot, 0, offsetof(ngx_core_conf_t, timer_resolution), NULL },
	{ ngx_string("pid"),                     NGX_MAIN_CONF|NGX_DIRECT_CONF|NGX_CONF_TAKE1,  ngx_conf_set_str_slot, 0, offsetof(ngx_core_conf_t, pid), NULL },
	{ ngx_string("lock_file"),               NGX_MAIN_CONF|NGX_DIRECT_CONF|NGX_CONF_TAKE1,  ngx_conf_set_str_slot, 0, offsetof(ngx_core_conf_t, lock_file), NULL },
	{ ngx_string("worker_processes"),        NGX_MAIN_CONF|NGX_DIRECT_CONF|NGX_CONF_TAKE1,  ngx_set_worker_processes, 0, 0, NULL },
	{ ngx_string("debug_points"),            NGX_MAIN_CONF|NGX_DIRECT_CONF|NGX_CONF_TAKE1,  ngx_conf_set_enum_slot, 0, offsetof(ngx_core_conf_t, debug_points), &ngx_debug_points },
	{ ngx_string("user"),                    NGX_MAIN_CONF|NGX_DIRECT_CONF|NGX_CONF_TAKE12, ngx_set_user, 0, 0, NULL },
	{ ngx_string("worker_priority"),         NGX_MAIN_CONF|NGX_DIRECT_CONF|NGX_CONF_TAKE1,  ngx_set_priority, 0, 0, NULL },
	{ ngx_string("worker_cpu_affinity"),     NGX_MAIN_CONF|NGX_DIRECT_CONF|NGX_CONF_1MORE,  ngx_set_cpu_affinity, 0, 0, NULL },
	{ ngx_string("worker_rlimit_nofile"),    NGX_MAIN_CONF|NGX_DIRECT_CONF|NGX_CONF_TAKE1,  ngx_conf_set_num_slot, 0, offsetof(ngx_core_conf_t, rlimit_nofile), NULL },
	{ ngx_string("worker_rlimit_core"),      NGX_MAIN_CONF|NGX_DIRECT_CONF|NGX_CONF_TAKE1,  ngx_conf_set_off_slot, 0, offsetof(ngx_core_conf_t, rlimit_core), NULL },
	{ ngx_string("worker_shutdown_timeout"), NGX_MAIN_CONF|NGX_DIRECT_CONF|NGX_CONF_TAKE1,  ngx_conf_set_msec_slot, 0, offsetof(ngx_core_conf_t, shutdown_timeout), NULL },
	{ ngx_string("working_directory"),       NGX_MAIN_CONF|NGX_DIRECT_CONF|NGX_CONF_TAKE1,  ngx_conf_set_str_slot, 0, offsetof(ngx_core_conf_t, working_directory), NULL },
	{ ngx_string("env"),                     NGX_MAIN_CONF|NGX_DIRECT_CONF|NGX_CONF_TAKE1,  ngx_set_env, 0, 0, NULL },
	{ ngx_string("load_module"),             NGX_MAIN_CONF|NGX_DIRECT_CONF|NGX_CONF_TAKE1,  ngx_load_module, 0, 0, NULL },
	ngx_null_command
};

static ngx_core_module_t ngx_core_module_ctx = {
	ngx_string("core"),
	ngx_core_module_create_conf,
	ngx_core_module_init_conf
};

ngx_module_t ngx_core_module = {
	NGX_MODULE_V1,
	&ngx_core_module_ctx,              /* module context */
	ngx_core_commands,                 /* module directives */
	NGX_CORE_MODULE,                   /* module type */
	NULL,                              /* init master */
	NULL,                              /* init module */
	NULL,                              /* init process */
	NULL,                              /* init thread */
	NULL,                              /* exit thread */
	NULL,                              /* exit process */
	NULL,                              /* exit master */
	NGX_MODULE_V1_PADDING
};

//static ngx_uint_t ngx_show_help;
//static ngx_uint_t ngx_show_version;
//static ngx_uint_t ngx_show_configure;
//static u_char * ngx_prefix;
//static u_char * ngx_conf_file;
//static u_char * ngx_conf_params;
//static char * ngx_signal;
static char  ** ngx_os_environ;

NgxStartUpOptions::NgxStartUpOptions() : Flags(0), SigID(0)
{
}

const char * NgxStartUpOptions::GetSignalText() const
{
	switch(SigID) {
		case sigStop: return "stop";
		case sigQuit: return "quit";
		case sigReOpen: return "reopen";
		case sigReLoad: return "reload";
		default: return 0;
	}
}

int NgxStartUpOptions::SetSignalString(const char * pSig)
{
	int    ok = 1;
	if(sstreqi_ascii(pSig, "stop"))
		SigID = sigStop;
	else if(sstreqi_ascii(pSig, "quit"))
		SigID = sigQuit;
	else if(sstreqi_ascii(pSig, "reopen"))
		SigID = sigReOpen;
	else if(sstreqi_ascii(pSig, "reload"))
		SigID = sigReLoad;
	else {
		SigID = 0;
		ok = 0;
	}
	return ok;
}

int NgxStartUpOptions::ProcessCmdLine(int argc, const char * argv[])
{
	for(int i = 1; i < argc; i++) {
		const char * p = argv[i];
		if(*p++ != '-') {
			ngx_log_stderr(0, "invalid option: \"%s\"", argv[i]);
			return NGX_ERROR;
		}
		while(*p) {
			switch(*p++) {
				case '?':
				case 'h':
					Flags |= (fShowVer|fShowHelp);
				    //ngx_show_version = 1;
				    //ngx_show_help = 1;
				    break;
				case 'v':
					Flags |= fShowVer;
				    //ngx_show_version = 1;
				    break;
				case 'V':
					Flags |= (fShowVer|fShowConf);
				    //ngx_show_version = 1;
				    //ngx_show_configure = 1;
				    break;
				case 't':
					Flags |= fTestConf;
				    //ngx_test_config = 1;
				    break;
				case 'T':
					Flags |= (fTestConf|fDumpConf);
				    //ngx_test_config = 1;
				    //ngx_dump_config = 1;
				    break;
				case 'q':
					Flags |= fQuietMode;
				    //ngx_quiet_mode = 1;
				    break;
				case 'p':
				    if(*p) {
						Prefix = p;
					    //ngx_prefix = p;
					    goto next;
				    }
				    else if(argv[++i]) {
						Prefix = argv[i];
					    //ngx_prefix = (u_char *)argv[i];
					    goto next;
				    }
					else { 
						ngx_log_stderr(0, "option \"-p\" requires directory name");
						return NGX_ERROR;
					}
					break;
				case 'c':
				    if(*p) {
						ConfFile = p;
					    //ngx_conf_file = p;
					    goto next;
				    }
				    else if(argv[++i]) {
						ConfFile = argv[i];
					    //ngx_conf_file = (u_char*)argv[i];
					    goto next;
				    }
					else {
						ngx_log_stderr(0, "option \"-c\" requires file name");
						return NGX_ERROR;
					}
					break;
				case 'g':
				    if(*p) {
						ConfParams = p;
					    //ngx_conf_params = p;
					    goto next;
				    }
				    else if(argv[++i]) {
						ConfParams = argv[i];
					    //ngx_conf_params = (u_char*)argv[i];
					    goto next;
				    }
					else {
						ngx_log_stderr(0, "option \"-g\" requires parameter");
						return NGX_ERROR;
					}
					break;
				case 's':
					{
						SString signal;
						if(*p) {
							signal = p;
							//ngx_signal = (char*)p;
						}
						else if(argv[++i]) {
							signal = argv[i];
							//ngx_signal = argv[i];
						}
						if(signal.NotEmptyS()) {
							if(SetSignalString(signal)) {
								ngx_process = NGX_PROCESS_SIGNALLER;
								goto next;
							}
							else {
								ngx_log_stderr(0, "invalid option: \"-s %s\"", signal.cptr());
								return NGX_ERROR;
							}
						}
						else {
							ngx_log_stderr(0, "option \"-s\" requires parameter");
							return NGX_ERROR;
						}
						/*
						if(sstreq(ngx_signal, "stop") || sstreq(ngx_signal, "quit") || sstreq(ngx_signal, "reopen") || sstreq(ngx_signal, "reload")) {
							ngx_process = NGX_PROCESS_SIGNALLER;
							goto next;
						}
						ngx_log_stderr(0, "invalid option: \"-s %s\"", ngx_signal);
						return NGX_ERROR;
						*/
					}
					break;
				default:
				    ngx_log_stderr(0, "invalid option: \"%c\"", *(p - 1));
				    return NGX_ERROR;
			}
		}
next:
		continue;
	}
	return NGX_OK;
}

#if 0 // {
static ngx_int_t ngx_get_options(int argc, char * const * argv)
{
	for(ngx_int_t i = 1; i < argc; i++) {
		u_char * p = (u_char*)argv[i];
		if(*p++ != '-') {
			ngx_log_stderr(0, "invalid option: \"%s\"", argv[i]);
			return NGX_ERROR;
		}
		while(*p) {
			switch(*p++) {
				case '?':
				case 'h':
				    ngx_show_version = 1;
				    ngx_show_help = 1;
				    break;
				case 'v':
				    ngx_show_version = 1;
				    break;
				case 'V':
				    ngx_show_version = 1;
				    ngx_show_configure = 1;
				    break;
				case 't':
				    ngx_test_config = 1;
				    break;
				case 'T':
				    ngx_test_config = 1;
				    ngx_dump_config = 1;
				    break;
				case 'q':
				    ngx_quiet_mode = 1;
				    break;
				case 'p':
				    if(*p) {
					    ngx_prefix = p;
					    goto next;
				    }
				    if(argv[++i]) {
					    ngx_prefix = (u_char*)argv[i];
					    goto next;
				    }
				    ngx_log_stderr(0, "option \"-p\" requires directory name");
				    return NGX_ERROR;
				case 'c':
				    if(*p) {
					    ngx_conf_file = p;
					    goto next;
				    }
				    if(argv[++i]) {
					    ngx_conf_file = (u_char*)argv[i];
					    goto next;
				    }
				    ngx_log_stderr(0, "option \"-c\" requires file name");
				    return NGX_ERROR;
				case 'g':
				    if(*p) {
					    ngx_conf_params = p;
					    goto next;
				    }
				    if(argv[++i]) {
					    ngx_conf_params = (u_char*)argv[i];
					    goto next;
				    }
				    ngx_log_stderr(0, "option \"-g\" requires parameter");
				    return NGX_ERROR;
				case 's':
				    if(*p) {
					    ngx_signal = (char*)p;
				    }
				    else if(argv[++i]) {
					    ngx_signal = argv[i];
				    }
				    else {
					    ngx_log_stderr(0, "option \"-s\" requires parameter");
					    return NGX_ERROR;
				    }
				    if(sstreq(ngx_signal, "stop") || sstreq(ngx_signal, "quit") || sstreq(ngx_signal, "reopen") || sstreq(ngx_signal, "reload")) {
					    ngx_process = NGX_PROCESS_SIGNALLER;
					    goto next;
				    }
				    ngx_log_stderr(0, "invalid option: \"-s %s\"", ngx_signal);
				    return NGX_ERROR;
				default:
				    ngx_log_stderr(0, "invalid option: \"%c\"", *(p - 1));
				    return NGX_ERROR;
			}
		}
next:
		continue;
	}
	return NGX_OK;
}
#endif // } 0

ngx_int_t ngx_signal_process(ngx_cycle_t * pCycle, const char * pSig)
{
	ssize_t n;
	ngx_pid_t pid;
	ngx_file_t file;
	ngx_core_conf_t  * ccf;
	u_char buf[NGX_INT64_LEN + 2];
	ngx_log_error(NGX_LOG_NOTICE, pCycle->log, 0, "signal process started");
	ccf = (ngx_core_conf_t*)ngx_get_conf(pCycle->conf_ctx, ngx_core_module);
	memzero(&file, sizeof(ngx_file_t));
	file.name = ccf->pid;
	file.log = pCycle->log;
	file.fd = ngx_open_file(file.name.data, NGX_FILE_RDONLY, NGX_FILE_OPEN, NGX_FILE_DEFAULT_ACCESS);
	if(file.fd == NGX_INVALID_FILE) {
		ngx_log_error(NGX_LOG_ERR, pCycle->log, ngx_errno, ngx_open_file_n " \"%s\" failed", file.name.data);
		return 1;
	}
	n = ngx_read_file(&file, buf, NGX_INT64_LEN + 2, 0);
	if(ngx_close_file(file.fd) == NGX_FILE_ERROR) {
		ngx_log_error(NGX_LOG_ALERT, pCycle->log, ngx_errno, ngx_close_file_n " \"%s\" failed", file.name.data);
	}
	if(n == NGX_ERROR) {
		return 1;
	}
	while(n-- && (buf[n] == __CR || buf[n] == LF)) { /* void */
	}
	pid = ngx_atoi(buf, ++n);
	if(pid == (ngx_pid_t)NGX_ERROR) {
		ngx_log_error(NGX_LOG_ERR, pCycle->log, 0, "invalid PID number \"%*s\" in \"%s\"", n, buf, file.name.data);
		return 1;
	}
	else
		return ngx_os_signal_process(pCycle, pSig, pid);
}

static void ngx_show_version_info(const NgxStartUpOptions & rO)
{
	ngx_write_stderr("nginx version: " NGINX_VER_BUILD NGX_LINEFEED);
	if(/*ngx_show_help*/rO.Flags & rO.fShowHelp) {
		ngx_write_stderr(
		    "Usage: nginx [-?hvVtTq] [-s signal] [-c filename] "
		    "[-p prefix] [-g directives]" NGX_LINEFEED
		    NGX_LINEFEED
		    "Options:" NGX_LINEFEED
		    "  -?,-h         : this help" NGX_LINEFEED
		    "  -v            : show version and exit" NGX_LINEFEED
		    "  -V            : show version and configure options then exit"
		    NGX_LINEFEED
		    "  -t            : test configuration and exit" NGX_LINEFEED
		    "  -T            : test configuration, dump it and exit"
		    NGX_LINEFEED
		    "  -q            : suppress non-error messages "
		    "during configuration testing" NGX_LINEFEED
		    "  -s signal     : send signal to a master process: "
		    "stop, quit, reopen, reload" NGX_LINEFEED
#ifdef NGX_PREFIX
		    "  -p prefix     : set prefix path (default: " NGX_PREFIX ")"
		    NGX_LINEFEED
#else
		    "  -p prefix     : set prefix path (default: NONE)" NGX_LINEFEED
#endif
		    "  -c filename   : set configuration file (default: " NGX_CONF_PATH
		    ")" NGX_LINEFEED
		    "  -g directives : set global directives out of configuration "
		    "file" NGX_LINEFEED NGX_LINEFEED
		    );
	}
	if(/*ngx_show_configure*/rO.Flags & rO.fShowConf) {
#ifdef NGX_COMPILER
		ngx_write_stderr("built by " NGX_COMPILER NGX_LINEFEED);
#endif
#if (NGX_SSL)
		if(sstreq(ngx_ssl_version(), OPENSSL_VERSION_TEXT)) {
			ngx_write_stderr("built with " OPENSSL_VERSION_TEXT NGX_LINEFEED);
		}
		else {
			ngx_write_stderr("built with " OPENSSL_VERSION_TEXT " (running with ");
			ngx_write_stderr((char*)(uintptr_t)ngx_ssl_version());
			ngx_write_stderr(")" NGX_LINEFEED);
		}
#ifdef SSL_CTRL_SET_TLSEXT_HOSTNAME
		ngx_write_stderr("TLS SNI support enabled" NGX_LINEFEED);
#else
		ngx_write_stderr("TLS SNI support disabled" NGX_LINEFEED);
#endif
#endif
		ngx_write_stderr("configure arguments:" NGX_CONFIGURE NGX_LINEFEED);
	}
}

#if 0 // {
static ngx_int_t ngx_save_argv(ngx_cycle_t * cycle, int argc, const char * argv[])
{
#if (NGX_FREEBSD)
	ngx_os_argv = (char**)argv;
	ngx_argc = argc;
	ngx_argv = (char**)argv;
#else
	size_t len;
	ngx_int_t i;
	ngx_os_argv = (char**)argv;
	ngx_argc = argc;
	ngx_argv = (char**)ngx_alloc((argc + 1) * sizeof(char *), cycle->log);
	if(ngx_argv == NULL) {
		return NGX_ERROR;
	}
	for(i = 0; i < argc; i++) {
		len = ngx_strlen(argv[i]) + 1;
		ngx_argv[i] = (char*)ngx_alloc(len, cycle->log);
		if(ngx_argv[i] == NULL) {
			return NGX_ERROR;
		}
		(void)ngx_cpystrn((u_char*)ngx_argv[i], (u_char*)argv[i], len);
	}
	ngx_argv[i] = NULL;
#endif
	ngx_os_environ = environ;
	return NGX_OK;
}
#endif // } 0

static ngx_int_t ngx_process_options(ngx_cycle_t * cycle, const NgxStartUpOptions & rO)
{
	u_char * p;
	size_t len;
	if(/*ngx_prefix*/rO.Prefix.NotEmpty()) {
		/*
		len = ngx_strlen(ngx_prefix);
		p = ngx_prefix;
		if(len && !ngx_path_separator(p[len-1])) {
			p = (u_char *)ngx_pnalloc(cycle->pool, len + 1);
			if(!p) {
				return NGX_ERROR;
			}
			memcpy(p, ngx_prefix, len);
			p[len++] = '/';
		}
		cycle->conf_prefix.len = len;
		cycle->conf_prefix.data = p;
		cycle->prefix.len = len;
		cycle->prefix.data = p;
		*/
		SStrDupToNgxStr(cycle->pool, &rO.Prefix, &cycle->conf_prefix);
		SStrDupToNgxStr(cycle->pool, &rO.Prefix, &cycle->prefix);
	}
	else {
#ifndef NGX_PREFIX
		p = (u_char *)ngx_pnalloc(cycle->pool, NGX_MAX_PATH);
		if(!p) {
			return NGX_ERROR;
		}
		if(ngx_getcwd(p, NGX_MAX_PATH) == 0) {
			ngx_log_stderr(ngx_errno, "[emerg]: " ngx_getcwd_n " failed");
			return NGX_ERROR;
		}
		len = ngx_strlen(p);
		p[len++] = '/';
		cycle->conf_prefix.len = len;
		cycle->conf_prefix.data = p;
		cycle->prefix.len = len;
		cycle->prefix.data = p;
#else
	#ifdef NGX_CONF_PREFIX
		ngx_str_set(&cycle->conf_prefix, NGX_CONF_PREFIX);
	#else
		ngx_str_set(&cycle->conf_prefix, NGX_PREFIX);
	#endif
		ngx_str_set(&cycle->prefix, NGX_PREFIX);
#endif
	}
	if(/*ngx_conf_file*/rO.ConfFile.NotEmpty()) {
		//cycle->conf_file.len = ngx_strlen(ngx_conf_file);
		//cycle->conf_file.data = ngx_conf_file;
		SStrDupToNgxStr(cycle->pool, &rO.ConfFile, &cycle->conf_file);
	}
	else {
		ngx_str_set(&cycle->conf_file, NGX_CONF_PATH);
	}
	if(ngx_conf_full_name(cycle, &cycle->conf_file, 0) != NGX_OK) {
		return NGX_ERROR;
	}
	for(p = cycle->conf_file.data + cycle->conf_file.len - 1; p > cycle->conf_file.data; p--) {
		if(ngx_path_separator(*p)) {
			cycle->conf_prefix.len = p - ngx_cycle->conf_file.data + 1;
			cycle->conf_prefix.data = ngx_cycle->conf_file.data;
			break;
		}
	}
	if(/*ngx_conf_params*/rO.ConfParams.NotEmpty()) {
		//cycle->conf_param.len = ngx_strlen(ngx_conf_params);
		//cycle->conf_param.data = ngx_conf_params;
		SStrDupToNgxStr(cycle->pool, &rO.ConfParams, &cycle->conf_param);
	}
#ifdef NDEBUG
	if(/*ngx_test_config*/rO.Flags & rO.fTestConf) {
		cycle->log->Level = NGX_LOG_INFO;
	}
#else
	cycle->log->Level = NGX_LOG_DEBUG; 
#endif
	return NGX_OK;
}

int NgxStartUp(const NgxStartUpOptions & rO)
{
	int   result = 0;
	ngx_buf_t * b;
	ngx_log_t * log;
	ngx_uint_t i;
	ngx_cycle_t * cycle, init_cycle;
	ngx_core_conf_t * ccf;
	ngx_debug_init();
	THROW(ngx_strerror_init() == NGX_OK);
	{
		ngx_test_config__ = BIN(rO.Flags & rO.fTestConf);
		ngx_dump_config__ = BIN(rO.Flags & rO.fDumpConf);
	}
	if(/*ngx_show_version*/rO.Flags & rO.fShowVer) {
		ngx_show_version_info(rO);
		if(!/*ngx_test_config*/(rO.Flags & rO.fTestConf)) {
			return 0;
		}
	}
	/* @todo */ ngx_max_sockets = -1;
	ngx_time_init();
#if (NGX_PCRE)
	ngx_regex_init();
#endif
	ngx_pid = ngx_getpid();
	log = ngx_log_init(/*ngx_prefix*/rO.Prefix.ucptr());
	THROW(log);
	/* STUB */
#if (NGX_OPENSSL)
	ngx_ssl_init(log);
#endif
	// 
	// init_cycle->log is required for signal handlers and ngx_process_options()
	// 
	memzero(&init_cycle, sizeof(ngx_cycle_t));
	init_cycle.log = log;
	ngx_cycle = &init_cycle;
	init_cycle.pool = ngx_create_pool(1024, log);
	THROW(init_cycle.pool);
	{
		//THROW(ngx_save_argv(&init_cycle, argc, argv) == NGX_OK);
		ngx_os_environ = environ;
	}
	THROW(ngx_process_options(&init_cycle, rO) == NGX_OK);
	THROW(ngx_os_init(log) == NGX_OK);
	/*
	 * ngx_crc32_table_init() requires ngx_cacheline_size set in ngx_os_init()
	 */
	THROW(ngx_crc32_table_init() == NGX_OK);
	THROW(ngx_add_inherited_sockets(&init_cycle) == NGX_OK);
	THROW(ngx_preinit_modules() == NGX_OK);
	cycle = ngx_init_cycle(&init_cycle, rO);
	if(!cycle) {
		if(/*ngx_test_config*/rO.Flags & rO.fTestConf)
			ngx_log_stderr(0, "configuration file %s test failed", init_cycle.conf_file.data);
		result = 1; // error
	}
	else if(/*ngx_test_config*/rO.Flags & rO.fTestConf) {
		if(!/*ngx_quiet_mode*/(rO.Flags & rO.fQuietMode)) {
			ngx_log_stderr(0, "configuration file %s test is successful", cycle->conf_file.data);
		}
		if(/*ngx_dump_config*/rO.Flags & rO.fDumpConf) {
			ngx_conf_dump_t * cd = (ngx_conf_dump_t *)cycle->config_dump.elts;
			for(i = 0; i < cycle->config_dump.nelts; i++) {
				ngx_write_stdout("# configuration file ");
				(void)ngx_write_fd(ngx_stdout, cd[i].name.data, cd[i].name.len);
				ngx_write_stdout(":" NGX_LINEFEED);
				b = cd[i].buffer;
				(void)ngx_write_fd(ngx_stdout, b->pos, b->last - b->pos);
				ngx_write_stdout(NGX_LINEFEED);
			}
		}
	}
	else if(/*ngx_signal*/rO.SigID) {
		result = ngx_signal_process(cycle, /*ngx_signal*/rO.GetSignalText());
	}
	else {
		ngx_os_status(cycle->log);
		ngx_cycle = cycle;
		ccf = (ngx_core_conf_t *)ngx_get_conf(cycle->conf_ctx, ngx_core_module);
		if(ccf->master && ngx_process == NGX_PROCESS_SINGLE) {
			ngx_process = NGX_PROCESS_MASTER;
		}
#if !(NGX_WIN32)
		THROW(ngx_init_signals(cycle->log) == NGX_OK);
		if(!ngx_inherited && ccf->daemon) {
			THROW(ngx_daemon(cycle->log) == NGX_OK);
			ngx_daemonized = 1;
		}
		if(ngx_inherited) {
			ngx_daemonized = 1;
		}
#endif
		THROW(ngx_create_pidfile(&ccf->pid, cycle->log, rO) == NGX_OK);
		THROW(ngx_log_redirect_stderr(cycle) == NGX_OK);
		if(log->file->fd != ngx_stderr) {
			if(ngx_close_file(log->file->fd) == NGX_FILE_ERROR) {
				ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno, ngx_close_file_n " built-in log failed");
			}
		}
		ngx_use_stderr = 0;
		if(ngx_process == NGX_PROCESS_SINGLE) {
			result = ngx_single_process_cycle(cycle); // Normally it won't return
		}
		else {
			result = ngx_master_process_cycle(cycle, rO); // Normally it won't return
		}
	}
	CATCH
		result = 1;
	ENDCATCH
	return result;
}

/*int ngx_cdecl main(int argc, const char * argv[])
{
	int   result = 0;
	NgxStartUpOptions startup_options;
	SLS.Init("nginx", 0);
	if(startup_options.ProcessCmdLine(argc, argv) != NGX_OK)
		result = 1;
	else
		result = NgxStartUp(startup_options);
	return result;
}*/

static ngx_int_t ngx_add_inherited_sockets(ngx_cycle_t * cycle)
{
	u_char * p, * v;
	u_char * inherited = (u_char*)getenv(NGINX_VAR);
	if(inherited == NULL) {
		return NGX_OK;
	}
	ngx_log_error(NGX_LOG_NOTICE, cycle->log, 0, "using inherited sockets from \"%s\"", inherited);
	if(ngx_array_init(&cycle->listening, cycle->pool, 10, sizeof(ngx_listening_t)) != NGX_OK) {
		return NGX_ERROR;
	}
	for(p = inherited, v = p; *p; p++) {
		if(*p == ':' || *p == ';') {
			ngx_int_t s = ngx_atoi(v, p - v);
			if(s == NGX_ERROR) {
				ngx_log_error(NGX_LOG_EMERG, cycle->log, 0, "invalid socket number \"%s\" in " NGINX_VAR " environment variable, ignoring the rest of the variable", v);
				break;
			}
			v = p + 1;
			{
				ngx_listening_t * ls = (ngx_listening_t *)ngx_array_push(&cycle->listening);
				if(ls == NULL) {
					return NGX_ERROR;
				}
				memzero(ls, sizeof(ngx_listening_t));
				ls->fd = (ngx_socket_t)s;
			}
		}
	}
	if(v != p) {
		ngx_log_error(NGX_LOG_EMERG, cycle->log, 0, "invalid socket number \"%s\" in " NGINX_VAR " environment variable, ignoring", v);
	}
	ngx_inherited = 1;
	return ngx_set_inherited_sockets(cycle);
}

char ** ngx_set_environment(ngx_cycle_t * cycle, ngx_uint_t * last)
{
	char ** p, ** env;
	ngx_str_t * var;
	ngx_uint_t i, n;
	ngx_pool_cleanup_t * cln;
	ngx_core_conf_t * ccf = (ngx_core_conf_t*)ngx_get_conf(cycle->conf_ctx, ngx_core_module);
	if(last == NULL && ccf->environment) {
		return ccf->environment;
	}
	var = (ngx_str_t*)ccf->env.elts;
	for(i = 0; i < ccf->env.nelts; i++) {
		if(sstreq(var[i].data, "TZ") || ngx_strncmp(var[i].data, "TZ=", 3) == 0) {
			goto tz_found;
		}
	}
	var = (ngx_str_t*)ngx_array_push(&ccf->env);
	if(var == NULL) {
		return NULL;
	}
	var->len = 2;
	var->data = (u_char*)"TZ";
	var = (ngx_str_t*)ccf->env.elts;
tz_found:
	n = 0;
	for(i = 0; i < ccf->env.nelts; i++) {
		if(var[i].data[var[i].len] == '=') {
			n++;
		}
		else {
			for(p = ngx_os_environ; *p; p++) {
				if(ngx_strncmp(*p, var[i].data, var[i].len) == 0 && (*p)[var[i].len] == '=') {
					n++;
					break;
				}
			}
		}
	}
	if(last) {
		env = (char**)ngx_alloc((*last + n + 1) * sizeof(char *), cycle->log);
		if(env == NULL) {
			return NULL;
		}
		*last = n;
	}
	else {
		cln = ngx_pool_cleanup_add(cycle->pool, 0);
		if(cln == NULL) {
			return NULL;
		}
		env = (char**)ngx_alloc((n + 1) * sizeof(char *), cycle->log);
		if(env == NULL) {
			return NULL;
		}
		cln->handler = ngx_cleanup_environment;
		cln->data = env;
	}
	n = 0;
	for(i = 0; i < ccf->env.nelts; i++) {
		if(var[i].data[var[i].len] == '=') {
			env[n++] = (char*)var[i].data;
		}
		else {
			for(p = ngx_os_environ; *p; p++) {
				if(ngx_strncmp(*p, var[i].data, var[i].len) == 0 && (*p)[var[i].len] == '=') {
					env[n++] = *p;
					break;
				}
			}
		}
	}
	env[n] = NULL;
	if(last == NULL) {
		ccf->environment = env;
		environ = env;
	}
	return env;
}

static void ngx_cleanup_environment(void * data)
{
	char ** pp_env = (char **)data;
	if(environ != pp_env) // if the environment is still used, as it happens on exit, the only option is to leak it
		ngx_free(pp_env);
}

ngx_pid_t ngx_exec_new_binary(ngx_cycle_t * pCycle, char * const * argv)
{
	char ** env, * var;
	u_char * p;
	ngx_uint_t i, n;
	ngx_pid_t pid;
	ngx_exec_ctx_t ctx;
	ngx_core_conf_t * ccf;
	ngx_listening_t * ls;
	memzero(&ctx, sizeof(ngx_exec_ctx_t));
	ctx.path = argv[0];
	ctx.name = "new binary process";
	ctx.argv = argv;
	n = 2;
	env = ngx_set_environment(pCycle, &n);
	if(env == NULL) {
		return NGX_INVALID_PID;
	}
	var = (char*)ngx_alloc(sizeof(NGINX_VAR) + pCycle->listening.nelts * (NGX_INT32_LEN + 1) + 2, pCycle->log);
	if(var == NULL) {
		ngx_free(env);
		return NGX_INVALID_PID;
	}
	p = ngx_cpymem(var, NGINX_VAR "=", sizeof(NGINX_VAR));
	ls = (ngx_listening_t *)pCycle->listening.elts;
	for(i = 0; i < pCycle->listening.nelts; i++) {
		p = ngx_sprintf(p, "%ud;", ls[i].fd);
	}
	*p = '\0';
	env[n++] = var;
#if (NGX_SETPROCTITLE_USES_ENV)
	/* allocate the spare 300 bytes for the new binary process title */
	env[n++] = "SPARE=XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"
	    "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"
	    "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"
	    "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"
	    "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";
#endif
	env[n] = NULL;
#if (NGX_DEBUG)
	{
		for(char ** e = env; *e; e++) {
			ngx_log_debug1(NGX_LOG_DEBUG_CORE, pCycle->log, 0, "env: %s", *e);
		}
	}
#endif
	ctx.envp = (char* const*)env;
	ccf = (ngx_core_conf_t*)ngx_get_conf(pCycle->conf_ctx, ngx_core_module);
	if(ngx_rename_file(ccf->pid.data, ccf->oldpid.data) == NGX_FILE_ERROR) {
		ngx_log_error(NGX_LOG_ALERT, pCycle->log, ngx_errno, ngx_rename_file_n " %s to %s failed before executing new binary process \"%s\"", 
			ccf->pid.data, ccf->oldpid.data, argv[0]);
		ngx_free(env);
		ngx_free(var);
		return NGX_INVALID_PID;
	}
	pid = ngx_execute(pCycle, &ctx);
	if(pid == NGX_INVALID_PID) {
		if(ngx_rename_file(ccf->oldpid.data, ccf->pid.data) == NGX_FILE_ERROR) {
			ngx_log_error(NGX_LOG_ALERT, pCycle->log, ngx_errno, ngx_rename_file_n " %s back to %s failed after an attempt to execute new binary process \"%s\"",
			    ccf->oldpid.data, ccf->pid.data, argv[0]);
		}
	}
	ngx_free(env);
	ngx_free(var);
	return pid;
}

static void * ngx_core_module_create_conf(ngx_cycle_t * cycle)
{
	ngx_core_conf_t  * ccf = (ngx_core_conf_t *)ngx_pcalloc(cycle->pool, sizeof(ngx_core_conf_t));
	if(ccf) {
		/*
		 * set by ngx_pcalloc()
		 *
		 *     ccf->pid = NULL;
		 *     ccf->oldpid = NULL;
		 *     ccf->priority = 0;
		 *     ccf->cpu_affinity_auto = 0;
		 *     ccf->cpu_affinity_n = 0;
		 *     ccf->cpu_affinity = NULL;
		 */
		ccf->daemon = NGX_CONF_UNSET;
		ccf->master = NGX_CONF_UNSET;
		ccf->timer_resolution = NGX_CONF_UNSET_MSEC;
		ccf->shutdown_timeout = NGX_CONF_UNSET_MSEC;
		ccf->worker_processes = NGX_CONF_UNSET;
		ccf->debug_points = NGX_CONF_UNSET;
		ccf->rlimit_nofile = NGX_CONF_UNSET;
		ccf->rlimit_core = NGX_CONF_UNSET;
		ccf->user = (ngx_uid_t)NGX_CONF_UNSET_UINT;
		ccf->group = (ngx_gid_t)NGX_CONF_UNSET_UINT;
		if(ngx_array_init(&ccf->env, cycle->pool, 1, sizeof(ngx_str_t)) != NGX_OK) {
			return NULL;
		}
	}
	return ccf;
}

static const char * ngx_core_module_init_conf(ngx_cycle_t * cycle, void * conf)
{
	ngx_core_conf_t * ccf = (ngx_core_conf_t *)conf;
	ngx_conf_init_value(ccf->daemon, 1);
	ngx_conf_init_value(ccf->master, 0); // @sobolev 1-->0
	ngx_conf_init_msec_value(ccf->timer_resolution, 0);
	ngx_conf_init_msec_value(ccf->shutdown_timeout, 0);
	ngx_conf_init_value(ccf->worker_processes, 1);
	ngx_conf_init_value(ccf->debug_points, 0);
#if (NGX_HAVE_CPU_AFFINITY)
	if(!ccf->cpu_affinity_auto && ccf->cpu_affinity_n && ccf->cpu_affinity_n != 1 && ccf->cpu_affinity_n != (ngx_uint_t)ccf->worker_processes) {
		ngx_log_error(NGX_LOG_WARN, cycle->log, 0, "the number of \"worker_processes\" is not equal to the number of \"worker_cpu_affinity\" masks, using last mask for remaining worker processes");
	}
#endif
	if(ccf->pid.len == 0) {
		ngx_str_set(&ccf->pid, NGX_PID_PATH);
	}
	if(ngx_conf_full_name(cycle, &ccf->pid, 0) != NGX_OK) {
		return NGX_CONF_ERROR;
	}
	ccf->oldpid.len = ccf->pid.len + sizeof(NGX_OLDPID_EXT);
	ccf->oldpid.data = (u_char *)ngx_pnalloc(cycle->pool, ccf->oldpid.len);
	if(ccf->oldpid.data == NULL) {
		return NGX_CONF_ERROR;
	}
	memcpy(ngx_cpymem(ccf->oldpid.data, ccf->pid.data, ccf->pid.len), NGX_OLDPID_EXT, sizeof(NGX_OLDPID_EXT));
#if !(NGX_WIN32)
	if(ccf->user == (uid_t)NGX_CONF_UNSET_UINT && geteuid() == 0) {
		struct group * grp;
		struct passwd  * pwd;
		ngx_set_errno(0);
		pwd = getpwnam(NGX_USER);
		if(pwd == NULL) {
			ngx_log_error(NGX_LOG_EMERG, cycle->log, ngx_errno, "getpwnam(\"" NGX_USER "\") failed");
			return NGX_CONF_ERROR;
		}
		ccf->username = NGX_USER;
		ccf->user = pwd->pw_uid;
		ngx_set_errno(0);
		grp = getgrnam(NGX_GROUP);
		if(grp == NULL) {
			ngx_log_error(NGX_LOG_EMERG, cycle->log, ngx_errno, "getgrnam(\"" NGX_GROUP "\") failed");
			return NGX_CONF_ERROR;
		}
		ccf->group = grp->gr_gid;
	}
	if(ccf->lock_file.len == 0) {
		ngx_str_set(&ccf->lock_file, NGX_LOCK_PATH);
	}
	if(ngx_conf_full_name(cycle, &ccf->lock_file, 0) != NGX_OK) {
		return NGX_CONF_ERROR;
	}
	{
		ngx_str_t lock_file = cycle->old_cycle->lock_file;
		if(lock_file.len) {
			lock_file.len--;
			if(ccf->lock_file.len != lock_file.len || ngx_strncmp(ccf->lock_file.data, lock_file.data, lock_file.len) != 0) {
				ngx_log_error(NGX_LOG_EMERG, cycle->log, 0, "\"lock_file\" could not be changed, ignored");
			}
			cycle->lock_file.len = lock_file.len + 1;
			lock_file.len += sizeof(".accept");
			cycle->lock_file.data = ngx_pstrdup(cycle->pool, &lock_file);
			if(cycle->lock_file.data == NULL) {
				return NGX_CONF_ERROR;
			}
		}
		else {
			cycle->lock_file.len = ccf->lock_file.len + 1;
			cycle->lock_file.data = ngx_pnalloc(cycle->pool, ccf->lock_file.len + sizeof(".accept"));
			if(cycle->lock_file.data == NULL) {
				return NGX_CONF_ERROR;
			}
			memcpy(ngx_cpymem(cycle->lock_file.data, ccf->lock_file.data, ccf->lock_file.len), ".accept", sizeof(".accept"));
		}
	}
#endif
	return NGX_CONF_OK;
}

static const char * ngx_set_user(ngx_conf_t * cf, const ngx_command_t * cmd, void * conf) // F_SetHandler
{
#if (NGX_WIN32)
	ngx_conf_log_error(NGX_LOG_WARN, cf, 0, "\"user\" is not supported, ignored");
	return NGX_CONF_OK;
#else
	ngx_core_conf_t * ccf = conf;
	char * group;
	struct passwd  * pwd;
	struct group   * grp;
	if(ccf->user != (uid_t)NGX_CONF_UNSET_UINT) {
		return "is duplicate";
	}
	else {
		if(geteuid() != 0) {
			ngx_conf_log_error(NGX_LOG_WARN, cf, 0, "the \"user\" directive makes sense only if the master process runs with super-user privileges, ignored");
		}
		else {
			ngx_str_t * value = cf->args->elts;
			ccf->username = (char*)value[1].data;
			ngx_set_errno(0);
			pwd = getpwnam((const char*)value[1].data);
			if(pwd == NULL) {
				ngx_conf_log_error(NGX_LOG_EMERG, cf, ngx_errno, "getpwnam(\"%s\") failed", value[1].data);
				return NGX_CONF_ERROR;
			}
			ccf->user = pwd->pw_uid;
			group = (char*)((cf->args->nelts == 2) ? value[1].data : value[2].data);
			ngx_set_errno(0);
			grp = getgrnam(group);
			if(grp == NULL) {
				ngx_conf_log_error(NGX_LOG_EMERG, cf, ngx_errno, "getgrnam(\"%s\") failed", group);
				return NGX_CONF_ERROR;
			}
			ccf->group = grp->gr_gid;
		}
		return NGX_CONF_OK;
	}
#endif
}

static const char * ngx_set_env(ngx_conf_t * cf, const ngx_command_t * cmd, void * conf) // F_SetHandler
{
	ngx_core_conf_t  * ccf = (ngx_core_conf_t *)conf;
	ngx_uint_t i;
	ngx_str_t * var = (ngx_str_t*)ngx_array_push(&ccf->env);
	if(var == NULL) {
		return NGX_CONF_ERROR;
	}
	else {
		const ngx_str_t * value = (ngx_str_t*)cf->args->elts;
		*var = value[1];
		for(i = 0; i < value[1].len; i++) {
			if(value[1].data[i] == '=') {
				var->len = i;
				return NGX_CONF_OK;
			}
		}
		return NGX_CONF_OK;
	}
}

static const char * ngx_set_priority(ngx_conf_t * cf, const ngx_command_t * cmd, void * conf) // F_SetHandler
{
	ngx_core_conf_t  * ccf = (ngx_core_conf_t *)conf;
	ngx_uint_t n, minus;
	if(ccf->priority != 0) {
		return "is duplicate";
	}
	else {
		ngx_str_t * value = (ngx_str_t*)cf->args->elts;
		if(value[1].data[0] == '-') {
			n = 1;
			minus = 1;
		}
		else if(value[1].data[0] == '+') {
			n = 1;
			minus = 0;
		}
		else {
			n = 0;
			minus = 0;
		}
		ccf->priority = ngx_atoi(&value[1].data[n], value[1].len - n);
		if(ccf->priority == NGX_ERROR) {
			return "invalid number";
		}
		if(minus) {
			ccf->priority = -ccf->priority;
		}
		return NGX_CONF_OK;
	}
}

static const char * ngx_set_cpu_affinity(ngx_conf_t * cf, const ngx_command_t * cmd, void * conf) // F_SetHandler
{
#if (NGX_HAVE_CPU_AFFINITY)
	ngx_core_conf_t  * ccf = conf;
	u_char ch, * p;
	ngx_str_t * value;
	ngx_uint_t i, n;
	ngx_cpuset_t   * mask;
	if(ccf->cpu_affinity) {
		return "is duplicate";
	}
	mask = ngx_palloc(cf->pool, (cf->args->nelts - 1) * sizeof(ngx_cpuset_t));
	if(mask == NULL) {
		return NGX_CONF_ERROR;
	}
	ccf->cpu_affinity_n = cf->args->nelts - 1;
	ccf->cpu_affinity = mask;
	value = cf->args->elts;
	if(sstreq(value[1].data, "auto")) {
		if(cf->args->nelts > 3) {
			ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "invalid number of arguments in \"worker_cpu_affinity\" directive");
			return NGX_CONF_ERROR;
		}
		ccf->cpu_affinity_auto = 1;
		CPU_ZERO(&mask[0]);
		for(i = 0; i < (ngx_uint_t)MIN(ngx_ncpu, CPU_SETSIZE); i++) {
			CPU_SET(i, &mask[0]);
		}
		n = 2;
	}
	else {
		n = 1;
	}
	for(/* void */; n < cf->args->nelts; n++) {
		if(value[n].len > CPU_SETSIZE) {
			ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "\"worker_cpu_affinity\" supports up to %d CPUs only", CPU_SETSIZE);
			return NGX_CONF_ERROR;
		}
		i = 0;
		CPU_ZERO(&mask[n - 1]);
		for(p = value[n].data + value[n].len - 1; p >= value[n].data; p--) {
			ch = *p;
			if(ch == ' ') {
				continue;
			}
			i++;
			if(ch == '0') {
				continue;
			}
			if(ch == '1') {
				CPU_SET(i - 1, &mask[n - 1]);
				continue;
			}
			ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "invalid character \"%c\" in \"worker_cpu_affinity\"", ch);
			return NGX_CONF_ERROR;
		}
	}
#else
	ngx_conf_log_error(NGX_LOG_WARN, cf, 0, "\"worker_cpu_affinity\" is not supported on this platform, ignored");
#endif
	return NGX_CONF_OK;
}

ngx_cpuset_t * ngx_get_cpu_affinity(ngx_uint_t n)
{
#if (NGX_HAVE_CPU_AFFINITY)
	ngx_uint_t i, j;
	ngx_cpuset_t   * mask;
	ngx_core_conf_t  * ccf;
	static ngx_cpuset_t result;
	ccf = (ngx_core_conf_t*)ngx_get_conf(ngx_cycle->conf_ctx, ngx_core_module);
	if(ccf->cpu_affinity == NULL) {
		return NULL;
	}
	if(ccf->cpu_affinity_auto) {
		mask = &ccf->cpu_affinity[ccf->cpu_affinity_n - 1];
		for(i = 0, j = n; /* void */; i++) {
			if(CPU_ISSET(i % CPU_SETSIZE, mask) && j-- == 0) {
				break;
			}
			if(i == CPU_SETSIZE && j == n) {
				/* empty mask */
				return NULL;
			}
			/* void */
		}
		CPU_ZERO(&result);
		CPU_SET(i % CPU_SETSIZE, &result);
		return &result;
	}
	if(ccf->cpu_affinity_n > n) {
		return &ccf->cpu_affinity[n];
	}
	return &ccf->cpu_affinity[ccf->cpu_affinity_n - 1];
#else
	return NULL;
#endif
}

static const char * ngx_set_worker_processes(ngx_conf_t * cf, const ngx_command_t * cmd, void * conf) // F_SetHandler
{
	ngx_core_conf_t * ccf = (ngx_core_conf_t*)conf;
	if(ccf->worker_processes != NGX_CONF_UNSET) {
		return "is duplicate";
	}
	else {
		ngx_str_t * value = (ngx_str_t*)cf->args->elts;
		if(sstreq(value[1].data, "auto")) {
			ccf->worker_processes = ngx_ncpu;
			return NGX_CONF_OK;
		}
		ccf->worker_processes = ngx_atoi(value[1].data, value[1].len);
		if(ccf->worker_processes == NGX_ERROR) {
			return "invalid value";
		}
		return NGX_CONF_OK;
	}
}

static const char * ngx_load_module(ngx_conf_t * cf, const ngx_command_t * cmd, void * conf) // F_SetHandler
{
#if (NGX_HAVE_DLOPEN)
	void  * handle;
	char   ** names, ** order;
	ngx_str_t * value, file;
	ngx_uint_t i;
	ngx_module_t * module, ** modules;
	ngx_pool_cleanup_t  * cln;
	if(cf->cycle->modules_used) {
		return "is specified too late";
	}
	value = (ngx_str_t*)cf->args->elts;
	file = value[1];
	if(ngx_conf_full_name(cf->cycle, &file, 0) != NGX_OK) {
		return NGX_CONF_ERROR;
	}
	cln = ngx_pool_cleanup_add(cf->cycle->pool, 0);
	if(cln == NULL) {
		return NGX_CONF_ERROR;
	}
	handle = ngx_dlopen(file.data);
	if(handle == NULL) {
		ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, ngx_dlopen_n " \"%s\" failed (%s)", file.data, ngx_dlerror());
		return NGX_CONF_ERROR;
	}
	cln->handler = ngx_unload_module;
	cln->data = handle;
	modules = (ngx_module_t **)ngx_dlsym((HMODULE)handle, "ngx_modules");
	if(modules == NULL) {
		ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, ngx_dlsym_n " \"%V\", \"%s\" failed (%s)", &value[1], "ngx_modules", ngx_dlerror());
		return NGX_CONF_ERROR;
	}
	names = (char **)ngx_dlsym((HMODULE)handle, "ngx_module_names");
	if(names == NULL) {
		ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, ngx_dlsym_n " \"%V\", \"%s\" failed (%s)", &value[1], "ngx_module_names", ngx_dlerror());
		return NGX_CONF_ERROR;
	}
	order = (char **)ngx_dlsym((HMODULE)handle, "ngx_module_order");
	for(i = 0; modules[i]; i++) {
		module = modules[i];
		module->name = names[i];
		if(ngx_add_module(cf, &file, module, order) != NGX_OK) {
			return NGX_CONF_ERROR;
		}
		ngx_log_debug2(NGX_LOG_DEBUG_CORE, cf->log, 0, "module: %s i:%ui", module->name, module->index);
	}
	return NGX_CONF_OK;
#else
	ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "\"load_module\" is not supported on this platform");
	return NGX_CONF_ERROR;
#endif
}

#if (NGX_HAVE_DLOPEN)

static void ngx_unload_module(void * data)
{
	void  * handle = data;
	if(ngx_dlclose((HMODULE)handle) != 0) {
		ngx_log_error(NGX_LOG_ALERT, ngx_cycle->log, 0, ngx_dlclose_n " failed (%s)", ngx_dlerror());
	}
}

#endif
