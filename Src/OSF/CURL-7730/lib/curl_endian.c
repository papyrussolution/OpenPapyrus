/***************************************************************************
*                                  _   _ ____  _
*  Project                     ___| | | |  _ \| |
*                             / __| | | | |_) | |
*                            | (__| |_| |  _ <| |___
*                             \___|\___/|_| \_\_____|
*
* Copyright (C) 1998 - 2019, Daniel Stenberg, <daniel@haxx.se>, et al.
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
unsigned int Curl_read32_le(const uchar * buf)
{
	return ((uint)buf[0]) | ((uint)buf[1] << 8) |
	       ((uint)buf[2] << 16) | ((uint)buf[3] << 24);
}

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
	return (ushort)(((ushort)buf[0] << 8) |
	       ((ushort)buf[1]));
}

#if (CURL_SIZEOF_CURL_OFF_T > 4)
/*
 * write32_le()
 *
 * This function converts a 32-bit integer from the native endian format,
 * to little endian format ready for sending down the wire.
 *
 * Parameters:
 *
 * value    [in]     - The 32-bit integer value.
 * buffer   [in]     - A pointer to the output buffer.
 */
static void write32_le(const int value, uchar * buffer)
{
	buffer[0] = (char)(value & 0x000000FF);
	buffer[1] = (char)((value & 0x0000FF00) >> 8);
	buffer[2] = (char)((value & 0x00FF0000) >> 16);
	buffer[3] = (char)((value & 0xFF000000) >> 24);
}

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
	write32_le((int)value, buffer);
	write32_le((int)(value >> 32), buffer + 4);
}

#endif /* CURL_SIZEOF_CURL_OFF_T > 4 */
