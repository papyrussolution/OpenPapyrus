#ifndef HEADER_CURL_MD5_H
#define HEADER_CURL_MD5_H
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

#if (defined(USE_CURL_NTLM_CORE) && !defined(USE_WINDOWS_SSPI)) || !defined(CURL_DISABLE_DIGEST_AUTH)

#include "curl_hmac.h"

#define MD5_DIGEST_LEN  16

typedef CURLcode (* Curl_MD5_init_func)(void * context);
typedef void (* Curl_MD5_update_func)(void * context, const uchar * data, uint len);
typedef void (* Curl_MD5_final_func)(uchar * result, void * context);

struct MD5_params {
	Curl_MD5_init_func md5_init_func; /* Initialize context procedure */
	Curl_MD5_update_func md5_update_func; /* Update context with data */
	Curl_MD5_final_func md5_final_func; /* Get final result procedure */
	uint md5_ctxtsize;      /* Context structure size */
	uint md5_resultlen;     /* Result length (bytes) */
};

struct MD5_context {
	const struct MD5_params * md5_hash; /* Hash function definition */
	void * md5_hashctx;/* Hash function context */
};

extern const struct MD5_params Curl_DIGEST_MD5[1];
extern const struct HMAC_params Curl_HMAC_MD5[1];

CURLcode Curl_md5it(uchar * output, const uchar * input, const size_t len);
struct MD5_context * Curl_MD5_init(const struct MD5_params * md5params);
CURLcode Curl_MD5_update(struct MD5_context * context, const uchar * data, uint len);
CURLcode Curl_MD5_final(struct MD5_context * context, uchar * result);

#endif

#endif /* HEADER_CURL_MD5_H */
