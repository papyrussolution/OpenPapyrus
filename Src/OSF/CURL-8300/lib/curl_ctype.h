#ifndef HEADER_CURL_CTYPE_H
#define HEADER_CURL_CTYPE_H
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

#define ISLOWHEXALHA(x) (((x) >= 'a') && ((x) <= 'f'))
#define ISUPHEXALHA(x) (((x) >= 'A') && ((x) <= 'F'))
#define ISLOWCNTRL(x) ((uchar)(x) <= 0x1f)
#define IS7F(x) ((x) == 0x7f)
#define ISLOWPRINT(x) (((x) >= 9) && ((x) <= 0x0d))
#define ISPRINT(x)  (ISLOWPRINT(x) || (((x) >= ' ') && ((x) <= 0x7e)))
#define ISGRAPH(x)  (ISLOWPRINT(x) || (((x) > ' ') && ((x) <= 0x7e)))
#define ISCNTRL(x) (ISLOWCNTRL(x) || IS7F(x))
// @sobolev (replaced with isasciialpha) #define ISALPHA_Removed(x) (ISLOWER(x) || ISUPPER(x))
// @sobolev (replaced with ishex) #define ISXDIGIT_Removed(x) (isdec(x) || ISLOWHEXALHA(x) || ISUPHEXALHA(x))
// @sobolev (replaced with isasciialnum) #define ISALNUM_Removed(x)  (isdec(x) || ISLOWER(x) || ISUPPER(x))
#define ISUPPER(x)  (((x) >= 'A') && ((x) <= 'Z'))
#define ISLOWER(x)  (((x) >= 'a') && ((x) <= 'z'))
// @sobolev (replaced with isdec) #define ISDIGIT_Removed(x)  (((x) >= '0') && ((x) <= '9'))
#define ISBLANK(x)  (((x) == ' ') || ((x) == '\t'))
#define ISSPACE(x)  (ISBLANK(x) || (((x) >= 0xa) && ((x) <= 0x0d)))

#endif /* HEADER_CURL_CTYPE_H */
