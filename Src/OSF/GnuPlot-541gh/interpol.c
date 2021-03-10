// GNUPLOT - interpol.c 
// Copyright 1986 - 1993, 1998, 2004   Thomas Williams, Colin Kelley
//
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
 *    - structs: curve_points, coordval, coordinate
 *
 *  setshow.h
 *    - samples, axis array[] variables
 *    - plottypes
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
 *  Jun 30, 1996 Jens Emmerich
 *      implemented handling of UNDEFINED points
 *  Dec 2019 EAM
 *	move solve_tri_diag from contour.c to here
 *	generalize cp_tridiag to work on any coordinate dimension
 */
#include <gnuplot.h>
#pragma hdrstop

static int next_curve(curve_points * plot, int * curve_start);
static int num_curves(curve_points * plot);
static double eval_kdensity(curve_points * cp, int first_point, int num_points, double x);
static double * cp_binomial(int points);
static void eval_bezier(const curve_points * cp, int first_point, int num_points, double sr, coordval * px, coordval * py, coordval * py2, double * c);
static int solve_tri_diag(tri_diag m[], double r[], double x[], int n);
static int solve_five_diag(GpFiveDiag m[], double r[], double x[], int n);
static int do_curve_cleanup(GpCoordinate * point, int npoints);
static int compare_points(SORTFUNC_ARGS p1, SORTFUNC_ARGS p2);
static int compare_z(SORTFUNC_ARGS p1, SORTFUNC_ARGS p2);
/*
 * position curve_start to index the next non-UNDEFINDED point,
 * start search at initial curve_start,
 * return number of non-UNDEFINDED points from there on,
 * if no more valid points are found, curve_start is set
 * to plot->p_count and 0 is returned
 */
static int next_curve(curve_points * plot, int * curve_start)
{
	int curve_length;
	// Skip undefined points 
	while(*curve_start < plot->p_count && plot->points[*curve_start].type == UNDEFINED) {
		(*curve_start)++;
	}
	curve_length = 0;
	// curve_length is first used as an offset, then the correct # points 
	while((*curve_start) + curve_length < plot->p_count && plot->points[(*curve_start) + curve_length].type != UNDEFINED) {
		curve_length++;
	}
	return (curve_length);
}
/*
 * determine the number of curves in plot->points, separated by
 * UNDEFINED points
 */
static int num_curves(curve_points * plot)
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
 */
static double kdensity_bandwidth = 0;

static void stats_kdensity(curve_points * cp,
    int first_point,    /* where to start in plot->points (to find x-range) */
    int num_points      /* to determine end in plot->points */
    ) {
	GpCoordinate * this_points = (cp->points) + first_point;
	double default_bandwidth;
	int i;
	double kdensity_avg = 0.0;
	double kdensity_sigma = 0.0;
	for(i = 0; i < num_points; i++) {
		kdensity_avg   += this_points[i].x;
		kdensity_sigma += this_points[i].x * this_points[i].x;
	}
	kdensity_avg /= (double)num_points;
	kdensity_sigma = sqrt(kdensity_sigma/(double)num_points - kdensity_avg*kdensity_avg);
	/* This is the optimal bandwidth if the point distribution is Gaussian.
	   (Applied Smoothing Techniques for Data Analysis
	   by Adrian W, Bowman & Adelchi Azzalini (1997)) */
	/* If the supplied bandwidth is zero of less, the default bandwidth is used. */
	default_bandwidth = pow(4.0/(3.0*num_points), 1.0/5.0) * kdensity_sigma;
	if(cp->smooth_parameter <= 0) {
		kdensity_bandwidth = default_bandwidth;
		cp->smooth_parameter = -default_bandwidth;
	}
	else
		kdensity_bandwidth = cp->smooth_parameter;
}
//
// eval_kdensity is a modification of eval_bezier 
//
static double eval_kdensity(curve_points * cp, int first_point/* where to start in plot->points (to find x-range) */,
    int num_points/* to determine end in plot->points */, double x/* x value at which to calculate y */) 
{
	GpCoordinate * this_points = (cp->points) + first_point;
	double period = cp->smooth_period;
	double Z;
	double y = 0.0;
	for(int i = 0; i < num_points; i++) {
		double dist = fabs(x - this_points[i].x);
		if(period > 0 && dist > period/2)
			dist = period - dist;
		Z = dist / kdensity_bandwidth;
		y += this_points[i].y * exp(-0.5*Z*Z) / kdensity_bandwidth;
	}
	y /= sqrt(SMathConst::Pi2);
	return y;
}
//
// do_kdensity is based on do_bezier, except for the call to eval_bezier 
// EAM Feb 2015: Don't touch xrange, but recalculate y limits  
//
//static void do_kdensity(curve_points * cp, int firstPoint/* where to start in plot->points */, int num_points/* to determine end in plot->points */,
    //GpCoordinate * dest/* where to put the interpolated data */)
void GnuPlot::DoKDensity(curve_points * cp, int firstPoint/* where to start in plot->points */, int num_points/* to determine end in plot->points */, GpCoordinate * dest/* where to put the interpolated data */)
{
	int i;
	double sxmin, sxmax, step;
	AxS.Idx_X = cp->AxIdx_X;
	AxS.Idx_Y = cp->AxIdx_Y;
	if(AxS.__X().log)
		IntWarn(NO_CARET, "kdensity components are Gaussian on x, not log(x)");
	sxmin = AxS.__X().min;
	sxmax = AxS.__X().max;
	step = (sxmax - sxmin) / (Gg.Samples1 - 1);
	stats_kdensity(cp, firstPoint, num_points);
	for(i = 0; i < Gg.Samples1; i++) {
		double x = sxmin + i * step;
		double y = eval_kdensity(cp, firstPoint, num_points, x);
		// now we have to store the points and adjust the ranges 
		dest[i].type = INRANGE;
		dest[i].x = x;
		store_and_update_range(&dest[i].y, y, &dest[i].type, &AxS.__Y(), cp->noautoscale);
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
	double * coeff;
	int n, k;
	int e;
	e = points;             /* well we're going from k=0 to k=p_count-1 */
	coeff = (double*)SAlloc::M(e * sizeof(double));
	n = points - 1;
	e = n / 2;
	/* HBB 990205: calculate these in 'logarithmic space',
	* as they become _very_ large, with growing n (4^n) */
	coeff[0] = 0.0;
	for(k = 0; k < e; k++) {
		coeff[k + 1] = coeff[k] + log(((double)(n - k)) / ((double)(k + 1)));
	}
	for(k = n; k >= e; k--)
		coeff[k] = coeff[n - k];
	return (coeff);
}

/* This is a subfunction of do_bezier() for BEZIER style computations.
 * It is passed the step fraction (STEP/MAXSTEPS) and the addresses of
 * the double values holding the next x and y coordinates.
 * (MGR 1992)
 * Feb 2020: Do yhigh also so that filledcurves can use it
 */
static void eval_bezier(const curve_points * cp,
    int first_point/* where to start in plot->points (to find x-range) */,
    int num_points/* to determine end in plot->points */,
    double sr/* position inside curve, range [0:1] */,
    coordval * px/* OUTPUT: x and y */,
    coordval * py,
    coordval * py2/* used for 2nd border of fillcurves */,
    double * c/* Bezier coefficient array */)
{
	uint n = num_points - 1;
	const GpCoordinate * p_points = (cp->points) + first_point;
	if(sr == 0.0) {
		*px = p_points[0].x;
		*py = p_points[0].y;
		*py2 = p_points[0].yhigh;
	}
	else if(sr == 1.0) {
		*px = p_points[n].x;
		*py = p_points[n].y;
		*py2 = p_points[n].yhigh;
	}
	else {
		// HBB 990205: do calculation in 'logarithmic space',
		// to avoid over/underflow errors, which would exactly cancel
		// out each other, anyway, in an exact calculation
		double lx = 0.0, ly = 0.0, ly2 = 0.0;
		double log_dsr_to_the_n = n * log(1 - sr);
		double log_sr_over_dsr = log(sr) - log(1 - sr);
		for(uint i = 0; i <= n; i++) {
			double u = exp(c[i] + log_dsr_to_the_n + i * log_sr_over_dsr);
			lx += p_points[i].x * u;
			ly += p_points[i].y * u;
			ly2 += p_points[i].yhigh * u;
		}
		*px = lx;
		*py = ly;
		*py2 = ly2;
	}
}
// 
// Generate a new set of coordinates representing the bezier curve.
// Note that these are sampled evenly across the x range (from "set samples N")
// rather than corresponding to x values of the original data points.
// 
//static void do_bezier(curve_points * cp, double * bc/* Bezier coefficient array */, int first_point/* where to start in plot->points */,
    //int num_points/* to determine end in plot->points */, GpCoordinate * dest/* where to put the interpolated data */)
void GnuPlot::DoBezier(curve_points * cp, double * bc/* Bezier coefficient array */, int firstPoint/* where to start in plot->points */,
	int numPoints/* to determine end in plot->points */, GpCoordinate * pDest/* where to put the interpolated data */)
{
	coordval x, y, yhigh;
	AxS.Idx_X = cp->AxIdx_X;
	AxS.Idx_Y = cp->AxIdx_Y;
	for(int i = 0; i < Gg.Samples1; i++) {
		eval_bezier(cp, firstPoint, numPoints, (double)i / (double)(Gg.Samples1 - 1), &x, &y, &yhigh, bc);
		pDest[i].type = INRANGE;
		store_and_update_range(&pDest[i].x, x, &pDest[i].type, &AxS.__X(), AxS.__X().autoscale);
		store_and_update_range(&pDest[i].y, y, &pDest[i].type, &AxS.__Y(), AxS.__Y().autoscale);
		pDest[i].xlow = pDest[i].xhigh = pDest[i].x;
		pDest[i].ylow = pDest[i].yhigh = pDest[i].y;
		pDest[i].z = -1;
		pDest[i].yhigh = yhigh;
	}
}
/*
 * Solve five diagonal linear system equation. The five diagonal matrix is
 * defined via matrix M, right side is r, and solution X i.e. M * X = R.
 * Size of system given in n. Return TRUE if solution exist.
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
static int solve_five_diag(GpFiveDiag m[], double r[], double x[], int n)
{
	int i;
	GpFiveDiag * hv = (GpFiveDiag *)SAlloc::M((n + 1) * sizeof(GpFiveDiag));
	hv[0][0] = m[0][2];
	if(hv[0][0] == 0) {
		SAlloc::F(hv);
		return FALSE;
	}
	hv[0][1] = m[0][3] / hv[0][0];
	hv[0][2] = m[0][4] / hv[0][0];

	hv[1][3] = m[1][1];
	hv[1][0] = m[1][2] - hv[1][3] * hv[0][1];
	if(hv[1][0] == 0) {
		SAlloc::F(hv);
		return FALSE;
	}
	hv[1][1] = (m[1][3] - hv[1][3] * hv[0][2]) / hv[1][0];
	hv[1][2] = m[1][4] / hv[1][0];

	for(i = 2; i < n; i++) {
		hv[i][3] = m[i][1] - m[i][0] * hv[i - 2][1];
		hv[i][0] = m[i][2] - m[i][0] * hv[i - 2][2] - hv[i][3] * hv[i-1][1];
		if(hv[i][0] == 0) {
			SAlloc::F(hv);
			return FALSE;
		}
		hv[i][1] = (m[i][3] - hv[i][3] * hv[i-1][2]) / hv[i][0];
		hv[i][2] = m[i][4] / hv[i][0];
	}

	hv[0][4] = 0;
	hv[1][4] = r[0] / hv[0][0];
	for(i = 1; i < n; i++) {
		hv[i + 1][4] = (r[i] - m[i][0] * hv[i-1][4] - hv[i][3] * hv[i][4]) / hv[i][0];
	}

	x[n-1] = hv[n][4];
	x[n - 2] = hv[n-1][4] - hv[n - 2][1] * x[n-1];
	for(i = n - 3; i >= 0; i--)
		x[i] = hv[i + 1][4] - hv[i][1] * x[i + 1] - hv[i][2] * x[i + 2];

	SAlloc::F(hv);
	return TRUE;
}
// 
// Calculation of approximation cubic splines
// Returns matrix of spline coefficients
// Dec 2019 EAM - modified original routine cp_approx_spline for use with
//   multi-dimensional splines
// original code: created spline for y given x = control, variable z = weight
// revised code:  create spline for coordinate indexed by spline_dim
//  given control variable indexed by path_dim
//   weights indexed by w_dim
// 
//static GpSplineCoeff * cp_approx_spline(const GpCoordinate * pPoints, int numPoints, int pathDim, int splineDim, int wDim)
GpSplineCoeff * GnuPlot::CpApproxSpline(const GpCoordinate * pPoints, int numPoints, int pathDim, int splineDim, int wDim)
{
	GpSplineCoeff * sc;
	GpFiveDiag * m;
	double * r, * x, * h, * xp, * yp;
	int i;
	// Define an overlay onto GpCoordinate that lets us select whichever
	// of x,y,z,... is needed by specifying an index 0-6
	struct gen_coord {
		enum coord_type type;
		coordval dimension[7];
	};
	if(numPoints < 4)
		IntError(NO_CARET, "Can't calculate approximation splines, need at least 4 points");
	const struct gen_coord * this_point = reinterpret_cast<const struct gen_coord *>(pPoints);
	for(i = 0; i < numPoints; i++)
		if(this_point[i].dimension[wDim] <= 0)
			IntError(NO_CARET, "Can't calculate approximation splines, all weights have to be > 0");
	sc = (GpSplineCoeff *)SAlloc::M((numPoints) * sizeof(GpSplineCoeff));
	m = (GpFiveDiag *)SAlloc::M((numPoints - 2) * sizeof(GpFiveDiag));
	r = (double *)SAlloc::M((numPoints - 2) * sizeof(double));
	x = (double *)SAlloc::M((numPoints - 2) * sizeof(double));
	h = (double *)SAlloc::M((numPoints - 1) * sizeof(double));
	xp = (double *)SAlloc::M((numPoints) * sizeof(double));
	yp = (double *)SAlloc::M((numPoints) * sizeof(double));
	xp[0] = this_point[0].dimension[pathDim];
	yp[0] = this_point[0].dimension[splineDim];
	for(i = 1; i < numPoints; i++) {
		xp[i] = this_point[i].dimension[pathDim];
		yp[i] = this_point[i].dimension[splineDim];
		h[i-1] = xp[i] - xp[i-1];
	}
	// set up the matrix and the vector 
	for(i = 0; i <= numPoints - 3; i++) {
		r[i] = 3 * ((yp[i + 2] - yp[i + 1]) / h[i + 1] - (yp[i + 1] - yp[i]) / h[i]);
		if(i < 2)
			m[i][0] = 0;
		else
			m[i][0] = 6 / this_point[i].dimension[wDim] / h[i-1] / h[i];
		if(i < 1)
			m[i][1] = 0;
		else
			m[i][1] = h[i] - 6 / this_point[i].dimension[wDim] / h[i] * (1 / h[i-1] + 1 / h[i]) - 6 / this_point[i + 1].dimension[wDim] / h[i] * (1 / h[i] + 1 / h[i + 1]);
		m[i][2] = 2 * (h[i] + h[i + 1])
		    + 6 / this_point[i].dimension[wDim] / h[i] / h[i]
		    + 6 / this_point[i + 1].dimension[wDim] * (1 / h[i] + 1 / h[i + 1]) * (1 / h[i] + 1 / h[i + 1])
		    + 6 / this_point[i + 2].dimension[wDim] / h[i + 1] / h[i + 1];
		if(i > numPoints - 4)
			m[i][3] = 0;
		else
			m[i][3] = h[i + 1] - 6 / this_point[i + 1].dimension[wDim] / h[i + 1] * (1 / h[i] + 1 / h[i + 1]) - 6 / this_point[i + 2].dimension[wDim] / h[i + 1] * (1 / h[i + 1] + 1 / h[i + 2]);
		if(i > numPoints - 5)
			m[i][4] = 0;
		else
			m[i][4] = 6 / this_point[i + 2].dimension[wDim] / h[i + 1] / h[i + 2];
	}
	// solve the matrix 
	if(!solve_five_diag(m, r, x, numPoints - 2)) {
		SAlloc::F(sc);
		SAlloc::F(h);
		SAlloc::F(x);
		SAlloc::F(r);
		SAlloc::F(m);
		SAlloc::F(xp);
		SAlloc::F(yp);
		IntError(NO_CARET, "Can't calculate approximation splines");
	}
	sc[0][2] = 0;
	for(i = 1; i <= numPoints - 2; i++)
		sc[i][2] = x[i-1];
	sc[numPoints-1][2] = 0;
	sc[0][0] = yp[0] + 2 / this_point[0].dimension[wDim] / h[0] * (sc[0][2] - sc[1][2]);
	for(i = 1; i <= numPoints - 2; i++)
		sc[i][0] = yp[i] - 2 / this_point[i].dimension[wDim] * (sc[i-1][2] / h[i-1] - sc[i][2] * (1 / h[i-1] + 1 / h[i]) + sc[i + 1][2] / h[i]);
	sc[numPoints-1][0] = yp[numPoints-1] - 2 / this_point[numPoints-1].dimension[wDim] / h[numPoints - 2] * (sc[numPoints - 2][2] - sc[numPoints-1][2]);
	for(i = 0; i <= numPoints - 2; i++) {
		sc[i][1] = (sc[i + 1][0] - sc[i][0]) / h[i] - h[i] / 3 * (sc[i + 1][2] + 2 * sc[i][2]);
		sc[i][3] = (sc[i + 1][2] - sc[i][2]) / 3 / h[i];
	}
	SAlloc::F(h);
	SAlloc::F(x);
	SAlloc::F(r);
	SAlloc::F(m);
	SAlloc::F(xp);
	SAlloc::F(yp);
	return (sc);
}
/*
 * Calculation of cubic splines
 * This can be treated as a special case of approximation cubic splines, with
 * all weights -> infinity.
 *
 * Returns matrix of spline coefficients
 *
 * Dec 2019 EAM - modified original routine cp_tridiag() for use to
 * create multi-dimensional splines
 *   original code: created a spline for y using x as the control variable
 *   revised code:  spline for arbitrary coord using another coordinate as control
 *
 * Previous call to cp_tridiag(plot, start, n)
 *      becomes     cp_tridiag(&plot->points[start], n, 0, 1)
 *                                                      X  Y  <==
 * To create a spline for an arbitrary coordinate, e.g. x, as a function of PATH
 *	load path increments into points[i].CRD_PATH
 *      cp_tridiag(points, n, PATHCOORD, 0)
 *
 */
//static GpSplineCoeff * cp_tridiag(const GpCoordinate * pPoints, int numPoints, int pathDim, int splineDim)
GpSplineCoeff * GnuPlot::CpTriDiag(const GpCoordinate * pPoints, int numPoints, int pathDim, int splineDim)
{
	GpSplineCoeff * sc;
	tri_diag * m;
	double * r, * x, * h, * xp, * yp;
	int i;
	// Define an overlay onto GpCoordinate that lets us select whichever
	// of x,y,z,... is needed by specifying an index 0-6
	struct gen_coord {
		enum coord_type type;
		coordval dimension[7];
	};
	if(numPoints < 3)
		IntError(NO_CARET, "Can't calculate splines, need at least 3 points");
	const struct gen_coord * this_point = reinterpret_cast<const struct gen_coord *>(pPoints);
	sc = (GpSplineCoeff *)SAlloc::M((numPoints) * sizeof(GpSplineCoeff));
	m = (tri_diag *)SAlloc::M((numPoints - 2) * sizeof(tri_diag));
	r = (double *)SAlloc::M((numPoints - 2) * sizeof(double));
	x = (double *)SAlloc::M((numPoints - 2) * sizeof(double));
	h = (double *)SAlloc::M((numPoints - 1) * sizeof(double));
	xp = (double *)SAlloc::M((numPoints) * sizeof(double));
	yp = (double *)SAlloc::M((numPoints) * sizeof(double));
	xp[0] = this_point[0].dimension[pathDim];
	yp[0] = this_point[0].dimension[splineDim];
	for(i = 1; i < numPoints; i++) {
		xp[i] = this_point[i].dimension[pathDim];
		yp[i] = this_point[i].dimension[splineDim];
		h[i-1] = xp[i] - xp[i-1];
	}
	// set up the matrix and the vector 
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
	// solve the matrix 
	if(!solve_tri_diag(m, r, x, numPoints - 2)) {
		SAlloc::F(sc);
		SAlloc::F(h);
		SAlloc::F(x);
		SAlloc::F(r);
		SAlloc::F(m);
		SAlloc::F(xp);
		SAlloc::F(yp);
		IntError(NO_CARET, "Can't calculate cubic splines");
	}
	sc[0][2] = 0;
	for(i = 1; i <= numPoints - 2; i++)
		sc[i][2] = x[i-1];
	sc[numPoints-1][2] = 0;
	for(i = 0; i <= numPoints - 1; i++)
		sc[i][0] = yp[i];
	for(i = 0; i <= numPoints - 2; i++) {
		sc[i][1] = (sc[i + 1][0] - sc[i][0]) / h[i] - h[i] / 3 * (sc[i + 1][2] + 2 * sc[i][2]);
		sc[i][3] = (sc[i + 1][2] - sc[i][2]) / 3 / h[i];
	}
	SAlloc::F(h);
	SAlloc::F(x);
	SAlloc::F(r);
	SAlloc::F(m);
	SAlloc::F(xp);
	SAlloc::F(yp);
	return (sc);
}
// 
// Solve tri diagonal linear system equation. The tri diagonal matrix is
// defined via matrix M, right side is r, and solution X i.e. M * X = R.
// Size of system given in n. Return TRUE if solution exist.
// 
static int solve_tri_diag(tri_diag m[], double r[], double x[], int n)
{
	int i;
	double t;
	for(i = 1; i < n; i++) { /* Eliminate element m[i][i-1] (lower diagonal). */
		if(m[i-1][1] == 0)
			return FALSE;
		t = m[i][0] / m[i-1][1]; /* Find ratio between the two lines. */
		m[i][1] = m[i][1] - m[i-1][2] * t;
		r[i] = r[i] - r[i-1] * t;
	}
	// Back substitution - update the solution vector X 
	if(m[n-1][1] == 0)
		return FALSE;
	x[n-1] = r[n-1] / m[n-1][1]; /* Find last element. */
	for(i = n - 2; i >= 0; i--) {
		if(m[i][1] == 0)
			return FALSE;
		x[i] = (r[i] - x[i + 1] * m[i][2]) / m[i][1];
	}
	return TRUE;
}

//void gen_interp_unwrap(curve_points * plot)
void GnuPlot::GenInterpUnwrap(curve_points * pPlot)
{
	double y, diff;
	int curves = num_curves(pPlot);
	int first_point = 0;
	for(int i = 0; i < curves; i++) {
		int num_points = next_curve(pPlot, &first_point);
		double lasty = 0; /* make all plots start the same place */
		for(int j = first_point; j < first_point + num_points; j++) {
			if(pPlot->points[j].type == UNDEFINED)
				continue;
			y = pPlot->points[j].y;
			do {
				diff = y - lasty;
				if(diff > SMathConst::Pi) 
					y -= SMathConst::Pi2;
				if(diff < -SMathConst::Pi) 
					y += SMathConst::Pi2;
			} while(fabs(diff) > SMathConst::Pi);
			pPlot->points[j].y = y;
			lasty = y;
		}
		DoFreq(pPlot, first_point, num_points);
		first_point += num_points + 1;
	}
}

//static void do_cubic(curve_points * pPlot/* still contains old plot->points */,
    //GpSplineCoeff * sc/* generated by cp_tridiag */, GpSplineCoeff * sc2/* optional spline for yhigh */,
    //int first_point/* where to start in plot->points */, int num_points/* to determine end in plot->points */,
    //GpCoordinate * dest/* where to put the interpolated data */)
void GnuPlot::DoCubic(curve_points * pPlot/* still contains old plot->points */,
	GpSplineCoeff * sc/* generated by cp_tridiag */, GpSplineCoeff * sc2/* optional spline for yhigh */,
	int first_point/* where to start in plot->points */, int num_points/* to determine end in plot->points */,
	GpCoordinate * dest/* where to put the interpolated data */)
{
	double xdiff, temp, x, y;
	double xstart, xend;    /* Endpoints of the sampled x range */
	int i, l;
	GpCoordinate * this_points;
	AxS.Idx_X = pPlot->AxIdx_X;
	AxS.Idx_Y = pPlot->AxIdx_Y;
	this_points = (pPlot->points) + first_point;
	l = 0;
	// HBB 20010727: Sample only across the actual x range, not the full range of input data 
#if SAMPLE_CSPLINES_TO_FULL_RANGE
	xstart = this_points[0].x;
	xend = this_points[num_points-1].x;
#else
	xstart = MAX(this_points[0].x, AxS.__X().min);
	xend = MIN(this_points[num_points-1].x, AxS.__X().max);
	if(xstart >= xend) {
		// This entire segment lies outside the current x range. 
		for(i = 0; i < Gg.Samples1; i++)
			dest[i].type = OUTRANGE;
		return;
	}
#endif
	xdiff = (xend - xstart) / (Gg.Samples1 - 1);
	for(i = 0; i < Gg.Samples1; i++) {
		x = xstart + i * xdiff;
		// Move forward to the spline interval this point is in 
		while((x >= this_points[l + 1].x) && (l < num_points - 2))
			l++;
		temp = x - this_points[l].x;
		// Evaluate cubic spline polynomial 
		y = ((sc[l][3] * temp + sc[l][2]) * temp + sc[l][1]) * temp + sc[l][0];
		dest[i].type = INRANGE;
		store_and_update_range(&dest[i].x, x, &dest[i].type, &AxS.__X(), AxS.__X().autoscale);
		store_and_update_range(&dest[i].y, y, &dest[i].type, &AxS.__Y(), AxS.__Y().autoscale);
		dest[i].xlow = dest[i].xhigh = dest[i].x;
		dest[i].ylow = dest[i].yhigh = dest[i].y;
		dest[i].z = -1;
		// This case is used when smoothing "x y yhigh with filledcurves" 
		if(sc2) {
			y = ((sc2[l][3] * temp + sc2[l][2]) * temp + sc2[l][1]) * temp + sc2[l][0];
			dest[i].yhigh = y;
		}
	}
}
//
// do_freq() is like the other smoothers only in that it
// needs to adjust the plot ranges. We don't have to copy
// approximated curves or anything like that.
//
//static void do_freq(curve_points * pPlot/* still contains old plot->points */, int first_point/* where to start in plot->points */, int num_points/* to determine end in plot->points */)
void GnuPlot::DoFreq(curve_points * pPlot/* still contains old plot->points */, int first_point/* where to start in plot->points */, int num_points/* to determine end in plot->points */)
{
	int x_axis = pPlot->AxIdx_X;
	int y_axis = pPlot->AxIdx_Y;
	GpCoordinate * p_this = (pPlot->points) + first_point;
	for(int i = 0; i < num_points; i++) {
		const double x = p_this[i].x;
		const double y = p_this[i].y;
		p_this[i].type = INRANGE;
		// Overkill.  All we really want to do is update the x and y range 
		store_and_update_range(&p_this[i].x, x, &p_this[i].type, &AxS.__X(), AxS.__X().autoscale);
		store_and_update_range(&p_this[i].y, y, &p_this[i].type, &AxS.__Y(), AxS.__Y().autoscale);
		p_this[i].xlow = p_this[i].xhigh = p_this[i].x;
		p_this[i].ylow = p_this[i].yhigh = p_this[i].y;
		p_this[i].z = -1;
	}
}
//
// Frequency plots have don't need new points allocated; we just need
// to adjust the plot ranges. Wedging this into gen_interp() would
// make that code even harder to read.
//
//void gen_interp_frequency(curve_points * pPlot)
void GnuPlot::GenInterpFrequency(curve_points * pPlot)
{
	int i, j;
	int first_point, num_points;
	double y;
	double y_total = 0.0;
	const int curves = num_curves(pPlot);
	if(oneof2(pPlot->plot_smooth, SMOOTH_FREQUENCY_NORMALISED, SMOOTH_CUMULATIVE_NORMALISED)) {
		first_point = 0;
		for(i = 0; i < curves; i++) {
			num_points = next_curve(pPlot, &first_point);
			for(j = first_point; j < first_point + num_points; j++) {
				if(pPlot->points[j].type == UNDEFINED)
					continue;
				y_total += pPlot->points[j].y;
			}
			first_point += num_points + 1;
		}
	}
	first_point = 0;
	for(i = 0; i < curves; i++) {
		num_points = next_curve(pPlot, &first_point);
		// If cumulative, replace the current y-value with the
		// sum of all previous y-values. This assumes that the
		// data has already been sorted by x-values. 
		if(pPlot->plot_smooth == SMOOTH_CUMULATIVE) {
			y = 0;
			for(j = first_point; j < first_point + num_points; j++) {
				if(pPlot->points[j].type == UNDEFINED)
					continue;
				y += pPlot->points[j].y;
				pPlot->points[j].y = y;
			}
		}
		/* Alternatively, cumulative normalised means replace the
		   current y-value with the sum of all previous y-values
		   divided by the total sum of all values.  This assumes the
		   data is sorted as before.  Normalising in this way allows
		   comparison of the CDF of data sets with differing total
		   numbers of samples.  */
		if(pPlot->plot_smooth == SMOOTH_CUMULATIVE_NORMALISED) {
			y = 0;
			for(j = first_point; j < first_point + num_points; j++) {
				if(pPlot->points[j].type == UNDEFINED)
					continue;
				y += pPlot->points[j].y;
				pPlot->points[j].y = y / y_total;
			}
		}
		// Finally, normalized frequency smoothing means that we take our
		// existing histogram and divide each value by the total 
		if(pPlot->plot_smooth == SMOOTH_FREQUENCY_NORMALISED) {
			for(j = first_point; j < first_point + num_points; j++) {
				if(pPlot->points[j].type == UNDEFINED)
					continue;
				pPlot->points[j].y /= y_total;
			}
		}
		DoFreq(pPlot, first_point, num_points);
		first_point += num_points + 1;
	}
}
// 
// This is the shared entry point used for the original smoothing options
// csplines acsplines bezier sbezier
// 
//void gen_interp(curve_points * pPlot)
void GnuPlot::GenInterp(curve_points * pPlot)
{
	GpSplineCoeff * sc = NULL;
	GpSplineCoeff * sc2 = NULL;
	double * bc;
	int i;
	int num_points;
	int curves = num_curves(pPlot);
	GpCoordinate * new_points = (GpCoordinate *)SAlloc::M((Gg.Samples1 + 1) * curves * sizeof(GpCoordinate));
	int first_point = 0;
	for(i = 0; i < curves; i++) {
		num_points = next_curve(pPlot, &first_point);
		switch(pPlot->plot_smooth) {
			case SMOOTH_CSPLINES:
			    // 0 and 1 signify x and y, the first two dimensions in GpCoordinate 
			    // for FILLEDCURVES_BETWEEN we do it again for x and yhigh 
			    sc = CpTriDiag(&pPlot->points[first_point], num_points, 0, 1);
			    if(pPlot->plot_style == FILLEDCURVES && oneof3(pPlot->filledcurves_options.closeto, FILLEDCURVES_BETWEEN, FILLEDCURVES_ABOVE, FILLEDCURVES_BELOW))
				    sc2 = CpTriDiag(&pPlot->points[first_point], num_points, 0, 4);
			    DoCubic(pPlot, sc, sc2, first_point, num_points,
				new_points + i * (Gg.Samples1 + 1));
			    SAlloc::F(sc);
			    SAlloc::F(sc2);
			    break;
			case SMOOTH_ACSPLINES:
			    // 0 = control axis x,  1 = spline on y,  2 = weights held in z 
			    sc = CpApproxSpline(&pPlot->points[first_point], num_points, 0, 1, 2);
			    if(pPlot->plot_style == FILLEDCURVES && oneof3(pPlot->filledcurves_options.closeto, FILLEDCURVES_BETWEEN, FILLEDCURVES_ABOVE, FILLEDCURVES_BELOW))
				    sc2 = CpApproxSpline(&pPlot->points[first_point], num_points, 0, 4, 2);
			    DoCubic(pPlot, sc, sc2, first_point, num_points,
				new_points + i * (Gg.Samples1 + 1));
			    SAlloc::F(sc);
			    SAlloc::F(sc2);
			    break;
			case SMOOTH_BEZIER:
			case SMOOTH_SBEZIER:
			    bc = cp_binomial(num_points);
			    DoBezier(pPlot, bc, first_point, num_points,
				new_points + i * (Gg.Samples1 + 1));
			    SAlloc::F((char *)bc);
			    break;
			case SMOOTH_KDENSITY:
			    DoKDensity(pPlot, first_point, num_points,
				new_points + i * (Gg.Samples1 + 1));
			    break;
			default: // keep gcc -Wall quiet 
			    ;
		}
		new_points[(i + 1) * (Gg.Samples1 + 1)-1].type = UNDEFINED;
		first_point += num_points;
	}
	SAlloc::F(pPlot->points);
	pPlot->points = new_points;
	pPlot->p_max = curves * (Gg.Samples1 + 1);
	pPlot->p_count = pPlot->p_max - 1;
}
// 
// sort_points
// 
static int compare_points(SORTFUNC_ARGS arg1, SORTFUNC_ARGS arg2)
{
	const GpCoordinate * p1 = (const GpCoordinate *)arg1;
	const GpCoordinate * p2 = (const GpCoordinate *)arg2;
	if(p1->x > p2->x)
		return 1;
	if(p1->x < p2->x)
		return -1;
	return 0;
}

static int compare_z(SORTFUNC_ARGS arg1, SORTFUNC_ARGS arg2)
{
	const GpCoordinate * p1 = (const GpCoordinate *)arg1;
	const GpCoordinate * p2 = (const GpCoordinate *)arg2;
	if(p1->z > p2->z)
		return 1;
	if(p1->z < p2->z)
		return -1;
	return 0;
}

void sort_points(curve_points * plot)
{
	int num_points;
	int first_point = 0;
	while((num_points = next_curve(plot, &first_point)) > 0) {
		// Sort this set of points, does qsort handle 1 point correctly? 
		qsort(plot->points + first_point, num_points, sizeof(GpCoordinate), compare_points);
		first_point += num_points;
	}
}
/*
 * Sort on z rather than x
 * used by "smooth zsort"
 */
void zsort_points(curve_points * plot)
{
	int i, first_point, num_points;
	/* save variable color into GpCoordinate */
	if(plot->varcolor) {
		for(i = 0; i < plot->p_count; i++)
			plot->points[i].CRD_COLOR = plot->varcolor[i];
	}

	first_point = 0;
	while((num_points = next_curve(plot, &first_point)) > 0) {
		qsort(plot->points + first_point, num_points,
		    sizeof(GpCoordinate), compare_z);
		first_point += num_points;
	}

	/* restore variable color */
	if(plot->varcolor) {
		for(i = 0; i < plot->p_count; i++)
			plot->varcolor[i] = plot->points[i].CRD_COLOR;
	}
	return;
}
// 
// cp_implode() if averaging is selected this function computes the new
//   entries and shortens the whole thing to the necessary size
// MGR Addendum
// 
//void cp_implode(curve_points * cp)
void GnuPlot::CpImplode(curve_points * pCp)
{
	int first_point, num_points;
	int i, j, k;
	double x = 0.0;
	double y = 0.0;
	double sux = 0.0;
	double slx = 0.0;
	double suy = 0.0;
	double sly = 0.0;
	double weight = 1.0; /* used for acsplines */
	bool all_inrange = FALSE;
	AxS.Idx_X = pCp->AxIdx_X;
	AxS.Idx_Y = pCp->AxIdx_Y;
	j = 0;
	first_point = 0;
	while((num_points = next_curve(pCp, &first_point)) > 0) {
		bool last_point = FALSE;
		k = 0;
		for(i = first_point; i <= first_point + num_points; i++) {
			if(i == first_point + num_points) {
				if(k == 0)
					break;
				last_point = TRUE;
			}
			if(!last_point && pCp->points[i].type == UNDEFINED)
				continue;
			if(k == 0) {
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
			else if(!last_point && pCp->points[i].x == x) {
				y += pCp->points[i].y;
				sux += pCp->points[i].xhigh;
				slx += pCp->points[i].xlow;
				suy += pCp->points[i].yhigh;
				sly += pCp->points[i].ylow;
				weight += pCp->points[i].z;
				if(pCp->points[i].type != INRANGE)
					all_inrange = FALSE;
				k++;
			}
			else {
				pCp->points[j].x = x;
				if(oneof4(pCp->plot_smooth, SMOOTH_FREQUENCY, SMOOTH_FREQUENCY_NORMALISED, SMOOTH_CUMULATIVE, SMOOTH_CUMULATIVE_NORMALISED))
					k = 1;
				pCp->points[j].y = y /= (double)k;
				pCp->points[j].xhigh = sux / (double)k;
				pCp->points[j].xlow = slx / (double)k;
				pCp->points[j].yhigh = suy / (double)k;
				pCp->points[j].ylow = sly / (double)k;
				pCp->points[j].z = weight / (double)k;
				// HBB 20000405: I wanted to use STORE_AND_FIXUP_RANGE here,
				// but won't: it assumes we want to modify the range, and
				// that the range is given in 'input' coordinates.
				pCp->points[j].type = INRANGE;
				if(!all_inrange) {
					if(((x < AxS.__X().min) && !(AxS.__X().autoscale & AUTOSCALE_MIN))
					    || ((x > AxS.__X().max) && !(AxS.__X().autoscale & AUTOSCALE_MAX))
					    || ((y < AxS.__Y().min) && !(AxS.__Y().autoscale & AUTOSCALE_MIN))
					    || ((y > AxS.__Y().max) && !(AxS.__Y().autoscale & AUTOSCALE_MAX)))
						pCp->points[j].type = OUTRANGE;
				}
				j++; // next valid entry 
				k = 0; // to read 
				i--; // from this (-> last after for(;;)) entry 
			}
		}
		// FIXME: Monotonic cubic splines support only a single curve per data set 
		if(j < pCp->p_count && pCp->plot_smooth == SMOOTH_MONOTONE_CSPLINE)
			break;
		// insert invalid point to separate curves 
		if(j < pCp->p_count) {
			pCp->points[j].type = UNDEFINED;
			j++;
		}
		first_point += num_points;
	}                       /* end while */
	pCp->p_count = j;
	cp_extend(pCp, j);
}
// 
// EAM December 2013
// monotonic cubic spline using the Fritsch-Carlson algorithm
// FN Fritsch & RE Carlson (1980). "Monotone Piecewise Cubic Interpolation".
// SIAM Journal on Numerical Analysis (SIAM) 17 (2): 238–246. doi:10.1137/0717021.
// 
//void mcs_interp(curve_points * pPlot)
void GnuPlot::McsInterp(curve_points * pPlot)
{
	// These track the original (pre-sorted) data points 
	int N = pPlot->p_count;
	GpCoordinate * p = (GpCoordinate *)SAlloc::R(pPlot->points, (N+1) * sizeof(GpCoordinate));
	int i;
	// These will track the resulting smoothed curve (>= 3X original count) 
	// Larger number of samples gives smoother curve (no surprise!) 
	int Nsamp = (Gg.Samples1 > 2*N) ? Gg.Samples1 : 2*N;
	int Ntot = N + Nsamp;
	GpCoordinate * new_points = (GpCoordinate *)SAlloc::M((Ntot) * sizeof(GpCoordinate));
	double xstart = MAX(p[0].x, AxS.__X().min);
	double xend = MIN(p[N-1].x, AxS.__X().max);
	double xstep = (xend - xstart) / (Nsamp - 1);
	// Load output x coords for sampling 
	for(i = 0; i<N; i++)
		new_points[i].x = p[i].x;
	for(; i < Ntot; i++)
		new_points[i].x = xstart + (i-N)*xstep;
	// Sort output x coords 
	qsort(new_points, Ntot, sizeof(GpCoordinate), compare_points);
	// Displace any collisions 
	for(i = 1; i<Ntot-1; i++) {
		double delta = new_points[i].x - new_points[i-1].x;
		if(new_points[i+1].x - new_points[i].x < delta/1000.0)
			new_points[i].x -= delta/2.0;
	}
	// Calculate spline coefficients 
#define DX      xlow
#define SLOPE   xhigh
#define C1      ylow
#define C2      yhigh
#define C3      z
	for(i = 0; i < N-1; i++) {
		p[i].DX = p[i+1].x - p[i].x;
		p[i].SLOPE = (p[i+1].y - p[i].y) / p[i].DX;
	}
	// The SIAM paper only mentions setting the final slope to zero if the
	// calculation is otherwise ill-behaved (how would one detect that?).
	// Retaining the data-derived slope makes the handling at the two ends
	// of the data range consistent. See Bug #2055
	// p[N-1].SLOPE = 0; 
	p[N-1].SLOPE = p[N-2].SLOPE;
	p[0].C1 = p[0].SLOPE;
	for(i = 0; i < N-1; i++) {
		if(p[i].SLOPE * p[i+1].SLOPE <= 0) {
			p[i+1].C1 = 0;
		}
		else {
			double sum = p[i].DX + p[i+1].DX;
			p[i+1].C1 = (3.0 * sum) / ((sum + p[i+1].DX) /  p[i].SLOPE + (sum + p[i].DX) /  p[i+1].SLOPE);
		}
	}
	p[N].C1 = p[N-1].SLOPE;
	for(i = 0; i < N; i++) {
		double temp = p[i].C1 + p[i+1].C1 - 2*p[i].SLOPE;
		p[i].C2 = (p[i].SLOPE - p[i].C1 -temp) / p[i].DX;
		p[i].C3 = temp / (p[i].DX * p[i].DX);
	}
	// Use the coefficients C1, C2, C3 to interpolate over the requested range 
	for(i = 0; i < Ntot; i++) {
		double x = new_points[i].x;
		double y;
		bool exact = FALSE;
		if(x == p[N-1].x) { /* Exact value for right-most point of original data */
			y = p[N-1].y;
			exact = TRUE;
		}
		else {
			int low = 0;
			int mid;
			int high = N-1;
			while(low <= high) {
				mid = ffloori((low + high) / 2);
				if(p[mid].x < x)
					low = mid + 1;
				else if(p[mid].x > x)
					high = mid - 1;
				else { /* Exact value for some point in original data */
					y = p[mid].y;
					exact = TRUE;
					break;
				}
			}
			if(!exact) {
				int j = MAX(0, high);
				double diff = x - p[j].x;
				y = p[j].y + p[j].C1 * diff + p[j].C2 * diff * diff + p[j].C3 * diff * diff * diff;
			}
		}
		xstart = AxS.__X().min;
		xend = AxS.__X().max;
		if(inrange(x, xstart, xend))
			new_points[i].type = INRANGE;
		else
			new_points[i].type = OUTRANGE;
		// FIXME:  simpler test for outrange would be sufficient 
		AxS.Idx_Y = pPlot->AxIdx_Y;
		store_and_update_range(&new_points[i].y, y, &new_points[i].type, &AxS.__Y(), pPlot->noautoscale);
	}
	// Replace original data with the interpolated curve 
	SAlloc::F(p);
	pPlot->points = new_points;
	pPlot->p_count = Ntot;
	pPlot->p_max = Ntot + 1;
#undef DX
#undef SLOPE
#undef C1
#undef C2
#undef C3
}
// 
// Binned histogram of input values.
// 
// plot FOO using N:(1) bins{=<nbins>} {binrange=[binlow:binhigh]}
// {binwidth=<width>} with boxes
// 
// If no binrange is given, binlow and binhigh are taken from the x range of the data.
// In either of these cases binlow is the midpoint x-coordinate of the first bin
// and binhigh is the midpoint x-coordinate of the last bin.
// Points that lie exactly on a bin boundary are assigned to the upper bin.
// Bin assignments are not affected by "set xrange".
// Notes:
//   binwidth = (binhigh-binlow) / (nbins-1)
//   xmin = binlow - binwidth/2
//   xmax = binhigh + binwidth/2
// first bin holds points with (xmin =< x < xmin + binwidth)
// last bin holds points with (xmax-binwidth =< x < binhigh + binwidth)
// 
//void make_bins(curve_points * pPlot, int nbins, double binlow, double binhigh, double binwidth)
void GnuPlot::MakeBins(curve_points * pPlot, int nbins, double binlow, double binhigh, double binwidth)
{
	int i, binno;
	double * bin;
	double bottom, top, range;
	GpAxis * xaxis = &AxS[pPlot->AxIdx_X];
	GpAxis * yaxis = &AxS[pPlot->AxIdx_Y];
	double ymax = 0;
	int N = pPlot->p_count;
	// Find the range of points to be binned 
	if(binlow != binhigh) {
		// Explicit binrange [min:max] in the plot command 
		bottom = binlow;
		top = binhigh;
	}
	else {
		// Take binrange from the data itself 
		bottom = VERYLARGE; top = -VERYLARGE;
		for(i = 0; i < N; i++) {
			const coordval _cx = pPlot->points[i].x;
			SETMIN(bottom, _cx);
			SETMAX(top,    _cx);
		}
		if(top <= bottom)
			IntWarn(NO_CARET, "invalid bin range [%g:%g]", bottom, top);
	}
	// If a fixed binwidth was provided, find total number of bins 
	if(binwidth > 0) {
		nbins = static_cast<int>(1 + (top - bottom) / binwidth);
		double temp = nbins * binwidth - (top - bottom);
		bottom -= temp/2.0;
		top += temp/2.0;
	}
	else { // otherwise we use (N-1) intervals between midpoints of bin 1 and bin N 
		binwidth = (top - bottom) / (nbins - 1);
		bottom -= binwidth/2.;
		top += binwidth/2.;
	}
	range = top - bottom;
	FPRINTF((stderr, "make_bins: %d bins from %g to %g, binwidth %g\n", nbins, bottom, top, binwidth));
	bin = (double *)SAlloc::M(nbins*sizeof(double));
	for(i = 0; i < nbins; i++)
		bin[i] = 0;
	for(i = 0; i < N; i++) {
		if(pPlot->points[i].type != UNDEFINED) {
			binno = ffloori(nbins * (pPlot->points[i].x - bottom) / range);
			if(0 <= binno && binno < nbins)
				bin[binno] += pPlot->points[i].y;
		}
	}
	if(xaxis->autoscale & AUTOSCALE_MIN) {
		SETMIN(xaxis->min, bottom);
	}
	if(xaxis->autoscale & AUTOSCALE_MAX) {
		SETMAX(xaxis->max, top);
	}
	/* Replace the original data with one entry per bin.
	 * new x = midpoint of bin
	 * new y = number of points in the bin
	 */
	pPlot->p_count = nbins;
	pPlot->points = (GpCoordinate *)SAlloc::R(pPlot->points, nbins * sizeof(GpCoordinate));
	for(i = 0; i < nbins; i++) {
		double bincent = bottom + (0.5 + (double)i) * binwidth;
		pPlot->points[i].type = INRANGE;
		pPlot->points[i].x     = bincent;
		pPlot->points[i].xlow  = bincent - binwidth/2.;
		pPlot->points[i].xhigh = bincent + binwidth/2.;
		pPlot->points[i].y     = bin[i];
		pPlot->points[i].ylow  = pPlot->points[i].y;
		pPlot->points[i].yhigh = pPlot->points[i].y;
		pPlot->points[i].z = 0; /* FIXME: leave it alone? */
		if(xaxis->InRange(bincent)) {
			SETMAX(ymax, bin[i]);
		}
		else {
			pPlot->points[i].type = OUTRANGE;
		}
		FPRINTF((stderr, "bin[%d] %g %g\n", i, pPlot->points[i].x, pPlot->points[i].y));
	}
	if(yaxis->autoscale & AUTOSCALE_MIN) {
		SETMIN(yaxis->min, 0.0);
	}
	if(yaxis->autoscale & AUTOSCALE_MAX) {
		SETMAX(yaxis->max, ymax);
	}
	// Recheck range on y 
	for(i = 0; i<nbins; i++)
		if(!yaxis->InRange(pPlot->points[i].y))
			pPlot->points[i].type = OUTRANGE;
	SAlloc::F(bin); // Clean up 
}
/*
 * spline approximation of 3D lines
 */

// 
// Replace one isocurve with a 3D natural cubic spline interpolation.
// If there are multiple isocurves, or multiple curves with isocurves,
// the caller must sort that out and call here separately for each one.
// TODO: number of spline samples should be independent of "set samples"
// 
//static void do_3d_cubic(iso_curve * pCurve, enum PLOT_SMOOTH smoothOption)
void GnuPlot::Do3DCubic(iso_curve * pCurve, enum PLOT_SMOOTH smoothOption)
{
	int i, l;
	int nseg = Gg.Samples1;
	GpCoordinate * new_points;
	double xrange, yrange, zrange;
	double dx, dy, dz;
	double maxdx, maxdy, maxdz;
	double t, tsum, tstep;
	GpSplineCoeff * sc_x = NULL;
	GpSplineCoeff * sc_y = NULL;
	GpSplineCoeff * sc_z = NULL;
	GpCoordinate * old_points = pCurve->points;
	/*
	 * Sanity check axis ranges.
	 * This catches curves that lie in a plane of constant x or y.
	 * The fixup prints a warning to the user but we don't see it here.
	 */
	AxisCheckedExtendEmptyRange(FIRST_X_AXIS, "at time of spline generation");
	AxisCheckedExtendEmptyRange(FIRST_Y_AXIS, "at time of spline generation");
	// prevent gross mismatch of x/y/z units 
	xrange = fabs(AxS[FIRST_X_AXIS].GetRange());
	yrange = fabs(AxS[FIRST_Y_AXIS].GetRange());
	zrange = fabs(AxS[FIRST_Z_AXIS].GetRange());
	// Construct path-length vector; store it in unused slot of old_points 
	t = tsum = 0.0;
	maxdx = maxdy = maxdz = 0.0;
	old_points[0].CRD_PATH = 0;
	for(i = 1; i < pCurve->p_count; i++) {
		dx = (old_points[i].x - old_points[i-1].x) / xrange;
		dy = (old_points[i].y - old_points[i-1].y) / yrange;
		dz = (old_points[i].z - old_points[i-1].z) / zrange;
		tsum += sqrt(dx*dx + dy*dy + dz*dz);
		old_points[i].CRD_PATH = tsum;
		/* Track planarity */
		if(fabs(dx) > maxdx)
			maxdx = fabs(dx);
		if(fabs(dy) > maxdy)
			maxdy = fabs(dy);
		if(fabs(dz) > maxdz)
			maxdz = fabs(dz);
	}
	/* Normalize so that the path always runs from 0 to 1 */
	for(i = 1; i < pCurve->p_count; i++)
		old_points[i].CRD_PATH /= tsum;
	tstep = old_points[pCurve->p_count-1].CRD_PATH / (double)(nseg - 1);
	/* Create new list to hold interpolated points */
	new_points = (GpCoordinate *)SAlloc::M((nseg+1) * sizeof(GpCoordinate));
	memzero(new_points, (nseg+1) * sizeof(GpCoordinate));
	/*
	 * If the curve being fitted lies entirely in one plane,
	 * we can do better by fitting a 2D spline rather than a 3D spline.
	 * This benefits the relatively common case of drawing a stack of
	 * 2D plots (e.g. fence plots).
	 * First check for a curve lying in the yz plane (x = constant).
	 */
	if(maxdx < FLT_EPSILON) {
		tstep = (old_points[pCurve->p_count-1].y - old_points[0].y) / (double)(nseg - 1);
		if(smoothOption == SMOOTH_ACSPLINES)
			sc_z = CpApproxSpline(pCurve->points, pCurve->p_count, 1, 2, 3);
		else
			sc_z = CpTriDiag(pCurve->points, pCurve->p_count, 1, 2);
		for(i = 0, l = 0; i < nseg; i++) {
			double temp;
			t = old_points[0].y + i * tstep;
			// Move forward to the spline interval this point is in 
			while((t >= old_points[l + 1].y) && (l < pCurve->p_count- 2))
				l++;
			temp = t - old_points[l].y;
			new_points[i].x = old_points[l].x; /* All the same */
			new_points[i].y = t;
			new_points[i].z = ((sc_z[l][3] * temp + sc_z[l][2]) * temp + sc_z[l][1]) * temp + sc_z[l][0];
		}
	}
	/*
	 * Check for a curve lying in the xz plane (y = constant).
	 */
	else if(maxdy < FLT_EPSILON) {
		tstep = (old_points[pCurve->p_count-1].x - old_points[0].x) / (double)(nseg - 1);
		if(smoothOption == SMOOTH_ACSPLINES)
			sc_z = CpApproxSpline(pCurve->points, pCurve->p_count, 0, 2, 3);
		else
			sc_z = CpTriDiag(pCurve->points, pCurve->p_count, 0, 2);
		for(i = 0, l = 0; i < nseg; i++) {
			double temp;
			t = old_points[0].x + i * tstep;
			// Move forward to the spline interval this point is in 
			while((t >= old_points[l + 1].x) && (l < pCurve->p_count- 2))
				l++;
			temp = t - old_points[l].x;
			new_points[i].x = t;
			new_points[i].y = old_points[l].y; /* All the same */
			new_points[i].z = ((sc_z[l][3] * temp + sc_z[l][2]) * temp + sc_z[l][1]) * temp + sc_z[l][0];
		}
	}
	/*
	 * Check for a curve lying in the xy plane (z = constant).
	 */
	else if(maxdz < FLT_EPSILON) {
		tstep = (old_points[pCurve->p_count-1].x - old_points[0].x) / (double)(nseg - 1);
		if(smoothOption == SMOOTH_ACSPLINES)
			sc_y = CpApproxSpline(pCurve->points, pCurve->p_count, 0, 1, 3);
		else
			sc_y = CpTriDiag(pCurve->points, pCurve->p_count, 0, 1);
		for(i = 0, l = 0; i < nseg; i++) {
			double temp;
			t = old_points[0].x + i * tstep;
			// Move forward to the spline interval this point is in 
			while((t >= old_points[l + 1].x) && (l < pCurve->p_count- 2))
				l++;
			temp = t - old_points[l].x;
			new_points[i].x = t;
			new_points[i].y = ((sc_y[l][3] * temp + sc_y[l][2]) * temp + sc_y[l][1]) * temp + sc_y[l][0];
			new_points[i].z = old_points[l].z; /* All the same */
		}
	}
	// 
	// This is the general case.
	// Calculate spline coefficients for each dimension x, y, z
	// 
	else {
		if(smoothOption == SMOOTH_ACSPLINES) {
			sc_x = CpApproxSpline(pCurve->points, pCurve->p_count, PATHCOORD, 0, 3);
			sc_y = CpApproxSpline(pCurve->points, pCurve->p_count, PATHCOORD, 1, 3);
			sc_z = CpApproxSpline(pCurve->points, pCurve->p_count, PATHCOORD, 2, 3);
		}
		else {
			sc_x = CpTriDiag(pCurve->points, pCurve->p_count, PATHCOORD, 0);
			sc_y = CpTriDiag(pCurve->points, pCurve->p_count, PATHCOORD, 1);
			sc_z = CpTriDiag(pCurve->points, pCurve->p_count, PATHCOORD, 2);
		}
		for(i = 0, l = 0; i < nseg; i++) {
			double temp;
			t = i * tstep;
			// Move forward to the spline interval this point is in 
			while((t >= old_points[l + 1].CRD_PATH) && (l < pCurve->p_count- 2))
				l++;
			temp = t - old_points[l].CRD_PATH;
			new_points[i].x = ((sc_x[l][3] * temp + sc_x[l][2]) * temp + sc_x[l][1]) * temp + sc_x[l][0];
			new_points[i].y = ((sc_y[l][3] * temp + sc_y[l][2]) * temp + sc_y[l][1]) * temp + sc_y[l][0];
			new_points[i].z = ((sc_z[l][3] * temp + sc_z[l][2]) * temp + sc_z[l][1]) * temp + sc_z[l][0];
		}
	}
	// We're done with the spline coefficients 
	SAlloc::F(sc_x);
	SAlloc::F(sc_y);
	SAlloc::F(sc_z);
	// Replace original data with spline approximation 
	SAlloc::F(pCurve->points);
	pCurve->points = new_points;
	pCurve->p_count = nseg;
	pCurve->p_max = nseg+1;  /* not sure why we asked for 1 extra */
}
/*
 * Generate 2D splines along a path for each set of points in the plot,
 * smoothing option SMOOTH_PATH.
 * TODO:
 * - number of spline samples should be controlled by something other
 *   than "set samples"
 * - spline weights from an additional column
 */
//void gen_2d_path_splines(curve_points * pPlot)
void GnuPlot::Gen2DPathSplines(curve_points * pPlot)
{
	int i;
	int ic, first_point;    /* indexes for original data */
	int is = 0;             /* index for new (splined) data */
	GpCoordinate * old_points = NULL;
	GpCoordinate * splined_points;
	GpSplineCoeff * sc_x = NULL;
	GpSplineCoeff * sc_y = NULL;
	const double xrange = fabs(AxS[pPlot->AxIdx_X].GetRange());
	const double yrange = fabs(AxS[pPlot->AxIdx_Y].GetRange());
	const int curves = num_curves(pPlot);
	// Allocate space to hold the interpolated points 
	splined_points = (GpCoordinate *)SAlloc::M((Gg.Samples1 * curves) * sizeof(GpCoordinate));
	memzero(splined_points, (Gg.Samples1 * curves) * sizeof(GpCoordinate));
	first_point = 0;
	for(ic = 0; ic < curves; ic++) {
		double t, tstep, tsum;
		double dx, dy;
		int l;
		int nold;
		int num_points = next_curve(pPlot, &first_point);
		bool closed = FALSE;
		// Make a copy of the original points so that we don't corrupt the
		// list by adding up to three new ones.
		old_points = (GpCoordinate *)SAlloc::R(old_points, (num_points + 3) * sizeof(GpCoordinate));
		memcpy(&old_points[1], &pPlot->points[first_point], num_points * sizeof(GpCoordinate));
		// Remove any unusable points (NaN, missing, duplicates) before fitting a spline.
		// If that leaves fewer than 3 points, skip it.
		nold = do_curve_cleanup(&old_points[1], num_points);
		if(nold < 3) {
			first_point += num_points;
			continue;
		}
		// We expect one of two cases. Either this really is a closed
		// curve (end point matches start point) or it is an open-ended
		// path that may not be monotonic on x.
		// For plot style "with filledcurves closed" we add an extra
		// point at the end if it is not already there.
		if(old_points[1].x == old_points[nold].x && old_points[1].y == old_points[nold].y)
			closed = TRUE;
		if((pPlot->plot_style == FILLEDCURVES) && !closed) {
			old_points[++nold] = old_points[1];
			closed = TRUE;
		}
		if(closed) {
			// Wrap around to one point before and one point after the path closure 
			nold += 2;
			old_points[0] = old_points[nold-3];
			old_points[nold-1] = old_points[2];
		}
		else {
			/* Dummy up an extension at either end */
			nold += 2;
			old_points[0].x = old_points[1].x + old_points[1].x - old_points[2].x;
			old_points[nold-1].x = old_points[nold-2].x + old_points[nold-2].x - old_points[nold-3].x;
			old_points[0].y = old_points[1].y + old_points[1].y - old_points[2].y;
			old_points[nold-1].y = old_points[nold-2].y + old_points[nold-2].y - old_points[nold-3].y;
		}

		/* Construct path-length vector; store it in unused slot of old_points */
		t = tsum = 0.0;
		old_points[0].CRD_PATH = 0;
		for(i = 1; i < nold; i++) {
			dx = (old_points[i].x - old_points[i-1].x) / xrange;
			dy = (old_points[i].y - old_points[i-1].y) / yrange;
			tsum += sqrt(dx*dx + dy*dy);
			old_points[i].CRD_PATH = tsum;
		}
		// Normalize so that the path fraction always runs from 0 to 1 
		for(i = 1; i < nold; i++)
			old_points[i].CRD_PATH /= tsum;
		tstep = 1.0 / (double)(Gg.Samples1 - 1);
		// Calculate spline coefficients for x and for y as a function of path 
		sc_x = CpTriDiag(old_points, nold, PATHCOORD, 0);
		sc_y = CpTriDiag(old_points, nold, PATHCOORD, 1);
		// First output point is the same as the original first point 
		splined_points[is++] = old_points[1];
		// Skip the points in the overlap region 
		for(i = 0; i * tstep < old_points[1].CRD_PATH; i++)
			;
		// Use spline coefficients to generate a new point at each sample interval. 
		for(l = 0; i < Gg.Samples1; i++) {
			double temp;
			t = i * tstep;
			/* Stop before wrapping around. Copy the original end point. */
			if(t > old_points[nold-2].CRD_PATH) {
				splined_points[is++] = old_points[nold-2];
				break;
			}
			/* Move forward to the spline interval this point is in */
			while((t >= old_points[l + 1].CRD_PATH) && (l < nold- 2))
				l++;
			temp = t - old_points[l].CRD_PATH;
			splined_points[is].x = ((sc_x[l][3] * temp + sc_x[l][2]) * temp + sc_x[l][1]) * temp + sc_x[l][0];
			splined_points[is].y = ((sc_y[l][3] * temp + sc_y[l][2]) * temp + sc_y[l][1]) * temp + sc_y[l][0];
			is++;
		}
		/* Done with spline coefficients */
		SAlloc::F(sc_x);
		SAlloc::F(sc_y);
		/* Add a seperator point after this set of splined points */
		splined_points[is++].type = UNDEFINED;
		first_point += num_points;
	}
	// Replace original data with splined approximation 
	SAlloc::F(old_points);
	SAlloc::F(pPlot->points);
	pPlot->points = splined_points;
	pPlot->p_max = curves * Gg.Samples1;
	pPlot->p_count = is;
}
//
// Externally callable interface to 3D spline routines
//
//void gen_3d_splines(GpSurfacePoints * pPlot)
void GnuPlot::Gen3DSplines(GpSurfacePoints * pPlot)
{
	for(iso_curve * curve = pPlot->iso_crvs; curve; curve = curve->next) {
		// Remove any unusable points before fitting a spline 
		curve->p_count = do_curve_cleanup(curve->points, curve->p_count);
		if(curve->p_count > 3)
			Do3DCubic(curve, pPlot->plot_smooth);
	}
}

static int do_curve_cleanup(GpCoordinate * point, int npoints)
{
	// Step through points in curve keeping only the usable ones. Discard duplicates
	int keep = 0;
	for(int i = 0; i < npoints; i++) {
		if(point[i].type == UNDEFINED)
			continue;
		if(isnan(point[i].x) || isnan(point[i].y) || isnan(point[i].z))
			continue;
		if(i != keep)
			point[keep] = point[i];
		// FIXME: should probably check fabs(this-prev) < EPS 
		if(keep > 0 && (point[keep].x == point[keep-1].x) && (point[keep].y == point[keep-1].y) && (point[keep].z == point[keep-1].z))
			continue;
		keep++;
	}
	return keep;
}
