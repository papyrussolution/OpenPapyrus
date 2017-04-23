/* GNUPLOT - getcolor.c */

/*[
 *
 * Petr Mikulik, December 1998 -- June 1999
 * Copyright: open source as much as possible
 *
   ]*/
#include <gnuplot.h>
#pragma hdrstop
/*
 * This file implements all the gray to color transforms except one:
 * calculate_color_from_formulae() which can be found in color.c.
 * calculate_color_from_formulae() belongs logicaly into this (getcolor.c)
 * file, but it can't be here due to linkage problems:  gnuplot_x11
 * needs a lot of code from getcolor.c but does not contain any function
 * evaluation code [and does not call calculate_color_from_formulae()].
 * This file is compiled twice:  Once as part of gnuplot and the second
 * time with -DGPLT_X11_MODE resulting in getcolor_x11.o which is linked
 * into gnuplot_x11.  With GPLT_X11_MODE defined this file does not
 * contain code for calculating colors from gray by user defined functions.
 */
#ifndef GPLT_X11_MODE
//static int calculate_color_from_formulae(double, rgb_color *);
#endif
//static void color_components_from_gray(double gray, rgb_color *color);
//static int interpolate_color_from_gray(double, rgb_color *);
static double get_max_dev(rgb_color *colors, int j, double limit);
static int is_extremum(rgb_color left, rgb_color mid, rgb_color right);
static void CMY_2_RGB(rgb_color *color);
static void CIEXYZ_2_RGB(rgb_color *color);
static void YIQ_2_RGB(rgb_color *color);
static void HSV_2_RGB(rgb_color *color);
//
// check if two palettes p1 and p2 differ significantly 
//
int palettes_differ(t_sm_palette * p1, t_sm_palette * p2)
{
	if(p1->colorMode != p2->colorMode)
		return 1;
	else if(p1->positive != p2->positive)
		return 1;
	else if(p1->cmodel != p2->cmodel)
		return 1;
	else if(p1->use_maxcolors != p2->use_maxcolors)
		return 1;
	else {
		switch(p1->colorMode) {
			case SMPAL_COLOR_MODE_NONE:
				return 0; /* ?? */
			case SMPAL_COLOR_MODE_GRAY:
				if(fabs(p1->gamma - p2->gamma) > 1e-3)
					return 1;
				break;
			case SMPAL_COLOR_MODE_RGB:
				if(p1->colorFormulae != p2->colorFormulae)
					return 1;
				if(p1->formulaR != p2->formulaR)
					return 1;
				if(p1->formulaG != p2->formulaG)
					return 1;
				if(p1->formulaB != p2->formulaB)
					return 1;
				/* if(p1->ps_allcF != p2->ps_allcF)
				return 1; */
				break;
			case SMPAL_COLOR_MODE_FUNCTIONS: // coarse check based on typed fnct definitions
				if(strcmp(p1->Afunc.definition, p2->Afunc.definition))
					return 1;
				else if(strcmp(p1->Bfunc.definition, p2->Bfunc.definition))
					return 1;
				else if(strcmp(p1->Cfunc.definition, p2->Cfunc.definition))
					return 1;
				break;
			case SMPAL_COLOR_MODE_GRADIENT: 
				{
					if(p1->gradient_num != p2->gradient_num)
						return 1;
					else {
						for(int i = 0; i<p1->gradient_num; ++i) {
							if(p1->gradient[i].pos != p2->gradient[i].pos)
								return 1;
							else if(!p1->gradient[i].col.IsEqual(p2->gradient[i].col))
								return 1;
						}
					}
				}
				break;
			case SMPAL_COLOR_MODE_CUBEHELIX:
				return 1;
				break;
		}
		return 0; // no real difference found 
	}
}

#define CONSTRAIN(x) ((x) < 0 ? 0 : ((x) > 1 ? 1 : (x)))

/*  This one takes the gradient defined in GpGg.SmPalette.gradient and
 *  returns an interpolated color for the given gray value.  It
 *  does not matter wether RGB or HSV or whatever values are stored
 *  in GpGg.SmPalette.gradient[i].col, they will simply be interpolated.
 *  Return 0 on sucess, 1 if gray outside the range covered by the
 *  gradient.  No gamma correction is done.  The user can implement
 *  gamma correction specifying more points.
 *  GpGg.SmPalette.gradient[] should be sorted acording to .pos.
 *  Returns 1 on failure and fills color with "nearest" color.
 */
//static int interpolate_color_from_gray(double gray, rgb_color * color)
int GpGadgets::InterpolateColorFromGray(double gray, rgb_color * pColor)
{
	int idx, maxidx;
	rgb_color * col1;
	rgb_color * col2;
	if(gray < 0) {
		//pColor->r = SmPalette.gradient[0].col.r;
		//pColor->g = SmPalette.gradient[0].col.g;
		//pColor->b = SmPalette.gradient[0].col.b;
		*pColor = SmPalette.gradient[0].col;
		return 1;
	}
	maxidx = SmPalette.gradient_num;
	if(gray > 1) {
		//pColor->r = SmPalette.gradient[maxidx-1].col.r;
		//pColor->g = SmPalette.gradient[maxidx-1].col.g;
		//pColor->b = SmPalette.gradient[maxidx-1].col.b;
		*pColor = SmPalette.gradient[maxidx-1].col;
		return 1;
	}
	// find index by bisecting 
	idx = 0;
	if(maxidx > 1) {
		int topidx = maxidx - 1;
		// treat idx as though it is bottom index 
		while(idx != topidx) {
			int tmpidx = (idx + topidx) / 2;
			if(SmPalette.gradient[tmpidx].pos < gray)
				idx = tmpidx + 1;  /* round up */
			else
				topidx = tmpidx;
		}
	}
	col2 = &SmPalette.gradient[idx].col;
	if(gray == SmPalette.gradient[idx].pos) {
		// exact hit 
		//pColor->r = col2->r;
		//pColor->g = col2->g;
		//pColor->b = col2->b;
		*pColor = *col2;
	}
	else {
		// linear interpolation of two colors 
		double dx = SmPalette.gradient[idx].pos - SmPalette.gradient[idx - 1].pos;
		double f = (gray - SmPalette.gradient[idx-1].pos) / dx;
		col1 = &SmPalette.gradient[idx - 1].col;
		pColor->Set((col1->R + f * (col2->R - col1->R)), (col1->G + f * (col2->G - col1->G)), (col1->B + f * (col2->B - col1->B)));
	}
	return 0;
}

#ifndef GPLT_X11_MODE
/*  Fills color with the values calculated from GpGg.SmPalette.[ABC]func
 *  The color values are clipped to [0,1] without further notice.
 *  Returns 0 or does an int_error() when function evaluatin failed.
 *  The result is not in RGB color space jet.
 */
//static int calculate_color_from_formulae(double gray, rgb_color * color)
int GpGadgets::CalculateColorFromFormulae(double gray, rgb_color * pColor)
{
	t_value v;
	double a, b, c;
#define NO_CARET (-1)
	SmPalette.Afunc.dummy_values[0].SetComplex(gray, 0.0);
	Ev.EvaluateAt(SmPalette.Afunc.at, &v);
	if(Ev.undefined)
		GpGg.IntErrorNoCaret("Undefined value first pColor during function evaluation");
	a = v.Real();
	a = CONSTRAIN(a);
	SmPalette.Bfunc.dummy_values[0].SetComplex(gray, 0.0);
	Ev.EvaluateAt(SmPalette.Bfunc.at, &v);
	if(Ev.undefined)
		GpGg.IntErrorNoCaret("Undefined value second pColor during function evaluation");
	b = v.Real();
	b = CONSTRAIN(b);
	SmPalette.Cfunc.dummy_values[0].SetComplex(gray, 0.0);
	Ev.EvaluateAt(SmPalette.Cfunc.at, &v);
	if(Ev.undefined)
		GpGg.IntErrorNoCaret("Undefined value third pColor during function evaluation");
	c = v.Real();
	c = CONSTRAIN(c);
	pColor->Set(a, b, c);
#undef NO_CARET
	return 0;
}

#endif  /* !GPLT_X11_MODE */

// Map gray in [0,1] to color components according to colorMode 
//static void color_components_from_gray(double gray, rgb_color * color)
void GpGadgets::ColorComponentsFromGray(double gray, rgb_color * pColor)
{
	if(gray < 0)
		gray = 0;
	else if(gray > 1.0)
		gray = 1.0;
	switch(SmPalette.colorMode) {
		case SMPAL_COLOR_MODE_GRAY:
		    pColor->SetGray(pow(gray, 1.0/SmPalette.gamma));
		    return; /* all done, no pColor space transformation needed  */
		case SMPAL_COLOR_MODE_RGB:
		    pColor->Set(GetColorValueFromFormula(SmPalette.formulaR, gray),
				GetColorValueFromFormula(SmPalette.formulaG, gray),
				GetColorValueFromFormula(SmPalette.formulaB, gray));
		    break;
		case SMPAL_COLOR_MODE_GRADIENT:
		    InterpolateColorFromGray(gray, pColor);
		    break;
#ifndef GPLT_X11_MODE
		case SMPAL_COLOR_MODE_FUNCTIONS:
		    CalculateColorFromFormulae(gray, pColor);
		    break;
#endif /* !GPLT_X11_MODE */
		case SMPAL_COLOR_MODE_CUBEHELIX: {
		    double a;
		    double phi = 2. * M_PI * (SmPalette.cubehelix_start/3. +  gray * SmPalette.cubehelix_cycles);
		    if(SmPalette.gamma != 1.0)
			    gray = pow(gray, 1./SmPalette.gamma);
		    a = SmPalette.cubehelix_saturation * gray * (1.-gray) / 2.;
		    pColor->Set(gray + a * (-0.14861 * cos(phi) + 1.78277 * sin(phi)),
				gray + a * (-0.29227 * cos(phi) - 0.90649 * sin(phi)),
				gray + a * ( 1.97294 * cos(phi)));
			pColor->Constrain();
	    }
	    break;
		default:
		    fprintf(stderr, "%s:%d ooops: Unknown colorMode '%c'.\n", __FILE__, __LINE__, (char)(SmPalette.colorMode));
	}
}

/*
 *  Map a gray value in [0,1] to the corresponding RGB values in [0,1],
 *  according to the current colorMode and color space.
 *
 *  Note -- November 2003: this routine has been renamed from color_from_gray()
 *  to rgb1_from_gray() in order to more clearly distinguish structures
 *  rgb_color and rgb255_color.
 */
//void rgb1_from_gray(double gray, rgb_color * color)
void GpGadgets::RGB1FromGray(double gray, rgb_color * pColor)
{
	// get the pColor 
	ColorComponentsFromGray(gray, pColor);
	if(SmPalette.colorMode != SMPAL_COLOR_MODE_GRAY) {
		// transform to RGB if necessary 
		switch(SmPalette.cmodel) {
			case C_MODEL_RGB:
				break;
			case C_MODEL_HSV:
				HSV_2_RGB(pColor);
				break;
			case C_MODEL_CMY:
				CMY_2_RGB(pColor);
				break;
			case C_MODEL_YIQ:
				YIQ_2_RGB(pColor);
				break;
			case C_MODEL_XYZ:
				CIEXYZ_2_RGB(pColor);
				break;
			default:
				fprintf(stderr, "%s:%d ooops: Unknown pColor model '%c'\n", __FILE__, __LINE__, (char)(SmPalette.cmodel));
		}
	}
}

/*
 *  Convenience function to map R, G and B float values [0,1] to uchars [0,255].
 */
void rgb255_from_rgb1(rgb_color rgb1, rgb255_color * rgb255)
{
	rgb255->r = (uchar)(255 * rgb1.R + 0.5);
	rgb255->g = (uchar)(255 * rgb1.G + 0.5);
	rgb255->b = (uchar)(255 * rgb1.B + 0.5);
}

/*
 *  Convenience function to map gray values to R, G and B values in [0,1],
 *  limiting the resolution of sampling a continuous palette to use_maxcolors.
 *
 *  EAM Sep 2010 - Guarantee that for palettes with multiple segments
 *  (created by 'set palette defined ...') the mapped gray value is always
 *  in the appropriate segment.  This has the effect of overriding
 *  use_maxcolors if the defined palette has more segments than the nominal
 *  limit.
 */
//void rgb1maxcolors_from_gray(double gray, rgb_color * color)
void GpGadgets::RGB1MaxColorsFromGray(double gray, rgb_color * pColor)
{
	if(SmPalette.use_maxcolors != 0)
		gray = QuantizeGray(gray);
	RGB1FromGray(gray, pColor);
}

//double quantize_gray(double gray)
double GpGadgets::QuantizeGray(double gray)
{
	double qgray = floor(gray * SmPalette.use_maxcolors) / (SmPalette.use_maxcolors-1);
	if(SmPalette.colorMode == SMPAL_COLOR_MODE_GRADIENT) {
		int j;
		gradient_struct * g = SmPalette.gradient;
		double small_interval = 1. / SmPalette.use_maxcolors;
		// Backward compatibility with common case of 1 segment 
		if((SmPalette.gradient_num <= 2) && (qgray == 0))
			;
		// All palette segments are large compared to the sampling interval.
		// Simple truncation of gray is good enough.
		else if(SmPalette.smallest_gradient_interval > small_interval)
			;

		/* There is at least one palette segment that is smaller than the sampling
		 * interval. Earlier versions of quantize_gray() handled this case poorly.
		 * This version isn't perfect either.  In particular it fails to handle the
		 * converse problematic case where qgray is inside some small interval but
		 * the true gray value is not.  This causes a color from the narrow segment
		 *  to be applied incorrectly to neighoring segments as well.
		 */
		else { 
			for(j = 0; j<SmPalette.gradient_num; j++) {
				/* Does the true gray value lie in this interval? */
				if((gray >= g[j].pos) &&  (gray <  g[j+1].pos)) {
					/* See if it is so small that truncation missed it */
					if((g[j+1].pos - g[j].pos) < small_interval)
						qgray = (g[j].pos + g[j+1].pos) / 2.;
					break;
				}
			}
		}
	}
	return qgray;
}

/*
 *  Convenience function to map gray values to R, G and B values in [0,255],
 *  taking care of palette maxcolors (i.e., discrete nb of colors).
 */
void rgb255maxcolors_from_gray(double gray, rgb255_color * rgb255)
{
	rgb_color rgb1;
	GpGg.RGB1MaxColorsFromGray(gray, &rgb1);
	rgb255_from_rgb1(rgb1, rgb255);
}
/*
 *  Used by approximate_palette
 */
static double get_max_dev(rgb_color * colors, int j, double limit)
{
	double max_dev = 0.0;
	double rdev, gdev, bdev;
	double r = colors[0].R;
	double g = colors[0].G;
	double b = colors[0].B;
	int i;
	double sr = (colors[j].R - r) / j;
	double sg = (colors[j].G - g) / j;
	double sb = (colors[j].B - b) / j;
	for(i = 1; i<j; ++i) {
		double dx = i;
		rdev = fabs(sr*dx + r - colors[i].R);
		gdev = fabs(sg*dx + g - colors[i].G);
		bdev = fabs(sb*dx + b - colors[i].B);
		if(rdev > max_dev)
			max_dev = rdev;
		if(gdev > max_dev)
			max_dev = gdev;
		if(bdev > max_dev)
			max_dev = bdev;
		if(max_dev >= limit)
			break;
	}
	return max_dev;
}

/*
 *  Used by approximate_palette:  true if one color component in mid
 *  is higher (or lower) than both left and right, flase other wise
 */
static int is_extremum(rgb_color left, rgb_color mid, rgb_color right)
{
	/* mid is maximum */
	if(left.R < mid.R && mid.R > right.R)
		return 1;
	if(left.G < mid.G && mid.G > right.G)
		return 1;
	if(left.B < mid.B && mid.B > right.B)
		return 1;
	/* mid is minimum */
	if(left.R > mid.R && mid.R < right.R)
		return 1;
	if(left.G > mid.G && mid.G < right.G)
		return 1;
	if(left.B > mid.B && mid.B < right.B)
		return 1;
	return 0;
}

#define GROW_GRADIENT(n) do {						\
		if(cnt == gradient_size) {					    \
			gradient_size += (n);						\
			gradient = (gradient_struct *)realloc(gradient, gradient_size * sizeof(* gradient)); \
		}								    \
} while(0)

/*
 *  This function takes a palette and constructs a gradient which can
 *  be used to approximate the palette by linear interpolation.
 *  The palette is sampled at samples+1 points equally spaced in [0,1].
 *  From this huge gradient a much smaler one is constructed by selecting
 *  just those sampling points which still do approximate the full sampled
 *  one well enough.  allowed_deviation determines the maximum deviation
 *  of all color components which is still acceptable for the reduced
 *  gradient.  Use a sufficiently large number of samples (500 to 5000).
 *  Please free() the returned gradient after use.  samples, allowed_deviation
 *  and max_skip may be <=0 and useful defaults will be used.
 *  Most probably it's useless to approximate a gradient- or rgbformulae-
 *  palette.  Use it to build gradients from function palettes.
 */
gradient_struct * approximate_palette(t_sm_palette * palette, int samples, double allowed_deviation, int * gradient_num)
{
	int i = 0, j = 0;
	double gray = 0;
	int gradient_size = 50;
	gradient_struct * gradient;
	int colors_size = 100;
	rgb_color * colors;
	int cnt = 0;
	rgb_color color;
	double max_dev = 0.0;
	/* int maximum_j=0, extrema=0; */

	/* useful defaults */
	if(samples <= 0)
		samples = 2000;
	if(allowed_deviation <= 0)
		allowed_deviation = 0.003;

	gradient = (gradient_struct*)
	    malloc(gradient_size * sizeof(gradient_struct));
	colors = (rgb_color*)malloc(colors_size * sizeof(rgb_color));

	/* start (gray=0.0) is needed */
	cnt = 0;
	GpGg.ColorComponentsFromGray(0.0, colors + 0);
	gradient[0].pos = 0.0;
	gradient[0].col = colors[0];
	++cnt;
	GpGg.ColorComponentsFromGray(1.0 / samples, colors + 1);
	for(i = 0; i < samples; ++i) {
		for(j = 2; i + j <= samples; ++j) {
			gray = ((double)(i + j)) / samples;
			if(j == colors_size) {
				colors_size += 50;
				colors = (rgb_color *)realloc(colors, colors_size*sizeof(*colors));
			}
			GpGg.ColorComponentsFromGray(gray, colors + j);
			// test for extremum 
			if(is_extremum(colors[j - 2], colors[j - 1], colors[j])) {
				// fprintf(stderr,"Extremum at %g\n", gray); 
				// ++extrema; 
				break;
			}
			// to big deviation 
			max_dev = get_max_dev(colors, j, allowed_deviation);
			if(max_dev > allowed_deviation) {
				/* fprintf(stderr,"Control Point at %.3g\n",gray); */
				break;
			}
		}
		GROW_GRADIENT(25);

		gradient[cnt].pos = gray;
		gradient[cnt].col = colors[j - 1];
		++cnt;

		/* if(j-1 > maximum_j) maximum_j = j-1; */

		colors[0] = colors[j - 1];
		colors[1] = colors[j];
		i += j - 1;
	}
	GpGg.ColorComponentsFromGray(1.0, &color);
	GROW_GRADIENT(1);
	gradient[cnt].pos = 1.0;
	gradient[cnt].col = color;
	++cnt;

	/***********
	   fprintf(stderr,
	         "PS interpolation table: %d samples, allowed deviation %.2f%%:\n",
	         samples, 100*allowed_deviation);

	   fprintf(stderr, "  --> new size %d, %d extrema, max skip %d\n",
	         cnt, extrema, maximum_j);
	 ************/
	free(colors);
	*gradient_num = cnt;
	return gradient; /* don't forget to free() it once you'r done with it */
}

#undef GROW_GRADIENT

/*
 *  Original fixed color transformations
 */
double GetColorValueFromFormula(int formula, double x)
{
	/* the input gray x is supposed to be in interval [0,1] */
	if(formula < 0) {       /* negate the value for negative formula */
		x = 1 - x;
		formula = -formula;
	}
	switch(formula) {
		case 0:
		    return 0;
		case 1:
		    return 0.5;
		case 2:
		    return 1;
		case 3:         /* x = x */
		    break;
		case 4:
		    x = x * x;
		    break;
		case 5:
		    x = x * x * x;
		    break;
		case 6:
		    x = x * x * x * x;
		    break;
		case 7:
		    x = sqrt(x);
		    break;
		case 8:
		    x = sqrt(sqrt(x));
		    break;
		case 9:
		    x = sin(90 * x * DEG2RAD);
		    break;
		case 10:
		    x = cos(90 * x * DEG2RAD);
		    break;
		case 11:
		    x = fabs(x - 0.5);
		    break;
		case 12:
		    x = (2 * x - 1) * (2.0 * x - 1);
		    break;
		case 13:
		    x = sin(180 * x * DEG2RAD);
		    break;
		case 14:
		    x = fabs(cos(180 * x * DEG2RAD));
		    break;
		case 15:
		    x = sin(360 * x * DEG2RAD);
		    break;
		case 16:
		    x = cos(360 * x * DEG2RAD);
		    break;
		case 17:
		    x = fabs(sin(360 * x * DEG2RAD));
		    break;
		case 18:
		    x = fabs(cos(360 * x * DEG2RAD));
		    break;
		case 19:
		    x = fabs(sin(720 * x * DEG2RAD));
		    break;
		case 20:
		    x = fabs(cos(720 * x * DEG2RAD));
		    break;
		case 21:
		    x = 3 * x;
		    break;
		case 22:
		    x = 3 * x - 1;
		    break;
		case 23:
		    x = 3 * x - 2;
		    break;
		case 24:
		    x = fabs(3 * x - 1);
		    break;
		case 25:
		    x = fabs(3 * x - 2);
		    break;
		case 26:
		    x = (1.5 * x - 0.5);
		    break;
		case 27:
		    x = (1.5 * x - 1);
		    break;
		case 28:
		    x = fabs(1.5 * x - 0.5);
		    break;
		case 29:
		    x = fabs(1.5 * x - 1);
		    break;
		case 30:
		    if(x <= 0.25)
			    return 0;
		    if(x >= 0.57)
			    return 1;
		    x = x / 0.32 - 0.78125;
		    break;
		case 31:
		    if(x <= 0.42)
			    return 0;
		    if(x >= 0.92)
			    return 1;
		    x = 2 * x - 0.84;
		    break;
		case 32:
		    if(x <= 0.42)
			    x *= 4;
		    else
			    x = (x <= 0.92) ? -2 * x + 1.84 : x / 0.08 - 11.5;
		    break;
		case 33:
		    x = fabs(2 * x - 0.5);
		    break;
		case 34:
		    x = 2 * x;
		    break;
		case 35:
		    x = 2 * x - 0.5;
		    break;
		case 36:
		    x = 2 * x - 1;
		    break;
		/*
		   IMPORTANT: if any new formula is added here, then:
		   (1) its postscript counterpart must be added to the array
		   ps_math_color_formulae[] below.
		   (2) number of colours must be incremented in color.c: variable
		   GpGg.SmPalette, first item---search for "t_sm_palette GpGg.SmPalette = "
		 */
		default:
		    /* Cannot happen! */
		    FPRINTF((stderr, "gnuplot:  invalid palette rgbformula"));
		    x = 0;
	}
	if(x <= 0)
		return 0;
	if(x >= 1)
		return 1;
	return x;
}

/* Implementation of pm3dGetColorValue() in the postscript way.
   Notice that the description, i.e. the part after %, is important
   since it is used in `show pm3d' for displaying the analytical formulae.
   The postscript formulae will be expanded into lines like:
        "/cF0 {pop 0} bind def\t% 0",
        "/cF4 {dup mul} bind def\t% x^2",
 */

const char * ps_math_color_formulae[] = {
	/* /cF0  */ "pop 0", "0",
	/* /cF1  */ "pop 0.5", "0.5",
	/* /cF2  */ "pop 1", "1",
	/* /cF3  */ " ", "x",
	/* /cF4  */ "dup mul", "x^2",
	/* /cF5  */ "dup dup mul mul", "x^3",
	/* /cF6  */ "dup mul dup mul", "x^4",
	/* /cF7  */ "sqrt", "sqrt(x)",
	/* /cF8  */ "sqrt sqrt", "sqrt(sqrt(x))",
	/* /cF9  */ "90 mul sin", "sin(90x)",
	/* /cF10 */ "90 mul cos", "cos(90x)",
	/* /cF11 */ "0.5 sub abs", "|x-0.5|",
	/* /cF12 */ "2 mul 1 sub dup mul", "(2x-1)^2",
	/* /cF13 */ "180 mul sin", "sin(180x)",
	/* /cF14 */ "180 mul cos abs", "|cos(180x)|",
	/* /cF15 */ "360 mul sin", "sin(360x)",
	/* /cF16 */ "360 mul cos", "cos(360x)",
	/* /cF17 */ "360 mul sin abs", "|sin(360x)|",
	/* /cF18 */ "360 mul cos abs", "|cos(360x)|",
	/* /cF19 */ "720 mul sin abs", "|sin(720x)|",
	/* /cF20 */ "720 mul cos abs", "|cos(720x)|",
	/* /cF21 */ "3 mul", "3x",
	/* /cF22 */ "3 mul 1 sub", "3x-1",
	/* /cF23 */ "3 mul 2 sub", "3x-2",
	/* /cF24 */ "3 mul 1 sub abs", "|3x-1|",
	/* /cF25 */ "3 mul 2 sub abs", "|3x-2|",
	/* /cF26 */ "1.5 mul .5 sub", "(3x-1)/2",
	/* /cF27 */ "1.5 mul 1 sub", "(3x-2)/2",
	/* /cF28 */ "1.5 mul .5 sub abs", "|(3x-1)/2|",
	/* /cF29 */ "1.5 mul 1 sub abs", "|(3x-2)/2|",
	/* /cF30 */ "0.32 div 0.78125 sub", "x/0.32-0.78125",
	/* /cF31 */ "2 mul 0.84 sub", "2*x-0.84",
	/* /cF32 */ "dup 0.42 le {4 mul} {dup 0.92 le {-2 mul 1.84 add} {0.08 div 11.5 sub} ifelse} ifelse",
	"4x;1;-2x+1.84;x/0.08-11.5",
	/* /cF33 */ "2 mul 0.5 sub abs", "|2*x - 0.5|",
	/* /cF34 */ "2 mul", "2*x",
	/* /cF35 */ "2 mul 0.5 sub", "2*x - 0.5",
	/* /cF36 */ "2 mul 1 sub", "2*x - 1",
	"", ""
};

/*
 *  Color Conversion Algorithms
 *  taken from http://www.cs.rit.edu/~ncs/color/t_convert.html
 *  each color model should provide a conversion to RGB.
 *  RGB values are clipped to [0,1] as some colors in some
 *  models have no RGB value.
 */
static void CMY_2_RGB(rgb_color * col)
{
	/*
	double c, m, y;
	c = col->r;
	m = col->g;
	y = col->b;
	col->r = CONSTRAIN(1.0 - c);
	col->g = CONSTRAIN(1.0 - m);
	col->b = CONSTRAIN(1.0 - y);
	*/
	col->R = (1.0 - col->R);
	col->G = (1.0 - col->G);
	col->B = (1.0 - col->B);
	col->Constrain();
}

static void CIEXYZ_2_RGB(rgb_color * col)
{
	/*
	double x, y, z;
	x = col->r;
	y = col->g;
	z = col->b;
	col->r = CONSTRAIN(1.9100 * temp.r - 0.5338 * temp.g - 0.2891 * temp.b);
	col->g = CONSTRAIN(-0.9844 * temp.r + 1.9990 * temp.g - 0.0279 * temp.b);
	col->b = CONSTRAIN(0.0585 * temp.r - 0.1187 * temp.g - 0.9017 * temp.b);
	*/
	rgb_color temp(*col);
	col->Set((1.9100 * temp.R - 0.5338 * temp.G - 0.2891 * temp.B), 
		(-0.9844 * temp.R + 1.9990 * temp.G - 0.0279 * temp.B), 
		(0.0585 * temp.R - 0.1187 * temp.G - 0.9017 * temp.B));
	col->Constrain();
}

static void YIQ_2_RGB(rgb_color * col)
{
	/*
	double y = col->r;
	double i = col->g;
	double q = col->b;
	col->r = CONSTRAIN(y - 0.956 * i + 0.621 * q);
	col->g = CONSTRAIN(y - 0.272 * i - 0.647 * q);
	col->b = CONSTRAIN(y - 1.105 * i - 1.702 * q);
	*/
	rgb_color temp;
	temp = *col;
	col->Set((temp.R - 0.956 * temp.G + 0.621 * temp.B), (temp.R - 0.272 * temp.G - 0.647 * temp.B), (temp.R - 1.105 * temp.G - 1.702 * temp.B));
	col->Constrain();
}

static void HSV_2_RGB(rgb_color * col)
{
	double f, p, q, t;
	int i;
	double h = col->R;
	double s = col->G;
	double v = col->B;
	if(s == 0) { // achromatic (gray) 
		col->SetGray(v);
		return;
	}
	h *= 6.; /* h range in gnuplot is [0,1] and not the usual [0,360] */
	i = (int)floor(h);
	f = h - i;
	p = v * (1.0 - s);
	q = v * (1.0 - s*f);
	t = v * (1.0 - s*(1.0-f));
	switch(i % 6) {
		case 0: col->Set(v, t, p); break;
		case 1: col->Set(q, v, p); break;
		case 2: col->Set(p, v, t); break;
		case 3: col->Set(p, q, v); break;
		case 4: col->Set(t, p, v); break;
		default: col->Set(v, p, q); break;
	}
}

#undef CONSTRAIN

/*
 * Support for user-callable rgb = hsv2rgb(h,s,v)
 */
uint hsv2rgb(rgb_color * color)
{
	HSV_2_RGB(color);
	return ((uint)(255.0*color->R) << 16) + ((uint)(255.0*color->G) << 8) + ((uint)(255.0*color->B));
}

/* eof getcolor.c */
