// amqp.h
// @codepage UTF-8
//
/*
 * Version: MIT
 *
 * Portions created by Alan Antonuk are Copyright (c) 2012-2014
 * Alan Antonuk. All Rights Reserved.
 *
 * Portions created by VMware are Copyright (c) 2007-2012 VMware, Inc. All Rights Reserved.
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
 */
#ifndef AMQP_H
#define AMQP_H
/** \cond HIDE_FROM_DOXYGEN */
#define AMQP_STATIC // @sobolev
#ifdef __cplusplus
	#define AMQP_BEGIN_DECLS // extern "C" {
	#define AMQP_END_DECLS // }
#else
	#define AMQP_BEGIN_DECLS
	#define AMQP_END_DECLS
#endif
/*
 * \internal
 * Important API decorators:
 *  AMQP_PUBLIC_FUNCTION - a public API function
 *  AMQP_PUBLIC_VARIABLE - a public API external variable
 *  - calling convension (used on Win32)
 */
#if defined(_WIN32) && defined(_MSC_VER)
	#if defined(AMQP_BUILD) && !defined(AMQP_STATIC)
		#define AMQP_PUBLIC_FUNCTION __declspec(dllexport)
		#define AMQP_PUBLIC_VARIABLE __declspec(dllexport) extern
	#else
		#define AMQP_PUBLIC_FUNCTION
		#if !defined(AMQP_STATIC)
			#define AMQP_PUBLIC_VARIABLE __declspec(dllimport) extern
		#else
			#define AMQP_PUBLIC_VARIABLE extern
		#endif
	#endif
#elif defined(_WIN32) && defined(__BORLANDC__)
	#if defined(AMQP_BUILD) && !defined(AMQP_STATIC)
		#define AMQP_PUBLIC_FUNCTION __declspec(dllexport)
		#define AMQP_PUBLIC_VARIABLE __declspec(dllexport) extern
	#else
		#define AMQP_PUBLIC_FUNCTION
		#if !defined(AMQP_STATIC)
			#define AMQP_PUBLIC_VARIABLE __declspec(dllimport) extern
		#else
			#define AMQP_PUBLIC_VARIABLE extern
		#endif
	#endif
#elif defined(_WIN32) && defined(__MINGW32__)
	#if defined(AMQP_BUILD) && !defined(AMQP_STATIC)
		#define AMQP_PUBLIC_FUNCTION __declspec(dllexport)
		#define AMQP_PUBLIC_VARIABLE __declspec(dllexport) extern
	#else
		#define AMQP_PUBLIC_FUNCTION
		#if !defined(AMQP_STATIC)
			#define AMQP_PUBLIC_VARIABLE __declspec(dllimport) extern
		#else
			#define AMQP_PUBLIC_VARIABLE extern
		#endif
	#endif
#elif defined(_WIN32) && defined(__CYGWIN__)
	#if defined(AMQP_BUILD) && !defined(AMQP_STATIC)
		#define AMQP_PUBLIC_FUNCTION __declspec(dllexport)
		#define AMQP_PUBLIC_VARIABLE __declspec(dllexport)
	#else
		#define AMQP_PUBLIC_FUNCTION
		#if !defined(AMQP_STATIC)
			#define AMQP_PUBLIC_VARIABLE __declspec(dllimport) extern
		#else
			#define AMQP_PUBLIC_VARIABLE extern
		#endif
	#endif
#elif defined(__GNUC__) && __GNUC__ >= 4
	#define AMQP_PUBLIC_FUNCTION __attribute__((visibility("default")))
	#define AMQP_PUBLIC_VARIABLE __attribute__((visibility("default"))) extern
#else
	#define AMQP_PUBLIC_FUNCTION
	#define AMQP_PUBLIC_VARIABLE extern
#endif
#if __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 1)
	#define AMQP_DEPRECATED(function) function __attribute__((__deprecated__))
#elif defined(_MSC_VER)
	#define AMQP_DEPRECATED(function) __declspec(deprecated) function
#else
	#define AMQP_DEPRECATED(function)
#endif
// 
// Define ssize_t on Win32/64 platforms
// See: http://lists.cs.uiuc.edu/pipermail/llvmdev/2010-April/030649.html for details
// 
#if !defined(_W64)
	#if !defined(__midl) && (defined(_X86_) || defined(_M_IX86)) && _MSC_VER >= 1300
		#define _W64 __w64
	#else
		#define _W64
	#endif
#endif
#ifdef _MSC_VER
	#ifdef _WIN64
		typedef __int64 ssize_t;
	#else
		typedef _W64 int ssize_t;
	#endif
#endif
#if defined(_WIN32) && defined(__MINGW32__)
	#include <sys/types.h>
#endif

/** \endcond */
//
//
//
struct amqp_field_value_t;
struct amqp_table_entry_t;
struct amqp_basic_properties_t;
struct timeval;
//
AMQP_BEGIN_DECLS
/**
 * \def AMQP_VERSION_MAJOR
 *
 * Major library version number compile-time constant
 *
 * The major version is incremented when backwards incompatible API changes
 * are made.
 *
 * \sa AMQP_VERSION, AMQP_VERSION_STRING
 *
 * \since v0.4.0
 */

/**
 * \def AMQP_VERSION_MINOR
 *
 * Minor library version number compile-time constant
 *
 * The minor version is incremented when new APIs are added. Existing APIs
 * are left alone.
 *
 * \sa AMQP_VERSION, AMQP_VERSION_STRING
 *
 * \since v0.4.0
 */

/**
 * \def AMQP_VERSION_PATCH
 *
 * Patch library version number compile-time constant
 *
 * The patch version is incremented when library code changes, but the API
 * is not changed.
 *
 * \sa AMQP_VERSION, AMQP_VERSION_STRING
 *
 * \since v0.4.0
 */

/**
 * \def AMQP_VERSION_IS_RELEASE
 *
 * Version constant set to 1 for tagged release, 0 otherwise
 *
 * NOTE: versions that are not tagged releases are not guaranteed to be API/ABI
 * compatible with older releases, and may change commit-to-commit.
 *
 * \sa AMQP_VERSION, AMQP_VERSION_STRING
 *
 * \since v0.4.0
 */
/*
 * Developer note: when changing these, be sure to update SOVERSION constants
 *  in CMakeLists.txt and configure.ac
 */
#define AMQP_VERSION_MAJOR 0
#define AMQP_VERSION_MINOR 10
#define AMQP_VERSION_PATCH 0
#define AMQP_VERSION_IS_RELEASE 0
/**
 * \def AMQP_VERSION_CODE
 *
 * Helper macro to geneate a packed version code suitable for
 * comparison with AMQP_VERSION.
 *
 * \sa amqp_version_number() AMQP_VERSION_MAJOR, AMQP_VERSION_MINOR,
 *  AMQP_VERSION_PATCH, AMQP_VERSION_IS_RELEASE, AMQP_VERSION
 *
 * \since v0.6.1
 */
#define AMQP_VERSION_CODE(major, minor, patch, release) ((major << 24) | (minor << 16) | (patch << 8) | (release))
/**
 * \def AMQP_VERSION
 *
 * Packed version number
 *
 * AMQP_VERSION is a 4-byte unsigned integer with the most significant byte
 * set to AMQP_VERSION_MAJOR, the second most significant byte set to
 * AMQP_VERSION_MINOR, third most significant byte set to AMQP_VERSION_PATCH,
 * and the lowest byte set to AMQP_VERSION_IS_RELEASE.
 *
 * For example version 2.3.4 which is released version would be encoded as
 * 0x02030401
 *
 * \sa amqp_version_number() AMQP_VERSION_MAJOR, AMQP_VERSION_MINOR,
 *  AMQP_VERSION_PATCH, AMQP_VERSION_IS_RELEASE, AMQP_VERSION_CODE
 *
 * \since v0.4.0
 */
#define AMQP_VERSION AMQP_VERSION_CODE(AMQP_VERSION_MAJOR, AMQP_VERSION_MINOR, AMQP_VERSION_PATCH, AMQP_VERSION_IS_RELEASE)
/** \cond HIDE_FROM_DOXYGEN */
// @v11.4.4 #define AMQ_STRINGIFY(s) AMQ_STRINGIFY_HELPER(s)
// @v11.4.4 #define AMQ_STRINGIFY_HELPER(s) #s // @todo replacewith(STRINGIZE)
#define AMQ_VERSION_STRING STRINGIZE(AMQP_VERSION_MAJOR)  "." STRINGIZE(AMQP_VERSION_MINOR) "." STRINGIZE(AMQP_VERSION_PATCH)
/** \endcond */
/**
 * \def AMQP_VERSION_STRING
 *
 * Version string compile-time constant
 *
 * Non-released versions of the library will have "-pre" appended to the
 * version string
 *
 * \sa amqp_version()
 *
 * \since v0.4.0
 */
#if AMQP_VERSION_IS_RELEASE
	#define AMQP_VERSION_STRING AMQ_VERSION_STRING
#else
	#define AMQP_VERSION_STRING AMQ_VERSION_STRING "-pre"
#endif
// 
// Descr: Returns the rabbitmq-c version as a packed integer.
// See \ref AMQP_VERSION
// 
// \return packed 32-bit integer representing version of library at runtime
// \sa AMQP_VERSION, amqp_version()
// 
AMQP_PUBLIC_FUNCTION uint32 amqp_version_number();
// 
// Descr: Returns the rabbitmq-c version as a string.
// See \ref AMQP_VERSION_STRING
// \return a statically allocated string describing the version of rabbitmq-c.
// \sa amqp_version_number(), AMQP_VERSION_STRING, AMQP_VERSION
// 
AMQP_PUBLIC_FUNCTION char const * amqp_version();

#define AMQP_DEFAULT_FRAME_SIZE SKILOBYTE(128) // Default frame size (128Kb)
#define AMQP_DEFAULT_MAX_CHANNELS 2047 // Default maximum number of channels (2047, RabbitMQ default limit of 2048, minus 1 for channel 0).
	// RabbitMQ set a default limit of 2048 channels per connection in v3.7.5 to prevent broken clients from leaking too many channels.
#define AMQP_DEFAULT_HEARTBEAT 0 // Default heartbeat interval (0, heartbeat disabled)
#define AMQP_DEFAULT_VHOST "/" // Default RabbitMQ vhost

typedef int    amqp_boolean_t_Unused; // boolean type 0 = false, true otherwise // @sobolev replaced with boolint
typedef uint32 amqp_method_number_t; // Method number
typedef uint32 amqp_flags_t; // Bitmask for flags
typedef uint16 amqp_channel_t; // Channel type
// 
// Descr: Buffer descriptor
//
struct amqp_bytes_t {
	size_t len;   // length of the buffer in bytes 
	void * bytes; // pointer to the beginning of the buffer 
};
// 
// Descr: Decimal data type
// 
struct amqp_decimal_t {
	uint8  decimals; // the location of the decimal point 
	uint32 value;    // the value before the decimal point is applied 
};
// 
// Descr: AMQP field table
// An AMQP field table is a set of key-value pairs.
// A key is a UTF-8 encoded string up to 128 bytes long, and are not null terminated.
// A value can be one of several different datatypes. \sa amqp_field_value_kind_t
// \sa amqp_table_entry_t
// 
struct amqp_table_t {
	int    num_entries; // length of entries array 
	amqp_table_entry_t * entries; // an array of table entries 
};
// 
// Descr: An AMQP Field Array
// A repeated set of field values, all must be of the same type
// 
struct amqp_array_t {
	int    num_entries; // Number of entries in the table 
	amqp_field_value_t * entries; // linked list of field values
};
/*
  0-9   0-9-1   Qpid/Rabbit  Type               Remarks
---------------------------------------------------------------------------
        t       t            Boolean
        b       b            Signed 8-bit
        B                    Unsigned 8-bit
        U       s            Signed 16-bit      (A1)
        u                    Unsigned 16-bit
  I     I       I            Signed 32-bit
        i                    Unsigned 32-bit
        L       l            Signed 64-bit      (B)
        l                    Unsigned 64-bit
        f       f            32-bit float
        d       d            64-bit float
  D     D       D            Decimal
        s                    Short string       (A2)
  S     S       S            Long string
        A                    Nested Array
  T     T       T            Timestamp (u64)
  F     F       F            Nested Table
  V     V       V            Void
                x            Byte array

Remarks:

 A1, A2: Notice how the types **CONFLICT** here. In Qpid and Rabbit,
   's' means a signed 16-bit integer; in 0-9-1, it means a short string.

 B: Notice how the signednesses **CONFLICT** here. In Qpid and Rabbit,
   'l' means a signed 64-bit integer; in 0-9-1, it means an unsigned 64-bit integer.

I'm going with the Qpid/Rabbit types, where there's a conflict, and
the 0-9-1 types otherwise. 0-8 is a subset of 0-9, which is a subset
of the other two, so this will work for both 0-8 and 0-9-1 branches of the code.
*/
// 
// Descr: A field table value
// 
struct amqp_field_value_t {
	uint8 kind; /**< the type of the entry /sa amqp_field_value_kind_t */
	union V {
		boolint boolean; /**< boolean type AMQP_FIELD_KIND_BOOLEAN */
		int8   i8; /**< int8 type AMQP_FIELD_KIND_I8 */
		uint8  u8; /**< uint8 type AMQP_FIELD_KIND_U8 */
		int16  i16; /**< int16 type AMQP_FIELD_KIND_I16 */
		uint16 u16; /**< uint16 type AMQP_FIELD_KIND_U16 */
		int32  i32; /**< int32 type AMQP_FIELD_KIND_I32 */
		uint32 u32; /**< uint32 type AMQP_FIELD_KIND_U32 */
		int64  i64; /**< int64 type AMQP_FIELD_KIND_I64 */
		uint64 u64; /**< uint64 type AMQP_FIELD_KIND_U64, AMQP_FIELD_KIND_TIMESTAMP */
		float  f32; /**< float type AMQP_FIELD_KIND_F32 */
		double f64; /**< double type AMQP_FIELD_KIND_F64 */
		amqp_decimal_t decimal; /**< amqp_decimal_t AMQP_FIELD_KIND_DECIMAL */
		amqp_bytes_t bytes; /**< amqp_bytes_t type AMQP_FIELD_KIND_UTF8, AMQP_FIELD_KIND_BYTES */
		amqp_table_t table; /**< amqp_table_t type AMQP_FIELD_KIND_TABLE */
		amqp_array_t array; /**< amqp_array_t type AMQP_FIELD_KIND_ARRAY */
	} value; /**< a union of the value */
};
/**
 * An entry in a field-table
 *
 * \sa amqp_table_encode(), amqp_table_decode(), amqp_table_clone()
 *
 * \since v0.1
 */
struct amqp_table_entry_t {
	amqp_table_entry_t()
	{
		THISZERO();
	}
	amqp_bytes_t key; // the table entry key. Its a null-terminated UTF-8 string, with a maximum size of 128 bytes 
	amqp_field_value_t value; // the table entry values 
};
/**
 * Field value types
 *
 * \since v0.1
 */
enum amqp_field_value_kind_t {
	AMQP_FIELD_KIND_BOOLEAN = 't', // boolean type. 0 = false, 1 = true @see boolint 
	AMQP_FIELD_KIND_I8      = 'b', // 8-bit signed integer, datatype: int8 
	AMQP_FIELD_KIND_U8      = 'B', // 8-bit unsigned integer, datatype: uint8 
	AMQP_FIELD_KIND_I16     = 's', // 16-bit signed integer, datatype: int16 
	AMQP_FIELD_KIND_U16     = 'u', // 16-bit unsigned integer, datatype: uint16
	AMQP_FIELD_KIND_I32     = 'I', // 32-bit signed integer, datatype: int32
	AMQP_FIELD_KIND_U32     = 'i', // 32-bit unsigned integer, datatype: uint32 
	AMQP_FIELD_KIND_I64     = 'l', // 64-bit signed integer, datatype: int64 
	AMQP_FIELD_KIND_U64     = 'L', // 64-bit unsigned integer, datatype: uint64 
	AMQP_FIELD_KIND_F32     = 'f', // single-precision floating point value, datatype: float 
	AMQP_FIELD_KIND_F64     = 'd', // double-precision floating point value, datatype: double 
	AMQP_FIELD_KIND_DECIMAL = 'D', // amqp-decimal value, datatype: amqp_decimal_t
	AMQP_FIELD_KIND_UTF8    = 'S', // UTF-8 null-terminated character string, datatype: amqp_bytes_t
	AMQP_FIELD_KIND_ARRAY   = 'A', // field array (repeated values of another datatype. datatype: amqp_array_t 
	AMQP_FIELD_KIND_TIMESTAMP = 'T', // 64-bit timestamp. datatype uint64 
	AMQP_FIELD_KIND_TABLE   = 'F', // field table. encapsulates a table inside a table entry. datatype: amqp_table_t 
	AMQP_FIELD_KIND_VOID    = 'V', // empty entry 
	AMQP_FIELD_KIND_BYTES   = 'x'  // unformatted byte string, datatype: amqp_bytes_t 
};
// 
// Descr: A list of allocation blocks
// 
struct amqp_pool_blocklist_t {
	int    num_blocks; // Number of blocks in the block list 
	void ** blocklist; // Array of memory blocks 
};
// 
// Descr: A memory pool
// 
struct amqp_pool_t {
	size_t pagesize; // the size of the page in bytes. Allocations less than or equal to this size are allocated in the pages block list.
		// Allocations greater than this are allocated in their own own block in the large_blocks block list 
	amqp_pool_blocklist_t pages; // blocks that are the size of pagesize 
	amqp_pool_blocklist_t large_blocks; // allocations larger than the pagesize 
	int    next_page;   // an index to the next unused page block 
	char * alloc_block; // pointer to the current allocation block 
	size_t alloc_used;  // number of bytes in the current allocation block that has been used 
};
// 
// Descr: An amqp method
// 
struct amqp_method_t {
	amqp_method_number_t id; // the method id number 
	void * decoded; // pointer to the decoded method, cast to the appropriate type to use 
};
// 
// Descr: An AMQP frame
// 
struct amqp_frame_t {
	amqp_frame_t() : frame_type(0), channel(0)
	{
		MEMSZERO(payload);
	}
	uint8 frame_type; // frame type. The types: AMQP_FRAME_METHOD - use the method union member; AMQP_FRAME_HEADER - use the properties union member;
		// AMQP_FRAME_BODY - use the body_fragment union member
	amqp_channel_t channel; /**< the channel the frame was received on */
	union {
		amqp_method_t method; // a method, use if frame_type == AMQP_FRAME_METHOD 
		struct {
			uint16 class_id;  // the class for the properties 
			uint64 body_size; // size of the body in bytes 
			void * decoded;   // the decoded properties 
			amqp_bytes_t raw; // amqp-encoded properties structure 
		} properties; // message header, a.k.a., properties, use if frame_type == AMQP_FRAME_HEADER 
		amqp_bytes_t body_fragment; // a body fragment, use if frame_type == AMQP_FRAME_BODY 
		struct {
			uint8 transport_high; /**< @internal first byte of handshake */
			uint8 transport_low; /**< @internal second byte of handshake */
			uint8 protocol_version_major; /**< @internal third byte of handshake */
			uint8 protocol_version_minor; /**< @internal fourth byte of handshake */
		} protocol_header; // Used only when doing the initial handshake with the broker, don't use otherwise 
	} payload; // the payload of the frame 
};
// 
// Descr: Response type
// 
enum amqp_response_type_enum {
	AMQP_RESPONSE_NONE = 0,          // the library got an EOF from the socket 
	AMQP_RESPONSE_NORMAL,            // response normal, the RPC completed successfully 
	AMQP_RESPONSE_LIBRARY_EXCEPTION, // library error, an error occurred in the library, examine the library_error 
	AMQP_RESPONSE_SERVER_EXCEPTION   // server exception, the broker returned an error, check replay 
};
// 
// Descr: Reply from a RPC method on the broker
// 
struct amqp_rpc_reply_t {
	amqp_rpc_reply_t() : reply_type(AMQP_RESPONSE_NONE), library_error(0)
	{
		reply.id = 0;
		reply.decoded = 0;
	}
	amqp_response_type_enum reply_type; // the reply type: AMQP_RESPONSE_NORMAL - the RPC completed successfully; 
		// AMQP_RESPONSE_SERVER_EXCEPTION - the broker returned an exception, check the reply field
		// AMQP_RESPONSE_LIBRARY_EXCEPTION - the library encountered an error, check the library_error field
	amqp_method_t reply; // in case of AMQP_RESPONSE_SERVER_EXCEPTION this field will be set to the method returned from the broker 
	int library_error; // in case of AMQP_RESPONSE_LIBRARY_EXCEPTION this field will be set to an error code. An error string can be retrieved using amqp_error_string 
};
/**
 * SASL method type
 *
 * \since v0.1
 */
typedef enum amqp_sasl_method_enum_ {
	AMQP_SASL_METHOD_UNDEFINED = -1, // Invalid SASL method 
	AMQP_SASL_METHOD_PLAIN     =  0, // the PLAIN SASL method for authentication to the broker 
	AMQP_SASL_METHOD_EXTERNAL  =  1  // the EXTERNAL SASL method for authentication to the broker 
} amqp_sasl_method_enum;
typedef struct amqp_connection_state_t_ * amqp_connection_state_t; // Connection state object
typedef struct amqp_socket_t_ amqp_socket_t; // Socket object
// 
// Descr: Status codes
// NOTE: When updating this enum, update the strings in librabbitmq/amqp_api.c
// 
typedef enum amqp_status_enum_ {
	AMQP_STATUS_OK                     =  0x0000, // Operation successful 
	AMQP_STATUS_NO_MEMORY              = -0x0001, // Memory allocation failed 
	AMQP_STATUS_BAD_AMQP_DATA          = -0x0002, // Incorrect or corrupt data was received from the broker. This is a protocol error. 
	AMQP_STATUS_UNKNOWN_CLASS          = -0x0003, // An unknown AMQP class was received. This is a protocol error. 
	AMQP_STATUS_UNKNOWN_METHOD         = -0x0004, // An unknown AMQP method was received. This is a protocol error. 
	AMQP_STATUS_HOSTNAME_RESOLUTION_FAILED = -0x0005, // Unable to resolve the hostname 
	AMQP_STATUS_INCOMPATIBLE_AMQP_VERSION  = -0x0006, // The broker advertised an incompaible AMQP version 
	AMQP_STATUS_CONNECTION_CLOSED      = -0x0007, // The connection to the broker has been closed
	AMQP_STATUS_BAD_URL                = -0x0008, // malformed AMQP URL 
	AMQP_STATUS_SOCKET_ERROR           = -0x0009, // A socket error occurred 
	AMQP_STATUS_INVALID_PARAMETER      = -0x000A, // An invalid parameter was passed into the function 
	AMQP_STATUS_TABLE_TOO_BIG          = -0x000B, // The amqp_table_t object cannot be serialized because the output buffer is too small 
	AMQP_STATUS_WRONG_METHOD           = -0x000C, // The wrong method was received 
	AMQP_STATUS_TIMEOUT                = -0x000D, // Operation timed out 
	AMQP_STATUS_TIMER_FAILURE          = -0x000E, // The underlying system timer facility failed 
	AMQP_STATUS_HEARTBEAT_TIMEOUT      = -0x000F, // Timed out waiting for heartbeat 
	AMQP_STATUS_UNEXPECTED_STATE       = -0x0010, // Unexpected protocol state 
	AMQP_STATUS_SOCKET_CLOSED          = -0x0011, // Underlying socket is closed 
	AMQP_STATUS_SOCKET_INUSE           = -0x0012, // Underlying socket is already open 
	AMQP_STATUS_BROKER_UNSUPPORTED_SASL_METHOD = -0x0013, // Broker does not support the requested SASL mechanism 
	AMQP_STATUS_UNSUPPORTED            = -0x0014, // Parameter is unsupported in this version 
	_AMQP_STATUS_NEXT_VALUE            = -0x0015, // Internal value 
	AMQP_STATUS_TCP_ERROR              = -0x0100, // A generic TCP error occurred 
	AMQP_STATUS_TCP_SOCKETLIB_INIT_ERROR       = -0x0101, // An error occurred trying to initialize the socket library
	_AMQP_STATUS_TCP_NEXT_VALUE        = -0x0102, // Internal value 
	AMQP_STATUS_SSL_ERROR              = -0x0200, // A generic SSL error occurred. 
	AMQP_STATUS_SSL_HOSTNAME_VERIFY_FAILED     = -0x0201, // SSL validation of hostname against peer certificate failed 
	AMQP_STATUS_SSL_PEER_VERIFY_FAILED = -0x0202, // SSL validation of peer certificate failed. 
	AMQP_STATUS_SSL_CONNECTION_FAILED  = -0x0203, // SSL handshake failed. 
	_AMQP_STATUS_SSL_NEXT_VALUE        = -0x0204  // Internal value 
} amqp_status_enum;
// 
// Descr: AMQP delivery modes.
// Use these values for the #amqp_basic_properties_t::delivery_mode field.
// 
enum amqp_delivery_mode_enum {
	AMQP_DELIVERY_NONPERSISTENT = 1, /**< Non-persistent message */
	AMQP_DELIVERY_PERSISTENT = 2     /**< Persistent message */
};

//AMQP_END_DECLS
//#include <amqp_framing.h>
// amqp_framing.h {
//AMQP_BEGIN_DECLS
	#define AMQP_PROTOCOL_VERSION_MAJOR    0 /**< AMQP protocol version major */
	#define AMQP_PROTOCOL_VERSION_MINOR    9 /**< AMQP protocol version minor */
	#define AMQP_PROTOCOL_VERSION_REVISION 1 /**< AMQP protocol version revision */
	#define AMQP_PROTOCOL_PORT 5672      /**< Default AMQP Port */
	#define AMQP_FRAME_METHOD 1          /**< Constant: FRAME-METHOD */
	#define AMQP_FRAME_HEADER 2          /**< Constant: FRAME-HEADER */
	#define AMQP_FRAME_BODY 3            /**< Constant: FRAME-BODY */
	#define AMQP_FRAME_HEARTBEAT 8       /**< Constant: FRAME-HEARTBEAT */
	#define AMQP_FRAME_MIN_SIZE 4096     /**< Constant: FRAME-MIN-SIZE */
	#define AMQP_FRAME_END 206           /**< Constant: FRAME-END */
	#define AMQP_REPLY_SUCCESS 200       /**< Constant: REPLY-SUCCESS */
	#define AMQP_CONTENT_TOO_LARGE 311   /**< Constant: CONTENT-TOO-LARGE */
	#define AMQP_NO_ROUTE 312            /**< Constant: NO-ROUTE */
	#define AMQP_NO_CONSUMERS 313        /**< Constant: NO-CONSUMERS */
	#define AMQP_ACCESS_REFUSED 403      /**< Constant: ACCESS-REFUSED */
	#define AMQP_NOT_FOUND 404           /**< Constant: NOT-FOUND */
	#define AMQP_RESOURCE_LOCKED 405     /**< Constant: RESOURCE-LOCKED */
	#define AMQP_PRECONDITION_FAILED 406 /**< Constant: PRECONDITION-FAILED */
	#define AMQP_CONNECTION_FORCED 320   /**< Constant: CONNECTION-FORCED */
	#define AMQP_INVALID_PATH 402        /**< Constant: INVALID-PATH */
	#define AMQP_FRAME_ERROR 501         /**< Constant: FRAME-ERROR */
	#define AMQP_SYNTAX_ERROR 502        /**< Constant: SYNTAX-ERROR */
	#define AMQP_COMMAND_INVALID 503     /**< Constant: COMMAND-INVALID */
	#define AMQP_CHANNEL_ERROR 504       /**< Constant: CHANNEL-ERROR */
	#define AMQP_UNEXPECTED_FRAME 505    /**< Constant: UNEXPECTED-FRAME */
	#define AMQP_RESOURCE_ERROR 506      /**< Constant: RESOURCE-ERROR */
	#define AMQP_NOT_ALLOWED 530         /**< Constant: NOT-ALLOWED */
	#define AMQP_NOT_IMPLEMENTED 540     /**< Constant: NOT-IMPLEMENTED */
	#define AMQP_INTERNAL_ERROR 541      /**< Constant: INTERNAL-ERROR */

	/* Function prototypes. */

	/**
	 * Get constant name string from constant
	 *
	 * @param [in] constantNumber constant to get the name of
	 * @returns string describing the constant. String is managed by
	 *  the library and should not be SAlloc::F()'d by the program
	 */
	AMQP_PUBLIC_FUNCTION char const * amqp_constant_name(int constantNumber);
	/**
	 * Checks to see if a constant is a hard error
	 *
	 * A hard error occurs when something severe enough
	 * happens that the connection must be closed.
	 *
	 * @param [in] constantNumber the error constant
	 * @returns true if its a hard error, false otherwise
	 */
	AMQP_PUBLIC_FUNCTION boolint amqp_constant_is_hard_error(int constantNumber);
	/**
	 * Get method name string from method number
	 *
	 * @param [in] methodNumber the method number
	 * @returns method name string. String is managed by the library
	 *  and should not be freed()'d by the program
	 */
	AMQP_PUBLIC_FUNCTION char const * amqp_method_name(amqp_method_number_t methodNumber);
	/**
	 * Check whether a method has content
	 *
	 * A method that has content will receive the method frame
	 * a properties frame, then 1 to N body frames
	 *
	 * @param [in] methodNumber the method number
	 * @returns true if method has content, false otherwise
	 */
	AMQP_PUBLIC_FUNCTION boolint amqp_method_has_content(amqp_method_number_t methodNumber);
	/**
	 * Decodes a method from AMQP wireformat
	 *
	 * @param [in] methodNumber the method number for the decoded parameter
	 * @param [in] pool the memory pool to allocate the decoded method from
	 * @param [in] encoded the encoded byte string buffer
	 * @param [out] decoded pointer to the decoded method struct
	 * @returns 0 on success, an error code otherwise
	 */
	AMQP_PUBLIC_FUNCTION int amqp_decode_method(amqp_method_number_t methodNumber, amqp_pool_t *pool, amqp_bytes_t encoded, void **decoded);
	/**
	 * Decodes a header frame properties structure from AMQP wireformat
	 *
	 * @param [in] class_id the class id for the decoded parameter
	 * @param [in] pool the memory pool to allocate the decoded properties from
	 * @param [in] encoded the encoded byte string buffer
	 * @param [out] decoded pointer to the decoded properties struct
	 * @returns 0 on success, an error code otherwise
	 */
	AMQP_PUBLIC_FUNCTION int amqp_decode_properties(uint16 class_id, amqp_pool_t *pool, amqp_bytes_t encoded, void **decoded);
	/**
	 * Encodes a method structure in AMQP wireformat
	 *
	 * @param [in] methodNumber the method number for the decoded parameter
	 * @param [in] decoded the method structure (e.g., amqp_connection_start_t)
	 * @param [in] encoded an allocated byte buffer for the encoded method
	 *     structure to be written to. If the buffer isn't large enough
	 *     to hold the encoded method, an error code will be returned.
	 * @returns 0 on success, an error code otherwise.
	 */
	AMQP_PUBLIC_FUNCTION int amqp_encode_method(amqp_method_number_t methodNumber, const void * decoded, amqp_bytes_t encoded);
	/**
	 * Encodes a properties structure in AMQP wireformat
	 *
	 * @param [in] class_id the class id for the decoded parameter
	 * @param [in] decoded the properties structure (e.g., amqp_basic_properties_t)
	 * @param [in] encoded an allocated byte buffer for the encoded properties to
	 * written to.
	 *     If the buffer isn't large enough to hold the encoded method, an
	 *     an error code will be returned
	 * @returns 0 on success, an error code otherwise.
	 */
	AMQP_PUBLIC_FUNCTION int amqp_encode_properties(uint16 class_id, void *decoded, amqp_bytes_t encoded);
	/* Method field records. */

	#define AMQP_CONNECTION_START_METHOD ((amqp_method_number_t)0x000A000A) /**< connection.start method id @internal 10, 10; 655370 */
	/** connection.start method fields */
	typedef struct amqp_connection_start_t_ {
		uint8 version_major; /**< version-major */
		uint8 version_minor; /**< version-minor */
		amqp_table_t server_properties; /**< server-properties */
		amqp_bytes_t mechanisms; /**< mechanisms */
		amqp_bytes_t locales; /**< locales */
	} amqp_connection_start_t;

	#define AMQP_CONNECTION_START_OK_METHOD ((amqp_method_number_t)0x000A000B) /**< connection.start-ok method id @internal 10, 11; 655371 */
	//
	// Descr: connection.start-ok method fields 
	//
	struct amqp_connection_start_ok_t {
		amqp_table_t client_properties; /**< client-properties */
		amqp_bytes_t mechanism; /**< mechanism */
		amqp_bytes_t response; /**< response */
		amqp_bytes_t locale; /**< locale */
	};

	#define AMQP_CONNECTION_SECURE_METHOD ((amqp_method_number_t)0x000A0014) /**< connection.secure method id @internal 10, 20; 655380 */
	#define AMQP_CONNECTION_SECURE_OK_METHOD ((amqp_method_number_t)0x000A0015) /**< connection.secure-ok method id @internal 10, 21; 655381 */
	#define AMQP_CONNECTION_TUNE_METHOD ((amqp_method_number_t)0x000A001E) /**< connection.tune method id @internal 10, 30; 655390 */
	#define AMQP_CONNECTION_TUNE_OK_METHOD ((amqp_method_number_t)0x000A001F) /**< connection.tune-ok method id @internal 10, 31; 655391 */
	#define AMQP_CONNECTION_OPEN_METHOD ((amqp_method_number_t)0x000A0028) /**< connection.open method id @internal 10, 40; 655400 */
	#define AMQP_CONNECTION_OPEN_OK_METHOD ((amqp_method_number_t)0x000A0029) /**< connection.open-ok method id @internal 10, 41; 655401 */
	#define AMQP_CONNECTION_CLOSE_METHOD ((amqp_method_number_t)0x000A0032) /**< connection.close method id @internal 10, 50; 655410 */
	#define AMQP_CONNECTION_CLOSE_OK_METHOD ((amqp_method_number_t)0x000A0033) /**< connection.close-ok method id @internal 10, 51; 655411 */
	#define AMQP_CONNECTION_BLOCKED_METHOD ((amqp_method_number_t)0x000A003C) /**< connection.blocked method id @internal 10, 60; 655420 */
	#define AMQP_CONNECTION_UNBLOCKED_METHOD ((amqp_method_number_t)0x000A003D) /**< connection.unblocked method id @internal 10, 61; 655421 */
	#define AMQP_CHANNEL_OPEN_METHOD ((amqp_method_number_t)0x0014000A) /**< channel.open method id @internal 20, 10; 1310730 */
	#define AMQP_CHANNEL_OPEN_OK_METHOD ((amqp_method_number_t)0x0014000B) /**< channel.open-ok method id @internal 20, 11; 1310731 */
	#define AMQP_CHANNEL_FLOW_METHOD ((amqp_method_number_t)0x00140014) /**< channel.flow method id @internal 20, 20; 1310740 */
	#define AMQP_CHANNEL_FLOW_OK_METHOD ((amqp_method_number_t)0x00140015) /**< channel.flow-ok method id @internal 20, 21; 1310741 */
	#define AMQP_CHANNEL_CLOSE_METHOD ((amqp_method_number_t)0x00140028) /**< channel.close method id @internal 20, 40; 1310760 */
	#define AMQP_CHANNEL_CLOSE_OK_METHOD ((amqp_method_number_t)0x00140029) /**< channel.close-ok method id @internal 20, 41; 1310761 */
	#define AMQP_ACCESS_REQUEST_METHOD ((amqp_method_number_t)0x001E000A) /**< access.request method id @internal 30, 10; 1966090 */
	#define AMQP_ACCESS_REQUEST_OK_METHOD ((amqp_method_number_t)0x001E000B) /**< access.request-ok method id @internal 30, 11; 1966091 */
	
	struct amqp_connection_secure_t { // connection.secure method fields 
		amqp_bytes_t challenge; /**< challenge */
	};

	struct amqp_connection_secure_ok_t { // connection.secure-ok method fields 
		amqp_bytes_t response; /**< response */
	};
	
	struct amqp_connection_tune_t { // connection.tune method fields 
		uint16 channel_max; /**< channel-max */
		uint32 frame_max; /**< frame-max */
		uint16 heartbeat; /**< heartbeat */
	};
	
	struct amqp_connection_tune_ok_t { // connection.tune-ok method fields 
		uint16 channel_max; /**< channel-max */
		uint32 frame_max; /**< frame-max */
		uint16 heartbeat; /**< heartbeat */
	};
	
	struct amqp_connection_open_t { // connection.open method fields 
		amqp_bytes_t virtual_host; /**< virtual-host */
		amqp_bytes_t capabilities; /**< capabilities */
		boolint insist; /**< insist */
	};
	
	/** connection.open-ok method fields */
	typedef struct amqp_connection_open_ok_t_ {
		amqp_bytes_t known_hosts; /**< known-hosts */
	} amqp_connection_open_ok_t;

	/** connection.close method fields */
	typedef struct amqp_connection_close_t_ {
		uint16 reply_code; /**< reply-code */
		amqp_bytes_t reply_text; /**< reply-text */
		uint16 class_id; /**< class-id */
		uint16 method_id; /**< method-id */
	} amqp_connection_close_t;

	/** connection.close-ok method fields */
	typedef struct amqp_connection_close_ok_t_ {
		char   dummy; // Dummy field to avoid empty struct
	} amqp_connection_close_ok_t;

	/** connection.blocked method fields */
	typedef struct amqp_connection_blocked_t_ {
		amqp_bytes_t reason; /**< reason */
	} amqp_connection_blocked_t;

	/** connection.unblocked method fields */
	typedef struct amqp_connection_unblocked_t_ {
		char   dummy; // Dummy field to avoid empty struct
	} amqp_connection_unblocked_t;
	
	/** channel.open method fields */
	typedef struct amqp_channel_open_t_ {
		amqp_bytes_t out_of_band; /**< out-of-band */
	} amqp_channel_open_t;

	/** channel.open-ok method fields */
	typedef struct amqp_channel_open_ok_t_ {
		amqp_bytes_t channel_id; /**< channel-id */
	} amqp_channel_open_ok_t;

	/** channel.flow method fields */
	typedef struct amqp_channel_flow_t_ {
		boolint active; /**< active */
	} amqp_channel_flow_t;

	/** channel.flow-ok method fields */
	typedef struct amqp_channel_flow_ok_t_ {
		boolint active; /**< active */
	} amqp_channel_flow_ok_t;
	
	/** channel.close method fields */
	typedef struct amqp_channel_close_t_ {
		uint16 reply_code; /**< reply-code */
		amqp_bytes_t reply_text; /**< reply-text */
		uint16 class_id; /**< class-id */
		uint16 method_id; /**< method-id */
	} amqp_channel_close_t;

	/** channel.close-ok method fields */
	typedef struct amqp_channel_close_ok_t_ {
		char   dummy; // Dummy field to avoid empty struct
	} amqp_channel_close_ok_t;

	/** access.request method fields */
	typedef struct amqp_access_request_t_ {
		amqp_bytes_t realm; /**< realm */
		boolint exclusive; /**< exclusive */
		boolint passive; /**< passive */
		boolint active; /**< active */
		boolint write; /**< write */
		boolint read; /**< read */
	} amqp_access_request_t;

	/** access.request-ok method fields */
	typedef struct amqp_access_request_ok_t_ {
		uint16 ticket; /**< ticket */
	} amqp_access_request_ok_t;

	#define AMQP_EXCHANGE_DECLARE_METHOD ((amqp_method_number_t)0x0028000A) /**< exchange.declare method id @internal 40, 10; 2621450 */
	/** exchange.declare method fields */
	typedef struct amqp_exchange_declare_t_ {
		uint16 ticket; /**< ticket */
		amqp_bytes_t exchange; /**< exchange */
		amqp_bytes_t type; /**< type */
		boolint passive; /**< passive */
		boolint durable; /**< durable */
		boolint auto_delete; /**< auto-delete */
		boolint internal; /**< internal */
		boolint nowait; /**< nowait */
		amqp_table_t arguments; /**< arguments */
	} amqp_exchange_declare_t;

	#define AMQP_EXCHANGE_DECLARE_OK_METHOD ((amqp_method_number_t)0x0028000B) /**< exchange.declare-ok method id @internal 40, 11; 2621451 */
	/** exchange.declare-ok method fields */
	typedef struct amqp_exchange_declare_ok_t_ {
		char   dummy; // Dummy field to avoid empty struct
	} amqp_exchange_declare_ok_t;

	#define AMQP_EXCHANGE_DELETE_METHOD ((amqp_method_number_t)0x00280014) /**< exchange.delete method id @internal 40, 20; 2621460 */
	/** exchange.delete method fields */
	typedef struct amqp_exchange_delete_t_ {
		uint16 ticket; /**< ticket */
		amqp_bytes_t exchange; /**< exchange */
		boolint if_unused; /**< if-unused */
		boolint nowait; /**< nowait */
	} amqp_exchange_delete_t;

	#define AMQP_EXCHANGE_DELETE_OK_METHOD ((amqp_method_number_t)0x00280015) /**< exchange.delete-ok method id @internal 40, 21; 2621461 */
	/** exchange.delete-ok method fields */
	typedef struct amqp_exchange_delete_ok_t_ {
		char   dummy; // Dummy field to avoid empty struct
	} amqp_exchange_delete_ok_t;

	#define AMQP_EXCHANGE_BIND_METHOD ((amqp_method_number_t)0x0028001E) /**< exchange.bind method id @internal 40, 30; 2621470 */
	//
	// Descr: exchange.bind method fields 
	//
	struct amqp_exchange_bind_t {
		uint16 ticket; /**< ticket */
		amqp_bytes_t destination; /**< destination */
		amqp_bytes_t source; /**< source */
		amqp_bytes_t routing_key; /**< routing-key */
		boolint nowait; /**< nowait */
		amqp_table_t arguments; /**< arguments */
	};

	#define AMQP_EXCHANGE_BIND_OK_METHOD ((amqp_method_number_t)0x0028001F) /**< exchange.bind-ok method id @internal 40, 31; 2621471 */
	/** exchange.bind-ok method fields */
	typedef struct amqp_exchange_bind_ok_t_ {
		char   dummy; // Dummy field to avoid empty struct
	} amqp_exchange_bind_ok_t;

	#define AMQP_EXCHANGE_UNBIND_METHOD ((amqp_method_number_t)0x00280028) /**< exchange.unbind method id @internal 40, 40; 2621480 */
	//
	// Descr: exchange.unbind method fields 
	//
	struct amqp_exchange_unbind_t {
		uint16 ticket; /**< ticket */
		amqp_bytes_t destination; /**< destination */
		amqp_bytes_t source; /**< source */
		amqp_bytes_t routing_key; /**< routing-key */
		boolint nowait; /**< nowait */
		amqp_table_t arguments; /**< arguments */
	};

	#define AMQP_EXCHANGE_UNBIND_OK_METHOD ((amqp_method_number_t)0x00280033) /**< exchange.unbind-ok method id @internal 40, 51; 2621491 */
	/** exchange.unbind-ok method fields */
	typedef struct amqp_exchange_unbind_ok_t_ {
		char   dummy; // Dummy field to avoid empty struct
	} amqp_exchange_unbind_ok_t;

	#define AMQP_QUEUE_DECLARE_METHOD ((amqp_method_number_t)0x0032000A) /**< queue.declare method id @internal 50, 10; 3276810 */
	/** queue.declare method fields */
	typedef struct amqp_queue_declare_t_ {
		uint16 ticket; /**< ticket */
		amqp_bytes_t queue; /**< queue */
		boolint passive; /**< passive */
		boolint durable; /**< durable */
		boolint exclusive; /**< exclusive */
		boolint auto_delete; /**< auto-delete */
		boolint nowait; /**< nowait */
		amqp_table_t arguments; /**< arguments */
	} amqp_queue_declare_t;

	#define AMQP_QUEUE_DECLARE_OK_METHOD ((amqp_method_number_t)0x0032000B) /**< queue.declare-ok method id @internal 50, 11; 3276811 */
	//
	// Descr: queue.declare-ok method fields 
	//
	struct amqp_queue_declare_ok_t {
		amqp_bytes_t queue; /**< queue */
		uint32 message_count; /**< message-count */
		uint32 consumer_count; /**< consumer-count */
	};

	#define AMQP_QUEUE_BIND_METHOD ((amqp_method_number_t)0x00320014) /**< queue.bind method id @internal 50, 20; 3276820 */
	/** queue.bind method fields */
	typedef struct amqp_queue_bind_t_ {
		uint16 ticket; /**< ticket */
		amqp_bytes_t queue; /**< queue */
		amqp_bytes_t exchange; /**< exchange */
		amqp_bytes_t routing_key; /**< routing-key */
		boolint nowait; /**< nowait */
		amqp_table_t arguments; /**< arguments */
	} amqp_queue_bind_t;

	#define AMQP_QUEUE_BIND_OK_METHOD ((amqp_method_number_t)0x00320015) /**< queue.bind-ok method id @internal 50, 21; 3276821 */
	/** queue.bind-ok method fields */
	typedef struct amqp_queue_bind_ok_t_ {
		char   dummy; // Dummy field to avoid empty struct
	} amqp_queue_bind_ok_t;

	#define AMQP_QUEUE_PURGE_METHOD ((amqp_method_number_t)0x0032001E) /**< queue.purge method id @internal 50, 30; 3276830 */
	/** queue.purge method fields */
	typedef struct amqp_queue_purge_t_ {
		uint16 ticket; /**< ticket */
		amqp_bytes_t queue; /**< queue */
		boolint nowait; /**< nowait */
	} amqp_queue_purge_t;

	#define AMQP_QUEUE_PURGE_OK_METHOD ((amqp_method_number_t)0x0032001F) /**< queue.purge-ok method id @internal 50, 31; 3276831 */
	/** queue.purge-ok method fields */
	typedef struct amqp_queue_purge_ok_t_ {
		uint32 message_count; /**< message-count */
	} amqp_queue_purge_ok_t;

	#define AMQP_QUEUE_DELETE_METHOD ((amqp_method_number_t)0x00320028) /**< queue.delete method id @internal 50, 40; 3276840 */
	/** queue.delete method fields */
	typedef struct amqp_queue_delete_t_ {
		uint16 ticket; /**< ticket */
		amqp_bytes_t queue; /**< queue */
		boolint if_unused; /**< if-unused */
		boolint if_empty; /**< if-empty */
		boolint nowait; /**< nowait */
	} amqp_queue_delete_t;

	#define AMQP_QUEUE_DELETE_OK_METHOD ((amqp_method_number_t)0x00320029) /**< queue.delete-ok method id @internal 50, 41; 3276841 */
	/** queue.delete-ok method fields */
	typedef struct amqp_queue_delete_ok_t_ {
		uint32 message_count; /**< message-count */
	} amqp_queue_delete_ok_t;

	#define AMQP_QUEUE_UNBIND_METHOD ((amqp_method_number_t)0x00320032) /**< queue.unbind method id @internal 50, 50; 3276850 */
	/** queue.unbind method fields */
	typedef struct amqp_queue_unbind_t_ {
		uint16 ticket; /**< ticket */
		amqp_bytes_t queue; /**< queue */
		amqp_bytes_t exchange; /**< exchange */
		amqp_bytes_t routing_key; /**< routing-key */
		amqp_table_t arguments; /**< arguments */
	} amqp_queue_unbind_t;

	#define AMQP_QUEUE_UNBIND_OK_METHOD ((amqp_method_number_t)0x00320033) /**< queue.unbind-ok method id @internal 50, 51; 3276851 */
	/** queue.unbind-ok method fields */
	typedef struct amqp_queue_unbind_ok_t_ {
		char   dummy; // Dummy field to avoid empty struct
	} amqp_queue_unbind_ok_t;

	#define AMQP_BASIC_QOS_METHOD ((amqp_method_number_t)0x003C000A) /**< basic.qos method id @internal 60, 10; 3932170 */
	/** basic.qos method fields */
	typedef struct amqp_basic_qos_t_ {
		uint32 prefetch_size; /**< prefetch-size */
		uint16 prefetch_count; /**< prefetch-count */
		boolint global; /**< global */
	} amqp_basic_qos_t;

	#define AMQP_BASIC_QOS_OK_METHOD ((amqp_method_number_t)0x003C000B) /**< basic.qos-ok method id @internal 60, 11; 3932171 */
	/** basic.qos-ok method fields */
	typedef struct amqp_basic_qos_ok_t_ {
		char   dummy; // Dummy field to avoid empty struct
	} amqp_basic_qos_ok_t;

	#define AMQP_BASIC_CONSUME_METHOD ((amqp_method_number_t)0x003C0014) /**< basic.consume method id @internal 60, 20; 3932180 */
	/** basic.consume method fields */
	typedef struct amqp_basic_consume_t_ {
		uint16 ticket; /**< ticket */
		amqp_bytes_t queue; /**< queue */
		amqp_bytes_t consumer_tag; /**< consumer-tag */
		boolint no_local; /**< no-local */
		boolint no_ack; /**< no-ack */
		boolint exclusive; /**< exclusive */
		boolint nowait; /**< nowait */
		amqp_table_t arguments; /**< arguments */
	} amqp_basic_consume_t;

	#define AMQP_BASIC_CONSUME_OK_METHOD ((amqp_method_number_t)0x003C0015) /**< basic.consume-ok method id @internal 60, 21; 3932181 */
	/** basic.consume-ok method fields */
	typedef struct amqp_basic_consume_ok_t_ {
		amqp_bytes_t consumer_tag; /**< consumer-tag */
	} amqp_basic_consume_ok_t;

	#define AMQP_BASIC_CANCEL_METHOD ((amqp_method_number_t)0x003C001E) /**< basic.cancel method id @internal 60, 30; 3932190 */
	/** basic.cancel method fields */
	typedef struct amqp_basic_cancel_t_ {
		amqp_bytes_t consumer_tag; /**< consumer-tag */
		boolint nowait; /**< nowait */
	} amqp_basic_cancel_t;

	#define AMQP_BASIC_CANCEL_OK_METHOD ((amqp_method_number_t)0x003C001F) /**< basic.cancel-ok method id @internal 60, 31; 3932191 */
	/** basic.cancel-ok method fields */
	typedef struct amqp_basic_cancel_ok_t_ {
		amqp_bytes_t consumer_tag; /**< consumer-tag */
	} amqp_basic_cancel_ok_t;

	#define AMQP_BASIC_PUBLISH_METHOD ((amqp_method_number_t)0x003C0028) /**< basic.publish method id @internal 60, 40; 3932200 */
	/** basic.publish method fields */
	typedef struct amqp_basic_publish_t_ {
		uint16 ticket; /**< ticket */
		amqp_bytes_t exchange; /**< exchange */
		amqp_bytes_t routing_key; /**< routing-key */
		boolint mandatory; /**< mandatory */
		boolint immediate; /**< immediate */
	} amqp_basic_publish_t;

	#define AMQP_BASIC_RETURN_METHOD ((amqp_method_number_t)0x003C0032) /**< basic.return method id @internal 60, 50; 3932210 */
	/** basic.return method fields */
	typedef struct amqp_basic_return_t_ {
		uint16 reply_code; /**< reply-code */
		amqp_bytes_t reply_text; /**< reply-text */
		amqp_bytes_t exchange; /**< exchange */
		amqp_bytes_t routing_key; /**< routing-key */
	} amqp_basic_return_t;

	#define AMQP_BASIC_DELIVER_METHOD ((amqp_method_number_t)0x003C003C) /**< basic.deliver method id @internal 60, 60; 3932220 */
	/** basic.deliver method fields */
	typedef struct amqp_basic_deliver_t_ {
		amqp_bytes_t consumer_tag; /**< consumer-tag */
		uint64 delivery_tag; /**< delivery-tag */
		boolint redelivered; /**< redelivered */
		amqp_bytes_t exchange; /**< exchange */
		amqp_bytes_t routing_key; /**< routing-key */
	} amqp_basic_deliver_t;

	#define AMQP_BASIC_GET_METHOD ((amqp_method_number_t)0x003C0046) /**< basic.get method id @internal 60, 70; 3932230 */
	/** basic.get method fields */
	typedef struct amqp_basic_get_t_ {
		uint16 ticket; /**< ticket */
		amqp_bytes_t queue; /**< queue */
		boolint no_ack; /**< no-ack */
	} amqp_basic_get_t;

	#define AMQP_BASIC_GET_OK_METHOD ((amqp_method_number_t)0x003C0047) /**< basic.get-ok method id @internal 60, 71; 3932231 */
	/** basic.get-ok method fields */
	typedef struct amqp_basic_get_ok_t_ {
		uint64 delivery_tag; /**< delivery-tag */
		boolint redelivered; /**< redelivered */
		amqp_bytes_t exchange; /**< exchange */
		amqp_bytes_t routing_key; /**< routing-key */
		uint32 message_count; /**< message-count */
	} amqp_basic_get_ok_t;

	#define AMQP_BASIC_GET_EMPTY_METHOD ((amqp_method_number_t)0x003C0048) /**< basic.get-empty method id @internal 60, 72; 3932232 */
	/** basic.get-empty method fields */
	typedef struct amqp_basic_get_empty_t_ {
		amqp_bytes_t cluster_id; /**< cluster-id */
	} amqp_basic_get_empty_t;

	#define AMQP_BASIC_ACK_METHOD ((amqp_method_number_t)0x003C0050) /**< basic.ack method id @internal 60, 80; 3932240 */
	/** basic.ack method fields */
	typedef struct amqp_basic_ack_t_ {
		uint64 delivery_tag; /**< delivery-tag */
		boolint multiple; /**< multiple */
	} amqp_basic_ack_t;

	#define AMQP_BASIC_REJECT_METHOD ((amqp_method_number_t)0x003C005A) /**< basic.reject method id @internal 60, 90; 3932250 */
	/** basic.reject method fields */
	typedef struct amqp_basic_reject_t_ {
		uint64 delivery_tag; /**< delivery-tag */
		boolint requeue; /**< requeue */
	} amqp_basic_reject_t;

	#define AMQP_BASIC_RECOVER_ASYNC_METHOD ((amqp_method_number_t)0x003C0064) /**< basic.recover-async method id @internal 60, 100; 3932260 */
	/** basic.recover-async method fields */
	typedef struct amqp_basic_recover_async_t_ {
		boolint requeue; /**< requeue */
	} amqp_basic_recover_async_t;

	#define AMQP_BASIC_RECOVER_METHOD ((amqp_method_number_t)0x003C006E) /**< basic.recover method id @internal 60, 110; 3932270 */
	/** basic.recover method fields */
	typedef struct amqp_basic_recover_t_ {
		boolint requeue; /**< requeue */
	} amqp_basic_recover_t;

	#define AMQP_BASIC_RECOVER_OK_METHOD ((amqp_method_number_t)0x003C006F) /**< basic.recover-ok method id @internal 60, 111; 3932271 */
	/** basic.recover-ok method fields */
	typedef struct amqp_basic_recover_ok_t_ {
		char   dummy; // Dummy field to avoid empty struct
	} amqp_basic_recover_ok_t;

	#define AMQP_BASIC_NACK_METHOD ((amqp_method_number_t)0x003C0078) /**< basic.nack method id @internal 60, 120; 3932280 */
	/** basic.nack method fields */
	typedef struct amqp_basic_nack_t_ {
		uint64 delivery_tag; /**< delivery-tag */
		boolint multiple; /**< multiple */
		boolint requeue; /**< requeue */
	} amqp_basic_nack_t;

	#define AMQP_TX_SELECT_METHOD ((amqp_method_number_t)0x005A000A) /**< tx.select method id @internal 90, 10; 5898250 */
	/** tx.select method fields */
	typedef struct amqp_tx_select_t_ {
		char   dummy; // Dummy field to avoid empty struct
	} amqp_tx_select_t;

	#define AMQP_TX_SELECT_OK_METHOD ((amqp_method_number_t)0x005A000B) /**< tx.select-ok method id @internal 90, 11; 5898251 */
	/** tx.select-ok method fields */
	typedef struct amqp_tx_select_ok_t_ {
		char   dummy; // Dummy field to avoid empty struct
	} amqp_tx_select_ok_t;

	#define AMQP_TX_COMMIT_METHOD ((amqp_method_number_t)0x005A0014) /**< tx.commit method id @internal 90, 20; 5898260 */
	/** tx.commit method fields */
	typedef struct amqp_tx_commit_t_ {
		char   dummy; // Dummy field to avoid empty struct
	} amqp_tx_commit_t;

	#define AMQP_TX_COMMIT_OK_METHOD ((amqp_method_number_t)0x005A0015) /**< tx.commit-ok method id @internal 90, 21; 5898261 */
	/** tx.commit-ok method fields */
	typedef struct amqp_tx_commit_ok_t_ {
		char   dummy; // Dummy field to avoid empty struct
	} amqp_tx_commit_ok_t;

	#define AMQP_TX_ROLLBACK_METHOD ((amqp_method_number_t)0x005A001E) /**< tx.rollback method id @internal 90, 30; 5898270 */
	/** tx.rollback method fields */
	typedef struct amqp_tx_rollback_t_ {
		char   dummy; // Dummy field to avoid empty struct
	} amqp_tx_rollback_t;

	#define AMQP_TX_ROLLBACK_OK_METHOD ((amqp_method_number_t)0x005A001F) /**< tx.rollback-ok method id @internal 90, 31; 5898271 */
	/** tx.rollback-ok method fields */
	typedef struct amqp_tx_rollback_ok_t_ {
		char   dummy; // Dummy field to avoid empty struct
	} amqp_tx_rollback_ok_t;

	#define AMQP_CONFIRM_SELECT_METHOD ((amqp_method_number_t)0x0055000A) /**< confirm.select method id @internal 85, 10; 5570570 */
	/** confirm.select method fields */
	typedef struct amqp_confirm_select_t_ {
		boolint nowait; /**< nowait */
	} amqp_confirm_select_t;

	#define AMQP_CONFIRM_SELECT_OK_METHOD ((amqp_method_number_t)0x0055000B) /**< confirm.select-ok method id @internal 85, 11; 5570571 */
	/** confirm.select-ok method fields */
	typedef struct amqp_confirm_select_ok_t_ {
		char   dummy; // Dummy field to avoid empty struct
	} amqp_confirm_select_ok_t;

	/* Class property records. */
	#define AMQP_CONNECTION_CLASS (0x000A) /**< connection class id @internal 10 \
				  */
	/** connection class properties */
	typedef struct amqp_connection_properties_t_ {
		amqp_flags_t _flags; /**< bit-mask of set fields */
		char   dummy; // Dummy field to avoid empty struct
	} amqp_connection_properties_t;

	#define AMQP_CHANNEL_CLASS (0x0014) /**< channel class id @internal 20 */
	/** channel class properties */
	typedef struct amqp_channel_properties_t_ {
		amqp_flags_t _flags; /**< bit-mask of set fields */
		char   dummy; // Dummy field to avoid empty struct
	} amqp_channel_properties_t;

	#define AMQP_ACCESS_CLASS (0x001E) /**< access class id @internal 30 */
	/** access class properties */
	typedef struct amqp_access_properties_t_ {
		amqp_flags_t _flags; /**< bit-mask of set fields */
		char   dummy; // Dummy field to avoid empty struct
	} amqp_access_properties_t;

	#define AMQP_EXCHANGE_CLASS (0x0028) /**< exchange class id @internal 40 */
	/** exchange class properties */
	typedef struct amqp_exchange_properties_t_ {
		amqp_flags_t _flags; /**< bit-mask of set fields */
		char   dummy; // Dummy field to avoid empty struct
	} amqp_exchange_properties_t;

	#define AMQP_QUEUE_CLASS (0x0032) /**< queue class id @internal 50 */
	/** queue class properties */
	typedef struct amqp_queue_properties_t_ {
		amqp_flags_t _flags; /**< bit-mask of set fields */
		char   dummy; // Dummy field to avoid empty struct
	} amqp_queue_properties_t;

	#define AMQP_BASIC_CLASS (0x003C) /**< basic class id @internal 60 */
	#define AMQP_BASIC_CONTENT_TYPE_FLAG (1 << 15)
	#define AMQP_BASIC_CONTENT_ENCODING_FLAG (1 << 14)
	#define AMQP_BASIC_HEADERS_FLAG (1 << 13)
	#define AMQP_BASIC_DELIVERY_MODE_FLAG (1 << 12)
	#define AMQP_BASIC_PRIORITY_FLAG (1 << 11)
	#define AMQP_BASIC_CORRELATION_ID_FLAG (1 << 10)
	#define AMQP_BASIC_REPLY_TO_FLAG (1 << 9)
	#define AMQP_BASIC_EXPIRATION_FLAG (1 << 8)
	#define AMQP_BASIC_MESSAGE_ID_FLAG (1 << 7)
	#define AMQP_BASIC_TIMESTAMP_FLAG (1 << 6)
	#define AMQP_BASIC_TYPE_FLAG (1 << 5)
	#define AMQP_BASIC_USER_ID_FLAG (1 << 4)
	#define AMQP_BASIC_APP_ID_FLAG (1 << 3)
	#define AMQP_BASIC_CLUSTER_ID_FLAG (1 << 2)
	//
	// basic class properties 
	//
	struct amqp_basic_properties_t {
		amqp_flags_t _flags;           // bit-mask of set fields
		amqp_bytes_t content_type;     // content-type
		amqp_bytes_t content_encoding; // content-encoding
		amqp_table_t headers;          // headers
		uint8  delivery_mode;          // delivery-mode
		uint8  priority;               // priority
		amqp_bytes_t correlation_id;   // correlation-id
		amqp_bytes_t reply_to;         // reply-to
		amqp_bytes_t expiration;       // expiration
		amqp_bytes_t message_id;       // message-id
		uint64 timestamp;              // timestamp
		amqp_bytes_t type;             // type
		amqp_bytes_t user_id;          // user-id
		amqp_bytes_t app_id;           // app-id 
		amqp_bytes_t cluster_id;       // cluster-id 
	};

	#define AMQP_TX_CLASS (0x005A) /**< tx class id @internal 90 */
	/** tx class properties */
	typedef struct amqp_tx_properties_t_ {
		amqp_flags_t _flags; /**< bit-mask of set fields */
		char   dummy; // Dummy field to avoid empty struct
	} amqp_tx_properties_t;

	#define AMQP_CONFIRM_CLASS (0x0055) /**< confirm class id @internal 85 */
	/** confirm class properties */
	typedef struct amqp_confirm_properties_t_ {
		amqp_flags_t _flags; /**< bit-mask of set fields */
		char   dummy; // Dummy field to avoid empty struct
	} amqp_confirm_properties_t;

	/* API functions for methods */

	/**
	 * amqp_channel_open
	 *
	 * @param [in] state connection state
	 * @param [in] channel the channel to do the RPC on
	 * @returns amqp_channel_open_ok_t
	 */
	AMQP_PUBLIC_FUNCTION amqp_channel_open_ok_t * amqp_channel_open(amqp_connection_state_t state, amqp_channel_t channel);
	/**
	 * amqp_channel_flow
	 *
	 * @param [in] state connection state
	 * @param [in] channel the channel to do the RPC on
	 * @param [in] active active
	 * @returns amqp_channel_flow_ok_t
	 */
	AMQP_PUBLIC_FUNCTION amqp_channel_flow_ok_t * amqp_channel_flow(amqp_connection_state_t state, amqp_channel_t channel, boolint active);
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
		amqp_bytes_t exchange, amqp_bytes_t type, boolint passive, boolint durable, boolint auto_delete, boolint internal, amqp_table_t arguments);
	/**
	 * amqp_exchange_delete
	 *
	 * @param [in] state connection state
	 * @param [in] channel the channel to do the RPC on
	 * @param [in] exchange exchange
	 * @param [in] if_unused if_unused
	 * @returns amqp_exchange_delete_ok_t
	 */
	AMQP_PUBLIC_FUNCTION amqp_exchange_delete_ok_t * amqp_exchange_delete(amqp_connection_state_t state, amqp_channel_t channel, amqp_bytes_t exchange, boolint if_unused);
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
		amqp_bytes_t destination, amqp_bytes_t source, amqp_bytes_t routing_key, amqp_table_t arguments);
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
		amqp_bytes_t destination, amqp_bytes_t source, amqp_bytes_t routing_key, amqp_table_t arguments);
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
		boolint passive, boolint durable, boolint exclusive, boolint auto_delete, amqp_table_t arguments);
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
		amqp_bytes_t exchange, amqp_bytes_t routing_key, amqp_table_t arguments);
	/**
	 * amqp_queue_purge
	 *
	 * @param [in] state connection state
	 * @param [in] channel the channel to do the RPC on
	 * @param [in] queue queue
	 * @returns amqp_queue_purge_ok_t
	 */
	AMQP_PUBLIC_FUNCTION amqp_queue_purge_ok_t * amqp_queue_purge(amqp_connection_state_t state, amqp_channel_t channel, amqp_bytes_t queue);
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
	AMQP_PUBLIC_FUNCTION amqp_queue_delete_ok_t * amqp_queue_delete(amqp_connection_state_t state, amqp_channel_t channel, amqp_bytes_t queue, boolint if_unused, boolint if_empty);
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
		amqp_bytes_t exchange, amqp_bytes_t routing_key, amqp_table_t arguments);
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
	AMQP_PUBLIC_FUNCTION amqp_basic_qos_ok_t * amqp_basic_qos(amqp_connection_state_t state, amqp_channel_t channel, uint32 prefetch_size, uint16 prefetch_count, boolint global);
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
		amqp_bytes_t consumer_tag, boolint no_local, boolint no_ack, boolint exclusive, amqp_table_t arguments);
	/**
	 * amqp_basic_cancel
	 *
	 * @param [in] state connection state
	 * @param [in] channel the channel to do the RPC on
	 * @param [in] consumer_tag consumer_tag
	 * @returns amqp_basic_cancel_ok_t
	 */
	AMQP_PUBLIC_FUNCTION amqp_basic_cancel_ok_t * amqp_basic_cancel(amqp_connection_state_t state, amqp_channel_t channel, amqp_bytes_t consumer_tag);
	/**
	 * amqp_basic_recover
	 *
	 * @param [in] state connection state
	 * @param [in] channel the channel to do the RPC on
	 * @param [in] requeue requeue
	 * @returns amqp_basic_recover_ok_t
	 */
	AMQP_PUBLIC_FUNCTION amqp_basic_recover_ok_t * amqp_basic_recover(amqp_connection_state_t state, amqp_channel_t channel, boolint requeue);
	/**
	 * amqp_tx_select
	 *
	 * @param [in] state connection state
	 * @param [in] channel the channel to do the RPC on
	 * @returns amqp_tx_select_ok_t
	 */
	AMQP_PUBLIC_FUNCTION amqp_tx_select_ok_t * amqp_tx_select(amqp_connection_state_t state, amqp_channel_t channel);
	/**
	 * amqp_tx_commit
	 *
	 * @param [in] state connection state
	 * @param [in] channel the channel to do the RPC on
	 * @returns amqp_tx_commit_ok_t
	 */
	AMQP_PUBLIC_FUNCTION amqp_tx_commit_ok_t * amqp_tx_commit(amqp_connection_state_t state, amqp_channel_t channel);
	/**
	 * amqp_tx_rollback
	 *
	 * @param [in] state connection state
	 * @param [in] channel the channel to do the RPC on
	 * @returns amqp_tx_rollback_ok_t
	 */
	AMQP_PUBLIC_FUNCTION amqp_tx_rollback_ok_t * amqp_tx_rollback(amqp_connection_state_t state, amqp_channel_t channel);
	/**
	 * amqp_confirm_select
	 *
	 * @param [in] state connection state
	 * @param [in] channel the channel to do the RPC on
	 * @returns amqp_confirm_select_ok_t
	 */
	AMQP_PUBLIC_FUNCTION amqp_confirm_select_ok_t * amqp_confirm_select(amqp_connection_state_t state, amqp_channel_t channel);
//AMQP_END_DECLS
// } amqp_framing.h 
//AMQP_BEGIN_DECLS
AMQP_PUBLIC_VARIABLE const amqp_bytes_t amqp_empty_bytes; // Empty bytes structure
AMQP_PUBLIC_VARIABLE const amqp_table_t amqp_empty_table; // Empty table structure
AMQP_PUBLIC_VARIABLE const amqp_array_t amqp_empty_array; // Empty table array structure
//
// Compatibility macros for the above, to avoid the need to update code written against earlier versions of librabbitmq. 
//
/**
 * \def AMQP_EMPTY_BYTES
 * \deprecated use \ref amqp_empty_bytes instead
 * \since v0.1
 */
// @sobolev #define AMQP_EMPTY_BYTES amqp_empty_bytes
/**
 * \def AMQP_EMPTY_TABLE
 * \deprecated use \ref amqp_empty_table instead
 * \since v0.1
 */
// @sobolev #define AMQP_EMPTY_TABLE amqp_empty_table
/**
 * \def AMQP_EMPTY_ARRAY
 * \deprecated use \ref amqp_empty_array instead
 * \since v0.1
 */
// @sobolev #define AMQP_EMPTY_ARRAY amqp_empty_array
/**
 * Initializes an amqp_pool_t memory allocation pool for use
 *
 * Readies an allocation pool for use. An amqp_pool_t
 * must be initialized before use
 *
 * \param [in] pool the amqp_pool_t structure to initialize.
 *     Calling this function on a pool a pool that has
 *     already been initialized will result in undefined
 *     behavior
 * \param [in] pagesize the unit size that the pool will allocate
 *     memory chunks in. Anything allocated against the pool
 *     with a requested size will be carved out of a block
 *     this size. Allocations larger than this will be
 *     allocated individually
 *
 * \sa recycle_amqp_pool(), empty_amqp_pool(), amqp_pool_alloc(), amqp_pool_alloc_bytes(), amqp_pool_t
 * \since v0.1
 */
AMQP_PUBLIC_FUNCTION void FASTCALL init_amqp_pool(amqp_pool_t *pool, size_t pagesize);
/**
 * Recycles an amqp_pool_t memory allocation pool
 *
 * Recycles the space allocate by the pool
 *
 * This invalidates all allocations made against the pool before this call is
 * made, any use of any allocations made before recycle_amqp_pool() is called
 * will result in undefined behavior.
 *
 * Note: this may or may not release memory, to force memory to be released
 * call empty_amqp_pool().
 *
 * \param [in] pool the amqp_pool_t to recycle
 *
 * \sa recycle_amqp_pool(), empty_amqp_pool(), amqp_pool_alloc(),
 *   amqp_pool_alloc_bytes()
 *
 * \since v0.1
 *
 */
AMQP_PUBLIC_FUNCTION void recycle_amqp_pool(amqp_pool_t *pool);
/**
 * Empties an amqp memory pool
  * Releases all memory associated with an allocation pool
  * \param [in] pool the amqp_pool_t to empty
  * \since v0.1
 */
AMQP_PUBLIC_FUNCTION void FASTCALL empty_amqp_pool(amqp_pool_t * pPool);
/**
 * Allocates a block of memory from an amqp_pool_t memory pool
 *
 * Memory will be aligned on a 8-byte boundary. If a 0-length allocation is
 * requested, a NULL pointer will be returned.
 *
 * \param [in] pool the allocation pool to allocate the memory from
 * \param [in] amount the size of the allocation in bytes.
 * \return a pointer to the memory block, or NULL if the allocation cannot be satisfied.
 *
 * \sa init_amqp_pool(), recycle_amqp_pool(), empty_amqp_pool(), amqp_pool_alloc_bytes()
  * \since v0.1
 */
AMQP_PUBLIC_FUNCTION void * FASTCALL amqp_pool_alloc(amqp_pool_t * pool, size_t amount);
/**
 * Allocates a block of memory from an amqp_pool_t to an amqp_bytes_t
 *
 * Memory will be aligned on a 8-byte boundary. If a 0-length allocation is
 * requested, output.bytes = NULL.
 *
 * \param [in] pool the allocation pool to allocate the memory from
 * \param [in] amount the size of the allocation in bytes
 * \param [in] output the location to store the pointer. On success
 * output.bytes will be set to the beginning of the buffer output.len will be set to amount
 * On error output.bytes will be set to NULL and output.len set to 0
 *
 * \sa init_amqp_pool(), recycle_amqp_pool(), empty_amqp_pool(), amqp_pool_alloc()
 * \since v0.1
 */
AMQP_PUBLIC_FUNCTION void FASTCALL amqp_pool_alloc_bytes(amqp_pool_t * pool, size_t amount, amqp_bytes_t * output);
/**
 * Wraps a c string in an amqp_bytes_t
 *
 * Takes a string, calculates its length and creates an
 * amqp_bytes_t that points to it. The string is not duplicated.
 *
 * For a given input cstr, The amqp_bytes_t output.bytes is the
 * same as cstr, output.len is the length of the string not including
 * the \0 terminator
 *
 * This function uses strlen() internally so cstr must be properly terminated
 *
 * \param [in] cstr the c string to wrap
 * \return an amqp_bytes_t that describes the string
 *
 * \since v0.1
 */
AMQP_PUBLIC_FUNCTION amqp_bytes_t FASTCALL amqp_cstring_bytes(char const *cstr);
/**
 * Duplicates an amqp_bytes_t buffer.
 *
 * The buffer is cloned and the contents copied.
 *
 * The memory associated with the output is allocated
 * with amqp_bytes_malloc() and should be freed with amqp_bytes_free()
 *
 * \param [in] src
 * \return a clone of the src
 * \sa amqp_bytes_free(), amqp_bytes_malloc()
 * \since v0.1
 */
AMQP_PUBLIC_FUNCTION amqp_bytes_t FASTCALL amqp_bytes_malloc_dup(const amqp_bytes_t & rS);
/**
 * Allocates a amqp_bytes_t buffer
 *
 * Creates an amqp_bytes_t buffer of the specified amount, the buffer should be freed using amqp_bytes_free()
 *
 * \param [in] amount the size of the buffer in bytes
 * \returns an amqp_bytes_t with amount bytes allocated.
 *  output.bytes will be set to NULL on error
 *
 * \sa amqp_bytes_free(), amqp_bytes_malloc_dup()
 * \since v0.1
 */
AMQP_PUBLIC_FUNCTION amqp_bytes_t amqp_bytes_malloc(size_t amount);
/**
 * Frees an amqp_bytes_t buffer
 *
 * Frees a buffer allocated with amqp_bytes_malloc() or amqp_bytes_malloc_dup()
 *
 * Calling amqp_bytes_free on buffers not allocated with one
 * of those two functions will result in undefined behavior
 *
 * \param [in] bytes the buffer to free
 *
 * \sa amqp_bytes_malloc(), amqp_bytes_malloc_dup()
 *
 * \since v0.1
 */
AMQP_PUBLIC_FUNCTION void FASTCALL amqp_bytes_free(amqp_bytes_t & rBytes);
/**
 * Allocate and initialize a new amqp_connection_state_t object
 *
 * amqp_connection_state_t objects created with this function
 * should be freed with amqp_destroy_connection()
 *
 * \returns an opaque pointer on success, NULL or 0 on failure.
 * \sa amqp_destroy_connection()
 * \since v0.1
 */
AMQP_PUBLIC_FUNCTION amqp_connection_state_t amqp_new_connection();
/**
 * Get the underlying socket descriptor for the connection
 *
 * \warning Use the socket returned from this function carefully, incorrect use
 * of the socket outside of the library will lead to undefined behavior.
 * Additionally rabbitmq-c may use the socket differently version-to-version,
 * what may work in one version, may break in the next version. Be sure to
 * throughly test any applications that use the socket returned by this
 * function especially when using a newer version of rabbitmq-c
 *
 * \param [in] state the connection object
 * \returns the socket descriptor if one has been set, -1 otherwise
 * \sa amqp_tcp_socket_new(), amqp_ssl_socket_new(), amqp_socket_open()
 * \since v0.1
 */
AMQP_PUBLIC_FUNCTION int FASTCALL amqp_get_sockfd(amqp_connection_state_t state);
/**
 * Deprecated, use amqp_tcp_socket_new() or amqp_ssl_socket_new()
 *
 * \deprecated Use amqp_tcp_socket_new() or amqp_ssl_socket_new()
 *
 * Sets the socket descriptor associated with the connection. The socket
 * should be connected to a broker, and should not be read to or written from
 * before calling this function.  A socket descriptor can be created and opened
 * using amqp_open_socket()
 *
 * \param [in] state the connection object
 * \param [in] sockfd the socket
 * \sa amqp_open_socket(), amqp_tcp_socket_new(), amqp_ssl_socket_new()
 * \since v0.1
 */
AMQP_DEPRECATED(AMQP_PUBLIC_FUNCTION void amqp_set_sockfd(amqp_connection_state_t state, int sockfd));
/**
 * Tune client side parameters
 *
 * \warning This function may call abort() if the connection is in a certain
 *  state. As such it should probably not be called code outside the library.
 *  connection parameters should be specified when calling amqp_login() or
 *  amqp_login_with_properties()
 *
 * This function changes channel_max, frame_max, and heartbeat parameters, on
 * the client side only. It does not try to renegotiate these parameters with
 * the broker. Using this function will lead to unexpected results.
 *
 * \param [in] state the connection object
 * \param [in] channel_max the maximum number of channels. The largest this can be is 65535
 * \param [in] frame_max the maximum size of an frame. The smallest this can be is 4096
 * The largest this can be is 2147483647. Unless you know what you're doing the recommended size is 131072 or 128KB
 * \param [in] heartbeat the number of seconds between heartbeats
 *
 * \return AMQP_STATUS_OK on success, an amqp_status_enum value otherwise.
 *  Possible error codes include:
 *  - AMQP_STATUS_NO_MEMORY memory allocation failed.
 *  - AMQP_STATUS_TIMER_FAILURE the underlying system timer indicated it failed.
 * \sa amqp_login(), amqp_login_with_properties()
 * \since v0.1
 */
AMQP_PUBLIC_FUNCTION int amqp_tune_connection(amqp_connection_state_t state, int channel_max, int frame_max, int heartbeat);
/**
 * Get the maximum number of channels the connection can handle
 *
 * The maximum number of channels is set when connection negotiation takes
 * place in amqp_login() or amqp_login_with_properties().
 *
 * \param [in] state the connection object
 * \return the maximum number of channels. 0 if there is no limit
 * \since v0.1
 */
AMQP_PUBLIC_FUNCTION int amqp_get_channel_max(const amqp_connection_state_t state);
/**
 * Get the maximum size of an frame the connection can handle
 *
 * The maximum size of an frame is set when connection negotiation takes
 * place in amqp_login() or amqp_login_with_properties().
 *
 * \param [in] state the connection object
 * \return the maximum size of an frame.
 * \since v0.6
 */
AMQP_PUBLIC_FUNCTION int amqp_get_frame_max(const amqp_connection_state_t state);
/**
 * Get the number of seconds between heartbeats of the connection
 *
 * The number of seconds between heartbeats is set when connection
 * negotiation takes place in amqp_login() or amqp_login_with_properties().
 *
 * \param [in] state the connection object
 * \return the number of seconds between heartbeats.
 *
 * \since v0.6
 */
AMQP_PUBLIC_FUNCTION int amqp_get_heartbeat(const amqp_connection_state_t state);
/**
 * Destroys an amqp_connection_state_t object
 *
 * Destroys a amqp_connection_state_t object that was created with
 * amqp_new_connection(). If the connection with the broker is open, it will be
 * implicitly closed with a reply code of 200 (success). Any memory that
 * would be freed with amqp_maybe_release_buffers() or
 * amqp_maybe_release_buffers_on_channel() will be freed, and use of that
 * memory will caused undefined behavior.
 *
 * \param [in] state the connection object
 * \return AMQP_STATUS_OK on success. amqp_status_enum value failure
 * \sa amqp_new_connection()
 * \since v0.1
 */
AMQP_PUBLIC_FUNCTION int amqp_destroy_connection(amqp_connection_state_t state);
/**
 * Process incoming data
 *
 * \warning This is a low-level function intended for those who want to
 *  have greater control over input and output over the socket from the
 *  broker. Correctly using this function requires in-depth knowledge of AMQP
 *  and rabbitmq-c.
 *
 * For a given buffer of data received from the broker, decode the first
 * frame in the buffer. If more than one frame is contained in the input buffer
 * the return value will be less than the received_data size, the caller should
 * adjust received_data buffer descriptor to point to the beginning of the
 * buffer + the return value.
 *
 * \param [in] state the connection object
 * \param [in] received_data a buffer of data received from the broker. The
 *  function will return the number of bytes of the buffer it used. The
 *  function copies these bytes to an internal buffer: this part of the buffer
 *  may be reused after this function successfully completes.
 * \param [in,out] decoded_frame caller should pass in a pointer to an
 *  amqp_frame_t struct. If there is enough data in received_data for a
 *  complete frame, decoded_frame->frame_type will be set to something OTHER
 *  than 0. decoded_frame may contain members pointing to memory owned by
 *  the state object. This memory can be recycled with
 *  amqp_maybe_release_buffers() or amqp_maybe_release_buffers_on_channel().
 * \return number of bytes consumed from received_data or 0 if a 0-length
 *  buffer was passed. A negative return value indicates failure. Possible
 * errors:
 *  - AMQP_STATUS_NO_MEMORY failure in allocating memory. The library is likely
 * in an indeterminate state making recovery unlikely. Client should note the
 * error and terminate the application
 *  - AMQP_STATUS_BAD_AMQP_DATA bad AMQP data was received. The connection
 * should be shutdown immediately
 *  - AMQP_STATUS_UNKNOWN_METHOD: an unknown method was received from the
 * broker. This is likely a protocol error and the connection should be
 * shutdown immediately
 *  - AMQP_STATUS_UNKNOWN_CLASS: a properties frame with an unknown class
 * was received from the broker. This is likely a protocol error and the
 * connection should be shutdown immediately
 *
 * \since v0.1
 */
AMQP_PUBLIC_FUNCTION int amqp_handle_input(amqp_connection_state_t state, amqp_bytes_t received_data, amqp_frame_t *decoded_frame);
/**
 * Check to see if connection memory can be released
 *
 * \deprecated This function is deprecated in favor of amqp_maybe_release_buffers() or amqp_maybe_release_buffers_on_channel()
 *
 * Checks the state of an amqp_connection_state_t object to see if
 * amqp_release_buffers() can be called successfully.
 *
 * \param [in] state the connection object
 * \returns TRUE if the buffers can be released FALSE otherwise
 *
 * \sa amqp_release_buffers() amqp_maybe_release_buffers()
 *  amqp_maybe_release_buffers_on_channel()
 *
 * \since v0.1
 */
AMQP_PUBLIC_FUNCTION boolint amqp_release_buffers_ok(const amqp_connection_state_t state);
/**
 * Release amqp_connection_state_t owned memory
 *
 * \deprecated This function is deprecated in favor of amqp_maybe_release_buffers() or amqp_maybe_release_buffers_on_channel()
 *
 * \warning caller should ensure amqp_release_buffers_ok() returns true before
 *  calling this function. Failure to do so may result in abort() being called.
 *
 * Release memory owned by the amqp_connection_state_t for reuse by the
 * library. Use of any memory returned by the library before this function is
 * called will result in undefined behavior.
 *
 * \note internally rabbitmq-c tries to reuse memory when possible. As a result
 * its possible calling this function may not have a noticeable effect on
 * memory usage.
 *
 * \param [in] state the connection object
 *
 * \sa amqp_release_buffers_ok() amqp_maybe_release_buffers()
 *  amqp_maybe_release_buffers_on_channel()
 *
 * \since v0.1
 */
AMQP_PUBLIC_FUNCTION void amqp_release_buffers(amqp_connection_state_t state);
/**
 * Release amqp_connection_state_t owned memory
 *
 * Release memory owned by the amqp_connection_state_t object related to any
 * channel, allowing reuse by the library. Use of any memory returned by the
 * library before this function is called with result in undefined behavior.
 *
 * \note internally rabbitmq-c tries to reuse memory when possible. As a result
 * its possible calling this function may not have a noticeable effect on memory usage.
 *
 * \param [in] state the connection object
 * \sa amqp_maybe_release_buffers_on_channel()
 * \since v0.1
 */
AMQP_PUBLIC_FUNCTION void amqp_maybe_release_buffers(amqp_connection_state_t state);
/**
 * Release amqp_connection_state_t owned memory related to a channel
 *
 * Release memory owned by the amqp_connection_state_t object related to the
 * specified channel, allowing reuse by the library. Use of any memory returned
 * the library for a specific channel will result in undefined behavior.
 *
 * \note internally rabbitmq-c tries to reuse memory when possible. As a result
 * its possible calling this function may not have a noticeable effect on
 * memory usage.
 *
 * \param [in] state the connection object
 * \param [in] channel the channel specifier for which memory should be
 *  released. Note that the library does not care about the state of the
 *  channel when calling this function
 *
 * \sa amqp_maybe_release_buffers()
 * \since v0.4.0
 */
AMQP_PUBLIC_FUNCTION void amqp_maybe_release_buffers_on_channel(amqp_connection_state_t state, amqp_channel_t channel);
/**
 * Send a frame to the broker
 *
 * \param [in] state the connection object
 * \param [in] frame the frame to send to the broker
 * \return AMQP_STATUS_OK on success, an amqp_status_enum value on error.
 *  Possible error codes:
 *  - AMQP_STATUS_BAD_AMQP_DATA the serialized form of the method or
 * properties was too large to fit in a single AMQP frame, or the
 * method contains an invalid value. The frame was not sent.
 *  - AMQP_STATUS_TABLE_TOO_BIG the serialized form of an amqp_table_t is
 * too large to fit in a single AMQP frame. Frame was not sent.
 *  - AMQP_STATUS_UNKNOWN_METHOD an invalid method type was passed in
 *  - AMQP_STATUS_UNKNOWN_CLASS an invalid properties type was passed in
 *  - AMQP_STATUS_TIMER_FAILURE system timer indicated failure. The frame
 * was sent
 *  - AMQP_STATUS_SOCKET_ERROR
 *  - AMQP_STATUS_SSL_ERROR
 *
 * \since v0.1
 */
AMQP_PUBLIC_FUNCTION int amqp_send_frame(amqp_connection_state_t state, amqp_frame_t const *frame);
/**
 * Compare two table entries
 *
 * Works just like strcmp(), comparing two the table keys, datatype, then values
 *
 * \param [in] entry1 the entry on the left
 * \param [in] entry2 the entry on the right
 * \return 0 if entries are equal, 0 < if left is greater, 0 > if right is greater
 *
 * \since v0.1
 */
AMQP_PUBLIC_FUNCTION int amqp_table_entry_cmp(void const *entry1, void const *entry2);
/**
 * Open a socket to a remote host
 *
 * \deprecated This function is deprecated in favor of amqp_socket_open()
 *
 * Looks up the hostname, then attempts to open a socket to the host using
 * the specified portnumber. It also sets various options on the socket to
 * improve performance and correctness.
 *
 * \param [in] hostname this can be a hostname or IP address. Both IPv4 and IPv6 are acceptable
 * \param [in] portnumber the port to connect on. RabbitMQ brokers listen on port 5672, and 5671 for SSL
 * \return a positive value indicates success and is the sockfd. A negative
 *  value (see amqp_status_enum)is returned on failure. Possible error codes:
 *  - AMQP_STATUS_TCP_SOCKETLIB_INIT_ERROR Initialization of underlying socket
 * library failed.
 *  - AMQP_STATUS_HOSTNAME_RESOLUTION_FAILED hostname lookup failed.
 *  - AMQP_STATUS_SOCKET_ERROR a socket error occurred. errno or
 * WSAGetLastError() may return more useful information.
 *
 * \note IPv6 support was added in v0.3
 * \sa amqp_socket_open() amqp_set_sockfd()
 * \since v0.1
 */
AMQP_PUBLIC_FUNCTION int amqp_open_socket(char const *hostname, int portnumber);
/**
 * Send initial AMQP header to the broker
 *
 * \warning this is a low level function intended for those who want to
 * interact with the broker at a very low level. Use of this function without
 * understanding what it does will result in AMQP protocol errors.
 *
 * This function sends the AMQP protocol header to the broker.
 *
 * \param [in] state the connection object
 * \return AMQP_STATUS_OK on success, a negative value on failure. Possible
 *  error codes:
 * - AMQP_STATUS_CONNECTION_CLOSED the connection to the broker was closed.
 * - AMQP_STATUS_SOCKET_ERROR a socket error occurred. It is likely the
 *   underlying socket has been closed. errno or WSAGetLastError() may provide
 *   further information.
 * - AMQP_STATUS_SSL_ERROR a SSL error occurred. The connection to the broker
 *   was closed.
 *
 * \since v0.1
 */
AMQP_PUBLIC_FUNCTION int amqp_send_header(amqp_connection_state_t state);
/**
 * Checks to see if there are any incoming frames ready to be read
 *
 * Checks to see if there are any amqp_frame_t objects buffered by the
 * amqp_connection_state_t object. Having one or more frames buffered means
 * that amqp_simple_wait_frame() or amqp_simple_wait_frame_noblock() will
 * return a frame without potentially blocking on a read() call.
 *
 * \param [in] state the connection object
 * \return TRUE if there are frames enqueued, FALSE otherwise
 *
 * \sa amqp_simple_wait_frame() amqp_simple_wait_frame_noblock()
 *  amqp_data_in_buffer()
 *
 * \since v0.1
 */
AMQP_PUBLIC_FUNCTION boolint FASTCALL amqp_frames_enqueued(const amqp_connection_state_t state);
/**
 * Read a single amqp_frame_t
 *
 * Waits for the next amqp_frame_t frame to be read from the broker.
 * This function has the potential to block for a long time in the case of
 * waiting for a basic.deliver method frame from the broker.
 *
 * The library may buffer frames. When an amqp_connection_state_t object
 * has frames buffered calling amqp_simple_wait_frame() will return an
 * amqp_frame_t without entering a blocking read(). You can test to see if
 * an amqp_connection_state_t object has frames buffered by calling the
 * amqp_frames_enqueued() function.
 *
 * The library has a socket read buffer. When there is data in an
 * amqp_connection_state_t read buffer, amqp_simple_wait_frame() may return an
 * amqp_frame_t without entering a blocking read(). You can test to see if an
 * amqp_connection_state_t object has data in its read buffer by calling the
 * amqp_data_in_buffer() function.
 *
 * \param [in] state the connection object
 * \param [out] decoded_frame the frame
 * \return AMQP_STATUS_OK on success, an amqp_status_enum value
 *  is returned otherwise. Possible errors include:
 *  - AMQP_STATUS_NO_MEMORY failure in allocating memory. The library is likely
 * in an indeterminate state making recovery unlikely. Client should note the
 * error and terminate the application
 *  - AMQP_STATUS_BAD_AMQP_DATA bad AMQP data was received. The connection
 * should be shutdown immediately
 *  - AMQP_STATUS_UNKNOWN_METHOD: an unknown method was received from the
 * broker. This is likely a protocol error and the connection should be
 * shutdown immediately
 *  - AMQP_STATUS_UNKNOWN_CLASS: a properties frame with an unknown class
 * was received from the broker. This is likely a protocol error and the
 * connection should be shutdown immediately
 *  - AMQP_STATUS_HEARTBEAT_TIMEOUT timed out while waiting for heartbeat
 * from the broker. The connection has been closed.
 *  - AMQP_STATUS_TIMER_FAILURE system timer indicated failure.
 *  - AMQP_STATUS_SOCKET_ERROR a socket error occurred. The connection has
 * been closed
 *  - AMQP_STATUS_SSL_ERROR a SSL socket error occurred. The connection has
 * been closed.
 *
 * \sa amqp_simple_wait_frame_noblock() amqp_frames_enqueued()
 *  amqp_data_in_buffer()
 *
 * \note as of v0.4.0 this function will no longer return heartbeat frames
 *  when enabled by specifying a non-zero heartbeat value in amqp_login().
 *  Heartbeating is handled internally by the library.
 *
 * \since v0.1
 */
AMQP_PUBLIC_FUNCTION int amqp_simple_wait_frame(amqp_connection_state_t state, amqp_frame_t *decoded_frame);
/**
 * Read a single amqp_frame_t with a timeout.
 *
 * Waits for the next amqp_frame_t frame to be read from the broker, up to
 * a timespan specified by tv. The function will return AMQP_STATUS_TIMEOUT
 * if the timeout is reached. The tv value is not modified by the function.
 *
 * If a 0 timeval is specified, the function behaves as if its non-blocking: it
 * will test to see if a frame can be read from the broker, and return
 * immediately.
 *
 * If NULL is passed in for tv, the function will behave like
 * amqp_simple_wait_frame() and block until a frame is received from the broker
 *
 * The library may buffer frames.  When an amqp_connection_state_t object
 * has frames buffered calling amqp_simple_wait_frame_noblock() will return an
 * amqp_frame_t without entering a blocking read(). You can test to see if an
 * amqp_connection_state_t object has frames buffered by calling the
 * amqp_frames_enqueued() function.
 *
 * The library has a socket read buffer. When there is data in an
 * amqp_connection_state_t read buffer, amqp_simple_wait_frame_noblock() may
 * return
 * an amqp_frame_t without entering a blocking read(). You can test to see if an
 * amqp_connection_state_t object has data in its read buffer by calling the
 * amqp_data_in_buffer() function.
 *
 * \note This function does not return heartbeat frames. When enabled,
 *  heartbeating is handed internally internally by the library.
 *
 * \param [in,out] state the connection object
 * \param [out] decoded_frame the frame
 * \param [in] tv the maximum time to wait for a frame to be read. Setting
 * tv->tv_sec = 0 and tv->tv_usec = 0 will do a non-blocking read. Specifying
 * NULL for tv will make the function block until a frame is read.
 * \return AMQP_STATUS_OK on success. An amqp_status_enum value is returned
 *  otherwise. Possible errors include:
 *  - AMQP_STATUS_TIMEOUT the timeout was reached while waiting for a frame
 * from the broker.
 *  - AMQP_STATUS_INVALID_PARAMETER the tv parameter contains an invalid value.
 *  - AMQP_STATUS_NO_MEMORY failure in allocating memory. The library is likely
 * in an indeterminate state making recovery unlikely. Client should note the
 * error and terminate the application
 *  - AMQP_STATUS_BAD_AMQP_DATA bad AMQP data was received. The connection
 * should be shutdown immediately
 *  - AMQP_STATUS_UNKNOWN_METHOD: an unknown method was received from the
 * broker. This is likely a protocol error and the connection should be
 * shutdown immediately
 *  - AMQP_STATUS_UNKNOWN_CLASS: a properties frame with an unknown class
 * was received from the broker. This is likely a protocol error and the
 * connection should be shutdown immediately
 *  - AMQP_STATUS_HEARTBEAT_TIMEOUT timed out while waiting for heartbeat
 * from the broker. The connection has been closed.
 *  - AMQP_STATUS_TIMER_FAILURE system timer indicated failure.
 *  - AMQP_STATUS_SOCKET_ERROR a socket error occurred. The connection has
 * been closed
 *  - AMQP_STATUS_SSL_ERROR a SSL socket error occurred. The connection has
 * been closed.
 *
 * \sa amqp_simple_wait_frame() amqp_frames_enqueued() amqp_data_in_buffer()
 *
 * \since v0.4.0
 */
AMQP_PUBLIC_FUNCTION int amqp_simple_wait_frame_noblock(amqp_connection_state_t state, amqp_frame_t *decoded_frame, struct timeval *tv);
/**
 * Waits for a specific method from the broker
 *
 * \warning You probably don't want to use this function. If this function
 *  doesn't receive exactly the frame requested it closes the whole connection.
 *
 * Waits for a single method on a channel from the broker.
 * If a frame is received that does not match expected_channel
 * or expected_method the program will abort
 *
 * \param [in] state the connection object
 * \param [in] expected_channel the channel that the method should be delivered
 *  on
 * \param [in] expected_method the method to wait for
 * \param [out] output the method
 * \returns AMQP_STATUS_OK on success. An amqp_status_enum value is returned
 *  otherwise. Possible errors include:
 *  - AMQP_STATUS_WRONG_METHOD a frame containing the wrong method, wrong frame
 * type or wrong channel was received. The connection is closed.
 *  - AMQP_STATUS_NO_MEMORY failure in allocating memory. The library is likely
 * in an indeterminate state making recovery unlikely. Client should note the
 * error and terminate the application
 *  - AMQP_STATUS_BAD_AMQP_DATA bad AMQP data was received. The connection
 * should be shutdown immediately
 *  - AMQP_STATUS_UNKNOWN_METHOD: an unknown method was received from the
 * broker. This is likely a protocol error and the connection should be
 * shutdown immediately
 *  - AMQP_STATUS_UNKNOWN_CLASS: a properties frame with an unknown class
 * was received from the broker. This is likely a protocol error and the
 * connection should be shutdown immediately
 *  - AMQP_STATUS_HEARTBEAT_TIMEOUT timed out while waiting for heartbeat
 * from the broker. The connection has been closed.
 *  - AMQP_STATUS_TIMER_FAILURE system timer indicated failure.
 *  - AMQP_STATUS_SOCKET_ERROR a socket error occurred. The connection has
 * been closed
 *  - AMQP_STATUS_SSL_ERROR a SSL socket error occurred. The connection has
 * been closed.
 *
 * \since v0.1
 */

AMQP_PUBLIC_FUNCTION int amqp_simple_wait_method(amqp_connection_state_t state, amqp_channel_t expected_channel, amqp_method_number_t expected_method, amqp_method_t *output);
/**
 * Sends a method to the broker
 *
 * This is a thin wrapper around amqp_send_frame(), providing a way to send
 * a method to the broker on a specified channel.
 *
 * \param [in] state the connection object
 * \param [in] channel the channel object
 * \param [in] id the method number
 * \param [in] decoded the method object
 * \returns AMQP_STATUS_OK on success, an amqp_status_enum value otherwise.
 *  Possible errors include:
 *  - AMQP_STATUS_BAD_AMQP_DATA the serialized form of the method or
 * properties was too large to fit in a single AMQP frame, or the
 * method contains an invalid value. The frame was not sent.
 *  - AMQP_STATUS_TABLE_TOO_BIG the serialized form of an amqp_table_t is
 * too large to fit in a single AMQP frame. Frame was not sent.
 *  - AMQP_STATUS_UNKNOWN_METHOD an invalid method type was passed in
 *  - AMQP_STATUS_UNKNOWN_CLASS an invalid properties type was passed in
 *  - AMQP_STATUS_TIMER_FAILURE system timer indicated failure. The frame
 * was sent
 *  - AMQP_STATUS_SOCKET_ERROR
 *  - AMQP_STATUS_SSL_ERROR
 *
 * \since v0.1
 */
AMQP_PUBLIC_FUNCTION int amqp_send_method(amqp_connection_state_t state, amqp_channel_t channel, amqp_method_number_t id, void *decoded);
/**
 * Sends a method to the broker and waits for a method response
 *
 * \param [in] state the connection object
 * \param [in] channel the channel object
 * \param [in] request_id the method number of the request
 * \param [in] expected_reply_ids a 0 terminated array of expected response
 *    method numbers
 * \param [in] decoded_request_method the method to be sent to the broker
 * \return a amqp_rpc_reply_t:
 *  - r.reply_type == AMQP_RESPONSE_NORMAL. RPC completed successfully
 *  - r.reply_type == AMQP_RESPONSE_SERVER_EXCEPTION. The broker returned an
 * exception:
 * - If r.reply.id == AMQP_CHANNEL_CLOSE_METHOD a channel exception
 *   occurred, cast r.reply.decoded to amqp_channel_close_t* to see details
 *   of the exception. The client should amqp_send_method() a
 *   amqp_channel_close_ok_t. The channel must be re-opened before it
 *   can be used again. Any resources associated with the channel
 *   (auto-delete exchanges, auto-delete queues, consumers) are invalid
 *   and must be recreated before attempting to use them again.
 * - If r.reply.id == AMQP_CONNECTION_CLOSE_METHOD a connection exception
 *   occurred, cast r.reply.decoded to amqp_connection_close_t* to see
 *   details of the exception. The client amqp_send_method() a
 *   amqp_connection_close_ok_t and disconnect from the broker.
 *  - r.reply_type == AMQP_RESPONSE_LIBRARY_EXCEPTION. An exception occurred
 * within the library. Examine r.library_error and compare it against
 * amqp_status_enum values to determine the error.
 *
 * \sa amqp_simple_rpc_decoded()
 *
 * \since v0.1
 */
AMQP_PUBLIC_FUNCTION amqp_rpc_reply_t amqp_simple_rpc(amqp_connection_state_t state, amqp_channel_t channel, 
	amqp_method_number_t request_id, amqp_method_number_t *expected_reply_ids, void *decoded_request_method);
/**
 * Sends a method to the broker and waits for a method response
 *
 * \param [in] state the connection object
 * \param [in] channel the channel object
 * \param [in] request_id the method number of the request
 * \param [in] reply_id the method number expected in response
 * \param [in] decoded_request_method the request method
 * \return a pointer to the method returned from the broker, or NULL on error.
 *  On error amqp_get_rpc_reply() will return an amqp_rpc_reply_t with
 *  details on the error that occurred.
 *
 * \since v0.1
 */
AMQP_PUBLIC_FUNCTION void * amqp_simple_rpc_decoded(amqp_connection_state_t state, amqp_channel_t channel, 
	amqp_method_number_t request_id, amqp_method_number_t reply_id, void *decoded_request_method);
/**
 * Get the last global amqp_rpc_reply
 *
 * The API methods corresponding to most synchronous AMQP methods
 * return a pointer to the decoded method result.  Upon error, they
 * return NULL, and we need some way of discovering what, if anything,
 * went wrong. amqp_get_rpc_reply() returns the most recent
 * amqp_rpc_reply_t instance corresponding to such an API operation
 * for the given connection.
 *
 * Only use it for operations that do not themselves return
 * amqp_rpc_reply_t; operations that do return amqp_rpc_reply_t
 * generally do NOT update this per-connection-global amqp_rpc_reply_t
 * instance.
 *
 * \param [in] state the connection object
 * \return the most recent amqp_rpc_reply_t:
 *  - r.reply_type == AMQP_RESPONSE_NORMAL. RPC completed successfully
 *  - r.reply_type == AMQP_RESPONSE_SERVER_EXCEPTION. The broker returned an
 * exception:
 * - If r.reply.id == AMQP_CHANNEL_CLOSE_METHOD a channel exception
 *   occurred, cast r.reply.decoded to amqp_channel_close_t* to see details
 *   of the exception. The client should amqp_send_method() a
 *   amqp_channel_close_ok_t. The channel must be re-opened before it
 *   can be used again. Any resources associated with the channel
 *   (auto-delete exchanges, auto-delete queues, consumers) are invalid
 *   and must be recreated before attempting to use them again.
 * - If r.reply.id == AMQP_CONNECTION_CLOSE_METHOD a connection exception
 *   occurred, cast r.reply.decoded to amqp_connection_close_t* to see
 *   details of the exception. The client amqp_send_method() a
 *   amqp_connection_close_ok_t and disconnect from the broker.
 *  - r.reply_type == AMQP_RESPONSE_LIBRARY_EXCEPTION. An exception occurred
 * within the library. Examine r.library_error and compare it against
 * amqp_status_enum values to determine the error.
 *
 * \sa amqp_simple_rpc_decoded()
 *
 * \since v0.1
 */
AMQP_PUBLIC_FUNCTION amqp_rpc_reply_t amqp_get_rpc_reply(const amqp_connection_state_t state);
/**
 * Login to the broker
 *
 * After using amqp_open_socket and amqp_set_sockfd, call
 * amqp_login to complete connecting to the broker
 *
 * \param [in] state the connection object
 * \param [in] vhost the virtual host to connect to on the broker. The default on most brokers is "/"
 * \param [in] channel_max the limit for number of channels for the connection.
 * 0 means no limit, and is a good default (AMQP_DEFAULT_MAX_CHANNELS)
 * Note that the maximum number of channels the protocol supports
 * is 65535 (2^16, with the 0-channel reserved). The server can
 * set a lower channel_max and then the client will use the lowest of the two
 * \param [in] frame_max the maximum size of an AMQP frame on the wire to
 *     request of the broker for this connection. 4096 is the minimum
 *     size, 2^31-1 is the maximum, a good default is 131072 (128KB),
 *     or AMQP_DEFAULT_FRAME_SIZE
 * \param [in] heartbeat the number of seconds between heartbeat frames to
 *     request of the broker. A value of 0 disables heartbeats.
 *     Note rabbitmq-c only has partial support for heartbeats, as of
 *     v0.4.0 they are only serviced during amqp_basic_publish() and
 *     amqp_simple_wait_frame()/amqp_simple_wait_frame_noblock()
 * \param [in] sasl_method the SASL method to authenticate with the broker.
 *     followed by the authentication information. The following SASL
 *     methods are implemented:
 *     -  AMQP_SASL_METHOD_PLAIN, the AMQP_SASL_METHOD_PLAIN argument
 *        should be followed by two arguments in this order:
 *        const char * username, and const char * password.
 *     -  AMQP_SASL_METHOD_EXTERNAL, the AMQP_SASL_METHOD_EXTERNAL
 *        argument should be followed one argument:
 *        const char * identity.
 * \return amqp_rpc_reply_t indicating success or failure.
 *  - r.reply_type == AMQP_RESPONSE_NORMAL. Login completed successfully
 *  - r.reply_type == AMQP_RESPONSE_LIBRARY_EXCEPTION. In most cases errors
 * from the broker when logging in will be represented by the broker closing
 * the socket. In this case r.library_error will be set to
 * AMQP_STATUS_CONNECTION_CLOSED. This error can represent a number of
 * error conditions including: invalid vhost, authentication failure.
 *  - r.reply_type == AMQP_RESPONSE_SERVER_EXCEPTION. The broker returned an
 * exception:
 * - If r.reply.id == AMQP_CHANNEL_CLOSE_METHOD a channel exception
 *   occurred, cast r.reply.decoded to amqp_channel_close_t* to see details
 *   of the exception. The client should amqp_send_method() a
 *   amqp_channel_close_ok_t. The channel must be re-opened before it
 *   can be used again. Any resources associated with the channel
 *   (auto-delete exchanges, auto-delete queues, consumers) are invalid
 *   and must be recreated before attempting to use them again.
 * - If r.reply.id == AMQP_CONNECTION_CLOSE_METHOD a connection exception
 *   occurred, cast r.reply.decoded to amqp_connection_close_t* to see
 *   details of the exception. The client amqp_send_method() a
 *   amqp_connection_close_ok_t and disconnect from the broker.
 *
 * \since v0.1
 */
AMQP_PUBLIC_FUNCTION amqp_rpc_reply_t amqp_login(amqp_connection_state_t state, char const * vhost, int channel_max, int frame_max, int heartbeat, 
	amqp_sasl_method_enum sasl_method, ...);
/**
 * Login to the broker passing a properties table
 *
 * This function is similar to amqp_login() and differs in that it provides a
 * way to pass client properties to the broker. This is commonly used to
 * negotiate newer protocol features as they are supported by the broker.
 *
 * \param [in] state the connection object
 * \param [in] vhost the virtual host to connect to on the broker. The default
 *     on most brokers is "/"
 * \param [in] channel_max the limit for the number of channels for the
 *    connection.
 *    0 means no limit, and is a good default
 *    (AMQP_DEFAULT_MAX_CHANNELS)
 *    Note that the maximum number of channels the protocol supports
 *    is 65535 (2^16, with the 0-channel reserved). The server can
 *    set a lower channel_max and then the client will use the lowest
 *    of the two
 * \param [in] frame_max the maximum size of an AMQP frame ont he wire to
 *     request of the broker for this connection. 4096 is the minimum
 *     size, 2^31-1 is the maximum, a good default is 131072 (128KB),
 *     or AMQP_DEFAULT_FRAME_SIZE
 * \param [in] heartbeat the number of seconds between heartbeat frame to
 *    request of the broker. A value of 0 disables heartbeats.
 *    Note rabbitmq-c only has partial support for hearts, as of
 *    v0.4.0 heartbeats are only serviced during amqp_basic_publish(),
 *    and amqp_simple_wait_frame()/amqp_simple_wait_frame_noblock()
 * \param [in] properties a table of properties to send the broker.
 * \param [in] sasl_method the SASL method to authenticate with the broker
 *    followed by the authentication information. The following SASL
 *    methods are implemented:
 *    -  AMQP_SASL_METHOD_PLAIN, the AMQP_SASL_METHOD_PLAIN argument
 *       should be followed by two arguments in this order:
 *       const char * username, and const char * password.
 *    -  AMQP_SASL_METHOD_EXTERNAL, the AMQP_SASL_METHOD_EXTERNAL
 *       argument should be followed one argument:
 *       const char * identity.
 * \return amqp_rpc_reply_t indicating success or failure.
 *  - r.reply_type == AMQP_RESPONSE_NORMAL. Login completed successfully
 *  - r.reply_type == AMQP_RESPONSE_LIBRARY_EXCEPTION. In most cases errors
 * from the broker when logging in will be represented by the broker closing
 * the socket. In this case r.library_error will be set to
 * AMQP_STATUS_CONNECTION_CLOSED. This error can represent a number of
 * error conditions including: invalid vhost, authentication failure.
 *  - r.reply_type == AMQP_RESPONSE_SERVER_EXCEPTION. The broker returned an
 * exception:
 * - If r.reply.id == AMQP_CHANNEL_CLOSE_METHOD a channel exception
 *   occurred, cast r.reply.decoded to amqp_channel_close_t* to see details
 *   of the exception. The client should amqp_send_method() a
 *   amqp_channel_close_ok_t. The channel must be re-opened before it
 *   can be used again. Any resources associated with the channel
 *   (auto-delete exchanges, auto-delete queues, consumers) are invalid
 *   and must be recreated before attempting to use them again.
 * - If r.reply.id == AMQP_CONNECTION_CLOSE_METHOD a connection exception
 *   occurred, cast r.reply.decoded to amqp_connection_close_t* to see
 *   details of the exception. The client amqp_send_method() a
 *   amqp_connection_close_ok_t and disconnect from the broker.
 *
 * \since v0.4.0
 */
AMQP_PUBLIC_FUNCTION amqp_rpc_reply_t amqp_login_with_properties(amqp_connection_state_t state, char const *vhost, int channel_max,
	int frame_max, int heartbeat, const amqp_table_t *properties, amqp_sasl_method_enum sasl_method, ...);
// 
// Descr: Publish a message to the broker
// Publish a message on an exchange with a routing key.
// Note that at the AMQ protocol level basic.publish is an async method:
// this means error conditions that occur on the broker (such as publishing to
// a non-existent exchange) will not be reflected in the return value of this function.
// 
// \param [in] state the connection object
// \param [in] channel the channel identifier
// \param [in] exchange the exchange on the broker to publish to
// \param [in] routing_key the routing key to use when publishing the message
// \param [in] mandatory indicate to the broker that the message MUST be routed
//   to a queue. If the broker cannot do this it should respond with a basic.return method.
// \param [in] immediate indicate to the broker that the message MUST be
//   delivered to a consumer immediately. If the broker cannot do this it should respond with a basic.return method.
// \param [in] properties the properties associated with the message
// \param [in] body the message body
// \return AMQP_STATUS_OK on success, amqp_status_enum value on failure. Note
//   that basic.publish is an async method, the return value from this
//   function only indicates that the message data was successfully
//   transmitted to the broker. It does not indicate failures that occur
//   on the broker, such as publishing to a non-existent exchange.
//   Possible error values:
//     - AMQP_STATUS_TIMER_FAILURE: system timer facility returned an error the message was not sent.
//     - AMQP_STATUS_HEARTBEAT_TIMEOUT: connection timed out waiting for a heartbeat from the broker. The message was not sent.
//     - AMQP_STATUS_NO_MEMORY: memory allocation failed. The message was not sent.
//     - AMQP_STATUS_TABLE_TOO_BIG: a table in the properties was too large to fit in a single frame. Message was not sent.
//     - AMQP_STATUS_CONNECTION_CLOSED: the connection was closed.
//     - AMQP_STATUS_SSL_ERROR: a SSL error occurred.
//     - AMQP_STATUS_TCP_ERROR: a TCP error occurred. errno or WSAGetLastError() may provide more information
// 
// Note: this function does heartbeat processing as of v0.4.0
// 
AMQP_PUBLIC_FUNCTION int amqp_basic_publish(amqp_connection_state_t state, amqp_channel_t channel, amqp_bytes_t exchange, 
	amqp_bytes_t routing_key, boolint mandatory, boolint immediate, amqp_basic_properties_t const * properties, amqp_bytes_t body);
/**
 * Closes an channel
 *
 * \param [in] state the connection object
 * \param [in] channel the channel identifier
 * \param [in] code the reason for closing the channel, AMQP_REPLY_SUCCESS is a
 *    good default
 * \return amqp_rpc_reply_t indicating success or failure
 *
 * \since v0.1
 */
AMQP_PUBLIC_FUNCTION amqp_rpc_reply_t amqp_channel_close(amqp_connection_state_t state, amqp_channel_t channel, int code);
/**
 * Closes the entire connection
 *
 * Implicitly closes all channels and informs the broker the connection
 * is being closed, after receiving acknowledgment from the broker it closes
 * the socket.
 *
 * \param [in] state the connection object
 * \param [in] code the reason code for closing the connection.
 *    AMQP_REPLY_SUCCESS is a good default.
 * \return amqp_rpc_reply_t indicating the result
 *
 * \since v0.1
 */
AMQP_PUBLIC_FUNCTION amqp_rpc_reply_t amqp_connection_close(amqp_connection_state_t state, int code);
/**
 * Acknowledges a message
 *
 * Does a basic.ack on a received message
 *
 * \param [in] state the connection object
 * \param [in] channel the channel identifier
 * \param [in] delivery_tag the delivery tag of the message to be ack'd
 * \param [in] multiple if true, ack all messages up to this delivery tag, if
 *     false ack only this delivery tag
 * \return 0 on success,  0 > on failing to send the ack to the broker.
 *   this will not indicate failure if something goes wrong on the
 *   broker
 *
 * \since v0.1
 */
AMQP_PUBLIC_FUNCTION int amqp_basic_ack(amqp_connection_state_t state, amqp_channel_t channel, uint64 delivery_tag, boolint multiple);
/**
 * Do a basic.get
 *
 * Synchonously polls the broker for a message in a queue, and
 * retrieves the message if a message is in the queue.
 *
 * \param [in] state the connection object
 * \param [in] channel the channel identifier to use
 * \param [in] queue the queue name to retrieve from
 * \param [in] no_ack if true the message is automatically ack'ed
 *     if false amqp_basic_ack should be called once the message
 *     retrieved has been processed
 * \return amqp_rpc_reply indicating success or failure
 *
 * \since v0.1
 */
AMQP_PUBLIC_FUNCTION amqp_rpc_reply_t amqp_basic_get(amqp_connection_state_t state, amqp_channel_t channel, amqp_bytes_t queue, boolint no_ack);
/**
 * Do a basic.reject
 *
 * Actively reject a message that has been delivered
 *
 * \param [in] state the connection object
 * \param [in] channel the channel identifier
 * \param [in] delivery_tag the delivery tag of the message to reject
 * \param [in] requeue indicate to the broker whether it should requeue the message or just discard it.
 * \return 0 on success, 0 > on failing to send the reject method to the broker.
 * This will not indicate failure if something goes wrong on the
 * broker.
 *
 * \since v0.1
 */
AMQP_PUBLIC_FUNCTION int amqp_basic_reject(amqp_connection_state_t state, amqp_channel_t channel, uint64 delivery_tag, boolint requeue);
/**
 * Do a basic.nack
 *
 * Actively reject a message, this has the same effect as amqp_basic_reject()
 * however, amqp_basic_nack() can negatively acknowledge multiple messages with
 * one call much like amqp_basic_ack() can acknowledge mutliple messages with
 * one call.
 *
 * \param [in] state the connection object
 * \param [in] channel the channel identifier
 * \param [in] delivery_tag the delivery tag of the message to reject
 * \param [in] multiple if set to 1 negatively acknowledge all unacknowledged
 *     messages on this channel.
 * \param [in] requeue indicate to the broker whether it should requeue the
 *     message or dead-letter it.
 * \return AMQP_STATUS_OK on success, an amqp_status_enum value otherwise.
 *
 * \since v0.5.0
 */
AMQP_PUBLIC_FUNCTION int amqp_basic_nack(amqp_connection_state_t state, amqp_channel_t channel, uint64 delivery_tag, boolint multiple, boolint requeue);
/**
 * Check to see if there is data left in the receive buffer
 *
 * Can be used to see if there is data still in the buffer, if so
 * calling amqp_simple_wait_frame will not immediately enter a
 * blocking read.
 *
 * \param [in] state the connection object
 * \return true if there is data in the recieve buffer, false otherwise
 *
 * \since v0.1
 */
AMQP_PUBLIC_FUNCTION boolint FASTCALL amqp_data_in_buffer(const amqp_connection_state_t state);
/**
 * Get the error string for the given error code.
 *
 * \deprecated This function has been deprecated in favor of \ref amqp_error_string2() which returns statically allocated
 *  string which do not need to be freed by the caller.
 *
 * The returned string resides on the heap; the caller is responsible
 * for freeing it.
 *
 * \param [in] err return error code
 * \return the error string
 *
 * \since v0.1
 */
AMQP_DEPRECATED(AMQP_PUBLIC_FUNCTION char * amqp_error_string(int err));
/**
 * Get the error string for the given error code.
 *
 * Get an error string associated with an error code. The string is statically
 * allocated and does not need to be freed
 *
 * \param [in] err the error code
 * \return the error string
 *
 * \since v0.4.0
 */
AMQP_PUBLIC_FUNCTION const char * amqp_error_string2(int err);
/**
 * Deserialize an amqp_table_t from AMQP wireformat
 *
 * This is an internal function and is not typically used by
 * client applications
 *
 * \param [in] encoded the buffer containing the serialized data
 * \param [in] pool memory pool used to allocate the table entries from
 * \param [in] output the amqp_table_t structure to fill in. Any existing
 *    entries will be erased
 * \param [in,out] offset The offset into the encoded buffer to start
 *        reading the serialized table. It will be updated
 *        by this function to end of the table
 * \return AMQP_STATUS_OK on success, an amqp_status_enum value on failure
 *  Possible error codes:
 *  - AMQP_STATUS_NO_MEMORY out of memory
 *  - AMQP_STATUS_BAD_AMQP_DATA invalid wireformat
 *
 * \since v0.1
 */
AMQP_PUBLIC_FUNCTION int amqp_decode_table(amqp_bytes_t encoded, amqp_pool_t *pool, amqp_table_t *output, size_t *offset);
/**
 * Serializes an amqp_table_t to the AMQP wireformat
 *
 * This is an internal function and is not typically used by
 * client applications
 *
 * \param [in] encoded the buffer where to serialize the table to
 * \param [in] input the amqp_table_t to serialize
 * \param [in,out] offset The offset into the encoded buffer to start
 *        writing the serialized table. It will be updated
 *        by this function to where writing left off
 * \return AMQP_STATUS_OK on success, an amqp_status_enum value on failure
 *  Possible error codes:
 *  - AMQP_STATUS_TABLE_TOO_BIG the serialized form is too large for the
 * buffer
 *  - AMQP_STATUS_BAD_AMQP_DATA invalid table
 *
 * \since v0.1
 */
AMQP_PUBLIC_FUNCTION int amqp_encode_table(amqp_bytes_t encoded, const amqp_table_t *input, size_t *offset);
/**
 * Create a deep-copy of an amqp_table_t object
 *
 * Creates a deep-copy of an amqp_table_t object, using the provided pool
 * object to allocate the necessary memory. This memory can be freed later by
 * call recycle_amqp_pool(), or empty_amqp_pool()
 *
 * \param [in] original the table to copy
 * \param [in,out] clone the table to copy to
 * \param [in] pool the initialized memory pool to do allocations for the table
 *    from
 * \return AMQP_STATUS_OK on success, amqp_status_enum value on failure.
 *  Possible error values:
 *  - AMQP_STATUS_NO_MEMORY - memory allocation failure.
 *  - AMQP_STATUS_INVALID_PARAMETER - invalid table (e.g., no key name)
 *
 * \since v0.4.0
 */
AMQP_PUBLIC_FUNCTION int amqp_table_clone(const amqp_table_t *original, amqp_table_t *clone, amqp_pool_t *pool);
// 
// Descr: A message object
// 
struct amqp_message_t {
	amqp_basic_properties_t properties; // message properties 
	amqp_bytes_t body; // message body 
	amqp_pool_t  pool; // pool used to allocate properties 
};
// 
// Descr: Reads the next message on a channel
// 
// Reads a complete message (header + body) on a specified channel. This
// function is intended to be used with amqp_basic_get() or when an AMQP_BASIC_DELIVERY_METHOD method is received.
// 
// \param [in,out] state the connection object
// \param [in] channel the channel on which to read the message from
// \param [in,out] message a pointer to a amqp_message_t object. Caller should
//    call amqp_message_destroy() when it is done using the
//    fields in the message object.  The caller is responsible for
//    allocating/destroying the amqp_message_t object itself.
// \param [in] flags pass in 0. Currently unused.
// \returns a amqp_rpc_reply_t object. ret.reply_type == AMQP_RESPONSE_NORMAL on success.
// 
AMQP_PUBLIC_FUNCTION amqp_rpc_reply_t amqp_read_message(amqp_connection_state_t state, amqp_channel_t channel, amqp_message_t *message, int flags);
// 
// Descr: Frees memory associated with a amqp_message_t allocated in amqp_read_message
// 
// \param [in] message
// 
AMQP_PUBLIC_FUNCTION void amqp_destroy_message(amqp_message_t *message);
// 
// Descr: Envelope object
// 
struct amqp_envelope_t {
	amqp_channel_t channel;    // channel message was delivered on 
	amqp_bytes_t consumer_tag; // the consumer tag the message was delivered to 
	uint64 delivery_tag;     // the messages delivery tag 
	boolint redelivered;       // flag indicating whether this message is being redelivered 
	amqp_bytes_t exchange;     // exchange this message was published to 
	amqp_bytes_t routing_key;  // the routing key this message was published with 
	amqp_message_t message;    // the message 
};
// 
// Descr: Wait for and consume a message
// 
// Waits for a basic.deliver method on any channel, upon receipt of
// basic.deliver it reads that message, and returns. If any other method is
// received before basic.deliver, this function will return an amqp_rpc_reply_t
// with ret.reply_type == AMQP_RESPONSE_LIBRARY_EXCEPTION, and
// ret.library_error == AMQP_STATUS_UNEXPECTED_STATE. The caller should then
// call amqp_simple_wait_frame() to read this frame and take appropriate action.
// 
// This function should be used after starting a consumer with the amqp_basic_consume() function
// 
// \param [in,out] state the connection object
// \param [in,out] envelope a pointer to a amqp_envelope_t object. Caller
//    should call #amqp_destroy_envelope() when it is done using
//    the fields in the envelope object. The caller is responsible
//    for allocating/destroying the amqp_envelope_t object itself.
// \param [in] timeout a timeout to wait for a message delivery. Passing in NULL will result in blocking behavior.
// \param [in] flags pass in 0. Currently unused.
// \returns a amqp_rpc_reply_t object.  ret.reply_type == AMQP_RESPONSE_NORMAL
//    on success. If ret.reply_type == AMQP_RESPONSE_LIBRARY_EXCEPTION,
//    and ret.library_error == AMQP_STATUS_UNEXPECTED_STATE, a frame other
//    than AMQP_BASIC_DELIVER_METHOD was received, the caller should call
//    amqp_simple_wait_frame() to read this frame and take appropriate action.
// 
AMQP_PUBLIC_FUNCTION amqp_rpc_reply_t amqp_consume_message(amqp_connection_state_t state, amqp_envelope_t *envelope, struct timeval *timeout, int flags);
// 
// Descr: Frees memory associated with a amqp_envelope_t allocated in amqp_consume_message()
// 
// \param [in] envelope
// 
AMQP_PUBLIC_FUNCTION void amqp_destroy_envelope(amqp_envelope_t *envelope);
// 
// Descr: Parameters used to connect to the RabbitMQ broker
// 
struct amqp_connection_info {
	const char * user; // the username to authenticate with the broker, default on most broker is 'guest' 
	const char * password; // the password to authenticate with the broker, default on most brokers is 'guest' 
	const char * host; // the hostname of the broker 
	const char * vhost; // the virtual host on the broker to connect to, a good default is "/" 
	int    port; // the port that the broker is listening on, default on most brokers is 5672 
	boolint ssl;
};
/**
 * Initialze an amqp_connection_info to default values
 *
 * The default values are:
 * - user: "guest"
 * - password: "guest"
 * - host: "localhost"
 * - vhost: "/"
 * - port: 5672
 *
 * \param [out] parsed the connection info to set defaults on
 *
 * \since v0.2
 */
AMQP_PUBLIC_FUNCTION void amqp_default_connection_info(struct amqp_connection_info *parsed);
/**
 * Parse a connection URL
 *
 * An amqp connection url takes the form:
 *
 * amqp://[$USERNAME[:$PASSWORD]\@]$HOST[:$PORT]/[$VHOST]
 *
 * Examples:
 *  amqp://guest:guest\@localhost:5672//
 *  amqp://guest:guest\@localhost/myvhost
 *
 *  Any missing parts of the URL will be set to the defaults specified in
 *  amqp_default_connection_info. For amqps: URLs the default port will be set
 *  to 5671 instead of 5672 for non-SSL URLs.
 *
 * \note This function modifies url parameter.
 *
 * \param [in] url URI to parse, note that this parameter is modified by the function.
 * \param [out] parsed the connection info gleaned from the URI. The char *
 *    members will point to parts of the url input parameter.
 *    Memory management will depend on how the url is allocated.
 * \returns AMQP_STATUS_OK on success, AMQP_STATUS_BAD_URL on failure
 *
 * \since v0.2
 */
AMQP_PUBLIC_FUNCTION int amqp_parse_url(char *url, struct amqp_connection_info *parsed);
/* socket API */

/**
 * Open a socket connection.
 *
 * This function opens a socket connection returned from amqp_tcp_socket_new()
 * or amqp_ssl_socket_new(). This function should be called after setting
 * socket options and prior to assigning the socket to an AMQP connection with
 * amqp_set_socket().
 *
 * \param [in,out] self A socket object.
 * \param [in] host Connect to this host.
 * \param [in] port Connect on this remote port.
 *
 * \return AMQP_STATUS_OK on success, an amqp_status_enum on failure
 *
 * \since v0.4.0
 */
AMQP_PUBLIC_FUNCTION int amqp_socket_open(amqp_socket_t *self, const char *host, int port);
/**
 * Open a socket connection.
 *
 * This function opens a socket connection returned from amqp_tcp_socket_new()
 * or amqp_ssl_socket_new(). This function should be called after setting
 * socket options and prior to assigning the socket to an AMQP connection with
 * amqp_set_socket().
 *
 * \param [in,out] self A socket object.
 * \param [in] host Connect to this host.
 * \param [in] port Connect on this remote port.
 * \param [in] timeout Max allowed time to spent on opening. If NULL - run in blocking mode
 *
 * \return AMQP_STATUS_OK on success, an amqp_status_enum on failure.
 *
 * \since v0.4.0
 */
AMQP_PUBLIC_FUNCTION int amqp_socket_open_noblock(amqp_socket_t *self, const char *host, int port, struct timeval *timeout);
/**
 * Get the socket descriptor in use by a socket object.
 *
 * Retrieve the underlying socket descriptor. This function can be used to
 * perform low-level socket operations that aren't supported by the socket
 * interface. Use with caution!
 *
 * \param [in,out] self A socket object.
 *
 * \return The underlying socket descriptor, or -1 if there is no socket
 *  descriptor associated with
 *
 * \since v0.4.0
 */
AMQP_PUBLIC_FUNCTION int amqp_socket_get_sockfd(amqp_socket_t *self);
/**
 * Get the socket object associated with a amqp_connection_state_t
 *
 * \param [in] state the connection object to get the socket from
 * \return a pointer to the socket object, or NULL if one has not been assigned
 *
 * \since v0.4.0
 */
AMQP_PUBLIC_FUNCTION amqp_socket_t * amqp_get_socket(amqp_connection_state_t state);
/**
 * Get the broker properties table
 *
 * \param [in] state the connection object
 * \return a pointer to an amqp_table_t containing the properties advertised
 *  by the broker on connection. The connection object owns the table, it
 *  should not be modified.
 *
 * \since v0.5.0
 */
AMQP_PUBLIC_FUNCTION amqp_table_t * amqp_get_server_properties(amqp_connection_state_t state);
/**
 * Get the client properties table
 *
 * Get the properties that were passed to the broker on connection.
 *
 * \param [in] state the connection object
 * \return a pointer to an amqp_table_t containing the properties advertised
 *  by the client on connection. The connection object owns the table, it
 *  should not be modified.
 *
 * \since v0.7.0
 */
AMQP_PUBLIC_FUNCTION amqp_table_t * amqp_get_client_properties(amqp_connection_state_t state);
/**
 * Get the login handshake timeout.
 *
 * amqp_login and amqp_login_with_properties perform the login handshake with
 * the broker.  This function returns the timeout associated with completing
 * this operation from the client side. This value can be set by using the
 * amqp_set_handshake_timeout.
 *
 * Note that the RabbitMQ broker has configurable timeout for completing the
 * login handshake, the default is 10 seconds.  rabbitmq-c has a default of 12
 * seconds.
 *
 * \param [in] state the connection object
 * \return a struct timeval representing the current login timeout for the state
 *  object. A NULL value represents an infinite timeout. The memory returned is
 *  owned by the connection object.
 *
 * \since v0.9.0
 */
AMQP_PUBLIC_FUNCTION const struct timeval * amqp_get_handshake_timeout(const amqp_connection_state_t state);
/**
 * Set the login handshake timeout.
 *
 * amqp_login and amqp_login_with_properties perform the login handshake with
 * the broker. This function sets the timeout associated with completing this
 * operation from the client side.
 *
 * The timeout must be set before amqp_login or amqp_login_with_properties is
 * called to change from the default timeout.
 *
 * Note that the RabbitMQ broker has a configurable timeout for completing the
 * login handshake, the default is 10 seconds. rabbitmq-c has a default of 12
 * seconds.
 *
 * \param [in] state the connection object
 * \param [in] timeout a struct timeval* representing new login timeout for the
 *  state object. NULL represents an infinite timeout. The value of timeout is
 *  copied internally, the caller is responsible for ownership of the passed in
 *  pointer, it does not need to remain valid after this function is called.
 * \return AMQP_STATUS_OK on success.
 *
 * \since v0.9.0
 */
AMQP_PUBLIC_FUNCTION int amqp_set_handshake_timeout(amqp_connection_state_t state, struct timeval *timeout);
/**
 * Get the RPC timeout
 *
 * Gets the timeout for any RPC-style AMQP command (e.g., amqp_queue_declare).
 * This timeout may be changed at any time by calling \amqp_set_rpc_timeout
 * function with a new timeout. The timeout applies individually to each RPC
 * that is made.
 *
 * The default value is NULL, or an infinite timeout.
 *
 * When an RPC times out, the function will return an error AMQP_STATUS_TIMEOUT,
 * and the connection will be closed.
 *
 *\warning RPC-timeouts are an advanced feature intended to be used to detect
 * dead connections quickly when the rabbitmq-c implementation of heartbeats
 * does not work. Do not use RPC timeouts unless you understand the implications
 * of doing so.
 *
 * \param [in] state the connection object
 * \return a struct timeval representing the current RPC timeout for the state
 * object. A NULL value represents an infinite timeout. The memory returned is
 * owned by the connection object.
 *
 * \since v0.9.0
 */
AMQP_PUBLIC_FUNCTION const struct timeval * amqp_get_rpc_timeout(const amqp_connection_state_t state);
/**
 * Set the RPC timeout
 *
 * Sets the timeout for any RPC-style AMQP command (e.g., amqp_queue_declare).
 * This timeout may be changed at any time by calling this function with a new
 * timeout. The timeout applies individually to each RPC that is made.
 *
 * The default value is NULL, or an infinite timeout.
 *
 * When an RPC times out, the function will return an error AMQP_STATUS_TIMEOUT,
 * and the connection will be closed.
 *
 *\warning RPC-timeouts are an advanced feature intended to be used to detect
 * dead connections quickly when the rabbitmq-c implementation of heartbeats
 * does not work. Do not use RPC timeouts unless you understand the implications
 * of doing so.
 *
 * \param [in] state the connection object
 * \param [in] timeout a struct timeval* representing new RPC timeout for the
 * state object. NULL represents an infinite timeout. The value of timeout is
 * copied internally, the caller is responsible for ownership of the passed
 * pointer, it does not need to remain valid after this function is called.
 * \return AMQP_STATUS_SUCCESS on success.
 *
 * \since v0.9.0
 */
AMQP_PUBLIC_FUNCTION int amqp_set_rpc_timeout(amqp_connection_state_t state, struct timeval *timeout);

AMQP_END_DECLS

//#include "amqp_time.h"
// amqp_time.h {
//#include <stdint.h>
#if defined(_WIN32) || defined(__MINGW32__) || defined(__MINGW64__)
	//
#else
	#include <sys/time.h>
#endif

#define AMQP_MS_PER_S  1000
#define AMQP_US_PER_MS 1000
#define AMQP_NS_PER_S  SlConst::OneBillion
#define AMQP_NS_PER_MS 1000000
#define AMQP_NS_PER_US 1000
// 
// This represents a point in time in reference to a monotonic clock.
// 
// The internal representation is ns, relative to the monotonic clock.
// 
// There are two 'special' values:
//   - 0: means 'this instant', its meant for polls with a 0-timeout, or
//     non-blocking option
//   - UINT64_MAX: means 'at infinity', its mean for polls with an infinite timeout
// 
struct amqp_time_t { 
	uint64 time_point_ns; 
};

uint64 amqp_get_monotonic_timestamp(); // Gets a monotonic timestamp. This will return 0 if the underlying call to the system fails.

/* Get a amqp_time_t that is timeout from now.
 * If timeout is NULL, an amqp_time_infinite() is created.
 * If timeout = {0, 0}, an amqp_time_immediate() is created.
 *
 * Returns AMQP_STATUS_OK on success.
 * AMQP_STATUS_INVALID_PARAMETER if timeout is invalid
 * AMQP_STATUS_TIMER_FAILURE if the underlying call to get the current timestamp
 * fails.
 */
int FASTCALL amqp_time_from_now(amqp_time_t *time, struct timeval *timeout);
// 
// Get a amqp_time_t that is seconds from now.
// If seconds <= 0, then amqp_time_infinite() is created.
// 
// Returns AMQP_STATUS_OK on success.
// AMQP_STATUS_TIMER_FAILURE if the underlying call to get the current timestamp fails.
// 
int    FASTCALL amqp_time_s_from_now(amqp_time_t *time, int seconds);
amqp_time_t amqp_time_immediate(); // Create an immediate amqp_time_t 
amqp_time_t amqp_time_infinite(); // Create an infinite amqp_time_t 
// 
// Gets the number of ms until the amqp_time_t, suitable for the timeout parameter in poll().
// 
// -1 will be returned for amqp_time_infinite values.
// 0 will be returned for amqp_time_immediate values.
// AMQP_STATUS_TIMEOUT will be returned if time was in the past.
// AMQP_STATUS_TIMER_FAILURE will be returned if the underlying call to get the current timestamp fails.
// 
int amqp_time_ms_until(amqp_time_t time);
// 
// Gets a timeval filled in with the time until amqp_time_t. Suitable for the parameter in select().
// 
// The in parameter specifies a storage location for *out.
// If time is an inf timeout, then *out = NULL.
// If time is a 0-timeout or the timer has expired, then *out = {0, 0}
// Otherwise *out is set to the time left on the time.
// 
// AMQP_STATUS_OK will be returned if successfully filled.
// AMQP_STATUS_TIMER_FAILURE is returned when the underlying call to get the current timestamp fails.
// 
int STDCALL amqp_time_tv_until(amqp_time_t time, struct timeval *in, struct timeval **out);
/* Test whether current time is past the provided time.
 *
 * TODO: this isn't a great interface to use. Fix this.
 *
 * Return AMQP_STATUS_OK if time has not past
 * Return AMQP_STATUS_TIMEOUT if time has past
 * Return AMQP_STATUS_TIMER_FAILURE if the underlying call to get the current
 * timestamp fails.
 */
int FASTCALL amqp_time_has_past(amqp_time_t time);
amqp_time_t FASTCALL amqp_time_first(amqp_time_t l, amqp_time_t r); // Return the time value that happens first 
int FASTCALL amqp_time_equal(amqp_time_t l, amqp_time_t r);
// } amqp_time.h
//#include "amqp_socket.h"
// amqp_socket.h {
AMQP_BEGIN_DECLS
	enum amqp_socket_flag_enum {
		AMQP_SF_NONE = 0,
		AMQP_SF_MORE = 1,
		AMQP_SF_POLLIN = 2,
		AMQP_SF_POLLOUT = 4,
		AMQP_SF_POLLERR = 8
	};

	enum amqp_socket_close_enum { 
		AMQP_SC_NONE = 0, 
		AMQP_SC_FORCE = 1 
	};

	int amqp_os_socket_error();
	int amqp_os_socket_close(int sockfd);

	/* Socket callbacks. */
	typedef ssize_t (* amqp_socket_send_fn)(void *, const void *, size_t, int);
	typedef ssize_t (* amqp_socket_recv_fn)(void *, void *, size_t, int);
	typedef int    (* amqp_socket_open_fn)(void *, const char *, int, struct timeval *);
	typedef int    (* amqp_socket_close_fn)(void *, amqp_socket_close_enum);
	typedef int    (* amqp_socket_get_sockfd_fn)(void *);
	typedef void   (* amqp_socket_delete_fn)(void *);

	/** V-table for amqp_socket_t */
	struct amqp_socket_class_t {
		amqp_socket_send_fn send;
		amqp_socket_recv_fn recv;
		amqp_socket_open_fn open;
		amqp_socket_close_fn close;
		amqp_socket_get_sockfd_fn get_sockfd;
		amqp_socket_delete_fn FnDelete;
	};
	//
	// Descr: Abstract base class for amqp_socket_t 
	//
	struct amqp_socket_t_ {
		const amqp_socket_class_t * klass;
	};
	/**
	 * Set set the socket object for a connection
	 *
	 * This assigns a socket object to the connection, closing and deleting any
	 * existing socket
	 *
	 * \param [in] state The connection object to add the socket to
	 * \param [in] socket The socket object to assign to the connection
	 */
	void amqp_set_socket(amqp_connection_state_t state, amqp_socket_t * socket);
	/**
	 * Send a message from a socket.
	 *
	 * This function wraps send(2) functionality.
	 *
	 * This function will only return on error, or when all of the bytes in buf
	 * have been sent, or when an error occurs.
	 *
	 * \param [in,out] self A socket object.
	 * \param [in] buf A buffer to read from.
	 * \param [in] len The number of bytes in \e buf.
	 * \param [in]
	 *
	 * \return AMQP_STATUS_OK on success. amqp_status_enum value otherwise
	 */
	ssize_t amqp_socket_send(amqp_socket_t * self, const void * buf, size_t len, int flags);
	ssize_t amqp_try_send(amqp_connection_state_t state, const void * buf, size_t len, amqp_time_t deadline, int flags);
	/**
	 * Receive a message from a socket.
	 *
	 * This function wraps recv(2) functionality.
	 *
	 * \param [in,out] self A socket object.
	 * \param [out] buf A buffer to write to.
	 * \param [in] len The number of bytes at \e buf.
	 * \param [in] flags Receive flags, implementation specific.
	 *
	 * \return The number of bytes received, or < 0 on error (\ref amqp_status_enum)
	 */
	ssize_t amqp_socket_recv(amqp_socket_t * self, void * buf, size_t len, int flags);
	/**
	 * Close a socket connection and free resources.
	 *
	 * This function closes a socket connection and releases any resources used by
	 * the object. After calling this function the specified socket should no
	 * longer be referenced.
	 *
	 * \param [in,out] self A socket object.
	 * \param [in] force, if set, just close the socket, don't attempt a TLS
	 * shutdown.
	 *
	 * \return Zero upon success, non-zero otherwise.
	 */
	int amqp_socket_close(amqp_socket_t * self, amqp_socket_close_enum force);
	/**
	 * Destroy a socket object
	 *
	 * \param [in] self the socket object to delete
	 */
	void amqp_socket_delete(amqp_socket_t * self);
	/**
	 * Open a socket connection.
	 *
	 * This function opens a socket connection returned from amqp_tcp_socket_new()
	 * or amqp_ssl_socket_new(). This function should be called after setting
	 * socket options and prior to assigning the socket to an AMQP connection with
	 * amqp_set_socket().
	 *
	 * \param [in] host Connect to this host.
	 * \param [in] port Connect on this remote port.
	 * \param [in] timeout Max allowed time to spent on opening. If NULL - run in
	 * blocking mode
	 *
	 * \return File descriptor upon success, non-zero negative error code otherwise.
	 */
	int amqp_open_socket_noblock(char const * hostname, int portnumber, struct timeval * timeout);
	int amqp_open_socket_inner(char const * hostname, int portnumber, amqp_time_t deadline);
	// Wait up to dealline for fd to become readable or writeable depending on
	// event (AMQP_SF_POLLIN, AMQP_SF_POLLOUT) 
	int STDCALL amqp_poll(int fd, int event, amqp_time_t deadline);
	int amqp_send_method_inner(amqp_connection_state_t state, amqp_channel_t channel, amqp_method_number_t id, void * decoded, int flags, amqp_time_t deadline);
	int amqp_queue_frame(amqp_connection_state_t state, amqp_frame_t * frame);
	int amqp_put_back_frame(amqp_connection_state_t state, amqp_frame_t * frame);
	int amqp_simple_wait_frame_on_channel(amqp_connection_state_t state, amqp_channel_t channel, amqp_frame_t * decoded_frame);
	int sasl_mechanism_in_list(amqp_bytes_t mechanisms, amqp_sasl_method_enum method);
	int amqp_merge_capabilities(const amqp_table_t * base, const amqp_table_t * add, amqp_table_t * result, amqp_pool_t * pool);
//AMQP_END_DECLS
// } amqp_socket.h
//#include "amqp_tcp_socket.h"
// amqp_tcp_socket.h {
//AMQP_BEGIN_DECLS
	/**
	 * Create a new TCP socket.
	 *
	 * Call amqp_connection_close() to release socket resources.
	 *
	 * \return A new socket object or NULL if an error occurred.
	 *
	 * \since v0.4.0
	 */
	AMQP_PUBLIC_FUNCTION amqp_socket_t * amqp_tcp_socket_new(amqp_connection_state_t state);
	/**
	 * Assign an open file descriptor to a socket object.
	 *
	 * This function must not be used in conjunction with amqp_socket_open(), i.e.
	 * the socket connection should already be open(2) when this function is
	 * called.
	 *
	 * \param [in,out] self A TCP socket object.
	 * \param [in] sockfd An open socket descriptor.
	 *
	 * \since v0.4.0
	 */
	AMQP_PUBLIC_FUNCTION void amqp_tcp_socket_set_sockfd(amqp_socket_t *self, int sockfd);
AMQP_END_DECLS
// } amqp_tcp_socket.h
//
// Descr: Транслирует коды статусов AMQP в коды ошибок SLIB.
//
int FASTCALL SlTranlateAmqpStatus(int amqpStatus);
//
// Descr: Проверяет статус AMQP на предмет ошибки. Если он сигнализирует об ошибке, то возвращает 0,
//   в противном случае !0.
//   В случае ошибки устанавливается код ошибки SLIB.
//
int FASTCALL SlCheckAmqpError(int amqpStatus, const char * pAddedMsg);

#endif /* AMQP_H */
