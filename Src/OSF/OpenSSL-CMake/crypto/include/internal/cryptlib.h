/*
 * Copyright 1995-2016 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the OpenSSL license (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */

#ifndef HEADER_CRYPTLIB_H
#define HEADER_CRYPTLIB_H

#include <slib.h>

#define OPENSSL_NO_DYNAMIC_ENGINE // @sobolev

//#include <stdlib.h>
//#include <string.h>
//#include <assert.h>
//
// BIO_LCL {
//
#define USE_SOCKETS
#include "e_os.h"

/* BEGIN BIO_ADDRINFO/BIO_ADDR stuff. */

#ifndef OPENSSL_NO_SOCK
/*
 * Throughout this file and b_addr.c, the existence of the macro
 * AI_PASSIVE is used to detect the availability of struct addrinfo,
 * getnameinfo() and getaddrinfo().  If that macro doesn't exist,
 * we use our own implementation instead.
 */

/*
 * It's imperative that these macros get defined before openssl/bio.h gets
 * included.  Otherwise, the AI_PASSIVE hack will not work properly.
 * For clarity, we check for internal/cryptlib.h since it's a common header
 * that also includes bio.h.
 */
#ifdef HEADER_CRYPTLIB_H
	//#error internal/cryptlib.h included before bio_lcl.h
#endif
	#ifdef HEADER_BIO_H
		//#error openssl/bio.h included before bio_lcl.h
	#endif

/*
 * Undefine AF_UNIX on systems that define it but don't support it.
 */
#if defined(OPENSSL_SYS_WINDOWS) || defined(OPENSSL_SYS_VMS)
	#undef AF_UNIX
#endif

#ifdef AI_PASSIVE
	/*
	* There's a bug in VMS C header file netdb.h, where struct addrinfo
	* always is the P32 variant, but the functions that handle that structure,
	* such as getaddrinfo() and freeaddrinfo() adapt to the initial pointer
	* size.  The easiest workaround is to force struct addrinfo to be the
	* 64-bit variant when compiling in P64 mode.
	*/
	#if defined(OPENSSL_SYS_VMS) && __INITIAL_POINTER_SIZE == 64
		#define addrinfo __addrinfo64
	#endif
	#define bio_addrinfo_st addrinfo
	#define bai_family      ai_family
	#define bai_socktype    ai_socktype
	#define bai_protocol    ai_protocol
	#define bai_addrlen     ai_addrlen
	#define bai_addr        ai_addr
	#define bai_next        ai_next
#else
	struct bio_addrinfo_st {
		int bai_family;
		int bai_socktype;
		int bai_protocol;
		size_t bai_addrlen;
		struct sockaddr *bai_addr;
		struct bio_addrinfo_st *bai_next;
	};
#endif

union bio_addr_st {
    struct sockaddr sa;
#ifdef AF_INET6
    struct sockaddr_in6 s_in6;
#endif
    struct sockaddr_in s_in;
#ifdef AF_UNIX
    struct sockaddr_un s_un;
#endif
};
#endif

/* END BIO_ADDRINFO/BIO_ADDR stuff. */

//#include "internal/cryptlib.h"
#include <internal/bio.h>

typedef struct bio_f_buffer_ctx_struct {
    /*-
     * Buffers are setup like this:
     *
     * <---------------------- size ----------------------->
     * +---------------------------------------------------+
     * | consumed | remaining          | free space        |
     * +---------------------------------------------------+
     * <-- off --><------- len ------->
     */
    /*- BIO *bio; *//*
     * this is now in the BIO struct
     */
    int ibuf_size;              /* how big is the input buffer */
    int obuf_size;              /* how big is the output buffer */
    char *ibuf;                 /* the char array */
    int ibuf_len;               /* how many bytes are in it */
    int ibuf_off;               /* write/read offset */
    char *obuf;                 /* the char array */
    int obuf_len;               /* how many bytes are in it */
    int obuf_off;               /* write/read offset */
} BIO_F_BUFFER_CTX;

struct bio_st {
    const BIO_METHOD *method;
    /* bio, mode, argp, argi, argl, ret */
    long (*callback) (struct bio_st *, int, const char *, int, long, long);
    char *cb_arg;               /* first argument for the callback */
    int init;
    int shutdown;
    int flags;                  /* extra storage */
    int retry_reason;
    int num;
    void *ptr;
    struct bio_st *next_bio;    /* used by filter BIOs */
    struct bio_st *prev_bio;    /* used by filter BIOs */
    int references;
    uint64_t num_read;
    uint64_t num_write;
    CRYPTO_EX_DATA ex_data;
    CRYPTO_RWLOCK *lock;
};

#ifndef OPENSSL_NO_SOCK
	#ifdef OPENSSL_SYS_VMS
	typedef uint socklen_t;
#endif

extern CRYPTO_RWLOCK *bio_lookup_lock;

int BIO_ADDR_make(BIO_ADDR *ap, const struct sockaddr *sa);
const struct sockaddr *BIO_ADDR_sockaddr(const BIO_ADDR *ap);
struct sockaddr *BIO_ADDR_sockaddr_noconst(BIO_ADDR *ap);
socklen_t BIO_ADDR_sockaddr_size(const BIO_ADDR *ap);
socklen_t BIO_ADDRINFO_sockaddr_size(const BIO_ADDRINFO *bai);
const struct sockaddr *BIO_ADDRINFO_sockaddr(const BIO_ADDRINFO *bai);
#endif

extern CRYPTO_RWLOCK *bio_type_lock;

void bio_sock_cleanup_int(void);

#if BIO_FLAGS_UPLINK==0
/* Shortcut UPLINK calls on most platforms... */
# define UP_stdin        stdin
# define UP_stdout       stdout
# define UP_stderr       stderr
# define UP_fprintf      fprintf
# define UP_fgets(mp_str,mp_sz,mp_stream)       fgets((mp_str),(mp_sz),(FILE *)(mp_stream))
# define UP_fread(mp_ptr,mp_sz,mp_n,mp_stream)  fread((mp_ptr),(mp_sz),(mp_n),(FILE *)(mp_stream))
# define UP_fwrite(mp_ptr,mp_sz,mp_n,mp_stream) fwrite((mp_ptr),(mp_sz),(mp_n),(FILE *)(mp_stream))
# undef  UP_fsetmod
# define UP_feof         feof
# define UP_fclose(mp_stream) fclose((FILE *)(mp_stream))

# define UP_fopen        fopen
# define UP_fseek(mp_stream,mp_offs,mp_org)  fseek((FILE *)mp_stream,mp_offs,mp_org)
# define UP_ftell(mp_stream)  ftell((FILE *)(mp_stream))
# define UP_fflush(mp_stream) fflush((FILE *)(mp_stream))
# define UP_ferror       ferror
#ifdef _WIN32
	#define UP_fileno       _fileno
	#define UP_open         _open
	#define UP_read         _read
	#define UP_write        _write
	#define UP_lseek        _lseek
	#define UP_close        _close
#else
	#define UP_fileno       fileno
	#define UP_open         open
	#define UP_read         read
	#define UP_write        write
	#define UP_lseek        lseek
	#define UP_close        close
#endif

#endif
//
// } BIO_LCL
//
// (BIO_LCL) #include "e_os.h"

#ifdef OPENSSL_USE_APPLINK
	#undef BIO_FLAGS_UPLINK
	#define BIO_FLAGS_UPLINK 0x8000
	#include "ms/uplink.h"
#endif
#include <openssl/crypto.h>
#include <openssl/buffer.h>
#include <openssl/rand.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/asn1.h>
#include <openssl/asn1t.h>
#include <openssl/md5.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/rsa.h>
#include <openssl/dsa.h>
#include <openssl/ec.h>
#include <openssl/cast.h>
//#include <internal/cryptlib.h>
#include <internal/engine.h>
#include <internal/thread_once.h>

#ifdef  __cplusplus
extern "C" {
#endif

typedef struct ex_callback_st EX_CALLBACK;

DEFINE_STACK_OF(EX_CALLBACK)

typedef struct app_mem_info_st APP_INFO;

typedef struct mem_st MEM;
DEFINE_LHASH_OF(MEM);

#ifndef OPENSSL_SYS_VMS
	#define X509_CERT_AREA          OPENSSLDIR
	#define X509_CERT_DIR           OPENSSLDIR "/certs"
	#define X509_CERT_FILE          OPENSSLDIR "/cert.pem"
	#define X509_PRIVATE_DIR        OPENSSLDIR "/private"
	#define CTLOG_FILE              OPENSSLDIR "/ct_log_list.cnf"
#else
	#define X509_CERT_AREA          "OSSL$DATAROOT:[000000]"
	#define X509_CERT_DIR           "OSSL$DATAROOT:[CERTS]"
	#define X509_CERT_FILE          "OSSL$DATAROOT:[000000]cert.pem"
	#define X509_PRIVATE_DIR        "OSSL$DATAROOT:[PRIVATE]"
	#define CTLOG_FILE              "OSSL$DATAROOT:[000000]ct_log_list.cnf"
#endif
#define X509_CERT_DIR_EVP        "SSL_CERT_DIR"
#define X509_CERT_FILE_EVP       "SSL_CERT_FILE"
#define CTLOG_FILE_EVP           "CTLOG_FILE"

// size of string representations 
#define DECIMAL_SIZE(type)      ((sizeof(type)*8+2)/3+1)
#define HEX_SIZE(type)          (sizeof(type)*2)

void OPENSSL_cpuid_setup(void);
extern uint OPENSSL_ia32cap_P[];
void OPENSSL_showfatal(const char *fmta, ...);
extern int OPENSSL_NONPIC_relocated;
void crypto_cleanup_all_ex_data_int(void);

int openssl_strerror_r(int errnum, char *buf, size_t buflen);
# if !defined(OPENSSL_NO_STDIO)
FILE *openssl_fopen(const char *filename, const char *mode);
# else
void *openssl_fopen(const char *filename, const char *mode);
# endif

#ifdef  __cplusplus
}
#endif
//
// ENG_INT.H
//
#ifdef  __cplusplus
extern "C" {
#endif

extern CRYPTO_RWLOCK *global_engine_lock;

/*
 * If we compile with this symbol defined, then both reference counts in the
 * ENGINE structure will be monitored with a line of output on stderr for
 * each change. This prints the engine's pointer address (truncated to
 * uint), "struct" or "funct" to indicate the reference type, the
 * before and after reference count, and the file:line-number pair. The
 * "engine_ref_debug" statements must come *after* the change.
 */
# ifdef ENGINE_REF_COUNT_DEBUG

#  define engine_ref_debug(e, isfunct, diff) \
        fprintf(stderr, "engine: %08x %s from %d to %d (%s:%d)\n", \
                (uint)(e), (isfunct ? "funct" : "struct"), \
                ((isfunct) ? ((e)->funct_ref - (diff)) : ((e)->struct_ref - (diff))), \
                ((isfunct) ? (e)->funct_ref : (e)->struct_ref), \
                (OPENSSL_FILE), (OPENSSL_LINE))

# else

#  define engine_ref_debug(e, isfunct, diff)

# endif

/*
 * Any code that will need cleanup operations should use these functions to
 * register callbacks. engine_cleanup_int() will call all registered
 * callbacks in order. NB: both the "add" functions assume the engine lock to
 * already be held (in "write" mode).
 */
typedef void (ENGINE_CLEANUP_CB) (void);
typedef struct st_engine_cleanup_item {
    ENGINE_CLEANUP_CB *cb;
} ENGINE_CLEANUP_ITEM;
DEFINE_STACK_OF(ENGINE_CLEANUP_ITEM)
void engine_cleanup_add_first(ENGINE_CLEANUP_CB *cb);
void engine_cleanup_add_last(ENGINE_CLEANUP_CB *cb);

/* We need stacks of ENGINEs for use in eng_table.c */
DEFINE_STACK_OF(ENGINE)

/*
 * If this symbol is defined then engine_table_select(), the function that is
 * used by RSA, DSA (etc) code to select registered ENGINEs, cache defaults
 * and functional references (etc), will display debugging summaries to
 * stderr.
 */
/* #define ENGINE_TABLE_DEBUG */

/*
 * This represents an implementation table. Dependent code should instantiate
 * it as a (ENGINE_TABLE *) pointer value set initially to NULL.
 */
typedef struct st_engine_table ENGINE_TABLE;
int engine_table_register(ENGINE_TABLE **table, ENGINE_CLEANUP_CB *cleanup, ENGINE *e, const int *nids, int num_nids, int setdefault);
void engine_table_unregister(ENGINE_TABLE **table, ENGINE *e);
void engine_table_cleanup(ENGINE_TABLE **table);
# ifndef ENGINE_TABLE_DEBUG
ENGINE *engine_table_select(ENGINE_TABLE **table, int nid);
# else
ENGINE *engine_table_select_tmp(ENGINE_TABLE **table, int nid, const char *f,
                                int l);
#  define engine_table_select(t,n) engine_table_select_tmp(t,n,OPENSSL_FILE,OPENSSL_LINE)
# endif
typedef void (engine_table_doall_cb) (int nid, STACK_OF(ENGINE) *sk, ENGINE *def, void *arg);
void engine_table_doall(ENGINE_TABLE *table, engine_table_doall_cb *cb, void *arg);

/*
 * Internal versions of API functions that have control over locking. These
 * are used between C files when functionality needs to be shared but the
 * caller may already be controlling of the engine lock.
 */
int engine_unlocked_init(ENGINE *e);
int engine_unlocked_finish(ENGINE *e, int unlock_for_handlers);
int engine_free_util(ENGINE *e, int locked);
/*
 * This function will reset all "set"able values in an ENGINE to NULL. This
 * won't touch reference counts or ex_data, but is equivalent to calling all
 * the ENGINE_set_***() functions with a NULL value.
 */
void engine_set_all_null(ENGINE *e);

/*
 * NB: Bitwise OR-able values for the "flags" variable in ENGINE are now
 * exposed in engine.h.
 */

/* Free up dynamically allocated public key methods associated with ENGINE */

void engine_pkey_meths_free(ENGINE *e);
void engine_pkey_asn1_meths_free(ENGINE *e);

/* Once initialisation function */
extern CRYPTO_ONCE engine_lock_init;
DECLARE_RUN_ONCE(do_engine_lock_init)
/*
 * This is a structure for storing implementations of various crypto algorithms and functions.
 */
struct engine_st {
    const char *id;
    const char *name;
    const RSA_METHOD *rsa_meth;
    const DSA_METHOD *dsa_meth;
    const DH_METHOD *dh_meth;
    const EC_KEY_METHOD *ec_meth;
    const RAND_METHOD *rand_meth;
    ENGINE_CIPHERS_PTR ciphers; /* Cipher handling is via this callback */
    ENGINE_DIGESTS_PTR digests; /* Digest handling is via this callback */
    ENGINE_PKEY_METHS_PTR pkey_meths; /* Public key handling via this callback */
    ENGINE_PKEY_ASN1_METHS_PTR pkey_asn1_meths; /* ASN1 public key handling via this callback */
    ENGINE_GEN_INT_FUNC_PTR destroy;
    ENGINE_GEN_INT_FUNC_PTR init;
    ENGINE_GEN_INT_FUNC_PTR finish;
    ENGINE_CTRL_FUNC_PTR ctrl;
    ENGINE_LOAD_KEY_PTR load_privkey;
    ENGINE_LOAD_KEY_PTR load_pubkey;
    ENGINE_SSL_CLIENT_CERT_PTR load_ssl_client_cert;
    const ENGINE_CMD_DEFN *cmd_defns;
    int flags;
    int struct_ref; // reference count on the structure itself 
    /*
     * reference count on usability of the engine type. NB: This controls the
     * loading and initialisation of any functionality required by this
     * engine, whereas the previous count is simply to cope with
     * (de)allocation of this structure. Hence, running_ref <= struct_ref at all times.
     */
    int funct_ref;
    CRYPTO_EX_DATA ex_data; // A place to store per-ENGINE data 
	//
    // Used to maintain the linked-list of engines.
	//
    struct engine_st *prev;
    struct engine_st *next;
};

typedef struct st_engine_pile ENGINE_PILE;

DEFINE_LHASH_OF(ENGINE_PILE);

#ifdef  __cplusplus
}
#endif
//
// } ENG_INT.H
//
#endif
