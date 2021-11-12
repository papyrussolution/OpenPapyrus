/*
  (C) 2002 Sebastian Hetze <s.hetze@linux-ag.de>
  Redistribution and use in source and binary forms, with or without
  modification, are permitted under terms and conditions of BSD copyright.
  
This gives you an overview of the major api changes from cyrus-sasl version 1.5 to 2.1.
Certainly not complete, but hopefully a point to start with when you want to upgrade
your apps.
Prototypes at the end of this file might be handy.
 
sasl_callbacks		CNONCE, GETSECRET and PUTSECRET are gone,
				USERDB_CHECKPASS, USERDB_SETPASS and CANON_USER are new
				sasl_authorize_t (for CB_PROXY_POLICY) has changed completely
sasl_checkpass		has lost one argument (errstr)
sasl_client_new		has got two new arguments (iplocalport, ipremoteport)
				flags type has changed from int to unsigned
sasl_client_start	has lost one argument (secret)
				clientout now const
sasl_client_step	clientout now const
sasl_decode		output now const
sasl_encode		output now const
sasl_getprop		pvalue now const
sasl_listmech		pcount has changed from unsigned to int
sasl_server_new		has got two new arguments (iplocalport, ipremoteport)
				flags type has changed from int to unsigned
sasl_server_start	has lost one argument (errstr)
				serverout now const
sasl_server_step	has lost one argument (errstr)
				serverout now const
sasl_user_exists	has got new arg conn
sasl_setpass		has got new arguments oldpass, oldpasslen
				lost argument errstr
				flags changed from int to unsigned
sasl_set_mutex		4arg sasl_mutex_dispose_t changed to sasl_mutex_free_t

sasl_usererr		is gone
sasl_client_auth	is gone
sasl_free_secret	is gone
sasl_cred_install	is gone
sasl_cred_uninstall	is gone

sasl_client_init	interface not changed
sasl_done		interface not changed
sasl_errstring		interface not changed
sasl_server_init	interface not changed
sasl_setprop		interface not changed
sasl_set_alloc		interface not changed
sasl_dispose		interface not changed
sasl_errstring		interface not changed
sasl_idle		interface not changed
*/


static sasl_callback_t callbacks[] = {
	{SASL_CB_GETOPT, (sasl_getopt_t)&getopt_func, NULL },
	{SASL_CB_LOG, (sasl_log_t)&log_func, NULL },
	{SASL_CB_GETPATH, (sasl_getpath_t)&getpath_func , NULL },
	{SASL_CB_VERIFYFILE, (sasl_verifyfile_t)&verifyfile_func , NULL },
	{SASL_CB_USER, (sasl_getsimple_t)&uname_func , NULL },
	{SASL_CB_AUTHNAME, (sasl_getsimple_t)&authname_func , NULL },
	{SASL_CB_LANGUAGE, (sasl_getsimple_t)&language_func , NULL },
#if SASL_VERSION_MAJOR < 2
	{SASL_CB_CNONCE, (sasl_getsimple_t)&cnonce_func , NULL },
#endif
	{SASL_CB_PASS, (sasl_getsecret_t)&getsecret_func , NULL },
	{SASL_CB_ECHOPROMPT, (sasl_chalprompt_t)&challenge_func , NULL },
	{SASL_CB_NOECHOPROMPT, (sasl_chalprompt_t)&challenge_func , NULL },
	{SASL_CB_GETREALM, (sasl_getrealm_t)&getrealm_func , NULL },
#if SASL_VERSION_MAJOR < 2
	/* sasl_authorize_t version 1 has almost nothing in common with v2 */
	{SASL_CB_PROXY_POLICY, (sasl_authorize_t)&authorize1_func, NULL },
	{SASL_CB_SERVER_GETSECRET, (sasl_server_getsecret_t)&srvgetsecret_func , NULL },
	{SASL_CB_SERVER_PUTSECRET, (sasl_server_putsecret_t)&srvputsecret_func, NULL },
#else
	{SASL_CB_PROXY_POLICY, (sasl_authorize_t)&authorize2_func, NULL },
	{SASL_CB_SERVER_USERDB_CHECKPASS, (sasl_server_userdb_checkpass_t)&sudbcp_func, NULL },
	{SASL_CB_SERVER_USERDB_SETPASS, (sasl_server_userdb_setpass_t)&sudbsp_func, NULL },
	{SASL_CB_CANON_USER, (sasl_canon_user_t)&canon_func , NULL },
#endif
	{SASL_CB_LIST_END, NULL, NULL },
};


int sasl_checkpass(sasl_conn_t *conn,
		const char *user,
		unsigned userlen,
		const char *pass,
		unsigned passlen
#if SASL_VERSION_MAJOR < 2
		, const char **errstr
#endif
		);	

int sasl_client_new(const char *service,
		const char *serverFQDN,
#if SASL_VERSION_MAJOR > 1
		const char *iplocalport,
		const char *ipremoteport,
#endif
		const sasl_callback_t *prompt_supp,
#if SASL_VERSION_MAJOR < 2
		int secflags,
#else
		unsigned secflags,
#endif
		sasl_conn_t ** pconn);

int sasl_client_start(sasl_conn_t *conn,
		const char *mechlist,
#if SASL_VERSION_MAJOR < 2
		sasl_secret_t *secret,
#endif
		sasl_interact_t **prompt_need,
#if SASL_VERSION_MAJOR < 2
		char **clientout,
#else
		const char **clientout,
#endif
		unsigned *clientoutlen,
		const char **mech);

int sasl_client_step(sasl_conn_t *conn,
		const char *serverin,
		unsigned serverinlen,
		sasl_interact_t **prompt_need,
#if SASL_VERSION_MAJOR < 2
		char **clientout,
#else
		const char **clientout,
#endif
		unsigned *clientoutlen);

int sasl_decode(sasl_conn_t *conn,
		const char *input,
		unsigned inputlen,
#if SASL_VERSION_MAJOR < 2
		char **output,
#else
		const char **output,
#endif
		unsigned *outputlen);

int sasl_encode(sasl_conn_t *conn,
		const char *input,
		unsigned inputlen,
#if SASL_VERSION_MAJOR < 2
		char **output,
#else
		const char **output,
#endif
		unsigned *outputlen);

sasl_getprop(sasl_conn_t *conn, int propnum,
#if SASL_VERSION_MAJOR < 2
		void **pvalue);
#else
		const void **pvalue);
#endif

int sasl_listmech(sasl_conn_t *conn,
		const char *user,
		const char *prefix,
		const char *sep,
		const char *suffix,
		const char **result,
		unsigned *plen,
#if SASL_VERSION_MAJOR < 2
		unsigned *pcount);
#else
		int *pcount);
#endif

int sasl_server_new(const char *service,
		const char *serverFQDN,
		const char *user_realm,
#if SASL_VERSION_MAJOR > 1
		const char *iplocalport,
		const char *ipremoteport,
#endif
		const sasl_callback_t *callbacks,
#if SASL_VERSION_MAJOR < 2
		int secflags,
#else
		unsigned flags,
#endif
		sasl_conn_t **pconn);


int sasl_server_start(sasl_conn_t *conn,
		const char *mech,
		const char *clientin,
		unsigned clientinlen,
#if SASL_VERSION_MAJOR < 2
		char **serverout,
#else
		const char **serverout,
#endif
		unsigned *serveroutlen
#if SASL_VERSION_MAJOR < 2
		,const char **errstr
#endif
		);

int sasl_server_step(sasl_conn_t *conn,
		const char *clientin,
		unsigned clientinlen,
#if SASL_VERSION_MAJOR < 2
		char **serverout,
#else
		const char **serverout,
#endif
		unsigned *serveroutlen
#if SASL_VERSION_MAJOR < 2
		,const char **errstr
#endif
		);
		

int sasl_user_exists(
#if SASL_VERSION_MAJOR > 1
		sasl_conn_t *conn,
#endif
		const char *service,
		const char *user_realm,
		const char *user);

int sasl_setpass(sasl_conn_t *conn,
		const char *user,
		const char *pass,
		unsigned passlen,
#if SASL_VERSION_MAJOR > 1
		const char *oldpass,
		unsigned oldpasslen,
		unsigned flags);
#else
		int flags,
		const char **errstr);
#endif
















