/* GNUPLOT - plot3d.c */

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
#include <gnuplot.h>
#pragma hdrstop
//
// static prototypes
//
/*static void calculate_set_of_isolines(AXIS_INDEX value_axis, bool cross, iso_curve ** this_iso,
	    AXIS_INDEX iso_axis, double iso_min, double iso_step, int num_iso_to_use,
	    AXIS_INDEX sam_axis, double sam_min, double sam_step, int num_sam_to_use);*/
//static int get_3ddata(SurfacePoints* this_plot);
//static void eval_3dplots();
//static void grid_nongrid_data(SurfacePoints* this_plot);
static void parametric_3dfixup(SurfacePoints * start_plot, int * plot_num);
static SurfacePoints * sp_alloc(int num_samp_1, int num_iso_1, int num_samp_2, int num_iso_2);
static void sp_replace(SurfacePoints * sp, int num_samp_1, int num_iso_1, int num_samp_2, int num_iso_2);
//
// helper functions for grid_nongrid_data()
//
static double splines_kernel(double h);
static void thin_plate_splines_setup( iso_curve * old_iso_crvs, double ** p_xx, int * p_numpoints );
static double qnorm( double dist_x, double dist_y, int q );
static double pythag( double dx, double dy );

// helper functions for parsing
static void load_contour_label_options(GpTextLabel * contour_label);

/* HBB 20000508: moved these functions to the only module that uses them
 * so they can be turned 'static' */
/*
 * sp_alloc() allocates a SurfacePoints structure that can hold 'num_iso_1'
 * iso-curves with 'num_samp_2' samples and 'num_iso_2' iso-curves with
 * 'num_samp_1' samples.
 * If, however num_iso_2 or num_samp_1 is zero no iso curves are allocated.
 */
static SurfacePoints * sp_alloc(int num_samp_1, int num_iso_1, int num_samp_2, int num_iso_2)
{
	lp_style_type default_lp_properties; // = DEFAULT_LP_STYLE_TYPE;
	SurfacePoints * sp = (SurfacePoints *)malloc(sizeof(*sp));
	memzero(sp, sizeof(SurfacePoints));
	/* Initialize various fields */
	sp->lp_properties = default_lp_properties;
	default_arrow_style(&(sp->arrow_properties));
	if(num_iso_2 > 0 && num_samp_1 > 0) {
		int i;
		iso_curve * icrv;
		for(i = 0; i < num_iso_1; i++) {
			icrv = iso_alloc(num_samp_2);
			icrv->next = sp->iso_crvs;
			sp->iso_crvs = icrv;
		}
		for(i = 0; i < num_iso_2; i++) {
			icrv = iso_alloc(num_samp_1);
			icrv->next = sp->iso_crvs;
			sp->iso_crvs = icrv;
		}
	}
	return (sp);
}
//
// sp_replace() updates a SurfacePoints structure so it can hold 'num_iso_1'
// iso-curves with 'num_samp_2' samples and 'num_iso_2' iso-curves with
// 'num_samp_1' samples.
// If, however num_iso_2 or num_samp_1 is zero no iso curves are allocated.
//
static void sp_replace(SurfacePoints * sp, int num_samp_1, int num_iso_1, int num_samp_2, int num_iso_2)
{
	int i;
	iso_curve * icrv;
	iso_curve * icrvs = sp->iso_crvs;
	while(icrvs) {
		icrv = icrvs;
		icrvs = icrvs->next;
		iso_free(icrv);
	}
	sp->iso_crvs = NULL;
	if(num_iso_2 > 0 && num_samp_1 > 0) {
		for(i = 0; i < num_iso_1; i++) {
			icrv = iso_alloc(num_samp_2);
			icrv->next = sp->iso_crvs;
			sp->iso_crvs = icrv;
		}
		for(i = 0; i < num_iso_2; i++) {
			icrv = iso_alloc(num_samp_1);
			icrv->next = sp->iso_crvs;
			sp->iso_crvs = icrv;
		}
	}
	else
		sp->iso_crvs = NULL;
}

/*
 * sp_free() releases any memory which was previously malloc()'d to hold
 *   surface points.
 */
/* HBB 20000506: don't risk stack havoc by recursion, use iterative list
 * cleanup instead */
void sp_free(SurfacePoints * sp)
{
	while(sp) {
		SurfacePoints * next = sp->next_sp;
		free(sp->title);
		ZFREE(sp->title_position);
		while(sp->contours) {
			gnuplot_contours * next_cntrs = sp->contours->next;
			free(sp->contours->coords);
			free(sp->contours);
			sp->contours = next_cntrs;
		}
		while(sp->iso_crvs) {
			iso_curve * next_icrvs = sp->iso_crvs->next;
			iso_free(sp->iso_crvs);
			sp->iso_crvs = next_icrvs;
		}
		GpTextLabel::Destroy(sp->labels);
		sp->labels = 0;
		free(sp);
		sp = next;
	}
}

//void plot3drequest()
void GpGadgets::Plot3DRequest(GpCommand & rC)
//
// in the parametric case we would say splot [u= -Pi:Pi] [v= 0:2*Pi] [-1:1]
// [-1:1] [-1:1] sin(v)*cos(u),sin(v)*cos(u),sin(u) in the non-parametric
// case we would say only splot [x= -2:2] [y= -5:5] sin(x)*cos(y)
//
{
	int dummy_token0 = -1, dummy_token1 = -1;
	/*AXIS_INDEX*/int u_axis, v_axis;
	Is3DPlot = true;
	if(IsParametric && strcmp(rC.P.SetDummyVar[0], "t") == 0) {
		strcpy(rC.P.SetDummyVar[0], "u");
		strcpy(rC.P.SetDummyVar[1], "v");
	}
	// put stuff into arrays to simplify access */
	InitAxis(FIRST_X_AXIS, 0);
	InitAxis(FIRST_Y_AXIS, 0);
	InitAxis(FIRST_Z_AXIS, 1);
	InitAxis(U_AXIS, 0);
	InitAxis(V_AXIS, 0);
	InitAxis(COLOR_AXIS, 1);
	if(!term)               /* unknown */
		IntErrorCurToken("use 'set term' to set terminal type first");
	// Range limits for the entire plot are optional but must be given
	// in a fixed order. The keyword 'sample' terminates range parsing
	u_axis = (IsParametric ? U_AXIS : FIRST_X_AXIS);
	v_axis = (IsParametric ? V_AXIS : FIRST_Y_AXIS);
	dummy_token0 = ParseRange((AXIS_INDEX)u_axis, rC);
	dummy_token1 = ParseRange((AXIS_INDEX)v_axis, rC);
	if(IsParametric) {
		ParseRange(FIRST_X_AXIS, rC);
		ParseRange(FIRST_Y_AXIS, rC);
	}
	ParseRange(FIRST_Z_AXIS, rC);
	CheckAxisReversed(FIRST_X_AXIS);
	CheckAxisReversed(FIRST_Y_AXIS);
	CheckAxisReversed(FIRST_Z_AXIS);
	if(rC.Eq("sample") && rC.Eq(rC.CToken+1, "["))
		rC.CToken++;
	// Clear out any tick labels read from data files in previous plot
	for(u_axis = (AXIS_INDEX)0; u_axis<AXIS_ARRAY_SIZE; u_axis++) {
		t_ticdef * ticdef = &AxA[u_axis].ticdef;
		if(ticdef->def.user)
			ticdef->def.user = prune_dataticks(ticdef->def.user);
		if(!ticdef->def.user && ticdef->type == TIC_USER)
			ticdef->type = TIC_COMPUTED;
	}
	// use the default dummy variable unless changed
	if(dummy_token0 > 0)
		rC.CopyStr(rC.P.CDummyVar[0], dummy_token0, MAX_ID_LEN);
	else
		strcpy(rC.P.CDummyVar[0], rC.P.SetDummyVar[0]);
	if(dummy_token1 > 0)
		rC.CopyStr(rC.P.CDummyVar[1], dummy_token1, MAX_ID_LEN);
	else
		strcpy(rC.P.CDummyVar[1], rC.P.SetDummyVar[1]);
	// In "set view map" mode the x2 and y2 axes are legal
	// but must be linked to the respective primary axis.
	if(splot_map) {
		if((AxA[SECOND_X_AXIS].ticmode && !AxA[SECOND_X_AXIS].P_LinkToPrmr) || (AxA[SECOND_Y_AXIS].ticmode && !AxA[SECOND_Y_AXIS].P_LinkToPrmr))
			GpGg.IntErrorNoCaret("Secondary axis must be linked to primary axis in order to draw tics");
	}
	Eval3DPlots(rC);
}

/* Helper function for refresh command.  Reexamine each data point and update the
 * flags for INRANGE/OUTRANGE/UNDEFINED based on the current limits for that axis.
 * Normally the axis limits are already known at this point. But if the user has
 * forced "set autoscale" since the previous plot or refresh, we need to reset the
 * axis limits and try to approximate the full auto-scaling behaviour.
 */
//void refresh_3dbounds(SurfacePoints * first_plot, int nplots)
void GpGadgets::Refresh3DBounds(SurfacePoints * pFirstPlot, int nPlots)
{
	SurfacePoints * this_plot = pFirstPlot;
	int iplot;      /* plot index */
	for(iplot = 0; iplot < nPlots; iplot++, this_plot = this_plot->next_sp) {
		int i;  /* point index */
		GpAxis * x_axis = &AxA[FIRST_X_AXIS];
		GpAxis * y_axis = &AxA[FIRST_Y_AXIS];
		GpAxis * z_axis = &AxA[FIRST_Z_AXIS];
		iso_curve * this_curve;
		// IMAGE clipping is done elsewhere, so we don't need INRANGE/OUTRANGE checks.
		if(oneof3(this_plot->plot_style, IMAGE, RGBIMAGE, RGBA_IMAGE)) {
			if(x_axis->SetAutoScale)
				ProcessImage(term, this_plot, IMG_UPDATE_AXES);
			continue;
		}
		for(this_curve = this_plot->iso_crvs; this_curve; this_curve = this_curve->next) {
			for(i = 0; i<this_curve->p_count; i++) {
				GpCoordinate * point = &this_curve->points[i];
				if(point->type != UNDEFINED) {
					point->type = INRANGE;
					// If the state has been set to autoscale since the last plot,
					// mark everything INRANGE and re-evaluate the axis limits now.
					// Otherwise test INRANGE/OUTRANGE against previous axis limits.
					//
					// This autoscaling logic is identical to that in refresh_bounds() in plot2d.c
					//
					if(!this_plot->noautoscale) {
						x_axis->AdjustAutoscale(point->x);
					}
					if(!x_axis->InRange(point->x)) {
						point->type = OUTRANGE;
						continue;
					}
					if(!this_plot->noautoscale) {
						y_axis->AdjustAutoscale(point->y);
					}
					if(!y_axis->InRange(point->y)) {
						point->type = OUTRANGE;
						continue;
					}
					if(!this_plot->noautoscale) {
						z_axis->AdjustAutoscale(point->z);
					}
					if(!z_axis->InRange(point->z)) {
						point->type = OUTRANGE;
						continue;
					}
				}
			}
		}
	}
	// handle 'reverse' ranges
	RevertRange(FIRST_X_AXIS);
	RevertRange(FIRST_Y_AXIS);
	RevertRange(FIRST_Z_AXIS);
	// Make sure the bounds are reasonable, and tweak them if they aren't
	AxisCheckedExtendEmptyRange(FIRST_X_AXIS, NULL);
	AxisCheckedExtendEmptyRange(FIRST_Y_AXIS, NULL);
	AxisCheckedExtendEmptyRange(FIRST_Z_AXIS, NULL);
}

static double splines_kernel(double h)
{
	return (h > 0.0) ? (h * h * log(h)) : 0.0;
}

/* PKJ:
   This function has been hived off out of the original grid_nongrid_data().
   No changes have been made, but variables only needed locally have moved
   out of grid_nongrid_data() into this functin. */
static void thin_plate_splines_setup(iso_curve * old_iso_crvs, double ** p_xx, int * p_numpoints)
{
	int i, j, k;
	double * xx, * yy, * zz, * b, ** K, d;
	int * indx;
	iso_curve * oicrv;
	int numpoints = 0;
	for(oicrv = old_iso_crvs; oicrv != NULL; oicrv = oicrv->next) {
		numpoints += oicrv->p_count;
	}
	xx = (double *)malloc(sizeof(xx[0]) * (numpoints + 3) * (numpoints + 8));
	/* the memory needed is not really (n+3)*(n+8) for now,
	   but might be if I take into account errors ... */
	K = (double **)malloc(sizeof(K[0]) * (numpoints + 3));
	yy = xx + numpoints;
	zz = yy + numpoints;
	b = zz + numpoints;

	/* HBB 20010424: Count actual input points without the UNDEFINED
	 * ones, as we copy them */
	numpoints = 0;
	for(oicrv = old_iso_crvs; oicrv != NULL; oicrv = oicrv->next) {
		GpCoordinate * opoints = oicrv->points;
		for(k = 0; k < oicrv->p_count; k++, opoints++) {
			// HBB 20010424: avoid crashing for undefined input
			if(opoints->type != UNDEFINED) {
				xx[numpoints] = opoints->x;
				yy[numpoints] = opoints->y;
				zz[numpoints] = opoints->z;
				numpoints++;
			}
		}
	}
	for(i = 0; i < numpoints + 3; i++) {
		K[i] = b + (numpoints + 3) * (i + 1);
	}
	for(i = 0; i < numpoints; i++) {
		for(j = i + 1; j < numpoints; j++) {
			double dx = xx[i] - xx[j], dy = yy[i] - yy[j];
			K[i][j] = K[j][i] = -splines_kernel(sqrt(dx * dx + dy * dy));
		}
		K[i][i] = 0.0;  /* here will come the weights for errors */
		b[i] = zz[i];
	}
	for(i = 0; i < numpoints; i++) {
		K[i][numpoints] = K[numpoints][i] = 1.0;
		K[i][numpoints + 1] = K[numpoints + 1][i] = xx[i];
		K[i][numpoints + 2] = K[numpoints + 2][i] = yy[i];
	}
	b[numpoints] = 0.0;
	b[numpoints + 1] = 0.0;
	b[numpoints + 2] = 0.0;
	K[numpoints][numpoints] = 0.0;
	K[numpoints][numpoints + 1] = 0.0;
	K[numpoints][numpoints + 2] = 0.0;
	K[numpoints + 1][numpoints] = 0.0;
	K[numpoints + 1][numpoints + 1] = 0.0;
	K[numpoints + 1][numpoints + 2] = 0.0;
	K[numpoints + 2][numpoints] = 0.0;
	K[numpoints + 2][numpoints + 1] = 0.0;
	K[numpoints + 2][numpoints + 2] = 0.0;
	indx = (int *)malloc(sizeof(indx[0]) * (numpoints + 3));
	// actually, K is *not* positive definite, but has only non zero real eigenvalues -> we can use an lu_decomp safely
	lu_decomp(K, numpoints + 3, indx, &d);
	lu_backsubst(K, numpoints + 3, indx, b);
	free(K);
	free(indx);
	*p_xx = xx;
	*p_numpoints = numpoints;
}

static double qnorm(double dist_x, double dist_y, int q)
{
	double dist = 0.0;
	switch(q) {
		case 1:
		    dist = dist_x + dist_y;
		    break;
		case 2:
		    dist = dist_x * dist_x + dist_y * dist_y;
		    break;
		case 4:
		    dist = dist_x * dist_x + dist_y * dist_y;
		    dist *= dist;
		    break;
		case 8:
		    dist = dist_x * dist_x + dist_y * dist_y;
		    dist *= dist;
		    dist *= dist;
		    break;
		case 16:
		    dist = dist_x * dist_x + dist_y * dist_y;
		    dist *= dist;
		    dist *= dist;
		    dist *= dist;
		    break;
		default:
		    dist = pow(dist_x, (double)q) + pow(dist_y, (double)q);
		    break;
	}
	return dist;
}
//
// This is from Numerical Recipes in C, 2nd ed, p70
//
static double pythag(double dx, double dy)
{
	double x = fabs(dx);
	double y = fabs(dy);
	if(x > y)
		return x*sqrt(1.0 + (y*y)/(x*x));
	else if(y == 0.0)
		return 0.0;
	else
		return y*sqrt(1.0 + (x*x)/(y*y));
}

//static void grid_nongrid_data(SurfacePoints * this_plot)
void GpGadgets::GridNongridData(SurfacePoints * pPlot)
{
	int i, j, k;
	double x, y, z, w, dx, dy, xmin, xmax, ymin, ymax;
	iso_curve * old_iso_crvs = pPlot->iso_crvs;
	iso_curve * icrv, * oicrv, * oicrvs;
	int numpoints = 0;
	/* these are only needed for thin_plate_splines */
	double * yy, * zz, * b;
	double * xx = NULL; /* save to call free() on NULL if xx has never been used */
	/* Compute XY bounding box on the original data. */
	/* FIXME HBB 20010424: Does this make any sense? Shouldn't we just
	 * use whatever the x and y ranges have been found to be, and
	 * that's that? The largest difference this is going to make is if
	 * we plot a datafile that doesn't span the whole x/y range
	 * used. Do we want a dgrid3d over the actual data rectangle, or
	 * over the xrange/yrange area? */
	xmin = xmax = old_iso_crvs->points[0].x;
	ymin = ymax = old_iso_crvs->points[0].y;
	for(icrv = old_iso_crvs; icrv != NULL; icrv = icrv->next) {
		GpCoordinate * points = icrv->points;
		for(i = 0; i < icrv->p_count; i++, points++) {
			// HBB 20010424: avoid crashing for undefined input
			if(points->type != UNDEFINED) {
				SETMIN(xmin, points->x);
				SETMAX(xmax, points->x);
				SETMIN(ymin, points->y);
				SETMAX(ymax, points->y);
			}
		}
	}
	dx = (xmax - xmin) / (dgrid3d_col_fineness - 1);
	dy = (ymax - ymin) / (dgrid3d_row_fineness - 1);
	// Create the new grid structure, and compute the low pass filtering from
	// non grid to grid structure.
	pPlot->iso_crvs = NULL;
	pPlot->num_iso_read = dgrid3d_col_fineness;
	pPlot->has_grid_topology = true;
	if(dgrid3d_mode == DGRID3D_SPLINES) {
		thin_plate_splines_setup(old_iso_crvs, &xx, &numpoints);
		yy = xx + numpoints;
		zz = yy + numpoints;
		b  = zz + numpoints;
	}
	for(i = 0, x = xmin; i < dgrid3d_col_fineness; i++, x += dx) {
		GpCoordinate * points;
		icrv = iso_alloc(dgrid3d_row_fineness + 1);
		icrv->p_count = dgrid3d_row_fineness;
		icrv->next = pPlot->iso_crvs;
		pPlot->iso_crvs = icrv;
		points = icrv->points;
		for(j = 0, y = ymin; j<dgrid3d_row_fineness; j++, y += dy, points++) {
			z = w = 0.0;
			// as soon as ->type is changed to UNDEFINED, break out of two inner loops! 
			points->type = INRANGE;
			if(dgrid3d_mode == DGRID3D_SPLINES) {
				z = b[numpoints];
				for(k = 0; k < numpoints; k++) {
					double dx = xx[k] - x, dy = yy[k] - y;
					z = z - b[k] * splines_kernel(sqrt(dx * dx + dy * dy));
				}
				z = z + b[numpoints + 1] * x + b[numpoints + 2] * y;
			}
			else { /* everything, except splines */
				for(oicrv = old_iso_crvs; oicrv != NULL; oicrv = oicrv->next) {
					GpCoordinate * opoints = oicrv->points;
					for(k = 0; k < oicrv->p_count; k++, opoints++) {
						if(dgrid3d_mode == DGRID3D_QNORM) {
							double dist = qnorm(fabs(opoints->x - x), fabs(opoints->y - y), dgrid3d_norm_value);
							if(dist == 0.0) {
								/* HBB 981209: revised flagging as undefined */
								/* Supporting all those infinities on various
								 * platforms becomes tiresome,
								 * to say the least :-(
								 * Let's just return the first z where this
								 * happens unchanged, and be done with this,
								 * period. */
								points->type = UNDEFINED;
								z = opoints->z;
								w = 1.0;
								break; /* out of inner loop */
							}
							else {
								z += opoints->z / dist;
								w += 1.0/dist;
							}
						}
						else { // ALL else: not spline, not qnorm!
							double weight = 0.0;
							const double dist = pythag((opoints->x-x)/dgrid3d_x_scale, (opoints->y-y)/dgrid3d_y_scale);
							if(dgrid3d_mode == DGRID3D_GAUSS) {
								weight = exp(-dist*dist);
							}
							else if(dgrid3d_mode == DGRID3D_CAUCHY) {
								weight = 1.0/(1.0 + dist*dist );
							}
							else if(dgrid3d_mode == DGRID3D_EXP) {
								weight = exp(-dist);
							}
							else if(dgrid3d_mode == DGRID3D_BOX) {
								weight = (dist<1.0) ? 1.0 : 0.0;
							}
							else if(dgrid3d_mode == DGRID3D_HANN) {
								if(dist < 1.0) {
									weight = 0.5*(1-cos(2.0*M_PI*dist));
								}
							}
							z += opoints->z * weight;
							w += weight;
						}
					}

					/* PKJ: I think this is only relevant for qnorm */
					if(points->type != INRANGE)
						break;  /* out of the second-inner loop as well ... */
				}
			} /* endif( dgrid3d_mode == DGRID3D_SPLINES ) */

			/* Now that we've escaped the loops safely, we know that we
			 * do have a good value in z and w, so we can proceed just as
			 * if nothing had happened at all. Nice, isn't it? */
			points->type = INRANGE;

			/* HBB 20010424: if log x or log y axis, we don't want to
			 * log() the value again --> just store it, and trust that
			 * it's always inrange */
			points->x = x;
			points->y = y;

			/* Honor requested x and y limits */
			/* Historical note: This code was not in 4.0 or 4.2. It imperfectly */
			/* restores the clipping behaviour of version 3.7 and earlier. */
			if((x < GetX().Range.low && !(GetX().AutoScale & AUTOSCALE_MIN))
			    ||  (x > GetX().Range.upp && !(GetX().AutoScale & AUTOSCALE_MAX))
			    ||  (y < GetY().Range.low && !(GetY().AutoScale & AUTOSCALE_MIN))
			    ||  (y > GetY().Range.upp && !(GetY().AutoScale & AUTOSCALE_MAX)))
				points->type = OUTRANGE;
			if(dgrid3d_mode != DGRID3D_SPLINES && !dgrid3d_kdensity)
				z = z / w;
			STORE_WITH_LOG_AND_UPDATE_RANGE(points->z, z, points->type, ZAxis, pPlot->noautoscale, NOOP, continue);
			if(pPlot->pm3d_color_from_column)
				GpGg.IntErrorNoCaret("Gridding of the color column is not implemented");
			else {
				COLOR_STORE_WITH_LOG_AND_UPDATE_RANGE(points->CRD_COLOR, z, points->type, COLOR_AXIS, pPlot->noautoscale, NOOP, continue);
			}
		}
	}

	free(xx); /* save to call free on NULL pointer if splines not used */

	/* Delete the old non grid data. */
	for(oicrvs = old_iso_crvs; oicrvs != NULL; ) {
		oicrv = oicrvs;
		oicrvs = oicrvs->next;
		iso_free(oicrv);
	}
}

/* Get 3D data from file, and store into this_plot data
 * structure. Takes care of 'set mapping' and 'set dgrid3d'.
 *
 * Notice: this_plot->token is end of datafile spec, before title etc
 * will be moved past title etc after we return */
//static int get_3ddata(SurfacePoints * pPlot)
int GpGadgets::Get3DData(SurfacePoints * pPlot)
{
	int xdatum = 0;
	int ydatum = 0;
	int j;
	double v[MAXDATACOLS];
	int pt_in_iso_crv = 0;
	iso_curve * this_iso;
	int retval = 0;
	if(mapping3d == MAP3D_CARTESIAN) {
		// do this check only, if we have PM3D / PM3D-COLUMN not compiled in 
		if(GpDf.df_no_use_specs == 2)
			GpGg.IntError(pPlot->token, "Need 1 or 3 columns for cartesian data");
		// HBB NEW 20060427: if there's only one, explicit using
		// column, it's z data.  df_axis[] has to reflect that, so
		// df_readline() will expect time/date input
		if(GpDf.df_no_use_specs == 1)
			GpDf.df_axis[0] = FIRST_Z_AXIS;
	}
	else {
		if(GpDf.df_no_use_specs == 1)
			GpGg.IntError(pPlot->token, "Need 2 or 3 columns for polar data");
	}
	pPlot->num_iso_read = 0;
	pPlot->has_grid_topology = true;
	pPlot->pm3d_color_from_column = false;

	/* we ought to keep old memory - most likely case
	 * is a replot, so it will probably exactly fit into
	 * memory already allocated ?
	 */
	if(pPlot->iso_crvs != NULL) {
		for(iso_curve * icrvs = pPlot->iso_crvs; icrvs;) {
			iso_curve * icrv = icrvs;
			icrvs = icrvs->next;
			iso_free(icrv);
		}
		pPlot->iso_crvs = NULL;
	}
	/* data file is already open */
	if(GpDf.df_matrix)
		pPlot->has_grid_topology = true;
	{
		/*{{{  read surface from text file */
		iso_curve * local_this_iso = iso_alloc(Samples1);
		GpCoordinate * cp;
		GpCoordinate * cphead = NULL; /* Only for VECTOR plots */
		double x, y, z;
		double xtail, ytail, ztail;
		double color = GPVL;
		int pm3d_color_from_column = false;
#define color_from_column(x) pm3d_color_from_column = x
		if(pPlot->plot_style == LABELPOINTS)
			GpDf.ExpectString(4);
		if(pPlot->plot_style == VECTOR) {
			local_this_iso->next = iso_alloc(Samples1);
			local_this_iso->next->p_count = 0;
		}

		/* If the user has set an explicit locale for numeric input, apply it */
		/* here so that it affects data fields read from the input file.      */
		set_numeric_locale();
		// Initial state
		GpDf.df_warn_on_missing_columnheader = true;
		while((retval = GpDf.DfReadLine(v, MAXDATACOLS)) != DF_EOF) {
			j = retval;
			if(j == 0) /* not blank line, but df_readline couldn't parse it */
				int_warn(NO_CARET, "Bad data on line %d of file %s", GpDf.df_line_number, NZOR(GpDf.df_filename, ""));
			if(j == DF_SECOND_BLANK)
				break;  /* two blank lines */
			if(j == DF_FIRST_BLANK) {
				/* Images are in a sense similar to isocurves.
				 * However, the routine for images is written to
				 * compute the two dimensions of coordinates by
				 * examining the data alone.  That way it can be used
				 * in the 2D plots, for which there is no isoline
				 * record.  So, toss out isoline information for
				 * images.
				 */
				if(oneof3(pPlot->plot_style, IMAGE, RGBIMAGE, RGBA_IMAGE))
					continue;
				if(pPlot->plot_style == VECTOR)
					continue;
				/* one blank line */
				if(pt_in_iso_crv == 0) {
					if(xdatum == 0)
						continue;
					pt_in_iso_crv = xdatum;
				}
				if(xdatum > 0) {
					local_this_iso->p_count = xdatum;
					local_this_iso->next = pPlot->iso_crvs;
					pPlot->iso_crvs = local_this_iso;
					pPlot->num_iso_read++;
					if(xdatum != pt_in_iso_crv)
						pPlot->has_grid_topology = false;
					local_this_iso = iso_alloc(pt_in_iso_crv);
					xdatum = 0;
					ydatum++;
				}
				continue;
			}
			else if(j == DF_FOUND_KEY_TITLE) {
				/* only the shared part of the 2D and 3D headers is used */
				GpDf.DfSetKeyTitle((CurvePoints*)pPlot);
				continue;
			}
			else if(j == DF_KEY_TITLE_MISSING) {
				fprintf(stderr, "get_data: key title not found in requested column\n");
				continue;
			}
			else if(j == DF_COLUMN_HEADERS) {
				continue;
			}
			/* its a data point or undefined */
			if(xdatum >= local_this_iso->p_max) {
				/* overflow about to occur. Extend size of points[]
				 * array. Double the size, and add 1000 points, to
				 * avoid needlessly small steps. */
				iso_extend(local_this_iso, xdatum + xdatum + 1000);
				if(pPlot->plot_style == VECTOR) {
					iso_extend(local_this_iso->next, xdatum + xdatum + 1000);
					local_this_iso->next->p_count = 0;
				}
			}
			cp = local_this_iso->points + xdatum;
			if(pPlot->plot_style == VECTOR) {
				if(j < 6) {
					cp->type = UNDEFINED;
					continue;
				}
				cphead = local_this_iso->next->points + xdatum;
			}
			if(j == DF_UNDEFINED || j == DF_MISSING) {
				cp->type = UNDEFINED;
				goto come_here_if_undefined;
			}
			cp->type = INRANGE; // unless we find out different 
			// EAM Oct 2004 - Substantially rework this section 
			// now that there are many more plot types.         
			x = y = z = 0.0;
			xtail = ytail = ztail = 0.0;
			// The x, y, z coordinates depend on the mapping type 
			switch(mapping3d) {
				case MAP3D_CARTESIAN:
				    if(j == 1) {
					    x = xdatum;
					    y = ydatum;
					    z = v[0];
					    j = 3;
					    break;
				    }
				    if(j == 2) {
					    if(PM3DSURFACE != pPlot->plot_style)
						    GpGg.IntError(pPlot->token, "2 columns only possible with explicit pm3d style (line %d)", GpDf.df_line_number);
					    x = xdatum;
					    y = ydatum;
					    z = v[0];
					    color_from_column(true);
					    color = v[1];
					    j = 3;
					    break;
				    }

				    /* Assume everybody agrees that x,y,z are the first three specs */
				    if(j >= 3) {
					    x = v[0];
					    y = v[1];
					    z = v[2];
					    break;
				    }

				    break;

				case MAP3D_SPHERICAL:
				    if(j < 2)
					    GpGg.IntError(pPlot->token, "Need 2 or 3 columns");
				    if(j < 3) {
					    v[2] = 1; /* default radius */
					    j = 3;
				    }

				    /* Convert to radians. */
				    v[0] *= Ang2Rad;
				    v[1] *= Ang2Rad;

				    x = v[2] * cos(v[0]) * cos(v[1]);
				    y = v[2] * sin(v[0]) * cos(v[1]);
				    z = v[2] * sin(v[1]);

				    break;

				case MAP3D_CYLINDRICAL:
				    if(j < 2)
					    GpGg.IntError(pPlot->token, "Need 2 or 3 columns");
				    if(j < 3) {
					    v[2] = 1; /* default radius */
					    j = 3;
				    }

				    /* Convert to radians. */
				    v[0] *= Ang2Rad;

				    x = v[2] * cos(v[0]);
				    y = v[2] * sin(v[0]);
				    z = v[1];

				    break;

				default:
				    GpGg.IntErrorNoCaret("Internal error: Unknown mapping type");
				    return retval;
			}
			if(j < GpDf.df_no_use_specs)
				GpGg.IntError(pPlot->token, "Wrong number of columns in input data - line %d", GpDf.df_line_number);
			/* Work-around for hidden3d, which otherwise would use the */
			/* color of the vector midpoint rather than the endpoint. */
			if(pPlot->plot_style == IMPULSES) {
				if(pPlot->lp_properties.pm3d_color.type == TC_Z) {
					color = z;
					color_from_column(true);
				}
			}
			/* After the first three columns it gets messy because */
			/* different plot styles assume different contents in the columns */
			if(oneof2(pPlot->plot_style, POINTSTYLE, LINESPOINTS)) {
				int varcol = 3;
				if(pPlot->lp_properties.p_size == PTSZ_VARIABLE)
					cp->CRD_PTSIZE = v[varcol++];
				if(pPlot->lp_properties.p_type == PT_VARIABLE)
					cp->CRD_PTTYPE = v[varcol++];
				if(j < varcol)
					GpGg.IntErrorNoCaret("Not enough input columns");
				else if(j == varcol) {
					color = z;
					color_from_column(false);
				}
				else {
					color = v[varcol];
					color_from_column(true);
				}
			}
			else if(pPlot->plot_style == LABELPOINTS) {
				if(j == 4) {
					/* 4th column holds label text rather than color */
					color = z;
					color_from_column(false);
				}
				else {
					color = v[4];
					color_from_column(true);
				}
			}
			else if(pPlot->plot_style == VECTOR) {
				if(j == 6) {
					xtail = x + v[3];
					ytail = y + v[4];
					ztail = z + v[5];
					color = z;
					color_from_column(false);
				}
				else if(j >= 7) {
					color = v[6];
					color_from_column(true);
				}
				else {
					color = v[3];
					color_from_column(true);
				}
			}
			else { /* all other plot styles */
				if(j >= 4) {
					color = v[3];
					color_from_column(true);
				}
			}
#undef color_from_column

			/* Adjust for logscales. Set min/max and point types. Store in cp.
			 * The macro cannot use continue, as it is wrapped in a loop.
			 * I regard this as correct goto use
			 */
			cp->type = INRANGE;
			STORE_WITH_LOG_AND_UPDATE_RANGE(cp->x, x, cp->type, XAxis, pPlot->noautoscale, NOOP, goto come_here_if_undefined);
			STORE_WITH_LOG_AND_UPDATE_RANGE(cp->y, y, cp->type, YAxis, pPlot->noautoscale, NOOP, goto come_here_if_undefined);
			if(pPlot->plot_style == VECTOR) {
				cphead->type = INRANGE;
				STORE_WITH_LOG_AND_UPDATE_RANGE(cphead->x, xtail, cphead->type, XAxis, pPlot->noautoscale, NOOP, goto come_here_if_undefined);
				STORE_WITH_LOG_AND_UPDATE_RANGE(cphead->y, ytail, cphead->type, YAxis, pPlot->noautoscale, NOOP, goto come_here_if_undefined);
			}
			if(dgrid3d) {
				// HBB 20010424: in dgrid3d mode, delay log() taking
				// and scaling until after the dgrid process. Only for
				// z, not for x and y, so we can layout the newly
				// created created grid more easily.
				cp->z = z;
				if(pPlot->plot_style == VECTOR)
					cphead->z = ztail;
			}
			else {
				// EAM Sep 2008 - Otherwise z=Nan or z=Inf or DF_MISSING fails
				// to set CRD_COLOR at all, since the z test bails to a goto.
				if(pPlot->plot_style == IMAGE) {
					cp->CRD_COLOR = (pm3d_color_from_column) ? color : z;
				}
				// Version 5: cp->z=0 in the UNDEF_ACTION recovers what	version 4 did
				STORE_WITH_LOG_AND_UPDATE_RANGE(cp->z, z, cp->type, ZAxis, pPlot->noautoscale, NOOP, cp->z = 0; goto come_here_if_undefined);
				if(pPlot->plot_style == VECTOR)
					STORE_WITH_LOG_AND_UPDATE_RANGE(cphead->z, ztail, cphead->type, ZAxis, pPlot->noautoscale, NOOP, goto come_here_if_undefined);
				if(pPlot->lp_properties.l_type == LT_COLORFROMCOLUMN)
					cp->CRD_COLOR = color;
				if(pm3d_color_from_column) {
					COLOR_STORE_WITH_LOG_AND_UPDATE_RANGE(cp->CRD_COLOR, color, cp->type, COLOR_AXIS, pPlot->noautoscale, NOOP, goto come_here_if_undefined);
				}
				else {
					COLOR_STORE_WITH_LOG_AND_UPDATE_RANGE(cp->CRD_COLOR, z, cp->type, COLOR_AXIS, pPlot->noautoscale, NOOP, goto come_here_if_undefined);
				}
			}
			// At this point we have stored the point coordinates. Now we need to copy
			// x,y,z into the GpTextLabel structure and add the actual text string.
			if(pPlot->plot_style == LABELPOINTS)
				store_label(pPlot->labels, cp, xdatum, GpDf.df_tokens[3], color);
			if(pPlot->plot_style == RGBIMAGE || pPlot->plot_style == RGBA_IMAGE) {
				// We will autoscale the RGB components to  a total range [0:255]
				// so we don't need to do any fancy scaling here.
				cp->CRD_R = v[3];
				cp->CRD_G = v[4];
				cp->CRD_B = v[5];
				cp->CRD_A = v[6]; /* Alpha channel */
			}

come_here_if_undefined:
			/* some may complain, but I regard this as the correct use of goto */
			++xdatum;
		}               /* end of whileloop - end of surface */

		/* We are finished reading user input; return to C locale for internal use */
		reset_numeric_locale();
		if(pm3d_color_from_column) {
			pPlot->pm3d_color_from_column = pm3d_color_from_column ? true : false;
		}
		if(xdatum > 0) {
			pPlot->num_iso_read++; /* Update last iso. */
			local_this_iso->p_count = xdatum;
			// If this is a VECTOR plot then iso->next is already 
			// occupied by the vector tail coordinates.           
			if(pPlot->plot_style != VECTOR)
				local_this_iso->next = pPlot->iso_crvs;
			pPlot->iso_crvs = local_this_iso;
			if(xdatum != pt_in_iso_crv)
				pPlot->has_grid_topology = false;
		}
		else { /* Free last allocation */
			if(pPlot->plot_style == VECTOR)
				iso_free(local_this_iso->next);
			iso_free(local_this_iso);
		}
		/*}}} */
	}
	if(dgrid3d && pPlot->num_iso_read > 0)
		GridNongridData(pPlot);
	// This check used to be done in graph3d 
	if(GetX().IsRangeUndef() || GetY().IsRangeUndef() || GetZ().IsRangeUndef()) {
		// FIXME: Should we set plot type to NODATA? 
		// But in the case of 'set view map' we may not care about Z 
		int_warn(NO_CARET, "No usable data in this plot to auto-scale axis range");
	}
	if(pPlot->num_iso_read <= 1)
		pPlot->has_grid_topology = false;
	if(pPlot->has_grid_topology && !hidden3d && (implicit_surface || pPlot->plot_style == SURFACEGRID)) {
		iso_curve * new_icrvs = NULL;
		int num_new_iso = pPlot->iso_crvs->p_count;
		int len_new_iso = pPlot->num_iso_read;
		// Now we need to set the other direction (pseudo) isolines
		for(int i = 0; i < num_new_iso; i++) {
			iso_curve * new_icrv = iso_alloc(len_new_iso);
			new_icrv->p_count = len_new_iso;
			for(j = 0, this_iso = pPlot->iso_crvs; this_iso != NULL; j++, this_iso = this_iso->next) {
				/* copy whole point struct to get type too.
				 * wasteful for windows, with padding */
				/* more efficient would be extra pointer to same struct */
				new_icrv->points[j] = this_iso->points[i];
			}
			new_icrv->next = new_icrvs;
			new_icrvs = new_icrv;
		}
		/* Append the new iso curves after the read ones. */
		for(this_iso = pPlot->iso_crvs; this_iso->next != NULL; this_iso = this_iso->next) 
			;
		this_iso->next = new_icrvs;
	}
	return retval;
}
//
// HBB 20000501: code isolated from eval_3dplots(), where practically
// identical code occured twice, for direct and crossing isolines,
// respectively.  The latter only are done for in non-hidden3d mode.
//
/* static void calculate_set_of_isolines(AXIS_INDEX value_axis,
    bool cross, iso_curve ** this_iso, AXIS_INDEX iso_axis,
    double iso_min, double iso_step, int num_iso_to_use, AXIS_INDEX sam_axis,
    double sam_min, double sam_step, int num_sam_to_use)*/
void GpGadgets::CalculateSetOfIsolines(AXIS_INDEX valueAxis,
    bool cross, iso_curve ** ppThisIso, AXIS_INDEX isoAxis,
    double iso_min, double iso_step, int num_iso_to_use, AXIS_INDEX sam_axis,
    double sam_min, double sam_step, int num_sam_to_use)
{
	int i, j;
	GpCoordinate * p_points = (*ppThisIso)->points;
	int do_update_color = (!IsParametric || (IsParametric && valueAxis == FIRST_Z_AXIS));
	for(j = 0; j < num_iso_to_use; j++) {
		double iso = iso_min + j * iso_step;
		// HBB 20000501: with the new code, it should
		// be safe to rely on the actual 'v' axis not to be improperly logscaled...
		plot3D_func.dummy_values[cross ? 0 : 1].SetComplex(DelogValue(isoAxis, iso), 0.0);
		for(i = 0; i < num_sam_to_use; i++) {
			double sam = sam_min + i * sam_step;
			t_value a;
			double temp;
			plot3D_func.dummy_values[cross ? 1 : 0].SetComplex(DelogValue(sam_axis, sam), 0.0);
			if(cross) {
				p_points[i].x = iso;
				p_points[i].y = sam;
			}
			else {
				p_points[i].x = sam;
				p_points[i].y = iso;
			}
			Ev.EvaluateAt(plot3D_func.at, &a);
			if(Ev.undefined || !IsZero(imag(&a))) {
				p_points[i].type = UNDEFINED;
				continue;
			}
			temp = a.Real();
			p_points[i].type = INRANGE;
			STORE_WITH_LOG_AND_UPDATE_RANGE(p_points[i].z, temp, p_points[i].type, valueAxis, false, NOOP, NOOP);
			if(do_update_color) {
				COLOR_STORE_WITH_LOG_AND_UPDATE_RANGE(p_points[i].CRD_COLOR, temp, p_points[i].type, COLOR_AXIS, false, NOOP, NOOP);
			}
		}
		(*ppThisIso)->p_count = num_sam_to_use;
		*ppThisIso = (*ppThisIso)->next;
		p_points = (*ppThisIso) ? (*ppThisIso)->points : NULL;
	}
}

/*
 * This parses the splot command after any range specifications. To support
 * autoscaling on the x/z axis, we want any data files to define the x/y
 * range, then to plot any functions using that range. We thus parse the
 * input twice, once to pick up the data files, and again to pick up the
 * functions. Definitions are processed twice, but that won't hurt.
 * div - okay, it doesn't hurt, but every time an option as added for
 * datafiles, code to parse it has to be added here. Change so that
 * we store starting-token in the plot structure.
 */
//static void eval_3dplots()
void GpGadgets::Eval3DPlots(GpCommand & rC)
{
	int i;
	SurfacePoints ** tp_3d_ptr;

	int start_token = 0, end_token;
	int highest_iteration = 0; /* last index reached in iteration [i=start:*] */
	bool eof_during_iteration = false; /* set when for [n=start:*] hits NODATA */
	int begin_token;
	bool some_data_files = false;
	bool some_functions = false;
	bool was_definition = false;
	int df_return = 0;
	int plot_num, line_num;
	/* part number of parametric function triplet: 0 = z, 1 = y, 2 = x */
	int crnt_param = 0;
	char * xtitle;
	char * ytitle;
	legend_key * key = &keyT;
	//
	// Free memory from previous splot.
	// If there is an error within this function, the memory is left allocated,
	// since we cannot call sp_free if the list is incomplete
	//
	if(P_First3DPlot && plot3d_num > 0)
		sp_free(P_First3DPlot);
	plot3d_num = 0;
	P_First3DPlot = NULL;
	XAxis = FIRST_X_AXIS;
	YAxis = FIRST_Y_AXIS;
	ZAxis = FIRST_Z_AXIS;
	tp_3d_ptr = &P_First3DPlot;
	plot_num = 0;
	line_num = 0;           /* default line type */
	IsVolatileData = false; /* Assume that the input data can be re-read later */
	xtitle = NULL;
	ytitle = NULL;
	begin_token = rC.CToken;

/*** First Pass: Read through data files ***/
	/*
	 * This pass serves to set the x/yranges and to parse the command, as
	 * well as filling in every thing except the function data. That is done
	 * after the x/yrange is defined.
	 */
	rC.P.P_PlotIterator = rC.CheckForIteration();
	while(true) {
		// Forgive trailing comma on a multi-element plot command 
		if(rC.EndOfCommand()) {
			if(plot_num == 0)
				IntErrorCurToken("function to plot expected");
			break;
		}
		if(crnt_param == 0 && !was_definition)
			start_token = rC.CToken;
		if(rC.IsDefinition()) {
			rC.Define();
			if(rC.Eq(","))
				rC.CToken++;
			was_definition = true;
			continue;
		}
		else {
			int specs = -1;
			SurfacePoints * p_plot;
			char * name_str;
			bool duplication = false;
			bool set_title = false, set_with = false;
			bool set_lpstyle = false;
			bool checked_once = false;
			bool set_labelstyle = false;
			int sample_range_token;
			if(!was_definition && (!IsParametric || crnt_param == 0))
				start_token = rC.CToken;
			was_definition = false;
			// Check for a sampling range
			sample_range_token = ParseRange(SAMPLE_AXIS, rC);
			if(sample_range_token > 0)
				AxA[SAMPLE_AXIS].range_flags |= RANGE_SAMPLED;
			// Should this be saved in p_plot?
			rC.P_DummyFunc = &plot3D_func;
			name_str = rC.P.StringOrExpress(rC, NULL);
			rC.P_DummyFunc = NULL;
			if(name_str) {
				/*{{{  data file to plot */
				if(IsParametric && crnt_param != 0)
					IntErrorCurToken("previous parametric function not fully specified");
				if(!some_data_files) {
					AxA[FIRST_X_AXIS].SetAutoscale(GPVL, -GPVL);
					AxA[FIRST_Y_AXIS].SetAutoscale(GPVL, -GPVL);
					some_data_files = true;
				}
				if(*tp_3d_ptr)
					p_plot = *tp_3d_ptr;
				else { // no memory malloc()'d there yet 
					// Allocate enough isosamples and samples
					p_plot = sp_alloc(0, 0, 0, 0);
					*tp_3d_ptr = p_plot;
				}
				p_plot->plot_type = DATA3D;
				p_plot->plot_style = DataStyle;
				eof_during_iteration = false;
				GpDf.DfSetPlotMode(MODE_SPLOT);
				specs = GpDf.DfOpen(rC, name_str, MAXDATACOLS, (CurvePoints*)p_plot);
				if(GpDf.df_matrix)
					p_plot->has_grid_topology = true;
				// EAM FIXME - this seems to work but I am uneasy that rC.P.CDummyVar[] is not being loaded with the variable name.
				if(sample_range_token > 0) {
					p_plot->sample_var = Ev.AddUdv(rC, sample_range_token);
				}
				else {
					/* FIXME: This has the side effect of creating a named variable x */
					/* or overwriting an existing variable x.  Maybe it should save   */
					/* and restore the pre-existing variable in this case?            */
					p_plot->sample_var = Ev.AddUdvByName(rC.P.CDummyVar[0]);
				}
				if(p_plot->sample_var->udv_value.type == NOTDEFINED)
					p_plot->sample_var->udv_value.SetComplex(0.0, 0.0);
				// for capture to key 
				p_plot->token = end_token = rC.CToken - 1;
				// FIXME: Is this really needed? 
				p_plot->iteration = rC.P.P_PlotIterator ? rC.P.P_PlotIterator->iteration : 0;
				// p_plot->token is temporary, for errors in get_3ddata() 
				if(specs < 3) {
					if(AxA[FIRST_X_AXIS].datatype == DT_TIMEDATE) {
						IntErrorCurToken("Need full using spec for x time data");
					}
					if(AxA[FIRST_Y_AXIS].datatype == DT_TIMEDATE) {
						IntErrorCurToken("Need full using spec for y time data");
					}
				}
				GpDf.df_axis[0] = FIRST_X_AXIS;
				GpDf.df_axis[1] = FIRST_Y_AXIS;
				GpDf.df_axis[2] = FIRST_Z_AXIS;
				/*}}} */
			}
			else {  /* function to plot */
				/*{{{  function */
				++plot_num;
				if(IsParametric) {
					/* Rotate between x/y/z axes */
					/* +2 same as -1, but beats -ve problem */
					crnt_param = (crnt_param + 2) % 3;
				}
				if(*tp_3d_ptr) {
					p_plot = *tp_3d_ptr;
					if(!hidden3d)
						sp_replace(p_plot, Samples1, iso_samples_1, Samples2, iso_samples_2);
					else
						sp_replace(p_plot, iso_samples_1, 0, 0, iso_samples_2);
				}
				else { // no memory malloc()'d there yet
					// Allocate enough isosamples and samples
					if(!hidden3d)
						p_plot = sp_alloc(Samples1, iso_samples_1, Samples2, iso_samples_2);
					else
						p_plot = sp_alloc(iso_samples_1, 0, 0, iso_samples_2);
					*tp_3d_ptr = p_plot;
				}
				p_plot->plot_type = FUNC3D;
				p_plot->has_grid_topology = true;
				p_plot->plot_style = FuncStyle;
				p_plot->num_iso_read = iso_samples_2;
				// ignore it for now
				some_functions = true;
				end_token = rC.CToken - 1;
				/*}}} */
			}       /* end of IS THIS A FILE OR A FUNC block */
			ZFREE(p_plot->title); // clear current title, if exist
			// default line and point types
			p_plot->lp_properties.l_type = line_num;
			p_plot->lp_properties.p_type = line_num;
			p_plot->lp_properties.d_type = line_num;
			// user may prefer explicit line styles
			p_plot->hidden3d_top_linetype = line_num;
			if(prefer_line_styles)
				lp_use_properties(&p_plot->lp_properties, line_num+1);
			else
				load_linetype(&p_plot->lp_properties, line_num+1);
			// pm 25.11.2001 allow any order of options 
			while(!rC.EndOfCommand() || !checked_once) {
				int save_token = rC.CToken;
				// deal with title 
				ParsePlotTitle(rC, (CurvePoints*)p_plot, xtitle, ytitle, &set_title);
				if(save_token != rC.CToken)
					continue;
				// deal with style 
				if(rC.AlmostEq("w$ith")) {
					if(set_with) {
						duplication = true;
						break;
					}
					p_plot->plot_style = get_style(rC);
					if((p_plot->plot_type == FUNC3D) && ((p_plot->plot_style & PLOT_STYLE_HAS_ERRORBAR) || (p_plot->plot_style == LABELPOINTS && !draw_contour))) {
						int_warn(rC.CToken-1, "This style cannot be used to plot a surface defined by a function");
						p_plot->plot_style = POINTSTYLE;
						p_plot->plot_type = NODATA;
					}
					if(oneof3(p_plot->plot_style, IMAGE, RGBA_IMAGE, RGBIMAGE)) {
						if(p_plot->plot_type == FUNC3D)
							IntError(rC.CToken-1, "a function cannot be plotted as an image");
						else
							get_image_options(&p_plot->image_properties);
					}
					if((p_plot->plot_style | DataStyle) & PM3DSURFACE) {
						if(rC.Eq("at")) {
							// option 'with pm3d [at ...]' is explicitly specified 
							rC.CToken++;
							if(get_pm3d_at_option(rC, &p_plot->pm3d_where[0]))
								return;  // error 
						}
					}
					if(p_plot->plot_style == TABLESTYLE)
						GpGg.IntErrorNoCaret("use `plot with table` rather than `splot with table`");
					set_with = true;
					continue;
				}
				// Hidden3D code by default includes points, labels and vectors	
				// in the hidden3d processing. Check here if this particular	
				// plot wants to be excluded.					
				if(rC.AlmostEq("nohidden$3d")) {
					rC.CToken++;
					p_plot->opt_out_of_hidden3d = true;
					continue;
				}
				// "set contour" is global.  Allow individual plots to opt out 
				if(rC.AlmostEq("nocon$tours")) {
					rC.CToken++;
					p_plot->opt_out_of_contours = true;
					continue;
				}
				// "set surface" is global.  Allow individual plots to opt out 
				if(rC.AlmostEq("nosur$face")) {
					rC.CToken++;
					p_plot->opt_out_of_surface = true;
					continue;
				}
				// Most plot styles accept line and point properties but do not 
				// want font or text properties.				
				if(p_plot->plot_style == VECTOR) {
					int stored_token = rC.CToken;
					if(!checked_once) {
						default_arrow_style(&p_plot->arrow_properties);
						load_linetype(&(p_plot->arrow_properties.lp_properties), line_num+1);
						checked_once = true;
					}
					arrow_parse(rC, &p_plot->arrow_properties, true);
					if(stored_token != rC.CToken) {
						if(set_lpstyle) {
							duplication = true;
							break;
						}
						else {
							set_lpstyle = true;
							p_plot->lp_properties = p_plot->arrow_properties.lp_properties;
							continue;
						}
					}
				}

				if(p_plot->plot_style == PM3DSURFACE) {
					// both previous and subsequent line properties override pm3d default border
					int stored_token = rC.CToken;
					if(!set_lpstyle)
						p_plot->lp_properties = Pm3D.border;
					LpParse(rC, p_plot->lp_properties, LP_ADHOC, false);
					if(stored_token != rC.CToken) {
						set_lpstyle = true;
						continue;
					}
				}
				if(p_plot->plot_style != LABELPOINTS) {
					int stored_token = rC.CToken;
					lp_style_type lp; // = DEFAULT_LP_STYLE_TYPE;
					int new_lt = 0;
					lp.l_type = line_num;
					lp.p_type = line_num;
					lp.d_type = line_num;
					// user may prefer explicit line styles
					if(prefer_line_styles)
						lp_use_properties(&lp, line_num+1);
					else
						load_linetype(&lp, line_num+1);
					new_lt = LpParse(rC, lp, LP_ADHOC, (p_plot->plot_style & PLOT_STYLE_HAS_POINT) ? true : false);
					checked_once = true;
					if(stored_token != rC.CToken) {
						if(set_lpstyle) {
							duplication = true;
							break;
						}
						else {
							p_plot->lp_properties = lp;
							set_lpstyle = true;
							if(new_lt)
								p_plot->hidden3d_top_linetype = new_lt - 1;
							if(p_plot->lp_properties.p_type != PT_CHARACTER)
								continue;
						}
					}
				}
				/* Labels can have font and text property info as plot options */
				/* In any case we must allocate one instance of the text style */
				/* that all labels in the plot will share.                     */
				if((p_plot->plot_style == LABELPOINTS) || (p_plot->plot_style & PLOT_STYLE_HAS_POINT && p_plot->lp_properties.p_type == PT_CHARACTER)) {
					int stored_token = rC.CToken;
					if(p_plot->labels == NULL) {
						p_plot->labels = new_text_label(-1);
						p_plot->labels->pos = CENTRE;
						p_plot->labels->layer = LAYER_PLOTLABELS;
					}
					ParseLabelOptions(rC, p_plot->labels, 3);
					if(draw_contour)
						load_contour_label_options(p_plot->labels);
					checked_once = true;
					if(stored_token != rC.CToken) {
						if(set_labelstyle) {
							duplication = true;
							break;
						}
						else {
							set_labelstyle = true;
							continue;
						}
					}
				}
				break; // unknown option
			}
			if(duplication)
				IntErrorCurToken("duplicated or contradicting arguments in plot options");
			// set default values for title if this has not been specified
			p_plot->title_is_filename = false;
			if(!set_title) {
				p_plot->title_no_enhanced = true; // filename or function cannot be enhanced
				if(key->auto_titles == FILENAME_KEYTITLES) {
					rC.MCapture(&(p_plot->title), start_token, end_token);
					if(crnt_param == 2)
						xtitle = p_plot->title;
					else if(crnt_param == 1)
						ytitle = p_plot->title;
					p_plot->title_is_filename = true;
				}
				else {
					ASSIGN_PTR(xtitle, 0);
					ASSIGN_PTR(ytitle, 0);
				}
			}

			/* No line/point style given. As lp_parse also supplies
			 * the defaults for linewidth and pointsize, call it now
			 * to define them. */
			if(!set_lpstyle) {
				if(p_plot->plot_style == VECTOR) {
					p_plot->arrow_properties.lp_properties.l_type = line_num;
					arrow_parse(rC, &p_plot->arrow_properties, true);
					p_plot->lp_properties = p_plot->arrow_properties.lp_properties;
				}
				else if(p_plot->plot_style == PM3DSURFACE) {
					// Use default pm3d border unless we see explicit line properties 
					p_plot->lp_properties = Pm3D.border;
					LpParse(rC, p_plot->lp_properties, LP_ADHOC, false);
				}
				else {
					int new_lt = 0;
					p_plot->lp_properties.l_type = line_num;
					p_plot->lp_properties.l_width = 1.0;
					p_plot->lp_properties.p_type = line_num;
					p_plot->lp_properties.d_type = line_num;
					p_plot->lp_properties.p_size = PtSz;
					// user may prefer explicit line styles
					if(prefer_line_styles)
						lp_use_properties(&p_plot->lp_properties, line_num+1);
					else
						load_linetype(&p_plot->lp_properties, line_num+1);
					new_lt = LpParse(rC, p_plot->lp_properties, LP_ADHOC, (p_plot->plot_style & PLOT_STYLE_HAS_POINT) ? true : false);
					p_plot->hidden3d_top_linetype = new_lt ? (new_lt - 1) : line_num;
				}
			}
			// Some low-level routines expect to find the pointflag attribute
			// in lp_properties (they don't have access to the full header).
			if(p_plot->plot_style & PLOT_STYLE_HAS_POINT)
				p_plot->lp_properties.flags |= LP_SHOW_POINTS;
			// Rule out incompatible line/point/style options
			if(p_plot->plot_type == FUNC3D) {
				if((p_plot->plot_style & PLOT_STYLE_HAS_POINT) && (p_plot->lp_properties.p_size == PTSZ_VARIABLE))
					p_plot->lp_properties.p_size = 1;
			}
			if(p_plot->plot_style == LINES) {
				p_plot->opt_out_of_hidden3d = false;
			}
			// don't increment the default line/point properties if p_plot is an EXPLICIT pm3d surface plot
			if(crnt_param == 0 && !oneof4(p_plot->plot_style, PM3DSURFACE, IMAGE, RGBIMAGE, RGBA_IMAGE)) { // same as above, for an (rgb)image plot
				line_num += 1 + (draw_contour != 0) + (hidden3d != 0);
			}
			if(oneof2(p_plot->plot_style, RGBIMAGE, RGBA_IMAGE))
				GetCB().SetAutoscale(0.0, 255.0);
			/* now get the data... having to think hard here...
			 * first time through, we fill in p_plot. For second
			 * surface in file, we have to allocate another surface
			 * struct. BUT we may allocate this store only to
			 * find that it is merely some blank lines at end of file
			 * tp_3d_ptr is still pointing at next field of prev. plot,
			 * before :    prev_or_first -> p_plot -> possible_preallocated_store
			 *                tp_3d_ptr--^
			 * after  :    prev_or_first -> first -> second -> last -> possibly_more_store
			 *                                        tp_3d_ptr ----^
			 * if file is empty, tp_3d_ptr is not moved. p_plot continues
			 * to point at allocated storage, but that will be reused later
			 */
			assert(p_plot == *tp_3d_ptr);
			if(p_plot->plot_type == DATA3D) {
				/*{{{  read data */
				/* pointer to the plot of the first dataset (surface) in the file */
				SurfacePoints * first_dataset = p_plot;
				int this_token = p_plot->token;
				/* Error check to handle missing or unreadable file */
				if(specs == DF_EOF) {
					/* FIXME: plot2d does ++line_num here; needed in 3D also? */
					p_plot->plot_type = NODATA;
					goto SKIPPED_EMPTY_FILE;
				}
				do {
					p_plot = *tp_3d_ptr;
					assert(p_plot != NULL);
					//
					// dont move tp_3d_ptr until we are sure we have read a surface
					//
					// used by get_3ddata()
					p_plot->token = this_token;
					df_return = Get3DData(p_plot);
					// for second pass
					p_plot->token = rC.CToken;
					p_plot->iteration = rC.P.P_PlotIterator ? rC.P.P_PlotIterator->iteration : 0;
					if(p_plot->num_iso_read == 0)
						p_plot->plot_type = NODATA;
					if(p_plot != first_dataset)
						// copy (explicit) "with pm3d at ..." option from the first dataset in the file
						strcpy(p_plot->pm3d_where, first_dataset->pm3d_where);
					// okay, we have read a surface
					++plot_num;
					tp_3d_ptr = &(p_plot->next_sp);
					if(df_return == DF_EOF)
						break;
					//
					// there might be another surface so allocate and prepare another surface structure
					// This does no harm if in fact there are no more surfaces to read
					//
					if((p_plot = *tp_3d_ptr) != NULL) {
						ZFREE(p_plot->title);
					}
					else {
						// Allocate enough isosamples and samples
						p_plot = *tp_3d_ptr = sp_alloc(0, 0, 0, 0);
					}
					p_plot->plot_type = DATA3D;
					p_plot->iteration = rC.P.P_PlotIterator ? rC.P.P_PlotIterator->iteration : 0;
					p_plot->plot_style = first_dataset->plot_style;
					p_plot->lp_properties = first_dataset->lp_properties;
					if((p_plot->plot_style == LABELPOINTS) || (p_plot->plot_style & PLOT_STYLE_HAS_POINT && p_plot->lp_properties.p_type == PT_CHARACTER)) {
						p_plot->labels = new_text_label(-1);
						*(p_plot->labels) = *(first_dataset->labels);
						p_plot->labels->next = NULL;
					}
				} while(df_return != DF_EOF);
				GpDf.DfClose();
				// Plot-type specific range-fiddling
				if(p_plot->plot_style == IMPULSES && !(AxA[FIRST_Z_AXIS].Flags & GpAxis::fLog)) {
					if(AxA[FIRST_Z_AXIS].AutoScale & AUTOSCALE_MIN) {
						SETMIN(AxA[FIRST_Z_AXIS].Range.low, 0);
					}
					if(AxA[FIRST_Z_AXIS].AutoScale & AUTOSCALE_MAX) {
						SETMAX(AxA[FIRST_Z_AXIS].Range.upp, 0);
					}
				}
				/*}}} */
			}
			else {  /* not a data file */
				tp_3d_ptr = &(p_plot->next_sp);
				p_plot->token = rC.CToken; /* store for second pass */
				p_plot->iteration = rC.P.P_PlotIterator ? rC.P.P_PlotIterator->iteration : 0;
			}

SKIPPED_EMPTY_FILE:
			if(empty_iteration(rC.P.P_PlotIterator))
				p_plot->plot_type = NODATA;
			if(forever_iteration(rC.P.P_PlotIterator) && (p_plot->plot_type == NODATA)) {
				highest_iteration = rC.P.P_PlotIterator->iteration_current;
				eof_during_iteration = true;
			}
			if(forever_iteration(rC.P.P_PlotIterator) && (p_plot->plot_type == FUNC3D)) {
				GpGg.IntErrorNoCaret("unbounded iteration in function plot");
			}
		}
		if(crnt_param != 0) {
			if(rC.Eq(",")) {
				rC.CToken++;
				continue;
			}
			else
				break;
		}
		// Iterate-over-plot mechanisms
		if(eof_during_iteration) {
			; // Nothing to do
		}
		else if(next_iteration(rC.P.P_PlotIterator)) {
			rC.CToken = start_token;
			highest_iteration = rC.P.P_PlotIterator->iteration_current;
			continue;
		}
		rC.P.CleanupPlotIterator();
		if(rC.Eq(",")) {
			rC.CToken++;
			rC.P.P_PlotIterator = rC.CheckForIteration();
		}
		else
			break;
	}
	if(IsParametric && crnt_param != 0)
		GpGg.IntErrorNoCaret("parametric function not fully specified");
/*** Second Pass: Evaluate the functions ***/
	/*
	 * Everything is defined now, except the function data. We expect no
	 * syntax errors, etc, since the above parsed it all. This makes the code
	 * below simpler. If AxA[FIRST_Y_AXIS].AutoScale, the yrange may still change.
	 * - eh ?  - z can still change.  x/y/z can change if we are parametric ??
	 */

	if(some_functions) {
		/* I've changed the controlled variable in fn plots to u_min etc since
		 * it's easier for me to think parametric - 'normal' plot is after all
		 * a special case. I was confused about x_min being both minimum of
		 * x values found, and starting value for fn plots.
		 */
		double u_min, u_max, u_step, v_min, v_max, v_step;
		double u_isostep, v_isostep;
		SurfacePoints * p_plot;
		// Make these point out the right 'u' and 'v' axis. In non-parametric mode, x is used as u, and y as v
		AXIS_INDEX u_axis = IsParametric ? U_AXIS : FIRST_X_AXIS;
		AXIS_INDEX v_axis = IsParametric ? V_AXIS : FIRST_Y_AXIS;
		if(!IsParametric) {
			/*{{{  check ranges */
			/* give error if xrange badly set from missing datafile error
			 * parametric fn can still set ranges
			 * if there are no fns, we'll report it later as 'nothing to plot'
			 */

			// check that xmin -> xmax is not too small
			AxisCheckedExtendEmptyRange(FIRST_X_AXIS, "x range is invalid");
			AxisCheckedExtendEmptyRange(FIRST_Y_AXIS, "y range is invalid");
			/*}}} */
		}
		if(IsParametric && !some_data_files) {
			/*{{{  set ranges */
			/* parametric fn can still change x/y range */
			AxA[FIRST_X_AXIS].SetAutoscale(GPVL, -GPVL);
			AxA[FIRST_Y_AXIS].SetAutoscale(GPVL, -GPVL);
			/*}}} */
		}
		/*{{{  figure ranges, taking logs etc into account */
		u_min = LogValueChecked(u_axis, AxA[u_axis].Range.low, "x range");
		u_max = LogValueChecked(u_axis, AxA[u_axis].Range.upp, "x range");
		v_min = LogValueChecked(v_axis, AxA[v_axis].Range.low, "y range");
		v_max = LogValueChecked(v_axis, AxA[v_axis].Range.upp, "y range");
		/*}}} */
		if(Samples1 < 2 || Samples2 < 2 || iso_samples_1 < 2 || iso_samples_2 < 2) {
			GpGg.IntErrorNoCaret("samples or iso_samples < 2. Must be at least 2.");
		}

		/* start over */
		p_plot = P_First3DPlot;
		rC.CToken = begin_token;
		rC.P.P_PlotIterator = rC.CheckForIteration();
		if(hidden3d) {
			u_step = (u_max - u_min) / (iso_samples_1 - 1);
			v_step = (v_max - v_min) / (iso_samples_2 - 1);
		}
		else {
			u_step = (u_max - u_min) / (Samples1 - 1);
			v_step = (v_max - v_min) / (Samples2 - 1);
		}
		u_isostep = (u_max - u_min) / (iso_samples_1 - 1);
		v_isostep = (v_max - v_min) / (iso_samples_2 - 1);
		/* Read through functions */
		while(true) {
			if(crnt_param == 0 && !was_definition)
				start_token = rC.CToken;
			if(rC.IsDefinition()) {
				rC.Define();
				if(rC.Eq(","))
					rC.CToken++;
				was_definition = true;
				continue;
			}
			else {
				AtType * at_ptr;
				char * name_str;
				was_definition = false;
				// Forgive trailing comma on a multi-element plot command
				if(rC.EndOfCommand() || p_plot == NULL) {
					int_warn(rC.CToken, "ignoring trailing comma in plot command");
					break;
				}
				// Check for a sampling range
				// Currently we are supporting only sampling of pseudofile '+' and
				// this loop is for functions only, so the sampling range is ignored
				ParseRange(SAMPLE_AXIS, rC);
				rC.P_DummyFunc = &plot3D_func;
				name_str = rC.P.StringOrExpress(rC, &at_ptr);
				if(!name_str) { /* func to plot */
					/*{{{  evaluate function */
					iso_curve * this_iso = p_plot->iso_crvs;
					int num_sam_to_use, num_iso_to_use;

					/* crnt_param is used as the axis number.  As the
					 * axis array indices are ordered z, y, x, we have
					 * to count *backwards*, starting starting at 2,
					 * to properly store away contents to x, y and
					 * z. The following little gimmick does that. */
					if(IsParametric)
						crnt_param = (crnt_param + 2) % 3;
					plot3D_func.at = at_ptr;
					num_iso_to_use = iso_samples_2;
					num_sam_to_use = hidden3d ? iso_samples_1 : Samples1;
					GpGg.CalculateSetOfIsolines((AXIS_INDEX)crnt_param, false, &this_iso, v_axis, v_min, v_isostep, num_iso_to_use, u_axis, u_min, u_step, num_sam_to_use);
					if(!hidden3d) {
						num_iso_to_use = iso_samples_1;
						num_sam_to_use = Samples2;
						GpGg.CalculateSetOfIsolines((AXIS_INDEX)crnt_param, true, &this_iso, u_axis, u_min, u_isostep, num_iso_to_use, v_axis, v_min, v_step, num_sam_to_use);
					}
					/*}}} */
				} /* end of ITS A FUNCTION TO PLOT */
				/* we saved it from first pass */
				rC.CToken = p_plot->token;

				/* we may have seen this one data file in multiple iterations */
				i = p_plot->iteration;
				do {
					p_plot = p_plot->next_sp;
				} while(p_plot && p_plot->token == rC.CToken && p_plot->iteration == i);
			}
			// Iterate-over-plot mechanism
			if(crnt_param == 0 && next_iteration(rC.P.P_PlotIterator)) {
				if(rC.P.P_PlotIterator->iteration_current <= highest_iteration) {
					rC.CToken = start_token;
					continue;
				}
			}
			if(crnt_param == 0)
				rC.P.CleanupPlotIterator();
			if(rC.Eq(",")) {
				rC.CToken++;
				if(crnt_param == 0)
					rC.P.P_PlotIterator = rC.CheckForIteration();
			}
			else
				break;
		}
		if(IsParametric) {
			// Now actually fix the plot triplets to be single plots.
			parametric_3dfixup(P_First3DPlot, &plot_num);
		}
	} // some functions 
	// if first_3dplot is NULL, we have no functions or data at all.
	// This can happen if you type "splot x=5", since x=5 is a variable assignment.
	if(plot_num == 0 || !P_First3DPlot) {
		IntErrorCurToken("no functions or data to plot");
	}
	AxisCheckedExtendEmptyRange(FIRST_X_AXIS, "All points x value undefined");
	RevertAndUnlogRange(FIRST_X_AXIS);
	AxisCheckedExtendEmptyRange(FIRST_Y_AXIS, "All points y value undefined");
	RevertAndUnlogRange(FIRST_Y_AXIS);
	AxisCheckedExtendEmptyRange(FIRST_Z_AXIS, splot_map ? 0 : "All points z value undefined"); // Suppress warning message if splot_map
	RevertAndUnlogRange(FIRST_Z_AXIS);
	AxA[FIRST_X_AXIS].SetupTics(20);
	AxA[FIRST_Y_AXIS].SetupTics(20);
	AxA[FIRST_Z_AXIS].SetupTics(20);
	if(splot_map) {
		AxA[SECOND_X_AXIS].SetupTics(20);
		AxA[SECOND_Y_AXIS].SetupTics(20);
	}
	SetPlotWithPalette(plot_num, MODE_SPLOT);
	if(is_plot_with_palette()) {
		SetCbMinMax();
		AxisCheckedExtendEmptyRange(COLOR_AXIS, "All points of colorbox value undefined");
		AxA[COLOR_AXIS].SetupTics(20);
	}
	if(plot_num == 0 || !P_First3DPlot) {
		IntErrorCurToken("no functions or data to plot");
	}
	// Creates contours if contours are to be plotted as well
	if(draw_contour) {
		SurfacePoints * p_plot;
		for(p_plot = P_First3DPlot, i = 0; i < plot_num; p_plot = p_plot->next_sp, i++) {
			if(p_plot->contours) {
				gnuplot_contours * cntrs = p_plot->contours;
				while(cntrs) {
					gnuplot_contours * cntr = cntrs;
					cntrs = cntrs->next;
					free(cntr->coords);
					free(cntr);
				}
				p_plot->contours = NULL;
			}
			// Make sure this one can be contoured
			if(oneof4(p_plot->plot_style, VECTOR, IMAGE, RGBIMAGE, RGBA_IMAGE))
				continue;
			// Allow individual surfaces to opt out of contouring 
			if(p_plot->opt_out_of_contours)
				continue;
			if(!p_plot->has_grid_topology) {
				int_warn(NO_CARET, "Cannot contour non grid data. Please use \"set dgrid3d\".");
			}
			else if(p_plot->plot_type == DATA3D) {
				p_plot->contours = Contour(p_plot->num_iso_read, p_plot->iso_crvs);
			}
			else {
				p_plot->contours = Contour(iso_samples_2, p_plot->iso_crvs);
			}
		}
	}                       /* draw_contour */
	/* Images don't fit the grid model.  (The image data correspond
	 * to pixel centers.)  To make image work in hidden 3D, add
	 * another non-visible phantom surface of only four points
	 * outlining the image.  Opt out of hidden3d for the {RGB}IMAGE
	 * to avoid processing large amounts of data.
	 */
	if(hidden3d && plot_num) {
		SurfacePoints * p_plot = P_First3DPlot;
		do {
			if(oneof2(p_plot->plot_style, IMAGE, RGBIMAGE) &&
				(p_plot->image_properties.nrows > 0 && p_plot->image_properties.ncols > 0) && !(p_plot->opt_out_of_hidden3d)) {
				SurfacePoints * new_plot = sp_alloc(2, 0, 0, 2);
				// Construct valid 2 x 2 parallelogram.
				new_plot->num_iso_read = 2;
				new_plot->iso_crvs->p_count = 2;
				new_plot->iso_crvs->next->p_count = 2;
				new_plot->next_sp = p_plot->next_sp;
				p_plot->next_sp = new_plot;
				// Set up hidden3d behavior, no visible lines but opaque to items behind the parallelogram.
				new_plot->plot_style = SURFACEGRID;
				new_plot->opt_out_of_surface = true;
				new_plot->opt_out_of_contours = true;
				new_plot->has_grid_topology = true;
				new_plot->hidden3d_top_linetype = LT_NODRAW;
				new_plot->plot_type = DATA3D;
				new_plot->opt_out_of_hidden3d = false;
				// Compute the geometry of the phantom
				ProcessImage(term, p_plot, IMG_UPDATE_CORNERS);
				// Advance over the phantom
				++plot_num;
				p_plot = p_plot->next_sp;
			}
			p_plot = p_plot->next_sp;
		} while(p_plot);
	}
	//
	// the following ~9 lines were moved from the end of the
	// function to here, as do_3dplot calles term->text, which
	// itself might process input events in mouse enhanced
	// terminals. For redrawing to work, line capturing and
	// setting the plot3d_num must already be done before
	// entering do_3dplot(). Thu Jan 27 23:54:49 2000 (joze)
	//
	// if we get here, all went well, so record the line for replot
	if(rC.PlotToken != -1) {
		// note that m_capture also frees the old replot_line
		rC.MCapture(&rC.P_ReplotLine, rC.PlotToken, rC.CToken - 1);
		rC.PlotToken = -1;
		Ev.FillGpValString("GPVAL_LAST_PLOT", rC.P_ReplotLine);
	}
	//
	// record that all went well
	//
	plot3d_num = plot_num;
	// perform the plot
	if(table_mode)
		print_3dtable(plot_num);
	else {
		Do3DPlot(term, P_First3DPlot, plot_num, 0);
		// after do_3dplot(), AxA[] and max_array[].min
		// contain the plotting range actually used (rounded
		// to tic marks, not only the min/max data values)
		// --> save them now for writeback if requested
		SaveWritebackAllAxes();
		// update GPVAL_ variables available to user
		Ev.UpdateGpValVariables(1);
		// Mark these plots as safe for quick refresh
		SetRefreshOk(E_REFRESH_OK_3D, plot_num);
	}
}

/*
 * The hardest part of this routine is collapsing the FUNC plot types in the
 * list (which are gauranteed to occur in (x,y,z) triplets while preserving
 * the non-FUNC type plots intact.  This means we have to work our way
 * through various lists.  Examples (hand checked):
 * start_plot:F1->F2->F3->NULL ==> F3->NULL
 * start_plot:F1->F2->F3->F4->F5->F6->NULL ==> F3->F6->NULL
 * start_plot:F1->F2->F3->D1->D2->F4->F5->F6->D3->NULL ==>
 * F3->D1->D2->F6->D3->NULL
 *
 * x and y ranges now fixed in eval_3dplots
 */
static void parametric_3dfixup(SurfacePoints * pStartPlot, int * pPlotNum)
{
	SurfacePoints * xp;
	SurfacePoints * new_list;
	SurfacePoints * free_list = NULL;
	SurfacePoints ** last_pointer = &new_list;
	int i, surface;
	/*
	 * Ok, go through all the plots and move FUNC3D types together.  Note:
	 * this originally was written to look for a NULL next pointer, but
	 * gnuplot wants to be sticky in grabbing memory and the right number of
	 * items in the plot list is controlled by the plot_num variable.
	 *
	 * Since gnuplot wants to do this sticky business, a free_list of
	 * SurfacePoints is kept and then tagged onto the end of the plot list
	 * as this seems more in the spirit of the original memory behavior than
	 * simply freeing the memory.  I'm personally not convinced this sort of
	 * concern is worth it since the time spent computing points seems to
	 * dominate any garbage collecting that might be saved here...
	 */
	new_list = xp = pStartPlot;
	for(surface = 0; surface < *pPlotNum; surface++) {
		if(xp->plot_type == FUNC3D) {
			SurfacePoints * yp = xp->next_sp;
			SurfacePoints * zp = yp->next_sp;
			// Here's a FUNC3D parametric function defined as three parts.
			// Go through all the points and assign the x's and y's from xp and
			// yp to zp. min/max already done
			iso_curve * xicrvs = xp->iso_crvs;
			iso_curve * yicrvs = yp->iso_crvs;
			iso_curve * zicrvs = zp->iso_crvs;
			(*pPlotNum) -= 2;
			assert(INRANGE < OUTRANGE && OUTRANGE < UNDEFINED);
			while(zicrvs) {
				GpCoordinate * xpoints = xicrvs->points;
				GpCoordinate * ypoints = yicrvs->points;
				GpCoordinate * zpoints = zicrvs->points;
				for(i = 0; i < zicrvs->p_count; ++i) {
					zpoints[i].x = xpoints[i].z;
					zpoints[i].y = ypoints[i].z;
					SETMAX(zpoints[i].type, xpoints[i].type);
					SETMAX(zpoints[i].type, ypoints[i].type);
				}
				xicrvs = xicrvs->next;
				yicrvs = yicrvs->next;
				zicrvs = zicrvs->next;
			}
			// add xp and yp to head of free list
			assert(xp->next_sp == yp);
			yp->next_sp = free_list;
			free_list = xp;
			// add zp to tail of new_list 
			*last_pointer = zp;
			last_pointer = &(zp->next_sp);
			xp = zp->next_sp;
		}
		else { // its a data plot 
			assert(*last_pointer == xp); // think this is true !
			last_pointer = &(xp->next_sp);
			xp = xp->next_sp;
		}
	}
	// Ok, append free list and write first_plot 
	*last_pointer = free_list;
	GpGg.P_First3DPlot = new_list;
}

static void load_contour_label_options(GpTextLabel * contour_label)
{
	lp_style_type * lp = &(contour_label->lp_properties);
	SETIFZ(contour_label->font, gp_strdup(GpGg.P_ClabelFont));
	lp->p_interval = GpGg.clabel_interval;
	lp->flags |= LP_SHOW_POINTS;
	GpGg.LpParse(GpGg.Gp__C, *lp, LP_ADHOC, true);
}

