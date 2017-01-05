/* GNUPLOT - graph3d.c */

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
 * AUTHORS
 *
 *   Original Software:
 *       Gershon Elber and many others.
 *
 * 19 September 1992  Lawrence Crowl  (crowl@cs.orst.edu)
 * Added user-specified bases for log scaling.
 *
 * 3.6 - split graph3d.c into graph3d.c (graph),
 *                            util3d.c (intersections, etc)
 *                            hidden3d.c (hidden-line removal code)
 *
 */
#include <gnuplot.h>
#pragma hdrstop

//GpGraphics3D Gp3Gr; // @global

#define f_max(a, b) MAX((a), (b))
#define f_min(a, b) MIN((a), (b))
#define i_inrange(z, a, b) inrange((z), (a), (b))
#define apx_eq(x, y) (fabs(x-y) < 0.001)
#define ABS(x) ((x) >= 0 ? (x) : -(x))
#define SQR(x) ((x) * (x))

static int    KeyEntryHeight;    /* bigger of t->VChr, pointsize*t->v_tick */
static int    KeyTitleHeight;
static int    KeyTitleExtra;     /* allow room for subscript/superscript */
static int    ptitl_cnt;
static int    max_ptitl_len;
static int    titlelin;
static int    key_sample_width;
static int    key_rows;
static int    key_cols;
static int    key_col_wth;
static int    yl_ref;
static double ktitle_lines = 0;
static bool   can_pm3d = false;
//
// put entries in the key
//
static void key_sample_line(int xl, int yl);
static void key_sample_point(int xl, int yl, int pointtype);
//static void key_sample_line_pm3d(SurfacePoints * plot, int xl, int yl);
//static void key_sample_point_pm3d(SurfacePoints * plot, int xl, int yl, int pointtype);
static void check3d_for_variable_color(SurfacePoints * plot, GpCoordinate * point);
//
// Define the boundary of the plot
// These are computed at each call to do_plot, and are constant over
// the period of one do_plot. They actually only change when the term
// type changes and when the 'set size' factors change.
//
// Boundary and scale factors, in user coordinates
//
/* There are several z's to take into account - I hope I get these right !
 *
 * ceiling_z is the highest z in use
 * floor_z   is the lowest z in use
 * base_z is the z of the base
 * min3d_z is the lowest z of the graph area
 * max3d_z is the highest z of the graph area
 *
 * ceiling_z is either max3d_z or base_z, and similarly for floor_z
 * There should be no part of graph drawn outside
 * min3d_z:max3d_z  - apart from arrows, perhaps
 */
// x and y input range endpoints where the three axes are to be
// displayed (left, front-left, and front-right edges of the cube)
static double xaxis_y, yaxis_x, zaxis_x, zaxis_y;
// ... and the same for the back, right, and front corners
static double back_x, back_y;
static double right_x, right_y;
static double front_x, front_y;
//
// the penalty for convenience of using tic_gen to make callbacks
// to tick routines is that we cannot pass parameters very easily.
// We communicate with the tick_callbacks using static variables
//
// unit vector (terminal coords)
static double tic_unitx, tic_unity, tic_unitz;
static void do_3dkey_layout(legend_key *key, int * xinkey, int * yinkey);
//static void plot3d_points(SurfacePoints* plot);
static void plot3d_vectors(SurfacePoints* plot);
// no pm3d for impulses
//static void plot3d_lines_pm3d(SurfacePoints* plot);
static void get_surface_cbminmax(SurfacePoints * plot, double * cbmin, double * cbmax);
//static void check_corner_height(GpCoordinate * point, double height[2][2], double depth[2][2]);
static int find_maxl_cntr(gnuplot_contours * contours, int * count);
//
// calculate the number and max-width of the keys for an splot.
// Note that a blank line is issued after each set of contours
//
//static int find_maxl_keys3d(SurfacePoints * plots, int count, int * kcnt)
int GpGraphics::FindMaxlKeys3D(SurfacePoints * plots, int count, int * kcnt)
{
	int len, surf;
	int mlen = 0;
	int cnt = 0;
	SurfacePoints * this_plot = plots;
	for(surf = 0; surf < count; this_plot = this_plot->next_sp, surf++) {
		/* we draw a main entry if there is one, and we are
		 * drawing either surface, or unlabeled contours
		 */
		if(this_plot->title && *this_plot->title && !this_plot->title_is_suppressed && !this_plot->title_position) {
			++cnt;
			len = estimate_strlen(this_plot->title);
			if(len > mlen)
				mlen = len;
		}
		if(draw_contour && !clabel_onecolor && this_plot->contours && this_plot->plot_style != LABELPOINTS) {
			len = find_maxl_cntr(this_plot->contours, &cnt);
			if(len > mlen)
				mlen = len;
		}
	}
	ASSIGN_PTR(kcnt, cnt);
	return (mlen);
}

static int find_maxl_cntr(gnuplot_contours * contours, int * count)
{
	int cnt;
	int mlen, len;
	gnuplot_contours * cntrs = contours;
	mlen = cnt = 0;
	while(cntrs) {
		if(cntrs->isNewLevel) {
			len = estimate_strlen(cntrs->label) - strspn(cntrs->label, " ");
			if(len)
				cnt++;
			if(len > mlen)
				mlen = len;
		}
		cntrs = cntrs->next;
	}
	*count += cnt;
	return (mlen);
}
//
// borders of plotting area
// computed once on every call to do_plot
//
//static void boundary3d(SurfacePoints * plots, int count)
void GpGadgets::Boundary3D(SurfacePoints * plots, int count)
{
	legend_key * key = &keyT;
	GpTermEntry * t = term;
	int ytlen, i;
	titlelin = 0;
	key_sample_width = (key->swidth >= 0) ? (int)(key->swidth * t->HChr + t->HTic) : 0;
	KeyEntryHeight = (int)(t->VTic * 1.25 * key->vert_factor);
	if(KeyEntryHeight < (int)t->VChr) {
		// is this reasonable ?
		KeyEntryHeight = (int)(t->VChr * key->vert_factor);
	}
	// count max_len key and number keys (plot-titles and contour labels) with len > 0
	max_ptitl_len = FindMaxlKeys3D(plots, count, &ptitl_cnt);
	ytlen = (int)(label_width(key->title.text, &i) - (key->swidth + 2));
	ktitle_lines = i;
	if(ytlen > max_ptitl_len)
		max_ptitl_len = ytlen;
	key_col_wth = (max_ptitl_len + 4) * t->HChr + key_sample_width;
	if(LMrg.scalex == screen)
		PlotBounds.xleft = (int)(LMrg.x * (float)t->xmax + 0.5);
	else if(LMrg.x >= 0)
		PlotBounds.xleft = (int)(LMrg.x * (float)t->HChr + 0.5);
	else
		PlotBounds.xleft = t->HChr * 2 + t->HTic;
	if(RMrg.scalex == screen)
		PlotBounds.xright = (int)(RMrg.x * (float)t->xmax + 0.5);
	else // No tic label on the right side, so ignore RMrg
		PlotBounds.xright = (int)(XSz * t->xmax - t->HChr * 2 - t->HTic);
	key_rows = ptitl_cnt;
	key_cols = 1;
	if(key_rows > key->maxrows && key->maxrows > 0) {
		key_rows = key->maxrows;
		key_cols = (ptitl_cnt - 1)/key_rows + 1;
	}
	if(key->visible)
		if((key->region == GPKEY_AUTO_EXTERIOR_MARGIN || key->region == GPKEY_AUTO_EXTERIOR_LRTBC) && key->margin == GPKEY_BMARGIN) {
			if(ptitl_cnt > 0) {
				// calculate max no cols, limited by label-length
				key_cols = (int)(PlotBounds.xright - PlotBounds.xleft) / ((max_ptitl_len + 4) * t->HChr + key_sample_width);
				// HBB 991019: fix division by zero problem
				SETIFZ(key_cols, 1);
				key_rows = (int)((ptitl_cnt - 1)/ key_cols) + 1;
				// Limit the number of rows if requested by user
				if(key_rows > key->maxrows && key->maxrows > 0)
					key_rows = key->maxrows;
				// now calculate actual no cols depending on no rows
				key_cols = (int)((ptitl_cnt - 1)/ key_rows) + 1;
				key_col_wth = (int)(PlotBounds.xright - PlotBounds.xleft) / key_cols;
			}
			else {
				key_rows = key_cols = key_col_wth = 0;
			}
		}
	// Sanity check top and bottom margins, in case the user got confused
	if(BMrg.scalex == screen && TMrg.scalex == screen)
		if(BMrg.x > TMrg.x) {
			Exchange(&BMrg.x, &TMrg.x);
		}
	// this should also consider the view and number of lines in xformat || yformat || xlabel || ylabel
	if(BMrg.scalex == screen)
		PlotBounds.ybot = (int)(BMrg.x * (float)t->ymax + 0.5);
	else if(splot_map && BMrg.x >= 0)
		PlotBounds.ybot = (int)((float)t->VChr * BMrg.x);
	else
		PlotBounds.ybot = (int)(t->VChr * 2.5 + 1);
	if(key->visible)
		if(key_rows && (key->region == GPKEY_AUTO_EXTERIOR_MARGIN || key->region == GPKEY_AUTO_EXTERIOR_LRTBC) && key->margin == GPKEY_BMARGIN)
			PlotBounds.ybot += (int)(key_rows * KeyEntryHeight + ktitle_lines * t->VChr);
	{
		const size_t tl = sstrlen(title.text);
		if(tl) {
			titlelin++;
			for(size_t ti = 0; ti < tl; ti++)
				if(title.text[ti] == '\\')
					titlelin++;
		}
	}
	if(TMrg.scalex == screen)
		PlotBounds.ytop = (int)(TMrg.x * (float)t->ymax + 0.5);
	else // FIXME: Why no provision for TMrg in terms of character height?
		PlotBounds.ytop = (int)(YSz * t->ymax - t->VChr * (titlelin + 1.5) - 1);
	if(key->visible)
		if(key->region == GPKEY_AUTO_INTERIOR_LRTBC || ((key->region == GPKEY_AUTO_EXTERIOR_LRTBC || key->region == GPKEY_AUTO_EXTERIOR_MARGIN) && key->margin == GPKEY_RMARGIN)) {
			// calculate max no rows, limited by PlotBounds.ytop-PlotBounds.ybot
			i = (int)((int)(PlotBounds.ytop - PlotBounds.ybot) / t->VChr - 1 - ktitle_lines);
			if(i > key->maxrows && key->maxrows > 0)
				i = key->maxrows;
			// HBB 20030321: div by 0 fix like above
			SETIFZ(i, 1);
			if(ptitl_cnt > i) {
				key_cols = (int)((ptitl_cnt - 1)/ i) + 1;
				// now calculate actual no rows depending on no cols
				key_rows = (int)((ptitl_cnt - 1) / key_cols) + 1;
			}
		}
	if(key->visible)
		if((key->region == GPKEY_AUTO_EXTERIOR_LRTBC || key->region == GPKEY_AUTO_EXTERIOR_MARGIN) && key->margin == GPKEY_RMARGIN) {
			int key_width = key_col_wth * (key_cols - 1) + key_col_wth - 2 * t->HChr;
			if(RMrg.scalex != screen)
				PlotBounds.xright -= key_width;
		}
	if(key->visible)
		if((key->region == GPKEY_AUTO_EXTERIOR_LRTBC || key->region == GPKEY_AUTO_EXTERIOR_MARGIN) && key->margin == GPKEY_LMARGIN) {
			int key_width = key_col_wth * (key_cols - 1) + key_col_wth - 2 * t->HChr;
			if(LMrg.scalex != screen)
				PlotBounds.xleft += key_width;
		}
	if(!splot_map && AspectRatio3D > 0) {
		const int height = (PlotBounds.ytop - PlotBounds.ybot);
		const int width  = (PlotBounds.xright - PlotBounds.xleft);
		if(height > width) {
			PlotBounds.ybot += (height-width)/2;
			PlotBounds.ytop -= (height-width)/2;
		}
		else {
			PlotBounds.xleft += (width-height)/2;
			PlotBounds.xright -= (width-height)/2;
		}
	}
	if(LMrg.scalex != screen)
		PlotBounds.xleft += (int)(t->xmax * XOffs);
	if(RMrg.scalex != screen)
		PlotBounds.xright += (int)(t->xmax * XOffs);
	if(TMrg.scalex != screen)
		PlotBounds.ytop += (int)(t->ymax * YOffs);
	if(BMrg.scalex != screen)
		PlotBounds.ybot += (int)(t->ymax * YOffs);
	xmiddle = (PlotBounds.xright + PlotBounds.xleft) / 2;
	ymiddle = (PlotBounds.ytop + PlotBounds.ybot) / 2;
	// HBB: Magic number alert!
	xscaler = ((PlotBounds.xright - PlotBounds.xleft) * 4L) / 7L;
	yscaler = ((PlotBounds.ytop - PlotBounds.ybot) * 4L) / 7L;
	// EAM Aug 2006 - Allow explicit control via set {}margin screen
	if(TMrg.scalex == screen || BMrg.scalex == screen)
		yscaler = (int)((PlotBounds.ytop - PlotBounds.ybot) / surface_scale);
	if(RMrg.scalex == screen || LMrg.scalex == screen)
		xscaler = (int)((PlotBounds.xright - PlotBounds.xleft) / surface_scale);
	// EAM Jul 2010 - prevent infinite loop or divide-by-zero if scaling is bad
	SETIFZ(yscaler, 1);
	SETIFZ(xscaler, 1);
	// HBB 20011011: 'set size {square|ratio}' for splots
	if(splot_map && AspectRatio != 0.0) {
		double current_aspect_ratio;
		if(AspectRatio < 0 && (GetX().GetRange()) != 0.0) {
			current_aspect_ratio = -AspectRatio * fabs((GetY().GetRange()) / (GetX().GetRange()));
		}
		else
			current_aspect_ratio = AspectRatio;
		//{{{  set aspect ratio if valid and sensible
		if(current_aspect_ratio >= 0.01 && current_aspect_ratio <= 100.0) {
			double current = (double)yscaler / xscaler;
			double required = current_aspect_ratio * t->VTic / t->HTic;
			if(current > required)
				yscaler = (int)(xscaler * required); // too tall
			else
				xscaler = (int)(yscaler / required); // too wide
		}
	}
	// Set default clipping
	if(splot_map)
		P_Clip = &PlotBounds;
	else if(term->flags & TERM_CAN_CLIP)
		P_Clip = NULL;
	else
		P_Clip = &Canvas;
}

//static bool get_arrow3d(arrow_def* arrow, int* sx, int* sy, int* ex, int* ey)
bool GpGadgets::GetArrow3D(arrow_def* arrow, int* sx, int* sy, int* ex, int* ey)
{
	Map3DPosition(arrow->start, sx, sy, "arrow");
	if(arrow->type == arrow_end_relative) {
		Map3DPositionR(arrow->end, ex, ey, "arrow");
		*ex += *sx;
		*ey += *sy;
	}
	else if(arrow->type == arrow_end_oriented) {
		double aspect = (double)term->VTic / (double)term->HTic;
		double radius;
		int junkw, junkh;
#ifdef WIN32
		if(strcmp(term->name, "windows") == 0)
			aspect = 1.;
#endif
		if(arrow->end.scalex != screen && arrow->end.scalex != character && !splot_map)
			return false;
		Map3DPositionR(arrow->end, &junkw, &junkh, "arrow");
		radius = junkw;
		*ex = (int)(*sx + cos(DEG2RAD * arrow->angle) * radius);
		*ey = (int)(*sy + sin(DEG2RAD * arrow->angle) * radius * aspect);
	}
	else {
		Map3DPosition(arrow->end, ex, ey, "arrow");
	}
	return true;
}

//static void place_labels3d(GpTextLabel * listhead, int layer)
void GpGadgets::PlaceLabels3D(GpTextLabel * listhead, int layer)
{
	int x, y;
	term->pointsize(PtSz);
	for(GpTextLabel * p_label = listhead; p_label; p_label = p_label->next) {
		if(p_label->layer == layer) {
			if(layer == LAYER_PLOTLABELS) {
				double xx, yy;
				Map3DXY(p_label->place.x, p_label->place.y, p_label->place.z, &xx, &yy);
				x = (int)xx;
				y = (int)yy;
				// Only clip in 2D 
				if(splot_map && ClipPoint(x, y))
					continue;
			}
			else
				Map3DPosition(p_label->place, &x, &y, "label");
			WriteLabel(term, x, y, p_label);
		}
	}
}

//static void place_arrows3d(int layer)
void GpGadgets::PlaceArrows3D(int layer)
{
	arrow_def * this_arrow;
	BoundingBox * clip_save = P_Clip;
	/* Allow arrows to run off the plot, so long as they are still on the Canvas */
	P_Clip = (term->flags & TERM_CAN_CLIP) ? 0 : &Canvas;
	for(this_arrow = first_arrow; this_arrow != NULL; this_arrow = this_arrow->next) {
		if(this_arrow->arrow_properties.layer == layer) {
			int sx, sy, ex, ey;
			if(GetArrow3D(this_arrow, &sx, &sy, &ex, &ey)) {
				ApplyLpProperties(term, &(this_arrow->arrow_properties.lp_properties));
				ApplyHeadProperties(this_arrow->arrow_properties);
				DrawClipArrow(term, sx, sy, ex, ey, this_arrow->arrow_properties.head);
			}
			else {
				FPRINTF((stderr, "place_arrows3d: skipping out-of-bounds arrow\n"));
			}
		}
	}
	P_Clip = clip_save;
}
//
// we precalculate features of the key, to save lots of nested
// ifs in code - x,y = user supplied or computed position of key taken to be inner edge of a line sample
//
static int key_sample_left;     /* offset from x for left of line sample */
static int key_sample_right;    /* offset from x for right of line sample */
static int key_point_offset;    /* offset from x for point sample */
static int key_text_left;       /* offset from x for left-justified text */
static int key_text_right;      /* offset from x for right-justified text */
static int key_size_left;       /* distance from x to left edge of box */
static int key_size_right;      /* distance from x to right edge of box */

//do_3dplot
void GpGadgets::Do3DPlot(GpTermEntry * pT, SurfacePoints * pPlotList, int pcount/* count of pPlotList in linked list */, int quick/* !=0 means plot only axes etc., for quick rotation */)
{
	int surface;
	SurfacePoints * p_plot = NULL;
	int xl, yl;
	int xl_save, yl_save, xl_prev, yl_prev;
	transform_matrix mat;
	int key_count;
	bool key_pass = false;
	legend_key * p_key = &keyT;
	bool pm3d_order_depth = 0;
	/* Initiate transformation matrix using the global view variables. */
	if(splot_map)
		SplotMapActivate(GpC);
	mat_rot_z(surface_rot_z, trans_mat);
	mat_rot_x(surface_rot_x, mat);
	mat_mult(trans_mat, trans_mat, mat);
	mat_scale(surface_scale / 2.0, surface_scale / 2.0, surface_scale / 2.0, mat);
	mat_mult(trans_mat, trans_mat, mat);
	/* The extrema need to be set even when a surface is not being
	 * drawn.   Without this, gnuplot used to assume that the X and
	 * Y axis started at zero.   -RKC
	 */
	if(IsPolar)
		GpGg.IntError(GpC, NO_CARET, "Cannot splot in polar GpCoordinate system.");
	// absolute or relative placement of xyplane along z
	base_z = xyplane.IsAbsolute ? LogValue((AXIS_INDEX)0, xyplane.Z) : (GetZ().Range.low - (GetZ().GetRange()) * xyplane.Z);
	// If we are to draw the bottom grid make sure zmin is updated properly.
	if(GetX().ticmode || GetY().ticmode || SomeGridSelected()) {
		if(GetZ().Range.low > GetZ().Range.upp) {
			floor_z   = MAX(GetZ().Range.low, base_z);
			ceiling_z = MIN(GetZ().Range.upp, base_z);
		}
		else {
			floor_z   = MIN(GetZ().Range.low, base_z);
			ceiling_z = MAX(GetZ().Range.upp, base_z);
		}
	}
	else {
		floor_z = GetZ().Range.low;
		ceiling_z = GetZ().Range.upp;
	}
	if(GetX().Range.low == GetX().Range.upp)
		GpGg.IntError(GpC, NO_CARET, "x_min3d should not equal x_max3d!");
	if(GetY().Range.low == GetY().Range.upp)
		GpGg.IntError(GpC, NO_CARET, "y_min3d should not equal y_max3d!");
	if(GetZ().Range.low == GetZ().Range.upp)
		GpGg.IntError(GpC, NO_CARET, "z_min3d should not equal z_max3d!");
	TermStartPlot(pT);
	(pT->layer)(TERM_LAYER_3DPLOT);
	screen_ok = false;
	// Sync point for epslatex text positioning 
	(pT->layer)(TERM_LAYER_BACKTEXT);
	// now compute boundary for plot 
	Boundary3D(pPlotList, pcount);
	AxA[FIRST_X_AXIS].SetScaleAndRange(PlotBounds.xleft, PlotBounds.xright);
	AxA[FIRST_Y_AXIS].SetScaleAndRange(PlotBounds.ybot, PlotBounds.ytop);
	AxA[FIRST_Z_AXIS].SetScaleAndRange((uint)floor_z, (uint)ceiling_z);
	// SCALE FACTORS 
	Scale3d.Set(2.0 / GetX().GetRange(), 2.0 / GetY().GetRange(), 2.0 / (ceiling_z - floor_z) * surface_zscale);
	// Allow 'set view equal xy' to adjust rendered length of the X and/or Y axes. 
	// FIXME EAM - This only works correctly if the GpCoordinate system of the       
	// terminal itself is isotropic.  E.g. x11 does not work because the x and y   
	// coordinates always run from 0-4095 regardless of the shape of the window.   
	Center3d.Set(0.0, 0.0, 0.0);
	if(AspectRatio3D >= 2) {
		if(Scale3d.y > Scale3d.x) {
			Center3d.y = 1.0 - Scale3d.x/Scale3d.y;
			Scale3d.y = Scale3d.x;
		}
		else if(Scale3d.x > Scale3d.y) {
			Center3d.x = 1.0 - Scale3d.y/Scale3d.x;
			Scale3d.x = Scale3d.y;
		}
		if(AspectRatio3D >= 3)
			Scale3d.z = Scale3d.x;
	}
	// Without this the rotation center would be located at 
	// the bottom of the plot. This places it in the middle.
	Center3d.z =  -(ceiling_z - floor_z) / 2.0 * Scale3d.z + 1;
	// Needed for mousing by outboard terminal drivers 
	if(splot_map) {
		GpAxis & r_ax_x = AxA[FIRST_X_AXIS];
		GpAxis & r_ax_y = AxA[FIRST_Y_AXIS];
		int xl, xr, yb, yt;
		Map3DXY(r_ax_x.Range.low, r_ax_y.Range.low, 0.0, &xl, &yb);
		Map3DXY(r_ax_x.Range.upp, r_ax_y.Range.upp, 0.0, &xr, &yt);
		r_ax_x.SetScaleAndRange(xl, xr);
		r_ax_y.SetScaleAndRange(yb, yt);
	}
	// Initialize palette 
	if(!quick) {
		can_pm3d = is_plot_with_palette() && !MakePalette(pT) && ((pT->flags & TERM_NULL_SET_COLOR) == 0);
	}
	// Give a chance for rectangles to be behind everything else 
	PlaceObjects(pT, first_object, LAYER_BEHIND, 3);
	ApplyLpProperties(pT, &BorderLp);   // border linetype 
	// must come before using draw_3d_graphbox() the first time 
	Setup3DBoxCorners();
	// DRAW GRID AND BORDER 
	// Original behaviour: draw entire grid in back, if 'set grid back': 
	// HBB 20040331: but not if in hidden3d mode 
	if(splot_map && BorderLayer != LAYER_FRONT)
		Draw3DGraphBox(pT, pPlotList, pcount, BORDERONLY, LAYER_BACK);
	else if(!hidden3d && (grid_layer == LAYER_BACK))
		Draw3DGraphBox(pT, pPlotList, pcount, ALLGRID, LAYER_BACK);
	else if(!hidden3d && (grid_layer == LAYER_BEHIND))
		/* Default layering mode.  Draw the back part now, but not if
		* hidden3d is in use, because that relies on all isolated
		* lines being output after all surfaces have been defined. */
		Draw3DGraphBox(pT, pPlotList, pcount, BACKGRID, LAYER_BACK);
	else if(hidden3d && BorderLayer == LAYER_BEHIND)
		Draw3DGraphBox(pT, pPlotList, pcount, ALLGRID, LAYER_BACK);
	/* Clipping in 'set view map' mode should be like 2D clipping */
	if(splot_map) {
		int map_x1, map_y1, map_x2, map_y2;
		Map3DXY(GetX().Range.low, GetY().Range.low, base_z, &map_x1, &map_y1);
		Map3DXY(GetX().Range.upp, GetY().Range.upp, base_z, &map_x2, &map_y2);
		PlotBounds.xleft = map_x1;
		PlotBounds.xright = map_x2;
		PlotBounds.ybot = map_y2;
		PlotBounds.ytop = map_y1;
	}
	/* Mar 2009 - This is a change!
	 * Define the clipping area in 3D to lie between the left-most and
	 * right-most graph box edges.  This is introduced for the benefit of
	 * zooming in the Canvas terminal.  It may or may not make any practical
	 * difference for other terminals.  If it causes problems, then we will need
	 * a separate BoundingBox structure to track the actual 3D graph box.
	 */
	else {
		int xl, xb, xr, xf, yl, yb, yr, yf;
		Map3DXY(zaxis_x, zaxis_y, base_z, &xl, &yl);
		Map3DXY(back_x, back_y, base_z, &xb, &yb);
		Map3DXY(right_x, right_y, base_z, &xr, &yr);
		Map3DXY(front_x, front_y, base_z, &xf, &yf);
		PlotBounds.xleft = MIN(xl, xb); /* Always xl? */
		PlotBounds.xright = MAX(xb, xr); /* Always xr? */
	}
	/* PLACE TITLE */
	if(title.text != 0) {
		uint x, y;
		int tmpx, tmpy;
		if(splot_map) { /* case 'set view map' */
			int map_x1, map_y1, map_x2, map_y2;
			int tics_len = 0;
			if(GetX().ticmode & TICS_MIRROR) {
				tics_len = (int)(GetX().ticscale * ((GetX().Flags & GpAxis::fTicIn) ? -1 : 1) * (pT->VTic));
				if(tics_len < 0) tics_len = 0;  /* take care only about upward tics */
			}
			Map3DXY(GetX().Range.low, GetY().Range.low, base_z, &map_x1, &map_y1);
			Map3DXY(GetX().Range.upp, GetY().Range.upp, base_z, &map_x2, &map_y2);
			// Distance between the title base line and graph top line or the upper part of tics is as given by character height:
			Map3DPositionR(title.offset, &tmpx, &tmpy, "3dplot");
#define DEFAULT_Y_DISTANCE 1.0
			x = (uint)((map_x1 + map_x2) / 2 + tmpx);
			y = (uint)(map_y1 + tics_len + tmpy + (DEFAULT_Y_DISTANCE + titlelin - 0.5) * (pT->VChr));
#undef DEFAULT_Y_DISTANCE
		}
		else { /* usual 3d set view ... */
			Map3DPositionR(title.offset, &tmpx, &tmpy, "3dplot");
			x = (uint)((PlotBounds.xleft + PlotBounds.xright) / 2 + tmpx);
			y = (uint)(PlotBounds.ytop + tmpy + titlelin * (pT->HChr));
		}
		ignore_enhanced(title.noenhanced);
		ApplyPm3DColor(pT, &(title.textcolor));
		/* PM: why there is JUST_TOP and not JUST_BOT? We should draw above baseline!
		 * But which terminal understands that? It seems vertical justification does
		 * not work... */
		pT->DrawMultiline(x, y, title.text, CENTRE, JUST_TOP, 0, title.font);
		reset_textcolor(&(title.textcolor));
		ignore_enhanced(false);
	}
	/* PLACE TIMEDATE */
	if(timelabel.text) {
		char str[MAX_LINE_LEN+1];
		time_t now;
		int tmpx, tmpy;
		uint x, y;
		Map3DPositionR(timelabel.offset, &tmpx, &tmpy, "3dplot");
		x = pT->VChr + tmpx;
		y = (uint)(timelabel_bottom ? YOffs * GetY().Range.upp + tmpy + pT->VChr : 
			PlotBounds.ytop + tmpy - pT->VChr);
		time(&now);
		strftime(str, MAX_LINE_LEN, timelabel.text, localtime(&now));
		if(timelabel_rotate && (*pT->text_angle)(TEXT_VERTICAL)) {
			pT->DrawMultiline(x, y, str, timelabel_bottom ? LEFT : RIGHT, JUST_TOP, TEXT_VERTICAL, timelabel.font);
			(*pT->text_angle)(0);
		}
		else {
			pT->DrawMultiline(x, y, str, LEFT, timelabel_bottom ? JUST_BOT : JUST_TOP, 0, timelabel.font);
		}
	}
	/* Add 'back' color box */
	if(!quick && can_pm3d && is_plot_with_colorbox() && ColorBox.layer == LAYER_BACK)
		DrawColorSmoothBox(pT, MODE_SPLOT);
	PlaceObjects(pT, first_object, LAYER_BACK, 3); /* Add 'back' rectangles */
	PlaceLabels3D(first_label, LAYER_BACK); /* PLACE LABELS */
	PlaceArrows3D(LAYER_BACK); /* PLACE ARROWS */
	/* Sync point for epslatex text positioning */
	(pT->layer)(TERM_LAYER_FRONTTEXT);
	if(hidden3d && draw_surface && !quick) {
		init_hidden_line_removal();
		reset_hidden_line_removal();
	}
	/* WORK OUT KEY POSITION AND SIZE */
	do_3dkey_layout(p_key, &xl, &yl);
	/* "set p_key opaque" requires two passes, with the p_key drawn in the second pass */
	xl_save = xl; yl_save = yl;
SECOND_KEY_PASS:
	/* This tells the Canvas, qt, and svg terminals to restart the plot   */
	/* count so that p_key titles are in sync with the pPlotList they describe. */
	(*pT->layer)(TERM_LAYER_RESET_PLOTNO);
	/* Key box */
	if(p_key->visible) {
		/* In two-pass mode, we blank out the p_key area after the graph	*/
		/* is drawn and then redo the p_key in the blank area.		*/
		if(key_pass && pT->fillbox && !(pT->flags & TERM_NULL_SET_COLOR)) {
			t_colorspec background_fill(TC_LT, LT_BACKGROUND, 0.0);
			(*pT->set_color)(&background_fill);
			(*pT->fillbox)(FS_OPAQUE, p_key->bounds.xleft, p_key->bounds.ybot, p_key->bounds.xright - p_key->bounds.xleft, p_key->bounds.ytop - p_key->bounds.ybot);
		}
		if(p_key->box.l_type > LT_NODRAW &&  p_key->bounds.ytop != p_key->bounds.ybot) {
			ApplyLpProperties(pT, &p_key->box);
			{
				newpath(pT);
				clip_move(p_key->bounds.xleft, p_key->bounds.ybot);
				ClipVector(pT, p_key->bounds.xleft, p_key->bounds.ytop);
				ClipVector(pT, p_key->bounds.xright, p_key->bounds.ytop);
				ClipVector(pT, p_key->bounds.xright, p_key->bounds.ybot);
				ClipVector(pT, p_key->bounds.xleft, p_key->bounds.ybot);
				closepath(pT);
			}
			// draw a horizontal line between p_key title and first entry  JFi
			clip_move(p_key->bounds.xleft, p_key->bounds.ytop - KeyTitleHeight - KeyTitleExtra);
			ClipVector(pT, p_key->bounds.xright, p_key->bounds.ytop - KeyTitleHeight - KeyTitleExtra);
		}
		if(p_key->title.text) {
			int center = (p_key->bounds.xright + p_key->bounds.xleft) / 2;
			if(p_key->textcolor.type == TC_RGB && p_key->textcolor.value < 0)
				ApplyPm3DColor(pT, &(p_key->box.pm3d_color));
			else
				ApplyPm3DColor(pT, &(p_key->textcolor));
			pT->DrawMultiline(center, p_key->bounds.ytop - (KeyTitleExtra + pT->VChr)/2, p_key->title.text, CENTRE, JUST_TOP, 0, p_key->title.font ? p_key->title.font : p_key->font);
			(*pT->linetype)(LT_BLACK);
		}
	}
	// DRAW SURFACES AND CONTOURS 
	if(!key_pass)
		if(hidden3d && (hidden3d_layer == LAYER_BACK) && draw_surface && !quick) {
			(pT->layer)(TERM_LAYER_BEFORE_PLOT);
			plot3d_hidden(pPlotList, pcount);
			(pT->layer)(TERM_LAYER_AFTER_PLOT);
		}

	/* Set up bookkeeping for the individual p_key titles */
#define NEXT_KEY_LINE()					\
	do {						    \
		if(++key_count >= key_rows) {			 \
			yl = yl_ref; xl += key_col_wth; key_count = 0;	\
		} else						    \
			yl -= KeyEntryHeight; \
	} while(0)
	key_count = 0;
	yl_ref = yl -= KeyEntryHeight / 2;    /* centralise the keys */

	/* PM January 2005: The mistake of missing blank lines in the data file is
	 * so frequently made (see questions at comp.graphics.apps.gnuplot) that it
	 * really deserves this warning. But don'pT show it too often --- only if it
	 * is a single surface in the plot.
	 */
	if(pcount == 1 && pPlotList->num_iso_read == 1 && can_pm3d && (pPlotList->plot_style == PM3DSURFACE || PM3D_IMPLICIT == Pm3D.implicit))
		fprintf(stderr,
		    "  Warning: Single isoline (scan) is not enough for a pm3d plot.\n\t   Hint: Missing blank lines in the data file? See 'help pm3d' and FAQ.\n");
	pm3d_order_depth = (can_pm3d && !draw_contour && Pm3D.direction == PM3D_DEPTH);
	if(pm3d_order_depth) {
		pm3d_depth_queue_clear();
	}
	p_plot = pPlotList;
	if(!quick)
		for(surface = 0; surface < pcount; p_plot = p_plot->next_sp, surface++) {
			/* just an abbreviation */
			bool lkey, draw_this_surface;
			/* Skip over abortive data structures */
			if(p_plot->plot_type == NODATA)
				continue;
			/* Sync point for start of new curve (used by svg, post, ...) */
			(pT->layer)(TERM_LAYER_BEFORE_PLOT);
			if(!key_pass)
				if(can_pm3d && PM3D_IMPLICIT == Pm3D.implicit)
					Pm3DDrawOne(pT, p_plot);
			lkey = (p_key->visible && p_plot->title && p_plot->title[0] && !p_plot->title_is_suppressed);
			draw_this_surface = (draw_surface && !p_plot->opt_out_of_surface);
			/* User-specified p_key locations can use the 2D code */
			if(p_plot->title_position && p_plot->title_position->scalex != character) {
				xl_prev = xl;
				yl_prev = yl;
				Map3DPosition(*p_plot->title_position, &xl, &yl, "p_key sample");
				xl -=  (p_key->just == GPKEY_LEFT) ? key_text_left : key_text_right;
			}
			if(lkey) {
				if(p_key->textcolor.type != TC_DEFAULT)
					ApplyPm3DColor(pT, &p_key->textcolor); /* Draw p_key text in same color as p_key title */
				else
					(*pT->linetype)(LT_BLACK); /* Draw p_key text in black */
				ignore_enhanced(p_plot->title_no_enhanced);
				KeyText(xl, yl, p_plot->title);
				ignore_enhanced(false);
			}
			ApplyLpProperties(pT, &(p_plot->lp_properties));
			/* First draw the graph plot itself */
			if(!key_pass)
				switch(p_plot->plot_style) {
					case BOXES: /* can'pT do boxes in 3d yet so use impulses */
					case FILLEDCURVES:
					case IMPULSES:
					    if(!(hidden3d && draw_this_surface))
						    Plot3DImpulses(p_plot);
					    break;
					case STEPS: /* HBB: I think these should be here */
					case FILLSTEPS:
					case FSTEPS:
					case HISTEPS:
					case SURFACEGRID:
					case LINES:
					    if(draw_this_surface) {
						    if(!hidden3d || p_plot->opt_out_of_hidden3d)
							    Plot3DLinesPm3D(p_plot);
					    }
					    break;
					case YERRORLINES: /* ignored; treat like points */
					case XERRORLINES: /* ignored; treat like points */
					case XYERRORLINES: /* ignored; treat like points */
					case YERRORBARS: /* ignored; treat like points */
					case XERRORBARS: /* ignored; treat like points */
					case XYERRORBARS: /* ignored; treat like points */
					case BOXXYERROR: /* HBB: ignore these as well */
					case BOXERROR:
					case CANDLESTICKS: /* HBB: ditto */
					case BOXPLOT:
					case FINANCEBARS:
#ifdef EAM_OBJECTS
					case CIRCLES:
					case ELLIPSES:
#endif
					case POINTSTYLE:
					case DOTS:
					    if(draw_this_surface) {
						    if(!hidden3d || p_plot->opt_out_of_hidden3d)
							    Plot3DPoints(pT, p_plot);
					    }
					    break;
					case LINESPOINTS:
					    if(draw_this_surface) {
						    if(!hidden3d || p_plot->opt_out_of_hidden3d) {
							    Plot3DLinesPm3D(p_plot);
							    Plot3DPoints(pT, p_plot);
						    }
					    }
					    break;
					case VECTOR:
					    if(!hidden3d || p_plot->opt_out_of_hidden3d)
						    plot3d_vectors(p_plot);
					    break;
					case PM3DSURFACE:
					    if(draw_this_surface) {
						    if(can_pm3d && PM3D_IMPLICIT != Pm3D.implicit) {
							    Pm3DDrawOne(pT, p_plot);
							    if(!pm3d_order_depth)
								    Pm3DDepthQueueFlush(pT); // draw plot immediately
						    }
					    }
					    break;

					case LABELPOINTS:
					    if(draw_this_surface) {
						    if(!hidden3d || p_plot->opt_out_of_hidden3d)
							    PlaceLabels3D(p_plot->labels->next, LAYER_PLOTLABELS);
					    }
					    break;

					case HISTOGRAMS: /* Cannot happen */
					    break;

					case IMAGE:
					    // Plot image using projection of 3D plot coordinates to 2D viewing coordinates
					    p_plot->image_properties.type = IC_PALETTE;
					    ProcessImage(pT, p_plot, IMG_PLOT);
					    break;
					case RGBIMAGE:
					    // Plot image using projection of 3D plot coordinates to 2D viewing coordinates.
					    p_plot->image_properties.type = IC_RGB;
					    ProcessImage(pT, p_plot, IMG_PLOT);
					    break;
					case RGBA_IMAGE:
					    p_plot->image_properties.type = IC_RGBA;
					    ProcessImage(pT, p_plot, IMG_PLOT);
					    break;
					case PARALLELPLOT:
					    GpGg.IntError(GpC, NO_CARET, "plot style parallelaxes not supported in 3D");
					    break;

					case PLOT_STYLE_NONE:
					case TABLESTYLE:
					    /* cannot happen */
					    break;
				} /* switch(plot-style) plot proper */

			/* Next draw the p_key sample */
			if(lkey)
				switch(p_plot->plot_style) {
					case BOXES: /* can'pT do boxes in 3d yet so use impulses */
					case FILLEDCURVES:
					case IMPULSES:
					    if(!(hidden3d && draw_this_surface))
						    key_sample_line(xl, yl);
					    break;
					case STEPS: /* HBB: I think these should be here */
					case FILLSTEPS:
					case FSTEPS:
					case HISTEPS:
					case SURFACEGRID:
					case LINES:
					    /* Normal case (surface) */
					    if(draw_this_surface)
						    KeySampleLinePm3D(pT, p_plot, xl, yl);
					    /* Contour plot with no surface, all contours use the same linetype */
					    else if(p_plot->contours != NULL && clabel_onecolor) {
						    key_sample_line(xl, yl);
					    }
					    break;
					case YERRORLINES: /* ignored; treat like points */
					case XERRORLINES: /* ignored; treat like points */
					case XYERRORLINES: /* ignored; treat like points */
					case YERRORBARS: /* ignored; treat like points */
					case XERRORBARS: /* ignored; treat like points */
					case XYERRORBARS: /* ignored; treat like points */
					case BOXXYERROR: /* HBB: ignore these as well */
					case BOXERROR:
					case CANDLESTICKS: /* HBB: ditto */
					case BOXPLOT:
					case FINANCEBARS:
#ifdef EAM_OBJECTS
					case CIRCLES:
					case ELLIPSES:
#endif
					case POINTSTYLE:
					    if(draw_this_surface)
						    KeySamplePointPm3D(pT, p_plot, xl, yl, p_plot->lp_properties.p_type);
					    break;
					case LABELPOINTS:
					    if((p_plot->labels->lp_properties.flags & LP_SHOW_POINTS)) {
						    ApplyLpProperties(pT, &p_plot->labels->lp_properties);
						    key_sample_point(xl, yl, p_plot->labels->lp_properties.p_type);
					    }
					    break;
					case LINESPOINTS:
					    if(draw_this_surface) {
						    if(p_plot->lp_properties.l_type != LT_NODRAW)
							    KeySampleLinePm3D(pT, p_plot, xl, yl);
						    KeySamplePointPm3D(pT, p_plot, xl, yl, p_plot->lp_properties.p_type);
					    }
					    break;
					case DOTS:
					    if(draw_this_surface)
						    KeySamplePointPm3D(pT, p_plot, xl, yl, -1);
					    break;
					case VECTOR:
					    KeySampleLinePm3D(pT, p_plot, xl, yl);
					    break;
					case PLOT_STYLE_NONE:
					/* cannot happen */
					default:
					    break;
				} /* switch(plot-style) p_key sample */
			/* move down one line in the p_key... */
			if(lkey)
				NEXT_KEY_LINE();
			/* but not if the plot title was drawn somewhere else */
			if(p_plot->title_position && p_plot->title_position->scalex != character) {
				xl = xl_prev;
				yl = yl_prev;
			}
			/* Draw contours for previous surface */
			if(draw_contour && p_plot->contours != NULL) {
				gnuplot_contours * cntrs = p_plot->contours;
				lp_style_type thiscontour_lp_properties;
				static char * thiscontour_label = NULL;
				int ic = 1; /* ic will index the contour linetypes */
				thiscontour_lp_properties = p_plot->lp_properties;
				/* EAM April 2015 - Disabled this, but I'm not really sure */
				/* Maybe this is now a dashtype correction?                */
				/* thiscontour_lp_properties.l_type += (hidden3d ? 1 : 0); */
				ApplyLpProperties(pT, &(thiscontour_lp_properties));
				while(cntrs) {
					if(!clabel_onecolor && cntrs->isNewLevel) {
						if(p_key->visible && !p_plot->title_is_suppressed && p_plot->plot_style != LABELPOINTS) {
							(*pT->linetype)(LT_BLACK);
							KeyText(xl, yl, cntrs->label);
						}
						if(thiscontour_lp_properties.pm3d_color.type == TC_Z)
							pT->SetColor(CB2Gray(Z2CB(cntrs->z) ) );
						else {
							lp_style_type ls = thiscontour_lp_properties;
							if(thiscontour_lp_properties.l_type == LT_COLORFROMCOLUMN) {
								thiscontour_lp_properties.l_type = 0;
							}
							ic++; /* Increment linetype used for contour */
							if(prefer_line_styles) {
								lp_use_properties(&ls, p_plot->hidden3d_top_linetype + ic);
							}
							else {
								/* The linetype itself is passed to hidden3d processing */
								/* EAM Apr 2015 - not sure why this is "ic -1" but otherwise */
								/* hidden3d contours are colored off by one (Bug #1603). */
								thiscontour_lp_properties.l_type = p_plot->hidden3d_top_linetype + ic -1;
								/* otherwise the following would be sufficient */
								load_linetype(&ls, p_plot->hidden3d_top_linetype + ic);
							}
							thiscontour_lp_properties.pm3d_color = ls.pm3d_color;
							ApplyLpProperties(pT, &thiscontour_lp_properties);
						}
						if(p_key->visible && !p_plot->title_is_suppressed && !(p_plot->plot_style == LABELPOINTS)) {
							switch(p_plot->plot_style) {
								case IMPULSES:
								case LINES:
								case LINESPOINTS:
								case BOXES:
								case FILLEDCURVES:
								case VECTOR:
								case STEPS:
								case FSTEPS:
								case HISTEPS:
								case PM3DSURFACE:
								    key_sample_line(xl, yl);
								    break;
								case POINTSTYLE:
								    key_sample_point(xl, yl, p_plot->lp_properties.p_type);
								    break;
								case DOTS:
								    key_sample_point(xl, yl, -1);
								    break;
								default:
								    break;
							} /* switch */
							NEXT_KEY_LINE();
						} /* p_key */
					} /* clabel_onecolor */
					/* now draw the contour */
					if(!key_pass)
						switch(p_plot->plot_style) {
							/* treat boxes like impulses: */
							case BOXES:
							case FILLEDCURVES:
							case VECTOR:
							case IMPULSES:
							    Cntr3DImpulses(cntrs, &thiscontour_lp_properties);
							    break;
							case STEPS:
							case FSTEPS:
							case HISTEPS:
							/* treat all the above like 'lines' */
							case LINES:
							case PM3DSURFACE:
							    Cntr3DLines(cntrs, &thiscontour_lp_properties);
							    break;
							case LINESPOINTS:
							    Cntr3DLines(cntrs, &thiscontour_lp_properties);
							/* Fall through to draw the points */
							case DOTS:
							case POINTSTYLE:
							    Cntr3DPoints(cntrs, &thiscontour_lp_properties);
							    break;
							case LABELPOINTS:
							    if(cntrs->isNewLevel) {
								    char * c = &cntrs->label[strspn(cntrs->label, " ")];
								    free(thiscontour_label);
								    thiscontour_label = gp_strdup(c);
							    }
							    // cntr3d_lines(cntrs, &thiscontour_lp_properties);
							    Cntr3DLabels(cntrs, thiscontour_label, p_plot->labels);
							    break;

							default:
							    break;
						} /*switch */

					cntrs = cntrs->next;
				}
			}
			// Sync point for end of this curve (used by svg, post, ...) 
			(pT->layer)(TERM_LAYER_AFTER_PLOT);
		}
	if(!key_pass)
		if(pm3d_order_depth) {
			Pm3DDepthQueueFlush(pT); /* draw pending pPlotList */
		}
	if(!key_pass)
		if(hidden3d && (hidden3d_layer == LAYER_FRONT) && draw_surface && !quick) {
			(pT->layer)(TERM_LAYER_BEFORE_PLOT);
			plot3d_hidden(pPlotList, pcount);
			(pT->layer)(TERM_LAYER_AFTER_PLOT);
		}
	/* Draw grid and border.
	 * The 1st case allows "set border behind" to override hidden3d processing.
	 * The 2nd case either leaves everything to hidden3d or forces it to the front.
	 * The 3rd case is the non-hidden3d default - draw back pieces (done earlier),
	 * then the graph, and now the front pieces.
	 */
	if(hidden3d && BorderLayer == LAYER_BEHIND)
		Draw3DGraphBox(pT, pPlotList, pcount, FRONTGRID, LAYER_FRONT);
	else if(hidden3d || grid_layer == LAYER_FRONT)
		Draw3DGraphBox(pT, pPlotList, pcount, ALLGRID, LAYER_FRONT);
	else if(grid_layer == LAYER_BEHIND)
		Draw3DGraphBox(pT, pPlotList, pcount, FRONTGRID, LAYER_FRONT);
	// Go back and draw the legend in a separate pass if "p_key opaque" 
	if(p_key->visible && p_key->front && !key_pass) {
		key_pass = true;
		xl = xl_save; yl = yl_save;
		goto SECOND_KEY_PASS;
	}
	/* Add 'front' color box */
	if(!quick && can_pm3d && is_plot_with_colorbox() && ColorBox.layer == LAYER_FRONT)
		DrawColorSmoothBox(pT, MODE_SPLOT);
	PlaceObjects(pT, first_object, LAYER_FRONT, 3); /* Add 'front' rectangles */
	PlaceLabels3D(first_label, LAYER_FRONT); /* PLACE LABELS */
	PlaceArrows3D(LAYER_FRONT); /* PLACE ARROWS */
#ifdef USE_MOUSE
	/* finally, store the 2d projection of the x and y axis, to enable zooming by mouse */
	{
		int x, y;
		Map3DXY(GetX().Range.low, GetY().Range.low, base_z, &axis3d_o_x, &axis3d_o_y); // @?
		Map3DXY(GetX().Range.upp, GetY().Range.low, base_z, &x, &y); // @?
		axis3d_x_dx = x - axis3d_o_x;
		axis3d_x_dy = y - axis3d_o_y;
		Map3DXY(GetX().Range.low, GetY().Range.upp, base_z, &x, &y);
		axis3d_y_dx = x - axis3d_o_x;
		axis3d_y_dy = y - axis3d_o_y;
	}
#endif
	/* Release the palette if we have used one (PostScript only?) */
	if(is_plot_with_palette() && pT->previous_palette)
		pT->previous_palette();
	TermEndPlot(pT);
	if(hidden3d && draw_surface) {
		term_hidden_line_removal();
	}
	if(splot_map)
		SplotMapDeactivate(GpC);
}
//
// plot3d_impulses:
// Plot the surfaces in IMPULSES style
//
//static void plot3d_impulses(SurfacePoints * plot)
void GpGadgets::Plot3DImpulses(SurfacePoints * plot)
{
	int x, y, xx0, yy0;             /* point in terminal coordinates */
	iso_curve * icrvs = plot->iso_crvs;
	int colortype = plot->lp_properties.pm3d_color.type;
	if(colortype == TC_RGB)
		SetRgbColorConst(term, plot->lp_properties.pm3d_color.lt);
	while(icrvs) {
		GpCoordinate * points = icrvs->points;
		for(int i = 0; i < icrvs->p_count; i++) {
			check3d_for_variable_color(plot, &points[i]);
			switch(points[i].type) {
				case INRANGE:
			    {
				    Map3DXY(points[i].x, points[i].y, points[i].z, &x, &y);
				    if(GetZ().InRange(0.0)) {
					    Map3DXY(points[i].x, points[i].y, 0.0, &xx0, &yy0);
				    }
				    else if(inrange(GetZ().Range.low, 0.0, points[i].z)) {
					    Map3DXY(points[i].x, points[i].y, GetZ().Range.low, &xx0, &yy0);
				    }
				    else {
					    Map3DXY(points[i].x, points[i].y, GetZ().Range.upp, &xx0, &yy0);
				    }
				    clip_move(xx0, yy0);
				    ClipVector(term, x, y);
				    break;
			    }
				case OUTRANGE:
			    {
				    if(!GetX().InRange(points[i].x) || !GetY().InRange(points[i].y))
					    break;
				    if(GetZ().InRange(0.0)) {
					    // zero point is INRANGE
					    Map3DXY(points[i].x, points[i].y, 0.0, &xx0, &yy0);
					    // must cross z = GetZ().min or GetZ().max limits
					    if(inrange(GetZ().Range.low, 0.0, points[i].z) && GetZ().Range.low != 0.0 && 
							GetZ().Range.low != points[i].z) {
						    Map3DXY(points[i].x, points[i].y, GetZ().Range.low, &x, &y);
					    }
					    else {
						    Map3DXY(points[i].x, points[i].y, GetZ().Range.upp, &x, &y);
					    }
				    }
				    else {
					    // zero point is also OUTRANGE 
					    if(inrange(GetZ().Range.low, 0.0, points[i].z) && inrange(GetZ().Range.upp, 0.0, points[i].z)) {
						    // crosses z = GetZ().min or GetZ().max limits 
						    Map3DXY(points[i].x, points[i].y, GetZ().Range.upp, &x, &y);
						    Map3DXY(points[i].x, points[i].y, GetZ().Range.low, &xx0, &yy0);
					    }
					    else {
						    break; // doesn't cross z = GetZ().min or GetZ().max limits
					    }
				    }
				    clip_move(xx0, yy0);
				    ClipVector(term, x, y);
				    break;
			    }
				default: /* just a safety */
				case UNDEFINED: {
				    break;
			    }
			}
		}
		icrvs = icrvs->next;
	}
}
//
// Plot the surfaces in LINES style
// We want to always draw the lines in the same direction, otherwise when
// we draw an adjacent box we might get the line drawn a little differently and we get splotches
//
//static void plot3d_lines(SurfacePoints * plot)
void GpGadgets::Plot3DLines(SurfacePoints * plot)
{
	int i;
	int x, y, xx0, yy0; /* point in terminal coordinates */
	double clip_x, clip_y, clip_z;
	iso_curve * icrvs = plot->iso_crvs;
	GpCoordinate * points;
	bool rgb_from_column;
	// These are handled elsewhere.
	if(plot->has_grid_topology && hidden3d)
		return;
	// These don't need to be drawn at all
	if(plot->lp_properties.l_type == LT_NODRAW)
		return;
	rgb_from_column = plot->pm3d_color_from_column && plot->lp_properties.pm3d_color.type == TC_RGB && plot->lp_properties.pm3d_color.value < 0.0;
	while(icrvs) {
		enum coord_type prev = UNDEFINED; /* type of previous plot */
		for(i = 0, points = icrvs->points; i < icrvs->p_count; i++) {
			if(rgb_from_column)
				SetRgbColorVar(term, (uint)points[i].CRD_COLOR);
			else if(plot->lp_properties.pm3d_color.type == TC_LINESTYLE) {
				plot->lp_properties.pm3d_color.lt = (int)(points[i].CRD_COLOR);
				ApplyPm3DColor(term, &(plot->lp_properties.pm3d_color));
			}
			switch(points[i].type) {
				case INRANGE: {
				    Map3DXY(points[i].x, points[i].y, points[i].z, &x, &y);
				    if(prev == INRANGE) {
					    ClipVector(term, x, y);
				    }
				    else {
					    if(prev == OUTRANGE) {
						    /* from outrange to inrange */
						    if(!ClipLines1) {
							    clip_move(x, y);
						    }
						    else {
								// Calculate intersection point and draw vector from there
							    Edge3DIntersect(&points[i-1], &points[i], &clip_x, &clip_y, &clip_z);
							    Map3DXY(clip_x, clip_y, clip_z, &xx0, &yy0);
							    clip_move(xx0, yy0);
							    ClipVector(term, x, y);
						    }
					    }
					    else {
						    clip_move(x, y);
					    }
				    }
				    break;
			    }
				case OUTRANGE: {
				    if(prev == INRANGE) {
					    /* from inrange to outrange */
					    if(ClipLines1) {
							// Calculate intersection point and draw vector to it
						    Edge3DIntersect(&points[i-1], &points[i], &clip_x, &clip_y, &clip_z);
						    Map3DXY(clip_x, clip_y, clip_z, &xx0, &yy0);
						    ClipVector(term, xx0, yy0);
					    }
				    }
				    else if(prev == OUTRANGE) {
					    /* from outrange to outrange */
					    if(ClipLines2) {
						    double lx[2], ly[2], lz[2]; /* two edge points */
						    // Calculate the two 3D intersection points if present
						    if(TwoEdge3DIntersect(&points[i-1], &points[i], lx, ly, lz)) {
							    Map3DXY(lx[0], ly[0], lz[0], &x, &y);
							    Map3DXY(lx[1], ly[1], lz[1], &xx0, &yy0);
							    clip_move(x, y);
							    ClipVector(term, xx0, yy0);
						    }
					    }
				    }
				    break;
			    }
				case UNDEFINED: {
				    break;
			    }
				default:
				    int_warn(NO_CARET, "Unknown point type in plot3d_lines");
			}
			prev = points[i].type;
		}
		icrvs = icrvs->next;
	}
}
//
// this is basically the same function as above, but:
//   - it splits the bunch of scans in two sets corresponding to
//   the two scan directions.
//   - reorders the two sets -- from behind to front
//   - checks if inside on scan of a set the order should be inverted
//
//static void plot3d_lines_pm3d(SurfacePoints * plot)
void GpGadgets::Plot3DLinesPm3D(SurfacePoints * plot)
{
	iso_curve** icrvs_pair[2];
	int invert[2] = {0, 0};
	int n[2] = {0, 0};
	int i, set, scan;
	int x, y, xx0, yy0; /* point in terminal coordinates */
	double clip_x, clip_y, clip_z;
	GpCoordinate * points;
	enum coord_type prev = UNDEFINED;
	double z;
	// just a shortcut
	bool color_from_column = plot->pm3d_color_from_column;
	// If plot really uses RGB rather than pm3d colors, let plot3d_lines take over
	if(plot->lp_properties.pm3d_color.type == TC_RGB) {
		ApplyPm3DColor(term, &(plot->lp_properties.pm3d_color));
		Plot3DLines(plot);
		return;
	}
	else if(plot->lp_properties.pm3d_color.type == TC_LT) {
		Plot3DLines(plot);
		return;
	}
	else if(plot->lp_properties.pm3d_color.type == TC_LINESTYLE) {
		Plot3DLines(plot);
		return;
	}
	// These are handled elsewhere.
	if(plot->has_grid_topology && hidden3d)
		return;
	/* split the bunch of scans in two sets in
	 * which the scans are already depth ordered */
	pm3d_rearrange_scan_array(plot, icrvs_pair, &n[0], &invert[0], icrvs_pair + 1, &n[1], &invert[1]);
	for(set = 0; set < 2; set++) {
		int begin = 0;
		int step;
		if(invert[set]) {
			// begin is set below to the length of the scan - 1
			step = -1;
		}
		else {
			step = 1;
		}
		for(scan = 0; scan < n[set] && icrvs_pair[set]; scan++) {
			int cnt;
			iso_curve * icrvs = icrvs_pair[set][scan];
			if(invert[set]) {
				begin = icrvs->p_count - 1;
			}
			prev = UNDEFINED; /* type of previous plot */
			for(cnt = 0, i = begin, points = icrvs->points; cnt < icrvs->p_count; cnt++, i += step) {
				switch(points[i].type) {
					case INRANGE:
					    Map3DXY(points[i].x, points[i].y, points[i].z, &x, &y);
					    if(prev == INRANGE) {
						    if(color_from_column)
							    z =  (points[i - step].CRD_COLOR + points[i].CRD_COLOR) * 0.5;
						    else
							    z =  (Z2CB(points[i - step].z) + Z2CB(points[i].z)) * 0.5;
						    term->SetColor(CB2Gray(z) );
						    ClipVector(term, x, y);
					    }
					    else {
						    if(prev == OUTRANGE) {
							    /* from outrange to inrange */
							    if(!ClipLines1) {
								    clip_move(x, y);
							    }
							    else {
								    /*
								     * Calculate intersection point and draw vector from there
								     */
								    Edge3DIntersect(&points[i-step], &points[i], &clip_x, &clip_y, &clip_z);
								    Map3DXY(clip_x, clip_y, clip_z, &xx0, &yy0);
								    clip_move(xx0, yy0);
								    if(color_from_column)
									    z =  (points[i - step].CRD_COLOR + points[i].CRD_COLOR) * 0.5;
								    else
									    z =  (Z2CB(points[i - step].z) + Z2CB(points[i].z)) * 0.5;
								    term->SetColor(CB2Gray(z) );
								    ClipVector(term, x, y);
							    }
						    }
						    else {
							    clip_move(x, y);
						    }
					    }
					    break;
					case OUTRANGE:
					    if(prev == INRANGE) {
						    /* from inrange to outrange */
						    if(ClipLines1) {
							    /*
							     * Calculate intersection point and draw vector to it
							     */
							    Edge3DIntersect(&points[i-step], &points[i], &clip_x, &clip_y, &clip_z);
							    Map3DXY(clip_x, clip_y, clip_z, &xx0, &yy0);
							    if(color_from_column)
								    z =  (points[i - step].CRD_COLOR + points[i].CRD_COLOR) * 0.5;
							    else
								    z =  (Z2CB(points[i - step].z) + Z2CB(points[i].z)) * 0.5;
							    term->SetColor(CB2Gray(z));
							    ClipVector(term, xx0, yy0);
						    }
					    }
					    else if(prev == OUTRANGE) {
						    /* from outrange to outrange */
						    if(ClipLines2) {
							    /*
							     * Calculate the two 3D intersection points if present
							     */
							    double lx[2], ly[2], lz[2];
							    if(TwoEdge3DIntersect(&points[i-step], &points[i], lx, ly, lz)) {
								    Map3DXY(lx[0], ly[0], lz[0], &x, &y);
								    Map3DXY(lx[1], ly[1], lz[1], &xx0, &yy0);
								    clip_move(x, y);
								    if(color_from_column)
									    z =  (points[i - step].CRD_COLOR + points[i].CRD_COLOR) * 0.5;
								    else
									    z =  (Z2CB(points[i - step].z) + Z2CB(points[i].z)) * 0.5;
								    term->SetColor(CB2Gray(z) );
								    ClipVector(term, xx0, yy0);
							    }
						    }
					    }
					    break;
					case UNDEFINED:
					    break;
					default:
					    int_warn(NO_CARET, "Unknown point type in plot3d_lines");
				}
				prev = points[i].type;
			}
		}
	}
	free(icrvs_pair[0]);
	free(icrvs_pair[1]);
}
//
// plot3d_points:
// Plot the surfaces in POINTSTYLE style
//
//static void plot3d_points(SurfacePoints * plot)
void GpGadgets::Plot3DPoints(GpTermEntry * pT, SurfacePoints * plot)
{
	int i;
	int x, y;
	iso_curve * icrvs = plot->iso_crvs;
	/* Set whatever we can that applies to every point in the loop */
	if(plot->lp_properties.p_type == PT_CHARACTER) {
		ignore_enhanced(true);
		if(plot->labels->font && plot->labels->font[0])
			(*pT->set_font)(plot->labels->font);
		(*pT->justify_text)(CENTRE);
	}
	while(icrvs) {
		GpCoordinate * point;
		int colortype = plot->lp_properties.pm3d_color.type;
		/* Apply constant color outside of the loop */
		if(colortype == TC_RGB)
			SetRgbColorConst(pT, plot->lp_properties.pm3d_color.lt);
		for(i = 0; i < icrvs->p_count; i++) {
			point = &(icrvs->points[i]);
			if(point->type == INRANGE) {
				Map3DXY(point->x, point->y, point->z, &x, &y);
				if(!ClipPoint(x, y)) {
					check3d_for_variable_color(plot, point);
					if((plot->plot_style == POINTSTYLE || plot->plot_style == LINESPOINTS) && plot->lp_properties.p_size == PTSZ_VARIABLE)
						(*pT->pointsize)(PtSz * point->CRD_PTSIZE);
					/* This code is also used for "splot ... with dots" */
					if(plot->plot_style == DOTS)
						(*pT->point)(x, y, -1);
					/* Print special character rather than drawn symbol */
					if(plot->lp_properties.p_type == PT_CHARACTER) {
						ApplyPm3DColor(pT, &(plot->labels->textcolor));
						(*pT->put_text)(x, y, plot->lp_properties.p_char);
					}
					/* variable point type */
					if(plot->lp_properties.p_type == PT_VARIABLE) {
						(*pT->point)(x, y, (int)(point->CRD_PTTYPE) - 1);
					}
					/* The normal case */
					else if(plot->lp_properties.p_type >= 0)
						(*pT->point)(x, y, plot->lp_properties.p_type);
				}
			}
		}
		icrvs = icrvs->next;
	}
	// Return to initial state 
	if(plot->lp_properties.p_type == PT_CHARACTER) {
		if(plot->labels->font && plot->labels->font[0])
			(*pT->set_font)("");
		ignore_enhanced(false);
	}
}
//
// Plot a surface contour in IMPULSES style
//
//static void cntr3d_impulses(gnuplot_contours * cntr, lp_style_type * lp)
void GpGadgets::Cntr3DImpulses(gnuplot_contours * cntr, lp_style_type * lp)
{
	GpVertex vertex_on_surface, vertex_on_base;
	if(draw_contour & CONTOUR_SRF) {
		for(int i = 0; i < cntr->num_pts; i++) {
			Map3DXYZ(cntr->coords[i].x, cntr->coords[i].y, cntr->coords[i].z, &vertex_on_surface);
			Map3DXYZ(cntr->coords[i].x, cntr->coords[i].y, 0.0, &vertex_on_base);
			// HBB 20010822: Provide correct color-coding for "linetype palette" PM3D mode
			vertex_on_base.real_z = cntr->coords[i].z;
			Draw3DLine(term, &vertex_on_surface, &vertex_on_base, lp);
		}
	}
	else {
		Cntr3DPoints(cntr, lp); /* Must be on base grid, so do points. */
	}
}
//
// cntr3d_lines: Plot a surface contour in LINES style
// HBB NEW 20031218: changed to use move/vector() style polyline
// drawing. Gets rid of variable "previous_vertex"
//
//static void cntr3d_lines(gnuplot_contours * cntr, lp_style_type * lp)
void GpGadgets::Cntr3DLines(gnuplot_contours * cntr, lp_style_type * lp)
{
	int    i; // point index
	GpVertex this_vertex;
	// In the case of "set view map" (only) clip the contour lines to the graph
	BoundingBox * clip_save = P_Clip;
	if(splot_map)
		P_Clip = &PlotBounds;
	if(draw_contour & CONTOUR_SRF) {
		Map3DXYZ(cntr->coords[0].x, cntr->coords[0].y, cntr->coords[0].z, &this_vertex);
		// move slightly frontward, to make sure the contours are
		// visible in front of the the triangles they're in, if this is a hidden3d plot
		if(hidden3d && !this_vertex.IsUndefined())
			this_vertex.z += 1e-2;
		PolyLine3DStart(term, this_vertex);
		for(i = 1; i < cntr->num_pts; i++) {
			Map3DXYZ(cntr->coords[i].x, cntr->coords[i].y, cntr->coords[i].z, &this_vertex);
			// move slightly frontward, to make sure the contours are
			// visible in front of the the triangles they're in, if this is a hidden3d plot
			if(hidden3d && !this_vertex.IsUndefined())
				this_vertex.z += 1e-2;
			PolyLine3DNext(term, this_vertex, lp);
		}
	}
	if(draw_contour & CONTOUR_BASE) {
		Map3DXYZ(cntr->coords[0].x, cntr->coords[0].y, base_z, &this_vertex);
		this_vertex.real_z = cntr->coords[0].z;
		PolyLine3DStart(term, this_vertex);
		for(i = 1; i < cntr->num_pts; i++) {
			Map3DXYZ(cntr->coords[i].x, cntr->coords[i].y, base_z, &this_vertex);
			this_vertex.real_z = cntr->coords[i].z;
			PolyLine3DNext(term, this_vertex, lp);
		}
	}
	if(splot_map)
		P_Clip = clip_save;
}
//
// Plot a surface contour in POINTSTYLE style
//
//static void cntr3d_points(gnuplot_contours * cntr, lp_style_type * lp)
void GpGadgets::Cntr3DPoints(gnuplot_contours * cntr, lp_style_type * lp)
{
	GpVertex v;
	if(draw_contour & CONTOUR_SRF) {
		for(int i = 0; i < cntr->num_pts; i++) {
			Map3DXYZ(cntr->coords[i].x, cntr->coords[i].y, cntr->coords[i].z, &v);
			// move slightly frontward, to make sure the contours and
			// points are visible in front of the triangles they're
			// in, if this is a hidden3d plot 
			if(hidden3d && !v.IsUndefined())
				v.z += 1e-2;
			Draw3DPoint(term, v, lp);
		}
	}
	if(draw_contour & CONTOUR_BASE) {
		for(int i = 0; i < cntr->num_pts; i++) {
			Map3DXYZ(cntr->coords[i].x, cntr->coords[i].y, base_z, &v);
			// HBB 20010822: see above 
			v.real_z = cntr->coords[i].z;
			Draw3DPoint(term, v, lp);
		}
	}
}
//
// Place contour labels on a contour line at the base.
// These are the same labels that would be used in the key.
// The label density is controlled by the point interval property
//   splot FOO with labels point pi 20 nosurface
//
//static void cntr3d_labels(gnuplot_contours * cntr, char * level_text, GpTextLabel * label)
void GpGadgets::Cntr3DLabels(gnuplot_contours * cntr, char * level_text, GpTextLabel * pLabel)
{
	GpVertex v;
	lp_style_type * lp = &(pLabel->lp_properties);
	// Drawing a pLabel at every point would be too crowded
	const int interval = (lp->p_interval > 0) ? lp->p_interval : 999; // 999 - Place pLabel only at start point 
	if(draw_contour & CONTOUR_BASE) {
		for(int i = 0; i < cntr->num_pts; i++) {
			if(((i-clabel_start) % interval) == 0) { // Offset to avoid sitting on the border
				int    x, y;
				Map3DXY(cntr->coords[i].x, cntr->coords[i].y, base_z, &x, &y);
				pLabel->text = level_text;
				if(hidden3d) {
					Map3DXYZ(cntr->coords[i].x, cntr->coords[i].y, base_z, &v);
					v.real_z = cntr->coords[i].z;
					v.label = pLabel;
					draw_label_hidden(&v, lp, x, y);
				}
				else {
					WriteLabel(term, x, y, pLabel);
				}
				pLabel->text = 0; // Otherwise someone will try to free it
			}
		}
	}
}
//
// map xmin | xmax to 0 | 1 and same for y 0.1 avoids any rounding errors
//
#define MAP_HEIGHT_X(x) ((int)(((x)-GpGg.GetX().Range.low)/(GpGg.GetX().GetRange())+0.1))
#define MAP_HEIGHT_Y(y) ((int)(((y)-GpGg.GetY().Range.low)/(GpGg.GetY().GetRange())+0.1))
//
// if point is at corner, update height[][] and depth[][]
// we are still assuming that extremes of surfaces are at corners,
// but we are not assuming order of corners
//
//static void check_corner_height(GpCoordinate * p, double height[2][2], double depth[2][2])
void GpGadgets::CheckCornerHeight(GpCoordinate * p, double height[2][2], double depth[2][2])
{
	if(p->type == INRANGE) {
		// FIXME HBB 20010121: don't compare 'zero' to data values in absolute terms.
		if(FEqEps(p->x, GetX().Range.low) || FEqEps(p->x, GetX().Range.upp) &&  (FEqEps(p->y, GetY().Range.low) || FEqEps(p->y, GetY().Range.upp))) {
			uint x = MAP_HEIGHT_X(p->x);
			uint y = MAP_HEIGHT_Y(p->y);
			SETMAX(height[x][y], p->z);
			SETMIN(depth[x][y], p->z);
		}
	}
}
//
// work out where the axes and tics are drawn
//
//static void setup_3d_box_corners()
void GpGadgets::Setup3DBoxCorners()
{
	int quadrant = (int)(surface_rot_z / 90);
	if((quadrant + 1) & 2) {
		zaxis_x  = GetX().Range.upp;
		right_x  = GetX().Range.low;
		back_y   = GetY().Range.low;
		front_y  = GetY().Range.upp;
	}
	else {
		zaxis_x  = GetX().Range.low;
		right_x  = GetX().Range.upp;
		back_y   = GetY().Range.upp;
		front_y  = GetY().Range.low;
	}
	if(quadrant & 2) {
		zaxis_y = GetY().Range.upp;
		right_y = GetY().Range.low;
		back_x  = GetX().Range.upp;
		front_x  = GetX().Range.low;
	}
	else {
		zaxis_y = GetY().Range.low;
		right_y = GetY().Range.upp;
		back_x  = GetX().Range.low;
		front_x  = GetX().Range.upp;
	}
	if(surface_rot_x > 90) {
		// labels on the back axes
		yaxis_x = back_x;
		xaxis_y = back_y;
	}
	else {
		yaxis_x = front_x;
		xaxis_y = front_y;
	}
}
//
// Draw all elements of the 3d graph box, including borders, zeroaxes,
// tics, gridlines, ticmarks, axis labels and the base plane
//
void GpGadgets::Draw3DGraphBox(GpTermEntry * pT, SurfacePoints * pPlot, int plot_num, WHICHGRID whichgrid, int current_layer)
{
	int x, y; // point in terminal coordinates 
	BoundingBox * clip_save = P_Clip;
	P_Clip = &Canvas;
	if(DrawBorder && splot_map) {
		if(BorderLayer == current_layer) {
			ApplyLpProperties(pT, &BorderLp);
			if((DrawBorder & 15) == 15)
				newpath(pT);
			Map3DXY(zaxis_x, zaxis_y, base_z, &x, &y);
			clip_move(x, y);
			Map3DXY(back_x, back_y, base_z, &x, &y);
			if(DrawBorder & 2)
				ClipVector(pT, x, y);
			else
				clip_move(x, y);
			Map3DXY(right_x, right_y, base_z, &x, &y);
			if(DrawBorder & 8)
				ClipVector(pT, x, y);
			else
				clip_move(x, y);
			Map3DXY(front_x, front_y, base_z, &x, &y);
			if(DrawBorder & 4)
				ClipVector(pT, x, y);
			else
				clip_move(x, y);
			Map3DXY(zaxis_x, zaxis_y, base_z, &x, &y);
			if(DrawBorder & 1)
				ClipVector(pT, x, y);
			else
				clip_move(x, y);
			if((DrawBorder & 15) == 15)
				closepath(pT);
		}
	}
	else if(DrawBorder)        {
		/// the four corners of the base plane, in normalized view
		// coordinates (-1..1) on all three axes.
		GpVertex bl, bb, br, bf;
		// map to normalized view coordinates the corners of the
		// baseplane: left, back, right and front, in that order:
		Map3DXYZ(zaxis_x, zaxis_y, base_z, &bl);
		Map3DXYZ(back_x, back_y, base_z, &bb);
		Map3DXYZ(right_x, right_y, base_z, &br);
		Map3DXYZ(front_x, front_y, base_z, &bf);
		if(BACKGRID != whichgrid) {
			// Draw front part of base grid, right to front corner:
			if(DrawBorder & 4)
				Draw3DLine(pT, &br, &bf, &BorderLp);
			// ... and left to front: 
			if(DrawBorder & 1)
				Draw3DLine(pT, &bl, &bf, &BorderLp);
		}
		if(FRONTGRID != whichgrid) {
			// Draw back part of base grid: left to back corner: 
			if(DrawBorder & 2)
				Draw3DLine(pT, &bl, &bb, &BorderLp);
			/* ... and right to back: */
			if(DrawBorder & 8)
				Draw3DLine(pT, &br, &bb, &BorderLp);
		}

		/* if surface is drawn, draw the rest of the graph box, too: */
		if(draw_surface || (draw_contour & CONTOUR_SRF) || (Pm3D.implicit == PM3D_IMPLICIT && strpbrk(Pm3D.where, "st") != NULL)) {
			GpVertex fl, fb, fr, ff; /* floor left/back/right/front corners */
			GpVertex tl, tb, tr, tf; /* top left/back/right/front corners */
			Map3DXYZ(zaxis_x, zaxis_y, floor_z, &fl);
			Map3DXYZ(back_x, back_y, floor_z, &fb);
			Map3DXYZ(right_x, right_y, floor_z, &fr);
			Map3DXYZ(front_x, front_y, floor_z, &ff);
			Map3DXYZ(zaxis_x, zaxis_y, ceiling_z, &tl);
			Map3DXYZ(back_x, back_y, ceiling_z, &tb);
			Map3DXYZ(right_x, right_y, ceiling_z, &tr);
			Map3DXYZ(front_x, front_y, ceiling_z, &tf);
			if((DrawBorder & 0xf0) == 0xf0) {
				/* all four verticals are drawn - save some time by
				 * drawing them to the full height, regardless of
				 * where the surface lies */
				if(FRONTGRID != whichgrid) {
					/* Draw the back verticals floor-to-ceiling, left: */
					Draw3DLine(pT, &fl, &tl, &BorderLp);
					/* ... back: */
					Draw3DLine(pT, &fb, &tb, &BorderLp);
					/* ... and right */
					Draw3DLine(pT, &fr, &tr, &BorderLp);
				}
				if(BACKGRID != whichgrid) {
					/* Draw the front vertical: floor-to-ceiling, front: */
					Draw3DLine(pT, &ff, &tf, &BorderLp);
				}
			}
			else {
				// find heights of surfaces at the corners of the xy rectangle 
				double height[2][2];
				double depth[2][2];
				uint zaxis_i = MAP_HEIGHT_X(zaxis_x);
				uint zaxis_j = MAP_HEIGHT_Y(zaxis_y);
				uint back_i = MAP_HEIGHT_X(back_x);
				uint back_j = MAP_HEIGHT_Y(back_y);
				height[0][0] = height[0][1] = height[1][0] = height[1][1] = base_z;
				depth[0][0] = depth[0][1] = depth[1][0] = depth[1][1] = base_z;

				/* FIXME HBB 20000617: this method contains the
				 * assumption that the topological corners of the
				 * surface mesh(es) are also the geometrical ones of
				 * their xy projections. This is only true for
				 * 'explicit' surface datasets, i.e. z(x,y) */
				for(; --plot_num >= 0; pPlot = pPlot->next_sp) {
					if(pPlot->plot_type != NODATA) {
						iso_curve * curve = pPlot->iso_crvs;
						int count;
						int iso;
						if(pPlot->plot_type == DATA3D) {
							if(!pPlot->has_grid_topology)
								continue;
							iso = pPlot->num_iso_read;
						}
						else
							iso = iso_samples_2;
						count = curve->p_count;
						CheckCornerHeight(curve->points, height, depth);
						CheckCornerHeight(curve->points + count - 1, height, depth);
						while(--iso)
							curve = curve->next;
						CheckCornerHeight(curve->points, height, depth);
						CheckCornerHeight(curve->points + count - 1, height, depth);
					}
				}

#define VERTICAL(mask, x, y, i, j, bottom, top)			      \
	if(DrawBorder&mask) {			       \
		Draw3DLine(pT, bottom, top, &BorderLp);	     \
	} \
	else if(height[i][j] != depth[i][j]) {	     \
		GpVertex a, b;				    \
		Map3DXYZ(x, y, depth[i][j], &a);	       \
		Map3DXYZ(x, y, height[i][j], &b);	       \
		Draw3DLine(pT, &a, &b, &BorderLp);	    \
	}

				if(FRONTGRID != whichgrid) {
					// Draw back verticals: floor-to-ceiling left: 
					VERTICAL(0x10, zaxis_x, zaxis_y, zaxis_i, zaxis_j, &fl, &tl);
					// ... back: 
					VERTICAL(0x20, back_x, back_y, back_i, back_j, &fb, &tb);
					// ... and right: 
					VERTICAL(0x40, right_x, right_y, 1 - zaxis_i, 1 - zaxis_j, &fr, &tr);
				}
				if(BACKGRID != whichgrid) {
					// Draw front verticals: floor-to-ceiling front 
					VERTICAL(0x80, front_x, front_y, 1 - back_i, 1 - back_j, &ff, &tf);
				}
#undef VERTICAL
			} /* else (all 4 verticals drawn?) */

			/* now border lines on top */
			if(FRONTGRID != whichgrid) {
				/* Draw back part of top of box: top left to back corner: */
				if(DrawBorder & 0x100)
					Draw3DLine(pT, &tl, &tb, &BorderLp);
				/* ... and top right to back: */
				if(DrawBorder & 0x200)
					Draw3DLine(pT, &tr, &tb, &BorderLp);
			}
			if(BACKGRID != whichgrid) {
				/* Draw front part of top of box: top left to front corner: */
				if(DrawBorder & 0x400)
					Draw3DLine(pT, &tl, &tf, &BorderLp);
				/* ... and top right to front: */
				if(DrawBorder & 0x800)
					Draw3DLine(pT, &tr, &tf, &BorderLp);
			}
		} /* else (surface is drawn) */
	} /* if(DrawBorder) */
	// In 'set view map' mode, treat grid as in 2D plots 
	if(splot_map && current_layer != abs(grid_layer))
		return;
	if(whichgrid == BORDERONLY)
		return;
	// Draw ticlabels and axis labels. x axis, first:
	if(GetX().ticmode || GetX().label.text) {
		GpVertex v0, v1;
		double other_end = GetY().Range.low + GetY().Range.upp - xaxis_y;
		double mid_x     = (GetX().Range.upp + GetX().Range.low) / 2;
		Map3DXYZ(mid_x, xaxis_y, base_z, &v0);
		Map3DXYZ(mid_x, other_end, base_z, &v1);
		tic_unitx = (v1.x - v0.x) / (double)yscaler;
		tic_unity = (v1.y - v0.y) / (double)yscaler;
		tic_unitz = (v1.z - v0.z) / (double)yscaler;
		// Don'pT output tics and grids if this is the front part of a two-part grid drawing process:
		if((surface_rot_x <= 90 && FRONTGRID != whichgrid) || (surface_rot_x > 90 && BACKGRID != whichgrid))
			if(GetX().ticmode)
				GenTics(pT, AxA[FIRST_X_AXIS], &GpGadgets::XTickCallback);
		if(GetX().label.text) {
			int angle = 0;
			/* label at xaxis_y + 1/4 of (xaxis_y-other_y) */
			/* FIXME: still needed??? what for? */
			if((surface_rot_x <= 90 && BACKGRID != whichgrid) || (surface_rot_x > 90 && FRONTGRID != whichgrid) || splot_map) {
				uint x1, y1;
				int tmpx, tmpy;
				if(splot_map) { /* case 'set view map' */
					// copied from xtick_callback(): baseline of tics labels
					GpVertex v1, v2;
					Map3DXYZ(mid_x, xaxis_y, base_z, &v1);
					v2.x = v1.x;
					v2.y = v1.y - tic_unity * pT->VChr * 1;
					if(!(GetX().Flags & GpAxis::fTicIn)) {
						/* FIXME
						 * This code and its source in xtick_callback() is wrong --- tics
						 * can be "in" but ticscale <0 ! To be corrected in both places!
						 */
						v2.y -= tic_unity * pT->VTic * GetX().ticscale;
					}
					TermCoord(v2, x1, y1);
					// DEFAULT_Y_DISTANCE is with respect to baseline of tics labels 
#define DEFAULT_Y_DISTANCE 0.5
					y1 -= (uint)((1 + DEFAULT_Y_DISTANCE) * pT->VChr);
#undef DEFAULT_Y_DISTANCE
					angle = GetX().label.rotate;
				}
				else { /* usual 3d set view ... */
					double step = (xaxis_y - other_end) / 4;
					/* The only angle that makes sense is running parallel to the axis */
					if(GetX().label.tag == ROTATE_IN_3D_LABEL_TAG) {
						double ang, angx0, angx1, angy0, angy1;
						Map3DXY(GetX().Range.low, xaxis_y, base_z, &angx0, &angy0);
						Map3DXY(GetX().Range.upp, xaxis_y, base_z, &angx1, &angy1);
						ang = atan2(angy1-angy0, angx1-angx0) / DEG2RAD;
						angle = (int)((ang > 0) ? floor(ang + 0.5) : floor(ang - 0.5));
						if(angle < -90) 
							angle += 180;
						if(angle > 90) 
							angle -= 180;
						step /= 2;
					}
					if(GetX().ticmode & TICS_ON_AXIS) {
						Map3DXYZ(mid_x, ((GetX().Flags & GpAxis::fTicIn) ? step : -step)/2., base_z, &v1);
					}
					else {
						Map3DXYZ(mid_x, xaxis_y + step, base_z, &v1);
					}
					if(!(GetX().Flags & GpAxis::fTicIn)) {
						v1.x -= tic_unitx * GetX().ticscale * pT->VTic;
						v1.y -= tic_unity * GetX().ticscale * pT->VTic;
					}
					TermCoord(v1, x1, y1);
				}
				Map3DPositionR(GetX().label.offset, &tmpx, &tmpy, "graphbox");
				x1 += tmpx; /* user-defined label offset */
				y1 += tmpy;
				ignore_enhanced(GetX().label.noenhanced);
				ApplyPm3DColor(pT, &(GetX().label.textcolor));
				if(angle != 0 && (pT->text_angle)(angle)) {
					pT->DrawMultiline(x1, y1, GetX().label.text, CENTRE, JUST_TOP, angle, GetX().label.font);
					(pT->text_angle)(0);
				}
				else {
					//pT->DrawMultiline(x1, y1, GetX().label.text, CENTRE, JUST_TOP, 0, GetX().label.font);
					DrawAxisLabel(x1, y1, GetX(), CENTRE, JUST_TOP, true);
				}
				reset_textcolor(&(GetX().label.textcolor));
				ignore_enhanced(false);
			}
		}
		if(splot_map && AxA[SECOND_X_AXIS].ticmode)
			GenTics(pT, AxA[SECOND_X_AXIS], &GpGadgets::XTickCallback);
	}
	// y axis: 
	if(GetY().ticmode || GetY().label.text) {
		GpVertex v0, v1;
		double other_end = GetX().Range.low + GetX().Range.upp - yaxis_x;
		double mid_y = (GetY().Range.upp + GetY().Range.low) / 2;
		Map3DXYZ(yaxis_x, mid_y, base_z, &v0);
		Map3DXYZ(other_end, mid_y, base_z, &v1);
		tic_unitx = (v1.x - v0.x) / (double)xscaler;
		tic_unity = (v1.y - v0.y) / (double)xscaler;
		tic_unitz = (v1.z - v0.z) / (double)xscaler;
		// Don'pT output tics and grids if this is the front part of a two-part grid drawing process:
		if((surface_rot_x <= 90 && FRONTGRID != whichgrid) || (surface_rot_x > 90 && BACKGRID != whichgrid))
			if(GetY().ticmode)
				GenTics(pT, AxA[FIRST_Y_AXIS], &GpGadgets::YTickCallback);
		if(GetY().label.text) {
			if((surface_rot_x <= 90 && BACKGRID != whichgrid) || (surface_rot_x > 90 && FRONTGRID != whichgrid) || splot_map) {
				uint x1, y1;
				int tmpx, tmpy;
				//int h_just, v_just;
				JUSTIFY h_just;
				VERT_JUSTIFY v_just;
				int    angle = 0;
				if(splot_map) { /* case 'set view map' */
					// copied from ytick_callback(): baseline of tics labels
					GpVertex v1, v2;
					Map3DXYZ(yaxis_x, mid_y, base_z, &v1);
					if(GetY().ticmode & TICS_ON_AXIS && !(GetX().Flags & GpAxis::fLog) && GetX().InRange(0.0)) {
						Map3DXYZ(0.0, yaxis_x, base_z, &v1);
					}
					v2.x = v1.x - tic_unitx * pT->HChr * 1;
					v2.y = v1.y;
					if(!(GetX().Flags & GpAxis::fTicIn))
						v2.x -= tic_unitx * pT->HTic * GetX().ticscale;
					TermCoord(v2, x1, y1);
					// calculate max length of y-tics labels
					widest_tic_strlen = 0;
					if(GetY().ticmode & TICS_ON_BORDER) {
						widest_tic_strlen = 0; // reset the global variable
						GenTics(pT, AxA[FIRST_Y_AXIS], &GpGadgets::WidestTicCallback);
					}
					// DEFAULT_Y_DISTANCE is with respect to baseline of tics labels
#define DEFAULT_X_DISTANCE 0.
					x1 -= (uint)((DEFAULT_X_DISTANCE + 0.5 + widest_tic_strlen) * pT->HChr);
#undef DEFAULT_X_DISTANCE
#if 0
					// another method ... but not compatible
					uint map_y1, map_x2, map_y2;
					int tics_len = 0;
					if(GetY().ticmode) {
						tics_len = (int)(GetX().ticscale * (GetX().tic_in ? 1 : -1) * (pT->VTic));
						if(tics_len > 0)
							tics_len = 0;  // take care only about left tics
					}
					map3d_xy(GetX().min, GetY().min, base_z, &x1, &map_y1);
					map3d_xy(GetX().max, GetY().max, base_z, &map_x2, &map_y2);
					y1 = (uint)((map_y1 + map_y2) * 0.5);
					// Distance between the title base line and graph top line or the upper part of
					// tics is as given by character height:
#define DEFAULT_X_DISTANCE 0
					x1 += (uint)(tics_len + (-0.5 + GetY().label.xoffset) * pT->HChr);
					y1 += (uint)((DEFAULT_X_DISTANCE + GetY().label.yoffset) * pT->VChr);
#undef DEFAULT_X_DISTANCE
#endif
					h_just = CENTRE; /* vertical justification for rotated text */
					v_just = JUST_BOT; /* horizontal -- does not work for rotated text? */
					angle = GetY().label.rotate;
				}
				else { // usual 3d set view ... 
					double step = (other_end - yaxis_x) / 4;
					// The only angle that makes sense is running parallel to the axis 
					if(GetY().label.tag == ROTATE_IN_3D_LABEL_TAG) {
						double ang, angx0, angx1, angy0, angy1;
						Map3DXY(yaxis_x, GetY().Range.low, base_z, &angx0, &angy0);
						Map3DXY(yaxis_x, GetY().Range.upp, base_z, &angx1, &angy1);
						ang = atan2(angy1-angy0, angx1-angx0) / DEG2RAD;
						angle = (int)((ang > 0) ? floor(ang + 0.5) : floor(ang - 0.5));
						if(angle < -90) angle += 180;
						if(angle > 90) angle -= 180;
						step /= 2;
					}
					if(GetY().ticmode & TICS_ON_AXIS) {
						Map3DXYZ(((GetX().Flags & GpAxis::fTicIn) ? -step : step)/2.0, mid_y, base_z, &v1);
					}
					else {
						Map3DXYZ(yaxis_x - step, mid_y, base_z, &v1);
					}
					if(!(GetX().Flags & GpAxis::fTicIn)) {
						v1.x -= tic_unitx * GetX().ticscale * pT->HTic;
						v1.y -= tic_unity * GetX().ticscale * pT->HTic;
					}
					TermCoord(v1, x1, y1);
					h_just = CENTRE;
					v_just = JUST_TOP;
				}
				Map3DPositionR(GetY().label.offset, &tmpx, &tmpy, "graphbox");
				x1 += tmpx; // user-defined label offset
				y1 += tmpy;
				// write_multiline mods it
				ignore_enhanced(GetY().label.noenhanced);
				ApplyPm3DColor(pT, &(GetY().label.textcolor));
				if(angle != 0 && (pT->text_angle)(angle)) {
					pT->DrawMultiline(x1, y1, GetY().label.text, h_just, v_just, angle, GetY().label.font);
					(pT->text_angle)(0);
				}
				else {
					//pT->DrawMultiline(x1, y1, GetY().label.text, h_just, v_just, 0, GetY().label.font);
					DrawAxisLabel(x1, y1, GetY(), h_just, v_just, true);
				}
				reset_textcolor(&(GetY().label.textcolor));
				ignore_enhanced(false);
			}
		}
		if(splot_map && AxA[SECOND_Y_AXIS].ticmode)
			GenTics(pT, AxA[SECOND_Y_AXIS], &GpGadgets::YTickCallback);
	}
	//
	// do z tics
	//
	if(GetZ().ticmode
	    // Don'pT output tics and grids if this is the front part of a two-part grid drawing process:
	    && (FRONTGRID != whichgrid) && !splot_map && (draw_surface || (draw_contour & CONTOUR_SRF)|| strchr(Pm3D.where, 's'))) {
		GenTics(pT, AxA[FIRST_Z_AXIS], &GpGadgets::ZTickCallback);
	}
	if(GetY().zeroaxis && !(GetX().Flags & GpAxis::fLog) && GetX().InRange(0.0)) {
		GpVertex v1, v2;
		// line through x=0
		Map3DXYZ(0.0, GetY().Range.low, base_z, &v1);
		Map3DXYZ(0.0, GetY().Range.upp, base_z, &v2);
		Draw3DLine(pT, &v1, &v2, GetY().zeroaxis);
	}
	if(GetZ().zeroaxis && !(GetX().Flags & GpAxis::fLog) && GetX().InRange(0.0)) {
		GpVertex v1, v2;
		// line through x=0 y=0
		Map3DXYZ(0.0, 0.0, GetZ().Range.low, &v1);
		Map3DXYZ(0.0, 0.0, GetZ().Range.upp, &v2);
		Draw3DLine(pT, &v1, &v2, GetZ().zeroaxis);
	}
	if(GetX().zeroaxis && !(GetY().Flags & GpAxis::fLog) && GetY().InRange(0.0)) {
		GpVertex v1, v2;
		ApplyLpProperties(pT, GetX().zeroaxis);
		// line through y=0
		Map3DXYZ(GetX().Range.low, 0.0, base_z, &v1);
		Map3DXYZ(GetX().Range.upp, 0.0, base_z, &v2);
		Draw3DLine(pT, &v1, &v2, GetX().zeroaxis);
	}
	// PLACE ZLABEL - along the middle grid Z axis - eh ?
	if(GetZ().label.text && !splot_map && (draw_surface || (draw_contour & CONTOUR_SRF) || strpbrk(Pm3D.where, "st"))) {
		int tmpx, tmpy;
		GpVertex v1;
		JUSTIFY h_just = CENTRE;
		VERT_JUSTIFY v_just = JUST_TOP;
		double mid_z = (GetZ().Range.upp + GetZ().Range.low) / 2.0;
		if(GetZ().ticmode & TICS_ON_AXIS) {
			Map3DXYZ(0, 0, mid_z, &v1);
			{
				uint ux, uy;
				TermCoord(v1, ux, uy);
				x = (int)ux;
				y = (int)uy;
			}
			x -= 5 * pT->HChr;
			h_just = RIGHT;
		}
		else {
			/* December 2011 - This caused the separation between the axis and the
			 * label to vary as the view angle changes (Bug #2879916).   Why???
			 * double other_end = GetX().min + GetX().max - zaxis_x;
			 * Map3DXYZ(zaxis_x - (other_end - zaxis_x) / 4., zaxis_y, mid_z, &v1);
			 * It seems better to use a constant default separation.
			 */
			Map3DXYZ(zaxis_x, zaxis_y, mid_z, &v1);
			{
				uint ux, uy;
				TermCoord(v1, ux, uy);
				x = (int)ux;
				y = (int)uy;
			}
			x -= 7 * pT->HChr;
			h_just = CENTRE;
		}
		Map3DPositionR(GetZ().label.offset, &tmpx, &tmpy, "graphbox");
		x += tmpx;
		y += tmpy;
		ignore_enhanced(GetZ().label.noenhanced);
		ApplyPm3DColor(pT, &(GetZ().label.textcolor));
		if(GetZ().label.tag == ROTATE_IN_3D_LABEL_TAG)
			GetZ().label.rotate = TEXT_VERTICAL;
		if(GetZ().label.rotate != 0 &&  (pT->text_angle)(GetZ().label.rotate)) {
			//pT->DrawMultiline(x, y, GetZ().label.text, h_just, v_just, GetZ().label.rotate, GetZ().label.font);
			DrawAxisLabel(x, y, GetZ(), h_just, v_just, false);
			(pT->text_angle)(0);
		}
		else {
			//pT->DrawMultiline(x, y, GetZ().label.text, h_just, v_just, 0, GetZ().label.font);
			DrawAxisLabel(x, y, GetZ(), h_just, v_just, true);
		}
		reset_textcolor(&GetZ().label.textcolor);
		ignore_enhanced(false);
	}
	P_Clip = clip_save;
}

void GpGadgets::XTickCallback(GpTermEntry * pT, GpTicCallbackParam * pP/*GpAxis * pAx, double place, char * text, int ticlevel, lp_style_type grid, ticmark * userlabels*/)
{
	double scale = GetTicScale(pP) * ((pP->P_Ax->Flags & GpAxis::fTicIn) ? 1 : -1);
	double other_end = GetY().Range.low + GetY().Range.upp - xaxis_y;
	GpVertex v1, v2, v3, v4;
	// Draw full-length grid line
	Map3DXYZ(pP->Place, xaxis_y, base_z, &v1);
	if(pP->Style.l_type > LT_NODRAW) {
		(pT->layer)(TERM_LAYER_BEGIN_GRID);
		// to save mapping twice, map non-axis y
		Map3DXYZ(pP->Place, other_end, base_z, &v3);
		Draw3DLine(pT, &v1, &v3, &pP->Style);
		(pT->layer)(TERM_LAYER_END_GRID);
	}
	if((GetX().ticmode & TICS_ON_AXIS) && !(GetY().Flags & GpAxis::fLog) && GetY().InRange(0.0)) {
		Map3DXYZ(pP->Place, 0.0, base_z, &v1);
	}
	// NB: secondary axis must be linked to primary 
	if(pP->P_Ax->Index == SECOND_X_AXIS && pP->P_Ax->P_LinkToPrmr && pP->P_Ax->link_udf->at) {
		pP->Place = AxA[FIRST_X_AXIS].EvalLinkFunction(pP->Place);
	}
	// Draw bottom tic mark
	if((pP->P_Ax->Index == FIRST_X_AXIS) || (pP->P_Ax->Index == SECOND_X_AXIS && (pP->P_Ax->ticmode & TICS_MIRROR))) {
		v2.x = v1.x + tic_unitx * scale * pT->VTic;
		v2.y = v1.y + tic_unity * scale * pT->VTic;
		v2.z = v1.z + tic_unitz * scale * pT->VTic;
		v2.real_z = v1.real_z;
		Draw3DLine(pT, &v1, &v2, &BorderLp);
	}
	// Draw top tic mark
	if((pP->P_Ax->Index == SECOND_X_AXIS) || (pP->P_Ax->Index == FIRST_X_AXIS && (pP->P_Ax->ticmode & TICS_MIRROR))) {
		Map3DXYZ(pP->Place, other_end, base_z, &v3);
		v4.x = v3.x - tic_unitx * scale * pT->VTic;
		v4.y = v3.y - tic_unity * scale * pT->VTic;
		v4.z = v3.z - tic_unitz * scale * pT->VTic;
		v4.real_z = v3.real_z;
		Draw3DLine(pT, &v3, &v4, &BorderLp);
	}
	// ApplyLpProperties(&BorderLp);
	//
	// Draw tic label
	if(pP->P_Text) {
		JUSTIFY just;
		uint   x2, y2;
		int    angle;
		int    offsetx, offsety;
		// Skip label if we've already written a user-specified one here
#define MINIMUM_SEPARATION 0.001
		while(pP->P_M) {
			if(fabs((pP->Place - pP->P_M->position) / (GetX().GetRange())) <= MINIMUM_SEPARATION) {
				pP->P_Text = NULL;
				break;
			}
			pP->P_M = pP->P_M->next;
		}
#undef MINIMUM_SEPARATION
		// get offset
		Map3DPositionR(pP->P_Ax->ticdef.offset, &offsetx, &offsety, "xtics");
		// allow manual justification of tick labels, but only for "set view map"
		if(splot_map && pP->P_Ax->Flags & GpAxis::fManualJustify)
			just = pP->P_Ax->label.pos;
		else if(tic_unitx * xscaler < -0.9)
			just = LEFT;
		else if(tic_unitx * xscaler < 0.9)
			just = CENTRE;
		else
			just = RIGHT;
		if(pP->P_Ax->Index == SECOND_X_AXIS) {
			v4.x = v3.x + tic_unitx * pT->HChr * 1;
			v4.y = v3.y + tic_unity * pT->VChr * 1;
			if(!(pP->P_Ax->Flags & GpAxis::fTicIn)) {
				v4.x += tic_unitx * pT->VTic * pP->P_Ax->ticscale;
				v4.y += tic_unity * pT->VTic * pP->P_Ax->ticscale;
			}
			TermCoord(v4, x2, y2);
		}
		else {
			v2.x = v1.x - tic_unitx * pT->HChr * 1;
			v2.y = v1.y - tic_unity * pT->VChr * 1;
			if(!(pP->P_Ax->Flags & GpAxis::fTicIn)) {
				v2.x -= tic_unitx * pT->VTic * pP->P_Ax->ticscale;
				v2.y -= tic_unity * pT->VTic * pP->P_Ax->ticscale;
			}
			TermCoord(v2, x2, y2);
		}
		// User-specified different color for the tics text
		if(pP->P_Ax->ticdef.textcolor.type != TC_DEFAULT)
			ApplyPm3DColor(pT, &pP->P_Ax->ticdef.textcolor);
		angle = pP->P_Ax->tic_rotate;
		if(!(splot_map && angle && pT->text_angle(angle)))
			angle = 0;
		ignore_enhanced(!pP->P_Ax->ticdef.enhanced);
		pT->DrawMultiline(x2+offsetx, y2+offsety, pP->P_Text, just, JUST_TOP, angle, pP->P_Ax->ticdef.font);
		ignore_enhanced(false);
		pT->text_angle(0);
		ApplyLpProperties(pT, &BorderLp);
	}
}

//static void ytick_callback(GpAxis * pAx, double place, char * text, int ticlevel, lp_style_type grid, ticmark * userlabels/* currently ignored in 3D plots */)
void GpGadgets::YTickCallback(GpTermEntry * pT, GpTicCallbackParam * pP /*GpAxis * pAx, double place, char * text, int ticlevel, lp_style_type grid, ticmark * userlabels*/)
{
	double scale = GetTicScale(pP) * ((pP->P_Ax->Flags & GpAxis::fTicIn) ? 1 : -1);
	double other_end = GetX().Range.low + GetX().Range.upp - yaxis_x;
	GpVertex v1, v2, v3, v4;
	// Draw full-length grid line
	Map3DXYZ(yaxis_x, pP->Place, base_z, &v1);
	if(pP->Style.l_type > LT_NODRAW) {
		(pT->layer)(TERM_LAYER_BEGIN_GRID);
		Map3DXYZ(other_end, pP->Place, base_z, &v3);
		Draw3DLine(pT, &v1, &v3, &pP->Style);
		(pT->layer)(TERM_LAYER_END_GRID);
	}
	if(GetY().ticmode & TICS_ON_AXIS && !(GetX().Flags & GpAxis::fLog) && GetX().InRange(0.0)) {
		Map3DXYZ(0.0, pP->Place, base_z, &v1);
	}
	// NB: secondary axis must be linked to primary
	if(pP->P_Ax->Index == SECOND_Y_AXIS && pP->P_Ax->P_LinkToPrmr && pP->P_Ax->link_udf->at) {
		pP->Place = AxA[FIRST_Y_AXIS].EvalLinkFunction(pP->Place);
	}
	// Draw left tic mark
	if((pP->P_Ax->Index == FIRST_Y_AXIS) || (pP->P_Ax->Index == SECOND_Y_AXIS && (pP->P_Ax->ticmode & TICS_MIRROR))) {
		v2.x = v1.x + tic_unitx * scale * pT->HTic;
		v2.y = v1.y + tic_unity * scale * pT->HTic;
		v2.z = v1.z + tic_unitz * scale * pT->HTic;
		v2.real_z = v1.real_z;
		Draw3DLine(pT, &v1, &v2, &BorderLp);
	}
	// Draw right tic mark 
	if((pP->P_Ax->Index == SECOND_Y_AXIS) || (pP->P_Ax->Index == FIRST_Y_AXIS && (pP->P_Ax->ticmode & TICS_MIRROR))) {
		Map3DXYZ(other_end, pP->Place, base_z, &v3);
		v4.x = v3.x - tic_unitx * scale * pT->HTic;
		v4.y = v3.y - tic_unity * scale * pT->HTic;
		v4.z = v3.z - tic_unitz * scale * pT->HTic;
		v4.real_z = v3.real_z;
		Draw3DLine(pT, &v3, &v4, &BorderLp);
	}
	// Draw tic label
	if(pP->P_Text) {
		JUSTIFY just;
		uint   x2, y2;
		int    angle;
		int    offsetx, offsety;
		// Skip label if we've already written a user-specified one here
#define MINIMUM_SEPARATION 0.001
		while(pP->P_M) {
			if(fabs((pP->Place - pP->P_M->position) / (GetY().GetRange())) <= MINIMUM_SEPARATION) {
				pP->P_Text = NULL;
				break;
			}
			pP->P_M = pP->P_M->next;
		}
#undef MINIMUM_SEPARATION
		// get offset
		Map3DPositionR(pP->P_Ax->ticdef.offset, &offsetx, &offsety, "ytics");
		// allow manual justification of tick labels, but only for "set view map"
		if(splot_map && pP->P_Ax->Flags & GpAxis::fManualJustify)
			just = pP->P_Ax->label.pos;
		else if(tic_unitx * xscaler < -0.9)
			just = (pP->P_Ax->Index == FIRST_Y_AXIS) ? LEFT : RIGHT;
		else if(tic_unitx * xscaler < 0.9)
			just = CENTRE;
		else
			just = (pP->P_Ax->Index == FIRST_Y_AXIS) ? RIGHT : LEFT;
		if(pP->P_Ax->Index == SECOND_Y_AXIS) {
			v4.x = v3.x + tic_unitx * pT->HChr * 1;
			v4.y = v3.y + tic_unity * pT->VChr * 1;
			if(!(pP->P_Ax->Flags & GpAxis::fTicIn)) {
				v4.x += tic_unitx * pT->HTic * pP->P_Ax->ticscale;
				v4.y += tic_unity * pT->VTic * pP->P_Ax->ticscale;
			}
			TermCoord(v4, x2, y2);
		}
		else {
			v2.x = v1.x - tic_unitx * pT->HChr * 1;
			v2.y = v1.y - tic_unity * pT->VChr * 1;
			if(!(pP->P_Ax->Flags & GpAxis::fTicIn)) {
				v2.x -= tic_unitx * pT->HTic * pP->P_Ax->ticscale;
				v2.y -= tic_unity * pT->VTic * pP->P_Ax->ticscale;
			}
			TermCoord(v2, x2, y2);
		}
		// User-specified different color for the tics text 
		if(pP->P_Ax->ticdef.textcolor.type != TC_DEFAULT)
			ApplyPm3DColor(pT, &pP->P_Ax->ticdef.textcolor);
		angle = pP->P_Ax->tic_rotate;
		if(!(splot_map && angle && pT->text_angle(angle)))
			angle = 0;
		ignore_enhanced(!pP->P_Ax->ticdef.enhanced);
		pT->DrawMultiline(x2+offsetx, y2+offsety, pP->P_Text, just, JUST_TOP, angle, pP->P_Ax->ticdef.font);
		ignore_enhanced(false);
		pT->text_angle(0);
		ApplyLpProperties(pT, &BorderLp);
	}
}

//static void ztick_callback(GpAxis * pAx, double place, char * text, int ticlevel, lp_style_type grid, ticmark * userlabels/* currently ignored in 3D plots */)
void GpGadgets::ZTickCallback(GpTermEntry * pT, GpTicCallbackParam * pP /* GpAxis * pAx, double place, char * text, int ticlevel, lp_style_type grid, ticmark * userlabels*/)
{
	int    len = (int)(GetTicScale(pP) * ((pP->P_Ax->Flags & GpAxis::fTicIn) ? 1 : -1) * (pT->HTic));
	GpVertex v1, v2, v3;
	if(pP->P_Ax->ticmode & TICS_ON_AXIS)
		Map3DXYZ(0.0, 0.0, pP->Place, &v1);
	else
		Map3DXYZ(zaxis_x, zaxis_y, pP->Place, &v1);
	if(pP->Style.l_type > LT_NODRAW) {
		(pT->layer)(TERM_LAYER_BEGIN_GRID);
		Map3DXYZ(back_x, back_y, pP->Place, &v2);
		Map3DXYZ(right_x, right_y, pP->Place, &v3);
		Draw3DLine(pT, &v1, &v2, &pP->Style);
		Draw3DLine(pT, &v2, &v3, &pP->Style);
		(pT->layer)(TERM_LAYER_END_GRID);
	}
	v2.x = v1.x + len / (double)xscaler;
	v2.y = v1.y;
	v2.z = v1.z;
	v2.real_z = v1.real_z;
	Draw3DLine(pT, &v1, &v2, &BorderLp);
	if(pP->P_Text) {
		uint x1, y1;
		int offsetx, offsety;
		/* Skip label if we've already written a user-specified one here */
#define MINIMUM_SEPARATION 0.001
		while(pP->P_M) {
			if(fabs((pP->Place - pP->P_M->position) / (GetZ().GetRange())) <= MINIMUM_SEPARATION) {
				pP->P_Text = NULL;
				break;
			}
			pP->P_M = pP->P_M->next;
		}
#undef MINIMUM_SEPARATION
		// get offset
		Map3DPositionR(pP->P_Ax->ticdef.offset, &offsetx, &offsety, "ztics");
		TermCoord(v1, x1, y1);
		x1 -= (pT->HTic) * 2;
		if(!(pP->P_Ax->Flags & GpAxis::fTicIn))
			x1 -= (int)((pT->HTic) * pP->P_Ax->ticscale);
		// User-specified different color for the tics text
		if(pP->P_Ax->ticdef.textcolor.type == TC_Z)
			pP->P_Ax->ticdef.textcolor.value = pP->Place;
		if(pP->P_Ax->ticdef.textcolor.type != TC_DEFAULT)
			ApplyPm3DColor(pT, &pP->P_Ax->ticdef.textcolor);
		ignore_enhanced(!pP->P_Ax->ticdef.enhanced);
		pT->DrawMultiline(x1+offsetx, y1+offsety, pP->P_Text, RIGHT, JUST_CENTRE, 0, pP->P_Ax->ticdef.font);
		ignore_enhanced(false);
		ApplyLpProperties(pT, &BorderLp);
	}
	if(GetZ().ticmode & TICS_MIRROR) {
		Map3DXYZ(right_x, right_y, pP->Place, &v1);
		v2.x = v1.x - len / (double)xscaler;
		v2.y = v1.y;
		v2.z = v1.z;
		v2.real_z = v1.real_z;
		Draw3DLine(pT, &v1, &v2, &BorderLp);
	}
}

//static int map3d_getposition(GpPosition * pos, const char * what, double * xpos, double * ypos, double * zpos)
int GpGadgets::Map3DGetPosition(GpPosition * pos, const char * what, double * xpos, double * ypos, double * zpos)
{
	bool screen_coords = false;
	bool char_coords = false;
	bool plot_coords = false;
	switch(pos->scalex) {
		case first_axes:
		case second_axes:
		    *xpos = LogValueChecked(FIRST_X_AXIS, *xpos, what);
		    plot_coords = true;
		    break;
		case graph:
		    *xpos = GetX().Range.low + *xpos * (GetX().GetRange());
		    plot_coords = true;
		    break;
		case screen:
		    *xpos = *xpos * (term->xmax -1) + 0.5;
		    screen_coords = true;
		    break;
		case character:
		    *xpos = *xpos * term->HChr + 0.5;
		    char_coords = true;
		    break;
	}
	switch(pos->scaley) {
		case first_axes:
		case second_axes:
		    *ypos = LogValueChecked(FIRST_Y_AXIS, *ypos, what);
		    plot_coords = true;
		    break;
		case graph:
		    if(splot_map)
			    *ypos = GetY().Range.upp - *ypos * (GetY().GetRange());
		    else
			    *ypos = GetY().Range.low + *ypos * (GetY().GetRange());
		    plot_coords = true;
		    break;
		case screen:
		    *ypos = *ypos * (term->ymax -1) + 0.5;
		    screen_coords = true;
		    break;
		case character:
		    *ypos = *ypos * term->VChr + 0.5;
		    char_coords = true;
		    break;
	}
	switch(pos->scalez) {
		case first_axes:
		case second_axes:
		    *zpos = LogValueChecked(FIRST_Z_AXIS, *zpos, what);
		    plot_coords = true;
		    break;
		case graph:
		    *zpos = GetZ().Range.low + *zpos * (GetZ().GetRange());
		    plot_coords = true;
		    break;
		case screen:
		    screen_coords = true;
		    break;
		case character:
		    char_coords = true;
		    break;
	}
	if(plot_coords && (screen_coords || char_coords))
		GpGg.IntError(GpC, NO_CARET, "Cannot mix screen or character coords with plot coords");
	return (screen_coords || char_coords);
}

/*
 * map3d_position()  wrapper for map3d_position_double
 */
//void map3d_position(GpPosition * pos, int * pX, int * pY, const char * pWhat)
void GpGadgets::Map3DPosition(GpPosition & rPos, int * pX, int * pY, const char * pWhat)
{
	double xx, yy;
	Map3DPositionDouble(rPos, &xx, &yy, pWhat);
	*pX = (int)xx;
	*pY = (int)yy;
}

//void map3d_position_double(GpPosition * pos, double * x, double * y, const char * what)
void GpGadgets::Map3DPositionDouble(GpPosition & rPos, double * pX, double * pY, const char * pWhat)
{
	double xpos = rPos.x;
	double ypos = rPos.y;
	double zpos = rPos.z;
	if(Map3DGetPosition(&rPos, pWhat, &xpos, &ypos, &zpos) == 0) {
		Map3DXY(xpos, ypos, zpos, pX, pY);
	}
	else {
		// Screen or character coordinates
		*pX = xpos;
		*pY = ypos;
	}
}

//void map3d_position_r(GpPosition * pos, int * x, int * y, const char * what)
void GpGadgets::Map3DPositionR(GpPosition & rPos, int * pX, int * pY, const char * pWhat)
{
	double xpos = rPos.x;
	double ypos = rPos.y;
	double zpos = rPos.z;
	// startpoint in graph coordinates
	if(Map3DGetPosition(&rPos, pWhat, &xpos, &ypos, &zpos) == 0) {
		int xoriginlocal, yoriginlocal;
		double xx, yy;
		Map3DXY(xpos, ypos, zpos, &xx, &yy);
		*pX = (int)xx;
		*pY = (int)yy;
		xpos = (rPos.scalex == graph) ? GetX().Range.low : 0;
		ypos = (rPos.scaley == graph) ? ((splot_map) ? GetY().Range.upp : GetY().Range.low) : 0;
		zpos = (rPos.scalez == graph) ? GetZ().Range.low : 0;
		Map3DXY(xpos, ypos, zpos, &xoriginlocal, &yoriginlocal);
		*pX -= xoriginlocal;
		*pY -= yoriginlocal;
	}
	else {
		// endpoint `screen' or 'character' coordinates
		*pX = (int)xpos;
		*pY = (int)ypos;
	}
}
//
// these code blocks were moved to functions, to make the code simpler
//
//static void key_text(int xl, int yl, char * text)
void GpGraphics::KeyText(int xl, int yl, char * text)
{
	legend_key * key = &GpGg.keyT;
	(term->layer)(TERM_LAYER_BEGIN_KEYSAMPLE);
	if(key->just == GPKEY_LEFT && key->region != GPKEY_USER_PLACEMENT) {
		term->DrawMultiline(xl + key_text_left, yl, text, LEFT, JUST_TOP, 0, key->font);
	}
	else {
		if((*term->justify_text)(RIGHT)) {
			term->DrawMultiline(xl + key_text_right, yl, text, RIGHT, JUST_TOP, 0, key->font);
		}
		else {
			int x = xl + key_text_right - (term->HChr) * estimate_strlen(text);
			term->DrawMultiline(x, yl, text, LEFT, JUST_TOP, 0, key->font);
		}
	}
	(term->layer)(TERM_LAYER_END_KEYSAMPLE);
}

static void key_sample_line(int xl, int yl)
{
	BoundingBox * clip_save = GpGg.P_Clip;
	/* Clip against GpGg.Canvas */
	GpGg.P_Clip = (term->flags & TERM_CAN_CLIP) ? 0 : &GpGg.Canvas;
	(term->layer)(TERM_LAYER_BEGIN_KEYSAMPLE);
	GpGg.DrawClipLine(term, xl + key_sample_left, yl, xl + key_sample_right, yl);
	(term->layer)(TERM_LAYER_END_KEYSAMPLE);
	GpGg.P_Clip = clip_save;
}

static void key_sample_point(int xl, int yl, int pointtype)
{
	BoundingBox * clip_save = GpGg.P_Clip;
	// Clip against GpGg.Canvas 
	GpGg.P_Clip = (term->flags & TERM_CAN_CLIP) ? 0 : &GpGg.Canvas;
	(term->layer)(TERM_LAYER_BEGIN_KEYSAMPLE);
	if(!GpGg.ClipPoint(xl + key_point_offset, yl))
		(*term->point)(xl + key_point_offset, yl, pointtype);
	(term->layer)(TERM_LAYER_END_KEYSAMPLE);
	GpGg.P_Clip = clip_save;
}

/*
 * returns minimal and maximal values of the cb-range (or z-range if taking the
 * color from the z value) of the given surface
 */
static void get_surface_cbminmax(SurfacePoints * plot, double * cbmin, double * cbmax)
{
	int i, curve = 0;
	bool color_from_column = plot->pm3d_color_from_column; /* just a shortcut */
	double cb;
	iso_curve * icrvs = plot->iso_crvs;
	GpCoordinate * points;
	*cbmin = GPVL;
	*cbmax = -GPVL;
	while(icrvs && curve < plot->num_iso_read) {
		// fprintf(stderr,"**** NEW ISOCURVE - nb of pts: %i ****\n", icrvs->p_count); 
		for(i = 0, points = icrvs->points; i < icrvs->p_count; i++) {
			// fprintf(stderr,"  point i=%i => x=%4g y=%4g z=%4lg cb=%4lg\n",i, points[i].x,points[i].y,points[i].z,points[i].CRD_COLOR);
			if(points[i].type == INRANGE) {
				// ?? if(!clip_point(x, y)) ...
				cb = color_from_column ? points[i].CRD_COLOR : points[i].z;
				if(cb < *cbmin) *cbmin = cb;
				if(cb > *cbmax) *cbmax = cb;
			}
		}
		icrvs = icrvs->next;
		curve++;
	}
}

/*
 * Draw a gradient color line for a key (legend).
 */
//static void key_sample_line_pm3d(SurfacePoints * plot, int xl, int yl)
void GpGadgets::KeySampleLinePm3D(GpTermEntry * pT, SurfacePoints * pPlot, int xl, int yl)
{
	int steps = MIN(24, abs(key_sample_right - key_sample_left));
	/* don't multiply by key->swidth --- could be >> palette.maxcolors */
	int x_to = xl + key_sample_right;
	double step = ((double)(key_sample_right - key_sample_left)) / steps;
	int i = 1, x1 = xl + key_sample_left, x2;
	double cbmin, cbmax;
	double gray, gray_from, gray_to, gray_step;
	int colortype = pPlot->lp_properties.pm3d_color.type;
	// If pPlot uses a constant color, set it here and then let simpler routine take over
	if((colortype == TC_RGB && pPlot->lp_properties.pm3d_color.value >= 0.0) || oneof2(colortype, TC_LT, TC_LINESTYLE)) {
		lp_style_type lptmp = pPlot->lp_properties;
		if(pPlot->lp_properties.l_type == LT_COLORFROMCOLUMN)
			lp_use_properties(&lptmp, (int)(pPlot->iso_crvs->points[0].CRD_COLOR));
		ApplyPm3DColor(pT, &lptmp.pm3d_color);
		key_sample_line(xl, yl);
	}
	else {
		// color gradient only over the cb-values of the surface, if smaller than the
		// cb-axis range (the latter are gray values [0:1]) 
		get_surface_cbminmax(pPlot, &cbmin, &cbmax);
		if(cbmin > cbmax) 
			return;  // splot 1/0, for example 
		cbmin = MAX(cbmin, GetCB().Range.low);
		cbmax = MIN(cbmax, GetCB().Range.upp);
		gray_from = CB2Gray(cbmin);
		gray_to = CB2Gray(cbmax);
		gray_step = (gray_to - gray_from)/steps;
		clip_move(x1, yl);
		x2 = x1;
		while(i <= steps) {
			/* if(i>1) set_color( i==steps ? 1 : (i-0.5)/steps ); ... range [0:1] */
			gray = (i==steps) ? gray_to : gray_from+i*gray_step;
			pT->SetColor(gray);
			clip_move(x2, yl);
			x2 = (i==steps) ? x_to : x1 + (int)(i*step+0.5);
			ClipVector(pT, x2, yl);
			i++;
		}
	}
}
//
// Draw a sequence of points with gradient color a key (legend).
//
//static void key_sample_point_pm3d(SurfacePoints * plot, int xl, int yl, int pointtype)
void GpGadgets::KeySamplePointPm3D(GpTermEntry * pT, SurfacePoints * plot, int xl, int yl, int pointtype)
{
	BoundingBox * clip_save = GpGg.P_Clip;
	int x_to = xl + key_sample_right;
	int i = 0, x1 = xl + key_sample_left, x2;
	double cbmin, cbmax;
	double gray, gray_from, gray_to, gray_step;
	int colortype = plot->lp_properties.pm3d_color.type;
	/* rule for number of steps: 3*char_width*pointsize or char_width for dots,
	 * but at least 3 points */
	double step = term->HChr * (pointtype == -1 ? 1 : 3*(1+(GpGg.PtSz-1)/2));
	int steps = (int)(((double)(key_sample_right - key_sample_left)) / step + 0.5);
	SETMAX(steps, 2);
	step = ((double)(key_sample_right - key_sample_left)) / steps;
	// If plot uses a constant color, set it here and then let simpler routine take over
	if((colortype == TC_RGB && plot->lp_properties.pm3d_color.value >= 0.0) || oneof2(colortype, TC_LT, TC_LINESTYLE)) {
		lp_style_type lptmp = plot->lp_properties;
		if(plot->lp_properties.l_type == LT_COLORFROMCOLUMN)
			lp_use_properties(&lptmp, (int)(plot->iso_crvs->points[0].CRD_COLOR));
		GpGg.ApplyPm3DColor(term, &lptmp.pm3d_color);
		key_sample_point(xl, yl, pointtype);
	}
	else {
		// color gradient only over the cb-values of the surface, if smaller than the
		// cb-axis range (the latter are gray values [0:1])
		get_surface_cbminmax(plot, &cbmin, &cbmax);
		if(cbmin <= cbmax) { // splot 1/0, for example, not
			cbmin = MAX(cbmin, GpGg.GetCB().Range.low);
			cbmax = MIN(cbmax, GpGg.GetCB().Range.upp);
			gray_from = GpGg.CB2Gray(cbmin);
			gray_to = GpGg.CB2Gray(cbmax);
			gray_step = (gray_to - gray_from)/steps;
			// Clip to GpGg.Canvas
			GpGg.P_Clip = (term->flags & TERM_CAN_CLIP) ? NULL : &GpGg.Canvas;
			while(i <= steps) {
				/* if(i>0) set_color( i==steps ? gray_to : (i-0.5)/steps ); ... range [0:1] */
				gray = (i==steps) ? gray_to : gray_from+i*gray_step;
				term->SetColor(gray);
				x2 = i==0 ? x1 : (i==steps ? x_to : x1 + (int)(i*step+0.5));
				/* x2 += key_point_offset; ... that's if there is only 1 point */
				if(!GpGg.ClipPoint(x2, yl))
					(*term->point)(x2, yl, pointtype);
				i++;
			}
			GpGg.P_Clip = clip_save;
		}
	}
}
//
// Plot the curves in VECTORS style
//
static void plot3d_vectors(SurfacePoints * plot)
{
	double x1, y1, x2, y2;
	GpCoordinate * tails = plot->iso_crvs->points;
	GpCoordinate * heads = plot->iso_crvs->next->points;
	/* Only necessary once, unless variable arrow style */
	arrow_style_type ap = plot->arrow_properties;
	GpGg.ApplyLpProperties(term, &ap.lp_properties);
	GpGg.ApplyHeadProperties(ap);
	for(int i = 0; i < plot->iso_crvs->p_count; i++) {
		if(heads[i].type != UNDEFINED && tails[i].type != UNDEFINED){
			/* variable arrow style read from extra data column */
			if(plot->arrow_properties.tag == AS_VARIABLE) {
				int as = (int)(heads[i].CRD_COLOR);
				arrow_use_properties(&ap, as);
				GpGg.ApplyLpProperties(term, &ap.lp_properties);
				GpGg.ApplyHeadProperties(ap);
			}
			else {
				check3d_for_variable_color(plot, &heads[i]);
			}
			// The normal case: both ends in range 
			if(heads[i].type == INRANGE && tails[i].type == INRANGE) {
				GpGg.Map3DXY(tails[i].x, tails[i].y, tails[i].z, &x1, &y1);
				GpGg.Map3DXY(heads[i].x, heads[i].y, heads[i].z, &x2, &y2);
				GpGg.DrawClipArrow(term, (int)x1, (int)y1, (int)x2, (int)y2, ap.head);
				// "set clip two" - both ends out of range 
			}
			else if(heads[i].type != INRANGE && tails[i].type != INRANGE) {
				double lx[2], ly[2], lz[2];
				if(!GpGg.ClipLines2)
					continue;
				GpGg.TwoEdge3DIntersect(&tails[i], &heads[i], lx, ly, lz);
				GpGg.Map3DXY(lx[0], ly[0], lz[0], &x1, &y1);
				GpGg.Map3DXY(lx[1], ly[1], lz[1], &x2, &y2);
				GpGg.DrawClipArrow(term, (int)x1, (int)y1, (int)x2, (int)y2, ap.head);
				// "set clip one" - one end out of range 
			}
			else if(GpGg.ClipLines1) {
				double clip_x, clip_y, clip_z;
				GpGg.Edge3DIntersect(&heads[i], &tails[i], &clip_x, &clip_y, &clip_z);
				if(tails[i].type == INRANGE) {
					GpGg.Map3DXY(tails[i].x, tails[i].y, tails[i].z, &x1, &y1);
					GpGg.Map3DXY(clip_x, clip_y, clip_z, &x2, &y2);
				}
				else {
					GpGg.Map3DXY(clip_x, clip_y, clip_z, &x1, &y1);
					GpGg.Map3DXY(heads[i].x, heads[i].y, heads[i].z, &x2, &y2);
				}
				GpGg.DrawClipArrow(term, (int)x1, (int)y1, (int)x2, (int)y2, ap.head);
			}
		}
	}
}

static void check3d_for_variable_color(SurfacePoints * plot, GpCoordinate * point)
{
	int colortype = plot->lp_properties.pm3d_color.type;
	const bool rgb_from_column = plot->pm3d_color_from_column && plot->lp_properties.pm3d_color.type == TC_RGB && plot->lp_properties.pm3d_color.value < 0.0;
	switch(colortype) {
		case TC_RGB:
		    if(rgb_from_column)
			    GpGg.SetRgbColorVar(term, (uint)point->CRD_COLOR);
		    break;
		case TC_Z:
		case TC_DEFAULT: // pm3d mode assumes this is default 
		    if(can_pm3d) {
				term->SetColor(GpGg.CB2Gray(plot->pm3d_color_from_column ? point->CRD_COLOR : GpGg.Z2CB(point->z)));
		    }
		    break;
		case TC_LINESTYLE: // color from linestyle in data column 
		    plot->lp_properties.pm3d_color.lt = (int)(point->CRD_COLOR);
		    GpGg.ApplyPm3DColor(term, &(plot->lp_properties.pm3d_color));
		    break;
		default:
		    break; // The other cases were taken care of already 
	}
}

void do_3dkey_layout(legend_key * key, int * xinkey, int * yinkey)
{
	GpTermEntry * t = term;
	int key_height, key_width;

	/* NOTE: All of these had better not change after being calculated here! */
	if(key->reverse) {
		key_sample_left = -key_sample_width;
		key_sample_right = 0;
		key_text_left = t->HChr;
		key_text_right = t->HChr * (max_ptitl_len + 1);
		key_size_right = (int)(t->HChr * (max_ptitl_len + 2 + key->width_fix));
		key_size_left = t->HChr + key_sample_width;
	}
	else {
		key_sample_left = 0;
		key_sample_right = key_sample_width;
		key_text_left = -(int)(t->HChr * (max_ptitl_len + 1));
		key_text_right = -(int)t->HChr;
		key_size_left = (int)(t->HChr * (max_ptitl_len + 2 + key->width_fix));
		key_size_right = t->HChr + key_sample_width;
	}
	key_point_offset = (key_sample_left + key_sample_right) / 2;
	KeyTitleHeight = (int)(ktitle_lines * t->VChr);
	KeyTitleExtra = 0;
	if((key->title.text) && (t->flags & TERM_ENHANCED_TEXT) && (strchr(key->title.text, '^') || strchr(key->title.text, '_')))
		KeyTitleExtra = t->VChr;
	key_width = key_col_wth * (key_cols - 1) + key_size_right + key_size_left;
	key_height = (int)(KeyTitleHeight + KeyTitleExtra + KeyEntryHeight * key_rows + key->height_fix * t->VChr);
	/* Now that we know the size of the key, we can position it as requested */
	if(key->region == GPKEY_USER_PLACEMENT) {
		int corner_x, corner_y;
		GpGg.Map3DPosition(key->user_pos, &corner_x, &corner_y, "key");
		if(key->hpos == CENTRE) {
			key->bounds.xleft = corner_x - key_width / 2;
			key->bounds.xright = corner_x + key_width / 2;
		}
		else if(key->hpos == RIGHT) {
			key->bounds.xleft = corner_x - key_width;
			key->bounds.xright = corner_x;
		}
		else {
			key->bounds.xleft = corner_x;
			key->bounds.xright = corner_x + key_width;
		}
		key->bounds.ytop = corner_y;
		key->bounds.ybot = corner_y - key_height;
		*xinkey = key->bounds.xleft + key_size_left;
		*yinkey = key->bounds.ytop - KeyTitleHeight - KeyTitleExtra;
	}
	else {
		if(key->region != GPKEY_AUTO_INTERIOR_LRTBC && key->margin == GPKEY_BMARGIN) {
			if(ptitl_cnt > 0) {
				/* we divide into columns, then centre in column by considering
				 * ratio of key_left_size to key_right_size
				 * key_size_left / (key_size_left+key_size_right)
				 *               * (GpGg.PlotBounds.xright-GpGg.PlotBounds.xleft)/key_cols
				 * do one integer division to maximise accuracy (hope we dont overflow!)
				 */
				*xinkey = GpGg.PlotBounds.xleft + ((GpGg.PlotBounds.xright - GpGg.PlotBounds.xleft) * key_size_left) /
					(key_cols * (key_size_left + key_size_right));
				key->bounds.xleft = *xinkey - key_size_left;
				key->bounds.xright = key->bounds.xleft + key_width;
				key->bounds.ytop = GpGg.PlotBounds.ybot;
				key->bounds.ybot = GpGg.PlotBounds.ybot - key_height;
				*yinkey = key->bounds.ytop - KeyTitleHeight - KeyTitleExtra;
			}
		}
		else {
			if(key->vpos == JUST_TOP) {
				key->bounds.ytop = GpGg.PlotBounds.ytop - t->VTic;
				key->bounds.ybot = key->bounds.ytop - key_height;
				*yinkey = key->bounds.ytop - KeyTitleHeight - KeyTitleExtra;
			}
			else {
				key->bounds.ybot = GpGg.PlotBounds.ybot + t->VTic;
				key->bounds.ytop = key->bounds.ybot + key_height;
				*yinkey = key->bounds.ytop - KeyTitleHeight - KeyTitleExtra;
			}
			if(key->region != GPKEY_AUTO_INTERIOR_LRTBC && key->margin == GPKEY_RMARGIN) {
				/* keys outside plot border (right) */
				key->bounds.xleft = GpGg.PlotBounds.xright + t->HTic;
				key->bounds.xright = key->bounds.xleft + key_width;
				*xinkey = key->bounds.xleft + key_size_left;
			}
			else if(key->region != GPKEY_AUTO_INTERIOR_LRTBC && key->margin == GPKEY_LMARGIN) {
				/* keys outside plot border (left) */
				key->bounds.xright = GpGg.PlotBounds.xleft - t->HTic;
				key->bounds.xleft = key->bounds.xright - key_width;
				*xinkey = key->bounds.xleft + key_size_left;
			}
			else if(key->hpos == LEFT) {
				key->bounds.xleft = GpGg.PlotBounds.xleft + t->HTic;
				key->bounds.xright = key->bounds.xleft + key_width;
				*xinkey = key->bounds.xleft + key_size_left;
			}
			else {
				key->bounds.xright = GpGg.PlotBounds.xright - t->HTic;
				key->bounds.xleft = key->bounds.xright - key_width;
				*xinkey = key->bounds.xleft + key_size_left;
			}
		}
		yl_ref = *yinkey - KeyTitleHeight - KeyTitleExtra;
	}
	/* Center the key entries vertically, allowing for requested extra space */
	*yinkey -= (int)((key->height_fix * t->VChr) / 2);
}

