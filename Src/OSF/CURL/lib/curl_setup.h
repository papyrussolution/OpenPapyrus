#ifndef HEADER_CURL_SETUP_H
#define HEADER_CURL_SETUP_H
/***************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) 1998 - 2016, Daniel Stenberg, <daniel@haxx.se>, et al.
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

#include <slib.h> // @sobolev
/*
 * Define WIN32 when build target is Win32 API
 */
#if (defined(_WIN32) || defined(__WIN32__)) && !defined(WIN32) && !defined(__SYMBIAN32__)
	#define WIN32
#endif
// 
// Include configuration script results or hand-crafted
// configuration file for platforms which lack config tool.
// 
#define USE_LIBSSH2 // @sobolev
#define HAVE_LIBSSH2_H
// @sobolev #define LIBSSH2_VERSION_NUM 0x010801
#ifdef HAVE_CONFIG_H
	#include "curl_config.h"
#else /* HAVE_CONFIG_H */
	#ifdef _WIN32_WCE
		#include "config-win32ce.h"
	#else
		#ifdef WIN32
			#include "config-win32.h"
		#endif
	#endif
	#if defined(macintosh) && defined(__MRC__)
		#include "config-mac.h"
	#endif
	#ifdef __riscos__
		#include "config-riscos.h"
	#endif
	#ifdef __AMIGA__
		#include "config-amigaos.h"
	#endif
	#ifdef __SYMBIAN32__
		#include "config-symbian.h"
	#endif
	#ifdef __OS400__
		#include "config-os400.h"
	#endif
	#ifdef TPF
		#include "config-tpf.h"
	#endif
	#ifdef __VXWORKS__
		#include "config-vxworks.h"
	#endif
#endif /* HAVE_CONFIG_H */

/* ================================================================ */
/* Definition of preprocessor macros/symbols which modify compiler  */
/* behavior or generated code characteristics must be done here,   */
/* as appropriate, before any system header file is included. It is */
/* also possible to have them defined in the config file included   */
/* before this point. As a result of all this we frown inclusion of */
/* system header files in our config files, avoid this at any cost. */
/* ================================================================ */
/*
 * AIX 4.3 and newer needs _THREAD_SAFE defined to build
 * proper reentrant code. Others may also need it.
 */
#ifdef NEED_THREAD_SAFE
	#ifndef _THREAD_SAFE
		#define _THREAD_SAFE
	#endif
#endif
/*
 * Tru64 needs _REENTRANT set for a few function prototypes and
 * things to appear in the system header files. Unixware needs it
 * to build proper reentrant code. Others may also need it.
 */
#ifdef NEED_REENTRANT
	#ifndef _REENTRANT
		#define _REENTRANT
	#endif
#endif

/* Solaris needs this to get a POSIX-conformant getpwuid_r */
#if defined(sun) || defined(__sun)
	#ifndef _POSIX_PTHREAD_SEMANTICS
		#define _POSIX_PTHREAD_SEMANTICS 1
	#endif
#endif
/* ================================================================ */
/*  If you need to include a system header file for your platform,  */
/*  please, do it beyond the point further indicated in this file.  */
/* ================================================================ */
#include <curl/curl.h>
// 
// Ensure that no one is using the old SIZEOF_CURL_OFF_T macro
// 
#ifdef SIZEOF_CURL_OFF_T
#error "SIZEOF_CURL_OFF_T shall not be defined!"
   Error Compilation_aborted_SIZEOF_CURL_OFF_T_shall_not_be_defined
#endif
// 
// Disable other protocols when http is the only one desired.
// 
#ifdef HTTP_ONLY
	#ifndef CURL_DISABLE_TFTP
		#define CURL_DISABLE_TFTP
	#endif
	#ifndef CURL_DISABLE_FTP
		#define CURL_DISABLE_FTP
	#endif
	#ifndef CURL_DISABLE_LDAP
		#define CURL_DISABLE_LDAP
	#endif
	#ifndef CURL_DISABLE_TELNET
		#define CURL_DISABLE_TELNET
	#endif
	#ifndef CURL_DISABLE_DICT
		#define CURL_DISABLE_DICT
	#endif
	#ifndef CURL_DISABLE_FILE
		#define CURL_DISABLE_FILE
	#endif
	#ifndef CURL_DISABLE_RTSP
		#define CURL_DISABLE_RTSP
	#endif
	#ifndef CURL_DISABLE_POP3
		#define CURL_DISABLE_POP3
	#endif
	#ifndef CURL_DISABLE_IMAP
		#define CURL_DISABLE_IMAP
	#endif
	#ifndef CURL_DISABLE_SMTP
		#define CURL_DISABLE_SMTP
	#endif
	#ifndef CURL_DISABLE_RTMP
		#define CURL_DISABLE_RTMP
	#endif
	#ifndef CURL_DISABLE_GOPHER
		#define CURL_DISABLE_GOPHER
	#endif
	#ifndef CURL_DISABLE_SMB
		#define CURL_DISABLE_SMB
	#endif
#endif
/*
 * When http is disabled rtsp is not supported.
 */
#if defined(CURL_DISABLE_HTTP) && !defined(CURL_DISABLE_RTSP)
	#define CURL_DISABLE_RTSP
#endif
/* ================================================================ */
/* No system header file shall be included in this file before this */
/* point. The only allowed ones are those included from curlbuild.h */
/* ================================================================ */
/*
 * OS/400 setup file includes some system headers.
 */
#ifdef __OS400__
	#include "setup-os400.h"
#endif
 /*
 * VMS setup file includes some system headers.
 */
#ifdef __VMS
	#include "setup-vms.h"
#endif
/*
 * Use getaddrinfo to resolve the IPv4 address literal. If the current network
 * interface doesn’t support IPv4, but supports IPv6, NAT64, and DNS64,
 * performing this task will result in a synthesized IPv6 address.
 */
#ifdef  __APPLE__
	#define USE_RESOLVE_ON_IPS 1
#endif
 /*
 * Include header files for windows builds before redefining anything.
 * Use this preprocessor block only to include or exclude windows.h,
 * winsock2.h, ws2tcpip.h or winsock.h. Any other windows thing belongs
 * to any other further and independent block.  Under Cygwin things work
 * just as under linux (e.g. <sys/socket.h>) and the winsock headers should
 * never be included when __CYGWIN__ is defined.  configure script takes
 * care of this, not defining HAVE_WINDOWS_H, HAVE_WINSOCK_H, HAVE_WINSOCK2_H,
 * neither HAVE_WS2TCPIP_H when __CYGWIN__ is defined.
 */
#ifdef HAVE_WINDOWS_H
	#if defined(UNICODE) && !defined(_UNICODE)
		#define _UNICODE
	#endif
	#if defined(_UNICODE) && !defined(UNICODE)
		#define UNICODE
	#endif
	#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN
	#endif
	#include <windows.h>
	#ifdef HAVE_WINSOCK2_H
		#include <winsock2.h>
		#ifdef HAVE_WS2TCPIP_H
			#include <ws2tcpip.h>
		#endif
	#else
		#ifdef HAVE_WINSOCK_H
			#include <winsock.h>
		#endif
	#endif
	#include <tchar.h>
	#ifdef UNICODE
		 typedef wchar_t *(*curl_wcsdup_callback)(const wchar_t *str);
	#endif
#endif
/*
 * Define USE_WINSOCK to 2 if we have and use WINSOCK2 API, else
 * define USE_WINSOCK to 1 if we have and use WINSOCK  API, else
 * undefine USE_WINSOCK.
 */
#undef USE_WINSOCK
#ifdef HAVE_WINSOCK2_H
	#define USE_WINSOCK 2
#else
	#ifdef HAVE_WINSOCK_H
		#define USE_WINSOCK 1
	#endif
#endif
#ifdef USE_LWIPSOCK
	#include <lwip/init.h>
	#include <lwip/sockets.h>
	#include <lwip/netdb.h>
#endif
#ifdef HAVE_EXTRA_STRICMP_H
	#include <extra/stricmp.h>
#endif
#ifdef HAVE_EXTRA_STRDUP_H
	#include <extra/strdup.h>
#endif
#ifdef TPF
	#include <strings.h>    /* for bzero, strcasecmp, and strncasecmp */
	#include <string.h>     /* for strcpy and sstrlen */
	#include <stdlib.h>     /* for rand and srand */
	#include <sys/socket.h> /* for select and ioctl*/
	#include <netdb.h>      /* for in_addr_t definition */
	#include <tpf/sysapi.h> /* for tpf_process_signals */
	   /* change which select is used for libcurl */
	#define select(a,b,c,d,e) tpf_select_libcurl(a,b,c,d,e)
#endif
#ifdef __VXWORKS__
	#include <sockLib.h>    /* for generic BSD socket functions */
	#include <ioLib.h>      /* for basic I/O interface functions */
#endif
#ifdef __AMIGA__
	#ifndef __ixemul__
		#include <exec/types.h>
		#include <exec/execbase.h>
		#include <proto/exec.h>
		#include <proto/dos.h>
		#define select(a,b,c,d,e) WaitSelect(a,b,c,d,e,0)
	#endif
#endif
#include <stdio.h>
#ifdef HAVE_ASSERT_H
	#include <assert.h>
#endif
#ifdef __TANDEM /* for nsr-tandem-nsk systems */
	#include <floss.h>
#endif
#ifndef STDC_HEADERS /* no standard C headers! */
	#include <curl/stdcheaders.h>
#endif
#ifdef __POCC__
	#include <sys/types.h>
	#include <unistd.h>
	#define sys_nerr EILSEQ
#endif
/*
 * Salford-C kludge section (mostly borrowed from wxWidgets).
 */
#ifdef __SALFORDC__
	#pragma suppress 353             /* Possible nested comments */
	#pragma suppress 593             /* Define not used */
	#pragma suppress 61              /* enum has no name */
	#pragma suppress 106             /* unnamed, unused parameter */
	#include <clib.h>
#endif
/*
 * Large file (>2Gb) support using WIN32 functions.
 */
#ifdef USE_WIN32_LARGE_FILES
	#include <io.h>
	#include <sys/types.h>
	#include <sys/stat.h>
	#undef  lseek
	#define lseek(fdes,offset,whence)  _lseeki64(fdes, offset, whence)
	#undef  fstat
	#define fstat(fdes,stp)            _fstati64(fdes, stp)
	#undef  stat
	#define stat(fname,stp)            _stati64(fname, stp)
	#define struct_stat                struct _stati64
	#define LSEEK_ERROR                (__int64)-1
#endif
// 
// Small file (<2Gb) support using WIN32 functions.
// 
#ifdef USE_WIN32_SMALL_FILES
#include <io.h>
#include <sys/types.h>
#include <sys/stat.h>
#  ifndef _WIN32_WCE
#    undef  lseek
#    define lseek(fdes,offset,whence)  _lseek(fdes, (long)offset, whence)
#    define fstat(fdes,stp)            _fstat(fdes, stp)
#    define stat(fname,stp)            _stat(fname, stp)
#    define struct_stat                struct _stat
#  endif
#define LSEEK_ERROR                (long)-1
#endif
#ifndef struct_stat
	#define struct_stat struct stat
#endif
#ifndef LSEEK_ERROR
	#define LSEEK_ERROR (off_t)-1
#endif
/*
 * Default sizeof(off_t) in case it hasn't been defined in config file.
 */
#ifndef SIZEOF_OFF_T
#  if defined(__VMS) && !defined(__VAX)
#    if defined(_LARGEFILE)
#      define SIZEOF_OFF_T 8
#    endif
#  elif defined(__OS400__) && defined(__ILEC400__)
#    if defined(_LARGE_FILES)
#      define SIZEOF_OFF_T 8
#    endif
#  elif defined(__MVS__) && defined(__IBMC__)
#    if defined(_LP64) || defined(_LARGE_FILES)
#      define SIZEOF_OFF_T 8
#    endif
#  elif defined(__370__) && defined(__IBMC__)
#    if defined(_LP64) || defined(_LARGE_FILES)
#      define SIZEOF_OFF_T 8
#    endif
#  endif
#  ifndef SIZEOF_OFF_T
#    define SIZEOF_OFF_T 4
#  endif
#endif
/*
 * Arg 2 type for gethostname in case it hasn't been defined in config file.
 */
#ifndef GETHOSTNAME_TYPE_ARG2
	#ifdef USE_WINSOCK
		#define GETHOSTNAME_TYPE_ARG2 int
	#else
		#define GETHOSTNAME_TYPE_ARG2 size_t
	#endif
#endif
/* Below we define some functions. They should

   4. set the SIGALRM signal timeout
   5. set dir/file naming defines
   */
#ifdef WIN32
	#define DIR_CHAR      "\\"
	#define DOT_CHAR      "_"
#else /* WIN32 */
	#ifdef MSDOS  /* Watt-32 */
		#include <sys/ioctl.h>
		#define select(n,r,w,x,t) select_s(n,r,w,x,t)
		#define ioctl(x,y,z) ioctlsocket(x,y,(char *)(z))
		#include <tcp.h>
		#ifdef word
			#undef word
		#endif
		#ifdef byte
			#undef byte
		#endif
	#endif /* MSDOS */
	#ifdef __minix
		 /* Minix 3 versions up to at least 3.1.3 are missing these prototypes */
		 extern char *strtok_r(char *s, const char *delim, char **last);
		 extern struct tm *gmtime_r(const time_t * const timep, struct tm *tmp);
	#endif
	#define DIR_CHAR      "/"
	#ifndef DOT_CHAR
		#define DOT_CHAR      "."
	#endif
	#ifdef MSDOS
		#undef DOT_CHAR
		#define DOT_CHAR      "_"
	#endif
	#ifndef fileno /* sunos 4 have this as a macro! */
		 int fileno(FILE *stream);
	#endif
#endif /* WIN32 */
/*
 * msvc 6.0 requires PSDK in order to have INET6_ADDRSTRLEN
 * defined in ws2tcpip.h as well as to provide IPv6 support.
 * Does not apply if lwIP is used.
 */
#if defined(_MSC_VER) && !defined(__POCC__) && !defined(USE_LWIPSOCK)
#  if !defined(HAVE_WS2TCPIP_H) || ((_MSC_VER < 1300) && !defined(INET6_ADDRSTRLEN))
#    undef HAVE_GETADDRINFO_THREADSAFE
#    undef HAVE_FREEADDRINFO
#    undef HAVE_GETADDRINFO
#    undef HAVE_GETNAMEINFO
#    undef ENABLE_IPV6
#  endif
#endif

/* ---------------------------------------------------------------- */
/*             resolver specialty compile-time defines              */
/*         CURLRES_* defines to use in the host*.c sources          */
/* ---------------------------------------------------------------- */

/*
 * lcc-win32 doesn't have _beginthreadex(), lacks threads support.
 */
#if defined(__LCC__) && defined(WIN32)
	#undef USE_THREADS_POSIX
	#undef USE_THREADS_WIN32
#endif
/*
 * MSVC threads support requires a multi-threaded runtime library.
 * _beginthreadex() is not available in single-threaded ones.
 */
#if defined(_MSC_VER) && !defined(__POCC__) && !defined(_MT)
	#undef USE_THREADS_POSIX
	#undef USE_THREADS_WIN32
#endif
/*
 * Mutually exclusive CURLRES_* definitions.
 */
#ifdef USE_ARES
	#define CURLRES_ASYNCH
	#define CURLRES_ARES
	// now undef the stock libc functions just to avoid them being used 
	#undef HAVE_GETADDRINFO
	#undef HAVE_FREEADDRINFO
	#undef HAVE_GETHOSTBYNAME
#elif defined(USE_THREADS_POSIX) || defined(USE_THREADS_WIN32)
	#define CURLRES_ASYNCH
	#define CURLRES_THREADED
#else
	#define CURLRES_SYNCH
#endif
#ifdef ENABLE_IPV6
	#define CURLRES_IPV6
#else
	#define CURLRES_IPV4
#endif

/* ---------------------------------------------------------------- */

/*
 * When using WINSOCK, TELNET protocol requires WINSOCK2 API.
 */
#if defined(USE_WINSOCK) && (USE_WINSOCK != 2)
	#define CURL_DISABLE_TELNET 1
#endif
/*
 * msvc 6.0 does not have struct sockaddr_storage and
 * does not define IPPROTO_ESP in winsock2.h. But both
 * are available if PSDK is properly installed.
 */
#if defined(_MSC_VER) && !defined(__POCC__)
	#if !defined(HAVE_WINSOCK2_H) || ((_MSC_VER < 1300) && !defined(IPPROTO_ESP))
		#undef HAVE_STRUCT_SOCKADDR_STORAGE
	#endif
#endif
/*
 * Intentionally fail to build when using msvc 6.0 without PSDK installed.
 * The brave of heart can circumvent this, defining ALLOW_MSVC6_WITHOUT_PSDK
 * in lib/config-win32.h although absolutely discouraged and unsupported.
 */
#if defined(_MSC_VER) && !defined(__POCC__)
	#if !defined(HAVE_WINDOWS_H) || ((_MSC_VER < 1300) && !defined(_FILETIME_))
		#if !defined(ALLOW_MSVC6_WITHOUT_PSDK)
			#error MSVC 6.0 requires "February 2003 Platform SDK" a.k.a. "Windows Server 2003 PSDK"
		#else
			#define CURL_DISABLE_LDAP 1
		#endif
	#endif
#endif
#ifdef NETWARE
	int netware_init(void);
	#ifndef __NOVELL_LIBC__
		#include <sys/bsdskt.h>
		#include <sys/timeval.h>
	#endif
#endif
#if defined(HAVE_LIBIDN2) && defined(HAVE_IDN2_H)
	#define USE_LIBIDN2 // The lib and header are present 
#endif
#ifndef SIZEOF_TIME_T
	#define SIZEOF_TIME_T 4 // assume default size of time_t to be 32 bit 
#endif
#define LIBIDN_REQUIRED_VERSION "0.4.1"
#if defined(USE_GNUTLS) || defined(USE_OPENSSL) || defined(USE_NSS) || defined(USE_POLARSSL) || defined(USE_AXTLS) || defined(USE_MBEDTLS) || \
    defined(USE_CYASSL) || defined(USE_SCHANNEL) || defined(USE_DARWINSSL) || defined(USE_GSKIT)
	#define USE_SSL    /* SSL support has been enabled */
#endif
// Single point where USE_SPNEGO definition might be defined 
#if !defined(CURL_DISABLE_CRYPTO_AUTH) && (defined(HAVE_GSSAPI) || defined(USE_WINDOWS_SSPI))
	#define USE_SPNEGO
#endif
// Single point where USE_KERBEROS5 definition might be defined 
#if !defined(CURL_DISABLE_CRYPTO_AUTH) && (defined(HAVE_GSSAPI) || defined(USE_WINDOWS_SSPI))
	#define USE_KERBEROS5
#endif
// Single point where USE_NTLM definition might be defined 
#if !defined(CURL_DISABLE_NTLM) && !defined(CURL_DISABLE_CRYPTO_AUTH)
	#if defined(USE_OPENSSL) || defined(USE_WINDOWS_SSPI) || defined(USE_GNUTLS) || defined(USE_NSS) || defined(USE_DARWINSSL) || defined(USE_OS400CRYPTO) || defined(USE_WIN32_CRYPTO)
		#define USE_NTLM
	#elif defined(USE_MBEDTLS)
		#include <mbedtls/md4.h>
		#if defined(MBEDTLS_MD4_C)
			#define USE_NTLM
		#endif
	#endif
#endif
// non-configure builds may define CURL_WANTS_CA_BUNDLE_ENV 
#if defined(CURL_WANTS_CA_BUNDLE_ENV) && !defined(CURL_CA_BUNDLE)
	#define CURL_CA_BUNDLE getenv("CURL_CA_BUNDLE")
#endif
/*
 * Provide a mechanism to silence picky compilers, such as gcc 4.6+.
 * Parameters should of course normally not be unused, but for example when
 * we have multiple implementations of the same interface it may happen.
 */
#if defined(__GNUC__) && ((__GNUC__ >= 3) || ((__GNUC__ == 2) && defined(__GNUC_MINOR__) && (__GNUC_MINOR__ >= 7)))
	#define UNUSED_PARAM __attribute__((__unused__))
	#define WARN_UNUSED_RESULT __attribute__((warn_unused_result))
#else
	#define UNUSED_PARAM /*NOTHING*/
	#define WARN_UNUSED_RESULT
#endif
//
// Include macros and defines that should only be processed once.
//
#ifndef HEADER_CURL_SETUP_ONCE_H
	#include "curl_setup_once.h"
#endif
/*
 * Definition of our NOP statement Object-like macro
 */
#ifndef Curl_nop_stmt
	#define Curl_nop_stmt do { } WHILE_FALSE
#endif
/*
 * Ensure that Winsock and lwIP TCP/IP stacks are not mixed.
 */
#if defined(__LWIP_OPT_H__) || defined(LWIP_HDR_OPT_H)
	#if defined(SOCKET) || defined(USE_WINSOCK) || defined(HAVE_WINSOCK_H) || defined(HAVE_WINSOCK2_H) || defined(HAVE_WS2TCPIP_H)
		#error "Winsock and lwIP TCP/IP stack definitions shall not coexist!"
	#endif
#endif
/*
 * Portable symbolic names for Winsock shutdown() mode flags.
 */
#ifdef USE_WINSOCK
	#define SHUT_RD   0x00
	#define SHUT_WR   0x01
	#define SHUT_RDWR 0x02
#endif
// Define S_ISREG if not defined by system headers, f.e. MSVC 
#if !defined(S_ISREG) && defined(S_IFMT) && defined(S_IFREG)
	#define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#endif
// Define S_ISDIR if not defined by system headers, f.e. MSVC 
#if !defined(S_ISDIR) && defined(S_IFMT) && defined(S_IFDIR)
	#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#endif
// In Windows the default file mode is text but an application can override it.
// Therefore we specify it explicitly. https://github.com/curl/curl/pull/258
#if defined(WIN32) || defined(MSDOS)
	#define FOPEN_READTEXT "rt"
	#define FOPEN_WRITETEXT "wt"
#elif defined(__CYGWIN__)
	/* Cygwin has specific behavior we need to address when WIN32 is not defined.
	https://cygwin.com/cygwin-ug-net/using-textbinary.html
	For write we want our output to have line endings of LF and be compatible with
	other Cygwin utilities. For read we want to handle input that may have line
	endings either CRLF or LF so 't' is appropriate.
	*/
	#define FOPEN_READTEXT "rt"
	#define FOPEN_WRITETEXT "w"
#else
	#define FOPEN_READTEXT "r"
	#define FOPEN_WRITETEXT "w"
#endif
//
// WinSock destroys recv() buffer when send() failed.
// Enabled automatically for Windows and for Cygwin as Cygwin sockets are
// wrappers for WinSock sockets. https://github.com/curl/curl/issues/657
// Define DONT_USE_RECV_BEFORE_SEND_WORKAROUND to force disable workaround.
// 
#if !defined(DONT_USE_RECV_BEFORE_SEND_WORKAROUND)
	#if defined(WIN32) || defined(__CYGWIN__)
		#define USE_RECV_BEFORE_SEND_WORKAROUND
	#endif
#else  /* DONT_USE_RECV_BEFORE_SEND_WORKAROUNDS */
	#ifdef USE_RECV_BEFORE_SEND_WORKAROUND
		#undef USE_RECV_BEFORE_SEND_WORKAROUND
	#endif
#endif /* DONT_USE_RECV_BEFORE_SEND_WORKAROUNDS */
//
// Detect Windows App environment which has a restricted access to the Win32 APIs. 
//
#if defined(_WIN32_WINNT) && (_WIN32_WINNT >= 0x0602)
	#include <winapifamily.h>
	#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP) && !WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
		#define CURL_WINDOWS_APP
	#endif
#endif

// @sobolev {
#define BUILDING_CURL_SMB_C
#ifdef HAVE_SETJMP_H
	#include <setjmp.h>
#endif
#ifdef HAVE_LIMITS_H
	//#include <limits.h>
#endif
#ifdef HAVE_SYS_IOCTL_H
	#include <sys/ioctl.h>
#endif
#ifdef HAVE_FCNTL_H
	//#include <fcntl.h>
#endif
#ifdef HAVE_SIGNAL_H
	#include <signal.h>
#endif
#ifdef HAVE_PROCESS_H
	#include <process.h>
#endif
#include <stddef.h>
#include "warnless.h"
#include "llist.h"
#include "wildcard.h"
#include "strtok.h"
#include "strcase.h"
#include "strerror.h"
#include "curl_multibyte.h"
#include "curl_addrinfo.h"
#include "curl_gethostname.h"
#include "smb.h"
#include "fileinfo.h"
#include "getinfo.h"
#include "transfer.h"
#include "vtls/vtls.h"
#include "sendf.h"
#include "content_encoding.h"
#include "if2ip.h"
#include "non-ascii.h"
//
//#include "escape.h"
//
// Descr: Escape and unescape URL encoding in strings. The functions return a new allocated string or NULL if an error occurred.  
//
CURLcode FASTCALL Curl_urldecode(struct Curl_easy * data, const char * string, size_t length, char ** ostring, size_t * olen, bool reject_crlf);
//
//#include "file.h"
// 
// FILE unique setup
// 
struct FILEPROTO {
	char * path; // the path we operate on 
	char * freepath; // pointer to the allocated block we must free, this might differ from the 'path' pointer 
	int fd; // open file descriptor to read from! 
};

#ifndef CURL_DISABLE_FILE
	extern const struct Curl_handler Curl_handler_file;
#endif
//
//#include "strtoofft.h"
//
// 
// Determine which string to integral data type conversion function we use
// to implement string conversion to our curl_off_t integral data type.
// 
// Notice that curl_off_t might be 64 or 32 bit wide, and that it might use
// an underlying data type which might be 'long', 'int64_t', 'long long' or
// '__int64' and more remotely other data types.
// 
// On systems where the size of curl_off_t is greater than the size of 'long'
// the conversion function to use is strtoll() if it is available, otherwise,
// we emulate its functionality with our own clone.
// 
// On systems where the size of curl_off_t is smaller or equal than the size
// of 'long' the conversion function to use is strtol().
// 
#if (CURL_SIZEOF_CURL_OFF_T > CURL_SIZEOF_LONG)
	#ifdef HAVE_STRTOLL
		#define curlx_strtoofft strtoll
	#else
		#if defined(_MSC_VER) && (_MSC_VER >= 1300) && (_INTEGRAL_MAX_BITS >= 64)
			#if defined(_SAL_VERSION)
				_Check_return_ _CRTIMP __int64 __cdecl _strtoi64(_In_z_ const char *_String, _Out_opt_ _Deref_post_z_ char **_EndPtr, _In_ int _Radix);
			#else
				_CRTIMP __int64 __cdecl _strtoi64(const char *_String, char **_EndPtr, int _Radix);
			#endif
				#define curlx_strtoofft _strtoi64
		#else
			curl_off_t curlx_strtoll(const char *nptr, char **endptr, int base);
			#define curlx_strtoofft curlx_strtoll
			#define NEED_CURL_STRTOLL 1
		#endif
	#endif
#else
	#define curlx_strtoofft strtol
#endif
#if (CURL_SIZEOF_CURL_OFF_T == 4)
	#define CURL_OFF_T_MAX CURL_OFF_T_C(0x7FFFFFFF)
#else
	#define CURL_OFF_T_MAX CURL_OFF_T_C(0x7FFFFFFFFFFFFFFF) // assume CURL_SIZEOF_CURL_OFF_T == 8 
#endif
#define CURL_OFF_T_MIN (-CURL_OFF_T_MAX - CURL_OFF_T_C(1))
//
//#include "select.h"
//
#ifdef HAVE_POLL_H
	#include <poll.h>
#elif defined(HAVE_SYS_POLL_H)
	#include <sys/poll.h>
#endif
// 
// Definition of pollfd struct and constants for platforms lacking them.
// 
#if !defined(HAVE_STRUCT_POLLFD) && !defined(HAVE_SYS_POLL_H) && !defined(HAVE_POLL_H)
	#define POLLIN      0x01
	#define POLLPRI     0x02
	#define POLLOUT     0x04
	#define POLLERR     0x08
	#define POLLHUP     0x10
	#define POLLNVAL    0x20

	struct pollfd {
		curl_socket_t fd;
		short events;
		short revents;
	};
#endif
#ifndef POLLRDNORM
	#define POLLRDNORM POLLIN
#endif
#ifndef POLLWRNORM
	#define POLLWRNORM POLLOUT
#endif
#ifndef POLLRDBAND
	#define POLLRDBAND POLLPRI
#endif
// there are three CSELECT defines that are defined in the public header that
// are exposed to users, but this *IN2 bit is only ever used internally and
// therefore defined here 
#define CURL_CSELECT_IN2 (CURL_CSELECT_ERR << 1)

int Curl_socket_check(curl_socket_t readfd, curl_socket_t readfd2, curl_socket_t writefd, time_t timeout_ms);

#define SOCKET_READABLE(x, z) Curl_socket_check(x, CURL_SOCKET_BAD, CURL_SOCKET_BAD, z)
#define SOCKET_WRITABLE(x, z) Curl_socket_check(CURL_SOCKET_BAD, CURL_SOCKET_BAD, x, z)

int Curl_poll(struct pollfd ufds[], uint nfds, int timeout_ms);
// 
// On non-DOS and non-Winsock platforms, when Curl_ack_eintr is set,
// EINTR condition is honored and function might exit early without
// awaiting full timeout.  Otherwise EINTR will be ignored and full timeout will elapse. 
// 
extern int Curl_ack_eintr;

int Curl_wait_ms(int timeout_ms);

#ifdef TPF
	int tpf_select_libcurl(int maxfds, fd_set* reads, fd_set* writes, fd_set* excepts, struct timeval* tv);
#endif
// 
// Winsock and TPF sockets are not in range [0..FD_SETSIZE-1], which
// unfortunately makes it impossible for us to easily check if they're valid
// 
#if defined(USE_WINSOCK) || defined(TPF)
	#define VALID_SOCK(x) 1
	#define VERIFY_SOCK(x) Curl_nop_stmt
#else
	#define VALID_SOCK(s) (((s) >= 0) && ((s) < FD_SETSIZE))
	#define VERIFY_SOCK(x) do { if(!VALID_SOCK(x)) { SET_SOCKERRNO(EINVAL); return -1; } } WHILE_FALSE
#endif
//
//#include "url.h"
//
// Prototypes for library-wide functions provided by url.c
// 
CURLcode Curl_init_do(struct Curl_easy *data, struct connectdata *conn);
CURLcode Curl_open(struct Curl_easy **curl);
CURLcode Curl_init_userdefined(struct UserDefined *set);
CURLcode Curl_setopt(struct Curl_easy *data, CURLoption option, va_list arg);
CURLcode Curl_dupset(struct Curl_easy * dst, struct Curl_easy * src);
void Curl_freeset(struct Curl_easy * data);
CURLcode Curl_close(struct Curl_easy *data); /* opposite of curl_open() */
CURLcode Curl_connect(struct Curl_easy *, struct connectdata **, bool *async, bool *protocol_connect);
CURLcode FASTCALL Curl_disconnect(struct connectdata *, bool dead_connection);
CURLcode Curl_protocol_connect(struct connectdata *conn, bool *done);
CURLcode Curl_protocol_connecting(struct connectdata *conn, bool *done);
CURLcode Curl_protocol_doing(struct connectdata *conn, bool *done);
CURLcode Curl_setup_conn(struct connectdata *conn, bool *protocol_done);
void Curl_free_request_state(struct Curl_easy *data);
int Curl_protocol_getsock(struct connectdata *conn, curl_socket_t *socks, int numsocks);
int Curl_doing_getsock(struct connectdata *conn, curl_socket_t *socks, int numsocks);
bool Curl_isPipeliningEnabled(const struct Curl_easy *handle);
CURLcode Curl_addHandleToPipeline(struct Curl_easy *handle, struct curl_llist *pipeline);
int Curl_removeHandleFromPipeline(struct Curl_easy *handle, struct curl_llist *pipeline);
struct connectdata * Curl_oldest_idle_connection(struct Curl_easy *data);
// remove the specified connection from all (possible) pipelines and related queues 
void Curl_getoff_all_pipelines(struct Curl_easy *data, struct connectdata *conn);
void Curl_close_connections(struct Curl_easy *data);

#define CURL_DEFAULT_PROXY_PORT 1080 /* default proxy port unless specified */
#define CURL_DEFAULT_HTTPS_PROXY_PORT 443 /* default https proxy port unless specified */

CURLcode Curl_connected_proxy(struct connectdata *conn, int sockindex);

#ifdef CURL_DISABLE_VERBOSE_STRINGS
	#define Curl_verboseconnect(x)  Curl_nop_stmt
#else
	void Curl_verboseconnect(struct connectdata *conn);
#endif
#define CONNECT_PROXY_SSL()                 (conn->http_proxy.proxytype == CURLPROXY_HTTPS && !conn->bits.proxy_ssl_connected[sockindex])
#define CONNECT_FIRSTSOCKET_PROXY_SSL()     (conn->http_proxy.proxytype == CURLPROXY_HTTPS && !conn->bits.proxy_ssl_connected[FIRSTSOCKET])
#define CONNECT_SECONDARYSOCKET_PROXY_SSL() (conn->http_proxy.proxytype == CURLPROXY_HTTPS &&!conn->bits.proxy_ssl_connected[SECONDARYSOCKET])
//
//#include "progress.h"
//
typedef enum {
	TIMER_NONE,
	TIMER_STARTOP,
	TIMER_STARTSINGLE,
	TIMER_NAMELOOKUP,
	TIMER_CONNECT,
	TIMER_APPCONNECT,
	TIMER_PRETRANSFER,
	TIMER_STARTTRANSFER,
	TIMER_POSTRANSFER,
	TIMER_STARTACCEPT,
	TIMER_REDIRECT,
	TIMER_LAST /* must be last */
} timerid;

int Curl_pgrsDone(struct connectdata *);
void Curl_pgrsStartNow(struct Curl_easy * data);
void Curl_pgrsSetDownloadSize(struct Curl_easy * data, curl_off_t size);
void Curl_pgrsSetUploadSize(struct Curl_easy * data, curl_off_t size);
void Curl_pgrsSetDownloadCounter(struct Curl_easy * data, curl_off_t size);
void Curl_pgrsSetUploadCounter(struct Curl_easy * data, curl_off_t size);
int Curl_pgrsUpdate(struct connectdata *);
void Curl_pgrsResetTimesSizes(struct Curl_easy * data);
void Curl_pgrsTime(struct Curl_easy * data, timerid timer);
long Curl_pgrsLimitWaitTime(curl_off_t cursize, curl_off_t startsize, curl_off_t limit, struct timeval start, struct timeval now);

// Don't show progress for sizes smaller than: 
#define LEAST_SIZE_PROGRESS BUFSIZE

#define PROGRESS_DOWNLOAD (1<<0)
#define PROGRESS_UPLOAD   (1<<1)
#define PROGRESS_DOWN_AND_UP (PROGRESS_UPLOAD | PROGRESS_DOWNLOAD)

#define PGRS_SHOW_DL (1<<0)
#define PGRS_SHOW_UL (1<<1)
#define PGRS_DONE_DL (1<<2)
#define PGRS_DONE_UL (1<<3)
#define PGRS_HIDE    (1<<4)
#define PGRS_UL_SIZE_KNOWN (1<<5)
#define PGRS_DL_SIZE_KNOWN (1<<6)

#define PGRS_HEADERS_OUT (1<<7) /* set when the headers have been written */
//
#include "pingpong.h"
#include "curl_sasl.h"
#include "multiif.h"
#include "socks.h"
//
//#include "sockaddr.h"
//
struct Curl_sockaddr_storage {
	union {
		struct sockaddr sa;
		struct sockaddr_in sa_in;
#ifdef ENABLE_IPV6
		struct sockaddr_in6 sa_in6;
#endif
#ifdef HAVE_STRUCT_SOCKADDR_STORAGE
		struct sockaddr_storage sa_stor;
#else
		char cbuf[256]; /* this should be big enough to fit a lot */
#endif
	} buffer;
};
//
#include "timeval.h"  
//
//#include "nonblock.h"
//
int curlx_nonblock(curl_socket_t sockfd/* operate on this */, int nonblock/* TRUE or FALSE */);
//
#include "connect.h"
//
//#include "slist.h"
// 
// Curl_slist_duplicate() duplicates a linked list. It always returns the
// address of the first record of the cloned list or NULL in case of an
// error (or if the input list was NULL).
// 
struct curl_slist *Curl_slist_duplicate(struct curl_slist *inlist);
//
// Curl_slist_append_nodup() takes ownership of the given string and appends it to the list.
//
struct curl_slist *Curl_slist_append_nodup(struct curl_slist *list, char *data);
//
//#include "strdup.h"
//
#ifndef HAVE_STRDUP
	extern char *curlx_strdup(const char *str);
#endif
void * Curl_memdup(const void *src, size_t buffer_length);
void * Curl_saferealloc(void *ptr, size_t size);
//
#include "asyn.h"
#include "hostip.h"
#include "hash.h"
#include "share.h"
#include "conncache.h"
#include "multihandle.h"
#include "pipeline.h"
#include "formdata.h"
#include "http_chunks.h"
#include "http.h"
#include "http_proxy.h"
#include "ftp.h"
#include "rtsp.h"
#include "splay.h"
#include "imap.h"
#include "pop3.h"
#include "smtp.h"
#include "ssh.h"
#include "urldata.h"
#include "cookie.h"
#include "netrc.h"
#include "dict.h"
#include "dotdot.h"
#include "inet_pton.h"
#include "inet_ntop.h"
#include "rand.h"
#include "vauth/vauth.h"
//
//#include "vauth/digest.h"
//
#if !defined(CURL_DISABLE_CRYPTO_AUTH)
	#define DIGEST_MAX_VALUE_LENGTH           256
	#define DIGEST_MAX_CONTENT_LENGTH         1024

	enum {
		CURLDIGESTALGO_MD5,
		CURLDIGESTALGO_MD5SESS
	};
	//
	// This is used to extract the realm from a challenge message 
	//
	bool Curl_auth_digest_get_pair(const char * str, char * value, char * content, const char ** endptr);
#endif
//
//#include "parsedate.h"
//
CURLcode Curl_gmtime(time_t intime, struct tm *store);
//
//#include "speedcheck.h"
//
void Curl_speedinit(struct Curl_easy *data);
CURLcode Curl_speedcheck(struct Curl_easy *data, struct timeval now);
//
//#include "curl_base64.h"
//
CURLcode Curl_base64_encode(struct Curl_easy * data, const char * inputbuff, size_t insize, char ** outptr, size_t * outlen);
CURLcode Curl_base64url_encode(struct Curl_easy * data, const char * inputbuff, size_t insize, char ** outptr, size_t * outlen);
CURLcode Curl_base64_decode(const char * src, uchar ** outptr, size_t * outlen);
//
#ifndef CURL_DISABLE_CRYPTO_AUTH
	//
	//#include "curl_hmac.h"
	//
	typedef void (*HMAC_hinit_func)(void * context);
	typedef void (*HMAC_hupdate_func)(void * context, const uchar * data, uint len);
	typedef void (*HMAC_hfinal_func)(uchar * result, void * context);
	//
	// Per-hash function HMAC parameters
	//
	typedef struct {
		HMAC_hinit_func hmac_hinit;     /* Initialize context procedure. */
		HMAC_hupdate_func hmac_hupdate; /* Update context with data. */
		HMAC_hfinal_func hmac_hfinal;   /* Get final result procedure. */
		uint hmac_ctxtsize;     /* Context structure size. */
		uint hmac_maxkeylen;    /* Maximum key length (bytes). */
		uint hmac_resultlen;    /* Result length (bytes). */
	} HMAC_params;
	//
	// HMAC computation context
	//
	typedef struct {
		const HMAC_params * hmac_hash; /* Hash function definition. */
		void * hmac_hashctxt1;  /* Hash function context 1. */
		void * hmac_hashctxt2;  /* Hash function context 2. */
	} HMAC_context;
	//
	// Prototypes
	//
	HMAC_context * Curl_HMAC_init(const HMAC_params * hashparams, const uchar * key, uint keylen);
	int Curl_HMAC_update(HMAC_context * context, const uchar * data, uint len);
	int Curl_HMAC_final(HMAC_context * context, uchar * result);
	//
	// #include "curl_md5.h"
	//
	#define MD5_DIGEST_LEN  16

	typedef void (*Curl_MD5_init_func)(void * context);
	typedef void (*Curl_MD5_update_func)(void * context, const uchar * data, uint len);
	typedef void (*Curl_MD5_final_func)(uchar * result, void * context);

	typedef struct {
		Curl_MD5_init_func md5_init_func; /* Initialize context procedure */
		Curl_MD5_update_func md5_update_func; /* Update context with data */
		Curl_MD5_final_func md5_final_func; /* Get final result procedure */
		uint md5_ctxtsize;      /* Context structure size */
		uint md5_resultlen;     /* Result length (bytes) */
	} MD5_params;

	typedef struct {
		const MD5_params * md5_hash; /* Hash function definition */
		void * md5_hashctx; /* Hash function context */
	} MD5_context;

	extern const MD5_params Curl_DIGEST_MD5[1];
	extern const HMAC_params Curl_HMAC_MD5[1];

	void Curl_md5it(uchar * output, const uchar * input);
	MD5_context * Curl_MD5_init(const MD5_params * md5params);
	int Curl_MD5_update(MD5_context * context, const uchar * data, uint len);
	int Curl_MD5_final(MD5_context * context, uchar * result);
#endif
//
// #include "curl_memrchr.h"
//
#ifdef HAVE_MEMRCHR
	#ifdef HAVE_STRING_H
		#include <string.h>
	#endif
	#ifdef HAVE_STRINGS_H
		#include <strings.h>
	#endif
#else
	void * Curl_memrchr(const void *s, int c, size_t n);
	#define memrchr(x,y,z) Curl_memrchr((x),(y),(z))
#endif
//
// #include "curl_memory.h"
//
//
// Nasty internal details ahead...
//
// File curl_memory.h must be included by _all_ *.c source files
// that use memory related functions strdup, malloc, calloc, realloc
// or free, and given source file is used to build libcurl library.
// It should be included immediately before memdebug.h as the last files
// included to avoid undesired interaction with other memory function
// headers in dependent libraries.
//
// There is nearly no exception to above rule. All libcurl source
// files in 'lib' subdirectory as well as those living deep inside
// 'packages' subdirectories and linked together in order to build
// libcurl library shall follow it.
//
// File lib/strdup.c is an exception, given that it provides a strdup
// clone implementation while using malloc. Extra care needed inside
// this one. TODO: revisit this paragraph and related code.
//
// The need for curl_memory.h inclusion is due to libcurl's feature
// of allowing library user to provide memory replacement functions,
// memory callbacks, at runtime with curl_global_init_mem()
//
// Any *.c source file used to build libcurl library that does not
// include curl_memory.h and uses any memory function of the five
// mentioned above will compile without any indication, but it will
// trigger weird memory related issues at runtime.
//
// OTOH some source files from 'lib' subdirectory may additionally be
// used directly as source code when using some curlx_ functions by
// third party programs that don't even use libcurl at all. When using
// these source files in this way it is necessary these are compiled
// with CURLX_NO_MEMORY_CALLBACKS defined, in order to ensure that no
// attempt of calling libcurl's memory callbacks is done from code
// which can not use this machinery.
//
// Notice that libcurl's 'memory tracking' system works chaining into
// the memory callback machinery. This implies that when compiling
// 'lib' source files with CURLX_NO_MEMORY_CALLBACKS defined this file
// disengages usage of libcurl's 'memory tracking' system, defining
// MEMDEBUG_NODEFINES and overriding CURLDEBUG purpose.
//
// CURLX_NO_MEMORY_CALLBACKS takes precedence over CURLDEBUG. This is
// done in order to allow building a 'memory tracking' enabled libcurl
// and at the same time allow building programs which do not use it.
//
// Programs and libraries in 'tests' subdirectories have specific
// purposes and needs, and as such each one will use whatever fits
// best, depending additionally whether it links with libcurl or not.
//
// Caveat emptor. Proper curlx_* separation is a work in progress
// the same as CURLX_NO_MEMORY_CALLBACKS usage, some adjustments may
// still be required. IOW don't use them yet, there are sharp edges.
//
#ifdef HEADER_CURL_MEMDEBUG_H
	#error "Header memdebug.h shall not be included before curl_memory.h"
#endif
#ifndef CURLX_NO_MEMORY_CALLBACKS
	#ifndef CURL_DID_MEMORY_FUNC_TYPEDEFS /* only if not already done */
		// 
		// The following memory function replacement typedef's are COPIED from
		// curl/curl.h and MUST match the originals. We copy them to avoid having to
		// include curl/curl.h here. We avoid that include since it includes stdio.h
		// and other headers that may get messed up with defines done here.
		// 
		typedef void *(*curl_malloc_callback)(size_t size);
		typedef void (*curl_free_callback)(void *ptr);
		typedef void *(*curl_realloc_callback)(void *ptr, size_t size);
		typedef char *(*curl_strdup_callback)(const char *str);
		typedef void *(*curl_calloc_callback)(size_t nmemb, size_t size);
		#define CURL_DID_MEMORY_FUNC_TYPEDEFS
	#endif
	extern curl_malloc_callback Curl_cmalloc;
	extern curl_free_callback Curl_cfree;
	extern curl_realloc_callback Curl_crealloc;
	extern curl_strdup_callback Curl_cstrdup;
	extern curl_calloc_callback Curl_ccalloc;
	#if defined(WIN32) && defined(UNICODE)
		extern curl_wcsdup_callback Curl_cwcsdup;
	#endif
	#ifndef CURLDEBUG
		// 
		// libcurl's 'memory tracking' system defines strdup, malloc, calloc,
		// realloc and free, along with others, in memdebug.h in a different
		// way although still using memory callbacks forward declared above.
		// When using the 'memory tracking' system (CURLDEBUG defined) we do
		// not define here the five memory functions given that definitions
		// from memdebug.h are the ones that shall be used.
		// 
		// @sobolev #undef strdup
		// @sobolev #define _strdup(ptr) Curl_cstrdup(ptr)
		// @sobolev #undef malloc
		// @sobolev #define malloc Curl_cmalloc(size)
		// @sobolev #undef calloc
		// @sobolev #define calloc(nbelem,size) Curl_ccalloc(nbelem, size)
		// @sobolev #undef realloc
		// @sobolev #define realloc(ptr,size) Curl_crealloc(ptr, size)
		// @sobolev #undef free
		// @sobolev #define free Curl_cfree(ptr)
		#ifdef WIN32
			#ifdef UNICODE
				#undef wcsdup
				#define wcsdup(ptr) Curl_cwcsdup(ptr)
				#undef _wcsdup
				#define _wcsdup(ptr) Curl_cwcsdup(ptr)
				#undef _tcsdup
				#define _tcsdup(ptr) Curl_cwcsdup(ptr)
			#else
				#undef _tcsdup
				#define _tcsdup(ptr) Curl_cstrdup(ptr)
			#endif
		#endif
	#endif
#else
	#ifndef MEMDEBUG_NODEFINES
		#define MEMDEBUG_NODEFINES
	#endif
#endif
//
// } @sobolev 

#endif /* HEADER_CURL_SETUP_H */
