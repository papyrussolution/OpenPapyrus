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
* RFC2195 CRAM-MD5 authentication
*
***************************************************************************/

#include "curl_setup.h"
#pragma hdrstop
#if !defined(CURL_DISABLE_CRYPTO_AUTH)

#include "curl_printf.h"
#include "memdebug.h"
/*
 * Curl_auth_decode_cram_md5_message()
 *
 * This is used to decode an already encoded CRAM-MD5 challenge message.
 *
 * Parameters:
 *
 * chlg64  [in]     - The base64 encoded challenge message.
 * outptr  [in/out] - The address where a pointer to newly allocated memory
 *               holding the result will be stored upon completion.
 * outlen  [out]    - The length of the output message.
 *
 * Returns CURLE_OK on success.
 */
CURLcode Curl_auth_decode_cram_md5_message(const char * chlg64, char ** outptr, size_t * outlen)
{
	CURLcode result = CURLE_OK;
	size_t chlg64len = sstrlen(chlg64);
	*outptr = NULL;
	*outlen = 0;
	// Decode the challenge if necessary 
	if(chlg64len && *chlg64 != '=')
		result = Curl_base64_decode(chlg64, (uchar**)outptr, outlen);
	return result;
}
/*
 * Curl_auth_create_cram_md5_message()
 *
 * This is used to generate an already encoded CRAM-MD5 response message ready
 * for sending to the recipient.
 *
 * Parameters:
 *
 * data    [in]     - The session handle.
 * chlg    [in]     - The challenge.
 * userp   [in]     - The user name.
 * passdwp [in]     - The user's password.
 * outptr  [in/out] - The address where a pointer to newly allocated memory
 *               holding the result will be stored upon completion.
 * outlen  [out]    - The length of the output message.
 *
 * Returns CURLE_OK on success.
 */
CURLcode Curl_auth_create_cram_md5_message(struct Curl_easy * data,
    const char * chlg, const char * userp, const char * passwdp, char ** outptr, size_t * outlen)
{
	CURLcode result = CURLE_OK;
	// Compute the digest using the password as the key 
	HMAC_context * ctxt = Curl_HMAC_init(Curl_HMAC_MD5, (const uchar *)passwdp, curlx_uztoui(sstrlen(passwdp)));
	if(!ctxt)
		result = CURLE_OUT_OF_MEMORY;
	else {
		uchar digest[MD5_DIGEST_LEN];
		size_t chlglen = sstrlen(chlg);
		// Update the digest with the given challenge 
		if(chlglen > 0)
			Curl_HMAC_update(ctxt, (const uchar *)chlg, curlx_uztoui(chlglen));
		// Finalise the digest 
		Curl_HMAC_final(ctxt, digest);
		{
			// Generate the response 
			char * response = aprintf("%s %02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
				userp, digest[0], digest[1], digest[2], digest[3], digest[4],
				digest[5], digest[6], digest[7], digest[8], digest[9], digest[10],
				digest[11], digest[12], digest[13], digest[14], digest[15]);
			if(!response)
				result = CURLE_OUT_OF_MEMORY;
			else {
				// Base64 encode the response 
				result = Curl_base64_encode(data, response, 0, outptr, outlen);
				SAlloc::F(response);
			}
		}
	}
	return result;
}

#endif /* !CURL_DISABLE_CRYPTO_AUTH */
