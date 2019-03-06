
#include <slib.h>
#include <tv.h>
#pragma hdrstop
//#include <config.h>
//#include <string.h>
//#include "punycode.h"

/*** Bootstring parameters for Punycode ***/

enum {
	base = 36,
	tmin = 1,
	tmax = 26,
	skew = 38,
	damp = 700,
	initial_bias = 72,
	initial_n = 0x80,
	delimiter = 0x2D
};

#if 0 // {
enum punycode_status {
	punycode_success = 0,
	punycode_bad_input = 1,	/* Input is invalid.                       */
	punycode_big_output = 2,	/* Output would exceed the space provided. */
	punycode_overflow = 3	/* Wider integers needed to process input. */
};

typedef enum {
	PUNYCODE_SUCCESS = punycode_success,
	PUNYCODE_BAD_INPUT = punycode_bad_input,
	PUNYCODE_BIG_OUTPUT = punycode_big_output,
	PUNYCODE_OVERFLOW = punycode_overflow
} Punycode_status;
#endif // }0

#define basic(cp) ((uint)(cp) < 0x80) // basic(cp) tests whether cp is a basic code point
#define delim(cp) ((cp) == delimiter) // delim(cp) tests whether cp is a delimiter

/* decode_digit(cp) returns the numeric value of a basic code */
/* point (for use in representing integers) in the range 0 to */
/* base-1, or base if cp does not represent a value.          */

static uint FASTCALL decode_digit(uint cp)
{
	return ((cp - 48) < 10) ? (cp - 22) : ((cp - 65) < 26 ? (cp - 65) : ((cp - 97 < 26) ? (cp - 97) : base));
}

/* encode_digit(d,flag) returns the basic code point whose value      */
/* (when used for representing integers) is d, which needs to be in   */
/* the range 0 to base-1.  The lowercase form is used unless flag is  */
/* nonzero, in which case the uppercase form is used.  The behavior   */
/* is undefined if flag is nonzero and digit d has no uppercase form. */

static char FASTCALL encode_digit(uint d, int flag)
{
	return (d + 22 + 75 * (d < 26) - ((flag != 0) << 5));
	/*  0..25 map to ASCII a..z or A..Z */
	/* 26..35 map to ASCII 0..9         */
}

/* flagged(bcp) tests whether a basic code point is flagged */
/* (uppercase).  The behavior is undefined if bcp is not a  */
/* basic code point.                                        */

#define flagged(bcp) ((uint)(bcp) - 65 < 26)

/* encode_basic(bcp,flag) forces a basic code point to lowercase */
/* if flag is zero, uppercase if flag is nonzero, and returns    */
/* the resulting code point.  The code point is unchanged if it  */
/* is caseless.  The behavior is undefined if bcp is not a basic */
/* code point.                                                   */

static char FASTCALL encode_basic(uint bcp, int flag)
{
	bcp -= (bcp - 97 < 26) << 5;
	return bcp + ((!flag && (bcp - 65 < 26)) << 5);
}

/*** Platform-specific constants ***/


static const uint maxint = -1; // the maximum value of a uint variable
// Because maxint is unsigned, -1 becomes the maximum value.
//
// Bias adaptation function
//
static uint adapt(uint delta, uint numpoints, int firsttime)
{
	uint k;
	delta = firsttime ? (delta / damp) : (delta >> 1);
	/* delta >> 1 is a faster way of doing delta / 2 */
	delta += delta / numpoints;
	for(k = 0; delta > ((base - tmin) * tmax) / 2; k += base) {
		delta /= base - tmin;
	}
	return k + (base - tmin + 1) * delta / (delta + skew);
}

/*** Main encode function ***/

/**
 * punycode_encode:
 * @input_length: The number of code points in the @input array and
 * the number of flags in the @pCaseFlags array.
 * @input: An array of code points.  They are presumed to be Unicode
 * code points, but that is not strictly REQUIRED.  The array
 * contains code points, not code units.  UTF-16 uses code units
 * D800 through DFFF to refer to code points 10000..10FFFF.  The
 * code points D800..DFFF do not occur in any valid Unicode string.
 * The code points that can occur in Unicode strings (0..D7FF and
 * E000..10FFFF) are also called Unicode scalar values.
 * @pCaseFlags: A %NULL pointer or an array of boolean values parallel
 * to the @input array.  Nonzero (true, flagged) suggests that the
 * corresponding Unicode character be forced to uppercase after
 * being decoded (if possible), and zero (false, unflagged) suggests
 * that it be forced to lowercase (if possible).  ASCII code points
 * (0..7F) are encoded literally, except that ASCII letters are
 * forced to uppercase or lowercase according to the corresponding
 * case flags.  If @pCaseFlags is a %NULL pointer then ASCII letters
 * are left as they are, and other code points are treated as
 * unflagged.
 * @output_length: The caller passes in the maximum number of ASCII
 * code points that it can receive.  On successful return it will
 * contain the number of ASCII code points actually output.
 * @output: An array of ASCII code points.  It is *not*
 * null-terminated; it will contain zeros if and only if the @input
 * contains zeros.  (Of course the caller can leave room for a
 * terminator and add one if needed.)
 *
 * Converts a sequence of code points (presumed to be Unicode code
 * points) to Punycode.
 *
 * Return value: The return value can be any of the #Punycode_status
 * values defined above except %PUNYCODE_BAD_INPUT.  If not
 * %PUNYCODE_SUCCESS, then @output_size and @output might contain
 * garbage.
 **/
int SPunycodeEncode(const uint * input, size_t input_length, SString & rOut, const uchar * pCaseFlags)
{
	rOut = 0;
	int    ok = 1;
	uint   input_len, n, delta, h, b, bias, j, m, q, k, t;
	/* The Punycode spec assumes that the input length is the same type */
	/* of integer as a code point, so we need to convert the size_t to  */
	/* a uint, which could overflow.                           */
	THROW_S(input_length <= maxint, SLERR_PUNYCODE_OVERFLOW);
	input_len = (uint)input_length;
	//
	// Initialize the state:
	//
	n = initial_n;
	delta = 0;
	bias = initial_bias;
	//
	// Handle the basic code points:
	//
	for(j = 0; j < input_len; ++j) {
		if(basic(input[j])) {
			rOut.CatChar(pCaseFlags ? encode_basic(input[j], pCaseFlags[j]) : (char)input[j]);
		}
		/* else if (input[j] < n) return punycode_bad_input; */
		/* (not needed for Punycode with unsigned code points) */
	}
	h = b = rOut.Len();
	/* cannot overflow because out <= input_len <= maxint */

	/* h is the number of code points that have been handled, b is the  */
	/* number of basic code points, and out is the number of ASCII code */
	/* points that have been output.                                    */
	if(b > 0) {
		rOut.CatChar(delimiter);
	}
	//
	// Main encoding loop
	//
	while(h < input_len) {
		/* All non-basic code points < n have been     */
		/* handled already.  Find the next larger one: */
		for(m = maxint, j = 0; j < input_len; ++j) {
			/* if (basic(input[j])) continue; */
			/* (not needed for Punycode) */
			if(input[j] >= n && input[j] < m)
				m = input[j];
		}
		/* Increase delta enough to advance the decoder's    */
		/* <n,i> state to <m,0>, but guard against overflow: */
		THROW_S((m - n) <= (maxint - delta) / (h + 1), SLERR_PUNYCODE_OVERFLOW);
		delta += (m - n) * (h + 1);
		n = m;
		for(j = 0; j < input_len; ++j) {
			/* Punycode does not need to check whether input[j] is basic: */
			if(input[j] < n /* || basic(input[j]) */) {
				delta++;
				THROW_S(delta != 0, SLERR_PUNYCODE_OVERFLOW);
			}
			if(input[j] == n) {
				/* Represent delta as a generalized variable-length integer: */
				for(q = delta, k = base;; k += base) {
					t = k <= bias /* + tmin */ ? tmin : /* +tmin not needed */ k >= bias + tmax ? tmax : k - bias;
					if(q < t)
						break;
					rOut.CatChar(encode_digit(t + (q - t) % (base - t), 0));
					q = (q - t) / (base - t);
				}
				rOut.CatChar(encode_digit(q, pCaseFlags && pCaseFlags[j]));
				bias = adapt(delta, h + 1, h == b);
				delta = 0;
				++h;
			}
		}
		++delta;
		++n;
	}
	CATCHZOK
	return ok;
}

/*** Main decode function ***/

/**
 * punycode_decode:
 * @input_length: The number of ASCII code points in the @input array.
 * @input: An array of ASCII code points (0..7F).
 * @output_length: The caller passes in the maximum number of code
 * points that it can receive into the @output array (which is also
 * the maximum number of flags that it can receive into the
 * @pCaseFlags array, if @pCaseFlags is not a %NULL pointer).  On
 * successful return it will contain the number of code points
 * actually output (which is also the number of flags actually
 * output, if pCaseFlags is not a null pointer).  The decoder will
 * never need to output more code points than the number of ASCII
 * code points in the input, because of the way the encoding is
 * defined.  The number of code points output cannot exceed the
 * maximum possible value of a uint, even if the supplied
 * @output_length is greater than that.
 * @output: An array of code points like the input argument of
 * punycode_encode() (see above).
 * @pCaseFlags: A %NULL pointer (if the flags are not needed by the
 * caller) or an array of boolean values parallel to the @output
 * array.  Nonzero (true, flagged) suggests that the corresponding
 * Unicode character be forced to uppercase by the caller (if
 * possible), and zero (false, unflagged) suggests that it be forced
 * to lowercase (if possible).  ASCII code points (0..7F) are output
 * already in the proper case, but their flags will be set
 * appropriately so that applying the flags would be harmless.
 *
 * Converts Punycode to a sequence of code points (presumed to be
 * Unicode code points).
 *
 * Return value: The return value can be any of the #Punycode_status
 * values defined above.  If not %PUNYCODE_SUCCESS, then
 * @output_length, @output, and @pCaseFlags might contain garbage.
 *
 **/
int SPunycodeDecode(const char * input, size_t input_length, size_t * output_length, uint * output, uchar * pCaseFlags)
{
	int    ok = 1;
	uint   oldi, w, k, digit, t;
	size_t b, j, in;
	//
	// Initialize the state
	//
	uint   n = initial_n;
	uint   out = 0;
	uint   i = 0;
	uint   max_out = (*output_length > maxint) ? maxint : ((uint)*output_length);
	uint   bias = initial_bias;
	//
	// Handle the basic code points:  Let b be the number of input code
	// points before the last delimiter, or 0 if there is none, then
	// copy the first b code points to the output.
	//
	for(b = j = 0; j < input_length; ++j)
		if(delim(input[j]))
			b = j;
	THROW_S(b <= max_out, SLERR_PUNYCODE_BIGOUTPUT);
	for(j = 0; j < b; ++j) {
		if(pCaseFlags)
			pCaseFlags[out] = flagged(input[j]);
		THROW_S(basic(input[j]), SLERR_PUNYCODE_BADINPUT);
		output[out++] = input[j];
	}
	//
	// Main decoding loop:  Start just after the last delimiter if any
	// basic code points were copied; start at the beginning otherwise.
	//
	for(in = b > 0 ? b + 1 : 0; in < input_length; ++out) {
		/* in is the index of the next ASCII code point to be consumed, */
		/* and out is the number of code points in the output array.    */

		/* Decode a generalized variable-length integer into delta,  */
		/* which gets added to i.  The overflow checking is easier   */
		/* if we increase i as we go, then subtract off its starting */
		/* value at the end to obtain delta.                         */

		for(oldi = i, w = 1, k = base;; k += base) {
			THROW_S(in < input_length, SLERR_PUNYCODE_BADINPUT);
			digit = decode_digit(input[in++]);
			THROW_S(digit < base, SLERR_PUNYCODE_BADINPUT);
			THROW_S(digit <= ((maxint - i) / w), SLERR_PUNYCODE_OVERFLOW);
			i += digit * w;
			t = (k <= bias /* + tmin */) ? tmin : (/* +tmin not needed */ k >= bias + tmax ? tmax : (k - bias));
			if(digit < t)
				break;
			THROW_S(w <= (maxint / (base - t)), SLERR_PUNYCODE_OVERFLOW);
			w *= (base - t);
		}
		bias = adapt(i - oldi, out + 1, oldi == 0);
		//
		// i was supposed to wrap around from out+1 to 0,
		// incrementing n each time, so we'll fix that now:
		//
		THROW_S((i / (out + 1)) <= (maxint - n), SLERR_PUNYCODE_OVERFLOW);
		n += i / (out + 1);
		i %= (out + 1);
		//
		// Insert n at position i of the output: not needed for Punycode:
		// if (basic(n)) return punycode_bad_input;
		//
		THROW_S(out < max_out, SLERR_PUNYCODE_BIGOUTPUT);
		if(pCaseFlags) {
			memmove(pCaseFlags + i + 1, pCaseFlags + i, out - i);
			// Case of last ASCII code point determines case flag
			pCaseFlags[i] = flagged(input[in - 1]);
		}
		memmove(output + i + 1, output + i, (out - i) * sizeof(*output));
		output[i++] = n;
	}
	*output_length = (size_t)out;
	// cannot overflow because out <= old value of *output_length
	CATCHZOK
	return ok;
}

