/***************************************************************************
*                                  _   _ ____  _
*  Project                     ___| | | |  _ | |
*                             / __| | | | |_) | |
*                            | (__| |_| |  _ <| |___
*                             ___|___/|_| ______|
*
* Copyright (C) Daniel Stenberg, <daniel.se>, et al.
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
#include "easyoptions.h"

#ifndef CURL_DISABLE_GETOPTIONS

/* Lookups easy options at runtime */
static struct curl_easyoption *lookup(const char * name, CURLoption id) 
{
	assert(name || id);
	assert(!Curl_easyopts_check());
	if(name || id) {
		struct curl_easyoption * o = &Curl_easyopts[0];
		do {
			if(name) {
				if(sstreqi_ascii(o->name, name))
					return o;
			}
			else {
				if((o->id == id) && !(o->flags & CURLOT_FLAG_ALIAS))
					/* don't match alias options */
					return o;
			}
			o++;
		} while(o->name);
	}
	return NULL;
}

const struct curl_easyoption *curl_easy_option_by_name(const char * name) { return lookup(name, CURLOPT_LASTENTRY); /* when name is used, the id argument is ignored */ }
const struct curl_easyoption *curl_easy_option_by_id(CURLoption id) { return lookup(NULL, id); }

/* Iterates over available options */
const struct curl_easyoption *curl_easy_option_next(const struct curl_easyoption * prev) 
{
	if(prev && prev->name) {
		prev++;
		if(prev->name)
			return prev;
	}
	else if(!prev)
		return &Curl_easyopts[0];
	return NULL;
}

#else
	const struct curl_easyoption *curl_easy_option_by_name(const char */*name*/) { return NULL; }
	const struct curl_easyoption *curl_easy_option_by_id(CURLoption /*id*/) { return NULL; }
	const struct curl_easyoption *curl_easy_option_next(const struct curl_easyoption * /*prev*/) { return NULL; }
#endif
