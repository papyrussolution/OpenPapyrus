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
//
// Copy of palette previously in use.
// Exported so that change_term() can invalidate contents
//
static t_sm_palette prev_palette = { -1, (palette_color_mode)-1, -1, -1, -1, -1, -1, -1, (rgb_color*)0, -1 }; // @global

// Internal prototype declarations: 
//static void draw_inside_color_smooth_box_postscript();
//static void cbtick_callback(GpAxis *, double place, char * text, int ticlevel, struct lp_style_type grid, struct ticmark * userlabels);

/*static*/const uint rgb255_color::AnsiTab16[16] = {
	0xf000, 0xf00a, 0xf0a0, 0xf0aa, 0xfa00, 0xfa0a, 0xfa50, 0xfaaa,
	0xf555, 0xf55f, 0xf5f5, 0xf5ff, 0xff55, 0xff5f, 0xfff5, 0xffff,
};

//static uint nearest_ansi(rgb255_color rgb255)
uint rgb255_color::NearestAnsi() const
{
	uint best = 0;
	uint dist = 0x3fff;
	for(uint i = 0; i < 16; i++) {
		uint d = 0;
		int a = (AnsiTab16[i] >> 0) & 0xf;
		int b = (r >> 4) & 0xf;
		d += (a - b) * (a - b);

		a = (AnsiTab16[i] >> 4) & 0xf;
		b = (g >> 4) & 0xf;
		d += (a - b) * (a - b);

		a = (AnsiTab16[i] >> 8) & 0xf;
		b = (b >> 4) & 0xf;
		d += (a - b) * (a - b);

		if(d < dist) {
			dist = d;
			best = i;
		}
	}
	return best;
}

uint rgb255_color::ToAnsi256() const
{
	if((r - 8) / 10 == (b - 8) / 10 && (r - 8) / 10 == (g - 8) / 10) {
		// gray scale
		if(g < 8) // black
			return 16;
		if(g >= 238) // white
			return 231;
		return (g - 8) / 10 + 232; // like XTerm, Mintty
	}
	else {
		// 6x6x6 color cube
#define RMAPCUBE6(n) ((n >= 55) ? ((n) - 35) / 40 : 0)
		return (((uint)RMAPCUBE6(r) * 36) + ((uint)RMAPCUBE6(g) *  6) + ((uint)RMAPCUBE6(b))) + 16;
	}
}
//
// code snippet adopted from libcaca:  WTFPL license 
// RGB colours for the ANSI palette. There is no real standard, so we
// use the same values as gnome-terminal. The 7th colour (brown) is a bit
// special: 0xfa50 instead of 0xfaa0. 
// end of libcaca code 
//
//static uint to_ansi256(const rgb255_color * pColor)
//static void dumb_rgb_color(rgb255_color rgb255, char * pColorString)
void TermDumbBlock::DumbRgbColor(rgb255_color rgb255, char * pColorString)
{
	switch(ColorMode) {
		case DUMB_ANSI: 
			{
				uint color = rgb255.NearestAnsi();
				sprintf(pColorString, "\033[%i;%im", color >= 8 ? 22 : 1, 30 + (color % 8));
			}
			break;
		case DUMB_ANSI256:
		    sprintf(pColorString, "\033[38;5;%im", rgb255.ToAnsi256());
		    break;
		case DUMB_ANSIRGB:
		    sprintf(pColorString, "\033[38;2;%i;%i;%im", rgb255.r, rgb255.g, rgb255.b);
		    break;
	}
}

//const char * ansi_colorstring(const t_colorspec * pColor, const t_colorspec * pPrevColor)
const char * GnuPlot::AnsiColorString(const t_colorspec * pColor, const t_colorspec * pPrevColor)
{
	static char colorstring[256];
	colorstring[0] = NUL;
	switch(pColor->type) {
		case TC_LT: {
		    int n;
		    if(TDumbB.ColorMode < DUMB_ANSI)
			    break;
		    if(pPrevColor && pPrevColor->type == TC_LT && pPrevColor->lt == pColor->lt)
			    break;
		    n = pColor->lt + 1;
		    // map line type to colors 
		    if(n <= 0) {
			    sprintf(colorstring, "\033[0;39m"); /* normal foreground color */
		    }
		    else {
			    if(n > 15) n = ((n - 1) % 15) + 1;
			    sprintf(colorstring, "\033[%i;%im", n > 8 ? 22 : 1, 30 + (n % 8));
		    }
		    break;
	    }
		case TC_FRAC: {
		    rgb255_color rgb255;
		    if(pPrevColor && pPrevColor->type == TC_FRAC && pPrevColor->value == pColor->value)
			    break;
		    Rgb255MaxColorsFromGray(pColor->value, &rgb255);
		    TDumbB.DumbRgbColor(rgb255, colorstring);
		    break;
	    }
		case TC_RGB: {
		    rgb255_color rgb255;
		    if(pPrevColor && pPrevColor->type == TC_RGB && pPrevColor->lt == pColor->lt)
			    break;
		    rgb255.r = (pColor->lt >> 16) & 0xff;
		    rgb255.g = (pColor->lt >>  8) & 0xff;
		    rgb255.b = (pColor->lt >>  0) & 0xff;
		    TDumbB.DumbRgbColor(rgb255, colorstring);
		    break;
	    }
		default:
		    break;
	}
	return colorstring;
}

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
// Put number of allocated colours into GnuPlot::SmPltt.colors
// 
//int make_palette()
int GnuPlot::MakePalette(GpTermEntry * pTerm)
{
	if(!pTerm->make_palette)
		return 1;
	else {
		// ask for suitable number of colours in the palette 
		int i = pTerm->make_palette(pTerm, NULL);
		SmPltt.Colors = i;
		if(i == 0) {
			// terminal with its own mapping (PostScript, for instance)
			// It will not change palette passed below, but non-NULL has to be
			// passed there to create the header or force its initialization
			if(memcmp(&prev_palette, &SmPltt, sizeof(t_sm_palette))) {
				pTerm->make_palette(pTerm, &SmPltt);
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
				if(_Plt.interactive)
					fprintf(stderr, "smooth palette in %s: using %i of %i available color positions\n", pTerm->name, SmPltt.Colors, i);
			}
			prev_palette = SmPltt;
			ZFREE(SmPltt.P_Color);
			SmPltt.P_Color = (rgb_color *)SAlloc::M(SmPltt.Colors * sizeof(rgb_color));
			// fill SmPltt.color[]  
			for(i = 0; i < SmPltt.Colors; i++) {
				const double gray = (double)i / (SmPltt.Colors - 1); /* rescale to [0;1] */
				Rgb1FromGray(gray, &(SmPltt.P_Color[i]) );
			}
			// let the terminal make the palette from the supplied RGB triplets 
			pTerm->make_palette(pTerm, &SmPltt);
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
void set_color(GpTermEntry * pTerm, double gray)
{
	t_colorspec color(isnan(gray) ? TC_LT : TC_FRAC, LT_BACKGROUND, gray);
	//color.value = gray;
	//color.lt = LT_BACKGROUND;
	//color.type = (isnan(gray)) ? TC_LT : TC_FRAC;
	pTerm->set_color(pTerm, &color);
}

//void set_rgbcolor_var(uint rgbvalue)
void GnuPlot::SetRgbColorVar(GpTermEntry * pTerm, uint rgbvalue)
{
	t_colorspec color(TC_RGB, static_cast<int>(rgbvalue), -1.0/* -1 flags that this came from "rgb variable" */);
	//color.type = TC_RGB;
	//*(uint*)(&color.lt) = rgbvalue;
	//color.value = -1; /* -1 flags that this came from "rgb variable" */
	ApplyPm3DColor(pTerm, &color);
}

//void set_rgbcolor_const(uint rgbvalue)
void GnuPlot::SetRgbColorConst(GpTermEntry * pTerm, uint rgbvalue)
{
	t_colorspec color(TC_RGB, static_cast<int>(rgbvalue), 0.0/* 0 flags that this is a constant color */);
	//color.type = TC_RGB;
	//*(uint*)(&color.lt) = rgbvalue;
	//color.value = 0; /* 0 flags that this is a constant color */
	ApplyPm3DColor(pTerm, &color);
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
// 
// plot the colour smooth box for from terminal's integer coordinates
// This routine is for postscript files --- actually, it writes a small
// PS routine.
// 
//static void draw_inside_color_smooth_box_postscript()
void GnuPlot::DrawInsideColorSmoothBoxPostScript()
{
	int scale_x = (Gg.ColorBox.bounds.xright - Gg.ColorBox.bounds.xleft), scale_y = (Gg.ColorBox.bounds.ytop - Gg.ColorBox.bounds.ybot);
	fputs("stroke gsave\t%% draw gray scale smooth box\nmaxcolors 0 gt {/imax maxcolors def} {/imax 1024 def} ifelse\n", gppsfile);
	// nb. of discrete steps (counted in the loop) 
	fprintf(gppsfile, "%i %i translate %i %i scale 0 setlinewidth\n", Gg.ColorBox.bounds.xleft, Gg.ColorBox.bounds.ybot, scale_x, scale_y);
	// define left bottom corner and scale of the box so that all coordinates
	// of the box are from [0,0] up to [1,1]. Further, this normalization
	// makes it possible to pass y from [0,1] as parameter to setgray 
	fprintf(gppsfile, "/ystep 1 imax div def /y0 0 def /ii 0 def\n");
	// local variables; y-step, current y position and counter ii;  
	if(SmPltt.Positive == SMPAL_NEGATIVE) // inverted gray for negative figure 
		fputs("{ 0.99999 y0 sub g ", gppsfile); // 1 > x > 1-1.0/1024 
	else
		fputs("{ y0 g ", gppsfile);
	if(Gg.ColorBox.rotation == 'v')
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
//static void draw_inside_colorbox_bitmap_mixed()
void GnuPlot::DrawInsideColorBoxBitmapMixed(GpTermEntry * pTerm)
{
	int i, j, xy, xy2, xy_from, xy_to;
	int jmin = 0;
	double xy_step, gray, range;
	gpiPoint corners[4];
	int steps = 128; // I think that nobody can distinguish more colours drawn in the palette
	if(SmPltt.UseMaxColors != 0) {
		steps = SmPltt.UseMaxColors;
	}
	else if(SmPltt.GradientNum > 128) {
		steps = SmPltt.GradientNum;
	}
	if(Gg.ColorBox.rotation == 'v') {
		corners[0].x = corners[3].x = Gg.ColorBox.bounds.xleft;
		corners[1].x = corners[2].x = Gg.ColorBox.bounds.xright;
		xy_from = Gg.ColorBox.bounds.ybot;
		xy_to = Gg.ColorBox.bounds.ytop;
		xy_step = (Gg.ColorBox.bounds.ytop - Gg.ColorBox.bounds.ybot) / (double)steps;
	}
	else {
		corners[0].y = corners[1].y = Gg.ColorBox.bounds.ybot;
		corners[2].y = corners[3].y = Gg.ColorBox.bounds.ytop;
		xy_from = Gg.ColorBox.bounds.xleft;
		xy_to = Gg.ColorBox.bounds.xright;
		xy_step = (Gg.ColorBox.bounds.xright - Gg.ColorBox.bounds.xleft) / (double)steps;
	}
	range = (xy_to - xy_from);
	for(i = 0, xy2 = xy_from; i < steps; i++) {
		// Start from one pixel beyond the previous box 
		xy = xy2;
		xy2 = xy_from + (int)(xy_step * (i + 1));
		// Set the colour for the next range increment 
		// FIXME - The "1 +" seems wrong, yet it improves the placement in gd 
		gray = (double)(1 + xy - xy_from) / range;
		if(SmPltt.Positive == SMPAL_NEGATIVE)
			gray = 1 - gray;
		set_color(pTerm, gray);
		// If this is a defined palette, make sure that the range increment 
		// does not straddle a palette segment boundary. If it does, split  
		// it into two parts.                                               
		if(SmPltt.colorMode == SMPAL_COLOR_MODE_GRADIENT) {
			for(j = jmin; j < SmPltt.GradientNum; j++) {
				const int boundary = xy_from + (int)(SmPltt.P_Gradient[j].pos * range);
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
		if(Gg.ColorBox.rotation == 'v') {
			corners[0].y = corners[1].y = xy;
			corners[2].y = corners[3].y = MIN(xy_to, xy2+1);
		}
		else {
			corners[0].x = corners[3].x = xy;
			corners[1].x = corners[2].x = MIN(xy_to, xy2+1);
		}
		// print the rectangle with the given colour 
		if(Gg.default_fillstyle.fillstyle == FS_EMPTY)
			corners->style = FS_OPAQUE;
		else
			corners->style = style_from_fill(&Gg.default_fillstyle);
		pTerm->filled_polygon(pTerm, 4, corners);
	}
}
// 
// plot a colour smooth box bounded by the terminal's integer coordinates
// [x_from,y_from] to [x_to,y_to].
// This routine is for non-postscript files and for the Discrete color gradient type
// 
//static void draw_inside_colorbox_bitmap_discrete()
void GnuPlot::DrawInsideColorBoxBitmapDiscrete(GpTermEntry * pTerm)
{
	int i, i0, i1, xy, xy2, xy_from, xy_to;
	double gray, range;
	gpiPoint corners[4];
	int steps = SmPltt.GradientNum;
	if(Gg.ColorBox.rotation == 'v') {
		corners[0].x = corners[3].x = Gg.ColorBox.bounds.xleft;
		corners[1].x = corners[2].x = Gg.ColorBox.bounds.xright;
		xy_from = Gg.ColorBox.bounds.ybot;
		xy_to = Gg.ColorBox.bounds.ytop;
	}
	else {
		corners[0].y = corners[1].y = Gg.ColorBox.bounds.ybot;
		corners[2].y = corners[3].y = Gg.ColorBox.bounds.ytop;
		xy_from = Gg.ColorBox.bounds.xleft;
		xy_to = Gg.ColorBox.bounds.xright;
	}
	range = (xy_to - xy_from);
	for(i = 0; i < steps-1; i++) {
		if(SmPltt.Positive == SMPAL_NEGATIVE) {
			i0 = steps-1 - i;
			i1 = i0 - 1;
		}
		else {
			i0 = i;
			i1 = i0 + 1;
		}
		xy  = xy_from + (int)(SmPltt.P_Gradient[i0].pos * range);
		xy2 = xy_from + (int)(SmPltt.P_Gradient[i1].pos * range);
		if(xy2 - xy == 0) {
			continue;
		}
		gray = SmPltt.P_Gradient[i1].pos;
		set_color(pTerm, gray);
		if(Gg.ColorBox.rotation == 'v') {
			corners[0].y = corners[1].y = xy;
			corners[2].y = corners[3].y = MIN(xy_to, xy2+1);
		}
		else {
			corners[0].x = corners[3].x = xy;
			corners[1].x = corners[2].x = MIN(xy_to, xy2+1);
		}
		// print the rectangle with the given colour 
		if(Gg.default_fillstyle.fillstyle == FS_EMPTY)
			corners->style = FS_OPAQUE;
		else
			corners->style = style_from_fill(&Gg.default_fillstyle);
		pTerm->filled_polygon(pTerm, 4, corners);
	}
}
// 
// plot a colour smooth box bounded by the terminal's integer coordinates [x_from,y_from] to [x_to,y_to].
// This routine is for non-postscript files and for the Smooth color gradient type
// 
//static void draw_inside_colorbox_bitmap_smooth()
void GnuPlot::DrawInsideColorBoxBitmapSmooth(GpTermEntry * pTerm)
{
	int i, xy, xy2, xy_from, xy_to;
	double xy_step, gray;
	gpiPoint corners[4];
	// Determins the steps for rectangles boxes from palette's color number specification. 
	int steps = 128; /* I think that nobody can distinguish more colours drawn in the palette */
	if(SmPltt.UseMaxColors != 0)
		steps = SmPltt.UseMaxColors;
	else if(SmPltt.GradientNum > 128)
		steps = SmPltt.GradientNum;
	if(Gg.ColorBox.rotation == 'v') {
		corners[0].x = corners[3].x = Gg.ColorBox.bounds.xleft;
		corners[1].x = corners[2].x = Gg.ColorBox.bounds.xright;
		xy_from = Gg.ColorBox.bounds.ybot;
		xy_to = Gg.ColorBox.bounds.ytop;
		xy_step = (Gg.ColorBox.bounds.ytop - Gg.ColorBox.bounds.ybot) / (double)steps;
	}
	else {
		corners[0].y = corners[1].y = Gg.ColorBox.bounds.ybot;
		corners[2].y = corners[3].y = Gg.ColorBox.bounds.ytop;
		xy_from = Gg.ColorBox.bounds.xleft;
		xy_to = Gg.ColorBox.bounds.xright;
		xy_step = (Gg.ColorBox.bounds.xright - Gg.ColorBox.bounds.xleft) / (double)steps;
	}
	for(i = 0, xy2 = xy_from; i < steps; i++) {
		xy = xy2;
		xy2 = xy_from + (int)(xy_step * (i + 1));
		gray = i / (double)steps;
		if(SmPltt.UseMaxColors != 0) {
			gray = QuantizeGray(gray);
		}
		if(SmPltt.Positive == SMPAL_NEGATIVE)
			gray = 1 - gray;
		set_color(pTerm, gray);
		if(Gg.ColorBox.rotation == 'v') {
			corners[0].y = corners[1].y = xy;
			corners[2].y = corners[3].y = MIN(xy_to, xy2+1);
		}
		else {
			corners[0].x = corners[3].x = xy;
			corners[1].x = corners[2].x = MIN(xy_to, xy2+1);
		}
		// print the rectangle with the given colour 
		corners->style = (Gg.default_fillstyle.fillstyle == FS_EMPTY) ? FS_OPAQUE : style_from_fill(&Gg.default_fillstyle);
		pTerm->filled_polygon(pTerm, 4, corners);
	}
}

//static void cbtick_callback(GpAxis * pAx, double place, char * text, int ticlevel, lp_style_type grid/* linetype or -2 for no grid */, ticmark * userlabels)
void GnuPlot::CbTickCallback(GpTermEntry * pTerm, GpAxis * pAx, double place, char * text, int ticlevel, const lp_style_type & rGrid/* linetype or -2 for no grid */, ticmark * userlabels)
{
	int len = static_cast<int>(tic_scale(ticlevel, pAx) * (pAx->TicIn ? -1 : 1) * (pTerm->TicH));
	uint x1, y1, x2, y2;
	double cb_place;
	// position of tic as a fraction of the full palette range 
	if(pAx->linked_to_primary) {
		const GpAxis * primary = pAx->linked_to_primary;
		place = EvalLinkFunction(primary, place);
		cb_place = (place - primary->min) / primary->GetRange();
	}
	else
		cb_place = (place - pAx->min) / pAx->GetRange();
	// calculate tic position 
	if(Gg.ColorBox.rotation == 'h') {
		x1 = x2 = static_cast<uint>(Gg.ColorBox.bounds.xleft + cb_place * (Gg.ColorBox.bounds.xright - Gg.ColorBox.bounds.xleft));
		y1 = Gg.ColorBox.bounds.ybot;
		y2 = Gg.ColorBox.bounds.ybot - len;
	}
	else {
		x1 = Gg.ColorBox.bounds.xright;
		x2 = Gg.ColorBox.bounds.xright + len;
		y1 = y2 = static_cast<uint>(Gg.ColorBox.bounds.ybot + cb_place * (Gg.ColorBox.bounds.ytop - Gg.ColorBox.bounds.ybot));
	}
	// draw grid line 
	if(rGrid.l_type > LT_NODRAW) {
		TermApplyLpProperties(pTerm, &rGrid); // grid linetype 
		if(Gg.ColorBox.rotation == 'h') {
			pTerm->move(pTerm, x1, Gg.ColorBox.bounds.ybot);
			pTerm->vector(pTerm, x1, Gg.ColorBox.bounds.ytop);
		}
		else {
			pTerm->move(pTerm, Gg.ColorBox.bounds.xleft, y1);
			pTerm->vector(pTerm, Gg.ColorBox.bounds.xright, y1);
		}
		TermApplyLpProperties(pTerm, &Gg.border_lp); /* border linetype */
	}
	// draw tic 
	if(len != 0) {
		int lt = Gg.ColorBox.cbtics_lt_tag;
		if(lt <= 0)
			lt = Gg.ColorBox.border_lt_tag;
		if(lt > 0) {
			lp_style_type lp = Gg.border_lp;
			LpUseProperties(pTerm, &lp, lt);
			TermApplyLpProperties(pTerm, &lp);
		}
		pTerm->move(pTerm, x1, y1);
		pTerm->vector(pTerm, x2, y2);
		if(pAx->ticmode & TICS_MIRROR) {
			if(Gg.ColorBox.rotation == 'h') {
				y1 = Gg.ColorBox.bounds.ytop;
				y2 = Gg.ColorBox.bounds.ytop + len;
			}
			else {
				x1 = Gg.ColorBox.bounds.xleft;
				x2 = Gg.ColorBox.bounds.xleft - len;
			}
			pTerm->move(pTerm, x1, y1);
			pTerm->vector(pTerm, x2, y2);
		}
		if(lt != 0)
			TermApplyLpProperties(pTerm, &Gg.border_lp);
	}
	// draw label 
	if(text) {
		JUSTIFY just;
		int offsetx, offsety;
		// Skip label if we've already written a user-specified one here 
#define MINIMUM_SEPARATION 0.001
		while(userlabels) {
			if(fabs((place - userlabels->position) / AxS.__CB().GetRange()) <= MINIMUM_SEPARATION) {
				text = NULL;
				break;
			}
			userlabels = userlabels->next;
		}
#undef MINIMUM_SEPARATION
		// get offset 
		Map3DPositionR(pTerm, &pAx->ticdef.offset, &offsetx, &offsety, "cbtics");
		// User-specified different color for the tics text 
		if(pAx->ticdef.textcolor.type != TC_DEFAULT)
			ApplyPm3DColor(pTerm, &(pAx->ticdef.textcolor));
		if(Gg.ColorBox.rotation == 'h') {
			int y3 = Gg.ColorBox.bounds.ybot - (pTerm->ChrV);
			int hrotate = 0;
			if(pAx->tic_rotate && pTerm->text_angle(pTerm, pAx->tic_rotate))
				hrotate = pAx->tic_rotate;
			if(len > 0) y3 -= len; /* add outer tics len */
			if(y3<0) y3 = 0;
			just = hrotate ? LEFT : CENTRE;
			if(pAx->manual_justify)
				just = pAx->tic_pos;
			write_multiline(pTerm, x2+offsetx, y3+offsety, text, just, JUST_CENTRE, hrotate, pAx->ticdef.font);
			if(hrotate)
				pTerm->text_angle(pTerm, 0);
		}
		else {
			uint x3 = Gg.ColorBox.bounds.xright + (pTerm->ChrH);
			if(len > 0) 
				x3 += len; // add outer tics len 
			just = LEFT;
			if(pAx->manual_justify)
				just = pAx->tic_pos;
			write_multiline(pTerm, x3+offsetx, y2+offsety, text, just, JUST_CENTRE, 0, pAx->ticdef.font);
		}
		TermApplyLpProperties(pTerm, &Gg.border_lp); // border linetype 
	}
}
// 
// Finally the main colour smooth box drawing routine
// 
//void draw_color_smooth_box(GpTermEntry * pTerm, int plot_mode)
void GnuPlot::DrawColorSmoothBox(GpTermEntry * pTerm, int plotMode)
{
	if(Gg.ColorBox.where == SMCOLOR_BOX_NO)
		return;
	if(!pTerm->filled_polygon)
		return;
	// 
	// firstly, choose some good position of the color box
	// 
	// user's position like that (?):
	// else {
	// x_from = Gg.ColorBox.xlow;
	// x_to   = Gg.ColorBox.xhigh;
	// }
	// 
	if(Gg.ColorBox.where == SMCOLOR_BOX_USER) {
		if(!Gg.Is3DPlot) {
			double xtemp, ytemp;
			MapPosition(pTerm, &Gg.ColorBox.origin, &Gg.ColorBox.bounds.xleft, &Gg.ColorBox.bounds.ybot, "cbox");
			MapPositionR(pTerm, &Gg.ColorBox.size, &xtemp, &ytemp, "cbox");
			Gg.ColorBox.bounds.xright = static_cast<int>(xtemp);
			Gg.ColorBox.bounds.ytop = static_cast<int>(ytemp);
		}
		else if(_3DBlk.splot_map && Gg.Is3DPlot) {
			// In map view mode we allow any coordinate system for placement 
			double xtemp, ytemp;
			Map3DPositionDouble(pTerm, &Gg.ColorBox.origin, &xtemp, &ytemp, "cbox");
			Gg.ColorBox.bounds.xleft = static_cast<int>(xtemp);
			Gg.ColorBox.bounds.ybot = static_cast<int>(ytemp);
			Map3DPositionR(pTerm, &Gg.ColorBox.size, &Gg.ColorBox.bounds.xright, &Gg.ColorBox.bounds.ytop, "cbox");
		}
		else {
			// But in full 3D mode we only allow screen coordinates 
			Gg.ColorBox.bounds.xleft = static_cast<int>(Gg.ColorBox.origin.x * (pTerm->MaxX) + 0.5);
			Gg.ColorBox.bounds.ybot = static_cast<int>(Gg.ColorBox.origin.y * (pTerm->MaxY) + 0.5);
			Gg.ColorBox.bounds.xright = static_cast<int>(Gg.ColorBox.size.x * (pTerm->MaxX-1) + 0.5);
			Gg.ColorBox.bounds.ytop = static_cast<int>(Gg.ColorBox.size.y * (pTerm->MaxY-1) + 0.5);
		}
		Gg.ColorBox.bounds.xright += Gg.ColorBox.bounds.xleft;
		Gg.ColorBox.bounds.ytop += Gg.ColorBox.bounds.ybot;
	}
	else { // Gg.ColorBox.where == SMCOLOR_BOX_DEFAULT 
		if(plotMode == MODE_SPLOT && !_3DBlk.splot_map) {
			// general 3D plot 
			Gg.ColorBox.bounds.xleft  = static_cast<int>(_3DBlk.Middle.x + 0.709 * _3DBlk.Scaler.x);
			Gg.ColorBox.bounds.xright = static_cast<int>(_3DBlk.Middle.x + 0.778 * _3DBlk.Scaler.x);
			Gg.ColorBox.bounds.ybot   = static_cast<int>(_3DBlk.Middle.y - 0.147 * _3DBlk.Scaler.y);
			Gg.ColorBox.bounds.ytop   = static_cast<int>(_3DBlk.Middle.y + 0.497 * _3DBlk.Scaler.y);
		}
		else {
			// 2D plot (including splot map) 
			GpPosition default_origin = {graph, graph, graph, 1.025, 0, 0};
			GpPosition default_size = {graph, graph, graph, 0.05, 1.0, 0};
			double xtemp, ytemp;
			MapPosition(pTerm, &default_origin, &Gg.ColorBox.bounds.xleft, &Gg.ColorBox.bounds.ybot, "cbox");
			Gg.ColorBox.bounds.xleft += Gg.ColorBox.xoffset;
			MapPositionR(pTerm, &default_size, &xtemp, &ytemp, "cbox");
			Gg.ColorBox.bounds.xright = static_cast<int>(xtemp + Gg.ColorBox.bounds.xleft);
			Gg.ColorBox.bounds.ytop = static_cast<int>(ytemp + Gg.ColorBox.bounds.ybot);
		}
		// now corrections for outer tics 
		if(Gg.ColorBox.rotation == 'v') {
			int cblen = static_cast<int>((AxS.__CB().TicIn ? -1 : 1) * AxS.__CB().ticscale * (pTerm->TicH)); // positive for outer tics 
			int ylen  = static_cast<int>((AxS.__Y().TicIn ? -1 : 1) * AxS.__Y().ticscale * (pTerm->TicH)); // positive for outer tics 
			if((cblen > 0) && (AxS.__CB().ticmode & TICS_MIRROR)) {
				Gg.ColorBox.bounds.xleft += cblen;
				Gg.ColorBox.bounds.xright += cblen;
			}
			if((ylen > 0) && (AxS[FIRST_Y_AXIS].ticmode & TICS_MIRROR)) {
				Gg.ColorBox.bounds.xleft += ylen;
				Gg.ColorBox.bounds.xright += ylen;
			}
		}
	}
	ExchangeToOrder(&Gg.ColorBox.bounds.ybot, &Gg.ColorBox.bounds.ytop);
	if(Gg.ColorBox.invert && Gg.ColorBox.rotation == 'v')
		Exchange(&Gg.ColorBox.bounds.ytop, &Gg.ColorBox.bounds.ybot);
	pTerm->layer(pTerm, TERM_LAYER_BEGIN_COLORBOX);
	// The PostScript terminal has an Optimized version 
	if(pTerm->flags & TERM_IS_POSTSCRIPT)
		DrawInsideColorSmoothBoxPostScript();
	else {
		if(SmPltt.GradientType == SMPAL_GRADIENT_TYPE_SMOOTH)
			DrawInsideColorBoxBitmapSmooth(pTerm);
		else if(SmPltt.GradientType == SMPAL_GRADIENT_TYPE_DISCRETE)
			DrawInsideColorBoxBitmapDiscrete(pTerm);
		else
			DrawInsideColorBoxBitmapMixed(pTerm);
	}
	pTerm->layer(pTerm, TERM_LAYER_END_COLORBOX);
	if(Gg.ColorBox.border) {
		// now make boundary around the colour box 
		if(Gg.ColorBox.border_lt_tag >= 0) {
			// user specified line type 
			lp_style_type lp = Gg.border_lp;
			LpUseProperties(pTerm, &lp, Gg.ColorBox.border_lt_tag);
			TermApplyLpProperties(pTerm, &lp);
		}
		else
			TermApplyLpProperties(pTerm, &Gg.border_lp); // black solid colour should be chosen, so it's border linetype 
		newpath(pTerm);
		pTerm->move(pTerm, Gg.ColorBox.bounds.xleft, Gg.ColorBox.bounds.ybot);
		pTerm->vector(pTerm, Gg.ColorBox.bounds.xright, Gg.ColorBox.bounds.ybot);
		pTerm->vector(pTerm, Gg.ColorBox.bounds.xright, Gg.ColorBox.bounds.ytop);
		pTerm->vector(pTerm, Gg.ColorBox.bounds.xleft, Gg.ColorBox.bounds.ytop);
		pTerm->vector(pTerm, Gg.ColorBox.bounds.xleft, Gg.ColorBox.bounds.ybot);
		closepath(pTerm);
		// Set line properties to some value, this also draws lines in postscript terminals. 
		TermApplyLpProperties(pTerm, &Gg.border_lp);
	}
	// draw tics 
	if(AxS[COLOR_AXIS].ticmode) {
		TermApplyLpProperties(pTerm, &Gg.border_lp); /* border linetype */
		GenTics(pTerm, &AxS[COLOR_AXIS], &GnuPlot::CbTickCallback);
	}
	// write the colour box label 
	if(AxS.__CB().label.text) {
		int x, y;
		int len;
		int save_rotation = AxS.__CB().label.rotate;
		ApplyPm3DColor(pTerm, &(AxS.__CB().label.textcolor));
		if(Gg.ColorBox.rotation == 'h') {
			len = static_cast<int>(AxS.__CB().ticscale * (AxS.__CB().TicIn ? 1 : -1) * (pTerm->TicV));
			x = (Gg.ColorBox.bounds.xleft + Gg.ColorBox.bounds.xright) / 2;
			y = static_cast<int>(Gg.ColorBox.bounds.ybot - 2.7 * pTerm->ChrV);
			if(len < 0) 
				y += len;
			if(AxS.__CB().label.rotate == TEXT_VERTICAL)
				AxS.__CB().label.rotate = 0;
		}
		else {
			len = static_cast<int>(AxS.__CB().ticscale * (AxS.__CB().TicIn ? -1 : 1) * (pTerm->TicH));
			// calculate max length of cb-tics labels 
			widest_tic_strlen = 0;
			if(AxS.__CB().ticmode & TICS_ON_BORDER) // Recalculate widest_tic_strlen 
				GenTics(pTerm, &AxS[COLOR_AXIS], &GnuPlot::WidestTicCallback);
			x = static_cast<int>(Gg.ColorBox.bounds.xright + (widest_tic_strlen + 1.5) * pTerm->ChrH);
			if(len > 0) 
				x += len;
			y = (Gg.ColorBox.bounds.ybot + Gg.ColorBox.bounds.ytop) / 2;
		}
		SETMAX(x, 0);
		SETMAX(y, 0);
		WriteLabel(pTerm, x, y, &(AxS.__CB().label));
		ResetTextColor(pTerm, &(AxS.__CB().label.textcolor));
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
	Pop(&v);
	Pop(&s);
	Pop(&h);
	if(h.Type == INTGR)
		color.r = h.v.int_val;
	else if(h.Type == CMPLX)
		color.r = h.v.cmplx_val.real;
	if(s.Type == INTGR)
		color.g = s.v.int_val;
	else if(s.Type == CMPLX)
		color.g = s.v.cmplx_val.real;
	if(v.Type == INTGR)
		color.b = v.v.int_val;
	else if(v.Type == CMPLX)
		color.b = v.v.cmplx_val.real;
	SETMAX(color.r, 0.0);
	SETMAX(color.g, 0.0);
	SETMAX(color.b, 0.0);
	SETMIN(color.r, 1.0);
	SETMIN(color.g, 1.0);
	SETMIN(color.b, 1.0);
	Ginteger(&result, Hsv2Rgb(&color));
	Push(&result);
}
// 
// user-callable lookup of palette color for specific z-value
// 
//void f_palette(union argument * arg)
void GnuPlot::F_Palette(union argument * arg)
{
	GpValue result;
	rgb255_color color;
	uint rgb;
	Pop(&result);
	double z = Real(&result);
	if((AxS.__CB().set_autoscale & AUTOSCALE_BOTH) && (fabs(AxS.__CB().min) >= VERYLARGE || fabs(AxS.__CB().max) >= VERYLARGE))
		IntError(NO_CARET, "palette(z) requires known cbrange");
	Rgb255MaxColorsFromGray(Cb2Gray(z), &color);
	rgb = (uint)color.r << 16 | (uint)color.g << 8 | (uint)color.b;
	Push(Ginteger(&result, rgb));
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
	Pop(&a);
	if(a.Type == STRING) {
		rgb = lookup_color_name(a.v.string_val);
		if(rgb == -2)
			rgb = 0;
		SAlloc::F(a.v.string_val);
	}
	else {
		rgb = 0;
	}
	Push(Ginteger(&a, rgb));
}
//
// A colormap can have specific min/max stored internally,
// but otherwise we use the current cbrange
//
//double map2gray(double z, const udvt_entry * pColorMap)
double GnuPlot::Map2Gray(double z, const udvt_entry * pColorMap)
{
	double cm_min, cm_max;
	get_colormap_range(pColorMap, &cm_min, &cm_max);
	double gray = (cm_min == cm_max) ? Cb2Gray(z) : ((z - cm_min) / (cm_max - cm_min));
	return gray;
}

void get_colormap_range(const udvt_entry * pColorMap, double * cm_min, double * cm_max)
{
	*cm_min = pColorMap->udv_value.v.value_array[1].v.cmplx_val.imag;
	*cm_max = pColorMap->udv_value.v.value_array[2].v.cmplx_val.imag;
}
// 
// gray is in the interval [0:1]
// colormap is an ARRAY containing a palette of 32-bit ARGB values
// 
uint rgb_from_colormap(double gray, const udvt_entry * pColorMap)
{
	const GpValue * palette = pColorMap->udv_value.v.value_array;
	int size = palette[0].v.int_val;
	uint rgb = (gray <= 0.0) ? palette[1].v.int_val : (gray >= 1.0) ? palette[size].v.int_val : palette[(int)(floor(size * gray)) + 1].v.int_val;
	return rgb;
}
// 
// Interpret the colorspec of a linetype to yield an RGB packed integer.
// This is not guaranteed to handle colorspecs that were not part of a linetype.
// 
//uint rgb_from_colorspec(t_colorspec * tc)
uint GnuPlot::RgbFromColorspec(t_colorspec * tc)
{
	double cbval;
	rgb255_color color;
	switch(tc->type) {
		case TC_DEFAULT:
		    return 0;
		case TC_RGB:
		    return tc->lt;
		case TC_Z:
		    cbval = Cb2Gray(tc->value);
		    break;
		case TC_CB:
		    cbval = (AxS.__CB().log && tc->value <= 0) ? AxS.__CB().min : tc->value;
		    cbval = Cb2Gray(cbval);
		    break;
		case TC_FRAC:
		    cbval = (SmPltt.Positive == SMPAL_POSITIVE) ?  tc->value : 1-tc->value;
		    break;
		case TC_COLORMAP:
		/* not handled but perhaps it should be? */
		default:
		    return 0; // cannot happen in a linetype 
	}
	Rgb255MaxColorsFromGray(cbval, &color);
	return (uint)color.r << 16 | (uint)color.g << 8 | (uint)color.b;
}
