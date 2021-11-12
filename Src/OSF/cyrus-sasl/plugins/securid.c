/* SECURID SASL plugin
 * Ken Murchison
 */
/* 
 * Copyright (c) 2006-2016 Carnegie Mellon University.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer. 
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. The name "Carnegie Mellon University" must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission. For permission or any other legal
 *    details, please contact  
 *      Carnegie Mellon University
 *      Center for Technology Transfer and Enterprise Creation
 *      4615 Forbes Avenue
 *      Suite 302
 *      Pittsburgh, PA  15213
 *      (412) 268-7393, fax: (412) 268-7395
 *      innovation@andrew.cmu.edu
 *
 * 4. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by Computing Services
 *     at Carnegie Mellon University (http://www.cmu.edu/computing/)."
 *
 * CARNEGIE MELLON UNIVERSITY DISCLAIMS ALL WARRANTIES WITH REGARD TO
 * THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS, IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY BE LIABLE
 * FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <config.h>
#include <stdio.h>
#include <string.h> 

#include <sdi_athd.h>
#include <sdconf.h>
#include <sdi_size.h>
#include <sdi_type.h>
#include <sdi_defs.h>
#include <sdacmvls.h>

#include <sasl.h>
#include <saslplug.h>

#include "plugin_common.h"

#ifdef WIN32
/* This must be after sasl.h */
# include "saslSECURID.h"
#endif /* WIN32 */

#ifdef macintosh 
#include <sasl_securid_plugin_decl.h> 
#endif 

int creadcfg(void);
int sd_init(struct SD_CLIENT *sd);
int sd_check(char  *passcode, char *username, struct SD_CLIENT *sd);
int sd_pin(char *pin, char canceled, struct SD_CLIENT *sd);
int sd_next(char *nextcode, struct SD_CLIENT *sd);
void sd_close(struct SD_CLIENT *sd);
union config_record configure;

static const char rcsid[] = "$Implementation: Carnegie Mellon SASL " VERSION " $";

#undef L_DEFAULT_GUARD
#define L_DEFAULT_GUARD (0)

typedef struct context {
    int state;
    struct SD_CLIENT sd;
    int need_pin;
    sasl_secret_t *passcode;
    char *out_buf;
    unsigned out_buf_len;
} context_t;

static int securid_server_mech_new(void *glob_context __attribute__((unused)), 
				 sasl_server_params_t *sparams,
				 const char *challenge __attribute__((unused)),
				 unsigned challen __attribute__((unused)),
				 void **conn)
{
  context_t *text;

  /* holds state are in */
  text=sparams->utils->malloc(sizeof(context_t));
  if (text==NULL) {
      MEMERROR(sparams->utils);
      return SASL_NOMEM;
  }

  memset(text, 0, sizeof(context_t));
#if 0
    /* access sdconf.rec */
    if (creadcfg()) {
	SETERROR(sparams->utils, "error reading sdconf.rec");
	return SASL_FAIL;
    }

  /* initialize socket */
  if (sd_init(&text->sd)) {
      SETERROR(sparams->utils,
	       "Cannot initialize ACE client-server communications");
      return SASL_FAIL;
  }
#endif
  text->state=1;

  *conn=text;

  return SASL_OK;
}

static void securid_both_mech_dispose(void *conn_context,
				      const sasl_utils_t *utils)
{
  context_t *text;
  text=conn_context;

  if (!text)
    return;

  /* free sensitive info */
  _plug_free_secret(utils, &(text->passcode));
  
  if(text->out_buf)
      utils->free(text->out_buf);

  utils->free(text);
}

static void securid_server_mech_dispose(void *conn_context,
					const sasl_utils_t *utils)
{
  context_t *text;
  text=conn_context;

  if (!text)
    return;
#if 0
  sd_close(&text->sd);	/* Shutdown the network connection */
#endif
  securid_both_mech_dispose(conn_context, utils);
}


static void securid_both_mech_free(void *global_context,
				 const sasl_utils_t *utils)
{
    if(global_context) utils->free(global_context);  
}

/* fills in passcode; remember to free passcode and wipe it out correctly */
static
int verify_passcode(sasl_server_params_t *params, 
		    const char *user, const char *pass)
{
    int result;
    
    /* if it's null, checkpass will default */
    result = params->utils->checkpass(params->utils->conn,
				      user, 0, pass, 0);
    
    return result;
}

static int
securid_server_mech_step(void *conn_context,
		       sasl_server_params_t *params,
		       const char *clientin,
		       unsigned clientinlen,
		       const char **serverout,
		       unsigned *serveroutlen,
		       sasl_out_params_t *oparams)
{
  context_t *text;
  text=conn_context;

  oparams->mech_ssf=0;
  oparams->maxoutbuf = 0;
  
  oparams->encode = NULL;
  oparams->decode = NULL;

  oparams->param_version = 0;

  if (text->state <= 2) {
    const char *authzid;
    const char *authid;
    const char *passcode;
    const char *pin;
    unsigned lup=0;
    int result, ret;

    /* should have received authzid NUL authid NUL passcode NUL [ pin NUL ] */

    /* get authzid */
    authzid = clientin;
    while ((lup < clientinlen) && (clientin[lup] != 0))
      ++lup;

    if (lup >= clientinlen)
    {
	SETERROR(params->utils, "Can only find authzid (no passcode)");
	return SASL_BADPROT;
    }

    /* get authid */
    ++lup;
    authid = clientin + lup;
    while ((lup < clientinlen) && (clientin[lup] != 0))
      ++lup;

    if (lup >= clientinlen)
    {
	params->utils->seterror(params->utils->conn, 0,
				"Can only find authzid/authid (no passcode)");
	return SASL_BADPROT;
    }

    /* get passcode */
    lup++;
    passcode = clientin + lup;
    while ((lup < clientinlen) && (clientin[lup] != 0))
      ++lup;

    if (text->state == 2 && text->need_pin) {
	if (lup >= clientinlen)
	    {
		params->utils->seterror(params->utils->conn, 0,
					"Can only find authzid/authid/passcode"
					" (no pin)");
		return SASL_BADPROT;
	    }

	/* get pin */
	lup++;
	pin = clientin + lup;
	while ((lup < clientinlen) && (clientin[lup] != 0))
	    ++lup;
    }

    if (lup >= clientinlen) {
	SETERROR(params->utils, "Got more data than we were expecting in the SECURID plugin\n");
	return SASL_BADPROT;
    }

    if (text->state == 1) {
	/* verify passcode - return sasl_ok on success */  
	if (!strcasecmp(passcode, "ok"))
	    ret = ACM_OK;
	else if (!strcasecmp(passcode, "fail"))
	    ret = ACM_ACCESS_DENIED;
	else if (!strcasecmp(passcode, "next"))
	    ret = ACM_NEXT_CODE_REQUIRED;
	else if (!strcasecmp(passcode, "pin"))
	    ret = ACM_NEW_PIN_REQUIRED;
	else
	    ret = ACM_ACCESS_DENIED;

	if (ret == ACM_ACCESS_DENIED) {
	    params->utils->seterror(params->utils->conn, 0,
				    "Passcode verification failed");
	    return SASL_BADAUTH;
	}

	if (! authzid || !*authzid)
	    authzid = authid;

	result = params->canon_user(params->utils->conn,
				    authid, 0, SASL_CU_AUTHID, oparams);
	if(result != SASL_OK) return result;

	result = params->canon_user(params->utils->conn,
				    authzid, 0, SASL_CU_AUTHZID, oparams);
	if(result != SASL_OK) return result;

	switch (ret) {
	case ACM_OK:
	    *serverout = NULL;
	    *serveroutlen = 0;

	    text->state = 3; /* so fails if called again */

	    oparams->doneflag = 1;

	    return SASL_OK;

	    break;

	case ACM_NEXT_CODE_REQUIRED:
	    *serveroutlen = strlen("passcode")+1;

	    result = _plug_buf_alloc(params->utils, &(text->out_buf),
				     &(text->out_buf_len), *serveroutlen);
	    if(result != SASL_OK) return result;

	    strcpy(text->out_buf, "passcode");

	    *serverout = text->out_buf;

	    text->state++;
	    return SASL_CONTINUE;

	    break;

	case ACM_NEW_PIN_REQUIRED:
	    *serveroutlen = strlen("pin")+1;

	    result = _plug_buf_alloc(params->utils, &(text->out_buf),
				     &(text->out_buf_len), *serveroutlen);
	    if(result != SASL_OK) return result;

	    strcpy(text->out_buf, "pin");

	    *serverout = text->out_buf;

	    text->state++;

	    text->need_pin = 1;

	    return SASL_CONTINUE;

	    break;
	}
    }

    if (text->state == 2) {
	if (text->need_pin) {
	}

	else {
	    /* verify next passcode - return sasl_ok on success */  
	    if (!strcasecmp(passcode, "ok"))
		ret = ACM_OK;
	    else if (!strcasecmp(passcode, "fail"))
		ret = ACM_ACCESS_DENIED;
	    else
		ret = ACM_ACCESS_DENIED;

	    if (ret == ACM_ACCESS_DENIED) {
		params->utils->seterror(params->utils->conn, 0,
					"Next passcode verification failed");
		return SASL_BADAUTH;
	    }

	    *serverout = NULL;
	    *serveroutlen = 0;

	    text->state = 3; /* so fails if called again */

	    oparams->doneflag = 1;

	    return SASL_OK;
	}
    }
  }

  SETERROR( params->utils,
	    "Unexpected State Reached in SECURID plugin");
  return SASL_FAIL; /* should never get here */
}

static sasl_server_plug_t securid_server_plugins[] = 
{
  {
    "SECURID",
    0,
    SASL_SEC_NOPLAINTEXT | SASL_SEC_NOANONYMOUS | SASL_SEC_FORWARD_SECRECY,
    SASL_FEAT_WANT_CLIENT_FIRST,
    NULL,
    &securid_server_mech_new,
    &securid_server_mech_step,
    &securid_server_mech_dispose,
    &securid_both_mech_free,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
  }
};

int securid_server_plug_init(const sasl_utils_t *utils,
			   int maxversion,
			   int *out_version,
			   sasl_server_plug_t **pluglist,
			   int *plugcount)
{
    if (maxversion<SASL_SERVER_PLUG_VERSION) {
	SETERROR(utils, "SECURID version mismatch");
	return SASL_BADVERS;
    }

    *pluglist=securid_server_plugins;

    *plugcount=1;  
    *out_version=SASL_SERVER_PLUG_VERSION;

    return SASL_OK;
}

/* put in sasl_wrongmech */
static int securid_client_mech_new(void *glob_context __attribute__((unused)),
				 sasl_client_params_t *params,
				 void **conn)
{
    context_t *text;

    /* holds state are in */
    text = params->utils->malloc(sizeof(context_t));
    if (text==NULL) {
	MEMERROR( params->utils );
	return SASL_NOMEM;
    }
    
    memset(text, 0, sizeof(context_t));

    text->state=1;
    *conn=text;

    return SASL_OK;
}

/* 
 * Trys to find the prompt with the lookingfor id in the prompt list
 * Returns it if found. NULL otherwise
 */
static sasl_interact_t *find_prompt(sasl_interact_t **promptlist,
				    unsigned int lookingfor)
{
  sasl_interact_t *prompt;

  if (promptlist && *promptlist)
    for (prompt = *promptlist;
	 prompt->id != SASL_CB_LIST_END;
	 ++prompt)
      if (prompt->id==lookingfor)
	return prompt;

  return NULL;
}

/*
 * Somehow retrieve the userid
 * This is the same as in digest-md5 so change both
 */
static int get_userid(sasl_client_params_t *params,
		      const char **userid,
		      sasl_interact_t **prompt_need)
{
  int result;
  sasl_getsimple_t *getuser_cb;
  void *getuser_context;
  sasl_interact_t *prompt;
  const char *id;

  /* see if we were given the userid in the prompt */
  prompt=find_prompt(prompt_need,SASL_CB_USER);
  if (prompt!=NULL)
    {
	*userid = prompt->result;
	return SASL_OK;
    }

  /* Try to get the callback... */
  result = params->utils->getcallback(params->utils->conn,
				      SASL_CB_USER,
				      &getuser_cb,
				      &getuser_context);
  if (result == SASL_OK && getuser_cb) {
    id = NULL;
    result = getuser_cb(getuser_context,
			SASL_CB_USER,
			&id,
			NULL);
    if (result != SASL_OK)
      return result;
    if (! id) {
	PARAMERROR(params->utils);
	return SASL_BADPARAM;
    }
    
    *userid = id;
  }

  return result;
}

static int get_authid(sasl_client_params_t *params,
		      const char **authid,
		      sasl_interact_t **prompt_need)
{

  int result;
  sasl_getsimple_t *getauth_cb;
  void *getauth_context;
  sasl_interact_t *prompt;
  const char *id;

  /* see if we were given the authname in the prompt */
  prompt=find_prompt(prompt_need,SASL_CB_AUTHNAME);
  if (prompt!=NULL)
  {
      *authid = prompt->result;
      
      return SASL_OK;
  }

  /* Try to get the callback... */
  result = params->utils->getcallback(params->utils->conn,
				      SASL_CB_AUTHNAME,
				      &getauth_cb,
				      &getauth_context);
  if (result == SASL_OK && getauth_cb) {
    id = NULL;
    result = getauth_cb(getauth_context,
			SASL_CB_AUTHNAME,
			&id,
			NULL);
    if (result != SASL_OK)
      return result;
    if (! id) {
	PARAMERROR( params->utils );
	return SASL_BADPARAM;
    }
    
    *authid = id;
  }

  return result;
}

static int get_passcode(sasl_client_params_t *params,
		      sasl_secret_t **passcode,
		      sasl_interact_t **prompt_need)
{

  int result;
  sasl_getsecret_t *getpass_cb;
  void *getpass_context;
  sasl_interact_t *prompt;

  /* see if we were given the passcode in the prompt */
  prompt=find_prompt(prompt_need,SASL_CB_PASS);
  if (prompt!=NULL)
  {
      /* We prompted, and got.*/
	
      if (! prompt->result) {
	  SETERROR(params->utils, "Unexpectedly missing a prompt result");
	  return SASL_FAIL;
      }
      
      /* copy what we got into a secret_t */
      *passcode = (sasl_secret_t *) params->utils->malloc(sizeof(sasl_secret_t)+
							  prompt->len+1);
      if (! *passcode) {
	  MEMERROR( params->utils );
	  return SASL_NOMEM;
      }
      
      (*passcode)->len=prompt->len;
      memcpy((*passcode)->data, prompt->result, prompt->len);
      (*passcode)->data[(*passcode)->len]=0;

      return SASL_OK;
  }


  /* Try to get the callback... */
  result = params->utils->getcallback(params->utils->conn,
				      SASL_CB_PASS,
				      &getpass_cb,
				      &getpass_context);

  if (result == SASL_OK && getpass_cb)
    result = getpass_cb(params->utils->conn,
			getpass_context,
			SASL_CB_PASS,
			passcode);

  return result;
}

/*
 * Make the necessary prompts
 */
static int make_prompts(sasl_client_params_t *params,
			sasl_interact_t **prompts_res,
			int user_res,
			int auth_res,
			int pass_res)
{
  int num=1;
  sasl_interact_t *prompts;

  if (user_res==SASL_INTERACT) num++;
  if (auth_res==SASL_INTERACT) num++;
  if (pass_res==SASL_INTERACT) num++;

  if (num==1) {
      SETERROR( params->utils, "make_prompts called with no actual prompts" );
      return SASL_FAIL;
  }

  prompts=params->utils->malloc(sizeof(sasl_interact_t)*(num+1));
  if ((prompts) ==NULL) {
      MEMERROR( params->utils );
      return SASL_NOMEM;
  }
  
  *prompts_res=prompts;

  if (user_res==SASL_INTERACT)
  {
    /* We weren't able to get the callback; let's try a SASL_INTERACT */
    (prompts)->id=SASL_CB_USER;
    (prompts)->challenge="Authzidization Name";
    (prompts)->prompt="Please enter your authzidization name";
    (prompts)->defresult=NULL;

    prompts++;
  }

  if (auth_res==SASL_INTERACT)
  {
    /* We weren't able to get the callback; let's try a SASL_INTERACT */
    (prompts)->id=SASL_CB_AUTHNAME;
    (prompts)->challenge="Authidtication Name";
    (prompts)->prompt="Please enter your authidtication name";
    (prompts)->defresult=NULL;

    prompts++;
  }


  if (pass_res==SASL_INTERACT)
  {
    /* We weren't able to get the callback; let's try a SASL_INTERACT */
    (prompts)->id=SASL_CB_PASS;
    (prompts)->challenge="Passcode";
    (prompts)->prompt="Please enter your passcode";
    (prompts)->defresult=NULL;

    prompts++;
  }

  /* add the ending one */
  (prompts)->id=SASL_CB_LIST_END;
  (prompts)->challenge=NULL;
  (prompts)->prompt   =NULL;
  (prompts)->defresult=NULL;

  return SASL_OK;
}



static int securid_client_mech_step(void *conn_context,
				  sasl_client_params_t *params,
				  const char *serverin __attribute__((unused)),
				  unsigned serverinlen __attribute__((unused)),
				  sasl_interact_t **prompt_need,
				  const char **clientout,
				  unsigned *clientoutlen,
				  sasl_out_params_t *oparams)
{
  int result, ret;
  const char *user, *authid;
  
  context_t *text;
  text=conn_context;

  /* set oparams */
  oparams->mech_ssf=0;
  oparams->maxoutbuf=0;
  oparams->encode=NULL;
  oparams->decode=NULL;

  oparams->param_version = 0;

  if (text->state > 2)
    return SASL_FAIL; /* should never get here */

  if (text->state == 1) {
    int user_result=SASL_OK;
    int auth_result=SASL_OK;
    int pass_result=SASL_OK;

    /* check if sec layer strong enough */
    if (params->props.min_ssf>0+params->external_ssf) {
	SETERROR( params->utils, "The SECURID plugin cannot support any SSF");
	return SASL_TOOWEAK;
    }

    /* try to get the authid */    
    if (oparams->authid==NULL)
    {
      auth_result=get_authid(params,
			     &authid,
			     prompt_need);

      if ((auth_result!=SASL_OK) && (auth_result!=SASL_INTERACT))
	return auth_result;
    }			

    /* try to get the userid */
    if (oparams->user==NULL)
    {
      user_result=get_userid(params,
			     &user,
			     prompt_need);

      /* Fallback to authid */
      if ((user_result!=SASL_OK) && (user_result!=SASL_INTERACT)) {
	  user = authid;
      }
    }

    /* try to get the passcode */
    if (text->passcode==NULL)
    {
      pass_result=get_passcode(params,
			       &text->passcode,
			       prompt_need);
      
      if ((pass_result!=SASL_OK) && (pass_result!=SASL_INTERACT))
	return pass_result;
    }

    /* free prompts we got */
    if (prompt_need && *prompt_need) {
	params->utils->free(*prompt_need);
	*prompt_need = NULL;
    }

    /* if there are prompts not filled in */
    if ((user_result==SASL_INTERACT) || (auth_result==SASL_INTERACT) ||
	(pass_result==SASL_INTERACT))
    {
      /* make the prompt list */
      result=make_prompts(params,prompt_need,
			  user_result, auth_result, pass_result);
      if (result!=SASL_OK) return result;
      
      return SASL_INTERACT;
    }
    
    ret = params->canon_user(params->utils->conn, user, 0,
			     SASL_CU_AUTHZID, oparams);
    if(ret != SASL_OK) return ret;
    ret = params->canon_user(params->utils->conn, authid, 0,
			     SASL_CU_AUTHID, oparams);
    if(ret != SASL_OK) return ret;
    
    if (!text->passcode) {
	PARAMERROR(params->utils);
	return SASL_BADPARAM;
    }
  }

  if (text->state == 2) {
    const char *request;
    const char *pin;
    unsigned lup=0;
    int user_result=SASL_OK;
    int auth_result=SASL_OK;
    int pass_result=SASL_OK;

    /* should have received "passcode" NUL | "pin" NUL [ suggested-pin NUL ] */

    /* get request */
    request = serverin;
    while ((lup < serverinlen) && (serverin[lup] != 0))
      ++lup;

    if (!strcmp(request, "passcode")) {
	text->need_pin = 0;
    }
    else if (!strcmp(request, "pin")) {
	text->need_pin = 1;

	if (lup < serverinlen) {
	    /* get pin */
	    lup++;
	    pin = serverin + lup;
	    while ((lup < serverinlen) && (serverin[lup] != 0))
		++lup;
	}
    }
    else {
	params->utils->seterror(params->utils->conn,
				"unknown SECURID server request '%s'\n",
				request);
	return SASL_BADPROT;
    }

    if (lup >= serverinlen) {
	SETERROR(params->utils, "Got more data than we were expecting in the SECURID plugin\n");
	return SASL_BADPROT;
    }

    /* free previous passcode info */
    _plug_free_secret(params->utils, &(text->passcode));

    /* try to get the passcode */
    if (text->passcode==NULL)
    {
      pass_result=get_passcode(params,
			       &text->passcode,
			       prompt_need);
      
      if ((pass_result!=SASL_OK) && (pass_result!=SASL_INTERACT))
	return pass_result;
    }

    /* free prompts we got */
    if (prompt_need && *prompt_need) {
	params->utils->free(*prompt_need);
	*prompt_need = NULL;
    }

    /* if there are prompts not filled in */
    if ((user_result==SASL_INTERACT) || (auth_result==SASL_INTERACT) ||
	(pass_result==SASL_INTERACT))
    {
      /* make the prompt list */
      result=make_prompts(params,prompt_need,
			  user_result, auth_result, pass_result);
      if (result!=SASL_OK) return result;
      
      return SASL_INTERACT;
    }
    
    if (!text->passcode) {
	PARAMERROR(params->utils);
	return SASL_BADPARAM;
    }
  }

  /* send authzid id NUL authid id NUL passcode NUL [ pin NUL ] */
  *clientoutlen = (oparams->ulen + 1
		   + oparams->alen + 1
		   + text->passcode->len + 1);

  result = _plug_buf_alloc(params->utils, &(text->out_buf),
			   &(text->out_buf_len), *clientoutlen);
  if(result != SASL_OK) return result;

  memset(text->out_buf, 0, *clientoutlen);
  memcpy(text->out_buf, oparams->user, oparams->ulen);
  memcpy(text->out_buf+oparams->ulen+1, oparams->authid, oparams->alen);
  memcpy(text->out_buf+oparams->ulen+oparams->alen+2,
	 text->passcode->data,
	 text->passcode->len);

  *clientout=text->out_buf;

  text->state++;

  return SASL_CONTINUE;
}

static sasl_client_plug_t securid_client_plugins[] = 
{
  {
    "SECURID",
    0,
    SASL_SEC_NOPLAINTEXT | SASL_SEC_NOANONYMOUS | SASL_SEC_FORWARD_SECRECY,
    SASL_FEAT_WANT_CLIENT_FIRST,
    NULL,
    NULL,
    &securid_client_mech_new,
    &securid_client_mech_step,
    &securid_both_mech_dispose,
    &securid_both_mech_free,
    NULL,
    NULL,
    NULL
  }
};

int securid_client_plug_init(sasl_utils_t *utils,
			   int maxversion,
			   int *out_version,
			   sasl_client_plug_t **pluglist,
			   int *plugcount)
{
    if (maxversion<SASL_CLIENT_PLUG_VERSION) {
	SETERROR(utils, "SECURID version mismatch");
	return SASL_BADVERS;
    }

    *pluglist=securid_client_plugins;

    *plugcount=1;
    *out_version=SASL_CLIENT_PLUG_VERSION;

    return SASL_OK;
}
