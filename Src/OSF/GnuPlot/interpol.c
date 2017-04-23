/* GNUPLOT - interpol.c */

/*[
 * Copyright 1986 - 1993, 1998, 2004   Thomas Williams, Colin Kelley
 *
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
   ]*/

/*
 * C-Source file identification Header
 *
 * This file belongs to a project which is:
 *
 * done 1993 by MGR-Software, Asgard  (Lars Hanke)
 * written by Lars Hanke
 *
 * Contact me via:
 *
 *  InterNet: mgr@asgard.bo.open.de
 *      FIDO: Lars Hanke @ 2:243/4802.22   (as long as they keep addresses)
 *
 **************************************************************************
 *
 *   Project: gnuplot
 *    Module:
 *      File: interpol.c
 *
 *   Revisor: Lars Hanke
 *   Revised: 26/09/93
 *  Revision: 1.0
 *
 **************************************************************************
 *
 * LEGAL
 *  This module is part of gnuplot and distributed under whatever terms
 *  gnuplot is or will be published, unless exclusive rights are claimed.
 *
 * DESCRIPTION
 *  Supplies 2-D data interpolation and approximation routines
 *
 * IMPORTS
 *  plot.h
 *    - cp_extend()
 *    - structs: CurvePoints, double, GpCoordinate
 *
 *  setshow.h
 *    - samples, axis array[] variables
 *    - plottypes
 *
 *  proto.h
 *    - solve_tri_diag()
 *    - typedef tri_diag
 *
 * EXPORTS
 *  gen_interp()
 *  sort_points()
 *  cp_implode()
 *
 * BUGS and TODO
 *  I would really have liked to use Gershon Elbers contouring code for
 *  all the stuff done here, but I failed. So I used my own code.
 *  If somebody is able to consolidate Gershon's code for this purpose
 *  a lot of gnuplot users would be very happy - due to memory problems.
 *
 **************************************************************************
 *
 * HISTORY
 * Changes:
 *  Nov 24, 1995  Markus Schuh (M.Schuh@meteo.uni-koeln.de):
 *      changed the algorithm for csplines
 *      added algorithm for approximation csplines
 *      copied point storage and range fix from plot2d.c
 *
 *  Dec 12, 1995 David Denholm
 *      oops - at the time this is called, stored co-ords are
 *      internal (ie maybe log of data) but min/max are in
 *      user co-ordinates.
 *      Work with min and max of internal co-ords, and
 *      check at the end whether external min and max need to
 *      be increased. (since GpGg.Samples1 is typically 100 ; we
 *      dont want to take more logs than necessary)
 *      Also, need to take into account which axes are active
 *
 *  Jun 30, 1996 Jens Emmerich
 *      implemented handling of UNDEFINED points
 */
#include <gnuplot.h>
#pragma hdrstop

/* in order to support multiple axes, and to simplify ranging in
 * parametric plots, we use arrays to store some things. For 2d plots,
 * elements are z=0,y1=1,x1=2,z2=4,y2=5,x2=6 these are given symbolic
 * names in plot.h
 */

/*
 * IMHO, code is getting too cluttered with repeated chunks of
 * code. Some macros to simplify, I hope.
 */

/* store VALUE or log(VALUE) in STORE, set TYPE as appropriate Do
 * OUT_ACTION or UNDEF_ACTION as appropriate. Adjust range provided
 * type is INRANGE (ie dont adjust y if x is outrange). VALUE must not
 * be same as STORE */
/* FIXME 20010610: UNDEF_ACTION is completely unused ??? Furthermore,
 * this is so similar to STORE_WITH_LOG_AND_UPDATE_RANGE() from axis.h
 * that the two should probably be merged.  */
#define STORE_AND_FIXUP_RANGE(store, value, type, min, max, _auto, out_action, undef_action) \
	do {									\
		store = value;							      \
		if(type != INRANGE)						   \
			break;  /* don't set y range if x is outrange, for example */	\
		if((value) < (min)) {						   \
			if((_auto) & AUTOSCALE_MIN)					\
				(min) = (value);					     \
			else {								 \
				(type) = OUTRANGE;					     \
				out_action;						     \
				break;							     \
			}								 \
		}								    \
		if((value) > (max)) {						   \
			if((_auto) & AUTOSCALE_MAX)					\
				(max) = (value);					     \
			else {								 \
				(type) = OUTRANGE;					     \
				out_action;						     \
			}								 \
		}								    \
	} while(0)

typedef double five_diag[5];

static int next_curve(const CurvePoints * plot, int * curve_start);
static int num_curves(const CurvePoints * plot);
static double eval_kdensity(CurvePoints * cp, int first_point, int num_points, double x);
//static void do_kdensity(CurvePoints * cp, int first_point, int num_points, GpCoordinate * dest);
static double * cp_binomial(int points);
static void eval_bezier(CurvePoints * cp, int first_point, int num_points, double sr, double * px, double *py, double * c);
//static void do_bezier(CurvePoints * cp, double * bc, int first_point, int num_points, GpCoordinate * dest);
static int solve_five_diag(five_diag m[], double r[], double x[], int n);
static spline_coeff * cp_approx_spline(CurvePoints * plot, int first_point, int num_points);
//static spline_coeff * cp_tridiag(CurvePoints * plot, int first_point, int num_points);
//static void do_cubic(CurvePoints * plot, spline_coeff * sc, int first_point, int num_points, GpCoordinate * dest);
//static void do_freq(CurvePoints * plot, int first_point, int num_points);
static int compare_points(const void * p1, const void * p2);

/*
 * position curve_start to index the next non-UNDEFINDED point,
 * start search at initial curve_start,
 * return number of non-UNDEFINDED points from there on,
 * if no more valid points are found, curve_start is set
 * to plot->p_count and 0 is returned
 */

static int next_curve(const CurvePoints * plot, int * curve_start)
{
	int curve_length = 0;
	// Skip undefined points
	while(*curve_start < plot->p_count && plot->points[*curve_start].type == UNDEFINED) {
		(*curve_start)++;
	}
	// curve_length is first used as an offset, then the correct # points
	while((*curve_start) + curve_length < plot->p_count && plot->points[(*curve_start) + curve_length].type != UNDEFINED) {
		curve_length++;
	}
	return curve_length;
}
//
// determine the number of curves in plot->points, separated by UNDEFINED points
//
static int num_curves(const CurvePoints * plot)
{
	int num_points;
	int first_point = 0;
	int curves = 0;
	while((num_points = next_curve(plot, &first_point)) > 0) {
		curves++;
		first_point += num_points;
	}
	return (curves);
}

/* PKJ - May 2008
   kdensity (short for Kernel Density) builds histograms using
   "Kernel Density Estimation" using Gaussian Kernels.
   Check: L. Wassermann: "All of Statistics" for example.

   The implementation is based closely on the implementation for Bezier
   curves, except for the way the actual interpolation is generated.

   EAM Feb 2015 - Revise to handle logscaled y axis and to
   pass in an actual x GpCoordinate rather than a fraction of the min/max range.
   NB: This code does not deal with logscaled x axis.
   FIXME: It's silly to recalculate the mean/stddev/bandwidth every time.
 */
//
// eval_kdensity is a modification of eval_bezier
//
static double eval_kdensity(CurvePoints * cp,
    int first_point,    /* where to start in plot->points (to find x-range) */
    int num_points,     /* to determine end in plot->points */
    double x            /* x value at which to calculate y */
    )
{
	int i;
	GpCoordinate  * this_points = (cp->points) + first_point;
	double y, Z, ytmp;
	double bandwidth, default_bandwidth;
	double avg = 0.0;
	double sigma = 0.0;
	for(i = 0; i < num_points; i++) {
		avg   += this_points[i].x;
		sigma += this_points[i].x * this_points[i].x;
	}
	avg /= (double)num_points;
	sigma = sqrt(sigma/(double)num_points - avg*avg); /* Standard Deviation */

	/* This is the optimal bandwidth if the point distribution is Gaussian.
	   (Applied Smoothing Techniques for Data Analysis
	   by Adrian W, Bowman & Adelchi Azzalini (1997)) */
	/* If the supplied bandwidth is zero of less, the default bandwidth is used. */
	default_bandwidth = pow(4.0/(3.0*num_points), 1.0/5.0)*sigma;
	if(cp->smooth_parameter <= 0) {
		bandwidth = default_bandwidth;
		cp->smooth_parameter = -default_bandwidth;
	}
	else
		bandwidth = cp->smooth_parameter;
	y = 0;
	for(i = 0; i < num_points; i++) {
		Z = ( x - this_points[i].x )/bandwidth;
		ytmp = this_points[i].y;
		y += GpGg.DelogValue((AXIS_INDEX)cp->y_axis, ytmp) * exp(-0.5*Z*Z) / bandwidth;
	}
	y /= sqrt(2.0*M_PI);
	return y;
}

/* do_kdensity is based on do_bezier, except for the call to eval_bezier */
/* EAM Feb 2015: Don't touch xrange, but recalculate y limits  */
//static void do_kdensity(CurvePoints * cp, int first_point, int num_points, GpCoordinate * dest)
void GpGadgets::DoKDensity(CurvePoints * cp, int first_point, int num_points, GpCoordinate * dest)
{
	int i;
	double x, y;
	double /*sxmin, sxmax,*/ step;
	RealRange sx;
	XAxis = (AXIS_INDEX)cp->x_axis;
	YAxis = (AXIS_INDEX)cp->y_axis;
	if(GetX().Flags & GpAxis::fLog)
		GpGg.IntErrorNoCaret("kdensity cannot handle logscale x axis");
	sx = GetX().Range;
	step = sx.GetDistance() / (Samples1 - 1);
	for(i = 0; i < Samples1; i++) {
		x = sx.low + i * step;
		y = eval_kdensity(cp, first_point, num_points, x);
		// now we have to store the points and adjust the ranges
		dest[i].type = INRANGE;
		dest[i].x = x;
		STORE_WITH_LOG_AND_UPDATE_RANGE(dest[i].y, y, dest[i].type, YAxis, cp->noautoscale, NOOP, NOOP);
		dest[i].xlow = dest[i].xhigh = dest[i].x;
		dest[i].ylow = dest[i].yhigh = dest[i].y;
		dest[i].z = -1;
	}
}

/* HBB 990205: rewrote the 'bezier' interpolation routine,
 * to prevent numerical overflow and other undesirable things happening
 * for large data files (num_data about 1000 or so), where binomial
 * coefficients would explode, and powers of 'sr' (0 < sr < 1) become
 * extremely small. Method used: compute logarithms of these
 * extremely large and small numbers, and only go back to the
 * real numbers once they've cancelled out each other, leaving
 * a reasonable-sized one. */

/*
 * cp_binomial() computes the binomial coefficients needed for BEZIER stuff
 *   and stores them into an array which is hooked to sdat.
 * (MGR 1992)
 */
static double * cp_binomial(int points)
{
	int    k;
	int    e = points; // well we're going from k=0 to k=p_count-1
	double * coeff = (double *)malloc(e * sizeof(double));
	int    n = points - 1;
	e = n / 2;
	// HBB 990205: calculate these in 'logarithmic space', as they become _very_ large, with growing n (4^n)
	coeff[0] = 0.0;
	for(k = 0; k < e; k++) {
		coeff[k + 1] = coeff[k] + log(((double)(n - k)) / ((double)(k + 1)));
	}
	for(k = n; k >= e; k--)
		coeff[k] = coeff[n - k];
	return (coeff);
}

/* This is a subfunction of do_bezier() for BEZIER style computations.
 * It is passed the stepration (STEP/MAXSTEPS) and the addresses of
 * the double values holding the next x and y coordinates.
 * (MGR 1992)
 */

static void eval_bezier(CurvePoints * cp,
    int first_point,            /* where to start in plot->points (to find x-range) */
    int num_points,             /* to determine end in plot->points */
    double sr,                  /* position inside curve, range [0:1] */
    double * px,               /* OUTPUT: x and y */
    double * py,
    double * c)                  /* Bezier coefficient array */
{
	uint n = num_points - 1;
	GpCoordinate * this_points = (cp->points) + first_point;
	if(sr == 0.0) {
		*px = this_points[0].x;
		*py = this_points[0].y;
	}
	else if(sr == 1.0) {
		*px = this_points[n].x;
		*py = this_points[n].y;
	}
	else {
		/* HBB 990205: do calculation in 'logarithmic space',
		 * to avoid over/underflow errors, which would exactly cancel
		 * out each other, anyway, in an exact calculation
		 */
		double lx = 0.0, ly = 0.0;
		double log_dsr_to_the_n = n * log(1 - sr);
		double log_sr_over_dsr = log(sr) - log(1 - sr);
		for(uint i = 0; i <= n; i++) {
			double u = exp(c[i] + log_dsr_to_the_n + i * log_sr_over_dsr);
			lx += this_points[i].x * u;
			ly += this_points[i].y * u;
		}
		*px = lx;
		*py = ly;
	}
}

#define UPDATE_RANGE(TEST, OLD, NEW, AXIDX) do { if(TEST) (OLD) = GpGg.DelogValue(AXIDX, NEW); } while(0)
/* @construction
void GpGadgets::UpdateRange(bool condition, double & rOldValue, double newValue, int axIdx)
{
	if(condition) 
		rOldValue = DelogValue(axIdx, newValue);
}
*/
// 
// generate a new set of coordinates representing the bezier curve and
// set it to the plot
// ARGS:
// bc - Bezier coefficient array 
// first_point - where to start in plot->points
// num_points - to determine end in plot->points
// dest - where to put the interpolated data
//
//static void do_bezier(CurvePoints * cp, double * bc, int first_point, int num_points, GpCoordinate * dest)
void GpGadgets::DoBezier(CurvePoints * pCp, double * pBc, int firstPoint, int numPoints, GpCoordinate * pDest)
{
	int i;
	double x, y;
	/* min and max in internal (eg logged) co-ordinates. We update
	 * these, then update the external extrema in user co-ordinates
	 * at the end.
	 */

	double ixmin, ixmax, iymin, iymax;
	double sxmin, sxmax, symin, symax; /* starting values of above */
	XAxis = (AXIS_INDEX)pCp->x_axis;
	YAxis = (AXIS_INDEX)pCp->y_axis;
	ixmin = sxmin = LogValue(XAxis, GetX().Range.low);
	ixmax = sxmax = LogValue(XAxis, GetX().Range.upp);
	iymin = symin = LogValue(YAxis, GetY().Range.low);
	iymax = symax = LogValue(YAxis, GetY().Range.upp);
	for(i = 0; i < Samples1; i++) {
		eval_bezier(pCp, firstPoint, numPoints, (double)i / (double)(Samples1 - 1), &x, &y, pBc);
		// now we have to store the points and adjust the ranges
		pDest[i].type = INRANGE;
		STORE_AND_FIXUP_RANGE(pDest[i].x, x, pDest[i].type, ixmin, ixmax, GetX().AutoScale, NOOP, continue);
		STORE_AND_FIXUP_RANGE(pDest[i].y, y, pDest[i].type, iymin, iymax, GetY().AutoScale, NOOP, NOOP);
		pDest[i].xlow = pDest[i].xhigh = pDest[i].x;
		pDest[i].ylow = pDest[i].yhigh = pDest[i].y;
		pDest[i].z = -1;
	}
	UPDATE_RANGE(ixmax > sxmax, GetX().Range.upp, ixmax, XAxis);
	UPDATE_RANGE(ixmin < sxmin, GetX().Range.low, ixmin, XAxis);
	UPDATE_RANGE(iymax > symax, GetY().Range.upp, iymax, YAxis);
	UPDATE_RANGE(iymin < symin, GetY().Range.low, iymin, YAxis);
}

/*
 * call contouring routines -- main entry
 */

/*
 * it should be like this, but it doesn't run. If you find out why,
 * contact me: mgr@asgard.bo.open.de or Lars Hanke 2:243/4802.22@fidonet
 *
 * Well, all this had originally been inside contour.c, so maybe links
 * to functions and of contour.c are broken.
 * ***deleted***
 * end of unused entry point to Gershon's code
 *
 */

/*
 * Solve five diagonal linear system equation. The five diagonal matrix is
 * defined via matrix M, right side is r, and solution X i.e. M * X = R.
 * Size of system given in n. Return true if solution exist.
 *  G. Engeln-Muellges/ F.Reutter:
 *  "Formelsammlung zur Numerischen Mathematik mit Standard-FORTRAN-Programmen"
 *  ISBN 3-411-01677-9
 *
 * /  m02 m03 m04   0   0   0   0    .       .       . \   /  x0  \    / r0  \
 * I  m11 m12 m13 m14   0   0   0    .       .       . I   I  x1  I   I  r1  I
 * I  m20 m21 m22 m23 m24   0   0    .       .       . I * I  x2  I = I  r2  I
 * I    0 m30 m31 m32 m33 m34   0    .       .       . I   I  x3  I   I  r3  I
 *      .   .   .   .   .   .   .    .       .       .        .        .
 * \                           m(n-3)0 m(n-2)1 m(n-1)2 /   \x(n-1)/   \r(n-1)/
 *
 */
static int solve_five_diag(five_diag m[], double r[], double x[], int n)
{
	int i;
	five_diag * hv = (five_diag *)malloc((n + 1) * sizeof(five_diag));
	hv[0][0] = m[0][2];
	if(hv[0][0] == 0) {
		free(hv);
		return false;
	}
	hv[0][1] = m[0][3] / hv[0][0];
	hv[0][2] = m[0][4] / hv[0][0];

	hv[1][3] = m[1][1];
	hv[1][0] = m[1][2] - hv[1][3] * hv[0][1];
	if(hv[1][0] == 0) {
		free(hv);
		return false;
	}
	hv[1][1] = (m[1][3] - hv[1][3] * hv[0][2]) / hv[1][0];
	hv[1][2] = m[1][4] / hv[1][0];

	for(i = 2; i < n; i++) {
		hv[i][3] = m[i][1] - m[i][0] * hv[i - 2][1];
		hv[i][0] = m[i][2] - m[i][0] * hv[i - 2][2] - hv[i][3] * hv[i - 1][1];
		if(hv[i][0] == 0) {
			free(hv);
			return false;
		}
		hv[i][1] = (m[i][3] - hv[i][3] * hv[i - 1][2]) / hv[i][0];
		hv[i][2] = m[i][4] / hv[i][0];
	}

	hv[0][4] = 0;
	hv[1][4] = r[0] / hv[0][0];
	for(i = 1; i < n; i++) {
		hv[i + 1][4] = (r[i] - m[i][0] * hv[i - 1][4] - hv[i][3] * hv[i][4]) / hv[i][0];
	}

	x[n - 1] = hv[n][4];
	x[n - 2] = hv[n - 1][4] - hv[n - 2][1] * x[n - 1];
	for(i = n - 3; i >= 0; i--)
		x[i] = hv[i + 1][4] - hv[i][1] * x[i + 1] - hv[i][2] * x[i + 2];

	free(hv);
	return true;
}

/*
 * Calculation of approximation cubic splines
 * Input:  x[i], y[i], weights z[i]
 *
 * Returns matrix of spline coefficients
 */
static spline_coeff * cp_approx_spline(CurvePoints * plot,
    int first_point/* where to start in plot->points */, int num_points/* to determine end in plot->points */)
{
	spline_coeff * sc;
	five_diag * m;
	double * r, * x, * h, * xp, * yp;
	GpCoordinate  * this_points;
	int i;
	GpGg.XAxis = (AXIS_INDEX)plot->x_axis;
	GpGg.YAxis = (AXIS_INDEX)plot->y_axis;
	sc = (spline_coeff *)malloc((num_points) * sizeof(spline_coeff));
	if(num_points < 4)
		GpGg.IntError(plot->Token, "Can't calculate approximation splines, need at least 4 points");
	this_points = (plot->points) + first_point;
	for(i = 0; i < num_points; i++)
		if(this_points[i].z <= 0)
			GpGg.IntError(plot->Token, "Can't calculate approximation splines, all weights have to be > 0");
	m = (five_diag *)malloc((num_points - 2) * sizeof(five_diag));
	r = (double *)malloc((num_points - 2) * sizeof(double));
	x = (double *)malloc((num_points - 2) * sizeof(double));
	h = (double *)malloc((num_points - 1) * sizeof(double));
	xp = (double *)malloc((num_points) * sizeof(double));
	yp = (double *)malloc((num_points) * sizeof(double));
	// KB 981107: With logarithmic axis first convert back to linear scale 
	xp[0] = GpGg.DelogValue(GpGg.XAxis, this_points[0].x);
	yp[0] = GpGg.DelogValue(GpGg.YAxis, this_points[0].y);
	for(i = 1; i < num_points; i++) {
		xp[i] = GpGg.DelogValue(GpGg.XAxis, this_points[i].x);
		yp[i] = GpGg.DelogValue(GpGg.YAxis, this_points[i].y);
		h[i - 1] = xp[i] - xp[i - 1];
	}
	// set up the matrix and the vector 
	for(i = 0; i <= num_points - 3; i++) {
		r[i] = 3 * ((yp[i + 2] - yp[i + 1]) / h[i + 1] - (yp[i + 1] - yp[i]) / h[i]);
		if(i < 2)
			m[i][0] = 0;
		else
			m[i][0] = 6 / this_points[i].z / h[i - 1] / h[i];
		if(i < 1)
			m[i][1] = 0;
		else
			m[i][1] = h[i] - 6 / this_points[i].z / h[i] * (1 / h[i - 1] + 1 / h[i])
			    - 6 / this_points[i + 1].z / h[i] * (1 / h[i] + 1 / h[i + 1]);
		m[i][2] = 2 * (h[i] + h[i + 1])
		    + 6 / this_points[i].z / h[i] / h[i]
		    + 6 / this_points[i + 1].z * (1 / h[i] + 1 / h[i + 1]) * (1 / h[i] + 1 / h[i + 1])
		    + 6 / this_points[i + 2].z / h[i + 1] / h[i + 1];
		if(i > num_points - 4)
			m[i][3] = 0;
		else
			m[i][3] = h[i + 1] - 6 / this_points[i + 1].z / h[i + 1] * (1 / h[i] + 1 / h[i + 1])
			    - 6 / this_points[i + 2].z / h[i + 1] * (1 / h[i + 1] + 1 / h[i + 2]);
		if(i > num_points - 5)
			m[i][4] = 0;
		else
			m[i][4] = 6 / this_points[i + 2].z / h[i + 1] / h[i + 2];
	}
	// solve the matrix 
	if(!solve_five_diag(m, r, x, num_points - 2)) {
		free(h);
		free(x);
		free(r);
		free(m);
		free(xp);
		free(yp);
		GpGg.IntError(plot->Token, "Can't calculate approximation splines");
	}
	sc[0][2] = 0;
	for(i = 1; i <= num_points - 2; i++)
		sc[i][2] = x[i - 1];
	sc[num_points - 1][2] = 0;
	sc[0][0] = yp[0] + 2 / this_points[0].z / h[0] * (sc[0][2] - sc[1][2]);
	for(i = 1; i <= num_points - 2; i++)
		sc[i][0] = yp[i] - 2 / this_points[i].z * (sc[i - 1][2] / h[i - 1] - sc[i][2] * (1 / h[i - 1] + 1 / h[i]) + sc[i + 1][2] / h[i]);
	sc[num_points - 1][0] = yp[num_points - 1]
	    - 2 / this_points[num_points - 1].z / h[num_points - 2]
	    * (sc[num_points - 2][2] - sc[num_points - 1][2]);
	for(i = 0; i <= num_points - 2; i++) {
		sc[i][1] = (sc[i + 1][0] - sc[i][0]) / h[i] - h[i] / 3 * (sc[i + 1][2] + 2 * sc[i][2]);
		sc[i][3] = (sc[i + 1][2] - sc[i][2]) / 3 / h[i];
	}
	free(h);
	free(x);
	free(r);
	free(m);
	free(xp);
	free(yp);
	return (sc);
}

/*
 * Calculation of cubic splines
 *
 * This can be treated as a special case of approximation cubic splines, with
 * all weights -> infinity.
 *
 * Returns matrix of spline coefficients
 */
//static spline_coeff * cp_tridiag(CurvePoints * plot, int first_point, int num_points)
spline_coeff * GpGadgets::CpTriDiag(CurvePoints * pPlot, int firstPoint, int numPoints)
{
	spline_coeff * sc;
	tri_diag * m;
	double * r, * x, * h, * xp, * yp;
	GpCoordinate  * this_points;
	int i;
	XAxis = (AXIS_INDEX)pPlot->x_axis;
	YAxis = (AXIS_INDEX)pPlot->y_axis;
	if(numPoints < 3)
		GpGg.IntError(pPlot->Token, "Can't calculate splines, need at least 3 points");
	this_points = (pPlot->points) + firstPoint;
	sc = (spline_coeff *)malloc((numPoints) * sizeof(spline_coeff));
	m = (tri_diag *)malloc((numPoints - 2) * sizeof(tri_diag));
	r = (double *)malloc((numPoints - 2) * sizeof(double));
	x = (double *)malloc((numPoints - 2) * sizeof(double));
	h = (double *)malloc((numPoints - 1) * sizeof(double));
	xp = (double *)malloc((numPoints) * sizeof(double));
	yp = (double *)malloc((numPoints) * sizeof(double));
	// KB 981107: With logarithmic axis first convert back to linear scale
	xp[0] = DelogValue(XAxis, this_points[0].x);
	yp[0] = DelogValue(YAxis, this_points[0].y);
	for(i = 1; i < numPoints; i++) {
		xp[i] = DelogValue(XAxis, this_points[i].x);
		yp[i] = DelogValue(YAxis, this_points[i].y);
		h[i - 1] = xp[i] - xp[i - 1];
	}
	/* set up the matrix and the vector */
	for(i = 0; i <= numPoints - 3; i++) {
		r[i] = 3 * ((yp[i + 2] - yp[i + 1]) / h[i + 1] - (yp[i + 1] - yp[i]) / h[i]);
		if(i < 1)
			m[i][0] = 0;
		else
			m[i][0] = h[i];
		m[i][1] = 2 * (h[i] + h[i + 1]);
		if(i > numPoints - 4)
			m[i][2] = 0;
		else
			m[i][2] = h[i + 1];
	}

	/* solve the matrix */
	if(!solve_tri_diag(m, r, x, numPoints - 2)) {
		free(h);
		free(x);
		free(r);
		free(m);
		free(xp);
		free(yp);
		GpGg.IntError(pPlot->Token, "Can't calculate cubic splines");
	}
	sc[0][2] = 0;
	for(i = 1; i <= numPoints - 2; i++)
		sc[i][2] = x[i - 1];
	sc[numPoints - 1][2] = 0;
	for(i = 0; i <= numPoints - 1; i++)
		sc[i][0] = yp[i];
	for(i = 0; i <= numPoints - 2; i++) {
		sc[i][1] = (sc[i + 1][0] - sc[i][0]) / h[i] - h[i] / 3 * (sc[i + 1][2] + 2 * sc[i][2]);
		sc[i][3] = (sc[i + 1][2] - sc[i][2]) / 3 / h[i];
	}
	free(h);
	free(x);
	free(r);
	free(m);
	free(xp);
	free(yp);
	return (sc);
}

void gen_interp_unwrap(CurvePoints * plot)
{
	double y, lasty, diff;
	int    curves = num_curves(plot);
	int    first_point = 0;
	for(int i = 0; i < curves; i++) {
		const int  num_points = next_curve(plot, &first_point);
		lasty = 0; // make all plots start the same place
		for(int j = first_point; j < first_point + num_points; j++) {
			if(plot->points[j].type != UNDEFINED) {
				y = plot->points[j].y;
				do {
					diff = y - lasty;
					if(diff >  M_PI)
						y -= 2*M_PI;
					if(diff < -M_PI)
						y += 2*M_PI;
				} while(fabs(diff) > M_PI);
				plot->points[j].y = y;
				lasty = y;
			}
		}
		GpGg.DoFreq(plot, first_point, num_points);
		first_point += num_points + 1;
	}
}
// 
// ARGS:
// plot - still containes old plot->points 
// sc - generated by cp_tridiag
// first_point - where to start in plot->points 
// num_points - to determine end in plot->points 
// dest - where to put the interpolated data 
//
//static void do_cubic(CurvePoints * plot, spline_coeff * sc, int first_point, int num_points, GpCoordinate * dest)
void GpGadgets::DoCubic(CurvePoints * pPlot, spline_coeff * pSc, int firstPoint, int numPoints, GpCoordinate * pDest)
{
	double xdiff, temp, x, y;
	double xstart, xend;    /* Endpoints of the sampled x range */
	int i, l;
	GpCoordinate * this_points;
	//
	// min and max in internal (eg logged) co-ordinates. We update
	// these, then update the external extrema in user co-ordinates at the end.
	//
	double ixmin, ixmax, iymin, iymax;
	double sxmin, sxmax, symin, symax; /* starting values of above */
	XAxis = (AXIS_INDEX)pPlot->x_axis;
	YAxis = (AXIS_INDEX)pPlot->y_axis;
	ixmin = sxmin = LogValue(XAxis, GetX().Range.low);
	ixmax = sxmax = LogValue(XAxis, GetX().Range.upp);
	iymin = symin = LogValue(YAxis, GetY().Range.low);
	iymax = symax = LogValue(YAxis, GetY().Range.upp);
	this_points = (pPlot->points) + firstPoint;
	l = 0;
	// HBB 20010727: Sample only across the actual x range, not the full range of input data
#if SAMPLE_CSPLINES_TO_FULL_RANGE
	xstart = this_points[0].x;
	xend = this_points[numPoints - 1].x;
#else
	xstart = MAX(this_points[0].x, sxmin);
	xend = MIN(this_points[numPoints - 1].x, sxmax);
	if(xstart >= xend)
		GpGg.IntError(pPlot->Token, "Cannot smooth: no data within fixed xrange!");
#endif
	xdiff = (xend - xstart) / (Samples1 - 1);
	for(i = 0; i < Samples1; i++) {
		x = xstart + i * xdiff;
		// Move forward to the spline interval this point is in
		while((x >= this_points[l + 1].x) && (l < numPoints - 2))
			l++;
		/* KB 981107: With logarithmic x axis the values were
		 * converted back to linear scale before calculating the
		 * coefficients. Use exponential for log x values. */
		temp = DelogValue(XAxis, x) - DelogValue(XAxis, this_points[l].x);
		// Evaluate cubic spline polynomial
		y = ((pSc[l][3] * temp + pSc[l][2]) * temp + pSc[l][1]) * temp + pSc[l][0];
		// With logarithmic y axis, we need to convert from linear to log scale now.
		if(GetY().Flags & GpAxis::fLog) {
			y = (y > 0.0) ? DoLog(YAxis, y) : (symin - (symax - symin));
		}
		pDest[i].type = INRANGE;
		STORE_AND_FIXUP_RANGE(pDest[i].x, x, pDest[i].type, ixmin, ixmax, GetX().AutoScale, NOOP, continue);
		STORE_AND_FIXUP_RANGE(pDest[i].y, y, pDest[i].type, iymin, iymax, GetY().AutoScale, NOOP, NOOP);
		pDest[i].xlow = pDest[i].xhigh = pDest[i].x;
		pDest[i].ylow = pDest[i].yhigh = pDest[i].y;
		pDest[i].z = -1;
	}
	UPDATE_RANGE(ixmax > sxmax, GetX().Range.upp, ixmax, XAxis);
	UPDATE_RANGE(ixmin < sxmin, GetX().Range.low, ixmin, XAxis);
	UPDATE_RANGE(iymax > symax, GetY().Range.upp, iymax, YAxis);
	UPDATE_RANGE(iymin < symin, GetY().Range.low, iymin, YAxis);
}
//
// do_freq() is like the other smoothers only in that it
// needs to adjust the plot ranges. We don't have to copy
// approximated curves or anything like that.
// 
// plot - still contains old plot->points
// first_point - where to start in plot->points
// num_points - to determine end in plot->points
//
//static void do_freq(CurvePoints * plot, int first_point, int num_points)
void GpGadgets::DoFreq(CurvePoints * pPlot, int firstPoint, int numPoints)
{
	const AXIS_INDEX x_axis = (AXIS_INDEX)pPlot->x_axis;
	const AXIS_INDEX y_axis = (AXIS_INDEX)pPlot->y_axis;
	/* min and max in internal (eg logged) co-ordinates. We update
	 * these, then update the external extrema in user co-ordinates
	 * at the end.
	 */
	double ixmin = LogValue(x_axis, GetX().Range.low);
	const double sxmin = ixmin;
	double ixmax = LogValue(x_axis, GetX().Range.upp);
	const double sxmax = ixmax;
	double iymin = LogValue(y_axis, GetY().Range.low);
	const double symin = iymin;
	double iymax = LogValue(y_axis, GetY().Range.upp);
	const double symax = iymax;
	GpCoordinate * p_this = (pPlot->points) + firstPoint;
	for(int i = 0; i < numPoints; i++) {
		double x = p_this[i].x;
		double y = p_this[i].y;
		p_this[i].type = INRANGE;
		STORE_AND_FIXUP_RANGE(p_this[i].x, x, p_this[i].type, ixmin, ixmax, GetX().AutoScale, NOOP, continue);
		STORE_AND_FIXUP_RANGE(p_this[i].y, y, p_this[i].type, iymin, iymax, GetY().AutoScale, NOOP, NOOP);
		p_this[i].xlow = p_this[i].xhigh = p_this[i].x;
		p_this[i].ylow = p_this[i].yhigh = p_this[i].y;
		p_this[i].z = -1;
	}
	UPDATE_RANGE(ixmax > sxmax, GetX().Range.upp, ixmax, x_axis);
	UPDATE_RANGE(ixmin < sxmin, GetX().Range.low, ixmin, x_axis);
	UPDATE_RANGE(iymax > symax, GetY().Range.upp, iymax, y_axis);
	UPDATE_RANGE(iymin < symin, GetY().Range.low, iymin, y_axis);
}

/*
 * Frequency plots have don't need new points allocated; we just need
 * to adjust the plot ranges. Wedging this into gen_interp() would
 * make that code even harder to read.
 */

void gen_interp_frequency(CurvePoints * plot)
{
	int    i, j;
	int    first_point, num_points;
	double y;
	double y_total = 0.0;
	int    curves = num_curves(plot);
	if(plot->plot_smooth == SMOOTH_CUMULATIVE_NORMALISED) {
		first_point = 0;
		for(i = 0; i < curves; i++) {
			num_points = next_curve(plot, &first_point);
			for(j = first_point; j < first_point + num_points; j++) {
				if(plot->points[j].type != UNDEFINED) {
					y_total += plot->points[j].y;
				}
			}
			first_point += num_points + 1;
		}
	}
	first_point = 0;
	for(i = 0; i < curves; i++) {
		num_points = next_curve(plot, &first_point);
		/* If cumulative, replace the current y-value with the
		   sum of all previous y-values. This assumes that the
		   data has already been sorted by x-values. */
		if(plot->plot_smooth == SMOOTH_CUMULATIVE) {
			y = 0;
			for(j = first_point; j < first_point + num_points; j++) {
				if(plot->points[j].type != UNDEFINED) {
					y += plot->points[j].y;
					plot->points[j].y = y;
				}
			}
		}
		/* Alternatively, cumulative normalised means replace the
		   current y-value with the sum of all previous y-values
		   divided by the total sum of all values.  This assumes the
		   data is sorted as before.  Normalising in this way allows
		   comparison of the CDF of data sets with differing total
		   numbers of samples.  */

		if(plot->plot_smooth == SMOOTH_CUMULATIVE_NORMALISED) {
			y = 0;
			for(j = first_point; j < first_point + num_points; j++) {
				if(plot->points[j].type == UNDEFINED)
					continue;

				y += plot->points[j].y;
				plot->points[j].y = y / y_total;
			}
		}
		GpGg.DoFreq(plot, first_point, num_points);
		first_point += num_points + 1;
	}
	return;
}
/*
 * This is the shared entry point used for the original smoothing options
 * csplines acsplines bezier sbezier
 */
//void gen_interp(CurvePoints * plot)
void GpGadgets::GenInterp(CurvePoints * pPlot)
{
	spline_coeff * sc;
	double * bc;
	int i;
	int curves = num_curves(pPlot);
	GpCoordinate * new_points = (GpCoordinate *)malloc((Samples1 + 1) * curves * sizeof(GpCoordinate));
	int first_point = 0;
	int num_points = 0;
	for(i = 0; i < curves; i++) {
		num_points = next_curve(pPlot, &first_point);
		switch(pPlot->plot_smooth) {
			case SMOOTH_CSPLINES:
			    sc = CpTriDiag(pPlot, first_point, num_points);
			    DoCubic(pPlot, sc, first_point, num_points, new_points + i * (Samples1 + 1));
			    free(sc);
			    break;
			case SMOOTH_ACSPLINES:
			    sc = cp_approx_spline(pPlot, first_point, num_points);
			    DoCubic(pPlot, sc, first_point, num_points, new_points + i * (Samples1 + 1));
			    free(sc);
			    break;

			case SMOOTH_BEZIER:
			case SMOOTH_SBEZIER:
			    bc = cp_binomial(num_points);
			    DoBezier(pPlot, bc, first_point, num_points, new_points + i * (Samples1 + 1));
			    free((char*)bc);
			    break;
			case SMOOTH_KDENSITY:
			    DoKDensity(pPlot, first_point, num_points, new_points + i * (Samples1 + 1));
			    break;
			default: /* keep gcc -Wall quiet */
			    ;
		}
		new_points[(i + 1) * (Samples1 + 1) - 1].type = UNDEFINED;
		first_point += num_points;
	}
	free(pPlot->points);
	pPlot->points = new_points;
	pPlot->p_max = curves * (Samples1 + 1);
	pPlot->p_count = pPlot->p_max - 1;
}

/*
 * sort_points
 *
 * sort data succession for further evaluation by plot_splines, etc.
 * This routine is mainly introduced for compilers *NOT* supporting the
 * UNIX qsort() routine. You can then easily replace it by more convenient
 * stuff for your compiler.
 * (MGR 1992)
 */

/* HBB 20010720: To avoid undefined behaviour that would be caused by
 * casting functions pointers around, changed arguments to what
 * qsort() *really* wants */
static int compare_points(const void * arg1, const void * arg2)
{
	GpCoordinate const * p1 = (GpCoordinate const *)arg1;
	GpCoordinate const * p2 = (GpCoordinate const *)arg2;
	if(p1->x > p2->x)
		return (1);
	if(p1->x < p2->x)
		return (-1);
	return (0);
}

void sort_points(CurvePoints * plot)
{
	int num_points;
	int first_point = 0;
	while((num_points = next_curve(plot, &first_point)) > 0) {
		/* Sort this set of points, does qsort handle 1 point correctly? */
		/* HBB 20010720: removed casts -- they don't help a thing, but
		 * may hide problems */
		qsort(plot->points + first_point, num_points, sizeof(GpCoordinate), compare_points);
		first_point += num_points;
	}
	return;
}

/*
 * cp_implode() if averaging is selected this function computes the new
 *              entries and shortens the whole thing to the necessary
 *              size
 * MGR Addendum
 */

//void cp_implode(CurvePoints * cp)
void GpGadgets::CpImplode(CurvePoints * pCp)
{
	int first_point, num_points;
	int i, j, k;
	double x = 0., y = 0., sux = 0., slx = 0., suy = 0., sly = 0.;
	double weight; /* used for acsplines */
	bool all_inrange = false;
	XAxis = (AXIS_INDEX)pCp->x_axis;
	YAxis = (AXIS_INDEX)pCp->y_axis;
	j = 0;
	first_point = 0;
	while((num_points = next_curve(pCp, &first_point)) > 0) {
		k = 0;
		for(i = first_point; i < first_point + num_points; i++) {
			// HBB 20020801: don't try to use undefined datapoints
			if(pCp->points[i].type != UNDEFINED) {
				if(!k) {
					x = pCp->points[i].x;
					y = pCp->points[i].y;
					sux = pCp->points[i].xhigh;
					slx = pCp->points[i].xlow;
					suy = pCp->points[i].yhigh;
					sly = pCp->points[i].ylow;
					weight = pCp->points[i].z;
					all_inrange = (pCp->points[i].type == INRANGE);
					k = 1;
				}
				else if(pCp->points[i].x == x) {
					y += pCp->points[i].y;
					sux += pCp->points[i].xhigh;
					slx += pCp->points[i].xlow;
					suy += pCp->points[i].yhigh;
					sly += pCp->points[i].ylow;
					weight += pCp->points[i].z;
					if(pCp->points[i].type != INRANGE)
						all_inrange = false;
					k++;
				}
				else {
					pCp->points[j].x = x;
					if(oneof3(pCp->plot_smooth, SMOOTH_FREQUENCY, SMOOTH_CUMULATIVE, SMOOTH_CUMULATIVE_NORMALISED))
						k = 1;
					pCp->points[j].y = y /= (double)k;
					pCp->points[j].xhigh = sux / (double)k;
					pCp->points[j].xlow = slx / (double)k;
					pCp->points[j].yhigh = suy / (double)k;
					pCp->points[j].ylow = sly / (double)k;
					pCp->points[j].z = weight / (double)k;
					/* HBB 20000405: I wanted to use STORE_AND_FIXUP_RANGE
					* here, but won't: it assumes we want to modify the
					* range, and that the range is given in 'input'
					* coordinates.  For logarithmic axes, the overhead
					* would be larger than the possible gain, so write it
					* out explicitly, instead:
					* */
					pCp->points[j].type = INRANGE;
					if(!all_inrange) {
						if(GetX().Flags & GpAxis::fLog) {
							if(x <= -GPVL) {
								pCp->points[j].type = OUTRANGE;
								goto is_outrange;
							}
							x = UndoLog(XAxis, x);
						}
						if(((x < GetX().Range.low) && !(GetX().AutoScale & AUTOSCALE_MIN)) || 
							((x > GetX().Range.upp) && !(GetX().AutoScale & AUTOSCALE_MAX))) {
							pCp->points[j].type = OUTRANGE;
							goto is_outrange;
						}
						if(GetY().Flags & GpAxis::fLog) {
							if(y <= -GPVL) {
								pCp->points[j].type = OUTRANGE;
								goto is_outrange;
							}
							y = UndoLog(YAxis, y);
						}
						if(((y < GetY().Range.low) && !(GetY().AutoScale & AUTOSCALE_MIN)) || 
							((y > GetY().Range.upp) && !(GetY().AutoScale & AUTOSCALE_MAX)))
							pCp->points[j].type = OUTRANGE;
is_outrange:
						;
					} /* if(! all inrange) */

					j++; /* next valid entry */
					k = 0; /* to read */
					i--; /* from this (-> last after for(;;)) entry */
				}
			}
		}
		if(k) {
			pCp->points[j].x = x;
			if(oneof3(pCp->plot_smooth, SMOOTH_FREQUENCY, SMOOTH_CUMULATIVE, SMOOTH_CUMULATIVE_NORMALISED))
				k = 1;
			pCp->points[j].y = y /= (double)k;
			pCp->points[j].xhigh = sux / (double)k;
			pCp->points[j].xlow = slx / (double)k;
			pCp->points[j].yhigh = suy / (double)k;
			pCp->points[j].ylow = sly / (double)k;
			pCp->points[j].z = weight / (double)k;
			pCp->points[j].type = INRANGE;
			if(!all_inrange) {
				if(GetX().Flags & GpAxis::fLog) {
					if(x <= -GPVL) {
						pCp->points[j].type = OUTRANGE;
						goto is_outrange2;
					}
					x = UndoLog(XAxis, x);
				}
				if(((x < GetX().Range.low) && !(GetX().AutoScale & AUTOSCALE_MIN)) || 
					((x > GetX().Range.upp) && !(GetX().AutoScale & AUTOSCALE_MAX))) {
					pCp->points[j].type = OUTRANGE;
					goto is_outrange2;
				}
				if(GetY().Flags & GpAxis::fLog) {
					if(y <= -GPVL) {
						pCp->points[j].type = OUTRANGE;
						goto is_outrange2;
					}
					y = UndoLog(YAxis, y);
				}
				if(((y < GetY().Range.low) && !(GetY().AutoScale & AUTOSCALE_MIN)) || 
					((y > GetY().Range.upp) && !(GetY().AutoScale & AUTOSCALE_MAX)))
					pCp->points[j].type = OUTRANGE;
is_outrange2:
				;
			}
			j++;    /* next valid entry */
		}

		/* FIXME: Monotonic cubic splines support only a single curve per data set */
		if(j < pCp->p_count && pCp->plot_smooth == SMOOTH_MONOTONE_CSPLINE) {
			break;
		}

		/* insert invalid point to separate curves */
		if(j < pCp->p_count) {
			pCp->points[j].type = UNDEFINED;
			j++;
		}
		first_point += num_points;
	}                       /* end while */
	pCp->p_count = j;
	cp_extend(pCp, j);
}

/*
 * EAM December 2013
 * monotonic cubic spline using the Fritsch-Carlson algorithm
 * FN Fritsch & RE Carlson (1980). "Monotone Piecewise Cubic Interpolation".
 * SIAM Journal on Numerical Analysis (SIAM) 17 (2): 238–246. doi:10.1137/0717021.
 */

//void mcs_interp(CurvePoints * plot)
void GpGadgets::McsInterp(CurvePoints * pPlot)
{
	/* These track the original (pre-sorted) data points */
	int N = pPlot->p_count;
	GpCoordinate * p = (GpCoordinate *)gp_realloc(pPlot->points, (N+1) * sizeof(GpCoordinate), "mcs");
	int i;

	/* These will track the resulting smoothed curve */
	/* V5: Try to ensure that the sampling is fine enough to pass through the original points */
	int Nsamp = (Samples1 > 2*N) ? Samples1 : 2*N;
	GpCoordinate * new_points = (GpCoordinate *)malloc((Nsamp+1) * sizeof(GpCoordinate));
	double sxmin = LogValue((AXIS_INDEX)pPlot->x_axis, GetX().Range.low);
	double sxmax = LogValue((AXIS_INDEX)pPlot->x_axis, GetX().Range.upp);
	double xstart = MAX(p[0].x, sxmin);
	double xend = MIN(p[N-1].x, sxmax);
	double xstep = (xend - xstart) / (Nsamp - 1);
	// Calculate spline coefficients
#define DX      xlow
#define SLOPE   xhigh
#define C1      ylow
#define C2      yhigh
#define C3      z
	// Work with the un-logged y values
	for(i = 0; i < N-1; i++)
		p[i].y = DelogValue((AXIS_INDEX)pPlot->y_axis, p[i].y);

	for(i = 0; i < N-1; i++) {
		p[i].DX = p[i+1].x - p[i].x;
		p[i].SLOPE = (p[i+1].y - p[i].y) / p[i].DX;
	}
	p[N-1].SLOPE = 0;

	p[0].C1 = p[0].SLOPE;
	for(i = 0; i < N-1; i++) {
		if(p[i].SLOPE * p[i+1].SLOPE <= 0) {
			p[i+1].C1 = 0;
		}
		else {
			const double sum = p[i].DX + p[i+1].DX;
			p[i+1].C1 = (3. * sum) / ((sum + p[i+1].DX) /  p[i].SLOPE + (sum + p[i].DX) /  p[i+1].SLOPE);
		}
	}
	p[N].C1 = p[N-1].SLOPE;
	for(i = 0; i < N; i++) {
		const double temp = p[i].C1 + p[i+1].C1 - 2*p[i].SLOPE;
		p[i].C2 = (p[i].SLOPE - p[i].C1 -temp) / p[i].DX;
		p[i].C3 = temp / (p[i].DX * p[i].DX);
	}
	// Use the coefficients C1, C2, C3 to interpolate over the requested range
	for(i = 0; i < Nsamp; i++) {
		double x = xstart + i * xstep;
		double y;
		bool exact = false;
		if(x == p[N-1].x) { /* Exact value for right-most point of original data */
			y = p[N-1].y;
			exact = true;
		}
		else {
			int low = 0;
			int mid;
			int high = N-1;
			while(low <= high) {
				mid = (int)floor((low + high) / 2.0);
				if(p[mid].x < x)
					low = mid + 1;
				else if(p[mid].x > x)
					high = mid - 1;
				else { /* Exact value for some point in original data */
					y = p[mid].y;
					exact = true;
					break;
				}
			}
			if(!exact) {
				int j = MAX(0, high);
				double diff = x - p[j].x;
				y = p[j].y + p[j].C1 * diff + p[j].C2 * diff * diff + p[j].C3 * diff * diff * diff;
			}
		}
		// FIXME:  Log x?  autoscale x?
		new_points[i].x = x;
		new_points[i].type = INRANGE;
		STORE_WITH_LOG_AND_UPDATE_RANGE(new_points[i].y, y, new_points[i].type, pPlot->y_axis, pPlot->noautoscale, NOOP, NOOP);
	}
	// Replace original data with the interpolated curve
	free(p);
	pPlot->points = new_points;
	pPlot->p_count = Nsamp;
	pPlot->p_max = Nsamp + 1;

#undef DX
#undef SLOPE
#undef C1
#undef C2
#undef C3
}

#ifdef SMOOTH_BINS_OPTION
/*
 * Binned histogram of input values.
 *   plot FOO using N:(1) bins{=<nbins>} {binrange=[binlow:binhigh]} with boxes
 * If no binrange is given, the range is taken from the x axis range.
 * In the latter case "set xrange" may exclude some data points,
 * while "set auto x" will include all data points.
 */
void make_bins(CurvePoints * plot, int nbins, double binlow, double binhigh)
{
	int i, binno;
	double * bin;
	double bottom, top, binwidth, range;
	GpAxis * xaxis = &GpGg[plot->x_axis];
	GpAxis * yaxis = &GpGg[plot->y_axis];
	double ymax = 0;
	int N = plot->p_count;

	/* Divide the range on X into the requested number of bins.
	 * NB: This range is independent of the values of the points.
	 */
	if(binlow == 0 && binhigh == 0) {
		bottom = xaxis->DataRange.low;
		top = xaxis->DataRange.upp;
	}
	else {
		bottom = binlow;
		top = binhigh;
	}
	bottom = xaxis->LogValue(bottom);
	top = xaxis->LogValue(top);
	binwidth = (top - bottom) / (nbins - 1);
	bottom -= binwidth/2.;
	top += binwidth/2.;
	range = top - bottom;
	bin = (double *)malloc(nbins*sizeof(double));
	for(i = 0; i<nbins; i++)
		bin[i] = 0;
	for(i = 0; i<N; i++) {
		if(plot->points[i].type != UNDEFINED) {
			binno = (int)floor(nbins * (plot->points[i].x - bottom) / range);
			// FIXME: Should outrange points be dumped in the first/last bin?
			if(0 <= binno && binno < nbins)
				bin[binno] += yaxis->DeLogValue(plot->points[i].y);
		}
	}
	if(xaxis->AutoScale & AUTOSCALE_MIN) {
		SETMIN(xaxis->Range.low, bottom);
	}
	if(xaxis->AutoScale & AUTOSCALE_MAX) {
		SETMAX(xaxis->Range.upp, top);
	}
	/* Replace the original data with one entry per bin.
	 * new x = midpoint of bin
	 * new y = number of points in the bin
	 */
	plot->p_count = nbins;
	plot->points = (GpCoordinate *)gp_realloc(plot->points, nbins * sizeof(GpCoordinate), "CurvePoints");
	for(i = 0; i<nbins; i++) {
		double bincent = bottom + (0.5 + (double)i) * binwidth;
		plot->points[i].type = INRANGE;
		plot->points[i].x     = bincent;
		plot->points[i].xlow  = bincent - binwidth/2.;
		plot->points[i].xhigh = bincent + binwidth/2.;
		plot->points[i].y     = yaxis->LogValue(bin[i]);
		plot->points[i].ylow  = plot->points[i].y;
		plot->points[i].yhigh = plot->points[i].y;
		plot->points[i].z = 0; /* FIXME: leave it alone? */
		if(xaxis->InRange(xaxis->DeLogValue(bincent))) {
			SETMAX(ymax, bin[i]);
		}
		else {
			plot->points[i].type = OUTRANGE;
		}
		FPRINTF((stderr, "bin[%d] %g %g\n", i, plot->points[i].x, plot->points[i].y));
	}
	if(yaxis->AutoScale & AUTOSCALE_MIN) {
		SETMIN(yaxis->Range.low, 0);
	}
	if(yaxis->AutoScale & AUTOSCALE_MAX) {
		SETMAX(yaxis->Range.upp, ymax);
	}
	// Clean up
	free(bin);
}

#endif
