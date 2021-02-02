// GNUPLOT - color.c 
// Copyright: open source as much as possible Petr Mikulik, since December 1998
// What is here:
// - Global variables declared in .h are initialized here
// - Palette routines
// - Colour box drawing
// 
#include <gnuplot.h>
#pragma hdrstop

/* COLOUR MODES - GLOBAL VARIABLES */

// (replaced with GnuPlot::SmPltt) t_sm_palette sm_palette_Removed; // initialized in plot.c on program entry 

/* Copy of palette previously in use.
 * Exported so that change_term() can invalidate contents
 */
static t_sm_palette prev_palette = { -1, (palette_color_mode)-1, -1, -1, -1, -1, -1, -1, (rgb_color*)0, -1 };

/* Internal prototype declarations: */

static void draw_inside_color_smooth_box_postscript();
static void cbtick_callback(GpAxis *, double place, char * text, int ticlevel, struct lp_style_type grid, struct ticmark * userlabels);

/* *******************************************************************
   ROUTINES
 */

//void init_color()
void GnuPlot::InitColor()
{
	// initialize global palette 
	SmPltt.colorFormulae = 37; /* const */
	SmPltt.formulaR = 7;
	SmPltt.formulaG = 5;
	SmPltt.formulaB = 15;
	SmPltt.Positive = SMPAL_POSITIVE;
	SmPltt.UseMaxColors = 0;
	SmPltt.Colors = 0;
	SmPltt.P_Color = NULL;
	SmPltt.ps_allcF = false;
	SmPltt.GradientNum = 0;
	SmPltt.P_Gradient = NULL;
	SmPltt.CModel = C_MODEL_RGB;
	SmPltt.Afunc.at = SmPltt.Bfunc.at = SmPltt.Cfunc.at = NULL;
	SmPltt.colorMode = SMPAL_COLOR_MODE_RGB;
	SmPltt.gamma = 1.5;
	SmPltt.GradientType = SMPAL_GRADIENT_TYPE_NONE;
}
// 
// Make the colour palette. Return 0 on success
// Put number of allocated colours into GPO.SmPltt.colors
// 
//int make_palette()
int GnuPlot::MakePalette()
{
	if(!term->make_palette)
		return 1;
	else {
		// ask for suitable number of colours in the palette 
		int i = term->make_palette(NULL);
		SmPltt.Colors = i;
		if(i == 0) {
			// terminal with its own mapping (PostScript, for instance)
			// It will not change palette passed below, but non-NULL has to be
			// passed there to create the header or force its initialization
			if(memcmp(&prev_palette, &SmPltt, sizeof(t_sm_palette))) {
				term->make_palette(&SmPltt);
				prev_palette = SmPltt;
				FPRINTF((stderr, "make_palette: calling term->make_palette for term with ncolors == 0\n"));
			}
			else {
				FPRINTF((stderr, "make_palette: skipping duplicate palette for term with ncolors == 0\n"));
			}
			return 0;
		}
		else {
			// set the number of colours to be used (allocated) 
			if(SmPltt.UseMaxColors > 0) {
				if(SmPltt.colorMode == SMPAL_COLOR_MODE_GRADIENT)
					SmPltt.Colors = i; // EAM Sep 2010 - could this be a constant? 
				else if(i > SmPltt.UseMaxColors)
					SmPltt.Colors = SmPltt.UseMaxColors;
			}
			if(prev_palette.colorFormulae < 0 || SmPltt.colorFormulae != prev_palette.colorFormulae || 
				SmPltt.colorMode != prev_palette.colorMode || SmPltt.formulaR != prev_palette.formulaR || 
				SmPltt.formulaG != prev_palette.formulaG || SmPltt.formulaB != prev_palette.formulaB || 
				SmPltt.Positive != prev_palette.Positive || SmPltt.Colors != prev_palette.Colors) {
				// print the message only if colors have changed 
				if(interactive)
					fprintf(stderr, "smooth palette in %s: using %i of %i available color positions\n", term->name, SmPltt.Colors, i);
			}
			prev_palette = SmPltt;
			ZFREE(SmPltt.P_Color);
			SmPltt.P_Color = (rgb_color *)gp_alloc(SmPltt.Colors * sizeof(rgb_color), "pm3d palette color");
			// fill SmPltt.color[]  
			for(i = 0; i < SmPltt.Colors; i++) {
				const double gray = (double)i / (SmPltt.Colors - 1); /* rescale to [0;1] */
				Rgb1FromGray(gray, &(SmPltt.P_Color[i]) );
			}
			// let the terminal make the palette from the supplied RGB triplets 
			term->make_palette(&SmPltt);
			return 0;
		}
	}
}
// 
// Force a mismatch between the current palette and whatever is sent next,
// so that the new one will always be loaded
// 
void invalidate_palette()
{
	prev_palette.Colors = -1;
}
// 
// Set the colour on the terminal
// Each terminal takes care of remembering the current colour,
// so there is not much to do here.
// FIXME: NaN could alternatively map to LT_NODRAW or TC_RGB full transparency
// 
void set_color(termentry * pTerm, double gray)
{
	t_colorspec color(isnan(gray) ? TC_LT : TC_FRAC, LT_BACKGROUND, gray);
	//color.value = gray;
	//color.lt = LT_BACKGROUND;
	//color.type = (isnan(gray)) ? TC_LT : TC_FRAC;
	pTerm->set_color(&color);
}

void set_rgbcolor_var(uint rgbvalue)
{
	t_colorspec color(TC_RGB, static_cast<int>(rgbvalue), -1.0/* -1 flags that this came from "rgb variable" */);
	//color.type = TC_RGB;
	//*(uint*)(&color.lt) = rgbvalue;
	//color.value = -1; /* -1 flags that this came from "rgb variable" */
	GPO.ApplyPm3DColor(term, &color);
}

void set_rgbcolor_const(uint rgbvalue)
{
	t_colorspec color(TC_RGB, static_cast<int>(rgbvalue), 0.0/* 0 flags that this is a constant color */);
	//color.type = TC_RGB;
	//*(uint*)(&color.lt) = rgbvalue;
	//color.value = 0; /* 0 flags that this is a constant color */
	GPO.ApplyPm3DColor(term, &color);
}
// 
// diagnose the palette gradient in three types.
//   1. Smooth gradient (SMPAL_GRADIENT_TYPE_SMOOTH)
//   2. Discrete gradient (SMPAL_GRADIENT_TYPE_DISCRETE)
//   3. Smooth and Discrete Mixed gradient (SMPAL_GRADIENT_TYPE_MIXED)
// 
//void check_palette_gradient_type()
void t_sm_palette::CheckGradientType()
{
	if(colorMode != SMPAL_COLOR_MODE_GRADIENT) {
		GradientType = SMPAL_GRADIENT_TYPE_SMOOTH;
	}
	else {
		int    has_smooth_part   = 0;
		int    has_discrete_part = 0;
		double p1 = P_Gradient[0].pos;
		rgb_color c1 = P_Gradient[0].col;
		for(int j = 1; j < GradientNum; j++) {
			const double p2 = P_Gradient[j].pos;
			const rgb_color c2 = P_Gradient[j].col;
			if(p1 == p2)
				has_discrete_part = 1;
			else if(c1.r == c2.r && c1.g == c2.g && c1.b == c2.b)
				has_discrete_part = 1;
			else
				has_smooth_part = 1;
			p1 = p2;
			c1 = c2;
		}
		if(!has_discrete_part)
			GradientType = SMPAL_GRADIENT_TYPE_SMOOTH;
		else if(!has_smooth_part)
			GradientType = SMPAL_GRADIENT_TYPE_DISCRETE;
		else
			GradientType = SMPAL_GRADIENT_TYPE_MIXED;
	}
}
/*
   Draw colour smooth box

   Firstly two helper routines for plotting inside of the box
   for postscript and for other terminals, finally the main routine
 */

/* plot the colour smooth box for from terminal's integer coordinates
   This routine is for postscript files --- actually, it writes a small
   PS routine.
 */
static void draw_inside_color_smooth_box_postscript()
{
	int scale_x = (color_box.bounds.xright - color_box.bounds.xleft), scale_y = (color_box.bounds.ytop - color_box.bounds.ybot);
	fputs("stroke gsave\t%% draw gray scale smooth box\nmaxcolors 0 gt {/imax maxcolors def} {/imax 1024 def} ifelse\n", gppsfile);
	/* nb. of discrete steps (counted in the loop) */
	fprintf(gppsfile, "%i %i translate %i %i scale 0 setlinewidth\n", color_box.bounds.xleft, color_box.bounds.ybot, scale_x, scale_y);
	/* define left bottom corner and scale of the box so that all coordinates
	   of the box are from [0,0] up to [1,1]. Further, this normalization
	   makes it possible to pass y from [0,1] as parameter to setgray */
	fprintf(gppsfile, "/ystep 1 imax div def /y0 0 def /ii 0 def\n");
	/* local variables; y-step, current y position and counter ii;  */
	if(GPO.SmPltt.Positive == SMPAL_NEGATIVE) /* inverted gray for negative figure */
		fputs("{ 0.99999 y0 sub g ", gppsfile); /* 1 > x > 1-1.0/1024 */
	else
		fputs("{ y0 g ", gppsfile);
	if(color_box.rotation == 'v')
		fputs("0 y0 N 1 0 V 0 ystep V -1 0 f\n", gppsfile);
	else
		fputs("y0 0 N 0 1 V ystep 0 V 0 -1 f\n", gppsfile);
	fputs("/y0 y0 ystep add def /ii ii 1 add def\nii imax ge {exit} if } loop\ngrestore 0 setgray\n", gppsfile);
}
// 
// plot a colour smooth box bounded by the terminal's integer coordinates
// [x_from,y_from] to [x_to,y_to].
// This routine is for non-postscript files and for the Mixed color gradient type
// 
static void draw_inside_colorbox_bitmap_mixed()
{
	int i, j, xy, xy2, xy_from, xy_to;
	int jmin = 0;
	double xy_step, gray, range;
	gpiPoint corners[4];
	int steps = 128; // I think that nobody can distinguish more colours drawn in the palette
	if(GPO.SmPltt.UseMaxColors != 0) {
		steps = GPO.SmPltt.UseMaxColors;
	}
	else if(GPO.SmPltt.GradientNum > 128) {
		steps = GPO.SmPltt.GradientNum;
	}
	if(color_box.rotation == 'v') {
		corners[0].x = corners[3].x = color_box.bounds.xleft;
		corners[1].x = corners[2].x = color_box.bounds.xright;
		xy_from = color_box.bounds.ybot;
		xy_to = color_box.bounds.ytop;
		xy_step = (color_box.bounds.ytop - color_box.bounds.ybot) / (double)steps;
	}
	else {
		corners[0].y = corners[1].y = color_box.bounds.ybot;
		corners[2].y = corners[3].y = color_box.bounds.ytop;
		xy_from = color_box.bounds.xleft;
		xy_to = color_box.bounds.xright;
		xy_step = (color_box.bounds.xright - color_box.bounds.xleft) / (double)steps;
	}
	range = (xy_to - xy_from);
	for(i = 0, xy2 = xy_from; i < steps; i++) {
		/* Start from one pixel beyond the previous box */
		xy = xy2;
		xy2 = xy_from + (int)(xy_step * (i + 1));
		/* Set the colour for the next range increment */
		/* FIXME - The "1 +" seems wrong, yet it improves the placement in gd */
		gray = (double)(1 + xy - xy_from) / range;
		if(GPO.SmPltt.Positive == SMPAL_NEGATIVE)
			gray = 1 - gray;
		set_color(term, gray);
		/* If this is a defined palette, make sure that the range increment */
		/* does not straddle a palette segment boundary. If it does, split  */
		/* it into two parts.                                               */
		if(GPO.SmPltt.colorMode == SMPAL_COLOR_MODE_GRADIENT) {
			for(j = jmin; j<GPO.SmPltt.GradientNum; j++) {
				const int boundary = xy_from + (int)(GPO.SmPltt.P_Gradient[j].pos * range);
				if(xy >= boundary) {
					jmin = j;
				}
				else if(xy2 > boundary) {
					xy2 = boundary;
					i--;
					break;
				}
				if(xy2 < boundary)
					break;
			}
		}
		if(color_box.rotation == 'v') {
			corners[0].y = corners[1].y = xy;
			corners[2].y = corners[3].y = MIN(xy_to, xy2+1);
		}
		else {
			corners[0].x = corners[3].x = xy;
			corners[1].x = corners[2].x = MIN(xy_to, xy2+1);
		}
		// print the rectangle with the given colour 
		if(default_fillstyle.fillstyle == FS_EMPTY)
			corners->style = FS_OPAQUE;
		else
			corners->style = style_from_fill(&default_fillstyle);
		term->filled_polygon(4, corners);
	}
}
// 
// plot a colour smooth box bounded by the terminal's integer coordinates
// [x_from,y_from] to [x_to,y_to].
// This routine is for non-postscript files and for the Discrete color gradient type
// 
static void draw_inside_colorbox_bitmap_discrete()
{
	int i, i0, i1, xy, xy2, xy_from, xy_to;
	double gray, range;
	gpiPoint corners[4];
	int steps = GPO.SmPltt.GradientNum;
	if(color_box.rotation == 'v') {
		corners[0].x = corners[3].x = color_box.bounds.xleft;
		corners[1].x = corners[2].x = color_box.bounds.xright;
		xy_from = color_box.bounds.ybot;
		xy_to = color_box.bounds.ytop;
	}
	else {
		corners[0].y = corners[1].y = color_box.bounds.ybot;
		corners[2].y = corners[3].y = color_box.bounds.ytop;
		xy_from = color_box.bounds.xleft;
		xy_to = color_box.bounds.xright;
	}
	range = (xy_to - xy_from);
	for(i = 0; i < steps-1; i++) {
		if(GPO.SmPltt.Positive == SMPAL_NEGATIVE) {
			i0 = steps-1 - i;
			i1 = i0 - 1;
		}
		else {
			i0 = i;
			i1 = i0 + 1;
		}
		xy  = xy_from + (int)(GPO.SmPltt.P_Gradient[i0].pos * range);
		xy2 = xy_from + (int)(GPO.SmPltt.P_Gradient[i1].pos * range);
		if(xy2 - xy == 0) {
			continue;
		}
		gray = GPO.SmPltt.P_Gradient[i1].pos;
		set_color(term, gray);
		if(color_box.rotation == 'v') {
			corners[0].y = corners[1].y = xy;
			corners[2].y = corners[3].y = MIN(xy_to, xy2+1);
		}
		else {
			corners[0].x = corners[3].x = xy;
			corners[1].x = corners[2].x = MIN(xy_to, xy2+1);
		}
		// print the rectangle with the given colour 
		if(default_fillstyle.fillstyle == FS_EMPTY)
			corners->style = FS_OPAQUE;
		else
			corners->style = style_from_fill(&default_fillstyle);
		term->filled_polygon(4, corners);
	}
}
// 
// plot a colour smooth box bounded by the terminal's integer coordinates [x_from,y_from] to [x_to,y_to].
// This routine is for non-postscript files and for the Smooth color gradient type
// 
static void draw_inside_colorbox_bitmap_smooth()
{
	int i, xy, xy2, xy_from, xy_to;
	double xy_step, gray;
	gpiPoint corners[4];
	// Determins the steps for rectangles boxes from palette's color number specification. 
	int steps = 128; /* I think that nobody can distinguish more colours drawn in the palette */
	if(GPO.SmPltt.UseMaxColors != 0)
		steps = GPO.SmPltt.UseMaxColors;
	else if(GPO.SmPltt.GradientNum > 128)
		steps = GPO.SmPltt.GradientNum;
	if(color_box.rotation == 'v') {
		corners[0].x = corners[3].x = color_box.bounds.xleft;
		corners[1].x = corners[2].x = color_box.bounds.xright;
		xy_from = color_box.bounds.ybot;
		xy_to = color_box.bounds.ytop;
		xy_step = (color_box.bounds.ytop - color_box.bounds.ybot) / (double)steps;
	}
	else {
		corners[0].y = corners[1].y = color_box.bounds.ybot;
		corners[2].y = corners[3].y = color_box.bounds.ytop;
		xy_from = color_box.bounds.xleft;
		xy_to = color_box.bounds.xright;
		xy_step = (color_box.bounds.xright - color_box.bounds.xleft) / (double)steps;
	}
	for(i = 0, xy2 = xy_from; i < steps; i++) {
		xy = xy2;
		xy2 = xy_from + (int)(xy_step * (i + 1));
		gray = i / (double)steps;
		if(GPO.SmPltt.UseMaxColors != 0) {
			gray = GPO.QuantizeGray(gray);
		}
		if(GPO.SmPltt.Positive == SMPAL_NEGATIVE)
			gray = 1 - gray;
		set_color(term, gray);
		if(color_box.rotation == 'v') {
			corners[0].y = corners[1].y = xy;
			corners[2].y = corners[3].y = MIN(xy_to, xy2+1);
		}
		else {
			corners[0].x = corners[3].x = xy;
			corners[1].x = corners[2].x = MIN(xy_to, xy2+1);
		}
		// print the rectangle with the given colour 
		corners->style = (default_fillstyle.fillstyle == FS_EMPTY) ? FS_OPAQUE : style_from_fill(&default_fillstyle);
		term->filled_polygon(4, corners);
	}
}

static void cbtick_callback(GpAxis * this_axis, double place, char * text, int ticlevel, struct lp_style_type grid/* linetype or -2 for no grid */, struct ticmark * userlabels)
{
	int len = tic_scale(ticlevel, this_axis) * (this_axis->tic_in ? -1 : 1) * (term->TicH);
	uint x1, y1, x2, y2;
	double cb_place;
	// position of tic as a fraction of the full palette range 
	if(this_axis->linked_to_primary) {
		const GpAxis * primary = this_axis->linked_to_primary;
		place = GPO.EvalLinkFunction(primary, place);
		cb_place = (place - primary->min) / (primary->max - primary->min);
	}
	else
		cb_place = (place - this_axis->min) / (this_axis->max - this_axis->min);
	// calculate tic position 
	if(color_box.rotation == 'h') {
		x1 = x2 = color_box.bounds.xleft + cb_place * (color_box.bounds.xright - color_box.bounds.xleft);
		y1 = color_box.bounds.ybot;
		y2 = color_box.bounds.ybot - len;
	}
	else {
		x1 = color_box.bounds.xright;
		x2 = color_box.bounds.xright + len;
		y1 = y2 = color_box.bounds.ybot + cb_place * (color_box.bounds.ytop - color_box.bounds.ybot);
	}
	// draw grid line 
	if(grid.l_type > LT_NODRAW) {
		GPO.TermApplyLpProperties(term, &grid); /* grid linetype */
		if(color_box.rotation == 'h') {
			(*term->move)(x1, color_box.bounds.ybot);
			(*term->vector)(x1, color_box.bounds.ytop);
		}
		else {
			(*term->move)(color_box.bounds.xleft, y1);
			(*term->vector)(color_box.bounds.xright, y1);
		}
		GPO.TermApplyLpProperties(term, &border_lp); /* border linetype */
	}
	// draw tic 
	if(len != 0) {
		int lt = color_box.cbtics_lt_tag;
		if(lt <= 0)
			lt = color_box.border_lt_tag;
		if(lt > 0) {
			lp_style_type lp = border_lp;
			lp_use_properties(&lp, lt);
			GPO.TermApplyLpProperties(term, &lp);
		}
		(*term->move)(x1, y1);
		(*term->vector)(x2, y2);
		if(this_axis->ticmode & TICS_MIRROR) {
			if(color_box.rotation == 'h') {
				y1 = color_box.bounds.ytop;
				y2 = color_box.bounds.ytop + len;
			}
			else {
				x1 = color_box.bounds.xleft;
				x2 = color_box.bounds.xleft - len;
			}
			(*term->move)(x1, y1);
			(*term->vector)(x2, y2);
		}
		if(lt != 0)
			GPO.TermApplyLpProperties(term, &border_lp);
	}
	// draw label 
	if(text) {
		int just;
		int offsetx, offsety;
		// Skip label if we've already written a user-specified one here 
#define MINIMUM_SEPARATION 0.001
		while(userlabels) {
			if(fabs((place - userlabels->position) / (GPO.AxS.__CB().max - GPO.AxS.__CB().min)) <= MINIMUM_SEPARATION) {
				text = NULL;
				break;
			}
			userlabels = userlabels->next;
		}
#undef MINIMUM_SEPARATION
		// get offset 
		GPO.Map3DPositionR(&(this_axis->ticdef.offset), &offsetx, &offsety, "cbtics");
		// User-specified different color for the tics text 
		if(this_axis->ticdef.textcolor.type != TC_DEFAULT)
			GPO.ApplyPm3DColor(term, &(this_axis->ticdef.textcolor));
		if(color_box.rotation == 'h') {
			int y3 = color_box.bounds.ybot - (term->ChrV);
			int hrotate = 0;
			if(this_axis->tic_rotate && (*term->text_angle)(this_axis->tic_rotate))
				hrotate = this_axis->tic_rotate;
			if(len > 0) y3 -= len; /* add outer tics len */
			if(y3<0) y3 = 0;
			just = hrotate ? LEFT : CENTRE;
			if(this_axis->manual_justify)
				just = this_axis->tic_pos;
			write_multiline(term, x2+offsetx, y3+offsety, text, (JUSTIFY)just, JUST_CENTRE, hrotate, this_axis->ticdef.font);
			if(hrotate)
				(*term->text_angle)(0);
		}
		else {
			uint x3 = color_box.bounds.xright + (term->ChrH);
			if(len > 0) 
				x3 += len; // add outer tics len 
			just = LEFT;
			if(this_axis->manual_justify)
				just = this_axis->tic_pos;
			write_multiline(term, x3+offsetx, y2+offsety, text, (JUSTIFY)just, JUST_CENTRE, 0.0, this_axis->ticdef.font);
		}
		GPO.TermApplyLpProperties(term, &border_lp); /* border linetype */
	}
}
// 
// Finally the main colour smooth box drawing routine
// 
//void draw_color_smooth_box(termentry * pTerm, int plot_mode)
void GnuPlot::DrawColorSmoothBox(termentry * pTerm, int plotMode)
{
	if(color_box.where == SMCOLOR_BOX_NO)
		return;
	if(!pTerm->filled_polygon)
		return;
	// 
	// firstly, choose some good position of the color box
	// 
	// user's position like that (?):
	// else {
	// x_from = color_box.xlow;
	// x_to   = color_box.xhigh;
	// }
	// 
	if(color_box.where == SMCOLOR_BOX_USER) {
		if(!is_3d_plot) {
			double xtemp, ytemp;
			MapPosition(pTerm, &color_box.origin, &color_box.bounds.xleft, &color_box.bounds.ybot, "cbox");
			MapPositionR(pTerm, &color_box.size, &xtemp, &ytemp, "cbox");
			color_box.bounds.xright = xtemp;
			color_box.bounds.ytop = ytemp;
		}
		else if(splot_map && is_3d_plot) {
			// In map view mode we allow any coordinate system for placement 
			double xtemp, ytemp;
			Map3DPositionDouble(&color_box.origin, &xtemp, &ytemp, "cbox");
			color_box.bounds.xleft = xtemp;
			color_box.bounds.ybot = ytemp;
			Map3DPositionR(&color_box.size, &color_box.bounds.xright, &color_box.bounds.ytop, "cbox");
		}
		else {
			// But in full 3D mode we only allow screen coordinates 
			color_box.bounds.xleft = color_box.origin.x * (pTerm->xmax) + 0.5;
			color_box.bounds.ybot = color_box.origin.y * (pTerm->ymax) + 0.5;
			color_box.bounds.xright = color_box.size.x * (pTerm->xmax-1) + 0.5;
			color_box.bounds.ytop = color_box.size.y * (pTerm->ymax-1) + 0.5;
		}
		color_box.bounds.xright += color_box.bounds.xleft;
		color_box.bounds.ytop += color_box.bounds.ybot;
	}
	else { // color_box.where == SMCOLOR_BOX_DEFAULT 
		if(plotMode == MODE_SPLOT && !splot_map) {
			// general 3D plot 
			color_box.bounds.xleft = xmiddle + 0.709 * xscaler;
			color_box.bounds.xright   = xmiddle + 0.778 * xscaler;
			color_box.bounds.ybot = ymiddle - 0.147 * yscaler;
			color_box.bounds.ytop   = ymiddle + 0.497 * yscaler;
		}
		else {
			// 2D plot (including splot map) 
			GpPosition default_origin = {graph, graph, graph, 1.025, 0, 0};
			GpPosition default_size = {graph, graph, graph, 0.05, 1.0, 0};
			double xtemp, ytemp;
			MapPosition(pTerm, &default_origin, &color_box.bounds.xleft, &color_box.bounds.ybot, "cbox");
			color_box.bounds.xleft += color_box.xoffset;
			MapPositionR(pTerm, &default_size, &xtemp, &ytemp, "cbox");
			color_box.bounds.xright = xtemp + color_box.bounds.xleft;
			color_box.bounds.ytop = ytemp + color_box.bounds.ybot;
		}
		// now corrections for outer tics 
		if(color_box.rotation == 'v') {
			int cblen = static_cast<int>((AxS.__CB().tic_in ? -1 : 1) * AxS.__CB().ticscale * (pTerm->TicH)); // positive for outer tics 
			int ylen  = static_cast<int>((AxS.__Y().tic_in ? -1 : 1) * AxS.__Y().ticscale * (pTerm->TicH)); // positive for outer tics 
			if((cblen > 0) && (AxS.__CB().ticmode & TICS_MIRROR)) {
				color_box.bounds.xleft += cblen;
				color_box.bounds.xright += cblen;
			}
			if((ylen > 0) && (AxS[FIRST_Y_AXIS].ticmode & TICS_MIRROR)) {
				color_box.bounds.xleft += ylen;
				color_box.bounds.xright += ylen;
			}
		}
	}
	if(color_box.bounds.ybot > color_box.bounds.ytop) {
		double tmp = color_box.bounds.ytop;
		color_box.bounds.ytop = color_box.bounds.ybot;
		color_box.bounds.ybot = tmp;
	}
	if(color_box.invert && color_box.rotation == 'v') {
		double tmp = color_box.bounds.ytop;
		color_box.bounds.ytop = color_box.bounds.ybot;
		color_box.bounds.ybot = tmp;
	}
	pTerm->layer(TERM_LAYER_BEGIN_COLORBOX);
	// The PostScript terminal has an Optimized version 
	if((pTerm->flags & TERM_IS_POSTSCRIPT) != 0)
		draw_inside_color_smooth_box_postscript();
	else {
		if(SmPltt.GradientType == SMPAL_GRADIENT_TYPE_SMOOTH)
			draw_inside_colorbox_bitmap_smooth();
		else if(SmPltt.GradientType == SMPAL_GRADIENT_TYPE_DISCRETE)
			draw_inside_colorbox_bitmap_discrete();
		else
			draw_inside_colorbox_bitmap_mixed();
	}
	pTerm->layer(TERM_LAYER_END_COLORBOX);
	if(color_box.border) {
		// now make boundary around the colour box 
		if(color_box.border_lt_tag >= 0) {
			// user specified line type 
			lp_style_type lp = border_lp;
			lp_use_properties(&lp, color_box.border_lt_tag);
			TermApplyLpProperties(pTerm, &lp);
		}
		else
			TermApplyLpProperties(pTerm, &border_lp); // black solid colour should be chosen, so it's border linetype 
		newpath(pTerm);
		(pTerm->move)(color_box.bounds.xleft, color_box.bounds.ybot);
		(pTerm->vector)(color_box.bounds.xright, color_box.bounds.ybot);
		(pTerm->vector)(color_box.bounds.xright, color_box.bounds.ytop);
		(pTerm->vector)(color_box.bounds.xleft, color_box.bounds.ytop);
		(pTerm->vector)(color_box.bounds.xleft, color_box.bounds.ybot);
		closepath(pTerm);
		// Set line properties to some value, this also draws lines in postscript terminals. 
		TermApplyLpProperties(pTerm, &border_lp);
	}
	// draw tics 
	if(AxS[COLOR_AXIS].ticmode) {
		TermApplyLpProperties(pTerm, &border_lp); /* border linetype */
		gen_tics(&AxS[COLOR_AXIS], cbtick_callback);
	}
	// write the colour box label 
	if(AxS.__CB().label.text) {
		int x, y;
		int len;
		int save_rotation = AxS.__CB().label.rotate;
		ApplyPm3DColor(pTerm, &(AxS.__CB().label.textcolor));
		if(color_box.rotation == 'h') {
			len = AxS.__CB().ticscale * (AxS.__CB().tic_in ? 1 : -1) * (pTerm->TicV);
			x = (color_box.bounds.xleft + color_box.bounds.xright) / 2;
			y = color_box.bounds.ybot - 2.7 * pTerm->ChrV;
			if(len < 0) 
				y += len;
			if(AxS.__CB().label.rotate == TEXT_VERTICAL)
				AxS.__CB().label.rotate = 0;
		}
		else {
			len = AxS.__CB().ticscale * (AxS.__CB().tic_in ? -1 : 1) * (pTerm->TicH);
			// calculate max length of cb-tics labels 
			widest_tic_strlen = 0;
			if(AxS.__CB().ticmode & TICS_ON_BORDER) /* Recalculate widest_tic_strlen */
				gen_tics(&AxS[COLOR_AXIS], widest_tic_callback);
			x = color_box.bounds.xright + (widest_tic_strlen + 1.5) * pTerm->ChrH;
			if(len > 0) 
				x += len;
			y = (color_box.bounds.ybot + color_box.bounds.ytop) / 2;
		}
		SETMAX(x, 0);
		SETMAX(y, 0);
		write_label(pTerm, x, y, &(AxS.__CB().label));
		reset_textcolor(&(AxS.__CB().label.textcolor));
		AxS.__CB().label.rotate = save_rotation;
	}
}
// 
// User-callable builtin color conversion
// 
//void f_hsv2rgb(union argument * /*arg*/)
void GnuPlot::F_Hsv2Rgb(union argument * /*arg*/)
{
	GpValue h, s, v, result;
	rgb_color color = {0., 0., 0.};
	EvStk.Pop(&v);
	EvStk.Pop(&s);
	EvStk.Pop(&h);
	if(h.type == INTGR)
		color.r = h.v.int_val;
	else if(h.type == CMPLX)
		color.r = h.v.cmplx_val.real;
	if(s.type == INTGR)
		color.g = s.v.int_val;
	else if(s.type == CMPLX)
		color.g = s.v.cmplx_val.real;
	if(v.type == INTGR)
		color.b = v.v.int_val;
	else if(v.type == CMPLX)
		color.b = v.v.cmplx_val.real;
	SETMAX(color.r, 0.0);
	SETMAX(color.g, 0.0);
	SETMAX(color.b, 0.0);
	SETMIN(color.r, 1.0);
	SETMIN(color.g, 1.0);
	SETMIN(color.b, 1.0);
	Ginteger(&result, hsv2rgb(&color));
	EvStk.Push(&result);
}
// 
// user-callable lookup of palette color for specific z-value
// 
//void f_palette(union argument * arg)
void GnuPlot::F_Palette(union argument * arg)
{
	GpValue result;
	double z;
	rgb255_color color;
	uint rgb;
	EvStk.Pop(&result);
	z = real(&result);
	if(((AxS.__CB().set_autoscale & AUTOSCALE_BOTH) != 0) && (fabs(AxS.__CB().min) >= VERYLARGE || fabs(AxS.__CB().max) >= VERYLARGE))
		IntError(NO_CARET, "palette(z) requires known cbrange");
	Rgb255MaxColorsFromGray(Cb2Gray(z), &color);
	rgb = (uint)color.r << 16 | (uint)color.g << 8 | (uint)color.b;
	EvStk.Push(Ginteger(&result, rgb));
}
// 
// User-callable interpretation of a string as a 24bit RGB color
// replicating the colorspec interpretation in e.g. 'linecolor rgb "foo"'.
// 
//void f_rgbcolor(union argument * arg)
void GnuPlot::F_RgbColor(union argument * arg)
{
	GpValue a;
	long rgb;
	EvStk.Pop(&a);
	if(a.type == STRING) {
		rgb = lookup_color_name(a.v.string_val);
		if(rgb == -2)
			rgb = 0;
		SAlloc::F(a.v.string_val);
	}
	else {
		rgb = 0;
	}
	EvStk.Push(Ginteger(&a, rgb));
}
//
// A colormap can have specific min/max stored internally,
// but otherwise we use the current cbrange
//
double map2gray(double z, udvt_entry * colormap)
{
	double gray;
	double cm_min, cm_max;
	get_colormap_range(colormap, &cm_min, &cm_max);
	if(cm_min == cm_max)
		gray = GPO.Cb2Gray(z);
	else
		gray = (z - cm_min) / (cm_max - cm_min);
	return gray;
}

void get_colormap_range(udvt_entry * colormap, double * cm_min, double * cm_max)
{
	*cm_min = colormap->udv_value.v.value_array[1].v.cmplx_val.imag;
	*cm_max = colormap->udv_value.v.value_array[2].v.cmplx_val.imag;
}
// 
// gray is in the interval [0:1]
// colormap is an ARRAY containing a palette of 32-bit ARGB values
// 
uint rgb_from_colormap(double gray, udvt_entry * colormap)
{
	GpValue * palette = colormap->udv_value.v.value_array;
	int size = palette[0].v.int_val;
	uint rgb = (gray <= 0.0) ? palette[1].v.int_val : (gray >= 1.0) ? palette[size].v.int_val : palette[ (int)(floor(size * gray)) + 1].v.int_val;
	return rgb;
}
/*
 * Interpret the colorspec of a linetype to yield an RGB packed integer.
 * This is not guaranteed to handle colorspecs that were not part of a linetype.
 */
uint rgb_from_colorspec(struct t_colorspec * tc)
{
	double cbval;
	rgb255_color color;
	switch(tc->type) {
		case TC_DEFAULT:
		    return 0;
		case TC_RGB:
		    return tc->lt;
		case TC_Z:
		    cbval = GPO.Cb2Gray(tc->value);
		    break;
		case TC_CB:
		    cbval = (GPO.AxS.__CB().log && tc->value <= 0) ? GPO.AxS.__CB().min : tc->value;
		    cbval = GPO.Cb2Gray(cbval);
		    break;
		case TC_FRAC:
		    cbval = (GPO.SmPltt.Positive == SMPAL_POSITIVE) ?  tc->value : 1-tc->value;
		    break;
		case TC_COLORMAP:
		/* not handled but perhaps it should be? */
		default:
		    return 0; // cannot happen in a linetype 
	}
	GPO.Rgb255MaxColorsFromGray(cbval, &color);
	return (uint)color.r << 16 | (uint)color.g << 8 | (uint)color.b;
}
