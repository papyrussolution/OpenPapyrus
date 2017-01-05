/* GNUPLOT - color.c */

/*[
 *
 * Petr Mikulik, since December 1998
 * Copyright: open source as much as possible
 *
 * What is here:
 *   - Global variables declared in .h are initialized here
 *   - Palette routines
 *   - Colour box drawing
 *
   ]*/
#include <gnuplot.h>
#pragma hdrstop
//
// COLOUR MODES - GLOBAL VARIABLES
//
//t_sm_palette sm_palette; // @global initialized in plot.c on program entry

/* Copy of palette previously in use.
 * Exported so that change_term() can invalidate contents
 * FIXME: better naming
 */
static t_sm_palette prev_palette = { // @global 
	-1, (palette_color_mode)-1, -1, -1, -1, -1, -1, -1, (rgb_color*)0, /*-1*/true
};
//
// Internal prototype declarations:
//
//static void draw_inside_color_smooth_box_postscript();
//static void draw_inside_color_smooth_box_bitmap();
//static void cbtick_callback(GpTermEntry * pT, GpTicCallbackParam * pP /* GpAxis *, double place, char * text, int ticlevel, lp_style_type grid, ticmark * userlabels */);

/* *******************************************************************
   ROUTINES
 */

//void init_color()
void GpGadgets::InitColor()
{
	// initialize global palette
	SmPalette.colorFormulae = 37; /* const */
	SmPalette.formulaR = 7;
	SmPalette.formulaG = 5;
	SmPalette.formulaB = 15;
	SmPalette.positive = SMPAL_POSITIVE;
	SmPalette.use_maxcolors = 0;
	SmPalette.colors = 0;
	SmPalette.color = NULL;
	SmPalette.ps_allcF = false;
	SmPalette.gradient_num = 0;
	SmPalette.gradient = NULL;
	SmPalette.cmodel = C_MODEL_RGB;
	SmPalette.Afunc.at = SmPalette.Bfunc.at = SmPalette.Cfunc.at = NULL;
	SmPalette.colorMode = SMPAL_COLOR_MODE_RGB;
	SmPalette.gamma = 1.5;
}
//
// Make the colour palette. Return 0 on success
// Put number of allocated colours into sm_palette.colors
//
//int make_palette()
int GpGadgets::MakePalette(GpTermEntry * pT)
{
	double gray;
	if(!pT->make_palette) {
		return 1;
	}
	else {
		// ask for suitable number of colours in the palette
		int    i = pT->make_palette(NULL);
		SmPalette.colors = i;
		if(i == 0) {
			// terminal with its own mapping (PostScript, for instance)
			// It will not change palette passed below, but non-NULL has to be
			// passed there to create the header or force its initialization
			if(memcmp(&prev_palette, &SmPalette, sizeof(t_sm_palette))) {
				pT->make_palette(&SmPalette);
				prev_palette = SmPalette;
				FPRINTF((stderr, "make_palette: calling pT->make_palette for pT with ncolors == 0\n"));
			}
			else {
				FPRINTF((stderr, "make_palette: skipping duplicate palette for pT with ncolors == 0\n"));
			}
		}
		else {
			// set the number of colours to be used (allocated) 
			if(SmPalette.use_maxcolors > 0) {
				if(SmPalette.colorMode == SMPAL_COLOR_MODE_GRADIENT)
					SmPalette.colors = i;  /* EAM Sep 2010 - could this be a constant? */
				else if(i > SmPalette.use_maxcolors)
					SmPalette.colors = SmPalette.use_maxcolors;
			}
			if(prev_palette.colorFormulae < 0 || SmPalette.colorFormulae != prev_palette.colorFormulae
				|| SmPalette.colorMode != prev_palette.colorMode || SmPalette.formulaR != prev_palette.formulaR
				|| SmPalette.formulaG != prev_palette.formulaG || SmPalette.formulaB != prev_palette.formulaB
				|| SmPalette.positive != prev_palette.positive || SmPalette.colors != prev_palette.colors) {
				// print the message only if colors have changed
				if(IsInteractive)
					fprintf(stderr, "smooth palette in %s: using %i of %i available color positions\n", 
						pT->name, SmPalette.colors, i);
			}
			prev_palette = SmPalette;
			free(SmPalette.color);
			SmPalette.color = (rgb_color *)malloc(SmPalette.colors * sizeof(rgb_color));
			// fill SmPalette.color[] 
			for(i = 0; i < SmPalette.colors; i++) {
				gray = (double)i / (SmPalette.colors - 1); /* rescale to [0;1] */
				RGB1FromGray(gray, &(SmPalette.color[i]) );
			}
			// let the terminal make the palette from the supplied RGB triplets 
			pT->make_palette(&SmPalette);
		}
		return 0;
	}
}
//
// Force a mismatch between the current palette and whatever is sent next,
// so that the new one will always be loaded
//
void invalidate_palette()
{
	prev_palette.colors = -1;
}
//
// Set the colour on the terminal
// Each terminal takes care of remembering the current colour,
// so there is not much to do here.
// FIXME: NaN could alternatively map to LT_NODRAW or TC_RGB full transparency
//
//void set_color(double gray)
void GpTermEntry::SetColor(double gray)
{
	t_colorspec color;
	color.value = gray;
	color.lt = LT_BACKGROUND;
	color.type = fisnan(gray) ? TC_LT : TC_FRAC;
	(*set_color)(&color);
}

//void set_rgbcolor_var(uint rgbvalue)
void GpGadgets::SetRgbColorVar(GpTermEntry * pT, uint rgbvalue)
{
	t_colorspec color;
	color.type = TC_RGB;
	*(uint*)(&color.lt) = rgbvalue;
	color.value = -1; // -1 flags that this came from "rgb variable" 
	ApplyPm3DColor(pT, &color);
}

//void set_rgbcolor_const(uint rgbvalue)
void GpGadgets::SetRgbColorConst(GpTermEntry * pT, uint rgbvalue)
{
	t_colorspec color;
	color.type = TC_RGB;
	*(uint*)(&color.lt) = rgbvalue;
	color.value = 0; // 0 flags that this is a constant color
	ApplyPm3DColor(pT, &color);
}

//void ifilled_quadrangle(gpiPoint * pICorners)
void GpGadgets::IFilledQuadrangle(GpTermEntry * pT, gpiPoint * pICorners)
{
	pICorners->style = (DefaultFillStyle.fillstyle == FS_EMPTY) ? FS_OPAQUE : style_from_fill(&DefaultFillStyle);
	pT->filled_polygon(4, pICorners);
	if(Pm3D.border.l_type != LT_NODRAW) {
		// It should be sufficient to set only the color, but for some 
		// reason this causes the svg terminal to lose the fill type.  
		ApplyLpProperties(pT, &Pm3DBorderLp);
		pT->move(pICorners[0].x, pICorners[0].y);
		for(int i = 3; i >= 0; i--) {
			pT->vector(pICorners[i].x, pICorners[i].y);
		}
	}
}

/* The routine above for 4 points explicitly.
 * This is the only routine which supportes extended
 * color specs currently.
 */
//#ifdef EXTENDED_COLOR_SPECS
//void filled_quadrangle(gpdPoint * corners, gpiPoint * icorners)
//#else
//void filled_quadrangle(gpdPoint * corners)
//#endif
#ifdef EXTENDED_COLOR_SPECS
void GpGadgets::FilledQuadrangle(GpTermEntry * pT, gpdPoint * corners, gpiPoint * icorners)
#else
void GpGadgets::FilledQuadrangle(GpTermEntry * pT, gpdPoint * corners)
#endif
{
#ifndef EXTENDED_COLOR_SPECS
	gpiPoint icorners[4];
#endif
	for(int i = 0; i < 4; i++) {
		double x, y;
		Map3DXY(corners[i].x, corners[i].y, corners[i].z, &x, &y);
		icorners[i].x = (int)x;
		icorners[i].y = (int)y;
	}
	IFilledQuadrangle(pT, icorners);
}

#ifdef PM3D_CONTOURS
//
// Makes mapping from real 3D coordinates, passed as coords array,
// to 2D terminal coordinates, then draws filled polygon
//
//void filled_polygon_common(int points, const GpCoordinate & rCoords, bool fixed, double z)
void GpGadgets::FilledPolygonCommon(GpTermEntry * pT, int points, const GpCoordinate & rCoords, bool fixed, double z)
{
	double x, y;
	gpiPoint * icorners = malloc(points * sizeof(gpiPoint));
	for(int i = 0; i < points; i++) {
		if(fixed)
			z = rCoords[i].z;
		Map3DXY(rCoords[i].x, rCoords[i].y, z, &x, &y);
		icorners[i].x = x;
		icorners[i].y = y;
	}
#ifdef EXTENDED_COLOR_SPECS
	if((pT->flags & TERM_EXTENDED_COLOR)) {
		icorners[0].spec.gray = -1; // force solid color 
	}
#endif
	icorners->style = (DefaultFillStyle.fillstyle == FS_EMPTY) ? FS_OPAQUE : style_from_fill(&DefaultFillStyle);
	pT->filled_polygon(points, icorners);
	free(icorners);
}
//
// Makes mapping from real 3D coordinates, passed as coords array,
// to 2D terminal coordinates, then draws filled polygon
//
//void filled_polygon_3dcoords(int points, const GpCoordinate & rCoords)
void GpGadgets::FilledPolygon3DCoords(GpTermEntry * pT, int points, const GpCoordinate & rCoords)
{
	FilledPolygonCommon(pT, points, rCoords, false, 0.0);
}
//
// Makes mapping from real 3D coordinates, passed as coords array, but at z GpCoordinate
// fixed (base_z, for instance) to 2D terminal coordinates, then draws filled polygon
//
//void filled_polygon_3dcoords_zfixed(int points, const GpCoordinate & rCoords, double z)
void GpGadgets::FilledPolygon3DCoords_ZFixed(GpTermEntry * pT, int points, const GpCoordinate & rCoords, double z)
{
	FilledPolygonCommon(pT, points, rCoords, true, z);
}

#endif /* PM3D_CONTOURS */

/*
   Draw colour smooth box

   Firstly two helper routines for plotting inside of the box
   for postscript and for other terminals, finally the main routine
 */
//
// plot the colour smooth box for from terminal's integer coordinates
// This routine is for postscript files --- actually, it writes a small PS routine
//
//static void draw_inside_color_smooth_box_postscript()
void GpGadgets::DrawInsideColorSmoothBoxPostscript(GpTermEntry * pT)
{
	int scale_x = (ColorBox.bounds.xright - ColorBox.bounds.xleft), scale_y = (ColorBox.bounds.ytop - ColorBox.bounds.ybot);
	fputs("stroke gsave\t%% draw gray scale smooth box\n"
	    "maxcolors 0 gt {/imax maxcolors def} {/imax 1024 def} ifelse\n", gppsfile);
	// nb. of discrete steps (counted in the loop) 
	fprintf(gppsfile, "%i %i translate %i %i scale 0 setlinewidth\n", ColorBox.bounds.xleft, ColorBox.bounds.ybot, scale_x, scale_y);
	// define left bottom corner and scale of the box so that all coordinates
	// of the box are from [0,0] up to [1,1]. Further, this normalization
	// makes it possible to pass y from [0,1] as parameter to setgray 
	fprintf(gppsfile, "/ystep 1 imax div def /y0 0 def /ii 0 def\n");
	/* local variables; y-step, current y position and counter ii;  */
	if(SmPalette.positive == SMPAL_NEGATIVE) // inverted gray for negative figure 
		fputs("{ 0.99999 y0 sub g ", gppsfile); // 1 > x > 1-1.0/1024 
	else
		fputs("{ y0 g ", gppsfile);
	if(ColorBox.rotation == 'v')
		fputs("0 y0 N 1 0 V 0 ystep V -1 0 f\n", gppsfile);
	else
		fputs("y0 0 N 0 1 V ystep 0 V 0 -1 f\n", gppsfile);
	fputs("/y0 y0 ystep add def /ii ii 1 add def\nii imax ge {exit} if } loop\ngrestore 0 setgray\n", gppsfile);
}
//
// plot a colour smooth box bounded by the terminal's integer coordinates
// [x_from,y_from] to [x_to,y_to].
// This routine is for non-postscript files, as it does an explicit loop over all thin rectangles
//
//static void draw_inside_color_smooth_box_bitmap()
void GpGadgets::DrawInsideColorSmoothBoxBitmap(GpTermEntry * pT)
{
	int steps = 128; // I think that nobody can distinguish more colours drawn in the palette 
	int i, j, xy, xy2, xy_from, xy_to;
	int jmin = 0;
	double xy_step, gray, range;
	gpiPoint corners[4];
	if(ColorBox.rotation == 'v') {
		corners[0].x = corners[3].x = ColorBox.bounds.xleft;
		corners[1].x = corners[2].x = ColorBox.bounds.xright;
		xy_from = ColorBox.bounds.ybot;
		xy_to = ColorBox.bounds.ytop;
		xy_step = (ColorBox.bounds.ytop - ColorBox.bounds.ybot) / (double)steps;
	}
	else {
		corners[0].y = corners[1].y = ColorBox.bounds.ybot;
		corners[2].y = corners[3].y = ColorBox.bounds.ytop;
		xy_from = ColorBox.bounds.xleft;
		xy_to = ColorBox.bounds.xright;
		xy_step = (ColorBox.bounds.xright - ColorBox.bounds.xleft) / (double)steps;
	}
	range = (xy_to - xy_from);
	for(i = 0, xy2 = xy_from; i < steps; i++) {
		// Start from one pixel beyond the previous box 
		xy = xy2;
		xy2 = xy_from + (int)(xy_step * (i + 1));
		// Set the colour for the next range increment 
		// FIXME - The "1 +" seems wrong, yet it improves the placement in gd 
		gray = (double)(1 + xy - xy_from) / range;
		if(SmPalette.positive == SMPAL_NEGATIVE)
			gray = 1 - gray;
		pT->SetColor(gray);

		// If this is a defined palette, make sure that the range increment 
		// does not straddle a palette segment boundary. If it does, split  
		// it into two parts.                                               
		if(SmPalette.colorMode == SMPAL_COLOR_MODE_GRADIENT)
			for(j = jmin; j<SmPalette.gradient_num; j++) {
				int boundary = xy_from + (int)(SmPalette.gradient[j].pos * range);
				if(xy >= boundary) {
					jmin = j;
				}
				else {
					if(xy2 > boundary) {
						xy2 = boundary;
						i--;
						break;
					}
				}
				if(xy2 < boundary)
					break;
			}

		if(ColorBox.rotation == 'v') {
			corners[0].y = corners[1].y = xy;
			corners[2].y = corners[3].y = MIN(xy_to, xy2+1);
		}
		else {
			corners[0].x = corners[3].x = xy;
			corners[1].x = corners[2].x = MIN(xy_to, xy2+1);
		}
#ifdef EXTENDED_COLOR_SPECS
		if((pT->flags & TERM_EXTENDED_COLOR))
			corners[0].spec.gray = -1;  // force solid color 
#endif
		// print the rectangle with the given colour 
		corners->style = (DefaultFillStyle.fillstyle == FS_EMPTY) ? FS_OPAQUE : style_from_fill(&DefaultFillStyle);
		pT->filled_polygon(4, corners);
	}
}

//static void cbtick_callback(GpTermEntry * pT, GpTicCallbackParam * pP)
void GpGadgets::CbTickCallback(GpTermEntry * pT, GpTicCallbackParam * pP)
{
	int    len = (int)(GetTicScale(pP) * ((pP->P_Ax->Flags & GpAxis::fTicIn) ? -1 : 1) * (pT->HTic));
	uint   x1, y1, x2, y2;
	double cb_place;
	// position of tic as a fraction of the full palette range 
#ifdef NONLINEAR_AXES
	if(pP->P_Ax->P_LinkToPrmr) {
		const GpAxis * p_primary = pP->P_Ax->P_LinkToPrmr;
		pP->Place = p_primary->EvalLinkFunction(pP->Place);
		cb_place = (pP->Place - p_primary->Range.low) / p_primary->Range.GetDistance();
	}
	else
#endif
		cb_place = (pP->Place - pP->P_Ax->Range.low) / pP->P_Ax->Range.GetDistance();
	// calculate tic position 
	if(ColorBox.rotation == 'h') {
		x1 = x2 = (uint)(ColorBox.bounds.xleft + cb_place * (ColorBox.bounds.xright - ColorBox.bounds.xleft));
		y1 = ColorBox.bounds.ybot;
		y2 = ColorBox.bounds.ybot - len;
	}
	else {
		x1 = ColorBox.bounds.xright;
		x2 = ColorBox.bounds.xright + len;
		y1 = y2 = (uint)(ColorBox.bounds.ybot + cb_place * (ColorBox.bounds.ytop - ColorBox.bounds.ybot));
	}
	// draw grid line 
	if(pP->Style.l_type > LT_NODRAW) {
		ApplyLpProperties(pT, &pP->Style); // grid linetype 
		if(ColorBox.rotation == 'h') {
			(*pT->move)(x1, ColorBox.bounds.ybot);
			(*pT->vector)(x1, ColorBox.bounds.ytop);
		}
		else {
			(*pT->move)(ColorBox.bounds.xleft, y1);
			(*pT->vector)(ColorBox.bounds.xright, y1);
		}
		ApplyLpProperties(pT, &BorderLp); // border linetype 
	}
	// draw tic 
	(*pT->move)(x1, y1);
	(*pT->vector)(x2, y2);
	// draw label 
	if(pP->P_Text) {
		/*int*/JUSTIFY just;
		int    offsetx;
		int    offsety;
		// Skip label if we've already written a user-specified one here 
#define MINIMUM_SEPARATION 0.001
		while(pP->P_M) {
			if(fabs((pP->Place - pP->P_M->position) / (GetCB().GetRange())) <= MINIMUM_SEPARATION) {
				pP->P_Text = NULL;
				break;
			}
			pP->P_M = pP->P_M->next;
		}
#undef MINIMUM_SEPARATION
		// get offset 
		Map3DPositionR(pP->P_Ax->ticdef.offset, &offsetx, &offsety, "cbtics");
		// User-specified different color for the tics text 
		if(pP->P_Ax->ticdef.textcolor.type != TC_DEFAULT)
			ApplyPm3DColor(pT, &pP->P_Ax->ticdef.textcolor);
		if(ColorBox.rotation == 'h') {
			int y3 = ColorBox.bounds.ybot - (pT->VChr);
			int hrotate = 0;
			if(pP->P_Ax->tic_rotate && (*pT->text_angle)(pP->P_Ax->tic_rotate))
				hrotate = pP->P_Ax->tic_rotate;
			if(len > 0) 
				y3 -= len; // add outer tics len 
			SETMAX(y3, 0);
			just = hrotate ? LEFT : CENTRE;
			if(pP->P_Ax->Flags & GpAxis::fManualJustify)
				just = pP->P_Ax->label.pos;
			pT->DrawMultiline(x2+offsetx, y3+offsety, pP->P_Text, just, JUST_CENTRE, hrotate, pP->P_Ax->ticdef.font);
			if(hrotate)
				(*pT->text_angle)(0);
		}
		else {
			uint x3 = ColorBox.bounds.xright + (pT->HChr);
			if(len > 0) 
				x3 += len;  /* add outer tics len */
			just = LEFT;
			if(pP->P_Ax->Flags & GpAxis::fManualJustify)
				just = pP->P_Ax->label.pos;
			pT->DrawMultiline(x3+offsetx, y2+offsety, pP->P_Text, just, JUST_CENTRE, 0, pP->P_Ax->ticdef.font);
		}
		ApplyLpProperties(pT, &BorderLp); /* border linetype */
	}
	// draw tic on the mirror side 
	if(pP->P_Ax->ticmode & TICS_MIRROR) {
		if(ColorBox.rotation == 'h') {
			y1 = ColorBox.bounds.ytop;
			y2 = y1 + len;
		}
		else {
			x1 = ColorBox.bounds.xleft;
			x2 = x1 - len;
		}
		(*pT->move)(x1, y1);
		(*pT->vector)(x2, y2);
	}
}
//
// Finally the main colour smooth box drawing routine
//
//void draw_color_smooth_box(GpTermEntry * pT, int plot_mode)
void GpGadgets::DrawColorSmoothBox(GpTermEntry * pT, int plot_mode)
{
	if(ColorBox.where != SMCOLOR_BOX_NO && pT->filled_polygon) {
		double tmp;
		//
		// firstly, choose some good position of the color box
		//
		// user's position like that (?):
		// else {
		//   x_from = ColorBox.xlow;
		//   x_to   = ColorBox.xhigh;
		// }
		//
		if(ColorBox.where == SMCOLOR_BOX_USER) {
			if(!Is3DPlot) {
				double xtemp, ytemp;
				MapPosition(pT, &ColorBox.origin, &ColorBox.bounds.xleft, &ColorBox.bounds.ybot, "cbox");
				MapPositionR(ColorBox.size, &xtemp, &ytemp, "cbox");
				ColorBox.bounds.xright = (int)xtemp;
				ColorBox.bounds.ytop = (int)ytemp;
			}
			else if(splot_map && Is3DPlot) {
				// In map view mode we allow any GpCoordinate system for placement 
				double xtemp, ytemp;
				Map3DPositionDouble(ColorBox.origin, &xtemp, &ytemp, "cbox");
				ColorBox.bounds.xleft = (int)xtemp;
				ColorBox.bounds.ybot = (int)ytemp;
				Map3DPositionR(ColorBox.size, &ColorBox.bounds.xright, &ColorBox.bounds.ytop, "cbox");
			}
			else {
				// But in full 3D mode we only allow screen coordinates 
				ColorBox.bounds.xleft = (int)(ColorBox.origin.x * (pT->xmax) + 0.5);
				ColorBox.bounds.ybot = (int)(ColorBox.origin.y * (pT->ymax) + 0.5);
				ColorBox.bounds.xright = (int)(ColorBox.size.x * (pT->xmax-1) + 0.5);
				ColorBox.bounds.ytop = (int)(ColorBox.size.y * (pT->ymax-1) + 0.5);
			}
			ColorBox.bounds.xright += ColorBox.bounds.xleft;
			ColorBox.bounds.ytop += ColorBox.bounds.ybot;
		}
		else { // ColorBox.where == SMCOLOR_BOX_DEFAULT 
			if(plot_mode == MODE_SPLOT && !splot_map) {
				// HBB 20031215: new code.  Constants fixed to what the result
				// of the old code in default view (set view 60,30,1,1)
				// happened to be. Somebody fix them if they're not right! 
				ColorBox.bounds.xleft  = (int)(xmiddle + 0.709 * xscaler);
				ColorBox.bounds.xright = (int)(xmiddle + 0.778 * xscaler);
				ColorBox.bounds.ybot   = (int)(ymiddle - 0.147 * yscaler);
				ColorBox.bounds.ytop   = (int)(ymiddle + 0.497 * yscaler);
			}
			else if(Is3DPlot) {
				// MWS 09-Dec-05, make color box full size for splot maps
				double dx = (GetX().GetRange());
				Map3DXY(GetX().Range.upp + dx * 0.025, GetY().Range.low, base_z, &ColorBox.bounds.xleft,  &ColorBox.bounds.ybot);
				Map3DXY(GetX().Range.upp + dx * 0.075, GetY().Range.upp, ceiling_z, &ColorBox.bounds.xright, &ColorBox.bounds.ytop);
			}
			else { // 2D plot 
				GpPosition default_origin;
				default_origin.Set(graph, graph, graph, 1.025, 0, 0);
				GpPosition default_size;
				default_size.Set(graph, graph, graph, 0.05, 1.0, 0);
				double xtemp, ytemp;
				MapPosition(pT, &default_origin, &ColorBox.bounds.xleft, &ColorBox.bounds.ybot, "cbox");
				ColorBox.bounds.xleft += ColorBox.xoffset;
				MapPositionR(default_size, &xtemp, &ytemp, "cbox");
				ColorBox.bounds.xright = (int)(xtemp + ColorBox.bounds.xleft);
				ColorBox.bounds.ytop = (int)(ytemp + ColorBox.bounds.ybot);
			}
			// now corrections for outer tics
			if(ColorBox.rotation == 'v') {
				int cblen = (int)(((GetCB().Flags & GpAxis::fTicIn) ? -1 : 1) * GetCB().ticscale * (pT->HTic)); // positive for outer tics
				int ylen = (int)(((GetY().Flags & GpAxis::fTicIn) ? -1 : 1) * GetY().ticscale * (pT->HTic)); // positive for outer tics 
				if((cblen > 0) && (GetCB().ticmode & TICS_MIRROR)) {
					ColorBox.bounds.xleft += cblen;
					ColorBox.bounds.xright += cblen;
				}
				if((ylen > 0) && (AxA[FIRST_Y_AXIS].ticmode & TICS_MIRROR)) {
					ColorBox.bounds.xleft += ylen;
					ColorBox.bounds.xright += ylen;
				}
			}
		}
		if(ColorBox.bounds.ybot > ColorBox.bounds.ytop) { /* switch them */
			tmp = ColorBox.bounds.ytop;
			ColorBox.bounds.ytop = ColorBox.bounds.ybot;
			ColorBox.bounds.ybot = (int)tmp;
		}
		// The PostScript terminal has an Optimized version
		if(pT->flags & TERM_IS_POSTSCRIPT)
			DrawInsideColorSmoothBoxPostscript(pT);
		else
			DrawInsideColorSmoothBoxBitmap(pT);
		if(ColorBox.border) {
			// now make boundary around the colour box 
			if(ColorBox.border_lt_tag >= 0) {
				// user specified line type 
				lp_style_type lp = BorderLp;
				lp_use_properties(&lp, ColorBox.border_lt_tag);
				ApplyLpProperties(pT, &lp);
			}
			else {
				// black solid colour should be chosen, so it's border linetype 
				ApplyLpProperties(pT, &BorderLp);
			}
			{
				newpath(pT);
				(pT->move)(ColorBox.bounds.xleft, ColorBox.bounds.ybot);
				(pT->vector)(ColorBox.bounds.xright, ColorBox.bounds.ybot);
				(pT->vector)(ColorBox.bounds.xright, ColorBox.bounds.ytop);
				(pT->vector)(ColorBox.bounds.xleft, ColorBox.bounds.ytop);
				(pT->vector)(ColorBox.bounds.xleft, ColorBox.bounds.ybot);
				closepath(pT);
			}
			// Set line properties to some value, this also draws lines in postscript terminals.
			ApplyLpProperties(pT, &BorderLp);
		}
		// draw tics 
		if(AxA[COLOR_AXIS].ticmode) {
			ApplyLpProperties(pT, &BorderLp); /* border linetype */
			GenTics(pT, AxA[COLOR_AXIS], &GpGadgets::CbTickCallback);
		}
		// write the colour box label
		if(GetCB().label.text) {
			int x, y;
			ApplyPm3DColor(pT, &(GetCB().label.textcolor));
			if(ColorBox.rotation == 'h') {
				int len = (int)(GetCB().ticscale * ((GetCB().Flags & GpAxis::fTicIn) ? 1 : -1) * (pT->VTic));
				Map3DPositionR(GetCB().label.offset, &x, &y, "smooth_box");
				x += (ColorBox.bounds.xleft + ColorBox.bounds.xright) / 2;
	#define DEFAULT_Y_DISTANCE 1.0
				y += (int)(ColorBox.bounds.ybot + (-DEFAULT_Y_DISTANCE - 1.7) * pT->VChr);
	#undef DEFAULT_Y_DISTANCE
				if(len < 0) 
					y += len;
				if(x<0) 
					x = 0;
				if(y<0) 
					y = 0;
				//pT->DrawMultiline(x, y, GetCB().label.text, CENTRE, JUST_CENTRE, 0, GetCB().label.font);
				DrawAxisLabel(x, y, GetCB(), CENTRE, JUST_CENTRE, true);
			}
			else {
				int len = (int)(GetCB().ticscale * ((GetCB().Flags & GpAxis::fTicIn) ? -1 : 1) * (pT->HTic));
				// calculate max length of cb-tics labels 
				widest_tic_strlen = 0;
				if(GetCB().ticmode & TICS_ON_BORDER) {
					widest_tic_strlen = 0; // reset the global variable 
					GenTics(pT, AxA[COLOR_AXIS], &GpGadgets::WidestTicCallback);
				}
				Map3DPositionR(GetCB().label.offset, &x, &y, "smooth_box");
	#define DEFAULT_X_DISTANCE 0.0
				x += (int)(ColorBox.bounds.xright + (widest_tic_strlen + DEFAULT_X_DISTANCE + 1.5) * pT->HChr);
	#undef DEFAULT_X_DISTANCE
				if(len > 0) 
					x += len;
				y += (ColorBox.bounds.ybot + ColorBox.bounds.ytop) / 2;
				SETMAX(x, 0);
				SETMAX(y, 0);
				if((*pT->text_angle)(GetCB().label.rotate)) {
					//pT->DrawMultiline(x, y, GetCB().label.text, CENTRE, JUST_TOP, GetCB().label.rotate, GetCB().label.font);
					DrawAxisLabel(x, y, GetCB(), CENTRE, JUST_TOP, false);
					(*pT->text_angle)(0);
				}
				else {
					//pT->DrawMultiline(x, y, GetCB().label.text, LEFT, JUST_TOP, 0, GetCB().label.font);
					DrawAxisLabel(x, y, GetCB(), LEFT, JUST_TOP, true);
				}
			}
			reset_textcolor(&(GetCB().label.textcolor));
		}
	}
}


