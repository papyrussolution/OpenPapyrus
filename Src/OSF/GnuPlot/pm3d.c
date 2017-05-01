/* GNUPLOT - pm3d.c */

/*[
 *
 * Petr Mikulik, since December 1998
 * Copyright: open source as much as possible
 *
 * What is here: global variables and routines for the pm3d splotting mode.
 * This file is included only if PM3D is defined.
 *
   ]*/
#include <gnuplot.h>
#pragma hdrstop

/*
   Global options for pm3d algorithm (to be accessed by set / show).
 */

//pm3d_struct pm3d;
//lp_style_type pm3d_border_lp;
//lighting_model pm3d_shade;

#if 0 // {
= {
	"s",                    /* where[6] */
	PM3D_FLUSH_BEGIN,       /* flush */
	0,                      /* no flushing triangles */
	PM3D_SCANS_AUTOMATIC,   /* scans direction is determined automatically */
	PM3D_CLIP_4IN,          /* clipping: all 4 points in ranges */
	PM3D_EXPLICIT,          /* implicit */
	PM3D_WHICHCORNER_MEAN,  /* color from which corner(s) */
	1,                      /* interpolate along scanline */
	1,                      /* interpolate between scanlines */
	DEFAULT_LP_STYLE_TYPE   /* for the border */
};
#endif // } 0

struct quadrangle {
	double gray;
	double z; /* maximal z value after rotation to graph GpCoordinate system */
	gpdPoint corners[4];
#ifdef EXTENDED_COLOR_SPECS
	gpiPoint icorners[4];
#endif
	t_colorspec * border_color; /* Only used by depthorder processing */
};

// Used to initialize `set pm3d border` 
// lp_style_type default_pm3d_border = DEFAULT_LP_STYLE_TYPE;
// Used by routine filled_quadrangle() in color.c 
static int allocated_quadrangles = 0;
static int current_quadrangle = 0;
static quadrangle * quadrangles = (quadrangle*)0;
static bool plot_has_palette;
static bool color_from_rgbvar = false;
static double light[3];

// Internal prototypes for this module 
static double geomean4(double, double, double, double);
static double harmean4(double, double, double, double);
static double median4(double, double, double, double);
static double rms4(double, double, double, double);
//static void pm3d_plot(SurfacePoints *, int);
static void pm3d_option_at_error();
//static void pm3d_rearrange_part(iso_curve *, const int, iso_curve ***, int *);
//static int apply_lighting_model( GpCoordinate *, GpCoordinate *, GpCoordinate *, GpCoordinate *, double gray );
/*
 * Utility routines.
 */

/* Geometrical mean = pow( prod(x_i > 0) x_i, 1/N )
 * In order to facilitate plotting surfaces with all color coords negative,
 * All 4 corners positive - return positive geometric mean
 * All 4 corners negative - return negative geometric mean
 * otherwise return 0
 * EAM Oct 2012: This is a CHANGE
 */
static double geomean4(double x1, double x2, double x3, double x4)
{
	const int neg = (x1 < 0) + (x2 < 0) + (x3 < 0) + (x4 < 0);
	double product = x1 * x2 * x3 * x4;
	if(product == 0) 
		return 0;
	else if(oneof3(neg, 1, 2, 3))
		return 0;
	else {
		product = sqrt(sqrt(fabs(product)));
		return (neg == 0) ? product : -product;
	}
}

static double harmean4(double x1, double x2, double x3, double x4)
{
	return (x1 <= 0 || x2 <= 0 || x3 <= 0 || x4 <= 0) ? not_a_number() : (4 / ((1/x1) + (1/x2) + (1/x3) + (1/x4)));
}

/* Median: sort values, and then: for N odd, it is the middle value; for N even,
 * it is mean of the two middle values.
 */
static double median4(double x1, double x2, double x3, double x4)
{
	// sort them: x1 < x2 and x3 < x4 
	if(x1 > x2)
		Exchange(&x1, &x2);
	if(x3 > x4)
		Exchange(&x3, &x4);
	// sum middle numbers 
	double tmp = (x1 < x3) ? x3 : x1;
	tmp += (x2 < x4) ? x2 : x4;
	return tmp * 0.5;
}

/* Minimum of 4 numbers.
 */
static double minimum4(double x1, double x2, double x3, double x4)
{
	x1 = MIN(x1, x2);
	x3 = MIN(x3, x4);
	return MIN(x1, x3);
}

/* Maximum of 4 numbers.
 */
static double maximum4(double x1, double x2, double x3, double x4)
{
	x1 = MAX(x1, x2);
	x3 = MAX(x3, x4);
	return MAX(x1, x3);
}

/* The root mean square of the 4 numbers */
static double rms4(double x1, double x2, double x3, double x4)
{
	return 0.5*sqrt(x1*x1 + x2*x2 + x3*x3 + x4*x4);
}

/*
 * Now the routines which are really just those for pm3d.c
 */

//
// Rescale z to cb values. Nothing to do if both z and cb are linear or log of the
// same base, other it has to un-log z and subsequently log it again.
//
//double _z2cb(double z)
double GpGadgets::Z2CB(double z)
{
	if(!(GetZ().Flags & GpAxis::fLog) && !(GetCB().Flags & GpAxis::fLog)) // both are linear 
		return z;
	else if(GetZ().Flags & GpAxis::fLog && !(GetCB().Flags & GpAxis::fLog)) // log z, linear cb 
		return exp(z * GetZ().log_base);  // unlog(z) 
	else if(!(GetZ().Flags & GpAxis::fLog) && GetCB().Flags & GpAxis::fLog) // linear z, log cb 
		return (z<=0) ? GetCB().Range.low : (log(z) / GetCB().log_base);
	else {
		// both are log
		if(GetZ().base == GetCB().base) // can we compare double numbers like that? 
			return z;
		else
			return z * GetZ().log_base / GetCB().log_base; // log_cb(unlog_z(z)) 
	}
}
//
// Rescale cb (color) value into the interval of grays [0,1], taking care
// of palette being positive or negative.
// Note that it is OK for logarithmic cb-axis too.
//
//double _cb2gray(double cb)
double GpGadgets::CB2Gray(double cb)
{
	GpAxis & r_cb_ax = GetCB();
	if(cb <= r_cb_ax.Range.low)
		return (SmPalette.positive == SMPAL_POSITIVE) ? 0 : 1;
	else if(cb >= r_cb_ax.Range.upp)
		return (SmPalette.positive == SMPAL_POSITIVE) ? 1 : 0;
	else {
#ifdef NONLINEAR_AXES
		if(r_cb_ax.P_LinkToPrmr) {
			r_cb_ax = *r_cb_ax.P_LinkToPrmr;
			cb = r_cb_ax.EvalLinkFunction(cb);
		}
#endif
		cb = fscale(cb, r_cb_ax.Range.low, r_cb_ax.Range.upp);
		return (SmPalette.positive == SMPAL_POSITIVE) ? cb : 1-cb;
	}
}
/*
 * Rearrange...
 */
//static void pm3d_rearrange_part(iso_curve * src, const int len, iso_curve *** dest, int * invert)
void GpGadgets::Pm3DRearRangePart(iso_curve * pSrc, const int len, iso_curve *** pppDest, int * pInvert)
{
	iso_curve * scanA;
	iso_curve * scanB;
	int i, scan;
	int invert_order = 0;
	/* loop over scans in one surface
	   Scans are linked from this_plot->iso_crvs in the opposite order than
	   they are in the datafile.
	   Therefore it is necessary to make vector scan_array of iso_curves.
	   Scans are sorted in scan_array according to pm3d.direction (this can
	   be PM3D_SCANS_FORWARD or PM3D_SCANS_BACKWARD).
	 */
	iso_curve ** scan_array = (iso_curve **)malloc(len * sizeof(scanA));
	*pppDest = scan_array;
	if(Pm3D.direction == PM3D_SCANS_AUTOMATIC) {
		int cnt;
		int len2 = len;
		bool exit_outer_loop = 0;
		for(scanA = pSrc; scanA && 0 == exit_outer_loop; scanA = scanA->next, len2--) {
			if((cnt = scanA->p_count - 1) > 0) {
				int    from, i;
				GpVertex vA, vA2;
				// ordering within one scan 
				for(from = 0; from<=cnt; from++) { // find 1st non-undefined point
					if(scanA->points[from].type != UNDEFINED) {
						Map3DXYZ(scanA->points[from].x, scanA->points[from].y, 0, &vA);
						break;
					}
				}
				for(i = cnt; i>from; i--) { // find the last non-undefined point 
					if(scanA->points[i].type != UNDEFINED) {
						Map3DXYZ(scanA->points[i].x, scanA->points[i].y, 0, &vA2);
						break;
					}
				}
				if((i - from) > (cnt * 0.1)) {
					// it is completely arbitrary to request at least 10% valid samples in this scan. (joze Jun-05-2002) 
					*pInvert = (vA2.z > vA.z) ? 0 : 1;
					// check the z ordering between scans
					// Find last scan. If this scan has all points undefined,
					// find last but one scan, an so on. 
					for(; len2 >= 3 && !exit_outer_loop; len2--) {
						for(scanB = scanA->next, i = len2 - 2; i && scanB; i--)
							scanB = scanB->next;  // skip over to last scan
						if(scanB && scanB->p_count) {
							GpVertex vB;
							for(i = from /* we compare vA.z with vB.z */; i<scanB->p_count; i++) {
								// find 1st non-undefined point
								if(scanB->points[i].type != UNDEFINED) {
									Map3DXYZ(scanB->points[i].x, scanB->points[i].y, 0, &vB);
									invert_order = (vB.z > vA.z) ? 0 : 1;
									exit_outer_loop = 1;
									break;
								}
							}
						}
					}
				}
				else {
					; // all points were undefined, so check next scan
				}
			}
		}
	}
	FPRINTF((stderr, "(pm3d_rearrange_part) invert       = %d\n", *invert));
	FPRINTF((stderr, "(pm3d_rearrange_part) invert_order = %d\n", invert_order));
	for(scanA = pSrc, scan = len - 1, i = 0; scan >= 0; --scan, i++) {
		if(Pm3D.direction == PM3D_SCANS_AUTOMATIC) {
			switch(invert_order) {
				case 1:
				    scan_array[scan] = scanA;
				    break;
				case 0:
				default:
				    scan_array[i] = scanA;
				    break;
			}
		}
		else if(Pm3D.direction == PM3D_SCANS_FORWARD)
			scan_array[scan] = scanA;
		else            // PM3D_SCANS_BACKWARD: i counts scans
			scan_array[i] = scanA;
		scanA = scanA->next;
	}
}

/*
 * Rearrange scan array
 *
 * Allocates *first_ptr (and eventually *second_ptr)
 * which must be freed by the caller
 */
void pm3d_rearrange_scan_array(SurfacePoints * this_plot,
    iso_curve *** first_ptr, int * first_n, int * first_invert,
    iso_curve *** second_ptr, int * second_n, int * second_invert)
{
	if(first_ptr) {
		GpGg.Pm3DRearRangePart(this_plot->iso_crvs, this_plot->num_iso_read, first_ptr, first_invert);
		*first_n = this_plot->num_iso_read;
	}
	if(second_ptr) {
		iso_curve * icrvs = this_plot->iso_crvs;
		iso_curve * icrvs2;
		int i;
		// advance until second part 
		for(i = 0; i < this_plot->num_iso_read; i++)
			icrvs = icrvs->next;
		// count the number of scans of second part 
		for(i = 0, icrvs2 = icrvs; icrvs2; icrvs2 = icrvs2->next)
			i++;
		if(i > 0) {
			*second_n = i;
			GpGg.Pm3DRearRangePart(icrvs, i, second_ptr, second_invert);
		}
		else
			*second_ptr = (iso_curve**)0;
	}
}

static int compare_quadrangles(const void* v1, const void* v2)
{
	const quadrangle* q1 = (const quadrangle*)v1;
	const quadrangle* q2 = (const quadrangle*)v2;
	if(q1->z > q2->z)
		return 1;
	else if(q1->z < q2->z)
		return -1;
	else
		return 0;
}

void pm3d_depth_queue_clear(void)
{
	if(GpGg.Pm3D.direction == PM3D_DEPTH) {
		ZFREE(quadrangles);
		allocated_quadrangles = 0;
		current_quadrangle = 0;
	}
}

//void pm3d_depth_queue_flush()
void GpGadgets::Pm3DDepthQueueFlush(GpTermEntry * pT)
{
	if(Pm3D.direction == PM3D_DEPTH) {
		if(current_quadrangle > 0 && quadrangles) {
			quadrangle* qp;
			quadrangle* qe;
			gpdPoint* gpdPtr;
#ifdef EXTENDED_COLOR_SPECS
			gpiPoint* gpiPtr;
			double w = trans_mat[3][3];
#endif
			GpVertex out;
			double z = 0; /* assignment keeps the compiler happy */
			int i;
			for(qp = quadrangles, qe = quadrangles + current_quadrangle; qp != qe; qp++) {
				gpdPtr = qp->corners;
#ifdef EXTENDED_COLOR_SPECS
				gpiPtr = qp->icorners;
#endif
				for(i = 0; i < 4; i++, gpdPtr++) {
					Map3DXYZ(gpdPtr->x, gpdPtr->y, gpdPtr->z, &out);
					if(i == 0 || out.z > z)
						z = out.z;
#ifdef EXTENDED_COLOR_SPECS
					gpiPtr->x = (uint)((out.x * xscaler / w) + xmiddle);
					gpiPtr->y = (uint)((out.y * yscaler / w) + ymiddle);
					gpiPtr++;
#endif
				}
				qp->z = z; /* maximal z value of all four corners */
			}
			qsort(quadrangles, current_quadrangle, sizeof(quadrangle), compare_quadrangles);
			for(qp = quadrangles, qe = quadrangles + current_quadrangle; qp != qe; qp++) {
				// set the color 
				if(color_from_rgbvar || Pm3DShade.strength > 0)
					SetRgbColorVar(pT, (uint)qp->gray);
				else
					pT->SetColor(qp->gray);
#ifdef EXTENDED_COLOR_SPECS
				IFilledQuadrangle(pT, qp->icorners);
#else
				FilledQuadrangle(pT, qp->corners);
#endif
			}
		}
		pm3d_depth_queue_clear();
	}
}
//
// Now the implementation of the pm3d (s)plotting mode
//
//static void pm3d_plot(SurfacePoints * pPlot, int at_which_z)
void GpGadgets::Pm3DPlot(GpTermEntry * pT, SurfacePoints * pPlot, int atWhichZ)
{
	int j, i, i1, ii, ii1, from, scan, up_to, up_to_minus, invert = 0;
	int go_over_pts, max_pts;
	int are_ftriangles, ftriangles_low_pt = -999, ftriangles_high_pt = -999;
	iso_curve * scanA, * scanB;
	GpCoordinate * pointsA, * pointsB;
	iso_curve ** scan_array;
	int scan_array_n;
	double avgC, gray = 0;
	double cb1, cb2, cb3, cb4;
	gpdPoint corners[4];
	int interp_i, interp_j;
#ifdef EXTENDED_COLOR_SPECS
	gpiPoint icorners[4];
#endif
	gpdPoint ** bl_point = NULL; /* used for bilinear interpolation */
	/* just a shortcut */
	bool color_from_column = pPlot->pm3d_color_from_column;
	color_from_rgbvar = (pPlot->lp_properties.pm3d_color.type == TC_RGB &&  pPlot->lp_properties.pm3d_color.value == -1);
	if(pPlot == NULL)
		return;
	// Apply and save the user-requested line properties
	Pm3DBorderLp = pPlot->lp_properties;
	ApplyLpProperties(pT, &Pm3DBorderLp);
	if(atWhichZ != PM3D_AT_BASE && atWhichZ != PM3D_AT_TOP && atWhichZ != PM3D_AT_SURFACE)
		return;
	// return if the terminal does not support filled polygons
	if(!pT->filled_polygon)
		return;
	switch(atWhichZ) {
		case PM3D_AT_BASE:
		    corners[0].z = corners[1].z = corners[2].z = corners[3].z = base_z;
		    break;
		case PM3D_AT_TOP:
		    corners[0].z = corners[1].z = corners[2].z = corners[3].z = ceiling_z;
		    break;
		    // the 3rd possibility is surface, PM3D_AT_SURFACE, coded below
	}
	scanA = pPlot->iso_crvs;
	pm3d_rearrange_scan_array(pPlot, &scan_array, &scan_array_n, &invert, (iso_curve***)0, (int*)0, (int*)0);
	interp_i = Pm3D.interp_i;
	interp_j = Pm3D.interp_j;
	if(interp_i <= 0 || interp_j <= 0) {
		/* Number of interpolations will be determined from desired number of points.
		   Search for number of scans and maximal number of points in a scan for points
		   which will be plotted (INRANGE). Then set interp_i,j so that number of points
		   will be a bit larger than |interp_i,j|.
		   If (interp_i,j==0) => set this number of points according to DEFAULT_OPTIMAL_NB_POINTS.
		   Ideally this should be comparable to the resulution of the output device, which
		   can hardly by done at this high level instead of the driver level.
		 */
	#define DEFAULT_OPTIMAL_NB_POINTS 200
		int max_scan_pts = 0;
		int max_scans = 0;
		int pts;
		for(scan = 0; scan < pPlot->num_iso_read - 1; scan++) {
			scanA = scan_array[scan];
			pointsA = scanA->points;
			pts = 0;
			for(j = 0; j<scanA->p_count; j++)
				if(pointsA[j].type == INRANGE) 
					pts++;
			if(pts > 0) {
				max_scan_pts = MAX(max_scan_pts, pts);
				max_scans++;
			}
		}
		if(max_scan_pts == 0 || max_scans == 0)
			GpGg.IntErrorNoCaret("all scans empty");
		if(interp_i <= 0) {
			ii = (interp_i == 0) ? DEFAULT_OPTIMAL_NB_POINTS : -interp_i;
			// @sobolev interp_i = floor(ii / max_scan_pts) + 1;
			interp_i = (ii / max_scan_pts) + 1; // @sobolev
		}
		if(interp_j <= 0) {
			ii = (interp_j == 0) ? DEFAULT_OPTIMAL_NB_POINTS : -interp_j;
			// @sobolev interp_j = floor(ii / max_scans) + 1;
			interp_j = (ii / max_scans) + 1; // @sobolev
		}
#if 0
		fprintf(stderr, "pm3d.interp_i=%i\t pm3d.interp_j=%i\n", pm3d.interp_i, pm3d.interp_j);
		fprintf(stderr, "INRANGE: max_scans=%i  max_scan_pts=%i\n", max_scans, max_scan_pts);
		fprintf(stderr, "seting interp_i=%i\t interp_j=%i => there will be %i and %i points\n", interp_i, interp_j, interp_i*max_scan_pts, interp_j*max_scans);
#endif
	}
	if(Pm3D.direction == PM3D_DEPTH) {
		for(scan = 0; scan < pPlot->num_iso_read - 1; scan++) {
			scanA = scan_array[scan];
			scanB = scan_array[scan + 1];
			are_ftriangles = Pm3D.ftriangles && (scanA->p_count != scanB->p_count);
			if(!are_ftriangles)
				allocated_quadrangles += MIN(scanA->p_count, scanB->p_count) - 1;
			else {
				allocated_quadrangles += MAX(scanA->p_count, scanB->p_count) - 1;
			}
		}
		allocated_quadrangles *= (interp_i > 1) ? interp_i : 1;
		allocated_quadrangles *= (interp_j > 1) ? interp_j : 1;
		quadrangles = (quadrangle*)gp_realloc(quadrangles, allocated_quadrangles * sizeof(quadrangle), "pm3d_plot->quadrangles");
		/* DEBUG: fprintf(stderr, "allocated_quadrangles = %d\n", allocated_quadrangles); */
	}
	/* pm3d_rearrange_scan_array(pPlot, (iso_curve***)0, (int*)0, &scan_array, &invert); */

#if 0
	/* debugging: print scan_array */
	for(scan = 0; scan < pPlot->num_iso_read; scan++) {
		printf("**** SCAN=%d  points=%d\n", scan, scan_array[scan]->p_count);
	}
#endif
#if 0
	/* debugging: this loop prints properties of all scans */
	for(scan = 0; scan < pPlot->num_iso_read; scan++) {
		GpCoordinate  points;
		scanA = scan_array[scan];
		printf("\n#IsoCurve = scan nb %d, %d points\n#x y z type(in,out,undef)\n", scan, scanA->p_count);
		for(i = 0, points = scanA->points; i < scanA->p_count; i++) {
			printf("%g %g %g %c\n", points[i].x, points[i].y, points[i].z, points[i].type == INRANGE ? 'i' : points[i].type == OUTRANGE ? 'o' : 'u');
			/* Note: INRANGE, OUTRANGE, UNDEFINED */
		}
	}
	printf("\n");
#endif
	/*
	 * if bilinear interpolation is enabled, allocate memory for the
	 * interpolated points here
	 */
	if(interp_i > 1 || interp_j > 1) {
		bl_point = (gpdPoint**)malloc(sizeof(gpdPoint*) * (interp_i+1));
		for(i1 = 0; i1 <= interp_i; i1++)
			bl_point[i1] = (gpdPoint*)malloc(sizeof(gpdPoint) * (interp_j+1));
	}
	/*
	 * this loop does the pm3d draw of joining two curves
	 *
	 * How the loop below works:
	 * - scanB = scan last read; scanA = the previous one
	 * - link the scan from A to B, then move B to A, then read B, then draw
	 */
	for(scan = 0; scan < pPlot->num_iso_read - 1; scan++) {
		scanA = scan_array[scan];
		scanB = scan_array[scan + 1];
		FPRINTF((stderr, "\n#IsoCurveA = scan nb %d has %d points   ScanB has %d points\n", scan, scanA->p_count, scanB->p_count));
		pointsA = scanA->points;
		pointsB = scanB->points;
		// if the number of points in both scans is not the same, then the
		// starting index (offset) of scan B according to the flushing setting has to be determined
		from = 0; // default is pm3d.flush==PM3D_FLUSH_BEGIN 
		if(Pm3D.flush == PM3D_FLUSH_END)
			from = abs(scanA->p_count - scanB->p_count);
		else if(Pm3D.flush == PM3D_FLUSH_CENTER)
			from = abs(scanA->p_count - scanB->p_count) / 2;
		// find the minimal number of points in both scans 
		up_to = MIN(scanA->p_count, scanB->p_count) - 1;
		up_to_minus = up_to - 1; // calculate only once 
		are_ftriangles = Pm3D.ftriangles && (scanA->p_count != scanB->p_count);
		if(!are_ftriangles)
			go_over_pts = up_to;
		else {
			max_pts = MAX(scanA->p_count, scanB->p_count);
			go_over_pts = max_pts - 1;
			/* the j-subrange of quadrangles; in the remaing of the interval
			 * [0..up_to] the flushing triangles are to be drawn */
			ftriangles_low_pt = from;
			ftriangles_high_pt = from + up_to_minus;
		}
		/* Go over
		 *   - the minimal number of points from both scans, if only quadrangles.
		 *   - the maximal number of points from both scans if flush triangles
		 *     (the missing points in the scan of lower nb of points will be
		 *     duplicated from the begin/end points).
		 *
		 * Notice: if it would be once necessary to go over points in `backward'
		 * direction, then the loop body below would require to replace the data
		 * point indices `i' by `up_to-i' and `i+1' by `up_to-i-1'.
		 */
		for(j = 0; j < go_over_pts; j++) {
			/* Now i be the index of the scan with smaller number of points,
			 * ii of the scan with larger number of points. */
			if(are_ftriangles && (j < ftriangles_low_pt || j > ftriangles_high_pt)) {
				i = (j <= ftriangles_low_pt) ? 0 : ftriangles_high_pt-from+1;
				ii = j;
				i1 = i;
				ii1 = ii + 1;
			}
			else {
				int jj = are_ftriangles ? j - from : j;
				i = jj;
				if(PM3D_SCANS_AUTOMATIC == Pm3D.direction && invert)
					i = up_to_minus - jj;
				ii = i + from;
				i1 = i + 1;
				ii1 = ii + 1;
			}
			/* From here, i is index to scan A, ii to scan B */
			if(scanA->p_count > scanB->p_count) {
				Exchange(&i, &ii);
				Exchange(&i1, &ii1);
			}
			FPRINTF((stderr, "j=%i:  i=%i i1=%i  [%i]   ii=%i ii1=%i  [%i]\n", j, i, i1, scanA->p_count, ii, ii1, scanB->p_count));
			// choose the clipping method 
			if(Pm3D.clip == PM3D_CLIP_4IN) {
				// (1) all 4 points of the quadrangle must be in x and y range 
				if(!(pointsA[i].type == INRANGE && pointsA[i1].type == INRANGE && pointsB[ii].type == INRANGE && pointsB[ii1].type == INRANGE))
					continue;
			}
			else {  /* (pm3d.clip == PM3D_CLIP_1IN) */
				/* (2) all 4 points of the quadrangle must be defined */
				if(pointsA[i].type == UNDEFINED || pointsA[i1].type == UNDEFINED || pointsB[ii].type == UNDEFINED || pointsB[ii1].type == UNDEFINED)
					continue;
				/* and at least 1 point of the quadrangle must be in x and y range */
				if(pointsA[i].type == OUTRANGE && pointsA[i1].type == OUTRANGE && pointsB[ii].type == OUTRANGE && pointsB[ii1].type == OUTRANGE)
					continue;
			}
			if((interp_i <= 1 && interp_j <= 1) || Pm3D.direction == PM3D_DEPTH) {
#ifdef EXTENDED_COLOR_SPECS
				if((pT->flags & TERM_EXTENDED_COLOR) == 0)
#endif
				{
					/* Get the gray as the average of the corner z- or gray-positions
					   (note: log scale is already included). The average is calculated here
					   if there is no interpolation (including the "pm3d depthorder" option),
					   otherwise it is done for each interpolated quadrangle later.
					 */
					if(color_from_column) {
						/* color is set in plot3d.c:get_3ddata() */
						cb1 = pointsA[i].CRD_COLOR;
						cb2 = pointsA[i1].CRD_COLOR;
						cb3 = pointsB[ii].CRD_COLOR;
						cb4 = pointsB[ii1].CRD_COLOR;
					}
					else {
						cb1 = Z2CB(pointsA[i].z);
						cb2 = Z2CB(pointsA[i1].z);
						cb3 = Z2CB(pointsB[ii].z);
						cb4 = Z2CB(pointsB[ii1].z);
					}
					/* Fancy averages of RGB color make no sense */
					if(color_from_rgbvar) {
						uint r, g, b, a;
						uint u1 = (uint)cb1;
						uint u2 = (uint)cb2;
						uint u3 = (uint)cb3;
						uint u4 = (uint)cb4;
						switch(Pm3D.which_corner_color) {
							default:
							    r = (u1&0xff0000) + (u2&0xff0000) + (u3&0xff0000) + (u4&0xff0000);
							    g = (u1&0xff00) + (u2&0xff00) + (u3&0xff00) + (u4&0xff00);
							    b = (u1&0xff) + (u2&0xff) + (u3&0xff) + (u4&0xff);
							    avgC = ((r>>2)&0xff0000) + ((g>>2)&0xff00) + ((b>>2)&0xff);
							    a = ((u1>>24)&0xff) + ((u2>>24)&0xff) + ((u3>>24)&0xff) + ((u4>>24)&0xff);
							    avgC += (a<<22)&0xff000000;
							    break;
							case PM3D_WHICHCORNER_C1: avgC = cb1; break;
							case PM3D_WHICHCORNER_C2: avgC = cb2; break;
							case PM3D_WHICHCORNER_C3: avgC = cb3; break;
							case PM3D_WHICHCORNER_C4: avgC = cb4; break;
						}

						/* But many different averages are possible for gray values */
					}
					else {
						switch(Pm3D.which_corner_color) {
							default:
							case PM3D_WHICHCORNER_MEAN: avgC = (cb1 + cb2 + cb3 + cb4) * 0.25; break;
							case PM3D_WHICHCORNER_GEOMEAN: avgC = geomean4(cb1, cb2, cb3, cb4); break;
							case PM3D_WHICHCORNER_HARMEAN: avgC = harmean4(cb1, cb2, cb3, cb4); break;
							case PM3D_WHICHCORNER_MEDIAN: avgC = median4(cb1, cb2, cb3, cb4); break;
							case PM3D_WHICHCORNER_MIN: avgC = minimum4(cb1, cb2, cb3, cb4); break;
							case PM3D_WHICHCORNER_MAX: avgC = maximum4(cb1, cb2, cb3, cb4); break;
							case PM3D_WHICHCORNER_RMS: avgC = rms4(cb1, cb2, cb3, cb4); break;
							case PM3D_WHICHCORNER_C1: avgC = cb1; break;
							case PM3D_WHICHCORNER_C2: avgC = cb2; break;
							case PM3D_WHICHCORNER_C3: avgC = cb3; break;
							case PM3D_WHICHCORNER_C4: avgC = cb4; break;
						}
					}
					/* The value is out of range, but we didn't figure it out until now */
					if(fisnan(avgC))
						continue;
					if(color_from_rgbvar) /* we were given an RGB color */
						gray = avgC;
					else /* transform z value to gray, i.e. to interval [0,1] */
						gray = CB2Gray(avgC);
					/* apply lighting model */
					if(Pm3DShade.strength > 0) {
						if(atWhichZ == PM3D_AT_SURFACE)
							gray = ApplyLightingModel(&pointsA[i], &pointsA[i1], &pointsB[ii], &pointsB[ii1], gray);
						/* Don't apply lighting model to TOP/BOTTOM projections  */
						/* but convert from floating point 0<gray<1 to RGB color */
						/* since that is what would have been returned from the  */
						/* lighting code.					     */
						else if(!color_from_rgbvar) {
							rgb255_color temp;
							rgb255maxcolors_from_gray(gray, &temp);
							gray = (long)((temp.r << 16) + (temp.g << 8) + (temp.b));
						}
					}
					// set the color 
					if(Pm3D.direction != PM3D_DEPTH) {
						if(color_from_rgbvar || Pm3DShade.strength > 0)
							SetRgbColorVar(pT, (uint)gray);
						else
							pT->SetColor(gray);
					}
				}
			}
			corners[0].x = pointsA[i].x;
			corners[0].y = pointsA[i].y;
			corners[1].x = pointsB[ii].x;
			corners[1].y = pointsB[ii].y;
			corners[2].x = pointsB[ii1].x;
			corners[2].y = pointsB[ii1].y;
			corners[3].x = pointsA[i1].x;
			corners[3].y = pointsA[i1].y;
			if(interp_i > 1 || interp_j > 1 || atWhichZ == PM3D_AT_SURFACE) {
				corners[0].z = pointsA[i].z;
				corners[1].z = pointsB[ii].z;
				corners[2].z = pointsB[ii1].z;
				corners[3].z = pointsA[i1].z;
				if(color_from_column) {
					corners[0].c = pointsA[i].CRD_COLOR;
					corners[1].c = pointsB[ii].CRD_COLOR;
					corners[2].c = pointsB[ii1].CRD_COLOR;
					corners[3].c = pointsA[i1].CRD_COLOR;
				}
			}
#ifdef EXTENDED_COLOR_SPECS
			if((pT->flags & TERM_EXTENDED_COLOR)) {
				if(color_from_column) {
					icorners[0].z = pointsA[i].CRD_COLOR;
					icorners[1].z = pointsB[ii].CRD_COLOR;
					icorners[2].z = pointsB[ii1].CRD_COLOR;
					icorners[3].z = pointsA[i1].CRD_COLOR;
				}
				else {
					/* the target wants z and gray value */
					icorners[0].z = pointsA[i].z;
					icorners[1].z = pointsB[ii].z;
					icorners[2].z = pointsB[ii1].z;
					icorners[3].z = pointsA[i1].z;
				}
				for(i = 0; i < 4; i++) {
					icorners[i].spec.gray = CB2Gray(color_from_column ? icorners[i].z : Z2CB(icorners[i].z) );
				}
			}
			if(pm3d.direction == PM3D_DEPTH) {
				/* copy quadrangle */
				quadrangle* qp = quadrangles + current_quadrangle;
				memcpy(qp->corners, corners, 4 * sizeof(gpdPoint));
				qp->gray = gray;
				for(i = 0; i < 4; i++) {
					qp->icorners[i].z = icorners[i].z;
					qp->icorners[i].spec.gray = icorners[i].spec.gray;
				}
				current_quadrangle++;
			}
			else
				FilledQuadrangle(pT, corners, icorners);
#else
			if(interp_i > 1 || interp_j > 1) {
				/* Interpolation is enabled.
				 * interp_i is the # of points along scan lines
				 * interp_j is the # of points between scan lines
				 * Algorithm is to first sample i points along the scan lines
				 * defined by corners[3],corners[0] and corners[2],corners[1]. */
				int j1;
				for(i1 = 0; i1 <= interp_i; i1++) {
					bl_point[i1][0].x = ((corners[3].x - corners[0].x) / interp_i) * i1 + corners[0].x;
					bl_point[i1][interp_j].x = ((corners[2].x - corners[1].x) / interp_i) * i1 + corners[1].x;
					bl_point[i1][0].y = ((corners[3].y - corners[0].y) / interp_i) * i1 + corners[0].y;
					bl_point[i1][interp_j].y = ((corners[2].y - corners[1].y) / interp_i) * i1 + corners[1].y;
					bl_point[i1][0].z = ((corners[3].z - corners[0].z) / interp_i) * i1 + corners[0].z;
					bl_point[i1][interp_j].z = ((corners[2].z - corners[1].z) / interp_i) * i1 + corners[1].z;
					if(color_from_column) {
						bl_point[i1][0].c = ((corners[3].c - corners[0].c) / interp_i) * i1 + corners[0].c;
						bl_point[i1][interp_j].c = ((corners[2].c - corners[1].c) / interp_i) * i1 + corners[1].c;
					}
					/* Next we sample j points between each of the new points
					 * created in the previous step (this samples between
					 * scan lines) in the same manner. */
					for(j1 = 1; j1 < interp_j; j1++) {
						bl_point[i1][j1].x = ((bl_point[i1][interp_j].x - bl_point[i1][0].x) / interp_j) * j1 + bl_point[i1][0].x;
						bl_point[i1][j1].y = ((bl_point[i1][interp_j].y - bl_point[i1][0].y) / interp_j) * j1 + bl_point[i1][0].y;
						bl_point[i1][j1].z = ((bl_point[i1][interp_j].z - bl_point[i1][0].z) / interp_j) * j1 + bl_point[i1][0].z;
						if(color_from_column)
							bl_point[i1][j1].c = ((bl_point[i1][interp_j].c - bl_point[i1][0].c) / interp_j) * j1 + bl_point[i1][0].c;
					}
				}
				/* Once all points are created, move them into an appropriate
				 * structure and call set_color on each to retrieve the
				 * correct color mapping for this new sub-sampled quadrangle. */
				for(i1 = 0; i1 < interp_i; i1++) {
					for(j1 = 0; j1 < interp_j; j1++) {
						corners[0].x = bl_point[i1][j1].x;
						corners[0].y = bl_point[i1][j1].y;
						corners[0].z = bl_point[i1][j1].z;
						corners[1].x = bl_point[i1+1][j1].x;
						corners[1].y = bl_point[i1+1][j1].y;
						corners[1].z = bl_point[i1+1][j1].z;
						corners[2].x = bl_point[i1+1][j1+1].x;
						corners[2].y = bl_point[i1+1][j1+1].y;
						corners[2].z = bl_point[i1+1][j1+1].z;
						corners[3].x = bl_point[i1][j1+1].x;
						corners[3].y = bl_point[i1][j1+1].y;
						corners[3].z = bl_point[i1][j1+1].z;
						if(color_from_column) {
							corners[0].c = bl_point[i1][j1].c;
							corners[1].c = bl_point[i1+1][j1].c;
							corners[2].c = bl_point[i1+1][j1+1].c;
							corners[3].c = bl_point[i1][j1+1].c;
						}
						FPRINTF((stderr, "(%g,%g),(%g,%g),(%g,%g),(%g,%g)\n", corners[0].x, corners[0].y, corners[1].x, corners[1].y, corners[2].x, corners[2].y, corners[3].x, corners[3].y));
						/* If the colors are given separately, we already loaded them above */
						if(color_from_column) {
							cb1 = corners[0].c;
							cb2 = corners[1].c;
							cb3 = corners[2].c;
							cb4 = corners[3].c;
						}
						else {
							cb1 = Z2CB(corners[0].z);
							cb2 = Z2CB(corners[1].z);
							cb3 = Z2CB(corners[2].z);
							cb4 = Z2CB(corners[3].z);
						}
						switch(Pm3D.which_corner_color) {
							default:
							case PM3D_WHICHCORNER_MEAN: avgC = (cb1 + cb2 + cb3 + cb4) * 0.25; break;
							case PM3D_WHICHCORNER_GEOMEAN: avgC = geomean4(cb1, cb2, cb3, cb4); break;
							case PM3D_WHICHCORNER_HARMEAN: avgC = harmean4(cb1, cb2, cb3, cb4); break;
							case PM3D_WHICHCORNER_MEDIAN: avgC = median4(cb1, cb2, cb3, cb4); break;
							case PM3D_WHICHCORNER_MIN: avgC = minimum4(cb1, cb2, cb3, cb4); break;
							case PM3D_WHICHCORNER_MAX: avgC = maximum4(cb1, cb2, cb3, cb4); break;
							case PM3D_WHICHCORNER_RMS: avgC = rms4(cb1, cb2, cb3, cb4); break;
							case PM3D_WHICHCORNER_C1: avgC = cb1; break;
							case PM3D_WHICHCORNER_C2: avgC = cb2; break;
							case PM3D_WHICHCORNER_C3: avgC = cb3; break;
							case PM3D_WHICHCORNER_C4: avgC = cb4; break;
						}
						if(color_from_rgbvar) // we were given an explicit color 
							gray = avgC;
						else // transform z value to gray, i.e. to interval [0,1] 
							gray = CB2Gray(avgC);
						// apply lighting model 
						if(Pm3DShade.strength > 0) {
							// FIXME: GpCoordinate->quadrangle->GpCoordinate seems crazy 
							GpCoordinate corcorners[4];
							for(int i = 0; i<4; i++) {
								corcorners[i].x = corners[i].x;
								corcorners[i].y = corners[i].y;
								corcorners[i].z = corners[i].z;
							}
							if(atWhichZ == PM3D_AT_SURFACE)
								gray = ApplyLightingModel(&corcorners[0], &corcorners[1], &corcorners[2], &corcorners[3], gray);
							/* Don't apply lighting model to TOP/BOTTOM projections  */
							/* but convert from floating point 0<gray<1 to RGB color */
							/* since that is what would have been returned from the  */
							/* lighting code.					     */
							else if(!color_from_rgbvar) {
								rgb255_color temp;
								rgb255maxcolors_from_gray(gray, &temp);
								gray = (long)((temp.r << 16) + (temp.g << 8) + (temp.b));
							}
						}
						if(Pm3D.direction == PM3D_DEPTH) {
							/* copy quadrangle */
							quadrangle* qp = quadrangles + current_quadrangle;
							memcpy(qp->corners, corners, 4 * sizeof(gpdPoint));
							qp->gray = gray;
							qp->border_color = &pPlot->lp_properties.pm3d_color;
							current_quadrangle++;
						}
						else {
							if(Pm3DShade.strength > 0 || color_from_rgbvar)
								SetRgbColorVar(pT, (uint)gray);
							else
								pT->SetColor(gray);
							if(atWhichZ == PM3D_AT_BASE)
								corners[0].z = corners[1].z = corners[2].z = corners[3].z = base_z;
							else if(atWhichZ == PM3D_AT_TOP)
								corners[0].z = corners[1].z = corners[2].z = corners[3].z = ceiling_z;
							FilledQuadrangle(pT, corners);
						}
					}
				}
			}
			else { // thus (interp_i == 1 && interp_j == 1) 
				if(Pm3D.direction != PM3D_DEPTH) {
					FilledQuadrangle(pT, corners);
				}
				else {
					// copy quadrangle 
					quadrangle* qp = quadrangles + current_quadrangle;
					memcpy(qp->corners, corners, 4 * sizeof(gpdPoint));
					qp->gray = gray;
					qp->border_color = &pPlot->lp_properties.pm3d_color;
					current_quadrangle++;
				}
			} /* interpolate between points */
#endif
		}
	}
	if(bl_point) {
		for(i1 = 0; i1 <= interp_i; i1++)
			free(bl_point[i1]);
		free(bl_point);
	}
	/* free memory allocated by scan_array */
	free(scan_array);
}

#ifdef PM3D_CONTOURS
//
// Now the implementation of the filled color contour plot
//
//static void filled_color_contour_plot(SurfacePoints * this_plot, int contours_where)
void GpGadgets::FilledColorContourPlot(GpTermEntry * pT, SurfacePoints * pPlot, int contoursWhere)
{
	double gray;
	// just a shortcut 
	bool color_from_column = pPlot->pm3d_color_from_column;
	if(pPlot == NULL || pPlot->contours == NULL)
		return;
	if(contoursWhere != CONTOUR_SRF && contoursWhere != CONTOUR_BASE)
		return;
	// return if the terminal does not support filled polygons 
	if(!pT->filled_polygon)
		return;
	// TODO: CHECK FOR NUMBER OF POINTS IN CONTOUR: IF TOO SMALL, THEN IGNORE! 
	gnuplot_contours * cntr = pPlot->contours;
	while(cntr) {
		printf("# Contour: points %i, z %g, label: %s\n", cntr->num_pts, cntr->coords[0].z, (cntr->label) ? cntr->label : "<no>");
		if(cntr->isNewLevel) {
			printf("\t...it isNewLevel\n");
			// contour split across chunks 
			// fprintf(gpoutfile, "\n# Contour %d, label: %s\n", number++, c->label); 
			// What is the color? 
			// get the z-GpCoordinate 
			// transform contour z-GpCoordinate value to gray, i.e. to interval [0,1] 
			gray = color_from_column ? CB2Gray(cntr->coords[0].CRD_COLOR) : CB2Gray(Z2CB(cntr->coords[0].z) );
			set_color(gray);
		}
		// draw one countour 
		if(contoursWhere == CONTOUR_SRF) // at CONTOUR_SRF 
			FilledPolygon3DCoords(pT, cntr->num_pts, cntr->coords);
		else // at CONTOUR_BASE 
			FilledPolygon3DCoords_ZFixed(pT, cntr->num_pts, cntr->coords, base_z);
		// next contour 
		cntr = cntr->next;
	}
}

#endif

/*
 * unset pm3d for the reset command
 */
void pm3d_reset()
{
	strcpy(GpGg.Pm3D.where, "s");
	GpGg.Pm3D.flush = PM3D_FLUSH_BEGIN;
	GpGg.Pm3D.ftriangles = 0;
	GpGg.Pm3D.direction = PM3D_SCANS_AUTOMATIC;
	GpGg.Pm3D.clip = PM3D_CLIP_4IN;
	GpGg.Pm3D.implicit = PM3D_EXPLICIT;
	GpGg.Pm3D.which_corner_color = PM3D_WHICHCORNER_MEAN;
	GpGg.Pm3D.interp_i = 1;
	GpGg.Pm3D.interp_j = 1;
	GpGg.Pm3D.border.l_type = LT_NODRAW;

	GpGg.Pm3DShade.strength = 0.0;
	GpGg.Pm3DShade.spec = 0.0;
	GpGg.Pm3DShade.fixed = true;
}
//
// Draw (one) PM3D color surface.
//
//void pm3d_draw_one(SurfacePoints * plot)
void GpGadgets::Pm3DDrawOne(GpTermEntry * pT, SurfacePoints * pPlot)
{
	int i = 0;
	char * where = pPlot->pm3d_where[0] ? pPlot->pm3d_where : Pm3D.where;
	// Draw either at 'where' option of the given surface or at pm3d.where global option. 
	if(where[0]) {
		// Initialize lighting model 
		if(Pm3DShade.strength > 0) {
			light[0] = cos(-DEG2RAD * Pm3DShade.rot_x)*cos(-(DEG2RAD * Pm3DShade.rot_z+90));
			light[2] = cos(-DEG2RAD * Pm3DShade.rot_x)*sin(-(DEG2RAD * Pm3DShade.rot_z+90));
			light[1] = sin(-DEG2RAD * Pm3DShade.rot_x);
		}
		// for pm3dCompress.awk 
		if(Pm3D.direction != PM3D_DEPTH)
			pT->_Layer(TERM_LAYER_BEGIN_PM3D_MAP);
		for(; where[i]; i++) {
			Pm3DPlot(pT, pPlot, where[i]);
		}
#ifdef PM3D_CONTOURS
		if(strchr(where, 'C') != NULL) {
			/* !!!!! FILLED COLOR CONTOURS, *UNDOCUMENTED*
			!!!!! LATER CHANGE TO STH LIKE
			!!!!!   (if_filled_contours_requested)
			!!!!!      ...
			Currently filled color contours do not work because gnuplot generates
			open contour lines, i.e. not closed on the graph boundary.
			*/
			if(draw_contour & CONTOUR_SRF)
				FilledColorContourPlot(pT, pPlot, CONTOUR_SRF);
			if(draw_contour & CONTOUR_BASE)
				FilledColorContourPlot(pT, pPlot, CONTOUR_BASE);
		}
#endif
		// for pm3dCompress.awk 
		if(Pm3D.direction != PM3D_DEPTH)
			pT->_Layer(TERM_LAYER_END_PM3D_MAP);
	}
}

/* Display an error message for the routine get_pm3d_at_option() below.
 */

static void pm3d_option_at_error()
{
	GpGg.IntErrorCurToken("parameter to `pm3d at` requires combination of up to 6 characters b,s,t\n\t(drawing at bottom, surface, top)");
}

/* Read the option for 'pm3d at' command.
 * Used by 'set pm3d at ...' or by 'splot ... with pm3d at ...'.
 * If no option given, then returns empty string, otherwise copied there.
 * The string is unchanged on error, and 1 is returned.
 * On success, 0 is returned.
 */
int get_pm3d_at_option(GpCommand & rC, char * pm3d_where)
{
	char * c;
	if(rC.EndOfCommand() || rC.P_Token[rC.CToken].length >= sizeof(GpGg.Pm3D.where)) {
		pm3d_option_at_error();
		return 1;
	}
	else {
		memcpy(pm3d_where, rC.P_InputLine + rC.P_Token[rC.CToken].start_index, rC.P_Token[rC.CToken].length);
		pm3d_where[rC.P_Token[rC.CToken].length] = 0;
		// verify the parameter 
		for(c = pm3d_where; *c; c++) {
			if(*c != 'C') // !!!!! CONTOURS, UNDOCUMENTED, THIS LINE IS TEMPORARILY HERE !!!!!
				if(*c != PM3D_AT_BASE && *c != PM3D_AT_TOP && *c != PM3D_AT_SURFACE) {
					pm3d_option_at_error();
					return 1;
				}
		}
		rC.CToken++;
		return 0;
	}
}
//
// Set flag plot_has_palette to true if there is any element on the graph
// which requires palette of continuous colors.
//
//void set_plot_with_palette(int plot_num, int plot_mode)
void GpGadgets::SetPlotWithPalette(int plot_num, int plot_mode)
{
	SurfacePoints * p_3dplot = P_First3DPlot;
	CurvePoints * p_2dplot = P_FirstPlot;
	int surface = 0;
	GpTextLabel * p_label = first_label;
	plot_has_palette = true;
	// Is pm3d switched on globally? 
	if(Pm3D.implicit != PM3D_IMPLICIT) {
		// Check 2D plots 
		if(plot_mode == MODE_PLOT) {
			while(p_2dplot) {
				if(p_2dplot->plot_style == IMAGE)
					return;
				if(p_2dplot->lp_properties.pm3d_color.UsesPalette())
					return;
				if(p_2dplot->labels && p_2dplot->labels->textcolor.UsesPalette())
					return;
				p_2dplot = p_2dplot->P_Next;
			}
		}
		// Check 3D plots 
		if(plot_mode == MODE_SPLOT) {
			// Any surface 'with pm3d', 'with image' or 'with line|dot palette'? 
			while(surface++ < plot_num) {
				int type;
				if(p_3dplot->plot_style == PM3DSURFACE)
					return;
				if(p_3dplot->plot_style == IMAGE)
					return;
				type = p_3dplot->lp_properties.pm3d_color.type;
				if(oneof3(type, TC_LT, TC_LINESTYLE, TC_RGB))
					; // don't return yet 
				else
					return; // TC_DEFAULT: splot x with line|lp|dot palette 
				if(p_3dplot->labels && p_3dplot->labels->textcolor.type >= TC_CB)
					return;
				p_3dplot = p_3dplot->next_sp;
			}
		}
		// Any label with 'textcolor palette'? 
		for(; p_label; p_label = p_label->next) {
			if(p_label->textcolor.UsesPalette())
				return;
		}
		// Any of title, xlabel, ylabel, zlabel, ... with 'textcolor palette'? 
		if(title.textcolor.UsesPalette()) 
			return;
		if(AxA[FIRST_X_AXIS].label.textcolor.UsesPalette()) 
			return;
		if(AxA[FIRST_Y_AXIS].label.textcolor.UsesPalette()) 
			return;
		if(AxA[SECOND_X_AXIS].label.textcolor.UsesPalette()) 
			return;
		if(AxA[SECOND_Y_AXIS].label.textcolor.UsesPalette()) 
			return;
		if(plot_mode == MODE_SPLOT)
			if(AxA[FIRST_Z_AXIS].label.textcolor.UsesPalette()) 
				return;
		if(AxA[COLOR_AXIS].label.textcolor.UsesPalette()) 
			return;
		// Palette with continuous colors is not used
		plot_has_palette = false; // otherwise it stays true 
	}
}

bool is_plot_with_palette()
{
	return plot_has_palette;
}

bool is_plot_with_colorbox()
{
	return plot_has_palette && (GpGg.ColorBox.where != SMCOLOR_BOX_NO);
}
//
// Adjust current RGB color based on pm3d lighting model.
//
//int apply_lighting_model(GpCoordinate * v0, GpCoordinate * v1, GpCoordinate * v2, GpCoordinate * v3, double gray)
int GpGadgets::ApplyLightingModel(GpCoordinate * pV0, GpCoordinate * pV1, GpCoordinate * pV2, GpCoordinate * pV3, double gray)
{
	double normal[3];
	double normal1[3];
	double reflect[3];
	double t;
	double phi;
	double psi;
	int rgb;
	rgb_color color;
	double r, g, b, tmp_r, tmp_g, tmp_b;
	double dot_prod, shade_fact, spec_fact;
	if(color_from_rgbvar) {
		rgb = (int)gray;
		r = (double)((rgb >> 16) & 0xFF) / 255.;
		g = (double)((rgb >>  8) & 0xFF) / 255.;
		b = (double)((rgb      ) & 0xFF) / 255.;
	}
	else {
		RGB1FromGray(gray, &color);
		r = color.R;
		g = color.G;
		b = color.B;
	}
	psi = -DEG2RAD*(surface_rot_z);
	phi = -DEG2RAD*(surface_rot_x);
	normal[0] = (pV1->y-pV0->y)*(pV2->z-pV0->z)*Scale3d.y*Scale3d.z - (pV1->z-pV0->z)*(pV2->y-pV0->y)*Scale3d.y*Scale3d.z;
	normal[1] = (pV1->z-pV0->z)*(pV2->x-pV0->x)*Scale3d.x*Scale3d.z - (pV1->x-pV0->x)*(pV2->z-pV0->z)*Scale3d.x*Scale3d.z;
	normal[2] = (pV1->x-pV0->x)*(pV2->y-pV0->y)*Scale3d.x*Scale3d.y - (pV1->y-pV0->y)*(pV2->x-pV0->x)*Scale3d.x*Scale3d.y;
	t = sqrt(normal[0]*normal[0] + normal[1]*normal[1] + normal[2]*normal[2]);
	normal[0] /= t;
	normal[1] /= t;
	normal[2] /= t;
	/* Correct for the view angle so that the illumination is "fixed" with */
	/* respect to the viewer rather than rotating with the surface.        */
	if(Pm3DShade.fixed) {
		normal1[0] =  cos(psi)*normal[0] -  sin(psi)*normal[1] + 0*normal[2];
		normal1[1] =  sin(psi)*normal[0] +  cos(psi)*normal[1] + 0*normal[2];
		normal1[2] =  0*normal[0] +                0*normal[1] + 1*normal[2];

		normal[0] =  1*normal1[0] +         0*normal1[1] +         0*normal1[2];
		normal[1] =  0*normal1[0] +  cos(phi)*normal1[1] -  sin(phi)*normal1[2];
		normal[2] =  0*normal1[0] +  sin(phi)*normal1[1] +  cos(phi)*normal1[2];
	}
	if(normal[2] < 0.0) {
		normal[0] *= -1.0;
		normal[1] *= -1.0;
		normal[2] *= -1.0;
	}
	dot_prod = normal[0]*light[0] + normal[1]*light[1] + normal[2]*light[2];
	shade_fact = (dot_prod < 0) ? -dot_prod : 0;
	tmp_r = r * (Pm3DShade.ambient - Pm3DShade.strength + shade_fact * Pm3DShade.strength);
	tmp_g = g * (Pm3DShade.ambient - Pm3DShade.strength + shade_fact * Pm3DShade.strength);
	tmp_b = b * (Pm3DShade.ambient - Pm3DShade.strength + shade_fact * Pm3DShade.strength);
	// Specular highlighting 
	if(Pm3DShade.spec > 0.0) {
		reflect[0] = -light[0]+2*dot_prod*normal[0];
		reflect[1] = -light[1]+2*dot_prod*normal[1];
		reflect[2] = -light[2]+2*dot_prod*normal[2];
		t = sqrt(reflect[0]*reflect[0] + reflect[1]*reflect[1] + reflect[2]*reflect[2]);
		reflect[0] /= t;
		reflect[1] /= t;
		reflect[2] /= t;
		dot_prod = -reflect[2];
		if(dot_prod < 0.0)
			dot_prod = 0;
		// old-style Phong equation; no need for bells or whistles 
		spec_fact = pow(dot_prod, Pm3DShade.Phong);
		tmp_r += Pm3DShade.spec*spec_fact;
		tmp_g += Pm3DShade.spec*spec_fact;
		tmp_b += Pm3DShade.spec*spec_fact;
	}
	tmp_r = clip_to_01(tmp_r);
	tmp_g = clip_to_01(tmp_g);
	tmp_b = clip_to_01(tmp_b);
	rgb = ((uchar)((tmp_r)*255.) << 16) + ((uchar)((tmp_g)*255.) <<  8) + ((uchar)((tmp_b)*255.));
	return rgb;
}

