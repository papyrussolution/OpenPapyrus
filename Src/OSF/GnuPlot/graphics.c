/* GNUPLOT - graphics.c */

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

/* Daniel Sebald: added plot_image_or_update_axes() routine for images.
 * (5 November 2003)
 */
#include <gnuplot.h>
#pragma hdrstop
//
// static fns and local macros
//
static t_colorspec background_fill(TC_LT, LT_BACKGROUND, 0.0); // used for filled points
//static void plot_impulses(CurvePoints * plot, int yaxis_x, int xaxis_y);
static int compare_ypoints(const void * arg1, const void * arg2);
//static void place_raxis();
//static void place_parallel_axes(const CurvePoints * plots, int pcount, int layer);
//static void plot_steps(CurvePoints* plot);    /* JG */
//static void plot_fsteps(CurvePoints* plot);   /* HOE */
//static void plot_histeps(CurvePoints* plot);  /* CAC */
//static int edge_intersect(GpCoordinate * points, int i, double * ex, double * ey);
//static bool two_edge_intersect(GpCoordinate * points, int i, double * lx, double * ly);
//static void ytick2d_callback(GpTermEntry * pT, GpTicCallbackParam * /* GpAxis *, double place, char * text, int ticlevel, lp_style_type grid, ticmark * userlabels */);
//static void xtick2d_callback(GpTermEntry * pT, GpTicCallbackParam * /* GpAxis *, double place, char * text, int ticlevel, lp_style_type grid, ticmark * userlabels */);
static int histeps_compare(const void * p1, const void * p2);
//static void attach_title_to_plot(CurvePoints * this_plot, legend_key *key);
#ifdef EAM_OBJECTS
static void plot_circles(CurvePoints* plot);
#endif
static void plot_parallel(CurvePoints* plot);

/* for plotting error bars
 * half the width of error bar tic mark
 */
//#define ERRORBARTIC MAX((t->HTic/2), 1)

/* For tracking exit and re-entry of bounding curves that extend out of plot */
/* these must match the bit values returned by clip_point(). */
#define LEFT_EDGE       1
#define RIGHT_EDGE      2
#define BOTTOM_EDGE     4
#define TOP_EDGE        8

#define f_max(a, b) MAX((a), (b))
#define f_min(a, b) MIN((a), (b))

/* True if a and b have the same sign or zero (positive or negative) */
#define samesign(a, b) ((sgn(a) * sgn(b)) >= 0)
/*}}} */

//static void get_arrow(arrow_def * arrow, int* sx, int* sy, int* ex, int* ey)
void GpGadgets::GetArrow(arrow_def * pArrow, int* pSx, int* pSy, int* pEx, int* pEy)
{
	double sx_d, sy_d, ex_d, ey_d;
	MapPositionDouble(term, &pArrow->start, &sx_d, &sy_d, "pArrow");
	*pSx = (int)(sx_d);
	*pSy = (int)(sy_d);
	if(pArrow->type == arrow_end_relative) {
		// different GpCoordinate systems:
		// add the values in the drivers GpCoordinate system.
		// For log scale: relative GpCoordinate is factor
		MapPositionR(pArrow->end, &ex_d, &ey_d, "pArrow");
		*pEx = (int)(ex_d + sx_d);
		*pEy = (int)(ey_d + sy_d);
	}
	else if(pArrow->type == arrow_end_oriented) {
		double aspect = (double)term->VTic / (double)term->HTic;
		double radius;
#ifdef WIN32
		if(strcmp(term->name, "windows") == 0)
			aspect = 1.;
#endif
		MapPositionR(pArrow->end, &radius, NULL, "arrow");
		*pEx = (int)(*pSx + cos(DEG2RAD * pArrow->angle) * radius);
		*pEy = (int)(*pSy + sin(DEG2RAD * pArrow->angle) * radius * aspect);
	}
	else {
		MapPositionDouble(term, &pArrow->end, &ex_d, &ey_d, "arrow");
		*pEx = (int)(ex_d);
		*pEy = (int)(ey_d);
	}
}

void GpGadgets::PlaceGrid(GpTermEntry * pT, int layer)
{
	//GpTermEntry * t = term;
	int save_lgrid = grid_lp.l_type;
	int save_mgrid = mgrid_lp.l_type;
	ApplyLpProperties(pT, &BorderLp);   /* border linetype */
	LargestPolarCircle = 0.0;
	//
	// We used to go through this process only once, drawing both the grid lines
	// and the axis tic labels.  Now we allow for a separate pass that redraws only
	// the labels if the user has chosen "set tics front".
	// This guarantees that the axis tic labels lie on top of all grid lines.
	//
	if(layer == LAYER_FOREGROUND)
		grid_lp.l_type = mgrid_lp.l_type = LT_NODRAW;

	// select first mapping
	XAxis = FIRST_X_AXIS;
	YAxis = FIRST_Y_AXIS;

	// label first y axis tics
	AxisOutputTics(pT, FIRST_Y_AXIS, &B.ytic_x, FIRST_X_AXIS, &GpGadgets::YTick2D_Callback);
	// label first x axis tics
	AxisOutputTics(pT, FIRST_X_AXIS, &B.xtic_y, FIRST_Y_AXIS, &GpGadgets::XTick2D_Callback);

	// select second mapping
	XAxis = SECOND_X_AXIS;
	YAxis = SECOND_Y_AXIS;

	AxisOutputTics(pT, SECOND_Y_AXIS, &B.y2tic_x, SECOND_X_AXIS, &GpGadgets::YTick2D_Callback);
	AxisOutputTics(pT, SECOND_X_AXIS, &B.x2tic_y, SECOND_Y_AXIS, &GpGadgets::XTick2D_Callback);
	// select first mapping
	XAxis = FIRST_X_AXIS;
	YAxis = FIRST_Y_AXIS;
	//
	// POLAR GRID
	//
	if(IsPolar && GetR().ticmode) {
		// Piggyback on the xtick2d_callback.  Avoid a call to the full
		// AxisOutputTics(), which wasn't really designed for this axis.
		TicStart   = MapY(0); // Always equivalent to tics on phi=0 axis
		TicMirror  = TicStart; // tic extends on both sides of phi=0
		TicText    = TicStart - pT->VChr;
		RotateTics = GetR().tic_rotate;
		if(RotateTics == 0)
			TicHJust = CENTRE;
		else if(pT->text_angle(RotateTics))
			TicHJust = (RotateTics == TEXT_VERTICAL) ? RIGHT : LEFT;
		if(GetR().Flags & GpAxis::fManualJustify)
			TicHJust = GetR().label.pos;
		TicDirection = 1;
		GenTics(pT, AxA[POLAR_AXIS], &GpGadgets::XTick2D_Callback);
		pT->text_angle(0);
	}
	// Radial lines
	if(polar_grid_angle) {
		double theta = 0.0;
		int ox = MapX(0);
		int oy = MapY(0);
		ApplyLpProperties(pT, &grid_lp);
		for(theta = 0; theta < 6.29; theta += polar_grid_angle) {
			const int x = MapX(LargestPolarCircle * cos(theta));
			const int y = MapY(LargestPolarCircle * sin(theta));
			DrawClipLine(pT, ox, oy, x, y);
		}
		DrawClipLine(pT, ox, oy, MapX(LargestPolarCircle * cos(theta)), MapY(LargestPolarCircle * sin(theta)));
	}
	// Restore the grid line types if we had turned them off to draw labels only
	grid_lp.l_type = save_lgrid;
	mgrid_lp.l_type = save_mgrid;
}

//static void place_arrows(int layer)
void GpGadgets::PlaceArrows(GpTermEntry * pT, int layer)
{
	BoundingBox * clip_save = P_Clip;
	// Allow arrows to run off the plot, so long as they are still on the Canvas
	P_Clip = (pT->flags & TERM_CAN_CLIP) ? 0 :  &Canvas;
	for(arrow_def * p_arrow = first_arrow; p_arrow != NULL; p_arrow = p_arrow->next) {
		int sx, sy, ex, ey;
		if(p_arrow->arrow_properties.layer == layer) {
			GetArrow(p_arrow, &sx, &sy, &ex, &ey);
			ApplyLpProperties(pT, &(p_arrow->arrow_properties.lp_properties));
			ApplyHeadProperties(p_arrow->arrow_properties);
			DrawClipArrow(pT, sx, sy, ex, ey, p_arrow->arrow_properties.head);
		}
	}
	ApplyLpProperties(pT, &BorderLp);
	P_Clip = clip_save;
}

//static void place_labels(GpTermEntry * pT, GpTextLabel * listhead, int layer, bool clip)
void GpGadgets::PlaceLabels(GpTermEntry * pT, GpTextLabel * listhead, int layer, bool clip)
{
	pT->pointsize(PtSz);
	for(GpTextLabel * p_label = listhead; p_label; p_label = p_label->next) {
		if(p_label->layer == layer) {
			int  x, y;
			if(layer == LAYER_PLOTLABELS) {
				x = MapX(p_label->place.x);
				y = MapY(p_label->place.y);
			}
			else
				MapPosition(pT, &p_label->place, &x, &y, "label");
			if(clip) {
				if(p_label->place.scalex == first_axes && !AxA[FIRST_X_AXIS].InRange(p_label->place.x))
					continue;
				if(p_label->place.scalex == second_axes && !AxA[SECOND_X_AXIS].InRange(p_label->place.x))
					continue;
				if(p_label->place.scaley == first_axes && !AxA[FIRST_Y_AXIS].InRange(p_label->place.y))
					continue;
				if(p_label->place.scaley == second_axes && !AxA[SECOND_Y_AXIS].InRange(p_label->place.y))
					continue;
			}
			WriteLabel(pT, x, y, p_label);
		}
	}
}

#ifdef EAM_OBJECTS
//void place_objects(t_object * pListHead, int layer, int dimensions)
void GpGadgets::PlaceObjects(GpTermEntry * pT, t_object * pListHead, int layer, int dimensions)
{
	double x1, y1;
	int style;
	for(t_object * p_obj = pListHead; p_obj != NULL; p_obj = p_obj->next) {
		if(p_obj->layer == layer) {
			// Extract line and fill style, but don't apply it yet
			lp_style_type lpstyle = p_obj->lp_properties;
			fill_style_type * fillstyle = (p_obj->fillstyle.fillstyle == FS_DEFAULT &&
				p_obj->object_type == OBJ_RECTANGLE) ? &DefaultRectangle.fillstyle : &p_obj->fillstyle;
			style = style_from_fill(fillstyle);
			ApplyLpProperties(pT, &lpstyle);
			switch(p_obj->object_type) {
				case OBJ_CIRCLE:
				{
					t_circle * e = &p_obj->o.circle;
					double radius;
					BoundingBox * clip_save = P_Clip;
					if(dimensions == 2 || e->center.scalex == screen) {
						MapPositionDouble(pT, &e->center, &x1, &y1, "rect");
						MapPositionR(e->extent, &radius, NULL, "rect");
					}
					else if(splot_map) {
						int junkw, junkh;
						Map3DPositionDouble(e->center, &x1, &y1, "rect");
						Map3DPositionR(e->extent, &junkw, &junkh, "rect");
						radius = junkw;
					}
					else
						break;
					if((e->center.scalex == screen || e->center.scaley == screen) || (p_obj->clip == OBJ_NOCLIP))
						P_Clip = &Canvas;
					do_arc((int)x1, (int)y1, radius, e->arc_begin, e->arc_end, style, false);
					// Retrace the border if the style requests it 
					if(NeedFillBorder(pT, fillstyle))
						do_arc((int)x1, (int)y1, radius, e->arc_begin, e->arc_end, 0, e->wedge);
					P_Clip = clip_save;
					break;
				}
				case OBJ_ELLIPSE:
				{
					t_ellipse * e = &p_obj->o.ellipse;
					BoundingBox * clip_save = P_Clip;
					if((e->center.scalex == screen || e->center.scaley == screen) || (p_obj->clip == OBJ_NOCLIP))
						P_Clip = &Canvas;
					if(dimensions == 2)
						DoEllipse(pT, 2, e, style, true);
					else if(splot_map)
						DoEllipse(pT, 3, e, style, true);
					else
						break;
					// Retrace the border if the style requests it 
					if(NeedFillBorder(pT, fillstyle))
						DoEllipse(pT, dimensions, e, 0, true);
					P_Clip = clip_save;
					break;
				}
				case OBJ_POLYGON:
				{
					DoPolygon(pT, dimensions, &p_obj->o.polygon, style, p_obj->clip);
					// Retrace the border if the style requests it 
					if(NeedFillBorder(pT, fillstyle))
						DoPolygon(pT, dimensions, &p_obj->o.polygon, 0, p_obj->clip);
					break;
				}
				case OBJ_RECTANGLE:
				{
					DoRectangle(pT, dimensions, p_obj, fillstyle);
					break;
				}
				default:
					break;
			}
		}
	}
}

#endif
//
// Apply axis range expansions from "set offsets" command
//
//static void adjust_offsets()
void GpGadgets::AdjustOffsets()
{
	double b = boff.scaley == graph ? fabs(GetY().GetRange())*boff.y : boff.y;
	double t = toff.scaley == graph ? fabs(GetY().GetRange())*toff.y : toff.y;
	double l = loff.scalex == graph ? fabs(GetX().GetRange())*loff.x : loff.x;
	double r = roff.scalex == graph ? fabs(GetX().GetRange())*roff.x : roff.x;
	if(GetY().Range.low < GetY().Range.upp) {
		GetY().Range.low -= b;
		GetY().Range.upp += t;
	}
	else {
		GetY().Range.upp -= b;
		GetY().Range.low += t;
	}
	if(GetX().Range.low < GetX().Range.upp) {
		GetX().Range.low -= l;
		GetX().Range.upp += r;
	}
	else {
		GetX().Range.upp -= l;
		GetX().Range.low += r;
	}
	if(GetX().Range.low == GetX().Range.upp)
		GpGg.IntErrorNoCaret("x_min should not equal x_max!");
	if(GetY().Range.low == GetY().Range.upp)
		GpGg.IntErrorNoCaret("y_min should not equal y_max!");
	if(AxA[FIRST_X_AXIS].P_LinkToScnd)
		clone_linked_axes(&AxA[FIRST_X_AXIS], &AxA[SECOND_X_AXIS]);
	if(AxA[FIRST_Y_AXIS].P_LinkToScnd)
		clone_linked_axes(&AxA[FIRST_Y_AXIS], &AxA[SECOND_Y_AXIS]);
}

//void do_plot(CurvePoints * plots, int pcount)
void GpGadgets::DoPlot(CurvePoints * pPlots, int pcount)
{
	int    curve;
	CurvePoints * p_plot = NULL;
	int xl = 0, yl = 0;
	int key_count = 0;
	bool key_pass = false;
	legend_key * key = &keyT;
	int previous_plot_style;
	XAxis = FIRST_X_AXIS;
	YAxis = FIRST_Y_AXIS;
	AdjustOffsets();
	//
	// EAM June 2003 - Although the comment below implies that font dimensions
	// are known after term_initialise(), this is not true at least for the X11
	// driver.  X11 fonts are not set until an actual display window is
	// opened, and that happens in term->graphics(), which is called from term_start_plot().
	//
	GpTermEntry * p_terminal = term;
	TermInitialise(); // may set xmax/ymax
	TermStartPlot(p_terminal);
	// Figure out if we need a colorbox for this plot
	SetPlotWithPalette(0, MODE_PLOT); /* EAM FIXME - 1st parameter is a dummy */
	/* compute boundary for plot (PlotBounds.xleft, PlotBounds.xright, PlotBounds.ytop, PlotBounds.ybot)
	 * also calculates tics, since xtics depend on PlotBounds.xleft
	 * but PlotBounds.xleft depends on ytics. Boundary calculations depend
	 * on term->VChr etc, so terminal must be initialised first.
	 */
	B.Boundary(p_terminal, *this, pPlots, pcount);
	// Make palette
	if(is_plot_with_palette())
		MakePalette(p_terminal);
	// Give a chance for rectangles to be behind everything else
	PlaceObjects(p_terminal, first_object, LAYER_BEHIND, 2);
	screen_ok = false;
	// Sync point for epslatex text positioning
	p_terminal->layer(TERM_LAYER_BACKTEXT);
	// DRAW TICS AND GRID
	if(oneof2(grid_layer, LAYER_BACK, LAYER_BEHIND))
		PlaceGrid(p_terminal, grid_layer);
	// DRAW ZERO AXES and update axis->term_zero
	Draw2DZeroAxis(p_terminal, FIRST_X_AXIS, FIRST_Y_AXIS);
	Draw2DZeroAxis(p_terminal, FIRST_Y_AXIS, FIRST_X_AXIS);
	Draw2DZeroAxis(p_terminal, SECOND_X_AXIS, SECOND_Y_AXIS);
	Draw2DZeroAxis(p_terminal, SECOND_Y_AXIS, SECOND_X_AXIS);
	// DRAW VERTICAL AXES OF PARALLEL GpAxis PLOTS
	PlaceParallelAxes(p_terminal, pPlots, pcount, LAYER_BACK);
	// DRAW PLOT BORDER
	if(DrawBorder)
		PlotBorder(p_terminal);
	// Add back colorbox if appropriate
	if(is_plot_with_colorbox() && ColorBox.layer == LAYER_BACK)
		DrawColorSmoothBox(p_terminal, MODE_PLOT);
	// And rectangles
	PlaceObjects(p_terminal, first_object, LAYER_BACK, 2);
	// PLACE LABELS
	PlaceLabels(p_terminal, first_label, LAYER_BACK, false);
	// PLACE ARROWS
	PlaceArrows(p_terminal, LAYER_BACK);
	// Sync point for epslatex text positioning
	p_terminal->layer(TERM_LAYER_FRONTTEXT);
	// Draw plot title and axis labels
	// Note: As of Dec 2012 these are drawn as "front" text.
	B.DrawTitles(p_terminal, *this);
	// Draw the key, or at least reserve space for it (pass 1)
	if(key->visible)
		B.DrawKey(p_terminal, *this, key, key_pass, &xl, &yl);
SECOND_KEY_PASS:
	// This tells the Canvas, qt, and svg terminals to restart the plot
	// count so that key titles are in sync with the plots they describe.
	p_terminal->layer(TERM_LAYER_RESET_PLOTNO);
	//
	// DRAW CURVES
	//
	p_plot = pPlots;
	previous_plot_style = 0;
	for(curve = 0; curve < pcount; p_plot = p_plot->P_Next, curve++) {
		bool localkey = key->visible; /* a local copy */
		// Sync point for start of new curve (used by svg, post, ...)
		if(p_terminal->hypertext) {
			char * plaintext = p_plot->title_no_enhanced ? p_plot->title : estimate_plaintext(p_plot->title);
			p_terminal->hypertext(TERM_HYPERTEXT_TITLE, plaintext);
		}
		p_terminal->layer(TERM_LAYER_BEFORE_PLOT);
		// set scaling for this plot's axes
		XAxis = (AXIS_INDEX)p_plot->x_axis;
		YAxis = (AXIS_INDEX)p_plot->y_axis;
		// Crazy corner case handling Bug #3499425
		if(p_plot->plot_style == HISTOGRAMS)
			if((!key_pass && key->front) &&  (prefer_line_styles)) {
				lp_style_type ls;
				lp_use_properties(&ls, p_plot->lp_properties.l_type+1);
				p_plot->lp_properties.pm3d_color = ls.pm3d_color;
			}
		ApplyLpProperties(p_terminal, &(p_plot->lp_properties));
		// Skip a line in the key between histogram clusters
		if(p_plot->plot_style == HISTOGRAMS && previous_plot_style == HISTOGRAMS && p_plot->histogram_sequence == 0 && yl != B.yl_ref) {
			if(++key_count >= B.key_rows) {
				yl = B.yl_ref;
				xl += B.key_col_wth;
				key_count = 0;
			}
			else
				yl = yl - B.KeyEntryHeight;
		}
		// Column-stacked histograms store their key titles internally
		if(p_plot->plot_style == HISTOGRAMS && histogram_opts.type == HT_STACKED_IN_TOWERS) {
			GpTextLabel * key_entry;
			localkey = 0;
			if(p_plot->labels && (key_pass || !key->front)) {
				lp_style_type save_lp = p_plot->lp_properties;
				for(key_entry = p_plot->labels->next; key_entry; key_entry = key_entry->next) {
					int histogram_linetype = key_entry->tag + p_plot->histogram->startcolor;
					p_plot->lp_properties.l_type = histogram_linetype;
					p_plot->fill_properties.fillpattern = histogram_linetype;
					if(key_entry->text) {
						if(prefer_line_styles)
							lp_use_properties(&p_plot->lp_properties, histogram_linetype);
						else
							load_linetype(&p_plot->lp_properties, histogram_linetype);
						B.DoKeySample(p_terminal, *this, p_plot, key, key_entry->text, xl, yl);
					}
					if(++key_count >= B.key_rows) {
						yl = B.yl_ref;
						xl += B.key_col_wth;
						key_count = 0;
					}
					else
						yl = yl - B.KeyEntryHeight;
				}
				GpTextLabel::Destroy(p_plot->labels);
				p_plot->labels = 0;
				p_plot->lp_properties = save_lp;
			}
		}
		else if(p_plot->title && !*p_plot->title) {
			localkey = false;
		}
		else if(p_plot->plot_type == NODATA) {
			localkey = false;
		}
		else if(key_pass || !key->front) {
			ignore_enhanced(p_plot->title_no_enhanced);
			// don't write filename or function enhanced
			if(localkey && p_plot->title && !p_plot->title_is_suppressed) {
				if(!p_plot->title_position) {
					key_count++;
					if(key->invert)
						yl = key->bounds.ybot + B.yl_ref + B.KeyEntryHeight/2 - yl;
				}
				B.DoKeySample(p_terminal, *this, p_plot, key, p_plot->title, xl, yl);
			}
			ignore_enhanced(false);
		}
		// If any plots have opted out of autoscaling, we need to recheck
		// whether their points are INRANGE or not.
		if(p_plot->noautoscale && !key_pass) {
			//
			// Plots marked "noautoscale" do not yet have INRANGE/OUTRANGE flags set.
			//
			const GpAxis & r_ax_x = AxA[p_plot->x_axis];
			const GpAxis & r_ax_y = AxA[p_plot->y_axis];
			for(int i = 0; i < p_plot->p_count; i++) {
				GpCoordinate & r_c = p_plot->points[i];
				r_c.type = (r_ax_x.InRange(r_c.x) && r_ax_y.InRange(r_c.y)) ? INRANGE : OUTRANGE;
			}
		}
		// and now the curves, plus any special key requirements
		// be sure to draw all lines before drawing any points
		// Skip missing/empty curves
		if(p_plot->plot_type != NODATA && !key_pass) {
			switch(p_plot->plot_style) {
				case IMPULSES:
				    PlotImpulses(p_terminal, p_plot, GetX().term_zero, GetY().term_zero);
				    break;
				case LINES:
				    PlotLines(p_terminal, p_plot);
				    break;
				case STEPS:
				case FILLSTEPS:
				    PlotSteps(p_terminal, p_plot);
				    break;
				case FSTEPS:
				    PlotFSteps(p_terminal, p_plot);
				    break;
				case HISTEPS:
				    PlotHiSteps(p_terminal, p_plot);
				    break;
				case POINTSTYLE:
				    PlotPoints(p_terminal, p_plot);
				    break;
				case LINESPOINTS:
				    PlotLines(p_terminal, p_plot);
				    PlotPoints(p_terminal, p_plot);
				    break;
				case DOTS:
				    PlotDots(p_terminal, p_plot);
				    break;
				case YERRORLINES:
				case XERRORLINES:
				case XYERRORLINES:
				    PlotLines(p_terminal, p_plot);
				    PlotBars(p_terminal, p_plot);
				    PlotPoints(p_terminal, p_plot);
				    break;
				case YERRORBARS:
				case XERRORBARS:
				case XYERRORBARS:
				    PlotBars(p_terminal, p_plot);
				    PlotPoints(p_terminal, p_plot);
				    break;
				case BOXXYERROR:
				case BOXES:
				    PlotBoxes(p_terminal, p_plot, GetY().term_zero);
				    break;
				case HISTOGRAMS:
				    if(BarLayer == LAYER_FRONT)
					    PlotBoxes(p_terminal, p_plot, GetY().term_zero);
				    // Draw the bars first, so that the box will cover the bottom half
				    if(histogram_opts.type == HT_ERRORBARS) {
					    // Note that the bar linewidth may not match the border or plot linewidth
					    p_terminal->linewidth(histogram_opts.bar_lw);
					    if(!NeedFillBorder(p_terminal, &DefaultFillStyle))
						    p_terminal->linetype(p_plot->lp_properties.l_type);
					    PlotBars(p_terminal, p_plot);
					    ApplyLpProperties(p_terminal, &(p_plot->lp_properties));
				    }
				    if(BarLayer != LAYER_FRONT)
					    PlotBoxes(p_terminal, p_plot, GetY().term_zero);
				    break;
				case BOXERROR:
				    if(BarLayer != LAYER_FRONT)
					    PlotBars(p_terminal, p_plot);
				    PlotBoxes(p_terminal, p_plot, GetY().term_zero);
				    if(BarLayer == LAYER_FRONT)
					    PlotBars(p_terminal, p_plot);
				    break;
				case FILLEDCURVES:
				    if(oneof4(p_plot->filledcurves_options.closeto, FILLEDCURVES_ATY1, FILLEDCURVES_ATY2, FILLEDCURVES_ATR, FILLEDCURVES_BETWEEN)) {
					    PlotBetweenCurves(p_terminal, p_plot);
				    }
				    else {
					    PlotFilledCurves(p_terminal, p_plot);
					    if(NeedFillBorder(p_terminal, &p_plot->fill_properties))
						    PlotLines(p_terminal, p_plot);
				    }
				    break;
				case VECTOR:
				    PlotVectors(p_terminal, p_plot);
				    break;
				case FINANCEBARS:
				    PlotFBars(p_terminal, p_plot);
				    break;
				case CANDLESTICKS:
				    PlotCBars(p_terminal, p_plot);
				    break;
				case BOXPLOT:
				    PlotBoxPlot(p_terminal, p_plot);
				    break;
				case PM3DSURFACE:
				case SURFACEGRID:
				    IntWarn(NO_CARET, "Can't use pm3d or surface for 2d plots");
				    break;
				case LABELPOINTS:
				    PlaceLabels(p_terminal, p_plot->labels->next, LAYER_PLOTLABELS, true);
				    break;
				case IMAGE:
				    p_plot->image_properties.type = IC_PALETTE;
				    ProcessImage(p_terminal, p_plot, IMG_PLOT);
				    break;
				case RGBIMAGE:
				    p_plot->image_properties.type = IC_RGB;
				    ProcessImage(p_terminal, p_plot, IMG_PLOT);
				    break;
				case RGBA_IMAGE:
				    p_plot->image_properties.type = IC_RGBA;
				    ProcessImage(p_terminal, p_plot, IMG_PLOT);
				    break;
#ifdef EAM_OBJECTS
				case CIRCLES:
				    plot_circles(p_plot);
				    break;

				case ELLIPSES:
				    PlotEllipses(p_terminal, p_plot);
				    break;
#endif
				case PARALLELPLOT:
				    plot_parallel(p_plot);
				    break;
				default:
				    GpGg.IntErrorNoCaret("unknown plot style");
			}
		}
		// If there are two passes, defer key sample till the second
		// KEY SAMPLES
		if(key->front && !key_pass)
			;
		else if(localkey && p_plot->title && !p_plot->title_is_suppressed) {
			// we deferred point sample until now
			if(p_plot->plot_style & PLOT_STYLE_HAS_POINT)
				B.DoKeySamplePoint(p_terminal, *this, p_plot, key, xl, yl);
			if(p_plot->plot_style == LABELPOINTS)
				B.DoKeySamplePoint(p_terminal, *this, p_plot, key, xl, yl);
			if(p_plot->plot_style == DOTS)
				B.DoKeySamplePoint(p_terminal, *this, p_plot, key, xl, yl);
			if(!p_plot->title_position) {
				if(key->invert)
					yl = key->bounds.ybot + B.yl_ref + B.KeyEntryHeight/2 - yl;
				if(key_count >= B.key_rows) {
					yl = B.yl_ref;
					xl += B.key_col_wth;
					key_count = 0;
				}
				else
					yl = yl - B.KeyEntryHeight;
			}
		}
		// Option to label the end of the curve on the plot itself
		if(p_plot->title_position && p_plot->title_position->scalex == character)
			AttachTitleToPlot(p_terminal, p_plot, key);
		// Sync point for end of this curve (used by svg, post, ...)
		p_terminal->layer(TERM_LAYER_AFTER_PLOT);
		previous_plot_style = p_plot->plot_style;
	}
	// Go back and draw the legend in a separate pass if necessary
	if(key->visible && key->front && !key_pass) {
		key_pass = true;
		B.DrawKey(p_terminal, *this, key, key_pass, &xl, &yl);
		goto SECOND_KEY_PASS;
	}
	//
	// DRAW TICS AND GRID
	//
	if(grid_layer == LAYER_FRONT)
		PlaceGrid(p_terminal, grid_layer);
	if(IsPolar && raxis)
		PlaceRAxis(p_terminal);
	// Redraw the axis tic labels and tic marks if "set tics front"
	if(grid_tics_in_front)
		PlaceGrid(p_terminal, LAYER_FOREGROUND);
	// DRAW ZERO AXES
	// redraw after grid so that axes linetypes are on top
	if(grid_layer == LAYER_FRONT) {
		Draw2DZeroAxis(p_terminal, FIRST_X_AXIS, FIRST_Y_AXIS);
		Draw2DZeroAxis(p_terminal, FIRST_Y_AXIS, FIRST_X_AXIS);
		Draw2DZeroAxis(p_terminal, SECOND_X_AXIS, SECOND_Y_AXIS);
		Draw2DZeroAxis(p_terminal, SECOND_Y_AXIS, SECOND_X_AXIS);
	}
	// DRAW VERTICAL AXES OF PARALLEL GpAxis PLOTS
	if(parallel_axis_style.layer == LAYER_FRONT)
		PlaceParallelAxes(p_terminal, pPlots, pcount, LAYER_FRONT);
	// REDRAW PLOT BORDER
	if(DrawBorder && BorderLayer == LAYER_FRONT)
		PlotBorder(p_terminal);
	// Add front colorbox if appropriate
	if(is_plot_with_colorbox() && ColorBox.layer == LAYER_FRONT)
		DrawColorSmoothBox(p_terminal, MODE_PLOT);
	// And rectangles
	PlaceObjects(p_terminal, first_object, LAYER_FRONT, 2);
	// PLACE LABELS
	PlaceLabels(p_terminal, first_label, LAYER_FRONT, false);
	// PLACE HISTOGRAM TITLES
	PlaceHistogramTitles(p_terminal);
	// PLACE ARROWS
	PlaceArrows(p_terminal, LAYER_FRONT);
	// Release the palette if we have used one (PostScript only?)
	if(is_plot_with_palette() && p_terminal->previous_palette)
		p_terminal->previous_palette();
	TermEndPlot(p_terminal);
}
//
// plot_impulses:
// Plot the curves in IMPULSES style
//
//static void plot_impulses(CurvePoints * plot, int yaxis_x, int xaxis_y)
void GpGadgets::PlotImpulses(GpTermEntry * pT, CurvePoints * plot, int yaxis_x, int xaxis_y)
{
	for(int i = 0; i < plot->p_count; i++) {
		if(plot->points[i].type != UNDEFINED) {
			if(IsPolar || GetX().InRange(plot->points[i].x)) {
				// This catches points that are outside trange[theta_min:theta_max]
				if(!IsPolar || plot->points[i].type != EXCLUDEDRANGE) {
					const int x = MapX(plot->points[i].x);
					const int y = MapY(plot->points[i].y);
					check_for_variable_color(plot, &plot->varcolor[i]);
					if(IsPolar)
						DrawClipLine(pT, yaxis_x, xaxis_y, x, y);
					else
						DrawClipLine(pT, x, xaxis_y, x, y);
				}
			}
		}
	}
}
//
// Plot the curves in LINES style
//
//static void plot_lines(CurvePoints * plot)
void GpGadgets::PlotLines(GpTermEntry * pT, CurvePoints * pPlot)
{
	int x, y;               /* point in terminal coordinates */
	enum coord_type prev = UNDEFINED; /* type of previous point */
	double ex, ey;          /* an edge point */
	double lx[2], ly[2];    /* two edge points */
	// If all the lines are invisible, don'pT bother to draw them
	if(pPlot->lp_properties.l_type != LT_NODRAW) {
		for(int i = 0; i < pPlot->p_count; i++) {
			// rgb variable  -  color read from data column
			check_for_variable_color(pPlot, &pPlot->varcolor[i]);
			switch(pPlot->points[i].type) {
				case INRANGE: {
					// MapX or MapY can hit NaN during eval_link_function(), in which
					// case the GpCoordinate value is garbage and undefined is true.
					Ev.undefined = false;
					x = MapX(pPlot->points[i].x);
					if(Ev.undefined)
						pPlot->points[i].type = UNDEFINED;
					y = MapY(pPlot->points[i].y);
					if(Ev.undefined)
						pPlot->points[i].type = UNDEFINED;
					if(pPlot->points[i].type == UNDEFINED)
						break;
					if(prev == INRANGE) {
						pT->_Vector(x, y);
					}
					else if(prev == OUTRANGE) {
						// from outrange to inrange
						if(!ClipLines1) {
							pT->_Move(x, y);
						}
						else {
							EdgeIntersect(pPlot->points, i, &ex, &ey);
							pT->_Move(MapX(ex), MapY(ey));
							pT->_Vector(x, y);
						}
					}
					else { /* prev == UNDEFINED */
						pT->_Move(x, y);
						pT->_Vector(x, y);
					}
					break;
				}
				case OUTRANGE: {
					if(prev == INRANGE) {
						// from inrange to outrange
						if(ClipLines1) {
							EdgeIntersect(pPlot->points, i, &ex, &ey);
							pT->_Vector(MapX(ex), MapY(ey));
						}
					}
					else if(prev == OUTRANGE) {
						// from outrange to outrange
						if(ClipLines2) {
							if(TwoEdgeIntersect(pPlot->points, i, lx, ly)) {
								pT->_Move(MapX(lx[0]), MapY(ly[0]));
								pT->_Vector(MapX(lx[1]), MapY(ly[1]));
							}
						}
					}
					break;
				}
				default: /* just a safety */
				case UNDEFINED: {
					break;
				}
			}
			prev = pPlot->points[i].type;
		}
	}
}
//
// plot_filledcurves:
// {closed | {above | below} {x1 | x2 | y1 | y2 | r}[=<a>] | xy=<x>,<y>}
//
// finalize and draw the filled curve
//static void finish_filled_curve(int points, gpiPoint * corners, CurvePoints * plot)
void GpGadgets::FinishFilledCurve(GpTermEntry * pT, int points, gpiPoint * corners, CurvePoints * pPlot)
{
	static gpiPoint * clipcorners = NULL; // @global
	int clippoints;
	filledcurves_opts * filledcurves_options = &pPlot->filledcurves_options;
	long side = 0;
	int i;
	if(points > 0) {
		// add side (closing) points
		switch(filledcurves_options->closeto) {
			case FILLEDCURVES_CLOSED:
				break;
			case FILLEDCURVES_X1:
				corners[points].x   = corners[points-1].x;
				corners[points+1].x = corners[0].x;
				corners[points].y   = corners[points+1].y = AxA[FIRST_Y_AXIS].TermBounds.low;
				points += 2;
				break;
			case FILLEDCURVES_X2:
				corners[points].x   = corners[points-1].x;
				corners[points+1].x = corners[0].x;
				corners[points].y   = corners[points+1].y = AxA[FIRST_Y_AXIS].TermBounds.upp;
				points += 2;
				break;
			case FILLEDCURVES_Y1:
				corners[points].y   = corners[points-1].y;
				corners[points+1].y = corners[0].y;
				corners[points].x   = corners[points+1].x = AxA[FIRST_X_AXIS].TermBounds.low;
				points += 2;
				break;
			case FILLEDCURVES_Y2:
				corners[points].y   = corners[points-1].y;
				corners[points+1].y = corners[0].y;
				corners[points].x   = corners[points+1].x = AxA[FIRST_X_AXIS].TermBounds.upp;
				points += 2;
				break;
			case FILLEDCURVES_ATX1:
			case FILLEDCURVES_ATX2:
				corners[points].x = corners[points+1].x = MapX(filledcurves_options->at);
				// should be mapping real x1/x2axis/graph/screen => screen
				corners[points].y   = corners[points-1].y;
				corners[points+1].y = corners[0].y;
				for(i = 0; i<points; i++)
					side += corners[i].x - corners[points].x;
				points += 2;
				break;
			case FILLEDCURVES_ATXY:
				corners[points].x = MapX(filledcurves_options->at);
				// should be mapping real x1axis/graph/screen => screen
				corners[points].y = MapY(filledcurves_options->aty);
				// should be mapping real y1axis/graph/screen => screen
				points++;
				break;
			case FILLEDCURVES_ATY1:
			case FILLEDCURVES_ATY2:
				corners[points].y = MapY(filledcurves_options->at);
				corners[points+1].y = corners[points].y;
				corners[points].x = corners[points-1].x;
				corners[points+1].x = corners[0].x;
				points += 2;
			// Fall through
			case FILLEDCURVES_BETWEEN:
				// fill_between() allocated an extra point for the above/below flag
				if(filledcurves_options->closeto == FILLEDCURVES_BETWEEN)
					side = (corners[points].x > 0) ? 1 : -1;
			// Fall through
			case FILLEDCURVES_ATR:
				// Prevent 1-pixel overlap of component rectangles, which
				// causes vertical stripe artifacts for transparent fill
				if(pPlot->fill_properties.fillstyle == FS_TRANSPARENT_SOLID) {
					int direction = (corners[2].x < corners[0].x) ? -1 : 1;
					if(points >= 4 && corners[2].x == corners[3].x) {
						corners[2].x -= direction, corners[3].x -= direction;
					}
					else if(points >= 5 && corners[3].x == corners[4].x) {
						corners[3].x -= direction, corners[4].x -= direction;
					}
				}
				break;
			default: // the polygon is closed by default
				break;
		}
		// Check for request to fill only on one side of a bounding line
		if(filledcurves_options->oneside > 0 && side < 0)
			return;
		else if(filledcurves_options->oneside < 0 && side > 0)
			return;
		else {
			// EAM Apr 2013 - Use new polygon clipping code
			clipcorners = (gpiPoint *)gp_realloc(clipcorners, 2*points*sizeof(gpiPoint), "filledcurve verticess");
			ClipPolygon(corners, clipcorners, points, &clippoints);
			clipcorners->style = style_from_fill(&pPlot->fill_properties);
			if(clippoints > 0)
				pT->filled_polygon(clippoints, clipcorners);
		}
	}
}

//static void plot_filledcurves(CurvePoints * plot)
void GpGadgets::PlotFilledCurves(GpTermEntry * pT, CurvePoints * pPlot)
{
	int i;                  /* point index */
	int x, y;               /* point in terminal coordinates */
	enum coord_type prev = UNDEFINED; /* type of previous point */
	int points = 0;                 /* how many corners */
	static gpiPoint * corners = 0;  /* array of corners */
	static int corners_allocated = 0; /* how many allocated */
	if(!pT->filled_polygon) { /* filled polygons are not available */
		PlotLines(pT, pPlot);
	}
	else {
		if(!pPlot->filledcurves_options.opt_given) {
			// no explicitly given filledcurves option for the current pPlot =>
			// use the default for data or function, respectively
			pPlot->filledcurves_options = (pPlot->plot_type == DATA) ? FilledcurvesOptsData : FilledcurvesOptsFunc;
		}
		// clip the "at" GpCoordinate to the drawing area
		switch(pPlot->filledcurves_options.closeto) {
			case FILLEDCURVES_ATX1:
				pPlot->filledcurves_options.at = AxA[FIRST_X_AXIS].Range.Clip(pPlot->filledcurves_options.at);
				break;
			case FILLEDCURVES_ATX2:
				pPlot->filledcurves_options.at = AxA[SECOND_X_AXIS].Range.Clip(pPlot->filledcurves_options.at);
				break;
			case FILLEDCURVES_ATY1:
			case FILLEDCURVES_ATY2:
				pPlot->filledcurves_options.at = AxA[pPlot->y_axis].Range.Clip(pPlot->filledcurves_options.at);
				break;
			case FILLEDCURVES_ATXY:
				pPlot->filledcurves_options.at  = AxA[FIRST_X_AXIS].Range.Clip(pPlot->filledcurves_options.at);
				pPlot->filledcurves_options.aty = AxA[FIRST_Y_AXIS].Range.Clip(pPlot->filledcurves_options.aty);
				break;
		}
		for(i = 0; i < pPlot->p_count; i++) {
			if(points+2 >= corners_allocated) { // there are 2 side points
				corners_allocated += 128; // reallocate more corners
				corners = (gpiPoint *)gp_realloc(corners, corners_allocated*sizeof(gpiPoint), "filledcurve vertices");
			}
			switch(pPlot->points[i].type) {
				case INRANGE:
				case OUTRANGE:
					x = MapX(pPlot->points[i].x);
					y = MapY(pPlot->points[i].y);
					corners[points].x = x;
					corners[points].y = y;
					if(points == 0)
						check_for_variable_color(pPlot, &pPlot->varcolor[i]);
					points++;
					break;
				case UNDEFINED:
					// UNDEFINED flags a blank line in the input file.
					// Unfortunately, it can also mean that the point was undefined.
					// Is there a clean way to detect or handle the latter case?
					if(prev != UNDEFINED) {
						FinishFilledCurve(pT, points, corners, pPlot);
						points = 0;
					}
					break;
				default: /* just a safety */
					break;
			}
			prev = pPlot->points[i].type;
		}
		FinishFilledCurve(pT, points, corners, pPlot);
	}
}
//
// Fill the area between two curves
//
//static void plot_betweencurves(CurvePoints * plot)
void GpGadgets::PlotBetweenCurves(GpTermEntry * pT, CurvePoints * pPlot)
{
	// If terminal doesn't support filled polygons, approximate with bars
	if(!pT->filled_polygon) {
		PlotBars(pT, pPlot);
	}
	else {
		/* Jan 2015: We are now using the plot_between code to also handle option
		* y=atval, but the style option in the pPlot header does not reflect this.
		* Change it here so that finish_filled_curve() doesn't get confused.
		*/
		pPlot->filledcurves_options.closeto = FILLEDCURVES_BETWEEN;
		/*
		* Fill the region one quadrilateral at a time.
		* Check each interval to see if the curves cross.
		* If so, split the interval into two parts.
		*/
		for(int i = 0; i < pPlot->p_count-1; i++) {
			/* FIXME: This isn't really testing for undefined points, it	*/
			/* is looking for blank lines. We need to distinguish these.	*/
			/* Anyhow, if there's a blank line then start a new fill area.	*/
			if(pPlot->points[i].type != UNDEFINED && pPlot->points[i+1].type != UNDEFINED) {
				const double x1  = pPlot->points[i].x;
				const double xu1 = pPlot->points[i].xhigh;
				const double yl1 = pPlot->points[i].y;
				const double yu1 = pPlot->points[i].yhigh;
				const double x2  = pPlot->points[i+1].x;
				const double xu2 = pPlot->points[i+1].xhigh;
				const double yl2 = pPlot->points[i+1].y;
				const double yu2 = pPlot->points[i+1].yhigh;
				/* EAM 19-July-2007  Special case for polar plots. */
				if(IsPolar) {
					/* Find intersection of the two lines.                   */
					/* Probably could use this code in the general case too. */
					const double A = (yl2-yl1) / (x2-x1);
					const double C = (yu2-yu1) / (xu2-xu1);
					const double b = yl1 - x1 * A;
					const double d = yu1 - xu1 * C;
					const double xmid = (d-b) / (A-C);
					const double ymid = A * xmid + b;
					if((x1-xmid)*(xmid-x2) > 0) {
						FillBetween(x1, xu1, yl1, yu1, xmid, xmid, ymid, ymid, pPlot);
						FillBetween(xmid, xmid, ymid, ymid, x2, xu2, yl2, yu2, pPlot);
					}
					else
						FillBetween(x1, xu1, yl1, yu1, x2, xu2, yl2, yu2, pPlot);
				}
				else if((yu1-yl1)*(yu2-yl2) < 0) {
					/* Cheap test for intersection in the general case */
					const double xmid = (x1*(yl2-yu2) + x2*(yu1-yl1)) / ((yu1-yl1) + (yl2-yu2));
					const double ymid = yu1 + (yu2-yu1)*(xmid-x1)/(x2-x1);
					FillBetween(x1, xu1, yl1, yu1, xmid, xmid, ymid, ymid, pPlot);
					FillBetween(xmid, xmid, ymid, ymid, x2, xu2, yl2, yu2, pPlot);
				}
				else
					FillBetween(x1, xu1, yl1, yu1, x2, xu2, yl2, yu2, pPlot);
			}
		}
	}
}

//static void fill_between(double x1, double xu1, double yl1, double yu1,
//	double x2, double xu2, double yl2, double yu2, CurvePoints * plot)
void GpGadgets::FillBetween(double x1, double xu1, double yl1, double yu1, double x2, double xu2, double yl2, double yu2, CurvePoints * pPlot)
{
	gpiPoint box[5]; /* Must leave room for additional point if needed after clipping */

	box[0].x   = MapX(x1);
	box[0].y   = MapY(yl1);
	box[1].x   = MapX(xu1);
	box[1].y   = MapY(yu1);
	box[2].x   = MapX(xu2);
	box[2].y   = MapY(yu2);
	box[3].x   = MapX(x2);
	box[3].y   = MapY(yl2);
	// finish_filled_curve() will handle clipping, fill style, and
	// any distinction between above/below (flagged in box[4].x)
	if(IsPolar) {
		// "above" or "below" evaluated in terms of radial distance from origin
		// FIXME: Most of this should be offloaded to a separate subroutine
		double ox = MapX(0);
		double oy = MapY(0);
		double plx = MapX(x1);
		double ply = MapY(yl1);
		double pux = MapX(xu1);
		double puy = MapY(yu1);
		double drl = (plx-ox)*(plx-ox) + (ply-oy)*(ply-oy);
		double dru = (pux-ox)*(pux-ox) + (puy-oy)*(puy-oy);
		double dx1 = dru - drl;

		double dx2;
		plx = MapX(x2);
		ply = MapY(yl2);
		pux = MapX(xu2);
		puy = MapY(yu2);
		drl = (plx-ox)*(plx-ox) + (ply-oy)*(ply-oy);
		dru = (pux-ox)*(pux-ox) + (puy-oy)*(puy-oy);
		dx2 = dru - drl;

		box[4].x = (dx1+dx2 < 0) ? 1 : 0;
	}
	else
		box[4].x = ((yu1-yl1) + (yu2-yl2) < 0) ? 1 : 0;
	GpGg.FinishFilledCurve(term, 4, box, pPlot);
}
//
// plot_steps:
// Plot the curves in STEPS or FILLSTEPS style
// Each new value is reached by tracing horizontally to the new x value
// and then up/down to the new y value.
//
//static void plot_steps(CurvePoints * plot)
void GpGadgets::PlotSteps(GpTermEntry * pT, CurvePoints * pPlot)
{
	int i;                          /* point index */
	int x = 0, y = 0;               /* point in terminal coordinates */
	enum coord_type prev = UNDEFINED; /* type of previous point */
	int xprev, yprev;               /* previous point coordinates */
	int xleft, xright, ytop, ybot;  /* pPlot limits in terminal coords */
	int y0;                         /* baseline */
	int style = 0;
	/* EAM April 2011:  Default to lines only, but allow filled boxes */
	if((pPlot->plot_style & PLOT_STYLE_HAS_FILL) && pT->fillbox) {
		double ey = 0;
		style = style_from_fill(&pPlot->fill_properties);
		ey = (GetY().Flags & GpAxis::fLog) ? GetY().Range.low : GetY().Range.Clip(ey);
		y0 = MapY(ey);
	}
	xleft  = MapX(GetX().Range.low);
	xright = MapX(GetX().Range.upp);
	ybot = MapY(GetY().Range.low);
	ytop = MapY(GetY().Range.upp);
	for(i = 0; i < pPlot->p_count; i++) {
		xprev = x;
		yprev = y;
		switch(pPlot->points[i].type) {
			case INRANGE:
			case OUTRANGE:
			    x = MapX(pPlot->points[i].x);
			    y = MapY(pPlot->points[i].y);
			    if(prev == UNDEFINED)
				    break;
			    if(style) {
				    /* We don'pT yet have a generalized draw_clip_rectangle routine */
				    int xl = xprev;
				    int xr = x;
				    cliptorange(xr, xleft, xright);
				    cliptorange(xl, xleft, xright);
				    cliptorange(y, ybot, ytop);
				    /* Entire box is out of range on x */
				    if(xr == xl && (xr == xleft || xr == xright))
					    break;
				    if(yprev - y0 < 0)
					    (*pT->fillbox)(style, xl, yprev, (xr-xl), y0-yprev);
				    else
					    (*pT->fillbox)(style, xl, y0, (xr-xl), yprev-y0);
			    }
			    else {
				    DrawClipLine(pT, xprev, yprev, x, yprev);
				    DrawClipLine(pT, x, yprev, x, y);
			    }
			    break;
			default: /* just a safety */
			case UNDEFINED:
			    break;
		}
		prev = pPlot->points[i].type;
	}
}
//
// plot_fsteps:
// Each new value is reached by tracing up/down to the new y value
// and then horizontally to the new x value.
//
//static void plot_fsteps(CurvePoints * plot)
void GpGadgets::PlotFSteps(GpTermEntry * pT, CurvePoints * pPlot)
{
	int x = 0, y = 0;       /* point in terminal coordinates */
	enum coord_type prev = UNDEFINED; /* type of previous point */
	for(int i = 0; i < pPlot->p_count; i++) {
		const int xprev = x;
		const int yprev = y;
		switch(pPlot->points[i].type) {
			case INRANGE:
			case OUTRANGE:
			    x = MapX(pPlot->points[i].x);
			    y = MapY(pPlot->points[i].y);
			    if(prev == INRANGE) {
				    DrawClipLine(pT, xprev, yprev, xprev, y);
				    DrawClipLine(pT, xprev, y, x, y);
			    }
			    else if(prev == OUTRANGE) {
				    DrawClipLine(pT, xprev, yprev, xprev, y);
				    DrawClipLine(pT, xprev, y, x, y);
			    } /* remaining case (prev == UNDEFINED) do nothing */
			    break;
			default: /* just a safety */
			case UNDEFINED:
			    break;
		}
		prev = pPlot->points[i].type;
	}
}

/* HBB 20010625: replaced homegrown bubblesort in plot_histeps() by
 * call of standard routine qsort(). Need to tell the compare function
 * about the plotted dataset via this file scope variable: */
static CurvePoints * histeps_current_plot;

static int histeps_compare(const void * p1, const void * p2)
{
	double x1 = histeps_current_plot->points[*(int*)p1].x;
	double x2 = histeps_current_plot->points[*(int*)p2].x;
	return (x1 < x2) ? -1 : (x1 > x2);
}
//
// CAC  
// plot_histeps:
// Plot the curves in HISTEPS style
//
//static void plot_histeps(CurvePoints * plot)
void GpGadgets::PlotHiSteps(GpTermEntry * pT, CurvePoints * pPlot)
{
	int i;                  /* point index */
	int x1m, y1m, x2m, y2m; /* mapped coordinates */
	double x, y, xn, yn;    /* point position */
	double y_null;          /* y GpCoordinate of histogram baseline */
	int  * gl;              // array to hold list of valid points
	int    goodcount = 0;   // preliminary count of points inside array
	for(i = 0; i < pPlot->p_count; i++)
		if(pPlot->points[i].type == INRANGE || pPlot->points[i].type == OUTRANGE)
			++goodcount;
	if(goodcount >= 2) { /* cannot pPlot less than 2 points */
		gl = (int *)malloc(goodcount * sizeof(int));
		// fill gl array with indexes of valid (non-undefined) points
		goodcount = 0;
		for(i = 0; i < pPlot->p_count; i++)
			if(pPlot->points[i].type == INRANGE || pPlot->points[i].type == OUTRANGE) {
				gl[goodcount] = i;
				++goodcount;
			}
		// sort the data --- tell histeps_compare about the pPlot
		// datastructure to look at, then call qsort()
		histeps_current_plot = pPlot;
		qsort(gl, goodcount, sizeof(*gl), histeps_compare);
		// play it safe: invalidate the static pointer after usage
		histeps_current_plot = NULL;
		// HBB 20010625: log y axis must treat 0.0 as -infinity.
		// Define the correct y position for the histogram's baseline.
		y_null = (GetY().Flags & GpAxis::fLog) ? GetY().Range.GetActualLow() : 0.0;
		x = (3.0 * pPlot->points[gl[0]].x - pPlot->points[gl[1]].x) / 2.0;
		y = y_null;
		for(i = 0; i < goodcount - 1; i++) {    /* loop over all points except last  */
			yn = pPlot->points[gl[i]].y;
			if((GetY().Flags & GpAxis::fLog) && yn < y_null)
				yn = y_null;
			xn = (pPlot->points[gl[i]].x + pPlot->points[gl[i + 1]].x) / 2.0;
			x1m = MapX(x);
			x2m = MapX(xn);
			y1m = MapY(y);
			y2m = MapY(yn);
			DrawClipLine(pT, x1m, y1m, x1m, y2m);
			DrawClipLine(pT, x1m, y2m, x2m, y2m);
			x = xn;
			y = yn;
		}
		yn = pPlot->points[gl[i]].y;
		xn = (3.0 * pPlot->points[gl[i]].x - pPlot->points[gl[i - 1]].x) / 2.0;
		x1m = MapX(x);
		x2m = MapX(xn);
		y1m = MapY(y);
		y2m = MapY(yn);
		DrawClipLine(pT, x1m, y1m, x1m, y2m);
		DrawClipLine(pT, x1m, y2m, x2m, y2m);
		DrawClipLine(pT, x2m, y2m, x2m, MapY(y_null));
		free(gl);
	}
}
//
// Plot the curves in ERRORBARS style
// we just plot the bars; the points are plotted in plot_points
//
//static void plot_bars(CurvePoints * plot)
void GpGadgets::PlotBars(GpTermEntry *pT, CurvePoints * plot)
{
	int i;                  /* point index */
	double x, y;            /* position of the bar */
	double ylow, yhigh;     /* the ends of the bars */
	double xlow, xhigh;
	int xM, ylowM, yhighM;  /* the mapped version of above */
	int yM, xlowM, xhighM;
	int tic = MAX((pT->HTic/2), 1);
	double halfwidth = 0;   /* Used to calculate full box width */
	/* Only if pT has no filled_polygon! */
	if(oneof7(plot->plot_style, YERRORBARS, XYERRORBARS, BOXERROR, YERRORLINES, XYERRORLINES, HISTOGRAMS, FILLEDCURVES)) {
		/* Draw the vertical part of the bar */
		for(i = 0; i < plot->p_count; i++) {
			if(plot->points[i].type != UNDEFINED) { /* undefined points don't count */
				/* check to see if in xrange */
				x = plot->points[i].x;
				if(plot->plot_style == HISTOGRAMS) {
					/* Shrink each cluster to fit within one unit along X axis,   */
					/* centered about the integer representing the cluster number */
					/* 'start' is reset to 0 at the top of eval_plots(), and then */
					/* incremented if 'plot new histogram' is encountered.        */
					const int clustersize = plot->histogram->clustersize + histogram_opts.gap;
					x  += (i-1) * (clustersize - 1) + plot->histogram_sequence;
					x  += histogram_opts.gap/2;
					x  /= clustersize;
					x  += plot->histogram->start + 0.5;
					/* Calculate width also */
					halfwidth = (plot->points[i].xhigh - plot->points[i].xlow) / (2. * clustersize);
				}
				if(GetX().InRange(x)) {
					xM = MapX(x);
					// check to see if in yrange
					y = plot->points[i].y;
					if(GetY().InRange(y)) {
						yM = MapY(y);
						// find low and high points of bar, and check yrange
						yhigh = plot->points[i].yhigh;
						ylow = plot->points[i].ylow;
						yhighM = MapY(yhigh);
						ylowM = MapY(ylow);
						// This can happen if the y errorbar on a log-scaled Y goes negative
						if(plot->points[i].ylow == -GPVL)
							ylowM = MapY(GetY().Range.GetActualLow());
						// find low and high points of bar, and check xrange
						xhigh = plot->points[i].xhigh;
						xlow = plot->points[i].xlow;
						if(plot->plot_style == HISTOGRAMS) {
							xlowM = MapX(x-halfwidth);
							xhighM = MapX(x+halfwidth);
						}
						else {
							xhighM = MapX(xhigh);
							xlowM = MapX(xlow);
						}
						// Check for variable color - June 2010
						if((plot->plot_style != HISTOGRAMS) && (plot->plot_style != FILLEDCURVES)) {
							check_for_variable_color(plot, &plot->varcolor[i]);
						}
						// Error bars can now have a separate line style
						if((BarLp.flags & LP_ERRORBAR_SET) != 0)
							ApplyLpProperties(pT, &BarLp);
						// Error bars should be drawn in the border color for filled boxes
						// but only if there *is* a border color
						else if((plot->plot_style == BOXERROR) && pT->fillbox)
							NeedFillBorder(pT, &plot->fill_properties);
						/* By here everything has been mapped */
						/* First draw the main part of the error bar */
						DrawClipLine(pT, xM, ylowM, xM, yhighM);
						/* Even if error bars are dotted, the end lines are always solid */
						if((BarLp.flags & LP_ERRORBAR_SET) != 0)
							pT->dashtype(DASHTYPE_SOLID, NULL);
						if(!IsPolar) {
							if(BarSize < 0.0) {
								DrawClipLine(pT, xlowM, ylowM, xhighM, ylowM); /* draw the bottom tic same width as box */
								DrawClipLine(pT, xlowM, yhighM, xhighM, yhighM); /* draw the top tic same width as box */
							}
							else if(BarSize > 0.0) {
								/* draw the bottom tic */
								DrawClipLine(pT, (int)(xM - BarSize * tic), ylowM, (int)(xM + BarSize * tic), ylowM);
								/* draw the top tic */
								DrawClipLine(pT, (int)(xM - BarSize * tic), yhighM, (int)(xM + BarSize * tic), yhighM);
							}
						}
						else { /* Polar error bars */
							/* Draw the whiskers perpendicular to the main bar */
							if(BarSize > 0.0) {
								const double slope = atan2((double)(yhighM - ylowM), (double)(xhighM - xlowM));
								int x1 = (int)(xlowM - (BarSize * tic * sin(slope)));
								int x2 = (int)(xlowM + (BarSize * tic * sin(slope)));
								int y1 = (int)(ylowM + (BarSize * tic * cos(slope)));
								int y2 = (int)(ylowM - (BarSize * tic * cos(slope)));
								DrawClipLine(pT, x1, y1, x2, y2); /* draw the bottom tic */
								x1 += xhighM - xlowM;
								x2 += xhighM - xlowM;
								y1 += yhighM - ylowM;
								y2 += yhighM - ylowM;
								DrawClipLine(pT, x1, y1, x2, y2); /* draw the top tic */
							}
						}
					}
				}
			}
		}
	}
	if(oneof4(plot->plot_style, XERRORBARS, XYERRORBARS, XERRORLINES, XYERRORLINES)) {
		/* Draw the horizontal part of the bar */
		for(i = 0; i < plot->p_count; i++) {
			if(plot->points[i].type != UNDEFINED) { /* undefined points don't count */
				y = plot->points[i].y;
				if(GetY().InRange(y)) { /* check to see if in yrange */
					yM = MapY(y);
					// find low and high points of bar, and check xrange 
					xhigh = plot->points[i].xhigh;
					xlow = plot->points[i].xlow;
					xhighM = MapX(xhigh);
					xlowM = MapX(xlow);
					// This can happen if the x errorbar on a log-scaled X goes negative 
					if(plot->points[i].xlow == -GPVL)
						xlowM = MapX(GetX().Range.GetActualLow());
					// Check for variable color - June 2010 
					check_for_variable_color(plot, &plot->varcolor[i]);
					// Error bars can now have their own line style 
					if((BarLp.flags & LP_ERRORBAR_SET) != 0)
						ApplyLpProperties(pT, &BarLp);
					// by here everything has been mapped 
					DrawClipLine(pT, xlowM, yM, xhighM, yM);
					// Even if error bars are dotted, the end lines are always solid 
					if((BarLp.flags & LP_ERRORBAR_SET) != 0)
						pT->dashtype(DASHTYPE_SOLID, NULL);
					if(BarSize > 0.0) {
						DrawClipLine(pT, xlowM, (int)(yM - BarSize * tic), xlowM, (int)(yM + BarSize * tic));
						DrawClipLine(pT, xhighM, (int)(yM - BarSize * tic), xhighM, (int)(yM + BarSize * tic));
					}
				}
			}
		}
	}
}
//
// plot_boxes:
// EAM Sep 2002 - Consolidate BOXES and FILLEDBOXES
//
//static void plot_boxes(CurvePoints * pPlot, int xaxis_y)
void GpGadgets::PlotBoxes(GpTermEntry *pT, CurvePoints * pPlot, int xAxisY)
{
	int i;                  /* point index */
	int xl, xr, yb, yt;     /* point in terminal coordinates */
	double dxl, dxr, dyt;
	enum coord_type prev = UNDEFINED; /* type of previous point */
	int lastdef = 0;                /* most recent point that was not UNDEFINED */
	double dyb = 0.0;
	//
	// The stackheight[] array contains the y coord of the top 
	// of the stack so far for each point.
	if(pPlot->plot_style == HISTOGRAMS) {
		int newsize = pPlot->p_count;
		if(histogram_opts.type == HT_STACKED_IN_TOWERS)
			PrevRowStackCount = 0;
		if(histogram_opts.type == HT_STACKED_IN_LAYERS && pPlot->histogram_sequence == 0)
			PrevRowStackCount = 0;
		if(!P_PrevRowStackHeight) {
			P_PrevRowStackHeight = (GpCoordinate *)malloc(newsize * sizeof(GpCoordinate));
			for(i = 0; i < newsize; i++) {
				P_PrevRowStackHeight[i].yhigh = 0;
				P_PrevRowStackHeight[i].ylow = 0;
			}
			PrevRowStackCount = newsize;
		}
		else if(PrevRowStackCount < newsize) {
			P_PrevRowStackHeight = (GpCoordinate *)gp_realloc(P_PrevRowStackHeight, newsize * sizeof(GpCoordinate), "stackheight array");
			for(i = PrevRowStackCount; i < newsize; i++) {
				P_PrevRowStackHeight[i].yhigh = 0;
				P_PrevRowStackHeight[i].ylow = 0;
			}
			PrevRowStackCount = newsize;
		}
	}
	for(i = 0; i < pPlot->p_count; i++) {
		switch(pPlot->points[i].type) {
			case OUTRANGE:
			case INRANGE: {
			    if(pPlot->points[i].z < 0.0) {
				    /* need to auto-calc width */
				    if(boxwidth < 0)
					    dxl = (pPlot->points[lastdef].x - pPlot->points[i].x) / 2.0;
				    else if(!boxwidth_is_absolute)
					    dxl = (pPlot->points[lastdef].x - pPlot->points[i].x) * boxwidth / 2.0;
				    else
					    dxl = -boxwidth / 2.0;
				    if(i < pPlot->p_count - 1) {
					    int nextdef;
					    for(nextdef = i+1; nextdef < pPlot->p_count; nextdef++)
						    if(pPlot->points[nextdef].type != UNDEFINED)
							    break;
					    if(nextdef == pPlot->p_count) /* i is the last non-UNDEFINED point */
						    nextdef = i;
					    if(boxwidth < 0)
						    dxr = (pPlot->points[nextdef].x - pPlot->points[i].x) / 2.0;
					    else if(!boxwidth_is_absolute)
						    dxr = (pPlot->points[nextdef].x - pPlot->points[i].x) * boxwidth / 2.0;
					    else // Hits here on 3 column BOXERRORBARS 
						    dxr = boxwidth / 2.0;
					    if(pPlot->points[nextdef].type == UNDEFINED)
						    dxr = -dxl;
				    }
				    else {
					    dxr = -dxl;
				    }
				    if(prev == UNDEFINED && lastdef == 0)
					    dxl = -dxr;
				    dxl = pPlot->points[i].x + dxl;
				    dxr = pPlot->points[i].x + dxr;
			    }
			    else { /* z >= 0 */
				    dxr = pPlot->points[i].xhigh;
				    dxl = pPlot->points[i].xlow;
			    }
			    if(pPlot->plot_style == BOXXYERROR) {
				    dyb = pPlot->points[i].ylow;
				    dyb = GetY().Range.Clip(dyb);
				    xAxisY = MapY(dyb);
				    dyt = pPlot->points[i].yhigh;
			    }
			    else {
				    dyt = pPlot->points[i].y;
			    }
			    if(pPlot->plot_style == HISTOGRAMS) {
				    int ix = (int)pPlot->points[i].x;
				    int histogram_linetype = i;
				    lp_style_type ls;
				    int stack = i;
				    if(pPlot->histogram->startcolor > 0)
					    histogram_linetype += pPlot->histogram->startcolor;

				    /* Shrink each cluster to fit within one unit along X axis,   */
				    /* centered about the integer representing the cluster number */
				    /* 'start' is reset to 0 at the top of eval_plots(), and then */
				    /* incremented if 'pPlot new histogram' is encountered.        */
				    if(oneof2(histogram_opts.type, HT_CLUSTERED, HT_ERRORBARS)) {
					    int clustersize = pPlot->histogram->clustersize + histogram_opts.gap;
					    dxl  += (ix-1) * (clustersize - 1) + pPlot->histogram_sequence;
					    dxr  += (ix-1) * (clustersize - 1) + pPlot->histogram_sequence;
					    dxl  += histogram_opts.gap/2;
					    dxr  += histogram_opts.gap/2;
					    dxl  /= clustersize;
					    dxr  /= clustersize;
					    dxl  += pPlot->histogram->start + 0.5;
					    dxr  += pPlot->histogram->start + 0.5;
				    }
				    else if(histogram_opts.type == HT_STACKED_IN_TOWERS) {
					    dxl  = pPlot->histogram->start - boxwidth / 2.0;
					    dxr  = pPlot->histogram->start + boxwidth / 2.0;
					    dxl += pPlot->histogram_sequence;
					    dxr += pPlot->histogram_sequence;
				    }
				    else if(histogram_opts.type == HT_STACKED_IN_LAYERS) {
					    dxl += pPlot->histogram->start;
					    dxr += pPlot->histogram->start;
				    }

				    switch(histogram_opts.type) {
					    case HT_STACKED_IN_TOWERS: /* columnstacked */
						stack = 0;
						/* Line type (color) must match row number */
						if(prefer_line_styles)
							lp_use_properties(&ls, histogram_linetype);
						else
							load_linetype(&ls, histogram_linetype);
						ApplyPm3DColor(pT, &ls.pm3d_color);
						pPlot->fill_properties.fillpattern = histogram_linetype;
					    /* Fall through */
					    case HT_STACKED_IN_LAYERS: /* rowstacked */
						if(pPlot->points[i].y >= 0) {
							dyb = P_PrevRowStackHeight[stack].yhigh;
							dyt += P_PrevRowStackHeight[stack].yhigh;
							P_PrevRowStackHeight[stack].yhigh += pPlot->points[i].y;
						}
						else {
							dyb = P_PrevRowStackHeight[stack].ylow;
							dyt += P_PrevRowStackHeight[stack].ylow;
							P_PrevRowStackHeight[stack].ylow += pPlot->points[i].y;
						}
						if((GetY().Range.low < GetY().Range.upp && dyb < GetY().Range.low) || (GetY().Range.upp < GetY().Range.low && dyb > GetY().Range.low))
							dyb = GetY().Range.low;
						if((GetY().Range.low < GetY().Range.upp && dyb > GetY().Range.upp) || (GetY().Range.upp < GetY().Range.low && dyb < GetY().Range.upp))
							dyb = GetY().Range.upp;
						break;
					    case HT_CLUSTERED:
					    case HT_ERRORBARS:
						break;
				    }
			    }
			    // clip to border 
			    dyt = GetY().Range.Clip(dyt);
			    dxr = GetX().Range.Clip(dxr);
			    dxl = GetX().Range.Clip(dxl);
			    // Entire box is out of range on x 
			    if(dxr == dxl && (dxr == GetX().Range.low || dxr == GetX().Range.upp))
				    break;
			    xl = MapX(dxl);
			    xr = MapX(dxr);
			    yt = MapY(dyt);
			    yb = xAxisY;

			    /* Entire box is out of range on y */
			    if(yb == yt && (dyt == GetY().Range.low || dyt == GetY().Range.upp))
				    break;
			    if(pPlot->plot_style == HISTOGRAMS && oneof2(histogram_opts.type, HT_STACKED_IN_LAYERS, HT_STACKED_IN_TOWERS))
				    yb = MapY(dyb);
			    // Variable color 
			    if(oneof3(pPlot->plot_style, BOXES, BOXXYERROR, BOXERROR)) {
				    check_for_variable_color(pPlot, &pPlot->varcolor[i]);
			    }
			    if((pPlot->fill_properties.fillstyle != FS_EMPTY) && pT->fillbox) {
				    int    x = xl;
				    int    y = yb;
				    int    w = xr - xl + 1;
				    int    h = yt - yb + 1;
				    // avoid negative width/height
				    if(w <= 0) {
					    x = xr;
					    w = xl - xr + 1;
				    }
				    if(h <= 0) {
					    y = yt;
					    h = yb - yt + 1;
				    }
				    int    style = style_from_fill(&pPlot->fill_properties);
				    (*pT->fillbox)(style, x, y, w, h);
				    if(!NeedFillBorder(pT, &pPlot->fill_properties))
					    break;
			    }
				{
					newpath(pT);
					pT->_Move(xl, yb);
					pT->_Vector(xl, yt);
					pT->_Vector(xr, yt);
					pT->_Vector(xr, yb);
					pT->_Vector(xl, yb);
					closepath(pT);
				}
			    if(pT->fillbox && pPlot->fill_properties.border_color.type != TC_DEFAULT) {
				    ApplyLpProperties(pT, &pPlot->lp_properties);
			    }
			    break;
		    }           /* case OUTRANGE, INRANGE */
			default: /* just a safety */
			case UNDEFINED: {
			    break;
		    }
		}
		prev = pPlot->points[i].type;
		if(prev != UNDEFINED)
			lastdef = i;
	}
}
//
// plot_points:
// Plot the curves in POINTSTYLE style
//
//static void plot_points(CurvePoints * plot)
void GpGadgets::PlotPoints(GpTermEntry * pT, CurvePoints * pPlot)
{
	int i;
	int x, y;
	int p_width, p_height;
	int pointtype;
	int interval = pPlot->lp_properties.p_interval;
	// Set whatever we can that applies to every point in the loop 
	if(pPlot->lp_properties.p_type == PT_CHARACTER) {
		ignore_enhanced(true);
		if(pPlot->labels->font && pPlot->labels->font[0])
			(*pT->set_font)(pPlot->labels->font);
		(*pT->justify_text)(CENTRE);
	}
	if(ClipPoints) {
		p_width  = (int)(pT->HTic * pPlot->lp_properties.p_size);
		p_height = (int)(pT->VTic * pPlot->lp_properties.p_size);
	}
	// Displace overlapping points if "set jitter" is in effect	
	// This operation leaves x and y untouched, but loads the	
	// jitter offsets into xhigh and yhigh.			
	if(jitter.spread > 0)
		JitterPoints(pPlot);
	for(i = 0; i < pPlot->p_count; i++) {
		if((pPlot->plot_style == LINESPOINTS) && (interval) && (i % interval)) {
			continue;
		}
		if(pPlot->points[i].type == INRANGE) {
			x = MapX(pPlot->points[i].x);
			y = MapY(pPlot->points[i].y);
			// Apply jitter offsets.                                    
			// The jitter x offset is a multiple of character width.    
			// The jitter y offset is in the original GpCoordinate system.
			if(jitter.spread > 0) {
				x += (int)(pPlot->points[i].xhigh * 0.7 * pT->HChr);
				y = MapY(pPlot->points[i].y + pPlot->points[i].yhigh);
			}
			// do clipping if necessary 
			if(!ClipPoints || (x >= PlotBounds.xleft + p_width
				    && y >= PlotBounds.ybot + p_height && x <= PlotBounds.xright - p_width && y <= PlotBounds.ytop - p_height)) {
				if(oneof2(pPlot->plot_style, POINTSTYLE, LINESPOINTS) &&  pPlot->lp_properties.p_size == PTSZ_VARIABLE)
					(*pT->pointsize)(PtSz * pPlot->points[i].z);
				// Feb 2016: variable point type 
				if((pPlot->plot_style == POINTSTYLE || pPlot->plot_style == LINESPOINTS) &&  pPlot->lp_properties.p_type == PT_VARIABLE) {
					pointtype = (int)(pPlot->points[i].CRD_PTTYPE - 1);
				}
				else {
					pointtype = pPlot->lp_properties.p_type;
				}
				// A negative interval indicates we should try to blank out the 
				// area behind the point symbol. This could be done better by   
				// implementing a special point type, but that would require    
				// modification to all terminal drivers. It might be worth it.  
				// ApplyLpProperties will restore the point type and size
				if(pPlot->plot_style == LINESPOINTS && interval < 0) {
					(*pT->set_color)(&background_fill);
					(*pT->pointsize)(PtSz * PtIntervalBox);
					(*pT->point)(x, y, 6);
					ApplyLpProperties(pT, &(pPlot->lp_properties));
				}
				// rgb variable  -  color read from data column 
				check_for_variable_color(pPlot, &pPlot->varcolor[i]);
				// Print special character rather than drawn symbol 
				if(pointtype == PT_CHARACTER) {
					ApplyPm3DColor(pT, &(pPlot->labels->textcolor));
					pT->_PutText(x, y, pPlot->lp_properties.p_char);
				}
				// The normal case 
				else if(pointtype >= -1)
					(*pT->point)(x, y, pointtype);
			}
		}
	}
	// Return to initial state 
	if(pPlot->lp_properties.p_type == PT_CHARACTER) {
		if(pPlot->labels->font && pPlot->labels->font[0])
			(*pT->set_font)("");
		ignore_enhanced(false);
	}
}

#ifdef EAM_OBJECTS
/* plot_circles:
 * Plot the curves in CIRCLES style
 */
static void plot_circles(CurvePoints * plot)
{
	int i;
	int x, y;
	double radius, arc_begin, arc_end;
	fill_style_type * fillstyle = &plot->fill_properties;
	int style = style_from_fill(fillstyle);
	bool withborder = false;
	BoundingBox * clip_save = GpGg.P_Clip;
	if(GpGg.DefaultCircle.clip == OBJ_NOCLIP)
		GpGg.P_Clip = &GpGg.Canvas;
	if(fillstyle->border_color.type != TC_LT ||  fillstyle->border_color.lt != LT_NODRAW)
		withborder = true;
	for(i = 0; i < plot->p_count; i++) {
		if(plot->points[i].type == INRANGE) {
			x = GpGg.MapX(plot->points[i].x);
			y = GpGg.MapY(plot->points[i].y);
			radius = x - GpGg.MapX(plot->points[i].xlow);
			if(plot->points[i].z == DEFAULT_RADIUS)
				GpGg.MapPositionR(GpGg.DefaultCircle.o.circle.extent, &radius, NULL, "radius");
			arc_begin = plot->points[i].ylow;
			arc_end = plot->points[i].xhigh;
			// rgb variable  -  color read from data column 
			if(!check_for_variable_color(plot, &plot->varcolor[i]) && withborder)
				GpGg.ApplyLpProperties(term, &plot->lp_properties);
			do_arc(x, y, radius, arc_begin, arc_end, style, false);
			if(withborder) {
				GpGg.NeedFillBorder(term, &plot->fill_properties);
				do_arc(x, y, radius, arc_begin, arc_end, 0, GpGg.DefaultCircle.o.circle.wedge);
			}
		}
	}
	GpGg.P_Clip = clip_save;
}
//
// Plot the curves in ELLIPSES style
//
//static void plot_ellipses(CurvePoints * plot)
void GpGadgets::PlotEllipses(GpTermEntry * pT, CurvePoints * plot)
{
	int i;
	t_ellipse * e = (t_ellipse*)malloc(sizeof(t_ellipse));
	double tempx, tempy, tempfoo;
	fill_style_type * fillstyle = &plot->fill_properties;
	int style = style_from_fill(fillstyle);
	bool withborder = false;
	BoundingBox * clip_save = P_Clip;
	if(DefaultEllipse.clip == OBJ_NOCLIP)
		P_Clip = &Canvas;
	if(fillstyle->border_color.type != TC_LT ||  fillstyle->border_color.lt != LT_NODRAW)
		withborder = true;
	e->extent.scalex = (plot->x_axis == SECOND_X_AXIS) ? second_axes : first_axes;
	e->extent.scaley = (plot->y_axis == SECOND_Y_AXIS) ? second_axes : first_axes;
	e->type = plot->ellipseaxes_units;
	for(i = 0; i < plot->p_count; i++) {
		if(plot->points[i].type == INRANGE) {
			e->center.x = MapX(plot->points[i].x);
			e->center.y = MapY(plot->points[i].y);
			e->extent.x = plot->points[i].xlow; /* major axis */
			e->extent.y = plot->points[i].xhigh; /* minor axis */
			/* the mapping can be set by the
			 * "set ellipseaxes" setting
			 * both x units, mixed, both y units */
			/* clumsy solution */
			switch(e->type) {
				case ELLIPSEAXES_XY:
				    MapPositionR(e->extent, &tempx, &tempy, "ellipse");
				    e->extent.x = tempx;
				    e->extent.y = tempy;
				    break;
				case ELLIPSEAXES_XX:
				    MapPositionR(e->extent, &tempx, &tempy, "ellipse");
				    tempfoo = tempx;
				    e->extent.x = e->extent.y;
				    MapPositionR(e->extent, &tempy, &tempx, "ellipse");
				    e->extent.x = tempfoo;
				    e->extent.y = tempy;
				    break;
				case ELLIPSEAXES_YY:
				    MapPositionR(e->extent, &tempx, &tempy, "ellipse");
				    tempfoo = tempy;
				    e->extent.y = e->extent.x;
				    MapPositionR(e->extent, &tempy, &tempx, "ellipse");
				    e->extent.x = tempx;
				    e->extent.y = tempfoo;
				    break;
			}
			if(plot->points[i].z <= DEFAULT_RADIUS) {
				/*memcpy(&(e->extent), &default_ellipse.o.ellipse.extent, sizeof(GpPosition));*/
				/*e->extent.x = default_ellipse.o.ellipse.extent.x;
				   e->extent.y = default_ellipse.o.ellipse.extent.y;*/
				MapPositionR(DefaultEllipse.o.ellipse.extent, &e->extent.x, &e->extent.y, "ellipse");
			}
			e->orientation = (plot->points[i].z == DEFAULT_ELLIPSE) ? DefaultEllipse.o.ellipse.orientation : plot->points[i].ylow;
			// rgb variable  -  color read from data column
			if(!check_for_variable_color(plot, &plot->varcolor[i]) && withborder)
				ApplyLpProperties(pT, &plot->lp_properties);
			DoEllipse(pT, 2, e, style, false);
			if(withborder) {
				NeedFillBorder(pT, &plot->fill_properties);
				DoEllipse(pT, 2, e, 0, false);
			}
		}
	}
	free(e);
	P_Clip = clip_save;
}

#endif

/* plot_dots:
 * Plot the curves in DOTS style
 */
//static void plot_dots(CurvePoints * plot)
void GpGraphics::PlotDots(GpTermEntry * pT, CurvePoints * plot)
{
	for(int i = 0; i < plot->p_count; i++) {
		if(plot->points[i].type == INRANGE) {
			int x = GpGg.MapX(plot->points[i].x);
			int y = GpGg.MapY(plot->points[i].y);
			// rgb variable  -  color read from data column
			check_for_variable_color(plot, &plot->varcolor[i]);
			// point type -1 is a dot
			pT->point(x, y, -1);
		}
	}
}
//
// Plot the curves in VECTORS style
//
//static void plot_vectors(CurvePoints * plot)
void GpGadgets::PlotVectors(GpTermEntry * pT, CurvePoints * pPlot)
{
	int x1, y1, x2, y2;
	GpCoordinate points[2];
	double ex, ey;
	double lx[2], ly[2];
	// Normally this is only necessary once because all arrows equal 
	arrow_style_type ap = pPlot->arrow_properties;
	ApplyLpProperties(pT, &ap.lp_properties);
	ApplyHeadProperties(ap);
	for(int i = 0; i < pPlot->p_count; i++) {
		points[0] = pPlot->points[i];
		if(points[0].type != UNDEFINED) {
			points[1].x = pPlot->points[i].xhigh;
			points[1].y = pPlot->points[i].yhigh;
			// variable arrow style read from extra data column 
			if(pPlot->arrow_properties.tag == AS_VARIABLE) {
				int as = (int)pPlot->points[i].z;
				arrow_use_properties(&ap, as);
				ApplyLpProperties(pT, &ap.lp_properties);
				ApplyHeadProperties(ap);
			}
			// variable color read from extra data column
			check_for_variable_color(pPlot, &pPlot->varcolor[i]);
			if(GetX().InRange(points[1].x) && GetY().InRange(points[1].y)) {
				// to inrange
				points[1].type = INRANGE;
				x2 = MapX(points[1].x);
				y2 = MapY(points[1].y);
				if(points[0].type == INRANGE) {
					x1 = MapX(points[0].x);
					y1 = MapY(points[0].y);
					(*pT->arrow)(x1, y1, x2, y2, ap.head);
				}
				else if(points[0].type == OUTRANGE) {
					// from outrange to inrange 
					if(ClipLines1) {
						EdgeIntersect(points, 1, &ex, &ey);
						x1 = MapX(ex);
						y1 = MapY(ey);
						if(ap.head & END_HEAD)
							(*pT->arrow)(x1, y1, x2, y2, END_HEAD);
						else
							(*pT->arrow)(x1, y1, x2, y2, NOHEAD);
					}
				}
			}
			else {
				// to outrange 
				points[1].type = OUTRANGE;
				if(points[0].type == INRANGE) {
					// from inrange to outrange 
					if(ClipLines1) {
						x1 = MapX(points[0].x);
						y1 = MapY(points[0].y);
						EdgeIntersect(points, 1, &ex, &ey);
						x2 = MapX(ex);
						y2 = MapY(ey);
						if(ap.head & BACKHEAD)
							(*pT->arrow)(x2, y2, x1, y1, BACKHEAD);
						else
							(*pT->arrow)(x1, y1, x2, y2, NOHEAD);
					}
				}
				else if(points[0].type == OUTRANGE) {
					// from outrange to outrange 
					if(ClipLines2) {
						if(TwoEdgeIntersect(points, 1, lx, ly)) {
							x1 = MapX(lx[0]);
							y1 = MapY(ly[0]);
							x2 = MapX(lx[1]);
							y2 = MapY(ly[1]);
							(*pT->arrow)(x1, y1, x2, y2, NOHEAD);
						}
					}
				}
			}
		}
	}
}
//
// Plot the curves in FINANCEBARS style
// EAM Feg 2010	- This routine is also used for BOXPLOT, which
//    loads a median value into xhigh
//
//static void plot_f_bars(CurvePoints * plot)
void GpGadgets::PlotFBars(GpTermEntry * pT, CurvePoints * pPlot)
{
	double x;               /* position of the bar */
	double ylow, yhigh, yclose, yopen; /* the ends of the bars */
	uint xM, ylowM, yhighM; /* the mapped version of above */
	bool low_inrange, high_inrange;
	int tic = MAX(MAX((pT->HTic/2), 1)/2, 1);
	for(int i = 0; i < pPlot->p_count; i++) {
		/* undefined points don'pT count */
		if(pPlot->points[i].type != UNDEFINED) {
			/* check to see if in xrange */
			x = pPlot->points[i].x;
			if(GetX().InRange(x)) {
				xM = MapX(x);
				/* find low and high points of bar, and check yrange */
				yhigh = pPlot->points[i].yhigh;
				ylow = pPlot->points[i].ylow;
				yclose = pPlot->points[i].z;
				yopen = pPlot->points[i].y;
				high_inrange = GetY().InRange(yhigh);
				low_inrange  = GetY().InRange(ylow);
				/* compute the pPlot position of yhigh */
				if(high_inrange)
					yhighM = MapY(yhigh);
				else if(samesign(yhigh - GetY().Range.upp, GetY().GetRange()))
					yhighM = MapY(GetY().Range.upp);
				else
					yhighM = MapY(GetY().Range.low);
				/* compute the pPlot position of ylow */
				if(low_inrange)
					ylowM = MapY(ylow);
				else if(samesign(ylow - GetY().Range.upp, GetY().GetRange()))
					ylowM = MapY(GetY().Range.upp);
				else
					ylowM = MapY(GetY().Range.low);
				if(high_inrange || low_inrange || ylowM != yhighM) { /* both out of range on the same side */
					/* variable color read from extra data column. June 2010 */
					check_for_variable_color(pPlot, &pPlot->varcolor[i]);
					/* by here everything has been mapped */
					pT->_Move(xM, ylowM);
					pT->_Vector(xM, yhighM); /* draw the main bar */
					/* draw the open tic */
					pT->_Move((uint)(xM - BarSize * tic), MapY(yopen));
					pT->_Vector(xM, MapY(yopen));
					/* draw the close tic */
					pT->_Move((uint)(xM + BarSize * tic), MapY(yclose));
					pT->_Vector(xM, MapY(yclose));
					/* Draw a bar at the median (stored in xhigh) */
					if(pPlot->plot_style == BOXPLOT) {
						uint ymedian = MapY(pPlot->points[i].xhigh);
						pT->_Move((uint)(xM - BarSize * tic), ymedian);
						pT->_Vector((uint)(xM + BarSize * tic), ymedian);
					}
				}
			}
		}
	}
}

/* plot_c_bars:
 * Plot the curves in CANDLESTICKS style
 * EAM Apr 2008 - switch to using empty/fill rather than empty/striped
 *		  to distinguish whether (open > close)
 * EAM Dec 2009	- allow an optional 6th column to specify width
 *		  This routine is also used for BOXPLOT, which
 *		  loads a median value into xhigh
 */
//static void plot_c_bars(CurvePoints * plot)
void GpGadgets::PlotCBars(GpTermEntry * pT, CurvePoints * pPlot)
{
	double dxl, dxr, ylow, yhigh, yclose, yopen;    /* the ends of the bars */
	int xlowM, xhighM, xM, ylowM, yhighM;           /* mapped version of above */
	int ymin, ymax;                                 /* clipped to pPlot extent */
	enum coord_type prev = UNDEFINED;               /* type of previous point */
	bool low_inrange, high_inrange;
	bool open_inrange, close_inrange;
	int tic = MAX(MAX((pT->HTic/2), 1)/2, 1);
	for(int i = 0; i < pPlot->p_count; i++) {
		bool skip_box = false;
		// undefined points don'pT count
		if(pPlot->points[i].type != UNDEFINED) {
			// check to see if in xrange
			const double x = pPlot->points[i].x; // position of the bar
			if(GetX().InRange(x)) {
				xM = MapX(x);
				// find low and high points of bar, and check yrange
				yhigh  = pPlot->points[i].yhigh;
				ylow   = pPlot->points[i].ylow;
				yclose = pPlot->points[i].z;
				yopen  = pPlot->points[i].y;
				// HBB 20010928: To make code match the documentation, ensure
				// yhigh is actually higher than ylow
				if(yhigh < ylow) {
					Exchange(&ylow, &yhigh);
				}
				high_inrange = GetY().InRange(yhigh);
				low_inrange  = GetY().InRange(ylow);
				// compute the pPlot position of yhigh
				if(high_inrange)
					yhighM = MapY(yhigh);
				else if(samesign(yhigh - GetY().Range.upp, GetY().GetRange()))
					yhighM = MapY(GetY().Range.upp);
				else
					yhighM = MapY(GetY().Range.low);
				// compute the pPlot position of ylow
				if(low_inrange)
					ylowM = MapY(ylow);
				else if(samesign(ylow - GetY().Range.upp, GetY().GetRange()))
					ylowM = MapY(GetY().Range.upp);
				else
					ylowM = MapY(GetY().Range.low);
				if(!high_inrange && !low_inrange && ylowM == yhighM)
					continue; // both out of range on the same side
				if(pPlot->points[i].xlow != pPlot->points[i].x) {
					dxl = pPlot->points[i].xlow;
					dxr = 2 * x - dxl;
					dxr = GetX().Range.Clip(dxr);
					dxl = GetX().Range.Clip(dxl);
					xlowM = MapX(dxl);
					xhighM = MapX(dxr);
				}
				else if(pPlot->plot_style == BOXPLOT) {
					dxr = (boxwidth_is_absolute && boxwidth > 0) ? boxwidth/2.0 : 0.25;
					xlowM = MapX(x-dxr);
					xhighM = MapX(x+dxr);
				}
				else if(boxwidth < 0.0) {
					xlowM = (int)(xM - BarSize * tic);
					xhighM = (int)(xM + BarSize * tic);
				}
				else {
					dxl = -boxwidth / 2.0;
					if(prev != UNDEFINED)
						if(!boxwidth_is_absolute)
							dxl = (pPlot->points[i-1].x - pPlot->points[i].x) * boxwidth / 2.0;
					dxr = -dxl;
					if(i < pPlot->p_count - 1) {
						if(pPlot->points[i + 1].type != UNDEFINED) {
							dxr = boxwidth_is_absolute ? (boxwidth / 2.0) : (pPlot->points[i+1].x - pPlot->points[i].x) * boxwidth / 2.0;
						}
					}
					if(prev == UNDEFINED)
						dxl = -dxr;
					dxl = x + dxl;
					dxr = x + dxr;
					dxr = GetX().Range.Clip(dxr);
					dxl = GetX().Range.Clip(dxl);
					xlowM = MapX(dxl);
					xhighM = MapX(dxr);
				}
				// EAM Feb 2007 Force width to be an odd number of pixels
				// so that the center bar can be centered perfectly
				if(((xhighM-xlowM) & 01) != 0) {
					xhighM++;
					if(xM-xlowM > xhighM-xM)
						xM--;
					if(xM-xlowM < xhighM-xM)
						xM++;
				}
				// EAM Feb 2006 Clip to pPlot vertical extent
				open_inrange  = GetY().InRange(yopen);
				close_inrange = GetY().InRange(yclose);
				yopen  = GetY().Range.Clip(yopen);
				yclose = GetY().Range.Clip(yclose);
				if(MapY(yopen) < MapY(yclose)) {
					ymin = MapY(yopen);
					ymax = MapY(yclose);
				}
				else {
					ymax = MapY(yopen);
					ymin = MapY(yclose);
				}
				if(!open_inrange && !close_inrange && ymin == ymax)
					skip_box = true;
				// Reset to original color, if we changed it for the border
				if(pPlot->fill_properties.border_color.type != TC_DEFAULT && !( pPlot->fill_properties.border_color.type == TC_LT && pPlot->fill_properties.border_color.lt == LT_NODRAW)) {
					ApplyLpProperties(pT, &pPlot->lp_properties);
				}
				// Reset also if we changed it for the errorbars
				else if((BarLp.flags & LP_ERRORBAR_SET) != 0) {
					ApplyLpProperties(pT, &pPlot->lp_properties);
				}
				// variable color read from extra data column. June 2010
				check_for_variable_color(pPlot, &pPlot->varcolor[i]);
				/* Boxes are always filled if an explicit non-empty fillstyle is set. */
				/* If the fillstyle is FS_EMPTY, fill to indicate (open > close).     */
				if(pT->fillbox && !skip_box) {
					int style = style_from_fill(&pPlot->fill_properties);
					if((style != FS_EMPTY) || (yopen > yclose)) {
						uint x = xlowM;
						uint y = ymin;
						uint w = (xhighM-xlowM);
						uint h = (ymax-ymin);
						if(style == FS_EMPTY && pPlot->plot_style != BOXPLOT)
							style = FS_OPAQUE;
						(*pT->fillbox)(style, x, y, w, h);
						if(style_from_fill(&pPlot->fill_properties) != FS_EMPTY)
							NeedFillBorder(pT, &pPlot->fill_properties);
					}
				}
				//
				// Draw open box
				//
				if(!skip_box) {
					newpath(pT);
					pT->_Move(xlowM, MapY(yopen));
					pT->_Vector(xhighM, MapY(yopen));
					pT->_Vector(xhighM, MapY(yclose));
					pT->_Vector(xlowM, MapY(yclose));
					pT->_Vector(xlowM, MapY(yopen));
					closepath(pT);
				}
				// BOXPLOT wants a median line also, which is stored in xhigh
				if(pPlot->plot_style == BOXPLOT) {
					int ymedianM = MapY(pPlot->points[i].xhigh);
					pT->_Move(xlowM,  ymedianM);
					pT->_Vector(xhighM, ymedianM);
				}
				/* Through 4.2 gnuplot would indicate (open > close) by drawing     */
				/* three vertical bars.  Now we use solid fill.  But if the current */
				/* terminal does not support filled boxes, fall back to the old way */
				if((yopen > yclose) && !(pT->fillbox)) {
					pT->_Move(xM, ymin);
					pT->_Vector(xM, ymax);
					pT->_Move( (xM + xlowM) / 2, ymin);
					pT->_Vector( (xM + xlowM) / 2, ymax);
					pT->_Move( (xM + xhighM) / 2, ymin);
					pT->_Vector( (xM + xhighM) / 2, ymax);
				}
				// Error bars can now have their own line style
				if((BarLp.flags & LP_ERRORBAR_SET) != 0) {
					ApplyLpProperties(pT, &BarLp);
				}
				// Draw whiskers
				pT->_Move(xM, ylowM);
				pT->_Vector(xM, ymin);
				pT->_Move(xM, ymax);
				pT->_Vector(xM, yhighM);
				// Some users prefer bars at the end of the whiskers
				if(pPlot->plot_style == BOXPLOT ||  pPlot->arrow_properties.head == BOTH_HEADS) {
					uint d;
					if(pPlot->plot_style == BOXPLOT) {
						d = (BarSize < 0) ? 0 : (uint)((xhighM-xlowM)/2. - (BarSize * pT->HTic));
					}
					else {
						double frac = pPlot->arrow_properties.head_length;
						d = (frac <= 0) ? 0 : (uint)((xhighM-xlowM)*(1.-frac)/2.0);
					}
					if(high_inrange) {
						pT->_Move(xlowM+d, yhighM);
						pT->_Vector(xhighM-d, yhighM);
					}
					if(low_inrange) {
						pT->_Move(xlowM+d, ylowM);
						pT->_Vector(xhighM-d, ylowM);
					}
				}
				prev = pPlot->points[i].type;
			}
		}
	}
}

static void plot_parallel(CurvePoints * plot)
{
	int x0, y0, x1, y1;
	for(int i = 0; i < plot->p_count; i++) {
		GpAxis * this_axis = &GpGg.P_ParallelAxis[0];
		// rgb variable  -  color read from data column
		check_for_variable_color(plot, &plot->varcolor[i]);
		x0 = GpGg.MapX(1.0);
		y0 = this_axis->Map(plot->z_n[0][i]);
		for(int j = 1; j < plot->n_par_axes; j++) {
			this_axis = &GpGg.P_ParallelAxis[j];
			x1 = GpGg.MapX((double)(j+1));
			y1 = this_axis->Map(plot->z_n[j][i]);
			GpGg.DrawClipLine(term, x0, y0, x1, y1);
			x0 = x1;
			y0 = y1;
		}
	}
}
//
// Plot the curves in BOXPLOT style
// helper functions: compare_ypoints, filter_boxplot
//
static int compare_ypoints(const void * arg1, const void * arg2)
{
	GpCoordinate const * p1 = (GpCoordinate const *)arg1;
	GpCoordinate const * p2 = (GpCoordinate const *)arg2;
	if(GpGg.BoxPlotFactorSortRequired) {
		// Primary sort key is the "factor" 
		if(p1->z > p2->z)
			return (1);
		if(p1->z < p2->z)
			return (-1);
	}
	if(p1->y > p2->y)
		return (1);
	if(p1->y < p2->y)
		return (-1);
	return (0);
}

int filter_boxplot(CurvePoints * plot)
{
	int N = plot->p_count;
	int i;
	// Force any undefined points to the end of the list by y value 
	for(i = 0; i<N; i++)
		if(plot->points[i].type == UNDEFINED)
			plot->points[i].y = plot->points[i].z = GPVL;

	// Sort the points to find median and quartiles 
	if(plot->boxplot_factors > 1)
		GpGg.BoxPlotFactorSortRequired = true;
	qsort(plot->points, N, sizeof(GpCoordinate), compare_ypoints);
	//
	// Return a count of well-defined points with this index 
	// FIXME: This could be moved into plot_boxplot() 
	while(plot->points[N-1].type == UNDEFINED)
		N--;
	return N;
}

//static void plot_boxplot(CurvePoints * plot)
void GpGadgets::PlotBoxPlot(GpTermEntry * pT, CurvePoints * pPlot)
{
	int N;
	GpCoordinate * save_points = pPlot->points;
	int saved_p_count = pPlot->p_count;
	GpCoordinate * subset_points;
	int subset_count, true_count;
	GpTextLabel * subset_label = pPlot->labels;
	GpCoordinate candle;
	double median, quartile1, quartile3;
	double whisker_top, whisker_bot;
	const  int    levels = NZOR(pPlot->boxplot_factors, 1);
	//
	// The entire collection of points was already sorted in filter_boxplot()
	// called from boxplot_range_fiddling().  That sort used the category
	// (a.k.a. "factor" a.k.a. "level") as a primary key and the y value as
	// a secondary key.  That is sufficient for describing all points in a
	// single boxplot, but if we want a separate boxplot for each category
	// then additional bookkeeping is required.
	// 
	for(int level = 0; level < levels; level++) {
		if(levels == 1) {
			subset_points = save_points;
			subset_count = saved_p_count;
		}
		else {
			subset_label = subset_label->next;
			true_count = 0;
			/* advance to first point in subset */
			for(subset_points = save_points;
			    subset_points->z != subset_label->tag;
			    subset_points++, true_count++) {
				/* No points found for this boxplot factor */
				if(true_count >= saved_p_count)
					break;
			}
			// count well-defined points in this subset
			for(subset_count = 0; true_count < saved_p_count && subset_points[subset_count].z == subset_label->tag; subset_count++, true_count++) {
				if(subset_points[subset_count].type == UNDEFINED)
					break;
			}
		}
		/* Not enough points left to make a boxplot */
		N = subset_count;
		if(N < 4) {
			candle.x = subset_points->x + boxplot_opts.separation * level;
			candle.yhigh = -GPVL;
			candle.ylow = GPVL;
			goto outliers;
		}
		median    = ((N & 0x1) == 0) ? (0.5 * (subset_points[N/2 - 1].y + subset_points[N/2].y)) : subset_points[(N-1)/2].y;
		quartile1 = ((N & 0x3) == 0) ? (0.5 * (subset_points[N/4 - 1].y + subset_points[N/4].y)) : subset_points[(N+3)/4 - 1].y;
		quartile3 = ((N & 0x3) == 0) ? (0.5 * (subset_points[N - N/4].y + subset_points[N - N/4 - 1].y)) : subset_points[N - (N+3)/4].y;
		FPRINTF((stderr, "Boxplot: quartile boundaries for %d points: %g %g %g\n", N, quartile1, median, quartile3));
		// Set the whisker limits based on the user-defined style
		if(boxplot_opts.limit_type == 0) {
			// Fraction of interquartile range
			double whisker_len = boxplot_opts.limit_value * (quartile3 - quartile1);
			int i;
			whisker_bot = quartile1 - whisker_len;
			for(i = 0; i<N; i++)
				if(subset_points[i].y >= whisker_bot) {
					whisker_bot = subset_points[i].y;
					break;
				}
			whisker_top = quartile3 + whisker_len;
			for(i = N-1; i>= 0; i--)
				if(subset_points[i].y <= whisker_top) {
					whisker_top = subset_points[i].y;
					break;
				}
		}
		else {
			// Set limits to include some fraction of the total number of points. 
			// The limits are symmetric about the median, but are truncated to    
			// lie on a point in the data set.                                    
			int top = N-1;
			int bot = 0;
			while((double)(top-bot+1)/(double)(N) >= boxplot_opts.limit_value) {
				whisker_top = subset_points[top].y;
				whisker_bot = subset_points[bot].y;
				if(whisker_top - median >= median - whisker_bot) {
					top--;
					while((top > 0) && (subset_points[top].y == subset_points[top-1].y))
						top--;
				}
				if(whisker_top - median <= median - whisker_bot) {
					bot++;
					while((bot < top) && (subset_points[bot].y == subset_points[bot+1].y))
						bot++;
				}
			}
		}
		// Dummy up a single-point candlesticks pPlot using these limiting values
		candle.type = INRANGE;
		candle.x = (pPlot->plot_type == FUNC) ? ((subset_points[0].x + subset_points[N-1].x) / 2.0) : (subset_points->x + boxplot_opts.separation * level);
		candle.y = quartile1;
		candle.z = quartile3;
		candle.ylow  = whisker_bot;
		candle.yhigh = whisker_top;
		candle.xlow  = subset_points->xlow + boxplot_opts.separation * level;
		candle.xhigh = median; // Crazy order of candlestick parameters! 
		pPlot->points = &candle;
		pPlot->p_count = 1;
		// for boxplots "lc variable" means color by factor index 
		if(pPlot->varcolor)
			pPlot->varcolor[0] = pPlot->base_linetype + level + 1;
		if(boxplot_opts.plotstyle == FINANCEBARS)
			PlotFBars(pT, pPlot);
		else
			PlotCBars(pT, pPlot);
		// Now draw individual points for the outliers
outliers:
		if(boxplot_opts.outliers) {
			const int p_width  = (int)(pT->HTic * pPlot->lp_properties.p_size);
			const int p_height = (int)(pT->VTic * pPlot->lp_properties.p_size);
			for(int i = 0; i < subset_count; i++) {
				if(subset_points[i].y < candle.ylow || subset_points[i].y > candle.yhigh) {
					if(subset_points[i].type != UNDEFINED) {
						int    x = MapX(candle.x);
						int    y = MapY(subset_points[i].y);
						// previous INRANGE/OUTRANGE no longer valid
						if(x >= (PlotBounds.xleft + p_width) && y >= (PlotBounds.ybot + p_height) && x <= (PlotBounds.xright - p_width) && y <= (PlotBounds.ytop - p_height)) {
							// Separate any duplicate outliers
							for(int j = 1; (i >= j) && (subset_points[i].y == subset_points[i-j].y); j++)
								x += p_width * ((j & 1) == 0 ? -j : j); ;
							(pT->point)(x, y, pPlot->lp_properties.p_type);
						}
					}
				}
			}
		}
		// Restore original dataset points and size
		pPlot->points = save_points;
		pPlot->p_count = saved_p_count;
	}
}

/* FIXME
 * there are LOADS of == style double comparisons in here!
 */
/* single edge intersection algorithm */
/* Given two points, one inside and one outside the plot, return
 * the point where an edge of the plot intersects the line segment defined
 * by the two points.
 */
//static int edge_intersect(GpCoordinate * points, int i, double * ex, double * ey)
int GpGadgets::EdgeIntersect(GpCoordinate * pPoints, int i, double * pEx, double * pEy)
{
	double ix = pPoints[i - 1].x;
	double iy = pPoints[i - 1].y;
	double ox = pPoints[i].x;
	double oy = pPoints[i].y;
	double x, y;            /* possible intersection point */
	if(pPoints[i].type == INRANGE) {
		// swap pPoints around so that ix/ix/iz are INRANGE and ox/oy/oz are OUTRANGE
		x = ix;
		ix = ox;
		ox = x;
		y = iy;
		iy = oy;
		oy = y;
	}
	/* nasty degenerate cases, effectively drawing to an infinity point (?)
	 * cope with them here, so don't process them as a "real" OUTRANGE point
	 *
	 * If more than one coord is -GPVL, then can't ratio the "infinities"
	 * so drop out by returning the INRANGE point.
	 *
	 * Obviously, only need to test the OUTRANGE point (coordinates) */
	if(ox == -GPVL || oy == -GPVL) {
		*pEx = ix;
		*pEy = iy;
		if(ox == -GPVL) {
			// can't get a direction to draw line, so simply return INRANGE point
			if(oy == -GPVL)
				return LEFT_EDGE|BOTTOM_EDGE;
			*pEx = GetX().Range.low;
			return LEFT_EDGE;
		}
		// obviously oy is -GPVL and ox != -GPVL
		*pEy = GetY().Range.low;
		return BOTTOM_EDGE;
	}
	//
	// Can't have case (ix == ox && iy == oy) as one point
	// is INRANGE and one point is OUTRANGE.
	//
	if(iy == oy) {
		// horizontal line
		// assume inrange(iy, GetY().min, GetY().max)
		*pEy = iy;       /* == oy */
		if(inrange(GetX().Range.upp, ix, ox) && GetX().Range.upp != ix) {
			*pEx = GetX().Range.upp;
			return RIGHT_EDGE;
		}
		else if(inrange(GetX().Range.low, ix, ox) && GetX().Range.low != ix) {
			*pEx = GetX().Range.low;
			return LEFT_EDGE;
		}
	}
	else if(ix == ox) {
		/* vertical line */
		/* assume inrange(ix, GetX().min, GetX().max) */
		*pEx = ix;       /* == ox */
		if(inrange(GetY().Range.upp, iy, oy) && GetY().Range.upp != iy) {
			*pEy = GetY().Range.upp;
			return TOP_EDGE;
		}
		else if(inrange(GetY().Range.low, iy, oy) && GetY().Range.low != iy) {
			*pEy = GetY().Range.low;
			return BOTTOM_EDGE;
		}
	}
	else {
		/* slanted line of some kind */

		/* does it intersect GetY().min edge */
		if(inrange(GetY().Range.low, iy, oy) && GetY().Range.low != iy && GetY().Range.low != oy) {
			x = ix + (GetY().Range.low - iy) * ((ox - ix) / (oy - iy));
			if(GetX().InRange(x)) {
				*pEx = x;
				*pEy = GetY().Range.low;
				return BOTTOM_EDGE; /* yes */
			}
		}
		/* does it intersect GetY().max edge */
		if(inrange(GetY().Range.upp, iy, oy) && GetY().Range.upp != iy && GetY().Range.upp != oy) {
			x = ix + (GetY().Range.upp - iy) * ((ox - ix) / (oy - iy));
			if(GetX().InRange(x)) {
				*pEx = x;
				*pEy = GetY().Range.upp;
				return TOP_EDGE; /* yes */
			}
		}
		// does it intersect GetX().min edge
		if(inrange(GetX().Range.low, ix, ox) && GetX().Range.low != ix && GetX().Range.low != ox) {
			y = iy + (GetX().Range.low - ix) * ((oy - iy) / (ox - ix));
			if(GetY().InRange(y)) {
				*pEx = GetX().Range.low;
				*pEy = y;
				return LEFT_EDGE;
			}
		}
		// does it intersect GetX().max edge
		if(inrange(GetX().Range.upp, ix, ox) && GetX().Range.upp != ix && GetX().Range.upp != ox) {
			y = iy + (GetX().Range.upp - ix) * ((oy - iy) / (ox - ix));
			if(GetY().InRange(y)) {
				*pEx = GetX().Range.upp;
				*pEy = y;
				return RIGHT_EDGE;
			}
		}
	}

	/* If we reach here, the inrange point is on the edge, and
	 * the line segment from the outrange point does not cross any
	 * other edges to get there. In this case, we return the inrange
	 * point as the 'edge' intersection point. This will basically draw
	 * line.
	 */
	*pEx = ix;
	*pEy = iy;
	return 0;
}

/* double edge intersection algorithm */
/* Given two points, both outside the plot, return
 * the points where an edge of the plot intersects the line segment defined
 * by the two points. There may be zero, one, two, or an infinite number
 * of intersection points. (One means an intersection at a corner, infinite
 * means overlaying the edge itself). We return false when there is nothing
 * to draw (zero intersections), and true when there is something to
 * draw (the one-point case is a degenerate of the two-point case and we do
 * not distinguish it - we draw it anyway).
 */
/* any intersection? */
//
// points - the points array 
// i - line segment from point i-1 to point i
// lx, ly /* lx[2], ly[2]*/ - points where it crosses edges
//
//static bool two_edge_intersect(GpCoordinate * points, int i, double * lx, double * ly)
bool GpGadgets::TwoEdgeIntersect(GpCoordinate * pPoints, int i, double * pLx, double * pLy)
{
	/* global GetX().min, GetX().max, GetY().min, GetX().max */
	int count;
	double ix = pPoints[i - 1].x;
	double iy = pPoints[i - 1].y;
	double ox = pPoints[i].x;
	double oy = pPoints[i].y;
	double t[4];
	double t_min, t_max;
	//
	// nasty degenerate cases, effectively drawing to an infinity
	// point (?)  cope with them here, so don't process them as a
	// "real" OUTRANGE point
	//If more than one coord is -GPVL, then can't ratio the
	// "infinities" so drop out by returning false */
	//
	count = 0;
	if(ix == -GPVL)
		count++;
	if(ox == -GPVL)
		count++;
	if(iy == -GPVL)
		count++;
	if(oy == -GPVL)
		count++;
	// either doesn't pass through graph area *or* can't ratio
	// infinities to get a direction to draw line, so simply return(false)
	if(count > 1) {
		return (false);
	}
	else if(ox == -GPVL || ix == -GPVL) {
		// Horizontal line
		if(ix == -GPVL) {
			// swap pPoints so ix/iy don't have a -GPVL component
			Exchange(&ix, &ox);
			Exchange(&iy, &oy);
		}
		// check actually passes through the graph area
		if(ix > GetX().Range.GetActualUpp() && GetY().InRange(iy)) {
			pLx[0] = GetX().Range.low;
			pLy[0] = iy;
			pLx[1] = GetX().Range.upp;
			pLy[1] = iy;
			return (true);
		}
		else {
			return (false);
		}
	}
	else if(oy == -GPVL || iy == -GPVL) {
		// Vertical line
		if(iy == -GPVL) {
			// swap pPoints so ix/iy don't have a -GPVL component
			Exchange(&ix, &ox);
			Exchange(&iy, &oy);
		}
		// check actually passes through the graph area
		if(iy > GetY().Range.GetActualUpp() && GetX().InRange(ix)) {
			pLx[0] = ix;
			pLy[0] = GetY().Range.low;
			pLx[1] = ix;
			pLy[1] = GetY().Range.upp;
			return (true);
		}
		else {
			return (false);
		}
	}
	//
	// Special horizontal/vertical, etc. cases are checked and remaining slant lines are checked separately.
	//
	// The slant line intersections are solved using the parametric form
	// of the equation for a line, since if we test x/y min/max planes explicitly
	// then e.g. a  line passing through a corner point (GetX().min,GetY().min)
	// actually intersects 2 planes and hence further tests would be required
	// to anticipate this and similar situations.
	//
	// Can have case (ix == ox && iy == oy) as both pPoints OUTRANGE
	//
	else if(ix == ox && iy == oy) {
		return (false); /* but as only define single outrange point, can't intersect graph area */
	}
	else if(ix == ox) {
		// line parallel to y axis
		// x coord must be in range, and line must span both GetY().min and GetY().max
		// note that spanning GetY().min implies spanning GetY().max, as both pPoints OUTRANGE
		if(!GetX().InRange(ix)) {
			return (false);
		}
		else if(inrange(GetY().Range.low, iy, oy)) {
			pLx[0] = ix;
			pLy[0] = GetY().Range.low;
			pLx[1] = ix;
			pLy[1] = GetY().Range.upp;
			return (true);
		}
		else
			return (false);
	}
	else if(iy == oy) {
		// already checked case (ix == ox && iy == oy)

		// line parallel to x axis
		// y coord must be in range, and line must span both GetX().min and GetX().max
		// note that spanning GetX().min implies spanning GetX().max, as both pPoints OUTRANGE
		if(!GetY().Range.CheckX(iy)) {
			return (false);
		}
		else if(inrange(GetX().Range.low, ix, ox)) {
			pLx[0] = GetX().Range.low;
			pLy[0] = iy;
			pLx[1] = GetX().Range.upp;
			pLy[1] = iy;
			return (true);
		}
		else
			return (false);
	}
	else {
		// nasty 2D slanted line in an xy plane
		// From here on, it's essentially the classical Cyrus-Beck, or
		// Liang-Barsky algorithm for line clipping to a rectangle
		/*
		Solve parametric equation

		(ix, iy) + t (diff_x, diff_y)

		where 0.0 <= t <= 1.0 and

		diff_x = (ox - ix);
		diff_y = (oy - iy);
		*/
		t[0] = fscale(GetX().Range.low, ix, ox);
		t[1] = fscale(GetX().Range.upp, ix, ox);
		if(t[0] > t[1]) {
			Exchange(&t[0], &t[1]);
		}
		t[2] = fscale(GetY().Range.low, iy, oy);
		t[3] = fscale(GetY().Range.upp, iy, oy);
		if(t[2] > t[3]) {
			Exchange(&t[2], &t[3]);
		}
		t_min = MAX(MAX(t[0], t[2]), 0.0);
		t_max = MIN(MIN(t[1], t[3]), 1.0);
		if(t_min > t_max)
			return (false);
		else {
			pLx[0] = ix + t_min * (ox - ix);
			pLy[0] = iy + t_min * (oy - iy);
			pLx[1] = ix + t_max * (ox - ix);
			pLy[1] = iy + t_max * (oy - iy);
			// Can only have 0 or 2 intersection pPoints -- only need test one coord
			// FIXME: this is UGLY. Need an 'almost_inrange()' function
			const double _eps = 1e-5;
			const double xrng_eps = GetX().GetRange() * _eps;
			const double yrng_eps = GetY().GetRange() * _eps;
			if(inrange(pLx[0], (GetX().Range.low - xrng_eps), (GetX().Range.upp + xrng_eps)) && 
				inrange(pLy[0], (GetY().Range.low - yrng_eps), (GetY().Range.upp + yrng_eps))) {
				return (true);
			}
			else
				return (false);
		}
	}
}
//
// display a x-axis ticmark - called by gen_ticks
// also uses global tic_start, tic_direction, tic_text and tic_just
//
// grid.l_type == LT_NODRAW means no grid 
//
//static void xtick2d_callback(GpTermEntry * pT, GpTicCallbackParam * pP)
void GpGadgets::XTick2D_Callback(GpTermEntry * pT, GpTicCallbackParam * pP)
{
	// minitick if text is NULL - beware - HTic is unsigned 
	int ticsize = (int)(TicDirection * (int)pT->VTic * GetTicScale(pP));
	int x = (int)MapX(pP->Place);
	// Skip label if we've already written a user-specified one here 
#define MINIMUM_SEPARATION 2
	while(pP->P_M) {
		int here = MapX(pP->P_Ax->LogValue(pP->P_M->position));
		if(abs(here-x) <= MINIMUM_SEPARATION) {
			pP->P_Text = NULL;
			break;
		}
		pP->P_M = pP->P_M->next;
	}
#undef MINIMUM_SEPARATION
	if(pP->Style.l_type > LT_NODRAW) {
		pT->_Layer(TERM_LAYER_BEGIN_GRID);
		ApplyLpProperties(pT, &pP->Style);
		if(polar_grid_angle) {
			double x = pP->Place;
			double y = 0;
			double s = sin(0.1);
			double c = cos(0.1);
			int i;
			int ogx = MapX(x);
			int ogy = MapY(0);
			int gx, gy;
			if(pP->Place > LargestPolarCircle)
				LargestPolarCircle = pP->Place;
			else if(-pP->Place > LargestPolarCircle)
				LargestPolarCircle = -pP->Place;
			for(i = 1; i <= 63 /* 2pi/0.1 */; ++i) {
				{
					// cos(pT+dt) = cos(pT)cos(dt)-sin(pT)cos(dt) 
					double tx = x * c - y * s;
					// sin(pT+dt) = sin(pT)cos(dt)+cos(pT)sin(dt) 
					y = y * c + x * s;
					x = tx;
				}
				gx = MapX(x);
				gy = MapY(y);
				DrawClipLine(term, ogx, ogy, gx, gy);
				ogx = gx;
				ogy = gy;
			}
		}
		else {
			legend_key * key = &keyT;
			if(key->visible && x < key->bounds.xright && x > key->bounds.xleft && key->bounds.ytop > PlotBounds.ybot && key->bounds.ybot < PlotBounds.ytop) {
				if(key->bounds.ybot > PlotBounds.ybot) {
					pT->_Move(x, PlotBounds.ybot);
					pT->_Vector(x, key->bounds.ybot);
				}
				if(key->bounds.ytop < PlotBounds.ytop) {
					pT->_Move(x, key->bounds.ytop);
					pT->_Vector(x, PlotBounds.ytop);
				}
			}
			else {
				pT->_Move(x, PlotBounds.ybot);
				pT->_Vector(x, PlotBounds.ytop);
			}
		}
		ApplyLpProperties(pT, &BorderLp); /* border linetype */
		pT->_Layer(TERM_LAYER_END_GRID);
	}
	// we precomputed tic posn and text posn in global vars
	if(x >= P_Clip->xleft && x <= P_Clip->xright) {
		pT->_Move(x, TicStart);
		pT->_Vector(x, TicStart + ticsize);
		if(TicMirror >= 0) {
			pT->_Move(x, TicMirror);
			pT->_Vector(x, TicMirror - ticsize);
		}
		if(pP->P_Text) {
			// get offset 
			double offsetx_d, offsety_d;
			MapPositionR(pP->P_Ax->ticdef.offset, &offsetx_d, &offsety_d, "xtics");
			// User-specified different color for the tics text 
			if(pP->P_Ax->ticdef.textcolor.type != TC_DEFAULT)
				ApplyPm3DColor(pT, &pP->P_Ax->ticdef.textcolor);
			ignore_enhanced(!pP->P_Ax->ticdef.enhanced);
			pT->DrawMultiline(x+(int)offsetx_d, TicText+(int)offsety_d, pP->P_Text, (JUSTIFY)TicHJust, (VERT_JUSTIFY)TicVJust, RotateTics, pP->P_Ax->ticdef.font);
			ignore_enhanced(false);
			ApplyLpProperties(pT, &BorderLp); /* reset to border linetype */
		}
	}
}
//
// display a y-axis ticmark - called by gen_ticks
// also uses global tic_start, tic_direction, tic_text and tic_just
//
// grid.l_type == LT_NODRAW means no grid 
//
//static void ytick2d_callback(GpTermEntry * pT, GpTicCallbackParam * pP)
void GpGadgets::YTick2D_Callback(GpTermEntry * pT, GpTicCallbackParam * pP)
{
	// minitick if text is NULL - VTic is unsigned 
	int ticsize = (int)(TicDirection * (int)pT->HTic * GetTicScale(pP));
	int y = (pP->P_Ax->Index >= PARALLEL_AXES) ? pP->P_Ax->Map(pP->Place) : MapY(pP->Place);
	// Skip label if we've already written a user-specified one here 
#define MINIMUM_SEPARATION 2
	while(pP->P_M) {
		int here = MapY(pP->P_Ax->LogValue(pP->P_M->position));
		if(abs(here-y) <= MINIMUM_SEPARATION) {
			pP->P_Text = NULL;
			break;
		}
		pP->P_M = pP->P_M->next;
	}
#undef MINIMUM_SEPARATION
	if(pP->Style.l_type > LT_NODRAW) {
		pT->_Layer(TERM_LAYER_BEGIN_GRID);
		ApplyLpProperties(pT, &pP->Style);
		if(polar_grid_angle) {
			double x = 0;
			double y = pP->Place;
			double s = sin(0.1);
			double c = cos(0.1);
			if(pP->Place > LargestPolarCircle)
				LargestPolarCircle = pP->Place;
			else if(-pP->Place > LargestPolarCircle)
				LargestPolarCircle = -pP->Place;
			clip_move(MapX(x), MapY(y));
			for(int i = 1; i <= 63 /* 2pi/0.1 */; ++i) {
				{
					// cos(pT+dt) = cos(pT)cos(dt)-sin(pT)cos(dt) 
					const double tx = x * c - y * s;
					// sin(pT+dt) = sin(pT)cos(dt)+cos(pT)sin(dt) 
					y = y * c + x * s;
					x = tx;
				}
				ClipVector(pT, MapX(x), MapY(y));
			}
		}
		else {
			// Make the grid avoid the key box 
			legend_key * key = &keyT;
			if(key->visible && y < key->bounds.ytop && y > key->bounds.ybot
			    && key->bounds.xleft < PlotBounds.xright && key->bounds.xright > PlotBounds.xleft) {
				if(key->bounds.xleft > PlotBounds.xleft) {
					pT->_Move(PlotBounds.xleft, y);
					pT->_Vector(key->bounds.xleft, y);
				}
				if(key->bounds.xright < PlotBounds.xright) {
					pT->_Move(key->bounds.xright, y);
					pT->_Vector(PlotBounds.xright, y);
				}
			}
			else {
				pT->_Move(PlotBounds.xleft, y);
				pT->_Vector(PlotBounds.xright, y);
			}
		}
		ApplyLpProperties(pT, &BorderLp); /* border linetype */
		pT->_Layer(TERM_LAYER_END_GRID);
	}
	// we precomputed tic posn and text posn
	pT->_Move(TicStart, y);
	pT->_Vector(TicStart + ticsize, y);
	if(TicMirror >= 0) {
		pT->_Move(TicMirror, y);
		pT->_Vector(TicMirror - ticsize, y);
	}
	if(pP->P_Text) {
		// get offset 
		double offsetx_d, offsety_d;
		MapPositionR(pP->P_Ax->ticdef.offset, &offsetx_d, &offsety_d, "ytics");
		// User-specified different color for the tics text 
		if(pP->P_Ax->ticdef.textcolor.type != TC_DEFAULT)
			ApplyPm3DColor(pT, &(pP->P_Ax->ticdef.textcolor));
		ignore_enhanced(!pP->P_Ax->ticdef.enhanced);
		pT->DrawMultiline(TicText+(int)offsetx_d, y+(int)offsety_d, pP->P_Text, (JUSTIFY)TicHJust, (VERT_JUSTIFY)TicVJust, RotateTics, pP->P_Ax->ticdef.font);
		ignore_enhanced(false);
		ApplyLpProperties(pT, &BorderLp); // reset to border linetype 
	}
}

/*{{{  map_position, wrapper, which maps double to int */
//void map_position(GpPosition * pos, int * x, int * y, const char * what)
void GpGadgets::MapPosition(GpTermEntry * pT, GpPosition * pPos, int * pX, int * pY, const char * pWhat)
{
	double xx, yy;
	MapPositionDouble(pT, pPos, &xx, &yy, pWhat);
	*pX = (int)xx;
	*pY = (int)yy;
}

/*}}} */

/*{{{  map_position_double */
//static void map_position_double(GpPosition * pos, double * x, double * y, const char * what)
void GpGadgets::MapPositionDouble(GpTermEntry * pT, GpPosition * pPos, double * pX, double * pY, const char * pWhat)
{
	switch(pPos->scalex) {
		case first_axes:
		case second_axes:
	    {
		    AXIS_INDEX index = (pPos->scalex == first_axes) ? FIRST_X_AXIS : SECOND_X_AXIS;
		    GpAxis * this_axis = &AxA[index];
		    GpAxis * primary = this_axis->P_LinkToPrmr;
		    double xx;
		    if(primary && primary->link_udf->at) {
			    xx = primary->EvalLinkFunction(pPos->x);
			    *pX = primary->Map(xx);
		    }
		    else {
			    xx = LogValueChecked(index, pPos->x, pWhat);
			    *pX = Map(index, xx);
		    }
		    break;
	    }
		case graph:
		    *pX = PlotBounds.xleft + pPos->x * (PlotBounds.xright - PlotBounds.xleft);
		    break;
		case screen:
		    *pX = pPos->x * (pT->xmax - 1);
		    break;
		case character:
		    *pX = pPos->x * pT->HChr;
		    break;
	}
	switch(pPos->scaley) {
		case first_axes:
		case second_axes:
	    {
		    AXIS_INDEX index = (pPos->scaley == first_axes) ? FIRST_Y_AXIS : SECOND_Y_AXIS;
		    GpAxis * this_axis = &AxA[index];
		    GpAxis * primary = this_axis->P_LinkToPrmr;
		    double yy;
		    if(primary && primary->link_udf->at) {
			    yy = primary->EvalLinkFunction(pPos->y);
			    *pY = primary->Map(yy);
		    }
		    else {
			    yy = LogValueChecked(index, pPos->y, pWhat);
			    *pY = Map(index, yy);
		    }
		    break;
	    }
		case graph:
		    *pY = PlotBounds.ybot + pPos->y * (PlotBounds.ytop - PlotBounds.ybot);
		    break;
		case screen:
		    *pY = pPos->y * (pT->ymax -1);
		    break;
		case character:
		    *pY = pPos->y * pT->VChr;
		    break;
	}
	*pX += 0.5;
	*pY += 0.5;
}

/*}}} */

void GpGadgets::MapPositionR(const GpPosition & rPos, double * pX, double * pY, const char * pWhat)
{
	// Catches the case of "first" or "second" coords on a log-scaled axis
	if(rPos.x == 0)
		*pX = 0;
	else {
		switch(rPos.scalex) {
			case first_axes:
		    {
			    double xx = LogValueChecked(FIRST_X_AXIS, rPos.x, pWhat);
			    *pX = xx * AxA[FIRST_X_AXIS].term_scale;
			    break;
		    }
			case second_axes:
		    {
			    double xx = LogValueChecked(SECOND_X_AXIS, rPos.x, pWhat);
			    *pX = xx * AxA[SECOND_X_AXIS].term_scale;
			    break;
		    }
			case graph:
		    {
			    *pX = rPos.x * (PlotBounds.xright - PlotBounds.xleft);
			    break;
		    }
			case screen:
			    *pX = rPos.x * (term->xmax - 1);
			    break;
			case character:
			    *pX = rPos.x * term->HChr;
			    break;
		}
	}
	if(pY) { // Maybe they only want one GpCoordinate translated?
		// Catches the case of "first" or "second" coords on a log-scaled axis
		if(rPos.y == 0)
			*pY = 0;
		else {
			switch(rPos.scaley) {
				case first_axes:
				{
					double yy = LogValueChecked(FIRST_Y_AXIS, rPos.y, pWhat);
					*pY = yy * AxA[FIRST_Y_AXIS].term_scale;
					return;
				}
				case second_axes:
				{
					double yy = LogValueChecked(SECOND_Y_AXIS, rPos.y, pWhat);
					*pY = yy * AxA[SECOND_Y_AXIS].term_scale;
					return;
				}
				case graph:
				{
					*pY = rPos.y * (PlotBounds.ytop - PlotBounds.ybot);
					return;
				}
				case screen:
					*pY = rPos.y * (term->ymax -1);
					return;
				case character:
					*pY = rPos.y * term->VChr;
					break;
			}
		}
	}
}

/*}}} */

//static void plot_border(GpTermEntry * pT)
void GpGadgets::PlotBorder(GpTermEntry * pT)
{
	int min, max;
	pT->_Layer(TERM_LAYER_BEGIN_BORDER);
	ApplyLpProperties(pT, &BorderLp); // border linetype 
	if((DrawBorder & 15) == 15)
		newpath(pT);
	pT->_Move(PlotBounds.xleft, PlotBounds.ytop);
	if((DrawBorder & WEST) && AxA[FIRST_Y_AXIS].ticdef.rangelimited) {
		max = Map(FIRST_Y_AXIS, AxA[FIRST_Y_AXIS].DataRange.upp);
		min = Map(FIRST_Y_AXIS, AxA[FIRST_Y_AXIS].DataRange.low);
		pT->_Move(PlotBounds.xleft, max);
		pT->_Vector(PlotBounds.xleft, min);
		pT->_Move(PlotBounds.xleft, PlotBounds.ybot);
	}
	else if(DrawBorder & WEST) {
		pT->_Vector(PlotBounds.xleft, PlotBounds.ybot);
	}
	else {
		pT->_Move(PlotBounds.xleft, PlotBounds.ybot);
	}
	if((DrawBorder & SOUTH) && AxA[FIRST_X_AXIS].ticdef.rangelimited) {
		max = Map(FIRST_X_AXIS, AxA[FIRST_X_AXIS].DataRange.upp);
		min = Map(FIRST_X_AXIS, AxA[FIRST_X_AXIS].DataRange.low);
		pT->_Move(min, PlotBounds.ybot);
		pT->_Vector(max, PlotBounds.ybot);
		pT->_Move(PlotBounds.xright, PlotBounds.ybot);
	}
	else if(DrawBorder & SOUTH) {
		pT->_Vector(PlotBounds.xright, PlotBounds.ybot);
	}
	else {
		pT->_Move(PlotBounds.xright, PlotBounds.ybot);
	}
	if((DrawBorder & EAST) && AxA[SECOND_Y_AXIS].ticdef.rangelimited) {
		max = Map(SECOND_Y_AXIS, AxA[SECOND_Y_AXIS].DataRange.upp);
		min = Map(SECOND_Y_AXIS, AxA[SECOND_Y_AXIS].DataRange.low);
		pT->_Move(PlotBounds.xright, max);
		pT->_Vector(PlotBounds.xright, min);
		pT->_Move(PlotBounds.xright, PlotBounds.ybot);
	}
	else if(DrawBorder & EAST) {
		pT->_Vector(PlotBounds.xright, PlotBounds.ytop);
	}
	else {
		pT->_Move(PlotBounds.xright, PlotBounds.ytop);
	}
	if((DrawBorder & NORTH) && AxA[SECOND_X_AXIS].ticdef.rangelimited) {
		max = Map(SECOND_X_AXIS, AxA[SECOND_X_AXIS].DataRange.upp);
		min = Map(SECOND_X_AXIS, AxA[SECOND_X_AXIS].DataRange.low);
		pT->_Move(min, PlotBounds.ytop);
		pT->_Vector(max, PlotBounds.ytop);
		pT->_Move(PlotBounds.xright, PlotBounds.ytop);
	}
	else if(DrawBorder & NORTH) {
		pT->_Vector(PlotBounds.xleft, PlotBounds.ytop);
	}
	else {
		pT->_Move(PlotBounds.xleft, PlotBounds.ytop);
	}
	if((DrawBorder & 15) == 15)
		closepath(pT);
	pT->_Layer(TERM_LAYER_END_BORDER);
}

void init_histogram(histogram_style * histogram, GpTextLabel * title)
{
	ZFREE(GpGg.P_PrevRowStackHeight);
	if(histogram) {
		memcpy(histogram, &GpGg.histogram_opts, sizeof(GpGg.histogram_opts));
		memcpy(&histogram->title, title, sizeof(GpTextLabel));
		memzero(title, sizeof(GpTextLabel));
		// Insert in linked list
		GpGg.histogram_opts.next = histogram;
	}
}

void free_histlist(histogram_style * hist)
{
	if(hist) {
		if(hist != &GpGg.histogram_opts) {
			free(hist->title.text);
			free(hist->title.font);
		}
		if(hist->next) {
			free_histlist(hist->next);
			ZFREE(hist->next);
		}
	}
}

//static void place_histogram_titles()
void GpGraphics::PlaceHistogramTitles(GpTermEntry * pT)
{
	histogram_style * hist = &GpGg.histogram_opts;
	while((hist = hist->next)) {
		if(hist->title.text && *(hist->title.text)) {
			double xoffset_d, yoffset_d;
			GpGg.MapPositionR(GpGg.histogram_opts.title.offset, &xoffset_d, &yoffset_d, "histogram");
			uint x = GpGg.MapX((hist->start + hist->end) / 2.);
			uint y = B.XLabelY;
			// NB: offset in "newhistogram" is additive with that in "set style hist" 
			x += (int)xoffset_d;
			y += (int)((int)yoffset_d + 0.25 * pT->VChr);
			GpGg.WriteLabel(pT, x, y, &(hist->title));
			reset_textcolor(&hist->title.textcolor);
		}
	}
}

/*
 * Draw a solid line for the polar axis.
 * If the center of the polar plot is not at zero (rmin != 0)
 * indicate this by drawing an open circle.
 */
//static void place_raxis()
void GpGadgets::PlaceRAxis(GpTermEntry * pT)
{
#ifdef EAM_OBJECTS
	/*
	t_object raxis_circle = {
		NULL, 1, 1, OBJ_CIRCLE, OBJ_CLIP, // link, tag, layer (front), object_type, clip //
		{FS_SOLID, 100, 0, BLACK_COLORSPEC},
		{0, LT_BACKGROUND, 0, DASHTYPE_AXIS, 0, 0.2, 0.0, DEFAULT_P_CHAR, BACKGROUND_COLORSPEC, DEFAULT_DASHPATTERN},
		{.circle = {1, {0, 0, 0, 0., 0., 0.}, {graph, 0, 0, 0.02, 0., 0.}, 0., 360. }}
	};
	*/
	t_object raxis_circle(t_object::defCircle);
	raxis_circle.SetRAxisCircleStyle();
#endif
	int x0, y0, xend, yend;
	double rightend;
	x0 = MapX(0);
	y0 = MapY(0);
	rightend = (GetR().AutoScale & AUTOSCALE_MAX) ? GetR().Range.upp : GetR().SetRange.upp;
	xend = MapX(LogValue(POLAR_AXIS, rightend) - LogValue(POLAR_AXIS, GetR().SetRange.low));
	yend = y0;
	ApplyLpProperties(pT, &BorderLp);
	DrawClipLine(pT, x0, y0, xend, yend);
#ifdef EAM_OBJECTS
	if(!(GetR().AutoScale & AUTOSCALE_MIN) && GetR().SetRange.low != 0.0)
		PlaceObjects(pT, &raxis_circle, LAYER_FRONT, 2);
#endif
}

//static void place_parallel_axes(const CurvePoints * pFirstPlot, int pcount, int layer)
void GpGadgets::PlaceParallelAxes(GpTermEntry * pT, const CurvePoints * pFirstPlot, int pcount, int layer)
{
	int j;
	int axes_in_use = 0;
	const CurvePoints * plot = pFirstPlot;
	// Check for use of parallel axes
	for(j = 0; j < pcount; j++, plot = plot->P_Next) {
		if(plot->plot_type == DATA && plot->plot_style == PARALLELPLOT && plot->p_count > 0)
			SETMAX(axes_in_use, plot->n_par_axes);
	}
	// Set up the vertical scales used by axis_map()
	for(j = 0; j < axes_in_use; j++) {
		GpAxis * p_ax = &P_ParallelAxis[j];
		p_ax->InvertIfRequested();
		p_ax->TermBounds.low = PlotBounds.ybot;
		p_ax->term_scale = (PlotBounds.ytop - PlotBounds.ybot) / p_ax->Range.GetDistance();
		FPRINTF((stderr, "axis p%d: min %g max %g set_min %g set_max %g autoscale %o set_autoscale %o\n",
			j, p_ax->Range.low, p_ax->Range.upp, p_ax->set_min, p_ax->set_max, p_ax->AutoScale, p_ax->SetAutoScale));
		p_ax->SetupTics(20);
	}
	if(parallel_axis_style.layer == LAYER_FRONT && layer == LAYER_BACK)
		return;
	// Draw the axis lines
	ApplyLpProperties(pT, &parallel_axis_style.lp_properties);
	for(j = 0; j < axes_in_use; j++) {
		GpAxis * p_ax = &P_ParallelAxis[j];
		int max = p_ax->Map(p_ax->DataRange.upp);
		int min = p_ax->Map(p_ax->DataRange.low);
		int axis_x = MapX((double)(j+1));
		DrawClipLine(pT, axis_x, min, axis_x, max);
	}
	// Draw the axis tickmarks and labels.  Piggyback on ytick2d_callback
	// but avoid a call to the full AxisOutputTics().
	for(j = 0; j < axes_in_use; j++) {
		GpAxis * p_ax = &P_ParallelAxis[j];
		double axis_coord = j+1; // paxis N is drawn at x=N
		if(p_ax->tic_rotate && pT->text_angle(p_ax->tic_rotate)) {
			TicHJust = LEFT;
			TicVJust = CENTRE;
		}
		else {
			TicHJust = CENTRE;
			TicVJust = JUST_TOP;
		}
		if(p_ax->Flags & GpAxis::fManualJustify)
			TicHJust = p_ax->label.pos;
		TicStart     = AxA[FIRST_X_AXIS].Map(axis_coord);
		TicMirror    = TicStart; /* tic extends on both sides of axis */
		TicDirection = -1;
		TicText      = (int)(TicStart - p_ax->ticscale * pT->VTic);
		TicText     -= pT->VChr;
		GenTics(pT, *p_ax, &GpGadgets::YTick2D_Callback);
		pT->text_angle(0);
	}
}
//
// Label the curve by placing its title at one end of the curve.
// This option is independent of the plot key, but uses the same
// color/font/text options controlled by "set key".
//
//static void attach_title_to_plot(CurvePoints * this_plot, legend_key * key)
void GpGadgets::AttachTitleToPlot(GpTermEntry * pT, CurvePoints * pPlot, legend_key * pKey)
{
	int index, x, y;
	if(pPlot->plot_type != NODATA) {
		// beginning or end of plot trace
		if(pPlot->title_position->x > 0) {
			for(index = pPlot->p_count-1; index > 0; index--)
				if(pPlot->points[index].type == INRANGE)
					break;
		}
		else {
			for(index = 0; index < pPlot->p_count-1; index++)
				if(pPlot->points[index].type == INRANGE)
					break;
		}
		if(pPlot->points[index].type == INRANGE) {
			x = MapX(pPlot->points[index].x);
			y = MapY(pPlot->points[index].y);
			if(pKey->textcolor.type == TC_VARIABLE)
				; /* Draw pKey text in same color as plot */
			else if(pKey->textcolor.type != TC_DEFAULT)
				ApplyPm3DColor(pT, &pKey->textcolor); /* Draw pKey text in same color as pKey title */
			else
				pT->_LineType(LT_BLACK); /* Draw pKey text in black */
			pT->DrawMultiline(x, y, pPlot->title, (pPlot->title_position->x > 0) ? LEFT : RIGHT, JUST_TOP, 0, pKey->font);
		}
	}
}

#ifdef EAM_OBJECTS
//void do_rectangle(int dimensions, t_object * this_object, fill_style_type * fillstyle)
void GpGadgets::DoRectangle(GpTermEntry * pT, int dimensions, t_object * pObj, fill_style_type * pFillStyle)
{
	double x1, y1, x2, y2;
	int x, y;
	int style;
	uint w, h;
	bool clip_x = false;
	bool clip_y = false;
	t_rectangle * p_rect = &pObj->o.rectangle;
	if(p_rect->type == 1) {      /* specified as center + size */
		double width, height;
		if(dimensions == 2 || p_rect->center.scalex == screen) {
			MapPositionDouble(pT, &p_rect->center, &x1, &y1, "rect");
			MapPositionR(p_rect->extent, &width, &height, "rect");
		}
		else if(splot_map) {
			int junkw, junkh;
			Map3DPositionDouble(p_rect->center, &x1, &y1, "rect");
			Map3DPositionR(p_rect->extent, &junkw, &junkh, "rect");
			width = abs(junkw);
			height = abs(junkh);
		}
		else
			return;
		x1 -= width/2;
		y1 -= height/2;
		x2 = x1 + width;
		y2 = y1 + height;
		w = (uint)width;
		h = (uint)height;
		if(pObj->clip == OBJ_CLIP) {
			if(p_rect->extent.scalex == first_axes || p_rect->extent.scalex == second_axes)
				clip_x = true;
			if(p_rect->extent.scaley == first_axes || p_rect->extent.scaley == second_axes)
				clip_y = true;
		}
	}
	else {
		if((dimensions == 2) || (p_rect->bl.scalex == screen && p_rect->tr.scalex == screen)) {
			MapPositionDouble(pT, &p_rect->bl, &x1, &y1, "rect");
			MapPositionDouble(pT, &p_rect->tr, &x2, &y2, "rect");
		}
		else if(splot_map) {
			Map3DPositionDouble(p_rect->bl, &x1, &y1, "rect");
			Map3DPositionDouble(p_rect->tr, &x2, &y2, "rect");
		}
		else
			return;
		if(x1 > x2) {
			Exchange(&x1, &x2);
		}
		if(y1 > y2) {
			Exchange(&y1, &y2);
		}
		if(pObj->clip == OBJ_CLIP) {
			if(p_rect->bl.scalex != screen && p_rect->tr.scalex != screen)
				clip_x = true;
			if(p_rect->bl.scaley != screen && p_rect->tr.scaley != screen)
				clip_y = true;
		}
	}
	// FIXME - Should there be a generic clip_rectangle() routine?
	// Clip to the graph boundaries, but only if the rectangle
	// itself was specified in plot coords.
	if(P_Clip) {
		BoundingBox * clip_save = P_Clip;
		P_Clip = &PlotBounds;
		if(clip_x) {
			cliptorange(x1, P_Clip->xleft, P_Clip->xright);
			cliptorange(x2, P_Clip->xleft, P_Clip->xright);
		}
		if(clip_y) {
			cliptorange(y1, P_Clip->ybot, P_Clip->ytop);
			cliptorange(y2, P_Clip->ybot, P_Clip->ytop);
		}
		P_Clip = clip_save;
	}
	w = (uint)(x2 - x1);
	h = (uint)(y2 - y1);
	x = (int)x1;
	y = (int)y1;
	if(w && h) {
		style = style_from_fill(pFillStyle);
		if(style != FS_EMPTY && pT->fillbox)
			(*pT->fillbox)(style, x, y, w, h);
		// Now the border
		if(NeedFillBorder(pT, pFillStyle)) {
			newpath(pT);
			pT->_Move(x, y);
			pT->_Vector(x, y+h);
			pT->_Vector(x+w, y+h);
			pT->_Vector(x+w, y);
			pT->_Vector(x, y);
			closepath(pT);
		}
	}
}

//void do_ellipse(int dimensions, t_ellipse * e, int style, bool do_own_mapping)
void GpGadgets::DoEllipse(GpTermEntry * pT, int dimensions, t_ellipse * pEllipse, int style, bool doOwnMapping)
{
	gpiPoint vertex[120];
	int i, in;
	double angle;
	double cx, cy;
	double xoff, yoff;
	double junkfoo;
	int junkw, junkh;
	double cosO = cos(DEG2RAD * pEllipse->orientation);
	double sinO = sin(DEG2RAD * pEllipse->orientation);
	double A = pEllipse->extent.x / 2.0;   /* Major axis radius */
	double B = pEllipse->extent.y / 2.0;   /* Minor axis radius */
	GpPosition pos = pEllipse->extent; /* working copy with axis info attached */
	double aspect = (double)pT->VTic / (double)pT->HTic;
	// Choose how many segments to draw for this ellipse
	int segments = 72;
	double ang_inc  =  M_PI / 36.;
#ifdef WIN32
	if(strcmp(pT->name, "windows") == 0)
		aspect = 1.;
#endif
	// Find the center of the ellipse 
	// If this ellipse is part of a plot - as opposed to an object -
	// then the caller plot_ellipses function already did the mapping for us.
	// Else we do it here. The 'ellipses' plot style is 2D only, but objects
	// can apparently be placed on splot maps too, so we do 3D mapping if needed. 
	if(!doOwnMapping) {
		cx = pEllipse->center.x;
		cy = pEllipse->center.y;
	}
	else if(dimensions == 2)
		MapPositionDouble(pT, &pEllipse->center, &cx, &cy, "ellipse");
	else
		Map3DPositionDouble(pEllipse->center, &cx, &cy, "ellipse");
	// Calculate the vertices
	for(i = 0, angle = 0.0; i<=segments; i++, angle += ang_inc) {
		/* Given that the (co)sines of same sequence of angles
		 * are calculated every time - shouldn't they be precomputed
		 * and put into a table? */
		pos.x = A * cosO * cos(angle) - B * sinO * sin(angle);
		pos.y = A * sinO * cos(angle) + B * cosO * sin(angle);
		if(!doOwnMapping) {
			xoff = pos.x;
			yoff = pos.y;
		}
		else if(dimensions == 2) {
			switch(pEllipse->type) {
				case ELLIPSEAXES_XY:
				    MapPositionR(pos, &xoff, &yoff, "ellipse");
				    break;
				case ELLIPSEAXES_XX:
				    MapPositionR(pos, &xoff, NULL, "ellipse");
				    pos.x = pos.y;
				    MapPositionR(pos, &yoff, NULL, "ellipse");
				    break;
				case ELLIPSEAXES_YY:
				    MapPositionR(pos, &junkfoo, &yoff, "ellipse");
				    pos.y = pos.x;
				    MapPositionR(pos, &junkfoo, &xoff, "ellipse");
				    break;
			}
		}
		else {
			switch(pEllipse->type) {
				case ELLIPSEAXES_XY:
				    Map3DPositionR(pos, &junkw, &junkh, "ellipse");
				    xoff = junkw;
				    yoff = junkh;
				    break;
				case ELLIPSEAXES_XX:
				    Map3DPositionR(pos, &junkw, &junkh, "ellipse");
				    xoff = junkw;
				    pos.x = pos.y;
				    Map3DPositionR(pos, &junkh, &junkw, "ellipse");
				    yoff = junkh;
				    break;
				case ELLIPSEAXES_YY:
				    Map3DPositionR(pos, &junkw, &junkh, "ellipse");
				    yoff = junkh;
				    pos.y = pos.x;
				    Map3DPositionR(pos, &junkh, &junkw, "ellipse");
				    xoff = junkw;
				    break;
			}
		}
		vertex[i].x = (int)(cx + xoff);
		vertex[i].y = doOwnMapping ? (int)(cy + yoff) : (int)(cy + yoff * aspect);
	}
	if(style) {
		// Fill in the center
		gpiPoint fillarea[120];
		ClipPolygon(vertex, fillarea, segments, &in);
		fillarea[0].style = style;
		if(pT->filled_polygon)
			pT->filled_polygon(in, fillarea);
	}
	else
		DrawClipPolygon(pT, segments+1, vertex); // Draw the arc
}

//void do_polygon(int dimensions, t_polygon * p, int style, t_clip_object clip)
void GpGadgets::DoPolygon(GpTermEntry * pT, int dimensions, t_polygon * pPolygon, int style, t_clip_object clip)
{
	static gpiPoint * corners = NULL; // @global
	static gpiPoint * clpcorn = NULL; // @global
	BoundingBox * clip_save = P_Clip;
	int nv;
	if(pPolygon->vertex && pPolygon->type >= 2) {
		corners = (gpiPoint *)gp_realloc(corners, pPolygon->type * sizeof(gpiPoint), "polygon");
		clpcorn = (gpiPoint *)gp_realloc(clpcorn, 2 * pPolygon->type * sizeof(gpiPoint), "polygon");
		for(nv = 0; nv < pPolygon->type; nv++) {
			if(dimensions == 3)
				Map3DPosition(pPolygon->vertex[nv], &corners[nv].x, &corners[nv].y, "pvert");
			else
				MapPosition(pT, &pPolygon->vertex[nv], &corners[nv].x, &corners[nv].y, "pvert");
			// Any vertex given in screen coords will disable clipping
			if(pPolygon->vertex[nv].scalex == screen || pPolygon->vertex[nv].scaley == screen)
				clip = OBJ_NOCLIP;
		}
		if(clip == OBJ_NOCLIP)
			P_Clip = &Canvas;
		if(pT->filled_polygon && style) {
			int out_length;
			ClipPolygon(corners, clpcorn, nv, &out_length);
			clpcorn[0].style = style;
			pT->filled_polygon(out_length, clpcorn);
		}
		else { // Just draw the outline?
			newpath(pT);
			DrawClipPolygon(pT, nv, corners);
			closepath(pT);
		}
		P_Clip = clip_save;
	}
}

#endif

bool check_for_variable_color(CurvePoints * plot, double * colorvalue)
{
	bool result = false;
	if(plot->varcolor) {
		if((plot->lp_properties.pm3d_color.value < 0.0) &&  (plot->lp_properties.pm3d_color.type == TC_RGB)) {
			GpGg.SetRgbColorVar(term, (uint)*colorvalue);
			result = true;
		}
		else if(plot->lp_properties.pm3d_color.type == TC_Z) {
			term->SetColor(GpGg.CB2Gray(*colorvalue) );
			result = true;
		}
		else if(plot->lp_properties.l_type == LT_COLORFROMCOLUMN) {
			lp_style_type lptmp;
			// lc variable will only pick up line _style_ as opposed to _type_ 
			// in the case of "set style increment user".  THIS IS A CHANGE.   
			if(GpGg.prefer_line_styles)
				lp_use_properties(&lptmp, (int)(*colorvalue));
			else
				load_linetype(&lptmp, (int)(*colorvalue));
			GpGg.ApplyPm3DColor(term, &(lptmp.pm3d_color));
			result = true;
		}
	}
	return result;
}
//
// Similar to HBB's comment above, this routine is shared with
// graph3d.c, so it shouldn't be in this module (graphics.c).
// However, I feel that 2d and 3d graphing routines should be
// made as much in common as possible.  They seem to be
// bifurcating a bit too much.  (Dan Sebald)
// 
/* process_image:
 *
 * IMG_PLOT - Plot the coordinates similar to the points option except
 *  use pixels.  Check if the data forms a valid image array, i.e., one
 *  for which points are spaced equidistant along two non-coincidence
 *  vectors.  If the two directions are orthogonal within some tolerance
 *  and they are aligned with the view box x and y directions, then use
 *  the image feature of the terminal if it has one.  Otherwise, use
 *  parallelograms via the polynomial function to represent pixels.
 *
 * IMG_UPDATE_AXES - Update the axis ranges for `set autoscale` and then
 *  return.
 *
 * IMG_UPDATE_CORNERS - Update the corners of the outlining phantom
 *  parallelogram for `set hidden3d` and then return.
 */
//void process_image(void * plot, t_procimg_action action)
void GpGadgets::ProcessImage(GpTermEntry * pT, void * pPlot, t_procimg_action action)
{
	GpCoordinate * p_pts;
	int    p_count;
	int    i;
	double p_start_corner[2], p_end_corner[2]; // Points used for computing hyperplane
	int    K = 0, L = 0; // Dimensions of image grid. K = <scan line length>, L = <number of scan lines>. 
	uint   ncols, nrows;              /* EAM DEBUG - intended to replace K and L above */
	double p_mid_corner[2];                /* Point representing first corner found, i.e. p(K-1) */
	double delta_x_grid[2] = {0, 0};       /* Spacings between p_pts, two non-orthogonal directions. */
	double delta_y_grid[2] = {0, 0};
	int    grid_corner[4] = {-1, -1, -1, -1}; /* The corner pixels of the image. */
	// Viewable portion of the image {
	RealRange view_port_x_; 
	RealRange view_port_y_;
	RealRange view_port_z_;
	view_port_z_.SetVal(0.0);
	// }
	t_imagecolor pixel_planes;
	// Detours necessary to handle 3D plots 
	bool   project_points = false; // True if 3D pPlot 
	int    image_x_axis, image_y_axis, image_z_axis;
	if((((SurfacePoints*)pPlot)->plot_type == DATA3D) || (((SurfacePoints*)pPlot)->plot_type == FUNC3D))
		project_points = true;
	if(project_points) {
		p_pts = ((SurfacePoints*)pPlot)->iso_crvs->points;
		p_count = ((SurfacePoints*)pPlot)->iso_crvs->p_count;
		pixel_planes = ((SurfacePoints*)pPlot)->image_properties.type;
		ncols = ((SurfacePoints*)pPlot)->image_properties.ncols;
		nrows = ((SurfacePoints*)pPlot)->image_properties.nrows;
		image_x_axis = FIRST_X_AXIS;
		image_y_axis = FIRST_Y_AXIS;
		image_z_axis = FIRST_Z_AXIS; /* FIXME:  Not sure this is correct */
	}
	else {
		p_pts = ((CurvePoints*)pPlot)->points;
		p_count = ((CurvePoints*)pPlot)->p_count;
		pixel_planes = ((CurvePoints*)pPlot)->image_properties.type;
		ncols = ((CurvePoints*)pPlot)->image_properties.ncols;
		nrows = ((CurvePoints*)pPlot)->image_properties.nrows;
		image_x_axis = ((CurvePoints*)pPlot)->x_axis;
		image_y_axis = ((CurvePoints*)pPlot)->y_axis;
		image_z_axis = ((CurvePoints*)pPlot)->z_axis;
	}
	if(p_count < 1) {
		IntWarn(NO_CARET, "No p_pts (visible or invisible) to pPlot.\n\n");
		return;
	}
	if(p_count < 4) {
		IntWarn(NO_CARET, "Image grid must be at least 4 p_pts (2 x 2).\n\n");
		return;
	}
	if(project_points && ((GetX().Flags & GpAxis::fLog) || (GetY().Flags & GpAxis::fLog) || (GetZ().Flags & GpAxis::fLog))) {
		IntWarn(NO_CARET, "Log scaling of 3D image plots is not supported");
		return;
	}
	//
	// Check if the pixel data forms a valid rectangular grid for potential image
	// matrix support.  A general grid orientation is considered.  If the grid
	// p_pts are orthogonal and oriented along the x/y dimensions the terminal
	// function for images will be used.  Otherwise, the terminal function for
	// filled polygons are used to construct parallelograms for the pixel elements.
	// 
#define GRIDX(X) DelogValue((AXIS_INDEX)image_x_axis, p_pts[X].x)
#define GRIDY(Y) DelogValue((AXIS_INDEX)image_y_axis, p_pts[Y].y)
#define GRIDZ(Z) DelogValue((AXIS_INDEX)image_z_axis, p_pts[Z].z)
	if(project_points) {
		Map3DXY(p_pts[0].x, p_pts[0].y, p_pts[0].z, &p_start_corner[0], &p_start_corner[1]);
		Map3DXY(p_pts[p_count-1].x, p_pts[p_count-1].y, p_pts[p_count-1].z, &p_end_corner[0], &p_end_corner[1]);
	}
	else if(GetX().Flags & GpAxis::fLog || GetY().Flags & GpAxis::fLog) {
		p_start_corner[0] = GRIDX(0);
		p_start_corner[1] = GRIDY(0);
		p_end_corner[0] = GRIDX(p_count-1);
		p_end_corner[1] = GRIDY(p_count-1);
	}
	else {
		p_start_corner[0] = p_pts[0].x;
		p_start_corner[1] = p_pts[0].y;
		p_end_corner[0] = p_pts[p_count-1].x;
		p_end_corner[1] = p_pts[p_count-1].y;
	}

	// This is a vestige of older code that calculated K and L on the fly	
	// rather than keeping track of matrix/array/image dimensions on input	
	K = ncols;
	L = nrows;

	// FIXME: We don't track the dimensions of image data provided as x/y/value	
	// with individual coords rather than via array, matrix, or image format.	
	// This might better be done when the data is entered rather than here.	
	if(L == 0 || K == 0) {
		if(p_pts[0].x == p_pts[1].x) {
			// y coord varies fastest 
			for(K = 0; p_pts[K].x == p_pts[0].x; K++)
				if(K >= p_count)
					break;
			L = p_count / K;
		}
		else {
			// x coord varies fastest 
			for(K = 0; p_pts[K].y == p_pts[0].y; K++)
				if(K >= p_count)
					break;
			L = p_count / K;
		}
		fprintf(stderr, "No dimension information for %d pixels total. Try %d x %d\n", p_count, L, K);
	}
	grid_corner[0] = 0;
	grid_corner[1] = K-1;
	grid_corner[3] = p_count - 1;
	grid_corner[2] = p_count - K;
	if(action == IMG_UPDATE_AXES) {
		for(i = 0; i < 4; i++) {
			coord_type dummy_type;
			double x, y;
			if((GetX().Flags & GpAxis::fLog) || (GetY().Flags & GpAxis::fLog)) {
				x = GRIDX(i);
				y = GRIDY(i);
				x -= (GRIDX((5-i)%4) - GRIDX(i)) / (2*(K-1));
				y -= (GRIDY((5-i)%4) - GRIDY(i)) / (2*(K-1));
				x -= (GRIDX((i+2)%4) - GRIDX(i)) / (2*(L-1));
				y -= (GRIDY((i+2)%4) - GRIDY(i)) / (2*(L-1));
			}
			else {
				x = p_pts[grid_corner[i]].x;
				y = p_pts[grid_corner[i]].y;
				x -= (p_pts[grid_corner[(5-i)%4]].x - p_pts[grid_corner[i]].x)/(2*(K-1));
				y -= (p_pts[grid_corner[(5-i)%4]].y - p_pts[grid_corner[i]].y)/(2*(K-1));
				x -= (p_pts[grid_corner[(i+2)%4]].x - p_pts[grid_corner[i]].x)/(2*(L-1));
				y -= (p_pts[grid_corner[(i+2)%4]].y - p_pts[grid_corner[i]].y)/(2*(L-1));
			}
			// Update range and store value back into itself. 
			dummy_type = INRANGE;
			STORE_WITH_LOG_AND_UPDATE_RANGE(x, x, dummy_type, image_x_axis, ((CurvePoints*)pPlot)->noautoscale, NOOP, x = -GPVL);
			dummy_type = INRANGE;
			STORE_WITH_LOG_AND_UPDATE_RANGE(y, y, dummy_type, image_y_axis, ((CurvePoints*)pPlot)->noautoscale, NOOP, y = -GPVL);
		}
		return;
	}
	if(action == IMG_UPDATE_CORNERS) {
		// Shortcut pointer to phantom parallelogram. 
		iso_curve * iso_crvs = ((SurfacePoints*)pPlot)->next_sp->iso_crvs;

		/* Set the phantom parallelogram as an outline of the image.  Use
		 * corner point 0 as a reference point.  Imagine vectors along the
		 * generally non-orthogonal directions of the two nearby corners. */

		double delta_x_1 = (p_pts[grid_corner[1]].x - p_pts[grid_corner[0]].x)/(2*(K-1));
		double delta_y_1 = (p_pts[grid_corner[1]].y - p_pts[grid_corner[0]].y)/(2*(K-1));
		double delta_z_1 = (p_pts[grid_corner[1]].z - p_pts[grid_corner[0]].z)/(2*(K-1));
		double delta_x_2 = (p_pts[grid_corner[2]].x - p_pts[grid_corner[0]].x)/(2*(L-1));
		double delta_y_2 = (p_pts[grid_corner[2]].y - p_pts[grid_corner[0]].y)/(2*(L-1));
		double delta_z_2 = (p_pts[grid_corner[2]].z - p_pts[grid_corner[0]].z)/(2*(L-1));

		iso_crvs->points[0].x = p_pts[grid_corner[0]].x - delta_x_1 - delta_x_2;
		iso_crvs->points[0].y = p_pts[grid_corner[0]].y - delta_y_1 - delta_y_2;
		iso_crvs->points[0].z = p_pts[grid_corner[0]].z - delta_z_1 - delta_z_2;
		iso_crvs->next->points[0].x = p_pts[grid_corner[2]].x - delta_x_1 + delta_x_2;
		iso_crvs->next->points[0].y = p_pts[grid_corner[2]].y - delta_y_1 + delta_y_2;
		iso_crvs->next->points[0].z = p_pts[grid_corner[2]].z - delta_z_1 + delta_z_2;
		iso_crvs->points[1].x = p_pts[grid_corner[1]].x + delta_x_1 - delta_x_2;
		iso_crvs->points[1].y = p_pts[grid_corner[1]].y + delta_y_1 - delta_y_2;
		iso_crvs->points[1].z = p_pts[grid_corner[1]].z + delta_z_1 - delta_z_2;
		iso_crvs->next->points[1].x = p_pts[grid_corner[3]].x + delta_x_1 + delta_x_2;
		iso_crvs->next->points[1].y = p_pts[grid_corner[3]].y + delta_y_1 + delta_y_2;
		iso_crvs->next->points[1].z = p_pts[grid_corner[3]].z + delta_z_1 + delta_z_2;
		return;
	}
	if(project_points) {
		Map3DXY(p_pts[K-1].x, p_pts[K-1].y, p_pts[K-1].z, &p_mid_corner[0], &p_mid_corner[1]);
	}
	else if((GetX().Flags & GpAxis::fLog) || (GetY().Flags & GpAxis::fLog)) {
		p_mid_corner[0] = GRIDX(K-1);
		p_mid_corner[1] = GRIDY(K-1);
	}
	else {
		p_mid_corner[0] = p_pts[K-1].x;
		p_mid_corner[1] = p_pts[K-1].y;
	}

	/* The grid spacing in one direction. */
	delta_x_grid[0] = (p_mid_corner[0] - p_start_corner[0])/(K-1);
	delta_y_grid[0] = (p_mid_corner[1] - p_start_corner[1])/(K-1);
	/* The grid spacing in the second direction. */
	delta_x_grid[1] = (p_end_corner[0] - p_mid_corner[0])/(L-1);
	delta_y_grid[1] = (p_end_corner[1] - p_mid_corner[1])/(L-1);

	/* Check if the pixel grid is orthogonal and oriented with axes.
	 * If so, then can use efficient terminal image routines.
	 */
	{
		bool rectangular_image = false;
		bool fallback = false;
#define SHIFT_TOLERANCE 0.01
		if(((fabs(delta_x_grid[0]) < SHIFT_TOLERANCE*fabs(delta_x_grid[1])) || (fabs(delta_x_grid[1]) < SHIFT_TOLERANCE*fabs(delta_x_grid[0])) )
		    && ((fabs(delta_y_grid[0]) < SHIFT_TOLERANCE*fabs(delta_y_grid[1])) || (fabs(delta_y_grid[1]) < SHIFT_TOLERANCE*fabs(delta_y_grid[0])))) {
			rectangular_image = true;
			// If the terminal does not have image support then fall back to using polygons to construct pixels.
			if(project_points)
				fallback = !splot_map || ((SurfacePoints*)pPlot)->image_properties.fallback;
			else
				fallback = ((CurvePoints*)pPlot)->image_properties.fallback;
		}
		if(pixel_planes == IC_PALETTE && MakePalette(pT)) {
			return; // IntWarn(NO_CARET, "This terminal does not support palette-based images.\n\n"); 
		}
		if(oneof2(pixel_planes, IC_RGB, IC_RGBA) && ((pT->flags & TERM_NULL_SET_COLOR))) {
			// IntWarn(NO_CARET, "This terminal does not support rgb images.\n\n"); 
			return;
		}
		// Use generic code to handle alpha channel if the terminal can't 
		if(pixel_planes == IC_RGBA && !(pT->flags & TERM_ALPHA_CHANNEL))
			fallback = true;
		// Also use generic code if the pixels are of unequal size, e.g. log scale 
		if((GetX().Flags & GpAxis::fLog) || (GetY().Flags & GpAxis::fLog))
			fallback = true;
		GetViewPortX(view_port_x_);
		GetViewPortY(view_port_y_);
		if(project_points) {
			GetViewPortZ(view_port_z_);
		}
		if(rectangular_image && pT->image && !fallback) {
			// There are eight ways that a valid pixel grid can be entered.  Use table
			// lookup instead of if() statements.  (Draw the various array combinations
			// on a sheet of paper, or see the README file.)
			int line_length, i_delta_pixel, i_delta_line, i_start;
			int pixel_1_1, pixel_M_N;
			double * image;
			int array_size;
			float xsts, ysts;
			if(!project_points) {
				// Determine axis direction according to the sign of the terminal scale
				xsts = (GetX().term_scale > 0 ? +1.0f : -1.0f);
				ysts = (GetY().term_scale > 0 ? +1.0f : -1.0f);
			}
			else {
				// 3D plots do not use the term_scale mechanism
				xsts = 1;
				ysts = 1;
			}
			//
			// Set up parameters for indexing through the image matrix to transfer data.
			// These formulas were derived for a terminal image routine which uses the
			// upper left corner as pixel (1,1).
			//
			if(fabs(delta_x_grid[0]) > fabs(delta_x_grid[1])) {
				line_length = K;
				i_start = (delta_y_grid[1]*ysts > 0 ? L : 1) * K - (delta_x_grid[0]*xsts > 0 ? K : 1);
				i_delta_pixel = (delta_x_grid[0]*xsts > 0 ? +1 : -1);
				i_delta_line = (delta_x_grid[0]*xsts > 0 ? -K : +K) + (delta_y_grid[1]*ysts > 0 ? -K : +K);
			}
			else {
				line_length = L;
				i_start = (delta_x_grid[1]*xsts > 0 ? 1 : L) * K - (delta_y_grid[0]*ysts > 0 ? 1 : K);
				i_delta_pixel = (delta_x_grid[1]*xsts > 0 ? +K : -K);
				i_delta_line = K*L*(delta_x_grid[1]*xsts > 0 ? -1 : +1) + (delta_y_grid[0]*ysts > 0 ? -1 : +1);
			}

			/* Assign enough memory for the maximum image size. */
			array_size = K*L;

			/* If doing color, multiply size by three for RGB triples. */
			if(pixel_planes == IC_RGB)
				array_size *= 3;
			else if(pixel_planes == IC_RGBA)
				array_size *= 4;
			image = (double*)malloc(array_size*sizeof(image[0]));

			/* Place p_pts into image array based upon the arrangement of point indices and
			 * the visibility of pixels.
			 */
			if(image) {
				int    j;
				gpiPoint corners[4];
				int    M = 0, N = 0; // M = number of columns, N = number of rows.  (K and L don't have a set direction, but M and N do.)
				int    i_image, i_sub_image = 0;
				int    line_pixel_count = 0;
				const double d_x_o_2 = ((p_pts[grid_corner[0]].x - p_pts[grid_corner[1]].x)/(K-1) + (p_pts[grid_corner[0]].x - p_pts[grid_corner[2]].x)/(L-1)) / 2;
				const double d_y_o_2 = ((p_pts[grid_corner[0]].y - p_pts[grid_corner[1]].y)/(K-1) + (p_pts[grid_corner[0]].y - p_pts[grid_corner[2]].y)/(L-1)) / 2;
				const double d_z_o_2 = ((p_pts[grid_corner[0]].z - p_pts[grid_corner[1]].z)/(K-1) + (p_pts[grid_corner[0]].z - p_pts[grid_corner[2]].z)/(L-1)) / 2;
				pixel_1_1 = -1;
				pixel_M_N = -1;
				// Step through the p_pts placing them in the proper spot in the matrix array
				for(i = 0, j = line_length, i_image = i_start; i < p_count; i++) {
					bool visible;
					RPoint3 _pnt;
					_pnt.Set(p_pts[i_image].x, p_pts[i_image].y, p_pts[i_image].z);
					const double x_low  = _pnt.x - d_x_o_2;
					const double x_high = _pnt.x + d_x_o_2;
					const double y_low  = _pnt.y - d_y_o_2;
					const double y_high = _pnt.y + d_y_o_2;
					const double z_low  = _pnt.z - d_z_o_2;
					const double z_high = _pnt.z + d_z_o_2;
					/* Check if a portion of this pixel will be visible.  Do not use the
					 * p_pts[i].type == INRANGE test because a portion of a pixel can
					 * extend into view and the INRANGE type doesn't account for this.
					 *
					 * This series of tests is designed for speed.  If one of the corners
					 * of the pixel in question falls in the view port range then the pixel
					 * will be visible.  Do this test first because it is the more likely
					 * of situations.  It could also happen that the view port is smaller
					 * than a pixel.  In that case, if one of the view port corners lands
					 * inside the pixel then the pixel in question will be visible.  This
					 * won't be as common, so do those tests last.  Set up the if structure
					 * in such a way that as soon as one of the tests is true, the conditional
					 * tests stop.
					 */
					if((inrange(x_low, view_port_x_.low, view_port_x_.upp) || inrange(x_high, view_port_x_.low, view_port_x_.upp))
					    && (inrange(y_low, view_port_y_.low, view_port_y_.upp) || inrange(y_high, view_port_y_.low, view_port_y_.upp))
					    && (!project_points || inrange(z_low, view_port_z_.low, view_port_z_.upp) || inrange(z_high, view_port_z_.low, view_port_z_.upp)))
						visible = true;
					else if((inrange(view_port_x_.low, x_low, x_high) || inrange(view_port_x_.upp, x_low, x_high))
					    && (inrange(view_port_y_.low, y_low, y_high) || inrange(view_port_y_.upp, y_low, y_high))
					    && (!project_points || inrange(view_port_z_.low, z_low, z_high) || inrange(view_port_z_.upp, z_low, z_high)))
						visible = true;
					else
						visible = false;
					if(visible) {
						if(pixel_1_1 < 0) {
							// First visible point
							pixel_1_1 = i_image;
							M = 0;
							N = 1;
							line_pixel_count = 1;
						}
						else {
							if(line_pixel_count == 0)
								N += 1;
							line_pixel_count++;
							if((N != 1) && (line_pixel_count > M)) {
								IntWarn(NO_CARET, "Visible pixel grid has a scan line longer than previous scan lines.");
								return;
							}
						}
						// This can happen if the data supplied for a matrix does not
						// match the matrix dimensions found when the file was opened
						if(i_sub_image >= array_size) {
							IntWarn(NO_CARET, "image data corruption");
							break;
						}
						pixel_M_N = i_image;
						if(pixel_planes == IC_PALETTE) {
							image[i_sub_image++] = CB2Gray(p_pts[i_image].CRD_COLOR);
						}
						else {
							image[i_sub_image++] = CB2Gray(p_pts[i_image].CRD_R);
							image[i_sub_image++] = CB2Gray(p_pts[i_image].CRD_G);
							image[i_sub_image++] = CB2Gray(p_pts[i_image].CRD_B);
							if(pixel_planes == IC_RGBA)
								image[i_sub_image++] = p_pts[i_image].CRD_A;
						}
					}
					i_image += i_delta_pixel;
					j--;
					if(j == 0) {
						if(M == 0)
							M = line_pixel_count;
						else if((line_pixel_count > 0) && (line_pixel_count != M)) {
							IntWarn(NO_CARET, "Visible pixel grid has a scan line shorter than previous scan lines.");
							return;
						}
						line_pixel_count = 0;
						i_image += i_delta_line;
						j = line_length;
					}
				}
				if((M > 0) && (N > 0)) {
					/* The information collected to this point is:
					 *
					 * M = <number of columns>
					 * N = <number of rows>
					 * image[] = M x N array of pixel data.
					 * pixel_1_1 = position in p_pts[] associated with pixel (1,1)
					 * pixel_M_N = position in p_pts[] associated with pixel (M,N)
					 */

					/* One of the delta values in each direction is zero, so add. */
					if(project_points) {
						double x, y;
						Map3DXY(p_pts[pixel_1_1].x, p_pts[pixel_1_1].y, p_pts[pixel_1_1].z, &x, &y);
						corners[0].x = (int)(x - fabs(delta_x_grid[0]+delta_x_grid[1])/2);
						corners[0].y = (int)(y + fabs(delta_y_grid[0]+delta_y_grid[1])/2);
						Map3DXY(p_pts[pixel_M_N].x, p_pts[pixel_M_N].y, p_pts[pixel_M_N].z, &x, &y);
						corners[1].x = (int)(x + fabs(delta_x_grid[0]+delta_x_grid[1])/2);
						corners[1].y = (int)(y - fabs(delta_y_grid[0]+delta_y_grid[1])/2);
						Map3DXY(view_port_x_.low, view_port_y_.low, view_port_z_.low, &x, &y);
						corners[2].x = (int)x;
						corners[2].y = (int)y;
						Map3DXY(view_port_x_.upp, view_port_y_.upp, view_port_z_.upp, &x, &y);
						corners[3].x = (int)x;
						corners[3].y = (int)y;
					}
					else {
						corners[0].x = MapX(p_pts[pixel_1_1].x - xsts*fabs(d_x_o_2));
						corners[0].y = MapY(p_pts[pixel_1_1].y + ysts*fabs(d_y_o_2));
						corners[1].x = MapX(p_pts[pixel_M_N].x + xsts*fabs(d_x_o_2));
						corners[1].y = MapY(p_pts[pixel_M_N].y - ysts*fabs(d_y_o_2));
						corners[2].x = MapX(view_port_x_.low);
						corners[2].y = MapY(view_port_y_.upp);
						corners[3].x = MapX(view_port_x_.upp);
						corners[3].y = MapY(view_port_y_.low);
					}
					(*pT->image)(M, N, image, corners, pixel_planes);
				}
				free((void*)image);
			}
			else {
				IntWarn(NO_CARET, "Could not allocate memory for image.");
				return;
			}
		}
		else { // no pT->image  or "with image pixels"
			// Use sum of vectors to compute the pixel corners with respect to its center
			RPoint3 delta_grid[2];
			RPoint3 delta_pixel[2];
			int j, i_image;
			const bool log_axes = ((GetX().Flags & GpAxis::fLog) || (GetY().Flags & GpAxis::fLog));
			if(!pT->filled_polygon)
				GpGg.IntErrorNoCaret("This terminal does not support filled polygons");
			pT->_Layer(TERM_LAYER_BEGIN_IMAGE);
			// Grid spacing in 3D space
			if(log_axes) {
				delta_grid[0].x = (GRIDX(grid_corner[1]) - GRIDX(grid_corner[0])) / (K-1);
				delta_grid[0].y = (GRIDY(grid_corner[1]) - GRIDY(grid_corner[0])) / (K-1);
				delta_grid[0].z = (GRIDZ(grid_corner[1]) - GRIDZ(grid_corner[0])) / (K-1);
				delta_grid[1].x = (GRIDX(grid_corner[2]) - GRIDX(grid_corner[0])) / (L-1);
				delta_grid[1].y = (GRIDY(grid_corner[2]) - GRIDY(grid_corner[0])) / (L-1);
				delta_grid[1].z = (GRIDZ(grid_corner[2]) - GRIDZ(grid_corner[0])) / (L-1);
			}
			else {
				delta_grid[0].x = (p_pts[grid_corner[1]].x - p_pts[grid_corner[0]].x)/(K-1);
				delta_grid[0].y = (p_pts[grid_corner[1]].y - p_pts[grid_corner[0]].y)/(K-1);
				delta_grid[0].z = (p_pts[grid_corner[1]].z - p_pts[grid_corner[0]].z)/(K-1);
				delta_grid[1].x = (p_pts[grid_corner[2]].x - p_pts[grid_corner[0]].x)/(L-1);
				delta_grid[1].y = (p_pts[grid_corner[2]].y - p_pts[grid_corner[0]].y)/(L-1);
				delta_grid[1].z = (p_pts[grid_corner[2]].z - p_pts[grid_corner[0]].z)/(L-1);
			}

			/* Pixel dimensions in the 3D space. */
			delta_pixel[0].x = (delta_grid[0].x + delta_grid[1].x) / 2;
			delta_pixel[0].y = (delta_grid[0].y + delta_grid[1].y) / 2;
			delta_pixel[0].z = (delta_grid[0].z + delta_grid[1].z) / 2;
			delta_pixel[1].x = (delta_grid[0].x - delta_grid[1].x) / 2;
			delta_pixel[1].y = (delta_grid[0].y - delta_grid[1].y) / 2;
			delta_pixel[1].z = (delta_grid[0].z - delta_grid[1].z) / 2;

			i_image = 0;
			for(j = 0; j < L; j++) {
				double x_line_start, y_line_start, z_line_start;
				if(log_axes) {
					x_line_start = GRIDX(grid_corner[0]) + j * delta_grid[1].x;
					y_line_start = GRIDY(grid_corner[0]) + j * delta_grid[1].y;
					z_line_start = GRIDZ(grid_corner[0]) + j * delta_grid[1].z;
				}
				else {
					x_line_start = p_pts[grid_corner[0]].x + j * delta_grid[1].x;
					y_line_start = p_pts[grid_corner[0]].y + j * delta_grid[1].y;
					z_line_start = p_pts[grid_corner[0]].z + j * delta_grid[1].z;
				}
				for(i = 0; i < K; i++) {
					double x, y, z;
					bool view_in_pixel = false;
					int corners_in_view = 0;
					struct {double x; double y; double z; } p_corners[4]; // Parallelogram corners.
					int k;
					/* If terminal can't handle alpha, treat it as all-or-none. */
					if(pixel_planes == IC_RGBA) {
						if((p_pts[i_image].CRD_A == 0) || (p_pts[i_image].CRD_A < 128 &&  !(pT->flags & TERM_ALPHA_CHANNEL))) {
							i_image++;
							continue;
						}
					}
					x = x_line_start + i * delta_grid[0].x;
					y = y_line_start + i * delta_grid[0].y;
					z = z_line_start + i * delta_grid[0].z;

					p_corners[0].x = x + delta_pixel[0].x;
					p_corners[0].y = y + delta_pixel[0].y;
					p_corners[0].z = z + delta_pixel[0].z;
					p_corners[1].x = x + delta_pixel[1].x;
					p_corners[1].y = y + delta_pixel[1].y;
					p_corners[1].z = z + delta_pixel[1].z;
					p_corners[2].x = x - delta_pixel[0].x;
					p_corners[2].y = y - delta_pixel[0].y;
					p_corners[2].z = z - delta_pixel[0].z;
					p_corners[3].x = x - delta_pixel[1].x;
					p_corners[3].y = y - delta_pixel[1].y;
					p_corners[3].z = z - delta_pixel[1].z;
					// Check if any of the corners are viewable
					for(k = 0; k < 4; k++) {
						if(inrange(p_corners[k].x, view_port_x_.low, view_port_x_.upp) && inrange(p_corners[k].y, view_port_y_.low, view_port_y_.upp)
						    && (inrange(p_corners[k].z, view_port_z_.low, view_port_z_.upp) || !project_points || splot_map))
							corners_in_view++;
					}
					if(corners_in_view > 0 || view_in_pixel) {
						int N_corners = 0; /* Number of corners. */
						gpiPoint corners[5]; /* At most 5 corners. */
						corners[0].style = FS_DEFAULT;
						if(corners_in_view > 0) {
							int i_corners;
							N_corners = 4;
							for(i_corners = 0; i_corners < N_corners; i_corners++) {
								if(project_points) {
									Map3DXY(p_corners[i_corners].x, p_corners[i_corners].y, p_corners[i_corners].z, &x, &y);
									corners[i_corners].x = (int)x;
									corners[i_corners].y = (int)y;
								}
								else {
									if(log_axes) {
										corners[i_corners].x = MapX(LogValue(XAxis, p_corners[i_corners].x));
										corners[i_corners].y = MapY(LogValue(YAxis, p_corners[i_corners].y));
									}
									else {
										corners[i_corners].x = MapX(p_corners[i_corners].x);
										corners[i_corners].y = MapY(p_corners[i_corners].y);
									}
								}
								// Clip rectangle if necessary
								if(rectangular_image && pT->fillbox && corners_in_view < 4) {
									SETMAX(corners[i_corners].x, P_Clip->xleft);
									SETMIN(corners[i_corners].x, P_Clip->xright);
									SETMIN(corners[i_corners].y, P_Clip->ytop); // @? По моему, здесь должно быть SETMAX
									SETMAX(corners[i_corners].y, P_Clip->ybot); // @? По моему, здесь должно быть SETMIN
								}
							}
						}
						else {
							/* DJS FIXME:
							 * Could still be visible if any of the four corners of the view
							 *port are
							 * within the parallelogram formed by the pixel.  This is tricky
							 *geometry.
							 */
						}

						if(N_corners > 0) {
							if(pixel_planes == IC_PALETTE) {
								if((p_pts[i_image].type == UNDEFINED) ||  (fisnan(p_pts[i_image].CRD_COLOR))) {
									/* EAM April 2012 Distinguish +/-Inf from NaN */
									FPRINTF((stderr, "undefined pixel value %g\n", p_pts[i_image].CRD_COLOR));
									if(fisnan(p_pts[i_image].CRD_COLOR))
										goto skip_pixel;
								}
								pT->SetColor(CB2Gray(p_pts[i_image].CRD_COLOR) );
							}
							else {
								int r = (int)(CB2Gray(p_pts[i_image].CRD_R) * 255. + 0.5);
								int g = (int)(CB2Gray(p_pts[i_image].CRD_G) * 255. + 0.5);
								int b = (int)(CB2Gray(p_pts[i_image].CRD_B) * 255. + 0.5);
								int rgblt = (r << 16) + (g << 8) + b;
								SetRgbColorVar(pT, rgblt);
							}
							if(pixel_planes == IC_RGBA) {
								int alpha = (int)(p_pts[i_image].CRD_A * 100./255.);
								if(alpha == 0)
									goto skip_pixel;
								if(pT->flags & TERM_ALPHA_CHANNEL)
									corners[0].style = FS_TRANSPARENT_SOLID + (alpha<<4);
							}
							if(rectangular_image && pT->fillbox && !(pT->flags & TERM_POLYGON_PIXELS)) {
								// Some terminals (Canvas) can do filled rectangles
								// more efficiently than filled polygons.
								(*pT->fillbox)(corners[0].style, MIN(corners[0].x, corners[2].x),
								    MIN(corners[0].y, corners[2].y), abs(corners[2].x - corners[0].x), abs(corners[2].y - corners[0].y));
							}
							else {
								(*pT->filled_polygon)(N_corners, corners);
							}
						}
					}
skip_pixel:
					i_image++;
				}
			}
			pT->_Layer(TERM_LAYER_END_IMAGE);
		}
	}
}
//
// JITTER
//
//t_jitter jitter;

static int compare_xypoints(const void * arg1, const void * arg2)
{
	GpCoordinate const * p1 = (GpCoordinate const *)arg1;
	GpCoordinate const * p2 = (GpCoordinate const *)arg2;
	/* Primary sort is on x */
	/* FIXME: I'd like to treat x coords within jitter.x as equal, */
	/*        but the GpCoordinate system mismatch makes this hard.  */
	if(p1->x > p2->x)
		return (1);
	else if(p1->x < p2->x)
		return (-1);
	else if(p1->y > p2->y)
		return (1);
	else if(p1->y < p2->y)
		return (-1);
	else
		return (0);
}
/*
 * "set jitter overlap <ydelta> spread <factor>"
 * displaces overlapping points in a point plot.
 * The jittering algorithm is inspired by the beeswarm plot variant in R.
 */
//static double jdist(GpCoordinate * pi, GpCoordinate * pj)
double GpGadgets::JDist(const GpCoordinate * pi, const GpCoordinate * pj)
{
	int delx = MapX(pi->x) - MapX(pj->x);
	int dely = MapY(pi->y) - MapY(pj->y);
	return sqrt((double)(delx*delx + dely*dely));
}

//void jitter_points(CurvePoints * plot)
void GpGadgets::JitterPoints(CurvePoints * plot)
{
	int i, j;
	/* The "x" and "xscale" stored in jitter are really along y */
	double xjit, ygap;
	GpPosition yoverlap;
	yoverlap.x = 0;
	yoverlap.y = jitter.overlap.x;
	yoverlap.scaley = jitter.overlap.scalex;
	GpGg.MapPositionR(yoverlap, &xjit, &ygap, "jitter");
	/* Clear xhigh and yhigh, where we will later store the jitter offsets. */
	/* Store variable color temporarily in ylow so it is not lost by sorting. */
	for(i = 0; i < plot->p_count; i++) {
		if(plot->varcolor)
			plot->points[i].ylow = plot->varcolor[i];
		plot->points[i].xhigh = 0.0;
		plot->points[i].yhigh = 0.0;
	}
	qsort(plot->points, plot->p_count, sizeof(GpCoordinate), compare_xypoints);
	/* For each point, check whether subsequent points would overlap it. */
	/* If so, displace them in a fixed pattern */
	i = 0;
	while(i < plot->p_count - 1) {
		for(j = 1; i+j < plot->p_count; j++) {
			if(GpGg.JDist(&plot->points[i], &plot->points[i+j]) >= ygap)
				break;
			/* Displace point purely on x */
			xjit  = (j+1)/2 * jitter.spread * plot->lp_properties.p_size;
			if(jitter.limit > 0)
				while(xjit > jitter.limit)
					xjit -= jitter.limit;
			if((j & 01) != 0)
				xjit = -xjit;
			plot->points[i+j].xhigh = xjit;
			if(jitter.style == JITTER_SQUARE)
				plot->points[i+j].yhigh = plot->points[i].y - plot->points[i+j].y;
		}
		i += j;
	}
	/* Copy variable colors back to where the plotting code expects to find them */
	if(plot->varcolor)
		for(i = 0; i < plot->p_count; i++) {
			plot->varcolor[i] = plot->points[i].ylow;
			plot->points[i].ylow = plot->points[i].y;
		}
}
//
// process 'set jitter' command 
//
//void set_jitter()
void GpGadgets::SetJitter(GpCommand & rC)
{
	rC.CToken++;
	// Default overlap criterion 1 character (usually on y) 
	jitter.overlap.scalex = character;
	jitter.overlap.x = 1;
	jitter.spread = 1.0;
	jitter.limit = 0.0;
	jitter.style = JITTER_DEFAULT;
	while(!rC.EndOfCommand()) {
		if(rC.AlmostEq("over$lap")) {
			rC.CToken++;
			GetPositionDefault(rC, &jitter.overlap, character, 2);
		}
		else if(rC.Eq("spread")) {
			rC.CToken++;
			jitter.spread = rC.RealExpression();
			if(jitter.spread <= 0)
				jitter.spread = 1.0;
		}
		else if(rC.Eq("swarm")) {
			rC.CToken++;
			jitter.style = JITTER_SWARM;
		}
		else if(rC.Eq("square")) {
			rC.CToken++;
			jitter.style = JITTER_SQUARE;
		}
		else if(rC.Eq("wrap")) {
			rC.CToken++;
			jitter.limit = rC.RealExpression();
		}
		else
			IntErrorCurToken("unrecognized keyword");
	}
}
//
// process 'show jitter' command 
//
void GpGadgets::ShowJitter()
{
	if(jitter.spread <= 0) {
		fprintf(stderr, "\tno jitter\n");
	}
	else {
		fprintf(stderr, "\toverlap criterion  %g %s coords\n", jitter.overlap.x, coord_msg[jitter.overlap.scalex]);
		fprintf(stderr, "\tspread multiplier on x: %g\n", jitter.spread);
		if(jitter.limit > 0)
			fprintf(stderr, "\twrap at %g character widths\n", jitter.limit);
		fprintf(stderr, "\tstyle: %s\n", jitter.style == JITTER_SQUARE ? "square" : "swarm");
	}
}
//
// process 'unset jitter' command 
//
void GpGadgets::UnsetJitter()
{
	jitter.spread = 0;
}
//
// called by the save command 
//
void GpGadgets::SaveJitter(FILE * fp)
{
	if(jitter.spread <= 0)
		fprintf(fp, "unset jitter\n");
	else {
		fprintf(fp, "set jitter overlap %s%g", jitter.overlap.scalex == character ? "" : coord_msg[jitter.overlap.scalex], jitter.overlap.x);
		fprintf(fp, "  spread %g  wrap %g", jitter.spread, jitter.limit);
		fprintf(fp, jitter.style == JITTER_SQUARE ? " square\n" : "\n");
	}
}

