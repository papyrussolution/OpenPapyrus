// PPAMQP.CPP
//
// ***** BEGIN LICENSE BLOCK *****
// Version: MIT
// 
// Portions created by Alan Antonuk are Copyright (c) 2012-2013 Alan Antonuk. All Rights Reserved.
// Portions created by VMware are Copyright (c) 2007-2012 VMware, Inc. All Rights Reserved.
// Portions created by Tony Garnock-Jones are Copyright (c) 2009-2010 VMware, Inc. and Tony Garnock-Jones. All Rights Reserved.
// 
// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use, copy,
// modify, merge, publish, distribute, sublicense, and/or sell copies
// of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
// BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
// ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// ***** END LICENSE BLOCK *****
// 
#include <slib-internal.h>
#pragma hdrstop
//#include <stdint.h>
#include <amqp.h>
#include <openssl/ssl.h>
#include <openssl/engine.h>
#include <openssl/x509v3.h>
#include "pthread.h" // @sobolev
//
//#include "amqp_private.h"
// config.h {
#define HAVE_SELECT
//#undef HAVE_POLL 
// } config.h

#define AMQ_PLATFORM "Windows"
#define AMQ_COPYRIGHT "Copyright (c) 2007-2014 VMWare Inc, Tony Garnock-Jones, and Alan Antonuk."
#ifdef _MSC_VER
	#define _CRT_SECURE_NO_WARNINGS
#endif
#if ((defined(_WIN32)) || (defined(__MINGW32__)) || (defined(__MINGW64__)))
	#ifndef WINVER
		// WINVER 0x0502 is WinXP SP2+, Windows Server 2003 SP1+
		// See: http://msdn.microsoft.com/en-us/library/windows/desktop/aa383745(v=vs.85).aspx#macros_for_conditional_declarations
		//#define WINVER 0x0502
	#endif
	#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN
	#endif
	//#include <winsock2.h>
	//#include <ws2tcpip.h>
#else
	#include <sys/types.h> // On older BSD types.h must come before net includes 
	#include <arpa/inet.h>
	#include <sys/uio.h>
	#include <netinet/in.h>
	#include <netinet/tcp.h>
	#include <sys/socket.h>
	#ifdef HAVE_SELECT
		#include <sys/select.h>
	#endif
	#include <fcntl.h>
	#include <netdb.h>
	#ifdef HAVE_POLL
		#include <poll.h>
	#endif
	#include <unistd.h>
#endif
// GCC attributes 
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
enum amqp_hostcheck_result {
	AMQP_HCR_NO_MATCH = 0,
	AMQP_HCR_MATCH = 1
};
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
int amqp_try_recv(amqp_connection_state_t state);
static inline int amqp_heartbeat_send(const amqp_connection_state_t state) { return state->heartbeat; }
static inline int amqp_heartbeat_recv(const amqp_connection_state_t state) { return 2 * state->heartbeat; }
static inline void * amqp_offset(void * data, size_t offset)  { return PTR8(data) + offset; }
//
// This macro defines the encoding and decoding functions associated with a simple type. 
//
#define DECLARE_CODEC_BASE_TYPE(bits)                                        \
	static /*inline*/ int FASTCALL amqp_encode_ ## bits(amqp_bytes_t encoded, size_t *offset, uint ## bits input) { \
		size_t o = *offset;                                                      \
		if((*offset = o + bits / 8) <= encoded.len) {                           \
			amqp_e ## bits(input, amqp_offset(encoded.bytes, o));                    \
			return 1;                                                              \
		}                                                                        \
		return 0;                                                                \
	}                                                                          \
	static /*inline*/ int FASTCALL amqp_decode_ ## bits(amqp_bytes_t encoded, size_t *offset, uint ## bits *output) { \
		size_t o = *offset;                                                      \
		if((*offset = o + bits / 8) <= encoded.len) {                           \
			*output = amqp_d ## bits(amqp_offset(encoded.bytes, o));                 \
			return 1;                                                              \
		}                                                                        \
		return 0;                                                                \
	}

/*static inline int is_bigendian() 
{
	union {
		uint32 i;
		char c[4];
	} bint = {0x01020304};
	return bint.c[0] == 1;
}*/

static inline void amqp_e8(uint8 val, void * data) 
{
	//memcpy(data, &val, sizeof(val));
	PTR8(data)[0] = val;
}

static inline uint8 amqp_d8(const void * data) 
{
	/*
	uint8 val;
	memcpy(&val, data, sizeof(val));
	return val;
	*/
	return PTR8C(data)[0];
}

static inline void amqp_e16(uint16 val, void * data) 
{
	if(!(SLS.GetSSys().Flags & SSystem::fBigEndian)) {
		val = ((val & 0xFF00u) >> 8u) | ((val & 0x00FFu) << 8u);
	}
	memcpy(data, &val, sizeof(val));
}

static inline uint16 amqp_d16(const void * data) 
{
	uint16 val;
	memcpy(&val, data, sizeof(val));
	if(!(SLS.GetSSys().Flags & SSystem::fBigEndian)) {
		val = ((val & 0xFF00u) >> 8u) | ((val & 0x00FFu) << 8u);
	}
	return val;
}

static inline void amqp_e32(uint32 val, void * data) 
{
	if(!(SLS.GetSSys().Flags & SSystem::fBigEndian)) {
		val = ((val & 0xFF000000u) >> 24u) | ((val & 0x00FF0000u) >> 8u) | ((val & 0x0000FF00u) << 8u) | ((val & 0x000000FFu) << 24u);
	}
	memcpy(data, &val, sizeof(val));
}

static inline uint32 amqp_d32(const void * data) 
{
	uint32 val;
	memcpy(&val, data, sizeof(val));
	if(!(SLS.GetSSys().Flags & SSystem::fBigEndian)) {
		val = ((val & 0xFF000000u) >> 24u) | ((val & 0x00FF0000u) >> 8u) | ((val & 0x0000FF00u) << 8u) | ((val & 0x000000FFu) << 24u);
	}
	return val;
}

static inline void amqp_e64(uint64 val, void * data) 
{
	if(!(SLS.GetSSys().Flags & SSystem::fBigEndian)) {
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
	if(!(SLS.GetSSys().Flags & SSystem::fBigEndian)) {
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

static /*inline*/ int FASTCALL amqp_encode_bytes(amqp_bytes_t encoded, size_t * offset, const amqp_bytes_t input) 
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

static /*inline*/ int FASTCALL amqp_decode_bytes(amqp_bytes_t encoded, size_t * offset, amqp_bytes_t * output, size_t len) 
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

static /*inline*/ amqp_rpc_reply_t FASTCALL amqp_rpc_reply_error(amqp_status_enum status) 
{
	amqp_rpc_reply_t reply;
	reply.reply_type = AMQP_RESPONSE_LIBRARY_EXCEPTION;
	reply.library_error = status;
	return reply;
}

int amqp_send_frame_inner(amqp_connection_state_t state, const amqp_frame_t * frame, int flags, amqp_time_t deadline);

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
enum amqp_hostname_validation_result {
	AMQP_HVR_MATCH_FOUND,
	AMQP_HVR_MATCH_NOT_FOUND,
	AMQP_HVR_NO_SAN_PRESENT,
	AMQP_HVR_MALFORMED_CERTIFICATE,
	AMQP_HVR_ERROR
};
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
	 * If the socket object creation fails, the \ref amqp_connection_state_t object will not be changed.
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
	 * \return \ref AMQP_STATUS_OK on success an \ref amqp_status_enum value on failure.
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
	 * \return \ref AMQP_STATUS_OK on success an \ref amqp_status_enum value on failure.
	 */
	AMQP_PUBLIC_FUNCTION int amqp_ssl_socket_set_key_buffer(amqp_socket_t * self, const char * cert, const void * key, size_t n);
	/**
	 * Enable or disable peer verification.
	 *
	 * \deprecated use \amqp_ssl_socket_set_verify_peer and \amqp_ssl_socket_set_verify_hostname instead.
	 *
	 * If peer verification is enabled then the common name in the server
	 * certificate must match the server name. Peer verification is enabled by
	 * default.
	 *
	 * \param [in,out] self An SSL/TLS socket object.
	 * \param [in] verify Enable or disable peer verification.
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
	 */
	AMQP_PUBLIC_FUNCTION void amqp_ssl_socket_set_verify_peer(amqp_socket_t * self, boolint verify);
	/**
	 * Enable or disable hostname verification.
	 *
	 * Hostname verification checks the broker cert for a CN or SAN that matches the
	 * hostname that amqp_socket_open() is presented. Peer verification is
	 * controlled by \amqp_ssl_socket_set_verify_peer
	 */
	AMQP_PUBLIC_FUNCTION void amqp_ssl_socket_set_verify_hostname(amqp_socket_t * self, boolint verify);

	enum amqp_tls_version_t {
		AMQP_TLSv1 = 1,
		AMQP_TLSv1_1 = 2,
		AMQP_TLSv1_2 = 3,
		AMQP_TLSvLATEST = 0xFFFF
	};

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
//
// AMQP_API {
//
#ifdef _MSC_VER
	#define _CRT_SECURE_NO_WARNINGS /* MSVC complains about sprintf being deprecated in favor of sprintf_s */
	#define _CRT_NONSTDC_NO_DEPRECATE /* MSVC complains about strdup being deprecated in favor of _strdup */
#endif

#define ERROR_MASK (0x00FF)
#define ERROR_CATEGORY_MASK (0xFF00)

enum error_category_enum_ { 
	EC_base = 0, 
	EC_tcp = 1, 
	EC_ssl = 2 
};

static const char * base_error_strings[] = {
	"operation completed successfully", /* AMQP_STATUS_OK 0x0 */
	"could not allocate memory", /* AMQP_STATUS_NO_MEMORY                  -0x0001 */
	"invalid AMQP data", /* AMQP_STATUS_BAD_AQMP_DATA              -0x0002 */
	"unknown AMQP class id", /* AMQP_STATUS_UNKNOWN_CLASS              -0x0003 */
	"unknown AMQP method id", /* AMQP_STATUS_UNKNOWN_METHOD             -0x0004 */
	"hostname lookup failed", /* AMQP_STATUS_HOSTNAME_RESOLUTION_FAILED -0x0005 */
	"incompatible AMQP version", /* AMQP_STATUS_INCOMPATIBLE_AMQP_VERSION  -0x0006 */
	"connection closed unexpectedly", /* AMQP_STATUS_CONNECTION_CLOSED          -0x0007 */
	"could not parse AMQP URL", /* AMQP_STATUS_BAD_AMQP_URL               -0x0008 */
	"a socket error occurred", /* AMQP_STATUS_SOCKET_ERROR               -0x0009 */
	"invalid parameter", /* AMQP_STATUS_INVALID_PARAMETER          -0x000A */
	"table too large for buffer", /* AMQP_STATUS_TABLE_TOO_BIG              -0x000B */
	"unexpected method received", /* AMQP_STATUS_WRONG_METHOD               -0x000C */
	"request timed out", /* AMQP_STATUS_TIMEOUT                    -0x000D */
	"system timer has failed", /* AMQP_STATUS_TIMER_FAILED               -0x000E */
	"heartbeat timeout, connection closed", /* AMQP_STATUS_HEARTBEAT_TIMEOUT          -0x000F */
	"unexpected protocol state", /* AMQP_STATUS_UNEXPECTED STATE           -0x0010 */
	"socket is closed", /* AMQP_STATUS_SOCKET_CLOSED              -0x0011 */
	"socket already open", /* AMQP_STATUS_SOCKET_INUSE               -0x0012 */
	"unsupported sasl method requested", /* AMQP_STATUS_BROKER_UNSUPPORTED_SASL_METHOD -0x00013 */
	"parameter value is unsupported" /* AMQP_STATUS_UNSUPPORTED                -0x0014 */
};

static const char * tcp_error_strings[] = {
	"a socket error occurred", /* AMQP_STATUS_TCP_ERROR                  -0x0100 */
	"socket library initialization failed" /* AMQP_STATUS_TCP_SOCKETLIB_INIT_ERROR   -0x0101 */
};

static const char * ssl_error_strings[] = {
	"a SSL error occurred", /* AMQP_STATUS_SSL_ERRO  R                -0x0200 */
	"SSL hostname verification failed", /* AMQP_STATUS_SSL_HOSTNAME_VERIFY_FAILED -0x0201 */
	"SSL peer cert verification failed", /* AMQP_STATUS_SSL_PEER_VERIFY_FAILED     -0x0202 */
	"SSL handshake failed" /* AMQP_STATUS_SSL_CONNECTION_FAILED      -0x0203 */
};

const amqp_bytes_t amqp_empty_bytes = {0, NULL};
const amqp_table_t amqp_empty_table = {0, NULL};
const amqp_array_t amqp_empty_array = {0, NULL};

static const char * unknown_error_string = "(unknown error)";
static int amqp_ssl_bio_initialized = 0;
static pthread_mutex_t * amqp_openssl_lockarray = NULL;
static pthread_mutex_t openssl_init_mutex = 0; //PTHREAD_MUTEX_INITIALIZER;
static boolint do_initialize_openssl = 1;
static boolint openssl_initialized = 0;
static boolint openssl_bio_initialized = 0;
static int openssl_connections = 0;

const char * amqp_error_string2(int code) 
{
	const char * error_string = 0;
	size_t category = (((-code) & ERROR_CATEGORY_MASK) >> 8);
	size_t error = (-code) & ERROR_MASK;
	switch(category) {
		case EC_base: error_string = (error < SIZEOFARRAY(base_error_strings)) ? base_error_strings[error] : unknown_error_string; break;
		case EC_tcp:  error_string = (error < SIZEOFARRAY(tcp_error_strings)) ? tcp_error_strings[error] : unknown_error_string; break;
		case EC_ssl:  error_string = (error < SIZEOFARRAY(ssl_error_strings)) ? ssl_error_strings[error] : unknown_error_string; break;
		default: error_string = unknown_error_string; break;
	}
	return error_string;
}

char * amqp_error_string(int code) 
{
	// 
	// Previously sometimes clients had to flip the sign on a return value from a
	// function to get the correct error code. Now, all error codes are negative.
	// To keep people's legacy code running correctly, we map all error codes to negative values.
	// 
	// This is only done with this deprecated function.
	// 
	if(code > 0)
		code = -code;
	return _strdup(amqp_error_string2(code));
}

void amqp_abort(const char * fmt, ...) 
{
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fputc('\n', stderr);
	abort();
}

int amqp_basic_publish(amqp_connection_state_t state, amqp_channel_t channel, amqp_bytes_t exchange, amqp_bytes_t routing_key,
    boolint mandatory, boolint immediate, amqp_basic_properties_t const * properties, amqp_bytes_t body) 
{
	amqp_frame_t f;
	size_t body_offset;
	size_t usable_body_payload_size = state->frame_max - (HEADER_SIZE + FOOTER_SIZE);
	int res;
	int flagz;
	amqp_basic_publish_t m;
	amqp_basic_properties_t default_properties;
	m.exchange = exchange;
	m.routing_key = routing_key;
	m.mandatory = mandatory;
	m.immediate = immediate;
	m.ticket = 0;
	// TODO(alanxz): this heartbeat check is happening in the wrong place, it should really be done in amqp_try_send/writev 
	res = amqp_time_has_past(state->next_recv_heartbeat);
	if(res == AMQP_STATUS_TIMER_FAILURE)
		return res;
	else if(res == AMQP_STATUS_TIMEOUT) {
		res = amqp_try_recv(state);
		if(res == AMQP_STATUS_TIMEOUT)
			return AMQP_STATUS_HEARTBEAT_TIMEOUT;
		else if(res != AMQP_STATUS_OK)
			return res;
	}
	res = amqp_send_method_inner(state, channel, AMQP_BASIC_PUBLISH_METHOD, &m, AMQP_SF_MORE, amqp_time_infinite());
	if(res < 0)
		return res;
	else {
		if(!properties) {
			memzero(&default_properties, sizeof(default_properties));
			properties = &default_properties;
		}
		f.frame_type = AMQP_FRAME_HEADER;
		f.channel = channel;
		f.payload.properties.class_id = AMQP_BASIC_CLASS;
		f.payload.properties.body_size = body.len;
		f.payload.properties.decoded = (void *)properties;
		if(body.len > 0)
			flagz = AMQP_SF_MORE;
		else
			flagz = AMQP_SF_NONE;
		res = amqp_send_frame_inner(state, &f, flagz, amqp_time_infinite());
		if(res < 0)
			return res;
		else {
			body_offset = 0;
			while(body_offset < body.len) {
				size_t remaining = body.len - body_offset;
				if(remaining == 0) {
					break;
				}
				f.frame_type = AMQP_FRAME_BODY;
				f.channel = channel;
				f.payload.body_fragment.bytes = amqp_offset(body.bytes, body_offset);
				if(remaining >= usable_body_payload_size) {
					f.payload.body_fragment.len = usable_body_payload_size;
					flagz = AMQP_SF_MORE;
				}
				else {
					f.payload.body_fragment.len = remaining;
					flagz = AMQP_SF_NONE;
				}
				body_offset += f.payload.body_fragment.len;
				res = amqp_send_frame_inner(state, &f, flagz, amqp_time_infinite());
				if(res < 0)
					return res;
			}
			return AMQP_STATUS_OK;
		}
	}
}

amqp_rpc_reply_t amqp_channel_close(amqp_connection_state_t state, amqp_channel_t channel, int code) 
{
	char codestr[13];
	amqp_method_number_t replies[2] = {AMQP_CHANNEL_CLOSE_OK_METHOD, 0};
	amqp_channel_close_t req;
	if(code < 0 || code > UINT16_MAX) {
		return amqp_rpc_reply_error(AMQP_STATUS_INVALID_PARAMETER);
	}
	req.reply_code = (uint16)code;
	req.reply_text.bytes = codestr;
	req.reply_text.len = sprintf(codestr, "%d", code);
	req.class_id = 0;
	req.method_id = 0;
	return amqp_simple_rpc(state, channel, AMQP_CHANNEL_CLOSE_METHOD, replies, &req);
}

amqp_rpc_reply_t amqp_connection_close(amqp_connection_state_t state, int code) 
{
	char codestr[13];
	amqp_method_number_t replies[2] = {AMQP_CONNECTION_CLOSE_OK_METHOD, 0};
	amqp_channel_close_t req;
	if(code < 0 || code > UINT16_MAX) {
		return amqp_rpc_reply_error(AMQP_STATUS_INVALID_PARAMETER);
	}
	req.reply_code = (uint16)code;
	req.reply_text.bytes = codestr;
	req.reply_text.len = sprintf(codestr, "%d", code);
	req.class_id = 0;
	req.method_id = 0;
	return amqp_simple_rpc(state, 0, AMQP_CONNECTION_CLOSE_METHOD, replies, &req);
}

int amqp_basic_ack(amqp_connection_state_t state, amqp_channel_t channel, uint64 delivery_tag, boolint multiple) 
{
	amqp_basic_ack_t m;
	m.delivery_tag = delivery_tag;
	m.multiple = multiple;
	return amqp_send_method(state, channel, AMQP_BASIC_ACK_METHOD, &m);
}

amqp_rpc_reply_t amqp_basic_get(amqp_connection_state_t state, amqp_channel_t channel, amqp_bytes_t queue, boolint no_ack) 
{
	amqp_method_number_t replies[] = {AMQP_BASIC_GET_OK_METHOD, AMQP_BASIC_GET_EMPTY_METHOD, 0};
	amqp_basic_get_t req;
	req.ticket = 0;
	req.queue = queue;
	req.no_ack = no_ack;
	state->most_recent_api_result = amqp_simple_rpc(state, channel, AMQP_BASIC_GET_METHOD, replies, &req);
	return state->most_recent_api_result;
}

int amqp_basic_reject(amqp_connection_state_t state, amqp_channel_t channel, uint64 delivery_tag, boolint requeue) 
{
	amqp_basic_reject_t req;
	req.delivery_tag = delivery_tag;
	req.requeue = requeue;
	return amqp_send_method(state, channel, AMQP_BASIC_REJECT_METHOD, &req);
}

int amqp_basic_nack(amqp_connection_state_t state, amqp_channel_t channel, uint64 delivery_tag, boolint multiple, boolint requeue) 
{
	amqp_basic_nack_t req;
	req.delivery_tag = delivery_tag;
	req.multiple = multiple;
	req.requeue = requeue;
	return amqp_send_method(state, channel, AMQP_BASIC_NACK_METHOD, &req);
}

const struct timeval * amqp_get_handshake_timeout(const amqp_connection_state_t state) { return state->handshake_timeout; }
const struct timeval * amqp_get_rpc_timeout(const amqp_connection_state_t state) { return state->rpc_timeout; }

int amqp_set_handshake_timeout(amqp_connection_state_t state, struct timeval * timeout) 
{
	if(timeout) {
		if(timeout->tv_sec < 0 || timeout->tv_usec < 0)
			return AMQP_STATUS_INVALID_PARAMETER;
		else {
			state->internal_handshake_timeout = *timeout;
			state->handshake_timeout = &state->internal_handshake_timeout;
		}
	}
	else
		state->handshake_timeout = NULL;
	return AMQP_STATUS_OK;
}

int amqp_set_rpc_timeout(amqp_connection_state_t state, struct timeval * timeout) 
{
	if(timeout) {
		if(timeout->tv_sec < 0 || timeout->tv_usec < 0)
			return AMQP_STATUS_INVALID_PARAMETER;
		else {
			state->rpc_timeout = &state->internal_rpc_timeout;
			*state->rpc_timeout = *timeout;
		}
	}
	else
		state->rpc_timeout = NULL;
	return AMQP_STATUS_OK;
}
// } AMQP_API
//
// AMQP_FRAMING {
//
char const * amqp_constant_name(int constantNumber) 
{
	switch(constantNumber) {
		case AMQP_FRAME_METHOD: return "AMQP_FRAME_METHOD";
		case AMQP_FRAME_HEADER: return "AMQP_FRAME_HEADER";
		case AMQP_FRAME_BODY: return "AMQP_FRAME_BODY";
		case AMQP_FRAME_HEARTBEAT: return "AMQP_FRAME_HEARTBEAT";
		case AMQP_FRAME_MIN_SIZE: return "AMQP_FRAME_MIN_SIZE";
		case AMQP_FRAME_END: return "AMQP_FRAME_END";
		case AMQP_REPLY_SUCCESS: return "AMQP_REPLY_SUCCESS";
		case AMQP_CONTENT_TOO_LARGE: return "AMQP_CONTENT_TOO_LARGE";
		case AMQP_NO_ROUTE: return "AMQP_NO_ROUTE";
		case AMQP_NO_CONSUMERS: return "AMQP_NO_CONSUMERS";
		case AMQP_ACCESS_REFUSED: return "AMQP_ACCESS_REFUSED";
		case AMQP_NOT_FOUND: return "AMQP_NOT_FOUND";
		case AMQP_RESOURCE_LOCKED: return "AMQP_RESOURCE_LOCKED";
		case AMQP_PRECONDITION_FAILED: return "AMQP_PRECONDITION_FAILED";
		case AMQP_CONNECTION_FORCED: return "AMQP_CONNECTION_FORCED";
		case AMQP_INVALID_PATH: return "AMQP_INVALID_PATH";
		case AMQP_FRAME_ERROR: return "AMQP_FRAME_ERROR";
		case AMQP_SYNTAX_ERROR: return "AMQP_SYNTAX_ERROR";
		case AMQP_COMMAND_INVALID: return "AMQP_COMMAND_INVALID";
		case AMQP_CHANNEL_ERROR: return "AMQP_CHANNEL_ERROR";
		case AMQP_UNEXPECTED_FRAME: return "AMQP_UNEXPECTED_FRAME";
		case AMQP_RESOURCE_ERROR: return "AMQP_RESOURCE_ERROR";
		case AMQP_NOT_ALLOWED: return "AMQP_NOT_ALLOWED";
		case AMQP_NOT_IMPLEMENTED: return "AMQP_NOT_IMPLEMENTED";
		case AMQP_INTERNAL_ERROR: return "AMQP_INTERNAL_ERROR";
		default: return "(unknown)";
	}
}

boolint amqp_constant_is_hard_error(int constantNumber) 
{
	switch(constantNumber) {
		case AMQP_CONNECTION_FORCED: return 1;
		case AMQP_INVALID_PATH: return 1;
		case AMQP_FRAME_ERROR: return 1;
		case AMQP_SYNTAX_ERROR: return 1;
		case AMQP_COMMAND_INVALID: return 1;
		case AMQP_CHANNEL_ERROR: return 1;
		case AMQP_UNEXPECTED_FRAME: return 1;
		case AMQP_RESOURCE_ERROR: return 1;
		case AMQP_NOT_ALLOWED: return 1;
		case AMQP_NOT_IMPLEMENTED: return 1;
		case AMQP_INTERNAL_ERROR: return 1;
		default: return 0;
	}
}

char const * amqp_method_name(amqp_method_number_t methodNumber) 
{
	switch(methodNumber) {
		case AMQP_CONNECTION_START_METHOD: return "AMQP_CONNECTION_START_METHOD";
		case AMQP_CONNECTION_START_OK_METHOD: return "AMQP_CONNECTION_START_OK_METHOD";
		case AMQP_CONNECTION_SECURE_METHOD: return "AMQP_CONNECTION_SECURE_METHOD";
		case AMQP_CONNECTION_SECURE_OK_METHOD: return "AMQP_CONNECTION_SECURE_OK_METHOD";
		case AMQP_CONNECTION_TUNE_METHOD: return "AMQP_CONNECTION_TUNE_METHOD";
		case AMQP_CONNECTION_TUNE_OK_METHOD: return "AMQP_CONNECTION_TUNE_OK_METHOD";
		case AMQP_CONNECTION_OPEN_METHOD: return "AMQP_CONNECTION_OPEN_METHOD";
		case AMQP_CONNECTION_OPEN_OK_METHOD: return "AMQP_CONNECTION_OPEN_OK_METHOD";
		case AMQP_CONNECTION_CLOSE_METHOD: return "AMQP_CONNECTION_CLOSE_METHOD";
		case AMQP_CONNECTION_CLOSE_OK_METHOD: return "AMQP_CONNECTION_CLOSE_OK_METHOD";
		case AMQP_CONNECTION_BLOCKED_METHOD: return "AMQP_CONNECTION_BLOCKED_METHOD";
		case AMQP_CONNECTION_UNBLOCKED_METHOD: return "AMQP_CONNECTION_UNBLOCKED_METHOD";
		case AMQP_CHANNEL_OPEN_METHOD: return "AMQP_CHANNEL_OPEN_METHOD";
		case AMQP_CHANNEL_OPEN_OK_METHOD: return "AMQP_CHANNEL_OPEN_OK_METHOD";
		case AMQP_CHANNEL_FLOW_METHOD: return "AMQP_CHANNEL_FLOW_METHOD";
		case AMQP_CHANNEL_FLOW_OK_METHOD: return "AMQP_CHANNEL_FLOW_OK_METHOD";
		case AMQP_CHANNEL_CLOSE_METHOD: return "AMQP_CHANNEL_CLOSE_METHOD";
		case AMQP_CHANNEL_CLOSE_OK_METHOD: return "AMQP_CHANNEL_CLOSE_OK_METHOD";
		case AMQP_ACCESS_REQUEST_METHOD: return "AMQP_ACCESS_REQUEST_METHOD";
		case AMQP_ACCESS_REQUEST_OK_METHOD: return "AMQP_ACCESS_REQUEST_OK_METHOD";
		case AMQP_EXCHANGE_DECLARE_METHOD: return "AMQP_EXCHANGE_DECLARE_METHOD";
		case AMQP_EXCHANGE_DECLARE_OK_METHOD: return "AMQP_EXCHANGE_DECLARE_OK_METHOD";
		case AMQP_EXCHANGE_DELETE_METHOD: return "AMQP_EXCHANGE_DELETE_METHOD";
		case AMQP_EXCHANGE_DELETE_OK_METHOD: return "AMQP_EXCHANGE_DELETE_OK_METHOD";
		case AMQP_EXCHANGE_BIND_METHOD: return "AMQP_EXCHANGE_BIND_METHOD";
		case AMQP_EXCHANGE_BIND_OK_METHOD: return "AMQP_EXCHANGE_BIND_OK_METHOD";
		case AMQP_EXCHANGE_UNBIND_METHOD: return "AMQP_EXCHANGE_UNBIND_METHOD";
		case AMQP_EXCHANGE_UNBIND_OK_METHOD: return "AMQP_EXCHANGE_UNBIND_OK_METHOD";
		case AMQP_QUEUE_DECLARE_METHOD: return "AMQP_QUEUE_DECLARE_METHOD";
		case AMQP_QUEUE_DECLARE_OK_METHOD: return "AMQP_QUEUE_DECLARE_OK_METHOD";
		case AMQP_QUEUE_BIND_METHOD: return "AMQP_QUEUE_BIND_METHOD";
		case AMQP_QUEUE_BIND_OK_METHOD: return "AMQP_QUEUE_BIND_OK_METHOD";
		case AMQP_QUEUE_PURGE_METHOD: return "AMQP_QUEUE_PURGE_METHOD";
		case AMQP_QUEUE_PURGE_OK_METHOD: return "AMQP_QUEUE_PURGE_OK_METHOD";
		case AMQP_QUEUE_DELETE_METHOD: return "AMQP_QUEUE_DELETE_METHOD";
		case AMQP_QUEUE_DELETE_OK_METHOD: return "AMQP_QUEUE_DELETE_OK_METHOD";
		case AMQP_QUEUE_UNBIND_METHOD: return "AMQP_QUEUE_UNBIND_METHOD";
		case AMQP_QUEUE_UNBIND_OK_METHOD: return "AMQP_QUEUE_UNBIND_OK_METHOD";
		case AMQP_BASIC_QOS_METHOD: return "AMQP_BASIC_QOS_METHOD";
		case AMQP_BASIC_QOS_OK_METHOD: return "AMQP_BASIC_QOS_OK_METHOD";
		case AMQP_BASIC_CONSUME_METHOD: return "AMQP_BASIC_CONSUME_METHOD";
		case AMQP_BASIC_CONSUME_OK_METHOD: return "AMQP_BASIC_CONSUME_OK_METHOD";
		case AMQP_BASIC_CANCEL_METHOD: return "AMQP_BASIC_CANCEL_METHOD";
		case AMQP_BASIC_CANCEL_OK_METHOD: return "AMQP_BASIC_CANCEL_OK_METHOD";
		case AMQP_BASIC_PUBLISH_METHOD: return "AMQP_BASIC_PUBLISH_METHOD";
		case AMQP_BASIC_RETURN_METHOD: return "AMQP_BASIC_RETURN_METHOD";
		case AMQP_BASIC_DELIVER_METHOD: return "AMQP_BASIC_DELIVER_METHOD";
		case AMQP_BASIC_GET_METHOD: return "AMQP_BASIC_GET_METHOD";
		case AMQP_BASIC_GET_OK_METHOD: return "AMQP_BASIC_GET_OK_METHOD";
		case AMQP_BASIC_GET_EMPTY_METHOD: return "AMQP_BASIC_GET_EMPTY_METHOD";
		case AMQP_BASIC_ACK_METHOD: return "AMQP_BASIC_ACK_METHOD";
		case AMQP_BASIC_REJECT_METHOD: return "AMQP_BASIC_REJECT_METHOD";
		case AMQP_BASIC_RECOVER_ASYNC_METHOD: return "AMQP_BASIC_RECOVER_ASYNC_METHOD";
		case AMQP_BASIC_RECOVER_METHOD: return "AMQP_BASIC_RECOVER_METHOD";
		case AMQP_BASIC_RECOVER_OK_METHOD: return "AMQP_BASIC_RECOVER_OK_METHOD";
		case AMQP_BASIC_NACK_METHOD: return "AMQP_BASIC_NACK_METHOD";
		case AMQP_TX_SELECT_METHOD: return "AMQP_TX_SELECT_METHOD";
		case AMQP_TX_SELECT_OK_METHOD: return "AMQP_TX_SELECT_OK_METHOD";
		case AMQP_TX_COMMIT_METHOD: return "AMQP_TX_COMMIT_METHOD";
		case AMQP_TX_COMMIT_OK_METHOD: return "AMQP_TX_COMMIT_OK_METHOD";
		case AMQP_TX_ROLLBACK_METHOD: return "AMQP_TX_ROLLBACK_METHOD";
		case AMQP_TX_ROLLBACK_OK_METHOD: return "AMQP_TX_ROLLBACK_OK_METHOD";
		case AMQP_CONFIRM_SELECT_METHOD: return "AMQP_CONFIRM_SELECT_METHOD";
		case AMQP_CONFIRM_SELECT_OK_METHOD: return "AMQP_CONFIRM_SELECT_OK_METHOD";
		default: return NULL;
	}
}

boolint amqp_method_has_content(amqp_method_number_t methodNumber) 
{
	switch(methodNumber) {
		case AMQP_BASIC_PUBLISH_METHOD: return 1;
		case AMQP_BASIC_RETURN_METHOD: return 1;
		case AMQP_BASIC_DELIVER_METHOD: return 1;
		case AMQP_BASIC_GET_OK_METHOD: return 1;
		default: return 0;
	}
}

int amqp_decode_method(amqp_method_number_t methodNumber, amqp_pool_t * pool, amqp_bytes_t encoded, void ** decoded) 
{
	size_t offset = 0;
	uint8 bit_buffer;
	switch(methodNumber) {
		case AMQP_CONNECTION_START_METHOD: {
		    amqp_connection_start_t * m = static_cast<amqp_connection_start_t *>(amqp_pool_alloc(pool, sizeof(amqp_connection_start_t)));
		    if(!m) {
			    return AMQP_STATUS_NO_MEMORY;
		    }
		    if(!amqp_decode_8(encoded, &offset, &m->version_major))
			    return AMQP_STATUS_BAD_AMQP_DATA;
		    if(!amqp_decode_8(encoded, &offset, &m->version_minor))
			    return AMQP_STATUS_BAD_AMQP_DATA;
		    {
			    int res = amqp_decode_table(encoded, pool, &(m->server_properties), &offset);
			    if(res < 0) return res;
		    }
		    {
			    uint32 len;
			    if(!amqp_decode_32(encoded, &offset, &len) || !amqp_decode_bytes(encoded, &offset, &m->mechanisms, len))
				    return AMQP_STATUS_BAD_AMQP_DATA;
		    }
		    {
			    uint32 len;
			    if(!amqp_decode_32(encoded, &offset, &len) || !amqp_decode_bytes(encoded, &offset, &m->locales, len))
				    return AMQP_STATUS_BAD_AMQP_DATA;
		    }
		    *decoded = m;
		    return 0;
	    }
		case AMQP_CONNECTION_START_OK_METHOD: {
		    amqp_connection_start_ok_t * m = static_cast<amqp_connection_start_ok_t *>(amqp_pool_alloc(pool, sizeof(amqp_connection_start_ok_t)));
		    if(!m) {
			    return AMQP_STATUS_NO_MEMORY;
		    }
		    {
			    int res = amqp_decode_table(encoded, pool, &(m->client_properties), &offset);
			    if(res < 0) 
					return res;
		    }
		    {
			    uint8 len;
			    if(!amqp_decode_8(encoded, &offset, &len) || !amqp_decode_bytes(encoded, &offset, &m->mechanism, len))
				    return AMQP_STATUS_BAD_AMQP_DATA;
		    }
		    {
			    uint32 len;
			    if(!amqp_decode_32(encoded, &offset, &len) || !amqp_decode_bytes(encoded, &offset, &m->response, len))
				    return AMQP_STATUS_BAD_AMQP_DATA;
		    }
		    {
			    uint8 len;
			    if(!amqp_decode_8(encoded, &offset, &len) || !amqp_decode_bytes(encoded, &offset, &m->locale, len))
				    return AMQP_STATUS_BAD_AMQP_DATA;
		    }
		    *decoded = m;
		    return 0;
	    }
		case AMQP_CONNECTION_SECURE_METHOD: {
		    amqp_connection_secure_t * m = static_cast<amqp_connection_secure_t *>(amqp_pool_alloc(pool, sizeof(amqp_connection_secure_t)));
		    if(!m) {
			    return AMQP_STATUS_NO_MEMORY;
		    }
		    {
			    uint32 len;
			    if(!amqp_decode_32(encoded, &offset, &len) || !amqp_decode_bytes(encoded, &offset, &m->challenge, len))
				    return AMQP_STATUS_BAD_AMQP_DATA;
		    }
		    *decoded = m;
		    return 0;
	    }
		case AMQP_CONNECTION_SECURE_OK_METHOD: {
		    amqp_connection_secure_ok_t * m = static_cast<amqp_connection_secure_ok_t *>(amqp_pool_alloc(pool, sizeof(amqp_connection_secure_ok_t)));
		    if(!m) {
			    return AMQP_STATUS_NO_MEMORY;
		    }
		    {
			    uint32 len;
			    if(!amqp_decode_32(encoded, &offset, &len) || !amqp_decode_bytes(encoded, &offset, &m->response, len))
				    return AMQP_STATUS_BAD_AMQP_DATA;
		    }
		    *decoded = m;
		    return 0;
	    }
		case AMQP_CONNECTION_TUNE_METHOD: {
		    amqp_connection_tune_t * m = static_cast<amqp_connection_tune_t *>(amqp_pool_alloc(pool, sizeof(amqp_connection_tune_t)));
		    if(!m) {
			    return AMQP_STATUS_NO_MEMORY;
		    }
		    if(!amqp_decode_16(encoded, &offset, &m->channel_max))
			    return AMQP_STATUS_BAD_AMQP_DATA;
		    if(!amqp_decode_32(encoded, &offset, &m->frame_max))
			    return AMQP_STATUS_BAD_AMQP_DATA;
		    if(!amqp_decode_16(encoded, &offset, &m->heartbeat))
			    return AMQP_STATUS_BAD_AMQP_DATA;
		    *decoded = m;
		    return 0;
	    }
		case AMQP_CONNECTION_TUNE_OK_METHOD: 
			{
				amqp_connection_tune_ok_t * m = static_cast<amqp_connection_tune_ok_t *>(amqp_pool_alloc(pool, sizeof(amqp_connection_tune_ok_t)));
				if(!m) {
					return AMQP_STATUS_NO_MEMORY;
				}
				if(!amqp_decode_16(encoded, &offset, &m->channel_max))
					return AMQP_STATUS_BAD_AMQP_DATA;
				if(!amqp_decode_32(encoded, &offset, &m->frame_max))
					return AMQP_STATUS_BAD_AMQP_DATA;
				if(!amqp_decode_16(encoded, &offset, &m->heartbeat))
					return AMQP_STATUS_BAD_AMQP_DATA;
				*decoded = m;
				return 0;
			}
		case AMQP_CONNECTION_OPEN_METHOD: {
		    amqp_connection_open_t * m = static_cast<amqp_connection_open_t *>(amqp_pool_alloc(pool, sizeof(amqp_connection_open_t)));
		    if(!m) {
			    return AMQP_STATUS_NO_MEMORY;
		    }
		    {
			    uint8 len;
			    if(!amqp_decode_8(encoded, &offset, &len) || !amqp_decode_bytes(encoded, &offset, &m->virtual_host, len))
				    return AMQP_STATUS_BAD_AMQP_DATA;
		    }
		    {
			    uint8 len;
			    if(!amqp_decode_8(encoded, &offset, &len) || !amqp_decode_bytes(encoded, &offset, &m->capabilities, len))
				    return AMQP_STATUS_BAD_AMQP_DATA;
		    }
		    if(!amqp_decode_8(encoded, &offset, &bit_buffer))
			    return AMQP_STATUS_BAD_AMQP_DATA;
		    m->insist = BIN(bit_buffer & (1 << 0));
		    *decoded = m;
		    return 0;
	    }
		case AMQP_CONNECTION_OPEN_OK_METHOD: {
		    amqp_connection_open_ok_t * m = static_cast<amqp_connection_open_ok_t *>(amqp_pool_alloc(pool, sizeof(amqp_connection_open_ok_t)));
		    if(!m) {
			    return AMQP_STATUS_NO_MEMORY;
		    }
		    {
			    uint8 len;
			    if(!amqp_decode_8(encoded, &offset, &len) || !amqp_decode_bytes(encoded, &offset, &m->known_hosts, len))
				    return AMQP_STATUS_BAD_AMQP_DATA;
		    }
		    *decoded = m;
		    return 0;
	    }
		case AMQP_CONNECTION_CLOSE_METHOD: {
		    amqp_connection_close_t * m = static_cast<amqp_connection_close_t *>(amqp_pool_alloc(pool, sizeof(amqp_connection_close_t)));
		    if(!m) {
			    return AMQP_STATUS_NO_MEMORY;
		    }
		    if(!amqp_decode_16(encoded, &offset, &m->reply_code))
			    return AMQP_STATUS_BAD_AMQP_DATA;
		    {
			    uint8 len;
			    if(!amqp_decode_8(encoded, &offset, &len) || !amqp_decode_bytes(encoded, &offset, &m->reply_text, len))
				    return AMQP_STATUS_BAD_AMQP_DATA;
		    }
		    if(!amqp_decode_16(encoded, &offset, &m->class_id))
			    return AMQP_STATUS_BAD_AMQP_DATA;
		    if(!amqp_decode_16(encoded, &offset, &m->method_id))
			    return AMQP_STATUS_BAD_AMQP_DATA;
		    *decoded = m;
		    return 0;
	    }
		case AMQP_CONNECTION_CLOSE_OK_METHOD: 
			{
				amqp_connection_close_ok_t * m = static_cast<amqp_connection_close_ok_t *>(amqp_pool_alloc(pool, sizeof(amqp_connection_close_ok_t)));
				if(!m)
					return AMQP_STATUS_NO_MEMORY;
				else {
					*decoded = m;
					return 0;
				}
			}
		case AMQP_CONNECTION_BLOCKED_METHOD: 
			{
				amqp_connection_blocked_t * m = static_cast<amqp_connection_blocked_t *>(amqp_pool_alloc(pool, sizeof(amqp_connection_blocked_t)));
				if(!m)
					return AMQP_STATUS_NO_MEMORY;
				{
					uint8 len;
					if(!amqp_decode_8(encoded, &offset, &len) || !amqp_decode_bytes(encoded, &offset, &m->reason, len))
						return AMQP_STATUS_BAD_AMQP_DATA;
				}
				*decoded = m;
				return 0;
			}
		case AMQP_CONNECTION_UNBLOCKED_METHOD: 
			{
				amqp_connection_unblocked_t * m = static_cast<amqp_connection_unblocked_t *>(amqp_pool_alloc(pool, sizeof(amqp_connection_unblocked_t)));
				if(!m)
					return AMQP_STATUS_NO_MEMORY;
				*decoded = m;
				return 0;
			}
		case AMQP_CHANNEL_OPEN_METHOD: 
			{
				amqp_channel_open_t * m = static_cast<amqp_channel_open_t *>(amqp_pool_alloc(pool, sizeof(amqp_channel_open_t)));
				if(!m)
					return AMQP_STATUS_NO_MEMORY;
				{
					uint8 len;
					if(!amqp_decode_8(encoded, &offset, &len) || !amqp_decode_bytes(encoded, &offset, &m->out_of_band, len))
						return AMQP_STATUS_BAD_AMQP_DATA;
				}
				*decoded = m;
				return 0;
			}
		case AMQP_CHANNEL_OPEN_OK_METHOD: 
			{
				amqp_channel_open_ok_t * m = static_cast<amqp_channel_open_ok_t *>(amqp_pool_alloc(pool, sizeof(amqp_channel_open_ok_t)));
				if(!m)
					return AMQP_STATUS_NO_MEMORY;
				{
					uint32 len;
					if(!amqp_decode_32(encoded, &offset, &len) || !amqp_decode_bytes(encoded, &offset, &m->channel_id, len))
						return AMQP_STATUS_BAD_AMQP_DATA;
				}
				*decoded = m;
				return 0;
			}
		case AMQP_CHANNEL_FLOW_METHOD: 
			{
				amqp_channel_flow_t * m = static_cast<amqp_channel_flow_t *>(amqp_pool_alloc(pool, sizeof(amqp_channel_flow_t)));
				if(!m)
					return AMQP_STATUS_NO_MEMORY;
				if(!amqp_decode_8(encoded, &offset, &bit_buffer))
					return AMQP_STATUS_BAD_AMQP_DATA;
				m->active = BIN(bit_buffer & (1 << 0));
				*decoded = m;
				return 0;
			}
		case AMQP_CHANNEL_FLOW_OK_METHOD: 
			{
				amqp_channel_flow_ok_t * m = static_cast<amqp_channel_flow_ok_t *>(amqp_pool_alloc(pool, sizeof(amqp_channel_flow_ok_t)));
				if(!m)
					return AMQP_STATUS_NO_MEMORY;
				if(!amqp_decode_8(encoded, &offset, &bit_buffer))
					return AMQP_STATUS_BAD_AMQP_DATA;
				m->active = BIN(bit_buffer & (1 << 0));
				*decoded = m;
				return 0;
			}
		case AMQP_CHANNEL_CLOSE_METHOD: 
			{
				amqp_channel_close_t * m = static_cast<amqp_channel_close_t *>(amqp_pool_alloc(pool, sizeof(amqp_channel_close_t)));
				if(!m)
					return AMQP_STATUS_NO_MEMORY;
				if(!amqp_decode_16(encoded, &offset, &m->reply_code))
					return AMQP_STATUS_BAD_AMQP_DATA;
				{
					uint8 len;
					if(!amqp_decode_8(encoded, &offset, &len) || !amqp_decode_bytes(encoded, &offset, &m->reply_text, len))
						return AMQP_STATUS_BAD_AMQP_DATA;
				}
				if(!amqp_decode_16(encoded, &offset, &m->class_id))
					return AMQP_STATUS_BAD_AMQP_DATA;
				if(!amqp_decode_16(encoded, &offset, &m->method_id))
					return AMQP_STATUS_BAD_AMQP_DATA;
				*decoded = m;
				return 0;
			}
		case AMQP_CHANNEL_CLOSE_OK_METHOD: 
			{
				amqp_channel_close_ok_t * m = static_cast<amqp_channel_close_ok_t *>(amqp_pool_alloc(pool, sizeof(amqp_channel_close_ok_t)));
				if(!m)
					return AMQP_STATUS_NO_MEMORY;
				else {
					*decoded = m;
					return 0;
				}
			}
		case AMQP_ACCESS_REQUEST_METHOD: 
			{
				amqp_access_request_t * m = static_cast<amqp_access_request_t *>(amqp_pool_alloc(pool, sizeof(amqp_access_request_t)));
				if(!m)
					return AMQP_STATUS_NO_MEMORY;
				else {
					{
						uint8 len;
						if(!amqp_decode_8(encoded, &offset, &len) || !amqp_decode_bytes(encoded, &offset, &m->realm, len))
							return AMQP_STATUS_BAD_AMQP_DATA;
					}
					if(!amqp_decode_8(encoded, &offset, &bit_buffer))
						return AMQP_STATUS_BAD_AMQP_DATA;
					m->exclusive = BIN(bit_buffer & (1 << 0));
					m->passive = BIN(bit_buffer & (1 << 1));
					m->active = BIN(bit_buffer & (1 << 2));
					m->write = BIN(bit_buffer & (1 << 3));
					m->read = BIN(bit_buffer & (1 << 4));
					*decoded = m;
					return 0;
				}
			}
		case AMQP_ACCESS_REQUEST_OK_METHOD: 
			{
				amqp_access_request_ok_t * m = static_cast<amqp_access_request_ok_t *>(amqp_pool_alloc(pool, sizeof(amqp_access_request_ok_t)));
				if(!m) {
					return AMQP_STATUS_NO_MEMORY;
				}
				if(!amqp_decode_16(encoded, &offset, &m->ticket))
					return AMQP_STATUS_BAD_AMQP_DATA;
				*decoded = m;
				return 0;
			}
		case AMQP_EXCHANGE_DECLARE_METHOD: 
			{
				amqp_exchange_declare_t * m = static_cast<amqp_exchange_declare_t *>(amqp_pool_alloc(pool, sizeof(amqp_exchange_declare_t)));
				if(!m) {
					return AMQP_STATUS_NO_MEMORY;
				}
				if(!amqp_decode_16(encoded, &offset, &m->ticket))
					return AMQP_STATUS_BAD_AMQP_DATA;
				{
					uint8 len;
					if(!amqp_decode_8(encoded, &offset, &len) || !amqp_decode_bytes(encoded, &offset, &m->exchange, len))
						return AMQP_STATUS_BAD_AMQP_DATA;
				}
				{
					uint8 len;
					if(!amqp_decode_8(encoded, &offset, &len) || !amqp_decode_bytes(encoded, &offset, &m->type, len))
						return AMQP_STATUS_BAD_AMQP_DATA;
				}
				if(!amqp_decode_8(encoded, &offset, &bit_buffer))
					return AMQP_STATUS_BAD_AMQP_DATA;
				m->passive = BIN(bit_buffer & (1 << 0));
				m->durable = BIN(bit_buffer & (1 << 1));
				m->auto_delete = BIN(bit_buffer & (1 << 2));
				m->internal = BIN(bit_buffer & (1 << 3));
				m->nowait = BIN(bit_buffer & (1 << 4));
				{
					int res = amqp_decode_table(encoded, pool, &(m->arguments), &offset);
					if(res < 0) return res;
				}
				*decoded = m;
				return 0;
			}
		case AMQP_EXCHANGE_DECLARE_OK_METHOD: 
			{
				amqp_exchange_declare_ok_t * m = static_cast<amqp_exchange_declare_ok_t *>(amqp_pool_alloc(pool, sizeof(amqp_exchange_declare_ok_t)));
				if(!m) {
					return AMQP_STATUS_NO_MEMORY;
				}
				*decoded = m;
				return 0;
			}
		case AMQP_EXCHANGE_DELETE_METHOD: 
			{
				amqp_exchange_delete_t * m = static_cast<amqp_exchange_delete_t *>(amqp_pool_alloc(pool, sizeof(amqp_exchange_delete_t)));
				if(!m) {
					return AMQP_STATUS_NO_MEMORY;
				}
				if(!amqp_decode_16(encoded, &offset, &m->ticket))
					return AMQP_STATUS_BAD_AMQP_DATA;
				{
					uint8 len;
					if(!amqp_decode_8(encoded, &offset, &len) || !amqp_decode_bytes(encoded, &offset, &m->exchange, len))
						return AMQP_STATUS_BAD_AMQP_DATA;
				}
				if(!amqp_decode_8(encoded, &offset, &bit_buffer))
					return AMQP_STATUS_BAD_AMQP_DATA;
				m->if_unused = BIN(bit_buffer & (1 << 0));
				m->nowait = BIN(bit_buffer & (1 << 1));
				*decoded = m;
				return 0;
			}
		case AMQP_EXCHANGE_DELETE_OK_METHOD: 
			{
				amqp_exchange_delete_ok_t * m = static_cast<amqp_exchange_delete_ok_t *>(amqp_pool_alloc(pool, sizeof(amqp_exchange_delete_ok_t)));
				if(!m)
					return AMQP_STATUS_NO_MEMORY;
				else {
					*decoded = m;
					return 0;
				}
			}
		case AMQP_EXCHANGE_BIND_METHOD: 
			{
				amqp_exchange_bind_t * m = static_cast<amqp_exchange_bind_t *>(amqp_pool_alloc(pool, sizeof(amqp_exchange_bind_t)));
				if(!m)
					return AMQP_STATUS_NO_MEMORY;
				else if(!amqp_decode_16(encoded, &offset, &m->ticket))
					return AMQP_STATUS_BAD_AMQP_DATA;
				else {
					{
						uint8 len;
						if(!amqp_decode_8(encoded, &offset, &len) || !amqp_decode_bytes(encoded, &offset, &m->destination, len))
							return AMQP_STATUS_BAD_AMQP_DATA;
					}
					{
						uint8 len;
						if(!amqp_decode_8(encoded, &offset, &len) || !amqp_decode_bytes(encoded, &offset, &m->source, len))
							return AMQP_STATUS_BAD_AMQP_DATA;
					}
					{
						uint8 len;
						if(!amqp_decode_8(encoded, &offset, &len) || !amqp_decode_bytes(encoded, &offset, &m->routing_key, len))
							return AMQP_STATUS_BAD_AMQP_DATA;
					}
					if(!amqp_decode_8(encoded, &offset, &bit_buffer))
						return AMQP_STATUS_BAD_AMQP_DATA;
					m->nowait = BIN(bit_buffer & (1 << 0));
					{
						int res = amqp_decode_table(encoded, pool, &(m->arguments), &offset);
						if(res < 0) return res;
					}
					*decoded = m;
					return 0;
				}
			}
		case AMQP_EXCHANGE_BIND_OK_METHOD: 
			{
				amqp_exchange_bind_ok_t * m = static_cast<amqp_exchange_bind_ok_t *>(amqp_pool_alloc(pool, sizeof(amqp_exchange_bind_ok_t)));
				if(!m) {
					return AMQP_STATUS_NO_MEMORY;
				}
				*decoded = m;
				return 0;
			}
		case AMQP_EXCHANGE_UNBIND_METHOD: {
		    amqp_exchange_unbind_t * m = static_cast<amqp_exchange_unbind_t *>(amqp_pool_alloc(pool, sizeof(amqp_exchange_unbind_t)));
		    if(!m) {
			    return AMQP_STATUS_NO_MEMORY;
		    }
		    if(!amqp_decode_16(encoded, &offset, &m->ticket))
			    return AMQP_STATUS_BAD_AMQP_DATA;
		    {
			    uint8 len;
			    if(!amqp_decode_8(encoded, &offset, &len) || !amqp_decode_bytes(encoded, &offset, &m->destination, len))
				    return AMQP_STATUS_BAD_AMQP_DATA;
		    }
		    {
			    uint8 len;
			    if(!amqp_decode_8(encoded, &offset, &len) || !amqp_decode_bytes(encoded, &offset, &m->source, len))
				    return AMQP_STATUS_BAD_AMQP_DATA;
		    }
		    {
			    uint8 len;
			    if(!amqp_decode_8(encoded, &offset, &len) || !amqp_decode_bytes(encoded, &offset, &m->routing_key, len))
				    return AMQP_STATUS_BAD_AMQP_DATA;
		    }
		    if(!amqp_decode_8(encoded, &offset, &bit_buffer))
			    return AMQP_STATUS_BAD_AMQP_DATA;
		    m->nowait = BIN(bit_buffer & (1 << 0));
		    {
			    int res = amqp_decode_table(encoded, pool, &(m->arguments), &offset);
			    if(res < 0) return res;
		    }
		    *decoded = m;
		    return 0;
	    }
		case AMQP_EXCHANGE_UNBIND_OK_METHOD: 
			{
				amqp_exchange_unbind_ok_t * m = static_cast<amqp_exchange_unbind_ok_t *>(amqp_pool_alloc(pool, sizeof(amqp_exchange_unbind_ok_t)));
				if(!m)
					return AMQP_STATUS_NO_MEMORY;
				else {
					*decoded = m;
					return 0;
				}
			}
		case AMQP_QUEUE_DECLARE_METHOD: 
			{
				amqp_queue_declare_t * m = static_cast<amqp_queue_declare_t *>(amqp_pool_alloc(pool, sizeof(amqp_queue_declare_t)));
				if(!m)
					return AMQP_STATUS_NO_MEMORY;
				else if(!amqp_decode_16(encoded, &offset, &m->ticket))
					return AMQP_STATUS_BAD_AMQP_DATA;
				else {
					{
						uint8 len;
						if(!amqp_decode_8(encoded, &offset, &len) || !amqp_decode_bytes(encoded, &offset, &m->queue, len))
							return AMQP_STATUS_BAD_AMQP_DATA;
					}
					if(!amqp_decode_8(encoded, &offset, &bit_buffer))
						return AMQP_STATUS_BAD_AMQP_DATA;
					m->passive = BIN(bit_buffer & (1 << 0));
					m->durable = BIN(bit_buffer & (1 << 1));
					m->exclusive = BIN(bit_buffer & (1 << 2));
					m->auto_delete = BIN(bit_buffer & (1 << 3));
					m->nowait = BIN(bit_buffer & (1 << 4));
					{
						int res = amqp_decode_table(encoded, pool, &(m->arguments), &offset);
						if(res < 0) return res;
					}
					*decoded = m;
					return 0;
				}
			}
		case AMQP_QUEUE_DECLARE_OK_METHOD: {
		    amqp_queue_declare_ok_t * m = (amqp_queue_declare_ok_t*)amqp_pool_alloc(pool, sizeof(amqp_queue_declare_ok_t));
		    if(!m) {
			    return AMQP_STATUS_NO_MEMORY;
		    }
		    {
			    uint8 len;
			    if(!amqp_decode_8(encoded, &offset, &len) || !amqp_decode_bytes(encoded, &offset, &m->queue, len))
				    return AMQP_STATUS_BAD_AMQP_DATA;
		    }
		    if(!amqp_decode_32(encoded, &offset, &m->message_count))
			    return AMQP_STATUS_BAD_AMQP_DATA;
		    if(!amqp_decode_32(encoded, &offset, &m->consumer_count))
			    return AMQP_STATUS_BAD_AMQP_DATA;
		    *decoded = m;
		    return 0;
	    }
		case AMQP_QUEUE_BIND_METHOD: 
			{
				amqp_queue_bind_t * m = static_cast<amqp_queue_bind_t *>(amqp_pool_alloc(pool, sizeof(amqp_queue_bind_t)));
				if(!m) {
					return AMQP_STATUS_NO_MEMORY;
				}
				if(!amqp_decode_16(encoded, &offset, &m->ticket))
					return AMQP_STATUS_BAD_AMQP_DATA;
				{
					uint8 len;
					if(!amqp_decode_8(encoded, &offset, &len) || !amqp_decode_bytes(encoded, &offset, &m->queue, len))
						return AMQP_STATUS_BAD_AMQP_DATA;
				}
				{
					uint8 len;
					if(!amqp_decode_8(encoded, &offset, &len) || !amqp_decode_bytes(encoded, &offset, &m->exchange, len))
						return AMQP_STATUS_BAD_AMQP_DATA;
				}
				{
					uint8 len;
					if(!amqp_decode_8(encoded, &offset, &len) || !amqp_decode_bytes(encoded, &offset, &m->routing_key, len))
						return AMQP_STATUS_BAD_AMQP_DATA;
				}
				if(!amqp_decode_8(encoded, &offset, &bit_buffer))
					return AMQP_STATUS_BAD_AMQP_DATA;
				m->nowait = BIN(bit_buffer & (1 << 0));
				{
					int res = amqp_decode_table(encoded, pool, &(m->arguments), &offset);
					if(res < 0) 
						return res;
				}
				*decoded = m;
				return 0;
			}
		case AMQP_QUEUE_BIND_OK_METHOD: 
			{
				amqp_queue_bind_ok_t * m = static_cast<amqp_queue_bind_ok_t *>(amqp_pool_alloc(pool, sizeof(amqp_queue_bind_ok_t)));
				if(!m) {
					return AMQP_STATUS_NO_MEMORY;
				}
				*decoded = m;
				return 0;
			}
		case AMQP_QUEUE_PURGE_METHOD: 
			{
				amqp_queue_purge_t * m = static_cast<amqp_queue_purge_t *>(amqp_pool_alloc(pool, sizeof(amqp_queue_purge_t)));
				if(!m) {
					return AMQP_STATUS_NO_MEMORY;
				}
				if(!amqp_decode_16(encoded, &offset, &m->ticket))
					return AMQP_STATUS_BAD_AMQP_DATA;
				{
					uint8 len;
					if(!amqp_decode_8(encoded, &offset, &len) || !amqp_decode_bytes(encoded, &offset, &m->queue, len))
						return AMQP_STATUS_BAD_AMQP_DATA;
				}
				if(!amqp_decode_8(encoded, &offset, &bit_buffer))
					return AMQP_STATUS_BAD_AMQP_DATA;
				m->nowait = BIN(bit_buffer & (1 << 0));
				*decoded = m;
				return 0;
			}
		case AMQP_QUEUE_PURGE_OK_METHOD: 
			{
				amqp_queue_purge_ok_t * m = static_cast<amqp_queue_purge_ok_t *>(amqp_pool_alloc(pool, sizeof(amqp_queue_purge_ok_t)));
				if(!m) {
					return AMQP_STATUS_NO_MEMORY;
				}
				if(!amqp_decode_32(encoded, &offset, &m->message_count))
					return AMQP_STATUS_BAD_AMQP_DATA;
				*decoded = m;
				return 0;
			}
		case AMQP_QUEUE_DELETE_METHOD: 
			{
				amqp_queue_delete_t * m = static_cast<amqp_queue_delete_t *>(amqp_pool_alloc(pool, sizeof(amqp_queue_delete_t)));
				if(!m) {
					return AMQP_STATUS_NO_MEMORY;
				}
				if(!amqp_decode_16(encoded, &offset, &m->ticket))
					return AMQP_STATUS_BAD_AMQP_DATA;
				{
					uint8 len;
					if(!amqp_decode_8(encoded, &offset, &len) || !amqp_decode_bytes(encoded, &offset, &m->queue, len))
						return AMQP_STATUS_BAD_AMQP_DATA;
				}
				if(!amqp_decode_8(encoded, &offset, &bit_buffer))
					return AMQP_STATUS_BAD_AMQP_DATA;
				m->if_unused = BIN(bit_buffer & (1 << 0));
				m->if_empty = BIN(bit_buffer & (1 << 1));
				m->nowait = BIN(bit_buffer & (1 << 2));
				*decoded = m;
				return 0;
			}
		case AMQP_QUEUE_DELETE_OK_METHOD: 
			{
				amqp_queue_delete_ok_t * m = static_cast<amqp_queue_delete_ok_t *>(amqp_pool_alloc(pool, sizeof(amqp_queue_delete_ok_t)));
				if(!m) {
					return AMQP_STATUS_NO_MEMORY;
				}
				if(!amqp_decode_32(encoded, &offset, &m->message_count))
					return AMQP_STATUS_BAD_AMQP_DATA;
				*decoded = m;
				return 0;
			}
		case AMQP_QUEUE_UNBIND_METHOD: 
			{
				amqp_queue_unbind_t * m = static_cast<amqp_queue_unbind_t *>(amqp_pool_alloc(pool, sizeof(amqp_queue_unbind_t)));
				if(!m) {
					return AMQP_STATUS_NO_MEMORY;
				}
				if(!amqp_decode_16(encoded, &offset, &m->ticket))
					return AMQP_STATUS_BAD_AMQP_DATA;
				{
					uint8 len;
					if(!amqp_decode_8(encoded, &offset, &len) || !amqp_decode_bytes(encoded, &offset, &m->queue, len))
						return AMQP_STATUS_BAD_AMQP_DATA;
				}
				{
					uint8 len;
					if(!amqp_decode_8(encoded, &offset, &len) || !amqp_decode_bytes(encoded, &offset, &m->exchange, len))
						return AMQP_STATUS_BAD_AMQP_DATA;
				}
				{
					uint8 len;
					if(!amqp_decode_8(encoded, &offset, &len) || !amqp_decode_bytes(encoded, &offset, &m->routing_key, len))
						return AMQP_STATUS_BAD_AMQP_DATA;
				}
				{
					int res = amqp_decode_table(encoded, pool, &(m->arguments), &offset);
					if(res < 0) 
						return res;
				}
				*decoded = m;
				return 0;
			}
		case AMQP_QUEUE_UNBIND_OK_METHOD: 
			{
				amqp_queue_unbind_ok_t * m = static_cast<amqp_queue_unbind_ok_t *>(amqp_pool_alloc(pool, sizeof(amqp_queue_unbind_ok_t)));
				if(!m) {
					return AMQP_STATUS_NO_MEMORY;
				}
				*decoded = m;
				return 0;
			}
		case AMQP_BASIC_QOS_METHOD: 
			{
				amqp_basic_qos_t * m = static_cast<amqp_basic_qos_t *>(amqp_pool_alloc(pool, sizeof(amqp_basic_qos_t)));
				if(!m) {
					return AMQP_STATUS_NO_MEMORY;
				}
				if(!amqp_decode_32(encoded, &offset, &m->prefetch_size))
					return AMQP_STATUS_BAD_AMQP_DATA;
				if(!amqp_decode_16(encoded, &offset, &m->prefetch_count))
					return AMQP_STATUS_BAD_AMQP_DATA;
				if(!amqp_decode_8(encoded, &offset, &bit_buffer))
					return AMQP_STATUS_BAD_AMQP_DATA;
				m->global = BIN(bit_buffer & (1 << 0));
				*decoded = m;
				return 0;
			}
		case AMQP_BASIC_QOS_OK_METHOD: 
			{
				amqp_basic_qos_ok_t * m = static_cast<amqp_basic_qos_ok_t *>(amqp_pool_alloc(pool, sizeof(amqp_basic_qos_ok_t)));
				if(!m) {
					return AMQP_STATUS_NO_MEMORY;
				}
				*decoded = m;
				return 0;
			}
		case AMQP_BASIC_CONSUME_METHOD: 
			{
				amqp_basic_consume_t * m = static_cast<amqp_basic_consume_t *>(amqp_pool_alloc(pool, sizeof(amqp_basic_consume_t)));
				if(!m) {
					return AMQP_STATUS_NO_MEMORY;
				}
				if(!amqp_decode_16(encoded, &offset, &m->ticket))
					return AMQP_STATUS_BAD_AMQP_DATA;
				{
					uint8 len;
					if(!amqp_decode_8(encoded, &offset, &len) || !amqp_decode_bytes(encoded, &offset, &m->queue, len))
						return AMQP_STATUS_BAD_AMQP_DATA;
				}
				{
					uint8 len;
					if(!amqp_decode_8(encoded, &offset, &len) || !amqp_decode_bytes(encoded, &offset, &m->consumer_tag, len))
						return AMQP_STATUS_BAD_AMQP_DATA;
				}
				if(!amqp_decode_8(encoded, &offset, &bit_buffer))
					return AMQP_STATUS_BAD_AMQP_DATA;
				m->no_local  = BIN(bit_buffer & (1 << 0));
				m->no_ack    = BIN(bit_buffer & (1 << 1));
				m->exclusive = BIN(bit_buffer & (1 << 2));
				m->nowait    = BIN(bit_buffer & (1 << 3));
				{
					int res = amqp_decode_table(encoded, pool, &(m->arguments), &offset);
					if(res < 0) 
						return res;
				}
				*decoded = m;
				return 0;
			}
		case AMQP_BASIC_CONSUME_OK_METHOD: 
			{
				amqp_basic_consume_ok_t * m = static_cast<amqp_basic_consume_ok_t *>(amqp_pool_alloc(pool, sizeof(amqp_basic_consume_ok_t)));
				if(!m) {
					return AMQP_STATUS_NO_MEMORY;
				}
				{
					uint8 len;
					if(!amqp_decode_8(encoded, &offset, &len) ||
					!amqp_decode_bytes(encoded, &offset, &m->consumer_tag, len))
						return AMQP_STATUS_BAD_AMQP_DATA;
				}
				*decoded = m;
				return 0;
			}
		case AMQP_BASIC_CANCEL_METHOD: 
			{
				amqp_basic_cancel_t * m = static_cast<amqp_basic_cancel_t *>(amqp_pool_alloc(pool, sizeof(amqp_basic_cancel_t)));
				if(!m) {
					return AMQP_STATUS_NO_MEMORY;
				}
				{
					uint8 len;
					if(!amqp_decode_8(encoded, &offset, &len) || !amqp_decode_bytes(encoded, &offset, &m->consumer_tag, len))
						return AMQP_STATUS_BAD_AMQP_DATA;
				}
				if(!amqp_decode_8(encoded, &offset, &bit_buffer))
					return AMQP_STATUS_BAD_AMQP_DATA;
				m->nowait = BIN(bit_buffer & (1 << 0));
				*decoded = m;
				return 0;
			}
		case AMQP_BASIC_CANCEL_OK_METHOD: 
			{
				amqp_basic_cancel_ok_t * m = static_cast<amqp_basic_cancel_ok_t *>(amqp_pool_alloc(pool, sizeof(amqp_basic_cancel_ok_t)));
				if(!m) {
					return AMQP_STATUS_NO_MEMORY;
				}
				{
					uint8 len;
					if(!amqp_decode_8(encoded, &offset, &len) || !amqp_decode_bytes(encoded, &offset, &m->consumer_tag, len))
						return AMQP_STATUS_BAD_AMQP_DATA;
				}
				*decoded = m;
				return 0;
			}
		case AMQP_BASIC_PUBLISH_METHOD: 
			{
				amqp_basic_publish_t * m = static_cast<amqp_basic_publish_t *>(amqp_pool_alloc(pool, sizeof(amqp_basic_publish_t)));
				if(!m) {
					return AMQP_STATUS_NO_MEMORY;
				}
				if(!amqp_decode_16(encoded, &offset, &m->ticket))
					return AMQP_STATUS_BAD_AMQP_DATA;
				{
					uint8 len;
					if(!amqp_decode_8(encoded, &offset, &len) || !amqp_decode_bytes(encoded, &offset, &m->exchange, len))
						return AMQP_STATUS_BAD_AMQP_DATA;
				}
				{
					uint8 len;
					if(!amqp_decode_8(encoded, &offset, &len) || !amqp_decode_bytes(encoded, &offset, &m->routing_key, len))
						return AMQP_STATUS_BAD_AMQP_DATA;
				}
				if(!amqp_decode_8(encoded, &offset, &bit_buffer))
					return AMQP_STATUS_BAD_AMQP_DATA;
				m->mandatory = BIN(bit_buffer & (1 << 0));
				m->immediate = BIN(bit_buffer & (1 << 1));
				*decoded = m;
				return 0;
			}
		case AMQP_BASIC_RETURN_METHOD: 
			{
				amqp_basic_return_t * m = static_cast<amqp_basic_return_t *>(amqp_pool_alloc(pool, sizeof(amqp_basic_return_t)));
				if(!m) {
					return AMQP_STATUS_NO_MEMORY;
				}
				if(!amqp_decode_16(encoded, &offset, &m->reply_code))
					return AMQP_STATUS_BAD_AMQP_DATA;
				{
					uint8 len;
					if(!amqp_decode_8(encoded, &offset, &len) || !amqp_decode_bytes(encoded, &offset, &m->reply_text, len))
						return AMQP_STATUS_BAD_AMQP_DATA;
				}
				{
					uint8 len;
					if(!amqp_decode_8(encoded, &offset, &len) || !amqp_decode_bytes(encoded, &offset, &m->exchange, len))
						return AMQP_STATUS_BAD_AMQP_DATA;
				}
				{
					uint8 len;
					if(!amqp_decode_8(encoded, &offset, &len) || !amqp_decode_bytes(encoded, &offset, &m->routing_key, len))
						return AMQP_STATUS_BAD_AMQP_DATA;
				}
				*decoded = m;
				return 0;
			}
		case AMQP_BASIC_DELIVER_METHOD: 
			{
				amqp_basic_deliver_t * m = static_cast<amqp_basic_deliver_t *>(amqp_pool_alloc(pool, sizeof(amqp_basic_deliver_t)));
				if(!m) {
					return AMQP_STATUS_NO_MEMORY;
				}
				{
					uint8 len;
					if(!amqp_decode_8(encoded, &offset, &len) || !amqp_decode_bytes(encoded, &offset, &m->consumer_tag, len))
						return AMQP_STATUS_BAD_AMQP_DATA;
				}
				if(!amqp_decode_64(encoded, &offset, &m->delivery_tag))
					return AMQP_STATUS_BAD_AMQP_DATA;
				if(!amqp_decode_8(encoded, &offset, &bit_buffer))
					return AMQP_STATUS_BAD_AMQP_DATA;
				m->redelivered = BIN(bit_buffer & (1 << 0));
				{
					uint8 len;
					if(!amqp_decode_8(encoded, &offset, &len) || !amqp_decode_bytes(encoded, &offset, &m->exchange, len))
						return AMQP_STATUS_BAD_AMQP_DATA;
				}
				{
					uint8 len;
					if(!amqp_decode_8(encoded, &offset, &len) || !amqp_decode_bytes(encoded, &offset, &m->routing_key, len))
						return AMQP_STATUS_BAD_AMQP_DATA;
				}
				*decoded = m;
				return 0;
			}
		case AMQP_BASIC_GET_METHOD: 
			{
				amqp_basic_get_t * m = static_cast<amqp_basic_get_t *>(amqp_pool_alloc(pool, sizeof(amqp_basic_get_t)));
				if(!m) {
					return AMQP_STATUS_NO_MEMORY;
				}
				if(!amqp_decode_16(encoded, &offset, &m->ticket))
					return AMQP_STATUS_BAD_AMQP_DATA;
				{
					uint8 len;
					if(!amqp_decode_8(encoded, &offset, &len) || !amqp_decode_bytes(encoded, &offset, &m->queue, len))
						return AMQP_STATUS_BAD_AMQP_DATA;
				}
				if(!amqp_decode_8(encoded, &offset, &bit_buffer))
					return AMQP_STATUS_BAD_AMQP_DATA;
				m->no_ack = BIN(bit_buffer & (1 << 0));
				*decoded = m;
				return 0;
			}
		case AMQP_BASIC_GET_OK_METHOD: 
			{
				amqp_basic_get_ok_t * m = static_cast<amqp_basic_get_ok_t *>(amqp_pool_alloc(pool, sizeof(amqp_basic_get_ok_t)));
				if(!m) {
					return AMQP_STATUS_NO_MEMORY;
				}
				if(!amqp_decode_64(encoded, &offset, &m->delivery_tag))
					return AMQP_STATUS_BAD_AMQP_DATA;
				if(!amqp_decode_8(encoded, &offset, &bit_buffer))
					return AMQP_STATUS_BAD_AMQP_DATA;
				m->redelivered = BIN(bit_buffer & (1 << 0));
				{
					uint8 len;
					if(!amqp_decode_8(encoded, &offset, &len) || !amqp_decode_bytes(encoded, &offset, &m->exchange, len))
						return AMQP_STATUS_BAD_AMQP_DATA;
				}
				{
					uint8 len;
					if(!amqp_decode_8(encoded, &offset, &len) || !amqp_decode_bytes(encoded, &offset, &m->routing_key, len))
						return AMQP_STATUS_BAD_AMQP_DATA;
				}
				if(!amqp_decode_32(encoded, &offset, &m->message_count))
					return AMQP_STATUS_BAD_AMQP_DATA;
				*decoded = m;
				return 0;
			}
		case AMQP_BASIC_GET_EMPTY_METHOD: 
			{
				amqp_basic_get_empty_t * m = static_cast<amqp_basic_get_empty_t *>(amqp_pool_alloc(pool, sizeof(amqp_basic_get_empty_t)));
				if(!m) {
					return AMQP_STATUS_NO_MEMORY;
				}
				{
					uint8 len;
					if(!amqp_decode_8(encoded, &offset, &len) || !amqp_decode_bytes(encoded, &offset, &m->cluster_id, len))
						return AMQP_STATUS_BAD_AMQP_DATA;
				}
				*decoded = m;
				return 0;
			}
		case AMQP_BASIC_ACK_METHOD: 
			{
				amqp_basic_ack_t * m = static_cast<amqp_basic_ack_t *>(amqp_pool_alloc(pool, sizeof(amqp_basic_ack_t)));
				if(!m) {
					return AMQP_STATUS_NO_MEMORY;
				}
				if(!amqp_decode_64(encoded, &offset, &m->delivery_tag))
					return AMQP_STATUS_BAD_AMQP_DATA;
				if(!amqp_decode_8(encoded, &offset, &bit_buffer))
					return AMQP_STATUS_BAD_AMQP_DATA;
				m->multiple = BIN(bit_buffer & (1 << 0));
				*decoded = m;
				return 0;
			}
		case AMQP_BASIC_REJECT_METHOD: 
			{
				amqp_basic_reject_t * m = static_cast<amqp_basic_reject_t *>(amqp_pool_alloc(pool, sizeof(amqp_basic_reject_t)));
				if(!m) {
					return AMQP_STATUS_NO_MEMORY;
				}
				if(!amqp_decode_64(encoded, &offset, &m->delivery_tag))
					return AMQP_STATUS_BAD_AMQP_DATA;
				if(!amqp_decode_8(encoded, &offset, &bit_buffer))
					return AMQP_STATUS_BAD_AMQP_DATA;
				m->requeue = BIN(bit_buffer & (1 << 0));
				*decoded = m;
				return 0;
			}
		case AMQP_BASIC_RECOVER_ASYNC_METHOD: 
			{
				amqp_basic_recover_async_t * m = static_cast<amqp_basic_recover_async_t *>(amqp_pool_alloc(pool, sizeof(amqp_basic_recover_async_t)));
				if(!m) {
					return AMQP_STATUS_NO_MEMORY;
				}
				if(!amqp_decode_8(encoded, &offset, &bit_buffer))
					return AMQP_STATUS_BAD_AMQP_DATA;
				m->requeue = BIN(bit_buffer & (1 << 0));
				*decoded = m;
				return 0;
			}
		case AMQP_BASIC_RECOVER_METHOD: 
			{
				amqp_basic_recover_t * m = static_cast<amqp_basic_recover_t *>(amqp_pool_alloc(pool, sizeof(amqp_basic_recover_t)));
				if(!m) {
					return AMQP_STATUS_NO_MEMORY;
				}
				if(!amqp_decode_8(encoded, &offset, &bit_buffer))
					return AMQP_STATUS_BAD_AMQP_DATA;
				m->requeue = BIN(bit_buffer & (1 << 0));
				*decoded = m;
				return 0;
			}
		case AMQP_BASIC_RECOVER_OK_METHOD: 
			{
				amqp_basic_recover_ok_t * m = static_cast<amqp_basic_recover_ok_t *>(amqp_pool_alloc(pool, sizeof(amqp_basic_recover_ok_t)));
				if(!m) {
					return AMQP_STATUS_NO_MEMORY;
				}
				*decoded = m;
				return 0;
			}
		case AMQP_BASIC_NACK_METHOD: 
			{
				amqp_basic_nack_t * m = static_cast<amqp_basic_nack_t *>(amqp_pool_alloc(pool, sizeof(amqp_basic_nack_t)));
				if(!m) {
					return AMQP_STATUS_NO_MEMORY;
				}
				if(!amqp_decode_64(encoded, &offset, &m->delivery_tag))
					return AMQP_STATUS_BAD_AMQP_DATA;
				if(!amqp_decode_8(encoded, &offset, &bit_buffer))
					return AMQP_STATUS_BAD_AMQP_DATA;
				m->multiple = BIN(bit_buffer & (1 << 0));
				m->requeue = BIN(bit_buffer & (1 << 1));
				*decoded = m;
				return 0;
			}
		case AMQP_TX_SELECT_METHOD: 
			{
				amqp_tx_select_t * m = static_cast<amqp_tx_select_t *>(amqp_pool_alloc(pool, sizeof(amqp_tx_select_t)));
				if(!m) {
					return AMQP_STATUS_NO_MEMORY;
				}
				*decoded = m;
				return 0;
			}
		case AMQP_TX_SELECT_OK_METHOD: 
			{
				amqp_tx_select_ok_t * m = static_cast<amqp_tx_select_ok_t *>(amqp_pool_alloc(pool, sizeof(amqp_tx_select_ok_t)));
				if(!m) {
					return AMQP_STATUS_NO_MEMORY;
				}
				*decoded = m;
				return 0;
			}
		case AMQP_TX_COMMIT_METHOD: 
			{
				amqp_tx_commit_t * m = static_cast<amqp_tx_commit_t *>(amqp_pool_alloc(pool, sizeof(amqp_tx_commit_t)));
				if(!m) {
					return AMQP_STATUS_NO_MEMORY;
				}
				*decoded = m;
				return 0;
			}
		case AMQP_TX_COMMIT_OK_METHOD: 
			{
				amqp_tx_commit_ok_t * m = static_cast<amqp_tx_commit_ok_t *>(amqp_pool_alloc(pool, sizeof(amqp_tx_commit_ok_t)));
				if(!m) {
					return AMQP_STATUS_NO_MEMORY;
				}
				*decoded = m;
				return 0;
			}
		case AMQP_TX_ROLLBACK_METHOD: 
			{
				amqp_tx_rollback_t * m = static_cast<amqp_tx_rollback_t *>(amqp_pool_alloc(pool, sizeof(amqp_tx_rollback_t)));
				if(!m) {
					return AMQP_STATUS_NO_MEMORY;
				}
				*decoded = m;
				return 0;
			}
		case AMQP_TX_ROLLBACK_OK_METHOD: 
			{
				amqp_tx_rollback_ok_t * m = static_cast<amqp_tx_rollback_ok_t *>(amqp_pool_alloc(pool, sizeof(amqp_tx_rollback_ok_t)));
				if(!m) {
					return AMQP_STATUS_NO_MEMORY;
				}
				*decoded = m;
				return 0;
			}
		case AMQP_CONFIRM_SELECT_METHOD: 
			{
				amqp_confirm_select_t * m = static_cast<amqp_confirm_select_t *>(amqp_pool_alloc(pool, sizeof(amqp_confirm_select_t)));
				if(!m) {
					return AMQP_STATUS_NO_MEMORY;
				}
				if(!amqp_decode_8(encoded, &offset, &bit_buffer))
					return AMQP_STATUS_BAD_AMQP_DATA;
				m->nowait = BIN(bit_buffer & (1 << 0));
				*decoded = m;
				return 0;
			}
		case AMQP_CONFIRM_SELECT_OK_METHOD: 
			{
				amqp_confirm_select_ok_t * m = static_cast<amqp_confirm_select_ok_t *>(amqp_pool_alloc(pool, sizeof(amqp_confirm_select_ok_t)));
				if(!m) {
					return AMQP_STATUS_NO_MEMORY;
				}
				*decoded = m;
				return 0;
			}
		default:
		    return AMQP_STATUS_UNKNOWN_METHOD;
	}
}

int amqp_decode_properties(uint16 class_id, amqp_pool_t * pool, amqp_bytes_t encoded, void ** decoded) 
{
	size_t offset = 0;
	amqp_flags_t flags = 0;
	int flagword_index = 0;
	uint16 partial_flags;
	do {
		if(!amqp_decode_16(encoded, &offset, &partial_flags))
			return AMQP_STATUS_BAD_AMQP_DATA;
		flags |= (partial_flags << (flagword_index * 16));
		flagword_index++;
	} while(partial_flags & 1);
	switch(class_id) {
		case 10: {
		    amqp_connection_properties_t * p = (amqp_connection_properties_t*)amqp_pool_alloc(pool, sizeof(amqp_connection_properties_t));
		    if(!p) {
			    return AMQP_STATUS_NO_MEMORY;
		    }
		    p->_flags = flags;
		    *decoded = p;
		    return 0;
	    }
		case 20: {
		    amqp_channel_properties_t * p = (amqp_channel_properties_t*)amqp_pool_alloc(pool, sizeof(amqp_channel_properties_t));
		    if(!p) {
			    return AMQP_STATUS_NO_MEMORY;
		    }
		    p->_flags = flags;
		    *decoded = p;
		    return 0;
	    }
		case 30: {
		    amqp_access_properties_t * p = (amqp_access_properties_t*)amqp_pool_alloc(pool, sizeof(amqp_access_properties_t));
		    if(!p) {
			    return AMQP_STATUS_NO_MEMORY;
		    }
		    p->_flags = flags;
		    *decoded = p;
		    return 0;
	    }
		case 40: {
		    amqp_exchange_properties_t * p = (amqp_exchange_properties_t*)amqp_pool_alloc(pool, sizeof(amqp_exchange_properties_t));
		    if(!p) {
			    return AMQP_STATUS_NO_MEMORY;
		    }
		    p->_flags = flags;
		    *decoded = p;
		    return 0;
	    }
		case 50: {
		    amqp_queue_properties_t * p = (amqp_queue_properties_t*)amqp_pool_alloc(pool, sizeof(amqp_queue_properties_t));
		    if(!p) {
			    return AMQP_STATUS_NO_MEMORY;
		    }
		    p->_flags = flags;
		    *decoded = p;
		    return 0;
	    }
		case 60: {
		    amqp_basic_properties_t * p = (amqp_basic_properties_t*)amqp_pool_alloc(pool, sizeof(amqp_basic_properties_t));
		    if(!p) {
			    return AMQP_STATUS_NO_MEMORY;
		    }
		    p->_flags = flags;
		    if(flags & AMQP_BASIC_CONTENT_TYPE_FLAG) {
			    {
				    uint8 len;
				    if(!amqp_decode_8(encoded, &offset, &len) || !amqp_decode_bytes(encoded, &offset, &p->content_type, len))
					    return AMQP_STATUS_BAD_AMQP_DATA;
			    }
		    }
		    if(flags & AMQP_BASIC_CONTENT_ENCODING_FLAG) {
			    {
				    uint8 len;
				    if(!amqp_decode_8(encoded, &offset, &len) || !amqp_decode_bytes(encoded, &offset, &p->content_encoding, len))
					    return AMQP_STATUS_BAD_AMQP_DATA;
			    }
		    }
		    if(flags & AMQP_BASIC_HEADERS_FLAG) {
			    {
				    int res = amqp_decode_table(encoded, pool, &(p->headers), &offset);
				    if(res < 0) return res;
			    }
		    }
		    if(flags & AMQP_BASIC_DELIVERY_MODE_FLAG) {
			    if(!amqp_decode_8(encoded, &offset, &p->delivery_mode))
				    return AMQP_STATUS_BAD_AMQP_DATA;
		    }
		    if(flags & AMQP_BASIC_PRIORITY_FLAG) {
			    if(!amqp_decode_8(encoded, &offset, &p->priority))
				    return AMQP_STATUS_BAD_AMQP_DATA;
		    }
		    if(flags & AMQP_BASIC_CORRELATION_ID_FLAG) {
			    {
				    uint8 len;
				    if(!amqp_decode_8(encoded, &offset, &len) ||
					!amqp_decode_bytes(encoded, &offset, &p->correlation_id, len))
					    return AMQP_STATUS_BAD_AMQP_DATA;
			    }
		    }
		    if(flags & AMQP_BASIC_REPLY_TO_FLAG) {
			    {
				    uint8 len;
				    if(!amqp_decode_8(encoded, &offset, &len) ||
					!amqp_decode_bytes(encoded, &offset, &p->reply_to, len))
					    return AMQP_STATUS_BAD_AMQP_DATA;
			    }
		    }
		    if(flags & AMQP_BASIC_EXPIRATION_FLAG) {
			    {
				    uint8 len;
				    if(!amqp_decode_8(encoded, &offset, &len) ||
					!amqp_decode_bytes(encoded, &offset, &p->expiration, len))
					    return AMQP_STATUS_BAD_AMQP_DATA;
			    }
		    }
		    if(flags & AMQP_BASIC_MESSAGE_ID_FLAG) {
			    {
				    uint8 len;
				    if(!amqp_decode_8(encoded, &offset, &len) ||
					!amqp_decode_bytes(encoded, &offset, &p->message_id, len))
					    return AMQP_STATUS_BAD_AMQP_DATA;
			    }
		    }
		    if(flags & AMQP_BASIC_TIMESTAMP_FLAG) {
			    if(!amqp_decode_64(encoded, &offset, &p->timestamp))
				    return AMQP_STATUS_BAD_AMQP_DATA;
		    }
		    if(flags & AMQP_BASIC_TYPE_FLAG) {
			    {
				    uint8 len;
				    if(!amqp_decode_8(encoded, &offset, &len) ||
					!amqp_decode_bytes(encoded, &offset, &p->type, len))
					    return AMQP_STATUS_BAD_AMQP_DATA;
			    }
		    }
		    if(flags & AMQP_BASIC_USER_ID_FLAG) {
			    {
				    uint8 len;
				    if(!amqp_decode_8(encoded, &offset, &len) ||
					!amqp_decode_bytes(encoded, &offset, &p->user_id, len))
					    return AMQP_STATUS_BAD_AMQP_DATA;
			    }
		    }
		    if(flags & AMQP_BASIC_APP_ID_FLAG) {
			    {
				    uint8 len;
				    if(!amqp_decode_8(encoded, &offset, &len) || !amqp_decode_bytes(encoded, &offset, &p->app_id, len))
					    return AMQP_STATUS_BAD_AMQP_DATA;
			    }
		    }
		    if(flags & AMQP_BASIC_CLUSTER_ID_FLAG) {
			    {
				    uint8 len;
				    if(!amqp_decode_8(encoded, &offset, &len) || !amqp_decode_bytes(encoded, &offset, &p->cluster_id, len))
					    return AMQP_STATUS_BAD_AMQP_DATA;
			    }
		    }
		    *decoded = p;
		    return 0;
	    }
		case 90: 
			{
				amqp_tx_properties_t * p = static_cast<amqp_tx_properties_t *>(amqp_pool_alloc(pool, sizeof(amqp_tx_properties_t)));
				if(!p)
					return AMQP_STATUS_NO_MEMORY;
				else {
					p->_flags = flags;
					*decoded = p;
					return 0;
				}
			}
		case 85: 
			{
				amqp_confirm_properties_t * p = static_cast<amqp_confirm_properties_t *>(amqp_pool_alloc(pool, sizeof(amqp_confirm_properties_t)));
				if(!p)
					return AMQP_STATUS_NO_MEMORY;
				else {
					p->_flags = flags;
					*decoded = p;
					return 0;
				}
			}
		default:
		    return AMQP_STATUS_UNKNOWN_CLASS;
	}
}

int amqp_encode_method(amqp_method_number_t methodNumber, const void * decoded, amqp_bytes_t encoded) 
{
	size_t offset = 0;
	uint8 bit_buffer;
	switch(methodNumber) {
		case AMQP_CONNECTION_START_METHOD: 
			{
				const amqp_connection_start_t * m = (const amqp_connection_start_t*)decoded;
				if(!amqp_encode_8(encoded, &offset, m->version_major))
					return AMQP_STATUS_BAD_AMQP_DATA;
				if(!amqp_encode_8(encoded, &offset, m->version_minor))
					return AMQP_STATUS_BAD_AMQP_DATA;
				{
					int res = amqp_encode_table(encoded, &(m->server_properties), &offset);
					if(res < 0) return res;
				}
				if(UINT32_MAX < m->mechanisms.len || !amqp_encode_32(encoded, &offset, (uint32)m->mechanisms.len) || !amqp_encode_bytes(encoded, &offset, m->mechanisms))
					return AMQP_STATUS_BAD_AMQP_DATA;
				if(UINT32_MAX < m->locales.len || !amqp_encode_32(encoded, &offset, (uint32)m->locales.len) || !amqp_encode_bytes(encoded, &offset, m->locales))
					return AMQP_STATUS_BAD_AMQP_DATA;
				return static_cast<int>(offset);
			}
		case AMQP_CONNECTION_START_OK_METHOD: 
			{
				const amqp_connection_start_ok_t * m = (const amqp_connection_start_ok_t*)decoded;
				{
					int res = amqp_encode_table(encoded, &(m->client_properties), &offset);
					if(res < 0) return res;
				}
				if(UINT8_MAX < m->mechanism.len || !amqp_encode_8(encoded, &offset, (uint8)m->mechanism.len) || !amqp_encode_bytes(encoded, &offset, m->mechanism))
					return AMQP_STATUS_BAD_AMQP_DATA;
				if(UINT32_MAX < m->response.len || !amqp_encode_32(encoded, &offset, (uint32)m->response.len) || !amqp_encode_bytes(encoded, &offset, m->response))
					return AMQP_STATUS_BAD_AMQP_DATA;
				if(UINT8_MAX < m->locale.len || !amqp_encode_8(encoded, &offset, (uint8)m->locale.len) || !amqp_encode_bytes(encoded, &offset, m->locale))
					return AMQP_STATUS_BAD_AMQP_DATA;
				return static_cast<int>(offset);
			}
		case AMQP_CONNECTION_SECURE_METHOD: 
			{
				const amqp_connection_secure_t * m = (const amqp_connection_secure_t*)decoded;
				if(UINT32_MAX < m->challenge.len || !amqp_encode_32(encoded, &offset, (uint32)m->challenge.len) || !amqp_encode_bytes(encoded, &offset, m->challenge))
					return AMQP_STATUS_BAD_AMQP_DATA;
				return static_cast<int>(offset);
			}
		case AMQP_CONNECTION_SECURE_OK_METHOD: 
			{
				const amqp_connection_secure_ok_t * m = (const amqp_connection_secure_ok_t*)decoded;
				if(UINT32_MAX < m->response.len || !amqp_encode_32(encoded, &offset, (uint32)m->response.len) || !amqp_encode_bytes(encoded, &offset, m->response))
					return AMQP_STATUS_BAD_AMQP_DATA;
				return static_cast<int>(offset);
			}
		case AMQP_CONNECTION_TUNE_METHOD: 
			{
				const amqp_connection_tune_t * m = (const amqp_connection_tune_t*)decoded;
				if(!amqp_encode_16(encoded, &offset, m->channel_max))
					return AMQP_STATUS_BAD_AMQP_DATA;
				if(!amqp_encode_32(encoded, &offset, m->frame_max))
					return AMQP_STATUS_BAD_AMQP_DATA;
				if(!amqp_encode_16(encoded, &offset, m->heartbeat))
					return AMQP_STATUS_BAD_AMQP_DATA;
				return static_cast<int>(offset);
			}
		case AMQP_CONNECTION_TUNE_OK_METHOD: 
			{
				const amqp_connection_tune_ok_t * m = (const amqp_connection_tune_ok_t*)decoded;
				if(!amqp_encode_16(encoded, &offset, m->channel_max))
					return AMQP_STATUS_BAD_AMQP_DATA;
				if(!amqp_encode_32(encoded, &offset, m->frame_max))
					return AMQP_STATUS_BAD_AMQP_DATA;
				if(!amqp_encode_16(encoded, &offset, m->heartbeat))
					return AMQP_STATUS_BAD_AMQP_DATA;
				return static_cast<int>(offset);
			}
		case AMQP_CONNECTION_OPEN_METHOD: 
			{
				const amqp_connection_open_t * m = (const amqp_connection_open_t*)decoded;
				if(UINT8_MAX < m->virtual_host.len || !amqp_encode_8(encoded, &offset, (uint8)m->virtual_host.len) || !amqp_encode_bytes(encoded, &offset, m->virtual_host))
					return AMQP_STATUS_BAD_AMQP_DATA;
				if(UINT8_MAX < m->capabilities.len || !amqp_encode_8(encoded, &offset, (uint8)m->capabilities.len) || !amqp_encode_bytes(encoded, &offset, m->capabilities))
					return AMQP_STATUS_BAD_AMQP_DATA;
				bit_buffer = 0;
				if(m->insist) 
					bit_buffer |= (1 << 0);
				if(!amqp_encode_8(encoded, &offset, bit_buffer))
					return AMQP_STATUS_BAD_AMQP_DATA;
				return static_cast<int>(offset);
			}
		case AMQP_CONNECTION_OPEN_OK_METHOD: 
			{
				const amqp_connection_open_ok_t * m = (const amqp_connection_open_ok_t*)decoded;
				if(UINT8_MAX < m->known_hosts.len || !amqp_encode_8(encoded, &offset, (uint8)m->known_hosts.len) || !amqp_encode_bytes(encoded, &offset, m->known_hosts))
					return AMQP_STATUS_BAD_AMQP_DATA;
				return static_cast<int>(offset);
			}
		case AMQP_CONNECTION_CLOSE_METHOD: 
			{
				const amqp_connection_close_t * m = (const amqp_connection_close_t*)decoded;
				if(!amqp_encode_16(encoded, &offset, m->reply_code))
					return AMQP_STATUS_BAD_AMQP_DATA;
				if(UINT8_MAX < m->reply_text.len || !amqp_encode_8(encoded, &offset, (uint8)m->reply_text.len) || !amqp_encode_bytes(encoded, &offset, m->reply_text))
					return AMQP_STATUS_BAD_AMQP_DATA;
				if(!amqp_encode_16(encoded, &offset, m->class_id))
					return AMQP_STATUS_BAD_AMQP_DATA;
				if(!amqp_encode_16(encoded, &offset, m->method_id))
					return AMQP_STATUS_BAD_AMQP_DATA;
				return static_cast<int>(offset);
			}
		case AMQP_CONNECTION_CLOSE_OK_METHOD: return static_cast<int>(offset);
		case AMQP_CONNECTION_BLOCKED_METHOD: 
			{
				const amqp_connection_blocked_t * m = (const amqp_connection_blocked_t*)decoded;
				if(UINT8_MAX < m->reason.len || !amqp_encode_8(encoded, &offset, (uint8)m->reason.len) || !amqp_encode_bytes(encoded, &offset, m->reason))
					return AMQP_STATUS_BAD_AMQP_DATA;
				return static_cast<int>(offset);
			}
		case AMQP_CONNECTION_UNBLOCKED_METHOD: return static_cast<int>(offset);
		case AMQP_CHANNEL_OPEN_METHOD: 
			{
				const amqp_channel_open_t * m = (const amqp_channel_open_t*)decoded;
				if(UINT8_MAX < m->out_of_band.len || !amqp_encode_8(encoded, &offset, (uint8)m->out_of_band.len) || !amqp_encode_bytes(encoded, &offset, m->out_of_band))
					return AMQP_STATUS_BAD_AMQP_DATA;
				return static_cast<int>(offset);
			}
		case AMQP_CHANNEL_OPEN_OK_METHOD: 
			{
				const amqp_channel_open_ok_t * m = (const amqp_channel_open_ok_t*)decoded;
				if(UINT32_MAX < m->channel_id.len || !amqp_encode_32(encoded, &offset, (uint32)m->channel_id.len) || !amqp_encode_bytes(encoded, &offset, m->channel_id)) 
					return AMQP_STATUS_BAD_AMQP_DATA;
				return static_cast<int>(offset);
			}
		case AMQP_CHANNEL_FLOW_METHOD: 
			{
				const amqp_channel_flow_t * m = (const amqp_channel_flow_t*)decoded;
				bit_buffer = 0;
				if(m->active) bit_buffer |= (1 << 0);
				if(!amqp_encode_8(encoded, &offset, bit_buffer))
					return AMQP_STATUS_BAD_AMQP_DATA;
				return static_cast<int>(offset);
			}
		case AMQP_CHANNEL_FLOW_OK_METHOD: 
			{
				const amqp_channel_flow_ok_t * m = (const amqp_channel_flow_ok_t*)decoded;
				bit_buffer = 0;
				if(m->active) bit_buffer |= (1 << 0);
				if(!amqp_encode_8(encoded, &offset, bit_buffer))
					return AMQP_STATUS_BAD_AMQP_DATA;
				return static_cast<int>(offset);
			}
		case AMQP_CHANNEL_CLOSE_METHOD: 
			{
				const amqp_channel_close_t * m = (const amqp_channel_close_t*)decoded;
				if(!amqp_encode_16(encoded, &offset, m->reply_code))
					return AMQP_STATUS_BAD_AMQP_DATA;
				if(UINT8_MAX < m->reply_text.len || !amqp_encode_8(encoded, &offset, (uint8)m->reply_text.len) || !amqp_encode_bytes(encoded, &offset, m->reply_text))
					return AMQP_STATUS_BAD_AMQP_DATA;
				if(!amqp_encode_16(encoded, &offset, m->class_id))
					return AMQP_STATUS_BAD_AMQP_DATA;
				if(!amqp_encode_16(encoded, &offset, m->method_id))
					return AMQP_STATUS_BAD_AMQP_DATA;
				return static_cast<int>(offset);
			}
		case AMQP_CHANNEL_CLOSE_OK_METHOD: return static_cast<int>(offset);
		case AMQP_ACCESS_REQUEST_METHOD: 
			{
				const amqp_access_request_t * m = (const amqp_access_request_t*)decoded;
				if(UINT8_MAX < m->realm.len || !amqp_encode_8(encoded, &offset, (uint8)m->realm.len) || !amqp_encode_bytes(encoded, &offset, m->realm))
					return AMQP_STATUS_BAD_AMQP_DATA;
				bit_buffer = 0;
				if(m->exclusive) bit_buffer |= (1 << 0);
				if(m->passive) bit_buffer |= (1 << 1);
				if(m->active) bit_buffer |= (1 << 2);
				if(m->write) bit_buffer |= (1 << 3);
				if(m->read) bit_buffer |= (1 << 4);
				if(!amqp_encode_8(encoded, &offset, bit_buffer))
					return AMQP_STATUS_BAD_AMQP_DATA;
				return static_cast<int>(offset);
			}
		case AMQP_ACCESS_REQUEST_OK_METHOD: 
			{
				const amqp_access_request_ok_t * m = (const amqp_access_request_ok_t*)decoded;
				if(!amqp_encode_16(encoded, &offset, m->ticket))
					return AMQP_STATUS_BAD_AMQP_DATA;
				return static_cast<int>(offset);
			}
		case AMQP_EXCHANGE_DECLARE_METHOD: 
			{
				const amqp_exchange_declare_t * m = (const amqp_exchange_declare_t*)decoded;
				if(!amqp_encode_16(encoded, &offset, m->ticket))
					return AMQP_STATUS_BAD_AMQP_DATA;
				if(UINT8_MAX < m->exchange.len ||
				!amqp_encode_8(encoded, &offset, (uint8)m->exchange.len) ||
				!amqp_encode_bytes(encoded, &offset, m->exchange))
					return AMQP_STATUS_BAD_AMQP_DATA;
				if(UINT8_MAX < m->type.len ||
				!amqp_encode_8(encoded, &offset, (uint8)m->type.len) ||
				!amqp_encode_bytes(encoded, &offset, m->type))
					return AMQP_STATUS_BAD_AMQP_DATA;
				bit_buffer = 0;
				if(m->passive) bit_buffer |= (1 << 0);
				if(m->durable) bit_buffer |= (1 << 1);
				if(m->auto_delete) bit_buffer |= (1 << 2);
				if(m->internal) bit_buffer |= (1 << 3);
				if(m->nowait) bit_buffer |= (1 << 4);
				if(!amqp_encode_8(encoded, &offset, bit_buffer))
					return AMQP_STATUS_BAD_AMQP_DATA;
				{
					int res = amqp_encode_table(encoded, &(m->arguments), &offset);
					if(res < 0) return res;
				}
				return static_cast<int>(offset);
			}
		case AMQP_EXCHANGE_DECLARE_OK_METHOD: return static_cast<int>(offset);
		case AMQP_EXCHANGE_DELETE_METHOD: 
			{
				const amqp_exchange_delete_t * m = (const amqp_exchange_delete_t*)decoded;
				if(!amqp_encode_16(encoded, &offset, m->ticket))
					return AMQP_STATUS_BAD_AMQP_DATA;
				if(UINT8_MAX < m->exchange.len || !amqp_encode_8(encoded, &offset, (uint8)m->exchange.len) || !amqp_encode_bytes(encoded, &offset, m->exchange))
					return AMQP_STATUS_BAD_AMQP_DATA;
				bit_buffer = 0;
				if(m->if_unused) bit_buffer |= (1 << 0);
				if(m->nowait) bit_buffer |= (1 << 1);
				if(!amqp_encode_8(encoded, &offset, bit_buffer))
					return AMQP_STATUS_BAD_AMQP_DATA;
				return static_cast<int>(offset);
			}
		case AMQP_EXCHANGE_DELETE_OK_METHOD: return static_cast<int>(offset);
		case AMQP_EXCHANGE_BIND_METHOD: 
			{
				const amqp_exchange_bind_t * m = (const amqp_exchange_bind_t*)decoded;
				if(!amqp_encode_16(encoded, &offset, m->ticket))
					return AMQP_STATUS_BAD_AMQP_DATA;
				if(UINT8_MAX < m->destination.len || !amqp_encode_8(encoded, &offset, (uint8)m->destination.len) || !amqp_encode_bytes(encoded, &offset, m->destination))
					return AMQP_STATUS_BAD_AMQP_DATA;
				if(UINT8_MAX < m->source.len || !amqp_encode_8(encoded, &offset, (uint8)m->source.len) || !amqp_encode_bytes(encoded, &offset, m->source))
					return AMQP_STATUS_BAD_AMQP_DATA;
				if(UINT8_MAX < m->routing_key.len || !amqp_encode_8(encoded, &offset, (uint8)m->routing_key.len) || !amqp_encode_bytes(encoded, &offset, m->routing_key))
					return AMQP_STATUS_BAD_AMQP_DATA;
				bit_buffer = 0;
				if(m->nowait) bit_buffer |= (1 << 0);
				if(!amqp_encode_8(encoded, &offset, bit_buffer))
					return AMQP_STATUS_BAD_AMQP_DATA;
				{
					int res = amqp_encode_table(encoded, &(m->arguments), &offset);
					if(res < 0) return res;
				}
				return static_cast<int>(offset);
			}
		case AMQP_EXCHANGE_BIND_OK_METHOD: return static_cast<int>(offset);
		case AMQP_EXCHANGE_UNBIND_METHOD: 
			{
				const amqp_exchange_unbind_t * m = (const amqp_exchange_unbind_t*)decoded;
				if(!amqp_encode_16(encoded, &offset, m->ticket))
					return AMQP_STATUS_BAD_AMQP_DATA;
				if(UINT8_MAX < m->destination.len || !amqp_encode_8(encoded, &offset, (uint8)m->destination.len) || !amqp_encode_bytes(encoded, &offset, m->destination))
					return AMQP_STATUS_BAD_AMQP_DATA;
				if(UINT8_MAX < m->source.len || !amqp_encode_8(encoded, &offset, (uint8)m->source.len) || !amqp_encode_bytes(encoded, &offset, m->source))
					return AMQP_STATUS_BAD_AMQP_DATA;
				if(UINT8_MAX < m->routing_key.len ||
				!amqp_encode_8(encoded, &offset, (uint8)m->routing_key.len) ||
				!amqp_encode_bytes(encoded, &offset, m->routing_key))
					return AMQP_STATUS_BAD_AMQP_DATA;
				bit_buffer = 0;
				if(m->nowait) bit_buffer |= (1 << 0);
				if(!amqp_encode_8(encoded, &offset, bit_buffer))
					return AMQP_STATUS_BAD_AMQP_DATA;
				{
					int res = amqp_encode_table(encoded, &(m->arguments), &offset);
					if(res < 0) return res;
				}
				return static_cast<int>(offset);
			}
		case AMQP_EXCHANGE_UNBIND_OK_METHOD: return static_cast<int>(offset);
		case AMQP_QUEUE_DECLARE_METHOD: 
			{
				const amqp_queue_declare_t * m = (const amqp_queue_declare_t*)decoded;
				if(!amqp_encode_16(encoded, &offset, m->ticket))
					return AMQP_STATUS_BAD_AMQP_DATA;
				if(UINT8_MAX < m->queue.len || !amqp_encode_8(encoded, &offset, (uint8)m->queue.len) || !amqp_encode_bytes(encoded, &offset, m->queue))
					return AMQP_STATUS_BAD_AMQP_DATA;
				bit_buffer = 0;
				if(m->passive) bit_buffer |= (1 << 0);
				if(m->durable) bit_buffer |= (1 << 1);
				if(m->exclusive) bit_buffer |= (1 << 2);
				if(m->auto_delete) bit_buffer |= (1 << 3);
				if(m->nowait) bit_buffer |= (1 << 4);
				if(!amqp_encode_8(encoded, &offset, bit_buffer))
					return AMQP_STATUS_BAD_AMQP_DATA;
				{
					int res = amqp_encode_table(encoded, &(m->arguments), &offset);
					if(res < 0) return res;
				}
				return static_cast<int>(offset);
			}
		case AMQP_QUEUE_DECLARE_OK_METHOD: 
			{
				const amqp_queue_declare_ok_t * m = (const amqp_queue_declare_ok_t*)decoded;
				if(UINT8_MAX < m->queue.len || !amqp_encode_8(encoded, &offset, (uint8)m->queue.len) || !amqp_encode_bytes(encoded, &offset, m->queue))
					return AMQP_STATUS_BAD_AMQP_DATA;
				if(!amqp_encode_32(encoded, &offset, m->message_count))
					return AMQP_STATUS_BAD_AMQP_DATA;
				if(!amqp_encode_32(encoded, &offset, m->consumer_count))
					return AMQP_STATUS_BAD_AMQP_DATA;
				return static_cast<int>(offset);
			}
		case AMQP_QUEUE_BIND_METHOD: 
			{
				const amqp_queue_bind_t * m = (const amqp_queue_bind_t*)decoded;
				if(!amqp_encode_16(encoded, &offset, m->ticket))
					return AMQP_STATUS_BAD_AMQP_DATA;
				if(UINT8_MAX < m->queue.len || !amqp_encode_8(encoded, &offset, (uint8)m->queue.len) || !amqp_encode_bytes(encoded, &offset, m->queue))
					return AMQP_STATUS_BAD_AMQP_DATA;
				if(UINT8_MAX < m->exchange.len || !amqp_encode_8(encoded, &offset, (uint8)m->exchange.len) || !amqp_encode_bytes(encoded, &offset, m->exchange))
					return AMQP_STATUS_BAD_AMQP_DATA;
				if(UINT8_MAX < m->routing_key.len || !amqp_encode_8(encoded, &offset, (uint8)m->routing_key.len) || !amqp_encode_bytes(encoded, &offset, m->routing_key))
					return AMQP_STATUS_BAD_AMQP_DATA;
				bit_buffer = 0;
				if(m->nowait) bit_buffer |= (1 << 0);
				if(!amqp_encode_8(encoded, &offset, bit_buffer))
					return AMQP_STATUS_BAD_AMQP_DATA;
				{
					int res = amqp_encode_table(encoded, &(m->arguments), &offset);
					if(res < 0) return res;
				}
				return static_cast<int>(offset);
			}
		case AMQP_QUEUE_BIND_OK_METHOD: return static_cast<int>(offset);
		case AMQP_QUEUE_PURGE_METHOD: 
			{
				const amqp_queue_purge_t * m = (const amqp_queue_purge_t*)decoded;
				if(!amqp_encode_16(encoded, &offset, m->ticket))
					return AMQP_STATUS_BAD_AMQP_DATA;
				if(UINT8_MAX < m->queue.len || !amqp_encode_8(encoded, &offset, (uint8)m->queue.len) || !amqp_encode_bytes(encoded, &offset, m->queue))
					return AMQP_STATUS_BAD_AMQP_DATA;
				bit_buffer = 0;
				if(m->nowait) bit_buffer |= (1 << 0);
				if(!amqp_encode_8(encoded, &offset, bit_buffer))
					return AMQP_STATUS_BAD_AMQP_DATA;
				return static_cast<int>(offset);
			}
		case AMQP_QUEUE_PURGE_OK_METHOD: 
			{
				const amqp_queue_purge_ok_t * m = (const amqp_queue_purge_ok_t*)decoded;
				if(!amqp_encode_32(encoded, &offset, m->message_count))
					return AMQP_STATUS_BAD_AMQP_DATA;
				return static_cast<int>(offset);
			}
		case AMQP_QUEUE_DELETE_METHOD: 
			{
				const amqp_queue_delete_t * m = (const amqp_queue_delete_t*)decoded;
				if(!amqp_encode_16(encoded, &offset, m->ticket))
					return AMQP_STATUS_BAD_AMQP_DATA;
				if(UINT8_MAX < m->queue.len || !amqp_encode_8(encoded, &offset, (uint8)m->queue.len) || !amqp_encode_bytes(encoded, &offset, m->queue))
					return AMQP_STATUS_BAD_AMQP_DATA;
				bit_buffer = 0;
				if(m->if_unused) bit_buffer |= (1 << 0);
				if(m->if_empty) bit_buffer |= (1 << 1);
				if(m->nowait) bit_buffer |= (1 << 2);
				if(!amqp_encode_8(encoded, &offset, bit_buffer))
					return AMQP_STATUS_BAD_AMQP_DATA;
				return static_cast<int>(offset);
			}
		case AMQP_QUEUE_DELETE_OK_METHOD: 
			{
				const amqp_queue_delete_ok_t * m = (const amqp_queue_delete_ok_t*)decoded;
				if(!amqp_encode_32(encoded, &offset, m->message_count))
					return AMQP_STATUS_BAD_AMQP_DATA;
				return static_cast<int>(offset);
			}
		case AMQP_QUEUE_UNBIND_METHOD: {
		    amqp_queue_unbind_t * m = (amqp_queue_unbind_t*)decoded;
		    if(!amqp_encode_16(encoded, &offset, m->ticket))
			    return AMQP_STATUS_BAD_AMQP_DATA;
		    if(UINT8_MAX < m->queue.len || !amqp_encode_8(encoded, &offset, (uint8)m->queue.len) || !amqp_encode_bytes(encoded, &offset, m->queue))
			    return AMQP_STATUS_BAD_AMQP_DATA;
		    if(UINT8_MAX < m->exchange.len || !amqp_encode_8(encoded, &offset, (uint8)m->exchange.len) || !amqp_encode_bytes(encoded, &offset, m->exchange))
			    return AMQP_STATUS_BAD_AMQP_DATA;
		    if(UINT8_MAX < m->routing_key.len || !amqp_encode_8(encoded, &offset, (uint8)m->routing_key.len) || !amqp_encode_bytes(encoded, &offset, m->routing_key))
			    return AMQP_STATUS_BAD_AMQP_DATA;
		    {
			    int res = amqp_encode_table(encoded, &(m->arguments), &offset);
			    if(res < 0) return res;
		    }
		    return static_cast<int>(offset);
	    }
		case AMQP_QUEUE_UNBIND_OK_METHOD: return static_cast<int>(offset);
		case AMQP_BASIC_QOS_METHOD: {
		    amqp_basic_qos_t * m = (amqp_basic_qos_t*)decoded;
		    if(!amqp_encode_32(encoded, &offset, m->prefetch_size))
			    return AMQP_STATUS_BAD_AMQP_DATA;
		    if(!amqp_encode_16(encoded, &offset, m->prefetch_count))
			    return AMQP_STATUS_BAD_AMQP_DATA;
		    bit_buffer = 0;
		    if(m->global) bit_buffer |= (1 << 0);
		    if(!amqp_encode_8(encoded, &offset, bit_buffer))
			    return AMQP_STATUS_BAD_AMQP_DATA;
		    return static_cast<int>(offset);
	    }
		case AMQP_BASIC_QOS_OK_METHOD: return static_cast<int>(offset);
		case AMQP_BASIC_CONSUME_METHOD: {
		    amqp_basic_consume_t * m = (amqp_basic_consume_t*)decoded;
		    if(!amqp_encode_16(encoded, &offset, m->ticket))
			    return AMQP_STATUS_BAD_AMQP_DATA;
		    if(UINT8_MAX < m->queue.len ||
			!amqp_encode_8(encoded, &offset, (uint8)m->queue.len) ||
			!amqp_encode_bytes(encoded, &offset, m->queue))
			    return AMQP_STATUS_BAD_AMQP_DATA;
		    if(UINT8_MAX < m->consumer_tag.len ||
			!amqp_encode_8(encoded, &offset, (uint8)m->consumer_tag.len) ||
			!amqp_encode_bytes(encoded, &offset, m->consumer_tag))
			    return AMQP_STATUS_BAD_AMQP_DATA;
		    bit_buffer = 0;
		    if(m->no_local) bit_buffer |= (1 << 0);
		    if(m->no_ack) bit_buffer |= (1 << 1);
		    if(m->exclusive) bit_buffer |= (1 << 2);
		    if(m->nowait) bit_buffer |= (1 << 3);
		    if(!amqp_encode_8(encoded, &offset, bit_buffer))
			    return AMQP_STATUS_BAD_AMQP_DATA;
		    {
			    int res = amqp_encode_table(encoded, &(m->arguments), &offset);
			    if(res < 0) return res;
		    }
		    return static_cast<int>(offset);
	    }
		case AMQP_BASIC_CONSUME_OK_METHOD: {
		    amqp_basic_consume_ok_t * m = (amqp_basic_consume_ok_t*)decoded;
		    if(UINT8_MAX < m->consumer_tag.len ||
			!amqp_encode_8(encoded, &offset, (uint8)m->consumer_tag.len) ||
			!amqp_encode_bytes(encoded, &offset, m->consumer_tag))
			    return AMQP_STATUS_BAD_AMQP_DATA;
		    return static_cast<int>(offset);
	    }
		case AMQP_BASIC_CANCEL_METHOD: {
		    amqp_basic_cancel_t * m = (amqp_basic_cancel_t*)decoded;
		    if(UINT8_MAX < m->consumer_tag.len ||
			!amqp_encode_8(encoded, &offset, (uint8)m->consumer_tag.len) ||
			!amqp_encode_bytes(encoded, &offset, m->consumer_tag))
			    return AMQP_STATUS_BAD_AMQP_DATA;
		    bit_buffer = 0;
		    if(m->nowait) bit_buffer |= (1 << 0);
		    if(!amqp_encode_8(encoded, &offset, bit_buffer))
			    return AMQP_STATUS_BAD_AMQP_DATA;
		    return static_cast<int>(offset);
	    }
		case AMQP_BASIC_CANCEL_OK_METHOD: 
			{
				amqp_basic_cancel_ok_t * m = (amqp_basic_cancel_ok_t*)decoded;
				if(UINT8_MAX < m->consumer_tag.len || !amqp_encode_8(encoded, &offset, (uint8)m->consumer_tag.len) || !amqp_encode_bytes(encoded, &offset, m->consumer_tag))
					return AMQP_STATUS_BAD_AMQP_DATA;
				return static_cast<int>(offset);
			}
		case AMQP_BASIC_PUBLISH_METHOD: {
		    amqp_basic_publish_t * m = (amqp_basic_publish_t*)decoded;
		    if(!amqp_encode_16(encoded, &offset, m->ticket))
			    return AMQP_STATUS_BAD_AMQP_DATA;
		    if(UINT8_MAX < m->exchange.len ||
			!amqp_encode_8(encoded, &offset, (uint8)m->exchange.len) ||
			!amqp_encode_bytes(encoded, &offset, m->exchange))
			    return AMQP_STATUS_BAD_AMQP_DATA;
		    if(UINT8_MAX < m->routing_key.len ||
			!amqp_encode_8(encoded, &offset, (uint8)m->routing_key.len) ||
			!amqp_encode_bytes(encoded, &offset, m->routing_key))
			    return AMQP_STATUS_BAD_AMQP_DATA;
		    bit_buffer = 0;
		    if(m->mandatory) bit_buffer |= (1 << 0);
		    if(m->immediate) bit_buffer |= (1 << 1);
		    if(!amqp_encode_8(encoded, &offset, bit_buffer))
			    return AMQP_STATUS_BAD_AMQP_DATA;
		    return static_cast<int>(offset);
	    }
		case AMQP_BASIC_RETURN_METHOD: {
		    amqp_basic_return_t * m = (amqp_basic_return_t*)decoded;
		    if(!amqp_encode_16(encoded, &offset, m->reply_code))
			    return AMQP_STATUS_BAD_AMQP_DATA;
		    if(UINT8_MAX < m->reply_text.len ||
			!amqp_encode_8(encoded, &offset, (uint8)m->reply_text.len) ||
			!amqp_encode_bytes(encoded, &offset, m->reply_text))
			    return AMQP_STATUS_BAD_AMQP_DATA;
		    if(UINT8_MAX < m->exchange.len ||
			!amqp_encode_8(encoded, &offset, (uint8)m->exchange.len) ||
			!amqp_encode_bytes(encoded, &offset, m->exchange))
			    return AMQP_STATUS_BAD_AMQP_DATA;
		    if(UINT8_MAX < m->routing_key.len ||
			!amqp_encode_8(encoded, &offset, (uint8)m->routing_key.len) ||
			!amqp_encode_bytes(encoded, &offset, m->routing_key))
			    return AMQP_STATUS_BAD_AMQP_DATA;
		    return static_cast<int>(offset);
	    }
		case AMQP_BASIC_DELIVER_METHOD: {
		    amqp_basic_deliver_t * m = (amqp_basic_deliver_t*)decoded;
		    if(UINT8_MAX < m->consumer_tag.len ||
			!amqp_encode_8(encoded, &offset, (uint8)m->consumer_tag.len) ||
			!amqp_encode_bytes(encoded, &offset, m->consumer_tag))
			    return AMQP_STATUS_BAD_AMQP_DATA;
		    if(!amqp_encode_64(encoded, &offset, m->delivery_tag))
			    return AMQP_STATUS_BAD_AMQP_DATA;
		    bit_buffer = 0;
		    if(m->redelivered) bit_buffer |= (1 << 0);
		    if(!amqp_encode_8(encoded, &offset, bit_buffer))
			    return AMQP_STATUS_BAD_AMQP_DATA;
		    if(UINT8_MAX < m->exchange.len ||
			!amqp_encode_8(encoded, &offset, (uint8)m->exchange.len) ||
			!amqp_encode_bytes(encoded, &offset, m->exchange))
			    return AMQP_STATUS_BAD_AMQP_DATA;
		    if(UINT8_MAX < m->routing_key.len ||
			!amqp_encode_8(encoded, &offset, (uint8)m->routing_key.len) ||
			!amqp_encode_bytes(encoded, &offset, m->routing_key))
			    return AMQP_STATUS_BAD_AMQP_DATA;
		    return static_cast<int>(offset);
	    }
		case AMQP_BASIC_GET_METHOD: {
		    amqp_basic_get_t * m = (amqp_basic_get_t*)decoded;
		    if(!amqp_encode_16(encoded, &offset, m->ticket))
			    return AMQP_STATUS_BAD_AMQP_DATA;
		    if(UINT8_MAX < m->queue.len ||
			!amqp_encode_8(encoded, &offset, (uint8)m->queue.len) ||
			!amqp_encode_bytes(encoded, &offset, m->queue))
			    return AMQP_STATUS_BAD_AMQP_DATA;
		    bit_buffer = 0;
		    if(m->no_ack) bit_buffer |= (1 << 0);
		    if(!amqp_encode_8(encoded, &offset, bit_buffer))
			    return AMQP_STATUS_BAD_AMQP_DATA;
		    return static_cast<int>(offset);
	    }
		case AMQP_BASIC_GET_OK_METHOD: {
		    amqp_basic_get_ok_t * m = (amqp_basic_get_ok_t*)decoded;
		    if(!amqp_encode_64(encoded, &offset, m->delivery_tag))
			    return AMQP_STATUS_BAD_AMQP_DATA;
		    bit_buffer = 0;
		    if(m->redelivered) bit_buffer |= (1 << 0);
		    if(!amqp_encode_8(encoded, &offset, bit_buffer))
			    return AMQP_STATUS_BAD_AMQP_DATA;
		    if(UINT8_MAX < m->exchange.len ||
			!amqp_encode_8(encoded, &offset, (uint8)m->exchange.len) ||
			!amqp_encode_bytes(encoded, &offset, m->exchange))
			    return AMQP_STATUS_BAD_AMQP_DATA;
		    if(UINT8_MAX < m->routing_key.len ||
			!amqp_encode_8(encoded, &offset, (uint8)m->routing_key.len) ||
			!amqp_encode_bytes(encoded, &offset, m->routing_key))
			    return AMQP_STATUS_BAD_AMQP_DATA;
		    if(!amqp_encode_32(encoded, &offset, m->message_count))
			    return AMQP_STATUS_BAD_AMQP_DATA;
		    return static_cast<int>(offset);
	    }
		case AMQP_BASIC_GET_EMPTY_METHOD: 
			{
				const amqp_basic_get_empty_t * m = (const amqp_basic_get_empty_t*)decoded;
				if(UINT8_MAX < m->cluster_id.len || !amqp_encode_8(encoded, &offset, (uint8)m->cluster_id.len) || !amqp_encode_bytes(encoded, &offset, m->cluster_id))
					return AMQP_STATUS_BAD_AMQP_DATA;
				return static_cast<int>(offset);
			}
		case AMQP_BASIC_ACK_METHOD: 
			{
				const amqp_basic_ack_t * m = (const amqp_basic_ack_t*)decoded;
				if(!amqp_encode_64(encoded, &offset, m->delivery_tag))
					return AMQP_STATUS_BAD_AMQP_DATA;
				bit_buffer = 0;
				if(m->multiple) bit_buffer |= (1 << 0);
				if(!amqp_encode_8(encoded, &offset, bit_buffer))
					return AMQP_STATUS_BAD_AMQP_DATA;
				return static_cast<int>(offset);
			}
		case AMQP_BASIC_REJECT_METHOD: 
			{
				const amqp_basic_reject_t * m = (const amqp_basic_reject_t*)decoded;
				if(!amqp_encode_64(encoded, &offset, m->delivery_tag))
					return AMQP_STATUS_BAD_AMQP_DATA;
				bit_buffer = 0;
				if(m->requeue) bit_buffer |= (1 << 0);
				if(!amqp_encode_8(encoded, &offset, bit_buffer))
					return AMQP_STATUS_BAD_AMQP_DATA;
				return static_cast<int>(offset);
			}
		case AMQP_BASIC_RECOVER_ASYNC_METHOD: 
			{
				const amqp_basic_recover_async_t * m = (const amqp_basic_recover_async_t*)decoded;
				bit_buffer = 0;
				if(m->requeue) bit_buffer |= (1 << 0);
				if(!amqp_encode_8(encoded, &offset, bit_buffer))
					return AMQP_STATUS_BAD_AMQP_DATA;
				return static_cast<int>(offset);
			}
		case AMQP_BASIC_RECOVER_METHOD: 
			{
				const amqp_basic_recover_t * m = (const amqp_basic_recover_t*)decoded;
				bit_buffer = 0;
				if(m->requeue) bit_buffer |= (1 << 0);
				if(!amqp_encode_8(encoded, &offset, bit_buffer))
					return AMQP_STATUS_BAD_AMQP_DATA;
				return static_cast<int>(offset);
			}
		case AMQP_BASIC_RECOVER_OK_METHOD: return static_cast<int>(offset);
		case AMQP_BASIC_NACK_METHOD: 
			{
				const amqp_basic_nack_t * m = (const amqp_basic_nack_t*)decoded;
				if(!amqp_encode_64(encoded, &offset, m->delivery_tag))
					return AMQP_STATUS_BAD_AMQP_DATA;
				bit_buffer = 0;
				if(m->multiple) bit_buffer |= (1 << 0);
				if(m->requeue) bit_buffer |= (1 << 1);
				if(!amqp_encode_8(encoded, &offset, bit_buffer))
					return AMQP_STATUS_BAD_AMQP_DATA;
				return static_cast<int>(offset);
			}
		case AMQP_TX_SELECT_METHOD: return static_cast<int>(offset);
		case AMQP_TX_SELECT_OK_METHOD: return static_cast<int>(offset);
		case AMQP_TX_COMMIT_METHOD: return static_cast<int>(offset);
		case AMQP_TX_COMMIT_OK_METHOD: return static_cast<int>(offset);
		case AMQP_TX_ROLLBACK_METHOD: return static_cast<int>(offset);
		case AMQP_TX_ROLLBACK_OK_METHOD: return static_cast<int>(offset);
		case AMQP_CONFIRM_SELECT_METHOD: {
		    amqp_confirm_select_t * m = (amqp_confirm_select_t*)decoded;
		    bit_buffer = 0;
		    if(m->nowait) bit_buffer |= (1 << 0);
		    if(!amqp_encode_8(encoded, &offset, bit_buffer))
			    return AMQP_STATUS_BAD_AMQP_DATA;
		    return static_cast<int>(offset);
	    }
		case AMQP_CONFIRM_SELECT_OK_METHOD: {
		    return static_cast<int>(offset);
	    }
		default:
		    return AMQP_STATUS_UNKNOWN_METHOD;
	}
}

int amqp_encode_properties(uint16 class_id, void * decoded, amqp_bytes_t encoded) 
{
	size_t offset = 0;
	// Cheat, and get the flags out generically, relying on the similarity of structure between classes 
	amqp_flags_t flags = *(const amqp_flags_t *)decoded; /* cheating! */
	{
		// We take a copy of flags to avoid destroying it, as it is used in the autogenerated code below. */
		amqp_flags_t remaining_flags = flags;
		do {
			amqp_flags_t remainder = remaining_flags >> 16;
			uint16 partial_flags = static_cast<uint16>(remaining_flags & 0xFFFE);
			if(remainder != 0) {
				partial_flags |= 1;
			}
			if(!amqp_encode_16(encoded, &offset, partial_flags))
				return AMQP_STATUS_BAD_AMQP_DATA;
			remaining_flags = remainder;
		} while(remaining_flags != 0);
	}
	switch(class_id) {
		case 10: return static_cast<int>(offset);
		case 20: return static_cast<int>(offset);
		case 30: return static_cast<int>(offset);
		case 40: return static_cast<int>(offset);
		case 50: return static_cast<int>(offset);
		case 60: {
		    amqp_basic_properties_t * p = (amqp_basic_properties_t*)decoded;
		    if(flags & AMQP_BASIC_CONTENT_TYPE_FLAG) {
			    if(UINT8_MAX < p->content_type.len ||
				!amqp_encode_8(encoded, &offset, (uint8)p->content_type.len) ||
				!amqp_encode_bytes(encoded, &offset, p->content_type))
				    return AMQP_STATUS_BAD_AMQP_DATA;
		    }
		    if(flags & AMQP_BASIC_CONTENT_ENCODING_FLAG) {
			    if(UINT8_MAX < p->content_encoding.len ||
				!amqp_encode_8(encoded, &offset,
				(uint8)p->content_encoding.len) ||
				!amqp_encode_bytes(encoded, &offset, p->content_encoding))
				    return AMQP_STATUS_BAD_AMQP_DATA;
		    }
		    if(flags & AMQP_BASIC_HEADERS_FLAG) {
			    {
				    int res = amqp_encode_table(encoded, &(p->headers), &offset);
				    if(res < 0) return res;
			    }
		    }
		    if(flags & AMQP_BASIC_DELIVERY_MODE_FLAG) {
			    if(!amqp_encode_8(encoded, &offset, p->delivery_mode))
				    return AMQP_STATUS_BAD_AMQP_DATA;
		    }
		    if(flags & AMQP_BASIC_PRIORITY_FLAG) {
			    if(!amqp_encode_8(encoded, &offset, p->priority))
				    return AMQP_STATUS_BAD_AMQP_DATA;
		    }
		    if(flags & AMQP_BASIC_CORRELATION_ID_FLAG) {
			    if(UINT8_MAX < p->correlation_id.len || !amqp_encode_8(encoded, &offset, (uint8)p->correlation_id.len) || !amqp_encode_bytes(encoded, &offset, p->correlation_id))
				    return AMQP_STATUS_BAD_AMQP_DATA;
		    }
		    if(flags & AMQP_BASIC_REPLY_TO_FLAG) {
			    if(UINT8_MAX < p->reply_to.len || !amqp_encode_8(encoded, &offset, (uint8)p->reply_to.len) || !amqp_encode_bytes(encoded, &offset, p->reply_to))
				    return AMQP_STATUS_BAD_AMQP_DATA;
		    }
		    if(flags & AMQP_BASIC_EXPIRATION_FLAG) {
			    if(UINT8_MAX < p->expiration.len || !amqp_encode_8(encoded, &offset, (uint8)p->expiration.len) || !amqp_encode_bytes(encoded, &offset, p->expiration))
				    return AMQP_STATUS_BAD_AMQP_DATA;
		    }
		    if(flags & AMQP_BASIC_MESSAGE_ID_FLAG) {
			    if(UINT8_MAX < p->message_id.len || !amqp_encode_8(encoded, &offset, (uint8)p->message_id.len) || !amqp_encode_bytes(encoded, &offset, p->message_id))
				    return AMQP_STATUS_BAD_AMQP_DATA;
		    }
		    if(flags & AMQP_BASIC_TIMESTAMP_FLAG) {
			    if(!amqp_encode_64(encoded, &offset, p->timestamp))
				    return AMQP_STATUS_BAD_AMQP_DATA;
		    }
		    if(flags & AMQP_BASIC_TYPE_FLAG) {
			    if(UINT8_MAX < p->type.len || !amqp_encode_8(encoded, &offset, (uint8)p->type.len) || !amqp_encode_bytes(encoded, &offset, p->type))
				    return AMQP_STATUS_BAD_AMQP_DATA;
		    }
		    if(flags & AMQP_BASIC_USER_ID_FLAG) {
			    if(UINT8_MAX < p->user_id.len || !amqp_encode_8(encoded, &offset, (uint8)p->user_id.len) || !amqp_encode_bytes(encoded, &offset, p->user_id))
				    return AMQP_STATUS_BAD_AMQP_DATA;
		    }
		    if(flags & AMQP_BASIC_APP_ID_FLAG) {
			    if(UINT8_MAX < p->app_id.len || !amqp_encode_8(encoded, &offset, (uint8)p->app_id.len) || !amqp_encode_bytes(encoded, &offset, p->app_id))
				    return AMQP_STATUS_BAD_AMQP_DATA;
		    }
		    if(flags & AMQP_BASIC_CLUSTER_ID_FLAG) {
			    if(UINT8_MAX < p->cluster_id.len || !amqp_encode_8(encoded, &offset, (uint8)p->cluster_id.len) || !amqp_encode_bytes(encoded, &offset, p->cluster_id))
				    return AMQP_STATUS_BAD_AMQP_DATA;
		    }
		    return static_cast<int>(offset);
	    }
		case 90: return static_cast<int>(offset);
		case 85: return static_cast<int>(offset);
		default: return AMQP_STATUS_UNKNOWN_CLASS;
	}
}
/**
 * amqp_channel_open
 *
 * @param [in] state connection state
 * @param [in] channel the channel to do the RPC on
 * @returns amqp_channel_open_ok_t
 */
AMQP_PUBLIC_FUNCTION amqp_channel_open_ok_t * amqp_channel_open(amqp_connection_state_t state, amqp_channel_t channel) 
{
	amqp_channel_open_t req;
	req.out_of_band = amqp_empty_bytes;
	return static_cast<amqp_channel_open_ok_t *>(amqp_simple_rpc_decoded(state, channel, AMQP_CHANNEL_OPEN_METHOD, AMQP_CHANNEL_OPEN_OK_METHOD, &req));
}
/**
 * amqp_channel_flow
 *
 * @param [in] state connection state
 * @param [in] channel the channel to do the RPC on
 * @param [in] active active
 * @returns amqp_channel_flow_ok_t
 */
AMQP_PUBLIC_FUNCTION amqp_channel_flow_ok_t * amqp_channel_flow(amqp_connection_state_t state, amqp_channel_t channel, boolint active) 
{
	amqp_channel_flow_t req;
	req.active = active;
	return static_cast<amqp_channel_flow_ok_t *>(amqp_simple_rpc_decoded(state, channel, AMQP_CHANNEL_FLOW_METHOD, AMQP_CHANNEL_FLOW_OK_METHOD, &req));
}
/**
 * amqp_exchange_declare
 *
 * @param [in] state connection state
 * @param [in] channel the channel to do the RPC on
 * @param [in] exchange exchange
 * @param [in] type type
 * @param [in] passive passive
 * @param [in] durable durable
 * @param [in] auto_delete auto_delete
 * @param [in] internal internal
 * @param [in] arguments arguments
 * @returns amqp_exchange_declare_ok_t
 */
AMQP_PUBLIC_FUNCTION amqp_exchange_declare_ok_t * amqp_exchange_declare(amqp_connection_state_t state, amqp_channel_t channel,
    amqp_bytes_t exchange, amqp_bytes_t type, boolint passive, boolint durable, boolint auto_delete, 
	boolint internal, amqp_table_t arguments) 
{
	amqp_exchange_declare_t req;
	req.ticket = 0;
	req.exchange = exchange;
	req.type = type;
	req.passive = passive;
	req.durable = durable;
	req.auto_delete = auto_delete;
	req.internal = internal;
	req.nowait = 0;
	req.arguments = arguments;
	return static_cast<amqp_exchange_declare_ok_t *>(amqp_simple_rpc_decoded(state, channel, AMQP_EXCHANGE_DECLARE_METHOD, AMQP_EXCHANGE_DECLARE_OK_METHOD, &req));
}
/**
 * amqp_exchange_delete
 *
 * @param [in] state connection state
 * @param [in] channel the channel to do the RPC on
 * @param [in] exchange exchange
 * @param [in] if_unused if_unused
 * @returns amqp_exchange_delete_ok_t
 */
AMQP_PUBLIC_FUNCTION amqp_exchange_delete_ok_t * amqp_exchange_delete(amqp_connection_state_t state, amqp_channel_t channel,
    amqp_bytes_t exchange, boolint if_unused) 
{
	amqp_exchange_delete_t req;
	req.ticket = 0;
	req.exchange = exchange;
	req.if_unused = if_unused;
	req.nowait = 0;
	return static_cast<amqp_exchange_delete_ok_t *>(amqp_simple_rpc_decoded(state, channel, AMQP_EXCHANGE_DELETE_METHOD, AMQP_EXCHANGE_DELETE_OK_METHOD, &req));
}
/**
 * amqp_exchange_bind
 *
 * @param [in] state connection state
 * @param [in] channel the channel to do the RPC on
 * @param [in] destination destination
 * @param [in] source source
 * @param [in] routing_key routing_key
 * @param [in] arguments arguments
 * @returns amqp_exchange_bind_ok_t
 */
AMQP_PUBLIC_FUNCTION amqp_exchange_bind_ok_t * amqp_exchange_bind(amqp_connection_state_t state, amqp_channel_t channel,
    amqp_bytes_t destination, amqp_bytes_t source, amqp_bytes_t routing_key, amqp_table_t arguments) 
{
	amqp_exchange_bind_t req;
	req.ticket = 0;
	req.destination = destination;
	req.source = source;
	req.routing_key = routing_key;
	req.nowait = 0;
	req.arguments = arguments;
	return static_cast<amqp_exchange_bind_ok_t *>(amqp_simple_rpc_decoded(state, channel, AMQP_EXCHANGE_BIND_METHOD, AMQP_EXCHANGE_BIND_OK_METHOD, &req));
}
/**
 * amqp_exchange_unbind
 *
 * @param [in] state connection state
 * @param [in] channel the channel to do the RPC on
 * @param [in] destination destination
 * @param [in] source source
 * @param [in] routing_key routing_key
 * @param [in] arguments arguments
 * @returns amqp_exchange_unbind_ok_t
 */
AMQP_PUBLIC_FUNCTION amqp_exchange_unbind_ok_t * amqp_exchange_unbind(amqp_connection_state_t state, amqp_channel_t channel,
    amqp_bytes_t destination, amqp_bytes_t source, amqp_bytes_t routing_key, amqp_table_t arguments) 
{
	amqp_exchange_unbind_t req;
	req.ticket = 0;
	req.destination = destination;
	req.source = source;
	req.routing_key = routing_key;
	req.nowait = 0;
	req.arguments = arguments;
	return static_cast<amqp_exchange_unbind_ok_t *>(amqp_simple_rpc_decoded(state, channel, AMQP_EXCHANGE_UNBIND_METHOD, AMQP_EXCHANGE_UNBIND_OK_METHOD, &req));
}
/**
 * amqp_queue_declare
 *
 * @param [in] state connection state
 * @param [in] channel the channel to do the RPC on
 * @param [in] queue queue
 * @param [in] passive passive
 * @param [in] durable durable
 * @param [in] exclusive exclusive
 * @param [in] auto_delete auto_delete
 * @param [in] arguments arguments
 * @returns amqp_queue_declare_ok_t
 */
AMQP_PUBLIC_FUNCTION amqp_queue_declare_ok_t * amqp_queue_declare(amqp_connection_state_t state, amqp_channel_t channel, amqp_bytes_t queue,
    boolint passive, boolint durable, boolint exclusive, boolint auto_delete, amqp_table_t arguments) 
{
	amqp_queue_declare_t req;
	req.ticket = 0;
	req.queue = queue;
	req.passive = passive;
	req.durable = durable;
	req.exclusive = exclusive;
	req.auto_delete = auto_delete;
	req.nowait = 0;
	req.arguments = arguments;
	return static_cast<amqp_queue_declare_ok_t *>(amqp_simple_rpc_decoded(state, channel, AMQP_QUEUE_DECLARE_METHOD, AMQP_QUEUE_DECLARE_OK_METHOD, &req));
}
/**
 * amqp_queue_bind
 *
 * @param [in] state connection state
 * @param [in] channel the channel to do the RPC on
 * @param [in] queue queue
 * @param [in] exchange exchange
 * @param [in] routing_key routing_key
 * @param [in] arguments arguments
 * @returns amqp_queue_bind_ok_t
 */
AMQP_PUBLIC_FUNCTION amqp_queue_bind_ok_t * amqp_queue_bind(amqp_connection_state_t state, amqp_channel_t channel, amqp_bytes_t queue,
    amqp_bytes_t exchange, amqp_bytes_t routing_key, amqp_table_t arguments) 
{
	amqp_queue_bind_t req;
	req.ticket = 0;
	req.queue = queue;
	req.exchange = exchange;
	req.routing_key = routing_key;
	req.nowait = 0;
	req.arguments = arguments;
	return static_cast<amqp_queue_bind_ok_t *>(amqp_simple_rpc_decoded(state, channel, AMQP_QUEUE_BIND_METHOD, AMQP_QUEUE_BIND_OK_METHOD, &req));
}
/**
 * amqp_queue_purge
 *
 * @param [in] state connection state
 * @param [in] channel the channel to do the RPC on
 * @param [in] queue queue
 * @returns amqp_queue_purge_ok_t
 */
AMQP_PUBLIC_FUNCTION amqp_queue_purge_ok_t * amqp_queue_purge(amqp_connection_state_t state, amqp_channel_t channel, amqp_bytes_t queue) 
{
	amqp_queue_purge_t req;
	req.ticket = 0;
	req.queue = queue;
	req.nowait = 0;
	return static_cast<amqp_queue_purge_ok_t *>(amqp_simple_rpc_decoded(state, channel, AMQP_QUEUE_PURGE_METHOD, AMQP_QUEUE_PURGE_OK_METHOD, &req));
}
/**
 * amqp_queue_delete
 *
 * @param [in] state connection state
 * @param [in] channel the channel to do the RPC on
 * @param [in] queue queue
 * @param [in] if_unused if_unused
 * @param [in] if_empty if_empty
 * @returns amqp_queue_delete_ok_t
 */
AMQP_PUBLIC_FUNCTION amqp_queue_delete_ok_t * amqp_queue_delete(amqp_connection_state_t state, amqp_channel_t channel, amqp_bytes_t queue,
    boolint if_unused, boolint if_empty) 
{
	amqp_queue_delete_t req;
	req.ticket = 0;
	req.queue = queue;
	req.if_unused = if_unused;
	req.if_empty = if_empty;
	req.nowait = 0;
	return static_cast<amqp_queue_delete_ok_t *>(amqp_simple_rpc_decoded(state, channel, AMQP_QUEUE_DELETE_METHOD, AMQP_QUEUE_DELETE_OK_METHOD, &req));
}
/**
 * amqp_queue_unbind
 *
 * @param [in] state connection state
 * @param [in] channel the channel to do the RPC on
 * @param [in] queue queue
 * @param [in] exchange exchange
 * @param [in] routing_key routing_key
 * @param [in] arguments arguments
 * @returns amqp_queue_unbind_ok_t
 */
AMQP_PUBLIC_FUNCTION amqp_queue_unbind_ok_t * amqp_queue_unbind(amqp_connection_state_t state, amqp_channel_t channel, amqp_bytes_t queue,
    amqp_bytes_t exchange, amqp_bytes_t routing_key, amqp_table_t arguments) 
{
	amqp_queue_unbind_t req;
	req.ticket = 0;
	req.queue = queue;
	req.exchange = exchange;
	req.routing_key = routing_key;
	req.arguments = arguments;
	return static_cast<amqp_queue_unbind_ok_t *>(amqp_simple_rpc_decoded(state, channel, AMQP_QUEUE_UNBIND_METHOD, AMQP_QUEUE_UNBIND_OK_METHOD, &req));
}
/**
 * amqp_basic_qos
 *
 * @param [in] state connection state
 * @param [in] channel the channel to do the RPC on
 * @param [in] prefetch_size prefetch_size
 * @param [in] prefetch_count prefetch_count
 * @param [in] global global
 * @returns amqp_basic_qos_ok_t
 */
AMQP_PUBLIC_FUNCTION amqp_basic_qos_ok_t * amqp_basic_qos(amqp_connection_state_t state, amqp_channel_t channel, 
	uint32 prefetch_size, uint16 prefetch_count, boolint global) 
{
	amqp_basic_qos_t req;
	req.prefetch_size = prefetch_size;
	req.prefetch_count = prefetch_count;
	req.global = global;
	return static_cast<amqp_basic_qos_ok_t *>(amqp_simple_rpc_decoded(state, channel, AMQP_BASIC_QOS_METHOD, AMQP_BASIC_QOS_OK_METHOD, &req));
}
/**
 * amqp_basic_consume
 *
 * @param [in] state connection state
 * @param [in] channel the channel to do the RPC on
 * @param [in] queue queue
 * @param [in] consumer_tag consumer_tag
 * @param [in] no_local no_local
 * @param [in] no_ack no_ack
 * @param [in] exclusive exclusive
 * @param [in] arguments arguments
 * @returns amqp_basic_consume_ok_t
 */
AMQP_PUBLIC_FUNCTION amqp_basic_consume_ok_t * amqp_basic_consume(amqp_connection_state_t state, amqp_channel_t channel, amqp_bytes_t queue,
    amqp_bytes_t consumer_tag, boolint no_local, boolint no_ack, boolint exclusive, amqp_table_t arguments) 
{
	amqp_basic_consume_t req;
	req.ticket = 0;
	req.queue = queue;
	req.consumer_tag = consumer_tag;
	req.no_local = no_local;
	req.no_ack = no_ack;
	req.exclusive = exclusive;
	req.nowait = 0;
	req.arguments = arguments;
	return static_cast<amqp_basic_consume_ok_t *>(amqp_simple_rpc_decoded(state, channel, AMQP_BASIC_CONSUME_METHOD, AMQP_BASIC_CONSUME_OK_METHOD, &req));
}
/**
 * amqp_basic_cancel
 *
 * @param [in] state connection state
 * @param [in] channel the channel to do the RPC on
 * @param [in] consumer_tag consumer_tag
 * @returns amqp_basic_cancel_ok_t
 */
AMQP_PUBLIC_FUNCTION amqp_basic_cancel_ok_t * amqp_basic_cancel(amqp_connection_state_t state, amqp_channel_t channel, amqp_bytes_t consumer_tag) 
{
	amqp_basic_cancel_t req;
	req.consumer_tag = consumer_tag;
	req.nowait = 0;
	return static_cast<amqp_basic_cancel_ok_t *>(amqp_simple_rpc_decoded(state, channel, AMQP_BASIC_CANCEL_METHOD, AMQP_BASIC_CANCEL_OK_METHOD, &req));
}
/**
 * amqp_basic_recover
 *
 * @param [in] state connection state
 * @param [in] channel the channel to do the RPC on
 * @param [in] requeue requeue
 * @returns amqp_basic_recover_ok_t
 */
AMQP_PUBLIC_FUNCTION amqp_basic_recover_ok_t * amqp_basic_recover(amqp_connection_state_t state, amqp_channel_t channel, boolint requeue) 
{
	amqp_basic_recover_t req;
	req.requeue = requeue;
	return static_cast<amqp_basic_recover_ok_t *>(amqp_simple_rpc_decoded(state, channel, AMQP_BASIC_RECOVER_METHOD, AMQP_BASIC_RECOVER_OK_METHOD, &req));
}
/**
 * amqp_tx_select
 *
 * @param [in] state connection state
 * @param [in] channel the channel to do the RPC on
 * @returns amqp_tx_select_ok_t
 */
AMQP_PUBLIC_FUNCTION amqp_tx_select_ok_t * amqp_tx_select(amqp_connection_state_t state, amqp_channel_t channel) 
{
	amqp_tx_select_t req;
	return static_cast<amqp_tx_select_ok_t *>(amqp_simple_rpc_decoded(state, channel, AMQP_TX_SELECT_METHOD, AMQP_TX_SELECT_OK_METHOD, &req));
}
/**
 * amqp_tx_commit
 *
 * @param [in] state connection state
 * @param [in] channel the channel to do the RPC on
 * @returns amqp_tx_commit_ok_t
 */
AMQP_PUBLIC_FUNCTION amqp_tx_commit_ok_t * amqp_tx_commit(amqp_connection_state_t state, amqp_channel_t channel) 
{
	amqp_tx_commit_t req;
	return static_cast<amqp_tx_commit_ok_t *>(amqp_simple_rpc_decoded(state, channel, AMQP_TX_COMMIT_METHOD, AMQP_TX_COMMIT_OK_METHOD, &req));
}
/**
 * amqp_tx_rollback
 *
 * @param [in] state connection state
 * @param [in] channel the channel to do the RPC on
 * @returns amqp_tx_rollback_ok_t
 */
AMQP_PUBLIC_FUNCTION amqp_tx_rollback_ok_t * amqp_tx_rollback(amqp_connection_state_t state, amqp_channel_t channel) 
{
	amqp_tx_rollback_t req;
	return static_cast<amqp_tx_rollback_ok_t *>(amqp_simple_rpc_decoded(state, channel, AMQP_TX_ROLLBACK_METHOD, AMQP_TX_ROLLBACK_OK_METHOD, &req));
}
/**
 * amqp_confirm_select
 *
 * @param [in] state connection state
 * @param [in] channel the channel to do the RPC on
 * @returns amqp_confirm_select_ok_t
 */
AMQP_PUBLIC_FUNCTION amqp_confirm_select_ok_t * amqp_confirm_select(amqp_connection_state_t state, amqp_channel_t channel) 
{
	amqp_confirm_select_t req;
	req.nowait = 0;
	return static_cast<amqp_confirm_select_ok_t *>(amqp_simple_rpc_decoded(state, channel, AMQP_CONFIRM_SELECT_METHOD, AMQP_CONFIRM_SELECT_OK_METHOD, &req));
}
// } AMQP_FRAMING
//
// AMQP_CONSUMER {
//
static int amqp_basic_properties_clone(amqp_basic_properties_t * original, amqp_basic_properties_t * clone, amqp_pool_t * pool) 
{
	memzero(clone, sizeof(*clone));
	clone->_flags = original->_flags;
#define CLONE_BYTES_POOL(original, clone, pool)        \
	if(!original.len) {                             \
		clone = amqp_empty_bytes;                          \
	} else {                                             \
		amqp_pool_alloc_bytes(pool, original.len, &clone); \
		if(!clone.bytes)                                   \
			return AMQP_STATUS_NO_MEMORY;                    \
		memcpy(clone.bytes, original.bytes, clone.len);    \
	}

	if(clone->_flags & AMQP_BASIC_CONTENT_TYPE_FLAG) {
		CLONE_BYTES_POOL(original->content_type, clone->content_type, pool)
	}
	if(clone->_flags & AMQP_BASIC_CONTENT_ENCODING_FLAG) {
		CLONE_BYTES_POOL(original->content_encoding, clone->content_encoding, pool)
	}
	if(clone->_flags & AMQP_BASIC_HEADERS_FLAG) {
		int res = amqp_table_clone(&original->headers, &clone->headers, pool);
		if(res != AMQP_STATUS_OK)
			return res;
	}
	if(clone->_flags & AMQP_BASIC_DELIVERY_MODE_FLAG) {
		clone->delivery_mode = original->delivery_mode;
	}
	if(clone->_flags & AMQP_BASIC_PRIORITY_FLAG) {
		clone->priority = original->priority;
	}
	if(clone->_flags & AMQP_BASIC_CORRELATION_ID_FLAG) {
		CLONE_BYTES_POOL(original->correlation_id, clone->correlation_id, pool)
	}
	if(clone->_flags & AMQP_BASIC_REPLY_TO_FLAG) {
		CLONE_BYTES_POOL(original->reply_to, clone->reply_to, pool)
	}
	if(clone->_flags & AMQP_BASIC_EXPIRATION_FLAG) {
		CLONE_BYTES_POOL(original->expiration, clone->expiration, pool)
	}
	if(clone->_flags & AMQP_BASIC_MESSAGE_ID_FLAG) {
		CLONE_BYTES_POOL(original->message_id, clone->message_id, pool)
	}
	if(clone->_flags & AMQP_BASIC_TIMESTAMP_FLAG) {
		clone->timestamp = original->timestamp;
	}
	if(clone->_flags & AMQP_BASIC_TYPE_FLAG) {
		CLONE_BYTES_POOL(original->type, clone->type, pool)
	}
	if(clone->_flags & AMQP_BASIC_USER_ID_FLAG) {
		CLONE_BYTES_POOL(original->user_id, clone->user_id, pool)
	}
	if(clone->_flags & AMQP_BASIC_APP_ID_FLAG) {
		CLONE_BYTES_POOL(original->app_id, clone->app_id, pool)
	}
	if(clone->_flags & AMQP_BASIC_CLUSTER_ID_FLAG) {
		CLONE_BYTES_POOL(original->cluster_id, clone->cluster_id, pool)
	}
	return AMQP_STATUS_OK;
#undef CLONE_BYTES_POOL
}

void amqp_destroy_message(amqp_message_t * message) 
{
	empty_amqp_pool(&message->pool);
	amqp_bytes_free(message->body);
}

void amqp_destroy_envelope(amqp_envelope_t * pEnvelope) 
{
	if(pEnvelope) {
		amqp_destroy_message(&pEnvelope->message);
		amqp_bytes_free(pEnvelope->routing_key);
		amqp_bytes_free(pEnvelope->exchange);
		amqp_bytes_free(pEnvelope->consumer_tag);
	}
}

static int FASTCALL amqp_bytes_malloc_dup_failed(const amqp_bytes_t & rBytes) { return BIN(rBytes.len && !rBytes.bytes); }

amqp_rpc_reply_t amqp_consume_message(amqp_connection_state_t state, amqp_envelope_t * envelope, struct timeval * timeout, AMQP_UNUSED int flags) 
{
	int res;
	amqp_frame_t frame;
	amqp_basic_deliver_t * delivery_method;
	amqp_rpc_reply_t ret;
	memzero(envelope, sizeof(*envelope));
	res = amqp_simple_wait_frame_noblock(state, &frame, timeout);
	if(res != AMQP_STATUS_OK) {
		ret.reply_type = AMQP_RESPONSE_LIBRARY_EXCEPTION;
		ret.library_error = res;
		goto error_out1;
	}
	if(AMQP_FRAME_METHOD != frame.frame_type || AMQP_BASIC_DELIVER_METHOD != frame.payload.method.id) {
		amqp_put_back_frame(state, &frame);
		ret.reply_type = AMQP_RESPONSE_LIBRARY_EXCEPTION;
		ret.library_error = AMQP_STATUS_UNEXPECTED_STATE;
		goto error_out1;
	}
	delivery_method = static_cast<amqp_basic_deliver_t *>(frame.payload.method.decoded);
	envelope->channel = frame.channel;
	envelope->consumer_tag = amqp_bytes_malloc_dup(delivery_method->consumer_tag);
	envelope->delivery_tag = delivery_method->delivery_tag;
	envelope->redelivered = delivery_method->redelivered;
	envelope->exchange = amqp_bytes_malloc_dup(delivery_method->exchange);
	envelope->routing_key = amqp_bytes_malloc_dup(delivery_method->routing_key);
	if(amqp_bytes_malloc_dup_failed(envelope->consumer_tag) || amqp_bytes_malloc_dup_failed(envelope->exchange) || amqp_bytes_malloc_dup_failed(envelope->routing_key)) {
		ret.reply_type = AMQP_RESPONSE_LIBRARY_EXCEPTION;
		ret.library_error = AMQP_STATUS_NO_MEMORY;
		goto error_out2;
	}
	ret = amqp_read_message(state, envelope->channel, &envelope->message, 0);
	if(AMQP_RESPONSE_NORMAL != ret.reply_type) {
		goto error_out2;
	}
	ret.reply_type = AMQP_RESPONSE_NORMAL;
	return ret;
error_out2:
	amqp_bytes_free(envelope->routing_key);
	amqp_bytes_free(envelope->exchange);
	amqp_bytes_free(envelope->consumer_tag);
error_out1:
	return ret;
}

amqp_rpc_reply_t amqp_read_message(amqp_connection_state_t state, amqp_channel_t channel, amqp_message_t * message, AMQP_UNUSED int flags) 
{
	amqp_frame_t frame;
	amqp_rpc_reply_t ret;
	size_t body_read;
	char * body_read_ptr;
	int res;
	memzero(message, sizeof(*message));
	res = amqp_simple_wait_frame_on_channel(state, channel, &frame);
	if(res != AMQP_STATUS_OK) {
		ret.reply_type = AMQP_RESPONSE_LIBRARY_EXCEPTION;
		ret.library_error = res;
		goto error_out1;
	}
	if(AMQP_FRAME_HEADER != frame.frame_type) {
		if(frame.frame_type == AMQP_FRAME_METHOD && oneof2(frame.payload.method.id, AMQP_CHANNEL_CLOSE_METHOD, AMQP_CONNECTION_CLOSE_METHOD)) {
			ret.reply_type = AMQP_RESPONSE_SERVER_EXCEPTION;
			ret.reply = frame.payload.method;
		}
		else {
			ret.reply_type = AMQP_RESPONSE_LIBRARY_EXCEPTION;
			ret.library_error = AMQP_STATUS_UNEXPECTED_STATE;
			amqp_put_back_frame(state, &frame);
		}
		goto error_out1;
	}
	init_amqp_pool(&message->pool, 4096);
	res = amqp_basic_properties_clone(static_cast<amqp_basic_properties_t *>(frame.payload.properties.decoded), &message->properties, &message->pool);
	if(res != AMQP_STATUS_OK) {
		ret.reply_type = AMQP_RESPONSE_LIBRARY_EXCEPTION;
		ret.library_error = res;
		goto error_out3;
	}
	if(!frame.payload.properties.body_size)
		message->body = amqp_empty_bytes;
	else {
		if(SIZE_MAX < frame.payload.properties.body_size) {
			ret.reply_type = AMQP_RESPONSE_LIBRARY_EXCEPTION;
			ret.library_error = AMQP_STATUS_NO_MEMORY;
			goto error_out1;
		}
		message->body = amqp_bytes_malloc((size_t)frame.payload.properties.body_size);
		if(!message->body.bytes) {
			ret.reply_type = AMQP_RESPONSE_LIBRARY_EXCEPTION;
			ret.library_error = AMQP_STATUS_NO_MEMORY;
			goto error_out1;
		}
	}
	body_read = 0;
	body_read_ptr = static_cast<char *>(message->body.bytes);
	while(body_read < message->body.len) {
		res = amqp_simple_wait_frame_on_channel(state, channel, &frame);
		if(res != AMQP_STATUS_OK) {
			ret.reply_type = AMQP_RESPONSE_LIBRARY_EXCEPTION;
			ret.library_error = res;
			goto error_out2;
		}
		if(AMQP_FRAME_BODY != frame.frame_type) {
			if(AMQP_FRAME_METHOD == frame.frame_type && (AMQP_CHANNEL_CLOSE_METHOD == frame.payload.method.id || AMQP_CONNECTION_CLOSE_METHOD == frame.payload.method.id)) {
				ret.reply_type = AMQP_RESPONSE_SERVER_EXCEPTION;
				ret.reply = frame.payload.method;
			}
			else {
				ret.reply_type = AMQP_RESPONSE_LIBRARY_EXCEPTION;
				ret.library_error = AMQP_STATUS_BAD_AMQP_DATA;
			}
			goto error_out2;
		}
		if(body_read + frame.payload.body_fragment.len > message->body.len) {
			ret.reply_type = AMQP_RESPONSE_LIBRARY_EXCEPTION;
			ret.library_error = AMQP_STATUS_BAD_AMQP_DATA;
			goto error_out2;
		}
		memcpy(body_read_ptr, frame.payload.body_fragment.bytes, frame.payload.body_fragment.len);
		body_read += frame.payload.body_fragment.len;
		body_read_ptr += frame.payload.body_fragment.len;
	}
	ret.reply_type = AMQP_RESPONSE_NORMAL;
	return ret;
error_out2:
	amqp_bytes_free(message->body);
error_out3:
	empty_amqp_pool(&message->pool);
error_out1:
	return ret;
}
// } AMQP_CONSUMER
//
// AMQP_TIME {
//
#if (defined(_WIN32) || defined(__WIN32__) || defined(WIN32) || defined(__MINGW32__) || defined(__MINGW64__))
	#define AMQP_WIN_TIMER_API
#elif (defined(machintosh) || defined(__APPLE__) || defined(__APPLE_CC__))
	#define AMQP_MAC_TIMER_API
#else
	#define AMQP_POSIX_TIMER_API
#endif
#ifdef AMQP_WIN_TIMER_API
	#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN
	#endif

	uint64 amqp_get_monotonic_timestamp() 
	{
		static double NS_PER_COUNT = 0.0;
		LARGE_INTEGER perf_count;
		if(NS_PER_COUNT == 0.0) {
			LARGE_INTEGER perf_frequency;
			if(!QueryPerformanceFrequency(&perf_frequency))
				return 0;
			NS_PER_COUNT = (double)AMQP_NS_PER_S / perf_frequency.QuadPart;
		}
		return QueryPerformanceCounter(&perf_count) ? static_cast<uint64>(perf_count.QuadPart * NS_PER_COUNT) : 0;
	}
#endif
#ifdef AMQP_MAC_TIMER_API
	#include <mach/mach_time.h>

	uint64 amqp_get_monotonic_timestamp() 
	{
		static mach_timebase_info_data_t s_timebase = {0, 0};
		uint64 timestamp = mach_absolute_time();
		if(s_timebase.denom == 0) {
			mach_timebase_info(&s_timebase);
			if(0 == s_timebase.denom) {
				return 0;
			}
		}
		timestamp *= (uint64)s_timebase.numer;
		timestamp /= (uint64)s_timebase.denom;
		return timestamp;
	}
#endif
#ifdef AMQP_POSIX_TIMER_API
	#include <time.h>

	uint64 amqp_get_monotonic_timestamp() 
	{
	#ifdef __hpux
		return (uint64)gethrtime();
	#else
		struct timespec tp;
		if(-1 == clock_gettime(CLOCK_MONOTONIC, &tp)) {
			return 0;
		}
		return ((uint64)tp.tv_sec * AMQP_NS_PER_S + (uint64)tp.tv_nsec);
	#endif
	}
#endif

int amqp_time_from_now(amqp_time_t * time, struct timeval * timeout) 
{
	assert(time);
	if(!timeout) {
		*time = amqp_time_infinite();
		return AMQP_STATUS_OK;
	}
	else if(!timeout->tv_sec && !timeout->tv_usec) {
		*time = amqp_time_immediate();
		return AMQP_STATUS_OK;
	}
	else if(timeout->tv_sec < 0 || timeout->tv_usec < 0) {
		return AMQP_STATUS_INVALID_PARAMETER;
	}
	else {
		uint64 delta_ns = (uint64)timeout->tv_sec * AMQP_NS_PER_S + (uint64)timeout->tv_usec * AMQP_NS_PER_US;
		uint64 now_ns = amqp_get_monotonic_timestamp();
		if(!now_ns)
			return AMQP_STATUS_TIMER_FAILURE;
		else {
			time->time_point_ns = now_ns + delta_ns;
			return (now_ns > time->time_point_ns || delta_ns > time->time_point_ns) ? AMQP_STATUS_INVALID_PARAMETER : AMQP_STATUS_OK;
		}
	}
}

int FASTCALL amqp_time_s_from_now(amqp_time_t * time, int seconds) 
{
	uint64 now_ns;
	uint64 delta_ns;
	assert(time);
	if(0 >= seconds) {
		*time = amqp_time_infinite();
		return AMQP_STATUS_OK;
	}
	now_ns = amqp_get_monotonic_timestamp();
	if(0 == now_ns) {
		return AMQP_STATUS_TIMER_FAILURE;
	}
	delta_ns = (uint64)seconds * AMQP_NS_PER_S;
	time->time_point_ns = now_ns + delta_ns;
	return (now_ns > time->time_point_ns || delta_ns > time->time_point_ns) ? AMQP_STATUS_INVALID_PARAMETER : AMQP_STATUS_OK;
}

amqp_time_t amqp_time_immediate() 
{
	amqp_time_t time;
	time.time_point_ns = 0;
	return time;
}

amqp_time_t amqp_time_infinite() 
{
	amqp_time_t time;
	time.time_point_ns = UINT64_MAX;
	return time;
}

int amqp_time_ms_until(amqp_time_t time) 
{
	if(UINT64_MAX == time.time_point_ns)
		return -1;
	else if(!time.time_point_ns)
		return 0;
	else {
		uint64 now_ns = amqp_get_monotonic_timestamp();
		if(!now_ns)
			return AMQP_STATUS_TIMER_FAILURE;
		else if(now_ns >= time.time_point_ns)
			return 0;
		else {
			uint64 delta_ns = time.time_point_ns - now_ns;
			int left_ms = (int)(delta_ns / AMQP_NS_PER_MS);
			return left_ms;
		}
	}
}

int FASTCALL amqp_time_tv_until(amqp_time_t time, struct timeval * in, struct timeval ** out) 
{
	uint64 now_ns;
	uint64 delta_ns;
	assert(in != NULL);
	if(time.time_point_ns == UINT64_MAX) {
		*out = NULL;
		return AMQP_STATUS_OK;
	}
	if(!time.time_point_ns) {
		in->tv_sec = 0;
		in->tv_usec = 0;
		*out = in;
		return AMQP_STATUS_OK;
	}
	now_ns = amqp_get_monotonic_timestamp();
	if(!now_ns) {
		return AMQP_STATUS_TIMER_FAILURE;
	}
	if(now_ns >= time.time_point_ns) {
		in->tv_sec = 0;
		in->tv_usec = 0;
		*out = in;
		return AMQP_STATUS_OK;
	}
	delta_ns = time.time_point_ns - now_ns;
	in->tv_sec = (int)(delta_ns / AMQP_NS_PER_S);
	in->tv_usec = (int)((delta_ns % AMQP_NS_PER_S) / AMQP_NS_PER_US);
	*out = in;
	return AMQP_STATUS_OK;
}

int amqp_time_has_past(amqp_time_t time) 
{
	if(time.time_point_ns == UINT64_MAX)
		return AMQP_STATUS_OK;
	else {
		uint64 now_ns = amqp_get_monotonic_timestamp();
		if(!now_ns)
			return AMQP_STATUS_TIMER_FAILURE;
		else if(now_ns > time.time_point_ns)
			return AMQP_STATUS_TIMEOUT;
		else
			return AMQP_STATUS_OK;
	}
}

amqp_time_t amqp_time_first(amqp_time_t l, amqp_time_t r) { return (l.time_point_ns < r.time_point_ns) ? l : r; }
int amqp_time_equal(amqp_time_t l, amqp_time_t r) { return l.time_point_ns == r.time_point_ns; }
// } AMQP_TIME
//
// AMQP_MEM {
//
char const * amqp_version() { return AMQP_VERSION_STRING; }
uint32 amqp_version_number() { return AMQP_VERSION; }

void FASTCALL init_amqp_pool(amqp_pool_t * pPool, size_t pagesize) 
{
	if(pPool) {
		pPool->pagesize = pagesize ? pagesize : 4096;
		pPool->pages.num_blocks = 0;
		pPool->pages.blocklist = NULL;
		pPool->large_blocks.num_blocks = 0;
		pPool->large_blocks.blocklist = NULL;
		pPool->next_page = 0;
		pPool->alloc_block = NULL;
		pPool->alloc_used = 0;
	}
}

static void FASTCALL empty_blocklist(amqp_pool_blocklist_t * x) 
{
	if(x->blocklist) {
		for(int i = 0; i < x->num_blocks; i++) {
			SAlloc::F(x->blocklist[i]);
		}
		SAlloc::F(x->blocklist);
	}
	x->num_blocks = 0;
	x->blocklist = NULL;
}

void recycle_amqp_pool(amqp_pool_t * pPool) 
{
	if(pPool) {
		empty_blocklist(&pPool->large_blocks);
		pPool->next_page = 0;
		pPool->alloc_block = NULL;
		pPool->alloc_used = 0;
	}
}

void FASTCALL empty_amqp_pool(amqp_pool_t * pPool) 
{
	if(pPool) {
		recycle_amqp_pool(pPool);
		empty_blocklist(&pPool->pages);
	}
}
//
// Returns 1 on success, 0 on failure 
//
static int record_pool_block(amqp_pool_blocklist_t * x, void * block) 
{
	int    ok = 1;
	const  size_t blocklistlength = sizeof(void *) * (x->num_blocks + 1);
	if(!x->blocklist) {
		x->blocklist = static_cast<void **>(SAlloc::M(blocklistlength));
		THROW(x->blocklist);
	}
	else {
		void * newbl = SAlloc::R(x->blocklist, blocklistlength);
		THROW(newbl)
		x->blocklist = static_cast<void **>(newbl);
	}
	x->blocklist[x->num_blocks] = block;
	x->num_blocks++;
	CATCHZOK
	return ok;
}

void * FASTCALL amqp_pool_alloc(amqp_pool_t * pool, size_t amount) 
{
	void * p_result = 0;
	if(amount) {
		amount = (amount + 7) & (~7); // round up to nearest 8-byte boundary 
		if(amount > pool->pagesize) {
			THROW(p_result = SAlloc::C(1, amount));
			THROW(record_pool_block(&pool->large_blocks, p_result));
		}
		else {
			assert(!pool->alloc_block || pool->alloc_used <= pool->pagesize);
			if(pool->alloc_block && (pool->alloc_used + amount) <= pool->pagesize) {
				p_result = pool->alloc_block + pool->alloc_used;
				pool->alloc_used += amount;
			}
			else {
				if(pool->next_page >= pool->pages.num_blocks) {
					THROW(pool->alloc_block = static_cast<char *>(SAlloc::C(1, pool->pagesize)));
					THROW(record_pool_block(&pool->pages, pool->alloc_block));
					pool->next_page = pool->pages.num_blocks;
				}
				else {
					pool->alloc_block = static_cast<char *>(pool->pages.blocklist[pool->next_page]);
					pool->next_page++;
				}
				pool->alloc_used = amount;
				p_result = pool->alloc_block;
			}
		}
	}
	CATCH
		ZFREE(p_result);
	ENDCATCH
	return p_result;
}

void FASTCALL amqp_pool_alloc_bytes(amqp_pool_t * pool, size_t amount, amqp_bytes_t * output) 
{
	output->len = amount;
	output->bytes = amqp_pool_alloc(pool, amount);
}

amqp_bytes_t FASTCALL amqp_cstring_bytes(char const * cstr) 
{
	amqp_bytes_t result;
	result.len = sstrlen(cstr);
	result.bytes = const_cast<void *>(static_cast<const void *>(cstr)); // @badcast
	return result;
}

amqp_bytes_t FASTCALL amqp_bytes_malloc_dup(const amqp_bytes_t & rS) 
{
	amqp_bytes_t result;
	result.len = rS.len;
	result.bytes = SAlloc::M(rS.len);
	if(result.bytes)
		memcpy(result.bytes, rS.bytes, rS.len);
	return result;
}

amqp_bytes_t amqp_bytes_malloc(size_t amount) 
{
	amqp_bytes_t result;
	result.len = amount;
	result.bytes = SAlloc::M(amount); // will return NULL if it fails 
	return result;
}

void FASTCALL amqp_bytes_free(amqp_bytes_t & rBytes) 
{
	SAlloc::F(rBytes.bytes);
	rBytes.bytes = 0; // @sobolev
	rBytes.len = 0; // @sobolev
}

amqp_pool_t * amqp_get_or_create_channel_pool(amqp_connection_state_t state, amqp_channel_t channel) 
{
	size_t index = channel % POOL_TABLE_SIZE;
	amqp_pool_table_entry_t * entry = state->pool_table[index];
	for(; entry; entry = entry->next) {
		if(channel == entry->channel)
			return &entry->pool;
	}
	entry = static_cast<amqp_pool_table_entry_t *>(SAlloc::M(sizeof(amqp_pool_table_entry_t)));
	if(entry) {
		entry->channel = channel;
		entry->next = state->pool_table[index];
		state->pool_table[index] = entry;
		init_amqp_pool(&entry->pool, state->frame_max);
		return &entry->pool;
	}
	else
		return 0;
}

static amqp_pool_t * amqp_get_channel_pool(amqp_connection_state_t state, amqp_channel_t channel) 
{
	size_t index = channel % POOL_TABLE_SIZE;
	for(amqp_pool_table_entry_t * entry = state->pool_table[index]; entry; entry = entry->next) {
		if(channel == entry->channel)
			return &entry->pool;
	}
	return NULL;
}

int amqp_bytes_equal(const amqp_bytes_t r, const amqp_bytes_t l) 
{
	return BIN(r.len == l.len && (r.bytes == l.bytes || 0 == memcmp(r.bytes, l.bytes, r.len)));
}
// } AMQP_MEM
//
// AMQP_TABLE {
//
#define INITIAL_ARRAY_SIZE 16
#define INITIAL_TABLE_SIZE 16

static int amqp_decode_field_value(amqp_bytes_t encoded, amqp_pool_t * pool, amqp_field_value_t * entry, size_t * offset);
static int amqp_encode_field_value(amqp_bytes_t encoded, amqp_field_value_t * entry, size_t * offset);

static int amqp_decode_array(amqp_bytes_t encoded, amqp_pool_t * pool, amqp_array_t * output, size_t * offset) 
{
	uint32 arraysize;
	int num_entries = 0;
	int allocated_entries = INITIAL_ARRAY_SIZE;
	amqp_field_value_t * entries;
	size_t limit;
	int res;
	if(!amqp_decode_32(encoded, offset, &arraysize)) {
		return AMQP_STATUS_BAD_AMQP_DATA;
	}
	if(arraysize + *offset > encoded.len) {
		return AMQP_STATUS_BAD_AMQP_DATA;
	}
	entries = static_cast<amqp_field_value_t *>(SAlloc::M(allocated_entries * sizeof(amqp_field_value_t)));
	if(entries == NULL) {
		return AMQP_STATUS_NO_MEMORY;
	}
	limit = *offset + arraysize;
	while(*offset < limit) {
		if(num_entries >= allocated_entries) {
			allocated_entries = allocated_entries * 2;
			void * newentries = SAlloc::R(entries, allocated_entries * sizeof(amqp_field_value_t));
			res = AMQP_STATUS_NO_MEMORY;
			if(!newentries) {
				goto out;
			}
			entries = static_cast<amqp_field_value_t *>(newentries);
		}
		res = amqp_decode_field_value(encoded, pool, &entries[num_entries], offset);
		if(res < 0) {
			goto out;
		}
		num_entries++;
	}
	output->num_entries = num_entries;
	output->entries = static_cast<amqp_field_value_t *>(amqp_pool_alloc(pool, num_entries * sizeof(amqp_field_value_t)));
	// NULL is legitimate if we requested a zero-length block
	if(!output->entries) {
		res = (num_entries == 0) ? AMQP_STATUS_OK : AMQP_STATUS_NO_MEMORY;
		goto out;
	}
	memcpy(output->entries, entries, num_entries * sizeof(amqp_field_value_t));
	res = AMQP_STATUS_OK;
out:
	SAlloc::F(entries);
	return res;
}

int amqp_decode_table(amqp_bytes_t encoded, amqp_pool_t * pool, amqp_table_t * output, size_t * offset) 
{
	uint32 tablesize;
	int num_entries = 0;
	amqp_table_entry_t * entries;
	int allocated_entries = INITIAL_TABLE_SIZE;
	size_t limit;
	int res;
	if(!amqp_decode_32(encoded, offset, &tablesize)) {
		return AMQP_STATUS_BAD_AMQP_DATA;
	}
	if((tablesize + *offset) > encoded.len) {
		return AMQP_STATUS_BAD_AMQP_DATA;
	}
	entries = static_cast<amqp_table_entry_t *>(SAlloc::M(allocated_entries * sizeof(amqp_table_entry_t)));
	if(!entries) {
		return AMQP_STATUS_NO_MEMORY;
	}
	limit = *offset + tablesize;
	while(*offset < limit) {
		uint8 keylen;
		res = AMQP_STATUS_BAD_AMQP_DATA;
		if(!amqp_decode_8(encoded, offset, &keylen)) {
			goto out;
		}
		if(num_entries >= allocated_entries) {
			allocated_entries = allocated_entries * 2;
			void * newentries = SAlloc::R(entries, allocated_entries * sizeof(amqp_table_entry_t));
			res = AMQP_STATUS_NO_MEMORY;
			if(!newentries)
				goto out;
			entries = static_cast<amqp_table_entry_t *>(newentries);
		}
		res = AMQP_STATUS_BAD_AMQP_DATA;
		if(!amqp_decode_bytes(encoded, offset, &entries[num_entries].key, keylen)) {
			goto out;
		}
		res = amqp_decode_field_value(encoded, pool, &entries[num_entries].value, offset);
		if(res < 0) {
			goto out;
		}
		num_entries++;
	}
	output->num_entries = num_entries;
	output->entries = static_cast<amqp_table_entry_t *>(amqp_pool_alloc(pool, num_entries * sizeof(amqp_table_entry_t)));
	// NULL is legitimate if we requested a zero-length block. 
	if(!output->entries) {
		if(!num_entries)
			res = AMQP_STATUS_OK;
		else
			res = AMQP_STATUS_NO_MEMORY;
		goto out;
	}
	memcpy(output->entries, entries, num_entries * sizeof(amqp_table_entry_t));
	res = AMQP_STATUS_OK;
out:
	SAlloc::F(entries);
	return res;
}

static int amqp_decode_field_value(amqp_bytes_t encoded, amqp_pool_t * pool, amqp_field_value_t * entry, size_t * offset) 
{
	int res = AMQP_STATUS_BAD_AMQP_DATA;
	if(!amqp_decode_8(encoded, offset, &entry->kind)) {
		goto out;
	}
#define TRIVIAL_FIELD_DECODER(bits) if(!amqp_decode_ ## bits(encoded, offset, &entry->value.u ## bits)) goto out; break
#define SIMPLE_FIELD_DECODER(bits, dest, how)                 \
	{                                                           \
		uint ## bits val;                                       \
		if(!amqp_decode_ ## bits(encoded, offset, &val)) goto out; \
		entry->value.dest = how;                                  \
	}                                                           \
	break

	switch(entry->kind) {
		case AMQP_FIELD_KIND_BOOLEAN: SIMPLE_FIELD_DECODER(8, boolean, val ? 1 : 0);
		case AMQP_FIELD_KIND_I8: SIMPLE_FIELD_DECODER(8, i8, (int8)val);
		case AMQP_FIELD_KIND_U8: TRIVIAL_FIELD_DECODER(8);
		case AMQP_FIELD_KIND_I16: SIMPLE_FIELD_DECODER(16, i16, (int16)val);
		case AMQP_FIELD_KIND_U16: TRIVIAL_FIELD_DECODER(16);
		case AMQP_FIELD_KIND_I32: SIMPLE_FIELD_DECODER(32, i32, (int32)val);
		case AMQP_FIELD_KIND_U32: TRIVIAL_FIELD_DECODER(32);
		case AMQP_FIELD_KIND_I64: SIMPLE_FIELD_DECODER(64, i64, (int64)val);
		case AMQP_FIELD_KIND_U64: TRIVIAL_FIELD_DECODER(64);
		case AMQP_FIELD_KIND_F32:
		    TRIVIAL_FIELD_DECODER(32);
		/* and by punning, f32 magically gets the right value...! */

		case AMQP_FIELD_KIND_F64:
		    TRIVIAL_FIELD_DECODER(64);
		/* and by punning, f64 magically gets the right value...! */

		case AMQP_FIELD_KIND_DECIMAL:
		    if(!amqp_decode_8(encoded, offset, &entry->value.decimal.decimals) || !amqp_decode_32(encoded, offset, &entry->value.decimal.value)) {
			    goto out;
		    }
		    break;
		case AMQP_FIELD_KIND_UTF8:
		// AMQP_FIELD_KIND_UTF8 and AMQP_FIELD_KIND_BYTES have the same implementation, but different interpretations. 
		// @fallthrough
		case AMQP_FIELD_KIND_BYTES: 
			{
				uint32 len;
				if(!amqp_decode_32(encoded, offset, &len) || !amqp_decode_bytes(encoded, offset, &entry->value.bytes, len))
					goto out;
			}
			break;
		case AMQP_FIELD_KIND_ARRAY:
		    res = amqp_decode_array(encoded, pool, &(entry->value.array), offset);
		    goto out;
		case AMQP_FIELD_KIND_TIMESTAMP:
		    TRIVIAL_FIELD_DECODER(64);
		case AMQP_FIELD_KIND_TABLE:
		    res = amqp_decode_table(encoded, pool, &(entry->value.table), offset);
		    goto out;
		case AMQP_FIELD_KIND_VOID:
		    break;
		default:
		    goto out;
	}
	res = AMQP_STATUS_OK;
out:
	return res;
}

/*---------------------------------------------------------------------------*/

static int amqp_encode_array(amqp_bytes_t encoded, amqp_array_t * input, size_t * offset) 
{
	size_t start = *offset;
	int    res = AMQP_STATUS_OK;
	*offset += 4; /* size of the array gets filled in later on */
	for(int i = 0; i < input->num_entries; i++) {
		res = amqp_encode_field_value(encoded, &input->entries[i], offset);
		if(res < 0) {
			goto out;
		}
	}
	if(!amqp_encode_32(encoded, &start, (uint32)(*offset - start - 4))) {
		res = AMQP_STATUS_TABLE_TOO_BIG;
		goto out;
	}
	res = AMQP_STATUS_OK;
out:
	return res;
}

int amqp_encode_table(amqp_bytes_t encoded, const amqp_table_t * input, size_t * offset) 
{
	size_t start = *offset;
	int    res = AMQP_STATUS_OK;
	*offset += 4; /* size of the table gets filled in later on */
	for(int i = 0; i < input->num_entries; i++) {
		if(!amqp_encode_8(encoded, offset, (uint8)input->entries[i].key.len)) {
			res = AMQP_STATUS_TABLE_TOO_BIG;
			goto out;
		}
		if(!amqp_encode_bytes(encoded, offset, input->entries[i].key)) {
			res = AMQP_STATUS_TABLE_TOO_BIG;
			goto out;
		}
		res = amqp_encode_field_value(encoded, &input->entries[i].value, offset);
		if(res < 0) {
			goto out;
		}
	}
	if(!amqp_encode_32(encoded, &start, (uint32)(*offset - start - 4))) {
		res = AMQP_STATUS_TABLE_TOO_BIG;
		goto out;
	}
	res = AMQP_STATUS_OK;
out:
	return res;
}

static int amqp_encode_field_value(amqp_bytes_t encoded, amqp_field_value_t * entry, size_t * offset) 
{
	int res = AMQP_STATUS_BAD_AMQP_DATA;
	if(!amqp_encode_8(encoded, offset, entry->kind)) {
		goto out;
	}
#define FIELD_ENCODER(bits, val)                   \
	if(!amqp_encode_ ## bits(encoded, offset, val)) { \
		res = AMQP_STATUS_TABLE_TOO_BIG;               \
		goto out;                                      \
	}                                                \
	break
	switch(entry->kind) {
		case AMQP_FIELD_KIND_BOOLEAN: FIELD_ENCODER(8, entry->value.boolean ? 1 : 0);
		case AMQP_FIELD_KIND_I8: FIELD_ENCODER(8, entry->value.i8);
		case AMQP_FIELD_KIND_U8: FIELD_ENCODER(8, entry->value.u8);
		case AMQP_FIELD_KIND_I16: FIELD_ENCODER(16, entry->value.i16);
		case AMQP_FIELD_KIND_U16: FIELD_ENCODER(16, entry->value.u16);
		case AMQP_FIELD_KIND_I32: FIELD_ENCODER(32, entry->value.i32);
		case AMQP_FIELD_KIND_U32: FIELD_ENCODER(32, entry->value.u32);
		case AMQP_FIELD_KIND_I64: FIELD_ENCODER(64, entry->value.i64);
		case AMQP_FIELD_KIND_U64: FIELD_ENCODER(64, entry->value.u64);
		case AMQP_FIELD_KIND_F32:
		    /* by punning, u32 magically gets the right value...! */
		    FIELD_ENCODER(32, entry->value.u32);
		case AMQP_FIELD_KIND_F64:
		    /* by punning, u64 magically gets the right value...! */
		    FIELD_ENCODER(64, entry->value.u64);
		case AMQP_FIELD_KIND_DECIMAL:
		    if(!amqp_encode_8(encoded, offset, entry->value.decimal.decimals) || !amqp_encode_32(encoded, offset, entry->value.decimal.value)) {
			    res = AMQP_STATUS_TABLE_TOO_BIG;
			    goto out;
		    }
		    break;

		case AMQP_FIELD_KIND_UTF8:
		// AMQP_FIELD_KIND_UTF8 and AMQP_FIELD_KIND_BYTES have the same implementation, but different interpretations. 
		// @fallthrough
		case AMQP_FIELD_KIND_BYTES:
		    if(!amqp_encode_32(encoded, offset, (uint32)entry->value.bytes.len) || !amqp_encode_bytes(encoded, offset, entry->value.bytes)) {
			    res = AMQP_STATUS_TABLE_TOO_BIG;
			    goto out;
		    }
		    break;
		case AMQP_FIELD_KIND_ARRAY:
		    res = amqp_encode_array(encoded, &entry->value.array, offset);
		    goto out;
		case AMQP_FIELD_KIND_TIMESTAMP:
		    FIELD_ENCODER(64, entry->value.u64);
		case AMQP_FIELD_KIND_TABLE:
		    res = amqp_encode_table(encoded, &entry->value.table, offset);
		    goto out;
		case AMQP_FIELD_KIND_VOID:
		    break;
		default:
		    res = AMQP_STATUS_INVALID_PARAMETER;
		    goto out;
	}
	res = AMQP_STATUS_OK;
out:
	return res;
}

int amqp_table_entry_cmp(void const * entry1, void const * entry2) 
{
	const amqp_table_entry_t * p1 = static_cast<const amqp_table_entry_t *>(entry1);
	const amqp_table_entry_t * p2 = static_cast<const amqp_table_entry_t *>(entry2);
	int d;
	size_t minlen = p1->key.len;
	if(p2->key.len < minlen) {
		minlen = p2->key.len;
	}
	d = memcmp(p1->key.bytes, p2->key.bytes, minlen);
	return (d != 0) ? d : ((int)p1->key.len - (int)p2->key.len);
}

static int amqp_field_value_clone(const amqp_field_value_t * original, amqp_field_value_t * clone, amqp_pool_t * pool) 
{
	int i;
	int res;
	clone->kind = original->kind;
	switch(clone->kind) {
		case AMQP_FIELD_KIND_BOOLEAN: clone->value.boolean = original->value.boolean; break;
		case AMQP_FIELD_KIND_I8: clone->value.i8 = original->value.i8; break;
		case AMQP_FIELD_KIND_U8: clone->value.u8 = original->value.u8; break;
		case AMQP_FIELD_KIND_I16: clone->value.i16 = original->value.i16; break;
		case AMQP_FIELD_KIND_U16: clone->value.u16 = original->value.u16; break;
		case AMQP_FIELD_KIND_I32: clone->value.i32 = original->value.i32; break;
		case AMQP_FIELD_KIND_U32: clone->value.u32 = original->value.u32; break;
		case AMQP_FIELD_KIND_I64: clone->value.i64 = original->value.i64; break;
		case AMQP_FIELD_KIND_U64:
		case AMQP_FIELD_KIND_TIMESTAMP: clone->value.u64 = original->value.u64; break;
		case AMQP_FIELD_KIND_F32: clone->value.f32 = original->value.f32; break;
		case AMQP_FIELD_KIND_F64: clone->value.f64 = original->value.f64; break;
		case AMQP_FIELD_KIND_DECIMAL: clone->value.decimal = original->value.decimal; break;
		case AMQP_FIELD_KIND_UTF8:
		case AMQP_FIELD_KIND_BYTES:
		    if(!original->value.bytes.len)
			    clone->value.bytes = amqp_empty_bytes;
		    else {
			    amqp_pool_alloc_bytes(pool, original->value.bytes.len, &clone->value.bytes);
			    if(!clone->value.bytes.bytes)
				    return AMQP_STATUS_NO_MEMORY;
				else {
					memcpy(clone->value.bytes.bytes, original->value.bytes.bytes,
					clone->value.bytes.len);
				}
		    }
		    break;
		case AMQP_FIELD_KIND_ARRAY:
		    if(!original->value.array.entries)
			    clone->value.array = amqp_empty_array;
		    else {
			    clone->value.array.num_entries = original->value.array.num_entries;
			    clone->value.array.entries = static_cast<amqp_field_value_t *>(amqp_pool_alloc(pool, clone->value.array.num_entries * sizeof(amqp_field_value_t)));
			    if(!clone->value.array.entries)
				    return AMQP_STATUS_NO_MEMORY;
				else {
					for(i = 0; i < clone->value.array.num_entries; ++i) {
						res = amqp_field_value_clone(&original->value.array.entries[i], &clone->value.array.entries[i], pool);
						if(res != AMQP_STATUS_OK)
							return res;
					}
				}
		    }
		    break;
		case AMQP_FIELD_KIND_TABLE: return amqp_table_clone(&original->value.table, &clone->value.table, pool);
		case AMQP_FIELD_KIND_VOID: break;
		default: return AMQP_STATUS_INVALID_PARAMETER;
	}
	return AMQP_STATUS_OK;
}

static int amqp_table_entry_clone(const amqp_table_entry_t * original, amqp_table_entry_t * clone, amqp_pool_t * pool) 
{
	if(!original->key.len)
		return AMQP_STATUS_INVALID_PARAMETER;
	else {
		amqp_pool_alloc_bytes(pool, original->key.len, &clone->key);
		if(!clone->key.bytes)
			return AMQP_STATUS_NO_MEMORY;
		else {
			memcpy(clone->key.bytes, original->key.bytes, clone->key.len);
			return amqp_field_value_clone(&original->value, &clone->value, pool);
		}
	}
}

int amqp_table_clone(const amqp_table_t * original, amqp_table_t * clone, amqp_pool_t * pool) 
{
	int res;
	clone->num_entries = original->num_entries;
	if(!clone->num_entries) {
		*clone = amqp_empty_table;
		return AMQP_STATUS_OK;
	}
	else {
		clone->entries = static_cast<amqp_table_entry_t *>(amqp_pool_alloc(pool, clone->num_entries * sizeof(amqp_table_entry_t)));
		if(!clone->entries)
			return AMQP_STATUS_NO_MEMORY;
		else {
			for(int i = 0; i < clone->num_entries; ++i) {
				res = amqp_table_entry_clone(&original->entries[i], &clone->entries[i], pool);
				if(res != AMQP_STATUS_OK)
					goto error_out1;
			}
			return AMQP_STATUS_OK;
		}
	}
error_out1:
	return res;
}

amqp_table_entry_t amqp_table_construct_utf8_entry(const char * key, const char * value) 
{
	amqp_table_entry_t ret;
	ret.key = amqp_cstring_bytes(key);
	ret.value.kind = AMQP_FIELD_KIND_UTF8;
	ret.value.value.bytes = amqp_cstring_bytes(value);
	return ret;
}

amqp_table_entry_t amqp_table_construct_table_entry(const char * key, const amqp_table_t * value) 
{
	amqp_table_entry_t ret;
	ret.key = amqp_cstring_bytes(key);
	ret.value.kind = AMQP_FIELD_KIND_TABLE;
	ret.value.value.table = *value;
	return ret;
}

amqp_table_entry_t amqp_table_construct_bool_entry(const char * key, const int value) 
{
	amqp_table_entry_t ret;
	ret.key = amqp_cstring_bytes(key);
	ret.value.kind = AMQP_FIELD_KIND_BOOLEAN;
	ret.value.value.boolean = value;
	return ret;
}

amqp_table_entry_t * amqp_table_get_entry_by_key(const amqp_table_t * table, const amqp_bytes_t key) 
{
	assert(table != NULL);
	for(int i = 0; i < table->num_entries; ++i) {
		if(amqp_bytes_equal(table->entries[i].key, key))
			return &table->entries[i];
	}
	return NULL;
}
// } AMQP_TABLE
//
// AMQP_HOSTCHECK {
//
// Portable, consistent toupper (remember EBCDIC). Do not use toupper()
// because its behavior is altered by the current locale.
//
static char amqp_raw_toupper(char in) 
{
	switch(in) {
		case 'a': return 'A';
		case 'b': return 'B';
		case 'c': return 'C';
		case 'd': return 'D';
		case 'e': return 'E';
		case 'f': return 'F';
		case 'g': return 'G';
		case 'h': return 'H';
		case 'i': return 'I';
		case 'j': return 'J';
		case 'k': return 'K';
		case 'l': return 'L';
		case 'm': return 'M';
		case 'n': return 'N';
		case 'o': return 'O';
		case 'p': return 'P';
		case 'q': return 'Q';
		case 'r': return 'R';
		case 's': return 'S';
		case 't': return 'T';
		case 'u': return 'U';
		case 'v': return 'V';
		case 'w': return 'W';
		case 'x': return 'X';
		case 'y': return 'Y';
		case 'z': return 'Z';
	}
	return in;
}
/*
 * amqp_raw_equal() is for doing "raw" case insensitive strings. This is meant
 * to be locale independent and only compare strings we know are safe for
 * this. See http://daniel.haxx.se/blog/2008/10/15/strcasecmp-in-turkish/ for
 * some further explanation to why this function is necessary.
 *
 * The function is capable of comparing a-z case insensitively even for non-ascii.
 */
static int amqp_raw_equal(const char * first, const char * second) 
{
	while(*first && *second) {
		if(amqp_raw_toupper(*first) != amqp_raw_toupper(*second)) {
			break; // get out of the loop as soon as they don't match 
		}
		first++;
		second++;
	}
	/* we do the comparison here (possibly again), just to make sure that if
	 * the loop above is skipped because one of the strings reached zero, we
	 * must not return this as a successful match
	 */
	return (amqp_raw_toupper(*first) == amqp_raw_toupper(*second));
}

static int amqp_raw_nequal(const char * first, const char * second, size_t max) 
{
	while(*first && *second && max) {
		if(amqp_raw_toupper(*first) != amqp_raw_toupper(*second)) {
			break;
		}
		max--;
		first++;
		second++;
	}
	if(0 == max) {
		return 1; /* they are equal this far */
	}
	return amqp_raw_toupper(*first) == amqp_raw_toupper(*second);
}
/*
 * Match a hostname against a wildcard pattern.
 * E.g. "foo.host.com" matches "*.host.com".
 *
 * We use the matching rule described in RFC6125, section 6.4.3. http://tools.ietf.org/html/rfc6125#section-6.4.3
 */
static amqp_hostcheck_result amqp_hostmatch(const char * hostname, const char * pattern) 
{
	const char * pattern_label_end;
	const char * hostname_label_end;
	int wildcard_enabled;
	size_t prefixlen, suffixlen;
	const char * pattern_wildcard = strchr(pattern, '*');
	if(!pattern_wildcard) {
		return amqp_raw_equal(pattern, hostname) ? AMQP_HCR_MATCH : AMQP_HCR_NO_MATCH;
	}
	// We require at least 2 dots in pattern to avoid too wide wildcard match. 
	wildcard_enabled = 1;
	pattern_label_end = strchr(pattern, '.');
	if(!pattern_label_end || strchr(pattern_label_end + 1, '.') == NULL || pattern_wildcard > pattern_label_end || amqp_raw_nequal(pattern, "xn--", 4))
		wildcard_enabled = 0;
	if(!wildcard_enabled) {
		return amqp_raw_equal(pattern, hostname) ? AMQP_HCR_MATCH : AMQP_HCR_NO_MATCH;
	}
	hostname_label_end = strchr(hostname, '.');
	if(!hostname_label_end || !amqp_raw_equal(pattern_label_end, hostname_label_end)) {
		return AMQP_HCR_NO_MATCH;
	}
	// The wildcard must match at least one character, so the left-most
	// label of the hostname is at least as large as the left-most label of the pattern.
	if(hostname_label_end - hostname < pattern_label_end - pattern) {
		return AMQP_HCR_NO_MATCH;
	}
	prefixlen = pattern_wildcard - pattern;
	suffixlen = pattern_label_end - (pattern_wildcard + 1);
	return amqp_raw_nequal(pattern, hostname, prefixlen) && amqp_raw_nequal(pattern_wildcard + 1, hostname_label_end - suffixlen, suffixlen) ? AMQP_HCR_MATCH : AMQP_HCR_NO_MATCH;
}

amqp_hostcheck_result amqp_hostcheck(const char * match_pattern, const char * hostname) 
{
	if(!match_pattern || !*match_pattern || !hostname || !*hostname) // sanity check 
		return AMQP_HCR_NO_MATCH;
	else if(amqp_raw_equal(hostname, match_pattern)) // trivial case 
		return AMQP_HCR_MATCH;
	else
		return amqp_hostmatch(hostname, match_pattern);
}
// } AMQP_HOSTCHECK
//
// AMQP_URL {
//
void amqp_default_connection_info(struct amqp_connection_info * ci) 
{
	/* Apply defaults */
	ci->user = "guest";
	ci->password = "guest";
	ci->host = "localhost";
	ci->port = 5672;
	ci->vhost = "/";
	ci->ssl = 0;
}

/* Scan for the next delimiter, handling percent-encodings on the way. */
static char find_delim(char ** pp, int colon_and_at_sign_are_delims) 
{
	char * from = *pp;
	char * to = from;
	for(;;) {
		char ch = *from++;
		switch(ch) {
			case ':':
			case '@':
			    if(!colon_and_at_sign_are_delims) {
				    *to++ = ch;
				    break;
			    }

			// @fallthrough
			case 0:
			case '/':
			case '?':
			case '#':
			case '[':
			case ']':
			    *to = 0;
			    *pp = from;
			    return ch;

			case '%': {
			    unsigned int val;
			    int chars;
			    int res = sscanf(from, "%2x%n", &val, &chars);
			    if(res == EOF || res < 1 || chars != 2 || val > CHAR_MAX)
				    return '%'; // Return a surprising delimiter to force an error. 
			    *to++ = (char)val;
			    from += 2;
			    break;
		    }
			default:
			    *to++ = ch;
			    break;
		}
	}
}
//
// Parse an AMQP URL into its component parts. 
//
int amqp_parse_url(char * url, struct amqp_connection_info * parsed) 
{
	int res = AMQP_STATUS_BAD_URL;
	char delim;
	char * start;
	char * host;
	char * port = NULL;
	amqp_default_connection_info(parsed);
	/* check the prefix */
	if(!strncmp(url, "amqp://", 7)) {
		/* do nothing */
	}
	else if(!strncmp(url, "amqps://", 8)) {
		parsed->port = 5671;
		parsed->ssl = 1;
	}
	else {
		goto out;
	}
	host = start = url += (parsed->ssl ? 8 : 7);
	delim = find_delim(&url, 1);
	if(delim == ':') {
		/* The colon could be introducing the port or the
		   password part of the userinfo.  We don't know yet,
		   so stash the preceding component. */
		port = start = url;
		delim = find_delim(&url, 1);
	}
	if(delim == '@') {
		/* What might have been the host and port were in fact
		   the username and password */
		parsed->user = host;
		if(port) {
			parsed->password = port;
		}

		port = NULL;
		host = start = url;
		delim = find_delim(&url, 1);
	}
	if(delim == '[') {
		// IPv6 address.  The bracket should be the first character in the host. 
		if(host != start || *host != 0) {
			goto out;
		}
		start = url;
		delim = find_delim(&url, 0);
		if(delim != ']') {
			goto out;
		}
		parsed->host = start;
		start = url;
		delim = find_delim(&url, 1);
		// Closing bracket should be the last character in the host. 
		if(*start != 0) {
			goto out;
		}
	}
	else {
		/* If we haven't seen the host yet, this is it. */
		if(*host != 0) {
			parsed->host = host;
		}
	}
	if(delim == ':') {
		port = start = url;
		delim = find_delim(&url, 1);
	}
	if(port) {
		char * end;
		long portnum = strtol(port, &end, 10);
		if(port == end || *end != 0 || portnum < 0 || portnum > 65535) {
			goto out;
		}
		parsed->port = portnum;
	}
	if(delim == '/') {
		start = url;
		delim = find_delim(&url, 1);
		if(delim != 0) {
			goto out;
		}
		parsed->vhost = start;
		res = AMQP_STATUS_OK;
	}
	else if(delim == 0) {
		res = AMQP_STATUS_OK;
	}
/* Any other delimiter is bad, and we will return AMQP_STATUS_BAD_AMQP_URL. */
out:
	return res;
}
// } AMQP_URL
//
// AMQP_SOCKET {
//
static int FASTCALL amqp_id_in_reply_list(amqp_method_number_t expected, const amqp_method_number_t * list) 
{
	while(*list != 0) {
		if(*list == expected) {
			return 1;
		}
		list++;
	}
	return 0;
}

static int amqp_os_socket_init() 
{
#ifdef _WIN32
	static int called_wsastartup = 0;
	if(!called_wsastartup) {
		WSADATA data;
		int res = WSAStartup(0x0202, &data);
		if(res) {
			return AMQP_STATUS_TCP_SOCKETLIB_INIT_ERROR;
		}
		called_wsastartup = 1;
	}
	return AMQP_STATUS_OK;
#else
	return AMQP_STATUS_OK;
#endif
}

int amqp_os_socket_error() 
{
#ifdef _WIN32
	return WSAGetLastError();
#else
	return errno;
#endif
}

int amqp_os_socket_close(int sockfd) 
{
#ifdef _WIN32
	return closesocket(sockfd);
#else
	return close(sockfd);
#endif
}

ssize_t amqp_socket_send(amqp_socket_t * self, const void * buf, size_t len, int flags) 
{
	assert(self);
	assert(self->klass->send);
	return self->klass->send(self, buf, len, flags);
}

ssize_t amqp_socket_recv(amqp_socket_t * self, void * buf, size_t len, int flags) 
{
	assert(self);
	assert(self->klass->recv);
	return self->klass->recv(self, buf, len, flags);
}

int amqp_socket_open(amqp_socket_t * self, const char * host, int port) 
{
	assert(self);
	assert(self->klass->open);
	return self->klass->open(self, host, port, NULL);
}

int amqp_socket_open_noblock(amqp_socket_t * self, const char * host, int port, struct timeval * timeout) 
{
	assert(self);
	assert(self->klass->open);
	return self->klass->open(self, host, port, timeout);
}

int amqp_socket_close(amqp_socket_t * self, amqp_socket_close_enum force) 
{
	assert(self);
	assert(self->klass->close);
	return self->klass->close(self, force);
}

void amqp_socket_delete(amqp_socket_t * self) {
	if(self) {
		assert(self->klass->FnDelete);
		self->klass->FnDelete(self);
	}
}

int amqp_socket_get_sockfd(amqp_socket_t * self) 
{
	assert(self);
	assert(self->klass->get_sockfd);
	return self->klass->get_sockfd(self);
}

int FASTCALL amqp_poll(int fd, int event, amqp_time_t deadline) 
{
#ifdef HAVE_POLL
	struct pollfd pfd;
	int res;
	int timeout_ms;
	/* Function should only ever be called with one of these two */
	assert(event == AMQP_SF_POLLIN || event == AMQP_SF_POLLOUT);
start_poll:
	pfd.fd = fd;
	switch(event) {
		case AMQP_SF_POLLIN: pfd.events = POLLIN; break;
		case AMQP_SF_POLLOUT: pfd.events = POLLOUT; break;
	}
	timeout_ms = amqp_time_ms_until(deadline);
	if(-1 > timeout_ms) {
		return timeout_ms;
	}
	res = poll(&pfd, 1, timeout_ms);
	if(0 < res) {
		/* TODO: optimize this a bit by returning the AMQP_STATUS_SOCKET_ERROR or
		 * equivalent when pdf.revent is POLLHUP or POLLERR, so an extra syscall
		 * doesn't need to be made. */
		return AMQP_STATUS_OK;
	}
	else if(0 == res) {
		return AMQP_STATUS_TIMEOUT;
	}
	else {
		switch(amqp_os_socket_error()) {
			case EINTR: goto start_poll;
			default: return AMQP_STATUS_SOCKET_ERROR;
		}
	}
	return AMQP_STATUS_OK;
#elif defined(HAVE_SELECT)
	fd_set fds;
	fd_set exceptfds;
	fd_set * exceptfdsp;
	int res;
	struct timeval tv;
	struct timeval * tvp;
	assert((0 != (event & AMQP_SF_POLLIN)) || (0 != (event & AMQP_SF_POLLOUT)));
#ifndef _WIN32
	/* On Win32 connect() failure is indicated through the exceptfds, it does not
	 * make any sense to allow POLLERR on any other platform or condition */
	assert(0 == (event & AMQP_SF_POLLERR));
#endif

start_select:
	FD_ZERO(&fds);
	FD_SET(fd, &fds);
	if(event & AMQP_SF_POLLERR) {
		FD_ZERO(&exceptfds);
		FD_SET(fd, &exceptfds);
		exceptfdsp = &exceptfds;
	}
	else
		exceptfdsp = NULL;
	res = amqp_time_tv_until(deadline, &tv, &tvp);
	if(res != AMQP_STATUS_OK) {
		return res;
	}
	if(event & AMQP_SF_POLLIN) {
		res = select(fd + 1, &fds, NULL, exceptfdsp, tvp);
	}
	else if(event & AMQP_SF_POLLOUT) {
		res = select(fd + 1, NULL, &fds, exceptfdsp, tvp);
	}
	if(0 < res) {
		return AMQP_STATUS_OK;
	}
	else if(0 == res) {
		return AMQP_STATUS_TIMEOUT;
	}
	else {
		switch(amqp_os_socket_error()) {
			case EINTR: goto start_select;
			default: return AMQP_STATUS_SOCKET_ERROR;
		}
	}
#else
	#error "poll() or select() is needed to compile rabbitmq-c"
#endif
}

static ssize_t do_poll(amqp_connection_state_t state, ssize_t res, amqp_time_t deadline) 
{
	int fd = amqp_get_sockfd(state);
	if(-1 == fd) {
		return AMQP_STATUS_SOCKET_CLOSED;
	}
	switch(res) {
		case AMQP_PRIVATE_STATUS_SOCKET_NEEDREAD:  res = amqp_poll(fd, AMQP_SF_POLLIN, deadline); break;
		case AMQP_PRIVATE_STATUS_SOCKET_NEEDWRITE: res = amqp_poll(fd, AMQP_SF_POLLOUT, deadline); break;
	}
	return res;
}

ssize_t amqp_try_send(amqp_connection_state_t state, const void * buf, size_t len, amqp_time_t deadline, int flags) 
{
	ssize_t res;
	const void * buf_left = buf;
	// Assume that len is not going to be larger than ssize_t can hold
	ssize_t len_left = (size_t)len;
start_send:
	res = amqp_socket_send(state->socket, buf_left, len_left, flags);
	if(res > 0) {
		len_left -= res;
		buf_left = PTR8C(buf_left) + res;
		if(!len_left)
			return (ssize_t)len;
		goto start_send;
	}
	res = do_poll(state, res, deadline);
	if(AMQP_STATUS_OK == res) {
		goto start_send;
	}
	return (res == AMQP_STATUS_TIMEOUT) ? ((ssize_t)len - len_left) : res;
}

int amqp_open_socket(char const * hostname, int portnumber) { return amqp_open_socket_inner(hostname, portnumber, amqp_time_infinite()); }

int amqp_open_socket_noblock(char const * hostname, int portnumber, struct timeval * timeout) 
{
	amqp_time_t deadline;
	int res = amqp_time_from_now(&deadline, timeout);
	return (res != AMQP_STATUS_OK) ? res : amqp_open_socket_inner(hostname, portnumber, deadline);
}

#ifdef _WIN32
static int connect_socket(struct addrinfo * addr, amqp_time_t deadline) 
{
	int one = 1;
	int last_error;
	/*
	 * This cast is to squash warnings on Win64, see:
	 * http://stackoverflow.com/questions/1953639/is-it-safe-to-cast-socket-to-int-under-win64
	 */
	SOCKET sockfd = (int)socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
	if(INVALID_SOCKET == sockfd) {
		return AMQP_STATUS_SOCKET_ERROR;
	}
	/* Set the socket to be non-blocking */
	if(SOCKET_ERROR == ioctlsocket(sockfd, FIONBIO, reinterpret_cast<u_long *>(&one))) {
		last_error = AMQP_STATUS_SOCKET_ERROR;
		goto err;
	}
	/* Disable nagle */
	if(SOCKET_ERROR == setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (const char *)&one, sizeof(one))) {
		last_error = AMQP_STATUS_SOCKET_ERROR;
		goto err;
	}
	/* Enable TCP keepalives */
	if(SOCKET_ERROR == setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, (const char *)&one, sizeof(one))) {
		last_error = AMQP_STATUS_SOCKET_ERROR;
		goto err;
	}
	if(SOCKET_ERROR != connect(sockfd, addr->ai_addr, (int)addr->ai_addrlen)) {
		return (int)sockfd;
	}
	if(WSAEWOULDBLOCK != WSAGetLastError()) {
		last_error = AMQP_STATUS_SOCKET_ERROR;
		goto err;
	}
	last_error = amqp_poll((int)sockfd, AMQP_SF_POLLOUT | AMQP_SF_POLLERR, deadline);
	if(AMQP_STATUS_OK != last_error) {
		goto err;
	}
	{
		int result;
		int result_len = sizeof(result);
		if(SOCKET_ERROR == getsockopt(sockfd, SOL_SOCKET, SO_ERROR, (char *)&result, &result_len) ||
		    result != 0) {
			last_error = AMQP_STATUS_SOCKET_ERROR;
			goto err;
		}
	}
	return (int)sockfd;
err:
	closesocket(sockfd);
	return last_error;
}

#else
static int connect_socket(struct addrinfo * addr, amqp_time_t deadline) 
{
	int one = 1;
	int sockfd;
	int flags;
	int last_error;
	sockfd = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
	if(-1 == sockfd) {
		return AMQP_STATUS_SOCKET_ERROR;
	}
	/* Enable CLOEXEC on socket */
	flags = fcntl(sockfd, F_GETFD);
	if(flags == -1 || fcntl(sockfd, F_SETFD, (long)(flags | FD_CLOEXEC)) == -1) {
		last_error = AMQP_STATUS_SOCKET_ERROR;
		goto err;
	}
	/* Set the socket as non-blocking */
	flags = fcntl(sockfd, F_GETFL);
	if(flags == -1 || fcntl(sockfd, F_SETFL, (long)(flags | O_NONBLOCK)) == -1) {
		last_error = AMQP_STATUS_SOCKET_ERROR;
		goto err;
	}
#ifdef SO_NOSIGPIPE
	/* Turn off SIGPIPE on platforms that support it, BSD, MacOSX */
	if(0 != setsockopt(sockfd, SOL_SOCKET, SO_NOSIGPIPE, &one, sizeof(one))) {
		last_error = AMQP_STATUS_SOCKET_ERROR;
		goto err;
	}
#endif /* SO_NOSIGPIPE */
	/* Disable nagle */
	if(0 != setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one))) {
		last_error = AMQP_STATUS_SOCKET_ERROR;
		goto err;
	}
	/* Enable TCP keepalives */
	if(0 != setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, &one, sizeof(one))) {
		last_error = AMQP_STATUS_SOCKET_ERROR;
		goto err;
	}
	if(0 == connect(sockfd, addr->ai_addr, addr->ai_addrlen)) {
		return sockfd;
	}
	if(EINPROGRESS != errno) {
		last_error = AMQP_STATUS_SOCKET_ERROR;
		goto err;
	}
	last_error = amqp_poll(sockfd, AMQP_SF_POLLOUT, deadline);
	if(AMQP_STATUS_OK != last_error) {
		goto err;
	}
	{
		int result;
		socklen_t result_len = sizeof(result);
		if(-1 == getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &result, &result_len) ||
		    result != 0) {
			last_error = AMQP_STATUS_SOCKET_ERROR;
			goto err;
		}
	}
	return sockfd;
err:
	close(sockfd);
	return last_error;
}
#endif

int amqp_open_socket_inner(char const * hostname, int portnumber, amqp_time_t deadline) 
{
	int last_error = amqp_os_socket_init();
	if(last_error != AMQP_STATUS_OK)
		return last_error;
	else {
		struct addrinfo hint;
		struct addrinfo * address_list;
		char portnumber_string[33];
		memzero(&hint, sizeof(hint));
		hint.ai_family = PF_UNSPEC; /* PF_INET or PF_INET6 */
		hint.ai_socktype = SOCK_STREAM;
		hint.ai_protocol = IPPROTO_TCP;
		sprintf(portnumber_string, "%d", portnumber);
		last_error = getaddrinfo(hostname, portnumber_string, &hint, &address_list);
		if(last_error)
			return AMQP_STATUS_HOSTNAME_RESOLUTION_FAILED;
		else {
			int sockfd = -1;
			for(struct addrinfo * addr = address_list; addr; addr = addr->ai_next) {
				sockfd = connect_socket(addr, deadline);
				if(sockfd >= 0) {
					last_error = AMQP_STATUS_OK;
					break;
				}
				else if(sockfd == AMQP_STATUS_TIMEOUT) {
					last_error = sockfd;
					break;
				}
			}
			freeaddrinfo(address_list);
			return (last_error != AMQP_STATUS_OK || sockfd == -1) ? last_error : sockfd;
		}
	}
}

static int send_header_inner(amqp_connection_state_t state, amqp_time_t deadline) 
{
	static const uint8 header[8] = {'A', 'M', 'Q', 'P', 0, AMQP_PROTOCOL_VERSION_MAJOR,
		AMQP_PROTOCOL_VERSION_MINOR, AMQP_PROTOCOL_VERSION_REVISION};
	ssize_t res = amqp_try_send(state, header, sizeof(header), deadline, AMQP_SF_NONE);
	return (sizeof(header) == res) ? AMQP_STATUS_OK : (int)res;
}

int amqp_send_header(amqp_connection_state_t state) 
{
	return send_header_inner(state, amqp_time_infinite());
}

static amqp_bytes_t sasl_method_name(amqp_sasl_method_enum method) 
{
	amqp_bytes_t res;
	switch(method) {
		case AMQP_SASL_METHOD_PLAIN: res = amqp_cstring_bytes("PLAIN"); break;
		case AMQP_SASL_METHOD_EXTERNAL: res = amqp_cstring_bytes("EXTERNAL"); break;
		default: amqp_abort("Invalid SASL method: %d", (int)method);
	}
	return res;
}

static int bytes_equal(amqp_bytes_t l, amqp_bytes_t r) 
{
	return BIN(l.len == r.len && l.bytes && r.bytes && memcmp(l.bytes, r.bytes, l.len) == 0);
}

int sasl_mechanism_in_list(amqp_bytes_t mechanisms, amqp_sasl_method_enum method) 
{
	amqp_bytes_t supported_mechanism;
	amqp_bytes_t mechanism = sasl_method_name(method);
	assert(mechanisms.bytes);
	uint8 * start = static_cast<uint8 *>(mechanisms.bytes);
	uint8 * current = start;
	uint8 * end = start + mechanisms.len;
	for(; current != end; start = current + 1) {
		// HACK: SASL states that we should be parsing this string as a UTF-8
		// string, which we're plainly not doing here. At this point its not worth
		// dragging an entire UTF-8 parser for this one case, and this should work most of the time 
		current = static_cast<uint8 *>(memchr(start, ' ', end - start));
		SETIFZ(current, end);
		supported_mechanism.bytes = start;
		supported_mechanism.len = current - start;
		if(bytes_equal(mechanism, supported_mechanism))
			return 1;
	}
	return 0;
}

static amqp_bytes_t sasl_response(amqp_pool_t * pool, amqp_sasl_method_enum method, va_list args) 
{
	amqp_bytes_t response;
	switch(method) {
		case AMQP_SASL_METHOD_PLAIN: 
		{
		    const char * username = va_arg(args, char *);
		    size_t username_len = sstrlen(username);
		    const char * password = va_arg(args, char *);
		    size_t password_len = sstrlen(password);
		    char * response_buf;
		    amqp_pool_alloc_bytes(pool, sstrlen(username) + sstrlen(password) + 2, &response);
		    if(!response.bytes)
			    return response; // We never request a zero-length block, because of the +2 above, so a NULL here really is ENOMEM. 
		    response_buf = static_cast<char *>(response.bytes);
		    response_buf[0] = 0;
		    memcpy(response_buf + 1, username, username_len);
		    response_buf[username_len + 1] = 0;
		    memcpy(response_buf + username_len + 2, password, password_len);
		    break;
	    }
		case AMQP_SASL_METHOD_EXTERNAL: 
			{
				const char * identity = va_arg(args, char *);
				size_t identity_len = sstrlen(identity);
				amqp_pool_alloc_bytes(pool, identity_len, &response);
				if(!response.bytes)
					return response;
				memcpy(response.bytes, identity, identity_len);
			}
			break;
		default:
		    amqp_abort("Invalid SASL method: %d", (int)method);
	}
	return response;
}

boolint amqp_frames_enqueued(const amqp_connection_state_t state) { return (state->first_queued_frame != NULL); }

/*
 * Check to see if we have data in our buffer. If this returns 1, we
 * will avoid an immediate blocking read in amqp_simple_wait_frame.
 */
boolint amqp_data_in_buffer(const amqp_connection_state_t state) { return (state->sock_inbound_offset < state->sock_inbound_limit); }

static int consume_one_frame(amqp_connection_state_t state, amqp_frame_t * decoded_frame) 
{
	int res;
	amqp_bytes_t buffer;
	buffer.len = state->sock_inbound_limit - state->sock_inbound_offset;
	buffer.bytes = PTR8(state->sock_inbound_buffer.bytes) + state->sock_inbound_offset;
	res = amqp_handle_input(state, buffer, decoded_frame);
	if(res < 0)
		return res;
	else {
		state->sock_inbound_offset += res;
		return AMQP_STATUS_OK;
	}
}

static int recv_with_timeout(amqp_connection_state_t state, amqp_time_t timeout) 
{
start_recv:
	ssize_t res = amqp_socket_recv(state->socket, state->sock_inbound_buffer.bytes, state->sock_inbound_buffer.len, 0);
	if(res < 0) {
		const int fd = amqp_get_sockfd(state);
		if(fd == -1)
			return AMQP_STATUS_CONNECTION_CLOSED;
		switch(res) {
			default: return (int)res;
			case AMQP_PRIVATE_STATUS_SOCKET_NEEDREAD: res = amqp_poll(fd, AMQP_SF_POLLIN, timeout); break;
			case AMQP_PRIVATE_STATUS_SOCKET_NEEDWRITE: res = amqp_poll(fd, AMQP_SF_POLLOUT, timeout); break;
		}
		if(res == AMQP_STATUS_OK)
			goto start_recv;
		return (int)res;
	}
	state->sock_inbound_limit = res;
	state->sock_inbound_offset = 0;
	res = amqp_time_s_from_now(&state->next_recv_heartbeat, amqp_heartbeat_recv(state));
	return res;
}

int amqp_try_recv(amqp_connection_state_t state) 
{
	amqp_time_t timeout;
	while(amqp_data_in_buffer(state)) {
		amqp_frame_t frame;
		int res = consume_one_frame(state, &frame);
		if(res != AMQP_STATUS_OK) {
			return res;
		}
		if(frame.frame_type != 0) {
			amqp_frame_t * frame_copy;
			amqp_link_t * link;
			amqp_pool_t * channel_pool = amqp_get_or_create_channel_pool(state, frame.channel);
			if(!channel_pool) {
				return AMQP_STATUS_NO_MEMORY;
			}
			frame_copy = static_cast<amqp_frame_t *>(amqp_pool_alloc(channel_pool, sizeof(amqp_frame_t)));
			link = static_cast<amqp_link_t *>(amqp_pool_alloc(channel_pool, sizeof(amqp_link_t)));
			if(!frame_copy || !link) {
				return AMQP_STATUS_NO_MEMORY;
			}
			*frame_copy = frame;
			link->next = NULL;
			link->data = frame_copy;
			if(state->last_queued_frame == NULL)
				state->first_queued_frame = link;
			else
				state->last_queued_frame->next = link;
			state->last_queued_frame = link;
		}
	}
	timeout = amqp_time_immediate();
	return recv_with_timeout(state, timeout);
}

static int wait_frame_inner(amqp_connection_state_t state, amqp_frame_t * decoded_frame, amqp_time_t timeout_deadline) 
{
	amqp_time_t deadline;
	int res;
	for(;;) {
		while(amqp_data_in_buffer(state)) {
			res = consume_one_frame(state, decoded_frame);
			if(res != AMQP_STATUS_OK)
				return res;
			else {
				if(decoded_frame->frame_type == AMQP_FRAME_HEARTBEAT) {
					amqp_maybe_release_buffers_on_channel(state, 0);
					continue;
				}
				if(decoded_frame->frame_type != 0)
					return AMQP_STATUS_OK; // Complete frame was read. Return it. 
			}
		}
beginrecv:
		res = amqp_time_has_past(state->next_send_heartbeat);
		if(res == AMQP_STATUS_TIMER_FAILURE)
			return res;
		else if(res == AMQP_STATUS_TIMEOUT) {
			amqp_frame_t heartbeat;
			heartbeat.channel = 0;
			heartbeat.frame_type = AMQP_FRAME_HEARTBEAT;
			res = amqp_send_frame(state, &heartbeat);
			if(res != AMQP_STATUS_OK)
				return res;
		}
		deadline = amqp_time_first(timeout_deadline, amqp_time_first(state->next_recv_heartbeat, state->next_send_heartbeat));
		// TODO this needs to wait for a _frame_ and not anything written from the socket 
		res = recv_with_timeout(state, deadline);
		if(res == AMQP_STATUS_TIMEOUT) {
			if(amqp_time_equal(deadline, state->next_recv_heartbeat)) {
				amqp_socket_close(state->socket, AMQP_SC_FORCE);
				return AMQP_STATUS_HEARTBEAT_TIMEOUT;
			}
			else if(amqp_time_equal(deadline, timeout_deadline))
				return AMQP_STATUS_TIMEOUT;
			else if(amqp_time_equal(deadline, state->next_send_heartbeat))
				goto beginrecv; // send heartbeat happens before we do recv_with_timeout 
			else
				amqp_abort("Internal error: unable to determine timeout reason");
		}
		else if(res != AMQP_STATUS_OK)
			return res;
	}
}

static amqp_link_t * FASTCALL amqp_create_link_for_frame(amqp_connection_state_t state, amqp_frame_t * frame) 
{
	amqp_link_t * link = 0;
	amqp_pool_t * channel_pool = amqp_get_or_create_channel_pool(state, frame->channel);
	if(channel_pool) {
		link = static_cast<amqp_link_t *>(amqp_pool_alloc(channel_pool, sizeof(amqp_link_t)));
		amqp_frame_t * frame_copy = static_cast<amqp_frame_t *>(amqp_pool_alloc(channel_pool, sizeof(amqp_frame_t)));
		if(link && frame_copy) {
			*frame_copy = *frame;
			link->data = frame_copy;
		}
		else
			link = 0;
	}
	return link;
}

int amqp_queue_frame(amqp_connection_state_t state, amqp_frame_t * frame) 
{
	amqp_link_t * link = amqp_create_link_for_frame(state, frame);
	if(!link) {
		return AMQP_STATUS_NO_MEMORY;
	}
	else {
		if(!state->first_queued_frame)
			state->first_queued_frame = link;
		else
			state->last_queued_frame->next = link;
		link->next = NULL;
		state->last_queued_frame = link;
		return AMQP_STATUS_OK;
	}
}

int amqp_put_back_frame(amqp_connection_state_t state, amqp_frame_t * frame) 
{
	amqp_link_t * link = amqp_create_link_for_frame(state, frame);
	if(!link)
		return AMQP_STATUS_NO_MEMORY;
	else {
		if(!state->first_queued_frame) {
			state->first_queued_frame = link;
			state->last_queued_frame = link;
			link->next = NULL;
		}
		else {
			link->next = state->first_queued_frame;
			state->first_queued_frame = link;
		}
		return AMQP_STATUS_OK;
	}
}

int amqp_simple_wait_frame_on_channel(amqp_connection_state_t state, amqp_channel_t channel, amqp_frame_t * decoded_frame) 
{
	for(const amqp_link_t * cur = state->first_queued_frame; cur; cur = cur->next) {
		const amqp_frame_t * frame_ptr = static_cast<const amqp_frame_t *>(cur->data);
		if(channel == frame_ptr->channel) {
			state->first_queued_frame = cur->next;
			if(!state->first_queued_frame)
				state->last_queued_frame = NULL;
			*decoded_frame = *frame_ptr;
			return AMQP_STATUS_OK;
		}
	}
	for(;;) {
		int res = wait_frame_inner(state, decoded_frame, amqp_time_infinite());
		if(res != AMQP_STATUS_OK)
			return res;
		else if(channel == decoded_frame->channel)
			return AMQP_STATUS_OK;
		else {
			res = amqp_queue_frame(state, decoded_frame);
			if(res != AMQP_STATUS_OK)
				return res;
		}
	}
}

int amqp_simple_wait_frame(amqp_connection_state_t state, amqp_frame_t * decoded_frame) 
{
	return amqp_simple_wait_frame_noblock(state, decoded_frame, NULL);
}

int amqp_simple_wait_frame_noblock(amqp_connection_state_t state, amqp_frame_t * decoded_frame, struct timeval * timeout) 
{
	amqp_time_t deadline;
	int res = amqp_time_from_now(&deadline, timeout);
	if(res != AMQP_STATUS_OK)
		return res;
	else if(state->first_queued_frame) {
		const amqp_frame_t * f = static_cast<const amqp_frame_t *>(state->first_queued_frame->data);
		state->first_queued_frame = state->first_queued_frame->next;
		if(!state->first_queued_frame)
			state->last_queued_frame = NULL;
		*decoded_frame = *f;
		return AMQP_STATUS_OK;
	}
	else
		return wait_frame_inner(state, decoded_frame, deadline);
}

static int amqp_simple_wait_method_list(amqp_connection_state_t state, amqp_channel_t expected_channel,
    const amqp_method_number_t * expected_methods, amqp_time_t deadline, amqp_method_t * output) 
{
	struct timeval tv;
	struct timeval * tvp;
	int res = amqp_time_tv_until(deadline, &tv, &tvp);
	if(res != AMQP_STATUS_OK)
		return res;
	else {
		amqp_frame_t frame;
		res = amqp_simple_wait_frame_noblock(state, &frame, tvp);
		if(res != AMQP_STATUS_OK)
			return res;
		else if(AMQP_FRAME_METHOD != frame.frame_type || expected_channel != frame.channel || !amqp_id_in_reply_list(frame.payload.method.id, expected_methods))
			return AMQP_STATUS_WRONG_METHOD;
		else {
			*output = frame.payload.method;
			return AMQP_STATUS_OK;
		}
	}
}

static int simple_wait_method_inner(amqp_connection_state_t state, amqp_channel_t expected_channel, 
	amqp_method_number_t expected_method, amqp_time_t deadline, amqp_method_t * output) 
{
	const amqp_method_number_t expected_methods[] = {expected_method, 0};
	return amqp_simple_wait_method_list(state, expected_channel, expected_methods, deadline, output);
}

int amqp_simple_wait_method(amqp_connection_state_t state, amqp_channel_t expected_channel, amqp_method_number_t expected_method, amqp_method_t * output) 
{
	return simple_wait_method_inner(state, expected_channel, expected_method, amqp_time_infinite(), output);
}

int amqp_send_method(amqp_connection_state_t state, amqp_channel_t channel, amqp_method_number_t id, void * decoded) 
{
	return amqp_send_method_inner(state, channel, id, decoded, AMQP_SF_NONE, amqp_time_infinite());
}

int amqp_send_method_inner(amqp_connection_state_t state, amqp_channel_t channel, amqp_method_number_t id, void * decoded, int flags, amqp_time_t deadline) 
{
	amqp_frame_t frame;
	frame.frame_type = AMQP_FRAME_METHOD;
	frame.channel = channel;
	frame.payload.method.id = id;
	frame.payload.method.decoded = decoded;
	return amqp_send_frame_inner(state, &frame, flags, deadline);
}

static amqp_rpc_reply_t simple_rpc_inner(amqp_connection_state_t state, amqp_channel_t channel,
    amqp_method_number_t request_id, amqp_method_number_t * expected_reply_ids, void * decoded_request_method, amqp_time_t deadline) 
{
	amqp_rpc_reply_t result;
	int status = amqp_send_method(state, channel, request_id, decoded_request_method);
	if(status < 0) {
		return amqp_rpc_reply_error(static_cast<amqp_status_enum>(status));
	}
	{
		amqp_frame_t frame;
retry:
		status = wait_frame_inner(state, &frame, deadline);
		if(status < 0) {
			if(status == AMQP_STATUS_TIMEOUT) {
				amqp_socket_close(state->socket, AMQP_SC_FORCE);
			}
			return amqp_rpc_reply_error(static_cast<amqp_status_enum>(status));
		}
		/*
		 * We store the frame for later processing unless it's something
		 * that directly affects us here, namely a method frame that is
		 * either
		 *  - on the channel we want, and of the expected type, or
		 *  - on the channel we want, and a channel.close frame, or
		 *  - on channel zero, and a connection.close frame.
		 */
		if(!((frame.frame_type == AMQP_FRAME_METHOD) && (((frame.channel == channel) && (amqp_id_in_reply_list(frame.payload.method.id, expected_reply_ids) ||
		    (frame.payload.method.id == AMQP_CHANNEL_CLOSE_METHOD))) || ((frame.channel == 0) && (frame.payload.method.id == AMQP_CONNECTION_CLOSE_METHOD))))) {
			amqp_frame_t * frame_copy;
			amqp_link_t * link;
			amqp_pool_t * channel_pool = amqp_get_or_create_channel_pool(state, frame.channel);
			if(!channel_pool) {
				return amqp_rpc_reply_error(AMQP_STATUS_NO_MEMORY);
			}
			frame_copy = static_cast<amqp_frame_t *>(amqp_pool_alloc(channel_pool, sizeof(amqp_frame_t)));
			link = static_cast<amqp_link_t *>(amqp_pool_alloc(channel_pool, sizeof(amqp_link_t)));
			if(!frame_copy || !link) {
				return amqp_rpc_reply_error(AMQP_STATUS_NO_MEMORY);
			}
			*frame_copy = frame;
			link->next = NULL;
			link->data = frame_copy;
			if(!state->last_queued_frame)
				state->first_queued_frame = link;
			else
				state->last_queued_frame->next = link;
			state->last_queued_frame = link;
			goto retry;
		}
		result.reply_type = (amqp_id_in_reply_list(frame.payload.method.id, expected_reply_ids)) ? AMQP_RESPONSE_NORMAL : AMQP_RESPONSE_SERVER_EXCEPTION;
		result.reply = frame.payload.method;
		return result;
	}
}

amqp_rpc_reply_t amqp_simple_rpc(amqp_connection_state_t state, amqp_channel_t channel, amqp_method_number_t request_id,
    amqp_method_number_t * expected_reply_ids, void * decoded_request_method) 
{
	amqp_time_t deadline;
	int res = amqp_time_from_now(&deadline, state->rpc_timeout);
	if(res != AMQP_STATUS_OK)
		return amqp_rpc_reply_error(static_cast<amqp_status_enum>(res));
	else
		return simple_rpc_inner(state, channel, request_id, expected_reply_ids, decoded_request_method, deadline);
}

void * amqp_simple_rpc_decoded(amqp_connection_state_t state, amqp_channel_t channel, amqp_method_number_t request_id,
    amqp_method_number_t reply_id, void * decoded_request_method) 
{
	amqp_time_t deadline;
	int res = amqp_time_from_now(&deadline, state->rpc_timeout);
	if(res != AMQP_STATUS_OK) {
		state->most_recent_api_result = amqp_rpc_reply_error(static_cast<amqp_status_enum>(res));
		return NULL;
	}
	else {
		amqp_method_number_t replies[2];
		replies[0] = reply_id;
		replies[1] = 0;
		state->most_recent_api_result = simple_rpc_inner(state, channel, request_id, replies, decoded_request_method, deadline);
		return (state->most_recent_api_result.reply_type == AMQP_RESPONSE_NORMAL) ? state->most_recent_api_result.reply.decoded : NULL;
	}
}

amqp_rpc_reply_t amqp_get_rpc_reply(const amqp_connection_state_t state) { return state->most_recent_api_result; }
// 
// Descr: Merge base and add tables. If the two tables contain an entry with the same
//   key, the entry from the add table takes precedence. For entries that are both
//   tables with the same key, the table is recursively merged.
// 
int amqp_merge_capabilities(const amqp_table_t * base, const amqp_table_t * add, amqp_table_t * result, amqp_pool_t * pool) 
{
	int i;
	int res;
	amqp_pool_t temp_pool;
	amqp_table_t temp_result;
	assert(base != NULL);
	assert(result != NULL);
	assert(pool != NULL);
	if(!add) {
		return amqp_table_clone(base, result, pool);
	}
	init_amqp_pool(&temp_pool, 4096);
	temp_result.num_entries = 0;
	temp_result.entries = static_cast<amqp_table_entry_t *>(amqp_pool_alloc(&temp_pool, sizeof(amqp_table_entry_t) * (base->num_entries + add->num_entries)));
	if(!temp_result.entries) {
		res = AMQP_STATUS_NO_MEMORY;
		goto error_out;
	}
	for(i = 0; i < base->num_entries; ++i) {
		temp_result.entries[temp_result.num_entries] = base->entries[i];
		temp_result.num_entries++;
	}
	for(i = 0; i < add->num_entries; ++i) {
		amqp_table_entry_t * p_entry = amqp_table_get_entry_by_key(&temp_result, add->entries[i].key);
		if(p_entry) {
			if(AMQP_FIELD_KIND_TABLE == add->entries[i].value.kind && AMQP_FIELD_KIND_TABLE == p_entry->value.kind) {
				amqp_table_entry_t * be = amqp_table_get_entry_by_key(base, add->entries[i].key);
				res = amqp_merge_capabilities(&be->value.value.table, &add->entries[i].value.value.table, &p_entry->value.value.table, &temp_pool);
				if(res != AMQP_STATUS_OK) {
					goto error_out;
				}
			}
			else
				p_entry->value = add->entries[i].value;
		}
		else {
			temp_result.entries[temp_result.num_entries] = add->entries[i];
			temp_result.num_entries++;
		}
	}
	res = amqp_table_clone(&temp_result, result, pool);
error_out:
	empty_amqp_pool(&temp_pool);
	return res;
}

static amqp_rpc_reply_t amqp_login_inner(amqp_connection_state_t state, char const * vhost, int channel_max,
    int frame_max, int heartbeat, const amqp_table_t * client_properties,
    struct timeval * timeout, amqp_sasl_method_enum sasl_method, va_list vl) 
{
	int res;
	amqp_method_t method;
	uint16 client_channel_max;
	uint32 client_frame_max;
	uint16 client_heartbeat;
	uint16 server_channel_max;
	uint32 server_frame_max;
	uint16 server_heartbeat;
	amqp_rpc_reply_t result;
	amqp_time_t deadline;
	if(channel_max < 0 || channel_max > UINT16_MAX) {
		return amqp_rpc_reply_error(AMQP_STATUS_INVALID_PARAMETER);
	}
	client_channel_max = (uint16)channel_max;
	if(frame_max < 0) {
		return amqp_rpc_reply_error(AMQP_STATUS_INVALID_PARAMETER);
	}
	client_frame_max = (uint32)frame_max;
	if(heartbeat < 0 || heartbeat > UINT16_MAX) {
		return amqp_rpc_reply_error(AMQP_STATUS_INVALID_PARAMETER);
	}
	client_heartbeat = (uint16)heartbeat;
	res = amqp_time_from_now(&deadline, timeout);
	if(res != AMQP_STATUS_OK) {
		goto error_res;
	}
	res = send_header_inner(state, deadline);
	if(res != AMQP_STATUS_OK) {
		goto error_res;
	}
	res = simple_wait_method_inner(state, 0, AMQP_CONNECTION_START_METHOD, deadline, &method);
	if(res != AMQP_STATUS_OK) {
		goto error_res;
	}
	{
		amqp_connection_start_t * s = static_cast<amqp_connection_start_t *>(method.decoded);
		if((s->version_major != AMQP_PROTOCOL_VERSION_MAJOR) || (s->version_minor != AMQP_PROTOCOL_VERSION_MINOR)) {
			res = AMQP_STATUS_INCOMPATIBLE_AMQP_VERSION;
			goto error_res;
		}
		res = amqp_table_clone(&s->server_properties, &state->server_properties, &state->properties_pool);
		if(res != AMQP_STATUS_OK) {
			goto error_res;
		}
		// TODO: check that our chosen SASL mechanism is in the list of
		// acceptable mechanisms. Or even let the application choose from the list! 
		if(!sasl_mechanism_in_list(s->mechanisms, sasl_method)) {
			res = AMQP_STATUS_BROKER_UNSUPPORTED_SASL_METHOD;
			goto error_res;
		}
	}
	{
		amqp_table_entry_t default_properties[6];
		amqp_table_t default_table;
		amqp_table_entry_t client_capabilities[2];
		amqp_table_t client_capabilities_table;
		amqp_connection_start_ok_t s;
		amqp_bytes_t response_bytes;
		amqp_pool_t * channel_pool = amqp_get_or_create_channel_pool(state, 0);
		if(!channel_pool) {
			res = AMQP_STATUS_NO_MEMORY;
			goto error_res;
		}
		response_bytes = sasl_response(channel_pool, sasl_method, vl);
		if(!response_bytes.bytes) {
			res = AMQP_STATUS_NO_MEMORY;
			goto error_res;
		}
		client_capabilities[0] = amqp_table_construct_bool_entry("authentication_failure_close", 1);
		client_capabilities[1] = amqp_table_construct_bool_entry("exchange_exchange_bindings", 1);
		client_capabilities_table.entries = client_capabilities;
		client_capabilities_table.num_entries = sizeof(client_capabilities) / sizeof(amqp_table_entry_t);
		default_properties[0] = amqp_table_construct_utf8_entry("product", "rabbitmq-c");
		default_properties[1] = amqp_table_construct_utf8_entry("version", AMQP_VERSION_STRING);
		default_properties[2] = amqp_table_construct_utf8_entry("platform", AMQ_PLATFORM);
		default_properties[3] = amqp_table_construct_utf8_entry("copyright", AMQ_COPYRIGHT);
		default_properties[4] = amqp_table_construct_utf8_entry("information", "See https://github.com/alanxz/rabbitmq-c");
		default_properties[5] = amqp_table_construct_table_entry("capabilities", &client_capabilities_table);
		default_table.entries = default_properties;
		default_table.num_entries = sizeof(default_properties) / sizeof(amqp_table_entry_t);
		res = amqp_merge_capabilities(&default_table, client_properties, &state->client_properties, channel_pool);
		if(res != AMQP_STATUS_OK)
			goto error_res;
		s.client_properties = state->client_properties;
		s.mechanism = sasl_method_name(sasl_method);
		s.response = response_bytes;
		s.locale = amqp_cstring_bytes("en_US");
		res = amqp_send_method_inner(state, 0, AMQP_CONNECTION_START_OK_METHOD, &s, AMQP_SF_NONE, deadline);
		if(res < 0)
			goto error_res;
	}
	amqp_release_buffers(state);
	{
		const amqp_method_number_t expected[] = { AMQP_CONNECTION_TUNE_METHOD, AMQP_CONNECTION_CLOSE_METHOD, 0 };
		res = amqp_simple_wait_method_list(state, 0, expected, deadline, &method);
		if(res != AMQP_STATUS_OK) {
			goto error_res;
		}
	}
	if(AMQP_CONNECTION_CLOSE_METHOD == method.id) {
		result.reply_type = AMQP_RESPONSE_SERVER_EXCEPTION;
		result.reply = method;
		result.library_error = 0;
		goto out;
	}
	{
		amqp_connection_tune_t * s = static_cast<amqp_connection_tune_t *>(method.decoded);
		server_channel_max = s->channel_max;
		server_frame_max = s->frame_max;
		server_heartbeat = s->heartbeat;
	}
	if(server_channel_max && (server_channel_max < client_channel_max || client_channel_max == 0))
		client_channel_max = server_channel_max;
	else if(server_channel_max == 0 && client_channel_max == 0)
		client_channel_max = UINT16_MAX;
	if(server_frame_max && server_frame_max < client_frame_max)
		client_frame_max = server_frame_max;
	if(server_heartbeat && server_heartbeat < client_heartbeat)
		client_heartbeat = server_heartbeat;
	res = amqp_tune_connection(state, client_channel_max, client_frame_max, client_heartbeat);
	if(res < 0) {
		goto error_res;
	}
	{
		amqp_connection_tune_ok_t s;
		s.frame_max = client_frame_max;
		s.channel_max = client_channel_max;
		s.heartbeat = client_heartbeat;
		res = amqp_send_method_inner(state, 0, AMQP_CONNECTION_TUNE_OK_METHOD, &s, AMQP_SF_NONE, deadline);
		if(res < 0)
			goto error_res;
	}
	amqp_release_buffers(state);
	{
		amqp_method_number_t replies[] = {AMQP_CONNECTION_OPEN_OK_METHOD, 0};
		amqp_connection_open_t s;
		s.virtual_host = amqp_cstring_bytes(vhost);
		s.capabilities = amqp_empty_bytes;
		s.insist = 1;
		result = simple_rpc_inner(state, 0, AMQP_CONNECTION_OPEN_METHOD, replies, &s, deadline);
		if(result.reply_type != AMQP_RESPONSE_NORMAL) {
			goto out;
		}
	}
	result.reply_type = AMQP_RESPONSE_NORMAL;
	result.reply.id = 0;
	result.reply.decoded = NULL;
	result.library_error = 0;
	amqp_maybe_release_buffers(state);
out:
	return result;
error_res:
	amqp_socket_close(state->socket, AMQP_SC_FORCE);
	result = amqp_rpc_reply_error(static_cast<amqp_status_enum>(res));
	goto out;
}

amqp_rpc_reply_t amqp_login(amqp_connection_state_t state, char const * vhost, int channel_max, int frame_max, int heartbeat, amqp_sasl_method_enum sasl_method, ...) 
{
	va_list vl;
	amqp_rpc_reply_t ret;
	va_start(vl, sasl_method);
	ret = amqp_login_inner(state, vhost, channel_max, frame_max, heartbeat, &amqp_empty_table, state->handshake_timeout, static_cast<amqp_sasl_method_enum>(sasl_method), vl);
	va_end(vl);
	return ret;
}

amqp_rpc_reply_t amqp_login_with_properties(amqp_connection_state_t state, char const * vhost, int channel_max,
    int frame_max, int heartbeat, const amqp_table_t * client_properties, int sasl_method, ...) 
{
	va_list vl;
	amqp_rpc_reply_t ret;
	va_start(vl, sasl_method);
	ret = amqp_login_inner(state, vhost, channel_max, frame_max, heartbeat, client_properties, state->handshake_timeout, static_cast<amqp_sasl_method_enum>(sasl_method), vl);
	va_end(vl);
	return ret;
}
// } AMQP_SOCKET
//
// AMQP_TCP_SOCKET {
//
struct amqp_tcp_socket_t {
	const  amqp_socket_class_t * klass;
	int    sockfd;
	int    internal_error;
	int    state;
};

static ssize_t amqp_tcp_socket_send(void * base, const void * buf, size_t len, int flags) 
{
	struct amqp_tcp_socket_t * self = static_cast<struct amqp_tcp_socket_t *>(base);
	ssize_t res;
	int flagz = 0;
	if(self->sockfd == -1) {
		return AMQP_STATUS_SOCKET_CLOSED;
	}
#ifdef MSG_NOSIGNAL
	flagz |= MSG_NOSIGNAL;
#endif
#if defined(MSG_MORE)
	if(flags & AMQP_SF_MORE) {
		flagz |= MSG_MORE;
	}
// Cygwin defines TCP_NOPUSH, but trying to use it will return not
// implemented. Disable it here. 
#elif defined(TCP_NOPUSH) && !defined(__CYGWIN__)
	if(flags & AMQP_SF_MORE && !(self->state & AMQP_SF_MORE)) {
		int one = 1;
		res = setsockopt(self->sockfd, IPPROTO_TCP, TCP_NOPUSH, &one, sizeof(one));
		if(0 != res) {
			self->internal_error = res;
			return AMQP_STATUS_SOCKET_ERROR;
		}
		self->state |= AMQP_SF_MORE;
	}
	else if(!(flags & AMQP_SF_MORE) && self->state & AMQP_SF_MORE) {
		int zero = 0;
		res = setsockopt(self->sockfd, IPPROTO_TCP, TCP_NOPUSH, &zero, sizeof(&zero));
		if(0 != res) {
			self->internal_error = res;
			res = AMQP_STATUS_SOCKET_ERROR;
		}
		else {
			self->state &= ~AMQP_SF_MORE;
		}
	}
#endif
start:
#ifdef _WIN32
	res = send(self->sockfd, static_cast<const char *>(buf), (int)len, flagz);
#else
	res = send(self->sockfd, buf, len, flagz);
#endif
	if(res < 0) {
		self->internal_error = amqp_os_socket_error();
		switch(self->internal_error) {
			case EINTR:
			    goto start;
#ifdef _WIN32
			case WSAEWOULDBLOCK:
#else
			case EWOULDBLOCK:
#endif
#if defined(EAGAIN) && EAGAIN != EWOULDBLOCK
			case EAGAIN:
#endif
			    res = AMQP_PRIVATE_STATUS_SOCKET_NEEDWRITE;
			    break;
			default:
			    res = AMQP_STATUS_SOCKET_ERROR;
		}
	}
	else
		self->internal_error = 0;
	return res;
}

static ssize_t amqp_tcp_socket_recv(void * base, void * buf, size_t len, int flags) 
{
	struct amqp_tcp_socket_t * self = static_cast<struct amqp_tcp_socket_t *>(base);
	ssize_t ret;
	if(self->sockfd == -1) {
		return AMQP_STATUS_SOCKET_CLOSED;
	}
start:
#ifdef _WIN32
	ret = recv(self->sockfd, static_cast<char *>(buf), (int)len, flags);
#else
	ret = recv(self->sockfd, buf, len, flags);
#endif
	if(0 > ret) {
		self->internal_error = amqp_os_socket_error();
		switch(self->internal_error) {
			case EINTR:
			    goto start;
#ifdef _WIN32
			case WSAEWOULDBLOCK:
#else
			case EWOULDBLOCK:
#endif
#if defined(EAGAIN) && EAGAIN != EWOULDBLOCK
			case EAGAIN:
#endif
			    ret = AMQP_PRIVATE_STATUS_SOCKET_NEEDREAD;
			    break;
			default:
			    ret = AMQP_STATUS_SOCKET_ERROR;
		}
	}
	else if(!ret)
		ret = AMQP_STATUS_CONNECTION_CLOSED;
	return ret;
}

static int amqp_tcp_socket_open(void * base, const char * host, int port, struct timeval * timeout) 
{
	struct amqp_tcp_socket_t * self = static_cast<struct amqp_tcp_socket_t *>(base);
	int    result = AMQP_STATUS_OK;
	if(self->sockfd != -1)
		result = AMQP_STATUS_SOCKET_INUSE;
	else {
		self->sockfd = amqp_open_socket_noblock(host, port, timeout);
		if(self->sockfd < 0) {
			int err = self->sockfd;
			self->sockfd = -1;
			result = err;
		}
	}
	return result;
}

static int amqp_tcp_socket_close(void * base, AMQP_UNUSED amqp_socket_close_enum force) 
{
	struct amqp_tcp_socket_t * self = static_cast<struct amqp_tcp_socket_t *>(base);
	if(self->sockfd == -1)
		return AMQP_STATUS_SOCKET_CLOSED;
	else if(amqp_os_socket_close(self->sockfd))
		return AMQP_STATUS_SOCKET_ERROR;
	else {
		self->sockfd = -1;
		return AMQP_STATUS_OK;
	}
}

static int amqp_tcp_socket_get_sockfd(void * base) 
{
	struct amqp_tcp_socket_t * self = static_cast<struct amqp_tcp_socket_t *>(base);
	return self->sockfd;
}

static void amqp_tcp_socket_delete(void * base) 
{
	struct amqp_tcp_socket_t * self = static_cast<struct amqp_tcp_socket_t *>(base);
	if(self) {
		amqp_tcp_socket_close(self, AMQP_SC_NONE);
		SAlloc::F(self);
	}
}

static const amqp_socket_class_t amqp_tcp_socket_class = {
	amqp_tcp_socket_send,   /* send */
	amqp_tcp_socket_recv,   /* recv */
	amqp_tcp_socket_open,   /* open */
	amqp_tcp_socket_close,  /* close */
	amqp_tcp_socket_get_sockfd, /* get_sockfd */
	amqp_tcp_socket_delete  /* delete */
};

amqp_socket_t * amqp_tcp_socket_new(amqp_connection_state_t state) 
{
	struct amqp_tcp_socket_t * self = static_cast<struct amqp_tcp_socket_t *>(SAlloc::C(1, sizeof(*self)));
	if(self) {
		self->klass = &amqp_tcp_socket_class;
		self->sockfd = -1;
		amqp_set_socket(state, reinterpret_cast<amqp_socket_t *>(self));
	}
	return reinterpret_cast<amqp_socket_t *>(self);
}

void amqp_tcp_socket_set_sockfd(amqp_socket_t * base, int sockfd) 
{
	if(base->klass != &amqp_tcp_socket_class) {
		amqp_abort("<%p> is not of type amqp_tcp_socket_t", base);
	}
	{
		struct amqp_tcp_socket_t * self = reinterpret_cast<struct amqp_tcp_socket_t *>(base);
		self->sockfd = sockfd;
	}
}
// } AMQP_TCP_SOCKET

//
// AMQP_CONNECTION {
//
#ifndef AMQP_INITIAL_FRAME_POOL_PAGE_SIZE
	#define AMQP_INITIAL_FRAME_POOL_PAGE_SIZE 65536
#endif
#ifndef AMQP_INITIAL_INBOUND_SOCK_BUFFER_SIZE
	#define AMQP_INITIAL_INBOUND_SOCK_BUFFER_SIZE 131072
#endif
#ifndef AMQP_DEFAULT_LOGIN_TIMEOUT_SEC
	#define AMQP_DEFAULT_LOGIN_TIMEOUT_SEC 12
#endif

#define ENFORCE_STATE(statevec, statenum)                                   \
	{                                                                         \
		amqp_connection_state_t _check_state = (statevec);                      \
		amqp_connection_state_enum _wanted_state = (statenum);                  \
		if(_check_state->state != _wanted_state)                               \
			amqp_abort("Programming error: invalid AMQP connection state: expected %d, got %d", _wanted_state, _check_state->state); \
	}

amqp_connection_state_t amqp_new_connection() 
{
	amqp_connection_state_t state = static_cast<amqp_connection_state_t>(SAlloc::C(1, sizeof(struct amqp_connection_state_t_)));
	if(state) {
		int res = amqp_tune_connection(state, 0, AMQP_INITIAL_FRAME_POOL_PAGE_SIZE, 0);
		if(res != 0) {
			goto out_nomem;
		}
		state->inbound_buffer.bytes = state->header_buffer;
		state->inbound_buffer.len = sizeof(state->header_buffer);
		state->state = CONNECTION_STATE_INITIAL;
		// the server protocol version response is 8 bytes, which conveniently is also the minimum frame size 
		state->target_size = 8;
		state->sock_inbound_buffer.len = AMQP_INITIAL_INBOUND_SOCK_BUFFER_SIZE;
		state->sock_inbound_buffer.bytes = SAlloc::M(AMQP_INITIAL_INBOUND_SOCK_BUFFER_SIZE);
		if(!state->sock_inbound_buffer.bytes) {
			goto out_nomem;
		}
		init_amqp_pool(&state->properties_pool, 512);
		// Use address of the internal_handshake_timeout object by default. 
		state->internal_handshake_timeout.tv_sec = AMQP_DEFAULT_LOGIN_TIMEOUT_SEC;
		state->internal_handshake_timeout.tv_usec = 0;
		state->handshake_timeout = &state->internal_handshake_timeout;
	}
	return state;
out_nomem:
	SAlloc::F(state->sock_inbound_buffer.bytes);
	SAlloc::F(state);
	return NULL;
}

int amqp_get_sockfd(amqp_connection_state_t state) { return state->socket ? amqp_socket_get_sockfd(state->socket) : -1; }
amqp_socket_t * amqp_get_socket(amqp_connection_state_t state) { return state->socket; }
int amqp_get_channel_max(const amqp_connection_state_t state) { return state->channel_max; }
int amqp_get_frame_max(const amqp_connection_state_t state) { return state->frame_max; }
int amqp_get_heartbeat(const amqp_connection_state_t state) { return state->heartbeat; }
amqp_table_t * amqp_get_server_properties(amqp_connection_state_t state) { return &state->server_properties; }
amqp_table_t * amqp_get_client_properties(amqp_connection_state_t state) { return &state->client_properties; }

void amqp_set_sockfd(amqp_connection_state_t state, int sockfd) 
{
	amqp_socket_t * socket = amqp_tcp_socket_new(state);
	if(!socket)
		amqp_abort("%s", strerror(errno));
	amqp_tcp_socket_set_sockfd(socket, sockfd);
}

void amqp_set_socket(amqp_connection_state_t state, amqp_socket_t * socket) 
{
	amqp_socket_delete(state->socket);
	state->socket = socket;
}

int amqp_tune_connection(amqp_connection_state_t state, int channel_max, int frame_max, int heartbeat) 
{
	int res;
	ENFORCE_STATE(state, CONNECTION_STATE_IDLE);
	state->channel_max = channel_max;
	state->frame_max = frame_max;
	state->heartbeat = heartbeat;
	if(0 > state->heartbeat) {
		state->heartbeat = 0;
	}
	res = amqp_time_s_from_now(&state->next_send_heartbeat, amqp_heartbeat_send(state));
	if(res != AMQP_STATUS_OK) {
		return res;
	}
	res = amqp_time_s_from_now(&state->next_recv_heartbeat, amqp_heartbeat_recv(state));
	if(res != AMQP_STATUS_OK) {
		return res;
	}
	state->outbound_buffer.len = frame_max;
	{
		void * newbuf = SAlloc::R(state->outbound_buffer.bytes, frame_max);
		if(!newbuf)
			return AMQP_STATUS_NO_MEMORY;
		else {
			state->outbound_buffer.bytes = newbuf;
			return AMQP_STATUS_OK;
		}
	}
}

int amqp_destroy_connection(amqp_connection_state_t state) 
{
	int status = AMQP_STATUS_OK;
	if(state) {
		for(int i = 0; i < POOL_TABLE_SIZE; ++i) {
			amqp_pool_table_entry_t * entry = state->pool_table[i];
			while(entry) {
				amqp_pool_table_entry_t * todelete = entry;
				empty_amqp_pool(&entry->pool);
				entry = entry->next;
				SAlloc::F(todelete);
			}
		}
		SAlloc::F(state->outbound_buffer.bytes);
		SAlloc::F(state->sock_inbound_buffer.bytes);
		amqp_socket_delete(state->socket);
		empty_amqp_pool(&state->properties_pool);
		SAlloc::F(state);
	}
	return status;
}

static void return_to_idle(amqp_connection_state_t state) 
{
	state->inbound_buffer.len = sizeof(state->header_buffer);
	state->inbound_buffer.bytes = state->header_buffer;
	state->inbound_offset = 0;
	state->target_size = HEADER_SIZE;
	state->state = CONNECTION_STATE_IDLE;
}

static size_t consume_data(amqp_connection_state_t state, amqp_bytes_t * received_data) 
{
	/* how much data is available and will fit? */
	size_t bytes_consumed = state->target_size - state->inbound_offset;
	if(received_data->len < bytes_consumed) {
		bytes_consumed = received_data->len;
	}
	memcpy(amqp_offset(state->inbound_buffer.bytes, state->inbound_offset), received_data->bytes, bytes_consumed);
	state->inbound_offset += bytes_consumed;
	received_data->bytes = amqp_offset(received_data->bytes, bytes_consumed);
	received_data->len -= bytes_consumed;
	return bytes_consumed;
}

int amqp_handle_input(amqp_connection_state_t state, amqp_bytes_t received_data, amqp_frame_t * decoded_frame) 
{
	size_t bytes_consumed;
	void * raw_frame;
	// Returning frame_type of zero indicates either insufficient input, or a complete, ignored frame was read. 
	decoded_frame->frame_type = 0;
	if(received_data.len == 0) {
		return AMQP_STATUS_OK;
	}
	if(state->state == CONNECTION_STATE_IDLE) {
		state->state = CONNECTION_STATE_HEADER;
	}
	bytes_consumed = consume_data(state, &received_data);
	// do we have target_size data yet? if not, return with the expectation that more will arrive 
	if(state->inbound_offset < state->target_size) {
		return (int)bytes_consumed;
	}
	raw_frame = state->inbound_buffer.bytes;
	switch(state->state) {
		case CONNECTION_STATE_INITIAL:
		    /* check for a protocol header from the server */
		    if(memcmp(raw_frame, "AMQP", 4) == 0) {
			    decoded_frame->frame_type = AMQP_PSEUDOFRAME_PROTOCOL_HEADER;
			    decoded_frame->channel = 0;
			    decoded_frame->payload.protocol_header.transport_high = amqp_d8(amqp_offset(raw_frame, 4));
			    decoded_frame->payload.protocol_header.transport_low = amqp_d8(amqp_offset(raw_frame, 5));
			    decoded_frame->payload.protocol_header.protocol_version_major = amqp_d8(amqp_offset(raw_frame, 6));
			    decoded_frame->payload.protocol_header.protocol_version_minor = amqp_d8(amqp_offset(raw_frame, 7));
			    return_to_idle(state);
			    return (int)bytes_consumed;
		    }
		// it's not a protocol header; fall through to process it as a regular frame header 
		case CONNECTION_STATE_HEADER: 
			{
				// frame length is 3 bytes in 
				amqp_channel_t channel = amqp_d16(amqp_offset(raw_frame, 1));
				state->target_size = amqp_d32(amqp_offset(raw_frame, 3)) + HEADER_SIZE + FOOTER_SIZE;
				if((size_t)state->frame_max < state->target_size) {
					return AMQP_STATUS_BAD_AMQP_DATA;
				}
				else {
					amqp_pool_t * channel_pool = amqp_get_or_create_channel_pool(state, channel);
					if(!channel_pool)
						return AMQP_STATUS_NO_MEMORY;
					else {
						amqp_pool_alloc_bytes(channel_pool, state->target_size, &state->inbound_buffer);
						if(!state->inbound_buffer.bytes)
							return AMQP_STATUS_NO_MEMORY;
						else {
							memcpy(state->inbound_buffer.bytes, state->header_buffer, HEADER_SIZE);
							raw_frame = state->inbound_buffer.bytes;
							state->state = CONNECTION_STATE_BODY;
							bytes_consumed += consume_data(state, &received_data);
							// do we have target_size data yet? if not, return with the expectation that more will arrive 
							if(state->inbound_offset < state->target_size)
								return (int)bytes_consumed;
						}
					}
				}
			}
		/* fall through to process body */
		case CONNECTION_STATE_BODY: 
			{
				amqp_bytes_t encoded;
				int res;
				amqp_pool_t * channel_pool;
				/* Check frame end marker (footer) */
				if(amqp_d8(amqp_offset(raw_frame, state->target_size - 1)) != AMQP_FRAME_END) {
					return AMQP_STATUS_BAD_AMQP_DATA;
				}
				decoded_frame->frame_type = amqp_d8(amqp_offset(raw_frame, 0));
				decoded_frame->channel = amqp_d16(amqp_offset(raw_frame, 1));
				channel_pool = amqp_get_or_create_channel_pool(state, decoded_frame->channel);
				if(!channel_pool) {
					return AMQP_STATUS_NO_MEMORY;
				}
				switch(decoded_frame->frame_type) {
					case AMQP_FRAME_METHOD:
						decoded_frame->payload.method.id = amqp_d32(amqp_offset(raw_frame, HEADER_SIZE));
						encoded.bytes = amqp_offset(raw_frame, HEADER_SIZE + 4);
						encoded.len = state->target_size - HEADER_SIZE - 4 - FOOTER_SIZE;
						res = amqp_decode_method(decoded_frame->payload.method.id, channel_pool, encoded, &decoded_frame->payload.method.decoded);
						if(res < 0) {
							return res;
						}
						break;
					case AMQP_FRAME_HEADER:
						decoded_frame->payload.properties.class_id = amqp_d16(amqp_offset(raw_frame, HEADER_SIZE));
						// unused 2-byte weight field goes here 
						decoded_frame->payload.properties.body_size = amqp_d64(amqp_offset(raw_frame, HEADER_SIZE + 4));
						encoded.bytes = amqp_offset(raw_frame, HEADER_SIZE + 12);
						encoded.len = state->target_size - HEADER_SIZE - 12 - FOOTER_SIZE;
						decoded_frame->payload.properties.raw = encoded;
						res = amqp_decode_properties(decoded_frame->payload.properties.class_id, channel_pool, encoded, &decoded_frame->payload.properties.decoded);
						if(res < 0)
							return res;
						break;
					case AMQP_FRAME_BODY:
						decoded_frame->payload.body_fragment.len = state->target_size - HEADER_SIZE - FOOTER_SIZE;
						decoded_frame->payload.body_fragment.bytes = amqp_offset(raw_frame, HEADER_SIZE);
						break;
					case AMQP_FRAME_HEARTBEAT:
						break;
					default:
						// Ignore the frame 
						decoded_frame->frame_type = 0;
						break;
				}
				return_to_idle(state);
				return (int)bytes_consumed;
			}
		default:
		    amqp_abort("Internal error: invalid amqp_connection_state_t->state %d",
			state->state);
	}
}

boolint amqp_release_buffers_ok(const amqp_connection_state_t state) { return (state->state == CONNECTION_STATE_IDLE); }

void amqp_release_buffers(amqp_connection_state_t state) 
{
	ENFORCE_STATE(state, CONNECTION_STATE_IDLE);
	for(int i = 0; i < POOL_TABLE_SIZE; ++i) {
		for(amqp_pool_table_entry_t * entry = state->pool_table[i]; entry; entry = entry->next)
			amqp_maybe_release_buffers_on_channel(state, entry->channel);
	}
}

void amqp_maybe_release_buffers(amqp_connection_state_t state) 
{
	if(amqp_release_buffers_ok(state))
		amqp_release_buffers(state);
}

void amqp_maybe_release_buffers_on_channel(amqp_connection_state_t state, amqp_channel_t channel) 
{
	if(state->state == CONNECTION_STATE_IDLE) {
		for(amqp_link_t * queued_link = state->first_queued_frame; queued_link; queued_link = queued_link->next) {
			amqp_frame_t * frame = static_cast<amqp_frame_t *>(queued_link->data);
			if(channel == frame->channel)
				return;
		}
		{
			amqp_pool_t * p_pool = amqp_get_channel_pool(state, channel);
			recycle_amqp_pool(p_pool);
		}
	}
}

static int amqp_frame_to_bytes(const amqp_frame_t * frame, amqp_bytes_t buffer, amqp_bytes_t * encoded) 
{
	void * out_frame = buffer.bytes;
	size_t out_frame_len;
	int res;
	amqp_e8(frame->frame_type, amqp_offset(out_frame, 0));
	amqp_e16(frame->channel, amqp_offset(out_frame, 1));
	switch(frame->frame_type) {
		case AMQP_FRAME_BODY: 
			{
				const amqp_bytes_t * body = &frame->payload.body_fragment;
				memcpy(amqp_offset(out_frame, HEADER_SIZE), body->bytes, body->len);
				out_frame_len = body->len;
			}
			break;
		case AMQP_FRAME_METHOD: 
			{
				amqp_bytes_t method_encoded;
				amqp_e32(frame->payload.method.id, amqp_offset(out_frame, HEADER_SIZE));
				method_encoded.bytes = amqp_offset(out_frame, HEADER_SIZE + 4);
				method_encoded.len = buffer.len - HEADER_SIZE - 4 - FOOTER_SIZE;
				res = amqp_encode_method(frame->payload.method.id, frame->payload.method.decoded, method_encoded);
				if(res < 0) {
					return res;
				}
				out_frame_len = res + 4;
			}
			break;
		case AMQP_FRAME_HEADER: 
			{
				amqp_bytes_t properties_encoded;
				amqp_e16(frame->payload.properties.class_id, amqp_offset(out_frame, HEADER_SIZE));
				amqp_e16(0, amqp_offset(out_frame, HEADER_SIZE + 2)); /* "weight" */
				amqp_e64(frame->payload.properties.body_size, amqp_offset(out_frame, HEADER_SIZE + 4));
				properties_encoded.bytes = amqp_offset(out_frame, HEADER_SIZE + 12);
				properties_encoded.len = buffer.len - HEADER_SIZE - 12 - FOOTER_SIZE;
				res = amqp_encode_properties(frame->payload.properties.class_id, frame->payload.properties.decoded, properties_encoded);
				if(res < 0) {
					return res;
				}
				out_frame_len = res + 12;
			}
			break;
		case AMQP_FRAME_HEARTBEAT:
		    out_frame_len = 0;
		    break;
		default:
		    return AMQP_STATUS_INVALID_PARAMETER;
	}
	amqp_e32((uint32)out_frame_len, amqp_offset(out_frame, 3));
	amqp_e8(AMQP_FRAME_END, amqp_offset(out_frame, HEADER_SIZE + out_frame_len));
	encoded->bytes = out_frame;
	encoded->len = out_frame_len + HEADER_SIZE + FOOTER_SIZE;
	return AMQP_STATUS_OK;
}

int amqp_send_frame(amqp_connection_state_t state, const amqp_frame_t * frame) 
{
	return amqp_send_frame_inner(state, frame, AMQP_SF_NONE, amqp_time_infinite());
}

int amqp_send_frame_inner(amqp_connection_state_t state, const amqp_frame_t * frame, int flags, amqp_time_t deadline) 
{
	ssize_t sent;
	amqp_bytes_t encoded;
	amqp_time_t next_timeout;
	/* TODO: if the AMQP_SF_MORE socket optimization can be shown to work
	 * correctly, then this could be un-done so that body-frames are sent as 3
	 * send calls, getting rid of the copy of the body content, some testing
	 * would need to be done to see if this would actually a win for performance.
	 * */
	int res = amqp_frame_to_bytes(frame, state->outbound_buffer, &encoded);
	if(res != AMQP_STATUS_OK) {
		return res;
	}
start_send:
	next_timeout = amqp_time_first(deadline, state->next_recv_heartbeat);
	sent = amqp_try_send(state, encoded.bytes, encoded.len, next_timeout, flags);
	if(0 > sent) {
		return (int)sent;
	}
	// A partial send has occurred, because of a heartbeat timeout (so try recv
	// something) or common timeout (so return AMQP_STATUS_TIMEOUT) 
	if((ssize_t)encoded.len != sent) {
		if(amqp_time_equal(next_timeout, deadline))
			return AMQP_STATUS_TIMEOUT; // timeout of method was received, so return from method
		res = amqp_try_recv(state);
		if(AMQP_STATUS_TIMEOUT == res)
			return AMQP_STATUS_HEARTBEAT_TIMEOUT;
		else if(res != AMQP_STATUS_OK)
			return res;
		encoded.bytes = PTR8(encoded.bytes) + sent;
		encoded.len -= sent;
		goto start_send;
	}
	res = amqp_time_s_from_now(&state->next_send_heartbeat, amqp_heartbeat_send(state));
	return res;
}
// } AMQP_CONNECTION
// AMQP_OPENSSL_BIO {
#ifdef MSG_NOSIGNAL
	#define AMQP_USE_AMQP_BIO
#endif

#ifdef AMQP_USE_AMQP_BIO
	static BIO_METHOD * amqp_bio_method;

	static int amqp_openssl_bio_should_retry(int res) 
	{
		if(res == -1) {
			int err = amqp_os_socket_error();
			if(
	#ifdef EWOULDBLOCK
				err == EWOULDBLOCK ||
	#endif
	#ifdef WSAEWOULDBLOCK
				err == WSAEWOULDBLOCK ||
	#endif
	#ifdef ENOTCONN
				err == ENOTCONN ||
	#endif
	#ifdef EINTR
				err == EINTR ||
	#endif
	#ifdef EAGAIN
				err == EAGAIN ||
	#endif
	#ifdef EPROTO
				err == EPROTO ||
	#endif
	#ifdef EINPROGRESS
				err == EINPROGRESS ||
	#endif
	#ifdef EALREADY
				err == EALREADY ||
	#endif
				0) {
				return 1;
			}
		}
		return 0;
	}

	static int amqp_openssl_bio_write(BIO * b, const char * in, int inl) 
	{
		int flags = 0;
		int fd;
		int res;
	#ifdef MSG_NOSIGNAL
		flags |= MSG_NOSIGNAL;
	#endif
		BIO_get_fd(b, &fd);
		res = send(fd, in, inl, flags);
		BIO_clear_retry_flags(b);
		if(res <= 0 && amqp_openssl_bio_should_retry(res)) {
			BIO_set_retry_write(b);
		}
		return res;
	}

	static int amqp_openssl_bio_read(BIO * b, char * out, int outl) 
	{
		int flags = 0;
		int fd;
		int res;
	#ifdef MSG_NOSIGNAL
		flags |= MSG_NOSIGNAL;
	#endif
		BIO_get_fd(b, &fd);
		res = recv(fd, out, outl, flags);
		BIO_clear_retry_flags(b);
		if(res <= 0 && amqp_openssl_bio_should_retry(res)) {
			BIO_set_retry_read(b);
		}
		return res;
	}

	#ifndef AMQP_OPENSSL_V110
		static int BIO_meth_set_write(BIO_METHOD * biom, int (*wfn)(BIO *, const char *, int)) 
		{
			biom->bwrite = wfn;
			return 0;
		}
		static int BIO_meth_set_read(BIO_METHOD * biom, int (*rfn)(BIO *, char *, int)) 
		{
			biom->bread = rfn;
			return 0;
		}
	#endif
#endif // AMQP_USE_AMQP_BIO 

int amqp_openssl_bio_init() 
{
	assert(!amqp_ssl_bio_initialized);
#ifdef AMQP_USE_AMQP_BIO
#ifdef AMQP_OPENSSL_V110
	if(!(amqp_bio_method = BIO_meth_new(BIO_TYPE_SOCKET, "amqp_bio_method"))) {
		return AMQP_STATUS_NO_MEMORY;
	}
	// casting away const is necessary until
	// https://github.com/openssl/openssl/pull/2181/, which is targeted for
	// openssl 1.1.1
	BIO_METHOD * meth = (BIO_METHOD*)BIO_s_socket();
	BIO_meth_set_create(amqp_bio_method, BIO_meth_get_create(meth));
	BIO_meth_set_destroy(amqp_bio_method, BIO_meth_get_destroy(meth));
	BIO_meth_set_ctrl(amqp_bio_method, BIO_meth_get_ctrl(meth));
	BIO_meth_set_callback_ctrl(amqp_bio_method, BIO_meth_get_callback_ctrl(meth));
	BIO_meth_set_read(amqp_bio_method, BIO_meth_get_read(meth));
	BIO_meth_set_write(amqp_bio_method, BIO_meth_get_write(meth));
	BIO_meth_set_gets(amqp_bio_method, BIO_meth_get_gets(meth));
	BIO_meth_set_puts(amqp_bio_method, BIO_meth_get_puts(meth));
#else
	if(!(amqp_bio_method = OPENSSL_malloc(sizeof(BIO_METHOD)))) {
		return AMQP_STATUS_NO_MEMORY;
	}
	memcpy(amqp_bio_method, BIO_s_socket(), sizeof(BIO_METHOD));
#endif
	BIO_meth_set_write(amqp_bio_method, amqp_openssl_bio_write);
	BIO_meth_set_read(amqp_bio_method, amqp_openssl_bio_read);
#endif
	amqp_ssl_bio_initialized = 1;
	return AMQP_STATUS_OK;
}

void amqp_openssl_bio_destroy()
{
	assert(amqp_ssl_bio_initialized);
#ifdef AMQP_USE_AMQP_BIO
#ifdef AMQP_OPENSSL_V110
	BIO_meth_free(amqp_bio_method);
#else
	OPENSSL_free(amqp_bio_method);
#endif
	amqp_bio_method = NULL;
#endif
	amqp_ssl_bio_initialized = 0;
}

BIO_METHOD_PTR amqp_openssl_bio()
{
	assert(amqp_ssl_bio_initialized);
#ifdef AMQP_USE_AMQP_BIO
	return amqp_bio_method;
#else
	return BIO_s_socket();
#endif
}
// } AMQP_OPENSSL_BIO
// AMQP_OPENSSL_HOSTNAME_VALIDATION {
// 
// Originally from:
// https://github.com/iSECPartners/ssl-conservatory
// https://wiki.openssl.org/index.php/Hostname_validation
// 
#define HOSTNAME_MAX_SIZE 255
/**
 * Tries to find a match for hostname in the certificate's Common Name field.
 *
 * Returns AMQP_HVR_MATCH_FOUND if a match was found.
 * Returns AMQP_HVR_MATCH_NOT_FOUND if no matches were found.
 * Returns AMQP_HVR_MALFORMED_CERTIFICATE if the Common Name had a NUL character
 * embedded in it.
 * Returns AMQP_HVR_ERROR if the Common Name could not be extracted.
 */
static amqp_hostname_validation_result amqp_matches_common_name(const char * hostname, const X509 * server_cert) 
{
	X509_NAME_ENTRY * common_name_entry = NULL;
	ASN1_STRING * common_name_asn1 = NULL;
	const char * common_name_str = NULL;
	// Find the position of the CN field in the Subject field of the certificate
	int common_name_loc = X509_NAME_get_index_by_NID(X509_get_subject_name((X509*)server_cert), NID_commonName, -1);
	if(common_name_loc < 0) {
		return AMQP_HVR_ERROR;
	}
	// Extract the CN field
	common_name_entry = X509_NAME_get_entry(X509_get_subject_name((X509*)server_cert), common_name_loc);
	if(!common_name_entry) {
		return AMQP_HVR_ERROR;
	}
	// Convert the CN field to a C string
	common_name_asn1 = X509_NAME_ENTRY_get_data(common_name_entry);
	if(!common_name_asn1) {
		return AMQP_HVR_ERROR;
	}
#ifdef AMQP_OPENSSL_V110
	common_name_str = (const char *)ASN1_STRING_get0_data(common_name_asn1);
#else
	common_name_str = (char *)ASN1_STRING_data(common_name_asn1);
#endif
	// Make sure there isn't an embedded NUL character in the CN
	if((size_t)ASN1_STRING_length(common_name_asn1) != sstrlen(common_name_str)) {
		return AMQP_HVR_MALFORMED_CERTIFICATE;
	}
	// Compare expected hostname with the CN
	if(amqp_hostcheck(common_name_str, hostname) == AMQP_HCR_MATCH)
		return AMQP_HVR_MATCH_FOUND;
	else
		return AMQP_HVR_MATCH_NOT_FOUND;
}
/**
 * Tries to find a match for hostname in the certificate's Subject Alternative
 * Name extension.
 *
 * Returns AMQP_HVR_MATCH_FOUND if a match was found.
 * Returns AMQP_HVR_MATCH_NOT_FOUND if no matches were found.
 * Returns AMQP_HVR_MALFORMED_CERTIFICATE if any of the hostnames had a NUL
 * character embedded in it.
 * Returns AMQP_HVR_NO_SAN_PRESENT if the SAN extension was not present in the
 * certificate.
 */
static amqp_hostname_validation_result amqp_matches_subject_alternative_name(const char * hostname, const X509 * server_cert) 
{
	amqp_hostname_validation_result result = AMQP_HVR_MATCH_NOT_FOUND;
	int i;
	int san_names_nb = -1;
	// Try to extract the names within the SAN extension from the certificate
	STACK_OF(GENERAL_NAME) * san_names = static_cast<STACK_OF(GENERAL_NAME) *>(X509_get_ext_d2i((X509*)server_cert, NID_subject_alt_name, NULL, NULL));
	if(san_names == NULL) {
		return AMQP_HVR_NO_SAN_PRESENT;
	}
	san_names_nb = sk_GENERAL_NAME_num(san_names);
	// Check each name within the extension
	for(i = 0; i < san_names_nb; i++) {
		const GENERAL_NAME * current_name = sk_GENERAL_NAME_value(san_names, i);
		if(current_name->type == GEN_DNS) {
			// Current name is a DNS name, let's check it
#ifdef AMQP_OPENSSL_V110
		    const char * dns_name = reinterpret_cast<const char *>(ASN1_STRING_get0_data(current_name->d.dNSName));
#else
		    const char * dns_name = reinterpret_cast<const char *>(ASN1_STRING_data(current_name->d.dNSName));
#endif
			// Make sure there isn't an embedded NUL character in the DNS name
			if((size_t)ASN1_STRING_length(current_name->d.dNSName) != sstrlen(dns_name)) {
				result = AMQP_HVR_MALFORMED_CERTIFICATE;
				break;
			}
			else if(amqp_hostcheck(dns_name, hostname) == AMQP_HCR_MATCH) { // Compare expected hostname with the DNS name
				result = AMQP_HVR_MATCH_FOUND;
				break;
			}
		}
	}
	sk_GENERAL_NAME_pop_free(san_names, GENERAL_NAME_free);
	return result;
}
/**
 * Validates the server's identity by looking for the expected hostname in the
 * server's certificate. As described in RFC 6125, it first tries to find a
 * match in the Subject Alternative Name extension. If the extension is not
 * present in the certificate, it checks the Common Name instead.
 *
 * Returns AMQP_HVR_MATCH_FOUND if a match was found.
 * Returns AMQP_HVR_MATCH_NOT_FOUND if no matches were found.
 * Returns AMQP_HVR_MALFORMED_CERTIFICATE if any of the hostnames had a NUL
 * character embedded in it.
 * Returns AMQP_HVR_ERROR if there was an error.
 */
amqp_hostname_validation_result amqp_ssl_validate_hostname(const char * hostname, const X509 * server_cert) 
{
	amqp_hostname_validation_result result;
	if(!hostname || !server_cert) 
		return AMQP_HVR_ERROR;
	// First try the Subject Alternative Names extension
	result = amqp_matches_subject_alternative_name(hostname, server_cert);
	if(result == AMQP_HVR_NO_SAN_PRESENT)
		result = amqp_matches_common_name(hostname, server_cert); // Extension was not found: try the Common Name
	return result;
}
// } AMQP_OPENSSL_HOSTNAME_VALIDATION 
// AMQP_OPENSSL {
static int initialize_ssl_and_increment_connections();
static int decrement_ssl_connections();
static /*unsigned long*/pthread_t ssl_threadid_callback();
static void ssl_locking_callback(int mode, int n, const char * file, int line);

#define CHECK_SUCCESS(condition)                                            \
	do {                                                                      \
		int check_success_ret = (condition);                                    \
		if(check_success_ret) {                                                \
			amqp_abort("Check %s failed <%d>: %s", #condition, check_success_ret, strerror(check_success_ret)); \
		}                                                                       \
	} while(0)

struct amqp_ssl_socket_t {
	const amqp_socket_class_t * klass;
	SSL_CTX * ctx;
	int sockfd;
	SSL * ssl;
	boolint verify_peer;
	boolint verify_hostname;
	int internal_error;
};

static ssize_t amqp_ssl_socket_send(void * base, const void * buf, size_t len, AMQP_UNUSED int flags) 
{
	amqp_ssl_socket_t * self = static_cast<amqp_ssl_socket_t *>(base);
	int res;
	if(self->sockfd == -1) {
		return AMQP_STATUS_SOCKET_CLOSED;
	}
	// SSL_write takes an int for length of buffer, protect against len being
	// larger than larger than what SSL_write can take 
	if(len > INT_MAX) {
		return AMQP_STATUS_INVALID_PARAMETER;
	}
	ERR_clear_error();
	self->internal_error = 0;
	// This will only return on error, or once the whole buffer has been
	// written to the SSL stream. See SSL_MODE_ENABLE_PARTIAL_WRITE 
	res = SSL_write(self->ssl, buf, (int)len);
	if(0 >= res) {
		self->internal_error = SSL_get_error(self->ssl, res);
		// TODO: Close connection if it isn't already? 
		// TODO: Possibly be more intelligent in reporting WHAT went wrong 
		switch(self->internal_error) {
			case SSL_ERROR_WANT_READ: res = AMQP_PRIVATE_STATUS_SOCKET_NEEDREAD; break;
			case SSL_ERROR_WANT_WRITE: res = AMQP_PRIVATE_STATUS_SOCKET_NEEDWRITE; break;
			case SSL_ERROR_ZERO_RETURN: res = AMQP_STATUS_CONNECTION_CLOSED; break;
			default: res = AMQP_STATUS_SSL_ERROR; break;
		}
	}
	else {
		self->internal_error = 0;
	}
	return (ssize_t)res;
}

static ssize_t amqp_ssl_socket_recv(void * base, void * buf, size_t len, AMQP_UNUSED int flags) 
{
	amqp_ssl_socket_t * self = static_cast<amqp_ssl_socket_t *>(base);
	int received;
	if(self->sockfd == -1) {
		return AMQP_STATUS_SOCKET_CLOSED;
	}
	// SSL_read takes an int for length of buffer, protect against len being
	// larger than larger than what SSL_read can take 
	if(len > INT_MAX) {
		return AMQP_STATUS_INVALID_PARAMETER;
	}
	ERR_clear_error();
	self->internal_error = 0;
	received = SSL_read(self->ssl, buf, (int)len);
	if(0 >= received) {
		self->internal_error = SSL_get_error(self->ssl, received);
		switch(self->internal_error) {
			case SSL_ERROR_WANT_READ: received = AMQP_PRIVATE_STATUS_SOCKET_NEEDREAD; break;
			case SSL_ERROR_WANT_WRITE: received = AMQP_PRIVATE_STATUS_SOCKET_NEEDWRITE; break;
			case SSL_ERROR_ZERO_RETURN: received = AMQP_STATUS_CONNECTION_CLOSED; break;
			default: received = AMQP_STATUS_SSL_ERROR; break;
		}
	}
	return (ssize_t)received;
}

static int amqp_ssl_socket_open(void * base, const char * host, int port, struct timeval * timeout) 
{
	amqp_ssl_socket_t * self = static_cast<amqp_ssl_socket_t *>(base);
	long result;
	int status;
	amqp_time_t deadline;
	X509 * cert;
	BIO * bio;
	if(-1 != self->sockfd) {
		return AMQP_STATUS_SOCKET_INUSE;
	}
	ERR_clear_error();
	self->ssl = SSL_new(self->ctx);
	if(!self->ssl) {
		self->internal_error = ERR_peek_error();
		status = AMQP_STATUS_SSL_ERROR;
		goto exit;
	}
	status = amqp_time_from_now(&deadline, timeout);
	if(AMQP_STATUS_OK != status) {
		return status;
	}
	self->sockfd = amqp_open_socket_inner(host, port, deadline);
	if(0 > self->sockfd) {
		status = self->sockfd;
		self->internal_error = amqp_os_socket_error();
		self->sockfd = -1;
		goto error_out1;
	}
	bio = BIO_new(amqp_openssl_bio());
	if(!bio) {
		status = AMQP_STATUS_NO_MEMORY;
		goto error_out2;
	}
	BIO_set_fd(bio, self->sockfd, BIO_NOCLOSE);
	SSL_set_bio(self->ssl, bio, bio);
	status = SSL_set_tlsext_host_name(self->ssl, host);
	if(!status) {
		self->internal_error = SSL_get_error(self->ssl, status);
		status = AMQP_STATUS_SSL_ERROR;
		goto error_out2;
	}
start_connect:
	status = SSL_connect(self->ssl);
	if(status != 1) {
		self->internal_error = SSL_get_error(self->ssl, status);
		switch(self->internal_error) {
			case SSL_ERROR_WANT_READ: status = amqp_poll(self->sockfd, AMQP_SF_POLLIN, deadline); break;
			case SSL_ERROR_WANT_WRITE: status = amqp_poll(self->sockfd, AMQP_SF_POLLOUT, deadline); break;
			default: status = AMQP_STATUS_SSL_CONNECTION_FAILED;
		}
		if(AMQP_STATUS_OK == status) {
			goto start_connect;
		}
		goto error_out2;
	}
	cert = SSL_get_peer_certificate(self->ssl);
	if(self->verify_peer) {
		if(!cert) {
			self->internal_error = 0;
			status = AMQP_STATUS_SSL_PEER_VERIFY_FAILED;
			goto error_out3;
		}
		result = SSL_get_verify_result(self->ssl);
		if(X509_V_OK != result) {
			self->internal_error = result;
			status = AMQP_STATUS_SSL_PEER_VERIFY_FAILED;
			goto error_out4;
		}
	}
	if(self->verify_hostname) {
		if(!cert) {
			self->internal_error = 0;
			status = AMQP_STATUS_SSL_HOSTNAME_VERIFY_FAILED;
			goto error_out3;
		}
		if(AMQP_HVR_MATCH_FOUND != amqp_ssl_validate_hostname(host, cert)) {
			self->internal_error = 0;
			status = AMQP_STATUS_SSL_HOSTNAME_VERIFY_FAILED;
			goto error_out4;
		}
	}
	X509_free(cert);
	self->internal_error = 0;
	status = AMQP_STATUS_OK;
exit:
	return status;
error_out4:
	X509_free(cert);
error_out3:
	SSL_shutdown(self->ssl);
error_out2:
	amqp_os_socket_close(self->sockfd);
	self->sockfd = -1;
error_out1:
	SSL_free(self->ssl);
	self->ssl = NULL;
	goto exit;
}

static int amqp_ssl_socket_close(void * base, amqp_socket_close_enum force) 
{
	amqp_ssl_socket_t * self = static_cast<amqp_ssl_socket_t *>(base);
	if(self->sockfd == -1) {
		return AMQP_STATUS_SOCKET_CLOSED;
	}
	if(AMQP_SC_NONE == force) {
		SSL_shutdown(self->ssl); // don't try too hard to shutdown the connection 
	}
	SSL_free(self->ssl);
	self->ssl = NULL;
	if(amqp_os_socket_close(self->sockfd)) {
		return AMQP_STATUS_SOCKET_ERROR;
	}
	self->sockfd = -1;
	return AMQP_STATUS_OK;
}

static int amqp_ssl_socket_get_sockfd(void * base) 
{
	amqp_ssl_socket_t * self = static_cast<amqp_ssl_socket_t *>(base);
	return self->sockfd;
}

static void amqp_ssl_socket_delete(void * base) 
{
	amqp_ssl_socket_t * self = static_cast<amqp_ssl_socket_t *>(base);
	if(self) {
		amqp_ssl_socket_close(self, AMQP_SC_NONE);
		SSL_CTX_free(self->ctx);
		SAlloc::F(self);
	}
	decrement_ssl_connections();
}

static const amqp_socket_class_t amqp_ssl_socket_class = {
	amqp_ssl_socket_send,
	amqp_ssl_socket_recv,
	amqp_ssl_socket_open,
	amqp_ssl_socket_close,
	amqp_ssl_socket_get_sockfd,
	amqp_ssl_socket_delete
};

amqp_socket_t * amqp_ssl_socket_new(amqp_connection_state_t state) 
{
	amqp_ssl_socket_t * self = static_cast<amqp_ssl_socket_t *>(SAlloc::C(1, sizeof(*self)));
	int status;
	if(!self) {
		return NULL;
	}
	self->sockfd = -1;
	self->klass = &amqp_ssl_socket_class;
	self->verify_peer = 1;
	self->verify_hostname = 1;
	status = initialize_ssl_and_increment_connections();
	if(status) {
		goto error;
	}
	self->ctx = SSL_CTX_new(SSLv23_client_method());
	if(!self->ctx) {
		goto error;
	}
	/* Disable SSLv2 and SSLv3 */
	SSL_CTX_set_options(self->ctx, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3);
	amqp_set_socket(state, (amqp_socket_t*)self);
	return (amqp_socket_t*)self;
error:
	amqp_ssl_socket_delete((amqp_socket_t*)self);
	return NULL;
}

void * amqp_ssl_socket_get_context(amqp_socket_t * base) 
{
	if(base->klass != &amqp_ssl_socket_class) {
		amqp_abort("<%p> is not of type amqp_ssl_socket_t", base);
	}
	return reinterpret_cast<amqp_ssl_socket_t *>(base)->ctx;
}

int amqp_ssl_socket_set_cacert(amqp_socket_t * base, const char * cacert) 
{
	if(base->klass != &amqp_ssl_socket_class) {
		amqp_abort("<%p> is not of type amqp_ssl_socket_t", base);
	}
	{
		amqp_ssl_socket_t * self = reinterpret_cast<amqp_ssl_socket_t *>(base);
		int status = SSL_CTX_load_verify_locations(self->ctx, cacert, NULL);
		if(1 != status) {
			return AMQP_STATUS_SSL_ERROR;
		}
		return AMQP_STATUS_OK;
	}
}

int amqp_ssl_socket_set_key(amqp_socket_t * base, const char * cert, const char * key) 
{
	if(base->klass != &amqp_ssl_socket_class) {
		amqp_abort("<%p> is not of type amqp_ssl_socket_t", base);
	}
	{
		amqp_ssl_socket_t * self = reinterpret_cast<amqp_ssl_socket_t *>(base);
		int status = SSL_CTX_use_certificate_chain_file(self->ctx, cert);
		if(1 != status) {
			return AMQP_STATUS_SSL_ERROR;
		}
		status = SSL_CTX_use_PrivateKey_file(self->ctx, key, SSL_FILETYPE_PEM);
		if(1 != status) {
			return AMQP_STATUS_SSL_ERROR;
		}
		return AMQP_STATUS_OK;
	}
}

static int password_cb(AMQP_UNUSED char * buffer, AMQP_UNUSED int length, AMQP_UNUSED int rwflag, AMQP_UNUSED void * user_data) 
{
	amqp_abort("rabbitmq-c does not support password protected keys");
}

int amqp_ssl_socket_set_key_buffer(amqp_socket_t * base, const char * cert, const void * key, size_t n) 
{
	int status = AMQP_STATUS_OK;
	BIO * buf = NULL;
	RSA * rsa = NULL;
	amqp_ssl_socket_t * self;
	if(base->klass != &amqp_ssl_socket_class) {
		amqp_abort("<%p> is not of type amqp_ssl_socket_t", base);
	}
	if(n > INT_MAX) {
		return AMQP_STATUS_INVALID_PARAMETER;
	}
	self = reinterpret_cast<amqp_ssl_socket_t *>(base);
	status = SSL_CTX_use_certificate_chain_file(self->ctx, cert);
	if(1 != status) {
		return AMQP_STATUS_SSL_ERROR;
	}
	buf = BIO_new_mem_buf((void *)key, (int)n);
	if(!buf) {
		goto error;
	}
	rsa = PEM_read_bio_RSAPrivateKey(buf, NULL, password_cb, NULL);
	if(!rsa) {
		goto error;
	}
	status = SSL_CTX_use_RSAPrivateKey(self->ctx, rsa);
	if(1 != status) {
		goto error;
	}
exit:
	BIO_vfree(buf);
	RSA_free(rsa);
	return status;
error:
	status = AMQP_STATUS_SSL_ERROR;
	goto exit;
}

int amqp_ssl_socket_set_cert(amqp_socket_t * base, const char * cert) 
{
	if(base->klass != &amqp_ssl_socket_class) {
		amqp_abort("<%p> is not of type amqp_ssl_socket_t", base);
	}
	else {
		amqp_ssl_socket_t * self = reinterpret_cast<amqp_ssl_socket_t *>(base);
		int status = SSL_CTX_use_certificate_chain_file(self->ctx, cert);
		return (status != 1) ? AMQP_STATUS_SSL_ERROR : AMQP_STATUS_OK;
	}
}

void amqp_ssl_socket_set_verify(amqp_socket_t * base, boolint verify) 
{
	amqp_ssl_socket_set_verify_peer(base, verify);
	amqp_ssl_socket_set_verify_hostname(base, verify);
}

void amqp_ssl_socket_set_verify_peer(amqp_socket_t * base, boolint verify) 
{
	if(base->klass != &amqp_ssl_socket_class)
		amqp_abort("<%p> is not of type amqp_ssl_socket_t", base);
	amqp_ssl_socket_t * self = reinterpret_cast<amqp_ssl_socket_t *>(base);
	self->verify_peer = verify;
}

void amqp_ssl_socket_set_verify_hostname(amqp_socket_t * base, boolint verify) 
{
	if(base->klass != &amqp_ssl_socket_class)
		amqp_abort("<%p> is not of type amqp_ssl_socket_t", base);
	amqp_ssl_socket_t * self = reinterpret_cast<amqp_ssl_socket_t *>(base);
	self->verify_hostname = verify;
}

int amqp_ssl_socket_set_ssl_versions(amqp_socket_t * base, amqp_tls_version_t min, amqp_tls_version_t max) 
{
	if(base->klass != &amqp_ssl_socket_class) {
		amqp_abort("<%p> is not of type amqp_ssl_socket_t", base);
	}
	amqp_ssl_socket_t * self = reinterpret_cast<amqp_ssl_socket_t *>(base);
	{
		long clear_options;
		long set_options = 0;
#if defined(SSL_OP_NO_TLSv1_2)
		amqp_tls_version_t max_supported = AMQP_TLSv1_2;
		clear_options = SSL_OP_NO_TLSv1 | SSL_OP_NO_TLSv1_1 | SSL_OP_NO_TLSv1_2;
#elif defined(SSL_OP_NO_TLSv1_1)
		amqp_tls_version_t max_supported = AMQP_TLSv1_1;
		clear_options = SSL_OP_NO_TLSv1 | SSL_OP_NO_TLSv1_1;
#elif defined(SSL_OP_NO_TLSv1)
		amqp_tls_version_t max_supported = AMQP_TLSv1;
		clear_options = SSL_OP_NO_TLSv1;
#else
#error "Need a version of OpenSSL that can support TLSv1 or greater."
#endif
		if(AMQP_TLSvLATEST == max)
			max = max_supported;
		if(AMQP_TLSvLATEST == min)
			min = max_supported;
		if(min > max)
			return AMQP_STATUS_INVALID_PARAMETER;
		if(max > max_supported || min > max_supported)
			return AMQP_STATUS_UNSUPPORTED;
		if(min > AMQP_TLSv1)
			set_options |= SSL_OP_NO_TLSv1;
#ifdef SSL_OP_NO_TLSv1_1
		if(min > AMQP_TLSv1_1 || max < AMQP_TLSv1_1)
			set_options |= SSL_OP_NO_TLSv1_1;
#endif
#ifdef SSL_OP_NO_TLSv1_2
		if(max < AMQP_TLSv1_2)
			set_options |= SSL_OP_NO_TLSv1_2;
#endif
		SSL_CTX_clear_options(self->ctx, clear_options);
		SSL_CTX_set_options(self->ctx, set_options);
	}
	return AMQP_STATUS_OK;
}

void amqp_set_initialize_ssl_library(boolint do_initialize) 
{
	CHECK_SUCCESS(pthread_mutex_lock(&openssl_init_mutex));
	if(openssl_connections == 0 && !openssl_initialized) {
		do_initialize_openssl = do_initialize;
	}
	CHECK_SUCCESS(pthread_mutex_unlock(&openssl_init_mutex));
}

static /*unsigned long*/pthread_t ssl_threadid_callback() 
{
	return /*(unsigned long)*/pthread_self();
}

static void ssl_locking_callback(int mode, int n, AMQP_UNUSED const char * file, AMQP_UNUSED int line) 
{
	if(mode & CRYPTO_LOCK) {
		CHECK_SUCCESS(pthread_mutex_lock(&amqp_openssl_lockarray[n]));
	}
	else {
		CHECK_SUCCESS(pthread_mutex_unlock(&amqp_openssl_lockarray[n]));
	}
}

static int setup_openssl()
{
	int status;
	amqp_openssl_lockarray = static_cast<pthread_mutex_t *>(SAlloc::C(CRYPTO_num_locks(), sizeof(pthread_mutex_t)));
	if(!amqp_openssl_lockarray) {
		status = AMQP_STATUS_NO_MEMORY;
		goto out;
	}
	for(int i = 0; i < CRYPTO_num_locks(); i++) {
		if(pthread_mutex_init(&amqp_openssl_lockarray[i], NULL)) {
			for(int j = 0; j < i; j++) {
				pthread_mutex_destroy(&amqp_openssl_lockarray[j]);
			}
			SAlloc::F(amqp_openssl_lockarray);
			status = AMQP_STATUS_SSL_ERROR;
			goto out;
		}
	}
	CRYPTO_set_id_callback(ssl_threadid_callback);
	CRYPTO_set_locking_callback(ssl_locking_callback);
#ifdef AMQP_OPENSSL_V110
	if(CONF_modules_load_file(NULL, "rabbitmq-c", CONF_MFLAGS_DEFAULT_SECTION|CONF_MFLAGS_IGNORE_MISSING_FILE) <= 0) {
		status = AMQP_STATUS_SSL_ERROR;
		goto out;
	}
#else
	OPENSSL_config(NULL);
#endif
	SSL_library_init();
	SSL_load_error_strings();
	status = AMQP_STATUS_OK;
out:
	return status;
}

int amqp_initialize_ssl_library() 
{
	int status;
	CHECK_SUCCESS(pthread_mutex_lock(&openssl_init_mutex));
	if(!openssl_initialized) {
		status = setup_openssl();
		if(status) {
			goto out;
		}
		openssl_initialized = 1;
	}
	status = AMQP_STATUS_OK;
out:
	CHECK_SUCCESS(pthread_mutex_unlock(&openssl_init_mutex));
	return status;
}

static int initialize_ssl_and_increment_connections() 
{
	int status;
	CHECK_SUCCESS(pthread_mutex_lock(&openssl_init_mutex));
	if(do_initialize_openssl && !openssl_initialized) {
		status = setup_openssl();
		if(status) {
			goto exit;
		}
		openssl_initialized = 1;
	}
	if(!openssl_bio_initialized) {
		status = amqp_openssl_bio_init();
		if(status) {
			goto exit;
		}
		openssl_bio_initialized = 1;
	}
	openssl_connections += 1;
	status = AMQP_STATUS_OK;
exit:
	CHECK_SUCCESS(pthread_mutex_unlock(&openssl_init_mutex));
	return status;
}

static int decrement_ssl_connections() 
{
	CHECK_SUCCESS(pthread_mutex_lock(&openssl_init_mutex));
	if(openssl_connections > 0)
		openssl_connections--;
	CHECK_SUCCESS(pthread_mutex_unlock(&openssl_init_mutex));
	return AMQP_STATUS_OK;
}

int amqp_uninitialize_ssl_library() 
{
	int status;
	CHECK_SUCCESS(pthread_mutex_lock(&openssl_init_mutex));
	if(openssl_connections > 0) {
		status = AMQP_STATUS_SOCKET_INUSE;
		goto out;
	}
	amqp_openssl_bio_destroy();
	openssl_bio_initialized = 0;
#ifndef AMQP_OPENSSL_V110
	ERR_remove_state(0);
#endif
#ifndef LIBRESSL_VERSION_NUMBER
	FIPS_mode_set(0);
#endif
	CRYPTO_set_locking_callback(NULL);
	CRYPTO_set_id_callback(NULL);
	{
		for(int i = 0; i < CRYPTO_num_locks(); i++) {
			pthread_mutex_destroy(&amqp_openssl_lockarray[i]);
		}
		SAlloc::F(amqp_openssl_lockarray);
	}
	ENGINE_cleanup();
	CONF_modules_free();
	EVP_cleanup();
	CRYPTO_cleanup_all_ex_data();
	ERR_free_strings();
#if (OPENSSL_VERSION_NUMBER >= 0x10002003L) && !defined(LIBRESSL_VERSION_NUMBER)
	SSL_COMP_free_compression_methods();
#endif
	openssl_initialized = 0;
	status = AMQP_STATUS_OK;
out:
	CHECK_SUCCESS(pthread_mutex_unlock(&openssl_init_mutex));
	return status;
}
// } AMQP_OPENSSL 
