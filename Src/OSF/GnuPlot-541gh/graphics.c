// GNUPLOT - graphics.c 
// Copyright 1986 - 1993, 1998, 2004   Thomas Williams, Colin Kelley
// Daniel Sebald: added plot_image_or_update_axes() routine for images. (5 November 2003)
//
#include <gnuplot.h>
#pragma hdrstop

static int    compare_ypoints(SORTFUNC_ARGS arg1, SORTFUNC_ARGS arg2);
static int    histeps_compare(SORTFUNC_ARGS p1, SORTFUNC_ARGS p2);
// 
// for plotting error bars half the width of error bar tic mark
// 
#define ERRORBARTIC(terminalPtr) MAX(((terminalPtr)->TicH/2), 1)

//static bool boxplot_factor_sort_required; // used by compare_ypoints via q_sort from filter_boxplot 

// For tracking exit and re-entry of bounding curves that extend out of plot 
// these must match the bit values returned by clip_point(). 
#define LEFT_EDGE       1
#define RIGHT_EDGE      2
#define BOTTOM_EDGE     4
#define TOP_EDGE        8

//#define f_max(a, b) MAX((a), (b))
//#define f_min(a, b) MIN((a), (b))
#define samesign(a, b) ((sgn(a) * sgn(b)) >= 0) // True if a and b have the same sign or zero (positive or negative) 

//static void get_arrow(arrow_def * pArrow, double * pSx, double * pSy, double * pEx, double * pEy)
void GnuPlot::GetArrow(GpTermEntry * pTerm, arrow_def * pArrow, double * pSx, double * pSy, double * pEx, double * pEy)
{
	MapPositionDouble(pTerm, &pArrow->start, pSx, pSy, "arrow");
	if(pArrow->type == arrow_end_relative) {
		// different coordinate systems:
		// add the values in the drivers coordinate system.
		// For log scale: relative coordinate is factor 
		MapPositionR(pTerm, &pArrow->end, pEx, pEy, "arrow");
		*pEx += *pSx;
		*pEy += *pSy;
	}
	else if(pArrow->type == arrow_end_oriented) {
		double aspect = (double)pTerm->TicV / (double)pTerm->TicH;
		double radius;
#ifdef _WIN32
		if(sstreq(pTerm->name, "windows"))
			aspect = 1.;
#endif
		MapPositionR(pTerm, &pArrow->end, &radius, NULL, "arrow");
		*pEx = *pSx + cos(SMathConst::PiDiv180 * pArrow->angle) * radius;
		*pEy = *pSy + sin(SMathConst::PiDiv180 * pArrow->angle) * radius * aspect;
	}
	else
		MapPositionDouble(pTerm, &pArrow->end, pEx, pEy, "arrow");
}

//static void place_grid(GpTermEntry * pTerm, int layer)
void GnuPlot::PlaceGrid(GpTermEntry * pTerm, int layer)
{
	int save_lgrid = grid_lp.l_type;
	int save_mgrid = mgrid_lp.l_type;
	BoundingBox * clip_save = V.P_ClipArea;
	TermApplyLpProperties(pTerm, &Gg.border_lp);   /* border linetype */
	Gr.LargestPolarCircle = 0.0;
	// We used to go through this process only once, drawing both the grid lines
	// and the axis tic labels.  Now we allow for a separate pass that redraws only
	// the labels if the user has chosen "set tics front".
	// This guarantees that the axis tic labels lie on top of all grid lines.
	if(layer == LAYER_FOREGROUND)
		grid_lp.l_type = mgrid_lp.l_type = LT_NODRAW;
	if(!grid_tics_in_front || (layer == LAYER_FOREGROUND)) {
		// select first mapping 
		AxS.Idx_X = FIRST_X_AXIS;
		AxS.Idx_Y = FIRST_Y_AXIS;
		AxisOutputTics(pTerm, FIRST_Y_AXIS, &_Bry.ytic_x, FIRST_X_AXIS, &GnuPlot::YTick2DCallback); // label first y axis tics 
		AxisOutputTics(pTerm, FIRST_X_AXIS, &_Bry.xtic_y, FIRST_Y_AXIS, &GnuPlot::XTick2DCallback); // label first x axis tics 
		// select second mapping 
		AxS.Idx_X = SECOND_X_AXIS;
		AxS.Idx_Y = SECOND_Y_AXIS;
		AxisOutputTics(pTerm, SECOND_Y_AXIS, &_Bry.y2tic_x, SECOND_X_AXIS, &GnuPlot::YTick2DCallback);
		AxisOutputTics(pTerm, SECOND_X_AXIS, &_Bry.x2tic_y, SECOND_Y_AXIS, &GnuPlot::XTick2DCallback);
	}
	// select first mapping 
	AxS.Idx_X = FIRST_X_AXIS;
	AxS.Idx_Y = FIRST_Y_AXIS;
	// Sep 2018: polar grid is clipped to x/y range limits 
	V.P_ClipArea = &V.BbPlot;
	// POLAR GRID circles 
	if(AxS.__R().ticmode && (raxis || Gg.Polar)) {
		// Piggyback on the xtick2d_callback.  Avoid a call to the full    
		// axis_output_tics(), which wasn't really designed for this axis. 
		tic_start = MapiY(0); /* Always equivalent to tics on theta=0 axis */
		tic_mirror = tic_start; /* tic extends on both sides of theta=0 */
		tic_text = tic_start - pTerm->ChrV;
		rotate_tics = AxS.__R().tic_rotate;
		if(rotate_tics == 0)
			tic_hjust = CENTRE;
		else if(pTerm->text_angle(pTerm, rotate_tics))
			tic_hjust = (rotate_tics == TEXT_VERTICAL) ? RIGHT : LEFT;
		if(AxS.__R().manual_justify)
			tic_hjust = AxS.__R().tic_pos;
		tic_direction = 1;
		GenTics(pTerm, &AxS[POLAR_AXIS], &GnuPlot::XTick2DCallback);
		pTerm->text_angle(pTerm, 0);
	}
	// POLAR GRID radial lines 
	if(polar_grid_angle > 0) {
		double theta = 0.0;
		int ox = MapiX(0);
		int oy = MapiY(0);
		pTerm->layer(pTerm, TERM_LAYER_BEGIN_GRID);
		TermApplyLpProperties(pTerm, &grid_lp);
		if(Gr.LargestPolarCircle <= 0.0)
			Gr.LargestPolarCircle = PolarRadius(AxS.__R().max);
		for(theta = 0; theta < 6.29; theta += polar_grid_angle) {
			int x = MapiX(Gr.LargestPolarCircle * cos(theta));
			int y = MapiY(Gr.LargestPolarCircle * sin(theta));
			DrawClipLine(pTerm, ox, oy, x, y);
		}
		pTerm->layer(pTerm, TERM_LAYER_END_GRID);
	}
	// POLAR GRID tickmarks along the perimeter of the outer circle 
	if(AxS.Theta().ticmode) {
		TermApplyLpProperties(pTerm, &Gg.border_lp);
		if(Gr.LargestPolarCircle <= 0.0)
			Gr.LargestPolarCircle = PolarRadius(AxS.__R().max);
		copy_or_invent_formatstring(&AxS.Theta());
		GenTics(pTerm, &AxS.Theta(), &GnuPlot::TTickCallback);
		pTerm->text_angle(pTerm, 0);
	}
	// Restore the grid line types if we had turned them off to draw labels only 
	grid_lp.l_type = save_lgrid;
	mgrid_lp.l_type = save_mgrid;
	V.P_ClipArea = clip_save;
}

//static void place_arrows(int layer)
void GnuPlot::PlaceArrows(GpTermEntry * pTerm, int layer)
{
	BoundingBox * clip_save = V.P_ClipArea;
	// Allow arrows to run off the plot, so long as they are still on the canvas 
	V.P_ClipArea = (pTerm->flags & TERM_CAN_CLIP) ? NULL : &V.BbCanvas;
	for(arrow_def * this_arrow = Gg.P_FirstArrow; this_arrow != NULL; this_arrow = this_arrow->next) {
		double dsx = 0, dsy = 0, dex = 0, dey = 0;
		if(this_arrow->arrow_properties.layer != layer)
			continue;
		if(this_arrow->type == arrow_end_undefined)
			continue;
		GetArrow(pTerm, this_arrow, &dsx, &dsy, &dex, &dey);
		TermApplyLpProperties(pTerm, &this_arrow->arrow_properties.lp_properties);
		ApplyHeadProperties(pTerm, &this_arrow->arrow_properties);
		DrawClipArrow(pTerm, dsx, dsy, dex, dey, this_arrow->arrow_properties.head);
	}
	TermApplyLpProperties(pTerm, &Gg.border_lp);
	V.P_ClipArea = clip_save;
}
// 
// place_pixmaps() handles both 2D and 3D pixmaps
// NOTE: implemented via term->image(), not individual pixels
// 
//void place_pixmaps(int layer, int dimensions)
void GnuPlot::PlacePixmaps(GpTermEntry * pTerm, int layer, int dimensions)
{
	t_pixmap * pixmap;
	gpiPoint corner[4];
	int x, y, dx, dy;
	if(!pTerm->image)
		return;
	for(pixmap = Gg.P_PixmapListHead; pixmap; pixmap = pixmap->next) {
		if(layer != pixmap->layer)
			continue;
		// ignore zero-size pixmap from read failure 
		if(!pixmap->nrows || !pixmap->ncols)
			continue;
		// Allow a single backing pixmap behind multiple multiplot panels 
		if(layer == LAYER_BEHIND && multiplot_count > 1)
			continue;
		if(dimensions == 3)
			Map3DPosition(pTerm, &pixmap->pin, &x, &y, "pixmap");
		else
			MapPosition(pTerm, &pixmap->pin, &x, &y, "pixmap");
		// dx = dy = 0 means 1-to-1 representation of pixels 
		if(pixmap->extent.x == 0 && pixmap->extent.y == 0) {
			dx = static_cast<int>(pixmap->ncols * pTerm->tscale);
			dy = static_cast<int>(pixmap->ncols * pTerm->tscale);
		}
		else if(dimensions == 3) {
			Map3DPositionR(pTerm, &pixmap->extent, &dx, &dy, "pixmap");
			if(pixmap->extent.scalex == first_axes)
				dx = static_cast<int>(pixmap->extent.x * _3DBlk.radius_scaler);
			if(pixmap->extent.scaley == first_axes)
				dy = static_cast<int>(pixmap->extent.y * _3DBlk.radius_scaler);
		}
		else {
			double Dx, Dy;
			MapPositionR(pTerm, &pixmap->extent, &Dx, &Dy, "pixmap");
			dx = static_cast<int>(fabs(Dx));
			dy = static_cast<int>(fabs(Dy));
		}
		// default is to keep original aspect ratio 
		if(pixmap->extent.y == 0)
			dy = static_cast<int>(dx * (double)(pixmap->nrows) / (double)(pixmap->ncols));
		if(pixmap->extent.x == 0)
			dx = static_cast<int>(dy * (double)(pixmap->ncols) / (double)(pixmap->nrows));
		if(pixmap->center) {
			x -= dx/2;
			y -= dy/2;
		}
		corner[0].x = x;
		corner[0].y = y + dy;
		corner[1].x = x + dx;
		corner[1].y = y;
		corner[2].x = 0;        /* no clipping */
		corner[2].y = pTerm->MaxY;
		corner[3].x = pTerm->MaxX;
		corner[3].y = 0;
		// Check for horizontal named palette colorbox 
		if(!pixmap->filename && dx > dy*2)
			pTerm->image(pTerm, pixmap->nrows, pixmap->ncols, pixmap->image_data, corner, IC_RGBA);
		else
			pTerm->image(pTerm, pixmap->ncols, pixmap->nrows, pixmap->image_data, corner, IC_RGBA);
	}
}
//
// place_labels() handles both individual labels and 2D plot with labels
//
//static void place_labels(GpTermEntry * pTerm, text_label * listhead, int layer, bool clip)
void GnuPlot::PlaceLabels(GpTermEntry * pTerm, text_label * pListHead, int layer, bool clip)
{
	int x, y;
	pTerm->pointsize(pTerm, Gg.PointSize);
	// Hypertext labels? 
	// NB: currently svg is the only terminal that needs this extra step 
	if(layer == LAYER_PLOTLABELS && pListHead && pListHead->hypertext && pTerm->hypertext) {
		pTerm->hypertext(pTerm, TERM_HYPERTEXT_FONT, pListHead->font);
	}
	for(text_label * this_label = pListHead; this_label; this_label = this_label->next) {
		if(this_label->layer == layer) {
			if(layer == LAYER_PLOTLABELS) {
				x = MapiX(this_label->place.x);
				y = MapiY(this_label->place.y);
			}
			else
				MapPosition(pTerm, &this_label->place, &x, &y, "label");
			// Trap undefined values from e.g. nonlinear axis mapping 
			if(invalid_coordinate(x, y))
				continue;
			if(clip) {
				if(this_label->place.scalex == first_axes)
					if(!AxS[FIRST_X_AXIS].InRange(this_label->place.x))
						continue;
				if(this_label->place.scalex == second_axes)
					if(!AxS[SECOND_X_AXIS].InRange(this_label->place.x))
						continue;
				if(this_label->place.scaley == first_axes)
					if(!AxS[FIRST_Y_AXIS].InRange(this_label->place.y))
						continue;
				if(this_label->place.scaley == second_axes)
					if(!AxS[SECOND_Y_AXIS].InRange(this_label->place.y))
						continue;
			}
			WriteLabel(pTerm, x, y, this_label);
		}
	}
}

//void place_objects(GpObject * listhead, int layer, int dimensions)
void GnuPlot::PlaceObjects(GpTermEntry * pTerm, GpObject * pListHead, int layer, int dimensions)
{
	double x1, y1;
	int style;
	for(t_object * this_object = pListHead; this_object != NULL; this_object = this_object->next) {
		lp_style_type lpstyle;
		fill_style_type * fillstyle;
		if(this_object->layer != layer && this_object->layer != LAYER_FRONTBACK)
			continue;
		// Extract line and fill style, but don't apply it yet 
		lpstyle = this_object->lp_properties;
		if(this_object->fillstyle.fillstyle == FS_DEFAULT && this_object->object_type == OBJ_RECTANGLE)
			fillstyle = &Gg.default_rectangle.fillstyle;
		else
			fillstyle = &this_object->fillstyle;
		style = style_from_fill(fillstyle);
		TermApplyLpProperties(pTerm, &lpstyle);
		switch(this_object->object_type) {
			case OBJ_CIRCLE:
		    {
			    t_circle * e = &this_object->o.circle;
			    double radius;
			    BoundingBox * clip_save = V.P_ClipArea;
			    if(dimensions == 2) {
				    MapPositionDouble(pTerm, &e->center, &x1, &y1, "object");
				    MapPositionR(pTerm, &e->extent, &radius, NULL, "object");
			    }
			    else if(_3DBlk.splot_map) {
				    int junkw, junkh;
				    Map3DPositionDouble(pTerm, &e->center, &x1, &y1, "object");
				    Map3DPositionR(pTerm, &e->extent, &junkw, &junkh, "object");
				    radius = junkw;
			    }
			    else { // General 3D splot 
				    if(e->center.scalex == screen)
					    MapPositionDouble(pTerm, &e->center, &x1, &y1, "object");
				    else if(e->center.scalex == first_axes || e->center.scalex == polar_axes)
					    Map3DPositionDouble(pTerm, &e->center, &x1, &y1, "object");
				    else
					    break;
				    // radius must not change with rotation 
				    if(e->extent.scalex == first_axes) {
					    radius = e->extent.x * _3DBlk.radius_scaler;
				    }
				    else {
					    MapPositionR(pTerm, &e->extent, &radius, NULL, "object");
				    }
			    }
			    if((e->center.scalex == screen || e->center.scaley == screen) || (this_object->clip == OBJ_NOCLIP))
				    V.P_ClipArea = &V.BbCanvas;
			    DoArc(pTerm, (int)x1, (int)y1, radius, e->ArcR.low, e->ArcR.upp, style, FALSE);
			    // Retrace the border if the style requests it 
			    if(NeedFillBorder(pTerm, fillstyle))
				    DoArc(pTerm, (int)x1, (int)y1, radius, e->ArcR.low, e->ArcR.upp, 0, e->wedge);
			    V.P_ClipArea = clip_save;
			    break;
		    }
			case OBJ_ELLIPSE:
		    {
			    t_ellipse * e = &this_object->o.ellipse;
			    BoundingBox * clip_save = V.P_ClipArea;
			    if((e->center.scalex == screen || e->center.scaley == screen) || (this_object->clip == OBJ_NOCLIP))
				    V.P_ClipArea = &V.BbCanvas;
			    if(dimensions == 2)
				    DoEllipse(pTerm, 2, e, style, TRUE);
			    else if(_3DBlk.splot_map)
				    DoEllipse(pTerm, 3, e, style, TRUE);
			    else
				    break;
			    // Retrace the border if the style requests it 
			    if(NeedFillBorder(pTerm, fillstyle))
				    DoEllipse(pTerm, dimensions, e, 0, TRUE);
			    V.P_ClipArea = clip_save;
			    break;
		    }
			case OBJ_POLYGON:
		    {
			    /* Polygons have an extra option LAYER_FRONTBACK that matches
			     * FRONT or BACK depending on which way the polygon faces
			     */
			    int facing = LAYER_BEHIND; /* This will be ignored */
			    if(this_object->layer == LAYER_FRONTBACK) {
				    if(oneof2(layer, LAYER_FRONT, LAYER_BACK))
					    facing = layer;
				    else
					    break;
			    }
			    DoPolygon(pTerm, dimensions, this_object, style, facing);
			    // Retrace the border if the style requests it 
			    if(this_object->layer != LAYER_DEPTHORDER)
				    if(NeedFillBorder(pTerm, fillstyle))
					    DoPolygon(pTerm, dimensions, this_object, 0, facing);

			    break;
		    }
			case OBJ_RECTANGLE:
			    DoRectangle(pTerm, dimensions, this_object, fillstyle);
			    break;
			default:
			    break;
		} /* End switch(object_type) */
	}
}
// 
// Apply axis range expansions from "set offsets" command
// 
//static void adjust_offsets(void)
void GnuPlot::AdjustOffsets()
{
	const double b = (Gr.BOff.scaley == graph) ? fabs(AxS.__Y().GetRange()) * Gr.BOff.y : Gr.BOff.y;
	const double t = (Gr.TOff.scaley == graph) ? fabs(AxS.__Y().GetRange()) * Gr.TOff.y : Gr.TOff.y;
	const double l = (Gr.LOff.scalex == graph) ? fabs(AxS.__X().GetRange()) * Gr.LOff.x : Gr.LOff.x;
	const double r = (Gr.ROff.scalex == graph) ? fabs(AxS.__X().GetRange()) * Gr.ROff.x : Gr.ROff.x;
	if(Gr.RetainOffsets)
		Gr.RetainOffsets = false;
	else {
		if((AxS.__Y().autoscale & AUTOSCALE_BOTH) != AUTOSCALE_NONE) {
			if(AxS.__Y().IsNonLinear()) {
				AdjustNonlinearOffset(&AxS.__Y());
			}
			else {
				if(AxS.__Y().min < AxS.__Y().max) {
					AxS.__Y().min -= b;
					AxS.__Y().max += t;
				}
				else {
					AxS.__Y().max -= b;
					AxS.__Y().min += t;
				}
			}
		}
		if((AxS.__X().autoscale & AUTOSCALE_BOTH) != AUTOSCALE_NONE) {
			if(AxS.__X().IsNonLinear()) {
				AdjustNonlinearOffset(&AxS.__X());
			}
			else {
				if(AxS.__X().min < AxS.__X().max) {
					AxS.__X().min -= l;
					AxS.__X().max += r;
				}
				else {
					AxS.__X().max -= l;
					AxS.__X().min += r;
				}
			}
		}
		if(AxS.__X().min == AxS.__X().max)
			IntError(NO_CARET, "x_min should not equal x_max!");
		if(AxS.__Y().min == AxS.__Y().max)
			IntError(NO_CARET, "y_min should not equal y_max!");
		if(AxS[FIRST_X_AXIS].linked_to_secondary)
			CloneLinkedAxes(&AxS[FIRST_X_AXIS], &AxS[SECOND_X_AXIS]);
		if(AxS[FIRST_Y_AXIS].linked_to_secondary)
			CloneLinkedAxes(&AxS[FIRST_Y_AXIS], &AxS[SECOND_Y_AXIS]);
	}
}
//
// This routine is called only if we know the axis passed in is either
// nonlinear X or nonlinear Y.  We apply the offsets to the primary (linear)
// end of the linkage and then transform back to the axis itself (seconary).
//
//static void adjust_nonlinear_offset(GpAxis * pAxSecondary)
void GnuPlot::AdjustNonlinearOffset(GpAxis * pAxSecondary)
{
	GpAxis * p_ax_primary = pAxSecondary->linked_to_primary;
	double range = fabs(p_ax_primary->GetRange());
	double offset1, offset2;
	if(pAxSecondary->index == FIRST_X_AXIS) {
		if((Gr.LOff.scalex != graph && Gr.LOff.x != 0) || (Gr.ROff.scalex != graph && Gr.ROff.x != 0))
			IntError(NO_CARET, "nonlinear axis offsets must be in graph units");
		offset1 = Gr.LOff.x;
		offset2 = Gr.ROff.x;
	}
	else {
		if((Gr.BOff.scaley != graph && Gr.BOff.y != 0) ||  (Gr.TOff.scaley != graph && Gr.TOff.y != 0))
			IntError(NO_CARET, "nonlinear axis offsets must be in graph units");
		offset1 = Gr.BOff.y;
		offset2 = Gr.TOff.y;
	}
	p_ax_primary->min -= range * offset1;
	p_ax_primary->max += range * offset2;
	pAxSecondary->min = EvalLinkFunction(pAxSecondary, p_ax_primary->min);
	pAxSecondary->max = EvalLinkFunction(pAxSecondary, p_ax_primary->max);
}

//void do_plot(GpTermEntry * pTerm, curve_points * plots, int pcount)
void GnuPlot::DoPlot(GpTermEntry * pTerm, curve_points * plots, int pcount)
{
	int curve;
	curve_points * this_plot = NULL;
	bool key_pass = FALSE;
	legend_key * key = &Gg.KeyT;
	int previous_plot_style;
	AxS.Idx_X = FIRST_X_AXIS;
	AxS.Idx_Y = FIRST_Y_AXIS;
	AdjustOffsets();
	// EAM June 2003 - Although the comment below implies that font dimensions
	// are known after term_initialise(), this is not true at least for the X11
	// driver.  X11 fonts are not set until an actual display window is
	// opened, and that happens in term->graphics(), which is called from term_start_plot().
	TermInitialise(pTerm); // may set xmax/ymax 
	TermStartPlot(pTerm);
	// Figure out if we need a colorbox for this plot 
	SetPlotWithPalette(0, MODE_PLOT); /* EAM FIXME - 1st parameter is a dummy */
	// compute boundary for plot (V.BbPlot.xleft, V.BbPlot.xright, V.BbPlot.ytop, V.BbPlot.ybot)
	// also calculates tics, since xtics depend on V.BbPlot.xleft
	// but V.BbPlot.xleft depends on ytics. Boundary calculations depend
	// on term->ChrV etc, so terminal must be initialised first.
	Boundary(pTerm, plots, pcount);
	// Make palette 
	if(IsPlotWithPalette())
		MakePalette(pTerm);
	// Give a chance for background items to be behind everything else 
	PlacePixmaps(pTerm, LAYER_BEHIND, 2);
	PlaceObjects(pTerm, Gg.P_FirstObject, LAYER_BEHIND, 2);
	screen_ok = FALSE;
	(pTerm->layer)(pTerm, TERM_LAYER_BACKTEXT); // Sync point for epslatex text positioning 
	// DRAW TICS AND GRID 
	if(oneof2(grid_layer, LAYER_BACK, LAYER_BEHIND))
		PlaceGrid(pTerm, grid_layer);
	// DRAW ZERO AXES and update axis->term_zero 
	AxisDraw2DZeroAxis(pTerm, FIRST_X_AXIS, FIRST_Y_AXIS);
	AxisDraw2DZeroAxis(pTerm, FIRST_Y_AXIS, FIRST_X_AXIS);
	AxisDraw2DZeroAxis(pTerm, SECOND_X_AXIS, SECOND_Y_AXIS);
	AxisDraw2DZeroAxis(pTerm, SECOND_Y_AXIS, SECOND_X_AXIS);
	PlaceParallelAxes(pTerm, plots, LAYER_BACK); // DRAW VERTICAL AXES OF PARALLEL AXIS PLOTS 
	PlaceSpiderPlotAxes(pTerm, plots, LAYER_BACK); // DRAW RADIAL AXES OF SPIDERPLOTS 
	// DRAW PLOT BORDER 
	if(Gg.draw_border)
		PlotBorder(pTerm);
	// Add back colorbox if appropriate 
	if(IsPlotWithColorbox() && Gg.ColorBox.layer == LAYER_BACK)
		DrawColorSmoothBox(pTerm, MODE_PLOT);
	PlacePixmaps(pTerm, LAYER_BACK, 2); // Pixmaps before objects 
	PlaceObjects(pTerm, Gg.P_FirstObject, LAYER_BACK, 2); // Fixed objects 
	PlaceLabels(pTerm, Gg.P_FirstLabel, LAYER_BACK, FALSE); // PLACE LABELS 
	PlaceArrows(pTerm, LAYER_BACK); // PLACE ARROWS 
	(pTerm->layer)(pTerm, TERM_LAYER_FRONTTEXT); // Sync point for epslatex text positioning 
	// Draw axis labels and timestamps 
	// Note: As of Dec 2012 these are drawn as "front" text. 
	DrawTitles(pTerm);
	// Draw the key, or at least reserve space for it (pass 1) 
	if(key->visible)
		DrawKey(pTerm, key, key_pass);
SECOND_KEY_PASS:
	// This tells the canvas, qt, and svg terminals to restart the plot   
	// count so that key titles are in sync with the plots they describe. 
	(pTerm->layer)(pTerm, TERM_LAYER_RESET_PLOTNO);
	// DRAW CURVES 
	this_plot = plots;
	previous_plot_style = 0;
	for(curve = 0; curve < pcount; this_plot = this_plot->next, curve++) {
		bool localkey = key->visible; /* a local copy */
		this_plot->current_plotno = curve;
		// Sync point for start of new curve (used by svg, post, ...) 
		if(pTerm->hypertext) {
			const char * plaintext = this_plot->title_no_enhanced ? this_plot->title : estimate_plaintext(this_plot->title);
			(pTerm->hypertext)(pTerm, TERM_HYPERTEXT_TITLE, plaintext);
		}
		(pTerm->layer)(pTerm, TERM_LAYER_BEFORE_PLOT);
		// set scaling for this plot's axes 
		AxS.Idx_X = this_plot->AxIdx_X;
		AxS.Idx_Y = this_plot->AxIdx_Y;
		// Crazy corner case handling Bug #3499425 
		if(this_plot->plot_style == HISTOGRAMS)
			if((!key_pass && key->front) && Gg.PreferLineStyles) {
				lp_style_type ls;
				LpUseProperties(pTerm, &ls, this_plot->lp_properties.l_type+1);
				this_plot->lp_properties.pm3d_color = ls.pm3d_color;
			}
		TermApplyLpProperties(pTerm, &(this_plot->lp_properties));
		// Skip a line in the key between histogram clusters 
		if(this_plot->plot_style == HISTOGRAMS && previous_plot_style == HISTOGRAMS && this_plot->histogram_sequence == 0 && !_Bry.AtLeftOfKey()) {
			_Bry.key_count++;
			AdvanceKey(true); // correct for inverted key 
			AdvanceKey(false);
		}
		// Column-stacked histograms store their key titles internally 
		if(this_plot->plot_style == HISTOGRAMS && Gg.histogram_opts.type == HT_STACKED_IN_TOWERS) {
			text_label * key_entry;
			localkey = 0;
			if(this_plot->labels && (key_pass || !key->front)) {
				lp_style_type save_lp = this_plot->lp_properties;
				for(key_entry = this_plot->labels->next; key_entry; key_entry = key_entry->next) {
					int histogram_linetype = key_entry->tag + this_plot->histogram->startcolor;
					this_plot->lp_properties.l_type = histogram_linetype;
					this_plot->fill_properties.fillpattern = histogram_linetype;
					if(key_entry->text) {
						if(Gg.PreferLineStyles)
							LpUseProperties(pTerm, &this_plot->lp_properties, histogram_linetype);
						else
							LoadLineType(pTerm, &this_plot->lp_properties, histogram_linetype);
						DoKeySample(pTerm, this_plot, key, key_entry->text, 0.0);
					}
					_Bry.key_count++;
					AdvanceKey(false);
				}
				free_labels(this_plot->labels);
				this_plot->labels = NULL;
				this_plot->lp_properties = save_lp;
			}
			// Parallel plot titles are placed as xtic labels 
		}
		else if(this_plot->plot_style == PARALLELPLOT) {
			localkey = FALSE;
			// Spiderplot key samples are handled in plot_spiderplot 
		}
		else if(this_plot->plot_style == SPIDERPLOT && !(this_plot->plot_type == KEYENTRY))
			localkey = FALSE;
		else if(this_plot->title && !*this_plot->title)
			localkey = FALSE;
		else if(this_plot->plot_type == NODATA)
			localkey = FALSE;
		else if(key_pass || !key->front) {
			ignore_enhanced(this_plot->title_no_enhanced);
			// don't write filename or function enhanced 
			if(localkey && this_plot->title && !this_plot->title_is_suppressed) {
				// If title is "at {end|beg}" do not draw it in the key 
				if(!this_plot->title_position ||  this_plot->title_position->scalex != character) {
					coordval var_color;
					_Bry.key_count++;
					AdvanceKey(true); // invert only 
					var_color = (this_plot->varcolor) ? this_plot->varcolor[0] : 0.0;
					DoKeySample(pTerm, this_plot, key, this_plot->title, var_color);
				}
			}
			ignore_enhanced(FALSE);
		}
		// If any plots have opted out of autoscaling, we need to recheck 
		// whether their points are INRANGE or not.                       
		if(this_plot->noautoscale && !key_pass)
			RecheckRanges(this_plot);
		// and now the curves, plus any special key requirements 
		// be sure to draw all lines before drawing any points 
		// Skip missing/empty curves 
		if(this_plot->plot_type != NODATA && !key_pass) {
			switch(this_plot->plot_style) {
				case IMPULSES: PlotImpulses(pTerm, this_plot, AxS.__X().term_zero, AxS.__Y().term_zero); break;
				case LINES: PlotLines(pTerm, this_plot); break;
				case STEPS: 
				case FILLSTEPS: PlotSteps(pTerm, this_plot); break;
				case FSTEPS: PlotFSteps(pTerm, this_plot); break;
				case HISTEPS: PlotHiSteps(pTerm, this_plot); break;
				case POINTSTYLE: PlotPoints(pTerm, this_plot); break;
				case LINESPOINTS:
				    PlotLines(pTerm, this_plot);
				    PlotPoints(pTerm, this_plot);
				    break;
				case DOTS: PlotDots(pTerm, this_plot); break;
				case YERRORLINES:
				case XERRORLINES:
				case XYERRORLINES:
				    PlotLines(pTerm, this_plot);
				    PlotBars(pTerm, this_plot);
				    PlotPoints(pTerm, this_plot);
				    break;
				case YERRORBARS:
				case XERRORBARS:
				case XYERRORBARS:
				    PlotBars(pTerm, this_plot);
				    PlotPoints(pTerm, this_plot);
				    break;
				case BOXXYERROR:
				case BOXES: PlotBoxes(pTerm, this_plot, AxS.__Y().term_zero); break;
				case HISTOGRAMS:
				    if(Gr.BarLayer == LAYER_FRONT)
					    PlotBoxes(pTerm, this_plot, AxS.__Y().term_zero);
				    // Draw the bars first, so that the box will cover the bottom half 
				    if(Gg.histogram_opts.type == HT_ERRORBARS) {
					    // Note that the bar linewidth may not match the border or plot linewidth 
					    pTerm->linewidth(pTerm, Gg.histogram_opts.bar_lw);
					    if(!NeedFillBorder(pTerm, &Gg.default_fillstyle))
						    pTerm->linetype(pTerm, this_plot->lp_properties.l_type);
					    PlotBars(pTerm, this_plot);
					    TermApplyLpProperties(pTerm, &(this_plot->lp_properties));
				    }
				    if(Gr.BarLayer != LAYER_FRONT)
					    PlotBoxes(pTerm, this_plot, AxS.__Y().term_zero);
				    break;
				case BOXERROR:
				    if(Gr.BarLayer != LAYER_FRONT)
					    PlotBars(pTerm, this_plot);
				    PlotBoxes(pTerm, this_plot, AxS.__Y().term_zero);
				    if(Gr.BarLayer == LAYER_FRONT)
					    PlotBars(pTerm, this_plot);
				    break;
				case FILLEDCURVES:
				case POLYGONS:
				    if(this_plot->filledcurves_options.closeto == FILLEDCURVES_DEFAULT) {
					    if(this_plot->plot_type == DATA)
						    memcpy(&this_plot->filledcurves_options, &Gg.filledcurves_opts_data, sizeof(filledcurves_opts));
					    else
						    memcpy(&this_plot->filledcurves_options, &Gg.filledcurves_opts_func, sizeof(filledcurves_opts));
				    }
				    if(oneof3(this_plot->filledcurves_options.closeto, FILLEDCURVES_BETWEEN, FILLEDCURVES_ABOVE, FILLEDCURVES_BELOW)) {
					    PlotBetweenCurves(pTerm, this_plot);
				    }
				    else if(!this_plot->plot_smooth && oneof3(this_plot->filledcurves_options.closeto, FILLEDCURVES_ATY1, FILLEDCURVES_ATY2, FILLEDCURVES_ATR)) {
					    /* Smoothing may have trashed the original contents	*/
					    /* of the 2nd y data column, so piggybacking on the	*/
					    /* code for FILLEDCURVES_BETWEEN will not work.	*/
					    /* FIXME: Maybe piggybacking is always a bad idea?		*/
					    /* IIRC the original rationale was to get better clipping	*/
					    /* but the general polygon clipping code should now work.	*/
					    PlotBetweenCurves(pTerm, this_plot);
				    }
				    else
					    PlotFilledCurves(pTerm, this_plot);
				    break;
				case VECTOR:
				case ARROWS: PlotVectors(pTerm, this_plot); break;
				case FINANCEBARS: PlotFBars(pTerm, this_plot); break;
				case CANDLESTICKS: PlotCBars(pTerm, this_plot); break;
				case BOXPLOT: PlotBoxPlot(pTerm, this_plot, FALSE); break;
				case PM3DSURFACE:
				case SURFACEGRID: IntWarn(NO_CARET, "Can't use pm3d or surface for 2d plots"); break;
				case LABELPOINTS: PlaceLabels(pTerm, this_plot->labels->next, LAYER_PLOTLABELS, TRUE); break;
				case IMAGE:
				    this_plot->image_properties.type = IC_PALETTE;
				    ProcessImage(pTerm, this_plot, IMG_PLOT);
				    break;
				case RGBIMAGE:
				    this_plot->image_properties.type = IC_RGB;
				    ProcessImage(pTerm, this_plot, IMG_PLOT);
				    break;
				case RGBA_IMAGE:
				    this_plot->image_properties.type = IC_RGBA;
				    ProcessImage(pTerm, this_plot, IMG_PLOT);
				    break;
				case CIRCLES: PlotCircles(pTerm, this_plot); break;
				case ELLIPSES: PlotEllipses(pTerm, this_plot); break;
				case PARALLELPLOT: PlotParallel(pTerm, this_plot); break;
				case SPIDERPLOT: PlotSpiderPlot(pTerm, this_plot); break;
				default: IntError(NO_CARET, "unknown plot style");
			}
		}
		// If there are two passes, defer key sample till the second 
		// KEY SAMPLES 
		if(key->front && !key_pass)
			;
		else if(localkey && this_plot->title && !this_plot->title_is_suppressed) {
			// we deferred point sample until now */
			if(this_plot->plot_style & PLOT_STYLE_HAS_POINT)
				DoKeySamplePoint(pTerm, this_plot, key);
			if(this_plot->plot_style == LABELPOINTS)
				DoKeySamplePoint(pTerm, this_plot, key);
			if(this_plot->plot_style == DOTS)
				DoKeySamplePoint(pTerm, this_plot, key);
			if(!this_plot->title_position)
				AdvanceKey(false);
		}
		// Option to label the end of the curve on the plot itself 
		if(this_plot->title_position && this_plot->title_position->scalex == character)
			AttachTitleToPlot(pTerm, this_plot, key);
		// Sync point for end of this curve (used by svg, post, ...) 
		(pTerm->layer)(pTerm, TERM_LAYER_AFTER_PLOT);
		previous_plot_style = this_plot->plot_style;
	}
	// Go back and draw the legend in a separate pass if necessary 
	if(key->visible && key->front && !key_pass) {
		key_pass = TRUE;
		DrawKey(pTerm, key, key_pass);
		goto SECOND_KEY_PASS;
	}
	// DRAW TICS AND GRID 
	if(grid_layer == LAYER_FRONT)
		PlaceGrid(pTerm, grid_layer);
	if(raxis)
		PlaceRAxis(pTerm);
	// Redraw the axis tic labels and tic marks if "set tics front" 
	if(grid_tics_in_front)
		PlaceGrid(pTerm, LAYER_FOREGROUND);
	// DRAW ZERO AXES 
	// redraw after grid so that axes linetypes are on top 
	if(grid_layer == LAYER_FRONT) {
		AxisDraw2DZeroAxis(pTerm, FIRST_X_AXIS, FIRST_Y_AXIS);
		AxisDraw2DZeroAxis(pTerm, FIRST_Y_AXIS, FIRST_X_AXIS);
		AxisDraw2DZeroAxis(pTerm, SECOND_X_AXIS, SECOND_Y_AXIS);
		AxisDraw2DZeroAxis(pTerm, SECOND_Y_AXIS, SECOND_X_AXIS);
	}
	// DRAW VERTICAL AXES OF PARALLEL AXIS PLOTS 
	if(Gg.ParallelAxisStyle.layer == LAYER_FRONT)
		PlaceParallelAxes(pTerm, plots, LAYER_FRONT);
	// DRAW RADIAL AXES OF SPIDERPLOTS 
	if(Gg.ParallelAxisStyle.layer == LAYER_FRONT)
		PlaceSpiderPlotAxes(pTerm, plots, LAYER_FRONT);
	// REDRAW PLOT BORDER 
	if(Gg.draw_border && Gg.border_layer == LAYER_FRONT)
		PlotBorder(pTerm);
	// Add front colorbox if appropriate 
	if(IsPlotWithColorbox() && Gg.ColorBox.layer == LAYER_FRONT)
		DrawColorSmoothBox(pTerm, MODE_PLOT);
	PlacePixmaps(pTerm, LAYER_FRONT, 2); // pixmaps in behind rectangles to enable rectangle as border 
	PlaceObjects(pTerm, Gg.P_FirstObject, LAYER_FRONT, 2); // And rectangles 
	PlaceLabels(pTerm, Gg.P_FirstLabel, LAYER_FRONT, FALSE); // PLACE LABELS 
	PlaceHistogramTitles(pTerm); // PLACE HISTOGRAM TITLES 
	PlaceArrows(pTerm, LAYER_FRONT); // PLACE ARROWS 
	PlaceTitle(pTerm, _Bry.TitlePos.x, _Bry.TitlePos.y); // PLACE TITLE LAST 
	// Release the palette if we have used one (PostScript only?) 
	if(IsPlotWithPalette() && pTerm->previous_palette)
		pTerm->previous_palette(pTerm);
	TermEndPlot(pTerm);
}
//
// Plots marked "noautoscale" do not yet have INRANGE/OUTRANGE flags set.
//
//static void recheck_ranges(curve_points * pPlot)
void GnuPlot::RecheckRanges(curve_points * pPlot)
{
	for(int i = 0; i < pPlot->p_count; i++) {
		if(pPlot->noautoscale && pPlot->points[i].type != UNDEFINED) {
			pPlot->points[i].type = INRANGE;
			if(!AxS[pPlot->AxIdx_X].InRange(pPlot->points[i].x))
				pPlot->points[i].type = OUTRANGE;
			if(!AxS[pPlot->AxIdx_Y].InRange(pPlot->points[i].y))
				pPlot->points[i].type = OUTRANGE;
		}
	}
}
// 
// plot_impulses:
// Plot the curves in IMPULSES style
// Mar 2017 - Apply "set jitter" to x coordinate of impulses
//
//static void plot_impulses(GpTermEntry * pTerm, curve_points * pPlot, int yaxis_x, int xaxis_y)
void GnuPlot::PlotImpulses(GpTermEntry * pTerm, curve_points * pPlot, int yaxis_x, int xaxis_y)
{
	int x, y;
	// Displace overlapping impulses if "set jitter" is in effect.
	// This operation loads jitter offsets into xhigh and yhigh.
	if(jitter.spread > 0)
		JitterPoints(pTerm, pPlot);
	for(int i = 0; i < pPlot->p_count; i++) {
		if(pPlot->points[i].type == UNDEFINED)
			continue;
		if(!Gg.Polar && !AxS.__X().InRange(pPlot->points[i].x))
			continue;
		// This catches points that are outside trange[theta_min:theta_max] 
		if(Gg.Polar && (pPlot->points[i].type == EXCLUDEDRANGE))
			continue;
		x = MapiX(pPlot->points[i].x);
		y = MapiY(pPlot->points[i].y);
		// The jitter x offset is a scaled multiple of character width. 
		if(!Gg.Polar && jitter.spread > 0)
			x += pPlot->points[i].CRD_XJITTER * 0.3 * pTerm->ChrH;
		if(invalid_coordinate(x, y))
			continue;
		CheckForVariableColor(pTerm, pPlot, &pPlot->varcolor[i]);
		if(Gg.Polar)
			DrawClipLine(pTerm, yaxis_x, xaxis_y, x, y);
		else
			DrawClipLine(pTerm, x, xaxis_y, x, y);
	}
}
// 
// plot_lines:
// Plot the curves in LINES style
// 
//static void plot_lines(GpTermEntry * pTerm, curve_points * plot)
void GnuPlot::PlotLines(GpTermEntry * pTerm, curve_points * pPlot)
{
	if(pPlot->lp_properties.l_type != LT_NODRAW) { // If all the lines are invisible, don't bother to draw them 
		int x = 0, y = 0; // current point in terminal coordinates 
		enum coord_type prev = UNDEFINED; // type of previous point 
		double xprev = 0.0;
		double yprev = 0.0;
		for(int i = 0; i < pPlot->p_count; i++) {
			double xnow = pPlot->points[i].x;
			double ynow = pPlot->points[i].y;
			CheckForVariableColor(pTerm, pPlot, &pPlot->varcolor[i]); // rgb variable  -  color read from data column 
			// Only map and pPlot the point if it is well-behaved (not UNDEFINED).
			// Note that map_x or map_y can hit NaN during eval_link_function(),
			// in which case the coordinate value is garbage so we set UNDEFINED.
			if(pPlot->points[i].type != UNDEFINED) {
				x = MapiX(xnow);
				y = MapiY(ynow);
				if(invalid_coordinate(x, y))
					pPlot->points[i].type = UNDEFINED;
			}
			switch(pPlot->points[i].type) {
				case INRANGE:
					if(prev == INRANGE) {
						pTerm->vector(pTerm, x, y);
					}
					else if(prev == OUTRANGE) {
						// from outrange to inrange 
						if(!Gg.ClipLines1) {
							pTerm->move(pTerm, x, y);
						}
						else if(Gg.Polar && Gg.ClipRadial) {
							DrawPolarClipLine(pTerm, xprev, yprev, xnow, ynow);
						}
						else {
							if(!DrawClipLine(pTerm, MapiX(xprev), MapiY(yprev), x, y))
								// This is needed if clip_line() doesn't agree that
								// the current point is INRANGE, i.e. it is on the
								//  border or just outside depending on rounding
								pTerm->move(pTerm, x, y);
						}
					}
					else { /* prev == UNDEFINED */
						pTerm->move(pTerm, x, y);
						pTerm->vector(pTerm, x, y);
					}
					break;
				case OUTRANGE:
					if(prev == INRANGE) {
						// from inrange to outrange 
						if(Gg.ClipLines1) {
							if(Gg.Polar && Gg.ClipRadial)
								DrawPolarClipLine(pTerm, xprev, yprev, xnow, ynow);
							else
								DrawClipLine(pTerm, MapiX(xprev), MapiY(yprev), x, y);
						}
					}
					else if(prev == OUTRANGE) {
						// from outrange to outrange 
						if(Gg.ClipLines2) {
							if(Gg.Polar && Gg.ClipRadial)
								DrawPolarClipLine(pTerm, xprev, yprev, xnow, ynow);
							else
								DrawClipLine(pTerm, MapiX(xprev), MapiY(yprev), x, y);
						}
					}
					break;
				case UNDEFINED:
				default: // just a safety 
					break;
			}
			prev = pPlot->points[i].type;
			xprev = xnow;
			yprev = ynow;
		}
	}
}

/* plot_filledcurves:
 *        {closed | {above | below} {x1 | x2 | y1 | y2 | r}[=<a>] | xy=<x>,<y>}
 */
//
// finalize and draw the filled curve 
//
//static void finish_filled_curve(GpTermEntry * pTerm, int points, gpiPoint * pCorners, curve_points * pPlot)
void GnuPlot::FinishFilledCurve(GpTermEntry * pTerm, int points, gpiPoint * pCorners, curve_points * pPlot)
{
	static gpiPoint * clipcorners = NULL;
	int clippoints;
	filledcurves_opts * filledcurves_options = &pPlot->filledcurves_options;
	long side = 0;
	if(points > 0) {
		// add side (closing) points 
		switch(filledcurves_options->closeto) {
			case FILLEDCURVES_CLOSED:
				break;
			case FILLEDCURVES_X1:
				pCorners[points].x   = pCorners[points-1].x;
				pCorners[points+1].x = pCorners[0].x;
				pCorners[points].y   = pCorners[points+1].y = AxS[FIRST_Y_AXIS].term_lower;
				points += 2;
				break;
			case FILLEDCURVES_X2:
				pCorners[points].x   = pCorners[points-1].x;
				pCorners[points+1].x = pCorners[0].x;
				pCorners[points].y   = pCorners[points+1].y = AxS[FIRST_Y_AXIS].term_upper;
				points += 2;
				break;
			case FILLEDCURVES_Y1:
				pCorners[points].y   = pCorners[points-1].y;
				pCorners[points+1].y = pCorners[0].y;
				pCorners[points].x   = pCorners[points+1].x = AxS[FIRST_X_AXIS].term_lower;
				points += 2;
				break;
			case FILLEDCURVES_Y2:
				pCorners[points].y   = pCorners[points-1].y;
				pCorners[points+1].y = pCorners[0].y;
				pCorners[points].x   = pCorners[points+1].x = AxS[FIRST_X_AXIS].term_upper;
				points += 2;
				break;
			case FILLEDCURVES_ATX1:
			case FILLEDCURVES_ATX2:
				{
					pCorners[points].x   = pCorners[points+1].x = MapiX(filledcurves_options->at);
					// should be mapping real x1/x2axis/graph/screen => screen 
					pCorners[points].y   = pCorners[points-1].y;
					pCorners[points+1].y = pCorners[0].y;
					for(int i = 0; i < points; i++)
						side += pCorners[i].x - pCorners[points].x;
					points += 2;
				}
				break;
			case FILLEDCURVES_ATXY:
				pCorners[points].x = MapiX(filledcurves_options->at);
				// should be mapping real x1axis/graph/screen => screen 
				pCorners[points].y = MapiY(filledcurves_options->aty);
				// should be mapping real y1axis/graph/screen => screen 
				points++;
				break;
			case FILLEDCURVES_ATY1:
			case FILLEDCURVES_ATY2:
				pCorners[points].y = MapiY(filledcurves_options->at);
				pCorners[points+1].y = pCorners[points].y;
				pCorners[points].x = pCorners[points-1].x;
				pCorners[points+1].x = pCorners[0].x;
				points += 2;
			// Fall through 
			case FILLEDCURVES_BETWEEN:
				// fill_between() allocated an extra point for the above/below flag 
				if(filledcurves_options->closeto == FILLEDCURVES_BETWEEN)
					side = (pCorners[points].x > 0) ? 1 : -1;
			// Fall through 
			case FILLEDCURVES_ATR:
				// Prevent 1-pixel overlap of component rectangles, which 
				// causes vertical stripe artifacts for transparent fill  
				if(pPlot->fill_properties.fillstyle == FS_TRANSPARENT_SOLID) {
					int direction = (pCorners[2].x < pCorners[0].x) ? -1 : 1;
					if(points >= 4 && pCorners[2].x == pCorners[3].x) {
						pCorners[2].x -= direction, pCorners[3].x -= direction;
					}
					else if(points >= 5 && pCorners[3].x == pCorners[4].x) {
						pCorners[3].x -= direction, pCorners[4].x -= direction;
					}
				}
				break;
			default: // the polygon is closed by default 
				break;
		}
		// Check for request to fill only on one side of a bounding line 
		if(filledcurves_options->oneside > 0 && side < 0)
			return;
		if(filledcurves_options->oneside < 0 && side > 0)
			return;
		// EAM Apr 2013 - Use new polygon clipping code 
		clipcorners = (gpiPoint *)SAlloc::R(clipcorners, 2*points*sizeof(gpiPoint));
		V.ClipPolygon(pCorners, clipcorners, points, &clippoints);
		clipcorners->style = style_from_fill(&pPlot->fill_properties);
		if(clippoints > 0)
			pTerm->filled_polygon(pTerm, clippoints, clipcorners);
	}
}

//static void plot_filledcurves(GpTermEntry * pTerm, curve_points * plot)
void GnuPlot::PlotFilledCurves(GpTermEntry * pTerm, curve_points * pPlot)
{
	int x, y;               /* point in terminal coordinates */
	enum coord_type prev = UNDEFINED; /* type of previous point */
	int points = 0;                 /* how many corners */
	static gpiPoint * corners = 0;  /* array of corners */
	static int corners_allocated = 0; /* how many allocated */
	if(!pTerm->filled_polygon) { // filled polygons are not available 
		PlotLines(pTerm, pPlot);
	}
	else {
		// clip the "at" coordinate to the drawing area 
		switch(pPlot->filledcurves_options.closeto) {
			case FILLEDCURVES_ATX1:
				pPlot->filledcurves_options.at = AxS[FIRST_X_AXIS].ClipToRange(pPlot->filledcurves_options.at);
				break;
			case FILLEDCURVES_ATX2:
				pPlot->filledcurves_options.at = AxS[SECOND_X_AXIS].ClipToRange(pPlot->filledcurves_options.at);
				break;
			case FILLEDCURVES_ATY1:
			case FILLEDCURVES_ATY2:
				pPlot->filledcurves_options.at = AxS[pPlot->AxIdx_Y].ClipToRange(pPlot->filledcurves_options.at);
				break;
			case FILLEDCURVES_ATXY:
				pPlot->filledcurves_options.at = AxS[FIRST_X_AXIS].ClipToRange(pPlot->filledcurves_options.at);
				pPlot->filledcurves_options.aty = AxS[FIRST_Y_AXIS].ClipToRange(pPlot->filledcurves_options.aty);
				break;
			default:
				break;
		}
		for(int i = 0; i < pPlot->p_count; i++) {
			if(points+2 >= corners_allocated) { /* there are 2 side points */
				corners_allocated += 128; /* reallocate more corners */
				corners = (gpiPoint *)SAlloc::R(corners, corners_allocated*sizeof(gpiPoint));
			}
			switch(pPlot->points[i].type) {
				case INRANGE:
				case OUTRANGE:
					x = MapiX(pPlot->points[i].x);
					y = MapiY(pPlot->points[i].y);
					corners[points].x = x;
					corners[points].y = y;
					if(points == 0)
						CheckForVariableColor(pTerm, pPlot, &pPlot->varcolor[i]);
					points++;
					break;
				case UNDEFINED:
					// UNDEFINED flags a blank line in the input file.
					// Unfortunately, it can also mean that the point was undefined.
					// Is there a clean way to detect or handle the latter case?
					if(prev != UNDEFINED) {
						FinishFilledCurve(pTerm, points, corners, pPlot);
						points = 0;
					}
					break;
				default: /* just a safety */
					break;
			}
			prev = pPlot->points[i].type;
		}
		FinishFilledCurve(pTerm, points, corners, pPlot);
		// If the fill style has a border and this is a closed curve then	
		// retrace the boundary.  Otherwise ignore "border" property.	
		if(pPlot->filledcurves_options.closeto == FILLEDCURVES_CLOSED && NeedFillBorder(pTerm, &pPlot->fill_properties)) {
			PlotLines(pTerm, pPlot);
		}
	}
}
// 
// Fill the area between two curves
// 
//static void plot_betweencurves(GpTermEntry * pTerm, curve_points * plot)
void GnuPlot::PlotBetweenCurves(GpTermEntry * pTerm, curve_points * plot)
{
	double x1, x2, yl1, yu1, yl2, yu2, dy;
	double xmid = 0, ymid = 0;
	double xu1, xu2; // For polar plots 
	int i, j, istart = 0, finish = 0, points = 0, max_corners_needed;
	static gpiPoint * corners = 0;
	static int corners_allocated = 0;
	// If terminal doesn't support filled polygons, approximate with bars 
	if(!pTerm->filled_polygon) {
		PlotBars(pTerm, plot);
	}
	else {
		// Jan 2015: We are now using the plot_between code to also handle option
		// y=atval, but the style option in the plot header does not reflect this.
		// Change it here so that finish_filled_curve() doesn't get confused.
		plot->filledcurves_options.closeto = FILLEDCURVES_BETWEEN;
		// there are possibly 2 side points plus one extra to specify above/below 
		max_corners_needed = plot->p_count * 2 + 3;
		if(max_corners_needed > corners_allocated) {
			corners_allocated = max_corners_needed;
			corners = (gpiPoint *)SAlloc::R(corners, corners_allocated*sizeof(gpiPoint));
		}
		// 
		// Form a polygon, first forward along the lower points
		//   and then backward along the upper ones.
		// Check each interval to see if the curves cross.
		// If so, split the polygon into multiple parts.
		// 
		for(i = 0; i < plot->p_count; i++) {
			// This isn't really testing for undefined points, it is looking 
			// for blank lines. If there is one then start a new fill area.  
			if(plot->points[i].type == UNDEFINED)
				continue;
			if(points == 0) {
				istart = i;
				dy = 0.0;
			}
			if(finish == 2) { // start the polygon at the previously-found crossing 
				corners[points].x = MapiX(xmid);
				corners[points].y = MapiY(ymid);
				points++;
			}
			x1  = plot->points[i].x;
			xu1 = plot->points[i].xhigh;
			yl1 = plot->points[i].y;
			yu1 = plot->points[i].yhigh;
			if(i+1 >= plot->p_count || plot->points[i+1].type == UNDEFINED)
				finish = 1;
			else {
				finish = 0;
				x2  = plot->points[i+1].x;
				xu2 = plot->points[i+1].xhigh;
				yl2 = plot->points[i+1].y;
				yu2 = plot->points[i+1].yhigh;
			}
			corners[points].x = MapiX(x1);
			corners[points].y = MapiY(yl1);
			points++;
			if(Gg.Polar) {
				double ox = MapiX(0);
				double oy = MapiY(0);
				double plx = MapiX(plot->points[istart].x);
				double ply = MapiY(plot->points[istart].y);
				double pux = MapiX(plot->points[istart].xhigh);
				double puy = MapiY(plot->points[istart].yhigh);
				double drl = (plx-ox)*(plx-ox) + (ply-oy)*(ply-oy);
				double dru = (pux-ox)*(pux-ox) + (puy-oy)*(puy-oy);
				dy += dru-drl;
			}
			else {
				dy += yu1-yl1;
			}
			if(!finish) {
				// EAM 19-July-2007  Special case for polar plots. 
				if(Gg.Polar) {
					// Find intersection of the two lines.
					// Probably could use this code in the general case too.
					double A = (yl2-yl1) / (x2-x1);
					double C = (yu2-yu1) / (xu2-xu1);
					double b = yl1 - x1 * A;
					double d = yu1 - xu1 * C;
					xmid = (d-b) / (A-C);
					ymid = A * xmid + b;
					if((x1-xmid)*(xmid-x2) > 0)
						finish = 2;
				}
				else if((yu1-yl1) == 0 && (yu2-yl2) == 0) {
					// nothing 
				}
				else if((yu1-yl1)*(yu2-yl2) <= 0) {
					// Cheap test for intersection in the general case 
					xmid = (x1*(yl2-yu2) + x2*(yu1-yl1)) / ((yu1-yl1) + (yl2-yu2));
					ymid = yu1 + (yu2-yu1)*(xmid-x1)/(x2-x1);
					finish = 2;
				}
			}
			if(finish == 2) { // curves cross 
				corners[points].x = MapiX(xmid);
				corners[points].y = MapiY(ymid);
				points++;
			}
			if(finish) {
				for(j = i; j >= istart; j--) {
					corners[points].x = MapiX(plot->points[j].xhigh);
					corners[points].y = MapiY(plot->points[j].yhigh);
					points++;
				}
				corners[points].x = (dy < 0) ? 1 : 0;
				FinishFilledCurve(pTerm, points, corners, plot);
				points = 0;
			}
		}
	}
}
// 
// plot_steps:
// Plot the curves in STEPS or FILLSTEPS style
// Each new value is reached by tracing horizontally to the new x value
// and then up/down to the new y value.
// 
//static void plot_steps(GpTermEntry * pTerm, curve_points * plot)
void GnuPlot::PlotSteps(GpTermEntry * pTerm, curve_points * plot)
{
	int i;                          /* point index */
	int x = 0, y = 0;               /* point in terminal coordinates */
	enum coord_type prev = UNDEFINED; /* type of previous point */
	int xprev, yprev;               /* previous point coordinates */
	int xleft, xright, ytop, ybot;  /* plot limits in terminal coords */
	int y0 = 0;                     /* baseline */
	int style = 0;
	// EAM April 2011:  Default to lines only, but allow filled boxes 
	if((plot->plot_style & PLOT_STYLE_HAS_FILL) && pTerm->fillbox) {
		style = style_from_fill(&plot->fill_properties);
		double ey = AxS.__Y().log ? AxS.__Y().min : AxS.__Y().ClipToRange(0.0);
		y0 = MapiY(ey);
	}
	xleft  = MapiX(AxS.__X().min);
	xright = MapiX(AxS.__X().max);
	ybot   = MapiY(AxS.__Y().min);
	ytop   = MapiY(AxS.__Y().max);
	for(i = 0; i < plot->p_count; i++) {
		xprev = x; yprev = y;
		switch(plot->points[i].type) {
			case INRANGE:
			case OUTRANGE:
			    x = MapiX(plot->points[i].x);
			    y = MapiY(plot->points[i].y);
			    if(prev == UNDEFINED || invalid_coordinate(x, y))
				    break;
			    if(style) {
				    // We don't yet have a generalized draw_clip_rectangle routine 
				    int xl = xprev;
				    int xr = x;
				    cliptorange(xr, xleft, xright);
				    cliptorange(xl, xleft, xright);
				    cliptorange(y, ybot, ytop);
				    // Entire box is out of range on x 
				    if(xr == xl && (xr == xleft || xr == xright))
					    break;
				    // Some terminals fail to completely color the join between boxes 
				    if(style == FS_OPAQUE)
					    DrawClipLine(pTerm, xl, yprev, xl, y0);
				    if(yprev - y0 < 0)
					    (pTerm->fillbox)(pTerm, style, xl, yprev, (xr-xl), y0-yprev);
				    else
					    (pTerm->fillbox)(pTerm, style, xl, y0, (xr-xl), yprev-y0);
			    }
			    else {
				    DrawClipLine(pTerm, xprev, yprev, x, yprev);
				    DrawClipLine(pTerm, x, yprev, x, y);
			    }
			    break;
			default: /* just a safety */
			case UNDEFINED:
			    break;
		}
		prev = plot->points[i].type;
	}
}
//
// plot_fsteps:
// Each new value is reached by tracing up/down to the new y value
// and then horizontally to the new x value.
//
//static void plot_fsteps(GpTermEntry * pTerm, curve_points * plot)
void GnuPlot::PlotFSteps(GpTermEntry * pTerm, curve_points * pPlot)
{
	int x = 0, y = 0; // point in terminal coordinates 
	enum coord_type prev = UNDEFINED; /* type of previous point */
	for(int i = 0; i < pPlot->p_count; i++) {
		const int xprev = x; // previous point coordinates 
		const int yprev = y;
		switch(pPlot->points[i].type) {
			case INRANGE:
			case OUTRANGE:
			    x = MapiX(pPlot->points[i].x);
			    y = MapiY(pPlot->points[i].y);
			    if(prev == UNDEFINED || invalid_coordinate(x, y))
				    break;
			    if(prev == INRANGE) {
				    DrawClipLine(pTerm, xprev, yprev, xprev, y);
				    DrawClipLine(pTerm, xprev, y, x, y);
			    }
			    else if(prev == OUTRANGE) {
				    DrawClipLine(pTerm, xprev, yprev, xprev, y);
				    DrawClipLine(pTerm, xprev, y, x, y);
			    }
			    break;
			default: // just a safety 
			case UNDEFINED:
			    break;
		}
		prev = pPlot->points[i].type;
	}
}

/* HBB 20010625: replaced homegrown bubblesort in plot_histeps() by
 * call of standard routine qsort(). Need to tell the compare function
 * about the plotted dataset via this file scope variable: */
static curve_points * histeps_current_plot;

static int histeps_compare(SORTFUNC_ARGS p1, SORTFUNC_ARGS p2)
{
	double x1 = histeps_current_plot->points[*(int *)p1].x;
	double x2 = histeps_current_plot->points[*(int *)p2].x;
	if(x1 < x2)
		return -1;
	else
		return (x1 > x2);
}
// 
// CAC  
// plot_histeps:
// Plot the curves in HISTEPS style
// 
//static void plot_histeps(curve_points * plot)
void GnuPlot::PlotHiSteps(GpTermEntry * pTerm, curve_points * pPlot)
{
	int i;                  /* point index */
	int x1m, y1m, x2m, y2m; /* mapped coordinates */
	double x, y, xn, yn;    /* point position */
	double y_null;          /* y coordinate of histogram baseline */
	int * gl, goodcount;    /* array to hold list of valid points */
	// preliminary count of points inside array 
	goodcount = 0;
	for(i = 0; i < pPlot->p_count; i++)
		if(pPlot->points[i].type == INRANGE || pPlot->points[i].type == OUTRANGE)
			++goodcount;
	if(goodcount < 2)
		return;         /* cannot pPlot less than 2 points */
	gl = (int *)SAlloc::M(goodcount * sizeof(int));
	// fill gl array with indexes of valid (non-undefined) points.  
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
	if(AxS.__Y().log)
		y_null = MIN(AxS.__Y().min, AxS.__Y().max);
	else
		y_null = 0.0;
	x = (3.0 * pPlot->points[gl[0]].x - pPlot->points[gl[1]].x) / 2.0;
	y = y_null;
	for(i = 0; i < goodcount - 1; i++) {    /* loop over all points except last  */
		yn = pPlot->points[gl[i]].y;
		if((AxS.__Y().log) && yn < y_null)
			yn = y_null;
		xn = (pPlot->points[gl[i]].x + pPlot->points[gl[i + 1]].x) / 2.0;
		x1m = MapiX(x);
		x2m = MapiX(xn);
		y1m = MapiY(y);
		y2m = MapiY(yn);
		DrawClipLine(pTerm, x1m, y1m, x1m, y2m);
		DrawClipLine(pTerm, x1m, y2m, x2m, y2m);
		x = xn;
		y = yn;
	}
	yn = pPlot->points[gl[i]].y;
	xn = (3.0 * pPlot->points[gl[i]].x - pPlot->points[gl[i-1]].x) / 2.0;
	x1m = MapiX(x);
	x2m = MapiX(xn);
	y1m = MapiY(y);
	y2m = MapiY(yn);
	DrawClipLine(pTerm, x1m, y1m, x1m, y2m);
	DrawClipLine(pTerm, x1m, y2m, x2m, y2m);
	DrawClipLine(pTerm, x2m, y2m, x2m, MapiY(y_null));
	SAlloc::F(gl);
}
// 
// plot_bars:
// Plot the curves in ERRORBARS style we just plot the bars; the points are plotted in plot_points
//
//static void plot_bars(GpTermEntry * pTerm, curve_points * plot)
void GnuPlot::PlotBars(GpTermEntry * pTerm, curve_points * plot)
{
	int i; // point index 
	double x, y; // position of the bar 
	double ylow, yhigh; // the ends of the bars 
	double xlow, xhigh;
	int xM, ylowM, yhighM; // the mapped version of above 
	int yM, xlowM, xhighM;
	int tic = ERRORBARTIC(pTerm);
	double halfwidth = 0;   /* Used to calculate full box width */
	if(oneof7(plot->plot_style, YERRORBARS, XYERRORBARS, BOXERROR, YERRORLINES, XYERRORLINES, HISTOGRAMS, FILLEDCURVES)) { // Only if term has no filled_polygon!
		// Draw the vertical part of the bar */
		for(i = 0; i < plot->p_count; i++) {
			// undefined points don't count 
			if(plot->points[i].type == UNDEFINED)
				continue;
			// check to see if in xrange 
			x = plot->points[i].x;
			if(plot->plot_style == HISTOGRAMS) {
				/* Shrink each cluster to fit within one unit along X axis,   */
				/* centered about the integer representing the cluster number */
				/* 'start' is reset to 0 at the top of eval_plots(), and then */
				/* incremented if 'plot new histogram' is encountered.        */
				int clustersize = plot->histogram->clustersize + Gg.histogram_opts.gap;
				x  += (i-1) * (clustersize - 1) + plot->histogram_sequence;
				x  += (Gg.histogram_opts.gap - 1) / 2.;
				x  /= clustersize;
				x  += plot->histogram->start + 0.5;
				/* Calculate width also */
				halfwidth = (plot->points[i].xhigh - plot->points[i].xlow) / (2.0 * clustersize);
			}
			if(!AxS.__X().InRange(x))
				continue;
			xM = MapiX(x);
			// check to see if in yrange 
			y = plot->points[i].y;
			if(!AxS.__Y().InRange(y))
				continue;
			yM = MapiY(y);
			// find low and high points of bar, and check yrange 
			yhigh = plot->points[i].yhigh;
			ylow = plot->points[i].ylow;
			yhighM = MapiY(yhigh);
			ylowM  = MapiY(ylow);
			// This can happen if the y errorbar on a log-scaled Y goes negative 
			if(plot->points[i].ylow == -VERYLARGE)
				ylowM = MapiY(MIN(AxS.__Y().min, AxS.__Y().max));
			// find low and high points of bar, and check xrange 
			xhigh = plot->points[i].xhigh;
			xlow  = plot->points[i].xlow;
			if(plot->plot_style == HISTOGRAMS) {
				xlowM  = MapiX(x-halfwidth);
				xhighM = MapiX(x+halfwidth);
			}
			else {
				xhighM = MapiX(xhigh);
				xlowM  = MapiX(xlow);
			}
			// Check for variable color - June 2010 
			if((plot->plot_style != HISTOGRAMS) && (plot->plot_style != FILLEDCURVES)) {
				CheckForVariableColor(pTerm, plot, &plot->varcolor[i]);
			}
			// Error bars can now have a separate line style 
			if(Gr.BarLp.flags & LP_ERRORBAR_SET)
				TermApplyLpProperties(pTerm, &Gr.BarLp);
			// Error bars should be drawn in the border color for filled boxes
			// but only if there *is* a border color. 
			else if((plot->plot_style == BOXERROR) && pTerm->fillbox)
				NeedFillBorder(pTerm, &plot->fill_properties);
			// By here everything has been mapped 
			// First draw the main part of the error bar 
			if(Gg.Polar) // only relevant to polar mode "with yerrorbars" 
				DrawClipLine(pTerm, xlowM, ylowM, xhighM, yhighM);
			else
				DrawClipLine(pTerm, xM, ylowM, xM, yhighM);
			// Even if error bars are dotted, the end lines are always solid 
			if(Gr.BarLp.flags & LP_ERRORBAR_SET)
				pTerm->dashtype(pTerm, DASHTYPE_SOLID, NULL);
			if(!Gg.Polar) {
				if(Gr.BarSize < 0.0) {
					// draw the bottom tic same width as box 
					DrawClipLine(pTerm, xlowM, ylowM, xhighM, ylowM);
					// draw the top tic same width as box 
					DrawClipLine(pTerm, xlowM, yhighM, xhighM, yhighM);
				}
				else if(Gr.BarSize > 0.0) {
					// draw the bottom tic 
					DrawClipLine(pTerm, (int)(xM - Gr.BarSize * tic), ylowM, (int)(xM + Gr.BarSize * tic), ylowM);
					// draw the top tic 
					DrawClipLine(pTerm, (int)(xM - Gr.BarSize * tic), yhighM, (int)(xM + Gr.BarSize * tic), yhighM);
				}
			}
			else { // Polar error bars 
				// Draw the whiskers perpendicular to the main bar 
				if(Gr.BarSize > 0.0) {
					double slope = atan2((double)(yhighM - ylowM), (double)(xhighM - xlowM));
					int x1 = static_cast<int>(xlowM - (Gr.BarSize * tic * sin(slope)));
					int x2 = static_cast<int>(xlowM + (Gr.BarSize * tic * sin(slope)));
					int y1 = static_cast<int>(ylowM + (Gr.BarSize * tic * cos(slope)));
					int y2 = static_cast<int>(ylowM - (Gr.BarSize * tic * cos(slope)));
					// draw the bottom tic 
					if(!V.ClipPoint(xlowM, ylowM)) {
						pTerm->move(pTerm, x1, y1);
						pTerm->vector(pTerm, x2, y2);
					}
					x1 += xhighM - xlowM;
					x2 += xhighM - xlowM;
					y1 += yhighM - ylowM;
					y2 += yhighM - ylowM;
					// draw the top tic 
					if(!V.ClipPoint(xhighM, yhighM)) {
						pTerm->move(pTerm, x1, y1);
						pTerm->vector(pTerm, x2, y2);
					}
				}
			}
		} /* for loop */
	}       /* if yerrorbars OR xyerrorbars OR yerrorlines OR xyerrorlines */
	if(oneof4(plot->plot_style, XERRORBARS, XYERRORBARS, XERRORLINES, XYERRORLINES)) {
		// Draw the horizontal part of the bar 
		for(i = 0; i < plot->p_count; i++) {
			// undefined points don't count 
			if(plot->points[i].type == UNDEFINED)
				continue;
			// check to see if in yrange 
			y = plot->points[i].y;
			if(!AxS.__Y().InRange(y))
				continue;
			yM = MapiY(y);
			// find low and high points of bar, and check xrange 
			xhigh = plot->points[i].xhigh;
			xlow = plot->points[i].xlow;
			xhighM = MapiX(xhigh);
			xlowM  = MapiX(xlow);
			/* This can happen if the x errorbar on a log-scaled X goes negative */
			if(plot->points[i].xlow == -VERYLARGE)
				xlowM = MapiX(MIN(AxS.__X().min, AxS.__X().max));
			// Check for variable color - June 2010 
			CheckForVariableColor(pTerm, plot, &plot->varcolor[i]);
			// Error bars can now have their own line style 
			if(Gr.BarLp.flags & LP_ERRORBAR_SET)
				TermApplyLpProperties(pTerm, &Gr.BarLp);
			// by here everything has been mapped 
			DrawClipLine(pTerm, xlowM, yM, xhighM, yM);
			// Even if error bars are dotted, the end lines are always solid 
			if(Gr.BarLp.flags & LP_ERRORBAR_SET)
				pTerm->dashtype(pTerm, DASHTYPE_SOLID, NULL);
			if(Gr.BarSize > 0.0) {
				DrawClipLine(pTerm, xlowM, (int)(yM - Gr.BarSize * tic), xlowM, (int)(yM + Gr.BarSize * tic));
				DrawClipLine(pTerm, xhighM, (int)(yM - Gr.BarSize * tic), xhighM, (int)(yM + Gr.BarSize * tic));
			}
		} /* for loop */
	}       /* if xerrorbars OR xyerrorbars OR xerrorlines OR xyerrorlines */
	// Restore original line properties 
	TermApplyLpProperties(pTerm, &(plot->lp_properties));
}
// 
// plot_boxes:
// EAM Sep 2002 - Consolidate BOXES and FILLEDBOXES
// 
//static void plot_boxes(GpTermEntry * pTerm, curve_points * plot, int xaxis_y)
void GnuPlot::PlotBoxes(GpTermEntry * pTerm, curve_points * plot, int xaxis_y)
{
	int i;                  /* point index */
	int xl, xr, yb, yt;     /* point in terminal coordinates */
	double dxl, dxr, dyt;
	enum coord_type prev = UNDEFINED; /* type of previous point */
	int lastdef = 0;                /* most recent point that was not UNDEFINED */
	double dyb = 0.0;
	// The stackheight[] array contains the y coord of the top
	// of the stack so far for each point.
	if(plot->plot_style == HISTOGRAMS) {
		int newsize = plot->p_count;
		if(Gg.histogram_opts.type == HT_STACKED_IN_TOWERS)
			Gr.StackCount = 0;
		if(Gg.histogram_opts.type == HT_STACKED_IN_LAYERS && plot->histogram_sequence == 0)
			Gr.StackCount = 0;
		if(!Gr.P_StackHeight) {
			Gr.P_StackHeight = (GpCoordinate *)SAlloc::M(newsize * sizeof(GpCoordinate));
			for(i = 0; i < newsize; i++) {
				Gr.P_StackHeight[i].yhigh = 0;
				Gr.P_StackHeight[i].ylow = 0;
			}
			Gr.StackCount = newsize;
		}
		else if(Gr.StackCount < newsize) {
			Gr.P_StackHeight = (GpCoordinate *)SAlloc::R(Gr.P_StackHeight, newsize * sizeof(GpCoordinate));
			for(i = Gr.StackCount; i < newsize; i++) {
				Gr.P_StackHeight[i].yhigh = 0;
				Gr.P_StackHeight[i].ylow = 0;
			}
			Gr.StackCount = newsize;
		}
	}
	for(i = 0; i < plot->p_count; i++) {
		switch(plot->points[i].type) {
			case OUTRANGE:
			case INRANGE: {
			    if(plot->points[i].z < 0.0) {
				    // need to auto-calc width 
				    if(V.BoxWidth < 0.0)
					    dxl = (plot->points[lastdef].x - plot->points[i].x) / 2.0;
				    else if(!V.BoxWidthIsAbsolute)
					    dxl = (plot->points[lastdef].x - plot->points[i].x) * V.BoxWidth / 2.0;
				    else
					    dxl = -V.BoxWidth / 2.0;
				    if(i < plot->p_count - 1) {
					    int nextdef;
					    for(nextdef = i+1; nextdef < plot->p_count; nextdef++)
						    if(plot->points[nextdef].type != UNDEFINED)
							    break;
					    if(nextdef == plot->p_count) /* i is the last non-UNDEFINED point */
						    nextdef = i;
					    if(V.BoxWidth < 0.0)
						    dxr = (plot->points[nextdef].x - plot->points[i].x) / 2.0;
					    else if(!V.BoxWidthIsAbsolute)
						    dxr = (plot->points[nextdef].x - plot->points[i].x) * V.BoxWidth / 2.0;
					    else // Hits here on 3 column BOXERRORBARS 
						    dxr = V.BoxWidth / 2.0;
					    if(plot->points[nextdef].type == UNDEFINED)
						    dxr = -dxl;
				    }
				    else {
					    dxr = -dxl;
				    }
				    if(prev == UNDEFINED && lastdef == 0)
					    dxl = -dxr;
				    dxl = plot->points[i].x + dxl;
				    dxr = plot->points[i].x + dxr;
			    }
			    else { /* z >= 0 */
				    dxr = plot->points[i].xhigh;
				    dxl = plot->points[i].xlow;
			    }
			    if(plot->plot_style == BOXXYERROR) {
				    dyb = AxS.__Y().ClipToRange(plot->points[i].ylow);
				    xaxis_y = MapiY(dyb);
				    dyt = plot->points[i].yhigh;
			    }
			    else {
				    dyt = plot->points[i].y;
			    }
			    if(plot->plot_style == HISTOGRAMS) {
				    int ix = static_cast<int>(plot->points[i].x);
				    int histogram_linetype = i;
				    lp_style_type ls;
				    int stack = i;
				    if(plot->histogram->startcolor > 0)
					    histogram_linetype += plot->histogram->startcolor;
				    /* Shrink each cluster to fit within one unit along X axis,   */
				    /* centered about the integer representing the cluster number */
				    /* 'start' is reset to 0 at the top of eval_plots(), and then */
				    /* incremented if 'plot new histogram' is encountered.        */
				    if(oneof2(Gg.histogram_opts.type, HT_CLUSTERED, HT_ERRORBARS)) {
					    int clustersize = plot->histogram->clustersize + Gg.histogram_opts.gap;
					    dxl  += (ix-1) * (clustersize - 1) + plot->histogram_sequence;
					    dxr  += (ix-1) * (clustersize - 1) + plot->histogram_sequence;
					    dxl  += (Gg.histogram_opts.gap - 1)/2.;
					    dxr  += (Gg.histogram_opts.gap - 1)/2.;
					    dxl  /= clustersize;
					    dxr  /= clustersize;
					    dxl  += plot->histogram->start + 0.5;
					    dxr  += plot->histogram->start + 0.5;
				    }
				    else if(Gg.histogram_opts.type == HT_STACKED_IN_TOWERS) {
					    dxl  = plot->histogram->start - V.BoxWidth / 2.0;
					    dxr  = plot->histogram->start + V.BoxWidth / 2.0;
					    dxl += plot->histogram_sequence;
					    dxr += plot->histogram_sequence;
				    }
				    else if(Gg.histogram_opts.type == HT_STACKED_IN_LAYERS) {
					    dxl += plot->histogram->start;
					    dxr += plot->histogram->start;
				    }
				    switch(Gg.histogram_opts.type) {
					    case HT_STACKED_IN_TOWERS: /* columnstacked */
						stack = 0;
						// Line type (color) must match row number 
						if(Gg.PreferLineStyles)
							LpUseProperties(pTerm, &ls, histogram_linetype);
						else
							LoadLineType(pTerm, &ls, histogram_linetype);
						ApplyPm3DColor(pTerm, &ls.pm3d_color);
						plot->fill_properties.fillpattern = histogram_linetype;
					    /* Fall through */
					    case HT_STACKED_IN_LAYERS: /* rowstacked */
						if(plot->points[i].y >= 0) {
							dyb = Gr.P_StackHeight[stack].yhigh;
							dyt += Gr.P_StackHeight[stack].yhigh;
							Gr.P_StackHeight[stack].yhigh += plot->points[i].y;
						}
						else {
							dyb = Gr.P_StackHeight[stack].ylow;
							dyt += Gr.P_StackHeight[stack].ylow;
							Gr.P_StackHeight[stack].ylow += plot->points[i].y;
						}
						if((AxS.__Y().min < AxS.__Y().max && dyb < AxS.__Y().min) || (AxS.__Y().max < AxS.__Y().min && dyb > AxS.__Y().min))
							dyb = AxS.__Y().min;
						if((AxS.__Y().min < AxS.__Y().max && dyb > AxS.__Y().max) || (AxS.__Y().max < AxS.__Y().min && dyb < AxS.__Y().max))
							dyb = AxS.__Y().max;
						break;
					    case HT_CLUSTERED:
					    case HT_ERRORBARS:
						break;
				    }
			    }
			    // clip to border 
			    dyt = AxS.__Y().ClipToRange(dyt);
			    dxr = AxS.__X().ClipToRange(dxr);
			    dxl = AxS.__X().ClipToRange(dxl);
			    // Entire box is out of range on x 
			    if(dxr == dxl && (dxr == AxS.__X().min || dxr == AxS.__X().max))
				    break;
			    xl = MapiX(dxl);
			    xr = MapiX(dxr);
			    yt = MapiY(dyt);
			    yb = xaxis_y;
			    // Entire box is out of range on y 
			    if(yb == yt && (dyt == AxS.__Y().min || dyt == AxS.__Y().max))
				    break;
			    if(plot->plot_style == HISTOGRAMS && oneof2(Gg.histogram_opts.type, HT_STACKED_IN_LAYERS, HT_STACKED_IN_TOWERS))
				    yb = MapiY(dyb);
			    // Variable color 
			    if(oneof3(plot->plot_style, BOXES, BOXXYERROR, BOXERROR)) {
				    CheckForVariableColor(pTerm, plot, &plot->varcolor[i]);
			    }
			    if((plot->fill_properties.fillstyle != FS_EMPTY) && pTerm->fillbox) {
				    int style;
				    int x = xl;
				    int y = yb;
				    int w = xr - xl + 1;
				    int h = yt - yb + 1;
				    // avoid negative width/height 
				    if(w <= 0) {
					    x = xr;
					    w = xl - xr + 1;
				    }
				    if(h <= 0) {
					    y = yt;
					    h = yb - yt + 1;
				    }
				    style = style_from_fill(&plot->fill_properties);
				    (pTerm->fillbox)(pTerm, style, x, y, w, h);
				    if(!NeedFillBorder(pTerm, &plot->fill_properties))
					    break;
			    }
			    newpath(pTerm);
			    pTerm->move(pTerm, xl, yb);
			    pTerm->vector(pTerm, xl, yt);
			    pTerm->vector(pTerm, xr, yt);
			    pTerm->vector(pTerm, xr, yb);
			    pTerm->vector(pTerm, xl, yb);
			    closepath(pTerm);
			    if(pTerm->fillbox && plot->fill_properties.border_color.type != TC_DEFAULT) {
				    TermApplyLpProperties(pTerm, &plot->lp_properties);
			    }
			    break;
		    } /* case OUTRANGE, INRANGE */
			default: /* just a safety */
			case UNDEFINED: {
			    break;
		    }
		}               /* switch point-type */
		prev = plot->points[i].type;
		if(prev != UNDEFINED)
			lastdef = i;
	}                       /*loop */
}
// 
// plot_points:
// Plot the curves in POINTSTYLE style
// 
//static void plot_points(GpTermEntry * pTerm, curve_points * plot)
void GnuPlot::PlotPoints(GpTermEntry * pTerm, curve_points * plot)
{
	int i;
	int x, y;
	int p_width, p_height;
	int pointtype;
	int interval = plot->lp_properties.p_interval;
	int number = abs(plot->lp_properties.p_number);
	int offset = 0;
	const char * ptchar;
	// The "pointnumber" property limits the total number of points drawn for this curve 
	if(number) {
		int pcountin = 0;
		for(i = 0; i < plot->p_count; i++) {
			if(plot->points[i].type == INRANGE) 
				pcountin++;
		}
		if(pcountin > number) {
			if(number > 1)
				interval = static_cast<int>((double)(pcountin-1)/(double)(number-1));
			else
				interval = pcountin;
			// offset the first point drawn so that successive plots are more distinct 
			offset = plot->current_plotno * fceili(interval/6.0);
			if(plot->lp_properties.p_number < 0)
				interval = -interval;
		}
	}
	// Set whatever we can that applies to every point in the loop 
	if(plot->lp_properties.PtType == PT_CHARACTER) {
		ignore_enhanced(TRUE);
		if(plot->labels->font && plot->labels->font[0])
			(pTerm->set_font)(pTerm, plot->labels->font);
		pTerm->justify_text(pTerm, CENTRE);
	}
	p_width  = static_cast<int>(pTerm->TicH * plot->lp_properties.PtSize);
	p_height = static_cast<int>(pTerm->TicV * plot->lp_properties.PtSize);
	// Displace overlapping points if "set jitter" is in effect	/
	// This operation leaves x and y untouched, but loads the	
	// jitter offsets into xhigh and yhigh.			
	if(jitter.spread > 0)
		JitterPoints(pTerm, plot);
	for(i = 0; i < plot->p_count; i++) {
		// Only print 1 point per interval 
		if((plot->plot_style == LINESPOINTS) && (interval) && ((i-offset) % interval))
			continue;
		if(plot->points[i].type == INRANGE) {
			x = MapiX(plot->points[i].x);
			y = MapiY(plot->points[i].y);
			// map_x or map_y can hit NaN during eval_link_function(), in which 
			// case the coordinate value is garbage and undefined is TRUE.      
			if(invalid_coordinate(x, y))
				plot->points[i].type = UNDEFINED;
			if(plot->points[i].type == UNDEFINED)
				continue;
			// Apply jitter offsets.
			// Swarm jitter x offset is a multiple of character width.
			// Swarm jitter y offset is in the original coordinate system.
			// vertical jitter y offset is a multiple of character heights.
			if(jitter.spread > 0) {
				x += plot->points[i].CRD_XJITTER * 0.7 * pTerm->ChrH;
				switch(jitter.style) {
					case JITTER_ON_Y:
					    y += plot->points[i].CRD_YJITTER * 0.7 * pTerm->ChrV;
					    break;
					case JITTER_SWARM:
					case JITTER_SQUARE:
					default:
					    y = MapiY(plot->points[i].y + plot->points[i].CRD_YJITTER);
					    break;
				}
			}
			// do clipping if necessary 
			if(!Gg.ClipPoints || (x >= V.BbPlot.xleft + p_width && y >= V.BbPlot.ybot + p_height && x <= V.BbPlot.xright - p_width && y <= V.BbPlot.ytop - p_height)) {
				if(oneof2(plot->plot_style, POINTSTYLE, LINESPOINTS) && plot->lp_properties.PtSize == PTSZ_VARIABLE)
					(pTerm->pointsize)(pTerm, Gg.PointSize * plot->points[i].CRD_PTSIZE);
				// Feb 2016: variable point type 
				if(oneof2(plot->plot_style, POINTSTYLE, LINESPOINTS) && (plot->lp_properties.PtType == PT_VARIABLE) && !(isnan(plot->points[i].CRD_PTTYPE)))
					pointtype = static_cast<int>(plot->points[i].CRD_PTTYPE-1);
				else
					pointtype = plot->lp_properties.PtType;
				// A negative interval indicates we should try to blank out the 
				// area behind the point symbol. This could be done better by   
				// implementing a special point type, but that would require    
				// modification to all terminal drivers. It might be worth it.  
				// GPO.TermApplyLpProperties will restore the point type and size
				if(plot->plot_style == LINESPOINTS && interval < 0) {
					pTerm->set_color(pTerm, &background_fill);
					(pTerm->pointsize)(pTerm, Gg.PointSize * Gg.PointIntervalBox);
					pTerm->point(pTerm, x, y, 6);
					TermApplyLpProperties(pTerm, &(plot->lp_properties));
				}
				// rgb variable  -  color read from data column 
				CheckForVariableColor(pTerm, plot, &plot->varcolor[i]);
				// There are two conditions where we will print a character rather
				// than a point symbol. Otherwise ptchar = NULL;
				// (1) plot->lp_properties.PtType == PT_CHARACTER
				// (2) plot->lp_properties.PtType == PT_VARIABLE and the data file contained a string rather than a number
				//
				if(plot->lp_properties.PtType == PT_CHARACTER)
					ptchar = plot->lp_properties.p_char;
				else if(pointtype == PT_VARIABLE && isnan(plot->points[i].CRD_PTTYPE))
					ptchar = (char *)(&plot->points[i].CRD_PTCHAR);
				else
					ptchar = NULL;
				// Print special character rather than drawn symbol 
				if(ptchar) {
					if(plot->labels && (plot->labels->textcolor.type != TC_DEFAULT))
						ApplyPm3DColor(pTerm, &(plot->labels->textcolor));
					pTerm->put_text(pTerm, x, y, ptchar);
				}
				// The normal case 
				else if(pointtype >= -1)
					pTerm->point(pTerm, x, y, pointtype);
			}
		}
	}
	// Return to initial state 
	if(plot->lp_properties.PtType == PT_CHARACTER) {
		if(plot->labels->font && plot->labels->font[0])
			(pTerm->set_font)(pTerm, "");
		ignore_enhanced(FALSE);
	}
}
// 
// plot_circles:
// Plot the curves in CIRCLES style
// 
//static void plot_circles(GpTermEntry * pTerm, curve_points * pPlot)
void GnuPlot::PlotCircles(GpTermEntry * pTerm, curve_points * pPlot)
{
	fill_style_type * fillstyle = &pPlot->fill_properties;
	const int style = style_from_fill(fillstyle);
	BoundingBox * clip_save = V.P_ClipArea;
	if(Gg.default_circle.clip == OBJ_NOCLIP)
		V.P_ClipArea = &V.BbCanvas;
	const bool withborder = (fillstyle->border_color.type != TC_LT || fillstyle->border_color.lt != LT_NODRAW);
	for(int i = 0; i < pPlot->p_count; i++) {
		if(pPlot->points[i].type == INRANGE) {
			const int x = MapiX(pPlot->points[i].x);
			const int y = MapiY(pPlot->points[i].y);
			if(!invalid_coordinate(x, y)) {
				double radius = x - MapiX(pPlot->points[i].xlow);
				if(pPlot->points[i].z == DEFAULT_RADIUS)
					MapPositionR(pTerm, &Gg.default_circle.o.circle.extent, &radius, NULL, "radius");
				const double arc_begin = pPlot->points[i].ylow;
				const double arc_end = pPlot->points[i].xhigh;
				// rgb variable  -  color read from data column 
				if(!CheckForVariableColor(pTerm, pPlot, &pPlot->varcolor[i]) && withborder)
					TermApplyLpProperties(pTerm, &pPlot->lp_properties);
				DoArc(pTerm, x, y, radius, arc_begin, arc_end, style, FALSE);
				if(withborder) {
					NeedFillBorder(pTerm, &pPlot->fill_properties);
					DoArc(pTerm, x, y, radius, arc_begin, arc_end, 0, Gg.default_circle.o.circle.wedge);
				}
			}
		}
	}
	V.P_ClipArea = clip_save;
}
// 
// plot_ellipses:
// Plot the curves in ELLIPSES style
// 
//static void plot_ellipses(GpTermEntry * pTerm, curve_points * pPlot)
void GnuPlot::PlotEllipses(GpTermEntry * pTerm, curve_points * pPlot)
{
	int i;
	t_ellipse * e = (t_ellipse *)SAlloc::M(sizeof(t_ellipse));
	double tempx, tempy, tempfoo;
	fill_style_type * fillstyle = &pPlot->fill_properties;
	int style = style_from_fill(fillstyle);
	bool withborder = FALSE;
	BoundingBox * clip_save = V.P_ClipArea;
	if(Gg.default_ellipse.clip == OBJ_NOCLIP)
		V.P_ClipArea = &V.BbCanvas;
	if(fillstyle->border_color.type != TC_LT || fillstyle->border_color.lt != LT_NODRAW)
		withborder = TRUE;
	e->extent.scalex = (pPlot->AxIdx_X == SECOND_X_AXIS) ? second_axes : first_axes;
	e->extent.scaley = (pPlot->AxIdx_Y == SECOND_Y_AXIS) ? second_axes : first_axes;
	e->type = pPlot->ellipseaxes_units;
	for(i = 0; i < pPlot->p_count; i++) {
		if(pPlot->points[i].type == INRANGE) {
			e->center.x = MapiX(pPlot->points[i].x);
			e->center.y = MapiY(pPlot->points[i].y);
			if(invalid_coordinate(e->center.x, e->center.y))
				continue;
			e->orientation = pPlot->points[i].ylow;
			if(pPlot->points[i].z <= DEFAULT_RADIUS) {
				MapPositionR(pTerm, &Gg.default_ellipse.o.ellipse.extent, &e->extent.x, &e->extent.y, "ellipse");
			}
			else {
				e->extent.x = pPlot->points[i].xlow; /* major axis */
				e->extent.y = pPlot->points[i].xhigh; /* minor axis */
				// the mapping can be set by the "set ellipseaxes" setting
				// both x units, mixed, both y units 
				// clumsy solution 
				switch(e->type) {
					case ELLIPSEAXES_XY:
					    MapPositionR(pTerm, &e->extent, &tempx, &tempy, "ellipse");
					    e->extent.x = tempx;
					    e->extent.y = tempy;
					    break;
					case ELLIPSEAXES_XX:
					    MapPositionR(pTerm, &e->extent, &tempx, &tempy, "ellipse");
					    tempfoo = tempx;
					    e->extent.x = e->extent.y;
					    MapPositionR(pTerm, &e->extent, &tempy, &tempx, "ellipse");
					    e->extent.x = tempfoo;
					    e->extent.y = tempy;
					    break;
					case ELLIPSEAXES_YY:
					    MapPositionR(pTerm, &e->extent, &tempx, &tempy, "ellipse");
					    tempfoo = tempy;
					    e->extent.y = e->extent.x;
					    MapPositionR(pTerm, &e->extent, &tempy, &tempx, "ellipse");
					    e->extent.x = tempx;
					    e->extent.y = tempfoo;
					    break;
				}
			}
			// rgb variable  -  color read from data column 
			if(!CheckForVariableColor(pTerm, pPlot, &pPlot->varcolor[i]) && withborder)
				TermApplyLpProperties(pTerm, &pPlot->lp_properties);
			DoEllipse(pTerm, 2, e, style, FALSE);
			if(withborder) {
				NeedFillBorder(pTerm, &pPlot->fill_properties);
				DoEllipse(pTerm, 2, e, 0, FALSE);
			}
		}
	}
	SAlloc::F(e);
	V.P_ClipArea = clip_save;
}
//
// plot_dots:
// Plot the curves in DOTS style
//
//static void plot_dots(GpTermEntry * pTerm, const curve_points * pPlot)
void GnuPlot::PlotDots(GpTermEntry * pTerm, const curve_points * pPlot)
{
	for(int i = 0; i < pPlot->p_count; i++) {
		if(pPlot->points[i].type == INRANGE) {
			int x = MapiX(pPlot->points[i].x);
			int y = MapiY(pPlot->points[i].y);
			if(invalid_coordinate(x, y))
				continue;
			// rgb variable  -  color read from data column 
			CheckForVariableColor(pTerm, pPlot, &pPlot->varcolor[i]);
			// point type -1 is a dot 
			pTerm->point(pTerm, x, y, -1);
		}
	}
}
// 
// plot_vectors:
// Plot the curves in VECTORS style
// 
//static void plot_vectors(GpTermEntry * pTerm, curve_points * plot)
void GnuPlot::PlotVectors(GpTermEntry * pTerm, curve_points * plot)
{
	BoundingBox * clip_save = V.P_ClipArea;
	// Normally this is only necessary once because all arrows equal 
	arrow_style_type ap = plot->arrow_properties;
	TermApplyLpProperties(pTerm, &ap.lp_properties);
	ApplyHeadProperties(pTerm, &ap);
	// Clip to plot 
	V.P_ClipArea = &V.BbPlot;
	for(int i = 0; i < plot->p_count; i++) {
		double x0, y0, x1, y1;
		GpCoordinate * tail = &(plot->points[i]);
		if(tail->type != UNDEFINED) {
			// The only difference between "with vectors" and "with arrows"
			// is that vectors already have the head coordinates in xhigh, yhigh
			// while arrows need to generate them from length + angle.
			x0 = MapX(tail->x);
			y0 = MapY(tail->y);
			if(plot->plot_style == VECTOR) {
				x1 = MapX(tail->xhigh);
				y1 = MapY(tail->yhigh);
			}
			else { // ARROWS 
				double length;
				double angle = SMathConst::PiDiv180 * tail->yhigh;
				double aspect = (double)pTerm->TicV / (double)pTerm->TicH;
				if(sstreq(pTerm->name, "windows"))
					aspect = 1.0;
				if(tail->xhigh > 0)
					// length > 0 is in x-axis coords 
					length = MapX(tail->x + tail->xhigh) - x0;
				else {
					// -1 < length < 0 indicates graph coordinates 
					length = tail->xhigh * (V.BbPlot.xright - V.BbPlot.xleft);
					length = fabs(length);
				}
				x1 = x0 + cos(angle) * length;
				y1 = y0 + sin(angle) * length * aspect;
			}
			// variable arrow style read from extra data column 
			if(plot->arrow_properties.tag == AS_VARIABLE) {
				int as = static_cast<int>(tail->z);
				arrow_use_properties(&ap, as);
				TermApplyLpProperties(pTerm, &ap.lp_properties);
				ApplyHeadProperties(pTerm, &ap);
			}
			// variable color read from extra data column. 
			CheckForVariableColor(pTerm, plot, &plot->varcolor[i]);
			// DrawClipArrow does the hard work for us 
			DrawClipArrow(pTerm, x0, y0, x1, y1, ap.head);
		}
	}
	V.P_ClipArea = clip_save;
}
// 
// plot_f_bars:
// Plot the curves in FINANCEBARS style
// EAM Feg 2010	- This routine is also used for BOXPLOT, which loads a median value into xhigh
//
//static void plot_f_bars(GpTermEntry * pTerm, curve_points * pPlot)
void GnuPlot::PlotFBars(GpTermEntry * pTerm, curve_points * pPlot)
{
	int i;                  /* point index */
	double x;               /* position of the bar */
	double ylow, yhigh, yclose, yopen; /* the ends of the bars */
	double ymedian;
	int xM, ylowM, yhighM;  /* the mapped version of above */
	int yopenM, ycloseM, ymedianM;
	bool low_inrange, high_inrange;
	int tic = MAX(ERRORBARTIC(pTerm)/2, 1);
	for(i = 0; i < pPlot->p_count; i++) {
		// undefined points don't count 
		if(pPlot->points[i].type == UNDEFINED)
			continue;
		// check to see if in xrange 
		x = pPlot->points[i].x;
		if(!AxS.__X().InRange(x))
			continue;
		xM = MapiX(x);
		// find low and high points of bar, and check yrange 
		yhigh = pPlot->points[i].yhigh;
		ylow = pPlot->points[i].ylow;
		yclose = pPlot->points[i].z;
		yopen = pPlot->points[i].y;
		ymedian = pPlot->points[i].xhigh;
		high_inrange = AxS.__Y().InRange(yhigh);
		low_inrange  = AxS.__Y().InRange(ylow);
		// compute the plot position of yhigh 
		if(high_inrange)
			yhighM = MapiY(yhigh);
		else if(samesign(yhigh - AxS.__Y().max, AxS.__Y().GetRange()))
			yhighM = MapiY(AxS.__Y().max);
		else
			yhighM = MapiY(AxS.__Y().min);
		// compute the plot position of ylow 
		if(low_inrange)
			ylowM = MapiY(ylow);
		else if(samesign(ylow - AxS.__Y().max, AxS.__Y().GetRange()))
			ylowM = MapiY(AxS.__Y().max);
		else
			ylowM = MapiY(AxS.__Y().min);
		if(!high_inrange && !low_inrange && ylowM == yhighM)
			// both out of range on the same side 
			continue;
		// variable color read from extra data column. June 2010 
		CheckForVariableColor(pTerm, pPlot, &pPlot->varcolor[i]);
		yopenM   = MapiY(yopen);
		ycloseM  = MapiY(yclose);
		ymedianM = MapiY(ymedian);
		// draw the main bar, open tic, close tic 
		DrawClipLine(pTerm, xM, ylowM, xM, yhighM);
		DrawClipLine(pTerm, static_cast<int>(xM - Gr.BarSize * tic), yopenM, xM, yopenM);
		DrawClipLine(pTerm, static_cast<int>(xM + Gr.BarSize * tic), ycloseM, xM, ycloseM);
		// Draw a bar at the median 
		if(pPlot->plot_style == BOXPLOT)
			DrawClipLine(pTerm, static_cast<int>(xM - Gr.BarSize * tic), ymedianM, static_cast<int>(xM + Gr.BarSize * tic), ymedianM);
	}
}
// 
// plot_c_bars:
// Plot the curves in CANDLESTICKS style
// EAM Apr 2008 - switch to using empty/fill rather than empty/striped
//   to distinguish whether (open > close)
// EAM Dec 2009	- allow an optional 6th column to specify width
//   This routine is also used for BOXPLOT, which
//   loads a median value into xhigh
// 
//static void plot_c_bars(GpTermEntry * pTerm, curve_points * plot)
void GnuPlot::PlotCBars(GpTermEntry * pTerm, curve_points * pPlot)
{
	int i;
	double x; // position of the bar 
	double dxl, dxr, ylow, yhigh, yclose, yopen, ymed; // the ends of the bars 
	int xlowM, xhighM, xM, ylowM, yhighM; // mapped version of above 
	int ymin, ymax; // clipped to plot extent 
	enum coord_type prev = UNDEFINED; // type of previous point 
	bool low_inrange, high_inrange;
	bool open_inrange, close_inrange;
	int tic = MAX(ERRORBARTIC(pTerm)/2, 1);
	for(i = 0; i < pPlot->p_count; i++) {
		bool skip_box = FALSE;
		// undefined points don't count 
		if(pPlot->points[i].type == UNDEFINED)
			continue;
		// check to see if in xrange 
		x = pPlot->points[i].x;
		if(!AxS.__X().InRange(x))
			continue;
		xM = MapiX(x);
		// find low and high points of bar, and check yrange 
		yhigh = pPlot->points[i].yhigh;
		ylow = pPlot->points[i].ylow;
		yclose = pPlot->points[i].z;
		yopen = pPlot->points[i].y;
		ymed = pPlot->points[i].xhigh;
		// HBB 20010928: To make code match the documentation, ensure yhigh is actually higher than ylow 
		ExchangeToOrder(&ylow, &yhigh);
		high_inrange = AxS[AxS.Idx_Y].InRange(yhigh);
		low_inrange  = AxS[AxS.Idx_Y].InRange(ylow);
		// compute the pPlot position of yhigh 
		if(high_inrange)
			yhighM = MapiY(yhigh);
		else if(samesign(yhigh - AxS[AxS.Idx_Y].max, AxS[AxS.Idx_Y].GetRange()))
			yhighM = MapiY(AxS[AxS.Idx_Y].max);
		else
			yhighM = MapiY(AxS[AxS.Idx_Y].min);
		// compute the pPlot position of ylow 
		if(low_inrange)
			ylowM = MapiY(ylow);
		else if(samesign(ylow - AxS[AxS.Idx_Y].max, AxS[AxS.Idx_Y].GetRange()))
			ylowM = MapiY(AxS[AxS.Idx_Y].max);
		else
			ylowM = MapiY(AxS[AxS.Idx_Y].min);
		if(!high_inrange && !low_inrange && ylowM == yhighM)
			// both out of range on the same side 
			continue;
		if(pPlot->points[i].xlow != pPlot->points[i].x) {
			dxl = pPlot->points[i].xlow;
			dxr = 2 * x - dxl;
			dxr = AxS.__X().ClipToRange(dxr);
			dxl = AxS.__X().ClipToRange(dxl);
			xlowM  = MapiX(dxl);
			xhighM = MapiX(dxr);
		}
		else if(pPlot->plot_style == BOXPLOT) {
			dxr = (V.BoxWidthIsAbsolute && V.BoxWidth > 0.0) ? (V.BoxWidth/2.0) : 0.25;
			xlowM  = MapiX(x-dxr);
			xhighM = MapiX(x+dxr);
		}
		else if(V.BoxWidth < 0.0) {
			xlowM  = static_cast<int>(xM - Gr.BarSize * tic);
			xhighM = static_cast<int>(xM + Gr.BarSize * tic);
		}
		else {
			dxl = -V.BoxWidth / 2.0;
			if(prev != UNDEFINED)
				if(!V.BoxWidthIsAbsolute)
					dxl = (pPlot->points[i-1].x - pPlot->points[i].x) * V.BoxWidth / 2.0;

			dxr = -dxl;
			if(i < pPlot->p_count - 1) {
				if(pPlot->points[i + 1].type != UNDEFINED) {
					if(!V.BoxWidthIsAbsolute)
						dxr = (pPlot->points[i+1].x - pPlot->points[i].x) * V.BoxWidth / 2.0;
					else
						dxr = V.BoxWidth / 2.0;
				}
			}
			if(prev == UNDEFINED)
				dxl = -dxr;
			dxl = x + dxl;
			dxr = x + dxr;
			dxr = AxS.__X().ClipToRange(dxr);
			dxl = AxS.__X().ClipToRange(dxl);
			xlowM  = MapiX(dxl);
			xhighM = MapiX(dxr);
		}
		// EAM Feb 2007 Force width to be an odd number of pixels
		// so that the center bar can be centered perfectly.	  
		if(((xhighM-xlowM) & 01) != 0) {
			xhighM++;
			if(xM-xlowM > xhighM-xM) 
				xM--;
			if(xM-xlowM < xhighM-xM) 
				xM++;
		}
		// EAM Feb 2006 Clip to plot vertical extent 
		open_inrange  = AxS[AxS.Idx_Y].InRange(yopen);
		close_inrange = AxS[AxS.Idx_Y].InRange(yclose);
		yopen  = AxS.__Y().ClipToRange(yopen);
		yclose = AxS.__Y().ClipToRange(yclose);
		if(MapiY(yopen) < MapiY(yclose)) {
			ymin = MapiY(yopen); 
			ymax = MapiY(yclose);
		}
		else {
			ymax = MapiY(yopen); 
			ymin = MapiY(yclose);
		}
		if(!open_inrange && !close_inrange && ymin == ymax)
			skip_box = TRUE;
		// Reset to original color, if we changed it for the border 
		if(pPlot->fill_properties.border_color.type != TC_DEFAULT && !(pPlot->fill_properties.border_color.type == TC_LT && pPlot->fill_properties.border_color.lt == LT_NODRAW)) {
			TermApplyLpProperties(pTerm, &pPlot->lp_properties);
		}
		// Reset also if we changed it for the errorbars 
		else if(Gr.BarLp.flags & LP_ERRORBAR_SET) {
			TermApplyLpProperties(pTerm, &pPlot->lp_properties);
		}
		// variable color read from extra data column. June 2010 
		CheckForVariableColor(pTerm, pPlot, &pPlot->varcolor[i]);
		// Boxes are always filled if an explicit non-empty fillstyle is set. 
		// If the fillstyle is FS_EMPTY, fill to indicate (open > close).     
		if(pTerm->fillbox && !skip_box) {
			int style = style_from_fill(&pPlot->fill_properties);
			if((style != FS_EMPTY) || (yopen > yclose)) {
				const int x = xlowM;
				const int y = ymin;
				const int w = (xhighM-xlowM);
				const int h = (ymax-ymin);
				if(style == FS_EMPTY && pPlot->plot_style != BOXPLOT)
					style = FS_OPAQUE;
				(pTerm->fillbox)(pTerm, style, x, y, w, h);
				if(style_from_fill(&pPlot->fill_properties) != FS_EMPTY)
					NeedFillBorder(pTerm, &pPlot->fill_properties);
			}
		}
		// Draw open box 
		if(!skip_box) {
			newpath(pTerm);
			pTerm->move(pTerm, xlowM, MapiY(yopen));
			pTerm->vector(pTerm, xhighM, MapiY(yopen));
			pTerm->vector(pTerm, xhighM, MapiY(yclose));
			pTerm->vector(pTerm, xlowM, MapiY(yclose));
			pTerm->vector(pTerm, xlowM, MapiY(yopen));
			closepath(pTerm);
		}
		// BOXPLOT wants a median line also, which is stored in xhigh.
		// If no special style has been assigned for the median line   
		// draw it now, otherwise wait until later.                    
		if(pPlot->plot_style == BOXPLOT && Gg.boxplot_opts.median_linewidth < 0) {
			int ymedianM = MapiY(ymed);
			DrawClipLine(pTerm, xlowM,  ymedianM, xhighM, ymedianM);
		}
		// Through 4.2 gnuplot would indicate (open > close) by drawing     
		// three vertical bars.  Now we use solid fill.  But if the current 
		// terminal does not support filled boxes, fall back to the old way 
		if((yopen > yclose) && !(pTerm->fillbox)) {
			pTerm->move(pTerm, xM, ymin);
			pTerm->vector(pTerm, xM, ymax);
			pTerm->move(pTerm, (xM + xlowM) / 2, ymin);
			pTerm->vector(pTerm, (xM + xlowM) / 2, ymax);
			pTerm->move(pTerm, (xM + xhighM) / 2, ymin);
			pTerm->vector(pTerm, (xM + xhighM) / 2, ymax);
		}
		// Error bars can now have their own line style 
		if(Gr.BarLp.flags & LP_ERRORBAR_SET) {
			TermApplyLpProperties(pTerm, &Gr.BarLp);
		}
		// Draw whiskers 
		DrawClipLine(pTerm, xM, ylowM, xM, ymin);
		DrawClipLine(pTerm, xM, ymax, xM, yhighM);
		// Some users prefer bars at the end of the whiskers 
		if(pPlot->plot_style == BOXPLOT || pPlot->arrow_properties.head == BOTH_HEADS) {
			int d;
			if(pPlot->plot_style == BOXPLOT) {
				d = (Gr.BarSize < 0) ? 0 : static_cast<int>((xhighM-xlowM)/2.0 - (Gr.BarSize * pTerm->TicH));
			}
			else {
				const double frac = pPlot->arrow_properties.head_length;
				d = static_cast<int>((frac <= 0.0) ? 0 : (xhighM-xlowM)*(1.0-frac)/2.0);
			}
			DrawClipLine(pTerm, xlowM+d, yhighM, xhighM-d, yhighM);
			DrawClipLine(pTerm, xlowM+d, ylowM, xhighM-d, ylowM);
		}
		// BOXPLOT wants a median line also, which is stored in xhigh. 
		// If a special linewidth has been assigned draw it now.       
		if(pPlot->plot_style == BOXPLOT && Gg.boxplot_opts.median_linewidth > 0) {
			int ymedianM = MapiY(ymed);
			pTerm->linewidth(pTerm, Gg.boxplot_opts.median_linewidth);
			DrawClipLine(pTerm, xlowM,  ymedianM, xhighM, ymedianM);
			pTerm->linewidth(pTerm, pPlot->lp_properties.l_width);
		}
		prev = pPlot->points[i].type;
	}
}

//static void plot_parallel(GpTermEntry * pTerm, curve_points * pPlot)
void GnuPlot::PlotParallel(GpTermEntry * pTerm, curve_points * pPlot)
{
	// The parallel axis data is stored in successive plot structures. 
	// We will draw it all at once when we see the first one and ignore the rest. 
	if(pPlot->AxIdx_P == 1) {
		for(int i = 0; i < pPlot->p_count; i++) {
			const GpAxis * this_axis = &AxS.Parallel(pPlot->AxIdx_P-1);
			bool prev_NaN = FALSE;
			// rgb variable  -  color read from data column 
			CheckForVariableColor(pTerm, pPlot, &pPlot->varcolor[i]);
			int x0 = MapiX(pPlot->points[i].x);
			int y0 = this_axis->MapI(pPlot->points[i].y);
			prev_NaN = isnan(pPlot->points[i].y);
			curve_points * thisplot = pPlot;
			while((thisplot = thisplot->next)) {
				if(thisplot->plot_style == PARALLELPLOT) {
					if(thisplot->points == NULL) {
						prev_NaN = TRUE;
						continue;
					}
					this_axis = &AxS.Parallel(thisplot->AxIdx_P-1);
					int x1 = MapiX(thisplot->points[i].x);
					int y1 = this_axis->MapI(thisplot->points[i].y);
					if(prev_NaN)
						prev_NaN = isnan(thisplot->points[i].y);
					else if(!(prev_NaN = isnan(thisplot->points[i].y)))
						DrawClipLine(pTerm, x0, y0, x1, y1);
					x0 = x1;
					y0 = y1;
				}
			}
		}
	}
}
// 
// Spiderplots, also known as radar charts, are a form of parallel-axis plot
// in which the axes are arranged radially.  Each sequential clause in a
// "plot ... with spiderplot" command provides values along a single one of
// these axes.  Line, point, and fill properties are taken from the first
// clause of the plot command.  Line properties have already been applied
// prior to calling this routine.
// 
//static void plot_spiderplot(curve_points * plot)
void GnuPlot::PlotSpiderPlot(GpTermEntry * pTerm, curve_points * pPlot)
{
	int i, j;
	curve_points * thisplot;
	static gpiPoint * corners = NULL;
	static gpiPoint * clpcorn = NULL;
	BoundingBox * clip_save = V.P_ClipArea;
	int n_spokes = 0;
	// The parallel axis data is stored in successive plot structures.
	// We will draw it all at once when we see the first one and ignore the rest.
	if(pPlot->AxIdx_P != 1)
		return;
	// This loop counts the number of radial axes 
	for(thisplot = pPlot; thisplot; thisplot = thisplot->next) {
		if(thisplot->plot_type == KEYENTRY)
			continue;
		if(thisplot->plot_style != SPIDERPLOT) {
			IntWarn(NO_CARET, "plot %d is not a spiderplot component", n_spokes);
			continue;
		}
		// Triggers when there is more than one spiderplot in the 'plot' command 
		if(thisplot->AxIdx_P < n_spokes-1)
			break;
		n_spokes++;
		// Use plot title to label the corresponding radial axis 
		if(thisplot->title) {
			SAlloc::F(AxS.Parallel(thisplot->AxIdx_P-1).label.text);
			AxS.Parallel(thisplot->AxIdx_P-1).label.text = sstrdup(thisplot->title);
		}
	}
	if(n_spokes < 3)
		IntError(NO_CARET, "at least 3 axes are needed for a spiderplot");
	// Allocate data structures for one polygon 
	corners = (gpiPoint *)SAlloc::R(corners, (n_spokes+1) * sizeof(gpiPoint));
	clpcorn = (gpiPoint *)SAlloc::R(clpcorn, (2*n_spokes+1) * sizeof(gpiPoint));
	V.P_ClipArea = &V.BbCanvas;
	// 
	// Each row of data (NB: *not* each column) describes a vertex of a polygon.
	// There is one vertex for each comma-separated clause within the overall 2D
	// "plot" command.  Thus p_count rows of data produce p_count polygons.
	// If any row contains NaN or a missing value, no polygon is produced.
	// 
	for(i = 0; i < pPlot->p_count; i++) {
		bool bad_data = FALSE;
		double x, y;
		int out_length;
		int p_type;
		bool already_did_one = FALSE;
		for(thisplot = pPlot; thisplot; thisplot = thisplot->next) {
			if(thisplot->plot_style == SPIDERPLOT && thisplot->plot_type == DATA) { // Ignore other stuff, e.g. KEYENTRY 
				// If any point is missing or NaN, skip the whole polygon 
				if(!thisplot->points || (thisplot->p_count <= i) || (thisplot->points[i].type == UNDEFINED) || isnan(thisplot->points[i].x) || isnan(thisplot->points[i].y)) {
					// FIXME EAM: how to exit cleanly? 
					bad_data = true;
					break;
				}
				// Ran off end of previous spiderplot 
				if(thisplot->AxIdx_P == 1 && already_did_one)
					break;
				else
					already_did_one = TRUE;
				{
					// stored values are axis number, unscaled R 
					GpAxis * this_axis = &AxS.Parallel(thisplot->AxIdx_P-1);
					const double theta = SMathConst::PiDiv2 - (thisplot->points[i].x - 1) * SMathConst::Pi2 / n_spokes;
					const double r = (thisplot->points[i].y - this_axis->min) / this_axis->GetRange();
					PolarToXY(theta, r, &x, &y, false);
					corners[thisplot->AxIdx_P-1].x = MapiX(x);
					corners[thisplot->AxIdx_P-1].y = MapiY(y);
				}
			}
		}
		// Spider plots are unusual in that each row starts a new plot
		// In order to associate the key entry with the correct plot we
		// must do it inside the loop over rows
		if(i > 0) {
			pTerm->layer(pTerm, TERM_LAYER_AFTER_PLOT);
			pTerm->layer(pTerm, TERM_LAYER_BEFORE_PLOT);
		}
		if(pPlot->labels) {
			for(text_label * key_entry = pPlot->labels->next; key_entry; key_entry = key_entry->next) {
				if(key_entry->tag == i) {
					const bool default_color = (pPlot->lp_properties.pm3d_color.type == TC_DEFAULT);
					if(default_color)
						LoadLineType(pTerm, &pPlot->lp_properties, key_entry->tag + 1);
					AdvanceKey(true);
					DoKeySample(pTerm, pPlot, &Gg.KeyT, key_entry->text, pPlot->points[i].CRD_COLOR);
					if(default_color)
						pPlot->lp_properties.pm3d_color.type = TC_DEFAULT;
					_Bry.key_count++;
					AdvanceKey(false);
				}
			}
		}
		if(bad_data) { // Do not draw anything if one or more of the values was bad 
			IntWarn(NO_CARET, "Skipping spiderplot with bad data");
		}
		else {
			corners[n_spokes].x = corners[0].x;
			corners[n_spokes].y = corners[0].y;
			V.ClipPolygon(corners, clpcorn, n_spokes, &out_length);
			clpcorn[0].style = style_from_fill(&pPlot->fill_properties);
			// rgb variable  -  color read from data column 
			if(!CheckForVariableColor(pTerm, pPlot, &pPlot->varcolor[i]) && pPlot->lp_properties.pm3d_color.type == TC_DEFAULT) {
				lp_style_type lptmp;
				LoadLineType(pTerm, &lptmp, i+1);
				ApplyPm3DColor(pTerm, &(lptmp.pm3d_color));
			}
			// variable point type 
			p_type = static_cast<int>(pPlot->points[i].CRD_PTTYPE-1);
			// Draw filled area 
			if(out_length > 1 && pPlot->fill_properties.fillstyle != FS_EMPTY) {
				if(pTerm->filled_polygon)
					pTerm->filled_polygon(pTerm, out_length, clpcorn);
			}
			// Draw perimeter 
			if(NeedFillBorder(pTerm, &pPlot->fill_properties)) {
				for(j = 0; j < n_spokes; j++)
					DrawClipLine(pTerm, corners[j].x, corners[j].y, corners[j+1].x, corners[j+1].y);
			}
			// Points 
			if(p_type) {
				for(j = 0; j < n_spokes; j++)
					pTerm->point(pTerm, corners[j].x, corners[j].y, p_type);
			}
		}
	} // End of loop over rows, each a separate polygon 
	V.P_ClipArea = clip_save;
}
//
// Plot the curves in BOXPLOT style
// helper functions: compare_ypoints, filter_boxplot
//
static int compare_ypoints(SORTFUNC_ARGS arg1, SORTFUNC_ARGS arg2)
{
	GpCoordinate const * p1 = (GpCoordinate const *)arg1;
	GpCoordinate const * p2 = (GpCoordinate const *)arg2;
	/*if(BoxplotFactorSortRequired) {
		// Primary sort key is the "factor" 
		if(p1->z > p2->z)
			return (1);
		if(p1->z < p2->z)
			return (-1);
	}*/
	if(p1->y > p2->y)
		return (1);
	if(p1->y < p2->y)
		return (-1);
	return (0);
}

static int compare_ypoints_boxplot_factor_sort_required(SORTFUNC_ARGS arg1, SORTFUNC_ARGS arg2)
{
	GpCoordinate const * p1 = (GpCoordinate const *)arg1;
	GpCoordinate const * p2 = (GpCoordinate const *)arg2;
	/*if(BoxplotFactorSortRequired)*/
	{
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

//int filter_boxplot(curve_points * pPlot)
int GpGraphics::FilterBoxplot(curve_points * pPlot)
{
	int N = pPlot->p_count;
	// Force any undefined points to the end of the list by y value 
	for(int i = 0; i < N; i++)
		if(pPlot->points[i].type == UNDEFINED)
			pPlot->points[i].y = pPlot->points[i].z = VERYLARGE;
	// Sort the points to find median and quartiles 
	if(pPlot->boxplot_factors > 1) {
		//BoxplotFactorSortRequired = true;
		qsort(pPlot->points, N, sizeof(GpCoordinate), compare_ypoints_boxplot_factor_sort_required);
	}
	else
		qsort(pPlot->points, N, sizeof(GpCoordinate), compare_ypoints);
	// Return a count of well-defined points with this index 
	while(pPlot->points[N-1].type == UNDEFINED)
		N--;
	return N;
}
// 
// wrapper called by do_plot after reading in data but before plotting
// 
//void autoscale_boxplot(GpTermEntry * pTerm, curve_points * plot)
void GnuPlot::AutoscaleBoxPlot(GpTermEntry * pTerm, curve_points * pPlot)
{
	PlotBoxPlot(pTerm, pPlot, true);
}

//static void plot_boxplot(GpTermEntry * pTerm, curve_points * plot, bool only_autoscale)
void GnuPlot::PlotBoxPlot(GpTermEntry * pTerm, curve_points * pPlot, bool onlyAutoscale)
{
	int N;
	GpCoordinate * subset_points;
	int subset_count, true_count;
	text_label * subset_label = pPlot->labels;
	GpCoordinate candle;
	double median, quartile1, quartile3;
	double whisker_top = 0, whisker_bot = 0;
	int level;
	int levels = pPlot->boxplot_factors;
	SETIFZ(levels, 1);
	if(!pPlot->points || pPlot->p_count == 0)
		return;
	// 
	// The entire collection of points was already sorted in filter_boxplot()
	// called from boxplot_range_fiddling().  That sort used the category
	// (a.k.a. "factor" a.k.a. "level") as a primary key and the y value as
	// a secondary key.  That is sufficient for describing all points in a
	// single boxplot, but if we want a separate boxplot for each category
	// then additional bookkeeping is required.
	// 
	for(level = 0; level<levels; level++) {
		if(levels == 1) {
			subset_points = pPlot->points;
			subset_count = pPlot->p_count;
		}
		else {
			subset_label = subset_label->next;
			true_count = 0;
			// advance to first point in subset 
			for(subset_points = pPlot->points; subset_points->z != subset_label->tag; subset_points++, true_count++) {
				// No points found for this boxplot factor 
				if(true_count >= pPlot->p_count)
					break;
			}
			// count well-defined points in this subset 
			for(subset_count = 0; true_count < pPlot->p_count && subset_points[subset_count].z == subset_label->tag; subset_count++, true_count++) {
				if(subset_points[subset_count].type == UNDEFINED)
					break;
			}
		}
		// Not enough points left to make a boxplot 
		N = subset_count;
		if(N < 4) {
			if(onlyAutoscale)
				continue;
			candle.x = subset_points->x + Gg.boxplot_opts.separation * level;
			candle.yhigh = -VERYLARGE;
			candle.ylow = VERYLARGE;
			goto outliers;
		}
		if((N & 0x1) == 0)
			median = 0.5 * (subset_points[N/2-1].y + subset_points[N/2].y);
		else
			median = subset_points[(N-1)/2].y;
		if((N & 0x3) == 0)
			quartile1 = 0.5 * (subset_points[N/4-1].y + subset_points[N/4].y);
		else
			quartile1 = subset_points[(N+3)/4-1].y;
		if((N & 0x3) == 0)
			quartile3 = 0.5 * (subset_points[N - N/4].y + subset_points[N - N/4-1].y);
		else
			quartile3 = subset_points[N - (N+3)/4].y;
		FPRINTF((stderr, "Boxplot: quartile boundaries for %d points: %g %g %g\n", N, quartile1, median, quartile3));
		// Set the whisker limits based on the user-defined style 
		if(Gg.boxplot_opts.limit_type == 0) {
			// Fraction of interquartile range 
			double whisker_len = Gg.boxplot_opts.limit_value * (quartile3 - quartile1);
			int i;
			whisker_bot = quartile1 - whisker_len;
			for(i = 0; i<N; i++)
				if(whisker_bot <= subset_points[i].y) {
					whisker_bot = subset_points[i].y;
					break;
				}
			whisker_top = quartile3 + whisker_len;
			for(i = N-1; i>= 0; i--)
				if(whisker_top >= subset_points[i].y) {
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
			while((double)(top-bot+1)/(double)(N) >= Gg.boxplot_opts.limit_value) {
				// This point is outside of the fractional limit. Remember where it is,
				// step over all points with the same value, then trim back one point.
				whisker_top = subset_points[top].y;
				whisker_bot = subset_points[bot].y;
				if(whisker_top - median >= median - whisker_bot) {
					while((top > 0) && (subset_points[top].y == subset_points[top-1].y))
						top--;
					top--;
				}
				if(whisker_top - median <= median - whisker_bot) {
					while((bot < top) && (subset_points[bot].y == subset_points[bot+1].y))
						bot++;
					bot++;
				}
			}
		}
		// X coordinate needed both for autoscaling and to draw the candlestick 
		if(pPlot->plot_type == FUNC)
			candle.x = (subset_points[0].x + subset_points[N-1].x) / 2.;
		else
			candle.x = subset_points->x + Gg.boxplot_opts.separation * level;
		// We're only here for autoscaling 
		if(onlyAutoscale) {
			AxS.__X().AutoscaleOnePoint(candle.x);
			AxS.__Y().AutoscaleOnePoint(whisker_bot);
			AxS.__Y().AutoscaleOnePoint(whisker_top);
			continue;
		}
		// Dummy up a single-point candlesticks plot using these limiting values 
		candle.type = INRANGE;
		candle.y = quartile1;
		candle.z = quartile3;
		candle.ylow  = whisker_bot;
		candle.yhigh = whisker_top;
		candle.xlow  = subset_points->xlow + Gg.boxplot_opts.separation * level;
		candle.xhigh = median; /* Crazy order of candlestick parameters! */
		// for boxplots "lc variable" means color by factor index 
		if(pPlot->varcolor)
			pPlot->varcolor[0] = pPlot->base_linetype + level + 1;
		// Use the current plot structure to plot the boxplot as a candlestick 
		{
			GpCoordinate save_point = pPlot->points[0];
			int save_count = pPlot->p_count;
			pPlot->points[0] = candle;
			pPlot->p_count = 1;
			if(Gg.boxplot_opts.plotstyle == FINANCEBARS)
				PlotFBars(pTerm, pPlot);
			else
				PlotCBars(pTerm, pPlot);
			pPlot->points[0] = save_point;
			pPlot->p_count = save_count;
		}
		// Now draw individual points for the outliers 
outliers:
		if(Gg.boxplot_opts.outliers) {
			int i, j, x, y;
			int p_width  = static_cast<int>(pTerm->TicH * pPlot->lp_properties.PtSize);
			int p_height = static_cast<int>(pTerm->TicV * pPlot->lp_properties.PtSize);
			for(i = 0; i < subset_count; i++) {
				if(subset_points[i].y >= candle.ylow && subset_points[i].y <= candle.yhigh)
					continue;
				if(subset_points[i].type == UNDEFINED)
					continue;
				x = MapiX(candle.x);
				y = MapiY(subset_points[i].y);
				// previous INRANGE/OUTRANGE no longer valid 
				if(x < V.BbPlot.xleft + p_width ||  y < V.BbPlot.ybot + p_height ||  x > V.BbPlot.xright - p_width ||  y > V.BbPlot.ytop - p_height)
					continue;
				// Separate any duplicate outliers 
				for(j = 1; (i >= j) && (subset_points[i].y == subset_points[i-j].y); j++)
					x += p_width * ((j & 1) == 0 ? -j : j); ;
				pTerm->point(pTerm, x, y, pPlot->lp_properties.PtType);
			}
		}
	}
}
//
// display a x-axis ticmark - called by gen_ticks 
// also uses global tic_start, tic_direction, tic_text and tic_just 
//
//static void xtick2d_callback(GpAxis * pAx, double place, char * text, int ticlevel,
    //lp_style_type grid/* grid.l_type == LT_NODRAW means no grid */, ticmark * userlabels/* User-specified tic labels */)
void GnuPlot::XTick2DCallback(GpTermEntry * pTerm, GpAxis * pAx, double place, char * text, int ticlevel, const lp_style_type & rGrid/* grid.l_type == LT_NODRAW means no grid */, ticmark * userlabels/* User-specified tic labels */)
{
	// minitick if text is NULL - beware - TicH is unsigned 
	int ticsize = static_cast<int>(tic_direction * (int)pTerm->TicV * tic_scale(ticlevel, pAx));
	int x = MapiX(place);
	// Skip label if we've already written a user-specified one here 
#define MINIMUM_SEPARATION 2
	while(userlabels) {
		int here = MapiX(userlabels->position);
		if(abs(here-x) <= MINIMUM_SEPARATION) {
			text = NULL;
			break;
		}
		userlabels = userlabels->next;
	}
#undef MINIMUM_SEPARATION
	if(rGrid.l_type > LT_NODRAW) {
		(pTerm->layer)(pTerm, TERM_LAYER_BEGIN_GRID);
		TermApplyLpProperties(pTerm, &rGrid);
		if(pAx->index == POLAR_AXIS) {
			if(fabs(place) > Gr.LargestPolarCircle)
				Gr.LargestPolarCircle = fabs(place);
			DrawPolarCircle(pTerm, place);
		}
		else {
			legend_key * key = &Gg.KeyT;
			if(key->visible && x < key->bounds.xright && x > key->bounds.xleft && key->bounds.ytop > V.BbPlot.ybot && key->bounds.ybot < V.BbPlot.ytop) {
				if(key->bounds.ybot > V.BbPlot.ybot) {
					pTerm->move(pTerm, x, V.BbPlot.ybot);
					pTerm->vector(pTerm, x, key->bounds.ybot);
				}
				if(key->bounds.ytop < V.BbPlot.ytop) {
					pTerm->move(pTerm, x, key->bounds.ytop);
					pTerm->vector(pTerm, x, V.BbPlot.ytop);
				}
			}
			else {
				pTerm->move(pTerm, x, V.BbPlot.ybot);
				pTerm->vector(pTerm, x, V.BbPlot.ytop);
			}
		}
		TermApplyLpProperties(pTerm, &Gg.border_lp); // border linetype 
		(pTerm->layer)(pTerm, TERM_LAYER_END_GRID);
	} // End of grid code 
	// we precomputed tic posn and text posn in global vars 
	if(x < V.P_ClipArea->xleft || x > V.P_ClipArea->xright)
		return;
	pTerm->move(pTerm, x, tic_start);
	pTerm->vector(pTerm, x, tic_start + ticsize);
	if(tic_mirror >= 0) {
		pTerm->move(pTerm, x, tic_mirror);
		pTerm->vector(pTerm, x, tic_mirror - ticsize);
	}
	if(text) {
		// get offset 
		double offsetx_d, offsety_d;
		MapPositionR(pTerm, &(pAx->ticdef.offset), &offsetx_d, &offsety_d, "xtics");
		// User-specified different color for the tics text 
		if(pAx->ticdef.textcolor.type != TC_DEFAULT)
			ApplyPm3DColor(pTerm, &(pAx->ticdef.textcolor));
		ignore_enhanced(!pAx->ticdef.enhanced);
		write_multiline(pTerm, x+(int)offsetx_d, tic_text+(int)offsety_d, text, tic_hjust, tic_vjust, rotate_tics, pAx->ticdef.font);
		ignore_enhanced(FALSE);
		TermApplyLpProperties(pTerm, &Gg.border_lp); /* reset to border linetype */
	}
}
//
// display a y-axis ticmark - called by gen_ticks 
// also uses global tic_start, tic_direction, tic_text and tic_just 
//
//static void ytick2d_callback(GpAxis * pAx, double place, char * text, int ticlevel,
    //lp_style_type grid/* grid.l_type == LT_NODRAW means no grid */, ticmark * userlabels/* User-specified tic labels */)
void GnuPlot::YTick2DCallback(GpTermEntry * pTerm, GpAxis * pAx, double place, char * text, int ticlevel, const lp_style_type & rGrid/* grid.l_type == LT_NODRAW means no grid */, ticmark * userlabels/* User-specified tic labels */)
{
	// minitick if text is NULL - TicV is unsigned 
	int ticsize = static_cast<int>(tic_direction * (int)pTerm->TicH * tic_scale(ticlevel, pAx));
	int y;
	if(pAx->index >= PARALLEL_AXES)
		y = pAx->MapI(place);
	else
		y = MapiY(place);
	// Skip label if we've already written a user-specified one here 
#define MINIMUM_SEPARATION 2
	while(userlabels) {
		int here = MapiY(userlabels->position);
		if(abs(here-y) <= MINIMUM_SEPARATION) {
			text = NULL;
			break;
		}
		userlabels = userlabels->next;
	}
#undef MINIMUM_SEPARATION
	if(rGrid.l_type > LT_NODRAW) {
		legend_key * key = &Gg.KeyT;
		(pTerm->layer)(pTerm, TERM_LAYER_BEGIN_GRID);
		TermApplyLpProperties(pTerm, &rGrid);
		// Make the grid avoid the key box 
		if(key->visible && y < key->bounds.ytop && y > key->bounds.ybot && key->bounds.xleft < V.BbPlot.xright && key->bounds.xright > V.BbPlot.xleft) {
			if(key->bounds.xleft > V.BbPlot.xleft) {
				pTerm->move(pTerm, V.BbPlot.xleft, y);
				pTerm->vector(pTerm, key->bounds.xleft, y);
			}
			if(key->bounds.xright < V.BbPlot.xright) {
				pTerm->move(pTerm, key->bounds.xright, y);
				pTerm->vector(pTerm, V.BbPlot.xright, y);
			}
		}
		else {
			pTerm->move(pTerm, V.BbPlot.xleft, y);
			pTerm->vector(pTerm, V.BbPlot.xright, y);
		}
		TermApplyLpProperties(pTerm, &Gg.border_lp); /* border linetype */
		(pTerm->layer)(pTerm, TERM_LAYER_END_GRID);
	}
	// we precomputed tic posn and text posn 
	pTerm->move(pTerm, tic_start, y);
	pTerm->vector(pTerm, tic_start + ticsize, y);
	if(tic_mirror >= 0) {
		pTerm->move(pTerm, tic_mirror, y);
		pTerm->vector(pTerm, tic_mirror - ticsize, y);
	}
	if(text) {
		// get offset 
		double offsetx_d, offsety_d;
		MapPositionR(pTerm, &(pAx->ticdef.offset), &offsetx_d, &offsety_d, "ytics");
		// User-specified different color for the tics text 
		if(pAx->ticdef.textcolor.type != TC_DEFAULT)
			ApplyPm3DColor(pTerm, &(pAx->ticdef.textcolor));
		ignore_enhanced(!pAx->ticdef.enhanced);
		write_multiline(pTerm, tic_text+(int)offsetx_d, y+(int)offsety_d, text, tic_hjust, tic_vjust, rotate_tics, pAx->ticdef.font);
		ignore_enhanced(FALSE);
		TermApplyLpProperties(pTerm, &Gg.border_lp); /* reset to border linetype */
	}
}
// 
// called by gen_ticks to place ticmarks on perimeter of polar grid circle 
// also uses global tic_start, tic_direction, tic_text and tic_just 
// 
//static void ttick_callback(GpAxis * pAx, double place, char * text, int ticlevel,
    //lp_style_type grid/* grid.l_type == LT_NODRAW means no grid */, ticmark * userlabels/* User-specified tic labels */)
void GnuPlot::TTickCallback(GpTermEntry * pTerm, GpAxis * pAx, double place, char * text, int ticlevel, const lp_style_type & rGrid/* grid.l_type == LT_NODRAW means no grid */, ticmark * userlabels/* User-specified tic labels */)
{
	int xl, yl; // Inner limit of ticmark 
	int xu, yu; // Outer limit of ticmark 
	int text_x, text_y;
	double delta = 0.05 * tic_scale(ticlevel, pAx) * (pAx->TicIn ? -1 : 1);
	double theta = (place * theta_direction + theta_origin) * SMathConst::PiDiv180;
	double cos_t = Gr.LargestPolarCircle * cos(theta);
	double sin_t = Gr.LargestPolarCircle * sin(theta);
	// Skip label if we've already written a user-specified one here 
	while(userlabels) {
		double here = userlabels->position;
		if(fabs(here - place) <= 0.02) {
			text = NULL;
			break;
		}
		userlabels = userlabels->next;
	}
	xl = MapiX(0.95 * cos_t);
	yl = MapiY(0.95 * sin_t);
	xu = MapiX(cos_t);
	yu = MapiY(sin_t);
	// The normal meaning of "offset" as x/y displacement doesn't work well 
	// for theta tic labels. Use it as a radial offset instead 
	text_x = static_cast<int>(xu + (xu-xl) * (2.0 + pAx->ticdef.offset.x));
	text_y = static_cast<int>(yu + (yu-yl) * (2.0 + pAx->ticdef.offset.x));
	xl = MapiX((1.0+delta) * cos_t);
	yl = MapiY((1.0+delta) * sin_t);
	if(pAx->ticmode & TICS_MIRROR) {
		xu = MapiX( (1.-delta) * cos_t);
		yu = MapiY( (1.-delta) * sin_t);
	}
	DrawClipLine(pTerm, xl, yl, xu, yu);
	if(text && !V.ClipPoint(xu, yu)) {
		if(pAx->ticdef.textcolor.type != TC_DEFAULT)
			ApplyPm3DColor(pTerm, &pAx->ticdef.textcolor);
		// The only rotation angle that makes sense is the angle being labeled 
		if(pAx->tic_rotate != 0.0)
			pTerm->text_angle(pTerm, static_cast<int>(place * theta_direction + theta_origin - 90.0));
		write_multiline(pTerm, text_x, text_y, text, tic_hjust, tic_vjust, 0/* FIXME: these are not correct */, pAx->ticdef.font);
		TermApplyLpProperties(pTerm, &Gg.border_lp);
	}
}

/*{{{  map_position, wrapper, which maps double to int */
//void map_position(const GpTermEntry * pTerm, GpPosition * pos, int * x, int * y, const char * what)
void GnuPlot::MapPosition(const GpTermEntry * pTerm, GpPosition * pos, int * x, int * y, const char * what)
{
	double xx = 0, yy = 0;
	MapPositionDouble(pTerm, pos, &xx, &yy, what);
	*x = static_cast<int>(xx);
	*y = static_cast<int>(yy);
}

/*}}} */

/*{{{  map_position_double */
//static void map_position_double(const GpTermEntry * pTerm, GpPosition * pos, double * x, double * y, const char * what)
void GnuPlot::MapPositionDouble(const GpTermEntry * pTerm, GpPosition * pos, double * x, double * y, const char * what)
{
	switch(pos->scalex) {
		case first_axes:
		case second_axes:
		default:
	    {
		    AXIS_INDEX index = (pos->scalex == first_axes) ? FIRST_X_AXIS : SECOND_X_AXIS;
		    GpAxis * this_axis = &AxS[index];
		    GpAxis * primary = this_axis->linked_to_primary;
		    if(primary && primary->link_udf->at) {
			    double xx = EvalLinkFunction(primary, pos->x);
			    *x = primary->MapI(xx);
		    }
		    else
			    *x = this_axis->MapI(pos->x);
		    break;
	    }
		case graph:
		    *x = V.BbPlot.xleft + pos->x * (V.BbPlot.xright - V.BbPlot.xleft);
		    break;
		case screen:
		    *x = pos->x * (pTerm->MaxX-1);
		    break;
		case character:
		    *x = pos->x * pTerm->ChrH;
		    break;
		case polar_axes:
	    {
		    double xx, yy;
		    PolarToXY(pos->x, pos->y, &xx, &yy, FALSE);
		    *x = AxS[FIRST_X_AXIS].MapI(xx);
		    *y = AxS[FIRST_Y_AXIS].MapI(yy);
		    pos->scaley = polar_axes; /* Just to make sure */
		    break;
	    }
	}
	switch(pos->scaley) {
		case first_axes:
		case second_axes:
		default:
	    {
		    AXIS_INDEX index = (pos->scaley == first_axes) ? FIRST_Y_AXIS : SECOND_Y_AXIS;
		    const GpAxis * this_axis = &AxS[index];
		    const GpAxis * primary = this_axis->linked_to_primary;
		    if(primary && primary->link_udf->at) {
			    double yy = EvalLinkFunction(primary, pos->y);
			    *y = primary->MapI(yy);
		    }
		    else
			    *y = this_axis->MapI(pos->y);
		    break;
	    }
		case graph:
		    *y = V.BbPlot.ybot + pos->y * (V.BbPlot.ytop - V.BbPlot.ybot);
		    break;
		case screen:
		    *y = pos->y * (pTerm->MaxY -1);
		    break;
		case character:
		    *y = pos->y * pTerm->ChrV;
		    break;
		case polar_axes:
		    break;
	}
	*x += 0.5;
	*y += 0.5;
}

/*}}} */

/*{{{  map_position_r */
//void map_position_r(const GpTermEntry * pTerm, GpPosition * pos, double * x, double * y, const char * what)
void GnuPlot::MapPositionR(const GpTermEntry * pTerm, GpPosition * pos, double * x, double * y, const char * what)
{
	// Catches the case of "first" or "second" coords on a log-scaled axis 
	if(pos->x == 0)
		*x = 0;
	else {
		switch(pos->scalex) {
			case first_axes:
		    {
			    double xx = AxisLogValueChecked(FIRST_X_AXIS, pos->x, what);
			    *x = xx * AxS[FIRST_X_AXIS].term_scale;
			    break;
		    }
			case second_axes:
		    {
			    double xx = AxisLogValueChecked(SECOND_X_AXIS, pos->x, what);
			    *x = xx * AxS[SECOND_X_AXIS].term_scale;
			    break;
		    }
			case graph:
			    *x = pos->x * (V.BbPlot.xright - V.BbPlot.xleft);
			    break;
			case screen:
			    *x = pos->x * (pTerm->MaxX-1);
			    break;
			case character:
			    *x = pos->x * pTerm->ChrH;
			    break;
			case polar_axes:
			    *x = 0;
			    break;
		}
	}
	if(y) { // Maybe they only want one coordinate translated? 
		// Catches the case of "first" or "second" coords on a log-scaled axis 
		if(pos->y == 0)
			*y = 0;
		else {
			switch(pos->scaley) {
				case first_axes:
				{
					double yy = AxisLogValueChecked(FIRST_Y_AXIS, pos->y, what);
					*y = yy * AxS[FIRST_Y_AXIS].term_scale;
					return;
				}
				case second_axes:
				{
					double yy = AxisLogValueChecked(SECOND_Y_AXIS, pos->y, what);
					*y = yy * AxS[SECOND_Y_AXIS].term_scale;
					return;
				}
				case graph:
					*y = pos->y * (V.BbPlot.ytop - V.BbPlot.ybot);
					return;
				case screen:
					*y = pos->y * (pTerm->MaxY -1);
					return;
				case character:
					*y = pos->y * pTerm->ChrV;
					break;
				case polar_axes:
					*y = 0;
					break;
			}
		}
	}
}

/*}}} */

void GnuPlot::PlotBorder(GpTermEntry * pTerm)
{
	int min, max;
	bool border_complete = ((Gg.draw_border & 15) == 15);
	(pTerm->layer)(pTerm, TERM_LAYER_BEGIN_BORDER);
	TermApplyLpProperties(pTerm, &Gg.border_lp);   /* border linetype */
	if(border_complete)
		newpath(pTerm);
	// Trace border anticlockwise from upper left 
	pTerm->move(pTerm, V.BbPlot.xleft, V.BbPlot.ytop);
	if(border_west && AxS[FIRST_Y_AXIS].ticdef.rangelimited) {
		AxS.Idx_Y = FIRST_Y_AXIS;
		max = MapiY(AxS[FIRST_Y_AXIS].data_max);
		min = MapiY(AxS[FIRST_Y_AXIS].data_min);
		pTerm->move(pTerm, V.BbPlot.xleft, max);
		pTerm->vector(pTerm, V.BbPlot.xleft, min);
		pTerm->move(pTerm, V.BbPlot.xleft, V.BbPlot.ybot);
	}
	else if(border_west) {
		pTerm->vector(pTerm, V.BbPlot.xleft, V.BbPlot.ybot);
	}
	else {
		pTerm->move(pTerm, V.BbPlot.xleft, V.BbPlot.ybot);
	}
	if(border_south && AxS[FIRST_X_AXIS].ticdef.rangelimited) {
		AxS.Idx_X = FIRST_X_AXIS;
		max = MapiX(AxS[FIRST_X_AXIS].data_max);
		min = MapiX(AxS[FIRST_X_AXIS].data_min);
		pTerm->move(pTerm, min, V.BbPlot.ybot);
		pTerm->vector(pTerm, max, V.BbPlot.ybot);
		pTerm->move(pTerm, V.BbPlot.xright, V.BbPlot.ybot);
	}
	else if(border_south) {
		pTerm->vector(pTerm, V.BbPlot.xright, V.BbPlot.ybot);
	}
	else {
		pTerm->move(pTerm, V.BbPlot.xright, V.BbPlot.ybot);
	}
	if(border_east && AxS[SECOND_Y_AXIS].ticdef.rangelimited) {
		AxS.Idx_Y = SECOND_Y_AXIS;
		max = MapiY(AxS[SECOND_Y_AXIS].data_max);
		min = MapiY(AxS[SECOND_Y_AXIS].data_min);
		pTerm->move(pTerm, V.BbPlot.xright, min);
		pTerm->vector(pTerm, V.BbPlot.xright, max);
		pTerm->move(pTerm, V.BbPlot.xright, V.BbPlot.ytop);
	}
	else if(border_east) {
		pTerm->vector(pTerm, V.BbPlot.xright, V.BbPlot.ytop);
	}
	else {
		pTerm->move(pTerm, V.BbPlot.xright, V.BbPlot.ytop);
	}
	if(border_north && AxS[SECOND_X_AXIS].ticdef.rangelimited) {
		AxS.Idx_X = SECOND_X_AXIS;
		max = MapiX(AxS[SECOND_X_AXIS].data_max);
		min = MapiX(AxS[SECOND_X_AXIS].data_min);
		pTerm->move(pTerm, max, V.BbPlot.ytop);
		pTerm->vector(pTerm, min, V.BbPlot.ytop);
		pTerm->move(pTerm, V.BbPlot.xright, V.BbPlot.ytop);
	}
	else if(border_north) {
		pTerm->vector(pTerm, V.BbPlot.xleft, V.BbPlot.ytop);
	}
	else {
		pTerm->move(pTerm, V.BbPlot.xleft, V.BbPlot.ytop);
	}
	if(border_complete)
		closepath(pTerm);
	// Polar border.  FIXME: Should this be limited to known AxS.__R().max? 
	if((Gg.draw_border & 0x1000) != 0) {
		lp_style_type polar_border = Gg.border_lp;
		BoundingBox * clip_save = V.P_ClipArea;
		V.P_ClipArea = &V.BbPlot;
		// Full-width circular border is visually too heavy compared to the edges 
		polar_border.l_width = polar_border.l_width / 2.;
		TermApplyLpProperties(pTerm, &polar_border);
		if(Gr.LargestPolarCircle <= 0.0)
			Gr.LargestPolarCircle = PolarRadius(AxS.__R().max);
		DrawPolarCircle(pTerm, Gr.LargestPolarCircle);
		V.P_ClipArea = clip_save;
	}
	(pTerm->layer)(pTerm, TERM_LAYER_END_BORDER);
}

//void init_histogram(histogram_style * pHistogram, text_label * pTitle)
void GnuPlot::InitHistogram(histogram_style * pHistogram, text_label * pTitle)
{
	ZFREE(Gr.P_StackHeight);
	if(pHistogram) {
		memcpy(pHistogram, &Gg.histogram_opts, sizeof(Gg.histogram_opts));
		memcpy(&pHistogram->title, pTitle, sizeof(text_label));
		memzero(pTitle, sizeof(*pTitle));
		// Insert in linked list 
		Gg.histogram_opts.next = pHistogram;
	}
}

void free_histlist(histogram_style * hist)
{
	if(hist) {
		if(hist != &GPO.Gg.histogram_opts) {
			SAlloc::F(hist->title.text);
			SAlloc::F(hist->title.font);
		}
		if(hist->next) {
			free_histlist(hist->next); // @recursion
			ZFREE(hist->next);
		}
	}
}

//static void place_histogram_titles()
void GnuPlot::PlaceHistogramTitles(GpTermEntry * pTerm)
{
	for(histogram_style * p_hist = &Gg.histogram_opts; (p_hist = p_hist->next) != 0;) {
		if(!isempty(p_hist->title.text)) {
			double xoffset_d, yoffset_d;
			MapPositionR(pTerm, &(Gg.histogram_opts.title.offset), &xoffset_d, &yoffset_d, "histogram");
			int x = MapiX((p_hist->start + p_hist->end) / 2.0);
			int y = _Bry.xlabel_y;
			// NB: offset in "newhistogram" is additive with that in "set style hist" 
			x += (int)xoffset_d;
			y += (int)yoffset_d + 0.25 * pTerm->ChrV;
			WriteLabel(pTerm, x, y, &(p_hist->title));
			ResetTextColor(pTerm, &p_hist->title.textcolor);
		}
	}
}
// 
// Draw a solid line for the polar axis.
// If the center of the polar plot is not at zero (rmin != 0)
// indicate this by drawing an open circle.
// 
//static void place_raxis()
void GnuPlot::PlaceRAxis(GpTermEntry * pTerm)
{
	#if 0 // 
	t_object raxis_circle = {
		NULL, 1, 1, OBJ_CIRCLE, OBJ_CLIP, /* link, tag, layer (front), object_type, clip */
		{FS_SOLID, 100, 0, BLACK_COLORSPEC},
		{0, LT_BACKGROUND, 0, DASHTYPE_AXIS, 0, 0, 0.2, 0.0, DEFAULT_P_CHAR, BACKGROUND_COLORSPEC, DEFAULT_DASHPATTERN},
		{.circle = {1, {0, 0, 0, 0., 0., 0.}, {graph, 0, 0, 0.02, 0., 0.}, 0., 360. }}
	};
	#endif // } 0
	t_object raxis_circle(t_object::defCircle); // @sobolev @todo (уточнить параметры)
	raxis_circle.layer = 1;
	raxis_circle.tag = 1;
	int x0, y0, xend, yend;
	if(Gg.InvertedRaxis) {
		xend = MapiX(PolarRadius(AxS.__R().set_min));
		x0   = MapiX(PolarRadius(AxS.__R().set_max));
	}
	else {
		double rightend = (AxS.__R().autoscale & AUTOSCALE_MAX) ? AxS.__R().max : AxS.__R().set_max;
		xend = MapiX(rightend - AxS.__R().set_min);
		x0 = MapiX(0);
	}
	yend = y0 = MapiY(0);
	TermApplyLpProperties(pTerm, &Gg.border_lp);
	DrawClipLine(pTerm, x0, y0, xend, yend);
	if(!Gg.InvertedRaxis)
		if(!(AxS.__R().autoscale & AUTOSCALE_MIN) && AxS.__R().set_min != 0)
			PlaceObjects(pTerm, &raxis_circle, LAYER_FRONT, 2);
}

//static void place_parallel_axes(GpTermEntry * pTerm, curve_points * pFirstPlot, int layer)
void GnuPlot::PlaceParallelAxes(GpTermEntry * pTerm, const curve_points * pFirstPlot, int layer)
{
	const curve_points * plot = pFirstPlot;
	GpAxis * this_axis;
	int j, axes_in_use = 0;
	// Walk through the plots and prepare parallel axes as needed 
	for(plot = pFirstPlot; plot; plot = plot->next) {
		if(plot->plot_style == PARALLELPLOT && plot->p_count > 0) {
			axes_in_use = plot->AxIdx_P;
			this_axis = &AxS.Parallel(plot->AxIdx_P - 1);
			axis_invert_if_requested(this_axis);
			this_axis->term_lower = V.BbPlot.ybot;
			this_axis->term_scale = (V.BbPlot.ytop - V.BbPlot.ybot) / this_axis->GetRange();
			setup_tics(this_axis, 20);
		}
	}
	if(Gg.ParallelAxisStyle.layer == LAYER_FRONT && layer == LAYER_BACK)
		return;
	// Draw the axis lines 
	TermApplyLpProperties(pTerm, &Gg.ParallelAxisStyle.lp_properties);
	for(j = 1; j <= axes_in_use; j++) {
		GpAxis * this_axis = &AxS.Parallel(j-1);
		int max = this_axis->MapI(this_axis->data_max);
		int min = this_axis->MapI(this_axis->data_min);
		int axis_x = MapiX(this_axis->paxis_x);
		DrawClipLine(pTerm, axis_x, min, axis_x, max);
	}
	/* Draw the axis tickmarks and labels.  Piggyback on ytick2d_callback */
	/* but avoid a call to the full axis_output_tics().               */
	for(j = 1; j <= axes_in_use; j++) {
		GpAxis * this_axis = &AxS.Parallel(j-1);
		double axis_coord = this_axis->paxis_x;
		if((this_axis->ticmode & TICS_MASK) == NO_TICS)
			continue;
		if(this_axis->tic_rotate && pTerm->text_angle(pTerm, this_axis->tic_rotate)) {
			tic_hjust = LEFT;
			tic_vjust = /*CENTRE*/JUST_CENTRE;
		}
		else {
			tic_hjust = CENTRE;
			tic_vjust = JUST_TOP;
		}
		if(this_axis->manual_justify)
			tic_hjust = this_axis->tic_pos;
		tic_start = AxS[FIRST_X_AXIS].MapI(axis_coord);
		tic_mirror = tic_start; /* tic extends on both sides of axis */
		tic_direction = -1;
		tic_text = static_cast<int>(tic_start - this_axis->ticscale * pTerm->TicV);
		tic_text -= pTerm->ChrV;
		GenTics(pTerm, this_axis, &GnuPlot::YTick2DCallback);
		pTerm->text_angle(pTerm, 0);
	}
}
// 
// Label the curve by placing its title at one end of the curve.
// This option is independent of the plot key, but uses the same
// color/font/text options controlled by "set key".
// This routine is shared by 2D and 3D plots.
// 
//void attach_title_to_plot(curve_points * pPlot, legend_key * pkey)
void GnuPlot::AttachTitleToPlot(GpTermEntry * pTerm, curve_points * pPlot, const legend_key * pkey)
{
	GpCoordinate * points;
	int npoints;
	int index, x, y;
	if(!oneof2(pPlot->plot_type, NODATA, KEYENTRY)) {
		bool is_3D;
		// This routine handles both 2D and 3D plots 
		if(oneof2(pPlot->plot_type, DATA3D, FUNC3D)) {
			points = ((GpSurfacePoints *)pPlot)->iso_crvs->points;
			npoints = ((GpSurfacePoints *)pPlot)->iso_crvs->p_count;
			is_3D = true;
		}
		else {
			points = pPlot->points;
			npoints = pPlot->p_count;
			is_3D = false;
		}
		// beginning or end of plot trace 
		if(pPlot->title_position->x > 0) {
			for(index = npoints-1; index > 0; index--)
				if(points[index].type == INRANGE)
					break;
		}
		else {
			for(index = 0; index < npoints-1; index++)
				if(points[index].type == INRANGE)
					break;
		}
		if(points[index].type == INRANGE) {
			if(is_3D) {
				Map3D_XY(points[index].x, points[index].y, points[index].z, &x, &y);
			}
			else {
				x = MapiX(points[index].x);
				y = MapiY(points[index].y);
			}
			if(pkey->textcolor.type == TC_VARIABLE)
				; // Draw key text in same color as plot 
			else if(pkey->textcolor.type != TC_DEFAULT)
				ApplyPm3DColor(pTerm, &pkey->textcolor); // Draw key text in same color as key title 
			else
				pTerm->linetype(pTerm, LT_BLACK); // Draw key text in black 
			{
				char * p_title = pPlot->title;
				if(pPlot->title_is_automated && (pTerm->flags & TERM_IS_LATEX))
					p_title = texify_title(p_title, pPlot->plot_type);
				write_multiline(pTerm, x, y, p_title, (JUSTIFY)(int)(pPlot->title_position->y), JUST_TOP, 0, pkey->font);
			}
		}
	}
}

//void do_rectangle(GpTermEntry * pTerm, int dimensions, t_object * this_object, const fill_style_type * fillstyle)
void GnuPlot::DoRectangle(GpTermEntry * pTerm, int dimensions, t_object * pObject, const fill_style_type * pFillStyle)
{
	double x1, y1, x2, y2;
	int x, y;
	int style;
	uint w, h;
	bool clip_x = FALSE;
	bool clip_y = FALSE;
	t_rectangle * this_rect = &pObject->o.rectangle;
	if(this_rect->type == 1) {      /* specified as center + size */
		double width, height;
		if(dimensions == 2 || this_rect->center.scalex == screen) {
			MapPositionDouble(pTerm, &this_rect->center, &x1, &y1, "rect");
			MapPositionR(pTerm, &this_rect->extent, &width, &height, "rect");
		}
		else if(_3DBlk.splot_map || _3DBlk.xz_projection || _3DBlk.yz_projection) {
			int junkw, junkh;
			Map3DPositionDouble(pTerm, &this_rect->center, &x1, &y1, "rect");
			Map3DPositionR(pTerm, &this_rect->extent, &junkw, &junkh, "rect");
			width = abs(junkw);
			height = abs(junkh);
		}
		else
			return;
		x1 -= width/2;
		y1 -= height/2;
		x2 = x1 + width;
		y2 = y1 + height;
		w = static_cast<uint>(width);
		h = static_cast<uint>(height);
		if(pObject->clip == OBJ_CLIP) {
			if(this_rect->extent.scalex == first_axes || this_rect->extent.scalex == second_axes)
				clip_x = TRUE;
			if(this_rect->extent.scaley == first_axes || this_rect->extent.scaley == second_axes)
				clip_y = TRUE;
		}
	}
	else {
		if((dimensions == 2) || (this_rect->bl.scalex == screen && this_rect->tr.scalex == screen)) {
			MapPositionDouble(pTerm, &this_rect->bl, &x1, &y1, "rect");
			MapPositionDouble(pTerm, &this_rect->tr, &x2, &y2, "rect");
		}
		else if(_3DBlk.splot_map || _3DBlk.xz_projection || _3DBlk.yz_projection) {
			Map3DPositionDouble(pTerm, &this_rect->bl, &x1, &y1, "rect");
			Map3DPositionDouble(pTerm, &this_rect->tr, &x2, &y2, "rect");
		}
		else
			return;
		//if(x1 > x2) { double t = x1; x1 = x2; x2 = t; }
		ExchangeToOrder(&x1, &x2);
		//if(y1 > y2) { double t = y1; y1 = y2; y2 = t; }
		ExchangeToOrder(&y1, &y2);
		if(pObject->clip == OBJ_CLIP) {
			if(this_rect->bl.scalex != screen && this_rect->tr.scalex != screen)
				clip_x = TRUE;
			if(this_rect->bl.scaley != screen && this_rect->tr.scaley != screen)
				clip_y = TRUE;
		}
	}
	// FIXME - Should there be a generic clip_rectangle() routine?
	// Clip to the graph boundaries, but only if the rectangle
	// itself was specified in plot coords.
	if(V.P_ClipArea) {
		BoundingBox * clip_save = V.P_ClipArea;
		V.P_ClipArea = &V.BbPlot;
		if(clip_x) {
			cliptorange(x1, static_cast<double>(V.P_ClipArea->xleft), static_cast<double>(V.P_ClipArea->xright));
			cliptorange(x2, static_cast<double>(V.P_ClipArea->xleft), static_cast<double>(V.P_ClipArea->xright));
		}
		if(clip_y) {
			cliptorange(y1, static_cast<double>(V.P_ClipArea->ybot), static_cast<double>(V.P_ClipArea->ytop));
			cliptorange(y2, static_cast<double>(V.P_ClipArea->ybot), static_cast<double>(V.P_ClipArea->ytop));
		}
		V.P_ClipArea = clip_save;
	}
	w = static_cast<uint>(x2 - x1);
	h = static_cast<uint>(y2 - y1);
	x = static_cast<int>(x1);
	y = static_cast<int>(y1);
	if(w && h) {
		style = style_from_fill(pFillStyle);
		if(style != FS_EMPTY && pTerm->fillbox)
			(pTerm->fillbox)(pTerm, style, x, y, w, h);
		// Now the border 
		if(NeedFillBorder(pTerm, pFillStyle)) {
			newpath(pTerm);
			pTerm->move(pTerm, x, y);
			pTerm->vector(pTerm, x, y+h);
			pTerm->vector(pTerm, x+w, y+h);
			pTerm->vector(pTerm, x+w, y);
			pTerm->vector(pTerm, x, y);
			closepath(pTerm);
		}
	}
}

//void do_ellipse(GpTermEntry * pTerm, int dimensions, t_ellipse * e, int style, bool do_own_mapping)
void GnuPlot::DoEllipse(GpTermEntry * pTerm, int dimensions, t_ellipse * pEllipse, int style, bool doOwnMapping)
{
	gpiPoint vertex[120];
	int i, in;
	double angle;
	double cx, cy;
	double xoff, yoff;
	double junkfoo;
	int junkw, junkh;
	double cosO = cos(SMathConst::PiDiv180 * pEllipse->orientation);
	double sinO = sin(SMathConst::PiDiv180 * pEllipse->orientation);
	double A = pEllipse->extent.x / 2.0;   /* Major axis radius */
	double B = pEllipse->extent.y / 2.0;   /* Minor axis radius */
	struct GpPosition pos = pEllipse->extent; /* working copy with axis info attached */
	double aspect = (double)pTerm->TicV / (double)pTerm->TicH;
	// Choose how many segments to draw for this ellipse 
	int segments = 72;
	double ang_inc  =  SMathConst::Pi / 36.0;
#ifdef _WIN32
	if(sstreq(pTerm->name, "windows"))
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
		MapPositionDouble(pTerm, &pEllipse->center, &cx, &cy, "ellipse");
	else
		Map3DPositionDouble(pTerm, &pEllipse->center, &cx, &cy, "ellipse");
	// Calculate the vertices 
	for(i = 0, angle = 0.0; i<=segments; i++, angle += ang_inc) {
		// Given that the (co)sines of same sequence of angles
		// are calculated every time - shouldn't they be precomputed
		// and put into a table? 
		pos.x = A * cosO * cos(angle) - B * sinO * sin(angle);
		pos.y = A * sinO * cos(angle) + B * cosO * sin(angle);
		if(!doOwnMapping) {
			xoff = pos.x;
			yoff = pos.y;
		}
		else if(dimensions == 2) {
			switch(pEllipse->type) {
				case ELLIPSEAXES_XY:
				    MapPositionR(pTerm, &pos, &xoff, &yoff, "ellipse");
				    break;
				case ELLIPSEAXES_XX:
				    MapPositionR(pTerm, &pos, &xoff, NULL, "ellipse");
				    pos.x = pos.y;
				    MapPositionR(pTerm, &pos, &yoff, NULL, "ellipse");
				    break;
				case ELLIPSEAXES_YY:
				    MapPositionR(pTerm, &pos, &junkfoo, &yoff, "ellipse");
				    pos.y = pos.x;
				    MapPositionR(pTerm, &pos, &junkfoo, &xoff, "ellipse");
				    break;
			}
		}
		else {
			switch(pEllipse->type) {
				case ELLIPSEAXES_XY:
				    Map3DPositionR(pTerm, &pos, &junkw, &junkh, "ellipse");
				    xoff = junkw;
				    yoff = junkh;
				    break;
				case ELLIPSEAXES_XX:
				    Map3DPositionR(pTerm, &pos, &junkw, &junkh, "ellipse");
				    xoff = junkw;
				    pos.x = pos.y;
				    Map3DPositionR(pTerm, &pos, &junkh, &junkw, "ellipse");
				    yoff = junkh;
				    break;
				case ELLIPSEAXES_YY:
				    Map3DPositionR(pTerm, &pos, &junkw, &junkh, "ellipse");
				    yoff = junkh;
				    pos.y = pos.x;
				    Map3DPositionR(pTerm, &pos, &junkh, &junkw, "ellipse");
				    xoff = junkw;
				    break;
			}
		}
		vertex[i].x = static_cast<int>(cx + xoff);
		if(!doOwnMapping)
			vertex[i].y = static_cast<int>(cy + yoff * aspect);
		else
			vertex[i].y = static_cast<int>(cy + yoff);
	}
	if(style) {
		// Fill in the center 
		gpiPoint fillarea[120];
		V.ClipPolygon(vertex, fillarea, segments, &in);
		fillarea[0].style = style;
		if((in > 1) && pTerm->filled_polygon)
			pTerm->filled_polygon(pTerm, in, fillarea);
	}
	else {
		DrawClipPolygon(pTerm, segments+1, vertex); // Draw the arc 
	}
}

//void do_polygon(int dimensions, t_object * this_object, int style, int facing)
void GnuPlot::DoPolygon(GpTermEntry * pTerm, int dimensions, t_object * pObject, int style, int facing)
{
	t_polygon * p = &pObject->o.polygon;
	t_clip_object clip = pObject->clip;
	static gpiPoint * corners = NULL;
	static gpiPoint * clpcorn = NULL;
	BoundingBox * clip_save = V.P_ClipArea;
	int vertices = p->type;
	int nv;
	if(!p->vertex || vertices < 2)
		return;
	// opt out of coordinate transform in xz or yz projection
	// that would otherwise convert graph x/y/z to hor/ver
	_3DBlk.in_3d_polygon = TRUE;
	corners = (gpiPoint *)SAlloc::R(corners, vertices * sizeof(gpiPoint));
	clpcorn = (gpiPoint *)SAlloc::R(clpcorn, 2 * vertices * sizeof(gpiPoint));
	for(nv = 0; nv < vertices; nv++) {
		if(dimensions == 3)
			Map3DPosition(pTerm, &p->vertex[nv], &corners[nv].x, &corners[nv].y, "pvert");
		else
			MapPosition(pTerm, &p->vertex[nv], &corners[nv].x, &corners[nv].y, "pvert");
		// Any vertex given in screen coords will disable clipping 
		if(p->vertex[nv].scalex == screen || p->vertex[nv].scaley == screen)
			clip = OBJ_NOCLIP;
	}
	// Do we require this polygon to face front or back? 
	if(dimensions == 3 && facing >= 0) {
		double v1[2], v2[2], cross_product;
		v1[0] = corners[1].x - corners[0].x;
		v1[1] = corners[1].y - corners[0].y;
		v2[0] = corners[vertices-2].x - corners[0].x;
		v2[1] = corners[vertices-2].y - corners[0].y;
		cross_product = v1[0]*v2[1] - v1[1]*v2[0];
		if(facing == LAYER_FRONT && cross_product > 0)
			return;
		if(facing == LAYER_BACK && cross_product < 0)
			return;
	}
	if(clip == OBJ_NOCLIP)
		V.P_ClipArea = &V.BbCanvas;
	if(pTerm->filled_polygon && style) {
		int out_length;
		V.ClipPolygon(corners, clpcorn, nv, &out_length);
		clpcorn[0].style = style;
		if(pObject->layer == LAYER_DEPTHORDER && vertices < 12) {
			// FIXME - size arbitrary limit 
			gpdPoint quad[12];
			for(nv = 0; nv < vertices; nv++) {
				quad[nv].x = p->vertex[nv].x;
				quad[nv].y = p->vertex[nv].y;
				quad[nv].z = p->vertex[nv].z;
			}
			// Allow 2-sided coloring 
			// FIXME: this assumes pm3d_color.type == TC_RGB
			// if type == TC_LT instead this will come out off-white
			quad[0].c = pObject->lp_properties.pm3d_color.lt;
			if(pObject->lp_properties.pm3d_color.type == TC_LINESTYLE) {
				int base_color = pObject->lp_properties.pm3d_color.lt;
				GpCoordinate triangle[3];
				lp_style_type face;
				int side;
				int t;
				for(t = 0; t<3; t++) {
					triangle[t].x = quad[t].x;
					triangle[t].y = quad[t].y;
					triangle[t].z = quad[t].z;
				}
				// NB: This is sensitive to the order of the vertices 
				side = Pm3DSide(&(triangle[0]), &(triangle[1]), &(triangle[2]) );
				LpUseProperties(pTerm, &face, side < 0 ? base_color+1 : base_color);
				quad[0].c = face.pm3d_color.lt;
			}
			// FIXME: could we pass through a per-quadrangle border style also? 
			quad[1].c = style;
			Pm3DAddPolygon(pTerm, NULL, quad, vertices);
		}
		else { // Not depth-sorted; draw it now 
			if(out_length > 1)
				pTerm->filled_polygon(pTerm, out_length, clpcorn);
		}
	}
	else { // Just draw the outline? 
		newpath(pTerm);
		DrawClipPolygon(pTerm, nv, corners);
		closepath(pTerm);
	}
	V.P_ClipArea = clip_save;
	_3DBlk.in_3d_polygon = false;
}

//bool check_for_variable_color(const curve_points * pPlot, const double * pColorValue)
bool GnuPlot::CheckForVariableColor(GpTermEntry * pTerm, const curve_points * pPlot, const double * pColorValue)
{
	if(!pPlot->varcolor)
		return false;
	if((pPlot->lp_properties.pm3d_color.value < 0.0) && (pPlot->lp_properties.pm3d_color.type == TC_RGB)) {
		SetRgbColorVar(pTerm, static_cast<uint>(*pColorValue));
		return true;
	}
	else if(pPlot->lp_properties.pm3d_color.type == TC_Z) {
		set_color(pTerm, Cb2Gray(*pColorValue) );
		return true;
	}
	else if(pPlot->lp_properties.pm3d_color.type == TC_COLORMAP) {
		double gray = Map2Gray(*pColorValue, pPlot->lp_properties.P_Colormap);
		SetRgbColorVar(pTerm, rgb_from_colormap(gray, pPlot->lp_properties.P_Colormap));
		return true;
	}
	else if(pPlot->lp_properties.l_type == LT_COLORFROMCOLUMN) {
		lp_style_type lptmp;
		// lc variable will only pick up line _style_ as opposed to _type_ 
		// in the case of "set style increment user".  THIS IS A CHANGE.  
		if(Gg.PreferLineStyles)
			LpUseProperties(pTerm, &lptmp, (int)(*pColorValue));
		else
			LoadLineType(pTerm, &lptmp, (int)(*pColorValue));
		ApplyPm3DColor(pTerm, &(lptmp.pm3d_color));
		return true;
	}
	else
		return false;
}
// 
// rgbscale
// RGB image color components are normally in the range [0:255] but some
// data conventions may use [0:1] instead.  This does the conversion.
// 
//static double rgbscale(double component)
double GpGraphics::RgbScale(double component) const
{
	if(RgbMax != 255.0)
		component = 255.0 * component/RgbMax;
	return (component > 255.0) ? 255.0 : ((component < 0.0) ? 0.0 : component);
}

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
//void process_image(GpTermEntry * pTerm, const void * plot, t_procimg_action action)
void GnuPlot::ProcessImage(GpTermEntry * pTerm, const void * plot, t_procimg_action action)
{
	GpCoordinate * points;
	int    p_count;
	int    i;
	double p_start_corner[2], p_end_corner[2]; /* Points used for computing hyperplane. */
	int    K = 0, L = 0;                      /* Dimensions of image grid. K = <scan line length>, L = <number of scan lines>. */
	uint   ncols, nrows;              /* EAM DEBUG - intended to replace K and L above */
	double p_mid_corner[2];                /* Point representing first corner found, i.e. p(K-1) */
	double delta_x_grid[2] = {0, 0};       /* Spacings between points, two non-orthogonal directions. */
	double delta_y_grid[2] = {0, 0};
	int    grid_corner[4] = {-1, -1, -1, -1}; /* The corner pixels of the image. */
	double view_port_x[2];                 /* Viewable portion of the image. */
	double view_port_y[2];
	double view_port_z[2] = {0, 0};
	t_imagecolor pixel_planes;
	udvt_entry * private_colormap = NULL; // "fc palette <colormap>" 
	// Detours necessary to handle 3D plots 
	bool project_points = false; // True if 3D plot 
	int image_x_axis, image_y_axis;
	if(static_cast<const GpSurfacePoints *>(plot)->plot_type == NODATA) {
		IntWarn(NO_CARET, "no image data");
		return;
	}
	if(oneof2(static_cast<const GpSurfacePoints *>(plot)->plot_type, DATA3D, FUNC3D))
		project_points = TRUE;
	if(project_points) {
		points = ((const GpSurfacePoints *)plot)->iso_crvs->points;
		p_count = ((const GpSurfacePoints *)plot)->iso_crvs->p_count;
		pixel_planes = ((const GpSurfacePoints *)plot)->image_properties.type;
		ncols = ((const GpSurfacePoints *)plot)->image_properties.ncols;
		nrows = ((const GpSurfacePoints *)plot)->image_properties.nrows;
		image_x_axis = FIRST_X_AXIS;
		image_y_axis = FIRST_Y_AXIS;
	}
	else {
		points = ((const curve_points *)plot)->points;
		p_count = ((const curve_points *)plot)->p_count;
		pixel_planes = ((const curve_points *)plot)->image_properties.type;
		ncols = ((const curve_points *)plot)->image_properties.ncols;
		nrows = ((const curve_points *)plot)->image_properties.nrows;
		image_x_axis = ((const curve_points *)plot)->AxIdx_X;
		image_y_axis = ((const curve_points *)plot)->AxIdx_Y;
	}
	if(p_count < 1) {
		IntWarn(NO_CARET, "No points (visible or invisible) to plot.\n\n");
		return;
	}
	if(p_count < 4) {
		IntWarn(NO_CARET, "Image grid must be at least 4 points (2 x 2).\n\n");
		return;
	}
	if(project_points && (AxS.__X().log || AxS.__Y().log || AxS.__Z().log)) {
		IntWarn(NO_CARET, "Log scaling of 3D image plots is not supported");
		return;
	}
	// Check if a special color map was provided 
	private_colormap = ((const GpSurfacePoints *)plot)->lp_properties.P_Colormap;
	// Check if the pixel data forms a valid rectangular grid for potential image
	// matrix support.  A general grid orientation is considered.  If the grid
	// points are orthogonal and oriented along the x/y dimensions the terminal
	// function for images will be used.  Otherwise, the terminal function for
	// filled polygons are used to construct parallelograms for the pixel elements.
	if(project_points) {
		Map3D_XY_double(points[0].x, points[0].y, points[0].z, &p_start_corner[0], &p_start_corner[1]);
		Map3D_XY_double(points[p_count-1].x, points[p_count-1].y, points[p_count-1].z, &p_end_corner[0], &p_end_corner[1]);
	}
	else {
		p_start_corner[0] = points[0].x;
		p_start_corner[1] = points[0].y;
		p_end_corner[0] = points[p_count-1].x;
		p_end_corner[1] = points[p_count-1].y;
	}
	// Catch pathological cases 
	if(isnan(p_start_corner[0]) || isnan(p_end_corner[0]) || isnan(p_start_corner[1]) || isnan(p_end_corner[1]))
		IntError(NO_CARET, "image coordinates undefined");
	// This is a vestige of older code that calculated K and L on the fly	
	// rather than keeping track of matrix/array/image dimensions on input
	K = ncols;
	L = nrows;
	// FIXME: We don't track the dimensions of image data provided as x/y/value	
	// with individual coords rather than via array, matrix, or image format.
	// This might better be done when the data is entered rather than here.	
	if(L == 0 || K == 0) {
		if(points[0].x == points[1].x) {
			/* y coord varies fastest */
			for(K = 0; points[K].x == points[0].x; K++)
				if(K >= p_count)
					break;
			L = p_count / K;
		}
		else {
			/* x coord varies fastest */
			for(K = 0; points[K].y == points[0].y; K++)
				if(K >= p_count)
					break;
			L = p_count / K;
		}
		FPRINTF((stderr, "No dimension information for %d pixels total. Trying %d x %d\n", p_count, L, K));
	}
	grid_corner[0] = 0;
	grid_corner[1] = K-1;
	grid_corner[3] = p_count - 1;
	grid_corner[2] = p_count - K;
	if(action == IMG_UPDATE_AXES) {
		for(i = 0; i < 4; i++) {
			coord_type dummy_type;
			double x = points[grid_corner[i]].x;
			double y = points[grid_corner[i]].y;
			x -= (points[grid_corner[(5-i)%4]].x - points[grid_corner[i]].x)/(2*(K-1));
			y -= (points[grid_corner[(5-i)%4]].y - points[grid_corner[i]].y)/(2*(K-1));
			x -= (points[grid_corner[(i+2)%4]].x - points[grid_corner[i]].x)/(2*(L-1));
			y -= (points[grid_corner[(i+2)%4]].y - points[grid_corner[i]].y)/(2*(L-1));
			// Update range and store value back into itself. 
			dummy_type = INRANGE;
			STORE_AND_UPDATE_RANGE(x, x, dummy_type, image_x_axis, ((curve_points *)plot)->noautoscale, x = -VERYLARGE);
			dummy_type = INRANGE;
			STORE_AND_UPDATE_RANGE(y, y, dummy_type, image_y_axis, ((curve_points *)plot)->noautoscale, y = -VERYLARGE);
		}
		return;
	}
	if(action == IMG_UPDATE_CORNERS) {
		// Shortcut pointer to phantom parallelogram. 
		const iso_curve * iso_crvs = ((const GpSurfacePoints *)plot)->next_sp->iso_crvs;
		// Set the phantom parallelogram as an outline of the image.  Use
		// corner point 0 as a reference point.  Imagine vectors along the
		// generally non-orthogonal directions of the two nearby corners. 
		const double delta_x_1 = (points[grid_corner[1]].x - points[grid_corner[0]].x)/(2*(K-1));
		const double delta_y_1 = (points[grid_corner[1]].y - points[grid_corner[0]].y)/(2*(K-1));
		const double delta_z_1 = (points[grid_corner[1]].z - points[grid_corner[0]].z)/(2*(K-1));
		const double delta_x_2 = (points[grid_corner[2]].x - points[grid_corner[0]].x)/(2*(L-1));
		const double delta_y_2 = (points[grid_corner[2]].y - points[grid_corner[0]].y)/(2*(L-1));
		const double delta_z_2 = (points[grid_corner[2]].z - points[grid_corner[0]].z)/(2*(L-1));
		iso_crvs->points[0].x = points[grid_corner[0]].x - delta_x_1 - delta_x_2;
		iso_crvs->points[0].y = points[grid_corner[0]].y - delta_y_1 - delta_y_2;
		iso_crvs->points[0].z = points[grid_corner[0]].z - delta_z_1 - delta_z_2;
		iso_crvs->next->points[0].x = points[grid_corner[2]].x - delta_x_1 + delta_x_2;
		iso_crvs->next->points[0].y = points[grid_corner[2]].y - delta_y_1 + delta_y_2;
		iso_crvs->next->points[0].z = points[grid_corner[2]].z - delta_z_1 + delta_z_2;
		iso_crvs->points[1].x = points[grid_corner[1]].x + delta_x_1 - delta_x_2;
		iso_crvs->points[1].y = points[grid_corner[1]].y + delta_y_1 - delta_y_2;
		iso_crvs->points[1].z = points[grid_corner[1]].z + delta_z_1 - delta_z_2;
		iso_crvs->next->points[1].x = points[grid_corner[3]].x + delta_x_1 + delta_x_2;
		iso_crvs->next->points[1].y = points[grid_corner[3]].y + delta_y_1 + delta_y_2;
		iso_crvs->next->points[1].z = points[grid_corner[3]].z + delta_z_1 + delta_z_2;
		return;
	}
	if(project_points) {
		Map3D_XY_double(points[K-1].x, points[K-1].y, points[K-1].z, &p_mid_corner[0], &p_mid_corner[1]);
	}
	else {
		p_mid_corner[0] = points[K-1].x;
		p_mid_corner[1] = points[K-1].y;
	}
	// The grid spacing in one direction. 
	delta_x_grid[0] = (p_mid_corner[0] - p_start_corner[0])/(K-1);
	delta_y_grid[0] = (p_mid_corner[1] - p_start_corner[1])/(K-1);
	// The grid spacing in the second direction. 
	delta_x_grid[1] = (p_end_corner[0] - p_mid_corner[0])/(L-1);
	delta_y_grid[1] = (p_end_corner[1] - p_mid_corner[1])/(L-1);
	// 
	// Check if the pixel grid is orthogonal and oriented with axes.
	// If so, then can use efficient terminal image routines.
	// 
	{
		bool rectangular_image = FALSE;
		bool fallback = FALSE;
#define SHIFT_TOLERANCE 0.01
		if(((fabs(delta_x_grid[0]) < SHIFT_TOLERANCE*fabs(delta_x_grid[1])) || 
			(fabs(delta_x_grid[1]) < SHIFT_TOLERANCE*fabs(delta_x_grid[0]))) && ((fabs(delta_y_grid[0]) < SHIFT_TOLERANCE*fabs(delta_y_grid[1])) || 
			(fabs(delta_y_grid[1]) < SHIFT_TOLERANCE*fabs(delta_y_grid[0])))) {
			rectangular_image = TRUE;
			// If the terminal does not have image support then fall back to using polygons to construct pixels.
			if(project_points)
				fallback = !_3DBlk.splot_map || ((const GpSurfacePoints *)plot)->image_properties.fallback;
			else
				fallback = ((const curve_points *)plot)->image_properties.fallback;
		}
		if(pixel_planes == IC_PALETTE && MakePalette(pTerm)) {
			// IntWarn(NO_CARET, "This terminal does not support palette-based images.\n\n"); 
			return;
		}
		if(oneof2(pixel_planes, IC_RGB, IC_RGBA) && (pTerm->flags & TERM_NULL_SET_COLOR)) {
			// IntWarn(NO_CARET, "This terminal does not support rgb images.\n\n"); 
			return;
		}
		// Use generic code to handle alpha channel if the terminal can't 
		if(pixel_planes == IC_RGBA && !(pTerm->flags & TERM_ALPHA_CHANNEL))
			fallback = true;
		// Also use generic code if the pixels are of unequal size, e.g. log scale 
		if(AxS.__X().log || AxS.__Y().log)
			fallback = true;
		view_port_x[0] = (AxS.__X().set_autoscale & AUTOSCALE_MIN) ? AxS.__X().min : AxS.__X().set_min;
		view_port_x[1] = (AxS.__X().set_autoscale & AUTOSCALE_MAX) ? AxS.__X().max : AxS.__X().set_max;
		view_port_y[0] = (AxS.__Y().set_autoscale & AUTOSCALE_MIN) ? AxS.__Y().min : AxS.__Y().set_min;
		view_port_y[1] = (AxS.__Y().set_autoscale & AUTOSCALE_MAX) ? AxS.__Y().max : AxS.__Y().set_max;
		if(project_points) {
			view_port_z[0] = (AxS.__Z().set_autoscale & AUTOSCALE_MIN) ? AxS.__Z().min : AxS.__Z().set_min;
			view_port_z[1] = (AxS.__Z().set_autoscale & AUTOSCALE_MAX) ? AxS.__Z().max : AxS.__Z().set_max;
		}
		if(rectangular_image && pTerm->image && !fallback) {
			// There are eight ways that a valid pixel grid can be entered.  Use table
			// lookup instead of if() statements.  (Draw the various array combinations
			// on a sheet of paper, or see the README file.)
			int line_length, i_delta_pixel, i_delta_line, i_start;
			int pixel_1_1, pixel_M_N;
			coordval * image;
			int array_size;
			float xsts, ysts;
			if(!project_points) {
				// Determine axis direction according to the sign of the terminal scale. 
				xsts = (AxS[AxS.Idx_X].term_scale > 0 ? +1.0f : -1.0f);
				ysts = (AxS[AxS.Idx_Y].term_scale > 0 ? +1.0f : -1.0f);
			}
			else {
				// 3D plots do not use the term_scale mechanism 
				xsts = 1.0f;
				ysts = 1.0f;
			}
			// Set up parameters for indexing through the image matrix to transfer data.
			// These formulas were derived for a terminal image routine which uses the
			// upper left corner as pixel (1,1).
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
			// Assign enough memory for the maximum image size. 
			array_size = K*L;
			// If doing color, multiply size by three for RGB triples.
			if(pixel_planes == IC_RGB)
				array_size *= 3;
			else if(pixel_planes == IC_RGBA)
				array_size *= 4;
			image = (coordval *)SAlloc::M(array_size*sizeof(image[0]));
			// Place points into image array based upon the arrangement of point indices and the visibility of pixels.
			if(image) {
				int j;
				gpiPoint corners[4];
				int M = 0, N = 0; /* M = number of columns, N = number of rows.  (K and L don't have a set direction, but M and N do.) */
				int i_image, i_sub_image = 0;
				int line_pixel_count = 0;
				const double d_x_o_2 = ((points[grid_corner[0]].x - points[grid_corner[1]].x)/(K-1) + (points[grid_corner[0]].x - points[grid_corner[2]].x)/(L-1)) / 2;
				const double d_y_o_2 = ((points[grid_corner[0]].y - points[grid_corner[1]].y)/(K-1) + (points[grid_corner[0]].y - points[grid_corner[2]].y)/(L-1)) / 2;
				const double d_z_o_2 = ((points[grid_corner[0]].z - points[grid_corner[1]].z)/(K-1) + (points[grid_corner[0]].z - points[grid_corner[2]].z)/(L-1)) / 2;
				pixel_1_1 = -1;
				pixel_M_N = -1;
				/* Step through the points placing them in the proper spot in the matrix array. */
				for(i = 0, j = line_length, i_image = i_start; i < p_count; i++) {
					bool visible;
					// This of course should not happen, but if an improperly constructed
					// input file presents more data than expected, the extra data can cause this overflow.
					if(i_image >= p_count || i_image < 0)
						IntError(NO_CARET, "Unexpected line of data in matrix encountered");
					const double x = points[i_image].x;
					const double y = points[i_image].y;
					const double z = points[i_image].z;
					const double x_low  = x - d_x_o_2;  
					const double x_high = x + d_x_o_2;
					const double y_low  = y - d_y_o_2;  
					const double y_high = y + d_y_o_2;
					const double z_low  = z - d_z_o_2;  
					const double z_high = z + d_z_o_2;
					// 
					// Check if a portion of this pixel will be visible.  Do not use the
					// points[i].type == INRANGE test because a portion of a pixel can
					// extend into view and the INRANGE type doesn't account for this.
					// 
					// This series of tests is designed for speed.  If one of the corners
					// of the pixel in question falls in the view port range then the pixel
					// will be visible.  Do this test first because it is the more likely
					// of situations.  It could also happen that the view port is smaller
					// than a pixel.  In that case, if one of the view port corners lands
					// inside the pixel then the pixel in question will be visible.  This
					// won't be as common, so do those tests last.  Set up the if structure
					// in such a way that as soon as one of the tests is true, the conditional tests stop.
					// 
					if((inrange(x_low, view_port_x[0], view_port_x[1]) || inrange(x_high, view_port_x[0], view_port_x[1])) && 
						(inrange(y_low, view_port_y[0], view_port_y[1]) || inrange(y_high, view_port_y[0], view_port_y[1])) && 
						(!project_points || inrange(z_low, view_port_z[0], view_port_z[1]) || inrange(z_high, view_port_z[0], view_port_z[1])))
						visible = true;
					else if((inrange(view_port_x[0], x_low, x_high) || inrange(view_port_x[1], x_low, x_high)) && 
						(inrange(view_port_y[0], y_low, y_high) || inrange(view_port_y[1], y_low, y_high)) && 
						(!project_points || inrange(view_port_z[0], z_low, z_high) || inrange(view_port_z[1], z_low, z_high)))
						visible = true;
					else
						visible = false;
					if(visible) {
						if(pixel_1_1 < 0) {
							// First visible point. 
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
						if(pixel_planes == IC_PALETTE)
							image[i_sub_image++] = Cb2Gray(points[i_image].CRD_COLOR);
						else {
							image[i_sub_image++] = Gr.RgbScale(points[i_image].CRD_R) / 255.0;
							image[i_sub_image++] = Gr.RgbScale(points[i_image].CRD_G) / 255.0;
							image[i_sub_image++] = Gr.RgbScale(points[i_image].CRD_B) / 255.0;
							if(pixel_planes == IC_RGBA)
								image[i_sub_image++] = Gr.RgbScale(points[i_image].CRD_A);
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
					 * pixel_1_1 = position in points[] associated with pixel (1,1)
					 * pixel_M_N = position in points[] associated with pixel (M,N)
					 */
					// One of the delta values in each direction is zero, so add. 
					if(project_points) {
						double x, y;
						Map3D_XY_double(points[pixel_1_1].x, points[pixel_1_1].y, points[pixel_1_1].z, &x, &y);
						corners[0].x = static_cast<int>(x - fabs(delta_x_grid[0]+delta_x_grid[1])/2);
						corners[0].y = static_cast<int>(y + fabs(delta_y_grid[0]+delta_y_grid[1])/2);
						Map3D_XY_double(points[pixel_M_N].x, points[pixel_M_N].y, points[pixel_M_N].z, &x, &y);
						corners[1].x = static_cast<int>(x + fabs(delta_x_grid[0]+delta_x_grid[1])/2);
						corners[1].y = static_cast<int>(y - fabs(delta_y_grid[0]+delta_y_grid[1])/2);
						Map3D_XY_double(view_port_x[0], view_port_y[0], view_port_z[0], &x, &y);
						corners[2].x = static_cast<int>(x);
						corners[2].y = static_cast<int>(y);
						Map3D_XY_double(view_port_x[1], view_port_y[1], view_port_z[1], &x, &y);
						corners[3].x = static_cast<int>(x);
						corners[3].y = static_cast<int>(y);
					}
					else {
						corners[0].x = MapiX(points[pixel_1_1].x - xsts*fabs(d_x_o_2));
						corners[0].y = MapiY(points[pixel_1_1].y + ysts*fabs(d_y_o_2));
						corners[1].x = MapiX(points[pixel_M_N].x + xsts*fabs(d_x_o_2));
						corners[1].y = MapiY(points[pixel_M_N].y - ysts*fabs(d_y_o_2));
						corners[2].x = MapiX(view_port_x[0]);
						corners[2].y = MapiY(view_port_y[1]);
						corners[3].x = MapiX(view_port_x[1]);
						corners[3].y = MapiY(view_port_y[0]);
					}
					(pTerm->image)(pTerm, M, N, image, corners, pixel_planes);
				}
				SAlloc::F((void*)image);
			}
			else {
				IntWarn(NO_CARET, "Could not allocate memory for image.");
				return;
			}
		}
		else { // no term->image  or "with image pixels" 
			// Use sum of vectors to compute the pixel corners with respect to its center. 
			struct { double x; double y; double z; } delta_grid[2], delta_pixel[2];
			int j, i_image;
			// If the pixels are rectangular we will call term->fillbox
			// non-rectangular pixels must be treated as general polygons 
			if(!pTerm->filled_polygon && !rectangular_image)
				IntError(NO_CARET, "This terminal does not support filled polygons");
			(pTerm->layer)(pTerm, TERM_LAYER_BEGIN_IMAGE);
			// Grid spacing in 3D space. 
			delta_grid[0].x = (points[grid_corner[1]].x - points[grid_corner[0]].x) / (K-1);
			delta_grid[0].y = (points[grid_corner[1]].y - points[grid_corner[0]].y) / (K-1);
			delta_grid[0].z = (points[grid_corner[1]].z - points[grid_corner[0]].z) / (K-1);
			delta_grid[1].x = (points[grid_corner[2]].x - points[grid_corner[0]].x) / (L-1);
			delta_grid[1].y = (points[grid_corner[2]].y - points[grid_corner[0]].y) / (L-1);
			delta_grid[1].z = (points[grid_corner[2]].z - points[grid_corner[0]].z) / (L-1);
			// Pixel dimensions in the 3D space. 
			delta_pixel[0].x = (delta_grid[0].x + delta_grid[1].x) / 2;
			delta_pixel[0].y = (delta_grid[0].y + delta_grid[1].y) / 2;
			delta_pixel[0].z = (delta_grid[0].z + delta_grid[1].z) / 2;
			delta_pixel[1].x = (delta_grid[0].x - delta_grid[1].x) / 2;
			delta_pixel[1].y = (delta_grid[0].y - delta_grid[1].y) / 2;
			delta_pixel[1].z = (delta_grid[0].z - delta_grid[1].z) / 2;
			i_image = 0;
			for(j = 0; j < L; j++) {
				double x_line_start = points[grid_corner[0]].x + j * delta_grid[1].x;
				double y_line_start = points[grid_corner[0]].y + j * delta_grid[1].y;
				double z_line_start = points[grid_corner[0]].z + j * delta_grid[1].z;
				for(i = 0; i < K; i++) {
					double x, y, z;
					bool view_in_pixel = FALSE;
					int corners_in_view = 0;
					struct { double x; double y; double z; } p_corners[4]; /* Parallelogram corners. */
					int k;
					// If terminal can't handle alpha, treat it as all-or-none. 
					if(pixel_planes == IC_RGBA) {
						if((points[i_image].CRD_A == 0) ||  (points[i_image].CRD_A < 128 && !(pTerm->flags & TERM_ALPHA_CHANNEL))) {
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
					/* Check if any of the corners are viewable */
					for(k = 0; k < 4; k++) {
						if(inrange(p_corners[k].x, view_port_x[0], view_port_x[1]) && inrange(p_corners[k].y, view_port_y[0], view_port_y[1]) && 
							(inrange(p_corners[k].z, view_port_z[0], view_port_z[1]) || !project_points || _3DBlk.splot_map))
							corners_in_view++;
					}
					if(corners_in_view > 0 || view_in_pixel) {
						int N_corners = 0; /* Number of corners. */
						gpiPoint corners[5]; /* At most 5 corners. */
						corners[0].style = FS_DEFAULT;
						if(corners_in_view > 0) {
							N_corners = 4;
							for(int i_corners = 0; i_corners < N_corners; i_corners++) {
								if(project_points) {
									Map3D_XY_double(p_corners[i_corners].x, p_corners[i_corners].y, p_corners[i_corners].z, &x, &y);
									corners[i_corners].x = static_cast<int>(x);
									corners[i_corners].y = static_cast<int>(y);
								}
								else {
									corners[i_corners].x = MapiX(p_corners[i_corners].x);
									corners[i_corners].y = MapiY(p_corners[i_corners].y);
								}
								// Clip rectangle if necessary 
								if(rectangular_image && pTerm->fillbox && (corners_in_view < 4) && V.P_ClipArea) {
									if(corners[i_corners].x < V.P_ClipArea->xleft)
										corners[i_corners].x = V.P_ClipArea->xleft;
									if(corners[i_corners].x > V.P_ClipArea->xright)
										corners[i_corners].x = V.P_ClipArea->xright;
									if(corners[i_corners].y > V.P_ClipArea->ytop)
										corners[i_corners].y = V.P_ClipArea->ytop;
									if(corners[i_corners].y < V.P_ClipArea->ybot)
										corners[i_corners].y = V.P_ClipArea->ybot;
								}
							}
						}
						else {
							// DJS FIXME:
							// Could still be visible if any of the four corners of the view port are
							// within the parallelogram formed by the pixel.  This is tricky geometry.
						}
						if(N_corners <= 0)
							continue;
						if(pixel_planes == IC_PALETTE) {
							if((points[i_image].type == UNDEFINED) || (isnan(points[i_image].CRD_COLOR))) {
								// EAM April 2012 Distinguish +/-Inf from NaN 
								FPRINTF((stderr, "undefined pixel value %g\n", points[i_image].CRD_COLOR));
								if(isnan(points[i_image].CRD_COLOR))
									goto skip_pixel;
							}
							if(private_colormap) {
								double gray = Map2Gray(points[i_image].CRD_COLOR, private_colormap);
								SetRgbColorVar(pTerm, rgb_from_colormap(gray, private_colormap));
							}
							else
								set_color(pTerm, Cb2Gray(points[i_image].CRD_COLOR) );
						}
						else {
							const int r = static_cast<int>(Gr.RgbScale(points[i_image].CRD_R));
							const int g = static_cast<int>(Gr.RgbScale(points[i_image].CRD_G));
							const int b = static_cast<int>(Gr.RgbScale(points[i_image].CRD_B));
							const int rgblt = (r << 16) + (g << 8) + b;
							SetRgbColorVar(pTerm, rgblt);
						}
						if(pixel_planes == IC_RGBA) {
							int alpha = static_cast<int>(Gr.RgbScale(points[i_image].CRD_A) * 100.0/255.0);
							if(alpha <= 0)
								goto skip_pixel;
							SETMIN(alpha, 100);
							if(pTerm->flags & TERM_ALPHA_CHANNEL)
								corners[0].style = FS_TRANSPARENT_SOLID + (alpha<<4);
						}
						if(rectangular_image && pTerm->fillbox && !(pTerm->flags & TERM_POLYGON_PIXELS)) {
							// Some terminals (canvas) can do filled rectangles 
							// more efficiently than filled polygons. 
							(pTerm->fillbox)(pTerm, corners[0].style, MIN(corners[0].x, corners[2].x), MIN(corners[0].y, corners[2].y), abs(corners[2].x - corners[0].x), abs(corners[2].y - corners[0].y));
						}
						else {
							(pTerm->filled_polygon)(pTerm, N_corners, corners);
						}
					}
skip_pixel:
					i_image++;
				}
			}
			(pTerm->layer)(pTerm, TERM_LAYER_END_IMAGE);
		}
	}
}
// 
// Draw one circle of the polar grid or border
// NB: place is in x-axis coordinates
// 
//static void draw_polar_circle(double place)
void GnuPlot::DrawPolarCircle(GpTermEntry * pTerm, double place)
{
	const double step = 2.5;
	double x = place;
	double y = 0.0;
	int ogx = MapiX(x);
	int ogy = MapiY(y);
	for(double angle = step; angle <= 360.0; angle += step) {
		x = place * cos(angle*SMathConst::PiDiv180);
		y = place * sin(angle*SMathConst::PiDiv180);
		int gx = MapiX(x);
		int gy = MapiY(y);
		DrawClipLine(pTerm, ogx, ogy, gx, gy);
		ogx = gx;
		ogy = gy;
	}
}

//static void place_spiderplot_axes(GpTermEntry * pTerm, curve_points * pFirstPlot, int layer)
void GnuPlot::PlaceSpiderPlotAxes(GpTermEntry * pTerm, const curve_points * pFirstPlot, int layer)
{
	const curve_points * plot = pFirstPlot;
	GpAxis * this_axis;
	int j, n_spokes = 0;
	if(Gg.SpiderPlot) {
		// Walk through the plots and adjust axis min/max as needed 
		for(plot = pFirstPlot; plot; plot = plot->next) {
			if(plot->plot_style == SPIDERPLOT && plot->p_count) {
				n_spokes = plot->AxIdx_P;
				if(n_spokes > AxS.GetParallelAxisCount())
					IntError(NO_CARET, "attempt to draw undefined radial axis");
				this_axis = &AxS.Parallel(plot->AxIdx_P - 1);
				setup_tics(this_axis, 20);
				// Use plot title to label the corresponding radial axis 
				if(plot->title) {
					SAlloc::F(this_axis->label.text);
					this_axis->label.text = sstrdup(plot->title);
				}
			}
		}
		// 
		// This should never happen if there really are spiderplots,
		// but we have left open the possibility that other plot types
		// could be mapped onto the radial axes.
		// That case does not yet have support in place.
		// 
		if(n_spokes && AxS.HasParallel()) {
			// Place the grid lines 
			if(grid_spiderweb && layer == LAYER_BACK) {
				this_axis = &AxS.Parallel(0);
				this_axis->gridmajor = TRUE;
				TermApplyLpProperties(pTerm, &grid_lp);
				// copy n_spokes somewhere that spidertick_callback can see it 
				this_axis->term_zero = n_spokes;
				this_axis->ticdef.rangelimited = FALSE;
				GenTics(pTerm, this_axis, &GnuPlot::SpiderTickCallback);
				this_axis->gridmajor = FALSE;
			}
			if(Gg.ParallelAxisStyle.layer == LAYER_FRONT && layer == LAYER_BACK)
				return;
			// Draw the axis lines 
			for(j = 1; j <= n_spokes; j++) {
				coordval theta = SMathConst::PiDiv2 - (j - 1) * SMathConst::Pi2 / n_spokes;
				// axis linestyle can be customized 
				this_axis = &AxS.Parallel(j-1);
				if(this_axis->zeroaxis)
					TermApplyLpProperties(pTerm, this_axis->zeroaxis);
				else
					TermApplyLpProperties(pTerm, &Gg.ParallelAxisStyle.lp_properties);
				PolarToXY(theta, 0.0, &Gr.Spoke0.x, &Gr.Spoke0.y, FALSE);
				PolarToXY(theta, 1.0, &Gr.Spoke1.x, &Gr.Spoke1.y, FALSE);
				DrawClipLine(pTerm, MapiX(Gr.Spoke0.x), MapiY(Gr.Spoke0.y), MapiX(Gr.Spoke1.x), MapiY(Gr.Spoke1.y) );
				// Draw the tickmarks and labels 
				if(this_axis->ticmode) {
					//SpokeD.x = (Spoke0.y - Spoke1.y) * 0.02;
					//SpokeD.y = (Spoke1.x - Spoke0.x) * 0.02;
					Gr.SpokeD.Set((Gr.Spoke0.y - Gr.Spoke1.y) * 0.02, (Gr.Spoke1.x - Gr.Spoke0.x) * 0.02);
					// FIXME: separate control of tic linewidth? 
					TermApplyLpProperties(pTerm, &Gg.border_lp);
					this_axis->ticdef.rangelimited = FALSE;
					GenTics(pTerm, this_axis, &GnuPlot::SpiderTickCallback);
				}
				// Draw the axis label 
				// Interpret any requested offset as a radial offset 
				if(this_axis->label.text) {
					double radial_offset = this_axis->label.offset.x;
					this_axis->label.offset.x = 0.0;
					WriteLabel(pTerm, MapiX(Gr.Spoke1.x + (1.0 + radial_offset) * 0.12 * (Gr.Spoke1.x-Gr.Spoke0.x)),
						MapiY(Gr.Spoke1.y + (1. + radial_offset) * 0.12 * (Gr.Spoke1.y-Gr.Spoke0.y)), &this_axis->label);
					this_axis->label.offset.x = radial_offset;
				}
			}
		}
	}
}

//static void spidertick_callback(GpAxis * pAx, double place, char * text, int ticlevel, lp_style_type grid, ticmark * userlabels)
void GnuPlot::SpiderTickCallback(GpTermEntry * pTerm, GpAxis * pAx, double place, char * text, int ticlevel, const lp_style_type & rGrid, ticmark * userlabels)
{
	const  double fraction = (place - pAx->min) / pAx->GetRange();
	if(fraction > 0.0) {
		const double tic_x = fraction * (Gr.Spoke1.x - Gr.Spoke0.x);
		const double tic_y = fraction * (Gr.Spoke1.y - Gr.Spoke0.y);
		const double ticsize = tic_scale(ticlevel, pAx);
		// This is an awkward place to draw the grid, but due to the general
		// mechanism of calculating tick positions via callback it is the only
		// place we know the desired radial position of the grid lines.
		if(grid_spiderweb && pAx->gridmajor && (grid_lp.l_type != LT_NODRAW)) {
			const int n_spokes = pAx->term_zero;
			gpiPoint * corners = (gpiPoint *)SAlloc::M((n_spokes+1) * sizeof(gpiPoint));
			int i;
			for(i = 0; i < n_spokes; i++) {
				double x, y;
				double theta = SMathConst::PiDiv2 - SMathConst::Pi2 * (double)i / (double)n_spokes;
				PolarToXY(theta, fraction, &x, &y, FALSE);
				corners[i].x = MapiX(x);
				corners[i].y = MapiY(y);
			}
			corners[n_spokes].x = corners[0].x;
			corners[n_spokes].y = corners[0].y;
			for(i = 0; i < n_spokes; i++)
				DrawClipLine(pTerm, corners[i].x, corners[i].y, corners[i+1].x, corners[i+1].y);
			SAlloc::F(corners);
		}
		else {
			// Draw tick mark itself 
			DrawClipLine(pTerm, MapiX(tic_x - ticsize * Gr.SpokeD.x), MapiY(tic_y - ticsize * Gr.SpokeD.y), MapiX(tic_x + ticsize * Gr.SpokeD.x), MapiY(tic_y + ticsize * Gr.SpokeD.y));
			// Draw tick label 
			if(text) {
				double offsetx_d, offsety_d;
				const int tic_label_x = MapiX(tic_x - (4.0+ticsize) * Gr.SpokeD.x);
				const int tic_label_y = MapiY(tic_y - (4.0+ticsize) * Gr.SpokeD.y);
				MapPositionR(pTerm, &(pAx->ticdef.offset), &offsetx_d, &offsety_d, "");
				if(pAx->ticdef.textcolor.type != TC_DEFAULT)
					ApplyPm3DColor(pTerm, &(pAx->ticdef.textcolor));
				ignore_enhanced(!pAx->ticdef.enhanced);
				write_multiline(pTerm, tic_label_x + (int)offsetx_d, tic_label_y + (int)offsety_d, text, CENTRE, JUST_CENTRE, pAx->tic_rotate, pAx->ticdef.font);
				ignore_enhanced(FALSE);
				// FIXME:  the plan is to have a separate lp for spiderplot tics 
				if(pAx->ticdef.textcolor.type != TC_DEFAULT)
					TermApplyLpProperties(pTerm, &Gg.border_lp);
			}
		}
	}
}
