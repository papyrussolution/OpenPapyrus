#ifndef HEADER_CURL_SMB_H
#define HEADER_CURL_SMB_H
/***************************************************************************
*                                  _   _ ____  _
*  Project                     ___| | | |  _ \| |
*                             / __| | | | |_) | |
*                            | (__| |_| |  _ <| |___
*                             \___|\___/|_| \_\_____|
*
* Copyright (C) 2014, Bill Nagel <wnagel@tycoint.com>, Exacq Technologies
*
* This software is licensed as described in the file COPYING, which
* you should have received as part of this distribution. The terms
* are also available at https://curl.haxx.se/docs/copyright.html.
*
* You may opt to use, copy, modify, merge, publish, distribute and/or sell
* copies of the Software, and permit persons to whom the Software is
* furnished to do so, under the terms of the COPYING file.
*
* This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
* KIND, either express or implied.
*
***************************************************************************/

enum smb_conn_state {
	SMB_NOT_CONNECTED = 0,
	SMB_CONNECTING,
	SMB_NEGOTIATE,
	SMB_SETUP,
	SMB_CONNECTED
};

struct smb_conn {
	enum smb_conn_state state;
	char * user;
	char * domain;
	uchar challenge[8];
	uint session_key;
	ushort uid;
	char * recv_buf;
	size_t upload_size;
	size_t send_size;
	size_t sent;
	size_t got;
};

/*
 * Definitions for SMB protocol data structures
 */
#ifdef BUILDING_CURL_SMB_C

#if defined(_MSC_VER) || defined(__ILEC400__)
	#define PACK
	#pragma pack(push)
	#pragma pack(1)
#elif defined(__GNUC__)
	#define PACK __attribute__((packed))
#else
	#define PACK
#endif
#define SMB_COM_CLOSE                 0x04
#define SMB_COM_READ_ANDX             0x2e
#define SMB_COM_WRITE_ANDX            0x2f
#define SMB_COM_TREE_DISCONNECT       0x71
#define SMB_COM_NEGOTIATE             0x72
#define SMB_COM_SETUP_ANDX            0x73
#define SMB_COM_TREE_CONNECT_ANDX     0x75
#define SMB_COM_NT_CREATE_ANDX        0xa2
#define SMB_COM_NO_ANDX_COMMAND       0xff

#define SMB_WC_CLOSE                  0x03
#define SMB_WC_READ_ANDX              0x0c
#define SMB_WC_WRITE_ANDX             0x0e
#define SMB_WC_SETUP_ANDX             0x0d
#define SMB_WC_TREE_CONNECT_ANDX      0x04
#define SMB_WC_NT_CREATE_ANDX         0x18

#define SMB_FLAGS_CANONICAL_PATHNAMES 0x10
#define SMB_FLAGS_CASELESS_PATHNAMES  0x08
#define SMB_FLAGS2_UNICODE_STRINGS    0x8000
#define SMB_FLAGS2_IS_LONG_NAME       0x0040
#define SMB_FLAGS2_KNOWS_LONG_NAME    0x0001

#define SMB_CAP_LARGE_FILES           0x08
#define SMB_GENERIC_WRITE             0x40000000
#define SMB_GENERIC_READ              0x80000000
#define SMB_FILE_SHARE_ALL            0x07
#define SMB_FILE_OPEN                 0x01
#define SMB_FILE_OVERWRITE_IF         0x05

#define SMB_ERR_NOACCESS              0x00050001

struct smb_header {
	uchar nbt_type;
	uchar nbt_flags;
	ushort nbt_length;
	uchar magic[4];
	uchar command;
	uint status;
	uchar flags;
	ushort flags2;
	ushort pid_high;
	uchar signature[8];
	ushort pad;
	ushort tid;
	ushort pid;
	ushort uid;
	ushort mid;
} PACK;

struct smb_negotiate_response {
	struct smb_header h;
	uchar word_count;
	ushort dialect_index;
	uchar security_mode;
	ushort max_mpx_count;
	ushort max_number_vcs;
	uint max_buffer_size;
	uint max_raw_size;
	uint session_key;
	uint capabilities;
	uint system_time_low;
	uint system_time_high;
	ushort server_time_zone;
	uchar encryption_key_length;
	ushort byte_count;
	char bytes[1];
} PACK;

struct andx {
	uchar command;
	uchar pad;
	ushort offset;
} PACK;

struct smb_setup {
	uchar word_count;
	struct andx andx;

	ushort max_buffer_size;
	ushort max_mpx_count;
	ushort vc_number;
	uint session_key;
	ushort lengths[2];
	uint pad;
	uint capabilities;
	ushort byte_count;
	char bytes[1024];
} PACK;

struct smb_tree_connect {
	uchar word_count;
	struct andx andx;

	ushort flags;
	ushort pw_len;
	ushort byte_count;
	char bytes[1024];
} PACK;

struct smb_nt_create {
	uchar word_count;
	struct andx andx;

	uchar pad;
	ushort name_length;
	uint flags;
	uint root_fid;
	uint access;
#ifdef HAVE_LONGLONG
	uint64 allocation_size;
#else
	uint64 allocation_size;
#endif
	uint ext_file_attributes;
	uint share_access;
	uint create_disposition;
	uint create_options;
	uint impersonation_level;
	uchar security_flags;
	ushort byte_count;
	char bytes[1024];
} PACK;

struct smb_nt_create_response {
	struct smb_header h;
	uchar word_count;
	struct andx andx;
	uchar op_lock_level;
	ushort fid;
	uint create_disposition;
#ifdef HAVE_LONGLONG
	uint64 create_time;
	uint64 last_access_time;
	uint64 last_write_time;
	uint64 last_change_time;
#else
	uint64 create_time;
	uint64 last_access_time;
	uint64 last_write_time;
	uint64 last_change_time;
#endif
	uint ext_file_attributes;
#ifdef HAVE_LONGLONG
	uint64 allocation_size;
	uint64 end_of_file;
#else
	uint64 allocation_size;
	uint64 end_of_file;
#endif
} PACK;

struct smb_read {
	uchar word_count;
	struct andx andx;
	ushort fid;
	uint offset;
	ushort max_bytes;
	ushort min_bytes;
	uint timeout;
	ushort remaining;
	uint offset_high;
	ushort byte_count;
} PACK;

struct smb_write {
	struct smb_header h;

	uchar word_count;
	struct andx andx;

	ushort fid;
	uint offset;
	uint timeout;
	ushort write_mode;
	ushort remaining;
	ushort pad;
	ushort data_length;
	ushort data_offset;
	uint offset_high;
	ushort byte_count;
	uchar pad2;
} PACK;

struct smb_close {
	uchar word_count;
	ushort fid;
	uint last_mtime;
	ushort byte_count;
} PACK;

struct smb_tree_disconnect {
	uchar word_count;
	ushort byte_count;
} PACK;

#if defined(_MSC_VER) || defined(__ILEC400__)
	#pragma pack(pop)
#endif

#endif /* BUILDING_CURL_SMB_C */

#if !defined(CURL_DISABLE_SMB) && defined(USE_NTLM) && (CURL_SIZEOF_CURL_OFF_T > 4)
	#if !defined(USE_WINDOWS_SSPI) || defined(USE_WIN32_CRYPTO)
		extern const struct Curl_handler Curl_handler_smb;
		extern const struct Curl_handler Curl_handler_smbs;
	#endif /* !USE_WINDOWS_SSPI || USE_WIN32_CRYPTO */
#endif /* CURL_DISABLE_SMB && USE_NTLM && CURL_SIZEOF_CURL_OFF_T > 4 */

#endif /* HEADER_CURL_SMB_H */
