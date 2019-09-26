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
#define USE_SOCKETS
#include "e_os.h"
//
// BIO_LCL {
//
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
		struct sockaddr * bai_addr;

		struct bio_addrinfo_st * bai_next;
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
	int ibuf_size;          /* how big is the input buffer */
	int obuf_size;          /* how big is the output buffer */
	char * ibuf;            /* the char array */
	int ibuf_len;           /* how many bytes are in it */
	int ibuf_off;           /* write/read offset */
	char * obuf;            /* the char array */
	int obuf_len;           /* how many bytes are in it */
	int obuf_off;           /* write/read offset */
} BIO_F_BUFFER_CTX;

struct bio_st {
	const BIO_METHOD * method;
	// bio, mode, argp, argi, argl, ret
	long (* callback)(struct bio_st *, int, const char *, int, long, long);

	char * cb_arg; // first argument for the callback
	int init;
	int shutdown;
	int flags; // extra storage
	int retry_reason;
	int num;
	void * ptr;
	struct bio_st * next_bio; // used by filter BIOs

	struct bio_st * prev_bio; // used by filter BIOs

	int references;
	uint64_t num_read;
	uint64_t num_write;
	CRYPTO_EX_DATA ex_data;
	CRYPTO_RWLOCK * lock;
};

#ifndef OPENSSL_NO_SOCK
	#ifdef OPENSSL_SYS_VMS
typedef uint socklen_t;
	#endif
extern CRYPTO_RWLOCK * bio_lookup_lock;

int BIO_ADDR_make(BIO_ADDR * ap, const struct sockaddr * sa);
const struct sockaddr * BIO_ADDR_sockaddr(const BIO_ADDR * ap);
struct sockaddr * BIO_ADDR_sockaddr_noconst(BIO_ADDR * ap);
socklen_t BIO_ADDR_sockaddr_size(const BIO_ADDR * ap);
socklen_t BIO_ADDRINFO_sockaddr_size(const BIO_ADDRINFO * bai);
const struct sockaddr * BIO_ADDRINFO_sockaddr(const BIO_ADDRINFO * bai);
#endif

extern CRYPTO_RWLOCK * bio_type_lock;

void bio_sock_cleanup_int(void);

#if BIO_FLAGS_UPLINK==0
/* Shortcut UPLINK calls on most platforms... */
	#define UP_stdin        stdin
	#define UP_stdout       stdout
	#define UP_stderr       stderr
	#define UP_fprintf      fprintf
	#define UP_fgets(mp_str, mp_sz, mp_stream)       fgets((mp_str), (mp_sz), (FILE *)(mp_stream))
	#define UP_fread(mp_ptr, mp_sz, mp_n, mp_stream)  fread((mp_ptr), (mp_sz), (mp_n), (FILE *)(mp_stream))
	#define UP_fwrite(mp_ptr, mp_sz, mp_n, mp_stream) fwrite((mp_ptr), (mp_sz), (mp_n), (FILE *)(mp_stream))
	#undef  UP_fsetmod
	#define UP_feof         feof
	#define UP_fclose(mp_stream) fclose((FILE *)(mp_stream))
	#define UP_fopen        fopen
	#define UP_fseek(mp_stream, mp_offs, mp_org)  fseek((FILE *)mp_stream, mp_offs, mp_org)
	#define UP_ftell(mp_stream)  ftell((FILE *)(mp_stream))
	#define UP_fflush(mp_stream) fflush((FILE *)(mp_stream))
	#define UP_ferror       ferror
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
#include <openssl/ossl_typ.h>
#include <openssl/crypto.h>
#include <openssl/buffer.h>
#include <openssl/rand.h>
#include <openssl/bio.h>
#include <openssl/stack.h>
#include <openssl/safestack.h>
#include <openssl/lhash.h>
#include <openssl/err.h>
#include <openssl/asn1.h>
#include <openssl/asn1t.h>
#include <openssl/md2.h>
#include <openssl/md4.h>
#include <openssl/md5.h>
#include <openssl/sha.h>
#include <openssl/conf.h>
#include <openssl/conf_api.h>
#include <openssl/rsa.h>
#include <openssl/dsa.h>
#include <openssl/des.h>
#include <openssl/dh.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/x509_vfy.h>
#include <openssl/ct.h>
#include <openssl/tls1.h>
#include <openssl/cms.h>
#include <openssl/pem.h>
#include <openssl/ec.h>
#include <openssl/cast.h>
#include <openssl/blowfish.h>
#include <openssl/aes.h>
#include <openssl/camellia.h>
#include <openssl/modes.h>
#include <openssl/engine.h>
#include <openssl/async.h>
#include <openssl/comp.h>
#include <openssl/ui.h>
#include <openssl/ocsp.h>
#include <openssl/pkcs12.h>
#include <openssl/ts.h>
#include <openssl/rc2.h>
#include <openssl/rc4.h>
#include <openssl/rc5.h>
#include <openssl/evp.h>
#include <openssl/cmac.h>
#include <openssl/hmac.h>
#include <openssl/kdf.h>
#include <openssl/pkcs7.h>
#include <openssl/ripemd.h>
#include <openssl/srp.h>
#include <openssl/seed.h>
//#include <internal/engine.h>
void engine_load_openssl_int(void);
void engine_load_cryptodev_int(void);
void engine_load_rdrand_int(void);
void engine_load_dynamic_int(void);
void engine_load_padlock_int(void);
void engine_load_capi_int(void);
void engine_load_dasync_int(void);
void engine_load_afalg_int(void);
void engine_cleanup_int(void);
//
#include <internal/thread_once.h>
//#include <internal/x509_int.h>
//
// Internal X509 structures and functions: not for application use 
// 
// Note: unless otherwise stated a field pointer is mandatory and should
// never be set to NULL: the ASN.1 code and accessors rely on mandatory fields never being NULL.
// 

// 
// name entry structure, equivalent to AttributeTypeAndValue defined in RFC5280 et al.
// 
struct X509_name_entry_st {
    ASN1_OBJECT *object;        /* AttributeType */
    ASN1_STRING *value;         /* AttributeValue */
    int set;                    /* index of RDNSequence for this entry */
    int size;                   /* temp variable */
};
// 
// Name from RFC 5280. 
// 
struct X509_name_st {
    STACK_OF(X509_NAME_ENTRY) *entries; /* DN components */
    int modified;               /* true if 'bytes' needs to be built */
    BUF_MEM *bytes;             /* cached encoding: cannot be NULL */
    /* canonical encoding used for rapid Name comparison */
    uchar *canon_enc;
    int canon_enclen;
} /* X509_NAME */ ;

/* PKCS#10 certificate request */

struct X509_req_info_st {
    ASN1_ENCODING enc;          /* cached encoding of signed part */
    ASN1_INTEGER *version;      /* version, defaults to v1(0) so can be NULL */
    X509_NAME *subject;         /* certificate request DN */
    X509_PUBKEY *pubkey;        /* public key of request */
    /*
 * Zero or more attributes.
 * NB: although attributes is a mandatory field some broken
 * encodings omit it so this may be NULL in that case.
     */
    STACK_OF(X509_ATTRIBUTE) *attributes;
};

struct X509_req_st {
    X509_REQ_INFO req_info;     /* signed certificate request data */
    X509_ALGOR sig_alg;         /* signature algorithm */
    ASN1_BIT_STRING *signature; /* signature */
    int references;
    CRYPTO_RWLOCK *lock;
};

struct X509_crl_info_st {
    ASN1_INTEGER *version;      /* version: defaults to v1(0) so may be NULL */
    X509_ALGOR sig_alg;         /* signature algorithm */
    X509_NAME *issuer;          /* CRL issuer name */
    ASN1_TIME *lastUpdate;      /* lastUpdate field */
    ASN1_TIME *nextUpdate;      /* nextUpdate field: optional */
    STACK_OF(X509_REVOKED) *revoked;        /* revoked entries: optional */
    STACK_OF(X509_EXTENSION) *extensions;   /* extensions: optional */
    ASN1_ENCODING enc;                      /* encoding of signed portion of CRL */
};

struct X509_crl_st {
    X509_CRL_INFO crl;          /* signed CRL data */
    X509_ALGOR sig_alg;         /* CRL signature algorithm */
    ASN1_BIT_STRING signature;  /* CRL signature */
    int references;
    int flags;
    /*
 * Cached copies of decoded extension values, since extensions
 * are optional any of these can be NULL.
     */
    AUTHORITY_KEYID *akid;
    ISSUING_DIST_POINT *idp;
    /* Convenient breakdown of IDP */
    int idp_flags;
    int idp_reasons;
    /* CRL and base CRL numbers for delta processing */
    ASN1_INTEGER *crl_number;
    ASN1_INTEGER *base_crl_number;
    STACK_OF(GENERAL_NAMES) *issuers;
    uchar sha1_hash[SHA_DIGEST_LENGTH]; // hash of CRL 
    const X509_CRL_METHOD * meth; // alternative method to handle this CRL 
    void *meth_data;
    CRYPTO_RWLOCK *lock;
};

struct x509_revoked_st {
    ASN1_INTEGER serialNumber; /* revoked entry serial number */
    ASN1_TIME *revocationDate;  /* revocation date */
    STACK_OF(X509_EXTENSION) *extensions;   /* CRL entry extensions: optional */
    /* decoded value of CRLissuer extension: set if indirect CRL */
    STACK_OF(GENERAL_NAME) *issuer;
    /* revocation reason: set to CRL_REASON_NONE if reason extension absent */
    int reason;
    /*
 * CRL entries are reordered for faster lookup of serial numbers. This
 * field contains the original load sequence for this entry.
     */
    int sequence;
};
/*
 * This stuff is certificate "auxiliary info": it contains details which are
 * useful in certificate stores and databases. When used this is tagged onto
 * the end of the certificate itself. OpenSSL specific structure not defined
 * in any RFC.
 */
struct x509_cert_aux_st {
    STACK_OF(ASN1_OBJECT) *trust; /* trusted uses */
    STACK_OF(ASN1_OBJECT) *reject; /* rejected uses */
    ASN1_UTF8STRING *alias;     /* "friendly name" */
    ASN1_OCTET_STRING *keyid;   /* key id of private key */
    STACK_OF(X509_ALGOR) *other; /* other unspecified info */
};

struct x509_cinf_st {
    ASN1_INTEGER *version;      /* [ 0 ] default of v1 */
    ASN1_INTEGER serialNumber;
    X509_ALGOR signature;
    X509_NAME *issuer;
    X509_VAL validity;
    X509_NAME *subject;
    X509_PUBKEY *key;
    ASN1_BIT_STRING *issuerUID; /* [ 1 ] optional in v2 */
    ASN1_BIT_STRING *subjectUID; /* [ 2 ] optional in v2 */
    STACK_OF(X509_EXTENSION) *extensions; /* [ 3 ] optional in v3 */
    ASN1_ENCODING enc;
};

struct x509_st {
    X509_CINF cert_info;
    X509_ALGOR sig_alg;
    ASN1_BIT_STRING signature;
    int references;
    CRYPTO_EX_DATA ex_data;
    /* These contain copies of various extension values */
    long ex_pathlen;
    long ex_pcpathlen;
    uint32_t ex_flags;
    uint32_t ex_kusage;
    uint32_t ex_xkusage;
    uint32_t ex_nscert;
    ASN1_OCTET_STRING *skid;
    AUTHORITY_KEYID *akid;
    X509_POLICY_CACHE *policy_cache;
    STACK_OF(DIST_POINT) *crldp;
    STACK_OF(GENERAL_NAME) *altname;
    NAME_CONSTRAINTS *nc;
#ifndef OPENSSL_NO_RFC3779
    STACK_OF(IPAddressFamily) *rfc3779_addr;
    struct ASIdentifiers_st *rfc3779_asid;
# endif
    uchar sha1_hash[SHA_DIGEST_LENGTH];
    X509_CERT_AUX *aux;
    CRYPTO_RWLOCK *lock;
} /* X509 */ ;

/*
 * This is a used when verifying cert chains.  Since the gathering of the
 * cert chain can take some time (and have to be 'retried', this needs to be
 * kept and passed around.
 */
struct x509_store_ctx_st {      /* X509_STORE_CTX */
    X509_STORE *ctx;
    /* The following are set by the caller */
    /* The cert to check */
    X509 *cert;
    STACK_OF(X509) * untrusted; // chain of X509s - untrusted - passed in 
    STACK_OF(X509_CRL) *crls; // set of CRLs passed in 
    X509_VERIFY_PARAM *param;
    void * other_ctx; // Other info for use with get_issuer() 
    /* Callbacks for various operations */
    int (*verify) (X509_STORE_CTX *ctx); /* called to verify a certificate */
    int (*verify_cb) (int ok, X509_STORE_CTX *ctx); /* error callback */
	int (*get_issuer) (X509 **issuer, X509_STORE_CTX *ctx, X509 *x); /* get issuers cert from ctx */
    int (*check_issued) (X509_STORE_CTX *ctx, X509 *x, X509 *issuer); /* check issued */
    int (*check_revocation) (X509_STORE_CTX *ctx); /* Check revocation status of chain */
    int (*get_crl) (X509_STORE_CTX *ctx, X509_CRL **crl, X509 *x); /* retrieve CRL */
    int (*check_crl) (X509_STORE_CTX *ctx, X509_CRL *crl); /* Check CRL validity */
    int (*cert_crl) (X509_STORE_CTX *ctx, X509_CRL *crl, X509 *x); /* Check certificate against CRL */
    int (*check_policy) (X509_STORE_CTX *ctx); /* Check policy status of the chain */
    STACK_OF(X509) *(*lookup_certs) (X509_STORE_CTX *ctx, X509_NAME *nm);
    STACK_OF(X509_CRL) *(*lookup_crls) (X509_STORE_CTX *ctx, X509_NAME *nm);
    int (*cleanup) (X509_STORE_CTX *ctx);
    /* The following is built up */
    /* if 0, rebuild chain */
    int valid;
    int num_untrusted; // number of untrusted certs 
    STACK_OF(X509) *chain; // chain of X509s - built up and trusted 
    X509_POLICY_TREE * tree; // Valid policy tree 
    int explicit_policy; // Require explicit policy value 
    int error_depth; // When something goes wrong, this is why 
    int error;
    X509 *current_cert;
    X509 *current_issuer; // cert currently being tested as valid issuer 
    X509_CRL *current_crl; // current CRL 
    int current_crl_score; // score of current CRL 
    uint current_reasons; // Reason mask 
    X509_STORE_CTX * parent; // For CRL path validation: parent context 
    CRYPTO_EX_DATA ex_data;
    SSL_DANE *dane;
    int bare_ta_signed; // signed via bare TA public key, rather than CA certificate 
};

/* PKCS#8 private key info structure */

struct pkcs8_priv_key_info_st {
    ASN1_INTEGER *version;
    X509_ALGOR *pkeyalg;
    ASN1_OCTET_STRING *pkey;
    STACK_OF(X509_ATTRIBUTE) *attributes;
};

struct X509_sig_st {
    X509_ALGOR *algor;
    ASN1_OCTET_STRING *digest;
};

struct x509_object_st {
    /* one of the above types */
    X509_LOOKUP_TYPE type;
    union {
        char *ptr;
        X509 *x509;
        X509_CRL *crl;
        EVP_PKEY *pkey;
    } data;
};

int a2i_ipadd(uchar *ipout, const char *ipasc);
int x509_set1_time(ASN1_TIME **ptm, const ASN1_TIME *tm);
//
//#include <internal/asn1_int.h>
//
// Internal ASN1 structures and functions: not for application use 
//
// ASN1 public key method structure 
//
struct evp_pkey_asn1_method_st {
	int pkey_id;
	int pkey_base_id;
	ulong pkey_flags;
	char * pem_str;
	char * info;
	int (* pub_decode)(EVP_PKEY * pk, X509_PUBKEY * pub);
	int (* pub_encode)(X509_PUBKEY * pub, const EVP_PKEY * pk);
	int (* pub_cmp)(const EVP_PKEY * a, const EVP_PKEY * b);
	int (* pub_print)(BIO * out, const EVP_PKEY * pkey, int indent, ASN1_PCTX * pctx);
	int (* priv_decode)(EVP_PKEY * pk, const PKCS8_PRIV_KEY_INFO * p8inf);
	int (* priv_encode)(PKCS8_PRIV_KEY_INFO * p8, const EVP_PKEY * pk);
	int (* priv_print)(BIO * out, const EVP_PKEY * pkey, int indent, ASN1_PCTX * pctx);
	int (* pkey_size)(const EVP_PKEY * pk);
	int (* pkey_bits)(const EVP_PKEY * pk);
	int (* pkey_security_bits)(const EVP_PKEY * pk);
	int (* param_decode)(EVP_PKEY * pkey, const uchar ** pder, int derlen);
	int (* param_encode)(const EVP_PKEY * pkey, uchar ** pder);
	int (* param_missing)(const EVP_PKEY * pk);
	int (* param_copy)(EVP_PKEY * to, const EVP_PKEY * from);
	int (* param_cmp)(const EVP_PKEY * a, const EVP_PKEY * b);
	int (* param_print)(BIO * out, const EVP_PKEY * pkey, int indent, ASN1_PCTX * pctx);
	int (* sig_print)(BIO * out, const X509_ALGOR * sigalg, const ASN1_STRING * sig, int indent, ASN1_PCTX * pctx);
	void (* pkey_free)(EVP_PKEY * pkey);
	int (* pkey_ctrl)(EVP_PKEY * pkey, int op, long arg1, void * arg2);
	/* Legacy functions for old PEM */
	int (* old_priv_decode)(EVP_PKEY * pkey, const uchar ** pder, int derlen);
	int (* old_priv_encode)(const EVP_PKEY * pkey, uchar ** pder);
	/* Custom ASN1 signature verification */
	int (* item_verify)(EVP_MD_CTX * ctx, const ASN1_ITEM * it, void * asn, X509_ALGOR * a, ASN1_BIT_STRING * sig, EVP_PKEY * pkey);
	int (* item_sign)(EVP_MD_CTX * ctx, const ASN1_ITEM * it, void * asn, X509_ALGOR * alg1, X509_ALGOR * alg2, ASN1_BIT_STRING * sig);
} /* EVP_PKEY_ASN1_METHOD */;

DEFINE_STACK_OF_CONST(EVP_PKEY_ASN1_METHOD)

extern const EVP_PKEY_ASN1_METHOD cmac_asn1_meth;
extern const EVP_PKEY_ASN1_METHOD dh_asn1_meth;
extern const EVP_PKEY_ASN1_METHOD dhx_asn1_meth;
extern const EVP_PKEY_ASN1_METHOD dsa_asn1_meths[5];
extern const EVP_PKEY_ASN1_METHOD eckey_asn1_meth;
extern const EVP_PKEY_ASN1_METHOD ecx25519_asn1_meth;
extern const EVP_PKEY_ASN1_METHOD hmac_asn1_meth;
extern const EVP_PKEY_ASN1_METHOD rsa_asn1_meths[2];
/*
 * These are used internally in the ASN1_OBJECT to keep track of whether the
 * names and data need to be free()ed
 */
# define ASN1_OBJECT_FLAG_DYNAMIC         0x01 /* internal use */
# define ASN1_OBJECT_FLAG_CRITICAL        0x02 /* critical x509v3 object id */
# define ASN1_OBJECT_FLAG_DYNAMIC_STRINGS 0x04 /* internal use */
# define ASN1_OBJECT_FLAG_DYNAMIC_DATA    0x08 /* internal use */
struct asn1_object_st {
	const char * sn, * ln;
	int nid;
	int length;
	const uchar * data; /* data remains const after init */
	int flags;              /* Should we free this one */
};
// 
// ASN1 print context structure 
// 
struct asn1_pctx_st {
	ulong flags;
	ulong nm_flags;
	ulong cert_flags;
	ulong oid_flags;
	ulong str_flags;
} /* ASN1_PCTX */;

int asn1_valid_host(const ASN1_STRING * host);
//
//#include <internal/evp_int.h>
struct evp_pkey_ctx_st {
	const EVP_PKEY_METHOD * pmeth; /* Method associated with this operation */
	ENGINE * engine; /* Engine that implements this method or NULL if builtin */
	EVP_PKEY * pkey; /* Key: may be NULL */
	EVP_PKEY * peerkey; /* Peer key for key agreement, may be NULL */
	int operation; /* Actual operation */
	void * data; /* Algorithm specific data */
	void * app_data; /* Application specific data */
	EVP_PKEY_gen_cb * pkey_gencb; /* Keygen callback */
	int * keygen_info; /* Keygen callback */
	int keygen_info_count;
} /* EVP_PKEY_CTX */;

#define EVP_PKEY_FLAG_DYNAMIC   1

struct evp_pkey_method_st {
	int pkey_id;
	int flags;
	int (* init)(EVP_PKEY_CTX * ctx);
	int (* copy)(EVP_PKEY_CTX * dst, EVP_PKEY_CTX * src);
	void (* cleanup)(EVP_PKEY_CTX * ctx);
	int (* paramgen_init)(EVP_PKEY_CTX * ctx);
	int (* paramgen)(EVP_PKEY_CTX * ctx, EVP_PKEY * pkey);
	int (* keygen_init)(EVP_PKEY_CTX * ctx);
	int (* keygen)(EVP_PKEY_CTX * ctx, EVP_PKEY * pkey);
	int (* sign_init)(EVP_PKEY_CTX * ctx);
	int (* sign)(EVP_PKEY_CTX * ctx, uchar * sig, size_t * siglen, const uchar * tbs, size_t tbslen);
	int (* verify_init)(EVP_PKEY_CTX * ctx);
	int (* verify)(EVP_PKEY_CTX * ctx, const uchar * sig, size_t siglen, const uchar * tbs, size_t tbslen);
	int (* verify_recover_init)(EVP_PKEY_CTX * ctx);
	int (* verify_recover)(EVP_PKEY_CTX * ctx, uchar * rout, size_t * routlen, const uchar * sig, size_t siglen);
	int (* signctx_init)(EVP_PKEY_CTX * ctx, EVP_MD_CTX * mctx);
	int (* signctx)(EVP_PKEY_CTX * ctx, uchar * sig, size_t * siglen, EVP_MD_CTX * mctx);
	int (* verifyctx_init)(EVP_PKEY_CTX * ctx, EVP_MD_CTX * mctx);
	int (* verifyctx)(EVP_PKEY_CTX * ctx, const uchar * sig, int siglen, EVP_MD_CTX * mctx);
	int (* encrypt_init)(EVP_PKEY_CTX * ctx);
	int (* encrypt)(EVP_PKEY_CTX * ctx, uchar * out, size_t * outlen, const uchar * in, size_t inlen);
	int (* decrypt_init)(EVP_PKEY_CTX * ctx);
	int (* decrypt)(EVP_PKEY_CTX * ctx, uchar * out, size_t * outlen, const uchar * in, size_t inlen);
	int (* derive_init)(EVP_PKEY_CTX * ctx);
	int (* derive)(EVP_PKEY_CTX * ctx, uchar * key, size_t * keylen);
	int (* ctrl)(EVP_PKEY_CTX * ctx, int type, int p1, void * p2);
	int (* ctrl_str)(EVP_PKEY_CTX * ctx, const char * type, const char * value);
} /* EVP_PKEY_METHOD */;

DEFINE_STACK_OF_CONST(EVP_PKEY_METHOD)

void evp_pkey_set_cb_translate(BN_GENCB * cb, EVP_PKEY_CTX * ctx);

extern const EVP_PKEY_METHOD cmac_pkey_meth;
extern const EVP_PKEY_METHOD dh_pkey_meth;
extern const EVP_PKEY_METHOD dhx_pkey_meth;
extern const EVP_PKEY_METHOD dsa_pkey_meth;
extern const EVP_PKEY_METHOD ec_pkey_meth;
extern const EVP_PKEY_METHOD ecx25519_pkey_meth;
extern const EVP_PKEY_METHOD hmac_pkey_meth;
extern const EVP_PKEY_METHOD rsa_pkey_meth;
extern const EVP_PKEY_METHOD tls1_prf_pkey_meth;
extern const EVP_PKEY_METHOD hkdf_pkey_meth;

struct evp_md_st {
	int type;
	int pkey_type;
	int md_size;
	ulong flags;
	int (* init)(EVP_MD_CTX * ctx);
	int (* update)(EVP_MD_CTX * ctx, const void * data, size_t count);
	int (* final)(EVP_MD_CTX * ctx, uchar * md);
	int (* copy)(EVP_MD_CTX * to, const EVP_MD_CTX * from);
	int (* cleanup)(EVP_MD_CTX * ctx);
	int block_size;
	int ctx_size;           /* how big does the ctx->md_data need to be */
	/* control function */
	int (* md_ctrl)(EVP_MD_CTX * ctx, int cmd, int p1, void * p2);
} /* EVP_MD */;

struct evp_cipher_st {
	int nid;
	int block_size;
	int key_len; // Default value for variable length ciphers 
	int iv_len;
	ulong flags; // Various flags 
	int (* init)(EVP_CIPHER_CTX * ctx, const uchar * key, const uchar * iv, int enc); // init key 
	int (* do_cipher)(EVP_CIPHER_CTX * ctx, uchar * out, const uchar * in, size_t inl); // encrypt/decrypt data 
	int (* cleanup)(EVP_CIPHER_CTX *); // cleanup ctx 
	int ctx_size; // how big ctx->cipher_data needs to be 
	int (* set_asn1_parameters)(EVP_CIPHER_CTX *, ASN1_TYPE *); // Populate a ASN1_TYPE with parameters 
	int (* get_asn1_parameters)(EVP_CIPHER_CTX *, ASN1_TYPE *); // Get parameters from a ASN1_TYPE 
	int (* ctrl)(EVP_CIPHER_CTX *, int type, int arg, void * ptr); // Miscellaneous operations 
	void * app_data; // Application data 
} /* EVP_CIPHER */;

/* Macros to code block cipher wrappers */

/* Wrapper functions for each cipher mode */

#define EVP_C_DATA(kstruct, ctx) ((kstruct*)EVP_CIPHER_CTX_get_cipher_data(ctx))
#define BLOCK_CIPHER_ecb_loop()	\
	size_t i, bl; \
	bl = EVP_CIPHER_CTX_cipher(ctx)->block_size;	\
	if(inl < bl) return 1; \
	inl -= bl; \
	for(i = 0; i <= inl; i += bl)

#define BLOCK_CIPHER_func_ecb(cname, cprefix, kstruct, ksched) \
	static int cname ## _ecb_cipher(EVP_CIPHER_CTX *ctx, uchar *out, const uchar *in, size_t inl) \
	{ \
		BLOCK_CIPHER_ecb_loop()	\
		cprefix ## _ecb_encrypt(in + i, out + i, &EVP_C_DATA(kstruct, ctx)->ksched, EVP_CIPHER_CTX_encrypting(ctx)); \
		return 1; \
	}

#define EVP_MAXCHUNK ((size_t)1<<(sizeof(long)*8-2))

#define BLOCK_CIPHER_func_ofb(cname, cprefix, cbits, kstruct, ksched) \
	static int cname ## _ofb_cipher(EVP_CIPHER_CTX *ctx, uchar *out, const uchar *in, size_t inl) \
	{ \
		while(inl>=EVP_MAXCHUNK) { \
			int num = EVP_CIPHER_CTX_num(ctx); \
			cprefix ## _ofb ## cbits ## _encrypt(in, out, (long)EVP_MAXCHUNK, &EVP_C_DATA(kstruct, \
				    ctx)->ksched, EVP_CIPHER_CTX_iv_noconst(ctx), &num); \
			EVP_CIPHER_CTX_set_num(ctx, num); \
			inl -= EVP_MAXCHUNK; \
			in += EVP_MAXCHUNK; \
			out += EVP_MAXCHUNK; \
		} \
		if(inl) { \
			int num = EVP_CIPHER_CTX_num(ctx); \
			cprefix ## _ofb ## cbits ## _encrypt(in, out, (long)inl, &EVP_C_DATA(kstruct, \
				    ctx)->ksched, EVP_CIPHER_CTX_iv_noconst(ctx), &num); \
			EVP_CIPHER_CTX_set_num(ctx, num); \
		} \
		return 1; \
	}

#define BLOCK_CIPHER_func_cbc(cname, cprefix, kstruct, ksched) \
	static int cname ## _cbc_cipher(EVP_CIPHER_CTX *ctx, uchar *out, const uchar *in, size_t inl) \
	{ \
		while(inl>=EVP_MAXCHUNK) { \
			cprefix ## _cbc_encrypt(in, out, (long)EVP_MAXCHUNK, \
			    &EVP_C_DATA(kstruct, ctx)->ksched, EVP_CIPHER_CTX_iv_noconst(ctx), EVP_CIPHER_CTX_encrypting(ctx));	\
			inl -= EVP_MAXCHUNK; \
			in += EVP_MAXCHUNK; \
			out += EVP_MAXCHUNK; \
		} \
		if(inl)	\
			cprefix ## _cbc_encrypt(in, out, (long)inl, &EVP_C_DATA(kstruct, ctx)->ksched, EVP_CIPHER_CTX_iv_noconst( \
				    ctx), EVP_CIPHER_CTX_encrypting(ctx)); \
		return 1; \
	}

#define BLOCK_CIPHER_func_cfb(cname, cprefix, cbits, kstruct, ksched)  \
	static int cname ## _cfb ## cbits ## _cipher(EVP_CIPHER_CTX *ctx, uchar *out, const uchar *in, size_t inl) \
	{ \
		size_t chunk = EVP_MAXCHUNK; \
		if(cbits == 1) chunk >>= 3; \
		if(inl < chunk) chunk = inl; \
		while(inl && inl >= chunk) { \
			int num = EVP_CIPHER_CTX_num(ctx); \
			cprefix ## _cfb ## cbits ## _encrypt(in, out, (long) \
			    ((cbits == 1) && !EVP_CIPHER_CTX_test_flags(ctx, EVP_CIPH_FLAG_LENGTH_BITS) ? inl*8 : inl), \
			    &EVP_C_DATA(kstruct, ctx)->ksched, EVP_CIPHER_CTX_iv_noconst(ctx), &num, EVP_CIPHER_CTX_encrypting(ctx)); \
			EVP_CIPHER_CTX_set_num(ctx, num); \
			inl -= chunk; \
			in += chunk; \
			out += chunk; \
			if(inl < chunk) chunk = inl; \
		} \
		return 1; \
	}

#define BLOCK_CIPHER_all_funcs(cname, cprefix, cbits, kstruct, ksched) \
	BLOCK_CIPHER_func_cbc(cname, cprefix, kstruct, ksched) \
	BLOCK_CIPHER_func_cfb(cname, cprefix, cbits, kstruct, ksched) \
	BLOCK_CIPHER_func_ecb(cname, cprefix, kstruct, ksched) \
	BLOCK_CIPHER_func_ofb(cname, cprefix, cbits, kstruct, ksched)

#define BLOCK_CIPHER_def1(cname, nmode, mode, MODE, kstruct, nid, block_size, key_len, iv_len, flags, init_key, cleanup, set_asn1, get_asn1, ctrl) \
	static const EVP_CIPHER cname ## _ ## mode = { \
		nid ## _ ## nmode, block_size, key_len, iv_len,	 flags | EVP_CIPH_ ## MODE ## _MODE, \
		init_key, cname ## _ ## mode ## _cipher, cleanup,  sizeof(kstruct), \
		set_asn1, get_asn1, ctrl, NULL \
	}; \
	const EVP_CIPHER * EVP_ ## cname ## _ ## mode(void) { return &cname ## _ ## mode; }

#define BLOCK_CIPHER_def_cbc(cname, kstruct, nid, block_size, key_len, iv_len, flags, init_key, cleanup, set_asn1, get_asn1, ctrl) \
	BLOCK_CIPHER_def1(cname, cbc, cbc, CBC, kstruct, nid, block_size, key_len, iv_len, flags, init_key, cleanup, set_asn1, get_asn1, ctrl)
#define BLOCK_CIPHER_def_cfb(cname, kstruct, nid, key_len, iv_len, cbits, flags, init_key, cleanup, set_asn1, get_asn1, ctrl) \
	BLOCK_CIPHER_def1(cname, cfb ## cbits, cfb ## cbits, CFB, kstruct, nid, 1, key_len, iv_len, flags, init_key, cleanup, set_asn1, get_asn1, ctrl)
#define BLOCK_CIPHER_def_ofb(cname, kstruct, nid, key_len, iv_len, cbits, flags, init_key, cleanup, set_asn1, get_asn1, ctrl) \
	BLOCK_CIPHER_def1(cname, ofb ## cbits, ofb, OFB, kstruct, nid, 1, key_len, iv_len, flags, init_key, cleanup, set_asn1, get_asn1, ctrl)
#define BLOCK_CIPHER_def_ecb(cname, kstruct, nid, block_size, key_len, flags, init_key, cleanup, set_asn1, get_asn1, ctrl) \
	BLOCK_CIPHER_def1(cname, ecb, ecb, ECB, kstruct, nid, block_size, key_len, 0, flags, init_key, cleanup, set_asn1, get_asn1, ctrl)
#define BLOCK_CIPHER_defs(cname, kstruct, nid, block_size, key_len, iv_len, cbits, flags, init_key, cleanup, set_asn1, get_asn1, ctrl) \
	BLOCK_CIPHER_def_cbc(cname, kstruct, nid, block_size, key_len, iv_len, flags, init_key, cleanup, set_asn1, get_asn1, ctrl) \
	BLOCK_CIPHER_def_cfb(cname, kstruct, nid, key_len, iv_len, cbits, flags, init_key, cleanup, set_asn1, get_asn1, ctrl)	\
	BLOCK_CIPHER_def_ofb(cname, kstruct, nid, key_len, iv_len, cbits, flags, init_key, cleanup, set_asn1, get_asn1, ctrl)	\
	BLOCK_CIPHER_def_ecb(cname, kstruct, nid, block_size, key_len, flags, init_key, cleanup, set_asn1, get_asn1, ctrl)

/*-
   #define BLOCK_CIPHER_defs(cname, kstruct, \
                                nid, block_size, key_len, iv_len, flags,\
                                 init_key, cleanup, set_asn1, get_asn1, ctrl)\
   static const EVP_CIPHER cname##_cbc = {\
        nid##_cbc, block_size, key_len, iv_len, \
        flags | EVP_CIPH_CBC_MODE,\
        init_key,\
        cname##_cbc_cipher,\
        cleanup,\
        sizeof(EVP_CIPHER_CTX)-sizeof((((EVP_CIPHER_CTX *)NULL)->c))+\
                sizeof((((EVP_CIPHER_CTX *)NULL)->c.kstruct)),\
        set_asn1, get_asn1,\
        ctrl, \
        NULL \
   };\
   const EVP_CIPHER *EVP_##cname##_cbc(void) { return &cname##_cbc; }\
   static const EVP_CIPHER cname##_cfb = {\
        nid##_cfb64, 1, key_len, iv_len, \
        flags | EVP_CIPH_CFB_MODE,\
        init_key,\
        cname##_cfb_cipher,\
        cleanup,\
        sizeof(EVP_CIPHER_CTX)-sizeof((((EVP_CIPHER_CTX *)NULL)->c))+\
                sizeof((((EVP_CIPHER_CTX *)NULL)->c.kstruct)),\
        set_asn1, get_asn1,\
        ctrl,\
        NULL \
   };\
   const EVP_CIPHER *EVP_##cname##_cfb(void) { return &cname##_cfb; }\
   static const EVP_CIPHER cname##_ofb = {\
        nid##_ofb64, 1, key_len, iv_len, \
        flags | EVP_CIPH_OFB_MODE,\
        init_key,\
        cname##_ofb_cipher,\
        cleanup,\
        sizeof(EVP_CIPHER_CTX)-sizeof((((EVP_CIPHER_CTX *)NULL)->c))+sizeof((((EVP_CIPHER_CTX *)NULL)->c.kstruct)),\
        set_asn1, get_asn1,\
        ctrl,\
        NULL \
   };\
   const EVP_CIPHER *EVP_##cname##_ofb(void) { return &cname##_ofb; }\
   static const EVP_CIPHER cname##_ecb = {\
        nid##_ecb, block_size, key_len, iv_len, \
        flags | EVP_CIPH_ECB_MODE,\
        init_key,\
        cname##_ecb_cipher,\
        cleanup,\
        sizeof(EVP_CIPHER_CTX)-sizeof((((EVP_CIPHER_CTX *)NULL)->c))+\
                sizeof((((EVP_CIPHER_CTX *)NULL)->c.kstruct)),\
        set_asn1, get_asn1,\
        ctrl,\
        NULL \
   };\
   const EVP_CIPHER *EVP_##cname##_ecb(void) { return &cname##_ecb; }
 */

#define IMPLEMENT_BLOCK_CIPHER(cname, ksched, cprefix, kstruct, nid, block_size, key_len, iv_len, cbits, flags, init_key, cleanup, set_asn1, get_asn1, ctrl) \
	BLOCK_CIPHER_all_funcs(cname, cprefix, cbits, kstruct, ksched) \
	BLOCK_CIPHER_defs(cname, kstruct, nid, block_size, key_len, iv_len, cbits, flags, init_key, cleanup, set_asn1, get_asn1, ctrl)

#define IMPLEMENT_CFBR(cipher, cprefix, kstruct, ksched, keysize, cbits, iv_len, fl) \
	BLOCK_CIPHER_func_cfb(cipher ## _ ## keysize, cprefix, cbits, kstruct, ksched) \
	BLOCK_CIPHER_def_cfb(cipher ## _ ## keysize, kstruct, NID_ ## cipher ## _ ## keysize, keysize/8, iv_len, cbits, \
	    (fl)|EVP_CIPH_FLAG_DEFAULT_ASN1, cipher ## _init_key, NULL, NULL, NULL, NULL)

/*
 * Type needs to be a bit field Sub-type needs to be for variations on the
 * method, as in, can it do arbitrary encryption....
 */
struct evp_pkey_st {
	int type;
	int save_type;
	int references;
	const EVP_PKEY_ASN1_METHOD * ameth;
	ENGINE * engine;
	union {
		void * ptr;
#ifndef OPENSSL_NO_RSA
		struct rsa_st * rsa; /* RSA */
#endif
#ifndef OPENSSL_NO_DSA
		struct dsa_st * dsa; /* DSA */
#endif
#ifndef OPENSSL_NO_DH
		struct dh_st * dh; /* DH */
#endif
#ifndef OPENSSL_NO_EC
		struct ec_key_st * ec; /* ECC */
#endif
	} pkey;
	int save_parameters;
	STACK_OF(X509_ATTRIBUTE) *attributes; /* [ 0 ] */
	CRYPTO_RWLOCK * lock;
} /* EVP_PKEY */;

void openssl_add_all_ciphers_int(void);
void openssl_add_all_digests_int(void);
void evp_cleanup_int(void);
// 
// Pulling defines out of C soure files 
// 
#define EVP_RC4_KEY_SIZE 16
#ifndef TLS1_1_VERSION
	#define TLS1_1_VERSION   0x0302
#endif
//
//#include <internal/cryptlib_int.h>
//#include <internal/cryptlib.h>
// This file is not scanned by mkdef.pl, whereas cryptlib.h is 

struct thread_local_inits_st {
    int async;
    int err_state;
};

int ossl_init_thread_start(uint64_t opts);
// 
// OPENSSL_INIT flags. The primary list of these is in crypto.h. Flags below
// are those omitted from crypto.h because they are "reserved for internal use".
// 
#define OPENSSL_INIT_ZLIB                   0x00010000L
// OPENSSL_INIT_THREAD flags 
#define OPENSSL_INIT_THREAD_ASYNC           0x01
#define OPENSSL_INIT_THREAD_ERR_STATE       0x02
//
//#include <internal/rand.h>
void rand_cleanup_int(void);
//
//#include <internal/conf.h>
#ifdef __cplusplus
extern "C" {
#endif
	struct ossl_init_settings_st {
		char * appname;
	};

	void openssl_config_int(const char *appname);
	void openssl_no_config_int(void);
	void conf_modules_free_int(void);
#ifdef __cplusplus
}
#endif
//
//#include <internal/async.h>
int async_init(void);
void async_deinit(void);
//
//#include <internal/comp.h>
void comp_zlib_cleanup_int(void);
//
#include <internal/err.h>
//#include <internal/err_int.h>
int err_load_crypto_strings_int(void);
void err_cleanup(void);
void err_delete_thread_state(void);
//
//#include <internal/objects.h>
void obj_cleanup_int(void);
//
//#include <internal/dso.h>
#ifdef __cplusplus
extern "C" {
#endif
	//
	// These values are used as commands to DSO_ctrl() 
	//
	#define DSO_CTRL_GET_FLAGS      1
	#define DSO_CTRL_SET_FLAGS      2
	#define DSO_CTRL_OR_FLAGS       3
	/*
	 * By default, DSO_load() will translate the provided filename into a form
	 * typical for the platform using the dso_name_converter function of the
	 * method. Eg. win32 will transform "blah" into "blah.dll", and dlfcn will
	 * transform it into "libblah.so". This callback could even utilise the
	 * DSO_METHOD's converter too if it only wants to override behaviour for
	 * one or two possible DSO methods. However, the following flag can be
	 * set in a DSO to prevent *any* native name-translation at all - eg. if
	 * the caller has prompted the user for a path to a driver library so the
	 * filename should be interpreted as-is.
	 */
	#define DSO_FLAG_NO_NAME_TRANSLATION            0x01
	/*
	 * An extra flag to give if only the extension should be added as
	 * translation.  This is obviously only of importance on Unix and other
	 * operating systems where the translation also may prefix the name with
	 * something, like 'lib', and ignored everywhere else. This flag is also
	 * ignored if DSO_FLAG_NO_NAME_TRANSLATION is used at the same time.
	 */
	#define DSO_FLAG_NAME_TRANSLATION_EXT_ONLY      0x02
	#define DSO_FLAG_NO_UNLOAD_ON_FREE              0x04 // Don't unload the DSO when we call DSO_free()
	/*
	 * The following flag controls the translation of symbol names to upper case.
	 * This is currently only being implemented for OpenVMS.
	 */
	#define DSO_FLAG_UPCASE_SYMBOL                  0x10
	/*
	 * This flag loads the library with public symbols. Meaning: The exported
	 * symbols of this library are public to all libraries loaded after this
	 * library. At the moment only implemented in unix.
	 */
	#define DSO_FLAG_GLOBAL_SYMBOLS                 0x20

	/*@funcdef*/typedef void (*DSO_FUNC_TYPE) (void);
	typedef struct dso_st DSO;
	typedef struct dso_meth_st DSO_METHOD;

	/*
	 * The function prototype used for method functions (or caller-provided
	 * callbacks) that transform filenames. They are passed a DSO structure
	 * pointer (or NULL if they are to be used independently of a DSO object) and
	 * a filename to transform. They should either return NULL (if there is an
	 * error condition) or a newly allocated string containing the transformed
	 * form that the caller will need to free with OPENSSL_free() when done.
	 */
	/*@funcdef*/typedef char *(*DSO_NAME_CONVERTER_FUNC)(DSO *, const char *);
	/*
	 * The function prototype used for method functions (or caller-provided
	 * callbacks) that merge two file specifications. They are passed a DSO
	 * structure pointer (or NULL if they are to be used independently of a DSO
	 * object) and two file specifications to merge. They should either return
	 * NULL (if there is an error condition) or a newly allocated string
	 * containing the result of merging that the caller will need to free with
	 * OPENSSL_free() when done. Here, merging means that bits and pieces are
	 * taken from each of the file specifications and added together in whatever
	 * fashion that is sensible for the DSO method in question.  The only rule
	 * that really applies is that if the two specification contain pieces of the
	 * same type, the copy from the first string takes priority.  One could see
	 * it as the first specification is the one given by the user and the second
	 * being a bunch of defaults to add on if they're missing in the first.
	 */
	/*@funcdef*/typedef char *(*DSO_MERGER_FUNC)(DSO *, const char *, const char *);

	DSO  * DSO_new(void);
	int    FASTCALL DSO_free(DSO *dso);
	int    DSO_flags(DSO *dso);
	int    DSO_up_ref(DSO *dso);
	long   DSO_ctrl(DSO *dso, int cmd, long larg, void *parg);
	/*
	 * These functions can be used to get/set the platform-independent filename
	 * used for a DSO. NB: set will fail if the DSO is already loaded.
	 */
	const char *DSO_get_filename(DSO *dso);
	int DSO_set_filename(DSO *dso, const char *filename);
	/*
	 * This function will invoke the DSO's name_converter callback to translate a
	 * filename, or if the callback isn't set it will instead use the DSO_METHOD's
	 * converter. If "filename" is NULL, the "filename" in the DSO itself will be
	 * used. If the DSO_FLAG_NO_NAME_TRANSLATION flag is set, then the filename is
	 * simply duplicated. NB: This function is usually called from within a
	 * DSO_METHOD during the processing of a DSO_load() call, and is exposed so
	 * that caller-created DSO_METHODs can do the same thing. A non-NULL return
	 * value will need to be OPENSSL_free()'d.
	 */
	char *DSO_convert_filename(DSO *dso, const char *filename);
	/*
	 * This function will invoke the DSO's merger callback to merge two file
	 * specifications, or if the callback isn't set it will instead use the
	 * DSO_METHOD's merger.  A non-NULL return value will need to be
	 * OPENSSL_free()'d.
	 */
	char *DSO_merge(DSO *dso, const char *filespec1, const char *filespec2);

	/*
	 * The all-singing all-dancing load function, you normally pass NULL for the
	 * first and third parameters. Use DSO_up_ref and DSO_free for subsequent
	 * reference count handling. Any flags passed in will be set in the
	 * constructed DSO after its init() function but before the load operation.
	 * If 'dso' is non-NULL, 'flags' is ignored.
	 */
	DSO *DSO_load(DSO *dso, const char *filename, DSO_METHOD *meth, int flags);

	/* This function binds to a function inside a shared library. */
	DSO_FUNC_TYPE DSO_bind_func(DSO *dso, const char *symname);

	/*
	 * This method is the default, but will beg, borrow, or steal whatever method
	 * should be the default on any particular platform (including
	 * DSO_METH_null() if necessary).
	 */
	DSO_METHOD *DSO_METHOD_openssl(void);

	/*
	 * This function writes null-terminated pathname of DSO module containing
	 * 'addr' into 'sz' large caller-provided 'path' and returns the number of
	 * characters [including trailing zero] written to it. If 'sz' is 0 or
	 * negative, 'path' is ignored and required amount of charachers [including
	 * trailing zero] to accommodate pathname is returned. If 'addr' is NULL, then
	 * pathname of cryptolib itself is returned. Negative or zero return value
	 * denotes error.
	 */
	int DSO_pathbyaddr(void *addr, char *path, int sz);

	/*
	 * Like DSO_pathbyaddr() but instead returns a handle to the DSO for the symbol
	 * or NULL on error.
	 */
	DSO *DSO_dsobyaddr(void *addr, int flags);

	/*
	 * This function should be used with caution! It looks up symbols in *all*
	 * loaded modules and if module gets unloaded by somebody else attempt to
	 * dereference the pointer is doomed to have fatal consequences. Primary
	 * usage for this function is to probe *core* system functionality, e.g.
	 * check if getnameinfo(3) is available at run-time without bothering about
	 * OS-specific details such as libc.so.versioning or where does it actually
	 * reside: in libc itself or libsocket.
	 */
	void *DSO_global_lookup(const char *name);

	/* BEGIN ERROR CODES */
	/*
	 * The following lines are auto generated by the script mkerr.pl. Any changes
	 * made after this point may be overwritten when the script is next run.
	 */

	int ERR_load_DSO_strings(void);

	/* Error codes for the DSO functions. */

	/* Function codes. */
	#define DSO_F_DLFCN_BIND_FUNC                            100
	#define DSO_F_DLFCN_LOAD                                 102
	#define DSO_F_DLFCN_MERGER                               130
	#define DSO_F_DLFCN_NAME_CONVERTER                       123
	#define DSO_F_DLFCN_UNLOAD                               103
	#define DSO_F_DL_BIND_FUNC                               104
	#define DSO_F_DL_LOAD                                    106
	#define DSO_F_DL_MERGER                                  131
	#define DSO_F_DL_NAME_CONVERTER                          124
	#define DSO_F_DL_UNLOAD                                  107
	#define DSO_F_DSO_BIND_FUNC                              108
	#define DSO_F_DSO_CONVERT_FILENAME                       126
	#define DSO_F_DSO_CTRL                                   110
	#define DSO_F_DSO_FREE                                   111
	#define DSO_F_DSO_GET_FILENAME                           127
	#define DSO_F_DSO_GLOBAL_LOOKUP                          139
	#define DSO_F_DSO_LOAD                                   112
	#define DSO_F_DSO_MERGE                                  132
	#define DSO_F_DSO_NEW_METHOD                             113
	#define DSO_F_DSO_PATHBYADDR                             105
	#define DSO_F_DSO_SET_FILENAME                           129
	#define DSO_F_DSO_UP_REF                                 114
	#define DSO_F_VMS_BIND_SYM                               115
	#define DSO_F_VMS_LOAD                                   116
	#define DSO_F_VMS_MERGER                                 133
	#define DSO_F_VMS_UNLOAD                                 117
	#define DSO_F_WIN32_BIND_FUNC                            101
	#define DSO_F_WIN32_GLOBALLOOKUP                         142
	#define DSO_F_WIN32_JOINER                               135
	#define DSO_F_WIN32_LOAD                                 120
	#define DSO_F_WIN32_MERGER                               134
	#define DSO_F_WIN32_NAME_CONVERTER                       125
	#define DSO_F_WIN32_PATHBYADDR                           109
	#define DSO_F_WIN32_SPLITTER                             136
	#define DSO_F_WIN32_UNLOAD                               121

	/* Reason codes. */
	#define DSO_R_CTRL_FAILED                                100
	#define DSO_R_DSO_ALREADY_LOADED                         110
	#define DSO_R_EMPTY_FILE_STRUCTURE                       113
	#define DSO_R_FAILURE                                    114
	#define DSO_R_FILENAME_TOO_BIG                           101
	#define DSO_R_FINISH_FAILED                              102
	#define DSO_R_INCORRECT_FILE_SYNTAX                      115
	#define DSO_R_LOAD_FAILED                                103
	#define DSO_R_NAME_TRANSLATION_FAILED                    109
	#define DSO_R_NO_FILENAME                                111
	#define DSO_R_NULL_HANDLE                                104
	#define DSO_R_SET_FILENAME_FAILED                        112
	#define DSO_R_STACK_ERROR                                105
	#define DSO_R_SYM_FAILURE                                106
	#define DSO_R_UNLOAD_FAILED                              107
	#define DSO_R_UNSUPPORTED                                108
#ifdef  __cplusplus
}
#endif
//
//#include <internal/chacha.h>
#ifdef __cplusplus
extern "C" {
#endif
	/*
	 * ChaCha20_ctr32 encrypts |len| bytes from |inp| with the given key and
	 * nonce and writes the result to |out|, which may be equal to |inp|.
	 * The |key| is not 32 bytes of verbatim key material though, but the
	 * said material collected into 8 32-bit elements array in host byte
	 * order. Same approach applies to nonce: the |counter| argument is
	 * pointer to concatenated nonce and counter values collected into 4
	 * 32-bit elements. This, passing crypto material collected into 32-bit
	 * elements as opposite to passing verbatim byte vectors, is chosen for
	 * efficiency in multi-call scenarios.
	 */
	void ChaCha20_ctr32(uchar *out, const uchar *inp, size_t len, const uint key[8], const uint counter[4]);
	/*
	 * You can notice that there is no key setup procedure. Because it's
	 * as trivial as collecting bytes into 32-bit elements, it's reckoned
	 * that below macro is sufficient.
	 */
	#define CHACHA_U8TOU32(p)  (((uint)(p)[0]) | ((uint)(p)[1]<<8) | ((uint)(p)[2]<<16) | ((uint)(p)[3]<<24))

	#define CHACHA_KEY_SIZE		32
	#define CHACHA_CTR_SIZE		16
	#define CHACHA_BLK_SIZE		64
#ifdef __cplusplus
}
#endif
//
#include <internal/numbers.h>
#include <internal/dane.h>
//#include <internal/o_str.h>
//#include <stddef.h> // to get size_t 
int OPENSSL_memcmp(const void *p1, const void *p2, size_t n);
//
//#include <internal/bn_int.h>
#ifdef  __cplusplus
extern "C" {
#endif
	BIGNUM * FASTCALL bn_wexpand(BIGNUM * a, int words);
	BIGNUM * FASTCALL bn_expand2(BIGNUM * a, int words);
	void FASTCALL bn_correct_top(BIGNUM * a);
	/*
	 * Determine the modified width-(w+1) Non-Adjacent Form (wNAF) of 'scalar'.
	 * This is an array r[] of values that are either zero or odd with an
	 * absolute value less than 2^w satisfying scalar = \sum_j r[j]*2^j where at
	 * most one of any w+1 consecutive digits is non-zero with the exception that
	 * the most significant digit may be only w-1 zeros away from that next
	 * non-zero digit.
	 */
	signed char *bn_compute_wNAF(const BIGNUM *scalar, int w, size_t *ret_len);
	int bn_get_top(const BIGNUM *a);
	void bn_set_top(BIGNUM *a, int top);
	int bn_get_dmax(const BIGNUM *a);
	/* Set all words to zero */
	void bn_set_all_zero(BIGNUM *a);
	/*
	 * Copy the internal BIGNUM words into out which holds size elements (and size
	 * must be bigger than top)
	 */
	int bn_copy_words(BN_ULONG *out, const BIGNUM *in, int size);
	BN_ULONG *bn_get_words(const BIGNUM *a);
	/*
	 * Set the internal data words in a to point to words which contains size
	 * elements. The BN_FLG_STATIC_DATA flag is set
	 */
	void bn_set_static_words(BIGNUM *a, BN_ULONG *words, int size);
	/*
	 * Copy words into the BIGNUM |a|, reallocating space as necessary.
	 * The negative flag of |a| is not modified.
	 * Returns 1 on success and 0 on failure.
	 */
	/*
	 * |num_words| is int because bn_expand2 takes an int. This is an internal
	 * function so we simply trust callers not to pass negative values.
	 */
	int bn_set_words(BIGNUM *a, BN_ULONG *words, int num_words);
	size_t bn_sizeof_BIGNUM(void);
	/*
	 * Return element el from an array of BIGNUMs starting at base (required
	 * because callers do not know the size of BIGNUM at compilation time)
	 */
	BIGNUM * bn_array_el(BIGNUM *base, int el);
#ifdef  __cplusplus
}
#endif
//
//#include <internal/poly1305.h>
//
#define POLY1305_BLOCK_SIZE 16

typedef struct poly1305_context POLY1305;

size_t Poly1305_ctx_size(void);
void Poly1305_Init(POLY1305 * ctx, const uchar key[32]);
void Poly1305_Update(POLY1305 * ctx, const uchar * inp, size_t len);
void Poly1305_Final(POLY1305 *ctx, uchar mac[16]);
//
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
void OPENSSL_showfatal(const char * fmta, ...);
extern int OPENSSL_NONPIC_relocated;
void crypto_cleanup_all_ex_data_int(void);
int openssl_strerror_r(int errnum, char * buf, size_t buflen);
#if !defined(OPENSSL_NO_STDIO)
FILE * openssl_fopen(const char * filename, const char * mode);
#else
void * openssl_fopen(const char * filename, const char * mode);
#endif

#ifdef  __cplusplus
}
#endif
//
// ENG_INT.H
//
#ifdef  __cplusplus
extern "C" {
#endif

extern CRYPTO_RWLOCK * global_engine_lock;
/*
 * If we compile with this symbol defined, then both reference counts in the
 * ENGINE structure will be monitored with a line of output on stderr for
 * each change. This prints the engine's pointer address (truncated to
 * uint), "struct" or "funct" to indicate the reference type, the
 * before and after reference count, and the file:line-number pair. The
 * "engine_ref_debug" statements must come *after* the change.
 */
#ifdef ENGINE_REF_COUNT_DEBUG
	#define engine_ref_debug(e, isfunct, diff) fprintf(stderr, "engine: %08x %s from %d to %d (%s:%d)\n", \
	    (uint)(e), (isfunct ? "funct" : "struct"), ((isfunct) ? ((e)->funct_ref - (diff)) : ((e)->struct_ref - (diff))), \
	    ((isfunct) ? (e)->funct_ref : (e)->struct_ref), (OPENSSL_FILE), (OPENSSL_LINE))
#else
	#define engine_ref_debug(e, isfunct, diff)
#endif
/*
 * Any code that will need cleanup operations should use these functions to
 * register callbacks. engine_cleanup_int() will call all registered
 * callbacks in order. NB: both the "add" functions assume the engine lock to
 * already be held (in "write" mode).
 */
typedef void (ENGINE_CLEANUP_CB)(void);
typedef struct st_engine_cleanup_item {
	ENGINE_CLEANUP_CB * cb;
} ENGINE_CLEANUP_ITEM;
DEFINE_STACK_OF(ENGINE_CLEANUP_ITEM)
void engine_cleanup_add_first(ENGINE_CLEANUP_CB * cb);
void engine_cleanup_add_last(ENGINE_CLEANUP_CB * cb);

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
int engine_table_register(ENGINE_TABLE ** table, ENGINE_CLEANUP_CB * cleanup, ENGINE * e, const int * nids, int num_nids, int setdefault);
void engine_table_unregister(ENGINE_TABLE ** table, ENGINE * e);
void engine_table_cleanup(ENGINE_TABLE ** table);
#ifndef ENGINE_TABLE_DEBUG
	/*@funcdef*/ENGINE * engine_table_select(ENGINE_TABLE ** table, int nid);
#else
	ENGINE * engine_table_select_tmp(ENGINE_TABLE ** table, int nid, const char * f, int l);
	#define engine_table_select(t, n) engine_table_select_tmp(t, n, OPENSSL_FILE, OPENSSL_LINE)
#endif
/*@funcdef*/typedef void (engine_table_doall_cb)(int nid, STACK_OF(ENGINE) * sk, ENGINE * def, void * arg);
void engine_table_doall(ENGINE_TABLE * table, engine_table_doall_cb * cb, void * arg);
/*
 * Internal versions of API functions that have control over locking. These
 * are used between C files when functionality needs to be shared but the
 * caller may already be controlling of the engine lock.
 */
int engine_unlocked_init(ENGINE * e);
int engine_unlocked_finish(ENGINE * e, int unlock_for_handlers);
int engine_free_util(ENGINE * e, int locked);
/*
 * This function will reset all "set"able values in an ENGINE to NULL. This
 * won't touch reference counts or ex_data, but is equivalent to calling all
 * the ENGINE_set_***() functions with a NULL value.
 */
void engine_set_all_null(ENGINE * e);
/*
 * NB: Bitwise OR-able values for the "flags" variable in ENGINE are now
 * exposed in engine.h.
 */
/* Free up dynamically allocated public key methods associated with ENGINE */
void engine_pkey_meths_free(ENGINE * e);
void engine_pkey_asn1_meths_free(ENGINE * e);
/* Once initialisation function */
extern CRYPTO_ONCE engine_lock_init;
DECLARE_RUN_ONCE(do_engine_lock_init)
/*
 * This is a structure for storing implementations of various crypto algorithms and functions.
 */
struct engine_st {
	const char * id;
	const char * name;
	const RSA_METHOD * rsa_meth;
	const DSA_METHOD * dsa_meth;
	const DH_METHOD * dh_meth;
	const EC_KEY_METHOD * ec_meth;
	const RAND_METHOD * rand_meth;
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
	const ENGINE_CMD_DEFN * cmd_defns;
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
	struct engine_st * prev;
	struct engine_st * next;
};

typedef struct st_engine_pile ENGINE_PILE;

DEFINE_LHASH_OF(ENGINE_PILE);

#ifdef  __cplusplus
}
#endif
//
// } ENG_INT.H
//
#include "evp_locl.h"
#include "blake2_locl.h"
#include "ocsp_lcl.h"
#include "comp_lcl.h"
#include "dso_locl.h"
#include "p12_lcl.h"
#include "pcy_int.h"
#include "ts_lcl.h"
#include "ext_dat.h"
#include "x509_lcl.h"
#include "asn1_locl.h"
#endif
