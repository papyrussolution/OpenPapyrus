#ifndef HEADER_CURL_NTLM_CORE_H
#define HEADER_CURL_NTLM_CORE_H
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

#if defined(USE_CURL_NTLM_CORE)

#if defined(USE_OPENSSL)
#include <openssl/ssl.h>
#elif defined(USE_WOLFSSL)
#include <wolfssl/options.h>
#include <wolfssl/openssl/ssl.h>
#endif

/* Helpers to generate function byte arguments in little endian order */
#define SHORTPAIR(x) ((int)((x) & 0xff)), ((int)(((x) >> 8) & 0xff))
#define LONGQUARTET(x) ((int)((x) & 0xff)), ((int)(((x) >> 8) & 0xff)), \
	((int)(((x) >> 16) & 0xff)), ((int)(((x) >> 24) & 0xff))

void Curl_ntlm_core_lm_resp(const uchar * keys, const uchar * plaintext, uchar * results);
CURLcode Curl_ntlm_core_mk_lm_hash(const char * password, uchar * lmbuffer /* 21 bytes */);
CURLcode Curl_ntlm_core_mk_nt_hash(const char * password, uchar * ntbuffer /* 21 bytes */);

#if !defined(USE_WINDOWS_SSPI)

CURLcode Curl_hmac_md5(const uchar * key, uint keylen, const uchar * data, uint datalen, uchar * output);
CURLcode Curl_ntlm_core_mk_ntlmv2_hash(const char * user, size_t userlen, const char * domain, size_t domlen, uchar * ntlmhash, uchar * ntlmv2hash);
CURLcode  Curl_ntlm_core_mk_ntlmv2_resp(uchar * ntlmv2hash, uchar * challenge_client, struct ntlmdata * ntlm, uchar ** ntresp, uint * ntresp_len);
CURLcode  Curl_ntlm_core_mk_lmv2_resp(uchar * ntlmv2hash, uchar * challenge_client, uchar * challenge_server, uchar * lmresp);

#endif /* !USE_WINDOWS_SSPI */
#endif /* USE_CURL_NTLM_CORE */
#endif /* HEADER_CURL_NTLM_CORE_H */
