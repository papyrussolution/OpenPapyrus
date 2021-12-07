// tulipindicators.c
//
// 
// Tulip Indicators https://tulipindicators.org/
// Copyright (c) 2010-2016 Tulip Charts LLC
// Lewis Van Winkle (LV@tulipcharts.org)
// 
// This file is part of Tulip Indicators.
// 
// Tulip Indicators is free software: you can redistribute it and/or modify it
// under the terms of the GNU Lesser General Public License as published by the
// Free Software Foundation, either version 3 of the License, or (at your option) any later version.
// 
// Tulip Indicators is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with Tulip Indicators.  If not, see <http://www.gnu.org/licenses/>.
// 
#include <slib-internal.h>
#pragma hdrstop
#include <tulipindicators.h>
// 
// This is used for the simple functions that take one input vectors and apply a unary
// operator for a single output. (e.g. sqrt, sin)
// 
// Fallback 
//#ifndef M_PI
//	#define M_PI 3.14159265358979323846
//#endif

#define SIMPLE1(START, FUN, OP)	int START(double const * options) { return 0; } \
	int FUN(int size, double const * const * inputs, double const * options, double * const * outputs) \
	{ \
		const double * in1 = inputs[0]; \
 		double * output = outputs[0]; \
		for(int i = 0; i < size; ++i) { output[i] = (OP); } \
		return TI_OKAY;	\
	} \
//
//#include "simple2.h"
// 
// This is used for the simple functions that take two input vectors and apply a binary
// operator for a single output. (e.g. add, multiply)
// 
#define SIMPLE2(START, FUN, OP) int START(double const *options) { return 0; } \
	int FUN(int size, double const *const *inputs, double const *options, double *const *outputs) \
	{ \
		const double *in1 = inputs[0]; \
		const double *in2 = inputs[1]; \
		double * output = outputs[0]; \
		for(int i = 0; i < size; ++i) { output[i] = (OP); } \
		return TI_OKAY; \
	} \

struct ti_buffer {
	int    size;
	int    pushes;
	int    index;
	double sum;
	double * P_Vals;
};

//ti_buffer * ti_buffer_new(int size);
//void ti_buffer_free(ti_buffer * buffer);

// Pushes a new value, plus updates sum. 
#define ti_buffer_push(BUFFER, VAL) \
	do { \
		if((BUFFER)->pushes >= (BUFFER)->size) { \
			(BUFFER)->sum -= (BUFFER)->P_Vals[(BUFFER)->index]; \
		} \
		(BUFFER)->sum += (VAL);	\
		(BUFFER)->P_Vals[(BUFFER)->index] = (VAL); \
		(BUFFER)->pushes += 1; \
		(BUFFER)->index = ((BUFFER)->index + 1); \
		if((BUFFER)->index >= (BUFFER)->size) (BUFFER)->index = 0; \
	} while(0)

// Pushes a new value, skips updating sum. 
#define ti_buffer_qpush(BUFFER, VAL) \
	do { \
		(BUFFER)->P_Vals[(BUFFER)->index] = (VAL); \
		(BUFFER)->index = ((BUFFER)->index + 1); \
		if((BUFFER)->index >= (BUFFER)->size) (BUFFER)->index = 0; \
	} while(0)

// With get, 0 = last value pushed, -1 = value before last, etc. 
#define ti_buffer_get(BUFFER, VAL) ((BUFFER)->P_Vals[((BUFFER)->index + (BUFFER)->size - 1 + (VAL)) % (BUFFER)->size])
//
static ti_buffer * ti_buffer_new(int size) 
{
	const int s = (int)sizeof(ti_buffer) + size * (int)sizeof(double);
	ti_buffer * ret = static_cast<ti_buffer *>(SAlloc::M((uint)s));
	ret->size = size;
	ret->pushes = 0;
	ret->index = 0;
	ret->sum = 0;
	return ret;
}

static void ti_buffer_free(ti_buffer * buffer) 
{
	SAlloc::F(buffer);
}

//#include "truerange.h"
#define CALC_TRUERANGE() do {\
	const double l = low[i];\
	const double h = high[i];\
	const double c = close[i-1];\
	const double ych = fabs(h - c);\
	const double ycl = fabs(l - c);\
	double v = h - l;\
	if(ych > v) v = ych;\
	if(ycl > v) v = ycl;\
	truerange = v;\
} while(0)
//
//#include "dx.h"
//
// This is used with the DX family of indicators. e.g. DX ADX ADXR
//
#define CALC_DIRECTION(up, down) do { \
		up = high[i] - high[i-1]; \
		down = low[i-1] - low[i]; \
		if(up < 0) \
			up = 0;	\
		else if(up > down) \
			down = 0; \
		if(down < 0) \
			down = 0; \
		else if(down > up) \
			up = 0;	\
} while(0)
//

SIMPLE1(ti_abs_start, ti_abs, fabs(in1[i]))
SIMPLE1(ti_cos_start, ti_cos, cos(in1[i]))
SIMPLE1(ti_cosh_start, ti_cosh, cosh(in1[i]))
SIMPLE1(ti_acos_start, ti_acos, acos(in1[i]))
SIMPLE1(ti_sin_start, ti_sin, sin(in1[i]))
SIMPLE1(ti_sinh_start, ti_sinh, sinh(in1[i]))
SIMPLE1(ti_asin_start, ti_asin, asin(in1[i]))
SIMPLE1(ti_tan_start, ti_tan, tan(in1[i]))
SIMPLE1(ti_tanh_start, ti_tanh, tanh(in1[i]))
SIMPLE1(ti_atan_start, ti_atan, atan(in1[i]))
SIMPLE1(ti_floor_start, ti_floor, floor(in1[i]))
SIMPLE1(ti_ceil_start, ti_ceil, ceil(in1[i]))
SIMPLE1(ti_round_start, ti_round, floor(in1[i] + 0.5))
SIMPLE1(ti_trunc_start, ti_trunc, (int)(in1[i]))
SIMPLE1(ti_sqrt_start, ti_sqrt, sqrt(in1[i]))
SIMPLE1(ti_exp_start, ti_exp, exp(in1[i]))
SIMPLE1(ti_ln_start, ti_ln, log(in1[i]))
SIMPLE1(ti_log10_start, ti_log10, log10(in1[i]))
SIMPLE1(ti_todeg_start, ti_todeg, (in1[i] * (180.0 / SMathConst::Pi)))
SIMPLE1(ti_torad_start, ti_torad, (in1[i] * (SMathConst::Pi / 180.0)))

SIMPLE2(ti_add_start, ti_add, in1[i] + in2[i])
SIMPLE2(ti_sub_start, ti_sub, in1[i] - in2[i])
SIMPLE2(ti_mul_start, ti_mul, in1[i] * in2[i])
SIMPLE2(ti_div_start, ti_div, in1[i] / in2[i])
//
// ad
int ti_ad_start(double const * options) { return 0; }

int ti_ad(int size, double const * const * inputs, double const * options, double * const * outputs) 
{
	const double * high = inputs[0];
	const double * low = inputs[1];
	const double * close = inputs[2];
	const double * volume = inputs[3];
	double * output = outputs[0];
	double sum = 0;
	for(int i = 0; i < size; ++i) {
		const double hl = (high[i] - low[i]);
		if(hl != 0.0) {
			sum += (close[i] - low[i] - high[i] + close[i]) / hl * volume[i];
		}
		output[i] = sum;
	}
	return TI_OKAY;
}
//
// adosc
int ti_adosc_start(double const * options) { return (int)(options[1])-1; }

int ti_adosc(int size, double const * const * inputs, double const * options, double * const * outputs) 
{
	const double * high = inputs[0];
	const double * low = inputs[1];
	const double * close = inputs[2];
	const double * volume = inputs[3];
	const int short_period = (int)options[0];
	const int long_period = (int)options[1];
	const int start = long_period - 1;
	if(short_period < 1) return TI_INVALID_OPTION;
	if(long_period < short_period) return TI_INVALID_OPTION;
	if(size <= ti_adosc_start(options)) return TI_OKAY;
	const double short_per = 2 / ((double)short_period + 1);
	const double long_per = 2 / ((double)long_period + 1);
	double * output = outputs[0];
	double sum = 0.0;
	double short_ema = 0.0;
	double long_ema = 0.0;
	for(int i = 0; i < size; ++i) {
		const double hl = (high[i] - low[i]);
		if(hl != 0.0) {
			sum += (close[i] - low[i] - high[i] + close[i]) / hl * volume[i];
		}
		if(i == 0) {
			short_ema = sum;
			long_ema = sum;
		}
		else {
			short_ema = (sum-short_ema) * short_per + short_ema;
			long_ema = (sum-long_ema) * long_per + long_ema;
		}
		if(i >= start) {
			*output++ = short_ema - long_ema;
		}
	}
	assert(output - outputs[0] == size - ti_adosc_start(options));
	return TI_OKAY;
}
//
// adx
int ti_adx_start(double const * options) { return ((int)options[0]-1) * 2; }

int ti_adx(int size, double const * const * inputs, double const * options, double * const * outputs) 
{
	const double * high = inputs[0];
	const double * low = inputs[1];
	const double * close = inputs[2];
	const int period = (int)options[0];
	double * output = outputs[0];
	if(period < 2) return TI_INVALID_OPTION;
	if(size <= ti_adx_start(options)) return TI_OKAY;
	const double per = ((double)period-1) / ((double)period);
	const double invper = 1.0 / ((double)period);
	double atr = 0;
	double dmup = 0;
	double dmdown = 0;
	int i;
	for(i = 1; i < period; ++i) {
		double truerange;
		CALC_TRUERANGE();
		atr += truerange;
		double dp, dm;
		CALC_DIRECTION(dp, dm);
		dmup += dp;
		dmdown += dm;
	}
	double adx = 0.0;
	{
		double di_up = dmup / atr;
		double di_down = dmdown / atr;
		double dm_diff = fabs(di_up - di_down);
		double dm_sum = di_up + di_down;
		double dx = dm_diff / dm_sum * 100;
		adx += dx;
	}
	for(i = period; i < size; ++i) {
		double truerange;
		CALC_TRUERANGE();
		atr = atr * per + truerange;
		double dp, dm;
		CALC_DIRECTION(dp, dm);
		dmup = dmup * per + dp;
		dmdown = dmdown * per + dm;
		double di_up = dmup / atr;
		double di_down = dmdown / atr;
		double dm_diff = fabs(di_up - di_down);
		double dm_sum = di_up + di_down;
		double dx = dm_diff / dm_sum * 100;
		if(i-period < period-2) {
			adx += dx;
		}
		else if(i-period == period-2) {
			adx += dx;
			*output++ = adx * invper;
		}
		else {
			adx = adx * per + dx;
			*output++ = adx * invper;
		}
	}
	assert(output - outputs[0] == size - ti_adx_start(options));
	return TI_OKAY;
}
//
// adxr
int ti_adxr_start(double const * options) { return ((int)options[0]-1) * 3; }

int ti_adxr(int size, double const * const * inputs, double const * options, double * const * outputs) 
{
	const double * high = inputs[0];
	const double * low = inputs[1];
	const double * close = inputs[2];
	const int period = (int)options[0];
	double * output = outputs[0];
	if(period < 2) 
		return TI_INVALID_OPTION;
	if(size <= ti_adxr_start(options)) 
		return TI_OKAY;
	const double per = ((double)period-1) / ((double)period);
	const double invper = 1.0 / ((double)period);
	double atr = 0;
	double dmup = 0;
	double dmdown = 0;
	int i;
	for(i = 1; i < period; ++i) {
		double truerange;
		CALC_TRUERANGE();
		atr += truerange;
		double dp, dm;
		CALC_DIRECTION(dp, dm);
		dmup += dp;
		dmdown += dm;
	}
	double adx = 0.0;
	{
		double di_up = dmup / atr;
		double di_down = dmdown / atr;
		double dm_diff = fabs(di_up - di_down);
		double dm_sum = di_up + di_down;
		double dx = dm_diff / dm_sum * 100;
		adx += dx;
	}
	ti_buffer * adxr = ti_buffer_new(period-1);
	const int first_adxr = ti_adxr_start(options);
	for(i = period; i < size; ++i) {
		double truerange;
		CALC_TRUERANGE();
		atr = atr * per + truerange;
		double dp, dm;
		CALC_DIRECTION(dp, dm);
		dmup = dmup * per + dp;
		dmdown = dmdown * per + dm;
		double di_up = dmup / atr;
		double di_down = dmdown / atr;
		double dm_diff = fabs(di_up - di_down);
		double dm_sum = di_up + di_down;
		double dx = dm_diff / dm_sum * 100;
		if(i-period < period-2) {
			adx += dx;
		}
		else if(i-period == period-2) {
			adx += dx;
			ti_buffer_qpush(adxr, adx * invper);
		}
		else {
			adx = adx * per + dx;
			if(i >= first_adxr) {
				*output++ = 0.5 * (adx * invper + ti_buffer_get(adxr, 1));
			}
			ti_buffer_qpush(adxr, adx * invper);
		}
	}
	ti_buffer_free(adxr);
	assert(output - outputs[0] == size - ti_adxr_start(options));
	return TI_OKAY;
}
//
// ao
int ti_ao_start(double const * options) { return 33; }

int ti_ao(int size, double const * const * inputs, double const * options, double * const * outputs) 
{
	const double * high = inputs[0];
	const double * low = inputs[1];
	const int period = 34;
	double * output = outputs[0];
	if(size <= ti_ao_start(options)) return TI_OKAY;
	double sum34 = 0;
	double sum5 = 0;
	const double per34 = 1.0 / 34.0;
	const double per5 = 1.0 / 5.0;
	int i;
	for(i = 0; i < 34; ++i) {
		double hl = 0.5 * (high[i] + low[i]);
		sum34 += hl;
		if(i >= 29) sum5 += hl;
	}
	*output++ = (per5 * sum5 - per34 * sum34);
	for(i = period; i < size; ++i) {
		double hl = 0.5 * (high[i] + low[i]);
		sum34 += hl;
		sum5 += hl;
		sum34 -= 0.5 * (high[i-34] + low[i-34]);
		sum5  -= 0.5 * (high[i-5] + low[i-5]);
		*output++ = (per5 * sum5 - per34 * sum34);
	}
	assert(output - outputs[0] == size - ti_ao_start(options));
	return TI_OKAY;
}
//
// apo
int ti_apo_start(double const * options) { return 1; }

int ti_apo(int size, double const * const * inputs, double const * options, double * const * outputs) 
{
	const double * input = inputs[0];
	double * apo = outputs[0];
	const int short_period = (int)options[0];
	const int long_period = (int)options[1];
	if(short_period < 1) return TI_INVALID_OPTION;
	if(long_period < 2) return TI_INVALID_OPTION;
	if(long_period < short_period) return TI_INVALID_OPTION;
	if(size <= ti_apo_start(options)) return TI_OKAY;
	double short_per = 2 / ((double)short_period + 1);
	double long_per = 2 / ((double)long_period + 1);
	double short_ema = input[0];
	double long_ema = input[0];
	int i;
	for(i = 1; i < size; ++i) {
		short_ema = (input[i]-short_ema) * short_per + short_ema;
		long_ema = (input[i]-long_ema) * long_per + long_ema;
		const double out = short_ema - long_ema;
		*apo++ = out;
	}
	assert(apo - outputs[0] == size - ti_apo_start(options));
	return TI_OKAY;
}
//
// aroon
int ti_aroon_start(double const * options) { return (int)options[0]; }

int ti_aroon(int size, double const * const * inputs, double const * options, double * const * outputs) 
{
	const double * high = inputs[0];
	const double * low = inputs[1];
	double * adown = outputs[0];
	double * aup = outputs[1];
	const int period = (int)options[0];
	if(period < 1) return TI_INVALID_OPTION;
	if(size <= ti_aroon_start(options)) return TI_OKAY;
	const double scale = 100.0 / period;
	int trail = 0, maxi = -1, mini = -1;
	double max = high[0];
	double min = low[0];
	double bar;
	int i, j;
	for(i = period; i < size; ++i, ++trail) {
		/* Maintain highest. */
		bar = high[i];
		if(maxi < trail) {
			maxi = trail;
			max = high[maxi];
			j = trail;
			while(++j <= i) {
				bar = high[j];
				if(bar >= max) {
					max = bar;
					maxi = j;
				}
			}
		}
		else if(bar >= max) {
			maxi = i;
			max = bar;
		}
		/* Maintain lowest. */
		bar = low[i];
		if(mini < trail) {
			mini = trail;
			min = low[mini];
			j = trail;
			while(++j <= i) {
				bar = low[j];
				if(bar <= min) {
					min = bar;
					mini = j;
				}
			}
		}
		else if(bar <= min) {
			mini = i;
			min = bar;
		}
		/* Calculate the indicator. */
		*adown++ = ((double)period - (i-mini)) * scale;
		*aup++ = ((double)period - (i-maxi)) * scale;
	}
	assert(adown - outputs[0] == size - ti_aroon_start(options));
	assert(aup - outputs[1] == size - ti_aroon_start(options));
	return TI_OKAY;
}
//
// aroonosc
int ti_aroonosc_start(double const * options) { return (int)options[0]; }

int ti_aroonosc(int size, double const * const * inputs, double const * options, double * const * outputs) 
{
	const double * high = inputs[0];
	const double * low = inputs[1];
	double * output = outputs[0];
	const int period = (int)options[0];
	if(period < 1) return TI_INVALID_OPTION;
	if(size <= ti_aroon_start(options)) return TI_OKAY;
	const double scale = 100.0 / period;
	int trail = 0, maxi = -1, mini = -1;
	double max = high[0];
	double min = low[0];
	int i, j;
	for(i = period; i < size; ++i, ++trail) {
		/* Maintain highest. */
		double bar = high[i];
		if(maxi < trail) {
			maxi = trail;
			max = high[maxi];
			j = trail;
			while(++j <= i) {
				bar = high[j];
				if(bar >= max) {
					max = bar;
					maxi = j;
				}
			}
		}
		else if(bar >= max) {
			maxi = i;
			max = bar;
		}

		/* Maintain lowest. */
		bar = low[i];
		if(mini < trail) {
			mini = trail;
			min = low[mini];
			j = trail;
			while(++j <= i) {
				bar = low[j];
				if(bar <= min) {
					min = bar;
					mini = j;
				}
			}
		}
		else if(bar <= min) {
			mini = i;
			min = bar;
		}

		/* Calculate the indicator. */
		/*
		    const double adown = ((double)period - (i-mini)) * scale;
		    const double aup = ((double)period - (i-maxi)) * scale;
		   *output = aup - adown
		   That simplifies to:
		    (maxi-mini) * scale
		 */

		*output++ = (maxi-mini) * scale;
	}
	assert(output - outputs[0] == size - ti_aroonosc_start(options));
	return TI_OKAY;
}
//
// bbands
int ti_bbands_start(double const * options) { return (int)options[0]-1; }

int ti_bbands(int size, double const * const * inputs, double const * options, double * const * outputs) 
{
	const double * input = inputs[0];
	double * lower = outputs[0];
	double * middle = outputs[1];
	double * upper = outputs[2];
	const int period = (int)options[0];
	const double stddev = options[1];
	const double div = 1.0 / period;
	if(period < 1) 
		return TI_INVALID_OPTION;
	if(size <= ti_bbands_start(options)) 
		return TI_OKAY;
	double sum = 0;
	double sum2 = 0;
	int i;
	for(i = 0; i < period; ++i) {
		sum += input[i];
		sum2 += input[i] * input[i];
	}
	double sd = sqrt(sum2 * div - (sum * div) * (sum * div));
	*middle = sum * div;
	*lower++ = *middle - stddev * sd;
	*upper++ = *middle + stddev * sd;
	++middle;
	for(i = period; i < size; ++i) {
		sum += input[i];
		sum2 += input[i] * input[i];
		sum -= input[i-period];
		sum2 -= input[i-period] * input[i-period];
		sd = sqrt(sum2 * div - (sum * div) * (sum * div));
		*middle = sum * div;
		*upper++ = *middle + stddev * sd;
		*lower++ = *middle - stddev * sd;
		++middle;
	}
	assert(lower - outputs[0] == size - ti_bbands_start(options));
	assert(middle - outputs[1] == size - ti_bbands_start(options));
	assert(upper - outputs[2] == size - ti_bbands_start(options));
	return TI_OKAY;
}
//
// cci
#define TYPPRICE(INDEX) ((high[(INDEX)] + low[(INDEX)] + close[(INDEX)]) * (1.0/3.0))

int ti_cci_start(double const * options) 
{
	const int period = (int)options[0];
	return (period-1) * 2;
}

int ti_cci(int size, double const * const * inputs, double const * options, double * const * outputs) 
{
	const double * high = inputs[0];
	const double * low = inputs[1];
	const double * close = inputs[2];
	const int period = (int)options[0];
	const double div = 1.0 / period;
	if(period < 1) return TI_INVALID_OPTION;
	if(size <= ti_cci_start(options)) return TI_OKAY;
	double * output = outputs[0];
	ti_buffer * sum = ti_buffer_new(period);
	int i, j;
	for(i = 0; i < size; ++i) {
		const double today = TYPPRICE(i);
		ti_buffer_push(sum, today);
		const double avg = sum->sum * div;
		if(i >= period * 2 - 2) {
			double acc = 0;
			for(j = 0; j < period; ++j) {
				acc += fabs(avg - sum->P_Vals[j]);
			}
			double cci = acc * div;
			cci *= .015;
			cci = (today-avg)/cci;
			*output++ = cci;
		}
	}
	ti_buffer_free(sum);
	assert(output - outputs[0] == size - ti_cci_start(options));
	return TI_OKAY;
}
//
// cmo
#define UPWARD(I) (input[(I)] > input[(I)-1] ? input[(I)] - input[(I)-1] : 0)
#define DOWNWARD(I) (input[(I)] < input[(I)-1] ? input[(I)-1] - input[(I)] : 0)

int ti_cmo_start(double const * options) { return (int)options[0]; }

int ti_cmo(int size, double const * const * inputs, double const * options, double * const * outputs) 
{
	const double * input = inputs[0];
	double * output = outputs[0];
	const int period = (int)options[0];
	if(period < 1) return TI_INVALID_OPTION;
	if(size <= ti_cmo_start(options)) return TI_OKAY;
	double up_sum = 0, down_sum = 0;
	int i;
	for(i = 1; i <= period; ++i) {
		up_sum += UPWARD(i);
		down_sum += DOWNWARD(i);
	}
	*output++ = 100 * (up_sum - down_sum) / (up_sum + down_sum);
	for(i = period+1; i < size; ++i) {
		up_sum -= UPWARD(i-period);
		down_sum -= DOWNWARD(i-period);
		up_sum += UPWARD(i);
		down_sum += DOWNWARD(i);
		*output++ = 100 * (up_sum - down_sum) / (up_sum + down_sum);
	}
	assert(output - outputs[0] == size - ti_cmo_start(options));
	return TI_OKAY;
}
//
// crossany
int ti_crossany_start(double const * options) { return 1; }

int ti_crossany(int size, double const * const * inputs, double const * options, double * const * outputs) 
{
	const double * a = inputs[0];
	const double * b = inputs[1];
	double * output = outputs[0];
	for(int i = 1; i < size; ++i) {
		*output++ = (a[i] > b[i] && a[i-1] <= b[i-1]) || (a[i] < b[i] && a[i-1] >= b[i-1]);
	}
	return TI_OKAY;
}
//
// crossover
int ti_crossover_start(double const * options) { return 1; }

int ti_crossover(int size, double const * const * inputs, double const * options, double * const * outputs) 
{
	const double * a = inputs[0];
	const double * b = inputs[1];
	double * output = outputs[0];
	for(int i = 1; i < size; ++i) {
		*output++ = a[i] > b[i] && a[i-1] <= b[i-1];
	}
	return TI_OKAY;
}
//
// cvi
int ti_cvi_start(double const * options) 
{
	const int n = (int)options[0];
	return n*2-1;
}

int ti_cvi(int size, double const * const * inputs, double const * options, double * const * outputs) 
{
	const double * high = inputs[0];
	const double * low = inputs[1];
	const int period = (int)options[0];
	double * output = outputs[0];
	if(period < 1) return TI_INVALID_OPTION;
	if(size <= ti_cvi_start(options)) return TI_OKAY;
	const double per = 2 / ((double)period + 1);
	ti_buffer * lag = ti_buffer_new(period);
	double val = high[0]-low[0];
	int i;
	for(i = 1; i < period*2-1; ++i) {
		val = ((high[i]-low[i])-val) * per + val;
		ti_buffer_qpush(lag, val);
	}
	for(i = period*2-1; i < size; ++i) {
		val = ((high[i]-low[i])-val) * per + val;
		const double old = lag->P_Vals[lag->index];
		*output++ = 100.0 * (val - old) / old;
		ti_buffer_qpush(lag, val);
	}
	ti_buffer_free(lag);
	assert(output - outputs[0] == size - ti_cvi_start(options));
	return TI_OKAY;
}
//
// tr (True range)
int ti_tr_start(double const * options) { return 0; }

int ti_tr(int size, double const * const * inputs, double const * options, double * const * outputs) 
{
	const double * high = inputs[0];
	const double * low = inputs[1];
	const double * close = inputs[2];
	double * output = outputs[0];
	double truerange;
	output[0] = high[0] - low[0];
	for(int i = 1; i < size; ++i) {
		CALC_TRUERANGE();
		output[i] = truerange;
	}
	return TI_OKAY;
}
//
// atr (Average true range)
int ti_atr_start(double const * options) { return (int)options[0]-1; }

int ti_atr(int size, double const * const * inputs, double const * options, double * const * outputs) 
{
	const double * high = inputs[0];
	const double * low = inputs[1];
	const double * close = inputs[2];
	const int period = (int)options[0];
	double * output = outputs[0];
	if(period < 1) return TI_INVALID_OPTION;
	if(size <= ti_atr_start(options)) return TI_OKAY;
	const double per = 1.0 / ((double)period);
	double sum = 0;
	double truerange;
	sum += high[0] - low[0];
	int i;
	for(i = 1; i < period; ++i) {
		CALC_TRUERANGE();
		sum += truerange;
	}
	double val = sum / period;
	*output++ = val;
	for(i = period; i < size; ++i) {
		CALC_TRUERANGE();
		val = (truerange-val) * per + val;
		*output++ = val;
	}
	assert(output - outputs[0] == size - ti_atr_start(options));
	return TI_OKAY;
}
//
// avgprice
int ti_avgprice_start(double const * options) { return 0; }

int ti_avgprice(int size, double const * const * inputs, double const * options, double * const * outputs) 
{
	const double * open = inputs[0];
	const double * high = inputs[1];
	const double * low = inputs[2];
	const double * close = inputs[3];
	double * output = outputs[0];
	for(int i = 0; i < size; ++i) {
		output[i] = (open[i] + high[i] + low[i] + close[i]) * 0.25;
	}
	return TI_OKAY;
}
//
// bop
int ti_bop_start(double const * options) { return 0; }

int ti_bop(int size, double const * const * inputs, double const * options, double * const * outputs) 
{
	const double * open = inputs[0];
	const double * high = inputs[1];
	const double * low = inputs[2];
	const double * close = inputs[3];
	double * output = outputs[0];
	int i;
	for(i = 0; i < size; ++i) {
		double hl = high[i] - low[i];
		if(hl <= 0.0) {
			output[i] = 0;
		}
		else {
			output[i] = (close[i] - open[i]) / hl;
		}
	}
	return TI_OKAY;
}
//
// decay
int ti_decay_start(double const * options) { return 0; }

int ti_decay(int size, double const * const * inputs, double const * options, double * const * outputs) 
{
	const double * input = inputs[0];
	double * output = outputs[0];
	const int period = (int)options[0];
	const double div = 1.0 / period;
	*output++ = input[0];
	for(int i = 1; i < size; ++i) {
		double d = output[-1] - div;
		*output++ = input[i] > d ? input[i] : d;
	}
	return TI_OKAY;
}
//
// edecay
int ti_edecay_start(double const * options) { return 0; }

int ti_edecay(int size, double const * const * inputs, double const * options, double * const * outputs) 
{
	const double * input = inputs[0];
	const int period = (int)options[0];
	double * output = outputs[0];
	const double div = 1.0 - 1.0 / period;
	*output++ = input[0];
	for(int i = 1; i < size; ++i) {
		double d = output[-1] * div;
		*output++ = input[i] > d ? input[i] : d;
	}
	return TI_OKAY;
}
//
// dema
int ti_dema_start(double const * options) 
{
	const int period = (int)options[0];
	return (period-1) * 2;
}

int ti_dema(int size, double const * const * inputs, double const * options, double * const * outputs) 
{
	const double * input = inputs[0];
	const int period = (int)options[0];
	double * output = outputs[0];
	if(period < 1) return TI_INVALID_OPTION;
	if(size <= ti_dema_start(options)) return TI_OKAY;
	const double per = 2 / ((double)period + 1);
	const double per1 = 1.0 - per;
	/*Calculate EMA(input)*/
	double ema = input[0];
	/*Calculate EMA(EMA(input))*/
	double ema2 = ema;
	int i;
	for(i = 0; i < size; ++i) {
		ema = ema * per1 + input[i] * per;
		if(i == period-1) {
			ema2 = ema;
		}
		if(i >= period-1) {
			ema2 = ema2 * per1 + ema * per;
			if(i >= (period-1) * 2) {
				*output = ema * 2 - ema2;
				++output;
			}
		}
	}
	assert(output - outputs[0] == size - ti_dema_start(options));
	return TI_OKAY;
}
//
// di
int ti_di_start(double const * options) { return (int)options[0]-1; }

int ti_di(int size, double const * const * inputs, double const * options, double * const * outputs) 
{
	const double * high = inputs[0];
	const double * low = inputs[1];
	const double * close = inputs[2];
	const int period = (int)options[0];
	double * plus_di = outputs[0];
	double * minus_di = outputs[1];
	if(period < 1) return TI_INVALID_OPTION;
	if(size <= ti_di_start(options)) return TI_OKAY;
	const double per = ((double)period-1) / ((double)period);
	double atr = 0;
	double dmup = 0;
	double dmdown = 0;
	int i;
	for(i = 1; i < period; ++i) {
		double truerange;
		CALC_TRUERANGE();
		atr += truerange;
		double dp, dm;
		CALC_DIRECTION(dp, dm);
		dmup += dp;
		dmdown += dm;
	}
	*plus_di++  = 100.0 * dmup / atr;
	*minus_di++ = 100.0 * dmdown / atr;
	for(i = period; i < size; ++i) {
		double truerange;
		CALC_TRUERANGE();
		atr = atr * per + truerange;
		double dp, dm;
		CALC_DIRECTION(dp, dm);
		dmup = dmup * per + dp;
		dmdown = dmdown * per + dm;
		*plus_di++  = 100.0 * dmup / atr;
		*minus_di++ = 100.0 * dmdown / atr;
	}
	assert(plus_di - outputs[0] == size - ti_di_start(options));
	assert(minus_di - outputs[1] == size - ti_di_start(options));
	return TI_OKAY;
}
//
// dm
int ti_dm_start(double const * options) { return (int)options[0]-1; }

int ti_dm(int size, double const * const * inputs, double const * options, double * const * outputs) 
{
	const double * high = inputs[0];
	const double * low = inputs[1];
	const int period = (int)options[0];
	double * plus_dm = outputs[0];
	double * minus_dm = outputs[1];
	if(period < 1) 
		return TI_INVALID_OPTION;
	if(size <= ti_dm_start(options)) 
		return TI_OKAY;
	const double per = ((double)period-1) / ((double)period);
	double dmup = 0;
	double dmdown = 0;
	int i;
	for(i = 1; i < period; ++i) {
		double dp, dm;
		CALC_DIRECTION(dp, dm);
		dmup += dp;
		dmdown += dm;
	}
	*plus_dm++ = dmup;
	*minus_dm++ = dmdown;
	for(i = period; i < size; ++i) {
		double dp, dm;
		CALC_DIRECTION(dp, dm);
		dmup = dmup * per + dp;
		dmdown = dmdown * per + dm;
		*plus_dm++ = dmup;
		*minus_dm++ = dmdown;
	}
	assert(plus_dm - outputs[0] == size - ti_dm_start(options));
	assert(minus_dm - outputs[1] == size - ti_dm_start(options));
	return TI_OKAY;
}
//
// dx
int ti_dx_start(double const * options) { return (int)options[0]-1; }

int ti_dx(int size, double const * const * inputs, double const * options, double * const * outputs) 
{
	const double * high = inputs[0];
	const double * low = inputs[1];
	const double * close = inputs[2];
	const int period = (int)options[0];
	double * output = outputs[0];
	if(period < 1) 
		return TI_INVALID_OPTION;
	if(size <= ti_dx_start(options)) 
		return TI_OKAY;
	const double per = ((double)period-1) / ((double)period);
	double atr = 0;
	double dmup = 0;
	double dmdown = 0;
	int i;
	for(i = 1; i < period; ++i) {
		double truerange;
		CALC_TRUERANGE();
		atr += truerange;
		double dp, dm;
		CALC_DIRECTION(dp, dm);
		dmup += dp;
		dmdown += dm;
	}
	{
		double di_up = dmup / atr;
		double di_down = dmdown / atr;
		double dm_diff = fabs(di_up - di_down);
		double dm_sum = di_up + di_down;
		double dx = dm_diff / dm_sum * 100;
		*output++ = dx;
	}
	for(i = period; i < size; ++i) {
		double truerange;
		CALC_TRUERANGE();
		atr = atr * per + truerange;
		double dp, dm;
		CALC_DIRECTION(dp, dm);
		dmup = dmup * per + dp;
		dmdown = dmdown * per + dm;
		double di_up = dmup / atr;
		double di_down = dmdown / atr;
		double dm_diff = fabs(di_up - di_down);
		double dm_sum = di_up + di_down;
		double dx = dm_diff / dm_sum * 100;
		*output++ = dx;
	}
	assert(output - outputs[0] == size - ti_dx_start(options));
	return TI_OKAY;
}
//
// natr
int ti_natr_start(double const * options) { return (int)options[0]-1; }

int ti_natr(int size, double const * const * inputs, double const * options, double * const * outputs) 
{
	const double * high = inputs[0];
	const double * low = inputs[1];
	const double * close = inputs[2];
	const int period = (int)options[0];
	double * output = outputs[0];
	if(period < 1) 
		return TI_INVALID_OPTION;
	if(size <= ti_natr_start(options)) 
		return TI_OKAY;
	const double per = 1.0 / ((double)period);
	double sum = 0;
	double truerange;
	sum += high[0] - low[0];
	int i;
	for(i = 1; i < period; ++i) {
		CALC_TRUERANGE();
		sum += truerange;
	}
	double val = sum / period;
	*output++ = 100 * (val) / close[period-1];
	for(i = period; i < size; ++i) {
		CALC_TRUERANGE();
		val = (truerange-val) * per + val;
		*output++ = 100 * (val) / close[i];
	}
	assert(output - outputs[0] == size - ti_natr_start(options));
	return TI_OKAY;
}
//
// dpo
int ti_dpo_start(double const * options) { return (int)options[0]-1; }

int ti_dpo(int size, double const * const * inputs, double const * options, double * const * outputs) 
{
	const double * input = inputs[0];
	const int period = (int)options[0];
	const int back = period / 2 + 1;
	double * output = outputs[0];
	const double div = 1.0 / period;
	if(period < 1) return TI_INVALID_OPTION;
	if(size <= ti_dpo_start(options)) return TI_OKAY;
	double sum = 0;
	int i;
	for(i = 0; i < period; ++i) {
		sum += input[i];
	}
	*output++ = input[period-1-back] - (sum * div);
	for(i = period; i < size; ++i) {
		sum += input[i];
		sum -= input[i-period];
		*output++ = input[i-back] - (sum * div);
	}
	assert(output - outputs[0] == size - ti_dpo_start(options));
	return TI_OKAY;
}
//
// ema
int ti_ema_start(double const * options) { return 0; }

int ti_ema(int size, double const * const * inputs, double const * options, double * const * outputs) 
{
	const double * input = inputs[0];
	const int period = (int)options[0];
	double * output = outputs[0];
	if(period < 1) return TI_INVALID_OPTION;
	if(size <= ti_ema_start(options)) return TI_OKAY;
	const double per = 2 / ((double)period + 1);
	double val = input[0];
	*output++ = val;
	for(int i = 1; i < size; ++i) {
		val = (input[i]-val) * per + val;
		*output++ = val;
	}
	assert(output - outputs[0] == size - ti_ema_start(options));
	return TI_OKAY;
}
//
// emv
int ti_emv_start(double const * options) { return 1; }

int ti_emv(int size, double const * const * inputs, double const * options, double * const * outputs) 
{
	const double * high = inputs[0];
	const double * low = inputs[1];
	const double * volume = inputs[2];
	double * output = outputs[0];
	if(size <= ti_emv_start(options)) return TI_OKAY;
	double last = (high[0] + low[0]) * 0.5;
	for(int i = 1; i < size; ++i) {
		double hl = (high[i] + low[i]) * 0.5;
		double br = volume[i] / 10000.0 / (high[i] - low[i]);
		*output++ = (hl - last) / br;
		last = hl;
	}
	assert(output - outputs[0] == size - ti_emv_start(options));
	return TI_OKAY;
}
//
// fisher
int ti_fisher_start(double const * options) { return (int)options[0]-1; }

#define HL(X) (0.5 * (high[(X)] + low[(X)]))

int ti_fisher(int size, double const * const * inputs, double const * options, double * const * outputs) 
{
	const double * high = inputs[0];
	const double * low = inputs[1];
	double * fisher = outputs[0];
	double * signal = outputs[1];
	const int period = (int)options[0];
	if(period < 1) return TI_INVALID_OPTION;
	if(size <= ti_fisher_start(options)) return TI_OKAY;
	int trail = 0, maxi = -1, mini = -1;
	double max = HL(0);
	double min = HL(0);
	double val1 = 0.0;
	double bar;
	double fish = 0.0;
	int i, j;
	for(i = period-1; i < size; ++i, ++trail) {
		/* Maintain highest. */
		bar = HL(i);
		if(maxi < trail) {
			maxi = trail;
			max = HL(maxi);
			j = trail;
			while(++j <= i) {
				bar = HL(j);
				if(bar >= max) {
					max = bar;
					maxi = j;
				}
			}
		}
		else if(bar >= max) {
			maxi = i;
			max = bar;
		}
		/* Maintain lowest. */
		bar = HL(i);
		if(mini < trail) {
			mini = trail;
			min = HL(mini);
			j = trail;
			while(++j <= i) {
				bar = HL(j);
				if(bar <= min) {
					min = bar;
					mini = j;
				}
			}
		}
		else if(bar <= min) {
			mini = i;
			min = bar;
		}
		double mm = max - min;
		if(mm == 0.0) mm = 0.001;
		val1 = 0.33 * 2.0 * ( (HL(i)-min) / (mm) - 0.5) + 0.67 * val1;
		if(val1 > 0.99) val1 = .999;
		if(val1 < -0.99) val1 = -.999;
		*signal++ = fish;
		fish = 0.5 * log((1.0+val1)/(1.0-val1)) + 0.5 * fish;
		*fisher++ = fish;
	}
	assert(fisher - outputs[0] == size - ti_fisher_start(options));
	assert(signal - outputs[1] == size - ti_fisher_start(options));
	return TI_OKAY;
}
//
// kvo
int ti_kvo_start(double const * options) { return 1; }

int ti_kvo(int size, double const * const * inputs, double const * options, double * const * outputs) 
{
	const double * high = inputs[0];
	const double * low = inputs[1];
	const double * close = inputs[2];
	const double * volume = inputs[3];
	const int short_period = (int)options[0];
	const int long_period = (int)options[1];
	if(short_period < 1) 
		return TI_INVALID_OPTION;
	if(long_period < short_period) 
		return TI_INVALID_OPTION;
	if(size <= ti_kvo_start(options)) 
		return TI_OKAY;
	const double short_per = 2 / ((double)short_period + 1);
	const double long_per = 2 / ((double)long_period + 1);
	double * output = outputs[0];
	double cm = 0;
	double prev_hlc = high[0] + low[0] + close[0];
	int trend = -1;
	double short_ema = 0, long_ema = 0;
	for(int i = 1; i < size; ++i) {
		const double hlc = high[i] + low[i] + close[i];
		const double dm = high[i] - low[i];
		if(hlc > prev_hlc && trend != 1) {
			trend = 1;
			cm = high[i-1] - low[i-1];
		}
		else if(hlc < prev_hlc && trend != 0) {
			trend = 0;
			cm = high[i-1] - low[i-1];
		}
		cm += dm;
		const double vf = volume[i] * fabs(dm / cm * 2 - 1) * 100 * (trend ? 1.0 : -1.0);
		if(i == 1) {
			short_ema = vf;
			long_ema = vf;
		}
		else {
			short_ema = (vf-short_ema) * short_per + short_ema;
			long_ema = (vf-long_ema) * long_per + long_ema;
		}
		*output++ = short_ema - long_ema;
		prev_hlc = hlc;
	}
	assert(output - outputs[0] == size - ti_kvo_start(options));
	return TI_OKAY;
}
//
// lag
int ti_lag_start(double const * options) { return (int)options[0]; }

int ti_lag(int size, double const * const * inputs, double const * options, double * const * outputs) 
{
	const double * input = inputs[0];
	const int period = (int)options[0];
	double * output = outputs[0];
	if(period < 0) 
		return TI_INVALID_OPTION;
	if(size <= ti_lag_start(options)) 
		return TI_OKAY;
	for(int i = period; i < size; ++i) {
		*output++ = input[i-period];
	}
	assert(output - outputs[0] == size - ti_lag_start(options));
	return TI_OKAY;
}
//
// hma
int ti_hma_start(double const * options) 
{
	const int period = (int)options[0];
	const int periodsqrt = (int)sqrt((double)period);
	return period + periodsqrt - 2;
}

int ti_hma(int size, double const * const * inputs, double const * options, double * const * outputs) 
{
	const double * input = inputs[0];
	const int period = (int)options[0];
	double * output = outputs[0];
	if(period < 1) 
		return TI_INVALID_OPTION;
	if(size <= ti_hma_start(options)) 
		return TI_OKAY;
	/* HMA(input, N) = WMA((2 * WMA(input, N/2) - WMA(input, N)), sqrt(N)) */
	/* Need to do three WMAs, with periods N, N/2, and sqrt N.*/
	const int period2 = (int)(period / 2);
	const int periodsqrt = (int)sqrt((double)period);
	const double weights = period * (period+1) / 2;
	const double weights2 = period2 * (period2+1) / 2;
	const double weightssqrt = periodsqrt * (periodsqrt+1) / 2;
	double sum = 0; /* Flat sum of previous numbers. */
	double weight_sum = 0; /* Weighted sum of previous numbers. */
	double sum2 = 0;
	double weight_sum2 = 0;
	double sumsqrt = 0;
	double weight_sumsqrt = 0;
	/* Setup up the WMA(period) and WMA(period/2) on the input. */
	int i;
	for(i = 0; i < period-1; ++i) {
		weight_sum += input[i] * (i+1);
		sum += input[i];
		if(i >= period - period2) {
			weight_sum2 += input[i] * (i+1-(period-period2));
			sum2 += input[i];
		}
	}
	ti_buffer * buff = ti_buffer_new(periodsqrt);
	for(i = period-1; i < size; ++i) {
		weight_sum += input[i] * period;
		sum += input[i];
		weight_sum2 += input[i] * period2;
		sum2 += input[i];
		const double wma = weight_sum / weights;
		const double wma2 = weight_sum2 / weights2;
		const double diff = 2 * wma2 - wma;
		weight_sumsqrt += diff * periodsqrt;
		sumsqrt += diff;
		ti_buffer_qpush(buff, diff);
		if(i >= (period-1) + (periodsqrt-1)) {
			*output++ = weight_sumsqrt / weightssqrt;
			weight_sumsqrt -= sumsqrt;
			sumsqrt -= ti_buffer_get(buff, 1);
		}
		else {
			weight_sumsqrt -= sumsqrt;
		}
		weight_sum -= sum;
		sum -= input[i-period+1];
		weight_sum2 -= sum2;
		sum2 -= input[i-period2+1];
	}
	ti_buffer_free(buff);
	assert(output - outputs[0] == size - ti_hma_start(options));
	return TI_OKAY;
}
//
// kama
int ti_kama_start(double const * options) { return (int)options[0]-1; }

int ti_kama(int size, double const * const * inputs, double const * options, double * const * outputs) 
{
	const double * input = inputs[0];
	const int period = (int)options[0];
	double * output = outputs[0];
	if(period < 1) 
		return TI_INVALID_OPTION;
	if(size <= ti_kama_start(options)) 
		return TI_OKAY;
	/* The caller selects the period used in the efficiency ratio.
	 * The fast and slow periods are hard set by the algorithm. */
	const double short_per = 2 / (2.0 + 1);
	const double long_per = 2 / (30.0 + 1);
	double sum = 0;
	int i;
	for(i = 1; i < period; ++i) {
		sum += fabs(input[i] - input[i-1]);
	}
	double kama = input[period-1];
	*output++ = kama;
	double er, sc;
	for(i = period; i < size; ++i) {
		sum += fabs(input[i] - input[i-1]);
		if(i > period) {
			sum -= fabs(input[i-period] - input[i-period-1]);
		}
		if(sum != 0.0) {
			er = fabs(input[i] - input[i-period]) / sum;
		}
		else {
			er = 1.0;
		}
		sc = pow(er * (short_per - long_per) + long_per, 2);
		kama = kama + sc * (input[i] - kama);
		*output++ = kama;
	}
	assert(output - outputs[0] == size - ti_kama_start(options));
	return TI_OKAY;
}
//
// sma
int ti_sma_start(double const * options) { return (int)options[0]-1; }

int ti_sma(int size, double const * const * inputs, double const * options, double * const * outputs) 
{
	const double * input = inputs[0];
	const int period = (int)options[0];
	double * output = outputs[0];
	const double div = 1.0 / period;
	if(period < 1) return TI_INVALID_OPTION;
	if(size <= ti_sma_start(options)) return TI_OKAY;
	double sum = 0;
	int i;
	for(i = 0; i < period; ++i) {
		sum += input[i];
	}
	*output++ = sum * div;
	for(i = period; i < size; ++i) {
		sum += input[i];
		sum -= input[i-period];
		*output++ = sum * div;
	}
	assert(output - outputs[0] == size - ti_sma_start(options));
	return TI_OKAY;
}
//
// tema
int ti_tema_start(double const * options) 
{
	const int period = (int)options[0];
	return (period-1) * 3;
}

int ti_tema(int size, double const * const * inputs, double const * options, double * const * outputs) 
{
	const double * input = inputs[0];
	const int period = (int)options[0];
	double * output = outputs[0];
	if(period < 1) return TI_INVALID_OPTION;
	if(size <= ti_tema_start(options)) return TI_OKAY;
	const double per = 2 / ((double)period + 1);
	const double per1 = 1.0 - per;
	/*Calculate EMA(input)*/
	double ema = input[0];
	/*Calculate EMA(EMA(input))*/
	double ema2 = 0;
	/*Calculate EMA(EMA(EMA(input)))*/
	double ema3 = 0;
	for(int i = 0; i < size; ++i) {
		ema = ema * per1 + input[i] * per;
		if(i == period-1) {
			ema2 = ema;
		}
		if(i >= period-1) {
			ema2 = ema2 * per1 + ema * per;
			if(i == (period-1) * 2) {
				ema3 = ema2;
			}
			if(i >= (period-1) * 2) {
				ema3 = ema3 * per1 + ema2 * per;
				if(i >= (period-1) * 3) {
					*output = 3 * ema - 3 * ema2 + ema3;
					++output;
				}
			}
		}
	}
	assert(output - outputs[0] == size - ti_tema_start(options));
	return TI_OKAY;
}
//
// trima
int ti_trima_start(double const * options) { return (int)options[0]-1; }

int ti_trima(int size, double const * const * inputs, double const * options, double * const * outputs) 
{
	const double * input = inputs[0];
	const int period = (int)options[0];
	double * output = outputs[0];
	if(period < 1) return TI_INVALID_OPTION;
	if(size <= ti_trima_start(options)) return TI_OKAY;
	if(period <= 2) return ti_sma(size, inputs, options, outputs);
	/* Weights for 6 period TRIMA:
	 * 1 2 3 3 2 1 = 12
	 *
	 * Weights for 7 period TRIMA:
	 * 1 2 3 4 3 2 1 = 16
	 */
	double weights = 1 / (double)((period%2) ? ((period/2+1) * (period/2+1)) : ((period/2+1) * (period/2)));
	double weight_sum = 0; /* Weighted sum of previous numbers, spans one period back. */
	double lead_sum = 0; /* Flat sum of most recent numbers. */
	double trail_sum = 0; /* Flat sum of oldest numbers. */
	/* example for period of 9 */
	/* weight_sum       1 2 3 4 5 4 3 2 1 */
	/* lead_sum                   1 1 1 1 */
	/* trail_sum        1 1 1 1 1        */
	const int lead_period = period%2 ? period/2 : period/2-1;
	const int trail_period = lead_period + 1;
	int i, w = 1;
	/* Initialize until before the first value. */
	for(i = 0; i < period-1; ++i) {
		weight_sum += input[i] * w;
		if(i+1 > period-lead_period) lead_sum += input[i];
		if(i+1 <= trail_period) trail_sum += input[i];
		if(i+1 < trail_period) ++w;
		if(i+1 >= period-lead_period) --w;
	}
	int lsi = (period-1)-lead_period+1;
	int tsi1 = (period-1)-period+1+trail_period;
	int tsi2 = (period-1)-period+1;
	for(i = period-1; i < size; ++i) {
		weight_sum += input[i];
		*output++ = weight_sum * weights;
		lead_sum += input[i];
		/* 1 2 3 4 5 4 3 2 1 */
		weight_sum += lead_sum;
		/* 1 2 3 4 5 5 4 3 2 */
		weight_sum -= trail_sum;
		/*   1 2 3 4 5 4 3 2 */
		/* weight_sum       1 2 3 4 5 4 3 2 1 */
		/* lead_sum                   1 1 1 1 */
		/* trail_sum        1 1 1 1 1        */
		lead_sum -= input[lsi++];
		trail_sum += input[tsi1++];
		trail_sum -= input[tsi2++];
	}
	assert(output - outputs[0] == size - ti_trima_start(options));
	return TI_OKAY;
}
//
// macd
int ti_macd_start(double const * options) 
{
	/* NB we return data before signal is strictly valid. */
	const int long_period = (int)options[1];
	return (long_period-1);
}

int ti_macd(int size, double const * const * inputs, double const * options, double * const * outputs) 
{
	const double * input = inputs[0];
	double * macd = outputs[0];
	double * signal = outputs[1];
	double * hist = outputs[2];
	const int short_period = (int)options[0];
	const int long_period = (int)options[1];
	const int signal_period = (int)options[2];
	if(short_period < 1) return TI_INVALID_OPTION;
	if(long_period < 2) return TI_INVALID_OPTION;
	if(long_period < short_period) return TI_INVALID_OPTION;
	if(signal_period < 1) return TI_INVALID_OPTION;
	if(size <= ti_macd_start(options)) return TI_OKAY;
	double short_per = 2 / ((double)short_period + 1);
	double long_per = 2 / ((double)long_period + 1);
	double signal_per = 2 / ((double)signal_period + 1);
	if(short_period == 12 && long_period == 26) {
		/* I don't like this, but it's what people expect. */
		short_per = 0.15;
		long_per = 0.075;
	}
	double short_ema = input[0];
	double long_ema = input[0];
	double signal_ema = 0;
	for(int i = 1; i < size; ++i) {
		short_ema = (input[i]-short_ema) * short_per + short_ema;
		long_ema = (input[i]-long_ema) * long_per + long_ema;
		const double out = short_ema - long_ema;
		if(i == long_period-1) {
			signal_ema = out;
		}
		if(i >= long_period-1) {
			signal_ema = (out-signal_ema) * signal_per + signal_ema;
			*macd++ = out;
			*signal++ = signal_ema;
			*hist++ = out - signal_ema;
		}
	}
	assert(macd - outputs[0] == size - ti_macd_start(options));
	assert(signal - outputs[1] == size - ti_macd_start(options));
	assert(hist - outputs[2] == size - ti_macd_start(options));
	return TI_OKAY;
}
//
// marketfi
int ti_marketfi_start(double const * options) { return 0; }

int ti_marketfi(int size, double const * const * inputs, double const * options, double * const * outputs) 
{
	const double * high = inputs[0];
	const double * low = inputs[1];
	const double * volume = inputs[2];
	double * output = outputs[0];
	if(size <= ti_marketfi_start(options)) 
		return TI_OKAY;
	for(int i = 0; i < size; ++i) {
		*output++ = (high[i] - low[i]) / volume[i];
	}
	assert(output - outputs[0] == size - ti_marketfi_start(options));
	return TI_OKAY;
}
//
// mass
int ti_mass_start(double const * options) 
{
	int sum_p = (int)options[0]-1;
	/* The ema uses a hard-coded period of 9. (9-1)*2 = 16 */
	return 16 + sum_p;
}

int ti_mass(int size, double const * const * inputs, double const * options, double * const * outputs) 
{
	const double * high = inputs[0];
	const double * low = inputs[1];
	const int period = (int)options[0];
	double * output = outputs[0];
	if(period < 1) return TI_INVALID_OPTION;
	if(size <= ti_mass_start(options)) return TI_OKAY;
	/*mass uses a hard-coded 9 period for the ema*/
	const double per = 2 / (9.0 + 1);
	const double per1 = 1.0 - per;
	/*Calculate EMA(h-l)*/
	double ema = high[0] - low[0];
	/*Calculate EMA(EMA(h-l))*/
	double ema2 = ema;
	ti_buffer * sum = ti_buffer_new(period);
	int i;
	for(i = 0; i < size; ++i) {
		double hl = high[i] - low[i];
		ema = ema * per1 + hl * per;
		if(i == 8) {
			ema2 = ema;
		}
		if(i >= 8) {
			ema2 = ema2 * per1 + ema * per;
			if(i >= 16) {
				ti_buffer_push(sum, ema/ema2);
				if(i >= 16 + period - 1) {
					*output++ = sum->sum;
				}
			}
		}
	}
	ti_buffer_free(sum);
	assert(output - outputs[0] == size - ti_mass_start(options));
	return TI_OKAY;
}
//
// md
int ti_md_start(double const * options) { return (int)options[0]-1; }

int ti_md(int size, double const * const * inputs, double const * options, double * const * outputs) 
{
	const double * input = inputs[0];
	const int period = (int)options[0];
	double * output = outputs[0];
	const double div = 1.0 / period;
	if(period < 1) 
		return TI_INVALID_OPTION;
	if(size <= ti_md_start(options)) 
		return TI_OKAY;
	double sum = 0;
	for(int i = 0; i < size; ++i) {
		const double today = input[i];
		sum += today;
		if(i >= period) 
			sum -= input[i-period];
		const double avg = sum * div;
		if(i >= period - 1) {
			double acc = 0;
			for(int j = 0; j < period; ++j) {
				acc += fabs(avg - input[i-j]);
			}
			*output++ = acc * div;
		}
	}
	assert(output - outputs[0] == size - ti_md_start(options));
	return TI_OKAY;
}
//
// medprice
int ti_medprice_start(double const * options) { return 0; }

int ti_medprice(int size, double const * const * inputs, double const * options, double * const * outputs) 
{
	const double * high = inputs[0];
	const double * low = inputs[1];
	double * output = outputs[0];
	for(int i = 0; i < size; ++i) {
		output[i] = (high[i] + low[i]) * 0.5;
	}
	return TI_OKAY;
}
//
// mfi
#define TYPPRICE(INDEX) ((high[(INDEX)] + low[(INDEX)] + close[(INDEX)]) * (1.0/3.0))

int ti_mfi_start(double const * options) { return (int)options[0]; }

int ti_mfi(int size, double const * const * inputs, double const * options, double * const * outputs) 
{
	const double * high = inputs[0];
	const double * low = inputs[1];
	const double * close = inputs[2];
	const double * volume = inputs[3];
	const int period = (int)options[0];
	if(period < 1) return TI_INVALID_OPTION;
	if(size <= ti_mfi_start(options)) return TI_OKAY;
	double * output = outputs[0];
	double ytyp = TYPPRICE(0);
	ti_buffer * up = ti_buffer_new(period);
	ti_buffer * down = ti_buffer_new(period);
	for(int i = 1; i < size; ++i) {
		const double typ = TYPPRICE(i);
		const double bar = typ * volume[i];
		if(typ > ytyp) {
			ti_buffer_push(up, bar);
			ti_buffer_push(down, 0.0);
		}
		else if(typ < ytyp) {
			ti_buffer_push(down, bar);
			ti_buffer_push(up, 0.0);
		}
		else {
			ti_buffer_push(up, 0.0);
			ti_buffer_push(down, 0.0);
		}
		ytyp = typ;
		if(i >= period) {
			*output++ = up->sum / (up->sum + down->sum) * 100.0;
		}
	}
	ti_buffer_free(up);
	ti_buffer_free(down);
	assert(output - outputs[0] == size - ti_mfi_start(options));
	return TI_OKAY;
}
//
// max
int ti_max_start(double const * options) { return (int)options[0]-1; }

int ti_max(int size, double const * const * inputs, double const * options, double * const * outputs) 
{
	const double * input = inputs[0];
	const int period = (int)options[0];
	double * output = outputs[0];
	if(period < 1) 
		return TI_INVALID_OPTION;
	if(size <= ti_max_start(options)) 
		return TI_OKAY;
	int trail = 0, maxi = -1;
	double max = input[0];
	int i, j;
	for(i = period-1; i < size; ++i, ++trail) {
		double bar = input[i];
		if(maxi < trail) {
			maxi = trail;
			max = input[maxi];
			j = trail;
			while(++j <= i) {
				bar = input[j];
				if(bar >= max) {
					max = bar;
					maxi = j;
				}
			}
		}
		else if(bar >= max) {
			maxi = i;
			max = bar;
		}
		*output++ = max;
	}
	assert(output - outputs[0] == size - ti_max_start(options));
	return TI_OKAY;
}
//
// min
int ti_min_start(double const * options) { return (int)options[0]-1; }

int ti_min(int size, double const * const * inputs, double const * options, double * const * outputs) 
{
	const double * input = inputs[0];
	const int period = (int)options[0];
	double * output = outputs[0];
	if(period < 1) return TI_INVALID_OPTION;
	if(size <= ti_min_start(options)) return TI_OKAY;
	int trail = 0, mini = -1;
	double min = input[0];
	int i, j;
	for(i = period-1; i < size; ++i, ++trail) {
		double bar = input[i];
		if(mini < trail) {
			mini = trail;
			min = input[mini];
			j = trail;
			while(++j <= i) {
				bar = input[j];
				if(bar <= min) {
					min = bar;
					mini = j;
				}
			}
		}
		else if(bar <= min) {
			mini = i;
			min = bar;
		}
		*output++ = min;
	}
	assert(output - outputs[0] == size - ti_min_start(options));
	return TI_OKAY;
}
//
// mom
int ti_mom_start(double const * options) { return (int)options[0]; }

int ti_mom(int size, double const * const * inputs, double const * options, double * const * outputs) 
{
	const double * input = inputs[0];
	const int period = (int)options[0];
	double * output = outputs[0];
	if(period < 1) 
		return TI_INVALID_OPTION;
	if(size <= ti_mom_start(options)) 
		return TI_OKAY;
	for(int i = period; i < size; ++i) {
		*output++ = input[i] - input[i-period];
	}
	assert(output - outputs[0] == size - ti_mom_start(options));
	return TI_OKAY;
}
//
// msw
int ti_msw_start(double const * options) { return (int)options[0]; }

int ti_msw(int size, double const * const * inputs, double const * options, double * const * outputs) 
{
	const double * input = inputs[0];
	double * sine = outputs[0];
	double * lead = outputs[1];
	const int period = (int)options[0];
	if(period < 1) return TI_INVALID_OPTION;
	if(size <= ti_msw_start(options)) return TI_OKAY;
	const double pi = SMathConst::Pi; //3.1415926;
	const double tpi = 2 * pi;
	double weight = 0, phase;
	double rp, ip;
	for(int i = period; i < size; ++i) {
		rp = 0;
		ip = 0;
		for(int j = 0; j < period; ++j) {
			weight = input[i-j];
			rp = rp + cos(tpi * j / period) * weight;
			ip = ip + sin(tpi * j / period) * weight;
		}
		if(fabs(rp) > .001) {
			phase = atan(ip/rp);
		}
		else {
			phase = tpi / 2.0 * (ip < 0 ? -1.0 : 1.0);
		}
		if(rp < 0.0) phase += pi;
		phase += pi/2.0;
		if(phase < 0.0) phase += tpi;
		if(phase > tpi) phase -= tpi;
		//phase = 180 * phase / pi;
		*sine++ = sin(phase);
		*lead++ = sin(phase + pi/4.0);
	}
	assert(sine - outputs[0] == size - ti_msw_start(options));
	assert(lead - outputs[1] == size - ti_msw_start(options));
	return TI_OKAY;
}
//
// nvi
int ti_nvi_start(double const * options) { return 0; }

int ti_nvi(int size, double const * const * inputs, double const * options, double * const * outputs) 
{
	const double * close = inputs[0];
	const double * volume = inputs[1];
	double * output = outputs[0];
	if(size <= ti_nvi_start(options)) 
		return TI_OKAY;
	double nvi = 1000;
	*output++ = nvi;
	for(int i = 1; i < size; ++i) {
		if(volume[i] < volume[i-1]) {
			nvi += ((close[i] - close[i-1])/close[i-1]) * nvi;
		}
		*output++ = nvi;
	}
	assert(output - outputs[0] == size - ti_nvi_start(options));
	return TI_OKAY;
}
//
// obv
int ti_obv_start(double const * options) { return 0; }

int ti_obv(int size, double const * const * inputs, double const * options, double * const * outputs) 
{
	const double * close = inputs[0];
	const double * volume = inputs[1];
	double * output = outputs[0];
	double sum = 0;
	*output++ = sum;
	double prev = close[0];
	for(int i = 1; i < size; ++i) {
		if(close[i] > prev) {
			sum += volume[i];
		}
		else if(close[i] < prev) {
			sum -= volume[i];
		}
		else {
			/* No change. */
		}
		prev = close[i];
		*output++ = sum;
	}
	return TI_OKAY;
}
//
// ppo
int ti_ppo_start(double const * options) { return 1; }

int ti_ppo(int size, double const * const * inputs, double const * options, double * const * outputs) 
{
	const double * input = inputs[0];
	double * ppo = outputs[0];
	const int short_period = (int)options[0];
	const int long_period = (int)options[1];
	if(short_period < 1) return TI_INVALID_OPTION;
	if(long_period < 2) return TI_INVALID_OPTION;
	if(long_period < short_period) return TI_INVALID_OPTION;
	if(size <= ti_ppo_start(options)) return TI_OKAY;
	double short_per = 2 / ((double)short_period + 1);
	double long_per = 2 / ((double)long_period + 1);
	double short_ema = input[0];
	double long_ema = input[0];
	for(int i = 1; i < size; ++i) {
		short_ema = (input[i]-short_ema) * short_per + short_ema;
		long_ema = (input[i]-long_ema) * long_per + long_ema;
		const double out = 100.0 * (short_ema - long_ema) / long_ema;
		*ppo++ = out;
	}
	assert(ppo - outputs[0] == size - ti_ppo_start(options));
	return TI_OKAY;
}
//
// psar
int ti_psar_start(double const * options) { return 1; }

int ti_psar(int size, double const * const * inputs, double const * options, double * const * outputs) 
{
	const double * high = inputs[0];
	const double * low = inputs[1];
	const double accel_step = options[0];
	const double accel_max = options[1];
	double * output = outputs[0];
	if(accel_step <= 0) 
		return TI_INVALID_OPTION;
	if(accel_max <= accel_step) 
		return TI_INVALID_OPTION;
	if(size < 2) 
		return TI_OKAY;
	/* Try to choose if we start as short or long.
	 * There is really no right answer here. */
	int lng;
	if(high[0] + low[0] <= high[1] + low[1])
		lng = 1;
	else
		lng = 0;
	double sar, extreme;
	if(lng) {
		extreme = high[0];
		sar = low[0];
	}
	else {
		extreme = low[0];
		sar = high[0];
	}
	double accel = accel_step;
	int i;
	for(i = 1; i < size; ++i) {
		sar = (extreme - sar) * accel + sar;
		if(lng) {
			if(i >= 2 && (sar > low[i-2])) 
				sar = low[i-2];
			if((sar > low[i-1])) 
				sar = low[i-1];
			if(accel < accel_max && high[i] > extreme) {
				accel += accel_step;
				if(accel > accel_max) accel = accel_max;
			}
			if(high[i] > extreme) 
				extreme = high[i];
		}
		else {
			if(i >= 2 && (sar < high[i-2])) 
				sar = high[i-2];
			if((sar < high[i-1])) 
				sar = high[i-1];
			if(accel < accel_max && low[i] < extreme) {
				accel += accel_step;
				if(accel > accel_max) 
					accel = accel_max;
			}
			if(low[i] < extreme) 
				extreme = low[i];
		}
		if((lng && low[i] < sar) || (!lng && high[i] > sar)) {
			accel = accel_step;
			sar = extreme;
			lng = !lng;
			if(!lng) 
				extreme = low[i];
			else 
				extreme = high[i];
		}
		*output++ = sar;
		/*
		   printf("%s%2d %.4f %.4f %.4f %.4f %.4f %s\n", i == 1 ? "\n" : "", i, high[i], low[i], accel, extreme,
		      sar, reverse ? (!lng ? "short" : "long") : "");
		 */
	}
	assert(output - outputs[0] == size - ti_psar_start(options));
	return TI_OKAY;
}
//
// pvi
int ti_pvi_start(double const * options) { return 0; }

int ti_pvi(int size, double const * const * inputs, double const * options, double * const * outputs) 
{
	const double * close = inputs[0];
	const double * volume = inputs[1];
	double * output = outputs[0];
	if(size <= ti_pvi_start(options)) 
		return TI_OKAY;
	double pvi = 1000;
	*output++ = pvi;
	for(int i = 1; i < size; ++i) {
		if(volume[i] > volume[i-1]) {
			pvi += ((close[i] - close[i-1])/close[i-1]) * pvi;
		}
		*output++ = pvi;
	}
	assert(output - outputs[0] == size - ti_pvi_start(options));
	return TI_OKAY;
}
//
// qstick
int ti_qstick_start(double const * options) { return (int)options[0]-1; }

int ti_qstick(int size, double const * const * inputs, double const * options, double * const * outputs) 
{
	const double * open = inputs[0];
	const double * close = inputs[1];
	double * output = outputs[0];
	const int period = (int)options[0];
	const double div = 1.0 / period;
	if(period < 1) 
		return TI_INVALID_OPTION;
	if(size <= ti_qstick_start(options)) 
		return TI_OKAY;
	double sum = 0;
	int i;
	for(i = 0; i < period; ++i) {
		sum += close[i] - open[i];
	}
	*output++ = sum * div;
	for(i = period; i < size; ++i) {
		sum += close[i] - open[i];
		sum -= close[i-period] - open[i-period];
		*output++ = sum * div;
	}
	assert(output - outputs[0] == size - ti_qstick_start(options));
	return TI_OKAY;
}
//
// roc
int ti_roc_start(double const * options) { return (int)options[0]; }

int ti_roc(int size, double const * const * inputs, double const * options, double * const * outputs) 
{
	const double * input = inputs[0];
	const int period = (int)options[0];
	double * output = outputs[0];
	if(period < 1) 
		return TI_INVALID_OPTION;
	if(size <= ti_roc_start(options)) 
		return TI_OKAY;
	for(int i = period; i < size; ++i) {
		*output++ = (input[i] - input[i-period]) / input[i-period];
	}
	assert(output - outputs[0] == size - ti_roc_start(options));
	return TI_OKAY;
}
//
// rocr
int ti_rocr_start(double const * options) { return (int)options[0]; }

int ti_rocr(int size, double const * const * inputs, double const * options, double * const * outputs) 
{
	const double * input = inputs[0];
	const int period = (int)options[0];
	double * output = outputs[0];
	if(period < 1) 
		return TI_INVALID_OPTION;
	if(size <= ti_rocr_start(options)) 
		return TI_OKAY;
	for(int i = period; i < size; ++i) {
		*output++ = input[i] / input[i-period];
	}
	assert(output - outputs[0] == size - ti_rocr_start(options));
	return TI_OKAY;
}
//
// rsi
int ti_rsi_start(double const * options) { return (int)options[0]; }

int ti_rsi(int size, double const * const * inputs, double const * options, double * const * outputs) 
{
	const double * input = inputs[0];
	const int period = (int)options[0];
	double * output = outputs[0];
	const double per = 1.0 / ((double)period);
	if(period < 1) return TI_INVALID_OPTION;
	if(size <= ti_rsi_start(options)) return TI_OKAY;
	double smooth_up = 0, smooth_down = 0;
	int i;
	for(i = 1; i <= period; ++i) {
		const double upward = input[i] > input[i-1] ? input[i] - input[i-1] : 0;
		const double downward = input[i] < input[i-1] ? input[i-1] - input[i] : 0;
		smooth_up += upward;
		smooth_down += downward;
	}
	smooth_up /= period;
	smooth_down /= period;
	*output++ = 100.0 * (smooth_up / (smooth_up + smooth_down));
	for(i = period+1; i < size; ++i) {
		const double upward = input[i] > input[i-1] ? input[i] - input[i-1] : 0;
		const double downward = input[i] < input[i-1] ? input[i-1] - input[i] : 0;
		smooth_up = (upward-smooth_up) * per + smooth_up;
		smooth_down = (downward-smooth_down) * per + smooth_down;
		*output++ = 100.0 * (smooth_up / (smooth_up + smooth_down));
	}
	assert(output - outputs[0] == size - ti_rsi_start(options));
	return TI_OKAY;
}
//
// stddev
int ti_stddev_start(double const * options) { return (int)options[0]-1; }

int ti_stddev(int size, double const * const * inputs, double const * options, double * const * outputs) 
{
	const double * input = inputs[0];
	const int period = (int)options[0];
	double * output = outputs[0];
	const double div = 1.0 / period;
	if(period < 1) 
		return TI_INVALID_OPTION;
	if(size <= ti_stddev_start(options)) 
		return TI_OKAY;
	double sum = 0;
	double sum2 = 0;
	int i;
	for(i = 0; i < period; ++i) {
		sum += input[i];
		sum2 += input[i] * input[i];
	}
	{
		double s2s2 = (sum2 * div - (sum * div) * (sum * div));
		if(s2s2 > 0.0) 
			s2s2 = sqrt(s2s2);
		*output++ = s2s2;
	}
	for(i = period; i < size; ++i) {
		sum += input[i];
		sum2 += input[i] * input[i];
		sum -= input[i-period];
		sum2 -= input[i-period] * input[i-period];
		double s2s2 = (sum2 * div - (sum * div) * (sum * div));
		if(s2s2 > 0.0) s2s2 = sqrt(s2s2);
		*output++ = s2s2;
	}
	assert(output - outputs[0] == size - ti_stddev_start(options));
	return TI_OKAY;
}
//
// stderr
int ti_stderr_start(double const * options) { return (int)options[0]-1; }

int ti_stderr(int size, double const * const * inputs, double const * options, double * const * outputs) 
{
	const double * input = inputs[0];
	const int period = (int)options[0];
	double * output = outputs[0];
	const double div = 1.0 / period;
	if(period < 1) 
		return TI_INVALID_OPTION;
	if(size <= ti_stderr_start(options)) 
		return TI_OKAY;
	double sum = 0;
	double sum2 = 0;
	const double mul = 1.0 / sqrt((double)period);
	int i;
	for(i = 0; i < period; ++i) {
		sum += input[i];
		sum2 += input[i] * input[i];
	}
	{
		double s2s2 = (sum2 * div - (sum * div) * (sum * div));
		if(s2s2 > 0.0) s2s2 = sqrt(s2s2);
		*output++ = mul * s2s2;
	}
	for(i = period; i < size; ++i) {
		sum += input[i];
		sum2 += input[i] * input[i];
		sum -= input[i-period];
		sum2 -= input[i-period] * input[i-period];
		double s2s2 = (sum2 * div - (sum * div) * (sum * div));
		if(s2s2 > 0.0) s2s2 = sqrt(s2s2);
		*output++ = mul * s2s2;
	}
	assert(output - outputs[0] == size - ti_stderr_start(options));
	return TI_OKAY;
}
//
// stoch
int ti_stoch_start(double const * options) 
{
	const int kperiod = (int)options[0];
	const int kslow = (int)options[1];
	const int dperiod = (int)options[2];
	return kperiod + kslow + dperiod - 3;
}

int ti_stoch(int size, double const * const * inputs, double const * options, double * const * outputs) 
{
	const double * high = inputs[0];
	const double * low = inputs[1];
	const double * close = inputs[2];
	const int kperiod = (int)options[0];
	const int kslow = (int)options[1];
	const int dperiod = (int)options[2];
	const double kper = 1.0 / kslow;
	const double dper = 1.0 / dperiod;
	double * stoch = outputs[0];
	double * stoch_ma = outputs[1];
	if(kperiod < 1) 
		return TI_INVALID_OPTION;
	if(kslow < 1) 
		return TI_INVALID_OPTION;
	if(dperiod < 1) 
		return TI_INVALID_OPTION;
	if(size <= ti_stoch_start(options)) 
		return TI_OKAY;
	int trail = 0, maxi = -1, mini = -1;
	double max = high[0];
	double min = low[0];
	double bar;
	ti_buffer * k_sum = ti_buffer_new(kslow);
	ti_buffer * d_sum = ti_buffer_new(dperiod);
	int i, j;
	for(i = 0; i < size; ++i) {
		if(i >= kperiod) 
			++trail;
		/* Maintain highest. */
		bar = high[i];
		if(maxi < trail) {
			maxi = trail;
			max = high[maxi];
			j = trail;
			while(++j <= i) {
				bar = high[j];
				if(bar >= max) {
					max = bar;
					maxi = j;
				}
			}
		}
		else if(bar >= max) {
			maxi = i;
			max = bar;
		}
		/* Maintain lowest. */
		bar = low[i];
		if(mini < trail) {
			mini = trail;
			min = low[mini];
			j = trail;
			while(++j <= i) {
				bar = low[j];
				if(bar <= min) {
					min = bar;
					mini = j;
				}
			}
		}
		else if(bar <= min) {
			mini = i;
			min = bar;
		}
		/* Calculate it. */
		const double kdiff = (max - min);
		const double kfast = kdiff == 0.0 ? 0.0 : 100 * ((close[i] - min) / kdiff);
		ti_buffer_push(k_sum, kfast);
		if(i >= kperiod-1 + kslow-1) {
			const double k = k_sum->sum * kper;
			ti_buffer_push(d_sum, k);
			if(i >= kperiod-1 + kslow-1 + dperiod-1) {
				*stoch++ = k;
				*stoch_ma++ = d_sum->sum * dper;
			}
		}
	}
	ti_buffer_free(k_sum);
	ti_buffer_free(d_sum);
	assert(stoch - outputs[0] == size - ti_stoch_start(options));
	assert(stoch_ma - outputs[1] == size - ti_stoch_start(options));
	return TI_OKAY;
}
//
// sum
int ti_sum_start(double const * options) { return (int)options[0]-1; }

int ti_sum(int size, double const * const * inputs, double const * options, double * const * outputs) 
{
	const double * input = inputs[0];
	const int period = (int)options[0];
	double * output = outputs[0];
	if(period < 1) 
		return TI_INVALID_OPTION;
	if(size <= ti_sum_start(options)) 
		return TI_OKAY;
	double sum = 0;
	int i;
	for(i = 0; i < period; ++i) {
		sum += input[i];
	}
	*output++ = sum;
	for(i = period; i < size; ++i) {
		sum += input[i];
		sum -= input[i-period];
		*output++ = sum;
	}
	assert(output - outputs[0] == size - ti_sum_start(options));
	return TI_OKAY;
}
//
// trix
int ti_trix_start(double const * options) 
{
	const int period = (int)options[0];
	return ((period-1)*3)+1;
}

int ti_trix(int size, double const * const * inputs, double const * options, double * const * outputs) 
{
	const double * input = inputs[0];
	const int period = (int)options[0];
	double * output = outputs[0];
	if(period < 1) 
		return TI_INVALID_OPTION;
	if(size <= ti_trix_start(options)) 
		return TI_OKAY;
	const int start = (period*3)-2;
	assert(start == ti_trix_start(options));
	const double per = 2 / ((double)period + 1);
	double ema1 = input[0];
	double ema2 = 0, ema3 = 0;
	int i;
	for(i = 1; i < start; ++i) {
		ema1 = (input[i]-ema1) * per + ema1;
		if(i == period-1) {
			ema2 = ema1;
		}
		else if(i > period-1) {
			ema2 = (ema1-ema2) * per + ema2;
			if(i == period * 2 - 2) {
				ema3 = ema2;
			}
			else if(i > period * 2 - 2) {
				ema3 = (ema2-ema3) * per + ema3;
			}
		}
	}
	for(i = start; i < size; ++i) {
		ema1 = (input[i]-ema1) * per + ema1;
		ema2 = (ema1-ema2) * per + ema2;
		const double last = ema3;
		ema3 = (ema2-ema3) * per + ema3;
		*output++ = (ema3-last)/ema3 * 100.0;
	}
	assert(output - outputs[0] == size - ti_trix_start(options));
	return TI_OKAY;
}
//
// typprice
int ti_typprice_start(double const * options) { return 0; }

int ti_typprice(int size, double const * const * inputs, double const * options, double * const * outputs) 
{
	const double * high = inputs[0];
	const double * low = inputs[1];
	const double * close = inputs[2];
	double * output = outputs[0];
	for(int i = 0; i < size; ++i) {
		output[i] = (high[i] + low[i] + close[i]) * (1.0/3.0);
	}
	return TI_OKAY;
}
//
// ultosc
int ti_ultosc_start(double const * options) { return (int)options[2]; }

int ti_ultosc(int size, double const * const * inputs, double const * options, double * const * outputs) 
{
	const double * high = inputs[0];
	const double * low = inputs[1];
	const double * close = inputs[2];
	const int short_period = (int)options[0];
	const int medium_period = (int)options[1];
	const int long_period = (int)options[2];
	double * output = outputs[0];
	if(short_period < 1) 
		return TI_INVALID_OPTION;
	if(medium_period < short_period) 
		return TI_INVALID_OPTION;
	if(long_period < medium_period) 
		return TI_INVALID_OPTION;
	if(size <= ti_ultosc_start(options)) 
		return TI_OKAY;
	ti_buffer * bp_buf = ti_buffer_new(long_period);
	ti_buffer * r_buf = ti_buffer_new(long_period);
	double bp_short_sum = 0, bp_medium_sum = 0;
	double r_short_sum = 0, r_medium_sum = 0;
	int i;
	for(i = 1; i < size; ++i) {
		const double true_low = MIN(low[i], close[i-1]);
		const double true_high = MAX(high[i], close[i-1]);
		const double bp = close[i] - true_low;
		const double r = true_high - true_low;
		bp_short_sum += bp;
		bp_medium_sum += bp;
		r_short_sum += r;
		r_medium_sum += r;
		ti_buffer_push(bp_buf, bp);
		ti_buffer_push(r_buf, r);
		// The long sum takes care of itself, but we're piggy-backing
		// the medium and short sums off the same buffers. 
		if(i > short_period) {
			int short_index = bp_buf->index - short_period - 1;
			if(short_index < 0) 
				short_index += long_period;
			bp_short_sum -= bp_buf->P_Vals[short_index];
			r_short_sum -= r_buf->P_Vals[short_index];
			if(i > medium_period) {
				int medium_index = bp_buf->index - medium_period - 1;
				if(medium_index < 0) 
					medium_index += long_period;
				bp_medium_sum -= bp_buf->P_Vals[medium_index];
				r_medium_sum -= r_buf->P_Vals[medium_index];
			}
		}
		if(i >= long_period) {
			const double first = 4 * bp_short_sum / r_short_sum;
			const double second = 2 * bp_medium_sum / r_medium_sum;
			const double third = 1 * bp_buf->sum / r_buf->sum;
			const double ult = (first + second + third) * 100.0 / 7.0;
			*output++ = ult;
		}
	}
	ti_buffer_free(bp_buf);
	ti_buffer_free(r_buf);
	assert(output - outputs[0] == size - ti_ultosc_start(options));
	return TI_OKAY;
}
//
// var
int ti_var_start(double const * options) { return (int)options[0]-1; }

int ti_var(int size, double const * const * inputs, double const * options, double * const * outputs) 
{
	const double * input = inputs[0];
	const int period = (int)options[0];
	double * output = outputs[0];
	const double div = 1.0 / period;
	if(period < 1) 
		return TI_INVALID_OPTION;
	if(size <= ti_var_start(options)) 
		return TI_OKAY;
	double sum = 0;
	double sum2 = 0;
	int i;
	for(i = 0; i < period; ++i) {
		sum += input[i];
		sum2 += input[i] * input[i];
	}
	*output++ = sum2 * div - (sum * div) * (sum * div);
	for(i = period; i < size; ++i) {
		sum += input[i];
		sum2 += input[i] * input[i];
		sum -= input[i-period];
		sum2 -= input[i-period] * input[i-period];
		*output++ = sum2 * div - (sum * div) * (sum * div);
	}
	assert(output - outputs[0] == size - ti_var_start(options));
	return TI_OKAY;
}
//
// vhf
int ti_vhf_start(double const * options) { return (int)options[0]; }

int ti_vhf(int size, double const * const * inputs, double const * options, double * const * outputs) 
{
	const double * in = inputs[0];
	const int period = (int)options[0];
	double * output = outputs[0];
	if(period < 1) 
		return TI_INVALID_OPTION;
	if(size <= ti_vhf_start(options)) 
		return TI_OKAY;
	int trail = 1, maxi = -1, mini = -1;
	double max = in[0], min = in[0];
	double bar;
	double sum = 0;
	int i, j;
	double yc = in[0];
	double c;
	for(i = 1; i < period; ++i) {
		c = in[i];
		sum += fabs(c - yc);
		yc = c;
	}
	for(i = period; i < size; ++i, ++trail) {
		c = in[i];
		sum += fabs(c - yc);
		yc = c;
		if(i > period) {
			sum -= fabs(in[i-period] - in[i-period-1]);
		}
		/* Maintain highest. */
		bar = c;
		if(maxi < trail) {
			maxi = trail;
			max = in[maxi];
			j = trail;
			while(++j <= i) {
				bar = in[j];
				if(bar >= max) {
					max = bar;
					maxi = j;
				}
			}
		}
		else if(bar >= max) {
			maxi = i;
			max = bar;
		}
		/* Maintain lowest. */
		bar = c;
		if(mini < trail) {
			mini = trail;
			min = in[mini];
			j = trail;
			while(++j <= i) {
				bar = in[j];
				if(bar <= min) {
					min = bar;
					mini = j;
				}
			}
		}
		else if(bar <= min) {
			mini = i;
			min = bar;
		}
		/* Calculate it. */
		*output++ = fabs(max - min) / sum;
	}
	assert(output - outputs[0] == size - ti_vhf_start(options));
	return TI_OKAY;
}
//
// vidya
int ti_vidya_start(double const * options) { return ((int)(options[1])) - 2; }

int ti_vidya(int size, double const * const * inputs, double const * options, double * const * outputs) 
{
	const double * input = inputs[0];
	const int short_period = (int)options[0];
	const int long_period = (int)options[1];
	const double alpha = options[2];
	double * output = outputs[0];
	const double short_div = 1.0 / short_period;
	const double long_div = 1.0 / long_period;
	if(short_period < 1) return TI_INVALID_OPTION;
	if(long_period < short_period) return TI_INVALID_OPTION;
	if(long_period < 2) return TI_INVALID_OPTION;
	if(alpha < 0.0 || alpha > 1.0) return TI_INVALID_OPTION;
	if(size <= ti_vidya_start(options)) return TI_OKAY;
	double short_sum = 0;
	double short_sum2 = 0;
	double long_sum = 0;
	double long_sum2 = 0;
	int i;
	for(i = 0; i < long_period; ++i) {
		long_sum += input[i];
		long_sum2 += input[i] * input[i];
		if(i >= long_period - short_period) {
			short_sum += input[i];
			short_sum2 += input[i] * input[i];
		}
	}
	double val = input[long_period-2];
	*output++ = val;
	if(long_period - 1 < size) {
		double short_stddev = sqrt(short_sum2 * short_div - (short_sum * short_div) * (short_sum * short_div));
		double long_stddev = sqrt(long_sum2 * long_div - (long_sum * long_div) * (long_sum * long_div));
		double k = short_stddev / long_stddev;
		if(k != k) k = 0; /* In some conditions it works out that we take the sqrt(-0.0), which gives NaN.
		                      That implies that k should be zero. */
		k *= alpha;
		val = (input[long_period-1]-val) * k + val;
		*output++ = val;
	}
	for(i = long_period; i < size; ++i) {
		long_sum += input[i];
		long_sum2 += input[i] * input[i];
		short_sum += input[i];
		short_sum2 += input[i] * input[i];
		long_sum -= input[i-long_period];
		long_sum2 -= input[i-long_period] * input[i-long_period];
		short_sum -= input[i-short_period];
		short_sum2 -= input[i-short_period] * input[i-short_period];
		{
			double short_stddev = sqrt(short_sum2 * short_div - (short_sum * short_div) * (short_sum * short_div));
			double long_stddev = sqrt(long_sum2 * long_div - (long_sum * long_div) * (long_sum * long_div));
			double k = short_stddev / long_stddev;
			if(k != k) 
				k = 0;
			k *= alpha;
			val = (input[i]-val) * k + val;
			*output++ = val;
		}
	}
	assert(output - outputs[0] == size - ti_vidya_start(options));
	return TI_OKAY;
}
//
// volatility
#define CHANGE(i) (input[i]/input[i-1]-1.0)

int ti_volatility_start(double const * options) { return (int)options[0]; }

int ti_volatility(int size, double const * const * inputs, double const * options, double * const * outputs) 
{
	const double * input = inputs[0];
	double * output = outputs[0];
	const int period = (int)options[0];
	const double div = 1.0 / period;
	const double annual = sqrt(252.0); /* Multiplier, number of trading days in year. */
	if(period < 1) 
		return TI_INVALID_OPTION;
	if(size <= ti_volatility_start(options)) 
		return TI_OKAY;
	double sum = 0;
	double sum2 = 0;
	int i;
	for(i = 1; i <= period; ++i) {
		const double c = CHANGE(i);
		sum += c;
		sum2 += c * c;
	}
	*output++ = sqrt(sum2 * div - (sum * div) * (sum * div)) * annual;
	for(i = period+1; i < size; ++i) {
		const double c = CHANGE(i);
		sum += c;
		sum2 += c * c;
		const double cp = CHANGE(i-period);
		sum -= cp;
		sum2 -= cp * cp;
		*output++ = sqrt(sum2 * div - (sum * div) * (sum * div)) * annual;
	}
	assert(output - outputs[0] == size - ti_volatility_start(options));
	return TI_OKAY;
}
//
// vosc
int ti_vosc_start(double const * options) { return (int)options[1]-1; }

int ti_vosc(int size, double const * const * inputs, double const * options, double * const * outputs) 
{
	const double * input = inputs[0];
	double * output = outputs[0];
	const int short_period = (int)options[0];
	const int long_period = (int)options[1];
	const double short_div = 1.0 / short_period;
	const double long_div = 1.0 / long_period;
	if(short_period < 1) 
		return TI_INVALID_OPTION;
	if(long_period < short_period) 
		return TI_INVALID_OPTION;
	if(size <= ti_vosc_start(options)) 
		return TI_OKAY;
	double short_sum = 0.0;
	double long_sum = 0.0;
	int i;
	for(i = 0; i < long_period; ++i) {
		if(i >= (long_period - short_period)) {
			short_sum += input[i];
		}
		long_sum += input[i];
	}
	{
		const double savg = short_sum * short_div;
		const double lavg = long_sum * long_div;
		*output++ = 100.0 * (savg - lavg) / lavg;
	}
	for(i = long_period; i < size; ++i) {
		short_sum += input[i];
		short_sum -= input[i-short_period];
		long_sum += input[i];
		long_sum -= input[i-long_period];
		const double savg = short_sum * short_div;
		const double lavg = long_sum * long_div;
		*output++ = 100.0 * (savg - lavg) / lavg;
	}
	assert(output - outputs[0] == size - ti_vosc_start(options));
	return TI_OKAY;
}
//
// vwma
int ti_vwma_start(double const * options) { return (int)options[0]-1; }

int ti_vwma(int size, double const * const * inputs, double const * options, double * const * outputs) 
{
	const double * input = inputs[0];
	const double * volume = inputs[1];
	const int period = (int)options[0];
	double * output = outputs[0];
	if(period < 1) 
		return TI_INVALID_OPTION;
	if(size <= ti_vwma_start(options)) 
		return TI_OKAY;
	double sum = 0;
	double vsum = 0;
	int i;
	for(i = 0; i < period; ++i) {
		sum += input[i] * volume[i];
		vsum += volume[i];
	}
	*output++ = sum / vsum;
	for(i = period; i < size; ++i) {
		sum += input[i] * volume[i];
		sum -= input[i-period] * volume[i-period];
		vsum += volume[i];
		vsum -= volume[i-period];
		*output++ = sum / vsum;
	}
	assert(output - outputs[0] == size - ti_vwma_start(options));
	return TI_OKAY;
}
//
// wad
int ti_wad_start(double const * options) { return 1; }

int ti_wad(int size, double const * const * inputs, double const * options, double * const * outputs) 
{
	const double * high = inputs[0];
	const double * low = inputs[1];
	const double * close = inputs[2];
	if(size <= ti_wad_start(options)) 
		return TI_OKAY;
	double * output = outputs[0];
	double sum = 0;
	double yc = close[0];
	for(int i = 1; i < size; ++i) {
		const double c = close[i];
		if(c > yc) {
			sum += c - MIN(yc, low[i]);
		}
		else if(c < yc) {
			sum += c - MAX(yc, high[i]);
		}
		else {
			//No change
		}
		*output++ = sum;
		yc = close[i];
	}
	assert(output - outputs[0] == size - ti_wad_start(options));
	return TI_OKAY;
}
//
// wcprice
int ti_wcprice_start(double const * options) { return 0; }

int ti_wcprice(int size, double const * const * inputs, double const * options, double * const * outputs) 
{
	const double * high = inputs[0];
	const double * low = inputs[1];
	const double * close = inputs[2];
	double * output = outputs[0];
	for(int i = 0; i < size; ++i) {
		output[i] = (high[i] + low[i] + close[i] + close[i]) * 0.25;
	}
	return TI_OKAY;
}
//
// wilders
int ti_wilders_start(double const * options) { return (int)options[0]-1; }

int ti_wilders(int size, double const * const * inputs, double const * options, double * const * outputs) 
{
	const double * input = inputs[0];
	const int period = (int)options[0];
	double * output = outputs[0];
	if(period < 1) 
		return TI_INVALID_OPTION;
	if(size <= ti_wilders_start(options)) 
		return TI_OKAY;
	const double per = 1.0 / ((double)period);
	double sum = 0;
	int i;
	for(i = 0; i < period; ++i) {
		sum += input[i];
	}
	double val = sum / period;
	*output++ = val;
	for(i = period; i < size; ++i) {
		val = (input[i]-val) * per + val;
		*output++ = val;
	}
	assert(output - outputs[0] == size - ti_wilders_start(options));
	return TI_OKAY;
}
//
// willr
int ti_willr_start(double const * options) { return (int)options[0]-1; }

int ti_willr(int size, double const * const * inputs, double const * options, double * const * outputs) 
{
	const double * high = inputs[0];
	const double * low = inputs[1];
	const double * close = inputs[2];
	const int period = (int)options[0];
	double * output = outputs[0];
	if(period < 1) 
		return TI_INVALID_OPTION;
	if(size <= ti_willr_start(options)) 
		return TI_OKAY;
	int trail = 0, maxi = -1, mini = -1;
	double max = high[0];
	double min = low[0];
	double bar;
	int i, j;
	for(i = period-1; i < size; ++i, ++trail) {
		/* Maintain highest. */
		bar = high[i];
		if(maxi < trail) {
			maxi = trail;
			max = high[maxi];
			j = trail;
			while(++j <= i) {
				bar = high[j];
				if(bar >= max) {
					max = bar;
					maxi = j;
				}
			}
		}
		else if(bar >= max) {
			maxi = i;
			max = bar;
		}
		/* Maintain lowest. */
		bar = low[i];
		if(mini < trail) {
			mini = trail;
			min = low[mini];
			j = trail;
			while(++j <= i) {
				bar = low[j];
				if(bar <= min) {
					min = bar;
					mini = j;
				}
			}
		}
		else if(bar <= min) {
			mini = i;
			min = bar;
		}
		/* Calculate it. */
		const double highlow = (max - min);
		const double r = highlow == 0.0 ? 0.0 : -100 * ((max - close[i]) / highlow);
		*output++ = r;
	}
	assert(output - outputs[0] == size - ti_willr_start(options));
	return TI_OKAY;
}
//
// wma
int ti_wma_start(double const * options) { return (int)options[0]-1; }

int ti_wma(int size, double const * const * inputs, double const * options, double * const * outputs) 
{
	const double * input = inputs[0];
	const int period = (int)options[0];
	double * output = outputs[0];
	if(period < 1) 
		return TI_INVALID_OPTION;
	if(size <= ti_wma_start(options)) 
		return TI_OKAY;
	/* Weights for 6 period WMA:
	 * 1 2 3 4 5 6
	 */
	const double weights = period * (period+1) / 2;
	double sum = 0; /* Flat sum of previous numbers. */
	double weight_sum = 0; /* Weighted sum of previous numbers. */
	int i;
	for(i = 0; i < period-1; ++i) {
		weight_sum += input[i] * (i+1);
		sum += input[i];
	}
	for(i = period-1; i < size; ++i) {
		weight_sum += input[i] * period;
		sum += input[i];
		*output++ = weight_sum / weights;
		weight_sum -= sum;
		sum -= input[i-period+1];
	}
	assert(output - outputs[0] == size - ti_wma_start(options));
	return TI_OKAY;
}
//
// zlema
int ti_zlema_start(double const * options) { return ((int)options[0] - 1) / 2 - 1; }

int ti_zlema(int size, double const * const * inputs, double const * options, double * const * outputs) 
{
	const double * input = inputs[0];
	const int period = (int)options[0];
	const int lag = (period - 1) / 2;
	double * output = outputs[0];
	if(period < 1) return TI_INVALID_OPTION;
	if(size <= ti_zlema_start(options)) return TI_OKAY;
	const double per = 2 / ((double)period + 1);
	double val = input[lag-1];
	*output++ = val;
	for(int i = lag; i < size; ++i) {
		double c = input[i];
		double l = input[i-lag];
		val = ((c + (c-l))-val) * per + val;
		*output++ = val;
	}
	assert(output - outputs[0] == size - ti_zlema_start(options));
	return TI_OKAY;
}
//
//
//
#include "tulipindicators-trend.h"
// linreg
int ti_linreg_start(double const * options) { return (int)options[0]-1; }

int ti_linreg(int size, double const * const * inputs, double const * options, double * const * outputs) 
{
	const double * input = inputs[0];
	const int period = (int)options[0];
	double * output = outputs[0];
	if(period < 1) 
		return TI_INVALID_OPTION;
	if(size <= ti_linreg_start(options)) 
		return TI_OKAY;
	LINEAR_REGRESSION(size, input, period, output, period);
	assert(output - outputs[0] == size - ti_linreg_start(options));
	return TI_OKAY;
}
//
// linregintercept
int ti_linregintercept_start(double const * options) { return (int)options[0]-1; }

int ti_linregintercept(int size, double const * const * inputs, double const * options, double * const * outputs) 
{
	const double * input = inputs[0];
	const int period = (int)options[0];
	double * output = outputs[0];
	if(period < 1) 
		return TI_INVALID_OPTION;
	if(size <= ti_linregintercept_start(options)) 
		return TI_OKAY;
	LINEAR_REGRESSION(size, input, period, output, 1);
	assert(output - outputs[0] == size - ti_linregintercept_start(options));
	return TI_OKAY;
}
//
// tsf
int ti_tsf_start(double const * options) { return (int)options[0]-1; }

int ti_tsf(int size, double const * const * inputs, double const * options, double * const * outputs) 
{
	const double * input = inputs[0];
	const int period = (int)options[0];
	double * output = outputs[0];
	if(period < 1) 
		return TI_INVALID_OPTION;
	if(size <= ti_tsf_start(options)) 
		return TI_OKAY;
	LINEAR_REGRESSION(size, input, period, output, period+1);
	assert(output - outputs[0] == size - ti_tsf_start(options));
	return TI_OKAY;
}
//
// fosc
#undef LINEAR_REGRESSION_POSTPROC
#undef LINEAR_REGRESSION_INIT
#undef LINEAR_REGRESSION_FINAL
#undef LINEAR_REGRESSION

#define LINEAR_REGRESSION_INIT() const double p = (1.0 / (period)); double tsf = 0;

#define LINEAR_REGRESSION_FINAL(forecast) do { \
		const double a = (y - b * x) * p; \
		if(i >= (period)) {*(output)++ = 100 * (input[i] - tsf) / input[i]; } \
		tsf = (a + b * (forecast)); \
} while(0)

#include "tulipindicators-trend.h"

int ti_fosc_start(double const * options) { return (int)options[0]; }

int ti_fosc(int size, double const * const * inputs, double const * options, double * const * outputs) 
{
	const double * input = inputs[0];
	const int period = (int)options[0];
	double * output = outputs[0];
	if(period < 1) 
		return TI_INVALID_OPTION;
	if(size <= ti_fosc_start(options)) 
		return TI_OKAY;
	LINEAR_REGRESSION(size, input, period, output, period+1);
	assert(output - outputs[0] == size - ti_fosc_start(options));
	return TI_OKAY;
}
//
// linregslope
#undef LINEAR_REGRESSION_POSTPROC
#undef LINEAR_REGRESSION_INIT
#undef LINEAR_REGRESSION_FINAL
#undef LINEAR_REGRESSION

#define LINEAR_REGRESSION_INIT() do {} while(0)
#define LINEAR_REGRESSION_FINAL(forecast) do { *(output)++ = b; } while(0)

#include "tulipindicators-trend.h"

int ti_linregslope_start(double const * options) { return (int)options[0]-1; }

int ti_linregslope(int size, double const * const * inputs, double const * options, double * const * outputs) 
{
	const double * input = inputs[0];
	const int period = (int)options[0];
	double * output = outputs[0];
	if(period < 1) 
		return TI_INVALID_OPTION;
	if(size <= ti_linregslope_start(options)) 
		return TI_OKAY;
	LINEAR_REGRESSION(size, input, period, output, period);
	assert(output - outputs[0] == size - ti_linregslope_start(options));
	return TI_OKAY;
}
#undef LINEAR_REGRESSION_POSTPROC
#undef LINEAR_REGRESSION_INIT
#undef LINEAR_REGRESSION_FINAL
#undef LINEAR_REGRESSION
//
//
//
struct ti_indicator_info ti_indicators[] = {
	{"abs", "Vector Absolute Value", ti_abs_start, ti_abs, TI_TYPE_SIMPLE, 1, 0, 1, {"real", 0}, {"", 0}, {"abs", 0}},
	{"acos", "Vector Arccosine", ti_acos_start, ti_acos, TI_TYPE_SIMPLE, 1, 0, 1, {"real", 0}, {"", 0}, {"acos", 0}},
	{"ad", "Accumulation/Distribution Line", ti_ad_start, ti_ad, TI_TYPE_INDICATOR, 4, 0, 1, {"high", "low", "close", "volume", 0}, {"", 0}, {"ad", 0}},
	{"add", "Vector Addition", ti_add_start, ti_add, TI_TYPE_SIMPLE, 2, 0, 1, {"real", "real", 0}, {"", 0}, {"add", 0}},
	{"adosc", "Accumulation/Distribution Oscillator", ti_adosc_start, ti_adosc, TI_TYPE_INDICATOR, 4, 2, 1, {"high", "low", "close", "volume", 0}, {"short period", "long period", 0}, {"adosc", 0}},
	{"adx", "Average Directional Movement Index", ti_adx_start, ti_adx, TI_TYPE_INDICATOR, 3, 1, 1, {"high", "low", "close", 0}, {"period", 0}, {"dx", 0}},
	{"adxr", "Average Directional Movement Rating", ti_adxr_start, ti_adxr, TI_TYPE_INDICATOR, 3, 1, 1, {"high", "low", "close", 0}, {"period", 0}, {"dx", 0}},
	{"ao", "Awesome Oscillator", ti_ao_start, ti_ao, TI_TYPE_INDICATOR, 2, 0, 1, {"high", "low", 0}, {"", 0}, {"ao", 0}},
	{"apo", "Absolute Price Oscillator", ti_apo_start, ti_apo, TI_TYPE_INDICATOR, 1, 2, 1, {"real", 0}, {"short period", "long period", 0}, {"apo", 0}},
	{"aroon", "Aroon", ti_aroon_start, ti_aroon, TI_TYPE_INDICATOR, 2, 1, 2, {"high", "low", 0}, {"period", 0}, {"aroon_down", "aroon_up", 0}},
	{"aroonosc", "Aroon Oscillator", ti_aroonosc_start, ti_aroonosc, TI_TYPE_INDICATOR, 2, 1, 1, {"high", "low", 0}, {"period", 0}, {"aroonosc", 0}},
	{"asin", "Vector Arcsine", ti_asin_start, ti_asin, TI_TYPE_SIMPLE, 1, 0, 1, {"real", 0}, {"", 0}, {"asin", 0}},
	{"atan", "Vector Arctangent", ti_atan_start, ti_atan, TI_TYPE_SIMPLE, 1, 0, 1, {"real", 0}, {"", 0}, {"atan", 0}},
	{"atr", "Average True Range", ti_atr_start, ti_atr, TI_TYPE_INDICATOR, 3, 1, 1, {"high", "low", "close", 0}, {"period", 0}, {"atr", 0}},
	{"avgprice", "Average Price", ti_avgprice_start, ti_avgprice, TI_TYPE_OVERLAY, 4, 0, 1, {"open", "high", "low", "close", 0}, {"", 0}, {"avgprice", 0}},
	{"bbands", "Bollinger Bands", ti_bbands_start, ti_bbands, TI_TYPE_OVERLAY, 1, 2, 3, {"real", 0}, {"period", "stddev", 0}, {"bbands_lower", "bbands_middle", "bbands_upper", 0}},
	{"bop", "Balance of Power", ti_bop_start, ti_bop, TI_TYPE_INDICATOR, 4, 0, 1, {"open", "high", "low", "close", 0}, {"", 0}, {"bop", 0}},
	{"cci", "Commodity Channel Index", ti_cci_start, ti_cci, TI_TYPE_INDICATOR, 3, 1, 1, {"high", "low", "close", 0}, {"period", 0}, {"cci", 0}},
	{"ceil", "Vector Ceiling", ti_ceil_start, ti_ceil, TI_TYPE_SIMPLE, 1, 0, 1, {"real", 0}, {"", 0}, {"ceil", 0}},
	{"cmo", "Chande Momentum Oscillator", ti_cmo_start, ti_cmo, TI_TYPE_INDICATOR, 1, 1, 1, {"real", 0}, {"period", 0}, {"cmo", 0}},
	{"cos", "Vector Cosine", ti_cos_start, ti_cos, TI_TYPE_SIMPLE, 1, 0, 1, {"real", 0}, {"", 0}, {"cos", 0}},
	{"cosh", "Vector Hyperbolic Cosine", ti_cosh_start, ti_cosh, TI_TYPE_SIMPLE, 1, 0, 1, {"real", 0}, {"", 0}, {"cosh", 0}},
	{"crossany", "Crossany", ti_crossany_start, ti_crossany, TI_TYPE_MATH, 2, 0, 1, {"real", "real", 0}, {"", 0}, {"crossany", 0}},
	{"crossover", "Crossover", ti_crossover_start, ti_crossover, TI_TYPE_MATH, 2, 0, 1, {"real", "real", 0}, {"", 0}, {"crossover", 0}},
	{"cvi", "Chaikins Volatility", ti_cvi_start, ti_cvi, TI_TYPE_INDICATOR, 2, 1, 1, {"high", "low", 0}, {"period", 0}, {"cvi", 0}},
	{"decay", "Linear Decay", ti_decay_start, ti_decay, TI_TYPE_MATH, 1, 1, 1, {"real", 0}, {"period", 0}, {"decay", 0}},
	{"dema", "Double Exponential Moving Average", ti_dema_start, ti_dema, TI_TYPE_OVERLAY, 1, 1, 1, {"real", 0}, {"period", 0}, {"dema", 0}},
	{"di", "Directional Indicator", ti_di_start, ti_di, TI_TYPE_INDICATOR, 3, 1, 2, {"high", "low", "close", 0}, {"period", 0}, {"plus_di", "minus_di", 0}},
	{"div", "Vector Division", ti_div_start, ti_div, TI_TYPE_SIMPLE, 2, 0, 1, {"real", "real", 0}, {"", 0}, {"div", 0}},
	{"dm", "Directional Movement", ti_dm_start, ti_dm, TI_TYPE_INDICATOR, 2, 1, 2, {"high", "low", 0}, {"period", 0}, {"plus_dm", "minus_dm", 0}},
	{"dpo", "Detrended Price Oscillator", ti_dpo_start, ti_dpo, TI_TYPE_INDICATOR, 1, 1, 1, {"real", 0}, {"period", 0}, {"dpo", 0}},
	{"dx", "Directional Movement Index", ti_dx_start, ti_dx, TI_TYPE_INDICATOR, 3, 1, 1, {"high", "low", "close", 0}, {"period", 0}, {"dx", 0}},
	{"edecay", "Exponential Decay", ti_edecay_start, ti_edecay, TI_TYPE_MATH, 1, 1, 1, {"real", 0}, {"period", 0}, {"edecay", 0}},
	{"ema", "Exponential Moving Average", ti_ema_start, ti_ema, TI_TYPE_OVERLAY, 1, 1, 1, {"real", 0}, {"period", 0}, {"ema", 0}},
	{"emv", "Ease of Movement", ti_emv_start, ti_emv, TI_TYPE_INDICATOR, 3, 0, 1, {"high", "low", "volume", 0}, {"", 0}, {"emv", 0}},
	{"exp", "Vector Exponential", ti_exp_start, ti_exp, TI_TYPE_SIMPLE, 1, 0, 1, {"real", 0}, {"", 0}, {"exp", 0}},
	{"fisher", "Fisher Transform", ti_fisher_start, ti_fisher, TI_TYPE_INDICATOR, 2, 1, 2, {"high", "low", 0}, {"period", 0}, {"fisher", "fisher_signal", 0}},
	{"floor", "Vector Floor", ti_floor_start, ti_floor, TI_TYPE_SIMPLE, 1, 0, 1, {"real", 0}, {"", 0}, {"floor", 0}},
	{"fosc", "Forecast Oscillator", ti_fosc_start, ti_fosc, TI_TYPE_INDICATOR, 1, 1, 1, {"real", 0}, {"period", 0}, {"fosc", 0}},
	{"hma", "Hull Moving Average", ti_hma_start, ti_hma, TI_TYPE_OVERLAY, 1, 1, 1, {"real", 0}, {"period", 0}, {"hma", 0}},
	{"kama", "Kaufman Adaptive Moving Average", ti_kama_start, ti_kama, TI_TYPE_OVERLAY, 1, 1, 1, {"real", 0}, {"period", 0}, {"kama", 0}},
	{"kvo", "Klinger Volume Oscillator", ti_kvo_start, ti_kvo, TI_TYPE_INDICATOR, 4, 2, 1, {"high", "low", "close", "volume", 0}, {"short period", "long period", 0}, {"kvo", 0}},
	{"lag", "Lag", ti_lag_start, ti_lag, TI_TYPE_MATH, 1, 1, 1, {"real", 0}, {"period", 0}, {"lag", 0}},
	{"linreg", "Linear Regression", ti_linreg_start, ti_linreg, TI_TYPE_OVERLAY, 1, 1, 1, {"real", 0}, {"period", 0}, {"linreg", 0}},
	{"linregintercept", "Linear Regression Intercept", ti_linregintercept_start, ti_linregintercept, TI_TYPE_INDICATOR, 1, 1, 1, {"real", 0}, {"period", 0}, {"linregintercept", 0}},
	{"linregslope", "Linear Regression Slope", ti_linregslope_start, ti_linregslope, TI_TYPE_INDICATOR, 1, 1, 1, {"real", 0}, {"period", 0}, {"linregslope", 0}},
	{"ln", "Vector Natural Log", ti_ln_start, ti_ln, TI_TYPE_SIMPLE, 1, 0, 1, {"real", 0}, {"", 0}, {"ln", 0}},
	{"log10", "Vector Base-10 Log", ti_log10_start, ti_log10, TI_TYPE_SIMPLE, 1, 0, 1, {"real", 0}, {"", 0}, {"log10", 0}},
	{"macd", "Moving Average Convergence/Divergence", ti_macd_start, ti_macd, TI_TYPE_INDICATOR, 1, 3, 3, {"real", 0}, {"short period", "long period", "signal period", 0}, {"macd", "macd_signal", "macd_histogram", 0}},
	{"marketfi", "Market Facilitation Index", ti_marketfi_start, ti_marketfi, TI_TYPE_INDICATOR, 3, 0, 1, {"high", "low", "volume", 0}, {"", 0}, {"marketfi", 0}},
	{"mass", "Mass Index", ti_mass_start, ti_mass, TI_TYPE_INDICATOR, 2, 1, 1, {"high", "low", 0}, {"period", 0}, {"mass", 0}},
	{"max", "Maximum In Period", ti_max_start, ti_max, TI_TYPE_MATH, 1, 1, 1, {"real", 0}, {"period", 0}, {"max", 0}},
	{"md", "Mean Deviation Over Period", ti_md_start, ti_md, TI_TYPE_MATH, 1, 1, 1, {"real", 0}, {"period", 0}, {"md", 0}},
	{"medprice", "Median Price", ti_medprice_start, ti_medprice, TI_TYPE_OVERLAY, 2, 0, 1, {"high", "low", 0}, {"", 0}, {"medprice", 0}},
	{"mfi", "Money Flow Index", ti_mfi_start, ti_mfi, TI_TYPE_INDICATOR, 4, 1, 1, {"high", "low", "close", "volume", 0}, {"period", 0}, {"mfi", 0}},
	{"min", "Minimum In Period", ti_min_start, ti_min, TI_TYPE_MATH, 1, 1, 1, {"real", 0}, {"period", 0}, {"min", 0}},
	{"mom", "Momentum", ti_mom_start, ti_mom, TI_TYPE_INDICATOR, 1, 1, 1, {"real", 0}, {"period", 0}, {"mom", 0}},
	{"msw", "Mesa Sine Wave", ti_msw_start, ti_msw, TI_TYPE_INDICATOR, 1, 1, 2, {"real", 0}, {"period", 0}, {"msw_sine", "msw_lead", 0}},
	{"mul", "Vector Multiplication", ti_mul_start, ti_mul, TI_TYPE_SIMPLE, 2, 0, 1, {"real", "real", 0}, {"", 0}, {"mul", 0}},
	{"natr", "Normalized Average True Range", ti_natr_start, ti_natr, TI_TYPE_INDICATOR, 3, 1, 1, {"high", "low", "close", 0}, {"period", 0}, {"natr", 0}},
	{"nvi", "Negative Volume Index", ti_nvi_start, ti_nvi, TI_TYPE_INDICATOR, 2, 0, 1, {"close", "volume", 0}, {"", 0}, {"nvi", 0}},
	{"obv", "On Balance Volume", ti_obv_start, ti_obv, TI_TYPE_INDICATOR, 2, 0, 1, {"close", "volume", 0}, {"", 0}, {"obv", 0}},
	{"ppo", "Percentage Price Oscillator", ti_ppo_start, ti_ppo, TI_TYPE_INDICATOR, 1, 2, 1, {"real", 0}, {"short period", "long period", 0}, {"ppo", 0}},
	{"psar", "Parabolic SAR", ti_psar_start, ti_psar, TI_TYPE_OVERLAY, 2, 2, 1, {"high", "low", 0}, {"acceleration factor step", "acceleration factor maximum", 0}, {"psar", 0}},
	{"pvi", "Positive Volume Index", ti_pvi_start, ti_pvi, TI_TYPE_INDICATOR, 2, 0, 1, {"close", "volume", 0}, {"", 0}, {"pvi", 0}},
	{"qstick", "Qstick", ti_qstick_start, ti_qstick, TI_TYPE_INDICATOR, 2, 1, 1, {"open", "close", 0}, {"period", 0}, {"qstick", 0}},
	{"roc", "Rate of Change", ti_roc_start, ti_roc, TI_TYPE_INDICATOR, 1, 1, 1, {"real", 0}, {"period", 0}, {"roc", 0}},
	{"rocr", "Rate of Change Ratio", ti_rocr_start, ti_rocr, TI_TYPE_INDICATOR, 1, 1, 1, {"real", 0}, {"period", 0}, {"rocr", 0}},
	{"round", "Vector Round", ti_round_start, ti_round, TI_TYPE_SIMPLE, 1, 0, 1, {"real", 0}, {"", 0}, {"round", 0}},
	{"rsi", "Relative Strength Index", ti_rsi_start, ti_rsi, TI_TYPE_INDICATOR, 1, 1, 1, {"real", 0}, {"period", 0}, {"rsi", 0}},
	{"sin", "Vector Sine", ti_sin_start, ti_sin, TI_TYPE_SIMPLE, 1, 0, 1, {"real", 0}, {"", 0}, {"sin", 0}},
	{"sinh", "Vector Hyperbolic Sine", ti_sinh_start, ti_sinh, TI_TYPE_SIMPLE, 1, 0, 1, {"real", 0}, {"", 0}, {"sinh", 0}},
	{"sma", "Simple Moving Average", ti_sma_start, ti_sma, TI_TYPE_OVERLAY, 1, 1, 1, {"real", 0}, {"period", 0}, {"sma", 0}},
	{"sqrt", "Vector Square Root", ti_sqrt_start, ti_sqrt, TI_TYPE_SIMPLE, 1, 0, 1, {"real", 0}, {"", 0}, {"sqrt", 0}},
	{"stddev", "Standard Deviation Over Period", ti_stddev_start, ti_stddev, TI_TYPE_MATH, 1, 1, 1, {"real", 0}, {"period", 0}, {"stddev", 0}},
	{"stderr", "Standard Error Over Period", ti_stderr_start, ti_stderr, TI_TYPE_MATH, 1, 1, 1, {"real", 0}, {"period", 0}, {"stderr", 0}},
	{"stoch", "Stochastic Oscillator", ti_stoch_start, ti_stoch, TI_TYPE_INDICATOR, 3, 3, 2, {"high", "low", "close", 0}, {"%k period", "%k slowing period", "%d period", 0}, {"stoch_k", "stoch_d", 0}},
	{"sub", "Vector Subtraction", ti_sub_start, ti_sub, TI_TYPE_SIMPLE, 2, 0, 1, {"real", "real", 0}, {"", 0}, {"sub", 0}},
	{"sum", "Sum Over Period", ti_sum_start, ti_sum, TI_TYPE_MATH, 1, 1, 1, {"real", 0}, {"period", 0}, {"sum", 0}},
	{"tan", "Vector Tangent", ti_tan_start, ti_tan, TI_TYPE_SIMPLE, 1, 0, 1, {"real", 0}, {"", 0}, {"tan", 0}},
	{"tanh", "Vector Hyperbolic Tangent", ti_tanh_start, ti_tanh, TI_TYPE_SIMPLE, 1, 0, 1, {"real", 0}, {"", 0}, {"tanh", 0}},
	{"tema", "Triple Exponential Moving Average", ti_tema_start, ti_tema, TI_TYPE_OVERLAY, 1, 1, 1, {"real", 0}, {"period", 0}, {"tema", 0}},
	{"todeg", "Vector Degree Conversion", ti_todeg_start, ti_todeg, TI_TYPE_SIMPLE, 1, 0, 1, {"real", 0}, {"", 0}, {"degrees", 0}},
	{"torad", "Vector Radian Conversion", ti_torad_start, ti_torad, TI_TYPE_SIMPLE, 1, 0, 1, {"real", 0}, {"", 0}, {"radians", 0}},
	{"tr", "True Range", ti_tr_start, ti_tr, TI_TYPE_INDICATOR, 3, 0, 1, {"high", "low", "close", 0}, {"", 0}, {"tr", 0}},
	{"trima", "Triangular Moving Average", ti_trima_start, ti_trima, TI_TYPE_OVERLAY, 1, 1, 1, {"real", 0}, {"period", 0}, {"trima", 0}},
	{"trix", "Trix", ti_trix_start, ti_trix, TI_TYPE_INDICATOR, 1, 1, 1, {"real", 0}, {"period", 0}, {"trix", 0}},
	{"trunc", "Vector Truncate", ti_trunc_start, ti_trunc, TI_TYPE_SIMPLE, 1, 0, 1, {"real", 0}, {"", 0}, {"trunc", 0}},
	{"tsf", "Time Series Forecast", ti_tsf_start, ti_tsf, TI_TYPE_OVERLAY, 1, 1, 1, {"real", 0}, {"period", 0}, {"tsf", 0}},
	{"typprice", "Typical Price", ti_typprice_start, ti_typprice, TI_TYPE_OVERLAY, 3, 0, 1, {"high", "low", "close", 0}, {"", 0}, {"typprice", 0}},
	{"ultosc", "Ultimate Oscillator", ti_ultosc_start, ti_ultosc, TI_TYPE_INDICATOR, 3, 3, 1, {"high", "low", "close", 0}, {"short period", "medium period", "long period", 0}, {"ultosc", 0}},
	{"var", "Variance Over Period", ti_var_start, ti_var, TI_TYPE_MATH, 1, 1, 1, {"real", 0}, {"period", 0}, {"var", 0}},
	{"vhf", "Vertical Horizontal Filter", ti_vhf_start, ti_vhf, TI_TYPE_INDICATOR, 1, 1, 1, {"real", 0}, {"period", 0}, {"vhf", 0}},
	{"vidya", "Variable Index Dynamic Average", ti_vidya_start, ti_vidya, TI_TYPE_OVERLAY, 1, 3, 1, {"real", 0}, {"short period", "long period", "alpha", 0}, {"vidya", 0}},
	{"volatility", "Annualized Historical Volatility", ti_volatility_start, ti_volatility, TI_TYPE_INDICATOR, 1, 1, 1, {"real", 0}, {"period", 0}, {"volatility", 0}},
	{"vosc", "Volume Oscillator", ti_vosc_start, ti_vosc, TI_TYPE_INDICATOR, 1, 2, 1, {"volume", 0}, {"short period", "long period", 0}, {"vosc", 0}},
	{"vwma", "Volume Weighted Moving Average", ti_vwma_start, ti_vwma, TI_TYPE_OVERLAY, 2, 1, 1, {"close", "volume", 0}, {"period", 0}, {"vwma", 0}},
	{"wad", "Williams Accumulation/Distribution", ti_wad_start, ti_wad, TI_TYPE_INDICATOR, 3, 0, 1, {"high", "low", "close", 0}, {"", 0}, {"wad", 0}},
	{"wcprice", "Weighted Close Price", ti_wcprice_start, ti_wcprice, TI_TYPE_OVERLAY, 3, 0, 1, {"high", "low", "close", 0}, {"", 0}, {"wcprice", 0}},
	{"wilders", "Wilders Smoothing", ti_wilders_start, ti_wilders, TI_TYPE_OVERLAY, 1, 1, 1, {"real", 0}, {"period", 0}, {"wilders", 0}},
	{"willr", "Williams %R", ti_willr_start, ti_willr, TI_TYPE_INDICATOR, 3, 1, 1, {"high", "low", "close", 0}, {"period", 0}, {"willr", 0}},
	{"wma", "Weighted Moving Average", ti_wma_start, ti_wma, TI_TYPE_OVERLAY, 1, 1, 1, {"real", 0}, {"period", 0}, {"wma", 0}},
	{"zlema", "Zero-Lag Exponential Moving Average", ti_zlema_start, ti_zlema, TI_TYPE_OVERLAY, 1, 1, 1, {"real", 0}, {"period", 0}, {"zlema", 0}},
	{0, 0, 0, 0, 0, 0, 0, 0, {0, 0}, {0, 0}, {0, 0}}
};

const ti_indicator_info * ti_find_indicator(const char * name) 
{
	int imin = 0;
	int imax = sizeof(ti_indicators) / sizeof(ti_indicator_info) - 2;
	// Binary search.
	while(imax >= imin) {
		const int i = (imin + ((imax-imin)/2));
		const int c = strcmp(name, ti_indicators[i].name);
		if(c == 0) {
			return ti_indicators + i;
		}
		else if(c > 0) {
			imin = i + 1;
		}
		else {
			imax = i - 1;
		}
	}
	return 0;
}
