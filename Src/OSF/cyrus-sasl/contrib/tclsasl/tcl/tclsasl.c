/* tclsasl.c - tcl interface to Cyrus SASLv2 */

/*
 * TBD:
 *
 * - add propctx calls from "sasl/prop.h"
 *
 * - handle property_{names,values} in security properties
 *
 */

#include <config.h>

#ifdef  TCLSASL

#define unused  dummy __attribute__ ((unused))

#include <tcl.h>
#include <stdio.h>
#include <stdlib.h>
#include "sasl.h"
#include "saslutil.h"


/* common datastructures */


struct kv_pair {
  char    *p_key;
  unsigned p_value;
};

typedef struct kv_pair kv_pair_t;


struct oc_pair {
    char           *p_key;
    Tcl_ObjCmdProc *p_proc;
};

typedef struct oc_pair oc_pair_t;


struct sasl_data {
    Tcl_Interp      *sd_interp;
    Tcl_Command      sd_token;

    oc_pair_t       *sd_dispatch; 
    sasl_conn_t     *sd_conn;
    sasl_callback_t *sd_cb;
};

typedef struct sasl_data sasl_data_t;


/* static variables */

static
Tcl_HashTable   allocTable;     /* char*        -> refcnt               */

static
Tcl_HashTable   connTable;      /* sasl_conn_t* -> sasl_data_t*         */

static
Tcl_HashTable   p2tTable;       /* propctx_t*   -> Tcl_Obj*             */
static
Tcl_HashTable   t2pTable;       /* Tcl_Obj*     -> propctx_t*           */


/* args management */

#define Tcl_Obj2String(o)       (((o) != NULL) ? Tcl_GetString (o) : NULL)


/* avoid a bug in Tcl's index caching code... */

static int
tcl_GetIndexFromObjStruct (Tcl_Interp *interp,
                           Tcl_Obj    *objPtr,
                           char      **tablePtr,
                           int         offset,
                           char       *msg,
                           int         flags,
                           int        *indexPtr) {
    Tcl_GetCharLength (objPtr);

    return Tcl_GetIndexFromObjStruct (interp, objPtr, tablePtr, offset, msg,
                                      flags, indexPtr);
}


static int
crack_args (Tcl_Interp *interp,
            int          objc,
            Tcl_Obj     *CONST objv[],
            kv_pair_t   *switches,
            int          optional,
            Tcl_Obj     *args[]) {
    int     i,
            result;

    Tcl_ResetResult (interp);
    for (objc--, objv++; objc > 0; objc -= 2, objv += 2) {
        int        offset;
        kv_pair_t *p;

        if ((result = tcl_GetIndexFromObjStruct (interp, objv[0],
                                                 (char **) switches,
                                                 sizeof *switches, "switch",
                                                 0, &offset)) != TCL_OK)
            return result;
        p = switches + offset;

        if (objc < 2) {
            Tcl_AppendResult (interp, "missing argument to \"",
                              Tcl_GetString (objv[0]), "\"", NULL);
            return TCL_ERROR;
        }
        args[p -> p_value] = objv[1];   
    }

    for (i = 0; i < optional; i++)
        if (!args[i]) {
            Tcl_AppendResult (interp, switches[i].p_key,
                              " switch must be provided", NULL);
            return TCL_ERROR;
        }

    return TCL_OK;
}

static int
t2c_usage (Tcl_Interp *interp,
           Tcl_Obj    *objPtr,
           kv_pair_t   pairs[],
           unsigned    optional,
           char       *operation,
           kv_pair_t   flags[]) {
    kv_pair_t *p;

    Tcl_ResetResult (interp);
    Tcl_AppendResult (interp, "usage: ", Tcl_GetString (objPtr), NULL);
    for (p = pairs; p -> p_key; p++) {
        char     buffer[BUFSIZ],
                  *cp,
                  *v;
        kv_pair_t *f;

        if (((v = operation) != NULL)
                && Tcl_StringCaseMatch ("-operation", p -> p_key, 0)) {}
        else if ((flags != NULL)
                     && Tcl_StringCaseMatch ("-flags", p -> p_key, 0)) {
            cp = buffer;
            for (f = flags, v = "{"; f -> p_key; f++, v = " ") {
                sprintf (cp, "%s%s", v, f -> p_key);
                cp += strlen (cp);
            }
            strcpy (cp, "}");
            v = buffer;
        } else
            v = "...";
        
        Tcl_AppendResult (interp, p -> p_value >= optional ? " ?" : " ",
                          p -> p_key, " ", v,
                          p -> p_value >= optional ? "?" : "", NULL);
    }

    return TCL_ERROR;
}


static int
t2c_flags (Tcl_Interp *interp,
           Tcl_Obj    *objPtr,
           kv_pair_t   pairs[],
           unsigned   *flags) {
    int      i,
             nelem,
             result;

    *flags = 0;
    if (!objPtr)
        return TCL_OK;

    if ((result = Tcl_ListObjLength (interp, objPtr, &nelem)) != TCL_OK)
        return result;

    for (i = 0; i < nelem; i++) {
        int        offset;
        kv_pair_t *p;
        Tcl_Obj   *elemPtr;

        if ((result = Tcl_ListObjIndex (interp, objPtr, i, &elemPtr))
                != TCL_OK)
            return result;

        if ((result = tcl_GetIndexFromObjStruct (interp, elemPtr,
                                                 (char **) pairs,
                                                 sizeof *pairs,
                                                 "flag", 0, &offset))
                != TCL_OK)
            return result;
        p = pairs + offset;

        *flags |= p -> p_value;
    }

    return TCL_OK;
}


/* result management */

static int
c2t_result (Tcl_Interp *interp,
            char       *fnx,
            int         result) {
    char   *cp,
            buffer[BUFSIZ];

    Tcl_ResetResult (interp);
    if (result == SASL_OK)
        return TCL_OK;

    Tcl_SetResult (interp, cp = (char *) sasl_errstring (result, NULL, NULL),
                   TCL_VOLATILE);

    sprintf (buffer, "%d", result);
    Tcl_SetErrorCode (interp, "SASL", fnx, buffer, cp, NULL);

    return TCL_ERROR;
}


/* propctx management */

typedef struct propctx propctx_t;

static Tcl_Obj *
c2t_propctx (propctx_t *propctx) {
    int            result;      
    char           buffer[BUFSIZ];
    Tcl_HashEntry *entryPtr;
    Tcl_Obj       *resultPtr;
    static int     nprop = 0;

    if ((entryPtr = Tcl_FindHashEntry (&p2tTable, (char *) propctx)) != NULL)
        return ((Tcl_Obj *) Tcl_GetHashValue (entryPtr));

    sprintf (buffer, "::sasl::propctx_%d", nprop++);
    resultPtr = Tcl_NewStringObj (buffer, -1);

    entryPtr = Tcl_CreateHashEntry (&p2tTable, (char *) propctx, &result);
    Tcl_SetHashValue (entryPtr, (ClientData) resultPtr);
    Tcl_IncrRefCount (resultPtr);

    entryPtr = Tcl_CreateHashEntry (&t2pTable, (char *) resultPtr, &result);
    Tcl_SetHashValue (entryPtr, (ClientData) propctx);

    return resultPtr;
}

#if     0
static propctx_t *
t2c_propctx (Tcl_Interp *interp,
             Tcl_Obj    *propctx) {
    Tcl_HashEntry *entryPtr;

    if ((entryPtr = Tcl_FindHashEntry (&t2pTable, (char *) propctx)) != NULL)
        return ((propctx_t *) Tcl_GetHashValue (entryPtr));

    Tcl_ResetResult (interp);
    Tcl_AppendResult (interp, "unknown propctx token: \"",
                      Tcl_GetString (propctx), "\"", NULL);

    return NULL;
}
#endif


/* callback management */

struct cb_context {
    Tcl_Interp *ctx_interp;
    int         ctx_id;
    Tcl_Obj    *ctx_cmdPtr;
};

typedef struct cb_context cb_context_t;


static kv_pair_t cb_pairs[] = {
  { "authname",                 SASL_CB_AUTHNAME                },
  { "canonuser",                SASL_CB_CANON_USER              },
  { "cnonce",                   SASL_CB_CNONCE                  },
  { "echoprompt",               SASL_CB_ECHOPROMPT              },
  { "getopt",                   SASL_CB_GETOPT                  },
  { "getpath",                  SASL_CB_GETPATH                 },
  { "getrealm",                 SASL_CB_GETREALM                },
  { "language",                 SASL_CB_LANGUAGE                },
  { "log",                      SASL_CB_LOG                     },
  { "noechoprompt",             SASL_CB_NOECHOPROMPT            },
  { "pass",                     SASL_CB_PASS                    },
  { "proxy_policy",             SASL_CB_PROXY_POLICY            },
  { "checkpass",                SASL_CB_SERVER_USERDB_CHECKPASS },
  { "setpass",                  SASL_CB_SERVER_USERDB_SETPASS   },
  { "user",                     SASL_CB_USER                    },
  { "verifyfile",               SASL_CB_VERIFYFILE              },

  { NULL,                       SASL_CB_LIST_END                }
};


static char *
allocate (Tcl_Interp *interp,
          int        *intPtr) {
    int            len;
    char          *cp,
                  *dp;
    Tcl_HashEntry *entryPtr;

    cp = Tcl_GetByteArrayFromObj (Tcl_GetObjResult (interp), &len);
    memcpy (dp = Tcl_Alloc (len + 1), cp, len);
    dp[len] = '\0';
    if (intPtr)
        *intPtr = len;

    entryPtr = Tcl_CreateHashEntry (&allocTable, dp, &len);
    Tcl_SetHashValue (entryPtr, (ClientData) 0);

    return dp;
}


static Tcl_Obj *
sd2Obj (sasl_data_t *sd) {
    char        *cp = Tcl_GetCommandName (sd -> sd_interp, sd -> sd_token);
    Tcl_Obj *resultPtr = Tcl_NewObj ();

    if (*cp != ':')
        Tcl_AppendToObj (resultPtr, "::sasl::", -1);
    Tcl_AppendToObj (resultPtr, cp, -1);

    return resultPtr;
}


static int
cb_getopt (void *context,
           char *CONST plugin_name,
           char *CONST option,
           char **CONST value,
           unsigned *len) {
    cb_context_t *cb = (cb_context_t *) context;
    Tcl_Interp   *interp = cb -> ctx_interp;
    Tcl_Obj      *argPtr,
                 *cmdPtr;

    argPtr = Tcl_NewObj ();

    if (plugin_name) {
        Tcl_ListObjAppendElement (interp, argPtr,
                                  Tcl_NewStringObj ("plugin", -1));
        Tcl_ListObjAppendElement (interp, argPtr,
                                  Tcl_NewStringObj (plugin_name, -1));
    }

    Tcl_ListObjAppendElement (interp, argPtr, Tcl_NewStringObj ("option", -1));
    Tcl_ListObjAppendElement (interp, argPtr, Tcl_NewStringObj (option, -1));

    cmdPtr = Tcl_DuplicateObj (cb -> ctx_cmdPtr);
    if ((Tcl_ListObjAppendElement (interp, cmdPtr, argPtr) != TCL_OK)
            || (Tcl_EvalObjEx (interp, cmdPtr, TCL_EVAL_DIRECT) != TCL_OK))
        return SASL_FAIL;

    *value = allocate (interp, len);

    return SASL_OK;
}

static int
cb_log (void *context,
        int   level,
        char *CONST message) {
    cb_context_t *cb = (cb_context_t *) context;
    Tcl_Interp   *interp = cb -> ctx_interp;
    Tcl_Obj      *argPtr,
                 *cmdPtr;

    argPtr = Tcl_NewObj ();

    Tcl_ListObjAppendElement (interp, argPtr, Tcl_NewStringObj ("level", -1));
    Tcl_ListObjAppendElement (interp, argPtr, Tcl_NewIntObj (level));

    Tcl_ListObjAppendElement (interp, argPtr,
                              Tcl_NewStringObj ("message", -1));
    Tcl_ListObjAppendElement (interp, argPtr, Tcl_NewStringObj (message, -1));

    cmdPtr = Tcl_DuplicateObj (cb -> ctx_cmdPtr);
    if ((Tcl_ListObjAppendElement (interp, cmdPtr, argPtr) != TCL_OK)
            || (Tcl_EvalObjEx (interp, cmdPtr, TCL_EVAL_DIRECT) != TCL_OK))
        return SASL_FAIL;

    return SASL_OK;
}

static int
cb_getpath (void *context,
            char **CONST value) {
    cb_context_t *cb = (cb_context_t *) context;
    Tcl_Interp   *interp = cb -> ctx_interp;

    if (Tcl_EvalObjEx (interp, cb -> ctx_cmdPtr, 0) != TCL_OK)
        return SASL_FAIL;

    *value = allocate (interp, NULL);

    return SASL_OK;
}

static kv_pair_t vf_pairs[] = {
                                /* SASL_VRFY_OTHER must be first */
  { "other",                    SASL_VRFY_OTHER                 },
  { "conf",                     SASL_VRFY_CONF                  },
  { "passwd",                   SASL_VRFY_PASSWD                },
  { "plugin",                   SASL_VRFY_PLUGIN                },

  { NULL,                       0                               }
};

static int
cb_verifyfile (void *context,
               char *CONST file,
               sasl_verify_type_t type) {
    int           intValue;
    cb_context_t *cb = (cb_context_t *) context;
    kv_pair_t    *p;
    Tcl_Interp   *interp = cb -> ctx_interp;
    Tcl_Obj      *argPtr,
                 *cmdPtr;

    for (p = vf_pairs; p -> p_key; p++)
        if (p -> p_value == type)
            break;
    if (!p -> p_key)
        p = vf_pairs;

    argPtr = Tcl_NewObj ();

    Tcl_ListObjAppendElement (interp, argPtr, Tcl_NewStringObj ("file", -1));
    Tcl_ListObjAppendElement (interp, argPtr, Tcl_NewStringObj (file, -1));

    Tcl_ListObjAppendElement (interp, argPtr, Tcl_NewStringObj ("type", -1));
    Tcl_ListObjAppendElement (interp, argPtr,
                              Tcl_NewStringObj (p -> p_key, -1));

    cmdPtr = Tcl_DuplicateObj (cb -> ctx_cmdPtr);
    if ((Tcl_ListObjAppendElement (interp, cmdPtr, argPtr) != TCL_OK)
            || (Tcl_EvalObjEx (interp, cmdPtr, TCL_EVAL_DIRECT) != TCL_OK)
            || (Tcl_GetIntFromObj (interp, Tcl_GetObjResult (interp),
                                   &intValue) != TCL_OK))
        return SASL_FAIL;

    return intValue;
}

static int
cb_getsimple (void     *context,
              unsigned  id,
              char    **CONST value,
              unsigned *len) {
    cb_context_t *cb = (cb_context_t *) context;
    kv_pair_t    *p;
    Tcl_Interp   *interp = cb -> ctx_interp;
    Tcl_Obj      *argPtr,
                 *cmdPtr;

    for (p = cb_pairs; p -> p_key; p++)
        if (p -> p_value == id)
            break;
    if (!p -> p_key)
        return SASL_BADPARAM;

    argPtr = Tcl_NewObj ();

    Tcl_ListObjAppendElement (interp, argPtr, Tcl_NewStringObj ("id", -1));
    Tcl_ListObjAppendElement (interp, argPtr,
                              Tcl_NewStringObj (p -> p_key, -1));

    cmdPtr = Tcl_DuplicateObj (cb -> ctx_cmdPtr);
    if ((Tcl_ListObjAppendElement (interp, cmdPtr, argPtr) != TCL_OK)
            || (Tcl_EvalObjEx (interp, cmdPtr, TCL_EVAL_DIRECT) != TCL_OK))
        return SASL_FAIL;

    *value = allocate (interp, len);

    return SASL_OK;
}

static int
cb_getsecret (sasl_conn_t    *conn,
              void           *context,
              unsigned        id,
              sasl_secret_t **value) {
    int            len;
    char          *cp,
                  *dp;
    cb_context_t  *cb = (cb_context_t *) context;
    kv_pair_t     *p;
    sasl_data_t   *sd;
    sasl_secret_t *ss;
    Tcl_HashEntry *entryPtr;
    Tcl_Interp    *interp = cb -> ctx_interp;
    Tcl_Obj       *argPtr,
                  *cmdPtr;

    if (!(entryPtr = Tcl_FindHashEntry (&connTable, (char *) conn)))
        return SASL_BADPARAM;
    sd = (sasl_data_t *) Tcl_GetHashValue (entryPtr);

    for (p = cb_pairs; p -> p_key; p++)
        if (p -> p_value == id)
            break;
    if (!p -> p_key)
        return SASL_BADPARAM;

    argPtr = Tcl_NewObj ();

    Tcl_ListObjAppendElement (interp, argPtr, Tcl_NewStringObj ("token", -1));
    Tcl_ListObjAppendElement (interp, argPtr, sd2Obj (sd));

    Tcl_ListObjAppendElement (interp, argPtr, Tcl_NewStringObj ("id", -1));
    Tcl_ListObjAppendElement (interp, argPtr,
                              Tcl_NewStringObj (p -> p_key, -1));

    cmdPtr = Tcl_DuplicateObj (cb -> ctx_cmdPtr);
    if ((Tcl_ListObjAppendElement (interp, cmdPtr, argPtr) != TCL_OK)
            || (Tcl_EvalObjEx (interp, cmdPtr, TCL_EVAL_DIRECT) != TCL_OK)) {
        *value = NULL;
        return SASL_FAIL;
    }

    cp = Tcl_GetByteArrayFromObj (Tcl_GetObjResult (interp), &len);
    dp = Tcl_Alloc (sizeof *ss + len);

    ss = (sasl_secret_t *) dp;
    ss -> len = len;
    memcpy (ss -> data, cp, len);
    ss -> data[len] = '\0';
    
#if     0       /* looks like the library frees this... */
    entryPtr = Tcl_CreateHashEntry (&allocTable, dp, &len);
    Tcl_SetHashValue (entryPtr, (ClientData) 1);
#endif

    *value = ss;

    return SASL_OK;
}

static int
cb_chalprompt (void      *context,
               unsigned  id,
               char     *CONST challenge,
               char     *CONST prompt,
               char     *CONST defresult,
               char    **CONST value,
               unsigned *len) {
    cb_context_t  *cb = (cb_context_t *) context;
    kv_pair_t     *p;
    Tcl_Interp    *interp = cb -> ctx_interp;
    Tcl_Obj       *argPtr,
                  *cmdPtr;

    for (p = cb_pairs; p -> p_key; p++)
        if (p -> p_value == id)
            break;
    if (!p -> p_key)
        return SASL_BADPARAM;

    argPtr = Tcl_NewObj ();

    Tcl_ListObjAppendElement (interp, argPtr, Tcl_NewStringObj ("id", -1));
    Tcl_ListObjAppendElement (interp, argPtr,
                              Tcl_NewStringObj (p -> p_key, -1));
    Tcl_ListObjAppendElement (interp, argPtr,
                              Tcl_NewStringObj ("challenge", -1));
    Tcl_ListObjAppendElement (interp, argPtr,
                              Tcl_NewStringObj (challenge, -1));

    Tcl_ListObjAppendElement (interp, argPtr,
                              Tcl_NewStringObj ("prompt", -1));
    Tcl_ListObjAppendElement (interp, argPtr,
                              Tcl_NewStringObj (prompt, -1));

    Tcl_ListObjAppendElement (interp, argPtr,
                              Tcl_NewStringObj ("default", -1));
    Tcl_ListObjAppendElement (interp, argPtr,
                              Tcl_NewStringObj (defresult, -1));

    cmdPtr = Tcl_DuplicateObj (cb -> ctx_cmdPtr);
    if ((Tcl_ListObjAppendElement (interp, cmdPtr, argPtr) != TCL_OK)
            || (Tcl_EvalObjEx (interp, cmdPtr, TCL_EVAL_DIRECT) != TCL_OK))
        return SASL_FAIL;

    *value = allocate (interp, len);

    return SASL_OK;
}

static int
cb_getrealm (void      *context,
              unsigned   id,
              char     **CONST availrealms,
              char     **CONST value) {
    cb_context_t  *cb = (cb_context_t *) context;
    kv_pair_t     *p;
    Tcl_Interp    *interp = cb -> ctx_interp;
    Tcl_Obj       *argPtr,
                  *cmdPtr;

    for (p = cb_pairs; p -> p_key; p++)
        if (p -> p_value == id)
            break;
    if (!p -> p_key)
        return SASL_BADPARAM;

    argPtr = Tcl_NewObj ();

    Tcl_ListObjAppendElement (interp, argPtr, Tcl_NewStringObj ("id", -1));
    Tcl_ListObjAppendElement (interp, argPtr, Tcl_NewStringObj (p -> p_key,
                                                                -1));

    if (availrealms) {
        char    **ap;
        Tcl_Obj  *elemPtr;

        elemPtr = Tcl_NewObj ();
        for (ap = availrealms; *ap; ap++) {
            Tcl_ListObjAppendElement (interp, elemPtr,
                                      Tcl_NewStringObj (*ap, -1));
        }
        Tcl_ListObjAppendElement (interp, argPtr,
                                  Tcl_NewStringObj ("available", -1));
        Tcl_ListObjAppendElement (interp, argPtr, elemPtr);
    }

    cmdPtr = Tcl_DuplicateObj (cb -> ctx_cmdPtr);
    if ((Tcl_ListObjAppendElement (interp, cmdPtr, argPtr) != TCL_OK)
            || (Tcl_EvalObjEx (interp, cmdPtr, TCL_EVAL_DIRECT) != TCL_OK))
        return SASL_FAIL;

    *value = allocate (interp, NULL);

    return SASL_OK;
}

static int
cb_authorize (sasl_conn_t *conn,
              void        *context,
              char        *CONST requested_user,
              int          rlen,
              char        *CONST auth_identity,
              int          alen,
              char        *CONST def_realm,
              int          urlen,
              propctx_t   *propctx) {
    int            intValue;
    cb_context_t  *cb = (cb_context_t *) context;
    sasl_data_t   *sd;
    Tcl_HashEntry *entryPtr;
    Tcl_Interp    *interp = cb -> ctx_interp;
    Tcl_Obj       *argPtr,
                  *cmdPtr;

    if (!(entryPtr = Tcl_FindHashEntry (&connTable, (char *) conn)))
        return SASL_BADPARAM;
    sd = (sasl_data_t *) Tcl_GetHashValue (entryPtr);

    argPtr = Tcl_NewObj ();

    Tcl_ListObjAppendElement (interp, argPtr, Tcl_NewStringObj ("token", -1));
    Tcl_ListObjAppendElement (interp, argPtr, sd2Obj (sd));

    Tcl_ListObjAppendElement (interp, argPtr,
                              Tcl_NewStringObj ("target", -1));
    Tcl_ListObjAppendElement (interp, argPtr,
                              Tcl_NewByteArrayObj (requested_user, rlen));

    Tcl_ListObjAppendElement (interp, argPtr,
                              Tcl_NewStringObj ("user", -1));
    Tcl_ListObjAppendElement (interp, argPtr,
                              Tcl_NewByteArrayObj (auth_identity, alen));

    if (def_realm) {
        Tcl_ListObjAppendElement (interp, argPtr,
                                  Tcl_NewStringObj ("realm", -1));
        Tcl_ListObjAppendElement (interp, argPtr,
                                  Tcl_NewByteArrayObj (def_realm, urlen));
    }

    if (propctx) {
        Tcl_ListObjAppendElement (interp, argPtr,
                                  Tcl_NewStringObj ("propctx", -1));
        Tcl_ListObjAppendElement (interp, argPtr, c2t_propctx (propctx));
    }

    cmdPtr = Tcl_DuplicateObj (cb -> ctx_cmdPtr);
    if ((Tcl_ListObjAppendElement (interp, cmdPtr, argPtr) != TCL_OK)
            || (Tcl_EvalObjEx (interp, cmdPtr, TCL_EVAL_DIRECT) != TCL_OK)
            || (Tcl_GetIntFromObj (interp, Tcl_GetObjResult (interp),
                                   &intValue) != TCL_OK))
        return SASL_FAIL;

    return intValue;
}

static int
cb_userdb_checkpass (sasl_conn_t *conn,
                     void        *context,
                     char        *CONST user,
                     char        *CONST pass,
                     unsigned     passlen,
                     propctx_t   *propctx) {
    int            intValue;
    cb_context_t  *cb = (cb_context_t *) context;
    sasl_data_t   *sd;
    Tcl_HashEntry *entryPtr;
    Tcl_Interp    *interp = cb -> ctx_interp;
    Tcl_Obj       *argPtr,
                  *cmdPtr;

    if (!(entryPtr = Tcl_FindHashEntry (&connTable, (char *) conn)))
        return SASL_BADPARAM;
    sd = (sasl_data_t *) Tcl_GetHashValue (entryPtr);

    argPtr = Tcl_NewObj ();

    Tcl_ListObjAppendElement (interp, argPtr, Tcl_NewStringObj ("token", -1));
    Tcl_ListObjAppendElement (interp, argPtr, sd2Obj (sd));

    Tcl_ListObjAppendElement (interp, argPtr, Tcl_NewStringObj ("user", -1));
    Tcl_ListObjAppendElement (interp, argPtr, Tcl_NewStringObj (user, -1));

    Tcl_ListObjAppendElement (interp, argPtr, Tcl_NewStringObj ("pass", -1));
    Tcl_ListObjAppendElement (interp, argPtr, Tcl_NewByteArrayObj (pass,
                                                                   passlen));

    if (propctx) {
        Tcl_ListObjAppendElement (interp, argPtr,
                                  Tcl_NewStringObj ("propctx", -1));
        Tcl_ListObjAppendElement (interp, argPtr, c2t_propctx (propctx));
    }

    cmdPtr = Tcl_DuplicateObj (cb -> ctx_cmdPtr);
    if ((Tcl_ListObjAppendElement (interp, cmdPtr, argPtr) != TCL_OK)
            || (Tcl_EvalObjEx (interp, cmdPtr, TCL_EVAL_DIRECT) != TCL_OK)
            || (Tcl_GetIntFromObj (interp, Tcl_GetObjResult (interp),
                                   &intValue) != TCL_OK))
        return SASL_FAIL;

    return intValue;
}

static kv_pair_t setpass_flags[] = {
  { "create",           SASL_SET_CREATE                         },
  { "disable",          SASL_SET_DISABLE                        },

  { NULL,                       0                               }
};

static int
cb_userdb_setpass (sasl_conn_t *conn,
                   void        *context,
                   char        *CONST user,
                   char        *CONST pass,
                   unsigned     passlen,
                   propctx_t   *propctx,
                   unsigned     flags) {
    int            intValue;
    cb_context_t  *cb = (cb_context_t *) context;
    kv_pair_t     *p;
    sasl_data_t   *sd;
    Tcl_HashEntry *entryPtr;
    Tcl_Interp    *interp = cb -> ctx_interp;
    Tcl_Obj       *argPtr,
                  *cmdPtr,
                  *elemPtr;

    if (!(entryPtr = Tcl_FindHashEntry (&connTable, (char *) conn)))
        return SASL_BADPARAM;
    sd = (sasl_data_t *) Tcl_GetHashValue (entryPtr);

    argPtr = Tcl_NewObj ();

    Tcl_ListObjAppendElement (interp, argPtr, Tcl_NewStringObj ("token", -1));
    Tcl_ListObjAppendElement (interp, argPtr, sd2Obj (sd));

    Tcl_ListObjAppendElement (interp, argPtr, Tcl_NewStringObj ("user", -1));
    Tcl_ListObjAppendElement (interp, argPtr, Tcl_NewStringObj (user, -1));

    Tcl_ListObjAppendElement (interp, argPtr, Tcl_NewStringObj ("pass", -1));
    Tcl_ListObjAppendElement (interp, argPtr,
                              Tcl_NewByteArrayObj (pass, passlen));

    Tcl_ListObjAppendElement (interp, argPtr,
                              Tcl_NewStringObj ("flags", -1));
    elemPtr = Tcl_NewObj ();
    for (p = setpass_flags; p -> p_key; p++)
        if (flags & p -> p_value) {
            Tcl_ListObjAppendElement (interp, elemPtr,
                                      Tcl_NewStringObj (p -> p_key, -1));
            flags &= ~p -> p_value;
        }
    if (flags) {
        char    buffer[BUFSIZ];

        sprintf (buffer, "%u", flags);
        Tcl_ListObjAppendElement (interp, elemPtr,
                                  Tcl_NewStringObj (buffer, -1));
    }
    Tcl_ListObjAppendElement (interp, argPtr, elemPtr);

    if (propctx) {
        Tcl_ListObjAppendElement (interp, argPtr,
                                  Tcl_NewStringObj ("propctx", -1));
        Tcl_ListObjAppendElement (interp, argPtr, c2t_propctx (propctx));
    }

    cmdPtr = Tcl_DuplicateObj (cb -> ctx_cmdPtr);
    if ((Tcl_ListObjAppendElement (interp, cmdPtr, argPtr) != TCL_OK)
            || (Tcl_EvalObjEx (interp, cmdPtr, TCL_EVAL_DIRECT) != TCL_OK)
            || (Tcl_GetIntFromObj (interp, Tcl_GetObjResult (interp),
                                   &intValue) != TCL_OK))
        return SASL_FAIL;

    return intValue;
}

static kv_pair_t canon_flags[] = {
  { "authid",           SASL_CU_AUTHID                          },
  { "authzid",          SASL_CU_AUTHZID                         },

  { NULL,                       0                               }
};

static int
cb_canonuser (sasl_conn_t *conn,
               void        *context,
               char        *CONST iptr,
               unsigned     ilen,
               unsigned     flags,
               char        *CONST realm,
               char        *optr,
               unsigned     omax,
               unsigned    *olen) {
    int            intValue;
    char          *cp;
    cb_context_t  *cb = (cb_context_t *) context;
    kv_pair_t     *p;
    sasl_data_t   *sd;
    Tcl_HashEntry *entryPtr;
    Tcl_Interp    *interp = cb -> ctx_interp;
    Tcl_Obj       *argPtr,
                  *cmdPtr,
                  *elemPtr;

    if (!(entryPtr = Tcl_FindHashEntry (&connTable, (char *) conn)))
        return SASL_BADPARAM;
    sd = (sasl_data_t *) Tcl_GetHashValue (entryPtr);

    argPtr = Tcl_NewObj ();

    Tcl_ListObjAppendElement (interp, argPtr, Tcl_NewStringObj ("token", -1));
    Tcl_ListObjAppendElement (interp, argPtr, sd2Obj (sd));

    Tcl_ListObjAppendElement (interp, argPtr, Tcl_NewStringObj ("in", -1));
    Tcl_ListObjAppendElement (interp, argPtr, Tcl_NewByteArrayObj (iptr,
                                                                   ilen));

    if (realm) {
        Tcl_ListObjAppendElement (interp, argPtr,
                                  Tcl_NewStringObj ("realm", -1));
        Tcl_ListObjAppendElement (interp, argPtr,
                                  Tcl_NewStringObj (realm, -1));
    }

    Tcl_ListObjAppendElement (interp, argPtr, Tcl_NewStringObj ("outmax", -1));
    Tcl_ListObjAppendElement (interp, argPtr, Tcl_NewIntObj (omax));

    Tcl_ListObjAppendElement (interp, argPtr,
                              Tcl_NewStringObj ("flags", -1));
    elemPtr = Tcl_NewObj ();
    for (p = canon_flags; p -> p_key; p++)
        if (flags & p -> p_value) {
            Tcl_ListObjAppendElement (interp, elemPtr,
                                      Tcl_NewStringObj (p -> p_key, -1));
            flags &= ~p -> p_value;
        }
    if (flags) {
        char    buffer[BUFSIZ];

        sprintf (buffer, "%u", flags);
        Tcl_ListObjAppendElement (interp, elemPtr,
                                  Tcl_NewStringObj (buffer, -1));
    }
    Tcl_ListObjAppendElement (interp, argPtr, elemPtr);

    cmdPtr = Tcl_DuplicateObj (cb -> ctx_cmdPtr);
    if ((Tcl_ListObjAppendElement (interp, cmdPtr, argPtr) != TCL_OK)
            || (Tcl_EvalObjEx (interp, cmdPtr, TCL_EVAL_DIRECT) != TCL_OK))
        return SASL_FAIL;

    cp = Tcl_GetByteArrayFromObj (Tcl_GetObjResult (interp), &intValue);
    if (intValue > ((int) omax))
        intValue = omax;
    memcpy (optr, cp, intValue);
    *olen = (unsigned) intValue;

    return SASL_OK;
}


static sasl_callback_t *
t2c_sasl_callback (Tcl_Interp *interp,
                   Tcl_Obj    *objPtr) {
    int              i,
                     nctx,
                     nelem,
                     result,
                     size;
    char            *mp;
    cb_context_t    *cb; 
    sasl_callback_t *pp,
                    *qp;
    Tcl_HashEntry   *entryPtr;
    Tcl_Obj         *elemPtr,
                    *fieldPtr;

    if ((result = Tcl_ListObjLength (interp, objPtr, &nelem)) != TCL_OK)
        return NULL;

    size = (nelem + 1) * sizeof *pp;

    nctx = 0;
    for (i = 0; i < nelem; i++) {
        int     nfield;

        if (((result = Tcl_ListObjIndex (interp, objPtr, i, &elemPtr))
                        != TCL_OK)
                || ((result = Tcl_ListObjLength (interp, elemPtr, &nfield))
                        != TCL_OK))
            return NULL;
        if ((nfield < 1) || (nfield > 2)) {
            Tcl_SetResult (interp,
                           "each callback should be list with 1 or 2 elements",
                           TCL_STATIC);
            return NULL;
        }
        if ((result = Tcl_ListObjIndex (interp, elemPtr, 1, &fieldPtr))
                != TCL_OK)
            return NULL;

        if (fieldPtr != NULL)
            size += sizeof *cb, nctx++;
    }

    mp = Tcl_Alloc (size);
    memset (mp, 0, size);

    pp = (sasl_callback_t *) mp, mp += (nelem + 1) * sizeof *pp;
    cb = (cb_context_t *) mp, mp += nctx * sizeof *cb;

    for (qp = pp, i = 0; i < nelem; qp++, i++) {
        int        offset;

        if (((result = Tcl_ListObjIndex (interp, objPtr, i, &elemPtr))
                        != TCL_OK)
                || ((result = Tcl_ListObjIndex (interp, elemPtr, 0, &fieldPtr))
                        != TCL_OK))
            goto out;

        if ((result = tcl_GetIndexFromObjStruct (interp, fieldPtr,
                                                 (char **) cb_pairs,
                                                 sizeof *cb_pairs, "callback",
                                                 0, &offset)) != TCL_OK)
            goto out;
        qp -> id = cb_pairs[offset]. p_value;

        if ((result = Tcl_ListObjIndex (interp, elemPtr, 1, &fieldPtr))
                != TCL_OK)
            goto out;

        if (fieldPtr != NULL) {
            switch (qp -> id) {
                case SASL_CB_GETOPT:
                    qp -> proc = cb_getopt;
                    break;

                case SASL_CB_LOG:
                    qp -> proc = cb_log;
                    break;

                case SASL_CB_GETPATH:
                    qp -> proc = cb_getpath;
                    break;

                case SASL_CB_VERIFYFILE:
                    qp -> proc = cb_verifyfile;
                    break;

                case SASL_CB_USER:
                case SASL_CB_AUTHNAME:
                case SASL_CB_LANGUAGE:
                case SASL_CB_CNONCE:
                    qp -> proc = cb_getsimple;
                    break;

                case SASL_CB_PASS:
                    qp -> proc = cb_getsecret;
                    break;

                case SASL_CB_ECHOPROMPT:
                case SASL_CB_NOECHOPROMPT:
                    qp -> proc = cb_chalprompt;
                    break;

                case SASL_CB_GETREALM:
                    qp -> proc = cb_getrealm;
                    break;

                case SASL_CB_PROXY_POLICY:
                    qp -> proc = cb_authorize;
                    break;

                case SASL_CB_SERVER_USERDB_CHECKPASS:
                    qp -> proc = cb_userdb_checkpass;
                    break;

                case SASL_CB_SERVER_USERDB_SETPASS:
                    qp -> proc = cb_userdb_setpass;
                    break;

                case SASL_CB_CANON_USER:
                    qp -> proc = cb_canonuser;
                    break;

                default:
                    Tcl_SetResult (interp,
                                   "internal error, missing known case",
                                   TCL_STATIC);
                    goto out;
            }
            qp -> context = cb;

            cb -> ctx_interp = interp;
            cb -> ctx_id = qp -> id;
            cb -> ctx_cmdPtr = fieldPtr;
            Tcl_IncrRefCount (cb -> ctx_cmdPtr);
            cb++;
        }
    }
    qp -> id = SASL_CB_LIST_END;

    entryPtr = Tcl_CreateHashEntry (&allocTable, (char *) pp, &result);
    Tcl_SetHashValue (entryPtr, (ClientData) 1);

    return pp;

out: ;
    Tcl_Free ((char *) pp);

    return NULL;
}


/* common routines */

static int
sasl_aux_proc (ClientData  data,
               Tcl_Interp *interp,
               int         objc,
               Tcl_Obj    *CONST objv[]) {
    int            argc,
                   offset,
                   result;
    char          *cp;
    oc_pair_t     *pp;
    sasl_data_t   *sd = (sasl_data_t *) data;
    Tcl_Obj      *CONST *argv;

    Tcl_ResetResult (interp);
    argc = objc, argv = objv;
    for (argc--, argv++; argc > 0; argc -= 2, argv += 2)
        if (Tcl_StringCaseMatch (Tcl_GetString (*argv), "-operation", 0))
            break;
    if (argc <= 0) {
        Tcl_AppendResult (interp, "usage: ", Tcl_GetString (objv[0]),
                          " -operation", NULL);
        for (pp = sd -> sd_dispatch, cp = " "; pp -> p_key; pp++, cp = "|")
            Tcl_AppendResult (interp, cp, pp -> p_key, NULL);
        Tcl_AppendResult (interp, " ?args...?", NULL);
        return TCL_ERROR;
    }

    argc--, argv++;
    if (!*argv) {
        Tcl_AppendResult (interp, "missing argument to: \"-operation\"", NULL);
        return TCL_ERROR;
    }
    if ((result = tcl_GetIndexFromObjStruct (interp, *argv,
                                             (char **) sd -> sd_dispatch,
                                             sizeof *sd -> sd_dispatch,
                                             "operation", 0, &offset))
             != TCL_OK)
        return result;
    pp = sd -> sd_dispatch + offset;

    return ((*(pp -> p_proc)) (data, interp, objc, objv));
};


static void
sasl_aux_free (ClientData data) {
    sasl_data_t   *sd = (sasl_data_t *) data;
    Tcl_HashEntry *entryPtr;

    if (sd -> sd_cb) {
        if ((entryPtr = Tcl_FindHashEntry (&allocTable, (char *) sd -> sd_cb))
                != NULL)
            Tcl_DeleteHashEntry (entryPtr);
        Tcl_Free ((char *) sd -> sd_cb);
    }

    if ((entryPtr = Tcl_FindHashEntry (&connTable, (char *) sd -> sd_conn))
            != NULL)
        Tcl_DeleteHashEntry (entryPtr);
    sasl_dispose (&sd -> sd_conn);

    Tcl_Free ((char *) data);
}


static int
sasl_aux_codec (Tcl_Interp  *interp,
                sasl_data_t *sd,
                char        *codep,
                int        (*codec) (),
                Tcl_Obj     *string) {
    int              result;
    unsigned         ilen,
                     olen;
    CONST char      *iptr,
                    *optr;

    iptr = Tcl_GetByteArrayFromObj (string, &ilen);

    if ((result = (*codec) (sd -> sd_conn, iptr, ilen, &optr, &olen))
            != SASL_OK)
        return c2t_result (interp, codep, result);

    Tcl_SetObjResult (interp, Tcl_NewByteArrayObj ((unsigned char *) optr,
                                                   olen));
    return TCL_OK;
}


#define DECODE_OPERATION        0
#define DECODE_INPUT            1

#define DECODE_MAXARGS          2
#define DECODE_OPTIONAL         DECODE_MAXARGS

static kv_pair_t decode_args[] = {
  { "-operation",               DECODE_OPERATION                },
  { "-input",                   DECODE_INPUT                    },

  { NULL,                       0                               }
};

static int
sasl_aux_decode (ClientData  data,
                 Tcl_Interp *interp,
                 int         objc,
                 Tcl_Obj    *CONST objv[]) {
    int              result;
    sasl_data_t     *sd = (sasl_data_t *) data;    
    Tcl_Obj         *args[DECODE_MAXARGS];

    memset (args, 0, sizeof *args * DECODE_MAXARGS);
    if ((result = crack_args (interp, objc, objv, decode_args,
                              DECODE_OPTIONAL, args)) != TCL_OK)
        return result;

    return sasl_aux_codec (interp, sd, "sasl_decode", sasl_decode,
                           args[DECODE_INPUT]);
}


#define ENCODE_OPERATION        0
#define ENCODE_OUTPUT           1

#define ENCODE_MAXARGS          2
#define ENCODE_OPTIONAL         ENCODE_MAXARGS

static kv_pair_t encode_args[] = {
  { "-operation",               ENCODE_OPERATION                },
  { "-output",                  ENCODE_OUTPUT                   },

  { NULL,                       0                               }
};

static int
sasl_aux_encode (ClientData  data,
                 Tcl_Interp *interp,
                 int         objc,
                 Tcl_Obj    *CONST objv[]) {
    int              result;
    sasl_data_t     *sd = (sasl_data_t *) data;    
    Tcl_Obj         *args[ENCODE_MAXARGS];

    memset (args, 0, sizeof *args * ENCODE_MAXARGS);
    if ((result = crack_args (interp, objc, objv, encode_args,
                              ENCODE_OPTIONAL, args)) != TCL_OK)
        return result;

    return sasl_aux_codec (interp, sd, "sasl_encode", sasl_encode,
                           args[ENCODE_OUTPUT]);
}


#define GETPROP_OPERATION       0
#define GETPROP_PROPERTY        1

#define GETPROP_MAXARGS         2
#define GETPROP_OPTIONAL        GETPROP_MAXARGS

static kv_pair_t getprop_args[] = {
  { "-operation",               GETPROP_OPERATION               },
  { "-property",                GETPROP_PROPERTY                },

  { NULL,                       0                               }
};

static kv_pair_t getprop_pairs[] = {
  { "auth_external",            SASL_AUTH_EXTERNAL              },
  { "authsource",               SASL_AUTHSOURCE                 },
  { "callbacks",                SASL_CALLBACK                   },
  { "defrealm",                 SASL_DEFUSERREALM               },
  { "getoptctx",                SASL_GETOPTCTX                  },
  { "iplocalport",              SASL_IPLOCALPORT                },
  { "ipremoteport",             SASL_IPREMOTEPORT               },
  { "maxoutbuf",                SASL_MAXOUTBUF                  },
  { "mechname",                 SASL_MECHNAME                   },
  { "plugerr",                  SASL_PLUGERR                    },
  { "sec_props",                SASL_SEC_PROPS                  },
  { "serverfqdn",               SASL_SERVERFQDN                 },
  { "service",                  SASL_SERVICE                    },
  { "ssf",                      SASL_SSF                        },
  { "ssf_external",             SASL_SSF_EXTERNAL               },
  { "username",                 SASL_USERNAME                   },

  { NULL,                       0                               }
};

static kv_pair_t secprops_flags[] = {
  { "forward_secrecy",          SASL_SEC_FORWARD_SECRECY        },
  { "noactive",                 SASL_SEC_NOACTIVE               },
  { "noanonymous",              SASL_SEC_NOANONYMOUS            },
  { "nodictionary",             SASL_SEC_NODICTIONARY           },
  { "noplaintext",              SASL_SEC_NOPLAINTEXT            },
  { "pass_credentials",         SASL_SEC_PASS_CREDENTIALS       },

  { NULL,                       0                               }
};


static int
sasl_aux_getprop (ClientData  data,
                  Tcl_Interp *interp,
                  int          objc,
                  Tcl_Obj     *CONST objv[]) {
    CONST void      *pvalue;
    int              offset,
                     result;
    kv_pair_t       *p;
    sasl_data_t     *sd = (sasl_data_t *) data;    
    Tcl_Obj         *elemPtr,
                    *resultPtr,
                    *args[GETPROP_MAXARGS];

    memset (args, 0, sizeof *args * GETPROP_MAXARGS);
    if ((result = crack_args (interp, objc, objv, getprop_args,
                              GETPROP_OPTIONAL, args)) != TCL_OK)
        return result;

    if ((result = tcl_GetIndexFromObjStruct (interp, args[GETPROP_PROPERTY],
                                             (char **) getprop_pairs,
                                             sizeof *getprop_pairs, "property",
                                             0, &offset)) != TCL_OK)
        return result;
    p = getprop_pairs + offset;

    if ((offset = p -> p_value) == SASL_GETOPTCTX)
        offset = SASL_CALLBACK;
    if ((result = sasl_getprop (sd -> sd_conn, offset, &pvalue)) != SASL_OK)
        return c2t_result (interp, "sasl_getprop", result);

    resultPtr = NULL;
    switch (p -> p_value) {
        case SASL_USERNAME:
        case SASL_DEFUSERREALM:
        case SASL_IPLOCALPORT:
        case SASL_IPREMOTEPORT:
        case SASL_SERVICE:
        case SASL_SERVERFQDN:
        case SASL_AUTHSOURCE:
        case SASL_MECHNAME:
        case SASL_PLUGERR:
        case SASL_AUTH_EXTERNAL:
            if (pvalue != NULL)
                resultPtr = Tcl_NewStringObj ((char *) pvalue, -1);
            break;

        case SASL_SSF:
        case SASL_SSF_EXTERNAL:
        case SASL_MAXOUTBUF:
            resultPtr = Tcl_NewLongObj (*((unsigned *) pvalue));
            break;
        
        case SASL_SEC_PROPS:
        {
            sasl_security_properties_t *sp =
                        (sasl_security_properties_t *) pvalue;

            resultPtr = Tcl_NewObj ();

            Tcl_ListObjAppendElement (interp, resultPtr,
                                     Tcl_NewStringObj ("min_ssf", -1));
            Tcl_ListObjAppendElement (interp, resultPtr,
                                     Tcl_NewLongObj (sp -> min_ssf));

            Tcl_ListObjAppendElement (interp, resultPtr,
                                     Tcl_NewStringObj ("max_ssf", -1));
            Tcl_ListObjAppendElement (interp, resultPtr,
                                     Tcl_NewLongObj (sp -> max_ssf));

            Tcl_ListObjAppendElement (interp, resultPtr,
                                     Tcl_NewStringObj ("max_bufsize", -1));
            Tcl_ListObjAppendElement (interp, resultPtr,
                                     Tcl_NewLongObj (sp -> maxbufsize));

            Tcl_ListObjAppendElement (interp, resultPtr,
                                     Tcl_NewStringObj ("flags", -1));
            elemPtr = Tcl_NewObj ();
            for (p = secprops_flags; p -> p_key; p++)
                if (sp -> security_flags & p -> p_value) {
                    Tcl_ListObjAppendElement (interp, elemPtr,
                                              Tcl_NewStringObj (p -> p_key,
                                                                -1));
                    sp -> security_flags &= ~p -> p_value;
                }
            if (sp -> security_flags) {
                char    buffer[BUFSIZ];

                sprintf (buffer, "%u", sp -> security_flags);
                Tcl_ListObjAppendElement (interp, elemPtr,
                                          Tcl_NewStringObj (buffer, -1));
            }
            Tcl_ListObjAppendElement (interp, resultPtr, elemPtr);

            break;
        }

        case SASL_GETOPTCTX:
            offset = SASL_GETOPTCTX;
        /* and fall... */

        case SASL_CALLBACK:
        {
            cb_context_t    *cb;
            sasl_callback_t *pp,
                            *qp;

            if (!(pp = (sasl_callback_t *) pvalue))
                break;

            if (offset == SASL_CALLBACK)
                resultPtr = Tcl_NewObj ();
            for (qp = pp; qp -> id != SASL_CB_LIST_END; qp++) {
                if (offset == SASL_GETOPTCTX) {
                    if (qp -> id != SASL_CB_GETOPT)
                        continue;
                    if ((cb = (cb_context_t *) qp -> context) != NULL)
                        resultPtr = cb -> ctx_cmdPtr;
                    break;
                }

                for (p = cb_pairs; p -> p_key; p++)
                    if (p -> p_value == qp -> id)
                        break;
                if (!p -> p_key)
                    continue;

                elemPtr = Tcl_NewObj ();

                Tcl_ListObjAppendElement (interp, elemPtr,
                                          Tcl_NewStringObj (p -> p_key, -1));

                if ((cb = (cb_context_t *) qp -> context) != NULL)
                    Tcl_ListObjAppendElement (interp, elemPtr,
                                              cb -> ctx_cmdPtr);

                Tcl_ListObjAppendElement (interp, resultPtr, elemPtr);
            }
            break;
        }

        default:
            Tcl_SetResult (interp, "internal error, missing known case",
                           TCL_STATIC);
            return TCL_ERROR;
    }

    if (resultPtr)
        Tcl_SetObjResult (interp, resultPtr);
    else
        Tcl_ResetResult (interp);
    return TCL_OK;
}


#define SETPROP_OPERATION       0
#define SETPROP_PROPERTY        1
#define SETPROP_VALUE           2

#define SETPROP_MAXARGS         3
#define SETPROP_OPTIONAL        SETPROP_MAXARGS

static kv_pair_t setprop_args[] = {
  { "-operation",               SETPROP_OPERATION               },
  { "-property",                SETPROP_PROPERTY                },
  { "-value",                   SETPROP_VALUE                   },

  { NULL,                       0                               }
};

static kv_pair_t setprop_pairs[] = {
  { "auth_external",            SASL_AUTH_EXTERNAL              },
  { "defrealm",                 SASL_DEFUSERREALM               },
  { "iplocalport",              SASL_IPLOCALPORT                },
  { "ipremoteport",             SASL_IPREMOTEPORT               },
  { "sec_props",                SASL_SEC_PROPS                  },
  { "ssf_external",             SASL_SSF_EXTERNAL               },

  { NULL,                       0                               }
};

#define SECPROPS_FLAGS          0
#define SECPROPS_MAXBUFSIZ      1
#define SECPROPS_MAXSSF         2
#define SECPROPS_MINSSF         3

#define SECPROPS_MAXPROPS       4

static kv_pair_t secprops_pairs[] = {
  { "flags",                    SECPROPS_FLAGS                  },
  { "max_bufsize",              SECPROPS_MAXBUFSIZ              },
  { "max_ssf",                  SECPROPS_MAXSSF                 },
  { "min_ssf",                  SECPROPS_MINSSF                 },

  { NULL,                       0                               }
};


static int
sasl_aux_setprop (ClientData  data,
                  Tcl_Interp *interp,
                  int          objc,
                  Tcl_Obj     *CONST objv[]) {
    CONST void      *pvalue;
    int              offset,
                     result;
    long             ssf;       
    kv_pair_t       *p;
    sasl_data_t     *sd = (sasl_data_t *) data;
    Tcl_Obj         *args[SETPROP_MAXARGS];

    memset (args, 0, sizeof *args * SETPROP_MAXARGS);
    if ((result = crack_args (interp, objc, objv, setprop_args,
                              SETPROP_OPTIONAL, args)) != TCL_OK)
        return result;

    if ((result = tcl_GetIndexFromObjStruct (interp, args[SETPROP_PROPERTY],
                                             (char **) setprop_pairs,
                                             sizeof *setprop_pairs, "property",
                                             0, &offset)) != TCL_OK)
        return result;
    p = setprop_pairs + offset;

    switch (p -> p_value) {
        case SASL_SSF_EXTERNAL:
            if ((result = Tcl_GetLongFromObj (interp, args[SETPROP_VALUE],
                                              &ssf)) != TCL_OK)
                return result;
            pvalue = (void *) &ssf;
            break;
        
        case SASL_AUTH_EXTERNAL:
        case SASL_DEFUSERREALM:
        case SASL_IPLOCALPORT:
        case SASL_IPREMOTEPORT:
            pvalue = (void *) Tcl_GetString (args[SETPROP_VALUE]);
            break;

        case SASL_SEC_PROPS:
            {
                int                         i,
                                            nelem,
                                            props[SECPROPS_MAXPROPS];
                sasl_security_properties_t  sps,
                                           *sp = &sps;

                if ((result = Tcl_ListObjLength (interp,
                                                 args[SETPROP_VALUE], &nelem))
                        != TCL_OK)
                    return result;
                if (nelem % 2) {
                    Tcl_SetResult (interp,
                                   "expecting an even number of list elements",
                                   TCL_STATIC);
                    return TCL_ERROR;
                }

                memset (props, 0, sizeof *props * SECPROPS_MAXPROPS);
                memset (sp, 0, sizeof *sp);
                for (i = 0; i < nelem; i += 2) {
                    int      offset;
                    long     longValue;
                    Tcl_Obj *elemPtr;

                    if ((result = Tcl_ListObjIndex (interp,
                                                    args[SETPROP_VALUE], i,
                                                    &elemPtr)) != TCL_OK)
                        return TCL_ERROR;
                    if ((result =
                             tcl_GetIndexFromObjStruct (interp, elemPtr,
                                                        (char **) secprops_pairs,
                                                        sizeof *secprops_pairs,
                                                        "security property",
                                                        0, &offset))
                            != TCL_OK)
                        return TCL_ERROR;
                    if (props[offset]) {
                        Tcl_ResetResult (interp);
                        Tcl_AppendResult (interp, "property \"",
                                          secprops_pairs[offset].p_key,
                                          "\" appears more than once", NULL);
                        return TCL_ERROR;
                    }
                    props[offset] = 1;

                    if ((result = Tcl_ListObjIndex (interp,
                                                    args[SETPROP_VALUE], i + 1,
                                                    &elemPtr)) != TCL_OK)
                        return TCL_ERROR;

                    switch (offset) {
                        case SECPROPS_FLAGS:
                            if ((result = t2c_flags (interp, elemPtr,
                                                     secprops_flags,
                                                     &sp -> security_flags))
                                    != TCL_OK)
                                return result;
                            break;

                        case SECPROPS_MINSSF:
                        case SECPROPS_MAXSSF:
                        case SECPROPS_MAXBUFSIZ:
                            if ((result = Tcl_GetLongFromObj (interp, elemPtr,
                                                              &longValue))
                                    != TCL_OK)
                                return result;
                            if (offset == SECPROPS_MINSSF)
                                sp -> min_ssf = longValue;
                            else if (offset == SECPROPS_MAXSSF)
                                sp -> max_ssf = longValue;
                            else
                                sp -> maxbufsize = longValue;
                            break;
                    }
                }
                pvalue = (void *) sp;
                break;
            }

        default:
            Tcl_SetResult (interp, "internal error, missing known case",
                           TCL_STATIC);
            return TCL_ERROR;
    }

    return c2t_result (interp, "sasl_setprop",
                       sasl_setprop (sd -> sd_conn, p -> p_value, pvalue));
}


static int
sasl_aux_errdetail (ClientData  data,
                  Tcl_Interp *interp,
                  int         objc,
                  Tcl_Obj    *CONST objv[]) {
    sasl_data_t   *sd = (sasl_data_t *) data;

    if (objc != 3) {
        Tcl_ResetResult (interp);
        Tcl_AppendResult (interp, "usage: ", Tcl_GetString (objv[0]),
                          " -operation errdetail", NULL);
        return TCL_ERROR;
    }

    Tcl_SetObjResult (interp,
                      Tcl_NewStringObj (sasl_errdetail (sd -> sd_conn), -1));
    return TCL_OK;
}


static int
sasl_aux_info (ClientData  data,
                  Tcl_Interp *interp,
                  int         objc,
                  Tcl_Obj    *CONST objv[]) {
    oc_pair_t     *pp;
    sasl_data_t   *sd = (sasl_data_t *) data;
    Tcl_Obj       *resultPtr;

    if (objc != 3) {
        Tcl_ResetResult (interp);
        Tcl_AppendResult (interp, "usage: ", Tcl_GetString (objv[0]),
                          " -operation info", NULL);
        return TCL_ERROR;
    }

    resultPtr = Tcl_NewObj ();
    for (pp = sd -> sd_dispatch; pp -> p_key; pp++)
        Tcl_ListObjAppendElement (interp, resultPtr,
                                  Tcl_NewStringObj (pp -> p_key, -1));

    Tcl_SetObjResult (interp, resultPtr);
    return TCL_OK;
}


/* server routines */

#define SERVERINIT_CALLBACKS    0
#define SERVERINIT_APPNAME      1

#define SERVERINIT_MAXARGS      2
#define SERVERINIT_OPTIONAL     SERVERINIT_APPNAME

static kv_pair_t serverinit_args[] = {
  { "-callbacks",               SERVERINIT_CALLBACKS            },
  { "-appname",                 SERVERINIT_APPNAME              },

  { NULL,                       0                               }
};

/* ARGSUSED */

static int
server_init (ClientData  unused,
             Tcl_Interp *interp,
             int          objc,
             Tcl_Obj     *CONST objv[]) {
    int              result;
    char            *appname;
    sasl_callback_t *cb;
    Tcl_Obj         *args[SERVERINIT_MAXARGS];

    if (objc == 1)
        return t2c_usage (interp, objv[0], serverinit_args,
                          SERVERINIT_OPTIONAL, NULL, NULL);

    memset (args, 0, sizeof *args * SERVERINIT_MAXARGS);
    if ((result = crack_args (interp, objc, objv, serverinit_args,
                              SERVERINIT_OPTIONAL, args) != TCL_OK))
        return result;

    if (args[SERVERINIT_APPNAME] != NULL)
        appname = Tcl_GetString (args[SERVERINIT_APPNAME]);
    else if (!(appname = Tcl_GetVar (interp, "argv0",
                                     TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG)))
        return TCL_ERROR;

    if (!(cb = t2c_sasl_callback (interp, args[SERVERINIT_CALLBACKS])))
        return TCL_ERROR;

    if ((result = sasl_server_init (cb, appname)) != SASL_OK) {
        Tcl_HashEntry *entryPtr;

        if ((entryPtr = Tcl_FindHashEntry (&allocTable, (char *) cb)) != NULL)
            Tcl_DeleteHashEntry (entryPtr);
        free ((char *) cb);
    }
    
    return c2t_result (interp, "sasl_server_init", result);
}


#define SAUXLIST_OPERATION      0
#define SAUXLIST_USER           1

#define SAUXLIST_MAXARGS        2
#define SAUXLIST_OPTIONAL       SAUXLIST_USER

static kv_pair_t sauxlist_args[] = {
  { "-operation",               SAUXLIST_OPERATION              },
  { "-user",                    SAUXLIST_USER                   },

  { NULL,                       0                               }
};

static int
server_aux_list (ClientData  data,
                  Tcl_Interp *interp,
                  int          objc,
                  Tcl_Obj     *CONST objv[]) {
    int              result;
    CONST char      *mptr;
    sasl_data_t     *sd = (sasl_data_t *) data;
    Tcl_Obj         *args[SAUXLIST_MAXARGS];

    memset (args, 0, sizeof *args * SAUXLIST_MAXARGS);
    if ((result = crack_args (interp, objc, objv, sauxlist_args,
                              SAUXLIST_OPTIONAL, args)) != TCL_OK)
        return result;

    if ((result = sasl_listmech (sd -> sd_conn,
                                 Tcl_Obj2String (args[SAUXLIST_USER]),
                                 NULL, " ", NULL, &mptr, NULL, NULL))
            != SASL_OK)
        return c2t_result (interp, "sasl_listmech", result);

    Tcl_SetObjResult (interp, Tcl_NewStringObj (mptr, -1));
    return TCL_OK;
}


#define SAUXSTART_OPERATION     0
#define SAUXSTART_MECHANISM     1
#define SAUXSTART_INPUT         2

#define SAUXSTART_MAXARGS       3
#define SAUXSTART_OPTIONAL      SAUXSTART_INPUT

static kv_pair_t sauxstart_args[] = {
  { "-operation",               SAUXSTART_OPERATION             },
  { "-mechanism",               SAUXSTART_MECHANISM             },
  { "-input",                   SAUXSTART_INPUT                 },

  { NULL,                       0                               }
};

static int
server_aux_start (ClientData  data,
                  Tcl_Interp *interp,
                  int          objc,
                  Tcl_Obj     *CONST objv[]) {
    int              ilen,
                     olen,
                     result;
    CONST char      *iptr,
                    *optr;
    sasl_data_t     *sd = (sasl_data_t *) data;
    Tcl_Obj         *args[SAUXSTART_MAXARGS];

    if (objc == 3)
        return t2c_usage (interp, objv[0], sauxstart_args, SAUXSTART_OPTIONAL,
                          "start", NULL);

    memset (args, 0, sizeof *args * SAUXSTART_MAXARGS);
    if ((result = crack_args (interp, objc, objv, sauxstart_args,
                              SAUXSTART_OPTIONAL, args)) != TCL_OK)
        return result;
    if (args[SAUXSTART_INPUT] != NULL)
        iptr = Tcl_GetByteArrayFromObj (args[SAUXSTART_INPUT], &ilen);
    else
        iptr = NULL, ilen = 0;

    optr = NULL, olen = 0;
    switch (result =
                sasl_server_start (sd -> sd_conn,
                                   Tcl_GetString (args[SAUXSTART_MECHANISM]),
                                    iptr, ilen, &optr, &olen)) {
        case SASL_OK:
        case SASL_CONTINUE:
            Tcl_SetObjResult (interp,
                              Tcl_NewByteArrayObj ((unsigned char *) optr,
                                                   olen));
            return ((result == SASL_OK) ? TCL_OK : TCL_CONTINUE);

        default:
            return c2t_result (interp, "sasl_server_start", result);
    }
}


#define SAUXSTEP_OPERATION      0
#define SAUXSTEP_INPUT          1

#define SAUXSTEP_MAXARGS        2
#define SAUXSTEP_OPTIONAL       SAUXSTEP_MAXARGS

static kv_pair_t sauxstep_args[] = {
  { "-operation",               SAUXSTEP_OPERATION              },
  { "-input",                   SAUXSTEP_INPUT                  },

  { NULL,                       0                               }
};

static int
server_aux_step (ClientData  data,
                 Tcl_Interp *interp,
                 int          objc,
                 Tcl_Obj     *CONST objv[]) {
    int              ilen,
                     olen,
                     result;
    CONST char      *iptr,
                    *optr;
    sasl_data_t     *sd = (sasl_data_t *) data;
    Tcl_Obj         *args[SAUXSTEP_MAXARGS];

    if (objc == 3)
        return t2c_usage (interp, objv[0], sauxstep_args, SAUXSTEP_OPTIONAL,
                          "step", NULL);

    memset (args, 0, sizeof *args * SAUXSTEP_MAXARGS);
    if ((result = crack_args (interp, objc, objv, sauxstep_args,
                              SAUXSTEP_OPTIONAL, args)) != TCL_OK)
        return result;
    iptr = Tcl_GetByteArrayFromObj (args[SAUXSTEP_INPUT], &ilen);

    switch (result = sasl_server_step (sd -> sd_conn, iptr, ilen, &optr,
                                       &olen)) {
        case SASL_OK:
        case SASL_CONTINUE:
            Tcl_SetObjResult (interp,
                              Tcl_NewByteArrayObj ((unsigned char *) optr,
                                                   olen));
            return ((result == SASL_OK) ? TCL_OK : TCL_CONTINUE);

        default:
            return c2t_result (interp, "sasl_server_step", result);
    }
};


#define SAUXCPASS_OPERATION     0
#define SAUXCPASS_USER          1
#define SAUXCPASS_PASS          2

#define SAUXCPASS_MAXARGS       3
#define SAUXCPASS_OPTIONAL      SAUXCPASS_MAXARGS

static kv_pair_t sauxcpass_args[] = {
  { "-operation",               SAUXCPASS_OPERATION             },
  { "-user",                    SAUXCPASS_USER                  },
  { "-pass",                    SAUXCPASS_PASS                  },

  { NULL,                       0                               }
};

static int
server_aux_cpass (ClientData  data,
                 Tcl_Interp *interp,
                 int          objc,
                 Tcl_Obj     *CONST objv[]) {
    int              ulen,
                     plen,
                     result;
    CONST char      *uptr,
                    *pptr;
    sasl_data_t     *sd = (sasl_data_t *) data;
    Tcl_Obj         *args[SAUXCPASS_MAXARGS];

    if (objc == 3)
        return t2c_usage (interp, objv[0], sauxcpass_args, SAUXCPASS_OPTIONAL,
                          "checkpass", NULL);

    memset (args, 0, sizeof *args * SAUXCPASS_MAXARGS);
    if ((result = crack_args (interp, objc, objv, sauxcpass_args,
                              SAUXCPASS_OPTIONAL, args)) != TCL_OK)
        return result;
    uptr = Tcl_GetByteArrayFromObj (args[SAUXCPASS_USER], &ulen);
    pptr = Tcl_GetByteArrayFromObj (args[SAUXCPASS_PASS], &plen);

    return c2t_result (interp, "sasl_checkpass",
                       sasl_checkpass (sd -> sd_conn, uptr, ulen, pptr, plen));
};


#define SAUXCUSER_OPERATION     0
#define SAUXCUSER_SERVER        1
#define SAUXCUSER_USER          2
#define SAUXCUSER_REALM         3

#define SAUXCUSER_MAXARGS       4
#define SAUXCUSER_OPTIONAL      SAUXCUSER_REALM

static kv_pair_t sauxcuser_args[] = {
  { "-operation",               SAUXCUSER_OPERATION             },
  { "-server",                  SAUXCUSER_SERVER                },
  { "-user",                    SAUXCUSER_USER                  },
  { "-realm",                   SAUXCUSER_REALM                 },

  { NULL,                       0                               }
};

static int
server_aux_cuser (ClientData  data,
                 Tcl_Interp *interp,
                 int          objc,
                 Tcl_Obj     *CONST objv[]) {
    int              result;
    sasl_data_t     *sd = (sasl_data_t *) data;
    Tcl_Obj         *args[SAUXCUSER_MAXARGS];

    if (objc == 3)
        return t2c_usage (interp, objv[0], sauxcuser_args, SAUXCUSER_OPTIONAL,
                          "userexists", NULL);

    memset (args, 0, sizeof *args * SAUXCUSER_MAXARGS);
    if ((result = crack_args (interp, objc, objv, sauxcuser_args,
                              SAUXCUSER_OPTIONAL, args)) != TCL_OK)
        return result;


    return
          c2t_result (interp, "sasl_user_exists",
                      sasl_user_exists (sd -> sd_conn,
                                        Tcl_GetString (args[SAUXCUSER_SERVER]),
                                        Tcl_Obj2String (args[SAUXCUSER_REALM]),
                                        Tcl_GetString (args[SAUXCUSER_USER])));
}


#define SAUXSPASS_OPERATION     0
#define SAUXSPASS_USER          1
#define SAUXSPASS_NEWPASS       2
#define SAUXSPASS_OLDPASS       3
#define SETPASS_FLAGS           4

#define SAUXSPASS_MAXARGS       5
#define SAUXSPASS_OPTIONAL      SAUXSPASS_NEWPASS

static kv_pair_t sauxspass_args[] = {
  { "-operation",               SAUXSPASS_OPERATION             },
  { "-user",                    SAUXSPASS_USER                  },
  { "-newpass",                 SAUXSPASS_NEWPASS               },
  { "-oldpass",                 SAUXSPASS_OLDPASS               },
  { "-flags",                   SETPASS_FLAGS                   },

  { NULL,                       0                               }
};

static int
server_aux_spass (ClientData  data,
                 Tcl_Interp *interp,
                 int          objc,
                 Tcl_Obj     *CONST objv[]) {
    int              nlen,
                     olen,
                     result;
    unsigned         flags;
    CONST char      *nptr,
                    *optr;
    sasl_data_t     *sd = (sasl_data_t *) data;
    Tcl_Obj         *args[SAUXSPASS_MAXARGS];

    if (objc == 3)
        return t2c_usage (interp, objv[0], sauxspass_args, SAUXSPASS_OPTIONAL,
                          "setpass", setpass_flags);

    memset (args, 0, sizeof *args * SAUXSPASS_MAXARGS);
    if ((result = crack_args (interp, objc, objv, sauxspass_args,
                              SAUXSPASS_OPTIONAL, args)) != TCL_OK)
        return result;
    if (args[SAUXSPASS_NEWPASS] != NULL)
        nptr = Tcl_GetByteArrayFromObj (args[SAUXSPASS_NEWPASS], &nlen);
    else
        nptr = NULL, nlen = 0;
    if (args[SAUXSPASS_OLDPASS] != NULL)
        optr = Tcl_GetByteArrayFromObj (args[SAUXSPASS_NEWPASS], &olen);
    else
        optr = NULL, olen = 0;
    if ((result = t2c_flags (interp, args[SETPASS_FLAGS], setpass_flags,
                             &flags)) != TCL_OK)
        return result;

    return c2t_result (interp, "sasl_setpass",
                       sasl_setpass (sd -> sd_conn,
                                     Tcl_GetString (args[SAUXSPASS_USER]),
                                     nptr, nlen, optr, olen, flags));
};


#define SPROPREQ_OPERATION      0
#define SPROPREQ_PROPERTIES     1

#define SPROPREQ_MAXARGS        2
#define SPROPREQ_OPTIONAL       SPROPREQ_PROPERTIES

static kv_pair_t spropreq_args[] = {
  { "-operation",               SPROPREQ_OPERATION              },
  { "-properties",              SPROPREQ_PROPERTIES             },

  { NULL,                       0                               }
};

static int
server_aux_propreq (ClientData  data,
                    Tcl_Interp *interp,
                    int         objc,
                    Tcl_Obj    *CONST objv[]) {
  int                i,
                     nelem,
                     result;
    char           **ap,
                    *mp;
    sasl_data_t     *sd = (sasl_data_t *) data;
    Tcl_Obj         *objPtr,
                    *args[SPROPREQ_MAXARGS];

    if (objc == 3)
        return t2c_usage (interp, objv[0], spropreq_args, SPROPREQ_OPTIONAL,
                          "userexists", NULL);

    memset (args, 0, sizeof *args * SPROPREQ_MAXARGS);
    if ((result = crack_args (interp, objc, objv, spropreq_args,
                              SPROPREQ_OPTIONAL, args)) != TCL_OK)
        return result;

    if (!(objPtr = args[SPROPREQ_PROPERTIES]))
        return c2t_result (interp, "sasl_auxprop_request",
                           sasl_auxprop_request (sd -> sd_conn, NULL));

    if ((result = Tcl_ListObjLength (interp, objPtr, &nelem)) != TCL_OK)
        return result;

    mp = Tcl_Alloc ((nelem + 1) * sizeof *ap);

    ap = (char **) mp;
    for (i = 0; i < nelem; i++) {
        Tcl_Obj *elemPtr;

        if ((result = Tcl_ListObjIndex (interp, objPtr, i, &elemPtr))
              != TCL_OK)
            goto out;
        *ap++ = Tcl_GetString (elemPtr);
    }
    *ap = NULL;

    result = c2t_result (interp, "sasl_auxprop_request",
                           sasl_auxprop_request (sd -> sd_conn,
                                                 (const char **) mp));

out: ;
    Tcl_Free (mp);

    return result;
}



#define SPROPGET_OPERATION      0

#define SPROPGET_MAXARGS        1
#define SPROPGET_OPTIONAL       SPROPGET_MAXARGS

static kv_pair_t spropget_args[] = {
  { "-operation",               SPROPGET_OPERATION              },

  { NULL,                       0                               }
};

static int
server_aux_propget (ClientData  data,
                    Tcl_Interp *interp,
                    int         objc,
                    Tcl_Obj    *CONST objv[]) {
    int              result;
    sasl_data_t     *sd = (sasl_data_t *) data;
    propctx_t       *pc;
    Tcl_Obj         *args[SPROPGET_MAXARGS];

    memset (args, 0, sizeof *args * SPROPGET_MAXARGS);
    if ((result = crack_args (interp, objc, objv, spropget_args,
                              SPROPGET_OPTIONAL, args)) != TCL_OK)
        return result;

    if ((pc = sasl_auxprop_getctx (sd -> sd_conn)) != NULL)
        Tcl_SetObjResult (interp, c2t_propctx (pc));
    else
        Tcl_ResetResult (interp);
    return TCL_OK;
}


#define SERVERNEW_SERVICE       0
#define SERVERNEW_SERVERFQDN    1
#define SERVERNEW_IPLOCALPORT   2
#define SERVERNEW_IPREMOTEPORT  3
#define SERVERNEW_CALLBACKS     4
#define SERVERNEW_FLAGS         5
#define SERVERNEW_REALM         6

#define SERVERNEW_MAXARGS       7
#define SERVERNEW_OPTIONAL      SERVERNEW_SERVERFQDN

static kv_pair_t servernew_args[] = {
  { "-service",                 SERVERNEW_SERVICE               },
  { "-serverFQDN",              SERVERNEW_SERVERFQDN            },
  { "-realm",                   SERVERNEW_REALM                 },
  { "-iplocalport",             SERVERNEW_IPLOCALPORT           },
  { "-ipremoteport",            SERVERNEW_IPREMOTEPORT          },
  { "-callbacks",               SERVERNEW_CALLBACKS             },
  { "-flags",                   SERVERNEW_FLAGS                 },

  { NULL,                       0                               }
};

static kv_pair_t servernew_flags[] = {
  { "success_data",             SASL_SUCCESS_DATA               },

  { NULL,                       0                               }
};

static oc_pair_t server_aux_pairs[] = {
  { "auxprop_request",          server_aux_propreq              },
  { "auxprop_getctx",           server_aux_propget              },
  { "checkpass",                server_aux_cpass                },
  { "decode",                   sasl_aux_decode                 },
  { "encode",                   sasl_aux_encode                 },
  { "errdetail",                sasl_aux_errdetail              },
  { "getprop",                  sasl_aux_getprop                },
  { "info",                     sasl_aux_info                   },
  { "list",                     server_aux_list                 },
  { "setpass",                  server_aux_spass                },
  { "setprop",                  sasl_aux_setprop                },
  { "start",                    server_aux_start                },
  { "step",                     server_aux_step                 },
  { "userexists",               server_aux_cuser                },

  { NULL,                       0                               }
};

/* ARGSUSED */

static int
server_new (ClientData  unused,
            Tcl_Interp *interp,
            int          objc,
            Tcl_Obj     *CONST objv[]) {
    int            result;
    unsigned       flags;
    char           buffer[BUFSIZ],
                  *cp;
    sasl_data_t   *sd;
    Tcl_HashEntry *entryPtr;
    Tcl_Obj       *args[SERVERNEW_MAXARGS];
    static int     nproc = 0;

    if (objc == 1)
        return t2c_usage (interp, objv[0], servernew_args, SERVERNEW_OPTIONAL,
                          NULL, servernew_flags);

    memset (args, 0, sizeof *args * SERVERNEW_MAXARGS);
    if ((result = crack_args (interp, objc, objv, servernew_args,
                              SERVERNEW_OPTIONAL, args) != TCL_OK))
        return result;
    if ((result = t2c_flags (interp, args[SERVERNEW_FLAGS], servernew_flags,
                             &flags)) != TCL_OK)
        return result;

    cp = Tcl_Alloc (sizeof *sd);
    memset (cp, 0, sizeof *sd);
    sd = (sasl_data_t *) cp;
    sd -> sd_dispatch = server_aux_pairs;

    if ((args[SERVERNEW_CALLBACKS] != NULL)
            && (!(sd -> sd_cb =
                      t2c_sasl_callback (interp, args[SERVERNEW_CALLBACKS]))))
        goto out;

    if ((result =
             sasl_server_new (Tcl_GetString (args[SERVERNEW_SERVICE]),
                              Tcl_Obj2String (args[SERVERNEW_SERVERFQDN]),
                              Tcl_Obj2String (args[SERVERNEW_REALM]),
                              Tcl_Obj2String (args[SERVERNEW_IPLOCALPORT]),
                              Tcl_Obj2String (args[SERVERNEW_IPREMOTEPORT]),
                              sd -> sd_cb, flags,
                              &sd -> sd_conn) != SASL_OK)) {
        c2t_result (interp, "sasl_server_new", result);
        goto out;
    }
    
    sprintf (buffer, "::sasl::server_new_%d", nproc++);
    sd -> sd_token = Tcl_CreateObjCommand (sd -> sd_interp = interp, buffer,
                                           sasl_aux_proc, (ClientData) sd,
                                           sasl_aux_free);

    entryPtr = Tcl_CreateHashEntry (&connTable, (char *) sd -> sd_conn,
                                    &result);
    Tcl_SetHashValue (entryPtr, (ClientData) sd);

    Tcl_SetResult (interp, buffer, TCL_VOLATILE);
    return TCL_OK;

out: ;
    sasl_aux_free ((ClientData) sd);

    return TCL_ERROR;
}


/* client routines */

#define CLIENTINIT_CALLBACKS    0

#define CLIENTINIT_MAXARGS      1
#define CLIENTINIT_OPTIONAL     CLIENTINIT_MAXARGS

static kv_pair_t clientinit_args[] = {
  { "-callbacks",               CLIENTINIT_CALLBACKS            },

  { NULL,                       0                               }
};

/* ARGSUSED */

static int
client_init (ClientData  unused,
             Tcl_Interp *interp,
             int          objc,
             Tcl_Obj     *CONST objv[]) {
    int              result;
    sasl_callback_t *cb;
    Tcl_Obj         *args[CLIENTINIT_MAXARGS];

    if (objc == 1)
        return t2c_usage (interp, objv[0], clientinit_args,
                          CLIENTINIT_OPTIONAL, NULL, NULL);

    memset (args, 0, sizeof *args * CLIENTINIT_MAXARGS);
    if ((result = crack_args (interp, objc, objv, clientinit_args,
                              CLIENTINIT_OPTIONAL, args) != TCL_OK))
        return result;

    if (!(cb = t2c_sasl_callback (interp, args[CLIENTINIT_CALLBACKS])))
        return TCL_ERROR;

    if ((result = sasl_client_init (cb)) != SASL_OK) {
        Tcl_HashEntry *entryPtr;

        if ((entryPtr = Tcl_FindHashEntry (&allocTable, (char *) cb)) != NULL)
            Tcl_DeleteHashEntry (entryPtr);
        free ((char *) cb);
    }
    
    return c2t_result (interp, "sasl_client_init", result);

}


static int
client_aux_interact (Tcl_Interp      *interp,
                     Tcl_Obj         *cmdPtr,
                     sasl_interact_t *interact) {
    int              result;
    kv_pair_t       *p;
    sasl_interact_t *ip;
    Tcl_Obj         *argPtr,
                    *evalPtr;

    if (!interact)
        return TCL_OK;

    for (ip = interact; ip -> id != SASL_CB_LIST_END; ip++) {
        ip -> result = NULL, ip -> len = 0;

        if (!cmdPtr)
            return TCL_OK;

        for (p = cb_pairs; p -> p_key; p++)
            if (p -> p_value == ip -> id)
                break;
        if (!p -> p_key)
            continue;

        argPtr = Tcl_NewObj ();

        Tcl_ListObjAppendElement (interp, argPtr,
                                  Tcl_NewStringObj ("id", -1));
        Tcl_ListObjAppendElement (interp, argPtr,
                                  Tcl_NewStringObj (p -> p_key, -1));

        if (ip -> challenge) {
            Tcl_ListObjAppendElement (interp, argPtr,
                                      Tcl_NewStringObj ("challenge", -1));
            Tcl_ListObjAppendElement (interp, argPtr,
                                      Tcl_NewStringObj (ip -> challenge, -1));
        }

        if (ip -> prompt) {
            Tcl_ListObjAppendElement (interp, argPtr,
                                      Tcl_NewStringObj ("prompt", -1));
            Tcl_ListObjAppendElement (interp, argPtr,
                                      Tcl_NewStringObj (ip -> prompt, -1));
        }

        if (ip -> defresult) {
            Tcl_ListObjAppendElement (interp, argPtr,
                                      Tcl_NewStringObj ("default", -1));
            Tcl_ListObjAppendElement (interp, argPtr,
                                      Tcl_NewStringObj (ip -> defresult, -1));
        }

        evalPtr = Tcl_DuplicateObj (cmdPtr);
        if ((result = Tcl_ListObjAppendElement (interp, evalPtr, argPtr))
                != TCL_OK)
            return result;
        if ((result = Tcl_EvalObjEx (interp, evalPtr, TCL_EVAL_DIRECT)) 
                == TCL_OK)
            ip -> result = allocate (interp, &ip -> len);
    }

    return TCL_OK;
}


static void
client_aux_interact_free () {
    Tcl_HashEntry  *entryPtr;
    Tcl_HashSearch  hs,
                   *searchPtr = &hs;

    for (entryPtr = Tcl_FirstHashEntry (&allocTable, searchPtr);
             entryPtr != NULL;
             entryPtr = Tcl_NextHashEntry (searchPtr)) {
        int     refcnt = (int) Tcl_GetHashValue (entryPtr);

        if (refcnt > 0)
            continue;

        Tcl_Free (Tcl_GetHashKey (&allocTable, entryPtr));
        Tcl_DeleteHashEntry (entryPtr);
    }
}


#define CAUXSTART_OPERATION     0
#define CAUXSTART_MECHANISMS    1
#define CAUXSTART_INTERACT      2

#define CAUXSTART_MAXARGS       3
#define CAUXSTART_OPTIONAL      CAUXSTART_INTERACT

static kv_pair_t cauxstart_args[] = {
  { "-operation",               CAUXSTART_OPERATION             },
  { "-mechanisms",              CAUXSTART_MECHANISMS            },
  { "-interact",                CAUXSTART_INTERACT              },

  { NULL,                       0                               }
};

static int
client_aux_start (ClientData  data,
                  Tcl_Interp *interp,
                  int          objc,
                  Tcl_Obj     *CONST objv[]) {
    int              olen,
                     result;
    CONST char      *mechused,
                    *optr;
    sasl_data_t     *sd = (sasl_data_t *) data;
    sasl_interact_t *interact,
                   **ip;
    Tcl_Obj         *args[CAUXSTART_MAXARGS],
                    *resultPtr;

    if (objc == 3)
        return t2c_usage (interp, objv[0], cauxstart_args, CAUXSTART_OPTIONAL,
                          "start", NULL);

    memset (args, 0, sizeof *args * CAUXSTART_MAXARGS);
    if ((result = crack_args (interp, objc, objv, cauxstart_args,
                              CAUXSTART_OPTIONAL, args)) != TCL_OK)
        return result;

    interact = NULL;
    if (args[CAUXSTART_INTERACT] != NULL)
        ip = &interact;
    else
        ip = NULL;
    while ((result =
                 sasl_client_start (sd -> sd_conn,
                                    Tcl_GetString (args[CAUXSTART_MECHANISMS]),
                                    ip, &optr, &olen, &mechused))
                == SASL_INTERACT)
        if ((result = client_aux_interact (interp, args[CAUXSTART_INTERACT],
                                           interact))
                != TCL_OK)
            return result;

    switch (result) {
        case SASL_OK:
            client_aux_interact_free ();
        /* and fall... */

        case SASL_CONTINUE:
            resultPtr = Tcl_NewObj ();

            Tcl_ListObjAppendElement (interp, resultPtr,
                                      Tcl_NewStringObj ("mechanism", -1));
            Tcl_ListObjAppendElement (interp, resultPtr,
                                      Tcl_NewStringObj (mechused, -1));

            Tcl_ListObjAppendElement (interp, resultPtr,
                                      Tcl_NewStringObj ("output", -1));
            Tcl_ListObjAppendElement (interp, resultPtr,
                                      Tcl_NewByteArrayObj ((unsigned char *) optr,
                                                           olen));

            Tcl_SetObjResult (interp, resultPtr);
            return ((result == SASL_OK) ? TCL_OK : TCL_CONTINUE);

        default:
            client_aux_interact_free ();
            return c2t_result (interp, "sasl_client_start", result);
    }
}


#define CAUXSTEP_OPERATION      0
#define CAUXSTEP_INPUT          1
#define CAUXSTEP_INTERACT       2

#define CAUXSTEP_MAXARGS        3
#define CAUXSTEP_OPTIONAL       CAUXSTEP_INTERACT

static kv_pair_t cauxstep_args[] = {
  { "-operation",               CAUXSTEP_OPERATION              },
  { "-input",                   CAUXSTEP_INPUT                  },
  { "-interact",                CAUXSTEP_INTERACT               },

  { NULL,                       0                               }
};

static int
client_aux_step (ClientData  data,
                 Tcl_Interp *interp,
                 int          objc,
                 Tcl_Obj     *CONST objv[]) {
    int              ilen,
                     olen,
                     result;
    CONST char      *iptr,
                    *optr;
    sasl_data_t     *sd = (sasl_data_t *) data;
    sasl_interact_t *interact,
                   **ip;
    Tcl_Obj         *args[CAUXSTEP_MAXARGS];

    if (objc == 3)
        return t2c_usage (interp, objv[0], cauxstep_args, CAUXSTEP_OPTIONAL,
                          "step", NULL);

    memset (args, 0, sizeof *args * CAUXSTEP_MAXARGS);
    if ((result = crack_args (interp, objc, objv, cauxstep_args,
                              CAUXSTEP_OPTIONAL, args)) != TCL_OK)
        return result;
    iptr = Tcl_GetByteArrayFromObj (args[CAUXSTEP_INPUT], &ilen);

    interact = NULL;
    if (args[CAUXSTEP_INTERACT] != NULL)
        ip = &interact;
    else
        ip = NULL;
    while ((result = sasl_client_step (sd -> sd_conn, iptr, ilen, ip, &optr,
                                       &olen)) == SASL_INTERACT)
        if ((result = client_aux_interact (interp, args[CAUXSTEP_INTERACT],
                                           interact))
                != TCL_OK)
            return result;

    switch (result) {
        case SASL_OK:
            client_aux_interact_free ();
        /* and fall... */

        case SASL_CONTINUE:
            Tcl_SetObjResult (interp,
                              Tcl_NewByteArrayObj ((unsigned char *) optr,
                                                   olen));
            return ((result == SASL_OK) ? TCL_OK : TCL_CONTINUE);

        default:
            client_aux_interact_free ();
            return c2t_result (interp, "sasl_client_step", result);
    }
};


#define CLIENTNEW_SERVICE       0
#define CLIENTNEW_SERVERFQDN    1
#define CLIENTNEW_IPLOCALPORT   2
#define CLIENTNEW_IPREMOTEPORT  3
#define CLIENTNEW_CALLBACKS     4
#define CLIENTNEW_FLAGS         5

#define CLIENTNEW_MAXARGS       6
#define CLIENTNEW_OPTIONAL      CLIENTNEW_IPLOCALPORT

static kv_pair_t clientnew_args[] = {
  { "-service",                 CLIENTNEW_SERVICE               },
  { "-serverFQDN",              CLIENTNEW_SERVERFQDN            },
  { "-iplocalport",             CLIENTNEW_IPLOCALPORT           },
  { "-ipremoteport",            CLIENTNEW_IPREMOTEPORT          },
  { "-callbacks",               CLIENTNEW_CALLBACKS             },
  { "-flags",                   CLIENTNEW_FLAGS                 },

  { NULL,                       0                               }
};

#define clientnew_flags servernew_flags

static oc_pair_t client_aux_pairs[] = {
  { "decode",                   sasl_aux_decode                 },
  { "encode",                   sasl_aux_encode                 },
  { "errdetail",                sasl_aux_errdetail              },
  { "getprop",                  sasl_aux_getprop                },
  { "info",                     sasl_aux_info                   },
  { "setprop",                  sasl_aux_setprop                },
  { "start",                    client_aux_start                },
  { "step",                     client_aux_step                 },

  { NULL,                       0                               }
};


/* ARGSUSED */

static int
client_new (ClientData  unused,
            Tcl_Interp *interp,
            int          objc,
            Tcl_Obj     *CONST objv[]) {
    int            result;
    unsigned       flags;
    char           buffer[BUFSIZ],
                  *cp;
    sasl_data_t   *sd;
    Tcl_HashEntry *entryPtr;
    Tcl_Obj       *args[CLIENTNEW_MAXARGS];
    static int     nproc = 0;

    if (objc == 1)
        return t2c_usage (interp, objv[0], clientnew_args, CLIENTNEW_OPTIONAL,
                          NULL, clientnew_flags);

    memset (args, 0, sizeof *args * CLIENTNEW_MAXARGS);
    if ((result = crack_args (interp, objc, objv, clientnew_args,
                              CLIENTNEW_OPTIONAL, args) != TCL_OK))
        return result;
    if ((result = t2c_flags (interp, args[CLIENTNEW_FLAGS], clientnew_flags,
                             &flags)) != TCL_OK)
        return result;

    cp = Tcl_Alloc (sizeof *sd);
    memset (cp, 0, sizeof *sd);
    sd = (sasl_data_t *) cp;
    sd -> sd_dispatch = client_aux_pairs;

    if ((args[CLIENTNEW_CALLBACKS] != NULL)
            && (!(sd -> sd_cb =
                      t2c_sasl_callback (interp, args[CLIENTNEW_CALLBACKS]))))
        goto out;

    if ((result =
             sasl_client_new (Tcl_GetString (args[CLIENTNEW_SERVICE]),
                              Tcl_GetString (args[CLIENTNEW_SERVERFQDN]),
                              Tcl_Obj2String (args[CLIENTNEW_IPLOCALPORT]),
                              Tcl_Obj2String (args[CLIENTNEW_IPREMOTEPORT]),
                              sd -> sd_cb, flags,
                              &sd -> sd_conn) != SASL_OK)) {
        c2t_result (interp, "sasl_client_new", result);
        goto out;
    }
    
    sprintf (buffer, "::sasl::client_new_%d", nproc++);
    sd -> sd_token = Tcl_CreateObjCommand (sd -> sd_interp = interp, buffer,
                                           sasl_aux_proc, (ClientData) sd,
                                           sasl_aux_free);

    entryPtr = Tcl_CreateHashEntry (&connTable, (char *) sd -> sd_conn,
                                    &result);
    Tcl_SetHashValue (entryPtr, (ClientData) sd);

    Tcl_SetResult (interp, buffer, TCL_VOLATILE);
    return TCL_OK;

out: ;
    sasl_aux_free ((ClientData) sd);

    return TCL_ERROR;
}


/* the {de,en}code64 commands */

static int
proc_codec64 (Tcl_Interp  *interp,
              char        *codep,
              int        (*codec) (),
              int          type,
              Tcl_Obj     *string) {
    int              result;
    unsigned         ilen,
                     olen,
                     omax;
    CONST char      *iptr;
    char            *optr;

    iptr = Tcl_GetByteArrayFromObj (string, &ilen);

    omax = (type == 'e') ? ((ilen + 2) * 4) / 3 : ilen;
    optr = Tcl_Alloc (++omax);

    if ((result = (*codec) (iptr, ilen, optr, omax, &olen)) != SASL_OK)
        result = c2t_result (interp, codep, result);
    else {
        result = TCL_OK;
        Tcl_SetObjResult (interp, Tcl_NewByteArrayObj (optr, olen));
    }

    Tcl_Free (optr);

    return TCL_OK;
}


/* ARGSUSED */

static int
proc_decode64 (ClientData  unused,
               Tcl_Interp *interp,
               int         objc,
               Tcl_Obj    *CONST objv[]) {
    if (objc != 2) {
        Tcl_ResetResult (interp);
        Tcl_AppendResult (interp, "usage: ", Tcl_GetString (objv[0]),
                          " string", NULL);
        return TCL_ERROR;
    }

    return proc_codec64 (interp, "sasl_decode64", sasl_decode64, 'd', objv[1]);
}


/* ARGSUSED */

static int
proc_encode64 (ClientData  unused,
               Tcl_Interp *interp,
               int         objc,
               Tcl_Obj    *CONST objv[]) {
    if (objc != 2) {
        Tcl_ResetResult (interp);
        Tcl_AppendResult (interp, "usage: ", Tcl_GetString (objv[0]),
                          " string", NULL);
        return TCL_ERROR;
    }

    return proc_codec64 (interp, "sasl_encode64", sasl_encode64, 'e', objv[1]);
}


/* the "errstring" command */

#define ERRSTRING_CODE          0
#define ERRSTRING_LANGUAGES     1

#define ERRSTRING_MAXARGS       2
#define ERRSTRING_OPTIONAL      ERRSTRING_LANGUAGES

static kv_pair_t errstring_args[] = {
  { "-code",                    ERRSTRING_CODE                  },
  { "-languages",               ERRSTRING_LANGUAGES             },

  { NULL,                       0                               }
};

/* ARGSUSED */

static int
proc_errstring (ClientData  unused,
                Tcl_Interp *interp,
                int         objc,
                Tcl_Obj    *CONST objv[]) {
    int      intValue,
             result;
    char    *langPtr;
    Tcl_Obj *resultPtr,
            *args[ERRSTRING_MAXARGS];

    if (objc == 1)
        return t2c_usage (interp, objv[0], errstring_args,
                          ERRSTRING_OPTIONAL, NULL, NULL);

    memset (args, 0, sizeof *args * ERRSTRING_MAXARGS);
    if ((result = crack_args (interp, objc, objv, errstring_args,
                              ERRSTRING_OPTIONAL, args) != TCL_OK))
        return result;

    if ((result = Tcl_GetIntFromObj (interp, args[ERRSTRING_CODE], &intValue))
            != TCL_OK)
        return result;


    resultPtr = Tcl_NewObj ();

    Tcl_ListObjAppendElement (interp, resultPtr,
                              Tcl_NewStringObj ("diagnostic", -1));
    Tcl_ListObjAppendElement (interp, resultPtr, Tcl_NewStringObj (
        sasl_errstring (intValue, Tcl_Obj2String (args[ERRSTRING_LANGUAGES]),
                        (const char **) &langPtr), -1));
    
    if (langPtr) {
        Tcl_ListObjAppendElement (interp, resultPtr,
                                  Tcl_NewStringObj ("language", -1));
        Tcl_ListObjAppendElement (interp, resultPtr,
                                  Tcl_NewStringObj (langPtr, -1));
    }
    
    Tcl_SetObjResult (interp, resultPtr);
    return TCL_OK;
}


/* the "mechanisms" command */

/* ARGSUSED */

static int
proc_mechanisms (ClientData  unused,
                 Tcl_Interp *interp,
                 int         objc,
                 Tcl_Obj    *CONST objv[]) {
    char    **mp;
    Tcl_Obj  *resultPtr;

    Tcl_ResetResult (interp);

    if (objc != 1) {
        Tcl_AppendResult (interp, "usage: ", Tcl_GetString (objv[0]), NULL);
        return TCL_ERROR;
    }

    if (!(mp = (char **) sasl_global_listmech ()))
        return TCL_OK;

    resultPtr = Tcl_NewObj ();

    for (; *mp; mp++)
        Tcl_ListObjAppendElement (interp, resultPtr,
                                  Tcl_NewStringObj (*mp, -1));
    
    Tcl_SetObjResult (interp, resultPtr);
    return TCL_OK;
}


/* the "info" command */


#define INFO_CALLBACKS  0
#define INFO_CANONFLAGS 1
#define INFO_CNEWFLAGS  2
#define INFO_GETPROPS   3
#define INFO_SECFLAGS   4
#define INFO_SECPROPS   5
#define INFO_SNEWFLAGS  6
#define INFO_SPASSFLAGS 7
#define INFO_SETPROPS   8
#define INFO_VRFYTYPES  9

static kv_pair_t info_pairs[] = {
  { "callbacks",                INFO_CALLBACKS                  },
  { "canon_flags",              INFO_CANONFLAGS                 },
  { "clientnew_flags",          INFO_CNEWFLAGS                  },
  { "getprops",                 INFO_GETPROPS                   },
  { "sec_flags",                INFO_SECFLAGS                   },
  { "sec_props",                INFO_SECPROPS                   },
  { "servernew_flags",          INFO_SNEWFLAGS                  },
  { "setpass_flags",            INFO_SPASSFLAGS                 },
  { "setprops",                 INFO_SETPROPS                   },
  { "verify_types",             INFO_VRFYTYPES                  },

  { NULL,                       0                               }
};


/* ARGSUSED */

static int
proc_info (ClientData  unused,
           Tcl_Interp *interp,
           int         objc,
           Tcl_Obj    *CONST objv[]) {
    int          result;
    kv_pair_t   *p;
    Tcl_Obj     *resultPtr;

    if (objc == 1)
        p = info_pairs;
    else if (objc != 2) {
        Tcl_ResetResult (interp);
        Tcl_AppendResult (interp, "usage: ", Tcl_GetString (objv[0]),
                          " option", NULL);
        return TCL_ERROR;
    } else {
        int     offset;
        if ((result = tcl_GetIndexFromObjStruct (interp, objv[1],
                                                 (char **) info_pairs,
                                                 sizeof *info_pairs,
                                                 "option", 0, &offset))
                != TCL_OK)
            return result;

        switch (offset) {
            case INFO_CALLBACKS:
                p = cb_pairs;
                break;
    
            case INFO_CANONFLAGS:
                p = canon_flags;
                break;

            case INFO_CNEWFLAGS:
                p = clientnew_flags;
                break;

            case INFO_GETPROPS:
                p = getprop_pairs;
                break;
    
            case INFO_SECFLAGS:
                p = secprops_flags;
                break;
    
            case INFO_SECPROPS:
                p = secprops_pairs;
                break;
    
            case INFO_SNEWFLAGS:
                p = servernew_flags;
                break;
        
            case INFO_SPASSFLAGS:
                p = setpass_flags;
                break;

            case INFO_SETPROPS:
                p = setprop_pairs;
                break;

            case INFO_VRFYTYPES:
                p = vf_pairs;
                break;

            default:
                Tcl_SetResult (interp, "internal error, missing known case",
                               TCL_STATIC);
                return TCL_ERROR;
            }
    }

    resultPtr = Tcl_NewObj ();
    for (; p -> p_key; p++)
        Tcl_ListObjAppendElement (interp, resultPtr,
                                  Tcl_NewStringObj (p -> p_key, -1));

    Tcl_SetObjResult (interp, resultPtr);
    return TCL_OK;
}


/* the "done" command */

/* ARGSUSED */

static int
proc_done (ClientData  unused,
           Tcl_Interp *interp,
           int         objc,
           Tcl_Obj    *CONST objv[]) {
    Tcl_HashEntry  *entryPtr;
    Tcl_HashSearch  hs,
                   *searchPtr = &hs;

    if (objc != 1) {
        Tcl_ResetResult (interp);
        Tcl_AppendResult (interp, "usage: ", Tcl_GetString (objv[0]), NULL);
        return TCL_ERROR;
    }

    for (entryPtr = Tcl_FirstHashEntry (&connTable, searchPtr);
             entryPtr != NULL;
             entryPtr = Tcl_NextHashEntry (searchPtr)) {
        sasl_data_t *sd = Tcl_GetHashValue (entryPtr);

        Tcl_DeleteCommandFromToken (sd -> sd_interp, sd -> sd_token);
    }

    for (entryPtr = Tcl_FirstHashEntry (&t2pTable, searchPtr);
             entryPtr != NULL;
             entryPtr = Tcl_NextHashEntry (searchPtr)) {
        Tcl_Obj *objPtr = (Tcl_Obj *) Tcl_GetHashKey (&t2pTable, entryPtr); 
        propctx_t *prop = (propctx_t *) Tcl_GetHashValue (entryPtr);

        Tcl_DecrRefCount (objPtr);
        Tcl_DeleteHashEntry (entryPtr);

        if ((entryPtr = Tcl_FindHashEntry (&p2tTable, (char *) prop)) != NULL)
            Tcl_DeleteHashEntry (entryPtr);

#if     0
        prop_dispose (&prop);
#endif
    }

    for (entryPtr = Tcl_FirstHashEntry (&allocTable, searchPtr);
             entryPtr != NULL;
             entryPtr = Tcl_NextHashEntry (searchPtr)) {
        Tcl_Free (Tcl_GetHashKey (&allocTable, entryPtr));
        Tcl_DeleteHashEntry (entryPtr);
    }

    sasl_done ();

    Tcl_ResetResult (interp);
    return TCL_OK;
}


/* module initialization */

static oc_pair_t init_pairs[] = {
  { "sasl::decode64",           proc_decode64                   },
  { "sasl::done",               proc_done                       },
  { "sasl::encode64",           proc_encode64                   },
  { "sasl::errstring",          proc_errstring                  },
  { "sasl::mechanisms",         proc_mechanisms                 },
  { "sasl::info",               proc_info                       },

  { "sasl::server_init",        server_init                     },
  { "sasl::server_new",         server_new                      },

  { "sasl::client_init",        client_init                     },
  { "sasl::client_new",         client_new                      },

  { NULL,                       0                               }
};


int
Tclsasl_Init (Tcl_Interp *interp) {
    oc_pair_t *pp;

    if (Tcl_InitStubs (interp, "8.0", 0) == NULL)
        return TCL_ERROR;

    if ((Tcl_PkgRequire (interp, "Tcl", TCL_VERSION, 0) == NULL)
            && (TCL_VERSION[0] == '7')
            && (Tcl_PkgRequire (interp, "Tcl", "8.0", 0) == NULL))
        return TCL_ERROR;

    if (Tcl_PkgProvide (interp, "sasl", VERSION) != TCL_OK)
        return TCL_ERROR;

    Tcl_InitHashTable (&allocTable, TCL_ONE_WORD_KEYS);
    Tcl_InitHashTable (&connTable, TCL_ONE_WORD_KEYS);
    Tcl_InitHashTable (&p2tTable, TCL_ONE_WORD_KEYS);
    Tcl_InitHashTable (&t2pTable, TCL_ONE_WORD_KEYS);

    for (pp = init_pairs; pp -> p_key; pp++)
        Tcl_CreateObjCommand (interp, pp -> p_key, pp -> p_proc,
                              (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

    return TCL_OK;
}


int
Sasl_Init (Tcl_Interp *interp) {
    return Tclsasl_Init (interp);
}

#endif  /* TCLSASL */
