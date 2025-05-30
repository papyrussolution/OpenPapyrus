/***************************************************************************
*                                  _   _ ____  _
*  Project                     ___| | | |  _ \| |
*                             / __| | | | |_) | |
*                            | (__| |_| |  _ <| |___
*                             \___|\___/|_| \_\_____|
*
* Copyright (C) Daniel Stenberg, <daniel@haxx.se>, et al.
*
* This software is licensed as described in the file COPYING, which
* you should have received as part of this distribution. The terms
* are also available at https://curl.se/docs/copyright.html.
*
* You may opt to use, copy, modify, merge, publish, distribute and/or sell
* copies of the Software, and permit persons to whom the Software is
* furnished to do so, under the terms of the COPYING file.
*
* This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
* KIND, either express or implied.
*
* SPDX-License-Identifier: curl
*
***************************************************************************/

#include "curl_setup.h"
#pragma hdrstop

#if defined(__INTEL_COMPILER) && defined(__unix__)
	//#ifdef HAVE_NETINET_IN_H
		//#include <netinet/in.h>
	//#endif
	//#ifdef HAVE_ARPA_INET_H
		//#include <arpa/inet.h>
	//#endif
#endif /* __INTEL_COMPILER && __unix__ */
//#include "warnless.h"

#ifdef WIN32
	#undef read
	#undef write
#endif
//#include <limits.h>
#define CURL_MASK_UCHAR   ((uchar)~0)
#define CURL_MASK_SCHAR   (CURL_MASK_UCHAR >> 1)

#define CURL_MASK_USHORT  ((ushort)~0)
#define CURL_MASK_SSHORT  (CURL_MASK_USHORT >> 1)

#define CURL_MASK_UINT    ((uint)~0)
#define CURL_MASK_SINT    (CURL_MASK_UINT >> 1)

#define CURL_MASK_ULONG   ((ulong)~0)
#define CURL_MASK_SLONG   (CURL_MASK_ULONG >> 1)

#define CURL_MASK_UCOFFT  ((unsigned CURL_TYPEOF_CURL_OFF_T)~0)
#define CURL_MASK_SCOFFT  (CURL_MASK_UCOFFT >> 1)

#define CURL_MASK_USIZE_T ((size_t)~0)
#define CURL_MASK_SSIZE_T (CURL_MASK_USIZE_T >> 1)

/*
** unsigned long to unsigned short
*/
ushort curlx_ultous(ulong ulnum)
{
#ifdef __INTEL_COMPILER
#  pragma warning(push)
#  pragma warning(disable:810) /* conversion may lose significant bits */
#endif

	assert(ulnum <= (ulong)CURL_MASK_USHORT);
	return (ushort)(ulnum & (ulong)CURL_MASK_USHORT);

#ifdef __INTEL_COMPILER
#  pragma warning(pop)
#endif
}
/*
** unsigned long to uchar
*/
uchar curlx_ultouc(ulong ulnum)
{
#ifdef __INTEL_COMPILER
#  pragma warning(push)
#  pragma warning(disable:810) /* conversion may lose significant bits */
#endif

	assert(ulnum <= (ulong)CURL_MASK_UCHAR);
	return (uchar)(ulnum & (ulong)CURL_MASK_UCHAR);

#ifdef __INTEL_COMPILER
#  pragma warning(pop)
#endif
}
/*
** unsigned size_t to signed curl_off_t
*/
curl_off_t curlx_uztoso(size_t uznum)
{
#ifdef __INTEL_COMPILER
#  pragma warning(push)
#  pragma warning(disable:810) /* conversion may lose significant bits */
#elif defined(_MSC_VER)
#  pragma warning(push)
#  pragma warning(disable:4310) /* cast truncates constant value */
#endif

	assert(uznum <= (size_t)CURL_MASK_SCOFFT);
	return (curl_off_t)(uznum & (size_t)CURL_MASK_SCOFFT);

#if defined(__INTEL_COMPILER) || defined(_MSC_VER)
#  pragma warning(pop)
#endif
}
/*
** unsigned size_t to signed int
*/
int curlx_uztosi(size_t uznum)
{
#ifdef __INTEL_COMPILER
#  pragma warning(push)
#  pragma warning(disable:810) /* conversion may lose significant bits */
#endif

	assert(uznum <= (size_t)CURL_MASK_SINT);
	return (int)(uznum & (size_t)CURL_MASK_SINT);

#ifdef __INTEL_COMPILER
#  pragma warning(pop)
#endif
}
/*
** unsigned size_t to unsigned long
*/
ulong curlx_uztoul(size_t uznum)
{
#ifdef __INTEL_COMPILER
# pragma warning(push)
# pragma warning(disable:810) /* conversion may lose significant bits */
#endif

#if ULONG_MAX < SIZE_T_MAX
	assert(uznum <= (size_t)CURL_MASK_ULONG);
#endif
	return (ulong)(uznum & (size_t)CURL_MASK_ULONG);

#ifdef __INTEL_COMPILER
# pragma warning(pop)
#endif
}
/*
** unsigned size_t to uint
*/
uint curlx_uztoui(size_t uznum)
{
#ifdef __INTEL_COMPILER
# pragma warning(push)
# pragma warning(disable:810) /* conversion may lose significant bits */
#endif

#if UINT_MAX < SIZE_T_MAX
	assert(uznum <= (size_t)CURL_MASK_UINT);
#endif
	return (uint)(uznum & (size_t)CURL_MASK_UINT);

#ifdef __INTEL_COMPILER
# pragma warning(pop)
#endif
}

/*
** signed long to signed int
*/

int curlx_sltosi(long slnum)
{
#ifdef __INTEL_COMPILER
#  pragma warning(push)
#  pragma warning(disable:810) /* conversion may lose significant bits */
#endif

	assert(slnum >= 0);
#if INT_MAX < LONG_MAX
	assert((ulong)slnum <= (ulong)CURL_MASK_SINT);
#endif
	return (int)(slnum & (long)CURL_MASK_SINT);

#ifdef __INTEL_COMPILER
#  pragma warning(pop)
#endif
}

/*
** signed long to uint
*/

uint curlx_sltoui(long slnum)
{
#ifdef __INTEL_COMPILER
#  pragma warning(push)
#  pragma warning(disable:810) /* conversion may lose significant bits */
#endif

	assert(slnum >= 0);
#if UINT_MAX < LONG_MAX
	assert((ulong)slnum <= (ulong)CURL_MASK_UINT);
#endif
	return (uint)(slnum & (long)CURL_MASK_UINT);

#ifdef __INTEL_COMPILER
#  pragma warning(pop)
#endif
}
/*
** signed long to unsigned short
*/
ushort curlx_sltous(long slnum)
{
#ifdef __INTEL_COMPILER
#  pragma warning(push)
#  pragma warning(disable:810) /* conversion may lose significant bits */
#endif

	assert(slnum >= 0);
	assert((ulong)slnum <= (ulong)CURL_MASK_USHORT);
	return (ushort)(slnum & (long)CURL_MASK_USHORT);

#ifdef __INTEL_COMPILER
#  pragma warning(pop)
#endif
}
/*
** unsigned size_t to signed ssize_t
*/
ssize_t curlx_uztosz(size_t uznum)
{
#ifdef __INTEL_COMPILER
#  pragma warning(push)
#  pragma warning(disable:810) /* conversion may lose significant bits */
#endif

	assert(uznum <= (size_t)CURL_MASK_SSIZE_T);
	return (ssize_t)(uznum & (size_t)CURL_MASK_SSIZE_T);

#ifdef __INTEL_COMPILER
#  pragma warning(pop)
#endif
}
/*
** signed curl_off_t to unsigned size_t
*/
size_t curlx_sotouz(curl_off_t sonum)
{
#ifdef __INTEL_COMPILER
#  pragma warning(push)
#  pragma warning(disable:810) /* conversion may lose significant bits */
#endif

	assert(sonum >= 0);
	return (size_t)(sonum & (curl_off_t)CURL_MASK_USIZE_T);

#ifdef __INTEL_COMPILER
#  pragma warning(pop)
#endif
}

/*
** signed ssize_t to signed int
*/

int curlx_sztosi(ssize_t sznum)
{
#ifdef __INTEL_COMPILER
#  pragma warning(push)
#  pragma warning(disable:810) /* conversion may lose significant bits */
#endif

	assert(sznum >= 0);
#if INT_MAX < SSIZE_T_MAX
	assert((size_t)sznum <= (size_t)CURL_MASK_SINT);
#endif
	return (int)(sznum & (ssize_t)CURL_MASK_SINT);

#ifdef __INTEL_COMPILER
#  pragma warning(pop)
#endif
}
/*
** uint to unsigned short
*/
ushort curlx_uitous(uint uinum)
{
#ifdef __INTEL_COMPILER
#  pragma warning(push)
#  pragma warning(disable:810) /* conversion may lose significant bits */
#endif
	assert(uinum <= (uint)CURL_MASK_USHORT);
	return (ushort)(uinum & (uint)CURL_MASK_USHORT);
#ifdef __INTEL_COMPILER
#  pragma warning(pop)
#endif
}
/*
** signed int to unsigned size_t
*/
size_t curlx_sitouz(int sinum)
{
#ifdef __INTEL_COMPILER
#  pragma warning(push)
#  pragma warning(disable:810) /* conversion may lose significant bits */
#endif
	assert(sinum >= 0);
	return (size_t)sinum;
#ifdef __INTEL_COMPILER
#  pragma warning(pop)
#endif
}

#ifdef USE_WINSOCK
/*
** curl_socket_t to signed int
*/
int curlx_sktosi(curl_socket_t s) { return (int)((ssize_t)s); }
/*
** signed int to curl_socket_t
*/
curl_socket_t curlx_sitosk(int i) { return (curl_socket_t)((ssize_t)i); }

#endif /* USE_WINSOCK */

#if defined(WIN32)

ssize_t curlx_read(int fd, void * buf, size_t count) { return (ssize_t)_read(fd, buf, curlx_uztoui(count)); }
ssize_t curlx_write(int fd, const void * buf, size_t count) { return (ssize_t)_write(fd, buf, curlx_uztoui(count)); }

/* Ensure that warnless.h continues to have an effect in "unity" builds. */
#undef HEADER_CURL_WARNLESS_H

#endif /* WIN32 */

#if defined(__INTEL_COMPILER) && defined(__unix__)

int curlx_FD_ISSET(int fd, fd_set * fdset)
{
  #pragma warning(push)
  #pragma warning(disable:1469) /* clobber ignored */
	return FD_ISSET(fd, fdset);
  #pragma warning(pop)
}

void curlx_FD_SET(int fd, fd_set * fdset)
{
  #pragma warning(push)
  #pragma warning(disable:1469) /* clobber ignored */
	FD_SET(fd, fdset);
  #pragma warning(pop)
}

void curlx_FD_ZERO(fd_set * fdset)
{
  #pragma warning(push)
  #pragma warning(disable:593) /* variable was set but never used */
	FD_ZERO(fdset);
  #pragma warning(pop)
}

ushort curlx_htons(ushort usnum)
{
#if (__INTEL_COMPILER == 910) && defined(__i386__)
	return (ushort)(((usnum << 8) & 0xFF00) | ((usnum >> 8) & 0x00FF));
#else
  #pragma warning(push)
  #pragma warning(disable:810) /* conversion may lose significant bits */
	return htons(usnum);
  #pragma warning(pop)
#endif
}

ushort curlx_ntohs(ushort usnum)
{
#if (__INTEL_COMPILER == 910) && defined(__i386__)
	return (ushort)(((usnum << 8) & 0xFF00) | ((usnum >> 8) & 0x00FF));
#else
  #pragma warning(push)
  #pragma warning(disable:810) /* conversion may lose significant bits */
	return ntohs(usnum);
  #pragma warning(pop)
#endif
}

#endif /* __INTEL_COMPILER && __unix__ */
