// GNUPLOT - stats.c 
//
#include <gnuplot.h>
#pragma hdrstop
#ifdef USE_STATS // Only compile this if configured with --enable-stats 

#define INITIAL_DATA_SIZE (4096)   /* initial size of data arrays */

static int comparator(const void * a, const void * b);
static struct SglColumnStats analyze_sgl_column(double * data, long n, long nr);
static struct TwoColumnStats analyze_two_columns(double * x, double * y, struct SglColumnStats res_x, struct SglColumnStats res_y, long n);
static char * fmt(char * buf, double val);

/* =================================================================
   Data Structures
   ================================================================= */
/* =================================================================
   Analysis and Output
   ================================================================= */

/* Needed by qsort() when we sort the data points to find the median.   */
/* FIXME: I am dubious that keeping the original index gains anything. */
/* It makes no sense at all for quartiles,  and even the min/max are not  */
/* guaranteed to be unique.                                               */
static int comparator(const void * a, const void * b)
{
	const GpPair * x = (const GpPair *)a;
	const GpPair * y = (const GpPair *)b;
	if(x->val < y->val) return -1;
	if(x->val > y->val) return 1;
	return 0;
}

//static GpFileStats analyze_file(long n, int outofrange, int invalid, int blank, int dblblank, int headers) 
GpFileStats GnuPlot::AnalyzeFile(long n, int outofrange, int invalid, int blank, int dblblank, int headers)
{
	GpFileStats res;
	res.records = n;
	res.invalid = invalid;
	res.blanks  = blank;
	res.blocks  = dblblank + 1;/* blocks are separated by dbl blank lines */
	res.outofrange = outofrange;
	res.header_records = headers;
	res.columns = _Df.df_last_col;
	return res;
}

static SglColumnStats analyze_sgl_column(double * data, long n, long nc) 
{
	SglColumnStats res;
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
	GpPair * tmp = (GpPair *)SAlloc::M(n*sizeof(GpPair));
	if(nc > 0) {
		res.sx = nc;
		res.sy = n / nc;
	}
	else {
		res.sx = 0;
		res.sy = n;
	}
	// Mean and centre of gravity 
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
	// Standard deviation, mean absolute deviation, skewness, and kurtosis 
	for(i = 0; i<n; i++) {
		double t = data[i] - res.mean;
		ad += fabs(t);
		d  += t;
		d2 += t*t;
		d3 += t*t*t;
		d4 += t*t*t*t;
	}
	// population (not sample) variance, stddev, skew, kurtosis 
	var = (d2 - d * d / n) / n;
	res.stddev = sqrt(var);
	res.adev = ad / n;
	if(var != 0.0) {
		res.skewness = d3 / (n * var * res.stddev);
		res.kurtosis = d4 / (n * var * var);
	}
	else {
		res.skewness = res.kurtosis = fgetnan();
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
	qsort(tmp, n, sizeof(GpPair), comparator);
	res.min = tmp[0];
	res.max = tmp[n-1];
	/*
	 * This uses the same quartile definitions as the boxplot code in graphics.c
	 */
	if((n & 0x1) == 0)
		res.median = 0.5 * (tmp[n/2-1].val + tmp[n/2].val);
	else
		res.median = tmp[(n-1)/2].val;
	if((n & 0x3) == 0)
		res.lower_quartile = 0.5 * (tmp[n/4-1].val + tmp[n/4].val);
	else
		res.lower_quartile = tmp[(n+3)/4-1].val;
	if((n & 0x3) == 0)
		res.upper_quartile = 0.5 * (tmp[n - n/4].val + tmp[n - n/4-1].val);
	else
		res.upper_quartile = tmp[n - (n+3)/4].val;
	// Note: the centre of gravity makes sense for positive value matrices only 
	if(cx == 0.0 && cy == 0.0) {
		res.cog_x = 0.0;
		res.cog_y = 0.0;
	}
	else {
		res.cog_x = cx / s;
		res.cog_y = cy / s;
	}
	SAlloc::F(tmp);
	return res;
}

static TwoColumnStats analyze_two_columns(double * x, double * y, SglColumnStats res_x, SglColumnStats res_y, long n)
{
	TwoColumnStats res;
	long i;
	double s = 0;
	double ssyy, ssxx, ssxy;
	for(i = 0; i<n; i++) {
		s += x[i] * y[i];
	}
	res.sum_xy = s;
	// Definitions according to
	// http://mathworld.wolfram.com/LeastSquaresFitting.html
	ssyy = res_y.sum_sq - res_y.sum * res_y.sum / n;
	ssxx = res_x.sum_sq - res_x.sum * res_x.sum / n;
	ssxy = res.sum_xy   - res_x.sum * res_y.sum / n;
	if(ssxx != 0.0)
		res.slope = ssxy / ssxx;
	else
		res.slope = fgetnan();
	res.intercept = res_y.mean - res.slope * res_x.mean;
	if(res_y.stddev != 0.0)
		res.correlation = res.slope * res_x.stddev / res_y.stddev;
	else
		res.correlation = fgetnan();
	if(n > 2) {
		double ss = (ssyy - res.slope * ssxy) / (n - 2);
		if(ssxx != 0.0) {
			res.slope_err = sqrt(ss / ssxx);
			res.intercept_err = sqrt(ss * (1./n + res_x.sum * res_x.sum / (n * n * ssxx)));
		}
		else {
			res.slope_err = res.intercept_err = fgetnan();
		}
	}
	else if(n == 2) {
		fprintf(stderr, "Warning:  Errors of slope and intercept are zero. There are as many data points as there are parameters.\n");
		res.slope_err = res.intercept_err = 0.0;
	}
	else {
		fprintf(stderr, "Warning:  Can't compute errors of slope and intercept. Not enough data points.\n");
		res.slope_err = res.intercept_err = fgetnan();
	}
	res.pos_min_y = x[res_y.min.index];
	res.pos_max_y = x[res_y.max.index];
	return res;
}

/* =================================================================
   Output
   ================================================================= */

/* Output */
/* Note: print_out is a FILE ptr, set by the "set print" command */

//static void ensure_output()
void GnuPlot::EnsureOutput()
{
	SETIFZ(Pgm.print_out, stderr);
}

static char* fmt(char * buf, double val)
{
	if(isnan(val) )
		sprintf(buf, "%11s", "undefined");
	else if(fabs(val) < 1e-14)
		sprintf(buf, "%11.4f", 0.0);
	else if(fabs(log10(fabs(val))) < 6)
		sprintf(buf, "%11.4f", val);
	else
		sprintf(buf, "%11.5e", val);
	return buf;
}

//static void file_output(GpFileStats s)
void GnuPlot::FileOutput(GpFileStats s)
{
	int width = 3;
	// Assuming that records is the largest number of the four... 
	if(s.records > 0)
		width = 1 + (int)( log10((double)s.records) );
	EnsureOutput();
	// Non-formatted to disk 
	if(Pgm.print_out != stdout && Pgm.print_out != stderr) {
		fprintf(Pgm.print_out, "%s\t%ld\n", "records", s.records);
		fprintf(Pgm.print_out, "%s\t%ld\n", "invalid", s.invalid);
		fprintf(Pgm.print_out, "%s\t%ld\n", "blanks", s.blanks);
		fprintf(Pgm.print_out, "%s\t%ld\n", "blocks", s.blocks);
		fprintf(Pgm.print_out, "%s\t%ld\n", "headers", s.header_records);
		fprintf(Pgm.print_out, "%s\t%ld\n", "outofrange", s.outofrange);
		return;
	}
	// Formatted to screen 
	fprintf(Pgm.print_out, "\n");
	fprintf(Pgm.print_out, "* FILE: \n");
	fprintf(Pgm.print_out, "  Records:           %*ld\n", width, s.records);
	fprintf(Pgm.print_out, "  Out of range:      %*ld\n", width, s.outofrange);
	fprintf(Pgm.print_out, "  Invalid:           %*ld\n", width, s.invalid);
	fprintf(Pgm.print_out, "  Header records:    %*ld\n", width, s.header_records);
	fprintf(Pgm.print_out, "  Blank:             %*ld\n", width, s.blanks);
	fprintf(Pgm.print_out, "  Data Blocks:       %*ld\n", width, s.blocks);
}

//static void sgl_column_output_nonformat(struct SglColumnStats s, char * x)
void GnuPlot::SglColumnOutputNonFormat(SglColumnStats s, char * x)
{
	fprintf(Pgm.print_out, "%s%s\t%f\n", "mean",     x, s.mean);
	fprintf(Pgm.print_out, "%s%s\t%f\n", "stddev",   x, s.stddev);
	fprintf(Pgm.print_out, "%s%s\t%f\n", "ssd",      x, s.ssd);
	fprintf(Pgm.print_out, "%s%s\t%f\n", "skewness", x, s.skewness);
	fprintf(Pgm.print_out, "%s%s\t%f\n", "kurtosis", x, s.kurtosis);
	fprintf(Pgm.print_out, "%s%s\t%f\n", "adev",     x, s.adev);
	fprintf(Pgm.print_out, "%s%s\t%f\n", "sum",      x, s.sum);
	fprintf(Pgm.print_out, "%s%s\t%f\n", "sum_sq",   x, s.sum_sq);
	fprintf(Pgm.print_out, "%s%s\t%f\n", "mean_err",     x, s.mean_err);
	fprintf(Pgm.print_out, "%s%s\t%f\n", "stddev_err",   x, s.stddev_err);
	fprintf(Pgm.print_out, "%s%s\t%f\n", "skewness_err", x, s.skewness_err);
	fprintf(Pgm.print_out, "%s%s\t%f\n", "kurtosis_err", x, s.kurtosis_err);
	fprintf(Pgm.print_out, "%s%s\t%f\n", "min",     x, s.min.val);
	if(s.sx == 0) {
		fprintf(Pgm.print_out, "%s%s\t%f\n", "lo_quartile", x, s.lower_quartile);
		fprintf(Pgm.print_out, "%s%s\t%f\n", "median",      x, s.median);
		fprintf(Pgm.print_out, "%s%s\t%f\n", "up_quartile", x, s.upper_quartile);
	}
	fprintf(Pgm.print_out, "%s%s\t%f\n", "max",     x, s.max.val);
	// If data set is matrix 
	if(s.sx > 0) {
		fprintf(Pgm.print_out, "%s%s\t%ld\n", "index_min_x",  x, (s.min.index) % s.sx);
		fprintf(Pgm.print_out, "%s%s\t%ld\n", "index_min_y",  x, (s.min.index) / s.sx);
		fprintf(Pgm.print_out, "%s%s\t%ld\n", "index_max_x",  x, (s.max.index) % s.sx);
		fprintf(Pgm.print_out, "%s%s\t%ld\n", "index_max_y",  x, (s.max.index) / s.sx);
		fprintf(Pgm.print_out, "%s%s\t%f\n", "cog_x",  x, s.cog_x);
		fprintf(Pgm.print_out, "%s%s\t%f\n", "cog_y",  x, s.cog_y);
	}
	else {
		fprintf(Pgm.print_out, "%s%s\t%ld\n", "min_index",  x, s.min.index);
		fprintf(Pgm.print_out, "%s%s\t%ld\n", "max_index",  x, s.max.index);
	}
}

//static void sgl_column_output(struct SglColumnStats s, long n)
void GnuPlot::SglColumnOoutput(SglColumnStats s, long n)
{
	int width = 1;
	char buf[32];
	char buf2[32];
	if(n > 0)
		width = 1 + (int)( log10( (double)n) );
	EnsureOutput();
	// Non-formatted to disk 
	if(Pgm.print_out != stdout && Pgm.print_out != stderr) {
		SglColumnOutputNonFormat(s, "_y");
		return;
	}
	// Formatted to screen 
	fprintf(Pgm.print_out, "\n");
	// First, we check whether the data file was a matrix 
	if(s.sx > 0)
		fprintf(Pgm.print_out, "* MATRIX: [%d X %d] \n", s.sx, s.sy);
	else
		fprintf(Pgm.print_out, "* COLUMN: \n");
	fprintf(Pgm.print_out, "  Mean:          %s\n", fmt(buf, s.mean) );
	fprintf(Pgm.print_out, "  Std Dev:       %s\n", fmt(buf, s.stddev) );
	fprintf(Pgm.print_out, "  Sample StdDev: %s\n", fmt(buf, s.ssd) );
	fprintf(Pgm.print_out, "  Skewness:      %s\n", fmt(buf, s.skewness) );
	fprintf(Pgm.print_out, "  Kurtosis:      %s\n", fmt(buf, s.kurtosis) );
	fprintf(Pgm.print_out, "  Avg Dev:       %s\n", fmt(buf, s.adev) );
	fprintf(Pgm.print_out, "  Sum:           %s\n", fmt(buf, s.sum) );
	fprintf(Pgm.print_out, "  Sum Sq.:       %s\n", fmt(buf, s.sum_sq) );
	fprintf(Pgm.print_out, "\n");

	fprintf(Pgm.print_out, "  Mean Err.:     %s\n", fmt(buf, s.mean_err) );
	fprintf(Pgm.print_out, "  Std Dev Err.:  %s\n", fmt(buf, s.stddev_err) );
	fprintf(Pgm.print_out, "  Skewness Err.: %s\n", fmt(buf, s.skewness_err) );
	fprintf(Pgm.print_out, "  Kurtosis Err.: %s\n", fmt(buf, s.kurtosis_err) );
	fprintf(Pgm.print_out, "\n");
	// For matrices, the quartiles and the median do not make too much sense 
	if(s.sx > 0) {
		fprintf(Pgm.print_out, "  Minimum:       %s [%*ld %ld ]\n", fmt(buf, s.min.val), width, (s.min.index) % s.sx, (s.min.index) / s.sx);
		fprintf(Pgm.print_out, "  Maximum:       %s [%*ld %ld ]\n", fmt(buf, s.max.val), width, (s.max.index) % s.sx, (s.max.index) / s.sx);
		fprintf(Pgm.print_out, "  COG:           %s %s\n", fmt(buf, s.cog_x), fmt(buf2, s.cog_y) );
	}
	else {
		// FIXME:  The "position" are randomly selected from a non-unique set. Bad! 
		fprintf(Pgm.print_out, "  Minimum:       %s [%*ld]\n", fmt(buf, s.min.val), width, s.min.index);
		fprintf(Pgm.print_out, "  Maximum:       %s [%*ld]\n", fmt(buf, s.max.val), width, s.max.index);
		fprintf(Pgm.print_out, "  Quartile:      %s \n", fmt(buf, s.lower_quartile) );
		fprintf(Pgm.print_out, "  Median:        %s \n", fmt(buf, s.median) );
		fprintf(Pgm.print_out, "  Quartile:      %s \n", fmt(buf, s.upper_quartile) );
		fprintf(Pgm.print_out, "\n");
	}
}

//static void two_column_output(SglColumnStats x, SglColumnStats y, TwoColumnStats xy, long n)
void GnuPlot::TwoColumnOutput(SglColumnStats x, SglColumnStats y, TwoColumnStats xy, long n)
{
	int width = 1;
	char bfx[32];
	char bfy[32];
	char blank[32];
	if(n > 0)
		width = 1 + (int)log10((double)n);
	// Non-formatted to disk 
	if(Pgm.print_out != stdout && Pgm.print_out != stderr) {
		SglColumnOutputNonFormat(x, "_x");
		SglColumnOutputNonFormat(y, "_y");
		fprintf(Pgm.print_out, "%s\t%f\n", "slope", xy.slope);
		if(n > 2)
			fprintf(Pgm.print_out, "%s\t%f\n", "slope_err", xy.slope_err);
		fprintf(Pgm.print_out, "%s\t%f\n", "intercept", xy.intercept);
		if(n > 2)
			fprintf(Pgm.print_out, "%s\t%f\n", "intercept_err", xy.intercept_err);
		fprintf(Pgm.print_out, "%s\t%f\n", "correlation", xy.correlation);
		fprintf(Pgm.print_out, "%s\t%f\n", "sumxy", xy.sum_xy);
		return;
	}
	// Create a string of blanks of the required length 
	strncpy(blank, "                 ", width+4);
	blank[width+4] = '\0';
	EnsureOutput();

	fprintf(Pgm.print_out, "\n");
	fprintf(Pgm.print_out, "* COLUMNS:\n");
	fprintf(Pgm.print_out, "  Mean:          %s %s %s\n", fmt(bfx, x.mean),   blank, fmt(bfy, y.mean) );
	fprintf(Pgm.print_out, "  Std Dev:       %s %s %s\n", fmt(bfx, x.stddev), blank, fmt(bfy, y.stddev) );
	fprintf(Pgm.print_out, "  Sample StdDev: %s %s %s\n", fmt(bfx, x.ssd), blank, fmt(bfy, y.ssd) );
	fprintf(Pgm.print_out, "  Skewness:      %s %s %s\n", fmt(bfx, x.skewness), blank, fmt(bfy, y.skewness) );
	fprintf(Pgm.print_out, "  Kurtosis:      %s %s %s\n", fmt(bfx, x.kurtosis), blank, fmt(bfy, y.kurtosis) );
	fprintf(Pgm.print_out, "  Avg Dev:       %s %s %s\n", fmt(bfx, x.adev), blank, fmt(bfy, y.adev) );
	fprintf(Pgm.print_out, "  Sum:           %s %s %s\n", fmt(bfx, x.sum),  blank, fmt(bfy, y.sum) );
	fprintf(Pgm.print_out, "  Sum Sq.:       %s %s %s\n", fmt(bfx, x.sum_sq), blank, fmt(bfy, y.sum_sq) );
	fprintf(Pgm.print_out, "\n");

	fprintf(Pgm.print_out, "  Mean Err.:     %s %s %s\n", fmt(bfx, x.mean_err),   blank, fmt(bfy, y.mean_err) );
	fprintf(Pgm.print_out, "  Std Dev Err.:  %s %s %s\n", fmt(bfx, x.stddev_err), blank, fmt(bfy, y.stddev_err) );
	fprintf(Pgm.print_out, "  Skewness Err.: %s %s %s\n", fmt(bfx, x.skewness_err), blank, fmt(bfy, y.skewness_err) );
	fprintf(Pgm.print_out, "  Kurtosis Err.: %s %s %s\n", fmt(bfx, x.kurtosis_err), blank, fmt(bfy, y.kurtosis_err) );
	fprintf(Pgm.print_out, "\n");

	// FIXME:  The "positions" are randomly selected from a non-unique set.  Bad! 
	fprintf(Pgm.print_out, "  Minimum:       %s [%*ld]   %s [%*ld]\n", fmt(bfx, x.min.val), width, x.min.index, fmt(bfy, y.min.val), width, y.min.index);
	fprintf(Pgm.print_out, "  Maximum:       %s [%*ld]   %s [%*ld]\n", fmt(bfx, x.max.val), width, x.max.index, fmt(bfy, y.max.val), width, y.max.index);
	fprintf(Pgm.print_out, "  Quartile:      %s %s %s\n", fmt(bfx, x.lower_quartile), blank, fmt(bfy, y.lower_quartile) );
	fprintf(Pgm.print_out, "  Median:        %s %s %s\n", fmt(bfx, x.median), blank, fmt(bfy, y.median) );
	fprintf(Pgm.print_out, "  Quartile:      %s %s %s\n", fmt(bfx, x.upper_quartile), blank, fmt(bfy, y.upper_quartile) );
	fprintf(Pgm.print_out, "\n");

	// Simpler below - don't care about alignment 
	if(xy.intercept < 0.0)
		fprintf(Pgm.print_out, "  Linear Model:       y = %.4g x - %.4g\n", xy.slope, -xy.intercept);
	else
		fprintf(Pgm.print_out, "  Linear Model:       y = %.4g x + %.4g\n", xy.slope, xy.intercept);

	fprintf(Pgm.print_out, "  Slope:              %.4g +- %.4g\n", xy.slope, xy.slope_err);
	fprintf(Pgm.print_out, "  Intercept:          %.4g +- %.4g\n", xy.intercept, xy.intercept_err);

	fprintf(Pgm.print_out, "  Correlation:        r = %.4g\n", xy.correlation);
	fprintf(Pgm.print_out, "  Sum xy:             %.4g\n", xy.sum_xy);
	fprintf(Pgm.print_out, "\n");
}

/* =================================================================
   Variable Handling
   ================================================================= */

//static void clear_one_var(const char * pPrefix, const char * pBase)
void GnuPlot::ClearOneVar(const char * pPrefix, const char * pBase)
{
	int len = strlen(pPrefix) + strlen(pBase) + 2;
	char * varname = (char *)SAlloc::M(len);
	sprintf(varname, "%s_%s", pPrefix, pBase);
	Ev.DelUdvByName(varname, TRUE);
	SAlloc::F(varname);
}

//static void clear_stats_variables(const char * pPrefix)
void GnuPlot::ClearStatsVariables(const char * pPrefix)
{
	// file variables 
	ClearOneVar(pPrefix, "records");
	ClearOneVar(pPrefix, "invalid");
	ClearOneVar(pPrefix, "headers");
	ClearOneVar(pPrefix, "blank");
	ClearOneVar(pPrefix, "blocks");
	ClearOneVar(pPrefix, "outofrange");
	ClearOneVar(pPrefix, "columns");

	/* one column variables */
	ClearOneVar(pPrefix, "mean");
	ClearOneVar(pPrefix, "stddev");
	ClearOneVar(pPrefix, "ssd");
	ClearOneVar(pPrefix, "skewness");
	ClearOneVar(pPrefix, "kurtosis");
	ClearOneVar(pPrefix, "adev");
	ClearOneVar(pPrefix, "mean_err");
	ClearOneVar(pPrefix, "stddev_err");
	ClearOneVar(pPrefix, "skewness_err");
	ClearOneVar(pPrefix, "kurtosis_err");
	ClearOneVar(pPrefix, "sum");
	ClearOneVar(pPrefix, "sumsq");
	ClearOneVar(pPrefix, "min");
	ClearOneVar(pPrefix, "max");
	ClearOneVar(pPrefix, "median");
	ClearOneVar(pPrefix, "lo_quartile");
	ClearOneVar(pPrefix, "up_quartile");
	ClearOneVar(pPrefix, "index_min");
	ClearOneVar(pPrefix, "index_max");
	// matrix variables 
	ClearOneVar(pPrefix, "index_min_x");
	ClearOneVar(pPrefix, "index_min_y");
	ClearOneVar(pPrefix, "index_max_x");
	ClearOneVar(pPrefix, "index_max_y");
	ClearOneVar(pPrefix, "size_x");
	ClearOneVar(pPrefix, "size_y");
	// two column variables 
	ClearOneVar(pPrefix, "slope");
	ClearOneVar(pPrefix, "intercept");
	ClearOneVar(pPrefix, "slope_err");
	ClearOneVar(pPrefix, "intercept_err");
	ClearOneVar(pPrefix, "correlation");
	ClearOneVar(pPrefix, "sumxy");
	ClearOneVar(pPrefix, "pos_min_y");
	ClearOneVar(pPrefix, "pos_max_y");
	// column headers 
	ClearOneVar(pPrefix, "column_header");
}

//static void create_and_set_var(double val, const char * pPrefix, const char * pBase, const char * pSuffix)
void GnuPlot::CreateAndSetVar(double val, const char * pPrefix, const char * pBase, const char * pSuffix)
{
	GpValue data;
	Gcomplex(&data, val, 0.0); /* data is complex, real=val, imag=0.0 */
	CreateAndStoreVar(&data, pPrefix, pBase, pSuffix);
}

//static void create_and_set_int_var(int ival, const char * pPrefix, const char * pBase, const char * pSuffix)
void GnuPlot::CreateAndSetIntVar(int ival, const char * pPrefix, const char * pBase, const char * pSuffix)
{
	GpValue data;
	Ginteger(&data, ival);
	CreateAndStoreVar(&data, pPrefix, pBase, pSuffix);
}

//static void create_and_store_var(const GpValue * pData, const char * pPrefix, const char * pBase, const char * pSuffix)
void GnuPlot::CreateAndStoreVar(const GpValue * pData, const char * pPrefix, const char * pBase, const char * pSuffix)
{
	// In case prefix (or suffix) is NULL - make them empty strings 
	SETIFZ(pPrefix, "");
	SETIFZ(pSuffix, "");
	int len = strlen(pPrefix) + strlen(pBase) + strlen(pSuffix) + 1;
	char * varname = (char *)SAlloc::M(len);
	sprintf(varname, "%s%s%s", pPrefix, pBase, pSuffix);
	// Note that add_udv_by_name() checks if the name already exists, and
	// returns the existing ptr if found. It also allocates memory for
	// its own copy of the varname.
	udvt_entry * udv_ptr = Ev.AddUdvByName(varname);
	udv_ptr->udv_value = *pData;
	SAlloc::F(varname);
}

//static void file_variables(struct file_stats s, const char * pPrefix)
void GnuPlot::FileVariables(GpFileStats s, const char * pPrefix)
{
	// Suffix does not make sense here! 
	CreateAndSetIntVar(s.records, pPrefix, "records", "");
	CreateAndSetIntVar(s.invalid, pPrefix, "invalid", "");
	CreateAndSetIntVar(s.header_records, pPrefix, "headers", "");
	CreateAndSetIntVar(s.blanks,  pPrefix, "blank",   "");
	CreateAndSetIntVar(s.blocks,  pPrefix, "blocks",  "");
	CreateAndSetIntVar(s.outofrange, pPrefix, "outofrange", "");
	CreateAndSetIntVar(s.columns, pPrefix, "columns", "");
	// copy column headers to an array 
	if(_Df.df_columnheaders) {
		GpValue headers;
		GpValue * A = (GpValue *)SAlloc::M((s.columns+1) * sizeof(GpValue));
		A[0].v.int_val = s.columns;
		for(int i = 1; i <= s.columns; i++)
			Gstring(&A[i], sstrdup(DfRetrieveColumnHead(i)));
		headers.Type = ARRAY;
		headers.v.value_array = A;
		CreateAndStoreVar(&headers, pPrefix, "column_header", "");
	}
}

//static void sgl_column_variables(SglColumnStats s, const char * prefix, const char * suffix)
void GnuPlot::SglColumnVariables(SglColumnStats s, const char * pPrefix, const char * pSuffix)
{
	CreateAndSetVar(s.mean,     pPrefix, "mean",     pSuffix);
	CreateAndSetVar(s.stddev,   pPrefix, "stddev",   pSuffix);
	CreateAndSetVar(s.ssd,      pPrefix, "ssd",      pSuffix);
	CreateAndSetVar(s.skewness, pPrefix, "skewness", pSuffix);
	CreateAndSetVar(s.kurtosis, pPrefix, "kurtosis", pSuffix);
	CreateAndSetVar(s.adev,     pPrefix, "adev",     pSuffix);

	CreateAndSetVar(s.mean_err,     pPrefix, "mean_err",     pSuffix);
	CreateAndSetVar(s.stddev_err,   pPrefix, "stddev_err",   pSuffix);
	CreateAndSetVar(s.skewness_err, pPrefix, "skewness_err", pSuffix);
	CreateAndSetVar(s.kurtosis_err, pPrefix, "kurtosis_err", pSuffix);
	CreateAndSetVar(s.sum,    pPrefix, "sum",   pSuffix);
	CreateAndSetVar(s.sum_sq, pPrefix, "sumsq", pSuffix);
	CreateAndSetVar(s.min.val, pPrefix, "min", pSuffix);
	CreateAndSetVar(s.max.val, pPrefix, "max", pSuffix);
	// If data set is matrix 
	if(s.sx > 0) {
		CreateAndSetIntVar( (s.min.index) % s.sx, pPrefix, "index_min_x", pSuffix);
		CreateAndSetIntVar( (s.min.index) / s.sx, pPrefix, "index_min_y", pSuffix);
		CreateAndSetIntVar( (s.max.index) % s.sx, pPrefix, "index_max_x", pSuffix);
		CreateAndSetIntVar( (s.max.index) / s.sx, pPrefix, "index_max_y", pSuffix);
		CreateAndSetIntVar(s.sx, pPrefix, "size_x", pSuffix);
		CreateAndSetIntVar(s.sy, pPrefix, "size_y", pSuffix);
	}
	else {
		CreateAndSetVar(s.median,         pPrefix, "median",      pSuffix);
		CreateAndSetVar(s.lower_quartile, pPrefix, "lo_quartile", pSuffix);
		CreateAndSetVar(s.upper_quartile, pPrefix, "up_quartile", pSuffix);
		CreateAndSetIntVar(s.min.index, pPrefix, "index_min", pSuffix);
		CreateAndSetIntVar(s.max.index, pPrefix, "index_max", pSuffix);
	}
}

//static void two_column_variables(TwoColumnStats s, const char * prefix, long n)
void GnuPlot::TwoColumnVariables(TwoColumnStats s, const char * pPrefix, long n)
{
	// Suffix does not make sense here! 
	CreateAndSetVar(s.slope,         pPrefix, "slope",         "");
	CreateAndSetVar(s.intercept,     pPrefix, "intercept",     "");
	// The errors can only calculated for n > 2, but we set them (to zero) anyway. 
	CreateAndSetVar(s.slope_err,     pPrefix, "slope_err",     "");
	CreateAndSetVar(s.intercept_err, pPrefix, "intercept_err", "");
	CreateAndSetVar(s.correlation,   pPrefix, "correlation",   "");
	CreateAndSetVar(s.sum_xy,        pPrefix, "sumxy",         "");
	CreateAndSetVar(s.pos_min_y,     pPrefix, "pos_min_y",     "");
	CreateAndSetVar(s.pos_max_y,     pPrefix, "pos_max_y",     "");
}

/* =================================================================
   Range Handling
   ================================================================= */
// 
// We validate our data here: discard everything that is outside
// the specified range. However, we have to be a bit careful here,
// because if no range is specified, we keep everything
// 
//static bool validate_data(double v, AXIS_INDEX ax)
bool GpAxisSet::ValidateData(double v, AXIS_INDEX ax) const
{
	// These are flag bits, not constants!!! 
	if((AxArray[ax].autoscale & AUTOSCALE_BOTH) == AUTOSCALE_BOTH) 
		return true;
	else if(((AxArray[ax].autoscale & AUTOSCALE_BOTH) == AUTOSCALE_MIN) && (v <= AxArray[ax].max))
		return true;
	else if(((AxArray[ax].autoscale & AUTOSCALE_BOTH) == AUTOSCALE_MAX) && (v >= AxArray[ax].min))
		return true;
	else if(((AxArray[ax].autoscale & AUTOSCALE_BOTH) == AUTOSCALE_NONE) && ((v <= AxArray[ax].max) && (v >= AxArray[ax].min)))
		return true;
	else
		return false;
}

/* =================================================================
   Parse Command Line and Process
   ================================================================= */

//void statsrequest(void)
void GnuPlot::StatsRequest()
{
	int    i;
	int    columns;
	double v[2];
	static char * file_name = NULL;
	char * temp_name;
	// Vars to hold data and results 
	long n; // number of records retained 
	long max_n;
	static double * data_x = NULL;
	static double * data_y = NULL; /* values read from file */
	long invalid;      /* number of missing/invalid records */
	long blanks;       /* number of blank lines */
	long doubleblanks; /* number of repeated blank lines */
	long header_records; /* number of records treated as headers rather than data */
	long out_of_range; /* number pts rejected, because out of range */
	GpFileStats res_file;
	SglColumnStats res_x = {0}, res_y = {0};
	TwoColumnStats res_xy = {0};
	// Vars for variable handling 
	static char * prefix = NULL; // prefix for user-defined vars names 
	bool prefix_from_columnhead = FALSE;
	// Vars that control output 
	bool do_output = TRUE;  // Generate formatted output 
	bool array_data = FALSE;
	Pgm.Shift();
	// Parse ranges 
	AxS[FIRST_X_AXIS].Init(FALSE);
	AxS[FIRST_Y_AXIS].Init(FALSE);
	ParseRange(FIRST_X_AXIS);
	ParseRange(FIRST_Y_AXIS);
	// Initialize 
	invalid = 0;      /* number of missing/invalid records */
	blanks = 0;       /* number of blank lines */
	header_records = 0; /* number of records treated as headers rather than data */
	doubleblanks = 0; /* number of repeated blank lines */
	out_of_range = 0; /* number pts rejected, because out of range */
	n = 0;            /* number of records retained */
	max_n = INITIAL_DATA_SIZE;
	SAlloc::F(data_x);
	SAlloc::F(data_y);
	data_x = vec(max_n); /* start with max. value */
	data_y = vec(max_n);
	SAlloc::F(prefix);
	prefix = NULL;
	if(!data_x || !data_y)
		IntError(NO_CARET, "Internal error: out of memory in stats");
	n = invalid = blanks = header_records = doubleblanks = out_of_range = 0;
	// Get filename 
	i = Pgm.GetCurTokenIdx();
	temp_name = StringOrExpress(NULL);
	if(temp_name) {
		SAlloc::F(file_name);
		file_name = sstrdup(temp_name);
	}
	else
		IntError(i, "missing filename or datablock");
	//
	// Jan 2015: We used to handle ascii matrix data as a special case but
	// the code did not work correctly.  Since df_read_matrix() dummies up
	// ascii matrix data to look as if had been presented as a binary blob,
	// we should be able to proceed with no special attention other than
	// to set the effective number of columns to 1.
	//
	if(TRUE) {
		DfSetPlotMode(MODE_PLOT);    /* Used for matrix datafiles */
		columns = DfOpen(file_name, 2, NULL); /* up to 2 using specs allowed */
		// 
		// "stats <badfilename> nooutput"
		// allows user to test for existance of a file without generating a fatal error.
		// The 'nooutput' keyword suppresses the resulting error message as well.
		//
		if(columns < 0) {
			Ev.FillGpValInteger("GPVAL_ERRNO", 1);
			while(!Pgm.EndOfCommand())
				if(Pgm.AlmostEquals(Pgm.CToken++, "noout$put"))
					do_output = FALSE;
			if(do_output)
				fprintf(stderr, "Cannot find or open file \"%s\"\n", file_name);
			goto stats_cleanup;
		}
		if(_Pb.df_array && columns == 0)
			array_data = TRUE;
		// For all these below: we could save the state, switch off, then restore 
		if(AxS[FIRST_X_AXIS].datatype == DT_TIMEDATE || AxS[FIRST_Y_AXIS].datatype == DT_TIMEDATE)
			IntError(NO_CARET, "Stats command not available in timedata mode");
		if(Gg.Polar)
			IntError(NO_CARET, "Stats command not available in polar mode");
		if(Gg.Parametric)
			IntError(NO_CARET, "Stats command not available in parametric mode");
		// Parse the remainder of the command line 
		while(!(Pgm.EndOfCommand()) ) {
			if(Pgm.AlmostEqualsCur("out$put") ) {
				do_output = TRUE;
				Pgm.Shift();
			}
			else if(Pgm.AlmostEqualsCur("noout$put") ) {
				do_output = FALSE;
				Pgm.Shift();
			}
			else if(Pgm.AlmostEqualsCur("pre$fix") || Pgm.EqualsCur("name")) {
				Pgm.Shift();
				if(Pgm.AlmostEqualsCur("col$umnheader")) {
					DfSetKeyTitleColumnHead(NULL);
					prefix_from_columnhead = TRUE;
					continue;
				}
				prefix = TryToGetString();
				if(!legal_identifier(prefix) || sstreq("GPVAL_", prefix))
					IntError(--Pgm.CToken, "illegal prefix");
			}
			else {
				IntErrorCurToken("Unrecognized stats option");
			}
		}
		// Clear any previous variables STATS_* so that if we exit early 
		// they cannot be mistaken as resulting from the current analysis. 
		ClearStatsVariables(prefix ? prefix : "STATS");
		// Special case for voxel grid stats: "stats $vgrid {name <prefix>} 
		if(_Df.df_voxelgrid) {
			VGrid * vgrid = GetVGridByName(file_name)->udv_value.v.vgrid;
			int N, nonzero;
			vgrid_stats(vgrid);
			N = vgrid->size;
			nonzero = N*N*N - vgrid->nzero;
			SETIFZ(prefix, sstrdup("STATS"));
			CreateAndSetVar(vgrid->mean_value, prefix, "_mean",   "");
			CreateAndSetVar(vgrid->stddev, prefix, "_stddev", "");
			CreateAndSetVar(vgrid->sum, prefix, "_sum", "");
			CreateAndSetVar(vgrid->min_value, prefix, "_min", "");
			CreateAndSetVar(vgrid->max_value, prefix, "_max", "");
			CreateAndSetVar(nonzero, prefix, "_nonzero", "");
			goto stats_cleanup;
		}
		// If the user has set an explicit locale for numeric input, apply it 
		// here so that it affects data fields read from the input file. 
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
		while((i = DfReadLine(v, 2)) != DF_EOF) {
			if(n >= max_n) {
				max_n = (max_n * 3) / 2; // increase max_n by factor of 1.5 
				// Some of the reallocations went bad: 
				if(!redim_vec(&data_x, max_n) || !redim_vec(&data_y, max_n)) {
					DfClose();
					IntError(NO_CARET, "Out of memory in stats: too many datapoints (%d)?", max_n);
				}
			} /* if (need to extend storage space) */
			// FIXME: ascii "matrix" input from df_readline via df_readbinary does not
			// flag NaN values so we must check each returned value here
			if(_Df.df_matrix && (i == 2) && isnan(v[1]))
				i = DF_UNDEFINED;
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
				case DF_COLUMN_HEADERS:
				    header_records += 1;
				    continue;
				case 0:
				    IntWarn(NO_CARET, "bad data on line %d of file %s", _Df.df_line_number, NZOR(_Df.df_filename, ""));
				    break;
				case 1: // Read single column successfully 
				    if(AxS.ValidateData(v[0], FIRST_Y_AXIS) ) {
					    data_y[n] = v[0];
					    n++;
				    }
				    else {
					    out_of_range++;
				    }
				    columns = 1;
				    break;
				case 2: // Read two columns successfully 
				    if(AxS.ValidateData(v[0], FIRST_X_AXIS) && AxS.ValidateData(v[1], FIRST_Y_AXIS) ) {
					    data_x[n] = v[0];
					    data_y[n] = v[1];
					    n++;
				    }
				    else {
					    out_of_range++;
				    }
				    columns = 2;
				    break;
				default: // Who are these? 
				    FPRINTF((stderr, "unhandled return code %d from df_readline\n", i));
				    break;
			}
		} // end-while : done reading file 
		DfClose();
		// now resize fields to actual length: 
		redim_vec(&data_x, n);
		redim_vec(&data_y, n);
	}
	// Now finished reading user input; return to C locale for internal use
	reset_numeric_locale();
	// No data! Try to explain why. 
	if(n == 0) {
		IntWarn(NO_CARET, (out_of_range > 0) ? "All points out of range" : "No valid data points found in file");
		goto stats_cleanup;
	}
	// The analysis variables are named STATS_* unless the user either 
	// gave a specific name or told us to use a columnheader.          
	if(!prefix && prefix_from_columnhead && !isempty(_Df.df_key_title)) {
		prefix = sstrdup(_Df.df_key_title);
		squash_spaces(prefix, 0);
		if(!legal_identifier(prefix)) {
			IntWarn(NO_CARET, "columnhead %s is not a valid prefix", prefix ? prefix : "");
			ZFREE(prefix);
		}
	}
	SETIFZ(prefix, sstrdup("STATS_"));
	i = strlen(prefix);
	if(prefix[i-1] != '_') {
		prefix = (char *)SAlloc::R(prefix, i+2);
		strcat(prefix, "_");
	}
	// Do the actual analysis 
	res_file = AnalyzeFile(n, out_of_range, invalid, blanks, doubleblanks, header_records);
	// Jan 2015: Revised detection and handling of matrix data 
	if(array_data)
		res_file.columns = columns = 1;
	if(_Df.df_matrix) {
		const int nc = _Df.df_bin_record[_Df.NumBinRecords-1].submatrix_ncols;
		res_y = analyze_sgl_column(data_y, n, nc);
		res_file.columns = nc;
		columns = 1;
	}
	else if(columns == 1) {
		res_y = analyze_sgl_column(data_y, n, 0);
	}
	else {
		// If there are two columns, then the data file is not a matrix 
		res_x  = analyze_sgl_column(data_x, n, 0);
		res_y  = analyze_sgl_column(data_y, n, 0);
		res_xy = analyze_two_columns(data_x, data_y, res_x, res_y, n);
	}
	// Store results in user-accessible variables 
	// Clear out any previous use of these variables 
	Ev.DelUdvByName(prefix, TRUE);
	FileVariables(res_file, prefix);
	if(columns == 1) {
		SglColumnVariables(res_y, prefix, "");
	}
	if(columns == 2) {
		SglColumnVariables(res_x, prefix, "_x");
		SglColumnVariables(res_y, prefix, "_y");
		TwoColumnVariables(res_xy, prefix, n);
	}
	// Output 
	if(do_output) {
		FileOutput(res_file);
		if(columns == 1)
			SglColumnOoutput(res_y, res_file.records);
		else
			TwoColumnOutput(res_x, res_y, res_xy, res_file.records);
	}
	// Cleanup 
stats_cleanup:
	ZFREE(data_x);
	ZFREE(data_y);
	ZFREE(file_name);
	ZFREE(prefix);
}

#endif /* The whole file is conditional on USE_STATS */
