// LIBSSH-INTERNAL.H
//
#ifndef __LIBSSH_INTERNAL_H
#define __LIBSSH_INTERNAL_H
#define LIBSSH_STATIC
#include <slib.h>
//typedef SOCKET socket_t_xx;
#include "libssh_config.h"
#include "libssh.h"
#include <stdbool.h>

#ifndef HAVE_STRTOULL
	#if defined(HAVE___STRTOULL)
		#define strtoull __strtoull
	#elif defined(HAVE__STRTOUI64)
		#define strtoull _strtoui64
	#elif defined(__hpux) && defined(__LP64__)
		#define strtoull strtoul
	#else
		#error "no strtoull function found"
	#endif
#endif
#if !defined(HAVE_STRNDUP)
	char * strndup(const char * s, size_t n);
#endif /* ! HAVE_STRNDUP */
#ifdef HAVE_BYTESWAP_H
	#include <byteswap.h>
#endif
#ifdef HAVE_ARPA_INET_H
	#include <arpa/inet.h>
#endif
#ifndef bswap_32
	#define bswap_32(x) ((((x) & 0xff000000) >> 24) | (((x) & 0x00ff0000) >>  8) | (((x) & 0x0000ff00) <<  8) | (((x) & 0x000000ff) << 24))
#endif
#ifdef _WIN32
// Imitate define of inttypes.h 
#ifndef PRIdS
	#define PRIdS "Id"
#endif
#ifndef PRIu64
	#if __WORDSIZE == 64
		#define PRIu64 "lu"
	#else
		#define PRIu64 "llu"
	#endif /* __WORDSIZE */
#endif /* PRIu64 */
#ifndef PRIu32
	#define PRIu32 "u"
#endif /* PRIu32 */
#ifndef PRIx64
	#if __WORDSIZE == 64
		#define PRIx64 "lx"
	#else
		#define PRIx64 "llx"
	#endif /* __WORDSIZE */
#endif /* PRIx64 */
#ifndef PRIx32
	#define PRIx32 "x"
#endif /* PRIx32 */
#ifdef _MSC_VER

/* On Microsoft compilers define inline to __inline on all others use inline */
#undef inline
#define inline __inline
#ifndef va_copy
	#define va_copy(dest, src) (dest = src)
#endif
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#if !defined(HAVE_ISBLANK)
	#define isblank(ch) ((ch) == ' ' || (ch) == '\t' || (ch) == '\n' || (ch) == '\r')
#endif
#define usleep(X) Sleep(((X)+1000)/1000)
#undef strtok_r
#define strtok_r strtok_s
#if defined(HAVE__SNPRINTF_S)
#undef snprintf
#define snprintf(d, n, ...) _snprintf_s((d), (n), _TRUNCATE, __VA_ARGS__)
#else /* HAVE__SNPRINTF_S */
#if defined(HAVE__SNPRINTF)
#     undef snprintf
#     define snprintf _snprintf
#else /* HAVE__SNPRINTF */
#if !defined(HAVE_SNPRINTF)
#error "no snprintf compatible function found"
#endif /* HAVE_SNPRINTF */
#endif /* HAVE__SNPRINTF */
#endif /* HAVE__SNPRINTF_S */
#if defined(HAVE__VSNPRINTF_S)
	#undef vsnprintf
	#define vsnprintf(s, n, f, v) _vsnprintf_s((s), (n), _TRUNCATE, (f), (v))
#else /* HAVE__VSNPRINTF_S */
	#if defined(HAVE__VSNPRINTF)
		#undef vsnprintf
		#define vsnprintf _vsnprintf
	#else
		#if !defined(HAVE_VSNPRINTF)
			#error "No vsnprintf compatible function found"
		#endif /* HAVE_VSNPRINTF */
	#endif /* HAVE__VSNPRINTF */
#endif /* HAVE__VSNPRINTF_S */

#ifndef _SSIZE_T_DEFINED
	#undef ssize_t
	#include <BaseTsd.h>
	typedef _W64 SSIZE_T ssize_t;
	#define _SSIZE_T_DEFINED
#endif /* _SSIZE_T_DEFINED */

#endif /* _MSC_VER */

struct timeval;

int gettimeofday(struct timeval * __p, void * __t);

#define _XCLOSESOCKET closesocket

#else /* _WIN32 */

#include <unistd.h>
#define PRIdS "zd"

#define _XCLOSESOCKET close

#endif /* _WIN32 */

#include "ssh-string.h"
#include "callbacks.h"

/* some constants */
#ifndef MAX_PACKAT_LEN
	#define MAX_PACKET_LEN 262144
#endif
#ifndef ERROR_BUFFERLEN
	#define ERROR_BUFFERLEN 1024
#endif
#ifndef CLIENT_BANNER_SSH2
	#define CLIENT_BANNER_SSH2 "SSH-2.0-libssh_" SSH_STRINGIFY(LIBSSH_VERSION)
#endif /* CLIENT_BANNER_SSH2 */
#ifndef KBDINT_MAX_PROMPT
	#define KBDINT_MAX_PROMPT 256 /* more than openssh's :) */
#endif
#ifndef MAX_BUF_SIZE
	#define MAX_BUF_SIZE 4096
#endif
#ifndef HAVE_COMPILER__FUNC__
	#ifdef HAVE_COMPILER__FUNCTION__
		#define __func__ __FUNCTION__
	#else
		#error "Your system must provide a __func__ macro"
	#endif
#endif
#if defined(HAVE_GCC_THREAD_LOCAL_STORAGE)
	#define LIBSSH_THREAD __thread
#elif defined(HAVE_MSC_THREAD_LOCAL_STORAGE)
	#define LIBSSH_THREAD __declspec(thread)
#else
	#define LIBSSH_THREAD
#endif
/*
 * This makes sure that the compiler doesn't optimize out the code
 *
 * Use it in a macro where the provided variable is 'x'.
 */
#if defined(HAVE_GCC_VOLATILE_MEMORY_PROTECTION)
	#define LIBSSH_MEM_PROTECTION __asm__ volatile ("" : : "r" (&(x)) : "memory")
#else
	#define LIBSSH_MEM_PROTECTION
#endif

/* forward declarations */
struct ssh_common_struct;
struct ssh_kex_struct;

enum ssh_digest_e {
	SSH_DIGEST_AUTO = 0,
	SSH_DIGEST_SHA1 = 1,
	SSH_DIGEST_SHA256,
	SSH_DIGEST_SHA384,
	SSH_DIGEST_SHA512,
};

int ssh_get_key_params(ssh_session session, ssh_key * privkey, enum ssh_digest_e * digest);

/* LOGGING */
void ssh_log_function(int verbosity, const char * function, const char * buffer);
#define SSH_LOG(priority, ...) _ssh_log(priority, __func__, __VA_ARGS__)

/* LEGACY */
void ssh_log_common(struct ssh_common_struct * common, int verbosity, const char * function, const char * format, ...) PRINTF_ATTRIBUTE(4, 5);

/* ERROR HANDLING */

/* error handling structure */
struct error_struct {
	int error_code;
	char error_buffer[ERROR_BUFFERLEN];
};

#define ssh_set_error(error, code, ...) _ssh_set_error(error, code, __func__, __VA_ARGS__)
void _ssh_set_error(void * error, int code, const char * function, const char * descr, ...) PRINTF_ATTRIBUTE(4, 5);
#define ssh_set_error_oom(error) _ssh_set_error_oom(error, __func__)
void _ssh_set_error_oom(void * error, const char * function);
#define ssh_set_error_invalid(error) _ssh_set_error_invalid(error, __func__)
void _ssh_set_error_invalid(void * error, const char * function);
void ssh_reset_error(void * error);

/* server.c */
#ifdef WITH_SERVER
	int ssh_auth_reply_default(ssh_session session, int partial);
	int ssh_auth_reply_success(ssh_session session, int partial);
#endif
/* client.c */

int ssh_send_banner(ssh_session session, int is_server);
/* connect.c */
socket_t ssh_connect_host_nonblocking(ssh_session session, const char * host, const char * bind_addr, int port);
/* in base64.c */
ssh_buffer base64_to_bin(const char * source);
uint8 * bin_to_base64(const uint8 * source, size_t len);
/* gzip.c */
int compress_buffer(ssh_session session, ssh_buffer buf);
int decompress_buffer(ssh_session session, ssh_buffer buf, size_t maxlen);
/* match.c */
int match_pattern_list(const char * string, const char * pattern, uint len, int dolower);
int match_hostname(const char * host, const char * pattern, uint len);
/* connector.c */
int ssh_connector_set_event(ssh_connector connector, ssh_event event);
int ssh_connector_remove_event(ssh_connector connector);

//#ifndef MIN
	//#define MIN(a, b) ((a) < (b) ? (a) : (b))
//#endif
//#ifndef MAX
	//#define MAX(a, b) ((a) > (b) ? (a) : (b))
//#endif

/** Free memory space */
#define SAFE_FREE(x) do { if((x) != NULL) { SAlloc::F(x); x = NULL;} } while(0)

#define ZERO_STRUCT(x) memzero((char*)&(x), sizeof(x)) /** Zero a structure */
/** Zero a structure given a pointer to the structure */
#define ZERO_STRUCTP(x) do { if((x) != NULL) memzero((char*)(x), sizeof(*(x))); } while(0)
//#define ARRAY_SIZE_Removed(a) (sizeof(a)/sizeof(a[0])) /** Get the size of an array */
#ifndef HAVE_EXPLICIT_BZERO
void memzero(void * s, size_t n);
#endif /* !HAVE_EXPLICIT_BZERO */

/**
 * This is a hack to fix warnings. The idea is to use this everywhere that we
 * get the "discarding const" warning by the compiler. That doesn't actually
 * fix the real issue, but marks the place and you can search the code for
 * discard_const.
 *
 * Please use this macro only when there is no other way to fix the warning.
 * We should use this function in only in a very few places.
 *
 * Also, please call this via the discard_const_p() macro interface, as that
 * makes the return type safe.
 */
#define discard_const(ptr) ((void *)((uintptr_t)(ptr)))

/**
 * Type-safe version of discard_const
 */
#define discard_const_p(type, ptr) ((type*)discard_const(ptr))

/**
 * Get the argument cound of variadic arguments
 */
/*
 * Since MSVC 2010 there is a bug in passing __VA_ARGS__ to subsequent
 * macros as a single token, which results in:
 *    warning C4003: not enough actual parameters for macro '_VA_ARG_N'
 *  and incorrect behavior. This fixes issue.
 */
#define VA_APPLY_VARIADIC_MACRO(macro, tuple) macro tuple

#define __VA_NARG__(...) (__VA_NARG_(__VA_ARGS__, __RSEQ_N()))
#define __VA_NARG_(...) VA_APPLY_VARIADIC_MACRO(__VA_ARG_N, (__VA_ARGS__))
#define __VA_ARG_N(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, \
		_21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, \
		_41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, \
		_61, _62, _63, N, ...) N
#define __RSEQ_N() \
	63, 62, 61, 60,                         \
	59, 58, 57, 56, 55, 54, 53, 52, 51, 50, \
	49, 48, 47, 46, 45, 44, 43, 42, 41, 40, \
	39, 38, 37, 36, 35, 34, 33, 32, 31, 30, \
	29, 28, 27, 26, 25, 24, 23, 22, 21, 20, \
	19, 18, 17, 16, 15, 14, 13, 12, 11, 10, \
	9,  8,  7,  6,  5,  4,  3,  2,  1,  0

#define CLOSE_SOCKET(s) do { if((s) != SSH_INVALID_SOCKET) { _XCLOSESOCKET(s); (s) = SSH_INVALID_SOCKET;} } while(0)

#ifndef HAVE_HTONLL
	#ifdef WORDS_BIGENDIAN
		#define htonll(x) (x)
	#else
		#define htonll(x) (((uint64_t)htonl((x) & 0xFFFFFFFF) << 32) | htonl((x) >> 32))
	#endif
#endif
#ifndef HAVE_NTOHLL
	#ifdef WORDS_BIGENDIAN
		#define ntohll(x) (x)
	#else
		#define ntohll(x) (((uint64_t)ntohl((x) & 0xFFFFFFFF) << 32) | ntohl((x) >> 32))
	#endif
#endif
#ifndef FALL_THROUGH
	#ifdef HAVE_FALLTHROUGH_ATTRIBUTE
		#define FALL_THROUGH __attribute__ ((fallthrough))
	#else /* HAVE_FALLTHROUGH_ATTRIBUTE */
		#define FALL_THROUGH
	#endif /* HAVE_FALLTHROUGH_ATTRIBUTE */
#endif /* FALL_THROUGH */
#ifndef __attr_unused__
	#ifdef HAVE_UNUSED_ATTRIBUTE
		#define __attr_unused__ __attribute__((unused))
	#else /* HAVE_UNUSED_ATTRIBUTE */
		#define __attr_unused__
	#endif /* HAVE_UNUSED_ATTRIBUTE */
#endif /* __attr_unused__ */
#ifndef UNUSED_PARAM
	//#define UNUSED_PARAM(param) param __attr_unused__
	#define UNUSED_PARAM(param) param
#endif /* UNUSED_PARAM */
#ifndef UNUSED_VAR
	#define UNUSED_VAR(var) __attr_unused__ var
#endif /* UNUSED_VAR */

void ssh_agent_state_free(void * data);
bool is_ssh_initialized();

#include "config.h"
#include "libcrypto.h"
#include "libgcrypt.h"
#include "libmbedcrypto.h"
#include "wrapper.h"
#include "callbacks.h"
#include "kex.h"
#include "packet.h"
#include "pcap.h"
#include "auth.h"
#include "channels.h"
#include "poll.h"
//#include "misc.h"
	// in misc.c 
	// gets the user home dir. 
	char * ssh_get_user_home_dir();
	char * ssh_get_local_username();
	int ssh_file_readaccess_ok(const char * file);
	int ssh_dir_writeable(const char * path);
	char * ssh_path_expand_tilde(const char * d);
	char * ssh_path_expand_escape(ssh_session session, const char * s);
	int ssh_analyze_banner(ssh_session session, int server);
	int ssh_is_ipaddr_v4(const char * str);
	int ssh_is_ipaddr(const char * str);
	//
	// list processing
	//
	struct ssh_list {
		struct ssh_iterator * root;
		struct ssh_iterator * end;
	};

	struct ssh_iterator {
		struct ssh_iterator * next;
		const void * data;
	};

	struct ssh_timestamp {
		long seconds;
		long useconds;
	};

	enum ssh_quote_state_e {
		NO_QUOTE,
		SINGLE_QUOTE,
		DOUBLE_QUOTE
	};

	struct ssh_list * ssh_list_new();
	void FASTCALL ssh_list_free(struct ssh_list * list);
	struct ssh_iterator * FASTCALL ssh_list_get_iterator(const struct ssh_list * list);
	struct ssh_iterator * ssh_list_find(const struct ssh_list * list, void * value);
	size_t ssh_list_count(const struct ssh_list * list);
	int ssh_list_append(struct ssh_list * list, const void * data);
	int ssh_list_prepend(struct ssh_list * list, const void * data);
	void ssh_list_remove(struct ssh_list * list, struct ssh_iterator * iterator);
	char * ssh_lowercase(const char* str);
	char * ssh_hostport(const char * host, int port);
	const void * _ssh_list_pop_head(struct ssh_list * list);

	#define ssh_iterator_value(type, iterator) ((type)((iterator)->data))

	/** @brief fetch the head element of a list and remove it from list
	 * @param type type of the element to return
	 * @param list the ssh_list to use
	 * @return the first element of the list, or NULL if the list is empty
	 */
	#define ssh_list_pop_head(type, ssh_list) ((type)_ssh_list_pop_head(ssh_list))

	int ssh_make_milliseconds(long sec, long usec);
	void ssh_timestamp_init(struct ssh_timestamp * ts);
	int ssh_timeout_elapsed(struct ssh_timestamp * ts, int timeout);
	int ssh_timeout_update(struct ssh_timestamp * ts, int timeout);
	int ssh_match_group(const char * group, const char * object);
	void uint64_inc(uchar * counter);
	void ssh_log_hexdump(const char * descr, const uchar * what, size_t len);
	int ssh_mkdirs(const char * pathname, mode_t mode);
	int ssh_quote_file_name(const char * file_name, char * buf, size_t buf_len);
	int ssh_newline_vis(const char * string, char * buf, size_t buf_len);
//
#include "buffer.h"
#include "fe25519.h"
#include "sc25519.h"
#include "ge25519.h"
#include "ed25519.h"
#include "pki.h"
#include "pki_priv.h"
//#include "bignum.h"
	bignum ssh_make_string_bn(ssh_string string);
	ssh_string ssh_make_bignum_string(bignum num);
	void ssh_print_bignum(const char * which, const_bignum num);
//
#include "session.h"
#include "server.h"
//#include "bind.h"
	struct ssh_bind_struct {
		struct ssh_common_struct common; /* stuff common to ssh_bind and ssh_session */
		struct ssh_bind_callbacks_struct * bind_callbacks;
		void * bind_callbacks_userdata;

		struct ssh_poll_handle_struct * poll;
		// options 
		char * wanted_methods[SSH_KEX_METHODS];
		char * banner;
		char * ecdsakey;
		char * dsakey;
		char * rsakey;
		char * ed25519key;
		ssh_key ecdsa;
		ssh_key dsa;
		ssh_key rsa;
		ssh_key ed25519;
		char * bindaddr;
		socket_t bindfd;
		uint bindport;
		int blocking;
		int toaccept;
		bool config_processed;
		char * config_dir;
		char * pubkey_accepted_key_types;
	};

	struct ssh_poll_handle_struct * ssh_bind_get_poll(struct ssh_bind_struct * sshbind);
//#include "bind_config.h"
enum ssh_bind_config_opcode_e {
	BIND_CFG_NOT_ALLOWED_IN_MATCH = -4, /* Known but not allowed in Match block */
	BIND_CFG_UNKNOWN = -3, /* Unknown opcode */
	BIND_CFG_NA = -2, /* Known and not applicable to libssh */
	BIND_CFG_UNSUPPORTED = -1, /* Known but not supported by current libssh version */
	BIND_CFG_INCLUDE,
	BIND_CFG_HOSTKEY,
	BIND_CFG_LISTENADDRESS,
	BIND_CFG_PORT,
	BIND_CFG_LOGLEVEL,
	BIND_CFG_CIPHERS,
	BIND_CFG_MACS,
	BIND_CFG_KEXALGORITHMS,
	BIND_CFG_MATCH,
	BIND_CFG_PUBKEY_ACCEPTED_KEY_TYPES,
	BIND_CFG_HOSTKEY_ALGORITHMS,
	BIND_CFG_MAX /* Keep this one last in the list */
};
// 
// @brief Parse configuration file and set the options to the given ssh_bind
// 
// @params[in] sshbind   The ssh_bind context to be configured
// @params[in] filename  The path to the configuration file
// @returns    0 on successful parsing the configuration file, -1 on error
// 
int ssh_bind_config_parse_file(ssh_bind sshbind, const char * filename);
//
#include "socket.h"
//#include "token.h"
	//
	// Tokens list handling
	//
	struct ssh_tokens_st {
		char * buffer;
		char ** tokens;
	};

	struct ssh_tokens_st * ssh_tokenize(const char * chain, char separator);
	void ssh_tokens_free(struct ssh_tokens_st * tokens);
	char * ssh_find_matching(const char * available_d, const char * preferred_d);
	char * ssh_find_all_matching(const char * available_d, const char * preferred_d);
	char * ssh_remove_duplicates(const char * list);
	char * ssh_append_without_duplicates(const char * list, const char * appended_list);
//
#include "config_parser.h"
//#include "options.h"
	int ssh_config_parse_file(ssh_session session, const char *filename);
	int ssh_options_set_algo(ssh_session session, enum ssh_kex_types_e algo, const char *list);
	int ssh_options_apply(ssh_session session);
//
#include "blf.h"
#include "chacha.h"
//#include "dh.h"
	struct dh_ctx;

	#define DH_CLIENT_KEYPAIR 0
	#define DH_SERVER_KEYPAIR 1

	/* functions implemented by crypto backends */
	int ssh_dh_init_common(struct ssh_crypto_struct * crypto);
	void ssh_dh_cleanup(struct ssh_crypto_struct * crypto);
	int ssh_dh_get_parameters(struct dh_ctx * ctx, const_bignum * modulus, const_bignum * generator);
	int ssh_dh_set_parameters(struct dh_ctx * ctx, const bignum modulus, const bignum generator);
	int ssh_dh_keypair_gen_keys(struct dh_ctx * ctx, int peer);
	int ssh_dh_keypair_get_keys(struct dh_ctx * ctx, int peer, const_bignum * priv, const_bignum * pub);
	int ssh_dh_keypair_set_keys(struct dh_ctx * ctx, int peer, const bignum priv, const bignum pub);
	int ssh_dh_compute_shared_secret(struct dh_ctx * ctx, int local, int remote, bignum * dest);

	void ssh_dh_debug_crypto(struct ssh_crypto_struct * c);

	/* common functions */
	int ssh_dh_init();
	void ssh_dh_finalize();

	int ssh_dh_import_next_pubkey_blob(ssh_session session, ssh_string pubkey_blob);
	ssh_key ssh_dh_get_current_server_publickey(ssh_session session);
	int ssh_dh_get_current_server_publickey_blob(ssh_session session, ssh_string * pubkey_blob);
	ssh_key ssh_dh_get_next_server_publickey(ssh_session session);
	int ssh_dh_get_next_server_publickey_blob(ssh_session session, ssh_string * pubkey_blob);
	int ssh_client_dh_init(ssh_session session);
	#ifdef WITH_SERVER
		void ssh_server_dh_init(ssh_session session);
	#endif /* WITH_SERVER */
	int ssh_server_dh_process_init(ssh_session session, ssh_buffer packet);
	int ssh_fallback_group(uint32_t pmax, bignum * p, bignum * g);
	bool ssh_dh_is_known_group(bignum modulus, bignum generator);
//
//#include "ecdh.h"
	#ifdef HAVE_LIBCRYPTO
		#ifdef HAVE_OPENSSL_ECDH_H
			#ifdef HAVE_ECC
				#define HAVE_ECDH 1
			#endif
		#endif
	#endif
	#ifdef HAVE_GCRYPT_ECC
		#define HAVE_ECDH 1
	#endif
	#ifdef HAVE_LIBMBEDCRYPTO
		#define HAVE_ECDH 1
	#endif

	extern struct ssh_packet_callbacks_struct ssh_ecdh_client_callbacks;
	/* Backend-specific functions.  */
	int ssh_client_ecdh_init(ssh_session session);
	int ecdh_build_k(ssh_session session);
	#ifdef WITH_SERVER
		extern struct ssh_packet_callbacks_struct ssh_ecdh_server_callbacks;
		void ssh_server_ecdh_init(ssh_session session);
		SSH_PACKET_CALLBACK(ssh_packet_server_ecdh_init);
	#endif
//
#include "curve25519.h"
#include "crypto.h"
//#include "poly1305.h"
	//
	// Public Domain poly1305 from Andrew Moon
	// poly1305-donna-unrolled.c from https://github.com/floodyberry/poly1305-donna
	//
	#define POLY1305_KEYLEN    32
	#define POLY1305_TAGLEN    16

	void poly1305_auth(uint8 out[POLY1305_TAGLEN], const uint8 *m, size_t inlen, const uint8 key[POLY1305_KEYLEN])
	#ifdef HAVE_GCC_BOUNDED_ATTRIBUTE
		__attribute__((__bounded__(__minbytes__, 1, POLY1305_TAGLEN)))
		__attribute__((__bounded__(__buffer__, 2, 3)))
		__attribute__((__bounded__(__minbytes__, 4, POLY1305_KEYLEN)))
	#endif
		;
//
#include "ssh2.h"
#include "messages.h"
//#include "dh-gex.h"
	int ssh_client_dhgex_init(ssh_session session);
	#ifdef WITH_SERVER
		void ssh_server_dhgex_init(ssh_session session);
	#endif
//
#include "threads.h"
#include "keys.h"
//#include "knownhosts.h"
	struct ssh_list * ssh_known_hosts_get_algorithms(ssh_session session);
	char * ssh_known_hosts_get_algorithms_names(ssh_session session);
	enum ssh_known_hosts_e ssh_session_get_known_hosts_entry_file(ssh_session session, const char * filename, struct ssh_knownhosts_entry ** pentry);
//
#include "gssapi.h"
//#include "bytearray.h"
	#define _DATA_BYTE_CONST(data, pos) ((uint8)(((const uint8 *)(data))[(pos)]))
	#define _DATA_BYTE(data, pos)       (((uint8 *)(data))[(pos)])
	/*
	 * These macros pull or push integer values from byte arrays stored in
	 * little-endian byte order.
	 */
	#define PULL_LE_U8(data, pos)  (_DATA_BYTE_CONST(data, pos))
	#define PULL_LE_U16(data, pos) ((uint16_t)PULL_LE_U8(data, pos) | ((uint16_t)(PULL_LE_U8(data, (pos) + 1))) << 8)
	#define PULL_LE_U32(data, pos) ((uint32_t)(PULL_LE_U16(data, pos) | ((uint32_t)PULL_LE_U16(data, (pos) + 2)) << 16))
	#define PULL_LE_U64(data, pos) ((uint64_t)(PULL_LE_U32(data, pos) | ((uint64_t)PULL_LE_U32(data, (pos) + 4)) << 32))
	#define PUSH_LE_U8(data, pos, val) (_DATA_BYTE(data, pos) = ((uint8)(val)))
	#define PUSH_LE_U16(data, pos, val) (PUSH_LE_U8((data), (pos), (uint8)((uint16_t)(val) & 0xff)), PUSH_LE_U8((data), (pos) + 1, (uint8)((uint16_t)(val) >> 8)))
	#define PUSH_LE_U32(data, pos, val) (PUSH_LE_U16((data), (pos), (uint16_t)((uint32_t)(val) & 0xffff)), PUSH_LE_U16((data), (pos) + 2, (uint16_t)((uint32_t)(val) >> 16)))
	#define PUSH_LE_U64(data, pos, val) (PUSH_LE_U32((data), (pos), (uint32_t)((uint64_t)(val) & 0xffffffff)), PUSH_LE_U32((data), (pos) + 4, (uint32_t)((uint64_t)(val) >> 32)))
	/*
	 * These macros pull or push integer values from byte arrays stored in
	 * big-endian byte order (network byte order).
	 */
	#define PULL_BE_U8(data, pos) (_DATA_BYTE_CONST(data, pos))
	#define PULL_BE_U16(data, pos) ((((uint16_t)(PULL_BE_U8(data, pos))) << 8) | (uint16_t)PULL_BE_U8(data, (pos) + 1))
	#define PULL_BE_U32(data, pos) ((((uint32_t)PULL_BE_U16(data, pos)) << 16) | (uint32_t)(PULL_BE_U16(data, (pos) + 2)))
	#define PULL_BE_U64(data, pos) ((((uint64_t)PULL_BE_U32(data, pos)) << 32) | (uint64_t)(PULL_BE_U32(data, (pos) + 4)))

	#define PUSH_BE_U8(data, pos, val) (_DATA_BYTE(data, pos) = ((uint8)(val)))
	#define PUSH_BE_U16(data, pos, val) (PUSH_BE_U8((data), (pos), (uint8)(((uint16_t)(val)) >> 8)), PUSH_BE_U8((data), (pos) + 1, (uint8)((val) & 0xff)))
	#define PUSH_BE_U32(data, pos, val) (PUSH_BE_U16((data), (pos), (uint16_t)(((uint32_t)(val)) >> 16)), PUSH_BE_U16((data), (pos) + 2, (uint16_t)((val) & 0xffff)))
	#define PUSH_BE_U64(data, pos, val) (PUSH_BE_U32((data), (pos), (uint32_t)(((uint64_t)(val)) >> 32)), PUSH_BE_U32((data), (pos) + 4, (uint32_t)((val) & 0xffffffff)))
//
#include "agent.h"
#include "scp.h"
#include "sftp.h"
//#include "sftp_priv.h"
	sftp_packet sftp_packet_read(sftp_session sftp);
	ssize_t sftp_packet_write(sftp_session sftp, uint8 type, ssh_buffer payload);
	void sftp_packet_free(sftp_packet packet);
	int buffer_add_attributes(ssh_buffer buffer, sftp_attributes attr);
	sftp_attributes sftp_parse_attr(sftp_session session, ssh_buffer buf, int expectname);
//
//#include <gssapi/gssapi.h>
#include "legacy.h"

#ifdef HAVE_OPENSSL_X25519
	#include <openssl/err.h>
#endif
#ifdef HAVE_OPENSSL_EC_H
	#include <openssl/ec.h>
#endif
#ifdef HAVE_OPENSSL_ECDSA_H
	#include <openssl/ecdsa.h>
#endif
#ifdef HAVE_OPENSSL_ED25519
	// If using OpenSSL implementation, define the signature lenght which would be defined in libssh/ed25519.h otherwise 
	#define ED25519_SIG_LEN 64
#else
	#include "ed25519.h"
#endif
#ifdef _WIN32
	#include <ws2tcpip.h>
	// <wspiapi.h> is necessary for getaddrinfo before Windows XP, but it isn't available on some platforms like MinGW.
	#ifdef HAVE_WSPIAPI_H
		#include <wspiapi.h>
	#endif
	#define SOCKOPT_TYPE_ARG4 char
#else /* _WIN32 */
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <netdb.h>
	#define SOCKOPT_TYPE_ARG4 int
#endif /* _WIN32 */

#endif // } __LIBSSH_INTERNAL_H
