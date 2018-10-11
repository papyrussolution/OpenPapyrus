/*
 *  Bijective, heapless and bignumless conversion of IEEE 754 double to string and vice versa
 *  http://www.gurucoding.com/en/dconvstr/
 *
 *  Copyright (c) 2014 Mikhail Kupchik <Mikhail.Kupchik@prime-expert.com>
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without modification, are permitted 
 *  provided that the following conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright notice, this list of conditions
 *     and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions
 *     and the following disclaimer in the documentation and/or other materials provided with the 
 *     distribution.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR 
 *  IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND 
 *  FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR 
 *  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL 
 *  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 *  IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 *  OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
#ifndef DCONVSTR_H
#define DCONVSTR_H

#define DCONVSTR_FLAG_HAVE_WIDTH     0x0001
#define DCONVSTR_FLAG_LEFT_JUSTIFY   0x0002
#define DCONVSTR_FLAG_SHARP          0x0004
#define DCONVSTR_FLAG_PRINT_PLUS     0x0008
#define DCONVSTR_FLAG_SPACE_IF_PLUS  0x0010
#define DCONVSTR_FLAG_PAD_WITH_ZERO  0x0020
#define DCONVSTR_FLAG_UPPERCASE      0x0040

#define DCONVSTR_DEFAULT_PRECISION   6
// 
// Descr: Print IEEE 754 floating-point double precision value to string
// @param  outbuf            Address of variable with a pointer to output buffer filled by the function.
//   On entry, this variable is initialized by caller.
//   On exit, this variable points to the end of printed string.
// @param  outbuf_size       Size of output buffer filled by the function.
//   On entry, this variable is initialized by caller to maximum allowed size.
//   On exit, this variable contains size of unused portion of the output buffer.
// @param  value             Input value (IEEE 754 floating-point double precision).
// @param  format_char       Format char. Either 'e', or 'f', or 'g'. Refer to printf(3) manual for details.
// @param  format_flags      Any combination of the above (DCONVSTR_FLAG_*).
// @param  format_width      Format width. Used only if DCONVSTR_FLAG_HAVE_WIDTH bit is set in flags.
//   Refer to printf(3) manual for details.
// @param  format_precision  Format precision. Set it to DCONVSTR_DEFAULT_PRECISION if unsure.
//   Refer to printf(3) manual for details.
// @returns  1  if value was successfully converted to string.
//   0  if there is not enough room in buffer or internal error happened during conversion.
// 
int dconvstr_print(char ** outbuf, int * outbuf_size, double value, int format_char, uint  format_flags, int format_width, int format_precision);
// 
// Descr: Convert string to IEEE 754 floating-point double precision value
// @param  input          Input buffer, C-style string. Filled by caller.
// @param  input_end      Address of pointer to end of scanned value in input buffer.
//   Filled by function if address is not NULL.
// @param  output         Conversion result (IEEE 754 floating-point double precision).
//   Set to 0.0 if string in input buffer has syntax errors.
// @param  output_erange  Address of overflow/underflow flag variable, filled by function.
//   0  if there is no overflow/underflow condition
//   1  if there is overflow/underflow condition: strtod(3) would set errno = ERANGE
// @returns  1  if there were no internal errors
//   0  if there was internal error during conversion.
// 
// In general, interface of this function is similar to strtod(3), except for returning overflow
// condition instead of setting errno. If you want just to convert C-style string to double with
// error checking, then set input_end != NULL and use ( ret_value != 0 )&&( **input_end == 0 )
// condition as an indication of successful conversion.
// 
int dconvstr_scan(const char * input, const char ** input_end, double * output, int * output_erange);

#endif // DCONVSTR_H
