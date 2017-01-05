#ifndef HEADER_CURL_WARNLESS_H
#define HEADER_CURL_WARNLESS_H
/***************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) 1998 - 2014, Daniel Stenberg, <daniel@haxx.se>, et al.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution. The terms
 * are also available at http://curl.haxx.se/docs/copyright.html.
 *
 * You may opt to use, copy, modify, merge, publish, distribute and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so, under the terms of the COPYING file.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ***************************************************************************/

#ifdef USE_WINSOCK
#include <curl/curl.h> /* for curl_socket_t */
#endif

ushort curlx_ultous(ulong ulnum);

uchar curlx_ultouc(ulong ulnum);

int curlx_ultosi(ulong ulnum);

int curlx_uztosi(size_t uznum);

curl_off_t curlx_uztoso(size_t uznum);

ulong curlx_uztoul(size_t uznum);

uint curlx_uztoui(size_t uznum);

int curlx_sltosi(long slnum);

uint curlx_sltoui(long slnum);

ushort curlx_sltous(long slnum);

ssize_t curlx_uztosz(size_t uznum);

size_t curlx_sotouz(curl_off_t sonum);

int curlx_sztosi(ssize_t sznum);

size_t curlx_sitouz(int sinum);

#ifdef USE_WINSOCK

int curlx_sktosi(curl_socket_t s);

curl_socket_t curlx_sitosk(int i);

#endif /* USE_WINSOCK */

#if defined(WIN32) || defined(_WIN32)

ssize_t curlx_read(int fd, void *buf, size_t count);

ssize_t curlx_write(int fd, const void *buf, size_t count);

#ifndef BUILDING_WARNLESS_C
#  undef  read
#  define read(fd, buf, count)  curlx_read(fd, buf, count)
#  undef  write
#  define write(fd, buf, count) curlx_write(fd, buf, count)
#endif

#endif /* WIN32 || _WIN32 */

#if defined(__INTEL_COMPILER) && defined(__unix__)

int curlx_FD_ISSET(int fd, fd_set *fdset);

void curlx_FD_SET(int fd, fd_set *fdset);

void curlx_FD_ZERO(fd_set *fdset);

ushort curlx_htons(ushort usnum);

ushort curlx_ntohs(ushort usnum);

#ifndef BUILDING_WARNLESS_C
#  undef  FD_ISSET
#  define FD_ISSET(a,b) curlx_FD_ISSET((a),(b))
#  undef  FD_SET
#  define FD_SET(a,b)   curlx_FD_SET((a),(b))
#  undef  FD_ZERO
#  define FD_ZERO(a)    curlx_FD_ZERO((a))
#  undef  htons
#  define htons(a)      curlx_htons((a))
#  undef  ntohs
#  define ntohs(a)      curlx_ntohs((a))
#endif

#endif /* __INTEL_COMPILER && __unix__ */

#endif /* HEADER_CURL_WARNLESS_H */
