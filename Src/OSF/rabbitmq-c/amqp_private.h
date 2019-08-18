#ifndef librabbitmq_amqp_private_h
#define librabbitmq_amqp_private_h
/*
 * ***** BEGIN LICENSE BLOCK *****
 * Version: MIT
 *
 * Portions created by Alan Antonuk are Copyright (c) 2012-2014
 * Alan Antonuk. All Rights Reserved.
 *
 * Portions created by VMware are Copyright (c) 2007-2012 VMware, Inc.
 * All Rights Reserved.
 *
 * Portions created by Tony Garnock-Jones are Copyright (c) 2009-2010
 * VMware, Inc. and Tony Garnock-Jones. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * ***** END LICENSE BLOCK *****
 */
#define __STDC_LIMIT_MACROS
#include <slib.h>
//#ifdef HAVE_CONFIG_H
	//#include "config.h"
//#endif
// config.h {
#ifndef __cplusplus
	#define inline __inline
#endif
#define HAVE_SELECT
#define __STDC_LIMIT_MACROS
//#undef HAVE_POLL 
#define AMQ_PLATFORM "Windows"
// } config.h
#define AMQ_COPYRIGHT "Copyright (c) 2007-2014 VMWare Inc, Tony Garnock-Jones, and Alan Antonuk."
#include "amqp.h"
#include <openssl/ssl.h>
//#include <openssl/err.h>
//#include <openssl/bn.h>
//#include <openssl/conf.h>
//#include <openssl/bio.h>
#include <openssl/engine.h>
//#include <openssl/evp.h>
//#include <openssl/rsa.h>
//#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include "pthread.h" // @sobolev

#if ((defined(_WIN32)) || (defined(__MINGW32__)) || (defined(__MINGW64__)))
	#ifndef WINVER
		// WINVER 0x0502 is WinXP SP2+, Windows Server 2003 SP1+
		// See: http://msdn.microsoft.com/en-us/library/windows/desktop/aa383745(v=vs.85).aspx#macros_for_conditional_declarations
		#define WINVER 0x0502
	#endif
	#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN
	#endif
	//#include <winsock2.h>
#else
	#include <arpa/inet.h>
	#include <sys/uio.h>
#endif
/* GCC attributes */
#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ > 4)
	#define AMQP_NORETURN __attribute__((__noreturn__))
	#define AMQP_UNUSED __attribute__((__unused__))
#elif defined(_MSC_VER)
	#define AMQP_NORETURN __declspec(noreturn)
	#define AMQP_UNUSED
#else
	#define AMQP_NORETURN
	#define AMQP_UNUSED
#endif
#if __GNUC__ >= 4
	#define AMQP_PRIVATE __attribute__((visibility("hidden")))
#else
	#define AMQP_PRIVATE
#endif

char * amqp_os_error_string(int err);
#ifdef WITH_SSL
	char * amqp_ssl_error_string(int err);
#endif

//#include "amqp_hostcheck.h"
// amqp_hostcheck.h {
typedef enum {
	AMQP_HCR_NO_MATCH = 0,
	AMQP_HCR_MATCH = 1
} amqp_hostcheck_result;
// 
// Determine whether hostname matches match_pattern.
// 
// match_pattern may include wildcards.
// 
// Match is performed based on the rules set forth in RFC6125 section 6.4.3. http://tools.ietf.org/html/rfc6125#section-6.4.3
// 
// \param match_pattern RFC6125 compliant pattern
// \param hostname to match against
// \returns AMQP_HCR_MATCH if its a match, AMQP_HCR_NO_MATCH otherwise.
// 
amqp_hostcheck_result amqp_hostcheck(const char * match_pattern, const char * hostname);
// } amqp_hostcheck.h
//#include "amqp_table.h"
// amqp_table.h {
/**
 * Initializes a table entry with utf-8 string type value.
 *
 * \param [in] key the table entry key. The string must remain valid for the
 * life of the resulting amqp_table_entry_t.
 * \param [in] value the string value. The string must remain valid for the life
 * of the resulting amqp_table_entry_t.
 * \returns An initialized table entry.
 */
amqp_table_entry_t amqp_table_construct_utf8_entry(const char *key, const char *value);
/**
 * Initializes a table entry with table type value.
 *
 * \param [in] key the table entry key. The string must remain value for the
 * life of the resulting amqp_table_entry_t.
 * \param [in] value the amqp_table_t value. The table must remain valid for the
 * life of the resulting amqp_table_entry_t.
 * \returns An initialized table entry.
 */
amqp_table_entry_t amqp_table_construct_table_entry(const char *key, const amqp_table_t *value);
/**
 * Initializes a table entry with boolean type value.
 *
 * \param [in] key the table entry key. The string must remain value for the
 * life of the resulting amqp_table_entry_t.
 * \param [in] value the boolean value. 0 means false, any other value is true.
 * \returns An initialized table entry.
 */
amqp_table_entry_t amqp_table_construct_bool_entry(const char *key, const int value);
/**
 * Searches a table for an entry with a matching key.
 *
 * \param [in] table the table to search.
 * \param [in] key the string to search with.
 * \returns a pointer to the table entry in the table if a matching key can be
 * found, NULL otherwise.
 */
amqp_table_entry_t * amqp_table_get_entry_by_key(const amqp_table_t *table, const amqp_bytes_t key);
// } amqp_table.h
/*
 * Connection states: XXX FIX THIS
 *
 * - CONNECTION_STATE_INITIAL: The initial state, when we cannot be
 *   sure if the next thing we will get is the first AMQP frame, or a
 *   protocol header from the server.
 *
 * - CONNECTION_STATE_IDLE: The normal state between
 *   frames. Connections may only be reconfigured, and the
 *   connection's pools recycled, when in this state. Whenever we're
 *   in this state, the inbound_buffer's bytes pointer must be NULL;
 *   any other state, and it must point to a block of memory allocated
 *   from the frame_pool.
 *
 * - CONNECTION_STATE_HEADER: Some bytes of an incoming frame have
 *   been seen, but not a complete frame header's worth.
 *
 * - CONNECTION_STATE_BODY: A complete frame header has been seen, but
 *   the frame is not yet complete. When it is completed, it will be
 *   returned, and the connection will return to IDLE state.
 *
 */
enum amqp_connection_state_enum {
	CONNECTION_STATE_IDLE = 0,
	CONNECTION_STATE_INITIAL,
	CONNECTION_STATE_HEADER,
	CONNECTION_STATE_BODY
};

enum amqp_status_private_enum {
	/* 0x00xx -> AMQP_STATUS_*/
	/* 0x01xx -> AMQP_STATUS_TCP_* */
	/* 0x02xx -> AMQP_STATUS_SSL_* */
	AMQP_PRIVATE_STATUS_SOCKET_NEEDREAD = -0x1301,
	AMQP_PRIVATE_STATUS_SOCKET_NEEDWRITE = -0x1302
};

/* 7 bytes up front, then payload, then 1 byte footer */
#define HEADER_SIZE 7
#define FOOTER_SIZE 1
#define AMQP_PSEUDOFRAME_PROTOCOL_HEADER 'A'
#define POOL_TABLE_SIZE 16

struct amqp_link_t {
	amqp_link_t * next;
	void * data;
};

struct amqp_pool_table_entry_t {
	amqp_pool_table_entry_t * next;
	amqp_pool_t pool;
	amqp_channel_t channel;
};

struct amqp_connection_state_t_ {
	amqp_pool_table_entry_t * pool_table[POOL_TABLE_SIZE];
	amqp_connection_state_enum state;
	int channel_max;
	int frame_max;
	// Heartbeat interval in seconds. If this is <= 0, then heartbeats are not
	// enabled, and next_recv_heartbeat and next_send_heartbeat are set to infinite 
	int heartbeat;
	amqp_time_t next_recv_heartbeat;
	amqp_time_t next_send_heartbeat;
	// buffer for holding frame headers.  Allows us to delay allocating
	// the raw frame buffer until the type, channel, and size are all known
	char header_buffer[HEADER_SIZE + 1];
	amqp_bytes_t inbound_buffer;
	size_t inbound_offset;
	size_t target_size;
	amqp_bytes_t outbound_buffer;
	amqp_socket_t * socket;
	amqp_bytes_t sock_inbound_buffer;
	size_t sock_inbound_offset;
	size_t sock_inbound_limit;
	amqp_link_t * first_queued_frame;
	amqp_link_t * last_queued_frame;
	amqp_rpc_reply_t most_recent_api_result;
	amqp_table_t server_properties;
	amqp_table_t client_properties;
	amqp_pool_t properties_pool;
	struct timeval * handshake_timeout;
	struct timeval internal_handshake_timeout;
	struct timeval * rpc_timeout;
	struct timeval internal_rpc_timeout;
};

amqp_pool_t * amqp_get_or_create_channel_pool(amqp_connection_state_t connection, amqp_channel_t channel);
amqp_pool_t * amqp_get_channel_pool(amqp_connection_state_t state, amqp_channel_t channel);
int amqp_try_recv(amqp_connection_state_t state);
static inline int amqp_heartbeat_send(const amqp_connection_state_t state) { return state->heartbeat; }
static inline int amqp_heartbeat_recv(const amqp_connection_state_t state) { return 2 * state->heartbeat; }
static inline void * amqp_offset(void * data, size_t offset)  { return (char*)data + offset; }
//
// This macro defines the encoding and decoding functions associated with a simple type. 
//
#define DECLARE_CODEC_BASE_TYPE(bits)                                        \
	static inline int amqp_encode_ ## bits(amqp_bytes_t encoded, size_t *offset, uint ## bits input) { \
		size_t o = *offset;                                                      \
		if((*offset = o + bits / 8) <= encoded.len) {                           \
			amqp_e ## bits(input, amqp_offset(encoded.bytes, o));                    \
			return 1;                                                              \
		}                                                                        \
		return 0;                                                                \
	}                                                                          \
	static inline int amqp_decode_ ## bits(amqp_bytes_t encoded, size_t *offset, uint ## bits *output) { \
		size_t o = *offset;                                                      \
		if((*offset = o + bits / 8) <= encoded.len) {                           \
			*output = amqp_d ## bits(amqp_offset(encoded.bytes, o));                 \
			return 1;                                                              \
		}                                                                        \
		return 0;                                                                \
	}

static inline int is_bigendian() 
{
	union {
		uint32 i;
		char c[4];
	} bint = {0x01020304};
	return bint.c[0] == 1;
}

static inline void amqp_e8(uint8 val, void * data) 
{
	memcpy(data, &val, sizeof(val));
}

static inline uint8 amqp_d8(const void * data) 
{
	uint8 val;
	memcpy(&val, data, sizeof(val));
	return val;
}

static inline void amqp_e16(uint16 val, void * data) 
{
	if(!is_bigendian()) {
		val = ((val & 0xFF00u) >> 8u) | ((val & 0x00FFu) << 8u);
	}
	memcpy(data, &val, sizeof(val));
}

static inline uint16 amqp_d16(const void * data) 
{
	uint16 val;
	memcpy(&val, data, sizeof(val));
	if(!is_bigendian()) {
		val = ((val & 0xFF00u) >> 8u) | ((val & 0x00FFu) << 8u);
	}
	return val;
}

static inline void amqp_e32(uint32 val, void * data) 
{
	if(!is_bigendian()) {
		val = ((val & 0xFF000000u) >> 24u) | ((val & 0x00FF0000u) >> 8u) | ((val & 0x0000FF00u) << 8u) | ((val & 0x000000FFu) << 24u);
	}
	memcpy(data, &val, sizeof(val));
}

static inline uint32 amqp_d32(const void * data) 
{
	uint32 val;
	memcpy(&val, data, sizeof(val));
	if(!is_bigendian()) {
		val = ((val & 0xFF000000u) >> 24u) | ((val & 0x00FF0000u) >> 8u) | ((val & 0x0000FF00u) << 8u) | ((val & 0x000000FFu) << 24u);
	}
	return val;
}

static inline void amqp_e64(uint64 val, void * data) 
{
	if(!is_bigendian()) {
		val = ((val & 0xFF00000000000000u) >> 56u) | ((val & 0x00FF000000000000u) >> 40u) |
		    ((val & 0x0000FF0000000000u) >> 24u) | ((val & 0x000000FF00000000u) >> 8u) |
		    ((val & 0x00000000FF000000u) << 8u) | ((val & 0x0000000000FF0000u) << 24u) |
		    ((val & 0x000000000000FF00u) << 40u) | ((val & 0x00000000000000FFu) << 56u);
	}
	memcpy(data, &val, sizeof(val));
}

static inline uint64 amqp_d64(const void * data) 
{
	uint64 val;
	memcpy(&val, data, sizeof(val));
	if(!is_bigendian()) {
		val = ((val & 0xFF00000000000000u) >> 56u) | ((val & 0x00FF000000000000u) >> 40u) |
		    ((val & 0x0000FF0000000000u) >> 24u) | ((val & 0x000000FF00000000u) >> 8u) |
		    ((val & 0x00000000FF000000u) << 8u) | ((val & 0x0000000000FF0000u) << 24u) |
		    ((val & 0x000000000000FF00u) << 40u) | ((val & 0x00000000000000FFu) << 56u);
	}
	return val;
}

DECLARE_CODEC_BASE_TYPE(8)
DECLARE_CODEC_BASE_TYPE(16)
DECLARE_CODEC_BASE_TYPE(32)
DECLARE_CODEC_BASE_TYPE(64)

static inline int amqp_encode_bytes(amqp_bytes_t encoded, size_t * offset, const amqp_bytes_t input) 
{
	size_t o = *offset;
	// The memcpy below has undefined behavior if the input is NULL. It is valid
	// for a 0-length amqp_bytes_t to have .bytes == NULL. Thus we should check before encoding.
	if(input.len == 0)
		return 1;
	else if((*offset = o + input.len) <= encoded.len) {
		memcpy(amqp_offset(encoded.bytes, o), input.bytes, input.len);
		return 1;
	}
	else
		return 0;
}

static inline int amqp_decode_bytes(amqp_bytes_t encoded, size_t * offset, amqp_bytes_t * output, size_t len) 
{
	size_t o = *offset;
	if((*offset = o + len) <= encoded.len) {
		output->bytes = amqp_offset(encoded.bytes, o);
		output->len = len;
		return 1;
	}
	else
		return 0;
}

AMQP_NORETURN void amqp_abort(const char * fmt, ...);
int amqp_bytes_equal(const amqp_bytes_t r, const amqp_bytes_t l);

static inline amqp_rpc_reply_t amqp_rpc_reply_error(amqp_status_enum status) 
{
	amqp_rpc_reply_t reply;
	reply.reply_type = AMQP_RESPONSE_LIBRARY_EXCEPTION;
	reply.library_error = status;
	return reply;
}

int amqp_send_frame_inner(amqp_connection_state_t state, const amqp_frame_t * frame, int flags, amqp_time_t deadline);
#endif

//#include "amqp_openssl_bio.h"
// amqp_openssl_bio.h {
int    amqp_openssl_bio_init();
void   amqp_openssl_bio_destroy();

#if (OPENSSL_VERSION_NUMBER >= 0x10100000L)
	#define AMQP_OPENSSL_V110
#endif
#ifdef AMQP_OPENSSL_V110
	typedef const BIO_METHOD * BIO_METHOD_PTR;
#else
	typedef BIO_METHOD * BIO_METHOD_PTR;
#endif
BIO_METHOD_PTR amqp_openssl_bio();
// } amqp_openssl_bio.h
//#include "amqp_openssl_hostname_validation.h"
// amqp_openssl_hostname_validation.h {
//
// 
// Originally from:
// https://github.com/iSECPartners/ssl-conservatory
// https://wiki.openssl.org/index.php/Hostname_validation
// 
typedef enum {
	AMQP_HVR_MATCH_FOUND,
	AMQP_HVR_MATCH_NOT_FOUND,
	AMQP_HVR_NO_SAN_PRESENT,
	AMQP_HVR_MALFORMED_CERTIFICATE,
	AMQP_HVR_ERROR
} amqp_hostname_validation_result;
/**
* Validates the server's identity by looking for the expected hostname in the
* server's certificate. As described in RFC 6125, it first tries to find a match
* in the Subject Alternative Name extension. If the extension is not present in
* the certificate, it checks the Common Name instead.
*
* Returns AMQP_HVR_MATCH_FOUND if a match was found.
* Returns AMQP_HVR_MATCH_NOT_FOUND if no matches were found.
* Returns AMQP_HVR_MALFORMED_CERTIFICATE if any of the hostnames had a NUL
* character embedded in it.
* Returns AMQP_HVR_ERROR if there was an error.
*/
amqp_hostname_validation_result amqp_ssl_validate_hostname(const char *hostname, const X509 *server_cert);
// } amqp_openssl_hostname_validation.h
//#include "amqp_ssl_socket.h"
// amqp_ssl_socket.h {
AMQP_BEGIN_DECLS
	/**
	 * Create a new SSL/TLS socket object.
	 *
	 * The returned socket object is owned by the \ref amqp_connection_state_t
	 * object and will be destroyed when the state object is destroyed or a new
	 * socket object is created.
	 *
	 * If the socket object creation fails, the \ref amqp_connection_state_t object
	 * will not be changed.
	 *
	 * The object returned by this function can be retrieved from the
	 * amqp_connection_state_t object later using the amqp_get_socket() function.
	 *
	 * Calling this function may result in the underlying SSL library being
	 * initialized.
	 * \sa amqp_set_initialize_ssl_library()
	 *
	 * \param [in,out] state The connection object that owns the SSL/TLS socket
	 * \return A new socket object or NULL if an error occurred.
	 *
	 * \since v0.4.0
	 */
	AMQP_PUBLIC_FUNCTION amqp_socket_t * amqp_ssl_socket_new(amqp_connection_state_t state);
	// 
	// Descr: Get the internal OpenSSL context. Caveat emptor.
	// \param [in,out] self An SSL/TLS socket object.
	// \return A pointer to the internal OpenSSL context. This should be cast to <tt>SSL_CTX*</tt>.
	// 
	AMQP_PUBLIC_FUNCTION void * amqp_ssl_socket_get_context(amqp_socket_t * self);
	// 
	// Descr: Set the CA certificate.
	// \param [in,out] self An SSL/TLS socket object.
	// \param [in] cacert Path to the CA cert file in PEM format.
	// \return \ref AMQP_STATUS_OK on success an \ref amqp_status_enum value on failure.
	// 
	AMQP_PUBLIC_FUNCTION int amqp_ssl_socket_set_cacert(amqp_socket_t * self, const char * cacert);
	/**
	 * Set the client key.
	 *
	 * \param [in,out] self An SSL/TLS socket object.
	 * \param [in] cert Path to the client certificate in PEM foramt.
	 * \param [in] key Path to the client key in PEM format.
	 *
	 * \return \ref AMQP_STATUS_OK on success an \ref amqp_status_enum value on
	 *  failure.
	 *
	 * \since v0.4.0
	 */
	AMQP_PUBLIC_FUNCTION int amqp_ssl_socket_set_key(amqp_socket_t * self, const char * cert, const char * key);
	/**
	 * Set the client key from a buffer.
	 *
	 * \param [in,out] self An SSL/TLS socket object.
	 * \param [in] cert Path to the client certificate in PEM foramt.
	 * \param [in] key A buffer containing client key in PEM format.
	 * \param [in] n The length of the buffer.
	 *
	 * \return \ref AMQP_STATUS_OK on success an \ref amqp_status_enum value on
	 *  failure.
	 *
	 * \since v0.4.0
	 */
	AMQP_PUBLIC_FUNCTION int amqp_ssl_socket_set_key_buffer(amqp_socket_t * self, const char * cert, const void * key, size_t n);
	/**
	 * Enable or disable peer verification.
	 *
	 * \deprecated use \amqp_ssl_socket_set_verify_peer and
	 * \amqp_ssl_socket_set_verify_hostname instead.
	 *
	 * If peer verification is enabled then the common name in the server
	 * certificate must match the server name. Peer verification is enabled by
	 * default.
	 *
	 * \param [in,out] self An SSL/TLS socket object.
	 * \param [in] verify Enable or disable peer verification.
	 *
	 * \since v0.4.0
	 */
	AMQP_DEPRECATED(AMQP_PUBLIC_FUNCTION void amqp_ssl_socket_set_verify(amqp_socket_t * self, boolint verify));
	/**
	 * Enable or disable peer verification.
	 *
	 * Peer verification validates the certificate chain that is sent by the broker.
	 * Hostname validation is controlled by \amqp_ssl_socket_set_verify_peer.
	 *
	 * \param [in,out] self An SSL/TLS socket object.
	 * \param [in] verify enable or disable peer validation
	 *
	 * \since v0.8.0
	 */
	AMQP_PUBLIC_FUNCTION void amqp_ssl_socket_set_verify_peer(amqp_socket_t * self, boolint verify);
	/**
	 * Enable or disable hostname verification.
	 *
	 * Hostname verification checks the broker cert for a CN or SAN that matches the
	 * hostname that amqp_socket_open() is presented. Peer verification is
	 * controlled by \amqp_ssl_socket_set_verify_peer
	 *
	 * \since v0.8.0
	 */
	AMQP_PUBLIC_FUNCTION void amqp_ssl_socket_set_verify_hostname(amqp_socket_t * self, boolint verify);

	typedef enum {
		AMQP_TLSv1 = 1,
		AMQP_TLSv1_1 = 2,
		AMQP_TLSv1_2 = 3,
		AMQP_TLSvLATEST = 0xFFFF
	} amqp_tls_version_t;

	/**
	 * Set min and max TLS versions.
	 *
	 * Set the oldest and newest acceptable TLS versions that are acceptable when
	 * connecting to the broker. Set min == max to restrict to just that
	 * version.
	 *
	 * \param [in,out] self An SSL/TLS socket object.
	 * \param [in] min the minimum acceptable TLS version
	 * \param [in] max the maxmium acceptable TLS version
	 * \returns AMQP_STATUS_OK on success, AMQP_STATUS_UNSUPPORTED if OpenSSL does
	 * not support the requested TLS version, AMQP_STATUS_INVALID_PARAMETER if an
	 * invalid combination of parameters is passed.
	 *
	 * \since v0.8.0
	 */
	AMQP_PUBLIC_FUNCTION int amqp_ssl_socket_set_ssl_versions(amqp_socket_t * self, amqp_tls_version_t min, amqp_tls_version_t max);
	/**
	 * Sets whether rabbitmq-c will initialize OpenSSL.
	 *
	 * OpenSSL requires a one-time initialization across a whole program, this sets
	 * whether or not rabbitmq-c will initialize the SSL library when the first call
	 * to amqp_ssl_socket_new() is made. You should call this function with
	 * do_init = 0 if the underlying SSL library is initialized somewhere else
	 * the program.
	 *
	 * Failing to initialize or double initialization of the SSL library will
	 * result in undefined behavior
	 *
	 * By default rabbitmq-c will initialize the underlying SSL library.
	 *
	 * NOTE: calling this function after the first socket has been opened with
	 * amqp_open_socket() will not have any effect.
	 *
	 * \param [in] do_initialize If 0 rabbitmq-c will not initialize the SSL
	 * library, otherwise rabbitmq-c will initialize the SSL library
	 *
	 * \since v0.4.0
	 */
	AMQP_PUBLIC_FUNCTION void amqp_set_initialize_ssl_library(boolint do_initialize);
	// 
	// Initialize the underlying SSL/TLS library.
	// The OpenSSL library requires a one-time initialization across the whole program.
	// This function unconditionally initializes OpenSSL so that rabbitmq-c may use it.
	// This function is thread-safe, and may be called more than once.
	// \return AMQP_STATUS_OK on success.
	// 
	AMQP_PUBLIC_FUNCTION int amqp_initialize_ssl_library();
	// 
	// Descr: Uninitialize the underlying SSL/TLS library.
	// \return AMQP_STATUS_OK on success.
	// 
	AMQP_PUBLIC_FUNCTION int amqp_uninitialize_ssl_library();
AMQP_END_DECLS
// } amqp_ssl_socket.h
