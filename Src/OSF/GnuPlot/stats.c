/* GNUPLOT - stats.c */

/*
 * Permission to use, copy, and distribute this software and its
 * documentation for any purpose with or without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.
 *
 * Permission to modify the software is granted, but not the right to
 * distribute the complete modified source code.  Modifications are to
 * be distributed as patches to the released version.  Permission to
 * distribute binaries produced by compiling modified sources is granted,
 * provided you
 *   1. distribute the corresponding source modifications from the
 *    released version in the form of a patch file along with the binaries,
 *   2. add special version identification to distinguish your version
 *    in addition to the base release version number,
 *   3. provide your name and address as the primary contact for the
 *    support of your modified version, and
 *   4. retain our contact information in regard to use of the base
 *    software.
 * Permission to distribute the released version of the source code along
 * with corresponding source modifications in the form of a patch file is
 * granted with same provisions 2 through 4 for binary distributions.
 *
 * This software is provided "as is" without express or implied warranty
 * to the extent permitted by applicable law.
 */
#include <gnuplot.h>
#pragma hdrstop
#ifdef USE_STATS  /* Only compile this if configured with --enable-stats */

#define INITIAL_DATA_SIZE (4096)   /* initial size of data arrays */

static int comparator( const void * a, const void * b );
static struct file_stats analyze_file( long n, int outofrange, int invalid, int blank, int dblblank );
static sgl_column_stats analyze_sgl_column( double * data, long n, long nr );
static two_column_stats analyze_two_columns( double * x, double * y, sgl_column_stats res_x, sgl_column_stats res_y, long n );

//static void ensure_output();
static char* fmt( char * buf, double val );
static void sgl_column_output_nonformat(GpCommand & rC, sgl_column_stats s, char * x);
static void file_output(GpCommand & rC, struct file_stats s);
static void sgl_column_output(GpCommand & rC, sgl_column_stats s, long n);
static void two_column_output(GpCommand & rC, sgl_column_stats x, sgl_column_stats y, two_column_stats xy, long n);
static void create_and_set_var( double val, char * prefix, char * base, char * suffix );
static void create_and_set_int_var( int ival, char * prefix, char * base, char * suffix );
static void create_and_store_var( t_value *data, char * prefix, char * base, char * suffix );
static void sgl_column_variables( sgl_column_stats res, char * prefix, char * postfix );

/* =================================================================
   Data Structures
   ================================================================= */

// Keeps info on a value and its index in the file 
struct pair {
	double val;
	long index;
};

/* Collect results from analysis */
struct file_stats {
	long records;
	long blanks;
	long invalid;
	long outofrange;
	long blocks; /* blocks are separated by double blank lines */
};

struct sgl_column_stats {
	/* Matrix dimensions */
	int sx;
	int sy;

	double mean;
	double adev;
	double stddev;
	double ssd;     /* sample standard deviation */
	double skewness;
	double kurtosis;
	double mean_err;
	double stddev_err;
	double skewness_err;
	double kurtosis_err;
	double sum;      /* sum x    */
	double sum_sq;   /* sum x**2 */
	struct pair min;
	struct pair max;
	double median;
	double lower_quartile;
	double upper_quartile;
	double cog_x; /* centre of gravity */
	double cog_y;
	/* info on data points out of bounds? */
};

struct two_column_stats {
	double sum_xy;
	double slope;    /* linear regression */
	double intercept;
	double slope_err;
	double intercept_err;
	double correlation;
	double pos_min_y; /* x GpCoordinate of min y */
	double pos_max_y; /* x GpCoordinate of max y */
};

/* =================================================================
   Analysis and Output
   ================================================================= */

/* Needed by qsort() when we sort the data points to find the median.   */
/* FIXME: I am dubious that keeping the original index gains anything. */
/* It makes no sense at all for quartiles,  and even the min/max are not  */
/* guaranteed to be unique.                                               */
static int comparator(const void * a, const void * b)
{
	struct pair * x = (struct pair*)a;
	struct pair * y = (struct pair*)b;
	if(x->val < y->val) return -1;
	if(x->val > y->val) return 1;
	return 0;
}

static struct file_stats analyze_file(long n, int outofrange, int invalid, int blank, int dblblank)
{
	struct file_stats res;
	res.records = n;
	res.invalid = invalid;
	res.blanks  = blank;
	res.blocks  = dblblank + 1; /* blocks are separated by dbl blank lines */
	res.outofrange = outofrange;
	return res;
}

static sgl_column_stats analyze_sgl_column(double * data, long n, long nc)
{
	sgl_column_stats res;
	long i;
	double s  = 0.0;
	double s2 = 0.0;
	double ad = 0.0;
	double d  = 0.0;
	double d2 = 0.0;
	double d3 = 0.0;
	double d4 = 0.0;
	double cx = 0.0;
	double cy = 0.0;
	double var;

	struct pair * tmp = (struct pair*)malloc(n*sizeof(struct pair), "analyze_sgl_column");
	if(nc > 0) {
		res.sx = nc;
		res.sy = n / nc;
	}
	else {
		res.sx = 0;
		res.sy = n;
	}
	/* Mean and centre of gravity */
	for(i = 0; i<n; i++) {
		s  += data[i];
		s2 += data[i]*data[i];
		if(nc > 0) {
			cx += data[i]*(i % res.sx);
			cy += data[i]*(i / res.sx);
		}
	}
	res.mean = s/(double)n;

	res.sum  = s;
	res.sum_sq = s2;

	/* Standard deviation, mean absolute deviation, skewness, and kurtosis */
	for(i = 0; i<n; i++) {
		double t = data[i] - res.mean;
		ad += fabs(t);
		d  += t;
		d2 += t*t;
		d3 += t*t*t;
		d4 += t*t*t*t;
	}

	/* population (not sample) variance, stddev, skew, kurtosis */
	var = (d2 - d * d / n) / n;
	res.stddev = sqrt(var);
	res.adev = ad / n;
	if(var != 0.0) {
		res.skewness = d3 / (n * var * res.stddev);
		res.kurtosis = d4 / (n * var * var);
	}
	else {
		res.skewness = res.kurtosis = not_a_number();
	}

	res.mean_err = res.stddev / sqrt((double)n);
	res.stddev_err = res.stddev / sqrt(2.0 * n);
	res.skewness_err = sqrt(6.0 / n);
	res.kurtosis_err = sqrt(24.0 / n);
	// sample standard deviation 
	res.ssd = res.stddev * sqrt((double)(n) / (double)(n-1));
	for(i = 0; i<n; i++) {
		tmp[i].val = data[i];
		tmp[i].index = i;
	}
	qsort(tmp, n, sizeof(struct pair), comparator);
	res.min = tmp[0];
	res.max = tmp[n-1];
	//
	// This uses the same quartile definitions as the boxplot code in graphics.c
	//
	if((n & 0x1) == 0)
		res.median = 0.5 * (tmp[n/2 - 1].val + tmp[n/2].val);
	else
		res.median = tmp[(n-1)/2].val;
	if((n & 0x3) == 0)
		res.lower_quartile = 0.5 * (tmp[n/4 - 1].val + tmp[n/4].val);
	else
		res.lower_quartile = tmp[(n+3)/4 - 1].val;
	if((n & 0x3) == 0)
		res.upper_quartile = 0.5 * (tmp[n - n/4].val + tmp[n - n/4 - 1].val);
	else
		res.upper_quartile = tmp[n - (n+3)/4].val;
	/* Note: the centre of gravity makes sense for positive value matrices only */
	if(cx == 0.0 && cy == 0.0) {
		res.cog_x = 0.0;
		res.cog_y = 0.0;
	}
	else {
		res.cog_x = cx / s;
		res.cog_y = cy / s;
	}
	free(tmp);
	return res;
}

static two_column_stats analyze_two_columns(double * x, double * y, sgl_column_stats res_x, sgl_column_stats res_y, long n)
{
	two_column_stats res;
	long i;
	double s = 0;
	double ssyy, ssxx, ssxy;
	for(i = 0; i<n; i++) {
		s += x[i] * y[i];
	}
	res.sum_xy = s;
	// Definitions according to http://mathworld.wolfram.com/LeastSquaresFitting.html
	ssyy = res_y.sum_sq - res_y.sum * res_y.sum / n;
	ssxx = res_x.sum_sq - res_x.sum * res_x.sum / n;
	ssxy = res.sum_xy   - res_x.sum * res_y.sum / n;
	res.slope = (ssxx != 0.0) ? (ssxy / ssxx) : not_a_number();
	res.intercept = res_y.mean - res.slope * res_x.mean;
	res.correlation = (res_y.stddev != 0.0) ? (res.slope * res_x.stddev / res_y.stddev) : not_a_number();
	if(n > 2) {
		double ss = (ssyy - res.slope * ssxy) / (n - 2);
		if(ssxx != 0.0) {
			res.slope_err = sqrt(ss / ssxx);
			res.intercept_err = sqrt(ss * (1./n + res_x.sum * res_x.sum / (n * n * ssxx)));
		}
		else {
			res.slope_err = res.intercept_err = not_a_number();
		}
	}
	else if(n == 2) {
		fprintf(stderr, "Warning:  Errors of slope and intercept are zero. There are as many data points as there are parameters.\n");
		res.slope_err = res.intercept_err = 0.0;
	}
	else {
		fprintf(stderr, "Warning:  Can't compute errors of slope and intercept. Not enough data points.\n");
		res.slope_err = res.intercept_err = not_a_number();
	}
	res.pos_min_y = x[res_y.min.index];
	res.pos_max_y = x[res_y.max.index];
	return res;
}

/* =================================================================
   Output
   ================================================================= */

// Output
// Note: GpC.F_PrintOut is a FILE ptr, set by the "set print" command
//static void ensure_output()
void GpCommand::EnsureOutput()
{
	SETIFZ(F_PrintOut, stderr);
}

static char* fmt(char * buf, double val)
{
	if(fisnan(val) )
		sprintf(buf, "%11s", "undefined");
	else if(fabs(val) < 1e-14)
		sprintf(buf, "%11.4f", 0.0);
	else if(fabs(log10(fabs(val))) < 6)
		sprintf(buf, "%11.4f", val);
	else
		sprintf(buf, "%11.5e", val);
	return buf;
}

static void file_output(GpCommand & rC, struct file_stats s)
{
	int width = 3;
	// Assuming that records is the largest number of the four...
	if(s.records > 0)
		width = 1 + (int)( log10((double)s.records) );
	rC.EnsureOutput();
	// Non-formatted to disk
	if(rC.F_PrintOut != stdout && rC.F_PrintOut != stderr) {
		fprintf(rC.F_PrintOut, "%s\t%ld\n", "records", s.records);
		fprintf(rC.F_PrintOut, "%s\t%ld\n", "invalid", s.invalid);
		fprintf(rC.F_PrintOut, "%s\t%ld\n", "blanks", s.blanks);
		fprintf(rC.F_PrintOut, "%s\t%ld\n", "blocks", s.blocks);
		fprintf(rC.F_PrintOut, "%s\t%ld\n", "outofrange", s.outofrange);
	}
	else {
		// Formatted to screen
		fprintf(rC.F_PrintOut, "\n");
		fprintf(rC.F_PrintOut, "* FILE: \n");
		fprintf(rC.F_PrintOut, "  Records:           %*ld\n", width, s.records);
		fprintf(rC.F_PrintOut, "  Out of range:      %*ld\n", width, s.outofrange);
		fprintf(rC.F_PrintOut, "  Invalid:           %*ld\n", width, s.invalid);
		fprintf(rC.F_PrintOut, "  Blank:             %*ld\n", width, s.blanks);
		fprintf(rC.F_PrintOut, "  Data Blocks:       %*ld\n", width, s.blocks);
	}
}

static void sgl_column_output_nonformat(GpCommand & rC, sgl_column_stats s, char * x)
{
	fprintf(rC.F_PrintOut, "%s%s\t%f\n", "mean",     x, s.mean);
	fprintf(rC.F_PrintOut, "%s%s\t%f\n", "stddev",   x, s.stddev);
	fprintf(rC.F_PrintOut, "%s%s\t%f\n", "ssd",      x, s.ssd);
	fprintf(rC.F_PrintOut, "%s%s\t%f\n", "skewness", x, s.skewness);
	fprintf(rC.F_PrintOut, "%s%s\t%f\n", "kurtosis", x, s.kurtosis);
	fprintf(rC.F_PrintOut, "%s%s\t%f\n", "adev",     x, s.adev);
	fprintf(rC.F_PrintOut, "%s%s\t%f\n", "sum",      x, s.sum);
	fprintf(rC.F_PrintOut, "%s%s\t%f\n", "sum_sq",   x, s.sum_sq);

	fprintf(rC.F_PrintOut, "%s%s\t%f\n", "mean_err",     x, s.mean_err);
	fprintf(rC.F_PrintOut, "%s%s\t%f\n", "stddev_err",   x, s.stddev_err);
	fprintf(rC.F_PrintOut, "%s%s\t%f\n", "skewness_err", x, s.skewness_err);
	fprintf(rC.F_PrintOut, "%s%s\t%f\n", "kurtosis_err", x, s.kurtosis_err);

	fprintf(rC.F_PrintOut, "%s%s\t%f\n", "min",     x, s.min.val);
	if(s.sx == 0) {
		fprintf(rC.F_PrintOut, "%s%s\t%f\n", "lo_quartile", x, s.lower_quartile);
		fprintf(rC.F_PrintOut, "%s%s\t%f\n", "median",      x, s.median);
		fprintf(rC.F_PrintOut, "%s%s\t%f\n", "up_quartile", x, s.upper_quartile);
	}
	fprintf(rC.F_PrintOut, "%s%s\t%f\n", "max",     x, s.max.val);

	/* If data set is matrix */
	if(s.sx > 0) {
		fprintf(rC.F_PrintOut, "%s%s\t%ld\n", "index_min_x",  x, (s.min.index) % s.sx);
		fprintf(rC.F_PrintOut, "%s%s\t%ld\n", "index_min_y",  x, (s.min.index) / s.sx);
		fprintf(rC.F_PrintOut, "%s%s\t%ld\n", "index_max_x",  x, (s.max.index) % s.sx);
		fprintf(rC.F_PrintOut, "%s%s\t%ld\n", "index_max_y",  x, (s.max.index) / s.sx);
		fprintf(rC.F_PrintOut, "%s%s\t%f\n", "cog_x",  x, s.cog_x);
		fprintf(rC.F_PrintOut, "%s%s\t%f\n", "cog_y",  x, s.cog_y);
	}
	else {
		fprintf(rC.F_PrintOut, "%s%s\t%ld\n", "min_index",  x, s.min.index);
		fprintf(rC.F_PrintOut, "%s%s\t%ld\n", "max_index",  x, s.max.index);
	}
}

static void sgl_column_output(GpCommand & rC, sgl_column_stats s, long n)
{
	int    width = 1;
	char   buf[32];
	char   buf2[32];
	if(n > 0)
		width = 1 + (int)( log10( (double)n) );
	rC.EnsureOutput();
	// Non-formatted to disk
	if(rC.F_PrintOut != stdout && rC.F_PrintOut != stderr) {
		sgl_column_output_nonformat(rC, s, "_y");
	}
	else {
		// Formatted to screen
		fprintf(rC.F_PrintOut, "\n");
		// First, we check whether the data file was a matrix
		if(s.sx > 0)
			fprintf(rC.F_PrintOut, "* MATRIX: [%d X %d] \n", s.sx, s.sy);
		else
			fprintf(rC.F_PrintOut, "* COLUMN: \n");

		fprintf(rC.F_PrintOut, "  Mean:          %s\n", fmt(buf, s.mean) );
		fprintf(rC.F_PrintOut, "  Std Dev:       %s\n", fmt(buf, s.stddev) );
		fprintf(rC.F_PrintOut, "  Sample StdDev: %s\n", fmt(buf, s.ssd) );
		fprintf(rC.F_PrintOut, "  Skewness:      %s\n", fmt(buf, s.skewness) );
		fprintf(rC.F_PrintOut, "  Kurtosis:      %s\n", fmt(buf, s.kurtosis) );
		fprintf(rC.F_PrintOut, "  Avg Dev:       %s\n", fmt(buf, s.adev) );
		fprintf(rC.F_PrintOut, "  Sum:           %s\n", fmt(buf, s.sum) );
		fprintf(rC.F_PrintOut, "  Sum Sq.:       %s\n", fmt(buf, s.sum_sq) );
		fprintf(rC.F_PrintOut, "\n");

		fprintf(rC.F_PrintOut, "  Mean Err.:     %s\n", fmt(buf, s.mean_err) );
		fprintf(rC.F_PrintOut, "  Std Dev Err.:  %s\n", fmt(buf, s.stddev_err) );
		fprintf(rC.F_PrintOut, "  Skewness Err.: %s\n", fmt(buf, s.skewness_err) );
		fprintf(rC.F_PrintOut, "  Kurtosis Err.: %s\n", fmt(buf, s.kurtosis_err) );
		fprintf(rC.F_PrintOut, "\n");
		// For matrices, the quartiles and the median do not make too much sense
		if(s.sx > 0) {
			fprintf(rC.F_PrintOut, "  Minimum:       %s [%*ld %ld ]\n", fmt(buf, s.min.val), width, (s.min.index) % s.sx, (s.min.index) / s.sx);
			fprintf(rC.F_PrintOut, "  Maximum:       %s [%*ld %ld ]\n", fmt(buf, s.max.val), width, (s.max.index) % s.sx, (s.max.index) / s.sx);
			fprintf(rC.F_PrintOut, "  COG:           %s %s\n", fmt(buf, s.cog_x), fmt(buf2, s.cog_y) );
		}
		else {
			// FIXME:  The "position" are randomly selected from a non-unique set. Bad!
			fprintf(rC.F_PrintOut, "  Minimum:       %s [%*ld]\n", fmt(buf, s.min.val), width, s.min.index);
			fprintf(rC.F_PrintOut, "  Maximum:       %s [%*ld]\n", fmt(buf, s.max.val), width, s.max.index);
			fprintf(rC.F_PrintOut, "  Quartile:      %s \n", fmt(buf, s.lower_quartile) );
			fprintf(rC.F_PrintOut, "  Median:        %s \n", fmt(buf, s.median) );
			fprintf(rC.F_PrintOut, "  Quartile:      %s \n", fmt(buf, s.upper_quartile) );
			fprintf(rC.F_PrintOut, "\n");
		}
	}
}

static void two_column_output(GpCommand & rC, sgl_column_stats x, sgl_column_stats y, two_column_stats xy, long n)
{
	int width = 1;
	char bfx[32];
	char bfy[32];
	char blank[32];

	if(n > 0)
		width = 1 + (int)log10((double)n);

	/* Non-formatted to disk */
	if(rC.F_PrintOut != stdout && rC.F_PrintOut != stderr) {
		sgl_column_output_nonformat(rC, x, "_x");
		sgl_column_output_nonformat(rC, y, "_y");
		fprintf(rC.F_PrintOut, "%s\t%f\n", "slope", xy.slope);
		if(n > 2)
			fprintf(rC.F_PrintOut, "%s\t%f\n", "slope_err", xy.slope_err);
		fprintf(rC.F_PrintOut, "%s\t%f\n", "intercept", xy.intercept);
		if(n > 2)
			fprintf(rC.F_PrintOut, "%s\t%f\n", "intercept_err", xy.intercept_err);
		fprintf(rC.F_PrintOut, "%s\t%f\n", "correlation", xy.correlation);
		fprintf(rC.F_PrintOut, "%s\t%f\n", "sumxy", xy.sum_xy);
		return;
	}

	/* Create a string of blanks of the required length */
	strncpy(blank, "                 ", width+4);
	blank[width+4] = '\0';

	rC.EnsureOutput();

	fprintf(rC.F_PrintOut, "\n");
	fprintf(rC.F_PrintOut, "* COLUMNS:\n");
	fprintf(rC.F_PrintOut, "  Mean:          %s %s %s\n", fmt(bfx, x.mean),   blank, fmt(bfy, y.mean) );
	fprintf(rC.F_PrintOut, "  Std Dev:       %s %s %s\n", fmt(bfx, x.stddev), blank, fmt(bfy, y.stddev) );
	fprintf(rC.F_PrintOut, "  Sample StdDev: %s %s %s\n", fmt(bfx, x.ssd), blank, fmt(bfy, y.ssd) );
	fprintf(rC.F_PrintOut, "  Skewness:      %s %s %s\n", fmt(bfx, x.skewness), blank, fmt(bfy, y.skewness) );
	fprintf(rC.F_PrintOut, "  Kurtosis:      %s %s %s\n", fmt(bfx, x.kurtosis), blank, fmt(bfy, y.kurtosis) );
	fprintf(rC.F_PrintOut, "  Avg Dev:       %s %s %s\n", fmt(bfx, x.adev), blank, fmt(bfy, y.adev) );
	fprintf(rC.F_PrintOut, "  Sum:           %s %s %s\n", fmt(bfx, x.sum),  blank, fmt(bfy, y.sum) );
	fprintf(rC.F_PrintOut, "  Sum Sq.:       %s %s %s\n", fmt(bfx, x.sum_sq), blank, fmt(bfy, y.sum_sq) );
	fprintf(rC.F_PrintOut, "\n");

	fprintf(rC.F_PrintOut, "  Mean Err.:     %s %s %s\n", fmt(bfx, x.mean_err),   blank, fmt(bfy, y.mean_err) );
	fprintf(rC.F_PrintOut, "  Std Dev Err.:  %s %s %s\n", fmt(bfx, x.stddev_err), blank, fmt(bfy, y.stddev_err) );
	fprintf(rC.F_PrintOut, "  Skewness Err.: %s %s %s\n", fmt(bfx, x.skewness_err), blank, fmt(bfy, y.skewness_err) );
	fprintf(rC.F_PrintOut, "  Kurtosis Err.: %s %s %s\n", fmt(bfx, x.kurtosis_err), blank, fmt(bfy, y.kurtosis_err) );
	fprintf(rC.F_PrintOut, "\n");

	/* FIXME:  The "positions" are randomly selected from a non-unique set.  Bad! */
	fprintf(rC.F_PrintOut, "  Minimum:       %s [%*ld]   %s [%*ld]\n", fmt(bfx, x.min.val), width, x.min.index, fmt(bfy, y.min.val), width, y.min.index);
	fprintf(rC.F_PrintOut, "  Maximum:       %s [%*ld]   %s [%*ld]\n", fmt(bfx, x.max.val), width, x.max.index, fmt(bfy, y.max.val), width, y.max.index);
	fprintf(rC.F_PrintOut, "  Quartile:      %s %s %s\n", fmt(bfx, x.lower_quartile), blank, fmt(bfy, y.lower_quartile) );
	fprintf(rC.F_PrintOut, "  Median:        %s %s %s\n", fmt(bfx, x.median), blank, fmt(bfy, y.median) );
	fprintf(rC.F_PrintOut, "  Quartile:      %s %s %s\n", fmt(bfx, x.upper_quartile), blank, fmt(bfy, y.upper_quartile) );
	fprintf(rC.F_PrintOut, "\n");
	// Simpler below - don't care about alignment
	if(xy.intercept < 0.0)
		fprintf(rC.F_PrintOut, "  Linear Model:       y = %.4g x - %.4g\n", xy.slope, -xy.intercept);
	else
		fprintf(rC.F_PrintOut, "  Linear Model:       y = %.4g x + %.4g\n", xy.slope, xy.intercept);
	fprintf(rC.F_PrintOut, "  Slope:              %.4g +- %.4g\n", xy.slope, xy.slope_err);
	fprintf(rC.F_PrintOut, "  Intercept:          %.4g +- %.4g\n", xy.intercept, xy.intercept_err);

	fprintf(rC.F_PrintOut, "  Correlation:        r = %.4g\n", xy.correlation);
	fprintf(rC.F_PrintOut, "  Sum xy:             %.4g\n", xy.sum_xy);
	fprintf(rC.F_PrintOut, "\n");
}

/* =================================================================
   Variable Handling
   ================================================================= */

static void create_and_set_var(double val, char * prefix, char * base, char * suffix)
{
	t_value data;
	Gcomplex(&data, val, 0.0); /* data is complex, real=val, imag=0.0 */
	create_and_store_var(&data, prefix, base, suffix);
}

static void create_and_set_int_var(int ival, char * prefix, char * base, char * suffix)
{
	t_value data;
	Ginteger(&data, ival);
	create_and_store_var(&data, prefix, base, suffix);
}

static void create_and_store_var(t_value * data, char * prefix, char * base, char * suffix)
{
	int len;
	char * varname;
	UdvtEntry * udv_ptr;
	// In case prefix (or suffix) is NULL - make them empty strings
	prefix = prefix ? prefix : "";
	suffix = suffix ? suffix : "";
	len = strlen(prefix) + strlen(base) + strlen(suffix) + 1;
	varname = (char*)malloc(len, "create_and_set_var");
	sprintf(varname, "%s%s%s", prefix, base, suffix);
	//
	// Note that GpGg.Ev.AddUdvByName() checks if the name already exists, and
	// returns the existing ptr if found. It also allocates memory for its own copy of the varname.
	//
	udv_ptr = GpGg.Ev.AddUdvByName(varname);
	udv_ptr->udv_value = *data;
	free(varname);
}

static void file_variables(struct file_stats s, char * prefix)
{
	/* Suffix does not make sense here! */
	create_and_set_int_var(s.records, prefix, "records", "");
	create_and_set_int_var(s.invalid, prefix, "invalid", "");
	create_and_set_int_var(s.blanks,  prefix, "blank",   "");
	create_and_set_int_var(s.blocks,  prefix, "blocks",  "");
	create_and_set_int_var(s.outofrange, prefix, "outofrange", "");
	create_and_set_int_var(df_last_col, prefix, "columns", "");
}

static void sgl_column_variables(sgl_column_stats s, char * prefix, char * suffix)
{
	create_and_set_var(s.mean,     prefix, "mean",     suffix);
	create_and_set_var(s.stddev,   prefix, "stddev",   suffix);
	create_and_set_var(s.ssd,      prefix, "ssd",      suffix);
	create_and_set_var(s.skewness, prefix, "skewness", suffix);
	create_and_set_var(s.kurtosis, prefix, "kurtosis", suffix);
	create_and_set_var(s.adev,     prefix, "adev",     suffix);

	create_and_set_var(s.mean_err,     prefix, "mean_err",     suffix);
	create_and_set_var(s.stddev_err,   prefix, "stddev_err",   suffix);
	create_and_set_var(s.skewness_err, prefix, "skewness_err", suffix);
	create_and_set_var(s.kurtosis_err, prefix, "kurtosis_err", suffix);

	create_and_set_var(s.sum,    prefix, "sum",   suffix);
	create_and_set_var(s.sum_sq, prefix, "sumsq", suffix);

	create_and_set_var(s.min.val, prefix, "min", suffix);
	create_and_set_var(s.max.val, prefix, "max", suffix);

	/* If data set is matrix */
	if(s.sx > 0) {
		create_and_set_int_var( (s.min.index) % s.sx, prefix, "index_min_x", suffix);
		create_and_set_int_var( (s.min.index) / s.sx, prefix, "index_min_y", suffix);
		create_and_set_int_var( (s.max.index) % s.sx, prefix, "index_max_x", suffix);
		create_and_set_int_var( (s.max.index) / s.sx, prefix, "index_max_y", suffix);
		create_and_set_int_var(s.sx, prefix, "size_x", suffix);
		create_and_set_int_var(s.sy, prefix, "size_y", suffix);
	}
	else {
		create_and_set_var(s.median,         prefix, "median",      suffix);
		create_and_set_var(s.lower_quartile, prefix, "lo_quartile", suffix);
		create_and_set_var(s.upper_quartile, prefix, "up_quartile", suffix);
		create_and_set_int_var(s.min.index, prefix, "index_min", suffix);
		create_and_set_int_var(s.max.index, prefix, "index_max", suffix);
	}
}

static void two_column_variables(two_column_stats s, char * prefix, long n)
{
	/* Suffix does not make sense here! */
	create_and_set_var(s.slope,         prefix, "slope",         "");
	create_and_set_var(s.intercept,     prefix, "intercept",     "");
	/* The errors can only calculated for n > 2, but we set them (to zero) anyway. */
	create_and_set_var(s.slope_err,     prefix, "slope_err",     "");
	create_and_set_var(s.intercept_err, prefix, "intercept_err", "");
	create_and_set_var(s.correlation,   prefix, "correlation",   "");
	create_and_set_var(s.sum_xy,        prefix, "sumxy",         "");

	create_and_set_var(s.pos_min_y,     prefix, "pos_min_y",     "");
	create_and_set_var(s.pos_max_y,     prefix, "pos_max_y",     "");
}

/* =================================================================
   Range Handling
   ================================================================= */

/* We validate our data here: discard everything that is outside
 * the specified range. However, we have to be a bit careful here,
 * because if no range is specified, we keep everything
 */
/* static bool validate_data(double v, AXIS_INDEX ax)
{
	// These are flag bits, not constants!!!
	if((GpGg[ax].AutoScale & AUTOSCALE_BOTH) == AUTOSCALE_BOTH)
		return true;
	else if(((GpGg[ax].AutoScale & AUTOSCALE_BOTH) == AUTOSCALE_MIN) && (v <= GpGg[ax].max))
		return true;
	else if(((GpGg[ax].AutoScale & AUTOSCALE_BOTH) == AUTOSCALE_MAX) && (v >= GpGg[ax].min))
		return true;
	else if(((GpGg[ax].AutoScale & AUTOSCALE_BOTH) == AUTOSCALE_NONE) && ((v <= GpGg[ax].max) && (v >= GpGg[ax].min)))
		return(true);
	else
		return(false);
} */

/* =================================================================
   Parse Command Line and Process
   ================================================================= */

void GpGadgets::StatsRequest(GpCommand & rC)
{
	static char * file_name = NULL;
	static double * data_x = NULL;
	static double * data_y = NULL; /* values read from file */
	static char * prefix = NULL;  /* prefix for user-defined vars names */

	int i;
	int columns;
	double v[2];
	char * temp_name;

	/* Vars to hold data and results */
	long n;            /* number of records retained */
	long max_n;
	long invalid;      /* number of missing/invalid records */
	long blanks;       /* number of blank lines */
	long doubleblanks; /* number of repeated blank lines */
	long out_of_range; /* number pts rejected, because out of range */

	struct file_stats res_file;
	sgl_column_stats res_x = {0}, res_y = {0};
	two_column_stats res_xy = {0};
	// Vars that control output
	bool do_output = true; // Generate formatted output
	bool array_data = false;
	rC.CToken++;
	// Parse ranges 
	AXIS_INIT2D(FIRST_X_AXIS, 0);
	AXIS_INIT2D(FIRST_Y_AXIS, 0);
	ParseRange(FIRST_X_AXIS, rC);
	ParseRange(FIRST_Y_AXIS, rC);
	// Initialize 
	invalid = 0;      /* number of missing/invalid records */
	blanks = 0;       /* number of blank lines */
	doubleblanks = 0; /* number of repeated blank lines */
	out_of_range = 0; /* number pts rejected, because out of range */
	n = 0;            /* number of records retained */
	max_n = INITIAL_DATA_SIZE;

	free(data_x);
	free(data_y);
	data_x = vec(max_n);   /* start with max. value */
	data_y = vec(max_n);
	if(!data_x || !data_y)
		IntError(GpC, NO_CARET, "Internal error: out of memory in stats");
	n = invalid = blanks = doubleblanks = out_of_range = 0;
	/* Get filename */
	i = rC.CToken;
	temp_name = string_or_express(NULL);
	if(temp_name) {
		free(file_name);
		file_name = gp_strdup(temp_name);
	}
	else
		IntError(GpC, i, "missing filename or datablock");
	/* Jan 2015: We used to handle ascii matrix data as a special case but
	 * the code did not work correctly.  Since df_read_matrix() dummies up
	 * ascii matrix data to look as if had been presented as a binary blob,
	 * we should be able to proceed with no special attention other than
	 * to set the effective number of columns to 1.
	 */
	if(true) {
		GpDf.DfSetPlotMode(MODE_PLOT); // Used for matrix datafiles 
		columns = GpDf.DfOpen(rC, file_name, 2, NULL); // up to 2 using specs allowed 
		if(columns < 0) {
			int_warn(NO_CARET, "Can't read data file");
			while(!rC.EndOfCommand())
				rC.CToken++;
			goto stats_cleanup;
		}
		if(rC.P.P_DfArray && columns == 0)
			array_data = true;
		// For all these below: we could save the state, switch off, then restore
		if(AxA[FIRST_X_AXIS].log || AxA[FIRST_Y_AXIS].log)
			IntError(GpC, NO_CARET, "Stats command not available with logscale active");
		if(AxA[FIRST_X_AXIS].datatype == DT_TIMEDATE || AxA[FIRST_Y_AXIS].datatype == DT_TIMEDATE)
			IntError(GpC, NO_CARET, "Stats command not available in timedata mode");
		if(polar)
			IntError(GpC, NO_CARET, "Stats command not available in polar mode");
		if(parametric)
			IntError(GpC, NO_CARET, "Stats command not available in parametric mode");
		/* If the user has set an explicit locale for numeric input, apply it */
		/* here so that it affects data fields read from the input file. */
		set_numeric_locale();
		/* The way readline and friends work is as follows:
		   - df_open will return the number of columns requested in the using spec
		   so that "columns" will be 0, 1, or 2 (no using, using 1, using 1:2)
		   - readline always returns the same number of columns (for us: 1 or 2)
		   - using n:m = return two columns, skipping lines w/ bad data
		   - using n   = return single column (supply zeros (0) for the second col)
		   - no using  = first two columns if both are present on the first line of data
		               else first column only
		 */
		while( (i = df_readline(v, 2)) != DF_EOF) {
			if(n >= max_n) {
				max_n = (max_n * 3) / 2; // increase max_n by factor of 1.5
				// Some of the reallocations went bad:
				if(!redim_vec(&data_x, max_n) || !redim_vec(&data_y, max_n) ) {
					df_close();
					IntError(GpC, NO_CARET, "Out of memory in stats: too many datapoints (%d)?", max_n);
				}
			} /* if(need to extend storage space) */
			switch(i) {
				case DF_MISSING:
				case DF_UNDEFINED:
				    invalid += 1;
				    continue;
				case DF_FIRST_BLANK:
				    blanks += 1;
				    continue;
				case DF_SECOND_BLANK:
				    blanks += 1;
				    doubleblanks += 1;
				    continue;
				case 0:
				    IntError(GpC, NO_CARET, "bad data on line %d of file %s",
				    df_line_number, df_filename ? df_filename : "");
				    break;

				case 1: /* Read single column successfully  */
				    if(ValidateData(v[0], FIRST_Y_AXIS) ) {
					    data_y[n] = v[0];
					    n++;
				    }
				    else {
					    out_of_range++;
				    }
				    columns = 1;
				    break;
				case 2: /* Read two columns successfully  */
				    if(ValidateData(v[0], FIRST_X_AXIS) && ValidateData(v[1], FIRST_Y_AXIS)) {
					    data_x[n] = v[0];
					    data_y[n] = v[1];
					    n++;
				    }
				    else {
					    out_of_range++;
				    }
				    columns = 2;
				    break;
			}
		}
		df_close();
		// now resize fields to actual length: 
		redim_vec(&data_x, n);
		redim_vec(&data_y, n);
	}
	// Now finished reading user input; return to C locale for internal use
	reset_numeric_locale();
	// No data! Try to explain why. 
	if(n == 0) {
		int_warn(NO_CARET, (out_of_range > 0) ? "All points out of range" : "No valid data points found in file");
		// Skip rest of command line and return error 
		while(!rC.EndOfCommand()) 
			rC.CToken++;
		goto stats_cleanup;
	}
	// Parse the remainder of the command line: 0 to 2 tokens possible 
	while(!rC.EndOfCommand()) {
		if(rC.AlmostEq("out$put") ) {
			do_output = true;
			rC.CToken++;
		}
		else if(rC.AlmostEq("noout$put") ) {
			do_output = false;
			rC.CToken++;
		}
		else if(rC.AlmostEq("pre$fix") || rC.Eq("name")) {
			rC.CToken++;
			free(prefix);
			prefix = rC.TryToGetString();
			if(!legal_identifier(prefix) || !strcmp("GPVAL_", prefix))
				IntError(GpC, --rC.CToken, "illegal prefix");
		}
		else {
			IntError(rC, rC.CToken, "Expecting [no]output or prefix");
		}
	}
	// Set defaults if not explicitly set by user
	SETIFZ(prefix, gp_strdup("STATS_"));
	i = strlen(prefix);
	if(prefix[i-1] != '_') {
		prefix = (char*)gp_realloc(prefix, i+2, "prefix");
		strcat(prefix, "_");
	}
	// Do the actual analysis
	res_file = analyze_file(n, out_of_range, invalid, blanks, doubleblanks);
	/* Jan 2015: Revised detection and handling of matrix data */
	if(array_data)
		columns = 1;
	if(df_matrix) {
		int nc = df_bin_record[df_num_bin_records-1].scan_dim[0];
		res_y = analyze_sgl_column(data_y, n, nc);
		columns = 1;
	}
	else if(columns == 1) {
		res_y = analyze_sgl_column(data_y, n, 0);
	}
	else {
		/* If there are two columns, then the data file is not a matrix */
		res_x = analyze_sgl_column(data_x, n, 0);
		res_y = analyze_sgl_column(data_y, n, 0);
		res_xy = analyze_two_columns(data_x, data_y, res_x, res_y, n);
	}
	/* Store results in user-accessible variables */
	/* Clear out any previous use of these variables */
	Ev.DelUdvByName(prefix, true);
	file_variables(res_file, prefix);
	if(columns == 1) {
		sgl_column_variables(res_y, prefix, "");
	}
	if(columns == 2) {
		sgl_column_variables(res_x, prefix, "_x");
		sgl_column_variables(res_y, prefix, "_y");
		two_column_variables(res_xy, prefix, n);
	}
	/* Output */
	if(do_output) {
		file_output(rC, res_file);
		if(columns == 1)
			sgl_column_output(rC, res_y, res_file.records);
		else
			two_column_output(rC, res_x, res_y, res_xy, res_file.records);
	}
	/* Cleanup */
stats_cleanup:
	ZFREE(data_x);
	ZFREE(data_y);
	ZFREE(file_name);
	ZFREE(prefix);
}

#endif /* The whole file is conditional on USE_STATS */
