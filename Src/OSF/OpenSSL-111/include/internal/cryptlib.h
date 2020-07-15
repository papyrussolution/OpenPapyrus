/*
 * Copyright 1995-2019 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the OpenSSL license (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */

#ifndef HEADER_CRYPTLIB_H
#define HEADER_CRYPTLIB_H

#include <slib.h>

// @v10.7.12 #define OPENSSL_NO_DYNAMIC_ENGINE // @sobolev
#define OPENSSL_NO_HW_PADLOCK // @sobolev @v10.7.12
#define OPENSSL_NO_CAPIENG    // @sobolev @v10.7.12

//#include <bio_lcl.h>
#include "e_os.h"
#include "internal/sockets.h"
#include "internal/refcount.h"

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

//#include "internal/cryptlib.h"
#include <openssl/crypto.h>
#include <cryptlib_int.h>
#include <internal/bio.h>
#ifdef OPENSSL_USE_APPLINK
	#undef BIO_FLAGS_UPLINK
	#define BIO_FLAGS_UPLINK 0x8000
	#include "ms/uplink.h"
#endif
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/err.h>
#include <openssl/kdf.h>
#include <openssl/evp.h>
#include <openssl/cmac.h>
#include <openssl/hmac.h>
#include <openssl/safestack.h>
#include <openssl/asn1.h>
#include <openssl/asn1t.h>
#include <openssl/tls1.h>
#include <openssl/x509.h>
#include <openssl/engine.h>
#include <openssl/ec.h>
#include <openssl/cms.h>
#include <openssl/ocsp.h>
#include <openssl/pkcs12.h>
#include <openssl/srp.h>
#include <openssl/txt_db.h>
#include <openssl/aes.h>
#include <openssl/modes.h>
#include <openssl/ct.h>
#include <openssl/asyncerr.h>
#include "internal/nelem.h"
#include <asn1_int.h>
#include <evp_int.h>
#include <ssl-crypto-ctype.h>
#include <aes_locl.h>
#include <asn1_locl.h>
#include "ext_dat.h"

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
	/* bio, mode, argp, argi, argl, ret */
	BIO_callback_fn callback;
	BIO_callback_fn_ex callback_ex;
	char * cb_arg;          /* first argument for the callback */
	int init;
	int shutdown;
	int flags;              /* extra storage */
	int retry_reason;
	int num;
	void * ptr;
	struct bio_st * next_bio; /* used by filter BIOs */
	struct bio_st * prev_bio; /* used by filter BIOs */
	CRYPTO_REF_COUNT references;
	uint64_t num_read;
	uint64_t num_write;
	CRYPTO_EX_DATA ex_data;
	CRYPTO_RWLOCK * lock;
};

#ifndef OPENSSL_NO_SOCK
	#ifdef OPENSSL_SYS_VMS
		typedef unsigned int socklen_t;
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
	#define UP_fgets        fgets
	#define UP_fread        fread
	#define UP_fwrite       fwrite
	#undef  UP_fsetmod
	#define UP_feof         feof
	#define UP_fclose       fclose

	#define UP_fopen        fopen
	#define UP_fseek        fseek
	#define UP_ftell        ftell
	#define UP_fflush       fflush
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
#ifdef NDEBUG
	#define ossl_assert(x) ((x) != 0)
#else
	__owur static ossl_inline int ossl_assert_int(int expr, const char * exprstr, const char * file, int line)
	{
		if(!expr)
			OPENSSL_die(exprstr, file, line);
		return expr;
	}

	#define ossl_assert(x) ossl_assert_int((x) != 0, "Assertion failed: "#x, __FILE__, __LINE__)
#endif

typedef struct ex_callback_st EX_CALLBACK;

DEFINE_STACK_OF(EX_CALLBACK)

typedef struct app_mem_info_st APP_INFO;
typedef struct mem_st MEM;
DEFINE_LHASH_OF(MEM);

#define OPENSSL_CONF             "openssl.cnf"
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

/* size of string representations */
#define DECIMAL_SIZE(type)      ((sizeof(type)*8+2)/3+1)
#define HEX_SIZE(type)          (sizeof(type)*2)

void OPENSSL_cpuid_setup(void);
extern unsigned int OPENSSL_ia32cap_P[];
void OPENSSL_showfatal(const char * fmta, ...);
void crypto_cleanup_all_ex_data_int(void);
int openssl_init_fork_handlers(void);
int openssl_get_fork_id(void);
char * ossl_safe_getenv(const char * name);
extern CRYPTO_RWLOCK * memdbg_lock;
int openssl_strerror_r(int errnum, char * buf, size_t buflen);
#if !defined(OPENSSL_NO_STDIO)
	FILE * openssl_fopen(const char * filename, const char * mode);
#else
	void * openssl_fopen(const char * filename, const char * mode);
#endif

uint32_t OPENSSL_rdtsc(void);
size_t OPENSSL_instrument_bus(unsigned int *, size_t);
size_t OPENSSL_instrument_bus2(unsigned int *, size_t, size_t);

#endif
