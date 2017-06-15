/***************************************************************************
*                                  _   _ ____  _
*  Project                     ___| | | |  _ \| |
*                             / __| | | | |_) | |
*                            | (__| |_| |  _ <| |___
*                             \___|\___/|_| \_\_____|
*
* Copyright (C) 2010 - 2015, Daniel Stenberg, <daniel@haxx.se>, et al.
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

#include "curl_setup.h"
#pragma hdrstop
//#include "strdup.h"
//#include "fileinfo.h"
//#include "curl_memory.h"
#include "memdebug.h" // The last #include file should be

struct curl_fileinfo * Curl_fileinfo_alloc(void)
{
	struct curl_fileinfo * tmp = (struct curl_fileinfo *)SAlloc::M(sizeof(struct curl_fileinfo));
	memzero(tmp, sizeof(struct curl_fileinfo));
	return tmp;
}

void Curl_fileinfo_dtor(void * user, void * element)
{
	struct curl_fileinfo * finfo = (struct curl_fileinfo *)element;
	(void)user;
	if(finfo) {
		ZFREE(finfo->b_data);
		SAlloc::F(finfo);
	}
}

