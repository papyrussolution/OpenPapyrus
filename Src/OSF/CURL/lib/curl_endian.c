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

#include "curl_setup.h"
#pragma hdrstop
#include "curl_endian.h"

/*
 * Curl_read16_le()
 *
 * This function converts a 16-bit integer from the little endian format, as
 * used in the incoming package to whatever endian format we're using
 * natively.
 *
 * Parameters:
 *
 * buf      [in]     - A pointer to a 2 byte buffer.
 *
 * Returns the integer.
 */
ushort Curl_read16_le(const uchar * buf)
{
	return (ushort)(((ushort)buf[0]) |
	    ((ushort)buf[1] << 8));
}

/*
 * Curl_read32_le()
 *
 * This function converts a 32-bit integer from the little endian format, as
 * used in the incoming package to whatever endian format we're using
 * natively.
 *
 * Parameters:
 *
 * buf      [in]     - A pointer to a 4 byte buffer.
 *
 * Returns the integer.
 */
uint Curl_read32_le(const uchar * buf)
{
	return ((uint)buf[0]) | ((uint)buf[1] << 8) |
	       ((uint)buf[2] << 16) | ((uint)buf[3] << 24);
}

#if (CURL_SIZEOF_CURL_OFF_T > 4)
/*
 * Curl_read64_le()
 *
 * This function converts a 64-bit integer from the little endian format, as
 * used in the incoming package to whatever endian format we're using
 * natively.
 *
 * Parameters:
 *
 * buf      [in]     - A pointer to a 8 byte buffer.
 *
 * Returns the integer.
 */
//#if defined(HAVE_LONGLONG)
uint64 Curl_read64_le(const uchar * buf)
{
	return ((uint64)buf[0])|((uint64)buf[1] << 8)|((uint64)buf[2] << 16)|((uint64)buf[3] << 24)|((uint64)buf[4] << 32)|((uint64)buf[5] << 40)|((uint64)buf[6] << 48)|((uint64)buf[7] << 56);
}
/*#else
uint64 Curl_read64_le(const uchar * buf)
{
	return ((uint64)buf[0])|((uint64)buf[1] << 8)|((uint64)buf[2] << 16)|((uint64)buf[3] << 24)|((uint64)buf[4] << 32)|((uint64)buf[5] << 40)|((uint64)buf[6] << 48)|((uint64)buf[7] << 56);
}
#endif*/

#endif /* CURL_SIZEOF_CURL_OFF_T > 4 */

/*
 * Curl_read16_be()
 *
 * This function converts a 16-bit integer from the big endian format, as
 * used in the incoming package to whatever endian format we're using
 * natively.
 *
 * Parameters:
 *
 * buf      [in]     - A pointer to a 2 byte buffer.
 *
 * Returns the integer.
 */
ushort Curl_read16_be(const uchar * buf)
{
	return (ushort)(((ushort)buf[0] << 8) | ((ushort)buf[1]));
}

/*
 * Curl_read32_be()
 *
 * This function converts a 32-bit integer from the big endian format, as
 * used in the incoming package to whatever endian format we're using
 * natively.
 *
 * Parameters:
 *
 * buf      [in]     - A pointer to a 4 byte buffer.
 *
 * Returns the integer.
 */
uint Curl_read32_be(const uchar * buf)
{
	return ((uint)buf[0] << 24) | ((uint)buf[1] << 16) |
	       ((uint)buf[2] << 8) | ((uint)buf[3]);
}

#if (CURL_SIZEOF_CURL_OFF_T > 4)
/*
 * Curl_read64_be()
 *
 * This function converts a 64-bit integer from the big endian format, as
 * used in the incoming package to whatever endian format we're using
 * natively.
 *
 * Parameters:
 *
 * buf      [in]     - A pointer to a 8 byte buffer.
 *
 * Returns the integer.
 */
#if defined(HAVE_LONGLONG)
uint64 Curl_read64_be(const uchar * buf)
{
	return ((uint64)buf[0] << 56) |
	       ((uint64)buf[1] << 48) |
	       ((uint64)buf[2] << 40) |
	       ((uint64)buf[3] << 32) |
	       ((uint64)buf[4] << 24) |
	       ((uint64)buf[5] << 16) |
	       ((uint64)buf[6] << 8) |
	       ((uint64)buf[7]);
}

#else
uint64 Curl_read64_be(const uchar * buf)
{
	return ((uint64)buf[0] << 56) | ((uint64)buf[1] << 48) |
	       ((uint64)buf[2] << 40) | ((uint64)buf[3] << 32) |
	       ((uint64)buf[4] << 24) | ((uint64)buf[5] << 16) |
	       ((uint64)buf[6] << 8) | ((uint64)buf[7]);
}

#endif

#endif /* CURL_SIZEOF_CURL_OFF_T > 4 */

/*
 * Curl_write16_le()
 *
 * This function converts a 16-bit integer from the native endian format,
 * to little endian format ready for sending down the wire.
 *
 * Parameters:
 *
 * value    [in]     - The 16-bit integer value.
 * buffer   [in]     - A pointer to the output buffer.
 */
void Curl_write16_le(const short value, uchar * buffer)
{
	buffer[0] = (char)(value & 0x00FF);
	buffer[1] = (char)((value & 0xFF00) >> 8);
}

/*
 * Curl_write32_le()
 *
 * This function converts a 32-bit integer from the native endian format,
 * to little endian format ready for sending down the wire.
 *
 * Parameters:
 *
 * value    [in]     - The 32-bit integer value.
 * buffer   [in]     - A pointer to the output buffer.
 */
void Curl_write32_le(const int value, uchar * buffer)
{
	buffer[0] = (char)(value & 0x000000FF);
	buffer[1] = (char)((value & 0x0000FF00) >> 8);
	buffer[2] = (char)((value & 0x00FF0000) >> 16);
	buffer[3] = (char)((value & 0xFF000000) >> 24);
}

#if (CURL_SIZEOF_CURL_OFF_T > 4)
/*
 * Curl_write64_le()
 *
 * This function converts a 64-bit integer from the native endian format,
 * to little endian format ready for sending down the wire.
 *
 * Parameters:
 *
 * value    [in]     - The 64-bit integer value.
 * buffer   [in]     - A pointer to the output buffer.
 */
#if defined(HAVE_LONGLONG)
void Curl_write64_le(const long long value, uchar * buffer)
#else
void Curl_write64_le(const __int64 value, uchar * buffer)
#endif
{
	Curl_write32_le((int)value, buffer);
	Curl_write32_le((int)(value >> 32), buffer + 4);
}

#endif /* CURL_SIZEOF_CURL_OFF_T > 4 */
